# REPRODUCTION — the evidence infrastructure for the sector-scan rebuild (Agent E, read-only survey)

> Scope: what EXISTS today to reproduce/detect the mandate symptoms on real
> Windows runners, what each tool captures, where evidence lands, and what is
> MISSING. Every claim carries a file:line citation. Survey date 2026-11-18,
> HEAD `9352c81d` (session prompt records start `976bced8`).

---

## 1. Forensic fixtures and the CI vehicles

### 1.1 The seeded VHDX corpus — `tools/forensic/seed-disks.ps1`

- Creates two dynamic VHDXs via diskpart: **disk1 = 64 GB GPT, one NTFS
  partition X: ("SeedNTFS")** and **disk2 = 8 GB MBR, one exFAT partition Y:
  ("SeedEXFAT")** (`seed-disks.ps1:117-119`, param defaults `:16-18`).
- Payloads carry **real signature bytes** (JFIF SOI/APP0/EOI `:30-39`, PNG
  IHDR/IEND `:41-52`, PDF head/`%%EOF` `:54-64`, MP4 ftyp `:66-75`, real
  zips via Compress-Archive `:77-86`) "so deep-scan carving has true
  positives to find; NTFS metadata stays intact for the quick-scan pass"
  (`:10-12`).
- Corpus, counted from the seed loops (`:173-218`): **disk1 = 178 files**
  (48 Photos jpegs `:173-175`, 12 Summer2024 pngs all deleted `:176-178`,
  10 Winter2025 jpegs `:179-181`, 6 videos incl. a 16,384 KB clip `:183-188`,
  40 Documents notes `:190-192`, 20 Reports pdfs `:193-195`, 10 Archives
  zips `:197-199`, 30 Work txts `:201-203`, plus a 32,768 KB
  `BIG_deleted_video.mp4` and `presentation.pdf` `:204-205`); **disk2 = 60
  files** (20 Docs `:210-212`, 25 Media jpegs `:213-215`, 15 Misc zips
  `:216-218`). **Deleted: 53 on disk1 + 8 on disk2 = 61 of 238** (delete
  flags at `:174,177,184-187,191,194,198,202,204,211,214,217`; deletions
  performed `:222-226`). The header's "~190 / ~60" (`:4-6`) is approximate;
  the loop math is the truth.
- Outputs: **`seed-manifest.json`** (every file: path, sizeKB, category,
  kind, deleted flag; totals block `:231-242`) and **`disk-map.json`**
  (vhdx → disk number/style/fs/letter `:123-139`). The manifest is the
  ground truth any detection-rate gate must be measured against — **today
  nothing consumes it as a gate** (it is only uploaded,
  `forensic-repro.yml:314`).

### 1.2 The probes — `tools/forensic/`

| Script | Captures | Output | Citation |
|---|---|---|---|
| `install-probe.ps1` | every child process spawn (watchlist powershell/pwsh/cmd/fsutil/certutil/msiexec/setup*.tmp) + every visible top-level window, sampled at 400 ms during install; installer exit code + duration | `install-process-events.jsonl`, `install-window-events.jsonl`, `install-result.json` | `install-probe.ps1:61-63, 65, 75-96, 109-119` |
| `collect-facts.ps1` | OS, physical disks, volumes, shell elevation + integrity level, the installed exe's manifest classification (requireAdministrator regex over the binary), install-dir file census, HKLM product/uninstall keys, System event log | `facts-<preinstall\|postinstall\|postrun\|postuninstall>.json` | `collect-facts.ps1:21-41` |
| `launch-app.ps1` | launches the INSTALLED exe with `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=9222`, waits for the CDP endpoint, probes the launched process's real token elevation via OpenProcessToken/GetTokenInformation | `app-pid.txt`, `app-stderr.log`, `app-stdout.log`, `cdp-ready.json`, `app-token-elevation.json` | `launch-app.ps1:19-25, 29-48, 55-87` |
| `io-probe.ps1` | the app process's IO counters (GetProcessIoCounters P/Invoke: read/write/other ops + bytes), handle count, working set, and all CHILD processes (the fsutil `_popen` flash evidence), every 2 s (default) for up to 2400 s (launched with 5400 s by the workflow) | `io-timeline.jsonl` | `io-probe.ps1:8-13, 20-49, 54-70`; workflow launch `forensic-repro.yml:152` |
| `drive-ui.mjs` (903 lines) | drives the real shipped UI over CDP: boot DOM + screenshots; the **no-mock gate** (`__TAURI_INTERNALS__` present, `__rwInjectConsent` absent `:123-133`); S2 per-drive health DOM; **S2b save/resume vehicle** on the 8 GB exFAT seed (stop mid-deep, during-drain save toast gate `:309-335`, 15-min drain-banner gate `:348-363`, session-save toast gate `:371-409`, Backup Resume-affordance gate `:421-426`, live-scan route gate `:441-468`, resumed-completion gate 22-min cap `:482-511`); **S3 scan recorder** on the 64 GB NTFS seed — 15-min timeline of {pcts, stage, sectorMatches, foundCounts, foundSize, complete} sampled every 5 s `:589-650` + the **D-SCAN-01 gates** `:655-705`; S3b recover gate; S4 other surfaces; console + pageerror streams | `evidence.json`, `console.jsonl`, `pageerror.jsonl`, `screenshots/*.png` | header `:1-29`, `extractScanState` `:75-86` |
| `unelevated-launch.ps1` | the OS-refusal twin: an unelevated CreateProcessWithLogonW start must be REFUSED by the OS once the manifest carries requireAdministrator | `unelevated.json` (+user file) | consumed at `forensic-repro.yml:174-196` |

