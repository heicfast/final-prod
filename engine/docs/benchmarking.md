# Benchmarking (v0.2 harness)

Four bodies, wired into CI:

## 1. Transport micro-benchmarks - `rrecov bench`

```
rrecov bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
            [--iterations N] [--seed N] [--json] [--no-rda]
```

- `seq` - sequential throughput (MiB/s) at 4 KiB..16 MiB blocks over a MiB
  span, QPC-timed, sector-aligned buffers
- `rand` - LCG-seeded random reads: mean latency + IOPS
- `open` - open/close churn (the RDA grammar path vs direct CreateFile)
- `read` - LBA0 contract probe through the active transport
- `info` - geometry/transport report (4Kn detection, rda vs direct)

The transport is RDA-first with direct fallback (the Device.mo duality),
so the numbers measure the stack exactly as a scan experiences it.
Mode names and metrics align with the rda-driver `rdabench` tool, so
driver-level and core-level numbers compare directly.

## 2. Recovery benchmark - `.github/workflows/benchmark.yml`

End-to-end recall/precision scoring of our stack on a deterministic
seeded disk (windows-2022 runner):

1. rrecov built (Release, vcpkg manifest)
2. RDA driver fetched from the rda-driver v0.2.0 release, test-signed
   with a job-local self-signed cert (SecureBoot-off hosted runners -
   no bcdedit/testsigning/reboot)
3. 8 GiB fixed VHD attached, GPT, NTFS + exFAT volumes
4. `scripts/bench_seed.ps1` seeds deterministic content (real format
   shapes: JPEG/PNG/PDF/ZIP/MP4 headers over pseudo-random bodies) and
   applies five scenarios: **keep** (control), **recycle** (Recycle-Bin
   delete via the headless-safe VB API), **perm** (Remove-Item), **wipe**
   (zero-fill + delete - honest floor, excluded from the recall
   denominator), **folder** (subtree removal). Manifest CSV carries
   name/size/SHA-256/scenario per file.
5. Driver installed, `rrecov scan --deep` + `tree --csv`
6. `scripts/bench_score.py` scores path-level and name-level recall per
   scenario; the keep-control gate FAILS the run if live files were
   missed. Artifacts: manifest, results, session, logs, scores.

## 3. EaseUS comparison - `.github/workflows/comparison.yml`

The original product vs ours on the SAME seeded disk:

- og leg: the easeus-drw-research corpus's og-codebase, DRW.exe launched
  and driven through its real UI (mouse injection - the corpus-proven
  recipe), both raw-disk drivers installed side by side (og's eprdtdrv
  under its native service name + our test-signed rdatest), window-title
  polling captures the files-found counter, screenshots at every phase,
  the auto-saved `.rsf` harvested from `<install>\SaveScan`
- our leg: `rrecov scan --deep` with full logging
- `scripts/compare_og_rsf.py` extracts og's .rsf tree (byte-exact
  validated against both shipped QA samples: 267,886/36,001,842 bytes
  and 218,513/29,894,861 bytes consumed; all 9 per-FS content grammars)
- `scripts/compare_report.py` applies the SAME name+size matcher to both
  products, structured-vs-carved split, per-scenario table, JSON + md

The og leg is best-effort by nature (UI automation); every step captures
maximum evidence for offline analysis.

## 4. Engine counters (in-process)

- `cr::carve::CarveStats` - bytes scanned, hits, validated, hashed
- `cr::CachedReader` - hits/misses
- `cr::ScanStats` - files/deleted/carved/lost-partitions, bytes, elapsed
- `refs::RefsScanStats`, `zfs::ZfsScanStats`, ... - per-engine counters
  surfaced by `rrecov scan --json --log`

## Regression baselines

The benchmark workflow runs on every src/** push; its scores.json
artifacts (30-day retention) form the per-commit baseline. The transport
micro-benchmarks are recorded per release tag and compared against the
previous release (>5% regression = investigate before merge).
