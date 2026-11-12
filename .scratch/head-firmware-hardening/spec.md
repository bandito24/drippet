# Head firmware hardening before the native app

Status: ready-for-agent

## Problem Statement

The head firmware is about to become the backend for the native app (drible), and a code review found that it doesn't behave the way the BLE spec and the glossary say it should:

- **Pairing** sends an empty key, so a node adopts key 0.
- A frame from an unknown **Node Address** can crash the head.
- A watering run that ends early leaves nodes stuck in a queued state, and they never water again.
- A duration write over the budget wipes the node's existing duration and shuts that node out of watering for good.
- Writes sent in quick succession overwrite each other, so only the last write of each command type takes effect, and its event is the only one the client sees.
- BLE callbacks and the head task share state with no locking. During **Pairing** this can be a use-after-free.
- The **Watering Start Time** is lost on every reboot.
- The client can't see the **Head Status**, so it can't tell when Pairing has finished or that a **Faulty Node** has stopped all watering.
- Malformed BLE writes are accepted silently.
- The Events payload doesn't match the spec.
- The service UUID isn't advertised, so the app can't scan by service.
- Subscription state never resets on disconnect.
- The docs say durations are minutes, but the firmware uses seconds.

Three host test cases fail because the test helpers pass a node's address and its value in the wrong order.

## Solution

Fix the firmware so it matches the BLE spec and glossary, and make the wire contract strict and predictable for the app:

- Pairing sends its key.
- Unknown or invalid frames are ignored instead of crashing the head.
- Every way a watering run can end returns nodes to a state where they can water again.
- Rejected writes have no side effects.
- Client requests and their events are handled first-in, first-out, with a bounded queue.
- Shared head state is protected by one lock.
- The Watering Start Time survives reboots.
- Head Status is exposed in the Configuration read.
- Writes of the wrong length are refused with a clear ATT error.
- The Events payload is exactly 3 bytes.
- The service UUID is advertised.
- Subscriptions reset on disconnect.
- Durations are documented as seconds.

The Python client and the docs are updated in step with every wire change. The app's parsers are updated wherever they already decode these characteristics.

## User Stories

1. As a gardener, I want Pairing to give every node on the chain its own working Node Address, so that each hose can be configured on its own.
2. As a gardener, I want Pairing to address nodes in chain order, one at a time, so that a node's Node Address always matches its physical position and its saved settings come back after re-pairing.
3. As a gardener, I want the head to keep running when a stray or corrupted frame arrives from an address it doesn't know, so that my watering isn't stopped by a reboot or a crash.
4. As a gardener, I want a node that rebooted in the middle of a run to be retried on the next run, not left out forever, so that one glitch doesn't quietly stop a hose.
5. As a gardener, I want every node that was queued but not watered in a run that ended early to be eligible again on the next run, so that a run that ends early doesn't strand hoses.
6. As a gardener, I want watering to stop on every node while a Faulty Node exists, and the status LED to show the fault, so that I find out and fix the problem instead of having a part of the garden quietly go unwatered.
7. As a gardener, I want watering to resume normally after I re-pair, once the faulty node is fixed, so that recovery is one clear action.
8. As an app user, I want a duration that would exceed the Watering Interval to be rejected with `INVALID_TIME`, keeping that node's previous duration, so that a typo doesn't wipe a working setting.
9. As an app user, I want a rejected duration write to leave the node's status unchanged, so that the node keeps watering on its old duration.
10. As an app user, I want to change durations for several nodes in quick succession and have every change applied, so that saving a whole screen of rows works.
11. As an app user, I want one Events notification per write, in the order I sent the writes, so that I can match each result to the row I edited.
12. As an app user, I want a write to be refused with a clear error when the head's request queue is full, so that I can retry instead of thinking it succeeded.
13. As an app user, I want every Events notification to be exactly `[command, status, node address]`, so that my parser matches the spec.
14. As an app user, I want any pending events delivered promptly, not one every half second, so that the UI confirms writes quickly.
15. As an app user, I want a write with the wrong length for its command to be refused with an "invalid attribute value length" error, so that encoding bugs in the app show up straight away.
16. As an app user, I want an empty or oversized write to be refused, not treated as a zero-filled command, so that a bad write can never change the clock, durations or pairing state.
17. As an app user, I want a write with an unknown command byte to be refused, so that I get feedback instead of silence.
18. As an app user, I want the Configuration read to include the Head Status, so that I can show "pairing…", "watering", "standing by" or "faulty node".
19. As an app user, I want to know that Pairing has finished (Head Status leaves pairing), so that I can re-read Node Durations and Node States at the right moment.
20. As a gardener, I want my Watering Start Time to survive a power cut, so that watering keeps to my schedule after an outage.
21. As a gardener on a fresh install, I want the head to default to a 02:00 Watering Start Time, so that it waters even before I configure a time.
22. As an app user, I want a Watering Start Time outside 0–23 hours or 0–59 minutes to be rejected with `INVALID_TIME`, not silently wrapped, so that what I set is what runs.
23. As an app user, I want the head to advertise the Drippet service UUID, so that the app can scan for that service instead of matching on the device name.
24. As an app user, I want the advertisement to include standard discoverability flags, so that every phone's BLE stack can find the head.
25. As an app user, I want notification and indication subscriptions to reset when I disconnect, so that a reconnect starts clean and the head doesn't notify a dead connection.
26. As an app user, I want unsubscribing from one characteristic to actually stop its notifications or indications, so that the head respects what I asked for.
27. As an app user, I want BLE reads during Pairing or a watering run to return consistent data and never crash the head, so that the app can read at any time.
28. As an app user, I want BLE reads to stay responsive while the head restarts its own node during Pairing, so that the app doesn't time out.
29. As an app developer, I want the BLE docs to say that durations are in seconds, so that I build the duration UI with the right units.
30. As an app developer, I want the BLE docs to use the term Node Address consistently, so that the spec, the glossary and the code agree.
31. As an app developer, I want the docs to list the exact write lengths and the ATT errors returned, so that I can handle failures in the app.
32. As a developer, I want the Python BLE client to decode the 14-byte Configuration (including Head Status) and the 3-byte Events payload, so that the CLI keeps working as a test harness.
33. As a developer, I want the host test suite to pass, so that I can trust it while I build the app.
34. As a developer, I want the test helpers to set a node's duration and Cycle on the right Node Address, so that the tests check what they claim to.
35. As a developer, I want the UART task to queue the message itself, not the address of its `optional` wrapper, so that frame forwarding doesn't depend on undefined behaviour.
36. As a developer, I want saving durations to fail safely when NVS didn't initialise, not dereference a null handle, so that a flash problem doesn't crash the head.

