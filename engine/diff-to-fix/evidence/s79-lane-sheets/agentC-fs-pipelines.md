# Agent C — FS Engines Function-Level Pipeline Census (FAT / exFAT / ext4 / HFS+ / Btrfs)

Read-only parity audit, og ground truth vs `core_recovery`. Every og cite is
`<file> FUN_xxx :line` in `re-workspace/reversed/ghidra-searchdll/`. Core cites are
`src/core/...` or `include/cr/...`. Mount/validation items marked **KNOWN COVERED**
are cross-referenced only, not re-filed: FAT media-word flavor (N-201), exFAT base−12
re-base (S-066), exFAT hardening bounds (N-198), ext4 SB bands (N-202), ext4 unwritten
ee_len (I-045), HFS+ freeBlocks/journalInfoBlock gates (M-142), ext4 volume caps
(M-143/M-144), ext4 journal-inode harvest (M-143 active).

---

## 1. FATSearchDLL (FAT12/16/32) — 64,658 lines

### 1.1 Exports & host contract
| Export | Body | Notes |
|---|---|---|
| `EUCreateFATSearcher` | :22-32 | `new(0x280)` → FUN_180005780 (CFATSearchController, complete scan) |
| `EUCreateFATQuickSearcher` | :37-71 | `new(0x1C0)` → CFATQuickSearch; 0x200 staging buffer via `FUN_180027ed0(puVar1+0x14, 0x200)` (:57); two string slots inited from `DAT_18005299a` (:64/:68) |

Quick scan is threaded per-worker (CFATQuickSearch::CFATQuickSearchWorker, vftable ctor
region :26931+; 13+ ctor params incl. thread control +0x38, `param_3 == 0 ? 512 : …`
sector defaulting). Complete scan adds deleted-chain reconstruction + CDoubleFATTable
mirror fallback + orphan re-linking.

### 1.2 Pipeline map (stage → function)
1. **Boot/open validation** (flavor strings :9860-9866/:27904-27910/:34341-34349;
   `_strnicmp(boot+0x36,"FAT16   ")` :38048, `"FAT32   "` :38062) — KNOWN COVERED (N-201 family; core `cr_chain.cpp:623-628` string-first flavor, `boot_passes_open_gates` :639-684).
2. **FAT-table factory** `FUN_180036e90` :44687-44793:
   - param_2 = flavor (0/1/2) → CFAT12Table (:44715)/CFAT16Table (:44742)/CFAT32Table (:44772);
   - cluster-count seed `param_4 << 7` then `sector_size*fat_sectors >> 2` (FAT12/FAT32) or `>> 1` (FAT16) (:44714/:44741/:44771);
   - allocation gates: FAT12 spf∈[1,12] (`uVar3-1 < 0xc` :44716), FAT16 spf∈[12,0x100] (:44743), FAT32 spf∈[0x100,0x96000] (:44775);
   - FAT32: `DAT_1800728d4 = cluster_count * 0.98` (:44782) — the 98% watermark bound for root-chain checks;
   - post-construct validity check `vt[0x10]()`; failure → delete, return NULL (:44784-44790).
3. **Whole-table validation** (mount-grade chain walk over the loaded table):
   - `CFAT16Table::validate` `FUN_180041b00` :54032-54120: FAT[0..1] u32 == 0xFFFFFFF8 (`*piVar2 == -8` :54052); sweep entries 2..count rejecting `≥count && <0xFFF7 || ==1 || self` (:54060); claims/back-map walk rejects cross-links and covering cycles (:54084-54112).
   - `CFAT32Table::validate` `FUN_1800417c0` :53831-53926: FAT[2] (root cluster) != 0xFFFFFFFF, FAT[0] != 0xFFFFFFFF, EOC-family check on FAT[0] (`(FAT[0]&0xFFFFFFF)==0xFFFFFF8` + FAT[1] family :53853-53863); optional deep path `FUN_180041990` :53931-53950 (root chain entries must be valid-or-EOC, bounded by DAT_1800728d4 = 98%) and `FUN_180041a00` :53967-54008 (contiguity heuristic, `> 5` runs → reject); back-map cycle check :53883-53920.
   - FAT12 flavor: no dedicated validate fn found (family-tolerant media check `FUN_1800416d0` :53767-53778 reads FAT[0] u32 `& 0xFFFFF0 == 0xFFFFF0`).
4. **Per-entry Next (chain stepper)**:
   - FAT12 `FUN_1800416f0` :53783-53808: byte offset `cluster*3>>1`; even→`&0xFFF`, odd→`>>4`; `0xFFF → 0xFFFFFFFF`; `>0xFF7 → 0xFFFFFF8`; **`0xFF7 (BAD) returned RAW as 0xFF7`** (:53801-53804); bounds = FAT byte size only (`+0x28 * +0x18`), no cluster-count bound.
   - FAT16 `FUN_180041c80` :54125-54143: cluster-count bound (:54131); `0xFFF7 → 0xFFFFFF7` (canonical BAD); `≥0xFFF8 → 0xFFFFFF8`.
   - FAT32 entry read `FUN_1800419e0` :53955-53962 (u32 slot, cluster-count bound).
   - **CDoubleFATTable::Next** `FUN_180041de0` :54207-54251: primary first; on primary 0 → mirror; mirror answer scrutinized (backward: reject shifted copy `mirror(c+1)==alt+1`, reject cross-link `mirror(alt-1)==alt`, accept contiguity `mirror(alt)==alt+1`; forward/EOC-family accepted) — core ports as `mirror_consult_` (cr_chain.cpp:343-374, M-012/N-022).
