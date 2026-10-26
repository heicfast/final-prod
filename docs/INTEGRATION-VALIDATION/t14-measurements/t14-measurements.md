# T14 — measured latency/throughput vs the retired simulator's budgets

> Evidence artifact for `docs/INTEGRATION-VALIDATION.md` ("Pending evidence" —
> T14). Captured from validation-evidence run **40385809220** at `55cb76f4`
> (2026-10-26T19:33Z, GitHub windows-2022, AMD EPYC 7763, 4 vCPU, 16 GB).
> Raw artifacts: `t14-engine.jsonl` (the C harness against the staged engine
> kit, pinned-header include, quick-stage scan of the runner's own 274 GB
> system volume — 1,899,589 files), `t14-bridge.jsonl` (the `#[ignore]d`
> bridge measurement tests, release, serialized), `runner-context.json`.

## Method

Two measurement legs, both executed by `validation-evidence.yml`:

1. **Engine-side** (`tools/validation/measure-cr-ui.c`): compiled on the
   runner with `cl /std:c17 /O2` against the staged kit, `#include`s the
   PINNED `engine/include/cr/cr_ui.h` (no struct transcription). Times the
   engine calls that back the connector surfaces. The bridge adds only
   marshaling (serde + IPC, sub-millisecond) on top, so engine-side is the
   honest lower bound for every connector budget.
2. **Bridge-side** (`ui/src-tauri/src/commands/results.rs` `t14_validation`):
   the G-7 filter pass over the metadata cache at 10^5/10^6 nodes (the two
   pred shapes the UI sends) and the `NodeMeta` heap cost via a counting
   `#[global_allocator]`.

The retired simulator's documented budgets were artificial latencies the
mock injected to feel realistic; they are the comparison baseline the plan
recorded, not a performance requirement. **Within budget** below means the
real surface lands inside the simulated window.

## Latency table (the simulator's budgets vs the measured engine side)

| Connector surface | Simulator budget (ms) | Measured engine-side | Verdict |
|---|---|---|---|
| `listDevices` | 180±60 (120–240) | enumerate disks+volumes: **p50 9.6 ms, p95 25.2 ms** (20 iterations) | within — an order of magnitude under the window |
| `startScan` | 420±120 (300–540) | prep path (open+create+attach) **not separately timed this round** — measured gap, recorded below; run-entry to first event **2.8 ms**; full quick-stage scan of the 274 GB / 1.9 M-file volume **52.6 s** | prep is a measurement gap (follow-up added to the harness); the 2.8 ms first-event latency is far under the window; the 52.6 s is a REAL full-volume scan, not a latency |
| `queryResults` | 90±50 (40–140) | bridge G-7 filter pass: **7.8 ns/node** (wide) — 7.8 ms at 10^6; **137.7 ns/node** (scoped+search) — 138 ms at 10^6 worst-case; engine paged row reads **0.106 ms / 500 rows** (p50, 1.9 M-node tree); snapshot build **1.66 s** (once per generation) | within for the interactive page path; the worst-case scoped+search pass at the 10^6 scale sits at the window's top edge — recorded as the honest worst case |
| `preview` (requestPreview) | 240±160 (80–400) | open **0.031 ms** + 64 KiB read **0.017 ms** + metadata **0.009 ms** | within — three orders of magnitude under the window |
| `getHealth` | 280±120 (160–400) | **unsupported on this device** (the runner's virtual disk has no SMART passthrough; recorded truthfully, 0 iterations) | not comparable on this hardware — the budget comparison needs a physical disk; recorded as the honest hardware gap |

## G-7 filter cost (bridge metadata-cache filter, `matches_pred`)

| Leg | Nodes | Hits | Total | Per node |
|---|---|---|---|---|
| wide 'all files' pred | 100,000 | 75,000 | 0.76 ms | **7.6 ns** |
| scoped (prefix+category+search) | 100,000 | 5,556 | 13.6 ms | **135.5 ns** |
| wide 'all files' pred | 1,000,000 | 750,000 | 7.8 ms | **7.8 ns** |
| scoped (prefix+category+search) | 1,000,000 | 55,556 | 137.7 ms | **137.7 ns** |

The scoped+search leg dominates because the search atom lowercases every
name (allocation per row); the wide pass is a pure predicate walk. At the
shipped page size (500 rows) the filter cost is invisible; the numbers above
are the FULL-cache pass.

## Memory bounds

| Bound | Measured / pinned |
|---|---|
| `NodeMeta` cache (bridge) | **213 bytes/node heap** (struct 144 B + name/path strings) — 213.6 MB at 10^6 nodes; the 1.9 M-file runner tree would hold ≈ 404 MB |
| Engine snapshot + tree (harness peak working set) | 2.05 GB after the scan, 2.42 GB after the paged walk of the 1.9 M-node tree (the harness also holds the 1.9 M×4 B dfs array; the engine's own structures dominate) |
| Bad-sector map cap | **4,096 rows** (pinned in the test output; count stays exact beyond the cap) |
| Scan event channel | **512 deep** (bounded sync channel; full = UI slow, emitter coalesces) |
| Recovery progress channel | **64 deep** (freshest-sample semantics) |

## Honest gaps recorded (not hidden)

- **startScan prep timing** — the harness timed run-to-first-event and the
  full scan, but not the open/create/attach prep path the connector's
  `startScan` actually awaits before returning the scanId. The prep calls
  are the same class as the measured enumerate/geometry calls (sub-ms to
  single-digit ms), but that is inference, not measurement. The harness now
  carries a prep-phase timer for the next run.
- **Health on real hardware** — SMART passthrough needs a physical disk;
  the runner's virtual disk truthfully reports unsupported. The 280±120 ms
  budget comparison requires a bare-metal run (or a passthrough-enabled
  self-hosted runner).
- **Quick-stage throughput units** — `cr_ui_scan_telemetry.bytes_scanned`
  stays 0 across the quick (MFT-walk) stage by engine semantics (it counts
  carve-true bytes); the honest quick-stage metric is **36.1k files/s**
  (1,899,589 files / 52.6 s). A bytes/s figure applies to the deep/carve
  stages, not measured in this quick-only run.
- **stage_mask=1** — the run used quick-only to bound runner time
  (`--quick-only`); deep+carve throughput on a real volume is the
  comparative-bench harness's domain (`engine/bench/`), not this leg.
