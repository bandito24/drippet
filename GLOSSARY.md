# Drippet

A drip irrigation system: a head board schedules watering for a set of node boards, each driving one hose, and a client (app or CLI) configures the head over BLE.

## Scheduling

**Watering Start Time**:
The local time of day (hour:minute) at which the head begins a watering run.
_Avoid_: phase start time, phase time, next phase

**Watering Interval**:
The length of time between watering runs; one day in production.
_Avoid_: phase length, phase

**Cycle**:
The set of weekdays on which a given node waters.
_Avoid_: schedule, watering days

**Cycle Day**:
The local weekday, Sunday first, that a node's Cycle is checked against when a watering run starts.
_Avoid_: phase, phase of cycle, cycle phase

## Nodes

**Node**:
A board on the chain that drives one hose; the head board is also a node, its own hose.
_Avoid_: hose (for the board), slave, device

**Node Address**:
A node's position on the chain, counting from 0 for the head's own hose.
_Avoid_: row, row index, node index

**Pairing**:
Re-addressing the whole chain: the head cuts power to the chain, then powers and addresses one node at a time in chain order.
_Avoid_: discovery (for the whole process), syncing

**Head Status**:
What the head is doing overall: pairing, standing by, sending watering commands, or halted because a node is faulty.
_Avoid_: head state, mode

**Faulty Node**:
A node that stopped answering or reported a hardware error; while one exists the head waters no node until the next Pairing.
_Avoid_: dead node, broken node

## Time

**Clock Sync**:
The client telling the head the current UTC time together with the client's UTC offset; the head does not water until it has received one.
_Avoid_: set time, time write

**UTC Offset**:
The minutes added to UTC to get the user's local time, as reported by the client at the last Clock Sync.
_Avoid_: timezone
