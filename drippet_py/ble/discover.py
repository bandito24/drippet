import asyncio
from bleak import BleakScanner


async def main():
    try:
        devices = await BleakScanner.discover()
        for d in devices:
            if d.name is not None:
                print(f"name: {d.name}: {d.address}")

    except KeyboardInterrupt:
        print("exiting...\n")
    except Exception:
        print("exiting for unforseen reason\n")


if __name__ == "__main__":
    asyncio.run(main())


def do_it():
    print("hiya")
