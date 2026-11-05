# PERFORMANCE — the driver baseline and the direct-access comparison

> The measurement ledger for the driver→direct replacement session (session-96).
> Rule (mandate §3): **no performance claim survives without a captured measurement.**
> Every row carries its GitHub Actions run ID, the runner class, and the disk profile.
> Runner class for all in-flight and completed runs: `windows-2022` (Windows Server
> 2022 Datacenter 10.0.20348, AMD EPYC 9V74 2C/4T @ 2596 MHz, hosted, elevated by
> construction — the same class every historical number below was captured on).

---

## §0 The measurement method

The product's own bench verb (`rrecov bench`) is the primary vehicle — it measures the
exact transport the product uses, at the exact read sizes the product issues:

- `bench info` — geometry probe (bytes, logical/physical sector, 4Kn flag)
- `bench seq` — sequential read throughput at a block size (default 1 MiB, 256 MiB span)
- `bench rand` — random-read latency/IOPS at a block size (default 1 MiB × 1000)
- `bench open` — transport open/close latency (1000 iterations)

The scenario matrix (test-benchmark.yml) adds the end-to-end walls: per-mode scan walls
(quick/deep/deepcarve) on a clean volume, TTFR, recovery throughput, and the API-lane
clean-benchmark (open/scan/save/query/recovery timings through cr_ui.dll).

Post-replacement, the SAME verbs on the SAME runner class with the SAME disk profile
are the comparison — nothing else is admissible as "meets or beats".

---

## §1 BASELINE — the driver-backed measurements

### 1.1 The fresh pre-replacement baseline (the session's reference point)

**Run 41042267586** (test-benchmark smoke, dispatched on the session-start tree
`619acec2` = release v0.6.1, reason `release-v0.6.1`). Target: disk 4,
8 GiB VHDX (8,589,934,592 bytes; 512 logical / 4096 physical = 512e; not 4Kn), volume
8,572,108,800 bytes, NTFS clean volume, 1209 live files + 2138 carved.

