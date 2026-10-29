# S55-D2 — SCAN STRATEGY + SECTOR/LBA MATH DEEP-DIVE (read-only context sheet)

Task ID: S55-D2. Auditor: sub-agent (read-only). Core tree: `/home/z/my-project/audit/core_recovery`
@ `1fccb0a4`. Corpus: `/home/z/my-project/audit/easeus-drw-research` @ `be0cfe9`.
Scope: (1) og raw/deep scan sector-stepping decode, (2) per-engine scan-range + walk-bound
matrix, (3) sector-number arithmetic audit (fresh eyes). Every claim cites file:line read
first-hand this session. Abbreviations: `RAW/` = `re-workspace/reversed/ghidra-searchdll/
RAWSearchDLL_decompiled.c`, `NTFS/` = same dir NTFSSearchDLL, `exFAT/` = exFATSearchDLL,
`FAT/` = FATSearchDLL, `BtrFS/` = BtrfsSearchDLL, `DRWUI/` = `re-workspace/reversed/
ghidra-drwui/DRWUI_decompiled.c`, `Device/` = `re-workspace/reversed/ghidra-mo/
Device_decompiled.c`. Core paths relative to `src/`.

Known-covered context (not re-filed): N-181 (raw-first start ordering), N-182 (deep-range
derivation), M-127 (FAT/exFAT cycle-guard O(n²)), M-126 (ETA EMA), N-162 (FAT 32-bit wrap),
D-72 (device-layer 4 MiB prefetch vs 8 MiB/256 KiB LRU), OQ-S4-3 (transport-relative perf),
M-005/M-039/N-031 (carve gates), D-04 (byte-anchored geometry), S-059 (hole semantics),
LA-1..LA-8 (s12 lane-A sector-math sheet).

---

## (a) THE OG RAW/DEEP SCAN LOOP — SECTOR STEPPING DECODE (first-hand)

### A.1 The engine-side read-block derivation — CRAWSearchController::SetScanRange-ish `FUN_18000c430` (RAW:9245-9329)

- Controller ctor `FUN_18000bba0` (RAW:8859-8894): sector-size field default
  `*(undefined4 *)(param_1 + 0x1e) = 0x200` (RAW:8881 — byte offset 0xF0 = **512 default**).
- `FUN_18000c430(param_1, sector_size, flag, …)` validates the sector size is a positive
  multiple of 0x200 (RAW:9265-9267, `>> 0x3f & 0x1ff` alignment check) and stores it at
  controller +0xF0 (RAW:9262).
- **Read block**: `*(int *)(param_1 + 0xc8) = 0x400000 / sector_size` (RAW:9271-9273, the
  same constant at RAW:9284-9286 when re-init) — **4 MiB read block** (8,192 sectors @ 512 B).
- **Double buffer**: `*(int *)(param_1 + 0xd8) = readBlockSectors * sectorSize * 2`
  (RAW:9274-9275; the init-path twin at RAW:9205-9206) — **8 MiB buffer**, allocated as
  `bufferByteSize + 0x1000` (RAW:9293-9296; +4 KiB guard; the init twin RAW:9207-9209).
- **Per-buffer results accumulator**: a separate 0x10048 allocation = 0x1000 (4,096) × 0x10
  elements + iterator tail, built with `_eh_vector_constructor_iterator_`
  (RAW:9298-9314) — a 4,096-entry `CRawFileInfo*` list with `std::_Xlength_error("list<T>
  too long")` growth checks (RAW:9597-9609).

### A.2 The per-chunk block scanner — `FUN_18000c820` (RAW:9385-9633) — the probe loop

Structure per call `scan_block(controller, startSector, sectorCount, filter)`:

1. **Read into the SECOND half**: `device->vtable[1](device, buffer + (bufferBytes >> 1),
   startSector, sectorCount)` (RAW:9434-9438 — the read lands at `buffer + 4 MiB`).
