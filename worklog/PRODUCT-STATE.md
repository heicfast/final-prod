# PRODUCT-STATE — session 2026-11-17 (SMART/scan/parity rebuild)

> Agent A, Phase-0 reconnaissance. READ-ONLY map of the PRODUCT repo
> `/home/z/my-project/final-prod` at HEAD `d145dd1b` (worklog commit on top of
> engine/ui state `e119cc71`). Every claim cites `path:line`; UNKNOWN marks
> what could not be determined statically. Claims needing a Windows runtime are
> marked "needs runtime evidence".

## 1 Build & repo

### 1.1 Layout
- Top level: `engine/` (C++ core), `ui/` (React 19 + Vite frontend), `ui/src-tauri/`
  (Rust bridge + it/ crate), `installer/` (NSIS `.nsi`, `devsign.ps1`,
  `portable.ps1`), `contracts/` (command-surface.json, error-codes.json,
  event-mapping.md, type-maps.md, validate.mjs), `tools/` (gen_abi_layouts.py,
  forensic, validation), `docs/`, `worklog/`, `build.ps1`, `build.sh`, `VERSION`,
  `worklog.md` (170 KB legacy log).
- `VERSION` file content: `0.9.2`.

### 1.2 Version / release state (the single most important repo fact)
- Last release tag: **v0.9.1 @ cdf66ff8** (2026-11-06). There is **NO v0.9.2
  tag** — the tree's VERSION/package.json/Cargo.toml all say 0.9.2
  (ui/package.json:4, ui/src-tauri/Cargo.toml:3) and commit `1ef7aa7c` says
  "chore(release): v0.9.2", but the tag was never cut. The user's installed
  product is therefore **v0.9.1 or older**; everything after cdf66ff8 is
  unreleased (26 commits v0.9.1..HEAD; DIAGNOSIS.md:13-16 states this).
- Which fixes the SHIPPED v0.9.1 lacks (verified via `git merge-base
  --is-ancestor`): `0e87689d` (D4f carve freeze fix) NOT in v0.9.1;
  `eb4bb5f3` (D-ENUM-02 bus table + D-NVMe-01 NVMe query fix) NOT in v0.9.1;
  `ba2b9347` (BF-ABI-01 bridge re-pin) NOT in v0.9.1 (also post-0.9.2);
  `693d4d60` (P-DEEP-01/P-HB-01/P-TICK-01 scan-truth train) NOT in v0.9.1.
  In v0.9.1 already: `ce71143e` (D4a/b/c live counters), `68460a4c`
  (D1+D2 privilege/console), `6e12f167` (NG-5..10 SMART truth).

### 1.3 Build system
- `engine/CMakeLists.txt`: project core_recovery, C++20, MSVC `/W4 /WX
  /permissive-` (engine/CMakeLists.txt:19-21), Win7 SP1 target macros
  `_WIN32_WINNT=0x0601` (:29-30), zlib REQUIRED (:48-53), lz4+zstd REQUIRED
  (:141-147), options `CR_BUILD_APP` (rrecov CLI, Windows only) and
  `CR_BUILD_UI` (the cr_ui.dll C-ABI shim, :40-41), GTest via FetchContent
  (:170-180).
- `engine/CMakePresets.json`: linux-gcc (ASan), linux-release, linux-ubsan,
  win-x64-debug/release (VS2022 + vcpkg toolchain).
- `build.ps1` (Windows full product): VERSION lockstep gate across
  package.json/Cargo.toml/tauri.conf.json (:37-46); engine lane =
  cmake+ctest+PE Win7 gates+engine kit staging+dev signing (:56-132); ui lane
  = pnpm typecheck/lint/test + tauri build + **D1 gate** (byte-scan for
  `requireAdministrator` in the exe, :174-190); installer lane = NSIS with
  version-locked bundle pick (:209-247); portable zip + SHA256SUMS (:249-268).
- `build.sh` (Linux subset only): contracts validate + engine linux-release +
  ui static checks; explicitly "the FULL product is Windows-only"
  (build.sh:2-9).
- `ui/package.json`: React 19.3.1, Tauri API/CLI 2.12.4, pnpm 12.15, node>=24,
  vitest/playwright/knip/jscpd lanes (:11-31).
- `ui/src-tauri/Cargo.toml`: crate `recovery-wizard-ui` 0.9.2, tauri 2,
  windows-sys 0.59 (shell-side probes only; the engine transport lives in
  cr_ui.dll, :21-34); release profile lto+opt-level=s+panic=abort (:36-41).
- `ui/src-tauri/it/`: the integration crate `bridge-it` (BF-ABI-02 closure) —
  only built by bridge-integration.yml; needs cr_ui.lib at link + cr_ui.dll at
  load (it/Cargo.toml:5,14-19,28).

