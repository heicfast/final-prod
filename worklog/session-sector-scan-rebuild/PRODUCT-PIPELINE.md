# PRODUCT-PIPELINE — sector-scan rebuild context sheet (Agent A, read-only survey)

Repo: `/home/z/my-project/repos/final-prod`, HEAD **976bced8** (branch tip 7920b976 = this session's T1 worklog commit). Every claim is `file:line` into THIS tree, verified by direct read this session. Companion sheets from prior sessions are summarized in §8; divergences vs today's tree are flagged inline.

---

## 1. SECTOR READ PATH (engine → disk)

### 1.1 The chain, end to end

```
engine (scan/FS walk/carve window read)
  → CachedReader (8 MiB, 32 shards, fill-outside-lock)     engine/src/core/storage/cr_cached.cpp:230
    → RetryingDevice (3 attempts, bisect to sector, Skip-mode zero-fill)  cr_retry.cpp:56-142
      → SliceShim (non-owning) → PartitionSlice (volume window on the parent disk)  cr_slice.cpp:16,58
        → IStorageDevice: DirectDevice (Windows) or image devices
          → AsyncRing (IOCP sliding window, 8 × 4 MiB) on the OVERLAPPED handle
          → companion SYNC handle via 64 KiB aligned envelope (every ring miss)
            → ReadFile / DeviceIoControl on \\.\PhysicalDriveN (GENERIC_READ, share R|W|D, FILE_FLAG_OVERLAPPED)
```

Single construction point: `build_read_pipe(slice, 8 MiB, 256 KiB)` at engine/src/core/orchestrator/cr_scan.cpp:191-230, called at cr_scan.cpp:1392. Degraded path is a pinned contract: cache-create failure → BARE SLICE (no retry isolation) — cr_scan.cpp:192-204, cr_scan.h:82-91.

### 1.2 The Windows transport (cr_transport.cpp, 1517 lines)

- **Data-path handle opens `FILE_FLAG_OVERLAPPED`** (cr_transport.cpp:890-892, `open_path`). The **AsyncRing** is built in the `DirectDevice` ctor (cr_transport.cpp:827-829): one IOCP (`CreateIoCompletionPort`, concurrency 1 — cr_transport.cpp:361), ONE completion thread (`completion_loop`, cr_transport.cpp:573-595), 8 slots × 4 MiB runs (defaults at cr_transport.cpp:341,352; `g_ring_run`/`g_ring_slots` tuning seam at :59-60, exposed via `ring_tuning` :1486-1493 and `open_file_async` :1496-1514). Slot buffers are `VirtualAlloc` page-aligned (cr_transport.cpp:367-369).
- **Read service algorithm** `AsyncRing::read` (cr_transport.cpp:406-541): per slice of ≤ `run_` — (1) DONE slot fully covering → serve; (2) INFLIGHT slot covering → predicate-wait on the slot's manual-reset event (lock-free wait, cr_transport.cpp:495-499); (3) **miss**: only a miss AT THE WALKING FRONT (`pos < cur+run && pos+take > cur`, cr_transport.cpp:447) recycles a victim slot (`pick_victim_locked` — IDLE → FAILED → LRU DONE, cr_transport.cpp:638-657); **every other miss (random/backward/8-slots-busy) goes to the COMPANION SYNC HANDLE** through `aligned_read_at` in ≤1 MiB slices with NO cursor advance (cr_transport.cpp:467-481). Serve copies under the slot lock with re-validation (cr_transport.cpp:507-532); refill-on-consume `kick_readahead` issues the next run after the cursor on a free slot (cr_transport.cpp:662-686).
- **Companion sync handle**: second open of the same path, `FILE_FLAG_OVERLAPPED` but never port-associated (cr_transport.cpp:823-826). Ring construction failure → degraded pre-ring envelope on `h_` (cr_transport.cpp:860-868).
- **Geometry**: `probe_geometry` (cr_transport.cpp:140-198) — `IOCTL_DISK_GET_LENGTH_INFO` for exact bytes (:117-138), `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` for logical sector, corrected by `StorageAccessAlignmentProperty` reading **BytesPerLogicalSector@+0x10 / BytesPerPhysicalSector@+0x14** with a [256,65536] sanity window (:170-175; the old +8/+12 cache-line misread is documented at :160-168). `sector_count` uses the og's CEIL form `(total+ss-1)/ss` — N-206, cr_transport.cpp:184-185,193.
- **Alignment**: the unified 64 KiB direct-read envelope `aligned_read_at` (cr_transport.cpp:244-268) rounds offset down / length up to 64 KiB multiples (GAP-S6-13 + sector-rejection constraints, cr_transport.cpp:200-243); already-aligned requests pass through with no bounce (:250-251); the bounce is per-thread and capped at 1 MiB+64 KiB (:255-260). Every ring-issued read is a 64 KiB-multiple at a 64 KiB-floor offset, tail-clamped to device size (cr_transport.cpp:450-459, and the comment at :303-305).
- **Handle self-heal**: `reopen_handle`/`reopen_handle_ov` (cr_transport.cpp:768-797); `DirectDevice::read_at` heals BOTH handles as a unit on `ERROR_INVALID_HANDLE` and rebinds the ring (cr_transport.cpp:848-857). `overlapped_read` classifies disconnect-family GLEs to `ERR_DEVICE_DISCONNECTED` (cr_transport.cpp:98-108).

### 1.3 Read sizes / queue depths / windows (the parameter table today)

| Surface | Size | Citation |
|---|---|---|
| Ring run (device read) | 4 MiB default (64 KiB multiple, tail-clamped) | cr_transport.cpp:341-351 |
| Ring window (QD) | 8 slots = 32 MiB in flight | cr_transport.cpp:352-357 |
| Ring miss (companion) | ≤ 1 MiB slices via 64 KiB envelope | cr_transport.cpp:473-474 |
| CachedReader block | 256 KiB | cr_scan.cpp:1392 |
| CachedReader budget | 8 MiB = 32 blocks, 32 shards | cr_scan.cpp:1392; cr_cached.cpp:59,163-184 |
| Carve window | 1 MiB + 4 KiB overlap | cr_carve.cpp:43-44 |
| Lost-discovery window | 64 KiB | cr_candidates.cpp:33-36 |
| Tail-scan chunk (raw-sign) | 1 MiB + sign overlap | cr_carve.cpp:1249 |
| Grading deep read | 1 sector per node | cr_grading.cpp:230-294 |

### 1.4 CachedReader (sharded, fill-outside-lock — the F1 rework)

32 shards keyed by block index (`(block_start/block_bytes) % 32`, cr_cached.cpp:244); phase 1 claims/finds under the SHARD lock with atomic state probe (FILLING=0/VALID=1/FAILED=2, cr_cached.cpp:133-134); hits memcpy under the shard lock with no refcount (cr_cached.cpp:262-276); the claimant fills via `inner_->read_at` with NO lock held (cr_cached.cpp:341-345); waiters park on the block's condition variable (cr_cached.cpp:330-340); failed blocks are never cached (audit row 38 — cr_cached.cpp:351-365). Buffers come from a page-aligned `BufferPool` (VirtualAlloc/mmap, freelist — cr_cached.cpp:75-123).

### 1.5 RetryingDevice + error map

`try_read` retries IO-class failures up to `policy_.max_attempts` (3, set at cr_scan.cpp:207) — cr_retry.cpp:42-54. On persistent failure: N-004 shifted-window CatchBuffer salvage (`[off-0x400, off+len)` re-read, cr_retry.cpp:83-99), then recursive bisection to single sectors (`kMaxBisectDepth` 24 — cr_retry.cpp:18,100-141); terminal bad ranges land in `ReadErrorMap` (`add` sorts + absorbs, module-attributed — cr_devmap.cpp:15-44; attribution model cr_retry.cpp:103-113). **Skip mode zero-fills and returns OK** (cr_retry.cpp:124-127) — every sector is *reachable* (a read never aborts the scan) but bad regions surface as zeros with the truth in the error map; the orchestrator's read-truth gate (§2) refuses fully-blind scans.

### 1.6 Reachability — is every sector addressable?

- PartitionSlice: start must be sector-aligned, length clamps to the parent end (M-119 tolerant create — cr_slice.cpp:37-46); straddling reads zero-fill the tail (cr_slice.cpp:72-79); `refresh_geometry` refuses invalidating media changes (cr_slice.cpp:85-102).
- Flat images pad the final partial sector to `sector_count*ss` (N-206 ceil — cr_image.cpp:170-190); VMDK/VHD/VHDX devices are extent/BAT-granular with parent chains (cr_image.cpp:448-530, 709-780, 999-1060).
- **Ring miss path**: a miss that is not at the front goes to the companion handle — data is served, so no gap; the documented hazard is only that the ring never ERR_NO_MEMORYs (comment at cr_transport.cpp:296-302). The carve counts unreadable windows as skipped, NOT scanned (cr_carve.cpp:1478-1482) and `bytes_scanned` attributes only the window's own range (cr_carve.cpp:1452-1455).
- **One real gap**: `probe_family` reads only LBA 0 plus a handful of magic offsets (cr_scan.cpp:968-1189) — recognition is signature-anchored, not coverage-driven; UDF/BitLocker are recognized with **no engine dispatch** (cr_scan.cpp:1109-1111 comment; FsFamily::Udf comment cr_scan.h:60-66 area).

---

## 2. SCAN ORCHESTRATION (cr_scan.cpp, 3300 lines — read in full)

### 2.1 Stage order in `ScanController::run` (cr_scan.cpp:1192-1682)

1. **Disk-truth pass** (M-042): capture live table into `disk_layout_` (:1216-1229); compare persisted layout → `stats.layout_changed` (:1230-1246); re-validate restored lost candidates on whole-disk resumes via `lost::revalidate_candidate` (:1247-1259).
2. Telemetry reset + coalescing state reset (:1261-1271).
3. **S-008 pre-scan repair** (consent-gated; outcome never aborts the scan; runs BEFORE the pipe so repair writes bypass the cache) — :1273-1341. Repair plan rebased to parent-absolute offsets before execute (A-002, :1320-1328).
4. **M-041 damage-audit check** (`check::run_fscheck`, read-only, informs never gates) — :1343-1365.
5. **Read pipe build** (:1392) + `probe_family` with blind-probe detection (error-range covering offset 0 → `stats.probe_unreadable`, :1406-1447). Volume facts + boot-sector CRC32 identity hash recorded from the raw slice (:1449-1483).
6. Cancel checkpoint (:1527-1528); `checkpoint_stats_` latch (:1529).
7. **Quick stage** (`run_quick` → `run_engine_pass`, :1530-1546): single shared engine pass with deep semantics when deep will run (cr_scan.cpp:1684-1704; `deep_coming` :1689). After: F-1 extent rebase (:1537), folder-filter prune (:1543), stage-boundary checkpoint (:1544).
8. **N-181 carve-before-deep**: when `quick_done_` (the standard config), carve runs BEFORE deep extras so carved files surface early (:1547-1581; `carve_before_deep` :1561, boundary lambda :1562-1578). When quick was disabled, the engine pass runs INSIDE `run_deep` and carve stays after (the F9 dedup needs engine nodes — :1557-1560).
9. **Deep stage** (`run_deep`, :1582-1593; body :2244-2546): (a) late engine pass if quick was off (:2254-2260); (b) **NTFS remnant sweep** `ntfs::scan_mft_remnants` with parallel fan-out (`threads` from `resolve_threads` — auto = min(hw-1, 8), :58-68) (:2279-2303); (c) **lost-partition discovery** `lost::discover` over the whole disk (:2305-2324), no-double-scan reconciliation vs `table_target_lbas_` (:2341-2356), forced checkpoint on first candidates (M-042 per-hit persistence, :2370-2382), candidate root naming (M-137 tiers, :2415-2458), **10-candidate scan cap** (og's insertion cap, :2400-1410), parallel `scan_candidate` fan-out over `ThreadPool` + `parallel_for` + `CancelPoolWatcher` (:2461-2529).
10. **Carve stage** (`run_carve_stage`, :2612-2789): latches `carve_bytes_base_` (:2620-2623); builds volume groups from the partition table or a single whole-slice group (:2652-2692); M-039 occupancy gate = aggregated structured extents (slice-relative, :2705-2741); M-005 free-map gate rebased slice-relative (:2748-2757); `carve::carve_slice` (:2767).
11. **Post-scan**: cache counters (:1600-1601); `tree_.refresh_stats()` (:1602); M-018 junk filter `filterfolder::apply` (:1614); S-023 `grade_tree` with a bounded reader = the run() slice (:1621-1636); **read-truth completion gate** — fully-blind (unreadable + slack ≥ volume, nothing found) or nothing-was-ever-attempted → `ERR_READ_FAILED` (:1637-1680); `stamp(OK)` (:1681).

### 2.2 Engine dispatch

`run_engine_dispatch` (cr_scan.cpp:1846-1968): one table for main pass + candidates (ARCH-DUP-4). Switch over FsFamily: `ntfs::walk_mft` (+ $Bitmap free map, S-009), `fat::scan_fat`, `exfat::scan_exfat`, `ext4::scan_ext4`, `hfsplus::scan_hfsplus`, `btrfs::scan_btrfs`, `zfs::scan_zfs`, `refs::scan_refs`; default = OK, nothing (unknown FS :1951-1953). `run_engine_pass` (cr_scan.cpp:1972-2242) first probes **LDM unconditionally** (:2006-2056), then — when the host did not force a family — the **whole-disk table-target flow**: every used non-container partition becomes a `PartitionSlice` target with its own display-name root, probed independently, skipped honestly when unrecognizable (:2096-2201). Single dispatch otherwise (:2202-2241). `scan_candidate` (cr_scan.cpp:2548-2610) = probe + candidate-mode dispatch + M-035 exFAT content-consistency rejection (detach subtree).

### 2.3 "Reading sector: X/Y" — the honest counters

- `tel_publish_` (cr_scan.cpp:517-582): byte-denominated publishes stamp `current_offset_bytes = done` (P-TICK-01, :545); carve adds `base + done` clamped-monotonic into `bytes_scanned` (D4a, :546-553); **deep assigns DIRECTLY (unclamped)** because each sub-pass re-covers the disk (P-DEEP-01 rationale, :554-565); quick reports 0/0 (metadata pass — :670-671).
- Coalescing (`emit_progress_event_`, cr_scan.cpp:697-728): fire when overall permille advanced OR bytes moved ≥ `kProgressBytesStep_` = **16 MiB** (cr_scan.h:904); ≤1000 events/scan plus stage flips; stale out-of-order worker delivery dropped (:721-722).
- Weights: quick 40 / deep 35 / carve 25 (cr_scan.h:72-76); `weighted_progress` exact single-division (cr_scan.cpp:131-146); **D-P6 `phase_progress`** — quick's own permille, then advanced restarts at 0 over the ACTIVE sub-stages only (cr_scan.cpp:148-173).
- Live counters: `refresh_live_counts_` samples the tree atomics on every event (N-166, cr_scan.cpp:633-645); ScanProgress events attribute the live-vs-snapshot delta to the active stage (:681-692).
- Cancellation checkpoints: run() head (:1527), before each stage (:1583, carve boundary :1563), engine loop heads via the bridge (`engine_progress_bridge_` :806-821 returns the user callback verdict), carve per-4 KiB (cr_carve.cpp:647-651) + 50 ms pool watchers (cr_scan.cpp:2491; cr_carve.cpp:1736-1743), discovery loop (cr_candidates.cpp:997-1000). A cancelled discovery must NOT stamp deep done (:2531-2538).

### 2.4 Completion

`stamp(rc)` (cr_scan.cpp:1485-1526): pause-compensated elapsed; error map always merged (save-on-cancel contract); boundary events once; finalizes telemetry under `tel_.m` and clears live pointers before destruction. cr_ui adds heartbeat join, checkpoint-sink teardown (leftover `.tmp` removed, cr_ui.cpp:2446-2460), cancel-path auto-save (S-057, cr_ui.cpp:2462-2482) and completion auto-save (W1c gates, cr_ui.cpp:2484-2495).

---

## 3. DETECTION (engine/src/core/carve/)

### 3.1 Grammar layer

- **154 registry rows** (`kTable`, cr_registry.cpp:117-1711; `registry_size` :1716; FMT_COUNT sentinel cr_signatures.h:458). Row shape `Signature{id, ext, header[16], header_len, header_offset, min_size, max_size, base_confidence, validator, name, suppress_overlap}`.
- **F8 gates**: `validate_registry` — no header-only rows, bounds, id-order (cr_registry.cpp:1720-1758); `self_probe_registry` proves every magic reachable through the same match primitive the scan uses (cr_registry.cpp:1760+).
- **Trial order** `kTrialOrder` (cr_carve.cpp:59-152): specificity-first (DOCX>ZIP, CR2>DNG>TIFF, RAR5>RAR4, HIK before MPEGPS); exhaustive vs FMT_COUNT (static_assert).
- **T13a first-byte dispatch bitsets** (`ctx.fb_mask[c]`, cr_carve.cpp:1603-1608, dispatch at :707-709) — the dominant CPU cost repair.
- **N-070 winner selection**: every magic-matching row validated; highest accumulated `base_confidence + validator confidence` wins; ties by trial order (cr_carve.cpp:672-789). M-117: the covered check runs BEFORE the validator (quadratic suppress_overlap blow-up removed, :729-747).
- **Gates**: M-005 used-cluster gate with cluster jump (:640-649); M-039 occupancy interval gate (:651-660); F9 extent dedup (len-class keyed, :766-781); M-024/M-158 typed-id disable (:749-764).
- **Emission** (cr_carve.cpp:830-1007): og naming `<PREFIX>_<%06d>.<UPPEREXT>` via `og_carve_name`/namer chain (S-049, :858-864; default_namer :1491-1527); disk-absolute extents + `NODE_FLAG_EXT_SHIFTED` (GAP-S6-8, :889-892); CRC32 header checksum (M-051, :870-876); sha128 for small files (:894-903); archive inner-entry child nodes (S-032, :955-996); `set_deleted_state(Deleted)` for every carve (S-031, :940-943).
- **Validators** live in cr_analysers.cpp (7,392 lines; e.g. `validate_jpeg` SOF+EXIF, `validate_pdf`).

### 3.2 Raw-sign layer (breadth)

139 formats / 199 header signs / 62 tail signs generated from the corpus (cr_rawsigns_data.cpp:14-16 area). `SignIndex::probe_header` buckets by (offset 0..46, first byte) (cr_rawsigns.cpp:34-49); probes ride at 512-byte strides after the grammar layer's claims (cr_carve.cpp:1008-1012); N-031 all-zero-stride fast skip (:1062-1073); built-in-first collision rule (S-055/M-048, :1075-1093); tail pairing = bounded forward walk in 1 MiB chunks with sign overlap, **capped at `kRawSignTailScanLimit` = 8 MiB** (cr_rawsigns.h:75; walk at cr_carve.cpp:1249-1320) — the DOCUMENTED DIVERGENCE vs the og's 200 MiB; confidence blend `(hdr+tail)/2+100` (:1304-1308).

### 3.3 User-sign layer

`usersigns::parse/install_set/snapshot` (cr_usersigns.cpp:207,337,350); lock-free snapshot probe per stride with zero-span fast skip (D4f, cr_carve.cpp:1103-1160); emits header-only records to device end, confidence 600, tag 0xFFFE0000 (:1161-1230); N-171: emission ticks BOTH `sign_hits` and `validated` so `carved_found` sees it (:1206-1219). No-reclaim: grammar-rejected starts are never reclaimed by weaker layers (S-055, cr_carve.cpp:631-639).

### 3.4 Carve windows + fan-out

`scan_window_index` (cr_carve.cpp:1444-1483): window = 1 MiB + 4 KiB overlap; unreadable windows skipped and NOT counted (FB-C01 honest fill-rate); `scan_one_window` per-byte walk with 4 KiB cancel cadence. `carve_slice` (cr_carve.cpp:1529-1774): OOM-hardened (N-139); single-threaded path or `ThreadPool` + `parallel_for` over window indices with `CancelPoolWatcher` (:1660-1746); per-worker `emitted_at` sets + the locked extent index as the cross-worker backstop (comment :1671-1673).

### 3.5 Lost-partition detection (cr_candidates.cpp, 1370 lines)

`lost::discover` (cr_candidates.cpp:942-1100+): 64 KiB `WindowScanner` over the whole disk; every stride boundary probed — pass 1a 55AA-gated boot families (NTFS/EXFAT/GPT/ReFS/BitLocker/FAT catch-all last, :1010-1058), pass 1b superblock families (ext/HFS+ in-window; btrfs/ZFS gated on 1 MiB grid alignment, :1060-1077); size from probes or device tail (N-206 ceiled, :1078-1097); pass 1r register channel (M-118, :1100+); content scoring `ln(count)/ln(max)` permille with stable sort (:1312-1327) and leader promotion (:1331-1352). `materialize` builds the candidate slice (:581).

---

## 4. RESULT TREE + CLASSIFICATION

### 4.1 ResultTree and the add_file chokepoint

`ResultTree::add_file` (cr_node.cpp:189-236): O(1) tail append under the tree mutex; N-166 live atomics `live_files_`/`live_raw_`/`live_logical_` (:220-228); `index_node_locked` builds `by_record_` and `by_extent_` dedup maps (cr_node.cpp:297-315). `sample_current_path_` (M13) samples every 128th attach into the 511-byte ticker cell OUTSIDE the lock (cr_node.cpp:238-286). Rebase machinery: `rebase_new_extents`/`rebase_subtree_extents` (cr_node.cpp:361-389), `detach_subtree` (cr_node.cpp:486-540, subtracts live counters).

### 4.2 Grading (cr_grading.cpp)

3-value Grade {Low, Average, High}. `grade_tree` (cr_grading.cpp:395-560+): N-205 forced tail-zero probe over grade-1 NTFS nodes (:396-427); pass 1 solo rules (`solo_rules` :330-389 — size<1→Low, Email→High, Active→High...); **S-058-corrected twin gate**: twin detection runs for extensions NOT in the 169-entry table (cr_grading.cpp:480-494 — the inverted-polarity repair; the table :26-49); pass 2 twin detection by extent-end key + category (:497-511); M-134 deep-read grade for non-twin not-in-table nodes only (`deep_read_grade` :230-294 — reads the extent's LAST sector (M-145), all-zero head→Low, 16-byte zero tail→High); recoverability gauge total<100 → matched 0 (:553-560 area). Undecoded og sub-gates deliberately not adopted (cr_grading.cpp:8-10).

### 4.3 Query + categories

`query::run` (cr_query.cpp:228+): single-pass filter walk, stable sort, offset/limit paging. Category enum: None/Image/Video/Audio/Document/Archive/Other/Email/Bookmarks + **Unsaved=9** (N-126, cr_query.h:34-57); `category_for_extension` = Unsaved overlay (.asd/.wbk) then binary search over the generated 1,481-extension / 8-category FileFilter table (cr_query.cpp:197-222; cr_filefilter_data.cpp:17-30). Display config: FilterFolder.ini SumaryFileType groups + PriorityDisplayType.xml 35-extension order (cr_config_display_data.cpp:33+). System-file handling: junk filter (below) + NTFS `$`-metafile/name filters inside the engines.

### 4.4 Folder noise filtering

- **filterfolder (M-018, post-walk)**: flags `NODE_FLAG_JUNK_FILTERED` by name pools (folders/files/FAT names, N-081 bounded split) + N-216 1 TiB default size cap; subtree kept (cr_filterfolder.cpp:126-152).
- **folderfilter (M-073, scan-time prune)**: prefix-normalized prune at stage boundaries; carve root + "File Path Lost" exempt; counters decremented to the pruned tree (cr_folderfilter.cpp:165-177; boundary application `apply_folder_filter_` cr_scan.cpp:882-908).
- Volume display names: `volume_target_name` tiers label → GPT name → FS-marker → LBA (cr_scan.cpp:1755-1813); carve grouping `carve_volume_display_name_` (cr_scan.cpp:250-308); "Reconstructed" root + 7 category skeleton folders (S-048, cr_scan.cpp:232-330).

### 4.5 Snapshot + disk map

`capture_snapshot` = full copy under the tree lock (G-02, cr_snapshot.cpp:17-85); disk map `choose_cell_bytes` pow-2 growth to an overlay budget (cr_diskmap.cpp:22-28).

---

## 5. PROGRESS + EVENT SURFACE

### 5.1 The 12 event types (engine/include/cr/cr_ui.h:151-224)

`CR_EVT_SCAN_STAGE(1), SCAN_PROGRESS(2), FILES_FOUND(3), ERROR_RANGE(4), CHECKPOINT(5), LOST_CANDIDATE(6), REPAIR(7), HEARTBEAT(8), SAVE_PROGRESS(9), FS_CHECK(10), SESSION_LOAD_PROGRESS(11), AUTOSAVED(12)`. The `cr_ui_event` POD (cr_ui.h:225-297) carries **aggregate counters only** — permille, bytes_done/total, files/deleted/carved counts, error-range deltas, checkpoint index, candidate LBA, repair outcome, check summary, autosave rc, bytes_found. **No event carries a result row.** (For the row-streaming rebuild this is the seam: the POD is append-only W1c discipline.)

### 5.2 Sink dispatch (cr_ui.cpp)

`scan_event_trampoline_` (cr_ui.cpp:1067-1109): core POD → `cr_ui_event` field-for-field, zeroed first; progress arrivals reset the heartbeat stall clock. Heartbeat watchdog: own thread, 5 s cadence, honest standing truth (cr_ui.cpp:884-906, 1143-1196; started in `scan_run_impl` :2428-2436 when events subscribed or memory monitor armed). Telemetry pull `cr_ui_scan_telemetry` → `scan_telemetry_impl` (cr_ui.cpp:2574-2613) now carries `current_path` (M13, :2600-2604), `current_offset_bytes` (P-TICK-01), `phase`/`phase_permille` (D-P6). Snapshot pull: `cr_ui_scan_snapshot` (cr_ui.cpp:3184). Completion auto-save gates + S-057 stop-window save (cr_ui.cpp:2462-2495; `autosave_if_due_` :2321).

### 5.3 Bridge (Rust)

- Sink: `scan_event_sink` — `try_send` a copy into a bounded `SyncSender`; full channel = drop, emitter coalesces (events.rs:53-60).
- Emitter: `spawn_scan_emitter` (events.rs:345+): per-event match; stage labels "quick-index"/"deep-walk"/"carve" (:359-366); overall clamped monotonic (:378); D4b size axis (:371-374); P-HB-01 heartbeat = refresh + emit standing truth (:447-460); CHECKPOINT consumed silently (:428); LOST_CANDIDATE emits `lost-candidate:{id}` with geometry joined from the log seam (:429-452).
- Payload: `compose_scan_progress` (events.rs:175-253) — camelCase contract shape incl. badSectorMap, etaSeconds, throughput, recoverableDuringScan, **currentPath (real sampled cell, null before first attach — events.rs:218-226)**, currentSector/currentSectorTotal from the byte cursor (:227-236), phase/phaseProgress (:238-252).
- Run thread: `launch_run_thread` (scan.rs:339-460): calls `cr_ui_scan_run`, latches terminal state from stats, force-refreshes the final snapshot, computes exact found-bytes from metas, emits `scan-finished:{id}`.
- Snapshot cache: `refresh_snapshot` — min 1500 ms cadence, frozen at terminal, generation-numbered (state.rs:628-677); `ensure_metas` NodeMeta cache per generation (state.rs:696+).
- **Result queries bypass the engine**: `query_page` filters the bridge's own NodeMeta vector ("the engine's cr_ui_query_run cannot express the contract's … treeMode semantics", results.rs:11-17; matches_pred :285; sort_matches :369).

### 5.4 TS consumption

`getScanStore` external store per scanId (scan.ts:32-90): on each progress event, tree/results/count invalidations at **10%-bucket cadence** (`bucket > lastBucket`, scan.ts:41-47) + status-change invalidations; `useScanState` polls 500 ms while running/starting/stopping (scan.ts:106-120, 'stopping' kept alive deliberately). `scan-session-store.ts` (zustand): active/known/celebration routing only. `scan-progress/scan-progress-page.tsx`: renders the D-P6 phase gauge (falls back to global curve, :96-98), sector numbers from the TARGET's sector size (never hard-coded 512, :143-145), the "Searching: path" ticker with the sector-cursor fallback (:193-210), `ResultsExplorer liveMode` embedded (:167-170), recover-during-scan gating on `recoverableDuringScan` (:346-348). `results-explorer` rides `useInfiniteQuery queryResults` (results.ts:40-55) re-fetched on the bucket invalidations.

**Streaming verdict (unchanged from prior sheets): batched snapshot-pull, not row streaming.** Progress/counter/telemetry axes are honest and live; result ROWS reach the UI only via `cr_ui_scan_snapshot` → NodeMeta → bridge-side filter, refreshed at max(10% bucket, 1.5 s snapshot cadence).

---

## 6. LOCAL DB / SESSION (.crsf)

- **Format**: CRSF container — magic `CRSFMT01` (8 bytes), version 1, flags (`CRSF_FLAG_ABS_EXTENTS` F-1) at +0x0C, record_count patched at +0x10 in `finalize`, unit 512, 64-byte header, 64-bit session id (hi-dword fix documented — cr_crsf.cpp:86-124; constants cr_crsf.h:30-35). Records framed in ≤4 KiB chunks, header `(len-1)|0x8000` when deflated, deflate-5 only when strictly smaller (cr_crsf.cpp:18-42); per-record CRC32 of decompressed data; corruption = skip-forward (F10 — cr_scan.cpp:3024-3026, 3068-3070).
- **12 record types** (cr_crsf.h:41-105): SESSION_STATE, VOLUME_INFO, PARTITION_CONTEXT, RESULT_TREE, ERROR_MAP, ASSISTANT_STATS, ALLOC_MAP, DISK_LAYOUT, FS_CHECK, LDM_LAYOUTS, CARVE_TALLY, SCAN_FILTER.
- **When persisted**: `save_session` (cr_scan.cpp:2791-2994) writes the full record set; the **result tree is serialized as ONE `REC_RESULT_TREE` record at save time** with per-node progress/cancel hooks (cr_records.cpp:89-123,350-351) — incremental persistence happens ONLY through **checkpoints**: `emit_checkpoint_` (cr_scan.cpp:910-966) re-runs the full save into an 'RKPP' container staged as `.tmp` + atomic rename (N-014), fired at every stage boundary (cr_scan.cpp:1544,1577,1591) and on first lost-candidate discovery (:2381). So: the tree is persisted at stage granularity (checkpoint), never row-incrementally; between checkpoints a crash loses everything since the last boundary.
- Load: `load_session_summary` (cr_scan.cpp:3047-3261) with mask-integrity check (:3245-3247) and F-1 basis normalization for pre-flag sessions (:3256-3258). `resume` masks completed stages (cr_scan.cpp:3263-3297). Last-scan pointer via cr_lastscan.cpp. No SQLite/WAL anywhere (grep-verified in prior sheets; unchanged).

---

## 7. PRIOR SHEETS vs TODAY (what they established, what's stale)

| Sheet | Established | Stale vs HEAD 976bced8 |
|---|---|---|
| **PRODUCT-PIPELINE.md** (2026-11-18 streaming session) | The full pipeline map; 12-event surface; snapshot-pull verdict; grading holes; bridge filter duplication | **STALE on the read path**: it cites the pre-ring transport ("sync QD1", cr_transport.cpp:291-293 old flags) — the AsyncRing + companion handle + sharded cache landed in the SAME-dated perf session AFTER that sheet. Also stale: currentPath "always null" (events.rs:218) — **now the real sampled cell** (cr_node.cpp:248, cr_ui.cpp:2600, events.rs:218-226). Its §7.1/§7.2 items are partially closed (see below). |
| **DIAGNOSIS.md** (streaming session) | "The drop stage": 0 of 12 events carry rows; UI starves between 10%-bucket pulls; fix order Phase 3 (row event) first | Still accurate on the core claim (verified this session: no row-carrying event exists, cr_ui.h:151-297). Its Layer-2/Layer-3 perf analysis (global cache mutex, QD1) is CLOSED by the landed F1/F2 rework (cr_cached.cpp header, cr_transport.cpp ring). |
| **PIPELINE-MATRIX.md** | Product-vs-og parity table; GAP on streaming; PARTIAL on tail-walk + grading | Grading S-058 is FIXED (gate now not-in-table, cr_grading.cpp:480-494) — the matrix's "S-058 inverted gate" row is STALE. Tail-walk 8 MiB vs 200 MiB still true. |
| **REFERENCE-SCAN.md** | og scan flow ground truth (quick→auto-advanced, same-page model, 10-run 4 MiB prefetch, per-phase gauge, status-bar grammar) | N/A (corpus side, not product); still the reference for the rebuild's behavior targets. |
| **REFERENCE-ALGORITHMS.md** | og RAW engine: dual registries (~70 4CC + 39 u16 keys), two-generation validators, raw_signs.json 139/199/62, BruteForce last-valid-EOF (200 MiB), .rsf format | N/A (corpus side). Product deltas it flagged remain: tail-walk bound, multimap collision chains. |
| **BEHAVIOR-FINDINGS.md** (session 2026-11-18 streaming section) | Precise scope of the incomplete: row event entry points enumerated (cr_ui.h:151-297, cr_ui.cpp sink, cr_node.cpp:188 chokepoint, abi.rs re-pin, events.rs:353 match arm, contract.ts/tauri-connector onScanResult, scan.ts subscribe) | Entry points verified this session and CONFIRMED current (all still open — none of the Phase 2/3 items landed). Its "grading coverage holes" item is STALE (S-058 fixed; deep-read coverage is by-design og-shaped). |

Also note for the rebuild: the perf session's in-flight benchmark verification and lane B/C worklogs exist under worklog/lane-b, lane-c; RELEASE-VALIDATION/ and VALIDATION/ are empty-by-record (BEHAVIOR-FINDINGS.md:200-203).

---

## 8. SUSPECTED BREAK POINTS (top 10, ranked)

1. **No row-carrying event in the ABI** — `cr_ui_event` (cr_ui.h:225-297) and the 12 `CR_EVT_*` types (cr_ui.h:151-224) carry counters only; results cross to the UI exclusively via snapshot pull + bridge-side NodeMeta filter (results.rs:11-17) refreshed at 10% buckets (scan.ts:41-47) and 1.5 s snapshot cadence (state.rs:628-641). THE streaming gap — the reported "sector reading produces no results" shape whenever a scan lives inside one bucket or a long carve phase.
2. **Result-tree persistence granularity** — the tree persists only at stage-boundary checkpoints (cr_scan.cpp:1544,1577,1591 + first-candidate :2381); between boundaries a crash/cancel-kill loses all rows since the last checkpoint; there is no row-incremental persistence (cr_records.cpp:89 serialize_tree is whole-tree).
3. **Bridge re-implements result filtering** — `query_page`/`matches_pred`/`sort_matches` (results.rs:285,369,434) duplicate engine `query::run` semantics (cr_query.cpp:228) with the documented drift risk (results.rs:11-17); the engine-side query is tested but UNUSED in the shipped result list.
4. **Tail-walk bound 8 MiB vs og 200 MiB** — `kRawSignTailScanLimit` (cr_rawsigns.h:75; walk cr_carve.cpp:1249-1320): tail-paired raw-sign formats fall back to header-only emission past 8 MiB → undersized carved files on media-heavy volumes (registered divergence S-015, but the og-parity target for this rebuild).
5. **Ring front-miss victim contention** — `AsyncRing::read` step 3 (cr_transport.cpp:445-465): only walking-front misses recycle slots; with 8 slots in flight and a sequential consumer plus concurrent stragglers, a front miss with no recyclable slot falls to the ≤1 MiB companion path (:467-481) — correct but throughput-dropping; the refill only issues ONE run per consume (`kick_readahead` returns after one claim, cr_carve... cr_transport.cpp:684) so the window may run dry under slow consumers.
6. **Carve window overlap re-scan + per-window allocation** — each window re-reads 4 KiB overlap and allocates a fresh ~1 MiB buffer per window per worker (cr_carve.cpp:1462-1472); no window buffer pool exists (only the cache block pool, cr_cached.cpp:75) — the Layer-4 churn the perf diagnosis measured, still open at the carve layer.
7. **Deep-stage byte counter double-coverage presentation** — `tel_publish_` assigns deep bytes DIRECTLY (unclamped, per sub-pass — cr_scan.cpp:554-565): the "Reading sector: X/Y" counter RESTARTS at each deep sub-pass (sweep → discovery → candidates); honest per the comment, but the overall permille (weights 40/35/25) can sit at 75% while the sector axis reads low — the exact "jumps to 75%" complaint shape if the phase axes are not consumed (UI does consume them now, scan-progress-page.tsx:96-98, but any surface reading only overallProgress reproduces it).
8. **Grading coverage is og-shaped but narrow** — deep-read grade runs only for non-twin, NOT-in-table-extension nodes with index==0 (cr_grading.cpp:516-527, 230-294): in-table extensions (most real files) and every multi-disk/FAT16-shifted node get NO content validation; combined with solo rules only, large populations keep emit-time grades.
9. **10-candidate lost-scan cap** — `kLostInsertCap = 10` (cr_scan.cpp:2400-2409): only the 10 best-scored lost partitions are scanned; the rest stay surfaced-but-unscanned. Og parity, but on heavily-partitioned damaged disks this silently caps recovery.
10. **probe_family single-sector anchoring + UDF/BitLocker dead ends** — recognition reads LBA 0 + a few offsets (cr_scan.cpp:968-1189); UDF and BitLocker return families with NO engine dispatch (cr_scan.cpp:1109-1111; FsFamily comment block) — those volumes yield zero structured rows and rely entirely on carve; a damaged boot sector on an otherwise-scannable volume degrades the whole structured pass to carve-only unless deep discovery's candidates rescue it.

---

*Survey method: full read of cr_scan.cpp (3300 lines) and cr_transport.cpp (1517 lines); targeted full/partial reads of every other cited file; prior sheets re-read and cross-checked against HEAD 976bced8. No files were modified except this sheet.*
