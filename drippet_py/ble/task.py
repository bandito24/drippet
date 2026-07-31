from .models import Node
from .models import Node, SysConfig, HourMin, EventResult, BLEClient
from dataclasses import dataclass
from enum import IntEnum, auto
from rich.console import Console
from beaupy import Config, confirm, prompt, select, select_multiple
from enum import StrEnum
import traceback
import asyncio
from typing import Optional
from bleak import BleakClient
from bleak.exc import BleakCharacteristicNotFoundError
from enum import Enum
from typing import cast

from ble.models import HourMin
from constants import CYCLE_LEN, NodeStatus

from . import protocol
from typing import Protocol


class Actions(StrEnum):
    INIT_PAIRING = "Init Pairing"
    WRITE_DURATION = "Write Node Duration"
    WRITE_NODE_CYCLE = "Write Node Cycle"
    READ_DURATIONS = "Read Node Durations"

    READ_NODE_STATUS = "Read Node Statuses"
    SET_TIME = "Set Time"
    SET_PHASE_TIME = "Set Water Start"
    SET_PHASE = "Set Day Of Week"
    READ_CONFIG = "Get Config"
    EXIT = "Exit"


def prompt_int(title: str) -> int:
    return prompt(
        title,
        target_type=int,
        validator=lambda count: count > 0,
    )


WEEKDAY_INDEX: dict[str, int] = {
    "Sunday": 0,
    "Monday": 1,
    "Tuesday": 2,
    "Wednesday": 3,
    "Thursday": 4,
    "Friday": 5,
    "Saturday": 6,
}


class CommandHandler:
    def handle_init_pairing(self) -> bytes:
        return protocol.initPairing()

    def handle_write_durations(self, node_index: int, duration: int) -> bytes:
        return protocol.writeNodeDuration(node_index, duration)

    def handle_write_cycle(self, node_index: int, selections: list[str]) -> bytes:
        bitmask = 0

        for item in selections:
            if item not in WEEKDAY_INDEX:
                raise RuntimeError("Item not in weekday index")

            bitmask |= 1 << WEEKDAY_INDEX[item]

        return protocol.writeNodeCycle(node_index, bitmask)

    def handle_write_sys_time(self, hour: int, minute: int) -> bytes:
        return protocol.writeConfTime(hour, minute)

    def handle_write_phase_time(self, hour: int, minute: int) -> bytes:
        return protocol.writePhaseStartTime(hour, minute)

    def handle_write_curr_phase(self, phase: str) -> bytes:
        if phase not in WEEKDAY_INDEX:
            raise RuntimeError("Item not in weekday index")

        return protocol.writeConfPhase(WEEKDAY_INDEX[phase])

    def handle_read_durations(self, data: bytearray) -> list[Node]:
        return protocol.read_durations(data)

    def handle_read_config(self, data: bytearray) -> SysConfig:
        return protocol.read_sys_conf(data)

    def handle_read_node_status(self, data: bytearray) -> list[NodeStatus]:
        return protocol.read_node_statuses(data)


async def ble_task(client: BLEClient):
    Config.raise_on_interrupt = True
    console = Console()
    actions = [x.value for x in Actions]
    action = "start"

    handler = CommandHandler()
    transport = protocol.Transport(client)

    try:
        while action != "Exit":
            console.print("Select Action")

            action = select(actions, cursor="->", cursor_style="cyan")

            match action:
                case Actions.INIT_PAIRING.value:
                    await transport.write(
                        protocol.GATT.SYS_CONF_CHAR,
                        handler.handle_init_pairing(),
                    )

                case Actions.WRITE_DURATION.value:
                    data = handler.handle_write_durations(
                        prompt_int("Which node index?"),
                        prompt_int("Please Enter Duration"),
                    )
                    await transport.write(protocol.GATT.DURATIONS_CHAR, data)

                case Actions.WRITE_NODE_CYCLE.value:
                    node_index = prompt_int("Which node index?")

                    console.print("Which Days Do You Want To Select")
                    items = cast(
                        list[str],
                        select_multiple(
                            list(WEEKDAY_INDEX.keys()),
                            tick_character="️✅ ",
                            ticked_indices=[],
                            maximal_count=CYCLE_LEN,
                        ),
                    )

                    data = handler.handle_write_cycle(node_index, items)
                    await transport.write(protocol.GATT.DURATIONS_CHAR, data)

                case Actions.SET_TIME.value:
                    data = handler.handle_write_sys_time(
                        prompt_int("Set Hour 0-23"),
                        prompt_int("Set Minutes 0-59"),
                    )
                    await transport.write(protocol.GATT.SYS_CONF_CHAR, data)

                case Actions.SET_PHASE_TIME.value:
                    data = handler.handle_write_phase_time(
                        prompt_int("Set Hour 0-23"),
                        prompt_int("Set Minutes 0-59"),
                    )
                    await transport.write(protocol.GATT.SYS_CONF_CHAR, data)

                case Actions.SET_PHASE.value:
                    phase = cast(
                        str,
                        select(
                            list(WEEKDAY_INDEX.keys()),
                            cursor="->",
                            cursor_style="cyan",
                        ),
                    )

                    data = handler.handle_write_curr_phase(phase)
                    await transport.write(protocol.GATT.SYS_CONF_CHAR, data)

                case Actions.READ_DURATIONS.value:
                    raw = await transport.read(protocol.GATT.DURATIONS_CHAR)

                    if raw is None:
                        print("Read operation failed on durations")
                    else:
                        print(handler.handle_read_durations(raw))

                case Actions.READ_CONFIG.value:
                    raw = await transport.read(protocol.GATT.SYS_CONF_CHAR)

                    if raw is None:
                        print("Read operation failed on config")
                    else:
                        print(handler.handle_read_config(raw))

                case Actions.READ_NODE_STATUS.value:
                    raw = await transport.read(protocol.GATT.NODE_STAT_CHAR)

                    if raw is None:
                        print("Read operation failed on statuses")
                    else:
                        print(handler.handle_read_node_status(raw))

    except KeyboardInterrupt:
        print("\n\nBye now.\n\n")