**D-SCAN-01 gate math** (`drive-ui.mjs:655-705`), asserted over the
advanced-phase samples of the 64 GB scan: (a) **no fully-frozen 180 s
stretch** — percent + foundCounts + sectorMatches all identical for 36
consecutive 5 s samples → red (`:669-697`); (b) **the advanced phase must
advance ≥ 1.0 percentage point** across the capture (`:699-701`); (c) **live
found-size non-zero** — once files are found, `foundSize` must not read
"0 B" (`:686-704`). S3's stall itself is a RECORDER, not a gate (`:583-588`).

### 1.3 The workflows

- **`forensic-repro.yml`** — `workflow_dispatch` only, input `release_tag`
  (default **v0.9.3**, `:44-50`), runner **windows-2022**, 90-min timeout
  (`:58-59`). Steps: download the shipped setup.exe + SHA256SUMS verify
  (`:76-91`); facts preinstall; **seed disks** (`:97-99`); install with full
  process/window capture S1 (`:101-103`); facts postinstall; **manifest
  gate** — `appManifest` must equal `requireAdministrator` (D-PRIV-01,
  `:109-123`); **disk census** `disk-census.json` = Get-Disk ground truth
  (`:125-135`); ONE step for app + io-probe + `drive-ui.mjs` (the runner's
  job object kills step-spawned processes, `:137-172`); unelevated twin
  (`:174-196`); app log + sessions snapshot (`:198-217`); facts postrun;
  silent uninstall + no-leftover assertions (`:223-303`); **artifact
  `forensic-repro-evidence`** = `C:\forensic\evidence\**` + ProgramData logs
  + twin-data + `seed-manifest.json` + `disk-map.json`, 30-day retention
  (`:305-317`).
- **`validation-evidence.yml`** — dispatch-only, windows-2022. Job **t14**:
  builds the engine kit, records elevation facts, compiles
  `tools/validation/measure-cr-ui.c` against the staged kit, runs
  `--quick-only` → `t14-engine.jsonl`; runs the `#[ignore]`d bridge T14
  tests (G-7 filter cost + NodeMeta memory) → `t14-bridge.jsonl`; uploads
  artifact `t14-measurements` (`:78-152`). Job **t15**: full build +
  `installer-matrix.ps1` (install/update/repair/uninstall rows, throws on
  any failing row) → artifacts `t15-runtime-matrix` + `t15-installer`
  (`:154-210`).
- **`benchmark-src.yml`** — dispatch-only (inputs profile smoke/full,
  scan_modes, reason, run_tests), windows-2022, **300-min** timeout,
  concurrency-queued (`:44-53`). Builds the checked-out commit with the
  release recipe (`build.ps1 -Target engine`), compiles the API harness
  (`engine/.github/scripts/api_harness.c`), optional full ctest correctness
  gate before measuring (`:79-84`), records `runner-context.json`, runs the
  **scenario matrix** via `Run-Benchmark.ps1` (seeded-VHD matrix,
  `-ScanCapSec 1800 -RecoverCapSec 1800`, `:104-130`) plus the **alignbench
  sweep** (read-size × QD to 16 MiB × QD32 on `\\.\C:`,
  `engine/bench/ours/run-alignbench.ps1`, `:132-148`); artifact
  `benchmark-src-evidence-<profile>-<run_id>`, **90-day** retention
  (`:150-158`). Comparability contract: same runner class + matrix as
  recorded baselines runs 41140212296 / 42012118585 (`:14-18`).

---

## 2. Existing evidence — what was actually captured vs what is empty

### 2.1 `worklog/REPRODUCTION/` (the D-* defect files)

