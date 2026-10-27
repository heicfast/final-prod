# REFERENCE-FOOTPRINT — EaseUS Data Recovery Wizard (DRW): Installed Footprint, Services, Registry & Installer Behavior

> Task T4-C (Agent C) evidence sheet. Reference product = **EaseUS Data Recovery Wizard
> ("DRW")**, as captured in the research corpus `/home/z/my-project/easeus-drw-research`
> @ `03ad46ccd6086000e53d7d3871ef43cd7f32e462`.
> Primary evidence: `og-codebase/` (the reference product's captured install tree — the
> footprint source), `re-workspace/inventory.{json,csv}` (565 PE binaries catalogued),
> `architecture/*.md` (9 distilled docs), `re-workspace/artifacts/**` (deep .md docs quoting
> decompiled/strings evidence), plus raw strings under `re-workspace/reversed/<bin>/`.
> Read-only task: nothing outside `final-prod/docs/` was written.
> Companion sheets: `docs/REFERENCE-MODULES.md` (module architecture, this task) and the
> earlier frozen sheets `docs/context-sheets/REFERENCE-LAYOUT.md` (D1) and
> `docs/context-sheets/REFERENCE-INSTALLER.md` (D2).

**Citation convention**: `[corpus-path]` = file in the research corpus. Quotes of
decompiled code / strings are byte-exact as documented in the cited artifact.

---

## 0. Scope & capture caveats (read first)

1. **`og-codebase/` is a captured, post-install + post-run tree**, not a pristine installer
   payload: it contains runtime-mutated state — vendor QA logs (`log/DRWUI.log`,
   `CalcPartition.log`, `DefaultLog.txt`; root `EPMLOG.log`, `Eaolog.log`,
   `strutureReg.log`, `reglog_win.txt`), 3 saved scan sessions (`SaveScan/*.rsf`), a
   runtime-mutated `config/version.ini` (`lastUpdateTs`, `InitActive`), and a packaging
   list (`needcopy.txt`). Evidence: `re-workspace/artifacts/config-files/qa-logs--session-forensics.md`
   (§1 "the shipped tree contains the vendor's own QA night"); `artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md` §4.
2. **`BUILDPE/` is a second, self-contained copy** of the product (the bootable WinPE
   "Emergency Disk" payload) — 194 files. It is *media content*, not part of the live
   install footprint, but it IS shipped on the install medium and is documented here
   because it carries 4 of the 7 kernel drivers. Evidence: `architecture/03-winpe-boot-environment.md`.
3. **NOT CAPTURED (do not invent)**: a live-machine registry diff, `C:\ProgramData`
   layout, Start-menu/desktop shortcut set, the main Inno Setup EXE/script itself, and
   driver-file timestamps inside `System32\Drivers` on a real machine. Only what the
   binaries *contain* (strings, imports, manifests, scripts in-tree) is evidenced.

---

## 1. Product identity

