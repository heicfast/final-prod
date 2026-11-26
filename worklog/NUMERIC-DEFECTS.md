# NUMERIC-DEFECTS.md — Audit Agent E (numeric/logic audit), small-bug session

Scope: every number the product displays and the math behind it — engine
(`engine/src`, C++20), Tauri bridge (`ui/src-tauri/src`, Rust), UI
(`ui/src`, TS/React). RESEARCH ONLY; no files modified except this one.

## Method

1. Enumerated every numeric-producing site by class: progress permille math,
   sector/LBA conversions, ETA/throughput, size formatters (both sides),
   confidence/health scores, session/job counters, pagination, and any
   hard-coded constants posing as measurements.
2. For each site: read the formula verbatim, re-derived it by hand, and
   recomputed a concrete input→output example (Python scratch recomputation,
   transcript below in each finding) before classifying it a defect.
3. Traced full data paths before claiming user-visible impact: engine
   `tel_publish_`/event → ABI `cr_ui_event` → bridge `events.rs` →
   `compose_scan_progress` payload → `queries/scan.ts` store →
   `scan-progress-page.tsx` gauge (gauge = `phaseProgress ?? overall`,
   both fed by engine permille /1000).
4. Div-by-zero, integer truncation, u64 overflow, off-by-one, clamping,
   unit (ms/s, bytes/sectors, 0-1 vs 0-100) checks applied at every site.

Scratch recomputations (Python) used for the worked examples:
`weighted_progress(40/35/25, 1000,1000,0) = 750‰`; `done=total → pm=1000`;
honest 64/32000 dentries = 2‰; recovery counters 10+2+1(+1) sum = 13 of 10;
4Kn size ratio 8.0×; HFS+ `300·1000/1024 = 292‰`; `eta_compute(250,1000,60000ms) = 180 s`;
imaging range bytes `n·512` vs full; `avg×3` vs default `2.4 MB×3`.

## Findings (defects, worked examples)

### SB-E-01 — FAT quick pass hits 1000‰ after the FIRST directory (premature 100%)
- File: `engine/src/core/fat/cr_fat_quick.cpp:294-295` (mid-loop variant at `:162`)
- Formula (verbatim, per-directory close tick):
  `if (ctx.progress && count) (void)ctx.progress(ctx.dentries_done, ctx.dentries_done, ctx.progress_user);`
- Mechanism: `ScanController::tel_publish_` (`cr_scan.cpp:528-536`) does
  `if (done >= total) pm = 1000; … if (pm < st.permille) pm = st.permille;`
  → after the first directory closes, quick-stage permille = 1000, and the
  monotonic clamp holds it there. With `covers_both` (quick+deep share the
  structured pass, `cr_scan.cpp:806-810`), stage 1 (deep) telemetry is set to
  1000‰ in the same tick.
- Worked example: FAT32 volume, root = 64 dentries, 500 subdirs (~32,000
  dentries). First directory closes → `progress(64, 64)` → quick.permille =
  deep.permille = 1000‰. `weighted_progress(40,35,25; 1000,1000,0) = 750‰`
  (recomputed) → **overall jumps to 75% within seconds**; `phase_progress`
  (quick phase active) = quick permille → **UI gauge shows "Quick Scanning
  100%"** while the remaining ~31,936 dentries (minutes of chain reads) run.
  Mathematically correct value at that moment: 64/32000 = **2‰ (0.2%)**.
- Root cause: terminal-style `(done, done)` tick emitted at per-directory
  scope instead of scan scope, combined with the monotonic clamp (wrong
  formula placement / missing guard).
- Impact: progress bar stuck at 100% (phase gauge) / 75% (overall) for the
  whole scan; ETA meaningless; user believes the scan is done.
- Severity: **high**. Confidence: **high** (code-path reasoning end-to-end;
  runtime confirmation listed below).

### SB-E-02 — exFAT quick pass: same premature 1000‰ per directory
- File: `engine/src/core/exfat/cr_exfat_quick.cpp:1002-1003` (mid-loop `:490`)
- Formula: `if (ctx.progress && count) (void)ctx.progress(ctx.dentries_done, ctx.dentries_done, …)`
  and mid-loop `progress(ctx.dentries_done, ctx.dentries_done + (count - i))`
  which reaches `done == total` when `i == count` (last slot) or when the
  tick lands on the final dentry.