### 1.4 CI workflows (trigger / OS / purpose)
- `bridge-integration.yml`: push main on `engine/**`, `ui/src-tauri/**`,
  `contracts/**`, `tools/gen_abi_layouts.py` + PR + dispatch (:27-40);
  windows-2022; one job "Build engine + link bridge + run integration tests"
  (:51-52): [1/5] generated-files freshness, [2/5] engine build, [3/5] layout
  manifest dump, [4/5] multi-disk VHDX attach + seed, [5/5] `cargo test
  --features layout-gate` over it/ (:219-243).
- `contracts-ci.yml`: push main on contracts/engine-include/ui-connector/
  ui-tauri-src; ubuntu-24.04; validate.mjs phase 1+2 + filefilter mirror
  freshness (:8-25, 39-42).
- `engine-ci.yml`: push main on `engine/**` + the workflow file, **excluding
  `**.md`, `engine/docs/**`, `engine/worklog*`** (:42-60); also
  workflow_call (release.yml gate) + dispatch; windows-2022; jobs `lint`
  (clang-format, dead-surface/arch lints, PS AST + YAML parse gates, :76-249)
  and `build-and-test` (MSVC /W4 /WX Release, full ctest incl. cr_ui ABI
  check, Win7 posture gate, G-09 schema, elevation refusal probes, M-071,
  :251-497).
- `forensic-repro.yml`: **workflow_dispatch only** (:44-45); windows-2022;
  installs the shipped release, seeds 64 GB NTFS + 8 GB exFAT vdisks, drives
  the real UI via WebView2 CDP, asserts D-PRIV-01/D-ENUM-01/D-SCAN-01 gates
  (:93-223).
- `release.yml`: tag push `v*` (:21-26); windows-2022; full product build via
  build.ps1 all lanes + artifacts + truthful notes (:29-49).
- `ui-ci.yml`: push main on `ui/**`, `contracts/**` (:9-19); windows-latest;
  typecheck/lint/format/arch-lint(A1-A7+ast-grep)/knip/jscpd/vitest (coverage
  70/60/70/70)/build/bundle budget + `cargo check --tests` + **Clippy strict**
  (:43-110).
- `ui-e2e.yml`: push main on `ui/**`; windows-latest; Playwright 12 specs
  mock-pinned against vite preview (:9-70).
- `validation-evidence.yml`: workflow_dispatch only; windows-2022; T14 engine
  latency/bridge measurements + T15 installer runtime matrix (:27-34).

### 1.5 Git history for the defect areas
- Health (`engine/src/core/health`, `engine/src/win/cr_health.cpp`,
  `ui/src/features/health`), most recent first: `e119cc71` (it-crate compile),
  `eb4bb5f3` (D-ENUM-02+D-NVMe-01: shifted bus table, dead NVMe pack(1)
  offset-9 query, RemovableMedia honored, bus_type ABI axis, health ex2),
  `68460a4c` (D1+D2 privilege/console), `6e12f167` (NG-5/6/7/9/10 SMART truth),
  plus older format/merge commits (696badec, 695f6da0, ...).
- Scan (`orchestrator/carve/tree/query`): `693d4d60` (P-DEEP-01+P-HB-01+
  P-TICK-01 scan-truth), `0e87689d` (D4f lock-free user-sign probe), `6565c112`
  (format), `ce71143e` (D4a+b+c frozen truth axes), then the M-107/M-160 carve
  decode train (SWF/APE/DWG/WPD members) and merge repairs.

## 2 SMART subsystem (current truth)

### 2.1 ABI surface
- `cr_ui_health_read` + `cr_ui_health_read_ex2` (engine/include/cr/cr_ui.h:1586,
  1602). Report struct `cr_ui_smart_report` (cr_ui.h:1552-1582): ATA fields
  (available, transport, versions, failing_now, reallocated/pending/
  uncorrectable/power-on-hours/spin-retry/wear/temperature), NVMe block
  (nvme_available, temp x10, spares, percentage_used, data units, unsafe
  shutdowns, media errors, critical_warning, host r/w, busy time, error-log
  count, byte conversions, power_cycles_nvme), media class (ssd_like), TRIM
  (trim_known/trim_enabled). Attr row `cr_ui_smart_attr` (cr_ui.h:1536-1550)
  with the BF-03 `threshold` tail (ex2 only).
- Engine impl `health_read_impl` (engine/src/ui/cr_ui.cpp:4889-4994): opens
  `\\.\PhysicalDrive%u` GENERIC_READ|WRITE, maps unelevated ACCESS_DENIED to
  ERR_ELEVATION_REQUIRED (BF-28 fix, :4907-4920), runs ATA SMART then NVMe then
  seek-penalty then per-device TRIM, refuses with ERR_HEALTH_UNAVAILABLE when
  both sources absent (:4990-4993).

