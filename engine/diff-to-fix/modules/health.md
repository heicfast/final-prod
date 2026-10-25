# modules/health.md — disk health audit verdicts

Reference: og DiskHealth subsystem (og-codebase/diskhealth/ + artifacts/DiskHealth/) —
SMART attribute surfacing with UI presentation; ATA passthrough + legacy ioctl paths.

Core (audited `67193b6`): `src/core/health/cr_nvme.cpp` (113 LOC parser) +
`src/win/cr_health.cpp` (ATA passthrough IOCTL_ATA_PASS_THROUGH + SMART_RCV_DRIVE_DATA
0x7C088 legacy fallback — the corpus-carried pair) + `include/cr/win/cr_health.h` +
`tests/unit/test_nvme_health.cpp` (3 areas — parser only).

## Verdicts

- **B1 detection** — `M`: NVMe Identify/Health-log field extraction; ATA SMART
  512-byte table (12-byte entries); transport labeling (ata-passthrough / nvme /
  legacy). Og surface includes threshold/PREDICT FAILURE semantics + attribute naming —
  presentation-layer features beyond the engine's data contract; core's data layer is
  field-equivalent for the raw attributes.
- **C20 field widths** — **N-009a** (percentage_used read as u16; spec u8).
- **Q65 tests** — thin (parser-only; the win-layer passthrough paths untested — test
  blind-spot family shared with all win/ code).
- **F59/F60** — `NA` (health is advisory; no failure policy divergence).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| N-009a | MINOR | NVMe percentage_used u16 vs spec u8 |

## Test debt

Win-layer passthrough paths; attribute-row decode vectors.

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new health-specific findings in session 2. The N-009a finding (NVMe
percentage_used u16 vs spec u8) stands. Health module is otherwise MATCH
per session 1's verdicts.

### Session 2 findings index (this module)

(No new findings in session 2.)
## Session-2 update (2026-10-25, core `e93decf`)

- **F61 (BitLocker) — never scored by session 1 — now audited: S-014 (SEVERE)** (wrong live
  registry path, no offline-hive path, unassociated enumeration). The module's F61 row moves
  from unscored to S-014.
- **F59/F60 re-verified MATCH** by the coverage sweep: driver-absent duality (EPRDTDRV probe →
  direct PhysicalDrive fallback), PID token, 200-disk cap, IOCTL table; DiskHealth semantics
  covered by the retry-layer error-map model.
- N-009a unchanged.

## Session-4 remediation update

**S-014 FIXED** (s4 session): the F61 verdict above is now `MATCH` on both halves —
(1) the live auto-unlock path is `SYSTEM\CurrentControlSet\Control\FVEAutoUnlock`
(DRWService_decompiled.c:104) with per-volume-GUID subkeys' `Data` REG_BINARY blobs,
the og's caps applied (50-char GUID / 256-byte blob / 100 records) and the target
volume's GUID matched first (`GetVolumeNameForVolumeMountPointW` association);
(2) the offline (dead-system) path exists: a new portable read-only regf parser
(`cr/reghive`, every structural offset decoded from the og's own
`RegLib_decompiled.c`) reads `<X>:\Windows\System32\config\SYSTEM` — the og's
bitlockerhelp flow (:3128-3160, hardcoded `\ControlSet001\Control\FVEAutoUnlock`) —
both through an explicit `try_auto_unlock_offline(letter, hive_path)` and an
automatic probe of every other mounted volume inside `try_auto_unlock`. The blind
all-binary-values enumeration is gone. 11 Linux tests + 5 Windows-lane tests +
`fuzz/fuzz_reghive.cpp`; Win7 gate additions documented (RegQueryValueExW,
GetVolumeNameForVolumeMountPointW). See `findings/FIXED/S-014-*.md`.
