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
3. 8 GiB fixed VHD attached, GPT, 5 GB NTFS + exFAT volumes
4. `scripts/bench_seed.ps1` seeds deterministic content (real format
   shapes: JPEG/PNG/PDF/ZIP/MP4 headers over pseudo-random bodies) and
   applies five scenarios: **keep** (control), **recycle** (Recycle-Bin
   delete via the headless-safe VB API), **perm** (Remove-Item), **wipe**
   (zero-fill + delete - honest floor, excluded from the recall
   denominator), **folder** (subtree removal). Manifest CSV carries
   name/size/SHA-256/scenario per file.
5. Driver installed, then TWO per-volume capped deep scans
   (`scan --volume X: --deep`): one per volume. Per-volume sessions
   keep ground-truth attribution unambiguous - both volumes are seeded
   by the same deterministic generator (identical relpaths), so a
   whole-disk result set could satisfy either manifest from either
   volume. Each scan is poll-capped (`scan_minutes` input, default 35)
   with partial-state capture on cap.
6. `scripts/bench_score.py` scores path-level and name-level recall per
   scenario for EACH volume manifest; the keep-control gate FAILS the
   run if live files were missed. Artifacts: manifests, results,
   sessions, logs, timing, scores.
7. **Deleted-partition scenario**: the GPT entry for the exFAT volume is
   then removed (`Remove-Partition` - the data sectors stay intact), and
   a whole-disk capped deep scan (`scan --disk N --deep`) runs in
   lost-partition mode: discovery probes + virtual candidate slices +
   the exFAT engine under per-candidate roots. The SAME exFAT manifest
   is scored against those results (scores-lost) - a two-mode regression
   matrix: normal per-volume recovery vs lost-partition recovery, same
   ground truth, keep-control gate in both modes.

Historical note: the first benchmark generations ran a single
whole-disk scan that stalled for tens of minutes per run. Root cause
was read amplification in the carve validators (P1-12: PDF/ZIP forward
rescans stepping in 0.5-1 KiB reads through the uncached slice - up to
2M syscalls per candidate). Fixed by 1 MiB block scans (commit af4bc8e);
the scan leg is now minutes, and the cap is a safety net, not the norm.

## 3. EaseUS comparison - `comparison-ours.yml` + `comparison-easeus.yml`

The original product vs ours. Originally one workflow; split (at the
user's direction) into two INDEPENDENT legs so a UI-automation hiccup
on the og side can never void the CLI leg and vice versa. Both legs
use the SAME methodology and environment:

- windows-2022, 8 GiB fixed VHD, GPT, one 5 GB NTFS volume ("CMP-NTFS")
- the SAME `bench_seed.ps1` DEFAULT parameters - deterministic seeding
  (fixed-seed System.Random) means identical manifests across runners
  by construction; "same disk" is achieved by identical parameters,
  not shared state
- BOTH raw-disk drivers installed side by side (og's eprdtdrv.sys under
  its native service name + our test-signed rdatest); driver starts are
  non-fatal on the leg that does not require that driver
- identical scoring: `scripts/compare_report.py` is the single matcher
  for both products (name+size, structured-vs-carved split, per-scenario
  table, JSON + md); `wipe` excluded from both denominators

**`comparison-ours.yml` (our leg)**: capped `scan --volume X: --deep`
(the same volume the og UI selects), full logs, `tree --csv` -> ours.csv,
uploads the `comparison-ours` artifact.

**`comparison-easeus.yml` (og leg)**: the easeus-drw-research corpus's
og-codebase, DRW.exe launched and driven through its real UI (mouse
injection - the corpus-proven recipe), window-title polling captures
the files-found counter, screenshots at every phase, the auto-saved
`.rsf` harvested from `<install>\SaveScan`, `compare_og_rsf.py`
extracts og's tree (byte-exact validated against both shipped QA
samples: 267,886/36,001,842 and 218,513/29,894,861 bytes consumed;
all 9 per-FS content grammars). Its final step fetches the latest
successful `comparison-ours` artifact (GitHub API) and runs
`compare_report.py` over both legs - degrading gracefully (empty ours
column + explicit note) when the ours leg has not run yet.

The og leg is best-effort by nature (UI automation); every step captures
maximum evidence for offline analysis.

## 4. Engine counters (in-process)

- `cr::carve::CarveStats` - bytes scanned, hits, validated, hashed
- `cr::CachedReader` - hits/misses
- `cr::ScanStats` - files/deleted/carved/lost-partitions, bytes, elapsed
- `refs::RefsScanStats`, `zfs::ZfsScanStats`, ... - per-engine counters
  surfaced by `rrecov scan --json --log`

## Regression baselines

The benchmark workflow is manual-trigger (`workflow_dispatch`) with
30-day score artifacts; the two comparison legs are manual too (run
`comparison-ours` first, then `comparison-easeus`). The transport
micro-benchmarks are recorded per release tag and compared against the
previous release (>5% regression = investigate before merge).
