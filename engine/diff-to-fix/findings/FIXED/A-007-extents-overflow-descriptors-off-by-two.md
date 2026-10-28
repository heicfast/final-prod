# ACUTE A-007 — extents-overflow descriptors parsed at key+10 instead of key+12 (off-by-two)

**Severity:** ACUTE
**Module:** hfsplus
**Parameters affected:** B2 (recovery correctness — fragmented-fork content), A26 (extents-overflow tree decode)
**Corpus references:** TN1150 "Extents Overflow File Key" (verbatim, fetched from
developer.apple.com/library/archive/technotes/tn/tn1150.html this session):
`struct HFSPlusExtentKey { UInt16 keyLength; UInt8 forkType; UInt8 pad; HFSCatalogNodeID fileID; UInt32 startBlock; };`
with "Keys in the extents overflow file always have the same length,
kHFSPlusExtentKeyMaximumLength (10)" — 10 is the key DATA length (forkType 1 +
pad 1 + fileID 4 + startBlock 4); the on-disk key occupies **12** bytes
including the keyLength field, and the 64-byte `HFSPlusExtentRecord` (8
descriptors) follows at **key+12**. Session-2 evidence quote
(`evidence/decompiled-quotes/ext4-hfsplus-session2.md`): "extents-overflow key
{10, cnid, startBlock} :7931-35" — same layout, corpus-confirmed.
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp` — the live
extents-overflow walk (step 1) and the S-021 journal extents branch both read
the 8 descriptors from `rec + 10` with the length gate `len < 10 + 64`
(discovered during the S-003/S-021 verification pass at `2aa4530`)
**Status:** NEW-FINDING (filed during the S-021 verification pass) — CONFIRMED-DIVERGENCE

## Summary

The core's HFS+ extents-overflow record parser reads the eight extent
descriptors from `record + 10`. The extent key occupies 12 bytes (the 2-byte
keyLength field + 10 bytes of key data), so the descriptors actually start at
`record + 12`. Every overflow record is therefore decoded from two bytes
early: the first parsed "descriptor" is built from the key's startBlock field
plus the top half of the real first descriptor.

## What the reference does

TN1150 (verbatim, quoted above): the overflow record is a 12-byte key
followed by a 64-byte HFSPlusExtentRecord. The og's HFS+ engine
(HFSPlusSearchDLL, hfspluslib port) reads this tree with the same layout
(session-2 evidence :7931-35).

## What our core does

`for_each_record` on the overflow leaf calls the parser with `rec+10` as the
descriptor base and gates on `len < 10 + 64` (74) instead of `len < 12 + 64`
(76). Two consequences, both wrong on real volumes:

1. The first parsed descriptor's `start_block` is assembled from the key's
   startBlock field plus the top half of the real first descriptor —
   typically a huge block number whose byte offset lands far outside the
   volume, so `resolve_file_extents` clamps its length to zero and the extent
   is **silently dropped** (a fragmented file loses its 9th+ extents and
   recovers truncated content); when the garbage happens to land in range,
   the file recovers **wrong bytes**.
2. The last parsed descriptor reads 2 bytes past the record's real end —
   benign within a node (the offset array follows) but still wrong data.

## Gap

Falsifiable claim: an HFS+ volume whose file F has a data fork with 8 initial
extents plus one extents-overflow record (extent 9 = block 12) yields, under
the og, a recovered F with 9 resolved extents — extent 9 at block 12's byte
offset. The core (pre-fix) resolves only the 8 initial extents; the 9th is
dropped or corrupted.

## Impact

- Every file with a fork longer than 8 extents (fragmented media files —
  exactly the large recoverables users care about) recovers truncated or
  wrong content **silently** — no error, no log, plausible size from the
  catalog record.
- Zero test coverage: the base fixture's extents-overflow fork is empty
  (`write_fork(vh + 0xC0, 0, 0, 0)`) and no test ever populated one — which
  is how the bug survived 396+ green tests.

## Reproduction / verification

- TN1150 verbatim quote (fetched this session, URL above) — the layout table.
- Core-side: the two parse sites (`rec + 10` in the live walk and the journal
  extents branch) — direct code cite.
- Executed: the fail-before run for `ExtentsOverflowRecordResolvesFragmentedFork`
  (this fix's test) on the pre-fix tree — 8/9 extents resolved, the 9th
  dropped; `evidence/executions/a007-extents-off-by-two.md`.

## Fix plan

1. Both sites: descriptors at `rec + 12`, length gate `len < 12 + 64`.
2. Add a fixture with a populated extents-overflow tree (header node + leaf
   with one overflow record) and a fragmented file (8 initial extents + the
   overflow extent), asserting the 9th extent resolves at the correct byte
   offset.

## Tests to add

- `ExtentsOverflowRecordResolvesFragmentedFork` (fail-before/pass-after).

## Related findings

- S-021 (the journal extents branch shares the parser — fixed in the same
  change).
- M-011-family parse-constant findings (same failure class: a wrong constant
  silently mis-decoding a metadata structure).

## Open questions

None; the layout is TN1150-verbatim.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** `bddf4df` (filed and fixed as A-005 in that commit; twice-renumbered A-005→A-006→A-007 after the ID collisions with the record-span and rawfixer-4Kn findings)
**Verified by:** HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork — fail-before executed on the pre-fix tree (extents 8/9, the overflow extent silently dropped by the bounds clamp; `evidence/executions/a007-extents-off-by-two.md`), pass-after (9/9 at the correct byte offset); full suite green.
**Resolution notes:** CONFIRMED-DIVERGENCE, fixed. Both parse sites (the live overflow walk and the S-021 journal extents branch) now read the eight descriptors from `rec + 12` with the `12 + 64` length gate — the TN1150-verbatim layout (12-byte key incl. the keyLength field + 64-byte HFSPlusExtentRecord). The new fixture populates the extents-overflow tree for the first time in the suite's history (header node + one-record leaf) and pins the 9th-extent resolution that the old code silently dropped.

---

*Citation repair (session-8, 2026-10-28): this finding's two evidence cites read
`a006-extents-off-by-two.md` (:79, :110) — a filename that never existed (the
evidence file is `a007-extents-off-by-two.md`, renamed through the A-005→A-006→A-007
ID-collision merges `2f61564`/`3b81dbc` without the cites being updated). Both cites
now point at the real file; the content cited (the fail-before execution log) is
unchanged and verified present. SUMMARY.md:3660's historical `a005-…` spelling in
the session-2 delta text is left as the dated record.*
