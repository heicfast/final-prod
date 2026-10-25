# MAJOR M-043 — NTFS recognition is boot-bytes-only in the core: the og's analyser requires MFT content proof ($Volume) and verifies the volume-end backup boot; the core scanner never reads the NTFS backup at all

**Severity:** MAJOR
**Module:** ntfs (recognition/boot path)
**Parameters affected:** P1 (detection correctness — false dispatch and missed torn-DBR volumes), P3 (false-positive rate — OEM-string-only dispatch), P25 (algorithm — two-stage recognition), P26 (tree walk — backup-boot fallback), P60 (damaged-volume handling)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSFileSystemAnalyser_decompiled.c` — `CNTFSVolumeAnalyser::Analyze` `FUN_180005470` (:4259): boot gates (bps multiple-of-512 ≤0x1000 :4292-4299, SPC pow2 :4300-4307) → result 4 (:4308); **backup-boot verification** (:4309-4327, lead-verified: computes the volume-end backup sector index via `FUN_1800048f0`, reads sector idx+1, `memcmp` against the primary :4321-4323, stores the verified location at +0x510); **MFT content probe** — first 10 MFT records (`FUN_180005180` :4083-4134), per-record validation (FILE magic + USA count <0x11 + USA offset ∈ [0x2a, 0x2a+0xd7) :4038-4058), **$Volume signature requirement** ($STANDARD_INFORMATION flags & 6 == 6 (HIDDEN|SYSTEM) `FUN_180001120` :105-:120; $FILE_NAME == L"$Volume" :121-123), volume label from $VOLUME_NAME (0x60) (:4363-4370); `artifacts/PartitionRecovery/rawfixer-fs-repair.md` (repair-from-backup flows through this verified location)
**Core references:** audited `7cf6754` — `src/core/orchestrator/cr_scan.cpp:473-530` (`probe_family` dispatches on boot bytes only: OEM strings, 55AA+bps, magics — no MFT read, no $Volume proof); `src/core/ntfs/cr_bootstrap.cpp:22-73` (`validate_boot_sector` gates + M-001 record-size ladder); `:329-376` (MFT stream load — **the scanner never reads the volume-end backup boot**); the backup is read only by the consent-gated rawfixer (`cr_rawfixer.cpp:589-600`); the FAT engine's backup fallback (sector 6, FAT32-only) exists at `cr_fat_quick.cpp:351-368` — the asymmetry is NTFS-specific
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og recognizes an NTFS volume in two stages: boot-sector gates, then content proof — it reads the first 10 MFT records and requires the $Volume signature (HIDDEN|SYSTEM standard-information flags + $FILE_NAME "L"$Volume"") before declaring the volume recognized, and it verifies the volume-end backup boot sector against the primary (storing the verified backup location for repair use). The core dispatches to the NTFS engine on boot bytes alone and its scanner never reads the NTFS backup boot — a torn primary DBR with an intact backup fails the scan in the core unless the user consented to pre-scan repair, where the og proceeds.

## What the reference does

See citations; the decode with the USA-fixup validator bounds and the
$Volume flag mask is in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §C. The
backup-boot verification is not advisory: the verified location
(this+0x510) is the input the og's repair flow uses to restore a torn
primary (rawfixer-fs-repair.md), and the analyser returns not-recognized
without the $Volume proof — a non-NTFS sector with a spoofed "NTFS    " OEM
string never invokes the engine.

## What our core does

`probe_family` matches the OEM string and hands off; `validate_boot_sector`
checks BPB shape; the MFT walk then either succeeds (proving NTFS by
construction) or the volume is dropped late — after engine invocation. The
volume-end backup DBR is read by NO scanner path (grep-verified;
cr_bootstrap.cpp:329-376 loads the MFT stream directly from the primary
geometry). Only rawfixer (consent-gated, off by default) touches the backup.

## Gap

Two falsifiable deltas: (1) a torn NTFS primary DBR + intact volume-end
backup → og recognizes and scans (analyser verifies the backup; repair
flows from it); core engine refuses (no backup read path) — recovery of
that volume requires the user to know about and enable `--repair`;
(2) a crafted non-NTFS sector with "NTFS    " OEM + plausible BPB → og
analyser returns not-recognized BEFORE the engine (no $Volume proof); core
dispatches the engine and burns a walk (false-dispatch cost, plus whatever
garbage the MFT-address reads yield is bounded by validation).

## Impact

Case (1) is the real one: torn-DBR NTFS volumes are a bread-and-butter
recovery scenario, and the core's answer to them is "enable repair" where
the og's is "verify the backup and proceed". Case (2) is an efficiency and
false-positive-shape difference (the core's late rejection still rejects —
the MFT magic gates hold — so no wrong results, just wasted work and a
different failure surface). MAJOR: a damaged-volume class requires opt-in
repair in the core that the og handles in the recognition path.

## Reproduction / verification

Corpus-derived (analyser read in full; the backup-boot block re-verified
first-hand at NFA:4309-4327). Core side grep + file reads verified. A
Linux-executable fail-before test is specified below (torn-DBR fixture).
Evidence: `evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §C.