## Implementation Decisions

**Pairing and frames (Head)**
- The Pairing DISCOVERY frame carries the 2-word key with its data length set, as the Python emulator already does.
- Pairing relies on daisy-chained downstream power, so only one unpaired node is ever on the bus. Sharing one tick-based key per round is therefore correct and stays.
- ACK and STATUS frames from a Node Address that has no node are ignored and logged. An ADDRESSING frame for an empty slot is treated as an invalid index (no assert).
- When the node table is full, an ADDRESSING frame that fails confirmation gets no reply. It never gets an ACK.
- Incoming STATUS values the head acts on: READY, WATERING, ERR, and INITIALIZING (a node that rebooted). Any other value is ignored and doesn't change the node's state.

**Watering run lifecycle (Head)**
- Every way a run can end returns nodes in IN_QUEUE or COMMAND_SENT to READY and clears their retry counts.
- **Faulty Node policy (decided: halt all):** when any node is ERR at the start of a run, no node is queued and the Head Status becomes FAULTY_NODE. That status lasts until the next Pairing. The ERR check runs before any node is queued, so nothing is left half-queued.

**Duration budget (Head)**
- A duration write that would push the total past the Watering Interval is rejected with `INVALID_TIME`. The node's duration, status and saved settings are left unchanged.
- **Units (decided):** durations are seconds everywhere.

**Request and event queue (ExtRqManager)**
- Requests and events are each a bounded FIFO, 16 entries deep, replacing the old one-slot-per-command-type arrays. A full request queue refuses the new entry, and the caller sees that.
- The manager guards its own queues with an internal lock, because the BLE host task writes and reads them while the head task processes them.
- Events are popped one at a time in the order they were produced.

**Head state locking (Head, HeadTask, BLELinkInterface)**
- `Head` owns one `std::mutex`. It works under ESP-IDF and in the host build, so `core/` stays free of ESP-IDF headers.
- The head task holds the lock for each loop step's state work and releases it before any blocking queue send.
- BLE reads of head state (Node Durations, Node States, Configuration) take the lock. BLE writes only touch the request queue, so they don't need it.
- **Deferred self-node restart:** starting Pairing no longer calls the restart callback inline. `Head` records that a restart is pending. The head task runs it after releasing the lock, so the restart (which can block for up to 10 s) never holds the lock.

**Watering Start Time (Esp32Clock, Storage, Head, boot)**
- Setting the Watering Start Time validates hour 0–23 and minute 0–59 and returns `INVALID_TIME` without changing anything when either is out of range. The `iClock` setter returns an `ActionStatus`.
- `Storage` gains save and read operations for the Watering Start Time. The head saves it after every successful `WRITE_CONF_TIME_PHASE`.
- At boot, storage initialises first. The clock's start time is then the saved one, or 02:00 when nothing is saved.
- Clock Sync is still required before any watering, and is not persisted.

