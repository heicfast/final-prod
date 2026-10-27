# REFERENCE-MODULES — EaseUS Data Recovery Wizard (DRW): Module Architecture (files → modules → loading)

> Task T4-C (Agent C) evidence sheet; companion to `docs/REFERENCE-FOOTPRINT.md`.
> Reference product = EaseUS Data Recovery Wizard, corpus
> `/home/z/my-project/easeus-drw-research` @ `03ad46c`. All claims cited to corpus
> evidence (`architecture/*.md`, `re-workspace/artifacts/**`, `re-workspace/reversed/**`,
> `re-workspace/inventory.json`). Gaps are marked **NOT-IN-CORPUS**. Read-only task.

**Citation shorthand**: `arch/NN` = `architecture/NN-*.md`; `art/<name>` =
`re-workspace/artifacts/<name>`; `rev/<bin>` = `re-workspace/reversed/<bin>`.

---

## 1. Process topology — which process hosts what (EVIDENCED)

```
DRW.exe (DRWLoader, requireAdministrator)                 [art/DRW-exe--launcher.md]
 └─ DRWUI.exe (requireAdministrator) — hosts EVERYTHING in-proc except the children below
     ├─ (in-proc) 9+1 *SearchDLL.dll engines            [art/DRWUI… §9.2]
     ├─ (in-proc) PartitionRecovery.dll → 30 .mo MOM modules  [art/PartitionRecovery/mom-module-system--architecture.md]
     ├─ (in-proc) cloud_session.dll / EuICloudRecovery.dll / EuUserRate.dll / archive.dll …
     ├─ DRWService.exe (own SCM service "Read FVE Server", LocalSystem, AUTO_START;
     │                 pipe CLIENT to \\.\Pipe\DRWBitlocker served by DRWUI/BitLockerLib)
     │                 [art/DRWService-exe--bitlocker-service.md §3]
     ├─ EuRepairHost.exe (repair licensing bridge, activation.easeus.com)
     │    └─ Fixo repair EXEs as children (CLI commandParser.cpp each):
     │       EUImgRepairFile / EuOfficeRepairFile / EuPDFRepairFile / EUArchiveRepair /
     │       EUAdobeRepair … (Win32/x86, out-of-process from x64 UI)
     │                 [arch/06-repair-chain.md §5; art/FileRepair/repair-architecture.md]
     ├─ Office/PDF/HWP viewers out-of-proc: EUOfficeViewer.exe / PDFViewer.exe /
     │   EUHwpViewer.exe — spawned by OfficePreviewStub::OnOpenViewerThreadPro with
     │   process-tree termination helpers on close  [arch/04 §6; art/DRWUI… §3.5]
     ├─ EaseUS_VideoViewer.exe (media preview helper): DRWUI spawns it with the preview
     │   port on its command line; talks back over TCP 127.0.0.1:20171/20172 + a ZeroMQ
     │   REP control channel (602B MediaInfo header + ≤1MiB BMP thumbnails)
     │                 [art/DRWUI… §9.5; art/DRWUI-subsystems--assistant-grading-preview.md]
     ├─ firebasefetch.exe (out-of-proc Firebase remote-config fetch →
     │   DrwMainWindow::doFirebaseFetchFinished)  [art/Cloud/cloud-recovery-architecture.md §4]
     ├─ VideoFileRepairer.exe (VideoViewer-side repair entry) [per-binary/VideoFileRepairer.md]
     ├─ DRWReport.exe (spawned by DRW.exe, not DRWUI, on crash events; XCrashReport;
     │   minidumps DRWDump0-5.DMP; upload via send_dmp_with_aliyun.exe → Aliyun OSS)
     │                 [art/DRWReport-exe--crash-reporter.md]
     └─ AliyunWrapExe.exe (observed live in the Wine process tree DRW→DRWUI→AliyunWrapExe)
                        [art/UIUX/dynamic-ground-truth--wine-loader-run.md]
Separate surfaces: diskhealth/DiskHealth.exe (own Qt Quick app; "EaseUS_DiskHealth"
window; console variant; launched from boot USB in PE) [art/DiskHealth/…companion.md];
PartitionRecovery/WinChkdsk.exe (chkdsk wrapper); 7za.exe; installer pair SetupUE/InfoForSetup.
Kernel (loaded via SCM, consumed through device namespaces):
  eprdtdrv.sys \Device\EPMNTDRV + \\.\EPRDTDRV\d\p — opened by PartitionRecovery Device.mo
  EuMrx.sys \Device\EuRdr — opened by DRWUI's CEuShare (\\.\EuRdrDN) + as Network Provider
  EUDCPEPM/EUEDKEPM.sys — WinPE-only (BUILDPE drivers/), no live opener for EUEDKEPM
  ebrntdrv/epmdkdrv.sys — WinPE-only copies of EPMNtDrv      [arch/02 §1, §6]
```

