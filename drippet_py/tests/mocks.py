from transport import Message, Transport

from crc import Calculator, Crc16
import constants


def serialize_key(key: int):
    return int(key).to_bytes(4, "little")


def calc_crc(res: bytearray) -> bytes:
    crc_calc = Calculator(Crc16.MODBUS.value)
    crc = int(crc_calc.checksum(res[1:])).to_bytes(2, "little")
    res.extend(crc)
    return bytes(res)


def create_addressing_frame(addr, key):
    key_bytes = serialize_key(key)
    res = bytearray(
        [
            170,  # AA
            addr,  # Address
            constants.Command.ADDRESSING.value,
            4,
        ]
    )

    res.extend(key_bytes)
    return calc_crc(res)


def create_discovery_frame(key):
    key_bytes = serialize_key(key)
    res = bytearray(
        [
            170,  # AA
            constants.ADDR_UNSET,  # Address
            constants.Command.DISCOVERY.value,
            4,
        ]
    )

    res.extend(key_bytes)
    return calc_crc(res)


def create_watering_frame(addr, duration) -> bytes:
    durations = int(duration).to_bytes(2, "little")

    res = bytearray(
        [
            170,  # AA
            addr,  # Address
            constants.Command.INIT_WATER_DURATIONS.value,
            2,
            durations[0],
            durations[1],
        ]
    )
    return calc_crc(res)


addressingBytes = bytes(
    [
        170,  # AA
        2,  # Address
        constants.Command.ADDRESSING.value,
        2,
        244,
        1,
        122,  # 7A
        252,  # FC
    ]
)
wateringBytes = bytes(
    [
        170,  # AA
        2,  # Address
        2,
        2,
        100,  # 64
        0,
        215,  # D7
        120,  # 78
    ]
)
emptyDataBytes = bytes(
    [
        170,  # AA
        0,  # Address
        0,
        0,
        113,  # 71
        192,  # C0
    ]
)

addressing_message = Message(
    address=2,
    command=constants.Command.ADDRESSING,
    data=[500],
)
watering_message = Message(
    address=2,
    command=constants.Command.INIT_WATER_DURATIONS,
    data=[100],
)
empty_message = Message(
    address=0,
    command=constants.Command.DISCOVERY,
    data=[],
)


class MockTransport(Transport):
    def __init__(self):
        self.buffer = b""

    def write(self, bytes) -> None:
        pass

    def read(self, byteCount) -> bytes:
        return self.buffer

    def in_waiting(self) -> int:
        return 1

    def connect(self) -> None:
        pass

    def set_buffer(self, buffer: bytes):
        self.buffer = buffer


# C++ implementation
#
# UartMessage addressingOutgoing{.address = 2,
#                               .command = Protocol::Command::ADDRESSING,
#                               // NOTE: The sample key is a uint32_t split into
#                               // two uint16_t (which will be 4 uint8_t)
#                               .data = Protocol::FrameDataArray{sample_key, 0},
#                               .data_length = 2};
# Protocol::FrameDataArray exampleWatering{100};
# UartMessage wateringOutgoing{.address = 2,
#                             .command = Protocol::Command::INIT_WATER_DURATIONS,
#                             .data = exampleWatering,
#                             .data_length = 1};
# UartMessage emptyDataOutgoing{.address = 0,
#                              .command = Protocol::Command::DISCOVERY,
#                              .data = Protocol::FrameDataArray{sample_key, 0},
#                              .data_length = 2};
##
