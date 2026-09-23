from datetime import datetime


def main():
    now = datetime.now()
    print(now.hour, now.minute, now.weekday())


if __name__ == "__main__":
    main()