---

## 2. Loading mechanisms — the five patterns (all EVIDENCED)

| # | Mechanism | Contract | Evidence |
|---|---|---|---|
| L1 | **Static import-table linkage** | Qt5*.dll, VC/UCRT/OpenSSL runtimes, zlib… resolved by loader | `re-workspace/inventory.json` import tables; `art/runtime-families/*` |
| L2 | **`LoadLibraryW` + `GetProcAddress` factory (engine plugin contract)** | DRWUI `FUN_14056c0d0` loads all 10 engine DLLs into globals `DAT_1425e6d30..78` at startup; per scan `GetProcAddress(h, "EUCreate<FS>QuickSearcher"/"EUCreate<FS>Searcher")(&p)` fills an interface pointer; FS-code→factory map in `FUN_140577d40`; full set + RAW in `FUN_140580720` | `art/DRWUI-exe--main-ui-architecture.md` §9.2; `art/SearchDLL/searchdll-family--architecture.md` §1 |
| L3 | **MOM plugin loading (in-house COM)** | engine requests module BY NAME (`FUN_1800017f0("DeviceManager")`) → name + `.mo` → `LoadLibraryA("Device.mo")` → `GetProcAddress("GetModule")` (required) → `GetModule(HMODULE)` self-registers; per-class factory `CreateObject`; on-demand + singleton-cached. **Two coupling styles**: 26 of 31 `.mo` are factory plugins; 5 (CallbackOperator, DeviceAdapter, Log, Fixup, NTFSLib) are link-time shared libs bound via import tables (NTFSLib = 310 exports); host DLLs (PartitionRecovery/MainModule/RecoveryModule/ToolsModule/UILogic) are both | `art/PartitionRecovery/mom-module-system--architecture.md` §1.1 (line-cited); `art/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md` §3 |
| L4 | **Out-of-process spawning (`CreateProcess`)** | repair chain via EuRepairHost; viewers; preview media helper (port passed on cmdline); firebasefetch; DRWReport | arch/06 §5; art/DRWUI… §9.5; arch/04 §6 |
| L5 | **SCM services / kernel drivers** | DRWService ("Read FVE Server"); eprdtdrv + EuMrx installed by setupdrvx64.exe / install.bat | REFERENCE-FOOTPRINT.md §4; arch/02 §5 |

Dynamic-only loads also evidenced: `libssh2.dll` "loaded dynamically (no static importers —
LoadLibrary pattern)" for NAS (`art/EuShare/eumrx-nas-architecture.md` §3.2); DRWLoader's
optional language DLLs (`Chinese.dll/Deutsch.dll/Japanese.dll` — `art/DRW-exe--launcher.md` §1).

**Engine→UI bridge (how loaded modules talk back)**: `CMessageProxy` singleton
(MI QObject + INotifyObserver) receives MOM observer callbacks on engine threads and
re-emits them as Qt signals via a per-message `CMessageSend` envelope with thread-affinity
dual-connect (direct vs queued marshaling) → 15 UI subscriber classes.
`art/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md` §1.

---

## 3. Module families (file → module → provision → loading → dependents)

### 3.A Supervisor & core processes

| File(s) | Module | Provides | Loading | Product surface depending on it |
|---|---|---|---|---|
| `DRW.exe` | DRWLoader | crash-event watchdog (`DRW_Crash_{GUID}`, `DRW_Memory_Crash_{GUID}`), 2 MB shared-log mapping flush every 5 s, DRWUI restart loop, DRWReport handoff with `{"Elapsed","Error","Result"}` JSON, `startTS` env epoch, `-recycle` passthrough | entry process (user launch / winpeshl.ini) | entire product (DRWUI verified to exit standalone — "DRWUI requires DRWLoader", `art/UIUX/dynamic-ground-truth--window-census-verification.md`) |
| `DRWUI.exe` (119,182 fns / 2.38M lines; 1,073 RTTI classes; 126 windows; 44 threads) | The Qt UI + orchestrator | wizard stack (`DrwMainViewManagerWizard` = QStackedWidget, 4 pages), scenario controllers (`CDoScanPartition/LostPartition/CloudDevice/RemoteDevice`), scan mgr, results tree/classify/filter/preview subsystems, `CRecoveryMgr` export chain, repair wizards, licensing/monetization, telemetry (195 events), crypto inventory (AES-256-CBC DRWCfg key `XZJE151628AED2A6ABF7158809CF4F3C`, LM-hash, MD5/SHA-256, zlib, AES-XTS via BitLockerLib) | host process; statically carries `drwintelligentscan\{filerecover,recoverycontroller,hfspluslib,cloudsearch,storagedevicemodule,uicontroller\eushare,videorepair,qtdrwui}` (embedded `__FILE__` paths, §9.1) | everything |
| `DRWService.exe` | "Read FVE Server" service | LocalSystem FVE auto-unlock key broker: enumerates `HKLM\…\FVEAutoUnlock`, pushes ≤100 records (0x138 stride: 50-char GUID + 256 B key blob) in ONE pipe message to `\\.\Pipe\DRWBitlocker` (service = pipe CLIENT) | SCM service (L5), AUTO_START; installed by DRWUI via `/install` | BitLocker volume scans/unlock (BitlockerDevice, CCheckBitlockerPartition, BitlockerUnlockThread) |
| `DRWReport.exe` + `DRWReport.ini` | XCrashReport crash reporter | minidumps (DRWDump0-5), loader-JSON telemetry, MFC report UI, Aliyun upload handoff | child of DRW.exe on crash (L4) | crash resilience + vendor telemetry |
| `PubLog.dll` (88 fns) | shared publish-log | logging for loader-side | L1 (imported by DRW.exe: inventory imports list) | DRW loader logging |
| `legacy.dll` | legacy crypto (libtomcrypt: DES, PBKDF1) | backward-compat crypto substrate | L1 | (legacy session/license paths — plausibly old .rsf; inference in artifact) `per-binary/deepening-batch1 §4` |
| `GDIPLUS.DLL` | local GDI+ copy | image rendering on hosts w/o OS GDI+ | L1 | preview/repair image paths |

