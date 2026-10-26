# MINOR N-079 — NTFS recycle-bin handling matched $I/$R NAME PREFIXES everywhere (no bin-scope): user files named "$I…" vanish, out-of-bin "$R…" can mis-pair

**Severity:** MINOR (narrow name class: NTFS-legal user filenames starting `$I`/`$R` outside the bin)
**Module:** ntfs
**Parameters affected:** B1 (result-set membership — `$I…`-named user files skipped), B4 (classification — out-of-bin `$R…` mis-remapped + wrongly forced Deleted)
**Corpus references:** `NTFSSearchDLL_decompiled.c` — the four-name bin-root recognizer `FUN_18002e5b0` (:37355-37411: `_stricmp` "Recycler" / "Recycled" / "$Recycle.Bin" / "$RECYCLE.BI0", same set as the FAT :1668-1677 and exFAT :40583-40592 recognizers); the ancestor-chain consumer `:43538-43550` (`while (puVar12 != 0) { if (FUN_18002e5b0(puVar12)) …; puVar12 = puVar12[1]; }` — the bin test WALKS THE PARENT CHAIN); the S- SID-subfolder context `:53985-53996`
**Core references:** `src/core/ntfs/cr_mftwalk.cpp` — the pre-fix `$I/$R` block matched `best->name[0] == '$' && (name[1] == 'I' || 'R')` with NO location test: the `$I` branch `continue`d (skipped the record) UNCONDITIONALLY — even when the resident $DATA did not parse as a $I record and even outside any bin; the `$R` branch remapped at emission whenever a same-token $I existed ANYWHERE on the volume (filed during M-052's verification at commit `9aa8448`-lineage)
**Status:** FIXED (NEW-FINDING filed + fixed in the same session — DFX35, discovered while verifying M-052's shared-helper extraction)

## Summary

The og scopes its recycle-bin handling to the recognized bin roots: a node is bin content only when the four-name `_stricmp` recognizer matches somewhere in its ancestor chain (NTFSSearchDLL :43538-43550). The core's NTFS landing keyed purely on the NAME PREFIX: every record named `$I<something>` was skipped as "bin metadata" (with no parse gate — a user file `$Image.txt` whose $DATA is plainly not a $I record still vanished), and every `$R<something>` could remap against a same-token $I located anywhere on the volume, forcing the Deleted state on a live user file.

## What the reference does

Citations above: the recognizer exists in all three SearchDLLs with the same four names; the NTFS consumer walks the parent chain before treating a node as bin content. Bin handling is location-scoped, not name-scoped.

## What our core did (pre-fix)

`cr_mftwalk.cpp`'s `$Recycle.Bin mapping` block ran on ANY `$I`/`$R`-prefixed `$FILE_NAME` regardless of the record's location or content shape: `$I…` → parse attempted but skipped EITHER WAY; `$R…` → emission-time remap on token match with no ancestry test.

## Gap

Falsifiable: (a) an NTFS volume with a root-level user file `$Image.txt` (16 bytes of ordinary content): the og emits it; the pre-fix core skipped it (absent from results); (b) a root-level user file `$R8Z4K2M.doc` while a real bin pair `$RECYCLE.BIN\S-…\$I8Z4K2M`+`$R8Z4K2M.bin` exists: the og emits the user file raw and active; the pre-fix core renamed it to the $I's original name, overrode its size/time, and forced the Deleted state.

## Impact

User files with NTFS-legal `$I…`/`$R…` names (copy-scripts, `$Recycle.Bin` copies kept for reference, tools writing `$R`-prefixed temp names) silently vanish or get misclassified. The in-bin pairing itself was correct; only the scope was wrong.

## Reproduction / verification

Corpus: the recognizer + ancestor-walk quotes read first-hand this session. Core: the pre-fix block read at `9aa8448`-lineage. Execution: both falsifiable claims proven by the fail-before run (test RED on the pre-fix engine, GREEN after).

## Fix (landed with M-052's shared-model extraction)

1. The `$I` skip is now PARSE-GATED: a record whose resident $DATA does not parse as a $I record (version/pathlen/shape) is ordinary content and is emitted. A `$I`-named file with a VALID record image outside the bin is still skipped — the known narrow residual (removing it needs a node-removal API the tree does not have; the residual requires a user file whose content is a well-formed $I record outside any bin — recorded here, not silently left).
2. The `$R` remap is now entirely POST-WALK and BIN-ANCESTRY-GATED: the node's parent chain must contain one of the four recognized roots (`recycle::is_bin_root`, the shared four-name set) before any remap or Deleted forcing.

## Tests

`NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw` — a self-contained volume (own geometry; the shared dump image's MFT only covers records 0..13) holding an in-bin pair + out-of-bin `$Image.txt` + out-of-bin token-matching `$R8Z4K2M.doc`: asserts the in-bin pair still remaps while both out-of-bin files emit raw and active. Fail-before EXECUTED (RED on the pre-fix engine).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-052/N-079 landing — DFX35; SHA in worklog.md at push)
**Verified by:** `NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw` (fail-before EXECUTED on the stashed pre-fix engine: RED); `NtfsVolume.RecycleBinMapping` (the in-bin regression pin — green on both trees); suite 794/794 linux-release.
**Resolution notes:** The og's bin handling is location-scoped (ancestor chain over the four-name recognizer, NTFSSearchDLL :43538-43550) — the landing was name-scoped. Fixed in the same change as M-052's shared `cr_recycle.h` extraction: parse-gated `$I` skip + fully post-walk, ancestry-gated `$R` remap. Residual documented above (valid-$I-format content outside the bin is still skipped — needs a tree node-removal API to fix; vanishingly rare).
