# REFERENCE-SMART — the og's disk-health architecture (corpus ground truth)

Agent B, Phase-0 reconnaissance. Sources: `/home/z/my-project/easeus-drw-research/` (install dump + RE workspace of EaseUS Data Recovery Wizard, "the og"). Tools used: `strings` (ASCII + UTF-16LE), `objdump -p` (import/export tables), custom .rcc/.qm parsing, plus the corpus's prior Ghidra decompilation (`re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c`, 93,316 lines) as cited by existing decode sheets. No PE disassembler was run in this session; everything marked [OBSERVED] below is from strings/imports/rcc/qm (read this session) or from the cited corpus artifact (verified against binaries this session where noted). Limits of method: string evidence proves presence of code paths, not call ordering; exact control flow claims are only made where the s6 Ghidra decode (already in corpus) backs them.

Corpus paths abbreviated: `og-codebase/diskhealth/` = `DH:`; `re-workspace/` = `RE:`. `u16 strings` = `strings -e l` output of DH:DiskHealth.exe; `ascii strings` = `strings` output of same; both reproduced under `/tmp/dh/` this session and consistent with the s42c scratch dumps (`dh-strings-ascii.txt`, `dh-strings-u16.txt`).

---

## 1 The surface — separate companion app CONFIRMED

**Disk health is a separate 32-bit executable, not a pane inside DRWUI.** [OBSERVED]

- **Binary**: `DH:DiskHealth.exe` — 1,781,480 bytes, `PE32 executable for MS Windows 6.00 (GUI), Intel i386, 8 sections` (file(1) this session). Inventory: `RE:inventory.csv` row `diskhealth/DiskHealth.exe,...,PE,x86,GUI,...`. 0 exports; 4,636 functions / 93,316 decompiled lines (`RE:reversed/ghidra-wave2/DiskHealth_decompiled.c`, census in `RE:artifacts/per-binary/DiskHealth.md`).
- **PDB path**: `F:\drw_backup\Output\Release_Win32\DiskHealth.pdb` [OBSERVED, ascii strings] — same monorepo build tree as the rest of DRW (`architecture/01-build-tree-intelligence.md:66`).
- **Internal product name**: "DiskSmart" — resources `:/res/disksmart.ico`, `:/res/disksmart.ico`, `ico_logo_pallets_disksmart16.png`, window-class mutexes `EaseUS_DiskHealth` / `EaseUS_DiskHealthConsole` [OBSERVED, u16 strings]. Window title: "EaseUS DiskHealth" (`RE:artifacts/UIUX/19-diskhealth-quick-ui.md`, from `qml/main.qml`).
- **GUI framework**: Qt 5.15.2 **Qt Quick/QML** — shipped `DH:Qt5Qml.dll, Qt5Quick.dll, Qt5QuickControls2.dll, Qt5QuickShapes.dll, Qt5QuickTemplates2.dll, Qt5QmlModels.dll, Qt5QmlWorkerScript.dll, Qt5RemoteObjects.dll` + QML import trees (QtQuick, QtQuick.2, QtQml, QtGraphicalEffects, Qt/labs/settings) [OBSERVED, directory walk]. Imports confirm linkage: `objdump -p DiskHealth.exe` lists `Qt5Widgets.dll, Qt5Gui.dll, Qt5Qml.dll, Qt5Network.dll, Qt5Core.dll` (also `mfc140u.dll, CFGMGR32.dll, SETUPAPI.dll, VERSION.dll, WTSAPI32.dll`) [OBSERVED this session]. DRWUI itself is Qt Widgets — the QML stack is DiskHealth-only (`RE:artifacts/runtime-families/qt5-quick-qml-stack.md`).
- **Window**: `NEW.ApplicationWindow` (QtQuick.Controls 2.14), **frameless** (`Qt.FramelessWindowHint`), **840×590** (856×606 with shadow-blur mode), custom `Title` title bar with language menu [OBSERVED, `RE:reversed/ui-resources/diskhealth/qml/main.qml` via `RE:artifacts/UIUX/19-diskhealth-quick-ui.md`; rcc name table this session contains `main.qml, Title.qml, CautionWidget.qml, InfoItem.qml, InfoList.qml, HoverWidget.qml, RotatingWait.qml, MultiLanguage.qml, Theme.qml, ThemeBase.json`].
- **C++/QML bridge**: context property `diskInfoMgr` (class `DiskInfoMgr`, RTTI `.?AVDiskInfoMgr@@` [OBSERVED]) exposing `showWindow()`, `refreshDisks()`, `firstCautionIndex()`, `isTrayExist()`, `setWindowIcon()`, `doExit()`, `theme()`, `currentLanguage`, `isMainland()`, plus signals `sigDiskInfosChanged`, `Result_LoadDiskInfo`, `getDiskInfos`, `diskInfos` [OBSERVED, ascii strings].
- **Visual structure** (dashboard, single-window):
  - disk selector header "Which disk do you care about?" with entries "Disk %1" / "Disk %1 (%2)", jumping to the first failing disk via `firstCautionIndex()` [OBSERVED, `diskhealth_English.qm` strings + UIUX/19].
  - `CautionWidget`: warning card "Your disk may have a physical failure:" with buttons "View Solutions" (locale help URLs) and "Launch Disk Health" [OBSERVED, .qm].
  - `InfoList`/`InfoItem`: per-disk S.M.A.R.T. attribute table, columns **ID / Attribute / Current / Threshold / RawValue / ST** (+ Status) and a "Temprature" [sic] card with °C↔°F toggle [OBSERVED, .qm strings: `ID`… `Attribute`, `Current`, `Threshold`, `RawValue`, `ST`, `Temprature`].
  - status icons `pic_status_good.png / pic_status_bad.png / pic_status_none.png`, temperature icons `pic_temprature_{none,normal,warning}.png` [OBSERVED, rcc name table].
  - 44 shared `publicQML/` EuS components (`EuBuyNowBtn`, `EuCircleProgress`, `EuMessageBox`, …) — Buy Now / license / activation integration [OBSERVED, rcc names + UIUX/19].
