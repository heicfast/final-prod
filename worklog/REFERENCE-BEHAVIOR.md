# REFERENCE-BEHAVIOR — EaseUS Data Recovery Wizard (the "og"), full refresh

> Agent E, Phase 0 reference-behavior digest. **Sources**: `easeus-drw-research/` corpus only
> (`architecture/`, `re-workspace/artifacts/`, `re-workspace/reversed/`, `og-codebase/`).
> Every claim carries a corpus path:line citation. Errata file
> `re-workspace/artifacts/CORRECTIONS.md` read in full and folded in (§2). This file
> OVERWRITES the previous five-topic digest; nothing from it was carried over unverified.
> Companion context (not corpus): `final-prod/worklog/DIAGNOSIS.md` defect register
> D-PRIV-01 / D-CONS-01 / D-ENUM-01 / D-SCAN-01 / D-UI-01 — each surface below is ordered
> to answer those defects.

---

## 1. Corpus inventory (what document families exist)

Repo root: `README.md` (layout table :9-17, reading order :18-27, method/integrity :37-45 —
"no deobfuscation-by-guessing… UI copy from shipped string tables").

| Family | Where | Contents |
|---|---|---|
| **Top-level architecture docs** | `architecture/00-INDEX.md` (master index, :5-17 docs, :18-75 artifact table, :101-121 coverage) | 01 build-tree intelligence, 02 kernel drivers, 03 WinPE, **04 recovery workflow** (the master step-by-step), **05 scan-engine architecture**, **05-uiux-architecture** (UI/UX master, 35 sub-artifacts), 06 repair chain, **07 user-flow UI/UX** (screen-by-screen w/ byte-exact copy) |
| **Per-target deep artifacts** | `re-workspace/artifacts/<target>/…md` | DRW launcher, DRWService, DRWUI (main UI + threads), SearchDLL family (9 engines + carving deep-dives), PartitionRecovery (drivers, MOM modules, RawFixer, BitLockerLib), SaveScan (.rsf format), DiskHealth, Installer, UIUX (35 numbered + 5 dynamic ground-truth), config-files, FileRepair, Cloud/EuShare/Licensing/WinPE, per-binary (110+) |
| **Errata** | `re-workspace/artifacts/CORRECTIONS.md` | 9 rows of re-verified corrections (§2 below) |
| **Raw RE evidence** | `re-workspace/reversed/` | Ghidra decompilations (DRWUI 2.38M lines, engines 349K, drivers 42K, .mo 689K, partition 1.17M, repair 4.02M, misc 2.82M — index :77-91), per-binary rabin2/strings/pefile, ci-multitool (capa+FLOSS), ui-resources (QSS/QM/telemetry/RCC extractions) |
| **Inventories & tooling** | `re-workspace/inventory.{json,csv}` (565 PEs), `pdb_paths.json`, `rtti_classes.json` (6,903 classes), `scripts/` (RTTI extractor, QM/QSS parsers, RSF parser, screenshot ingest) | README.md:93-99 |
| **Dynamic ground-truth runs** | `re-workspace/raw-evidence/ui-*` (splash, landing, scan-phase, complete, results, recovery, atlas — 63+ organized screenshots) + the 5 `UIUX/dynamic-ground-truth--*.md` artifacts + `30/35` | Pixel-verified captures on Wine + Windows CI runners; every read string correlated 1:1 against the binary (`UIUX/dynamic-ground-truth--landing-page-windows.md:47-57` — "100% of read text verbatim-verified") |
| **Vendor's own runtime logs** | `og-codebase/log/` → `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` | QA-night session forensics: build id, scan timings, tree-builder stage machine, partition-search output |
| **Chronological worklog** | `worklog.md` (root, 62 KB) | Session stages 0→5 |

## 2. Errata — CORRECTIONS.md and in-artifact correction registers (task 9)

