from _pytest import config
from transport import Transport, Uart, Message
import pytest
from .mocks import (
    addressingBytes,
    create_addressing_frame,
    create_discovery_frame,
    create_watering_frame,
    emptyDataBytes,
    wateringBytes,
    addressing_message,
    empty_message,
    watering_message,
    MockTransport,
)
import constants


"""
This file tests the functionality or serialization, the values of the bytes array correspond to the c++ struct values found at the bottom. For Order look at constants.py.
Data and crc values are broken into two bits using little endian
"""

DEFAULT_BUFFER = addressingBytes + wateringBytes + bytes([10, 20]) + emptyDataBytes


@pytest.fixture
def uartTransport():
    mockTransport = MockTransport()
    mockTransport.set_buffer(DEFAULT_BUFFER)

    return [Uart(mockTransport), mockTransport]


@pytest.fixture
def uart(uartTransport):
    uart, transport = uartTransport
    return uart


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


def test_rejects_invalid_indexes(uart):
    exBytes1 = bytes([0, 0, 0, constants.START_BYTE, 0, 2, 4, 3])
    exBytes2 = bytes([constants.START_BYTE])
    with pytest.raises(IndexError) as err1:
        uart.extract_frame(0, exBytes1)
    with pytest.raises(IndexError) as err2:
        uart.extract_frame(0, exBytes2)
    assert "invalid frame index" in str(err1.value)
    assert "invalid frame index" in str(err2.value)
    assert len(uart.messages) == 0


def test_rejects_invalid_crc(uart):
    invalid = bytearray(wateringBytes)
    invalid[-1] = 0x11
    with pytest.raises(ValueError) as err1:
        uart.extract_frame(0, invalid)
    assert "Crc16 does not match" in str(err1.value)
    assert len(uart.messages) == 0


@pytest.mark.parametrize(
    "msgInput,expected_data",
    [
        (create_watering_frame(1, 500), [500]),
        (create_addressing_frame(1, 500), [500, 0]),
        (create_discovery_frame(500), [500, 0]),
    ],
)
def test_local_creation_of_frame_is_accurate(uart, msgInput, expected_data):
    uart.extract_frame(0, msgInput)

    assert uart.messages[0].data == expected_data


def test_writes_uartMsg_correctly(uart, uart_msgs):
    assert uart.prepare_write(uart_msgs["addressing"]) == addressingBytes
    assert uart.prepare_write(uart_msgs["empty"]) == emptyDataBytes
    assert uart.prepare_write(uart_msgs["watering"]) == wateringBytes
