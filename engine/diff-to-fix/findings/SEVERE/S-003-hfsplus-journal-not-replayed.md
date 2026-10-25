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
