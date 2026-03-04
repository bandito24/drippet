from transport.uart import Uart, uart_task


def init():
    try:
        uart_task(Uart())
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"\nError encountered: {e}")


if __name__ == "__main__":
    init()
