# SEVERE S-021 — HFS+ journal deleted-record harvester missing: the og's PRIMARY deleted-file surface (GetDeleteRecord + FUN_18000c8f0 emit) has no core counterpart

**Severity:** SEVERE
**Module:** hfsplus
**Parameters affected:** B4 (deleted-file classification), B5 (orphan/deleted reconstruction), A25 (engine algorithm steps — a whole pass missing), B2 (recovery correctness — content of deleted files), Q47 (time-to-first-result is unaffected; total yield diverges)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c :13637-14252` (`CHFSPlusJournal::GetDeleteRecord` — the ring-walk harvester), `:10017-10127` (`FUN_18000c8f0` — the emit driver), `:13709-13713` (journal start offset trace "Journal Start Offset:0x"), `:13737` (transaction block-list header test), `:13748` (entry list at block+0x20), `:13779-13782` (ring wrap `% journal_size`), `:13784/:13789` (node-kind gate FUN_180010720 + scratch-btree install FUN_180010110), `:13799` (kBTLeafNode == -1 only), `:13809-13817` (tree-by-address-range: catalog→extents→attributes→allocation→startup), `:13939-14130` (catalog branch), `:13837-13937` (extents branch), `:13961-13962` (cnid≠parentCNID insertion rule), `:13970-13978` (HFSX/HFS+ collation switch on byte −0x31), `:10037-10041` (progress total = journal_count + 1 + folderCount + fileCount), `:10077-10089` (CHFSPlusFileNode construction, kind-5 deleted branch)
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp:525` (`journal_info_block` parsed and never used — grep "journal" in the engine yields this one hit), `:826-938` (the freed-node sweep — the core's ONLY deleted surface), audited commit `d4e5f5b` (core `34b3441` lineage)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's HFS+ engine contains a second journal consumer besides replay: a
**deleted-catalog-record harvester** that walks the journal ring, extracts
transaction block images, installs leaf-kind B-tree nodes into a scratch tree, and
emits every harvested catalog/extents record as a deleted-file result
(`CHFSPlusJournal::GetDeleteRecord` + the `FUN_18000c8f0` emit driver). This is the
og's primary deleted-file surface for HFS+ (the live catalog carries no deleted
records; the journal is where deletions remain visible). The core parses
`journal_info_block` and never uses it, and recovers deleted files only via the
freed-node sweep — a different, smaller surface. On any journaled HFS+ volume with
recent deletions, the og surfaces deleted files the core cannot.

## What the reference does

Mechanically decoded (all cites in the header; full walk-through in
`diff-to-fix/evidence/decompiled-quotes/s3-hfsplus-inventory.md` §3):

1. Start at the journal start offset (`*(u64*)(jinfo+0x10)`), buffer = max(block size,
   `jinfo+0x28`, `jinfo+0x20`).
2. Ring walk: a block whose first u32 equals `(u32@(jinfo+0x20) >> 4) − 1` is a
   transaction block-list header; entries live at block+0x20 as
   `{?, image_size u32@+4, target_block u16@+8}`; each image is read from the ring
   with wrap `% journal_size`.
3. Each image passes the B-tree node-kind gate and is installed into a scratch btree;
   only **leaf-kind** images (`*(char*)(image+8) == −1`, kBTLeafNode) proceed.
4. The image's target block is mapped to its owning tree by address range, tried in
   order catalog → extents → attributes → allocation → startup (extents chased via
   the 8 initial fork extents + extents-overflow records, key `{10, cnid}`).
5. Catalog-branch leaves: every record is parsed and inserted into the caller's
   {key → record} map unless the record's cnid equals the key's parentCNID; HFSX vs
   HFS+ key collation switches on a byte tag. Extents-branch: same into a second map.
6. The emit driver builds a `CHFSPlusFileNode` per harvested catalog record (0xD0
   bytes) and inserts it into the result tree's kind-5 (deleted) branch; progress
   total = journal_count + 1 + folderCount(VH+0x24) + fileCount(VH+0x20).

## What our core does

