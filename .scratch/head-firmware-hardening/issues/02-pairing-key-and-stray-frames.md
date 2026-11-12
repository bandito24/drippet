# 02: Pairing sends its key, and stray frames can't crash the head

**What to build:** During Pairing, the head's DISCOVERY frame carries its key, as the Python emulator already does, so nodes adopt a real key instead of 0. A frame from a Node Address that has no node can no longer crash the head:
- ACK and STATUS from an unknown address are ignored and logged.
- An ADDRESSING frame for an empty slot counts as an invalid index.
- When the table is full, an unconfirmed ADDRESSING frame gets no reply. It never gets an ACK.

Incoming STATUS values the head acts on are READY, WATERING, ERR and INITIALIZING (a node that rebooted). Any other value leaves the node unchanged. Also remove the stray function declaration inside the helper that ends Pairing.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] The Pairing DISCOVERY frame contains the 2-word key with data length 2 (tested through the Head's pairing step)
- [ ] ACK and STATUS from a Node Address with no node return nothing and change no state (no assert, no null dereference)
- [ ] ADDRESSING for an empty slot returns nothing harmful and doesn't assert
- [ ] With a full table, a failed ADDRESSING confirmation returns no ACK
- [ ] A STATUS of INITIALIZING marks the node INITIALIZING; an out-of-range STATUS value changes nothing
- [ ] Host suite passes

Spec: `../spec.md`
