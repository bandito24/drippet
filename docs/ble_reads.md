
# BLE Reads

This document describes the BLE characteristics exposed by the irrigation controller for reading state and receiving notifications.

## Service UUID

```text
00020025-1212-efde-1523-785feabcd123
```

---

# Characteristics

| Characteristic | UUID                                   |
| -------------- | -------------------------------------- |
| Node Durations | `00000025-1212-efde-1523-785feabcd123` |
| Node States    | `01000025-1212-efde-1523-785feabcd123` |
| Configuration  | `02000025-1212-efde-1523-785feabcd123` |
| Events         | `03000025-1212-efde-1523-785feabcd123` |

---

# Node Durations

## UUID

```text
00000025-1212-efde-1523-785feabcd123
```

## Access Type

Read

## Description

Returns the configured watering duration and watering schedule for each discovered node.

Each node contributes three bytes to the response payload:

| Byte Offset | Description                                   |
| ----------- | --------------------------------------------- |
| 0-1         | Watering duration (`uint16_t`, little-endian) |
| 2           | Watering cycle bitmask                        |

The payload is repeated for every discovered node.

### Example

Two nodes:

```text
EB 00 7F D2 00 15
```

Decoded:

| Node | Duration    | Cycle Bitmask |
| ---- | ----------- | ------------- |
| 0    | 235 minutes | `0x7F`        |
| 1    | 210 minutes | `0x15`        |

---

## Watering Cycle Bitmask

The watering cycle bitmask contains one bit for each day of the week.

| Bit | Day       |
| --- | --------- |
| 0   | Sunday    |
| 1   | Monday    |
| 2   | Tuesday   |
| 3   | Wednesday |
| 4   | Thursday  |
| 5   | Friday    |
| 6   | Saturday  |

Example:

```text
0x15 = 00010101
```

Means:

```text
Sunday    = ON
Monday    = OFF
Tuesday   = ON
Wednesday = OFF
Thursday  = ON
Friday    = OFF
Saturday  = OFF
```

---

# Node States

## UUID

```text
01000025-1212-efde-1523-785feabcd123
```

## Access Type

Indication

## Description

Returns the current state of each discovered node.

The response contains one byte per node.

The array index corresponds to the node index.

Example:

```text
[1, 4, 1]
```

Means:

| Node | State    |
| ---- | -------- |
| 0    | READY    |
| 1    | WATERING |
| 2    | READY    |

---

## Node Status Values

| Value | Status           |
| ----- | ---------------- |
| 0     | INITIALIZING     |
| 1     | READY            |
| 2     | IN_QUEUE         |
| 3     | COMMAND_SENT     |
| 4     | WATERING         |
| 5     | ERR              |
| 6     | INVALID_TIME     |
| 7     | NODE_NONEXISTANT |

### Status Definitions

#### INITIALIZING

Node has been discovered but is still initializing.

#### READY

Node is operational and ready to accept commands.

#### IN_QUEUE

A command is queued for transmission to the node.

#### COMMAND_SENT

A command has been transmitted and is awaiting completion.

#### WATERING

The node is actively watering.

#### ERR

A communication or processing error occurred.

#### INVALID_TIME

The node rejected the supplied time configuration.

#### NODE_NONEXISTANT

The node index does not correspond to a discovered node.

---

# Configuration

## UUID

```text
02000025-1212-efde-1523-785feabcd123
```

## Access Type

Read

## Description

Returns controller-level configuration information.

Payload length: **13 bytes**. Multi-byte fields are little-endian.

| Byte  | Description                                        |
| ----- | -------------------------------------------------- |
| 0-7   | Current time: UTC Unix seconds (`int64_t`)         |
| 8-9   | UTC offset in minutes (`int16_t`, `-720` to `840`) |
| 10    | Watering time set flag                             |
| 11    | Watering hour, local time (0-23)                   |
| 12    | Watering minute, local time (0-59)                 |

### Current Time

Bytes 0-7 hold the controller's clock as true UTC Unix seconds. The UTC offset is **not** applied. To get the controller's local time:

```text
local = utc + offset_minutes * 60
```

The client sets the clock with `WRITE_CONF_TIME` on every connect, so the current time is always valid when this characteristic is read.

### Watering Time Set Flag

| Value | Meaning                                            |
| ----- | -------------------------------------------------- |
| 0     | No daily watering time set                         |
| 1     | Daily watering time set by `WRITE_CONF_TIME_PHASE` |

When no watering time is set, bytes 11 and 12 are `0` and should be ignored.

The watering hour and minute are the **local** time of day exactly as written with `WRITE_CONF_TIME_PHASE`. They are not a date: whether a node waters on a given day depends on its cycle bitmask.

### Cycle Phase

The current cycle phase is not sent. It is the local weekday (Sunday = 0), which the client can work out from the current time and offset.

---

## Example

```text
48 CE BA 6A 00 00 00 00 5C FE 01 02 00
```

Decoded:

| Field             | Value                                  |
| ----------------- | -------------------------------------- |
| Current Time      | `1790627400` = 2026-09-28 20:30:00 UTC |
| UTC Offset        | `-420` minutes (-07:00)                |
| Local Time        | 2026-09-28 13:30                       |
| Watering Time Set | Yes                                    |
| Watering Time     | 02:00 local                            |

---

## Example (No Watering Time Set)

```text
48 CE BA 6A 00 00 00 00 5C FE 00 00 00
```

Decoded:

| Field             | Value                   |
| ----------------- | ----------------------- |
| Current Time      | 2026-09-28 20:30:00 UTC |
| UTC Offset        | -07:00                  |
| Watering Time Set | No                      |
| Watering Time     | N/A                     |

---

# Events

## UUID

```text
03000025-1212-efde-1523-785feabcd123
```

## Access Type

Notification

## Description

Events provide asynchronous responses to commands previously sent by the host.

The notification payload is:

| Byte | Description |
| ---- | ----------- |
| 0    | Command     |
| 1    | Status      |
| 2    | Node Index  |

---

## Command Values

| Value | Command               |
| ----- | --------------------- |
| 0     | WRITE_CONF_TIME       |
| 1     | WRITE_NODE_DURATION   |
| 2     | WRITE_NODE_CYCLE      |
| 3     | WRITE_CONF_TIME_PHASE |
| 4     | INIT_PAIRING          |

---

## Status Values

| Value | Status       |
| ----- | ------------ |
| 0     | OK           |
| 1     | INVALID_TIME |
| 2     | INVALID_NODE |

---

## Node Index

For node-specific commands (`WRITE_NODE_DURATION` and `WRITE_NODE_CYCLE`), this value contains the affected node index.

For controller-wide commands, this value may be ignored.

---

## Example Notifications

### Node Duration Updated Successfully

```text
01 00 03
```

Decoded:

```text
Command: WRITE_NODE_DURATION
Status: OK
Node: 3
```

### Invalid Node

```text
02 02 07
```

Decoded:

```text
Command: WRITE_NODE_CYCLE
Status: INVALID_NODE
Node: 7
```

### Invalid Time

```text
00 01 00
```

Decoded:

```text
Command: WRITE_CONF_TIME
Status: INVALID_TIME
```
