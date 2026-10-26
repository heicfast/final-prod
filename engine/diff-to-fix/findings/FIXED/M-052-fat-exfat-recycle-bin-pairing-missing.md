# MAJOR M-052 — FAT/exFAT recycle-bin $I/$R pairing missing (engine capability NTFS has)

**Severity:** MAJOR
**Module:** fat, exfat (vs ntfs)
**Parameters affected:** B4 (deleted-file classification), B5 (orphan reconstruction), D11 (node fields), A29 (merge/dedup)
**Corpus references:** `re-workspace/artifacts/SearchDLL/fat-exfat-engine-internals.md:21` (IRecycleBinNode2UI in the FAT engine class inventory), `:37-40` (Recycle Bin step 4: `$RECYCLE.BIN`/`Recycled`/`Recycler` recognized byte-exact; `IRecycleBinNode2UI` surfaces bin items through the dedicated UI flow — `CRecycleBinRecoveryController` on the DRWUI side — "including the `$I…/$R…` info/restore pairing by index across bin subfolders"), `:71` (exFAT: "$RECYCLE.BIN handling mirrored from FAT engine (multiple `_stricmp` sites)"), `:101` ("$RECYCLE.BIN comparisons verified in code")
**Core references:** `src/core/fat/` + `src/core/exfat/` — grep `-i "recycle"` = **0 hits** (verified this session, audited commit `022c81d`-lineage / landing `bf7df39`); the NTFS engine has the full pairing: `src/core/ntfs/cr_mftwalk.cpp:162-250` (`$Recycle.Bin mapping (og parity: recycle-bin files surface under ...)`, `parse_recycle_info`, the `$I/$R` pairing token, "0/26 recycle deletions surfaced raw-named" test note)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's FAT and exFAT engines both recognize the Windows recycle-bin roots (`$RECYCLE.BIN`, the Win9x `Recycled`, the NT4-2000 `Recycler`) and surface recycle-bin deletions through the `$I…/$R…` info/restore pairing — the same capability the core implements for NTFS only. The core's FAT/exFAT engines contain zero recycle-bin handling, so recycle-bin deletions on FAT/exFAT volumes surface as raw `$I`/`$R` hex names with no original-name/date restoration.

## What the reference does

The FAT engine's class inventory includes `IRecycleBinNode2UI` ("RECYCLE BIN bridge — see below", fat-exfat-engine-internals.md:21), and scan-flow step 4 is explicit (ibid :37-40): the strings `$RECYCLE.BIN`, `Recycled`, `Recycler` are matched byte-exact; bin items are surfaced through a dedicated UI-flow interface; the `$I…/$R…` **info/restore pairing by index across bin subfolders** reconstructs the original name and deletion metadata (the `$I` file is a ~544-byte Vista+ metadata record holding original path + size + FILETIME; the `$R` file holds the content). The exFAT engine mirrors this handling (ibid :71, "multiple `_stricmp` sites"), and the artifact's verification ledger independently confirms "`$RECYCLE.BIN` comparisons verified in code" (:101).

## What our core does

The core implements the full recycle-bin model exactly once — in the NTFS engine (`cr_mftwalk.cpp:162-250`): `RecycleInfo` parse of the `$I` metadata, the `$I/$R` pairing token (the random suffix after the `$I`/`$R` marker), original-name restoration, and the dedicated recycle folderization. The FAT and exFAT engines have no equivalent: `grep -rn -i "recycle" src/core/fat/ src/core/exfat/ include/cr/fat/ include/cr/exfat/` returns nothing — their directory walks emit `$RECYCLE.BIN` subtree files raw, exactly as `S-006`-era lost-probe naming would.

## Gap

Falsifiable claim: for a FAT32 volume holding `$RECYCLE.BIN\S-1-5-21-…\$R7ABC12.jpg` + `$I7ABC12.jpg`, the og emits a recycle-surfaced result carrying the original name/path/deletion date from the `$I` record; the core emits two raw-named files (`$I7ABC12.jpg`, `$R7ABC12.jpg`) with no pairing, no original name, no deletion date.

## Impact

User-observable on every FAT/exFAT (removable media, old-system disks, camera cards formatted FAT32 by a PC) recycle-bin recovery: the single most consumer-recognizable recovery class ("files I deleted from the Recycle Bin") returns hex junk names on these filesystems while returning proper names on NTFS — an internal inconsistency in our own surface, not just an og gap. Content itself is not lost (`$R` data is recovered); classification and naming are.

## Reproduction / verification

Corpus-derived (decompile-verified artifact, first-cited this session): quotes above with exact lines. Core absence grep-verified first-hand at the audited commit; NTFS counterpart read in-source (`cr_mftwalk.cpp:162-250`). Not executed (no FAT32 recycle fixture in the bench corpus yet — see tests below).

