# Clock Migration: Unix Timestamp + UTC Offset

This is the checklist for replacing the `hour:minute` system clock sync with a full UTC Unix timestamp plus the phone's UTC offset.

## Why

- `Esp32Clock::set_time()` always rebuilds the clock on the fixed date `DEFAULT_YEAR/MONTH/DAY`, so every resync moves the device back to 2026-12-12.
- The re-align loop in `set_time()` only moves `next_watering_point` **forward**. When the clock moves backwards, the next watering stays days in the future.
- When the clock moves forwards across several phases, `next_watering_point` advances but `phase_of_cycle` does not.
- `makeTime()` leaves some `struct tm` fields uninitialized (`tm_isdst`, etc.), so `mktime` can shift the result by an hour.

## Design Summary

| Concern | Decision |
| --- | --- |
| System clock (`settimeofday`) | Always **true UTC**. `TZ=UTC` stays. |
| Local time | `local = utc + utc_offset`. The clock stores the offset and applies it only when converting to/from `hour:minute`. |
| DST | Handled by the phone, which resends the timestamp + current offset on every connect / sync. |
| Timestamp width | `int64` seconds, little-endian (matches ESP-IDF 5.x 64-bit `time_t`, no 2038 limit). |
| Offset width | `int16` **minutes**, little-endian (covers +05:30, +05:45, etc.). Valid range `-720 … +840`. |
| Phase start time (`WRITE_CONF_TIME_PHASE`) | Unchanged on the wire: still **local** `hour:minute`. It is a time of day, not a date. |
| Next watering | Recomputed from a stored local `phase_start` `HourMin` every time the clock or the phase start changes. The loop that only moved it forward is removed. |

---

## 1. Wire Protocol

### 1.1 `WRITE_CONF_TIME` (write, Configuration characteristic)

New payload, **11 bytes**:

| Byte | Description |
| --- | --- |
| 0 | Command (`WRITE_CONF_TIME` = 0) |
| 1-8 | UTC Unix seconds (`int64_t`, little-endian) |
| 9-10 | UTC offset in minutes (`int16_t`, little-endian) |

Example: `2026-09-28 20:30:00 UTC`, offset `-07:00` (`-420`):

```text
00  48 CE BA 6A 00 00 00 00  5C FE     (ts = 1790627400)
```

- [ ] Choose the constants and add them to `drippet_head/main/core/include/ble_types.hpp`:
  - [ ] `CONF_TIME_TS_IDX = HEADER_LEN` (1)
  - [ ] `CONF_TIME_OFFSET_IDX = HEADER_LEN + 8` (9)
  - [ ] `CONF_TIME_PKT_LEN = HEADER_LEN + 8 + 2` (11)
  - [ ] `MIN_VALID_EPOCH` (e.g. `1767225600` = 2026-01-01 UTC)
  - [ ] `MIN_UTC_OFFSET_MIN = -720`, `MAX_UTC_OFFSET_MIN = 840`
- [ ] Leave `HOUR_IDX` / `MINUTE_IDX` in place. `WRITE_CONF_TIME_PHASE` still uses them.
- [ ] Decide whether the old stale comment block at the top of `ble_types.hpp` (`[CMD, LEN, ...]`) should be removed or updated. It doesn't describe the current format.

### 1.2 `WRITE_CONF_TIME` event response

- [ ] The event can now return `INVALID_TIME` (1) when the timestamp is below `MIN_VALID_EPOCH`, the offset is out of range, or the packet is shorter than `CONF_TIME_PKT_LEN`. Today it always returns `OK`.

### 1.3 `READ_CONFIGURATION` (read, Configuration characteristic)

New payload, **13 bytes** (fits easily in `MTU_SIZE = 110`). Index constants: `BLE::READ_CONF_*` in `ble_types.hpp`.

| Byte | Constant | Description |
| --- | --- | --- |
| 0-7 | `READ_CONF_TS_IDX` | Device's current time, true UTC Unix seconds (`int64_t`, LE). Offset **not** applied |
| 8-9 | `READ_CONF_OFFSET_IDX` | Stored UTC offset in minutes (`int16_t`, LE) |
| 10 | `READ_CONF_WATER_SET_IDX` | Watering time set flag (0/1) |
| 11 | `READ_CONF_WATER_HOUR_IDX` | Watering hour, **local** (0-23). 0 if flag is 0 |
| 12 | `READ_CONF_WATER_MIN_IDX` | Watering minute, **local** (0-59). 0 if flag is 0 |

