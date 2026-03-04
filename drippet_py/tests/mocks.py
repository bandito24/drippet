"""
This file tests the functionality or serialization, the values of the bytes array correspond to the c++ struct values found at the bottom. For Order look at constants.py.
Data and crc values are broken into two bits using little endian
"""

addressingBytes = bytes(
    [
        170,
        2,
        1,
        2,
        244,
        1,
        122,
        252,
    ]
)


wateringBytes = bytes(
    [
        170,
        2,
        3,
        10,
        100,
        0,
        200,
        0,
        44,
        1,
        144,
        1,
        244,
        1,
        175,
        68,
    ]
)


emptyDataBytes = bytes(
    [
        170,
        0,
        2,
        0,
        112,
        160,
    ]
)

# UartMessage addressingBytes{.address = 2,
#                               .command = Protocol::Command::ADDRESSING,
#                               .data = Protocol::FrameDataArray{sample_key},
#                               .data_length = 1};
# Protocol::FrameDataArray exampleWatering{100, 200, 300, 400, 500};
# UartMessage wateringBytes{.address = 2,
#                             .command = Protocol::Command::INIT_WATER_DURATIONS,
#                             .data = exampleWatering,
#                             .data_length = 5};
# UartMessage emptyDataBytes{.address = 0,
#                              .command = Protocol::Command::BROADCAST,
#                              .data = Protocol::FrameDataArray{},
#                              .data_length = 0};
