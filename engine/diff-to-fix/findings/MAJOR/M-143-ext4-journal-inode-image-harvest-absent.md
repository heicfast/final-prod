# MAJOR M-143 — ext4 journal inode-image harvest branch absent: the og's jbd2 pass treats each journal block as EITHER a directory block OR an inode-table image (extent-gated inode scan feeding the iNode%N finalize); the core parses directories only — journal-recoverable files whose directory blocks are gone are invisible

**Severity:** MAJOR
**Module:** ext4 (journal salvage)
**Parameters affected:** B2 (recovery correctness — a recoverable-file class invisible), A26 (tree walk logic), A25 (engine algorithm steps)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` FUN_180005e50 (:4388-4573 — the per-block classifier; the inode-image loop quoted below), fed per-block by FUN_18000e5b0 (:12230-12250); the orphan finalize consumer FUN_1800041e0 (:2982-2993, "iNode%I64d" naming)
**Core references:** audited `34d6cf10` — `src/core/ext4/cr_ext4_engine.cpp:867-925` (journal pass: `parse_one` → `parse_dir_block` ONLY; no inode-shaped record scan anywhere in the journal path)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's jbd2 journal harvest classifies every in-range journal block: first the leading '.'/'..' directory-entry pair test (the directory model), and — on failure — an inode-table-image scan that walks the block at inode-size stride, accepting records with i_size != 0, EXTENTS_FL set, extent magic 0xF30A and eh_entries != 0, then emits file/dir records from them through the same machinery the orphan finalize consumes ("iNode%N" naming). The core's journal pass implements only the directory model. Files whose only surviving metadata is a journaled inode-table image (ordered-mode writes after the directory block was rewritten) are recovered by the og and invisible to the core.

## What the reference does

FUN_180005e50 (:4388-4424, re-read first-hand this session):

1. The '.'/'..' test (:4388-4411) — a block passing it is a directory image (parent-map insert, return).
2. Otherwise the inode-image scan (:4414-4424):
```c
iVar10 = 0;
while ((…cancel…) && (iVar10 < *(int *)(param_1 + 0x58))) {          /* stride < block_size */
  puVar11 = (ushort *)(iVar10 + param_2);                             /* record base */
  if (((*(int *)(puVar11 + 2) == 0) ||                                /* i_size_lo(+4) == 0 → skip */
       ((*(uint *)(puVar11 + 0x10) & 0x80000) == 0)) ||               /* EXTENTS_FL(+0x20) unset → skip */
     ((puVar11[0x14] != 0xf30a || (puVar11[0x15] == 0)))) {          /* extent magic(+0x28) bad / eh_entries(+0x2A)==0 → skip */
    iVar10 = iVar10 + *(int *)(param_1 + 100);                        /* += inode_size */
  } else {
    … the dir branch (mode 0x4000) or the file branch …
```
3. The file branch (:4514-4573) copies the 128-byte inode into the engine's record list (+0x4b8) and links it into the +0x4c8 map — the same structures the orphan finalize (FUN_1800041e0 :2982-2993) and reachability checks consume; the record's display name is the "iNode%I64d" form.

jbd2 journals metadata in ordered and data=journal modes — inode-table blocks are journaled on create/truncate/chmod, so recently-created files whose directory entries were later rewritten (the classic accidental-deletion window) survive as journal inode images even when no directory block survives.

## What our core does

`cr_ext4_engine.cpp:867-925` — journal pass 1: `parse_one` calls `parse_dir_block` per block (the M-037/S-004 directory model with the journal-salvage gate set); on rejection the block contributes nothing. No inode-stride scan exists in the journal path (grep-verified: no inode_size-stride loop over journal buffers).

## Gap

Falsifiable: an ext4 image with a jbd2 journal whose in-range block is an inode-table image (a regular-file inode: size != 0, EXTENTS_FL, valid extent header with entries) and no leading '.'/'..' pair: the og surfaces a file record (named iNode%N) with the inode's extents as content; the core emits nothing from that block (the dir parser's majority gate rejects the image bytes).

## Impact

A recoverable-file class invisible to the core — the sibling surface of S-004's directory-block harvest (which was filed SEVERE when absent and landed); the inode-image branch is the og's second recovery surface from the same journal and covers the no-directory-block-survives case. Rated MAJOR (not SEVERE) only because the og-side emission chain's user-visible grouping (which tree branch the records surface under) has an unresolved decode — the recovery-class loss itself is certain.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: the classifier's two branches, the inode-image gate set (all five terms quoted), the stride loop bound, and the +0x4b8/+0x4c8 consumers located; the per-block feed (FUN_18000e5b0 :12230-12250) confirmed. Core side: the journal pass quoted at `34d6cf10`; the S-004 fixture family in the tests confirms the directory-only scope.

## Fix plan (for the engineering team)

1. In the journal pass, extend `parse_one`: after `parse_dir_block` rejects, run the inode-image scan — stride `inode_size` (already decoded), gates: `i_size_lo != 0 && (i_flags & EXTENTS_FL) && extent magic 0xF30A && eh_entries != 0`; mode 0x4000 → the dir record path, else the file record path.
2. Name the emitted records with the og's "iNode%N" convention (the existing orphan-finalize naming — reuse it; the finalize path is the model).
3. Wire the extents: the record's content extents come from the inode's extent header (the engine's existing extent walker applies — bounded by the P1-11 budget; note S-065's ladder is a different, non-extents path).
4. Files: `src/core/ext4/cr_ext4_engine.cpp`; fixtures in the ext4 journal test home (model on the S-004 fixtures).
5. Acceptance criteria: the inode-image fixture (no dir pair) surfaces the file with correct extents and the iNode%N name; the dir-pair fixture unchanged; suite green.

## Tests to add

- `Ext4.JournalInodeImageSurfacesFile` — RED today.
- `Ext4.JournalInodeImageDirModeSurfacesDirRecord` (the 0x4000 branch).
- `Ext4.JournalInodeImageZeroSizeSkipped` / `NoExtentsFlagSkipped` / `BadExtentMagicSkipped` (the gate set pinned).
- `Ext4.JournalDirPairTakesPrecedence` (a block that is both-shaped → directory model wins, og order).

## Related findings

- S-004 (retired; the directory-block harvest — the same journal pass's first surface).
- S-065 (the ext4 hang-safety finding — both touch `read_inode`'s extent consumption; land together to avoid double-churn).

## Open questions

The og-side grouping decode: which tree branch the journal-inode records surface under (the +0x4b8 list's consumer in the results-tree assembly) — one probe: trace the +0x4b8 list's reader in FUN_1800041e0's callers and check the branch label it lands under; upgrades this finding's precision (and possibly severity) but does not gate the fix.
