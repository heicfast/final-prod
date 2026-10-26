# SEVERE S-016 — ErrorRange events never fire: the telemetry snapshot's error map is fed only AFTER the boundary emitters (the documented error-delta stream is dead code)

**Severity:** SEVERE (missing documented event on the public API surface; discovered during A-004's fail-before proof, diff-fix session 3)
**Module:** orchestration (typed event stream, GAP-S6-10)
**Parameters affected:** O38 (progress semantics), P42 (public surface shape), B8 (error-map observability — the resume/monitor UI's primary damage signal)
**Corpus references:** core-internal contract — `docs/ABI-CONTRACT.md` §6 ("Files-found/error events are STAGE-BOUNDARY samples ... the error-map delta carries the newest appended range plus the cumulative count"); `include/cr/orchestrator/cr_scan.h:60-87` (the same contract, and `Type::ErrorRange = 4 /* a read-failure range was appended (delta) */`). Og-parity context: the reference surfaces read-failure state to the UI as BadSectorTip/"trying to skip bad sectors" (FB-O03, cited in `include/cr/storage/cr_retry.h:16-19`) — the mid-run error signal is user-visible in the og.
**Core references:** `src/core/orchestrator/cr_scan.cpp` — pre-fix: `tel_counts_` (fed counters only; error map NOT fed), `stamp()` :646-656 (tel_counts_ → emit_boundary_events_ ×2 → THEN `tel_.snap.errors.merge(stats.errors)`), stage ends :721-722/:1084-1085/:1138-1139 (tel_counts_ → emit_boundary_events_, same empty snapshot), `emit_boundary_events_` reading `tel_.snap.errors.ranges()` and CAS-ing `evt_last_errors_` — discovered and proven at `7f2c83e` (A-004 verification)
**Status:** FIXED in the A-004 fix commit (same delivery path — the two defects are prerequisites of each other's user-visible symptom). Full Resolution below.

## Summary

Every `emit_boundary_events_` call site reads the error map from the telemetry SNAPSHOT (`tel_.snap.errors`), but the snapshot's error map was only merged inside `stamp()`'s finalize block — which runs AFTER the boundary emits. At every emit point (three stage ends + the stamp-time pair) `count` was therefore always 0, the `count > last` delta gate never opened, and **the `ErrorRange` event never fired — not once, in any scan, since the event stream's introduction (`56cce87`)**. The documented contract (ABI-CONTRACT.md §6) promises stage-boundary error deltas; consumers of the typed event stream (the cr_ui bridge, any monitoring UI) received nothing. The public `telemetry()` query path was correct (it live-merges the retry pipe, G-04/G-05/G-06) — only the event path was dead, which is also why the suite stayed green: no test asserted ErrorRange delivery.

## What the reference does

The event contract (`cr_scan.h:60-87`, ABI-CONTRACT.md §6): "Files-found/error events are STAGE-BOUNDARY samples (the engines do not publish per-file increments); the error-map delta carries the newest appended range plus the cumulative count." The og-parity frame: the reference surfaces bad-sector state mid-run (BadSectorTip, FB-O03); the typed event stream is the core's transport-agnostic equivalent of that signal.

## What our core does (pre-fix)

```cpp
/* stamp(): the ONLY place tel_.snap.errors was fed */
tel_counts_(stats, cancel);                        /* counters only  :646 */
emit_boundary_events_(...);                       /* count == 0    :647 */
emit_boundary_events_(...);                        /* count == 0    :648 */
...
tel_.snap.errors.merge(stats.errors);              /* fed here      :656 — after every emit */
```

Stage ends (`run_quick` :721-722, `run_deep` :1084-1085, carve :1138-1139) follow the same order: `tel_counts_` (no error feed) → `emit_boundary_events_` (reads the still-empty snapshot). The CAS guard `evt_last_errors_` (initialized so the first nonzero count emits) never saw a nonzero count.

Executed proof (fail-before, session 3 probe — a FAT16 fixture with one deterministically failing sector inside the FAT2 span, read wholesale by `FatTable::load` at `cr_chain.cpp:100`): the scan completes with `stats.errors` carrying exactly one range (`off=15360 len=512`, rc=0, files=2) — the retry pipe records the isolation — while an event recorder attached via `set_event_sink` receives **zero** `ErrorRange` events. The error data existed in the pipe; the event path could not see it.

## Gap

Falsifiable and executed (above): with a populated retry-pipe error map and a live sink, no ErrorRange event fires. The contract in ABI-CONTRACT.md §6 is violated for the entire error-delta stream.

## Impact

- **Observability:** the single most damage-relevant mid-run signal (where reads are failing) is absent from the event stream that bridges (cr_ui) and embedders consume. The `error_ranges` cumulative field of every other event is also frozen at 0 mid-run (only `telemetry()` queries could see the truth).
- **Downstream masking:** this defect made A-004's null-call crash UNREACHABLE in practice (the wrong-sink branch sits inside the delta gate that never opened) — the audit's A-004 "crashes on the first appended read-error range" premise was latent behind S-016. A-004's code path was real and verified by read; S-016 is why no embedder had crashed yet.
- **Test gap:** no test asserted ErrorRange delivery (the suite stayed green through both defects). Fixed alongside: the A-004 proof tests now pin the stream.

## Fix (in the A-004 commit — same function family, one delivery path)

`tel_counts_` now live-merges the retry pipe's error map into the snapshot under the telemetry lock (the exact `error_map_copy()` merge `telemetry()` already performs, G-04/G-05/G-06), because `tel_counts_` is the common ancestor of every `emit_boundary_events_` call site (stamp + all three stage ends). Idempotent by construction: `ReadErrorMap::add` sorts and absorbs touching/overlapping runs (`cr_devmap.cpp`), so re-merging pipe state at each boundary only grows the count when new failures were recorded; the `evt_last_errors_` CAS still deduplicates the delta emits across worker threads.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the A-004 fix — S-016 is fixed in the same commit because the two defects are one delivery path: fixing either alone leaves the ErrorRange stream broken or crashing)
**Verified by:** fail-before/pass-after executed proof — session-3 probe (FAT16 + one failing FAT2 sector, `FatTable::load` reads the span wholesale): pre-fix `ScanEvents.ErrorRangeGoesToLiveSink` FAILED with zero ErrorRange events received while `stats.errors` carried the range (probe log above); post-fix both `ScanEvents.ErrorRangeGoesToLiveSink` and `ScanEvents.SinkSwitchDeliversAllTypes` PASS (ErrorRange arrives with `err_start_bytes=15360, err_len_bytes=512`, `error_ranges >= 1`). Full suite 305/305 Linux ASan green (303 + the two ScanEvents tests).
**Resolution notes:** `tel_counts_` feeds the live retry-pipe error map into `tel_.snap.errors` before every boundary emit; `emit_boundary_events_` delivers through the live sink (A-004's half — see that finding for the wrong-sink fix). The stamp-time final merge (:656) is retained — idempotent under `ReadErrorMap` normalization. No safety hardening near this fix; no contract change (the fix restores the documented contract rather than altering it). Win7-inclusive gate unaffected (no new imports).
