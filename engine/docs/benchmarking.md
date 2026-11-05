# Benchmarking (v0.3 harness — session-97)

Two benchmark lanes, wired into CI, plus the in-process counters. The full
harness documentation (phases, evidence tree, measurement rules, the
reproduction recipe for every recorded number) lives in
**`worklog/BENCHMARK.md`**; the gates and tolerances live in
`worklog/PERFORMANCE-TARGET.md`; the measurement ledger in
`worklog/PERFORMANCE.md`; the versioned gate rows in `bench/baseline.json`.

## 1. The two lanes

| | `test-benchmark.yml` (the release lane) | `benchmark-src.yml` (the source lane) |
|---|---|---|
| measures | PUBLISHED RELEASES (hash-verified fetch, never a build) | the checked-out commit, built with the release recipe |
| dispatch | auto after every publish + manual | manual (`profile`, `scan_modes`, `run_tests`, `reason`) |
| role | release-quality gate | measuring UNRELEASED changes (the perf discipline: every optimization is measured before release) |

Both run the SAME matrix (`.github/scripts/Run-Benchmark.ps1`): seeded
VHD disks with a deterministic 23-format corpus, the feature probes
(cancel+resume with `cancel_to_stop_ms`), the clean-volume benchmark, the
7-scenario matrix (CLI + API lanes), the gate table, and the
**performance regression gate** (the driver-parity rows from
`bench/baseline.json`, with tolerances; a missing metric fails the run —
no silent skips). Evidence artifacts are always uploaded.

## 2. Transport micro-benchmarks - `rrecov bench`

```
rrecov bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
            [--iterations N] [--seed N] [--json]
```

- `seq` - sequential throughput (MiB/s) at 4 KiB..16 MiB blocks over a MiB
  span, QPC-timed, sector-aligned buffers
- `rand` - LCG-seeded random reads: mean latency + IOPS (the offsets are
  generally NOT 64 KiB-aligned — the leg measures the ENVELOPE path, the
  same path consumers' unaligned reads take; the honest number)
- `open` - open/close churn (the \\.\\PhysicalDriveN open path)
- `read` - LBA0 contract probe through the active transport
- `info` - geometry/transport report (4Kn detection, direct vs image)

The clean-volume phase of every dispatch runs the **consumer-native size
sweep**: `info`, seq at 1 MiB / 64 KiB / 256 KiB / 4 MiB, rand at
1 MiB / 4 KiB, `open` — each JSON tagged with a `leg` label in
`clean-volume-benchmark.json` (the cache-block fill size, the og's 4 MiB
chunking, the image chunk, the NTFS record granularity).

The transport is DIRECT USER-MODE RAW I/O (session-96: the kernel driver
was replaced by \\.\\PhysicalDriveN opens under an elevated token) - bench
refuses with exit 4 (ERR_ELEVATION_REQUIRED) when the process is not
elevated, so the numbers measure the stack exactly as a scan experiences
it. `rrecov bench` reports transport "direct" (or "image"). Mode names and
metrics historically aligned with the rda-driver `rdabench` tool, so
driver-era and core-level numbers compare directly (the historical
driver-era numbers stay citable as history).

## 3. Performance numbers - where they live

- **The standing driver-parity baseline of record**: run 41042267586
  (v0.6.1, driver-backed, windows-2022, 8 GiB 512e VHDX): bench seq 1 MiB
  536.92 MiB/s, rand 1993.22 µs / 502 IOPS, open 17,019 ns; scan walls
  quick 150 ms / deep 16,505 ms / deepcarve 1,501,622 ms; API open 0.9 ms
  / scan 1,513,221.7 ms / save 10.8 ms / recovery 49.2 ms.
- **The direct-path comparison**: `worklog/PERFORMANCE.md` §2 (per-row
  baselines, results, run IDs, verdicts).
- **The gate**: `bench/baseline.json` (versioned rows + tolerances,
  changed only by reviewed commit with evidence) — evaluated by
  Run-Benchmark.ps1 phase 5 on every dispatch of either lane.
- Historical transcriptions: docs/PERFORMANCE-BASELINE.md /
  docs/BENCHMARK-REPORT.md (the from-source-lane era; read with their
  historical markers).

## 4. Engine counters (in-process)

- `cr::carve::CarveStats` - bytes scanned, hits, validated, hashed
- `cr::CachedReader` - hits/misses
- `cr::ScanStats` - files/deleted/carved/lost-partitions, bytes, elapsed
- `refs::RefsScanStats`, `zfs::ZfsScanStats`, ... - per-engine counters
  surfaced by `rrecov scan --json --log`

Cache-budget intent (N-045, PARITY-DELTAS D-72): the scan pipe's
`CachedReader` runs an 8 MiB / 256 KiB-block reactive LRU where the og's
`CStorageDeviceWithCache` is a 4 MiB prefetch read-ahead engine
(DRWUI_decompiled.c:919080-:919092, :962250-:962597). The 2x budget and
the reactive algorithm are REGISTERED deliberate divergences — measured
superior at the runner profile (the direct path beats the driver's walls
by 26-32% without a prefetch thread; worklog/PERF-LEVERS.md F10); the
budget must not drift silently (doc-truth pin:
`Cached.ScanCacheBudgetIsTheRegisteredDivergence`).

## 5. Process sampling (session-97)

Every clean-volume scan and every scenario-lane scan/recover is
process-sampled at 500 ms (`samples-*.csv`: t_ms, working_set_bytes,
cpu_seconds); the phase-4 roll-up transcribes CPU-seconds and WS-peak
into `perf-summary.json`. The API harness records `cancel_to_stop_ms`
(cancel-to-scan-return latency) and the preview probe's `probe_ms`.

## 6. Historical note (the deleted pre-session-96 lanes)

The pre-consolidation benchmark bodies are deleted - `benchmark.yml`,
`comparison-ours.yml` and `comparison-easeus.yml` (both historical, their
og-comparison role retired with the corpus's captured evidence),
`comparative-bench.yml` (deleted), `api-benchmark.yml` (deleted), and the
driver-era install chain (historical, removed in session-96). Their
correctness role lives in the scenario matrix, their transport-property
proofs in the unit lane (tests/unit/test_directalign.cpp) and the release
verification. The og-comparison role (running the reference product) is
retired - the corpus's captured evidence is the reference.
