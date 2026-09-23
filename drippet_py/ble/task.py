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
import time
from threading import Thread

from ble.models import HourMin
from constants import CYCLE_LEN, NodeStatus, WEEKDAY_INDEX

from . import protocol
from typing import Protocol

from datetime import datetime


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

    SYNC_TIME = "Sync Time"
    EXIT = "Exit"


def prompt_int(title: str) -> int:
    return prompt(
        title,
        target_type=int,
        validator=lambda count: count > -1,
    )


class CommandHandler:
    def handle_init_pairing(self) -> bytes:
        return protocol.initPairing()

    def handle_write_durations(self, node_index: int, duration: int) -> bytes:
        return protocol.writeNodeDuration(node_index, duration)

    def handle_write_cycle(self, node_index: int, selections: list[str]) -> bytes:
        bitmask = Node.weekdayToBitmask(selections)
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

    def handle_read_events(self, data: bytearray) -> EventResult:
        return protocol.read_events(data)

    def node_stat_cb(self, sender, data: bytearray):
        if data is None:
            print("Read operation failed on statuses")
        else:
            res = self.handle_read_node_status(data)
            for i in range(len(res)):
                name = res[i].name.replace("_", " ").title()
                print(f"\n--- Node Status: {name} ---\n")

    def read_events_cb(self, sender, data: bytearray):
        if data is None:
            print("Read operation failed on events")
            return

        event = self.handle_read_events(data)

        print("\n--- Event ---")
        print(event)
        print()


action = "start"


async def ble_task(client: BleakClient):
    global action
    Config.raise_on_interrupt = True

    console = Console()
    actions = [x.value for x in Actions]

    handler = CommandHandler()
    transport = protocol.Transport(client)

    await client.start_notify(protocol.GATT.RESPONSE_CHAR.value, handler.read_events_cb)
    await client.start_notify(protocol.GATT.NODE_STAT_CHAR.value, handler.node_stat_cb)

    async def prompt_user():
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
            case Actions.SYNC_TIME.value:
                now = datetime.now()
                data = handler.handle_write_sys_time(
                    now.hour,
                    now.minute,
                )
                # 0 == Monday with datetime
                adj_day = (now.weekday() + 1) % 7
                await transport.write(protocol.GATT.SYS_CONF_CHAR, data)
                weekday = list(WEEKDAY_INDEX.keys())[
                    list(WEEKDAY_INDEX.values()).index(adj_day)
                ]
                data2 = handler.handle_write_curr_phase(weekday)
                await transport.write(protocol.GATT.SYS_CONF_CHAR, data2)

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
        await asyncio.sleep(1)
        return action

    try:
        while action != "Exit":
            action = await prompt_user()

    except KeyboardInterrupt:
        print("\n\nBye now.\n\n")