- **Launch paths**:
  - from DRWUI: DRWUI.exe strings contain `Disk Health`, `DiskHealth.exe`, `/diskhealth/`, `HKEY_LOCAL_MACHINE\SOFTWARE\EaseUS\DiskHealth` (and WOW6432Node twin) [OBSERVED, `RE:reversed/DRWUI.exe/strings_ascii.txt`]; DRWUI home page has a "Disk Health" tool card ("More Recovery Tools: 6 tool cards Crashed PC/NAS+Linux/RAID/Cloud/Image/Disk Health" — corpus `worklog.md:789`). The launcher passes **`diskHealthIndex=<n>`** so the companion opens pre-targeted at that disk [OBSERVED, u16 strings, arg parsed at 2 sites per s42c §3 C13].
  - from Windows toast notification: `Click_Open_DiskHealth_By_Notification` [OBSERVED, u16 strings]; toast payload `{"Disk":"disk %1"}` (s42c §3 C12).
  - other CLI args: `isPe=`, `auto`, `EPM`, `themeName=` [OBSERVED, u16 strings; s42c C13].
  - autostart: `DH:addtask.bat` = `schtasks.exe /create /f /sc ONLOGON /rl HIGHEST /tn %1 /tr %2` with `deltask.bat`/`querytask.bat` twins [OBSERVED, file contents]. The scheduled-task target is `/DiskHealthAuto.exe` (string present in exe, **the file itself is absent from the tree**) [OBSERVED string / s42c S28].
- **Process lifetime**: tray-resident guardian. `CTrayIcon` + `QMenu`/`QTimer` classes [OBSERVED, artifact vftable census]; `WTSRegisterSessionNotification` import [OBSERVED, objdump]; closing hides to tray when tray exists (`close.accepted = false; window.hide()` — UIUX/19); "Disk Health is guarding your disk health." with "Display alert when disk problem are found. (%1)" [OBSERVED, .qm]. Telemetry events `Result_Startup_BGS` / `Result_Stop_BGS` name a background-service mode [OBSERVED, ascii strings]. One-shot startup prompt "Ask for autostart boot" (UIUX/19).
- **Settings/persistence**: registry `HKLM\SOFTWARE\EaseUS\DiskHealth` — `ShowIcon`, `TempF` (°F toggle), `Language`; app-dir `config.ini [main/realStyleName]` theme; Aliyun telemetry opt-out `DH:AliyunConfig.ini` = `[Config] bAllowSendInfo=0` [OBSERVED: ini file read this session; registry keys via s42c C14 + DRWUI strings].
- **Localization**: 17 product .qm files (Arabic…Thai incl. English and Mainland) + 23 qt_*.qm [OBSERVED, dir listing].

**Mandate check (D-P4 reversal):** the reference model is *separate disk-health surface* — DRWUI keeps only health facts inside its own pages (TRIM SSD notice) and delegates the SMART deep-dive to DiskHealth.exe via `diskHealthIndex=`. This sheet is the ground truth for that model.

---

## 2 Transports per disk class

All [OBSERVED] from `u16 strings` of DH:DiskHealth.exe unless noted; call-flow claims cite the s6 decode (`final-prod/engine/diff-to-fix/evidence/decompiled-quotes/s6-health-grading-decode.md`) or s42c (`final-prod/engine/diff-to-fix/evidence/s42c-diskhealth-walk.md`).

