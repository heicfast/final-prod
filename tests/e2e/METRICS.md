# METRICS — definitions, measurement, and the baseline

Every metric below is computed by `engine/bench/metrics/compute_metrics.py`
and `engine/bench/metrics/inventory_recovered.py` from the run's artifacts,
per scenario × lane × scan mode. Ground truth is the seed manifest + the
recorded deletion method — never a tool's own output. Numbers are rounded to
4 decimals; counts are integers.

## Recovery-quality metrics (per scenario × lane, deepcarve headline)

| Metric | Definition | Measured from |
|---|---|---|
| `recall` (hash) | of the manifest's deletable entries (keep=false, size>0), the fraction recovered **with a byte-identical SHA-256** (inventory `hash_match`). For quick_format/full_format/delete_partition the denominator is the whole seeded set (the deletion method destroys the set boundary) — the bench convention | `recovered-inventory.json` records vs the manifest |
| `recall_name` | same denominator, recovered by path/name match without requiring hash equality | same |
| `precision` | of everything the lane recovered, the fraction that matches the manifest (any match level). Unmatched recoveries are false positives, classified `tool-internal` / `unmatched-carveable-plausible` / `noise` | same |
| `f1` | harmonic mean of recall and precision | same |
| `false_positives` + `fp_class` | count + classification of recovered-but-not-in-manifest files | same |
| `path_preservation` | share of matched rows whose match level is `path` (vs namesize/name/hash-only) — the name/path preservation rate | same |
| `physical_ceiling` (decay rows) | post-decay per-file extent-range hashes from `apply-scenario.ps1`: recall re-judged against the physically-intact set (GAP-S6-5 — the NTFS decay_80 cliff is sector reuse, not an engine defect) | `state/physical-ceiling.json` |
| `preservation.json` | attribute preservation probes on recovered counterparts: ADS streams present, hard-link group both-recovered + link count, sparse attribute re-set, timestamps | `engine/bench/ours/run-ours.ps1` |

## Divergence metrics (per scenario, `divergence.json`)

| Metric | Definition |
|---|---|
| `agree` | the CLI and API lanes' hash-match sets are identical, per-rel hash flags agree, common recovered sizes agree, full-fidelity scan stats (files/lost_partitions) agree |
| `findings[]` | each set difference, flag difference, size difference, or stat difference — any entry is a hard scenario failure |
| `checked` | false only when a lane's inventory is absent (that lane's own gate owns the failure; the incomparable state is recorded, never guessed) |

## Performance metrics (per scenario × lane × mode)

| Metric | Definition | Measured from |
|---|---|---|
| `scan_wall_seconds` | the scan process's own span (ExitTime − StartTime — never the sampler's poll quantum, the run-42012118585 lesson) | `ours-run.json` + `samples-<mode>.csv` |
| `recover_wall_seconds` | the recover process's own span, same clocking | same |
| `cpu_seconds`, `ws_peak_bytes` | 500 ms process sampling of the CLI lane: CPU time consumed, working-set peak | `samples-<mode>.csv` |
| API phase timings | QPC per phase: open, scan run, save, reload, query, preview, recovery process, report — `timings_ms` block | `api-result.json` |
| `bytes_scanned`, `elapsed_ms`, `files`, `lost_partitions` | the engine's own scan summary (JSON line of `rrecov scan --json`) | `scan-<mode>.stdout` |
| `capped` | whether the run hit its time budget (ScanCapSec/RecoverCapSec) — capped walls never enter baselines | `ours-run.json` |

Cancellation responsiveness is exercised by the bench lane's cancel+resume
probe (the smoke/full lanes inherit the exit-3 + `--resume` contract; the
bench cancel_resume_probe is the recorded precedent — see
engine/.github/scenarios/benchmark-matrix.json).

## The baseline (tests/e2e/baseline.json)

- Schema: `scenarios.<scenario>.{recall, precision, perf.<mode>.scan_wall_seconds}`
  — the `engine/bench/metrics/regression_gate.py` contract.
- Source: a completed e2e run's CLI lane (`04-scenarios/<id>/cli/
  metrics-deepcarve.json` + per-mode walls), produced by
  `scripts/New-Baseline.ps1` from a reviewed green run.
- Thresholds (enforced by `e2e-regression.yml`): recall drop > 2% absolute →
  FAIL; precision drop > 2% absolute → FAIL; scan wall growth > 25% → FAIL;
  a baseline scenario missing from a full run → FAIL (`--require-all-baseline`).
- Update policy: human-reviewed commit only — see BASELINE.md.
