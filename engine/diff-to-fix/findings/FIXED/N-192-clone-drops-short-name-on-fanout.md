# MINOR N-192 — FileProperties::clone() drops `short_name` — the hard-link fan-out's cloned nodes lose the 8.3 name (the ifileproperties-part1 census)

**Severity:** MINOR
**Module:** tree / session (the NTFS walk's hard-link fan-out; the 8.3 short-name carrier)
**Parameters affected:** D11 (node fields — the short_name field on cloned nodes), B10 (the name axes a cloned link exposes), the part1 census slot +0x08 SetShortName
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md` §1 slot +0x08 (SetShortName — the heap-copied short name) and §2 (the og's clone copies the whole props block — the contract the core's own comment cites); the census table: `evidence/s50-ifileproperties-part1-census.md`
**Core references:** audited `34d6cf10` — `src/core/tree/cr_node.cpp:49-80` (clone() copies name, name_raw, times, extents, both NTFS anchors, carve provenance, the hash — NOT `short_name`; read first-hand this session); `src/core/ntfs/cr_mftwalk.cpp:856-865` (the N-011a short-name capture on the canonical props), :981-992 (the fan-out clone: `(!flags.single_link_per_record && parents.size() > 1) || link_name != best` -> `props->clone()`), `src/ui/cr_ui.cpp:3795` (the preview clone, same class); the field persists via cr_records.cpp:156-157/:207-212 (nflags bit2)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`FileProperties::clone()` does not copy `short_name`. On the NTFS walk's
hard-link fan-out (and the UI preview clone), the cloned node loses the
8.3 DOS name that the canonical props captured — where the og's clone
copies the whole props block (a contract the core's own comment states at
cr_node.cpp:75-76). The field is a session-persisted axis (nflags bit2),
so cloned links' session records also lose it.

## What the reference does

The part1 artifact's slot +0x08 (SetShortName) is part of the persisted
props block; the og's clone semantics copy the block whole — every cloned
hard link carries the record's short name. (Census row: the core's own
comment at cr_node.cpp:75-76 "the og's clone copies the whole props
block", written for the OQ-SA-1 anchor twins.)

## What our core does

clone() (cr_node.cpp:49-80) enumerates its copies: record_ref, fs_kind,
sizes, times, attributes, flags, name, name_raw, resident_data, extents,
carve fields, ntfs_origin_lcn, ntfs_total_clusters, sha128/hash —
`short_name` is absent. The fan-out path (cr_mftwalk.cpp:981-992) then
overwrites name/size/times from the link's own $FILE_NAME but never
re-derives or copies the 8.3 twin. The capture at :856-865 grabs the
distinct DOS-namespace name only for the canonical props.

## Gap

Falsifiable: a multi-name MFT record fan-out under
`single_link_per_record=false` (or any `link_name != best` clone) yields
cloned nodes whose session records and query surface carry no short name,
while the og's cloned links carry the record's 8.3 name.

## Impact

Narrow and honest: the axis surfaces in session records and any 8.3-aware
consumer; the default config (`single_link_per_record=true`) suppresses
most of the fan-out population. Information loss on cloned links only —
MINOR.

## Reproduction / verification

Code-read class, both sides first-hand this session (the census lane
surfaced it; the lead re-verified clone() and the fan-out site); the
census table at `evidence/s50-ifileproperties-part1-census.md` (the two
NEW items), the og contract quote at cr_node.cpp:75-76.

## Fix plan (for the engineering team)

1. Architectural: none — one line.
2. Files: `src/core/tree/cr_node.cpp` (clone()).
3. Algorithm: add `p->short_name = short_name;` beside the name/name_raw
   copies (the og's whole-block contract; the other per-FS anchors are
   clone-safe by construction — the non-NTFS engines never clone, per the
   existing OQ-SA-1 comment).
4. Tests: `NtfsVolume.FanOutCloneKeepsShortName` — a two-name record (a
   WIN32_AND_DOS display name + a distinct DOS name) walked with
   single_link_per_record=false: expect the cloned link's props to carry
   the same short_name as the canonical node. RED before.
5. Contract/doc: the part1 census note in modules/session.md (the census
   discharges the under-cited flag; this finding owns the clone residual).
6. Acceptance: the new test green; the N-011a round-trip tests unchanged
   (the canonical path is untouched).

## Tests to add

`NtfsVolume.FanOutCloneKeepsShortName` (RED before).

## Related findings

N-011a/N-024 (FIXED — the field's own landing), M-007 (the fan-out
shape), OQ-SA-1 (FIXED — the anchor twins whose clone handling this
mirrors), N-131 (the parent-anchor persistence family).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (the S60 lane's N-192 landing — one-line carrier + the fan-out/session pins; SHA in the worklog entry and the ledger row)
**Verified by:** NtfsVolume.HardLinkFanOutClonesKeepShortName — RED pre-fix (the cloned POSIX link's `short_name` empty at the walk AND at the session roundtrip; the executed fail output quotes `posix2[0]->props()->short_name Which is: u""`), GREEN post-fix; the full NtfsVolume family 71/71; baseline suite 1460/1460 green before the lane's first change.
**Resolution notes:** Corpus re-verified first-hand before the change: ifileproperties-part1's slot +0x08 (SetShortName, the heap-copied 8.3 name) and §2's whole-block clone contract — the same artifact cr_node.cpp's own N-100 comment cites. Core re-verified: clone() at cr_node.cpp enumerated its copies with `short_name` absent; the capture at cr_mftwalk.cpp:915 and the nflags-bit2 persistence at cr_records.cpp:160/:218 confirmed the field is live on canonical props and lost only at the clone boundary. The fix: `p->short_name = short_name;` added to clone()'s copy set (the name/name_raw block — the field travels with the name axes), which repairs the NTFS walk's hard-link fan-out clones, the UI preview clone (cr_ui.cpp:3795 rides the same clone()), and the cloned links' session records in one line. The fix plan's optional exFAT/other-engine sweep: no other clone site exists (grep clone() — cr_node.cpp is the only props duplication path). No behavior other than the carrier: the default single_link_per_record mode never clones, and the fan-out tests (HardLinkFanOutOptIn et al.) stay green.
