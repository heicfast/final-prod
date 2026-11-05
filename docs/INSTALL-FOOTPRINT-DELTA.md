# INSTALL-FOOTPRINT-DELTA — our installed footprint vs the reference's, every artifact categorized

The mandate (§5): the reference installs far more than we do — verify,
per artifact, whether that difference is (a) legitimate consolidation,
(b) a Missing defect, (c) a Not-applicable feature we never claimed, or
(d) a reference-only artifact with no analog. Sources:
`docs/INSTALL-FOOTPRINT-CURRENT.md` and `docs/REFERENCE-FOOTPRINT.md` /
`docs/REFERENCE-MODULES.md` (the reference: 1,470 files / ~670 MiB /
~114 modules, every count corpus-cited).

> **Status correction (2026-11-05, consolidation session).** The kernel
> driver is REMOVED from our product (commit 318b9655) — our footprint is
> now the flat 8-file install dir + ProgramData, with no service, no
> driver, no certificate stores. The driver rows below that describe OUR
> side are marked historical; the reference-side rows (its own drivers,
> WinPE copy, etc.) are facts about the reference and stand unchanged.

**Headline answer to the user's hypothesis** ("we install 20 files where
the reference installs 400 — are we missing 380 files' worth of
functionality?"): **No — the functional core is present and consolidated;
the numeric gap is dominated by features this product has never claimed**
(video repair suite, Office/PDF viewers, WinPE boot media, cloud/NAS,
downloader/updater, 37 language packs, QA leftovers). Every module the
product's CLAIMED surfaces need is shipped inside `cr_ui.dll` (the FS
engines, carving, grading, sessions, repair, health) or beside it. The
genuinely-missing claimed capabilities are exactly two, both already
recorded and truthfully surfaced (below).

## A. Matched — same function, same class of location

| Reference artifact | Ours | Evidence |
|---|---|---|
| `DRWUI.exe` (main orchestrator UI) | `recovery-wizard-ui.exe` (Tauri shell + bridge) | CURRENT §A |
| `eprdtdrv.sys` raw-disk driver, SCM service (CreateService, group "Boot File System", demand) | **No analog — driver removed** (consolidation session, commit 318b9655: user-mode direct raw-disk transport, `ERR_ELEVATION_REQUIRED` posture; historically 0.6.x shipped `rda.sys`, service `RDA`, identical SCM profile — CURRENT §A.2, historical) | CURRENT §A; REFERENCE-FOOTPRINT §services |
| The reference's session/persist format (`.rsf`) | our `.crsf` sessions in `ProgramData\HeicFast\Recovery Wizard\sessions` | CURRENT §A |
| `DRWReport.exe` (recovery report) | in-app report (`cr_ui_recovery_report_json`) | CURRENT §A |
| Uninstall entry + uninstaller | `HeicFastUninstall.exe` + the Add/Remove entry | CURRENT §A |

## B. Matched by consolidation — the reference ships N files, we ship one module

| Reference files | Our consolidation | What the reference splits that we compile in |
|---|---|---|
| `NTFSSearchDLL.dll` | `cr_ui.dll` | NTFS MFT walk + INDX second source + remnant sweep |
| `FAT32SearchDLL.dll`, `EXFATSearchDLL.dll` | `cr_ui.dll` | FAT/exFAT quick engines, deleted-chain reconstruction |
| `EXT4SearchDLL.dll`, `BtrfsSearchDLL.dll`, `ReFSSearchDLL`, `HFS+`, `ZFS` modules | `cr_ui.dll` | the same FS engine families (926-test suite) |
| `RAWSearchDLL.dll` + `raw_signs.json` + `FileFilter.xml` | `cr_ui.dll` (signature + carve engine; the signature table in `cr_signatures`/`cr_rawsigns`, the extension→category mirror generated into the bridge) | carving, classification, categorization |
| `PartitionRecovery` MOM set (Device/DeviceManager/PartitionRecoveryD `.mo` modules) | `cr_ui.dll` (lost-partition discovery + LDM + apply, the `lost`/`table`/`apply` modules) | lost partitions, dynamic disks, partition-table apply |
| `EPMLOG.log`/`Logger.dll` three-layer logging | the bridge's `app.log` + the engine log seam (NG-15) + the installer's `install.log` | diagnostics |
| `DiskHealth.exe` companion app | the in-app Health page + the engine's health layer (multi-transport) | the SMART surface |
| `rrecov`-equivalent CLI surface (reference has none public) | `rrecov.exe` (scan/tree/recover/health/fix/apply/bench) | — |

