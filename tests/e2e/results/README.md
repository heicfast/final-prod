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
| _(to be filled by the first committed run)_ | | | | | |
