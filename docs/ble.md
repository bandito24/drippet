
# BLE Communication Protocol

This document describes the Bluetooth Low Energy (BLE) communication protocol used by the system.

## Packet Format

All communication between the BLE central and peripheral begins with a two-byte header:

| Byte | Description |
| ---- | ----------- |
| 0    | Command     |
| 1    | Data Length |

The `Command` byte identifies the operation being requested or responded to.

The `Data Length` byte specifies the number of bytes that follow in the packet payload.

```text
[COMMAND][DATA_LENGTH][DATA...]
```

### Example

A command with two bytes of payload:

```text
[WRITE_CONF_TIME][2][12][30]
```

would indicate a request to update the configured time to `12:30`.

---

## Supported Commands

| Value | Command            | Description                                 |
| ----- | ------------------ | ------------------------------------------- |
| 0     | `LOAD_ROW`         | Load a row of configuration data.           |
| 1     | `WRITE_CYCLE`      | Update a node watering cycle configuration. |
| 2     | `WRITE_CELL`       | Update an individual configuration cell.    |
| 3     | `WRITE_CONF_TIME`  | Update the configured clock time.           |
| 4     | `WRITE_CONF_PHASE` | Update the configured phase start time.     |
| 5     | `INIT_PAIRING`     | Place the system into pairing mode.         |

## Notes

* The payload format depends on the command being executed.
* Commands with no associated data should specify a data length of `0`.
* All multi-byte values are encoded in little-endian.



