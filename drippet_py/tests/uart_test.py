from .mocks import addressingBytes, wateringBytes, emptyDataBytes
from transport import Uart, uart_task
from constants import START_BYTE


def add(x, y):
    return x + y


def test_it():
    assert add(2, 3) == 5