- Same telemetry consequence as SB-E-01 (permille=1000 at first directory
  close, monotonic hold, quick+deep both spiked).
- Worked example: exFAT volume, first directory of 40 entries →
  `progress(40,40)` → 1000‰; correct value ≈ 40/N×1000‰ of the walk.
- Root cause / impact / severity / confidence: same as SB-E-01 (**high**).

### SB-E-03 — ext4: no ticks during the live walk, then terminal tick BEFORE the deleted-inode sweep
- File: `engine/src/core/ext4/cr_ext4_engine.cpp:1254-1255`
- Formula: `if (progress) (void)progress(v.sb.groups, v.sb.groups, progress_user);`
  — emitted after the live-tree `walk_dir` (line 1250) but **before** the
  deleted-inode sweep (lines 1272-1301, which itself reports
  `progress(ino, sweep)` at `:1300-1301`).
- Worked example: 1M-inode volume; live walk 90 s with **zero** progress
  ticks (walk_dir has none) → gauge shows 0%; then `(groups, groups)` →
  1000‰; the sweep then reports e.g. `progress(500000, 1000000)` = 500‰ but
  is **clamped up to 1000‰** by the monotonic rule for its full 60 s.
  Correct: ramp 0→~90% during the live walk, then continue 90→100% through
  the sweep.
- Root cause: terminal tick at wrong scope (mid-pass), plus a tick-less
  live-walk phase (coverage gap, not a fake number).
- Impact: 0%-stuck then 100%-stuck phases on every ext4 quick+deep scan.
- Severity: **medium-high**. Confidence: high.

