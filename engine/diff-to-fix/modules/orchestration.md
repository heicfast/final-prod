# modules/orchestration.md — scan orchestrator audit verdicts

**S47 addendum (2026-10-29):** M-126 FIXED (`089b7cc8`) — the og's ETA model landed (FUN_1401af410's whole-run base + 0.2/0.8 rate EMA + the x0.6 time-budget variant + the '3m00s' placeholder + the 1 Hz countdown): the query-layer `eta_compute`/`eta_format_m_s`/`eta_countdown_tick` helper family beside the M-125 progress-display math, `ScanTelemetrySnapshot.eta_seconds`/`eta_smoothed_rate` computed live (the EMA state server-side), and the CLI's remaining-time line. O39's 'UI-side formatting' premise is superseded — the decode shows a computed model, and it is now a core surface. #38/#39 → MATCH.

## S12 addendum (2026-10-29 — the session-2 deep-dive — progress/ETA mathematics + state management)

New active findings on this module: **M-125** (the og's piecewise progress remap +
99.99% cap decoded via FUN_1401af820 — unregistered; the O38 "M-equivalent" verdict
superseded); **M-126** (the og's rate-EMA ETA model decoded — no core surface; the O39
premise superseded); **N-166** (live counters stage-boundary-only vs the og's ~2s
cadence — the :210-213 claim inaccurate); **N-167 FIXED (Session-44 —
ScanStats::bytes_found stamped from tree total_logical at every count refresh; the
session record grew to v3; the CLI renders the og's "(X GB)" shape; RED→GREEN
proven; findings/FIXED/N-167)** (no found-size aggregate);
premise superseded); **N-166 FIXED (session-46 lane)** (live counters: the emit-time
live-refresh landed — ResultTree's atomic attach counters [live_files/live_raw,
bumped in add_file] feed every emit_event_/emit_progress_event_ through
refresh_live_counts_, monotonically; deleted_found stays boundary-only [the
engines assign the deleted-state post-attach]; the header contract
re-truthed; the old :210-213-style "counters refreshing at progress ticks"
claim is NOW true and the STAGE-BOUNDARY admission is gone); **N-167** (no found-size aggregate);
**N-178** (the pause CLI legs); **I-041 (ACKNOWLEDGED — registered this lane)** (stopped-vs-finished: the og's verdict is a heuristic — FUN_1400f1b10 [DRWUI:205797-205813, the 0x62=98 threshold re-read first-hand this session] infers "stopped" from progress<98% at the advanced-end transition and persists/restores it [:314692-712, :837740-800]; the core derives it truthfully — stage-done bits stamp only on OK completion [cr_scan.cpp:2134-2137, the G-07 integrity comment] and resume skips only truthfully-completed stages [:2766-2770]. The explicit mapping — og states 3/7 = core cancelled-stage; og 4/8 = core stage-done bit — registered so parity tables stop looking for a "state 7" equivalent); **I-042 (ACKNOWLEDGED — registered this lane)** (paused observability: the og surfaces pause on four surfaces [enum 2/6, nStatus==2, the "Scanning Paused." strip, sigTaskBarPaused], the core exposes `ScanTelemetrySnapshot.paused` [cr_scan.h:520] + cr_ui.h:689 only, no dedicated `cr_ui_scan_paused` query — hosts render pause from telemetry; a library-appropriate subset, the taskbar a host concern); **I-043 (ACKNOWLEDGED — PARITY-DELTAS D-122)** (state ownership). Key decodes this session:
the og's 14-state scan enum (DRWUI:208243-296), the pause machinery (engine vtable
+0x88 notify + gate + GetTickCount compensation — the core's PausableToken is a
verified RE-parity), the stop-organize -> completion-handler chain, the 7-input
hang-trace table (no unregistered hang asymmetry).


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
- **O38 progress** — MATCH (M-125 FIXED, the S42 fix lane): the engine's weighted
  single-division permilles (per-stage monotonic, pause-compensated elapsed) stay the
  TRUTHFUL engine telemetry — the deliberate engine-side truth registered in
  PARITY-DELTAS D-118 — and the og's DISPLAY math now lands as the query-layer helper
  family (`query::progress_display_remap/_percent/_publish`, cr_query.h:88-119):
  FUN_1401af820's five-segment piecewise remap verbatim (raw<11 → ×2; 11..30 →
  (raw−11)·0.2+20; 30..90 → ×0.8; 90..98 → (raw−88)·2.7+72; >98 → identity; the
  defensive 99.99 stall cap — dead for raw in [0,100], the og's defensive branch
  ported as decoded), FUN_1401af950's type dispatch (0 → reset, 1 → remap+clamp, else
  raw), FUN_1401afa60's monotonic publish guard (the 98-boundary discontinuity: the
  2.7 segment tops at 99.0 and the identity tail DROPS to 98.5 — the guard exists to
  hide exactly that). Tests ProgressDisplay.* (5) + the engine's linear regression
  pin; og's 40→65% save formula and bottomarea semantics remain UI-session constructs
  (session module note); engine-level callbacks bridge with veto (progress-false →
  cancel) — matches the og's cancel cadence. The finding's own test table carried
  four transcription errors against the corpus constants (raw 20 → 21.8 not 22; 29 →
  23.6 not 23.8; 98 → 99.0 not 99.4; 99 → 99.0 not 99.99) — corrected per the corpus
  (the corpus wins).
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
  N-148 note (the passive-datum registration): the ReFS engine's
  og_prop_flags (the type-2/1/3 property-flag decode, cr_refs_engine.cpp:
  1180-1211) is a PASSIVE datum - tracked for og-parity documentation,
  not consumed by the core's grading model (the og's grading consumers
  of the flags are undecoded; wiring it in needs that decode first).

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
| N-075 (renumbered from the session-6-local N-059 at integration) | MINOR | Event-bus shape: og query-signals (sigIsBusy(bool&) ×14, sigIsStopping ×8 — reference-out-param queries) + pull-polled progress convention + typed IUIProgressInfo vs core one-way push + telemetry pull. FIXED session 28: the two query exports (`cr_ui_scan_busy`/`cr_ui_scan_stopping`, synchronous) + D-78 registration + the ABI-CONTRACT 5 mapping — pinned by `CrUi.ScanBusyStoppingQueries` + plain-C edges |
| I-019 | INFO | DRWCfg.bin persistent config channel — scope register (host-owned settings; the scan-affecting subset is M-024's family) |

| M-044 UDF recognition | **MATCH** (decompile-exact probe, gate a + anchor + LE VDS load + "UDF"@0x19; no engine, carve rides) | `Orchestrator.UdfVolumeRecognized`, `Orchestrator.UdfNotConfusedWithFat`, `Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg`, `Orchestrator.UdfSurfacesAsCarveFamily` |
| N-076 FsFamily contract | **MATCH** (reserved-5 anchored; documented 6..11 numbering) | `Orchestrator.FsFamilyValuesMatchContract` |

| N-075 event-bus query signals | **MATCH** (busy/stopping query exports + D-78 pull-progress register + ABI-CONTRACT 5 mapping) | `CrUi.ScanBusyStoppingQueries`, plain-C edges in `cr_ui_abi_check` |
---

## Session-27 row (DFX27 lane, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| M-058 | MAJOR (NEW-FINDING, fixed same session) | run()'s cache-failure fallback dereferenced the retry device CachedReader::create destroys on failure returns (latent UAF on the defensive path, ASan-proven in the standalone repro) + the adjacent comment documented the opposite (bare-slice) fallback. FIXED: build_read_pipe() extracted — the degraded path is a pinned contract (bare slice, no retry isolation, surfaced), 3 ReadPipe pin tests, N-045 budget pin re-anchored to the call site (lockstep preserved). |


## Session-33 verdict update (2026-10-26, DFX33 — M-018 FIXED)

**B1/D16 → MATCH (post-fix):** the og's PushFolderFilter junk filter is
now the core's — `filterfolder::apply()` runs post-walk (pre-grading,
pre-session-capture) marking the FilterFolder.ini hidden sets on the
nodes (NODE_FLAG_JUNK_FILTERED); query rows, the CLI tree and recover-all
planning all skip flagged nodes (sessions re-derive on resume). The lists
ship as generated data (scripts/gen_filterfolder.py from the corpus
config, byte-exact — the file's `._.DS_Store` and `新建資料夾` correct the
finding's typos); matching is exact u16 equality with ASCII-fold
(comparator mode undecoded in the corpus — registered D-82).
`ScanStats.junk_filtered` added. TxtViewFiles/SumaryFileType stay
out-of-scope (preview/count-unification surfaces, not the hidden set).

## Session-24 verdict update (M-039 FIXED, DFX24, 2026-10-26)

**Cross-engine occupancy feed — `M-039` FIXED.** The orchestrator
aggregates every STRUCTURED node's extents (post-engine-pass, from
the result tree) into a sorted, coalesced interval set and feeds it
to both carve dispatch layers as a skip gate: positions inside an
interval are suppressed (the grammar layer jumps to the interval
end); the raw-sign 512-strides suppress the same way; carved nodes
never feed the gate; the dedup tree stays the safety net.
`occupancy_skipped` telemetry on CarveStats/ScanStats. A28 (carve
target policy) / B29 (dedup, net) / O37 (cross-stage data flow)
verdicts: MATCH (the interval shape is the finding's own sanctioned
first step; the og's sparse-file mmap stays profiling-gated).


## Session-8 section (2026-10-28, audited `35adcb9`)

**M-073** (MAJOR — the scan-time folder filter absent: the og pushes the
AdvanceFilterCondition into the scan controller at start (PushFolderFilter,
UIUX/31:142, runtime-verified), constraining incremental tree insertion; the
core's ScanConfig has no filter input — post-hoc query only. Fix = a
`filter_folders` vector + post-parse pre-insertion prune + the ABI twin + the
CLI flag + session round-trip). The W1a/W1b/W1c orchestration claims verified
(load progress threading, cancel propagation, the completion auto-save leg).


## Session-8 wave-2 (audited `ff879f9` lineage, corpus `be0cfe9`; complements the deep-audit lane's rows)

- **S-044 (FIXED this session)** — the completion auto-save's gating chain
  (see modules/session.md): results-found, not-already-saved AND
  not-import gates all landed (the resume path suppresses via the
  is_resumed flag, as this row suggested); the og's singleton byte-flag
  enable surface is the registered delta D-105.
- Progressive result emission dispositioned MATCH: the core's carve
  stage attaches nodes live (carve_slice streams into the tree during
  the pass, cr_scan.cpp:1877-88) with counters refreshing at progress
  ticks — the og's live tree growth (UIUX/30:14) has its equivalent.
- **N-106 (MINOR)** — GetDriverInfo.dll (the TB-lineage drive-letter
  topology MOM module) dispositioned: OUT-OF-SCOPE by lineage; the
  og's recovery-flow enumeration remains the N-071 census (WMI/
  setupapi/VDS/StorageSpaces/FunctionDiscovery).


