# VERIFICATION — independent evidence checks (session 2026-11-17, agent E rerun)

Independent verification leg of Phase-0 reconnaissance. All evidence below was gathered
statically in this sandbox (no Windows, no wine — nothing in the corpus was executed).
Every verdict is one of VERIFIED / REFUTED / PARTIAL / UNVERIFIED, backed by quoted
evidence (exact strings, path:line, import lists). No speculation dressed as verification.

## 0 Tooling available in this sandbox

- `strings`, `objdump`, `nm`, `file`: present at `/usr/bin/` (binutils set).
- `xxd`: NOT present (would use `od`/`hexdump` as fallback; not needed this pass).
- `python3` with `pefile 2024.8.26`: present (import verified, version printed). Not strictly
  required — strings + objdump covered everything below.
- No Windows, no wine. Static analysis only; no corpus binary was executed.

## V1 Product engine transports today

**Verdict: VERIFIED (transport ladder present in engine) with one naming nuance and one explicit non-goal.**

Grep of `final-prod/engine/src` + `engine/include` for storage IOCTL names:

- `IOCTL_ATA_PASS_THROUGH` / `ATA_PASS_THROUGH_EX` — **present**:
  - `engine/src/win/cr_health.cpp:5` ("1. IOCTL_ATA_PASS_THROUGH (ATA_PASS_THROUGH_EX)"), `:40` (`ATA_PASS_THROUGH_EX apt;`), `:59` (`DeviceIoControl(h, IOCTL_ATA_PASS_THROUGH, ...)`).
- `SMART_RCV_DRIVE_DATA` — **present, but NOT under the name `IOCTL_SMART_RCV_DRIVE_DATA`**: the engine spells it without the `IOCTL_` prefix and defines the code manually:
  - `engine/src/win/cr_health.cpp:69-70` `#ifndef SMART_RCV_DRIVE_DATA / #define SMART_RCV_DRIVE_DATA 0x0007c088`, used at `:105` (`DeviceIoControl(h, SMART_RCV_DRIVE_DATA, ...)`), header note at `cr_health.h:11` ("2. SMART_RCV_DRIVE_DATA 0x7C088 - the legacy fallback").
  - `SMART_GET_VERSION` — **absent** everywhere in the engine (grep returned nothing).
- `IOCTL_STORAGE_QUERY_PROPERTY` — **present**: `engine/src/win/cr_enumerate.cpp:44,59,80`; `engine/src/win/cr_transport.cpp:109`; `engine/src/win/cr_health.cpp:333,452,473`.
- `StorageDeviceProtocolSpecificProperty` — **present**: `engine/src/win/cr_health.cpp:323` (`query->PropertyId = StorageDeviceProtocolSpecificProperty;` — the NVMe identify/health-log route per `cr_health.h:86`).
- `IOCTL_SCSI_PASS_THROUGH` as such — **absent**; the SCSI-miniport route is CSMI CC-0x19 instead: `engine/src/core/health/cr_csmi.cpp` (SRB_IO_CONTROL, signature "CSMISAS", per `engine/include/cr/health/cr_csmi.h:65`), wired as transport 3 in `cr_health.cpp:7,135` ("CSMI CC-0x19 SCSI-miniport pass-through (N-032)... for Intel RST / AMD rcraid arrays").
- WMI: `MSFT_PhysicalDisk` appears **only in a comment**, not as an implementation: `engine/include/cr/win/cr_health.h:21` — "DRWUI's SSD detection used WMI MSFT_PhysicalDisk MediaType==4; ours uses the seek-penalty IOCTL heuristic (HEALTH-5)". `MSStorageDriver_FailurePredictStatus`-style classes: **absent** (no `MSStorageDriver` hit at all).

So the product today implements a 3-route ATA SMART ladder (ATA PASS_THROUGH → SMART_RCV_DRIVE_DATA → CSMI CC-0x19), NVMe via `IOCTL_STORAGE_QUERY_PROPERTY` + `StorageDeviceProtocolSpecificProperty`, enumeration via `IOCTL_STORAGE_QUERY_PROPERTY`. Vendor family logic exists in `engine/src/core/health/cr_family.cpp` (JMicron 60x `:115`, JMicron 66x `:120`, Samsung helper `:129-131` with "SAMSUNG" contains / "MZ-" prefix, plus Toshiba/Corsair/Realtek/Indilinx/IntelDc/SiliconMotion/Marvell at `:13`). Compared to REFERENCE-SMART's transport list (ATA/SAT/CSMI/7-NVMe-routes/USB-quirk-ladder/WMI-fallback), the engine-side WMI fallback is deliberately NOT implemented (documented as a design choice in `cr_health.h:21`), and RAID-census routing + LSILOGIC sibling transport are explicitly left companion-side per the `cr_health.h:15-19` comment.

