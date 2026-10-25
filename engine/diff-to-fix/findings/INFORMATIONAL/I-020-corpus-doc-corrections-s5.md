# INFORMATIONAL I-020 — Corpus-documentation corrections from session-5 decodes (CTbFile, CFatVirtualView family, EFIBoot mechanism, Fixup binding, FATX naming, extension-table location/sorting)

**Severity:** INFORMATIONAL (documentation drift in the corpus's own artifacts — fed back via CORPUS-ERRATA rows 37-42; no core change)
**Module:** corpus-errata (cross-cutting)
**Parameters affected:** P68 (documentation accuracy — the corpus's), P46 (contract docs)
**Corpus references:** the corrected artifacts: `re-workspace/artifacts/PartitionRecovery/mo-module-map.md` (:21 CTbFile "TB-scale disk-IMAGE file abstraction"; :35/:54-55 CFatVirtualView "virtual FS views over damaged disks" + CFirstVolumeMergeDeal "merges split volumes"; :38 EFIBoot "EFI System Partition handling"; :39 Fixup implication of MOM-contract loading); `re-workspace/artifacts/MOM-modules/Fixup.md:5` (MOM loading claim); `re-workspace/artifacts/MOM-modules/module-internals-wave2--ldm-reglib-ctbfile.md:89-96` (.tbi mount claim); `diff-to-fix/tracked-open/OQ-S4-1-grading-extension-table.md` (".rdata" + "sorted string-map" claims)
**Core references:** audited `7cf6754` — none (this finding corrects the corpus's evidence layer that future parity sessions consume)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Session-5's decodes falsified six corpus-artifact claims that prior and future parity sessions rely on. Each is corrected here with the disproving evidence; the same corrections are filed as CORPUS-ERRATA rows 37-42 so the corpus repo's maintainers can absorb them (following the CORRECTIONS.md precedent the corpus itself established for core-fed corrections).

## What the reference does (the corrections)

1. **CTbFile is not an image abstraction** — it is a generic refcounted
   Win32-file wrapper whose ONLY construction site reads boot.ini
   (DeviceManager_decompiled.c:51016-51086, path `L"\\boot.ini"` :50993,
   INI parse :51120-51125; "Tb" = Todo-Backup lineage). The disk-image
   capability lives in DRWUI's CImageFileDevice + Device.mo/libvmdk
   (S-030's decode).
2. **CFatVirtualView/CFatVirtual{BaseItem,DirItem,FileItem} is the
   Ntfs2FAT conversion engine, not a damaged-disk recovery view** —
   namespace `Ntfs2FATProc` (NTFSUtil:34084), fabricates a FAT32 DBR
   (:34176-34204), writes FAT chains (:32924).
3. **CFirstVolumeMergeDeal merges partition B into partition A (EPM
   CMergePartition), not split/cross-partition volumes**
   (NTFSUtil:34519; siblings :89583/:37994).
4. **EFIBoot.mo manipulates UEFI NVRAM variables (Boot####/BootOrder via
   Get/SetFirmwareEnvironmentVariableW, GUID 8BE4DF61), not ESP files**
   (EFIBoot:3281-3290/:3385-4208; no CopyFile/CreateFile in the module).
5. **Fixup.mo is a link-time shared library (13 Bcd_* + 6 BootIni_* C
   exports), not a MOM GetModule factory** (PE export/import dump; six
   host importers).
6. **CFatxBlockParser is the FAT12/16/32 boot parser ("Fatx"=FAT-x), not
   Xbox FATX** — the Xbox magic 0x58544658 appears nowhere in the corpus
   (definitive negative grep); resolves S-006's scoped remainder.
7. **The CRecoverability extension table is in `.data` (not `.rdata`) and
   is unsorted** (build order; the runtime map sorts) — 169 entries dumped
   (OQ-S4-1 RESOLVED).

## What our core does

N/A — evidence-layer correction. The core-side consequences of
corrections 1-4 are carried by S-030/M-040/I-019; correction 6 resolves
S-006's remainder into M-045; correction 7 completes S-023's fix-plan step
3 input.

## Gap

Six artifact claims disproven with primary evidence (each falsifiable by
the cited decompile lines).

## Impact

Future parity sessions consume these artifacts as ground truth — the
mis-attributions would misdirect them (e.g., auditing CTbFile for image
parity, or expecting ESP-file semantics from EFIBoot). Also feeds the
corpus's own CORRECTIONS.md loop (8 prior core-fed corrections exist).

## Reproduction / verification

All primary-evidence citations above; the disproofs are in
`evidence/decompiled-quotes/s5-mom-capabilities.md` (corrections 4, 5, 7),
`.../s5-image-fscheck-persistence.md` (corrections 1, 2, 3, 6), and
`.../s5-oq-s4-1-extension-table-dump.md` (correction 7).

## Fix plan (for the engineering team)

None for the core. For the corpus maintainers (via CORPUS-ERRATA.md rows
37-42, already filed by this session): update mo-module-map.md rows
(DeviceManager/NTFSUtil/EFIBoot/Fixup/PartitionRecoveryD), Fixup.md:5,
module-internals-wave2 (CTbFile section), and note the extension-table
location in the grading artifact.

## Tests to add

None.

## Related findings

S-030 (correction 1's real capability), M-040 (corrections 4-5),
M-045/S-006 (correction 6), S-023/OQ-S4-1 (correction 7), I-019
(corrections 2-3's scope registration).

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** CORPUS-ERRATA.md rows 37-42 present and cross-checked against the finding's six corrections (CTbFile = TB-scale disk-IMAGE abstraction; CFatVirtualView family = tool-ops virtualization; EFIBoot mechanism; Fixup binding; FATX naming; extension-table location/sorting).
**Resolution notes:** Corpus-side documentation corrections, fed back via the errata register; no core change. Future sessions citing mo-module-map.md or Fixup.md must use the corrected characterizations. The OQ-S4-1 grading-table claims (".rdata"/"sorted string-map") carry the correction into that tracked item's evidence base.