### 3.B Filesystem scan engines (9 DLLs + RAW; plugin family "DRWIntelligentScan")

| File | Module (controller classes) | Provides | Loading | Dependents |
|---|---|---|---|---|
| `NTFSSearchDLL.dll` (1,931 fns) | `CNTFSSearchController` (0x940 B), `CNTFSQuickSearch(Base)`, `CNTFSBuildVolumeBitmap` | MFT record walk incl. deleted (bit-4 scoring), $ATTRIBUTE_LIST fork, runlist nibble math, volume bitmap, fastsearch default | L2 | quick+advanced scans on NTFS; exports `EUCreateNTFS{,Quick}Searcher` |
| `FATSearchDLL.dll` (1,265 fns) | CFATSearchController + FAT dir-entry parsers | FAT12/16/32 chain walk, LFN, 2 FAT tables, EOC masks, 0xE5 deleted markers | L2 | FAT scans |
| `exFATSearchDLL.dll` (1,299 fns) | exFAT controller family | bitmap/dentry/UP-case walk, InUse-mask triples | L2 | exFAT scans |
| `EXT4SearchDLL.dll` (475 fns) | CEXT4 controller | superblock/inode/group-desc/extent(htree) walk | L2 | ext2/3/4 scans (also `EUCreateEXT2QuickSearcher` for FS codes 4-5) |
| `HFSPlusSearchDLL.dll` (665 fns) | HFS+ controller + full hfspluslib | catalog/extents/attribute B-trees, journal, raw file access | L2 | HFS+ scans (macOS disks) |
| `ReFSSearchDLL.dll` (629 fns) | ReFS controller | metadata tables, identity B+ trees, refs:: MSB | L2 | ReFS scans |
| `BtrfsSearchDLL.dll` (452 fns) | Btrfs controller | superblock@0x10000, chunk/b-trees, zlib inflate, RC4, CRC32c; exports `EUCreateBtrfsComplateSearcher` [sic] | L2 | Btrfs scans |
| `ZfsSearchDLL.dll` (981 fns) | ZFS controller | uberblock ring (0xBAB10C), txg selection, MOS/DMU/ZAP | L2 | ZFS scans |
| `RAWSearchDLL.dll` (1,921 fns; 182 RTTI types, 106 analyser classes) | `CRAWSearchController` + 190+ `IRawFileAnalyser` (namespaces BruteForce/RawSpeed/xjq/Brance/File-gen) | signature carving: u16 magic-key RB-tree registry + analyser ID map; 70 magic keys byte-decoded; extra exports `EUGetAnalyserList/Map/Multimap`, `GetRawUserInfoMap`, `EUCreateRAWSearcherForCellTest` | L2, **gated**: created in advanced scan only when settings flag (obj+0x60) enables carving; receives a `std::function` naming/filter callback via vtable+0x48 | "Advanced Scan… sector by sector", Deep Video Reconstruct, lost-data branches |

