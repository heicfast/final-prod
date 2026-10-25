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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's fix commit — `git log` "fix: S-027")
**Verified by:** `Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed`, `Ext4Engine.BackupRescueFallsBackToBackupGdt`, `Ext4Engine.BackupRescueWholeDiskRebase` (3 fail-before proven on the engine-reverted build: ERR_BAD_MAGIC with the volume invisible), `Ext4Engine.BadMagicWithoutBackupsStillRejected` (regression guard: no backups → the original contract stands, probes > 0); full suite **434/434** Linux Release serial (430 + 4 net-new; one pre-existing timing-sensitive parallel-ctest flake in Snapshot.ConcurrentEmitCaptureWalk reproduced both with and without this change — serial green); ASan+UBSan+leak fuzz smoke clean over 9 structured seeds incl. dead-primary/hostile-candidate shapes.
**Resolution notes:** Independent re-verification pinned the og mechanics first-hand: FUN_180001b00 sits at CEXT4CompleteSearchController vtable slot 53 (.rdata 0x180016880 — the ONLY reference; no textual caller, so the audit's "strides group boundaries" gloss on param_2/param_3 could not be confirmed — the walker itself probes consecutive sector multiples `(i + param_2) * sector_size`), validates each window with the SAME validator as the primary open (FUN_180004710 :636), and spawns a sub-searcher rebased at `found − 0x400` (:637-645, collected only if it yields ≥ 1, :646-660). The corpus artifact (ext4-engine--deep-dive.md §2) documents the probed positions as "the primary offset and backup superblock positions (sparse/backup superblock blocks: 8193, 32768…)" — the canonical group-boundary backups. This session's empirical work added the missing ground truth: real mke2fs images prove backup SBs sit at `(first_data_block + g*bpg) * block_size` with `s_block_group_nr = g` (SB+0x5A: primary 0, group-1 copy 1, group-3 copy 3 — all verified byte-level), and the backup GDT immediately follows ("Backup superblock at 8193, Group descriptors at 8194-8197" — the FULL table on classic volumes; meta-group-0's single block on meta_bg volumes, whose later meta-groups stay distributed and resolve through the S-026 resolver). The fix implements the rescue INSIDE scan_ext4 (the engine-side locus the og uses, not the lost-partition layer — probe_ext's block_group_nr==0 rejection of backups is the og's own CExtX gate and stays): primary SB invalid → probe a 1 MiB grid (m ≥ 8, one buffered 8 KiB read per window, ~0.8% of the slice, bounded 4 TiB, cancel-polled, read-failure tolerant) covering both candidate families (m*1MiB + {0, 1024} — every canonical geometry's group boundaries; the +1024 family is the 1KiB-block volume); on a valid backup, adopt it, compute the EXACT volume base from `s_block_group_nr` (`base = found − (fdb + gnr*bpg) * bs`), and run the GDT ladder: front GDT first (liveness = group-0 descriptor references a real inode table), then the backup GDT at the found copy's block + 1. Emitted extents are base-adjusted (slice-relative) so downstream recovery reads land correctly. Deliberate divergences registered as PARITY-DELTAS **D-59**: (a) canonical-position grid instead of the og's unpinned sector sweep; (b) failed probe reads skip onward (the og aborts the whole walk — its CatchBuffer proxy pre-salvages; our transport retry layer owns bad sectors); (c) exact `s_block_group_nr` rebasing instead of the og's `found − 0x400` boot-area model, which misplaces every structure by 1024 bytes for backup copies (group g's copy has no boot skip — the same defect family as the og's 1KiB front-GDT bug, I-014 E9); (d) legacy cylinder-aligned whole-disk starts are not covered by the 1 MiB grid (they fall to the carve layer) — volume-start slices (partition/lost-candidate materialization, the og's own sub-searcher dispatch shape) are covered alignment-independently. The finding's fix-plan claim "spawn a sub-searcher with base = found−0x400 (the SB's block-1 offset)" is corrected: that base is the og's (defective) boot-area rebase, not block arithmetic.
