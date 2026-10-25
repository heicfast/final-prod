# SEVERE S-003 — HFS+ journal not replayed: post-crash recent state unrecoverable

**Severity:** SEVERE
**Module:** hfsplus
**Parameters affected:** B2 (recovery correctness — recency), B6 (damaged-volume behavior), A26 (tree walk)
**Corpus references:** `re-workspace/artifacts/SearchDLL/hfsplus-engine--deep-dive.md` :43–45 (`CHFSPlusFileCache` + `CHFSPlusRawfile` — the journal-aware raw file reader; "the journal replay logic lives in journal.cpp per the source tree"); `re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (HFS+ section: embedded `journal.cpp`)
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp:356` (`journal_info_block = be32(vh + VH_JOURNAL_INFO)` — parsed, then unused); no journal read/replay anywhere in the module — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The reference reads the HFS+ journal (a circular hot-file protected by the journal info
block) and uses it to recover the **most recent metadata state** on dirty/crashed volumes.
Our core parses the journal-info block offset and stops: no journal read, no replay, no
recent-state recovery. On a volume that crashed mid-write, files created/renamed/moved in
the moments before the crash exist only in the journal — the og surfaces them, our core
shows only the last checkpointed catalog state.

## What the reference does

Deep-dive :43–45 (verified quote): `CHFSPlusFileCache` + `CHFSPlusRawfile` form the
journal-aware raw fork reader; the replay logic lives in the embedded hfspluslib
`journal.cpp`. The HFS+ journal is a circular buffer of metadata block images with
transaction sequence numbers; a recovery reader walks it (oldest→newest within the ring)
and applies still-valid block images over the checkpointed trees, yielding the
most-recent catalog/extents state.

## What our core does

`cr_hfsplus_engine.cpp:356` reads the journal info block number into
`out.journal_info_block` — the only journal-related line in the module (grep: one
occurrence of "journal"). The catalog walk reads only the on-disk (checkpointed) trees.

## Gap

Falsifiable claim: on an HFS+ volume with a dirty journal containing a newer catalog leaf
than the checkpointed catalog (file F created before the crash), the og's result set
contains F and our core's does not.

## Impact

- **Recency of recovery**: the primary scenario where users run a recovery product on an
  HFS+ volume is *after a crash* — exactly when the journal carries the newest state.
