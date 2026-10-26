# MAJOR M-020 — Btrfs: og has chunk-profile machinery (RAID10 gates, 2-copy rotating mirror reads) and a default-subvolume pick; the core is stripe-0-only with synthetic subvol names

**Severity:** MAJOR (multi-device/RAID volumes degrade to wrong content; default-subvol trees named wrong)
**Module:** btrfs
**Parameters affected:** B1 (detection), B2 (content on profiled volumes), A26 (tree walk), B5-adjacent
**Corpus references:** `BtrfsSearchDLL_decompiled.c` — chunk parse `:2145-2169` (type bits: &1 DATA, &4 SYSTEM, &2 METADATA; max-chunk trackers `:2148-2160`; **RAID10 gate**: `sub_stripes u16@0x2e == 0 || num_stripes u16@0x2c < sub_stripes → reject` `:2162-2168`); profile resolution `:2498-2543` (DUP &0x20 → 2-copy; RAID0 &8; RAID1 &0x10 → 2-copy; RAID10 &0x40; **2-copy profiles use a rotating mirror index** `*(ushort*)(map+0x20)`, `index = (index+1) % num_stripes` `:2534-2537`, physical = `stripes[index].offset + (logical - chunk)` `:2540`); default-subvolume pick `:3975-4053` (name-hash 0x8dbfc2d2 "default", name_len==7, key type 0x84 DIR_ITEM `:4019`, fallback root 5 `:4060`); node read + error-map `FUN_180015400 :2544-2550`
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp:199-207` (parse_chunk stripe-0 only, single-device scope documented), `:569` (synthetic `subvol-<id>` names) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's Btrfs engine resolves chunk profiles: it validates RAID10 chunk geometry, resolves 2-copy profiles (DUP/RAID1/RAID10) through a **rotating mirror index** across stripes, maps logical→physical per stripe, and records sector-granular read errors into an error map. It also picks the default subvolume by the canonical "default" DIR_ITEM (hash 0x8dbfc2d2) with root-5 fallback, so the default subvol's files appear at tree root with real names. The core reads stripe 0 of every chunk only and names every subvolume `subvol-<id>` — on any 2-copy or striped profile the core reads the wrong physical copy (rotating mirrors mean stripe 0 is not always the intended member), and the default subvolume is never identified.

## What the reference does

Quotes above — particularly `:2534-2540`: for 2-copy profiles the physical read target rotates across the stripe list per access, which is also a wear-leveling/truth mechanism the core's fixed stripe-0 choice does not reproduce.

## What our core does

`parse_chunk` takes stripe 0 unconditionally (`:199-207`, with an honest scope comment); subvolume enumeration synthesizes names (`:569`).

## Gap

Falsifiable claims: (a) a DUP/RAID1 volume whose stripe-0 member has stale data: og reads the fresh copy on rotation; core reads stale bytes — recovered content differs; (b) a RAID10 chunk with `sub_stripes > 0` and `num_stripes < sub_stripes`: og rejects the chunk; core walks it; (c) default subvolume: og's tree root shows its contents directly (root-5 fallback); core shows `subvol-5` as a sibling container.

## Impact

Content correctness on profiled volumes (the common modern-btrfs case: DUP metadata at minimum) and tree-shape parity for the default subvolume.

## Reproduction / verification

Decompile-verified with full quotes (agent S2-C2; `evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`); core lines verified first-hand.

## Fix plan

1. `parse_chunk`: decode the full stripe array; validate RAID10 geometry (`sub_stripes != 0 && num_stripes >= sub_stripes` else reject — og `:2162-2168`); map logical→physical per profile: 2-copy (DUP/RAID1) via the rotating mirror index; RAID0 by `(logical - chunk) / stripe_len % num_stripes`; single-device unchanged.
2. Default-subvolume pick: after the root tree walk, look up key type 0x84 DIR_ITEM with name-hash 0x8dbfc2d2, name "default" (`:3975-4019`); on absence use root 5 (`:4060`); present its children at the tree root and keep other subvols as named containers (drop the synthetic name for the default only — keep `subvol-<id>` for the rest).
3. Wire per-node read failures into the existing retry-layer error map (the og's `FUN_180015400` analog) so partial chunk reads surface as bad-sector ranges rather than silent truncation.

## Tests to add

- `Btrfs.Raid1RotatingMirrorReads` — DUP metadata fixture with divergent copies: content matches the og's rotation choice (fails before — always stripe 0).
- `Btrfs.Raid10GeometryGate` — bad sub_stripes chunk rejected.
- `Btrfs.DefaultSubvolumeAtTreeRoot` — "default" DIR_ITEM fixture: children at root (fails before).

## Related findings

- I-003 (mirror probing at the superblock level — core superset, re-confirmed with exact quotes) · N-006 (nodesize band — re-confirmed, upper bound inclusive)

## Open questions

Whether the core's Btrfs cache file (`BtrfsCache.txt` og side-effect) has value — og writes `.\BtrfsCache.txt` (`:1991`); register a decision (likely informational exclusion).

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-020 landing (DFX33; commit sha in the worklog entry)
**Verified by:** `Btrfs.Raid1RotatingMirrorReads` + `Btrfs.Raid10GeometryGate` + `Btrfs.Raid0StripeMathSplitsAtStripeBoundaries` + `Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack` (all 4 fail-before EXECUTED RED on the pre-fix-behavior tree — rotation/gate/default-rendering temporarily disabled in place, 4/4 RED; pass-after 4/4 GREEN) + the updated `Btrfs.EndToEndWalk`/`Btrfs.SubvolumeNamedFromRootRef` (default-at-root expectations, RED on the same tree) + full suite **747/747** linux-release at the fix commit (incl. win_syntax_check).
**Resolution notes:** Corpus re-verified LINE BY LINE first-hand before the fix, and the audit's Summary contains a decode error this fix corrects in the record: the og's profile machinery at BtrfsSearchDLL :2498-2543 decodes classes DUP(&0x20)/RAID1(&0x10)/SINGLE(no bits)→class 0x20 served by the rotating mirror (index=(index+1)%num_stripes per access, :2534-2537; physical=stripes[index].offset+(logical−chunk), :2540), RAID0(&8)→class 8 and RAID10(&0x40)→class 0x40 — and the class-8/0x40 paths FALL THROUGH to LAB_1800043fe (:2521-2583, the failure return): **the og FAILS RAID0 and RAID10 node reads entirely; RAID10 is NOT resolved through the rotating mirror** as the audit's Summary claimed (the finding's own corpus-quote block was right; the prose was wrong). Implemented: (1) `parse_chunk` decodes the full stripe array + stripe_len/sub_stripes/num_stripes into ChunkMap and applies the og's RAID10 geometry gate verbatim (:2162-2168: sub_stripes==0 or num_stripes<sub_stripes → chunk rejected); (2) `logical_to_physical` is profile-aware: class-0x20 through the per-chunk rotating mirror index (og-exact, SINGLE degenerates to stripe 0 at num_stripes=1); RAID0 via the kernel stripe math (`idx = stripe_nr % n`, physical = stripes[idx].offset + (stripe_nr/n)·stripe_len + in-stripe) and RAID10 via mirror-set math (set = stripe_nr % (n/sub_stripes), og-style member rotation) — both REGISTERED SUPERSETS (PARITY-DELTAS D-78: the og fails these classes; the pre-fix core's stripe-0 read emitted WRONG BYTES on striped chunks, the fix either maps correctly or refuses); classes &0x80/&0x100 refused exactly like the og's failing read path; (3) raw extents on striped profiles split at stripe boundaries (`map_extent_pieces`) so spanning extents emit per-stripe pieces — compressed extents stay atomic and are refused+counted (`unmappable_extents` stat) when spanning, never emitted with wrong bytes; (4) default-subvolume pick og-exact (FUN_1800061f0 :3975-4060): the root tree's DIR_ITEM (ROOT_TREE_DIR 6, 0x54, crc32c("default")=0x8dbfc2d2) must spell "default" (len 7) AND carry a located key of type 0x84 (both gates with the og's error strings as the design source); missing/invalid/unknown target falls back to root 5; the default's children render directly at the volume root (the ReFS root convention — the og's default IS the volume content), every other subvolume keeps its N-047 container (ROOT_REF name / subvol-<id>). Fix-plan item 3 (per-node read failures into the error map) verified ALREADY SATISFIED BY CONSTRUCTION: the engine reads through the orchestrator's Skip-mode RetryingDevice pipe (cr_scan.cpp:802) whose ReadErrorMap records every failing sector range with bisection isolation (cr_retry.h contract) — the og's FUN_180015400 analog lives one layer down; no duplicate per-engine recording added (double-recording would corrupt the map's accounting). `mirror_rotations` stat added (non-first-stripe 2-copy accesses). Safety hardenings preserved: bytenr/fsid/chunk-tree-uuid node gates unchanged; BTR-2/BTR-3/BTR-4/S-005/M-010/N-016b/N-047/S-028 behaviors unchanged and re-proven by the untouched tests in the same suite run.