5. **Run-collector** `FUN_18000f950` :11053-11132: batches contiguous clusters (`next == cur+1`) while `spc*count < 0x80` sectors (:11121) and not cancelled; checks the visited-range tree at +0x3D/+0x35 before accepting a run (:11075-11098).
6. **Directory chain materializer** `FUN_18000fb80` :11188-11345:
   ```
   while (uVar7 < 0xffffff8) {                       // EOC termination :11231
     if (cancel.vt[0x88]) break;                     // :11236-11238
     next/run = FUN_18000f950(...)                    // run-batched
     sector = (uVar7-2)*spc(P+4) + first_data(P+8)    // :11239
     if (device_size < sector) → free+return          // :11241
     if (cap(entry_cap*0x82) < run_bytes+filled) break// :11245
     device.vt[8](buf+filled, sector, spc*count)      // whole-run read :11247
     range-tree lookup uVar7 in P+0x10; hit && (ctl vt[0x68]&9) → abort  // :11250-11276 cycle guard
     if (cluster == start) { "."-sig 0x202020202020202E + ' '+attr&0x10 :11283;
                              ".."-sig 0x2020202020202e2e :11295 }
     entry-parser FUN_18000ffa0(...) :11323; leftover<0x400 bytes memmove :11328-11333
     insert run clusters [c, c+count) into tree :11319-11321 }
   ```
   `"Found unproessed buffer, lengh:%d bytes"` trace :11329 [sic].
7. **Dentry walk** `FUN_18000ffa0` :11350-11859 (+ entry validator `FUN_1800365a0` :44263-44375):
   - LFN: attr==0x0F (:11551) with run-start tracking local_148; LFN slots pass the validator's `attr==0xf && *(short*)(+0x1a)==0` gate (:44287).
   - Volume label: `(attr&2)==0 && (attr-8)&0xdf==0` (:11570); label copied to context (+0x1d8/+0x1e0, 400-char cap :11585-11587).
   - Dot files: `*pcVar21 == '.'` counter ≤1 (:11603-11608).
   - Cluster sanity: `first < 3 || ((first-2)*spc <= P+9)` (:11612); self-cluster == parent's → skip (:11614).
   - Deleted: `*pcVar21 == -0x1b` passed as flag to dir-recursion FUN_180015a30 (:11741-11742).
   - $RECYCLE.BIN: `_stricmp(name,"$RECYCLE.BIN")` (:11799).
   - Shape gates: file size⟺cluster nonzero (:44344-44348); dir size==0 && high-cluster(+0x14)==0 (:44350); DOS date sanity month/day/hour/min/2-sec on +0x10/+0x18/+0x12 (:44307-44318) and time/date +0xE/+0x16 with `day>0x1d` reject (:44499-44508, gated on global `DAT_18006be84` :44498); `~N` alias tail N∈[1,5] (:44335-44339); name-byte histogram, ≥10 repeats of one byte → reject (:44356-44369).
8. **Deleted-chain inference (FAT32 high-16 re-base)**: region :14600-14794 and :14825-14865:
   ```
   if ((*(char *)*param_2 == -0x1b) && (*param_7 < 0x10000)) {   // :14634 64KB guard
       … walk candidate uVar9 = (high<<16)|low via FUN_180012c00(:14720)
       if fail → retry from plain low16 (:14726)
       … *param_7 = (DAT_1800728c8 & ~0xFFFF) + uVar5  // re-base onto last-good high16 :14754-14755
       if ((0x310000 < DAT_1800728c8) && …) FUN_180012c00((DAT_1800728c8&~0xFFFF)+uVar2 …) // :14834-14836
       *(short*)(entry+0x14) = high; *(short*)(entry+0x1a) = low  // write back :14772-14773
   ```
   `DAT_1800728c8` = last successful high-16 region hint; 0x310000 gate = volumes with >3.1M clusters. Companion `FUN_1800134f0` validity probe (:14827) and `FUN_1800136f0` fallback (:14846).
9. **Root pre-scan / deleted counters** `FUN_1800351e0` walk region :42940-43103: LFN run counter global `DAT_180072938`; deleted counter at param_1+4 (:43074); "."/.. anchor clusters at param_1+1 / +0xC (:43015/:43040); max-cluster tracker param_1+2 (:42988).
10. **EOC-family tolerant checks** (dir-chain start validation) :8030-8046 and :8188-8364: `entry==0xFFFFFFF8` → EOC; `(e&0xFFFFFFF)==0xFFFFFF8` + `(next&0xFFFFFF8)==0xFFFFFF8` or `(next&0x7FFFFF8)==0x7FFFFF8`; alternate family `(e&0xFFFFFFF)==0xFFFFFF0` + `(next&0xFFFFFF0)==0xFFFFFF0`; bVar6 flag then feeds FUN_180008d50/FUN_18000b430 with `param_3-6` backtrack (:8048-8052) [INFERRED: EOC-neighbor probing].

### 1.3 Constants table
| Constant | Value | Og cite |
|---|---|---|
| EOC canonical (FAT16/32) | 0xFFFFFF8 | :53802/:54138 |
| EOC alt (FAT12 0xFFF) | 0xFFFFFFFF | :53799 |
| BAD canonical (FAT16) | 0xFFFFFF7 (from 0xFFF7) | :54135-54136 |
| BAD FAT12 | 0xFF7 returned RAW | :53801-53804 |
| FAT16 media pair | u32@FAT[0] == 0xFFFFFFF8 | :54052 |
| Deleted marker | 0xE5 (`-0x1b`) ×10 sites | :11742, :14634, :14825, :28571, :28601, :29978, :30178, :32155, :43066, :44169 |
| Deleted 64KB guard | `< 0x10000` clusters | :14634, :29978 |
| Run-batch cap | spc*count < 0x80 sectors | :11121 |
| Dir buffer capacity | entry_cap(0x13) * 0x82 bytes | :11221 |
| Leftover carry | < 0x400 bytes | :11328 |
| FAT32 watermark | cluster_count * 0.98 | :44782 |
| High16 re-base gate | 0x310000 < last-hint | :14834 |
| Dir shape | size==0 && +0x14==0 | :44350 |
| Spf windows | 12 / 12..0x100 / 0x100..0x96000 | :44716/:44743/:44775 |