### 2.2 Transports (exact Win32 calls)
- ATA passthrough: `DeviceIoControl(IOCTL_ATA_PASS_THROUGH)` with
  ATA_PASS_THROUGH_EX, 512-byte data-in, task file feature/count/0xA0/command
  (engine/src/win/cr_health.cpp:38-64).
- Legacy: `SMART_RCV_DRIVE_DATA` 0x0007c088 with the og's exact SMART IDEREGS
  (count 1, sector 1, cyl 4F/C2, cmd B0) (cr_health.cpp:69-109).
- CSMI: `IOCTL_SCSI_GET_ADDRESS` on the disk → open `\\.\Scsi%u:` →
  `IOCTL_SCSI_MINIPORT` CC-0x19 wire request (cr_health.cpp:143-208) for
  Intel RST/AMD rcraid.
- NVMe: `IOCTL_STORAGE_QUERY_PROPERTY` with
  StorageDeviceProtocolSpecificProperty / ProtocolTypeNvme /
  NVMeDataTypeLogPage page 2, protocol data at
  `offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)==8` with
  static_asserts (cr_health.cpp:264-355). The comment records D-NVMe-01: the
  pre-`eb4bb5f3` `#pragma pack(1)` buffer placed protocol data at offset 9 and
  **every real NVMe controller refused** — "the user's 'switch to another disk
  and it shows nothing'" (cr_health.cpp:277-288).
- SSD heuristic: `StorageDeviceSeekPenaltyProperty` (cr_health.cpp:461-478);
  per-device TRIM: `StorageDeviceTrimProperty` (:431-459); global TRIM via
  hidden `fsutil behavior query disabledeletenotify` CreateProcessW pipe with
  CREATE_NO_WINDOW + 10 s bound (D2, :357-429).

### 2.3 Pure parse + grading
- `cr_smart_ata.cpp`: first channel answering IDENTIFY (0xEC) serves
  READ DATA (0xB0/D0) + READ THRESHOLDS (0xB0/D1, non-fatal); 30x12 attribute
  table parse; derived counters; IDENTIFY model string; then the S-024 grading
  pass (engine/src/core/health/cr_smart_ata.cpp:94-220).
- `cr_grade.cpp`: 4-status model Good/Caution/Bad/Unknown decoded from the og
  DiskHealth grader — per-attr rule chain (:46-201), disk grade (:214-338),
  NVMe grade `grade_nvme` (critical_warning→Bad; life>10→Good; ==10→Caution;
  else Bad; Parallels virtual disk→Good, :359-375). **No percentage is
  computed anywhere in the engine** — status words only.
- Unit tests: engine/tests/unit/test_health_csmi.cpp (270 l), 
  test_health_grade.cpp (466 l), test_nvme_health.cpp (256 l, the D-NVMe-01
  source-truth layout pins per cr_health.cpp:285-288).

### 2.4 Bridge + UI (current tree)
- `ui/src-tauri/src/commands/health.rs`: ex2 first, base export fallback
  (:37-44); refuses when both sources absent (:62-67); status grading
  failing/critical→bad, attr-failing|wear>=80|low-spare→caution, data
  present→good, else unknown (:100-111); **"No percentage is invented anywhere
  in this file"** (:11); wearPercent passthrough only (:156).
- `ui/src/features/health/health-page.tsx`: one SELECTED disk at a time
  (useHealth(deviceId), :51); ring shows the STATUS by class fill
  good=1/caution=0.66/bad=0.34 — "explicitly NOT a numeric claim"
  (F-HLT-02 fix, :100-110); 4 KPI tiles (temperature/hours/class/TRIM-wear);
  SMART table with ID/Attribute/Current/Worst/Threshold/Raw/Status
  (:478-554). The pre-fix v0.9.1 page computed a fake `healthScore`
  (max(88,100-wear)) — the exact "fake green percentage" the user saw
  (DIAGNOSIS.md:100-102).
- Disk enumeration for the picker: `devices_list` → bridge `enumerate_disks`
  (two-call cr_ui_enumerate_disks) → engine `enumerate_disks_impl`
  (engine/src/ui/cr_ui.cpp:1829-1892, max 200, win::enumerate_disks).
- `engine/src/win/cr_enumerate.cpp`: two stacks — numbered
  `\\.\PhysicalDrive0..199` loop with D3 keep-present-but-unopenable rows
  (:122-162) + cfgmgr32 `CM_Get_Device_Interface_ListW`
  GUID_DEVINTERFACE_DISK pass (:205-263); per-disk IOCTLs:
  IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, StorageAccessAlignmentProperty,
  StorageDeviceProperty (product@+16/revision@+20/serial@+24/bus@+28),
  StorageDeviceIdProperty, IOCTL_STORAGE_GET_DEVICE_NUMBER; volumes via
  GetLogicalDriveStringsW exact-size retry (E-01, :265-325).
