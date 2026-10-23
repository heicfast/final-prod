# PARITY-CORE-FS2 — Filesystem Engines Audit (ReFS / ZFS / Btrfs / ext4 / HFS+)

**Task ID:** 2-a2 (retry) · **Agent:** parity auditor (read-only) · **Date:** 2025 (session)

**Pins:**
- Ours: `/home/z/my-project/repos/core_recovery` @ working tree (read-only audit)
- Corpus: `/home/z/my-project/repos/easeus-drw-research` — `re-workspace/artifacts/SearchDLL/*.md` (primary evidence), `re-workspace/reversed/<DLL>/` (targeted Grep only)

**Method:** our code first → corpus artifacts second → targeted Grep in `reversed/` third. Citations: ours `file:line`, corpus `file:line` or `file:§`. UNVERIFIED where corpus silent. Not re-deriving items already verified in core's AUDIT.md (spot-check only).

## Coverage

| Engine | Our code | Test read | Corpus artifact | Reversed Grep | Status |
|---|---|---|---|---|---|
| ReFS | ✅ | ✅ | ✅ | ✅ | DONE |
| ZFS | ✅ | ✅ | ✅ | ✅ | DONE |
| Btrfs | ✅ | ✅ | ✅ | ✅ | DONE |
| ext4 | ✅ | ✅ | ✅ | partial | DONE |
| HFS+ | ✅ | ✅ | ✅ | ✅ | DONE |
| docs/ground-truth-contracts.md spot-check | — | — | — | — | DONE (sampled) |