### 1.4 DIVERGENCE CANDIDATES (FAT)
- **FAT-D1. FAT12 bad-cluster (0xFF7) semantics.** og `FUN_1800416f0:53801-53804` returns 0xFF7 raw (a value < the EOC threshold — the walker `FUN_18000fb80:11231` then *follows* cluster 0xFF7 subject only to device bounds; data pollution, not a stop). Core `cr_chain.cpp:48-52` canonicalizes `0xFF7 → CHAIN_BAD` and `follow_chain:453-455` ends the chain **broken**. Falsifiable: image with a FAT12 chain whose interior entry is 0xFF7 — og emits ≥1 extra cluster, core truncates with broken=true. Needs lead verification (downstream cluster-count clamp may exist elsewhere in og).
- **FAT-D2. FAT32 deleted-entry high-16 re-base heuristic absent in core.** og :14634-14860 (FUN_180012c00 candidate walk, `DAT_1800728c8` last-good-hint re-base :14754, 0x310000 gate :14834, write-back to dentry :14772-14773). Core `cr_fat_quick.cpp:286` takes `first_cluster_high<<16|low` verbatim and `reconstruct_deleted_chain` (cr_chain.cpp:468-507) walks from it; no high-16 candidate sweep, no hint cache. Falsifiable: deleted FAT32 file on >2^21-cluster volume with zeroed +0x14 word — og recovers correct chain, core yields wrong/partial chain. Needs lead verification.
- **FAT-D3. Run-batch IO shape.** og batches contiguous runs to ≤128 sectors per device call (`FUN_18000f950:11121`) with leftover-entry carry (:11328); core `read_dir_chain` (cr_fat_quick.cpp:44-58) batches unbounded runs (≤4 MiB total) and carries entries implicitly. Semantics equal, IO pattern/trace cadence differ. Low priority.
- **FAT-D4. Cycle-guard activation condition.** og range-tree abort fires only when `(controller vt+0x68 & 9) != 0` (`FUN_18000fb80:11271-11276`) — mode-conditional; core `follow_chain` always enforces VisitedRuns + max_len (cr_chain.cpp:437-450). Falsifiable only in the og's non-flagged modes. Needs lead verification (mode bits unpinned).
- **FAT-D5. Strict-date day>29 reject.** og time/date sanity at :44499-44508 applies `0x1d < (day & 0x1f)` to the DATE field too (day 30/31 rejected when the config global `DAT_18006be84` is on); core N-010c (cr_fat_quick.cpp:246-263) rejects only `month > 12 || day == 0`. Falsifiable: dentry with write-date day=31, strict flag on — og rejects, core emits. Needs lead verification (mask reuse in decompile is my inference).

### 1.5 Coverage notes
- Mirror scrutiny, spf windows, flavor brackets, string-first flavor, media-word, EOC families: covered (M-011/M-012/N-002/N-010a/S-064/N-201; core cr_chain.cpp:76-127, :343-374, :623-628).
- LFN: core implements checksum assembly + deleted-slot reverse-order fix (FAT-1) + illegal-char fallback (N-176) + deleted-checksum brute force (GAP-S6-1 superset) + '#' placeholder (N-172) — og placeholder rendering `FUN_180036350 :44166-44172` per core comment. The og's own LFN-slot→8.3 glue state machine is visible at :11540-11556/:42963-42977.
- Root pre-scan counters (DAT_180072938, deleted counter) have no direct core counterpart (core walks one pass; stats differ, not results).
- Recycle bin: og `_stricmp("$RECYCLE.BIN")` :11799; core M-052 with $I/$R pairing model (superset detail).

---

## 2. exFATSearchDLL — 58,043 lines

### 2.1 Exports & host contract
| Export | Body | Notes |
|---|---|---|
| `EUCreateEXFATSearcher` | :11-21 | `new(0x420)` → FUN_180003ad0 (CExFatCompleteSearch) |
| `EUCreateEXFATQuickSearcher` | :26-46 | `new(0x8E0)` → CExFatQuickSearch + sub-object `FUN_18000db40(+0x88)` (:43) — embeds the bitmap builder (CQuickSearchAndBuildBitmap vftables :10523/:10577/:11120/:11328) |

### 2.2 Pipeline map
1. **Boot validation + base−12 re-base ladder** — KNOWN COVERED (S-066; core `cr_exfat_dentry.cpp:131-207` ports `FUN_180021e80` gates :28180-28208 + `FUN_180001630` :341-420).
2. **Dentry validators (single entry)**:
   - `FUN_180003090` :1837-1873 — dispatcher over `(t-X)&0x7F` classes:
     - volume-label triple `(t-3)&0x7F==0`: requires next `0x81` (`-0x7f`) and `+0x40 == 0x82` (`-0x7e`) (:1847);
     - file `(t-5)&0x7F==0` → `FUN_180002f90` (:1848-1849);
     - stream `(t-0x40)&0x7F==0`: reserved +2,+6/+7,+0x10..0x13 zero; `u32@+0x14 == 1 || ≥ 0xFFFFFFF7` (:1853-1860) — **polarity: 1 or ≥EOC-family is INVALID** unless equal (per core EXFAT-1 re-read; artifact v2 inverted it);
     - filename `(t+0x3F)&0x7F==0` → `FUN_180003030` (:1863-1864).
   - `FUN_180002f90` :1779-1803 — File dentry: secondary-count∈[2,18] (`(byte)(sc-2)<0x11` :1787), 3× timestamp sanity via `FUN_180002f40`, reserved zero (:1795-1797).
   - `FUN_180002f40` :1756-1774 — packed timestamp: day≠0, month-1<12, hour<0xB801, min<0x761, 2sec<0x1E.
   - `FUN_180003030` :1808-1832 — name fragment: flags&3==0, flags<4, first char ≠0; 15 UTF-16 chars scanned; low-byte==0 chars checked against table `PTR_180045a00` (:1820-1822).
3. **Deleted triple reconstruction**:
   - `FUN_180003120` :1882-1966 — backward single step (entry at −0x40), three parallel std::list builders `FUN_18000b760`; requires BOTH stream-list and name-list non-empty (:1935-1937).
   - `FUN_180003390` :1971-2266 — full cascade over a directory buffer: 0x20-stride forward walk with per-class validation; triple complete when `2 < count && all three lists non-empty` (:2093-2094); invalid entry → all-zero check → backward single-step or reset (:2128-2148); deleted-pair detection `0xC0→0xC1` / live `0x40→0x41` (:2202-2218).