## Fix plan (for the engineering team)

1. Extract the NTFS recycle model (`parse_recycle_info` + pairing token + folderization) into a shared helper (`src/core/common/` or a small `recycle` header) parameterized by "give me the file's name and content stream".
2. Wire it into the FAT dentry walk (`src/core/fat/cr_fat_walk.cpp` — the directory-recursion site) and the exFAT entry-set walk (`src/core/exfat/cr_exfat_quick.cpp`): when a directory name matches `$RECYCLE.BIN`/`Recycled`/`Recycler` (case-insensitive), mark the subtree as recycle-bin; parse `$I` streams for the metadata record (format: v2 header + version 2 = Vista+; original path at +0x18 as NUL-terminated UTF-16, size +8, FILETIMEs at +0x10/+0x20 — the NTFS-side parser already decodes this shape).
3. Pair `$I<token>` with `$R<token>` by exact token string; emit the `$R` content under the `$I` original name + deletion time; folderize under the same recycle root the NTFS engine uses.
4. Tests: `FAT.RecyclePairingRestoresOriginalName`, `FAT.RecycleLegacyRoots` (Recycled/Recycler), `ExFAT.RecyclePairingMirrored`, plus a no-regression pin `FAT.NonRecycleDollarFilesStayRaw` (other `$`-prefixed dirs must not be special-cased).
5. Contract/doc updates: `docs/CAPABILITY-MATRIX.md` recycle-bin row currently says NTFS-only — extend it; register the shared-helper refactor in PARITY-DELTAS if the NTFS behavior moves files.
6. Acceptance: a FAT32 fixture with a filled `$RECYCLE.BIN` yields the original filename + deletion date in `--json` output (mirroring the NTFS e2e test's "0/26 recycle deletions surfaced raw-named" assertion, which must hold for FAT too).

## Tests to add

- `FAT.RecyclePairingRestoresOriginalName` (fails today: emits `$R…` raw)
- `FAT.RecycleLegacyRoots` (Recycled/Recycler recognized — the og matches all three byte-exact)
- `ExFAT.RecyclePairingMirrored`
- `FAT.NonRecycleDollarFilesStayRaw` (pin: no over-matching)

## Related findings

S-019-fixed lineage (lost BitLocker probes — same "probe family" layering site); M-017 (lost-partition scoring — same engine-completeness family); the NTFS pairing implementation itself (cr_mftwalk.cpp:162-250) is the in-repo reference implementation to generalize.

## Open questions

None — the corpus states the behavior directly and the NTFS side proves the model in-repo.


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-052 commit — DFX24)
**Verified by:** `FatEngine.RecyclePairingRestoresOriginalName`, `FatEngine.RecycleLegacyRoots`, `FatEngine.NonRecycleDollarFilesStayRaw`, `ExfatEngine.RecyclePairingMirrored` — fail-before EXECUTED as an engine mutation (the bin-root marker neutralized: 3/4 RED — the pairing and both engines' mirrors fail, the raw-named $R surfaces; the over-matching pin stays green by design), reverted-green; suite 848/848 linux-release, ASan lane 848/848.
**Resolution notes:** The NTFS engine's in-repo reference implementation (cr_mftwalk.cpp: $I parse, extension-stripped pairing token, $R remap + post-walk fixup) is extracted into the shared `cr/recycle` helper (include/cr/cr_recycle.h + src/core/recovery/cr_recycle.cpp) parameterized by name + content-stream bytes, exactly as the fix plan specified; the NTFS side now consumes it through thin attribute-view glue (behavior identical, the refactor is the plan's step 1). Wired into the FAT dentry walk (cr_fat_quick.cpp) and the exFAT entry-set walk (cr_exfat_quick.cpp): the three corpus roots ($RECYCLE.BIN / Recycled / Recycler, case-blind — the byte-exact _stricmp family, fat-exfat-engine-internals.md :37-40) mark their whole subtree; $I<token> streams (v1: 28+2*pathlen, v2: 544 fixed — the shared parse honors both forms incl. the Win10+ variable form's repurposed @0x18) are parsed for the original name/size/deletion-time and never surface; $R<token> files surface under their ORIGINAL name and $I-recorded size/time, marked Deleted (in-use inside the bin, deleted from the user's location — the NTFS model); a $R whose $I appears later is fixed post-walk (the NTFS GAP-P1-3 pattern). New stats `recycle_info` / `recycle_remapped` on both engines. The corpus's dedicated UI flow (IRecycleBinNode2UI / CRecycleBinRecoveryController) is host-layer surface — the core contract (original name + metadata restoration) is what the engines provide. No safety hardening touched; the deleted-$I limitation (0xE5-mangled FAT names lose the token) matches the NTFS side's deleted-record posture and is recorded here as the known edge.
