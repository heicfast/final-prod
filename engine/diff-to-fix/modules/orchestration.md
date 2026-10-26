# modules/orchestration.md — scan orchestrator audit verdicts

Reference: og stage model per `architecture/04-recovery-workflow.md` +
`DRWUI-exe--main-ui-architecture.md` §9.2 (quick → advanced composition: all full FS
searchers + RAW gated on settings+0x60), progress semantics (status strings, "Remaining
time", Reading sector X/Y), completion state machine (bottomarea nStatus=4,
m_iScanType 3→4), thread priority 7 workers, scan-abort manual-reset event.

Core (audited `67193b6` + concurrent fixes in tree): `src/core/orchestrator/cr_scan.cpp`
(1,126 LOC) + `tests/unit/test_orchestrator.cpp` + `test_pause_resume.cpp` +
`test_parallel.cpp`.

## Verdicts

- **O37 stage model** — `M`-equivalent: quick → deep (+remnant sweep + lost discovery) →
  carve; the og's quick/advanced split maps to core's quick/deep+carve (carve is a
  separate stage in core; in the og, RAWSearcher is part of the advanced set — S-006/M-005
  cover the composition differences).
- **O38 progress** — `M`-equivalent: weighted single-division permilles, per-stage
  monotonic, pause-compensated elapsed; og's 40→65% save formula and bottomarea
  semantics are UI-session constructs (session module note); engine-level callbacks
  bridge with veto (progress-false → cancel) — matches the og's cancel cadence.
- **O39 ETA** — `M`-note: core exposes telemetry for ETA computation (elapsed, rates);
  the og's "Remaining time" formatting is UI-side. No engine gap.
- **O40 threading** — `M`: engine pass single-threaded with intra-engine parallelism
  (remnant fan-out, carve workers, candidate scans — per-worker stats, F-B-11 deleted
  merge fix); og workers at priority 7 — core does not set thread priority (NA for a
  CLI/engine library; UI layer decision).
- **O41 callback threading** — `M`: documented bridge (callbacks fire from worker
  threads via engine_progress_bridge_; cr_ui documents the lock order).
- **S31–S35 state machine** — `M`: stage bits stamped only on success; pause/resume
  truth (G-07); checkpoint per stage boundary (G-13, fresh container, 'RKPP' tag);
  cancel reaches every path incl. discovery; retry→cache pipe with live error map.
- **S36 session lifecycle** — `M` (see session module).
- **R54–R58 architecture** — `M`: single dispatch table (ARCH-DUP-4), LDM probed
  unconditionally pre-hint, layering clean (AUDIT-ARCH 0 violations re-verified by
  agent C).
- **B8 multi-volume** — `M`: LDM volumes via LdmVolumeDevice under "Dynamic Volume"
  roots; lost candidates parallel-scanned with per-candidate rebase (F-1).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-005 | MAJOR | Carve stage scans whole slice (og: bitmap-gated free-space on volume scans) |
| S-008 | SEVERE | (wiring side) pre-scan repair absent |

## Post-baseline note

Concurrent fixes landed in-window and are IN the audited tree: GAP-S6-6 (candidate
dedup containment), GAP-S6-7 (metadata-only skip), GAP-S6-8 (carve absolute-extent
dedup), GAP-S6-13 (64 KiB transport envelope).

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **O38 — N-016 (MINOR):** `emit_boundary_events_` double-call bug at
  `cr_scan.cpp:577-578` — the same call appears twice, once with 4-space
  indent and once with 8-space indent (clear merge artifact). The function
  emits `FilesFound` + `ErrorRange` events; double-calling double-emits these
  on every stage exit. The other 3 call sites (`:652`, `:1016`, `:1070`)
  each call it once. UI consumers counting events see double counts on the
  final boundary tick.
- **X53 — N-017 (MINOR):** cancel watcher coverage inconsistent. The carve
  multi-threaded path spawns a 50 ms watcher (`cr_carve.cpp:418-429`) that
  polls `cancel->cancelled()` and calls `pool->abort()`. The orchestrator's
  parallel candidate scan (`cr_scan.cpp:952-1003`) uses `parallel_for(pool,
  n, worker_job)` with NO equivalent watcher; workers poll the external
  `cancel` token at engine loop heads only. A queued candidate that starts
  AFTER a user cancel will not bail until its first engine loop head — could
  be hundreds of records in. Worst-case latency bounded by one candidate's
  first poll cadence.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-016 | MINOR | emit_boundary_events_ double-call bug (telemetry inflation) |
