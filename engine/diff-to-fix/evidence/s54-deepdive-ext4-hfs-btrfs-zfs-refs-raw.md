# Deep dive E — ext4/HFS+/Btrfs/ZFS/ReFS/RAW: math, scan, validators, hang-safety (pin 2794da8b / corpus be0cfe9)

Read-only parity audit. Exclusion list (`scratch/s54/exclusion-list.md`) and
`docs/PARITY-DELTAS.md` read first; every candidate below was cross-checked
against both. Corpus cites are `re-workspace/reversed/ghidra-searchdll/*_decompiled.c`
line numbers unless another file is named. Core cites are `src/core/...` + include/.

## Coverage log

| # | Site walked | Notes |
|---|---|---|
| 1 | exclusion-list.md (full), PARITY-DELTAS.md (engine sections: D-04/05/06/07/09/46/48/50/58/59/93/94, D-59b..D-59k, S-015/D-115 carve rows) | registered-divergence baseline |
| 2 | AUDIT-LINUX-FS.md (ext4/HFS+/Btrfs/ZFS/ReFS sections), AUDIT.md rows 20/204 | prior audit deductions |
| 3 | Core ext4: `cr_ext4_engine.cpp` (all 1251 lines) | superblock/validator, Volume/GDT/meta_bg, extent walk, indirect ladder, dir parser, journal pass 1+2, sweep, backup-SB rescue |
| 4 | og EXT4SearchDLL: FUN_180004710 (:3214-3247 SB validator), FUN_1800047c0 (:3254-3437 mount/GDT init), FUN_180004c70/FUN_180004e40 (:3500-3718 meta_bg resolver), FUN_180005020 (:3723-3762 inode read), FUN_180005120/FUN_180005190 (:3767-3862 block read + indirect ladder), FUN_180005340 (:3867-3920 extent logical→phys), FUN_180005490/FUN_180005670 (:3925-4056 datarun builders), FUN_180005e50 (:4340-4579 journal block parser incl. inode branch), FUN_1800068b0 (:4898-5128 inode/dir processor), FUN_180003de0 (:2722-2802 complete-search run), FUN_1800041e0 (:2961-3044 orphan finalize), FUN_18000e5b0/e890 (:12159-12368 journal passes) | first-hand re-derivation of all four axes |
| 5 | Core HFS+: `cr_hfsplus_engine.cpp` (all 1802 lines) | VH validator, forks, B-tree node/record readers, attributes tree, journal harvest + fallback salvage, catalog chain walk, orphan sweep |
| 6 | og HFSPlusSearchDLL: FUN_18000c6e0 (:9900-10012 VH validator), the VH read at :9128, FUN_18000c8f0 (:10017-10128 journal emit driver), CHFSPlusRawfile::Read (:13311-13376 fork-extent math), FUN_180013a70/FUN_180013de0 (:15362-15711 CHFSPlusBTree::Traverse), CHFSPlusJournal ctor (:13440-13539) | first-hand |
| 7 | Core Btrfs: `cr_btrfs_engine.cpp` (all 1194 lines), `cr_btrfs_cache.cpp` (skim) | SB validator, mirror select, chunk map/classes, node/item readers, tree walk, subvol/dir walk |
| 8 | og BtrfsSearchDLL: FUN_18000d540 (:10269-10326 mirror SB scan + validator), FUN_18000d700 (:10331-10544 leaf/internal item iteration), FUN_1800023f0 (:961-1109 walk driver + claim-bitmap check), FUN_180007260 (:4721+ claim-bitmap test), chunk sites :2044/:2162-2174/:2473/:2534-2540 (M-020 pins re-verified) | first-hand |
| 9 | Core ZFS: `cr_zfs_engine.cpp` (all 1809 lines) | labels/nvlist, blkprop/DVA decode, fletcher4, gang, dnode/objset, ZAP, SA registry, ring-state walk |
| 10 | og ZfsSearchDLL: ring walk :5760-5842 (stride 0x400, addBlock), vdev/nvlist FUN at :21030-21150 (read 0x4000@0x4000... see Axis-1 note), uberblock magic sites :15049/:32321/:32365/:35244-35264, fletcher gate :18076-18124 (core-cited, re-verified shape) | first-hand |
| 11 | Core ReFS: `cr_refs_detect.cpp` (all 103 lines), `cr_refs_engine.cpp` (:1-1200 walked in detail, remainder skimmed — the file is inline-og-cited throughout) | boot validator, page/SUPB/CHKP chain, checkpoint descriptor list, container translate, tables walk, attribute/run walk |
| 12 | og ReFS: `refs-btrfs-zfs-engine-algorithms-v2.md` §1 (page-signature chain), detect gates cited by core (FUN_180012390 :13529-13549, FUN_180012650 :13744-13790) re-verified via artifact + core pins | artifact-level (see UNRESOLVED-6) |
| 13 | Core RAW: `cr_carve.cpp` (all 1448 lines), `cr_rawsigns.cpp` (all 85), `cr_rawsigns_data.cpp` (head — 139 formats/199 header/62 tail signs, generated from corpus raw_signs.json), `cr_registry.cpp` (structure + F8 gate), `cr_analysers.cpp` (JPEG validator spot-check :526-760), `cr_usersigns.cpp` (skim) | scan strategy, gates, guards |
| 14 | og RAWSearchDLL: FUN_18000c820 (:9385-9569 block-scan loop: sector stepping, bitmap gate, zero-dword metrics, dispatch), zero-skip :9490-9520 (N-031 pin), pending-16 cap :9573-9591 (I-031 pin) | first-hand re-derivation of Axis 2 |
| 15 | Artifacts: ext4-engine--deep-dive.md, ext4-hfs-engine-algorithms-v2.md, hfsplus-engine--deep-dive.md, btrfs-engine--deep-dive.md, refs-btrfs-zfs-engine-algorithms-v2.md, zfs-engine--deep-dive.md, zfs-vdev--mftmirr--calcpartition.md, remaining-engines--ext4-hfs-refs-btrfs-zfs.md, rawsearch-carving--deep-dive.md, raw-dual-registry--u16-magic-keys.md, rawsearchdll--family-rawspeed/special.md (skim) | claim index; two artifact errata found (see §RAW notes) |

---

## Per-engine comparison