| Fact | Value | Citation |
|---|---|---|
| Product / internal name | EaseUS Data Recovery Wizard; internal project **DRWIntelligentScan** | `architecture/01-build-tree-intelligence.md` §1.1 |
| Shipped build | **21.0.0.0**, edition `free`, channel `easeus`, market `abroad`, `product-code=RE-DRW-FREE-WIN`, `SetupBuildTime=1013`; build commit `40504ccb…` built Oct 13 2026 | `og-codebase/config/version.ini` `[ver]`; `artifacts/config-files/qa-logs--session-forensics.md` §1 |
| Install root (live Windows) | `\Program Files\EaseUS\DRW` — literal UTF-16 strings in DRWUI: `\Program Files\EaseUS\DRW`, `\Program Files\EaseUS\DRW\`, `\Program Files\EaseUS\DRW\PartitionRecovery` | `re-workspace/reversed/DRWUI.exe/strings_utf16.txt` (lines 15424–15615) |
| WinPE boot path | `x:\Program Files\easeus\DRW\DRW.exe` (launched by `winpeshl.ini`) | `og-codebase/BUILDPE/x64/Windows/system32/winpeshl.ini` (byte-exact) |
| Sibling-path references | `\Program Files\EaseUS\BootRepair`, `\EaseUS\DRW\CSBackup.exe`, `C:\Program Files\EaseUS\BL` (bitlockerhelp.mo), `%AppData%\EaseUS\DRW\connect.zip` (NAS agent download) | DRWUI `strings_utf16.txt` 15426 / `strings_ascii.txt` 10256; `re-workspace/reversed/ghidra-mo/bitlockerhelp_decompiled.c` line 5463–5682; `artifacts/EuShare/eumrx-nas-architecture.md` §3.3 |

---

## 2. Footprint totals (measured from the corpus tree)

| Area | Files | Apparent size | PE binaries |
|---|---|---|---|
| **Live install tree** (`og-codebase/` excl. `BUILDPE/`) | **1,276** | **518,749,215 B ≈ 519 MB (495 MiB)** | 435 |
| — install root (flat) | 149 (24 EXE + 99 DLL incl. 40 `api-ms-*` + 26 data/config/manifest/log files) | — | 123 |
| — 17 subdirectories | 1,127 | — | 312 |
| **BUILDPE/** (bootable-media payload) | 194 | 184,084,980 B ≈ 184 MB | 130 |
| — `BUILDPE/EaseUS-x64/` (app copy in WIM) | 180 | ≈158 MB | 116 |
| — `BUILDPE/x64/` (PE OS overlay: drivers + shell config) | 14 | ≈12 MB | 4 (.sys) |
| **Whole captured tree** | **1,470** | **702,834,195 B ≈ 703 MB (670 MiB)** | **565** |

Measured by `find`/`du` over `og-codebase/`; PE counts from `re-workspace/inventory.json`
(565 PE total; class field: 1,244 `easeus` + 226 `external` files across all 1,470).

Largest single files (live tree): `VideoViewer/avcodec-62.dll` 58.4 MB, `SaveScan/…(H_).rsf`
52.7 MB (runtime artifact), `ResourceDark/drw_dark.rcc` 32.3 MB, `Resource/drw_light.rcc`
30.7 MB, `DRWUI.exe` 30.6 MB, `VideoViewer/avfilter-11.dll` 21.2 MB,
`EUFileRepair/EuOfficeRepairFile.exe` 10.5 MB.

---

## 3. Manifest — live install tree (`<ProgramFiles>\EaseUS\DRW\`)

### 3.1 Install root — product executables (all EVIDENCED; per-binary roles in REFERENCE-MODULES.md)

| File | Size | Class | Evidence |
|---|---|---|---|
| `DRW.exe` | 127,208 | **DRWLoader** supervisor (entry point) | `artifacts/DRW-exe--launcher.md` |
| `DRWUI.exe` | 30,571,752 | Main Qt UI application ("the product") | `artifacts/DRWUI-exe--main-ui-architecture.md` |
| `DRWService.exe` | 40,168 | BitLocker FVE key service binary | `artifacts/DRWService-exe--bitlocker-service.md` |
| `DRWReport.exe` + `DRWReport.ini` | 4,805,352 + 515 | Crash reporter (XCrashReport build); .ini lists files to attach (log\*.log, Drwdump0-5.DMP) | `artifacts/DRWReport-exe--crash-reporter.md`; `og-codebase/DRWReport.ini` |
| `SetupUE.exe`, `InfoForSetup.exe`, `unins000.{exe,dat,msg}` | 177,384 / 117,776 / 1,259,408+513,373+22,791 | Installer bootstrap pair + Inno Setup uninstaller trio (live-tree only) | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` |
| `EuDownloader.exe` | 1,381,608 | Range/MD5 downloader (WAIK ISO carve, agent zips, holiday resources) | `artifacts/config-files/config-data-reference.md` (dlconfig.xml consumer); `per-binary/EuDownloader.md` |
| `EuRepairHost.exe` | 23,784 | Repair licensing bridge (activation.easeus.com) | `artifacts/FileRepair/repair-architecture.md`; `Licensing/licensing-activation-chain.md` |
| `EUImgRepair.exe`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`, `EuOfficeRepairWin32.exe`, `EuOfficeTools.exe`, `EUUnZip.exe`, `EURawImg.exe` | 203–2,739 KB | Fixo repair chain (root copies of the Win32 repair engines) | `architecture/06-repair-chain.md` §1; `per-binary/*` |
| `EUOfficeViewer.exe`, `PDFViewer.exe`, `EUHwpViewer.exe` (+ `EUHwpFilePathCheckerModule.dll`) | 3.0 / 3.0 / 0.4 MB | Out-of-process preview viewers (Office / PDFium / Korean HWP) | `architecture/04-recovery-workflow.md` §6 |
| `EUinApp.exe` | 37,608 | In-app purchase / store bridge (orphaned IE-emulation bootstrapper in this build) | `architecture/05-uiux-architecture.md` §11 (artifact 23 role correction); `Licensing/licensing-activation-chain.md` |
| `EuUserRate.dll` | 1,619,176 | Rate/recommendation prompt (live-only) | `artifacts/WinPE/pe-payload-census…` §2 |
| `firebasefetch.exe`, `firebaseonlineconfig.dll` | 90,344 / 1,155,816 | Firebase remote-config fetch pair (out-of-proc + DLL) | `artifacts/Cloud/cloud-recovery-architecture.md` §4 |
| `AliyunWrap.dll`, `AliyunWrapExe.exe`, `send_dmp_with_aliyun.exe` | 0.6 / 0.2 / 0.1 MB | Aliyun OSS upload channel (crash dumps/telemetry) — live-only | `artifacts/DRWReport-exe--crash-reporter.md`; PE census §2 |
| `cloud_session.dll`, `cpprest141_2_10.dll` | 943,336 / 3,756,032 | OAuth2 cloud-drive sessions (C++ REST SDK) — live-only | `artifacts/Cloud/cloud-recovery-architecture.md` §2 |
| `EuICloudRecovery.dll` | 7,187,688 | Full iCloud client (esicloud, SRP-6a verified) | same artifact §3+§8 |
| `PE_Tools.dll` (+ `PEtools.xml`, `dlconfig.xml`) | 976,104 | WinPE bootable-media builder (WAIK download + WIM inject + ISO write) | `architecture/03-winpe-boot-environment.md` §4 |
| `DiskHealth.exe` (in `diskhealth/`), `7za.exe`, `WinChkdsk.exe` (in `PartitionRecovery/`) | — | Companions: SMART monitor (CrystalDiskInfo lineage), standalone 7-Zip archiver (external, x86 console), MOM chkdsk wrapper (`mod.winchkdsk`, runs real chkdsk w/ VSS coordination) | SMART: `artifacts/DiskHealth/diskhealth--smart-companion.md`; 7za: `re-workspace/inventory.csv` row 1 (external, CONSOLE); WinChkdsk: `per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §2 |
| `Japanese.dll` | 4,206,312 | Win32 ja-JP resource satellite (dialogs/bitmaps/strings, lang 1041) | `per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §1 |
| `archive.dll`, `legacy.dll`, `PubLog.dll`, `GDIPLUS.DLL` | 688,128 / 101,888 / 29,928 / 1,773,568 | Archive lib (1,353 fns); legacy crypto (libtomcrypt); publish-log; GDI+ local copy | `per-binary/{archive,legacy,PubLog}.md` |

### 3.2 Install root — scan engines (in-proc plugins; DRWUI `LoadLibraryW`s all 10 at startup)

| File | Size | Engine | Evidence |
|---|---|---|---|
| `NTFSSearchDLL.dll` | 577,256 | NTFS (MFT walk) | `artifacts/SearchDLL/searchdll-family--architecture.md` |
| `FATSearchDLL.dll` | 469,224 | FAT12/16/32 | same |
| `exFATSearchDLL.dll` | 413,416 | exFAT | same |
| `EXT4SearchDLL.dll` | 129,256 | ext2/3/4 (inode/extent) | same |
| `HFSPlusSearchDLL.dll` | 185,064 | HFS+ (B-trees) | same |
| `ReFSSearchDLL.dll` | 154,856 | ReFS | same |
| `BtrfsSearchDLL.dll` | 150,760 | Btrfs (zlib/RC4) | same |
| `ZfsSearchDLL.dll` | 275,176 | ZFS (uberblock/DMU) | same |
| `RAWSearchDLL.dll` | 644,840 | RAW carving (190+ analysers) | same §4; `rawsearchdll--analyser-registry-census.md` |

(Evidence for "always loaded at startup": DRWUI `FUN_14056c0d0` loads all 10 into globals
`DAT_1425e6d30..78` — `artifacts/DRWUI-exe--main-ui-architecture.md` §9.2.)

### 3.3 Install root — repair/media/preview support DLLs

`EUVideoRepair.dll` (948,456; in-proc video repair, 7,154 fns), `EUImageEnhance.dll`
(3,056,872; OpenCV 2.4 static), `heif.dll`, `de265.dll`, `libx265.dll` (HEIF/HEVC codecs),
`pdfium.dll` (4,119,552), `dbghelp.dll` (crash minidumps), `libzmq-mt-4_3_5.dll`
(preview IPC), `libssh2.dll` (NAS SSH-2.0-libssh2_1.11.0), `liblzma.dll`, `zlib.dll`,
`zlib1.dll`. Evidence: `architecture/06-repair-chain.md`; `artifacts/DRWUI…` §9.5;
`eumrx-nas-architecture.md` §3.2.

### 3.4 Install root — runtimes (all EVIDENCED via inventory + runtime-families)

| Family | Files | Evidence |
|---|---|---|
| Qt 5.15.2 (Widgets stack) | `Qt5Core/Gui/Network/Svg/Widgets/WinExtras.dll` (25.7 MB total) | `runtime-families/qt5-core-stack.md`; build provenance `architecture/01` §1.6 |
| OpenSSL — **4 generations in one tree** | `libeay32.dll`+`ssleay32.dll` (0.9.8/1.0.x names), `libeay32MD/ssleay32MD`, `libssl/libcrypto-1_1[-x64]`, `libssl/libcrypto-3[-x64]` (10 files) | `architecture/01` §1.6; PE census §1 "THREE OpenSSL generations coexist" |
| VC runtimes — 4 generations (VS2008→VS2015/17) | `msvcm90/msvcp90/msvcr90` (+2 SxS manifests), `msvcp120/msvcr120`, `msvcp140/vcruntime140/concrt140` | `runtime-families/vc-runtime-generations.md`; `architecture/01` §1.6 |
| UCRT self-carriage | `ucrtbase.dll` + 40 `api-ms-win-{core,crt}-*` forwarder DLLs at root (49 in PE copy) | `runtime-families/ucrt-forwarders.md`; PE census §1 |
| Media (preview/repair) | VideoViewer ffmpeg-6x set (in `VideoViewer/`), GDIPLUS.DLL, heif/de265/libx265 | `runtime-families/media-codecs.md` |

### 3.5 Install root — config / data files (formats decoded in corpus)

| File | Size | Content (decoded) | Citation |
|---|---|---|---|
| `FileFilter.xml` | 54,597 | 8 categories / **1,489 extensions** (typo `<Extention>` original) | `artifacts/config-files/config-data-reference.md` |
| `raw_signs.json` | 35,342 | RAW signature DB #1: 139 ext, header+tail hex, confidence 0.50–1.00, min/max | same |
| `EUUserData` | 16,891 | RAW signature DB #2 (magic+MIME, libmagic-style) | same |
| `Dict` | 2,742 | 228-entry password brute-force dictionary | same (byte-verified) |
| `DataFile.ini`, `Upgrade.ini`, `recoveryFree.ini` | 84 / 235 / 128 | UTF-16 counters; upgrade pointer (20.8.0.0, `killDRWUI=1`); free-quota state | same; `og-codebase/Upgrade.ini` |
| `AliyunConfig.ini` | 1,802 | Alibaba OSS upload config | same |
| `google-services.json` | 997 | Firebase project config | same |
| `remote_config_data` | 62,579 | Fetched Firebase remote-config snapshot (feature flags) | same |
| `style.css` | 437,565 | QSS for the product UI (20,802 lines; 118-slot palette via config/themdark.ini) | `architecture/05-uiux-architecture.md` §1 |
| `dlconfig.xml`, `PEtools.xml` | 16,202 / 12,756 | WAIK ISO carve manifest (byte-ranges+MD5); WAIK deployment manifest | `config-data-reference.md`; `architecture/03` §4 |
| `needcopy.txt`, `auto.txt` | 1,670 / 316 | Packaging list (Inno source list of CRT shims); opaque GUID blob | `og-codebase/` (raw) |
| `EPMLOG.log`, `Eaolog.log`, `strutureReg.log`, `reglog_win.txt`, `icloud_logger` | — | Vendor registration/installer + QA session logs (install-time + run-time forensics) | `artifacts/config-files/vendor-logs--qa-session-forensics.md` |
| `log/` (4 files) | 120 KB | Runtime logs of the vendor QA night (DRWUI.log, CalcPartition.log, DefaultLog.txt, empty DRWLog.log) | `artifacts/config-files/qa-logs--session-forensics.md` |

### 3.6 Subdirectories (live tree)

| Directory | Files | Size | Contents | Evidence |
|---|---|---|---|---|
| `config/` | 9 | 44 KB | `version.ini`, `config.ini` (22 languages, window 1240×840, ENS buy-button coords), `themdark.ini` (118-slot palette), `FilterFolder.ini` (junk blocklist), `RawConfig.ini` (37 carve switches), `usefullostfiles.xml`, `PriorityDisplayType.xml`, `nasconfig.ini`, `shellexe.ini` | `config-data-reference.md`; `architecture/05-uiux-architecture.md` §4 |
| `language/` | 37 | 6.2 MB | 26 product `.qm` (1,556 strings each; 4 Mainland OEM sub-brands incl. `Mainland_Foxit/JZ/LC`) + 11 `qtbase_*.qm` | `artifacts/Languages/language-packs--family-census.md` |
| `Resource/` + `ResourceDark/` | 2 + 2 | 63 MB | Theme RCC bundles `drw_light.rcc` (30.7 MB) / `drw_dark.rcc` (32.3 MB) + `ImageResource.xml` (~7,400 assets, 105 filetype SVGs) | `artifacts/UIUX/01-visual-inventory`; `architecture/05-uiux` §1 |
| `platforms/`, `styles/`, `imageformats/`, `iconengines/` | 3+1+3+1 | 3.8 MB | Qt plugins: qwindows/qoffscreen/qminimal, qwindowsvistastyle, qgif/qjpeg/qsvg, qsvgicon | PE census §1 |
| `PartitionRecovery/` | 57 | 27 MB | **30 `.mo` MOM modules + host DLLs (PartitionRecovery, MainModule, RecoveryModule, ToolsModule, UILogic, CloneModule, RawFixer, BitLockerLib, Win32Bcd, Boot, BootDriver, Burn, AutoUpdate, Logger, peformat, GetDriverInfo, libvmdk, zlib×2, Qt5Core) + `eprdtdrv.sys` + `eprdtdrv_win7older.sys` + `setupdrvx64.exe` + `WinChkdsk.exe` + `setupdrv.ini` + `lostpart.dat`/`partinfo.dat`** | `architecture/01` §1.3; `artifacts/PartitionRecovery/mom-module-system--architecture.md` |
| `EuShare/` | 3 | 224 KB | **`EuMrx.sys`** (NAS redirector) + `install.bat` + `uninstall.bat` (service install scripts) | `artifacts/EuShare/eumrx-nas-architecture.md` §2.6 |
| `NasConfig/` | 2 | 16 KB | `NasModel.xml` (Synology model→arch catalog), `ssh_config.ini` (agent push state, `PushPath=/tmp/EaseUS/`) | same §3.3 |
| `License/` | 19 | 160 KB | Third-party license compliance set (Qt, OpenSSL, pdfium 62 KB, ImageMagick… ) | `config-data-reference.md` |
| `EUFileRepair/` | 87 | 46 MB | Fixo engine suite + ImageMagick: `EUImgRepairFile.exe`, `EuOfficeRepairFile.exe`, `EuPDFRepairFile.exe`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`, `7z.dll`, 24× `CORE_RL_*` ImageMagick, 7× `IM_MOD_RL_*` delegates, heif/libde265/libcurl/OpenSSL-3, own UCRT set | `architecture/06-repair-chain.md`; `runtime-families/imagemagick-modules.md` |
| `VideoViewer/` | 76 | 104 MB | `EaseUS_VideoViewer.exe` (preview helper), `EaseUS_MediaGraph.dll`, `VideoFileRepairer.exe`, `ffmpeg.exe`, ffmpeg-6x DLLs (`avcodec-62` 58 MB, `avfilter-11`, `avformat-62`, `avutil-60`, `swscale-9`, `swresample-6`, `postproc-58`, `avdevice-62`), UI PNG/cursor assets, own CRT set | `per-binary/{EaseUS_VideoViewer,VideoFileRepairer,EaseUS_MediaGraph}.md`; `architecture/01` §1.1 |
| `diskhealth/` | 818 | 55 MB | `DiskHealth.exe` (Qt Quick/QML SMART companion, CrystalDiskInfo lineage) + full QML plugin tree (QtQuick 622 files), `DiskHealth.rcc`, task .bats, own Qt5/CRT set | `artifacts/DiskHealth/diskhealth--smart-companion.md` |
| `SaveScan/` | 3 | 69 MB | `auto_Local Disk (G_/H_/I_)*.rsf` — saved scan sessions (runtime artifacts, vendor QA scans) | `artifacts/SaveScan/rsf-format--scan-session.md` |

### 3.7 Files placed OUTSIDE the install directory (live Windows)

| Target | File | Mechanism | Citation |
|---|---|---|---|
| `%SystemRoot%\System32\Drivers\eprdtdrv.sys` (and versioned `%s%d.sys` variants) | driver | `setupdrvx64.exe` copies + `CreateServiceA`/`StartServiceA` | `architecture/02-kernel-driver-architecture.md` §5; `per-binary/deepening-batch1…` §3 (strings: `\System32\Drivers\%s.sys`, `%s\System32\Drivers\%s%d.sys`, "CreateService failed!  Error = %d", "Boot File System") |
| `%SystemRoot%\System32\DRIVERS\EuMrx.sys` | driver | `EuShare/install.bat`: `copy /Y "%~dp0EuMrx.sys" "%systemroot%\system32\DRIVERS"` | `og-codebase/EuShare/install.bat` (byte-exact) |
| `%AppData%\EaseUS\DRW\connect.zip` (+ extracted agent) | runtime download | NAS/Linux agent fetch (d1.easeus.com), extracted by `EUUnZip.exe` | `artifacts/EuShare/eumrx-nas-architecture.md` §3.3 |
| `<install dir>\DRWCfg.bin` | runtime-written | AES-256-CBC config cache (hardcoded key) written by DRWUI — **not present in captured tree** | `artifacts/config-files/drwcfg-bin--aes-config-cache.md` §2 |
| `C:\Program Files\EaseUS\BL`, `…\BootRepair`, `…\CSBackup.exe` | referenced sibling installs | strings in bitlockerhelp.mo / DRWUI (presence on disk NOT CAPTURED) | `re-workspace/reversed/ghidra-mo/bitlockerhelp_decompiled.c` 5463+; DRWUI strings |
| Desktop shortcuts (PE only) | "Data Recovery Wizard" → `x:\program files\EaseUS\DRW\DRW.exe` etc. | `auto.bat` Phase D | `architecture/03` §2; `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` |

**NOT CAPTURED**: any `C:\ProgramData\EaseUS` layout (DRWUI contains the bare string
"ProgramData" next to "Program Files (x86)"/"Program Files" in a path-enumeration context —
`re-workspace/reversed/DRWUI.exe/strings_utf16.txt` line 14976 — but no concrete
ProgramData subpaths exist in the corpus), Start Menu group, scheduled tasks
(`diskhealth/addtask.bat` / `querytask.bat` / `deltask.bat` exist — schtasks-style task
management for DiskHealth tray — but the corpus does not capture the resulting task set).

---

## 4. Services (complete list the product can install)

### 4.1 Live-Windows services

| # | Service (SCM name) | Binary | Type | Start | Account | Install mechanism | Citation |
|---|---|---|---|---|---|---|---|
| 1 | **"Read FVE Server"** (display = service name) | `DRWService.exe` (full path at install time) | `SERVICE_WIN32_OWN_PROCESS` (0x10) | **`SERVICE_AUTO_START` (2)** — "present at every boot; harmlessly idles when DRW isn't running" | LocalSystem | DRWUI runs `<install>\DRWService.exe /install` → `OpenSCManagerA(SC_MANAGER_ALL_ACCESS)` → `CreateServiceA(...)` → `StartServiceA` (decompiled `FUN @1205-1270`); description string "DRW read registry key value."; uninstall via `/uninstall` → stop+`DeleteService` | `artifacts/DRWService-exe--bitlocker-service.md` §1 (CreateServiceA call cited line 1234); DRWUI strings `\DRWService.exe /install`, `/uninstall` (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt` 25749-25753) |
| 2 | **`eprdtdrv`** (service name = file base name; internal driver name EPMNtDrv) | `System32\Drivers\eprdtdrv.sys` | kernel driver (`Type 1`) | demand (started at install; re-created "on first scan needing raw access") | SYSTEM | `PartitionRecovery/setupdrvx64.exe` (PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`): copy to `\System32\Drivers\%s.sys` (+ `%s%d.sys` versioned variant), `CreateServiceA` w/ group "Boot File System", `StartServiceA`; `-Uninstall` mode; registry write+verify under `SYSTEM\CurrentControlSet\Services\%s`. `setupdrv.ini` contains `[eprdtdrv]` | `architecture/02` §5; `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §7; `per-binary/deepening-batch1…` §3 (strings verified) |
| 3 | **`EuMrx`** | `system32\DRIVERS\EuMrx.sys` | file-system driver (`sc type= filesys`) | **demand** (`start= demand`), group `Network` | SYSTEM | `EuShare/install.bat` (byte-exact): `sc create EuMrx binPath= "system32\DRIVERS\EuMrx.sys" type= filesys start= demand error= normal group= Network DisplayName= EuMrx`; `sc description EuMrx "MiniMrx Driver"`; **also registers a Windows Network Provider** ("EaseUS Network", `\Device\EuRdr`, ProviderPath `System32\EuMrxnp.dll` — NP shim referenced but NOT shipped, "driver-only drop") and **prepends `EuMrx` to `NetworkProvider\Order\ProviderOrder`**. Uninstall mirrors (sc stop/delete, file delete, ProviderOrder cleanup) | `og-codebase/EuShare/install.bat`; `artifacts/EuShare/eumrx-nas-architecture.md` §2.6 |
| 4 | `eprdtdrv_win7older` | `eprdtdrv_win7older.sys` | kernel driver | demand; deployed for Windows 7- (legacy WDK ABI build) | SYSTEM | same setupdrvx64 mechanism (service name from `setupdrv.ini`/filename) | `architecture/02` §1-2 (PDB-identical build, 27,272 B) |

### 4.2 WinPE-only driver services (registered by `BUILDPE/x64/regdrv.reg` at media build; NOT installed on live Windows)

| Service | Type | Start | ImagePath (in WIM) | Extra | Citation |
|---|---|---|---|---|---|
| `epmdkdrv` | 1 (KERNEL_DRIVER) | 3 (demand) | `\SystemRoot\System32\epmdkdrv.sys` | EPMNtDrv raw-disk proxy | `architecture/03` §3 (regdrv.reg decoded) |
| `ebrntdrv` | 1 | 3 (demand) | `\SystemRoot\System32\ebrntdrv.sys` | Group="Boot File System" | same |
| `EUDCPEPM` | 1 | **0 (BOOT_START)** | `system32\drivers\EUDCPEPM.sys` | DeviceName=`epmdiskcopydrv`, **IoctlBase=0x10000**; **UpperFilters=EUDCPEPM on GUID_DEVCLASS_DISKDRIVE** — attaches as class upper filter to every disk | same; `architecture/02` §3 |
| `EUEDKEPM` | 1 | 1 (SYSTEM_START) | `system32\drivers\EUEDKEPM.sys` | DeviceName=EUEDKEPM (`\Device\EUDKEPM`), IoctlBase=0x10000 (unused) | same; `architecture/02` §7 |

The 7 kernel drivers, one table (sizes from tree): `eprdtdrv.sys` 27,272 ·
`eprdtdrv_win7older.sys` 27,272 · `ebrntdrv.sys` / `epmdkdrv.sys` (BUILDPE, 27,272 each,
same PDB `epmntdrv.pdb` — "SAME family, THREE compilations") · `EUDCPEPM.sys` 72,144 ·
`EUEDKEPM.sys` 19,408 · `EuMrx.sys` 215,704. All WDK `fre_wlh_amd64` builds,
Authenticode-signed "Chengdu Yiwo Tech Development Co., Ltd".
(`architecture/02-kernel-driver-architecture.md` §1; `artifacts/PartitionRecovery/epmntdrv-trio--byte-diff-verification.md`.)

---

## 5. Registry keys (everything the corpus evidences)

### 5.1 Product keys (HKLM\SOFTWARE)

| Key | Read/Write | By | Purpose | Citation |
|---|---|---|---|---|
| `SOFTWARE\EASEUS\DRW\Version` | **write + read** | SetupUE (installer bootstrap); read by DRWUI (many string refs) and auto.bat PE-side for sibling key `SOFTWARE\EASEUS\EPM\Version\OEM` | product version + OEM branch ("Mainland" vs international) | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1; DRWUI `strings_utf16.txt` (≥8 refs); `architecture/03` §6 |
| `SOFTWARE\EASEUS\DRW\Language` | read | DRW.exe loader (`RegOpenKeyExA` line 311) → fallback `LoadLibrary` Chinese.dll/Deutsch.dll/Japanese.dll | UI language selection | `artifacts/DRW-exe--launcher.md` §1 step 5 |
| `SOFTWARE\EaseUS\DRW\Retention` | read/write | DRWUI (≥12 string refs; `value_uninstallretain172`, `value_inappuninstallretain`, `value_uninstallretain__abroad`) | uninstall/retention funnel state (A/B experiment values) | DRWUI `strings_ascii.txt` (lines 9790-20664) |
| `SOFTWARE\EASEUS\Fixo` · `SOFTWARE\EaseUS\ChronoSnap` · `SOFTWARE\EaseUS\DiskHealth` | read | DRWUI | cross-product/companion detection | DRWUI `strings_utf16.txt` (Fixo, ChronoSnap; DiskHealth in ascii set) |
| `SOFTWARE\EASEUS\EaseUS Data Recovery Wizard\EUImgRepair` | read/write (`FullFileName`, `FullRepairedFileName`) | DRWUI ↔ image-repair chain | window-IPC + registry invocation chain for repair | DRWUI `strings_utf16.txt` 14896-14898; `artifacts/FileRepair/image-repair--format-grammar.md` |
| `SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` (+ `Software\Wow6432Node\...`) | enum | DRWUI | installed-software detection (uninstall-retention triggers) | DRWUI `strings_utf16.txt` |
| `HKCU\Software\CodeProject\XCrashReportTest` | residue | DRWReport | XCrashReport provenance marker | `artifacts/DRWReport-exe--crash-reporter.md` |
| `HKCU\…\FEATURE_BROWSER_EMULATION` (9000/10000/11000) | write | EUinApp (orphaned in this build) | IE-version bootstrap for embedded webviews | `architecture/05-uiux-architecture.md` §11 artifact 23 |

### 5.2 System keys touched

| Key | Operation | By | Citation |
|---|---|---|---|
| `HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock` (each subkey's `Data` value) | enum + read | DRWService (needs SYSTEM) — the service's entire purpose | `artifacts/DRWService-exe--bitlocker-service.md` §2 |
| `HKLM\SYSTEM\CurrentControlSet\Services\%s` (eprdtdrv / EuMrx / …) | write + verify | setupdrvx64; install.bat | driver service registration | deepening-batch1 §3; `EuShare/install.bat` |
| `HKLM\SYSTEM\CurrentControlSet\Services\EuMrx\Parameters\LogRate`, `…\EuMrx\NetworkProvider\{DeviceName,Name,ProviderPath}` | write | install.bat | redirector config | `og-codebase/EuShare/install.bat` |
| `HKLM\SYSTEM\CurrentControlSet\Control\NetworkProvider\Order\ProviderOrder` | prepend `EuMrx` / cleanup on uninstall | install.bat / uninstall.bat | make UNC resolution consult the redirector | same |
| `HKLM\SYSTEM\CurrentControlSet\Services\BitLocker`-adjacent FVE keymgmt | read | BitLockerLib (`manage-bde`/`Unlock-BitLocker` PowerShell fallback + registry fallbacks) | `architecture/02` §6 map |
| PE-only: `HKLM\PE_Sys\ControlSet001\…\Services\{epmdkdrv,ebrntdrv,EUDCPEPM,EUEDKEPM}` + `Control\Class\{71A27CDD-…}` `UpperFilters=EUDCPEPM` | import at media build | `regdrv.reg` | `architecture/03` §3 |

**NOT CAPTURED**: a full before/after registry diff of a real install (the corpus holds
only in-binary strings + the PE `regdrv.reg`); any `Uninstall` key the Inno installer
writes for DRW itself; license-data registry values (`writeActivateData` is named in the
licensing artifact but the key/value layout is NOT-IN-CORPUS).

---

## 6. Installer behavior

### 6.1 What the installer is

- **Inno Setup-based**: `unins000.exe` + `unins000.dat` + `unins000.msg` is the Inno
  uninstaller trio ("unins000 naming + .dat/.msg data files are Inno's signature");
  `unins000.exe` is a 2016-era VS2008 binary (build timestamps, `architecture/01` §2).
  `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1.
- **SetupUE.exe = pre-install bootstrap** (x86 GUI, 24,545-line decompilation): 16-OS
  version gate ("Microsoft Windows 2000"…"Server 2016", editions incl. Itanium/x64) via
  `GetNativeSystemInfo` + `RtlGetNtVersionNumbers`; reads/writes
  `SOFTWARE\EASEUS\DRW\Version`; **launches InfoForSetup.exe**; owns the `DRW_Installer`
  window (the WM_COPYDATA cmd-999 target of EuSendMessage.exe in PE contexts). Same artifact §1.
- **InfoForSetup.exe = install-time telemetry collector**: fields `Activity`,
  `Attribute`, `Country`, `Timezone`, `Install`, `Test_id`/`Testid` (A/B bucketing);
  upload verb `/SendInfo "..."`. Same artifact §2.
- The main Inno setup EXE and its script are **NOT IN CORPUS** (only the uninstaller +
  bootstrap pair shipped in the installed tree). Silent/`/VERYSILENT` flag documentation:
  **NOT-IN-CORPUS** (Inno defaults would apply — inference, not evidence).

### 6.2 Elevation model (the privilege boundary — evidence)

| Binary | Manifest `requestedExecutionLevel` (byte-exact from `.rsrc`) | Meaning |
|---|---|---|
| `DRW.exe` (DRWLoader) | **`requireAdministrator`** | product entry point demands UAC elevation | 
| `DRWUI.exe` | **`requireAdministrator`** | main app re-asserts elevation (any direct launch also elevates) |
| `DRWService.exe` | `asInvoker` | launched by SCM as LocalSystem — no manifest elevation needed |
| `SetupUE.exe` | `asInvoker` | bootstrap runs unelevated (Inno wizard itself not in corpus) |
| `unins000.exe` | `asInvoker` (embedded manifest; Inno runtime also carries `highestAvailable`/`requireAdministrator` compiler-option strings) | uninstaller as shipped |

Citations: `re-workspace/reversed/{DRW.exe,DRWUI.exe,DRWService.exe}/strings_ascii.txt` +
`rabin2_strings_data.txt` (manifest blocks in `.rsrc`); `reversed/SetupUE.exe/strings_ascii.txt`
line 350-357; `reversed/unins000.exe/strings_ascii.txt`.

**Consequence (the reference's answer to privilege-without-PowerShell)**: the product
elevates ONCE per launch via the `requireAdministrator` UAC manifest on DRW.exe/DRWUI.exe,
then performs every privileged operation **in-process with native Win32 APIs**:
- driver install → `setupdrvx64.exe` (SCM `CreateServiceA`/`StartServiceA` + file copy —
  no shell); 
- service install → `DRWService.exe /install` (SCM APIs, decompiled);
- NAS driver → `EuShare/install.bat` using `sc.exe`/`reg.exe`/`copy` (a console script —
  whether its console window is suppressed by the invoking parent is NOT-IN-CORPUS);
- registry writes → direct `Reg*` calls.
The ONLY PowerShell documented anywhere in the corpus is **runtime BitLocker unlocking**:
`BitLockerLib.dll` shells `manage-bde -unlock … -RecoveryPassword` /
`Unlock-BitLocker -MountPoint … -Password $SecureString` as a *fallback* to the
DRWService pipe path (`artifacts/PartitionRecovery/…bitlockerlib`; `architecture/02` §6
map; `architecture/04` §4a) — i.e., PowerShell appears as an occasional elevated runtime
helper, never as the installer's elevation mechanism.

### 6.3 Install-time side effects beyond file copy (evidenced)

1. `unins000.*` written (Inno data) — per-binary/unins000.md.
2. `SetupBuildTime=1013` + `xurlPageID` stamped into `config/version.ini` by setup
   ("the live copy stamped by installer (marketing/page attribution)") — PE census §4.
3. `needcopy.txt` (packaging list: CRT shims, d3d11/dxgi, msvcrt generations) shows the
   tree was assembled by a copy-list — consistent with Inno `[Files]`-driven staging
   (inference from file content).
4. Registration logs `strutureReg.log`/`reglog_win.txt`/`Eaolog.log`/`EPMLOG.log`
   recorded install/run events (vendor forensics artifact documents their content).
5. **No scheduled-task creation, no driver-store (DIFx/`DriverPackageInstall`) usage, and
   no `pnputil` usage is evidenced anywhere** — driver deployment is strictly
   SCM-service + file copy. (Gap statement: absence of evidence in corpus.)

### 6.4 Uninstall behavior

- Inno uninstaller (`unins000.exe`) drives removal; DRWUI strings show uninstall-time
  retention triggers (`Uninstall_Retain`, `value_uninstallretain172`,
  `UninstallDlgRetentionTrigger`) — the uninstall funnel is instrumented.
  (DRWUI strings; `artifacts/Licensing/licensing-activation-chain.md` §Flow.)
- `Upgrade.ini` (`killDRWUI=1`) documents that upgrades kill a running DRWUI before
  replacing files (`og-codebase/Upgrade.ini`).
- Driver/service cleanup: DRWService `/uninstall` (stop+delete); setupdrvx64
  `-Uninstall`; `EuShare/uninstall.bat` (sc stop/delete + ProviderOrder cleanup).

---

## 7. WinPE (BUILDPE) payload summary — the second footprint

`BUILDPE/EaseUS-x64/` = **180 files**: the full app minus (verified name-level diff,
PE census §2): installer trio, 7za, Aliyun channel, cloud_session/cpprest, EuUserRate,
MD-CRT OpenSSL pair, msvcm90, and **8 whole directories** — `PartitionRecovery/` (all 30
.mo — "Zero `.mo` and zero `.rsf` files anywhere under BUILDPE, find-verified"),
`EuShare/`, `NasConfig/`, `License/`, `SaveScan/`, `EUFileRepair/`, `VideoViewer/`,
`diskhealth/` — plus **6 PE-only binaries** (EuSendMessage.exe, d3d8thk/d3d9/d3d11/dxgi
D3D shims, mfc90u.dll). Consequences (PE census §3): PE edition cannot do lost-partition
recovery (no MOM), has no NAS redirector, is FREE edition by construction
(`version=free`, `product-code=RE-DRW-FREE-WIN`).

`BUILDPE/x64/` = the PE OS overlay (14 files): `auto.bat` (6-phase shell build: ~30 .reg
imports, Chrome-as-viewer registration, desktop shortcuts), `regdrv.reg` (§4.2 driver
table), `software.reg`, `wallpaper.bmp`, `winpeshl.ini` (boot chain: wpeinit → ChangeRes →
launch auto.bat → `DRW.exe` → keep-alive), `Unattend.xml`, `system32/{ebrntdrv,epmdkdrv,msi,oledlg}`,
`drivers/{EUDCPEPM,EUEDKEPM}.sys`, `SysWOW64/{msi,oledlg}`. Citations:
`architecture/03-winpe-boot-environment.md` §1-3; `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md`.

---

## 8. Evidenced vs. inferred (ledger)

**EVIDENCED (cite-checked)**: every manifest table row above (file presence + size from
`og-codebase/`; roles from artifacts); install root path string; the 3 live services and
4 PE driver services with types/starts; all registry keys in §5 (strings/decompiled);
installer = Inno + SetupUE/InfoForSetup pair; UAC manifests (byte-exact); driver/service
install mechanisms (decompiled + script files); PE-vs-live diff; runtime artifacts
provenance (QA-night logs).

**INFERENCES (labeled)**: (a) "eprdtdrv start type = demand" — the corpus states
on-demand deployment ("on first scan needing raw access", `eprdtdrv-sys--driver-internals.md`
§7) and the PE siblings are demand-start, but the exact `CreateService` start-type
argument for live install is not quoted; (b) `needcopy.txt` ⇒ Inno staging style;
(c) Inno wizard UAC behavior (wizard EXE not captured); (d) EuMrx install.bat's console
visibility at runtime.

**NOT-IN-CORPUS (gaps)**: real-machine `System32\Drivers` post-state; ProgramData layout;
Start Menu/desktop shortcuts on live Windows (PE shortcuts ARE evidenced); full registry
diff incl. DRW's own `Uninstall` key; license-data registry layout; Inno script + silent
flags; EuMrxnp.dll (referenced `ProviderPath` — not shipped, "driver-only drop").
