import collections
from typing import Optional
import serial
import constants
from crc import Calculator, Crc16
from collections import deque

HEAD_ADDR = 0x00


class Message:
    def __init__(self, addr: int, cmd: constants.Command, data: list[int]):
        self.address = addr
        self.command = cmd
        self.data = data


class Uart:
    def __init__(self) -> None:
        self.messages: deque[Message] = deque()

        self.serial = self.connect()
        self.key = 0xAF
        self.crc_calc = Calculator(Crc16.MODBUS.value)
        self.self_address: Optional[int] = None

    def connect(self) -> serial.Serial:
        ser = serial.Serial(
            port="/dev/cu.usbserial-BG01PTMO",  # Device path
            baudrate=115200,  # Speed (9600, 115200, etc.)
            bytesize=serial.EIGHTBITS,  # Data bits
            parity=serial.PARITY_NONE,  # Parity checking
            stopbits=serial.STOPBITS_ONE,  # Stop bits
            timeout=1,  # Read timeout (seconds)
            write_timeout=1,  # Write timeout
        )

        print(f"Connected to {ser.name}")
        return ser

    def to_int(self, byte: bytes):
        return byte[0]

    type NextFrameIndex = int

    def extract_frame(self, start_index: int, buffer: bytes) -> NextFrameIndex:
        while (
            buffer[start_index] != constants.START_BYTE
            and start_index < len(buffer) - 1
        ):
            start_index += 1

        data_length = buffer[start_index + constants.HeaderOrder.DATA_LENGTH]
        frame_length = constants.HeaderOrder.HEADER_LENGTH + data_length + 2
        frame_end = start_index + frame_length
        if frame_end > len(buffer):
            raise ValueError("Frame length extends past buffer")
        address = buffer[start_index + constants.HeaderOrder.ADDRESS]
        command: constants.Command = constants.Command(
            buffer[start_index + constants.HeaderOrder.COMMAND]
        )
        data = []
        if data_length != 0:
            data_start_index = start_index + constants.HeaderOrder.HEADER_LENGTH
            data_end_index = data_start_index + data_length - 1
            for i in range(data_start_index, data_end_index - 1, 2):
                uint16 = int.from_bytes(buffer[i : i + 2], byteorder="big")
                data.append(uint16)
        crc_check = self.crc_calc.checksum(
            buffer[(start_index + 1) : (frame_end - 2)]
        )  # Skips the start bit and the 2 crc bits at the end
        crc_received = int.from_bytes(
            buffer[frame_end - 2 : frame_end], byteorder="big"
        )
        if crc_check != crc_received:
            raise ValueError("Crc16 does not match")

        message = Message(address, command, data)

        self.messages.append(message)
        return frame_end

    def write_data(self, msg: Message) -> None:
        order = constants.HeaderOrder
        bytes = bytearray(order.HEADER_LENGTH)
        bytes[order.START_BIT] = constants.START_BYTE
        bytes[order.COMMAND] = msg.command
        bytes[order.ADDRESS] = HEAD_ADDR  # Zero address is Head Node
        bytes[order.DATA_LENGTH] = len(msg.data) * 2
        for i in range(0, len(msg.data) - 1, 2):
            endian = (msg.data[i]).to_bytes(2, "big")
            bytes.extend(endian)

        crc = (self.crc_calc.checksum(bytes[1:])).to_bytes(2, "big")
        bytes.extend(crc)
        try:
            self.serial.write(bytes)
        except serial.SerialTimeoutException:
            print("Write operation timed out")
        except Exception as e:
            print(f"Unknown write error occurred {e}")

    def has_msg(self) -> bool:
        return len(self.messages) != 0

    def poll_data(self) -> None:
        if self.serial.in_waiting:
            buffer = self.serial.read(self.serial.in_waiting)
            next_begin = 0
            while next_begin < len(buffer):
                next_begin = self.extract_frame(next_begin, buffer)

    def broadcast_pairing_key(self):
        msg = Message(HEAD_ADDR, constants.Command.DISCOVERY, [self.key])
        self.write_data(msg)

    def handle_incoming(self, msg: Message):
        match constants.Command(msg.command):
            case constants.Command.ADDRESSING:
                if msg.data != self.key:
                    print(f"Addressing data key does not belong to program: {msg.data}")
                    return
                given_addr = msg.address
                print(f"Received given address of {given_addr}")
                self.self_address = given_addr
                # Write Address and Key back in the write order


def uart_task(uart: Uart):
    while True:
        try:
            if not uart.self_address:
                uart.broadcast_pairing_key()
            elif uart.has_msg():
                message = uart.messages.popleft()

        except ValueError as e:
            print(f"Location Operation Failed: {e}")
        except serial.SerialTimeoutException as e:
            print(f"Serial functionality failed: {e}")
