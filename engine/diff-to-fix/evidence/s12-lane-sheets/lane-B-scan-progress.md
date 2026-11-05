# LANE B — SCAN STRATEGY & PROGRESS MATHEMATICS & NO-STUCK/FREEZE (read-only audit)

Auditor: LB deep-dive lane. Core tree = 8d0d88b8 (`/home/z/audit/core_recovery`); corpus =
be0cfe9 (`/home/z/audit/easeus-drw-research`). Every cite below was read first-hand this
session. Corpus decompile cites use `DRWUI:<line>` = `re-workspace/reversed/ghidra-drwui/
DRWUI_decompiled.c`, engine cites = `re-workspace/reversed/ghidra-searchdll/<file>:<line>`,
`Device:<line>` = `re-workspace/reversed/ghidra-mo/Device_decompiled.c`, log cites =
`og-codebase/log/DRWUI.log:<line>` (null-stripped). Core cites are file:line in the core tree.

Headline decodes this lane contributes (both NEW, first-hand):

1. **The og's scan-progress percent formula** — raw sector ratio → piecewise-linear UX remap
   with a 99.99% stall cap (DRWUI:314931-314959 + DRWUI:314964-314986, decoded below). Never
   decoded before; the O38 "M-equivalent" verdict in `diff-to-fix/modules/orchestration.md:19-22`
   predates it (O38 cites only the session-save 40→65% formula).
2. **The og's ETA model** — whole-run rate + 0.2/0.8 EMA smoothing + a type-gated time-budget
   variant ×0.6 + a "3m00s" placeholder + 1 Hz local label countdown
   (DRWUI:314835-314887, DRWUI:1527650-1527768). O39's "the og's Remaining time formatting
   is UI-side. No engine gap" (`modules/orchestration.md:23-24`) dismisses a computed model,
   not formatting.

---

## 1. CONTEXT SHEET (surfaces examined, both-side cites, verdicts)

### Area 1 — Scan pass/phase structure (6 surfaces)

| # | Surface | Og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1.1 | Pipeline order | `CScanMgr::doScan:254` → `CDoScanPartition::PushScanDevice:58` → `PushFolderFilter:84` → `DoScan:94→:153` → `ScanThread type=0` (quick, ~21 s) → `UpdateScanFinishedInfo:498` → `ScanThread type=1` (advanced, ~14.5 min) → `Build tree finish.` (~40 s post-scan tree build) → auto-save `.rsf` → `CScanMgr::ScanFinished:636` (log:35,56-59,68-69,132,139-140; runtime-verified `UIUX/dynamic-ground-truth--scan-phase-captured.md:56,138`) | `run()` stages quick→deep→carve each checkpoint-gated (`cr_scan.cpp:1170-1201`); post-scan filterfolder + grading + read-truth gate (`:1219-1278`) | **MATCH (registered family)** — D-36 single-walk, S-006/M-005 composition, S-051 table targets; orchestration.md O37 |
| 1.2 | Quick⇄advanced combination | Quick results CARRY into advanced (ground truth: advanced 9% with 1.11 M files already found; `CQuickScanCompleteWnd` "Advanced Scan is trying to find more other lost files for you while recovering" — `UIUX/08-scan-phase-deep-dive.md:36`); advanced = separate complete-searcher generation (`FUN_140580720` builds `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` + `EUCreateRAWSearcher`, `architecture/04-recovery-workflow.md:61-65`) | Single walk with deep flags when deep configured (`run_quick` `cr_scan.cpp:1282-1302`, `run_engine_pass` deep_coming) | **COVERED-BY-D-36** (SCAN-ENGINE-DELTA C1/Q8 concur) |
| 1.3 | Sector range per phase | Quick AND deep use the SAME volume span: `Quick scan StartSector=705794048, TotalSector=270977023` / `Deep scan StartSector=705794048, TotalSector=270977023` logged back-to-back at scan setup (log:48-49); HFS+ quick attaches volume reader `FUN_18000af90(dev, 0, totalSectors…)` over the full span (HFSPlusSearchDLL:461,8790-8816) | Engines scan the caller slice; whole-disk scans fan out per-partition `PartitionSlice` targets (S-051 block, `cr_scan.cpp:1662-1763`) | **MATCH** (same-range semantics; S-051 landed) |
| 1.4 | Lost partitions as extra targets | `Insert lost partition … StartSector=0, TotalSector=270977023` fired INSIDE the advanced phase (log:70, 22:58:25 — 0.3 s after `ScanThread type=1`); lost-partition search sweep range `0-270977023` (`log/CalcPartition.log:1`); "reads the partition list, inserts lost partitions as additional scan targets" (`architecture/04-recovery-workflow.md:66-69`) | Discovery in the deep stage (`run_deep` `cr_scan.cpp:1854-2014`), candidates materialized + parallel-scanned, no-double-scan reconciliation vs table targets (`:1889-1904`) | **MATCH** |
| 1.5 | Carve placement + unallocated policy | RAWSearcher registered LAST in the advanced engine list, gated on settings+0x60 (`architecture/04:61-65`, `rawsearch-carving--deep-dive.md:70-83`); carve bitmap-gated by the engines' used-cluster bitmaps (M-005's ground truth); "Advanced Scan is searching files sector by sector" (UI string, `architecture/04:65`) | Carve = separate 3rd stage over the slice with free_map gate (M-005) + occupancy gate (M-039) + zero-stride skip (N-031) (`run_carve_stage` `cr_scan.cpp:2092-2198`) | **COVERED-BY-M-005/M-039/D-36** |
| 1.6 | Whole-disk vs partition targets | Scenario routing: partition → CDoScanPartition; lost/whole-disk → CDoScanLostPartition (`architecture/04:39-49` §2 table) | `--whole-disk` discovery + S-051 partition-list targets; `table_volumes/scanned/skipped` stats (`cr_scan.h:462-478`) | **MATCH** |

