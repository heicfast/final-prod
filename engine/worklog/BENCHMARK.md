# BENCHMARK — the performance measurement harness and its CI workflows

> Session-97 T6 deliverable (the mandate §6: "the benchmark harness and its
> CI workflows are documented in worklog/BENCHMARK.md and are a first-class
> deliverable — future sessions must be able to reproduce every number this
> session reports"). Companion: `worklog/PERFORMANCE-TARGET.md` (the gates
> and tolerances), `worklog/PERFORMANCE.md` (the ledger), and
> `bench/baseline.json` (the versioned rows).

---

## §1 The two lanes and their contract

| | `test-benchmark.yml` (the release lane) | `benchmark-src.yml` (the source lane, session-97) |
|---|---|---|
| Measures | PUBLISHED RELEASES only (fetch-release.ps1 enforces SHA256SUMS + MANIFEST, never a build from source, never a fallback to an older tag) | the checked-out commit, built with the release recipe (same preset, same vcpkg manifest, same staging shape) |
| Exists because | the product's release-quality gate | the perf discipline (PERFORMANCE-TARGET §6): every optimization is measured BEFORE release |
| Dispatch | `workflow_dispatch` (core_tag, profile=smoke/full, scan_modes, reason) | `workflow_dispatch` (profile, scan_modes, run_tests, reason) |
| Concurrency | group `test-benchmark`, cancel-in-progress | group `benchmark-src`, queued (measurement runs are never dropped) |
| Runner | windows-2022, 120/180-min timeout | windows-2022, 300-min timeout (build + matrix) |
| Binary provenance | the release zip's SHA256SUMS (resolved-release.json in evidence) | source SHA + build hashes (staged-core.json in evidence) |
| Rest | identical: same `Run-Benchmark.ps1` matrix, same evidence tree, same gates | identical |

Both lanes produce the same structured evidence artifact (see §4) — a number
from either lane is comparable with the other (same verbs, same runner class,
same disk profile); the only delta is the binary's provenance, recorded in
`00-environment/`.

## §2 The matrix (Run-Benchmark.ps1 — the orchestrator both lanes share)

Phases, in order:

0. **The disk environment** (New-TestDisk.ps1): the matrix's fixed VHDX
   disks (small-fat32 2 GiB; multi-gpt 5 GiB with CRNTFS p2 + CREXFAT p3;
   large-ntfs 8 GiB CRBIG), each seeded with the deterministic 23-format
   corpus (generate_seed.ps1: CJK names, long paths, ADS, hard links,
   sparse, fragmented, nested, many-small, keep-control group). Ground
   truth is ONLY the seed manifest + the deletion method.
1. **Feature probes**: health, fix dry-run, user-signs, carve-registry
   parity, cancel+resume (the cancel probe now records
   `cancel_to_stop_ms` — session-97's T11 metric).
2. **The clean-volume benchmark** (the headline numbers): per-mode scan
   walls (quick/deep/deepcarve) — process-sampled since session-97
   (`samples-<mode>.csv`: t_ms, working_set_bytes, cpu_seconds → the
   CPU-per-byte and WS-peak rows); the bench verb sweep
   (`info` + seq at 1 MiB/64 KiB/256 KiB/4 MiB + rand at 1 MiB/4 KiB +
   `open`, each JSON tagged with a `leg` label); the API lane
   (api_harness: open → scan(mask 7, 4 threads) → save → query → preview
   (timed: `probe_ms`) → recovery → report), all into
   `07-performance/clean-volume-benchmark.json`.
3. **The scenario loop** (7 smoke scenarios: S-DEL-NTFS/EXFAT/FAT32,
   S-RECYCLE-NTFS, S-BINEMPTY-NTFS, S-FMT-FAT32, S-PART-DELETE):
   deterministic reseed → apply-scenario (the only ground-truth mutation;
   delete_partition scenarios get the session-97 table-settle guard — two
   consecutive identical partition snapshots before the scan lanes) →
   CLI lane (run-ours.ps1: scan/tree/recover per mode, process sampling,
   transport assertion) → API lane (api_harness) → inventory+metrics →
   gate evaluation → post-actions (the APPLY probe) → cleanup. The
   no-silent-skip throw carries its own diagnosis (exit code, transport
   lines, stream tails) and the catch archives every lane diagnostic
   (session-97: run 41140212296 archived nothing on its S-PART-DELETE
   abort).
4. **The verdict**: 06-comparison (summary.json, gates.md — the per-scenario
   gate table) + 07-performance (perf-summary.json with the per-scenario
   walls + CPU/WS transcription of the samples CSVs).
5. **The performance regression gate** (session-97): the versioned
   driver-parity rows from `bench/baseline.json` (`perf.rows`) are
   evaluated against this run's capture — seq/rand/open bench legs, the
   scan walls, the API timings — each with its tolerance. A row that
   violates its tolerance FAILS the run; a metric that cannot be FOUND in
   the capture FAILS the run too (no silent skip: a metric that stops
   being measured is a gate break). Output:
   `06-comparison/perf-gate.json` + the gates.md table.

Exit code: 1 iff any HARD gate failed (scenario gates, perf-gate rows,
lane completion rules). A failure without its evidence artifact is a
second failure (upload is `if: always()`).

## §3 The measurement rules (PERFORMANCE-TARGET §6, restated)

1. The product's own verbs only (`rrecov bench` + the scenario matrix +
   the API harness) — never a side harness.
