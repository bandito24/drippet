from constants import BleCommand
from ble.protocol import GATT


class BLEClientMock:
    def __init__(self):
        self.read_buffer: bytearray = bytearray()
        self.write_buffer: bytes = bytes()

    async def write_gatt_char(self, gatt: str, data: bytes) -> None:
        self.write_buffer = data

    async def read_gatt_char(self, gatt: str) -> bytearray:
        return self.read_buffer


class Mocks:
    @staticmethod
    def write_conf_time(hour: int, minute: int) -> bytes:
        return bytes(
            [
                BleCommand.WRITE_CONF_TIME.value,
                hour,
                minute,
            ]
        )

    @staticmethod
    def write_node_duration(node: int, duration: int) -> bytes:
        return bytes(
            [
                BleCommand.WRITE_NODE_DURATION.value,
                node,
                *duration.to_bytes(2, "little"),
            ]
        )

    @staticmethod
    def write_node_cycle(node: int, bitmask: int) -> bytes:
        return bytes(
            [
                BleCommand.WRITE_NODE_CYCLE.value,
                node,
                bitmask,
            ]
        )

    @staticmethod
    def write_conf_phase(phase: int) -> bytes:
        return bytes(
            [
                BleCommand.WRITE_CONF_PHASE.value,
                phase,
            ]
        )

    @staticmethod
    def write_conf_time_phase(hour: int, minute: int) -> bytes:
        return bytes(
            [
                BleCommand.WRITE_CONF_TIME_PHASE.value,
                hour,
                minute,
            ]
        )

    @staticmethod
    def write_init_pairing() -> bytes:
        return bytes(
            [
                BleCommand.INIT_PAIRING.value,
            ]
        )

    @staticmethod
    def read_node_durations(*nodes: tuple[int, int]) -> bytes:
        """
        nodes = (duration_minutes, day_bitmask)

        Example:
            Mocks.node_durations(
                (235, 0x7F),
                (210, 0x15),
            )
        """
        data = bytearray()

        for duration, cycle in nodes:
            data.extend(duration.to_bytes(2, "little"))
            data.append(cycle)

        return bytes(data)

    @staticmethod
    def read_node_states(*states: int) -> bytearray:
        return bytearray(states)

    @staticmethod
    def read_config(
        hour: int = 20,
        minute: int = 30,
        phase: int = 2,
        next_phase: tuple[int, int] | None = (22, 0),
    ) -> bytearray:
        data = bytearray(
            [
                hour,
                minute,
                phase,
            ]
        )

        if next_phase is None:
            data.extend([0, 0, 0])
        else:
            next_hour, next_min = next_phase
            data.extend([1, next_hour, next_min])

        return bytearray(data)

    @staticmethod
    def read_event(
        command: int,
        status: int,
        node: int = 0,
    ) -> bytes:
        return bytes(
            [
                command,
                status,
                node,
            ]
        )
