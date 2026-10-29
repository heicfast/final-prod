# SEVERE S-063 — NTFS torn-$MFT bootstrap: the og falls back to reading record 0 at the $MFTMirr LCN when the primary read fails; the core refuses the whole volume — and its own comment talks itself out of the fallback the og actually has

**Severity:** SEVERE
**Module:** ntfs (volume bootstrap)
**Parameters affected:** B1 (detection correctness), B2 (recovery correctness), A25 (engine algorithm steps), S35 (error recovery / fallback)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_1800272b0 (:31585-31630) — the walk-entry bootstrap; getters FUN_18003f990 (:51179-51183, returns struct+0x30 = $MFT LCN) and FUN_18003f970 (:51170-51174, returns struct+0x38 = $MFTMirr LCN), re-read first-hand; `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md:30-33` ("MFT LCN, MFT mirror LCN … `$MFT` datarun read … `L"$MFTMirr"` — mirror fallback")
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp:96-119` (`load_mft_stream`, primary-only) and `:410-421` (`NtfsVolume::open`'s refusal with the "nothing to mirror FROM" comment)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When the og's NTFS walk entry cannot read MFT record 0 at the primary `$MFT` LCN (or the record geometry is odd/oversized), it retries the identical chunk-reader call at the `$MFTMirr` LCN before giving up (error 0x610016). The core's `NtfsVolume::open` returns nullptr on the first failure; a code comment explicitly reasons that "if primary is unreadable there is nothing to mirror FROM" — but the mirror's record-0 slot holds a copy of the $MFT's own record, which carries the full $DATA runlist: there is everything to bootstrap from. On volumes with a torn first $MFT cluster and an intact mirror, the og scans the whole volume and the core recovers nothing structured.

## What the reference does

FUN_1800272b0 (`NTFSSearchDLL_decompiled.c:31585-31630`), the MFT walk entry — re-read first-hand this session, with both getters resolved to their definitions:

Primary path (:31616-31627):
```c
uVar3  = FUN_18003f940(pvVar1);          /* spc (shift-decoded, FUN_18003f5d0) */
lVar11 = FUN_18003f990(pvVar1);          /* struct+0x30 — $MFT LCN */
lVar2  = *(longlong *)(param_1 + 0x38);  /* volume base LBA */
uVar8  = record_bytes / device_sector;   /* record size in device sectors */
if (((uVar8 & 1) != 0) || (0x10 < (byte)uVar8)) goto LAB_180027467;   /* odd/>16 sectors → fallback */
iVar5 = FUN_18001a3c0(lVar11 * (ulonglong)uVar3 + lVar2, (byte)uVar15, …);  /* read record 0 */
if (iVar5 == 0) goto LAB_180027467;      /* read failed → fallback */
```

Fallback path (LAB_180027467, :31595-31609):
```c
uVar3  = FUN_18003f940(pvVar1);
lVar11 = FUN_18003f970(pvVar1);          /* struct+0x38 — $MFTMirr LCN */
lVar2  = *(longlong *)(param_1 + 0x38);
uVar7  = (record_bytes + …) >> 9;        /* record size in 512-byte units, rounded */
if (((uVar7 & 1) == 0) && ((byte)uVar7 < 0x11)) {
    iVar5 = FUN_18001a3c0((ulonglong)uVar3 * lVar11 + lVar2, (byte)uVar15, …);  /* SAME reader, mirror LCN */
    if (iVar5 != 0) goto LAB_1800274f5;  /* bootstrapped from the mirror */
}
uVar8 = 0x610016;                        /* both failed → the walk's refusal code */
```

The two getters are adjacent qwords of the parsed boot struct (`FUN_18003f990` → +0x30, `FUN_18003f970` → +0x38 — the kernel-layout $MFT/$MFTMirr LCN pair), confirmed at their definitions (:51170-51183). The corpus's own engine doc corroborates the mechanism ("`L"$MFTMirr"` — mirror fallback", `ntfs-engine-internals.md:31-32`). Note the fallback also catches the odd/oversized record-geometry case the primary path's `uVar8 & 1` / `> 0x10` gates reject (the 4-KiB-record/4Kn-device quirk — see F-D10/N-189).

## What our core does

`load_mft_stream` (`cr_bootstrap.cpp:96-119`) reads record 0 at `p_.mft_lcn * p_.cluster_bytes` exactly once; magic/fixup/attribute failures return the error. `NtfsVolume::open` (`cr_bootstrap.cpp:410-421`):

```cpp
rc = v->load_mft_stream();
if (rc != OK) {
    /* corpus: $MFTMirr cross-reference as repair/verification source when
     * primary fails. Try mirror's $MFT? The mirror holds records 0..3
     * copies - for bootstrap we only need record 0's runlist; if primary
     * is unreadable there is nothing to mirror FROM - surface the error. */
    if (err)
        *err = rc;
    return nullptr;
}
```

The comment's premise is wrong: `load_mftmirr_stream` (the best-effort call at :424) proves the core knows the mirror region is readable independently — and the mirror's first record is a copy of $MFT record 0 itself, whose $DATA runlist describes the entire primary $MFT. Bootstrapping from the mirror does not require the primary's first cluster at all.

## Gap

A volume whose first $MFT cluster is torn/unreadable (single unreadable sector at `mft_lcn × cluster_bytes`) with an intact $MFTMirr (standard NTFS places the mirror at the volume midpoint, so a head-of-volume tear leaves it intact): the og reads record 0 at the mirror LCN, decodes the same $DATA runlist, and walks the full MFT; the core refuses the volume (zero structured files; carve only). Falsifiable: corrupt the first cluster of $MFT in a fixture image; the og's file count ≈ the intact control; the core returns the open error.

## Impact

Whole-volume structured recovery loss on the torn-$MFT population — the classic NTFS damage mode (boot-area tears) that $MFTMirr exists for. The og's resilience path is documented in the corpus artifact; the core's refusal is user-observable ("og finds thousands of files, we find none"). Also a self-inflicted documentation hazard: the core's comment asserts a corpus reading that is the opposite of what the decompile shows.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session (both getters traced to definitions; the fallback's reader call confirmed identical to the primary's; the artifact's mirror-fallback line corroborates). Core side: full read of `load_mft_stream`/`open`/`load_mftmirr_stream` at `34d6cf10`. No executed fixture this session — the fix plan's first test is the executable pin (the corpus decode is unambiguous).

## Fix plan (for the engineering team)

1. In `NtfsVolume::open`, when `load_mft_stream()` fails on the primary: before giving up, retry the record-0 read at `p_.mftmirr_lcn * p_.cluster_bytes` (the mirror's record-0 slot), applying the same magic/fixup/attribute decoding; on success, continue the open with the mirror-derived runlist and set a flag (e.g. `boot_from_mftmirr`, analogous to `boot_from_backup`).
2. Keep the mirror failure non-fatal afterward (current `load_mftmirr_stream` best-effort semantics unchanged).
3. Record the fallback in the skip-honesty/stats family (`mft_bootstrapped_from_mirror` counter) and log the event (the og logs its mirror events).
4. Pin the geometry quirk while there: the og routes odd/>16-device-sector record sizes through the same fallback — decide deliberately whether the core's P1-4 record-size cap covers that class (it does, more strictly) and register the stance (cross-ref N-196).
5. Files: `src/core/ntfs/cr_bootstrap.cpp` (+ the flag in `cr_bootstrap.h`'s `VolumeParams`/flags), tests in `tests/unit/test_ntfs_volume.cpp` (or the engine's bootstrap test home).
6. Acceptance criteria: torn-first-$MFT-cluster fixture (mirror intact) → full tree equal to the intact control; intact-volume behavior byte-identical; the `boot_from_mftmirr` flag observable in the open diagnostics.

## Tests to add

- `NtfsVolume.TornMftRecord0BootstrapsFromMirror` — RED today (open fails).
- `NtfsVolume.MirrorBootstrapProducesSameRunlist` (the mirror record-0 $DATA decodes to the primary's extents on the intact control).
- `NtfsVolume.BothMftAndMirrorTornRefuses` (the honest refusal preserved).
- `NtfsVolume.MirrorBootstrapFlaggedInStats`.

## Related findings

- S-066 (the exFAT anchor re-base — the same damaged-volume salvage class).
- D-99 (the registered backup-DBR fallback this finding complements: that one covers the boot sector, this one covers the MFT bootstrap).
- N-187 (the lost-scan probe's missing mirror retry — the PartitionRecoveryD twin of the same og pattern on the candidate side).

## Open questions

None on the og side (the decode is complete). One implementation note: the og's fallback uses a 512-byte-rounded record size (`>> 9`) where the primary uses device-sector units — the fix should use the core's existing `record_size` for both reads; the difference only matters on non-512 device geometry and the core's P1-4 cap already governs.

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the S56 ACUTE-first lane, 2026-10-29)
**Verified by:** fail-before/pass-after on three of the four pins (the fourth
is the preserved-refusal control, green both sides):
`NtfsVolume.TornMftRecord0BootstrapsFromMirror` (RED: "open err bad magic" —
the whole-volume refusal reproduced; GREEN: the open succeeds,
`boot_from_mftmirr` set, and the walk recovers the same file set ≥ 5 through
the mirror-derived runlist), `NtfsVolume.MirrorBootstrapSameRunlistAsIntactControl`
(RED: the same refusal; GREEN: the torn volume's walked set == the intact
control's — the mirror record-0 $DATA decodes to the primary's extents),
`NtfsVolume.BothMftAndMirrorTornRefuses` (green both sides — the og's
0x610016 refusal class preserved, the fallback is not a bypass),
`NtfsVolume.MirrorBootstrapFlaggedInOpenDiagnostics` (RED: the refusal;
GREEN: the params flag + the Info log line at the seam). The pre-fix RED was
proven by neutralizing the fallback branch at an otherwise-identical tree.
NTFS volume family 76/76; RawFixer 33/33 (one test RE-TRUTHED — below);
full suite 1453/1453.
**Resolution notes:** CONFIRMED-DIVERGENCE, corpus re-verified first-hand:
FUN_1800272b0's primary path and the LAB_180027467 fallback block re-read
at NTFSSearchDLL:31585-31630 — the identical chunk reader
(FUN_18001a3c0) retried at the $MFTMirr LCN (FUN_18003f970 → boot
struct +0x38) when the primary read fails or the odd/>16-sector geometry
gates reject it; both failing is the 0x610016 refusal. The fix is the
finding's plan: `load_mft_stream(u64 at_lcn, bool from_mirror)`
parameterizes the record-0 read position; `NtfsVolume::open` retries at
`p_.mftmirr_lcn` when the primary fails (mft_lcn != mftmirr_lcn guarded),
sets `VolumeParams::boot_from_mftmirr` (the report-only open diagnostic,
`boot_from_backup`'s sibling) and logs the event through the cr::log seam
(Level::Info — the og logs its mirror events); both failing surfaces the
primary's error exactly as before. The wrong "nothing to mirror FROM"
comment is replaced by the verified decode. En-route truth work: the
RawFixer `MftRestoreMakesVolumeOpenable` test RE-TRUTHED — its "torn
record 0 must fail volume open" premise was the finding's own defect class
(the pin now asserts the boot SOURCE: mirror pre-repair, primary
post-repair, mirror again post-undo — a sharper transactional pin than the
old nullptr form). The fixture quirk documented: the test fixture's
$MFTMirr region overlaps the $MFT extent (clusters 68..71), so the
mirror's record-0 copy must be re-planted in the S-063 tests (a fixture
artifact, not a volume-model defect — real volumes never overlap the two).
The geometry-quirk stance registered per the fix plan: the core's P1-4
record-size cap already governs both positions more strictly than the
og's 512-byte-rounded fallback re-check (cross-ref N-196). No safety
hardening touched — the repair path stays consent-gated (RawFixer S-008).
