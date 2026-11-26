# BASELINE — the versioned regression baseline and its policy

`tests/e2e/baseline.json` is the versioned truth the regression gate
(`e2e-regression.yml` → `engine/bench/metrics/regression_gate.py`) compares
every full run against. It is the e2e twin of `engine/bench/baseline.json`
(same schema, same thresholds, same discipline — the bench lane's gate,
reused verbatim).

## The schema

```json
{
  "_comment": "Versioned regression baseline for the tests/e2e suite. Updated ONLY by human-reviewed commits …",
  "schema": 1,
  "source_run_id": "full-458123456",
  "scenarios": {
    "S-E2E-NTFS-PERMDEL": {
      "recall": 0.428,
      "precision": 0.99,
      "perf": { "quick": {"scan_wall_seconds": 1.2}, "deep": {"…": "…"}, "deepcarve": {"scan_wall_seconds": 41.5} }
    }
  }
}
```

Only the CLI lane is baselined (the deepcarve headline + per-mode walls).
The API lane's agreement with the CLI lane is enforced per-run by
`Compare-Lanes.ps1` — baselining both would double the surface without
adding a gate.

## The thresholds (regression_gate.py, verbatim from the bench lane)

- `recall` drops more than **2% absolute** → FAIL
- `precision` drops more than **2% absolute** → FAIL
- `scan_wall_seconds` grows more than **25%** → FAIL
- a baseline scenario missing from a full-matrix run → FAIL
  (`--require-all-baseline`; no silent scenario drops)

Dips within tolerance are reported as `regressions_within_tolerance`, not
passes: the gate prints them so a trend is visible before it trips.

## How the baseline may change

1. A green `e2e-full` run completes and its merged results artifact
   (`e2e-full-results-<run_id>`) is downloaded.
2. The candidate is generated — never written in place:
   `tests/e2e/scripts/New-Baseline.ps1 -ResultsRoot <run tree> -OutJson baseline-candidate.json -RunId <run-id>`
   (capped scans are excluded from the time rows — a capped wall is the cap,
   not the engine's speed; failed lanes are excluded and listed).
3. A human reviews the numbers against the run's `gates.md` + analysis.
4. The candidate replaces `baseline.json` in ONE reviewed commit that cites
   the run URL as its evidence. **Never by CI auto-commit. Never to hide a
   regression.** A baseline update accompanying a recall drop must say in
   the commit message why the drop is legitimate (corpus change, engine
   behavior change with its own evidence) — otherwise the regression gate
   stays red on purpose.

## Current state

Until the first reviewed promotion lands, `baseline.json` does not exist and
`e2e-regression.yml` fails fast with the diagnostic "baseline not yet
promoted" — a vacuous pass is the one forbidden outcome. The promotion
procedure is exactly the four steps above.
