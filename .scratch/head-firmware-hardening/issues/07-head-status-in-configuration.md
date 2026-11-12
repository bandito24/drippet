# 07: Head Status in the Configuration read

**What to build:** The app can see what the head is doing overall, so it can show "pairing…", "watering", "standing by" or "faulty node", and knows when Pairing has finished. The Configuration read grows from 13 to 14 bytes, and byte 13 is the Head Status: `PAIRING=0, STANDBY=1, FAULTY_NODE=2, WATERING_CMDS=3`. Update in step:
- the Python client's Configuration decoding, its model and its tests,
- any app parser that already decodes Configuration,
- the Configuration section of the BLE read docs.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] READ_CONFIGURATION is 14 bytes, and byte 13 matches the Head Status in each of the four states
- [ ] Bytes 0–12 are unchanged
- [ ] The Python client decodes and shows the Head Status; pytest passes
- [ ] Any existing app Configuration parser accepts 14 bytes
- [ ] The docs show the 14-byte layout with the Head Status values
- [ ] Host suite passes

Spec: `../spec.md`