`re-workspace/artifacts/CORRECTIONS.md` (all 9 rows; header :1-7: "these rows supersede their
originals", evidence bar = decompile cite or mechanical recount):

| # | Corrected item | Currently-authoritative reading | Cite |
|---|---|---|---|
| 1 | NTFS "resident $DATA cap 0xFFF" | **No 0xFFF resident gate exists** — constant belongs to an MSVC `std::string` dtor sequence, not the attribute parser | CORRECTIONS.md:11 |
| 2 | FUN_180041970 = "in-use test" | FUN_180041970 is the **BASE-RECORD test** (`base_record != 0`); extension-record skip logic keys on it | CORRECTIONS.md:12 |
| 3 | FAT BAD-cluster parity | Og follows FAT12 BAD cluster 4087 **raw** (garbage-tolerant continuation), no 12-bit-boundary canonicalization; core's canonicalization is a *registered delta*, not og behavior | CORRECTIONS.md:13 |
| 4 | ext4 backup-superblock positions (8193, 32768…) | **UNVERIFIED for the engine** — constants absent from EXT4SearchDLL decompile; kernel-knowledge overreach | CORRECTIONS.md:14 |
| 5 | FileFilter.xml "8 categories / 1,489 extensions"; RawConfig.ini "37 toggles" | Recount: 8 categories / **1,481** extensions; RawConfig.ini ships **34** toggles | CORRECTIONS.md:15 |
| 6 | ext4 journal commit/descriptor semantics | Engine gates ONLY on jsb magic 0xC03B3998 + blocktype {3,4}; every block in [s_first, s_maxlen) feeds the dir parser; **no commit/revoke handling engine-side** | CORRECTIONS.md:16 |
| 7 | NTFS viability gate boundary | Og is **strict `0x3ff < size`** — files of exactly 1023 bytes take the fast-path with no bitmap check; any `>=` reading is wrong | CORRECTIONS.md:17 |
| 8 | Btrfs chunk-tree-uuid validation | Og does **NO chunk-tree-uuid validation** (name memcmps only) | CORRECTIONS.md:18 |
| 9 | EUUserData feeds RAW carving ("custom file type") | **EUUserData never reaches the carving engine** — sole consumer is DRWUI's `CHKRepairFileSign` repair chain; `GetRawUserInfoMap` is scan-*output* statistics, never scan input | CORRECTIONS.md:24 |

Additional in-artifact correction registers (each supersedes its own earlier text):
- `.rsf` is **zlib-chunked, NOT CryptoAPI-encrypted** (GUIDs = session ids; entropy = deflate):
  `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:15-23`; workflow doc carries the
  same correction flag (`architecture/04-recovery-workflow.md:22`).
- Record-4 node grammar: `rsf-record4--tree-node-grammar.md:3-13` — the byte-exact-validated
  §5.4 of the rsf artifact is authoritative over the preliminary per-case FS guesses.
- EUinApp.exe is an **orphaned IE-emulation registry bootstrapper**, not a purchase host:
  `architecture/05-uiux-architecture.md:86-90`, `UIUX/23-euinapp-ie-emulation-bootstrapper.md`.
- Completion celebration = **standalone 540×380 OS child window**, not an in-window overlay:
  `UIUX/30-interaction-driver-ground-truth.md:9-19`; "Go to Recover" lands in **results browse**
  (+AI toast), not a destination dialog (:21-30); destination dialog only via sub-selection
  under the free limit (:48).
- "Go to Recover" / completion-CTA landing corrections and QSS-target recount 1,677→1,710:
  `architecture/05-uiux-architecture.md:203-204`.
- Splash identity: the 640×480 "modal" is the CStartWindow splash itself; landing page requires
  enumeration completion: `UIUX/dynamic-ground-truth--splash-captured.md:29-31,68-77`.
- CDetailPage page-3 insertion attributed to follow-up init `FUN_140835910` (order/indices
  unchanged): `architecture/05-uiux-architecture.md:126-130`.
- UIUX §10 independently re-verified the highest-risk UI claims (all PASS):
  `architecture/05-uiux-architecture.md:112-124`.

## 3. PRIVILEGE MODEL (task 2)

- **DRW.exe (DRWLoader, the Windows entry point) ships `requireAdministrator`** — embedded
  manifest in .rsrc: `re-workspace/reversed/DRW.exe/strings_ascii.txt:351-355`
  (`<requestedExecutionLevel level='requireAdministrator' uiAccess='false' />` at :355),
  byte dump `DRW.exe/rabin2_strings_data.txt:1183` (RVA 0x14001ecd8).
- **DRWUI.exe (the real app) is also `requireAdministrator`**: `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:41023`
  + `DRWUI.exe/rabin2_strings_data.txt:1728` (RVA 0x142782e88, .rsrc).
- **Legacy refusal string still shipped in DRW.exe** (UTF-16 resource): "You must have
  administrator rights to run EaseUS Data Recovery Wizard!…" — `DRW.exe/strings_utf16.txt:45`
  (rabin2 `DRW.exe/rabin2_strings_data.txt:1182`, .rsrc); adjacent single-instance string
  "&Data Recovery Wizard has been running!" at `strings_utf16.txt:43`.
- **Launch chain (no second prompt)**: DRW.exe supervisor `CreateProcessA("DRWUI",
  inheritHandles=TRUE)` — `re-workspace/artifacts/DRW-exe--launcher.md:29-31` (decompile
  DRW_decompiled.c:2521). The elevated token is inherited, so **UAC fires exactly once, at
  DRW.exe launch, OS-managed via the manifest**. The loader stays windowless (supervisor);
  crash-restart loop guarded `1 < uVar17` (max 1 auto-restart) and a 5 s log flusher
  (`DRW-exe--launcher.md:35-65`, ledger :100-113).
- **DRWUI does not run standalone**: direct launch renders zero windows and exits in all three
  environments tested — `UIUX/dynamic-ground-truth--window-census-verification.md:36-40`;
  standalone prints Firebase-init lines then dies (`UIUX/dynamic-ground-truth--wine-loader-run.md:48-55`).
  So there is no "app started unelevated" state to recover from in the reference: the OS blocks
  launch before any UI exists.
- **No restart-as-admin flow exists**: no `ShellExecute`/`runas` verb in `ghidra-core/DRW_decompiled.c`
  (rg negative across the decompile); in-app "admin" strings are NAS/Linux SSH credential prompts,
  not local elevation — "Account without sufficient permission. Try with your Administrator
  account." / "Administrator account required" / "Password of Administrator" at
  `DRWUI.exe/strings_ascii.txt:12678,13151-13152` (NAS connect context, string neighbors
  "Install & start SSH service on target Linux", "connect.exe", "NasConfig").
- **Degraded no-admin IO path exists engine-side**: when the EPMNtDrv probe fails, MOM `Device.mo`
  falls back to opening `\\.\HarddiskVolume%d` directly (GENERIC_READ) — "the 'no-admin'
  degraded mode visible in the UI flows" — `re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md:145-151`.
  The driver itself has **no ACL**; control comes from signed-driver install + the PID-token
  handshake (:59-96, and `eprdtdrv-sys--driver-internals.md:90`).
- **Privileged services instead of mid-session UAC**: DRWService.exe ("Read FVE Server") is
  installed AUTO_START and "runs elevated so a standard-user DRW instance can obtain the keys
  without UAC" — `re-workspace/artifacts/DRWService-exe--bitlocker-service.md:5` (FVE keys are
  ACL-restricted to SYSTEM/admin, :48,:79). DRWService's own manifest is `asInvoker`
  (`DRWService.exe/strings_ascii.txt:201`) — it is the SCM, not the manifest, that elevates it.
- **Installer**: SetupUE.exe bootstrap = `asInvoker` (`SetupUE.exe/strings_ascii.txt:354`),
  gates on a 16-OS table then hands off to the Inno Setup wizard (unins000 trio):
  `re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:9-16`. All other
  binaries (engines, viewers, DRWReport, EuRepairHost :81, EuDownloader :8163, ZfsSearchDLL :644)
  are `asInvoker` and inherit the elevated parent token.
- **Elevation is functionally required**: raw sector IO via `\\.\EPRDTDRV` (workflow :76-78),
  `fsutil behavior set disabledeletenotify` TRIM toggling at scan start/exit (§5 below), and the
  netsh firewall prep (`UIUX/dynamic-ground-truth--window-census-verification.md:52`) all need admin.

**Net reference behavior**: single manifest-driven UAC at launch of the loader; loader spawns the
UI elevated with inherited handles; no in-app elevation banners, no restart-as-admin dialogs, no
per-action UAC (services carry privileged work).

## 4. DISK ENUMERATION (task 3)

- **Boot sequence**: DRW.exe → DRWUI splash `CStartWindow` "Reading the disk information,
  please wait…" with a thin blue progress bar (`UIUX/dynamic-ground-truth--splash-captured.md:9-27`;
  widget names StartWindowIconLabel/StartWindowTitelLabel/bg_downloader.png at :33-49). The splash
  **is the enumeration phase**: WMI `ConnectServer` ×2 + mountmgr `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`
  ×4 + storage-property + geometry IOCTLs (0x7405c/0x4d004/0x70048) + `fsutil behavior` ×2 run
  behind it (`dynamic-ground-truth--splash-captured.md:51-66`, `--wine-loader-run.md:28-47`,
  `--window-census-verification.md:42-55`). Main window is created but hidden until enumeration
  completes (`--splash-captured.md:49`).