## Session-8 lead-lane wave (audited `ff879f9`)
- **N-125**: the drive-disconnect mid-scan states (the "Drive Disconnected" Retry/Cancel
  flow, UIUX/08:39-47) — no classification, no reattach-continue model, no registration;
  from R-16, never formalized through every prior session.

## Session-10 wave-2 addendum (2026-10-29, continuation lane — audited `84a1832`; EXECUTED evidence)

- **S-051 (SEVERE — FIXED at `6ecd6bb4`, session 40)** — the whole-disk scan target now consults the
  partition list: `cr_ui_open_disk` + scan (the CLI `scan --disk N` without
  `--whole-disk`) probes LBA 0 (the MBR), dispatches Unknown, and returns
  rc=0 with ZERO files on a healthy multi-volume disk. The og's
  CDoScanPartition flow reads the partition list and scans every known
  volume (architecture/04 §2/§3.4; the real G session carries both volumes'
  trees). EXECUTED: evidence/executions/s10-wholedisk-scan-targets.md RUN 2.
- **M-104 (MAJOR)** — with `--whole-disk`, the in-table volumes are found
  only via the discovery's +500 table-match candidates, surfacing as
  anonymous `"Candidate @ <LBA>"` roots: no display names, no FS-type
  markers, known-vs-lost indistinguishable, Found/Deleted roots empty. The
  og's real-session per-volume display-name shape (`Local Disk(G:)` +
  `System Reserved (NTFS)` + the `NTFS` marker) is unreachable. EXECUTED:
  the same evidence file RUN 1 + the first-hand G-session depth-1 census.
