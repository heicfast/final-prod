# REFERENCE-BEHAVIOR — what the EaseUS DRW ("og") corpus documents for each reported defect

> Agent E (T2-E) deliverable. Every claim below carries a corpus citation `path:line` relative to
> `/home/z/my-project/easeus-drw-research/` (shortened: `artifacts/` = `re-workspace/artifacts/`,
> `rev/` = `re-workspace/reversed/`). Items the corpus does NOT document are explicitly marked
> **corpus-silent**. Reported defects: (1) no elevation at launch; (2) console popups on disk
> health; (3) incomplete disk enumeration (2 internal + 1 SD, only one shows); (4) scan stuck at
> 75% "Advanced Scanning", "Files Found: 21 (0 B)", frozen sector progress; (5) residual
> elevation-prompt visual standard.

---

## 1. Corpus orientation — what lives where

| Corpus path | Contents | Authority for |
|---|---|---|
| `og-codebase/` | Original shipped binaries (DRW.exe, DRWUI.exe, DRWService.exe, SetupUE.exe, 9 `*SearchDLL.dll`, `PartitionRecovery/` .mo modules, `diskhealth/DiskHealth.exe`, configs, vendor logs `EPMLOG.log`, `log/DRWUI.log`) | Raw ground truth |
| `re-workspace/reversed/` | Per-binary rabin2/strings/pefile output + Ghidra decompilations (`ghidra-drwui/DRWUI_decompiled.c` = 2.38M lines, 119,182 fns) | Byte-level behavior (cited as `rev/ghidra-drwui/DRWUI_decompiled.c:LINE`) |
| `re-workspace/artifacts/` | 254+ curated deep-dive .md docs, line-cited against the decompilations | The most authoritative narrative layer |
| `re-workspace/raw-evidence/` | ui-screenshots/ (atlas, 7 areas), ui-ground-truth-* (dynamic runs: censuses, PNGs, logs) | Pixel/runtime ground truth |
| `architecture/` | 9 cross-linked master docs (00-INDEX … 07-user-flow-uiux) | Synthesis index (`architecture/00-INDEX.md:5-16`) |

Repo layout table: `README.md:9-17`; scale of extraction: `README.md:29-35`.

**Most authoritative docs per topic**:
- UI behavior → `architecture/07-user-flow-uiux.md` (00-INDEX.md:15), `artifacts/UIUX/01…35` series, `architecture/05-uiux-architecture.md` (00-INDEX.md:16)
- Scan engine → `architecture/05-scan-engine-architecture.md` (00-INDEX.md:13), `architecture/04-recovery-workflow.md` (00-INDEX.md:12), `artifacts/SearchDLL/*`
- Privilege/elevation → `artifacts/DRW-exe--launcher.md`, `artifacts/DRWService-exe--bitlocker-service.md`, `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`, PE manifests in `rev/DRW.exe`, `rev/DRWUI.exe`
- Device enumeration → `artifacts/PartitionRecovery/mom-module-system--architecture.md`, `artifacts/MOM-modules/{Device,DeviceManager}.md`, `artifacts/UIUX/24-dynamic-ground-truth-ingestion.md` §6, vendor logs `og-codebase/EPMLOG.log`, `og-codebase/log/DRWUI.log`
- Disk health → `artifacts/DiskHealth/diskhealth--smart-companion.md`, `artifacts/UIUX/19-diskhealth-quick-ui.md`, `rev/ghidra-drwui/DRWUI_decompiled.c:178673-178790`
- Scan-phase runtime → `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md` (GT-scan doc), `artifacts/UIUX/30-interaction-driver-ground-truth.md`, `raw-evidence/ui-screenshots/03-scan-phase/` (incl. `scan-midscan-210s.png`)

---

## 2. Privilege model — how the reference elevates (defect 1)

**Corpus documents: the product is elevated at launch by manifest, not by an in-app prompt.**

1. **DRW.exe (the launcher, i.e. the product entry point) carries `requireAdministrator`** in its
   embedded manifest: `rev/DRW.exe/strings_ascii.txt:355` (rabin2 raw: `rev/DRW.exe/rabin2_strings_data.txt:1183`
   — `<requestedExecutionLevel level='requireAdministrator' uiAccess='false' />`).
