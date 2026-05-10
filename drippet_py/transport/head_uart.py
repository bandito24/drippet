from uart import Transport, Uart, Message
import constants as config
import constants

import time

class MockNode:


class HeadUart(Uart):
    def __init__(self, uartSerial: Transport):
        Uart.__init__(self, uartSerial)
        self.mock_tick = 0


    def handle_incoming_head(self, msg: Message):
        match constants.Command(msg.command):
            case constants.Command.ACK:
                return
            case constants.Command.ADDRESSING:
                # Write Same Frame Back To Confirm It was Received
                pass
            case _:
                print(f"Unknown incoming message of {msg.command}")
        return

    def head_uart_task(self):
        self.mock_tick += 1
        self.poll_data()
        if self.has_msg():
            print("incoming message")
            message = self.messages.popleft()
            self.handle_incoming_head(message)
        time.sleep(5)