- **Enumeration stack**: DRWUI `EnumCollectPartition::{EnumDevice,EnumDiskDevice,RemoveCache}`
  ("Enum disk begin/end" traces) — `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md:225`;
  device-manager singleton `FUN_14076df60` with `FUN_14076e1d0(dev,1)` refresh start
  (`UIUX/07-home-phase-deep-dive.md:60-63`); engine-side MOM modules: DeviceManager.mo
  "enumeration, hot-plug notify, WMI queries" and Device.mo opens `\\.\EPRDTDRV\…`,
  `\\.\PhysicalDrive%d`, and WMI storage classes `\\.\ROOT\microsoft\windows\storage:MSFT_Disk /
  MSFT_PhysicalDisk / MSFT_Volume / MSFT_StoragePool / MSFT_VirtualDisk`
  (`re-workspace/artifacts/MOM-modules/DeviceManager.md:36-56`, `MOM-modules/Device.md:36-56`,
  `PartitionRecovery/mom-module-system--architecture.md:91-99`). Devices are listed from
  enumeration APIs; opening a device (raw path) happens later at scan/IO time
  (`mom-module-system--architecture.md:93-98`).
- **Landing page (pixel-verified)**: left sidebar sections "Data Recovery" (Hardware Disk / SD
  Card / More Recovery Tools), "Advanced Features" (File Repair / Lost Partition Rescue),
  "File Backup" (ChronoSnap Backup); center header "Select a location to search for lost data";
  device groups **"Internal drives (N)"** and **"External drives (%1)"** — BOTH rendered with
  live counts (`DRWUI.exe/qt_strings_english.txt:470,636`; capture `UIUX/dynamic-ground-truth--landing-page-windows.md:24-45`).
- **Device cards**: drive icon + friendly name + **used/total capacity + used-space bar**
  ("Windows(C:) 30.96 GB/149.45 GB"); selection = blue border + checkmark
  (`dynamic-ground-truth--landing-page-windows.md:31-45,63`; `35-redesign-reference-atlas.md:149-164`).
  Tile map with click centers: `35-redesign-reference-atlas.md:124-164`.
- **Home rows (PC-recovery tab `DrwHomePage`)**: per-partition icon from ImageResource.xml type
  map (system/USB/lost/lock/unlock/camera/UAV/SD/dashcam/recycle-bin), drive letter + label, file
  system, capacity bar, **TRIM chip "TRIM OFF"/"TRIM ON"**, per-row `BtnScan`
  (`UIUX/07-home-phase-deep-dive.md:13-17`). Header: "Select a location to start recovering" +
  "Can't find location?" + "Refresh" + "Devices"/"Locations" sections + "Hard Disk Drivers" [sic]
  group (:15). Whole-disk group subtext: "Scan the entire disk if you don't know where desired
  data was exactly saved." (:19).
- **Whole physical disks**: `DrwLostDiskPage` (stack idx 1) lists whole disks, type codes 22–25 →
  `drivetype_ico_disk.png` (`UIUX/07-home-phase-deep-dive.md:44-46`); scanning from there uses
  `CDoScanLostPartition` (`architecture/04-recovery-workflow.md:44`).
- **SD/removable are first-class**: dedicated sidebar page "Connect Your Memory Card to Recover
  Lost Photos & Videos" + 3 feature tiles (Fix SD card access / Photos that open / Video that
  play) + "Show all external devices" + "Get Help" (`35-redesign-reference-atlas.md:143`);
  scan-flow classes `CSDCardDeviceListPage` + `CSDCardScanTipsWnd` (`architecture/07-user-flow-uiux.md:33`).
  Vendor QA log shows the SD flow live: "CameraDeviceManager: Insert Camera Device SD/MMC/MS PRO
  (size=62607327232)" with `nMaxRemovebleDiskSize=549755813888` (512 GB threshold)
  (`config-files/qa-logs--session-forensics.md:22-24`). Device naming: "PE device
  name=ST500DM002-1BD142, F:, G:, H:, I:" / "PE device name=Generic-SD/MMC/MS PRO, E:" (:21-22).
- **Refresh behavior**: user-triggerable **modal refresh flow** (view-manager slot 7:
  `QDialog::exec` + `refreshFinished()`), lists rebuild on `deviceEnumFinished()`
  (`UIUX/07-home-phase-deep-dive.md:8-11`); hot-plug notify is engine-side (DeviceManager.mo,
  `mom-module-system--architecture.md:93`).
- **Empty/dead-end state**: "NoDeviceGuide*" frames, "Drive Not Show Up" troubleshooting link,
  telemetry `Click_Cannotfindlocation` (`UIUX/07-home-phase-deep-dive.md:34`; `architecture/07-user-flow-uiux.md:17`).
  The landing page also carries "Unable to find your hard drive?" + ⟳ (`qt_strings_english.txt:1363`,
  capture `--landing-page-windows.md:41`).
- **Session resume banner** on home: "Continue last scanning · Scan time: · Scan files: ·
  Scan disk: · Load" (`UIUX/07-home-phase-deep-dive.md:21`).
- **Per-disk identity persisted per scan session** (.rsf record 2/3): name[100] + devtype +
  SYSTEMTIME + 2 stat qwords + flags; record 3 = 1781 B partition struct + 5400 B volume-device
  blob = disk identity + **512-byte boot-sector snapshot** (`architecture/00-INDEX.md:114`;
  `SaveScan/rsf-format--scan-session.md:55-62,133+`).

## 5. DISK HEALTH / SMART (task 4)

- **Disk health is a separate companion app, not a DRWUI page**: `DiskHealth.exe` (internal name
  **DiskSmart**), **CrystalDiskInfo lineage** (CAtaSmart / DiskInfoMgr / CTrayIcon + USB-bridge
  tables `AddDisk - ASM1352R`…), Qt Quick/QML, frameless **840×590** window, tray-resident
  (close hides to tray), startup-permission prompt — `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md:9-13`,
  `UIUX/19-diskhealth-quick-ui.md:6-14`.
- **What the health view shows per disk**: health states "Good" / "Caution!" / "Normal" /
  "Your disk may have a physical failure:"; **S.M.A.R.T. attribute table with columns
  ID / Attribute / Current / Worst (ST) / Threshold / RawValue / Status + "Temprature" [sic]**;
  disk selector "Which disk do you care about?" / "Disk %1"; `firstCautionIndex()` jumps to the
  first caution disk; CautionWidget card with "View Solutions" — `UIUX/19-diskhealth-quick-ui.md:24-34`.
- **Engine**: `CAtaSmart` multi-transport SMART polling — `GetSmartAttributeSat` (SAT/ATA
  passthrough), `GetSmartAttributeCsmi` (CSMI for Intel RST), `GetSmartAttributeNVMeRealtek`
  (Realtek NVMe bridge); `RefreshThread` periodic background refresh; `AttrModel` feeds the QML
  table; interface awareness incl. "INTERFACE_TYPE_NVME" and "Parallels Virtual NVMe Disk" —
  `diskhealth--smart-companion.md:12,14-23`. **No smartctl, no PowerShell probe — the SMART data
  path is native ATA passthrough/CSMI/NVMe inside the app.**
