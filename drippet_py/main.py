from transport import Uart, UartSerial, uart_task


def init():
    try:
        serial = UartSerial()
        uart_task(Uart(serial))
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"\nError encountered: {e}")


if __name__ == "__main__":
    init()
