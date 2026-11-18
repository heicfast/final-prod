# PRODUCT-STATE — performance-diagnostic session, full-state context sheet

> Agent A (Task 2-A), Phase-0 reconnaissance. READ-ONLY audit of `/home/z/my-project/final-prod` at HEAD `ffd1e0c2` ("perf(T1): session opens — prompt archived token-redacted, hot-path recon recorded, prior PRODUCT-STATE archived to context-sheets"), VERSION `0.9.3`. Date: 2026-11-18. Scope: the performance lens — read path, threading, queue depth, buffers, bench/CI reality, prior perf state. Every claim cites `file:line` from the tree; UNVERIFIED marks what could not be determined statically. No source files modified; this sheet is the session's only created file.

## 1. THE READ PATH END TO END

### 1.1 Windows transport (engine/src/win/cr_transport.cpp — 887 lines)

**Open sites/flags.** Every CreateFileW opens `FILE_ATTRIBUTE_NORMAL`; **no site uses
FILE_FLAG_OVERLAPPED or FILE_FLAG_NO_BUFFERING** (checked at every site): `open_path` =
GENERIC_READ, share R|W|D, OPEN_EXISTING (:291-293); `open_rw_device` =
GENERIC_READ|GENERIC_WRITE share R|W|D (:732-734); `reopen_handle` keeps the caller's
access/share (:240-241); volume-side opens at :336-338 / :428-430 / :803-804 / :861-862
(the latter FILE_FLAG_BACKUP_SEMANTICS). The "FILE_FLAG_NO_BUFFERING semantics" phrasing
in cr_directalign.h:5-6 describes raw-handle stack behavior, not an open flag.

**overlapped_read() is BLOCKING despite the name.** ReadFile with OVERLAPPED; on
ERROR_IO_PENDING → `GetOverlappedResult(h, &ov, &got, TRUE)` — bWait=TRUE; success =
got==len (true byte counts, driver-era F11 scaling gone) (cr_transport.cpp:53-80). No
completion port, no ring, no second in-flight read.

**The 64 KiB envelope (GAP-S6-13).** `kDirectAlignBytes = 64*1024`
(cr_directalign.h:33). `direct_envelope` = floor64k(off)/ceil64k(off+len) clamped at
device end + skip/take; `direct` fast path = aligned offset, 64 KiB-multiple length,
unclamped → read into the caller's buffer, no copy (cr_directalign.h:48-73).
`aligned_read_at` (:191-215): direct → no-copy read (:197-198); else bounded bounce
refusing issued_len > 1 MiB+64 KiB (:199-203), `thread_local std::vector<u8> bounce`
resized per call (:204-208), memcpy out (:211-213). Origin: bench-run-40073093997 sector
rejection + GAP-S6-13 4-KiB-multiple slow path (~3.5x, ~830 vs ~2900 MB/s NVMe), both
cited in-tree (cr_transport.cpp:152-190); pure portable math, unit-tested
(test_directalign), both transports unified (PERFORMANCE-PROFILE.md:57).

**Device classes.** `DirectDevice::read_at` = bounds → envelope → self-healing reopen on
ERR_READ_FAILED+GLE==ERROR_INVALID_HANDLE under `heal_mu_`, one attempt, original open
parameters, geometry not re-probed (:264-277, :217-249). `RwDevice::read_at` routes the
SAME envelope (M-009+core-api, :490-507); `write_at` stages sector-aligned buffers via
`_aligned_malloc` when unaligned (:622-659), heals the same way (:509-525).

### 1.2 CachedReader (cr_cached.cpp + include/cr/storage/cr_cached.h)

- **Mutex scope: the whole read incl. the transport fill** — lock at cr_cached.cpp:31,
  `inner_->read_at(...)` at :61. THE single-flight point.
- Block scan O(blocks) linear (:46-51); LRU move-to-front O(n) shift (:76-82, fine at
  capacity 32 — cr_cached.h:76-77).
- **Per-miss allocation**: fresh `Block` + `b.data.resize(blen)` per miss (:52-61);
  failed blocks NOT cached (audit row 38, :62-68); eviction pop_back (:93-94). No free-list/pool
  anywhere.
