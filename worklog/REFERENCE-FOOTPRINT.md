# REFERENCE-FOOTPRINT — the og's installed census & module composition

> Agent D, Phase-0 reconnaissance. Definitive sheet for the REFERENCE product's installed
> footprint and module composition, from `/home/z/my-project/easeus-drw-research` (install
> dump + RE workspace of EaseUS Data Recovery Wizard 21.0.0.0, edition `free`, channel
> `easeus`, market `abroad`, `product-code=RE-DRW-FREE-WIN`, build commit `40504ccb…`,
> built Oct 13 2026 — [OBSERVED] `og-codebase/config/version.ini`, `Upgrade.ini` 20.8.0.0
> updater marker, worklog.md resume-4 forensics).
>
> Method: fresh full walk of `og-codebase/` (every count/size below re-measured this session,
> script `/home/z/my-project/scripts/ref_footprint_census.py`), cross-checked 1:1 against
> `re-workspace/inventory.{csv,json}` (1470/1470 rows match, 0 size mismatches — the prior
> census is exactly reproduced), plus `strings`/PE-header evidence per module and direct reads
> of every config/xml/ini. Product-side numbers from `final-prod/docs/INSTALL-FOOTPRINT-CURRENT.md`
> (+ its 2026-11-05 status correction), `docs/INSTALL-FOOTPRINT-DELTA.md`, `docs/PARITY-DELTAS.md`,
> `installer/recoverywizard.nsi`, `worklog/context-sheets/REFERENCE-{LAYOUT,INSTALLER}.md`.
> Discipline: every claim tagged [OBSERVED] (measured/read this session), [INFERRED] (reasoned
> from strings/imports/naming), or [UNKNOWN]. Prior-sheet claims I re-verified are marked
> "prior claim CONFIRMED"; discrepancies are called out inline.

---

## 1 The census

### 1.1 Headline numbers [OBSERVED — full-tree walk]