### SB-E-04 — btrfs: `progress(4, 4)` fires per SUBVOLUME, not at scan end
- File: `engine/src/core/btrfs/cr_btrfs_engine.cpp:1184` (final at `:1188` is fine)
- Formula: inside the subvol loop — `(void)progress(4, 4, progress_user); /* final tick per subvol */`
- Worked example: 12 subvolumes; after subvolume #1 completes →
  `progress(4,4)` → 1000‰; the remaining 11 subvolume walks (the bulk of
  the scan) render at 100% (clamped). Correct value after subvol 1: ≈
  1/12 → 83‰ (or the phase-1-of-4 marker ≈ 250‰ by the file's own 1..4 scale).
- Root cause: loop-scoped terminal tick (copy-paste of the end-of-scan
  tick into the per-subvol loop; the comment itself says "final tick per
  subvol").
- Severity: **medium**. Confidence: high.

### SB-E-05 — HFS+: hard-coded 1024-node denominator for catalog-walk progress
- File: `engine/src/core/hfsplus/cr_hfsplus_engine.cpp:1397-1398`
- Formula: `if (progress && (chain_len & 15) == 0) (void)progress(chain_len, 1024, progress_user);`
- Worked examples: catalog leaf chain of 300 nodes → permille caps at
  `300·1000/1024 = 292‰` (recomputed) for the whole walk (display ≤29.2%);
  chain of 2000 nodes → `done > total` → clamped to **1000‰ at the 1025th
  node, mid-walk** (recomputed 2000·1000/1024 → clamped 1000).
- Root cause: phantom hard-coded denominator (the total is unknown; the
  constant is not a measurement of anything).
- Impact: HFS+ volumes show either an arbitrarily-capped gauge or a
  premature 100%.
- Severity: **medium**. Confidence: high.

### SB-E-06 — recovery job live counters double-count (completed can exceed total)
- File: `ui/src-tauri/src/bridge/events.rs:653-656`
- Formula (verbatim):
  `live.done_count = stats.done + stats.salvaged;`
  `live.failed_count = stats.errors + stats.hash_mismatch;`
- Engine semantics (verified in `engine/src/core/recovery/cr_safewriter.cpp`):
  `salvaged ⊂ done` (an item hits `++stats.done` at `:958` and `++stats.salvaged`
  at `:962` — a sub-bucket of committed items); a **hash_mismatch item still
  commits** and also counts in `done` (`:909-919` → `:957-958`); `resumed`
  items are Done-state but counted only in `resumed` (`:599-602`) — that one
  is disjoint and fine.
- Worked example (recomputed): queue = 10 items — 5 clean, 2 partial
  (salvaged), 1 hash-mismatch (committed), 1 resumed, 1 hard error.
  Engine: done=8, salvaged=2, hash_mismatch=1, errors=1, resumed=1.
  Bridge: done_count = **10**, failed_count = **2**, skipped = 1 →
  **13 outcomes for 10 items**; QueueProgress renders
  `completedCount/totalCount` = **"10/10" plus 2 failed and 1 skipped**.
  With all-committed items salvaged the pair can read "12/10"
  (progress > 100% class).
- Root cause: additive mapping of non-disjoint engine sub-counters (type/
  classification error, not a formula error per se).
- Correct value: done_count = stats.done (8, or 10 counting the mismatch
  row), failed = errors (1) with mismatches surfaced per-item (the report
  rows in `commands/recovery.rs` do classify disjointly — only the live
  counter path is wrong).
- Severity: **medium**. Confidence: high.

### SB-E-07 — recovery "size to recover" is a count×average estimate posing as a measured total, and it gates the space check
- File: `ui/src/features/recovery-queue/recovery-page.tsx:80-95`
- Formula (verbatim): `avgFileBytes = filesFound>0 ? dataFoundBytes/filesFound : 2_400_000;`
  `neededBytes = max(1, round((allCount | selectedCount) × avgFileBytes))`
- Worked examples (recomputed): scan with 100,000 files, dataFoundBytes
  49 GB → avg = 490,000 B. Selecting 3 files whose true total is 6 GB (three
  2 GB videos) shows **"3 files (1.5 MB)"** — 4,000× under. Selecting 3 files
  with filesFound=0 (state not loaded) shows **"7.2 MB"** (3 × 2,400,000 — a
  hard-coded phantom average).
- The same estimate is sent to `system_validate_destination` →
  `system.rs:230` `if needed_bytes > 0 && free < needed_bytes → "out-of-space"`:
  a low estimate passes the gate and the run hits the engine's truthful
  N-015 pre-check (`cr_safewriter.cpp:982-994`) only AFTER Start; a high
  estimate spuriously blocks with the "Insufficient Disk Space" modal.
- Root cause: estimate presented as a measurement (no "≈", no per-selection
  sum — the tree's exact per-node sizes are available via the snapshot but
  not summed here); hard-coded 2,400,000 default.
- Severity: **medium**. Confidence: high.

### SB-E-08 — scan ETA/throughput computed from a restartable byte counter against whole-run elapsed (mismatched windows)
- File: `ui/src-tauri/src/bridge/events.rs:540-554`
- Formula (verbatim): `let bps = tel.bytes_scanned as f64 / elapsed_s;`
  `live.last_eta_seconds = Some(((live.bytes_total - live.bytes_scanned) as f64 / live.last_throughput) as u64);`
- The engine deliberately restarts `bytes_scanned` per deep sub-pass
  (`cr_scan.cpp:554-565`, DIRECT assignment, documented "current sub-pass's
  position"), while `elapsed_ms` is the whole-run clock. Throughput =
  (current sub-pass bytes)/(whole-run seconds) — numerator and denominator
  cover different windows; ETA inherits it.
- Worked example: 500 GB volume; quick 120 s (bytes_scanned = 0 → no ETA);
  remnant sweep reaches ~500 GB at t≈1400 s (bps ≈ 357 MB/s); discovery
  sub-pass restarts bytes_scanned ≈ 0 with elapsed still 1400 s → bps ≈ 0 →
  EMA decays (0.6 weight) → `eta = (bytes_total − 0)/ε` → seconds in the
  1e7 range → `formatDuration` renders "2,777:46:40"-style strings; the
  "Remaining time" and throughput figures oscillate wildly mid-scan.
- Root cause: unit/window mismatch in a derived measurement (should be
  Δbytes/Δt between samples, as `spawn_job_emitter` correctly does at
  `events.rs:622-632` for recovery/image jobs).
- Severity: **medium** (display quality; no crash). Confidence: high on the
  mechanism, medium on exact magnitude (runtime listed).

### SB-E-09 — session list row: `dataFoundBytes` field carries `bytes_scanned`
- File: `ui/src-tauri/src/commands/sessions.rs:217`
- Formula: `"dataFoundBytes": (if valid { json!(r.bytes_scanned) } else { json!(0) })`
- The light row ABI (`abi.rs:447-464`) has NO found-bytes field — only
  `bytes_scanned` (coverage read). The bridge maps scanned bytes into the
  contract field named `dataFoundBytes`.
- Worked example: a full 500 GB scan that found 3 GB of files → any future
  consumer of `SessionMeta.dataFoundBytes` renders "500 GB found" instead
  of 3 GB (166× wrong).
- Currently latent (no UI consumer renders the sessions-row field — the
  backup page shows only `filesFound`), but the contract itself is wrong.
- Root cause: mislabeled field mapping (unit-of-measure error at the seam).
- Severity: **low** (latent). Confidence: high.

### SB-E-10 — sector-size `?? 512` fallbacks contradict the "never a hard-coded 512" rule
- Files: `ui/src/features/scan-progress/scan-progress-page.tsx:144`
  (`progress?.sectorSize ?? 512`), `ui/src/features/device-selection/create-image-dialog.tsx:131`
  (`device?.sectorSize ?? 512`)
- On a 4Kn drive, during the window before `scanState`/`progress` carry the
  real geometry, `sectorsScanned/sectorsTotal` display 8× too small
  (e.g. offset 512,000,000 B → 250,000 sectors shown instead of 125,000…
  i.e. 8× over-count of sectors). The bridge itself always sets the true
  `sector_size` (`ScanLive::new(geom.total_bytes, geom.logical_sector)`,
  `scan.rs:451`), so the fallback only fires pre-telemetry — but the same
  files' own comments pin "Never a hard-coded 512 — a 4Kn drive rendered
  eightfold-wrong sector numbers before" (P-DEEP-01).
- Root cause: default constant instead of a null/`—` render for unknown.
- Severity: **low**. Confidence: high.

### SB-E-11 — stop-confirm dialog's "Time" falls back to the ETA (remaining), not elapsed
- File: `ui/src/features/scan-progress/scan-progress-page.tsx:283-287`
- Formula: `time: usedTime ?? formatDuration(progress?.etaSeconds ?? 0, i18n.language)`
- `stopStats.time` is rendered under the "Time" (elapsed) label
  (`:414-419`). When `scanState.data?.startedAt` has not loaded yet, the
  dialog shows the REMAINING estimate as the elapsed time.
- Worked example: true elapsed 300 s, ETA 660 s, state not yet loaded →
  dialog's Time cell reads "11:00" instead of "05:00".
- Root cause: wrong fallback source (unit/semantics swap).
- Severity: **low**. Confidence: high.

### SB-E-12 — lost-partition candidate "Size" column hard-codes 512-byte sectors
- File: `ui/src/features/lost-partition/lost-partition-page.tsx:216`
- Formula: `{c.sectorCount !== null ? formatBytes(c.sectorCount * 512, i18n.language) : '—'}`
- The candidate's `sectorCount` is in the DISK's own sector units (engine
  geometry), but the size column multiplies by a constant 512. The page has
  the device row (with `sectorSize`) in scope and does not use it.
- Worked example (recomputed): 4Kn disk, candidate spanning 1,000,000
  sectors → true 4,096,000,000 B (3.81 GiB) → displayed
  512,000,000 B ("488.3 MB") — **8× under**.
- Root cause: hard-coded sector-size assumption (the exact class the
  P-DEEP-01 pin bans elsewhere).
- Severity: **medium** (display only; apply path uses engine geometry).
  Confidence: high.

### SB-E-13 — recoverability ring % counts a different population than the engine's verdict score
- Files: `ui/src/features/scan-progress/recoverability-card.tsx:24`
  (`const ratio = stats.matched / stats.total;`), engine
  `engine/src/core/tree/cr_grading.cpp:555,566-567,600-602`
- `out.matched`/`out.total` are computed over ALL files (every node with
  grade High / every file), while the engine's own verdict `score`
  (`:600-602`) uses the og's 20-format gauge population
  (`gauge_matched/gauge_total`). The UI ring/percent uses the former; the
  engine's 0..10 score (which the og's gauge used) is never surfaced.
- Worked example: a scan of 1,000 intact deleted text files (none in the
  20-format set): ring = matched 1000/total 1000 = **100%** "highly
  recoverable", while the engine's gauge score = 0/0 → **0**. Two
  contradictory "recoverability" numbers from the same tree.
- Root cause: population divergence between display fraction and grading
  score (both individually honest).
- Severity: **low-medium** (semantics). Confidence: medium-high.

### Observations (not defects, recorded for the ledger)
- `engine/src/core/query/cr_query.cpp:579-619` — `progress_display_remap` /
  `progress_display_percent` / `progress_display_publish`: the og's
  piecewise display-remap curve (raw 20 → 21.8 etc.) is ported verbatim but
  has **zero callers** in engine, bridge, and UI (verified by grep). It is
  the one place a non-linear "displayed ≠ actual" percent could have
  entered the product; it is dead code. If anyone wires it up, it violates
  the session's no-fake-progress rule — recommend deleting or gating.
- `display_size_ui` tier boundaries (`≥ 0x40000001` etc., `cr_query.cpp:709-749`):
  exactly 1 GiB renders as "1024.00 MB" (not "1.00 GB"). Faithful port of
  the og's off-by-one tier gates; corpus wins (documented in the file).
- `phase_progress` (`cr_scan.cpp:166-167`): a quick-only scan between
  quick-stage-end and terminal latch reports phase=1, permille=0 ("Advanced
  0%") for a scan that is actually finished. Transient (terminal payload
  fixes phaseProgress=1.0); cosmetic.
- `formatBytes`/`formatBytesCompact` cap at PB/T units: ≥1 EiB / ≥1 PiB
  render as "1536 PB"/"1024 T". Unreachable in product (u64 sizes, real
  disks ≤ 2^52).
- Mock connector (`ui/src/connector/mock/mock-connector.ts`): synthetic
  progress (`elapsedMs = tick*80`, `eta = (totalTicks−tick)·150/1000`,
  `matched = round(total·0.82)`, hard-coded 512 in lost-candidate
  `lba = floor(startByte/512)`). TEST DOUBLE only — never the shipped
  backend (mock is excluded from production connector selection); no fake
  progress exists in the production path (heartbeat explicitly carries
  standing truth only, `cr_ui.cpp:1132-1140`).

## Verified-correct numbers (check → result)

1. `weighted_progress` (`cr_scan.cpp:131-146`): single-division weighted
   mean, u64 numerator, clamps — `(40·500+35·500+25·500)/100 = 500‰` exactly
   (recomputed); no double truncation; div-by-zero guarded (`total==0 → 0`).
2. `phase_progress` (`cr_scan.cpp:148-173`): active-sub-stage re-normalization
   correct; disabled stages drop weight; clamps at 1000.
3. `tel_publish_` permille (`cr_scan.cpp:526-539`): `done·1000/total` in u64,
   `done>=total → 1000`, monotonic within stage — 123/1000 → 123‰ ✓.
4. Bridge permille→fraction: `events.rs:375-378`
   `(evt.overall_permille as f64)/1000.0`, monotonic clamp; terminal = 1.0 —
   no 0-100 vs 0-1 confusion anywhere on the path.
5. `currentSector`/`currentSectorTotal` (`events.rs:227-236`): integer
   byte→sector division with `.max(1)` zero guard ✓ (512,000,000/512 =
   1,000,000 ✓).
6. `eta_compute` (`cr_query.cpp:623-652`): gate `done==0||total<done||total==0`
   → no div-by-zero; `(total−done)·elapsed_ms/done/1000` recomputed:
   (250,1000,60000ms) → **180 s** = (750/250)·60 s ✓; EMA 0.2/0.8 weights
   correct; ms→s conversions right.
7. `eta_format_m_s` (`:654-662`): minutes/seconds split correct; negative →
   og's "3m00s" placeholder (faithful).
8. `scan-progress-page.usedTime` (`:128-134`): `(ref−parse(startedAt))/1000`
   — ms→s ✓, `Math.max(0, …)` guard, completion freezes at engine timestamp.
9. `formatBytes` (`ui/src/lib/format.ts:13-31`): binary 1024 steps with
   decimal labels (documented Windows/corpus convention); NaN/negative →
   '—'; 1536 → "1.5 KB"; 149.45·2³⁰ → "149.5 GB" ✓ (matches pinned test).
10. `formatDriveGb` (`:48-53`): `bytes/1024**3`, 2-decimals — 31.16 GB-class
    labels ✓.
11. `formatDuration` (`:110-119`): floor, mm:ss / hh:mm:ss, negative → '—'
    ✓ (65→"01:05", 3671→"01:01:11").
12. `formatPercent` (`:127-131`): clamps [0,1] before Intl percent — 1.4 →
    "100%", −0.2 → "0%" ✓ (all UI callers pass fractions; wearPercent is
    correctly divided by 100 at both health-page call sites).
13. `formatThroughput` (`:122-125`): ≤0 → '—' (no 0 B/s flash, no
    div-by-zero downstream).
14. `formatHexRows` (`ui/src/lib/hex-format.ts:18-33`): 16-byte rows,
    offset arithmetic `baseOffset + i` ✓, HEX_ROW_WIDTH = 16·3+1 = 49
    exactly matches 16 bytes + the double space after byte 8 ✓.
15. Carve progress (`engine/src/core/carve/cr_carve.cpp:1661-1705`): serial
    `progress(wi·WINDOW, ctx.total)` — `(window_count−1)·WINDOW < total`
    always ⇒ pm < 1000 until the true terminal `(total,total)`; parallel
    `progress(min(done·WINDOW, total), total)` reaches total only at the
    Nth completion ⇒ no premature 100% (re-derived).
16. NTFS MFT walk (`cr_mftwalk.cpp:505,1763`): `progress(record_no,
    max_record)` every 64 records — true fraction; terminal at end only ✓.
17. NTFS remnant sweep (`cr_mftwalk.cpp:2345-2363,2417-2418`): coverage
    atomic per 4 MiB block vs `progress_total`; skipped blocks counted as
    scanned (coverage semantics, documented) ✓.
18. ReFS walk (`cr_refs_engine.cpp:1099-1100`): `progress(emitted,
    emitted+0x10)` — self-scaling denominator, pm strictly < 1000 mid-walk
    (asymptote ≈ 99.9%), terminal `(emitted, emitted)` at true end ✓ (no
    premature spike; cosmetic asymptote noted in observations).
19. ZFS markers (`cr_zfs_engine.cpp:1416,1462,1766,1804`): 1..4-of-4 phase
    markers, terminal at end ✓.
20. Deep-stage boundary ticks (`cr_scan.cpp:1701,2543,2786`): post-stamp
    `tel_overall_()` forwarding — monotonic, never a hardcoded weighted
    guess (comment at 1697-1700 explains why) ✓.
21. SafeWriter byte progress (`cr_safewriter.cpp:1043-1045`):
    `done_bytes/total_bytes` with `total_bytes:1` guard; error items never
    count toward done_bytes (N-009b honesty) ✓.
22. Image creation byte range (`imaging.rs:312-329`): `first·ls < size_bytes`
    gate, `total_sectors = size/ls`, `n = count==0 ? ts−first :
    min(count, ts−first)`, `n·ls` — recomputed 1 TB-class disk with
    first=1000: `imaged_bytes = 1,000,204,374,016` vs full
    1,000,204,886,016 (exactly 512,000 sectors' worth less) ✓; saturating
    guards throughout.
23. Image/recovery job emitter ETA (`events.rs:620-686`, `imaging.rs:426-456`):
    `Δbytes/Δt` with `.max(1)` ms guard, EMA 0.6/0.4, ETA gated on
    `total > done && throughput > 0`, terminal sets `overall=1.0`/`eta=0`
    and reconciles `bytes_done = bytes_total` ✓.
24. Disk-map cell math (`engine/src/core/tree/cr_diskmap.cpp:22-29,93,50-53`):
    pow-2 cell growth under the 64 MiB overlay budget; `cell_count =
    ceil(slice/cell)`; `last_cell = (end−base−1)/cell` (no off-by-one);
    bad ranges clamped to the view ✓.
25. Bridge cell budget (`diagnostics.rs:327-345`): `div_ceil` + 64 KiB
    floor + `next_multiple_of` — 1 TiB with 4096 cells → 256 MiB cells,
    `total.div_ceil(256MiB) = 4096` ≤ cap ✓; caller cap clamped
    [64, 65536].
26. Sector-map merge (`sector-map-panel.tsx:29-52`): stride merge with
    max-rank fold; factor=ceil(len/max) ⇒ output ≤ max+… ✓ (1030 cells,
    factor 2 → 515 cells ≤ 1024).
27. Health grading (`engine/src/core/health/cr_grade.cpp`): all rule
    thresholds decoded 1:1 with clamps (`life<0→0`, `≥101→100`);
    `grade_nvme` cutoffs recomputed: percentage_used 95 → life 5 → Bad;
    90 → 10 → Caution; 89 → 11 → Good ✓; 4-state mapping in
    `health.rs:94-99` is 1:1 (no percentage invented; ring fill is a
    documented status-class visual, `health-page.tsx:102-112`).
28. NVMe parse (`cr_nvme.cpp:23-50` + `cr_nvme.h:71-73`): temperature_k
    LE16 ✓; `decidegrees_c = k·10 − 2731` → 303 K → 29.9 °C ✓ (273.15 →
    273.1 documented bias); data units × 512,000 ✓ (1000 units → exactly
    512 MB); percentage_used as single u8 (N-009a fix verified — no
    reserved-byte read).
29. SMART ATA parse (`cr_smart_ata.cpp:284-330`): 30×12-byte entries at +2;
    value@+3, worst@+4, raw[6]@+5 ✓; temp raw[0] with `!=0` guard;
    triple(raw) little-endian ✓; threshold matched by id ✓.
30. `letter_usage` (`devices.rs:53-75`): `total−free` saturating; unlettered
    volumes contribute 0 (documented in the module header) ✓.
31. Terminal counters (`scan.rs:542-606`): `files_found =
    files+deleted+carved` (disjoint engine categories) ✓; `data_found` =
    exact sum of non-dir node sizes from the frozen snapshot (G-3) ✓.
32. Results pagination (`results.ts:34-55` + `results.rs:518-531`):
    `skip(offset).take(limit)`; `getNextPageParam` = `offset+pageSize <
    total` — no off-by-one (next == total stops) ✓; `totalBytes` summed over
    the filter's hits ✓.
33. Recovery-report item classification (`commands/recovery.rs:10-13` +
    report outcomes): per-item outcomes are disjoint
    (done/salvaged→done, refused/resumed/nothing→skipped,
    error/hash_mismatch→failed) ✓ — only the aggregate stats path
    (SB-E-06) is wrong.
34. Disk-health window + health page: temperature `x10/10.0` bridge-side ✓;
    `Math.round` in the window vs fractional in the page — both truthful
    renderings; power-on-hours `floor(h/24)d h%24` ✓; wearPercent/100 →
    fraction before formatPercent ✓.
35. Recoverability small-result rule: engine `total < 100 → matched = 0`
    (`cr_grading.cpp:592-594`) faithfully mirrored in UI
    `verdictOf` (`recoverability-card.tsx:21-27`) — gauge hides under 100
    graded files ✓; ring geometry `2π·18 = 113.097 → 113` ✓;
    `Math.round(ratio·100)` percent ✓.
36. `Progress`/`ProgressGauge` (`ui/src/components/ui/progress.tsx`):
    clamp [0,1], `2π·(size−thickness)/2`, `dashoffset = c·(1−clamped)` ✓;
    linear bar `translateX(−(100−value))` with value∈[0,100] ✓ (recovery
    passes `overallProgress·100`, gauge passes the fraction — unit-correct).
37. Session load/save fractions: engine permille `min(1000)` →
    `fraction = permille/1000` (`events.rs:461-466`) →
    `Math.round(fraction·100)` renders ✓.
38. Stop-dialog files counter and celebration modal counters
    (`scan-progress-page.tsx:407,480-488`): engine `filesFound`/
    `dataFoundBytes` verbatim ✓.
39. CLI progress line (`engine/src/app/rrecov.cpp:171-203`):
    `permille/10.0` percent ✓; "100.0% done" printed only on success ✓;
    ETA via the verified `eta_compute` ✓.
40. Size-bucket filter boundaries (`cr_query.cpp:529-566`): inclusive upper
    bounds (≤512 KiB, (512 KiB,1 MiB], …) — og-exact, mutually exclusive ✓.
41. LBA/cluster→byte conversions (spot-verified):
    `lcn·cluster_bytes` in u64 at `cr_bitmap.cpp:55-56`,
    `cr_bootstrap.cpp:136,204,323,630,634`, `cr_mftwalk.cpp:353,1033-1035`
    — no u32 truncation, no overflow for representable disks ✓;
    `mft_lcn·u64(spc)·bps` (`cr_scan.cpp:1023`) u64 ✓.
42. Runlist codec (`cr_runlist.cpp`): signed-LE decode with sign extension,
    width round-trip in u64 space, INT64_MIN UB guard ✓.
43. `refresh_live_counts_`/`tel_counts_` (`cr_scan.cpp:584-645`): monotonic
    clamps on every live counter; `bytes_found` only grows ✓ (N-166/D4b).
44. Heartbeat watchdog (`cr_ui.cpp:1132-1140` + `events.rs:445-460`):
    explicitly carries standing telemetry only — no synthetic permille
    advance anywhere in the production path (the honest-variant rule) ✓.
45. `apply_folder_filter_` counters decrement to the pruned tree (counts
    describe the presented tree) ✓.
46. `weighted_progress` overflow safety: `u64(w.quick)·quick_pm` — weights
    are u32, permilles clamped ≤1000 ⇒ max 3·10⁶ fits trivially ✓.
47. Mock connector determinism: seeded RNG (`mulberry32`) with bounded
    `randInt` ✓ (pinned tests) — dev-only surface.

## Needs-runtime-proof list

1. SB-E-01/SB-E-02: pin the exact first-spike moment on a real FAT/exFAT
   volume (expected: after the first completed directory walk; gauge
   "Quick Scanning 100%" with overall 75% under default 40/35/25 weights).
2. SB-E-03/SB-E-04/SB-E-05: ext4/btrfs/HFS+ volumes — confirm 0%→100%
   jump (ext4 live walk), 100%-after-first-subvol (btrfs), ≤29% cap or
   mid-walk 100% (HFS+ chain length 300 / >1024).
3. SB-E-06: a recovery batch with salvaged + hash-mismatch items — observe
   `completedCount > totalCount` in `recovery-progress` payload (expected
   "12/10" shape for a 10-item all-salvaged queue).
4. SB-E-08: deep-stage discovery sub-pass on a multi-GB volume — observe
   etaSeconds/throughput oscillation after the sweep→discovery restart.
5. SB-E-10: 4Kn physical disk, scan page in the pre-state-load window —
   sector figures 8× off until telemetry arrives.
6. SB-E-12: 4Kn disk lost-partition candidate row — "Size" column 8× low.
7. Observation: `progress_display_remap` dead-code status (any caller
   appearing in a future merge should trip the fake-progress rule).
8. Deep sub-pass cursor restart: confirm the bottom "Reading sector: X/Y"
   line regresses by design at sub-pass boundaries (P-DEEP-01 semantics)
   and the ticker (currentSector/currentSectorTotal) stays consistent.

## Tally

~115 distinct numeric sites checked across engine / bridge / UI (progress
permille math ×23, ETA/throughput ×8, sector/LBA conversions ×12, size
formatters ×14, health/confidence ×10, counters/pagination ×20, misc ×28).
13 defects (5 engine progress-scope, 1 hard-coded denominator, 1 counter
double-count, 1 estimate-as-measurement, 1 ETA window mismatch, 4
UI-side unit/label/constant), 5 observations, 48 verified-correct checks.
No synthetic/fake progress generator exists in the production path; the
mock is dev/e2e-only and the heartbeat rule forbids permille invention.
