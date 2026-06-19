from abc import abstractmethod, ABC

from dataclasses import dataclass, field
from typing import Optional, Callable
import serial
import constants
from crc import Calculator, Crc16
from collections import deque
import time


HEAD_ADDR = 0x00


@dataclass
class Message:
    address: int
    command: constants.Command
    data: list[int] = field(default_factory=list)

    def __str__(self):
        return f"Address: {self.address}\nCommand: {self.command.name}\nData: {self.data}\n-------\n"


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
        self.crc_calc = Calculator(Crc16.MODBUS.value)
        # NOTE: for node
        # self.self_address: Optional[int] = None
        # self.key = 500

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
            message = Message(address, command, data)
            print(message)
            print(crc_check, crc_received)
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


@dataclass
class MockNode:
    address: int
    key: int
    status: constants.NodeStatus = constants.NodeStatus.INITIALIZING

    def __str__(self):
        return (
            f"mock node: address={self.address}, key: {self.key}, status: {self.status}"
        )


class HeadUart(Uart):
    def __init__(self, uartSerial: Transport):
        Uart.__init__(self, uartSerial)
        self.mock_nodes: dict[int, MockNode] = {}
        self.addr_to_key_map: list[int] = []
        self.mock_tick = 0

    def handle_incoming_head(self, msg: Message) -> Optional[Message]:
        print(msg)
        match constants.Command(msg.command):
            case constants.Command.DISCOVERY.value:
                key = (msg.data[1] << 16) | msg.data[0]
                if key in self.mock_nodes:
                    node_addr = self.mock_nodes[key].address
                else:
                    newMock = MockNode(len(self.mock_nodes), key)
                    node_addr = newMock.address
                    self.mock_nodes[key] = newMock

                return Message(
                    address=node_addr,
                    command=constants.Command.ADDRESSING,
                    data=msg.data,
                )
            case constants.Command.ADDRESSING.value:
                # Write Same Frame Back To Confirm It was Received
                key = (msg.data[1] << 16) | msg.data[0]
                if key not in self.mock_nodes:
                    raise RuntimeError(f"key of {key} was not recognized by head")
                if msg.address != self.mock_nodes[key].address:
                    raise RuntimeError(
                        f"key address mismatch. Got addr {msg.address} for {self.mock_nodes[key]}"
                    )
                self.mock_nodes[key].status = constants.NodeStatus.READY
                self.addr_to_key_map.append(key)
                return Message(
                    address=self.mock_nodes[key].address, command=constants.Command.ACK
                )
            case constants.Command.STATUS.value:
                status = constants.NodeStatus(msg.data[0])
                print(f"Received a status of {status.name} from index {msg.address}")
            case _:
                print(f"Received Message: {msg}")
                return None

    def prompt_input(self) -> Optional[Message]:
        msg = None
        command = int(
            input(
                "select command: 0 -> discovery, 2 -> init_water_durations, 4 -> status\n"
            )
        )
        if command not in [0, 2, 4]:
            print("invalid option")
            return
        match command:
            case constants.Command.DISCOVERY.value:
                msg = Message(
                    address=constants.ADDR_UNSET,
                    command=constants.Command.DISCOVERY,
                    data=[self.mock_tick, 0],
                )
            case constants.Command.INIT_WATER_DURATIONS.value:
                if not len(self.addr_to_key_map):
                    print("no nodes connected")
                durs = int(input("Enter duration\n"))
                index = self.get_index_operation()
                msg = Message(
                    # self.mock_nodes[self.addr_to_key_map[index]].address,
                    index,
                    constants.Command.INIT_WATER_DURATIONS,
                    [durs],
                )

            case constants.Command.STATUS.value:
                if not len(self.addr_to_key_map):
                    print("no nodes connected")
                index = self.get_index_operation()
                msg = Message(
                    # self.mock_nodes[self.addr_to_key_map[index]].address,
                    index,
                    constants.Command.STATUS,
                )
        return msg

    def get_index_operation(self):
        if len(self.addr_to_key_map) > 1:
            index = int(input("enter index for durations\n"))
        else:
            index = 0

        while index >= len(self.addr_to_key_map):
            print("This node index does not exist")
            index = int(input("enter index for durations\n"))
        return index

    def poll_incoming(self) -> Optional[Message]:
        self.poll_data()
        if self.has_msg():
            message = self.messages.popleft()
            return self.handle_incoming_head(message)

    def uart_task(self):
        self.mock_tick += 1
        time.sleep(0.3)
        msg = self.poll_incoming()
        if not msg:
            msg = self.prompt_input()
        if msg:
            print(f"Outgoing message: {msg}")
            self.write_data(msg)