Family evidence: `art/SearchDLL/searchdll-family--architecture.md` (exports verified in
all 9; per-engine deep-dives `art/SearchDLL/*-engine--deep-dive.md` ×9);
engine source tree `drwintelligentscan\{ntfs,fat,exfat,ext4,refsearch,btrfssearch,zfssearch,rawsearch}search\`
(arch/05 §1). **All 10 DLLs are LoadLibrary'd at startup into globals** (always resident);
searcher objects are created per scan (L2 evidence §9.2).

### 3.C RAW carving data files (decoded formats)

| File | Format (decoded) | Consumer |
|---|---|---|
| `raw_signs.json` | 139 ext → `{header/tail: [offset,HEX,confidence 0.50-1.00], min, max}`; 46 with tail signs | RAWSearchDLL (CJsonObject parse; per-ext carve switches in `config/RawConfig.ini`, 37 types) |
| `EUUserData` | ext → `{signs:[offset,hex], mime}` (libmagic-style, e.g. `"doc": ["0,0D444F43",…]`) | file-type ID for preview + repair routing |
| `Dict` | 228 comma-terminated passwords | BitLocker password unlock (BitLockerLib), encrypted HWPX (EuOfficeRepairFile aesOperate), protected archives/PDF |

`art/config-files/config-data-reference.md` (byte-verified counts).

### 3.D PartitionRecovery engine (MOM module system — "lost partition" substrate, EPM-shared)

**Hosts (all x64 PE DLLs in `PartitionRecovery/`)**:

| File | Role | Loading | Citation |
|---|---|---|---|
| `PartitionRecovery.dll` (892,648 B; 59 IObject-family exports) | engine host + device object model (`CPartitionDevice`, `IDiskDevice`, `IStorageDevice`, `CCallBackOperate`), loads .mo on demand | L3 host (loaded by DRWUI; path fallback `<exe_dir>\RecoveryModule.dll` → `<exe_dir>\PartitionRecovery\…`) | mom-module-system §1.1/§2 |
| `MainModule.dll` (1,826,536 B; 153 RTTI classes; 268 mangled exports) | partition-editing engine (EPM-shared): CBaseOperation family (create/delete/merge/move-resize, MBR↔GPT, copy disk/partition, repartition), `COperationManager` transaction queue, BCD surgery (`CBcdAccess`, CGenUEFISystemFile), ACL handling | L3 host | mom-module-system §2 |
| `RecoveryModule.dll` (1,348,840 B) | recovery-scenario driver (scan orchestration for lost partitions) | L3 host | same |
| `ToolsModule.dll` (2,578,664 B) | toolbox: WinPE media building, burning, imaging (links PE_Tools, Burn.dll, libvmdk) | L3 host (heaviest importer: 7 modules) | same; dependency matrix §2 |
| `UILogic.dll` (1,448,168 B) | presentation logic for partition lists/confirmations | L3 host | same |
| `CloneModule.dll` (2,260,712 B) | disk-clone host (Todo-Backup lineage) | L3 host | dependency matrix §2 |
| `RawFixer.dll` (706,792 B) | transactional FS repair: NTFS/FAT/exFAT fixers, 31-method inventory (CheckDBR/FixFromDBR/FixByScanSector/Undo), pre-scan FS repair | L1 (imported by hosts) | `art/PartitionRecovery/rawfixer-fs-repair.md` |
| `BitLockerLib.dll` (722,152 B) | BitLocker impl: protectors, BEK, PSUnlock (`manage-bde`/`Unlock-BitLocker` fallback), EOW metadata, AES-XTS, unlock flow | L1 | `art/PartitionRecovery/bitlockerlib--bitlocker-implementation.md` |
| `Boot.dll`, `BootDriver.dll`, `Win32Bcd.dll` (261,864 B), `EFIBoot.mo`/`Fixup.mo` | boot-chain analysis/repair (BCD/EFI) | L1/L3 | mo-module-map |
| `Burn.dll` (165,608 B) | IMAPI2 disc writer + real-time streaming burn mode | L1 | `art/PartitionRecovery/burn--imapi2-disc-writer.md` |
| `AutoUpdate.dll`, `Logger.dll`, `peformat.dll`, `GetDriverInfo.dll` (TB-MOM drive-letter topology module — naming trap), `libvmdk.dll` (VMDK parser), `WinChkdsk.exe` (chkdsk wrapper w/ VSS coordination), `zlib.dll`/`zlib1.dll`, `Qt5Core.dll` (local copy) | support | L1/L4 | per-binary/*; mo-module-map; deepening-batch1 §2 |
| `eprdtdrv.sys` + `eprdtdrv_win7older.sys`, `setupdrvx64.exe`, `setupdrv.ini`, `lostpart.dat`, `partinfo.dat` | driver payloads + installer + data | L5 | REFERENCE-FOOTPRINT §4 |

**The 30 `.mo` modules** (all decompiled, 689K lines; RTTI-verified roles —
`art/PartitionRecovery/mo-module-map.md`; per-module docs `art/MOM-modules/<Name>.md` ×30):

Infra kernel (link-time): `Log.mo` (15+ importers — universal logging), `CallbackOperator.mo`
(event contract, 8 importers), `DeviceAdapter.mo` (76 exports), `Fixup.mo` (36),
`NTFSLib.mo` (310 exports — entire NTFS format library), `Mom.mo` (MOM runtime:
CMomModule, IAggObject/IModule, functors), `Common.mo`/`Config.mo` (INI config),
`Thread.mo` (CThread/CMutex/CEvent), `Error.mo`, `CheckVersion.mo`
(IsWindowsServerInstalled — OS-aware recovery), `RegLib.mo` (offline registry HIVE
parser: Hive/Key/FastLeaf/HashLeaf — FVE keys, installed-OS detection).

Domain (factory plugins, lazily loaded by name): `Device.mo` (3,474 fns; opens
`\\.\EPRDTDRV\…`), `DeviceManager.mo` (3,428 fns; enumeration/WMI/hot-plug; CTbFile
TB-scale disk-image abstraction), `Partition.mo` (MBR/GPT/EBR), `LdmManager.mo` (3,622
fns; dynamic disks Dmrp*/EditLdm2*), `PartitionRecoveryD.mo` (deep-scan block parsers
CBlockParseWin32/Linux, FATX, BitlockerParser, CGeneratorSetCompleteSearch),
`FileSystem.mo`, `FileSystemCheck.mo`, `FileSystemAnalyser.mo` +
`{NTFS,FAT,ExFat,Ext,Refs}FileSystemAnalyser.mo`, `NTFSUtil.mo` (2,871 fns;
CFatVirtual* views), `FatLib.mo`, `FatFormat.mo`, `bitlockerhelp.mo` (1,239 fns; FVE
structures/FVEK; references `C:\Program Files\EaseUS\BL`).

**Runtime position**: DeviceManager enumerates → Partition/Ldm parse → analysers
validate → PartitionRecoveryD deep-scan → SearchDLL file layer presents results; lost
partitions inserted as scan targets by `CGenericRecoveryController`
("Insert lost partition %s, %d, StartSector=%I64d…"). `arch/04` §3-4;
mom-module-system §3. **In WinPE this whole directory is absent** → no lost-partition
mode in the bootable edition (PE census §3).

### 3.E File-repair chain (two worlds)

| File(s) | Module | Provides | Loading | Dependents |
|---|---|---|---|---|
| `EUVideoRepair.dll` (7,154 fns; DRW monorepo `videorepair\` 26 files) | in-proc video repair | weighted ISO-BMFF atom grammar, two-scan, moov triage + **sample-file substitution**, sector-bitmap fragment tracking, org-FS-datarun vs regrouped-datarun selection, MP4/MOV/AVI/MTS/MXF regroup | in-proc (DRWUI; statically from drwintelligentscan) | "Try Advanced Repair", Deep Video Reconstruct; can consume engine dataruns |
| `EuRepairHost.exe` (50 fns) | repair licensing bridge | activation.easeus.com[.cn] check before engines run | L4 parent of engines | all Fixo repair features |
| `EUFileRepair/EUImgRepairFile.exe` (5.2 MB; 37-file source tree, 29 C<Format>Info classes) | image repair engine | JPEG marker state machine, PNG/WebP/HEIC state machines, GIF RB-tree re-chain, BMP DIB taxonomy, TIFF 4-tier, SVG envelope-injection, 21 RAW camera formats, JXL libjxl, AVIF ffmpeg shell-out, LibRaw static (560 exports), window-IPC + registry invocation (`SOFTWARE\EASEUS\EaseUS Data Recovery Wizard\EUImgRepair`) | L4 child (CLI commandParser) | image repair wizards; also invoked via window IPC |
| `EUFileRepair/EuOfficeRepairFile.exe` (10.5 MB; 20 files) | Office/iWork/HWPX repair | HWPX container + OpenSSL CFB decrypt, legacy HWP 3/5 record surgery, **Apple iWork IWA engine** (iwk:: ZipFile + TSP/TST/TN), OPC manifest synthesis | L4 child | office repair |
| `EUFileRepair/EuPDFRepairFile.exe` (4,649 fns) | PDF repair | xref/trailer synthesis (byte-exact `"%010lld 00000 n"`), /Encrypt /ID preservation, decrypt (Dict brute-force + user password) | L4 child | PDF repair wizard |
| `EUFileRepair/EUArchiveRepair.exe` + `7z.dll` | archive repair | 7z next-header recovery (7zNextHeader.cpp), bit7z pinned SDK, password-aware | L4 child | archive repair |
| `EUFileRepair/EUAdobeRepair.exe` (3 files) | Adobe repair + cloud client | Adobe format repair, server-assist | L4 child | Adobe repair |
| root copies `EUImgRepair.exe`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`, `EuOfficeRepairWin32.exe`, `EuOfficeTools.exe` | root-level repair entries/hosts | launcher/Win32 hosts of the same engines (window-IPC invocation chain) | L4 | repair home + results-screen repair hooks |
| `EUImageEnhance.dll` (3.06 MB; 9,841 fns) | OpenCV 2.4 static | post-repair photo denoise/enhance | in-proc | image repair save path |
| `EURawImg.exe` (784 fns), `EUUnZip.exe` (1,325 fns) | RAW container conversion; repair-tolerant zip extraction | agent zip extraction (connect.zip), archive handling | L4 | NAS agent flow; archive paths |
| ImageMagick set: 24 `CORE_RL_*` + 7 `IM_MOD_RL_{heic,jpeg,png,psd,svg,tiff,webp}` | ImageMagick delegates | image decode/encode for repair | L1 (imported by EUFileRepair engines) | image repair |
| `pdfium.dll`, `heif.dll`, `libde265.dll`, `libx265.dll`, `libcurl.dll` (EUFileRepair copy) | render/codec/net | PDF raster, HEIF/HEVC, cloud assist (net/client.cpp) | L1 | repair + preview |

