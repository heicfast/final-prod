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

---

## Session-3 parallel-leg audit (integration 2026-10-26; baseline `7fd00e0`)

The DiskHealth **grading layer** decode (previously UNVERIFIED) and the NVMe
route verification. Evidence: `evidence/decompiled-quotes/s3-filefilter-health.md`
Part B. (This leg's local IDs S-017/M-020/I-013 renumbered to S-024/M-036/I-018
during reconciliation.)

| ID | Severity | One-line |
|---|---|---|
| S-024 | SEVERE — **FIXED** (FIXED/S-024) | No grading layer: og Unknown/Good/Caution/Bad with vendor thresholds (SMART 0xD1) + SSD-wear vendor mapping + caution rules (5/197/198 raw>=1, SSD life <=10%, temp 50/60); ours a single failing bit |
| M-036 | MAJOR | NVMe: host r/w + controller-busy + error-log fields dropped; data units unconverted (og x512,000); folds the N-009a percentage_used width fix |
| I-018 | INFO | NVMe transport route verified byte-parity MATCH (IOCTL 0x2D1400 + {proto 3, type 2, page 2, 512B}) — upgrades AUDIT-PIPELINE's "corpus silent" row |

## Session-5 audit (audited commit `7cf6754`)

- **N-055 (MINOR, NEW — disposition correction):** ATA SMART thresholds (READ ATTRIBUTE
  THRESHOLDS 0xD1) are never read; the og's engine reads them and its UI composes
  threshold-based verdicts (Threshold column, firstCautionIndex — UIUX 19 + the
  diskhealth companion artifact). This session's correction: the prior "presentation-layer
  feature" verdict on the threshold semantics was WRONG — the read is an engine-side data
  gap. NVMe unaffected (`available_spare_threshold` is read, cr_nvme.cpp:30).
- **N-064 (MINOR, NEW):** the og's Dict password dictionary (228 entries; manage-bde
  -Password consent-gated attempts) has no core counterpart — capability register, any
  port must keep the consent gate (parameter F61).

## Session-6 fix leg (2026-10-26 — the S-024 decoded-truth upgrade)

S-024's engine re-decoded first-hand from DiskHealth_decompiled.c and the
first landing's analogs replaced with the exact rules (evidence:
`evidence/decompiled-quotes/s6-health-grading-decode.md`):

| Parameter | Verdict |
|---|---|
| B15 grading model | **MATCH** — FUN_0040cdf0 exact: 4 statuses; Unknown gates (data present / ATA side / read error); duplicate-attr-id → Unknown; SSD any-attr-below-threshold vs HDD in-range table (1-13/187-189/191-193/195-209/211-212/220-228/230-231/240/250/254); 5/197/198 raw16 caution with the all-FF raw[0..3] guard (disk-side only); bad-count > caution-flag > graded-signal > Unknown aggregation; family-0x13 NVMe path (critical_warning byte ≠ 0 → Bad; life > 10 Good / == 10 Caution / < 10 Bad; Parallels early-Good); temp NEVER graded (0x1808 unread by the grader — the first landing's temp-Caution rule removed) |
| D16-vendor classification | **MATCH (decoded set)** — model_says_ssd (the 0x1724 writer list) + classify_family (FUN_0040dad0 cascade: WDC/Seagate/Mtron/JMicron60x/Intel/Samsung/Micron/Crucial/SandForce/OCZ/Vector/Plextor/SanDisk/SKhynix/Kingston/Kioxia/Phison/Apacer + NVMe + fallbacks) with sub-family codes, the raw/invert life forms, the SanDisk G1001 gate and the attr-230 raw16/100 form; exotic-thunk residual = classifier data gap (engine consumes all codes) |
| C23 thresholds | **MATCH** — 0xD1 read verified against the og's internal mirror (:14108-14118) |
| Q65 tests | 20 Health tests incl. the family-gate matrix, the FF-guard asymmetry, duplicate-Unknown, the NVMe exact bits, temp-never-grades, is-SSD list, classifier vendors/sigs/forms |
| F-gate deliberate superset | failing_bit → disk Bad registered as **D-60** (PARITY-DELTAS); per-attr grades stay og-exact |

M-036 (NVMe field coverage) and N-064 (Dict dictionary) remain open
(untouched by this leg). The S-027 lane's Ext4Engine.Backup* reds visible
on main mid-session are that lane's in-flight state, not health.

