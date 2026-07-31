from typing import Protocol
from constants import BleCommand
from constants import CYCLE_LEN

from dataclasses import dataclass


class BLEClient(Protocol):
    async def write_gatt_char(self, gatt: str, data: bytes) -> None: ...
    async def read_gatt_char(self, gatt: str) -> bytearray: ...


@dataclass
class Node:
    nodeIndex: int
    duration: int
    cycle: list[bool]

    def __str__(self) -> str:
        return f"Node index {self.nodeIndex}: has duration {self.duration} with cycle {self.cycle}"

    @staticmethod
    def bitmaskToCycle(mask: int) -> list[bool]:
        res: list[bool] = []
        for i in range(CYCLE_LEN):
            res.append(bool(mask & (1 << i)))
        return res

    @staticmethod
    def cycleToBitmask(cycle: list[bool]):
        if len(cycle) != CYCLE_LEN:
            raise RuntimeError(f"Cycle length for cycle bitmask is {len(cycle)}")
        res: int = 0
        for i in range(CYCLE_LEN):  # noqa: F821
            if cycle[i]:
                res = res | (1 << i)

        return res


@dataclass
class HourMin:
    hour: int
    min: int

    def __str__(self) -> str:
        return f"{self.hour}:{self.min}"


@dataclass
class SysConfig:
    sys_time: HourMin
    phase: int
    next_watering: HourMin | None

    def __str__(self) -> str:
        return f"System Config: System Time={self.sys_time}. Current Phase={self.phase}. Next Watering={self.next_watering if self.next_watering else 'Not Set'}"


@dataclass
class EventResult:
    event: BleCommand
    data: list[int]

    def __str__(self) -> str:
        return (
            f"Event of {COMMAND_NAMES[self.event.value]}: returned data of {self.data}"
        )


COMMAND_NAMES = [
    "Write Configuration Time",
    "Write Node Duration",
    "Write Node Cycle",
    "Write Configuration Phase",
    "Write Configuration Time & Phase",
    "Initialize Pairing",
]
