# 08: Watering Start Time is validated and survives reboots

**What to build:** A gardener's Watering Start Time survives a power cut, and an invalid one is refused.
- Setting the Watering Start Time rejects hour > 23 or minute > 59 with `INVALID_TIME` and changes nothing. Today it silently wraps the value.
- Each successful set is saved to storage.
- At boot, storage initialises first. The clock then uses the saved start time, or 02:00 on a fresh install.
- Clock Sync is still required before watering and is not persisted.

The BLE docs note that the start time persists across reboots.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] An out-of-range hour or minute returns `INVALID_TIME` and leaves the stored start time and the next watering point unchanged (Esp32Clock tests)
- [ ] A `WRITE_CONF_TIME_PHASE` event reports the real result, not always OK
- [ ] A successful set is saved to storage; a rejected one is not (Head tests with the mocked storage)
- [ ] Boot loads the saved start time, falling back to 02:00 (checked on hardware by power-cycling)
- [ ] Docs updated
- [ ] Host suite passes

Spec: `../spec.md`
