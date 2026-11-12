# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Drippet is an ESP32-based drip irrigation system. The repo holds three separate projects that share one wire protocol:

| Dir | What | Stack |
| --- | --- | --- |
| `drippet_head/` | Firmware for every ESP32 board (head and nodes run the same binary) | C++20, ESP-IDF, FreeRTOS, NimBLE |
| `drippet_py/` | Dev/test tooling: BLE CLI client and a UART emulator for the head | Python, bleak, pyserial, pytest |
| `drible/` | Mobile app that controls the head over BLE | Expo / React Native, `react-native-ble-plx`, expo-router |

`drible/` has its own `CLAUDE.md` (→ `AGENTS.md`): **Expo has changed. Read the versioned docs at https://docs.expo.dev/versions/v57.0.0/ before writing Expo code.**

## Commands

### Firmware (ESP-IDF, run from `drippet_head/`)
```sh
idf.py build
idf.py -p <PORT> flash monitor
```
`drippet_head/main/CMakeLists.txt` registers sources by glob (`core/src`, `core/env/esp32`, `tasks`, `ble`). New `.cpp` files in those dirs are picked up automatically; re-run cmake if one isn't.

### C++ host unit tests (run from repo root)
The root `CMakeLists.txt` is a **host** build, separate from ESP-IDF. It compiles `drippet_head/main/core/src/*.cpp` + `core/env/host/*.cpp` into the `HEAD_SRC` library and links it into `tests/` (Catch2 v3 + FakeIt, fetched via FetchContent). `tests/` picks up every `*_test.cpp`. Coverage is ON by default and needs clang/LLVM.
```sh
cmake -S . -B build.clang -G Ninja && cmake --build build.clang
./build.clang/tests/tests                      # all tests
./build.clang/tests/tests "[clock]"            # by tag
./build.clang/tests/tests "test case name"     # single test
cmake --build build.clang --target coverage_report   # HTML in build.clang/tests/coverage_report
```
Code in `core/` must stay free of ESP-IDF headers so it builds on host. Hardware/RTOS access sits behind interfaces (`iClock`, `Driver`, `Storage`, `Switch`, system-clock hooks), and tests mock them with FakeIt (`tests/fixtures.hpp`, `tests/mocks.hpp`).

### Python (run from `drippet_py/`, venv in `drippet_py/venv`)
```sh
pip install -e . && pip install -r requirements.txt
python -m ble          # interactive BLE client: finds the device named "Drippet" and shows a menu
python main.py         # pretends to be the head over a USB serial port (HeadUart) to drive a real node
pytest                 # all tests
pytest tests/ble_test.py::testWriteDurations
```

### App (run from `drible/`)
```sh
npm start              # expo start (needs a dev client; BLE won't work in Expo Go)
npm run ios | npm run android    # expo run:* (native build)
npm run lint
```

## Firmware architecture

**Role selection at boot** (`main/drippet.cpp`): every board starts as a node (`SelfNodeTask`, `NodeMode::STANDARD`) and listens on UART for `LISTEN_FOR_INIT_COMMS_DUR` (10 s). If no head talks to it, it becomes the **head**: UART switches to dual mode, and `HeadEntrypointTask` takes over. It destroys and rebuilds the self-node task in `NodeMode::DUAL` on internal queues, so the head board also waters its own hose as a node. It also starts the head, BLE, and status tasks.

**Layers:**
- `core/` is plain C++ logic. `Head` (`head.hpp/cpp`) owns the node table (`config::max_nodes`), pairing, watering scheduling, and the external-request queue (`ExtRqManager`). `SelfNode` is the node-side state machine. `UartProtocol` frames/parses the RS485/UART protocol. `Esp32Clock` does schedule timing. `BLELinkInterface` turns BLE bytes into `Head` requests and serializes read responses.
- `core/env/esp32/` holds the ESP-IDF implementations (UART driver, NVS storage, GPIO switches, queues, logger). `core/env/host/` holds host stubs.
- `tasks/` holds FreeRTOS wrappers. Each subclasses `Task` and overrides `run()`. Tasks talk only through FreeRTOS queues handed out by `QueueManager` (`QueueOrder::EXT_*`, `INTERNAL_*`).
- `ble/` is the NimBLE GAP/GATT glue. It calls into `BLELinkInterface`.

