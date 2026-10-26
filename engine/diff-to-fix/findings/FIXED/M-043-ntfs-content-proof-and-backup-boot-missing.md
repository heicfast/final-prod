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


---

## Resolution (session 14, DFX14)

**Status:** FIXED — with one sub-claim DISPROVEN and corrected in this file
(the finding's "backup-boot fallback missing" half was a false premise; the
corpus wins)
**Fixed in commit:** (this session's M-043 commit — `git log` "fix(M-043)")
**Verified by:** fail-before/pass-after executed: `Orchestrator.ProbeFamily`'s
spoofed-OEM leg FAILED on the fixed tree before its expectation was corrected
(a spoofed "NTFS    " OEM on a FAT16 volume used to dispatch the NTFS engine;
now the content proof rejects it and the 55AA FAT signature underneath wins —
the og's multi-analyser outcome). New `Orchestrator.NtfsProbeContentProof`
(5 legs: zeros-at-$MFT / USA count out of bound / USA offset out of range /
real-shaped record recognized / LCN-zero geometry gate). New pin
`NtfsVolume.TornPrimaryBootWithIntactBackupStillRefusedUntilRepair` (torn
primary + byte-identical volume-end backup -> ERR_BAD_MAGIC; the scan path
never consults the backup). Suite 599/599.
**Resolution notes:** CORPUS RE-VERIFIED FIRST-HAND BEFORE ANY CHANGE —
and it DISPROVED the finding's first half. FUN_180005470 decoded in full
(this session, NTFSFileSystemAnalyser_decompiled.c :4259-4340): (a) the
analyser gates on the PRIMARY boot (bps 512-multiple <= 0x1000; SPC pow2
nonzero) and bails BEFORE result 4 on failure; (b) the "backup-boot
verification" (:4309-4327) is a memcmp of the volume-end backup AGAINST
the primary, storing the verified location at +0x510 (+1 when identical,
bare index when not) — a REPAIR input, never a scan fallback; (c)
NTFSSearchDLL_decompiled.c carries ZERO backup-boot reads (grep clean) —
the og NEVER scans from the backup. So the finding's claim (1) — "the og
recognizes and scans torn-DBR volumes via the backup" — is FALSE; the
core's existing shape (engine refuses; the consent-gated rawfixer owns
the backup, S-008) was already og-parity on that half. NO backup-fallback
code was added (that would have been a divergence, not a fix). Claim (2)
— the missing content proof — was CONFIRMED and FIXED at the recognition
boundary exactly where the og puts it: probe_family's NTFS branch now
applies the og's boot gates (bps 512-multiple <= 0x1000, SPC pow2 nonzero
— NFA :4292-4307) plus the record-0 probe (FILE magic; USA count u16@0x08
in [1, 0x11); USA offset u16@0x06 in [0x2a, 0x2a+0xd7) — FUN_180005180
:4038-4058), one 512-byte read at the $MFT LCN; failure falls through to
the remaining family probes (the og's analyser family). The analyser's
gate ordering (result 4 set before the MFT probe) was noted; the
probe-level form implements the finding's validated semantics. The
finding's fix-plan step 1 (scanner-side backup fallback) is REJECTED on
corpus evidence; step 2 (the content proof) landed as specified.
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


### Concurrent-filing reconciliation (session 14, DFX14 — both legs preserved)

Both legs fixed M-043 in the same window (their landing `1468393`, this
leg's `c2316c2` lineage). The union keeps both Resolutions above as the two
legs' records and reconciles the technical truth on the merged tree:

- CONTENT PROOF: both legs landed it; the merged probe carries THIS leg's
  corpus-exact form (the og boot gates bps 512-multiple <= 0x1000 + SPC
  pow2 from FUN_180005470 :4292-4307; record-0 USA bounds from
  FUN_180005180 :4038-4058; the CORRECT $MFT LCN field at 0x28 — the
  parallel leg's version read 0x30, the $MFTMirr LCN, which only works by
  accident on real volumes where the mirror carries a copy of record 0;
  on mirror-dead volumes it would refuse a valid $MFT). Their
  `return Unknown` short-circuit was also dropped for the og's
  analyser-family fall-through (a spoofed OEM over a FAT sector probes
  Fat, exactly like the og's multi-analyser recognition).
- BACKUP FALLBACK (their step 1): KEPT as landed, but RE-REGISTERED — it
  is NOT og parity. This session's first-hand corpus decode (recorded in
  this leg's Resolution above) proves the og has NO scan-path backup
  fallback: the analyser's backup step is a REPAIR-input verification
  (verified location +0x510, +1-when-identical), and NTFSSearchDLL never
  reads the backup. The fallback is a deliberate RECOVERY SUPERSET —
  torn-DBR volumes open read-only with the boot_from_backup flag while
  repair stays consent-gated. Registered as PARITY-DELTAS D-73; bench
  comparisons must expect core-only finds on torn-DBR NTFS fixtures.
  This leg's pre-fallback pin (refusal assertion) is superseded by the
  landing's own test set (TornPrimaryBackupBootScans /
  TornPrimaryShapeBackupBootScans / BackupBadFallsBackToRefusal /
  BootFromBackupFlagged) and removed in the same merge.