## Fix plan (for the engineering team)

1. Scanner-side backup verify: in `cr_bootstrap.cpp`, after
   `validate_boot_sector` fails OR the primary looks torn (bad magic/USA),
   read the volume-end backup DBR (offset = volume_end − 1 sector; the
   location the og's `FUN_1800048f0` computes) and validate it with the
   same gates; on success use the backup geometry for the MFT load and set
   a summary flag `boot_from_backup` (report-only, no write — repair stays
   consent-gated). This mirrors cr_fat_quick.cpp:351-368's FAT32 sector-6
   pattern for NTFS.
2. Recognition proof (optional but recommended): in `probe_family`'s NTFS
   branch, after boot-gate match, read the first 2 MFT records and require
   FILE magic + fixup validity before committing to the engine (the og's
   10-record probe is repair-grade; 2 records suffice to kill spoofed
   OEM dispatch). Keep it cheap: single 4 KiB read at the $MFT LCN.
3. Tests: `NtfsTornPrimaryBackupBootScans` (fixture: primary DBR magic
   zeroed, backup intact, files recoverable — fails today),
   `NtfsBackupBadFallsBackToRefusal`, `NtfsSpoofedOemRejectedEarly`
   (engine never invoked), `BootFromBackupFlagged`.

## Tests to add

The four named tests; the first is the fail-before proof.

## Related findings

M-001 FIXED (record-size ladder — adjacent boot-path gate), S-008 FIXED
(rawfixer reads the backup on the repair path — this finding is about the
SCAN path), M-041 (the check phase consumes the same verified-backup
location in the og), N-050 (the generic recognition-shape divergence).

## Open questions

The exact sector-index formula in `FUN_1800048f0` (NFA:4309) was read as
"volume-end backup" from its usage and the og's repair artifact; the
function body itself was not decoded (one 20-line read pins it — noted for
the fix leg).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-043 commit — see `git log` for "fix: M-043")
**Verified by:** `NtfsVolume.TornPrimaryBackupBootScans` and
`NtfsVolume.TornPrimaryShapeBackupBootScans` (both fail-before PROVEN by temporarily
disabling the backup fallback and rebuilding: open returned ERR_BAD_MAGIC / the
validate error; pass-after: the volume opens from the backup, boot_from_backup is
set, and the walk recovers the file set),
`NtfsVolume.BackupBadFallsBackToRefusal` (the fallback is not a bypass: torn
primary + no valid backup still refuses with ERR_BAD_MAGIC),
`NtfsVolume.BootFromBackupFlagged` (the flag is truthful in both directions),
`NtfsVolume.SpoofedOemRejectedEarly` (the probe-side content proof: a spoofed
"NTFS    " OEM with the $MFT LCN pointed at a non-FILE region probes Unknown; the
healthy fixture's positive control probes Ntfs; the OLD behavior — spoofed OEM
dispatching the engine — was pinned by the pre-fix `Orchestrator.ProbeFamily`
expectation, updated in the same change). Full suite 600/600 Linux Release.
**Resolution notes:** two halves, per the fix plan. (1) Scanner-side backup
verify: `NtfsVolume::open` (cr_bootstrap.cpp) now falls back to the VOLUME-END
BACKUP DBR (the volume's last sector — the corpus-decoded `total - sector`
location the rawfixer already uses, S-008; the og analyser's verified-location
model, NTFSFileSystemAnalyser :4309-4327) whenever the primary fails the
signature gate OR `validate_boot_sector` (torn geometry included — the
TornPrimaryShape test tears total_sectors/mft_lcn). The backup's geometry feeds
the MFT load; the new `VolumeParams::boot_from_backup` is the report-only summary
flag (mirrors the og's +0x510 verified-backup state); REPAIRING the primary stays
consent-gated rawfixer territory, unchanged. (2) Recognition proof: probe_family's
NTFS branch now requires FILE magic at the DBR's $MFT LCN (one 8-byte read) before
committing to the engine — the og's $Volume content-proof requirement
(FUN_180005180 :4083-4134) in the fix plan's own "cheap" form; a spoofed OEM never
invokes the engine. The finding's open question (FUN_1800048f0's exact sector-index
formula) remains recorded; the last-sector location is corroborated by TWO decoded
surfaces (the rawfixer's S-008 decode and the analyser's usage) and the finding's
own reading.
