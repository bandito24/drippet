from abc import abstractmethod, ABC
from dataclasses import dataclass
import time
from typing import Optional
import serial
import constants
from crc import Calculator, Crc16
from collections import deque
import traceback

HEAD_ADDR = 0x00


@dataclass
class Message:
    address: int
    command: constants.Command
    data: list[int]


class Transport(ABC):
    @abstractmethod
    def write(self, bytes) -> None:
        pass

    @abstractmethod
    def read(self, byteCount) -> bytes:
        pass

    @abstractmethod
    def in_waiting(self) -> int:
        pass

    @abstractmethod
    def connect(self) -> None:
        pass


class UartSerial(Transport):
    def __init__(self, uart_port="/dev/cu.usbserial-BG01PTMO"):
        self.port = uart_port
        self.serial = self.connect()

    def connect(self) -> None:
        ser = serial.Serial(
            port=self.port,  # Device path
            baudrate=115200,  # Speed (9600, 115200, etc.)
            bytesize=serial.EIGHTBITS,  # Data bits
            parity=serial.PARITY_NONE,  # Parity checking
            stopbits=serial.STOPBITS_ONE,  # Stop bits
            timeout=1,  # Read timeout (seconds)
            write_timeout=1,  # Write timeout
        )

        print(f"Connected to {ser.name}")
        self.serial = ser

    def write(self, bytes: bytes) -> None:
        if not self.serial:
            raise RuntimeError("Serial not initialized")
        self.serial.write(bytes)

    def read(self, byteCount) -> bytes:
        if not self.serial:
            raise RuntimeError("Serial not initialized")
        return self.serial.read(byteCount)

    def in_waiting(self) -> int:
        if not self.serial:
            raise RuntimeError("Serial not initialized")
        return self.serial.in_waiting


class Uart:
    def __init__(self, uartSerial: Transport):
        self.messages: deque[Message] = deque()
        self.serial = uartSerial
        self.key = 500
        self.crc_calc = Calculator(Crc16.MODBUS.value)
        self.self_address: Optional[int] = None

        self.serial.connect()

    def connect(self) -> None:
        self.serial.connect()

    type NextFrameIndex = int

    def extract_frame(self, start_index: int, buffer: bytes) -> NextFrameIndex:
        while buffer[start_index] != constants.START_BYTE and start_index < len(buffer):
            start_index += 1
        data_length_index = start_index + constants.HeaderOrder.DATA_LENGTH
        if data_length_index > len(buffer):
            raise IndexError("invalid frame index")
        data_length = buffer[data_length_index]
        frame_length = constants.HeaderOrder.HEADER_LENGTH + data_length + 2
        frame_end = start_index + frame_length
        if frame_end > len(buffer):
            raise IndexError("invalid frame index")
        address = buffer[start_index + constants.HeaderOrder.ADDRESS]
        command: constants.Command = constants.Command(
            buffer[start_index + constants.HeaderOrder.COMMAND]
        )
        data = []
        if data_length != 0:
            data_start_index = start_index + constants.HeaderOrder.HEADER_LENGTH
            data_end_index = data_start_index + data_length
            for i in range(data_start_index, data_end_index, 2):
                uint16 = int.from_bytes(buffer[i : i + 2], byteorder="little")
                data.append(uint16)

        crc_check = self.crc_calc.checksum(
            buffer[(start_index + 1) : (frame_end - 2)]
        )  # Skips the start bit and the 2 crc bits at the end
        crc_received = int.from_bytes(
            buffer[frame_end - 2 : frame_end], byteorder="little"
        )

        if crc_check != crc_received:
            raise ValueError("Crc16 does not match")

        message = Message(address, command, data)

        self.messages.append(message)
        return frame_end

    def write_data(self, msg: Message) -> None:
        outgoing_bytes = self.prepare_write(msg)
        try:
            if not self.serial:
                raise RuntimeError("Must connect uart")
            self.serial.write(outgoing_bytes)
        except serial.SerialTimeoutException:
            print("Write operation timed out")
        except Exception as e:
            print(f"Unknown write error occurred {e}")

    def prepare_write(self, msg: Message) -> bytes:
        order = constants.HeaderOrder
        bytesArr = bytearray(order.HEADER_LENGTH)
        bytesArr[order.START_BIT] = constants.START_BYTE
        bytesArr[order.COMMAND] = msg.command
        bytesArr[order.ADDRESS] = msg.address  # Zero address is Head Node
        bytesArr[order.DATA_LENGTH] = len(msg.data) * 2
        for i in range(0, len(msg.data)):
            endian = (msg.data[i]).to_bytes(2, "little")
            bytesArr.extend(endian)

        crc = (self.crc_calc.checksum(bytesArr[1:])).to_bytes(2, "little")
        bytesArr.extend(crc)
        return bytes(bytesArr)

    def has_msg(self) -> bool:
        return len(self.messages) != 0

    def poll_data(self) -> int:
        if self.serial.in_waiting():
            buffer = self.serial.read(self.serial.in_waiting())
            next_begin = 0
            while next_begin < len(buffer):
                next_begin = self.extract_frame(next_begin, buffer)
            return next_begin
        return 0

    def broadcast_pairing_key(self):
        print("Broadcasting pariring key")
        msg = Message(HEAD_ADDR, constants.Command.DISCOVERY, [self.key])
        self.write_data(msg)

    def handle_incoming(self, msg: Message):
        match constants.Command(msg.command):
            case constants.Command.ADDRESSING:
                if msg.data[0] != self.key:
                    print(f"Addressing data key does not belong to program: {msg.data}")
                    return
                print(f"Received given address of {msg.address}")
                self.self_address = msg.address
                self.write_data(msg)
                # Write Same Frame Back To Confirm It was Received
            case _:
                print(f"Unknown incoming message of {msg.command}")


def uart_task(uart: Uart):
    while True:
        try:
            uart.poll_data()
            if uart.has_msg():
                print("message is had")
                message = uart.messages.popleft()
                uart.handle_incoming(message)
            elif not uart.self_address:
                uart.broadcast_pairing_key()
            time.sleep(2)

        except ValueError as e:
            print(f"Location Operation Failed: {e}")
        except serial.SerialTimeoutException as e:
            print(f"Serial functionality failed: {e}")

        except Exception as e:
            print("Serial functionality failed:")
            traceback.print_exc()
            return
