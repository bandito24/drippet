import uart


def init():
    try:
        uart.connect()
    except KeyboardInterrupt: 
        print("\nExiting...")
    except Exception as e:
        print(f"\nError encountered: {e}")

if __name__ == "__main__":
    init()