**Files read (ours):** include/cr/refs/{cr_refs_detect.h,cr_refs_engine.h}, src/core/refs/{cr_refs_detect.cpp,cr_refs_engine.cpp}, tests/unit/test_refs_volume.cpp; include/cr/zfs/*.h, src/core/zfs/cr_zfs_engine.cpp, tests/unit/test_zfs_volume.cpp; include/cr/btrfs/*.h, src/core/btrfs/cr_btrfs_engine.cpp, tests/unit/test_btrfs_volume.cpp; include/cr/ext4/*.h, src/core/ext4/cr_ext4_engine.cpp, tests/unit/test_ext4_volume.cpp; include/cr/hfsplus/*.h, src/core/hfsplus/cr_hfsplus_engine.cpp, tests/unit/test_hfsplus_volume.cpp.

**Corpus artifacts read:** SearchDLL/refs-engine--deep-dive.md; SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md; SearchDLL/zfs-engine--deep-dive.md; SearchDLL/btrfs-engine--deep-dive.md; SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md; SearchDLL/ext4-engine--deep-dive.md; SearchDLL/ext4-hfs-engine-algorithms-v2.md; SearchDLL/hfsplus-engine--deep-dive.md.

**NOT-COVERED (time):** none — all five engines covered; reversed-tree Greps were targeted (not exhaustive per-function diff).

---

## ReFS audit grid

| Axis | Verdict | Notes |
|---|---|---|
| Behavior (bootstrap chain) | MATCH (core) / MISMATCH (chain detail) | VBR→SUPB(cluster 30)→CHKP(dual, clock)→ptr list→ObjectID→dir tables matches og chain CHKP→SUPB→MSB (corpus §4). But og follows block-reference `next` chains; ours does not (F-A2-2). |
| Constants & offsets (DBR) | MATCH | +3 "ReFS", +0x10 "FSRS", +0x18 sectors, +0x20 bps, +0x24 spc, +0x28/+0x29 major/minor, +0x38 serial, +0x40 bytesPerContainer — all verified vs og FUN_180012390. |
| Constants (CHKP pointers) | MATCH | count u32@+0x90, list u32[]@+0x94 (og FUN_180012650 @13763-13776: count at header.size+0x40=0x90, ptrs at header.size+0x44=0x94 for minor<14). |
| Page/node geometry | MISMATCH | F-A2-1: og page size is version-dependent (v1=16KiB, v3=cluster-sized); ours always 16KiB at <16KiB clusters. |
| Version gate | DELIBERATE-DELTA (ours wider) + doc error | og: major==3 && minor>3 ONLY (FUN_180012390 @13530-13533). Ours: major 1..3 (engine:388) — documented wider scope. But our comment misstates og gate as "3.7-3.10" and cites nonexistent FUN_180013420 (F-A2-3). |
| Data model (rows/attrs) | MATCH (thesis-level) | ROW_FILE 0x00010030, ROW_DIR_LINK 0x00020030, ROW_ID2 0x80000020, $DATA 0x80, run flags 0x10 — thesis-cited; corpus silent on exact markers (UNVERIFIED at og level). |
| State machine | MATCH | newest-virtual-clock checkpoint wins (engine:467-468); og same pattern per corpus §4 synthesis. Clock offset +0x60 thesis-cited, og-side UNVERIFIED. |
| Error surface | MATCH | ERR_BAD_MAGIC on boot/SUPB/CHKP/table failures (engine:405,441,472,502,521,788); og marks pages invalid via header.valid flag (FUN_18000fc80). |
| Threading | MATCH (single-threaded engine) | No threads in engine; cancel-token checked per entry (engine:517,770). Corpus: no evidence of threading in ReFSSearchDLL walk. |
| Deleted classification | UNVERIFIED (mechanism MATCH at concept) | ENTRY_DELETED 0x4 + ID2 harvest + allocator grading — thesis-cited; og-side exact flags/grading silent in artifacts. CRefsAllocatorMgr exists only in partition-side RefsFileSystemAnalyser.mo (deep-dive §2). |
| Capability claims honesty | MISMATCH (minor) | Header cites docs/formats.md (nonexistent) and wrong og function/versions (F-A2-3). "Full chain walked" claim true for b-tree recursion, false for block-reference next-chains (F-A2-2). |
| 64KB system area | NOT FOUND / N.A. | No "64KB system area" construct in our code or corpus artifacts. Closest ground truth: og restricts cluster size to exactly {4KiB, 64KiB} and bps∈{256,512,1024,2048,4096} (FUN_180012390 @13546-13551); ours accepts bps∈{512,4096} (engine:371) with any pow2 spc≤128 — mixed wider/narrower, undocumented (F-A2-4). |
| Object-ID table | MATCH (thesis-level) | ptr[0]=ObjectID table, root dir fixed 0x600, parent-child ptr[4] — thesis-cited; og artifact describes 32-byte object IDs w/ hash halves (deep-dive §2) — different granularity, ours keyed by u64 table IDs; UNVERIFIED at og level. |
| Test coverage (mandate) | MISMATCH | F-A2-5: mandate "full MSB-chain present and tested" — b-tree inner-node recursion exists (engine:246-253) but NO test builds a multi-level tree, dual-checkpoint clock compare, or superblock backup path. |

### ReFS findings

**F-A2-1 (P2, Constants/geometry, MISMATCH)** — ReFS 3.x page geometry.
Ours: `src/core/refs/cr_refs_engine.cpp:414-419` — `page_clusters = cluster_bytes < 16384 ? 16384/cluster_bytes : 1` (16 KiB pages for every version when clusters are 4 KiB).
Og: `ReFSSearchDLL_decompiled.c` FUN_180012390 (refs::RefsDbrImpl) @13546-13556 — page-geometry field (DBR object +0x30) = `0x4000 / bytesPerSector` when major==1, but = `sectorsPerCluster` when major==3 (and container field +0x34 = 0x1000 for minor==0, else bytesPerContainer/cluster). Under the only reading consistent with the v1 formula (+0x30 = sectors-per-page), **og uses cluster-sized pages for v3.x and 16 KiB pages only for v1.x** — and the og's DBR gate accepts v3.4+ exclusively (@13530-13533), so v3 geometry is the only one the og ever uses.
Impact: on a real 4 KiB-cluster v3 volume our `read_page` (engine:101-135) reads 4 clusters per page and requires all four `read_cluster` calls to succeed; if the LCN-quad tail entries on v3 point at stale/absent mirrors, we count the page damaged where og (which keeps all non-null LCNs in a fallback list, FUN_18000ff60 @11813-11875) would recover. Node parse itself is bounds-driven so over-reading does not corrupt results.
Suggested fix: derive page geometry from boot major like the og (v1 → 16 KiB; v3 → cluster-sized); treat non-null LCN-quad entries as alternate locations, not concat fragments, for v3.

**F-A2-2 (P2, Behavior, MISMATCH)** — MSB block-reference `next` chain not followed.
Og: `RefsMetadataBlockRefrenceImpl::parse` FUN_18000ff60 @11870-11874 stores `next = qword @ref+0x28` (copied to object +0x38; also artifact `refs-btrfs-zfs-engine-algorithms-v2.md` §1 "copy {valid, block_id, flags(+0x2C), size(+0x30), next(+0x38)}"), and `RefsSupperBlockImpl` FUN_180011770 @12925-12950 even relocates the superblock cluster (default 0x1E) from the parsed reference list. The chain is threaded through `RefsBaseDataImplManager<T>` containers.
Ours: `cr_refs_engine.cpp:101-135` (read_page) validates signature+volume-sig only; no reference parse at page+8, no `next` traversal anywhere in the file (grep "next" → none). Our chain is VBR→SUPB(30/backups)→CHKP→pointer list→LCN quads only.
So: the FULL b-tree under each table IS walked (inner-node recursion engine:246-253), but the og's page-level block-reference chaining (and superblock relocation via reference) has no counterpart. "Stop at the first MSB" is accurate at the block-reference level.
Suggested fix: parse the reference at page+8 (4×u64 + flag@0x22 + next@0x28, size 0x30) and follow `next` when a table's root page is a chained header; at minimum document the delta.

**F-A2-3 (P3, Claims honesty, MISMATCH)** — engine comment misstates the og gate and cites a nonexistent function.
Ours: `cr_refs_engine.cpp:362-365` "og gate (FUN_180013420): … the og build accepts 3.7-3.10 only".
Og: no FUN_180013420 exists in `ReFSSearchDLL_decompiled.c` (grep: 0 hits); the actual DBR gate is FUN_180012390 @13530-13533: `major==3 && minor>3` (i.e. 3.4+, not 3.7-3.10); the same 3/>3 gate is repeated in the checkpoint parser FUN_180012650 @13752-13757.
Suggested fix: correct the comment (gate = v3.4+, function = FUN_180012390).

**F-A2-4 (P3, Constants, DELIBERATE-DELTA candidate, undocumented)** — geometry acceptance band.
Ours: bps ∈ {512,4096} only (engine:371-372); spc any pow2 ≤128 (engine:373-375) → clusters 512B..64KiB accepted.
Og: bps ∈ {256,512,1024,2048,4096} AND cluster size must be exactly 0x1000 or 0x10000 (FUN_180012390 @13546-13551). Ours is narrower on bps (rejects 256/1024/2048-byte-sector media the og accepts) and wider on cluster size (accepts e.g. 512B or 1 KiB clusters the og rejects). The header claims "validates the boot shape and accepts any 1.x-3.x" (cr_refs_engine.h:38-41) but does not document this band difference.
Suggested fix: either adopt og band or document the delta in the header.

**F-A2-5 (P2, Testing, MISMATCH vs mandate)** — "full MSB-chain … tested" not satisfied.
Ours: `tests/unit/test_refs_volume.cpp` — every table is a single root-leaf node (`build_node` always writes flags=0x2, height=0 at lines 110-111; pages written one per table at 323-424). The fixture writes both checkpoint refs to the SAME cluster 100 (lines 253-254) so the newer-clock comparison (engine:467-468) is never exercised; the superblock backup path (engine:433-439) is never exercised; inner-node recursion (engine:246-253) is never exercised.
Suggested fix: add a fixture with (a) a two-level ObjectID table (inner node → leaf), (b) two checkpoints with different virtual clocks, (c) primary superblock damaged → backup path.

**F-A2-6 (P3, Claims honesty, MISMATCH)** — stale doc pointer.
`cr_refs_engine.h:5` cites "see docs/formats.md" — no such file exists in core_recovery/docs/ (verified by ls). The on-disk model lives only in the header comment.
Suggested fix: point at the header itself or create the doc.

**ReFS spot-check of AUDIT.md row 28 (detect version fabrication):** FIXED — `cr_refs_detect.cpp:58-66` now reads real major/minor from +0x28/+0x29 and only falls back to the bare "ReFS" string when the probe window is too small (<0x2A). Consistent with og fields (FUN_180012390 @13536-13539).

## ZFS audit grid

| Axis | Verdict | Notes |
|---|---|---|
| Behavior (bootstrap) | MATCH | labels→config nvlist→uberblock ring (max txg)→MOS→objdir ZAP→dsl_dir chain→per-dataset objset→master node→ZPL walk. Og same shape (artifact §2; remaining-engines §ZFS). |
| Label layout | MATCH (spec) / og-label1..3 UNVERIFIED | Ours: 4 labels {0, 256K, end-512K, end-256K} (engine:1039) = OpenZFS vdev_impl.h. Og: label-0 ring read at absolute 0x20000 (FUN_180018220 call @21365: 128K@128K) ✓ matches; og's other label bases not located in this pass (UNVERIFIED). |
| Config nvlist | DELIBERATE-DELTA (documented) | Og reads a 16 KiB window at absolute **0x1C000** (FUN_180018220(dev,buf,0x4000,0x1c000) @21052; parse limit 0x1BFFC @21054-21056) — the tail of label-0 vdev_phys, NOT the OpenZFS config location (label+0x4000). Ours reads full 112 KiB vdev_phys at label+16K (engine:1048-1049). Delta documented in cr_zfs_engine.h:27-29 + AUDIT §3.4. XDR/BE + vdev_tree child confirmed on og side (memcmp "vdev_tree" @21095, BE byte-accumulation @21140) ✓ ours XDR+native both parsed (engine:332-411). |
| Uberblock ring | DELIBERATE-DELTA (improvement) | Og: fixed 1 KiB slots (0x400 memset/copy loop @21362-21370; stride 0x80 qwords @35226+), backward walk, txg newest-wins (comparisons @35244-35247). Ours: slot size probed shifts 10..13 (engine:1105-1132), forward scan, max txg — handles 4Kn/ashift-12 pools the og's fixed stride handles only accidentally. Magic 0x00BAB10C both ✓ (engine:58/1117 vs decompile 35244). |
| Gang blocks | MATCH (code) / MISMATCH (docs+tests) | read_gang implemented (engine:491-526: 512B header, 3 blkptrs, nested-gang depth 4, hole/embedded skip) + plan_gang_pieces emits ordered member extents (engine:871-912). BUT untested (no gang fixture in test_zfs_volume.cpp) and G-18 still says "gang blocks counted damaged" (F-A2-7/F-A2-8). |
| Single-vs-multi-vdev | MATCH (per G-18 scope) | read_raw refuses vdev≠0 (engine:439-440); other-vdev L0s skipped in extent plan (engine:968-969); vdev_type≠"disk" → stats.unsupported_vdevs++ report-only (engine:1144-1145). Consistent with G-18 "multi-vdev detected+reported unsupported". |
| Compressed codecs | MATCH (superset) | off/on/inherit/empty passthrough + lzjb(3)/gzip(5..13)/zle(14)/lz4(15)/zstd(16) (engine:72-76, 466-483; CodecMapping test @778). Og: lzjb/gzip per artifact §4 + std::map<int64,zio_compress> registry (remaining-engines:81). Ours adds lz4/zstd — superset, no og contradiction. |
| SA vs znode | MATCH (ours) / og UNVERIFIED | Both handled: SA layout 1 (magic 0x2F505A, layout@+4, engine:1270-1286) and legacy znode_phys (engine:1287-1299). Og ZplNode/ZplNodeFile RTTI known; SA-layout handling not evidenced in artifacts. |
| BE pools refused | MATCH | magic read reversed (0x0CB1BA00/0x0CBA0000) → ERR_UNSUPPORTED (engine:1118-1120); G-18 claims it; header claims it (cr_zfs_engine.h:23-25). |
| Checksum verification | MISSING | Og: "checksum-aware reads" / "checksum verify" (zfs-engine--deep-dive.md §2, §4). Ours: blk_cksum (bp+88..119) never read; damaged counted only on I/O or decompress failure (engine:452,480-481). F-A2-9. |
| Deleted classification | MATCH (deliberate) | Live-state only; flags.keep_deleted and root_deleted intentionally unused (engine:1026-1027); documented parity decision D-05 (AUDIT row 26: og also reads newest-txg live state, carving covers deleted). |
| State machine | MATCH | newest-txg uberblock across labels/slots (engine:1124-1128); tested (NewestTxgAcrossLabelsWins @735). |
| Error surface | MATCH | ERR_BAD_PARAMETER (size<2 labels, engine:1033-1034), ERR_BAD_MAGIC (no ub / MOS / objdir, engine:1135,1150,1154), ERR_UNSUPPORTED (BE), ERR_CANCELLED (1217). |
| Threading | MATCH | Single-threaded, cancel-checked (1174,1216,1320). Og: delay-execution polling for long walks (artifact §3) — ours lacks the retry/poll layer (P3, minor). |
| Test coverage | MISMATCH (gap) | Labels-from-tail ✓ (720), newest-txg ✓ (735), embedded bps ✓, zle ✓, codecs ✓ — but NO gang-block test, NO multi-vdev report test, NO BE-refusal test. F-A2-8. |

### ZFS findings

**F-A2-7 (P2, Claims honesty / docs staleness, MISMATCH)** — G-18 still claims "gang blocks counted damaged".
Ours (code): `cr_zfs_engine.cpp:491-526` assembles gang blocks (512 B header, 3 member blkptrs, nested recursion) and `cr_zfs_engine.cpp:871-912` emits member extents for file data — gang content IS recovered.
Stale claim: `repos/rda-wizard/docs/CORE-CONTRACT.md:707` (G-18): "ZFS: single-vdev pools only; multi-vdev detected+reported unsupported; BE pools refused ERR_UNSUPPORTED; **gang blocks counted damaged** | by-design".
Already flagged as U-6 in PARITY-DELTAS-draft.md but NOT yet applied. Suggested fix: update G-18 to "gang blocks assembled; mirror/raidz multi-vdev report-only" (the deltas draft's own wording).

**F-A2-8 (P2, Testing, MISMATCH)** — gang-block path has zero test coverage.
`tests/unit/test_zfs_volume.cpp` TESTs: EndToEndWalk, LabelPositionsRecoveredFromTail, NewestTxgAcrossLabelsWins, BadMagicRejected, ZleRoundTrip, CodecMapping (lines 648-792); no fixture sets the DVA gang bit (grep "gang" → 0 hits in test). read_gang/plan_gang_pieces (engine:491-526, 871-912) are untested — combined with F-A2-7 the capability is simultaneously under-documented and under-tested while the code moved ahead.
Suggested fix: fixture with one gang-block file (leader DVA + 512 B header + 2 members) asserting ordered extents and stats; plus a multi-vdev-report and BE-refusal test.

**F-A2-9 (P2, Behavior/capability, MISSING)** — no block-checksum verification.
Ours: `cr_zfs_engine.cpp` never reads `blk_cksum` (bp+88..119, per our own layout comment engine:62); "damaged" counts only device-read failures (engine:451-453) and decompress failures (engine:479-482).
Og: `zfs-engine--deep-dive.md` §2 "CStreamBase abstracts block-pointer following (gang blocks/indirect blocks) **with checksum-aware reads**" and §4 "…compression [lzjb/gzip] and **checksum verify** to mount-free read a pool". (Artifact wording; not pinned to decompile lines in its ledger — treat og-side as medium-confidence.)
Impact: on CoW ZFS a block that reads successfully but is stale/corrupt is parsed as valid; og would detect and skip it. Our header (cr_zfs_engine.h:24-25) honestly does NOT claim checksum verify — claims honesty OK, parity gap real.
Suggested fix: implement zio_checksum verify for at least fletcher2/fletcher4 (on-disk legacy) + sha256 (the common modern case) on metadata blocks; count mismatches as damaged.

**F-A2-10 (P3, Claims honesty, MISMATCH)** — stale/misleading `unsupported_vdevs` comment.
`include/cr/zfs/cr_zfs_engine.h:58`: `u64 unsupported_vdevs = 0;  /* gang/indirect/mirror/raid multi-vdev */` — lumps "gang" into the unsupported bucket while gang blocks are in fact assembled (engine:491-526); the stat itself only counts non-"disk" vdev types (engine:1144-1145).
Suggested fix: comment → "mirror/raidz/replacing multi-vdev topologies (report-only); gang blocks are assembled".

**F-A2-11 (P3, Report hygiene, MISMATCH)** — PARITY-DELTAS-draft.md D-09/U-6 text corrupted.
`/home/z/my-project/parity/PARITY-DELTAS-draft.md` lines ~193-212/812-813 contain "read_n is implemented", "n blocks counted damaged", "update n/CORE-CONTRACT" — the token "gang"/"G-18"/"G-19" was over-broadly redacted to "n" (same sweep that redacted corpus docs), making the delta registry partially unreadable.
Suggested fix: restore the words gang/G-18/G-19 in D-09/U-6.

**Reconciliation requested by mandate (G-18 vs deltas draft):** G-18's "multi-vdev unsupported" and the deltas' "gang blocks now assembled" are COMPATIBLE — the engine assembles gang blocks within the single-vdev scope (all member DVAs still must resolve on vdev 0, engine:439-440/891-892); only G-18's "gang blocks counted damaged" clause is stale (F-A2-7). Multi-vdev support itself was NOT added (no claim in the deltas draft contradicts this — D-09 correctly describes it as report-only).

## Btrfs audit grid

| Axis | Verdict | Notes |
|---|---|---|
| Behavior (bootstrap) | MATCH | mirror SB select (max generation) → sys chunk array → chunk tree walk (full map) → root tree (ROOT_ITEM(CHUNK_TREE) cross-check) → per-subvol FS trees. Og: artifact §3 same shape. |
| Constants (SB gate) | MATCH | magic u64 "_BHRfS_M"@+0x40, generation@+0x48, sectorsize pow2@+0x90, nodesize@+0x94 band [0x86, 0x86+0xFF7B]=[0x86,0x10000], nodesize%sectorsize==0 — ours engine:18-26,296-304 vs og FUN@10296-10305 (byte-identical checks incl. the 0x86/0xFF7B band). |
| Mirror offsets | MATCH (kernel) / og-table UNVERIFIED | Ours {0x10000, 0x4000000, 0x4000000000, 0x4000000000000} (engine:358) = kernel btrfs superblock mirrors (64K/64M/256G/1P). Og table DAT_180018d60 referenced @10293-10316 (0-terminated candidate loop ✓) but values live in .rdata (not dumped in decompile). NOTE: corpus artifact btrfs-engine--deep-dive.md:30 says mirrors at "0x40000/0x4000000/…" — 0x40000 (256K) is not a btrfs mirror offset; artifact typo, kernel + our table are consistent. |
| Sys chunk array | MATCH | size u32@+0xA0, entries @+0x32B [key 17B type 228=0xE4][chunk 48+32*n, num_stripes u16@+0x2C] — ours engine:23-24,316-338 vs og @2029-2045 (array_size@0xa0, walk from 0x32b, `type != -0x1c`(0xE4), `num_stripes*0x20+0x30`). |
| Chunk mapping | MATCH | parse_chunk: length@0, type@0x18, stripe0 offset@48+8, logical_start from key.offset (engine:187-210, 453) — kernel btrfs_chunk/stripe layout; og parity via sys-array walk above. Single-stripe (stripe 0) scope documented (engine:197-199). |
| Node validation | MATCH + 1 MISMATCH | bytenr self-reference + fsid (engine:175-178) = kernel disk-io.c gates ✓; but chunk_tree_uuid compare uses a bogus SB field (F-A2-12). |
| Dir items | MATCH | name_len@+0x1B(27), data_len@+0x19(25), name@+0x1E(30), advance by 30+name+data, packed records iterated (engine:90-103,641-662) vs og @3424-3444 (verified byte-for-byte incl. "user.DOSATTRIB" match constants). |
| Inline extents | MISSING (content) | FE_TYPE==0 sets inline_data flag + counts stat only (engine:612-616); payload discarded. Og models inline data as resident runs (CResidentNTFSDataRunList reuse, artifact §1). F-A2-13. |
| crc32c | MISSING | Node csum[32]@0 never verified; og capa-verified CRC32/crc32c checksum layer (artifact §2). F-A2-14. |
| Deleted classification | MATCH (deliberate) | Live-tree walk only; flags.keep_deleted/root_deleted unused (engine:347-349); deltas draft D-06 documents the decision; og likewise walks current trees (no evidence of og deleted-btrfs recovery in artifacts). |
| Subvols | DELIBERATE-DELTA (documented) | Ours: ROOT_ITEM(≥256 + FS_TREE 5) each as "subvol-<id>" node nested under root_active (engine:540-586, 726-731); og/DRWUI renders subvols as separate result roots (artifact §3, CBtrfsQuickTreeBuilder). Nesting choice documented in code comment 572-574. Subvol names default "subvol-<id>"; ROOT_REF names not parsed (P3). |
| Xattrs (DOSATTRIB) | MISSING | Og recovers user.DOSATTRIB xattrs (decompile 3424-3444) — EaseUS stores DOS attrs in btrfs xattrs; ours has no XATTR item handling. F-A2-15. |
| Error surface | MATCH | ERR_BAD_PARAMETER / ERR_BAD_MAGIC (no SB / no chunk root / no root tree / no subvols: engine:350,379,445,519,559) / ERR_CANCELLED. |
| Threading | MATCH | Single-threaded BFS with visited-set + node cap 2^20 (engine:253); cancel-checked (249). |
| Test coverage | MATCH (good) | SuperblockValidation, EndToEndWalk, BadMagicRejected, PackedDirItemsInOneItem, MirrorSelectionNewestGeneration (primary corrupted → 64M mirror wins), InternalKeyPtrWalk (multi-level trees) — test:321-560. Gap: fixture writes CTUUID at SB+0x60 masking F-A2-12. |

### Btrfs findings

**F-A2-12 (P2, Constants, MISMATCH)** — `chunk_tree_uuid` read from a non-existent superblock field.
Ours: `cr_btrfs_engine.cpp:25` `SB_CHUNK_TREE_UUID = 0x60`, copied at engine:310 into `sb.chunk_tree_uuid`, then enforced by `read_node(..., chunk_tree=true)` (engine:179-181) for every chunk-root candidate (engine:409).
Ground truth: kernel `btrfs_super_block` (include/uapi/linux/btrfs_tree.h) has **no chunk_tree_uuid** — the field at +0x60 is `log_root` (+0x68 `log_root_transid`); chunk_tree_uuid exists only in `btrfs_header` @node+0x40 (which our HDR_CHUNK_TREE_UUID=0x40 correctly reads, engine:34). So on any real volume `sb.chunk_tree_uuid` = (log_root, log_root_transid) ≠ the node's uuid → ALL canonical chunk-root candidates are rejected → the engine always lands in the fallback bounded system-chunk scan (engine:414-442). Functional impact contained (fallback finds the chunk tree), but the intended validation is dead code and weaker than designed.
The test masks it: `test_btrfs_volume.cpp:204,326` write `memcpy(sb + 0x60, CTUUID, 16)` — a self-fulfilling fixture (kernel-true layout would put log_root there).
Og: no SB-uuid comparison evidenced (the og chunk walk at 1990-2075 keys off array bounds only).
Suggested fix: drop the SB-side uuid entirely — validate chunk-tree nodes by bytenr + fsid + presence of CHUNK items (what the fallback already does), and fix the fixture to kernel-true layout.

**F-A2-13 (P2, Capability, MISSING)** — inline extent content is discarded.
Ours: `cr_btrfs_engine.cpp:612-616` — `ftype == 0` (inline) only sets `ii.inline_data = true`; the payload bytes after the 53-byte `btrfs_file_extent_item` head are never copied; `stats.inline_files` counts them (engine:700-701); header admits "no extents to recover" (cr_btrfs_engine.h:43).
Og: reuses `CResidentNTFSDataRunList` for inline btrfs data (btrfs-engine--deep-dive.md §1: "reused CResidentNTFSDataRunList/CNoneResidentNTFSDataRunList") — i.e., inline file content IS surfaced as resident data.
Our own ZFS engine already has the mechanism (`props->resident_data`, cr_zfs_engine.cpp:1367-1373 for embedded bps).
Suggested fix: for FE_TYPE==0, copy `size - 53` payload bytes (bounded) into `props->resident_data`.

**F-A2-14 (P2, Capability, MISSING)** — no crc32c node-checksum verification.
Ours: `read_node` (engine:168-183) validates bytenr/fsid/uuid but never the `csum[32]` header field (HDR offset 0, never read); grep "crc"/"csum" in cr_btrfs_engine.cpp → 0 hits.
Og: capa-verified CRC32 + adler32 layer — "crc32c for data/metadata (the csum tree)" (btrfs-engine--deep-dive.md §2).
Suggested fix: verify crc32c (Castagnoli, seed ~0, either LE or byteswapped per csum_type) on tree nodes; count mismatches as damaged_nodes.

**F-A2-15 (P3, Capability, MISSING)** — `user.DOSATTRIB` xattrs and ROOT_REF subvol names not parsed.
Og: decompile @3424-3444 walks DIR_ITEM/XATTR records specifically matching "user.DOSATTRIB" (constants 0x534f442e72657375 "user.DOS", 0x52545441 "ATTR", 0x4249 "IB") and extracts the value — EaseUS stores Windows DOS attributes there.
Ours: no KEY_XATTR (24) handling; subvols get synthetic "subvol-<id>" names (engine:567-571) instead of ROOT_REF (type 144) names.
Suggested fix: parse XATTR items for user.DOSATTRIB → props->attributes; parse ROOT_REF for subvol names.

---

## ext4 audit grid

| Axis | Verdict | Notes |
|---|---|---|
| Behavior (bootstrap) | MATCH | SB@1024 → GDT@first_data_block+1 → inode tables → extents/indirect → dirs (linear) → live walk + dtime sweep + journal salvage. Og same (artifact §4). |
| SB gate constants | MATCH (all but one) | 0xEF53@+0x38, inodes/blocks/bpg/ipg nonzero, first_data_block×log rules, 1024<<log, blocks_lo@0x04\|hi@0x150, groups=blocks/bpg sanity — ours engine:18-34,121-149 vs og FUN_180004710 @3214-3250 (verified line-by-line; -0x10AD==0xEF53, 0x400<<log&0x1F, CONCAT44(hi@0x150,lo)). ONE field error: low-bit guard (F-A2-16). |
| inode size @0x58 | MATCH | SB_INODE_SIZE=0x58 u16, rev>=1 gated, else 128, sanity clamp (engine:33,158-160); test InodeSizeOffsetIsSpecTrue @398. Kernel s_inode_size=+0x58 ✓. |
| GDT hi words | MATCH | desc_size u16@+0xFE; 64BIT feature (0x80) → 64-byte descriptors with hi words @0x20/0x24/0x28 (engine:31,167-175,203-209) — kernel ext4_group_desc ✓. NOTE: artifact ext4-hfs-algorithms-v2 §2 claims the og gates desc_size via a bit 0x10 (META_BG) at sb+0x60 — og-side UNVERIFIED by me; ours uses kernel 64BIT. desc_size clamped to exactly 64 (engine:169-172) — kernel-true for all real volumes. |
| Extent tree | MATCH | 0xF30A magic, root inline in i_block (60 B), depth@+6, ee_len unwritten bit-15 decode (raw>0x8000 → &0x7FFF), ei_leaf lo@4/hi@6, per-node entry bound (bs-12)/12, depth cap 5, node budget 65536 (engine:45-46,218-271,320-326) — kernel ext4_extents ✓. Indirect ladder (1-3 levels) for ext2/3 ✓ (engine:327-369). |
| jbd2 BE journal | MATCH (kernel-true) | magic 0xC03B3998 BE, blocktypes 1-5, jsb incompat@0x28 → tag widths (V3=16 flags BE32@4; 12/8 by 64BIT flags BE16@6; +2 CSUM_V2), first-tag UUID-16 skip, ESCAPE restore C0 3B 39 98, commit-sequence gating, revoke surfacing (documented divergence) (engine:660-859). Og: journal walk confirmed (remaining-engines:15-18 "GotJournalDir/TraverseJournalDir" traces) — og's exact tag handling UNVERIFIED. |
| Deleted classification | MATCH (kernel-true) / og UNVERIFIED | live walk marks dtime!=0 \|\| links==0 Deleted (engine:533-534); sweep harvests dtime!=0 or links==0 (engine:588-599), graded vs group block bitmaps → Excellent/PartialRisk/Overwritten/MetadataOnly (engine:564-657). Og-side classification details silent in artifacts (only "orphan-file/list handling" mention). |
| Bigalloc | DELTA (honest refusal) | ERR_UNSUPPORTED on INCOMPAT_BIGALLOC (engine:449-452) with rationale; og behavior UNVERIFIED. |
| Bad-sector salvage | DELTA (documented) | Ours: transport retry layer + zero-fill-continue (engine:387-391, header:8). Og: CEXT4CatchBufferDeviceProxy 0xE00 retry buffer, 0x400 backtrack (artifact §3). |
| Error surface | MATCH | ERR_BAD_PARAMETER/READ_FAILED/BAD_MAGIC/UNSUPPORTED/CANCELLED/NO_MEMORY (engine:438-461,480,515,532). |
| Threading | MATCH | Single-threaded; cancel + inode budget 200k (engine:476,482-486). |
| Test coverage | MATCH (good) | SuperblockValidation, EndToEndWalk, BadMagicRejected, UnixTimeConversion, JournalSalvage (BE + escape + UUID), InodeSizeOffsetIsSpecTrue (test:264-416). |

### ext4 findings

**F-A2-16 (P3, Constants, MISMATCH vs og)** — superblock "low-bit guard" applied to the wrong field.
Ours: `cr_ext4_engine.cpp:124` `if ((sb[SB_INCOMPAT] & 1) != 0) return false; /* compression unsupported (decompile low-bit guard) */` — SB_INCOMPAT=0x60 (feature_incompat).
Og: FUN_180004710 @3219: `(*(byte *)(param_1 + 0x18) & 1) == 0` — the guard is on **s_log_block_size (+0x18)**, not feature_incompat (ext4-hfs-engine-algorithms-v2.md §1 line 13 corroborates: "sb.s_log_block_size(+0x18, byte) & 1").
Divergence cuts both ways: ours rejects incompat-COMPRESSION volumes the og accepts (rare), and accepts odd log_block_size (1/3/5 → 2 KiB/8 KiB/32 KiB blocks) the og rejects — ours is kernel-friendlier there (kernel allows log=1), but the comment misattributes the source.
Suggested fix: either mirror the og gate (reject odd s_log_block_size) or keep the kernel-true behavior and fix the comment to say "ours: reject COMPRESSION; og: reject odd log_bs".

---

## HFS+ audit grid

| Axis | Verdict | Notes |
|---|---|---|
| Behavior (bootstrap) | MATCH | VH@1024 (BE) → fork descriptors → node-0 bootstrap → BTHeaderRec → first/last leaf chain → records → 2-pass hierarchy → rsrc forks → freed-node orphan sweep. Og: hfspluslib port same shape (artifact remaining-engines §HFS+: volume/btree/btnode/catalog/extents/attribute/journal/rawfile). |
| 'H+'/'HX' signatures | MATCH | 0x482B/0x4858 @VH+0 (engine:35-36,334-336) vs og decompile @10005 ✓; version<0x11 (engine:338) vs og @10007 ✓; blockSize%512==0 (engine:341) vs og @10005 ✓. |
| Volume header offsets | MATCH | journalInfo@0xC, fileCount@0x20, folderCount@0x24, blockSize@0x28, totalBlocks@0x2C, freeBlocks@0x30, nextCatalogID@0x40, forks @0x70/0xC0/0x110/0x160/0x1B0 (engine:20-33) — TN1150 ✓. ForkData logicalSize@0/totalBlocks@0xC/8 extents@0x10 ✓. |
| **B-tree node kinds** | **MISMATCH (P1)** | Ours: 0=leaf, 1=index, 2=header, −1=map (engine:55; want_kind=0 at engine:403,462,621; orphan sweep kind!=0 skip at engine:618-620). Truth (TN1150 enum + og): −1=leaf, 0=index, 1=header, 2=map — og FUN_180010720 @13095-13125 gates kind∈{−1,0,1,2} (`3 < (byte)(kind+1)`), kind==1 → header node (first record offset must be 14 = BTHeaderRec), kind==−1 → leaf (height must be 1). F-A2-17. |
| **BTHeaderRec offsets** | **MISMATCH (P1)** | Ours: leafRecords@0x04 (overlaps rootNode!), firstLeaf@0x08, lastLeaf@0x0C, nodeSize@0x10 (engine:59-65; reads at engine:390,395-396,436-441). Truth (TN1150 packed / Linux hfsplus_raw.h): treeDepth@0, rootNode@0x02, leafRecords@0x06, firstLeaf@0x0A, lastLeaf@0x0E, nodeSize@0x12. Every field after rootNode is read 2 bytes early. F-A2-18. |
| Node descriptor / record array | MATCH | 14-byte descriptor {fLink@0, bLink@4, kind@8, height@9, numRecords@0xA}; record offsets as BE u16 at nodeSize−2(i+1), records grow downward (engine:52-57,302-324) — TN1150 ✓; og node validator walks the same offset array (FUN_180010720 @13110-13120). |
| Catalog records | MATCH | key {keyLength@0, parentCNID@2, nameLen@6, UTF16BE name@8} ✓; HFSPlusCatalogFile: recordType@0, flags@2, fileID@8, dates@0xC..0x18, textEncoding@0x50, dataFork@0x58, rsrcFork@0xA8 ✓; folder: valence@4, folderID@8 ✓ (engine:73-90,222-299) — TN1150 exact. |
| 1904 epoch | MATCH | mac_to_filetime = (mac_sec + 9561628800) × 10^7 (engine:328-331); 11644473600−2082844800 = 9561628800 ✓ (1601↔1904 delta). Tested (MacTimeConversion @299). |
| Deleted detection | MATCH (ours) / og UNVERIFIED | freed catalog leaf nodes swept for stale records not in the live cnid map, graded vs allocation bitmap MSB-first (engine:564-676). Og-side classification silent in artifacts (hfsplus deep-dive is class-level only). |
| Embedded volumes | MISSING | No HFS-wrapper ("BD" signature) handling — VH only read at fixed 1024 (engine:372-377); grep "embed" → 0. Og-side: UNVERIFIED (no artifact evidence). P3 (rare case). |
| Error surface | MATCH | ERR_BAD_PARAMETER/READ_FAILED/BAD_MAGIC/CANCELLED/NO_MEMORY (engine:369-377,437-439,492). |
| Threading | MATCH | Single-threaded; cancel + chain/node caps (engine:451,465). |
| Test coverage | MISMATCH (self-fulfilling) | VolumeHeaderValidation, EndToEndWalk, MacTimeConversion, BadSignatureRejected (test:202-310) — but the fixture writes kind=0 for leaves (test:120), kind=2 for the header node (test:166) and the BTHeaderRec at the engine's wrong offsets (test:181-187), masking F-A2-17/F-A2-18. |

### HFS+ findings

**F-A2-17 (P1, Constants, MISMATCH)** — B-tree node kind constants inverted vs TN1150 and the og.
Ours: `cr_hfsplus_engine.cpp:55` — `NH_KIND = 0x08; /* 0 leaf, 1 index, 2 header, -1 map */`; leaf records parsed only when kind==0 (`for_each_record(..., 0, ...)` at engine:403, 462, 621; orphan sweep skips kind!=0 at engine:618-620).
Truth: TN1150 / Apple Files.h enum: `kBTLeafNode = -1, kBTIndexNode = 0, kBTHeaderNode = 1, kBTMapNode = 2`. Og: `HFSPlusSearchDLL_decompiled.c` FUN_180010720 @13095-13125 — validates `(byte)(kind+1) <= 3` (kind ∈ {−1,0,1,2}), and treats kind==1 as the **header** node (its first record must start at offset 14, where BTHeaderRec lives) and kind==−1 as **leaf** (requires height==1).
Impact: on any real HFS+ volume the catalog leaf nodes carry kind=−1 → our `for_each_record(want_kind=0)` rejects them → zero files recovered; conversely index nodes (kind 0) would be parsed as "leaves".
Test masks it: `test_hfsplus_volume.cpp:120` `node[8] = 0; /* kind = leaf */` and `:166` `hdr[8] = 2; /* kind = header */` — fixture mirrors the wrong constants.
Suggested fix: kind constants → leaf=−1, index=0, header=1, map=2; fix `for_each_record` call sites and the orphan-sweep check; fix the fixture.

**F-A2-18 (P1, Constants, MISMATCH)** — BTHeaderRec field offsets read 2 bytes early.
Ours: `cr_hfsplus_engine.cpp:59-65` — `0x02 rootNode, 0x04 leafRecords, 0x08 firstLeafNode, 0x0C lastLeafNode, 0x10 nodeSize, 0x12 maxKeyLength, 0x14 totalNodes, 0x18 freeNodes`; reads at engine:390 (extents tree nodeSize), 395-396 (ext tree first/last leaf), 436-441 (catalog nodeSize/first/last leaf).
Truth (TN1150 "The B-Tree Header Record", packed; identical in Linux `fs/hfsplus/hfsplus_raw.h`): treeDepth@0x00, rootNode@0x02, leafRecords@0x06, firstLeafNode@0x0A, lastLeafNode@0x0E, nodeSize@0x12, maxKeyLength@0x14, totalNodes@0x16, freeNodes@0x1A. Ours reads leafRecords inside rootNode's upper half and every subsequent field 2 bytes early: nodeSize is actually read from the LOW 16 bits of lastLeafNode, firstLeafNode from leafRecords' tail — garbage on every real volume (typically nodeSize reads as lastLeafNode&0xFFFF, e.g. 5 → rejected at engine:437-439 with ERR_BAD_MAGIC).
Test masks it: fixture writes the BTHeaderRec at the engine's offsets (`test_hfsplus_volume.cpp:180-187`).
Og-side: the exact BTHeaderRec parse was not located in the decompile within this pass (UNVERIFIED og-side), but our own header cites TN1150 as the layout source (engine:3), and the 14-byte node-descriptor offset (which our code gets right) confirms the same reference.
Suggested fix: BTH_FIRST_LEAF=0x0A, BTH_LAST_LEAF=0x0E, BTH_NODE_SIZE=0x12 (+ leafRecords 0x06, totalNodes 0x16, freeNodes 0x1A if used); fix fixture; add a test asserting nodeSize against a TN1150-true layout.

**F-A2-19 (P3, Capability, MISSING)** — no embedded (HFS-wrapper) volume support.
Ours: VH is only read at fixed offset 1024 (engine:372-377); an HFS+ volume embedded in a classic-HFS wrapper (driver signature "BD", VH at the embedded volume's start block) is not probed. Og-side: no artifact evidence either way (UNVERIFIED).
Suggested fix: probe for the HFS wrapper (sig "BD" @1024, HFS Plus signature word in the master directory block) and re-base the VH read; or document the limitation in the header.

---

## Cross-check: docs/ground-truth-contracts.md evidence pointers (sampled)

Sample of 5 pointers checked, **all verified** (the contracts doc covers the non-FS-engine areas; the five engines audited above have no section in it — itself a gap worth noting):

| Contract claim | Pointer | Verification |
|---|---|---|
| NTFS 'FILE' magic 0x454C4946 | NTFSSearchDLL_decompiled.c:21152 | ✓ line reads `if ((*piVar1 == 0x454c4946) &&` |
| NTFS metafile filter (flags&4 or name '$') | same file:21170 | ✓ `((uVar7 & 4) != 0) \|\| … (*pcVar15 == '$')` |
| JPEG SOF0 0xC0 dimension check, precision 8 | RAWSearchDLL_decompiled.c:39653 | ✓ `(bVar1 == 0xc0) && (pbVar25[4] == 8)` |
| Error family 0x5002xxxx | architecture/05-scan-engine-architecture.md | ✓ line 62 (open-fail 0x5002200000004 / read-fail 0x5002c0000004) |
| Node sizes 0x93 / 0x290 | SaveScan/rsf-format--scan-session.md:256-257 | ✓ CFileNode 0x93, CFileSystemNode 0x290 |
| FAT deleted 0xE5 | fat-engine--deep-dive.md | ✓ `-0x1b` deleted-keep gate (corroborated in passing) |

**Note:** ground-truth-contracts.md has no sections for ReFS/ZFS/Btrfs/ext4/HFS+ — the FS-engine constants live only in engine headers; given F-A2-17/F-A2-18, promoting the audited constants into this doc would likely have caught the HFS+ errors.

## UNVERIFIED list (corpus silent — never guessed)

1. ReFS: exact deleted-row flag values (0x4) and ID2 semantics at og level (thesis-cited only); allocator-grading semantics inside ReFSSearchDLL (CRefsAllocatorMgr is in the partition-side .mo); backup-superblock locations (n-3/n-2) og-side; checkpoint clock offset +0x60 og-side; og's MSB-chain `next` real-world trigger frequency.
2. ZFS: og's label-1..3 bases; og checksum-verify function identity (artifact wording only); og's exact SA-vs-znode handling; og delay-execution retry semantics.
3. Btrfs: og mirror-offset table values (DAT_180018d60 in .rdata, not dumped); og SB-side uuid usage (none seen); og crc32c application points.
4. ext4: og's desc_size feature-bit (artifact claims incompat bit 0x10 = META_BG at volume+0xF8; kernel uses 64BIT=0x80 — ours kernel-true); og's deleted-inode classification; og's journal tag-width handling; og's bigalloc behavior.
5. HFS+: og's BTHeaderRec parse offsets (not located in decompile within budget); og's deleted classification; og's embedded-volume handling.

## Deliberate-delta candidates (documented, accepted)

- ReFS version gate wider (1.x–3.x vs og 3.4+) — documented in header; F-A2-3 corrects the og-gate description.
- ZFS config nvlist at OpenZFS label+16K vs og's odd 0x1C000 window — documented (cr_zfs_engine.h:27-29, AUDIT §3.4).
- ZFS uberblock slot-size probe (shifts 10–13) vs og fixed 1 KiB — improvement, documented in DELTAS draft.
- ZFS deleted = live-state only — D-05 parity decision.
- Btrfs deleted = live-tree only — D-06 parity decision; subvols nested vs og's separate roots (code comment).
- ext4 journal revoke-surfacing (recovery-grade) vs kernel replay skip — documented in engine comment.
- ext4 CatchBuffer equivalence delegated to transport retry layer — documented in header.
- ext4 BIGALLOC honest refusal (ERR_UNSUPPORTED).

## Severity tally

| Severity | Count | IDs |
|---|---|---|
| P0 | 0 | — |
| P1 | 2 | F-A2-17, F-A2-18 (HFS+ cannot scan real volumes; tests self-fulfilling) |
| P2 | 9 | F-A2-1, F-A2-2, F-A2-5, F-A2-7, F-A2-8, F-A2-9, F-A2-12, F-A2-13, F-A2-14 |
| P3 | 8 | F-A2-3, F-A2-4, F-A2-6, F-A2-10, F-A2-11, F-A2-15, F-A2-16, F-A2-19 |

**Top recommended actions:** (1) fix HFS+ kind constants + BTHeaderRec offsets and rebuild the fixture TN1150-true (F-A2-17/18); (2) refresh G-18 and the ZFS header comment (gang blocks assembled) + add a gang-block test (F-A2-7/8/10); (3) add the missing MSB-chain test coverage (F-A2-5); (4) fix Btrfs SB-uuid misuse + fixture (F-A2-12); (5) add checksum verification for Btrfs crc32c and ZFS zio checksums (F-A2-14/9); (6) recover inline-extent content (F-A2-13).

**Method note:** our-code-first read of all five engines + tests; corpus artifacts (9 SearchDLL deep-dives) read in full; targeted decompile Greps verified 20+ specific claims line-by-line (ReFS FUN_180012390/180012650/180011770/18000fc80/18000ff60, ZFS FUN_180018220/180018930/1800270d0, Btrfs FUN@10280/1990-2075/3424-3444, ext4 FUN_180004710, HFS+ FUN@10005/180010720). Every MISMATCH is independently checkable from the cited line numbers alone.