- **Launch integration**: DRWUI launches `<appDir>/diskhealth/DiskHealth.exe` via `CreateProcessW`
  (theme argument ` themeName=dark` when dark; language handed over via QSettings registry
  `HKLM\SOFTWARE\WOW6432Node\EaseUS\DiskHealth\Language`) —
  `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:1337994-1338055` (path build at :1337996-1338000,
  CreateProcessW at :1338054). Entry surfaces: "Disk Health" tool card under More Recovery Tools
  (`35-redesign-reference-atlas.md:144`; `DRWUI.exe/qt_strings_english.txt:385`), "Launch Disk
  Health" (`UIUX/19-diskhealth-quick-ui.md:28`), and Windows-notification launch
  ("Click_Open_DiskHealth_By_Notification", `diskhealth--smart-companion.md:28`).
- **In DRWUI proper there is NO SMART page**. Health-adjacent surfaces: per-row **TRIM chip**
  "TRIM OFF"/"TRIM ON" on home rows (`UIUX/07-home-phase-deep-dive.md:17`;
  `qt_strings_english.txt:1277-1278`); bad-sector coaching "Program is trying to skip bad sectors
  to find data." (`DRWUI.exe/strings_ascii.txt` — qt string 0001d05c) and "There are %1 bad
  sectors in the disk." (qt string 0002a01b); `BadSectorTipsDlg` (274×240/256×240 QSS moves)
  (`UIUX/06-dialog-telemetry-inventory.md:18`); disk-image advice "Disk image is a copy of the
  entire disk, which can improve the recovery performance when the disk has bad sectors."
  (`qt_strings_english.txt:389-390`).
- **DRWUI DOES spawn one helper subprocess for health-adjacent state — `fsutil.exe`, hidden**:
  query path `FUN_1400c91a0` → `FUN_1400c91d0(" behavior query disabledeletenotify")`
  (`DRWUI_decompiled.c:178674-178683`); the spawn builds `%s\fsutil.exe` from GetSystemDirectoryA,
  `CreatePipe` + `CreateProcessA` with `dwFlags=0x101` and **`wShowWindow=0` (SW_HIDE)**,
  `WaitForSingleObject(10000)`, parses output after `"= "`: `'0'` → TRIM ON, `'1'` → TRIM OFF,
  cached in global `DAT_141ba9514` (−1 unknown) (`DRWUI_decompiled.c:178725-178767`; command
  strings `DRWUI.exe/strings_ascii.txt:35319-35323`). **TRIM write-toggle**: at scan start on an
  SSD-flagged target `FUN_1400c9450(0)` = `fsutil behavior set disabledeletenotify 1` (TRIM
  disabled during scan) — `DRWUI_decompiled.c:178768-178790,314214-314224`; on exit
  `FUN_1400c9450(1)` = `set disabledeletenotify 0` — `:315408`. Runtime-confirmed twice
  (`UIUX/dynamic-ground-truth--wine-loader-run.md:45,62`, `--window-census-verification.md:53`).
- **PowerShell/wmic strings in DRWUI are display copy, not probes**: `PowerShell
  "Get-PhysicalDisk | Format-Table -AutoSize"` and `wmic DISKDRIVE` are tr-wrapped strings built
  by string-builder functions (`DRWUI_decompiled.c:365727/365769`;
  `DRWUI.exe/strings_utf16.txt:13920-13922`) with no CreateProcess site — troubleshooting/support
  text. The only executed subprocess in the health path is the hidden fsutil above.
- **Devices without SMART**: not documented as a special case in DiskHealth artifacts; DRW-side
  TRIM chip derives from fsutil (which reports 0/1 for the OS-level delete-notify state, i.e. a
  global OS setting, unknown −1 when the query fails — `DRWUI_decompiled.c:178658-178683`).
  Virtual disks are named in DiskHealth ("Parallels Virtual NVMe Disk" —
  `diskhealth--smart-companion.md:12,23`), i.e. virtual/USB devices appear in the list with
  transport-specific accessors rather than being excluded.

## 6. SCAN FLOW (task 5)

- **Entry**: landing-page "Search for Lost Data" blue CTA (capture coords
  `35-redesign-reference-atlas.md:162`) or per-row `BtnScan`; signal chain
  `DrwHomePage::startScanDevice(ScanDeviceInfo)` → `scanClickedSlot` → CVersionCtrl prep
  (vtable+0x1f0/0x1f8) → `CDetailPage::setScanDevice` → orchestrator vtable+0x18 START →
  `QStackedWidget::setCurrentIndex(3)` (scan runs INSIDE the results page, same page)
  (`UIUX/13-ui-core-wiring-state.md:18-42`; `architecture/05-uiux-architecture.md:29-34`).
- **Two-phase model**: **Quick scan** = FS-metadata pass: FS type of the volume (partition-table /
  boot-sector probe) indexes the factory map — codes 0-2 FAT, 3 NTFS, 4-5 EXT2, 7 exFAT, 8 HFS,
  9 EXT4, 10 ReFS, 13 ZFS (+Btrfs) → `EUCreate<FS>QuickSearcher` → `IQuickFileSystemSearchController`
  (`architecture/04-recovery-workflow.md:52-60`; `DRWUI-exe--main-ui-architecture.md:239-252`).
  **Advanced/Deep scan** (`FUN_140580720`): creates the FULL searcher set
  (`EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher`) in a std::list **plus `EUCreateRAWSearcher`
  when settings+0x60 enables carving**, registers a `std::function<string(string,string)>`
  naming/filter callback into the RAW engine (vtable+0x48) — "Advanced Scan is searching files
  sector by sector…" (`architecture/04-recovery-workflow.md:61-65`;
  `DRWUI-exe--main-ui-architecture.md:255-261`). **Advanced = all FS engines in full mode + RAW
  carving in parallel** (`DRWUI-exe--main-ui-architecture.md:261`).