| Dir | Files | Substance |
|---|---|---|
| D-SCAN-01 | `README.md`, `scan-timeline-extract.json` | The reproduced stall. Run **42255492260** (shipped **v0.8.4**), 172 samples: first sample at elapsed 0 s already `pcts:[75,75] stage:advanced` (the exact quick-40 + deep-35 carve boundary, `cr_scan.h:72-75` weights); at +891 s the percent reads **76.1 — 1.1% in 15 min**; `foundCounts:["197 items"]` frozen first→last; `sectorMatches:[]` every sample; `complete:false` throughout (`scan-timeline-extract.json:4-49`). README: `dataFoundBytes` pinned 0 for the whole live scan (no ABI field; bridge latches terminal-only, `scan.rs:416-443`); `bytesScanned` is carve-true-bytes only, zeroed for stages 0/1 (`cr_scan.cpp:596-597`) (`README.md:18-39`). Fix status: D4a live bytes, D4b bytes_found ABI, D4c intra-window carve checkpoints, D4d verification (`README.md:51-59`). |
| D-CONS-01 | `README.md` only | Console-flash mechanism: `cr_health.cpp:306` `_popen("fsutil …")` in the GUI-subsystem product process, reached on EVERY health read (`cr_ui.cpp:4948`); honest caveat — the 2 s child census cannot catch a ms-scale flash; fix = direct CreateProcessW + tree-wide build-time no-`_popen` guard (`README.md:14-50`). |
| D-ENUM-01 | `README.md`, `home-dom-extract.json` | The Home filter defect: pre-fix Home rendered `kind==='internal'` only (`home-page.tsx:80-83`) while the bridge degrades USB/SD/MMC bus to `kind:'usb'` (`devices.rs:215`). Runner evidence `home-dom-extract.json:2-19` lists all four runner disks (Windows C:, Temporary Storage D:, SeedNTFS X: 63.85 GB, SeedEXFAT Y: 7.98 GB) — all Azure-internal-bus, so the runner proves enumeration, not the user's USB/SD mix; the filter is proven by the unit pins (`README.md:23-37`). Fix landed (commit c6ff7c10). |
| D-PRIV-01 | `README.md`, `facts-postinstall-appmanifest.json`, `app-token-elevation.json`, `unelevated.json` | The v0.8.4 binary carried **no manifest** (`facts-postinstall-appmanifest.json:2` `"no-manifest-match"`); the probe pre-elevated the launch (`app-token-elevation.json:3` `"elevated"`); the v0.8.4-era unelevated twin showed the in-app restart-as-admin banner + scan refusal (`unelevated.json:8-14`). Fix: `ui/src-tauri/app.manifest` requireAdministrator; the v0.9.x gates assert it (`README.md:39-47`). |

**The raw artifacts (io-timeline.jsonl, full evidence.json, screenshots) are
NOT in the repo** — they live in the GitHub run artifacts (30-day retention,
`forensic-repro.yml:317`). The repo carries curated extracts + run IDs.

### 2.2 The stall/drain captures cited by `worklog/BEHAVIOR-FINDINGS.md`

- **BF-22** (stall; `BEHAVIOR-FINDINGS.md:36`): reproduced run 42255492260
  (75→76.1% over 891 s, found frozen at 197, sectorMatches empty) AND on
  the user's real machine. Marked **FIXED** (commit ce71143e: D4a/D4b/D4c);
  the carve's absolute throughput (the **8.45 MiB/s** effective validation
  wall vs 544 MiB/s device, 76-124 s/GiB) remains the measured wall (BF-30,
  `:55`).
- **BF-24** (`:38`): post-cancel drain 20+ s, io-timeline evidence run
  **42143819243**.
- **BF-25** (`:39`): run **42148493806** (v0.7.0): 64 GB scan stalled at
  76.1% with reads idle from +6 min; cancel at +28 min; **15 save attempts
  over 136 s all refused**; no write spike (the .crsf never written); the
  "stopped early" banner never rendered; recovery itself succeeded.
  GENERALIZED by run **42176600928** (v0.8.1): a stop ~2 min into a
  HEALTHY 8 GB exFAT scan (0.6% into carve — the exFAT deep stage is nearly
  empty) sat un-drained 180+ s with reads ceased ~90 s after the cancel;
  the matcher grinds ~9-10 min in memory. Measured baselines: fresh 8 GB
  scan **11.8 min** (run 42170466748), resumed **15.6 min** (42201790752)
  and **16.03 min** (42212963611). D4c's bounded cancel checkpoint landed
  (ce71143e); the drain gate stays as regression proof (`:45`).