4. **Main dentry switch (inline, quick walk)** :24340-24451 — case 0x03/0x83 label (count<0xE, per-char table `DAT_18004a150`, tail zeros, live-only copy when `type ≥ 0x80` :24365); case 0x05/0x85 file (full field validation :24381-24394, `+0x1F == 0`); case 0x40/0xC0 stream (reserved zeros :24402-24403, first-cluster polarity :24404-24412, size<<shift < cluster bound, NoFatChain run `< 0xFFFFFFF6` :24408-24412); case 0x41/0xC1 name (:24415-24428); case 0x81 bitmap (cluster−2 < 0xFFFFFFF4, length fit, sum < 0xFFFFFFF6, reserved zeros :24430-24438); case 0x82 upcase (:24441-24449).
5. **FAT access** `FUN_180021c60` :28039-28213 — **1 MiB windowed cache**: byte offset `cluster*4`, sector-aligned window, rb-tree of windows (≤8; evict oldest), entry = `buf[(cluster & 0x3FFFF) * 4]` (:28148). Core S-013 ports this (cr_exfat_quick.cpp:24-83).
6. **Chain walk (quick scan driver)** `FUN_180032090` :41195-41519: `while (uVar30 < 0xFFFFFFF6)` (:41324) — termination at reserved/BAD/EOC family; FAT read `FUN_180021c60(P+0x12, cluster, &flag)` (:41347); **out-of-range entry → 0xFFFFFFFF** (:41348-41350); break on self-loop or next<2 (:41351); visited-cluster rb-tree at +0xDE8/:41379-41394; `"buildFileSet:%lld,sec:%lld,ICalcPartition:null,partition:%lld,%u"` trace (:41436-41437); ExFatCalcPartition vftable use :41453.
7. **Entry-set checksum** `FUN_18001eba0` :25153-25198 — dual-form ROL15+add (`(v>>1|v<<0xf)+b` :25172), skip indices 2..3 (:25171), InUse-restored ladder 0x85/0xC0/0xC1 when first byte == 5 (:25173-25186); valid iff either form == stored (:25194). Core M-013 ports exactly (cr_exfat_dentry.cpp:59-102).
8. **Name filters**: `"$RmMetadata$Reparse$Repair"` filter (:1599, 26-char compare), "$"/"#"/"$R" prefixes (:1608-1716); "._" AppleDouble sink (N-174, core :719-723); `$RECYCLE.BIN` `$I/$R` cascade :25715-25860 (`pcVar3[0x22] == '$'`).

### 2.3 Constants table
| Constant | Value | Og cite |
|---|---|---|
| Walk terminator | cluster ≥ 0xFFFFFFF6 | :41324 |
| Out-of-range FAT value | 0xFFFFFFFF | :41349 |
| First-cluster invalid | == 1 or ≥ 0xFFFFFFF7 | :1859, :24405, :24553 |
| Secondary count | [2,18] | :1787, :24381 |
| NoFatChain flag | stream flags & 0x02 | :24408 |
| Contig run bound | start+len < 0xFFFFFFF6 | :24409-24412, :24434 |
| FAT window | 1 MiB, cluster & 0x3FFFF | :28089, :28148 |
| Entry-set checksum | ROL15+add, skip 2..3, dual form | :25153-25198 |
| Deleted type classes | (t−3/−5/−0x40/+0x3F)&0x7F | :1847-1863 |
| Timestamp gates | day≠0, m∈[1,12], h≤23, min≤59, 2s≤58 | :1756-1774 |

### 2.4 DIVERGENCE CANDIDATES (exFAT)
- **EXFAT-D1. Reserved value 0xFFFFFFF6 chain termination.** og walk condition `uVar30 < 0xFFFFFFF6` (:41324) ends the chain (clean) at 0xFFFFFFF6 AND 0xFFFFFFF7; core `follow_exfat_chain` (cr_exfat_quick.cpp:113-133) returns raw 0xFFFFFFF6 from `next()` (not ≥ EOC_MIN, not FREE/BAD), then the bounds probe marks `broken=true` — a confidence downgrade the og never applies. Falsifiable: FAT entry 0xFFFFFFF6 mid-chain → og clean end, core `r.broken`. Needs lead verification (rare on-disk value).
- **EXFAT-D2. Out-of-range FAT entry as EOC vs FREE.** og :41348-41350 maps entry ≥ cluster_count to 0xFFFFFFFF (clean EOC); core `entry()` returns EXFAT_FREE for out-of-window (cr_exfat_quick.cpp:57-59) → EXFAT-2 assumed-contiguous continuation, not clean end. Falsifiable: chain whose next entry index ≥ cluster_count — og stops clean, core may assume c+1. Needs lead verification.
- **EXFAT-D3. Bitmap consultation posture.** og CQuickSearchAndBuildBitmap builds an in-memory mirror of the allocation bitmap *while* walking (class evidence only — vftables :10523+); the on-disk 0x81 dentry is validated (:24430-24438) but I found no read of bitmap bits to gate emission in the walked region. Core reads the bitmap stream (cr_exfat_quick.cpp:441-476) but uses it only for the S-009 free map, not gating. Behavioral parity unclear — the og's "deleted-cluster reasoning" claim (family doc §2) is unproven at function level. Needs lead verification.

### 2.5 Coverage notes
- M-013 (dual-form checksum + parked-set oracle), M-014 (structural gates), S-013 (windowed FAT), EXFAT-1/2/3/6, N-010e (byte-granular reserved OR-forms), N-067 (upcase validation grade-not-reject), N-174 ("._" sink) all ported with first-hand cites — verified against the same og lines this session.
- Name-hash (CRC16-of-upcased-name) verification: **absent in og** (no crc16/0xA001 computation anywhere; grep-verified) and absent in core — parity by mutual absence.
- Deleted triple backward walk (FUN_180003120/180003390) has no direct core twin; core's per-entry walk + M-013 parked-set/tail pool is the functional analog (torn-set parking). Candidate only if og's backward walk recovers sets the forward walk misses — unpinned.

---

## 3. EXT4SearchDLL — 18,071 lines

