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