| Disk class | Transport / API (exact evidence) |
|---|---|
| **ATA/SATA internal** | Direct `\\.\PhysicalDrive%d` with IDENTIFY/SMART IOCTLs: log strings `SendAtaCommandPd - IDENTIFY_DEVICE (ATA_PASS_THROUGH)` + `SendAtaCommandPd - IDENTIFY_DEVICE`, `SMART_READ_DATA (ATA_PASS_THROUGH)` + legacy `SMART_READ_DATA`, `SMART_READ_THRESHOLDS (ATA_PASS_THROUGH)` + legacy, `SMART_CONTROL_STATUS (ATA_PASS_THROUGH)` + legacy — i.e. **IOCTL_ATA_PASS_THROUGH with fallback to the older SMART ioctl family** (CrystalDiskInfo's Pd path). Command set: IDENTIFY (0xEC), SMART READ DATA (0xD0), READ THRESHOLDS (0xD1), RETURN STATUS (0xD8 — "PREDICT FAILURE" style), SMART enable. |
| **SAT (SCSI-to-ATA, USB/IEEE1394 bridges)** | `DoIdentifyDeviceSat pd=%d, tt=%d, ct=%d`, `GetSmartAttributeSat - 1 [%s]`, `GetSmartAttributeSat - 1A/-2/-2A` — SAT passthrough via `\\.\SCSI%d:` (`\\.\Scsi%d:` also present) with task-file stamp variants. |
| **CSMI (Intel RST / RAID)** | `DoIdentifyDeviceCsmi`, `GetSmartAttributeCsmi - 1 CSMI`, `FillSmartData`, `GetSmartThresholdCsmi`, `FillSmartThreshold`, `ControlSmartStatusCsmi`, command type `CMD_TYPE_CSMI_PHYSICAL_DRIVE`; RAID census `CC_CSMI_SAS_GET_{DRIVER_INFO,RAID_INFO,RAID_CONFIG,PHY_INFO}` (failed-log strings). CSMI header classes `CSMIALL / CSMIARY / CSMISAS` [also ascii strings]. |
| **NVMe (native)** | **7 routes**: `DoIdentifyDeviceNVMeStorageQuery` → `AddDiskNVMe - CMD_TYPE_NVME_STORAGE_QUERY` (IOCTL_STORAGE_QUERY_PROPERTY protocol-specific; IOCTL 0x2D1400 at 4 call sites per s42c C5/I-018), plus vendor routes `CMD_TYPE_NVME_{INTEL_RST,SAMSUNG,INTEL,JMICRON,ASMEDIA,REALTEK}` (`DoIdentifyDeviceNVMe{IntelRst,Samsung,Intel,JMicron,ASMedia,Realtek}` + `GetSmartAttributeNVMeRealtek`, `NvmeMini`). Health info read from the NVMe health log page; engine class family `SmartNVMe`. |
| **SCSI/SAS** | `DoIdentifyDeviceScsi(%d, %d) - 4/-7` — SCSI pass-through identify; display " SCSI Disk Device"/" SCSI Device". |
| **USB-bridged** | Bridge quirk ladder with named vendors: `usbVendorId == USB_VENDOR_JMICRON`, `USB_VENDOR_SUNPLUS`, `USB_VENDOR_CYPRESS`, `USB_VENDOR_INITIO || USB_VENDOR_OXFORD`, `USB_VENDOR_IO_DATA` (product 0x0122), `USB_VENDOR_LOGITEC && productId == 0x00D9` (explicitly FALSE-skipped), Genesys `0x05E3/0x0702` (FALSE-skipped), `AddDisk - ASM1352R`; 28 numbered branches `AddDisk - USB0A … AddDisk - USB20` + `AddDisk - Si`, plus `FlagUsbSat=%d` and `AddDisk - USB10` fallback. Controller association via WMI `Win32_USBController` + `USBSTOR\VID_/PID_` parsing. |
| **IEEE 1394** | `1394`, `IEEE 1394 SBP2 Device`, `INTERFACE_TYPE_IEEE1394`, display "IEEE 1394 (%s)". |
| **WMI fallback** | `SELECT * FROM MSStorageDriver_FailurePredictData` / `...FailurePredictThresholds` from `\\.\root\WMI` (InstanceName/VendorSpecific) — the StorageDriver SMART class pair. |
| **SD cards** | No SD-specific transport strings exist; SD appears only as removable mediaType from `Win32_DiskDrive` enumeration. → see §5. [OBSERVED by absence] |

Device-write side capabilities (live code, unreachable from shipped QML per s42c S13): AAM enable/disable (0xEF/0x42, 0xEF/0xC2), APM enable/disable (0xEF/0x05, 0xEF/0x85), SMART auto-enable (`SMART_CONTROL_STATUS`, 0xB0/0xD8 task file) [OBSERVED, s42c C9 citing decompile :9565/:8058/:9575/:8068/:4420-4426].

WMI is so central the app **force-starts the service**: `config Winmgmt start= auto`, `StartService - 1/2`, `Waiting... Winmgmt`, `OK:WMI Init` [OBSERVED, u16 strings; s42c C11].

---

## 3 The attribute surface per transport

### 3.1 Table shape (all transports)
QML `InfoList` columns: **ID / Attribute / Current / Threshold / RawValue / ST** — i.e. attribute id, display name, current value, threshold, raw value, and worst ("ST") + per-row Status color (1=green/2=amber/3=red per s42c C17; icons pic_status_good/bad/none). Data model per disk: 30-attribute table, 12 bytes per attribute row, value/worst/raw6 (s42c S1, s6 decode §8; core parity `cr_smart_ata.cpp`). [OBSERVED]

### 3.2 ATA/SATA attribute names (display vocabulary)
The engine carries CrystalDiskInfo's per-vendor display-name tables — family names in u16 strings: `Smart, SmartSsd, SmartMtron, SmartIndilinx, SmartJMicron, SmartIntel, SmartSamsung, SmartSandForce, SmartMicron, SmartOcz, SmartSeagate, SmartWdc, SmartPlextor, SmartSanDisk, SmartOczVector, SmartToshiba, SmartCorsair, SmartKingston, SmartMicronMU02, SmartNVMe, SmartRealtek, SmartSKhynix, SmartKioxia, SmartSsstc, SmartIntelDc, SmartApacer, SmartSiliconMotion, SmartPhison, SmartMarvell, SmartMaxiotek, SmartYmtc` (plus sub-families `SmartJMicron66x/61x/60x`, `SmartSanDiskGb`, `SmartKingstonSA400/KC600/DC500/Suv`, `SmartSeagateIronWolf/BarraCuda`). Sample English names from `DH:translations/diskhealth_English.qm` (QObject-context, i.e. C++ tables, not QML): `Raw data error rate`, `Airflow Temperature`, `Bad Block Count`, `Current Pending Sector Count`, `Command Timeout`, `CRC Error Count`, `ECC Rate`, `Current Helium Level`, `Flying Height`, `Disk Shift`, `Timed Workload Timer` (ascii), `SATA CRC Error Count`/`SATA Interface Downshift`/`SATA PHY Error` variants, `Grown Bad Blocks`, SSD wear names (`Erase Count`, `Average P/E Cycles`, `Available Reserve Space`, …) [OBSERVED, .qm + ascii strings]. Vendor-specific model quirks are hardcoded (u16 strings: `OCZ-VERTEX3`/`2.02`, `OCZ-VERTEX2`/`1.27`, `SAMSUNG HD155UI`/`HD204UI`+`1AQ10003`, `FuzeDrive`, `StoreMI`, Micron M500/M510/M550/M600/MU01/1100/1300, Crucial BX/MX series, SanDisk X110-X600/SDSSDP/SDSSDRC/pSSD, Kingston SA400/KC600/DC500/SUV300-500, SK hynix SC311/SC401, KIOXIA, `LITEON IT LMT`, `ADATA SU700/SX950/SP580`, Seagate BarraCuda/IronWolf, ZHITAI, Apacer, Maxio). [OBSERVED]
ATA interface/spec strings for display: `PIO/DMA, UDMA/16..133, SATA/150, SATA/300, SATA/600, Parallel ATA, Serial ATA`, and the full ATA major-version list `ATA-1 … ATA/ATAPI-7, ATA8-ACS, ACS-2/3/4` (published-ANSI variants). [OBSERVED, u16 strings]
Feature-string builder (from IDENTIFY words): "S.M.A.R.T., AAM, APM, NCQ, TRIM, DevSleep, VolatileWriteCache" (s42c C19). [OBSERVED]

### 3.3 NVMe fields
Health-log fields displayed (English .qm, QObject context): `Critical Warning`, `Composite Temperature`, `Available Spare`, `Available Spare Threshold`, `Data Units Read`, `Data Units Written`, `Media and Data Integrity Errors`, `Percentage Used` (as life, see §4), plus generic `Controller Busy Time`, `Power Cycles`, `Power On Hours`, `Unsafe Shutdowns`, `Warning`/`Critical` spare states, `NAND Temperature`, `Drive Temperature`, `Enclosure temperature` [OBSERVED, .qm]. Conversion decode (s6 §9, decompile :5579-5595, :26985-27010): **data units × 512,000** (×0x7D000) to bytes; **temperature = Kelvin − 273**; **life = 100 − percentage_used** (clamped ≥ 0); page+0 byte = critical_warning. NVMe model detection: `NVM Express`, `NVM Express 1.0/1.1`, `NVM Express %d.%d`, `Parallels Virtual NVMe Disk` [OBSERVED, u16 strings].

### 3.4 SCSI
SCSI disks get identify via `DoIdentifyDeviceScsi` and the generic table; the grading treats them via the same Unknown/Good/Caution/Bad chain; media-error/bad-block names present (`Bad Block Count`, `Bad Block Rate`, `Grown Bad Block Count`, `Bad Cluster Count`) [OBSERVED strings; depth of SCSI log-page parsing = UNKNOWN, Agent E].

---

## 4 Health grading — the decode

Ground truth: `final-prod/engine/diff-to-fix/evidence/decompiled-quotes/s6-health-grading-decode.md`, itself verified against `RE:reversed/ghidra-wave2/DiskHealth_decompiled.c`. Status vocabulary [OBSERVED, exe strings: `Unknown`, `Good`, `Caution`, `Bad`; QList built at :37087-37100]. **No percentage health score — the disk grade is a 4-state enum stored at disk+0x1800.**

1. **Disk grade** `FUN_0040cdf0` returns 0/1/2/3 = Unknown/Good/Caution/Bad.
2. **NVMe path first** (family 0x13): `Parallels Virtual NVMe Disk` → Good; **critical_warning byte ≠ 0 → Bad**; life = 100 − percentage_used: **life > 10 → Good; == 10 → Caution; < 10 → Bad. Media errors are NEVER read by the grader.**
3. **Unknown gates**: no attribute data (0x16d8==0); HDD with no attrs (0x1724==0 && 0x16dc==0); failure flag 0x16f8≠0 → Unknown.
4. **Per-attr loop**: duplicate attr id → Unknown; attr 194 (and 184) always Good — **temperature is never graded** (the temp-caution field 0x1808 with defaults 50°C HDD / 60°C SSD is display-only); threshold==0 → Good; value<threshold → bad_count++ for SSDs (any attr), for HDDs only attrs in {1..13, 187..189, 191..193, 195..209, 211..212, 220..228, 230, 231, 240, 250, 254}.
5. **Attrs 5/197/198 (HDD only)**: raw[0..3] not all-FF && caution-count ≠ 0 && count ≤ raw16 → Caution flag (counts default 1 at 0x182a/0x182c/0x182e).
6. **SSD-wear life attrs** (per vendor family): 169→{0x14,0x1a,0x11&sub3}; 173→{0xf,0x16}; 177→{6}; 187→{2}; 202→{8,0x12,0x18}; 209→{3}; 231→{7,0x10,0x11,0x15,0x14,0xd,0x17,0x19,4,0xa,0x1b,0x1d,0x1e}; 232→{0xc}; 233→{5,9,0xe,0x15}. Life = 100−raw[0] (0x17e8 path) or raw[0] (0x17e4 path) or value, clamped [0,100]; **life==0 → Bad; life > 10 (0x1830) → Good; else Caution.** Attr 230 (WDC 0xb/SanDisk 0xd): life = 100 − used (raw16/100 or raw[1]); same thresholds.
7. **Per-attr display grade chain** (:34148-34167): attr 194/184 → Good; 5/197/198 → threshold rule + count rule; 230 → wear rule; SSD-wear attrs → family gates; attr 1 family-7 (SandForce) all-zero/0xB0 rule; general: threshold==0 → Good, threshold ≤ value → Good, else Bad.
8. **is-SSD detection** (writer :6300-6344): model prefix `OCZ`/`SPCC`/`PATRIOT`/`PHOTOFAST`/`STT_FTM`/`Super Talent`, or contains `Solid`/`SSD`/`SiliconHardDisk` (note: `KINGSTON SV300S37A` misses → graded as HDD, og-faithful quirk).
9. **Vendor family cascade** `FUN_0040dad0`: 30+ families with model-prefix/attribute-signature gates — full table in s6 §5 (WDC `"WDC "`, Seagate IronWolf attr signature {1,5,9,12,100,0x66,0x67,0xAA,0xAB,0xAC,0xB3,0xB5,0xB7,0xB9,0xBB}, MTRON, JMicron 60x/61x/66x signatures, Intel, Samsung `"SAMSUNG"`/`"MZ-"`, Micron M500-M600/MU01, MicronMU02 CRUCIAL/CT, SandForce sig+0x16f0, OCZ, OCZ-Vector, Plextor, SanDisk, SK hynix, Kingston, Kioxia, Phison (firmware `"S9"` gate), Apacer, Maxiotek, Marvell, SiliconMotion, Ymtc, Ssstc, IntelDc, Indilinx (also sets skip-flag 0x16ec), NVMe "NVM Express", fallback Smart/SmartSsd).
10. **Aggregation**: bad_count>0 → Bad; graded → (caution ? Caution : Good); else Unknown.

UI corroboration [OBSERVED, .qm]: "Good", "Caution!", "Your disk may have a physical failure:", first-Caution jump (`firstCautionIndex()`), per-row status colors, caution card + "View Solutions".

---

## 5 Not-supported handling

- The observable vocabulary is the 4-state **"Unknown"** status (icon `pic_status_none.png`, gray) — a disk whose SMART cannot be read lands in Unknown via the §4.3 gates [OBSERVED: string + icons; OBSERVED decode gates].
- Devices that enumerate but expose no SMART (typical SD cards / unsupported USB bridges) still appear in the disk list (from `Win32_DiskDrive`, removable MediaType) but their attribute table is empty and the disk grade is Unknown [INFERRED — enumeration strings include removable-media naming (`SanDisk Extreme`, `Kingston DT Ultimate`, `removable`) while no SD transport exists; grade gates in §4.3].
- Bridge-level failures are logged (not user-visible): `FAILED: CC_CSMI_SAS_GET_DRIVER_INFO/PHY_INFO/RAID_INFO/RAID_CONFIG`, `GetDiskInfo - FALSE0/FALSE1`, `COMMAND_TYPE_UNKNOWN`, `NG:WMI Init 1/2/3` [OBSERVED, u16 strings].
- No "not supported" phrase was found in the English .qm UI strings read this session; exact user-facing wording for a no-SMART disk is [UNKNOWN — Agent E: check InfoList/InfoItem empty-state rendering and the full 32-string qsTr census in `RE:artifacts/UIUX/19-diskhealth-quick-ui.md` §3 — no such string is in that census either, supporting Unknown-status-as-the-answer].

---

## 6 Enumeration model

[OBSERVED, u16 strings unless noted]
1. **PhysicalDrive ladder**: probe `\\.\PhysicalDrive%d` 0..N with `DoIdentifyDevicePd(%d, 0xA0)` (SATA/ATA master) and `DoIdentifyDevicePd(%d, 0xB0)`; `AddDisk(%d, %d, %d)`; `GetDiskInfo pd=%d, sp=%d, st=%d, mt=%s`; log `OK:GetDiskInfo - PhysicalDrive`.
2. **WMI cross-check**: `SELECT * FROM Win32_DiskDrive` (fields read: Size, DeviceId, Model, FirmwareRevision, SCSIPort, SCSITargetId, SCSIBus, MediaType, InterfaceType, PNPDeviceID, SerialNumber); controller maps via `SELECT Name, DeviceID FROM Win32_IDEController` (+ `PCIIDE\IDECHANNEL` filter, `ASSOCIATORS OF … Win32_IDEControllerDevice`), `Win32_SCSIController` (blacklist entries: `NVIDIA`, `Marvell`, `VIA VT6410`, `ITE IT8212`, `DVDFab Virtual Drive`, `Silicon Image SiI`, `BUFFALO IFC-PCI2ES/IFC-PCIE2SA` — the CrystalDiskInfo AMD/NVIDIA-virtual-controller skip list), `Win32_USBController` (`USBSTOR\VID_/PID_`, `VID=%04Xh, PID=%04Xh`), plus `select DeviceName, DriverVersion from Win32_PnPSignedDriver` (AMD-RAID detection). WMI lifecycle: `CoInitializeSecurity/CoCreateInstance/ConnectServer(\\.\root\cimv2)/CoSetProxyBlanket`, `CAtaSmart::Init WMI on/off - Start`, `--ControllerMap--`.
3. **SetupAPI/CFGMRGR**: imports `SetupDiGetClassDevsW`, `SetupDiEnumDeviceInfo`, `CM_Get_Device_IDW` [OBSERVED, objdump this session].
4. **Drive-letter → disk mapping**: `\\.\%c:` with `volumeDiskExtents.NumberOfDiskExtents = %d`, `n = %d, pDiskExtent->DiskNumber = %d` — IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS (0x560000 per s42c C10).
5. **Per-disk identity shown**: Model (`WmiModel:`/`Model:`), `SerialNumber:`, FirmwareRev, diskSize, interface type; display strings `" SCSI/SATA/ATA Disk Device"`, `" SCSI/SATA/ATA Device"`, `"NVMe "`, `"UASP (%s)"`, `"USB (%s)"`, `"IEEE 1394 (%s)"`; form factors `5.25/3.5/2.5/1.8/<1.8 inch`; interface strings from §3.2.
6. **Ordering/dedupe**: `OK:qsort`, model check `OK:Check Model Name`, `DISABLED: vars.RemoveAt(index) - 2` (hidden entries), FuzeDrive/StoreMI handling.
7. **Scope**: internal SATA/NVMe/SCSI + USB (UASP/BOT) + IEEE1394 + virtual (Parallels). SD = removable media only.

---

## 7 Refresh model

- **Manual**: `refreshDisks()` QML action + refresh icon `icon_button_refresh_blue18.png`/`gicon_button_refresh_gray18.png` [OBSERVED, strings/rcc]; telemetry `Click_Refresh` [OBSERVED].
- **Background poller**: RTTI `.?AVRefreshThread@@` — a dedicated refresh thread [OBSERVED ascii strings; vftable census in DiskHealth artifact §2]; `WakeUp(%d)` [OBSERVED, u16]. **Poll interval: UNKNOWN** — no interval/minutes string in either dump; needs Agent E (Ghidra: RefreshThread::run / timer setup). QML `Timer{ interval: 40 }` is UI-level (40 ms, not the poll).
- **Live monitoring**: tray + toast `{"Disk":"disk %1"}` on Caution/Bad transitions ("Display alert when disk problem are found. (%1)") [OBSERVED]; `Result_LoadDiskInfo` telemetry [OBSERVED].
- **Session awareness**: WTSRegisterSessionNotification [OBSERVED import].

---

## 8 Windows version matrix

- The exe embeds CrystalDiskInfo's full Windows-version detector: `Windows 95/95 OSR2*/98/98 SE/Me/9x`, `NT3.%d/NT4/2000/XP/Server 2003(R2)/Storage Server 2003/Server 2008(R2)/Vista/7/Server 2012(R2)/8/8.1/Server 2016/2019/10`, `Windows NT %d.%d`, ARM32/ARM64/IA64, plus SKU strings (Home Basic…Datacenter) and `SOFTWARE\Microsoft\NET Framework Setup\NDP\v2.0.50727/v4` probes [OBSERVED, u16 strings]. This detector only feeds display/log lines; no per-OS transport branch is evidenced in strings.
- **Transport implications** [INFERRED]: NVMe `CMD_TYPE_NVME_STORAGE_QUERY` (IOCTL_STORAGE_QUERY_PROPERTY protocol-specific) requires Win10+ storage stacks; the vendor NVMe routes (Samsung/Intel/Intel-RST/JMicron/ASMedia/Realtek) exist precisely for Win7/8.x NVMe and USB-NVMe bridges where the generic route is unavailable — same rationale as CrystalDiskInfo. WMI route requires Winmgmt (auto-started, §2). Qt 5.15.2 runs Win7+; i386 build runs on x64 via WOW64 (WOW6432Node registry strings present in DRWUI).
- No Win7-vs-Win11 behavioral difference is pinned in the corpus beyond the above [UNKNOWN — Agent E].

---

## 9 CrystalDiskInfo lineage — CONFIRMED

Evidence, all [OBSERVED] this session from DH:DiskHealth.exe strings/RTTI:
- RTTI class `.?AVCAtaSmart@@` with nested structs `.?AV?$CArray@UATA_SMART_INFO@CAtaSmart@@`, `...UDISK_POSITION@CAtaSmart@@`, `...UEXTERNAL_DISK_INFO@CAtaSmart@@` — **`CAtaSmart` is CrystalDiskInfo's engine class name** (its AtaSmart.cpp).
- Trace/log function names match CrystalDiskInfo's source line-for-line: `CAtaSmart::Init WMI on/off - Start/Complete`, `GetSmartAttributePd/Sat/Csmi`, `GetSmartThresholdPd/Csmi`, `DoIdentifyDevicePd/Scsi/Sat/Csmi/NVMe*`, `FillSmartData/FillSmartThreshold`, `CheckSmartAttributeCorrect`, `vars.RemoveAt(index)`, `OK:Check Model Name`, `DO:GetDiskInfo`, `USB-HDD Check`, `Check Bus Type`, `Check Reverse`.
- The `Smart*` vendor family table (§3.2) matches CrystalDiskInfo's SmartTAttribute/TAttributeName tables including exotic families (MTRON, Indilinx, SandForce, OCZ-Vector, Maxiotek, Ymtc, Ssstc).
- The USB-bridge quirk ladder (`AddDisk - USB0A…USB20`, `AddDisk - Si`, `AddDisk - ASM1352R`, LOGITEC/IO-DATA/SUNPLUS/CYPRESS/INITIO/OXFORD/JMICRON vendors) is CrystalDiskInfo's AtaSmart.cpp USB-bridge database.
- The Windows-version mapping (§8), ATA spec-version table, form-factor list, and the AMD/NVIDIA/ITE/VIA controller blacklist are CrystalDiskInfo staples.
- Corroboration in corpus: `RE:artifacts/DiskHealth/diskhealth--smart-companion.md` §1 ("CrystalDiskInfo lineage: the engine class census … EaseUS's DiskHealth is built on that codebase family, re-skinned with the Qt Quick UI"); corpus worklog.md:435/466/494 ("DiskHealth CrystalDiskInfo lineage").
- **What is NOT CrystalDiskInfo**: the whole Qt Quick shell (840×590 frameless dashboard, EuS component library, buy/license), the DRW monorepo build, EaseUS Aliyun telemetry, locale/Mainland gating, help URLs; and CrystalDiskInfo features **stripped**: no sound alarm, no mail report, no graph/plot, no export (grep-negative for .wav/PlaySound/mail/smtp/export — s42c C20). Conclusion: the **SMART engine is derived from the open-source CrystalDiskInfo codebase** (function-level identity), wrapped in an original EaseUS QML product shell. [OBSERVED for engine; INFERRED for "derived from open source" as opposed to convergent reimplementation — function-name + table identity makes independent reimplementation implausible.]

---

## 10 Rebuild spec for our product (checkable behaviors)

Every item cites the section above. Implement the **separate disk-health surface** model (D-P4 reversed).

**Surface**
1. [OBSERVED §1] Disk health lives in a separate process (`DiskHealth.exe`, 32-bit GUI) launched from the DRW home page "Disk Health" card, with CLI `diskHealthIndex=<n>` pre-selecting the disk; single-instance via named mutex (`EaseUS_DiskHealth`); second launch focuses the existing window.
2. [OBSERVED §1] Own window: ~840×590 frameless, custom title bar, tray-resident on close (guardian model), toast notifications on bad health that re-open the app (`Click_Open_DiskHealth_By_Notification`).
3. [OBSERVED §1] Layout: drive selector ("Which disk do you care about?" → "Disk %1 (%2)"), caution card ("Your disk may have a physical failure:" + "View Solutions" + "Launch Disk Health"), S.M.A.R.T. table (ID / Attribute / Current / Threshold / RawValue / ST, per-row Good/Caution/Bad/Unknown color), temperature card (°C↔°F), language menu (17 locales, Mainland-locked variant), Buy/license surface.
4. [OBSERVED §1] Auto-start prompt at first run; persistence via `schtasks /create /sc ONLOGON /rl HIGHEST` + delete/query; registry HKLM\SOFTWARE\EaseUS\DiskHealth {ShowIcon, TempF, Language}.

**Transports (priority order per disk)**
5. [OBSERVED §2] ATA/SATA: IDENTIFY(0xEC) + SMART READ DATA(0xD0) + READ THRESHOLDS(0xD1) via ATA_PASS_THROUGH on `\\.\PhysicalDrive%d`, legacy-ioctl fallback.
6. [OBSERVED §2] SAT for bridges (`\\.\SCSI%d:`), CSMI (`CC_CSMI_SAS_*`, `CMD_TYPE_CSMI_PHYSICAL_DRIVE`) for Intel RST/RAID, SCSI identify for SAS, WMI `MSStorageDriver_FailurePredict{Data,Thresholds}` fallback.
7. [OBSERVED §2] NVMe: protocol-specific storage-query health log first; vendor routes (Samsung/Intel/Intel-RST/JMicron/ASMedia/Realtek) as fallbacks for Win7/8-era and USB-NVMe.
8. [OBSERVED §2] USB bridge VID/PID quirk table incl. ASM1352R, Sunplus, Cypress, Initio/Oxford, IO-DATA, JMicron; Genesys 0x05E3/0x0702 and LOGITEC 0x00D9 explicitly skipped.
9. [OBSERVED §2] Force Winmgmt auto-start before WMI queries.

**Grading**
10. [OBSERVED §4] 4-state disk grade (Unknown/Good/Caution/Bad) — no percentage. NVMe: critical_warning→Bad; life=100−pct_used with 10/10 cutoffs; media errors ignored. HDD threshold rules on attr set {1..13,187..189,191..193,195..209,211..212,220..228,230,231,240,250,254}; attrs 5/197/198 raw16-count rule; SSD-wear life attrs 169/173/177/187/202/209/231/232/233/230 with family gates and life>10 rule; attrs 194/184 always Good; duplicate-attr→Unknown.
11. [OBSERVED §4] is-SSD by model-string gates; 30-family vendor classifier (table in s6 §5) — replicate the decoded set, document the exotic residual.
12. [OBSERVED §3.3] NVMe display conversions: data units ×512,000; Kelvin−273; life=100−pct_used.

**Enumeration & refresh**
13. [OBSERVED §6] PhysicalDrive0..N ladder + WMI `Win32_DiskDrive` cross-check + controller blacklist + USB VID/PID; per-disk model/serial/firmware/size/interface; drive-letter↔disk via VOLUME_DISK_EXTENTS.
14. [OBSERVED §7] Manual refresh + background RefreshThread + tray/toast alerts. [INFERRED] interval configurable/undocumented — pick a sane default (e.g. CrystalDiskInfo-style minutes) and mark as divergence-for-Agent-E.

**Not-supported**
15. [OBSERVED §5] No-SMART devices show Unknown status with gray icon, remain in the disk list; no dedicated "not supported" string is used.

---

## 11 Open questions for Agent E (need binary RE beyond strings)

1. **RefreshThread poll interval** and whether it only refreshes on tray-alert conditions (decompile `RefreshThread::run` / timer setup in DiskHealth_decompiled.c).
2. **DiskHealthAuto.exe** (string `/DiskHealthAuto.exe` @0x8b73c, absent from tree): is it the scheduled-task stub that relaunches DiskHealth hidden (`auto` arg)? Behavior of `isPe=`, `EPM`, `themeName=`.
3. **Exact empty-state UI for no-SMART disks** (InfoList/InfoItem rendering when the attribute table is empty; whether a specific string is shown). [UNKNOWN]
4. **The 6 vendor NVMe routes' IOCTL details** (Samsung/Intel/Intel-RST/JMicron/ASMedia/Realtek) — never dispositioned (s42c §5.1); which log pages each reads.
5. **USB bridge quirk payload**: what each of `AddDisk - USB0A…USB20` branches actually changes (command-selection vs skip).
6. **SAT/Pd `-1A/-2/-2A` stamp variants**: do they differ beyond task-file stamps? (s42c S12).
7. **SCSI/SAS attribute depth**: which SCSI log pages (if any) populate the table.
8. **SD-card / removable handling**: whether such disks are hidden, graded Unknown, or skipped in the selector.
9. **DRWUI→DiskHealth launch mechanism**: the exact CreateProcess/ShellExecute site in DRWUI.exe for the tool card, and whether EPM (partition product) shares it.
10. **AAM/APM setters reachability** (s42c S13: dead capability in this skin?) and the `sc config Winmgmt` elevation behavior (S27).

---

*Sheet composed 2026-11-17 by Agent B (Phase-0). Primary evidence re-verified this session: DiskHealth.exe strings (ASCII + UTF-16), objdump imports, addtask/deltask/querytask.bat contents, AliyunConfig.ini, DiskHealth.rcc name-table parse (109-entry prefix verified: res/publicQML/qml + QML/json/ico/png names), translations/ listing, diskhealth_English.qm string extraction. Cross-checked against: re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md, artifacts/UIUX/19-diskhealth-quick-ui.md, evidence/s6-health-grading-decode.md, evidence/s42c-diskhealth-walk.md, inventory.csv, corpus worklog.md.*