2. Runner class windows-2022; disk profile: the matrix's 8 GiB 512e VHDX
   for the headline rows; threads 4, mask 7.
3. Run-ID provenance for every number; dispatch reasons carry the session
   and TODO (`session-97-T5-...`).
4. Headline verdicts need 3-run medians before they are final
   (single-sample verdicts are marked as such).
5. Tolerances (the noise bands, PERFORMANCE-TARGET §0): throughput ±3%,
   latency ±10%, open ±15%, CPU/byte ±5%, memory ±10%.
6. The baseline changes ONLY by reviewed commit with evidence
   (bench/baseline.json carries its own provenance comments).

## §4 The evidence tree (both lanes, always uploaded)

```
00-environment/   env manifest (lane, source SHA or release tag, reason),
                  machine.json, disk-list, staged-core.json (hashes)
02-disk-setup/    disk-state.json (per-disk numbers/letters/partitions)
03-baseline/      seed manifests + pre-deletion listings (the truth)
04-scenarios/<id>/{cli,api,state}  per-lane logs, trees, reports,
                  inventories, metrics, samples, verdicts
05-feature-probes/  health, fix dry-run, user-signs, carve parity,
                  cancel+resume (cancel_to_stop_ms), harness build log
06-comparison/    summary.json + gates.md + perf-gate.json (session-97)
07-performance/   clean-volume-benchmark.json (the sweep + sampling),
                  perf-summary.json (walls + CPU/WS), bench-*.stdout,
                  api/api-result.json, sessions
99-teardown/      detach + cleanup records (the M-061 contract)
```

## §5 Reproducing any number in worklog/PERFORMANCE.md

1. Find the row's run ID in PERFORMANCE.md.
2. The run's artifact `test-benchmark-evidence-<profile>-<runid>.zip` or
   `benchmark-src-evidence-<profile>-<runid>.zip` carries the evidence
   tree; the row's "Source" column names the file inside it.
3. To re-measure: dispatch the lane with the same profile/scan_modes and a
   reason carrying your session; compare against the same file paths.
4. The gate table itself: `bench/baseline.json` (rows, tolerances,
   provenance) — evaluate mentally against the artifact's
   clean-volume-benchmark.json, or re-dispatch and read
   06-comparison/perf-gate.json.

## §6 Known limitations (honest record)

- The runner's VHDX is a virtual disk on the runner's physical media —
  absolute throughput numbers are runner-class-relative; the comparison
  discipline (same class, same profile, run-ID'd) is what makes them
  meaningful. The historical direct-vs-driver gap datapoint (run
  40227375329, −10.5% @256 KiB) is re-measured by the session-97 sweep
  legs on every dispatch.
- rand-leg offsets are LCG-deterministic but generally NOT 64-KiB-aligned
  → they measure the ENVELOPE path (offset-down/length-up + bounce), the
  same path the consumers' unaligned reads take (ACCESS-LAYER-CURRENT
  F5). A future alignment-controlled leg is scoped in PERFORMANCE-TARGET
  §2 (the QD lever's precondition).
- CPU/WS rows for the clean volume exist only from session-97 forward
  (phase 2 was unsampled before); the driver-era CPU/WS baselines are
  scenario-lane numbers (cross-workload parity bound — the perf gate's
  scan_cpu_ws rows activate by reviewed commit once same-workload
  direct-reference medians exist).
- The scenario recall floors remain at their versioned values
  (bench/baseline.json scenarios section); the session-97 F12 arming fix
  (worklog/RECALL0-DIAGNOSIS.md) is what makes them achievable again —
  the next release-lane dispatch against a post-fix release is the
  validating capture.

## §7 The CI wiring

- `checks.yml` (push/PR): the format/lint/build/test gates + the PS AST
  parse gate (every dispatched script) + the YAML parse gate (every
  workflow + every inline pwsh block) — the harness itself is gated.
- `test-benchmark.yml`: release-dispatched after every publish; manually
  dispatchable any time.
- `benchmark-src.yml` (session-97): the optimization-measurement lane;
  a future push-gated perf-gate-only fast lane (matrix skipped, §2's
  phase-2 + phase-5 only) is a scoped extension, not yet built.
- `installer.yml`, `release.yml`: unchanged by this session (core-only
  since session-96).
