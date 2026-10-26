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

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — "fix: S-021 + S-003 - HFS+ journal deleted-record harvest")
**Verified by:** HfsPlus.JournalHarvestEmitsDeletedRecords, HfsPlus.JournalDifferentialDifferentFileIdKept, HfsPlus.JournalBadHeaderGates, HfsPlus.JournalWrapHandled; fail-before executed (3/4 fail on the stashed pre-fix engine) — `evidence/executions/s021-failbefore-journal.md`; full suite 423/423 Linux ASan; every cited decompile span re-read first-hand at `03ad46c` this session before coding.
**Resolution notes:** CONFIRMED-DIVERGENCE, fixed. `journal_harvest_pass` (in `cr_hfsplus_engine.cpp`) implements the decoded mechanism: VH+0x0C journalInfoBlock → JournalInfoBlock (TN1150 BE: 'jnlx' magic, byte offset, size) → jnl_header (LITTLE-endian jnl.c layout: 0x4A4E4C78 magic, 0x12345678 endian, start@+0x10, size@+0x18 cross-checked, sizes at +0x20/+0x28 — the corrected magic from the S-003 addendum); ring walk from start with wrap-aware split reads (images spanning the ring end reconstruct — pinned by the wrap test), the transaction test `first_u16 == (u32@+0x20 >> 4) - 1`, entry list at block+0x20 with 16-byte stride and the og's count-1 loop, per-entry cancel polls, leaf-only images (kind −1, height 1 — the FUN_180010720 gate subset), fork classification in og order (catalog→extents→attributes→allocation→startup) by target address = size_b × entry block, the cnid==parentCNID skip, the differential against the live catalog (kept iff not-found-live or fileID differs), extents-branch records feeding a {cnid→extents} fallback map for empty forks, and emission flat under root_deleted (og kind-5) with allocation-bitmap grading (the display-tier superset discipline; the og grades downstream — OQ-S2M-1) and the N-028a global sink filter (dot-prefixed + "HFS+ Private Data" — the og's sink is one callback, journal records pass it too). Endianness note recorded in the evidence log: journal-internal structures are LE (the og's ushort loads are native) — the first draft's BE fixture reproduced the exact silent-zero-harvest failure mode the open question warned about. Safety hardenings over the og: the ring walk is bounded by one full ring of advances (the og's loop can cycle forever on a corrupt ring — observed defect), image sizes are capped, a damaged journal degrades to no-harvest (never a scan failure). The transaction-magic derivation stays OPEN (mechanically decoded + pinned by fixtures that encode the same formula; a wrong constant yields silent zero-harvest, never wrong results) — tracked in OQ-S2M-1's probe list. PARITY-DELTAS D-56 registers the model.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — together with A-005, whose discovery and fix were prerequisites: the harvest's node images are TN1150 records-UP nodes)
**Verified by:** fail-before — with the harvest call disabled on the fixed tree, `HfsPlus.JournalHarvestEmitsDeletedRecords`, `HfsPlus.JournalWrapHandled` and `HfsPlus.NonLeafImagesSkipped` all FAIL (zero journal activity); post-fix the four journal tests + `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat` pass and the full suite is 401/401 Linux ASan. The A-005 fail-before run additionally proves the substrate: with the corrected fixtures the pre-fix reader fails the live-walk tests.
**Resolution notes:** The og's `CHFSPlusJournal::GetDeleteRecord` (:13637-14252) re-verified first-hand this session, gate by gate, before implementation — including three corrections to the finding's text (all recorded in the evidence addendum, s3-hfsplus-inventory.md §3a): (1) the transaction-header test reads the FIRST U16 (`*local_c8` is ushort*; Ghidra's `(uint)` cast widened it in the finding's rendering) — `u16@blocklist+0 == (blhdr_size >> 4) - 1`; (2) the entry layout is `{u64 target volume block @+0, u32 image size @+8, u32 next @+12}` (16-byte stride, `local_d0 + 8` ushorts) — the earlier legs' `{?, size@+4, target u16@+8}` mis-decoded `local_d0[4]`, which is the node SIZE low half passed to the installer, not a target block; (3) the walk origin and size live at jhdr+0x10/+0x18 with the JIB carrying the journal base at +0x24 and size at +0x2C (both BE; the jhdr is LE-stored per jnl.c with magic 0x4A4E4C78 'JNLx' + endian 0x12345678 — cross-checked == the JIB size). Implemented: `journal_harvest` in cr_hfsplus_engine.cpp — JIB gates (flags bit 0; base/size; the VH attributes journaled flag 0x2000 per the og's constructor), jhdr gates (magic/endian/size cross-check/bounds), the ring walk (probe by block_size, block lists by blhdr_size with the u16 header test, images serially by size, wrap modulo journal size, termination at the walk origin + a corruption step-cap), the og's node-image validator (`FUN_180010720` semantics incl. the lenient mid-chain behavior), leaf-only harvest, fork classification by VOLUME address ranges in the og's order (catalog → extents; attributes/allocation/startup images have no deleted-record surface), catalog-branch differential (keep only records the live catalog lacks, or holds under a different fileID — implemented against the collected live keys, the same result set as the og's per-record live-tree search), the cnid==parentCNID in-map skip, the extents side map ({cnid → descriptors} from extents-overflow images, applied to empty-fork harvested records), deleted-node emission under root_deleted, and the freed-node sweep deduped against harvested cnids (the sweep is a documented core superset — both surfaces finding the same record now emits once). Deliberate notes registered in PARITY-DELTAS D-52: the ring-read corruption step-cap (the og relies on the ring's own termination — a hostile header could loop; bounded, never silently), the `journal_harvest=false` off switch, and `Confidence::Unknown` on harvested records with content (the og emits ungraded; grading stays downstream). Tests: `HfsPlus.JournalHarvestEmitsDeletedRecords` (ghost + side-extent gone2 + differential readme.txt + selfthread skip + sweep coexistence), `HfsPlus.JournalWrapHandled` (image bytes straddling the ring end, reassembled through the wrap), `HfsPlus.NonLeafImagesSkipped` (valid index-kind image counted but never harvested), `HfsPlus.JournalHarvestDisabledByFlag`. The finding's open question (the `(blhdr_size >> 4) - 1` constant's derivation from a real journaled volume) remains the bench harness's vehicle — no mkfs.hfsplus -J in-container; the fixture encodes the og's reader exactly, and the decode chain is recorded for the harness pass.

