# modules/rawfixer.md — RawFixer audit verdicts

Reference: RawFixer.dll per `PartitionRecovery/rawfixer-fs-repair.md` +
`rawfixer-v2--method-inventory.md` (33 methods, strategy trees, PPL-async, CopySectors
undo, CheckMFT mechanics decompile-verified, ChecksumDBR loop bound verified).

Core (audited `67193b6`): `src/core/repair/cr_rawfixer.cpp` (558 LOC) +
`tests/unit/test_rawfixer.cpp` (13 areas).

## Verdicts

- **F63 consent** — `M`-shape: core is explicitly invoked (CLI command) with a
  transactional plan→execute→undo; og is pre-scan consent-gated — **S-008** (wiring +
  surface subset).
- **B6 damaged-volume** — **S-008**: core repairs boot sectors only (NTFS volume-end
  backup, exFAT backup-12 + checksum recompute + SectorsPerClusterShift rederivation =
  TryFixFromSectorsPerCluster parity, FAT backup-6); og additionally repairs
  MFT-from-mirror (CheckMFT), FAT-table (CheckFatTable), root dirs, and deep DBR
  reconstruction (FixByScanSector).
- **Transactional discipline** — `M` (exceeds): sector-granular RMW, full-sector undo
  log ("CRUNDO01" + CRC), read-back verify S5, distinct REFUSED vs UNDO_FAILED codes
  (FIX-2), unrestored-evidence preservation (S4).
- **exFAT boot checksum** — `M` on the shared implementation (spec skip-set; og
  predicate undecoded — OQ-T5).
- **S32** — `NA`-note (execute is short; no cancel tokens — fine).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| S-008 | SEVERE | Boot-sector subset + not wired pre-scan (og: 33 methods incl. CheckMFT/FAT-table, pre-scan) |
| OQ-T5 | tracked-open | ChecksumDBR per-byte predicate |
