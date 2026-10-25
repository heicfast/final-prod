# SEVERE S-027 — ext4 backup-superblock lost-volume scan missing (og strides group boundaries and spawns sub-searchers)

**Severity:** SEVERE
**Module:** ext4, lost-partition
**Parameters affected:** B6 (post-format/remnant scan), B1 (detection correctness), A30 (candidate discovery)
**Corpus references:** EXT4SearchDLL_decompiled.c — `FUN_180001b00` :624-649: strides group boundaries (read at `(N+param_2)*sector`), validates each candidate superblock, spawns a sub-searcher with base = found−0x400, collects hits.
**Core references:** `include/cr/lost/cr_lostpart.h:28` (CandidateFs = {Ntfs, Fat, Exfat, GptTable, Unknown} — no ext4), `src/core/ext4/cr_ext4_engine.cpp:425-430` (primary SB only). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `S-020` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's ext4 engine doubles as a lost-volume scanner: when the primary superblock is gone it strides group boundaries looking for backup superblocks and re-bases a sub-searcher at each hit. Our lost-partition discovery probes only NTFS/FAT/exFAT/GPT signatures, and our ext4 engine reads only the primary SB — a disk whose ext4 partition-table entry and primary SB are destroyed finds nothing.

## What the reference does

`FUN_180001b00` (:624-649): for candidate offsets strided by group size, read at `(N + base) * sector`, validate as a superblock, spawn a sub-searcher with `base = found − 0x400` (the SB's block-1 offset), collect the hit. This is the engine-side lost-ext4 mechanism (complementing the .mo partition analysers).

## What our core does

`cr_lostpart.h:28` has no ext4 candidate type; `scan_ext4` reads only the primary SB (:425-430); lost-partition discovery is 55AA/OEM-probe based (S-006's scope) with no ext4 superblock probe.

## Gap

Lost ext4 volumes (destroyed table entry + damaged primary SB) are invisible. Falsifiable: a disk with a deleted ext4 partition whose backup SBs survive lists the volume under the og, nothing under ours.

## Impact

Whole-volume recovery loss for ext4 in the damaged-table scenario — the feature's core case. (Part of the S-006 family but distinct mechanism: S-006 covers the signature-probe SET; this is the engine-internal backup-SB stride.)

## Reproduction / verification

Corpus-derived (quote first-hand). UNVERIFIED-BY-EXECUTION (mkfs image with primary SB zeroed — generatable).

## Fix plan

1. Add `CandidateFs::Ext4` (and the HFS+/Btrfs/ZFS/ReFS siblings tracked in S-006) to the lost-partition enum.
2. Implement the backup-SB probe: ext4 SB magic 0xEF53 at group-boundary strides (the og's `(N+base)*sector` walk); on hit, materialize a slice rebased by the group arithmetic and dispatch scan_ext4.
3. Record overlap/dedup interplay with S-006's probe set (a found ext4 SB is also a partition candidate).

## Tests to add

- `Lost.Ext4BackupSuperblockScan` — fixture with zeroed primary SB + intact backup; assert the volume is discovered (fails today).
- `Lost.Ext4SubSearcherRebase` — assert the sub-searcher's base arithmetic (found − 0x400 semantics).

## Related findings

S-006 (lost-partition probe families — the umbrella finding), S-026 (meta_bg — the same group-boundary blocks), I-014 (parallel leg register) (E-notes).

## Open questions

Whether the og's stride is exactly group-size or a multiple (the `param_2` unit at :624-649 needs one more decode pass on real geometry — low risk; the mechanism is verified).
