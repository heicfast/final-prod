# Session prompt — Small-Bug Hunting & Stability Polish Agent (saved at session start, token redacted)

> Token: `[REDACTED — stored in env/git config only, never committed]`
> Saved by: T1, 2026-11-26. Read twice before starting.

```
SESSION PROMPT — Small-Bug Hunting & Stability Polish Agent

Senior engineer (25+ years), production debugging, Windows UI stability, memory/CPU profiling, systems reliability.

Find and fix the class of defects that are individually small but wreck the product experience: unexpected freezes, sector-scan stalls, wrong numbers, overflow/underflow, memory spikes, CPU spikes, unwanted loops, broken buttons (close / minimize / maximize / stop-scan), stuck UI states, dead wiring between UI, core, and API. These are the bugs that make a product feel broken even when the big features "work." Find every one, fix at the root cause, prove with capture, validate on real Windows.

Repos:
- Product: https://github.com/heicfast/final-prod.git
- Reference corpus (ground truth for expected behavior): https://github.com/heicfast/easeus-drw-research
- Token (env/git config only, never committed): [REDACTED]

Read the entire repository end to end with parallel agents before touching anything. Every defect is a reproduction. Every fix is proven before and after.

Phase 0 — Read, then hunt

Agents (mandatory):

- A — Source audit: every source file — swallowed errors, unchecked returns, unbounded loops, missing try/catch, race conditions on shared state, wait() without timeout, blocking calls on UI thread, integer arithmetic without bounds check, unbounded queues/allocations, missing cancellation checks, dead buttons (handlers not wired), missing event subscriptions, broken lifecycle (mount/unmount, start/stop). → worklog/SMALL-BUGS-SOURCE.md
- B — Wiring audit: every UI event → service → core/API chain. Every button. Every menu item. Every keyboard shortcut. Every screen transition. Every stop/cancel/pause. Every close/minimize/maximize. Every dialog. Trace each to its handler and its backend call and its response. Any break in the chain is a defect. → worklog/WIRING-AUDIT.md
- C — Runtime profiling: on a real Windows runner with seeded data, exercise the full product while capturing CPU, memory, thread count, handle count, GC pressure, and UI frame time at one-second intervals. Identify every spike, every freeze, every stall. → worklog/PROFILING.md
- D — UI/UX interaction: drive every button, every state, every transition. Capture every case where a button does nothing, a state gets stuck, an animation jitters, a progress bar freezes, a scan cannot be stopped, a window cannot be closed/minimized/maximized, or the UI thread blocks. → worklog/UI-DEFECTS.md
- E — Numeric/logic audit: every displayed number, every progress percentage, every sector count, every time/ETA, every size, every confidence. Verify against the actual source. Any hard-coded value, any wrong formula, any overflow/underflow path, any division-by-zero, any precision loss. → worklog/NUMERIC-DEFECTS.md

Consolidate → push → then fix.

Phase 1 — Reproduce and classify

Every candidate defect reproduced on real Windows with capture:

- Freezes / stalls: captured trace showing where, how long, what stack. Sector scan stall, UI hang, stop-scan failure — each with the exact code path and evidence.
- Memory spikes: captured allocation trace, heap snapshot, growth curve. Instant peak vs slow growth. Bounded vs unbounded.
- CPU spikes: captured profiler trace, hot function, thread dump.
- Unwanted loops: captured loop trace, iteration count, exit condition.
- Broken buttons: captured interaction, expected handler, observed behavior.
- Stuck UI: captured state, transition graph, missing transition.
- Wrong numbers: captured value vs expected value, source of wrongness.
- Wiring gaps: captured event chain with the break point named.

Classified by impact (user-visible severity) and cause (root defect). Deliverable: worklog/DEFECT-LEDGER.md — every defect with reproduction, evidence, root cause, and location. Push before any fix.

Phase 2 — Fix at root cause

Fix in order of user impact:

- Freezes and stalls: move blocking work off the UI thread; add per-operation timeouts; ensure cancellation reaches the blocking point; remove lock contention on hot paths; fix retry loops that loop without bound; ensure overlapped I/O with completion-driven flow (never a blocking wait that ignores cancellation). Every freeze proven gone with before/after capture.
- Memory spikes: identify the allocation site and eliminate the spike (streaming instead of materializing, bounded buffer instead of unbounded list, reused buffer instead of per-iteration allocation, cap every queue/cache with correct eviction). Memory stays bounded under stress.
- CPU spikes: eliminate the hot path (tight loop, per-byte processing, unnecessary recomputation, polling instead of eventing). CPU profile proves the spike gone.
- Unwanted loops: fix the exit condition, the bound, or the retry policy. No unbounded iteration under any input.
- Broken buttons: wire the handler, wire the response, wire the state update. Verify interaction end to end. Every button does what its label says, or is truthfully disabled.
- Stuck UI: ensure every state has an exit transition; ensure async operations surface completion, error, and cancellation to the state machine; ensure loading states time out with a truthful error if the backend never responds.
- Wrong numbers: fix the formula, the source, the type, the units. Every displayed number verified against the actual source. No hard-coded percentages, no phantom counts.
- Wiring gaps: connect every break. UI → service → core/API chain complete and tested end to end.
- Race conditions: fix with proper synchronization or ownership; verify under TSan / stress.
- Unhandled errors: every error path has a handler; every handler surfaces to the user or logs deterministically. No silent swallow.

Every fix: a regression test that fails before, passes after. Every fix: a captured before/after. Every fix: root cause named in the commit.

Phase 3 — Validate

- Every reproduced defect: fixed, proven, and no longer reproducible on the same scenario.
- Product full exercise on real Windows with seeded data: no freeze, no stall, no stuck UI, no memory spike, no CPU spike, no wrong number, no broken button.
- UI frame time remains responsive (no blocking > 50 ms on the UI thread) across the whole exercise.
- Stop-scan, cancel, close, minimize, maximize, restore — all work in every state, including mid-scan.
- Memory remains bounded across long-running scans and repeated operations.
- CPU remains consistent — no runaway, no idle waste.
- Full Windows CI green.
- Release candidate validated on clean Windows.

Rules

- No guessing. Every defect is reproduced with evidence. Every fix is proven.
- Root cause only. No symptom patches, no retry loops masking a failure, no Thread.Sleep to hide a race, no unbounded timeout to hide a hang.
- No fake anything. No fake progress, no fake numbers, no fake completion.
- No behavior change beyond the fix. Every fix targets the specific defect; adjacent behavior stays identical.
- No regression. Every previously working surface works after.
- No shortcuts. No stubs. No TODOs. If a defect cannot be fixed this session, it goes to docs/OPEN-ITEMS.md with precise scope, and the surface is truthfully disabled.
- Preserve audit trail.
- Token in env/git config only.
- Push after every coherent fix batch. Never force-push. main green.

Validation gates

1. Every defect in worklog/DEFECT-LEDGER.md resolved (fixed, or in docs/OPEN-ITEMS.md with precise scope).
2. Every freeze / stall / stuck UI: before/after capture proving gone.
3. Every memory spike: captured bound before, captured bound after.
4. Every CPU spike: profiler trace before, gone after.
5. Every wrong number: value before, value after, source fix.
6. Every button: interaction captured, behavior correct.
7. Every wiring gap: chain complete, tested end to end.
8. Stop-scan, cancel, close, minimize, maximize — all work in every state.
9. Full exercise on real Windows with seeded data: responsive UI, bounded resources, correct numbers, working controls.
10. Full Windows CI green.
11. Release candidate validated on clean Windows.
12. Docs updated where behavior changed; no stale claims.

Deliverables

1. Five Phase 0 context sheets, pushed before any code.
2. worklog/DEFECT-LEDGER.md — every defect, evidence, root cause, resolution.
3. Every fix with regression test + before/after capture in worklog/FIXES/<defect-id>/.
4. worklog/PROFILING.md before/after — CPU, memory, thread, handle, frame time traces.
5. docs/OPEN-ITEMS.md updated with anything not fixed, precise scope.
6. Full Windows CI green.
7. Release candidate validated on clean Windows with seeded data.
8. worklog.md — chronological handoff with every SHA, every capture, every fix, every CI link.
9. All pushed with evidence-citing messages.

TODOs (spawn 20+, complete 19, spawn 20 more)

- T1: Clone, record commit, save prompt (redacted), worklog entry.
- T2: Spawn agents A-E, produce five context sheets, push.
- T3: Consolidate worklog/DEFECT-LEDGER.md, push.
- T4: Reproduce every candidate defect on real Windows with capture, push.
- T5-T7: Fix freezes and stalls (sector scan, UI, stop-scan) at root cause, prove with before/after, push per fix.
- T8-T9: Fix memory spikes and CPU spikes, prove with profile traces, push per fix.
- T10: Fix unwanted loops and runaway iterations, push.
- T11: Fix broken buttons — close, minimize, maximize, stop-scan, and every other control — push per batch.
- T12: Fix stuck UI states and missing transitions, push.
- T13: Fix wrong numbers — progress, sector counts, sizes, times, percentages, confidence — push per fix.
- T14: Fix wiring gaps between UI, services, core, and API — complete every chain, push per chain.
- T15: Fix race conditions and unhandled errors, push.
- T16: Full product exercise on real Windows with seeded data — verify every fix in context, push.
- T17: Full Windows CI sweep, fix reds, push.
- T18: Release candidate on clean Windows, capture full evidence, push.
- T19: Update docs to current reality, final verification sweep, push.
- T20: Handoff — complete worklog.md, final push.

Final reminder

- Small bugs, large impact. These are the defects that make a product feel broken even when the "big features" work. Find them all, fix them at the root, prove each one.
- Every defect is reproduced. Every fix is proven. No guessing. No symptom patches.
- Freezes, stalls, stuck UI, memory spikes, CPU spikes, wrong numbers, broken buttons, wiring gaps — none of them survive this session.
- Stop-scan, cancel, close, minimize, maximize — every control works in every state, including mid-operation.
- No hallucination. No shortcuts. No fake anything. Production grade only.
- Work long-horizon. Commit and push often. main green at every push.
- Save to worklog/session-small-bugs-prompt.md (token redacted), read twice, start T1.

Good luck.
```

## Environment note recorded honestly at session start (agent)

The executing agent for this session is a Linux sandbox (no Windows runner, no Windows
CI executor, no GUI desktop). Windows-only work (runtime profiling captures with
Windows performance counters, UI interaction testing on Windows, the Windows CI
sweep, release-candidate validation on clean Windows) is documented with precise
scope and handed to the owner in `docs/OPEN-ITEMS.md`; nothing is fabricated. All
Linux-runnable evidence (engine suite, UI unit/RTL, mock-connector e2e, static
audit) is executed for real in this session.
