# BLE Write Characteristics

BLE write commands are sent to one of two characteristics depending on the type of operation being performed.

| Characteristic | UUID                                   |
| -------------- | -------------------------------------- |
| Node Durations | `00000025-1212-efde-1523-785feabcd123` |
| Configuration  | `02000025-1212-efde-1523-785feabcd123` |

---

## Configuration Characteristic

UUID:

```text id="k6o3fd"
02000025-1212-efde-1523-785feabcd123
```

The following commands must be written to the Configuration characteristic:

| Command               |
| --------------------- |
| WRITE_CONF_TIME       |
| WRITE_CONF_TIME_PHASE |
| INIT_PAIRING          |

These commands affect controller-wide configuration and are not associated with a specific irrigation node.

---

## Node Durations Characteristic

UUID:

```text id="h6epfo"
00000025-1212-efde-1523-785feabcd123
```

The following commands must be written to the Node Durations characteristic:

| Command             |
| ------------------- |
| WRITE_NODE_DURATION |
| WRITE_NODE_CYCLE    |

These commands modify watering schedules for individual irrigation nodes.

The payload for these commands contains a node index identifying which node should be updated.

---

# BLE Command Payload Format

This document describes the expected payload format for incoming BLE write commands.

All commands are transmitted as a sequence of bytes where the first byte is always the command identifier (`Cmds` enum value).

# BLE Command Payload Format

This document describes the expected payload format for incoming BLE write commands.

All commands are transmitted as a sequence of bytes where the first byte is always the command identifier (`Cmds` enum value).

---

## Command Enum

```cpp
enum class Cmds {
  WRITE_CONF_TIME,
  WRITE_NODE_DURATION,
  WRITE_NODE_CYCLE,
  WRITE_CONF_TIME_PHASE,
  INIT_PAIRING,
  REQUEST_COUNT
};
```

| Command | Enum Value |
|----------|------------|
| WRITE_CONF_TIME | 0 |
| WRITE_NODE_DURATION | 1 |
| WRITE_NODE_CYCLE | 2 |
| WRITE_CONF_TIME_PHASE | 3 |
| INIT_PAIRING | 4 |

---

# WRITE_NODE_CYCLE

Updates the watering schedule for a specific row/node.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_NODE_CYCLE`) |
| 1 | Row Index |
| 2 | Watering Cycle Bitmask |

## Example

```text
[2, 0, 77]
```

Where:

```text
Command    = WRITE_NODE_CYCLE
Row Index  = 0
Bitmask    = 77
```

### Bitmask Layout

Each bit represents one day of the week.

```text
Bit 0 = Sunday
Bit 1 = Monday
Bit 2 = Tuesday 
Bit 3 = Wednesday
Bit 4 = Thursday
Bit 5 = Friday
Bit 6 = Saturday
```

Example cycle:

```cpp
{1, 0, 1, 1, 0, 0, 0}
```

Produces:

```text
0b00001101
```

---

# WRITE_NODE_DURATION

Updates watering duration for a specific row/node.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_NODE_DURATION`) |
| 1 | Row Index |
| 2 | Duration LSB |
| 3 | Duration MSB |

Duration is encoded as a 16-bit unsigned integer in little-endian format.

## Example

```text
[1, 0, 235, 0]
```

Represents:

```text
Row Index = 0
Duration  = 235 minutes
```

Calculation:

```text
Duration = 0x00EB = 235
```

---

# WRITE_CONF_TIME

Sets the controller clock from the phone's current time.

## Payload

Payload length: **11 bytes**. Multi-byte fields are little-endian.

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_CONF_TIME`) |
| 1-8 | Current time: UTC Unix seconds (`int64_t`) |
| 9-10 | UTC offset in minutes (`int16_t`, `-720` to `840`) |

The timestamp is true UTC. Do not add the offset to it: the controller does that itself (`local = utc + offset_minutes * 60`).

The event for this command returns `INVALID_TIME` if the timestamp is before 2026-01-01 UTC (`1767225600`) or the offset is out of range. In that case the clock is not changed.

Because the offset changes with daylight saving time, the client should resend this command on every connect.

## Example

```text
[0, 0x48, 0xCE, 0xBA, 0x6A, 0x00, 0x00, 0x00, 0x00, 0x5C, 0xFE]
```

Represents:

```text
Timestamp = 0x000000006ABACE48 = 1790627400 = 2026-09-28 20:30:00 UTC
Offset    = 0xFE5C = -420 minutes (-07:00)
Local     = 2026-09-28 13:30
```

---

# WRITE_CONF_TIME_PHASE

Sets the daily watering start time, as a **local** time of day (not UTC).

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_CONF_TIME_PHASE`) |
| 1 | Hour (0-23) |
| 2 | Minute (0-59) |

## Example

```text
[3, 2, 12]
```

Represents:

```text
02:12
```

---

# INIT_PAIRING

Resets node configuration and begins pairing mode.

> Note: This operation may reset node durations and other configuration data.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`INIT_PAIRING`) |

## Example

```text
[4]
```

---

# Summary

| Command | Payload Length |
|----------|---------------|
| WRITE_CONF_TIME | 11 bytes |
| WRITE_NODE_DURATION | 4 bytes |
| WRITE_NODE_CYCLE | 3 bytes |
| WRITE_CONF_TIME_PHASE | 3 bytes |
| INIT_PAIRING | 1 byte |
