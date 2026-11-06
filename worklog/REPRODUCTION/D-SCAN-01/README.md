# D-SCAN-01 — "jumps to 75% advanced scanning, files found N (0 B), no
sector progress, all stuck"

**Reproduced on:** the shipped v0.8.4 release, real Windows runner,
seeded 64 GB NTFS vdisk (forensic run
[42255492260](https://github.com/heicfast/final-prod/actions/runs/42255492260)).
This is BF-22 reproduced — the stall the prior session dismissed as
"environmental/known-perf" is a REAL product defect on real hardware.

## Expected

The scan starts, advances smoothly, surfaces live found-file count AND
found-data size, shows sector read progress, and completes. Reference
semantics: Advanced Scan runs its own moving 0–100% with "Files Found:
N (GB)" growing, "Reading sector: X/Y" ticking, remaining time, and a
live-growing results tree.

## Observed (`scan-timeline-extract.json`, 172 samples)

- **The 75% jump is exact math:** first sample at elapsed 0 s already
  reads `pcts: [75, 75]`, stage `advanced` — the quick (40%) + deep
  (35%) stages complete in seconds on a healthy volume, so the UI opens
  at exactly 75.0% (the carve boundary; `cr_scan.h:72-75` weights).
- **The stall:** at elapsed 891 s the percent reads `76.1` — **1.1%
  in 15 minutes**, `complete: false` throughout. The carve matcher
  grinds per-byte over 1 MiB windows with progress ticking only per
  COMPLETED window; in-window reads are memcpy-served (device IO
  idles) and the 5 s heartbeat does NOT re-emit progress.
- **The frozen counter:** `foundCounts: ["197 items"]` — the same value
  in the first and the last sample (the user saw "21" on their disk;
  both are real early-scan counts frozen at the carve boundary).
- **"0 B":** `dataFoundBytes` is pinned 0 for the entire live scan (no
  ABI field carries found-bytes; the bridge latches it only at
  terminal — `scan.rs:416-443`), while the reference shows cumulative
  GB live.
- **No sector progress:** `sectorMatches: []` every sample —
  `bytesScanned` is carve-true-bytes only, zeroed for stages 0/1
  (`cr_scan.cpp:596-597`), so "Reading sector: 0/N" holds through
  quick+deep and then freezes in the carve stall.

## Break point

Four stacked (all root-caused in DIAGNOSIS.md §D-SCAN-01 + lane B's
D4a-d decomposition): the carve checkpoint granularity (engine), the
missing bytes_found ABI field (engine+bridge), the carve-only
bytes_scanned stamping (engine telemetry), and the UI's single global
percent plateau (presentation of the same weights).

## Fix status

Engine-side (lane B per COORDINATION.md): D4a live bytes into
telemetry + the bridge emitter consuming `evt.bytes_done`, D4b the
bytes_found ABI append with live stamping, D4c intra-window carve
checkpoints, D4d the throughput verification. UI-side (landed): the
instant-completion bridge (W-01), the live tree growth (W-01), the
autosave stamp (W-01). The user-visible "0 B" and frozen sector ticker
resolve with lane B's telemetry landing; the release is held until
they do (a release with the frozen scan surface would ship the user's
#4 defect).