| Area | Files | Bytes | MiB | Notes |
|---|---|---|---|---|
| **Whole captured tree** (`og-codebase/`) | **1,470** | **702,834,195** | 670.3 | 69 directories; 565 PE binaries (inventory.json `format=PE`) |
| **Live install tree** (excl. `BUILDPE/`) | **1,276** | **518,749,215** | 494.7 | what lands in `\Program Files\EaseUS\DRW` |
| — install root (flat) | 149 | 140,060,599 | 133.6 | 24 exe + 100 dll + 25 data/config/manifest/log |
| — 17+ subdirectories | 1,127 | 378,688,616 | 361.1 | |
| **BUILDPE/** (bootable-media payload) | **194** | **184,084,980** | 175.6 | media content, not live-install files |
| — `BUILDPE/EaseUS-x64/` (app copy in WIM) | 180 | 171,915,664 | 164.0 | second build of the app for WinPE |
| — `BUILDPE/x64/` (PE OS overlay) | 14 | 12,169,316 | 11.6 | 4 boot drivers + shell config + msi/oledlg shims + wallpaper |
| **Runtime artifacts in capture** (not install-time) | 11 | 69,174,116 | 66.0 | `SaveScan/*.rsf` ×3 (65.9 MiB), `log/*` ×4, 4 root QA logs |
| **Install-time files (tree minus artifacts)** | **1,459** | 633,660,079 | 604.2 | the honest "what the installer lays down" figure |

Prior claim (docs/REFERENCE-FOOTPRINT.md §2: 1,470 / 702,834,195 B / 565 PE / live 1,276 /
BUILDPE 194): **CONFIRMED** on every figure except one — the prior sheet lists
`BUILDPE/EaseUS-x64/` as "≈158 MB"; the measured value is 171,915,664 B ≈ 172 MB (164.0 MiB).
The 184,084,980 B BUILDPE total matches exactly.

### 1.2 Per top-level directory [OBSERVED]

| Dir | Files | MiB | Role |
|---|---|---|---|
| `(root)` | 149 | 133.57 | product exes, search DLLs, Qt, media/preview DLLs, cloud stack, runtimes, configs |
| `BUILDPE/` | 194 | 175.56 | WinPE emergency-disk payload (app copy + PE OS overlay) |
| `VideoViewer/` | 76 | 99.59 | video preview player + ffmpeg 7 + video repairer (44 UCRT shims) |
| `SaveScan/` | 3 | 65.91 | runtime scan sessions (QA-night artifacts, NOT installed) |
| `diskhealth/` | 818 | 52.81 | disk-health companion (Qt Quick/QML app + its own Qt runtime) |
| `EUFileRepair/` | 87 | 45.63 | standalone file-repair workbench (ImageMagick + 4 repair engines) |
| `ResourceDark/` | 2 | 30.83 | dark-theme Qt resource blob (`drw_dark.rcc` 32,290,147 B) |
| `Resource/` | 2 | 29.35 | light-theme resource blob (`drw_light.rcc` 30,742,205 B) |
| `PartitionRecovery/` | 57 | 26.85 | lost-partition MOM module set + boot tools + drivers |
| `language/` | 37 | 6.09 | 26 product .qm + 11 qtbase_*.qm translations |
| `platforms/` | 3 | 3.01 | Qt platform plugins (qwindows/qminimal/qoffscreen) |
| `imageformats/` | 3 | 0.47 | Qt image plugins (qgif/qjpeg/qsvg) |
| `EuShare/` | 3 | 0.21 | NAS redirector driver `EuMrx.sys` + install bat pair |
| `styles/` | 1 | 0.13 | qwindowsvistastyle.dll |
| `log/` | 4 | 0.11 | runtime logs (QA artifacts) |
| `License/` | 19 | 0.10 | third-party license texts (19 libs) |
| `iconengines/` | 1 | 0.04 | qsvgicon.dll |
| `config/` | 9 | 0.01 | product config files |
| `NasConfig/` | 2 | 0.01 | NAS model catalog + ssh config |

### 1.3 File classes (install-time, 1,459 files) [OBSERVED]

| Class | Files | MiB | Notes |
|---|---|---|---|
| PE binaries (exe/dll/sys) | 535 | 477.56 | 474 dll + 54 exe + 7 sys across the tree |
| Qt resource containers (.rcc) | 4 | 89.85 | drw_light/drw_dark rcc + DiskHealth.rcc + one more |
| Qt translations (.qm) | 114 | 17.41 | 37 main app + 40 diskhealth + PE copy + qtbase |
| MOM modules (.mo) | 30 | 6.44 | all in `PartitionRecovery/` |
| QML/JS (qml/qmlc/qmldir/qmltypes/js/jsc) | 612 | 4.51 | entire diskhealth QML runtime |
| Wallpaper/bmp | 1 | 5.93 | BUILDPE/x64/wallpaper.bmp |
| CSS | 2 | 0.83 | root `style.css` (437,565 B webview stylesheet) + 1 |
| XML/JSON/INI/TXT/DAT & other data | ~140 | ~1.6 | configs, manifests, license texts, .reg, .bat |
| api-ms-win-* UCRT forwarders | 200 | ~3.9 | 40 at root; private copies in VideoViewer/, EUFileRepair/, diskhealth/, BUILDPE/EaseUS-x64/ |

### 1.4 The product's current footprint and the quantified delta [OBSERVED — product docs + installer]

- Product **CURRENT (post-consolidation, 2026-11-05)**: **8 files in a flat `$INSTDIR`**
  (`recovery-wizard-ui.exe` 5,188,608; `uninstall.exe` 87,737; `cr_ui.dll` 1,192,912;
  `rrecov.exe` 1,230,288; `z.dll` 91,136; `lz4.dll` 126,464; `zstd.dll` 657,920;
  `HeicFastUninstall.exe` 61,973 — sum **8,637,038 B ≈ 8.2 MiB**) + `%ProgramData%\HeicFast\
  Recovery Wizard\{sessions,logs}` + registry keys; no service, no driver, no System32 writes.
  Sources: `docs/INSTALL-FOOTPRINT-CURRENT.md` §A.1 + Quick totals + status correction;
  `installer/recoverywizard.nsi:191-313`.
- Historical v0.6.0-rc1: 15 files + `System32\drivers\RDA.sys` + service (driver since removed,
  commit 318b9655).
- **Delta vs reference live tree: 1,276 vs 8 files (159.5×), 518,749,215 vs 8,637,038 B
  (60.1×).** Vs the whole capture (1,470/670 MiB): 1,462 files / ~662 MiB. The "400 files"
  story is really "~1,276 live files (≈1,459 install-time incl. the PE copy) vs 8" — and
  §5/§6 below split that delta into legitimately-out-of-scope vs functional gaps.
- Composition of the gap (live tree, by family): video stack 99.6 MiB (VideoViewer/), repair
  workbench 45.6 MiB (EUFileRepair/ + root repair exes ≈ 18.6 MiB), Qt runtime & resources
  ≈ 155 MiB (Qt5 DLLs + rcc pair + plugins), diskhealth companion 52.8 MiB, PartitionRecovery
  26.9 MiB, cloud/telemetry/update family ≈ 30 MiB, languages 6.1 MiB, VC/UCRT runtimes ≈ 12 MiB
  (incl. 200 shims), the rest configs/licenses.

---

## 2 Module-by-module map

Each table: file | size (B) | purpose | evidence tag | product's current state (from
`docs/INSTALL-FOOTPRINT-{CURRENT,DELTA}.md`, `docs/PARITY-DELTAS.md`, `docs/INTEGRATION-GAPS.md`).

### (a) Core scan engine

| File | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `DRW.exe` | 127,208 | **DRWLoader** supervisor/loader; spawns DRWUI.exe with inherited elevation, crash-restart loop; PDB `DRWIntelligentScan\DRWLoader` | [OBSERVED] strings: `DRWReport.exe`, `DRW_Crash_*`, `DRWLoader.pdb` | consolidated: ONE exe `recovery-wizard-ui.exe` (D-P1 — deliberate, no supervisor loop) |
| `DRWUI.exe` | 30,571,752 | main Qt UI orchestrator (scan chain, preview, results, repair UI) | [OBSERVED] 30.5 MB Qt binary, RTTI classes `CPreviewController` etc. | present as the Tauri UI + bridge; the scan/preview/results surfaces are the UI |
| `NTFSSearchDLL.dll` | 577,256 | NTFS engine (MFT walk + INDX + remnant) | [OBSERVED] file; DRWUI exports `EUCreateNTFSSearcher`/`QuickSearcher` | consolidated into `cr_ui.dll` (DELTA §B) |
| `FATSearchDLL.dll` | 469,224 | FAT engine | [OBSERVED] `EUCreateFATSearcher` | in `cr_ui.dll` |
| `exFATSearchDLL.dll` | 413,416 | exFAT engine | [OBSERVED] `EUCreateEXFATSearcher` | in `cr_ui.dll` |
| `EXT4SearchDLL.dll` | 129,256 | EXT2/3/4 engine | [OBSERVED] `EUCreateEXT4QuickSearcher`/`EXT2` | in `cr_ui.dll` |
| `ReFSSearchDLL.dll` | 154,856 | ReFS engine | [OBSERVED] `EUCreateReFSSearcher` | in `cr_ui.dll` |
| `HFSPlusSearchDLL.dll` | 185,064 | HFS+ (Mac) engine | [OBSERVED] `EUCreateHFSSearcher` | in `cr_ui.dll` |
| `BtrfsSearchDLL.dll` | 150,760 | Btrfs engine | [OBSERVED] `EUCreateBtrfsQuickSearcher` | in `cr_ui.dll` |
| `ZfsSearchDLL.dll` | 275,176 | ZFS engine | [OBSERVED] `EUCreateZFSSearcher` | in `cr_ui.dll` |
| `RAWSearchDLL.dll` | 644,840 | raw carving engine | [OBSERVED] `EUCreateRAWSearcher`; fed by `raw_signs.json` + `EUUserData` | in `cr_ui.dll` (`cr_signatures`/`cr_rawsigns`) |
| `raw_signs.json` | 35,342 | header/tail signature table for RAW carving (scored, min/max sizes) | [OBSERVED] read: e.g. `cpr` NUNDROOT header 0.54 / tail 0.69, min 10 KiB | mirrored in `cr_ui.dll` signature table |
| `FileFilter.xml` | 54,597 | file-category filter tree for results filtering (Graphic/… with per-type extension lists incl. 25 camera-RAW exts) | [OBSERVED] read | extension→category mirror generated into the bridge |
| `EUUserData` | 16,891 | user-extensible carving signature DB (111 custom sigs; JSON "signs"/"mime" per ext) | [OBSERVED] read; worklog resume-4 | no analog (not a claimed surface); note as minor gap for custom-sig carving |
| `DRWService.exe` | 40,168 | "DRW read FVE Server" — BitLocker key-broker service (`/install`, `/uninstall`, SCM) | [OBSERVED] strings `BitlockerReadFVEService.pdb`, CreateService | BitLocker in-process in `cr_ui.dll` (D-I5) |
| `PartitionRecovery/` (57 files, 28,158,618 B) | — | lost-partition stack: 30 `.mo` MOM modules (Device/DeviceManager/Partition/NTFS*/FAT*/ExFat/Ext/Refs analysers/LdmManager/…) + 19 DLLs (MainModule 1.83 MB, ToolsModule 2.58 MB, RecoveryModule 1.35 MB, PartitionRecovery.dll, Boot.dll, BitLockerLib, CloneModule, RawFixer, Win32Bcd, Burn.dll, AutoUpdate.dll, libvmdk, UILogic, Qt5Core.dll private copy) + drivers `eprdtdrv.sys`/`eprdtdrv_win7older.sys` + `setupdrvx64.exe` (live-Windows driver deployer) + `WinChkdsk.exe` (chkdsk+VSS) + data `lostpart.dat`/`partinfo.dat` | [OBSERVED] tree; worklog resume-3/5; arch doc 03 | lost-partition discovery/apply consolidated in `cr_ui.dll` (`lost`/`table`/`apply` modules); apply UI is CLI-shipped (`rrecov apply`, DELTA §C row 2) |
| `Dict` | 2,742 | password dictionary (common passwords: `12345678`, `password`, …) for encrypted-file password attacks | [OBSERVED] read; feeds EUArchiveRepair/EUOffice repair brute-force | no analog; belongs with the repair workbench gap (§6) |
| `dbghelp.dll` | 1,558,912 | crash-dump generation for the report/crash chain | [OBSERVED] file beside `Drwdump*.DMP` refs in DRWReport.ini | no analog (bridge logging seam NG-15) |
| `PubLog.dll` | 29,928 | shared logging shim (`log/DefaultLog.txt`, `DRWLOG_{GUID}` mutex) | [OBSERVED] strings | in-app `app.log` + engine log seam |