| N-017 | MINOR | Cancel watcher coverage inconsistent (carve has, orchestrator doesn't) |
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (post-closeout commits 56cce87/e93decf were never session-1-audited):**
- **A-004 (ACUTE, new) — FIXED (session 3):** `emit_boundary_events_` guarded on the
  live sink but delivered ErrorRange through `cfg_.event_fn` — null-call crash on the
  documented `set_event_sink` pattern, mis-delivery when both sinks differ. FIXED:
  live-sink delivery + two ScanEvents tests. **S-016 (SEVERE, session-3 discovery,
  fixed in the same commit):** the snapshot error map was fed only after the boundary
  emitters, so the ErrorRange stream never fired in any scan (A-004's crash was latent
  behind it); `tel_counts_` now feeds the live pipe map before every emit.
- **N-014a filed (open):** duplicate boundary emission at scan teardown — also tracked
  by the parallel leg as N-016 (the `stamp()` :647-648 pair); one MINOR fix pending.
- **M-018 filed:** og's default junk filter (FilterFolder.ini family, PushFolderFilter
  wiring) has no core counterpart and was never registered.
- **M-017:** og candidate scoring lives in DRWUI (content-driven) — orchestration-level
  consequence: lost-partition ordering diverges.
- **GAP-S6-10 typed event stream otherwise verified consistent** with the og's event-bus
  model (granularity/payloads; the og's MO-layer progress weighting remains matched per
  session-1 verdicts — no new deltas found in the MO sweep).


## S-023 grading verdicts (close-all-tiers session — the post-scan CRecoverability pass)

- **P15 (grading model)** — **S-023 FIXED** (findings/FIXED/S-023): the og's
  3-grade model {Low, Average, High} now exists as `tree::Grade` with the og
  setter semantics (monotonic-max, grade-2 sticky, overlap flag set with
  forcing ≥ 2). The post-scan pass (`tree::grading::grade_tree`,
  `cr_grading.cpp`) runs at scan completion: size floor, Email-category fast
  path, existing-file rule, carve-device small-file rule, NTFS device-index
  rule (+ QuickScanDeleteExtend), and the 169-entry extension-table-gated
  extent-end twin detection (FAT-family −2 address offset). Per-selection
  `regrade_node` (grade-1 only). The in-walk 5-tier Confidence is untouched
  (allocation-axis superset, different question).
- **P13 (assistant statistics)** — S-023 FIXED: `AssistantStats` (8 counters
  + 8 category counts + matched/total with the small-result heuristic) in
  ScanStats; persisted as REC_ASSISTANT_STATS (record type 6); surfaced
  via `cr_ui_scan_assistant` and the report's `scan_analysis` block.
- **P38 (grading telemetry)** — S-023 FIXED: graded_count on the stats; the
  pass is a deterministic post-scan stage (the og's two background jobs are
  its message-pump implementation detail).
- Registered non-adoptions (in the Resolution): the og's UNDECODED NTFS
  vt+0xb8/vt+0x100 sub-rule and its internal matched/total composition.

---

## Session-5 audit (audited commit `7cf6754`)

- **N-061 (MINOR, NEW):** no persistent engine run-log (the og's EPMLOG.log + qa/vendor
  channels record doScan, sector ranges, filter pushes, batch-1000 tree builds,
  CalcPartition resolution) and no session auto-naming convention.
- **N-062 (MINOR, NEW):** no memory monitor (og CMemMonitorManager — advisory only; do
  not couple to scheduling without corpus evidence).
- **N-060 (MINOR, NEW):** save_session lacks progress/cancel (og 40%→65% per-node with
  per-child IsCancelled teardown).
- **S-023 addendum:** OQ-S4-1 RESOLVED — the 169-entry grading extension table dumped
  byte-exact (evidence/decompiled-quotes/s5-grading-extension-table-dump.md); the fix
  plan's step 3 is actionable (last-dot substring → ASCII-uppercase → exact set::find,
  with the `""` and dead-`Raw` quirks).
- **N-043 addendum:** the og's live "%1 file chunks scanned" milestone cadence cited
  (UIUX 06); fix decision unchanged.
---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-075 (renumbered from the session-6-local N-059 at integration) | MINOR | Event-bus shape: og query-signals (sigIsBusy(bool&) ×14, sigIsStopping ×8 — reference-out-param queries) + pull-polled progress convention + typed IUIProgressInfo vs core one-way push + telemetry pull. FIXED session 28: the two query exports (`cr_ui_scan_busy`/`cr_ui_scan_stopping`, synchronous) + D-77 registration + the ABI-CONTRACT 5 mapping — pinned by `CrUi.ScanBusyStoppingQueries` + plain-C edges |
| I-019 | INFO | DRWCfg.bin persistent config channel — scope register (host-owned settings; the scan-affecting subset is M-024's family) |

| M-044 UDF recognition | **MATCH** (decompile-exact probe, gate a + anchor + LE VDS load + "UDF"@0x19; no engine, carve rides) | `Orchestrator.UdfVolumeRecognized`, `Orchestrator.UdfNotConfusedWithFat`, `Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg`, `Orchestrator.UdfSurfacesAsCarveFamily` |
| N-076 FsFamily contract | **MATCH** (reserved-5 anchored; documented 6..11 numbering) | `Orchestrator.FsFamilyValuesMatchContract` |

| N-075 event-bus query signals | **MATCH** (busy/stopping query exports + D-77 pull-progress register + ABI-CONTRACT 5 mapping) | `CrUi.ScanBusyStoppingQueries`, plain-C edges in `cr_ui_abi_check` |
