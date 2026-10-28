# MINOR N-201 — FAT quick-open media-word flavor refinement absent: the og reads FAT1/FAT2 first sectors and RE-FLAVORS on FAT32-shaped media words (last read wins); combined with S-064's bracket, an internally inconsistent volume is re-flavored by the og and refused/mis-decoded by the core

**Severity:** MINOR
**Module:** fat (engine-open flavor refinement)
**Parameters affected:** B1 (detection correctness on torn/hybrid volumes), A25 (engine algorithm steps)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` FUN_180021000 (:26318-26350 region — the quick-open consumer: reads 1 sector at `reserved + base` (FAT1), tests `FAT[0] == 0xFFFFFFFF` / the FAT32 shape family (`(FAT[0] & 0xFFFFFFF) == 0xFFFFFF8 && (FAT[1] & 0xFFFFFF8) == 0xFFFFFF8 || (FAT[1] & 0x7FFFFF8) == 0x7FFFFF8`) → flavor 2; else `FAT[0] == 0xFFFFFFF8` → flavor 1; then repeats the shape test at `spf + reserved + base` (FAT2) and OVERWRITES the flavor again — last word wins)
**Core references:** audited `34d6cf10` — `src/core/fat/cr_chain.cpp:646-666` (no media-word read at open; the flavor stands as computed) and `:184-216` (`fat_table_validate`: Fat16's exact `0xFFFFFFF8` media check refuses the volume the og would re-flavor — M-015's ported check)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** N-188 → **N-201** (2026-10-29, the S56 lane's third collision rider: the retired m119-window N-188 (renumbered into the seat at 23:22:29 UTC) owns it per SA-9; the seat was taken before this filing pushed - the doc_truth_gate's active-vs-ledger check caught it live; no content change)

## Summary

After the BPB gates, the og's quick-open reads the first sector of FAT1 and FAT2 and re-assigns the flavor from the media words' shape (FAT32-shaped → 2, FAT16 canonical → 1, FAT2's test overwriting FAT1's — the last read wins). The core has no equivalent refinement; an internally inconsistent volume (string/bracket say FAT16, FAT media words FAT32-shaped) is re-flavored to FAT32 and scanned by the og, and refused by the core's exact Fat16 media check (`ERR_BAD_MAGIC` unless the mirror validates).

## What the reference does

FUN_180021000's post-gate refinement (the :26318-26350 region re-read this session): two single-sector reads (FAT1 at `reserved + base`, FAT2 at `spf + reserved + base`), each followed by the shape tests above, the flavor variable overwritten after the second read — the og trusts the FAT CONTENT over the BPB's self-description when they disagree.

## What our core does

The flavor is fixed at `validate_boot` (bracket — S-064's subject) and never revisited at open; the M-015 whole-table validation later applies the exact `media != 0xFFFFFFF8 → invalid` check for Fat16 and refuses the volume on mismatch (mirror promotion aside).

## Gap

Falsifiable: a volume with "FAT16   " + valid FAT16 BPB shape but FAT32-shaped media words (torn/hybrid — real after interrupted format conversions): og re-flavors to 2 and scans; core refuses (ERR_BAD_MAGIC) or (post-S-064-fix, string-driven FAT16) mis-validates.

## Impact

Narrow (torn/hybrid volumes) but the outcome is volume-level refusal vs. og recovery. Interaction note: this finding's fix should land WITH S-064's — both touch the flavor-decision point, and the og's total order is: string flavor first (FUN_18002f970), then media-word re-flavor (FUN_180021000).

## Reproduction / verification

Decompiled evidence (the two reads and the shape-test family located and the constants resolved this session — the 0xFFFFFFF8/0xFFFFFFFF/0x7FFFFF8 masks are the canonical FAT media families). Core side: the absence + the exact media check quoted at `34d6cf10`. The precise line numbers within FUN_180021000's body drift with Ghidra output; the function-level decode and the constants are the evidence.

## Fix plan (for the engineering team)

1. After `boot_passes_open_gates` (post-S-064's string-first fix), add the refinement step: read the first sector of FAT1 (and FAT2 when present), apply the og's shape tests, and adopt the indicated flavor when it disagrees with the string/bracket decision (last-read-wins, og-exact).
2. Relax M-015's exact Fat16 media check accordingly (family-tolerant when the shape test indicates the other flavor) or keep it as the final gate with the re-flavored flavor — decide and register.
3. Files: `src/core/fat/cr_chain.cpp`.
4. Acceptance criteria: the hybrid fixture (FAT16 string + FAT32 media words) scans as FAT32 with the og's chain results; consistent volumes byte-identical.

## Tests to add

- `FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32` — RED today.
- `FatEngine.Fat2ReadWinsOverFat1` (the last-read-wins order pinned).
- `FatEngine.ConsistentVolumeUnreflavored` (control).

## Related findings

- S-064 (the string-vs-bracket selection — land together; the combined order is the og's true decision sequence).
- M-015 (retired; the whole-table validation whose exact media check is the refusal site).

## Open questions

The exact mask family semantics (`(FAT[1] & 0x7FFFFF8) == 0x7FFFFF8` — the FAT32-with-flag variant) — one re-read of the two test conjunctions to transcribe the masks exactly into the port; the function-level behavior is already verified.