- Rename/delete recency also affected (stale names/deleted-state linger in our results vs
  the og's replayed truth — both directions of divergence).

## Reproduction / verification

- Corpus-derived: artifact quote (deep-dive :43–45); embedded source-tree listing
  (`journal.cpp` present in remaining-engines HFS+ section).
- Core-derived: grep evidence (journal_info_block unused; no journal code).
- Executed: not run (constructing a dirty-journal HFS+ fixture is expensive; the absence of
  any journal-reading code makes the behavioral delta deterministic from the two code
  states).

## Fix plan

1. Read the journal info block (`journal_info_block` already parsed): {magic 0x6a6e6c78,
   endian, start/end block pointers, size, checksummed trailer}.
2. Read the journal block range into memory (bounded, e.g. ≤ 64 MiB, mirroring the $J cap
   discipline in the NTFS module).
3. Walk transactions from `start` forward through the ring: each entry is {sector count,
   sector images...} with an optionally checksummed trailer; validate block checksums
   (default crc32, per the journal info flags).
4. Maintain a write-order overlay map: physical block → latest journal image. Apply the
   overlay when reading catalog/extents B-tree nodes (cheapest integration point: the
   node-read shim already used for the file cache).
5. Emit both replayed-state entries and (flagged, `DeletedState::Damaged`) checkpoint-state
   entries that the replay superseded — stale-snapshot salvage matches the recovery-product
   posture already used for ext4 journal names.
6. Files: `src/core/hfsplus/cr_hfsplus_engine.cpp` (+ a small `cr_hfsplus_journal.cpp` if
   the walker exceeds ~250 lines); header additions in `include/cr/hfsplus/`.
7. Tests: synthetic mini-journal fixture (hand-built 4-block ring with a newer leaf) —
   `Journal_Replayed_State_Wins`; `Journal_Bad_Checksum_Skips_Entry`;
   `Journal_Superseded_Checkpoint_Entry_Flagged`.

## Tests to add

(fix plan step 7 — all fail before, pass after)

## Related findings

- S-002 (attributes tree — the other missing HFS+ subsystem).

## Open questions

None material; the journal on-disk format is public (Apple TN1150 + hfspluslib source
structure documented in the corpus).

---

## Verification addendum (diff-to-fix remediation session — evidence decoded, fix deferred)

**Status:** CONFIRMED-DIVERGENCE (re-verified) — implementation deliberately not
rushed in this session; the following first-hand decompile evidence replaces the
artifact-level cites and is the next session's starting point. Nothing below is
guessed: every line was read from `HFSPlusSearchDLL_decompiled.c` this session.

New og-verified facts (CHFSPlusJournal, `hfspluslib\hfsplusjournal.cpp` trace
strings at :13417-14213; replay driver at :13660-13800):

1. **JournalInfoBlock gate** (read as a 44-byte block; error codes
   0x5004000000501 bad magic / 0x5004100000501 bad endian / 0x5004200000501 size
   mismatch, :13542-13554 and :1715-1722):
   `[info+0] u32 == 0x4A4E4C78` (Apple jnl.c `JOURNAL_INFO_MAGIC` — **the
   finding's fix-plan magic `0x6a6e6c78` is wrong**; 0x4A4E4C78 is 'JNLx' and
   jnl.c journal structures are LITTLE-endian, unlike the rest of HFS+),
   `[info+4] u32 == 0x12345678` (endian marker),
   `[info+0x18] u64 == volume.journal_size` (volume object +0x2c — the SAME
   field the ring wrap uses), `[info+0x10] u64` = journal start offset (printed
   by the "Journal Start Offset:0x" trace at :13705).
2. **Replay driver** (:13660-13800): reads the first block at the journal
   start; the transaction's entry list lives at block offset +0x20 as an array
   of `{?, u32 image_size@+4, u16 target_block@+8}` (local_d0 = buf+0x10 u16s;
   `*(u32*)(local_d0+4)` allocated and read per entry, `local_d0[4]` passed to
   the installer as the target block). Each image is read from the journal
   ring with the offset ADVANCING by the image size and WRAPPING via
   `offset %= journal_size` (volume+0x2c, :13743-13748) — the circular-buffer
   walk. Cancel is polled per entry (vt[0x10] + vt[0x88]).
3. **Image install**: every image is validated by `FUN_180010720` — the SAME
   B-tree node-kind gate ({-1,0,1,2}) the catalog reader uses — and then
   installed by `FUN_180010110(ctx, image, target_block, 1)` (call at :13789;
   the 4th arg is a constant 1 = "apply/replace" presumably). The overlay is
   therefore per-B-TREE-NODE keyed by block number, not per-raw-block: replayed
   images enter the tree readers as node-cache entries. **Remaining decode for
   the next session (small):** `FUN_180010110` (:12766-12850) — which tree(s)
   receive the install and whether the 4th argument is apply-vs-peek; and the
   first-block header fields before +0x20 (checksum/sequence — the entry list
   start is +0x20, i.e. a 32-byte transaction header).
4. **Fallback flow when the info block is invalid** (:1715-1760): the og does
   NOT give up — it allocates 0x8000, scans node-shaped images with the same
   `FUN_180010720` gate and installs them via `FUN_180003ea0` (a second
   installer!) — a recovery-grade salvage of raw node images from the journal
   region even without a valid info block. This is a SECOND og behavior the
   original finding did not record; the fix should implement it behind the
   same flag.

**Remaining scope for the fix (next session):** (a) implement the
JournalInfoBlock gate with the corrected magic 0x4A4E4C78 + endian + size
cross-check; (b) transaction-list walk with the ring arithmetic above +
per-entry cancel; (c) node-image overlay keyed by {tree, block number}
applied in the core's read_tree_node (the engine's single node-read choke
point); (d) the FUN_180010110 4th-arg semantic decode; (e) the info-block-
invalid fallback salvage; (f) tests per the original fix plan (mini 4-block
ring fixture). Estimated: one focused session.
**Finding text correction:** the fix-plan's `magic 0x6a6e6c78` must read
`0x4A4E4C78` (jnl.c JOURNAL_INFO_MAGIC, LE-stored) — the audit's value was
wrong; corrected here per the corpus-wins rule.