- SD: `map_kind` maps bus 12/13 (SD/MMC) → `'sd'` (ui/src-tauri/src/commands/
  devices.rs:145-158) — only possible since `eb4bb5f3` (engine bus_type axis)
  + `ba2b9347` (Rust mirror re-pin). In v0.9.1 `kind:'sd'` could never fire
  (BF-31) and the SD page showed a hard-coded fake card (DIAGNOSIS.md:83-85).

## 3 Scan pipeline (current truth)

### 3.1 Stage model
- Stages: quick | deep | carve (`stage_mask` bits 1/2/4, cr_ui.h:525-527;
  default 7). Weights default **40/35/25 percent-of-total** (cr_ui.h:530-532).
- Quick = ONE structured engine pass (single MFT walk shared with deep —
  `run_quick` passes `deep_coming` into `run_engine_pass`,
  engine/src/core/orchestrator/cr_scan.cpp:1642-1662). Deep = remnant sweep
  (NTFS `scan_mft_remnants`) + whole-disk lost-partition discovery
  (`lost::discover`) + parallel candidate scans capped at 10 (og's cap,
  cr_scan.cpp:2237-2418). Carve = 1 MiB windows (`WINDOW = 1ull << 20`,
  engine/src/core/carve/cr_carve.cpp:45) fanned over a thread pool
  (resolve_threads: hw-1 capped 8, cr_scan.cpp:58-68).
- Order: quick → carve-before-deep when quick done (N-181, og starts RAW
  controllers before deep, cr_scan.cpp:1505-1539) → deep → carve; then junk
  folder filter (filterfolder, :1572), S-023 recoverability grading (:1579-
  1594), and the read-truth completion gate (fully-blind scan REFUSES with
  ERR_READ_FAILED, :1595-1639).

### 3.2 Progress computation (where 30-75% comes from)
- `weighted_progress` = (w_q*q_pm + w_d*d_pm + w_c*c_pm)/total in permille,
  monotonic per stage (cr_scan.cpp:131-146, 500-507). Stage permille from
  engine callbacks via `tel_publish_` (:485-545); overall monotonic-clamped.
