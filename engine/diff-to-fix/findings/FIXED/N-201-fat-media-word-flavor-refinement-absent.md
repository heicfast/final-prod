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

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S70 MINOR-first fix lane, 2026-10-29)
**Verified by:** `FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32` (RED with the implementation stashed — the pre-fix core refuses the hybrid volume, the finding's exact gap; GREEN post-fix — the volume re-flavors to flavor 2, the FAT32 root chain surfaces HYBRID.TXT with fs_kind 2 and the extent at the flavor-2 data-area base) + `FatEngine.Fat2ReadWinsOverFat1` (the last-read-wins pin: FAT1 FAT32-shaped, FAT2 canonical FAT16 → flavor 1 stands, the FAT16 decode byte-identical; fails any first-wins or intermediate-assignment implementation) + `FatEngine.ConsistentVolumeUnreflavored` (the control: canonical FAT16 scans byte-identically — fs_kind 1, the same single 1536-byte extent). Full suite 1434/1434 (1392 pass + 42 Win-skips, 0 fail) at the moved tree; FatEngine family 87/87.
**Resolution notes:** Corpus re-verified first-hand at the fix — and the verification UPGRADED the evidence: Ghidra's decompiled text at FUN_180021000 (:26318-26350) carries two rendering defects at this seam, both corrected against the RAW ASSEMBLY (objdump of og-codebase/FATSearchDLL.dll, 0x180021171-0x18002127c): (1) the FAT[0] mask prints as `0xfffffff` (a digit dropped — the instruction bytes `25 ff ff ff 0f` decode to AND EAX, 0x0FFFFFFF), so the true F8-family test is `(FAT[0] & 0x0FFFFFFF) == 0x0FFFFFF8` (the decompiled form is tautologically false — caught by the core's own -Werror tautological-compare); (2) the `FAT[0] == 0xFFFFFFFF` branch's target prints as the flavor-2 assignment — the bytes `74 4d` at 0x18002117f jump to 0x1800211ce, the SECOND read: NO assignment happens in either leg for the pure-0xFFFFFFFF media word. Landed per the fix plan: `refine_flavor_by_media_words` (cr_chain.cpp, declared in cr_fat_engine.h with the full asm-verified shape family) wired into scan_fat after `boot_passes_open_gates`, gated to string-derived flavors only (the og's own reachability — FUN_180021000 bails at the 0xE flavor before the media-word legs, 0x18002107e; the core's D-94/I-002 Fat12 bracket superset is untouched). The flavor flip re-derives the flavor's own BPB field family coherently (validate_boot's tail extracted into derive_flavor_dependent_fields — the og's constructor consumes flavor-dependent spf/root fields, never a mix; the S-064 string-override hybrids are coherent by the same path). M-015's whole-table validation stays the final gate per the fix plan's registered option. THREE scoped boundaries, each with asm evidence: (a) the og's caller-side no-assignment decline + backup-boot retry (:26519-26535) is NOT ported — the core's established posture accepts Fat32 volumes with zeroed media words (the og's quick engine routes them to other engines; the core's tolerant Fat32 media check is the standing model, unchanged); (b) the og's second read is unconditional (no numFATs gate — ported og-exact); (c) the og's mis-flip on plain FAT16 volumes whose first two data clusters are EOC-terminated (packed entries 2,3 read as F1 == 0xFFFFFFFF and fire the F8-family mask) is registered as an og defect class (the I-045 family) and NOT adopted — the og's own constructor then consumes the STRING-first flavor's spf field anyway (:26546-26554 re-runs FUN_18002f970 and selects spf16/spf32 by the string flavor), so the core adopts the re-flavor only when the indicated flavor's own spf field is usable (nonzero — a torn conversion remnant carries the new format's real FAT size); consistent volumes scan byte-identically, the finding's own acceptance criterion. The mask-family transcription from the finding's Open questions is settled: the exact masks are `(FAT[1] & 0x0FFFFFF8) == 0x0FFFFFF8` / `(FAT[1] & 0x07FFFFF8) == 0x07FFFFF8` / the F0-family `(FAT[0] & 0x0FFFFFFF) == 0x0FFFFFF0 && (FAT[1] & 0x0FFFFFF0) == 0x0FFFFFF0`.