## Session-2 main-auditor leg — GetDeleteRecord differential-salvage reading (2026-10-25) — reconcile with the overlay model above

This leg decoded the same journal machinery independently and read the CONSUMPTION side the
overlay framing above does not cite. Quotes (`HFSPlusSearchDLL_decompiled.c`, full chain in
`../evidence/decompiled-quotes/ext4-hfsplus-session2.md`):

- `CHFSPlusJournal::GetDeleteRecord` = `FUN_180011300 :13637-14252`: ring walk from
  `jhdr.start`, **LEAF nodes only** (kind == -1, `:13799`), per-block owning-fork
  classification (:13809-17), and the deciding differential — for catalog leaves, the live
  tree is searched and the record is kept **only if not-found or the fileID differs**
  (`:13958-62`); extents entries not-found → volume+0x68 map (:13859); attribute records
  not-found-live → volume+0x78 map (:14096).
- Consumption `FUN_18000c8f0 :10017-10127`: the harvested records become FileNodes flagged
  deleted and are **emitted with grade 5** (:10069, :10083); **the live catalog tree is then
  walked unchanged** (:10122) — this line is the datum the overlay model must reconcile:
  if journal blocks were overlaid onto the live tree, the post-harvest walk would not be
  "unchanged". Extent/attribute harvests feed data-run and decmpfs-size **fallbacks**
  (`FUN_180009f40 :7937-82`), not tree replacement.

Both legs agree on: magic 0x4A4E4C78 LE + endian marker + size cross-check; ring arithmetic;
per-entry cancel; the info-block-invalid raw-node salvage fallback (a real second behavior —
this leg confirms it exists at `:1715-60` per the parallel leg's citation). The open question
for the fix session is which model the *salvaged-node installer* (`FUN_180003ea0`, the
parallel leg's "second installer") serves: delete-record construction or live-tree overlay.
The consumption-side evidence above (:10083 emit-grade-5, :10122 unchanged walk) favors
**differential salvage** as the primary user-visible behavior — recently-deleted files get
surfaced with extents/sizes; the og never "makes live entries newer". Recommend the fix
implement the differential first and pin `FUN_180003ea0` before any overlay work.

---

## Resolution

**Status:** FIXED (outcome parity via the S-021 substrate; the "replay" mechanism premise corrected; the header-invalid fallback residual tracked)
**Fixed in commit:** (this commit — "fix: S-021 + S-003 - HFS+ journal deleted-record harvest")
**Verified by:** the S-021 test set (HfsPlus.JournalHarvestEmitsDeletedRecords etc. — the journal-carried recent state now surfaces); fail-before/pass-after in `evidence/executions/s021-failbefore-journal.md`; suite 423/423 ASan; corpus re-verification cited in the S-021 Resolution.
**Resolution notes:** The finding's headline claim ("post-crash recent state unrecoverable: files created/renamed/moved in the moments before the crash exist only in the journal — the og surfaces them, our core shows only the checkpointed catalog") is CONFIRMED and now FIXED: with the S-021 harvest landed, a file whose only catalog record lives in a journal transaction image surfaces in the core's deleted tier exactly as the og's kind-5 emission serves it. The MECHANISM premise is corrected in the audit trail: the og does NOT overlay journal images onto the live catalog ("replay") — three converging first-hand reads settle it: (1) images install into a SCRATCH record (FUN_180010110 writes a stack-local `local_f8` — never the volume trees); (2) the emit driver walks the live catalog UNCHANGED after the harvest (:10122); (3) the "replay driver" the first addendum decoded at :13660-13800 is GetDeleteRecord's own ring walk (inside :13637-14252). The differential-salvage model (session-2 leg) is the correct reading — this session's implementation follows it. The finding's original fix-plan magic `0x6a6e6c78` was already corrected to 0x4A4E4C78 by the addendum (corpus-wins). Remaining scope, precisely tracked: the journal-header-invalid fallback salvage (:1690-1790, FUN_180003ea0 consumer undecoded) → `tracked-open/OQ-S003-journal-fallback-salvage.md`.