**Threading rule:** BLE writes never change `Head` state directly. They queue an `ExtRequest`. `Head::process_external_requests` applies it on the head task, and the result goes back as a BLE Events notification (`[cmd, status, node_idx]`).

**UART protocol** (`protocol_types.hpp`, mirrored in `drippet_py/constants.py` and `transport/uart.py`): frame = `0xAA, address, command, data_len, data (uint16 words), CRC16`. The head is address `0x00`. Commands: `DISCOVERY, ADDRESSING, INIT_WATER_DURATIONS, ACK, STATUS, BUGGER_OFF`.

## BLE protocol (shared by firmware, Python, and app)

The full byte layouts are in `docs/ble_reads.md` and `docs/ble_writes.md`. Treat those docs as the spec, and when you change the wire format, update all three clients and the docs together:
- Firmware: `core/include/ble_types.hpp`, `core/src/ble_link_interface.cpp`
- Python: `drippet_py/ble/protocol.py`, `constants.py`, `ble/models.py`
- App: `drible/src/constants/gatt-chars.ts`, `src/lib/ByteIf.ts`, `src/contexts/ble-manager-context.tsx`

Key points:
- Service `00020025-…`. Characteristics: Node Durations `00…` (read and per-node writes), Node States `01…` (indicate), Configuration `02…` (read and controller writes), Events `03…` (notify).
- Write byte 0 is always the `Cmds` enum: `WRITE_CONF_TIME=0, WRITE_NODE_DURATION=1, WRITE_NODE_CYCLE=2, WRITE_CONF_TIME_PHASE=3, INIT_PAIRING=4`.
- Everything is little-endian. Cycle bitmask: bit 0 = Sunday … bit 6 = Saturday.
- **Time model:** the device's system clock is true UTC (`TZ=UTC`). `WRITE_CONF_TIME` sends `int64` UTC seconds and an `int16` UTC offset in minutes (-720..840), 11 bytes total. The client must resend it on every connect, which is how DST stays correct. The watering start time (`WRITE_CONF_TIME_PHASE`) is a **local** hour:minute. The cycle phase is the local weekday.

## Time model history

The clock moved from `hour:minute` to a UTC timestamp plus offset. `docs/clock_timestamp_migration.md` is the design record and opens with the decisions taken. `DEV_PHASE_LEN = 60` (60-second "days") is still the dev setting. Production is 86400. Under dev the watering fires every minute, but the Cycle Day is still the real local weekday. Domain terms (Watering Start Time, Cycle, Cycle Day, Clock Sync) are in `GLOSSARY.md`.

## Gotchas
- Node-count constants differ between projects: C++ `config::max_nodes = 12` (and `ADDR_UNSET = max_nodes`), Python `MAX_NODE_COUNT = ADDR_UNSET = 5`. Check both before you change addressing.
- `boop.cpp` and `drippet_head/main/core/main.cpp` are scratch files, not entry points. The firmware entry point is `app_main` in `drippet_head/main/drippet.cpp`.
- `.clangd` strips the Xtensa-only GCC flags so clangd can use the ESP-IDF `compile_commands.json`.

## Agent skills

### Issue tracker

Issues are local markdown files under `.scratch/<feature>/`. See `docs/agents/issue-tracker.md`.

### Triage labels

Default labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`), recorded as a `Status:` line in each issue file. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `GLOSSARY.md` and `docs/adr/` at the repo root, covering all three subprojects. See `docs/agents/domain.md`.
