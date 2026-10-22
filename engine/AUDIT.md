# AUDIT.md — Session 5 Verification & Reconciliation Audit

**Date:** 2026-10-23 · **Auditor:** session-5 senior reviewer
**Method:** three parallel line-by-line reads (RE corpus / core_recovery / rda-driver),
followed by **independent verification of every disputed finding** against (a) the
corpus decompiled code, (b) the corpus binaries themselves (PE .rdata extraction),
and (c) authoritative upstream sources (OpenZFS master headers, Linux kernel
btrfs/ext4/ldm sources, Apple TN1150). Nothing below is unverified guesswork;
the verification column names the source for every finding.

**Status legend:** MATCH (impl correct vs ground truth) · MISMATCH (wrong vs ground
truth — fix required) · MISSING (absent — implement) · PARITY (deliberate
corpus-parity decision, documented) · REJECTED (finding from reading agents that
failed independent verification — recorded to prevent re-litigation).

---

## 1. Module audit matrix

| # | Module (impl) | RE/spec ground truth | Status | Findings → fix plan |
|---|---|---|---|---|
| 1 | `ntfs/cr_fixup.cpp` | corpus FUN_180041bb0 chain (NTFS deep-dive §3) | **MATCH** | USA gate semantics, single-slot USN compare, sector bounds — all spec-exact. |
| 2 | `ntfs/cr_runlist.cpp` | Linux fs/ntfs3 run.c; corpus size-walk | **MATCH** (impl) / **MISMATCH** (header doc) | Impl has the corrected LOW=len, HIGH=off nibble order; `cr_runlist.h:4-6` header comment still teaches the OLD INVERTED order — stale doc, must be rewritten. |
| 3 | `ntfs/cr_bootstrap.cpp` | corpus FUN_18001a3c0/a540 | **MISMATCH (1)** | Nonresident `$MFT $ATTRIBUTE_LIST` → `ERR_UNSUPPORTED` (cr_bootstrap.cpp:132): the og chases it (FUN_18001a540 fragmented-MFT handler). Implement nonresident attrlist stream reads. |
| 4 | `ntfs/cr_attributes.cpp` | spec + corpus FUN_1800434f0 | **MATCH** | Offsets validated against NTFS spec (worklog session-2 corrections hold). |
| 5 | `ntfs/cr_bitmap.cpp` | corpus FUN_18002c2e0 | **MATCH** | bit test, base re-basing, clamp — parity. |
| 6 | `ntfs/cr_mftwalk.cpp` | corpus FUN_18001acc0 loop | **MISMATCH (4)** | (a) `$ATTRIBUTE_LIST` chase is depth-1 only ("deeper lists are corrupt", line 177) — real volumes nest; follow to the FRAG_DEPTH_LIMIT like the bootstrap does. (b) resident ADS content stored size-only — content unrecoverable; store resident payload (bounded by record size) so the writer can emit it. (c) ADS attach targets `emitted.back()` = last hard-link node (primary-link semantic risk); attach to the first (canonical) node. (d) remnant scanner lacks the og's shifted-"FILE0" salvage (`'FILE'+0x30` at +0x58/+0x60 — verified in NTFSSearchDLL FUN_18002a780 @ lines 34152-34166): records whose header is damaged but body shifted-in get salvaged by the og. |
| 7 | `ntfs` $UsnJrnl cross-check | corpus: complete scan cross-checks `$Extend/$UsnJrnl` ($J) for recently-deleted files | **MISSING** | Implement: walk `$UsnJrnl:$J` (bounded, e.g. 64 MiB), parse USN_RECORD_V2 (reason FILE_DELETE 0x20000 / FILE_CREATE 0x10000, name+parent), surface journal-evidenced deleted names under root_lost that the MFT pass missed. |
| 8 | `fat/cr_chain.cpp` `cr_lfn.cpp` | corpus FAT deep-dive + FAT whitepaper | **MATCH** | EOC families (0x…F8-0x…FF incl. 0x…F0 BIOS forms via mask ≥0xFFFFFF8), BAD 0xFFFFFF7 norm, mirrored-FAT choose-healthy, LFN checksum — parity. |
| 9 | `fat/cr_fat_quick.cpp` | corpus | **MISMATCH (1)** | Backup-boot fallback reads at hardcoded `6*512` bytes (line 274) — on 4Kn volumes the FAT32 backup boot is at 6*4096. Use geometry sector size (violates locked fix F1). |
| 10 | `exfat/cr_exfat_dentry.cpp` | spec 3.1.14 + corpus ChecksumDBR | **MISMATCH (1)** | `volume_length` truncated to u32 (line 90 comment admits) — spec is u64 sectors; caps at 2 TiB@512b / 8 TiB@4Kn. Fix to u64 compare. |
| 11 | `exfat/cr_exfat_quick.cpp` | corpus FUN_180003090 dentry gates | **MISMATCH (1, minor)** | Og validates stream-dentry reserved fields (+2, +6/+7, +0x10..0x13 must be zero); we don't. Add the gate for parity-grade validation (damaged-set counting unaffected). |
| 12 | `ext4/cr_ext4_engine.cpp` SB/gates | corpus FUN_180004710 (verified by corpus agent + kernel ext4.h) | **MATCH** | 0xEF53@0x38, first_data_block×log_block_size kernel rule, 64-bit GDT desc_size@0xFE via flag 0x10@0xF8 — parity. |
| 13 | `ext4` inode decode | kernel `fs/ext4/ext4.h` `struct ext4_inode` | **MISMATCH (1)** | `i_crtime` read from +0x94 — that is `i_crtime_extra` (nsec+epoch). Real `i_crtime` @ **0x90**. All ext4 creation times wrong on real volumes. Also `I_CRTIME=0x80` constant mislabeled (0x80 = `i_extra_isize`). Fix offsets + use extra fields' epoch extension (extra>>30). |
| 14 | `ext4` journal salvage | corpus TraverseJournalDir | **MISMATCH (1)** | Parses EVERY journal block as a directory snapshot; no descriptor-block magic check (`0xC03B3998` cited in comment but never tested), no commit-chain walk (sequence-ordered transaction replay). Implement: descriptor magic gate, commit-block (magic 0x78224604?) chain with seq ordering, then directory-block extraction. |
| 15 | `hfsplus/cr_hfsplus_engine.cpp` VH + B-tree | corpus FUN_18000c8f0 area + TN1150 | **MATCH** | sig H+/HX, version<0x11, blockSize%512, fork-count ≤ totalBlocks, node header 14 B, MSB-first allocation bitmap — all verified, including against corpus decompile (0x482b/0x4858 gate at lines 10005-10031). |
| 16 | `hfsplus` catalog file layout | **TN1150 recount + Linux hfsplus_raw.h** | **MATCH — REJECTED finding** | Agent B claimed fork off-by-4 (0x5C/0xAC). Independent recount: recordType(2)+flags(2)+reserved1(4)+fileID(4)+6×dates(24)=0x20, permissions(16)@0x20, userInfo(16)@0x30, finderInfo(16)@0x40, textEncoding(4)@0x50, reserved(4)@0x54, **dataFork@0x58, resourceFork@0xA8, total 0xF8**. Impl correct; the "phantom reserved2[2]" in the claim does not exist. Fixture stays. |
| 17 | `btrfs/cr_btrfs_engine.cpp` SB | corpus mirror table **extracted from BtrfsSearchDLL.dll .rdata (VA 0x180018D60)** + kernel `btrfs_sb_offset()` | **MISMATCH (1)** | Impl `{0x10000, 0x40000, 0x4000000, 0x4000000000}`. Binary-extracted og table AND kernel: **`{0x10000, 0x4000000, 0x4000000000, 0x4000000000000}`** = {64 KiB, 64 MiB, 256 GiB, 1 PiB} (kernel: `16K << 12*mirror`). Impl has bogus 256 KiB slot, missing 1 PiB slot. |
| 18 | `btrfs` dir_item | kernel `btrfs_tree.h` + corpus FUN_1800044a0-area (lines 3424-3444: name_len read @+0x1B, data_len @+0x19, name @+0x1E, "user.DOSATTRIB" xattr walk) | **MISMATCH (2)** | Impl: `DI_MTIME@25, DI_CTIME@37, DI_NAMELEN@49, DI_DATALEN@51, DI_NAME@53` — invented mtime/ctime fields; real `btrfs_dir_item` = location(17) + transid@17 + data_len@25 + name_len@27 + type@29 + name@30. Also `de.type` read from byte 16 (location offset byte). Fix layout + drop the dead double name-read loop (lines 599-605). |
| 19 | `btrfs` root_item / misc | kernel btrfs_tree.h | **MISMATCH (2, minor)** | `s.level` dead double-assignment (lines 494-495); real `btrfs_root_item.level` @ **0xEE** (after inode 0xA0 + gen/root_dirid/bytenr/limits/…). Also chunk stripe offsets (devid@48/offset@56 within chunk item) — MATCH; SB core offsets (magic@0x40, gen@0x48, sectorsize@0x90, nodesize@0x94, sys_chunk_array_size@0xA0) — MATCH. |
| 20 | `btrfs` deleted recovery | corpus (newest-generation-wins only) | **PARITY** | Og reads only the newest generation tree; deleted files surface via RAW carving. Document as parity decision in engine header; no old-generation walk. |
| 21 | `zfs/cr_zfs_engine.cpp` labels | OpenZFS `vdev_impl.h` + `vdev_label_offset()` | **MISMATCH (2)** | (a) `label_base[1..3]` = quarter-points of (size−512K) — real: L1@**256 KiB**, L2@**size−512 KiB**, L3@**size−256 KiB** (L1 is fixed, not relative). (b) `VDEV_PHYS_OFFSET = 8K` — real **16K** (8K pad + 8K boot-env block precede vdev_phys). Only label 0 + lucky offsets can ever work today. (Note: og's odd 0x1C000 config probe is an og-side quirk — the spec offset is label+0x4000.) |
| 22 | `zfs` uberblock/blkptr/DVA | OpenZFS spa.h/dnode.h | **MATCH** | ring@label+128K, 128×1 KiB slots, txg@16, rootbp@40; DVA vdev=w0>>40 (24b), asize=(w0&0xFFFFFF)<<9, offset=(w1&2^63−1)<<9 — parity. **BUT** lsize/psize decode is raw (u16 of prop) — real encoding is `(stored+1)<<9` (BF64_GET_SB with bias); sizes are currently nonsense units. |
| 23 | `zfs` dnode_phys | OpenZFS `include/sys/dnode.h` (fetched, struct quoted) | **MISMATCH (4)** | Definitive layout: `dn_datablkszsec@8, dn_bonuslen@10, dn_extra_slots@12, dn_maxblkid@16, dn_used@24, dn_blkptr@64` (bonus at `64+nblkptr*128`, spill at end-of-tail). Impl uses 12/14/24/40 — everything 4–24 bytes off. |
| 24 | `zfs` znode bonus | OpenZFS znode_phys + SA (sa_impl.h) | **MISMATCH (4)** | Legacy `znode_phys`: mode@0x48 ✓, **size@0x70, parent@0x60, links@0x68** (impl reads 0x50/0x58/0x60 = uid/gid/parent). `ZP_ZAP@0xA0` is invented (no such field; directory content IS the object's ZAP). Modern pools use **SA** bonus (`SA_MAGIC 0x2F505A`, layout_info u16@4, layout 1 = atime..flags after 8-byte header → offsets shifted +8). Implement both paths keyed on `dn_bonustype` (DMU_OT_ZNODE=17 / DMU_OT_SA=43). |
| 25 | `zfs` ZAP micro/fat | OpenZFS zap_impl.h/zap_leaf.h (fetched, quoted) | **MISMATCH (5)** | `ZBT_MICRO = 0x8000000000000003` (impl: 0xFFFF…— never matches real blocks); `ZAP_MAGIC = 0x2F52AB2AB` (impl: 0x2F52AB52); mzap entry = **64 B** (value u64@0, cd u32@8, name@14 len 50) — impl: 88 B, value@16, name@24; entries-start@64 ✓. fzap: zt_blk@**16** (impl reads 144); `read_fzap` is `return false` — **unimplemented** (the only true stub in the tree). Full fzap: ptrtbl (embedded at half-block when zt_blk==0, else zt_blk blocks of u64 leaf block numbers) → leaf blocks (ZAP_LEAF_MAGIC 0x2AB1EAF, 24-byte chunks: ENTRY/ARRAY/FREE types, hash chains, name/value chunk arrays). |
| 26 | `zfs` data path | OpenZFS | **MISSING (3)** | (a) **No decompression** — real pools are LZ4-on by default; add off/lzjb/lz4/zle/gzip(zlib)/zstd. (b) **No indirect-block walk for file extents** — dn_nlevels≥2 files currently get *metadata blocks mapped as file data* (silent garbage, worse than truncation). (c) No embedded-BP (BPE) support (bit 39 of blk_prop). Also `stats.deleted_found`/`damaged_blocks` never accumulated. |
| 27 | `refs/cr_refs_engine.cpp` | corpus ReFS deep-dive + Prade thesis offsets | **MATCH** (module-level) | MSB chain, checkpoint dual, ObjectID table, allocator grading — full engine; deliberately wider version gate documented. One item below. |
| 28 | `refs/cr_refs_detect.cpp` | — | **MISMATCH (1, minor)** | Fallback `memcpy(out.version, "ReFS", 4)` fabricates a version string (line 56) — read the real major/minor from the boot sector (+0x28/+0x29) like the engine does. |
| 29 | `table/cr_table_parse.cpp` (MBR/EBR/GPT) | corpus PartitionRecoveryD + spec | **MATCH** | CRC walks, backup salvage, EBR cycle guard — clean. Dangling comment references nonexistent `refine_ntfs_vs_exfat` (line 82) — fix comment. |
| 30 | `table/cr_ldm.cpp` | Linux `block/partitions/ldm.c` (fetched as reference) + corpus LdmManager.mo | **MISMATCH — REWRITE (full LDM mandated)** | Current: recognition-only with **fabricated** fields — `disk_group_name="config present"` hardcoded (line 57); `status` read at +0x10 overlapping vblk_size (lines 72-73); "log" bitmap never checked; PRIVHEAD never scanned despite header claim. **Full LDM**: PRIVHEAD (3 copies: LBA 6, db+1856, db+2047; magic BE64, config_start/size, disk GUID@0x30, ver 2.11/2.12 gates), TOCBLOCK (4 copies: db+1/2/2045/2046; "config"/"log" bitmaps w/ names@0x24/0x46 + start/size), VMDB (db+17; vblk_size@+8, vblk_offset@+0xC, last_seq@+4, ver 4.10), VBLK array (composite 16-frame groups, 1 KiB? no — frames of vblk_size? per Linux: blocks of 512 B @ OFF_VBLK containing up to 4? — implement per reference), VBLK types {VOL5 0x51, CMP3 0x32, PRT3 0x33, DSK3/4 0x34/0x44, DGR3/4 0x35/0x45} with variable-width BE fields (vnum/vstr/ldm_relative chain), object graph → volumes (simple/spanned via component children, striped w/ chunksize interleave, mirrored = n copies, RAID-5) → **read-only virtual devices** for the scan orchestrator. |
| 31 | `lost/cr_candidates.cpp` | corpus CalcPartition scoring | **MISMATCH (2)** | (a) orchestrator passes `known={}` always (cr_scan.cpp:322) — overlap scoring (Inside −20 / Overlaps −10) is dead code; pass the live table. (b) `sector_count=2048` fallback ("minimum plausible") is fabricated — derive from device end (bounded probe) instead. |
| 32 | `carve/*` registry + validators | corpus rawsearch deep-dive + raw_signs.json (139 entries, fully read) + registry census (106 analysers, dual u16/u32 magic registries) | **MISMATCH — EXTEND** | 30 formats shipped with real grammar validators (good). Missing vs corpus: (a) the **raw_signs.json runtime layer** (header+tail hex signs w/ offset+confidence+min/max per extension — the og's layer-1 that covers 139 formats); (b) dedicated analysers the og ships that we lack: gzip, BMP, GIF, TIFF-MM variants, OGG/OGA, FLAC, APE, MPEG-PS/M2TS/ADTS sync families, MID, WMF/EMF, LNK, ICO/CUR, JP2/JXL?, SVG, HTML/XML text family, LZH, SIT, CHM, HLP, DWG/DXF, EPS, MOBI, DBX/MBOX?, X3F, MRW, ORF-II variants, WPG, PSP, dBase, AU, RM, MXF, MPEG-PS… (prioritize by recovery value; see fix plan). MP3 validator caps at 32 MiB fabricated extent — og walks frames to EOF (fix: strict frame walk, min 2 frames, stop at invalid, no arbitrary cap). PST 1 MiB damaged-header floor = **corpus parity** (og FUN_180041eb0 floor 0x100000) — keep. |
| 33 | `session/cr_crsf.cpp` + records | corpus .rsf format (chunked zlib, per-chunk fallback) | **MATCH (1 minor)** | Container parity incl. F10 graceful degradation. `Writer::open` writes only the lo u32 of session id (hi documented +0x1C never written) — write both dwords. |
| 34 | `tree/cr_node.cpp` | corpus CFileNode/CFileSystemNode dual-class | **MISMATCH (perf)** | by_record/by_extent are linear scans — O(n²) at carved-file scale (og uses indexed containers). Add hash indexes (by record_ref; extent bucket by first-offset). |
| 35 | `recovery/cr_safewriter.cpp` | corpus recovery manager | **MISMATCH (3)** | (a) `WriterStats.resumed` never incremented — no `.part` resume detection; implement (size+hash match on existing part → finalize + count). (b) hash-mismatch still commits (salvage) but header doc says "refuse-after-write mismatch is an error" — make behavior explicit: commit + per-item `hash_mismatch` flag surfaced to the CLI (already counted; surface it), fix the header contract. (c) `WinDestIo` ignores the computed per-id temp suffix and uses bare ".part" (cr_destio.cpp:14) — concurrent same-name items collide; honor the suffix. |
| 36 | `repair/cr_rawfixer.cpp` | corpus RawFixer v2 method inventory | **MATCH** | 33-method inventory cross-check: CheckDBR/FixFromDBR/UpdateDBR/FixDBR/FixByScanSector/CheckSector/Undo shared protocol ✓, FAT backup@6 ✓, exFAT TryFixFromSectorsPerCluster ✓ (two-candidate invariant), ChecksumDBR ✓ (VHD-style CRC over 11..209+510), CheckMustBeZero ✓. Solid. |
| 37 | `storage/cr_slice,cr_devmap` | R1 contracts | **MATCH** | — |
| 38 | `storage/cr_cached.cpp` | corpus CStorageDeviceWithCache | **MISMATCH (1)** | Caches *invalid* blocks permanently on inner failure — subsequent hits return ERR_READ_FAILED without re-attempt; do not cache IO-failure blocks (or retry once per miss). |
| 39 | `storage/cr_retry.cpp` | R1.6 | **MATCH (wiring gap)** | Library correct (bisection to sector, skip-mode, error map) — see #40. |
| 40 | orchestrator wiring | architecture.md §"CStorageDeviceWithCache equivalent" | **MISMATCH (2)** | (a) CachedReader + RetryingDevice are **never constructed on the scan path** (tests only) — R1.6 bad-sector isolation and live error-map population do not exist in production scans; wrap the transport in the orchestrator. (b) `run_deep` lost-partition discovery gated to whole-disk mode only — fine — but always with empty known table (see #31a). |
| 41 | `win/cr_transport.cpp` + `cr_rda.h` | rda-driver wire contract (Agent C, verified vs headers) | **MATCH (2 minor)** | Grammar/PID-token/÷512 Information/GET_INFO IoctlBase discovery — all correct (IoctlBase learned at open, cr_transport.cpp:254-257). (a) `session_get_next_block` granted estimate ignores plan-remaining — track cursor vs DataFirstSec+TotalSecs. (b) OPEN_TARGET explicit path unused (grammar path is valid — no change needed). |
| 42 | `win/cr_bitlocker.cpp` | corpus BitLockerLib (GetAutoUnlockKey/BEK) | **MISMATCH (2)** | (a) `try_auto_unlock` enumerates registry value NAMES and passes them as recovery passwords (lines 145-149) — can never succeed; the auto-unlock external keys are binary value DATA (BEK-class blobs); fix: extract binary data → temp .BEK file → `Unlock-BitLocker -RecoveryKeyFile` (documented, no crypto in-process, F13 honored). (b) `status()` dead Locked branch (lines 88-89) — first condition can never return Locked; fix order. |
| 43 | `win/cr_health.cpp` | corpus DiskHealth (multi-transport SMART) | **MISMATCH (1)** | Header documents legacy `SMART_RCV_DRIVE_DATA (0x7C088)` fallback — not implemented (ATA_PASS_THROUGH only). Add the legacy path (old-drivers parity). |
| 44 | `win/cr_enumerate.cpp` | — | **MISMATCH (1, minor)** | model/serial `while(*p)` bounded only by the 4096-B descriptor buffer — add explicit bound (malformed-descriptor overread). |
| 45 | fs_kind numbering | corpus: factory map {0-2 FAT12/16/32, 3 NTFS, 6 RAW, 7 exFAT, 8 HFS+, 9 EXT4, 10 ReFS, 11 Btrfs, 13 ZFS} + rsf content map {0 NTFS, 1 FAT, 3 RAW, 4 exFAT, 5 HFS, 6 ReFS, 7 Btrfs, 8 EXT4, 9 generic, 0xC ZFS} | **MISMATCH (collision)** | Impl mixes the two maps: NTFS=3 ✓factory, exFAT=7 ✓factory, HFS=8 ✓factory, EXT4=9 ✓factory, ReFS=10 ✓factory, FAT=0/1/2 ✓factory, but **Btrfs=7 collides with exFAT** (factory: 11), ZFS=12 (rsf map's 0xC — mixed scheme; factory: 13), carved=0xFF (factory RAW: 6). Fix: adopt the factory map completely (Btrfs=11, Zfs=13, carved/RAW=6), single enum in cr_types.h, session-load remap for v1 files (12→13 unambiguous; 7 that was Btrfs is indistinguishable from exFAT — documented limitation, noted in error-fix-registry as F-new). VolumeInfo/PartitionContext keep the internal FsFamily (renamed comment) — no wire change there. |
| 46 | tests | — | **MISMATCH (fixtures)** | ZFS/Btrfs/ext4-crtime fixtures were authored from the same wrong reading as the engines (self-fulfilling). Rewrite against upstream layouts: real dir_item bytes, real dnode/znode/SA offsets, labels at spec positions (incl. L1-L3), crtime@0x90, mzap entry format. HFS+ fixture is CORRECT (keep — it guards the verified layout). |
| 47 | fuzz harnesses | session mandate | **MISSING (extend)** | 4 targets exist (pure/engines/carve/container); add per-parser structure-aware targets (ntfs, fat, exfat, ext4, hfsplus, btrfs, zfs, refs, ldm, crsf-reader) as separate entry points, and a TSan CI job (mandate C). |
| 48 | CI | mandate C | **MISSING (1)** | No TSan job; add to ci.yml (linux, -fsanitize=thread, separate from ASan). |
| 49 | docs | — | **MISMATCH (stale)** | testing.md says "54 tests" (96 TEST() macros actual, 106 post-fix); architecture.md error-family list omits 0x5007/8/9; README v0.1 scope (fixed this session, commit 0db53c8); ground-truth-contracts LDM note (fixed). cr_runlist.h header (see #2). |

## 2. Rejected findings (independent verification)

| Claim | Verdict | Evidence |
|---|---|---|
| HFS+ catalog forks off-by-4 (0x5C/0xAC, total 0xFC) | **REJECTED** | TN1150 struct: permissions@0x20, userInfo@0x30, finderInfo@0x40, textEncoding@0x50, reserved@0x54, dataFork@**0x58**, resourceFork@**0xA8**, total **0xF8**. The claim inserted a phantom `reserved2[2]` after accessDate (backupDate lives at 0x1C). Impl + fixture correct. |
| rda session codes assume IoctlBase 0 | **REJECTED** | `RdaClient::open` queries GET_INFO and stores `info.ioctl_base` (cr_transport.cpp:254-257); all session codes add it (lines 395+). |
| Btrfs chunk stripe offsets wrong | **REJECTED** | devid@48/offset@56 match kernel `btrfs_chunk` (stripes start @0x30: devid@0x30=48, offset@0x38=56). |
| Btrfs SB field offsets wrong | **REJECTED** | magic@0x40, generation@0x48, sectorsize@0x90, nodesize@0x94, sys_chunk_array_size@0xA0 all match kernel `btrfs_super_block`. |
| ZFS DVA decode wrong | **REJECTED** | vdev=w0>>40&0xFFFFFF, asize=(w0&0xFFFFFF)<<9, offset=(w1&mask)<<9 match OpenZFS `DVA_GET_*`. |
| Uberblock ring geometry wrong | **REJECTED** | ring@label+128K, 128 slots × 1 KiB, txg@+0x10 — matches both corpus and OpenZFS. |
| PST 1 MiB floor is a fabrication | **REJECTED as a defect** (it IS corpus parity) | og FUN_180041eb0: `else floor 0x100000`. Keep; label retained. |

## 3. Corpus-vs-corpus discrepancies recorded (upstream wins)

1. `zfs-engine--deep-dive.md` says "0x80-byte uberblocks" — stride is 0x80 qwords = 1 KiB (algorithms-v2 correct; OpenZFS confirms).
2. Artifact ZIP field labels (`version_needed@+8`, `extradata@+0x1A`) — decompile shows +8 = compression method (gate ≤10 / probe ≤0x13 or 0x62 WavPack), +0x1A = file-name length (1..0x103). Numbers right, names wrong.
3. NTFS deep-dive v1 labels `(flags&0x10)==0` "in-use branch" — it is the folder-bit gate for the viability walk; v2 + code confirm.
4. Og reads the ZFS config nvlist at disk offset 0x1C000 (16 KiB window, limit 0x1BFFC) — real nvlist at label+0x4000 (16K) per OpenZFS; the og offset/limit pair is internally inconsistent (limit exceeds its own window). We follow the spec; divergence documented here.
5. Og mirror table {64K, 64M, 256G, 1P} — matches kernel exactly (this session's .rdata extraction), correcting the reading agent's "0x40000" transcription.

## 4. Fix plan (execution order)

**Wave 1 — engine truth fixes (highest value):**
1. ZFS engine rewrite of on-disk access: labels/L1-L3, vdev_phys@16K, dnode 8/10/16/64,
   znode legacy+SA, mzap 64B format, fzap full, (stored+1)<<9 sizes, indirect-block
   extent walk, embedded BPs, decompression (off/lzjb/lz4/zle/gzip/zstd), deleted stats.
2. Btrfs dir_item/mirrors/level + fixture rewrite.
3. ext4 crtime@0x90 (+extra decode) + journal descriptor/commit chain.
4. LDM full implementation (PRIVHEAD/TOCBLOCK/VMDB/VBLK incl. composites, object graph,
   spanned/striped/mirrored/RAID-5 volume reconstruction, virtual read-only devices,
   orchestrator + CLI surfacing).
5. NTFS: nonresident $MFT attrlist, multi-level attrlist, resident ADS content,
   UsnJrnl harvest, FILE0 remnant salvage, ADS attach fix.

**Wave 2 — wiring/semantic fixes:** scan-path cache+retry wiring (live error map),
lost::discover known table, fs_kind unification + session remap, FAT 4Kn backup boot,
exFAT u64 volume_length + stream reserved gates, BitLocker auto-unlock, SafeWriter
resume/temp-suffix/mismatch surfacing, refs detect version, CachedReader invalid-block,
enumerate bounds, health legacy SMART, transport remaining-tracking, CRSF session hi.

**Wave 3 — carving depth:** raw_signs.json runtime layer (139 formats), missing
high-value analysers (gzip, BMP, GIF, TIFF-MM, OGG, FLAC, MP3-to-EOF fix, …),
MP3 frame walk, registry confidence merge with JSON signs.

**Wave 4 — quality infrastructure:** fixture rewrites to upstream layouts, per-parser
fuzz targets, TSan CI job, node index hashes, doc refresh (testing/architecture/
runlist header), full ASan/UBSan/TSan + Windows /W4 /WX green, benchmark + release.

## 5. Verification ledger (how each ground truth was established)

- **OpenZFS master** (raw.githubusercontent.com/openzfs/zfs/master): dnode.h
  (dnode_phys), zap_impl.h (ZAP_MAGIC 0x2F52AB2AB, ZBT_*, mzap_phys), zap_leaf.h
  (leaf chunks), sa_impl.h (SA_MAGIC 0x2F505A, layout_info), vdev_impl.h (label
  layout 8K+8K+112K+128K, VDEV_LABELS 4), dmu.h (object types incl. DMU_OT_ZNODE=17,
  DMU_OT_SA=43, DMU_OT_DIRECTORY_CONTENTS=20).
- **Linux kernel master** (torvalds mirror): fs/btrfs/disk-io.h
  (`btrfs_sb_offset`: 16K<<12·mirror; BTRFS_SUPER_MIRROR_SHIFT=12),
  include/uapi/linux/btrfs_tree.h (btrfs_dir_item), block/partitions/ldm.{c,h}
  (full LDM reference — saved to scripts/ldm_reference.*), fs/ext4/ext4.h
  (inode layout i_crtime@0x90).
- **Corpus binaries**: BtrfsSearchDLL.dll .rdata VA 0x180018D60 extracted via PE
  section math (scripts/extract_btrfs_mirrors.py) → {0x10000, 0x4000000,
  0x4000000000, 0x4000000000000, 0}.
- **Corpus decompile**: NTFSSearchDLL FUN_18002a780 (FILE0 salvage), BtrfsSearchDLL
  lines 3424-3444 (dir_item field offsets via "user.DOSATTRIB" walk), HFSPlusSearchDLL
  lines 10005-10031 (VH gates), ZfsSearchDLL lines 21052/21365 (label reads).
- **Apple TN1150**: HFSPlusCatalogFile recount (see §2).
