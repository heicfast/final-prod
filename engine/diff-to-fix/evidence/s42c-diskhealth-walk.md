> **S42-C pin-time + renumber note (added at the third reconciliation):** this
> sheet records the tree state as verified at the lane's pin **`869da779`** (core) /
> **`be0cfe9`** (corpus). THREE hot windows landed while this lane worked; the
> sheet's internal draft IDs are superseded as follows: this lane's "M-118"/"M-121"
> (the Stbio large-IDAT finding) → final **M-122**; "I-035"/"I-037" (the diskhealth
> register) → final **I-038**; "I-036"/"I-038" (the corpus-worklog register) → final
> **I-039**; the red-main draft "N-154" → folded as the N-157 corroboration addendum.
> The remote lanes' same-numbered findings (the S12 MountPoints2 M-118, the DBX
> chain's M-121, the decode register I-037, the S42 lane's config-layer I-036) are
> different subjects. The sheet's liveness verdicts for findings the windows have
> since FIXED (S-048, M-117/M-121-dbx, N-152, N-125, N-126, N-135, N-148,
> S-054-the-N-013-regression, N-149, M-076, the INFO wave, N-115's N-158 tail
> aside) are pin-time-historical (true at the pin, superseded at the reconciled
> tree). The landing-parity VERIFIED-PARITY verdicts are unaffected. This lane is
> the S42-C lane (rebranded twice: the remote's S42 and S42-B lanes pushed first).

# W3a — og-codebase/diskhealth/ deep walk (S42 continuation lane)

**Task ID:** S42/W3a · **Sub-agent:** read-only audit walk · **Date:** 2026-10-29
**Baselines:** core `869da779` (HEAD d10b893a is trail-only above it) · corpus `be0cfe9`
**Scratch artifacts:** `dh-strings-ascii.txt` (6,253 lines), `dh-strings-u16.txt` (1,111 lines) — full
string dumps of DiskHealth.exe, both encodings, in this scratch dir.

---

## 1. Inventory — what this tree IS

`og-codebase/diskhealth/` = **55 MB, 818 files, 39 directories** — a complete, self-contained
deployment of a **separate companion product**: the "EaseUS DiskHealth" SMART monitoring app
(internal product name **DiskSmart**, per the `disksmart.ico`/`ico_logo_pallets_disksmart16.png`
resources and window title "EaseUS DiskHealth"). It is a **32-bit (machine 0x14c) Qt 5.15.2
Quick/QML GUI app** (Qt version string verified in Qt5Core.dll; corpus architecture/01:75 agrees
"Qt 5.15.2 official binaries … only for DiskHealth"), built from the DRW monorepo
(PDB path string: `F:\drw_backup\Output\Release_Win32\DiskHealth.pdb` — matches
architecture/01-build-tree-intelligence.md:66).

### 1.1 Extension / family census (818 files)

| Family | Count | Nature |
|---|---|---|
| `DiskHealth.exe` | 1 | the product binary (1,781,480 B; PE x86 GUI; 4,636 fns / 93,316 decompiled lines; 0 exports) |
| `DiskHealth.rcc` | 1 | compiled Qt resource bundle: 125 files (10 app qml + 65 res + 50 publicQML — corpus worklog:104 claim "125 files incl. 54 QML components" verified: 10 app + 44 shared QML = 54) |
| Qt5 runtime DLLs (top level) | 13 | Qt5Core/Gui/Network/Qml/QmlModels/QmlWorkerScript/Quick/QuickControls2/QuickShapes/QuickTemplates2/RemoteObjects/Svg/Widgets (Qt 5.15.2 stock) |
| VC/CRT runtime | 5 | mfc140u, msvcp140(+_1), ucrtbase, vcruntime140 |
| api-ms-win-* forwarders | 40 | UCRT API-set forwarder stubs (25 core + 15 crt) |
| Qt plugin DLLs | 25 | imageformats(9), qmltooling(11 debug — shipped!), platforms/qwindows(1), styles(1), iconengines(1), scenegraph qsgd3d12backend(1), bearer qgenericbearer(1) |
| QML/Quick runtime trees | 615 | 440 `.qml` + 146 `.qmlc` + 20 `.qmltypes` + 3 `.jsc` + 3 `.js` + 3 `.js`-adjacent — all stock Qt Quick/Controls/GraphicalEffects (dirs: QtQuick, QtQuick.2, QtQml, QtGraphicalEffects, Qt) + 25 `qmldir` |
| Product translations | 17 | `diskhealth_{Arabic,ChineseSimp,ChineseTrad,Danish,Dutch,English,French,German,Italian,Japanese,Korean,Mainland,Polish,Portuguese,Russian,Spanish,Thai}.qm` |
| Qt runtime translations | 23 | `qt_*.qm` |
| PNG | 31 | ALL stock QtQuick Controls style images (`QtQuick/Controls/Styles/Base/images/*`) |
| Batch scripts | 3 | `addtask.bat` (82 B), `deltask.bat` (51 B), `querytask.bat` (47 B) |
| Config | 1 | `AliyunConfig.ini` (28 B: `[Config] bAllowSendInfo=0`) |

**Not present in the tree:** DiskHealthAuto.exe (referenced in the binary's string table at file
offset 0x8b73c — see §5.6), any `aliyun*.*` DLL (the boot-USB copy script copies `aliyun*.*` next
to it — WinPE/boot-config--winpeshl-autobat-regdrv.md:74,81), any product .png outside the .rcc.

### 1.2 What the corpus already says about it (all verified against the tree)

- `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md` — CrystalDiskInfo-lineage
  SMART companion (CAtaSmart/DiskInfoMgr/RefreshThread/CTrayIcon/AttrModel vftable census),
  boot-USB deployment (auto.bat Phase E), notification-IPC pairing with DRW.
- `re-workspace/artifacts/per-binary/DiskHealth.md` — identity table (SHA256 short a4f1b5b1ff1c63ad,
  529 imports/27 DLLs, 0 exports, ghidra-wave2 4,636 fns/93,316 lines).
- `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` (93,316 lines) + `_functions.txt`
  + `_imports.txt` + `_blocks.txt` (.text = 498 KB @0x401000, .rsrc 512 KB, .rdata 670 KB).
- `re-workspace/reversed/ui-resources/diskhealth/` — the .rcc extracted: qml/ (10 app QML),
  publicQML/ (50 shared EuS components), res/ (65 assets), _manifest.json.
- `re-workspace/artifacts/UIUX/19-diskhealth-quick-ui.md` — 840×590 frameless window, 32-string
  census, tray-guarding model. `runtime-families/qt5-quick-qml-stack.md` — the QML stack is
  DiskHealth-only (DRWUI is widgets).
- corpus worklog.md:104/115/202/435/466/494 — rcc extraction (125 files), wave2 CI ingestion,
  UI artifact 19, CrystalDiskInfo-lineage family artifact.

**Verdict on identity:** a separate companion product's *installed deployment* (bin + Qt runtime
+ QML UI bundle + translations + autostart scripts + telemetry opt-out flag), NOT a data/log store,
NOT a DRW plugin set. The DRW-side product integration is: notification-launch
(`Click_Open_DiskHealth_By_Notification`), boot-USB side deployment, and the shared
`diskHealthIndex=` CLI arg contract (see §3.5).