- **Timing (vendor QA log, real disk)**: quick pass ≈ **21 s** ("22:57:57 doScan … 22:58:23
  UpdateScanFinishedInfo (quick pass ~21s)"), deep scan same StartSector/TotalSector range, full
  session ~15 min producing the .rsf (`config-files/qa-logs--session-forensics.md:25-33`).
  Dynamic run: full scan ≈14.7 min for a 149 GB volume, 1,139,286 files / 147.12 GB ≈ 98.5% of
  capacity enumerated (`UIUX/dynamic-ground-truth--scan-phase-captured.md:138`).
- **Status line (stage names, byte-exact)**: "Quick Scanning%1, Files Found:" (:10314),
  "Advanced Scanning%1, Files Found:" (:10318), "Cloud Scanning%1, Files Found:" (:10313),
  "Scan Paused%1, Files found:" (:10307), "Scan Completed" (:10326) — with inline colored spans
  `<font color='#5094ff'>%1</font>` / `#0560fd` / gray `#616777` (`DRWUI.exe/strings_ascii.txt:10303-10326`).
  Dynamic verbatim: `Advanced Scanning , Files Found: 1139157 (143.74 GB)`
  (`dynamic-ground-truth--scan-phase-captured.md:102`).
- **Progress axes displayed simultaneously**:
  1. **Percent**: circular blue-ring gauge (`RadiusProgressBar`, ScanProgress/ 188 RCC assets) —
     "Advanced Scanning, 9%" at t=75 s (quick already done) → 34% at t≈460 s
     (`dynamic-ground-truth--scan-phase-captured.md:56,88-91`); remaining-time estimate "Remaining
     time: 07m43s" — linear sector-throughput model (:95-96,102; string `strings_ascii.txt:15216-15217`).
  2. **Files found + cumulative size**: "Files Found: 1,139,157 (143.74 GB)" live (:91,102).
  3. **Sector progress**: "Reading sector: 134316032/31413598" ticker (:96,102); smart partition
     scan variant "Scanning disk sectors: %1" (`strings_ascii.txt:14224-14226`).
  4. **Path ticker**: full-width light-blue "Searching: <current file path>" strip (:38,60).
  5. **Milestone line**: "%1 file chunks scanned..." (`strings_ascii.txt:11346`).
  6. **Sector-grid visualization**: `AnalysisGridWidget` (54 v-methods, disk-map painting)
     (`UIUX/08-scan-phase-deep-dive.md:10-12`).
- **Phase-boundary state machine** (vendor source lines captured at completion):
  `bottomarea.cpp, 838 nStatus=4, m_iScanType=3` (quick finished) then `:856 nStatus=4,
  m_iScanType=4` (advanced finished) — `UIUX/30-interaction-driver-ground-truth.md:73-74`;
  `slotLostPartitonBegin` switches the status bar to lost-partition search
  (`UIUX/34-navigatebar-bottomarea-chrome.md:72-74` — cited via 05-uiux wave-5 §34).
- **Quick→Advanced transition UX**: `CQuickScanCompleteWnd` "Quick Scan Completed" + primary CTA
  "Recover Found Deleted %1 files %2" + coaching "Tips: Advanced Scan is trying to find more
  other lost files for you while recove[ring…]" — advanced continues in the background while the
  user may already recover (`UIUX/08-scan-phase-deep-dive.md:34-36`); sector-by-sector copy
  "Tips: Advanced Scan is searching files sector bu sector to ensure data recovery as much as
  possible." [sic, shipped typo] (`architecture/07-user-flow-uiux.md:42-45`).
- **Live tree during scan (browse-while-scanning)**: results tree fills progressively; the
  3-branch tree (existing volume / File Path Lost / Reconstructed) is visible mid-scan; the
  **Reconstructed (RAW-carved) branch grows live** (1→16 files in 6 min; 16→87 by completion);
  Path/Type tabs and the preview panel are live; "Recover All …" CTA is active mid-scan
  (`dynamic-ground-truth--scan-phase-captured.md:56-63,94-105,99-101`;
  `UIUX/30-interaction-driver-ground-truth.md:26`; `35-redesign-reference-atlas.md:279` rule 9).
- **Controls**: Pause (❚❚) and Stop (■) bottom-left next to the gauge; paused state = "Scan
  Paused" + red strip, gauge frozen at pause %; stop-confirm via `CScanCommonPopDlg`
  (`35-redesign-reference-atlas.md:170-182`; `UIUX/08-scan-phase-deep-dive.md:55`). Cancellation
  is cooperative: `CStopScanThread` + manual-reset Win32 **scan-abort event in CScanMgr+0x108**
  (`UIUX/13-ui-core-wiring-state.md:42,63`).
- **Completion**: standalone **540×380 OS child window** "Congratulations! Scan Completed!" +
  stats grid (Files found / Files scanned / Used Time / Size / Organized / Completed / Video
  Repaired / Video to be repaired) + "Go to Recover" CTA; no ESC rejection; bottom bar becomes
  "Scan Completed, Files Found: N (S)" + active "Recover All ..." (`35-redesign-reference-atlas.md:186-188`;
  `UIUX/30-interaction-driver-ground-truth.md:9-19`; `dynamic-ground-truth--scan-phase-captured.md:111-127`).
- **Completion pipeline (telemetry-verbatim)**: cache-reset cascade (SearchDataSource/
  PreviewDataSource/OfficePreviewStub/ThumbnailFactory ClearCache) → **Freeze** →
  **auto-save `auto_<Volume>_<timestamp>.rsf`** → **Unfreeze** → `CDoScanPartition::ScanThread:483`
  → `UpdateScanFinishedInfo:498` → `CScanMgr::ScanFinished:636` → bottomarea.cpp:838/856 →
  quick-scan + recoverability passes (`CCheckNodeThread`, re-fires on Go-to-Recover and during
  browsing/selection) — `UIUX/30-interaction-driver-ground-truth.md:54-96,155-159`.
- **Organize phase** after engines finish: "Organizing found files..." modal state = tree
  building (`CDirectoryTreeManager`) with `CSetClassifyThread`/`CSortNodeThread` workers
  (`UIUX/08-scan-phase-deep-dive.md:31`). Vendor log for the tree builder: "prepare to create
  **3 new file system tree**" then BuildTree cycles in **incremental batches ~1000 items**
  ("show file system node" ⇄ "build raw file node"), 223 M cumulative items on the stress disk
  (`config-files/qa-logs--session-forensics.md:44-57`).
- **Resume support**: "Save Scan Session" → .rsf (zlib-chunked, 4 records; see §2 errata);
  "Continue last scanning" banner on home with Load; "Load Scan Session"/"Select scan session";
  "Load Scan Session Successful, Scan files:" status strings (`strings_ascii.txt:10322-10323`);
  import-from-file "(1-Import Scan Result" (`architecture/04-recovery-workflow.md:157-165`).
  **Auto-save on completion** is the primary resume vehicle (auto_*.rsf above). Save progress
  mapping (the only documented stage→percent map): 40% records 1-3 + 25% tree walk (40→65%) +
  remainder compression/finalize — `SaveScan/rsf-record4--tree-node-grammar.md:93-100`.
- **Smart partition scanning** (lost-disk path): "Smart Partition Scanning" → "Scanning disk
  sectors: %1" → "Smart Partition Scan Completed"/"Scan completed for Disk %1!" → found-partition
  list rows with Disk / Basic / Dynamic / MBR / GPT / File System properties + "Preview for
  disk %1" (`UIUX/08-scan-phase-deep-dive.md:17-23`; `strings_ascii.txt:14222-14226`). Engine:
  candidate generation → file-count scoring → `AdjustPartitionInfoOrder` final set
  (`config-files/qa-logs--session-forensics.md:37-42`).

## 7. RESULTS DISPLAY (task 6)

- **3-pane layout on CDetailPage**: left tree (Path/Type tabs), middle sortable list, right
  preview/details pane; toolbar = Home/Back/Up cluster + breadcrumb "Windows (C:)" + list⇄icon
  toggle + Filter funnel + "Search files or folders" box (`35-redesign-reference-atlas.md:192-208`;
  `dynamic-ground-truth--scan-phase-captured.md:23-37`).
- **The 3 roots (live-verified = .rsf record-4 roots)**: existing filesystem branch
  ("Windows (C:) 1.11M"), "File Path Lost" (warning icon, 4.4K), "Reconstructed" (puzzle-piece
  folder icon, RAW-carved regrouped) — `dynamic-ground-truth--scan-phase-captured.md:43-53`.
- **5 special branches with coaching copy**: Tags Extracted / File Name Lost / File Path Lost /
  Versions (NTFS multi-version) / Unimportant (collapsed junk bucket; `usefullostfiles.xml`
  promotes carved RAW photos out of it) — `UIUX/09-results-phase-deep-dive.md:27-37,45`.
- **Classify tabs**: 8 categories (Graphics/Video/Audio/Document/Email/Web&Others/Archive/Folders;
  FileFilter.xml — **1,481 extensions after CORRECTIONS row 5**) + "Unsaved File" in the search
  dropdown; display priority from PriorityDisplayType.xml (`UIUX/09-results-phase-deep-dive.md:39-49`;
  `architecture/04-recovery-workflow.md:103-105` w/ correction).
- **List columns**: Name / Date Modified / Type (+Size), sortable headers + **recovery-chance
  column** (`CListHeaderView`, 96 v-methods); async per-row thumbnails (`CGetListThumbnailThread`);
  select-all tri-state; 6-px slim scrollbar (`UIUX/09-results-phase-deep-dive.md:53-55`).
- **Size display**: live cumulative size in the scan status ("Files Found: N (GB)") and completion
  stats ("1139275 Files found" / "149.85 GB Size" panels); per-file size in details pane
  ("Size: 7.29 MB") (`dynamic-ground-truth--scan-phase-captured.md:31-33`;
  `UIUX/30-interaction-driver-ground-truth.md:19`).
- **Recoverability state**: per-file grading by `CCheckNodeThread` ("SearchDataSource::OnCheckNode")
  visualized as **"Recovery Chances: High" + circular gauge** and **Status: "Existing"** in the
  preview pane; re-grading re-fires during browsing/selection
  (`UIUX/30-interaction-driver-ground-truth.md:142-149,155-159`; `architecture/04-recovery-workflow.md:106-107`).
- **Preview**: double-click → live preview **panel** (no new window); the "Preview" button/context
  item → `PreviewMainDialog` modal (Freeze→exec→Unfreeze); per-type previewers (image zoom, video
  15 s free cap via `VideoPreviewWnd::timeout()` QTimer 15000 ms, 100 MB size cap, archive
  browsing, PDF raster paging, RtfTxt renders source inline); out-of-proc viewers for office
  (`EUOfficeViewer.exe`, `PDFViewer.exe`, `EUHwpViewer.exe`); `IInWindowStream` streams large
  files; `PreviewFailedWnd` on unreadable; free-tier banner "Preview only. Unlock Pro to enjoy
  full features and unlimited recovery." (`35-redesign-reference-atlas.md:262-267`;
  `architecture/05-uiux-architecture.md:107`; `architecture/04-recovery-workflow.md:117-125`;
  `architecture/07-user-flow-uiux.md:60-64`).