These consolidations are deliberate (one ABI, one DLL to load, the
seam-count the session exists to fix). Each reference file's FUNCTION is
present; the count difference is architecture, not absence.

## C. Missing — the reference ships it, we claim the capability, we do not ship it

Exactly two, both already honestly surfaced in the product and recorded:

| Reference module | Our status | Surface behavior |
|---|---|---|
| `EUFileRepair` suite (`EUImgRepair.exe`, `EUVideoRepair.dll`, `EUArchiveRepair.exe`, `EUOfficeViewer.exe`, …) — the standalone file-repair workbench | **Not shipped** — G-1 (b): the ABI trio (`cr_ui_repair_analyze/apply/undo`) is the recorded follow-up | The Repair page states the workbench is unavailable and describes the REAL in-product repair (the pre-scan consent flow, wired end to end) |
| The reference's in-UI partition-apply (PartitionRecoveryD's write path surfaced in its UI) | **CLI-shipped** — `rrecov apply` beside the app (S-029 engine + CLI complete); the bridge/UI flow is the follow-up | The Lost-Partition page launches the REAL whole-disk search and states the table-restore ships via `rrecov apply` |

Nothing else the product CLAIMS is missing from the installed footprint.

## D. Not applicable — capabilities this product has never claimed

The bulk of the reference's 1,470 files. Documented exclusions (the
product makes no claim on any of these surfaces, so no surface exists to
disable):

- **WinPE boot media** (`BUILDPE/` — 194 files, 180-file app copy + PE OS
  overlay + 4 boot drivers): the product is a live-Windows product.
- **The 6 non-RDA kernel drivers** (WinPE hooks, the backup filter, the
  network redirector `EuMrx` + `EuShare`, NAS): the WinPE/backup/NAS
  features they serve are not claimed.
- **Cloud/NAS user-mode stack** (`AliyunWrap*`, `NasModel.xml`, the
  downloader `EuDownloader.exe`): no cloud surface exists in the UI.
- **Viewers/players** (`VideoViewer/` — 104 MB, `EUHwpViewer`, Adobe/Office
  repair chains): previews ship as real bytes + metadata with truthful
  "preview not rendered" hints (G-4); rendering is a feature, not a wiring
  gap.
- **37 language `.qm` packs**: the product ships EN + zh-CN.
- **QA/telemetry leftovers** (`EPMLOG.log`, `Eaolog.log`, `Dict` staging
  residue in the capture): reference-only artifacts (see E).

## E. Reference-only artifacts — exist for the reference's own internal reasons

The installer's staging metadata, update-server configuration
(`dlconfig.xml`, `DataFile.ini`), license/activation plumbing
(`EUinApp.exe`), crash/diag residue (`Eaolog.log`), and the QA-run
leftovers the corpus capture contains. No analog intended; not mimicked.

## The honest numeric reconciliation

| Reference total | ~1,470 files |
|---|---|
| − WinPE copy (194) and boot drivers (4) | not claimed |
| − viewers/repair-suite/office/cloud/NAS/downloader families (~800+) | not claimed |
| − language packs (37) | EN/zh-CN only |
| − QA/log/staging residue | reference-only |
| = functional core the reference ships for the CLAIMED surface | ~40 module-equivalents |
| of which we consolidate into | 1 `cr_ui.dll` + the app + the CLI (**no driver** — removed, commit 318b9655) |

Our installed set (post-consolidation: 8 flat files + ProgramData +
registry; historically 15 files + service + driver) covers the claimed
surface. The delta is architecture and scope, not absence — and the two
real gaps are disabled truthfully in the UI with their follow-ups
recorded in `docs/INTEGRATION-GAPS.md` (G-1 (b), the apply-UI note in
NG-11).