2. **DRWUI.exe (the main UI, the only window-owning process) also carries `requireAdministrator`**:
   `rev/DRWUI.exe/strings_ascii.txt:41023` (raw: `rev/DRWUI.exe/rabin2_strings_data.txt:1728`).
   Either launch path (DRW.exe → DRWUI.exe, or DRWUI.exe direct) therefore triggers OS UAC.
3. **Launch chain**: DRW.exe is a thin supervisor that `CreateProcessA("DRWUI", inheritHandles=TRUE)`
   (`artifacts/DRW-exe--launcher.md:31`, ledger `:107`) and supervises crash events + a 2 MB shared
   log mapping (`artifacts/DRW-exe--launcher.md:13-17,68-75`). DRWUI launched standalone **does not
   render UI and exits** (verified 3 environments — `artifacts/UIUX/dynamic-ground-truth--window-census-verification.md:36-40`);
   the loader path is the production launch.
4. **In-process privilege adjustment**: DRWUI imports `LookupPrivilegeValueA/W` +
   `AdjustTokenPrivileges` (`rev/DRWUI.exe/strings_ascii.txt:39520-39523`) — the app adjusts its own
   token privileges after the manifest-guaranteed elevation; it never re-prompts.
5. **Privileged helper as service, not as spawned elevated process**: `DRWService.exe` ("Read FVE
   Server") is installed `SERVICE_AUTO_START` (`CreateServiceA`, start type 2 —
   `artifacts/DRWService-exe--bitlocker-service.md:12-19`) and "Runs elevated so a standard-user DRW
   instance can obtain the keys without UAC" (`artifacts/DRWService-exe--bitlocker-service.md:5`).
   I.e. the architecture avoids mid-session UAC prompts by doing privileged work in-process
   (already elevated) or via a LocalSystem service.
6. **Installer**: `SetupUE.exe` + `InfoForSetup.exe` are the pre-install bootstrap (16-OS gate,
   `SOFTWARE\EASEUS\DRW\Version` registry contract, `/SendInfo` telemetry), handing off to an
   **Inno Setup** wizard (`unins000.{exe,dat,msg}` trio) —
   `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:11-16,27-35`. Both bootstrap EXEs
   carry `asInvoker` manifests (`rev/SetupUE.exe/strings_ascii.txt:354`;
   `rev/InfoForSetup.exe/strings_ascii.txt:396`). **Corpus-silent** on the Inno
   `PrivilegesRequired` directive (how the wizard itself elevates).
7. **In-app privilege-failure copy exists** (remote/privileged-account contexts, not elevation
   prompts): "Account without sufficient permission. Try with your Administrator account."
   (`rev/DRWUI.exe/qt_strings_english.txt:142`), "Administrator account required" (`:161`),
   "Password of Administrator" (`:831`).
8. **No elevation dialog exists in the modal inventory**: the 226-site dialog census lists every
   exec-capable dialog class; none is an elevation/consent prompt
   (`artifacts/UIUX/06-dialog-telemetry-inventory.md:6-31`). **Corpus-silent** on UAC-dialog
   visuals themselves (OS surface; the dynamic runs executed on CI where no UAC dialog appears —
   `artifacts/UIUX/dynamic-ground-truth--landing-page-windows.md:9-17`).
9. **WinPE**: same DRW.exe is launched by `winpeshl.ini` in the boot environment
   (`artifacts/DRW-exe--launcher.md:89-90`; `architecture/03-winpe-boot-environment.md` per
   00-INDEX.md:11) — PE runs as SYSTEM, no UAC in that path.

**Net standard for defect 1**: launch through a `requireAdministrator`-manifested entry EXE; the
whole process tree (loader + UI + in-proc engines) is elevated from process creation; no
per-feature elevation; privileged background work via an AUTO_START LocalSystem service.

---

## 3. Device enumeration — the reference's multi-stack fan-in (defect 3)

**Corpus documents a five-source fan-in, all merged into one device list:**

1. **WMI (`ROOT\CIMV2`)** — DRWUI side: `CoInitializeSecurity` + `CoCreateInstance` + connect
   `ROOT\CIMV2` + `CoSetProxyBlanket` (`rev/ghidra-drwui/DRWUI_decompiled.c:259840-259880`, cited
   `artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:112`); runtime wbemprox ConnectServer ×2
   (`artifacts/UIUX/dynamic-ground-truth--wine-loader-run.md:37`). Device.mo has its own WMI/SWbem
   helpers (10 blanket sites — `24-dynamic-ground-truth-ingestion.md:112`).