- **BF-29** (`:54`): the "Searching: \<path\>" ticker is dead on the real
  backend — the bridge's progress payload carries `currentPath: null`
  always; the **mock DOES simulate one** (`mock-connector.ts:1152`
  `rt.currentPath = … searchingPathAt(rt.tick, …)`), the test-double
  divergence that masked the gap.
- The **v0.9.0 forensic run 42278088695**
  (`worklog/RELEASE-VALIDATION/RELEASE-RUN.md:501-529`): manifest gate
  GREEN, home census GREEN ("4 OS disks, 0 missing"), **live found-size
  GREEN** ("Files Found: N (196.5 MB)" live from t=0 — the "0 B" defect
  gone), smooth-carve axes GREEN (75.0→75.1→…→76.1, ~0.1 pt / 21 s), S2b
  vehicle GREEN — but **RED on a 655 s fully-frozen stretch at 76.1**:
  D4e root cause, the deep remnant sweep (`scan_mft_remnants`) ticked only
  per completed 16 GiB chunk; fixed in 62fb7670 (4 MiB block-level ticks).
  v0.9.1+ carries the fix; the workflow's current default tag is v0.9.3
  (`forensic-repro.yml:50`).

### 2.3 `worklog/VALIDATION/` + `docs/INTEGRATION-VALIDATION/`

- `worklog/VALIDATION/it-gates/` — the committed engine/bridge integration
  captures (source: **bridge-integration run 44244567592**, HEAD be3b8256;
  fixture = a **6 GiB seeded NTFS VHDX** — 120 live + 10 categorized live
  incl. real JPEGs + a genuine 43-entry zip + 80 deleted `del-*.bin` + one
  3 GiB bulk — plus a 512 MiB raw VHDX; `README.md:3-8`):
  - `scan-timeline.csv` — **ONE terminal sample only**: `overall_pm=750`
    (the global quick|deep cap — "the exact 75% behind the user's report"),
    `phase=1, phase_permille=1000`, quick+deep completed,
    `bytes_found=3,241,140,501` (~3.02 GiB), **305 files**, 0 errors
    (`README.md:14-18`). No per-second series.
  - `scan-events.json` — engine event stream counts for that run:
    `files_found_events: 3, heartbeat_events: 0, progress_events: 5,
    samples: 1`.
  - `deleted-tree.txt` — **81/81 deleted rows** recovered under
    reconstructed parents `Basic data partition/./gone/` (80 del-*.bin +
    the 3 GiB bulk), plus category/size/manifest-total gate notes
    (`README.md:19-25`).
  - `health.json` (truthful no-source SMART refusals on all 4 virtual
    disks), `enumerate.json` (4 disks), `preview.txt` (archive node, 43
    entries).
  - NOT in this dir: `abi-layouts.json` is described but absent (it lives
    in the run artifact; `README.md:41-42`).
- `docs/INTEGRATION-VALIDATION/t14-measurements/` — committed: run
  **40442056802** (2026-10-27, windows-2022, EPYC 9V74 4 vCPU 16 GB):
  quick-stage scan of the runner's own 274 GB / 1.9 M-file system volume
  **72.1 s** (≈ **26-41k files/s**), enumerate p50 7.9 ms / p95 130.9 ms,
  startScan prep 5.5 ms, run-entry→first-event 12.5 ms, G-7 filter
  3.6-4.2 ns/node (wide) and 117-124 ns/node (scoped+search) at 10^6,
  NodeMeta **212-213 B/node** heap (`t14-measurements.md:46-76`). Honest
  gaps recorded: `bytes_scanned` stays 0 across the quick stage by engine
  semantics; deep+carve throughput not measured by this leg (`:89-97`).
- `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/` — committed installer
  matrix rows (baseline/install/reinstall/repair/uninstall facts + JSON).
- `worklog/RELEASE-VALIDATION/` — RELEASE-RUN.md (release + forensic
  post-mortems per tag), BUILDS/CAPABILITY-MAP/CORE-MERGE/DRIVER-REMOVAL/
  UI-WIRING.
- `engine/worklog/` — PERFORMANCE.md (the 2026-11-18 optimization ledger:
  CachedReader sharding numbers, **fix 3 carve first-byte dispatch table
  161 s → 29 s (5.55x) on the 315-test Carve suite**; the runner-class
  deepcarve wall baseline **1,040,054 ms @ v0.7.0** rides benchmark-src),
  BENCHMARK.md (the harness contract), PERFORMANCE-TARGET.md, BASELINE.md,
  PERF-LEVERS.md. No raw per-run JSONL committed — numbers cite run IDs.
