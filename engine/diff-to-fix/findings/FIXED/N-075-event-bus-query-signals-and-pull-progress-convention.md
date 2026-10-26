# MINOR N-075 — Event-bus contract shape: og query-signals (bidirectional, reference-out) + pull-polled progress + typed IUIProgressInfo vs core one-way push

**Severity:** MINOR
**Module:** orchestration (callback model), abi
**Parameters affected:** A38 (progress semantics), A41 (callback threading), P42 (public surface shape), A44 (async model)
**Corpus references:** `re-workspace/artifacts/UIUX/25-connect-wiring-graph.md:41-62` (`doDispatchMessage(int,void*,int&)` — 15 UI subscribers, the int& write-back edge; "the double bridge at 1369866/1369872 … the module-side publisher feeding the UI-side bus"), `:125-144` (`sigIsBusy(bool&) ×14`, `sigIsStopping(bool&) ×8`, `sigGetRightIndex(int&,int&) ×4` — "reference-out-param query signals"), `:175-192` ("Progress = poll, events = push: scan/finish events push; progress percentages pull (38 timeout-consumers). The only progress-push signal is `sigPercentChange` (CUISink's)"), `UIUX/20-moc-surface-sink-architecture.md:145` (typed `IUIProgressInfo` recovery-progress interface)
**Core references:** `src/core/orchestrator/cr_scan.cpp:126-133` (one-way typed event stream, single sink, subscribed pre-run); `include/cr/cr_ui.h` telemetry pull op (exists — the core DOES have one pull surface); no isBusy/isStopping query op; progress = push (CR_EVT progress events); no typed progress-info interface (bytes-only counters)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's engine→UI bus is bidirectional (push events AND query-signals with reference-out-params: `sigIsBusy(bool&)`, `sigIsStopping(bool&)`, `sigGetRightIndex(int&,int&)`), its progress model is predominantly pull-polled (38 `timeout()` consumers calling `onGet*Progress`), and its recovery progress is a typed `IUIProgressInfo` interface. The core's contract is one-way push (typed events) with a single pre-run-subscribed sink, one pull op (telemetry), and bytes-only progress counters.

## What the reference does

The connect-wiring-graph decode (first-cited this session): the MOM bus `doDispatchMessage(int,void*,int&)` carries an int& write-back edge and 15 subscribers; the results-page hub wires `sigIsBusy(bool&) ×14` and `sigIsStopping(bool&) ×8` — hosts QUERY engine/UI state through reference-out-param signals; progress percentages are pull-polled (`onGet*Progress` via 38 timer consumers), with `sigPercentChange` the only push; recovery progress rides a typed `IUIProgressInfo` (moc-surface-sink:145).

## What our core does

The core pushes typed events (CR_EVT_*) to a single sink subscribed before run; `telemetry()` is the one pull surface (stage/permille/files counters); there is no isBusy/isStopping-style query op and no typed progress-info interface — the ABI's progress payload is the byte/permille set.

## Gap

Falsifiable claim: an og host can synchronously ask "is the scan busy/stopping?" and "which results page/index is live?" through the bus and get an answer through the out-param; a cr_ui host cannot (it can only subscribe to push events and call telemetry for counters). Additionally the og's typed IUIProgressInfo (recovery: per-item states) has no direct ABI counterpart (the core's repair/report events are the nearest).

## Impact

API-contract nuance for hosts porting from the og's wiring style: a host built around query-signals must restructure to event-driven. The core's push model is cleaner and deliberate (D-21/D-30x register the threading/event design); the residual is the missing busy/stopping QUERY semantics (achievable today only by racing on events) and the untyped progress payload.

## Reproduction / verification

Corpus-derived (both artifacts first-cited this session; counts and the poll/push verdict quoted with lines). Core side verified in-source at the audited commit.

## Fix plan (for the engineering team)

