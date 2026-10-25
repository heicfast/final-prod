# MINOR N-059 — No memory monitor: the og's Log.mo carries CMemMonitorManager (memory-pressure tracking during scans)

**Severity:** MINOR
**Module:** orchestration
**Parameters affected:** F35 (error recovery under memory pressure), P49 (memory — steady-state monitoring), F40 (threading — the monitor runs on the log thread family)
**Corpus references:** `re-workspace/artifacts/MOM-modules/Log.md` (Log.mo census: the EPMLOG sink AND `CMemMonitorManager` — memory monitoring manager class), `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` (memory-pressure entries in run logs)
**Core references:** negative grep across `src/` for any memory-monitor/watchdog surface (none); the orchestrator has no memory-pressure path — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og tracks memory pressure during scans through CMemMonitorManager (censused in Log.mo)
and logs pressure events into the run-log channels. The core has no memory monitor at any
layer — large-N scans (the scalability parameter's worst case) run unmonitored, and no
memory-pressure state is available to the orchestrator's scheduling decisions.

## What the reference does

Log.mo's census includes CMemMonitorManager (the memory-monitoring manager, paired with
the log sink); qa-logs shows memory-pressure entries in the forensic trail.

## What our core does

Nothing — no monitor class, no pressure events, no log entries. (The core's memory
EFFICIENCY is separately audited and strong — block caches etc.; this finding is the
monitoring surface, not efficiency.)

## Gap

Falsifiable: an og scan under memory pressure emits monitor events and (per the qa-logs
channel) records them; a core scan under identical pressure emits nothing anywhere.

## Impact

Advisory/telemetry only — no evidence in the corpus that the og THROTTLES on pressure
(the manager feeds logs/UI). MINOR.

## Reproduction / verification

Log.md census read first-hand; negative grep verified.

## Fix plan

1. If/when N-058's run-log lands, add a lightweight process-memory sampler on the same
   sink (period + threshold configurable; default conservative) emitting pressure events.
2. Do NOT couple it to scheduling decisions without corpus evidence of og behavior.

## Tests to add

- `Orchestrator.MemoryMonitorEmitsUnderPressure` (synthetic low threshold)

## Related findings

- N-058 (run-log — the paired surface)

## Open questions

Whether CMemMonitorManager feeds anything besides logs/UI is corpus-UNVERIFIED (census
class only) — deliberately not assumed.