### 3.1 Exports & host contract
Exports `EUCreateEXT2Searcher`/`EUCreateEXT2QuickSearcher` (family doc; also EXT4 variants). CEXT4QuickSearch/CEXT4CompleteSearch controllers; **CEXT4CatchBufferDeviceProxy** bad-sector salvage proxy (0xE00 buffer, 0x400 backtrack re-read; deep-dive §3).

### 3.2 Pipeline map
1. **SB validation** `FUN_180004710` :3214-3253 — KNOWN COVERED (N-202; core `validate_superblock` cr_ext4_engine.cpp:135-204 adds log_bs≤6 and MAX_GROUPS caps).
2. **Mount/geometry** `FUN_1800047c0` :3254-3441 — reads 0x200@0x400, 64-bit desc_size via feature 0x10@+0xF8 → u16@+0x196 (:3300-3310 region), GDT buffer 2×block_size, strncpy label 0xF. Feature gate `+0xF8 & 0x10` reused by the inode resolvers (:3522/:3633).
3. **Inode-number → inode-table resolution** `FUN_180004c70` :3500-3606 / `FUN_180004e40` :3611-3718: group = ino % s_inodes_per_group (+0xDF8); descriptors-per-block = block_size>>(has64?6:5) (:3530); primary GDT buffer +0x4A0 (64-bit) / +0x498; entry = inode_table_lo at `(group<<5|6)+8` (:3547); per-group GDT block cache = rb-tree at +0xDE8, read `first_meta(+0x60) * block_size * group + partition_start(+0x30)` (:3582-3583). Core S-026 documents the og's meta-group stride divergence (D-59 family).
4. **Inode-table sweep** `FUN_180005e50` :4340-4581 — per-buffer loop at stride inode_size (P+0x64):
   ```
   while (iVar10 < *(int *)(param_1 + 0x58)) {                    // inodes per buffer :4417
     if (i_mode==0 || (flags@+0x20 & 0x80000)==0 || magic@+0x28 != 0xF30A || depth@+0x2A != 0) skip  // :4419-4420
     if (mode & 0xF000 == 0x4000) { dir: first-block resolve (FUN_180005340 for extents, i_block[0] raw) :4446-4453;
                                    register in dir-block trees +0x4B8/+0x4E8 }
     else { copy 256-byte inode into accumulator list; insert into size-keyed tree +0x4C8 }   // :4514-4573
   }
   ```
   **Filter = extent-bearing LEAF inodes only (EXTENTS_FL + depth 0)**; dot-entry pre-check for dir buffers (:4388-4411).
5. **Extent decode**:
   - lookup `FUN_180005340` :3867-3920: leaf scan of `ee_block ≤ logical < ee_block+ee_len` → `phys = ee_start − ee_block + logical` (:3886-3889); index nodes → read child block (block-size alloc :3914) and recurse.
   - run-list `FUN_180005670` :4021-4080: leaf extents → {48-bit-phys × block_size, ee_len × block_size} pairs appended to vector (:4044-4048); depth>0 → recursive child reads (:4064-4072). **ee_len high bit NOT masked** (I-045 KNOWN COVERED).
   - indirect (ext2/3) `FUN_180005490` :3925-4016: FUN_180005190 pointer walk, **EOF-bounded** (`param_4 <= lVar9` break :3953), merge-contiguous (:3974-3978). Core S-065 ports the size bound + merge.
6. **Dentry walk** `FUN_180006fc0` :5203-5450 — linear per-block: rec_len stride; filetype byte +7; dir (type 2) children with name≠"."/".." and inode≠parent-self registered in rb-trees (:5283-5294); **deleted-run counter** local_134 incremented for inode==0 entries with name_len≠0 (:5397-5400) passed into the entry processor; `"--FDT:fileName:%s,id=%I64d,type:%d,parentId:%I64d\n"` trace (:5430).
7. **Deleted-entry slack scanner** `FUN_180005820` :4085-4283 — from a deleted (inode==0) entry: 4-byte-aligned scan for subsequent valid dirents (filetype∈[1..7]-ish, rec_len 4-aligned ≥ align4(name_len+8), inode ≤ inodes_count :4164-4168); registers subdirs ("--SubDirFDT:" :4224) and files ("--SubFDT:" :4251) via `FUN_1800085f0`; **M-037 ports the resync slide** (core parse_dir_block :595-604).
8. **CatchBuffer proxy**: reads strided by geometry; on failure allocates 0xE00 buffer, retries at shifted window (lVar8−0x400) — deep-dive §3, vtable sites.
9. **Journal handling** — KNOWN COVERED (M-143 active: journal-inode harvest FUN_180003de0 :2773/:2778 pass 1/2; core S-025 cr_ext4_engine.cpp:822-990 + :1210-1320).

### 3.3 Constants table
| Constant | Value | Og cite |
|---|---|---|
| SB magic | 0xEF53 | :3222 |
| Extent magic | 0xF30A | :3826/:4420 |
| EXTENTS_FL | 0x80000 | :4419 |
| Leaf filter depth | == 0 | :4420 |
| Dir mode | 0x4000 | :4446 |
| Dirent stride floor | align4(name_len+8) | :4167, :4174 |
| Deleted dirent | inode == 0 | :4155/:4168 |
| Inode resolvers gate | +0xF8 & 0x10 | :3522/:3633 |
| GDT entry inode_table | +8 | :3547 |
| CatchBuffer | 0xE00 buf, 0x400 backtrack | deep-dive §3 |