### (b) UI / Qt runtime

| File/Dir | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `Qt5Core/Gui/Widgets/Network/Svg/WinExtras.dll` (root) | 6,162,552 / 7,160,952 / 5,593,720 / 1,426,552 / 358,520 / 468,600 | Qt 5 widget runtime for DRWUI | [OBSERVED] files | no analog needed: product is Tauri/WebView2 (legitimate architecture difference, DELTA §B preamble) |
| `platforms/` (qwindows.dll 1,505,400; qminimal 872,568; qoffscreen 780,408) | 3.01 MiB | Qt platform plugins (offscreen for headless/PE) | [OBSERVED] files | N/A (WebView2) |
| `imageformats/` (qgif 37,496; qjpeg 420,984; qsvg 30,840) | 489,320 | **Qt in-process image preview plugins** — GIF/JPEG/SVG thumbnails in DRWUI | [OBSERVED] files | **absent — part of the preview gap (§3, §6)** |
| `iconengines/qsvgicon.dll` | 41,592 | SVG icon engine | [OBSERVED] | N/A |
| `styles/qwindowsvistastyle.dll` | 140,408 | Vista native style | [OBSERVED] | N/A |
| `Resource/` (`drw_light.rcc` 30,742,205 + `ImageResource.xml` 38,706) | 29.35 MiB | light-theme resource blob: the entire icon/illustration set (~7,400 assets per worklog) | [OBSERVED] files | product ships its own web assets inside the app |
| `ResourceDark/` (`drw_dark.rcc` 32,290,147 + `ImageResource.xml`) | 30.83 MiB | dark-theme resource blob | [OBSERVED] | same |
| `style.css` | 437,565 | web stylesheet (PUBLIC skin, `NORMAL_FONT_FAMILY`, 14px) — [INFERRED] styles an embedded HTML/Chromium surface (in-app buy pages via `UIInnerBuy.exe -url` launch string in DRWUI) | [INFERRED] | N/A — monetization skin (out of scope, §5) |
| `GDIPLUS.DLL` | 1,773,568 | side-by-side GDI+ (Win7-era image decode for preview/repair) | [OBSERVED] file at root + PE copy | part of preview gap context |
| `DRWReport.exe` | 4,805,352 | MFC report/diagnostics packer — collects logs + `Drwdump*.DMP` crash dumps into a report zip; registers a Preview Handler | [OBSERVED] DRWReport.ini file list; strings `Software\...\PreviewHandlers`, `Caller: the file had already been partially unzipped` | in-app report (`cr_ui_recovery_report_json`) — matched by consolidation |

### (c) Preview / viewer stack (detail in §3)

| File | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `heif.dll` | 585,728 | libheif (vcpkg x64-windows build) — HEIC/HEIF decode for image preview | [OBSERVED] strings `C:\heic\vcpkg\...libheif`, "Bit depth not supported by x265" | **absent — preview gap** |
| `de265.dll` | 444,928 | libde265 HEVC/H.265 decoder (libheif backend) | [OBSERVED] `libde265.pdb`, CTB/WPP strings | absent — preview gap |
| `libx265.dll` | 5,145,600 | x265 HEVC encoder (re-encode of repaired HEIC; also libheif plugin) | [OBSERVED] x265 CLI-param strings | absent — preview gap |
| `pdfium.dll` | 4,119,552 | PDFium — in-process PDF render for PDFViewer.exe | [OBSERVED] Adobe copyright + PDF filter strings | **absent — document preview gap** |
| `EUOfficeViewer.exe` | 3,088,104 | MFC office-document previewer (Word/Excel/PPT text extraction view) | [OBSERVED] MFC strings; DRWUI `OfficePreviewStub::OnOpenViewerThreadPro` | absent — document preview gap |
| `PDFViewer.exe` | 2,974,952 | MFC PDF preview window (over pdfium) | [OBSERVED] CMFC* strings | absent — document preview gap |
| `EUHwpViewer.exe` | 422,120 | Hangul Word Processor (.hwp) document previewer | [OBSERVED] `CHwpctrlctrl1` control | absent (hwp niche; decide scope) |
| `EUHwpFilePathCheckerModule.dll` | 155,880 | HWP file-path checker module (loader-shim pattern, `FilePathCheckerModuleExample.dll`) | [OBSERVED] strings; worklog resume-3 "HWP path-checker" | absent |
| `EURawImg.exe` | 736,488 | camera-RAW image previewer (statically links LibRaw — exports `LibRaw_*`) | [OBSERVED] LibRaw export names; DRWUI mutex `DRW_EURawImg_Mutex` (single instance) | absent — RAW preview gap |
| `EUImageEnhance.dll` | 3,056,872 | in-process image enhancement/upscale for preview (libtiff/zlib inside; `EuInitialize`/`EuShowWindow`/`EuUnInitialize` exported and called by DRWUI) | [OBSERVED] exports in DRWUI strings | absent — enhancement is polish, optional |
| `EUUnZip.exe` | 255,208 | ZIP extraction helper for archive preview (`unzip 0.15` inside) | [OBSERVED] strings | absent — archive preview gap |
| `7za.exe` | 596,936 | 7-Zip CLI (x86) — archive listing/extraction for preview + repair input | [OBSERVED] x86 CONSOLE PE (inventory.csv row 1) | absent — archive preview gap |
| `VideoViewer/` (76 files, 99.59 MiB) | — | out-of-process video/audio player + repair (see §3) | [OBSERVED] tree | **absent — video preview gap** |
| `EUImgRepair.exe` | 203,496 | image repair driver (JPEG lib) launched by DRWUI (`EUImgRepair_Wndname`) | [OBSERVED] DRWUI strings; JPEG strings | absent — repair gap (§6) |

