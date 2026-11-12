# 04: An over-budget duration is rejected with no side effects

**What to build:** When the app writes a duration that would push the total of all node durations past the Watering Interval, the head rejects it with `INVALID_TIME` in the Events result. Unlike today, the node keeps its previous duration and status, nothing is written to storage, and the node keeps watering as before. Durations are seconds.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] An over-budget write emits an event `[WRITE_NODE_DURATION, INVALID_TIME, node address]`
- [ ] The node's duration and status are unchanged afterwards
- [ ] Storage is not asked to save for a rejected write
- [ ] A later valid write for that node succeeds, and the node still takes part in watering
- [ ] Host suite passes

Spec: `../spec.md`