## V2 Product scan stages & progress weights today

**Verdict: VERIFIED for the 40/35/25 weights and the quick→advanced stage machine — with a location correction: the numeric weights live in the ENGINE header + bridge call-site comment, not in `contract.ts:177-184` (that area holds the `ScanStage`/`ScanPhase` types). PRODUCT-STATE's "weights live in the bridge/contract" is therefore PARTIAL on location.**

Stages in code today (`engine/src/core/orchestrator/cr_scan.cpp`):
- Stage flags `STAGE_QUICK | STAGE_DEEP` + `run_carve` config (`cr_scan.cpp:309,317,337-338,360,365`). Header comment `cr_scan.cpp:6-9`: "engine pass (quick and deep share it - the old code walked the MFT twice and double-emitted FAT), deep adds the remnant sweep + parallel lost-partition candidate scans, carve fans window ranges out over the [worker] pool".
- Carve tree skeleton S-048: `cr_scan.cpp:212` `kCarveCategorySkeleton[] = {"Pictures", "Documents", "Archives", "Videos", ...}` (7 categories, line 206 comment) + resume roots `"Other Deleted Files (Carved)"` / `"Reconstructed"` (`cr_scan.cpp:356`).

Progress computation:
- `engine/src/core/orchestrator/cr_scan.cpp:131` `u32 weighted_progress(const StageWeights& w, u32 quick_pm, u32 deep_pm, u32 carve_pm)` — per-stage permille (0-1000, clamped at `:135-140`), combined at `:143` `num = w.quick*quick_pm + w.deep*deep_pm + w.carve*carve_pm`, used at `:479-482` and `:541-544` (monotonic overall).
- Weights defined at `engine/include/cr/orchestrator/cr_scan.h:72-76`:
  `struct StageWeights { u32 quick = 40; /* percent-of-total per stage (monotonic weighted) */ u32 deep = 35; u32 carve = 25; };`
- Bridge side: `ui/src-tauri/src/commands/scan.rs:6-7` documents `scan_create (zero-init config: stage_mask 7, weights 40/35/25, threads auto...)`; the payload composition is `ui/src-tauri/src/bridge/events.rs:154-200` `compose_scan_progress(...)` emitting `stage`, `stageProgress`, `overallProgress` (terminal → 1.0), `filesFound`, `dataFoundBytes`, `badSectorMap`, `etaSeconds`, `throughputBytesPerSecond`, `recoverableDuringScan`, `currentSector` (real byte-offset cursor, `currentPath` always null per G-2).
- Contract vocabulary: `ui/src/connector/contract.ts:173-182` — `ScanStage = 'volume-analysis' | 'quick-index' | 'deep-walk' | 'carve' | 'tree-build' | 'done'`; `ScanPhase = 'quick' | 'advanced' | 'organizing'`; `scanPhaseOf()` maps `quick-index|volume-analysis → quick`, `deep-walk|carve → advanced`. Automatic transition confirmed at `contract.ts:187-189`: "The scan ALWAYS runs quick -> advanced automatically (ADR-008): the reference exposes no mode selection." — this matches REFERENCE-SCAN's quick→advanced AUTOMATIC claim on the product side too.

So: 3 weighted stages (40/35/25) exist end-to-end (engine struct → bridge scan_create → contract event), and the numeric weights are NOT in `contract.ts` (grep for `40.*35.*25` in `contracts/` only matched an unrelated note about "the mock's 0.4 threshold" at `contracts/type-maps.md:50`).

## V3 DiskHealth.exe separate-surface verification

**Verdict: VERIFIED that DiskHealth.exe is a separate, Qt 5.15.2 QML application with CrystalDiskInfo lineage (RTTI `CAtaSmart`) and a schtasks ONLOGON autostart trio; PARTIAL on REFERENCE-SMART's specifics — the 840x590 window size and a `diskInfoMgr` symbol were NOT found as strings, and the refresh interval is UNVERIFIED.**

File: `/home/z/my-project/easeus-drw-research/og-codebase/diskhealth/DiskHealth.exe` — `PE32 executable for MS Windows 6.00 (GUI), Intel i386, 8 sections`, 1,781,480 bytes.

