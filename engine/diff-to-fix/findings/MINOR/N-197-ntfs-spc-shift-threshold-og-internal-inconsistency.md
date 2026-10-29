# MINOR N-197 — spc shift-form decode threshold is og-internally inconsistent (NTFSSearchDLL decodes at (spc & 0xF0) == 0xF0; NTFSLib/analyser at >= 0x80/0x81; PartitionRecoveryD raw): the core picked one side unregistered

**Severity:** MINOR
**Module:** ntfs (BPB spc decode) / cross-layer consistency
**Parameters affected:** C20 (on-disk struct decode), Q68 (registration completeness — D-52 records one og form as *the* form)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_18003f5d0 (the definition, re-read first-hand this session):
```c
if ((*(byte *)(param_1 + 0xd) & 0xf0) == 0xf0) {
    uVar1 = 1 << (~*(byte *)(param_1 + 0xd) + 1 & 0x1f);
} else {
    uVar1 = (uint)*(byte *)(param_1 + 0xd);
}
```
vs `ghidra-mo/NTFSLib_decompiled.c:19307` (`bVar2 < 0x81` shift branch) and `NTFSFileSystemAnalyser_decompiled.c` FUN_1800048c0 (`0x80 < bVar1`); `ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_180011560 (:10048-10049) uses the RAW byte with no decode
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp:47-51` (shift at >= 0x80; D-52 registered this as *the* og form citing LoadNtfsVolume only)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's four layers decode the sectors-per-cluster byte three different ways: the scan controller (NTFSSearchDLL) shifts only when the high nibble is 0xF (bytes 0x80..0xEF are taken as DIRECT sector counts — 0x85 → 133 sectors/cluster); NTFSLib and the analyser shift at >= 0x80/0x81; PartitionRecoveryD's block parser uses the raw byte. The core implements the NTFSLib form and D-52 records it as the single og form. On a 0x80..0xEF spc byte with positive cpr, the og's own scan controller computes different MFT addresses than the core.

## What the reference does

The three forms quoted above (the FUN_18003f5d0 definition read directly this session; the NTFSLib/analyser forms read in the M-138/M-140 trails; the PartitionRecoveryD raw use read in the N-187 trail).

## What our core does

`cr_bootstrap.cpp:47-51` — the >= 0x80 shift decode, with D-52's registration citing LoadNtfsVolume as the og's form.

## Gap

Falsifiable: a (synthetic) volume with spc byte 0x85 and positive cpr: NTFSSearchDLL computes spc=133 (raw); the core computes spc=2^7=128 (shift) — different MFT LBA, different cluster size. Which og layer drives the shipped scan determines the "true" og behavior (the scan controller, per the engine docs).

## Impact

Deep edge (shift-form spc implies ≥256-sector clusters — rare even in synthetic images); wrong MFT addressing only on that geometry. The registration gap (D-52 presenting one form as the form) is the real deliverable here.

## Reproduction / verification

Decompiled evidence, all four sites read first-hand this session (FUN_18003f5d0's body quoted verbatim above). Core side quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Document the og-internal inconsistency in D-52 (the 0xF0 variant at FUN_18003f5d0, the raw byte at PartitionRecoveryD, the shift forms at NTFSLib/analyser) — the register currently overclaims a single form.
2. Keep the current decode (the NTFSLib/analyser form is the majority and the lib-form is what the corpus artifacts document) and add a pin test asserting the chosen convention, with the D-52 note recording the controller's divergence.
3. Files: `docs/PARITY-DELTAS.md` (D-52), the pin test.

## Tests to add

- `Boot.SpcShiftDecodeConventionPinned` (0x80 → 128; 0xF0 → 16; 0x85 → 128 under the chosen convention — the assertion documents the choice).

## Related findings

- M-138/M-146 (the same getter/decode family — one coherent pass over D-52's record).

## Open questions

None — the decode is complete on all four sites; the finding is a registration-completeness item.