### (d) Repair stack

| File | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `EUFileRepair/` (87 files, 45.63 MiB) | — | **standalone file-repair workbench**: 25 `CORE_RL_*` ImageMagick libs (MagickCore/Wand/cairo/freetype/harfbuzz/heif 3.63 MB/jpeg-xl 1.8 MB/webp/rsvg/tiff/png/lzma/brotli…), 7 `IM_MOD_RL_*` ImageMagick codecs (heic/jpeg/png/psd/svg/tiff/webp), `7z.dll`, `heif.dll` 781,400, `libde265.dll`, `libcurl.dll`, openssl-3 pair, plus FOUR engines: `EUImgRepairFile.exe` 5,226,728; `EuOfficeRepairFile.exe` 10,537,704; `EuPDFRepairFile.exe` 1,896,680; and copies of `EUAdobeRepair.exe`/`EUArchiveRepair.exe`; own UCRT (44 shims) | [OBSERVED] tree | **not shipped — the recorded gap G-1 (b)** (DELTA §C row 1: `cr_ui_repair_analyze/apply/undo` ABI trio is the follow-up; Repair page truthfully states unavailability) |
| `EUVideoRepair.dll` | 948,456 | in-process video repair engine for DRWUI (`GetIMovMp4Repair`, writes `reglog_win.txt`) | [OBSERVED] strings | same gap family |
| `EUAdobeRepair.exe` (root) | 434,408 | Adobe file repair (PSD/RAW family) | [OBSERVED] file + PE copy | same |
| `EUArchiveRepair.exe` (root) | 602,344 | ZIP/RAR/7z archive repair (7z engine; `CArchiveRarInfo::AnalysisData`, password path) | [OBSERVED] strings; worklog "EUArchiveRepair 7z engine" | same |
| `EuOfficeRepairWin32.exe` | 2,739,432 | Win32 office-document repair engine (embeds unzip 1.01) | [OBSERVED] strings | same |
| `EuOfficeTools.exe` | 1,653,480 | office repair support tools (BIFF/OLE structure parsing strings) | [OBSERVED] strings (`BIFF version`) | same |
| `EuRepairHost.exe` | 23,784 | repair host broker (`RepairHost.pdb`) — hosts repair engines out-of-process | [OBSERVED] strings | same |
| `Dict` | 2,742 | password dictionary for encrypted ZIP/RAR/Office repair attacks | [OBSERVED] | same |

### (e) Disk health — see §4 (the separate-surface rebuild input)

### (f) Boot / WinPE

| File/Dir | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `BUILDPE/EaseUS-x64/` | 180 files, 171,915,664 B | full second app build inside the WinPE WIM (DRW/DRWUI/SearchDLLs/Qt/preview/repair minus PartitionRecovery, EuShare, NasConfig, License, SaveScan, EUFileRepair, VideoViewer, diskhealth, setup trio, cloud stack) | [OBSERVED] full name-level diff this session (174 common names, 172 size-identical); arch doc 03 §6 | **out of scope** — live-Windows product (DELTA §D) |
| `BUILDPE/x64/` | 14 files, 12,169,316 B | PE OS overlay: boot drivers `EUDCPEPM.sys` 72,144 (disk-class upper filter, BOOT_START) + `EUEDKEPM.sys` 19,408; `ebrntdrv.sys`/`epmdkdrv.sys` (EPMNtDrv raw-disk pair); msi.dll/oledlg.dll PE shims (x2 bitness); `winpeshl.ini` (launches DRW.exe), `auto.bat` shell setup, `Unattend.xml`, `regdrv.reg`, wallpaper.bmp | [OBSERVED] files; arch doc 03 §2-3 | out of scope |
| `PE_Tools.dll` | 976,104 | "Create Emergency Disk" builder (51 RTTI classes; DISM mount + WAIK download + Burn/IMAPI2 ISO write) | [OBSERVED] libcurl strings; arch doc 03 §4 | out of scope |
| `PEtools.xml` | 12,756 | deployment manifest for WAIK PETools trees (bcdboot/wimmount/oscdimg/imagex/DIS…) | [OBSERVED] read | out of scope |
| `dlconfig.xml` | 16,202 | download manifest: byte-ranges + MD5s carving boot files (BOOTMGR, WINPE.WIM, BCD, ETFSBOOT.COM, BOOT.SDI, BOOTX64.EFI) out of Microsoft's WAIK supplement ISO | [OBSERVED] read (arch doc 03 §4 decoded) | out of scope |
| `EuShare/` (3 files, 218,602 B) | — | NAS network redirector: `EuMrx.sys` 215,704 + install/uninstall .bat | [OBSERVED] files; worklog EUMRX artifacts | out of scope (no NAS claim) |
| `PartitionRecovery` boot subset | — | `Boot.dll`, `BootDriver.dll`, `Win32Bcd.dll`, `Burn.dll`, `setupdrvx64.exe`, `eprdtdrv*.sys`, `AutoUpdate.dll`, `WinChkdsk.exe` | [OBSERVED] tree | live-side: engine health/apply consolidated; boot-media subset out of scope |

### (g) Cloud / telemetry / update / online services

