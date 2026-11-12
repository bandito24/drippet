# 06: Strict write lengths with ATT errors

**What to build:** A malformed BLE write is refused with a clear ATT error instead of being treated as a zero-filled command. The BLE link layer receives the real write length and accepts each command only at its exact length:

| Command | Length |
| --- | --- |
| `WRITE_CONF_TIME` | 11 |
| `WRITE_NODE_DURATION` | 4 |
| `WRITE_NODE_CYCLE` | 3 |
| `WRITE_CONF_TIME_PHASE` | 3 |
| `INIT_PAIRING` | 1 |

The GATT glue maps each result to an ATT error:
- Wrong length, empty, or too long → `0x0D` Invalid Attribute Value Length.
- Request queue full → `0x11` Insufficient Resources.
- Unknown command → `0x0E` Unlikely Error.

Update the BLE write docs with the lengths and errors, mark item 2.10 done in the migration doc, and make sure the Python client sends exact lengths.

**Blocked by:** 05 (supplies the queue-full result)

**Status:** ready-for-agent

- [ ] Each command is accepted at exactly its length, and rejected one byte short or one byte long
- [ ] Unknown command bytes are rejected
- [ ] A write when the queue is full returns the queue-full result
- [ ] Existing BLE tests pass exact-length spans instead of the fixed 20-byte mock arrays
- [ ] The phone sees the ATT errors (checked on hardware with the Python client)
- [ ] The docs list the lengths and error codes; the migration doc marks 2.10 done
- [ ] Host suite and pytest pass

Spec: `../spec.md`
