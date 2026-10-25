# MINOR N-060 — save_session has no progress or cancel contract: the og drives 40%→65% per-node progress with per-child IsCancelled teardown

**Severity:** MINOR
**Module:** session (+ orchestration)
**Parameters affected:** F32 (cancellation — save cannot be cancelled), F38 (progress semantics during save), F41 (callback threading during save)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (the save path's per-node progress window: the og's scan progress reserves 40%..65% for the save/tree-serialization phase with per-child IsCancelled checks during teardown — the artifact's flow section); `re-workspace/artifacts/UIUX/04-userflow-state-machine.md` (save-phase progress is user-visible in the scan flow)
**Core references:** `src/core/orchestrator/cr_scan.cpp:1183-1248` (`save_session` — synchronous full-tree write, no progress events, no cancellation token consulted) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's session save reports progress inside the scan's 40%→65% window (per-node) and can
be cancelled mid-save (per-child teardown checks). The core's `save_session` is a silent
synchronous write — no progress, no cancel. On large trees (millions of nodes — the
scalability parameter's large-N case) the UI freezes at the save boundary with no feedback
and no escape.

## What the reference does

Per the rsf-format flow section: the save walks the tree emitting progress within the
reserved window, checking cancellation per child; a cancelled save tears down cleanly.

## What our core does

`cr_scan.cpp:1183-1248`: iterate roots, serialize, write — no event emission, no token
check. (Load has its own budget caps — this finding is the save side.)

## Gap

Falsifiable: cancelling during save in the og aborts the save cleanly with partial progress
reported; the core's save runs to completion regardless, reporting nothing.

## Impact

UX/liveness on large sessions only — no data correctness impact. MINOR (contract gap on
parameters F32/F38, small in practice because saves are fast relative to scans).

## Reproduction / verification

Corpus flow section read first-hand; save path read first-hand (no token/event sites).

## Fix plan

1. Thread the orchestrator's cancellation token into `save_session`; check per root and
   every N nodes; on cancel return ERR_CANCELLED with the partial file removed (atomic
   temp-then-rename, mirroring the writer's discipline).
2. Emit progress events within a documented sub-window (e.g. carve the existing save phase
   out of the scan's 100% tail, or add a dedicated save-progress event — coordinate with
   N-043's heartbeat decision so the two land coherently).

## Tests to add

- `Session.SaveCancelsCleanly`
- `Session.SaveEmitsProgress`

## Related findings

- N-043 (progress liveness watchdog — same progress-contract family; land together)
- N-058 (loader hardening — same module wave)

## Open questions

None.