- No clock set flag: the client always sends `WRITE_CONF_TIME` on connect, so the clock is set before any read.
- The watering time is sent as the stored local `phase_start` hour/minute, not as a next-watering timestamp. It reads back exactly what `WRITE_CONF_TIME_PHASE` wrote, and with weekday-based phases the next daily trigger isn't necessarily a day anything waters.
- The cycle phase byte is dropped. The client derives it from the local weekday (Sunday = 0).
- [ ] The app can auto-resync when `|device_ts - phone_ts|` exceeds a threshold (e.g. 60 s) or when the offset differs.

---

## 2. Firmware (`drippet_head/`)

### 2.1 `core/include/constants.hpp`

- [ ] Fix `Time::Day_In_Seconds`. `Time_Seconds` is `uint16_t`, so `86400` overflows to `20864`. Make it `Time::Long` (or `int32_t`), or remove it if it stays unused.
- [ ] Add `using Epoch = int64_t;` and `using UtcOffsetMin = int16_t;` to `namespace Time` so the types are named once.

### 2.2 `core/include/util.hpp`

- [ ] Add `put_le64(void *buf, uint64_t x)` and `get_le64(const void *buf)` next to the existing `le16`/`le32` helpers.
- [ ] Add a signed `get_le16`-style read for the offset (or cast: `static_cast<int16_t>(Util::get_le16(...))`).
- [ ] Add unit tests for the new helpers in `tests/utils_test.cpp` (including a negative `int64` and a negative `int16` round-trip).

### 2.3 `core/include/clock.hpp`

- [ ] Change the `iClock` interface:
  - [ ] `virtual time_t set_time(int hour, int min) = 0;` → `virtual ActionStatus set_time(Time::Epoch utc_seconds, Time::UtcOffsetMin offset_min) = 0;`
  - [ ] Add `virtual Time::UtcOffsetMin get_utc_offset() const = 0;`
  - [ ] Add `virtual std::optional<HourMin> get_phase_start() const = 0;` (needed by the new read layout).
  - [ ] Add `virtual bool is_synced() const = 0;` (if doing step 2.9).
- [ ] In `Esp32Clock`, add members:
  - [ ] `std::chrono::minutes utc_offset{0};`
  - [ ] `std::optional<HourMin> phase_start = std::nullopt;` (local time of day the watering starts)
