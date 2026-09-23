from bleak import BleakClient
from ble.task import CommandHandler
from constants import NodeStatus, BleCommand

from constants import CYCLE_LEN
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


def writeConfTime(hour: int, minute: int) -> bytes:
    return bytes([BleCommand.WRITE_CONF_TIME, hour, minute])


def writePhaseStartTime(hour: int, minute: int) -> bytes:
    return bytes([BleCommand.WRITE_CONF_TIME_PHASE, hour, minute])


def writeConfPhase(currentPhase: int) -> bytes:
    if currentPhase >= CYCLE_LEN:
        raise RuntimeError(f"Invalid phase request of {currentPhase}")
    return bytes([BleCommand.WRITE_CONF_PHASE, currentPhase])


def read_sys_conf(data: bytearray) -> SysConfig:
    return SysConfig(
        sys_time=HourMin(data[0], data[1]),
        phase=data[2],
        next_watering=(HourMin(data[4], data[5]) if data[3] else None),
    )


def read_node_statuses(data: bytearray) -> list[NodeStatus]:
    return [NodeStatus(x) for x in data]


def read_events(data: bytearray) -> EventResult:
    return EventResult(data)
