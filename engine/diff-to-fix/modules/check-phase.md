# modules/check-phase.md — the damage-audit check phase (M-041)

**Module:** `check` (`include/cr/check/cr_fscheck.h` + `src/core/check/cr_fscheck.cpp`)
**Corpus ground truth:** `re-workspace/reversed/ghidra-mo/FileSystemCheck_decompiled.c`
(CCheckFAT::Check FUN_1800015b0 :462, CCheckNTFSVolume::Check FUN_1800045a0
:2757, the dir audit FUN_180003360 :1772, the cross-check FUN_180004d80
:3061 — every gate re-verified first-hand at corpus `03ad46c` during the
M-041 fix session)
**Core landing:** DFX30 session 30 (M-041 FIXED; see
`findings/FIXED/M-041-damage-audit-check-phase-missing.md`)

## What the og does

The og runs a chkdsk-grade audit layer over a target volume before/alongside
repair: CCheckFAT validates the DBR against per-type cluster-count ceilings,
scans both FAT copies for dead-zone entries and duplicate references,
compares the copies entry-by-entry, walks the whole directory tree
('.'/'..' pairs, first-cluster validity, file-size-vs-chain, cycles), and
sweeps for orphan clusters; CCheckNTFSVolume proves $MFT/$MFTMirr, gates the
NTFS version (1.2/3.0/3.1), validates $Bitmap, and cross-checks every
allocated MFT record's LCNs against the bitmap (free-but-allocated and
double-allocation). Failures dispatch to RawFixer, then WinChkdsk, and only
volumes that resist both fall through to RAW carving
(rawfixer-fs-repair.md:59-65). Per-error reports (0x1001..0x100e) surface
through an 8-event host callback contract.

## Core implementation (M-041)

- **Error taxonomy, numeric parity:** `check::CHK_*` = the og's own codes
  (0x1001 volume-too-small, 0x1002 duplicate-alloc, 0x1003 orphan /
  free-but-allocated, 0x1004 size!=chain, 0x1005 FAT-copy mismatch, 0x1006
  table invalid (dead-zone / dup-ref / root chain), 0x1007 dir-tree invalid,
  0x1008 $MFT record, 0x1009 $MFTMirr, 0x100A version, 0x100B $Bitmap
  structural, 0x100D DBR, 0x100E exception; 0x100C unused by the og —
  preserved gap).
- **FAT pass, og phase order:** DBR ceilings (341.3333333 / 0x100 /
  (bps×spf32)>>2 with the signature dispatch) -> size floor -> per-copy
  table validation -> pairwise compare (entry-0 media mask; silent stop on
  mismatch — og parity) -> root chain -> dir-tree audit -> orphan sweep
  (host decision; default report).
- **NTFS pass:** $MFT proof (terminal) -> $MFTMirr proof -> version gate
  (param = the version word) -> $Bitmap structural floor -> MFT×$Bitmap
  cross-check gated by $MFT::$BITMAP AND record in-use flags.
- **Termination semantics:** 0x100D/0x1006/0x1005 terminal for FAT;
  0x1008 terminal for NTFS; 0x1009/0x100A continue; dir-audit per-entry.
- **Callbacks:** the og's 8-event surface as optional pointers
  (set_range/set_pos/on_error false=stop/status/path/orphan_decision).
- **Wiring:** optional pre-scan stage AFTER repair, BEFORE the engine pipe
  (`ScanConfig::pre_scan_check`, default OFF); NEVER aborts the scan.
  Surfaces: `ScanStats.check_*`, `event::Type::FsCheck` (wire 10),
  `REC_FS_CHECK` session record, the cr_ui ABI
  (`cr_ui_scan_config.pre_scan_check`, stats `check_*`,
  `CR_EVT_FS_CHECK`), CLI `scan --check`.

## Parameter verdicts

| Parameter | Verdict | Notes |
|---|---|---|
| P1 recovery-correctness context | MATCH | the audit layer exists; damage assessment feeds repair dispatch (escalation ladder documented) |
| P3 false-positive surface (cross-link/orphan) | MATCH | 0x1002/0x1003/0x1006 decoded and implemented |
| P5 orphan reconstruction (orphan-cluster scan) | MATCH | the sweep + host decision (FUN_180002770) |
| P15 confidence grading input | MATCH | the report is surfaced to hosts; the S-023 grading consumption is host-side as in the og |
| P25 per-check validators | MATCH | every threshold decompile-cited in the header |
| P35 error recovery ladder | MATCH (check phase) | audit informs, never gates; boot-repair/WinChkdsk endpoints = M-040 (open) |
| P42 public surface | MATCH | core config/stats/event + ABI + CLI `--check` |

## Registered deliberate deltas (PARITY-DELTAS D-87)

1. The og's $Bitmap slack term (checkntfs.cpp :3869-3871,
   CCheckNTFSVolume+0x30) is layout-undecoded — only the firing half
   (bitmap bits < MFT record count) is implemented; no constant guessed.
2. `CheckIssue` carries the FAT 0x1003 cluster + the claiming record number
   as superset context (the og's FAT 0x1003 passes param 0; NTFS codes
   carry the LCN param as decoded).
3. A partial $Bitmap run load fails the load (the 0x100B path) instead of
   auditing against a zero-filled tail (the og's LoadVolumeBitmap failure
   shape).

## Open items

- M-040 (the escalation ladder's boot-chain endpoints: BCD patch, NVRAM,
  post-op chkdsk) remains open on its own row.
- The DRWUI error→strategy mapping (CCheckFatPartition/CCheckNtfsPartition)
  stays out of core scope (UI-layer classes; the finding's Open question).
