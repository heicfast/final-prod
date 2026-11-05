# PERFORMANCE-TARGET — the session's gate document (what "done" means, numerically)

> Consolidated at T3 from the five Phase-0 sheets (REFERENCE-ARCHITECTURE /
> ACCESS-LAYER-CURRENT / DRIVER-BASELINE / CONSUMER-PERFORMANCE /
> PLATFORM-CAPABILITIES, all at HEAD `5a8acff0`). This document is the
> single gate every optimization and every CI verdict is judged against.
> Companion: `worklog/PERFORMANCE.md` (the measurement ledger),
> `worklog/BENCHMARK.md` (the harness, T6).
>
> Rule (mandate §2): a consumer's metric is either MEETS (within noise of
> baseline), EXCEEDS (better than baseline beyond noise), or GAP (worse
> than baseline beyond tolerance). A GAP on any metric that matters to a
> consumer is a defect, not a trade-off. UNMEASURED is not a verdict —
> it is unfinished work.

---

## §0 The verdict algebra

- **Noise band** (from the captured inter-run variance on the windows-2022
  runner class, 8 GiB 512e VHDX profile): throughput ±3%, latency ±10%,
  open-latency ±15% (17,019 → 9,213 ns shows the class's actual spread is
  dominated by implementation, not noise; the band guards runner drift).
- **MEETS** = direct result ≥ baseline × (1 − 0.03) for throughput;
  ≤ baseline × (1 + 0.10) for latencies; ≤ baseline × (1 + 0.05) for
  CPU-per-byte; ≤ baseline × (1 + 0.10) for memory peak; ≤ baseline +
  250 ms for cancellation time-to-stop.
- **EXCEEDS** = better than baseline beyond the noise band.
- **GAP** = outside the MEETS tolerance → the optimization is reverted or
  the cause is diagnosed and fixed. Single-sample GAPs trigger a 3-run
  median re-measure before any verdict is final (the variance discipline;
  run 39807929475's og-driver leg dropped 533.1 → 391.0 MiB/s on a rerun —
  single samples on hosted runners can lie).
