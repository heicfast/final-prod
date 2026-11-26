# tests/e2e — the end-to-end recovery validation harness

Real Windows runners. Real VHDX virtual disks. Real seeded files. Real
deletions. The real product, driven through both surfaces — the `rrecov` CLI
and the `cr_ui` C ABI — with every result judged against exactly one ground
truth: the deterministic seed manifest. Nothing in the product path is mocked
or simulated.

## What this suite is

For every scenario in `matrix.json` (filesystem × deletion method × scan
mode × lane):

1. **VHD orchestration** — fixed-size VHDX per disk, created and attached via
   diskpart (`engine/.github/scripts/New-TestDisk.ps1`; the diskpart-only
   pattern deliberately avoids the Hyper-V module — every Windows runner
   image carries diskpart). NTFS, exFAT, FAT32, and ReFS (where the runner
   image supports it; an unsupported ReFS is a diagnostic failure, never a
   silent skip). Physical disk numbers are resolved by before/after diff of
   `Get-Disk` and recorded in `02-disk-setup/disk-state.json`. Detach +
   delete happens in the always-run teardown (`99-teardown`), recorded.
2. **Seed corpus** — `engine/bench/seed/generate_seed.ps1`, corpus v2.0.0:
   23 real-magic file formats (JPEG/PNG/HEIC/TIFF/CR2/NEF/ARW/DNG/MP4/MOV/
   AVI/MKV/PDF/ZIP/7z/RAR5/SQLite/PST/DOCX/XLSX/PPTX-class/TXT/BIN), CJK and
   Unicode names, 5+ level nesting, >260-char long paths, empty folders,
   special characters, ADS, hard links, sparse, compressed, contiguous /
   fragmented / highly-fragmented layout classes, and a keep-control group.
   The manifest (`seed-manifest-<vol>.jsonl`) records per file: path, size,
   SHA-256, timestamps, attributes, extents, MFT record numbers. **That
   manifest is the only ground truth results are compared against.**
3. **Deletion scenarios** — `engine/bench/scenarios/apply-scenario.ps1`, the
   12-mode matrix: recycle-bin delete, permanent delete, native
   `DeleteFileW` unlink, folder delete + empty bin, quick format, full
   format, partition delete (entries removed, data untouched), overwrite
   decay (fill to 50/80/100% then delete the fillers), partial overwrite
   (0xAA over the first 8 sectors of each deleted extent), MFT corruption +
   `$Bitmap` XOR. Pre/post sector-range hash maps and changed-block counts
   are captured per scenario — ground truth is provable from the artifact.
4. **The product, both lanes** — CLI lane: `rrecov scan --quick-only |
   --no-carve | default` per mode → `tree` → `recover --report` with full
   stdout/stderr/exit-code/session/log/timing capture and 500 ms process
   sampling (`engine/bench/ours/run-ours.ps1`). API lane: the `cr_ui` C-ABI
   lifecycle open → scan(+events) → session save/reload → query → preview →
   recovery → G-09 report, QPC-timed per phase (`api_harness.exe` compiled
   from `engine/.github/scripts/api_harness.c`).
5. **Divergence check** — `scripts/Compare-Lanes.ps1` proves the CLI and API
   lanes agree on every scenario: identical hash-match sets (byte-identical
   recovered content follows — both must equal the manifest SHA-256),
   identical sizes, matching full-fidelity scan stats. **Any divergence is a
   hard finding** (a scenario fails; the finding is recorded in
   `divergence.json`).
6. **Metrics + evidence** — recovered trees are inventoried against the
   manifest (`engine/bench/metrics/inventory_recovered.py`) and scored
   (`engine/bench/metrics/compute_metrics.py`): recall, precision, F1,
   false-positive classification, path/name preservation, per-mode scan +
   recover walls, CPU, working-set peak, throughput. Results land under
   `results/<run-id>/` in the RESULTS-SCHEMA layout; at least one full run's
   results and the analysis are committed to the repo.

## The reuse discipline (read this before extending)

The suite reuses the CI-proven engine primitives **by invocation, never by
fork** — one copy of code that has survived dozens of recorded CI failures
(worklog/TEST-INVENTORY.md § the reuse map):

| This suite's component | Invokes (proven primitive) |
|---|---|
| `Run-E2E.ps1` (orchestrator) | everything below, in order |
| VHD create/attach/format | `engine/.github/scripts/New-TestDisk.ps1` |
| teardown | `engine/.github/scripts/Teardown-Benchmark.ps1` |
| corpus + manifest | `engine/bench/seed/generate_seed.ps1` |
| deletion matrix | `engine/bench/scenarios/apply-scenario.ps1` |
| CLI lane | `engine/bench/ours/run-ours.ps1` |
| API lane | `engine/.github/scripts/api_harness.c` (compiled with `cl` against `cr_ui.lib`) |
| inventory | `engine/bench/metrics/inventory_recovered.py` |
| metrics | `engine/bench/metrics/compute_metrics.py` |
| regression gate | `engine/bench/metrics/regression_gate.py` |

New code that lives in `tests/e2e/scripts/`: `Run-E2E.ps1`,
`Compare-Lanes.ps1` (the CLI-vs-API divergence check), `Collect-Results.ps1`
(results assembly + completeness gate), `New-Baseline.ps1` (baseline
candidate builder), `Assert-Clean.ps1` (the pre-commit hygiene gate).

## How to run locally (a Windows admin PowerShell)