1. Cheapest honest close: add `cr_ui_scan_busy(handle)` + `cr_ui_scan_stopping(handle)` query exports (trivial over the existing state machine — the scan controller already tracks running/cancel-requested under the telemetry mutex); register the pull-progress convention difference as DELIBERATE in PARITY-DELTAS (the core's push events + telemetry pull already cover the information content).
2. If a host needs typed per-item recovery progress: the writer's report_items/queue surfaces already expose item states; document the mapping in ABI-CONTRACT rather than adding a parallel interface.
3. Tests: `Abi.ScanBusyStoppingQueries` (busy=true mid-run via a blocking read fixture, stopping=true after cancel(), false at rest).
4. Acceptance: the two query ops land with the ABI append rule (no bump) or the deliberate-difference note lands in PARITY-DELTAS; either closes the finding.

## Tests to add

- `Abi.ScanBusyStoppingQueries`

## Related findings

N-016/N-017 (emit-boundary/cancel-watcher — same callback-model family); S-016/A-004-FIXED (event delivery); N-043 (progress watchdog — the liveness side of the progress contract); D-21/D-30x (the registered threading/event deltas this builds on).

## Open questions

Whether the og's query-signals are answered SYNCHRONOUSLY on the caller's thread (the direct-connection Qt semantics suggest yes for same-thread wiring; queued for cross-thread) — the core's query ops in the fix plan are synchronous by construction, which is the stronger contract.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-075 commit — `git log` "fix(N-075)")
**Verified by:** `CrUi.ScanBusyStoppingQueries` (the C++ mid-run proof: busy=true observed through the pause-gate window, stopping flips the instant cancel() lands while busy stays true through wind-down, busy=false + one-shot stopping=true after the cancelled run returns, at-rest zeros, the four bad-argument contracts) + the plain-C edges in `cr_ui_abi_check` (at-rest zeros, stopping=1 after pre-run cancel, busy=0 pre-run, the bad-argument contracts compile and refuse under plain C). Fail-before EXECUTED: build RED on the stash-isolated pre-fix tree (`cr_ui_scan_busy`/`cr_ui_scan_stopping` not declared — the symbols did not exist; ui_abi_check.c + test_cr_ui.cpp both fail at compile), pass-after green. Suite **738/738** linux-release.
**Resolution notes:** The finding's own "cheapest honest close" fix plan executed exactly. (1) `cr_ui_scan_busy` + `cr_ui_scan_stopping` appended to the cr_ui ABI (v2-era append, no bump — ABI-CONTRACT §9 rule): synchronous query ops over the shim's run-lifecycle atomic and the PausableToken's live cancel request — the og's `sigIsBusy(bool&)` ×14 / `sigIsStopping(bool&)` ×8 reference-out query signals (corpus UIUX/25-connect-wiring-graph.md :125-144, re-verified first-hand this session: the results-page hub wiring table and the ×8/×14 counts) surfaced at the ABI boundary. stopping reads the LIVE token state (distinct from telemetry's post-run `cancelled` flag) and persists after the one-shot run ends — the honest "a cancel WAS requested on this handle" answer, the same truth the og's one-shot stop event gives its isStopping subscribers. (2) The pull-progress convention difference registered as PARITY-DELTAS **D-78**: the og's poll-dominant model (38 timeout consumers calling `onGet*Progress`, `sigPercentChange` the only push — :175-192) vs the core's coalesced monotonic push + any-thread telemetry pull; the information content is covered by the two existing surfaces, so a host wanting og-style polling calls telemetry on its own timer. (3) The typed `IUIProgressInfo` mapping documented in ABI-CONTRACT §5 (recovery-progress mapping note): `cr_ui_recovery_progress_fn` (progress edge) + `cr_ui_recovery_queue_count` (live depth) + `cr_ui_recovery_get_stats` (per-outcome counters) + `cr_ui_recovery_report_json` (per-item state truth, valid at any time) — deliberately NO parallel interface. The synchronous-by-construction note in the finding's open question is resolved in the stronger direction (direct call, no Qt-connection dependence). ABI-CONTRACT §5 flow diagram gains the state-query line; the C consumer test exercises the new exports per the ABI-append proof rule.
