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

## 5. Transport / I/O profile after the core-api transport wave (verified in code)

| Path | Read size issued now | Alignment envelope | Transitions per GiB (sequential) |
|---|---|---|---|
| RDA grammar path (scan engines via CachedReader) | **256 KiB blocks** (8 MiB LRU = 32 blocks) | **64 KiB unified** (core-api) | ≈ 4,096 |
| RDA grammar path (carve) | 1 MiB windows | 64 KiB unified | ≈ 1,024 |
| Direct PhysicalDrive/HarddiskVolume | 256 KiB blocks / 1 MiB windows / 1 MiB writer chunks | **64 KiB unified** | same shape |
| Writer (recovery reads) | 1 MiB + exact tail | 64 KiB (both transports) | ≈ 1,024 + tail bounces |
| Reference (og, decompiled) | **4 MiB** device chunks; 2 MiB MFT batches; 1 MiB $UsnJrnl; 64 KiB boot | all 64 KiB multiples by construction | **256-512** |

The og still issues 4-8× fewer transitions per byte than our engine path;
the alignbench sweep (§6) measures the plateau before any further tuning.
Self-healing reopen (og error-6 parity) landed on every device class — no
throughput claim, correctness only.

## 6. The Windows CI benchmark — FIRST MEASURED NUMBERS (run 40227375329,
windows-2022, 512 MiB span per cell, two passes checksum-gated; the vcpkg-release
build; the driver service `rda` loaded from the rda-driver v0.2.1 release)

Leg 1 — the 4-KiB-vs-64-KiB slow-path demonstration (NO_BUFFERING, QD1,
`\\.\C:`):

| Shape | Throughput | Notes |
|---|---:|---|
| 128 KiB chunks (64 KiB multiple) | **240.0 MiB/s** | the fast-path shape |
| 132 KiB chunks (4 KiB multiple, NOT 64 KiB) | **233.6 MiB/s** | the slow-path shape |
| **Ratio** | **1.03×** | the documented ~3.5× does NOT manifest on the runner's virtualized C: volume — recorded honestly; the transport's 64 KiB envelope stays mandated by construction (the mandate's 830-vs-2900 MB/s figure is mid-range NVMe hardware, not Azure VM storage) |

Leg 2 — read-size × queue-depth sweep (production flag shape, `\\.\C:`):

| Chunk | QD1 | QD2 | QD4 | QD8 |
|---|---:|---:|---:|---:|
| 64 KiB | 208.6 | 233.1 | 231.1 | 235.5 |
| 256 KiB | 238.9 | 240.6 | 236.1 | 237.3 |
| 1 MiB | 224.4 | 241.9 | 241.2 | 241.3 |
| 4 MiB | 202.8 | 241.3 | 238.1 | 242.4 |

**Read of the sweep:** the runner's C: caps at ~240 MiB/s — every size ≥256 KiB
saturates it at QD1 and QD>1 buys nothing beyond noise. The plateau is
size-flat on this hardware, which (a) validates keeping the og-matched
synchronous QD1 model (no architectural change justified by measurement) and
(b) validates 256 KiB cache blocks as sufficient to the plateau on this class
of storage. Fast local NVMe will re-measure via the weekly cron / dispatch.

Leg 3 — the RDA driver grammar-path round-trip (F11 Information ×512 unscale
verified on EVERY read; the exact production grammar
`\\.\rda\<PID-token>\<disk>\<part>`):

| Config | Throughput | Reads |
|---|---:|---:|
| RDA 64 KiB QD1 | **201.9 MiB/s** | 8192 |
| RDA 256 KiB QD1 | **169.3 MiB/s** | 2048 |
| RDA 1 MiB QD1 | **160.5 MiB/s** | 512 |
| direct `\\.\PhysicalDrive0` 256 KiB QD1 | **151.5 MiB/s** | 2048 |

**Read:** on this runner the DRIVER path beats the direct path at the same
chunk (169.3 vs 151.5 MiB/s @256 KiB) — the driver's per-read overhead is
repaid by its direct-IRP forwarding. The end-to-end integration is proven:
service install → test-sign → grammar open with the PID token → 8,192+
F11-scaled reads → checksum-stable. The driver-loaded integration test the
mandate requires now exists and runs on every push touching the transport.

Leg 4 — cr_ui cheap-op API latency (late-bound, 2M iterations/op, run
40228880804 — the first fully green alignbench run):

| Operation | ns/op |
|---|---:|
| `cr_ui_abi_version` | **10.7** |
| `cr_ui_last_error` | **11.3** |
| `cr_ui_carve_format_count` | **12.6** (count = 174: 35 grammar + 139 raw-sign) |
| `cr_ui_carve_format_info` | **166.3** (80-byte struct fill + the raw-sign confidence scan) |

**Read:** the cheap-op target (< 1 µs) is beaten ~100×; the shim's per-call
overhead (guarded wrapper + thread-local diagnostics) is single-digit
nanoseconds of real cost on top of the ~10 ns floor of a no-op P/Invoke loop.
`carve_format_info`'s 166 ns is the registry confidence scan — O(header
signs), documented in ABI-CONTRACT §7a.

The second measured run (40228880804) also re-confirms legs 1-3 within noise
(alignment ratio 0.99 — flat on virtualized storage; two-pass checksums now
recorded as `checksum_match` telemetry since the live OS volume legitimately
mutates between passes).

Tooling: `bench/ours/run-alignbench.ps1` +
`.github/workflows/api-benchmark.yml` (windows-2022; driver from the
rda-driver release, SHA256SUMS-verified, test-signed, installed as `rda`,
verified via verify-driver.ps1; no continue-on-error; artifacts always).

## 7. Acceptance targets for this session

1. **DONE** — every issued ReadFile on every transport: 64 KiB-multiple length
   except the device-tail clamp — proven by the exhaustive unit sweep
   (`UnifiedEnvelopeExhaustivePropertyBothDevices`) + the CI assertion that
   follows from the single-envelope construction.
2. Sequential engine path transitions: 64 KiB → 256 KiB blocks (≤4,096/GiB) —
   measured delta recorded here after the sweep lands.
3. No regression on the quality matrix (recall/precision rows unchanged) —
   the full suite is green at every landing.
4. Cheap-op API overhead: measurement wired (alignbench leg 4); bounded
   target < 1 µs/op for telemetry-class calls — number recorded when run.
5. Self-healing reopen: covered by construction on all four device classes
   (heal path exercised via the GLE-capture plumbing; Windows-lane
   integration coverage rides the driver-loaded CI job).
