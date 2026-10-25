# PERFORMANCE-BASELINE — measured numbers at session start

> Session: core-api (2026-10-26). Baselines: core `e6d5b01`, driver `7e88e38`,
> corpus `03ad46c`. Every number below is measured and citable, with its
> environment recorded. Entries marked **NOT MEASURED** are explicit holes
> this session fills — nothing is projected.

## 1. Environments

| Environment | Hardware/OS | Toolchain | Role |
|---|---|---|---|
| Local container | CI container, Linux | gcc 14.2 (`-O2 -Werror`; ASan preset) | logic-shape baselines, unit suites |
| windows-2022 runner | GitHub-hosted (2 vCPU), seeded 4 GiB GPT VHDX (NTFS+FAT32, JPEG/PDF/MP4/ZIP/text/1000 small files) | MSVC v143 / WDK 26100.6584 | driver microbench, comparative bench, integration |
| windows-2022 runner | same | MSVC v143, vcpkg (zlib, gtest) | core CI (Debug+Release), the new API/transport benchmarks |

## 2. Driver transport microbench (driver repo, run 39807929475, 2026-10-22)

Side-by-side `rdatest` (our driver, installed as `eprdtdrv`) vs. the original
`eprdtdrv.sys` on the same runner/VHDX — `rda-driver/docs/benchmarks/
2026-10-22-runner-comparison.md`:

| Metric | Original eprdtdrv | Ours R1 | Ours R2 | Verdict |
|---|---:|---:|---:|---|
| Sequential 64 KiB read | 391.0 MiB/s | 371.1 | **532.9** | parity+ (vs. worklog's 533.1 narrative — both cited) |
| Sequential 1 MiB read | 500.6 MiB/s | 458.6 | — | — |
| Random 4 KiB IOPS | 8,170 | 7,932 | 7,932 | −2.9% (documented price of cancel-safety + DACL) |
| Random 4 KiB latency | 122.4 µs | 126.1 | 126.1 µs | +3.0% |
| Device open (mean) | 8,042 ns | 10,993 | **8,217 ns** | +2.2% = SDDL check cost |
| Open churn (mean) | 8,537 ns | 11,331 | **8,532 ns** | parity |
| Hostile fuzz dispatch | 553,752 IOCTL/s | 707,161 | **841,698 IOCTL/s** | +52% |
| Data correctness | — | — | **64/64 SHA-256 ranges byte-identical** | exact |

Read-path context: 4 KiB-aligned buffers, `FILE_FLAG_NO_BUFFERING`, explicit
seeks, Information ÷512 rescale (rdabench.c:61-79 — the same discipline the
core's transport implements).

## 3. Core local baselines (Linux container, this session's start state)

| Measurement | Build | Result |
|---|---|---|
| Full unit suite (~305 tests) | linux-release (gcc -O2) | suite green (session-3 close: 305/305 ASan) |
| Full unit suite | linux-gcc ASan | green |
| Query/filter on 1M-node session tree | linux-release | 362 ms cold (filter+count+page); 31-36 ms under the in-suite ASan timing gate (200 ms budget) |
| Engine fixture suites (NTFS+ReFS walks) | linux-release | 22 ms (synthetic fixtures — not volume-scale truth) |

Source: docs/PERFORMANCE-PROFILE.md §1 (HEAD `064fcf4`; suite size has grown
since — counts refreshed at each landing).

## 4. Recovery-quality matrix (Windows comparative harness, runs 40073093997 / 40079065019)

Quality (recall/precision) is not this session's axis but is recorded for
completeness — docs/BENCHMARK-REPORT.md §1-§2, bench/root-causes.json. The
transport-relevant row: every recovered file with `size % 512 != 0` was
zero-filled "salvaged" before the sector-alignment shim (bench run
40073093997) — the shim fixed it; the 64 KiB envelope unification must keep
that property (it does — 64 KiB is a sector multiple).

## 5. Transport / I/O profile at session start (verified in code)

| Path | Read size issued today | Alignment envelope | Transitions per GiB (sequential) |
|---|---|---|---|
| RDA grammar path (scan engines via CachedReader) | 64 KiB blocks (8 MiB LRU) | **sector-only** — GAP | ~16,384 |
| RDA grammar path (carve) | 1 MiB windows | sector-only | ~1,024 |
| Direct PhysicalDrive/HarddiskVolume | 64 KiB blocks / 1 MiB windows / 1 MiB writer chunks | **64 KiB** (GAP-S6-13) | same shape |
| Writer (recovery reads) | 1 MiB + exact tail | 64 KiB (direct) / sector (RDA) | ~1,024 + tail bounces |
| Reference (og, decompiled) | **4 MiB** device chunks; 2 MiB MFT batches; 1 MiB $UsnJrnl; 64 KiB boot | all 64 KiB multiples by construction | **256-512** |

The og issues 16-64× fewer transitions per byte on the sequential engine
path. At ~1-2 µs syscall+dispatch per transition this is the dominant
user-mode overhead on fast NVMe — the read-size sweep (§7) quantifies it.

## 6. NOT MEASURED (this session's benchmark plan — Windows CI, driver loaded)

| # | Measurement | Method | Status |
|---|---|---|---|
| 1 | Core scan-path read throughput vs. read size (64 KiB / 256 KiB / 1 MiB / 4 MiB) | new Windows CI benchmark job over a seeded VHDX, both transports (RDA + direct), sequential read of a 1 GiB span | **planned (T9/T10)** |
| 2 | Queue-depth sweep (1/2/4/8 outstanding) | overlapped reads on the same job | **planned** — reference is QD=1 per handle; sweep documents the plateau, changes only if measurement proves headroom |
| 3 | The 4 KiB-vs-64 KiB slow-path demonstration (the ~3.5× claim) | same job: read a 1 GiB span at 64 KiB-multiple lengths vs. 4 KiB-multiple-not-64 KiB lengths through the direct path | **planned (T9)** — verifies the mandate's 830 vs 2900 MB/s shape on runner hardware |
| 4 | API call overhead (cheap ops) | `cr_ui_abi_version` / `last_error` / `telemetry` / `snapshot` microbench loop, ns/op | **planned (T15)** |
| 5 | Driver round-trip overhead from the core | time 100k sequential ReadFiles through the grammar path at 64 KiB vs. driver-repo rdabench numbers | **planned (T15)** |
| 6 | Scan end-to-end throughput (files/s, MiB/s) on the seeded scenarios | comparative-bench harness extension | **planned** |
| 7 | C-consumer lifecycle wall time | ui_abi_check timing | **planned (T13)** |

Rules for every new number: environment recorded (runner, VHDX, build
preset), N≥3 runs with median reported, no continue-on-error, artifacts kept.

## 7. Acceptance targets for this session

1. Every issued ReadFile on every transport: 64 KiB-multiple length except the
   device-tail clamp — proven by unit test + CI assertion.
2. Sequential engine path transitions: 64 KiB → 1 MiB blocks (≤1,024/GiB) —
   measured delta recorded here after the sweep.
3. No regression on the quality matrix (recall/precision rows unchanged).
4. Cheap-op API overhead: measured and bounded (target: < 1 µs/op for
   telemetry/last_error; snapshot documented separately as O(n)).
5. Self-healing reopen: covered by a contract test (no throughput claim).