### ext4

#### Axis 1 — block/LBA arithmetic

| Quantity | og (EXT4SearchDLL, first-hand) | core (`cr_ext4_engine.cpp`) | verdict |
|---|---|---|---|
| SB read | 0x200 bytes at +0x400 (FUN_1800047c0 :3302-3303) | 1024 bytes at 1024 (:659) | parity (core reads the full SB; og only needs 0x200) |
| block_size | `0x400 << (s_log_block_size & 0x1F)`, accept while `(int)result > 0` (:3232-3233) → effective band log 0..20 | `1024 << (log & 0x1F)`, reject `log_bs > 6` (:147-149, :155) | core stricter (see F-E6) |
| total blocks | `CONCAT44(s_blocks_hi@0x150, blocks_lo)` 64-bit (:3234) | `blocks_lo \| hi<<32` (:158) | parity |
| groups | `blocks / bpg`, gates `0 < groups && groups <= blocks` (:3238-3239) | same + `groups > MAX_GROUPS(1M)` reject (:161-163) | core extra cap (F-E4) |
| front GDT byte offset | `base + block_size` (:3420-3422) — correct for ≥2KiB (block 1); one block early on 1KiB volumes (GDT is block 2) — the registered I-014 E9 defect | `base + (first_data_block+1)*block_size` (:709, :724) — correct all geometries | registered (D-58/D-59 refs); confirmed at :3422 |
| GDT bytes / desc_size | `_Var19 = round_up(groups*desc_size, block_size)` (:3406-3408); desc_size = clamp(s_desc_size,32,64) unconditionally (:3392-3405); META_BG-only `+0xdf8` = s_desc_size min 1 (:3368-3373) | `groups*desc_size`, reject `> 8 MiB` (:720-722); desc_size = 64BIT ? clamp(s_desc_size,64,64) : 32 (:191-199); meta stride `block_size/desc_size` | parity on canonical geometries; core caps (F-E4), combo differences (F-E6), meta_bg family registered (D-58) |
| inode table location | `group=(ino-1)/ipg`; block = `desc.inode_table + (index*inode_size)/block_size`; offset = `(index*inode_size) % block_size` (FUN_180005020 :3736-3757, stride 0x20/0x40 by GDT buffer class) | `table_off = base + inode_table*bs + index*inode_size` (:384-389) | parity (same result); og allows `group == groups` (off-by-one, :3738), core `g >= groups` rejects |
| extent leaf math | ee_block u32@0, ee_len u16@4, start = ee_start_lo u32@8 \| ee_start_hi u16@6 <<32 (FUN_180005340 :3885-3889); run builder FUN_180005670: `off = start*bs`, `len = (u16)ee_len * bs` (:4044-4048) — **raw ee_len, no 0x8000 unwritten mask** | same field decode (:340-347); `len = raw>0x8000 ? raw&0x7FFF : raw` (kernel decode, :345-346) | core kernel-correct; og defect on unwritten extents (F-E5) |
| extent interior | child = ei_leaf_lo@4 \| ei_leaf_hi@6<<32; iterative descent `do{...}while(magic)` — no depth limit (FUN_180005340 :3905-3917) | child = same (:368); recursive, `level > 5` reject + per-inode budget 65536 (:335, :426) | core bounded (extra guard, kernel max depth 5) |
| indirect blocks (ext2/3) | direct 12; single/double/triple via FUN_180005190 (:3788-3862); per-file content walk bounded by file size (FUN_180005490 :3949-3998 `param_4 <= lVar9`, lVar9 += bs) | same ladder (:437-475) — **no size bound, no node budget** (F-E3) | divergence — core unbounded |
| i_size | `CONCAT44(i_size_high@0x6C, i_size_lo)` applied to ALL inode kinds (:5000) | 64-bit size only for regular files (:404-406) | registered (I-040/D-123) |
| inode_size | rev≥1 → s_inode_size else 128 (:3388-3391) | same + sanity fallback to 128 when out of [128,1024] range (:172-174) | parity + core fallback |
| journal inum | copied from SB only when feature_compat HAS_JOURNAL (bit 2) set (:3365-3367) | read unconditionally; default 8 when 0 (:177-179) | minor delta (F-E6 item) |

#### Axis 2 — scan strategy

Og complete search (FUN_180003de0 :2722-2802): mount init (SB at 1024, re-validate,
GDT read) → 10 MiB VirtualAlloc IO buffer → journal pass 1 (FUN_18000e5b0, inum from
compat-gated field) → live walk from root inode 2 (FUN_1800068b0, recursion over dir
entries) → journal pass 2 (FUN_18000e890) → cleanup → orphan finalize (FUN_1800041e0,
"iNode%I64d" naming for unreferenced harvested records :2988-2991). **No whole-inode-table
sweep** in the og complete path. Core: same skeleton (S-025-pinned) + registered supersets:
dtime sweep (D-59g), backup-SB rescue grid (D-59), meta_bg resolver (D-58). The journal
passes differ in one branch — the og's per-block parser FUN_180005e50 treats a journal
block as EITHER a directory block (leading '.'/'..' pair) OR an **inode-table image**
(extent-gated inode scan, :4414-4576); the core's journal pass has only the directory
model (F-E2). Lost-volume scan: og FUN_180001b00 probes caller-supplied sector positions
with the primary validator, rebases sub-searcher at `found-0x400` (D-59 registered);
core probes the canonical 1 MiB grid + exact s_block_group_nr rebase (registered).

#### Axis 3 — detection validators

| Gate | og | core | verdict |
|---|---|---|---|
| magic 0xEF53 @+0x38 | :3223 | :136 | parity |
| incompat compression bit0 clear | `(byte@0x60 & 1)==0` :3223 | `sb[0x60]&1` :138 | parity |
| inodes/blocks/bpg/ipg nonzero | :3225 | :144 | parity |
| first_data_block×log_bs kernel rule (both forms) | :3226-3231 | :151-154 | parity |
| log_bs band | ≤20 effective (int-sign) | ≤6 | core stricter (F-E6) |
| rev≥1 requires inode_size != 0 | `rev==0 \|\| inode_size!=0` :3236 | fallback to 128, accepts | og stricter (F-E6) |
| blocks_hi sign | `-1 < hi` :3237 | unchecked | cosmetic |
| groups sanity | nonzero, ≤blocks, gdt-word check :3239 | + MAX_GROUPS 1M | core cap (F-E4) |
| BIGALLOC | no gate | ERR_UNSUPPORTED refusal | registered (D-59e) |

