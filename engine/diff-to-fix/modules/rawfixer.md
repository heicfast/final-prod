# modules/rawfixer.md — RawFixer audit verdicts

Reference: RawFixer.dll per `PartitionRecovery/rawfixer-fs-repair.md` +
`rawfixer-v2--method-inventory.md` (33 methods, strategy trees, PPL-async, CopySectors
undo, CheckMFT mechanics decompile-verified, ChecksumDBR loop bound verified).

Core (audited `67193b6`; **S-008 fixed post-audit**): `src/core/repair/cr_rawfixer.cpp` +
`tests/unit/test_rawfixer.cpp` (24 areas).

## Verdicts

- **F63 consent** — `M (S-008 FIXED)`: pre-scan wiring lives in the orchestrator
  (`ScanConfig::pre_scan_repair` + consent/open-writer/close-writer hooks), CLI
  `--repair`/`--repair-apply`, cr_ui consent ABI (`cr_ui_repair_consent_fn` +
  `cr_ui_repair_plan`), CR_EVT_REPAIR events, undo-log persistence
  (`cr_ui_scan_repair_save_undo`, CLI `--repair-undo`); S1 default-deny without a
  consent callback.
- **B6 damaged-volume** — `M (S-008 FIXED)`: boot-sector repairs (NTFS volume-end
  backup, exFAT backup-12 + checksum recompute + SectorsPerClusterShift rederivation =
  TryFixFromSectorsPerCluster parity, FAT backup-6) PLUS CheckMFT ($MFT record 0
  validated magic+fixup+"$MFT" name, restored from $MFTMirr slot 0 only), CheckFatTable
  (sector-granular torn-copy restore from the signature-valid sibling; S3 on equal
  validity), CheckRootDir/IsValidRootDir/CheckRoot report-only advisories, and
  FixByScanSector (bounded 64 MiB windowed probe, content-deduped candidates, S3 on
  distinct multiples).
- **Transactional discipline** — `M` (exceeds): sector-granular RMW, full-sector undo
  log ("CRUNDO01" + CRC), read-back verify S5, distinct REFUSED vs UNDO_FAILED codes
  (FIX-2), unrestored-evidence preservation (S4).
- **exFAT boot checksum** — `M` on the shared implementation (spec skip-set; og
  predicate undecoded — OQ-T5).
- **S32** — `NA`-note (execute is short; no cancel tokens — fine).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| S-008 | SEVERE | Boot-sector subset + not wired pre-scan (og: 33 methods incl. CheckMFT/FAT-table, pre-scan) — **FIXED** |
| OQ-T5 | tracked-open | ChecksumDBR per-byte predicate |

---

## Session 2 re-audit (audited commit `5d6da5f`)

Two new findings on the RawFixer WRITE path (session 1's S-008 covered the
WIRING gap; session 2 covers the SAFETY + PERF gaps on the write path):

- **F60/F63/R58/S35 — S-010 (SEVERE, safety):** `RwDevice` in
  `cr_transport.cpp:822` opens with `GENERIC_READ|GENERIC_WRITE` +
  `FILE_SHARE_READ|FILE_SHARE_WRITE`, NO `FSCTL_LOCK_VOLUME`,
  NO `FSCTL_DISMOUNT_VOLUME`, NO `DeleteVolumeMountPoint`,
  NO `FlushFileBuffers`, NO `Sleep`-settle, NO `FSCTL_IS_VOLUME_MOUNTED`
  verify. The og's `RecoveryModule.dll` (and the duplicate in
  `PartitionRecovery.dll`) performs the full sequence at
  `RecoveryModule_decompiled.c:44686-44720`:
  `FSCTL_LOCK_VOLUME` (0x90018) → `DeleteVolumeMountPointA` →
  `FlushFileBuffers` → `Sleep(2000)` settle → `FSCTL_IS_VOLUME_MOUNTED`
  (0x90020) verify → `FSCTL_DISMOUNT_VOLUME` (0x9001c). The core's repair
  writes can race with the OS file-system driver (NTFS journaling, AV
  scanners, background indexers, Explorer browsing). The transactional
  undo log mitigates the worst case (rollback on verify-mismatch), but a
  torn write that the read-back verify itself races with could land
  inconsistent bytes — silent volume corruption. The SCAN path is MATCH
  (both og and core open the scan volume with `FILE_SHARE_READ|WRITE|DELETE`).
- **X51 — M-009 (MAJOR, perf):** `RwDevice::read_at` (`cr_transport.cpp:743-749`)
  calls `overlapped_read(h_, off, buf, len, via_rda_)` directly, bypassing
  the `aligned_read_at` envelope (`:148-170`) that GAP-S6-13 mandates for
  every direct read path. The scan-path devices (`DirectDevice:213-219`,
  `RdaTargetDevice:263-269`) both route through the envelope; only
  `RwDevice` skips it. On 4Kn NVMe devices this triggers the documented
  ~3.5× slow path (~830 MB/s vs ~2900 MB/s). The mandate (`cr_directalign.h:1-22`)
  is violated. One-line fix: route through `aligned_read_at`.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-010 | SEVERE | Repair-path volume lock missing (FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME) |
| M-009 | MAJOR | RawFixer read path bypasses the 64 KiB direct-I/O envelope |
| OQ-S2C-3 | open question | Og's pre-scan RawFixer consent UX (auto vs prompted) |

### Continuation-leg addendum (A-003, offset-basis unification)

The continuation remediation leg's independent S-008 verification discovered
**A-003 (ACUTE)**: cmd_fix analyzed through `PartitionSlice` but executed on
the whole-disk device with no offset translation — `fix --part N --apply`
wrote the repair at DISK LBA 0 (over the MBR/GPT), and the S-008 volume-scan
undo logs carried slice-relative offsets that `--undo-apply` replays against
the whole disk (the undo-direction instance). FIXED with one basis mechanism:
`repair::rebase_plan` applied after consent, before execute (orchestrator by
`slice->absolute_base()`; cmd_fix by `part_base`, with the dry-run display
showing disk-absolute targets); the S-008 writer adapters
(`VolumeWriteSlice`/`UiVolumeWriteSlice`) removed (mapping + rebase would
double the offset); `ImageWriteDevice` opens at base 0. Undo logs — and
`fix --undo-apply` — now always carry parent-absolute offsets per the
documented contract. Proof: `RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0`
(disk LBA 0 byte-identical, partition-absolute landing, disk-absolute undo
offsets). Finding + Resolution in `findings/FIXED/A-003-*.md` (renumbered from the local A-002 to clear the collision with the diff-audit-s2 leg's A-002 GUID-regression finding).