Evidence: `arch/06-repair-chain.md` (whole doc); `art/FileRepair/repair-architecture.md`
+ the six FileRepair deep-dive artifacts.

### 3.F Preview & viewers

| File(s) | Module | Provides | Loading |
|---|---|---|---|
| DRWUI in-proc previewers | `CPreviewController` + 41-class system, `IPreviewStub` plugin family (Mem/Office/Pdf/Image/RtfTxt/Archive/AV), `IInWindowStream` streaming, 15 s video cap (`VideoPreviewWnd::timeout()` QTimer 15000), 100 MB cap, damage grading | in-proc |
| `PDFViewer.exe` (9,206 fns) | PDFium preview host | L4 (`OfficePreviewStub::OnOpenViewerThreadPro`; process-tree termination on close) |
| `EUOfficeViewer.exe` | Office document viewer | L4 |
| `EUHwpViewer.exe` + `EUHwpFilePathCheckerModule.dll` | Korean HWP viewer + IsAccessiblePath security gate (window-station checks) | L4 (`art/FileRepair/hwp-pathchecker--module-decode.md`) |
| `VideoViewer/EaseUS_VideoViewer.exe` + `EaseUS_MediaGraph.dll` + ffmpeg-6x set (`avcodec-62` etc.) + `ffmpeg.exe` | media preview helper | L4 + TCP 20171/20172 + zmq REP (§1) |
| `VideoViewer/VideoFileRepairer.exe` | video-repair UI entry (VideoViewer side) | L4 |
| `Japanese.dll` | Win32 ja-JP resource satellite (dialogs/strings/bitmaps lang 1041; zh-CN cursor leftovers fingerprint source lineage) | L1 resource DLL |