2. **SetupAPI + IoCtl** — "DeviceManager.mo enumerates physical disks/volumes (WMI + setupapi +
   IoCtl)" (`artifacts/PartitionRecovery/mom-module-system--architecture.md:93`); DeviceManager.mo
   role = "Enumeration, hot-plug notify, WMI queries" (`:58`).
3. **mountmgr IOCTLs** — `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` ×4 + storage-property query +
   disk geometry (0x7405C/0x4D004/0x70048) at startup
   (`artifacts/UIUX/dynamic-ground-truth--window-census-verification.md:54`;
   `--wine-loader-run.md:41`). "WMI-first enumeration — DRWUI's device enumeration issues WMI +
   mountmgr queries in the first seconds, BEFORE any scan starts" (`--wine-loader-run.md:60`).
4. **VDS** — `CDiskEnumerator : Vds loadservice` in Device.mo: `CoCreateInstance(...,4,...)`
   (CLSCTX_LOCAL_SERVER), gated `dwMajorVersion >= 6`
   (`rev/ghidra-mo/Device_decompiled.c:51709-51719`; corrected attribution
   `artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:111`). Vendor QA log shows the call firing:
   `og-codebase/EPMLOG.log:5` ("CDiskEnumerator : Vds loadservice end.").
5. **Storage Spaces (Windows Storage mgmt)** — Device.mo carries the MSFT_* WMI storage-class
   strings `\\.\ROOT\microsoft\windows\storage:MSFT_{Disk,PhysicalDisk,StoragePool,StorageSubSystem,VirtualDisk,Volume}`
   (`artifacts/MOM-modules/Device.md:45-50`); runtime log: "Storage space init start/end" +
   "Storage space GetUsedPhysicalDisks end." (`og-codebase/EPMLOG.log:2-4`).
6. (**+FunctionDiscovery**) — `CMyFDHelper` / `CFunctionDiscoveryNotificationWrapper`,
   `CoCreateInstance(CLSID {c72be2ec-…}, 0x17)` — Windows **Function Discovery**, failure treated
   as soft (`artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:110`).

**Runtime merge order (vendor QA log, real machine, 2026-10-14)** —
`og-codebase/EPMLOG.log:1-19`:
```
CDeviceManagerBuilder : pDiskEnumerator->GetDiskEntries start.
CDiskEnumerator : Storage space init start/end → GetUsedPhysicalDisks end.
CDiskEnumerator : Vds loadservice end. → disk enumerate end.
CDeviceManagerBuilder : pDiskEnumerator->GetDiskEntries end.,4      ← 4 physical disks merged
CDeviceManagerBuilder : CreateDiskManagerItem end. → CalcDriveLetter end. → UpdatePartitionInformation start.
```
DRWUI-side enumeration: `EnumCollectPartition::{EnumDevice,EnumDiskDevice,RemoveCache}` traces
("Enum disk begin/end") — `artifacts/DRWUI-exe--main-ui-architecture.md:225-226`;
`og-codebase/log/DRWUI.log:420` ("run to EnumCollectPartition::EnumDevice 100").

**What devices appear & how they are classified (kind axes)** — vendor QA log is explicit on
internal vs SD/removable on a machine with 1 internal disk + 1 SD reader:
- `PE device name=ST500DM002-1BD142, F:, G:, H:, I:` (internal Seagate 500 GB, 4 volumes) and
  `PE device name=Generic-SD/MMC/MS PRO, E:` (SD reader) — `og-codebase/log/DRWUI.log:5-6`.
- **Removable/camera classification with a size threshold**: `nMaxRemovebleDiskSize=549755813888`
  (~512 GiB) compared against each candidate; `Add to device list, name= 02-1BD142 ,
  size=500107862016` (internal disk) and `Insert Camera Device, name= SD/MMC/MS PRO ,
  size=62607327232` (the SD card added via `CameraDeviceManager::AddCameraDevice`, diskIndex 2/3),
  `Judge finish SD/MMC/MS PRO type=12`, then `GetCameraDeviceCount/GetCameraDevice` —
  `og-codebase/log/DRWUI.log:20-32` (repeat passes `:176-188`, `:341-353`, `:607-611`).