> **Correction (session-97, T3 — Agent D's first-hand artifact re-read):** this run
> was recorded above as "completed green"; the artifact shows it hit its 120-min
> job budget mid-scenario — 4/7 scenarios captured, no 06-comparison/gates output,
> teardown at exactly start+120:00. The **07-performance clean-volume section
> completed before the budget hit and is the valid baseline of record** (every §1.1
> number below comes from it); the scenario-lane walls from this run are partial.
> The "green" was the job-level conclusion, not the matrix's.

| Metric | Value | Source |
|---|---|---|
| bench info (rda) | 8,589,934,592 B; 512/4096; 4Kn=false | clean-volume-benchmark.json |
| bench seq 1 MiB × 256 MiB (rda) | **536.92 MiB/s** | clean-volume-benchmark.json |
| bench rand 1 MiB × 1000 (rda) | **1993.22 µs mean; 502 IOPS; 1000/1000 hits** | clean-volume-benchmark.json |
| bench open × 1000 (rda) | **17,019 ns mean; 58,757 ops/s** | clean-volume-benchmark.json |
| scan quick (clean vol) | 150 ms wall | clean-volume-benchmark.json |
| scan deep (clean vol) | 16,505 ms wall; 1209 files; TTFR 0 ms | clean-volume-benchmark.json |
| scan deepcarve (clean vol) | 1,501,622 ms wall; 8,572,108,800 B scanned | clean-volume-benchmark.json |
| API clean-benchmark (rda, threads 4, mask 7) | open 0.9 ms; scan 1,513,221.7 ms; save 10.8 ms; query 0.2 ms; recovery 49.2 ms (82 files); preview 4096/4096 | api-result.json |

**Run 41082469649** (test-benchmark FULL profile — dispatched at the Phase-0
tree `b9430745`): ran ~3 h against the v0.6.1 release with the full driver
machinery, then was cancelled externally at 18:38 UTC before publishing its
artifact — no full-profile numbers were recoverable.

**Why no full-profile driver-backed baseline exists (the honest record):**
the removal landed before a full-profile baseline could complete, and the
test-benchmark lane no longer installs a kernel driver — by design, and
permanently. A re-dispatch against the v0.6.1 release under the new lane
was attempted (run 41111403030) and correctly FAILED: the driver-era
release refuses every raw-disk verb with its exit-4 driver-absent refusal
when no driver is installed — which is itself evidence the removal is
complete and the old refusal worked exactly as designed. The reference
baseline for every comparison is therefore §1.1's smoke-profile run
(41042267586, captured pre-change with the driver installed and verified)
plus the historical run-ID'd numbers in §1.2. The post-replacement
measurement (§2) runs the SAME smoke+full scenario matrix on the SAME
runner class against the new release — the apples-to-apples comparison the
mandate requires.

### 1.1.1 Full-profile baseline

Not capturable post-removal (see above); §1.1 + §1.2 are the baseline of
record. The post-replacement full-profile run in §2 establishes the new
full-profile reference going forward.

### 1.2 The historical driver-era baselines (already captured, run-ID'd)

**Run 39807929475** (driver-repo bench lane, windows-2022, 4 GiB seeded VHDX; the
driver vs the original eprdtdrv reference driver — the driver's own parity table):

| Metric | RDA driver | og driver (eprdtdrv) | Delta |
|---|---|---|---|
| seq 64 KiB | 532.9 MiB/s | 533.1 MiB/s (391.0 in a later rerun) | parity |
| seq 1 MiB | 458.6 MiB/s (R1) | 500.6 MiB/s | −8.4% |
| rand 4 KiB | 7,932 IOPS | 8,170 IOPS | −2.9% |
| rand 4 KiB latency | 126.1 µs | 122.4 µs | +3.0% |
| open latency | 8,217 ns | 8,042 ns | +2.2% |
| handle churn | 8,532/s | 8,537/s | parity |
| IOCTL fuzz | 841,698/s | 553,752/s | +52% |
| 64/64 1 MiB SHA-256 | byte-identical | byte-identical | — |
| hostile IOCTLs | 50,000 survived | — | — |

**Run 40227375329** (alignbench sweep, windows-2022): the read-size sweep plateau
~240 MiB/s flat (128/132 KiB: 240.0 vs 233.6 = 1.03×); RDA legs 201.9/169.3/160.5;
**direct legs 151.5 MiB/s @ 256 KiB — the runner's direct-vs-driver gap (−10.5% vs
169.3)**. This is the one historical datapoint where a direct path measured SLOWER
than the driver path on the same runner. Investigation duty: the direct legs in that
run predate the core's current envelope implementation; the replacement's acceptance
is measured against §1.1's fresh baseline, and this gap is explicitly re-measured
(see §3).

**Run 40315255368** (alignbench, the OQ-S4-3 transport verdict): og 4/7 MiB chunking
197.32/196.15 MiB/s vs core 64 KiB envelope 203.98 MiB/s and 256 KiB 241.64 MiB/s
(+18–19% at lower CPU; the core's RDA legs: 194.34/237.54).

**Run 40073093997** (+40079065019, the scenario matrix): scan walls quick 0.51–0.53 s,
deep 2.01–8.03 s, deepcarve 41.98–90.06 s per 3 GiB volume; recall/precision matrix
(exFAT 99.94/99.16); og recycle 26/26 (39924932944); M-117 220 MB TS >600 s → 11.3 s.

**API latency (run 40228880804)**: 10.7–166.3 ns per call across the cr_ui surface.

### 1.3 What the baseline means for the replacement

The driver's data path is a synchronous forward-IRP with zero caching, zero queueing,
zero retry (DRIVER-CONTRACT §3): its throughput IS the storage stack's throughput at
the issued read sizes. The direct path removes one kernel hop (the grammar-path
re-issue through the driver's dispatch) and the F11 Information scaling — the physics
say direct ≥ driver at every read size, provided:

1. the envelope (offset-down/length-up + bounce) is preserved — the ~3.5× unaligned
   slow path (run 40073093997, GAP-S6-13) is the failure mode to avoid;
2. reads stay synchronous QD1 (the consumers are QD1 by design — CONSUMERS §3);
3. no extra copy enters the path (the F11 unscale disappears; the bounce buffer only
   on unaligned requests, same as today).

The runner's direct-vs-driver gap in run 40227375329 is the risk to retire: it must
be re-measured with the replacement and, if it persists on the runner's VHDX, its
cause documented (VHDX on the same physical disk as the OS can penalize either path
differently — the measurement, not a claim, decides).

---

## §2 REPLACEMENT — the direct-access measurements

**v0.7.0 (the direct-access release), run 41140212296** — windows-2022, same
8 GiB 512e VHDX profile, same verbs, same disk-4 geometry. Provenance note: the
RUN is red (the scenario lane's gates — see §2.2), but the 07-performance
clean-volume phase completed before the failures and is a valid capture;
every number below is from its artifact. Single-sample (the 3-run median
discipline applies before any verdict is final — PERFORMANCE-TARGET §6).

### 2.1 The clean-volume table (headline rows)

| Metric | Baseline (§1.1) | Direct result | Delta | Verdict (single-sample) |
|---|---|---|---|---|
| bench seq 1 MiB | 536.92 MiB/s | **544.17 MiB/s** | +1.4% | EXCEEDS |
| bench rand 1 MiB | 1993.22 µs / 502 IOPS | **1976.72 µs / 506 IOPS** | −0.8% | MEETS/+ |
| bench open | 17,019 ns | **9,213 ns** | −45.8% | EXCEEDS |
| scan quick wall | 150 ms | **27 ms** | −82.0% | EXCEEDS |
| scan deep wall | 16,505 ms | **16,467 ms** | −0.2% | MEETS |
| scan deepcarve wall (8 GiB) | 1,501,622 ms | **1,014,235 ms** | −32.4% | EXCEEDS |
| API scan (8 GiB, mask 7, 4 threads) | 1,513,221.7 ms | **1,113,445.2 ms** | −26.4% | EXCEEDS |
| API open | 0.9 ms | **0.5 ms** | −44% | EXCEEDS |
| API recovery (82 files) | 49.2 ms | **36.8 ms** | −25.2% | EXCEEDS |
| session save | 10.8 ms | **9.3 ms** | −13.9% | MEETS/+ |
| files found | 1209 live + 2138 carved | 1209 live + 2134 carved | shape-identical | MEETS (floor) |
| API query | 0.2 ms | 0.1 ms | −50% | MEETS/+ |
| CPU per GiB (scenario deepcarve samples, 2.990 GiB vol) | 501.2 CPU-s/GiB | **303.8 CPU-s/GiB** | −39.4% | EXCEEDS (verified from both artifacts' samples-deepcarve.csv) |
| working-set peak (same samples) | 52.0 MiB | 53.7 MiB | +3.3% | MEETS |

### 2.2 The scenario-lane state (why the run is red, and what is NOT a transport regression)

1. **Recall-0 on every floor-gated scenario — PRE-EXISTING, both eras.** The
   driver-backed baseline run 41042267586's S-DEL-NTFS verdict shows the same
   hash_matched=0 with 0-byte placeholder inventories (92 files, 1 name-only
   match vs v0.7.0's 94/3). The recovery inventory in both eras contains only
   directory-skeleton entries. The old lane had no gate table so this never
   redened a job; session-96's gate discipline now enforces it. Disposition:
   diagnose (harness matcher vs product recovery path) — NOT a direct-path
   regression; parity holds by measurement.
2. **Stale transport expectation (harness defect, session-96 miss):** the
   API-lane gate asserts `transport == 'rda'` (Run-Benchmark.ps1:628-630);
   the transport is now `direct` — every API scenario carries a spurious
   failure reason. Fix is one line.
3. **S-PART-DELETE (new scenario, undiagnosed):** whole-disk deepcarve
   (`rrecov scan --disk 3 --deep`) on the partition-deleted disk exits within
   ~0.5 s with no session and was not capped. Top hypothesis (ACCESS-LAYER-
   CURRENT §7.4-H1): `open_disk(3)` fails in the PnP/volume-stack
   reconfiguration window ~30 ms after Remove-Partition (the disk WAS
   raw-openable 35 ms earlier for the hash map). The exception path archived
   no stdout/stderr/exit code (evidence gap, Run-Benchmark.ps1:688-717) —
   the discriminating fix lands first, then the re-dispatch decides.
4. **The lane-level wall-clock record:** the direct run completed all 7
   scenarios + phase 4 in 1 h 42 m; the driver-era baseline could not finish
   4 scenarios in its 120-min budget.

### 2.3 Pending rows (require the T5/T6 harness work)

| Metric | Baseline | Direct result | Run ID | Verdict |
|---|---|---|---|---|
| full-profile walls | §1.1.1 (not capturable, driver era) | pending the full-profile dispatch | — | — |
| rand 4 KiB / seq 64K·256K·4M legs | not in either lane | pending the bench sweep extension | — | — |
| cancel time-to-stop | not measured | pending the api_harness probe | — | — |
| preview/thumbnail/image/health TTFB | not measured | pending the probe timing | — | — |
| S-DEL-NTFS scenario deep wall | 7.56 s (partial-era) | 8.10 s | 41140212296 | GAP (single-sample; 3-run median before scoping) |
| FS-engine walls (ext4/Btrfs/ReFS/HFS+/ZFS), LDM, BitLocker | no fixtures | pending T6 fixtures | — | — |

---

## §3 The comparison verdicts

Interim (single-sample, §2.1): the direct path MEETS or EXCEEDS the driver
baseline on every clean-volume metric measured — seq +1.4%, open −46%, quick
−82%, deep parity, deepcarve −32%, API scan −26%, recovery −25%, CPU/GiB −39%.
The historical direct-vs-driver risk datapoint (run 40227375329: direct legs
151.5 vs driver 169.3 MiB/s @256 KiB, −10.5%) predates the current envelope
implementation; it is explicitly re-measured by the T6 bench size sweep on the
current path. Final verdicts issue per PERFORMANCE-TARGET §6 (3-run medians,
run IDs) as the T5/T6 harness work completes; every GAP is diagnosed and either
fixed or documented (mandate §3: no access pattern regresses silently).
