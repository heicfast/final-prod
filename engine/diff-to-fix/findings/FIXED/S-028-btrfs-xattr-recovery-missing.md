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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for "fix(S-028)")
**Verified by:** 3 tests in `tests/unit/test_btrfs_volume.cpp`: `Btrfs.XattrDosAttribApplied` (a user.DOSATTRIB "0x20" item lands the archive bit on the file's attributes; DOSATTRIB is applied, not a record), `Btrfs.UserXattrRecordsEmitted` (two user.* xattrs surface as `..xattr/<name>` children carrying the values as resident content — the N-011 ADS-children shape), `Btrfs.CompressionPropertyTagged` (a btrfs.compression=zstd item increments the property counter while the per-extent codec stays 0/raw — I-014 B4's truth separation pinned). Fail-before by construction: KEY_XATTR (24) had no dispatch case pre-fix (grep-verified in the finding; the xattr item died unhandled), so no fixture carrying an XATTR item could produce any of the three surfaces. Suite 484/484 Linux Release (481 + 3); clang-format 18.1.8 clean.
**Resolution notes:** KEY_XATTR (24, the og's type 0x18) added to the phase-1 item walk. XATTR items share the dir_item record shape (the kernel shares struct btrfs_dir_item across dir entries/xattrs/verity descriptors; the og parses them identically at :3424-3441 — data_len@+0x19, name_len@+0x1b, name@+0x1e, value@+0x1e+name_len): `user.DOSATTRIB` (name_len 0xE, the 14 bytes "user.DOSATTRIB" — byte-compared against the og's split reads :3437-3439) parses the Samba text value ("0x20" hex via strtoul base-0) onto `FileProperties::attributes` (OR'd with the directory bit); `btrfs.compression` maps {"zlib"→1, "zlo"→2, "zstd"→3, else 0 — the og's :3517-3543 exact strings, including the vendor's "zlo" misspelling} to a per-inode property tag counted in `stats.compression_property_files` (the property is METADATA alongside the per-extent codec truth per I-014 B4 — deliberately NOT written onto extents, which carry the extent's own compression flags); every other `user.*` xattr (name_len > 5, the "user." prefix — the og's :3544-3602 predicate) surfaces as a `..xattr/<name>` child node with the value as resident_data (the N-011 ADS-children shape, mirroring S-002's `..namedfork` precedent) and counts in `stats.xattr_records`. Deleted-inode xattrs: the walk collects per-objectid XATTR items into the same `inodes` map the INODE_ITEM phase fills — deleted inodes have no INODE_ITEM (mode 0) and emit no node, so their xattrs surface exactly when the inode does (the finding's open question: we emit records for every emitted inode, deleted included, keyed on the item walk — the og's quick-path probe stays OQ-territory but our Complete-path behavior is the superset that matches). Every fix-plan test from the finding's §Tests-to-add is present.