#### Axis 4 — hang safety

| Loop | og guard | core guard | verdict |
|---|---|---|---|
| extent-tree descent | none (iterative, cancel-only) | depth ≤5 + budget 65536/inode | core guarded (extra, kernel-bounded) |
| indirect ladder | bounded by file size (per-block translate, FUN_180005490) | **none** — full per_block^levels tree walk | core defect (F-E3) |
| dir walk | visited via record maps; cancel per entry | visited_inodes set + depth 64 + budget 200000 | core guarded |
| journal block loop | cancel per block | cancel per block | parity |
| journal fixpoint (core pass 2) | n/a | rounds ≤ journal_dirs+2 (:1197) | core guarded |
| dir-entry resync | 4-byte slide, gates (M-037) | same | parity (pinned) |
| meta_bg resolver | n/a (D-58 defect shape) | read budget 4096 meta-groups | core guarded |

### HFS+

#### Axis 1 — block/LBA arithmetic

| Quantity | og | core | verdict |
|---|---|---|---|
| VH location | 512-byte read at 1024 (:9128) | 512 at 1024 (:1278-1279) | parity; no backup-VH probe on either side (both miss TN1150 alternate VH at end-1024 — shared limitation, noted) |
| fork extent math | `phys = startBlock*blockSize + (logical-cursor)`; extent walk `blockCount*blockSize` per descriptor (CHFSPlusRawfile::Read :13312-13360) | `try_extents`: same cursor/extent math (:164-178) | parity |
| node → block | node N at fork-logical `N*node_size`, mapped through initial 8 + overflow extents | same (`read_tree_node` :157-191) | parity |
| BTHeaderRec | nodeSize@0x12, first/lastLeaf@0x0A/0x0E (FUN_180012160, audit F-A2-18 pin) | same (:71-73) | parity |
| record offsets | records UP from 14; record i = [slot(i+1), slot(i+2)) (og journal validator FUN_180010720, A-005) | same (`for_each_record` :317-343) | parity (pinned) |
| allocation bitmap | MSB-first bit per byte | MSB-first (:1136, :1690) | parity |

#### Axis 2 — scan strategy

Og: VH at 1024 → hfspluslib port (volume → B-trees). Catalog enumeration via
CHFSPlusBTree::Traverse (FUN_180013a70→FUN_180013de0): **root-descent** — fetch root
node, kind gate (only -1 leaf / 0 index proceed, :15544), index nodes recurse per record
(:15587 self-call), leaves emit records through the callback (grade 5, :15662). Core:
**leaf-chain walk** (firstLeaf → fLink … → lastLeaf, :1380-1406) — ignores index damage
entirely (superset on damaged trees; strategy difference, see UNRESOLVED-5). Extents
overflow preloaded by core vs og lazy per-file lookup (result-equal). Journal: D-93
(parity + registered hardening). Orphan sweep: core superset (registered D-59g/I-014).

#### Axis 3 — detection validators (VH)

First-hand decode of FUN_18000c6e0 :10005-10006. The function copies 512 bytes
(dest = &local_218, 4×128-byte iterations) then byte-swaps exactly six fields; stack
offsets (rbp-relative, copy dest base = local_218 ⇒ buffer offset = 0x218 − stackoff)
map them to: sig@+0x00, version@+0x02, journalInfoBlock@+0x0C, blockSize@+0x28,
totalBlocks@+0x2C, freeBlocks@+0x30. The gate:

```
sig ∈ {0x482B, 0x4858} && (blockSize & 0x1FF) == 0 && version < 0x11
   && freeBlocks(VH+0x30) <= totalBlocks(VH+0x2C)
   && journalInfoBlock(VH+0x0C) <= totalBlocks(VH+0x2C)
```

Core (`validate_volume_header` :554-592) instead gates
`allocationFile.totalBlocks(VH+0x7C) <= totalBlocks && extentsFile.totalBlocks(VH+0xCC) <= totalBlocks`
(:577) — **different fields**. The artifact (hfsplus-engine--deep-dive.md §1 :23-24)
and AUDIT-LINUX-FS §2.1 both encode the fork-clamp reading; the fork fields are never
byte-swapped by the og, so the og cannot be comparing them. See **F-E1**. Remaining
gates: sig/version/512-multiple parity; blockSize floor 512 + 16 MiB ceiling = core
hardening (N-005 registered); totalBlocks != 0 = core-only (the og can accept a zeroed
totalBlocks when freeBlocks/jib are also 0 — sub-case of F-E1).

#### Axis 4 — hang safety

| Loop | og guard | core guard |
|---|---|---|
| B-tree Traverse | **recursive, no depth limit, no visited set** (cancel per record :15552) | leaf chain: MAX_TREE_NODES 4M, self-loop break, last-leaf break (:1380-1405) |
| extents-overflow walk | lazy | guard 65536 nodes (:1311) |
| attributes walk | lazy (B-tree search) | MAX_TREE_NODES guard + 1M entry cap (:402, :474) |
| journal ring walk | terminates only on `off == jhdr.start` or cancel — can cycle (D-93 documented) | one full ring of advances + size caps (D-93 registered hardening) |
| fzap leaves | n/a | seen_blocks dedup + 64 ptrtbl-blk cap + 16K leaf cap (:894-929) |

Core strictly guarded; og's Traverse recursion is an og-side crash risk (stack overflow
on corrupt index chains) — core's leaf-chain strategy is immune by construction.

### Btrfs

#### Axis 1 — arithmetic

