# 05: FIFO requests and events, delivered promptly

**What to build:** The app can send many writes in quick succession and have every one applied. Today a second write of the same command type overwrites the first.
- Requests and events become bounded first-in, first-out queues, 16 deep.
- A full request queue refuses the new entry, and the caller can see that it was refused.
- The queue manager guards itself with an internal lock, because the BLE host task and the head task both use it.
- Each Events notification is exactly 3 bytes, `[command, status, node address]`, with the address byte 0 for commands that apply to the whole controller.
- The status task keeps notifying while events are pending, so the app isn't waiting one head loop per event. It stops if a notify fails.

Update the Python client's Events decoding and its tests, and the Events section of the BLE read docs.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] Two WRITE_NODE_DURATION writes for different Node Addresses before one head step are both applied, and both events pop in the order sent
- [ ] The 17th queued request is refused, and the refusal is visible to the caller
- [ ] READ_EVENTS returns exactly 3 bytes
- [ ] The status task drains all pending events per wake-up and stops on a failed notify (checked on hardware)
- [ ] The Python client decodes 3-byte Events; pytest passes
- [ ] The docs describe the FIFO order and the queue limit
- [ ] Host suite passes

Spec: `../spec.md`