---

## 2. Method

1. Full tree inventory (find/du/extension census, PE header reads, stock-Qt verification).
2. `strings` both encodings → scratch dumps; every product-defining string class traced into
   `DiskHealth_decompiled.c` with Read-with-offset (never whole-file).
3. Corpus docs read: the DiskHealth artifact, per-binary census, UIUX/19, qt5-quick-qml-stack,
   WinPE boot-config §2E, architecture greps (diskhealth/SMART/CSMI/NVMe), corpus worklog greps.
4. Audit trail read: `diff-to-fix/modules/health.md` (full, all 8 sessions), SURFACE-INVENTORY:81,
   API-DELTA:75, FIXED-LEDGER, CROSS-REF-MATRIX health rows, `docs/PARITY-DELTAS.md` D-82, D-63,
   `docs/PARITY-OG-CORE-INVENTORY.md` (the UI/RUNTIME/INSTALLER/REPORTING scope lines),
   `docs/REFERENCE-API-GAP.md` R-60/61/62, PLAN-API-EXPANSION:88, `findings/INFORMATIONAL/I-027`
   (full), `findings/MAJOR/M-076`, evidence `s6-health-grading-decode.md` (head), `s3-filefilter-health.md`.
5. Core health implementation read in full at `869da779`:
   `src/core/health/{cr_grade,cr_family,cr_smart_ata,cr_csmi,cr_nvme}.cpp` (1,039 LOC),
   `include/cr/health/*` (456 LOC), `src/win/cr_health.cpp` (346 LOC),
   `include/cr/win/cr_health.h` (104 LOC), tests `test_health_grade.cpp` (21 TESTs),
   `test_health_csmi.cpp` (9), `test_nvme_health.cpp` (8) — 929 LOC of tests.

---

## 3. Capability map of DiskHealth.exe (strings + decompile, first-hand)