- [ ] Add a private `void recompute_next_watering();`.
- [ ] Add private helpers `Time::Time_Point to_local(Time::Time_Point utc) const` / `from_local(...)` (just `± utc_offset`).
- [ ] Rename `initalized` → `initialized` (it's currently misspelled and never read).
- [ ] Remove `DEFAULT_YEAR/MONTH/DAY`, or replace them with a single `constexpr Time::Epoch BOOT_DEFAULT_EPOCH` used only at boot.

### 2.4 `core/src/clock.cpp`

- [ ] **`makeTime`**: initialize `tm` with `struct tm tm{};`. Better: replace it with `std::chrono` calendar types (`sys_days{year/month/day} + hours{h} + minutes{m}`), which never touch `TZ`. The project already uses `std::chrono::days` / `hh_mm_ss`, so C++20 chrono is available.
- [ ] **`set_time(epoch, offset_min)`**:
  - [ ] Validate `epoch >= MIN_VALID_EPOCH` and the offset range. Return `ActionStatus::INVALID_TIME` on failure without touching the clock.
  - [ ] `settimeofday({.tv_sec = epoch, .tv_usec = 0})` via `sys.change_system_clock`.
  - [ ] Store `utc_offset = minutes{offset_min}`.
  - [ ] Set `initialized = true`.
  - [ ] Call `recompute_next_watering()`.
  - [ ] **Delete** the `while (next_watering_point < t_point)` loop that only moved forward.
- [ ] **`set_next_phase_start_time(hour, min)`**:
  - [ ] Store `phase_start = HourMin{hour % 24, min % 60}`.
  - [ ] Call `recompute_next_watering()`.
  - [ ] Remove the `localtime_r`/`mktime` logic (it assumed `TZ` = local time, which is no longer true).
- [ ] **`recompute_next_watering()`**:
  - [ ] If `!phase_start` → `next_watering_point = nullopt`, return.
  - [ ] `local_now = now() + utc_offset`
  - [ ] `local_midnight = floor<days>(local_now)`
  - [ ] `candidate_local = local_midnight + hours{phase_start->hour} + minutes{phase_start->minute}`
  - [ ] Convert back: `candidate = candidate_local - utc_offset`
  - [ ] `while (candidate <= now()) candidate += seconds{phase_length};`
  - [ ] `next_watering_point = candidate`
  - [ ] Note: with `DEV_PHASE_LEN = 60` this still works (it steps in 60 s from today's start time). With 1-day phases it lands on today or tomorrow.
- [ ] **`cast_hour_min`**: convert to local first (`time_point + utc_offset`) before `floor<days>`. That fixes `get_hourmin_curr_time()` and `get_hourmin_next_phase()` for non-UTC users.
- [ ] **`progress_watering_due()`**: no change needed. It still steps by `phase_length` and advances the phase. Double-check it after the phase decision in 2.5.
- [ ] Implement `get_utc_offset()`, `get_phase_start()` (const), and `is_synced()` if doing step 2.9.

### 2.5 Phase of cycle (decision required)

The cycle bitmask is Sunday–Saturday and `CyclePhase` has 7 values, so the phase is effectively "weekday" once phases are 1 day long.

- [ ] **Decide:**
  - [ ] **Option 1 (recommended):** when `phase_length == 86400`, derive the phase from the weekday of `next_watering_point` in local time. Do it inside `recompute_next_watering()` (`weekday{floor<days>(local)}.c_encoding()` gives Sunday = 0, which matches the bitmask). Keep `WRITE_CONF_PHASE` only for dev mode (`DEV_PHASE_LEN`).
  - [ ] **Option 2:** keep the phase as a counter. In `recompute_next_watering()`, when a forward clock jump skips `N` phases, advance `phase_of_cycle` by `N % CYCLE_LEN`. Leave backward jumps alone.
- [ ] If Option 1: remove the second `WRITE_CONF_PHASE` write from the Python `SYNC_TIME` action (see 3.3).

### 2.6 `core/include/external_requests.hpp`

`ExtRequest::data` is `std::array<uint16_t, 4>`. An `int64` fills all four slots and leaves no room for the offset.

- [ ] Add a dedicated payload to `ExtRequest`, e.g.
  ```cpp
  struct ClockSync { Time::Epoch utc_seconds; Time::UtcOffsetMin offset_min; };
  std::optional<ClockSync> clock_sync{};
  ```
  Only `WRITE_CONF_TIME` uses it.
- [ ] Remove `REQ_HOUR_INPUT`/`REQ_MIN_INPUT` only if nothing else uses them. `WRITE_CONF_TIME_PHASE` still does, so **keep them**.

### 2.7 `core/include/head.hpp`

- [ ] `ext_req_set_clock(uint8_t hour, uint8_t minute)` → `ext_req_set_clock(Time::Epoch utc_seconds, Time::UtcOffsetMin offset_min)`. Fill `req.clock_sync`.
- [ ] Add pass-through getters used by the new read: `get_utc_offset()`, `get_phase_start()`, and something that returns the current **UTC** `Time_Point` (e.g. `clock_now()`). The read must send UTC, so don't use `get_curr_time(true)` for it.
- [ ] Decide whether `get_hourmin_curr_time()` / `get_hourmin_next_phase()` are still needed once the read response sends timestamps. Keep them if logs/other tasks use them.

### 2.8 `core/src/head.cpp` (`process_external_requests`, ~line 430)

- [ ] `case BLE::Cmds::WRITE_CONF_TIME:`
  - [ ] Guard: if `!req.clock_sync` → put an `INVALID_TIME` event and `break`.
  - [ ] `ActionStatus status = clock.set_time(req.clock_sync->utc_seconds, req.clock_sync->offset_min);`
  - [ ] Put an event with the real `status` (it's hard-coded to `OK` today).
- [ ] `case BLE::Cmds::WRITE_CONF_TIME_PHASE:` remove the leftover `Logger::log_error("NOT AN ERROR");`.

### 2.9 Boot behaviour: `tasks/head_entrypoint.cpp`

- [ ] Replace `clock.set_time(12, 0)` with a boot default, e.g. `clock.set_time(BOOT_DEFAULT_EPOCH, 0)`. Don't let that call set `initialized = true`: split out a `set_boot_default()` or pass a flag.
- [ ] **Decide** what should happen before the first sync after a power loss (there's no RTC):
  - [ ] **Option A (recommended):** `is_watering_due()` returns `false` until `is_synced()`. That means no watering on a made-up clock.
  - [ ] **Option B:** water on the boot-default clock anyway (runs at a fake time, but the plants still get water).
- [ ] Keep `setenv("TZ", "UTC", 1); tzset();`. It's now a real requirement, since the system clock is UTC.
- [ ] `set_next_phase_start_time(2, 0)` at boot is fine. It is now stored as `phase_start` and re-applied on sync.

### 2.10 BLE parsing: `ble/gatt_service.cpp` and `core/src/ble_link_interface.cpp`

- [ ] `BleReadRes` (`core/include/gatt_service.hpp`) has no length. Add `size_t len;` and fill it in `GattService::read_incoming_data`.
- [ ] Change `BLELinkInterface::handle_writes(std::span<const uint8_t>)` to receive only the valid bytes (`std::span(raw_data.data(), len)`).
- [ ] `case BLE::Cmds::WRITE_CONF_TIME:`
  - [ ] If `raw_data.size() < CONF_TIME_PKT_LEN` → log and queue an `INVALID_TIME` event (or ignore).
  - [ ] `auto ts = static_cast<int64_t>(Util::get_le64(&raw_data[CONF_TIME_TS_IDX]));`
  - [ ] `auto off = static_cast<int16_t>(Util::get_le16(&raw_data[CONF_TIME_OFFSET_IDX]));`
  - [ ] `head_node.ext_req_set_clock(ts, off);`
- [ ] While you're there: add minimum-length checks for the other commands too (`WRITE_NODE_DURATION` reads index 3, etc.).
- [ ] `case BLE::Read_T::READ_CONFIGURATION:` rebuild with the new layout from 1.3, using `Util::put_le64` / `put_le16`. Update the explanatory comment block.
- [ ] Update `buffer.len` to `BLE::READ_CONF_PKT_LEN` (13).

---

## 3. Python Client (`drippet_py/`)

### 3.1 `ble/protocol.py`

- [ ] `writeConfTime(hour, minute)` → `writeConfTime(utc_seconds: int, offset_min: int) -> bytes`:
  ```python
  return bytes([BleCommand.WRITE_CONF_TIME]) + struct.pack("<qh", utc_seconds, offset_min)
  ```
- [ ] `read_sys_conf(data)`:
  ```python
  now_ts, offset, has_water, hour, minute = struct.unpack_from("<qhBBB", data)
  ```
  Build `datetime`s with `timezone(timedelta(minutes=offset))`.

### 3.2 `ble/models.py`

- [ ] `SysConfig`: change `sys_time: HourMin` → `sys_time: datetime`. Keep `next_watering: HourMin | None` (local watering time). Add `utc_offset_min: int`. Drop the phase field or derive it from `sys_time`.
- [ ] Update `SysConfig.__str__` to show the date, e.g. `Sat Sep 28 8:30 PM`.
- [ ] Keep `HourMin`. It's still used for phase start time input.

### 3.3 `ble/task.py`

- [ ] Add a helper:
  ```python
  def current_utc_and_offset() -> tuple[int, int]:
      now = datetime.now().astimezone()
      return int(now.timestamp()), int(now.utcoffset().total_seconds() // 60)
  ```
- [ ] `handle_write_sys_time(hour, minute)` → `handle_write_sys_time(utc_seconds, offset_min)`.
- [ ] `SYNC_TIME` action: send `current_utc_and_offset()`. If you chose phase Option 1 (2.5), **remove** the follow-up `handle_write_curr_phase(weekday)` write.
- [ ] `SET_TIME` (manual) action: prompt hour/minute, build today's local `datetime` with those values, convert it to `(utc_seconds, offset_min)`, and send. Or drop the manual action entirely.
- [ ] **Auto-sync on connect:** right after connecting, send `SYNC_TIME` once. This is what keeps DST correct.
- [ ] Optional: after a `READ_CONFIGURATION`, compare the device's timestamp/offset with the phone's and resync if drift > 60 s or the offset changed.

### 3.4 `constants.py`

- [ ] No enum value changes needed (`WRITE_CONF_TIME` stays `0`).

---

## 4. Tests

### 4.1 C++ (`tests/`)

- [ ] `fixtures.hpp` (`ClockFixture`):
  - [ ] Make the `change_system_clock` mock actually update `curr_time` (`AlwaysDo([this](const timeval &tv){ curr_time = system_clock::from_time_t(tv.tv_sec); })`). Otherwise `recompute_next_watering()` can't be tested against the new time.
  - [ ] `set_test_time(hour, min)` currently calls `espClock.set_time(3, 3)`. Rewrite it to take an epoch (plus an optional offset).
  - [ ] `make_ex_time` uses `makeTime(2026, 3, 3, ...)`. Keep it or switch it to chrono calendar types.
  - [ ] `fixtures.hpp:50` `When(Method(clockMock, set_time)).AlwaysReturn();`: update for the new signature/return type.
- [ ] `clock_test.cpp`: update existing cases and add:
  - [ ] Resync **backwards** by 2 days → next watering is the next occurrence after the new now (bug #2 regression).
  - [ ] Resync **forwards** across several phases → next watering correct; phase follows the 2.5 decision.
  - [ ] Resync to exactly the watering minute → next watering is **the next** phase (`<=` check).
  - [ ] Non-zero offset (e.g. `-420`): `get_hourmin_curr_time()` returns local hour, and a `phase_start` of `02:00` local lands at `09:00 UTC`.
  - [ ] Offset change only (same epoch, `-420` → `-480`, i.e. DST) → next watering shifts by 1 h.
  - [ ] Invalid epoch / offset → `INVALID_TIME`, clock untouched.
  - [ ] `set_next_phase_start_time` before any `set_time` still produces a point, and it's recomputed after `set_time`.
  - [ ] (If 2.9 Option A) `is_watering_due()` is `false` before the first sync.
- [ ] `ble_test.cpp`:
  - [ ] `BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_CONF_TIME)` (~line 57): build the 11-byte payload.
  - [ ] ~line 93 `Verify(Method(fix.clockMock, set_time)...)`: verify the epoch/offset args.
  - [ ] ~line 160 switch case for `WRITE_CONF_TIME`: update.
  - [ ] ~lines 228/243 read-config mocks (`get_hourmin_curr_time`, `get_hourmin_next_phase`): switch to the new getters and assert the 13-byte layout.
  - [ ] Add a short-packet test (length < 11 → no clock change, `INVALID_TIME` event).
- [ ] `utils_test.cpp`: `le64` round-trip tests (2.2).

### 4.2 Python (`drippet_py/tests/`)

- [ ] `ble_mocks.py`: `write_conf_time(hour, minute)` → `write_conf_time(utc_seconds, offset_min)`, and `read_config(...)` → build the new 13-byte layout.
- [ ] `ble_test.py`:
  - [ ] `testWriteSetTime`: use a fixed epoch + negative offset.
  - [ ] `testReadConfig`: assert the new `SysConfig` with a `datetime` current time, both with and without a watering time.
  - [ ] Add a test for `current_utc_and_offset()` with a patched timezone.

---

## 5. Docs

- [ ] `docs/ble_writes.md`:
  - [ ] Rewrite the `WRITE_CONF_TIME` section with the 11-byte layout and example from 1.1.
  - [ ] Update the Summary table: `WRITE_CONF_TIME` → 11 bytes.
  - [ ] Note that `WRITE_CONF_TIME_PHASE` hour/minute is **local** time.
  - [ ] Remove the duplicated "BLE Command Payload Format" heading/intro (it appears twice).
  - [ ] If phase Option 1: note that `WRITE_CONF_PHASE` is dev-only / derived automatically.
- [ ] `docs/ble_reads.md`:
  - [ ] Rewrite the Configuration section with the layout from 1.3 and new examples.
  - [ ] Remove the stray 7-byte example (`14 1E 02 00 01 16 00`) and the empty code fence above it.
  - [ ] Events: note that `WRITE_CONF_TIME` can now return `INVALID_TIME`.

---

## 6. Rollout / Verification

- [ ] Flash the firmware and update the Python client together. The payload sizes change, so old/new versions are **not** compatible.
- [ ] On hardware: sync, read config, confirm the date/time/offset shown by the client matches the phone.
- [ ] Set `phase_start` 2 minutes ahead, confirm watering fires once and the next watering moves forward by one phase.
- [ ] Resync with a timestamp 2 days in the past, then back to now. Confirm the next watering is always the next upcoming occurrence.
- [ ] Power-cycle the board: confirm the chosen 2.9 behaviour, and that the first sync after connecting restores the correct schedule.
- [ ] Switch `DEV_PHASE_LEN` → 86400 (1-day phases) when ready, and re-run the clock tests with that value.