- Defaults `cache_bytes=1<<20, block_bytes=65536` (cr_cached.h:54-55); the scan pipe
  overrides **8 MiB / 256 KiB** (cr_scan.cpp:1387) = 32 blocks. 8-vs-4 MiB + reactive-LRU-vs-og-prefetch
  = REGISTERED divergence (N-045, D-72; cr_cached.h:9-36). Counters →
  ScanStats (:1595-1596).

### 1.3 Device abstraction (include/cr/storage/cr_device.h)

`IStorageDevice::read_at(offset, buffer, length)` — byte-granular, bounded, "safe for
calls from engine worker threads" (:119-128); PartitionSlice bounds engine views
(:8-10); Geometry sector fields + CEIL count N-206 (:43-63); RetryPolicy (3 attempts,
Skip zero-fill, backtrack 0x400) (:98-116); CancelToken/PausableToken (:144-242, §2.5);
NAS/remote excluded — D-88/M-034 (:16-23).

### 1.4 Scan pipe construction + phase model (cr_scan.cpp — 3295 lines)

**Stack (one site, M-058):** slice → SliceShim → RetryingDevice (3 attempts, Skip =
surface + zero-fill, never abort) → CachedReader(8 MiB, 256 KiB) — build_read_pipe
(cr_scan.cpp:191-230; constants :206-208, :1387). Degraded path = bare slice, pinned
contract (:194-202). RetryingDevice: attempts → N-004 backtrack → bisection →
ReadErrorMap + zero-fill (cr_retry.cpp:56-136).

**Phases** (weights 40/35/25, cr_scan.h:72-75; gauge restarts at boundaries, D-P6,
cr_scan.cpp:131-173):
- **quick** = one engine pass (run_quick :1679-1698); with deep configured the pass
  already uses deep settings — SINGLE shared walk (:1681-1683). Reads: per-MFT-record
  `read_record` (record_size, typically 1 KiB — cr_bootstrap.cpp:599-635;
  cancel/progress per record, cr_mftwalk.cpp:481-488). **Batching exists only at the 256
  KiB cache block** — the og batches MFT at 2 MiB (cr_scan.cpp:1373-1375; D-51).
- **deep** = same pass + remnant sweep + lost discovery (run_deep :2239-2298). Remnant
  sweep: **4 MiB blocks** (REMNANT_BLOCK=4ull<<20, cr_mftwalk.cpp:231; read :2307-2311;
  block-buffered scan :1742-1746), parallel chunk fan-out (chunk = total/threads,
  :2368-2378; invoked with `threads`, cr_scan.cpp:2290-2293). Lost discovery: **64 KiB
  buffered signature window** (cr_candidates.cpp:32, fill :54), stride_sectors default 1
  (:962-963; AUDIT-PIPELINE.md:108), candidates over `ThreadPool pool(threads)`
  (cr_scan.cpp:2473-2487).
- **carve** (run_carve_stage :2607-2726; threads = resolve_threads :2609): **WINDOW = 1
  MiB, OVERLAP = 4096** (cr_carve.cpp:45-46); window_count = ceil(total/WINDOW) (:1612);
  serial when 1 thread (:1615-1628) else window fan-out over `ThreadPool
  pool(flags.threads)` + CancelPoolWatcher (:1630-1678). Per window: fresh
  `std::vector<u8> win` ≤ 1 MiB+4 KiB (:1427-1431), ONE read_at fill (:1439), then the
  M-025 window-serving context serves in-window reads from the buffer, zero device
  traffic (cr_carve.cpp:467-479); tail-scan overlap re-reads (:1255-1266). Carve runs
  before deep when quick completed (:1550-1576).
- Post-scan: grading deep-reads — "One sector per not-in-table no-twin node, content-gated"
  (cr_scan.cpp:1623-1630); read-truth completion gate (D-DRV-6) :1632-1675.

### 1.5 Other read consumers

- **SafeWriter**: source read granularity **1 MiB** default (cr_safewriter.h:158; used
  cr_safewriter.cpp:543-544, :576-577); host override "0 = 1 MiB" (cr_ui.h:1484 →
  cr_ui.cpp:4456-4457); no scan overlap by construction (separate handle/lifetime).
- **Preview**: kMaxRead = **4 MiB** cap per call (cr_preview.h:59; clamps
  cr_preview.cpp:24-25, :116-117); kMaxDecode 256 MiB (:61). Probe TTFB measured 0.0 ms
  (PERFORMANCE.md:180).
