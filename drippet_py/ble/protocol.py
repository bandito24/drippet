import struct
from datetime import datetime, timedelta, timezone

from bleak import BleakClient
from ble.task import CommandHandler
from constants import NodeStatus, BleCommand

from .models import Node, SysConfig, HourMin, EventResult, BLEClient
from enum import StrEnum

ESP_ADDR = "1DDD0340-3563-5744-09EF-9E17AED13045"


class GATT(StrEnum):
    DURATIONS_CHAR = "00000025-1212-efde-1523-785feabcd123"
    NODE_STAT_CHAR = "01000025-1212-efde-1523-785feabcd123"
    SYS_CONF_CHAR = "02000025-1212-efde-1523-785feabcd123"
    RESPONSE_CHAR = "03000025-1212-efde-1523-785feabcd123"


class Transport:
    def __init__(self, client: BleakClient):
        self.client = client

    async def write(self, gatt: GATT, data: bytes) -> None:
        await self.client.write_gatt_char(gatt.value, data)

    async def read(self, gatt: GATT) -> bytearray:
        return await self.client.read_gatt_char(gatt.value)


def initPairing() -> bytes:
    return bytes([BleCommand.INIT_PAIRING])


def writeNodeDuration(nodeIndex: int, duration: int) -> bytes:
    return bytes([BleCommand.WRITE_NODE_DURATION, nodeIndex]) + duration.to_bytes(
        2, "little"
    )


def writeNodeCycle(nodeIndex: int, cycleBitmask: int):
    return bytes([BleCommand.WRITE_NODE_CYCLE, nodeIndex, cycleBitmask])


def read_durations(data: bytearray) -> list[Node]:
    res = []

    for i in range(0, len(data), 3):
        res.append(Node.create_from_bytes(i // 3, data[i : i + 3]))
    return res


# WRITE_CONF_TIME: [cmd, int64 UTC seconds, int16 UTC offset minutes], LE
CONF_TIME_FORMAT = "<Bqh"
MIN_UTC_OFFSET_MIN = -720
MAX_UTC_OFFSET_MIN = 840


def writeConfTime(utc_seconds: int, offset_min: int) -> bytes:
    if not MIN_UTC_OFFSET_MIN <= offset_min <= MAX_UTC_OFFSET_MIN:
        raise ValueError(f"UTC offset {offset_min} min is out of range")
    return struct.pack(
        CONF_TIME_FORMAT, BleCommand.WRITE_CONF_TIME, utc_seconds, offset_min
    )


def writePhaseStartTime(hour: int, minute: int) -> bytes:
    return bytes([BleCommand.WRITE_CONF_TIME_PHASE, hour, minute])


# READ_CONFIGURATION: int64 UTC seconds, int16 offset, water set, hour, minute
READ_CONF_FORMAT = "<qhBBB"
READ_CONF_LEN = struct.calcsize(READ_CONF_FORMAT)  # 13


def read_sys_conf(data: bytearray) -> SysConfig:
    if len(data) < READ_CONF_LEN:
        raise ValueError(
            f"Config read is {len(data)} bytes, expected {READ_CONF_LEN}"
        )
    utc_seconds, offset_min, has_water, hour, minute = struct.unpack_from(
        READ_CONF_FORMAT, data
    )
    device_tz = timezone(timedelta(minutes=offset_min))
    return SysConfig(
        sys_time=datetime.fromtimestamp(utc_seconds, device_tz),
        utc_offset_min=offset_min,
        next_watering=HourMin(hour, minute) if has_water else None,
    )


def read_node_statuses(data: bytearray) -> list[NodeStatus]:
    return [NodeStatus(x) for x in data]


def read_events(data: bytearray) -> EventResult:
    return EventResult(data)
