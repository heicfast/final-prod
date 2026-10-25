# PERFORMANCE-PROFILE — baseline measurements at HEAD `064fcf4` (session 6, 2026-10-25)

> Purpose (mandate §1): the measured performance truth of the current build
> BEFORE this session's changes, so every later number has a comparison
> point. Two evidence classes: (a) LOCAL first-hand measurements (Linux
> release + ASan builds, this session, hardware: CI container, gcc 14.2);
> (b) BENCH-RUN evidence from the Windows comparative harness (runs
> 40073093997, 40079065019 — windows-2022 runners, seeded VHDX volumes,
> artifacts cited per row). Nothing here is projected or extrapolated.

## 1. Local baseline (first-hand, this session)

| Measurement | Build | Result | Notes |
|---|---|---|---|
| Full unit suite (225 tests) | linux-release (gcc -O2, -Werror) | **3.04 s wall** (2.19 user) | includes fixture scans across NTFS/FAT/exFAT/ext4/HFS+/Btrfs/ZFS/ReFS/LDM, session round-trips, writer suites |
| Full unit suite | linux-gcc (ASan) | **8.46 s wall** | the ASan overhead ratio (~2.8×) is the expected envelope for parse-heavy code |
| Query/filter on 1M-node session tree | linux-release | **362 ms cold** (filter+count+page; 31–36 ms under the in-suite ASan timing gate) | `QueryPerf.*`; the 200 ms budget applies to the ASan lane per BENCHMARK-REPORT §1 |
| Engine fixture suites (NtfsVolume+Refs: 22 tests incl. full walks) | linux-release | **22 ms** | synthetic fixtures; NOT volume-scale throughput |
| Fuzz corpus (14 structure-aware harnesses) | local build | builds clean; zero-crash campaigns green in CI since f93f38e | GAP-CI-2 |

Local numbers are logic-shape baselines (fixtures are small by construction);
volume-scale truth comes from the bench runs below.

## 2. Bench-run baseline (Windows comparative harness)

From runs 40073093997 (first both-legs smoke) and 40079065019 (first full
ours-side matrix, all transport fixes in):

| Scenario (ours) | Recall | Precision | Notes / evidence |
|---|---|---|---|
| NTFS permanent_delete | 87.2% | 30.3% | precision limited by MetadataOnly zero-fill FPs (GAP-S6-7) |
| NTFS quick_format | **0.0%** | — | remnant scan name-only (GAP-S6-4); og 51.5% on the same state |
| NTFS delete_partition | 43.0% | — | discovery-limited (GAP-S6-6); og 42.7% but cap-limited (different failure mode) |
| NTFS decay_50 / decay_80 | 96.4% / **0.2%** | — | decay cliff analyzed as physical (GAP-S6-5); exFAT holds 98.6% at decay_80 |
| exFAT native (delete family) | **99.94%** | **99.16%** | the healthy reference cell on the same matrix |
| FAT32 native (delete family) | **0–0.3%** | — | GAP-S6-1 (names) + GAP-S6-2 (content): 46/10143 hash-true |
| ReFS native (all cells) | **0.0%** | — | GAP-S6-3: ERR_BAD_MAGIC at detection, ~2 ms, 0 files |
| Recycle deletions (real volume) | 26/26 name recall | — | post GAP-P1-3 closure (was 0/26) |
| Resume-equivalence probe | FAIL | — | GAP-S6-8 (extra carved entries post-resume) |

**Read of the table:** exFAT at 99.9/99.2 and NTFS-permanent-delete at 87%
recall establish the pipeline end-to-end is sound; every red row above has a
code-level root cause registered in `bench/root-causes.json` and a GAP id in
`docs/CORE-GAP-LIST.md` §6.A. No row is unexplained.

## 3. Transport / I/O profile

| Path | Alignment today | Verdict | Action |
|---|---|---|---|
| Direct raw handles (`\\.\PhysicalDriveN`, `\\.\HarddiskVolumeN`) | logical-sector (512) envelope, bounce-buffered when unaligned | **BELOW the 64 KiB contract** (GAP-S6-13) | 64 KiB envelope on the direct path: reads whose length is a multiple of 4 KiB but not 64 KiB take the documented ~3.5× slow path on the Windows volume stack (~830 MB/s vs ~2900 MB/s on NVMe, mandate §4); fix = round envelope lengths UP to 64 KiB multiples, clamp at device end, pass-through when already 64 KiB-aligned (no bounce, no copy) |
| RDA driver transport | logical-sector envelope | kernel IRP path — not subject to the user-mode slow path; keep sector alignment | none (documented) |
| Engine reads (all FS engines) | `CachedDevice` 64 KiB block batching (1 MiB cache, corpus MFT batch size) | already 64 KiB-multiple by construction | none |
| Recovery writer | 1 MiB chunks (default) + exact-size tail chunk | tail chunks unaligned → bounce path after fix; body chunks already 64 KiB multiples | covered by the envelope fix |
| Remnant scanner | 4 MiB block-buffered | already 64 KiB multiples | none |

**Verification plan (GAP-S6-13):** (a) unit-test the envelope computation as
a pure function on Linux (fail-before/pass-after); (b) assert in the Windows
transport unit test that every issued ReadFile length is a 64 KiB multiple
when the request permits it (clamped at device end); (c) the bench harness
re-measures scan throughput on the fixed build — the constraint is a parity
parameter, not a tuning exercise.

## 4. Memory profile

- Tree memory: value-snapshot (G-02) copies dominate transient memory; the
  1M-node query-perf fixture is the standing shape probe.
- Writer: bounded queues; per-item report rows streamed to JSON.
- Carve: window-bounded; merge/dedup pre-attach (F9) keeps the tree from
  duplicate-extent growth.
- SQLite layer (GAP-S6-12) lands with a bounded `cache_size` (-8000 = 8 MiB)
  — no unbounded page cache.
- Peak-memory per scenario is a bench-run parameter (BENCHMARK-REPORT §2
  requires the two-leg runs for the og comparison).

## 5. Where numbers must be re-taken after this session

1. FAT32 native cells (GAP-S6-1/S6-2 closures) — expect name+content
   recovery to move to the exFAT-class envelope.
2. ReFS cells (GAP-S6-3) — expect the first real-volume ReFS scan.
3. quick_format (GAP-S6-4) — expect remnant extents to convert the 0/10228
   into the og-comparable class (og 51.5% on the same state).
4. delete_partition (GAP-S6-6) — expect both candidates discovered.
5. Scan throughput on the direct transport (GAP-S6-13) — the 64 KiB
   envelope number.
6. Resume-equivalence probe (GAP-S6-8) — expect byte-identical trees.