**BLE wire contract (BLELinkInterface, GATT glue)**
- `handle_writes` receives the real write length and returns a result: accepted, invalid length, queue full, or unknown command.
- Exact lengths: `WRITE_CONF_TIME` 11 bytes, `WRITE_NODE_DURATION` 4 bytes, `WRITE_NODE_CYCLE` 3 bytes, `WRITE_CONF_TIME_PHASE` 3 bytes, `INIT_PAIRING` 1 byte.
- The GATT glue maps write results to ATT errors:
  - Invalid length or an empty/oversized write → `0x0D` Invalid Attribute Value Length.
  - Queue full → `0x11` Insufficient Resources.
  - Unknown command → `0x0E` Unlikely Error. (This is the agent's choice; any non-success code that the docs name is acceptable.)
- The Events payload is exactly 3 bytes: `[command, status, node address]`. For commands that apply to the whole controller, the address byte is 0.
- The Configuration read grows to **14 bytes**. Byte 13 is the Head Status: `PAIRING=0, STANDBY=1, FAULTY_NODE=2, WATERING_CMDS=3` (the enum's order).
- Every wire change ships together in the firmware, the Python client (`protocol`/`models`/`constants` and its tests), the BLE docs, and any app parser that already decodes these characteristics.

**Advertising and subscriptions (GAP, StatusTask)**
- The advertisement carries the Flags (general discoverable, BR/EDR not supported), the 128-bit Drippet service UUID in the service-UUID list, and the complete name "Drippet". That fits in 31 bytes.
- Appearance, LE role and the advertising interval move to the scan response.
- Subscribe events set the notify and indicate state from the current values reported for the characteristic that changed. They don't OR new values into the old ones.
- A disconnect clears both flags and the connection handle.
- The status task drains Events by notifying repeatedly while events are pending. It stops if a notify fails, so it can't spin.

**Small fixes**
- The UART task queues the parsed message itself, not the address of its `optional` wrapper.
- `save_durations` returns an error without touching the NVS handle when storage didn't initialise.
- Remove the stray function declaration inside the pairing-exit helper.
- The test helpers pass (address, value) in the right order.

**Docs and glossary**
- `ble_reads.md` / `ble_writes.md`: durations in seconds; "Row Index" and "Node Index" become Node Address; Configuration is 14 bytes with Head Status; exact write lengths and ATT errors; the queue limit; the Watering Start Time persists across reboots.
- The migration doc records that item 2.10 (write length) is done.
- `GLOSSARY.md` already defines Node, Node Address, Pairing, Head Status and Faulty Node (added during the review).

## Testing Decisions

- Good tests drive behaviour through a module's public interface and assert on what is visible from outside: returned frames, node statuses, Head Status, emitted events, serialized BLE buffers, and calls on the mocked clock and storage. They don't reach into private fields or count internal steps.
- **Seam 1: `Head` public interface**, with the FakeIt `iClock` and `Storage` mocks (prior art: the head, mocks and time-pool tests).
  - Pairing emits DISCOVERY with its key and data length.
  - ACK/STATUS/ADDRESSING from unknown addresses return nothing and change nothing.
  - A full table never ACKs.
  - Runs that end early, and a node reporting INITIALIZING, leave no node in IN_QUEUE or COMMAND_SENT.
  - An ERR node stops all queueing and gives FAULTY_NODE.
  - An over-budget duration leaves the old duration and status unchanged and emits `INVALID_TIME`.
  - A Watering Start Time write saves to storage only on success.
  - Starting Pairing marks a restart as pending, and running the pending restart calls the callback exactly once.
- **Seam 2: `BLELinkInterface`** (prior art: the BLE link test case).
  - Each command's write is accepted at exactly its length and rejected one byte short or one byte long.
  - Unknown commands are rejected.
  - The 17th queued request is refused.
  - Several writes of the same command type are all processed, and their events pop in FIFO order.
  - Events are 3 bytes.
  - Configuration is 14 bytes with the correct Head Status byte.
  - Existing tests that write the fixed 20-byte mock arrays must pass exact-length spans.
- **Seam 3: `Esp32Clock`** with the `iSysClock` mock (prior art: the clock tests). Out-of-range hour or minute returns `INVALID_TIME` and leaves the stored start time and the next watering point unchanged.
- The Python client's pytest suite gets updated fixtures for the 14-byte Configuration and the 3-byte Events.
- **Hardware only, no host seam:**
  - Advertising contents and service-filtered scanning from a phone.
  - ATT error codes as the phone sees them.
  - Subscription reset across disconnect and reconnect.
  - Events draining.
  - UART forwarding.
  - The NVS guard.
  - Lock behaviour while BLE reads overlap Pairing.
  Check these with the Python BLE client and the app against a flashed board.
- Done means the full host suite (`./build.clang/tests/tests`) and `pytest` pass.

## Out of Scope

- Changing the Faulty Node policy to skip a faulty node or retry it (decided: halt all).
- Persisting Clock Sync or the UTC Offset across reboots. The client resends them on every connect.
- Building app features beyond updating parsers that already exist for the changed characteristics.
- Renaming the `phase_*` code names to the glossary terms.
- Switching `DEV_PHASE_LEN` to production (86400).
- Aligning the Node-count constants between C++ (12) and Python (5).
- Enforcing the per-hose maximum duration beyond the total-budget check.
- RS485 echo handling on the head's own transmissions.

## Further Notes

- The first three host test failures come from the swapped helper arguments, not from the firmware. Fix those first so the suite becomes a trustworthy baseline before the other changes.
- The Configuration and Events changes break wire compatibility with older builds of the clients. Flash the firmware and update the clients together.
