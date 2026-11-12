# 11: ESP-side safety fixes

**What to build:** Two crash and undefined-behaviour risks in the ESP-only code are removed.
- The UART task queues the parsed frame itself, not the address of the `optional` around it. Forwarding then doesn't depend on how `optional` is laid out in memory.
- Saving node durations when NVS failed to initialise returns an error without touching the null storage handle.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The UART task sends the contained message to the incoming queue
- [ ] Saving durations with uninitialised storage logs and returns an error, with no dereference
- [ ] Head-to-node watering still works end to end on hardware
- [ ] The firmware builds with `idf.py build`

Spec: `../spec.md`
