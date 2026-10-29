# AUDIT-LINUX-FS — ext4 / HFS+ / Btrfs / ZFS / ReFS deep-parity audit

Task ID: **Refactor-E** · READ-ONLY audit (no source modified, no git state changed).

## 0. Pins

| Item | Pin |
|---|---|
| core_recovery | @ 2ea5d457ee67c7bcc490ce1769b0e15bb7012873 (main) |
| easeus-drw-research (RE corpus) | @ 18d6f14a1add9fc761ed7b3732bb1715b15aa79f (main) |
| Ours (read fully) | `src/core/{ext4,hfsplus,btrfs,zfs,refs}/*.cpp`, `include/cr/{ext4,hfsplus,btrfs,zfs,refs}/*.h` (5,108 LOC) |
| Corpus artifacts (read fully) | `re-workspace/artifacts/SearchDLL/{ext4,hfsplus,btrfs,zfs,refs}-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md`, `refs-btrfs-zfs-engine-algorithms-v2.md`, `zfs-vdev--mftmirr--calcpartition.md`, `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`, `engine-matrix--factory-contract-verification.md`; `MOM-modules/{ExtFileSystemAnalyser,RefsFileSystemAnalyser}.md` |
| Decompiled (FUN_ pins, ±context) | `re-workspace/reversed/ghidra-searchdll/{EXT4,HFSPlus,Btrfs,Zfs,ReFS}SearchDLL_decompiled.c` (124,635 LOC total for the five) |
| Tests (read) | `tests/unit/test_{ext4,hfsplus,btrfs,zfs,refs}_volume.cpp`, `test_refs_ldm.cpp` |
| FS codes (DRWUI factory map, engine-matrix artifact §2) | 4-5=EXT2/3, 8=HFS+, 9=EXT4, 10=ReFS, 13=ZFS — ours: ext4→9 ✓, hfs→8 ✓, refs→10 (`REFS_FS_KIND` ✓), zfs→13 ✓; btrfs→11 (code comment cites "row 45"; matrix does not list Btrfs's dispatch code — UNVERIFIED) |

**Methodology**: line-by-line read of our five engines + headers; corpus artifacts as the claim index; every checklist item re-derived against the decompile (Grep for FUN_ addresses / magic constants, Read ±150 lines); prior-audit claims (F-A2-2/3/5/12/13/14/17/18, D-05/07/09, G-18) re-verified against current code and fresh decompile pins; dead/duplicate code via repo-wide symbol-count greps; stale "ReFS report-only" language sweep across src/include/docs.

---

## 1. ext4 (`cr_ext4_engine.cpp`, 866 LOC)

### 1.1 Parameter table (ours vs og vs kernel)

| Parameter | Ours | og (decompile pin) | Verdict |
|---|---|---|---|
| SB magic 0xEF53 @+0x38 | ✓ | FUN_180004710:3222-23 (`(short)param_1[0xe] == -0x10ad`) | MATCH |
| inodes_count@0x00, blocks_lo@0x04, bpg@0x20, ipg@0x28 ≠ 0 | ✓ | FUN_180004710:3224-25 | MATCH |
| blocks_hi@0x150 (64-bit count) | ✓ | `CONCAT44(param_1[0x54],…)` :3234 | MATCH |
| first_data_block@0x14 × log_block_size@0x18 cross rules | ✓ (fdb==0∧log==0 reject; fdb==1∧log≠0 reject) | :3226-31 | MATCH |
| block_size = 1024<<(log&0x1F) | ✓ | `0x400 << (log & 0x1f)` :3232 | MATCH |
| incompat low-bit (compression) guard | ✓ (`sb[0x60]&1`) | `(*(byte*)(param_1+0x18)&1)==0` :3223 (int*+0x18 = sb+0x60) | MATCH |
| rev_level@0x4C, inode_size@0x58 (clamped 128..1024, &7) | ✓ | rev==0 ∨ inode_size≠0 gate :3236 | MATCH (equivalent sanity) |
| GDT @ first_data_block+1, desc 32/64B, hi words @0x20/0x24/0x28 | ✓ | FUN_1800047c0: desc_size u16@SB+0xFE (via +0xF8 flag path, stored min 1) | MATCH |
| Inode: mode@0, size lo@4/hi@0x6C (regular only), atime/ctime/mtime/dtime@0x08-0x14, links@0x1A, flags@0x20, i_block@0x28 | ✓ | flags 0x80000 extents test :12190; size hi @inode+0x6C :12187 | MATCH |
| extra-time fields (crtime@0x90, *_extra epoch/nsec decode) | ✓ kernel-exact | (not og-pinned) | SUPERSET |
| Extent tree: magic 0xF30A, entries@+2, depth@+6, 12-B entries, ee_len 0x8000 unwritten decode | ✓ | CExt4DataRunList (class census) | MATCH |
| Dir entries: inode, rec_len@4, name_len@6, type@7 (FILETYPE), majority-valid gate | ✓ | (family convention) | MATCH |
| Deleted heuristics: dtime≠0 ∨ links==0, whole-table sweep, block-bitmap grading | ✓ | v1 artifact: "orphan-file/list handling for deleted inodes" | MATCH |
| BIGALLOC | refused ERR_UNSUPPORTED (documented) | og has **no** bigalloc check (would mis-map) | ours safer; divergence documented |

### 1.2 Findings — ext4

- **E-1 · INFO · VERIFIED-MATCH** — Superblock gate is field-for-field identical to og FUN_180004710. *Acceptance:* `Ext4Engine.SuperblockValidation` (existing, green).
- **E-2 · LOW · MATCH-WITH-DIVERGENCE (D-07 PIN DELIVERED)** — jbd2 journal. **Og behavior pinned:** `FUN_18000e5b0` (EXT4SearchDLL_decompiled.c:12159-12260) and `FUN_18000e890` (:12265-12368): read journal superblock; gate = BE magic `0xC03B3998` **and blocktype ∈ {3,4}** (jsb V1/V2); then read s_first (jsb+0x14)…s_maxlen (jsb+0x10) and feed **every block in [s_first, s_maxlen)** to the directory-block parser (`FUN_1800043f0`; "GotJournalDir/TraverseJournalDir" traces). **Og implements NO descriptor/commit/revoke semantics** — revoke blocks are not filtered (a REVOKE block simply fails the dir-parse). Ours parses descriptor/commit blocks, salvages only **committed** transactions, and deliberately surfaces revoked blocks (documented divergence). Net asymmetry: our commit gate drops uncommitted-tail dir blocks that og would surface. *Acceptance:* extend `Ext4Engine.JournalSalvage` with an uncommitted descriptor+data block whose dir entries og would surface; assert behavior matches the chosen policy and the divergence note stays truthful.
- **E-3 · LOW · UNVERIFIED (kernel re-derivation)** — our jbd2 tag-width table (V3: 16 B, flags BE32@+4; 64BIT: 12 B, flags BE16@+6; CSUM_V2: +2 → 14) is not kernel-verified; kernel `journal_tag_bytes()` returns 12 for csum_v2/v3 tags. Og parses no tags at all (see E-2), so this is fidelity-beyond-parity, but the widths should be re-derived from `fs/jbd2/journal.h`. *Acceptance:* unit test with a hand-built CSUM_V2 journal fixture compared against `jbd2_logdump` output.
- **E-4 · INFO · PARITY-NEUTRAL** — backup superblock/GDT: deep-dive artifact §2 claims og probes backups at "8193, 32768…", but the decompile contains **no** such constants (grep 0x2001/8193/32768: only unrelated VirtualFree hits). Both og and ours are primary-only. Artifact claim moved to the UNVERIFIED register. (If recovery-grade backup salvage is wanted, it would be a superset, not a parity debt.)
- **E-5 · INFO** — CatchBuffer bad-sector proxy (og `CEXT4CatchBufferDeviceProxy`, 0xE00 buffer / 0x400 backtrack) is delegated to our transport retry layer — documented in the header; behavior-equivalent salvage, architecture divergence accepted.
- **E-6 · LOW · GAP (superset target, not og debt)** — dir_index/htree: we treat directories as linear streams (works for dx dirs since linear rec_len chains persist); og's htree-specific handling is class-name inference only (UNVERIFIED).

**ext4 MATCH%: 90** (bootstrap, GDT, inode/extent/dir/deleted/journal chain all pinned; deductions: journal asymmetry E-2, unverified tag widths E-3).

---

## 2. HFS+ (`cr_hfsplus_engine.cpp`, 693 LOC)

### 2.1 Parameter table

| Parameter | Ours | og (decompile pin) | Verdict |
|---|---|---|---|
| VH sig 'H+'(0x482B)/'HX'(0x4858) @1024+0, version<0x11 @+2, blockSize%512==0 @+0x28, **freeBlocks≤totalBlocks @+0x30, journalInfoBlock≤totalBlocks @+0x0C** (M-142: the fork clamps were a doc-borne misreading — the og never gates the forks; FUN_18000cf90 is the fork byte-swapper, not a gate) | ✓ (M-142 FIXED) | HFSPlusSearchDLL:10005-10006 (`local_218==0x482b‖0x4858`, `&0x1ff==0`, `<0x11`, `local_1e8<=uStack_1ec` [freeBlocks], `uStack_20c<=uStack_1ec` [journalInfoBlock]) | MATCH |
| VH forks: alloc@0x70, extents@0xC0, catalog@0x110 | ✓ | (hfsplusvolume.cpp flow) | MATCH |
| Node kinds: −1 leaf, 0 index, 1 header, 2 map; **sign-extended kind byte** | ✓ (`i16(i8(node[8]))`) | FUN_180010720:13068-13078: node size ≥0x200 & %512==0; `3 < (byte)((char)kind+1U)` ⇒ kind∈{−1,0,1,2}; kind==1 header | **F-A2-17 FIXED & VERIFIED** |
| BTHeaderRec (after 14-B node descriptor): treeDepth@0, rootNode@2, leafRecords@6, firstLeaf@0x0A, lastLeaf@0x0E, **nodeSize@0x12** | ✓ | FUN_180012160:14335-50 — node0 bootstrap reads qwords at node+0x0E/0x16/0x1E/0x26…; `sVar9 = local_88._2_2_` = qword@(node+0x1E) bytes 2-3 = **BTH+0x12 = nodeSize** | **F-A2-18 FIXED & VERIFIED** |
| Catalog record: recType@0, fileID@8, create@0x0C, contentMod@0x10, attrMod@0x14, access@0x18, textEncoding@0x50, **dataFork@0x58, rsrcFork@0xA8, size 0xF8**; folder: valence@4, folderID@8, ≥0x58 | ✓ | FUN_180009940:7648-7686 — type 1/2 at rec+0 (i16); fileID@+8; dates @+0x0C/0x10/0x18; flag byte @+0x52 & 0x20 selects rsrc; **dataFork@rec+0x58** (`plVar10 = param_6+0x2c` shorts), logicalSize@fork+0, totalBlocks@fork+0x0C (<<0xC fallback), **rsrcFork@rec+0xA8** (`param_6+0x54` shorts) | MATCH (248-B TN1150 record with Finder-info region — re-derived, both agree) |
| Catalog key: keyLength@0 (excl. self), parent@2, nameLen@6, UTF-16BE@8 | ✓ | FUN_180009940:7616-33 (leading-zero name skip, 255 cap) | MATCH |
| Unicode fast-compare table | N/A (full leaf-chain walk, no key search) | FUN_18000ecf0:11783-11810 + DAT_180028040 two-stage fold | parity-neutral (same result set) |
| Fork model: data + resource forks, `..namedfork/rsrc` children | ✓ (both forks always) | og switches to rsrc-as-primary when Finder flag 0x20 set | ours = superset |
| Extents-overflow B-tree (CNID 3): preloaded map, forkType 0 / 0xFF | ✓ | CHFSPlusExtents ("8-overflow forks followed") | MATCH |
| Orphan sweep: freed leaf nodes outside live chain, alloc-bitmap (MSB-first) grading | ✓ | deep-dive §2: "orphaned-cnid sweeps and the RAW carving engine" | MATCH |
| BSD info (permissions/uid/gid) | not read | og record parser skips +0x20..0x4F too (FUN_180009940) | MATCH |

### 2.2 Findings — HFS+

- **HFS-1 · VERIFIED-FIXED (P1 closure confirmed)** — F-A2-17/F-A2-18 both fixed and now decompile-pinned (table above). Fixtures were corrected in the same wave (leaf kind=0xFF, header kind=1, BTHeaderRec at TN1150 offsets — test_hfsplus_volume.cpp:120/166/169-176).
- **HFS-2 · LOW · GAP** — Attributes B-tree not walked (`VH_ATTRIBUTES_FORK` is a dead constant). og ships `CHFSPlusAttribute` (named forks, **decmpfs** compressed-file content). HFS+ compressed files (frontmatter-only data fork) recover as metadata-only in ours. *Acceptance:* fixture with a decmpfs-style attribute record; assert either content recovery or an explicit "compressed-HFS+ unsupported" refusal path.
- **HFS-3 · LOW · GAP** — HFS+ journal replay absent (og: hfspluslib `journal.cpp` + `CHFSPlusFileCache/Rawfile`). Recent-state recovery from the journal is not attempted. *Acceptance:* journaled fixture where catalog is stale vs journal; assert documented behavior.
- **HFS-4 · INFO** — orphan sweep caps at 65,536 nodes (`sweep_limit`) — large-catalog volumes get partial deleted coverage; not og-pinned either way.

**HFS+ MATCH%: 85** (bootstrap/catalog/forks/overflow/orphans all pinned; deductions: attributes B-tree, journal replay).

---

## 3. Btrfs (`cr_btrfs_engine.cpp`, 743 LOC)

### 3.1 Parameter table

| Parameter | Ours | og (decompile pin) | Verdict |
|---|---|---|---|
| Mirrors {0x10000, 0x4000000, 0x4000000000, 0x4000000000000} (rdata DAT_180018d60) | ✓ | FUN_18000d540:10284-10316 walks `DAT_180018d60` 0-terminated table | MATCH |
| magic `_BHRfS_M`@+0x40, sectorsize pow2@+0x90, nodesize band 0x86..0x10000@+0x94, nodesize%sectorsize==0, **newest generation@+0x48 wins** | ✓ | :10298-10307 | MATCH |
| sys chunk array @+0x32B, size u32@+0xA0, entry=[key 17][chunk 48+32n], chunk key type 228 | ✓ | :2035-2063 (`uVar7+0x32b+lVar1`, `*(int*)(lVar1+0xa0)`, `(char)*plVar9 != -0x1c`) | MATCH |
| Chunk item: length@0, type@0x18, num_stripes@0x2C, stripe0 offset@+56; single-device stripe-0 mapping | ✓ | FUN_18000dac0 (sys-array chunk parse) | MATCH (single-disk scope; og multi-dev extent unknown) |
| btrfs_header: fsid@0x20, bytenr@0x30 (self-ref check), chunk_tree_uuid@0x40, generation@0x50, nritems@0x60, level@0x64, items@0x65 | ✓ | (node walk family) | MATCH |
| Items 25 B {key 17, offset u32@17, size u32@21}; key_ptr 33 B | ✓ | FUN_18000d700: item stride 0x19 @ node+0x65 :10417-29 | MATCH |
| ROOT_ITEM: bytenr@0xB0, root_dirid@0xA8, level@0xEE | ✓ | (ROOT_ITEM 0x84 handling :4019) | MATCH |
| EXTENT_DATA (108): type@+20 ∈ {0 inline, 1 reg, 2 prealloc}, disk_bytenr@+21, disk_num@+29, size≥53 for regular | ✓ | :3785-3827 — type∈{1,2} requires size≥0x35 ∧ bytenr≠0 ∧ num≠0 else skip; **type 0 (inline) collected with {objectid, size, data}** | layout MATCH, content GAP (BTR-3) |
| dir_item: location@0, transid@17, data_len@25, name_len@27, type@29, name@30, packed records advanced by 30+name_len+data_len | ✓ | corpus parity note (og advances data_len+30+name_len); xattr walk reads name_len@+0x1B, data_len@+0x19 :3424-3444 | MATCH |
| inode_item: size@0x10, nlink@0x28, mode@0x34, timespecs atime@0x70/ctime@0x7C/mtime@0x88/otime@0x94 | ✓ | (kernel layout, og-consistent) | MATCH |
| **chunk_tree_uuid source** | SB+0x60 (**= log_root in the kernel SB — bogus**) | og performs **no chunk-tree-uuid validation at all** (only two memcmps in the whole decompile, both name compares; the og SB+0x70 read at :2017-19 passes a u64 to a device vtable method, not a uuid) | **MISMATCH (BTR-2)** |

### 3.2 Findings — Btrfs

- **BTR-1 · VERIFIED-MATCH** — mirror selection + geometry gates + sys-chunk parse identical to og FUN_18000d540. *Acceptance:* `Btrfs.MirrorSelectionNewestGeneration`, `Btrfs.SuperblockValidation` (green).
- **BTR-2 · MEDIUM · OPEN (F-A2-12, re-pinned)** — `SB_CHUNK_TREE_UUID = 0x60` reads `{log_root, log_root_transid}` as a 16-byte uuid; the kernel superblock has **no** chunk_tree_uuid (fs_info->chunk_tree_uuid is copied from the chunk-root node header @0x40). Effect on real volumes: the candidate path `read_node(cand, /*chunk_tree=*/true)` always fails (expected uuid is zeros/garbage) and the scan proceeds via the fallback "bounded scan of system chunks" — correct results, wasted work, dead primary path. F-A2-12's own "SB+0x70" pin is also wrong (see table). *Fix:* derive the expected uuid from the first self-validating chunk-root node header, then validate subsequent chunk-tree nodes against it (kernel semantics). *Acceptance:* fixture with a real chunk_tree_uuid ≠ 0 at SB+0x60; assert candidate path selects the chunk root directly (no fallback scan), and a wrong-uuid node is rejected.
- **BTR-3 · MEDIUM-HIGH · OPEN (F-A2-13)** — inline extent **content** discarded (`inline_data` flag + `stats.inline_files` only). Og collects type-0 items (item bytes = file content, resident-run model via `CResidentNTFSDataRunList` reuse). Files < ~2 KiB (typical inline threshold) recover as size-correct, content-empty nodes. *Acceptance:* fixture with an inline-extent file; assert recovered content equals the inline bytes (resident_data).
- **BTR-4 · MEDIUM-HIGH · OPEN** — compressed extents unhandled: `FE_COMPRESSION` (declared @+16) is never read; extents are mapped as {phys(disk_bytenr), len(FE_NUM_BYTES logical)}. On zlib/lzo/zstd volumes the on-disk run is `psize` bytes — our extents over-read and the writer emits compressed bytes as raw. Og: EXTENT_DATA handler collects compressed regular extents; zlib capa present in the binary (decompile call sites not visible — see UNVERIFIED-3). *Acceptance:* zlib-compressed fixture; assert decoded content (or extent tagged with codec+psize for writer-time decode, ZFS-style).
- **BTR-5 · MEDIUM · OPEN** — no deleted-file story: `root_deleted`/`root_lost` explicitly `(void)`-cast; ROOT_ITEM `refs`@0xD8 unread (dead/deleted subvolumes not distinguished); orphan items (BTRFS_ORPHAN_ITEM) not read. Og-side deleted behavior is not pinned in the corpus (UNVERIFIED-5) — but our zero-surface is a capability hole for a recovery product. *Acceptance:* dead-subvol fixture (refs==0) → root_deleted emission with viability grading.
- **BTR-6 · LOW · OPEN** — subvolume naming: ours `subvol-<id>` placeholders; og resolves the **default** subvol by root-tree DIR_ITEM name hash (0x8dbfc2d2) + `memcmp("default")` (:3974-4016, 4940-5005). Names for other subvols come from root-tree DIR_ITEM/ROOT_REF — recoverable, skipped. *Acceptance:* fixture with named subvol; assert the real name surfaces.
- **BTR-7 · LOW · OPEN (F-A2-14, downgraded)** — crc32c node-checksum verification absent. Og decompile shows **no crc32 call sites** (capa-only evidence) → parity-neutral absence; keep as P3 polish (kernel-faithful `btrfs_buffer_uptodate` would need csum type from SB).
- **BTR-8 · LOW** — block group items (key type 192) not parsed (only CHUNK); og side not pinned (UNVERIFIED-4). Affects nothing in the current emit path.
- **BTR-9 · INFO** — og persists `BtrfsCache.txt` for re-scans (:1991-2012); ours is stateless — fine for recovery grade.

**Btrfs MATCH%: 70** (bootstrap/chunk/root-tree/inode/dir-item paths pinned MATCH; deductions: inline content, compressed extents, no deleted surface, bogus uuid source, subvol names).

---

## 4. ZFS (`cr_zfs_engine.cpp`, 1,402 LOC)

### 4.1 Parameter table

| Parameter | Ours | og (decompile pin) | Verdict |
|---|---|---|---|
| Labels: L0@0, L1@256K, L2@end−512K, L3@end−256K (4 per vdev) | ✓ | uberblock walk sites :32321/:32365/:35244 (4-label model); the prompt's "LBA0/LBA256(257)" phrasing resolves to the OpenZFS 4-label layout | MATCH |
| nvlist: XDR + native decode, type-9 vdev_tree recursion, BE u64 accumulate | ✓ (spec offset label+16K, all 4 labels) | og reads 0x4000 @ **0x1C000** (label 0), `FUN_180018730` 0x40-stride entries, "vdev_tree" type-9 recursion, children (type 19) recursion :21052-21150 | MATCH (offset divergence documented in header — og's window is internally inconsistent per corpus artifact) |
| Uberblock ring @ label+128K, 1 KiB slots, magic 0x00BAB10C | ✓ (geometry probe shifts 10-13 for 4Kn) | :32321/:32365 0x80-qword stride, magic `0xbab10c` | MATCH |
| Newest selection | **txg only** (u64@+16) | `plVar8[-0x7e] < best` txg compare :32365-70 — **no timestamp tie-break, no checksum verify in og either** | MATCH (checklist question answered) |
| BE pools | reversed magic (0x0CB1BA00/0x0CBA0000) → **ERR_UNSUPPORTED** | (BE accumulate path exists for values; pool-level refusal is our documented scope) | MATCH (error code verified ERR_UNSUPPORTED) |
| MOS: rootbp@ub+40 → objset_phys → object dir ZAP (obj 1) → dataset dir chain | ✓ | deep-dive §2 (UberData/objset/ZplNode) | MATCH |
| dnode: type@0, nlevels@2, nblkptr@3, datablkszsec@8, bonuslen@10, extra_slots@12, maxblkid@16, blkptr@64, bonus tail rule | ✓ | (zfs_struct:: layer) | MATCH |
| ZPL: master node → ROOT → micro/fat ZAP (embedded + block ptrtbl, chunk array walk) | ✓ | (ZplNode/ZplNodeFile) | MATCH |
| SA layout 1 + legacy znode bonus | ✓ | | MATCH |
| **Gang blocks (D-09/G-18)** | `read_gang` (512 B header, **3** blkptrs = (512−32)/128, nested depth 4) + `plan_gang_pieces` (ordered member extents) | og: `CStreamBase` "gang blocks/indirect blocks" — inference only, no strings | MATCH at kernel level; og side UNVERIFIED-6 |
| Embedded BPs (BP_EMBEDDED) | ✓ (etype 0, blkptr.c payload port, ≤112 B) | | MATCH |
| Compression lzjb/zle/gzip/lz4/zstd via `compress::codec_from_zfs`, extent carries {codec, psize} | ✓ | per-object `std::map<int64, zio_compress>` registry (deep-dive) | MATCH |
| Deleted story (D-05) | newest-txg live state only; `root_deleted`/`root_lost` deliberately unused (documented parity decision) | og: no deleted/snapshot machinery in decompile (grep "snapshot"/"zombie": zero hits; deep-dive's snapshot claim was inference) | MATCH (deliberate) |
| Multi-vdev | vdev≠0 DVA skip; `unsupported_vdevs` stat (report-only) | og assembles vdev_tree **children** with id→map (:21107-50) — topology aware | GAP (ZFS-2, scope-documented) |

### 4.2 Findings — ZFS

- **ZFS-1 · VERIFIED-MATCH** — label geometry, uberblock selection (txg-only, exactly like og), BE refusal, nvlist recursion: all pinned. *Acceptance:* `Zfs.LabelPositionsRecoveredFromTail`, `Zfs.NewestTxgAcrossLabelsWins`, `Zfs.BadMagicRejected` (green).
- **ZFS-2 · LOW · GAP (scope)** — multi-vdev topology not assembled (og: children recursion + id map). Ours refuses non-vdev-0 DVAs and reports. Consistent with the G-18/CORE-CONTRACT single-volume scope; note the og is topology-aware. *Acceptance:* two-vdev fixture → `unsupported_vdevs` ≥ 1, no mis-mapped extents.
- **ZFS-3 · LOW · GAP** — gang-block and embedded-BP paths have **no test fixtures** (prior audit F-A2-8 already noted; still true). *Acceptance:* gang fixture (leader + 2 members) and embedded-BP fixture (≤112 B payload, lz4) with content assertions.
- **ZFS-4 · INFO** — uberblock checksum (fletcher/sha256) not verified by either engine; ZFS self-validation instead comes from structure (blk_prop sanity, dnode gates). Parity-neutral.

**ZFS MATCH%: 92** (whole chain pinned; deductions: multi-vdev topology, missing gang/embedded tests).

---

## 5. ReFS — FIRST-CLASS DEEP CHECK (`cr_refs_engine.cpp` 820 LOC + `cr_refs_detect.cpp` 81 LOC)

### 5.1 ReFS first-class capability assessment (explicit answers)

**Is our ReFS engine a FULL MSB-chain engine today? — YES.** `scan_refs` implements the complete chain: VBR gate → superblock page (cluster 30, backups 2nd/3rd-last clusters) → volume signature (GUID XOR) → **dual-clock checkpoint selection** (two CHKP pages, max virtual clock wins) → checkpoint pointer list → **ObjectID table** (tableID → LCN quad) → **parent-child table** (root fallback) → **medium allocator** (bitmap rows, polarity-calibrated grading) → directory tables (root 0x600, fallback: not-a-child) → mini B-tree recursion (inner nodes via child LCN quads in entry values) → embedded file tables (MACE times, sizes) → $DATA data-run tables → deleted-row (flag 4) + **ID2 (0x80000020)** harvest → orphan tables → root_lost. It is dispatched by the orchestrator on both the partition path and the lost-partition candidate path (`cr_scan.cpp:604-608`, `:821-824`) and fuzzed (`fuzz_refs.cpp`). **Nothing in the production path is "report-only"** — the only "report-only" wording left is the detect probe's file-top comment (REFS-8).

**What exactly is missing vs og (mapped to every og behavior found):**

| # | og behavior (decompile pin) | Ours | Severity |
|---|---|---|---|
| 1 | **Next-chain following**: `RefsMetadataBlockRefrenceImpl` parse copies `{valid, block_id, flags@+0x2C, size@+0x30, next@+0x38}` into `RefsBaseDataImplManager` chains (FUN_18000fc80:11716-11731; corpus v2 artifact §1). F-A2-2. | **NOT implemented** — no next traversal anywhere in `cr_refs_engine.cpp`. Nodes read at the referenced (possibly older) version; post-checkpoint updates that live at the chain tail are missed. | **HIGH** |
| 2 | **Version-dependent checkpoint layout**: pointer-list base 0x44 (minor<0xE) vs 0x58 (minor≥0xE) after the header-size body offset (FUN_180012650:13754-58) → page+0x94 vs **page+0xA8**. | Hardcoded ptr count@+0x90, list@+0x94 (= og's minor<14 layout only). **Wrong pointer list on ReFS ≥ 3.7 (minor≥14)**. | **HIGH** |
| 3 | **Version gates**: DBR `major==3 && minor>3` (FUN_180012390:13529-31) and again on the checkpoint body (u16@body+4==3 ∧ u16@body+6>3, FUN_180012650:13744-50); separate major==1 path (page = 16 KiB). Ours accepts 1.x–3.x with no checkpoint version check. | Wider acceptance (deliberate) but **no checkpoint version gate at all**. | MEDIUM |
| 4 | **Container table**: og computes container count = bytesPerContainer@DBR+0x40 / cluster bytes (FUN_180012390:13560) + `CRefsContainerTable` (.mo analyser). | `container_lcn` map exists but is **never populated**; container-tag VCNs are masked (single-container assumption). Multi-container (3.5+) volumes mis-resolve. | MEDIUM |
| 5 | **Security descriptor / system identity directories** (System Dir 0x0-0x5; .mo `CRefsMetaSymbolicDirItem`, `CRefsLogFileProc`, `CRefsVolumeInfoProc`). | No security table walk, no symlink resolution, no log-file walk. | MEDIUM |
| 6 | **Resident dataruns**: `CRefsResidentDataRunList` is the one per-FS runlist class DRWUI re-materializes by name in .rsf (deep-dive §1) — resident $DATA is a first-class og result type. | $DATA handled only as embedded data-run table (works for the common node-shaped value); header documents 0xB0 $NAMED_DATA / 0xC0 $REPARSE_POINT but **code only handles 0x80** — ADS and reparse points are not emitted (dead doc line). | MEDIUM |
| 7 | Dual-checkpoint compare: `if (chk1.clock@obj+0x10 < chk2.clock) pick chk2` (decompile :1027-1031) — max-clock wins. | ✓ compares page+0x60 u64 clocks, max wins. Clock field provenance on the og side is partially pinned (set outside FUN_180012650). | MATCH (partial pin) |
| 8 | Page signatures "SUPB"/"CHKP"/"MSB"+`ReFSFSRS` dwords (FUN_18000fc80:11688-11701, FUN_18000ff20/ff40). | ✓ `SIG_SUPB/SIG_CHKP/SIG_MSB` + detect module probes all three full strings. | MATCH |
| 9 | LCN quadruples: reference parser appends 4 entries (FUN_18000ff60:11826-11860). | ✓ `read_page` assembles 4 LCNs (zeroed tail ⇒ contiguous). | MATCH |
| 10 | DBR field model (ReFS@+3, FSRS@+0x10, sectorCount@+0x18, bps@+0x20, spc@+0x24, major@+0x28, minor@+0x29, serial@+0x38, bytesPerContainer@+0x40; bps∈{512..4096}, cluster∈{16K,64K}). | ✓ all fields; page geometry (4 clusters @4 KiB / 1 cluster @≥16 KiB) matches og's v1/v3 page-size rules. | MATCH |
| 11 | Volume signature gate on every signature-bearing page. | ✓ (`read_page` rejects SUPB/CHKP/MSB pages with mismatched sig). | MATCH |
| 12 | Sector-granular MSB page scan in the boot region (:5846-5881, +0x200 steps, SUPB/CHKP dispatch). | ✓ detect module (1 MiB, 512-B steps). | MATCH |

### 5.2 Findings — ReFS

- **REFS-1 · HIGH · OPEN (F-A2-2 re-confirmed)** — MSB next-chain not followed (table item 1). *Acceptance:* fixture where a table's root LCN quad points to an old leaf and the reference's next-chain leads to a newer leaf carrying an extra file; assert the newer content is emitted.
- **REFS-2 · HIGH · OPEN (new)** — checkpoint pointer-list offset must be version-dependent (0x94 vs 0xA8 at minor≥14). *Acceptance:* fixture pair — same volume at minor 6 (list@0x94) and minor 14 (list@0xA8); both must walk the ObjectID table.
- **REFS-3 · MEDIUM · OPEN (F-A2-3/F-A2-5 resolved)** — stale/incorrect og pins in our comments: `cr_refs_engine.h:38-39` + `cr_refs_engine.cpp:366-368` say og "gates on ReFS 3.7-3.10 only" — **decompile truth: major==3 && minor>3 ⇒ 3.4+** (FUN_180012390:13529-31, FUN_180012650:13744-45); the cited `FUN_180013420` **does not exist** (real DBR gate = FUN_180012390); `cr_refs_engine.h:5` cites `docs/formats.md` which **does not exist**. *Acceptance:* grep-level: no "3.7-3.10", no FUN_180013420, no docs/formats.md references remain.
- **REFS-4 · MEDIUM · OPEN** — container table not built (table item 4). *Acceptance:* two-container fixture with tagged VCNs; assert resolution through container 1's start LCN.
- **REFS-5 · MEDIUM · OPEN** — security/symlink/log surfaces absent (table item 5); ADS/reparse attributes documented but unimplemented (table item 6). *Acceptance:* $NAMED_DATA fixture emits an ADS child; symlink row emits a reparse-point node.
- **REFS-6 · LOW · OPEN (F-A2-5 test mandate, partial)** — no test builds a multi-level mini B-tree, a dual-clock compare, or exercises the superblock backup path (2nd/3rd-last clusters). Existing tests cover boot validation, hierarchy, deleted+ID2 grading, orchestrator probe+scan. *Acceptance:* three new fixtures (multi-level, clock compare, primary-SUPB-corrupt→backup).
- **REFS-7 · LOW · INFO** — "reconstruction of lost files by walking free space": ours covers unreachable *tables* (orphan pass → root_lost) but does not scan free clusters for detached MSB pages (og .mo analyser scope, `CRefsVolumeAnalyser`). Partial.
- **REFS-8 · STALE LANGUAGE · FLAGGED (non-negotiable scope correction)** — `cr_refs_detect.cpp:1` still reads `/* cr_refs_detect.cpp - ReFS detection (report-only). */`. The header (`cr_refs_detect.h:9-13`) correctly scopes it as the lightweight triage probe with the full engine in `cr_refs_engine.h`, and the orchestrator dispatches `scan_refs` — but the file-top line carries the exact stale pattern the session mandate requires flagged (AUDIT-ARCH.md ARCH-5 proposed the reword; not yet applied). Related stale texts seen in docs (LDM-flavored, another auditor's lane, noted in passing): `docs/roadmap.md:42-46` "IN PROGRESS (session 5…)", `docs/ground-truth-contracts.md:85` "(recognition only through v0.2)".
- **REFS-9 · VERIFIED-MATCH** — `REFS_FS_KIND = 10` ✓ (engine-matrix: DRWUI FS code 10 = ReFS).

**ReFS MATCH%: 75** (chain skeleton, page signatures, boot model, dual-clock, ObjectID/parent-child/allocator/dir-table/deleted/ID2/orphan surfaces all present and og-pinned; deductions: next-chain, version-dependent checkpoint layout, stale pins, containers, security/ADS surfaces).

---

## 6. Dead / duplicate code register (five engines; grep-verified usage counts)

| # | Symbol / item | Location | Evidence | Class |
|---|---|---|---|---|
| DC-1 | `RO_COMPAT_METADATA_CSUM` | ext4:40 | 1 occurrence (declaration only) | dead constant |
| DC-2 | `VH_ATTRIBUTES_FORK`, `VH_STARTUP_FORK` | hfsplus:32-33 | 1 occurrence each | dead constants (mark the un-walked attributes B-tree — see HFS-2) |
| DC-3 | `MAX_FORK_BYTES`, `CF_TEXT_ENCODING` | hfsplus:110, 87 | 1 occurrence each | dead constants |
| DC-4 | `FE_COMPRESSION` | btrfs:108 | 1 occurrence | dead constant — **marks the compressed-extent gap BTR-4** |
| DC-5 | `KEY_DIR_INDEX`, `KEY_INODE_REF` | btrfs:65, 62 | 1 occurrence each | dead constants (DIR_INDEX would double-cover hash-collided names; INODE_REF carries hard-link names) |
| DC-6 | `OID_ROOT_TREE`, `MAX_KEY_PTRS` | btrfs:70, 47 | 1 occurrence each (runtime limit recomputed at :267) | dead constants |
| DC-7 | `UB_TIMESTAMP`, `DN_CHECKSUM` | zfs:54, 153 | 1 occurrence each | dead constants (UB_TIMESTAMP marks the deliberate txg-only selection — keep as doc or use) |
| DC-8 | `ZP_GEN/ZP_XATTR/ZP_RDEV/ZP_FLAGS/ZP_UID/ZP_GID` | zfs:211-220 | 1 occurrence each | dead constants (legacy-layout documentation) |
| DC-9 | `NV_DATA_BYTE`, `NV_DATA_UINT64_ARRAY` | zfs:298, 303 | 1 occurrence each | dead constants |
| DC-10 | `FZ_FREEBLK`, `FZ_NUM_LEAFS`, `FZ_NUM_ENTRIES` | zfs:270-272 | 1 occurrence each | dead constants (header-field docs) |
| DC-11 | `refs::detect_refs` | refs_detect.cpp:20 | production call sites: **zero** (tests ×4, fuzz ×1) | unused-in-production public API (triage probe; either wire into partition triage or mark internal) |
| DC-12 | byte-reader duplication | ext4 `rd16/rd32`; hfs `be16/be32/be64`; btrfs `le16/le32/le64`; zfs `rdle*/be*`; refs `le16/le32/le64` | 7 near-identical copies across engines | duplicate code (consolidation candidate, low priority) |
| DC-13 | `unix_to_filetime` (ext4:107) vs `unix_to_filetime` (zfs:1015) vs `timespec_to_filetime` (btrfs:286) | three copies of epoch conversion | duplicate code (consolidation candidate) |

No engine-level dead *functions* found (all walk/parse functions are reachable from `scan_*`); the duplication is constants + byte readers + time conversions.

---

## 7. UNVERIFIED register

1. **ext4 backup-SB probing in og** — deep-dive §2 claims backup superblock positions (8193, 32768…); no such constants in the decompile. Treat the artifact claim as overreach until pinned.
2. **ext4 og htree (dx_dir) parsing** — class-name/convention inference only.
3. **Btrfs og zlib/crc32c verification during scan** — capa digest says the capability exists in the binary; no crc32/inflate call sites in the decompile. F-A2-14 downgraded accordingly.
4. **Btrfs og block-group-item parsing** — not pinned.
5. **Btrfs og deleted-subvol / dead-tree handling** — not pinned (our zero-surface may or may not be parity).
6. **ZFS og gang-block implementation** — inferred from `CStreamBase`; no strings/loops pinned. Our kernel-faithful implementation stands on kernel truth (D-09/G-18 verified at our end).
7. **ZFS og label-nvlist window 0x1C000** — corpus artifact itself flags the og's internal inconsistency; our spec-following divergence is documented.
8. **ReFS og checkpoint-clock field provenance** — the compare (`chk.clock@obj+0x10`, max wins) is pinned; the assignment site of that field was not located in FUN_180012650 (set in a ctor helper or alternate parse path). Our +0x60 on-disk offset follows the thesis/ReadFS model.
9. **ReFS og identifier-directory ↔ checkpoint-pointer-index mapping** — our {0=ObjectID, 1=MediumAllocator, 4=ParentChild} is thesis-derived; the og dispatch uses table-kind bits (:12547-79) not fully mapped to indices.
10. **jbd2 tag widths vs kernel** — see E-3.
11. **Btrfs DRWUI factory code 11** — our comment cites "row 45"; the engine matrix does not list Btrfs's dispatch code.

---

## 8. Summary

| Engine | MATCH% | Headline |
|---|---|---|
| ext4 | 90 | Bootstrap/GDT/inode/extent/dir/deleted/journal all og-pinned; D-07 pinned (og has **no** revoke/commit semantics — walks [s_first, s_maxlen) raw); our commit gate is the one asymmetry. |
| HFS+ | 85 | F-A2-17/18 verified FIXED and re-pinned (nodeSize@BTH+0x12 via FUN_180012160; kind gate via FUN_180010720); catalog record layout (dataFork@0x58/rsrc@0xA8/248 B) og-exact; gaps: attributes B-tree (decmpfs), journal replay. |
| Btrfs | 70 | Bootstrap/mirror/chunk/root-tree/dir-item MATCH; open: inline content (F-A2-13), compressed extents, bogus SB+0x60 chunk_tree_uuid (F-A2-12 re-pinned — og does no uuid check; kernel truth = chunk-root header), no deleted surface, subvol names. |
| ZFS | 92 | Prior "MATCH" confirmed with decompile pins (txg-only selection like og, BE refusal ERR_UNSUPPORTED, 4 labels, gang/embedded/compression per kernel); gaps: multi-vdev topology, missing gang/embedded tests. |
| ReFS | 75 | **Full MSB-chain engine confirmed first-class** (chain, dual clock, ObjectID/allocator/parent-child, deleted/ID2/orphans, orchestrator dispatch). Missing vs og: **next-chain following (F-A2-2)**, **version-dependent checkpoint pointer list (≥3.7)**, container table, security/ADS/symlink/log surfaces, checkpoint version gate; stale "3.7-3.10"/FUN_180013420/docs-formats.md pins and the detect file-top "report-only" comment flagged. |

*Audit complete; no source files were modified. Findings above are the input queue for the next fix wave.*
