# Time Architecture Migration: Remaining Work

What's left to finish moving from the `hour:minute` + default-date clock to a UTC Unix timestamp + UTC offset.

**Model:** the system clock holds **true UTC**. The offset is stored in `iClock::UTC_OFFSET_MIN` and applied only where local time matters (watering time, weekday phase). BLE always sends raw UTC; the client adds the offset.

**Done:**
- `ble_types.hpp`: `CONF_TIME_TS_IDX`, `CONF_TIME_OFFSET_IDX`, `CONF_TIME_PKT_LEN`, `READ_CONF_*`.
- `util.hpp`: `put_le64` / `get_le64`.
- `READ_CONFIGURATION` writes the 13-byte layout using `Head::get_curr_utc_time()` and `Head::get_utc_offset_min()`.
- `iClock::set_time(time_t, int offset_min)` returns `ActionStatus` and validates input.
- `get_phase_of_cycle()` derives the phase from the weekday.
- `docs/ble_writes.md` and `docs/ble_reads.md` describe the new formats.

Steps 1–9 make the feature work on the device. The rest is cleanup, tests and the client.

---

## Firmware: Write Path (blocker, nothing reaches the clock yet)

- [ ] **1. Finish `WRITE_CONF_TIME` parsing** (`core/src/ble_link_interface.cpp:19-25`).
  - Wrap the case body in `{ }`. `time` and `min_offset` are declared in a `case` without braces, so jumping to the later cases crosses their initialization (compile error).
  - Cast the offset as signed. `get_le16` returns `uint16_t`, so `-420` currently becomes `65116`:
    ```cpp
    auto ts = static_cast<int64_t>(Util::get_le64(&raw_data[BLE::CONF_TIME_TS_IDX]));
    auto off = static_cast<int16_t>(Util::get_le16(&raw_data[BLE::CONF_TIME_OFFSET_IDX]));
    ```
  - Uncomment / call `this->head_node.ext_req_set_clock(ts, off);`.
- [ ] **2. Change `ext_req_set_clock`** (`core/include/head.hpp:177`). It still takes `(uint8_t hour, uint8_t minute)`. Change it to `(int64_t ts, int16_t offset_min)`. `ExtRequest::data` (four `uint16_t`) can't hold 64 + 16 bits, so add a field to `ExtRequest` (`core/include/external_requests.hpp`):
  ```cpp
  struct ClockSync { int64_t ts; int16_t offset_min; };
  std::optional<ClockSync> clock_sync{};
  ```
