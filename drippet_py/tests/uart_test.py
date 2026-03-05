from _pytest import config
from transport import Transport, Uart, Message
import pytest
from .mocks import (
    addressingBytes,
    emptyDataBytes,
    wateringBytes,
    addressing_message,
    empty_message,
    watering_message,
)
import constants


"""
This file tests the functionality or serialization, the values of the bytes array correspond to the c++ struct values found at the bottom. For Order look at constants.py.
Data and crc values are broken into two bits using little endian
"""

DEFAULT_BUFFER = addressingBytes + wateringBytes + bytes([10, 20]) + emptyDataBytes


class MockTransport(Transport):
    def write(self, bytes) -> None:
        pass

    def read(self, byteCount) -> bytes:
        return DEFAULT_BUFFER

    def in_waiting(self) -> int:
        return 1

    def connect(self) -> None:
        pass


@pytest.fixture
def uart():
    mockTransport = MockTransport()

    return Uart(mockTransport)


@pytest.fixture
def uart_msgs():
    return {
        "addressing": addressing_message,
        "watering": watering_message,
        "empty": empty_message,
    }


@pytest.fixture
def uart_do(uart, monkeypatch):
    def change(*args):
        return 3

    monkeypatch.setattr(Uart, "do_one", change)
    return uart


def test_data_is_entirely_read_from_buffer(uart, uart_msgs):
    res = uart.poll_data()
    assert res == len(DEFAULT_BUFFER)  # Reads the buffer in entirely
    assert len(uart.messages) == 3
    assert uart.messages[0] == uart_msgs["addressing"]
    assert uart.messages[1] == uart_msgs["watering"]
    assert uart.messages[2] == uart_msgs["empty"]


def extract_frame_rejects_invalid_indexes(uart):
    exBytes1 = bytes([0, 0, 0, constants.START_BYTE, 0, 2, 4, 3])
    exBytes2 = bytes([constants.START_BYTE])