### Area 2 — Sector-range walk order & granularity (5 surfaces)

| # | Surface | Og | Core | Verdict |
|---|---|---|---|---|
| 2.1 | Walk order | Sequential forward sweep over the registered range; live ticker `Reading sector: 134316032/31413598` during advanced (ground truth :96,102); "sector by sector" copy (`architecture/04:65`) | Sequential forward: carve windows `wi=0..window_count` (`cr_carve.cpp:1143-1150`), lost discovery 64 KiB windows base+=win_bytes (`cr_candidates.cpp:773-787`), MFT record_no 0..max (`cr_mftwalk.cpp:441`) | **MATCH** |
| 2.2 | Device read chunk | 4 MiB chunked device layer: `*(int*)(p+0x10) = 0x400000 / sector_size` (Device:56545); 4 MiB prefetch read-ahead cache (DRWUI:919080-919092, :962250-962597, per D-72's cites) | 256 KiB cache blocks in an 8 MiB reactive LRU (`build_read_pipe(slice, 8<<20, 256*1024)` `cr_scan.cpp:1032`) | **COVERED-BY-D-72** (budget+algorithm registered); **OQ-S4-3 tracked** for scan-level relative perf |
| 2.3 | Engine batching | NTFS MFT stream batched at 2 MiB (4096-sector chunks: `(**(code**)(*p+8))(p, buf, lVar2, 0x1000, 0)` loop, NTFSSearchDLL:5651-5685); MFT bootstrap 64 KiB batch (Q2 row) | Cache-coalesced MFT reads (D-43), record reads through the retry+cache pipe | **COVERED-BY-D-43/OQ-S4-3** |
| 2.4 | I/O-error restart point | EXT4 CatchBuffer: on failure allocate fresh 0xE00 buffer, retry with 1 KB-backtrack shifted window, salvage what's readable, continue (`ext4-engine--deep-dive.md:25-33`); skip-mode "unreadable sectors are replaced with zeros" (BadSectorTipsDlg copy, D-38); driver completes the IRP with error (no retry) — `FltReadWriteSectorsCompletion` logs "Io error" (`eprdtdrv-sys--driver-internals.md:71`) | Transport-wide bisection retry down to 2× sector granularity, Skip-mode zero-fill, persistent ReadErrorMap, forward continue (`build_read_pipe` RetryPolicy max_attempts=3 Skip `cr_scan.cpp:170-172`; D-38) | **COVERED-BY-D-38** (core = registered superset) |
| 2.5 | Cache-block alignment | og every chunk a 64 KiB multiple by construction (D-72 note, `cr_scan.cpp:1017-1031`) | 256 KiB blocks; measured within 3.8% of og 4/7 MiB envelope, 18-19% faster at 256 KiB (OQ-S4-3 transport half) | **REGISTERED-D-72 + tracked OQ-S4-3** |

### Area 3 — Progress mathematics (7 surfaces)

| # | Surface | Og | Core | Verdict |
|---|---|---|---|---|
| 3.1 | Raw percent input | Engine progress messages `{type@+0, total@+8, done@+0x10}` → `FUN_1401ad580` (CScanMgr handler, DRWUI:313925-313974); percent = `(done·100)/total` (`FUN_1401af950` DRWUI:314964-314986); done==0 or total<done or total==0 → no update (returns −1, `FUN_1401af410` DRWUI:314846-848) | Stage permille `pm = done·1000/total`, done≥total→1000 (`tel_publish_` `cr_scan.cpp:354-376`); units are engine-native (MFT records `cr_mftwalk.cpp:444`, dentries `cr_fat_quick.cpp:148`, inodes `cr_ext4_engine.cpp:1067`, carve BYTES `cr_carve.cpp:1146`) | **DIVERGENT** → LB-1 (input ratio MATCH; units + downstream model differ) |
| 3.2 | Display-percent remap | Type-gated: type==0 → 0.0; type==1 → piecewise remap `FUN_1401af820` (DRWUI:314931-314959): raw<11 → raw·2; 11≤raw<30 → (raw−11)·0.2+20; 30≤raw≤90 → raw·0.8; 90<raw≤98 → (raw−88)·2.7+72; raw>98 → raw; cap 99.99 unless raw==100; type==6 → raw percent (cloud) | No remap anywhere; linear permille end-to-end | **DIVERGENT** → LB-1 |
| 3.3 | Phase weighting | Per-phase percent (no cross-phase weights): label flips Quick→Advanced (`m_iScanType 1→2`, bottomarea.cpp:838/856, log:60-61), percent restarts low for advanced (ground truth: 9% at 75 s while quick had finished) | Weighted overall `(40·q+35·d+25·c)/100` permille (`weighted_progress` `cr_scan.cpp:129-144`, weights `cr_scan.h:71-75`); per-stage telemetry ALSO exposed (restarts per stage, `tel_stage_begin_` resets `cr_scan.cpp:319-335`) | **DIVERGENT** → LB-1 (both shapes expressible; defaults differ) |
| 3.4 | Monotonicity | Publish only raises: `FUN_1401afa60` reads current via vt+8, sets only if current<new (DRWUI:315014-315024) | Within-stage clamp `pm < st.permille → pm = st.permille`; overall ratchet `overall > snap.overall → update` (`cr_scan.cpp:371-372,387-388`) | **MATCH** |
| 3.5 | Completion forcing | Message type==3 → `FUN_1401af410(obj,100,100)` → 100% (DRWUI:313954-313957) | `tel_stage_end_` sets permille=1000+completed (`cr_scan.cpp:337-352`); boundary ticks forward post-stamp truth, never a hardcoded weighted guess (`cr_scan.cpp:1295-1300,2022-2024,2194-2196`) | **MATCH** |
| 3.6 | Event/callback cadence | Push: engines post progress messages; UI polls 38 `timeout()` consumers; sole push signal `sigPercentChange` (UIUX/25:107-116,186-188); `Quick scan matched=N total=M` counter lines ~2 s cadence (log:62-112) | Push: `CR_EVT_SCAN_PROGRESS` coalesced by overall-permille advance (≤1000+flips per scan) via `emit_progress_event_` CAS gate (`cr_scan.cpp:441-459`); telemetry pull any thread (`telemetry()` :554-578) | **COVERED-BY-D-78** (delivery model); cadence granularity → LB-4 for counters |
| 3.7 | Pause compensation | Pause exists (capture: Pause/Stop controls :99); og elapsed fields uncompensated (rsf record-2 stat1 = raw elapsed ms, `rsf-format--scan-session.md:173`) | Pause-compensated elapsed everywhere (`stamp()` `cr_scan.cpp:1125-1130`, `tel_publish_` :379-384) | **MATCH (core superset)** |

### Area 4 — ETA computation (3 surfaces)

| # | Surface | Og | Core | Verdict |
|---|---|---|---|---|
| 4.1 | ETA model | `FUN_1401af410` (DRWUI:314835-314887): elapsed=GetTickCount−startTick; `eta_s = ((total−done)·elapsed/done)/1000`; rate EMA `smoothed=(2·rate+8·prev)/10`; type==0 → store raw eta; type≠0 → `eta=(budget(+0x120) − budget·(elapsed/1000)/projected)·0.6` with `projected=total·(elapsed/1000)/done` | No ETA computation anywhere (grep over src/+include/: zero ETA surfaces; CLI prints elapsed only `rrecov.cpp:129-132`) | **DIVERGENT** → LB-2 |
| 4.2 | ETA display machinery | "Remaining time: %1" label (qt_strings 0x1fede; DRWUI:1527656,1527752); initial placeholder hardcoded "3m00s" when label empty (DRWUI:1527664-7669); 1 Hz local decrement parses Xm Ys and re-renders (DRWUI:1527676-1527762); runtime evidence "Remaining time: 07m43s" at 34% (ground truth :95,102) | None (hosts may derive linear ETA from telemetry permille+elapsed; nothing implements the og model) | **DIVERGENT** → LB-2 |
| 4.3 | Sector ticker | `Reading sector: X/Y` alongside ETA (ground truth :96,102); done/total stored at obj+0x36/+0x37 in the type-6 branch of `FUN_1401ad580` (DRWUI:313949-313950) | `bytes_done/bytes_total` on stage-2 events (carve-true bytes) + `bytes_scanned` telemetry (`cr_scan.h:146,506`) | **MATCH (shape)** — structured stages report engine units, not sectors (see OQ-5) |

### Area 5 — NO STUCK OR FREEZE / liveness (12 surfaces)

| # | Surface | Og | Core | Verdict |
|---|---|---|---|---|
| 5.1 | Engine-hang watchdog | 5 s watchdog thread `FUN_140577100` (DRWUI:910565-910585, VERIFIED first-hand): `while(!stopped){Sleep(5000); if(stepActual(+0x8fc) < stepExpected(+0x8f8)){ coarseProgress(+0xe8)++; stepActual++; re-fire progress callback; }}` — FABRICATES coarse progress movement during stalls; started CreateThread :910864 | Honest variant: `CR_EVT_HEARTBEAT` every 5 s when no progress in window, carrying standing telemetry truth (never bumped) (`cr_ui.cpp:983-1059`) | **COVERED — N-043 registered divergence** (re-verified both sides) |
| 5.2 | Blocked-read hang | Driver has NO IRP cancellation: `KeWaitForSingleObject` forward-wait, no cancel routine — "Blocked reads therefore hang the stage" (D-18, `02-kernel-driver-architecture.md:45-51`) | CancelIoEx/abortable reads; token polled at every loop head (D-18) | **COVERED-BY-D-18** (core strictly better) |
| 5.3 | FAT chain cycle guard | Visited-range red-black tree: `range_tree.insert(cluster)` per run; repeated range → stop — "a cycle-breaker so corrupt FAT loops cannot hang the scan" (`fat-exfat-engine-algorithms-v2.md:43-56`); chain sanity arithmetic at 0x10000 scale (FATSearchDLL:11839-11857) | Visited `std::vector<u64>` + LINEAR scan per step: `for (u64 v : visited) if (v==c)` (`cr_chain.cpp:429-447`, `:475-494`; exFAT twin `cr_exfat_quick.cpp:94-107`) — O(n²) per chain, n≤0x10000 at call sites (`cr_fat_quick.cpp:32,263`) | **DIVERGENT** → LB-3 (both terminate; complexity differs) |
| 5.4 | Chain length caps | og sanity < 0x10000 family (Q3 row; FATSearchDLL:11852) | `follow_chain` default cap 0x100000 (header), call sites pass 0x10000; `reconstruct_deleted_chain` limit = expect_clusters or 0x10000 (`cr_fat_engine.h:175-184`, `cr_chain.cpp:478`) | **REGISTERED** (Q3 "chain cap 0x100000 core constant registered") |
| 5.5 | MFT walk termination | Record loop `for i in 0..record_count` with per-record cancel callback (`ntfs-engine-algorithms-v2--deleted-viability.md:15-28`) | `for (record_no=0; record_no<max_record; ++record_no)` + cancel per record + progress every 64 (`cr_mftwalk.cpp:441-445`) | **MATCH** |
| 5.6 | $ATTRIBUTE_LIST cycles | og chase UNBOUNDED (D-14/F6: "no depth bound recorded") | `FRAG_DEPTH_LIMIT=64` + visited-list cycle guard (`cr_mftwalk.cpp:24,504-528`) | **COVERED-BY-D-14/F6** (core strictly better) |
| 5.7 | Btrfs chunk-tree cycles | No corpus decode of a chunk-tree cycle guard located (btrfs-engine--deep-dive) | `walk_tree`: `std::set` visited + `MAX_NODES=1<<20` + cancel per node (`cr_btrfs_engine.cpp:129,385-395`) | **MATCH (core bounded; og open)** → OQ-4 |
| 5.8 | HFS+/ZFS/ext4/ReFS bounds | (corpus: per-engine walks; no unbounded loops identified in the deep-dives) | HFS+ `MAX_TREE_NODES=1<<22` + per-node guard (`cr_hfsplus_engine.cpp:98,402,1380,1640`); ZFS MAX_OBJECTS/MAX_ZAP/depth 48 (`cr_zfs_engine.cpp:389-395`); exFAT `EXFAT_MAX_DEPTH=64` (`cr_exfat_quick.cpp:291`) | **MATCH** |
| 5.9 | Carve loop bounds | Per-format size caps (ZIP padding cap 5,242,881 B, global carve cap 0xBEBC200 — SCAN-ENGINE-DELTA A4) | 1 MiB windows × window_count, cancel per window, per-format max-size gates (`cr_carve.cpp:1137-1150`) | **MATCH** |
| 5.10 | Cancel check frequency | Cancel callback at every engine loop head (D-18 cite: `controller.cancel_callback.vt[0x10]()` per record, `ntfs-engine-algorithms-v2:18`); UI `CStopScanThread` cooperative | Per MFT record (`cr_mftwalk.cpp:442`), per FAT chain step (`cr_chain.cpp:433-437,480-484`), per carve window (`cr_carve.cpp:1144`), per discovery window (`cr_candidates.cpp:775`), per btrfs node; 50 ms pool watchers N-017 (`cr_parallel.cpp:152-169`, `cr_scan.cpp:1971`, `cr_carve.cpp:1180-1187`) | **MATCH (core ≥ og)** — N-017 fixed the orchestrator gap |
| 5.11 | exFAT FREE-entry continuation | `n = c+1` under "assumed" flag when FAT entry zeroed, bounded by data-length expectation (FUN_180021c60:28158-28163, decoded in `cr_exfat_quick.cpp:112-127`) | Same heuristic, bounded by max_clusters/cluster_count (`cr_exfat_quick.cpp:111-131`) | **MATCH** |
| 5.12 | Read-truth / fake-completion | og: zero-fill skip mode can complete silently (product copy only); no refusal surface decoded | Fully-blind scan REFUSES `ERR_READ_FAILED`; partial damage surfaced via bytes_unreadable + warn (`cr_scan.cpp:1235-1278`) | **COVERED** (D-38/D-DRV-6 registered superset) |

Concrete hang-inducing inputs traced (the directive's enumeration):

| Input | Og behavior | Core behavior |
|---|---|---|
| Cyclic FAT chain (cluster→earlier cluster) | RB-tree range guard stops the chain in O(log n) (fat-exfat-algorithms-v2:44-55) | Terminates via visited scan, but O(n²) comparisons — seconds per maximal chain (LB-3) |
| Cyclic $ATTRIBUTE_LIST (A→B→A) | Unbounded chase — can hang (registered D-14/F6) | Depth 64 + visited list — terminates |
| MFT runlist loop | Length-bounded per record; record loop bounded | Same — bounded |
| Corrupt btrfs chunk tree (logical cycle) | No guard decoded (OQ-4) | set-visited + 2^20 node cap — terminates |
| Blocked device read (dying disk / cable pull) | Driver forward-wait, no cancel → stage hangs; watchdog fakes progress; UI offers Drive-Disconnected Retry/Cancel (N-125) | Abortable reads + cancel everywhere + read-truth gate |
| Zeroed exFAT FAT (deleted chains) | c+1 assumed-contiguous, expectation-bounded | Same, cap-bounded |
| Engine stall (slow I/O, not hang) | 5 s watchdog bumps fake coarse progress (FUN_140577100) | 5 s honest heartbeat event |

### Area 6 — Scan statistics accuracy (6 surfaces)

| # | Surface | Og | Core | Verdict |
|---|---|---|---|---|
| 6.1 | Headline counts | "Scan Completed, Files Found: 1139286 (147.12 GB)" — count + total SIZE (ground truth :125); modal grid "Files found · Files scanned · Used Time · Size · Organized…" (`UIUX/08:32`) | `files_found/deleted_found/carved_found/lost_partitions` counts; NO total-size-of-found aggregate in ScanStats (cr_scan.h:345-357); CLI prints counts only (`rrecov.cpp:1662`) | **DIVERGENT** → LB-5 |
| 6.2 | Live count cadence | Counts grow live during scan (Files Found updating; Reconstructed 1→16; `Quick scan matched=N` every ~2 s, log:62-112) | Counters refresh only at stage boundaries (`tel_counts_` call sites `cr_scan.cpp:1135,1293,2020,2192`); event contract documents stage-boundary FilesFound (cr_scan.h:116-118); tree nodes attach live but telemetry counters freeze mid-stage | **DIVERGENT** → LB-4 |
| 6.3 | Elapsed time | rsf record-2 stat1 = elapsed ms, stat2 = tick counter (rsf-format:173-176) | `elapsed_ms` pause-compensated (cr_scan.cpp:1125-1130) | **MATCH (core superset)** |
| 6.4 | Bad-sector accounting | Zero-fill skip + BadSectorTipsDlg copy; per-engine salvage (EXT4 CatchBuffer) | ReadErrorMap + `bytes_unreadable` + per-range module/attribution + salvaged flags (cr_scan.h:382-403, M-049) | **COVERED-BY-D-38** (superset) |
| 6.5 | Per-phase file counts | Quick pass files (9,423 in the log session) then advanced totals; "Recovery ability matched/total" grading counters (log:99,110) | Per-stage counts + AssistantStats (S-023) persisted; carve per-format stats = **N-140 (already filed)** | **MATCH / COVERED-BY-N-140** |
| 6.6 | Sectors-scanned surface | "Scanned disk sectors: %1" on stop (`UIUX/08:20`); `Reading sector: X/Y` ticker | `bytes_scanned` is carve-true-bytes only — structured stages deliberately not byte-fabricated (cr_scan.h:481-489,506) | **MATCH (honest-units design; documented)** |

---

## 2. DIVERGENCE CANDIDATES

### LB-1 — Scan-progress display mathematics: the og's piecewise UX remap + 99.99% stall cap vs the core's linear weighted permille

- **Severity: MAJOR** (user-observable on the headline percent every scan; honest-linear is
  defensible and per-stage permille IS exposed — but the og's curve is decoded nowhere,
  implemented nowhere, and unregistered; O38's "M-equivalent" was written without it).
- **Og behavior (cites):** engine message `{type, total@+8, done@+0x10}` → `FUN_1401ad580`
  (DRWUI:313925-313974) → `FUN_1401af410` (DRWUI:314835-314887) → `FUN_1401af950`
  (DRWUI:314964-314986):
  ```
  raw = (done * 100) / total                    // sector-based for the deep/RAW pass
  if type == 0: percent = 0
  elif type == 1: percent = REMAP(raw)          // FUN_1401af820, DRWUI:314931-314959
  else: percent = raw                           // type 6 (cloud) observed
  REMAP(raw):
    raw < 11        -> raw * 2.0
    11 <= raw < 30  -> (raw - 11) * 0.2 + 20.0
    30 <= raw <= 90 -> raw * 0.8
    90 < raw <= 98  -> (raw - 88) * 2.7 + 72.0
    raw > 98        -> raw
    if percent > 100 and raw != 100: percent = 99.99
  publish only if percent > current             // FUN_1401afa60, DRWUI:315014-315024
  on completion msg (type 3): force (100,100)   // DRWUI:313954-313957
  ```
  Per-phase percent (restarts at the quick→advanced label flip, bottomarea m_iScanType 1→2,
  log:60-61); ground truth anchors: displayed 9% at t=75 s, 34% at t≈460 s
  (dynamic-ground-truth--scan-phase-captured.md:39,90).
- **Core behavior (cites):** `weighted_progress` (cr_scan.cpp:129-144):
  ```
  overall_permille = (40*q + 35*d + 25*c) / 100     // q,d,c ∈ [0,1000] stage permilles
  stage_pm = done*1000/total (clamped, monotonic)   // tel_publish_ cr_scan.cpp:354-376
  ```
  No remap, no 99.99 cap (cap is 1000‰ = the true end), single monotonic overall across all
  stages; per-stage telemetry also exposed and resets per stage (cr_scan.cpp:319-335).
- **Worked example (where they differ):** advanced pass at raw sector-completion 50%,
  quick already done, carve not started:
  - og display: REMAP(50) = 50·0.8 = **40%** (the user sees 40%).
  - core overall: q=1000, d=500, c=0 → (40·1000+35·500+25·0)/100 = 575‰ = **57.5%**.
  - Early scan (raw 5% into quick on an NTFS volume): og REMAP(5)=**10%**; core
    (40·50)/100 = 20‰ = **2.0%** — a 5× presentation gap in the other direction.
  - Stall at raw 99%: og caps the display at 99.99% forever until the completion message;
    the core's bar sits at the true 99%-equivalent permille — same observable, different rule.
- **PARITY-DELTAS check:** D-78 registers the DELIVERY model (push+pull vs og's poll farm) —
  not the math. D-36/Q8/O38 register the single-walk + weighted-progress STAGE model. The
  remap curve itself appears in no registry, no module doc, and no finding; O38's
  "M-equivalent" verdict (orchestration.md:19-22) predates this decode and cites only the
  session-save 40→65% formula as "UI-session constructs". New ground.
- **ID placeholder: LB-1.**

### LB-2 — ETA: the og's rate-EMA remaining-time model (with time-budget variant, 3m00s placeholder, 1 Hz countdown) vs no ETA surface anywhere in the core

- **Severity: MAJOR** (a whole user-visible surface absent; the inputs to build it are
  exposed, the model is not).
- **Og behavior (cites):** computed in `FUN_1401af410` (DRWUI:314835-314887) on every
  progress message:
  ```
  elapsed_ms = GetTickCount() - startTick          // whole-run elapsed (not a window)
  eta_s      = ((total - done) * elapsed_ms / done) / 1000
  rate       = (total - done) / eta_s
  smoothed   = (2*rate + 8*prev_smoothed) / 10     // EMA, 0.2 instant / 0.8 history
  if type == 0: eta_field = eta_s
  else: projected = total * (elapsed_ms/1000) / done
        eta_field  = (budget(+0x120) - budget*(elapsed_ms/1000)/projected) * 0.6
  ```
  Display: "Remaining time: %1" (qt_strings 0x1fede; DRWUI:1527656,1527752); when the label
  is EMPTY the placeholder is the literal "3m00s" (DRWUI:1527664-7669); a 1 Hz slot
  (FUN_1409cb530, DRWUI:1527550-1527768, meta-slot 7 at DRWUI:1783107) parses "Xm Ys",
  subtracts a second, re-renders — the label counts down locally between computed refreshes.
  Runtime evidence: "Remaining time: 07m43s" at 34% (dynamic-ground-truth:95,102).
- **Core behavior (cites):** zero ETA computation in engine, CLI, or cr_ui (grep
  remaining_time/estimate/eta over src/+include/: no scan-ETA surface; CLI prints elapsed
  seconds only, rrecov.cpp:129-132). Telemetry exposes overall_permille + elapsed_ms +
  per-stage bytes (cr_scan.h:490-512) — sufficient for a host-side linear ETA
  (elapsed·(1000−pm)/pm) but nobody implements it, and the og's EMA/time-budget/×0.6 model
  is decoded nowhere. O39 (orchestration.md:23-24) dispositioned "the og's Remaining time
  formatting is UI-side. No engine gap" — the decode shows it is a computed model in DRWUI
  (host layer), so "no ENGINE gap" is technically true but the product gap is real and the
  verdict's premise (formatting) is wrong.
- **Worked example:** 100 GB volume, 40 GB scanned in 400 s: og eta =
  (60 GB·400 s)/40 GB = 600 s, displayed via the 1 Hz countdown; EMA damps rate spikes
  (a 10× rate burst moves smoothed by ~22%). Core: the CLI shows "40.0% (400 s)" — no
  remaining time; a host would have to derive it.
- **PARITY-DELTAS check:** no ETA entry in any registry (D-78 is delivery; O39 is the
  disposition being corrected). New ground.
- **ID placeholder: LB-2.**

### LB-3 — FAT/exFAT chain cycle-guard complexity: O(n²) visited-vector scan vs the og's visited-range red-black tree

- **Severity: MAJOR** (edge-case/scale: user-observable multi-minute stall — "stuck"
  appearance — on FAT32 volumes with many large files; bounded, never infinite).
- **Og behavior (cites):** the FAT chain walker inserts each cluster RUN into a red-black
  range tree (`range_tree.insert(cluster)` per run; repeated range → stop) — "a
  cycle-breaker so corrupt FAT loops (a cluster pointing backward) cannot hang the scan"
  (`fat-exfat-engine-algorithms-v2.md:43-56`); chain sanity arithmetic at the 0x10000 scale
  (FATSearchDLL:11839-11857).
- **Core behavior (cites):** `follow_chain` (cr_chain.cpp:423-463) and
  `reconstruct_deleted_chain` (cr_chain.cpp:465-494) push every cluster into
  `std::vector<u64> visited` and do a LINEAR scan `for (u64 v : visited) if (v == c)` on
  EVERY step; the exFAT twin `follow_exfat_chain` (cr_exfat_quick.cpp:91-133) has the same
  shape. Call sites cap chains at 0x10000 = 65,536 clusters (cr_fat_quick.cpp:32,263;
  cr_exfat_quick.cpp:257,446,514,740) → worst case ~n²/2 ≈ 2.1×10⁹ comparisons per maximal
  chain. Note the btrfs walker already uses the correct structure (`std::set` visited +
  MAX_NODES, cr_btrfs_engine.cpp:389-395) — this is FAT/exFAT-specific.
- **Worked example:** FAT32, 4 KiB clusters, 100 files × 256 MB = 100 chains × 65,536
  clusters. og: 100 × 65,536 × O(log n) inserts ≈ milliseconds. Core: 100 × ~2.1×10⁹
  comparisons ≈ **100+ seconds of pure cycle-check overhead** before any I/O — the quick
  stage appears frozen (progress ticks only every 64 records / per dentry batch), where the
  og finishes in seconds. Cancel still works (checked per chain step, cr_chain.cpp:433-437),
  so this is a stall-class issue, not a hang.
- **PARITY-DELTAS check:** D-14/F7 registers the visited-set guards as a hardening family
  (og guard exists, core adds guards to EBR/GPT/attrlist) and Q3 registers the chain-cap
  VALUE (0x100000 core default). The ALGORITHMIC complexity divergence (vector scan vs
  RB-tree) is in no registry. New ground.
- **ID placeholder: LB-3.**

### LB-4 — Live file-count telemetry is stage-boundary-only; the og's counters grow live during the scan

- **Severity: MINOR** (data reachable via the tree; the event stream lags; one module-doc
  claim is inaccurate).
- **Og behavior (cites):** "Files Found: 1139157 (143.74 GB)" updating during the advanced
  scan; the Reconstructed branch grows 1→16 live over 6 minutes
  (dynamic-ground-truth:91-105); `Quick scan matched=N, total=9423` counter lines at ~2 s
  cadence throughout (DRWUI.log:62-112) — the UI's counts track the live tree.
- **Core behavior (cites):** `tel_counts_` (cr_scan.cpp:391-411) runs only at stamp
  (:1135) and the three stage ends (:1293, :2020, :2192); during a stage,
  `emit_progress_event_` → `emit_event_` re-reads `tel_.snap.files_found/carved_found`
  frozen at the last boundary (cr_scan.cpp:417-439) — a 10-minute carve stage streams
  ScanProgress events whose file counters never move, even though carved nodes attach to
  the tree live (carve_slice streams during the pass). The header contract is explicit
  ("FilesFound … are STAGE-BOUNDARY samples", cr_scan.h:116-118), but
  `modules/orchestration.md:210-213` claims "counters refreshing at progress ticks" —
  that claim is inaccurate per the code.
- **Worked example:** a host rendering "Files Found" from `CR_EVT_SCAN_PROGRESS` shows a
  frozen count for the entire carve stage; the same host counting the query tree shows the
  og's live growth. The og's number and the tree agree at every 2 s tick.
- **PARITY-DELTAS check:** D-78 covers delivery, not count-refresh granularity; the
  session-8 orchestration note (210-213) is the only place that touches it and gets it
  wrong. New ground (small).
- **ID placeholder: LB-4.**

### LB-5 — Scan-stats headline: the og reports found-count + total found SIZE; the core's ScanStats has no size aggregate

- **Severity: MINOR.**
- **Og behavior (cites):** completion line "Scan Completed, Files Found: 1139286
  (147.12 GB)" (dynamic-ground-truth:125); completion-modal grid "Files found · Files
  scanned · Used Time · Size · Organized · Completed …" (UIUX/08:32).
- **Core behavior (cites):** `ScanStats` (cr_scan.h:345-479) carries counts
  (files/deleted/carved/lost), `bytes_scanned` = carve-true bytes only, `bytes_unreadable`,
  elapsed — no sum of found logical sizes; the CLI summary prints counts only
  (rrecov.cpp:1662); no report field for total found size (docs/recovery-report-schema.md).
- **Worked example:** same scan both sides: og headline "1,139,286 files (147.12 GB)";
  core headline "files=1139286 deleted=… carved=…" — the GB figure must be recomputed by
  walking the tree.
- **PARITY-DELTAS check:** no registry entry covers a size aggregate (N-140 is per-format
  carve stats — adjacent but distinct, already filed). New ground (small).
- **ID placeholder: LB-5.**

---

## 3. OPEN QUESTIONS

1. **OQ-1 (quick-phase percent units):** the og's quick-scan percent source is unresolved —
   the (done,total) message decode is solid, but which quantity the QUICK searchers report
   (MFT records vs sectors vs something else) was not located in the engine decompiles
   (SearchDLL artifacts are silent on progress). The deep/RAW pass is sector-based
   ("Reading sector: X/Y"). Affects any host replicating the og's quick-phase bar.
2. **OQ-2 (the ETA time budget +0x120):** in `FUN_1401af410`'s type≠0 branch, the field at
   +0x120 acts as a total-time budget scaled ×0.6. What sets it (configured expected
   duration? a previously projected total? a per-scan-type constant?) is unresolved; if
   +0x120 == the projected total the formula reduces to 0.6·remaining-projected.
3. **OQ-3 (og advanced-phase concurrency):** `architecture/04-recovery-workflow.md:73`
   says the structured scan and RAW carving "run concurrently", while the searcher list
   (NTFS→FAT→…→RAW last, FUN_140580720) reads as sequential dispatch. Which threads run
   the FS searchers vs RAWSearcher during the advanced phase is unresolved (matters for
   time-to-first-carved-result comparisons).
4. **OQ-4 (og btrfs chunk-tree cycle guard):** no corpus decode located for a cycle/depth
   guard in BtrfsSearchDLL's chunk-tree walk; the core is bounded (visited set + 2^20 cap).
   A corrupt-chunk-tree input's og behavior is undetermined.
5. **OQ-5 ("Reading sector: 134316032/31413598" semantics):** current > total in the
   capture — X is likely a disk-absolute LBA (or a byte count) against a volume-relative
   total Y; the exact unit pair is unresolved (both sides expose current/total shapes, so
   this is cosmetic, but a parity ticker needs the units).
6. **OQ-6 (progress-object type ids):** the og's percent path gates on
   GetType()-style ids (0 → percent forced 0; 1 → remap; 6 → raw + done/total slots stored).
   Which id maps to the standard disk scan (presumed 1) vs cloud (6) vs the 0 case is
   inferred from the code shape, not from a type-table decode.
7. **OQ-7 (LB-1 disposition):** should the og's remap curve be (a) registered as a
   deliberate non-adoption (honest-linear progress), (b) ported into the wizard host layer
   for og-identical UX, or (c) exposed as a host-side helper? The engine already exposes
   per-stage permille; the decision is a product-policy call that O38's current text does
   not capture.

---

### Verdict roll-up

- 39 surfaces examined across the 6 directive areas.
- MATCH / core-superset: 21 · COVERED-BY-registered-delta: 12 (D-18, D-36, D-38, D-43,
  D-72/D-72-family, D-14/F6/F7, D-78, M-005/M-039, N-043, N-140, Q3-cap, OQ-S4-3) ·
  registered-scope dispositions needing correction: 2 (O38, O39 — superseded by LB-1/LB-2
  decodes) · DIVERGENT candidates: 5 (LB-1..LB-5).
- No ACUTE findings: every hang-class input traced terminates on both sides except where
  already registered (og's unbounded attrlist chase D-14/F6; og's non-cancellable driver
  reads D-18) — the core is strictly more live than the og on those. The one core-side
  liveness regression is LB-3 (bounded but O(n²) — stall-class, not a hang).