### 3.G Downloader / updater / config-fetch

| File | Module | Provides | Loading | Dependents |
|---|---|---|---|---|
| `EuDownloader.exe` (4,646 fns) | resumable range-downloader w/ MD5 verify | WAIK ISO carving (dlconfig.xml), connect.zip agent, tools.zip, holiday resources | L4 | Emergency-disk builder; NAS agent |
| `PE_Tools.dll` + `PEtools.xml` + `dlconfig.xml` | WinPE media builder | WAIK download → stage → DISM WIM mount/customize → inject DRW tree + drivers + auto.bat → ISO/USB via Burn.dll (EuPEMaker source files also embedded in DRWUI §9.1) | in-proc (DRWUI CPEStartWnd flow) | "Create Emergency Disk" |
| `firebasefetch.exe` + `firebaseonlineconfig.dll` + `google-services.json` + `remote_config_data` | Firebase remote-config | feature flags/A-B (`COnlineConfigFirebase`, `DrwMainWindow::doFirebaseFetchFinished`) | L4 + in-proc DLL | monetization surfaces, holiday theming |
| `PartitionRecovery/AutoUpdate.dll` | update checker module | version checks | L3/L1 | CheckUpdateThread (`CVersionCtrl::doCheckUpdate`) |
| `Upgrade.ini` | upgrade descriptor | 20.8→21 upgrade URL, killDRWUI=1 | data | updater handoff |
| `EuUserRate.dll` | rate prompt | post-recovery survey | in-proc (live only) | recovery-complete survey |

### 3.H Cloud & NAS