- **Empty today**: `worklog/VALIDATION/` has ONLY `it-gates/` — the
  streaming-detection session created the dir "intentionally EMPTY — no
  evidence was captured this session and none was invented" and its gates
  1-8 "are therefore NOT proven yet" (`BEHAVIOR-FINDINGS.md:201-204`).

### 2.4 What the CURRENT product's scan actually does on the seeded fixtures

Measured, cited above: on the 64 GB NTFS seed, the UI opens at exactly
**75.0%** (quick 40 + deep 35 complete in seconds on a healthy volume —
`D-SCAN-01/README.md:20-24`); the pre-fix carve ground at ~1.1 pt / 15 min
with the counter frozen at 197 items and the sector ticker empty (run
42255492260); post-D4a-d (v0.9.0) the percent moves ~0.1 pt / 21 s through
the free-map region and found-size renders live, but the remnant-sweep
freeze (655 s) kept the frozen gate RED until 62fb7670. On the 8 GB exFAT
seed: fresh scan **11.8 min** end-to-end, resume **15.6-16.03 min**, stop
drain bounded by the matcher grind (~9-10 min) with reads ceasing ~90 s
after cancel (runs 42170466748 / 42201790752 / 42212963611 / 42176600928,
`drive-ui.mjs:347-348, 476-479`). Quick-stage throughput on a real 274 GB
volume ≈ 26-41k files/s (T14). Carve validation wall ≈ **8.45 MiB/s**
effective (BF-30). Detection vs manifest: the 6 GiB it-gate fixture proves
**81/81 deleted + 305 files / bytes_found 3.02 GiB**, but on the FORENSIC
64 GB/8 GB corpus **no capture compares result counts to the
seed-manifest.json totals** — the drive-ui S3 result capture records
treeitems/rows/bodyText (`drive-ui.mjs:707-724`) with no manifest gate.

---

## 3. The benchmark harness — `engine/bench/`

- Layout and contract (`engine/bench/README.md`): `seed/generate_seed.ps1`
  (deterministic 23-format corpus: CJK names, long paths, ADS, hard links,
  sparse, fragmented, nested, many-small, keep-control), `volume/volume.ps1`
  (VHDX create/format/hash-map capture), `scenarios/apply-scenario.ps1`
  (deletion matrix: recycle_bin, permanent_delete, quick/full format,
  delete_partition, decay, partial_overwrite, mft_corrupt),
  `ours/run-ours.ps1` (quick/deep/deepcarve scans + tree + recover +
  process sampling), `ours/run-behavior.ps1` (unelevated-refusal, cancel
  responsiveness, session save→kill→resume), `og/` (the EaseUS DRW
  comparison driver), `metrics/` (recall/precision/F1, hash-match,
  regression gate).
- The matrix VHDX set: **small-fat32 2 GiB; multi-gpt 5 GiB (CRNTFS p2 +
  CREXFAT p3); large-ntfs 8 GiB (CRBIG)**, threads 4, mask 7
  (`engine/worklog/BENCHMARK.md` §2).
- **Gates** — `engine/bench/baseline.json` `perf.rows` (source run
  41042267586): bench-seq-1m ≥ 536.92 MiB/s (±3%); **bench-rand-1m ≤
  1993.22 µs (±10%)**; **bench-open ≤ 17,019 ns (±15%)**;
  scan-quick-wall ≤ 150 ms; **scan-deep-wall ≤ 16,505 ms (±10%)**;
  scan-deepcarve-wall ≤ 1,501,622 ms (±3%); api-open ≤ 0.9 ms;
  api-scan-run ≤ 1,513,221.7 ms; **api-save-session ≤ 10.8 ms (±10%)**;
  **api-recovery ≤ 49.2 ms (±10%)**. Scenario recall floors live in the
  (currently empty) `scenarios` block; the S-DEL-NTFS hash-recall floor
  **0.35** is cited at `worklog.md:3907-3912`.
