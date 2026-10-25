# MINOR N-017 — Cancel watcher coverage inconsistent: carve has 50 ms watcher; orchestrator parallel candidate scan does not

**Severity:** MINOR
**Module:** orchestrator, carve
**Parameters affected:** X53 (cancellation responsiveness — worst-case latency bounded by one engine poll cadence), S32 (cancellation — per-stage, not per-item), O41 (callback threading — pool dispatch model)
**Corpus references:** `re-workspace/artifacts/DRWUI-exe--thread-architecture.md` (mentions `CStopScanThread` worker but doesn't specify cancel-latency bound per stage)
**Core references:** `src/core/carve/cr_carve.cpp:418-429` (the carve multi-threaded path spawns a watcher thread that polls `cancel->cancelled()` every 50 ms and calls `pool->abort()` to drain queued windows); `src/core/orchestrator/cr_scan.cpp:952-1003` (the parallel candidate scan uses `parallel_for(pool, n, worker_job)` with NO equivalent watcher; workers poll the external `cancel` token inside `scan_candidate → run_engine_dispatch → engine.scan_*` at engine loop heads); `src/core/cr_parallel.cpp:104-150` (the `parallel_for` worker dispatch loop does not poll cancel between item dispatches) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The carve multi-threaded path spawns a 50 ms watcher thread that
actively aborts the pool on cancel. The orchestrator's parallel
candidate scan uses the same `parallel_for` pool but spawns NO watcher —
it relies on each worker's first engine loop head to detect cancel.
A queued candidate (lost partition) that starts AFTER a user cancel
will not bail until its FIRST engine loop head — could be hundreds of
records in (or in pathological cases, an entire candidate scan).

## What the reference does

The corpus does not document the og's per-stage cancel-latency bound.
`DRWUI-exe--thread-architecture.md` mentions a `CStopScanThread`
worker but does not specify whether it actively aborts the worker pool
or just sets a flag. The og's effective behavior (per the corpus's
"scan-abort manual-reset event" description) is probably similar to
the core's carve-watcher pattern; the core's orchestrator path
diverges from the core's own carve-watcher pattern (an internal
inconsistency, not an og-parity divergence).

## What our core does

```cpp
// src/core/carve/cr_carve.cpp:418-429 (audited 5d6da5f) — HAS WATCHER
std::thread watcher([&cancel, &pool]() {
    while (!cancel->cancelled()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (cancel->cancelled()) {
            pool->abort();   // drain queued windows
            return;
        }
    }
});
// ... workers process windows in parallel; watcher aborts pool on cancel ...

// src/core/orchestrator/cr_scan.cpp:952-1003 (audited 5d6da5f) — NO WATCHER
auto worker_job = [&, this](size_t i) {
    if (cancel->checkpoint()) return;       // <-- per-item poll (only at start)
    scan_candidate(candidates[i], cancel);
};
parallel_for(pool, candidates.size(), worker_job);

// src/core/cr_parallel.cpp:104-150 (parallel_for dispatch)
//   The pool's worker dispatch loop does NOT poll cancel between item dispatches.
//   A worker takes the next item from the queue and runs worker_job(item).
//   If cancel arrives mid-item-dispatch, the worker still runs the full item.
```

`cr_scan.cpp:977-980` `worker_job` calls `cancel->checkpoint()` first,
but the pool's `parallel_for` (`cr_parallel.cpp:104-150`) does not poll
cancel between item dispatches. Compare `cr_carve.cpp:419-425` where
the watcher actively aborts the pool on cancel.

## Gap

Carve has a 50 ms watcher; the orchestrator candidate scan does not.
Worst-case cancel latency for the candidate scan is bounded by one
candidate's first engine loop head (could be hundreds of records in,
or in pathological cases an entire candidate scan if the engine's
internal cancel check is per-stage rather than per-record).

## Impact

- **Cancel responsiveness:** the user hits cancel; the orchestrator
  candidate scan continues until the current candidate's first engine
  loop head. For a small candidate (a few hundred records), this is
  ~10-50 ms. For a large candidate (millions of records on a damaged
  volume), this could be seconds.
- **No correctness impact:** the cancel does eventually take effect;
  no work is committed after cancel.
- **UX:** the user perceives a "cancel didn't work" delay. The carve
  path's 50 ms watcher is the standard; the orchestrator path is the
  outlier.
