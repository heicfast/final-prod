# MINOR N-062 — No memory monitor: the og's Log.mo carries CMemMonitorManager (memory-pressure tracking during scans)

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

1. If/when N-061's run-log lands, add a lightweight process-memory sampler on the same
   sink (period + threshold configurable; default conservative) emitting pressure events.
2. Do NOT couple it to scheduling decisions without corpus evidence of og behavior.

## Tests to add

- `Orchestrator.MemoryMonitorEmitsUnderPressure` (synthetic low threshold)

## Related findings

- N-061 (run-log — the paired surface)

## Open questions

Whether CMemMonitorManager feeds anything besides logs/UI is corpus-UNVERIFIED (census
class only) — deliberately not assumed.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-062 commit — DFX18 session 18)
**Verified by:** `Orchestrator.MemoryMonitorEmitsUnderPressure` (the pure threshold check: Warn line at/above with both numbers; silent below; silent when OFF (0) even at an enormous working set) + the `CrUi.HeartbeatDuringStall` extension (the monitor rides the same 5 s watchdog cadence during the parked stall window: ≥1 "memory pressure: working_set=" Warn line through the cr::log seam — fail-before RED executed on the stash-isolated tree: the shim reverted, the seam and tests present, zero memory lines during the window). Suite 734/734 linux-release at the landing tree.
**Resolution notes:** Per the finding's own fix plan: "a lightweight process-memory sampler on the same sink" — the sink is the N-048 cr::log seam landed earlier this session. The pure check (`cr::log::check_memory_pressure(threshold_mb, working_set_mb)` — Warn line with both numbers; OFF at threshold 0) lives in the core (no OS headers in the core — the sampling is platform-side); the sampler (`working_set_mb_()`: /proc/self/statm resident on POSIX, GetProcessMemoryInfo WorkingSetSize on Windows; 0 on an unreadable source — a failed sample never warns) lives in the shim and rides the heartbeat watchdog's 5 s cadence (the watchdog now starts when events are subscribed OR the monitor is armed — a silent host can still consume the seam). Config: `cr_ui_scan_config.memory_warn_mb` (appended tail field, zero-init safe; 0 = OFF = the conservative library default — the og's always-on monitor is its UI process's choice, registered in the header comment). NOT coupled to scheduling decisions — the finding's own rule (no corpus evidence the og throttles; the manager feeds logs/UI); the emitted Warn line is advisory only. The period rides the watchdog's fixed 5 s cadence (the og's CMemMonitorManager cadence is not decoded in the corpus — the Log.md census names the class only; the 5 s cadence is this repo's existing liveness tick, documented as the chosen period).