- **The 75% jump is exact math**: on a healthy volume quick (40%) + deep's
  engine pass (35%) complete in seconds, so the first UI sample lands at the
  carve boundary = 75.0% (worklog/REPRODUCTION/D-SCAN-01/README.md "The 75%
  jump is exact math"; DIAGNOSIS.md:69). "Never shows a quick-scan phase":
  the quick stage is a single metadata pass that finishes in seconds, and the
  UI phase label 'advanced' spans deep-walk+carve
  (ui/src/connector/contract.ts:177-184; events.rs maps engine stage 0/1/2 →
  'quick-index'/'deep-walk'/'carve', ui/src-tauri/src/bridge/events.rs:277-285;
  label picked at ui/src/features/scan-progress/scan-progress-page.tsx:224).
- Byte axes (current tree only, `693d4d60`): deep stage bytes move
  (P-DEEP-01, cr_scan.cpp:522-533), carve live bytes (D4a, :514-521), live
  read cursor `current_offset_bytes` (P-TICK-01, :510-513) surfaced as
  currentSector/currentSectorTotal (events.rs:199-208). Progress events
  coalesce per overall-permille step OR >=16 MiB bytes step (cr_scan.cpp:660-
  691). 5 s heartbeat refreshes telemetry + emits (P-HB-01,
  events.rs:361-376). **None of this is in the shipped v0.9.1.**

### 3.3 Sector-reading loop, stall points
- Read chain: slice → SliceShim → RetryingDevice (3 attempts, Skip-mode
  zero-fill, never aborts) → CachedReader 8 MiB / 256 KiB blocks
  (build_read_pipe, cr_scan.cpp:164-203; R1.6/M-058 comments :1325-1350).
- Carve window loop: per-byte grammar dispatch at 512-byte strides with
  cancel checkpoint every 4 KiB (D4c, cr_carve.cpp:622-634); all-zero strides
  fast-skipped; the pre-D4f probe took a registry mutex twice per stride over
  ~134M zero strides = the **605 s freeze** ("stuck in sector scanning"),
  fixed lock-free at `0e87689d` (cr_carve.cpp:176-179 comment) — fix NOT in
  shipped v0.9.1.
- Remaining known wall: BF-30 carve matcher throughput 8.45 MiB/s (76-124
  s/GiB) — OPEN, upstream bench scope (worklog/BEHAVIOR-FINDINGS.md:55).
- Cancel: token checked at stage heads, engine loop heads, per-4 KiB in
  windows; BF-24/BF-25 record the drain-window semantics.

### 3.4 Results streaming + filtering + tree
- Events: engine `event_fn` → channel → single emitter thread → Tauri
  `scan-progress:<id>` (events.rs:46-52, 230-252, 258+); FilesFound counts
  ride progress ticks (N-166, cr_scan.cpp:644-655).
- Results: `cr_ui_scan_snapshot` → bridge NodeMeta cache (one DFS,
  ui/src-tauri/src/bridge/state.rs:625-760) → **bridge-side filtering**
  (results.rs query_page, :440-482) — the engine query_run is deliberately
  unused because the contract's 'existing' atom (active-only) has no engine
  knob (results.rs:11-13; PARITY-DELTAS D-B3). Category bits come from the
  GENERATED extension table (bridge/filefilter.rs, 1481 extensions mirrored
  from engine/src/core/query/cr_filefilter_data.cpp; freshness gate in ui-ci).
- Deleted vs live: tree roots "Found" / "Deleted" / "File Path Lost" +
  "Reconstructed" (carve) (cr_scan.cpp:305-321). MFT walk emits live AND
  deleted records (`default_root = in_use ? root_active : root_deleted`,
  engine/src/core/ntfs/cr_mftwalk.cpp:1033; DeletedState set :1087-1089);
  **deleted directory structure IS reconstructed** — records attach under
  their parent-record chain (emit_parents, cr_mftwalk.cpp:1035-1041); only
  unresolvable parents land flat under the roots.
- Carve tree: "Reconstructed" root + volume display name + 7 empty category
  folders Pictures/Documents/Archives/Videos/Audio/Emails/Others
  (cr_scan.cpp:205-303).
- Whole-disk: table-target flow scans every partition under its display-name
  root (S-051, cr_scan.cpp:2015-2159); LDM dynamic volumes probed
  unconditionally (:1964-2014).

### 3.5 Scan wiring (bridge)
- `scan_start` zero-init config (stage_mask 7, weights 40/35/25 default),
  whole_disk = !volume-target, pre_scan_repair only on volume targets (NG-21
  guard), pre_scan_check on, autosave to sessions dir
  (ui/src-tauri/src/commands/scan.rs:217-245); run thread calls
  `cr_ui_scan_run` synchronously (scan.rs:339-364); emitter + consent pipe
  threads launched (:339-346).

## 4 Open registers

### 4.1 worklog/BEHAVIOR-FINDINGS.md — OPEN rows
- BF-07: ERR_DEVICE_DISCONNECTED reattach/resume UX gap — Open.
- BF-12: image targets (open_image → VMDK/VHD/VHDX scans) unsurfaced — Open.
- BF-16: core query engine unused (NodeMeta carries the UI) — Open.
- BF-21: M-101 version_group/tags NTFS grouping unsurfaced — Open.
- BF-28: engine health/open refusal codes not uniformly
  ERR_ELEVATION_REQUIRED — Open, upstream.
- BF-29: "Searching: <path>" ticker dead (currentPath null) — row says Open,
  but commit 693d4d60 claims BF-29 closed via the real read cursor
  (events.rs:191-208); register row is stale relative to HEAD.
- BF-30: carve throughput wall 8.45 MiB/s — Open, upstream.
- BF-31: kind 'sd' never fired from real hardware — row says Open; closed at
  ba2b9347 per its commit message (register row predates it).
- BF-32: scan-emitter teardown UAF (SendPtr raw scan handle) — Open, bridge
  lane; two-phase fix design recorded (BEHAVIOR-FINDINGS.md:57).
- Mitigated/closed rows: BF-01..06, 08-11, 13-15, 17-20, 22 (D4a-c fixed),
  23-27.

### 4.2 docs/PARITY-DELTAS.md — requested rows
- **D-P4**: reference ships Disk health as a SEPARATE companion app
  (DiskHealth.exe, QML, 840x590); ours is an in-app /health page — one
  product/one install (PARITY-DELTAS.md:53).
- **D-P6**: reference's advanced phase runs its OWN 0-100% per phase with
  status line (Files Found N (GB), sector ticker, Searching ticker,
  live-growing tree); ours is one global 40/35/25-weighted curve — recorded
  divergence; frozen/stalled presentation is a DEFECT class, not a divergence
  (:55).
- Scan-flow/filter/preview/footprint rows: D-B3 bridge-side filtering for
  both live scans and sessions (:37); D-P2 hidden fsutil TRIM (:51); D-P7 no
  silent TRIM disable during SSD scans (:56); D-IMG-1..3 image create no
  cancel / flat .img vs .edisk / whole-disk only (:63-65); D-MAP-1 sector-map
  paint rules honest-to-engine not og-pixels (:66); D-I1..I11 installer
  divergences (:19-29); D-FALL/D-DSIGN driver-era reversals (:40-41).

### 4.3 worklog/REPRODUCTION/
- **D-CONS-01** (console popups): v0.8.4's `_popen("fsutil ...")` at
  cr_health.cpp:306 (then) allocated a visible cmd.exe on EVERY health read
  in the GUI-subsystem process; regression from engine re-merge 283a5d90;
  fixed at 68460a4c with a no-_popen build-time guard
  (REPRODUCTION/D-CONS-01/README.md:9-27).
- **D-ENUM-01** (only one disk of three): code-level reproduction — pre-fix
  Home page rendered `kind==='internal'` rows only while the bridge degraded
  USB/SD/MMC-bus disks to 'usb' (BF-02), so non-internal-bus devices were
  invisible; runner showed all four disks because Azure disks are all
  internal-bus "Virtual Disk" (README.md:16-30 + home-dom-extract.json).
- **D-PRIV-01** (no admin request): installed exe had NO trustInfo manifest
  ("no-manifest-match") through v0.8.x — the docs' requireAdministrator claim
  was "documentation hallucination"; fixed in 0.9.0 via app.manifest + D1
  byte-scan gate (README.md:17-27; facts-postinstall-appmanifest.json).
- **D-SCAN-01** (75% advanced, stuck, 0 B): 172-sample timeline — first
  sample already 75.0% 'advanced'; 1.1% in 15 minutes with complete:false;
  found counter frozen at 197 the whole run; dataFoundBytes pinned 0 (no ABI
  field then; bridge latched at terminal). = BF-22 reproduced on real
  hardware (README.md:9-39 + scan-timeline-extract.json).

### 4.4 Footprints
- docs/INSTALL-FOOTPRINT-CURRENT.md: carries a STALE-BY-CONSTRUCTION ERRATUM
  (frozen at the v0.6.0-rc1 driver era, :3-10); current truth per §A.1 and
  the 2026-11-05 correction: **flat 8-file install dir**
  (recovery-wizard-ui.exe, uninstall.exe, cr_ui.dll, rrecov.exe, z/lz4/zstd
  DLLs, HeicFastUninstall.exe) + %ProgramData% sessions/logs + registry; no
  service, no driver, no System32, no certificates (:28-33).
- docs/INSTALL-FOOTPRINT-DELTA.md: reference = 1,470 files / ~670 MiB /
  ~114 modules; ours = 8 files. Verdict: numeric gap dominated by
  never-claimed features (WinPE 194 files, viewers/repair/cloud/NAS ~800+,
  37 language packs, QA residue); everything claimed ships inside cr_ui.dll;
  **exactly two real gaps**: EUFileRepair suite not shipped (truthfully
  disabled in UI) and partition-apply CLI-only via rrecov (DELTA.md:19-27,
  56-65, 95-111).

### 4.5 engine/diff-to-fix/
- modules/: 33 per-module sheets (abi, apply, boot-repair, btrfs, carve,
  check-phase, cli, driver-install, err, exfat, ext4, fat, health,
  hfsplus, imaging, ldm, lost-partition, mom-capabilities, ntfs,
  orchestration, orchestrator, preview, query, rawfixer, refs, reghive,
  session, storage, transport, tree, visualization, writer, zfs).
- findings/: FIXED 221, MINOR 2, MAJOR 3 (M-086 driver fileversion pin,
  M-087 regdrv UpperFilters, M-089 rdabench phantom-zero ioctl gap — all
  driver-era), tracked-open/ 3 (OQ-S10-1 thumbnail codec bound formats,
  OQ-S10-3 driver-repo-resident fixes, OQ-S4-3 chunking relative perf).
- Health/scan-relevant open findings in diff-to-fix: none in MAJOR/
  tracked-open; health.md and orchestrator.md module sheets carry the
  historical M-/N- finding trails (details beyond the file list: UNKNOWN, not
  read line-by-line this session).

## 5 CI status (facts as provided + local verification)

At HEAD e119cc71 (worklog commit d145dd1b on top):

- **bridge-integration.yml: FAILURE** — job "Build engine + link bridge + run
  integration tests", step "[5/5] cargo test — the bridge integration suite"
  (run 43108277926). LOCAL VERIFICATION: the it crate **still does not
  compile** — `ui/src-tauri/it/tests/bridge_integration.rs:461` assigns
  integer literal `0` to the f64 variable `frozen_run_s` (declared `0.0f64`
  at :454) — a hard E0308 type error; commit e119cc71's message claims
  "frozen_run_s as f64" was fixed, but only the declaration was. The other
  claimed fixes verify clean: `pub mod bridge` (bridge/mod.rs:19-27), `r#type`
  escaping (abi_layouts.rs:115), `recovery_wizard_ui_lib::bridge::abi/util`
  imports resolve (bridge_integration.rs:27-28). CONFIRMED by code reading;
  final word needs the CI log (no cargo in this sandbox).
- **ui-ci.yml: FAILURE** — job "ci", step "Clippy (strict; the lib surface)"
  (run 43108277889). Cause UNKNOWN statically — no cargo/clippy in this
  sandbox; candidates are the Rust files touched by the recent train
  (693d4d60 touched bridge/abi.rs, events.rs, state.rs, commands/scan.rs,
  sessions.rs; e119cc71 touched commands/imaging.rs, lib.rs). Needs the CI
  log; CI is the only gate for this lane.
- **engine-ci.yml: last run at 3415c71b FAILURE** (run 43102577756). Trigger
  verified locally: 3415c71b itself touches only ui/src/connector/contract.ts,
  but it was pushed together with 693d4d60 (engine/** changes), so the
  engine/** path filter fired with push-head 3415c71b. It did NOT run at
  e119cc71/d145dd1b because those commits touch no engine/** file (e119cc71:
  tools/, build.ps1, installer/, ui/ only — verified via git show --stat).
  Prior session labeled the failure the deliberate RED Phase-1 reproduction
  (drifted ABI mirror, per commit c3e0767f's message); exact failing step:
  UNKNOWN locally, needs the run log.
- contracts-ci.yml: SUCCESS at e119cc71. ui-e2e.yml: SUCCESS at e119cc71.
- release.yml: last success cdf66ff8 = the v0.9.1 tag (2026-11-06); **no
  release has shipped any post-v0.9.1 fix**.
- forensic-repro.yml: last runs failed (2026-11-06); note CI-STALE-01 — its
  default tag is v0.8.4 (DIAGNOSIS.md:113-114).
- validation-evidence.yml: last success 8edb721c (2026-11-06).

## 6 Local build feasibility in the sandbox

- Available: g++ 14.2.0, node v24.22.3, python3. Absent: **cmake, cargo,
  rustc, clang/clang-format, pnpm** (`which` found none).
- Consequences: the engine Linux build (build.sh / ci-local.sh) cannot run
  (cmake required, build.sh:12; g++ alone insufficient). **No local cargo
  check/clippy/test reproduction is possible — CI is the only gate for the
  Rust surface** (bridge + it/ crate + the two red lanes). contracts
  validation (node contracts/validate.mjs) would run; ui static checks need
  pnpm (absent).
- Documented local flows (for a Windows/Linux dev box, not this sandbox):
  build.ps1 all lanes (Windows full product, docs/BUILD.md:9-11); build.sh
  contracts|engine|ui (Linux subset, build.sh:45-50);
  engine/ci-local.sh --san/--fuzz/--all (10-step gate mirroring CI,
  ci-local.sh:8-14; needs clang-format); engine/ci-local.ps1 (Windows);
  engine/README.md:65-79 (cmake presets win/linux + ctest);
  engine/CONTRIBUTING.md's never-push-a-red-commit rule.

## 7 The defect hypotheses (user-reported → root causes)

### 7.1 SMART: "reports only ONE disk, fake green percentage, no attributes, SD weirdness, nothing like the reference"
- **CONFIRMED (stratum 1, the shipped build)**: the user runs v0.9.1-or-older
  (no release since cdf66ff8 carries any of this):
  - "Only one disk / switching shows nothing": D-NVMe-01 — every real NVMe
    controller refused the mis-packed protocol query, so only the first (SATA)
    disk had health (cr_health.cpp:277-288 comment; DIAGNOSIS.md:22-26);
    D-ENUM-02 — the shifted bus table mislabeled SATA/removable rows
    (DIAGNOSIS.md:18-21).
  - "Fake green percentage": v0.9.1's health page computed an invented
    healthScore (max(88, 100-wear)) — F-HLT-02, fixed only in the current
    tree (health-page.tsx:101-107 comment; DIAGNOSIS.md:100-102).
  - "No attributes": pre-eb4bb5f3 the ex2 threshold axis was unbound and the
    table could render empty; on NVMe disks no ATA table exists at all
    (health.rs handles this honestly only post-NG-6/9).
  - "SD shows something weird": kind 'sd' never fired (BF-31); SD cards
    rendered as USB or a hard-coded fake card (DIAGNOSIS.md:49-50, 83-85).
- **CONFIRMED (stratum 2, current tree until ba2b9347)**: ABI drift — engine
  structs appended bus_type/power_cycles_nvme/threshold while the Rust mirror
  lagged: 8×N-byte heap overflow in enumerate_disks (row collapse → "only one
  disk") and a 16-byte stack smash per health read (DIAGNOSIS.md:41-52).
  Fixed at ba2b9347; the bridge-integration lane now pins it.
- **Structural parity gap (by design, registered)**: our health surface is a
  per-disk in-app page — ONE selected disk at a time (health-page.tsx:51) —
  while the reference is a separate DiskHealth app listing ALL disks with
  per-disk status at once (D-P4). "Works nothing like the reference" is
  partly this registered divergence, not only bugs.
- Needs runtime evidence: which stratum the user's exact build hits (their
  install version is not recorded).

### 7.2 Scan: "jumps to 30-75% advanced, no quick phase, stalls in sector reading, no results, filters dead, no images/docs/archives, no deleted tree"
- **CONFIRMED — the 75% jump / no quick phase**: weights 40/35/25 + quick
  completing in seconds ⇒ first sample at the carve boundary = 75.0%;
  "advanced" spans deep+carve (contract.ts:177-184; D-SCAN-01 README; engine
  cr_scan.cpp:131-146). The 30-75% band is the deep+carve span of the same
  curve. The reference's per-phase restarted 0-100% is the registered D-P6
  divergence.
- **CONFIRMED — stall in sector reading (shipped build)**: D4f — the
  user-sign probe took a registry mutex 2× per 512-B stride over ~134M zero
  strides = 605 s all-axes freeze (cr_carve.cpp:176-179; fixed 0e87689d,
  NOT in v0.9.1); plus pre-693d4d60 the deep stage had no byte axis and the
  5 s heartbeat was swallowed (events.rs:361-376 comment; DIAGNOSIS.md:89-97)
  — even healthy sub-passes looked frozen.
- **CONFIRMED — "no real results"**: on the user's machine the scan never got
  past the frozen carve window (found-count frozen at 21; D-SCAN-01 analog);
  results only surface via snapshot after stages emit. Additionally the
  read-truth gate now refuses fully-blind scans (cr_scan.cpp:1595-1639) — the
  user's build pre-dates even that honesty.
- **HYPOTHESIS (high confidence) — "filters don't work"**: filters are
  bridge-side over the NodeMeta cache (results.rs:440-482; D-B3). With the
  scan stalled pre-carve, the snapshot/cache is empty or tiny, so every
  category chip (images/documents/archives) returns nothing — the filter is
  not dead, it has no data. Needs runtime evidence on the user's build to
  separate "empty cache" from an actual filter defect.
- **HYPOTHESIS (high confidence) — "images/documents/archives not found"**:
  those categories are populated by the CARVE stage (Reconstructed/Pictures/
  Documents/Archives skeleton, cr_scan.cpp:205-303) and by extension mapping
  (filefilter.rs). The user's scan stalled at the carve boundary, so carved
  categories never appeared. Engine-side carve capability itself is
  extensive (M-107 11-member decode train).
- **CONFIRMED (engine side) — deleted tree reconstruction EXISTS**: MFT walk
  emits deleted records under "Deleted" with parent-chain attachment
  (cr_mftwalk.cpp:1033-1098). If the user saw no deleted structure it is
  because the scan never completed the pass on their build (or their volume
  is not NTFS — FAT/exFAT deleted-chain reconstruction is deep_extras-gated).
  Needs runtime evidence.
- Remaining engine-side perf risk (current tree): BF-30 carve throughput wall
  (8.45 MiB/s) — multi-TB scans take hours even with all D4/P fixes.

### 7.3 "Product ships far fewer files than the reference"
- **CONFIRMED as registered architecture, not absence**: 8-file install vs
  1,470-file reference; the claimed functional core is consolidated into
  cr_ui.dll; only two real gaps (EUFileRepair suite; partition-apply
  CLI-only) (INSTALL-FOOTPRINT-DELTA.md:19-27, 95-111). The user-perceived
  "fewer files" is dominated by never-claimed features (WinPE, viewers,
  37 language packs, cloud/NAS). If the rebuild wants reference-like module
  separation (per-FS DLLs, DiskHealth companion), that is a packaging
  decision to re-open, currently recorded as deliberate (D-P4, D-B1).

## 8 What a rebuild session must know (one-paragraph orientation)

The codebase is a clean-room parity rebuild against a decompiled reference
(easeus-drw-research corpus, cited everywhere as DRWUI/DiskHealth_decompiled
line numbers). The engine is mature (1,683+ tests, 93-export C ABI, decode
trains for carve formats), the bridge is newly re-pinned (ba2b9347) with a
dedicated integration lane that is currently RED on a one-character test
compile error (bridge_integration.rs:461), and the user's reported symptoms
are almost entirely explained by fixes that exist on main but were never
released (no tag since v0.9.1) plus the registered D-P4/D-P6 divergences.
The first lever is releasing the current train; the second is the D-P6
per-phase progress model; the third is the DiskHealth-style multi-disk
surface.
