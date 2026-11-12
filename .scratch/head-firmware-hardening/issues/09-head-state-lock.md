# 09: Head state lock and deferred self-node restart

**What to build:** The app can read Node Durations, Node States and Configuration at any time, including during Pairing, without getting torn data or crashing the head. Today a read during Pairing can walk the node table while it is being cleared.
- The Head owns one mutex. It is portable, so `core/` stays free of ESP-IDF headers.
- The head task holds it for each loop step's state work and releases it before any blocking queue send.
- BLE reads of head state take the same lock.
- Starting Pairing no longer restarts the head's self-node inline. It marks a restart as pending, and the head task runs it after releasing the lock, so the restart (up to 10 s) never blocks BLE reads.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] Starting Pairing marks a restart as pending without calling the restart callback; running the pending restart calls it exactly once and clears the mark (Head tests)
- [ ] BLE reads of Node Durations, Node States and Configuration take the Head lock
- [ ] The head task never holds the lock across a blocking queue send or the self-node restart
- [ ] Reading over BLE while Pairing starts doesn't crash the head and returns well-formed data (checked on hardware)
- [ ] Host suite passes

Spec: `../spec.md`