- **Filter/search**: filter dropdowns (File Type / Date Modified / File Size with Custom entries;
  toggles Existing / Hide 0KB / Hide System / Hide Temp / Hide Browser Cache; per-category
  "Select All (%1)" live counts + Reset) → 0x48-byte `AdvanceFilterCondition` → `CSetFilterThread`
  (`35-redesign-reference-atlas.md:229-237`); search with type dropdown + removable chip +
  "Search results for system" breadcrumb state (:212,222); blocked-state copy "The file was
  filtered. Please clear the filter and then [search] again." (`UIUX/09-results-phase-deep-dive.md:51`).
- **Context menu** (6 items): Recover / Preview / Open file location / Select unrecovered files /
  Smart select best files / Clear selection (`35-redesign-reference-atlas.md:214-225`).
- **AI layer**: "AI Recommend is on. Showing %1 recommended files.", "AI Recommend available."
  toast + persistent AI FAB at results entry (`UIUX/09-results-phase-deep-dive.md:55`;
  `UIUX/30-interaction-driver-ground-truth.md:27-29`).
- **Selection→recovery**: bottom CTA is contextual "Recover All ..." ⇄ "Recover ..." on selection
  (`35-redesign-reference-atlas.md:208-210` rule 8).

## 8. ERROR SURFACES + PROMPTS (task 7)

- **Disconnect states (CDetailPage region, tr-verified table)**: "Drive Disconnected" —
  "Unable to continue scanning. Reconnect your drive and try again." (Retry/Cancel); "NAS
  Disconnected" — "Check network and make sure NAS is on. Then try again." (Retry/Cancel/
  Disconnect); Linux "Disconnect with Linux" flow; "Cloud Drive Disconnected" — "Please check
  your network connection and try again."; parameterized "Check your network to make sure the %1
  server is accessible and then r[etry]"; NAS overlay `CNasNetErrorTipsWnd` floating tips
  attached at scan start — `UIUX/08-scan-phase-deep-dive.md:38-47`.
- **NAS/Linux connect errors**: "Incorrect IP or port or disabled SSH service." / "Incorrect
  password or user." / "Account without sufficient permission. Try with your Administrator
  account." / "Password has expired…" / retry logic `CRemoteDevice::Connect` ("retry %d")
  (`architecture/04-recovery-workflow.md:184-185`; `DRWUI.exe/strings_ascii.txt:12670-12686`).
- **Stop-confirm** (`CScanCommonPopDlg` multimode modal, 25 strings): "Stop searching for lost
  data?" + risk framing "Remaining data can be rescanned and recovered anytime later, but
  continuing now save you time."; navigation "Return to Home?"; organize "Stop Organizing Found
  Files?"; completion mode with stats grid — `UIUX/08-scan-phase-deep-dive.md:25-33`;
  `35-redesign-reference-atlas.md:182` (Stop = pink/red, Continue = blue).
