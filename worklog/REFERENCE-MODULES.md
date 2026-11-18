# REFERENCE-MODULES — Installed OG Footprint: Every Module, Signature File, Data File, and Runtime Artifact of the Result Pipeline

> Agent D, Phase 0 parallel read. Corpus: `/home/z/my-project/repos/easeus-drw-research`.
> Scope: the installed footprint of the ORIGINAL product (og-codebase = the shipped
> install tree, 167 top-level entries) that is RELEVANT TO THE RESULT PIPELINE —
> detection modules, signature/config data files, session files, caches, logs,
> services/drivers, and the files created at runtime during a scan session.
> Every claim cites a corpus file. "not covered" = corpus silence. No guessing.

---

## 1. The install tree (Program Files layout)

- **Live-Windows tree = `og-codebase/` itself** — the shipped install directory
  (Inno Setup wizard copies it; uninstaller trio `unins000.exe/.dat/.msg` is Inno's
  signature; bootstrap = `SetupUE.exe`). Cite:
  `re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1/§3.
- **WinPE target path**: `%systemdrive%\Program Files\easeus\DRW\DRW.exe` — the PE
  edition deploys the same payload (staged at `BUILDPE/EaseUS-x64/`) to
  `Program Files\easeus\DRW\` (lowercase easeus). Cite:
  `re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` §6-1.
- The live-tree Program Files path itself (e.g. `C:\Program Files\EaseUS\...`) is
  **not covered** by any artifact; only the PE target path is documented.
- Registry install identity: `SOFTWARE\EASEUS\DRW\Version` (written/read by
  SetupUE bootstrap; `HKLM\SOFTWARE\EASEUS\DRW\Language` read by DRW.exe loader;
  `SOFTWARE\EASEUS\EaseUS Data Recovery Wizard` product-state key; a
  `SOFTWARE\Foxit Software\FoxitRestore\Foxit Information` codename key exists).
  Cites: `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1;
  `artifacts/DRW-exe--launcher.md` §1-5; `artifacts/UIUX/04-userflow-state-machine.md`
  :121; `artifacts/EuShare/eumrx-nas-architecture.md` §3.3.

### 1.1 The process topology (who loads whom)

```
DRW.exe (DRWLoader supervisor)
  └─> DRWUI.exe .................. main app (the product)
        ├─> DRWService.exe ........ BitLocker FVE key service (pipe \\.\Pipe\DRWBitlocker)
        ├─> *SearchDLL.dll ........ in-proc FS scan engines (9 DLLs, factory contract)
        ├─> MainModule.dll + *.mo .. in-proc MOM module system (31 .mo partition layer)
        ├─> EuRepairHost.exe ...... out-of-proc repair licensing bridge
        │     └─> Fixo repair EXE chain (EUFileRepair/..., EUVideoRepair.dll, ...)
        ├─> EuShare + EuMrx.sys ... NAS redirector path (live only)
        ├─> cloud_session.dll / EuICloudRecovery.dll / firebase* ... cloud+flags
        ├─> DRWReport.exe ........ crash reporter (launched by DRW.exe on crash)
        └─> kernel: EPMNtDrv family (eprdtdrv.sys et al.) via \\.\EPRDTDRV\<pid>\<disk>\<part>
```
Cite: `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md` §2 (diagram verbatim).

---

## 2. Module → role map (og-codebase/ top level, result-pipeline relevant)

### 2.1 Core process binaries

| File | Role | Cite |
|---|---|---|
| `DRW.exe` | DRWLoader supervisor: creates `<exe_dir>\log\`, boots `log\DRWLog.log` + 2 MB shared-log file mapping `DRW_{1E956C11-...}` + `DRWLOG_{...}` mutex, spawns DRWUI.exe, watches `DRW_Crash_{FC1A0E29-...}` / `DRW_Memory_Crash_{c3d61a40-...}` events, flushes the shared log every 5 s, restarts DRWUI after crash (max 1 restart), on crash spawns DRWReport.exe with `{"Elapsed","Error","Result"}` JSON | `artifacts/DRW-exe--launcher.md` §1-2 |
| `DRWUI.exe` (30.2 MB, Qt5) | THE product: wizard flow home→scan→results→recovery; loads all 9 SearchDLL engines (FUN_14056c0d0 LoadLibraryW table), MOM `.mo` system via MainModule.dll, shells out to Fixo repair chain, persists `.rsf` sessions, free/paid gating | `artifacts/DRWUI-exe--main-ui-architecture.md` §1, §9.2 |
| `DRWService.exe` (~40 KB) | LocalSystem service "Read FVE Server", SERVICE_AUTO_START: reads BitLocker FVE auto-unlock external keys from `HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock\<GUID>\Data`, pushes them to DRWUI over named pipe `\\.\Pipe\DRWBitlocker` (service is pipe CLIENT; one-shot ≤100-record message, 0x138-byte stride) | `artifacts/DRWService-exe--bitlocker-service.md` §1-3 |
| `DRWReport.exe` | Crash reporter — customized XCrashReport (CodeProject); PDB `XCrashReport.pdb`; writes rotating dump pair `DRWDump0.DMP`/`DRWDump1.DMP` (up to Drwdump5 per DRWReport.ini) in app dir; upload via `send_dmp_with_aliyun.exe` → Aliyun OSS | `artifacts/DRWReport-exe--crash-reporter.md`; `og-codebase/DRWReport.ini` |
| `SetupUE.exe` + `InfoForSetup.exe` | Installer bootstrap pair: 16-OS version gate, reads/writes `SOFTWARE\EASEUS\DRW\Version`, `/SendInfo` install telemetry (Activity/Country/Timezone/TestId); absent from WinPE build | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` |
| `unins000.exe/.dat/.msg` | Inno Setup uninstaller trio | same artifact §1 |
| `EuRepairHost.exe` (50 fns) | Thin licensing/activation bridge for repair features (activation.easeus.com / AWS Global Accelerator); repair is a paid-feature gate | `artifacts/FileRepair/repair-architecture.md` §1, §7 |

### 2.2 The per-filesystem Search DLLs (the detection engines — result pipeline core)

All in `og-codebase/`, loaded in-proc by DRWUI. Contract: each exports
`EUCreate<FS>Searcher` (full/deep controller, `ICompleteFileSystemSearchController`)
+ `EUCreate<FS>QuickSearcher` (quick scan, `IQuickFileSystemSearchController`).
Cite: `artifacts/SearchDLL/searchdll-family--architecture.md` §1.

