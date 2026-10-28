# MINOR N-125 — the drive-disconnect mid-scan states are undispositioned: the og detects target removal ("Drive Disconnected — Unable to continue scanning. Reconnect your drive and try again." Retry/Cancel) — core surfaces it as ERR_READ_FAILED with no retry/reattach model and no registration

**Severity:** MINOR
**Module / Surface:** orchestrator / storage (modules/orchestration.md, modules/storage.md)
**Parameters affected:** A14-adjacent (error classification), 35-adjacent (error recovery — the retry model), 32-adjacent (cancel semantics of the retry gate)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:39-47` (the mid-scan target-removal dialog: the exact copy, the Retry/Cancel triad, the same triad for NAS/Linux/Cloud offline); the R-16 row (docs/REFERENCE-API-GAP.md:205: "GAP. No disconnect detection/retry surface; no reattach-and-continue model").
**Core references:** audited commit `ff879f9` — The engine surfaces device loss as ERR_READ_FAILED storms (the read-truth gate); no distinct classification, no retry window, no documented reattach-continue flow — core's session resume covers reattach-continue only at the whole-session granularity. Grep the trail: R-16 has no finding, no queue row, no PARITY-DELTAS entry (verified this session).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** R-16 sat in the gap table unformalized through every session; the formalization sweep (this session) reached it.

## Summary

("When the target drive is yanked mid-scan the og detects it, tells the user precisely what happened, and offers a reconnect-then-continue path; core returns a read-failure ladder whose storm is indistinguishable from a dying disk. The retry/reattach model is the capability gap; the missing classification is the contract gap; the missing registration is the trail gap. The evidence is UI-level (the corpus shows the dialog behavior, not an engine error code) — so the honest fix is classification + a documented reattach flow, with the og's engine-side mechanism recorded as a corpus question.",)

## What the reference does

As cited — the disconnect dialog with Retry/Cancel and the offline triad for the remote modes.

## What our core does

As cited — ERR_READ_FAILED, no classification, resume-only continuation.

## Gap

Falsifiable: remove the target device mid-scan — the og's UI enters the reconnect flow with the scan resumable after reattach; core ends the run (or storms read failures) with the only continuation being a session resume.

## Impact

User experience on removable media (the top consumer scenario class: USB drives); engine-side it is a classification+flow gap, not a recovery-correctness one.

## Reproduction / verification

Corpus documentation evidence (UIUX/08:39-47 — the runtime-verified dialog decode); core state verified this session.

## Fix plan (for the engineering team)

1. Classification: a removable-media loss heuristic in the transport layer (the device-gone/IO-error distinction the win layer can make: ERROR_DEV_NOT_EXIST-family vs media errors) → a distinct error code (the 0x5005-family append-only allocation) so hosts can branch. 2. Flow: document the reattach-continue contract — a host detects the code, waits for the reattach (its own loop), then resumes from the session checkpoint (the machinery exists; document it as THE flow). 3. Register the deliberately-lighter engine posture in PARITY-DELTAS if no in-engine retry window is built. 4. CLI: the scan verb's exit codes gain the disconnect case.

## Tests to add

- `Scan.DisconnectClassifiedDistinctly` — a device-gone injection returns the new code, not ERR_READ_FAILED (transport-level fixture).
- `Scan.ResumeAfterReattach` — the documented flow round-trips (resume from the checkpoint on the reattached device).
- `Scan.MediaErrorsStillReadFailed` — dying-disk storms keep the old semantics (classification precision).

## Related findings

M-073 (the scan-config surface family); R-16 (the gap row); N-120 (the board-drift sweep that surfaced it).

## Open questions

Whether the og ENGINE carries a distinct disconnect error code (vs a UI-side heuristic over the read-failure storm) is undecoded — the corpus shows only the dialog; a DRWUI decompile probe of the dialog's trigger condition would settle it before any deeper engine work.