| # | Capability | First-hand corpus evidence |
|---|---|---|
| C1 | SMART engine (CrystalDiskInfo `CAtaSmart` lineage), 30×12 attr table, 0xD0/0xD1/0xEC, PREDICT FAILURE/RETURN STATUS (0xD8) | u16 strings `SendAtaCommandPd - IDENTIFY_DEVICE/SMART_READ_DATA/SMART_READ_THRESHOLDS/SMART_CONTROL_STATUS (ATA_PASS_THROUGH)` + legacy twins; s6 decode :14108-14118; core cr_smart_ata.cpp:98-129 issues 0xEC+0xD0+0xD1 |
| C2 | 4-status grading engine (Unknown/Good/Caution/Bad) | `{"Unknown","Good","Caution","Bad"}` fromAscii ×4 in strings; FUN_0040cdf0 :5755-6130 (s6 decode, modules/health.md:107) |
| C3 | Vendor family classification + SSD model gates | fromAscii census: SmartNVMe etc.; cr_family.cpp decoded cascade; SAMSUNG HD155UI/HD204UI+1AQ10003, OCZ-VERTEX3+2.02 (:4441-4449), ADATA SSD, FuzeDrive/StoreMI quirks in u16 strings |
| C4 | Transport chain: Pd(-1/-1A/-2/-2A), Si, Sat(-1/-1A/-2/-2A), Csmi(-1/-1A), CSMI_PHYSICAL_DRIVE, LSILOGIC sibling | u16 strings :558-574 (scratch dump); CSMI wire = FUN_00425450 (D-82); FUN_00426040 LSILOGIC (SMART-enable task file :4420-4426) |
| C5 | NVMe: 7 CMD_TYPE routes | `CMD_TYPE_NVME_{STORAGE_QUERY,INTEL_RST,SAMSUNG,INTEL,JMICRON,ASMEDIA,REALTEK}` (u16 strings :715-731 scratch dump); STORAGE_QUERY route verified in code (0x2D1400 DeviceIoControl :8917/:10804/:13362/:16257; I-018 byte-parity) |
| C6 | USB-bridge quirk table | `AddDisk - Si` :10761; `AddDisk - USB0A…USB20` (28 log strings, :11043-11163+); USB VID/PID quirks (LOGITEC 0x00D9, Genesys 0x05E3/0x0702, IO-DATA 0x0122, JMicron) in strings |
| C7 | NVMe health-log fields incl. data-units ×512,000, K-273 temp, 100−percentage_used life | :5579-5595 (×0x7D000 first-hand), :26989 (K-273), :5584 (100−pct); M-036 cite verified first-hand |
| C8 | RAID census routing (CSMI GET_DRIVER/PHY/RAID_INFO/RAID_CONFIG) | `FAILED: CC_CSMI_SAS_GET_{DRIVER_INFO,PHY_INFO,RAID_CONFIG,RAID_INFO}` :4620/:4657/:4818/:4828 |
| C9 | Device writes: AAM enable/disable (0xEF/0x42, 0xEF/0xC2), APM enable/disable (0xEF/0x05, 0xEF/0x85), SMART auto-enable (0xB0/0xD8) | :9565/:8058 (AAM), :9575/:8068 (APM), :4420-4426 (SMART enable) — **AAM/APM setters never dispositioned before this walk** (§5.3) |
| C10 | Disk enumeration: PhysicalDrive probe ladder, WMI `SELECT * FROM Win32_DiskDrive` cross-check, drive-letter→disk mapping via IOCTL 0x560000 (VOLUME_DISK_EXTENTS) | u16 strings :516-545 scratch dump; :16703/:18212/:20683/:22780/:33338 (0x560000 call sites) |
| C11 | WMI service management: `sc config Winmgmt start= auto` before WMI queries | :9508 `ShellExecuteW(...,L"sc",L"config Winmgmt start= auto",...)` — **never dispositioned** (§5.4) |
| C12 | Tray + toast notification companion: Shell_NotifyIconW tray (:30609/:30684/:31115), WTSRegisterSessionNotification, toast payload `{"Disk":"disk %1"}` + tr(`Info_ShowNotification`) + activation strings | :37253-37266, :39309-39311 (`Click_Open_DiskHealth_By_Notification`), :37714-37716 (`Click_Open_Browser_By_Notification`) |
| C13 | Single-instance + CLI IPC | mutex `EaseUS_DiskHealth{,Console}` :72-79 + EnumWindows; args `diskHealthIndex=` (:233-254, :37772-37797), `isPe=` (:37802), `auto` (:37886-37889), `EPM` (:37892-37897), `themeName=` (:37841-37856) |
| C14 | Settings: QSettings root `HKEY_LOCAL_MACHINE\SOFTWARE\EaseUS\DiskHealth` — `ShowIcon` (read :32239-32243, write 1 :35201-35205 / 0 :34497-34501), `TempF` (:32206-32212, :39144), `Language` (:36740/:36777/:38927); app-dir `config.ini [main/realStyleName]` theme fallback (:37866-37884) | all first-hand this walk |
| C15 | Autostart persistence via the bat trio (`schtasks /create /sc ONLOGON /rl HIGHEST` + delete/query) | :33735-33743 addtask call, :34463-34471 deltask, :36824-36832 querytask; string `/DiskHealthAuto.exe` @0x8b73c adjacent (§5.6) |
| C16 | Aliyun telemetry (own client, opt-out default): reads/writes `AliyunConfig.ini [Config] bAllowSendInfo` | :39732 (GetPrivateProfileIntW), :39762 (WritePrivateProfileStringW); event taxonomy `Info_Start`, `Result_Startup_BGS`, `Result_Stop_BGS`, `Result_LoadDiskInfo`, `Click_Refresh`, `Click_Done`, `Click_SysIcon_Menu`, `Info_SysIcon_Hover` (:32311/:32331/:35114/:35161/:35261/:37214/:38453) + `easeusSendType` u16 |
| C17 | QML dashboard UI (840×590 frameless, SMART table with ID/Attribute/Current/Threshold/RawValue/ST columns, per-row grade colors 1=green/2=amber/3=red, caution popup, temperature card w/ °C↔°F toggle, 14-locale language menu with Mainland lock, buy/license components) | ui-resources/diskhealth/qml/{main,CautionWidget,InfoList,InfoItem,Title,MultiLanguage,HoverWidget,Theme,RotatingWait}.qml — all read this walk |
| C18 | Health-solutions help links, 9 locales (hard-drive-failure pages) | strings: br/es/it/jp/tw/kr/com/de/fr easeus URLs; consumed by `getPopLink(languageShort)` (CautionWidget.qml:321) |
| C19 | Vendor SMART attribute display-name tables (~40 vendor model families: SmartApacer…SmartNVMe incl. Kingston DC500/KC600/SA400/Suv, Micron MU02/MU03, JMicron60x/61x/66x…) + ATA spec-version strings (ATA/ATAPI-4 … ATA8-ACS) + feature-string builder "S.M.A.R.T., AAM, APM, NCQ, TRIM, DevSleep, VolatileWriteCache" | ASCII strings dump; DISPLAY-layer |
| C20 | No sound-alarm/mail-report/export capabilities (CrystalDiskInfo's alarm/mail features stripped in this skin) | grep-negative: no .wav/PlaySound/mail/smtp/export strings in either dump |

---

## 4. Per-surface disposition table

Legend: **COVERED** (core matches, cites), **NOT-CORE-SCOPE** (with the trail-convention reason),
**DIVERGENCE**, **NEVER-DISPOSITIONED** (candidate filing), **UNVERIFIED**.

| Surface (og evidence) | Verdict | Core / trail cite |
|---|---|---|
| S1. SMART read data model (30×12 @+2, value/worst/raw6, attrs 5/9/10/190/194/197/198/231/233) — C1 | **COVERED** | cr_smart_ata.cpp:131-184 (parse+derived); cr_health.h:45-75 (SmartReport); S-024/N-055 history in modules/health.md:82-121 |
| S2. Vendor thresholds 0xD1 read | **COVERED** | cr_smart_ata.cpp:126-129+144-147; N-055 FIXED (modules/health.md:88-93; FIXED-LEDGER:238) |
| S3. 4-status grading (disk + per-attr rule chain, FF-guard, dup-Unknown, temp-never-graded) | **COVERED** | cr_grade.cpp:48-338 (rule-by-rule vs FUN_0040cdf0/:34148-34167); cr_grade.h:5-35 citation block; 21 tests (test_health_grade.cpp) |
| S4. Vendor family classification + is-SSD string gates | **COVERED** (decoded set; exotic-vendor residual documented) | cr_family.cpp:11-18 coverage-honesty block, :78+ cascade; tests :366-443; S-024 Resolution in modules/health.md:108 |
| S5. CSMI CC-0x19 transport (Intel RST) incl. IOCTL_SCSI_GET_ADDRESS routing | **COVERED** (remainders registered) | cr_csmi.cpp:37-76 + include/cr/health/cr_csmi.h; win/cr_health.cpp:143-208; N-032 FIXED (s30); D-82 registration docs/PARITY-DELTAS.md:2614-2648 |
| S6. CSMI RAID census + LSILOGIC sibling + SMART auto-enable (device writes) | **NOT-CORE-SCOPE (REGISTERED DIVERGENCE)** — census = companion device-manager machinery; auto-enable = device write, report-only policy | D-82 (docs/PARITY-DELTAS.md:2640-2648); og cite :4620/:4657/:4818/:4828, FUN_00426040, :4420-4426 |
| S7. NVMe STORAGE_QUERY route (0x2D1400, proto 3/type 2/page 2/512B) | **COVERED** | win/cr_health.cpp:264-299; I-018 (modules/health.md:84); og sites :8917/:10804/:13362/:16257 |
| S8. NVMe health fields + ×512,000 units + K-273 + u8 percentage | **COVERED** | cr_nvme.cpp:23-51; M-036/N-009a/N-042 all FIXED (modules/health.md:145-165); og :5579-5595 verified first-hand this walk |
| S9. NVMe grade (family-0x13 path, Parallels early-Good, critical_warning→Bad, life 10 thresholds) | **COVERED** | cr_grade.cpp:359-375; test_health_grade.cpp:327 |
| S10. NVMe vendor routes CMD_TYPE_{INTEL_RST,SAMSUNG,INTEL,JMICRON,ASMEDIA,REALTEK} | **NEVER-DISPOSITIONED** (§5.1) | string-table evidence only (u16 dump :715-731); no trail row (grep-negative across diff-to-fix/ + docs/) |
| S11. USB-bridge AddDisk quirk table (Si, ASM1352R, USB0A-USB20) | **NEVER-DISPOSITIONED** (§5.2) | :10761, :11043-11163+; N-032's title names "usb-bridges" but D-82's registration text does not; grep-negative |
| S12. SAT/Pd stamp variants (-1A/-2/-2A) | **NOT-CORE-SCOPE (subsumed)** — the chain contract is "first channel answering IDENTIFY serves the set" (N-032 model, cr_smart_ata.cpp:98-116); the retry-stamp variants are companion-side accessor fallbacks | u16 dump :558-574; modules/health.md:156-159; UNVERIFIED nuance: whether -1A/-2/-2A differ beyond task-file stamps (next artifact: SendAtaCommandPd family in DiskHealth_decompiled.c) |
| S13. AAM/APM device-write setters (0xEF 0x42/0xC2, 0x05/0x85) | **NEVER-DISPOSITIONED** (§5.3) | :8058/:8068/:9565/:9575; no trail row; not reachable from the shipped QML (no sliders in the 54-component census — dead capability in this skin, live code in the binary) |
| S14. Temperature display model (50/60 caution defaults 0x1808 display-only, °C↔°F toggle TempF, tempIndex) | **COVERED (display-only)** — the grader never reads temp (cr_grade.cpp:187-190 forced-Good); the app-side card is OOS-UI | s6 decode §3 (evidence/s6-health-grading-decode.md); cr_grade.h:10-14; QML main.qml:422-446 |
| S15. Periodic refresh/tray/monitoring service (RefreshThread, BGS) | **NOT-CORE-SCOPE (registered)** — "tray/notify/loop is host UI; the pull API is the core contract" | docs/PLAN-API-EXPANSION.md:88; REFERENCE-API-GAP.md:280 (R-61 PARTIAL, deliberate) |
| S16. QML dashboard + caution popup + notifications + single-instance/CLI IPC + language menu | **NOT-CORE-SCOPE (OOS-UI)** | docs/PARITY-OG-CORE-INVENTORY.md:30 (UI exclusion line names "DiskHealth.exe app payload incl. diskhealth/ QML/Qt/bat scaffolding" verbatim); the data contract the QML consumes IS core-covered (S1-S9; API-DELTA.md:75 row 24 `cr_ui_health_read` MATCH+) |
| S17. Per-row TRIM chip datum | **DIVERGENCE (open, known)** — M-076 (MAJOR, ACTIVE): TRIM datum lives health-facet-only; the og's per-row chip needs row-shape fields (R-63) | findings/MAJOR/M-076-trim-facet-enum-rows.md; modules/health.md:179-182 |
| S18. Feature-string bits (AAM/APM/NCQ/TRIM/DevSleep/VolatileWriteCache display from IDENTIFY words) | **NOT-CORE-SCOPE (display-layer)** — the TRIM *datum* is covered by the DRW-side fsutil story (cr_health.cpp:301-323, HEALTH-6); M-076 covers the row shape | strings dump; cr_health.h:21-24 |
| S19. Vendor attr display-name tables + ATA spec-version strings (C19) | **NOT-CORE-SCOPE (presentation)** — core has the curated well-known names (cr_smart_ata.cpp:29-92, pinned by test_health_csmi.cpp:261 `SmartAttrNameTableCoversTheCuratedIds`) | strings dump; S-024's D16-adjacent note (modules/health.md:108) |
| S20. Scheduled-task autostart (bat trio, ONLOGON /rl HIGHEST, ShowIcon registry pairing) | **COVERED (host-concern datum)** | findings/INFORMATIONAL/I-027 item 4 (:22); PARITY-OG-CORE-INVENTORY.md:278 (INSTALLER row names "diskhealth task bats"); og :33735/:34463/:35186-35209 |
| S21. Registry settings HKLM\SOFTWARE\EaseUS\DiskHealth {ShowIcon,TempF,Language} + config.ini theme | **NOT-CORE-SCOPE (OOS-UI/host-app settings)** | inventory header line :30; og :32206-32243/:35201-35205/:34497-34501 |
| S22. AliyunConfig.ini + own telemetry client + event taxonomy (C16) | **NOT-CORE-SCOPE (REPORTING-OOS)** — the family row covers it; og ships opt-out default `bAllowSendInfo=0` | PARITY-OG-CORE-INVENTORY.md:51; I-027 item 3 (:20, the telemetry caps register) — NOTE the row's "consumers = the crash-report chain only" is imprecise for the diskhealth copy (DiskHealth.exe reads/writes it itself, :39732-39762 — see §5.5) |
| S23. Help URLs (9 locales) | **NOT-CORE-SCOPE (OOS-UI)** | inventory :30; CautionWidget.qml:321 |
| S24. 17 product .qm + 23 qt_*.qm | **NOT-CORE-SCOPE (OOS-UI locales)** | inventory :30 ("language/*.qm + translations/"); :99 (Languages registered exclusion) |
| S25. Qt/VC runtime + api-ms-* + plugins + QML caches + 31 PNGs + DiskHealth.rcc assets | **NOT-CORE-SCOPE (RUNTIME/UI payloads)** | inventory :260 (RUNTIME families name diskhealth roots) + :30; runtime-families/qt5-quick-qml-stack.md |
| S26. WMI disk-model cross-check + drive-letter→disk extents mapping (C10) | **NOT-CORE-SCOPE (companion enumeration machinery)** — same class as the D-82 census rationale; core enumerates disks in its storage module | D-82 rationale :2640-2642; og :16703 etc. |
| S27. `sc config Winmgmt start= auto` (C11) | **NEVER-DISPOSITIONED** (§5.4) | :9508; grep-negative in trail; no I-027 item |
| S28. DiskHealthAuto.exe (referenced, absent) | **UNVERIFIED** (§5.6) | string @0x8b73c between the bat-call strings; not in tree, not in inventory.csv |
| S29. `diskHealthIndex=` external-launch contract (DRW/EPM launch the companion pre-targeted at a disk) | **NOT-CORE-SCOPE (companion app-shell)** — but note the contract's shape for any future host: arg format `diskHealthIndex=<n>` parsed at 2 sites | og :233-254/:37772-37797; no core row needed (the core's equivalent data is SmartReport per disk index) |
| S30. Cross-run disk marking persistence | **NO SURFACE** — the og companion persists only UI flags (ShowIcon/TempF/Language/no-remind); grades are recomputed per run; the rwConfig.ini pendingMarkedDisk marker (N-145) is DRW-side, not diskhealth | N-145 (findings/MINOR/N-145-pending-marked-disk-marker-absent.md; modules/orchestration.md:280) |