## Convergence postscript (DF4, 2026-10-26)

Two implementations landed in the same window (the s5 leg's `journal_harvest_pass`
with S-003's framing correction — no live-tree overlay, the consumption-side
evidence stands — and the DF4 leg's independent decode). Reconciled onto one:
the s5 pass structure (count-1 entry loop, newest-wins map, the four-tree
classification, allocation-bitmap grading, the S-003 disposition) kept as the
base; the DF4 decode's corrections applied on top — the og's JIB offsets
(**base BE-u64@+0x24, size BE-u64@+0x2C**: FUN_18000db70 swaps exactly [0..3],
[0x24..0x2B], [0x2C..0x33] — the three BE fields the og reads; the ring reader's
raw longlong arithmetic reads *(u64*)(jinfo+0x24) as the journal base; the s5
draft's +8/+16 rendering is superseded) and the og's flags gate (bit 0) added
alongside the real-format 'jnlx' magic check; the node-image validator extended
to the og's full gate (nonzero header word, the free-space sentinel == 14, the
ascending offset chain — which is what exposed **A-005**, the record-span
inversion, fixed in the same arc); the harvest moved before the freed-node
sweep with the harvested-cnids dedup (both surfaces finding one record emit
once); the `journal_harvest=false` off-switch pinned by test. The residual
open question (the `(blhdr_size >> 4) - 1` constant's derivation, and the final
JIB-offset confirmation) remains the real-volume bench harness's vehicle — no
mkfs.hfsplus -J in either container.

---

## Verification addendum (closing session, df5 lineage — corrections to the landed fix)

The parallel legs' fix (c75f646 arc) was independently verified against this
session's complete first-hand corpus decode (CHFSPlusJournal ctor/walker
disassembly + the TN1150 journal section fetched verbatim from
developer.apple.com) — the full decode record lives in
`evidence/executions/s003-s021-verification-corrections.md`. The walk mechanics
were confirmed og-exact (ring walk from jhdr+0x10, the max_blocks probe marker,
the count-1 entry loop, the fork classification order, the differential, the
wrap-aware split reads). Four divergences in the LANDED code were corrected:

1. **JournalInfoBlock layout (CRITICAL — silent zero-harvest on every real
   journaled volume):** the landed code required a 'jnlx' magic at JIB+4.
   TN1150 verbatim: `struct JournalInfoBlock { flags; device_signature[8];
   offset; size; reserved[32]; }` — device_signature[8] occupies +4..+0x23
   (reserved, zeros on internal journals); there is NO magic field. The og's
   FUN_18000db70 swaps exactly [0..3]/[0x24..0x2B]/[0x2C..0x33] and the ctor
   never compares +4 (the 0x4A4E4C78/0x12345678 magic gates the 44-byte
   journal HEADER at jbase — a different structure). Both parse gates and
   fixtures corrected; the no-magic discriminator poisons the whole
   device_signature region to prove the bytes are ignored.
2. **Missing og entry gates:** kHFSVolumeJournaledBit (VH attributes & 0x2000,
   og :13456) and kJIJournalInFSMask (JIB flags bit 0, og :13513) — both added.
3. **Validator strictness:** a mid-chain offset-table violation REJECTS the
   image (asm-exact: 0x18001080c/:812 jump to 0x18001082a, past the
   `mov edi,1` success flag at :825) — the landed `break` was lenient.
   Also: the og's marker constant (size_a>>4)-1 is now DERIVED (block_list_
   header.max_blocks = (blhdr_size-16)/16 per TN1150 + jnl.c) — the open
   question is closed.
4. **Missing surfaces:** the attributes branch (og :14096 — decmpfs
   size-only fallback, FUN_180009c10 :7840-42 frees the payload) and the
   header-invalid fallback salvage ladder (:1715-1760 + FUN_180003ea0, OQ-S003
   RESOLVED) — both implemented. Plus the in_fork u32-overflow fix.

The og's count-1 entry loop (vs TN1150's inclusive binfo[1..num_blocks] replay)
is replicated deliberately for parity and noted in-code. Fail-before: 6 tests
red on the pre-correction tree (the three pre-existing journal tests +
JournalInfoBlockHasNoMagicField + JournalAttributeDecmpfsSizeFallback +
JournalFallbackSalvageWhenHeaderInvalid). Pass-after: 11/11 journal tests,
suite 491/491 Linux Release.## Convergence addendum 2 (final-sweep session, 2026-10-26)

A third independent decode (from-scratch this session, corpus 03ad46c first-hand)
landed two corrections the converged implementation still carried, now fixed forward:

1. **The "cnid == parentCNID skip" rule does not exist.** The audit's reading
   of `:13961-62` mis-parsed the operand: `puVar25` is the record DATA pointer
   (FUN_18000f2d0 = the record reader — its 0x58/0xF8 copy sizes prove it
   returns bodies, not keys), and `*(int*)(puVar25 + 4)` is ushort* arithmetic
   = DATA+8 bytes = the canonical fileID. Both comparison operands are the
   fileID; the rule is the og's harvest-map dedup {key, fileID} — same key +
   same fileID replaces (newest image wins), same key under a DIFFERENT
   fileID keeps BOTH entries (the map's comparator covers both fields).
   Corrected in journal_harvest_pass (the kept map is now keyed
   {parent, name, cnid}); pinned by JournalSameKeyDifferentFileIdBothKept
   (fail-before: the pre-correction newest-wins map emitted one).
2. **The attributes branch is consumed og-side.** The converged pass counted
   attributes-branch leaf images but dropped their records; the og harvests
   them into volume+0x78's map (:14085-14145, not-found-only differential)
   and consumes them as the decmpfs fallback for harvested files
   (FUN_180009f40 :7937-82). Wired: journal attribute records with absolute
   image-relative value offsets; harvested files with empty data forks
   resolve mode-3 (zlib inside the attribute value) and mode-4
   (resource-fork offset) content through the same codec-extent contract as
   the live path. Pinned by JournalAttrDecmpfsFallbackFeedsEmptyForkFile
   (fail-before: journal_attr_records was never counted).
   Also fixed en route: a merge-mangled decmpfs layout comment in the
   S-002 header block, and two leaked `>>>>>>>` conflict markers in
   worklog.md.

Suite 466/466 Linux Release + 466/466 Linux ASan (leaks ON) with both
corrections. The transaction-gate/size-cross-check constant-derivation
question and the fallback-salvage decode are consolidated in
tracked-open/OQ-S003-journal-fallback-salvage.md (Q2 appended).