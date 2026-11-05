# PERF-LEVERS — the T7–T12 optimization dispositions (evaluated against measured evidence)

> Session-97's lever ledger. The mandate's rule (§2): "Where a consumer's
> pattern does not benefit from any optimization, the baseline is preserved
> and the reason is documented." This sheet is that documentation: every
> lever from ACCESS-LAYER-CURRENT §4 (F1–F14) evaluated against the
> captured measurements, with the arithmetic that decided it.
>
> The wall math that frames everything (run 41140212296, v0.7.0, 8 GiB 512e
> VHDX, windows-2022):
>
> - deepcarve wall 1,014,235 ms for 8,572,108,800 B = **8.45 MiB/s effective**
>   (driver era: 5.70 MiB/s) — while the transport's measured device rate at
>   the consumer's block size is **544 MiB/s** (bench seq 1 MiB). The scan
>   walls are ~64× below the device rate: **the walls are validation-CPU-
>   bound, not transport-bound.**
> - CPU per GiB: 303.8 CPU-s (4 threads) ≈ 76 s wall-equivalent per GiB; the
>   deepcarve wall is ~124 s/GiB — consistent with CPU-bound + pipeline
>   overheads.
>
> Consequence: any transport-side micro-optimization (syscall counts, lock
> scopes, buffer copies) moves microseconds against walls measured in
> hundreds of seconds. The levers are dispositioned per-metric below.

---

## §1 The dispositions

