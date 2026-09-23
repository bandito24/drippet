from _pytest.fixtures import pytest_fixture_setup
import pytest
from ble import Node, protocol
from ble.models import HourMin, SysConfig
from ble.task import WEEKDAY_INDEX, CommandHandler
from .ble_mocks import Mocks


"""Fixtures related to the input handler"""


@pytest.fixture
def handler():
    return CommandHandler()


@pytest.fixture
def example_weekdays():
    return list(WEEKDAY_INDEX.keys())[2:]


@pytest.fixture
def weekday_keys():
    return list(WEEKDAY_INDEX.keys())


"""Testing Raw Bytes From Writes"""


def testInitPairingBytes(handler: CommandHandler):
    res = handler.handle_init_pairing()
    assert res == Mocks.write_init_pairing()


def testWriteDurations(handler: CommandHandler):
    res = handler.handle_write_durations(0, 46)
    assert res == Mocks.write_node_duration(0, 46)


def testWeekdaysToBitmaskAndViceVersa(example_weekdays):
    bitmask = Node.weekdayToBitmask(example_weekdays)
    days2 = Node.bitmaskToWeekday(bitmask)
    assert example_weekdays == days2


def testWriteCycle(handler: CommandHandler, example_weekdays):
    bitmask = Node.weekdayToBitmask(example_weekdays)
    assert handler.handle_write_cycle(0, example_weekdays) == Mocks.write_node_cycle(
        0, bitmask
    )


def testNodeCreationFromBytes(example_weekdays):
    node1 = Node(0, 50, example_weekdays)
    node1_bytes = node1.serialize_data()
    node2 = Node.create_from_bytes(0, node1_bytes)
    assert node1 == node2


def testWriteSetTime(handler: CommandHandler):
    assert Mocks.write_conf_time(12, 12) == handler.handle_write_sys_time(12, 12)


def testWriteSetPhaseTime(handler: CommandHandler):
    assert Mocks.write_conf_time_phase(12, 12) == handler.handle_write_phase_time(
        12, 12
    )


def testWriteCurrPhase(handler: CommandHandler, weekday_keys: list[str]):
    assert Mocks.write_conf_phase(4) == handler.handle_write_curr_phase(weekday_keys[4])


def testReadNodeDurations(handler: CommandHandler, example_weekdays):
    values: list[Node] = []
    ser: bytearray = bytearray()
    for i in range(len(example_weekdays)):
        new_node = Node(i, i + 5 * 10, example_weekdays.copy())
        values.append(new_node)
        example_weekdays.pop()
        ser.extend(new_node.serialize_data())
    conf = handler.handle_read_durations(ser)
    assert values == conf


def testReadConfig(handler: CommandHandler):
    data = Mocks.read_config(20, 30, 5, (6, 5))
    data2 = Mocks.read_config(20, 30, 5, None)
    config1 = handler.handle_read_config(data)
    config2 = handler.handle_read_config(data2)
    assert config1 == SysConfig(HourMin(20, 30), 5, HourMin(6, 5))
    assert config2 == SysConfig(HourMin(20, 30), 5, None)