```powershell
# 1. build the engine kit (the release recipe)
./build.ps1 -Target engine -SkipTests

# 2. stage the kit in the release-zip shape Run-E2E.ps1 expects
$version = (Get-Content VERSION -Raw).Trim()
$staged  = 'D:\e2e-bench\core'
New-Item -ItemType Directory -Force -Path "$staged\bin" | Out-Null
Copy-Item "build\out\stage-$version\engine\*" "$staged\bin" -Force

# 3. compile the API harness (x64 Native Tools prompt)
cl /nologo /std:c17 /O2 /W4 /DWIN32_LEAN_AND_MEAN `
   engine\.github\scripts\api_harness.c /I engine\include `
   /Fe:"$staged\bin\api_harness.exe" /link "$staged\bin\cr_ui.lib"

# 4. run one shard (stateless: creates its own disks, tears them down)
.\tests\e2e\scripts\Run-E2E.ps1 `
    -MatrixPath tests\e2e\matrix.json `
    -RepoRoot $PWD `
    -CoreStaged $staged `
    -ApiHarness "$staged\bin\api_harness.exe" `
    -WorkRoot 'D:\e2e-bench' `
    -ResultsRoot 'D:\e2e-results' `
    -Profile full -ShardId ntfs-basic

# or the whole matrix on one machine (all 45 scenarios, serial):
#    ... -Profile full   (omit -ShardId)

# 5. inspect the verdict
Get-Content D:\e2e-results\gates.md
```

Runner requirements: elevated PowerShell (raw-disk verbs + `--disk N` scans),
free space on the work drive (matrix disks + 20 GiB margin — the setup script
probes and refuses with the exact numbers), `python` on PATH for the metrics
tools.

## How to run in CI

| Workflow | Trigger | Matrix | Lanes | Retention |
|---|---|---|---|---|
| `e2e-smoke.yml` | pull_request | 4 smoke scenarios (NTFS permanent-delete, exFAT recycle-bin, FAT32 quick-format, GPT partition-delete) | CLI only (~10 min; the API-lane skip is the recorded design reason) | 7 days |
| `e2e-full.yml` | workflow_dispatch + nightly 02:30 UTC | all 45 scenarios, 9 parallel shards | CLI + API + divergence | fragments 30 d, merged run 90 d |
| `e2e-regression.yml` | workflow_dispatch | all 45 scenarios (9 shards) | CLI + API + divergence + baseline gate | 30 d |
| `release-validation.yml` | workflow_dispatch | all 45 scenarios vs the installed release product | CLI (installed exe) + API (shipped DLL bits) | 90 d |

All four: windows-2022 runners (the execution-validated runner class),
SHA-pinned actions, no `continue-on-error` anywhere, failure evidence
uploaded via `if: always()`, and every stage that cannot execute fails with a
diagnostic artifact — never a silent skip.

**Runner choice, documented:** GitHub-hosted windows-2022 runners execute the
whole suite — diskpart creates/attaches VHDX without the Hyper-V role (the
bench lane's recorded design), raw-disk verbs work under the runner's
elevated token, and ReFS formats on the Datacenter image. A self-hosted
Windows runner becomes preferable only when the full matrix's nightly compute
(~9 runners × ~1–2 h) matters more than hermetic reproducibility; that is a
documented option, not the current choice.

## How to add a scenario

1. Add the row to `matrix.json` `scenarios` (id, mode, disk, volume,
   target_mode, scan_modes, gate) — the deletion `mode` must be one
   `apply-scenario.ps1` implements, or you must extend that script first.
2. Assign it to a shard (or add a shard). Keep `delete_partition` rows LAST
   on their disk.
3. Document it in `SCENARIOS.md` (seed, deletion method, expected
   recoverability characteristics, gate rationale with citations).
4. Add the baseline expectations when a measured run exists
   (`New-Baseline.ps1` picks it up automatically from
   `04-scenarios/<id>/cli/metrics-deepcarve.json`).

## How to read results

- `gates.md` — the human gate table (scenario × gate × CLI/API recall ×
  verdict).
- `run.json` — the machine anchor: run id/URL, commit, product SHA-256,
  scenario index, completeness (missing scenarios = collector failure).
- `04-scenarios/<id>/` — `scenario.json` (definition + manifest SHA),
  `state/` (deletion record + hash maps), `cli/` + `api/` (full lane
  captures: stdout/stderr, sessions, trees, reports, inventories, metrics,
  preservation probes, sample recovered files), `divergence.json`,
  `verdict.json`.
- `03-baseline/` — the seed manifests (the ground truth of record for the
  run).
- `99-teardown/` — the VHD detach/delete log (zero leftover mounts is
  asserted by teardown; the log is the proof).

Interpretation rules are in `METRICS.md`; the JSON layout is normative in
`RESULTS-SCHEMA.md`; regression thresholds and the baseline policy are in
`BASELINE.md`.

## Evidence commitment policy

CI produces artifacts; the repo carries evidence. After a green `e2e-full`
run: download `e2e-full-results-<run_id>`, copy it to
`tests/e2e/results/<run-id>/`, write `analysis/<run-id>.md`, run
`Assert-Clean.ps1`, and commit in a reviewed evidence-citing commit
(virtual-disk images are NOT committed — the sector-range hash maps +
sessions + reports are the provable ground truth, and the bounded-artifacts
rule holds; CI artifacts carry the byte-level payloads under retention).
