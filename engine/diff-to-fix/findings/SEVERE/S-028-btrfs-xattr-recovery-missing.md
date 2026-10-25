# SEVERE S-028 — Btrfs xattr recovery missing: user.DOSATTRIB, user.* streams, and the compression property tag

**Severity:** SEVERE
**Module:** btrfs
**Parameters affected:** B7 (alternate streams/xattr handling), B11 (node fields — attribute records), A25 (engine algorithm steps — XATTR item walk)
**Corpus references:** BtrfsSearchDLL_decompiled.c — XATTR item (type 0x18) walk: `user.DOSATTRIB` value extraction :3424-3441 ("found xattr %s" :3430); other `user.*` xattrs surfaced as per-file attribute records :3544-3602; `btrfs.compression` property → codec enum zlib=1/"zlo"=2/zstd=3 :3517-3543.
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp:619-720` (item-type dispatch handles INODE_ITEM/EXTENT_DATA/DIR_ITEM/ROOT_ITEM/CHUNK only — no KEY_XATTR anywhere; grep-verified). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `S-021` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's Btrfs engine walks XATTR items and surfaces `user.DOSATTRIB` values (Windows attribute semantics for Samba-shared volumes), every other `user.*` xattr as per-file attribute records, and the `btrfs.compression` property as a codec tag. Our engine has no XATTR handling at all. On a Samba-shared Btrfs volume, Windows-visible file attributes and user streams are silently dropped.

## What the reference does

The cited walks: XATTR items (key type 0x18) are matched per inode; `user.DOSATTRIB`'s value is extracted and applied (:3424-3441); non-DOSATTRIB `user.*` xattrs are appended to a per-file attribute list (:3544-3602); the `btrfs.compression` property value maps to a codec enum {zlib=1, "zlo"=2, zstd=3} (:3517-3543).

## What our core does

`cr_btrfs_engine.cpp` dispatches only INODE_ITEM/EXTENT_DATA/DIR_ITEM/ROOT_ITEM/CHUNK (verified: no XATTR case). No attribute records are produced from xattrs; DOSATTRIB-stored Windows attributes are absent from recovered FileProperties.

## Gap

XATTR items are walked by the og and ignored by us. Falsifiable: a Btrfs fixture with `setfattr -n user.DOSATTRIB -v 0x20` on a file shows the Windows "archive" attribute under the og; ours shows none.

## Impact

User-observable attribute fidelity on Samba-shared Btrfs volumes (a primary Btrfs deployment pattern for mixed-OS NAS); lost metadata streams for user.xattr users.

## Reproduction / verification

Corpus-derived (cites first-hand). UNVERIFIED-BY-EXECUTION (mkfs.btrfs + setfattr fixture — generatable on Linux).

## Fix plan

1. Add KEY_XATTR (0x18) to the item dispatch in `cr_btrfs_engine.cpp`.
2. Parse the xattr name/value pairs; special-case `user.DOSATTRIB` → Windows attribute bits on FileProperties.
3. Surface other `user.*` xattrs as attribute records (the node-attribute shape from the og's per-file list; pick the closest existing core shape — likely the same mechanism NTFS ADS children use, N-011's shape).
4. Record the `btrfs.compression` property → codec tag mapping (zlib/zlo/zstd) alongside the per-extent FE_COMPRESSION truth (see I-014 (parallel leg register) B4 note — the og's property source and our extent source differ; keep ours as truth, add the property as metadata).

## Tests to add

- `Btrfs.XattrDosAttribApplied` — setfattr fixture; assert the Windows attribute bit lands on FileProperties (fails today).
- `Btrfs.UserXattrRecordsEmitted` — non-DOSATTRIB user.* xattr; assert an attribute record.
- `Btrfs.CompressionPropertyTagged` — btrfs.compression property; assert the codec tag.

## Related findings

N-011 (NTFS ADS shape precedent), F-B4/B5 notes folded into I-014 (parallel leg register) (compression source difference; lzo probe), M-020 (parallel leg, rotating mirror index) (multi-stripe — same engine).

## Open questions

Whether the og emits xattr records for *deleted* inodes too (quick-only DLL — the Complete searcher is a stub, I-003); probe the quick path before pinning the deleted behavior.
