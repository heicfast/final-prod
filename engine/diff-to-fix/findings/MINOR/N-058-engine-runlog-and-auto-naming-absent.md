# MINOR N-058 — No persistent engine run-log and no session auto-naming: the og writes EPMLOG.log / vendor-log session forensics (doScan ranges, filter pushes, batch-1000 tree build) and auto-names sessions auto_<label><M><D><H><m><s>

**Severity:** MINOR
**Module:** orchestration + session (+ logging)
**Parameters affected:** F35 (error recovery — field diagnostics), Q68 (docs), D12-adjacent (session naming contract), Q46 (support workflows)
**Corpus references:** `re-workspace/artifacts/MOM-modules/Log.md` (Log.mo = the EPMLOG.log file-mapping sink + CMemMonitorManager), `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` + `vendor-logs--qa-session-forensics.md` (the og's run-log semantics: `CScanMgr::doScan` entries, quick/deep sector ranges, `PushFolderFilter` calls, 3-branch tree build in ~1000-item batches, `CalcPartition` fileCount-ranked candidate resolution), `og-codebase/EPMLOG.log` (the shipped log file itself)
**Core references:** `src/app/rrecov.cpp` (CLI `--log` flag is the only logging surface — no persistent engine log), `src/core/orchestrator/cr_scan.cpp` (no run-log emission; session files named by the caller, no auto-naming convention) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og leaves a forensic trail for every scan (EPMLOG.log + vendor/QA log channels:
scan-manager entries, sector ranges, filter pushes, tree-build batching, candidate
resolution) and auto-names saved sessions `auto_<label><M><D><H><m><s>`. The core has
neither: logging is a CLI-only flag with no persistent engine log, and session naming is
entirely caller-driven with no default convention.

## What the reference does

The qa-logs/vendor-logs artifacts (both first-read this session) document the og's run-log
content: `CScanMgr::doScan`, quick/deep sector ranges, `PushFolderFilter`, the 3-branch
tree build in ~1000-item batches, `CalcPartition`'s fileCount-ranked resolution, and the
session auto-naming convention. Log.mo provides the EPMLOG.log file-mapping sink.

## What our core does

The orchestrator emits structured events (telemetry family) but writes no persistent log;
the CLI's `--log` prints to the console only. Session files take caller-provided names;
no auto-naming exists anywhere.

## Gap

Falsifiable: after an og scan, EPMLOG.log + the vendor channel reconstruct what was
scanned (ranges, filters, batches); after a core scan, no artifact reconstructs the run
without the caller subscribing to every event. Saved sessions default to
`auto_<label>...` in the og; the core requires an explicit name.

## Impact

Support/forensics workflows (the reason the corpus documents these channels) degrade to
best-effort event replay; session libraries (the og's SaveScan list) rely on the
auto-naming convention for default entries. Engine-adjacent but presentation-weighted:
MINOR.

## Reproduction / verification

Both corpus artifacts + Log.md read first-hand this session (the content list is the
artifacts' own); core negative-space greps verified (no persistent log writer, no
auto-naming).

## Fix plan

1. Add an optional persistent run-log sink to the orchestrator (default OFF; enabled by
   config/flag): one line per scan start/end, per stage boundary, per filter push, per
   candidate resolution — the qa-logs channel's semantic set, append-only, size-capped.
2. Add a session auto-naming helper (`auto_<label><M><D><H><m><s>` local time, matching
   the og convention) used when the caller passes no name; document it in the session
   contract.
3. Register both in PARITY-DELTAS as deliberate-if-deferred additions.

## Tests to add

- `Orchestrator.RunLogRecordsScanLifecycle`
- `Session.AutoNamingConvention`

## Related findings

- M-043 (error-model — the run-log is the natural consumer of error contexts)
- N-059 (memory monitor — the same Log.mo census surface)

## Open questions

Whether the og's vendor-log channel is opt-in (QA builds) vs always-on is not fully
documented — the fix's default-OFF choice is the conservative read; a corpus decode of the
channel gating would pin it.
