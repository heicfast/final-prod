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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-060 commit — DFX18 session 18)
**Verified by:** `Session.SaveEmitsProgress` + `Session.SaveCancelsCleanly` (core level: per-node (done,total) ticks in NODES, constant total == the serialized node count, monotonic, terminal (total,total); false return → ERR_CANCELLED at the entry tick and the first per-node tick, zero output) + `CrUi.SaveProgressEventsAndAtomicCancel` (shim level: coalesced CR_EVT_SAVE_PROGRESS events with the save's own 0..1000 stage_permille and the STANDING overall 1000; cr_ui_scan_save_cancel from the first save-progress event → CR_UI_ERR_CANCELLED with the final path byte-identical to the previous good session and no staging .tmp; the scan token untouched — telemetry cancelled==0). Fail-before by construction (serialize_tree had no progress hook; cr_ui_scan_save_cancel did not exist — the tests do not compile on the pre-fix tree). Suite 716/716 linux-release at the landing tree.
**Resolution notes:** Corpus re-verified first-hand (rsf-format--scan-session.md flow section): the og's save progress is `40 + 50·processed/(2·remaining)` reported through a progress callback with `IUIProgressInfo::IsCancelled` checked AFTER EVERY CHILD (writer @186644-186674) — and the cancel object is the SAVE PROGRESS OBJECT's own state, NOT the scan's context flag (+200 is the watchdog's loop flag, a different surface): this is why the og's save-on-scan-cancel works, and the initial design (gating the save on the scan's PausableToken) was CORRECTED mid-fix — it would have broken the repo's save-on-cancel flow (the `CancelSaveAndResume` contract). Implemented: `session::SerializeProgress` hook through `serialize_tree` (per-node ticks, total known from the pre-order walk, ERR_CANCELLED on false, terminal tick) + `save_session` pass-through param (NULL = the silent pre-N-060 save) + the shim's coalescing adapter (CR_EVT_SAVE_PROGRESS 9u, appended type — first tick, permille steps, terminal; overall stays the standing 1000) + the save-scoped `cr_ui_scan_save_cancel` export (appended export — no version bump per §9) + ATOMIC temp-then-rename saves in both the shim and the CLI's save_session_to (the N-014 checkpoint discipline: a torn save never destroys the previous good session file; the staging .tmp removed on cancel/failure). The og's 40%→65% window is NOT adopted: our scan completes at 100% before save is callable — mapping save progress into the scan's window would REGRESS the monotonic overall permille; the save's own stage_permille (0..1000) is the honest equivalent, registered in ABI-CONTRACT §5/§6.