- **Drive-not-detected coaching**: "Unable to find your hard drive?" + refresh; Q1/Q2 help
  strings "How to get my drive detected when it didn't show up?" (`qt_strings_english.txt:1363`;
  `DRWUI.exe/strings_ascii.txt:12688-12690`).
- **Bad-sector surfaces**: "Program is trying to skip bad sectors to find data." (qt 0001d05c);
  "There are %1 bad sectors in the disk." (qt 0002a01b); `BadSectorTipsDlg` —
  `UIUX/06-dialog-telemetry-inventory.md:18`.
- **BitLocker**: `CBitlockerStartWnd` → `BitlockerUnlockDialog` (password or recovery key,
  "Unlock and Recover") + `BitlockerUnlockRetWithPwdDlg` states + `BitLockerMSGuideDlg` how-to;
  keys via DRWService FVE pipe, BitLockerLib **manage-bde PowerShell fallback** engine-side
  (`architecture/07-user-flow-uiux.md:29`; `architecture/04-recovery-workflow.md:182-183`).
- **Recovery-path errors** (numbered taxonomy): "Cannot save to original partition" hard block;
  "Might be invalid path to save." / "2.The recovery path is too long." / "6.The data amount you
  want to recover has exceeded the free quota."; overwrite ladder "Data might be %1 and %2 if
  you save files to original drive…" / "Do not save data to this drive" —
  `UIUX/11-recovery-phase-funnel.md:12-17,23-30`.
- **Insufficient-space gate** (dynamically captured): "Insufficient Disk Space" + "Need an
  additional 146.41 GB of space. Please free up space or select another drive." + Continue /
  Reselect — `35-redesign-reference-atlas.md:250-252` (space check precedes recovery, rule 14 :285).
- **Free-tier/retention prompts**: `CRecoverLimitDlg` "Sure to give up recovering remaining
  valuable files?" / "Valuable data might be LOST PERMANENTLY…" / Give Up vs Recover Remaining;
  retention gate "Wait! You are at risk of losing data!" + Get Up to 40% Off / Online Service /
  Quit — **a PERIODIC nag every ~30-60 s of active use on ANY page on Free builds**
  (`UIUX/11-recovery-phase-funnel.md:23-30`; `35-redesign-reference-atlas.md:254-258,286`).
- **Styling/behavior of prompts**: standard `DrwMessageBox` 578×302 per shipped QSS
  (`og-codebase/style.css:819-827`) — note artifact 05's decompiled figure "540×240 fixed"
  (`UIUX/05-styling-theming-layout.md` §2) disagrees; QSS is the shipped truth (flagged
  discrepancy). Buttons: secondary = #e0eaff bg / #2978ff text / 28 px / radius 4, hover #c4d7ff,
  pressed #84acff, disabled #dee1ed (`og-codebase/style.css:603-624`). Palette: brand blue
  #0560fd (light)/#5094ff (dark), CTA gradient #0c99ff→#0560fd, warn #f72c2c/#ffc10e, success
  #30b373 (`architecture/05-uiux-architecture.md:38`). Scan status line inline font spans
  (`strings_ascii.txt:10303-10326`). Retention gate chrome: 755×519 frame BG_songfuli.png,
  radius 12; Buy button 114×28 radius 14 #FF610E / hover #FF8443 / pressed #DB4C00
  (`og-codebase/style.css:12953-12960,12977-12992`). Modal rules: modals dim the main window,
  swallow outside clicks; ESC rejected by Activate dialog + celebration window; menus close on
  single ESC; **double-ESC from results = exit attempt → retention gate** —
  `35-redesign-reference-atlas.md:270-287`.
- **Crash path (invisible)**: loader watchdog → DRWReport.exe minidump + Elapsed/Error/Result
  JSON → restart DRWUI (max 1 auto-restart) — `architecture/04-recovery-workflow.md:176-179`;
  `DRW-exe--launcher.md:77-90`.
- **No elevation/admin notice dialog or banner exists anywhere in the reference** — elevation is
  manifest-at-launch only (§3). The only red-warning tip styling in QSS is
  `RecoveryOriginalTipDlg` `tipsSpecStyle: "color:#ff3c3c; font-weight:900; …"`
  (`og-codebase/style.css:837-842`).

## 9. ENGINE INTERNALS relevant to scan correctness (task 8)

- **NTFS quick pass** = direct $MFT record enumeration including deleted records (fixup-aware),
  paths reconstructed from parent-reference chains; **complete pass** adds $Bitmap volume-bitmap
  build + USN-journal cross-check ($UsnJrnl strcmp sites) + orphaned-record sweep +
  attribute-stream assembly; **$MFTMirr** cross-referenced as repair source;
  `fastsearch` config **defaults ON** (`GetPrivateProfileIntW(L"search",L"fastsearch",1,…)`)
  — `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:102-114` (ledger :162-180).
