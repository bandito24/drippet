from transport import Uart, HeadUart, UartSerial
import serial
import traceback


def init():
    try:
        serialTrans = UartSerial()
        uart = HeadUart(serialTrans)

        while True:
            try:
                uart.uart_task()
            except ValueError as e:
                print(f"Location Operation Failed: {e}")
            except serial.SerialTimeoutException as e:
                print(f"Serial functionality failed: {e}")

            except Exception as e:
                print("Serial functionality failed:")
                traceback.print_exc()
                return

    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"\nError encountered: {e}")


if __name__ == "__main__":
    init()
