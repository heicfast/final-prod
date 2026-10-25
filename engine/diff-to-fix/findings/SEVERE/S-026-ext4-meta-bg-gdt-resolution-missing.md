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
