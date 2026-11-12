# 03: Watering runs never strand nodes, and a Faulty Node halts cleanly

**What to build:** However a watering run ends, every node is left able to water on the next run:
- finished normally,
- a node reports something unexpected, such as rebooting and reporting INITIALIZING,
- a node goes faulty.

Nodes left in IN_QUEUE or COMMAND_SENT return to READY with their retry counts cleared. Under the agreed halt-all Faulty Node policy, if any node is ERR when a run starts, nothing is queued and the Head Status becomes FAULTY_NODE until the next Pairing. The ERR check runs before any node is queued, so no node is left half-queued.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] After a run ends because the active node reports INITIALIZING, no node is IN_QUEUE or COMMAND_SENT, and the next run queues them again
- [ ] After a run ends with FAULTY_NODE, no node is left IN_QUEUE or COMMAND_SENT
- [ ] With any ERR node present at run start, no node is queued and the Head Status is FAULTY_NODE
- [ ] A normal full run still ends in STANDBY with no active watering index
- [ ] Tests go through the Head's public interface with the mocked clock

Spec: `../spec.md`
