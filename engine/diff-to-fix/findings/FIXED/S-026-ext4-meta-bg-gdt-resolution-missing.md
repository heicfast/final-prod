# SEVERE S-026 — ext4 meta_bg group-descriptor resolution missing: files silently missing on meta_bg volumes

**Severity:** SEVERE
**Module:** ext4
**Parameters affected:** B1 (detection/correctness — inode reads fail for unmapped groups), C20 (on-disk struct offsets — meta-group descriptor block layout), A25 (engine algorithm steps)
**Corpus references:** EXT4SearchDLL_decompiled.c — meta_bg gate `incompat & 0x10` :3633; meta-group stride seeded from s_desc_size :3368-3373; descriptor blocks located at `(first_data_block + meta_group) * block_size` with per-meta-group cache :3664-3703; twin resolver `FUN_180004c70` :3500-3569.
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:175-191` (front GDT only; no meta_bg branch), group() resolution returns zero descriptors for unmapped groups → read_inode fails. Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `S-019` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og resolves group descriptors per meta-group when the meta_bg feature flag is set (descriptors for later groups live in backup-superblock blocks, not the front GDT). Our engine reads only the front GDT: on a meta_bg volume, every group whose descriptor is not in the front table resolves to garbage — inode reads fail and those files are silently missing from recovery.

## What the reference does

At :3633 the og gates on `incompat & 0x10`; the meta-group stride derives from s_desc_size (:3368-3373); descriptor blocks are located at `(first_data_block + meta_group) * block_size` and cached per meta-group (:3664-3703; the twin at :3500-3569). (Note: the og's stride seed at :3368-3373 looks wrong vs the kernel's `block_size/desc_size * blocks_per_group` — see Open Questions; the location arithmetic is the verified part.)

## What our core does

`cr_ext4_engine.cpp:175-191` reads the front GDT only; there is no meta_bg code path at all (grep: no `0x10` incompat branch). Groups beyond the front table read zeroed descriptors → `read_inode` fails → files in those groups never emitted.

## Gap

meta_bg volumes (mkfs `-O meta_bg`; large-volume layouts where the GDT overflows) lose all files in meta-group-served groups. Falsifiable: an ext4 formatted with `-O meta_bg` yields most files under the og and near-none under ours.

## Impact

Silent missing files on meta_bg volumes (large-disk configurations). No crash, no wrong bytes — absence.

## Reproduction / verification

Corpus-derived (cites first-hand). UNVERIFIED-BY-EXECUTION (needs a `-O meta_bg` mkfs image — trivially generatable with mkfs.ext4 on Linux).

## Fix plan

1. In the superblock gate, accept and record `incompat & 0x10` (currently only compression bit 0 is tested — bigalloc refusal is separate and deliberate, I-014 (parallel leg register)/E-notes).
2. Implement per-meta-group descriptor resolution at `(first_data_block + meta_group) * block_size` with the cache map, per the og arithmetic; validate the stride against the kernel formula on real images (open question below).
3. Route `group()` lookups through the resolver.

## Tests to add

- `Ext4.MetaBgVolumeRecoversAllGroups` — mkfs -O meta_bg fixture (e.g. 256MB, 1KiB blocks to force multiple meta-groups); assert group count and file census vs a non-meta_bg twin image (fails today).
- `Ext4.MetaBgDescriptorCacheReuse` — repeated group reads hit the cache (perf guard).

## Related findings

I-014 (parallel leg register) (E9: the 1KiB-block GDT offset og-bug non-adoption — adjacent area), S-027 (backup-SB scan — the same meta-group blocks are backup SBs).

## Open questions

The og's stride seed (`+0xdf8` from s_desc_size, :3368-3373) vs the kernel's meta-group size formula — build a real `-O meta_bg` image and record which groups each side resolves before trusting the og constant (tracked OQ-E2).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's fix commit — `git log` "fix: S-026")
**Verified by:** `Ext4Engine.MetaBgVolumeRecoversAllGroups`, `Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups`, `Ext4Engine.MetaBgDescriptorBlockReadOnce`, `Ext4Engine.MetaBgFirstMetaBgFrontRegion` (front-region regression guard), `Ext4Engine.MetaBgSuperblockFieldsParsed` — 4/5 fail-before proven on the engine-reverted build (stash/rebuild/run/pop: far.bin missing, group-16 deleted inode missing, descriptor block never read / never parsed), full suite 401/401 Linux Release (396 + 5 net-new); ASan+UBSan+leak fuzz smoke clean over structured meta_bg seeds (incl. the two real-mkfs geometries below).
**Resolution notes:** Independent re-verification went beyond the decompile: two REAL meta_bg images were built this session with e2fsprogs 1.47.2 (mke2fs -O meta_bg; 1KiB/64B-desc and 4KiB/64B-desc) and the layout pinned empirically with dumpe2fs + hexdump — the descriptor block for meta-group m sits at `first_data_block + (m * meta_group_size) * blocks_per_group` (+1 only for m == 0, whose group hosts the primary SB); meta-group starts carry NO backup superblock (verified: "Group descriptor at 131073/262145/2097152" with no SB line; descriptor contents byte-matched dumpe2fs's reported bitmap/itable blocks); meta_group_size = block_size / desc_size (EXT2_DESC_PER_BLOCK); meta-groups below s_first_meta_bg keep the classic front location `first_data_block + 1 + m` (e2fsprogs ext2fs_descriptor_block_loc2, fetched verbatim). The fix implements exactly that kernel/e2fsprogs-correct arithmetic: `SuperBlock.first_meta_bg` parsed from SB+0x230 (clamped to groups), `Volume::group()` routes through a lazy per-meta-group cache (one bounded device read per meta-group, MAX 4096), and the front-GDT bulk read now applies ONLY to non-meta_bg volumes (the pre-fix bug: `groups * desc_size` bytes were swallowed from the front block, so groups beyond the front meta-group read bitmap/inode-table bytes as descriptors and their files vanished silently — reproduced fail-before). The og's two layout defects are deliberately NOT adopted and registered as PARITY-DELTAS (D-56): (a) stride seeded from `s_desc_size` (:3368-3373, asm-verified at 180004cb9 `divl 0xdf8(%rcx)`), which coincides with the kernel meta-group size only when desc_size² == block_size (4K/64B and 1K/32B) and misresolves 2K/32B + 4K/32B volumes; (b) the descriptor read at `m*blocks_per_group*block_size + base` with no `first_data_block` adjustment (:3582-3583, asm-verified `imul` chain at 180004d8d-180004d9a), one block early on 1KiB-block volumes — same family as the already-documented 1K front-GDT og-bug (I-014 E9). The finding's own fix-plan location formula "(first_data_block + meta_group) * block_size" is corrected by this resolution: the multiplier is blocks_per_group, not 1.