| File(s) | Module | Provides | Loading |
|---|---|---|---|
| `cloud_session.dll` + `cpprest141_2_10.dll` | OAuth2 cloud sessions (Dropbox/Google/OneDrive) | loopback code listener, seekable download streams (engines read cloud files with arbitrary offsets), ItemInfo async enumeration | in-proc (live only; absent in PE) |
| `EuICloudRecovery.dll` (7.2 MB; 7,064 exports; esicloud namespace; libsrp + OpenSSL static) | full iCloud client | SRP-6a "Grand Slam" auth (idmsa.apple.com/appleauth/auth), setup/ws endpoints, requestPCS (ADP), Drive/Photos/PIM/Keychain/Health, ICloudDriveUploadTask | in-proc |
| `EuShare/EuMrx.sys` + `libssh2.dll` + `NasConfig/{NasModel.xml,ssh_config.ini}` | NAS subsystem | static-RDBSS mini-redirector (`\Device\EuRdr`), SSH agent push to `/tmp/EaseUS/`, EuRdrDN session IOCTL protocol (0x140320/324/328/4b0/4b4/640/644), Synology model→arch agent selection, loopback bridge `\\127.0.0.1\...` | driver L5 + `libssh2` LoadLibrary; DRWUI side = `CEuShare::OpenMrx` (`\\.\EuRdrDN`) | NAS & Linux recovery |

### 3.I Kernel drivers (all 7)

| Driver | Device/namespace | Role | Deployment | Citation |
|---|---|---|---|---|
| `eprdtdrv.sys` (EPMNtDrv) | `\Device\EPMNTDRV`, `\\.\EPRDTDRV\{d}\{p}` / `…\Volume\{n}` | raw-disk proxy: CREATE resolves `\Device\Harddisk{d}\Partition0` (DR{p} fallback, stack walk-down), READ/WRITE = MDL async sector IO (len in 512 B sectors, `Information = Length<<9`), 3 custom IOCTLs (0x60002400/04/08) + transparent IOCTL passthrough; pool tag "esdr"; PID checks | live, via setupdrvx64 (L5) | `art/PartitionRecovery/eprdtdrv-sys--driver-internals.md` |
| `eprdtdrv_win7older.sys` | same | Win7- build of same source | shipped in tree, deployed by same setup | arch/02 §1 |
| `ebrntdrv.sys` | same | EPMNtDrv renamed for WinPE | BUILDPE system32, regdrv.reg demand | arch/02-03 |
| `epmdkdrv.sys` | same | EPMNtDrv renamed for WinPE | BUILDPE system32, regdrv.reg demand | arch/02-03 |
| `EUDCPEPM.sys` (72 KB) | dynamic `\Device\%s`; "epmdiskcopydrv" | device-stack HOOK: all 28 MajorFunction slots hooked, PnP AddDevice attach, custom 15-cmd IOCTL protocol (0x800C2AC8-B38: OpenSession/GETNEXTBLOCK pump/GET_PROGRESS…), VolSnap hook; disk-class UpperFilter in PE | BUILDPE drivers, BOOT_START | `art/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md`; arch/02 §3 |
| `EUEDKEPM.sys` (19 KB; EuDskAcs) | `\Device\EUDKEPM` | named raw-disk access door (PID-token gated, `\Driver\Disk` forwarding); SYSTEM_START; **no in-tree opener — dormant** | BUILDPE drivers | arch/02 §7 |
| `EuMrx.sys` (215 KB) | `\Device\EuRdr`; `\\.\EuRdrDN` user door | RDBSS network redirector (NAS) + Windows Network Provider "EaseUS Network" | live via install.bat (L5); absent in PE | `art/EuShare/eumrx-nas-architecture.md`; `art/EuShare/eumrx-rdbss-internals--minirdr-dispatch.md` |

### 3.J Companions & installer-side

| File | Module | Provides | Loading |
|---|---|---|---|
| `diskhealth/DiskHealth.exe` + Qt Quick stack (818 files) | DiskSmart SMART companion (CrystalDiskInfo lineage: CAtaSmart multi-transport SAT/CSMI/NVMe-Realtek, USB bridge tables) | SMART dashboard, tray guard, notification launch ("Click_Open_DiskHealth_By_Notification") | own process; addtask/querytask/deltask.bat = scheduled-task management (task set NOT-IN-CORPUS) |
| `7za.exe` | standalone archiver | setup-time operations (live only) | L4 |
| `EUinApp.exe` | EUSetWebCtrlIEVersion bootstrapper | FEATURE_BROWSER_EMULATION writes; **orphaned** (zero references corpus-wide); purchase webviews actually live inside DRWUI's embedded-IE windows | not loaded (orphan) — `architecture/05-uiux-architecture.md` §11 |
| `SetupUE.exe` / `InfoForSetup.exe` / `unins000.exe` | installer bootstrap / telemetry / Inno uninstaller | see REFERENCE-FOOTPRINT §6 | install-time only (absent from PE) |
| `AliyunWrap.dll`, `AliyunWrapExe.exe`, `send_dmp_with_aliyun.exe` | Aliyun OSS upload channel | crash-dump/log upload | L1/L4 (live only) |
| `BUILDPE-only`: `EuSendMessage.exe` (WM_COPYDATA cmd 999 → `DRW_Installer` window), `d3d8thk/d3d9/d3d11/dxgi.dll` D3D shims, `mfc90u.dll`, `qoffscreen.dll` | PE-environment helpers | scripted-builder messaging; video preview on WinPE graphics stack | PE only — PE census §2 |

