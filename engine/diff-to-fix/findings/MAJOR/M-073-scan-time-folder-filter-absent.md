# MAJOR M-073 — Scan-time folder filter absent (PushFolderFilter: the og constrains tree insertion, the core filters only post-hoc)

**Severity:** MAJOR
**Module:** orchestration / query / ABI / CLI
**Parameters affected:** S31 (scan lifecycle — a pre-set filter constrains incremental tree insertion), D16 (filter model), P42 (public surface shape)
**Corpus references:** `re-workspace/artifacts/UIUX/31-filterbar-family-decode.md:142` ("**Filter → scan interplay** (runtime-verified): at scan start the log prints `FilterPath=` + `CDoScanPartition::PushFolderFilter:84` + `AdvanceFilter current time is :...` — the filter condition is pushed INTO the scan controller, so pre-set filters also constrain incremental tree insertion (not just post-hoc list filtering)"); corroborated by the og runtime log `og-codebase/log/DRWUI.log` (`CScanMgr::doScan → CDoScanPartition::PushScanDevice/PushFolderFilter/DoScan/ScanThread` — the PushFolderFilter call in the live scan-start sequence)
**Core references:** audited `35adcb9`. `include/cr/orchestrator/cr_scan.h:185-327` — the full `ScanConfig` (stages, fs_hint, threads, repair hooks, check, labels/identity, autosave_dir, checkpoint sink, LDM resolver, events): **no user-supplied folder-filter input**. ABI twin `include/cr/cr_ui.h:516-588` — same absence. The only scan-time filter is the fixed junk blocklist (`cr_filterfolder.h`, applied once per stage at `src/core/orchestrator/cr_scan.cpp:1202` — a hidden-set name filter, not a caller path filter). Grep `rg -n "filter_folder|filter-folder"` → only `docs/PLAN-API-EXPANSION.md:48`.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

In the og, a pre-set filter condition is pushed into the scan controller at scan start (`PushFolderFilter`), constraining which folders' results enter the tree at all. The core has no scan-time folder-filter input in `ScanConfig` or its ABI twin — a caller cannot restrict a scan to one or more folder prefixes; only post-hoc query filtering (M-072's axes, M-029's prefix) exists.

## What the reference does

At scan start the og's `CDoScanPartition::PushFolderFilter` (:84 in the live call chain) receives the current AdvanceFilterCondition and constrains incremental tree insertion — the runtime-verified interplay recorded at UIUX/31:142 with the `FilterPath=` log line at scan start. The effect: scans of a sub-tree (e.g. `\Users\alice\Documents`) don't pay tree/merge cost for out-of-scope folders.

## What our core does

`ScanConfig` (cr_scan.h:185-327) exposes stages, hints, threads, repair hooks, check config, identity/labels, autosave, checkpoints, LDM resolution, events — no filter input. The ABI twin matches. The scan pipeline applies exactly one fixed, non-configurable filter: the FilterFolder junk blocklist (M-018), applied once at scan completion. Out-of-scope folder exclusion is available only after the fact via `query` (path_prefix) — the tree itself is always full.

## Gap

A caller cannot restrict a scan's result tree to folder prefixes at scan time: no `filter_folders` in `ScanConfig`/`cr_ui_scan_config`, no CLI flag. The og pushes the filter in, constraining insertion.

## Impact

Efficiency/consistency gap (the user-visible outcome — "show me only files under X" — is reachable post-hoc through query, so this is not a recall gap). Large-volume scans (deep trees) pay full insertion cost where the og prunes at insertion; a scan+filter workflow that re-opens a session sees the full tree every time.

## Reproduction / verification

- Corpus-derived (UIUX/31:142 quoted verbatim; the live DRWUI.log sequence).
- Core absence first-hand: `ScanConfig` + `cr_ui_scan_config` read field-by-field (the enumerations above); grep zero hits for scan-time filter inputs.

## Fix plan (for the engineering team)

1. `include/cr/orchestrator/cr_scan.h`: `ScanConfig` gains `std::vector<std::string> filter_folders;` (UTF-8 path prefixes; empty = off).
2. `src/core/orchestrator/cr_scan.cpp`: post-parse prune before node insertion (the plan-of-record's exact posture: "engines still read metadata — the og's filter is a results filter riding the scan… post-parse prune before node insertion") — reuse the `path_compatible` prefix pruning from `cr_query.cpp:165-185`; document the ordering interplay with M-018's junk pass.
3. ABI append: `cr_ui_scan_config` tail `const char* const* filter_folders_utf8; uint32_t filter_folder_count;` (zero-init safe, append-only).
4. CLI: `scan --filter-folder P` (repeatable) + the three-site lock (+ the `-h` block per N-094).
5. Session round-trip: persist the filter so resume preserves it (a config-record field or the REC extension mechanism; the plan's test words this).
6. Tests: seeded tree with out-of-prefix nodes → absent from the saved snapshot; filter preserved across save/resume; empty-filter = unchanged behavior pin.

## Tests to add

- `ScanTimeFolderFilter.PrunesOutOfPrefixNodes`
- `ScanTimeFolderFilter.EmptyIsOff` (behavior-unchanged pin)
- `ScanTimeFolderFilter.SurvivesSessionRoundTrip`
- CLI: `scan --filter-folder` then `tree` shows only in-prefix rows.

## Related findings

- M-072 (the post-hoc axes of the same og structure).
- M-018 (the fixed junk filter — the only scan-time filtering that exists today; ordering interplay noted).

## Open questions

None.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

ScanConfig now closes at cr_scan.h:332 (grew by M-079 label fields — still no filter_folders); ABI twin cr_ui.h:531-608; the junk blocklist pass now cr_scan.cpp:1196-1207 (apply at :1207).
