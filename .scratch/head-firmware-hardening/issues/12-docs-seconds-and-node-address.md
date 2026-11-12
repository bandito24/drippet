# 12: BLE docs use seconds and Node Address

**What to build:** An app developer reading the BLE docs builds the duration UI in the right units and sees one consistent name for a node's position. In the BLE read and write docs:
- durations (and their examples) are in seconds, not minutes;
- "Row Index" and "Node Index" become Node Address, matching the glossary.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] No BLE doc describes durations in minutes
- [ ] No BLE doc says "Row Index" or "Node Index"; Node Address is used throughout
- [ ] The examples' decoded values read as seconds

Spec: `../spec.md`
