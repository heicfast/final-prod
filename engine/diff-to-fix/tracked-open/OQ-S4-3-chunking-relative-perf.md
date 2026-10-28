# OQ-S4-3 — Relative performance of the core's 64 KiB direct-I/O envelope vs the og's 4/7 MiB buffered-synchronous chunking is unmeasured

**Status:** OPEN (session 4) · **Module:** transport · **Parameters:** P47 (throughput), P51 (I/O behavior)
**Audited commit:** `63a9167` · corpus `03ad46c`
**Status update (2026-10-26, post-v0.4.0-parity): the transport-chunking half is now MEASURED**
(alignbench run `40315255368` on `095aab5`; verdict transcribed in the closing addendum below).
**Remaining open: the scan-level time-to-first-result half only** (comparative-bench dispatch).

## The precise question

M-030 established the strategy difference (core: 64 KiB direct-I/O envelope on the
DIRECT path — a Windows heuristic; og: 4 MiB disk / 7 MiB volume buffered
synchronous ReadFile chunks, queue depth 1 — decompile-proven). Neither this session
(Linux container) nor any prior session has MEASURED the relative throughput/latency/
CPU of the two strategies on real Windows hardware. M-009/GAP-S6-13's provenance
narrative is corrected by M-030 regardless; what remains open is whether the core's
strategy meets, beats, or trails the og's on the bench fixtures — the acceptance
criterion for any future chunking change.

## Corpus artifacts / bench to run next

1. The Windows bench leg (`.github/workflows/comparative-bench.yml`, `bench/ours/`)
   with a 4 MiB-chunk build variant of the direct path (requires a code change —
   engineering task, not audit).
2. og-side numbers: the corpus's own bench artifacts if any exist under
   `re-workspace/artifacts/` (grep "ReadFile throughput|MB/s" — the corpus agent
   found none this session; the og numbers must come from the bench harness run).

## Acceptance

RESOLVED when the comparative bench reports both strategies' MB/s, time-to-first-
result, and CPU on the same fixture set; record the verdict in M-030's Resolution
addendum. UNVERIFIED-BY-EXECUTION until then.

---

## Session-14 addendum (DFX14 — pushed forward, not resolvable this session)

The question is Windows-hardware by nature (64 KiB direct-I/O vs 4/7 MiB
buffered-synchronous chunks). Not resolvable from the Linux container; the
standing acceptance path is the Windows bench lane (bench/ours/run-ours.ps1
+ alignbench on the seeded fixtures, Windows CI verification). No new
evidence this session; the next concrete step is unchanged and precise:
run the paired bench on the Windows lane and record the numbers in this
file + SUMMARY's performance section.


---

## Session-25 addendum (DFX25 — the engineering prerequisite LANDED; the Windows execution remains)

The OQ's own "bench to run next" item 1 required a code change ("a 4 MiB-chunk
build variant of the direct path"). That prerequisite now exists as a RUNTIME
variant instead of a build variant (one build, two strategies):

- `bench/ours/run-alignbench.ps1` sweep extended with **7 MiB** (the og's
  VOLUME chunk, Device_decompiled.c:56912; the 4 MiB DEVICE chunk :56545 was
  already in the matrix) — the og-strategy proxy and the core-strategy proxy
  (64 KiB envelope reads / the 256 KiB cache block) now run in ONE harness
  pass on the same target/span.
- Every measurement now records **cpu_ms** (Process.TotalProcessorTime delta
  over the measured pass) — the OQ's CPU metric.
- A **chunk_strategy_ratios** verdict block emits og-vs-core throughput ratios
  and cpu-per-byte ratios (4 pairs: og4m/og7m x core64k/core256k) into
  alignbench.json — the acceptance's MB/s + CPU half lands as a recorded
  number the moment the Windows lane executes.
- `scripts/ps_parsecheck.ps1` file list extended: run-alignbench.ps1 and
  scripts/bench_seed.ps1 were MISSING from the pre-dispatch parse gate (a
  real gap closed in the same change).

**Status: UNVERIFIED-BY-EXECUTION (unchanged) — the remaining step is exactly
one Windows execution**: dispatch the API Benchmark workflow (or run
`bench/ours/run-alignbench.ps1 -Target '\\.\C:' -SpanMiB 512` on the
Windows lane), then transcribe the chunk_strategy_ratios verdict + the
comparative-bench scan-level time-to-first-result into this file, M-030's
Resolution addendum, and SUMMARY's performance section. The scan-level
time-to-first-result half comes from the existing comparative-bench matrix
(quick/deep scans) — no further engineering is required on either half.