- **Current pass/fail state**: the pre-2026-11-18 re-measure (run
  42012118585) evaluated 10 driver-parity rows — **9 PASS, 1 FAIL**
  (quick-wall sampling artifact) (`engine/worklog/PERFORMANCE.md:185`).
  The first full matrix through the 2026-11-18 ring rework —
  **benchmark-src run 44275941738** at `a2561acd` — failed **SEVEN hard
  gates** (`worklog.md:3907-3913`, `worklog/COORDINATION.md:228,256`):
  cancel+resume `resumedFiles=0`; **S-DEL-NTFS hash-recall 0.272 < 0.35
  floor**; bench-rand-1m 9,427 ms > 2,192 cap; bench-open 227,869 ms >
  19,571; **scan-deep-wall 26,253 ms > 18,155**; api-save-session 13.7 >
  11.88; **api-recovery 5,524.5 ms > 54.12 cap (102x)**. Root cause pinned
  from source: `AsyncRing::pick_victim_locked()` never returns INFLIGHT →
  out-of-window reads returned ERR_NO_MEMORY (misclassified hard by
  `cr_retry.cpp:49`) and the 4 remaining misses caused ~1024x read
  amplification; the engine-ci ring tests' 24-48 MiB fixtures fit inside
  the 32 MiB window so the miss path never fired (`worklog.md:3915-3931`).
  The repair (companion sync handle) is pushed with regression pins; **the
  Windows matrix re-run is dispatched but NOT yet green** (`worklog.md:3956`,
  15:46 dispatch also red, `worklog.md:3969-3977`). Evidence lands in
  artifact `benchmark-src-evidence-<profile>-44275941738` (90-day
  retention) — not committed to the repo.
- `tools/validation/measure-cr-ui.c` — the T14 harness: measures
  enumerate (listDevices), health (getHealth), scan (startScan + progress
  stream: time-to-first-event, run wall, telemetry throughput, files
  found — quick-stage of the system volume via `--quick-only`),
  snapshot/paged row reads (queryResults), preview (open+64 KiB
  read+metadata), and PeakWorkingSet memory checkpoints; includes the
  PINNED `engine/include/cr/cr_ui.h` (no struct transcription); streams
  one JSON line per phase; exit 2 on a hard phase failure
  (`measure-cr-ui.c:1-43, 141, 217, 227-246`).

---

## 4. E2E / unit coverage

- **`engine/tests/`** — one cross-platform GoogleTest suite (`cr_tests`,
  Linux AND Windows; `CMakeLists.txt:1-3`) with ~70 unit files (carve,
  cached, transport-logic, orchestrator, grading, node-tree, pause-resume,
  query, session, …). The Windows-only layers (real transport/ring,
  enumeration, privilege) are `WIN32`-gated executables
  (`CMakeLists.txt:109, 179, 202` — e.g. `test_transport_win.cpp` with the
  new 128 MiB random-miss and 96 MiB walker pins, `worklog.md:3950-3955`);
  Linux uses `tests/win-stub/` SDK stubs with a structural-corruption
  gate (`CMakeLists.txt:240-245`). Known blind spot: ring fixtures
  24-48 MiB fit inside the 32 MiB window — the miss path was untested
  until the 2026-11-18 pins (`worklog.md:3930-3931`).
- **`ui/e2e/`** — 4 specs: `smoke.spec.ts` (golden path scan → results →
  recover; quick-target direct scans; `:26-117`), `consent.spec.ts`,
  `lane-c-surfaces.spec.ts`, `responsive.spec.ts`. **All mock-pinned by
  design**: ui-e2e.yml builds with `VITE_CONNECTOR=mock` and states "the
  real connector needs the [shipped] binary" (`.github/workflows/ui-e2e.yml:5-6,
  47-50`). Real-UI coverage of scan-progress/results-explorer exists only
  via the forensic probe (drive-ui S3/S3b) on dispatched runs.
- **`ui/src/connector/mock/`** — a fully typed deterministic simulator:
  progress ticks 100-250 ms, cancel within 300 ms
  (`mock-connector.ts:11`); it **simulates `currentPath`**
  (`mock-connector.ts:1149-1155`) while the real backend always carries
  `currentPath: null` (BF-29, `BEHAVIOR-FINDINGS.md:54`) — the test-double
  divergence; it batches results where "the real engine streams at disk
  speed" (`mock-connector.ts:1725, 1909`). The run-20 lesson is on
  record: the mock's synchronous cancel (no 'stopping' phase) kept every
  unit/e2e proof green while the shipped product never rendered the
  stopped-scan banner (`worklog.md:1975-1990`).
- **UI data path**: `ui/src/queries/scan.ts:32-60` — progress events
  invalidate tree/results/count queries only at **10%-bucket cadence**
  (`bucket > lastBucket`, `scan.ts:41-47`); `useScanState` polls 500 ms.
  The streaming session's DIAGNOSIS: **0 of 12 `CR_EVT_*` types carry a
  row; the UI starves between 10%-bucket snapshot pulls** — "detection and
  persistence were measured WORKING" while the drop stage is the
  engine→UI event surface (`BEHAVIOR-FINDINGS.md:176-179`;
  `worklog/DIAGNOSIS.md:25,30`; no row-carrying event exists,
  `cr_ui.h:151-297`, per `session-sector-scan-rebuild/PRODUCT-PIPELINE.md:196,208`).

---

## 5. Gap analysis vs the session mandate's symptoms

(a) **Scan stalls**
- EXISTS: `worklog/REPRODUCTION/D-SCAN-01/scan-timeline-extract.json`
  (run 42255492260, v0.8.4: 75→76.1% over 891 s, counter frozen at 197,
  sectorMatches empty); the v0.9.0 655 s remnant-sweep freeze
  (`RELEASE-VALIDATION/RELEASE-RUN.md:501-521`); BF-25's unbounded
  cancel-drain captures (runs 42148493806 / 42176600928). Gates exist:
  D-SCAN-01 no-180 s-frozen-stretch (`drive-ui.mjs:696-698`), S2b drain
  gate (`drive-ui.mjs:348-363`).
- MISSING: a GREEN forensic-repro run on a **current** tag (v0.9.3+,
  post-62fb7670 + post-ring-repair) proving the 64 GB scan's full timeline;
  and a benchmark-src matrix run green on scan-deep-wall (44275941738 is
  red at 26,253 ms vs 18,155 cap). The needed capture: dispatch
  `forensic-repro` on the rebuilt release; artifact `forensic-repro-evidence`
  → curate a fresh `D-SCAN-01/scan-timeline-extract.json` (before) vs the
  post-rebuild timeline (after) with maxFrozenSeconds < 180 end-to-end.

(b) **Only system files shown**
- EXISTS (partially): the 6 GiB it-gate proves deleted-file surfacing
  (81/81 rows, `worklog/VALIDATION/it-gates/deleted-tree.txt:1-81`;
  `scan-timeline.csv` 305 files) — but on a DIFFERENT fixture, and
  `scan-events.json` shows `files_found_events: 3` for the whole run (the
  streaming starvation). On the forensic corpus, S3's `foundCounts:["197
  items"]` was never reconciled against the manifest (disk1 seeds 178
  files / 53 deleted).
