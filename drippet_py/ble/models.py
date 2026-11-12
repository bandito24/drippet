from typing import cast
from typing import Protocol
from constants import ActionStatus, BleCommand
from constants import WEEKDAY_INDEX


from dataclasses import dataclass
from datetime import datetime


class BLEClient(Protocol):
    async def write_gatt_char(self, gatt: str, data: bytes) -> None: ...
    async def read_gatt_char(self, gatt: str) -> bytearray: ...


@dataclass
class Node:
    nodeIndex: int
    duration: int
    cycle: list[str]

    def __str__(self) -> str:
        return f"Node index {self.nodeIndex}: has duration {self.duration} with cycle {self.cycle}"

    @staticmethod
    def create_from_bytes(nodeIndex: int, bites: bytearray) -> "Node":
        if len(bites) != 3:
            raise RuntimeError("Invalid length for node creation from bytes")
        return Node(
            nodeIndex,
            int.from_bytes(bites[0:1], "little"),
            Node.bitmaskToWeekday(bites[2]),
        )

    def serialize_data(self) -> bytearray:
        return cast(
            bytearray,
            self.duration.to_bytes(2, "little")
            + bytes([Node.weekdayToBitmask(self.cycle)]),
        )

    @staticmethod
    def bitmaskToWeekday(bitmask: int) -> list[str]:
        res = []
        weekdays = list(WEEKDAY_INDEX.keys())
        for i in range(len(weekdays)):
            if (1 << i) & bitmask:
                res.append(weekdays[i])
        return res

    @staticmethod
    def weekdayToBitmask(weekdays: list[str]) -> int:
        bitmask = 0

        for item in weekdays:
            if item not in WEEKDAY_INDEX:
                raise RuntimeError("Item not in weekday index")

            bitmask |= 1 << WEEKDAY_INDEX[item]
        return bitmask


@dataclass
class HourMin:
    hour: int
    min: int

    def __str__(self) -> str:
        hour = self.hour % 12 or 12
        period = "AM" if self.hour < 12 else "PM"

        return f"{hour}:{self.min:02d} {period}"


@dataclass
class SysConfig:
    # Device clock, in the device's stored UTC offset
    sys_time: datetime
    utc_offset_min: int
    # Watering Start Time, local
    next_watering: HourMin | None

    @property
    def phase(self) -> int:
        """Cycle Day: local weekday, Sunday = 0 (matches the cycle bitmask)."""
        return (self.sys_time.weekday() + 1) % 7

    @property
    def offset_str(self) -> str:
        sign = "+" if self.utc_offset_min >= 0 else "-"
        hours, minutes = divmod(abs(self.utc_offset_min), 60)
        return f"UTC{sign}{hours:02d}:{minutes:02d}"

    def __str__(self) -> str:
        t = self.sys_time
        clock = HourMin(t.hour, t.minute)
        return (
            f"System Config: System Time={t:%a %b} {t.day} {clock} ({self.offset_str}). "
            f"Current Phase={list(WEEKDAY_INDEX.keys())[self.phase]}. "
            f"Next Watering={self.next_watering if self.next_watering else 'Not Set'}"
        )


@dataclass
class EventResult:
    event: BleCommand
    status: ActionStatus
    node_index: int | None = None

    def __init__(self, data: bytearray):
        #  if len(data) > 3:
        #      raise RuntimeError(f"Invalid Event Result Byte length of {len(data)}")
        self.event = BleCommand(data[0])
        self.status = ActionStatus(data[1])
        if len(data) > 2:
            self.node_index = data[2]

    def __str__(self) -> str:
        node_info = (
            f" Action Performed on Node Index: {self.node_index}"
            if self.node_index is not None
            else ""
        )

        return (
            f"Event of {COMMAND_NAMES[self.event.value]}: "
            f"returned data status of {self.status.name}."
            f"{node_info}"
        )


# Indexed by BleCommand value
COMMAND_NAMES = [
    "Write Configuration Time",
    "Write Node Duration",
    "Write Node Cycle",
    "Write Watering Start Time",
    "Initialize Pairing",
]
assert len(COMMAND_NAMES) == len(BleCommand)