---

## 5. Divergence candidates / never-dispositioned items (for the parent lane's filings)

All below are report-only-module-class questions (the core's health module is
report-only-by-policy, cr_health.h:26-27) — none contradicts an existing MATCH verdict; each is a
**missing trail disposition**, not necessarily missing code.

### 5.1 The 6 vendor NVMe CMD_TYPE routes — NEVER-DISPOSITIONED (candidate INFO / D-82 addendum)
- **Og:** `DoIdentifyDeviceNVMe{IntelRst,Samsung,Intel,JMicron,ASMedia,Realtek}` +
  `AddDiskNVMe - CMD_TYPE_NVME_{INTEL_RST,SAMSUNG,INTEL,JMICRON,ASMEDIA,REALTEK}` (UTF-16 string
  table, DiskHealth.exe .rdata; scratch dump lines 715-731). The 7th route (STORAGE_QUERY) is
  covered: I-018 byte-parity + core win/cr_health.cpp:264-299. The 6 vendor routes serve NVMe
  controllers/USB-NVMe bridges where the Win10 storage-query route fails (Samsung/Intel vendor
  interfaces, RST NVMe arrays, JMicron/ASMedia/Realtek bridge chips).
- **Core:** one NVMe route (storage query), ERR_HEALTH_UNAVAILABLE on failure.
- **Trail gap:** grep-negative for `NVME_JMICRON|NVME_ASMEDIA|NVME_SAMSUNG|NVME_INTEL|NVME_REALTEK|NVME_STORAGE_QUERY|CMD_TYPE` across diff-to-fix/ + docs/. F-A4-19 named "NVMe incl. Realtek bridge" (docs/parity/PARITY-CORE-DOM.md:80,333) but its fix acceptance (N-032, s30) registered only the CSMI/ATA-side remainders in D-82 (docs/PARITY-DELTAS.md:2614-2648).
- **Suggested disposition:** INFO register row extending D-82: "vendor NVMe CMD_TYPE routes not
  ported; the storage-query route is the universal modern route; vendor routes serve exotic
  bridge/vendor-stack disks → ERR_HEALTH_UNAVAILABLE is the report-only answer" — same rationale
  class as D-82's three.
