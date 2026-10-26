# MAJOR M-031 — ReFS progress: one terminal tick vs og per-file progress callback

**Severity:** MAJOR
**Module:** refs, orchestration
**Parameters affected:** O38 (progress semantics — weighting/monotonicity/events), O39 (ETA inputs), Q41 (callback events)
**Corpus references:** ReFSSearchDLL_decompiled.c :617-619 — quick-searcher registers a per-file progress binder `bool(unsigned __int64, refs::RefsFileProperties*&)` (RTTI string, binary-verified): the engine reports progress per file with the file in hand.
**Core references:** `src/core/refs/cr_refs_engine.cpp:922-923` — single terminal `progress(1, 1)`. Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-012` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's ReFS engine emits per-file progress (counter + the file itself, enabling incremental UI population during the scan). Our engine emits exactly one progress tick at the end — the UI shows 0% for the whole ReFS stage then jumps to 100%, and no incremental file events exist.

## What the reference does

The binder signature (u64 counter + RefsFileProperties&) is RTTI-verified in the binary and the registration site is quoted (:617-619); the per-file callback fires as the walk emits each file.

## What our core does

One `progress(1,1)` at :922-923; no per-file or per-table hooks (grep: the engine's progress callback is called exactly once).

## Gap

Progress granularity + incremental-file events. Falsifiable: a UI test observing intermediate permille during a ReFS scan sees none today.

## Impact

User-observable stage progress (0→100 jump); ETA for the ReFS stage is undefined until completion; no incremental results during long ReFS scans.

## Reproduction / verification

Corpus-derived (RTTI + registration quote). Core: code read (single call site).

## Fix plan

1. Thread the existing progress callback through `walk_table`/`emit_file`: report `(files_done, files_estimate)` or a monotonic permille derived from table LCN progress.
2. If the orchestrator contract supports it, add the per-file event (the og's binder is the model) — otherwise document the per-stage granularity as the contract.
3. Keep monotonicity guarantees (the orchestrator's weighted model requires it).

## Tests to add

- `Refs.ProgressMonotonicIntermediateTicks` — capture progress callbacks during a multi-file fixture scan; assert > 2 distinct values before completion (fails today: 1 tick).
- `Refs.ProgressCancellationMidWalk` — cancel between ticks; assert partial results survive.

## Related findings

M-022 (parallel leg) (same walk), N-016 (orchestrator emit-boundary — adjacent event discipline).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-031 commit — see `git log` for "fix: M-031")
**Verified by:** `Refs.ProgressMonotonicIntermediateTicks` and
`Refs.ProgressCancellationMidWalk` (both fail-before — proven this session by
temporarily reverting to the single-terminal-tick model and rebuilding: 2/2 FAILED;
fix restored → green). Full suite 575/575 Linux Release.
**Resolution notes:** the Walker now carries the scan's progress callback; `emit_file`
reports intermediate progress every 16th emitted file with a growing-estimate total
`(done, done + 16)` — the FAT engine's dentry pattern (the og's binder is per-file
with the file in hand, `bool(u64, RefsFileProperties*&)` at ReFSSearchDLL :617-619;
our progress ABI is (done, total), so the counter-with-estimate is the faithful
mapping; the orchestrator's `tel_publish_` derives permille and clamps monotonic).
The terminal tick reports `(emitted, emitted)` — 1000 permille — replacing the old
`progress(1, 1)`. Monotonicity verified by test across all ticks. The per-file EVENT
surface (the og's binder carrying the file itself) is not in the core's engine
progress ABI; the typed event stream (GAP-S6-10) is the registered carrier for
per-file events — the finding's fix-plan step 2 alternative ("document the per-stage
granularity as the contract") is superseded by the real intermediate ticks + the
existing event stream. Cancellation: the walk callbacks already poll
`eng.cancelled()` per row, so a mid-walk cancel preserves partial results — pinned
by `Refs.ProgressCancellationMidWalk`.