- MISSING: a results-vs-`seed-manifest.json` **detection-rate gate**. No
  script or gate compares the results tree (paths/names/categories) to the
  manifest's deleted set on the 64 GB/8 GB fixtures. Needed: extend
  `drive-ui.mjs` S3 (or add a bridge-integration it-gate) to dump the full
  results tree and assert found ⊇ manifest-deleted with a recall floor;
  artifact + curated extract under `worklog/VALIDATION/`.

(c) **Phantom non-existent files**
- EXISTS: **NOTHING**. No evidence file in the repo captures a phantom
  result; no gate compares result names against the manifest for
  false positives. The bench harness measures precision on ITS fixtures
  (bench/metrics recall/precision) but the last full run was red and its
  evidence is not committed.
- MISSING: the defining capture of this rebuild. Needed: a results-tree
  dump (tool: `drive-ui.mjs` S3 result capture or an api_harness/it-gate
  tree dump) cross-checked against `seed-manifest.json` — every rendered
  row must map to a manifest entry (live or deleted) or be a validated
  carve; pass criterion: 0 unexplained rows (or precision ≥ floor) on the
  64 GB NTFS + 8 GB exFAT fixtures; artifact curated under
  `worklog/REPRODUCTION/` or `worklog/VALIDATION/`.

(d) **Progress hard-coding / jumps to 75%**
- EXISTS: the jump is exactly reproduced and explained — first sample
  `pcts:[75,75]` (`scan-timeline-extract.json:10-12`); `overall_pm=750`
  pinned on the it-gate fixture (`worklog/VALIDATION/it-gates/scan-timeline.csv:1`,
  `README.md:14-18`); weights cited at `cr_scan.h:72-75`
  (`D-SCAN-01/README.md:22-24`). Post-D4a-d the axes move (v0.9.0
  75.0→76.1 continuously, `RELEASE-RUN.md:513-515`).
- MISSING: a capture of the REBUILT weighted-monotonic progress model —
  a timeline where the global percent is continuous across ALL phases
  (quick → deep → remnant → carve) with no plateau at the 75 boundary and
  per-phase axes live; today's gate only demands ≥ 1.0 pt advance in 15
  min (`drive-ui.mjs:699-701`), which a mostly-frozen scan can pass. The
  it-gate `scan_progress_axes_move` timeline should also capture a
  per-second series (the committed CSV has ONE sample). Pass criterion:
  monotonic, no >180 s flat stretch, sector counters ticking in every
  phase.

(e) **No real-time streaming of results**
- EXISTS: the quantified drop stage — 0 of 12 event types carry rows; UI
  refreshes only at 10% buckets (`scan.ts:41-47`); it-gate event counts
  (`scan-events.json`: 3 files_found events / whole run, 0 heartbeats);
  BF-29's dead `currentPath` on the real backend vs the simulating mock
  (`mock-connector.ts:1152`). The streaming session's Phase 3 (row event)
  was NOT completed (`BEHAVIOR-FINDINGS.md:182-191`).