- **Health**: pure DeviceIoControl control path (ATA passthrough :68, legacy
  SMART_RCV_DRIVE_DATA :114, CSMI :206, SCSI passthrough :270 in cr_health.cpp) — no
  read_at stream reads.
- **Image create/verify**: 64 KiB chunks (cr_image.cpp:1560-1581; F6 disposition PERF-LEVERS.md:36).
  **Session save**: measured 9.3 ms (PERFORMANCE.md:174; checkpoint path
  §2.6).

## 2. THREADING MODEL

### 2.1 The worker pool (cr_parallel.cpp)

`ThreadPool(0)` → auto_workers = hardware_concurrency()-1 clamp 1..8 (:13-20); hard cap
64 (:26-27); FIFO queue + cv; `abort()` empties + wakes (:78-87); `parallel_for` =
workers + calling thread pulling a shared atomic counter, first-error wins + abort
(:104-150). `CancelPoolWatcher` polls the token every **50 ms** → pool.abort()
(:152-179).

### 2.2 Where scans run

`cr_ui_scan_run`/`cr_ui_recovery_process` run SYNCHRONOUSLY on the CALLER's thread (ABI-CONTRACT.md:57-59);
the engine pass is serial on that thread (cr_mftwalk.cpp:481-488);
parallelism is per-stage only (remnant/lost/carve, above). The Tauri host launches a
dedicated run thread + spawn_blocking prep (ui/src-tauri/src/commands/scan.rs:8, :102,
:108). **Nothing engine-side runs on a UI thread**; cr_ui has no dispatch thread pool —
its only extra thread is the heartbeat watchdog (std::thread, 5000 ms, cr_ui.cpp:2381,
:1145; ABI-CONTRACT.md:349-357). `resolve_threads` (cr_scan.cpp:58-68): 0 → hw-1 clamp
8; 1 → serial; >64 → 64; ScanConfig default threads=0 (cr_scan.h:209).

### 2.3 Lock inventory (read path + result/query)

| Lock | Scope | Cite |
|---|---|---|
| CachedReader::mtx_ | **one mutex serializes ALL pipe reads incl. the transport fill** | cr_cached.cpp:31, :61; comment cr_transport.cpp:186-189 |
| RetryingDevice::map_mutex_ | error-map add/merge only | cr_retry.cpp:33, :38, :112 |
| DirectDevice/RwDevice heal_mu_ | dead-handle reopen only | cr_transport.cpp:287/:712; taken :272/:502/:520 |
| ResultTree::mutex_ | **ONE GLOBAL mutex per tree — every add_root/add_folder/add_file/snapshot/rebase locks the whole tree, not per-subtree** | cr_node.h:656; cr_node.cpp:133-577 |
| carve group_mtx + ctx.lock | volume-group list + tree attach | cr_carve.cpp:294, :261, :552 |
| ThreadPool mutex_ | task queue | cr_parallel.cpp:35-100 |
| ScanController tel_.m | telemetry snapshot/counts/stage | cr_scan.cpp:479-761 |
| CrScan ctrl_mu/progress_mu/heartbeat_mu | shim control / single-flight progress / heartbeat | cr_ui.cpp:848, :880, :903; :1200-1206 |
| cr_usersigns g_mu / cr_log g_mu / image mu_ | caches | cr_usersigns.cpp:333; cr_log.cpp:25; cr_image_win.cpp:107 |

**Query path:** `query::run` walks `tree.roots()` const with NO lock
(cr_query.cpp:228-243; no mutex in cr_query.cpp) — safety rests on the ABI one-thread-per-handle
discipline + G-02 immutable value snapshots (ABI-CONTRACT.md:65-68).

### 2.4 Queue depth — the ABI contract

Outstanding reads per device = **1, synchronous**: blocking
GetOverlappedResult(...,TRUE) (cr_transport.cpp:70) + the cache mutex covering the fill
(cr_cached.cpp:31/61). In-tree statement (cr_transport.cpp:186-189): *"Threading: reads
through the engine pipe arrive serialized under the CachedReader mutex; per-handle
single-flight is the ABI contract (ABI-CONTRACT section 3), so the shared HANDLE is safe
here."* Normative text ABI-CONTRACT.md §3 (:57-68): *"cr_ui_scan_run and
cr_ui_recovery_process run SYNCHRONOUSLY on the CALLER's thread … One handle, one
controlling thread … All other entry points: call from one thread at a time per
handle."* Consumers are "QD1 by design" (the og itself serializes QD1 —
worklog/PERFORMANCE.md:131-132; PERF-LEVERS.md:32).

