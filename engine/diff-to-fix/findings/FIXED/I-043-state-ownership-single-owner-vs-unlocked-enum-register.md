# INFORMATIONAL I-043 — state ownership: the og's master scan-state enum sits UNLOCKED on the service singleton (cross-thread reads); the core is single-owner with mutexed telemetry — a deliberate thread-safety improvement, registered

**Severity:** INFORMATIONAL
**Module:** orchestration (state ownership/threading)
**Parameters affected:** threading model (40), callback threading (41)
**Corpus references:** the master 14-value state enum at singleton +0x78 (`DRWUI_decompiled.c:208243-296` — telemetry names notstarted…repairvideo_pause) written by the scan flow; the parallel locked carrier `CUIScanProgressInfo` nStatus +0x44 with mutex +0x90 — i.e. the og itself carries a locked carrier BESIDE the unlocked master
**Core references:** the controller-owned state + mutexed telemetry snapshots (`src/core/orchestrator/cr_scan.cpp` telemetry publish paths, `:319-376`); audited at core `8d0d88b8
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

The og's master state variable is read/written across threads without its own lock (the
mutexed nStatus carrier exists beside it — the og's own mitigation). The core keeps the
authoritative state single-owner (the controller) and publishes through mutexed
telemetry snapshots. Registered as a deliberate improvement (data-race-free by
construction).

## What the reference does

The unlocked master + the parallel locked carrier (cites above).

## What our core does`

Single-owner + snapshot publish.

## Gap

None (improvement).

## Impact

None — recorded so the threading-parameter audit row closes as REGISTERED, not open.

## Reproduction / verification

Both sides read first-hand (lane D; the enum region re-read by the lead auditor).

## Fix plan

1. Register in PARITY-DELTAS (threading family, beside D-18-class rows) — doc lane.

## Tests to add

- None (the telemetry mutex is already covered by the parallel-scan tests).

## Related findings

- I-041 / I-042 (the state-model register family)
- D-18 lineage (the driver-side cancel threading register)

## Open questions

None.

## Resolution

**Status:** FIXED (ACKNOWLEDGED — deliberate improvement registered)
**Fixed in commit:** (this session's I-043 landing, S48 INFO-first lane — SHA recorded in the worklog entry)
**Verified by:** corpus re-read first-hand this session: the 14-value state enum confirmed at DRWUI_decompiled.c:208243-296 (notstarted/quickscanning/quickscanpaused/quickscanstopped/quickscanfinish/advancedscanning/advancedscanpaused/advancedscanstopped/…/repairvideo_pause — the telemetry-name table re-read on-disk); core side re-read at src/core/orchestrator/cr_scan.cpp:319-376 (the mutexed telemetry publish paths). PARITY-DELTAS now carries D-122 (the threading family, beside D-18/D-121) — `grep -c "D-122" docs/PARITY-DELTAS.md` ≥ 1 post-landing.
**Resolution notes:** Registered as D-122 in PARITY-DELTAS per the finding's one-line fix plan: the og's unlocked master scan-state enum (+0x78 on the service singleton, cross-thread reads) vs the core's single-owner controller + mutexed telemetry snapshots — a deliberate thread-safety improvement, data-race-free by construction, in the D-18 cancellation/cancellation-threading family. No code change (the finding's own verdict: the telemetry mutex is already covered by the parallel-scan tests + TSan CI). The I-041/I-042 state-model register family remains open on their own files.
