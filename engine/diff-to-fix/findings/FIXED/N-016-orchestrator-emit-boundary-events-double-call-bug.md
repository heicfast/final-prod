# MINOR N-016 — orchestrator: emit_boundary_events_ double-call bug (telemetry inflation on stage exit)

**Severity:** MINOR
**Module:** orchestration
**Parameters affected:** O38 (progress semantics — event count doubled on stage exit), GAP-S6-10 (typed event stream — the feature's surface)
**Corpus references:** no corpus parallel — the typed event stream is a core-specific surface (GAP-S6-10). The bug is a merge artifact, not an og-parity divergence.
**Core references:** `src/core/orchestrator/cr_scan.cpp:577-578` (the same call appears twice, once with 4-space indent and once with 8-space indent — clear merge artifact); `:308-343` (`emit_boundary_events_` emits `FilesFound` + `ErrorRange` events); the other 3 call sites at `:652`, `:1016`, `:1070` each call it once — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`emit_boundary_events_` is called twice in a row inside the `stamp`
lambda (the exit-path of `run()`), once with 4-space indent and once
with 8-space indent — a clear merge artifact. The function emits
`FilesFound` + `ErrorRange` events; double-calling double-emits these
on every stage exit. The other 3 call sites (`:652`, `:1016`, `:1070`)
each call it once. Only the `stamp` lambda has the duplication.

## What the reference does

No corpus parallel — the typed event stream is a core-specific feature
(GAP-S6-10). The og has its own progress UI (DRWUI status strings +
"Remaining time" + Reading sector X/Y display per `DRWUI-exe--main-ui-architecture.md
§9.2`) but no typed event stream.

## What our core does

```cpp
// src/core/orchestrator/cr_scan.cpp:577-578 (audited 5d6da5f)
emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
    emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
// ^^ TWO CALLS, different indentation — clear merge artifact.

// Compare the other 3 call sites (each calls once):
// src/core/orchestrator/cr_scan.cpp:652
emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed));
// src/core/orchestrator/cr_scan.cpp:1016
emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed));
// src/core/orchestrator/cr_scan.cpp:1070
emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed));
```

`emit_boundary_events_` (`:308-343`) emits `FilesFound` + `ErrorRange`
typed events into the telemetry stream. Double-calling double-emits
these on every stage exit (OK, cancel, error paths via the `stamp`
lambda).

## Gap

One call site has a duplicate call; the other three don't. The
`stamp` lambda fires on every `run()` exit path (the most common path
in production), so the inflation is consistent and observable.

## Impact

- **Telemetry inflation:** UI consumers counting events for progress
  display see double counts on the final boundary tick. A UI showing
  "Found N files" would show "Found 2N files" at the stage-exit tick
  (the doubled events both fire with the same data, so the COUNT is
  doubled but the DATA is the same — the second event overwrites the
  first if the UI dedupes by event type).
- **Network/UI overhead:** if the event stream is consumed by a remote
  UI (or a debug logger), double-emitting doubles the network traffic
  / log volume for boundary events.
- **No correctness impact:** the events carry the same data; the
  second event is redundant, not wrong. A UI that processes each event
  independently would see a momentary double-count, but a UI that
  dedupes by event type would see the correct final state.
- **No data loss:** no events are lost; they're just emitted twice.

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_scan.cpp:577-578` (the
  duplicate call), `:652`, `:1016`, `:1070` (the three single-call
  sites), `:308-343` (the function body). The duplicate is unambiguous
  in the source.
- **Executed comparison:** not run — the duplicate is reproducible
  from the source; running it would just confirm the double-emit
  behavior.

## Fix plan (for the engineering team)

1. **Architectural change:** none — the fix is removing one line.
2. **Files to modify:** `src/core/orchestrator/cr_scan.cpp:577-578` —
   remove the second call (the one with 8-space indent).
3. **Algorithm/constant:** none.
4. **Tests to add:**
   - `Orchestrator.BoundaryEventsEmittedOnceOnStageExit` — fixture: a
     scan that completes one stage; assert the event stream contains
     exactly one `FilesFound` event per stage exit, not two. FAILS
     today.
   - `Orchestrator.BoundaryEventsEmittedOnceOnCancel` — same shape on
     cancel path.
5. **Acceptance criteria:** the two tests above pass.

## Tests to add

- `Orchestrator.BoundaryEventsEmittedOnceOnStageExit` (FAILS today)
- `Orchestrator.BoundaryEventsEmittedOnceOnCancel` (FAILS today)

## Related findings

- **S-009 / GAP-S6-10** (the typed event stream feature) — N-016 is a
  bug in the implementation of S-009's feature surface. The fix is
  trivial; the finding is filed for completeness.

## Open questions

- **OQ-S2C-5:** is the double-call a pure cosmetic merge artifact, or
  did one of the calls intend to be a different event type? Both
  calls pass the same `tel_stage_` argument. Looks like a pure
  duplicate, but worth confirming with `git blame cr_scan.cpp:577-578`
  to see if one of the lines was added by a different commit (and
  whether the commit message suggests a different intent).

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's quick-wins batch (one line removed from cr_scan.cpp's stamp lambda)
**Verified by:** `ScanEvents.SingleBoundaryAtTeardown` — the quick+deep fixture produced 4 FilesFound events pre-fix (the teardown double-fire) and exactly 3 post-fix; fail-before/pass-after proven on the pre-batch source (worktree at `6b58b6f`). Suite 397/397.
**Resolution notes:** Pure merge-artifact removal as the finding planned (git blame confirmed both lines came from 56cce87; the 8-space-indented duplicate removed). The adjacent-boundary shape is preserved — stage-end emits and the stamp remain distinct legitimate boundaries (the test's count pin documents the exact contract). OQ-S2C-5 resolved by the same evidence: both calls passed identical `tel_stage_` payloads — a pure duplicate, no distinct intent.