### 3.4 DIVERGENCE CANDIDATES (ext4)
- **EXT4-D1. Deleted-inode sweep filter mismatch.** og sweep `FUN_180005e50:4419-4420` admits inodes with `mode != 0 && EXTENTS_FL && magic 0xF30A && depth == 0` — no dtime/links test in that loop; deleted inference is dirent-side (inode==0 + run counter :5397). Core sweep (cr_ext4_engine.cpp:1135-1147) requires `dtime != 0 || links == 0`. Falsifiable: an inode with dtime==0, links==0 (orphan-reserved) — og sweep path admits it (if extent-leaf), core skips it by design; conversely an inode with dtime≠0 but indirect-block (ext2/3) storage — og sweep filter EXCLUDES it, core includes it. Needs lead verification (which og pass emits sweep nodes is host-side).
- **EXT4-D2. Depth>0 extent-root inodes skipped by the og sweep.** og :4420 requires eh_depth == 0; large files with an index root never register in the dir-block registry / accumulator. Core `walk_extent_node` (cr_ext4_engine.cpp:329-382) walks any depth ≤ 5. Falsifiable: deleted file >4 extents with depth-1 root — core recovers extents, og sweep passes it over. Needs lead verification.
- **EXT4-D3. GDT read geometry.** og reads each group's GDT block at `first_meta(+0x60) * block_size * group + base` (:3582-3583) with no first_data_block adjustment; core S-026/D-59 documents this as a deliberate non-adoption (core is e2fsprogs-correct). Registered divergence — listed for completeness.
- **EXT4-D4. Dir-block checksum (htree tail csum)**: neither og nor core verify — parity by mutual absence (no crc call in either walk). Coverage note, not a candidate.

### 3.5 Coverage notes
- S-025 journal two-pass, S-026 meta_bg, S-027 backup-SB rescue (og FUN_180001b00 rebase found−0x400 vs core s_block_group_nr math — D-59), S-065 indirect ladder bound+merge, M-037 resync slide + inode==0 termination (core keeps past holes as I-014 superset), N-177 size-exceeds-blocks marker — all covered with cites.
- The og's dot-entry check and the dir-block registry rb-trees are structural; the core's parse_dir_block + hierarchy build is the analog.

---

## 4. HFSPlusSearchDLL — 22,304 lines

### 4.1 Exports & host contract
`EUCreateHFSSearcher`, `EUCreateHFSQuickSearcher` (family doc). Controllers:
CHFSPlus{Quick,Complete}FileSystemSearchController; ctor dual-vtable + `IDirecotryTreeBuilder` [sic] :1240-1254; QuickSearchFiles trace strings :495-511 ("Return Error:0x" / "\tFile:" / "\tLine:" / "\tFuction:" [sic]).

### 4.2 Pipeline map
1. **Volume-header validation** :10005-10020 — sig `0x482B|0x4858`, block&0x1FF==0, version<0x11, field clamps — KNOWN COVERED (M-142; core cr_hfsplus_engine.cpp:554+ N-005).
2. **Catalog leaf driver** `FUN_18000c8f0` :10017-10128: resolve-child via **journal replay** `FUN_180011300` (:10034); leaf-record math `base(+0x60) + 1 + record_count(+0x24) + attr_count(+0x20)` (:10037-10041); per-node cancel (vt+0x10) + EOF (vt+0x88) checks (:10047-10065); sibling-list walk building 0xD0-byte CHFSPlusFileNode via `FUN_180009770` (:10066-10070); invalid nodes (`[10] == -1`) destroyed; record-emit callback `vt[0x28](param_2, node, 5, &out)` (:10083); error codes 0x5017900000002 (:10031), 0x500af00000002 (:10118).
3. **Journal replay reader** `FUN_180011300` :13637-13796+ ("Journal Start Offset:0x" :13710, hfsplusjournal.cpp :13794): circular wrap `offset = (offset + size) % total` (:13743-13747/:13780-13783); block-sequence validation via `FUN_180011220` with per-entry sizes (`local_d0+4`); node images validated by `FUN_180010720` (hfsplusbtnode.cpp :12936-13036, `CHFSPlusBTNode::TestNodeBelongToFile` :13036) and built by `FUN_180010110`; error codes 0x5008c00000501 / 0x5009700000501 / 0x500a500000004 / 0x500a800000501.
4. **Catalog record decode** `FUN_180009940` :7588-7708 (via `FUN_180009770` ctor :7516-7558):
   - key: parent u32@+2, nameLen u16@+6, UTF-16 name@+8 with **leading-zero skip + 255-char cap** (:7618-7633);
   - record type 1 = folder/thread-ish (flags|=2, cnid@+4, type 0x10 at +0xCC :7648-7661);
   - type 2 = file (:7662-7691): fork-selection bit `flags@+0x29 & 0x20` → resource fork at +0x54 else data fork at +0x2C (:7665-7678); **zero-size fallback `size = u32@+0x32 << 0xC`** (:7669); dates +0x18/+0xC4/+0x19; **finder magic `*(int*)(+0x18) == 0x66647270`** ("prdf" family) → folderize flag :7687-7689; type 0x20 at +0xCC.
   - Journal-recovered records flagged (+0xCE bit 0, :7613-7615); resource-fork nodes +0xCE bit 2 (:7677).
5. **Key comparators** `FUN_18000ecf0` :11773-11833 (case-folded, keyCompareType 0xCF path via `FUN_18000eed0` :11884+): parent-ID first, then UTF-16 with **2-level fold table `DAT_180028040`** (:11798-11803) and **':'(0x3A) → '/'(0x2F)** conversion (:11816-11821); fast binary compare `FUN_18000ee30` :11838-11879 (same colon conversion, no folding).
6. **Extents overflow B-tree** — hfsplusextents.cpp functions :12425-12732 (trace strings); 8-fork overflow resolution for fragmented forks (class-level, deep-dive §2).
7. **Attributes B-tree / decmpfs** — hfsplusbtreeoption.cpp :14363-14599 region; core S-002 ports the key/record layout (og `FUN_180009c10` :7771-7841 per core comment).

### 4.3 Constants table
| Constant | Value | Og cite |
|---|---|---|
| VH signatures | 0x482B "H+", 0x4858 "HX" | :10005 |
| Leaf record base | +1 + record_count + attr_count | :10037-10041 |
| Error codes | 0x5017900000002, 0x500af0000002, 0x5008c00000501 family | :10031, :10118, :13733+ |
| Fork-select bit | flags@+0x29 & 0x20 | :7665 |
| Zero-size fallback | totalBlocks << 12 | :7669 |
| Finder folderize magic | 0x66647270 | :7687 |
| Colon fold | 0x3A → 0x2F | :11816-11821 |
| keyCompareType | 0xCF (case-fold) | :11895 |
| Name cap | 255 UTF-16 chars | :7630-7632 |

