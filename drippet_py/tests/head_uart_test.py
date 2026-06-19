import pytest
from constants import ADDR_UNSET, Command, NodeStatus
import constants
from transport import Uart
from tests.mocks import MockTransport, create_addressing_frame, create_discovery_frame
import transport
from transport.uart import HeadUart


@pytest.fixture
def uartTransport():
    mockTransport = MockTransport()
    return [HeadUart(mockTransport), mockTransport]


@pytest.fixture
def uart(uartTransport):
    return uartTransport[0]


@pytest.fixture
def handle_discovery(uartTransport):
    head, mockTransport = uartTransport
    mockTransport.set_buffer(create_discovery_frame(500))
    return (head, head.poll_incoming(), mockTransport)


@pytest.fixture
def handle_addressing(handle_discovery):
    head, msg, mockTransport = handle_discovery
    mockTransport.set_buffer(create_addressing_frame(0, 500))
    return (head, head.poll_incoming(), mockTransport)


def test_initialization_sequence_saves_node(handle_discovery):
    head, msg, _ = handle_discovery
    assert msg.command == constants.Command.ADDRESSING.value
    assert len(head.mock_nodes) == 1
    assert head.mock_nodes[500].key == 500
    assert msg.data == [500, 0]
    assert head.mock_nodes[500].status == constants.NodeStatus.INITIALIZING
    assert head.mock_nodes[500].address == 0


def test_addressing_confirms_node(handle_addressing):
    head, msg, _ = handle_addressing

    assert msg.command == constants.Command.ACK
    assert head.mock_nodes[500].status == constants.NodeStatus.READY


def test_wrong_address_throws_e(handle_discovery):
    head, _, mockTransport = handle_discovery
    mockTransport.set_buffer(create_addressing_frame(2, 500))
    with pytest.raises(RuntimeError):
        head.poll_incoming()


def test_unseen_key_throws_e(handle_discovery):
    head, _, mockTransport = handle_discovery
    mockTransport.set_buffer(create_addressing_frame(0, 300))
    with pytest.raises(RuntimeError):
        head.poll_incoming()