- **Internal inconsistency:** the core's own carve-watcher pattern is
  the right model; the orchestrator candidate scan should match.

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_carve.cpp:418-429` (the
  watcher), `cr_scan.cpp:952-1003` (no watcher), `cr_parallel.cpp:104-150`
  (no per-dispatch cancel check).
- **Executed comparison:** not run — would require a fixture with a
  long-running candidate that ignores cancel for a measurable time.
  The behavior is reproducible from the source.

## Fix plan (for the engineering team)

1. **Either replicate the carve watcher pattern in `run_deep`'s
   parallel candidate fan-out, OR add a per-item `cancel->checkpoint()`
   check inside `parallel_for`'s worker dispatch loop.**

2. **Files to modify (option a — replicate watcher):**
   - `src/core/orchestrator/cr_scan.cpp:952-1003` — spawn a watcher
     thread before `parallel_for` that polls `cancel->cancelled()`
     every 50 ms and calls `pool->abort()` on cancel. Mirror the carve
     pattern.

3. **Files to modify (option b — per-item poll in parallel_for):**
   - `src/core/cr_parallel.cpp:114-135` — inside the worker dispatch
     loop, before taking the next item from the queue, check
     `cancel->cancelled()`. If true, return early (don't dispatch the
     item).
   - Option b is one-line and covers all callers of `parallel_for`,
     not just the orchestrator candidate scan. It's the safer default.

4. **Tests to add:**
   - `Orchestrator.ParallelCandidateScanCancelsWithin50ms` — fixture: a
     candidate that takes 1 second to scan (sleep loop); cancel after
     100 ms; assert the scan aborts within 50 ms (not 900 ms). FAILS
     today.
   - `ParallelFor.WorkerDispatchPollsCancel` — unit test for
     `parallel_for`: queue 100 items; each item sleeps 10 ms; cancel
     after 50 ms; assert no more than 5 items ran (the pool drained
     within ~50 ms of cancel). FAILS today (option b fix needed).
5. **Acceptance criteria:** the two tests above pass.

## Tests to add

- `Orchestrator.ParallelCandidateScanCancelsWithin50ms` (FAILS today)
- `ParallelFor.WorkerDispatchPollsCancel` (FAILS today)

## Related findings

- **N-016** — `emit_boundary_events_` double-call bug. Both are
  orchestrator telemetry/cancel hygiene findings. Both are MINOR and
  trivially fixable.

## Open questions

- **OQ-S2-D-2 (carried from S2-D):** what is the og's cancel-latency
  bound for the parallel candidate scan? Corpus
  `DRWUI-exe--thread-architecture.md` doesn't give a per-stage
  cancel-latency number. The og may use the `CStopScanThread` worker
  with a similar watcher pattern. Need Ghidra-side decompile of
  `CStopScanThread` to compare. Low priority: the practical worst
  case is one candidate's first poll, bounded.

## Resolution

**Status:** FIXED (option a — the carve watcher pattern, factored)
**Fixed in commit:** this session's quick-wins batch (new `cr::CancelPoolWatcher` in cr_parallel.h/.cpp; wired at the orchestrator candidate fan-out; the carve site refactored onto the same class — behavior unchanged)
**Verified by:** `Pool.CancelPoolWatcherDrainsQueueWithinOneTick` — 64×150 ms items over 4 workers, cancel at 50 ms: rc == ERR_CANCELLED, ≤ 6 items ever start (workers + slack), wall time well under the full 2.4 s queue; `Pool.CancelPoolWatcherNullTokenIsInert` (null token: no-op, joins cleanly). The tests cannot compile pre-fix (the class is new) — fail by construction. All existing carve + orchestrator + pause/resume tests stay green (397/397) — the carve refactor is behavior-identical.
**Resolution notes:** Implemented the finding's option (a): the watcher mirrors cr_carve.cpp's pattern at the orchestrator's parallel candidate fan-out. The implementation is factored into `CancelPoolWatcher` so both fan-outs share ONE watcher (the finding's own framing: "the core's own carve-watcher pattern is the right model; the orchestrator candidate scan should match"). Option (b) (per-item poll inside parallel_for) was not taken: parallel_for has no token parameter and changing its signature would touch every caller; the watcher achieves the same drain semantics without an ABI/API change. OQ-S2-D-2 (the og's own cancel-latency bound) remains corpus-undecoded — the watcher matches the core's established carve standard.
