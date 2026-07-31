from ble.protocol import GATT


class BLEClientMock:
    def __init__(self):
        self.read_buffer: bytearray = bytearray()
        self.write_buffer: bytes = bytes()

    async def write_gatt_char(self, gatt: str, data: bytes) -> None:
        self.write_buffer = data

    async def read_gatt_char(self, gatt: str) -> bytearray:
        return self.read_buffer


# Node Durations
# Node 0: 235 min, all days
# Node 1: 210 min, Sun/Tue/Thu
NODE_DURATIONS = bytes(
    [
        0xEB,
        0x00,
        0x7F,
        0xD2,
        0x00,
        0x15,
    ]
)


# Three nodes:
# 30 min, Mon/Wed/Fri
# 120 min, every day
# 5 min, Sunday only
NODE_DURATIONS_3 = bytes(
    [
        0x1E,
        0x00,
        0x2A,
        0x78,
        0x00,
        0x7F,
        0x05,
        0x00,
        0x01,
    ]
)


# ----------------------------------------------------
# Node States
# ----------------------------------------------------

NODE_STATES = bytes(
    [
        1,  # READY
        4,  # WATERING
        1,  # READY
    ]
)

NODE_STATES_ALL = bytes(
    [
        0,  # INITIALIZING
        1,  # READY
        2,  # IN_QUEUE
        3,  # COMMAND_SENT
        4,  # WATERING
        5,  # ERR
        6,  # INVALID_TIME
        7,  # NODE_NONEXISTANT
    ]
)


# ----------------------------------------------------
# Configuration
# ----------------------------------------------------

# 20:30
# phase = Tuesday (2)
# next phase enabled
# next phase = 22:00
CONFIG_WITH_NEXT = bytes(
    [
        20,
        30,
        2,
        1,
        22,
        0,
    ]
)

# 20:30
# phase = Tuesday
# no next phase
CONFIG_NO_NEXT = bytes(
    [
        20,
        30,
        2,
        0,
        0,
        0,
    ]
)


# ----------------------------------------------------
# Events
# ----------------------------------------------------

# WRITE_NODE_DURATION
# OK
# node 3
EVENT_DURATION_OK = bytes(
    [
        1,
        0,
        3,
    ]
)

# WRITE_NODE_CYCLE
# INVALID_NODE
# node 7
EVENT_INVALID_NODE = bytes(
    [
        2,
        2,
        7,
    ]
)

# WRITE_CONF_TIME
# INVALID_TIME
EVENT_INVALID_TIME = bytes(
    [
        0,
        1,
        0,
    ]
)

# WRITE_CONF_PHASE
# OK
EVENT_PHASE_OK = bytes(
    [
        3,
        0,
        0,
    ]
)

# WRITE_CONF_TIME_PHASE
# OK
EVENT_TIME_PHASE_OK = bytes(
    [
        4,
        0,
        0,
    ]
)

# INIT_PAIRING
# OK
EVENT_PAIRING_OK = bytes(
    [
        5,
        0,
        0,
    ]
)
