# Agent D — core_recovery STAGE-AND-BOUNDS MAP (scan orchestration / state / session persistence)

Scope: `src/core/orchestrator/cr_scan.cpp` (3043 ln, read fully), `cr_filterfolder.cpp`, `cr_vollabel.cpp`,
`src/core/tree/cr_grading.cpp`, `cr_snapshot.cpp`, `cr_diskmap.cpp`, `src/core/session/cr_crsf.cpp`,
`cr_records.cpp`, `src/core/storage/cr_retry.cpp`, `cr_cached.cpp`, `cr_slice.cpp`,
`src/core/lost/cr_candidates.cpp`, `cr_consistency.cpp`, `src/app/rrecov.cpp` (3.7 k ln; scan/state/persistence
paths read fully, remaining verbs census-scanned), plus the `include/cr/{orchestrator,session,tree,lost,storage}/*.h`
headers and `cr_device.h` (CancelToken/PausableToken), `cr_parallel.cpp` (pool/watcher). All cites are file:line in
`core_recovery/`. READ-ONLY audit — no files modified. Known-covered items NOT re-filed: N-203 (PartitionContext v4
data-area extent — verified live at cr_records.cpp:961-999 / cr_candidates.cpp:617-624), N-205 (raw_tail_zero /
raw_count grading population — verified live at cr_grading.cpp:398-437, 459-461), M-135/M-136 (recovery-read
semantics — verified LIVE per diff-to-fix/evidence/s55-lane-sheets/S55-L1-liveness-tier1.md:56 and re-confirmed in
this tree: rrecov.cpp:2375 hands the RAW device to `SafeWriter` while `build_read_pipe` (cr_scan.cpp:163-202) is
scan-only; zero-fill+delete posture in cr_safewriter.cpp per the sheet).

---

## 1. Stage model table

The pipeline is `ScanController::run()` (cr_scan.cpp:1061-1513). Stage weights default quick=40 / deep=35 / carve=25
permille-of-total (cr_scan.h:72-76). All stages **merge additively** into one shared `ResultTree` under distinct
roots (Found / Deleted / "File Path Lost" / Reconstructed — ctor cr_scan.cpp:304-320; resume root recovery by role
:336-369). No stage replaces another's results; carve dedups against structured output via two gates (below).

| # | Stage / sub-pass | Range derived from | Merge semantics | Cancel checkpoints (file:line) |
|---|---|---|---|---|
| 0 | Search-start disk-truth pass (M-042) — live table capture → `disk_layout_`; layout compare vs persisted snapshot; re-validate restored lost candidates | whole disk (slice) via `parse_disk_table` (:1085-1098) | stats-only (layout_changed, dropped_stale_candidates); candidates REPLACED by confirmed subset (:1116-1128) | none (pre-stage; bounded table reads) |
| 1 | Pre-scan repair (S-008, optional `pre_scan_repair`) | caller's raw slice, pre-pipe | plan surfaced in stats/events; writes only on consent+writer; **never aborts scan** (:1150-1207) | gate `!(cancel && cancel->checkpoint())` :1150 |
| 2 | Pre-scan damage audit (M-041, optional `pre_scan_check`) | caller's raw slice, post-repair | report into stats + REC_FS_CHECK; never aborts (:1217-1231) | gate :1217 |
| 3 | Read pipe build — `build_read_pipe(slice, 8 MiB, 256 KiB)` :1258 (cr_scan.cpp:163-202): slice→SliceShim→RetryingDevice(max_attempts=3, Skip, backtrack)→CachedReader(8 MiB/32 blocks). Degraded path = bare slice (M-058 pinned contract) | whole slice | n/a | n/a |
| 4 | `probe_family` (only when fs_hint Unknown) — fixed reads at LBA0/1024/64K/128K; blind-probe detection (error range covering offset 0 → `stats.probe_unreadable`) :1272-1313 | boot sector + fixed anchors | sets cfg_.fs_hint (dispatch selector) | engine-free; single reads |
| 5 | Volume facts + boot-sector CRC32 identity (SESS-2) :1317-1349 | first 512 B of slice | vol_offset_/vol_bytes_/vol_boot_hash_/boot snapshot | n/a |
| 6 | **QUICK stage** — `run_quick` :1515-1535 → `run_engine_pass(slice, deep_mode=run_deep, bridge_stage=0, covers_both=deep_coming)` :1803-2066. One shared engine walk (single-walk design; deep-coming sets deep-semantics flags). Then F-1 rebase (:1403) + stage-boundary checkpoint (:1404). | Whole slice, single dispatch; OR whole-disk table-target fan-out (per used non-container partition `PartitionSlice`, probed independently, candidate-mode error swallowing) :1927-2030; OR LDM dynamic volumes :1837-1887 | appends into Found/Deleted/Lost roots; per-volume display-name roots on table fan-out; free_map set by main pass only (:2062-2064) | stage-entry :1393; per-table-target :1947-1948; engine loop-head polls (bridge contract :1806-1816); progress-false |
| 7 | **DEEP stage** — `run_deep` :2068-2363. (a) late engine pass iff !quick_done_ (:2078-2084, rebase, `pass_deep_done_`); (b) NTFS MFT remnant sweep, parallel chunk fan-out (:2097-2121); (c) whole-disk lost discovery (`lost::discover` stride 1, :2124-2141); no-double-scan reconciliation vs `table_target_lbas_` (:2158-2173); forced checkpoint on first candidates (:2198, M-042 per-hit persistence); ≤10 candidate scans (M-137 cap `kLostInsertCap=10` :2217) via ThreadPool+50 ms CancelPoolWatcher (:2279-2333) or serial (:2335-2345); exFAT content-consistency reject detaches subtree (M-035, :2405-2425). `deep_extras_done_` stamped only when rc==OK (:2354-2355). | structured: whole slice / table targets; remnant sweep: MFT stream; discovery: every sector of the disk, 64 KiB windows, stride 1 (cr_candidates.cpp:863-984) | appends (remnants → File Path Lost root; candidates → own named roots, extents rebased per-candidate) | stage-entry :1408; discovery per-window cr_candidates.cpp:864-867 (cancel + progress-false); worker pool polls external token in engine loop heads; watcher aborts queued tasks within 50 ms (cr_parallel.cpp:152-169); discovery cancel propagates rc (:2348-2350) |
| 8 | **CARVE stage** — `run_carve_stage` :2429-2592. Volume groups from table (or single-volume display name) :2461-2501; M-039 occupancy gate (sorted coalesced structured extents, slice-relative) :2514-2549; M-005 free-map gate (rebased copy) :2551-2566; `carve::carve_slice` :2576; counts; `carve_done_` on OK. Then rebase (carve nodes skip — GAP-S6-8 abs-extents guard) + checkpoint (:1424-1426). | whole slice bytes (carve window fan-out is the carve lane) | appends under Reconstructed root; dedup vs structured via occupancy + bitmap gates; dedup tree safety net | stage-entry :1416; carve engine loop heads (bridge) |
| 9 | Post-scan: `refresh_stats` :1433; FilterFolder junk flag pass (M-018) :1445 (cr_filterfolder.cpp:99-120); grading pass (S-023) :1452-1467; read-truth completion gate :1484-1511 (fully_blind / nothing_read → ERR_READ_FAILED) | whole tree | flags + grades only (no removal — junk nodes stay, flagged) | n/a (post-run, quiesced tree) |

