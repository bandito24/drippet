from enum import IntEnum


START_BYTE = 0xAA

MAX_NODE_COUNT = 5
NODE_HOSE_COUNT = 5
ADDR_UNSET = 5


class Command(IntEnum):
    DISCOVERY = 0
    ADDRESSING = 1
    BROADCAST = 2
    INIT_WATER_DURATIONS = 3
    ACK = 4
    STATUS = 5
    BUGGER_OFF = 6


class HeaderOrder(IntEnum):
    START_BIT = 0
    ADDRESS = 1
    COMMAND = 2
    DATA_LENGTH = 3
    HEADER_LENGTH = 4  # Strictly for reference