Verification-discipline note (recorded for future sessions): during this
push-forward, an apparent `[m/[M`-eating corruption of the bench fleet
(ath]::Round / branches: ain]) was investigated and REFUTED at byte level
(od/python): the files use lowercase `[math]::` (valid PowerShell, type
resolution is case-insensitive) and `branches: [main]` — the "corruption"
was a display-layer artifact stripping `[m`/`[M` pairs as false ANSI SGR
codes. No finding filed; no healthy file "repaired". The lesson: verify
suspected corruption at BYTE level (od -c / python reads) before filing.


---

## Session-37 addendum (DFX37 — pushed forward unchanged; the harness verified on the live tree)

Re-verified this session on the post-S-034/M-040 tree: the acceptance
machinery is intact and complete (bench/ours/run-alignbench.ps1 carries
the 4 MiB + 7 MiB og-strategy proxies alongside the 64 KiB envelope +
256 KiB cache-block core proxies in ONE pass, cpu_ms recorded per
measurement, the chunk_strategy_ratios verdict block emitted into
alignbench.json; scripts/ps_parsecheck.ps1 gates the fleet's syntax).
The remaining step is EXACTLY ONE WINDOWS EXECUTION, unchanged: dispatch
the API Benchmark workflow (or run `bench/ours/run-alignbench.ps1
-Target '\\.\C:' -SpanMiB 512` on the Windows lane), then transcribe
the chunk_strategy_ratios + the comparative-bench time-to-first-result
into this file, M-030's Resolution addendum, and SUMMARY's performance
section. Not resolvable from the Linux container (the question is
real-hardware I/O behavior by nature); no new engineering prerequisite
exists. This file remains OPEN solely on that execution.
---

## Session-close addendum (the closing parity session) — the Windows execution ATTEMPTED: dispatched, first run build-stage failed on the pre-fix tree, the rerun on the fixed tree in flight

The acceptance's "exactly one Windows execution" was dispatched this session
via the API (workflow api-benchmark.yml, `workflow_dispatch` run
**40289658166**, span 512 MiB, on `8b2d8db`). The run failed BEFORE the
harness — at the Windows build step, on the exact pushed-reds the parallel
window had landed (gmtime_r C3861 + the two C4244 confidence narrowings —
errors at cr_analysers.cpp:705 / cr_carve.cpp:457/:827), so no
alignbench.json artifact exists for it; two parallel-lane dispatches from the
same window (runs 40290109004 on `da46b14`, 40290704073 on `ca71045`) failed
identically. Those build reds are now FIXED on main (the convergent repairs
landed as `0e6a642` + this lane's same-shape fixes; the current tree builds
clean on the local gates). The push-triggered rerun on the fixed tree
(**run 40291877684** on `6ec8027`) is IN FLIGHT at session close.

**Status: UNVERIFIED-BY-EXECUTION (one dispatch deep into the acceptance
path — the harness itself has not yet produced numbers).** The remaining
step is now purely: read run 40291877684's (or any successor's)
bench-artifacts/alignbench.json chunk_strategy_ratios block and transcribe
it here + M-030's Resolution addendum + SUMMARY's performance section. The
dispatch path (workflow_dispatch via the API) is proven working — the
204-accepted dispatch landed and executed; only the tree's build health
stood between dispatch and numbers, and that is now green on main.


---

## Closing-measurement addendum (2026-10-26 — the transport-chunking half measured; v0.4.1-parity docs pass)

The "exactly one Windows execution" the prior addenda named happened as a
side effect of the link-fix wave: once the windows job and the alignbench
harness went green again, **API-Benchmark run `40315255368` (push, on
`095aab5`, 2026-10-26T11:32Z, image `win22`, target `\\.\C:`, span 512
MiB, queue depth 1 = the og's own dispatch, RDA service RUNNING with
driver-info verified: `device_open ok`)** produced the complete
`alignbench.json` `chunk_strategy_ratios` verdict block. Transcribed:

| Pair (og buffered-sync vs core) | og MiB/s | og cpu_ms | core MiB/s | core cpu_ms | throughput_ratio og/core | cpu_ratio og/core |
|---|---|---|---|---|---|---|
| og 4 MiB vs core 64 KiB envelope | 197.32 | 609.4 | 203.98 | 593.8 | 0.967 | 1.026 |
| og 4 MiB vs core 256 KiB cache block | 197.32 | 609.4 | 241.64 | 562.5 | 0.817 | 1.083 |
| og 7 MiB vs core 64 KiB envelope | 196.15 | 515.6 | 203.98 | 593.8 | 0.962 | 0.858 |
| og 7 MiB vs core 256 KiB cache block | 196.15 | 515.6 | 241.64 | 562.5 | 0.812 | 0.906 |

