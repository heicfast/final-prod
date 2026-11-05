# INFORMATIONAL I-042 — paused-state observability: the og surfaces pause in the master state enum, nStatus, the UI strip, and the taskbar; the core surfaces it in telemetry only — registered shape difference

**Severity:** INFORMATIONAL
**Module:** orchestration (pause observability)
**Parameters affected:** callback/observability contract (41 adjacent), API surface shape (42)
**Corpus references:** paused states 2/6 (`DRWUI_decompiled.c:208944-208996`), the nStatus==2 branch (:1098851-59), the `paused` widget property + "Scanning Paused." strip (:1098139, :1098406-24), the `sigTaskBarPaused(bool)` wiring (:1323035)
**Core references:** `ScanTelemetrySnapshot.paused` (`include/cr/orchestrator/cr_scan.h:509`; `include/cr/cr_ui.h:658` — paused/running/cancelled); the N-075 query pair is busy/stopping only (`src/ui/cr_ui.cpp` scan_busy_impl/scan_stopping_impl); audited at core `8d0d88b8
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

The og exposes pause across four surfaces (enum, nStatus, strip, taskbar); the core
exposes it via the telemetry snapshot's `paused` flag only — no dedicated
`cr_ui_scan_paused` query, no taskbar concept (a host concern). Hosts CAN observe
pause; a dedicated query would be cosmetic. Registered as a deliberate shape
difference.

## What the reference does

The four surfaces (cites above).

## What our core does`

Telemetry-only (cites above).

## Gap

None (library-appropriate subset).

## Impact

None — hosts render pause from telemetry.

## Reproduction / verification

Both sides read first-hand (lane D).

## Fix plan

1. Register in the orchestration/query module docs (one line). Optionally add
   `cr_ui_scan_paused` if a host requests it — not required for parity.

## Tests to add

- None (telemetry pause already tested via test_pause_resume.cpp).

## Related findings

- N-178 (the pause surface's actionable legs)
- I-041 / I-043 (the same state-model register family)

## Open questions

None.

## Resolution

**Status:** FIXED (ACKNOWLEDGED — deliberate shape difference registered)
**Fixed in commit:** (this session's I-042 landing, S48 INFO-first lane — SHA recorded in the worklog entry)
**Verified by:** corpus re-read first-hand this session: DRWUI:208944-208950 (the paused-state handler with CUIScanProgressInfo RTTI dynamic-cast), :1098851-59 (the nStatus==2 branch region), :1098139 (`QString::operator==(this,"paused")` — the paused widget property), :1098406 (`QVariant(local_158,"paused")` — the strip), :1323035 (the `sigTaskBarPaused(bool)` connect) — all five sites confirmed on-disk. Core side re-read: `ScanTelemetrySnapshot.paused` at include/cr/orchestrator/cr_scan.h:520, the cr_ui.h:689 paused/running/cancelled triple, and the busy/stopping-only query pair at src/ui/cr_ui.cpp:2274/2281 — the finding's core description confirmed current at HEAD. Module doc registered (modules/orchestration.md:10) — `grep -c "I-042 (ACKNOWLEDGED" modules/orchestration.md` = 1 post-landing.
**Resolution notes:** Registered in modules/orchestration.md per the finding's one-line fix plan: the og's four pause surfaces (master enum states 2/6, nStatus==2, the "Scanning Paused." strip + paused widget, sigTaskBarPaused) vs the core's telemetry-only exposure (`ScanTelemetrySnapshot.paused`; no dedicated `cr_ui_scan_paused` query; the taskbar a host concern). A library-appropriate subset — hosts render pause from telemetry; the optional dedicated query stays declined unless a host requests it (the finding's own option). No code change; the telemetry pause path is already tested via test_pause_resume.cpp. The related actionable legs live on N-178 (open); I-041 remains open on its own file; I-043 landed as PARITY-DELTAS D-122.
