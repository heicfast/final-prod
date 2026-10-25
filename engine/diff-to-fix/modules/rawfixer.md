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
