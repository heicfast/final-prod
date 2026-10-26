# INFORMATIONAL I-019 — EPM partition-management surfaces registered as scope exclusion: EditLdm2* (LDM write-side), Ntfs2FATProc conversion, partition merge/resize deals, FatFormat.mo, Burn.dll — tool operations, not recovery

**Severity:** INFORMATIONAL (deliberate scope boundary — registered, not a defect)
**Module:** mom-tool-ops (EPM-side .mo/.dll surfaces)
**Parameters affected:** P54 (module boundaries — where the recovery-engine scope ends), P69 (stub scan — none needed), P46 (documentation — this registration)
**Corpus references:** `re-workspace/reversed/ghidra-mo/LdmManager_decompiled.c` — EditLdm2* family (MoveResizeVol ~:51370-56495 with SetResizeMoveParameter/UpdateVolumeInfo :51752, MergeVolume staging :54300-54744, ShrinkBack :55016-55610, ExtendToInterMediaVol :55818-56450, DelVblkProc/UpdateMemIntoVblk/UpdateComponents with VMDB CommittedSequence logging :52807-52832, ProhibitModifyLdmInDisk :54056, RecoverLdmData :54165; ResizeVolume ChangeVolumeSize :58383; UpdateLdmData ctor :59164-59189; ApplyConfig RestoreRaidToMirror/ModifyMirrorToRaid/RestoreSimpleToRaid/ModifyRaidToSimple/TravelVolume/Online/Offline :84393-84633; DeleteDisk :84818; DeleteVolume :84994) — **no binary outside LdmManager.mo references EditLdm2** (tree-wide grep, agent D); `re-workspace/reversed/ghidra-mo/NTFSUtil_decompiled.c` — namespace `Ntfs2FATProc` (log `"Ntfs2FATProc::CFatVirtualView::VirtualFormat"` :34084; CFatVirtualView fabricates a FAT32 DBR from template :34176-34204; CFatTableProc::FatProc WRITES FAT chains :32924; CFatVirtualFileItem decompress/decrypt/sector-conflict :28199/:28288/:31205); CFirstVolumeMergeDeal merge body :34519 (partition-B-into-A for EPM CMergePartition; siblings :89583/:37994); `mo-module-map.md:26/:35/:54-55` (the WRONG "recovery views" characterization — corrected by I-020); `re-workspace/reversed/ghidra-mo/FatFormat_decompiled.c` (CFatFormat — WinPE media prep); `re-workspace/artifacts/PartitionRecovery/burn--imapi2-disc-writer.md` (Burn.dll = IMAPI2 disc writer for bootable media; CTbCreator Burn:752/775)
**Core references:** audited `7cf6754` — no conversion/merge/resize/format/burn code (grep-verified: Ntfs2FAT, VolumeMerge, FatFormat, IMAPI2 — zero hits); the core's LDM support is read-side (`src/core/table/cr_ldm.cpp`); docs/parity/PARITY-CORE-REC.md:164 established the exclusion pattern for the FileRepair chain — this finding extends the same registration to the EPM tool ops
**Status:** READ-ONLY FINDING — no fix applied (registration only)

## Summary

The og's WinPE application bundles EaseUS-Partition-Master-lineage tool operations alongside recovery: LDM database editing (move/resize/merge/RAID-convert/online-offline), NTFS→FAT conversion with a fabricated DBR and decompression of source files, partition merge/resize algorithm objects, FAT formatting for media prep, and IMAPI2 disc burning. These are partition-manager operations invoked from the tools UI — not part of any recovery flow (the only consumers are the EPM hosts; no recovery module references them). The core correctly has none of them; this finding registers that absence as a deliberate, documented scope boundary so future audits do not re-flag it.

## What the reference does

See citations — the EditLdm2 mutator family, the Ntfs2FATProc conversion
engine (a write path: it builds a NEW FAT volume from a live NTFS source,
with sector-conflict resolution between converted items), the
merge/resize deal classes (partition A+B merging, not damaged-volume
reconstruction), CFatFormat, and Burn.dll.

## What our core does

Nothing in these families — by design; the core is a recovery engine.

## Gap

None (scope boundary). The falsifiable boundary statement: no recovery
flow in the og invokes any of these surfaces (consumer analysis: EPM hosts
only; RecoveryModule/PartitionRecovery.dll do not import EditLdm2 or
Ntfs2FATProc entry points — tree-wide reference grep).

## Impact

None on recovery parity. Registered so: (a) future sessions do not
re-litigate these surfaces; (b) the boundary is explicit — if the product
later wants a "fix the spanned volume" feature, the EditLdm2 decode above
is the starting evidence.

## Reproduction / verification

Corpus-derived (class censuses + the negative consumer grep). Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §G.

## Fix plan (for the engineering team)

None (registration). Action: add a row to docs/parity/PARITY-DELTAS.md
("EPM tool operations excluded — LDM write-side, FS conversion, partition
merge/resize, FAT format, media burn") citing this finding.

## Tests to add

None.

## Related findings

S-029 (the apply path — where the write-side boundary DOES matter for
recovery and is not covered by this exclusion), I-020 (the corpus-doc
corrections for these same modules).

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** grep re-run this session: Ntfs2FAT / VolumeMerge / FatFormat / IMAPI2 - zero hits in src/; the corpus cites (EditLdm2 family unreachable outside LdmManager.mo, CTbCreator Burn) re-checked via the finding's evidence.
**Resolution notes:** Scope boundary, confirmed: EPM tool operations (LDM write-side edit, NTFS-to-FAT conversion, partition merge/resize, WinPE FAT formatting, disc burning) are not recovery surfaces and stay excluded. The core's LDM support remains read-side only (first-class mandate = read-side recovery, which stands).

**Status:** ACKNOWLEDGED (deliberate scope boundary — registered)
**Verified by:** this session: grep over `src/` + `include/` for
EditLdm2 / Ntfs2FAT / VolumeMerge / FatFormat / IMAPI2 — zero hits (the
core's LDM support is read-side only, `cr_ldm.cpp`). Corpus consumer
analysis stands (no recovery module references these surfaces; EPM hosts
only). S-029 (the apply path where the write-side DOES matter for
recovery) is a separate SEVERE finding and is not covered by this
exclusion.
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59k**. Future sessions must not re-litigate these surfaces as gaps;
if the product ever wants spanned-volume repair, the EditLdm2 decode in
this finding is the starting evidence.