2. Cancel check via a global flag object (RAW:9439-9441).
3. **Probe loop over `sectorCount` sectors** (RAW:9443-9618):
   - Optional skip-filter ("filter_generalraw", RAW:9076): when the filter's flag byte at
     +0x1D is clear, granularity `uVar9 = *(uint *)(filter + 24)` (RAW:9452), bit test
     `FUN_180058df0(filter, sector / uVar9)` (RAW:9453; the bitmap test itself at
     RAW:67842-67856 — base-rebased bit array, OOB returns 0 → dispatch proceeds); when the
     bit is set AND the position is granularity-aligned, `local_1e0 += uVar9 - 1`
     (RAW:9457-9458) — **the bitmap marks regions to SKIP (cluster jump)**.
   - **Device-total bound**: breaks when the current sector ≥ device total sectors
     (RAW:9481; total fetched via device vtable slots 3/4, CellTest +0x38, RAW:9463-9478).
   - **Probe address = `buffer + 4 MiB + idx * sectorSize`** (RAW:9496-9497) — probes are
     **SECTOR-ALIGNED**; the analyser dispatch `FUN_18000cf60` (RAW:9533) is keyed on the
     first u32 at the sector start (RAW:9499-9501 — zero u32 → flag 4).
   - **Zero-sector shortcut**: trailing-zero quadword scan backwards from the sector end
     (RAW:9503-9510) passed to the dispatch (all-zero sectors short-circuit).
   - **Look-back context**: the analyser's buffer base sector = `startSector − 8192`
     (RAW:9511-9526: `lVar11 = lVar14 − ((bufferBytes >> 1) / sectorSize)`) — the FIRST
     4 MiB of the buffer holds the PREVIOUS chunk, so every probe carries **4 MiB of
     look-back context**.
   - **Probe stride = 1 sector**: `local_1e0 = local_1e0 + 1` (RAW:9616), loop while
     `local_1e0 < sectorCount` (RAW:9618).
4. **Slide**: `memmove(buffer, buffer + 4 MiB, 4 MiB)` (RAW:9620-9622) — the analyzed half
   becomes the next call's look-back half.
5. **Progress**: `progress->vtable[8](progress, sectorSize * startSector, buffer, 4 MiB)`
   (RAW:9623-9627) — position reported as sectorSize × startSector (64-bit product).

### A.3 The chunk driver (CellTest export mirror) — `FUN_18000c650` (RAW:9337-9380)

`EUCreateRAWSearcherForCellTest` binds a synthetic device and iterates the whole range:
`totalSectors = end − start + 1` (RAW:9348); `chunkCount = ceil(total / readBlockSectors)`
(RAW:9361-9364); per chunk: `start = readBlockSensors × i + rangeStart` (RAW:9368),
`count = readBlockSectors`, **clamped at the inclusive end**: `if (end < (rbs−1)+start)
count = end − start + 1` (RAW:9370-9372); then virtual slot 0x98 (19) = the block scanner
(RAW:9373). The shipping host drives the same per-chunk contract from
`CGenericRecoveryController` (corpus `artifacts/DRWUI-exe--main-ui-architecture.md` §9.2;
vendor QA log below).

### A.4 Host-side range semantics (vendor QA log, og-codebase/log/DRWUI.log, ascii strings)

