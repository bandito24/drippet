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

from datetime import datetime, timedelta

# Warn after a config read when the device clock is this far off
MAX_CLOCK_DRIFT_SEC = 60


def utc_and_offset(moment: datetime) -> tuple[int, int]:
    """Split an aware datetime into (UTC Unix seconds, UTC offset minutes)."""
    offset = moment.utcoffset()
    if offset is None:
        raise ValueError("datetime must be timezone-aware")
    return int(moment.timestamp()), int(offset.total_seconds() // 60)


def current_utc_and_offset() -> tuple[int, int]:
    """Clock Sync payload values for this computer's clock and timezone."""
    return utc_and_offset(datetime.now().astimezone())


def mock_local_time(
    weekday: int, hour: int, minute: int, now: datetime | None = None
) -> datetime:
    """The next local hour:minute on `weekday` (Sunday = 0), within the
    7 days starting today. Today counts, even if the time has passed."""
    now = (now or datetime.now()).astimezone()
    today = (now.weekday() + 1) % 7  # Python Monday = 0 -> Sunday = 0
    day = now + timedelta(days=(weekday - today) % 7)
    naive = day.replace(hour=hour, minute=minute, second=0, microsecond=0,
                        tzinfo=None)
    # Re-resolve the offset for that date, in case DST differs from today
    return naive.astimezone()


def clock_drift_warning(config: SysConfig, now: datetime | None = None) -> str | None:
    now = (now or datetime.now()).astimezone()
    phone_ts, phone_offset = utc_and_offset(now)
    drift = int(config.sys_time.timestamp()) - phone_ts
    problems = []
    if abs(drift) > MAX_CLOCK_DRIFT_SEC:
        problems.append(f"device clock is {drift:+d} s off")
    if config.utc_offset_min != phone_offset:
        problems.append(
            f"device offset is {config.utc_offset_min} min, this computer's is {phone_offset} min"
        )
    if not problems:
        return None
    return "Warning: " + "; ".join(problems) + f". Run '{Actions.SYNC_TIME.value}'."


class Actions(StrEnum):
    INIT_PAIRING = "Init Pairing"
    WRITE_DURATION = "Write Node Duration"
    WRITE_NODE_CYCLE = "Write Node Cycle"
    READ_DURATIONS = "Read Node Durations"
    READ_NODE_STATUS = "Read Node Statuses"
    MOCK_TIME = "Mock Time"
    SET_PHASE_TIME = "Set Water Start"
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

    def handle_write_sys_time(self, utc_seconds: int, offset_min: int) -> bytes:
        return protocol.writeConfTime(utc_seconds, offset_min)

    def handle_write_phase_time(self, hour: int, minute: int) -> bytes:
        return protocol.writePhaseStartTime(hour, minute)

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

    # The head won't water until its clock is synced, and resyncing on every
    # connect is what keeps DST correct
    await transport.write(
        protocol.GATT.SYS_CONF_CHAR,
        handler.handle_write_sys_time(*current_utc_and_offset()),
    )
    console.print("Synced device clock to this computer")

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

            case Actions.MOCK_TIME.value:
                console.print("Which day?")
                day_name = cast(str, select(list(WEEKDAY_INDEX.keys()), cursor="->"))
                moment = mock_local_time(
                    WEEKDAY_INDEX[day_name],
                    prompt_int("Set Hour 0-23") % 24,
                    prompt_int("Set Minutes 0-59") % 60,
                )
                console.print(f"Setting device clock to {moment:%a %b %d %H:%M %Z}")
                data = handler.handle_write_sys_time(*utc_and_offset(moment))
                await transport.write(protocol.GATT.SYS_CONF_CHAR, data)
            case Actions.SYNC_TIME.value:
                data = handler.handle_write_sys_time(*current_utc_and_offset())
                await transport.write(protocol.GATT.SYS_CONF_CHAR, data)

            case Actions.SET_PHASE_TIME.value:
                data = handler.handle_write_phase_time(
                    prompt_int("Set Hour 0-23"),
                    prompt_int("Set Minutes 0-59"),
                )
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
                    config = handler.handle_read_config(raw)
                    print(config)
                    warning = clock_drift_warning(config)
                    if warning:
                        console.print(warning, style="yellow")

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