QML evidence (all quoted exactly):
- Embedded QML file names (UTF-16 strings): `main.qml`, `CautionWidget.qml`, `InfoList.qml`, `InfoItem.qml`, `MultiLanguage.qml`, `Theme.qml`, `Title.qml`, `RotatingWait.qml`, `HoverWidget.qml`, `EuDefaultRoundButton.qml`, `EuRoundedGradientRectangle.qml`, `EuBuyNowBtn.qml`, `EuTreeView.qml`, `EuRadioButton.qml`, `EuPageLoader.qml`, ... (an "Eu*" QML widget family).
- `DiskHealth.rcc` (444,492 B) contains readable QML source: `import QtQuick 2.14`, `import QtQuick.Window 2.2`, `import QtQuick.Controls 2.14 as NEW`, `import QtQuick.Controls 1.4`.
- Static import table (`objdump -p ... | grep "DLL Name"`): KERNEL32, USER32, ADVAPI32, SHELL32, ole32, OLEAUT32, **Qt5Widgets.dll, Qt5Gui.dll, Qt5Qml.dll, Qt5Network.dll, Qt5Core.dll**, MSVCP140, mfc140u.dll, CFGMGR32, SETUPAPI, VERSION, WTSAPI32, VCRUNTIME140, api-ms-win-crt-*. Nuance: **Qt5Quick.dll is NOT in the exe's static import table** — but the deployment ships it (Qt5Quick.dll 3.49MB, Qt5QuickControls2.dll, Qt5QuickShapes.dll, Qt5QuickTemplates2.dll, Qt5QmlModels.dll, Qt5QmlWorkerScript.dll, Qt5RemoteObjects.dll, Qt5Svg.dll + `Qt/`, `QtQml/`, `QtQuick/`, `QtQuick.2/`, `QtGraphicalEffects/` plugin dirs), consistent with Quick being loaded at runtime via the QML engine rather than linked directly.
- Qt version: `strings Qt5Core.dll | grep -oE "5\.15\.[0-9]"` → **`5.15.2`** — REFERENCE-SMART's "Qt 5.15.2" VERIFIED.

CrystalDiskInfo lineage (ASCII RTTI strings in DiskHealth.exe):
- `.?AVCAtaSmart@@` (the CrystalDiskInfo class), `.?AV?$CArray@UATA_SMART_INFO@CAtaSmart@@U12@@@@`, `.?AV?$CArray@UDISK_POSITION@CAtaSmart@@U12@@@@`, `.?AV?$CArray@UEXTERNAL_DISK_INFO@CAtaSmart@@U12@@@@`.
- Transport/enum vocabulary: `CSMIALL`, `CSMIARY`, `CSMISAS`, `NVME`, `NvmeMini`, `SCSIDISK`, `SmartNVMe`. Debug strings: `DO:GetDiskInfo pd=%d, sp=%d, st=%d, mt=%s`, `OK:GetDiskInfo - PhysicalDrive`, `OK:GetDiskInfo - Scsi`, `GetDiskInfo - FALSE0/1`.

Directory census: `diskhealth/` = **818 files, 55M (du)** — matches REFERENCE-FOOTPRINT's "diskhealth companion 818 files/52.8 MiB" (818 exact; 55 MB decimal ≈ 52.4 MiB, consistent with the 52.8 MiB figure up to block-size rounding).

schtasks autostart trio — VERIFIED, all three .bat files exist and were read in full:
- `addtask.bat`: `@echo off` / `chcp 437` / `schtasks.exe /create /f /sc ONLOGON /rl HIGHEST /tn %1 /tr %2`
- `deltask.bat`: `schtasks.exe /delete /f /tn %1`
- `querytask.bat`: `schtasks.exe /query /tn %1`
So autostart = a scheduled task created at logon, running with HIGHEST privileges; task name/target passed as `%1`/`%2` by the caller.

Single-instance strings: UTF-16 `EaseUS_DiskHealth` and `EaseUS_DiskHealthConsole` present in DiskHealth.exe (mutex/name candidates — see V4 for the DRWUI side).

