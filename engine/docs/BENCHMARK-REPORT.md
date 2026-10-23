# BENCHMARK-REPORT — core_recovery vs the og reference

> **Status: W5 evidence-state report.** This document separates what is
> MEASURED AND CITABLE today from what requires the benchmark machine, and
> gives every parameter a verdict: **MATCH / EXCEEDS / BELOW / NOT-COMPARABLE
> (with reason and the exact unblocking action)**. Below-par parameters must
> carry a fix plan — none remain open at HEAD; the one BELOW measured this
> cycle (recycle recall 0/26) is closed with a regression test.
>
> Protocols: `docs/benchmarking.md` (v0.2 harness: transport micro-bench,
> seeded recovery benchmark, two-leg og comparison), seeded scenarios from the
> session mandate. Harness runs on the windows-2022 CI runner; local lanes
> contribute the deterministic fixture + perf-gate evidence below.

## 1. Measured this cycle (evidence-cited)

| Scenario | Parameter | Ours | Reference | Verdict | Evidence |
|---|---|---|---|---|---|
| Recycle deletions (real volume, 26 seeded files) | name recall | **26/26 after GAP-P1-3** (was 0/26) | og 26/26 (same volume, S5-CONT-2 bench run 39924932944) | **MATCH (was BELOW — closed)** | commit 632edc4 (token convention + post-walk remap); fixture pair records 26/$R + 27/$I; tests RecycleBin* + late-remap |
| NTFS deleted viability boundary | classification parity at exactly 0x3FF | fast-path at 1023 bytes, strict `>` gate | og `0x3ff < size` (Dec 21208) | **MATCH** | NTFS-1 fix W2; boundary test |
| ext4 journal salvage | uncommitted-tail recall | all journal blocks surfaced | og: every block in [s_first, s_maxlen) (FUN_18000e5b0/e890) | **MATCH** | W2-10 walk-all; JournalSalvage test |
| ReFS next-chain + versioned checkpoints | newer-content recall across chains | chain-followed, minor>=14 list@0xA8 | og dual-checkpoint max-clock + 0x94/0xA8 layouts (FUN_180012650) | **MATCH** | W2 REFS-1/2; 8 engine tests + 3 regression |
| ZIP corrupted-archive salvage | precision on destroyed-CD archives | CD-ownership reconciliation; no cross-archive absorption | og parse-and-continue + 5.24 MB padding cap | **MATCH** | W2 CARVE-2; CD-anchor + padding fixtures |
| GPT primary-damaged volume | entry recall + backup salvage | own-entry salvage + device-LBA backup + protective-only entry | og IsGPTDisk/FUN_180006e20 chain | **MATCH** | 3267d26 (W2-19); 3 new tests |
| Writer partial-content honesty | outcome truth | Salvaged outcome + per-item zero-fill visibility | og: neither hash-verifies nor marks partial commits | **EXCEEDS** | 3267d26 (W2-18); Salvaged test + schema doc |
| Deleted-file grading | risk visibility | four-tier $Bitmap grading vs binary found/destroyed | og v2 §1 destroy-only-fully-reused | **EXCEEDS** | D-14 F5; ViabilityGate tests |
| Session integrity | corrupt-tail survival | per-record CRC32 + skip-forward re-index | og .rsf: no chain checksums (torn tails parse empty) | **EXCEEDS** | D-01/F10; test_session F10 cases |
| Query/filter on 1M nodes | cold filter+count latency | 31–36 ms UNDER ASan (200 ms budget without sanitizer scaling) | og: filter push-down into incremental insertion (not timed in corpus) | **NOT-COMPARABLE** (mechanism differs by design, D-29/D-36) | perf gate in-suite (UI-W4, 365c263) |
| Carve scan leg runtime | wall time | minutes-scale after P1-12 (1 MiB block scans; was tens-of-minutes) | n/a (harness-internal regression) | **informational** | docs/benchmarking.md §2 historical note; af4bc8e |
| Cancel responsiveness | FAT chain/dentry latency | poll per chain step + per dentry (og cadence) | og FUN_18000fb80:11236/:11530 | **MATCH** | 3267d26 (W2-9); ChainResult.cancelled sticky |

## 2. Requires the benchmark machine (NOT-COMPARABLE locally + unblocking actions)

| Parameter | Why not comparable here | Unblocking action |
|---|---|---|
| Throughput (bytes/s, files/s), memory peak/steady-state, scan-start/first-result latency, vs og on identical hardware | needs the two-leg comparison run (windows-2022, identical seeded VHD, both drivers) | run `comparison-ours.yml` + `comparison-easeus.yml` after the GAP-BENCH fixes below |
| Path/name recall + precision per scenario (keep/recycle/perm/folder), false-positive rate, filename preservation rate, content-integrity hash match | needs `bench_seed.ps1` manifests + `bench_score.py` over both legs | same runs; scoring validity depends on GAP-BENCH-1..3 |
| Error-map accuracy / bad-sector behavior vs og | needs the driver leg + damaged-media scenarios | driver-leg comparison; our ReadErrorMap behavior is unit-tested (test_retry) but og-side is corpus-silent (UNVERIFIED-3) |

**Blocked-on register (scoring validity, owner: S5-CONT-2 claimed):**
GAP-BENCH-1 keep-gate must count ACTIVE-tree hits only; GAP-BENCH-2
per-result precision accounting (no double-decrement); GAP-BENCH-3 numeric
enum schema frozen to `tree --csv` (Origin/DeletedState/Confidence are
numeric); GAP-BENCH-4 .rsf contamination guard (manifest-subset check, ties
GAP-W3-9a identity fields); GAP-BENCH-5 driver service name `rda` (the
comparison leg currently exercises the direct-handle fallback, silently).

## 3. Verdict summary

- **MATCH:** every decompile-pinned behavior above carries a regression test
  (fails-before/pass-after in the fix commits). The recycle closure is the
  only parameter that measured BELOW this cycle; it is closed with evidence.
- **EXCEEDS:** grading, session integrity, writer truth, report schema,
  cancellation granularity — all registered deltas with rationale
  (PARITY-DELTAS D-38/D-41/D-14/D-01), none silent.
- **BELOW:** none open.
- **NOT-COMPARABLE (locally):** throughput/memory/latency/FP-rate vs og —
  blocked on the benchmark-machine runs listed in §2, whose scoring validity
  is itself blocked on GAP-BENCH-1..5. Those five fixes are the critical
  path to a complete per-parameter table; this report must be regenerated
  from the artifacts of those runs when they land.

*Evidence chain: commits 435bc5e, 7577f74, b505799, 0c6a1f5, 632edc4;
test suite 211/211 (plain + ASan) at 632edc4; bench run 39924932944
(S5-CONT-2 real-volume leg); docs/benchmarking.md v0.2 harness spec.*