- **$MFT bootstrap**: 64 KB first-read, USA/fixup validation, header sanity (magic "FILE"
  0x454c4946), **$ATTRIBUTE_LIST (type 0x20) fork** for fragmented MFT (logs "MFT file record
  has 0x20 attribute, MFT start sector is %I64d") — `ntfs-engine--deep-dive.md:42-60`.
- **MFT walk filters**: skip flags&4 temp/system, name==NULL, `$`-prefixed metafiles; deleted
  branch = in-use bit clear; **deleted-file recovery-quality scoring** iterates data runs and
  bitmap-tests each run start → readable-extent ratio surfaced later as recovery chances —
  `ntfs-engine--deep-dive.md:60-101`. Viability gate boundary: strict `0x3ff < size`
  (CORRECTIONS.md:17). FUN_180041970 = base-record test (CORRECTIONS.md:2).
- **Thread-safe refcounted property objects** (LOCK/UNLOCK interlocked) so UI async workers can
  share nodes — `ntfs-engine--deep-dive.md:84-87`. Reads via `CStorageDeviceWithCache`
  (read cache survives MFT re-reads), **cluster-strided MFT reads** (aligned blocks, not
  per-record), raw IOCTL path on damaged volumes — :143-151.
- **Error discipline**: HRESULT-style 0x5002xxxx engine codes; **fixup failure skips the record,
  never aborts the walk** (maximize salvage) — `ntfs-engine--deep-dive.md:153-160`.
- **RAW carving**: signature registry `std::map<u32 fourcc, Analyser*>` (verified keys: JPEG
  FFD8FFEE, %PDF, TIFF×4 byte-orders, PNG, Rar!, ZIP) + **raw_signs.json** (139 extensions,
  header/tail hex + offset + confidence + min/max size) loaded at engine init + hardcoded
  per-format analysers that compute file END by walking structure; brute-force namespace walks
  forward until grammar breaks (maximize-salvage trade-off); per-extension runtime toggles via
  `config/RawConfig.ini` (**34 toggles** after CORRECTIONS row 5) —
  `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:28-50,52-68,85-91`.
- **Carving ↔ UI contract**: RAW searcher registered LAST in the advanced list; receives the
  naming/filter callback (vtable+0x48) — carved-file naming policy lives in the UI; carved nodes
  feed `IRawDirectoryTreeBuilder` → flat "Lost" views; **merge rule: structured nodes win,
  carved attach as "Lost"/"Other deleted files"** — `rawsearch-carving--deep-dive.md:70-83`;
  `architecture/05-scan-engine-architecture.md:95-105`. GetRawUserInfoMap is scan-output
  statistics only (CORRECTIONS.md:9).
- **Engine controller = tree builder (multiple inheritance)**: each engine controller IS its
  directory-tree builder (`DetourDirectoryTree` decorator feeds progress + dedup +
  raw/structured merge) — `architecture/05-scan-engine-architecture.md:45-57`. `CPartitionVirtualVolumeDevice`
  abstracts volumes the OS can't see (lost partitions) — :59-65.
- **Progress tick cadence (documented facts)**: (a) scan-side — file-count milestones emit
  telemetry; engine callbacks post progress/message events continuously (`ScaningRecoverMessage`,
  `AnalysisGridWidget`) (`UIUX/08-scan-phase-deep-dive.md:49-56`); dynamic captures show the
  "Searching:" path ticker + sector ticker updating within seconds during advanced scan
  (`dynamic-ground-truth--scan-phase-captured.md:60,96`); (b) tree-building — incremental
  batches of ~1000 items ("show file system node"/"build raw file node" cycles) so the UI never
  blocks (`config-files/qa-logs--session-forensics.md:44-57`); (c) session-save — per-node-batch
  `CSaveAndRestoreProgressInfo` objects, percent = 40% + (processed×50)/(total×2) → 40-65%
  across record 4 (`SaveScan/rsf-record4--tree-node-grammar.md:93-100`).
- **Volume IO layers**: normal file API when healthy → raw via EPMNtDrv `\\.\EPRDTDRV\<d>\<p>`
  (MDL async sector IO, 512 B sectors, max 200 disks, disk.sys stack verification) → BitLocker
  FVE keys + XTS-AES → NAS EuMrx redirector — `architecture/05-scan-engine-architecture.md:116-125`;
  `epmntdrv-trio--naming-pid-handshake.md:103-123`.
- **Lost partitions as scan targets**: "Insert lost partition %s, %d, StartSector=%I64d,
  TotalSector=%I64d" trace; sector size default 512 — `architecture/04-recovery-workflow.md:66-69`.
- **Scan-abort mechanics**: manual-reset Win32 event at CScanMgr+0x108 + `CStopScanThread`
  cooperative cancel (`UIUX/13-ui-core-wiring-state.md:42,63`).

## 10. Explicit unknowns (corpus does NOT document)

1. **UAC prompt appearance** — no dynamic capture of the consent dialog (all CI runs were
   admin); only the manifest posture is proven (§3).
2. **Unlettered/no-drive-letter volume presentation** in the device list — not documented;
   lost-disk page covers whole physical disks, "File Path Lost" covers pathless results.
3. **Quick→Advanced percent handoff constant** — no such constant exists in the corpus; advanced
   scan demonstrably runs its own 0-100% (9%→34% captures, §6).
4. **Outer installer (Inno wrapper) elevation posture** — binary absent from corpus.
5. **AnalysisGridWidget rendering rules** (sector-grid visualization) — census'd (54 v-methods)
   but not decoded (`UIUX/08-scan-phase-deep-dive.md:11`).
6. **DiskHealth SMART attribute list beyond the column schema** (which raw attributes are shown
   per transport) and behavior for devices with no SMART — the companion artifact documents the
   engine classes and column headers, not the row set (`UIUX/19-diskhealth-quick-ui.md:24-30`).
7. **DrwMessageBox size** — 540×240 (decompile-derived, UIUX 05) vs 578×302 (shipped QSS
   style.css:819-827): unresolved discrepancy; QSS preferred.
8. **Intra-window carve progress / heartbeat cadence numbers** for the og engine — the corpus
   proves the *UI-level* tickers move continuously during advanced scan (§6) but does not
   document the engine's internal publish interval.

## 11. Where final-prod most plausibly diverges from this reference (reading-time assessment)

1. **Privilege**: no `requireAdministrator` manifest → asInvoker + in-app restart-as-admin
   banner, vs reference's single OS-managed UAC with zero in-app elevation UI (§3; defect D-PRIV-01/D-UI-01).
2. **Enumeration**: internal-only Home list + removable→"usb" kind degradation, vs reference's
   always-both "Internal drives (N)"/"External drives (%1)" groups, SD first-class page, and
   card fields icon+name+used/total bar (§4; D-ENUM-01).
3. **Health**: visible `cmd.exe` console per health read (`_popen` fsutil), vs reference's
   hidden-window fsutil spawn (wShowWindow=0, `DRWUI_decompiled.c:178725-178767`) + native SMART
   companion with ID/Attribute/Current/Worst/Threshold/Raw/Status table (§5; D-CONS-01).
4. **Scan progress**: "Advanced Scanning 75% frozen, files found N (0 B), no sector progress"
   vs reference's simultaneously-moving percent gauge + live count+cumulative-GB + "Reading
   sector: X/Y" + "Searching: <path>" ticker + live-growing tree (§6; D-SCAN-01).
5. **Found-bytes seam**: no live bytes-found in the ABI, vs reference showing "(143.74 GB)"
   live next to the file count (§6; D-SCAN-01 sub-cause 3).

## 12. Most load-bearing corpus docs for fixing the scan pipeline

1. `re-workspace/artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md` (+ its two
   addenda) — the ONLY runtime evidence of progress axes, tickers, timings, live-tree growth.
2. `architecture/04-recovery-workflow.md` §3-5 — the engine wiring (quick vs advanced searcher
   sets, RAW gate, lost-partition insertion) any scan-fix must preserve.
3. `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` — vendor's own scan
   session: quick-pass timing, 3-tree creation, ~1000-item incremental batch cadence.
4. `re-workspace/artifacts/UIUX/30-interaction-driver-ground-truth.md` — completion pipeline
   (Freeze→auto-rsf→Unfreeze→ScanFinished→bottomarea 838/856→recoverability pass) with verbatim
   telemetry.
5. `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` + `rawsearch-carving--deep-dive.md`
   (+ CORRECTIONS.md rows 1/2/7/9) — engine-side correctness contracts for MFT walk, viability
   gate, carving registry, and the structured-wins merge rule.