| File | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `AliyunWrap.dll` + `AliyunWrapExe.exe` | 624,144 / 224,272 | Aliyun OSS log-upload channel (libcurl-based) | [OBSERVED] libcurl strings; AliyunConfig.ini keys | out of scope (telemetry, §5) |
| `AliyunConfig.ini` | 1,802 | Aliyun endpoints/keys (obfuscated), `track.easeus.com` allow-send URL | [OBSERVED] read | out of scope |
| `send_dmp_with_aliyun.exe` | 107,456 | crash-dump uploader to Aliyun OSS | [OBSERVED] strings | out of scope |
| `cloud_session.dll` | 943,336 | cloud session client (WinHTTP download tasks) | [OBSERVED] strings | out of scope |
| `firebasefetch.exe` / `firebaseonlineconfig.dll` | 90,344 / 1,155,816 | Firebase Remote Config fetcher (project `drweaseus-remoteconfig`) | [OBSERVED] google-services.json + strings | out of scope |
| `google-services.json` | 997 | Firebase project config (API key, app id `easeus.com.pm`) | [OBSERVED] read | out of scope |
| `remote_config_data` | 62,579 | ~600 remote-config flags cache (`__abroad`/`__free` segmentation) | [OBSERVED] strings; worklog resume-4 | out of scope |
| `EuICloudRecovery.dll` | 7,187,688 | iCloud recovery client (drive wsid; login/2FA viewer strings `ICloudVerifyViewer*`; `icloud_logger` beside it) | [OBSERVED] strings; icloud_logger log lines | out of scope (cloud recovery not claimed) |
| `icloud_logger` | 88 | iCloud session log stub | [OBSERVED] read | out of scope |
| `EuDownloader.exe` | 1,381,608 | resumable HTTP downloader (cookie jar, chunk upload) — used for WAIK/NAS-agent downloads | [OBSERVED] strings; arch doc 03 | out of scope (D-I8 family) |
| `SetupUE.exe` + `InfoForSetup.exe` | 177,384 / 117,776 | Inno-lineage installer pair (16-OS gate; install telemetry via AliyunInstall* markers) | [OBSERVED] strings; worklog resume-3 | product uses NSIS (D-I1/D-I10, deliberate) |
| `EUinApp.exe` | 37,608 | in-app buy/activation IE bootstrapper (`UIInnerBuy.exe -url` pattern) | [OBSERVED] strings; worklog resume-4 | monetization out of scope (§5) |
| `EuUserRate.dll` | 1,619,176 | user-rating/prompts network module (libcurl) | [OBSERVED] strings | out of scope |
| `unins000.exe/.dat/.msg` | 1,259,408 / 513,373 / 22,791 | Inno Setup uninstaller trio | [OBSERVED] files | NSIS `HeicFastUninstall.exe` (D-I1/D-I4/D-I7) |
| `Upgrade.ini` / `DataFile.ini` | 235 / 84 | updater version marker (20.8.0.0, killDRWUI, download URL) / update-staging state | [OBSERVED] read | no auto-updater (D-I8: releases via GitHub) |
| `cpprest141_2_10.dll`, `libzmq-mt-4_3_5.dll` (root + VideoViewer copy), `libssh2.dll`, `libcrypto/libssl` ×3 generations (3-x64 4.68 MB; 1_1-x64 2.89 MB; libeay32/ssleay32 1.0.x-era ×2) | ~17 MiB | C++ REST SDK (cloud APIs), ZeroMQ (IPC to VideoViewer), SSH (NAS), TLS runtime zoo | [OBSERVED] files | none needed (engine's own TLS/transport choices) |

### (h) Config / resources / languages

| File/Dir | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `config/version.ini` | 389 | build identity (21.0.0.0 free abroad) + OEM fields | [OBSERVED] read | product version registry keys |
| `config/shellexe.ini` | 2,098 | shell context-menu registration strings ("Recover any deleted data with EaseUS") in 25 languages | [OBSERVED] read (UTF-16) | no shell integration claimed — verify with Agent E |
| `config/FilterFolder.ini` | 1,155 | junk-folder/fileName filter + `TxtViewFiles` previewable-text-extension list + `SumaryFileType` grouping | [OBSERVED] read | filtering is bridge-side (D-B3); the TxtView list is a preview-scope input |
| `config/RawConfig.ini` | 257 | per-extension RAW-carve toggles (HEIC=1, WEBP=1, CRW family…) | [OBSERVED] read | engine signature table |
| `config/PriorityDisplayType.xml` | 1,630 | results priority-display config (5-file filtering pipeline per worklog) | [OBSERVED] file | bridge-side ordering |
| `config/usefullostfiles.xml` / `themdark.ini` / `config.ini` / `nasconfig.ini` | 623/5,203/1,785/274 | misc UI/feature config | [OBSERVED] files | in-app equivalents or N/A |
| `NasConfig/NasModel.xml` + `ssh_config.ini` | 6,138/92 | NAS vendor/model catalog (Synology FS6400…, QNAP, Buffalo…) + SSH push config (`/tmp/EaseUS/`) | [OBSERVED] read | out of scope (NAS) |
| `language/` | 37 files, 6.09 MiB | 26 product .qm (Arabic…Turkish incl. Mainland OEM variants Mainland{,_Foxit,_JZ,_LC}) + 11 qtbase_*.qm | [OBSERVED] list | product ships EN + zh-CN (DELTA §D) |
| `Japanese.dll` | 4,206,312 | ja-JP MFC resource satellite (VC90-era; lang 1041) | [OBSERVED] strings; worklog resume-5 | N/A (Qt .qm does this in the og; ours is web-tech i18n) |
| `License/` | 19 files, 0.10 MiB | third-party license texts (Qt, pdfium 62 KB, libzmq 16 KB, openssl, ImageMagick-adjacent libraw/opencv…) | [OBSERVED] list | product must carry equivalent notices when it bundles preview/repair libs (§6 note) |

### (i) Legacy / runtime support

| File | Size | Purpose | Evidence | Product state |
|---|---|---|---|---|
| `msvcr90/msvcp90/msvcm90.dll` + VC90 manifests | 626,688/856,576/224,768 | VC9 CRT for the VC90-era tools (EUUnZip, send_dmp_with_aliyun, EuUserRate, Japanese.dll) | [OBSERVED] files + manifests | N/A — build with one modern toolchain |
| `msvcr120/msvcp120.dll` | 963,240/659,616 | VC12 CRT (euAuxiliary-era components) | [OBSERVED] | N/A |
| `msvcp140/vcruntime140/concrt140/ucrtbase.dll` + 200 `api-ms-win-*` forwarders | ~9.5 MiB | VC14/UCRT — 40 shims at root, private copies in VideoViewer/, EUFileRepair/, diskhealth/, BUILDPE | [OBSERVED] count 200 | N/A (product statically links / uses OS UCRT) |
| `legacy.dll` | 101,888 | embedded libtomcrypt (compat crypto for legacy formats) | [OBSERVED] strings; worklog resume-5 | engine-native crypto |
| `zlib.dll`/`zlib1.dll`/`archive.dll`/`liblzma.dll` | 83,968/90,624/688,128/186,880 | compression runtime (archive.dll = libarchive) | [OBSERVED] strings (libarchive/`liblzma/`) | `z.dll`/`lz4.dll`/`zstd.dll` beside `cr_ui.dll` (matched class) |
| `msi.dll`/`oledlg.dll` (BUILDPE/x64 ×2 bitness), `d3d8thk/d3d9/d3d11/dxgi.dll`, `mfc90u.dll` (PE copy) | ~9 MiB | PE-compat shims (MSI, D3D for Qt video on WinPE) | [OBSERVED] tree; arch doc 03 §6 | out of scope (boot media) |

---

## 3 The preview stack (deep)

### 3.1 Inventory and invocation model [OBSERVED unless noted]

The reference previews four media classes, with a **hybrid in-process / out-of-process model**
driven from DRWUI.exe:

1. **Images — in-process (Qt) + helper exes.**
   - In-process: Qt `imageformats/` plugins (qgif, qjpeg, qsvg) render standard thumbnails;
     `heif.dll` (libheif) + `de265.dll` (libde265) decode HEIC/HEIF stills inside DRWUI;
     `libx265.dll` provides the HEVC encode path (repaired-HEIF re-write, and libheif's
     encoder plugin) — [INFERRED for the encode path] from the libheif x265 plugin string.
   - Out-of-process: `EURawImg.exe` (single-instance, `DRW_EURawImg_Mutex` observed in DRWUI
     strings) opens camera RAW (LibRaw statically linked — `LibRaw_*` exports observed);
     `EUImageEnhance.dll` is loaded **in-process** by DRWUI via its `EuInitialize`/
     `EuShowWindow`/`EuUnInitialize` exports for enhance/upscale of damaged previews.
   - Archive of what counts as an image: FileFilter.xml "Graphic" category (25 RAW extensions)
     and RawConfig.ini toggles.
2. **Video/audio — fully out-of-process (`VideoViewer/`, 76 files, 99.59 MiB).**
   - `EaseUS_VideoViewer.exe` (121,064 B) is the player window (GDI+ UI — GdipCreateBitmapFromFile
     etc. observed); `EaseUS_MediaGraph.dll` (99,560 B) is the decode/render graph
     (`MediaGraph::VideoRenderThread`, calling `avcodec-62.dll` send/receive_frame API).
   - The **ffmpeg 7 runtime** is bundled as DLLs: `avcodec-62.dll` 58,423,310; `avfilter-11.dll`
     21,193,742; `avformat-62.dll` 13,447,182; `avutil-60.dll` 2,808,334; `swscale-9.dll`
     1,680,910; `swresample-6.dll` 721,422; `avdevice-62.dll` 165,902; `postproc-58.dll`
     87,054; plus `ffmpeg.exe` CLI 532,494 and `libzmq-mt-4_3_5.dll` 369,152 — [INFERRED] the
     viewer talks to a libzmq socket (DRWUI's root copy of libzmq is the other end) so preview
     playback runs in the child process while the UI stays responsive.
   - `VideoFileRepairer.exe` (1,919,208 B) is the video repair engine (`.mp4/.mov/.mxf`,
     `AacSampleParser`, `AmrSampleParser`, `AviSampleChecker`, `BrawSampleParser`,
     `H264SampleChecker`, moov/bmdf atoms observed) — the repair side of the same folder.
   - `sfc` (211,614 B, binary data, header ` cfs-…`) — [UNKNOWN] purpose; candidate: a packed
     font/cipher table used by the player skin.
3. **Documents — separate MFC exes.**
   - `EUOfficeViewer.exe` (3.09 MB) — Word/Excel/PowerPoint preview (MFC; DRWUI's
     `OfficePreviewStub::OnOpenViewerThreadPro` launches it on a viewer thread).
   - `PDFViewer.exe` (2.97 MB) — MFC window rendering through `pdfium.dll` (4.12 MB at root).
   - `EUHwpViewer.exe` (422 KB) — Hangul .hwp preview via the `CHwpctrlctrl1` control, with
     `EUHwpFilePathCheckerModule.dll` as its loader-side path checker.
   - DRWUI also carries PDF repair-preview classes (`CPDFRepairPreview`, `CRepairPDFController`,
     `CPDFRepairGetPixmapThread`) — repair previews render through the same pdfium stack.
4. **Archives — in-process model with CLI helpers.**
   - DRWUI contains `PreviewForCompressFile` (`ReaderAndCache`), `ArchivePreview{Model,View,
     Window,Stub,Delegate}` classes — archive preview (listing + selective preview of members)
     is in-process, backed by `EUUnZip.exe` (ZIP; embedded unzip 0.15) and `7za.exe`
     (7z/RAR/other) as extraction helpers, with password handling (`%1 needs to enter password
     below to proceed with repair` string observed).

### 3.2 What parity requires the product to bundle [INFERRED — mandate mapping]

To match the reference's preview support the product needs, per class:

| Class | Reference bundles | Minimal parity bundle for the product |
|---|---|---|
| Images (GIF/JPEG/SVG) | imageformats/ qgif+qjpeg+qsvg | a native image decode path (WebView2 covers JPEG/PNG/GIF/SVG natively; parity is about depth, not file count) |
| HEIC/HEIF stills | heif.dll + de265.dll (+ libx265.dll for re-encode) | libheif + libde265 equivalent (e.g., compiled into the engine or a side DLL) |
| Camera RAW | EURawImg.exe (LibRaw) | LibRaw (libraw) decode path |
| Video/audio | VideoViewer/ ffmpeg-7 DLL set (99.6 MiB total, ~99 MiB of it ffmpeg) | an ffmpeg-class demux/decode set (shared by preview and the video-repair follow-up); exact codec breadth is a scope call |
| PDF | pdfium.dll + PDFViewer.exe | pdfium (or equivalent) render path |
| Office docs | EUOfficeViewer.exe | office text/structure preview — scope call: full fidelity needs an OOXML parser; text-grade preview is much smaller |
| HWP | EUHwpViewer.exe + checker DLL | niche; recommend honest "not previewed" unless HWP is a claimed locale surface |
| Archives | EUUnZip.exe + 7za.exe + PreviewForCompressFile | 7-Zip/zip extraction for archive preview (7z.dll/7za or libarchive — the engine already ships compression libs) |
| Text files | TxtViewFiles extension list in FilterFolder.ini | trivial (the bridge can render text) |

Note: the reference bundles its media tooling **twice on the medium** (root + BUILDPE copy) and
**three times** for some repair codecs (root exes + EUFileRepair/ copies + ImageMagick codec
modules). The product should match FUNCTIONALITY (one shared decode set serving preview and
repair), not the reference's copy count — per the mandate "COMPLETE, not padded".

---

## 4 The diskhealth companion footprint

[OBSERVED — full subtree walk]

| Metric | Value |
|---|---|
| Files | **818** |
| Total size | **55,371,700 B = 52.81 MiB** |
| Root files (64) | 33.80 MiB |
| QML/JS runtime files | ~612 (QtQuick/Controls 1+2, Controls.2 Fusion/Imagine/Material/Universal, GraphicalEffects, Layouts, Shapes, Window.2, qmltooling, scenegraph, bearer) |
| Own Qt runtime | Qt5Core 5,381,616; Qt5Gui 5,894,128; Qt5Widgets 4,459,504; Qt5Quick 3,489,776; Qt5Qml 2,975,216; Qt5Network 1,051,120; Qt5QuickTemplates2 942,576; Qt5RemoteObjects 376,816; + QmlModels/QuickControls2/QuickShapes/Svg/QmlWorkerScript |
| App binaries | `DiskHealth.exe` 1,781,480 (QML app — `diskInfoMgr.*` QML observed, ATA SMART `CAtaSmart`/`ATA_SMART_INFO` classes, tray integration, schtasks via addtask/deltask/querytask.bat) + `DiskHealth.rcc` 444,492 |
| Non-Qt runtime | mfc140u.dll 5,052,184; msvcp140/vcruntime140/ucrtbase + 32 api-ms-win shims |
| Plugins | own imageformats/ (9 files, 1.33 MiB), platforms/ (qwindows 1,219,568), iconengines, styles, translations/ (40 .qm, 5.48 MiB) |
| Config | `AliyunConfig.ini` stub (28 B) |

Character: a **CrystalDiskInfo-lineage** SMART monitor (worklog resume-3/4) shipped as a fully
self-contained Qt Quick app — 94% of its files exist because it carries its own complete Qt/QML
runtime and translations. The product's current state: an **in-app /health page** (SMART table
per drive) consolidated into the main app — recorded as D-P4 (one product, one install). The
functional content to reproduce on the separate-surface rebuild: per-drive SMART attribute
table, health/temperature badges, tray/task scheduling (the .bat trio drives schtasks), and the
40-language localization story at least for the languages the product claims.

---

## 5 Legitimately out of scope (with divergence-register citations)

Per `docs/PARITY-DELTAS.md` and `docs/INSTALL-FOOTPRINT-DELTA.md` §D-E, these reference
families are deliberately NOT product scope — each is a monetization/telemetry/cloud/packaging
surface, not a data-recovery capability:

| Reference family | Files/MiB | Citation |
|---|---|---|
| Auto-update chain (`Upgrade.ini`, `DataFile.ini`, AutoUpdate.dll in PartitionRecovery, downloader use for updates) | — | **D-I8** ("no auto-updater; releases via GitHub, manual download — deliberate v1 scope cut") |
| Free-tier RetentionActiveDlg periodic nag | — | **D-P3** ("the nag is the reference's MONETIZATION surface… deliberately out of scope") |
| Installer tech (Inno unins000 trio, SetupUE 16-OS gate + install telemetry) | 1.97 MiB | **D-I1, D-I4, D-I7, D-I10** (NSIS chosen; no telemetry) |
| Aliyun/Firebase telemetry + crash-dump upload (`AliyunWrap*`, `send_dmp_with_aliyun.exe`, `firebase*`, `google-services.json`, `remote_config_data`, `EuUserRate.dll`) | ~2.8 MiB | DELTA §E ("reference-only artifacts… No analog intended"); D-I10 "no telemetry" |
| Activation/buy surfaces (`EUinApp.exe`, `style.css` in-app store skin, `Japanese.dll`-style satellite packaging) | ~4.6 MiB | DELTA §E; D-P3 family |
| Cloud recovery (`EuICloudRecovery.dll`, `cloud_session.dll`, cpprest) | ~10.9 MiB | DELTA §D ("no cloud surface exists in the UI") |
| NAS stack (`EuShare/EuMrx.sys`, `NasConfig/`, `libssh2.dll`, nasconfig.ini) | ~0.3 MiB + ssh | DELTA §D |
| WinPE boot media (`BUILDPE/` 194 files, `PE_Tools.dll`, `PEtools.xml`, `dlconfig.xml`, boot drivers) | 176 MiB | DELTA §D ("the product is a live-Windows product"); arch doc 03 |
| 35 non-shipped language packs (product keeps EN + zh-CN) | ~5.9 MiB | DELTA §D |
| QA/runtime residue (`SaveScan/*.rsf`, `log/*`, `EPMLOG.log`, `Eaolog.log`, `strutureReg.log`, `reglog_win.txt`) | 66.0 MiB | DELTA §D/E; docs/REFERENCE-FOOTPRINT.md §0 capture caveats |
| Driver-era divergences (reference's `eprdtdrv` SCM service; DRWService key-broker) | — | **D-I5, D-I6, D-FALL (reversed)** — the product's posture is user-mode direct raw-handle access under an admin token; no driver, no service |
| Companion-app packaging of DiskHealth | 818 files | **D-P4** (in-app /health page; the split is "the reference's packaging decision, not a capability") |

Also legitimately different by consolidation (DELTA §B, verified against the tree this session):
the 9 SearchDLLs + RAW carving + PartitionRecovery MOM set + logging + report all live inside
`cr_ui.dll`; sessions are `.crsf` (D-I9) instead of `.rsf` in `SaveScan/`; data goes to
%ProgramData% not the install dir (D-I2).

---

## 6 The functional-gap list (what the product must ADD to be complete)

Ordered by user-visible capability, per the mandate (functionality complete, not file-count
padded). Each row: what the reference ships → what the product must gain.

1. **Preview rendering for images** — reference: imageformats plugins + heif/de265 HEIC path +
   EURawImg (LibRaw) + EUImageEnhance. Product today: previews ship as real bytes + metadata
   with truthful "preview not rendered" hints (DELTA §D/G-4). Gap: a decode path at least for
   JPEG/PNG/GIF/WebP + HEIC/HEIF; camera RAW (LibRaw-class) if RAW recovery is a claimed
   surface (FileFilter.xml's Graphic category and RawConfig.ini say the reference treats 25 RAW
   extensions as first-class).
2. **Video/audio preview** — reference: VideoViewer/ out-of-process ffmpeg-7 player (~99.6 MiB,
   of which ~99 MiB is the ffmpeg DLL set). Gap: an ffmpeg-class demux/decode bundle shared
   with the video-repair engine (one bundle, two consumers — avoids the reference's duplicate
   copies).
3. **Document preview** — reference: EUOfficeViewer (Office), PDFViewer + pdfium.dll (PDF),
   EUHwpViewer (+path checker) for HWP. Gap: PDF render (pdfium-class) is the clear must;
   Office preview is a scope call (text-grade vs full fidelity); HWP recommended as an honest
   not-previewed unless the Korean-market surface is claimed.
4. **Archive preview** — reference: PreviewForCompressFile in-process + EUUnZip.exe + 7za.exe
   helpers. Gap: archive listing/member preview over a 7z/zip extraction path.
5. **The repair workbench** — reference: EUFileRepair/ (45.6 MiB: 4 engines — image, office,
   PDF, plus archive/Adobe repair at root; ImageMagick codec zoo; `Dict` password dictionary;
   EuRepairHost broker; EUVideoRepair.dll). Product today: **not shipped, G-1 (b)** — the
   `cr_ui_repair_analyze/apply/undo` ABI trio is the recorded follow-up and the Repair page
   truthfully states unavailability. This is the single largest genuinely-missing claimed
   capability (pre-scan repair consent flow IS shipped — D-B5).
6. **Partition-apply UI** — reference: PartitionRecoveryD's write path surfaced in its UI.
   Product: engine + `rrecov apply` CLI complete; the in-UI flow is the recorded follow-up
   (DELTA §C row 2, S-029).
7. **Disk-health separate surface** — reference: 818-file companion (content: SMART table,
   health/temperature, tray, scheduling, 40 languages). Product: in-app page exists; the
   separate-surface rebuild should reproduce the CONTENT (see §4), not the 818-file packaging
   (D-P4 records the deliberate in-app consolidation).
8. **License notices** — when the product bundles preview/repair media libraries (libheif,
   libde265, x265, pdfium, 7-Zip, LibRaw, ImageMagick-family), it must ship their license
   texts as the reference does (`License/`, 19 files) — [INFERRED] a packaging obligation that
   follows from closing gaps 1-5.

Explicitly NOT gaps (verified consolidations, DELTA §B): FS engines, carving/signatures,
filtering, sessions, report, uninstaller, BitLocker handling, SMART health data path.

---

## 7 Open questions for Agent E

1. **`VideoViewer/sfc`** (211,614 B, binary, magic `20 63 66 73 2d` " cfs-"): what is it? Not a
   PE, not an archive; candidate player-skin/cipher blob. [UNKNOWN]
2. **`EuSendMessage.exe`** (20,200 B, PE-only in the BUILDPE copy, absent live): arch doc 03
   says "role TBD" — confirm whether it is the PE-side telemetry sender and therefore pure
   out-of-scope. [UNKNOWN]
3. **Video preview IPC**: confirm the libzmq channel (root `libzmq-mt-4_3_5.dll` + VideoViewer
   copy) is DRWUI↔EaseUS_VideoViewer control transport, and whether the product's preview
   design should reuse an equivalent child-process split or render in-app. [INFERRED both ways]
4. **HWP surface**: is Korean-market HWP preview a claimed surface for the product? Decides
   gap 3's HWP row. [UNKNOWN — product docs silent]
5. **RAW image depth**: FileFilter.xml lists 25 RAW extensions and RawConfig.ini gates ~40 more
   carve types — should the product claim camera-RAW preview (LibRaw bundle) or keep RAW as
   recover-but-not-preview? [scope decision needed]
6. **Office preview fidelity**: text-grade vs layout-grade — the reference's EUOfficeViewer
   appears text/extraction-grade (MFC list views); confirm before scoping. [INFERRED]
7. **`config/shellexe.ini`**: the reference registers an Explorer context-menu entry in 25
   languages; the product has no shell integration claimed anywhere — confirm this is a
   deliberate non-claim or a missed surface. [UNKNOWN]
8. **License compliance bundle**: which license texts must ship once gaps 1-5 close (the
   reference's 19-file License/ set is the checklist). [follow-up to §6.8]
9. **Pe-copy accounting**: for parity bookkeeping, should BUILDPE's 194 files count toward the
   reference "installed footprint" at all (they live in the WIM on the emergency media, not on
   the live machine)? This sheet counts the live tree (1,276) as the honest comparator, with
   BUILDPE documented separately — confirm the convention. [convention call]

---

### Source index (primary evidence)

- Full-tree walk + per-dir/class census: this session, `scripts/ref_footprint_census.py` output
  (1470 files / 702,834,195 B; per-dir and per-class tables in §1).
- `re-workspace/inventory.{csv,json}`: 1,470 rows, all sizes match the tree exactly; 565 PE
  binaries; class split 1,244 easeus / 226 external.
- Config reads: `needcopy.txt` (copy-list: InfoForSetup + Aliyun pair + full UCRT/VCRT set —
  the WinPE-builder's "files that must accompany the app"), `auto.txt` (binary GUID/token blob
  — [UNKNOWN] exact consumer), `DataFile.ini`, `Upgrade.ini`, `recoveryFree.ini` (binary),
  `DRWReport.ini`, `AliyunConfig.ini`, `google-services.json`, `FileFilter.xml`,
  `raw_signs.json`, `Dict`, `EUUserData`, `PEtools.xml`, `config/*`, `NasConfig/*`.
- Strings: DRW.exe, DRWUI.exe, DRWService.exe, EUinApp.exe, InfoForSetup.exe, SetupUE.exe,
  EuDownloader.exe, EuRepairHost.exe, heif/de265/libx265/pdfium, EUImageEnhance/EUVideoRepair,
  EuICloudRecovery/cloud_session/firebase*, AliyunWrap/PE_Tools/EuUserRate, legacy/archive/
  PubLog/Japanese.dll, EURawImg/EUOfficeViewer/PDFViewer/EUHwpViewer/EUUnZip/EUImgRepair/
  EUArchiveRepair/EuOfficeTools/EUHwpFilePathCheckerModule, VideoViewer/*, diskhealth/
  DiskHealth.exe, BUILDPE/x64/{auto.bat,winpeshl.ini,Unattend.xml,regdrv.reg}.
- Corpus distillations: `easeus-drw-research/architecture/03-winpe-boot-environment.md` (all
  76 lines), `worklog.md` (resume-3/4/5 entries: build identity, EUUserData 111 sigs,
  remote_config_data ~600 flags, Japanese.dll, WinChkdsk, setupdrvx64, legacy.dll, DiskHealth
  CrystalDiskInfo lineage, EUArchiveRepair 7z engine, EUinApp IE-bootstrapper).
- Product side: `final-prod/docs/INSTALL-FOOTPRINT-CURRENT.md` (§A.1, Quick totals, status
  correction), `docs/INSTALL-FOOTPRINT-DELTA.md` (§A-E + numeric reconciliation),
  `docs/PARITY-DELTAS.md` (D-I1…I11, D-P1…P7, D-B1…B5, D-FALL, D-DSIGN), prior
  `docs/REFERENCE-FOOTPRINT.md` / `REFERENCE-MODULES.md` and
  `worklog/context-sheets/REFERENCE-{LAYOUT,INSTALLER}.md` (claims cross-checked; see §1.1
  confirmation note), `installer/recoverywizard.nsi`.