`cr_hfsplus_engine.cpp:525` parses the journal info block (magic/size/start) and
stores it; no other journal code exists (grep "journal" in the engine → 1 hit). The
deleted surface is the freed-node sweep (`:826-938`): allocation-bitmap-derived free
regions scanned for catalog-leaf remnants. The core therefore misses every deleted
record that lives only in journal images — the exact records the og harvests — and
conversely carries a sweep surface the og lacks (see I-014 for that superset side).

## Gap

Falsifiable: on a journaled HFS+ volume with recent deletions (journal not yet
overwritten), records of deleted files exist as journal transaction images; the og
emits them as deleted results; the core emits none of them. (Complementary direction:
the core's freed-node sweep finds records the og's journal walk may miss when the
journal has wrapped — the two surfaces are non-overlapping by mechanism.)

## Impact

Recovery correctness for deleted files on HFS+ — the product's flagship scenario.
Journaled volumes (default on macOS for two decades) carry recent deletions in the
journal; the core's yield on "empty Trash last week" volumes is strictly lower than
the og's for recently-deleted content. User-observable: missing results, not wrong
bytes.

## Reproduction / verification

Decompiled evidence (strength 2): the full mechanism with line cites above and in
the inventory §3; the method-name map (§0) ties `GetDeleteRecord` to the vendor's own
`\tFuction:` [sic] trace strings. Core side verified by code read + grep (1 journal
hit). NOT execution-verified: no HFS+ fixture with a populated journal exists in
either tree (the core's HFS+ tests are synthetic; building one requires hfsplus
tooling + a journaled mkfs — listed in tests below and in the fix plan).

## Fix plan (for the engineering team)

1. Add a journal-harvest pass to the HFS+ engine, gated on `journal_info_block`
   validity (the parse already exists at `:525`): ring-walk from the journal start
   offset with the transaction-header test (`first_u32 == (blksize_field >> 4) − 1`),
   entry images at block+0x20 (`{pad, image_size u32, target_block u16}`), ring
   wrap modulo journal size.
2. Install leaf-kind images (node kind at image+8 == −1) into an in-memory scratch
   map; map target blocks to trees by the volume header's fork address ranges
   (catalog VH+0x110, extents VH+0xC0, attributes VH+0x160, allocation VH+0x70,
   startup VH+0x1B0) with extents-overflow chase for out-of-range blocks
   (key `{10, cnid}`).
3. Emit harvested catalog records as deleted-kind nodes (the tree's deleted branch),
   skipping records whose cnid equals the key's parentCNID; honor the HFSX/HFS+
   collation switch. Extents-branch records feed a {cnid → extents} side map used to
   size/locate the harvested files' content.
4. Wire the pass into progress accounting (progress total += journal block count) and
   cancellation (per-block poll — the og ticks per record).
5. Tests: `HfsPlus.JournalHarvestEmitsDeletedRecords` (fixture: synthetic journal with
   one transaction image containing a catalog leaf with a deleted file record → the
   file appears as deleted); `HfsPlus.JournalWrapHandled` (image spanning the ring
   end); `HfsPlus.NonLeafImagesSkipped`; `HfsPlus.ParentCnidSelfRecordsSkipped`;
   end-to-end: mkfs.hfsplus -J + delete + scan (CI mac lane or checked-in fixture).
6. Acceptance: on a journaled fixture with K recent deletions, the core's deleted-file
   count ≥ K matches the og's .rsf for the same volume (bench diff).

## Tests to add

See fix plan §5 (four unit tests + the end-to-end journaled-volume bench).

## Related findings

- S-003 (HFS+ journal replay — the OTHER journal consumer: restores live-state; this
  finding is the deleted-record surface. Implement both on the same ring-walk
  substrate; S-003's evidence addendum decoded the replay mechanics).
- N-028/N-015b (fork selection — the harvester's emit path uses the same
  FUN_180009940 record decode; N-038's predicate note applies to harvested records
  too).
- I-014 (the core's freed-node sweep superset — the complementary surface).

## Open questions

The journal block-list header test constant `(u32@(jinfo+0x20) >> 4) − 1` is decoded
mechanically but its derivation (bytes-per-block >> 4 − 1 as the og's transaction
magic) is not documented anywhere in the corpus; verify against one real journaled
volume before shipping the gate (a wrong constant = zero harvest, silent).
