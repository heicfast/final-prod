# SEVERE S-008 — RawFixer: boot-sector-only subset, not wired pre-scan (og: 33-method FS repair incl. MFT/FAT-table/root, invoked before scanning)

**Severity:** SEVERE
**Module:** rawfixer, orchestration (wiring)
**Parameters affected:** B6 (damaged-volume behavior), F63 (RawFixer consent/gating), A25, R57 (wiring)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/rawfixer-fs-repair.md` (role: "repairs FS metadata enough for structured scan"; every fixer implements Undo; strategy trees per FS); `rawfixer-v2--method-inventory.md` §1 (33 verbatim-traced methods: shared CheckDBR/FixFromDBR/UpdateDBR/FixDBR/FixByScanSector/CheckSector/Undo; FATFixer CheckRootDir/IsValidDBR/CheckFatTable/CheckCluster/CheckBackupDBR; NTFSFixer IsValidRootDir/**CheckMFT**; ExFATFixer TryFixFromSectorsPerCluster/ReadCluster/CopySectors/ChecksumDBR/CheckRoot/CheckMustBeZero); §3 (CheckMFT mechanics, decompile-verified); `CRawFixer::Start` trace @0x169 / `CheckPartition` dispatch (pre-scan position in the workflow)
**Core references:** `src/core/repair/cr_rawfixer.cpp` (558 LOC — boot sectors only; header comment "No MFT mirror repair, no FAT-table repair, no GPT/MBR repair — boot sectors only" per agent-D read; analyze :171–436, transactional execute :440–538); `src/core/orchestrator/cr_scan.cpp` (no repair invocation — grep: zero references); `src/app/rrecov.cpp:1325–1387` (repair is a manual CLI command) — audited commit `67193b6`
**Status:** CLOSED — FIXED in `e0175b3` (verified independently by the session-2 audit: RawFixer_decompiled.c FUN_18005de40/FUN_18005df00 re-read first-hand at :64981/:65013 — both check `*piVar3 == 0x454c4946` ('FILE' magic) then the record-name; core CheckMFT/CheckFatTable/CheckRootDir-advisories/FixByScanSector in cr_rawfixer.cpp with corpus cites + pre-scan wiring in cr_scan.cpp :530-532 behind ScanConfig::pre_scan_repair + RepairHooks consent confirmed in-tree; 16 fail-before tests incl. MftRecord0RestoredFromMirror / FatTornMirrorCopyRestoredSectorGranular / PreScanRepairRestoresTornFatAndScans green in the 313/313 ASan suite; see Resolution)

## Summary

The og's RawFixer is a **pre-scan** transactional repair subsystem with per-FS strategy
trees: it validates the DBR, the MFT (restoring from $MFTMirr), the FAT tables, and the
root directory, fixing what it can (with CopySectors undo logs) so the structured engines
can scan. Our core implements only the boot-sector subset (NTFS backup restore, exFAT
backup+checksum+geometry, FAT backup-6) and exposes it as a **manual CLI command** — the
scan flow never invokes it. On volumes with a damaged $MFT record 0 or a damaged FAT
table, the og repairs and proceeds to structured recovery; our engines fail to open and
the user gets carving only.

## What the reference does

Method inventory (33 methods, verbatim trace strings, rawfixer-v2 §1). Strategy tree
(rawfixer-fs-repair §Repair strategy): NTFS: CheckDBR ok? CheckMFT ok? done; else FixDBR
(in-place) / FixFromDBR (backup restore) / FixByScanSector (deep) → UpdateDBR → verify
(IsValidRootDir/CheckMFT). FAT: CheckDBR → CheckBackupDBR → CheckFatTable →
CheckRootDir/CheckCluster. exFAT: FixFromDBR → ReadCluster verification. `CRawFixer::Start`
+ `CheckPartition` sit in the pre-scan position of the recovery workflow (repair, then
scan). `NTFSFixer::CheckMFT` decodes MFT record sizing from the BPB (signed
clusters-per-record, `1 << (-cpr & 0x1F)` — decompile-verified) and validates/repairs the
$MFT (mirror-backed), aborting with the 0xFFFFFFFF sentinel on validation failure.

## What our core does

`cr_rawfixer.cpp` implements: family detection, NTFS volume-end backup restore, exFAT
backup-12 restore + checksum recompute + SectorsPerClusterShift rederivation (the
TryFixFromSectorsPerCluster parity), FAT backup-6 restore — all transactional with an
excellent undo-log discipline (sector-span RMW, read-back verify S5, distinct
REFUSED/UNDO_FAILED codes). It does **not** implement CheckMFT, CheckFatTable,
CheckRootDir, IsValidRootDir, FixByScanSector, or ReadCluster verification. The
orchestrator never calls it; `rrecov` exposes `repair`/`undo` as manual commands.

## Gap

Falsifiable claims: (1) a volume whose $MFT record 0 is torn but whose $MFTMirr is intact
is structured-scannable in the og (after CheckMFT repair) and not in the core (NTFS open
fails → carve only, unless the user manually runs repair); (2) a volume with a damaged
FAT1 but intact FAT2 is repaired+scanned in the og (CheckFatTable) and not in the core
(no FAT-table repair exists).

## Impact

- Structured recovery (names, paths, tree) is lost exactly on the damaged volumes that
  are the product's core scenario; carving-only results (no names) are strictly worse
  UX and recall.
- The repair code that exists is high quality but unreachable from the normal flow.

## Reproduction / verification

- Corpus-derived: method inventory (verbatim traces), strategy tree, workflow position
  (rawfixer-fs-repair + rawfixer-v2, both cited above).
- Core-derived: module scope comment; grep evidence of no orchestrator wiring; CLI
  command list.

## Fix plan

1. **Wire pre-scan repair (consent-gated)**: add a `ScanConfig::pre_scan_repair` stage —
   before `probe_family`, run `repair::analyze` on the slice; if the plan is non-empty,
   surface it through the progress/consent callback (the cr_ui ABI already has a
   callback surface) and only execute on consent; then proceed to probing. Default OFF in
   the CLI, ON in the UI flow (matches the og's consent posture).
2. **Add the missing repair methods**, in value order:
   a. `CheckMFT`/`FixMFT` — validate $MFT record 0 (magic+fixup); on failure, locate
      $MFTMirr (mftmirr_lcn), copy its records over the primary's record 0 (sector-granular,
      undo-logged — reuse the existing transactional machinery).
   b. `CheckFatTable` — compare FAT1/FAT2 sector-by-sector; if one copy is torn and the
      other sane, restore the torn one (undo-logged).
   c. `CheckRootDir`/`IsValidRootDir` — validate the root's first sector shape (FAT) /
      root record (NTFS); report-only in v1 (repair requires directory reconstruction —
      register as follow-up).
   d. `FixByScanSector` — the og's deep DBR reconstruction: scan candidate sectors for
      boot-shaped data and rebuild the DBR. Implement as a bounded windowed probe of the
      first 64 MiB (register scope).
3. Files: `src/core/repair/cr_rawfixer.cpp` (+ header), `src/core/orchestrator/cr_scan.cpp`
   (stage), `src/app/rrecov.cpp` (flag), `src/ui/cr_ui.cpp` (consent surface).
4. Tests: per method — `Repair_MFT_From_Mirror`, `Repair_FAT2_From_FAT1`,
   `PreScan_Repair_Consent_Gated`, plus transaction/rollback cases mirroring the existing
   13 rawfixer tests.

## Tests to add

(fix plan step 4 — fail before, pass after)

## Related findings

- S-006 (lost-partition probes — the other damaged-volume on-ramp).
- A-001 (LDM GUIDs — structured path for dynamic disks).

## Open questions

The og's consent UX for RawFixer (auto vs prompted) is not fully decoded from the corpus
(the PPL-async progress plumbing suggests a UI prompt); the fix plan gates on consent
either way.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the S-008 fix commit on main)
**Verified by:** 16 new tests, all fail-before/pass-after (fail-before reproduced by stashing only
`src/core/repair/cr_rawfixer.cpp` resp. `src/core/orchestrator/cr_scan.cpp` and re-running):
`RawFixer.MftRecord0RestoredFromMirror`, `RawFixer.MftCleanNeverTouched`,
`RawFixer.MftBothInvalidNoWrite`, `RawFixer.MftRestoreMakesVolumeOpenable` (the finding's
falsifiable claim (1) proven at the engine boundary: torn $MFT record 0 + intact $MFTMirr ->
`NtfsVolume::open` fails pre-repair, succeeds post-repair, fails again after undo),
`RawFixer.FatTornMirrorCopyRestoredSectorGranular`, `RawFixer.FatTornPrimaryRestoredFromMirror`,
`RawFixer.FatMirrorAmbiguityNeverWritten` (S3), `RawFixer.FatCleanMirrorNoAction`,
`RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate`,
`RawFixer.FixByScanSectorAmbiguousCandidatesRefused` (S3),
`RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous` (content-dedupe),
`Orchestrator.PreScanRepairRestoresTornFatAndScans` (falsifiable claim (2) end-to-end: torn FAT1
repaired pre-scan after consent, engines observe repaired bytes, structured scan completes, event
stream carries ready->applied), `Orchestrator.PreScanRepairDeniedNeverWrites`,
`Orchestrator.PreScanRepairWithoutHooksNeverWrites` (S1 default-deny),
`CrUi.PreScanRepairConsentGateAndUndoLog` + `CrUi.PreScanRepairDeniedLeavesFileUntouched` (the C
ABI lifecycle incl. on-disk repair of the image file, stats fields, CR_EVT_REPAIR events, undo
container persistence) + the extended `tests/ui_abi_check.c` (MSVC /W4 /WX consumer, Linux
ASan-clean run). Full suite 297/297 (Linux ASan preset).

**Resolution notes:** Independently re-verified the finding against the corpus before fixing:
`rawfixer-v2--method-inventory.md` (33-method table; CheckMFT record-size math §3; FAT
CheckFatTable reads both copies - wFatTableStartSector and backupFatStartSector = reserved +
sectors-per-FAT - and flags damage) and the raw decompile
(`re-workspace/reversed/ghidra-partition/RawFixer_decompiled.c`: FUN_18005de40 = 'FILE' magic +
record-name check; FUN_18005df00 = $MFTMirr identity check; NTFSFixer::CheckSector's state
machine classifies $MFT/$MFTMirr records; FATFixer::CheckFatTable @0x10e) - the finding's
reference claims held; nothing in it needed correction. CONFIRMED-DIVERGENCE, fixed as follows.
(1) Repair methods added to `cr_rawfixer.cpp` analyze(): CheckMFT - $MFT record 0 validated by
magic + USA fixup (the engine's own `fixup_validate`, single source of truth) + $FILE_NAME
identity "$MFT" (the og's name check); on failure the $MFTMirr slot 0 is validated the same way
and planned as `RestoreMftRecordFromMirror` (only the mirror-backed restore, never a synthesized
record; both invalid -> advisory, no write). CheckFatTable - both FAT copies stream-compared in
64 KiB blocks; when exactly one copy carries a valid entry signature (entry0=media, entry1=EOC at
the width's packing), the torn copy's differing sectors are planned sector-granular
(coalesced runs) as `RestoreFatSectorFromMirror`; equal validity + differences = S3 advisory, no
write. CheckRootDir/IsValidRootDir/CheckRoot - report-only advisories per the fix plan's v1 scope
(repair requires directory reconstruction; registered here as the remaining follow-up surface).
FixByScanSector - bounded 64 MiB windowed probe (64 KiB block reads, X51 discipline) for
boot-shaped sectors, family-masked; byte-identical candidates dedupe to one repair; distinct
multiples refused (S3); wired as the deep fallback for NTFS (no volume-end backup), exFAT (no
sector-12 backup), FAT (no sector-6 backup) and the family-unknown terminal. (2) Pre-scan wiring:
`ScanConfig::pre_scan_repair` + `RepairHooks` (consent / open_writer / close_writer) run
repair::analyze on the caller's slice BEFORE the retry/cache pipe (writes bypass the cache; the
engines see repaired bytes); a non-empty plan is surfaced through the typed event stream (new
`Repair` event type, ready->applied/denied/failed) and `ScanStats` repair fields incl. the undo
log; execution is consent-gated (S1: no consent callback = never executed) and a denied/failed
repair never aborts the scan (corpus fall-through to the unrepaired scan). (3) CLI:
`rrecov scan --repair` (analyze + report, dry-run) / `--repair-apply` (the flag is the consent) /
`--repair-undo <file>` (durable undo container; default rrecov-scan-repair-diskN.undo; restore
via the existing `fix --undo-apply`); volume scans map writes through the slice via
`VolumeWriteSlice`. (4) cr_ui C ABI (ABI-since 1.2, append-only; the event struct consumes the
reserved slots so the layout is unchanged): `cr_ui_scan_config.pre_scan_repair` +
`cr_ui_repair_consent_fn(const cr_ui_repair_plan*)`, CR_EVT_REPAIR, repair fields in
`cr_ui_scan_stats`, `cr_ui_scan_repair_save_undo`; the shim opens the image file r+b (portable)
or the Windows RW transport mapped through the device slice; resumed scans keep the gate.
Safety posture preserved and extended (noted per mandate): the consent gate (S1), clean-refusal
(S2), ambiguity-refusal (S3 - the CheckFatTable equal-validity case and multi-candidate deep
scan), transaction (S4) and verify (S5) all hold; S6's scope line was rewritten to include the
two new mirror-backed restore families (FAT table copies, $MFT record 0 from $MFTMirr) while
directory CONTENT stays unwritable by design.

## Session-2 main-auditor leg — full 33-method og repair table for fix auditing (2026-10-25)

`e0175b3` (pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector) verified as a
substantial down-payment; the complete og set (RawFixer_decompiled.c, quotes in
`../evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`) for auditing
completeness: FixFromDBR (NTFS fallback = **last sector of volume** `:66010-14`; exFAT
backup-12) · FixDBR in-place · **UpdateDBR: 11-sector template + sector-0 overlay, checksum
fill-repeated, read-back verify, writes 12 sectors @LBA0 AND 12 @LBA12 (main AND
backup)** `:11413-66` · FixByScanSector · CheckMFT (spec-exact record sizing `cpr<0 →
1<<(-cpr&0x1f)` else `cpr*bps` `:67878-87`, mirror-backed repair, disk write) ·
CheckBackupDBR (FAT sector-6) · **CheckFatTable (FAT2 mirror read at FAT1+spf, repair =
WriteSector FAT2→FAT1)** `:60940-81` · CheckRootDir/CheckCluster · ChecksumDBR (predicate =
ROL31+add skipping table DAT_180079a40 = {106,107,112} — MS-spec skip, core MATCH; this leg
corrected its own earlier no-skip mis-read) · TryFixFromSectorsPerCluster · CheckMustBeZero ·
CheckRoot · ReadCluster post-repair verify · CopySectors snapshot + **Undo on every path** ·
consent = pre-scan DRWUI damaged-volume phase (no in-DLL prompt). Remaining edges if the fix
aims at full parity: backup-region rewrite, ReadCluster verify, CheckRootDir/CheckCluster
wiring, and N-025's geometry defaults (zero-interleave → 64 KiB).