NOT found this pass:
- `840` / `590` as strings in DiskHealth.exe (UTF-16 or ASCII) or as `width:`/`height:` literals in the readable `DiskHealth.rcc` QML — the 840x590 window size is plausibly a C++ immediate; UNVERIFIED statically here.
- `diskInfoMgr` — not a string in DiskHealth.exe, but **CONFIRMED as the QML context object** in the reverser-extracted `re-workspace/reversed/ui-resources/diskhealth/qml/main.qml` (see V4 for quotes). V3's strings-only method could not see it; corrected by V4 evidence.
- Refresh interval: `refresh|interval|timer` UTF-16 grep hits only icon names (`gicon_button_refresh_blue18.png`, `icon_button_refresh_gray18.png`); bare number strings `1394/2002/2004/2005/2280` are not interval-shaped. No poll-interval constant found — **UNVERIFIED, needs decompile-level pass or Windows runtime.**

## V4 DRWUI → DiskHealth launch path

**Verdict: VERIFIED that DiskHealth receives and parses a `diskHealthIndex=<n>` argument, uses single-instance mutex names `EaseUS_DiskHealth` / `EaseUS_DiskHealthConsole`, and closes to tray; PARTIAL on the DRWUI→DiskHealth direction — DRWUI.exe contains `DiskHealth.exe`, `/diskhealth/`, two registry keys and imports `QProcess::startDetached`, but the `diskHealthIndex=` construction site was NOT found inside DRWUI.exe strings (no DRWUI-side decompile is in the corpus; only DiskHealth was decompiled in ghidra-wave2).**

DRWUI.exe evidence (og-codebase binary + reversed strings dumps, ASCII; UTF-16 grep for "diskhealth" returned nothing):
- `strings DRWUI.exe | grep -i diskhealth` → exactly: `DiskHealth.exe`, `/diskhealth/`, `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\EaseUS\DiskHealth`, `HKEY_LOCAL_MACHINE\SOFTWARE\EaseUS\DiskHealth` (same 4 lines in `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:12994-12999`).
- Launch API: `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:36483` imports `?startDetached@QProcess@@SA_NAEBVQString@@AEBVQStringList@@@Z` (static `QProcess::startDetached(QString, QStringList)`) — a detached-launch mechanism is available in DRWUI.
- No `diskHealthIndex` string exists in DRWUI.exe (ASCII or UTF-16).

DiskHealth side (the receiver) — `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c`:
- `:233` and `:37772` — two sites build `QString::fromAscii_helper("diskHealthIndex=",0x10)` and `QString::indexOf(arg, ...)` over the QCoreApplication arguments `QList` — i.e. DiskHealth parses `diskHealthIndex=<n>` from its own command line (received argument, not self-constructed).
- `:72-74` — `DAT_0051f8e8 = L"EaseUS_DiskHealthConsole"; ... DAT_0051f8e8 = L"EaseUS_DiskHealth";` — the single-instance mutex/name pair (matches the same two UTF-16 strings found in the binary, V3).

Extracted QML (`re-workspace/reversed/ui-resources/diskhealth/qml/main.qml` — reverser-extracted resource, not in og-codebase):
- Title + frameless + size: `title:qsTr("EaseUS DiskHealth")`, `flags: Qt.Window | Qt.FramelessWindowHint | ...`, `width: blur === 0?840:856`, `height: blur === 0?590:606` (lines 15-16, 18) — **840x590 VERIFIED** (856x606 when the drop-shadow is enabled).
- Tray-resident close VERIFIED (lines 62-68):
  ```
  onClosing:
  {
      if (diskInfoMgr.isTrayExist())
      {
          close.accepted = false;
          window.hide();
      }
  }
  ```