Quick vs deep scan difference: ONE engine walk shared (quick+deep ⇒ deep semantics in the same pass, `covers_both`
feeds both stage telemetries :722-723); deep ADDS remnant sweep + lost discovery; `--quick-only` clears run_deep +
run_carve (rrecov.cpp:1459-1463). Deep is "always on" in the CLI (N-044).

---

## 2. Loop-bounds inventory (the no-stuck-or-freeze census)

Verdict legend: BOUNDED = explicit bound/guard/monotone-progress termination; UNBOUNDED = no termination argument;
BOUNDED-BY-ASSUMPTION = terminates only under an invariant this file set does not enforce (or an I/O/memory hazard).

### cr_scan.cpp
| file:line | structure walked | bound / guard | cycle protection | verdict |
|---|---|---|---|---|
| cr_scan.cpp:108-125 `collect_consistency_files` | candidate subtree (explicit stack) | stack drains; each node pushed once | tree is a DAG-by-construction (parent-only links) | BOUNDED |
| cr_scan.cpp:289-301 `add_carve_skeleton_` | 7 skeleton categories + root children | fixed 7; children = prior skeleton adds | idempotent role reuse | BOUNDED |
| cr_scan.cpp:338-358 / 976 resume-root scan | roots vector | roots size | n/a | BOUNDED |
| cr_scan.cpp:416-426 label/model/serial adoption copies | fixed arrays 100/48/32 | sizeof-bounded loops | n/a | BOUNDED |
| cr_scan.cpp:837-1058 `probe_family` | fixed sector reads | constant number of reads | n/a | BOUNDED |
| cr_scan.cpp:1102-1110 layout compare | disk_layout_ | partition count (table parse bounded, GPT ≤128/MBR+EBR chain; N-161 span gate drops invalid) | n/a | BOUNDED |
| cr_scan.cpp:1120-1125 candidate re-validation | stats.lost_candidates | persisted record count (see §7 caveats) | n/a | BOUNDED |
| cr_scan.cpp:1857-1879 LDM volume loop | ldm.layouts | LDM DB parse output (DB-size-bounded) | broken/copies-empty skipped | BOUNDED |
| cr_scan.cpp:1946-2022 table-target loop | targets (used, non-container) | partition count; per-target cancel poll :1947 | per-volume errors swallowed (candidate-mode) | BOUNDED |
| cr_scan.cpp:2161-2173 no-double-scan filter | candidates × table_target_lbas_ | both vectors small (≤10 inserted + table) | n/a | BOUNDED |
| cr_scan.cpp:2219-2276 candidate work build | dr.candidates | `kLostInsertCap = 10` hard break (:2220-2227, M-137) | cap | BOUNDED |
| cr_scan.cpp:2309-2315 `parallel_for` over work | ≤10 items | item counter; `pool.aborted()` checked per pull (cr_parallel.cpp:115) | abort + 50 ms watcher | BOUNDED |
| cr_scan.cpp:2317-2344 outs merge + serial path | work.size() ≤ 10 | fixed | n/a | BOUNDED |
| cr_scan.cpp:2467-2488 carve volume-group build | t.partitions | table parse | skip non-used/container | BOUNDED |
| cr_scan.cpp:2517-2527 occupancy collect (recursive λ) | whole result tree | node count | acyclic tree | BOUNDED (recursion depth = tree depth — see risk #6) |
| cr_scan.cpp:2531-2547 occupied coalesce | raw extents sorted | sort + linear merge | n/a | BOUNDED |
| cr_scan.cpp:2633-2662 label/card copies | fixed arrays | sizeof-bounded | n/a | BOUNDED |
| cr_scan.cpp:2667-2703 lost-candidate record emit | stats.lost_candidates | record count | n/a | BOUNDED |
| cr_scan.cpp:2781-2812 `read_summary` record loop | container count_ | count ≤ indexed offsets ≤ guard 65536 (cr_crsf.cpp:241) | peek-by-type skips heavy | BOUNDED |
| cr_scan.cpp:2834-2962 `load_session_summary` loop | container count_ | same 65536-index guard + torn-tail clamp (:273-274) | ERR_BAD_CHECKSUM skip-forward (F10) | BOUNDED |

### cr_filterfolder.cpp / cr_vollabel.cpp
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_filterfolder.cpp:39-47 `split_into` | pool bytes | pool_len exported length (N-081 ASan fix) | BOUNDED |
| cr_filterfolder.cpp:64-73 / 82-85 `eq_fold`/`in_set` | set entries | ≤ 25-entry linear fold | BOUNDED |
| cr_filterfolder.cpp:104-118 `apply` walk | whole tree (recursive λ) | node count | BOUNDED (depth assumption) |
| cr_vollabel.cpp:49-56, 73-78 label narrowing | fixed 33/11 | fixed | BOUNDED |
| cr_vollabel.cpp:97-104 exFAT label dentry scan | first root cluster | `off + 32 <= buf.size()`; 0x00 end-of-directory break | BOUNDED |
| cr_vollabel.cpp:131-140 NTFS $Volume attr walk | one MFT record | record size | BOUNDED |
| cr_vollabel.cpp:157-159 ext4 label | 16 bytes | fixed | BOUNDED |

### cr_grading.cpp
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_grading.cpp:60-64 extension table lookup | 169 static entries | binary search | BOUNDED |
| cr_grading.cpp:79-90 `slot_for` | devices | binary search | BOUNDED |
| cr_grading.cpp:411-436 N-205 pass-0 walk (stack) | tree nodes | node count; one 512 B read per matching node | BOUNDED |
| cr_grading.cpp:441-502 pass-1 solo+twin collect (stack) | tree nodes | node count | BOUNDED |
| cr_grading.cpp:511-516 pass-2 twin pair scan | twins vector | sorted linear | BOUNDED |
| cr_grading.cpp:526-531 deep-read validation | twins | one sector read per non-overlap twin | BOUNDED |
| cr_grading.cpp:537-553 deep-surface census (stack) | lost roots | node count | BOUNDED |
| cr_grading.cpp:559-593 gauge pass (stack) | tree nodes | node count | BOUNDED |
| cr_grading.cpp:636-652 `regrade_node` twin walk (stack) | tree | node count | BOUNDED |

### cr_snapshot.cpp / cr_diskmap.cpp
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_snapshot.cpp:33-62, 67-78 capture passes | nodes_ | arena size; whole copy under tree mutex | BOUNDED |
| cr_snapshot.cpp:100-109 `dfs` | snapshot indices | explicit stack; each node popped once | BOUNDED |
| cr_diskmap.cpp:26-28 `choose_cell_bytes` | cell size growth | `while total/cell > 64 MiB budget: cell *= 2` — ≤ ~54 doublings from 512 to span any u64 | BOUNDED |
| cr_diskmap.cpp:48-71 `mark_extents` | extents × cells | extent count × cell span | BOUNDED |
| cr_diskmap.cpp:119-125, 134-146 node/free passes | nodes / cell_count | budget-derived cells | BOUNDED |
| cr_diskmap.cpp:161-181 bad-range overlay | error ranges | `count <= bad_cap` fill + ERR_TRUNCATED signal | BOUNDED |
| cr_diskmap.cpp:215-220 `downsample_free_map` | cells | cell count | BOUNDED |

### cr_crsf.cpp (container)
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_crsf.cpp:144-158 Writer chunk framing | record payload | `remaining -= take` (take ≥ 1) | BOUNDED |
| cr_crsf.cpp:241-262 Reader::open record indexing | file bytes | **guard `guard++ < 65536`** + pos advances ≥ 512 (UNIT) or break; unfinalized-tail break :254-256; torn-tail clamp :273-274 | BOUNDED |
| cr_crsf.cpp:308-323 read_record inflate | framed chain | pos advances ≥ 3/iter; **64 MiB record cap** (P1-7 decompression-bomb guard :304, :317-320) | BOUNDED |

### cr_records.cpp (session codecs)
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_records.cpp:101-109 `serialize_tree` walk | whole tree (RECURSIVE λ) | node count | BOUNDED (depth assumption — risk #6) |
| cr_records.cpp:122-313 node emit loop | order.size() | measured count; N-060 per-node cancel (:123, :314) | BOUNDED |
| cr_records.cpp:361-629 `deserialize_tree` node loop | **wire u32 count** | c.fail break on first truncated node; per-node cancel :627 | **BOUNDED-BY-ASSUMPTION** — `by_id(count)` (:345) pre-allocates 8·count bytes with NO cap (the LDM codec caps its twin at 4096 :698 "the same bomb class"; the tree codec never got one). Hostile count → up to 32 GiB commit / bad_alloc escaping as an exception. Risk #2 |
| cr_records.cpp:633-639 deferred-parent resolve | deferred vector | ≤ count | BOUNDED |
| cr_records.cpp:661-755 LDM layouts codec | layouts/copies/members | count ≤ 4096 (:698), copies ≤ 64 (:719), members ≤ 4096 (:726) | BOUNDED |
| cr_records.cpp:784-805 free map | bits | bits_len must equal (cluster_count+7)/8 AND ≤ remaining (:796-799) | BOUNDED |
| cr_records.cpp:842-901 volume info | fixed fields | len-gated v1..v6 tails | BOUNDED |
| cr_records.cpp:921-938 session state | fixed | len-gated v1..v3 | BOUNDED |
| cr_records.cpp:969-1001 partition context | fixed | len-gated v1..v4 (77 B = N-203) | BOUNDED |
| cr_records.cpp:1005-1033 disk layout | entries | count ≤ 0xFFFF (:1007, :1023) | BOUNDED |
| cr_records.cpp:1038-1073 assistant stats | fixed | fixed | BOUNDED |

### cr_retry.cpp / cr_cached.cpp / cr_slice.cpp
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_retry.cpp:43-53 `try_read` | retry attempts | `attempt >= max_attempts (3)` + `attempt == 0xFFFFFFFF` overflow guard | BOUNDED |
| cr_retry.cpp:64-142 `read_isolated` (RECURSIVE bisection) | failing span | **depth budget kMaxBisectDepth = 24** (:18); stops when length ≤ sector or half < sector; backtrack = one shifted-window second chance (:83-99, N-004) | BOUNDED |
| cr_retry.cpp:101-129 bad-range record | leaf span | single map_.add under map_mutex_ | BOUNDED |
| cr_cached.cpp:38-95 `read_at` block loop | requested bytes | `remain -= take`, take ≥ 1 (in-bounds) | BOUNDED |
| cr_cached.cpp:46-51 block lookup | blocks_ vector | O(capacity) = 32 (8 MiB / 256 KiB) | BOUNDED |
| cr_cached.cpp:77-81 LRU move-to-front | ≤ capacity | ≤ 32 shifts | BOUNDED |
| cr_cached.cpp:93-94 eviction | ≤ capacity | pop_back | BOUNDED |
| cr_slice.cpp (no loops) | — | bounds-checked reads; straddle clamps + zero-fills tail (M-119) | BOUNDED |

### cr_candidates.cpp (lost discovery)
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_candidates.cpp:33-73 `WindowScanner` | 64 KiB window | single read per fetch; N-206 ceiled bound | BOUNDED |
| cr_candidates.cpp:78-508 format probes | fixed sector reads | constant reads each | BOUNDED |
| cr_candidates.cpp:670-680 `count_tree` | scratch probe tree | probe tree bounded by CONTENT_PROBE_BUDGET | BOUNDED |
| cr_candidates.cpp:686-709 `content_counts` budget | engine walk per candidate | **CONTENT_PROBE_BUDGET = 200 000 progress calls** (:688) — the registered safety valve (the og's own walk is UNBOUNDED; deliberate divergence, comment :682-687) | BOUNDED (deliberate divergence from og) |
| cr_candidates.cpp:863-984 discovery pass 1 | every sector of the disk | `base += 64 KiB` monotone to `total`; inner `off += stride*ls` monotone to win_len; per-window cancel :864 | BOUNDED by device size (linear; no iteration cap — terminates by construction) |
| cr_candidates.cpp:989-1011 pass-2 dedup | found × keep | O(n²) on candidate count (noise-heavy disk ⇒ slow but finite) | BOUNDED (quadratic — risk #5) |
| cr_candidates.cpp:1025-1049 pass-3 containment drop | keep × known | vectors | BOUNDED |
| cr_candidates.cpp:1055-1070 scoring loops | adjusted | n × content probe (I/O-heavy: n·≤200 k calls) | BOUNDED |
| cr_candidates.cpp:1075-1079 stable_sort | adjusted | O(n log n), stable (og merge-sort parity) | BOUNDED |
| cr_candidates.cpp:1095-1111 size-match promotion | adjusted | one promotion + break | BOUNDED |

### cr_consistency.cpp
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_consistency.cpp:122-174 `score_content_consistency` | files | count; inner registry scan (registry_size) | BOUNDED |
| cr_consistency.cpp:49-65 checkable_exts | registry | static-once | BOUNDED |

### cr_parallel.cpp (engine fan-out substrate)
| file:line | structure | bound | verdict |
|---|---|---|---|
| cr_parallel.cpp:113-135 worker job loop | item counter | `i >= count` break; `pool.aborted()` break per pull; exceptions never cross thread edge (:126-128) | BOUNDED |
| cr_parallel.cpp:152-169 `CancelPoolWatcher` | 50 ms poll thread | `submit_done_` flag + join (:175-179); aborts pool on cancel within one tick | BOUNDED |

### rrecov.cpp (CLI — scan/state-relevant loops)
| file:line | structure | bound | verdict |
|---|---|---|---|
| rrecov.cpp:937-975 print_tree walk | whole tree (recursive) | node count | BOUNDED (depth assumption) |
| rrecov.cpp:1126-1127 FindNextFile session enum | directory | OS enumeration | BOUNDED |
| rrecov.cpp:1327 / 1360 `--user-signs` file slurp | file bytes | fread-until-EOF — **NO byte cap** (the dict path caps at 256 KiB :583-592; this one does not) | **BOUNDED-BY-ASSUMPTION** — risk #3 |
| rrecov.cpp:1393-1543 argument loop | argc | argv length | BOUNDED |
| rrecov.cpp:1629-1645 LDM member enumeration | disks (≤32) | enumerate_disks(32) | BOUNDED |
| rrecov.cpp:2420-2451 recover plan recursion | whole tree | node count | BOUNDED (depth assumption) |
| rrecov.cpp:2895 bench span loop | total_bytes | done+block monotone | BOUNDED |

**Census verdict: 55+ loops inventoried → 50 BOUNDED (explicit guard/monotone termination), 0 UNBOUNDED (every loop
in these files terminates), 5 BOUNDED-BY-ASSUMPTION / hazard-flagged** (deserialize_tree count-allocation, --user-signs
uncapped slurp, 3 recursion-depth assumptions + the deliberate content-probe budget divergence). The engine-side
walks (MFT/FAT/B-tree) are another lane; the orchestrator's contract with them is the bridge + loop-head cancel polls.

---

## 3. State machine (text form), with transition cites

```
[Idle] --ctor(tree,cfg)-----------> [Constructed]          cr_scan.cpp:304-320 (roots pre-created)
        --ctor(tree,cfg,resumed)--> [Constructed-resumed]   cr_scan.cpp:322-376 + adopt_resume_state_ :387-437
                                     (quick_done_/pass_deep_done_/deep_extras_done_/carve_done_ restored from
                                      stage_done_mask; fs_hint adopted; identity card adopted;
                                      session_configured_mask_ preserved :436)

[Constructed] --run(slice,cancel,progress)--> [Running]
   tel_running_=true :1136; tel_cancel_=cancel :1137
   (pre-passes 0..5 of §1 run in order; repair/check skipped if token already cancelled :1150/:1217)

[Running] --engine pass OK--------> quick_done_=true :1523-1524
[Running] --any stage rc!=OK------> [stamp(rc)] -> [Done]   cr_scan.cpp:1351-1394 (stamp on EVERY exit:
                                                            pause-compensated elapsed :1355-1356, error-map
                                                            merge :1359-1360, boundary events :1365, snapshot
                                                            finalized + live pointers cleared under tel_.m
                                                            :1366-1380, cancelled flag = rc==ERR_CANCELLED :1375)
[Running] --cancel->checkpoint() at any loop head--> ERR_CANCELLED propagates -> [stamp(ERR_CANCELLED)]
   Checkpoints: :1150, :1217, :1393 (pre-quick), :1408 (pre-deep), :1416 (pre-carve), :1947 (per table target),
   engine loop heads (bridge contract :1806-1816 — user progress-false also cancels), discovery per-window
   cr_candidates.cpp:864-867, save/load per-node hooks cr_records.cpp:123/:314/:627, pool watcher 50 ms.
[Running] --PAUSE (PausableToken.pause)--> [Paused]         cr_device.h:176-242: checkpoint() BLOCKS on cv while
   paused; cancel() wakes waiters; paused_ms() accumulates for elapsed compensation (RE parity).
   Telemetry: tel_counts_/telemetry() read tok->paused()/paused_ms() :535/:749-756.
[Running] --quick done--> checkpoint emit :1404 (sink error ABORTS scan :779-835 — no silent gap)
[Running] --deep: late pass / remnant / discovery; deep_extras_done_ ONLY on rc==OK :2354-2355
   (a cancelled discovery never stamps deep done — G-07 resume integrity)
[Running] --carve done--> carve_done_=true :2583-2584; checkpoint :1425
[Running] --post-passes (junk :1445, grading :1466, read-truth gate :1484-1511)
   fully_blind || nothing_read --> [stamp(ERR_READ_FAILED)]
[Running] --OK--> [stamp(OK)] -> [Done]                    :1512

[Done] telemetry(): finalized snapshot returned early (running=false) :745-748; telemetry() callable after run().

Partial-result behavior on cancel:
  - the tree keeps every node attached so far; stage_done_mask stamps ACTUAL completion only :2604-2606;
  - CLI (rrecov.cpp): ERR_CANCELLED -> prints "cancelled at %.1f%%" :1919; saves session when --out given :1920-1925
    (exit 3 :1939); else S-057 stop-window auto-save into --autosave-dir when results found :1926-1937;
  - ERR_DEVICE_DISCONNECTED -> distinct exit 5 with resume hint :1946-1957;
  - resume(): completed stages masked off the config :3021-3026; controller continues where the session stopped;
    mask-integrity violation (done ⊄ configured) -> ERR_SESSION_FORMAT :2993-2995 (never silent partial resume);
    documented truth: pre-mask-truth sessions stamped the CONFIGURED mask as done ⇒ no-op resume (cr_scan.h:634-636).
```

Notable state-mismatch guards already landed: G-07 configured-mask survival on re-save (:428-436), stage-mask truth
(:2599-2606), BENCH-F1 torn-tail salvage (cr_crsf.cpp:265-274), F10 checksum skip-forward, A-004 live event sink.

---

## 4. Progress / ETA math (exact formulas + cites)

- **Weights**: `StageWeights{quick=40, deep=35, carve=25}` (cr_scan.h:72-76) — permille-of-total, monotonic weighted.
- **Overall**: `weighted_progress()` cr_scan.cpp:130-145:
  `overall = (w.quick*q_pm + w.deep*d_pm + w.carve*c_pm) / (w.quick+w.deep+w.carve)`, each pm clamped to 1000,
  result clamped to 1000; single-division form (no double truncation). total==0 → 0 (all-weights-zero pins at 0).
- **Stage permille**: `tel_publish_` :484-519: `pm = done*1000/total` (done≥total → 1000), clamped, then
  **monotonic within stage** (`if (pm < st.permille) pm = st.permille` :501-502); `units_done/total` recorded;
  bytes_units only stage 2 (carve). **Overall monotonic guard**: `if (overall > snap.overall_permille) ...`
  :515-518 (also :478-481 at stage end).
- **Stage end**: permille=1000, completed=true, pause-compensated stage elapsed `span - (paused_now - stage_pause_base)`
  :467-482.
- **Elapsed**: run elapsed `ms - paused_ms` :1351-1356; live telemetry same :749-756; stage elapsed :510-514.
- **ETA (M-126)**: `query::eta_compute(overall_permille, 1000, elapsed_ms, tel_.eta_prev_rate, false, 0)` in
  `telemetry()` :764-770 — the og's whole-run leg with EMA-smoothed rate carried BETWEEN snapshot reads
  (`tel_.eta_prev_rate`); -1 = no estimate. CLI renders `eta_format_m_s` + literal "3m00s" placeholder
  (rrecov.cpp:149-158).
- **Boundary ticks**: stages forward `tel_overall_()` (the post-stamp monotonic truth), never a hardcoded weighted()
  guess (comments :1528-1533, :2359-2361, :2588-2590).
- **Live counters (N-166)**: `refresh_live_counts_()` :566-574 reads tree atomics `live_files()/live_raw()`
  (cr_node.h:452-453); progress-event delta over the boundary snapshot is attributed to the ACTIVE stage's counter
  :606-617; deleted_found stays boundary-only (engines assign deleted state after attach).
- **Event coalescing**: `emit_progress_event_` :622-641 — fires only when overall permille advanced; CAS on
  `evt_last_overall_` rejects duplicates and stale out-of-order worker deliveries → ≤ 1000 progress events per scan.
- **bytes_found (N-167)**: stamped from `tree_.total_logical()` at every `tel_counts_` :521-528; completion headline
  "N file(s) (X GB)" uses 1024³ divisor (rrecov.cpp:2006-2010).

---

## 5. Session / persistence semantics (.crsf)

**Container** (cr_crsf.cpp/h): 64-B header (magic CRSFMT01, version 1, flags at +0x0C — bit0
`CRSF_FLAG_ABS_EXTENTS` F-1 basis, unit 512, 64-bit session id — hi-dword fix audit row 33 :113-119); 24-B record
headers (magic/type/session-id/chain/crc); payload framed in ≤ 4 096-B chunks (u16 hdr, bit15 deflate, bits0-11
len-1), deflate level 5 only when strictly smaller (SESS-4); per-record CRC32 over DECOMPRESSED data; finalize
patches record_count + per-record chain/crc via seek (:173-201). Reader: version check, index walk with
**65536-iteration guard**, 512-B skip-forward on broken header (F10), unfinalized-tail stop, torn-tail count clamp
(:217-278); `read_record` inflates chunk-by-chunk with raw-payload fallback and a **64 MiB record cap**
(decompression-bomb guard, :304-320); `peek_record` header-only (W1a light summary).

**What is saved, when** (`save_session` cr_scan.cpp:2594-2765), record order:
1. REC_SESSION_STATE — `stage_done_mask` from ACTUAL completion (quick_done_ | (pass_deep_done_&&deep_extras_done_)
   | carve_done_ :2604-2606) + counters (v2 deleted/carved, v3 bytes_found);
2. REC_ASSISTANT_STATS (S-023 grading counters);
3. REC_VOLUME_INFO — fs_kind (FsFamily wire truth), `flags = session_configured_mask_ | current cfg stage bits`
   :2627-2628, slice placement/geometry, label[32]+label_full[100] (N-090), identity card v2 (model/serial/boot_hash),
   v3 image path, v4 firmware/wwn/interface + 512-B boot snapshot (N-066), v5 save_unix, all append-only len-gated;
4. REC_PARTITION_CONTEXT per lost candidate — v2 geometry+flags (bit0 size-from-device-end, bit1 have_geometry),
   v3 disk_layout/disk_flags/volume_label[12] (M-137), **v4 data_start_lba/data_sector_count (N-203 — landed, do
   not re-file)** :2693-2698;
5. REC_DISK_LAYOUT (whole-disk table snapshot captured at run() start :1085-1098 — the og's partinfo semantics);
6. REC_FS_CHECK (M-041, conditional);
7. REC_RESULT_TREE — full node emit with v2 nflags bits (extents codec, resident data, short name, grade byte,
   M-051 RAW provenance, N-016b name_raw, N-059 per-FS secondary, OQ-SA-1 fat_origin_cluster / ntfs_origin_lcn,
   N-100 ntfs_total_clusters, S-059 logical+hole extents, S-052 root basis id); N-060 per-node progress+cancel;
8. REC_LDM_LAYOUTS (S-052, heap-owned dynamic-volume devices survive run());
9. REC_ERROR_MAP; 10. REC_ALLOC_MAP (S-009 downsampled free map, ≤ 8 MiB via the 64 MiB overlay budget).

**Checkpoint frequency (G-13)**: emit_checkpoint_ at each STAGE BOUNDARY (:1404, :1412, :1425) **plus one forced
checkpoint the moment lost discovery produces candidates** (:2198 — M-042 per-hit persistence parity). Sink =
cfg_.checkpoint IO triple + optional commit hook; file-backed sinks stage `<path>.tmp` + flush + rename (N-014
atomic); a failed write OR commit ABORTS the scan with the sink's error (:779-835) — no silent checkpoint gap.
Checkpoint container tag 'RKPP' 0x52434B50 :796; same record set as save_session; mask truth included so resume
skips finished stages by construction. CLI sink = CliCkSink (rrecov.cpp:1022-1058) — tmp staging, close-then-
MoveFileExW replace, fresh tmp per emit; the final path is never opened for truncation (A-013).

**Completion saves**: CLI saves to `--out`/`--resume` (in-place) via tmp+rename :1848-1874; auto-save
`--autosave-dir` on completion only per the core contract (cr_scan.h:316-326) — but see risk #4 (S-057 stop-window
auto-save on CANCEL also lands, rrecov.cpp:1926-1937). Auto-name `auto_<label>_<M>_<D>_<H>_<m>_<s>` with the
Windows name-forbidden sanitizer (cr_crsf.cpp:353-404).

**Load / resume round-trip** (`load_session_summary` :2818-3009; `resume` :3011-3040): F10 corruption semantics
(checksum-failed records skipped, load continues); heavy records decoded by type; PartitionContext restored into
`lost::Candidate` with fs_kind ≤ BitLocker gate :2888 and re-validation through the CURRENT disk when provided
(:2930-2936 — the og's at-search-start drop); mask-integrity check done⊆configured → ERR_SESSION_FORMAT :2993-2995;
pre-F-1 slice-relative extents normalized via vi.slice_offset when the flag is absent :3004-3006 (old whole-disk
sessions with candidate-relative nodes are a documented limitation); resume() clears completed stages
:3021-3026, adopts fs_hint, builds the controller (roots recovered by role, idempotent skeleton). VolumeSessionInfo
carries the full card incl. ldm_layouts (S-052). W1a `read_summary` (:2774-2816) decodes only the two light records
by peek — milliseconds even on 100 MB sessions.

---

## 6. Lost-partition scoring math (constants + cites)

Pipeline: collect (discover pass 1) → per-family structural probes → same-fs dedup (data-area containment) →
containment-in-known drop → content probe + weighted count + bonuses → ln-ratio score → stable sort → 16 MiB
size-match promotion → orchestrator-side no-double-scan filter + ≤10 insertion cap + naming + per-candidate engine
scan + exFAT content-consistency validation.

**Structural probe points** (cr_candidates.cpp):
- NTFS :78-130 — `boot-sig` +15, `bpb-valid` +25 (validate_boot_sector), `mft-probe` +30 ($MFT record 0 magic +
  fixup OK), `size-from-device-end` +1; unreadable/mismatch → −100.
- FAT :132-208 — `fat-bpb` +20 (validate_boot + og gate ladder `lost_boot_flavor` — flavor 0 ⇒ −100, N-034),
  flavor gate +5, `fat-probe` +10 (FAT entries 2..4 readable); label capture from +0x2B (M-137); data-area anchored
  at first_data_sector (N-035 :194-197).
- exFAT :210-229 — `exfat-bpb` 15 (+20 when volume_length ≥ 2048 && 0 < cluster_count ≤ 0xFFFFFFF5).
- BitLocker :256-291 — `boot-sig` +15, `fve-shape` +25 (printable version region +0x0B..+0x13, ≥3 version chars);
  device-tail size fallback (corpus-undecoded geometry).
- GPT header :293-307 — 10 (+20 when entry_size==128 && 0 < entry_count ≤ 1024); sector_count = 34.
- ext :347-405 — `ext-sb-magic` 15 + `ext-sb-valid` 25 + `ext-gdt-probe` 30; og gates: s_block_group_nr==0,
  s_first_ino ≥ 3, group-count cross-check (gb == gi).
- HFS+ :407-433 — `hfsplus-vh` 35 + `hfsplus-geometry` 5 (total_blocks > 0).
- Btrfs :435-450 — `btrfs-sb` 35 (device-tail fallback).
- ZFS :452-478 — `zfs-uberblock-magic` 25 + `zfs-txg-live` 10.
- ReFS :480-508 — `refs-msb-pages` 35 + `refs-checkpointed` 5 (8 MiB page-signature window).

**Dedup (pass 2, :987-1011)**: same-fs candidate whose start falls inside an earlier candidate's DATA-AREA extent
[data_start_lba, +data_sector_count) is dropped (outer/earlier wins); a `size_from_device_end` outer cannot claim
containment (GAP-S6-6).

**Containment-in-known (pass 3a, :1023-1049)**: data area fully inside a live used non-container partition ⇒ DROP
(AdjustPartitionInfoOrder rule (a)); same-start table match is exempt (takes +500 instead).

**Weighted count (:645-653)** — og FUN_14050fda0 :842176-842200 exact:
- og FS code 8 (HFS+): `files + 10*folders`
- og FS code 1/2 (FAT): `(files + folders) * 50`
- default (NTFS family): `files + 50*folders`

**Bonuses**: `fs_bonus` :781-794 — NTFS 60 (mft-probe present, evidence ≥ 3) else 30; FAT 20; others 0
(OQ-S2M-2 unpinned). **Table match**: +500 when start_lba equals a used table entry's start ("Found once
partition", :799-807, :1060-1062).

**Score (:655-665, :1069-1070)**: `score = int(score_ratio(count, max_count) * 1000)` where
`num = count > 1 ? ln(count) : 1.0` (the count==1 identity quirk), `den = max_count > 1 ? ln(max_count) : 1.0`,
`den <= 0 → 1.0`; max_count = best count THIS scan (:1054-1065). Structural points are the tie-break key only
(:981-982).

**Ordering**: `std::stable_sort` desc by score, tie by structural_points :1075-1079 (stability = og merge-sort
parity).

**~16 MiB size-match promotion (:1085-1112)**: `TOL = 16*1024*1024/512` sectors; the FIRST candidate whose
data_sector_count is within TOL of the leader's, with ≥ 1 non-matching candidate before it, copies the leader's
score and is spliced to position #2; one promotion per pass; adjacent matcher breaks instead.

**Orchestrator-side (:2124-2347)**: no-double-scan reconciliation drops candidates whose start equals an
already-scanned table target (2158-2173, S-051 — honestly-skipped targets are NOT recorded so discovery retries
them); forced checkpoint (:2198); insertion cap **kLostInsertCap = 10** (:2217, M-137/DRWUI:904229-234) — beyond
the cap candidates stay surfaced (apply-able) but never scanned; naming "<label> (<FS>)" / "Lost Partition
<n>(<FS>)" with per-scan counter / "Candidate @ LBA" fallback (:2241-2272); per-candidate scans in a pool with the
50 ms cancel watcher or serial; exFAT candidates content-consistency scored (M-035) — reject when
`filterable != 0 && (considered >> 1) <= filterable && ratio < 0.3` or a structurally impossible data start
(cr_consistency.cpp:138-181) — rejected candidate's subtree detached, counts zeroed, candidate stays listed.

---

## 7. DIVERGENCE-RISK notes (numbered; fragile / assumption-laden / state-mismatch-prone)

1. **No read deadline anywhere; cache mutex held across transport I/O** — cr_cached.cpp:31 `std::lock_guard
   guard(mtx_)` spans `inner_->read_at` :61. All engine worker threads serialize through this one lock; a hung
   transport read (driver stall, USB retry loop) stalls EVERY thread's cache hit with no timeout at any layer
   (cr_retry retries 3× then blocks again in the cache; transports do not poll the cancel token — F-B-16 scope
   note in cr_device.h:144-148). Termination is guaranteed only if reads eventually return. The og's device layer
   had the same synchronous shape, so this is fragility, not parity divergence — but it is the single largest
   perceived-freeze surface in the engine layer.
2. **`deserialize_tree` hostile node-count allocation** — cr_records.cpp:345 `std::vector<FileNode*> by_id(count,
   nullptr)` allocates 8·count bytes from an attacker-controlled u32 BEFORE any per-node bounds check; no cap
   (contrast the LDM codec's explicit 4096 cap at :698 which calls this "the same bomb class"). A hostile 0xFFFFFFF0
   count ⇒ ~32 GiB commit / bad_alloc escaping `deserialize_tree` as an uncaught exception (load paths have no
   try/catch). Loop itself terminates (c.fail break) — the ALLOCATION is the bomb.
3. **`--user-signs` unbounded file slurp** — rrecov.cpp:1327 and :1360 `while (fread...) json.insert(...)`: an
   arbitrarily large file is read fully into memory; the sibling `--bitlocker-dict` path caps at 256 KiB
   (:583-592). Memory-exhaustion hazard unique to the CLI leg.
4. **Doc-vs-behavior mismatch on cancel auto-save** — cr_scan.h:316-326 states the autosave trigger fires "on
   run() COMPLETION (rc == OK; a cancelled run never auto-saves — the og's flow is the completion window)", but
   rrecov.cpp:1926-1937 (S-057 landing) DOES auto-save on ERR_CANCELLED into `--autosave-dir` when no --out was
   given ("the og's stop window auto-saves"). The header comment is stale; an auditor diffing the og's
   stop-window behavior against the header will mis-score this. (The core itself never auto-saves — host-side —
   so the code is consistent; the CONTRACT TEXT is not.)
5. **Quadratic candidate dedup + I/O-heavy scoring** — cr_candidates.cpp:989-1011 O(candidates²) containment
   (noise-heavy disk ⇒ many false FAT BPBs); :1055-1066 runs one full engine content probe PER surviving candidate
   (each bounded at 200 000 progress calls but that is up to ~minutes of I/O per candidate, single-threaded, before
   the ≤10 insertion fan-out even starts). Bounded ≠ fast; a hostile image of repeated pseudo-boot sectors makes
   deep-discovery latency balloon with no wall-clock guard.
6. **Unbounded recursion depth on tree walks** — serialize_tree (cr_records.cpp:101-109), the occupancy collect
   (cr_scan.cpp:2517-2527), print_tree / recover-plan recursion (rrecov.cpp:935, :2420) recurse to tree depth.
   Node count is bounded but depth is engine-controlled; a pathological chain (e.g. a carve/engine bug emitting
   nested folders 10⁶ deep) is a stack-overflow class. The iterative (explicit-stack) walkers elsewhere in the
   codebase show the safe pattern; these five did not adopt it.
7. **Session-state truth dependency for legacy sessions** — documented but audit-relevant: sessions written before
   the mask-truth fix stamped the CONFIGURED mask as done ⇒ resume yields a no-op run that claims completion
   (cr_scan.h:634-636); pre-F-1 whole-disk sessions keep candidate-relative node extents (cr_scan.cpp:2996-3006
   limitation note). Both are honest-degradation paths, but a parity comparison against the og's resume behavior
   must know which build wrote the session file.
8. **Progress weight vs. actual work skew** — deep carries 35 % of the permille budget but, in whole-disk mode, its
   real cost is discovery (linear whole-disk sector sweep) + per-candidate engine scans; the deep permille advances
   only via the discovery progress bridge (`deep_cb`, cr_candidates.cpp:866) and the candidate scans' bridge ticks.
   With slow candidates the bar can sit near-frozen at the deep band while minutes of I/O elapse — monotonicity
   holds, ETA accuracy degrades (EMA lags). No stage-level wall-clock cap exists to bound the dwell.
9. **Checkpoint abort coupling** — a failed checkpoint sink write/commit ABORTS the whole scan with the sink's
   error (cr_scan.cpp:779-835, contract at cr_scan.h:328-337). Correct for "no silent gap", but it makes the scan's
   lifetime hostage to the session sink's health mid-run (a full disk writing .tmp kills an otherwise healthy scan
   at the next stage boundary). The og's checkpoint failure path is not decoded in the corpus — flag for the
   comparison lane rather than a verdict.
10. **Table-target loop geometry math** — cr_scan.cpp:1949 `PartitionSlice::create(slice, p.start_lba * ls,
    p.sector_count * ls)`: u64 product of table fields; the N-161 span gate (invalid GPT entries) drops the corrupt
    population at parse time and M-119 clamps overruns, so termination and bounds hold, but a gigantic-but-valid
    GPT entry yields a huge slice whose engine pass then runs unbounded-in-time (only cancel/progress bound it).
    Same class as #1: no wall-clock budget on any single engine invocation.

**M-135/M-136 live-state verification (not re-filed)**: M-136 — recovery hands the RAW device to SafeWriter
(rrecov.cpp:2375 `recovery::SafeWriter writer(io, src.device.get(), wcfg)`; scan's build_read_pipe at
cr_scan.cpp:163-202 is scan-only) — still LIVE. M-135 — zero-fill-and-continue + wholesale temp-delete posture in
cr_safewriter (per S55-L1 sheet; outside this lane's file set) — still LIVE. Error propagation to the caller
otherwise: engine errors in MAIN mode abort the scan (run_engine_dispatch :1691/:1797), candidate/table-volume/LDM
errors are swallowed (candidate-mode contract), hard non-IO transport errors propagate unchanged (cr_retry.cpp:71-72),
and the read-truth gate refuses fully-blind scans with ERR_READ_FAILED (cr_scan.cpp:1484-1511).

---

### Quick reference: cancellation checkpoint index (file:line)
cr_scan.cpp **:1150** (pre-repair gate), **:1217** (pre-check gate), **:1393** (pre-quick), **:1408** (pre-deep),
**:1416** (pre-carve), **:1947** (per table-target), engine loop heads via bridge **:1806-1816** (+ progress-false
path), discovery per-window **cr_candidates.cpp:864-867**, content-probe budget-false **cr_candidates.cpp:706-709**
(returns partial counts, not cancel), save/load per-node hooks **cr_records.cpp:118/:123/:314/:627**, pool watcher
**cr_parallel.cpp:152-169** (50 ms), PausableToken cv-wait **cr_device.h:219-223**.