- **Baseline of record** = driver v0.6.1 run **41042267586** (07-performance
  section, captured pre-removal with the RDA driver installed and verified)
  + the historical driver-era run IDs in PERFORMANCE.md §1.2. Correction
  recorded at T3 (Agent D's finding): that run hit its 120-min budget
  mid-scenario (4/7 scenarios, no 06-comparison) — its clean-volume
  benchmark section completed BEFORE the budget hit and is valid; its
  scenario walls are partial. No new driver-era captures are possible
  (the lane is core-only by design; re-adding a driver is forbidden).

## §1 The headline gate table (the primary metrics)

| # | Metric (clean-volume, 8 GiB 512e VHDX, windows-2022) | Driver baseline (run 41042267586) | Direct v0.7.0 (run 41140212296 evidence) | Verdict | Tolerance |
|---|---|---|---|---|---|
| 1 | bench seq 1 MiB × 256 MiB | 536.92 MiB/s | 544.17 MiB/s | EXCEEDS (+1.4%) | ≥ 520.8 |
| 2 | bench rand 1 MiB × 1000 | 1993.22 µs / 502 IOPS | 1976.72 µs / 506 IOPS | MEETS/+ | ≤ 2192.5 µs |
| 3 | bench open × 1000 | 17,019 ns | 9,213 ns | EXCEEDS (−46%) | ≤ 19,572 ns |
| 4 | scan quick wall | 150 ms | 27 ms | EXCEEDS (−82%) | ≤ 165 ms |
| 5 | scan deep wall | 16,505 ms | 16,467 ms | MEETS (−0.2%) | ≤ 18,156 ms |
| 6 | scan deepcarve wall (8 GiB) | 1,501,622 ms | 1,014,235 ms | EXCEEDS (−32.4%) | ≤ 1,546,670 ms |
| 7 | API open | 0.9 ms | 0.5 ms | EXCEEDS | ≤ 1.0 ms |
| 8 | API scan (mask 7, 4 threads) | 1,513,221.7 ms | 1,113,445.2 ms | EXCEEDS (−26.4%) | ≤ 1,558,616 ms |
| 9 | API session save | 10.8 ms | 9.3 ms | MEETS/+ | ≤ 11.9 ms |
| 10 | API query | 0.2 ms | 0.1 ms | MEETS/+ | ≤ 0.3 ms |
| 11 | API recovery (82 files) | 49.2 ms | 36.8 ms | EXCEEDS (−25%) | ≤ 54.1 ms |
| 12 | TTFR (deep, clean vol) | 0 ms | 0 ms | MEETS (floor) | ≤ 50 ms |
| 13 | CPU per GiB (deepcarve, samples CSV) | 502 CPU-s/GiB | 305 CPU-s/GiB | EXCEEDS (−39%) — *untranscribed in the run verdicts; transcription is a T6 harness fix* | ≤ 527 |
| 14 | Working set (deepcarve peak) | ≤ 40 MB | ≤ 40 MB (flat) | MEETS | ≤ 44 MB |
| 15 | Files found (1209 live + 2134-2138 carved) | identical shape | identical shape | MEETS (correctness floor) | exact |

Rows 1-12 are re-measured by every benchmark dispatch (the existing
verbs). Row 13-14 require the T6 harness fix (transcribe the existing
samples CSV into perf-summary). Row 15 is the correctness floor — any
deviation is a functional defect, not a performance verdict.

## §2 The per-consumer gate (from CONSUMER-PERFORMANCE §6, the 27 rows)

Verdict distribution at Phase-0: **EXCEEDS 6 · MEETS 6 · GAP 1 ·
UNMEASURED 15 · n/a 2.**

The one GAP: scenario deep wall S-DEL-NTFS 7.56 → 8.10 s (+7.1%,
single sample, clean-volume deep is parity) — re-measure with a 3-run
median (T5/T6) before any optimization is scoped; if confirmed, the
lever is F3 (read coalescing) or F1 (mutex), both parity-preserving.

The 15 UNMEASURED, each with its establishment plan:

| Consumer / metric | Why unmeasured | Establishment plan |
|---|---|---|
| ext4/Btrfs/ReFS/HFS+/ZFS engine walls | no fixture in the matrix | T6: add a 4-volume fixture disk (one per FS, seeded) + per-FS deep walls to the matrix |
| LDM crossdisk reads | no LDM fixture | T6: 2-disk LDM fixture, plan-stage wall |
| BitLocker decrypt path (F8) | no lane exercises it | T6: unlocked-BitLocker VHDX fixture, deep wall vs the same volume unencrypted |
| Lost-partition discovery wall | S-PART-DELETE aborts the lane | T5: fix the harness (settle + evidence), re-dispatch, wall from the scenario |
| Repair (analyze) wall | feature probe untimed | T6: time the existing fix dry-run probe |
| Apply (plan+execute) wall | probe untimed | T6: time the existing APPLY probe |
| Preview TTFB | api_harness probe untimed | T6: add ttfb_ms to the preview probe |
| Thumbnail TTFB | absent from CI entirely | T6: add a thumbnail probe (seeded JPEG corpus) |
| Image create throughput | round-trip probe untimed | T6: time the existing image probe (and F6 lever) |
| Health TTFB | probe untimed | T6: time the health probe |
| Cancellation time-to-stop | cancel fired, never timed | T6: add cancel_to_stop_ms to api_harness (cancel → scan_run return) |
| rand 4 KiB (NTFS record granularity) | verb never run at 4 KiB in the lane | T6: extend the bench verb sweep in Run-Benchmark.ps1 (rand --block 4096) |
| seq 64 KiB / 256 KiB / 4 MiB legs | same | T6: seq sweep at 64K/256K/4M (consumer-native sizes) |
| Volume-size scalability | no ladder | T6: 2/8/32 GiB ladder on the full profile |
| QD>1 transport legs | inexpressible today (F2) | T7: after OVERLAPPED lands, add bench --qd N |

## §3 The correctness invariants that bound every optimization (non-negotiable)

From DRIVER-BASELINE §4 (the driver's guarantees the direct path must keep)
and the mandate §3/§5:

1. **Verbatim error passthrough** — no masking, no zero-fill on failure
   (the Skip-mode zero-fill is the pinned RETRY contract, unchanged), no
   retry that hides a dead disk, `Information=0`-equivalent on failure.
2. **Byte-exact data fidelity, read-size independence** — any 512 B–16 MiB
   read returns the same bytes; the 64 KiB envelope + bounce must survive
   or be replaced by an equally tolerant mechanism (GAP-S6-13).
3. **Geometry/IOCTL fidelity** — every passthrough IOCTL's status AND
   output length preserved (the CI geometry-passthrough assertion).
4. **Cancellation** — cancel must abort an in-flight read promptly with
   the same observable states (STATUS_CANCELLED-equivalent error shape).
5. **Lifecycle** — close releases everything; concurrent shared opens
   unchanged; the per-handle single-flight ABI contract (cr_transport
   comments) preserved or consciously widened with tests.
6. **Synchronous ordering** — the consumers are QD1 by design (PERFORMANCE
   §1.3); any pipelining must not reorder observed reads per handle.
7. **Output equivalence** — sessions, trees, reports, recovered bytes
   byte-identical on the same seeded disks (T15's matrix proves it).

## §4 The optimization levers, in priority order (mapped to the TODOs)

Ordered by (expected metric impact × consumers touched ÷ risk):

| Pri | Lever (ACCESS-LAYER-CURRENT §) | Metrics | Consumers | Risk | TODO |
|---|---|---|---|---|---|
| 1 | **Harness truth first**: stale 'rda' gate fix, exception-path evidence archive, S-PART-DELETE settle+diagnose, CPU/WS transcription, bench size sweep | measurement fidelity | all | none (scripts only) | T5/T6 |
| 2 | **F4 sector-granular envelope for sub-4 KiB reads** (64 KiB stays for ≥4 KiB) | probe/table/check/grading latency + CPU/byte | orchestrator, table, LDM, check, grading | low (envelope semantics preserved; unit tests + E2E) | T7 |
| 3 | **F5 bench rand alignment control** (measure the device, not the bounce) | measurement fidelity | bench verb | none | T7 |
| 4 | **F1 CachedReader lock scope** (fill outside the mutex; block-level, not transport-level) | rand throughput, worker CPU starvation | carve, remnant, candidates | medium (concurrency correctness; needs TSan + tests) | T8 |
| 5 | **F2 FILE_FLAG_OVERLAPPED + per-handle slot array (QD2-4)** | seq throughput on fast media, pipelining expressibility | carve, remnant, image | high (single-flight contract, cancellation rework — CancelIoEx) | T8 |
| 6 | **F3 large sequential fills for known-sequential consumers** (1-4 MiB inner reads) | seq CPU/byte, deepcarve wall | carve, remnant, FAT table loads | low | T7 |
| 7 | **F6/F7 image path: 1 MiB chunks + unbuffered dest + (post-F2) overlap** | image create throughput | image create, IMG lane | low | T9 |
| 8 | **F8 decrypt buffer reuse + sector batching** | BitLocker scan throughput | decrypt device | low | T9 |
| 9 | **F10 prefetch (the og's documented model, D-72)** | seq wall tails | all seq consumers | medium (memory bounds; the og's 10-run ring is the corpus shape) | T8/T12 |
| 10 | **F14 SIMD/multi-window discovery probe** | deepcarve CPU/byte | discovery | low | T9 |
| 11 | **Cancellation latency probe + fix if GAP** | cancel-to-stop | interactive scans | low | T11 |
| 12 | **Memory boundedness audit at 32 GiB ladder** | peak WS | large scans | low | T12 |

Levers NOT taken this session (documented, with reasons):
- **F9 corpus-pinned Sleeps** (quiesce/restore/settle) — og-parity
  contract; touching them changes verb walls the corpus pins. Documented
  as deliberate non-optimization.
- **NO_BUFFERING on the transport** — changes buffer ownership across the
  CachedReader boundary and the ABI's allocation contract; only viable
  post-F2 with a dedicated lane; deferred with this record (revisit
  evidence: PLATFORM-CAPABILITIES G2/G7).
- **ReadFileScatter/IOCP full rework** — the consumers are QD1 by design;
  the win is confined to imaging + hypothetical future engines. Deferred.

## §5 The parity workstream (from REFERENCE-ARCHITECTURE §5, the 64-item checklist)

[ M ] 31 matches · [ D ] 15 divergences (already registered or to
register in PARITY-DELTAS) · [ U ] 9 unknown (check-tasks for T14) ·
[DD] 9 deliberate divergences (to document with evidence). The 9 [U]:
MFTMirr fallback, layout-IOCTL retry, writability probe, ext4 journal
gate, RawConfig toggles, pause verb, candidate scoring, memory-adaptive
batching, exFAT windows-absence. Each becomes either a parity fix or a
documented deliberate divergence in T14 — none silently ignored.

## §6 The measurement protocol (every number this session reports)

1. **Vehicle**: the product's own verbs — `rrecov bench
   {info,seq,rand,open}`, the scenario matrix, the API harness — never a
   side harness, per PERFORMANCE.md §0.
2. **Runner class**: windows-2022 (the execution-validated class), same
   disk profile (8 GiB 512e VHDX for the headline table; the T6 ladder
   adds 2/32 GiB), threads 4, mask 7 where applicable.
3. **Dispatch**: `test-benchmark.yml` workflow_dispatch with
   `core_tag=<release>` + `reason=session-97-<T#>`; the run ID is the
   number's provenance. Unreleased optimizations measure through the
   T6 source-build lane (`benchmark-src`, build-from-source + same
   matrix) — never an ad-hoc local build.
4. **Repetition**: headline rows re-measured ≥ 3 runs (median) before any
   EXCEEDS/GAP is claimed final; single-run verdicts are marked
   "single-sample".
5. **Recording**: PERFORMANCE.md §2 rows carry baseline, result, run ID,
   verdict, delta — no number without its run ID.
6. **The regression gate (T6)**: CI fails if any §1 row violates its
   tolerance vs the versioned baseline JSON (bench/baseline.json evolves
   to carry the per-metric tolerances); a benchmark that cannot run
   fails the job with its evidence artifact (no silent skip — the
   existing lane's rule, kept).

## §7 The completion gate for this session (the T19 checklist)

1. Every §1 row: final verdict MEETS or EXCEEDS with ≥ 3-run medians and
   run IDs.
2. Every §2 UNMEASURED row: measured (or its fixture landed + first
   capture recorded), or the gap documented with precise scope and a
   deferred-TODO — no silent drops.
3. The one §2 GAP (S-DEL-NTFS deep wall): 3-run median verdict + either
   fixed or diagnosed-and-documented.
4. Harness defects: stale-gate fixed, evidence-archive fixed,
   S-PART-DELETE root-caused with archived evidence, CPU/WS transcribed,
   scenario gates re-baselined honestly (the recall-0 pre-existing state
   dispositioned — fixed if harness-side, defect-filed if product-side).
5. The correctness matrix (T15) byte-for-byte green on the full profile.
6. PARITY-DELTAS updated: the 9 [U] items dispositioned, every divergence
   deliberate-and-evidenced or removed.
7. CI fully green on main for the right reason (T17), docs current (T18),
   worklog.md complete with every SHA and run ID (T20).