- `22:57:57:975 Quick scan StartSector=705794048, TotalSector=270977023`
- `22:57:57:987 Deep scan StartSector=705794048, TotalSector=270977023` (same range)
- `23:57:53:831 Quick scan StartSector=206848, TotalSector=434608127`
- `23:57:53:844 Deep scan StartSector=0, TotalSector=434814975` — **deep = whole disk**
  (206848 + 434608127 = 434814975 = disk end; quick = the volume's range)
- `23:58:16:330/351 Insert/Found lost partition … StartSector=2048, TotalSector=204799`
  (genericrecoverycontroller.cpp:130/:737)

→ The deep range derivation (whole-disk basis) is the N-182 lane's territory; the stepping
INSIDE the range is what this sheet decodes.

### A.5 The og's layered read sizes (three distinct layers — do not conflate)

| Layer | Size | Evidence |
|---|---|---|
| ENGINE carve read block (RAWSearchDLL) | **4 MiB** (0x400000) in an 8 MiB double buffer | RAW:9271-9275, :9434-9438, :9620-9622 |
| DEVICE layer, physical disk | **4 MiB** (0x400000/sectorSize → +0x10) | Device:56545 |
| DEVICE layer, volume (CVolumeDevice) | **7 MiB** (0x700000/sectorSize → +0x18) | Device:56912, :56928 (the "og 4/7 MiB" of OQ-S4-3) |
| Recovery read chunk (DRWUI CFileRecoverWorker) | **1 MiB** (0x100000), 10 MiB variant (0xa00000) | DRWUI:875616-875625 |

---

## (b) CORE COMPARISON — per parameter (cr_carve.cpp / cr_scan.cpp)

Core: `WINDOW = 1ull << 20` (1 MiB), `OVERLAP = 4096` (cr_carve.cpp:45-46); window scan
`scan_window_index` (cr_carve.cpp:1191-1232): `base = wi × WINDOW`, `want = min(avail,
WINDOW + OVERLAP)` (:1195), read `slice->read_at(base, win, u32(want))` (:1222), honest
byte accounting `counted = min(want, WINDOW)` (:1203, :1230); windows tile `[0, total)`
exactly (comment :1198-1202).

| Parameter | og | Core | Verdict |
|---|---|---|---|
| Carve read block | 4 MiB engine-side (RAW:9271-9273) on 8 MiB double buffer (RAW:9274-9275) | 1 MiB window + 4 KiB overlap (cr_carve.cpp:45-46, :1195) | **MISMATCH (value)** — core's 1 MiB+4 KiB registered as constants (SCAN-ENGINE-DELTA F6); the og ENGINE-side 4 MiB/8 MiB decode is NEW (prior lanes registered only the DEVICE-side 4 MiB as D-72/lane-B 2.2). Transport-relative parity tracked OQ-S4-3 |
| Probe stride (grammar/keyed layer) | **1 sector (512 B)** — probes only at sector boundaries (RAW:9616, :9496); registry keyed on first u32 at sector start (RAW:9499) | **1 byte** — `for (u64 off = 0; off + 12 <= want; ++off)` (cr_carve.cpp:526) with first-byte dispatch (:577-578) | **MISMATCH (superset)** — core probes every byte; the og never probes mid-sector. Lane-A 3.1 noted "core byte-granular" as MATCH-equivalent; never registered as a detection-surface delta |
| Probe stride (raw-sign layer) | 1 sector (same loop) | `off += 512` (cr_carve.cpp:846) — explicitly modeled on the og's stride (comment :840-842) | **MATCH** |
| First probe position | chunk start sector, `local_1e0 = 0` (RAW:9442) → range StartSector itself | window offset 0 (cr_carve.cpp:526, :846) → range start | **MATCH** (both offset 0; og sector-aligned, core byte 0) |
| Look-back context for analysers | 4 MiB previous chunk in-buffer (RAW:9511-9526, slide :9620) | 4 KiB overlap only; validators re-read beyond via WindowDevice→slice (M-025 comment cr_carve.cpp:522-525) | **MISMATCH (mechanism)** — og in-buffer depth 4 MiB; core 4 KiB window + slice re-reads (functionally unbounded via device reads; no analyser in the core requires >4 KiB look-back) |
| End-of-range handling | last chunk clamped to inclusive end (RAW:9370-9372); probe loop breaks at device total (RAW:9481) | `want = min(avail, WINDOW+OVERLAP)` (cr_carve.cpp:1195); loop bounds `off + 12/512 <= want` (:526, :846) | **MATCH** (clamp-to-end both sides) |
| Zero-region skip | all-zero sector shortcut: first-u32 flag + trailing-zero scan (RAW:9499-9510) | zero-stride skip N-031 (cr_carve.cpp:959-978, `zero_strides_skipped`) | **MATCH** (N-031 landed on this decode) |
| Bitmap skip gate | filter bitmap marks skip regions; aligned → jump granularity (RAW:9451-9459; FUN_180058df0 RAW:67842-67856) | M-005 free_map gate + cluster jump (cr_carve.cpp:257-333, :532-541) | **MATCH** (M-005 cites RAW:9440-9466 first-hand) |
| Occupancy gate | per-volume bitmap fed by every FS engine (M-039's corpus basis) | coalesced structured-extent interval set (cr_scan.cpp:2286-2307; cr_carve.cpp:547-553) | **MATCH** (M-039) |
| Double buffering | 8 MiB buffer, read→analyze→slide (RAW:9434-9438, :9620-9622) | fresh vector per window (cr_carve.cpp:1210-1214); parallel mode fans window-index space (header comment cr_carve.cpp:10-13) | **MISMATCH (architecture)** — core single-buffer + parallel fan-out vs og serial slide; perf-posture difference, results-neutral |
| Deep range basis | whole disk from sector 0 (DRWUI.log 23:57:53 rows) | `run_carve_stage` whole-slice basis (cr_scan.cpp:2213+; volume grouping :2245-2284) | **COVERED** by N-182 (derivation lane) |

---

## (c) PER-ENGINE WALK-BOUND MATRIX (og protection vs core protection)

| Engine | og bound / guard (cite) | Core bound / guard (cite) | Verdict |
|---|---|---|---|
| **NTFS MFT walk** | batch loop over `record_count` per 64 KiB batch (NTFS:21095-21146; `uVar9 = record_size × param_3` :21144); responsive cancel per record (:21148-21150); outer walk bounded by $MFT $DATA size (corpus ntfs-engine-internals.md §"MFT acquisition": 64 KB batches; fragmented-MFT path FUN_18001a540 builds extent map first) | `for (u64 record_no = 0; record_no < max_record; ++record_no)` (cr_mftwalk.cpp:441; max_record from $MFT size, cr_bootstrap.h:61); cancel + progress every 64 records (:442-445); `read_record` OOB → skip (:448-450) | **MATCH** — both size-bounded, cancel-checked. No cycle hazard (linear record sweep) |
| **NTFS runlist decode** | byte-bounded only: `while(true)` with `param_3 <= pos → return 1` (NTFS:41873-41877, :41888); header 0 terminator (:41880); `len_width > 8 → break` (:41891); `off_width > 8 → return` (:41992-41994); NO run-count cap, NO cluster-total cap; runs inserted into hash container (:41943-41963); pos strictly advances ≥2/run (:41960) | `max_clusters` accumulated cap (cr_runlist.cpp:87-89); `RUNLIST_MAX_RUNS = 0x100000` → ERR_CYCLE_LIMIT (:90-91; cr_runlist.h:64); strict rejects len_width 0/>8, zero-length, truncation (:77-86) | **CORE STRICTER** (caps the og lacks); og's tolerance divergence = LA-5 (filed). Neither can loop unbounded (og: attribute bytes; core: caps) |
| **NTFS $ATTRIBUTE_LIST chase** | detector FUN_1800419e0 (NTFS-Engine v2 artifact §4); chase mechanics "mftwith0x20" path — no decoded cap in corpus | breadth-first to `FRAG_DEPTH_LIMIT` with a `visited` set (cr_mftwalk.cpp:504-528, depth :517, visited check :528) | **CORE ADDS** cycle guard + depth cap the og's decode shows no equivalent for (hardening direction; D-14 family posture) |
| **FAT chain walk (directory materializer)** | EOC bound `while (uVar7 < 0xffffff8)` (FAT:11231); cancel via vtable 0x88 (:11236-11238); device-size bound (:11240-11243); capacity guard (:11245-11246); **visited-range RB-tree** at controller+0x10 — descent (:11250-11263), repeat-hit → destroy/stop (:11271-11275) | `follow_chain`: max_len guard (cr_chain.cpp:438-440), O(n²) visited vector (:442-447), EOC (:454-455), bounds (:456-458), per-step cancel (:433-437); deleted twin limit `expect ?: 0x10000` (:478) | **COVERED M-127** (complexity); guard existence parity, cap value registered (Q3 row) |
| **exFAT FAT-table access** | 1 MiB window cache in an RB-tree keyed by FAT byte offset (exFAT:28062-28080, :28089); window read via device (:28119-28122); entry `(cluster & 0x3ffff) × 4` (:28148, :28158); FREE entry → `next = cluster+1` under an untrusted flag (:28160-28163) | S-013 windowed mode, `win + (cluster & 0x3ffff) * 4` 1 MiB windows (cr_exfat_quick.cpp:26-69; comment cites og :28039-28213); FREE→contiguous heuristic EXFAT-2 (:111-127, cites og :28158-28163) | **MATCH** (windowed twin, S-013/EXFAT-2 landed) |
| **exFAT chain walk (CExFatDataRunList builder)** | EOC-family break `0xfffffff5 < cluster` (exFAT:44990); **size bound** — remaining bytes ≤ 0 → return (:44991-44992, decrement :45096-45097); valid start `uVar12 − 2 < 0xfffffff4` (:44980); **cluster-count guard** — next ≥ cluster_count(+0xc0) → forced EOC (:45037-45039, :45047-45050); loop exit `1 < cluster` (:45098); **NO visited set / range tree** (size-bound is the terminator) | `follow_exfat_chain`: iteration cap (cr_exfat_quick.cpp:98-100), O(n²) visited (:102-106), cluster-count guard (:121-124), EOC (:128-129); call sites pass **size-derived `expect` for files** (:740-742), 0x10000 for directories (:257, :447, :515) | **MATCH for files** (both size-bounded); directory chains: core 0x10000 cap vs og size-bound — cap value registered (M-127/Q3); the og's no-visited-set posture (size-bound only) is a decode refinement: M-127's "og range tree" applies to the FAT engine, not the exFAT chain builder |
| **Btrfs superblock probe** | 0-terminated mirror-offset table loop (BtrFS:10289-10316); nodesize sanity power-of-2 + range (:10299-10302); best-generation selection (:10303-10307); device-size gate `0x10fff < size` (:10283) | mirror offsets + nodesize gates (cr_btrfs_engine.cpp — module verdicts; M-020 fixed) | **MATCH** |
| **Btrfs leaf/internal walk** | nritems-bounded loops with **per-item buffer-bound prechecks** `bufSize < items×0x19+0x65 → abort` (BtrFS:10417, :10414-10509; internal twin 0x21 stride :10514-10547); 10 MiB scratch accumulator with `0x9fffff` bound + reset (BtrFS:10450-10458) | `MAX_ITEMS = 4096` per node (cr_btrfs_engine.cpp:130, :361, :578…); `MAX_NODES = 1<<20` + `std::set<u64> visited` logical-address cycle guard (:129, :389-395); `DIR_DEPTH_LIMIT 64` (:131, :1049) | **CORE STRICTER** — og nritems/buffer-bounded but **no visited set**; a bytenr cycle in the og's B-tree search recursion (BtrFS:2710-2802, self-recursion at :2784, no level-compare/visited) would recurse until stack exhaustion (crash-class, not hang-class); core's visited set terminates it |
| **ext4 iteration** | (not decoded this session; corpus ext4-hfs-engine-algorithms-v2.md §math: first_data_block×log_block_size rules; reads strided `(block+param_2)*geometry`) | `EXTENT_DEPTH_LIMIT 5` (cr_ext4_engine.cpp:81, :335); `MAX_GROUPS 1<<20` (:83, :162); `MAX_INODES 16<<20` sweep bound (:86); `MAX_META_DESC_READS 4096` (:50, :291); `BACKUP_SB_MAX_GRID 4 TiB` (:74, :602-608) | **CORE GUARDED** (og side undecoded at loop level — noted as open decode, no contradiction found) |
| **HFS+ B-tree walk** | self-recursive descent exists (HFSPlusSearchDLL FUN_180013de0 :15504, recursion at :15587 — body not decoded this session) | `MAX_TREE_NODES 1<<22` guard (:98, :402, :1311, :1380); journal-ring cycle comment (:644 — "a corrupt ring can cycle without ever re-hitting jhdr.start"); node-size gates (:159) | **CORE GUARDED**; og loop-level decode open (no contradiction found) |
| **ZFS / ReFS / LDM span** | ZFS: DVA `<<9`/`×0x200` math + 4 MiB vdev skip (lane-A 2.14 cites); ReFS container map FUN_180011e90 (lane-A 2.15); LDM span/RAID walk (lane-A area 4 — all MATCH/COVERED) | ZFS `dva_asize/dva_offset <<9` (cr_zfs_engine.cpp:143-147); ReFS identity/container map (cr_refs_engine.cpp:92-121); LDM spanned/RAID5 with member re-anchoring (cr_ldm.cpp:1234-1263, :1390-1409; S-052) | **COVERED** (lane-A rows 2.13-2.16, area 4; M-046/S-052/D-59/D-79-83) |

**No-stuck-or-freeze verdict:** no engine pair shows an og-bounded walk that the core walks
unbounded. Every core walker carries an explicit cap/guard; where the two differ the core is
the stricter side (NTFS caps, Btrfs visited set, exFAT iteration cap) with one registered
complexity regression (M-127, O(n²) vs the og's FAT range tree).

---

## (d) SECTOR-NUMBER ARITHMETIC AUDIT TABLE (fresh-eyes pass)

| # | Surface | og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1 | FAT cluster→sector width | `(ulonglong)((uVar7−2)*(int)param_1[4]) + param_1[8]` — u32 multiply then widen (FAT:11239; first-hand this session; also FAT:13194/:28172/:39068-39080 per lane-A 2.1) | `(u64(c)−2)*cluster_bytes + first_data_sector×bps` full u64 (cr_chain.cpp:24-28) | **COVERED N-162** (og wraps >2 TiB; core correct) — site re-verified first-hand |
| 2 | exFAT cluster→sector | 64-bit `(u64)(c−2)×spc + base` (exFAT:44947-44948, :45027-45028 — `& 0xffffffff` cluster mask then u64 multiply; lane-A 2.10 cites :12999-13001 family) | `(u64(c)−2)*cluster_bytes + cluster_heap_offset×bps` u64 (cr_exfat_quick.cpp:146-147) | **MATCH** (both 64-bit) |
| 3 | exFAT FAT window indexing | `(cluster & 0x3ffff) * 4` — 256K-entry window mask (exFAT:28148, :28158); window base `(cluster×4)/sectorSize & mask + FAT_base` (:28062-28065) | `off = u64(c)*4` then 1 MiB-aligned window select (cr_exfat_quick.cpp:54-64) | **MATCH** (S-013; both handle >256K-cluster tables via windows; core additionally OOB-returns FREE :57-58) |
| 4 | NTFS LCN accumulation | u64 accumulator + signed deltas, sign-extension cases (NTFS:41937-41939, :41962; lane-A 2.7) | u64 wrap-add, INT64_MIN rejected (cr_runlist.cpp:100-105) | **MATCH** (core hardening on INT64_MIN) |
| 5 | NTFS runlist→sector (recovery read) | `LCN × SPC + StartSector + runIdx` u64, device-end clamp (DRWUI:875663, clamp :875665-875668 via vtable 0x18 GetTotalSectors) | slice-relative u64 byte extents; `extents[ext_i].offset + in_ext` (cr_safewriter.cpp:498) | **MATCH** (u64 both sides) |
| 6 | Recovery destination write position | sequential-append writer (dest offset implicit in cumulative writes; write call DRWUI:875751-875752); remaining-size clamp `param_7 & 0xffffffff` on bounded values only (chunk ≤ 1/10 MiB, :875699-875706) | explicit-offset `io_.write(item_id, done, …)` with u64 `done` cursor (cr_safewriter.cpp:388, :428, :482, :504); extent locate u64 (:419-422, :446) | **MATCH** (both u64; no 32-bit truncation on either side — og's `(int)` casts apply only to chunk sizes bounded ≤10 MiB) |
| 7 | Recovery hole/sparse run | buffer memset 0 (DRWUI:875661) + write OUTSIDE the `LCN != −1` gate → zero chunks written, full logical size produced | sparse runs dropped at decode (cr_runlist.cpp:190-191 `extents()` skip), no per-extent logical offset → ERR_TRUNCATED refusal (per S-059 trail) | **COVERED S-059** (filed SEVERE — not re-filed) |
| 8 | Recovery read chunk | 1 MiB default / 10 MiB flag variant (DRWUI:875616-875625: 0x10000a / 0xa0000a allocs) | `read_chunk = 1 << 20` (cr_safewriter.h:114; use :399, :415-417) | **MATCH** (1 MiB default parity) |
| 9 | og volume-device read length bound | byte length high-dword ≠ 0 → refuse (Device:57004-57006); 64-bit SetFilePointer two-part (:57010-57011); 7 MiB chunk loop (:57018-57028); return `bytes/ss & 0xffffffff` (:57031) | u32 read lengths (`u32(want)` cr_carve.cpp:1222; read_at ABI) | **MATCH-in-effect** (both u32-bounded read lengths; no overflow on either side) |
| 10 | Disk total sectors | cascade: GET_DRIVE_GEOMETRY 0x70000 (Device:56523) → GET_PARTITION_INFO 0x74004 `PartitionLength/ss` (:56526-56531) → **GET_LENGTH_INFO 0x7405C authoritative: `(L−1+ss)/ss` = ceil(L/ss)** (:56532-56538); sector default 512 (:56535-56536); NO Cylinders×Tracks×Sectors multiply anywhere | GET_LENGTH_INFO exact first (cr_transport.cpp:86-94), GEOMETRY_EX + StorageAccessAlignment 4Kn (:102-132), `sector_count = total_bytes / logical_sector` = **floor** (:133-134; volume fallback :135-141) | **MISMATCH (rounding)** — ceil vs floor differ only when device length is not a sector multiple (truncated images); both u64-clean at >2 TiB and >4G-sector boundaries; core's 4Kn logical detection is the registered D-04 superset |
| 11 | Device chunk sectors | disk 4 MiB: `(int)(0x400000/ss)` (Device:56545); volume 7 MiB: `(int)(0x700000/ss)` (Device:56912, :56928) | 64 KiB unified envelope + 256 KiB cache blocks (D-72/GAP-S6-13; cr_scan.cpp:1032 build_read_pipe) | **COVERED D-72 / OQ-S4-3** (value decode: the 4-vs-7 MiB split is first-hand here; lane-B 2.2 had the 4 MiB, OQ-S4-3 the "4/7 MiB" phrase) |
| 12 | Carve progress position | `sectorSize × startSector` u64 (RAW:9623-9627) | bytes permille (cr_scan.cpp:129-141 weighted_progress) | **COVERED D-36** (progress model) |
| 13 | Carve offset→absolute | analyser batch-relative; absolute = batchByteOffset/stride + batchBase, stride = vtable+0x28 sector size (SCAN-ENGINE-DELTA "Consequence 2" decode; lane-A 3.1 MATCH) | `cand_abs = base + cand_rel` bytes + ext_base (cr_carve.cpp:582; lane-A 3.1) | **MATCH** (lane-A 3.1) |

---

## (e) DIVERGENCE CANDIDATES

**DC-1 (NEW, MINOR) — og engine-side carve read-block/look-back decode vs core window.**
- What differs: og carves in 4 MiB engine-side read blocks inside an 8 MiB double buffer
  with a 4 MiB look-back half (RAW:9271-9275, :9434-9438, :9511-9526, slide :9620-9622);
  core carves in 1 MiB windows + 4 KiB overlap (cr_carve.cpp:45-46, :1195) with slice
  re-reads beyond the window.
- Evidence: og RAW:9271-9275 (`0x400000/sector_size`, ×2 buffer), RAW:9616/:9620-9622;
  core cr_carve.cpp:45-46, :1191-1231.
- Already covered? The core's constants are registered (SCAN-ENGINE-DELTA F6 "1 MiB window
  + 4 KiB overlap … registered constants") and the DEVICE-layer 4 MiB is D-72/lane-B 2.2 —
  but the ENGINE-layer 4 MiB/8 MiB/double-buffer/slide decode is NEW (prior corpus rows
  attributed 4 MiB only to the Device layer; RAWSearchDLL's own 0x400000 computation was
  never cited). Proposed severity: **MINOR** (decode-registration item; no core defect —
  WindowDevice re-reads make validators functionally unbounded; perf posture owned by
  OQ-S4-3).

**DC-2 (NEW, MINOR) — probe granularity: og probes sector-aligned only; core grammar layer
probes every byte.**
- What differs: og dispatches analysers only at sector boundaries (RAW:9616 stride 1 sector;
  probe address RAW:9496; first-u32 key RAW:9499); the core's grammar layer steps every byte
  (cr_carve.cpp:526 `++off`) — 512× the probe positions, finding headers the og cannot
  (non-sector-aligned starts), at dispatch cost (mitigated by the first-byte table
  cr_carve.cpp:577-578).
- Evidence: og RAW:9496-9499, :9616; core cr_carve.cpp:526, :577-578; the raw-sign layer
  matches the og at 512 (cr_carve.cpp:846, comment :840-842).
- Already covered? Lane-A row 3.1 called this "MATCH (equivalent; core byte-granular)" —
  the detection-surface consequence (superset results on damaged volumes) was never
  registered. Proposed severity: **MINOR** (results-set delta, core superset; also a
  scan-cost factor that belongs in the OQ-S4-3 relative-perf ledger).

**DC-3 (NEW, INFO/NEGLIGIBLE) — disk sector-count rounding: og ceil vs core floor.**
- What differs: og GET_LENGTH_INFO path computes `(L−1+ss)/ss` = ceil(L/ss)
  (Device:56532-56538); core computes `total_bytes / logical_sector` = floor
  (cr_transport.cpp:134, :140).
- Evidence: Device:56538 vs cr_transport.cpp:134.
- Impact: differs only when the device length is not a whole number of sectors (truncated
  image files); the og would report/scan the partial trailing sector, the core drops it.
  Both u64-clean at >2 TiB / >4 G sectors. Proposed severity: **INFO** (register as a
  deliberate-rounding note or align to ceil; no real-disk impact).

**DC-4 (decode addendum, no new ID needed) — the og's geometry cascade + 4/7 MiB split.**
- First-hand: GET_DRIVE_GEOMETRY (0x70000, Device:56523) supplies BytesPerSector
  (stored +0x38, :56540); GET_PARTITION_INFO (0x74004) PartitionLength/ss as intermediate
  total (:56526-56531); GET_LENGTH_INFO (0x7405C) overrides with ceil (:56532-56538);
  sector default 512 only inside the LENGTH branch (:56535-56536). CVolumeDevice chunk =
  0x700000/ss = 7 MiB (Device:56912/:56928) vs physical-disk 4 MiB (:56545).
- Covered by: D-04 (geometry/4Kn superset), D-72 + lane-B 2.2 (device chunking), OQ-S4-3
  ("4/7 MiB"). The 0x74004/0x7405C cascade order and the ceil form are new first-hand
  detail supporting DC-3.

**DC-5 (verification, COVERED) — exFAT chain-walk bound semantics.**
- First-hand decode: the og's exFAT chain builder has NO visited tree — it terminates via
  EOC-family break (exFAT:44990), the file-size bound (:44991, :45096-45098), the
  cluster-count guard forcing EOC (:45037-45039, :45047-45050), and valid-start ≥2
  (:44980). The core's file chains use the same size-derived bound
  (cr_exfat_quick.cpp:740-742 `expect`) plus the O(n²) visited vector (M-127) and the
  0x10000 dir cap (registered cap value, Q3 row).
- Not a new divergence; refines M-127's corpus citation (the og range tree is the FAT
  engine's directory walker, FAT:11250-11275 — re-verified first-hand; the exFAT twin
  relies on the size bound instead).

**DC-6 (verification, COVERED) — NTFS runlist walk bounds.**
- The og's runlist decoder is attribute-byte-bounded with no run-count/cluster-total cap
  and tolerant truncation (NTFS:41873-41894, :41960); the core adds `max_clusters` +
  `RUNLIST_MAX_RUNS` (cr_runlist.cpp:87-91) and rejects where the og tolerates (LA-5,
  filed). No cycle hazard exists on either side (flat run arrays); the og's hash-container
  insert loop (NTFS:41943-41963) advances pos ≥2 per run — bounded.

**DC-7 (verification, INFO) — Btrfs/HFS+ og-side cycle-guard absence (core superset).**
- The og's B-tree search recurses on child bytenrs with no visited set and no parent/child
  level comparison (BtrFS:2710-2802, recursion :2784) — a bytenr cycle between internal
  nodes would recurse to stack exhaustion (crash-class). The og's leaf walk is nritems- and
  buffer-bounded with per-item prechecks (BtrFS:10417, :10483) and a 10 MiB scratch cap
  (:10450-10458). The core's visited-set + MAX_NODES + MAX_ITEMS + DIR_DEPTH_LIMIT
  (cr_btrfs_engine.cpp:129-132, :389-395) is strictly hardening. Direction: core stricter —
  consistent with the D-14/F7 hardening family; no action.

**DC-8 (verification, COVERED) — recovery-side extent arithmetic.**
- og: u64 sector math with device-end clamp (DRWUI:875663-875672), sequential-append
  destination, 1 MiB/10 MiB chunk buffers (DRWUI:875616-875625), hole zero-fill
  (DRWUI:875661 + write path :875751); all `(int)/(uint)` casts land on values bounded by
  the chunk size (≤10 MiB) — no 32-bit truncation hazard.
- core: u64 cursors throughout (cr_safewriter.cpp:403-408, :419-422, :446, :498, :504),
  1 MiB read chunk (cr_safewriter.h:114). Arithmetic parity; the hole-semantics delta is
  S-059 (filed, not re-filed).

---

## Session ledger (what was read)

RAWSearchDLL: 8859-9008, 9009-9348, 9349-9478, 9479-9808, 67842-67901. NTFSSearchDLL:
21080-21179, 41853-41977. exFATSearchDLL: 28018-28234, 44920-45199. FATSearchDLL:
11180-11299. BtrfsSearchDLL: 10269-10598, 2710-2829. DRWUI: 875590-875779. Device:
56500-56599, 56855-57074. Core: cr_carve.cpp:44-163, 255-379, 516-640, 836-900,
1191-1320; cr_runlist.cpp (full); cr_mftwalk.cpp:425-528; cr_chain.cpp:418-497;
cr_exfat_quick.cpp:20-134, 715-774; cr_safewriter.cpp:36-53, 381-510; cr_scan.cpp:2213-2307;
cr_transport.cpp:80-154; cr_btrfs_engine.cpp / cr_ext4_engine.cpp / cr_hfsplus_engine.cpp
(guard greps). Documents: corpus architecture/05-scan-engine-architecture.md (full),
artifacts rawsearch-carving--deep-dive.md, rawsearchdll--family-rawspeed.md,
fat-exfat-engine-algorithms-v2.md, ntfs-engine-algorithms-v2--deleted-viability.md,
ntfs-engine-internals.md (§MFT acquisition), btrfs-engine--deep-dive.md,
DRWUI-exe--main-ui-architecture.md (§9); core diff-to-fix SCAN-ENGINE-DELTA.md (rows
A1-A6, C9, F1-F7, session-5 log), s12 lane-A sheet (full), lane-B sheet (rows 2.2/2.5/5.4),
s12-core-deepread (rows), M-127 finding (full), worklog N-162 entry. Logs: DRWUI.log
(scan-range strings), CalcPartition.log path check.
