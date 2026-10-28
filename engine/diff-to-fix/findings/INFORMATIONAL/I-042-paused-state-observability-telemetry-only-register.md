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
