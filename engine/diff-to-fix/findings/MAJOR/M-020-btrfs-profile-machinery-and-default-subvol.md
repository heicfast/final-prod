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
