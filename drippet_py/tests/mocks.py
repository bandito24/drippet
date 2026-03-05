from transport import Message
import constants

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

addressing_message = Message(
    address=2,
    command=constants.Command.ADDRESSING,
    data=[500],
)
watering_message = Message(
    address=2,
    command=constants.Command.INIT_WATER_DURATIONS,
    data=[100, 200, 300, 400, 500],
)
empty_message = Message(
    address=0,
    command=constants.Command.BROADCAST,
    data=[],
)

# C++ implementation
# UartMessage addressingBytes{.address = 2,
#                               .command = Protocol::Command::ADDRESSING,
#                               .data = Protocol::FrameDataArray{500},
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