- **UNVERIFIED sub-item:** the decompile call sites for the 6 vendor routes were not recovered by
  the wave2 pass (grep of DiskHealth_decompiled.c for `DoIdentifyDeviceNVMe`/`AddDiskNVMe` = 0 hits
  — string-table evidence only). Next artifact: targeted re-decompile/xref of the CAtaSmart AddDisk
  dispatch around the `INTERFACE_TYPE_NVME` sites (:19403-19434, :21444-21451).

### 5.2 USB-bridge quirk table — NEVER-DISPOSITIONED (candidate INFO; plus bookkeeping, §5.7)
- **Og:** `AddDisk - Si` :10761; `AddDisk - USB0A/0B/1A/1B/USB2…USB20` (28 distinct log strings,
  :11043-11163+); bridge VID/PID quirks (LOGITEC 0x00D9, Genesys 0x05E3/0x0702, IO-DATA 0x0122,
  JMicron) — the CrystalDiskInfo USB-bridge database (`AddDisk - ASM1352R`, `AddDisk - Si` per the
  companion artifact :11).
- **Core:** no bridge-specific handling; a USB bridge disk gets ATA-passthrough → legacy → CSMI,
  then ERR_HEALTH_UNAVAILABLE (D-82's philosophy).
- **Trail gap:** N-032's own title (`...csmi-usb-bridges-missing`, FIXED-LEDGER.md:215) names the
  usb-bridges half, but the s30 landing text + D-82 register only the RAID census/LSILOGIC/auto-enable
  remainders — the USB-bridge half has **no disposition text anywhere** (grep-negative for
  ASM1352/bridge in diff-to-fix/ + docs/ except the pre-baseline PARITY-CORE-DOM.md F-A4-19:333-335).
- **Suggested disposition:** fold into the same D-82 addendum: "USB-bridge quirk table not ported;
  usbstor's SAT layer serves the standard routes; bridge-only dialects read
  health-unavailable — registered."

### 5.3 AAM/APM device-write setters — NEVER-DISPOSITIONED (candidate INFO)
- **Og:** enable AAM (`0xEF` feature `0x42` + level, :9565), disable AAM (`0xC2`, :8058), enable APM
  (`0x05` + level, :9575), disable APM (`0x85`, :8068) — the CrystalDiskInfo acoustic/power setters
  are live code in the binary, but **unreachable from the shipped QML** (no AAM/APM control in the
  54-component census — dead capability in this skin).
- **Core:** zero device writes (cr_health.h:26-27 report-only contract); D-82 registered only the
  SMART auto-enable (0xD8) as the known device-write, dispatch cite :4427-4438 (first-hand verified:
  task file {0x8a1, 0xe, 0xd8, LBA 0xc24f0001, 0xb0} at :4420-4426 via FUN_00426040).
- **Suggested disposition:** one INFO/parity note: "the companion carries AAM/APM setters (dead in
  this UI skin); the core is report-only and will not grow them" — completes D-82's
  device-write census.