### 4.4 DIVERGENCE CANDIDATES (HFS+)
- **HFS-D1. Colon→slash name mapping absent in core.** og converts ':' to '/' in both comparators (:11816-11821) — and HFS+ conventionally *stores* Finder names with ':' where POSIX shows '/'. Core `parse_catalog_key` (cr_hfsplus_engine.cpp:221-237) decodes names verbatim; no colon mapping anywhere in the core (grep-verified; only cr_safewriter/crsf *sanitize* colons at write time). Falsifiable: a volume with a catalog name containing ':' — og-compare treats it as '/', core emits the raw ':'; restore-time name differs from Finder-truth. Needs lead verification (og emission path may not mutate the stored name — comparators only).
- **HFS-D2. Leaf-chain traversal via journal vs direct fork reads.** og's catalog walk resolves every child node through the journal replay (`FUN_180011300`, wrap arithmetic :13743/:13780); the core reads catalog nodes directly from the catalog fork extents + a separate journal *harvest* pass (cr_hfsplus_engine.cpp:1651-1664). On a torn-but-unjournaled catalog, both walk the same nodes; on a journaled volume with in-flight transactions the og's node set can differ from the core's committed-fork set. Needs lead verification (og fallback path when journal read fails is at 0x5008c… error exit — unpinned).
- **HFS-D3. Leading-zero name skip.** og skips leading 0x0000 chars in catalog names (:7623-7629) before the 255 cap; core `parse_catalog_key` copies name_len chars verbatim. Falsifiable: name with embedded leading NULs — og emits the trimmed name, core emits the NULs. Minor; needs lead verification.

### 4.5 Coverage notes
- A25/A-005 (records-up offset array), N-005, M-142, N-028a (dot-name + "HFS+ Private Data" sink), N-028c ('prdf' folderize), N-038 (ownerFlags 0x20 rsrc selection), N-155 (zero-size <<12 fallback — og hardcodes 4096), N-059 (flags/type-id carriers), S-002 (decmpfs modes 3/4 + kZlib extents), S-021 (journal harvest + attr differential), F-A2-17 (kind sign-extend) — all covered in core with og cites matching my reads.
- The og's freed-node "unused record" surface = freed catalog leaf nodes keep stale records (deep-dive §2); core scan_freed_nodes sweep + allocation-bitmap grading is the I-014-registered superset.
- The og's case-fold table (DAT_180028040, 2-level) is used for key ordering only; core's full leaf-chain sweep needs no comparator — parity posture, no user-visible delta unless ordering affects emission order (it does not in a chain walk).

---

## 5. BtrfsSearchDLL — 18,626 lines

### 5.1 Exports & host contract
| Export | Body | Notes |
|---|---|---|
| `EUCreateBtrfsComplateSearcher` [sic] | :11-17 | **STUB: `*param_1 = 0; return;` — the complete searcher is NOT implemented** |
| `EUCreateBtrfsQuickSearcher` | :22-55 | `new(0xC30)` → CBtrfsQuickSearch; sub-objects `FUN_180002f50(+0x98)` (:41), `FUN_1800013c0(+0x368)` (:50), 0x820 memset at +0x410 (:51) |

### 5.2 Pipeline map
1. **Superblock mirror probe** `FUN_18000d540` :10269-10326:
   ```
   if (0x10fff < sector_size * total_sectors) {                     // device-size gate :10283
     for (mirror in DAT_180018d60[]) {                              // NULL-terminated offset table
       if (device < mirror+0x1000) break;
       read 0x1000 at mirror;
       if (*(u64*)(sb+0x40) == 0x4D5F53665248425F) {                // "_BHRfS_M" :10298
          sectorsize@+0x90 pow2 ≠ 0; nodesize@+0x94 in [0x86,0x10000]; nodesize % sectorsize == 0 // :10299-10302
          if (first || gen@+0x48 newer) memcpy winner;              // best-generation select :10303-10307
       } else if (mirror_index == 0) break;                         // primary magic dead → ABANDON :10311-10312
     } }
   ```
   **No crc32c verification of the SB** (in either engine). Mirror table at `DAT_180018d60` (core ground-truthed to {0x10000, 0x4000000, 0x4000000000, 0x4000000000000}).
2. **Chunk-profile decode / logical→physical** — M-020 covered (core cr_btrfs_engine.cpp:158-267 ports og :2498-2543: 0x20-class rotating mirror `idx=saved; saved=(idx+1)%n` :2534-2540; RAID0/RAID10 = registered core supersets).
3. **Leaf item iteration** `FUN_18000d700` :10331-10509+: item count u32@+0xC (leaf-relative nritems), items at +0x65 + i*0x19 (17-byte key + offset u32@+0x11 + size u32@+0x15); each item's payload copied into a **0xA00000 (10 MiB) mutex-guarded arena** with cumulative size (:10450-10478) — the decompressed-extent cache; per-item 0x40-byte records list (:10485-10503).
4. **Per-inode item-range scan** (FS-tree walker, fn containing :3700-3931): items visited in key order from a 0x40-stride vector; **early termination** when `key.objectid > target` or `type > 0x6C` (:3781-3783); type dispatch:
   - `0x18` (24 = DIR_INDEX) → dir-entry decode via `FUN_1800057a0` when item size > 0x1D (:3831-3833);
   - `0x6C` (108 = EXTENT_DATA) → item size > 0x15; extent type∈{1,2} (`(type-1)<2` :3790); degenerate bytenr/num_bytes skip (:3791-3793); type 0 (INLINE) falls to accumulation (:3795-3800); extents accumulate into a 0x28-stride list keyed by key.offset (:3802-3826); compression byte tracked (:3811/:3823).
