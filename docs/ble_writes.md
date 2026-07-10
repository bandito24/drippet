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
  WRITE_CONF_PHASE,
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
| WRITE_CONF_PHASE | 3 |
| WRITE_CONF_TIME_PHASE | 4 |
| INIT_PAIRING | 5 |

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
Bit 0 = Monday
Bit 1 = Tuesday
Bit 2 = Wednesday
Bit 3 = Thursday
Bit 4 = Friday
Bit 5 = Saturday
Bit 6 = Sunday
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

# WRITE_CONF_PHASE

Sets the active configuration phase.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_CONF_PHASE`) |
| 1 | Phase Number |

## Example

```text
[3, 3]
```

Represents:

```text
Phase = 3
```

---

# WRITE_CONF_TIME

Sets the controller clock time.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_CONF_TIME`) |
| 1 | Hour (0-23) |
| 2 | Minute (0-59) |

## Example

```text
[0, 12, 59]
```

Represents:

```text
12:59
```

---

# WRITE_CONF_TIME_PHASE

Sets the start time associated with a configuration phase.

## Payload

| Byte | Description |
|--------|-------------|
| 0 | Command (`WRITE_CONF_TIME_PHASE`) |
| 1 | Hour (0-23) |
| 2 | Minute (0-59) |

## Example

```text
[4, 2, 12]
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
[5]
```

---

# Summary

| Command | Payload Length |
|----------|---------------|
| WRITE_CONF_TIME | 3 bytes |
| WRITE_NODE_DURATION | 4 bytes |
| WRITE_NODE_CYCLE | 3 bytes |
| WRITE_CONF_PHASE | 2 bytes |
| WRITE_CONF_TIME_PHASE | 3 bytes |
| INIT_PAIRING | 1 byte |
