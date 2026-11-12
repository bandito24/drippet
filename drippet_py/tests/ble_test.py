from _pytest.fixtures import pytest_fixture_setup
import pytest
from ble import Node, protocol
from ble.models import HourMin, SysConfig
from datetime import datetime, timedelta, timezone
import time

from ble.task import (
    WEEKDAY_INDEX,
    CommandHandler,
    clock_drift_warning,
    current_utc_and_offset,
    mock_local_time,
    utc_and_offset,
)
from .ble_mocks import Mocks


"""Fixtures related to the input handler"""


@pytest.fixture
def handler():
    return CommandHandler()


@pytest.fixture
def example_weekdays():
    return list(WEEKDAY_INDEX.keys())[2:]


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


# 2026-09-28 20:30:00 UTC, offset -07:00 (the docs/ble_writes.md example)
EX_TS = 1790627400
PDT = -420
PDT_TZ = timezone(timedelta(minutes=PDT))


def testWriteSetTime(handler: CommandHandler):
    res = handler.handle_write_sys_time(EX_TS, PDT)
    assert res == Mocks.write_conf_time(EX_TS, PDT)
    assert res == bytes.fromhex("00 48CEBA6A00000000 5CFE")
    assert len(res) == 11


def testWriteSetTimeRejectsBadOffset(handler: CommandHandler):
    with pytest.raises(ValueError):
        handler.handle_write_sys_time(EX_TS, 841)
    with pytest.raises(ValueError):
        handler.handle_write_sys_time(EX_TS, -721)


def testWriteSetPhaseTime(handler: CommandHandler):
    assert Mocks.write_conf_time_phase(12, 12) == handler.handle_write_phase_time(
        12, 12
    )


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
    config1 = handler.handle_read_config(Mocks.read_config(EX_TS, PDT, (6, 5)))
    config2 = handler.handle_read_config(Mocks.read_config(EX_TS, PDT, None))
    local = datetime(2026, 9, 28, 13, 30, tzinfo=PDT_TZ)

    assert config1 == SysConfig(local, PDT, HourMin(6, 5))
    assert config2 == SysConfig(local, PDT, None)
    assert config1.sys_time.hour == 13  # local, not UTC
    assert config1.phase == WEEKDAY_INDEX["Monday"]
    assert str(config1).startswith(
        "System Config: System Time=Mon Sep 28 1:30 PM (UTC-07:00)."
    )


def testReadConfigRejectsShortPacket(handler: CommandHandler):
    with pytest.raises(ValueError):
        handler.handle_read_config(bytearray(7))


"""Client time helpers"""


@pytest.fixture
def los_angeles(monkeypatch):
    monkeypatch.setenv("TZ", "America/Los_Angeles")
    time.tzset()
    yield
    monkeypatch.undo()
    time.tzset()


def testUtcAndOffset():
    assert utc_and_offset(datetime(2026, 9, 28, 13, 30, tzinfo=PDT_TZ)) == (EX_TS, PDT)
    with pytest.raises(ValueError):
        utc_and_offset(datetime(2026, 9, 28, 13, 30))


def testCurrentUtcAndOffset(los_angeles):
    ts, offset = current_utc_and_offset()
    assert abs(ts - time.time()) < 5
    assert offset in (PDT, -480)


def testMockLocalTime(los_angeles):
    # Monday 2026-09-28 13:30 PDT
    now = datetime(2026, 9, 28, 13, 30).astimezone()
    same_day = mock_local_time(WEEKDAY_INDEX["Monday"], 2, 0, now)
    assert (same_day.date(), same_day.hour) == (now.date(), 2)

    wednesday = mock_local_time(WEEKDAY_INDEX["Wednesday"], 9, 15, now)
    assert wednesday.isoformat() == "2026-09-30T09:15:00-07:00"

    sunday = mock_local_time(WEEKDAY_INDEX["Sunday"], 9, 15, now)
    assert sunday.date().isoformat() == "2026-10-04"


def testMockLocalTimeUsesThatDaysOffset(los_angeles):
    # Friday 2026-10-30 PDT; Monday 2026-11-02 is after DST ends (PST)
    now = datetime(2026, 10, 30, 12, 0).astimezone()
    monday = mock_local_time(WEEKDAY_INDEX["Monday"], 2, 0, now)
    assert utc_and_offset(monday)[1] == -480


def testClockDriftWarning():
    now = datetime(2026, 9, 28, 13, 30, tzinfo=PDT_TZ)
    in_sync = SysConfig(now + timedelta(seconds=30), PDT, None)
    assert clock_drift_warning(in_sync, now) is None

    drifted = SysConfig(now + timedelta(seconds=90), PDT, None)
    assert "+90 s" in (clock_drift_warning(drifted, now) or "")

    wrong_offset = SysConfig(now.astimezone(timezone(timedelta(hours=-8))), -480, None)
    assert "offset" in (clock_drift_warning(wrong_offset, now) or "")


def testEventResult(handler: CommandHandler):
    event = handler.handle_read_events(bytearray(Mocks.read_event(2, 2, 3)))
    assert event.node_index == 3
    assert "Write Node Cycle" in str(event)

    pairing = handler.handle_read_events(bytearray(Mocks.read_event(4, 0)))
    assert "Initialize Pairing" in str(pairing)