- MISSING: a during-scan streaming capture on the current product: a
  time-series of result-count (and first-appearance timestamps) DURING a
  live scan — e.g. the drive-ui S3 timeline's `foundCounts` at 5 s cadence
  (which exists as a recorder but is not gated) promoted to a gate (row
  arrivals at ≤ N s cadence, no starvation window > X s inside any phase),
  plus the post-Phase-3 equivalent after the row-carrying event lands
  (entry points listed at `BEHAVIOR-FINDINGS.md:183-191`). Pass criterion:
  results visible on screen within seconds of detection during every
  phase, incl. inside the carve.

### Capture plan (the before/after captures this rebuild must produce on the Windows runners)

| # | Name | Tool / workflow | Fixture | Metric | Pass criterion |
|---|---|---|---|---|---|
| C1 | Stall timeline — BEFORE | `forensic-repro` (any pre-rebuild tag, e.g. v0.9.3) + `drive-ui.mjs` S3 recorder | 64 GB SeedNTFS | maxFrozenSeconds, advancedPctDelta, sector/found movement (`drive-ui.mjs:688-695`) | RED expected: frozen ≥ 180 s or Δpct < 1.0 — the recorded baseline |
| C2 | Stall timeline — AFTER | same, on the rebuilt release | 64 GB SeedNTFS | same + stage boundaries (quick→deep→remnant→carve times) | maxFrozen < 180 s; Δpct ≥ 1.0; sector counters tick in every phase; completes within the capture cap |
| C3 | Scan wall + throughput — BEFORE/AFTER | `benchmark-src` (dispatch, full profile) | 8 GiB CRBIG + matrix VHDs | scan-deep-wall, scan-deepcarve-wall, bench-rand-1m, bench-open, api-recovery, api-save-session vs `engine/bench/baseline.json` rows | All 10 perf rows green (run 44275941738 is the red BEFORE: 7 gate failures); deepcarve ≤ 1,501,622 ms ±3% |
| C4 | Hash-recall / detection rate | `benchmark-src` scenario S-DEL-NTFS | seeded matrix disk | hash-recall vs floor 0.35 | ≥ 0.35 (BEFORE 0.272, run 44275941738); plus cancel+resume `resumedFiles > 0` |
| C5 | Results vs manifest — detection rate on the forensic corpus | extended `drive-ui.mjs` S3 (new: dump full results tree + counts) | 64 GB SeedNTFS + 8 GB SeedEXFAT | found vs `seed-manifest.json` totals (deleted set recall, category buckets) | every deleted seed family represented; recall ≥ floor; counts reconcile with the manifest |
| C6 | Phantom-file audit | same dump (C5) or api_harness tree CSV | same fixtures | rows not attributable to any manifest entry (live or deleted) | 0 unexplained rows (or precision ≥ floor) — the (c) symptom's defining capture |
| C7 | Progress model capture | it-gate `scan_progress_axes_move` extended to a per-second series (today 1 sample) + `measure-cr-ui.c` scan phase | 6 GiB it-gate VHDX / system volume | overall_pm continuity, phase axes, bytes_scanned/bytes_found per phase | monotonic global percent, no 75-plateau by construction, live bytes_found > 0 from first find |
| C8 | Streaming cadence capture | extended `drive-ui.mjs` S3 (gate on the existing 5 s `foundCounts` series) + post-Phase-3 `scan-result:{id}` event | 64 GB SeedNTFS | result-arrival cadence during scan; starvation windows | rows appear ≤ every N s during every phase incl. carve; no > X s window with zero new rows while detection is active |
| C9 | Cancel/drain — AFTER | `forensic-repro` S2b vehicle (already gated) | 8 GB SeedEXFAT | drain-banner latency, cancel_to_stop_ms (`bench/ours/run-behavior.ps1`) | drain banner within the 15-min gate with margin; cancel_to_stop bounded (checkpoint cadence) |
| C10 | Curated extracts committed | manual curation of run artifacts | — | `worklog/REPRODUCTION/D-SCAN-01/` refresh + `worklog/VALIDATION/` (today only `it-gates/`) | every number above reproducible from a committed file + run ID |

All captures ride existing vehicles (`forensic-repro`, `benchmark-src`,
`bridge-integration` it-gates, `validation-evidence`); the only NEW tooling
this rebuild needs is the results-tree-vs-manifest dump/gate (C5/C6) and
the per-second progress/streaming series (C7/C8).
