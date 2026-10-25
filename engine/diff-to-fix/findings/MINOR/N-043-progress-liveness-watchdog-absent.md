# MINOR N-043 — Progress liveness watchdog absent: the og advances coarse progress every 5 s when an engine is silent; the core's progress freezes honestly

**Severity:** MINOR
**Module:** orchestration / abi
**Parameters affected:** P38 (progress semantics — liveness vs weighting/monotonicity), P53-adjacent (cancellation responsiveness display), P41-adjacent (callback threading — the watchdog thread fires callbacks off the run thread)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:910565-910585, :910864` — DRWUI starts a watchdog thread (CreateThread FUN_140577100) that every 5000 ms bumps the coarse progress counter (+0xe8++) and re-fires the progress callback (vt+8) when the engine's step counter (+0x8fc) lags the expected count (+0x8f8); cancellation = flag at scanctx+200, join with 10 s wait; the engine-side cancel poll (vt+10, per record batch) is the real responsiveness mechanism
**Core references:** `src/core/orchestrator/cr_scan.cpp` (progress bridge: single-flight monotonic weighted permille, stale-drop — engine-driven only, no heartbeat; grep "watchdog|heartbeat" → 0), `include/cr/cr_ui.h:103-118` (progress contract), `src/ui/cr_ui.cpp` progress bridge implementation; audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied (candidate for INFORMATIONAL-superset reclassification — see Open questions)

## Summary

The og's host runs a 5-second watchdog that advances the displayed coarse progress whenever an engine goes silent (long bad-sector retries, SMR stalls, pathological clusters), keeping the progress bar alive; the real progress values catch up when the engine resumes. The core's progress is strictly engine-driven: a stalled engine means a frozen progress bar AND no events — honest, but a UX regression relative to the reference for stalls >5 s. Cancellation itself is unaffected (the cancel token is polled at loop heads, which is the actual responsiveness contract, and matches).

## What the reference does

Watchdog thread (quotes in
`evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §5): every
5000 ms, if the engine step counter lags the expected count, bump +0xe8 (coarse
progress) and re-fire the progress callback. The displayed percentage therefore
advances during stalls — a deliberate cosmetic liveness mechanism (the og shows
movement that does not correspond to completed work).

## What our core does

The orchestrator's progress bridge publishes per-stage telemetry and a monotonic
weighted permille computed from engine-reported done/total only. No timer, no
heartbeat, no synthetic advance. A stalled engine (e.g. 30 s of bad-sector retries
inside one read) produces zero progress events for 30 s.

## Gap

Falsifiable: for an engine silent ≥5 s, the og's host emits progress events; the core
emits none. Verified by decompile (og) and code read + grep (core).

## Impact

User-facing perceived hang during stalls; hosts building "is it alive?" heuristics on
progress-event frequency will false-positive. No correctness impact; no recovery-
results impact. Note the og's mechanism is dishonest progress (it fabricates
movement) — the core's choice is defensible; the finding exists so the difference is
a recorded decision, not an accident.

## Reproduction / verification

- Decompiled evidence (method 2) for the og side (cited lines); core side verified by
  read + `rg -i "watchdog|heartbeat" src/` → 0 hits.
- Not executed against a stalling device this session (would need fault-injection
  hardware/driver mock; the mechanism difference is fully evidenced without it).

## Fix plan (for the engineering team)

1. Decision: either (a) accept-and-document as a deliberate honesty improvement
   (reclassify this finding INFORMATIONAL with a doc note in ABI-CONTRACT's progress
   section: "progress is strictly work-derived; no synthetic heartbeat"), or
   (b) add a liveness heartbeat event (new event type `CR_UI_EVENT_HEARTBEAT` with
   elapsed-time payload, NOT a fabricated percentage advance) so hosts can distinguish
   "alive, stalled on I/O" from "hung". Recommend (b) — it preserves honesty while
   closing the UX gap.
2. **Files to modify**: `include/cr/cr_ui.h` (event type), `src/ui/cr_ui.cpp`
   (timer thread or poll-point emission), `src/core/orchestrator/cr_scan.cpp`
   (optional stall telemetry: last-progress-delta timestamp in the snapshot),
   `docs/ABI-CONTRACT.md`.
3. **Tests to add**: `Progress.HeartbeatDuringStall` (mock device with 10 s read
   latency → ≥1 heartbeat event, zero fabricated permille advance),
   `Progress.PermilleStrictlyMonotonicUnderHeartbeat`.
4. **Acceptance criteria**: a stalled mock device yields periodic heartbeat events
   with unchanged work-permille; suite green.

## Tests to add

`Progress.HeartbeatDuringStall`, `Progress.PermilleStrictlyMonotonicUnderHeartbeat`.

## Related findings

- **O38 progress (M-equivalent)** — the weighting/monotonicity verdict stands; this
  finding adds the liveness dimension only.

## Open questions

1. Whether the heartbeat should also carry a stall-reason hint (retry-pipe state) —
   design question for the engineering team, not an audit question.
