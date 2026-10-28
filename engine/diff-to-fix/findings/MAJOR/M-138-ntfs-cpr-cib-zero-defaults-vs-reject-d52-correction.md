# MAJOR M-138 — NTFS cpr==0/cib==0: the og's scan path DEFAULTS (record 0x400 / index block 0x1000) at four independent sites; the core rejects the volume — and PARITY-DELTA D-52's "og rejects" premise is a decompile misread of the NTFSLib store-guards

**Severity:** MAJOR
**Module:** ntfs (boot parameter decoding)
**Parameters affected:** B1 (detection correctness — single-byte tear refuses the volume), A25 (engine algorithm steps), Q68 (documentation accuracy — D-52's WHAT-OG-DOES premise)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSFileSystemAnalyser_decompiled.c` FUN_180004830 (:3427-3440, `if (bVar1 == 0) return 0x400;`) and FUN_180004870 (:3443-3456, `if (bVar1 == 0) return 0x1000;`) — re-read first-hand; `ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_18003f630 (the identical 0x400 default); `ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_180011a10 (:10231-10247, 0x400) + FUN_180011a90 (:10254-10268, 0x1000); `ghidra-mo/NTFSLib_decompiled.c:19315-19348` (the store-guard structure and the shared exit — see below)
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp:44-46` (`clusters_per_index_block == 0 → ERR_BAD_PARAMETER`), `:78-82` (`record_size == 0 → ERR_BAD_PARAMETER` via `record_size_from_dbr`, `include/cr/ntfs/cr_ntfs_wire.h:234`); pin test `BootRecordSizeZeroRejected` (D-52's pin)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Every og scan-path layer that decodes the NTFS record/index-block sizes (the analyser, NTFSSearchDLL, and PartitionRecoveryD twice) substitutes a default when the BPB byte is zero: 1024-byte records, 4096-byte index blocks. The core rejects the volume outright at open. D-52 registered the core's rejection as og parity citing NTFSLib's LoadNtfsVolume — but the cited `:19322/:19331` lines are store-guards around the success stores, not rejection branches; both the guarded and unguarded paths fall through to the same shared exit. A single zeroed byte at +0x40 on a standard 1-KiB-record volume is a whole-volume false negative in the core.

## What the reference does

The analyser's record-size getter (FUN_180004830, :3427-3440) — quoted verbatim:

```c
bVar1 = *(byte *)(param_1 + 0x40);           /* clusters_per_record */
if ((char)bVar1 < '\0') return 1 << (~bVar1 + 1 & 0x1f);   /* shift form */
if (bVar1 == 0) return 0x400;                /* ZERO → DEFAULT 1024 */
return (uint)*(byte *)(param_1 + 0xd) * (uint)*(ushort *)(param_1 + 0xb) * (int)(char)bVar1;
```

and the index-block getter (FUN_180004870, :3443-3456):

```c
bVar1 = *(byte *)(param_1 + 0x44);           /* clusters_per_index_block */
if ((char)bVar1 < '\0') return 1 << (~bVar1 + 1 & 0x1f);
if (bVar1 == 0) return 0x1000;               /* ZERO → DEFAULT 4096 */
return … product …;
```

NTFSSearchDLL's FUN_18003f630 carries the same 0x400 default (the engine's own getter), and PartitionRecoveryD's block-parser pair (FUN_180011a10/FUN_180011a90) default identically — four independent scan-path sites.

The NTFSLib `LoadNtfsVolume` sites D-52 cited as the rejection are, on re-read (:19315-19348): `if (local_1014 != 0) { … if (iVar1 != 0) { …stores… CloseDisk… } } goto LAB_18001683f;` — store-guards; LAB_18001683f is the shared cleanup/return (`return extraout_EAX`, :19348-19353), and the only paths that set the "It is not a NTFS volume!" error string (:19344) are the OEM mismatch, the bps mask, and the `spc × mft_lcn >= total_sectors` gate (:19296-19301). Whether the LIB layer's register-carried return value distinguishes the zero paths cannot be proven from the decompile (`extraout_EAX` is Ghidra's register escape) — but the scan path (which drives recovery behavior) defaults unambiguously at four sites.

## What our core does

`validate_boot_sector` (`cr_bootstrap.cpp:44-46, 78-82`): `clusters_per_index_block == 0` → `ERR_BAD_PARAMETER`; `record_size_from_dbr` returns 0 for cpr==0 (raw wire struct, `cr_ntfs_wire.h:234`) → `record_size == 0` → `ERR_BAD_PARAMETER` ("og: It is not a NTFS volume!" — the comment cites the NTFSLib string). The D-52 pin test `BootRecordSizeZeroRejected` asserts the refusal. Unless the backup DBR is intact (D-99's fallback), the volume is refused.

## Gap

Falsifiable: a standard NTFS volume (1-KiB records, 4-KiB index blocks) with the +0x40 byte zeroed (torn BPB) and a torn/absent volume-end backup: the og's scan path decodes 1024/4096 from the defaults and scans normally; the core refuses the volume (zero structured files). The single-byte tear is in the boot sector's most exposed region.

## Impact

Whole-volume false negative on a single-byte tear — the recovery product's core population. Registry-integrity impact: D-52's WHAT-OG-DOES section asserts the og rejects cpr==0, which is false for every scan-path layer; the register must be corrected regardless of the code decision.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: both analyser getters quoted above; the NTFSSearchDLL/PartitionRecoveryD default sites located via the agent's coverage log and the 0x400 constant confirmed; the NTFSLib store-guard structure and shared exit read directly (the `extraout_EAX` ambiguity recorded honestly above). Core side: the two rejection sites + the pin test confirmed at `34d6cf10`.

## Fix plan (for the engineering team)

1. Decide the stance: (a) og parity — at open, decode cpr==0 → 1024 bytes and cib==0 → 4096 bytes (the four-site default), keep the P1-4 exponent cap and the sub-sector-record rejection; or (b) keep the rejection and REGISTER it as deliberate hardening in D-52 with the four og default sites acknowledged. Recommendation: (a) — the og's behavior is well-defined and the tear population is real.
2. If (a): modify `record_size_from_dbr` (or its caller) to apply the defaults; update `BootRecordSizeZeroRejected` → `BootRecordSizeZeroDefaultsTo1024` (re-pin).
3. Correct D-52's record in either case: the WHAT-OG-DOES premise (rejection) → the four-site default map + the NTFSLib store-guard reading; keep the registered-hardening list accurate (the 4096 bps cap, P1-4, P1-3, bps-vs-device match remain).
4. Files: `src/core/ntfs/cr_bootstrap.cpp`, `include/cr/ntfs/cr_ntfs_wire.h`, `docs/PARITY-DELTAS.md` (D-52), the pin test.
5. Acceptance criteria: the zeroed-+0x40 fixture (no backup) scans with 1024-byte records and the full tree; D-52's text matches the four sites; suite green.

## Tests to add

- `Boot.CprZeroDefaultsTo1024Records` (or the re-pinned name) — RED under current behavior.
- `Boot.CibZeroDefaultsTo4096IndexBlocks`.
- `Boot.CprZeroShiftFormStillDecodes` (the negative-byte path unchanged).
- `Boot.CprZeroWithIntactBackupUnchanged` (the D-99 fallback precedence pinned).

## Related findings

- M-145/M-146/M-141 (the same open-ladder acceptance-band family — the og's salvage posture vs. the core's purity gates).
- S-063 (the og's torn-$MFT mirror fallback — the same "single-point tear must not refuse the volume" principle one layer deeper).

## Open questions

The NTFSLib return value on the zero paths (`extraout_EAX` register escape) — resolvable only by dynamic RE (a debugger on LoadNtfsVolume with a zeroed +0x40) or by accepting the four-scan-path-site evidence as the behavioral ground truth (recommended: the scan path is what recovery behavior follows).