- [ ] **3. Use the new request in `process_external_requests`** (`core/src/head.cpp:420-423`).
  - It still calls `set_time(req.data[REQ_HOUR_INPUT], req.data[REQ_MIN_INPUT])`.
  - If `!req.clock_sync`, put an `INVALID_TIME` event and `break`.
  - Otherwise call `clock.set_time(req.clock_sync->ts, req.clock_sync->offset_min)` and put the returned `ActionStatus` in the event (it's hard-coded to `OK`).
  - Remove the leftover `Logger::log_error("NOT AN ERROR");` in the `WRITE_CONF_TIME_PHASE` case (`head.cpp:412`).

## Firmware: Clock (`core/src/clock.cpp`, `core/include/clock.hpp`)

- [ ] **4. Fix `set_time`** (`clock.cpp:24-44`).
  - `adjusted = time_point + offsetMin` adds minutes as seconds, and then both the validation and `settimeofday` use `adjusted`. Validate and set the clock with the raw `time_point`.
  - Delete the `while (next_watering_point < t_point)` loop. It only moves forward, and on the first sync after boot it walks 1970 → 2026 in 60-second `DEV_PHASE_LEN` steps (effectively hangs).
  - Call `recompute_next_watering()` (step 5) instead.
- [ ] **5. Store the watering time as local and recompute from it.**
  - Add a member `std::optional<HourMin> phase_start` to `Esp32Clock`.
  - Add a private `recompute_next_watering()`:
    - if `!phase_start` → `next_watering_point = nullopt`, return
    - `local_now = now() + minutes{UTC_OFFSET_MIN}`
    - `candidate = floor<days>(local_now) + hours{phase_start->hour} + minutes{phase_start->minute} - minutes{UTC_OFFSET_MIN}`
    - `while (candidate <= now()) candidate += seconds{phase_length};`
  - `set_next_phase_start_time` (`clock.cpp:73-98`): store `phase_start = {hour % 24, min % 60}` and call `recompute_next_watering()`. Delete the `localtime_r` / `mktime` code: with `TZ=UTC` it treats the hour as UTC, so `02:00` at offset `-420` fires at 19:00 local.
- [ ] **6. Apply the offset where local time is needed.**
  - `cast_hour_min` (`clock.cpp:56`): add `minutes{UTC_OFFSET_MIN}` before `floor<days>`.
  - `get_phase_of_cycle()` (`clock.hpp:70-74`): same, otherwise the weekday is the UTC weekday.
- [ ] **7. `get_hourmin_next_phase()` returns `phase_start`** instead of casting `next_watering_point`. The read currently sends the UTC hour/minute of the next watering, but the read layout promises the local time "exactly as written".
- [ ] **8. Don't water before the first sync.** `initalized` is set in `set_time` but never read. Make `is_watering_due()` return `false` until `set_time` succeeds. (Optionally rename to `initialized`.)
- [ ] **9. Fix boot** (`tasks/head_entrypoint.cpp:7`). Remove `this->clock.set_time(12, 0)`. It still compiles but now returns `INVALID_TIME` (12 < `MIN_VALID_EPOCH`) and does nothing. Keep `setenv("TZ", "UTC", 1); tzset();`. Keep `set_next_phase_start_time(2, 0)`, which now just stores the default `phase_start`.

## Firmware: Cleanup

- [ ] **10. `progress_watering_due()`** (`clock.cpp:106-113`): delete the dead `phase` / `phase_len` locals.
- [ ] **11. `core/include/clock.hpp`:**
  - remove `#include <bits/chrono.h>` (libstdc++ internal header; this is the editor error)
  - remove `DEFAULT_YEAR`, `DEFAULT_MONTH`, `DEFAULT_DAY`
  - remove the commented-out `get_hourmin_*` and phase blocks in `iClock` and `Esp32Clock`
- [ ] **12. `core/include/constants.hpp`:** change `const int MIN_VALID_EPOCH` to `constexpr time_t`, and replace `OFFSET_RANGES[]` with `MIN_UTC_OFFSET_MIN` / `MAX_UTC_OFFSET_MIN`.
- [ ] **13. `core/include/util.hpp:41-42`:** in `put_le64`, `int low/high` should be `uint32_t`.
- [ ] **14. `core/include/head.hpp:83`:** `get_utc_offset_min()` returns `int`; make it `int16_t` so the type matches the wire format.
- [ ] **15. Pass the real write length (optional but worth it).** `ble/gatt_service.cpp:164` knows the real `len`, but `handle_writes` gets the full 20-byte array. Pass `std::span(raw_data.data(), len)` and reject a `WRITE_CONF_TIME` shorter than `BLE::CONF_TIME_PKT_LEN` (queue an `INVALID_TIME` event).
- [ ] **16. `ble_types.hpp:7`:** update or remove the stale `// NOTE: WRITE_CONF_TIME will now handle time and phase with timestamp` comment.

## C++ Tests (`tests/`)

- [ ] **17. `fixtures.hpp`:**
  - line 43: drop the `set_int_phase_of_cycle` mock (method no longer exists)
  - line 50: update the `set_time` mock for the new signature and `ActionStatus` return
  - lines 83-87: rewrite `set_test_time` to take an epoch (+ optional offset); it currently calls `set_time(3, 3)` and stores the result in a `time_t`
  - line 105: make the `change_system_clock` mock actually update `curr_time`
- [ ] **18. `ble_test.cpp`:**
  - line 88: verify the timestamp and offset arguments
  - line 217: remove `set_phase_of_cycle`
  - lines 214-230: mock `get_curr_utc_time` / `get_utc_offset_min` and assert the 13-byte config layout
  - add a short-packet test (if you do step 15)
- [ ] **19. `mocks.hpp:129`:** `WRITE_CONF_TIME` is still `{cmd, 0, 1}`. Build the 11-byte payload.
- [ ] **20. `clock_test.cpp`:** rewrite the hour/minute tests and the phase-progression test. Add cases for:
  - a clock change backwards (next watering is the next upcoming one)
  - resync to exactly the watering minute (next watering is the following phase)
  - a non-zero offset (02:00 local at offset `-420` lands at 09:00 UTC)
  - a DST offset change (same timestamp, `-420` → `-480`, next watering shifts 1 h)
  - an invalid timestamp or offset (returns `INVALID_TIME`, clock untouched)
  - `is_watering_due()` is `false` before the first sync
- [ ] **21. `utils_test.cpp`:** add `le64` round-trip tests, including large values and a negative `int64`, plus a negative `int16` through `get_le16`.

## Python (`drippet_py/`)

- [ ] **22. `ble/protocol.py:51`, `writeConfTime`:** still sends `[cmd, hour, minute]`.
  ```python
  bytes([BleCommand.WRITE_CONF_TIME]) + struct.pack("<qh", ts, offset_min)
  ```
- [ ] **23. `ble/protocol.py:59`, `read_sys_conf`:** still parses `HourMin(data[0], data[1])` + a phase byte.
  ```python
  ts, offset, has_water, hour, minute = struct.unpack_from("<qhBBB", data)
  ```
  Build the `datetime` with `timezone(timedelta(minutes=offset))`.
- [ ] **24. `ble/models.py:75-81`, `SysConfig`:** make `sys_time` a `datetime`, add `utc_offset_min`, and derive the phase from the weekday or drop it. Update `__str__` to show the date.
- [ ] **25. `ble/models.py:112-119`, `COMMAND_NAMES`:** has 6 entries for a 5-value enum ("Write Configuration Phase" is extra), so `WRITE_CONF_TIME_PHASE` and `INIT_PAIRING` print the wrong names. Make it match `BleCommand` exactly.
- [ ] **26. `ble/task.py`:**
  - `handle_write_sys_time(hour, minute)` (line 60) → `(ts, offset_min)`
  - `SYNC_TIME` (line 157) sends `now.hour, now.minute`. Send `datetime.now().astimezone()` as timestamp + offset:
    ```python
    now = datetime.now().astimezone()
    ts, offset_min = int(now.timestamp()), int(now.utcoffset().total_seconds() // 60)
    ```
  - send it automatically right after connecting (the design relies on this; it also keeps DST correct)
  - `SET_TIME` (line 151): remove it, or build today's local `datetime` from the prompted hour/minute and send that
- [ ] **27. `tests/ble_mocks.py` and `tests/ble_test.py`:** update `write_conf_time` (line 19), `read_config` (line 90), `testWriteSetTime` and `testReadConfig` to the new formats.

## Rollout

- [ ] **28. Flash the firmware and update the Python client together.** Payload sizes changed, so old and new versions aren't compatible. Then on hardware:
  - sync, read the config and compare the time with the phone
  - set the watering time two minutes ahead and confirm it fires once
  - power-cycle the board and confirm it doesn't water before the first sync
  - when ready, switch `DEV_PHASE_LEN` → 86400 and re-run the clock tests