- `diskInfoMgr` QML context object CONFIRMED (correcting V3's strings-based non-find — the name lives in QML source, resolved at runtime, so it is not an exe string): `diskInfoMgr.isTrayExist()`, `.showWindow()`, `.setWindowIcon()`, `.refreshDisks()`, `.refreshForPop()`, `.setVisible()`, `.showWindowShadow()`; plus `function toDiskIndex(index) { disks.currentIndex = index; }` — the likely QML end-point of the `diskHealthIndex=<n>` argument (select the Nth disk in the list on launch).
- `Component.onCompleted`: when launched hidden, calls `diskInfoMgr.refreshForPop()` instead of `refreshDisks()` — a pop-up/refresh-on-event path consistent with an always-resident companion.

Cross-check against REFERENCE-SMART: the launch-with-`diskHealthIndex=<n>`, mutex `EaseUS_DiskHealth`, and tray-resident close claims are all consistent with the evidence above; the only unproven link is DRWUI actually building that command line (its strings don't contain the literal, so it may be assembled at runtime — DRWUI decompile is not in the corpus).

## V5 Scan-stage vocabulary + search-DLL ABI/imports

**Verdict: VERIFIED that the status-bar vocabulary exists byte-exactly in DRWUI.exe (ASCII strings, not UTF-16) and in `language/English.qm` — with two corrections to REFERENCE-SCAN's rendition: the byte-exact template is `Advanced Scanning%1, Files Found:` (no space before the comma), and the sector string is `Reading Sector:` (capital S; no `X/Y` suffix in the string itself). Search-DLL ABI VERIFIED: NTFSSearchDLL exports exactly 2 factory functions, RAWSearchDLL exports 6. NOTABLE: RAWSearchDLL does NOT import any device-I/O API (no CreateFile/DeviceIoControl/ReadFile/Overlapped) — its KERNEL32 imports are sync primitives only, so the RAW searcher consumes already-opened/analysed data, not raw devices.**

Scan vocabulary — `strings -el DRWUI.exe` (UTF-16) returned NOTHING for these phrases; they are ASCII/UTF-8 Qt strings. Byte-exact hits in `re-workspace/reversed/DRWUI.exe/strings_ascii.txt`:
- `Quick Scanning` (x2), `Quick Scanning%1, Files Found:`, `Quick Scan Completed`, `Quick Scan Begin`
- `Advanced Scanning`, `Advanced Scanning%1, Files Found:`
- `Cloud Scanning%1, Files Found:`
- `Scan Paused%1, Files found:`
- `Reading Sector:` (two occurrences, capital "S")
- `language/English.qm` (strings) additionally shows: `Advanced Scanning`, `!Advanced Scanning%1, Files Found:`, `Cloud Scanning%1, Files Found:`, `Quick Scanning`, `Quick Scan Completed`, `Estimated time remaining: %1s`, `Files found`, `%1 files (%2)`, `%1/%2`, `%1 of %2`.
- No "Deep scan" / "Remaining time" literals — the og's phase word is "Advanced", and remaining time is `Estimated time remaining: %1s`. REFERENCE-SCAN's `Advanced Scanning , Files Found: N (GB)` grammar is therefore PARTIAL on byte-exactness: real template `Advanced Scanning%1, Files Found:` with GB likely assembled via separate `%1 files (%2)`-style templates; and `Reading sector: X/Y` is really `Reading Sector:` plus an X/Y assembled at runtime (the `%1/%2` template exists) — the X/Y composition itself is UNVERIFIED statically.

Search-DLL ABI (export tables via `objdump -p`):
- `NTFSSearchDLL.dll` — exactly 2 exports (ordinal base 1): **`EUCreateNTFSQuickSearcher`** `[0]`, **`EUCreateNTFSSearcher`** `[1]` — a quick/full searcher factory pair, confirming the two-pass NTFS design in the ABI itself.
- `RAWSearchDLL.dll` — 6 exports: **`EUCreateRAWSearcher`**, **`EUCreateRAWSearcherForCellTest`**, **`EUGetAnalyserList`**, **`EUGetAnalyserMap`**, **`EUGetAnalyserMultimap`**, **`GetRawUserInfoMap`** — a creator plus analyser-list/map/multimap accessors and a user-info map getter.
- RAWSearchDLL imports: `MSVCP140.dll`, `WS2_32.dll`, `VCRUNTIME140.dll`, CRT api-ms-win-crt-*, `KERNEL32.dll`, `PubLog.dll`. The full KERNEL32 thunk list contains only runtime/synchronization functions (`CreateEventW`, `WaitForSingleObjectEx`, `SetEvent`, `ResetEvent`, `Enter/LeaveCriticalSection`, `Heap*`, `CloseHandle`, `QueryPerformanceCounter`, `GlobalMemoryStatusEx`, exception plumbing) — **no `CreateFileW`, no `DeviceIoControl`, no `ReadFile`, no OVERLAPPED**. Conclusion: sector access does not happen inside RAWSearchDLL; it must operate on buffers/handles provided by its caller (DRWUI-side or engine-side). WS2_32 in a disk-search DLL is an oddity worth a follow-up (possibly for license/telemetry sockets via PubLog).

## V6 FileFilter.xml + raw_signs.json structure

**Verdict: VERIFIED — FileFilter.xml has exactly 8 `<FileCategory>` elements and exactly 1489 `<Extention>` elements (note the og's misspelling "Extention"); raw_signs.json is a dict of 139 formats / 261 signatures with an `offset,hex-pattern,score` grammar and min/max size gates. No priority or hidden attributes exist in either file; the only non-descriptive attribute is `IsMutex="1"` on the "Other" category.**

`og-codebase/FileFilter.xml`:
- Root: `<xml RootName = "All types" Selected = "2" iconIndex = "-1">`.
- Exactly **8 categories** (`<FileCategory ...>`), quoted by iconIndex: `Graphic`(0, ShowComment `file.jpeg`), `Document`(1, `file.pmd`), `Email`(2, `file.eml`), `Audio`(3, `file.wav`), `Video`(4, `file.WMA`), `Other`(5, `file.dll`, **`IsMutex = "1"`**), `Archive`(6, `file.zip`), `Bookmarks`(7, `file.bookmarks`). Each carries `ShowComment`, `Description`, `Label`, `iconIndex`; no `priority`, no `hidden`.
- Extension element count: `grep -c "<.*[Ee]xt" FileFilter.xml` → **1489** — matches REFERENCE-SCAN's "8 categories/1489 ext" exactly. Element spelling in the file is `<Extention>` (sic). Selection state is an element (`<Selected>2</Selected>`, also at root and inside `<FileType>`), suggesting a tri-state (2 = all-selected).
- `<FileType>` groups nest inside categories with their own `Description` (e.g. `FileType Description="TIFF Graphics file(.tif.tiff.epx.nef.arw)"` containing 25+ `<Extention>` children including `.cr3`, `.heic`).

`og-codebase/raw_signs.json` (parsed with python json):
- Top-level **dict of 139 format keys** (sample keys: `itc2`, `cpr`, `flp`, `ifo`, `zdct`, `loggz`, `prproj`, `sample`, `master`, `map`, `plist`, `lrf`).
- Entry shape: `{"header": {"signs": [...]}, "tail": {"signs": [...]}, "min": <bytes>, "max": <bytes>}` — `tail` present in 46/139 formats.
- **261 total signatures** (199 header, 62 tail). Each sign string is `"<offset>,<hex pattern>,<score>"`, e.g.:
  - `"2,011c69746368000000020000000200000002000000006172747700000000,1.00"` (itc2 header, offset 2, score 1.00)
  - `"8,4e554e44524f4f54,0.54"` (cpr header — hex `4e554e44524f4f54` = ASCII "NUNDROOT")
- **Score range 0.5–1.0** — that is the quality/confidence field; there is no separate priority/ordering field. Size gates are per-format byte ranges (e.g. itc2: min 1, max 51200; cpr: 10240–204800).

## V7 Census spot-check

**Verdict: VERIFIED exactly for the whole-capture and diskhealth figures (1470 files / 670.3 MiB apparent; diskhealth 818 files / 52.8 MiB apparent). The "live tree 1,276 files / 518.7 MB" sub-claim is UNVERIFIED — no directory in this sandbox matches those numbers, so it must rely on a filter definition (which files the live product actually touches) that REFERENCE-FOOTPRINT did not make reproducible here.**

Measured in `/home/z/my-project/easeus-drw-research`:
- `find og-codebase -type f | wc -l` → **1470** (REFERENCE-FOOTPRINT: 1,470 — exact match).
- `du -sb og-codebase` → **670.3 MiB apparent size** (`du -sh` reports 660M because of block-size rounding; the apparent-size figure reproduces the sheet's 670.3 MiB exactly).
- `find og-codebase/diskhealth -type f | wc -l` → **818**; `du -sb og-codebase/diskhealth` → **52.8 MiB** — both exact matches for "diskhealth companion 818 files/52.8 MiB".
- Cross-check of other candidate trees: `final-prod/` = 3,217 files / 112.9 MiB apparent (that is the rebuilt product, not the reference); `re-workspace/` = 9,956 files; `architecture/` 9; `scripts/` 50. None is 1,276 files / 518.7 MB. If "live tree" means "subset of og-codebase the running product loads" (1,470 − 1,276 = 194 excluded files; 670.3 − 518.7 = 151.6 MiB excluded), the exclusion rule is not reproducible from the sheet — flagged for the footprint owner to document.

## V8 Health-grading decode cross-check

**Verdict: VERIFIED — the cited artifact exists and is byte-identical to the state the s6 document was written against (git diff empty between cited commit `03ad46c` and HEAD); the 4-state enum and the NVMe life rule reproduce exactly in the decompile. One flag: a few of the s6 line citations are loose ranges (see below), though every claim they support is real at a nearby line.**

Artifact: `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` — exists, **93,316 lines** (matches the doc's "93316 lines" exactly). Citation-stability check: `git -C easeus-drw-research diff --stat 03ad46c HEAD -- <file>` → empty (file unchanged since the cited commit `03ad46c "errata: CORRECTIONS.md - 8 RE-output corrections..."`); current blob `750931a8`.

Claim-by-claim:
- **4-state enum VERIFIED**: the status list is built at exactly the cited lines — `:37087-37100` shows `QString::fromAscii_helper("Caution",7)`, `("Good",4)`, `("Unknown",7)` (plus `"Bad"` just above) pushed into a QList. The grader `FUN_0040cdf0` returns plain ints: Unknown gates `return 0` (`:5848-5856` region), aggregation `if (0 < local_1c) return 3;` (bad_count>0 → Bad), `if (bVar5) return 2;` (caution), graded-else-Good — all read directly.
- **NVMe life rule VERIFIED byte-level**: conversion at `:27000-27005`: `*(int*)(... + 0x17e0 + uVar21) = 100 - (int)piVar12;` (percentage_used → life), with the `< 0 → = 0` clamp immediately after. Grading at `:5823-5847`: model `L"Parallels Virtual NVMe Disk"` → `return 1` (Good); critical-warning byte at +0x1405 nonzero → `return 3` (Bad); then `10 < life(0x17e0) → return 1` (Good), `== 10 → return 2` (Caution), `< 10 → return 3` (Bad). This is exactly the claimed "100−pct_used; >10 Good, =10 Caution, <10 Bad".
- Call sites cited as `:26960-26962, :27143-27144` — grep confirms `thunk_FUN_0040cdf0(this,(int)param_1)` at **:26960 and :27143**, exact.
- **Citation looseness (flag, not a refutation)**: the doc cites `FUN_0040cdf0 ... :5755-6130`, but the function actually begins at `:5790` (`grep -n "FUN_0040cdf0"` → definition at 5790); the NVMe branch cited as ":5795-5810" actually sits at `:5823-5847`. Since the file is byte-identical to the cited commit, these particular ranges are approximate rather than drifted. All other spot-checked citations (`:37087-37100`, `:26999-27005`, `:26960`, `:27143`) are exact.
- Product-side note (ties to V1): the engine's `cr_family.cpp` family table mirrors the s6 vendor cascade (JMicron 60x/66x, Samsung "SAMSUNG"/"MZ-", etc.), so the decode is what the product classifier was built from.

## V9 Sibling-agent open questions (attempted subset)

Attempted 2 of the suggested questions (budget discipline; both are load-bearing for REFERENCE-SMART).

### V9a USB bridge quirk ladder (REFERENCE-SMART §"USB-bridged")

**Verdict: VERIFIED (existence and shape), PARTIAL on the exact branch count (23 distinct `AddDisk - USB*` strings observed vs "28 numbered branches" claimed).**

Critical method note: the ladder strings are **UTF-16 only** — an ASCII `strings` grep finds NOTHING for `Sunplus|Cypress|0x152d|Genesys`, which is why the first probe came up empty. `strings -el DiskHealth.exe` yields:
- `AddDisk - USB0A`, `AddDisk - USB0B`, `AddDisk - USB10`, `AddDisk - USB11`, `AddDisk - USB12`, `AddDisk - USB13`, `AddDisk - USB14`, ... — **23 distinct `AddDisk - USB*` strings** (`strings -el | grep -c "AddDisk - USB"` → 23), plus `AddDisk - ASM1352R` and `AddDisk - Si`.
- A literal condition string: `(usbVendorId == USB_VENDOR_IO_DATA && productId == 0x0122)`.
- The WMI association query: `ASSOCIATORS OF {Win32_USBController.DeviceID="%s"} WHERE AssocClass = Win32_USBControllerDevice`.
- The decompile contains 30 `AddDisk` occurrences total. REFERENCE-SMART's "28 numbered branches AddDisk - USB0A … AddDisk - USB20" is not exactly reproducible from distinct strings (23); the 28 may count decompile branch sites or include duplicates — flagging the number, not the substance. ` USB Device` (leading space) is also present.

### V9b The 6 vendor NVMe routes (REFERENCE-SMART open question #4)

**Verdict: VERIFIED that all 7 route names exist as strings in DiskHealth.exe; the IOCTL details per route remain UNVERIFIED (needs decompile reading, out of budget).**

`strings -el DiskHealth.exe | grep CMD_TYPE_NVMe` → all seven, byte-exact:
`AddDiskNVMe - CMD_TYPE_NVME_STORAGE_QUERY`, `AddDiskNVMe - CMD_TYPE_NVME_INTEL_RST`, `AddDiskNVMe - CMD_TYPE_NVME_SAMSUNG`, `AddDiskNVMe - CMD_TYPE_NVME_INTEL`, `AddDiskNVMe - CMD_TYPE_NVME_JMICRON`, `AddDiskNVMe - CMD_TYPE_NVME_ASMEDIA`, `AddDiskNVMe - CMD_TYPE_NVME_REALTEK` — i.e. the generic STORAGE_QUERY route + exactly the 6 vendor routes REFERENCE-SMART lists. Function-name strings also present: `DoIdentifyDeviceNVMeASMedia`, `DoIdentifyDeviceNVMeIntel`, `DoIdentifyDeviceNVMeIntelRst`, `DoIdentifyDeviceNVMeJMicron`, `DoIdentifyDeviceNVMeRealtek`, ... (ASCII family names `SmartNVMe`, `SmartRealtek`, `SmartSamsung`, `NvmeMini`, `LSILOGIC` also present — see V3). Not resolved this pass: which IOCTL each vendor route issues and which log pages each reads (REFERENCE-SMART open question #4 stays open).

### V9c 'Reading sector' X>Y semantics (not attempted)

Skipped — grep of reversed `*.md` notes for "Reading sector" returned nothing, and the string-level facts are already recorded in V5 (template `Reading Sector:` + `%1/%2`); the runtime composition needs a session/trace, out of budget.

## 10 NOT verified this pass

Honest list of what this pass could not settle, with the reason and what would be needed:

1. **DiskHealth refresh/poll interval** — no interval-shaped constant or `refresh|interval|timer` string beyond icon names; needs a decompile read of the QTimer setup in DiskHealth_decompiled.c or a Windows runtime trace.
2. **840x590 as seen by strings in DiskHealth.exe itself** — VERIFIED instead via the reverser-extracted QML (`re-workspace/reversed/ui-resources/diskhealth/qml/main.qml:15-16`, `width: blur === 0?840:856`); the C++ side never states it, so the extraction is the only static source. If the QML extraction's provenance is ever questioned, this reverts to UNVERIFIED.
3. **DRWUI actually constructing the `diskHealthIndex=<n>` command line** — DRWUI.exe has `DiskHealth.exe`, `/diskhealth/`, registry keys and imports `QProcess::startDetached`, but the literal `diskHealthIndex` appears only on the DiskHealth (receiver) side; DRWUI has no decompile in the corpus, so the construction site is unproven.
4. **`Reading Sector:` X/Y runtime composition** — the string and a `%1/%2` template exist, but how X and Y are filled (and whether X can exceed Y) needs a live session or decompile of DRWUI (not in corpus).
5. **`Advanced Scanning , Files Found: N (GB)` exact rendering** — byte-exact template is `Advanced Scanning%1, Files Found:`; the "(GB)" suffix assembly and the space-before-comma in the sheet's rendition are not byte-exact; needs a UI session to confirm the final rendered string.
6. **The 6 vendor NVMe routes' IOCTL details** (which IOCTL, which log pages) — route names verified (V9b), payloads not read.
7. **USB quirk ladder branch payloads** (what each `AddDisk - USBxx` branch changes) and the exact branch count (23 strings vs "28 branches" claimed).
8. **REFERENCE-FOOTPRINT "live tree 1,276 files / 518.7 MB"** — no tree in the sandbox matches; the exclusion rule behind "live" is undocumented (V7). Needs the footprint owner to publish the filter.
9. ~~REFERENCE-SMART's WMI-fallback claim~~ — **resolved after the list was drafted**: `strings -el DiskHealth.exe` DOES contain the WMI SMART fallback: `SELECT * FROM MSStorageDriver_FailurePredictData`, `SELECT * FROM MSStorageDriver_FailurePredictThresholds`, `\\.\root\WMI`, plus init/debug strings `CAtaSmart::Init WMI on - Start` / `CAtaSmart::Init WMI off - Start`, `OK:WMI Init`, `NG:WMI Init 1/2/3`, `EX:WMI Init`. REFERENCE-SMART's WMI-fallback transport is VERIFIED on the reference side; V1 already established the product engine deliberately does NOT implement it (comment-only at `cr_health.h:21`).
10. **Anything requiring execution** — all evidence here is static; no corpus binary was run (no Windows/wine in sandbox).
