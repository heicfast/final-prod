# MINOR N-050 — Node-field gaps: no per-file type-registry ID, no per-FS secondary payload slots (Btrfs gen/transid, ZFS secondary flags, HFS+ mode/type)

**Severity:** MINOR
**Module:** session (node data model), btrfs, zfs, hfsplus
**Parameters affected:** D11 (node fields), D12 (session persistence), P42 (public surface — typed metadata)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:76-90` (RAW: type-registry ID u32@+0xF8 "35 for the shot*.jpg carve set — **same ID space as NTFS +0x60**"; Btrfs/ZFS secondary payloads), `ifileproperties-part1--ntfs-fat-exfat.md:84,86,128-129,148` (NTFS +0x40 parent/directory anchor, **+0x60 type-registry ID**, FAT 2×u32 anchors), `ifileproperties-part2--ext4-hfs-refs.md` (HFS+ mode/type fields), `rsf-record4--tree-node-grammar.md` (per-FS record factories via `data-vt+0x98`)
**Core references:** `include/cr/tree/cr_node.h:57-92` (`FileProperties`: times×4, attrs, sizes, extents, carve_format/carve_confidence, sha128, fs_kind — no type-ID slot, no per-FS secondary payload); `src/core/btrfs/cr_btrfs_engine.cpp`, `src/core/zfs/cr_zfs_engine.cpp`, `src/core/hfsplus/cr_hfsplus_engine.cpp` (engines parse the per-FS fields but have no persistence slot — e.g. Btrfs inode generation/transid, ZFS flags, HFS+ mode/filetype are dropped at emission)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's IFileProperties model persists a per-file **type-registry ID** (one shared ID space across engines — the ID that feeds the results-UI Type column) and per-FS secondary payload fields (Btrfs generation/transaction ids, ZFS secondary flags, HFS+ mode/type). The core's `FileProperties` has neither a type-ID slot nor any per-FS secondary slot — those fields are parsed by our engines and then dropped, so they cannot be persisted, queried, or reported.

## What the reference does

The ifileproperties artifacts (parts 1-3, first-cited together this session) show per-engine serialization carrying: a **type-registry ID** per file (part3:+0xF8 for RAW files — "same ID space as NTFS +0x60"; part1:148 for NTFS), parent/directory anchors (part1:84,128-129), and per-FS secondary payloads (part3's Btrfs generation/transid + ids A/B, ZFS secondary flags; part2's HFS+ mode/type). The record-4 grammar (rsf-record4:26) serializes each node via its FS-specific factory (`[4B type][type-specific body via data-vt+0x98]`) — the per-FS payload is a first-class part of the session tree.

## What our core does

`FileProperties` (cr_node.h:57-92) is FS-generic: times, attributes, sizes, extents, carve fields, hash, fs_kind. There is no type-registry ID (the only typed field is `carve_format`, carve-only), and no per-FS secondary slot. The Btrfs engine parses inode generation/transid during the walk; the ZFS engine reads the flags word; the HFS+ engine reads mode/filetype — none survive emission.

## Gap

Falsifiable claim: for a Btrfs file with generation G and transid T, the og's session record persists both (part3 payload fields); the core's node (and session round-trip) carries neither. For any file, the og persists a type-registry ID; the core persists none outside carve.

## Impact

Data-model completeness: the missing type-registry ID is the engine-side half of the query model's Type sort key / Type column gap (M-029 registers the query side; without the field, the query side cannot be fixed properly). The per-FS secondary payloads are provenance/diagnostic fields (Btrfs transaction ordering matters for choosing the freshest copy of a multiply-snapshotted file; HFS+ mode/type feeds BSD-type display). Low urgency, but they are fields the reference persists and we structurally cannot.

## Reproduction / verification

Corpus-derived (all three ifileproperties artifacts + record-4 grammar first-cited this session; quotes with offsets above). Core side verified in-source: `FileProperties` read at cr_node.h:57-92; engine emission sites grep-checked for generation/transid/mode drops.

## Fix plan (for the engineering team)

1. Add an optional typed-metadata tail to `FileProperties` (keep the hot struct lean): `u16 type_id = 0;` (0 = none) + a small variant/`std::string` or fixed 32-byte per-FS secondary blob (`fs_extra`). Enumerate the og's shared type-registry ID space as a stable table (start with the values the corpus artifacts evidence: 35 = the JPG carve set's ID etc.; the full table is OQ-S4-1's extension-table dump).
2. Populate in each engine at emission (btrfs: generation/transid/ids; zfs: secondary flags; hfsplus: mode/filetype; all: type_id).
3. Extend the session v2 tail (append-only per the established pattern) and the report/query projections as consumers land (query Type column per M-029's fix plan).
4. Tests: `Node.BtrfsSecondaryPayloadRoundTrip`, `Node.ZfsFlagsRoundTrip`, `Node.HfsplusModeTypeRoundTrip`, `Node.TypeIdPopulated` (per-engine).
5. Acceptance: a Btrfs scan's session round-trip preserves generation/transid; the query layer can order by type once M-029's sort key lands.

## Tests to add

- `Node.BtrfsSecondaryPayloadRoundTrip`
- `Node.ZfsFlagsRoundTrip`
- `Node.HfsplusModeTypeRoundTrip`
- `Node.TypeIdPopulated`

## Related findings

M-029 (query model — the Type sort key is the direct consumer); M-041 (carve emission family — the type-registry ID is one of the fields it drops); OQ-S4-1 (the 169-entry grading extension table — adjacent ID space to decode before finalizing the type-registry table).

## Open questions

The exact og type-registry ID table beyond the evidenced examples (35 = JPG carve set) — the full mapping needs the OQ-S4-1 extension-table dump; the FIELD's existence and cross-engine sharing are already evidenced.
