# 10: The advertisement finds the service, and subscriptions reset

**What to build:** The native app can scan for the Drippet service UUID instead of matching on the device name, and every phone's BLE stack can discover the head. The advertisement carries:
- the Flags (general discoverable, BR/EDR not supported),
- the 128-bit Drippet service UUID in the service-UUID list,
- the complete name "Drippet".

That fits in 31 bytes. Appearance, LE role and the advertising interval move to the scan response.

Subscriptions follow what the client actually asked for:
- A subscribe event sets the notify or indicate state from its current value, instead of OR-ing it into the old one.
- A disconnect clears both flags and the connection handle, so a reconnect starts clean and the head never notifies a dead connection.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] A phone scanning filtered by the service UUID finds the head (checked on hardware)
- [ ] Setting the advertising fields succeeds, with no size error in the log
- [ ] Unsubscribing from Events stops notifications; unsubscribing from Node States stops indications
- [ ] After disconnecting and reconnecting, nothing is sent until the client subscribes again
- [ ] The firmware builds with `idf.py build`

Spec: `../spec.md`
