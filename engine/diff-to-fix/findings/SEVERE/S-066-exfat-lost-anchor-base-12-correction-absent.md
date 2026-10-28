# SEVERE S-066 — exFAT lost-anchor recognition: the og re-bases a volume whose scan anchor is the surviving BACKUP boot (base − 12 sectors, gated by FAT media-word probes); the core scans 12 sectors off — garbage FAT/heap or full-volume loss

**Severity:** SEVERE
**Module:** exfat (engine recognition ladder) / lost-partition (the candidate-materialization path that feeds it)
**Parameters affected:** B1 (detection correctness), B2 (recovery correctness), A25 (engine algorithm steps), A30 (lost-partition candidate acceptance)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` FUN_180001630 (:341-420) — every line quoted below re-read first-hand this session at corpus `be0cfe9`
**Core references:** audited `34d6cf10` — `src/core/exfat/cr_exfat_dentry.cpp:104-197` (`validate_boot_region`); `src/core/exfat/cr_exfat_dentry.cpp:174-177` (backup flag informational); `src/core/lost/cr_exfat_probe.cpp` + the orchestrator's candidate materialization (`cr_scan.cpp` probe family → `PartitionSlice` at the anchor LBA) — no base-correction code anywhere (grep-verified: no `0xc`-sector re-base, no FAT media-word probe at open)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When the og's exFAT recognizer accepts a boot sector but the sector 12 positions later fails the full structural gates, it probes the FAT at the candidate base and at base−12 for the canonical media words (FAT[0]=0xFFFFFFF8, FAT[1]=0xFFFFFFFF); if the shifted probe matches, it re-bases the whole volume to base−12 and scans the true volume. The core has no equivalent: it materializes the candidate at the anchor LBA and reads the FAT and cluster heap 12 sectors off. On the damaged-volume population this product exists for (primary boot destroyed, backup boot surviving as the scan anchor), the og recovers the volume and the core recovers garbage chains or nothing.

## What the reference does

FUN_180001630 (`exFATSearchDLL_decompiled.c:341-420`), the recognition function that assigns flavor 7 (exFAT) to a scan anchor — re-read first-hand this session:

1. The main-boot gates run on the anchor sector (`FUN_180021e80(param_3, …)` :354, returns nonzero on pass).
2. The og reads the sector at `base + 0xc` (the backup boot position) and re-runs the FULL structural gate function on it (:357-359: `FUN_180021e80(0x180061280, iVar1)` over the read buffer).
3. **If the backup boot FAILS the gates** (`(int)uVar3 == 0` :366), the og reads 1 sector at `fat_offset(+0x50) + base` (:368-370) and tests the canonical FAT media-word pair:
   ```c
   if ((DAT_180061280 != -8) || (DAT_180061284 != -1)) {   /* :366 — FAT[0]!=0xFFFFFFF8 || FAT[1]!=0xFFFFFFFF */
   ```
4. On mismatch — the FAT is NOT where the boot says, relative to this base — the og requires `base >= 0xc` (:371-373), then reads the FAT at `(base - 0xc) + fat_offset` (:372-374) and re-tests the pair (:377-381: `DAT_180061280 != -8` → return 0; `DAT_180061284 != -1` → return 0).
5. On match, it executes the re-base:
   ```c
   *(longlong *)(param_5 + 2) = *(longlong *)(param_5 + 2) + -0xc;   /* :377-378 — base -= 12 */
   ```
   and continues with the volume length from the boot (`*(undefined8 *)(param_3 + 0x48)` :382).

The semantics: a scan anchor whose +12 sector is not a valid backup boot is presumed to BE the backup boot of a volume whose primary is destroyed (exFAT places the backup boot exactly 12 sectors after the primary); the FAT media words confirm which base is the true one, and the volume base is corrected before any engine work runs.

## What our core does

`validate_boot_region` (`cr_exfat_dentry.cpp:104-197`) reads the 24-sector boot region at the candidate base, gates on the main boot's structural fields (MustBeZero sweep, fat_count, fat_length, shifts, heap fit — the EXFAT-4 set), and computes `p.boot_backup_ok`:

```cpp
/* cr_exfat_dentry.cpp:174-177 */
const BootSectorMain* bak =
    reinterpret_cast<const BootSectorMain*>(region.data() + BACKUP_BOOT_SECTOR * bps);
p.boot_backup_ok = oem_is_exfat(*bak) && bak->signature[0] == 0x55 && bak->signature[1] == 0xAA;
```

`boot_backup_ok` is an informational flag — a failed backup boot does not fail the open, and nothing downstream acts on it. There is no FAT media-word probe and no base correction anywhere in the open path (grep over `src/core/exfat/` + `src/core/lost/`: no read at `fat_offset` before the engine's own FAT load, no `base - 12` arithmetic). The orchestrator materializes the lost-volume candidate at the anchor LBA and the engine reads `fat_offset`/`cluster_heap_offset` relative to that anchor as-is.

## Gap

A lost exFAT volume whose primary boot sector is destroyed and whose surviving backup boot is the scan anchor (anchor LBA = true start + 12): the og detects the failed backup at anchor+12, confirms the true FAT at anchor−12+fat_offset by media words, re-bases to anchor−12, and scans the volume. The core passes `validate_boot_region` at the anchor (the backup boot IS a complete valid main boot), then reads the FAT at anchor+fat_offset = true FAT start + 12 sectors and the heap likewise 12 sectors off. Falsifiable: construct the fixture (destroy sector 0 of an exFAT volume image, scan from LBA 12); the og's tree is the full volume; the core's chains are garbage (the FAT read is 12 sectors off) or the open fails outright.

## Impact

Full-volume recovery loss or garbage extents on exactly the damaged-volume population a recovery product targets — user-observable "the og finds it, we don't". This is the exFAT twin of the NTFS backup-DBR problem the core already solved for boot sectors (D-99's backup fallback); the og solves the mirror-image case (anchor IS the backup) that the core does not.

## Reproduction / verification

Corpus-derived (decompiled evidence, lead-verified first-hand this session): the full ladder quoted above from `exFATSearchDLL_decompiled.c:341-420`; the gate function `FUN_180021e80` (:28180-28210) re-read to confirm the backup re-validation uses the identical structural gate set; media-word constants 0xFFFFFFF8/0xFFFFFFFF confirmed against the exFAT spec's FAT[0]/FAT[1] requirements. Core side: full read of `validate_boot_region` + grep census of the exfat/lost/orchestrator paths for any base-correction equivalent (none). Execution reproduction (fixture image, both implementations) was not run this session — the corpus decode is unambiguous; the fix plan's first test is the executable pin.

## Fix plan (for the engineering team)

1. In `validate_boot_region` (or the orchestrator's exFAT dispatch, wherever the candidate base is finalized): after the main gates pass, read the backup boot at base+12 and run the same structural gates.
2. If the backup fails: read the first sector at `base + fat_offset` and at `base - 12 + fat_offset`; test both for `{FAT[0], FAT[1]} == {0xFFFFFFF8, 0xFFFFFFFF}`.
3. If only the shifted probe matches and `base >= 12`: set the volume base to `base - 12` (flag it, e.g. `boot_base_shifted`, for logging/stat surfacing), and proceed. If only the in-place probe matches: keep the base. If neither: keep current behavior (the gates already passed on the main boot).
4. Mirror the correction into the lost-partition candidate materialization so the surfaced candidate's LBA is the corrected base (the og re-bases before the candidate is surfaced — check FUN_180001630's caller chain for where `param_5 + 2` is consumed; probe task below).
5. Files: `src/core/exfat/cr_exfat_dentry.cpp` (+ a base out-param or a post-open correction step in the orchestrator), `include/cr/exfat/cr_exfat_wire.h` (the flag), `src/core/orchestrator/cr_scan.cpp` (dispatch wiring if done there).
6. Acceptance criteria: fixture with destroyed primary boot scanned from LBA 12 → full tree identical to the undamaged-volume tree; fixture with intact primary boot unchanged; stat surfaced (`exfat_base_shifted` analog of the existing skip-honesty counters).

## Tests to add

- `Exfat.LostAnchorBackupBootRebasedToTrueStart` (destroyed primary, anchor at +12 → full tree, base shifted) — RED today.
- `Exfat.IntactPrimaryNotShifted` (control: no re-base when the backup at +12 validates).
- `Exfat.MediaWordsAbsentNoShift` (both probes mismatch → base kept, current behavior preserved).
- `Exfat.ShiftedBaseBelowZeroRefused` (`base < 12` with failed backup → no negative-base read).
- `LostPart.ExfatCandidateLbaIsCorrectedBase` (the surfaced candidate carries the corrected LBA).

## Related findings

- S-063 (the NTFS MFTMirr bootstrap fallback — the same "og salvages the damaged-volume population, core refuses" class, NTFS side).
- D-99 (the registered backup-DBR fallback — the complementary primary-destroyed case, already landed).
- M-132 (GPT backup probe position — the same backup-placement reasoning on the table layer).

## Open questions

The PartitionRecoveryD `CExFatBlockParser` twin of this ladder (the lost-scan side) was not decoded this session — confirm it carries the same −12 correction before choosing the fix's layer (engine open vs. candidate materialization). The probe: locate the exFAT block-parser function family in `ghidra-mo/PartitionRecoveryD_decompiled.c` (search for the FUN_180021e80-equivalent gate calls from the partition-scan entry) and check for the `+ -0xc` store on the base field.
