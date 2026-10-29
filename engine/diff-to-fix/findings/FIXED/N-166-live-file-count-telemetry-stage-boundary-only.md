# MINOR N-166 — live file-count telemetry is stage-boundary-only: the og's counters grow live at ~2 s cadence; a 10-minute carve stage streams progress events whose file counters never move — and orchestration.md's claim is inaccurate

**Severity:** MINOR
**Module:** orchestration (telemetry cadence)
**Parameters affected:** progress semantics (38), callback contract truthfulness
**Corpus references:** "Files Found: 1139157 (143.74 GB)" updating during the advanced scan; the Reconstructed branch growing 1→16 live over 6 minutes (`dynamic-ground-truth--scan-phase-captured.md:91-105`); "Quick scan matched=N, total=9423" counter lines at ~2 s cadence (`DRWUI.log:62-112`)
**Core references:** `src/core/orchestrator/cr_scan.cpp:391-411` (`tel_counts_` — runs only at stamp :1135 and stage ends :1293/:2020/:2192), `:417-439` (`emit_progress_event_` re-reads the frozen snapshot); the honest header contract `include/cr/orchestrator/cr_scan.h:116-118` ("STAGE-BOUNDARY samples"); the inaccurate claim `diff-to-fix/modules/orchestration.md:210-213` ("counters refreshing at progress ticks"); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's ScanProgress events carry files_found/carved_found snapshots that update
only at stage boundaries — during a long stage the counters are frozen even though
carved nodes attach to the tree live. The og's counters track the live tree at ~2 s
cadence. The header contract is honest about it; orchestration.md:210-213 is not.

## What the reference does

Live counters (cites above): the UI's "Files Found" tracks the tree continuously;
DRWUI.log shows matched/total counter lines every ~2 s through the scan.

## What our core does

`tel_counts_` at the four boundary sites only; `emit_progress_event_` re-reads the
snapshot. The carve attach path streams nodes (carve_slice) without refreshing counts.

## Gap

Counter refresh granularity: og ~2 s live vs core stage-boundary; plus one doc claim
that says otherwise.

## Impact

Hosts rendering "Files Found" from progress events show a frozen number for a whole
stage (user-observable staleness on long carves). Data is reachable via the tree/query —
a presentation-contract gap, not data loss.

## Reproduction / verification

Corpus-derived (the runtime ground-truth + log cadence cites). Core-derived (the four
call sites + the frozen snapshot read; the header contract's own admission). Doc-truth:
orchestration.md:210-213 read directly.

## Fix plan

1. Refresh the counts on the telemetry tick (the progress emit already runs at a
   cadence): read the live counters in `emit_progress_event_` (atomic loads — the
   counters already are atomics) instead of the boundary snapshot; keep stage-boundary
   stamps as the authoritative close-of-stage sample.
2. Files: `src/core/orchestrator/cr_scan.cpp` (:417-439); `modules/orchestration.md`
   (the :210-213 claim becomes true).
3. Tests: `Telemetry.CountersMoveDuringStage` (a carve fixture with progress observers
   asserts monotonic count growth mid-stage) — fails today.
4. Acceptance: mid-stage events carry non-decreasing, actually-moving counts; the
   header contract line updated to "live-refreshed".

## Tests to add

- `Telemetry.CountersMoveDuringStage`
- `Telemetry.StageBoundaryStampUnchanged` (the close-of-stage sample stays authoritative)

## Related findings

- M-125 / M-126 (the same message family)
- N-167 (the size aggregate the counters render beside)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-166 landing, session-53 minors lane — post-push SHA in the worklog entry)
**Verified by:** `Telemetry.CountersMoveDuringStage` (RED pre-fix proven by the stashed-overlay rebuild — with the live refresh disabled, every mid-carve ScanProgress sample carried carved_found == 0 while three carved JPEGs were already attached; GREEN post-fix: the last pre-park sample carries the attach-time delta) + `Telemetry.LiveAttachCounterTracksTheTree` (the O(1) counter's add/detach/reset lifecycle) + the full Telemetry family green; suite 1382/1382 at the landing.
**Resolution notes:** The live datum is a new O(1) attach counter on ResultTree itself (`live_files()`: an atomic ticked in add_file under the tree's own lock, subtracted by detach_subtree, zeroed by reset — a deserialized tree self-seeds through its own add_file calls; lock-free atomic read at event time). `emit_event_` overlays the counter's delta over the boundary snapshot on ScanProgress samples ONLY, attributing the growth to the ACTIVE stage's counter (carve growth -> carved_found, structured growth -> files_found) so the three counters' semantics are preserved and their sum tracks the og's live-tree headline; the delta clamps at zero across a lost-candidate detach (non-decreasing by construction). The stage-boundary stamps and the FilesFound boundary events stay snapshot-based exactly as before — the finding's "authoritative close-of-stage sample" requirement. The header contract line (cr_scan.h §events) now states the live refresh. The finding's premise correction: the stats counters are NOT atomics (the finding's "the counters already are atomics" was the plan's assumption; the ScanStats fields are plain u64 in the caller's struct — the tree's own atomic is the live source that exists), recorded here rather than silently deviating from the plan.
**Fixed in commit:** (this session's N-166 landing, session-46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Orchestrator.TelemetryCountersMoveDuringCarveStage` (the pin: monotonic counters across the whole stream; at least one mid-carve ScanProgress event carries a non-zero live carved count — RED pre-fix with the refresh stashed, every mid-stage event frozen at the stage-begin snapshot; the final event equals the authoritative stats). Suite 1312/1312 linux-release (1270 pass / 42 env-skips / 0 fail); cr_ui_tests 73/73.
**Resolution notes:** The landing follows the finding's fix plan: the counts refresh at the telemetry tick. ResultTree gained two relaxed atomic attach counters (live_files_/live_raw_ — bumped inside add_file's existing lock; the raw subset keyed on fs_kind==Raw at attach, which carve stamps before the attach), and emit_event_/emit_progress_event_ refresh the snapshot through refresh_live_counts_ (monotonic — never regresses below what a boundary stamp or earlier event reported; the boundary stamps stay the authoritative close-of-stage sample, tel_counts_ re-truths from ScanStats at every boundary). deleted_found deliberately stays boundary-only: the engines assign the deleted-state AFTER the attach (the finding's own 'the counters already are atomics' premise corrected — ScanStats's counters are plain u64s and the deleted-state is post-attach, so the tree-level atomics are the race-free carrier; the header contract re-truthed to the exact split). The progress-event coalescing stays (permille-advance driven) — the og's ~2 s cadence is the same order on real scans and the event cadence was never the complaint; the frozen counters were. modules/orchestration.md's inaccurate claim (the 'counters refreshing at progress ticks' line the finding cited at :210-213 was already restructured by the parallel window — the surviving N-166 reference row now records the landed truth). The same push carries the hot-window red-main repairs this lane executed FIRST (the leave-main-green contract): the N-177 convergence duplicate-TEST redefinition (build-breaking; the S53 lane's byte-equivalent test renamed to SizeExceedsBlocksDamagedDirectoryStaysUnflagged keeping its unique damaged-directory pin; the duplicate PFLAG constant folded into the winner's NODE_FLAG_), the N-171 validated double-count (the 994e0bb0 merge kept both lanes' increments), the 4d450da2 crossed grading assertions, and the user-sign test-hygiene leak (the S-055/S-056 battery's 190-sign default installs never torn down — the og's weak 3-byte 'lha' '-lh' prefix sign leaked into LzhWrongDashByteRejects and carved the wrong-dash form; per-test teardowns now restore the empty registry, bisected first-hand).