### 3.K Config/data files with decoded formats (full decode in `art/config-files/*`)

`config/version.ini` (channel/edition/product-code/qq secrets), `config/config.ini`
(22 languages, fonts, 1240×840, ENS buy-button coords), `config/themdark.ini`
(118-slot dual palette), `config/FilterFolder.ini` (junk blocklists, 10-locale New-Folder),
`config/RawConfig.ini` (37 carve switches), `config/usefullostfiles.xml` (27 "worth
recovering" ext), `config/PriorityDisplayType.xml`, `config/nasconfig.ini`,
`config/shellexe.ini`, `FileFilter.xml` (8 cats/1,489 ext), `raw_signs.json`,
`EUUserData`, `Dict`, `dlconfig.xml`/`PEtools.xml`, `AliyunConfig.ini`,
`google-services.json`, `remote_config_data`, `DataFile.ini`, `Upgrade.ini`,
`recoveryFree.ini`, `style.css` (20,802-line QSS), `DRWReport.ini` (report attachments),
`SaveScan/*.rsf` (**fully reversed**: zlib-chunked records, magic 0xEAF4B31B, record-4
tree grammar, CFileNode/CFileSystemNode, 1.99M-node validated — `art/SaveScan/*`),
`DRWCfg.bin` (runtime-written AES-256-CBC cache, hardcoded key — not present in tree).
`SaveScan/` sessions are per-machine runtime artifacts (0 .rsf on PE media).

### 3.L Runtimes (L1 static-import substrate)

Qt 5.15.2 Widgets (6 DLLs, 25.7 MB) + plugins (`platforms/` incl. PE-only qoffscreen,
`imageformats/`, `iconengines/`, `styles/`); OpenSSL ×4 generations (libeay32/ssleay32,
MD pair, 1.1-x64, 3-x64) — "any symbol-level analysis must attribute each consumer to
its specific libcrypto (EuRepairHost→1.1, cloud→3.x)"; VC runtimes ×4 generations
(VC90+manifests, VC120, VC140); UCRT self-carriage (ucrtbase + 40-49 api-ms-win
forwarders); zlib/zlib1/liblzma; libzmq-mt-4_3_5; GDIPLUS.
`art/runtime-families/*` (10 family docs, 218 binaries consumer-mapped); PE census §1/§5.

---

## 4. Per-scan vs. always-resident (what the corpus says)

| Module set | When loaded | Evidence |
|---|---|---|
| 10 `*SearchDLL.dll` | **always**: LoadLibraryW'd into DRWUI globals at startup (`FUN_14056c0d0`) | DRWUI §9.2 |
| FS searcher objects (`EUCreate*Searcher/QuickSearcher`) | **per scan**: quick = FS-code-indexed factory; advanced = full set + RAW | DRWUI §9.2 (`FUN_140577d40` / `FUN_140580720`) |
| RAWSearcher | **per advanced scan, gated** on settings flag (obj+0x60) | DRWUI §9.4 |
| MOM `.mo` domain modules | **lazy, on first use, singleton-cached** (`FUN_1800017f0("DeviceManager")` call sites); link-time kernel (Log/NTFSLib/…) resident with hosts | mom-module-system §1.1 |
| `eprdtdrv` service/driver | installed+started **on first scan needing raw access** | eprdtdrv-internals §7 |
| DRWService | **AUTO_START at every boot**; pushes key table when DRWUI's pipe server appears (≤10 s wait) | DRWService artifact §1/§3 |
| EuMrx | **demand** service; driver drop installed with product, started on demand | install.bat; arch/02 §5 |
| Fixo engines, viewers, media helper, firebasefetch | **on demand, out-of-process** | arch/06 §5; DRWUI §9.5 |
| PartitionRecovery + EuShare + NasConfig + License + SaveScan + EUFileRepair + VideoViewer + diskhealth | **never loaded in WinPE** (dirs absent from BUILDPE) | PE census §2-3 |

---

## 5. Gaps (NOT-IN-CORPUS)

- Exact import graph of DRWUI for every side DLL beyond the census (inventory import
  lists exist per binary; a full DRWUI→DLL closure is derivable from `inventory.json`
  but not authored as a doc).
- `EuMrxnp.dll` (Network Provider shim referenced by `ProviderPath` — not shipped).
- `Ext2SearchDLL.dll` — referenced by DRWUI LoadLibrary strings but **not shipped**
  (EXT4SearchDLL covers FS codes 4/5) — searchdll-family §1.
- Which component invokes `EuShare/install.bat` at runtime (and console visibility).
- Full decode of `auto.txt` (opaque GUID blob) and `lostpart.dat`/`partinfo.dat`.
- DiskHealth scheduled-task definitions (addtask.bat exists; resulting tasks not captured).
