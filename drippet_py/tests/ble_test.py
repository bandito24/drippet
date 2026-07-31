from _pytest.fixtures import pytest_fixture_setup
import pytest
from ble import Node, protocol
from ble.task import CommandHandler
from . import ble_mocks as mocks


"""Fixtures related to the input handler"""


@pytest.fixture
def transport():
    return protocol.Transport(mocks.BLEClientMock())


@pytest.fixture
def handler(transport):
    return CommandHandler(transport)


"""Testing Raw Bytes From Writes"""
async def testInitPairingBytes(handler: CommandHandler):
    await handler.handle_init_pairing()
    assert handler. 


def testNodeParsesCycle():
    data = [1, 0, 1, 0, 1, 1, 0]
    data = [bool(x) for x in data]
    res1 = Node.cycleToBitmask(data)
    print(res1)
    res2 = Node.bitmaskToCycle(res1)
    assert data == res2