| Lever (ACCESS-LAYER-CURRENT §4) | Metrics it targets | Disposition | The arithmetic / the reason |
|---|---|---|---|
| **F1** CachedReader mutex scope (fill outside the lock → effective QD>1) | rand throughput, worker starvation | **DEFERRED (documented)** | The transport is 64× faster than the wall's consumer rate — zero I/O overlap costs nothing while workers validate. The mutex rework (block-level locking, duplicate-fill avoidance) is the highest-risk concurrency change in the layer, and its payoff (QD2-4 pipelining) is **unvalidatable on the CI runner class**: the VHDX saturates at ~544 MiB/s already at QD1 (the direct 1-MiB leg = the driver's 536.92 within noise). The mandate's own rule — every claim a captured measurement — forbids claiming a QD win no available surface can show. Revisit trigger: a physical-NVMe runner profile or a QD-sensitive fixture. |
| **F2** FILE_FLAG_OVERLAPPED + per-handle slots | seq ceiling on fast media, pipelining | **DEFERRED (documented)** | Same frame as F1: the consumers are QD1 by design (PERFORMANCE §1.3; the og itself serializes QD1 — REFERENCE-ARCHITECTURE checklist #5), the runner's VHDX is the limiter, and the single-flight ABI contract + cancellation rework (CancelIoEx) ride the change. No measurable payoff exists on any current surface; the risk is the ABI contract. Revisit with F1. |
| **F3** coalesced sequential fills (1–4 MiB inner reads) | seq CPU/byte | **NOT TAKEN (negligible)** | 4→1 syscalls per MiB saves ~3 × 2 µs ≈ 6 µs/MiB ≈ **6 ms per GiB** against a 124 s/GiB wall (0.005%). The LRU must still memcpy per block (blocks are separate allocations); only the syscall count drops. |
| **F4** sector-granular envelope for sub-4 KiB reads | probe/table/grading latency | **NOT TAKEN (behavior risk > nil gain)** | Sub-4K transport reads are ~10 per scan (probe_family, table LBA0/GPT, LDM PRIVHEAD, identity) — the wall impact is unmeasurable. The REAL effect would be on damaged media: the envelope's 64 KiB rounding sets the ReadErrorMap/salvage granularity; a finer envelope changes recovered bytes on sick disks = an output change the mandate forbids (byte-for-byte equivalence, §3.2). GAP-S6-13's slow-path class is unmeasured for sub-4K lengths — another unknown purchased for nothing. |
| **F5** bench rand alignment control | measurement fidelity | **NOT TAKEN (surface constraint)** | Requires a new `bench` flag — the CLI surface must stay identical (mandate §5). The envelope path IS what consumers' unaligned reads take, so rand measuring it is the honest number (BENCHMARK §6 documents this); the seq legs at consumer-native sizes (64K/256K/4M, landed) give the aligned device profile. |
| **F6** image-create chunk 64 KiB → 1 MiB | image throughput | **NOT TAKEN (negligible)** | 16,384→2,048 iterations per GiB saves ~15k × ~2 µs ≈ 30 ms per 8 GiB; the lane is device-bound both sides (read 544 MiB/s + buffered write). SHA-256 is streaming — chunk size cannot change the digest (output-identical), but the cancel-prefix boundary coarsens (a .partial observable). Nothing worth that trade. |
| **F7** image mutex scope | IMG-lane throughput | **NOT TAKEN** | The IMG lane's consumers are QD1 by design; same frame as F1. |
| **F8** decrypt buffer reuse + batching | BitLocker scan throughput | **DEFERRED (no fixture)** | No benchmark fixture exercises the decrypt path (PERFORMANCE-TARGET §2's unmeasured row); optimizing unmeasured code violates the measure-first discipline. The fixture lands first; the lever re-evaluates against its wall. |
| **F9** corpus-pinned Sleeps | repair/apply latency | **NOT TAKEN (parity contract)** | The og's own quiesce/settle timings; changing them alters verb walls the corpus pins (ACCESS-LAYER-CURRENT F9). Documented as deliberate. |
| **F10** prefetch thread (the og's model, D-72) | seq wall tails | **NOT TAKEN (measured superior without it)** | The reactive 8 MiB LRU already beats the driver by 32% on the deepcarve wall and 26% on the API scan — the divergence is measured-superior at the runner profile (PARITY-DELTAS §2.3). A prefetch thread adds memory pressure + a failure surface for a win the walls cannot show (transport 64× over provision). Revisit only if a profile shows tail latency the reactive cache loses. |
| **F11** checkpoint session rewrite | boundary latency spikes | **NOT TAKEN (measured fine)** | save_session measures 9.3 ms (driver 10.8); the boundary spikes are sub-perceptible at current session sizes. |
| **F12** geometry-probe caching | open latency | **NOT TAKEN (already superior)** | open 9,213 ns vs the driver's 17,019 (−46%); the remaining headroom (per-process geometry cache) buys ~2 µs against a 0.5 ms API open that is already 44% better. |
| **F13** volume-open dance | TTFR | **NOT TAKEN (measured fine)** | API open 0.5 ms total; the 3-open dance is not the interactive surface's cost. |
| **F14** SIMD discovery probe | deepcarve CPU/byte | **NOT TAKEN (mis-sized)** | The probe's inner loop is 2 byte-compares per 512 B sector ≈ 33 ms per 8 GiB — 0.003% of the wall. The deepcarve CPU (303.8 CPU-s/GiB) lives in the carve signature matcher and validators (engine internals, outside the driver-parity frame); reworking the matcher risks output identity for a win the parity objective does not require (already −32% vs the driver). Scoped as future engine work, not this session. |

## §2 The one code change this evaluation produced

The **F12-adjacent defect fix** (worklog/RECALL0-DIAGNOSIS.md) — not a
performance lever but the arming defect the Phase-0 sheets surfaced: the
cross-disk false refusal that zeroed every scenario's recall. It is the
session's only product-code change, separately committed (4d4ef333) with
its own evidence chain and unit suite.

## §3 What WOULD move the walls (the honest frontier, for the next session)

1. **The carve/validation pipeline** (the 303.8 CPU-s/GiB): signature
   matcher vectorization, validation batching, the analysers' early-exit
   order. Output-identity must be proven per change (the carve registry's
   exact-result contract); the measurement vehicle exists (the source
   lane's deepcarve wall + CPU/GiB rows).
2. **QD2-4 pipelining on physical NVMe** (F1/F2): the design sketches are
   in PLATFORM-CAPABILITIES G1; the blocker is a measurement surface that
   can show it (a physical-NVMe runner or a QD-sensitive storage fixture).
3. **The decrypt path** (F8) once its fixture exists.
4. **Cancellation of an in-flight read** (the one registered peripheral
   GAP, PARITY-DELTAS §3): CancelSynchronousIo wiring — decided by the
   cancel_to_stop_ms measurements now captured per dispatch.
