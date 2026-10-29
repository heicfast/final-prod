# Comparative Benchmark Harness (ours vs og EaseUS DRW free)

Evidence-grade, fully instrumented, reproducible comparison of `rrecov`
(core_recovery + rda-driver) against the original EaseUS Data Recovery
Wizard free build (v21.0.0.0, corpus snapshot under `easeus-drw-research`).

Everything runs on GitHub-hosted Windows runners via
`.github/workflows/test-benchmark.yml` (the CI rebuild's 3-lane
consolidation — `8943993` — replaced the old comparative-bench.yml
deliberately; the prebuilt-release lane carries the same seeded-VHD
matrix, and the TTFR probe rides its `--log stage-<mode>.log` harness).
No local interactive Windows machine is used or required.

## Principles

- **No masking.** No `continue-on-error`. A leg that cannot run fails the
  run with a diagnostic artifact. Skipped stages are recorded with a reason.
- **Ground truth is the seed manifest + the deletion method**, never either
  tool's output. Every scenario captures a 1 MiB-block SHA-256 hash map of
  the volume before and after the deletion so state changes are provable.
- **Every number is traceable** to an artifact under
  `benchmark-results/<scenario>/<tool>/` (uploaded, 90-day retention).
- ReFS/LDM/other unsupported combinations are recorded `NOT-COMPARABLE`
  with a precise reason - never silently dropped.

## Layout

| Path | Role |
|---|---|
| `bench/seed/generate_seed.ps1` | Deterministic, versioned seed corpus (file matrix: types, sizes, attributes, depth, CJK, long paths, fragmentation). Emits `seed-manifest.jsonl` + `manifest.csv` + per-file extents / NTFS record numbers. |
| `bench/volume/volume.ps1` | VHDX creation (New-VHD dynamic, diskpart fallback), format NTFS/exFAT/FAT32/ReFS, mount/teardown, raw-volume access (lock/dismount, FSCTL_GET_RETRIEVAL_POINTERS, NTFS volume data), hash-map capture, MFT/$Bitmap corruption, partial-overwrite writer. |
| `bench/scenarios/apply-scenario.ps1` | The deletion matrix: recycle_bin, permanent_delete, unlink_native, folder_delete_bin_empty, quick_format, full_format, delete_partition, decay_50/80/100, partial_overwrite, mft_corrupt. Emits `scenario-state.json`. |
| `bench/ours/run-ours.ps1` | rrecov scans (quick / deep / deep+carve) with `--log`/`--json`/`--out` session, `tree --csv/--json`, `recover --report`, process sampling (memory/CPU). |
| `bench/ours/run-behavior.ps1` | Driver-absent probe, cancellation responsiveness (CTRL_BREAK), session save->kill->resume equivalence. |
| `bench/og/og-driver.ps1` | UI automation library for the og (coordinate driver proven by the corpus atlas + window census + screenshots + gate dismissal). |
| `bench/og/run-og.ps1` | Per-scenario og run: state reset, launch via DRW.exe, volume selection with log verification, scan, `.rsf` harvest (`compare_og_rsf.py`), Recover-All funnel with 500 MB free-cap gate handling, quota accounting. |
| `bench/og/FlaUiProbe/` | Best-effort FlaUI/UIA3 element-tree dump + capture (auxiliary evidence). |
| `bench/metrics/` | `inventory_recovered.py`, `compute_metrics.py`, `compare.py`, `regression_gate.py`. |
| `bench/tools/pe_info.py` | PE OS-version + import dump for the driver/binary provenance records. |
| `bench/baseline.json` | Versioned regression baseline (human-reviewed commits only). |

## Metrics (per scenario x tool x scan mode)

recall, precision, F1, false-positive rate (classified), filename/path
preservation, timestamp preservation, attribute-class preservation
(ADS/hard-link/sparse), hash-match integrity, scan wall time, bytes scanned,
throughput, recovery time + throughput, peak memory, CPU, found vs recovered
(og free-cap normalization), session persistence, cancellation
responsiveness, driver presence/absence behavior.

## Reports

**[N-150 truth repair]** the two report files named above are RUNTIME
OUTPUTS of `bench/metrics/compare.py` (which IS on disk) — they appear
under `docs/` only when a comparative run actually executes, and are not
committed by design (generated artifacts do not ship). The earlier
present-tense phrasing read as standing files, which sent the S42 lane's
OQ-S4-3 registration looking for committed reports. The report contract,
when a run produces one: per-metric verdicts
`MATCH / EXCEEDS / BELOW / NOT-COMPARABLE` with root-cause notes for
every BELOW, from real run artifacts only; unfavorable results are
findings, not suppressions. The standing benchmark evidence of record is
`docs/BENCHMARK-REPORT.md` + `docs/PERFORMANCE-BASELINE.md` (both
committed) plus the per-landing numbers in each finding's Resolution.