- The two findings compose with S-048/M-103 (the per-volume branch family)
  and M-079 (the volume-label plumbing): one whole-disk shape pass in the
  fix order resolves the family coherently.

## Session-11D fix lane (2026-10-29 — lane D; S-051 + M-104 FIXED, this session)

**DU-merge note:** the S40 lane's block above documents THEIR convergent
landing (`6ecd6bb4`); the merged tree keeps the union — their host-hint
gate + stats names, this lane's display names/skip accounting/scanned-only
reconciliation. See S-051's Resolution addendum in `findings/FIXED/`.

- **S-051 → FIXED** (`run_engine_pass`'s partition-list target block): a
  whole-disk-basis scan with an unforced hint now parses the partition
  table and makes every used, non-container, non-LDM entry a scan target —
  per-target `probe_family`, `scan_candidate` under the volume's own root,
  F-1 subtree rebase, honest skips counted in `table_volume_skipped` (rc
  stays OK), a Warn/Info log line per target (the og's insertion-trace
  analog). The discovery reconciliation (run_deep) drops candidates whose
  start LBA matches an already-scanned target — the no-double-scan
  acceptance; a target skipped for a dead probe stays discoverable (the
  og's lost partitions are additional targets, never replacements). The
  superfloppy and forced-hint paths are pinned unchanged (tests).
- **M-104 → FIXED** (rides the same builder): the volume roots carry
  display names — tier 1 the FAT-family boot-sector label
  (`fat::read_volume_label`, validate_boot-gated; NTFS/ext4/exFAT label
  decodes compose via N-133's landing), tier 2 the `NTFS @ <LBA>`
  FS-marker, tier 3 the anonymous LBA form now reserved for genuinely-lost
  discovery candidates. One tree per volume (deleted as a node flag, the
  og's shape); the names round-trip through the session format.
- **A-012 (ACUTE, discovered during the S-051 verification)** — see
  modules/carve.md's row and the finding file: the 81ee9775 rebase-window
  sweep dropped M-093's FMT_SVG registry row, hard-downing the carve stage
  on main; restored this session (`RegistryF8Gate` RED→GREEN).
- Residuals recorded in the Resolutions: the og's FS-type marker ROW
  (corpus-ambiguous per the finding's own open question) and the
  standard-root narrowing stay the S-048/M-103 wave's scope.
### Session-11-lead rows (2026-10-29)

- **M-114** (MAJOR): MainModule's silent System-Restore purge (srclient SRRemoveRestorePoint via WMI enumerate-all, 510-index fallback, BOTH drives of a copy/repartition pair) — absent, undecided, undocumented even in the corpus. The posture decision (host-consent plan item vs registered decline) is the fix lane's.
- **N-145** (MINOR): the cross-run apply-interruption marker (the og's pendingMarkedDisk; core's CRUNDO01-only posture) — the in-flight journal recommendation.
- **N-148 item 6** (MINOR): the ReFS og_prop_flags decoded-but-unconsumed datum (the consumer decode or the passive-datum registration).

## S42 continuation lane (2026-10-29 — the scan-mode semantics focus battery; core `b26017a7`)

- **O31/O38 (the auto-advance state machine) — REGISTERED (I-044, ACKNOWLEDGED — landed `f0163182`)**: decoded engine-side
  (CDoScanPartition::DoScan pushes state 1 unconditionally, DRWUI:567875-892; the
  ScanThread pops states with telemetry events 0-3; controllers 0/1/5/4 = quick/
  advanced/recycle-bin; the recoverability grade only selects coaching text) — the
  C1 disposition's "UI-layer state machine" rationale is corrected; the core's
  default-on stages are confirmed as the correct parity translation. Also
  registered: settings+0x60/+0x6c are hardwired constants (:947721-22), the
  4 MiB all-allocated chunk skip (FUN_140585150), the FUN_140581500-vs-FUN_140580720
  citation erratum, and ZFS compiled out of the shipped og (version.ini scanzfs absent). **The register's doc-side fix plan landed at `f0163182`** (C1/A1/A5 annotations + D-36 citation correction + the CORPUS-ERRATA row); the citation erratum's corpus-side twin is recorded for the corpus doc's own errata list.
- **O37 (stage order) — DIVERGENCE-MINOR, N-181**: the og starts the RAW carve
  controllers before the FS deep engines (:916484-916509); the core's carve stage
  is strictly last — time-to-first-carved-result diverges, result sets identical.
- **O31/A2 (the deep range) — DIVERGENCE-MINOR, N-182**: the og's deep-only resolver
  (FUN_14035a6d0: device-type-6 sub-ranges, the 300 MB small-volume rule,
  neighbor-entry extension); the core reuses the quick slice — D-36's "same
  StartSector" is the common case, not an invariant.
- **Grading cells (the recovery-chances battery)**: S-058 (SEVERE — the extension-
  table gate polarity inverted; the s5 erratum), S-061 (SEVERE — ReFS/Btrfs
  insert-time High absent), M-138 — FIXED (session-52: the twin-key device family narrowed to the og's {FAT16} shift, the ext2/3 leg registered OQ-S52-1; renumbered M-133→M-138 at the N-160 doc-truth repair), M-134 — FIXED (session-52: the deep-read grade + the one-shot full-tree re-grade landed; suite 1379/1379; the pre-check trio stays OQ-S42-3) (the deep-read
  validation + full-tree re-grade), N-183 (the gauge population/score/QSDE batch).
  Worksheet: `../evidence/decompiled-quotes/s42-grading-gate-polarity.md`.


## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

New active findings touching this module: **M-146** (probe_family's NTFS gate
family mismatch — the dispatch layer's gate set re-derived from the analyser),
**N-199** (the ReFS second signature FSRS@+0x10 absent in probe_family),
**N-194** (the size display formatters absent — the query surface). Verified-MATCH
this session (the state-management axis, the user's focus battery): the scan
lifecycle states and every transition writer re-walked against cr_scan's
single-owner model (the I-041/I-042/I-043/D-122 registers corroborated — no new
state divergence), the watchdog's bounded teardown, the CUIScanProgressInfo
monotonicity protocol, the telemetry clamp family (the 99→100 clamp kept distinct
from the 98% classifier and the 99.99 display cap — three different constants,
now each pinned), and the progress/stage weighting. The pause type-gate and the
"Reading sector" ticker are recorded as precise next probes in
`evidence/s54-deepdive-orchestration.md` (UNRESOLVED #1/#2).
## Session-46 addendum (2026-10-29 — the MINOR-first fix lane)

**N-166 FIXED — the live file-count telemetry.** ResultTree's atomic attach
counters (live_files/live_raw, bumped in add_file under its own lock) +
refresh_live_counts_ at every emit_event_/emit_progress_event_ (monotonic;
the boundary stamps stay the authoritative close-of-stage sample).
deleted_found deliberately boundary-only: the engines assign the
deleted-state AFTER the attach — the finding's "already atomics" premise
corrected in the Resolution. The header contract re-truthed (cr_scan.h).
TelemetryCountersMoveDuringCarveStage RED pre-fix (the frozen mid-stage
counts), GREEN post; suite 1312.
- **Grading cells (the recovery-chances battery)**: S-058 **FIXED** (the
  extension-table gate polarity righted — the S52 winning landing ea405392 with
  the S57/S56 convergence records; this lane's in-place s5 erratum + the
  CORPUS-ERRATA row ride the S56 commit), S-061 **FIXED** (the S52 lane's
  same-commit ReFS/Btrfs insert-time High), M-138 — FIXED (session-52: the
  twin-key device family narrowed to the og's {FAT16} shift, the ext2/3 leg
  registered OQ-S52-1; renumbered M-133→M-138 at the N-160 doc-truth repair),
  M-134 (the deep-read  validation + full-tree re-grade), N-183 (the gauge population/score/QSDE batch).