| Quantity | og | core | verdict |
|---|---|---|---|
| SB mirrors | .rdata `DAT_180018d60` = {0x10000, 0x4000000, 0x4000000000, 0x4000000000000, 0} (FUN_18000d540 :10296) | same table (:494-495) | parity (artifact's "0x40000" second-mirror claim is a corpus-doc typo) |
| SB fields | magic u64@0x40, generation@0x48, sectorsize@0x90, nodesize@0x94 (:10298-10304) | same (:22-28) | parity |
| nodesize band | `(ns - 0x86) < 0xFF7B` unsigned → [0x86, 0x10000]; `ns % ss == 0`; ss pow2 (:10299-10302) | same band + **ns pow2 required** (:433-438) | core stricter on non-pow2 ns (table note; no real mkfs output hits it) |
| item math | leaf items at node+0x65, 25-byte stride; data at node+0x65+item.offset, size item.size (FUN_18000d700 :10417-10479); internal key_ptr 33-byte stride (:10514-10527) | same (:43, :57-61, :356-380) | parity |
| chunk item | size = `num_stripes*32 + 48` (:2044); RAID10 geometry gate (:2162-2168) | same (:321, :324) | parity (M-020 pinned) |
| logical→physical | class decode :2498-2543: DUP/RAID1/SINGLE → class 0x20 rotating mirror (`idx=saved; saved=(idx+1)%n; phys=stripe[idx].offset+rel` :2534-2540); RAID0(8)/RAID10(0x40) → read failure | class_2copy identical incl. rotation; RAID0/RAID10 kernel math = registered superset (M-020) | parity + registered superset |
| chunk-tree uuid | **no uuid check anywhere** (only two memcmps in the whole decompile, both name compares — BTR-2 pin) | bytenr + fsid + chunk-uuid checks (:282-301) | core hardening superset (BTR-2 registered) |

#### Axis 2 — scan strategy

Og (FUN_18000d540): for each mirror offset (device must satisfy `total ≥ 0x11000`
units, :10283 — a ~68 KiB minimum; core has no minimum but the first-mirror bound
makes outcomes equal): read 0x1000; if magic matches but geometry fails → **continue to
next mirror**; if magic fails at mirror 0 → **abort the whole scan** (:10311-10312);
newest generation wins (strictly-greater, ties keep earlier mirror, :10303-10307).
Core: any gate failure on any mirror → continue (D-48a registered superset). Tree
discovery: core probes SB slot candidates (0x50/0x58/0x60/0x68/0x70) + system-chunk
scan, validated by self-referential bytenr + CHUNK-item presence (:559-623); og
equivalent not fully decoded (UNRESOLVED-1). Subvol walk: ROOT_ITEMs, default-subvol
DIR_ITEM crc32c("default")=0x8dbfc2d2 pick (M-020 pinned), ROOT_REF names (N-047),
"@eaDir" suppression (M-110) — all pinned inline.

#### Axis 3 — validators

- Superblock CRC32C: **neither og nor core verifies it.** The og memcpy's the winning
  SB (:10305) with no csum check; the only CRC32 string in the decompile is the
  DIR_ITEM name-hash error (:4011). Core `validate_superblock` has no csum check
  either. Parity — the answer to the mission's question is "the og copies, it does not
  verify". Node-header csums: also unverified by both (core checks bytenr+fsid instead).
- Geometry gates: parity except core's extra nodesize pow2 (above).
- Per-item bounds: og aborts the whole node when `size + 0x7E > nodesize`
  (:10483 `goto LAB_18000da8a`); core skips the damaged item and keeps the rest
  (:373-374). Core more tolerant (superset on damaged leaves; table note). The og also
  does not bounds-check `item.offset` before its 10 MiB-pool memcpy (:10475-10478) — an
  og overread the core's `data_pos` checks do not replicate.
- num_stripes: core rejects >16 (:319); og has no explicit bound (bounded only by the
  item-length arithmetic).

#### Axis 4 — hang safety

Og walk driver (FUN_1800023f0): per node a **claim-bitmap check** (FUN_180007260 —
"is this logical block already claimed/known?" via two bitmap objects, :990-991) that
returns early for claimed nodes, plus cancel per record. No visited set or depth cap
found in the decoded surface. Core `walk_tree`: BFS with `std::set<u64> visited`,
MAX_NODES 1M, queue cap 65536, nritems ≤ node capacity (:385-423). Core strictly
bounded. (Full og guard census: UNRESOLVED-1.)

### ZFS

#### Axis 1 — arithmetic

| Quantity | og | core | verdict |
|---|---|---|---|
| label geometry | 4 labels 256 KiB (2 front/2 back); ring slots stride **0x400 fixed** (:5841 `pvVar18 += 0x400`) | same layout; ring probes ashift shifts 10..13 (D-04 registered superset) | parity + registered superset |
| vdev_phys config read | FUN_180018220(..., **0x4000, 0x1C000**) then parse from buf+4 with limit 0x1BFFC (:21052-21055) — arg order is (dev, buf, offset=0x4000, size=0x1C000): the whole 112 KiB vdev_phys at label+16 KiB (the 0x1BFFC limit = size−4 proves it) | reads VDEV_PHYS_SIZE (112 KiB) at VDEV_PHYS_OFFSET (16 KiB), parses from +4 (:1239-1243) | parity — the zfs-vdev artifact's "read(offset=0x1C000, size=0x4000)" has the two args swapped (corpus-doc erratum) |
| nvlist fields | og hand-rolled parser; recursion into `vdev_tree` (type field == 9 at :21089), uint64s accumulated big-endian (type 2, :21113-21141), only the `id` field consumed (name check 'i','d' :21114-21117) | OpenZFS XDR/native parse: UINT64=8 BE, STRING=9, NVLIST=19; consumes name/ashift/asize/offset/guid | result-parity on real pools; og's internal type enum mapping undecoded (UNRESOLVED-2); field superset registered (D-59e) |
| blkprop/DVA | checksum bits 40-47 (`prop>>0x28 & 0xFF`, :18010 core-cited); DVA vdev=(w0>>32)&0xFFFFFF, asize=(w0&0xFFFFFF)<<9, offset=(w1&2^63-1)<<9, gang=w1 bit63 | identical (:116-151) | parity (M-019 pins) |
| fletcher4 | interleaved two-word form over RAW bytes, compare 32 bytes vs blk_cksum (:18093-18117) | byte-exact port (:160-191) | parity (M-019) |
| gang | 512-byte header, 3 blkptrs | same (:599-634) | parity |
| dnode/indirect | levels via indblkshift, per_node = (1<<shift)/128 | same (:699-741) | parity |

#### Axis 2 — scan strategy

Og quick searcher: walks the label-0 ring at 0x400 stride, every slot's txg →
addBlock per-txg registry (S-034 pinned; the "neighbor-txg" comparisons at :35244-35264
are ring-wrap ordering, not newest-only selection — D-05's refutation confirmed).
Core: all four labels' rings (registered geometry superset), txg-dedup + newest-first,
per-state live/recall walks (S-034 design). N-072 label retry with backoff = registered.
BE pools refused by core with a clear error (D-09 scope).

#### Axis 3 — validators

- **Uberblock checksum: verified by neither.** Og: magic 0x00BAB10C + txg handling
  only (ring sites :5760-5842; addBlock FUN_180003150 takes txg, no SHA256/LABEL
  verification); torn states are caught later by the MOS tree-open failure
  (FUN_180016c50, S-034's model). Core: magic + txg≠0; torn states skipped at
  `mos.load` (:1402-1407). Parity.
- Data-block checksums: og verifies **only fletcher4 (zio cksum type 7) and type 0**
  (`(uVar22==7)||(uVar22==0)`, :18076) — SHA256/other types pass unverified; core
  matches exactly (:565-573, og-parity comment). Parity (both under-verify vs OpenZFS;
  not a divergence).
- nvlist version gates: neither gates on the nvlist version word (both skip
  {version, nvflag}); table note.
- txg 0 slots: og addBlock no-ops (registry miss); core skips (:1316-1317). Parity.

#### Axis 4 — hang safety

Og: per D-05/S-034 first-hand decode — the state walk is an **unbounded descent**
(`from_txg < birth_txg || from_txg == 0` always true at top level); cancel token only.
Core guards: MAX_OBJECTS 1M, MAX_ZAP_ENTRIES 65536, MAX_ZAP_LEAFS 16384, MAX_DEPTH 48,
MAX_L0_PER_DNODE 4096, gang depth 4, nvlist depth 6 + 512 pairs, fzap leaf dedup set,
per-dnode L0 budget (:389-395, :1088, :1128). All core-only hardening (same family as
D-93's registered journal bound) — no finding.

### ReFS

#### Axis 1 — arithmetic (core pins re-verified at artifact level)

- Boot: "ReFS"@+3, "FSRS"@0x10, bps@0x20 ∈ {256,512,1024,2048,4096} ladder,
  spc@0x24, **cluster product gate** `bps*spc ∈ {4096, 65536}` (og
  FUN_180012390 :13544-13549, N-013b) — core `validate_boot` :598-623 matches exactly.
- Pages: 16 KiB (4 clusters @4 KiB / 1 @64 KiB) — core :710-715; page LCN quads
  {c0..c3} with zeroed tail = contiguous (:143-149); volume sig = GUID 4×u32 XOR
  (:754-755); SUPB at cluster 30 + backups n-3/n-2 (:725-742 — og-side probe positions
  per AUDIT-LINUX-FS row, see UNRESOLVED-6).
- Checkpoint: dual pages, newer virtual clock @+0x60 wins (:787-788); descriptor
  count@page+0x90, list @+0x94 (minor<14) / +0xA8 (minor≥14) — the BODY minor gates
  (GAP-S6-3); next-chain @ref+0x28 followed ≤16 hops with forward-progress check
  (:850-860).
- Container translate: `key = ((block/span)>>1 - 2)*span; block' = (block & (span-1))
  | map[key]` (N-036, og FUN_180011e90 :13223-13267) — core :111-121 identical incl.
  unsigned wrap.
- Allocator bitmap rows, parent-child table, ObjectID tableID→LCN-quad: core walk
  matches the pinned og shapes (inline cites).

#### Axis 2/3 — strategy and validators

Version gates: og accepts `major==3 && minor>3` (checkpoint body re-validated,
FUN_180012650) + a major==1 path; core accepts 1..3 with full shape validation —
**registered superset (D-48b)**. One core-stricterness delta not covered by D-48b:
core rejects a checkpoint body whose major differs from the VBR major
(`body_major != 0 && body_major != boot.major → ERR_BAD_MAGIC`, :815-816); the og
compares the body only against the constant 3/>3, never against the VBR. On a mixed
VBR/body (torn VBR major byte), og proceeds, core refuses — garbage-volume edge; table
note (folded into UNRESOLVED-6 follow-ups rather than a numbered finding: impact is a
corrupt-header acceptance band, same class as F-E6's ext4 items but on a shape both
sides already heavily gate).
Zero-size/no-dot filter, empty-name emission, $I/$R recycle staging, resident
materialization gates, ID2/orphan harvest — all inline-og-cited (N-141, S-053, M-027,
S-020/M-022) and spot-verified against their cited decompile lines during the earlier
audit waves; no new deltas found in this pass.

#### Axis 4 — hang safety

Core: `walk_table` — depth ≤ MAX_TABLE_DEPTH(32), nodes ≤ 4096/table, per-first-LCN
visited set (cycle break :271-275); attr_walk_runs — depth ≤32 + hops ≤64 (:434);
checkpoint next-chain ≤16 hops + monotonic (:850-858). Og-side guard census for the
table walk was not decoded this session (UNRESOLVED-6); the og's page-read path is
driven by the same reference chains (RefsBaseDataImplManager), and the audit's ReFS
section did not flag a cycle risk. Core's guards are strictly defensive; no missing
guard in core.

### RAW (carve)

#### Axis 2 — scan strategy (first-hand og block scan, FUN_18000c820)

- **Stepping**: the og dispatches candidates at **sector granularity** — the loop
  variable counts sectors (`local_1e0`), candidate byte = `sector_size*i + base`
  (:9496-9497 `puVar19 = iVar8 * local_1e0 + base`), step 1 sector, with the
  used-cluster gate able to jump a whole cluster (`local_1e0 += spc-1`, :9457-9459 —
  the M-005 pin). Core grammar layer steps **every byte** (`for off…++off`,
  cr_carve.cpp:526) — **registered superset D-59c** (do not re-file); the raw-sign
  layer steps 512-byte strides (:846) matching the og's sign stride.
- **Sign dispatch**: og probes the dual registry (u16 first-2-byte keys, 39 entries,
  then u32 4CC table, then raw_signs.json signs at intra-stride offsets 0..46);
  core `probe_header` tests offsets 0..46 within each 512-stride
  (cr_rawsigns.cpp:34-49) — same span; the 139-format/199-header/62-tail tables are
  GENERATED from the corpus raw_signs.json (cr_rawsigns_data.cpp) — parity by
  construction.
- **Zero fast-skip**: og leading/trailing zero-dword metrics skip all-zero sectors
  (:9499-9510); core skips a stride whose first 47 bytes are zero (:965-977, N-031).
- **Tail pairing**: og streams the full KiB-scaled window (FUN_1800205b0, up to
  max<<10 = 200 MiB); core caps the forward walk at the registry's tail-scan limit
  with 1 MiB chunked overlap reads (:1013-1080) — **registered divergence (S-015)**.
- **Windowing**: core 1 MiB windows + 4096-byte overlap (:45-46); og windowing shape
  (the block-scan caller's chunking) not re-derived this session — the D-115/M-025
  records cover the read-model deltas.
- Candidate ranking: og accumulated header+tail confidence; core N-070 score
  accumulate + registry-order ties (:657-666) — pinned.
- Pending-stream cap: og 16 size-unknown candidates, force-finalize oldest
  (:9573-9591); Stop() force-emits partials — registered I-031 addenda (stateless
  core architecture).

#### Axis 3 — validators

Grammar layer: per-format structure walks in core validators (cr_analysers.cpp, e.g.
JPEG SOF precision-8 + EOI + fallback-salvage, og-pinned :39653/FUN_18004bbd0);
og's hardcoded C++ analysers are the counterpart (488 vftable refs; brute-force
school walks to EOF). Header-only vs footer/size: og raw_signs.json rows carry
min/max KiB bounds + tail signs; core enforces min/max + tail pairing
(:1014-1085). Registry census parity (u16/u32 dual keys vs core trial table +
raw-sign tags) was covered by the engine-matrix/M-133 work; no new delta found at
the structural level. Per-format gate-by-gate comparison of all ~150 validators is
out of scope for this pass (the analyser families are under active audit —
M-098/099/106/112/117/121/122, N-151/153 etc.); the two spot checks (JPEG, PDF
version grammar :41230) matched their pins.

#### Axis 4 — hang safety

Core: window loop + per-byte dispatch with cancel checks (:527-528); M-117 moved the
covered check before the validator (quadratic walk removed); overlap_covered ranges
for suppress_overlap streams; OOM contract N-139; F9 extent-index dedup. Og: cancel
per sector; pending-16 cap; the zero-skip. No unguarded iteration found on either
side at the scan-loop level.

---

## Candidate findings (excluding already-filed IDs)

### F-E1 · MAJOR · hfsplus · VH validator gates the wrong fields (fork clamps vs freeBlocks/journalInfoBlock)

- **Og** (HFSPlusSearchDLL FUN_18000c6e0, :10005-10006):
  `(((local_218 == 0x482b) || (local_218 == 0x4858)) && ((uStack_1f0 & 0x1ff) == 0)) &&
   (((uStack_216 < 0x11 && (local_1e8 <= uStack_1ec)) && (uStack_20c <= uStack_1ec)))`
  — with the copy destination at `&local_218` and Ghidra frame offsets, the six
  swapped fields are exactly sig(+0), version(+2), journalInfoBlock(+0x0C),
  blockSize(+0x28), totalBlocks(+0x2C), freeBlocks(+0x30); the fork fields are never
  swapped, so the compared values cannot be fork counts. Gates: **freeBlocks ≤
  totalBlocks AND journalInfoBlock ≤ totalBlocks**.
- **Core** (`cr_hfsplus_engine.cpp:574-578`): `if (alloc.total_blocks > total_blocks ||
  ext.total_blocks > total_blocks) return false;` — gates allocationFile(+0x7C) and
  extentsFile(+0xCC) fork counts instead (comment claims "decompile-verified gate").
- **Gap (falsifiable)**: fixture A — VH with freeBlocks=0xFFFFFFFF, totalBlocks=0x10000,
  valid forks/sig/version/blockSize: og rejects, core accepts. Fixture B — VH with
  allocationFile.totalBlocks=0xFFFFFFFF (torn), everything else valid: og accepts and
  scans, core returns ERR_BAD_MAGIC and the whole volume is skipped.
- **Impact**: acceptance-band divergence on torn/damaged volume headers — exactly the
  recovery-target population. Direction B (core refuses an og-scannable volume) is
  lost recovery; direction A is a false-positive-volume risk the og guards.
- **Propagation note**: the misreading is encoded in
  `artifacts/SearchDLL/hfsplus-engine--deep-dive.md` §1 (:23-24
  "allocFileBlockCount ≤ totalBlocks && extentsFileBlockCount ≤ totalBlocks") and in
  AUDIT-LINUX-FS.md §2.1 row 1 ("two fork-count ≤ totalBlocks") — both docs need the
  correction when the code is fixed.
- **Fix direction**: replace the fork clamps with `free_blocks <= total_blocks &&
  journal_info_block <= total_blocks` in `validate_volume_header` (keep the
  N-005-registered blockSize bounds and consider adding the og's two gates on top of,
  not instead of, the fork clamps if a strictly-wider recovery stance is wanted —
  then register it). Update the two corpus-doc claims.

### F-E2 · MAJOR · ext4 · journal inode-image harvest branch absent (og FUN_180005e50 inode scan)

- **Og**: the journal passes feed EVERY journal block to FUN_180005e50
  (FUN_18000e5b0 :12230-12250: per-block calls `FUN_180005e50(engine, buffer+i*unit,
  …)`). FUN_180005e50 first tests the block for a leading '.'/'..' dir-entry pair
  (:4388-4411 → parent-map insert, return); otherwise it scans the block as an
  **inode-table image**: loop `i*inode_size < block_size` (:4417-4422), per record
  gates `i_size_lo(+4) != 0 && (i_flags(+0x20) & 0x80000) && extent magic 0xF30A at
  +0x28 && eh_entries != 0` (:4419-4420), then the dir branch (mode 0x4000) or the
  file branch — the file branch copies the 128-byte inode into the engine's record
  list (+0x4b8, :4514-4545) and links it into the +0x4c8 map (:4547-4573), the same
  machinery the orphan finalize (FUN_1800041e0 :2982-2993, "iNode%I64d" naming) and
  reachability checks consume.
- **Core** (`cr_ext4_engine.cpp:867-925`): journal pass 1 parses each block with
  `parse_dir_block` ONLY (directory model; no inode-shaped record scan anywhere).
- **Gap (falsifiable)**: ext4 image with a jbd2 journal (jsb magic + blocktype 3/4,
  s_first/s_maxlen covering it) whose in-range block is an inode-table image (a
  regular-file inode with size!=0, EXTENTS_FL, valid extent header) and no leading
  '.'/'..' pair: the og surfaces a file record from it; the core emits nothing from
  that block (its dir parser's majority gate rejects the image).
- **Impact**: ext4's jbd2 logs metadata — inode-table blocks are journaled in
  ordered/data=journal modes, so recently-written files whose directory blocks are
  gone are recoverable og-side from journal inode images and are invisible to the
  core. This is the sibling surface of S-004's directory-block harvest (filed SEVERE
  when absent); rated MAJOR here only because the og-side emission chain for the
  inode branch is decoded down to the record-list insert, not all the way to the
  result callback (see UNRESOLVED-3).
- **Fix direction**: extend the pass-1 harvest with the og's inode-image branch
  (same gate set: size_lo!=0, EXTENTS_FL, 0xF30A+entries, mode dir/reg; harvested
  inodes enter the same fixpoint/reachability flow as harvested dir entries —
  unreferenced ones take the "Inode%u" naming the sweep already uses).

### F-E3 · MAJOR · ext4 · indirect-block ladder is unbounded (hang/OOM); og bounds the walk by file size

- **Core** (`cr_ext4_engine.cpp:447-475`): `read_ladder` recurses levels 1..3 over
  every nonzero pointer in each indirection block — **no file-size bound, no node
  budget, no extent-count cap** (the per-inode budget at :426 gates only the
  extents path). Worst case per inode ≈ 1 + per_block + per_block² + per_block³
  block reads (4 KiB blocks: ~1.07e9 reads) and up to per_block³ pushed extents
  (~17 GB of extent vector) — triggered by a single deleted ext2/3 inode whose
  stale i_block[13]/[14] pointer now targets reused data (e.g. a block full of
  0xFFFFFFFF). The deleted-inode sweep calls `read_inode` for every inode
  (:1063-1071), so one poison inode freezes the scan or exhausts memory.
- **Og**: the indirect path translates logical→physical per block with the loop
  bounded by the file's size (FUN_180005490 :3949-3998 `param_4 <= lVar9`,
  `lVar9 += block_size`; content loop FUN_1800068b0 :5021-5024 `lVar16 < size`) —
  the og never walks a stale tree past EOF.
- **Gap (falsifiable)**: image with one inode, mode regular, no EXTENTS_FL,
  i_size small, i_block[14] → a block of 0xFFFFFFFF: core issues ~10⁹ reads
  (measure via a read-counting device); og reads at most size/block_size blocks.
- **Impact**: freeze/OOM on corrupt or stale-pointer ext2/ext3 images — the recovery
  threat model. Core-only defect (the core is the *less* safe side here).
- **Fix direction**: bound the ladder by the inode's logical size (walk only
  logical blocks < ceil(size/bs), like the og) or by the per-inode node budget
  already used for extents; cap the extent count per inode.

### F-E4 · MAJOR · ext4 · large-volume refusal caps the og does not have (8 MiB GDT read cap; MAX_GROUPS)

- **Core**: `cr_ext4_engine.cpp:720-722` — `gdt_bytes > 8 MiB → ERR_BAD_PARAMETER`
  (gdt_bytes = groups × desc_size): a 64BIT volume with 64-byte descriptors is
  refused above **16 TiB** (131072 groups × 64 B = 8 MiB exactly at 16 TiB; the check
  is `>`, so 16 TiB passes and anything larger fails); 32-byte descriptors refuse
  above 32 TiB. Additionally `validate_superblock` rejects `groups > MAX_GROUPS
  (1<<20)` (:162) — volumes above 128 TiB @4 KiB. Neither cap is in PARITY-DELTAS
  (AUDIT.md row 204 references MAX_GROUPS only as a fixture bound).
- **Og**: mount init allocates `round_up(groups*desc_size, block_size)` with no cap
  (FUN_1800047c0 :3406-3408 — a 100 TiB 64BIT volume allocates a ~52 MiB GDT buffer
  and scans); the validator has no group-count cap (:3236-3239).
- **Gap (falsifiable)**: mke2fs a >16 TiB 64BIT ext4 image (or synthesize the SB/GDT
  geometry): og scans; core returns ERR_BAD_PARAMETER (whole volume skipped).
- **Impact**: legitimate large ext4 volumes (>16 TiB — big NAS arrays) get no
  engine-side recovery in the core while the og scans them.
- **Fix direction**: raise the caps to memory-safe streaming (read the GDT in
  bounded chunks, or keep a cap aligned to the true max volume, e.g. groups ≤ 2²⁶) —
  or register the refusal as a deliberate divergence with the threshold in
  PARITY-DELTAS. A 52 MiB GDT allocation is unremarkable on 64-bit hosts; the 8 MiB
  constant looks like a hardening default that became a capability cliff.

### F-E5 · MINOR · ext4 · unwritten-extent ee_len: og multiplies the raw u16 (int overflow → garbage run length); core decodes per kernel — register the non-adoption

- **Og** (FUN_180005670 :4044-4048, the extent→datarun builder used by the file
  properties path): `len = (int)((uint)(ushort)ee_len * block_size)` then sign-extend
  and clear bit 63 — for ee_len ≥ 0x8000 (kernel unwritten extents: bit 15 set) the
  32-bit product overflows into the sign bit and the run length becomes
  garbage-huge (~0x7FFFxxxxxxxxxxxx). No masking anywhere in the function.
- **Core** (`cr_ext4_engine.cpp:345-346`): `raw_len > 0x8000 ? raw_len & 0x7FFF :
  raw_len` — kernel `ext4_extents.h` semantics (unwritten bit stripped, 0x8000
  itself = written 32768).
- **Gap (falsifiable)**: image with a fallocate-style unwritten extent
  (ee_len = 0x8064): og emits a garbage-length run; core emits 100 blocks.
- **Impact**: parity benches on volumes with unwritten extents (fallocate,
  databases, torrent preallocation) will show og/core run-length differences; the
  core is kernel-correct. Register-class finding (same family as I-040/D-123's
  registered og-defect non-adoptions) so the divergence is on the books.
- **Fix direction**: no code change; add to the og-defect non-adoption register
  (cite :4044-4048). If og-parity mode is ever wanted, gate behind a flag — not
  recommended (the og's runs are wrong).

### F-E6 · MINOR · ext4 · superblock validator acceptance-band deltas (consolidated garbage-gate differences)

- **inode_size == 0 with rev_level ≥ 1**: og rejects
  (`(rev==0) || (short)inode_size != 0`, :3236); core falls back to 128 and accepts
  (:172-174).
- **log_block_size 7..20**: og accepts (block sizes 128 KiB..1 GiB pass the int-sign
  check at :3232-3233); core rejects `log_bs > 6` (:148-149). Real volumes never
  exceed 64 KiB, but the acceptance bands differ on garbage SBs.
- **journal_inum**: og copies s_journal_inum only when feature_compat HAS_JOURNAL
  (bit 2) is set (:3365-3367 — 0 otherwise → journal pass skipped); core reads it
  unconditionally and defaults to inode 8 when 0 (:177-179), probing inode 8 on
  non-journal volumes (harmless in practice — the jsb magic gate filters — but a
  behavioral delta: on an ext2 volume whose inode 8 is a user file starting with
  jbd2-looking bytes the core could salvage from a non-journal).
- **desc_size combos**: og clamps s_desc_size to [32,64] unconditionally for the
  front GDT (:3397-3405); core uses 64 only when 64BIT, else 32 (:191-199) — differs
  only on invalid combos (64BIT+ds=32 → og 32/core 64; !64BIT+ds=64 → og 64/core 32).
- **Impact**: garbage-SB acceptance band differences (both directions) with
  near-zero real-volume impact; filed so the bands are documented and the deliberate
  choices can be registered rather than accidental.
- **Fix direction**: either adopt the og gates (reject rev≥1+inode_size==0; drop the
  log_bs ≤6 cap to the og's effective ≤20) or register the strictness deltas in
  PARITY-DELTAS; make journal_inum compat-gated for exact parity.

---

## UNRESOLVED (precise next probes)

1. **Btrfs og FS-tree descent guard census.** FUN_1800023f0 checks the claim bitmap
   (FUN_180007260, :990-991) before processing each node — determine whether claimed
   blocks are *inserted* per processed node (making the bitmap a de-facto visited set
   and cycle guard) or only populated by the extent-claim map (no cycle protection).
   Probe: walk FUN_18000d700's callers from FUN_180002120/FUN_180006b90 and find every
   writer of the bitmap objects at param_1+0x240/+0x248. Decides whether the og has
   ANY cycle guard on metadata-tree walks (affects the Axis-4 table's "og: none
   found" line).
2. **ZFS og nvlist internal type enum.** FUN_180018730's entry records classify
   `vdev_tree` as type 9 (:21089) and read uint64s from type-2 entries BE
   (:21113-21141), while OpenZFS XDR uses 19=NVLIST and 8=UINT64. Decode
   FUN_180018730's type-field assignment to map the og's internal enum onto the
   on-disk encoding; confirms the core's XDR type handling is result-equivalent on
   all label configs (it is OpenZFS-correct; the og demonstrably works on real pools,
   so its enum must be a fixed remap — the mapping just isn't decoded).
3. **ext4 journal-inode emission chain (feeds F-E2's severity).** From FUN_180005e50's
   file-branch inserts (+0x4a8/+0x4b8/+0x4c8, :4443-4573), trace which map the
   FUN_1800041e0 finalize (:2982-2993) and the FUN_180003fb0 reachability check
   consume, and confirm the journal-harvested inode records reach the result
   callback (vs. only feeding the parent-map). A 30-minute decompile walk; upgrades
   F-E2 to SEVERE if the emission is direct.
4. **ext4 og unwritten-run downstream clamping (bounds F-E5).** Check whether the og's
   CExt4DataRunList consumers clamp the garbage run length from FUN_180005670
   (:4044-4048) to the volume size (making the observable "volume-sized run" instead
   of "astronomical"). Probe the runlist clamp sites in the NTFS-runlist classes
   shared by the ext4 engine.
5. **HFS+ og Traverse recursion + core leaf-chain strategy.** The og's
   CHFSPlusBTree::Traverse recursion (FUN_180013de0 :15587) has no depth cap —
   confirm the stack-overflow risk shape (self-referential index node) for the
   og-defect register; and decide whether the core's leaf-chain-vs-root-descent
   strategy difference (superset on damaged index nodes) should become a registered
   D- entry rather than an unregistered implementation choice.
6. **ReFS og-side first-hand probe verification.** The core's superblock backup
   positions (clusters 30, n-3, n-2) and the checkpoint/volume-sig math rest on
   AUDIT-LINUX-FS pins; re-verify FUN_180012390's caller chain in
   ReFSSearchDLL_decompiled.c for the SUPB probe order + any additional backup
   positions the og tries that the core doesn't, and decode the og-side table-walk
   guards for the Axis-4 table (the core's visited-set/depth caps currently have no
   og counterpart decoded).
7. **RAW og block-scan windowing.** The og block-scan caller of FUN_18000c820 (chunk
   size fed as param_2/param_3, the buffer at param_1[3]) was not fully decoded;
   derive the og's read-chunk geometry (1 MiB? ping-pong 4/8 MiB?) to complete the
   D-115/M-025 window-model record.
8. **Corpus-doc errata to file upstream** (not core defects): (a)
   `hfsplus-engine--deep-dive.md` §1 fork-clamp misread (see F-E1);
   (b) `btrfs-engine--deep-dive.md` §3 "mirror copies at 0x40000" — .rdata says
   0x4000000 (64 MiB); (c) `zfs-vdev--mftmirr--calcpartition.md` §1 swapped
   offset/size on the vdev_phys read (offset 0x4000, size 0x1C000 — proven by the
   0x1BFFC parse limit); (d) `zfs-engine--deep-dive.md` "0x80-byte uberblock ring
   stride" — the pointer arithmetic (`plVar8 - 0x80` on a longlong*) is 0x400 bytes,
   matching the stride at :5841.