| DLL | FS parsed | Functions / lines | Notes |
|---|---|---|---|
| `NTFSSearchDLL.dll` | $MFT (1KB records), $BITMAP, $UsnJrnl, MFTMirr, attrs 0x30/0x80/0xB0 | 1,931 / 62,394 | flagship scanner; CNTFSSearchController 0x940 B; quick=deleted-record fast pass, complete=bitmap build+USN cross-check+orphan sweep |
| `FATSearchDLL.dll` | FAT12/16/32, dir entries, LFN, 2 FAT tables | 1,265 / 64,658 | |
| `exFATSearchDLL.dll` | exFAT bitmap, dentries, UP-case table | 1,299 / 58,043 | |
| `EXT4SearchDLL.dll` | superblock, group descriptors, inode tables, extents, htree dirs | 475 / 18,071 | 0xEF53 magic; journal gates ONLY on jsb magic 0xC03B3998 + blocktype {3,4} (CORRECTIONS #6) |
| `HFSPlusSearchDLL.dll` | B*-tree catalog, extents overflow, allocation file | 665 / 22,304 | H+/HX signatures |
| `ReFSSearchDLL.dll` | ReFS metadata tables, identity B+ trees | 629 / 20,270 | |
| `BtrfsSearchDLL.dll` | superblock @ 0x10000, chunk tree, b-trees, checksums | 452 / 18,626 | exports `EUCreateBtrfsComplateSearcher` (sic); NO chunk-tree-uuid validation (CORRECTIONS #8) |
| `ZfsSearchDLL.dll` | uberblock, MOS, DMU objects, ZAP | 981 / 45,364 | |
| `RAWSearchDLL.dll` | no FS — signature carving over unallocated space | 1,921 / 80,614 | see §3 — 190+ analysers, 70 magic keys; 6 exports adds `EUCreateRAWSearcherForCellTest`, `EUGetAnalyserList/Map/Multimap`, `GetRawUserInfoMap` |

- DRWUI also holds a literal `Ext2SearchDLL.dll` LoadLibrary reference — **not shipped**
  (legacy). FS-code→factory map (quick): 0,1,2→FAT; 3→NTFS; 4,5→EXT2; 7→exFAT; 8→HFS;
  9→EXT4; 10→ReFS; 13→ZFS; 3rd dispatcher→Btrfs. Cite: family artifact §1 + DRWUI
  architecture §9.2 (`FUN_140577d40`, `FUN_140580720`).
- Advanced scan = all FS engines in full mode + RAW searcher (gated on a settings
  field at +0x60, `FUN_1400b1f30`), RAW registered LAST, with a
  `std::function<string(string,string)>` carved-file naming callback via vtable+0x48
  (naming policy lives in the UI, not the engine). Cite: DRWUI architecture §9.2/§9.4.

### 2.3 PartitionRecovery/ — MOM module system + drivers (the partition substrate)

Directory `og-codebase/PartitionRecovery/`: 31 `.mo` modules + 6 host DLLs + drivers +
`setupdrv.ini`/`lostpart.dat`/`partinfo.dat`.

| File | Role | Cite |
|---|---|---|
| `PartitionRecovery.dll` | engine host (imports Log, DeviceAdapter, CallbackOperator, NTFSLib, Fixup...) | `artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md` §2 |
| `MainModule.dll` | partition-op host; loads `.mo` modules by name: `<Name>.mo` → LoadLibraryA → `GetModule` (26 of 31 export the factory) | same §3 |
| `Mom.mo` (331 fns) | MOM runtime core: CMomModule, IAggObject/IModule/IErrorObject | `artifacts/PartitionRecovery/mo-module-map.md` |
| `Common.mo` / `Config.mo` | shared INI-style config service (CConfig/CConfigSection, IConfig) | same |
| `Log.mo` (139 fns) | universal logging layer — imported by 15+ modules | matrix artifact §2 |
| `Device.mo` (3,474 fns) | device layer: CDiskEnumerator, CDiskDriveLayoutReadWriter, CStorageDisk, BitLockerManager; opens `\\.\EPRDTDRV\<PID-token>\Volume\<N>` / `\\.\EPRDTDRV\<PID-token>\<disk>\<part>` (client side of the PID-token handshake; fallback opens `\\.\HarddiskVolume%d` directly when driver probe fails = no-admin degraded mode) | mo-module-map; `artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md` §3.2/§6 |
| `DeviceManager.mo` (3,428 fns) | CDeviceManager, CPartitionManager(GPT), CTbFile (TB-scale disk image mount), CBaseReg | mo-module-map |
| `PartitionRecoveryD.mo` (1,571 fns) | the lost-partition deep-scan orchestrator: CRecoverySearchEngine ×18, CBlockParseWin32/Linux, CExFatBlockParser, CFatxBlockParser (FATX), BitlockerParser, CGeneratorSetCompleteSearch; embeds strings `lostpart.dat`, `partinfo.dat` (shipped as data files in the same dir; role beyond the name: **not covered**) | `artifacts/MOM-modules/PartitionRecoveryD.md` |
| `RegLib.mo` (302 fns) | offline registry HIVE parser (Hive/Key/Node/BigData/FastLeaf/...) — FVE keys, installed-OS detection | mo-module-map |
| `CheckVersion.mo` | CheckVersion, IsWindowsServerInstalled — OS-aware recovery | mo-module-map |
| `LdmManager.mo` (3,622 fns) | Windows dynamic disks (LDM database: Dmrp*, EditLdm2*, RAID/spanned support) | mo-module-map |
| `NTFSLib.mo` (310 exports) / `NTFSUtil.mo` / `FatLib.mo` / `FatFormat.mo` | NTFS format library (link-time C++ DLL in .mo clothing), utilities incl. CFatVirtual* damaged-disk views, FAT library, FAT formatting | mo-module-map + matrix artifact §3 |
| `FileSystemAnalyser*.mo` (NTFS/FAT/ExFat/Ext/Refs, 265-1,752 fns) | volume recognition/assessment layer (two-layer recovery: analysers recognize → PartitionRecoveryD block parsers deep-scan → SearchDLLs present) | mo-module-map "Architectural insights" |
| `Fixup.mo` / `EFIBoot.mo` / `Win32Bcd.dll` / `WinChkdsk.exe` | boot repair chain: BCD/EFI first-boot fixup, ESP handling, chkdsk wrapper | mo-module-map + per-binary/WinChkdsk.md |
| `bitlockerhelp.mo` (1,239 fns) + `BitLockerLib.dll` | BitLocker integration (XTS-AES content decrypt via OpenSSL; `C:\Program Files\EaseUS\BL` path strings) | mo-module-map; `artifacts/MOM-modules/bitlockerhelp.md` |
| `GetDriverInfo.dll`, `RawFixer.dll`, `RecoveryModule.dll`, `CloneModule.dll`, `ToolsModule.dll`, `UILogic.dll`, `AutoUpdate.dll`, `Boot.dll`, `BootDriver.dll`, `peformat.dll`, `libvmdk.dll` | host/aux DLLs of the module system (per-binary cards exist for each: `artifacts/per-binary/*.md`) | `artifacts/per-binary/` listing |
| `eprdtdrv.sys` + `eprdtdrv_win7older.sys` | EPMNtDrv raw-sector kernel driver (see §6) | `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` |
| `setupdrvx64.exe` | driver deployer (mod.WinDiskAccessDriverSetup): installs+starts the eprdtdrv service on first scan needing raw access; `%s%d` numbered driver variants (eprdtdrv2/3...) enable in-place upgrades without reboot | eprdtdrv artifact §7.1; epmntdrv-trio §1 |
| `setupdrv.ini` | file containing the single section `[eprdtdrv]` — service/driver install config consumed by the setupdrv deployer (full semantics beyond the section name: **not covered**) | `og-codebase/PartitionRecovery/setupdrv.ini` (direct read) |
| `lostpart.dat`, `partinfo.dat` | data files referenced by PartitionRecoveryD.mo strings (lost-partition scan data; exact format: **not covered** in corpus) | PartitionRecoveryD.md §"Notable string evidence" |

### 2.4 RAW carving module detail (RAWSearchDLL.dll)

- Exports (6): `EUCreateRAWSearcher` (0x160-byte CRAWSearchController — lean, no FS
  state), `EUCreateRAWSearcherForCellTest` (in-house unit-test harness:
  CRawFileCellTestStorageDevice fake device), `EUGetAnalyserList/Map/Multimap`
  (three projections of one registry singleton), `GetRawUserInfoMap`.
  Cite: `artifacts/SearchDLL/rawsearch-carving--deep-dive.md` §1.
- Registry = `std::map<uint32 fourcc_magic, Analyser*>` RB-tree; 70 unique magic keys
  (FFD8FFxx JPEG ×4, %PDF, II*/MM* TIFF ×4, PNG, Rar!, PK\x03\x04 ZIP, OggS, EBML
  1A45DFA3 MKV, D0CF11E0 OLE, MSCF CAB, ITSF CHM, AC10xx DWG, SQLi SQLite, RIFF,
  060E2B34 MXF, 75B22630 ASF...); 190+ analyser classes in namespaces
  `CommonRawAnalyser`/`Brance::*`/`BruteForce::*`/`RawSpeed::*`; every format has a
  modern analyser + legacy `File*Analyser` fallback (two validators must both fail
  before a match is discarded). Cite:
  `artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` §1-3.
- Confidence: raw_signs.json entries carry explicit confidence (e.g. ifo
  `0,44564456,0.96`); header+tail+size-gate combination ranks overlapping carves.
  Cite: carving deep-dive §3.
- Carved files feed `IRawDirectoryTreeBuilder` → LostTreeModel/LostTableModel flat
  "Lost data" views; merged with structured results (structured wins; carved attach
  as "Other deleted files"; "File Path Lost" root in .rsf tree). Cite: carving
  deep-dive §4 + SaveScan rsf artifact §5.4.6.

### 2.5 Repair modules (result-pipeline adjacent: preview-failed → repair offer)

| File | Repairs | Cite |
|---|---|---|
| `EUVideoRepair.dll` (in-proc, x64) | MP4/MOV/AVI/MTS/MXF (moov rebuild, regroup, sample-file transplant, self-built sector bitmap) | `artifacts/FileRepair/repair-architecture.md` §1/§B |
| `VideoViewer/EaseUS_VideoViewer.exe` + `EaseUS_MediaGraph.dll` + `VideoViewer/VideoFileRepairer.exe` | video preview/repair UI host (spawned by DRWUI with port 20171/20172 on cmdline; VAPreviewListener TCP+ZeroMQ IPC) | DRWUI architecture §9.5 |
| `EUImgRepair.exe` + `EUFileRepair/EUImgRepairFile.exe` | JPG/PNG/RAW images (Fixo ImageRepair, 37 raw formats: 3fr..x3f) | repair-architecture §1 |
| `EUFileRepair/EuPDFRepairFile.exe` | PDF xref/trailer reconstruction | §5 |
| `EUFileRepair/EuOfficeRepairFile.exe` | HWP/HWPX (Korean Hangul) + encrypted HWPX (aesOperate + Dict passwords); spreadsheet formula AST rebuild | §4 |
| `EuOfficeRepairWin32.exe` | MS Office DOC/XLS/PPT + OOXML package shell synthesis ([Content_Types].xml template) | §4 |
| `EUAdobeRepair.exe` | PSD/PSB (cloud-assisted: NETCLIENT::CClient::ConnectServer, ConvertPsdbToJpeg) | §3 |
| `EUArchiveRepair.exe` + `EUFileRepair/7z.dll` | ZIP/7Z/RAR (7z next-header re-derivation) | §6 |
| `EUHwpViewer.exe` + `EUHwpFilePathCheckerModule.dll` | HWP preview in results UI | §1 table |
| `EUImageEnhance.dll` | image enhancement of repaired thumbnails (OpenCV 2.4 static, 293 RTTI classes) | §1 + DECOMPILED UPDATE A |
| `EURawImg.exe` | RAW image processing | §1 |
| `EUFileRepair/` dir | the Fixo engine drop: 24 `CORE_RL_*` ImageMagick DLLs + 8 `IM_MOD_RL_*` codec modules + 7z.dll + the repair EXEs | og-codebase listing; `artifacts/runtime-families/imagemagick-modules.md` |
| DRWUI classes `CHKPERepair/CHKRepair/CHKRepairAdapterMgr/CHKRepairFileSign` | repair orchestration + **the file-signature checker** (see §4 errata #9: the actual EUUserData consumer) | repair-architecture §4 |

### 2.6 Report / service / cloud / misc binaries

| File | Role | Cite |
|---|---|---|
| `DRWReport.exe` + `DRWReport.ini` | crash reporter; ini declares the report bundle: `log\DelFat.log`, `log\DRWUI.log`, `log\DefaultLog.txt`, `Drwdump0-5.DMP`, `log\BuildRaw.log` — i.e. these log/dump names are the product's own crash-report file manifest | `og-codebase/DRWReport.ini` (direct read) |
| `send_dmp_with_aliyun.exe`, `AliyunWrap.dll`, `AliyunWrapExe.exe`, `AliyunConfig.ini` | crash-dump/log upload chain to Alibaba Cloud OSS (AliyunConfig.ini `[Config]` = obfuscated LOGENDPOINT/ACCESSKEYID/SECRET/PROJECTNAME/LOGSTORENAME, `bAllowSendInfo=0`, `ProductName=drw`) | `artifacts/DRWReport-exe--crash-reporter.md`; `og-codebase/AliyunConfig.ini` |
| `firebasefetch.exe` / `firebaseonlineconfig.dll` / `google-services.json` / `remote_config_data` | Firebase Remote Config fetch + shipped feature-flag snapshot cache (61KB binary key-value, ~600 enable_* flags with `__abroad`/`__free` suffix segmentation; local fallback when fetch fails) | `artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` §2 |
| `cloud_session.dll` | cloud-drive OAuth/session (Dropbox/GDrive/OneDrive) | DRWUI architecture §2 |
| `EuICloudRecovery.dll` + `icloud_logger/icloud_logger` | full iCloud client (esicloud namespace, SRP auth); `icloud_logger` is a RUNTIME LOG file (88 B ASCII: `[2026-10-15 01:31:29.937] INFO: [esicloud::ICloudRecovery::logout:367] User login out.`) — vendor QA leftover | `artifacts/Cloud/cloud-recovery-architecture.md`; direct read |
| `EuDownloader.exe` + `dlconfig.xml` | resumable range-downloader with MD5 verification; dlconfig.xml = WAIK ISO carve manifest (winpe3/4/5/win10pe objects with byte ranges + md5) for the bootable-media builder | `artifacts/config-files/config-data-reference.md` "dlconfig.xml" |
| `PE_Tools.dll` + `PEtools.xml` | WinPE media builder + WAIK tools deployment manifest (bcdboot, wimmount, oscdimg, imagex, DISM stack into `\Windows AIK\Tools\{x86,amd64}`) | same |
| `EuShare/` (EuMrx.sys + install.bat/uninstall.bat) | NAS kernel redirector (static RDBSS, `\Device\EuRdr`, `\\.\EuRdrDN` client in DRWUI, network provider `\\EuShare.com\...`) | `artifacts/EuShare/eumrx-nas-architecture.md` |
| `diskhealth/DiskHealth.exe` (+ Qt5Qml stack, DiskHealth.rcc) | SMART disk-health companion (CrystalDiskInfo-lineage engine CAtaSmart/DiskInfoMgr; internal name DiskSmart; deployed from boot USB by auto.bat Phase E) | `artifacts/DiskHealth/` (artifact header) |
| `EuUserRate.dll`, `EUinApp.exe`, `EuOfficeTools.exe`, `EUUnZip.exe`, `EUOfficeViewer.exe`, `PDFViewer.exe`, `Japanese.dll` | user-rating/in-app-store/office-tools/unzip helper/office+PDF preview viewers/MFC resource-string DLL (loader fallback language DLL: Chinese/Deutsch/Japanese per DRW.exe §1-5; Japanese.dll itself is x86, 4.2 MB, 24 fns) | per-binary cards; DRW-exe--launcher §1 |
| `PubLog.dll` (88 fns, 14 exports) | shared logging DLL (public log helper loaded by UI processes) | `artifacts/per-binary/PubLog.md` |
| `legacy.dll`, `archive.dll`, `InfoForSetup.exe` | legacy/archive helpers — per-binary cards exist, pipeline role: **not covered beyond names** | `artifacts/per-binary/legacy.md`, `archive.md` |
| `7za.exe`, `Burn.dll` (in PartitionRecovery/) | 7z CLI; IMAPI2 disc writer (ISO/USB emergency media, RealTimeWrite streaming) | `artifacts/PartitionRecovery/burn--imapi2-disc-writer.md` |
| `Language/` (37 .qm) | i18n: 25 product locales + 4 Mainland OEM variants (Foxit/JZ/LC channels) + 8 qtbase; language persisted in version.ini SelLanguage | `artifacts/Languages/language-packs--family-census.md` |
| `Resource/` (`drw_light.rcc`, ImageResource.xml), `ResourceDark/`, `styles/`, `style.css` (427 KB) | Qt resource caches + CSS for embedded web views (EUinApp/EUWeChat windows) | `artifacts/config-files/config-data-reference.md` §Install/misc |
| `LICENSE/` (19 files) | third-party license compliance set (opencv, openssl, qt, tinyxml, ...) | og-codebase listing + config-data-reference |
| `auto.txt`, `needcopy.txt` | Inno Setup file manifests (needcopy.txt lists the CRT/api-set DLL set the installer copies); auto.txt carries a GUID + binary blob (role beyond manifest: **not covered**) | direct read of og-codebase |
| `Microsoft.VC90.CRT/MFC.manifest`, msvcrt/msvcp/vcruntime/concrt140, ucrtbase, api-ms-win-* (~60 files) | VC90 + VC120 + VC140 + UCRT runtime generations (three CRT eras shipped simultaneously: VC9-era EAO/legacy, VC120-era Fixo Win32, VC140-era main) | `artifacts/runtime-families/vc-runtime-generations.md`, `ucrt-forwarders.md` |
| Qt5 stack (`Qt5Core/Gui/Network/Svg/Widgets/WinExtras.dll`, `platforms/`, `imageformats/`, `iconengines/`, `styles/`) | UI framework + plugins | `artifacts/runtime-families/qt5-core-stack.md` |
| OpenSSL trio (`libcrypto-1_1(-x64)/3(-x64)`, `libssl*`, `libeay32`, `ssleay32`) | crypto — BitLocker XTS-AES, iCloud, NAS auth, licensing | `artifacts/runtime-families/openssl-crypto-trio.md` |
| `libssh2.dll` (SSH-2.0-libssh2_1.11.0), `liblzma`, `zlib/zlib1`, `pdfium.dll`, `heif.dll`, `de265.dll`, `libx265.dll`, `libzmq-mt-4_3_5.dll`, `cpprest141_2_10.dll`, `dbghelp.dll`, `GDIPLUS.DLL`, media codecs | NAS SSH transport, decompression, PDF/HEIF/AVC codecs, ZeroMQ preview IPC, REST, crash-dump help, GDI+ legacy graphics | runtime-families artifacts; carving/eumrx cross-refs |

---

## 3. Signature & config data files (what each detection layer loads)

### 3.1 raw_signs.json — the built-in RAW signature DB (root of install dir)

- **34,542 B; 139 extensions; 139/139 have header signs, 46 also tail signs**
  (byte-verified by direct JSON parse this session; matches
  `artifacts/config-files/config-data-reference.md` §"Independent verification note").
- Grammar per extension: `{"header": {"signs": ["offset,HEXBYTES,confidence", ...]},
  "tail": {...}, "min": bytes, "max": bytes}`; confidence 0.50-1.00; size bounds prune
  absurd candidates. Example: `"ifo"`: header `"0,44564456,0.96"` (DVD-Video IFO "DVDV").
- **Runtime location & load**: loaded at engine init via
  `std::ifstream("raw_signs.json")` — relative path, i.e. resolved against the
  RAWSearchDLL working/module directory at the install root — read whole, parsed by a
  hand-rolled istream JSON tokenizer (FUN_180021460/21380; NOT CJsonObject for this
  file). Cite: `artifacts/SearchDLL/rawsearch-carving--deep-dive.md` §3 (line cites
  23836-23900).
- Consumer: RAWSearchDLL carving (header/tail/size/confidence ranking of overlapping
  candidates). The same file is NOT what RawConfig.ini toggles (see 3.6).

### 3.2 EUUserData — file-signature DB (root of install dir, no extension)

- **16,891 B JSON; 111 per-extension entries** (byte-verified this session; the
  config-data-reference "16KB" and the userdata-remoteconfig artifact's 111 agree).
- Grammar: `{".ext": {"signs": ["offset,HEX"], "mime": "application/..."}}` —
  libmagic-style with MIME metadata; sharper signatures than raw_signs.json
  (e.g. doc gets OLE GUID + Word6 magic; MXF gets full SMPTE KLVP key).
- **Runtime consumer — THE errata #9 finding**: EUUserData NEVER reaches the RAW
  carving engine. Sole consumer = DRWUI's `CHKRepairFileSign` (RTTI `.?AVCHKRepairFileSign@@`
  / `.?AVCHKRepair@@` / `.?AVICHKRepairObject@@` / `.?AVCHKRepairAdapterMgr@@`):
  read at `%s/EUUserData` (DRWUI_decompiled.c :1005387), parsed (:1005497+, "signs"
  :1005818), consulted by the MatchedFile/NotMatchedFile file-signature checker
  (:721520-721680) for the CHK repair chain. RAWSearchDLL has ZERO EUUserData
  references (rg over 80,614 lines). Cite: `re-workspace/artifacts/CORRECTIONS.md` #9.
- `GetRawUserInfoMap` = the RAWSearchDLL singleton's **per-scan carve-statistics map**
  (insert-or-increment from ~70 analyser detection paths, FUN_18000ed00 :11376-11525;
  cleared at every searcher init FUN_18000beb0 :9044-9051; read one-way post-scan by
  DRWUI's "Trigger_Raw"/"FileSystem" report builder :227538-:227625) — scan OUTPUT,
  never scan input. Cite: CORRECTIONS.md #9 (both-direction decode).

### 3.3 FileFilter.xml — result-category tree (root of install dir)

- **54,597 B; 8 FileCategories: Graphic, Document, Email, Audio, Video, Other,
  Archive, Bookmarks; 1,489 `<Extention>` elements** (grep count this session = 1489;
  the "Extention" typo is in the XML tag itself). ⚠ **CORRECTIONS #5**: the recount
  in the parity-refactor session made it **1,481** extensions (mechanical recount,
  core PARITY-REFACTOR-PLAN §1 #10) — the two counts disagree; the raw grep on the
  shipped file gives 1489, the recount claims 1481. Sheet records both with sources.
- `Selected=2` state flags per node (UI tree state); 40+ camera RAW formats under
  Graphic/TIFF group. Consumer: DRWUI `CAdvanceFileFilterMgr` (Filter dropdown) +
  `CClassify*` tree controllers (Type view). Cite:
  `artifacts/config-files/config-data-reference.md` §FileFilter.xml;
  `artifacts/config-files/filtering-priority--config-layer.md` §6.

### 3.4 config/ INI+XML set (config subdirectory of install dir)

| File | Decoded role | Cite |
|---|---|---|
| `config/RawConfig.ini` | per-extension RAW-carve on/off table (`[Filter] FORMAT=1/0`); ⚠ **CORRECTIONS #5: ships 34 toggles, not 37** (recount; raw grep this session = 34 `=` lines, agreeing with the correction). Maps 1:1 onto the analyser registry's standalone classes (CHFPAnalyser, CHIKAnalyser, COSPAnalyser...video-editor project formats DRP/HFP/OSP/WFP); loaded by RAWSearchDLL at :13133 and DRWUI at :13133 ini machinery | filtering-priority §4; CORRECTIONS #5 |
| `config/FilterFolder.ini` | noise suppression: FolderName/FileName blocklists ($Windows.~BT, Config.Msi, desktop.ini, $UsnJrnl...), FatFilterName = localized "New Folder" in 10 languages + System Volume Information + BitLocker recovery-password artifacts, TxtViewFiles (119 text-preview extensions), SumaryFileType (family grouping: `m4p_m4a_m4v_mp4`, `zip_docx_pptx_xlsx`) | filtering-priority §3 (literal contents) |
| `config/PriorityDisplayType.xml` | Type-tree ordering contract: 5 categories, 35 extensions, `alias` attr = 2-digit sort key (00jpg, 01png...) — presentation order within category; the priority tier vs FileFilter's complete membership tier | filtering-priority §2 |
| `config/usefullostfiles.xml` | "worth recovering" whitelist: 27 `<ext>` (12 photos incl. RAW camera, 9 video, 6 documents) — ranks/highlights valuable files in lost-partition results and the home-screen pitch | filtering-priority §1 |
| `config/config.ini` | global UI config: 22 language IDs + per-locale font stacks, window 1240x840 min 800x600, high_dpi, User state (showRaw, UserRated, FileRepairSavePath, VideoVolume), ENS buy-button coordinates, RecoveryChances toggles, BadSectorTip string | config-data-reference §config.ini |
| `config/shellexe.ini` | UTF-16 shell-extension marketing taglines, 25 locales ("Recover any deleted data with EaseUS") — the ONLY user-visible string outside the .qm system | filtering-priority §5 |
| `config/nasconfig.ini` | NAS connection defaults | config-data-reference §Install/misc |
| `config/themdark.ini` | dark theme config (pairs with ResourceDark/) | og-codebase listing (role: name-evidenced only) |
| `config/version.ini` | build identity: versionNumber=21.0.0.0, version=free, channelName=easeus, marketName=abroad, product-code=RE-DRW-FREE-WIN, SetupBuildTime=1013, SelLanguage; read by DRW.exe loader for channelName | direct read + DRW-exe--launcher §1-4 |

### 3.5 Other root config/data files

| File | Role | Cite |
|---|---|---|
| `Dict` (2,742 B, 228 comma-terminated password lines) | password brute-force dictionary for encrypted-file recovery (BitLocker password unlock via BitLockerLib/manage-bde, encrypted HWPX via EuOfficeRepairFile aesOperate, protected archives) | config-data-reference §Dict (byte-verified counts) |
| `DataFile.ini` (UTF-16, `[HEAD_DATA] TOTALCOUNT=0 STARTINDEX=0`) | data/quota state file — "data/upgrade/free-quota state"; exact consumer: **not covered** | direct read + config-data-reference §Install/misc |
| `Upgrade.ini` (UTF-16, `[VERSION] Product Version=20.8.0.0 ... killDRWUI=1 URL=http://www.easeus.com/...`) | upgrade/version state + download URL | direct read |
| `recoveryFree.ini` | free-edition quota state (sparse/binary-ish content) | config-data-reference §Install/misc |
| `google-services.json` | Firebase project config (remote config + analytics) | config-data-reference §Install/misc |
| `remote_config_data` (61 KB) | fetched Firebase Remote-Config snapshot — shipped DEFAULTS/local fallback (~600 enable_* flags, market/edition suffixes) | userdata-remoteconfig §2 |
| `dlconfig.xml`, `PEtools.xml` | WAIK download + deployment manifests (see §2.6 EuDownloader/PE_Tools) | config-data-reference |
| `NasConfig/NasModel.xml` | Synology model catalog (X86: FS/SA/DS/RS/DVA 2010-2022; ARM: DS1xx-4xx j/play/slim) mapping NAS model → CPU arch → which agent build to push | eumrx-nas-architecture §3.3 |
| `NasConfig/ssh_config.ini` | `[SSH] is_exit/error/errcode/process` state slots, `PushPath=/tmp/EaseUS/`, ScanDev | eumrx-nas-architecture §3.3 (literal contents) |
| `SaveScan/*.rsf` | shipped auto-saved scan sessions (see §5) | SaveScan artifacts |

---

## 4. CORRECTIONS.md errata (mandatory inclusion; rows supersede their originals)

Source: `re-workspace/artifacts/CORRECTIONS.md` — cross-verification errata fed back by
the core_recovery parity sessions. Full register as read (9 entries; #8 and #9
explicitly required by the task):

| # | Artifact / claim | Correction (verbatim substance) |
|---|---|---|
| 1 | NTFS deep-dive: "resident $DATA cap 0xFFF" | Not a resident-size cap — the constant belongs to an MSVC `std::string` destructor sequence, not the attribute parser. No 0xFFF resident gate in the engine. |
| 2 | Two artifacts label FUN_180041970 as the in-use test | FUN_180041970 is the BASE-RECORD test (base_record != 0), not the in-use bit test. Extension-record skip logic keys on it. |
| 3 | FAT algorithms doc: chain walker implied BAD-cluster handling parity | Og follows FAT12 BAD cluster 4087 RAW (garbage-tolerant continuation) — no canonicalization at the 12-bit boundary. Core deliberately canonicalizes; registered delta. |
| 4 | ext4 deep-dive §2: backup-superblock positions (8193, 32768, …) | No such constants exist in the EXT4SearchDLL decompile — overreach from kernel knowledge. UNVERIFIED for the engine. |
| 5 | FileFilter.xml census: 8 categories / 1,489 extensions; RawConfig.ini 37 toggles | Recount: 8 categories / **1,481** extensions; RawConfig.ini ships **34** toggles. |
| 6 | ext4 deep-dive: journal salvage implies commit/descriptor semantics | EXT4SearchDLL gates ONLY on jsb magic 0xC03B3998 + blocktype {3,4}; every block in [s_first, s_maxlen) feeds the directory parser — no commit/revoke handling engine-side. |
| 7 | NTFS artifacts: viability gate boundary | Og is strict `0x3ff < size` — files of exactly 1023 bytes take the fast-path with no bitmap check. Any `>=` reading is wrong. |
| **8** | **Btrfs deep-dive: implied chunk-tree-uuid validation** | **Og does NO chunk-tree-uuid validation (name memcmps only); the kernel SB field at +0x60 is `log_root`-adjacent, not a uuid source.** Evidence: core AUDIT-LINUX-FS BTR-2 re-pin. |
| **9** | **userdata-remoteconfig §1 ("custom RAW signatures (user-extensible carving)") + rawsearch-carving deep-dive :61 area ("consulted alongside raw_signs")** | **EUUserData never reaches the RAW carving engine. Sole consumer: DRWUI's `CHKRepairFileSign` (RTTI .?AVCHKRepairFileSign@@/.?AVCHKRepair@@/.?AVICHKRepairObject@@/.?AVCHKRepairAdapterMgr@@) — read at `%s/EUUserData` (DRWUI :1005387), parsed (:1005497+, "signs" :1005818), consulted by the MatchedFile/NotMatchedFile file-signature checker (:721520-721680) for the CHK repair chain. RAWSearchDLL has zero EUUserData references (rg over 80,614 lines). `GetRawUserInfoMap` = the singleton's per-scan carve-statistics map (insert-or-increment from ~70 analyser detection paths, FUN_18000ed00 :11376-11525; cleared at every searcher init FUN_18000beb0 :9044-9051; read one-way post-scan by DRWUI's "Trigger_Raw"/"FileSystem" report builder :227538-:227625) — scan output, never scan input.** Evidence: first-hand both-direction decode, Agent 2 scan-engine session 2026-10-28; core_recovery CORPUS-ERRATA E-A/E-B + PARITY-DELTAS D-104. |

---

## 5. Runtime file formats — files created/loaded during a scan session

### 5.1 `.rsf` session files (SaveScan/)

- **Format**: 16-byte magic `EA F4 B3 1B 05 43 B6 4A B4 B8 24 4A 33 D9 13 BB` header;
  record stream at 0x8C of 36-byte headers (magic + 16-byte SessionId + chain size) +
  chunked zlib-compressed payloads (4 KB chunks, u16 headers: bit 15 = compressed,
  bits 0-11 = payload_size−1; level-5 deflate; NOT encrypted — CryptoAPI encryption
  hypothesis disproven). Writer `CSaveAndRestoreResult::SaveResult`; owner classes
  IRSData→CRSData→CRSDataEx (0x81178 B) + CZlibCompressor/czlibdecompressor.
  Four records per session: (1) session state, (2) volume/session info incl.
  SYSTEMTIME + elapsed stats, (3) partition context incl. 512-byte boot-sector
  snapshot, (4) the complete found-file tree (CFileNodeRS::SaveFileTree BFS
  serialization; 1,986,806 nodes across the 3 shipped samples).
  Cite: `artifacts/SaveScan/rsf-format--scan-session.md` §2-5 (FULLY REVERSED status).
- **Naming**: `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` for auto-saves — but note
  the SHIPPED samples' names are `auto_Local Disk (G_)10_15_00_44_10.rsf` etc., i.e.
  `<MM>_<DD>...` is the date+time with volume label "Local Disk (G:)" sanitized
  (`/ * : ? " < > |` → `_`, FUN_140286930). User saves via QFileDialog with filter
  `Recovery State File(*.rsf)|*.rsf`. Cite: rsf artifact §5.5 + og-codebase/SaveScan listing.
- **Location**: `<install dir>\SaveScan\` — the folder path is derived from the module
  directory (GetModuleFileNameW → strip filename → append "SaveScan", FUN_1402737f0;
  FUN_140273940 enumerates `auto_*` + `.rsf` there). Cite: rsf artifact §5.5.
- **UI actions**: "SaveScanSession"/"RestoreScanSession", "(1-Save Scan Result" /
  "(1-Import Scan Result", LastScanRecordView session list. Cite: rsf artifact header.

### 5.2 Logs (where they save)

| File | Writer / location | Evidence |
|---|---|---|
| `log\DRWLog.log` | DRW.exe (loader) creates `<exe_dir>\log\` + this file; drains the 2 MB shared mapping `DRW_{1E956C11-...}` every 5 s under mutex `DRWLOG_{1BFCF65F-...}` | DRW-exe--launcher §1 |
| `log\DRWUI.log` | DRWUI main run log (691 lines in shipped QA copy; build fingerprint line: `DRWUI [21.0.0.0_free easeus] build at[Oct 13 2026...] [40504ccb...]`) | `artifacts/config-files/qa-logs--session-forensics.md` §1 |
| `log\CalcPartition.log` | PartitionRecoveryD lost-partition search output (candidate rounds, AdjustPartitionInfoOrder resolution) | qa-logs §3 |
| `log\DefaultLog.txt` | CDirectoryTreeBuilder stage machine ("create 3 new file system tree", build cycles) | qa-logs §4 |
| `log\DelFat.log`, `log\BuildRaw.log` | FAT deletion pass + RAW file-count logs — named in DRWReport.ini's crash-report bundle (content beyond names: **not covered**) | `og-codebase/DRWReport.ini` |
| `EPMLOG.log` (install root) | PartitionRecovery device-manager boot trace (CDeviceManagerBuilder/CDiskEnumerator; shipped vendor QA leftover, 15,269 B) | `artifacts/config-files/vendor-logs--qa-session-forensics.md` §1 |
| `Eaolog.log` (install root) | CEaoActiveOnline (EaseUS Active Online activation/telemetry) init log, UTF-16LE; dynamic runs append `applog_Eaolog.log` entries | vendor-logs §2; `artifacts/UIUX/24-dynamic-ground-truth-ingestion.md` |
| `strutureReg.log` (install root) | video-repair regroup engine JSON debug log (Chinese labels: module 视频重组, branch 19.2回退分支) | vendor-logs §3 |
| `reglog_win.txt` (install root) | installer/registration log | config-data-reference §Install/misc |
| `icloud_logger` (install root, no ext) | esicloud iCloud client log (`INFO: [esicloud::ICloudRecovery::logout:367] User login out.`) | direct read; cloud artifact |
| `DRWDump0..5.DMP` (app dir) | crash dump rotating pair set written by DRWReport (XCrashReport) | DRWReport-exe--crash-reporter + DRWReport.ini |

### 5.3 Config cache & user state created at runtime

| File | What | Evidence |
|---|---|---|
| `DRWCfg.bin` (`<install dir>\`) | AES-256-CBC config cache written by DRWUI's `AES_CBC256` class with a HARDCODED key `XZJE151628AED2A6ABF7158809CF4F3C` / IV `XZJ2030405060708`; config items packed into 0x80-byte records, padded to 16, encrypted, CREATE_ALWAYS. Stores per-scan-state/feature flags/MRU values surviving restarts. Obfuscation, not security. | `artifacts/config-files/drwcfg-bin--aes-config-cache.md` §1-3 |
| `SaveScan\auto_*.rsf` | session auto-saves (§5.1) | rsf artifact |
| `%AppData%\EaseUS\DRW\connect.zip` → `NasConfig\connect.exe` | NAS/Linux agent downloaded at runtime (CLinuxDownloadConnectZipThread fetch, EUUnZip extracts; connect.exe NOT shipped) | eumrx-nas-architecture §3.4/§6.1 |
| `remote_config_data` refresh | Firebase Remote Config fetch cached locally (shipped snapshot = fallback defaults) | userdata-remoteconfig §2 |
| Registry: `SOFTWARE\EASEUS\DRW\Version`, `...\Language`, `SOFTWARE\EASEUS\EaseUS Data Recovery Wizard`, `HKCU\Software\CodeProject\XCrashReportTest` (DRWReport residue), `HKLM\SYSTEM\...\FVEAutoUnlock` (read by DRWService) | installer/loader/DRWService identity keys | cites in §1 + DRWService artifact §2 + DRWReport artifact |

### 5.4 The scan-session runtime footprint (loaded → created, in order)

```
LAUNCH     DRW.exe → creates log\ + DRWLog.log, maps 2 MB DRW_{GUID} + DRWLOG_{GUID} mutex
           reads config/version.ini (channelName), HKLM\...\EASEUS\DRW\Language, startTS env
CONFIG     DRWUI: config/config.ini, FileFilter.xml, FilterFolder.ini, PriorityDisplayType.xml,
           usefullostfiles.xml, raw_signs.json (RAWSearchDLL init, ifstream tokenizer),
           config/RawConfig.ini (:13133), EUUserData (%s/EUUserData → CHKRepairFileSign),
           remote_config_data (Firebase fallback), DRWCfg.bin (decrypt on read)
DRIVERS     setupdrvx64.exe installs+starts eprdtdrv service (SCM) on first raw access;
           Device.mo opens \\.\EPRDTDRV\<PID-token>\<disk>\<part> (or \Volume\{N};
           fallback \\.\HarddiskVolume%d if probe fails)
SCAN       LoadLibraryW ×9 *SearchDLL.dll → EUCreate<FS>QuickSearcher → quick pass;
           Advanced: EUCreate<FS>Searcher set + EUCreateRAWSearcher (gated) → carving;
           PartitionRecovery MainModule.dll + .mo chain for lost partitions (CalcPartition.log)
DURING     log\DRWUI.log, log\CalcPartition.log, log\DefaultLog.txt grow; Eaolog.log appended
           (EAO init); GetRawUserInfoMap accumulates per-extension carve statistics
RESULTS    tree build (3 branches: existing + "File Path Lost" + "Reconstructed");
           classification/filter/sort per config files; previews spawn VideoViewer
           (port 20171/20172) / PDFViewer / OfficeViewer; repair offer → EuRepairHost +
           Fixo EXEs; Dict consulted for encrypted targets
SAVE       SaveScan\auto_<Volume>_<MM_DD_HH_MM_SS>.rsf (zlib-chunked 4-record session);
           DRWCfg.bin rewritten (AES_CBC256); icloud_logger on iCloud use
CRASH      DRW_Crash_/DRW_Memory_Crash_ events → DRWReport.exe → DRWDump0/1.DMP
           (+ DelFat/DRWUI/DefaultLog/BuildRaw bundle per DRWReport.ini)
           → send_dmp_with_aliyun.exe → Aliyun OSS; loader restarts DRWUI (≤1 restart)
```
Assembled from: DRW-exe--launcher §1-2; DRWUI architecture §9.2/§9.4; carving deep-dive
§3-4; rsf artifact §5.5; qa-logs; vendor-logs; drwcfg artifact; DRWReport artifact;
eprdtdrv artifact §7.

---

## 6. Drivers & services installed (pipeline-relevant)

| Driver/service | Install | Device | Role | Cite |
|---|---|---|---|---|
| **eprdtdrv.sys** (live) + `eprdtdrv_win7older.sys` (legacy WDK ABI) | service `eprdtdrv`, deployed by `setupdrvx64.exe` (System32\Drivers via SCM, admin); `%s%d` numbered variants for in-place upgrade | `\Device\EPRDTDRV` + DosDevices symlink (runtime-composed UPPERCASE(service-name); fallback EPMNTDRV) | raw sector passthrough: `\\.\EPRDTDRV\<PID-token>\Volume\<N>` (volume stack top) or `\\.\EPRDTDRV\<PID-token>\<disk>\<part>` (whole-disk Partition0, absolute LBA, 512-B sectors hardcoded); PID-token handshake (transformed-PID match at CREATE, STATUS_INVALID_PARAMETER otherwise); 3 custom IOCTLs 0x60002400/04/08 + transparent IOCTL proxy; pool tag "esdr"; disk.sys stack verification loop | eprdtdrv-sys--driver-internals.md; epmntdrv-trio--naming-pid-handshake.md §1-4 |
| `ebrntdrv.sys` / `epmdkdrv.sys` (WinPE spares; also in live tree) | service demand-start, "Boot File System" group (PE regdrv.reg) | EBRNTDRV / EPMDKDRV | same driver source ×3 builds — PE failover instances when primary blocked | epmntdrv-trio §5; WinPE boot-config §3 |
| `EUDCPEPM.sys` (WinPE only, `BUILDPE/x64/Windows/system32/drivers/`) | BOOT_START class-filter, UpperFilters on GUID_DEVCLASS_DISKDRIVE; DeviceName "epmdiskcopydrv", IoctlBase 0x10000 | filter device | Todo-Backup-lineage hook framework (VolSnap hooking, urgent COW cache, bitmap persistence); shipped in PE because media is shared across EaseUS products | WinPE boot-config §3; eudcpepm-euedkepm artifact §3 |
| `EUEDKEPM.sys` (WinPE only) | SYSTEM_START; DeviceName "EUEDKEPM" | registry DeviceName | Todo-Backup raw-disk building block; dormant in PE | same |
| `EuMrx.sys` (in `EuShare/`, live only; absent from PE) | install.bat (RDBSS static, network provider) | `\Device\EuRdr`, client `\\.\EuRdrDN` | NAS redirector: maps `\\EuShare.com\...` sessions so *SearchDLL scans the NAS letter like a local volume | eumrx-nas-architecture §3.4/§4 |
| DRWService.exe "Read FVE Server" | SCM CreateServiceA, SERVICE_AUTO_START, LocalSystem | pipe `\\.\Pipe\DRWBitlocker` (service = client) | pushes FVE auto-unlock key table (≤100 records, 0x138 stride) to DRWUI | DRWService artifact §1-3 |

Registry keys/services summary for the pipeline: `HKLM\SOFTWARE\EASEUS\DRW\Version`
(installer), `HKLM\SOFTWARE\EASEUS\DRW\Language` (loader), `HKLM\SYSTEM\...\
FVEAutoUnlock\<GUID>\Data` (DRWService read), PE hive seeds in `BUILDPE/x64/regdrv.reg`
(epmdkdrv/ebrntdrv/EUDCPEPM/EUEDKEPM service entries + diskdrive UpperFilters) and
`software.reg` (`Language=English`), `SOFTWARE\EASEUS\EaseUS Data Recovery Wizard`
(product state), `HKCU\Software\CodeProject\XCrashReportTest` (DRWReport residue),
`SOFTWARE\Foxit Software\FoxitRestore\...` (codename key). Full list of cites in §1,
§5.3, §6 rows.

---

## 7. Not covered (corpus silence — no claim made)

- Exact live-tree install path (`C:\Program Files\EaseUS\...`): only the WinPE target
  `Program Files\easeus\DRW\` is documented.
- `lostpart.dat` / `partinfo.dat` format semantics (only PartitionRecoveryD string
  evidence exists).
- `DataFile.ini` / `recoveryFree.ini` exact consumers.
- `auto.txt` GUID blob semantics.
- DelFat.log / BuildRaw.log content (names only, from DRWReport.ini).
- Whether FileFilter.xml's true count is 1489 or 1481 (CORRECTIONS #5 recount vs raw
  grep of the shipped file — both recorded with sources in §3.3).
- The loader's shared-log ring buffer file backing (mapping exists; on-disk file other
  than DRWLog.log: not covered).
