# BASELINE — reference-vs-product, side by side (Agent E, task 2-E)

> Session: performance-diagnostic (2026-11-18). Repo HEAD `ffd1e0c2` (clean tree;
> no product code touched by this task). Provenance rule for every number below:
> **(CITED)** = transcribed from the repo's recorded measured evidence with its
> file+section and run ID; **(MEASURED-HERE)** = produced by a command run in
> this Linux container during this task (component-level only, commands and
> environment recorded); **(NOT-MEASURED-requires-Windows)** = explicit hole with
> the exact procedure to obtain it (Section 5).
>
> Era labels: **DRIVER-ERA** = the kernel-driver transport (session-96 removed
> it; rows are historical records), **DIRECT-ERA** = the current direct raw-I/O
> transport (`\\.\PhysicalDriveN` opens, v0.7.0+), **COMPONENT** = engine-internal
> or container-side measurement, not end-to-end on media.
>
> Standing caveat (the repo's own): the GitHub-hosted windows-2022 runner's
> storage is **virtualized** (VHDX on the runner's physical media; the `\\.\C:`
> sweep plateaus at ~240 MiB/s QD-flat) — its numbers are runner-class-relative
> truth, NOT consumer-NVMe truth. Sources: `engine/worklog/BENCHMARK.md` §6
> ("The runner's VHDX is a virtual disk on the runner's physical media —
> absolute throughput numbers are runner-class-relative"), `engine/docs/
> PERFORMANCE-BASELINE.md` §6 leg 2 ("the runner's C: caps at ~240 MiB/s ...
> the mandate's 830-vs-2900 MB/s figure is mid-range NVMe hardware, not Azure
> VM storage").

---

## Section 1 — Environments

| Environment | Hardware / OS | Toolchain | What it can measure | Status |
|---|---|---|---|---|
| **This container** (Agent E) | 2 vCPU "Intel(R) Xeon(R) Processor" (masked model), 4,159 MiB RAM, Linux 5.10.134-013.15.kangaroo.al8.x86_64, overlayfs root (disk-backed, no tmpfs on /tmp), no Windows, no sudo (cannot drop page cache) | gcc 14.2.0 (Debian) — the same gcc major the repo's local lane documents; cmake 4.4.5 installed via pip into `/home/z/.venv` (the image ships none) | engine build feasibility; component microbenchmarks of `cr_cached` (the cache-layer design); the unit/QueryPerf fixture gates | MEASURED-HERE (Section 4) |
| **windows-2022 runner** (GitHub-hosted, 2 vCPU, Windows Server 2022 Datacenter 10.0.20348, AMD EPYC 9V74 2C/4T @ 2596 MHz, hosted, elevated by construction) — the class every recorded Windows number was captured on | 8 GiB 512e VHDX (8,589,934,592 B; 512 logical / 4096 physical; not 4Kn) is the headline disk profile; seeded 2/5/8 GiB VHDX matrix for scenarios | MSVC v143 + WDK 26100.6584 (driver era); MSVC v143 + vcpkg (zlib, gtest, lz4, zstd) for core | transport microbench, clean-volume scan walls, API lane, quality matrix, driver-vs-direct comparison | CITED (Sections 2-3); virtualized-storage caveat above |
| **Local container lane of the core repo** (prior sessions) | CI container, Linux, gcc 14.2 (`-O2 -Werror`; ASan preset) | same shape as this container | logic-shape baselines, unit suites, query-perf gates | CITED (Section 2.8) |
| **Physical consumer hardware (NVMe / SATA SSD / HDD / USB / SD)** | none — never measured anywhere in the repo | — | per-transport QD/read-size optima; real scan throughput; the ~3.5× unaligned slow-path ratio | NOT-MEASURED-requires-Windows (Section 5) |

Sources: `engine/docs/PERFORMANCE-BASELINE.md` §1 (environments table);
`engine/worklog/PERFORMANCE.md` §0 (runner class spec);
`/proc/cpuinfo`, `free -m`, `uname -a`, `stat -f /tmp` (this container, MEASURED-HERE).

---

## Section 2 — Ours-baseline (every recorded measured number, with source)

### 2.1 DRIVER-ERA transport microbench — our `rda` driver vs the original `eprdtdrv` (CITED)

Run **39807929475** (2026-10-22, windows-2022, 4 GiB seeded VHDX; side-by-side
`rdatest` installed as `eprdtdrv`). Source: `engine/docs/PERFORMANCE-BASELINE.md`
§2; `engine/worklog/PERFORMANCE.md` §1.2 (identical rows).

| Metric | Ours R2 (driver era) | Verdict recorded |
|---|---:|---|
| seq 64 KiB read | 532.9 MiB/s | parity+ (vs worklog narrative 533.1) |
| seq 1 MiB read | 458.6 MiB/s (R1) | −8.4% vs og 500.6 |
| rand 4 KiB | 7,932 IOPS | −2.9% (price of cancel-safety + DACL) |
| rand 4 KiB latency | 126.1 µs | +3.0% |
| device open (mean) | 8,217 ns | +2.2% = SDDL check cost |
| open churn (mean) | 8,532 ns | parity |
| hostile fuzz dispatch | 841,698 IOCTL/s | +52% |
| data correctness | 64/64 SHA-256 ranges byte-identical | exact |

Read-path context: 4 KiB-aligned buffers, `FILE_FLAG_NO_BUFFERING`, explicit
seeks, Information ÷512 rescale (`PERFORMANCE-BASELINE.md` §2 note).

### 2.2 The Azure sweep — legs 1-4 (CITED; the core-api session's first measured Windows transport numbers)

Run **40227375329** (alignbench, windows-2022, 512 MiB span per cell, two
checksum-gated passes, vcpkg-release build, driver service `rda` loaded from
the rda-driver v0.2.1 release; leg 4 from run **40228880804**, the first fully
green alignbench run). Source: `engine/docs/PERFORMANCE-BASELINE.md` §6;
`engine/worklog/PERFORMANCE.md` §1.2; `engine/bench/ours/run-alignbench.ps1`
(the in-tree harness).

**Leg 1 — 4-KiB-vs-64-KiB slow-path demonstration (NO_BUFFERING, QD1, `\\.\C:`):**

| Shape | Throughput |
|---|---:|
| 128 KiB chunks (64 KiB multiple) | 240.0 MiB/s |
| 132 KiB chunks (4 KiB multiple, NOT 64 KiB) | 233.6 MiB/s |
| ratio | **1.03×** — the documented ~3.5× does NOT manifest on the runner's virtualized C: volume (recorded honestly; the 830-vs-2900 MB/s mandate figure is mid-range NVMe, not Azure VM storage) |

**Leg 2 — read-size × queue-depth sweep (`\\.\C:`, production flag shape):**

| Chunk | QD1 | QD2 | QD4 | QD8 |
|---|---:|---:|---:|---:|
| 64 KiB | 208.6 | 233.1 | 231.1 | 235.5 |
| 256 KiB | 238.9 | 240.6 | 236.1 | 237.3 |
| 1 MiB | 224.4 | 241.9 | 241.2 | 241.3 |
| 4 MiB | 202.8 | 241.3 | 238.1 | 242.4 |

(mib/s). Repo's own read: the runner's C: **caps at ~240 MiB/s**; every size
≥256 KiB saturates it at QD1; QD>1 buys nothing beyond noise — validates the
og-matched synchronous QD1 model and 256 KiB cache blocks **on this storage
class only**; fast local NVMe must re-measure (weekly cron / dispatch).

**Leg 3 — RDA grammar-path round-trip (production grammar
`\\.\rda\<PID-token>\<disk>\<part>`, F11 Information ×512 verified on every read):**

| Config | Throughput | Reads |
|---|---:|---:|
| RDA 64 KiB QD1 | 201.9 MiB/s | 8,192 |
| RDA 256 KiB QD1 | 169.3 MiB/s | 2,048 |
| RDA 1 MiB QD1 | 160.5 MiB/s | 512 |
| direct `\\.\PhysicalDrive0` 256 KiB QD1 | 151.5 MiB/s | 2,048 |

**Leg 4 — cr_ui cheap-op API latency (late-bound, 2M iterations/op):**
`cr_ui_abi_version` **10.7 ns** · `cr_ui_last_error` **11.3 ns** ·
`cr_ui_carve_format_count` **12.6 ns** (count=174) · `cr_ui_carve_format_info`
**166.3 ns** (the registry confidence scan). Target <1 µs beaten ~100×.

### 2.3 DRIVER-ERA baseline of record — the 10 versioned gate rows (CITED)

Run **41042267586** (test-benchmark smoke on v0.6.1 `619acec2`, windows-2022,
disk 4, 8 GiB 512e VHDX, NTFS clean volume, 1209 live files + 2138 carved;
07-performance clean-volume section completed before the run's 120-min budget
hit — that budget caveat is recorded in `engine/worklog/PERFORMANCE.md` §1.1
and `engine/worklog/PERFORMANCE-TARGET.md` §0). Source:
`engine/bench/baseline.json` (perf.rows, with tolerances) = `engine/worklog/
PERFORMANCE.md` §1.1 = `engine/docs/benchmarking.md` §3.

| Row id | Value | Tolerance | What it gates |
|---|---:|---:|---|
| bench-seq-1m | 536.92 MiB/s | ≥ −3% | transport seq throughput (rda) |
| bench-rand-1m | 1993.22 µs mean / 502 IOPS | ≤ +10% | random-read latency |
| bench-open | 17,019 ns (58,757 ops/s) | ≤ +15% | transport open latency |
| scan-quick-wall | 150 ms | ≤ +10% | quick scan wall (clean vol) |
| scan-deep-wall | 16,505 ms (TTFR 0 ms; 1209 files) | ≤ +10% | deep scan wall |
| scan-deepcarve-wall | 1,501,622 ms (8,572,108,800 B scanned) | ≤ +3% | deep+carve wall (8 GiB) |
| api-open | 0.9 ms | ≤ +11% | cr_ui open |
| api-scan-run | 1,513,221.7 ms (mask 7, 4 threads) | ≤ +3% | cr_ui full scan |
| api-save-session | 10.8 ms | ≤ +10% | session save |
| api-recovery | 49.2 ms (82 files) | ≤ +10% | recovery leg |

(API query 0.2 ms and preview 4096/4096 from the same run's api-result.json —
`engine/worklog/PERFORMANCE.md` §1.1.)

### 2.4 DIRECT-ERA (v0.7.0) — the current transport's measured table (CITED)

Runs **41140212296** (v0.7.0 first capture; run red on scenario-lane gates but
the clean-volume phase valid) and **42012118585** (fixed-harness re-measure;
first capture of the sweep legs, CPU/WS, cancel-to-stop, preview probe).
Sources: `engine/worklog/PERFORMANCE.md` §2.1, §2.1.1; `engine/worklog/
PERFORMANCE-TARGET.md` §1 (the 15-row headline gate table).

| Metric | Driver baseline (2.3) | 41140212296 | 42012118585 | vs driver | Verdict |
|---|---:|---:|---:|---:|---|
| bench seq 1 MiB | 536.92 MiB/s | 544.17 | 554.83 MiB/s | +3.3% | EXCEEDS (2-sample) |
| bench seq 64 KiB | — | — | 494.24 MiB/s | (new leg) | captured |
| bench seq 256 KiB | — | — | 551.65 MiB/s | (retires run 40227375329's −10.5% direct-vs-driver concern) | captured |
| bench seq 4 MiB | — | — | 556.83 MiB/s | (new leg) | captured |
| bench rand 1 MiB | 1993.22 µs / 502 IOPS | 1976.72 / 506 | 1943.47 µs / 515 IOPS | −2.5% | EXCEEDS |
| bench rand 4 KiB | 126.1 µs / 7,932 IOPS (run 39807929475) | — | 125.3 µs / 7,979 IOPS | −0.6% | MEETS/+ |
| bench open | 17,019 ns | 9,213 | 10,058 ns | −40.9% | EXCEEDS |
| scan quick wall | 150 ms | 27 ms | (512 ms carried the sampler's 500 ms quantum — harness defect caught by the gate) | −82% | EXCEEDS |
| scan deep wall | 16,505 ms | 16,467 | 16,257 ms | −1.5% | MEETS/+ |
| scan deepcarve wall (8 GiB) | 1,501,622 ms | 1,014,235 | 1,040,054 ms | −30.7% | EXCEEDS (2-sample) |
| API scan (mask 7, 4 threads) | 1,513,221.7 ms | 1,113,445.2 | 1,176,096.2 ms | −22.3% | EXCEEDS |
| API open | 0.9 ms | 0.5 | 0.7 ms | −22% | EXCEEDS |
| API recovery (82 files) | 49.2 ms | 36.8 | 34.2 ms | −30.5% | EXCEEDS |
| session save | 10.8 ms | 9.3 | 9.4 ms | −13% | MEETS/+ |
| API query | 0.2 ms | 0.1 ms | — | — | MEETS/+ |
| TTFR (deep, clean vol) | 0 ms | 0 ms | — | — | MEETS (floor) |
| files found | 1209 + 2138 carved | 1209 + 2134 | 1209 + 2163 | shape-identical | MEETS (correctness floor) |
| scan CPU (deep) | — | — | 0.844 CPU-s / 16.3 s wall | I/O-bound | captured |
| scan CPU (deepcarve) | — | — | 4,014.9 CPU-s / 1,040 s wall (386% avg of 4 threads) | validation-bound | captured |
| scan WS peak (deepcarve, clean 8 GiB) | — | — | 245.4 MiB (scenario-lane 3 GiB: 52.0 → 53.7 MiB) | — | captured |
| cancel-to-stop | never measured | — | 282.6 ms (first capture) | — | captured |
| preview probe | never measured | — | 0.0 ms (sub-ms TTFB) | — | captured |
| CPU per GiB (scenario deepcarve, 2.990 GiB) | 501.2 CPU-s/GiB | 303.8 | (same binary; confirmed) | −39.4% | EXCEEDS |

Perf-gate first live run (42012118585): **9 PASS / 1 FAIL** —
`scan-quick-wall: observed 512 ≤ 165` was the gate catching the new sampler's
500 ms poll quantum (true quick wall ~27 ms by stopwatch); both samplers now
measure the process span (`PERFORMANCE.md` §2.1.1).

**The wall math that frames all optimization (CITED, `engine/worklog/
PERF-LEVERS.md` §0):** deepcarve wall 1,014,235 ms for 8,572,108,800 B =
**8.45 MiB/s effective** (driver era: 5.70 MiB/s) while the transport's
measured device rate at the consumer's block size is **544 MiB/s** — the scan
walls are **~64× below the device rate: validation-CPU-bound, not
transport-bound**. CPU per GiB: 303.8 CPU-s (4 threads) ≈ 76 s wall-equivalent
per GiB; deepcarve wall ≈ 124 s/GiB.

### 2.5 Historical scenario walls + quality matrix (CITED; pre-fix recorded state)

Run **40073093997**-smoke via `bench/metrics/make_baseline.py` →
`engine/bench/baseline-candidate.json` (schema 1, per-scenario walls and
recall/precision), plus the full matrix run **40079065019**. Sources:
`engine/bench/baseline-candidate.json`; `engine/docs/PERFORMANCE-PROFILE.md`
§2; `engine/bench/root-causes.json` (every row cites its run + artifact).

| Scenario (ours, recorded pre-fix) | Recall | Precision | quick / deep / deepcarve walls (3 GiB) |
|---|---:|---:|---|
| delete_partition | 43.0% | 19.81% | 0.51 s / 8.03 s / 90.06 s |
| permanent_delete | 87.2% | 30.3% | 0.52 s / 7.05 s / 59.87 s |
| quick_format | 0.0% | 0.0% | 0.53 s / 2.01 s / 41.98 s |
| exFAT native (delete family) | 99.94% | 99.16% | — (the healthy reference cell) |
| FAT32 native | 0–0.3% | — | — |
| ReFS native (all cells) | 0.0% (~2 ms ERR_BAD_MAGIC) | — | — |
| NTFS decay_50 / decay_80 | 96.4% / 0.2% | — | exFAT holds 98.6% at decay_80 |
| Recycle deletions (real volume) | 26/26 name recall (was 0/26) | — | — |
| Resume-equivalence probe | FAIL pre-fix | — | — |

Every red row is closed in code with a landing commit (root-causes.json
`resolution` fields: quick_format FIXED 7797bd6; delete_partition FIXED
03ddcd2; permanent_delete zero-fill FPs FIXED 03ddcd2; FAT32 FIXED 9fc82b0 +
N-087 8451913; ReFS FIXED 7797bd6; resume-equivalence FIXED cd9ea16+03ddcd2;
decay cliff CLOSED-AS-NOT-A-DEFECT with the physical-ceiling metric landed) —
**the recorded numbers predate the fixes and stand until the next bench
dispatch re-measures them** (`PERFORMANCE-PROFILE.md` §2 "Read of the table").
Engine-side historical perf fix: **M-117** — 220 MB contiguous TS fixture
**>600 s non-completion → 11.3 s** (carve-stage quadratic mid-stream re-walk
removed, tree `d7ba41f6`; `engine/diff-to-fix/tracked-open/
OQ-S4-3-chunking-relative-perf.md` §255-264; `engine/worklog/PERFORMANCE.md`
§1.2).

### 2.6 The current session's perf-relevant defect strata (CITED, from the repo-root `worklog/DIAGNOSIS.md`)

- **Stratum 1 (user's installed v0.9.1, tag cdf66ff8):** D4f — the carve
  user-sign probe took a registry mutex 2×/512 B stride over ~134 M zero
  strides → the **605 s all-axes freeze** ("stuck in sector scanning");
  fixed lock-free at `0e87689d`, validated engine-side, NEVER RELEASED
  (no tag after v0.9.1). `worklog/DIAGNOSIS.md` §0/§1 R7.
- **BF-30 — the carve throughput wall: 8.45 MiB/s (76-124 s/GiB)** —
  multi-TB scans take hours-to-days even with every D4 fix; registered as a
  performance defect, engine-side; the current session scopes "bounded BF-30
  perf work" (Phase 2) and `engine/worklog/PERF-LEVERS.md` §3 names the
  frontier: the carve/validation pipeline (303.8 CPU-s/GiB lives in the
  signature matcher and validators, not the transport).
- Residual wall note: BF-30's number is the same 8.45 MiB/s effective as
  §2.4's wall math (consistent across `DIAGNOSIS.md` §2 and `PERF-LEVERS.md`
  §0 — two sessions, one number).

### 2.7 Local container component numbers recorded by prior sessions (CITED)

`engine/docs/PERFORMANCE-PROFILE.md` §1 (HEAD `064fcf4`, session 6; and
`PERFORMANCE-BASELINE.md` §3 refresh): full unit suite **3.04 s wall** /
**8.46 s ASan** (~2.8× ASan overhead envelope); Query/filter on 1M-node
session tree **362 ms cold** (filter+count+page; 31-36 ms under the in-suite
ASan timing gate, 200 ms budget); engine fixture suites (NTFS+ReFS walks)
**22 ms** (synthetic fixtures — NOT volume-scale truth); suite size has grown
since (305/305 ASan at session-3 close).

### 2.8 Transport/I/O shape (design of record, CITED)

`engine/docs/PERFORMANCE-BASELINE.md` §5: RDA grammar path (scan engines via
CachedReader) issues **256 KiB blocks** (8 MiB LRU = 32 blocks) under the
**64 KiB unified envelope**, ≈4,096 transitions/GiB sequential; carve 1 MiB
windows ≈1,024/GiB; writer 1 MiB + exact tail; the og issues 4-8× fewer
transitions per byte (Section 3). Self-healing reopen on every device class
(correctness, no throughput claim). The `CachedReader` budget (8 MiB/256 KiB
reactive LRU) is a REGISTERED deliberate divergence pinned by the
`Cached.ScanCacheBudgetIsTheRegisteredDivergence` doc-truth test
(`engine/include/cr/storage/cr_cached.h` divergence block; measured superior
at the runner profile: direct path beats the driver's walls by 26-32%
without a prefetch thread — `engine/docs/benchmarking.md` §4).

---

## Section 3 — og-reference numbers (from the repo's comparative evidence; CITED)

All og-side numbers are historical recorded evidence; the og-comparison CI
lanes were retired ("the corpus's captured evidence is the reference" —
`engine/docs/benchmarking.md` §6) and the og product is not runnable in this
container.

### 3.1 og driver (eprdtdrv.sys) transport — run 39807929475 (CITED)

`engine/docs/PERFORMANCE-BASELINE.md` §2; `engine/worklog/PERFORMANCE.md`
§1.2 (same runner/VHDX as our driver rows in §2.1):

| Metric | Original eprdtdrv |
|---|---:|
| seq 64 KiB read | 533.1 MiB/s (391.0 in a later rerun — single samples on hosted runners can lie; 3-run median discipline exists for exactly this) |
| seq 1 MiB read | 500.6 MiB/s |
| rand 4 KiB IOPS | 8,170 |
| rand 4 KiB latency | 122.4 µs |
| device open (mean) | 8,042 ns |
| open churn (mean) | 8,537 ns |
| hostile fuzz dispatch | 553,752 IOCTL/s |

### 3.2 og chunk-strategy transport legs — run 40315255368 (CITED; OQ-S4-3)

`engine/worklog/PERFORMANCE.md` §1.2; `engine/bench/ours/run-alignbench.ps1`
header (the OQ-S4-3 sweep): **og 4 MiB / 7 MiB VOLUME chunking
197.32 / 196.15 MiB/s vs core 64 KiB envelope 203.98 MiB/s and 256 KiB
241.64 MiB/s** (+18-19% at lower CPU for the core; the core's RDA legs
194.34/237.54 MiB/s). The og's chunk sizes: 4 MiB device chunk
(Device_decompiled.c:56545), 7 MiB VOLUME chunk (:56912).

### 3.3 og read pattern + cache model (CITED, design-of-record from the decompiled corpus)

`engine/docs/PERFORMANCE-BASELINE.md` §5 (read shape):
**4 MiB device chunks; 2 MiB MFT batches; 1 MiB $UsnJrnl; 64 KiB boot —
256-512 transitions/GiB vs our ≈4,096/GiB** (the og issues 4-8× fewer
transitions per byte than our engine path).
`engine/include/cr/storage/cr_cached.h` (divergence block) +
`engine/docs/benchmarking.md` §4 (the og's `CStorageDeviceWithCache`):
**4 MiB budget**, a **prefetch read-ahead engine** — dedicated prefetch
thread, 10-run look-ahead window, 1000-sector default runs, CRITICAL_SECTION
locking, constructed once per physical disk and shared across partition
scans (DRWUI_decompiled.c:919080-:919092, ctor :962250-:962597). Ours: 8 MiB
budget, reactive true-LRU, std::mutex — REGISTERED divergence D-72/N-045,
measured superior at the runner profile (direct beats driver walls 26-32%
without the prefetch thread; `engine/worklog/PERF-LEVERS.md` F10 disposition).

### 3.4 og quality-side comparative numbers (CITED)

`engine/docs/BENCHMARK-REPORT.md` §1 + `engine/bench/root-causes.json`:

| Scenario | og measured | Ours recorded (same state) |
|---|---|---|
| Recycle deletions (26 seeded, real volume, run 39924932944 S5-CONT-2) | **26/26 name recall** | 26/26 post GAP-P1-3 (was 0/26) — MATCH |
| NTFS quick_format | **51.5% recall (305/592, smoke)** | 0.0% pre-fix (FIXED 7797bd6, re-measure pending) |
| NTFS delete_partition | **42.7% but CAP-limited** (Give-Up at 520.29 MB of the 512 MiB free quota while recovering partition A; partition B never attempted — the og free product's 500 MB cap) | 43.0% discovery-limited pre-fix (FIXED 03ddcd2) |
| Deleted-file viability boundary | og `0x3ff < size` strict gate (Dec 21208) | MATCH (parity at exactly 0x3FF) |
| Cancel cadence | og FUN_18000fb80:11236/:11530 (per chain step + per dentry) | MATCH |
| ext4 journal salvage | og: every block in [s_first, s_maxlen) | MATCH (all journal blocks surfaced) |

Parity verdict table (BENCHMARK-REPORT §1): every decompile-pinned behavior
carries a fails-before/pass-after regression test; EXCEEDS rows (grading,
session integrity, writer truth, report schema, cancellation granularity)
are registered deltas with rationale (PARITY-DELTAS D-38/D-41/D-14/D-01).

### 3.5 og transport model (CITED)

The og itself **serializes QD1** (REFERENCE-ARCHITECTURE checklist #5, cited
via `engine/worklog/PERF-LEVERS.md` F2) — the synchronous QD1 model our
direct transport matches is a parity property, not a compromise.

---

## Section 4 — NEW container component measurements (MEASURED-HERE, this task)

**What these are:** component microbenchmarks of the engine's real
`cr::CachedReader` (the repo's unmodified `engine/src/core/storage/
cr_cached.cpp` compiled by this harness) on this Linux container's storage.
They are **NOT device-throughput truth** — the backing file is served by the
Linux page cache (no sudo → drop_caches impossible; page cache warm and
recorded). Their value is the **RELATIVE cost of the cache-layer design**:
the mutex serialization factor, the LRU lookup overhead, the allocation
overhead.

**Script:** `/home/z/my-project/scripts/bench_cachedreader.cpp` (outside the
repo, per the task constraint).
**Exact build command:**
```
g++ -O2 -std=c++20 -Wall -Wextra \
    -I/home/z/my-project/final-prod/engine/include \
    /home/z/my-project/scripts/bench_cachedreader.cpp \
    /home/z/my-project/final-prod/engine/src/core/storage/cr_cached.cpp \
    -pthread -o /home/z/my-project/scripts/bench_cachedreader
```
**Exact run command:** `cd /home/z/my-project/scripts && ./bench_cachedreader /tmp/bench_dev.img`
(raw outputs saved: `scripts/bench_cachedreader.out` = run 1,
`scripts/bench_cachedreader.out2` = run 2, the fixed-counter rerun; numbers
below are run 2 with run-1 in parentheses where they differ).
**Environment:** backing file `/tmp/bench_dev.img`, 256 MiB, **overlayfs**
(stat -f: overlayfs), page cache warm; kernel 5.10.134-013.15.kangaroo.al8.x86_64;
2 vCPU (Intel Xeon, model masked); 4,159 MiB RAM; gcc 14.2.0 -O2; the device
fake is a `pread()`-backed `IStorageDevice` written for this harness (the
unit tests' fake is the RAM `MemDevice`, `engine/tests/unit/test_cached.cpp:12`
— same IStorageDevice contract); `CachedReader::create(dev, 8<<20, 256*1024)`
(the production shape: 8 MiB / 256 KiB = 32 blocks).

### 4.1 (a) Sequential steady-state throughput + hit/miss — MEASURED-HERE

| Leg | Result |
|---|---:|
| raw FileDevice, 256 KiB sequential over 256 MiB | **759.0 MiB/s** (run 1: 772.7) |
| CachedReader all-miss pass (hits=0, misses=1024) | **689.4 MiB/s** (run 1: 710.3) |
| **cache-layer overhead on the streaming miss path** | **+10.1%** (run 1: +8.8%); checksum FNV-1a **MATCH** (cache bytes == direct bytes) |
| hot 8 MiB window × 10 passes (hits=320, misses=0) | **23,429 MiB/s** (run 1: 23,654) — the hit path is ~34× the miss path |

Reading: on this container the cache layer costs ~9-10% of streaming
throughput (per miss: one 256 KiB buffer malloc + the pread + the extra
memcpy through the block + LRU housekeeping) and turns a 64×-amplified miss
(256 KiB fill per request-class) into a ~34× faster hit.

### 4.2 (b) Lock contention — the global-mutex serialization factor — MEASURED-HERE

N threads, random-ish (xorshift) 4 KiB reads, all hits (warm 8 MiB window),
through ONE `CachedReader` (the `std::mutex mtx_` guards the whole
`read_at`, `cr_cached.h:81` / `cr_cached.cpp:31`):

| N threads | aggregate ops/s | aggregate MiB/s | scaling vs N=1 |
|---:|---:|---:|---:|
| 1 | 4,436,338 | 17,329.4 | 1.00× |
| 2 | 2,277,890 | 8,898.0 | **0.51×** |
| 4 | 1,030,968 | 4,027.2 | **0.23×** |
| 8 | 892,302 | 3,485.6 | **0.20×** (run 1: 0.11×) |

**The serialization cost measured:** aggregate throughput does not scale —
it actively *degrades* 2-9× at N=8 on 2 vCPU (mutex handoff + the LRU vector
mutation under the lock + oversubscribed context switches; a flat 1.0× would
be "merely serialized"). On the production read mix (misses hold the mutex
through the whole fill), the effect is bounded by the fill itself: the
miss-path variant (b2) — random 4 KiB reads over the FULL 256 MiB (97%
misses, 256 KiB fill under the lock) — measured **29,334 ops/s at N=1 vs
20,691 ops/s aggregate at N=4 (0.71×)**. Caveat: 2 vCPU container; the
production engine runs 4 scan worker threads on ≥4-core hosts, so this
number is the *contention shape*, not the deployed magnitude. It is the
component-side evidence for PERF-LEVERS **F1** ("CachedReader lock scope —
fill outside the mutex; block-level, not transport-level"), which was
DEFERRED precisely because its QD2-4 payoff "is unvalidatable on the CI
runner class" — the container now supplies the contention half of that
evidence; the throughput-payoff half still needs real media (Section 5).

### 4.3 (c) LRU block-lookup overhead (the O(n) scan) — MEASURED-HERE

2M random 512-byte reads (all hits), 256 KiB blocks, capacity n blocks:

| Capacity | ns/read | delta vs 1-block |
|---:|---:|---:|
| 1 block | 20.0 | — |
| 2 blocks | 27.6 | +7.7 ns |
| **32 blocks (the production shape)** | **105.0** | **+85.1 ns** (run 1: +89.0) |

Reading: at the production capacity (32 blocks) the linear find +
move-to-front costs **~85 ns per read** on top of a ~20 ns floor — i.e. the
O(n) LRU housekeeping roughly 5×'s the pure hit cost, though absolutely tiny
(85 ns vs 122 µs random-read latency on the runner: ~0.07% — consistent with
PERF-LEVERS F3's "microseconds against walls measured in hundreds of
seconds" frame).

### 4.4 (d) Per-miss allocation churn — MEASURED-HERE

| Loop | ns/iteration |
|---|---:|
| resize-churn: `{ vector<u8> b; b.resize(256 KiB); }` (the miss path's `blocks_.data.resize(blen)` shape, `cr_cached.cpp:60`) | **4,632** (run 1: 5,400) |
| reused-buffer memcpy 256 KiB | **5,670** (run 1: 6,274) |

Reading: the per-miss 256 KiB buffer churn costs **~4.6 µs** — about **0.8×
the cost of the 256 KiB block copy itself** on this container (glibc
recycling the 256 KiB chunks; not the dominant term of a miss next to the
device read, but a real per-miss cost that a block-pool design would remove).

### 4.5 Build feasibility + suite facts (MEASURED-HERE)

- **Configure:** `cmake --preset linux-release -DCR_ENABLE_TESTS=OFF` —
  clean configure in 0.4 s (cmake 4.4.5 pip-installed; the image ships no
  cmake — that was the one missing tool).
- **Deps:** zlib 1.3.1, liblz4 1.10.0, libzstd 1.5.7 all discoverable via
  pkg-config/headers (vcpkg.json wants zlib/gtest/lz4/zstd; gtest is NOT a
  system package here but CMakeLists fetches googletest v1.15.2 via
  FetchContent — network to github.com works in this container, fetch
  verified).
- **Build:** `cmake --build --preset linux-release -j2` — **1 m 25 s wall**
  (user 1 m 58 s) for `libcr_core.a` + `libcr_ui.so`, gcc 14.2.0 with the
  repo's global `-Wall -Wextra -Wpedantic -Werror -Wshadow` — **zero
  warnings/errors**.
- **Tests:** second build dir with `-DCR_ENABLE_TESTS=ON`; `cr_tests` built
  100%; subset runs green: `Cached.*` **4/4** (1 ms — includes the
  `ScanCacheBudgetIsTheRegisteredDivergence` doc-truth pin);
  `QueryPerf.MillionNodesFilterCountUnder200ms` **PASS** —
  **1M nodes filter+count 26 ms, sorted page(100) 64 ms** (test wall 421 ms
  incl. fixture build) vs the recorded session-6 "362 ms cold" — same class,
  container-relative.
- ASan/TSan/UBSan lanes NOT run (timebox; they exist in ci-local.sh and the
  presets).

---

## Section 5 — NOT-MEASURED register (requires real Windows / physical hardware; exact procedure each)

Every row below is explicitly NOT measured anywhere in this repo at HEAD
(checked against PERFORMANCE-TARGET §2's UNMEASURED list, PERF-LEVERS' two
DEFERRED levers, and this task's container limits), with the procedure to
obtain it.

### 5.1 Per-transport QD/read-size optima per device class (NVMe / SATA SSD / HDD / USB / SD)

**Procedure:** on a physical Windows machine (elevated), run the in-tree
harness `engine/bench/ours/run-alignbench.ps1` against each media class —
per-device targets `\\.\PhysicalDriveN` / `\\.\C:`-class volumes. What it
sweeps today (read from the script, lines 301-320): the 4-KiB-vs-64-KiB
slow-path demonstration (128 KiB vs 132 KiB chunks, NO_BUFFERING, QD1); the
**read-size × queue-depth sweep — chunk {64 KiB, 256 KiB, 1 MiB, 4 MiB,
7 MiB} × QD {1, 2, 4, 8}** (QD1 = the production synchronous shape; QD>1 = a
FILE_FLAG_OVERLAPPED ring with manual-reset events reset before every
re-issue), 512 MiB span per cell (param `-SpanMiB`), two passes, FNV-1a
checksum telemetry (`checksum_match`, recorded not gated on live OS
volumes), `cpu_ms` per cell (OQ-S4-3), the og-vs-core chunk-strategy verdict
block, and the cr_ui cheap-op API latency leg (`-CrUiDll`). **How to extend
for QD 16/32:** the ring already handles arbitrary depth — add `16, 32` to
the QD list at the sweep loop (`foreach ($qd in @(1, 2, 4, 8))`, script line
~316; the slot array, event reset, and harvest loop are all QD-generic).
Per-class optima then read off the sweep plateau per device. Runner caveat
applies: on the Azure windows-2022 class the sweep is QD-flat at ~240 MiB/s
(PERFORMANCE-BASELINE §6 leg 2) — **this row needs physical media, full
stop** (PERF-LEVERS F1/F2's revisit trigger: "a physical-NVMe runner profile
or a QD-sensitive fixture").

### 5.2 End-to-end scan throughput on physical media (deep/deepcarve walls, TTFR, CPU/WS outside the VHDX class)

**Procedure:** the benchmark matrix — `.github/scripts/Run-Benchmark.ps1`
(1,000 lines, in-tree at HEAD) drives: disk environment (New-TestDisk.ps1
seeds the fixed VHDX disks: small-fat32 2 GiB, multi-gpt 5 GiB, large-ntfs
8 GiB with the deterministic 23-format corpus), feature probes
(cancel+resume with `cancel_to_stop_ms`), the clean-volume benchmark (the
consumer-native bench sweep: `seq` at 1 MiB/64 KiB/256 KiB/4 MiB + `rand` at
1 MiB/4 KiB + `open` — Run-Benchmark.ps1 lines 431-435 — plus process
sampling at 500 ms → CPU-s and WS-peak), the 7-scenario CLI+API matrix
(api_harness.c: open → scan(mask 7, 4 threads) → save → query → preview
(`probe_ms`) → recovery → report), and phase 5, the performance regression
gate against `bench/baseline.json`'s 10 rows. Dispatch surface: the lanes
documented in `engine/worklog/BENCHMARK.md` §1 (`test-benchmark.yml`
release-lane / `benchmark-src.yml` source-lane, workflow_dispatch with
core_tag/profile/scan_modes/reason) — **note at HEAD `ffd1e0c2` those two
workflow YAMLs are not present under `.github/workflows/` (the repo root
carries engine-ci/bridge-integration/contracts-ci/ui-ci/ui-e2e/forensic-repro/
release/validation-evidence)**; the scripts and `engine/.github/scenarios/
benchmark-matrix.json` ARE in-tree, so either re-wire the lane per
BENCHMARK.md §1 or execute Run-Benchmark.ps1 directly, elevated, on a
physical-media Windows box (bench refuses with exit 4 when not elevated —
by design). 3-run medians before any verdict (PERFORMANCE-TARGET §6).

### 5.3 The ~3.5× unaligned slow-path ratio on mid-range NVMe (830 vs ~2900 MB/s)

**Procedure:** alignbench leg 1 (128 KiB vs 132 KiB NO_BUFFERING QD1) on
physical NVMe — on the virtualized runner it measured 1.03× and that is
recorded as runner-class-relative only (PERFORMANCE-BASELINE §6 leg 1's own
caveat). The 64 KiB envelope is mandated by construction regardless (its
correctness is unit-tested, `tests/unit/test_directalign.cpp`).

### 5.4 Volume-size scalability ladder (2 / 8 / 32 GiB full profile)

**Procedure:** PERFORMANCE-TARGET §2 unmeasured row — the T6 ladder: run the
full-profile dispatch (5.2's lane) against 2/8/32 GiB seeded volumes; walls,
CPU/GiB, WS-peak per rung; memory boundedness audit at 32 GiB (PERF-LEVERS
lever 12).

### 5.5 QD2-4 pipelining payoff (F1 mutex rework + F2 OVERLAPPED slots)

**Procedure:** after landing F1/F2 behind a flag, re-run 5.1's extended sweep
(QD 16/32 included) on physical NVMe; the CI VHDX cannot show the win
(saturates ~544 MiB/s at QD1 — PERF-LEVERS F1's arithmetic). This task's
container microbench (Section 4.2) supplies the contention-side cost; the
throughput-side payoff remains Windows+media-bound. TSan lane + the
concurrency-correctness tests ride any F1 change (PERFORMANCE-TARGET §4
lever 4's risk row).

### 5.6 UI-interactive latencies on real hardware (first-painted-result, filter/preview interaction)

**Procedure:** the api_harness probe already captures `probe_ms` (preview
TTFB; 0.0 ms first capture — sub-ms on the runner) per dispatch; UI-interactive
latencies (bridge event → React paint, filter-bar latency on a 1M-node tree)
need a Windows interactive session: run the bridge-integration lane
(`ui/src-tauri/it/tests/bridge_integration.rs`, attaches seeded VHDs) with
timing instrumentation, or a manual elevated run of the staged app against
the seeded fixture. Not obtainable in this Linux container.

### 5.7 Alignment-controlled rand leg (F5 — measure the device, not the envelope)

**Procedure:** needs a new `bench` flag (CLI surface currently frozen by the
mandate; the honest number today is the ENVELOPE path rand measures —
BENCHMARK §6). Scoped in PERFORMANCE-TARGET §2 as the QD lever's
precondition; re-open with 5.1's extension if ever needed.

### 5.8 Fresh og-side comparative runs

**Procedure:** none — the og-comparison lanes were retired with the corpus's
captured evidence as the reference (`engine/docs/benchmarking.md` §6);
Section 3's og numbers are the closed record. Any new og evidence would
require re-activating `bench/og/run-og.ps1` (UI automation of DRW.exe) on a
Windows runner with the og free build — out of scope for this session and
explicitly retired.

---

*Assembly: every Section 2-3 row cites its file+section+run ID; every
Section 4 row cites its script, build/run command, and environment. Nothing
projected. The one file created in the repo by this task is this sheet;
the harness lives outside it at `/home/z/my-project/scripts/`.*
