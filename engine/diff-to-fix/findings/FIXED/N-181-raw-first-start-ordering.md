# MINOR N-181 — the og starts the RAW carve controllers before the FS deep engines; the core runs carve strictly last

**Severity:** MINOR
**Module:** orchestration / carve (scan-mode-semantics focus battery)
**Parameters affected:** #38 Progress semantics; #48 Latency (time-to-first-carved-result); #31 Scan lifecycle stage order
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:916484-916509` (`FUN_140580720`: loop 1 RTTI-casts each controller to `CRAWSearchController` and starts those first; loop 2 starts the rest) — tree-verified this session; `re-workspace/artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md:39,56` (carved files visible while the advanced phase is still running)
**Core references:** `src/core/orchestrator/cr_scan.cpp:1170-1275` (the `run()` order: quick → deep → carve — carve strictly after `run_deep` completes); `include/cr/orchestrator/cr_scan.h:519-521` (`STAGE_QUICK/DEEP/CARVE` masks); the registered O37 stage-model delta at `diff-to-fix/modules/orchestration.md:16-18` — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Inside the og's advanced phase, the RAW (carve) controllers are started **before** the filesystem deep controllers — carved files can appear while the FS deep pass is still sweeping (the live capture shows the Reconstructed branch growing during the advanced scan). The core's stage model runs the carve stage strictly after the deep stage completes. The result sets are identical; the ordering — and therefore the time-to-first-carved-result and the interleaving of the two result feeds — diverges.

## What the reference does

`FUN_140580720` (`:916484-916509`, verified first-hand): the first loop walks the controller list, `__RTDynamicCast`s each entry to `CRAWSearchController`, and calls `vtable+0x80()` (start) on every cast that succeeds; only afterwards does the second loop start the remaining (FS) controllers, all behind the `settings+0x6c == 0` deep gate at `:916511`. The sector-sweep then hands every non-skipped chunk to **every** controller (`vtable[0x98]`, `:916880-916885`) — the engines run concurrently over the same chunk stream, the carve side simply started earlier.

## What our core does

`cr_scan.cpp:1170-1275`: `run_quick` → checkpoint → `run_deep` → checkpoint → `run_carve` → checkpoint. The carve stage only begins when `run_deep` returns (`:1189-1196`); a long remnant sweep delays every carved result behind it. The registered O37 delta ("carve is a separate stage in core") covers the stage-model difference but not the ordering detail — this finding adds it.

## Gap

Stated falsifiably: in a scan with a 10-minute deep pass and carveable files in the first GiB, the og's carved results appear minutes before its deep pass finishes; the core's appear only after the deep pass completes.

## Impact

Time-to-first-result for the carve population; progress-feed interleaving a host UI would observe. No recovery-result difference. MINOR (the O37 superset registration covers the architecture; this is the ordering refinement).

## Reproduction / verification

Decompiled evidence first-hand (`:916484-916509` + the per-chunk fan-out at `:916880-885`); core evidence first-hand (`cr_scan.cpp:1189-1196` order); the live-capture corroboration in the scan-phase ground truth. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-scanmode-state-machine.md` §1.

## Fix plan (for the engineering team)

1. Either start the carve pass concurrently with the deep pass on the scan's existing ThreadPool (the two passes read disjoint metadata vs content regions and already share the occupancy/free-map gates), or register the strict-last ordering as a deliberate delta with the reasoning (deterministic stage masks for resume). The concurrent option matches the og.
2. If concurrent: the stage masks must stay truthful for resume (carve completion tracked independently — `STAGE_CARVE` already is).
3. Acceptance: a bench fixture where carve finds files in the first GiB and deep takes longer shows first carved-file event before the deep stage's completion event.

## Tests to add

- `Scan.CarveStartsBeforeDeepCompletes` (event-order assertion on a synthetic slow-deep + early-carve fixture)

## Related findings

I-044 (the scan-mode decode batch this rides), M-039/M-005 (the occupancy/free-map gates the concurrent passes share).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's push (the S80/S81 MINOR-first fix lane's N-181 landing)
**Verified by:** `ScanEvents.CarveStageBeginsBeforeDeepStage` (the ScanStage begin sequence quick(0) → carve(2) → deep(1) in the standard configuration — the finding's own observable: the carved-result class precedes the deep stage's completion) + `ScanEvents.DeepFirstWhenQuickDisabledForTheCarveDedup` (the guarded configuration keeps deep-first — the engine pass rides inside run_deep and must precede carve for the F9 extent dedup) + the re-pinned `CrUi.TypedEventStreamFullLifecycle` (quick < carve < deep, the M-140 re-pin precedent for a corpus-verified contract change); full suites green (cr_tests 1564 ran / 0 FAILED; cr_ui_tests 84/84).
**Resolution notes:** The RAW-first order landed: the carve stage now runs BEFORE the deep extras whenever the engine pass has already emitted its nodes (`quick_done_` — the standard quick+deep+carve configuration and every resume leg with quick done), matching the og's controller-start order (FUN_140580720's first loop starts the CRAWSearchController casts first, DRWUI:916484-916509; the live capture's Reconstructed branch growing mid-scan). The carved results then surface before the deep stage even begins — closing the time-to-first-carved-result gap the finding filed. The finding's fix-plan option 1 (full concurrency) was deliberately narrowed to the reorder: the og's engines interleave over a shared chunk stream (a different architecture — the O37-registered stage-model delta), while the core's sequential stages achieve the same first-result ordering without the checkpoint/progress/tree races the concurrent variant would need TSan-proofed; the residual (interleaved feeds vs sequential stages) is covered by the existing O37 registration. The quick-disabled configuration keeps the original deep-before-carve order BY DESIGN: with quick off, the engine pass runs inside run_deep and must precede carve (the F9 carve-vs-structured extent dedup needs the engine nodes in the tree — a reorder there would emit duplicate carved nodes for files the engine pass later finds structured). Stage masks stay truthful (STAGE_CARVE tracks its own completion; resume with quick done takes the new order, quick-disabled legs the old); the telemetry's weighted aggregation is stage-indexed, not order-dependent. One prior test re-pinned (the M-140 precedent).
