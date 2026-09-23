from bleak import BleakScanner
import asyncio
import traceback
from bleak import BleakClient
from .task import ble_task
from . import protocol


async def init() -> BleakClient | None:
    try:
        print("Connecting...")
        device = await BleakScanner.find_device_by_name("Drippet", 15)
        if not device:
            raise RuntimeError("Could not discover the Drippet Node by Name")
        async with BleakClient(device) as client:
            await ble_task(client)

    except KeyboardInterrupt:
        print("exiting...\n")
    except Exception:
        traceback.print_exc()


async def main():
    try:
        await init()

    except RuntimeError as e:
        print(e)
    except:
        print("\nExiting...")


if __name__ == "__main__":
    asyncio.run(main())
