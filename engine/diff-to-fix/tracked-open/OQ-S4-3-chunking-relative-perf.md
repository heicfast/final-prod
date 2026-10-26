# OQ-S4-3 — Relative performance of the core's 64 KiB direct-I/O envelope vs the og's 4/7 MiB buffered-synchronous chunking is unmeasured

**Status:** OPEN (session 4) · **Module:** transport · **Parameters:** P47 (throughput), P51 (I/O behavior)
**Audited commit:** `63a9167` · corpus `03ad46c`

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