### 5.4 `sc config Winmgmt start= auto` — NEVER-DISPOSITIONED (candidate I-027 addendum)
- **Og:** :9508 — the companion force-configures the **WMI service to autostart** before its
  Win32_DiskDrive/Win32_PnPSignedDriver queries (u16 strings :516-545, :696).
- **Core:** no WMI dependency at all (SSD detection is seek-penalty IOCTL, cr_health.cpp:325-342,
  F-A4-20's aligned story, cr_health.h:21-23).
- **Trail gap:** I-027 item 4 covers the autostart bats as a host-concern datum; this
  system-service modification has no row (grep-negative for "Winmgmt" in diff-to-fix/ + docs/).
- **Suggested disposition:** extend I-027 as item 6 (host-concern datum: the companion mutates
  service startup state; a merged product's installer/host would own any equivalent decision —
  core-side no-op is correct).

### 5.5 AliyunConfig.ini consumer-precision (row-correction candidate, INFO)
- PARITY-OG-CORE-INVENTORY.md:51 says consumers = "the crash-report chain only". First-hand this
  walk: `diskhealth/AliyunConfig.ini` is read AND written by DiskHealth.exe itself
  (GetPrivateProfileIntW/WritePrivateProfileStringW `Config\bAllowSendInfo`, :39732/:39762) — the
  companion ships its own in-process Aliyun OSS telemetry client (Qt5Network; event taxonomy
  C16). The REPORTING-OOS verdict stands (opt-out default 0; core has zero network code); only
  the row's consumer note needs a one-line correction (or an "incl. the DiskHealth companion's own
  client" clause).

### 5.6 `/DiskHealthAuto.exe` — UNVERIFIED datum (candidate I-027 addendum)
- String at file offset 0x8b73c, adjacent to the querytask/deltask/addtask call-format strings
  (`"%1/querytask.bat" %2`, `"%1/deltask.bat" %2`, `/DiskHealthAuto.exe`, `"%1/addtask.bat" %2 "'%3'"`).
- The file is absent from the tree and from inventory.csv (grep-negative) — the scheduled-task
  target %3 is constructed at :33733-33747 from `applicationDirPath()` (with `/`→`\`), so the
  registered task normally points at the installed DiskHealth.exe; the DiskHealthAuto.exe string
  most plausibly names the **auto-mode variant binary** (tray-only) that the installer packaging
  could ship for the ONLOGON task (CrystalDiskInfo ships a similar "CdiTask"/silent variant
  pattern; the `auto` CLI flag +0x388 exists in-binary, :37886-37889).
- **UNVERIFIED — exact use.** Next artifact: the caller chain of FUN_00458600 (addtask runner,
  callers :35209 and :40719 — the latter is the QML-invokable path) to see whether %3 is ever the
  auto exe; plus SetupUE/InfoForSetup decompiled trees for the packaging side.
- Suggested disposition: a data-asset-register line ("referenced-but-absent companion binary;
  autostart task target name; absent from the installed tree — installer-scope datum").

### 5.7 Bookkeeping: the N-032 FIXED finding file is missing from the tree
- FIXED-LEDGER.md:215 points at
  `diff-to-fix/findings/FIXED/N-032-health-transports-csmi-usb-bridges-missing.md`; that file does
  not exist (find over diff-to-fix = 0 hits; no *transport*/*csmi* named file under findings/).
  The ledger row even shows a lowercased `n-health-transports...` variant in one column — both
  paths absent. The technical content survives in modules/health.md Session-30 leg (:167-173),
  SUMMARY.md:3002-3018, and D-82, so no audit information is lost — but the ledger's pointer is
  dangling (same class as the N-134/FIXED-ALREADY bookkeeping repairs the S42 lane already
  handles).

---

## 6. Clean / covered surfaces (verified this walk)

- **The engine's health story is fully covered and pinned by tests** at `869da779`: 37 health
  tests (20 grade + 9 CSMI/chain + 8 NVMe-parser incl. the CLI decidegree test), executing
  pure-core code on Linux CI (G-17 posture documented cr_grade.h:37-38).
- First-hand re-verified og cites used by the trail: ×512,000 data-unit conversion (:5595/:5611),
  K-273 (:26989), 100−percentage_used (:5584), the SMART-enable task file (:4420-4426), the CSMI
  census failure strings (:4620/:4657/:4818/:4828), the "Unknown/Good/Caution/Bad" string census
  (fromAscii), the PDB/build-tree string, Qt 5.15.2, x86 0x14c.
- The diskindex→drive mapping (IOCTL 0x56000-family at :16703/:18212/:20683/:22780/:33338) and
  WMI cross-checks are companion enumeration plumbing — dispositioned with the same rationale class
  as D-82 (companion device-manager machinery).
- The whole deployment payload (QML bundle, Qt/VC runtime, plugins, translations, PNGs, qmldir/qmlc
  caches) sits under the inventory's explicit UI/RUNTIME exclusion lines (:30, :260) — no
  unclaimed file remains in the tree after this walk (every one of the 818 files falls into a
  censused family above).

## 7. Corpus-worklog cross-checks (claims vs tree)

| Claim (corpus worklog.md) | Verified |
|---|---|
| :104 "DiskHealth.rcc = 125 files incl. 54 QML components" | ✅ manifest: 125 files; 10 app + 44 shared QML = 54 |
| :115 wave2 = "DiskHealth … ingested" | ✅ ghidra-wave2/DiskHealth_* present (93,316 lines matches REPORT.md) |
| :202 "DiskHealth QML UI documented (840x590 frameless, SMART table, 32 strings)" | ✅ main.qml:15-17 (840/856×590/606), UIUX/19 |
| :435/:466/:494 "CrystalDiskInfo lineage: CAtaSmart SAT-1A/-2/-2A/CSMI/NVMe-Realtek, USB bridge tables, DiskSmart" | ✅ all strings/sites re-found this walk (see C1-C9) |
| companion artifact :27 "auto.bat Phase E copies DiskHealth … with api*.* + aliyun*.*" + WinPE §2E :74 (`Tools\DiskHealth\bin\DiskInfo.exe`) | ✅ consistent: the boot-media copy is the PE-side variant (named DiskInfo.exe on the USB); the installed tree under audit is the Win32 DiskHealth.exe deployment; the referenced aliyun*.* DLLs are NOT in this tree (boot-USB-side) — cross-consistent with §1.1 |

## 8. Recommended next actions (for the parent lane)

1. **File (INFO-class, or fold into D-82 as an addendum):** the diskhealth transport remainders —
   (a) 6 vendor NVMe CMD_TYPE routes (§5.1), (b) USB-bridge quirk table (§5.2), (c) AAM/APM
   setters device-write census completion (§5.3). One combined register row is proportionate:
   all three are companion device-manager/transport-scope items under the existing report-only
   policy rationale.
2. **File (INFO):** extend I-027 with (d) the `sc config Winmgmt start= auto` host-concern datum
   (§5.4) and (e) the DiskHealthAuto.exe referenced-but-absent datum (§5.6, UNVERIFIED tag kept
   with its next-artifact pointer).
3. **Row-correction (one line):** PARITY-OG-CORE-INVENTORY.md:51 consumer note (§5.5).
4. **Bookkeeping (existing S42 wave-1 class):** the dangling N-032 FIXED file pointer
   (FIXED-LEDGER.md:215 → nonexistent path) (§5.7).
5. **No code changes recommended for the core's health module from this walk** — the walk found
   no engine-side behavior gap beyond the registered/pending items (M-076 stays the one ACTIVE
   health finding; S-024's temp-never-graded decode is confirmed against the QML-side display
   helpers (tempIndex card is display-only, main.qml:422-446; the <50 helper FUN_0045f350
   :37906-37912 is display logic)).

**UNVERIFIED register (all with next artifact):**
- U1: the 6 vendor NVMe routes' decompile call sites (wave2 pass did not recover them) → re-xref
  around :19403-19434/:21444-21451 in DiskHealth_decompiled.c.
- U2: SAT stamp-variant deltas (-1A/-2/-2A) → SendAtaCommandPd family decode.
- U3: DiskHealthAuto.exe exact role → FUN_00458600 caller chain (:35209/:40719) + SetupUE packaging.
- U4: whether the scheduled-task name arg %2 (DAT_00526cc8, the global product-name QString whose
  assignment was not recovered in the decompile pass) is literally "DiskHealth" — evidenced by
  the 16× fromAscii("DiskHealth") census and the EaseUS_DiskHealth{,Console} mutex names, but the
  exact init site was not found → functions.txt static-initializer walk.

---
*Method note: all DiskHealth_decompiled.c line cites were read first-hand this session
(Read-with-offset only; the 93,316-line file was never whole-read). No repository file was
modified; this sheet lives only in /home/z/my-project/s42-scratch/.*