- **Icon/kind system = 50 device-type codes** in `og-codebase/Resource/ImageResource.xml`:
  0-5 system (±BitLocker lock/unlock), 6-13 USB (existing/lost/lock/unlock), 14-21 other-disk
  partitions, 22-25 whole disks, 26-29 cameras, 30 UAV, **31-32 SD cards**, 33 recycle bin, 34
  folder, 35 generic USB, 36 HD, 38-41 dashcams —
  `artifacts/UIUX/01-visual-inventory.md:70-89` (asset dirs `DriveTypes/` 60 icons `:54`,
  `SDCard/` 29 assets `:60`).
- **Grouping on the home/landing screen**: "Internal drives (%1)" and "External drives (%1)"
  group headers with live counts (`rev/DRWUI.exe/qt_strings_english.txt:636` and `:470`);
  captured live as "─ Internal drives (2) ─" with device cards showing label `Windows(C:)` +
  used/capacity "30.96 GB/149.45 GB" + used-space bar
  (`artifacts/UIUX/dynamic-ground-truth--landing-page-windows.md:29-33,63`); "Show all external
  devices" expander (`qt_strings_english.txt:1179`); dead-end affordance "Drive Not Show Up"
  (`artifacts/UIUX/07-home-phase-deep-dive.md:34`).
- **Per-row home list content**: icon (type map) + drive letter + label + file system + capacity
  bar + **TRIM chip** + per-row Scan button (`artifacts/UIUX/07-home-phase-deep-dive.md:17`);
  whole-disk rows use type 22-25 `drivetype_ico_disk` (`:46`).
