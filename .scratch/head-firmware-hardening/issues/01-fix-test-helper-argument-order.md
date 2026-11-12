# 01: Fix swapped test helper arguments

**What to build:** The host test helpers that set a node's duration and Cycle pass their arguments in the wrong order: the value goes where the Node Address should be, and the Node Address where the value should be. Three host test cases fail because of it. Fix the helpers so they set the value on the intended Node Address, giving a fully passing suite to build the rest of the spec on. No firmware behaviour changes.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The duration and Cycle helpers pass (Node Address, value) in the order the Head's external-request methods expect
- [ ] The full host suite passes (the previously failing head, time-pool and mocks cases go green)
- [ ] No firmware source is changed

Spec: `../spec.md`
