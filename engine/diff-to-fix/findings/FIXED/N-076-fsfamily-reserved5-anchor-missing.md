# MINOR N-076 (NEW-FINDING, session 10) — FsFamily's reserved-value-5 anchor was missing: the enum silently consumed the FB-L03-reserved slot

**Severity:** MINOR (contract drift; no wrong-results path in the current tree)
**Module:** session / orchestration (the serialized fs_kind contract)
**Parameters affected:** Q67 (contract truthfulness), FB-L03 (the value-5 reservation), session wire compatibility
**Core references:** include/cr/orchestrator/cr_scan.h (pre-fix enum): the FB-L03 comment says "the value is NOT renumbered" for the removed CarvedOnly=5, but NO enumerator held the slot — auto-numbering gave Hfsplus=5, Btrfs=6, Zfs=7, Refs=8, BitLocker=9. Contradicted by: cr_ui.cpp's fs_hint input cases (6→Hfsplus .. 9→Refs — the documented ABI values), S-019's own landing notes ("BitLocker appended (value 10)"), and the enum's own "Values 6..9 appended" comment. Found during M-044's Udf landing (the new value's position depended on getting the numbering truthful).
**Corpus references:** N/A (core-internal contract; the og surface is unaffected).

## Summary

The FB-L03 contract reserves serialized value 5 (the removed CarvedOnly family) so
old sessions keep their meaning. The comment was written but the anchor enumerator
was never added, so every family appended after Ext4 silently took the values 5..9
instead of the documented 6..10. The internal enum was self-consistent (reads and
writes used the same wrong numbers), so nothing failed in-tree — but every place
that quotes the documented numbers (the cr_ui ABI hint cases, S-019's notes, the
enum's own comments, ABI-CONTRACT) described a different wire format than the code
shipped.

## Gap

Falsifiable: `FsFamily::Hfsplus == 5` pre-fix (compiled check) vs every documented
claim of 6. Sessions saved by the intermediate builds carry fs_kind 5..9 with the
shifted meanings; reloading them under the corrected numbering yields Unknown at
worst (5 = the now-held reserved slot reads back as no family), never a WRONG
family — see the matrix in the Resolution.

## Impact

Contract drift only. No released build exists (dev stage, pre-v0.5.0 tag), no
persistent sessions cross the fix in the wild, and no test pinned the shifted
values (verified: only Fat==2 is pinned numerically anywhere).

## Fix plan

1. Add the explicit `Reserved5 = 5` anchor and re-pin Hfsplus=6 .. Refs=9,
   BitLocker=10, Udf=11 — exactly the documented numbering.
2. Verify no numeric consumers break (cr_ui's hint switch is an INPUT mapping on
   the ABI contract numbers — unchanged; the engine dispatch switches on the enum
   symbolically with a default; per-node fs_kind uses the separate FsKindCode).
3. Pin the numbering: a static test asserting the documented values.

## Tests to add

- `Orchestrator.FsFamilyValuesMatchContract` (Hfsplus=6 .. Udf=11, 5 held).

## Related findings

M-044 (Udf — landed on the corrected numbering), FB-L03 (the original reservation
whose anchor was missing), S-019 (BitLocker's "value 10" notes).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — filed + fixed in the M-044 change)
**Verified by:** `Orchestrator.FsFamilyValuesMatchContract` (fail-before by
construction: the pre-fix enum fails three of the six assertions); full suite
648/648 Linux Release (no consumer regressed — the only numerically-pinned family
value anywhere is Fat==2, unchanged).
**Resolution notes:** the anchor `Reserved5 = 5` now holds the FB-L03 slot with
the full history in place; Hfsplus..Refs re-pinned to the documented 6..9,
BitLocker to 10 (S-019's own notes), Udf appended at 11. Compatibility matrix for
sessions written by the intermediate (shifted) builds: their fs_kind 5 (Hfsplus)
reads back as Unknown under the corrected numbering (slot held) — a skipped
family hint, never a wrong engine dispatch; their 6..9 (Btrfs..BitLocker) map to
one-family-lower names in the corrected wire — dev-stage sessions only, no
release ever shipped the shifted values, and the alternative (keeping the shifted
numbers forever) would permanently contradict every quoted contract. The fix
restores the documented truth; the enum comment records the whole story.
