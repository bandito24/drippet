import asyncio
import traceback
from bleak import BleakClient
from .task import ble_task
from . import protocol


async def init() -> BleakClient | None:
    try:
        print("Connecting...")
        async with BleakClient(protocol.ESP_ADDR) as client:
            if client is not None:
                print(f"name: {client.name}: {client.address}")
                for service in client.services:
                    print(f"Service uuid is: {service.uuid}")
                    for characteristic in service.characteristics:
                        print(f"characteristic uuid is {characteristic.uuid}")

                return await ble_task(client)

    except KeyboardInterrupt:
        print("exiting...\n")
    except Exception:
        traceback.print_exc()


async def main():
    try:
        client = await init()
        if not client:
            raise RuntimeError("Client connection could not be established")
        await ble_task(client)

    except RuntimeError as e:
        print(e)
    except:
        print("\nExiting...")


if __name__ == "__main__":
    asyncio.run(main())
