import traceback
import asyncio
from typing import Optional
from bleak import BleakClient
from bleak.exc import BleakCharacteristicNotFoundError
from enum import Enum


ESP_ADDR = "9C891099-DFFD-39B2-7509-0247C708220F"
GATT_CHAR = "00000025-1212-efde-1523-785feabcd123"
MAX_NODE_COUNT = 5
HOSE_COUNT = 5
NODE_HOSE_COUNT = 5
WRITE_CELL_PKT_LEN = 5  # CMD, DATA_LEN, ROW, COL, DURATION
WRITE_ROW_PKT_LEN = 3 + NODE_HOSE_COUNT  # CMD, DATA_LEN, ROW, NODE_HOSE_COUNTS
LOAD_ROW_PKT_LEN = 3  # CMD, DATA_LEN, ROW
VAL_THRESHOLD = 65535
GATT_CHR_HANDLE = None


class Ble_Cmd(Enum):
    LOAD_ROW_CMD = 0
    WRITE_ROW_CMD = 1
    WRITE_CELL_CMD = 2
    READ_BUFF_CMD = 3


class HeaderIdx(Enum):
    CMDS = 0
    DATA_LEN = 1
    TGT_ROW = 2


ROW_OP_IDX = HeaderIdx.TGT_ROW.value
COL_OP_IDX = ROW_OP_IDX + 1
DUR_IDX_START_ROW = ROW_OP_IDX + 1
DUR_IDX_START_CELL = COL_OP_IDX + 1


def print_bytes(raw_bytes: bytearray):
    print([byte for byte in bytes(raw_bytes)])


def prepare_write_bytes(input: str, cmd: Ble_Cmd) -> bytearray:
    argArray = input.split(" ")
    print(argArray)
    packet = [int(x) for x in argArray]
    if int(packet[ROW_OP_IDX]) >= MAX_NODE_COUNT:
        raise RuntimeError("invalid row provided")

    match cmd:
        case Ble_Cmd.WRITE_CELL_CMD:
            if packet[COL_OP_IDX] >= HOSE_COUNT:
                raise RuntimeError(f"Invalid column index of {packet[COL_OP_IDX]}")
            data_start = DUR_IDX_START_CELL
            needed_len = WRITE_CELL_PKT_LEN
        case Ble_Cmd.WRITE_ROW_CMD:
            data_start = DUR_IDX_START_ROW
            needed_len = WRITE_ROW_PKT_LEN
        case Ble_Cmd.LOAD_ROW_CMD:
            data_start = len(packet)
            needed_len = LOAD_ROW_PKT_LEN
        case _:
            raise RuntimeError("Invalid arg provided for command idx")

    if len(packet) != needed_len:
        raise RuntimeError(
            f"Invalid length, received {len(packet)} but needed {needed_len}"
        )

    bytesVal = bytearray(packet[:data_start])
    if data_start != len(packet):
        bytesVal += b"".join(x.to_bytes(2, "little") for x in packet[data_start:])
    return bytesVal


async def ble_task(client: BleakClient):
    if not GATT_CHR_HANDLE:
        raise RuntimeError("Could not find needed gatt chr uuid")

    user_input = None
    while user_input != "exit" and client.is_connected:
        try:
            user_input = input("Please enter the next input: ")
            if user_input == "exit":
                break
            user_input = int(user_input)
            if user_input not in Ble_Cmd:
                print("Invalid Command")
            elif user_input == Ble_Cmd.READ_BUFF_CMD.value:
                raw_bytes = await client.read_gatt_char(GATT_CHR_HANDLE)
                print(f"raw bytes are {raw_bytes}")
                res = []
                res.append(raw_bytes[0])
                for i in range(1, len(raw_bytes), 2):
                    num = int.from_bytes(raw_bytes[i : i + 2], "little")
                    res.append(num)
                print(f"response from ble peripheral: {res}")
            elif user_input == Ble_Cmd.LOAD_ROW_CMD.value:
                ui = input("Please enter row: ")
                args = f"{Ble_Cmd.LOAD_ROW_CMD.value} 1 {ui}"  # Data length of four is row, col, 2 duration bytes for uint16
                byteData = prepare_write_bytes(args, Ble_Cmd.LOAD_ROW_CMD)
                await client.write_gatt_char(GATT_CHR_HANDLE, byteData)
            elif user_input == Ble_Cmd.WRITE_CELL_CMD.value:
                ui = input("Please enter row, cell, and duration: ")
                args = f"{Ble_Cmd.WRITE_CELL_CMD.value} 4 {ui}"  # Data length of four is row, col, 2 duration bytes for uint16
                byteData = prepare_write_bytes(args, Ble_Cmd.WRITE_CELL_CMD)
                await client.write_gatt_char(GATT_CHR_HANDLE, byteData)
            elif user_input == Ble_Cmd.WRITE_ROW_CMD.value:
                ui = input("Please enter row and durations: ")
                args = f"{Ble_Cmd.WRITE_ROW_CMD.value} 11 {ui}"  # Data length is row, 2 duration bytes for 5 durations
                byteData = prepare_write_bytes(args, Ble_Cmd.WRITE_ROW_CMD)
                await client.write_gatt_char(GATT_CHR_HANDLE, byteData)
            else:
                raise RuntimeError("Unrecognized input. Please re-enter")
        except BleakCharacteristicNotFoundError:
            print("Bleak characteristic UUID not found...\n")
        except RuntimeError:
            traceback.print_exc()

    print("Goodbye \n")


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
                await ble_task(client)
        print("exiting due to loop termination")
    except KeyboardInterrupt:
        print("exiting...\n")
    except Exception:
        traceback.print_exc()


if __name__ == "__main__":
    asyncio.run(main())
