from enum import StrEnum
from rich.console import Console
from beaupy import Config, confirm, prompt, select, select_multiple

import traceback
import asyncio
from typing import Optional
from bleak import BleakClient
from bleak.exc import BleakCharacteristicNotFoundError
from enum import Enum


ESP_ADDR = "9C891099-DFFD-39B2-7509-0247C708220F"
GATT_CHAR = "00000025-1212-efde-1523-785feabcd123"
NODE_STAT_CHAR = "01000025-1212-efde-1523-785feabcd123"
SYS_CONF_CHAR = "02000025-1212-efde-1523-785feabcd123"
EXT_REQ_CHAR = "03000025-1212-efde-1523-785feabcd123"


class CMD(Enum):
    LOAD_ROW = 0
    WRITE_NODE_CYCLE = 1
    WRITE_NODE_DURATION = 2
    WRITE_CONF_TIME = 3
    WRITE_CONF_PHASE = 4
    INIT_PAIRING = 5


def ble_task():
    Config.raise_on_interrupt = True
    console = Console()
    action = "start"

    class Actions(StrEnum):
        INIT_PAIRING = "Init Pairing"
        WRITE_DURATION = "Write Node Duration"
        WRITE_NODE_CYCLE = "Write Node Cycle"
        SET_TIME = "Set Time"
        SET_PHASE = "Set Next Phase"
        GET_TIME = "Get Clock Times"
        EXIT = "Exit"

    actions = [x.value for x in list(Actions)]

    try:
        while action != "Exit":
            console.print("Select Action")
            # Choose one item from a list
            action = select(actions, cursor="->", cursor_style="cyan")
            match action:
                case Actions.INIT_PAIRING.value:
                    print("ini")
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")

    print("Goodbye")


async def main():
    global GATT_CHR_HANDLE
    try:
        print("Connecting...")
        async with BleakClient(ESP_ADDR) as client:
            if client is not None:
                print(f"name: {client.name}: {client.address}")
                for service in client.services:
                    print(f"Service uuid is: {service.uuid}")
                    for characteristic in service.characteristics:
                        print(f"characteristic uuid is {characteristic.uuid}")
                        if characteristic.uuid == GATT_CHAR:
                            GATT_CHR_HANDLE = characteristic
                if not GATT_CHR_HANDLE:
                    raise RuntimeError("Could not find needed gatt chr uuid")
        print("exiting due to loop termination")
    except KeyboardInterrupt:
        print("exiting...\n")
    except Exception:
        traceback.print_exc()


if __name__ == "__main__":
    asyncio.run(main())