- **SD cards additionally get a dedicated sidebar flow**: "SD Card" page "Connect your SD card to
  PC/Laptop with a card reader to recover the deleted, formatted and other lost photos and videos
  from cameras, UAV, dash cams and monitors, etc." (`qt_strings_english.txt:318`;
  screenshot `raw-evidence/ui-screenshots/02-landing/landing-sidebar-sdcard-page.png` per
  `raw-evidence/ui-screenshots/README.md:26`), plus not-detected coaching ("SD card connected but
  not detected?", "Try to do as follows if SD card was not detected:" — `qt_strings_english.txt:1068-1069,1351-1352`)
  and "If your digital camera didn't show up, take SD card out and insert it into a card reader."
  (`:605`).
- **Hot-plug**: DeviceManager.mo "hot-plug notify" (`mom-module-system--architecture.md:58`);
  `CDevNotifyManager`/`CDevNotifyWnd` WM_DEVICECHANGE window (`:83`); auto-refresh security pause
  copy ("For security reasons, auto-refresh is paused. Please refresh manually to confirm new
  devices." — `qt_strings_english.txt:515`); disconnection retry coaching "Remove all external
  devices" (`:992-993`). Refresh = modal progress flow (`07-home-phase-deep-dive.md:57`).

**Net standard for defect 3**: internal disks AND SD/removable devices are enumerated from
multiple stacks, merged by `CDeviceManagerBuilder`, and ALL land in the visible device list —
internal under "Internal drives (N)", SD/removable under "External drives (N)" / the SD Card
flow — with size-threshold-based camera/removable classification. A machine with 2 internal + 1
SD must show all 3.

---

## 4. Scan flow + progress semantics (defect 4)

**Two-phase model (corpus-documented statically + dynamically):**
1. **Quick scan** = single FS-metadata engine chosen by FS code (0-2 FAT, 3 NTFS, 4-5 EXT2,
   7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 13 ZFS, +Btrfs) via `EUCreate<FS>QuickSearcher`
   (`architecture/04-recovery-workflow.md:56-60`; `artifacts/DRWUI-exe--main-ui-architecture.md:239-253`).
2. **Advanced/deep scan** = ALL full searchers in parallel (`EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher`)
   + `EUCreateRAWSearcher` carving (settings-gated +0x60) —
   `architecture/04-recovery-workflow.md:61-65`; "Advanced scan = ALL FS engines in full mode +
   RAW carving engine, in parallel" (`DRWUI-exe--main-ui-architecture.md:261`).

**Transition (quick → advanced), with exact log evidence**: vendor QA log shows
`ScanThread type=0` (quick) at 22:58:01 → `CDoScanPartition::ScanThread:483` +
`UpdateScanFinishedInfo:498` at 22:58:22 (**quick pass ≈21 s**) → status-bar controller flips
`bottomarea.cpp:838/856, nStatus=2, m_iScanType=1→2` → `ScanThread type=1` (deep) →
`Insert lost partition … StartSector=0, TotalSector=270977023` —
`og-codebase/log/DRWUI.log:57-70` (timing corroboration:
`artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md:138`). Both scan ranges are set up
front: `Quick scan StartSector=705794048, TotalSector=270977023` / `Deep scan StartSector=…,
TotalSector=…` (`og-codebase/log/DRWUI.log:48-49`). Quick-complete upsell copy: "Quick Scan
Completed" / "Recover Found Deleted %1 files %2" / "Tips: Advanced Scan is trying to find more
other lost files for you while recovering…" (`artifacts/UIUX/08-scan-phase-deep-dive.md:36`);
"Advanced Scan is searching files sector by sector to ensure data recovery as much as possible."
(`artifacts/DRWUI-exe--main-ui-architecture.md:136-137`).

**Status-line formats (shipped strings, verbatim)** — `rev/DRWUI.exe/qt_strings_english.txt`
(raw ascii equivalents `rev/DRWUI.exe/strings_ascii.txt`):
- "Quick Scanning %1, Files Found:" `:898` (`strings_ascii.txt:10314`)
- "Advanced Scanning %1, Files Found:" `:172` (`strings_ascii.txt:10318`)
- "Cloud Scanning %1, Files Found:" `:296`
- "Reading sector:" `:913`; "Remaining time:" `:979`
- "Searching:" `:1139` (live path ticker); "%1 file chunks scanned..." `:30`
- "Scan Completed, Files Found:" `:1095`; "Scan Stopped, Files Found:" `:1101`
- "Scan Paused %1, Files found:" `:1098`; "Scanning Paused." `:1120`
- "Organizing Found Files…" `:812-813`; "Stop Searching for Lost Data?" `:1231`
- Composite runtime-verbatim (GT-scan addendum): **"Advanced Scanning , Files Found: 1139157
  (143.74 GB)"** and **"Remaining time: 07m43s | Reading sector: 134316032/31413598"** —
  `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md:102`. So the count **and the
  byte-size both render and update live** in the status line.

**Progress model**: advanced-scan percent is **sector-position-based and advances linearly** —
runtime captures: 9% @ t=75 s, 34% @ t≈460 s, complete @ t=880 s (14.7 min total on a 149 GB
volume; ≈98.5% capacity enumerated) — `dynamic-ground-truth--scan-phase-captured.md:88-96,111-138`.
"The remaining-time estimate (7m43s at 34%) implies a linear sector-throughput model — matches
the sector-based progress decoded in the engines" (`:106`). **Corpus documents no explicit
quick/advanced phase-weight formula** for the scan % (the only percent-weighting found is the
.rsf *save*-progress mapping 40%/25%/remainder — `artifacts/SaveScan/rsf-record4--tree-node-grammar.md:100`,
`rsf-format--scan-session.md:367`; and splash progress sustaining 60-70% during enumeration —
`artifacts/UIUX/dynamic-ground-truth--splash-captured.md:75`).

**Live found-file counting**: "at capture time the progress circle shows 'Advanced Scanning, 9%'
with 1.11M files already found — the Quick scan pass had already completed" and the count grows
during the advanced pass (`dynamic-ground-truth--scan-phase-captured.md:56`); the **Reconstructed
branch grows 1→16 during 6 minutes of advanced scanning** (`:94-95`), then 16→87 by completion
(`artifacts/UIUX/30-interaction-driver-ground-truth.md:26`). Browse-while-scanning is fully
interactive (tree populated, list browsable, preview live) (`dynamic-ground-truth--scan-phase-captured.md:57`).
Engine-side counters keep ticking in the vendor log during deep scan ("Quick scan matched=843→5682,
total=9423" — `og-codebase/log/DRWUI.log:71-98`). 3-branch tree roots: `Windows (C:)` /
`File Path Lost` / `Reconstructed` (`dynamic-ground-truth--scan-phase-captured.md:45-53`).

**Pause / resume / stop**: Pause (❚❚) and Stop (■) controls in the scan bottom bar
(`dynamic-ground-truth--scan-phase-captured.md:99`); paused state = "Scan Paused" + red
"Scanning Paused." strip + resume/stop buttons, resume returns to scanning
(`raw-evidence/ui-screenshots/README.md:38-39`); stop requires confirm modal `CScanCommonPopDlg`
"Stop searching for lost data?" with risk framing + files-found/used-time stats
(`artifacts/UIUX/08-scan-phase-deep-dive.md:28`; screenshot `README.md:40`); cancellation is
cooperative via `CStopScanThread` (`08-scan-phase-deep-dive.md:55`) and lands in
`CDoScanDeviceBase::DoStop` + `ClearResource` (`og-codebase/log/DRWUI.log:154-156`).

**Duration expectations**: quick pass "will take a few minutes to find deleted files and lost
partitions" (`qt_strings_english.txt:897`); "Quick Scanning may take more time to find lost files
due to larger volumes." (`:896`); deep scan runs to completion (~14 min for 149 GB —
`DRWUI.log:69→131` timestamps 22:58:25→23:12:31; `dynamic-ground-truth--scan-phase-captured.md:138`).
Completion pipeline (verbatim telemetry): `Start Freeze` → auto-save
`auto_<Volume>_<MM_DD_HH_MM_SS>.rsf` → `Unfreeze` → `CScanMgr::ScanFinished` →
`bottomarea.cpp:838/856 nStatus=4, m_iScanType=3→4` → recoverability pass
(`artifacts/UIUX/30-interaction-driver-ground-truth.md:56-96`; `og-codebase/log/DRWUI.log:131-147`).
Completion UI = standalone 540×380 celebration window: "Congratulations! Scan Completed!", stats
panels ("1139275 Files found" / "149.85 GB Size"), "Great news: Your files are highly
recoverable", blue "Go to Recover" (`30-interaction-driver-ground-truth.md:9-19`;
`dynamic-ground-truth--scan-phase-captured.md:116-127`).

**Net standard for defect 4**: at any advanced-scan %, the Files-Found count **and** size keep
growing, "Reading sector: X/Y" and "Remaining time: NNmNNs" keep updating, progress advances
linearly by sectors, and the UI stays browsable. A frozen ticker + "21 (0 B)" is a deviation.

---

## 5. Disk-health surface (defect 2 + SMART context)

1. **TRIM state probe (the fsutil discipline)** — fully decompiled:
   - Query entry `FUN_1400c91a0` runs `" behavior query disabledeletenotify"` and returns the
     cached global (0 when unset −1): `rev/ghidra-drwui/DRWUI_decompiled.c:178673-178684`.
   - Runner `FUN_1400c91d0` (`:178689-178763`): `GetSystemDirectoryA` + `wsprintfA("%s\fsutil.exe")`
     (`:178712-178713`), `CreatePipe` with inheritable handles (`:178726`), STARTUPINFO
     `dwFlags=0x101` (STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW) **`wShowWindow=0` (SW_HIDE)**
     (`:178730-178731`), `hStdOutput=hStdError=pipe` (`:178728-178729`),
     `CreateProcessA(…, bInheritHandles=1, 0x10, …)` (`:178732-178733`),
     `WaitForSingleObject(proc, 10000)` (`:178734`), `PeekNamedPipe`→`ReadFile(≤0x400)`
     (`:178738-178740`), then the parse: `StrStrA(out, "= ")` and `lpString[2]=='0'` →
     `DAT_141ba9514=1`, `=='1'` → `=0` (`:178742-178749`) — a **tri-state global (−1/0/1)**
     (`artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:113`). Setter `FUN_1400c9450`
     runs `" behavior set disabledeletenotify 0/1"` (`:178768-178790`).
   - It fires **twice at startup** (runtime-observed: `artifacts/UIUX/dynamic-ground-truth--wine-loader-run.md:45`;
     `24-dynamic-ground-truth-ingestion.md:60`).
   - Result surfaces as the home-list **TRIM chip: "TRIM OFF" / "TRIM ON"** per row
     (`rev/DRWUI.exe/qt_strings_english.txt:1277-1278`; `artifacts/UIUX/07-home-phase-deep-dive.md:17`).
2. **DiskHealth.exe companion** (separate Qt Quick app in `diskhealth/`): CrystalDiskInfo
   lineage — `CAtaSmart` SMART engine with multi-transport accessors (SAT/ATA passthrough, CSMI
   for Intel RST, NVMe-Realtek bridge), `DiskInfoMgr`, `RefreshThread`, tray integration
   (`artifacts/DiskHealth/diskhealth--smart-companion.md:11,18-22`). UI = frameless 840×590 QML
   window, disk selector + health card + **S.M.A.R.T. attribute table (ID / Attribute / Current /
   Worst(ST) / Threshold / RawValue / Status / "Temprature" [sic])**, health states
   "Good" / "Caution!" / "Normal", caution card "Your disk may have a physical failure:"
   (`artifacts/UIUX/19-diskhealth-quick-ui.md:8,26-28`). Launched/notification-opened via
   "Click_Open_DiskHealth_By_Notification" + `EaseUS_DiskHealth` window names
   (`diskhealth--smart-companion.md:28`).
3. **DRWUI integration**: "Disk Health" tile — "Check disk before recovery." — references
   `DiskHealth.exe`, `/diskhealth/`, `HKEY_LOCAL_MACHINE\SOFTWARE\(+WOW6432Node\)EaseUS\DiskHealth`
   (`rev/DRWUI.exe/strings_ascii.txt:12985-12999`).
4. **"No SMART" rendering**: **corpus-silent** — the corpus documents Good/Caution!/Normal states
   and attribute-table columns but no dedicated "SMART unavailable" string/screenshot.

**Net standard for defect 2**: disk-health data gathering (fsutil, SMART) is **invisible** —
piped stdout/stderr, hidden window, 10 s wait, output parsed into a cached global. No console
ever flashes (see §7).

---

## 6. UI visual identity (for the residual elevation-prompt standard, defect 5)

1. **Three-layer theme**: master QSS `og-codebase/style.css` (20,802 lines) + `config/themdark.ini`
   118-slot dual palette `[normal]/[dark]` + RCC bundles `:/Resource/` (light) and
   `:/ResourceDark/` (dark) (`artifacts/UIUX/05-styling-theming-layout.md:6-12`).
2. **Palette (light)**: primary EaseUS blue `#0560fd`, CTA gradient `#0c99ff→#0560fd`, hover ring
   `rgba(5,96,253,0.1..0.2)`, warn `#f72c2c`/`#ffc10e`, success `#30b373`, tooltip bg `#0560FD`
   (`05-styling-theming-layout.md:10`). **Dark**: primary `#5094ff` (CTA `#059afd→#0ccdff`),
   surfaces `#1b1d1f/#202428/#32383e`, text `#f1f5ff`, accent `#ff7342`, warn `#ff5a5a`
   (`:11`).
3. **Window geometry**: normal 1240×840, minimum 800×600, `window_show_state=StatusMaxed`
   (opens maximized), high-DPI (`05-styling-theming-layout.md:18-22`); splash = 640×480
   frameless `CStartWindow` ("Reading the disk information, please wait…" + 60-70% progress bar)
   (`artifacts/UIUX/dynamic-ground-truth--splash-captured.md:21-27,49`).
4. **Dialog standards** (what any premium prompt should match): shared `DrwMessageBox` **540×240
   fixed**, scenario dialogs 636×433 / 672×627, icon standards 48/56/72/80 px
   (`05-styling-theming-layout.md:23`); tooltips 14 px, opacity 0.9, radius 8 px, max-width
   400 px, `#0560FD` bg / white text (`:30`); 4-step neutral border ladder
   `#e7e7e7/#dee1ed/#c1ccdf/#a0a6b1` (`:57`); typography Segoe UI base + CJK stacks, DIN-Black
   display font (`:27-29`); modal chrome family `PublicShadowDialog`/`PublicLoadingDialog`/
   `UiBasicDialog` (`artifacts/UIUX/06-dialog-telemetry-inventory.md:31`).
5. **Elevation prompts specifically**: the reference has **none of its own** — elevation is the
   OS UAC at EXE launch via `requireAdministrator` (§2); no consent dialog exists in the
   226-site modal census (`06-dialog-telemetry-inventory.md:6-31`). **Corpus-silent** on UAC
   styling. If our product must show a residual in-app privilege prompt, the closest in-surface
   precedents are the privilege-failure strings (§2.7) and the `DrwMessageBox`/`BadSectorTipsDlg`
   (274×240) confirm-dialog pattern (`06-dialog-telemetry-inventory.md:10,18`).

---

## 7. No visible console windows — the reference's subprocess discipline (defect 2)

1. **fsutil** (TRIM probe): piped stdout+stderr, `STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW`,
   `wShowWindow=0` (SW_HIDE) — `rev/ghidra-drwui/DRWUI_decompiled.c:178726-178733`. No console is
   ever mapped for it.
2. **netsh** (NAS firewall prep): `WinExec("netsh advfirewall firewall set rule group=\"Network
   Discovery\" new enable=Yes",0)` — second arg **0 = SW_HIDE**; same for the GBK 网络发现
   variant — `artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:108-109` (decompiled site
   `FUN_14023e4a0`, `DRWUI_decompiled.c:397497`).
3. **Deliberate visible launch where intended**: `WinExec("explorer ::{F02C1A0D-BE21-4350-88B0-
   7367FC96EF3C}",5)` (SW_SHOW) to open Network Computers — visibility is a per-call choice,
   not an accident (`24-dynamic-ground-truth-ingestion.md:121`).
4. **Census**: 21 `wShowWindow` sites in DRWUI's decompilation — **18 set 0 (SW_HIDE)**, 2 set 1,
   1 other (`rg -c "wShowWindow = 0"` = 18 vs total 21 over
   `rev/ghidra-drwui/DRWUI_decompiled.c`). The pattern: helper/child processes are created hidden
   with redirected stdio; only user-facing shell opens are shown.
5. **Loader-spawned children are GUI processes** (DRWUI, DRWReport) via `CreateProcessA`
   inheritHandles (`artifacts/DRW-exe--launcher.md:31,57,107,110`); the telemetry child
   AliyunWrapExe.Exe is a GUI uploader (`artifacts/UIUX/dynamic-ground-truth--wine-loader-run.md:17`).
6. **Runtime confirmation**: full window censuses (Wine + Windows-native, boot → scan → complete)
   enumerate **only Qt windows** (`Qt5QWindowIcon` etc.) — no console-class windows at any stage
   (`artifacts/UIUX/dynamic-ground-truth--window-census-verification.md:34`;
   `--landing-page-windows.md:15-17`; `--scan-phase-captured.md:14-17`).

**Net standard for defect 2**: every helper execution (fsutil, netsh, future SMART cmds) must run
hidden with redirected output; a visible console window is a defect against the corpus.

---

## 8. Defect → reference behavior → best citation (quick matrix)

| # | Reported defect | Reference behavior (corpus) | Best single citation |
|---|---|---|---|
| 1 | No elevation at launch | Entry EXEs manifest `requireAdministrator`; whole tree elevated from process creation; privileged service instead of re-prompting | `rev/DRWUI.exe/strings_ascii.txt:41023` (+ `rev/DRW.exe/strings_ascii.txt:355`) |
| 2 | Console popups on disk health | fsutil runs via `CreatePipe` + `CreateProcessA` with `wShowWindow=0` (SW_HIDE) and piped stdio; 18/21 STARTUPINFO sites are SW_HIDE | `rev/ghidra-drwui/DRWUI_decompiled.c:178726-178733` |
| 3 | 2 internal + 1 SD, only one shows | Five-stack fan-in (WMI + SetupAPI/IoCtl + VDS + Storage Spaces + FunctionDiscovery) merged by `CDeviceManagerBuilder`; internal disks AND SD ("Insert Camera Device, name= SD/MMC/MS PRO") both land in the device list; "Internal drives (N)"/"External drives (N)" groups | `og-codebase/log/DRWUI.log:20-32` (+ `og-codebase/EPMLOG.log:1-7`) |
| 4 | Stuck at 75%, "Files Found: 21 (0 B)", frozen sector ticker | Advanced % is sector-linear and keeps moving (9%→34%→100% over ~15 min); status line = "Advanced Scanning , Files Found: N (size GB)" with count+size growing live; "Reading sector: X/Y" + "Remaining time: NNmNNs" update continuously; UI stays browsable | `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md:102` (+ `:88-96`) |
| 5 | Residual elevation-prompt visuals | Reference shows no in-app elevation dialog (OS UAC only); premium dialog standard = DrwMessageBox 540×240, #0560fd primary, 8px-radius tooltips, Segoe UI | `artifacts/UIUX/05-styling-theming-layout.md:10,23` |

*Corpus-silent items (do not invent standards): UAC-dialog visuals; Inno `PrivilegesRequired`;
"no SMART available" rendering; explicit scan-phase weight formula for the % readout.*
