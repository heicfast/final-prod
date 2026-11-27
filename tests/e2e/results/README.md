# tests/e2e/results — the committed evidence of record

This directory carries the results of completed e2e runs, committed per the
suite's evidence policy (`../README.md` § Evidence commitment policy):

- Each subdirectory `<run-id>/` is one complete run's merged results tree in
  the `../RESULTS-SCHEMA.md` layout — downloaded from the
  `e2e-full-results-<run_id>` CI artifact of the corresponding green run
  (`../analysis/<run-id>.md` is the human analysis of that run).
- The ground truth of each run is its own `03-baseline/seed-manifest-*.jsonl`
  (the deterministic corpus manifest for that run) — results were compared
  against it, never against the product's own output.
- Byte-level payloads (virtual disk images, full recovered trees) are NOT
  committed: the sector-range hash maps, sessions, reports, inventories, and
  sample recovered files carry the provable evidence, and the CI artifacts
  (90-day retention) carry the bulk. The bounded-artifacts rule holds.
- Committed trees pass `../scripts/Assert-Clean.ps1` (no token material, no
  placeholder residue) before every commit.

The runs present here are indexed below as they land:

| run-id | workflow run | commit | product | analysis | verdict |
|---|---|---|---|---|---|
| full-45916806570 | https://github.com/heicfast/final-prod/actions/runs/45916806570 | 20eaec0c (release recipe) | build-tree kit | ../analysis/full-45916806570.md | **45/45 PASS, all hard gates green** — NTFS PERMDEL/UNLINK/FOLDER 0.9993, QKFMT 0.9773, DECAY50 0.907, PARTDEL 0.9976; exFAT PARTDEL 0.9982 (discovery); ReFS clean-refusal probes PASS (the recorded finding: README claims first-class); CLI-vs-API agree on every measured scenario |

The `full-45916806570` tree is the pruned evidence core (manifests, deletion
records + sector-range hash maps, per-mode metrics, divergence records,
verdicts, gate tables); the byte-level detail (G-09 reports, recovered
inventories, trees, sessions) rides the run's CI artifacts.