### 2.5 Cancellation + pause

Token functions from any thread (cr_ui.cpp:2443-2459; ABI-CONTRACT.md:60-62). Engine
loops poll `checkpoint()` cooperatively: per MFT record (cr_mftwalk.cpp:482), per carve
window AND every 4 KiB inside a window (cr_carve.cpp:1639; D4c :622), per 4 MiB remnant
block (cr_mftwalk.cpp:2307); pools aborted within 50 ms (cr_parallel.cpp:166).
**Transports do NOT poll the token** — "an in-flight read completes or fails on its own"
(cr_device.h:144-148, F-B-16); measured impact: cancel-to-stop **282.6 ms**
(PERFORMANCE.md:179). PausableToken blocks in checkpoint() while paused
(cr_device.h:219-223).

### 2.6 Session checkpoint path

Stage-boundary only (G-13: cr_scan.cpp:1539, :1572, :1586); `emit_checkpoint_` writes a
fresh 'RKPP' container with atomic-rename commit (N-014, :905-949); sink wired by the
cr_ui shim (tmp path + commit hook, cr_ui.cpp:2340-2350; staging cleanup :2392-2404).
Not interval-based; boundary latency measured fine (9.3-10.8 ms, PERF-LEVERS.md:41).

## 3. BUFFER MANAGEMENT (hot-path per-read allocations)

- CachedReader: one fresh Block + 256 KiB vector per miss (cr_cached.cpp:52-61), freed
  at eviction.
- Transport bounce: thread_local vector resized per unaligned request, ≤ 1 MiB+64 KiB
  (cr_transport.cpp:199-208).
- Carve: per-window vector ≈ 1 MiB+4 KiB per worker (cr_carve.cpp:1427-1431); tail-scan
  overlap re-reads (:1255-1266).
- SafeWriter: per-item 1 MiB chunk vector (cr_safewriter.cpp:543-544).
- RwDevice staged writes: _aligned_malloc per unaligned write
  (cr_transport.cpp:627-632).
- MFT walk: reused per-record vector (cr_mftwalk.cpp:480-488); compressed-unit buffers
  (:299-340); GUID-dump chunks ≤ 0x8000 sectors × sector_bytes + stride slack
  (:1459-1477).
- No buffer pool anywhere; every layer allocates per-call/per-miss. Allocator cost vs
  wall: UNMEASURED (PERF-LEVERS.md:9-19 frames all transport-side micro-costs as
  microseconds against hundred-second walls).

## 4. THE BENCH + CI REALITY

### 4.1 The bench harness

