# Benchmarking (v0.2 harness)

Four bodies, wired into CI:

## 1. Transport micro-benchmarks - `rrecov bench`

```
rrecov bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
            [--iterations N] [--seed N] [--json]
```

- `seq` - sequential throughput (MiB/s) at 4 KiB..16 MiB blocks over a MiB
  span, QPC-timed, sector-aligned buffers
- `rand` - LCG-seeded random reads: mean latency + IOPS
- `open` - open/close churn (the RDA grammar path vs direct CreateFile)
- `read` - LBA0 contract probe through the active transport
- `info` - geometry/transport report (4Kn detection, rda vs direct)

The transport is DRIVER-ONLY (docs/DRIVER-REQUIRED.md): the grammar path
is the only leg - bench requires the driver and refuses with exit 4
otherwise - so the numbers measure the stack exactly as a scan experiences
it. Mode names and metrics align with the rda-driver `rdabench` tool, so
driver-level and core-level numbers compare directly.

## 2. Recovery benchmark - WIPPED, superseded by the staged-scenario truth matrix

The historical from-source benchmark/comparison workflows
(`benchmark.yml`, `comparison-ours.yml`, `comparison-easeus.yml`, the later
`comparative-bench.yml` and the standalone `api-benchmark.yml`) are DELETED
(driver-only session, PLAN-CI-REBUILD section 1): every one of them rebuilt
the core (and drove the og leg) from source per run - the exact waste the
prebuilt-artifact mandate removes. Their surviving value lives on:

- the DRIVER-install mechanics (fetch release + SHA256SUMS verify +
  test-sign + service install + verification) historically rode the shared
  composite `.github/actions/install-driver/action.yml` (deleted by the CI
  consolidation — the mechanism now lives in
  `.github/scripts/Install-ReleaseDriver.ps1`);
- the end-to-end recall/precision role is superseded by
  `staged-scenarios.yml` (docs/CI-SCENARIOS.md): seeded VHD + scripted
  deletion + scan + expected-vs-observed + recovery + SHA256 verification,
  all against PREBUILT releases - no from-scratch build in the correctness
  path;
- the transport-property proofs (64 KiB envelope, sector alignment) live
  in the unit lane (tests/unit/test_directalign.cpp) and the release
  artifacts' own verification.

## 3. Performance numbers - where they live now

Transport-level numbers: `rrecov bench` on a driver-attached host, and the
rda-driver repo's own `rdabench` + runner-comparison docs. Baseline
transcriptions live in docs/PERFORMANCE-BASELINE.md /
docs/BENCHMARK-REPORT.md (historical, recorded when the from-source lanes
ran). New performance evidence is produced by measuring the prebuilt
releases (the same artifacts users run) - never by rebuilding per run.

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
the reactive algorithm are REGISTERED deliberate divergences — the
transport-level alignbench sweep owns the tuning decision; the budget
must not drift silently (doc-truth pin:
`Cached.ScanCacheBudgetIsTheRegisteredDivergence`).

## Regression baselines

The benchmark workflow is manual-trigger (`workflow_dispatch`) with
30-day score artifacts; the two comparison legs are manual too (run
`comparison-ours` first, then `comparison-easeus`). The transport
micro-benchmarks are recorded per release tag and compared against the
previous release (>5% regression = investigate before merge).
