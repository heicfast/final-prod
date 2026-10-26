# MINOR N-040 — NTFS pass-1 attribute filter read $FILE_NAME SYSTEM-only; the og's gate is (og-15 name table) AND ($SI SYSTEM+HIDDEN)

**Severity:** MINOR (result-set membership on the system-flagged user-file edge; discovered during S-012 verification)
**Module:** ntfs
**Parameters affected:** B1 (detection/result-set membership)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` — walk gate `:18952` (`engine+0x259 == 0 || FUN_180034b20(rec) == 0` → record skipped when the gate returns 1; engine+0x259 is never written in this DLL → default 0 → gate active); the processor's second refusal `:18566` (`if (!bVar19) { Release; return 0x80004005; }` where bVar19 = NOT-gate-hit); `FUN_180034b20` `:41992-42003` (attribute iterator + `FUN_180040ea0`); `FUN_180040ea0` `:51960-51980` (requires `FUN_180040f20() != -1` AND the type-0x10 `$STANDARD_INFORMATION` attributes dword via `FUN_180044f60` carrying BOTH 0x4 SYSTEM and 0x2 HIDDEN; no `$SI` → 0 → keep); `FUN_180040f20` `:51993-52015` (finds the $FILE_NAME (type 0x30), compares its name against the 15-entry table `PTR_u__MFT_180086068` — `$MFT`..`$RmMetadata` — returning the per-entry value from `DAT_180086060`, or -1)
**Core references (pre-fix):** `src/core/ntfs/cr_mftwalk.cpp` — `flags.skip_system_files && (best->dos_flags & 0x04)` (the $FILE_NAME flags field, SYSTEM bit only), with the header gloss "corpus: dos_flags & 4 -> skip" — a drift from the corpus gate: $FILE_NAME flags are never consulted by the og's pass-1 filter
**Status:** FIXED (landed with the S-012 verification-and-correction commit)

## Summary

The og's pass-1 metafile/system filter is a compound gate: a record is
skipped when its $FILE_NAME name matches the classic 15-entry metafile
table AND its $STANDARD_INFORMATION attributes dword carries both SYSTEM
(0x4) and HIDDEN (0x2) — the signature real NTFS metafiles carry
($MFT, $LogFile, ...: SYSTEM|HIDDEN|ARCHIVE). Records with no `$SI`
pass (FUN_180040ea0 returns 0 when the attribute is absent); files whose
name is not in the og-15 table pass regardless of attributes.

The core's filter instead read the `$FILE_NAME` flags field and required
only the SYSTEM bit. Membership diverged in both directions: a user file
with `$FN`-SYSTEM flags (pagefile.sys, hiberfil.sys, bootmgr-class — all
carry FILE_ATTRIBUTE_SYSTEM|HIDDEN at the $FN level) was dropped by the
core and kept by the og; the og's `$SI`-condition was never applied at
all. On real volumes the first direction silently removed the
pagefile/hiberfil/bootmgr family from every scan.

## What the fix does

The `$FILE_NAME`-flags gate is REMOVED (with the `skip_system_files`
flag and the `system_skipped` counter — dead surface). The og-15 name
table is subsumed by the core's known-name metafile list (the I-001
documented deliberate superset, which already filters every og-15 name),
and real metafiles always carry `$SI` SYSTEM|HIDDEN — so name-list-only
filtering reproduces the og's gate on every real volume. The residual
edge (a metafile-table name with clean/torn `$SI` — kept by the og's
AND-semantics, skipped by the known list) stays inside I-001's
registered divergence.

## Resolution

**Status:** FIXED
**Fixed in commit:** (same commit as the S-012 verification-and-correction addendum)
**Verified by:** corpus first-hand decode (FUN_180040f20's 15-entry table + FUN_180040ea0's $SI dword check, both read line-by-line this session); `NtfsVolume.MainWalkKeepsSystemFlaggedUserFile` — a live `pagefile.sys` with `$FN` SYSTEM|HIDDEN flags and `$SI` ARCHIVE-only surfaces post-fix and was skipped pre-fix (git-stash fail-before proof); suite 399/399 Linux ASan (no other fixture carries a $FN-SYSTEM non-metafile record, so no other membership changes).
**Resolution notes:** gate removed; flag + counter removed with it;
N-040 registered in PARITY-DELTAS D-51's correction note and the
S-012 Resolution addendum; CAPABILITY-MATRIX row updated.