- **bench/README.md** names `.github/workflows/test-benchmark.yml` as the runner ("the
  CI rebuild's 3-lane consolidation (8943993) replaced the old comparative-bench.yml",
  :7-12). **test-benchmark.yml does NOT exist in the live root `.github/workflows/`** —
  present: bridge-integration, contracts-ci, engine-ci, forensic-repro, release, ui-ci,
  ui-e2e, validation-evidence; engine/.github/workflows is absent too ("pruned as
  redundant subtree CI", engine-ci.yml:5-7) while engine/.github/scripts/ (Run-Benchmark.ps1,
  fetch-release.ps1, New-TestDisk.ps1, Teardown-Benchmark.ps1,
  api_harness.c) survives. engine/docs/CI-CURRENT.md:15-23 still documents
  checks.yml/release.yml/test-benchmark.yml/benchmark-src.yml/installer.yml as CURRENT,
  and benchmarking.md:12-24 describes both benchmark lanes — a live doc-tree mismatch at
  HEAD (the rebuild folded driver-install/core-install/staged-scenarios INTO test-benchmark.yml,
  PLAN-CI-REBUILD.md:84-105 — the lane the root now lacks). **No
  dispatchable bench workflow exists at HEAD**; bench entry points are manual Windows
  runs of bench/ours/*.ps1 and `rrecov bench` (benchmarking.md:29-46).
- **run-alignbench.ps1** sweeps: 4-KiB-vs-64-KiB demo (128 vs 132 KiB, NO_BUFFERING,
  QD1, :301-307); read-size × QD sweep [64 KiB, 256 KiB, 1 MiB, 4 MiB, 7 MiB] × QD
  [1,2,4,8] (:309-320) — **QD>1 achieved only in the bench via a FILE_FLAG_OVERLAPPED
  ring on a separately opened handle** (:115-126, ring :155-193; production never does);
  cr_ui cheap-op latency (:322-328); OQ-S4-3 chunk-strategy verdict (og 4/7 MiB vs core
  64 KiB/256 KiB + CPU, :340-367), "UNVERIFIED-BY-EXECUTION until the Windows lane runs
  it" (:24-25). 512 MiB span/cell, two passes, hard gates (:27-41).
- **bench/baseline.json** (schema 2): scenarios empty (:4); 10 driver-parity perf rows
  from run 41042267586 with tolerances — seq 1 MiB ≥ 536.92 MiB/s, rand ≤ 1993.22 µs,
  open ≤ 17,019 ns, quick ≤ 150 ms, deep ≤ 16,505 ms, deepcarve ≤ 1,501,622 ms, api rows
  (:8-17). **baseline-candidate.json** (schema 1, run 40073093997-smoke): walls quick
  0.51-0.53 s, deep 2.01-8.03 s, deepcarve 41.98-90.06 s per 3 GiB volume (:5-52).
- **docs/PERFORMANCE-BASELINE.md**: the Azure sweep (run 40227375329, windows-2022,
  \\.\C:): plateau ~240 MiB/s flat; **alignment ratio 1.03× — "the documented ~3.5× does
  NOT manifest on the runner's virtualized C: volume … the mandate's 830-vs-2900 MB/s
  figure is mid-range NVMe hardware, not Azure VM storage"** (:84-86); "every size ≥256
  KiB saturates it at QD1 and QD>1 buys nothing beyond noise … validates keeping the og-matched
  synchronous QD1 model … and 256 KiB cache blocks" (:97-102). §5 profile:
  engine 256 KiB (≈4,096 transitions/GiB), carve 1 MiB, writer 1 MiB, og 4 MiB/2 MiB
  (256-512/GiB) (:62-68); §2 driver-era rows historical (:18-37). **docs/PERFORMANCE-PROFILE.md**
  (session-6 historical, N-020 note :3-6): local Linux suite 225 tests /
  3.04 s; query 1M-node tree 362 ms cold (:20-22); GAP-S6-13 landed state (:53-61).
  **docs/benchmarking.md**: two lanes (neither in root workflows, :12-24); `rrecov
  bench` verb (:29-46); baseline of record (:59-63); N-045 intent + doc-truth pin
  (:81-89); 500 ms process sampling + cancel_to_stop_ms + preview probe_ms (:91-97).

### 4.2 CI lanes at HEAD

- **engine-ci.yml**: 2 jobs, both windows-2022 (:77-78, :252-254); push/PR (docs-filtered)
  + workflow_dispatch (:43-61). Job 1 = clang-format, N-041 dead-surface +
  direct-access lints, PS AST gate, workflow YAML gate, doc-truth gate M-088
  (:241-249). Job 2 = MSVC /W4 /WX Release build (win-x64-release, vcpkg cache :281-293)
  + full ctest (:295-300) + Win7 posture (:304-319) + G-09 schema (:324-334) + elevation
  refusal probes (:343-360) + CLI junk-gate (:388+). **No performance measurements in
  any live lane.** Header records the windows-only consolidation (ubuntu sanitizer/fuzz
  dropped by owner directive; covered locally via ci-local.sh --san/--fuzz, :9-14).
- **forensic-repro.yml**: workflow_dispatch only (:34-45), windows-2022 (:58) — symptom
  reproduction on an installed release; no perf steps. **release.yml**: tag push vX.Y.Z
  + dispatch (:21-31), windows-2022 — build + artifact gates; no bench phase.
- **The Linux lane (this container, feasibility facts):** presets linux-gcc (ASan,
  CR_BUILD_APP=OFF) / linux-release / linux-ubsan (CMakePresets.json:14-31, :55-66); ci-local.sh
  runs 10 components incl. linux-release build + ctest (:107-126) + optional
  TSan/UBSan/fuzz (:202-226). Deps: zlib REQUIRED (CMakeLists.txt:48-53;
  /usr/include/zlib.h present); lz4+zstd REQUIRED (:56-147; liblz4.so + libzstd.so in
  ldconfig); vcpkg NOT used on Linux; vcpkg.json (zlib/gtest/lz4/zstd, :4-9) is the
  Windows manifest. gtest NOT installed locally → FetchContent googletest v1.15.2 from
  GitHub at configure (CMakeLists.txt:170-178). **Configure probe PASSED here** (cmake
  4.4.5, g++ 14.2, 2 vCPU, no sudo; tests-ON configure clean in 2.9 s). A prior
  build/linux-release cache exists with CR_ENABLE_TESTS=OFF + libcr_core.a built. Full
  build not attempted (session constraint).

## 5. STATE + PRIOR PERFORMANCE WORK (do not re-litigate)

### 5.1 Registered perf-relevant divergences (engine/docs/PARITY-DELTAS.md)

- **D-48c** — 64 KiB direct-I/O envelope (GAP-S6-13, ACCEPTED): "ours issues 64 KiB-aligned
  direct reads with a zero-copy fast path (~3.5x on the slow-path shapes)"
  (:1164-1168).
- **D-51** — "engine cache blocks at 256 KiB (og batches 2-4 MiB) … 4,096
  transitions/GiB vs the og's 256-512 … The alignbench sweep owns further tuning"
  (:1229-1235).
- **D-72** — scan cache: 8 MiB reactive LRU vs og 4 MiB prefetch read-ahead (N-045; og:
  dedicated prefetch thread, 10-run look-ahead, CRITICAL_SECTION; "aligning the budget
  to 4 MiB or introducing a prefetch thread is a benchmark-gated tuning decision … not a
  parity fix"; pinned by `Cached.ScanCacheBudgetIsTheRegisteredDivergence`)
  (:2287-2309).
- **D-78** — push events + telemetry pull vs the og's poll timer farm (N-075,
  REGISTERED) (:2505).
- **D-88** — NAS/remote excluded from core device scope (M-034) (:3054;
  cr_device.h:16-23).
- **D-DRV-6** — read-truth completion gate: fake scans refuse where the og fake-completes
  (:3318-3339; cr_scan.cpp:1632-1675).
- **D-104** — user-signature carve channel superset, rejection tracking bounded at 2^20
  (:3341-3354). D-number errata: 75/78/79/80 collisions (:35-59) — check before citing.

### 5.2 CI state at HEAD + the prior sessions' record

- **CI state:** engine-ci.yml:9-14 records the windows-only consolidation; green
  evidence for the CURRENT root lanes is UNVERIFIED in-tree (no run artifacts committed
  at this HEAD; the committed evidence trees are installer/UI-lane artifacts —
  docs/INTEGRATION-VALIDATION/t15-runtime-matrix, engine/bench/vlm-evidence). The
  benchmark-lane docs (CI-CURRENT.md, benchmarking.md, bench/README.md) describe the
  engine subtree's pre-consolidation CI, not the live root lanes.
- **engine/AUDIT-PIPELINE.md** (Refactor-F parity audit): carve threading "window fan-out
  + tree mutex + per-worker emitted_at — ours-only, no og decode" (:83); lost stride
  scan "64 KiB windows … (stride param default 1)" (:108); PDF validator 1 MiB block
  reads (:42); open items CARVE-1..11 are quality, not transport (:87-100).
- **worklog/PERFORMANCE.md** ("no performance claim survives without a captured
  measurement"): §1.1 baseline of record run 41042267586 (driver v0.6.1, 8 GiB 512e
  VHDX, windows-2022): seq 1 MiB 536.92 MiB/s; rand 1993.22 µs/502 IOPS; open 17,019 ns;
  quick 150 ms; deep 16,505 ms; deepcarve 1,501,622 ms; API open 0.9 / scan 1,513,221.7
  / save 10.8 / recovery 49.2 ms (:48-57; budget-hit correction :40-47). §1.2 driver-era:
  39807929475 (driver-vs-og: seq 64 KiB 532.9 vs 533.1 MiB/s; rand 4 KiB 7,932 vs
  8,170 IOPS); 40227375329 (plateau + the one direct-vs-driver gap −10.5% @256 KiB,
  since retired); 40315255368 (OQ-S4-3: og 4/7 MiB 197.32/196.15 vs core 64 KiB 203.98 /
  256 KiB 241.64 MiB/s, +18-19% at lower CPU); 40228880804 (API 10.7-166.3 ns/op)
  (:85-119). §2 direct-era v0.7.0 (runs 41140212296 + 42012118585): seq 1 MiB 554.83
  MiB/s (+3.3%); 64 KiB 494.24 / 256 KiB 551.65 / 4 MiB 556.83; rand 4 KiB 125.3 µs;
  open 10,058 ns (−40.9%); deep 16,257 ms (parity); deepcarve 1,040,054 ms (−30.7%); API
  scan −22.3%; CPU/GiB −39.4%; deepcarve CPU 4,014.9 CPU-s at 386% avg of 4 threads; WS
  245.4 MiB; cancel-to-stop 282.6 ms (:151-181); first live gate run 9/10 PASS
  (:183-192). §2.2 red-run analysis: recall-0 floors predate the direct path; stale
  `transport=='rda'` gate; S-PART-DELETE unclosed-comment bug now guarded by
  `-Werror=comment` on the Linux lane (:194-225).
- **worklog/PERFORMANCE-TARGET.md**: verdict algebra (noise ±3% throughput/±10% latency;
  3-run medians, :20-33); headline gate table (:43-61); the ONE open GAP — scenario deep
  wall S-DEL-NTFS 7.56→8.10 s (+7.1%, single-sample; levers F3/F1) (:73-76); 15
  UNMEASURED consumer rows (ext4/Btrfs/ReFS/HFS+/ZFS walls, LDM, BitLocker, lost wall,
  repair/apply walls, preview/thumbnail TTFB, image throughput) (:78-90).
- **worklog/PERF-LEVERS.md** (binds this session): wall math — deepcarve 8.45 MiB/s
  effective vs transport 544 MiB/s — "**the walls are validation-CPU-bound, not
  transport-bound**" (:9-19); F1 (mutex fill-outside-lock → effective QD>1) DEFERRED —
  "unvalidatable on the CI runner class … revisit trigger: a physical-NVMe runner
  profile or a QD-sensitive fixture" (:31); F2 (OVERLAPPED + slots) DEFERRED (:32); F3
  coalesced fills NOT TAKEN (6 ms/GiB vs 124 s/GiB, :33); F4 sector-granular envelope
  NOT TAKEN (output change on sick disks, :34); F10 prefetch thread NOT TAKEN —
  "measured superior without it" (:40); §3 what WOULD move the walls: carve/validation
  CPU (303.8 CPU-s/GiB), matcher vectorization, validation batching, early-exit — with
  output identity proven per change (:54-60).
- **Origins not to re-litigate:** the 64 KiB envelope (GAP-S6-13 mandate,
  cr_directalign.h:4-16, landed 7777e1c per PERFORMANCE-PROFILE.md:5-6); the N-045 cache
  divergence (cr_cached.h:9-36); the driver-era rows above.

## 6. SESSION-RELEVANT SYNTHESIS (current-state parameters)

| # | Parameter | Value | Cite |
|---|---|---|---|
| 1 | Transport read issued (engine path) | 256 KiB cache-block fills, 64 KiB-envelope aligned | cr_scan.cpp:1387; cr_cached.cpp:57 |
| 2 | Envelope | 64 KiB: offset-down/length-up, tail clamp, no-copy fast path | cr_directalign.h:33-73; cr_transport.cpp:191-215 |
| 3 | Open flags | GENERIC_READ[+WRITE], share R|W|D, NORMAL; NEVER OVERLAPPED/NO_BUFFERING | cr_transport.cpp:291-293, :732-734 |
| 4 | Read wait | synchronous GetOverlappedResult(...,TRUE) | cr_transport.cpp:70 |
| 5 | QD per device | 1 (single-flight; cache mutex covers the fill) | cr_cached.cpp:31+61; ABI-CONTRACT.md:57-68 |
| 6 | Scan cache | 8 MiB / 256 KiB / 32 blocks / reactive LRU / no prefetch (D-72) | cr_scan.cpp:1387; cr_cached.h:9-36 |
| 7 | MFT walk | per-record (1 KiB class); batching only at the cache block | cr_bootstrap.cpp:599-635; cr_mftwalk.cpp:481-488 |
| 8 | Remnant sweep | 4 MiB blocks, parallel fan-out | cr_mftwalk.cpp:231, :2368-2378 |
| 9 | Lost discovery | 64 KiB window, stride default 1 sector | cr_candidates.cpp:32, :962-963 |
| 10 | Carve | 1 MiB + 4 KiB windows; pool fan-out; window-buffer serving | cr_carve.cpp:45-46, :1631, :467-479 |
| 11 | Writer / preview / image | 1 MiB / 4 MiB cap / 64 KiB chunks | cr_safewriter.h:158; cr_preview.h:59; cr_image.cpp:1560 |
| 12 | Threads | run() synchronous on caller thread; pools hw-1 clamp 8; engine pass serial | cr_scan.cpp:58-68; ABI-CONTRACT.md:57-59 |
| 13 | Read-path locks | ONE cache mutex (+ retry map_mu, heal_mu on dead handle) | cr_cached.cpp:31; cr_retry.cpp:33; cr_transport.cpp:287 |
| 14 | Result tree lock | ONE GLOBAL mutex per tree (not per-subtree); query lock-free under ABI + snapshots | cr_node.h:656; cr_query.cpp:228-243 |
| 15 | Hot-path buffers | per-miss Block; thread_local bounce ≤1 MiB+64 KiB; per-window 1 MiB+4 KiB; per-item 1 MiB; no pools | cr_cached.cpp:52-61; cr_transport.cpp:204-208; cr_carve.cpp:1427-1431; cr_safewriter.cpp:543-544 |
| 16 | Cancellation | cooperative checkpoint per record/window/4 KiB/4 MiB; pools aborted ≤50 ms; transports never poll; measured stop 282.6 ms | cr_device.h:144-148; cr_parallel.cpp:166; PERFORMANCE.md:179 |
| 17 | Bench/CI | no live perf lane at HEAD; alignbench manual-only; engine-ci = build+ctest only | §4 |
| 18 | Linux lane | buildable here (configure proven; zlib/lz4/zstd system libs; gtest via FetchContent; 2 vCPU) | CMakeLists.txt:48-178; §4.2 |
| 19 | Binding verdict | walls are validation-CPU-bound (8.45 MiB/s effective vs 544 MiB/s transport); F1-F14 dispositioned | PERF-LEVERS.md:9-19, :29-44 |
| 20 | The one open GAP | S-DEL-NTFS deep wall +7.1% single-sample (3-run median pending) | PERFORMANCE-TARGET.md:73-76 |

**Bottom line:** the product is a synchronous, QD1, single-mutex-serialized read
pipeline whose transport already meets/exceeds the driver-era baseline on the only
runner class available; measured walls are CPU-bound in carve/validation, not transport-bound
(PERF-LEVERS.md:9-19). Any overlapped/IOCP/QD>1 proposal must (a) re-litigate the
F1/F2 deferrals with new hardware evidence, and (b) respect the registered divergences
D-48c/D-51/D-72 (doc-truth-pinned).

## 7. NOT MEASURED — requires real Windows/hardware (no runner can measure)

- Consumer NVMe / SATA SSD / HDD / USB / SD per-transport classes — the only captured
  class is Azure virtualized storage (~240 MiB/s plateau; "the mandate's 830-vs-2900
  MB/s figure is mid-range NVMe hardware, not Azure VM storage", PERFORMANCE-BASELINE.md:86).
  
- The ~3.5× 4-KiB-multiple slow-path ratio on real NVMe (recorded 1.03× on the runner;
  envelope stays mandated by construction, PERFORMANCE-BASELINE.md:84-99).
- QD>1 production payoff (F1/F2 DEFERRED for exactly this: "no measurable payoff exists
  on any current surface … revisit trigger: a physical-NVMe runner profile", PERF-LEVERS.md:31-32).
  
- Queue-depth plateau per device class — the QD legs are bench-harness-only shapes (run-alignbench.ps1:11-13);
  cancel-to-stop on slow physical media (282.6 ms is the in-flight-read
  window on a fast runner volume, PERFORMANCE.md:179).
- The 15 UNMEASURED consumer rows: ext4/Btrfs/ReFS/HFS+/ZFS walls, LDM, BitLocker
  decrypt, lost-partition wall, repair/apply walls, preview/thumbnail TTFB, image
  throughput (PERFORMANCE-TARGET.md:78-90).
- Direct-vs-driver historical gap on physical media (retired on the runner class by run
  42012118585's 256 KiB leg 551.65 MiB/s vs the old 151.5, PERFORMANCE.md:163 — still
  only that class).
- Any Windows-runtime claim from this container: the audit is static; the engine's win/
  transport cannot run here (linux presets build no app lane, CMakePresets.json:20-21).