5. **Default-subvolume resolution** `FUN_1800061f0` :3938-4039+: root-tree search for DIR_ITEM `(objectid, 0x54, 0x8DBFC2D2)` (:3974-3975 — crc32c("default")); size ≥ 0x1F; name_len==7 + `memcmp(item+0x1E, "default", 7)` (:4009-4016); "root DIR_ITEM had same CRC32, but was not \"default\"" trace (:4011); location.type == 0x84 (ROOT_ITEM) → subvolume root lookup in rb-tree (:4019-4035). Core M-020 ports (fallback 5 at :4060; core :733-751).
6. **Dir-item/inode processing** region :4955-5054: inode-record gates (`flags@+0x1C & 8`, type>6, size@+0x62 ≤ +0x1D :4963-4964); name-tree memcmp matching (:5004); `CBtrfsNoResidentDataRunList` built from {phys, len} pairs (:5047-5054).
7. **Root refs / xattrs / generation**: N-047 (ROOT_REF 156 names), S-028 (XATTR 24, og :3424-3602), N-059 (inode qword@0) — covered in core (:68-78, :859-:1109).

### 5.3 Constants table
| Constant | Value | Og cite |
|---|---|---|
| SB magic | 0x4D5F53665248425F "_BHRfS_M" | :10298 |
| SB read size | 0x1000 | :10284-10296 |
| Nodesize band | [0x86, 0x10000], % sectorsize == 0 | :10301-10302 |
| Best-SB rule | max generation@+0x48 | :10303-10305 |
| Mirror table | DAT_180018d60 (NULL-terminated) | :10316 |
| Item header | 0x19 stride, items@+0x65 | :10423/:10456 |
| Dir index key | 0x18 (24) | :3784 |
| Extent data key | 0x6C (108) | :3785 |
| Extent types | {1,2} regular/prealloc; 0 inline | :3790/:3797 |
| Default-subvol key offset | 0x8DBFC2D2 | :3975 |
| Default-subvol name | "default" (len 7) | :4009-4016 |
| Location type | 0x84 ROOT_ITEM | :4019 |
| Arena | 0xA00000 (10 MiB), mutex | :10450-10451 |

### 5.4 DIVERGENCE CANDIDATES (Btrfs)
- **BTRFS-D1. Mirror-probe abandonment on dead primary.** og `FUN_18000d540:10311-10312` — a magic mismatch at mirror index 0 **breaks** the probe loop (mirrors are only consulted when the primary read+magic survived but validation failed, or when earlier mirrors were valid). Core (cr_btrfs_engine.cpp:500-513) `continue`s past a failed/invalid primary and can adopt a mirror SB. Falsifiable: volume with destroyed primary SB (non-_BHRfS_M bytes at 0x10000) + intact mirror — og returns no volume, core scans. Core superset. Needs lead verification (my decompile reading of the else-branch break is confident but the control flow is tangled).
- **BTRFS-D2. Device-size gate.** og requires `sector_size * total_sectors > 0x10FFF` before probing (:10283); core has no equivalent floor (probe loop bounded only by `mirror+4096 > slice->size()` :501). Only affects <64 KiB devices — cosmetic. Needs lead verification.
- **BTRFS-D3. Reserved-value termination semantics in extent accumulation.** og skips EXTENT_DATA items whose bytenr or num_bytes is 0 (:3791-3793) but treats type 0 (INLINE) via a separate fall-through; core BTR-3 (cr_btrfs_engine.cpp:881-896) requires `len ≥ FE_TYPE+1` for inline and emits resident data. Different acceptance edges on short items (og needs >0x15 bytes; core needs ≥21). Falsifiable: 0x15/0x16-byte EXTENT_DATA item — og skips, core may accumulate. Needs lead verification.
- **BTRFS-D4. Complete-scan stub.** og `EUCreateBtrfsComplateSearcher` returns NULL (:11-17) — the og has NO btrfs complete/deleted engine; deleted btrfs files surface only via RAW carving. Core `scan_btrfs` also emits only live-tree results (`(void)root_deleted; (void)root_lost;` :484-485). Parity — coverage note, not a divergence.

### 5.5 Coverage notes
- M-020 (chunk profiles + rotating mirror + default subvol), BTR-2 (chunk_tree_uuid from chunk-root node, og does NO uuid check — :10298 region has only the two name memcmps per core comment), BTR-3 (inline extents), N-072 (re-scan cache; og persists BtrfsCache.txt — string evidence, format undecoded; core's format is the registered one), S-028, N-047, N-059, N-016b — covered.
- SB crc32c: not verified by either engine — parity by mutual absence.
- Item-order early termination (objectid/type bounds, og :3781-3783) has no core twin (core scans all items per node) — result-equivalent, perf-only.

---

## 6. Cross-engine summary of top divergence candidates
(Also the source list for the final report.)
1. FAT-D1 FAT12 BAD 0xFF7 raw-follow vs core CHAIN_BAD break.
2. FAT-D2 FAT32 deleted high-16 re-base heuristic (og :14634-14860) missing in core.
3. FAT-D5 strict-date day>29 reject (og :44499-44508) vs core day==0-only.
4. FAT-D4 mode-conditional cycle guard vs core unconditional.
5. EXFAT-D1 reserved 0xFFFFFFF6: og clean end vs core broken.
6. EXFAT-D2 out-of-range FAT entry: og→EOC vs core→FREE/assumed-contiguous.
7. EXFAT-D3 bitmap-gating posture unpinned in og.
8. EXT4-D1 sweep filter: extent-leaf-any vs dtime/links.
9. EXT4-D2 depth>0 extent roots excluded by og sweep.
10. HFS-D1 colon→slash name mapping (comparator-side) absent in core.
11. HFS-D2 journal-mediated node resolution vs core direct-fork + harvest.
12. HFS-D3 leading-zero name trim.
13. BTRFS-D1 mirror-probe break on dead primary vs core continue.
14. BTRFS-D3 short-EXTENT_DATA acceptance edge.
15. BTRFS-D4 og complete-searcher is a NULL stub (parity posture).

Coverage-status one-liner: FAT/exFAT/ext4/HFS+ core engines are at high parity
(covered tags verified against the same lines this session); Btrfs core exceeds
the og (RAID0/10 supersets, cache, mirror-continue); the open deltas concentrate
in deleted-chain inference (FAT-D2), sentinel-canonicalization edges (FAT-D1,
EXFAT-D1/D2), sweep filters (EXT4-D1/D2), and name-convention edges (HFS-D1/D3).