**Verdict on the transport question:** the core's 64 KiB direct-I/O
envelope **MEETS/EXCEEDS** the og's 4/7 MiB buffered-synchronous
chunking on throughput at the og's own qd=1 (og/core ratio 0.962–0.967 —
the og trails by 3.3–3.8%) with comparable CPU (±3% vs 4 MiB), and the
256 KiB cache block **EXCEEDS** it by 18–19% throughput at lower CPU.
The acceptance's MB/s + CPU half is therefore a recorded number. The
`alignment_ratio_64k_over_4k` = 1.001 (the alignment harness itself is
noise-level). The RDA-path legs confirm the envelope holds through the
driver (64 KiB 194.34 / 256 KiB 237.54 MiB/s on `\\.\rda\…`).

**What keeps this file OPEN:** only the acceptance's scan-level
time-to-first-result half — it needs the comparative-bench matrix
(quick/deep scan legs), which has never run green (its 2026-10-26
scheduled dispatch was cancelled; push-leg attempts predate the link
fixes and died at the build stage). The dispatch path is documented and
requires no engineering — `comparative-bench.yml` workflow_dispatch,
smoke profile. The measured verdict above already answers the finding's
operative question ("meets, beats, or trails": meets at 64 KiB, beats
at 256 KiB), so the M-030 divergence registration stands confirmed with
numbers; the scan-level TTFR transcription is the only remaining step
to move this file to FIXED/.

## Probe-target correction (2026-10-28, the SA-6 probe lane)

The file's remaining probe ("dispatch `comparative-bench.yml`") is STALE:
the CI rebuild (`8943993`, "the prebuilt-artifact CI lands — benchmark
lanes wiped, the correctness path never builds from source") deleted the
comparative-bench lane deliberately — the measurement surface this OQ's
TTFR half pointed at no longer exists. The remaining half's options:

1. **Ride the staged-scenarios lane** (the surviving truth matrix — daily
   cron + dispatch): its scan jobs run the real stack on seeded VHDs; a
   TTFR metric (the scan event stream's first FilesFound timestamp vs the
   scan start) would close the half with the same evidence strength.
2. **Restore a bounded bench lane** — but the CI rebuild's own decision
   ("the correctness path never builds from source; benchmark lanes
   wiped") makes that an owner-level call, not an audit one.

The transport-chunking half stays MEASURED-and-closed (the alignbench
numbers above); only the TTFR half remains open, now with a corrected
target. No engineering prerequisite exists for option 1 (the event stream
already carries the timestamps).

## The TTFR harness (2026-10-28, PARITY-FIX-5 — option 1 implemented)

The probe harness landed (the measurement = the lane's next dispatch):

- `rrecov scan` now consumes the typed event stream (GAP-S6-10) for the
  TTFR datum: `TtfrProbe` + `ttfr_event_fn` log ONE line —
  `ttfr_ms=<run-elapsed> first_files=<n> stage=<s>` — at the FIRST
  FilesFound event with a positive count (the event's `timestamp_ms` is
  run-elapsed, pause-compensated; counters coalesce at stage boundaries,
  so the first positive boundary is the earliest observable first result —
  the honest granularity the event stream carries). Set BEFORE the
  ScanController construction (the controller copies the config); the
  once-flag is atomic (events fire from worker threads). Parse-gated
  (win_syntax_check); no new CLI flag — the existing `--log` carries it.
- The extraction lives in the CI-rebuilt lane's harness
  (`.github/scripts/Run-Benchmark.ps1` — the owner's 3-lane consolidation
  replaced staged-scenarios.yml mid-landing; the harness already ran every
  scan with `--log stage-<mode>.log`, so the probe line lands there without
  a flag change): the per-mode scan block extracts `ttfr_ms` /
  `ttfr_first_files` into the clean-benchmark performance JSON
  (`$cleanPerf.scans[]`) and prints the TTFR line per mode.

**Remaining:** the lane's next green dispatch transcribes the TTFR number
into this file (execution-bound — daily cron 05:30 or a manual dispatch);
the og-side TTFR comparison then closes the acceptance's second half with
the same evidence strength as the transport numbers above.
