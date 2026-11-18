# REFERENCE-FOOTPRINT — EaseUS Data Recovery Wizard (DRW): the installed footprint (consolidated session truth)

> **Task 2-b (Phase 0, Agent B) — Installer Integration & API Surface Expansion session of
> heicfast/final-prod.** READ-ONLY corpus research + this one sheet.
> **Corpus**: `/home/z/my-project/repos/easeus-drw-research` @ HEAD
> `be0cfe95ecf22ac67e0f3a823dd003163b196d38`.
> **Product**: `/home/z/my-project/repos/final-prod` @ HEAD `0c2c3c52` (= the code state of
> `8217bc26` plus one worklog-only commit; the brief's "HEAD 8217bc26" names the code-true
> commit — verified: `git merge-base --is-ancestor 8217bc26 HEAD`, only `worklog.md` +
> a session-prompt file differ).
> **Date**: 2026-11-18.

**IP BOUNDARY (binding, verbatim from the task's CRITICAL IP RULE).** The corpus directory
`og-codebase/` contains proprietary EaseUS binaries (an extracted EaseUS Data Recovery
Wizard installation). You may list its tree (file names and sizes via git ls-tree /
ls-files) and read the research markdown, but you MUST NOT open, decompile, extract strings
from, or otherwise analyze the CONTENT of any og-codebase binary. You must NEVER propose
copying any og file into any product. og files are read-only research artifacts; integration
means capability parity in the product's own original code — never file copying.

> This sheet **supersedes and consolidates** the product's prior footprint sheets —
> `docs/REFERENCE-FOOTPRINT.md` + `docs/REFERENCE-MODULES.md` (T4-C, corpus @ `03ad46c`),
> the archived `worklog/context-sheets/REFERENCE-LAYOUT.md` (D1) +
> `worklog/context-sheets/REFERENCE-INSTALLER.md` (D2), the T14
> `worklog/REFERENCE-FOOTPRINT.md` (this file's predecessor, 2026-11-17), and the parity
> rows of `docs/PARITY-DELTAS.md` — re-verified line-by-line at corpus HEAD `be0cfe95`.
> All counts/sizes below re-measured this session via
> `git -C easeus-drw-research ls-tree -r -l HEAD -- og-codebase` (names/sizes only; no
> og binary was opened). Roles cite the corpus research markdown only.
>
> **Errata folded in at be0cfe95** (these supersede the prior sheets' versions):
> 1. `FileFilter.xml` = 8 categories / **1,481** extensions (not 1,489) —
>    `re-workspace/artifacts/CORRECTIONS.md` #5 supersedes
>    `artifacts/config-files/config-data-reference.md`'s recount note. (The product's
>    generated table already carries 1,481 — `engine/src/core/query/cr_filefilter_data.cpp`
>    header — so this erratum confirms, not changes, the product.)
> 2. `config/RawConfig.ini` ships **34** toggles (not "37 types listed") — CORRECTIONS #5.
> 3. **`EUUserData`'s consumer is corrected**: it never reaches the RAW carving engine.
>    Sole consumer = DRWUI's `CHKRepairFileSign` (the CHK repair chain's file-signature
>    checker); `GetRawUserInfoMap` is the per-scan carve-statistics **map (output, not
>    input)** — CORRECTIONS #9, superseding `config-data-reference.md`'s "RAW signature
>    DB #2" row, `userdata-remoteconfig--signature-and-flags.md` §1's "user-extensible
>    carving" framing, and the rawsearch deep-dive's :61 area claim. (111-entry JSON shape
>    itself unchanged.)
> 4. `eprdtdrv.sys` = **27,728 B** (the T4-C sheet's "27,272" was a transcription error;
>    `eprdtdrv_win7older.sys` is the 27,272-byte one). BUILDPE `ebrntdrv.sys`/`epmdkdrv.sys`
>    = 27,272 each — git ls-tree at HEAD.
> 5. `BUILDPE/EaseUS-x64/` = 180 files / **171,915,664 B ≈ 164.0 MiB** (T4-C's "≈158 MB"
>    corrected; the T14 predecessor already re-measured it).

**Citation shorthand**: `arch/NN` = `architecture/NN-*.md`; `art/<name>` =
`re-workspace/artifacts/<name>`; all relative to the corpus root. Tree facts (names/sizes)
are from `git ls-tree -r -l HEAD -- og-codebase` unless otherwise noted.

---

## 0. Capture provenance (read first)

1. `og-codebase/` is a **captured post-install + post-run tree**, not a pristine installer
   payload: it carries runtime-mutated state — vendor QA logs (`log/DRWUI.log`,
   `CalcPartition.log`, `DefaultLog.txt`; root `EPMLOG.log`, `Eaolog.log`,
   `strutureReg.log`, `reglog_win.txt`), 3 saved scan sessions (`SaveScan/*.rsf`), a
   runtime-mutated `config/version.ini` (`InitActive`/`lastUpdateTs`/
   `forceshowcloseretain`), and a packaging list (`needcopy.txt`).
   `art/config-files/qa-logs--session-forensics.md` §1 ("the shipped tree contains the
   vendor's own QA night"); `art/WinPE/pe-payload-census--buildpe-easeus-x64.md` §4.
2. `BUILDPE/` is a **second, self-contained copy** of the product (the bootable WinPE
   "Emergency Disk" payload) — 194 files. Media content, not live-install files, but
   shipped on the install medium and documented here because it carries 4 of the 7 kernel
   drivers. `arch/03-winpe-boot-environment.md`.
3. Install root path (live Windows): `\Program Files\EaseUS\DRW` — literal UTF-16 strings
   in DRWUI (per `docs/REFERENCE-FOOTPRINT.md` §1, citing
   `re-workspace/reversed/DRWUI.exe/strings_utf16.txt` 15424–15615; binary-content
   evidence, not re-derived here). WinPE path: `x:\Program Files\easeus\DRW\DRW.exe`
   (`og-codebase/BUILDPE/x64/Windows/system32/winpeshl.ini`, byte-exact decode in
   `art/WinPE/boot-config--winpeshl-autobat-regdrv.md` §1).
4. NOT CAPTURED (do not invent): a live-machine registry diff, `C:\ProgramData` layout,
   Start-menu/desktop shortcut set on live Windows, the main Inno Setup EXE/script,
   driver-file post-state inside a real `System32\Drivers`. Only what the research
   markdown documents from the binaries is evidenced.

---

## 1. The complete installed tree (live Windows, `\Program Files\EaseUS\DRW\`)

### 1.1 Footprint totals (re-measured at be0cfe95; matches the T14 census 1:1)

| Area | Files | Bytes | MiB | Notes |
|---|---|---|---|---|
| **Whole captured tree** (`og-codebase/`) | **1,470** | 702,834,195 | 670.3 | 565 PE binaries (`re-workspace/inventory.json`; class split 1,244 easeus / 226 external) |
| **Live install tree** (excl. `BUILDPE/`) | **1,276** | 518,749,215 | 494.7 | what lands in `\Program Files\EaseUS\DRW` |
| — install root (flat) | 149 | 140,060,599 | 133.6 | 24 EXE + 99 DLL (incl. 40 `api-ms-*` at root) + 26 data/config/manifest/log |
| — 17 subdirectories | 1,127 | 378,688,616 | 361.1 | see §1.3 |
| **BUILDPE/** | **194** | 184,084,980 | 175.6 | media payload (§4) |
| — `BUILDPE/EaseUS-x64/` | 180 | 171,915,664 | 164.0 | app copy in WIM |
| — `BUILDPE/x64/` | 14 | 12,169,316 | 11.6 | PE OS overlay: 4 drivers + shell config + msi/oledlg shims + wallpaper |
| Runtime artifacts inside capture (not install-time) | 11 | 69,174,116 | 66.0 | `SaveScan/*.rsf` ×3 (65.9 MiB), `log/` ×4, 4 root QA logs |

Largest single files: `VideoViewer/avcodec-62.dll` 58.4 MB · `SaveScan/…(H_).rsf` 52.7 MB
(runtime artifact) · `ResourceDark/drw_dark.rcc` 32.3 MB · `Resource/drw_light.rcc`
30.7 MB · `DRWUI.exe` 30.6 MB · `VideoViewer/avfilter-11.dll` 21.2 MB ·
`EUFileRepair/EuOfficeRepairFile.exe` 10.5 MB.

### 1.2 File classes across the live tree (the T14 census, counts confirmed at HEAD)

| Class | Files | Notes |
|---|---|---|
| PE binaries (exe/dll/sys) | 535 (474 dll + 54 exe + 7 sys) | inventory + tree |
| Qt resource containers (.rcc) | 4 | drw_light / drw_dark / DiskHealth.rcc (+1 in PE copy) |
| Qt translations (.qm) | 37 live (+40 diskhealth +37 PE copy = 114) | `art/Languages/language-packs--family-census.md` |
| MOM modules (.mo) | 30 | all in `PartitionRecovery/` — `art/PartitionRecovery/mo-module-map.md` |
| QML/JS | ~612 | the entire diskhealth QML runtime |
| `api-ms-win-*` UCRT forwarders | 200 across the tree (40 at root) | private copies in VideoViewer/, EUFileRepair/, diskhealth/, BUILDPE — `art/runtime-families/ucrt-forwarders.md` |
| config/data (xml/json/ini/txt/dat/reg/bat) | ~140 | formats decoded in `art/config-files/*` |

### 1.3 Per-directory manifest — file class, role, citation

**Install root — product executables (24 EXE)**

| File | Size | Class → role | Citation |
|---|---|---|---|
| `DRW.exe` | 127,208 | loader/supervisor — "DRWLoader" crash-event watchdog, 2 MB shared log, DRWUI restart loop, DRWReport handoff; `requireAdministrator` manifest | `art/DRW-exe--launcher.md`; `docs/REFERENCE-FOOTPRINT.md` §6.2 (manifest, binary strings) |
| `DRWUI.exe` | 30,571,752 | the main Qt UI application ("the product"): wizard stack, scan orchestrator, results tree/classify/filter/preview, CRecoveryMgr export chain, repair wizards, licensing, telemetry; `requireAdministrator` | `art/DRWUI-exe--main-ui-architecture.md` |
| `DRWService.exe` | 40,168 | service binary — "Read FVE Server" BitLocker FVE key broker (§3) | `art/DRWService-exe--bitlocker-service.md` §1 |
| `DRWReport.exe` + `DRWReport.ini` | 4,805,352 + 515 | crash reporter (XCrashReport build; minidumps DRWDump0-5.DMP; .ini lists report attachments) | `art/DRWReport-exe--crash-reporter.md` |
| `SetupUE.exe` | 177,384 | installer bootstrap — 16-OS gate, `SOFTWARE\EASEUS\DRW\Version` read/write, owns `DRW_Installer` window (§3) | `art/Installer/setupue-infoforsetup--bootstrap-pair.md` §1 |
| `InfoForSetup.exe` | 117,776 | install-time telemetry collector (`/SendInfo`, Activity/Country/Timezone/TestId) | same §2 |
| `unins000.exe` / `unins000.dat` / `unins000.msg` | 1,259,408 / 513,373 / 22,791 | **Inno Setup uninstaller trio** (Inno 5.5.9 strings; 2016-era VS2008 binary) | same §1; per-binary/unins000.md; `arch/01` §2 |
| `EuDownloader.exe` | 1,381,608 | resumable range/MD5 downloader (WAIK ISO carve, agent zips) | `art/config-files/config-data-reference.md` (dlconfig.xml consumer) |
| `EuRepairHost.exe` | 23,784 | repair licensing bridge (activation.easeus.com[.cn]) — hosts Fixo engines out-of-process | `art/Licensing/licensing-activation-chain.md` |
| `EUImgRepair.exe`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`, `EuOfficeRepairWin32.exe`, `EuOfficeTools.exe`, `EUUnZip.exe`, `EURawImg.exe` | 203–2,739 KB each | Fixo repair chain — root copies/Win32 hosts of the repair engines (Win32/x86, out-of-process from the x64 UI) | `arch/06-repair-chain.md` §1 |
| `EUOfficeViewer.exe` / `PDFViewer.exe` / `EUHwpViewer.exe` | 3.09 / 2.97 / 0.42 MB | out-of-process preview viewers (Office / PDFium / Korean HWP) | `arch/04-recovery-workflow.md` §6 |
| `EUinApp.exe` | 37,608 | in-app purchase / store bridge — **orphaned** IE-emulation bootstrapper in this build | `arch/05-uiux-architecture.md` §11 (role correction) |
| `firebasefetch.exe` | 90,344 | Firebase remote-config fetch (out-of-proc) | `art/Cloud/cloud-recovery-architecture.md` §4 |
| `7za.exe` | 596,936 | standalone archiver (external, x86 console; setup-time ops) | `re-workspace/inventory.csv` row 1 (per prior sheet §3.1) |
| `AliyunWrapExe.exe`, `send_dmp_with_aliyun.exe` | 224,272 / 107,456 | Aliyun OSS upload / crash-dump uploader (live-tree only) | `art/DRWReport-exe--crash-reporter.md`; PE census §2 |

**Install root — the 9+1 scan engines (in-proc plugin DLLs; DRWUI `LoadLibraryW`s the set at startup)**

| File | Size | Engine | Citation |
|---|---|---|---|
| `NTFSSearchDLL.dll` | 577,256 | NTFS (MFT walk, deleted-bit scoring, $ATTRIBUTE_LIST fork) | `art/SearchDLL/ntfs-engine--deep-dive.md` |
| `FATSearchDLL.dll` | 469,224 | FAT12/16/32 | `art/SearchDLL/fat-engine--deep-dive.md` |
| `exFATSearchDLL.dll` | 413,416 | exFAT | `art/SearchDLL/exfat-engine--deep-dive.md` |
| `EXT4SearchDLL.dll` | 129,256 | ext2/3/4 (also `EUCreateEXT2QuickSearcher` for FS codes 4–5) | `art/SearchDLL/ext4-engine--deep-dive.md` |
| `HFSPlusSearchDLL.dll` | 185,064 | HFS+ (B-trees, journal) | `art/SearchDLL/hfsplus-engine--deep-dive.md` |
| `ReFSSearchDLL.dll` | 154,856 | ReFS (refs:: MSB) | `art/SearchDLL/refs-engine--deep-dive.md` |
| `BtrfsSearchDLL.dll` | 150,760 | Btrfs (superblock@0x10000, zlib/RC4) | `art/SearchDLL/btrfs-engine--deep-dive.md` |
| `ZfsSearchDLL.dll` | 275,176 | ZFS (uberblock/txg/DMU) | `art/SearchDLL/zfs-engine--deep-dive.md` |
| `RAWSearchDLL.dll` | 644,840 | RAW carving — 190+ analysers, u16 magic-key RB-tree registry, 70 magic keys | `art/SearchDLL/rawsearchdll--analyser-registry-census.md` |

The **"9+1"**: 9 SearchDLLs ship (8 per-FS + RAW carving); the 10th `LoadLibraryW` target
in DRWUI's engine loader is the legacy `Ext2SearchDLL.dll` — **referenced but not shipped**
(EXT4SearchDLL covers FS codes 4/5). `art/SearchDLL/searchdll-family--architecture.md` §1
("confirmed for all 9 + a legacy Ext2SearchDLL.dll reference (not shipped)");
`docs/REFERENCE-MODULES.md` §1/§5. Plugin contract: each exports
`EUCreate<FS>Searcher` + `EUCreate<FS>QuickSearcher` (RAW adds
`EUCreateRAWSearcher`/`EUGetAnalyserList/Map/Multimap`/`GetRawUserInfoMap`);
DRWUI loads the set into globals at startup (`art/DRWUI-exe--main-ui-architecture.md` §9.2).

**Install root — repair/media/preview support DLLs**
`EUVideoRepair.dll` 948,456 (in-proc video repair, 7,154 fns) · `EUImageEnhance.dll`
3,056,872 (OpenCV 2.4 static) · `heif.dll`/`de265.dll`/`libx265.dll` (HEIF/HEVC) ·
`pdfium.dll` 4,119,552 · `dbghelp.dll` (crash minidumps) · `libzmq-mt-4_3_5.dll` (preview
IPC) · `libssh2.dll` (NAS SSH-2.0-libssh2_1.11.0) · `liblzma.dll`/`zlib.dll`/`zlib1.dll` ·
`archive.dll` (libarchive, 1,353 fns) · `legacy.dll` (libtomcrypt) · `PubLog.dll` ·
`Japanese.dll` 4,206,312 (Win32 ja-JP resource satellite) · `GDIPLUS.DLL` (local GDI+
copy). Citations: `arch/06-repair-chain.md`; `art/DRWUI-exe--main-ui-architecture.md` §9.5;
`art/EuShare/eumrx-nas-architecture.md` §3.2; `per-binary/*` (roles per
`docs/REFERENCE-MODULES.md` §3.A/§3.F, verified unchanged at HEAD).

**Install root — cloud stack (live-tree only)**
`cloud_session.dll` 943,336 + `cpprest141_2_10.dll` 3,756,032 (OAuth2 cloud sessions,
C++ REST SDK) · `EuICloudRecovery.dll` 7,187,688 (full iCloud client, SRP-6a verified) ·
`firebaseonlineconfig.dll` 1,155,816 · `AliyunWrap.dll` 624,144 · `EuUserRate.dll`
1,619,176. `art/Cloud/cloud-recovery-architecture.md`; PE census §2 (live-only list).

**Install root — runtimes (L1 static-import substrate; consumer-mapped)**
Qt 5.15.2 Widgets (`Qt5Core/Gui/Network/Svg/Widgets/WinExtras.dll`, 25.7 MB) —
`art/runtime-families/qt5-core-stack.md`; build provenance `arch/01` §1.6 (official
Qt binaries, `C:/Users/qt/work/qt/...`). OpenSSL — **four generations in one tree**
(`libeay32/ssleay32` 0.9.8-lineage names; MD pair; `libssl/libcrypto-1_1[-x64]`; 3-x64) —
`arch/01` §1.6; PE census §1 ("THREE OpenSSL generations coexist" in PE; the MD pair is
live-only). VC runtimes ×4 generations (VC90 + SxS manifests / VC120 / VC140 +
ucrtbase + 40 root `api-ms-win-*` forwarders) — `art/runtime-families/vc-runtime-generations.md`,
`art/runtime-families/ucrt-forwarders.md`. Media codecs (heif/de265/libx265 + VideoViewer
ffmpeg set) — `art/runtime-families/media-codecs.md`.

**Install root — config / data files (formats decoded in the corpus)**

| File | Size | Content (decoded) | Citation |
|---|---|---|---|
| `FileFilter.xml` | 54,597 | result-category tree: 8 FileCategories / **1,481** `<Extention>` entries (CORRECTIONS #5) | `art/config-files/config-data-reference.md` + `art/CORRECTIONS.md` #5 |
| `raw_signs.json` | 35,342 | RAW signature DB: 139 ext, header+tail hex signs, confidence 0.50–1.00, min/max | `art/config-files/config-data-reference.md` ("Signature & magic databases") |
| `EUUserData` | 16,891 | 111-entry libmagic-style JSON `{ext: {signs, mime}}` — **consumer: DRWUI's CHKRepairFileSign (CHK repair chain), NOT the carving engine**; `GetRawUserInfoMap` = per-scan carve-statistics map | `art/CORRECTIONS.md` #9 (supersedes `userdata-remoteconfig--signature-and-flags.md` §1 framing) |
| `Dict` | 2,742 | 228-entry password brute-force dictionary (BitLocker password unlock, encrypted HWPX, protected archives) | `art/config-files/config-data-reference.md` (byte-verified) |
| `DataFile.ini` / `Upgrade.ini` / `recoveryFree.ini` | 84 / 235 / 128 | UTF-16 counters; upgrade pointer (20.8.0.0, `killDRWUI=1`); free-quota state | same; `og-codebase/Upgrade.ini` |
| `AliyunConfig.ini` | 1,802 | Alibaba OSS upload config | same |
| `google-services.json` | 997 | Firebase project config | same |
| `remote_config_data` | 62,579 | fetched Firebase remote-config snapshot (~600 flags, market/edition suffix segmentation) | `art/config-files/userdata-remoteconfig--signature-and-flags.md` §2 |
| `style.css` | 437,565 | QSS for the product UI (20,802 lines; 118-slot palette) | `arch/05-uiux-architecture.md` §1 |
| `dlconfig.xml` / `PEtools.xml` | 16,202 / 12,756 | WAIK ISO carve manifest (byte-ranges + MD5) / WAIK deployment manifest | `config-data-reference.md`; `arch/03` §4 |
| `needcopy.txt`, `auto.txt` | 1,670 / 316 | packaging copy-list / opaque GUID blob (auto.txt decode UNKNOWN) | `og-codebase/` raw; T14 sheet §7 |
| `EPMLOG.log`, `Eaolog.log`, `strutureReg.log`, `reglog_win.txt`, `icloud_logger` | 15,269 / 336 / 405 / 133 / 88 | vendor registration/installer + QA session logs | `art/config-files/vendor-logs--qa-session-forensics.md` |

**`config/` — 9 files, 13,414 B** — `version.ini` (build identity: 21.0.0.0, edition
`free`, channel `easeus`, market `abroad`, `product-code=RE-DRW-FREE-WIN`,
`SetupBuildTime=1013`; runtime-mutated on the live copy) · `config.ini` (22 languages,
window 1240×840, ENS buy-button coords) · `themdark.ini` (118-slot dual palette) ·
`FilterFolder.ini` (junk blocklists, 10-locale New-Folder, TxtViewFiles,
SumaryFileType) · `RawConfig.ini` (**34** carve toggles — CORRECTIONS #5) ·
`usefullostfiles.xml` (27 "worth recovering" ext) · `PriorityDisplayType.xml` ·
`nasconfig.ini` · `shellexe.ini` (25-language shell context-menu strings).
`art/config-files/config-data-reference.md` ("Filtering & categorization") +
`art/config-files/filtering-priority--config-layer.md` (the 5-file filtering pipeline).

**`language/` — 37 .qm, 6,386,275 B** — 25 product locales + 4 Mainland OEM sub-brands
(`Mainland{,_Foxit,_JZ,_LC}` — independently built, Foxit channel NAS/BitLocker-forward)
+ 8 qtbase Qt-internal catalogs (`qtbase_en.qm` = 16-byte header-only). Selection:
`software.reg` seeds `HKLM\SOFTWARE\EASEUS\DRW\Language=English` in PE; DRWUI
QTranslator::load + `version.ini` SelLanguage persists.
`art/Languages/language-packs--family-census.md` §1–§3.

**`Resource/` + `ResourceDark/` — 2 + 2 files, 63.2 MB** — theme RCC bundles
`drw_light.rcc` 30,742,205 / `drw_dark.rcc` 32,290,147 + `ImageResource.xml` 38,706 each
(~7,400 assets). `arch/05-uiux-architecture.md` §1 (per prior sheet §3.6).

**Qt plugin dirs** — `platforms/` (qwindows 1,505,400 / qminimal 872,568 / qoffscreen
780,408), `imageformats/` (qgif/qjpeg/qsvg), `iconengines/` (qsvgicon),
`styles/` (qwindowsvistastyle). PE census §1.

**`PartitionRecovery/` — 57 files, 28,158,618 B (26.85 MiB)** — the lost-partition stack:
**30 `.mo` MOM modules** (PE DLLs with a custom extension; in-house COM — name→.mo→
`LoadLibraryA`→`GetModule`/`CreateObject`; link-time kernel: Log/CallbackOperator/
DeviceAdapter/Fixup/NTFSLib(310 exports); lazy factory plugins: Device, DeviceManager,
Partition, LdmManager, PartitionRecoveryD, FileSystem*Analyser family, NTFSUtil,
FatLib/FatFormat, RegLib, CheckVersion, bitlockerhelp, EFIBoot, Mom…) —
`art/PartitionRecovery/mom-module-system--architecture.md`,
`art/PartitionRecovery/mo-module-map.md`; **19 host/support DLLs**
(PartitionRecovery.dll 892,648 — engine host; MainModule.dll 1,826,536 — EPM-shared
partition-editing engine; RecoveryModule 1,348,840; ToolsModule 2,578,664; UILogic
1,448,168; CloneModule 2,260,712; RawFixer 706,792 — transactional FS repair;
BitLockerLib 722,152; Boot/BootDriver/Win32Bcd 261,864; Burn 165,608 — IMAPI2;
AutoUpdate 158,440; Logger 87,784; peformat 50,920; GetDriverInfo 50,920 — drive-letter
topology (naming trap); libvmdk 1,108,712 + zlib ×2 + private Qt5Core 6,168,808) —
`arch/01` §1.3, `docs/REFERENCE-MODULES.md` §3.D; **`WinChkdsk.exe`** 72,936 (chkdsk
wrapper w/ VSS); **driver payloads + deployer**: `eprdtdrv.sys` 27,728 +
`eprdtdrv_win7older.sys` 27,272 + `setupdrvx64.exe` 174,024 + `setupdrv.ini` 10;
data `lostpart.dat` 1,096 / `partinfo.dat` 24.

**`EuShare/` — 3 files, 218,602 B** — `EuMrx.sys` 215,704 (static-RDBSS NAS mini-redirector,
`\Device\EuRdr`) + `install.bat`/`uninstall.bat` (SCM service + Network Provider
registration — §3). `art/EuShare/eumrx-nas-architecture.md` §2.6.

**`NasConfig/` — 2 files** — `NasModel.xml` 6,138 (Synology model→arch catalog) +
`ssh_config.ini` 92 (agent push state, `PushPath=/tmp/EaseUS/`). Same §3.3.

**`License/` — 19 files, 104,925 B** — third-party license compliance set (Qt, OpenSSL,
pdfium 62 KB, ImageMagick-family libraw/opencv, libzmq 16 KB…).
`art/config-files/config-data-reference.md` ("Install/misc").

**`EUFileRepair/` — 87 files, 47,843,550 B (45.63 MiB)** — the Fixo engine suite +
ImageMagick: `EUImgRepairFile.exe` 5,226,728 (image repair, 29 C<Format>Info classes) ·
`EuOfficeRepairFile.exe` 10,537,704 (Office/iWork-HWPX) · `EuPDFRepairFile.exe`
1,896,680 (PDF xref synthesis) · `EUArchiveRepair.exe` 599,128 + `7z.dll` 1,346,560 ·
`EUAdobeRepair.exe` 431,192 · 24× `CORE_RL_*` ImageMagick + 7× `IM_MOD_RL_*` delegates ·
own UCRT set (44 shims) + openssl-3 pair + libcurl/libde265/heif.
`arch/06-repair-chain.md`; `art/FileRepair/*` (six deep-dives);
`art/runtime-families/imagemagick-modules.md`.

**`VideoViewer/` — 76 files, 104,424,473 B (99.59 MiB)** — `EaseUS_VideoViewer.exe`
121,064 (out-of-process player; DRWUI passes the preview port on the cmdline; TCP
127.0.0.1:20171/20172 + zmq REP channel) + `EaseUS_MediaGraph.dll` 99,560 (decode graph)
+ `VideoFileRepairer.exe` 1,919,208 + `ffmpeg.exe` 532,494 + ffmpeg-6x DLL set
(`avcodec-62` 58,423,310, `avfilter-11` 21,193,742, `avformat-62` 13,447,182,
`avutil-60`, `swscale-9`, `swresample-6`, `avdevice-62`, `postproc-58`) + UI PNG/cur
assets + own CRT set + `sfc` 211,614 (magic " cfs-…"; purpose UNKNOWN).
`per-binary/{EaseUS_VideoViewer,VideoFileRepairer,EaseUS_MediaGraph}.md`;
`arch/01` §1.1; `docs/REFERENCE-MODULES.md` §3.F.

**`diskhealth/` — 818 files, 55,371,700 B (52.81 MiB)** — `DiskHealth.exe` 1,781,480 (Qt
Quick/QML SMART companion, CrystalDiskInfo lineage: CAtaSmart multi-transport SAT/CSMI/
NVMe-Realtek, USB bridge tables) + `DiskHealth.rcc` 444,492 + full QML plugin tree
(~612 QtQuick/Controls.2 files — Fusion/Imagine/Material/Universal, GraphicalEffects…)
+ own Qt5 stack (12 DLLs) + mfc140u + own UCRT + `translations/` (40 .qm) +
`addtask/querytask/deltask.bat` (schtasks-style task management; the resulting task set
NOT-IN-CORPUS) + AliyunConfig.ini stub. `art/DiskHealth/diskhealth--smart-companion.md`;
94% of the files exist because it carries its own complete Qt/QML runtime.

**`SaveScan/` — 3 files, 69,107,396 B** — `auto_Local Disk (G_/H_/I_)*.rsf` saved scan
sessions (runtime artifacts of the vendor QA night, not install-time). The .rsf format
is FULLY REVERSED in the corpus: zlib-chunked records, magic 0xEAF4B31B, record-1/2/3
schemas + record-4 tree grammar (CFileNode/CFileSystemNode, 34-byte base grammar,
IFileProperties content family), 1.99M nodes validated; filename sanitizer `/ * : ? " < > |`→`_`.
`art/SaveScan/rsf-format--scan-session.md` + `rsf-record4--tree-node-grammar.md`.

**`log/` — 4 files, 113,973 B** — runtime logs (DRWUI.log 41,646 / CalcPartition.log
31,592 / DefaultLog.txt 40,735 / empty DRWLog.log) — the writable-data-inside-install-dir
posture's evidence. `art/config-files/qa-logs--session-forensics.md`.

**Files placed OUTSIDE the install directory (live Windows)** —
`%SystemRoot%\System32\Drivers\eprdtdrv.sys` (+ versioned `%s%d.sys` variants) by
`setupdrvx64.exe` (`art/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §7;
`per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3) ·
`%SystemRoot%\System32\DRIVERS\EuMrx.sys` by `EuShare/install.bat` (byte-exact script
file) · `%AppData%\EaseUS\DRW\connect.zip` + extracted NAS agent (runtime download via
d1.easeus.com, extracted by EUUnZip.exe — `art/EuShare/eumrx-nas-architecture.md` §3.3) ·
`<install dir>\DRWCfg.bin` AES-256-CBC config cache written at runtime by DRWUI
(hardcoded key/IV — `art/config-files/drwcfg-bin--aes-config-cache.md`; NOT present in
the captured tree) · sibling-install references `C:\Program Files\EaseUS\BL`,
`…\BootRepair`, `…\CSBackup.exe` (strings evidence; presence on disk NOT CAPTURED).
NOT CAPTURED: `C:\ProgramData` layout, Start Menu group, scheduled tasks, the Inno
`Uninstall` key for DRW itself, license-data registry layout.

---

## 2. Module taxonomy (reference class → role)

### 2.1 Processes & plugin families

| Class | Module | Role | Citation |
|---|---|---|---|
| **Loader exe** | `DRW.exe` (DRWLoader) | entry point; requireAdministrator; spawns DRWUI with inherited elevation; crash-restart supervision; optional language DLL loads (Chinese/Deutsch/Japanese.dll per registry Language) | `art/DRW-exe--launcher.md` §1 |
| **UI exe** | `DRWUI.exe` | Qt5 main app; hosts ALL engines in-proc; 1,073 RTTI classes, 126 windows, 44 threads; embeds the DRWIntelligentScan source tree | `art/DRWUI-exe--main-ui-architecture.md` §9.1 |
| **Service exe** | `DRWService.exe` | "Read FVE Server" — LocalSystem AUTO_START; pipe CLIENT to `\\.\Pipe\DRWBitlocker` served by DRWUI/BitLockerLib; ≤100 records, 0x138 stride (50-char GUID + 256 B key blob), one-shot push | `art/DRWService-exe--bitlocker-service.md` §1/§3 |
| **9+1 engines** | 8 per-FS SearchDLLs + RAWSearchDLL (+ phantom Ext2SearchDLL) | the "DRWIntelligentScan" plugin family; EUCreate* factory contract; RAW = 190+ analysers (BruteForce/RawSpeed/xjq/Brance/File-gen/special families) | `art/SearchDLL/searchdll-family--architecture.md` §1/§3/§4 |
| **Partition engine (MOM)** | `PartitionRecovery.dll` + 30 `.mo` | lost-partition substrate, EPM-shared; name→.mo→LoadLibraryA plugin loading; engine→UI bridge via CMessageProxy singleton | `art/PartitionRecovery/mom-module-system--architecture.md`; `art/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md` |
| **Repair tools (EU\*/Fixo)** | root exes + `EUFileRepair/` + `EUVideoRepair.dll` + EuRepairHost | two repair worlds: DRW video (in-proc) vs Fixo (out-of-proc x86 CLI commandParser children); 21 RAW image formats, HWPX+OpenSSL, 7zNextHeader, PDF xref synthesis | `arch/06-repair-chain.md`; `art/FileRepair/repair-architecture.md` |
| **Companion apps** | `DiskHealth.exe` (SMART), `PDFViewer.exe`, `EUOfficeViewer.exe`, `EUHwpViewer.exe`, `EaseUS_VideoViewer.exe`, `WinChkdsk.exe`, `7za.exe`, `EUUnZip.exe`, `EURawImg.exe` | separate-process surfaces for health/preview/archive/raw-image | §1.3 rows + `docs/REFERENCE-MODULES.md` §3.F/§3.J |
| **Cloud modules** | `cloud_session.dll`, `EuICloudRecovery.dll`, firebase pair | OAuth2 loopback (Dropbox/GDrive/OneDrive); full iCloud client (SRP-6a); Firebase remote config | `art/Cloud/cloud-recovery-architecture.md` |
| **NAS modules** | `EuMrx.sys` + `libssh2.dll` + `NasConfig/` + `CEuShare` (in DRWUI) | static-RDBSS redirector, EuRdrDN session IOCTL protocol, SSH agent push, Synology model DB, loopback bridge | `art/EuShare/eumrx-nas-architecture.md` |
| **Kernel drivers (7)** | see §2.2 | raw disk access + device-stack hooks | `arch/02-kernel-driver-architecture.md` |
| **Installer-side** | SetupUE + InfoForSetup + unins000 trio + `7za.exe` | §3 | `art/Installer/setupue-infoforsetup--bootstrap-pair.md` |
| **Telemetry/monetization** | Aliyun channel, firebasefetch, EuUserRate, EUinApp, DRWReport | crash-dump upload, A/B flags, rate prompt, (orphaned) store bridge | `art/DRWReport-exe--crash-reporter.md`; `art/Licensing/licensing-activation-chain.md` |

### 2.2 The 7 kernel drivers (sizes from git ls-tree at HEAD)

| Driver | Size (B) | Device/namespace | Role | Deployment | Citation |
|---|---|---|---|---|---|
| `eprdtdrv.sys` | **27,728** | `\Device\EPMNTDRV`, `\\.\EPRDTDRV\d\p` | EPMNtDrv raw-disk proxy: MDL async sector IO, 3 IOCTLs + passthrough | live, via setupdrvx64 (demand) | `art/PartitionRecovery/eprdtdrv-sys--driver-internals.md` |
| `eprdtdrv_win7older.sys` | 27,272 | same | same source, legacy WDK ABI build | shipped in tree, same setup | `arch/02` §1–2 |
| `ebrntdrv.sys` | 27,272 | same | EPMNtDrv renamed for WinPE | BUILDPE system32, demand | `arch/03` §3 |
| `epmdkdrv.sys` | 27,272 | same | EPMNtDrv renamed for WinPE | BUILDPE system32, demand | `arch/03` §3 |
| `EUDCPEPM.sys` | 72,144 | dynamic `\Device\%s` ("epmdiskcopydrv") | device-stack HOOK: all-28-slots hook + PnP AddDevice, 15-cmd IOCTL protocol; disk-class UpperFilter in PE | BUILDPE drivers/, BOOT_START | `art/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` |
| `EUEDKEPM.sys` | 19,408 | `\Device\EUDKEPM` | named raw-disk access door (PID-token gated); dormant | BUILDPE drivers/, SYSTEM_START | `arch/02` §7 |
| `EuMrx.sys` | 215,704 | `\Device\EuRdr` + `\\.\EuRdrDN` | RDBSS network redirector + Network Provider "EaseUS Network" | live via EuShare/install.bat | `art/EuShare/eumrx-nas-architecture.md` |

"SAME family, THREE compilations" for eprdtdrv/ebrntdrv/epmdkdrv —
`art/PartitionRecovery/epmntdrv-trio--byte-diff-verification.md`. All WDK
`fre_wlh_amd64` builds; drivers shared with EaseUS Partition Master (`_epm_main_` PDB
paths — `arch/01` §1.2).

### 2.3 Loading mechanisms (five patterns)

L1 static-import (runtimes) · L2 `LoadLibraryW`+factory (the engine plugin contract) ·
L3 MOM plugin loading (name→.mo→GetModule; 26 factory plugins vs 5 link-time libs) ·
L4 out-of-process CreateProcess (repair/viewers/firebasefetch/DRWReport) · L5 SCM
services/kernel drivers. `docs/REFERENCE-MODULES.md` §2 (patterns verified unchanged at
HEAD — the artifacts it cites are all pre-be0cfe95 and untouched by the errata).

---

## 3. Install / uninstall / update behavior (as documented)

### 3.1 Installer technology & bootstrap chain

- **Inno Setup 5.5.x** (NOT NSIS/MSI): the `unins000.exe`+`.dat`+`.msg` trio is Inno's
  signature; the binary self-identifies "Inno Setup version 5.5.9 (u)", "JR.Inno.Setup"
  manifest. 2016-era VS2008 uninstaller — a frozen installer-generation constant vs the
  2026 main build. `worklog/context-sheets/REFERENCE-INSTALLER.md` §1 (verbatim string
  cites); `arch/01` §2; `art/Installer/setupue-infoforsetup--bootstrap-pair.md` §1.
  The main Inno setup EXE/script is NOT-IN-CORPUS.
- **SetupUE.exe = pre-install bootstrap** (x86, 24,545-line decompile): 16-OS version
  gate ("Microsoft Windows 2000"…"Server 2016", editions incl. Itanium/x64) via
  `GetNativeSystemInfo`+`RtlGetNtVersionNumbers` (WOW64-aware); writes/reads
  `SOFTWARE\EASEUS\DRW\Version`; spawns `InfoForSetup.exe` with the telemetry verb
  `/SendInfo "Window" "Install" "Activity" "Info_Userinfo" "Attribute" "<JSON>" "vid"
  "<Testid>"` (Country/Timezone/OS getters; A/B Testid read from the Version key); owns
  the `DRW_Installer` window (EuSendMessage.exe's WM_COPYDATA cmd-999 target in PE).
  `art/Installer/setupue-infoforsetup--bootstrap-pair.md` §1–§3.
- **Install-time side effects beyond file copy**: `unins000.*` written;
  `SetupBuildTime=1013` + `xurlPageID` stamped into `config/version.ini` by setup;
  `needcopy.txt` (the copy-list: InfoForSetup + Aliyun pair + full UCRT/VCRT set) shows
  Inno `[Files]`-style staging; registration logs recorded install events; **no
  scheduled-task creation, no driver-store/DIFx/pnputil usage is evidenced anywhere** —
  driver deployment is strictly SCM-service + file copy. Prior sheet §6.3 (citations
  verified).
- **Elevation model**: DRW.exe and DRWUI.exe both carry `requireAdministrator` manifests
  (byte-exact from .rsrc); DRWService `asInvoker` (SCM/LocalSystem); SetupUE/unins000
  `asInvoker`. One UAC per launch, then every privileged operation in-process with
  native Win32 APIs. The only documented PowerShell anywhere is BitLockerLib's
  runtime-unlock fallback (`manage-bde`/`Unlock-BitLocker`) — never the installer's
  mechanism. Prior sheet §6.2 (manifest string cites).

### 3.2 Service registration

- **"Read FVE Server"** (DRWService.exe): installed by DRWUI via
  `<install>\DRWService.exe /install` → `OpenSCManagerA(SC_MANAGER_ALL_ACCESS)` →
  `CreateServiceA(SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, LocalSystem)` →
  `StartServiceA` (decompiled); `/uninstall` → stop+DeleteService.
  `art/DRWService-exe--bitlocker-service.md` §1.
- **`eprdtdrv`** (service name = file base): `setupdrvx64.exe` copies to
  `\System32\Drivers\%s.sys` (+ `%s%d.sys` versioned variant), `CreateServiceA` group
  "Boot File System", `StartServiceA`, registry write+verify under
  `SYSTEM\CurrentControlSet\Services\%s`; `-Uninstall` mode; `setupdrv.ini` carries
  `[eprdtdrv]`. Installed **on first scan needing raw access** —
  `art/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §7 ("setupdrvx64.exe
  installs+starts eprdtdrv service on first scan needing raw access");
  `per-binary/deepening-batch1…` §3.
- **`EuMrx`**: `EuShare/install.bat` (byte-exact): `sc create EuMrx binPath=
  "system32\DRIVERS\EuMrx.sys" type= filesys start= demand group= Network`; registers
  Windows Network Provider "EaseUS Network" (`\Device\EuRdr`, ProviderPath
  `System32\EuMrxnp.dll` — referenced but NOT shipped, "driver-only drop") and prepends
  `EuMrx` to `NetworkProvider\Order\ProviderOrder`; uninstall mirrors.
  `og-codebase/EuShare/install.bat`; `art/EuShare/eumrx-nas-architecture.md` §2.6.
- Registry keys the corpus evidences: `SOFTWARE\EASEUS\DRW\Version` (write+read),
  `SOFTWARE\EASEUS\DRW\Language` (DRWLoader read → language DLL fallback),
  `SOFTWARE\EaseUS\DRW\Retention` (uninstall-retention A/B), sibling detection keys
  (`Fixo`, `ChronoSnap`, `DiskHealth`), `EUImgRepair` invocation chain,
  `Uninstall` enumeration, FVEAutoUnlock (service), Services\%s (drivers),
  NetworkProvider order. Prior sheet §5 (citations verified).

### 3.3 Uninstall behavior

- Inno `unins000.exe` drives removal; DRWUI strings show the instrumented uninstall
  funnel (`Uninstall_Retain`, `value_uninstallretain172`, `UninstallDlgRetentionTrigger`).
- `Upgrade.ini` (`killDRWUI=1`) documents that upgrades kill a running DRWUI before
  replacing files.
- Driver/service cleanup: DRWService `/uninstall`; setupdrvx64 `-Uninstall`;
  `EuShare/uninstall.bat` (sc stop/delete + ProviderOrder cleanup).
  Prior sheet §6.4 (citations verified).

### 3.4 Update behavior — the CVersionCtrl chain

- `CVersionCtrl` is a DRWUI service singleton **woven into the scan path**
  (`scanClickedSlot` vtable+0x1f0/0x1f8 pre-scan prep, +0x220 scan-mode notify) —
  `art/UIUX/13-ui-core-wiring-state.md`; `arch/05-uiux-architecture.md` §3 ("Service
  singletons: CVersionCtrl (licensing woven into the scan path…)").
- Background: `CheckUpdateThread` (`CVersionCtrl::doCheckUpdate`) + Firebase remote-config
  fetch at startup ("Try to initialize Remote Config…", falls back to the shipped
  `remote_config_data` snapshot) — `arch/04-recovery-workflow.md` §1 (LAUNCH background:
  `CheckUpdateThread` (CVersionCtrl::doCheckUpdate), firebase remote config fetch);
  `art/config-files/userdata-remoteconfig--signature-and-flags.md` §2.
- Download/update payloads: `EuDownloader.exe` (range/MD5), `Upgrade.ini`
  (Product Version 20.8.0.0, download URL, killDRWUI), `DataFile.ini` (update-staging
  state), `PartitionRecovery/AutoUpdate.dll` (version-check module consumed by
  CheckUpdateThread). `docs/REFERENCE-MODULES.md` §3.G (citations verified).

### 3.5 Writable-data-inside-install-dir posture

The reference writes RUNTIME state INTO `C:\Program Files\EaseUS\DRW\` (an
admin-writable install dir): `log/` (4 runtime log files) · `SaveScan/*.rsf` sessions ·
`DRWCfg.bin` (AES-256-CBC config cache, hardcoded key, runtime-written — not present in
the captured tree) · `config/version.ini` mutated at runtime (`InitActive`,
`lastUpdateTs`, `forceshowcloseretain`; the PE copy is pristine build output).
`art/config-files/qa-logs--session-forensics.md`; PE census §4; DRWCfg artifact §2.
(The product's deliberate divergence D-I2: ProgramData + per-user state.)

---

## 4. The WinPE / BUILDPE environment (what the boot tree adds beyond the installed product)

`BUILDPE/EaseUS-x64/` — **180 files, 171,915,664 B**: the full second app build in the
WIM: product core (DRW/DRWUI/DRWService/DRWReport), all 9 SearchDLLs, the repair line,
viewers, cloud pair, Qt runtime + plugins, three VC generations + self-carried UCRT
(49 forwarders + ucrtbase — WinPE has no Universal CRT), three OpenSSL generations,
media codecs, config set, 37 .qm language files.
**PE-only binaries (6)**: `EuSendMessage.exe` (WM_COPYDATA cmd-999 messenger to the
`DRW_Installer` window — scripted-builder messaging), `d3d8thk/d3d9/d3d11/dxgi.dll` D3D
shims (Qt video preview on WinPE's reduced graphics stack), `mfc90u.dll`.
**Live-only (15 + 8 directories)**: setup trio, 7za, Aliyun channel, cloud_session +
cpprest, EuUserRate, MD-CRT OpenSSL pair, msvcm90; and whole dirs `PartitionRecovery/`
(zero .mo anywhere under BUILDPE — find-verified), `EuShare/`, `NasConfig/`, `License/`,
`SaveScan/` (zero .rsf on media), `EUFileRepair/`, `VideoViewer/`, `diskhealth/`.
**Consequences** (verified): the bootable edition CANNOT do lost-partition recovery (no
MOM modules), has no NAS redirector (network = PENetwork SMB / runtime-downloaded
connect.exe), ships no licensing blobs → FREE edition by construction
(`version=free`, `product-code=RE-DRW-FREE-WIN`); diskhealth deploys from the boot USB's
`Tools\` tree at runtime (auto.bat Phase E) keeping the WIM lean.
`art/WinPE/pe-payload-census--buildpe-easeus-x64.md` §1–§3.

`BUILDPE/x64/` — the PE OS overlay (14 files, 12,169,316 B): `winpeshl.ini` (launch
list: wpeinit → ChangeRes setmax → launch.exe 0 auto.bat → `DRW.exe` → keep-alive;
**DRW is launched by the shell, not by auto.bat** — the product starts even if shell
setup fails) · `auto.bat` (6-phase pseudo-desktop builder: ~30 .reg imports, MMC snap-in
regsvr32, shortcuts, OEM Mainland branch via `HKLM\SOFTWARE\EASEUS\EPM\Version\OEM`,
GetDriver USB detection + Chrome-as-viewer kiosk + DiskHealth/DiskMark deployment, PENetwork
+ `explorer.exe -winpe`) · `regdrv.reg` (driver services: epmdkdrv/ebrntdrv demand,
EUDCPEPM **BOOT_START** + `UpperFilters=EUDCPEPM` on GUID_DEVCLASS_DISKDRIVE — class
upper filter on every disk; EUEDKEPM SYSTEM_START) · `software.reg` (Language=English
seed) · `Unattend.xml` (1024×768 display floor) · `wallpaper.bmp` ·
`Windows/system32/{ebrntdrv,epmdkdrv,msi,oledlg}.dll` + `winpeshl.ini` +
`SysWOW64/{msi,oledlg}` + `drivers/{EUDCPEPM,EUEDKEPM}.sys`. Note: `eprdtdrv.sys` (the
live-Windows on-demand driver) is ABSENT from the PE image. `arch/03` §1–§3;
`art/WinPE/boot-config--winpeshl-autobat-regdrv.md` §1–§3.

The bootable-media **builder** lives in the live product: `PE_Tools.dll` (51 RTTI
classes) + `PEtools.xml` + `dlconfig.xml` + `EuDownloader.exe` — download WAIK supplement
ISO (byte-range carve + MD5), stage the WAIK tree, DISM-mount/customize WINPE.WIM,
inject the DRW tree + drivers + auto.bat, write ISO/USB via Burn.dll (IMAPI2); PE3/4/5/
Win10PE variants. `arch/03` §4.

---

## 5. Parity mapping — reference module class → the product's own component TODAY

Status marks: **ALREADY-INTEGRATED** (shipped, capability-parity), **PARTIAL** (exists,
gaps recorded), **ABSENT** (not shipped — each row names whether it is a recorded
deliberate cut or an open gap). Product paths relative to `final-prod/`. Product state
per `docs/PARITY-DELTAS.md`, `worklog/FOOTPRINT-DELTA.md`, `docs/INSTALL-FOOTPRINT-CURRENT.md`
(§A.1 + 2026-11-05 correction), and direct file verification this session.

| # | Reference module class (corpus cite) | Product equivalent TODAY (path) | Status |
|---|---|---|---|
| 1 | DRW.exe loader/supervisor + DRWUI.exe UI host (`art/DRW-exe--launcher.md`) | ONE exe `ui/src-tauri/` → `recovery-wizard-ui.exe` with `requireAdministrator` manifest (`ui/src-tauri/app.manifest`, since 0.9.0); no supervisor restart loop | **PARTIAL** (deliberate D-P1: one-process product; manifest parity real since 0.9.0 — D-I3 note) |
| 2 | 9+1 SearchDLL engines (per-FS plugin family) | one engine `cr_ui.dll`: per-FS modules `engine/src/core/{ntfs,fat,exfat,ext4,hfsplus,refs,btrfs,zfs,carve}/` | **ALREADY-INTEGRATED** (consolidated; per-FS parity tracked in engine AUDIT docs + PARITY-DELTAS) |
| 3 | `raw_signs.json` (139-ext signature DB) | `engine/src/core/carve/{cr_rawsigns.cpp,cr_rawsigns_data.cpp}` | **ALREADY-INTEGRATED** |
| 4 | `EUUserData` (111-entry ext-signature JSON; og consumer = CHKRepairFileSign per CORRECTIONS #9) | `engine/data/userdata-defaults.json` + `engine/src/core/carve/cr_usersigns.cpp` (rrecov `--user-signs`, M-048, EUUserData JSON shape) | **ALREADY-INTEGRATED** (shape parity; the og's CHK-repair consumer role recorded — the product uses the shape for its own user-signature surface) |
| 5 | `FileFilter.xml` (8 cats / 1,481 ext) | `engine/src/core/query/cr_filefilter_data.cpp` (generated by `engine/scripts/gen_filefilter.py`; header: "8 categories / 1,481 unique extensions") | **ALREADY-INTEGRATED** |
| 6 | `config/FilterFolder.ini` (junk blocklists) | `engine/src/core/orchestrator/cr_filterfolder_data.cpp` + `engine/scripts/gen_filterfolder.py` (rrecov `--filter-folder`, M-073) | **ALREADY-INTEGRATED** |
| 7 | `config/RawConfig.ini` (34 carve toggles) | the carve registry's token-gated probes (`engine/src/core/carve/cr_registry.cpp` — "RawConfig MOBI/HIK/JP2…=1 shipped" per-site) | **ALREADY-INTEGRATED** |
| 8 | `Dict` (228-entry password dictionary) | `engine/src/core/recovery/{cr_dict.cpp,cr_dict_data.cpp}` (N-064) | **ALREADY-INTEGRATED** |
| 9 | `.rsf` saved-scan sessions (`art/SaveScan/rsf-format…`) | `.crsf` (CRSFMT01) sessions: `engine/src/core/session/{cr_crsf.cpp,cr_records.cpp,cr_lastscan.cpp}`; %ProgramData%\...\sessions (D-I9; auto-naming N-061) | **ALREADY-INTEGRATED** (format IS the product's own; UTC auto-name D-B2 recorded) |
| 10 | PartitionRecovery MOM system (lost partitions; `mo-module-map.md`) | `engine/src/core/{table,lost,apply}/` (cr_table_parse, cr_ldm, cr_candidates, cr_consistency, cr_apply) + `rrecov apply` CLI | **PARTIAL** (discovery + engine + CLI complete; the in-UI partition-apply flow is the recorded follow-up S-029) |
| 11 | `RawFixer.dll` (transactional FS repair) + pre-scan repair consent | `engine/src/core/repair/cr_rawfixer.cpp` + `engine/src/core/check/{cr_fscheck.cpp,cr_fsck_launch.cpp}`; consent flow shipped (D-B5); `rrecov` undo (D-B4) | **ALREADY-INTEGRATED** (volume-repair scope; the FILE-repair workbench is row 14) |
| 12 | `Win32Bcd.dll`/`Boot.dll`/`EFIBoot.mo` (BCD/EFI boot surgery) | `engine/src/core/{bcdfix,nvramfix}/` (BCD partition-offset rewriter M-040; NVRAM/UEFI fix) | **ALREADY-INTEGRATED** (engine-side; scope per engine docs) |
| 13 | `DRWService.exe` + `BitLockerLib.dll` (FVE broker + BitLocker impl) | in-process: `engine/src/win/cr_bitlocker.cpp` + `engine/src/core/health/cr_fve_policy.cpp` (+ cr_decrypt_device in storage) — no service (D-I5) | **ALREADY-INTEGRATED** (deliberate divergence: in-process, no SCM broker) |
| 14 | Fixo file-repair workbench (`EUFileRepair/` 87 files + root repair exes + EuRepairHost + EUVideoRepair.dll) | shipped today: the pre-scan repair consent flow (S-008/D-B5) + `cr_ui_fsck_volume` transactional volume repair (`engine/include/cr/cr_ui.h:1677`) + `rrecov fix --apply/--undo` CLI (D-B4); the file-repair workbench itself (`cr_ui_repair_analyze/apply/undo` ABI trio) is the recorded follow-up — Repair page truthfully states unavailability | **ABSENT** (open gap G-1(b) — deferred with precise scope; FOOTPRINT-DELTA §1 row 5) |
| 15 | Preview/viewer stack (PDFViewer+pdfium, EUOfficeViewer, EUHwpViewer, VideoViewer+ffmpeg, imageformats, EURawImg, EUImageEnhance) | `ui/src/features/preview/` + `engine/src/core/preview/{cr_thumbnail.cpp,cr_preview_text_exts*.cpp}` — real bytes + metadata + truthful "preview not rendered" hints; image classify+thumbnail (M-102), archive browse-inside (M-116), text previews | **PARTIAL** (D-P8 DEFERRED: renderer bundling is a recorded supply-chain decision — ~130 MiB family vs 8.2 MiB total) |
| 16 | `DiskHealth.exe` companion (818 files, CrystalDiskInfo lineage) | the separate disk-health window `ui/src/features/health/disk-health-window.tsx` (840×590, all-disks selector, 4-state grades, the reference's SMART table) + engine `engine/src/core/health/{cr_sat.cpp,cr_csmi.cpp,cr_nvme.cpp,cr_smart_ata.cpp,cr_grade.cpp}` + `engine/src/win/{cr_health.cpp,cr_health_wmi.cpp}` (SAT byte-exact D-P4a, WMI fallback D-P4b) | **ALREADY-INTEGRATED** (D-P4 CLOSED 2026-11-17; sub-divergences recorded: same-process window, 5-min poll, retained in-app /health route; vendor bridge dialects D-P4c deferred) |
| 17 | Language packs (37 .qm incl. 25 locales) | `ui/src/i18n/{en.ts,zh-CN.ts}` | **PARTIAL** (2 locales shipped deliberately; i18n sweep keeps parity) |
| 18 | `Resource/`+`ResourceDark/` RCC theme bundles + style.css | the app's own themed web assets (`ui/src/{assets,theme,styles}/`) | **ALREADY-INTEGRATED** (consolidated) |
| 19 | Qt runtime + plugins (25.7 MB + diskhealth QML stack) | WebView2 (OS renderer) + React — no bundled GUI runtime | **ALREADY-INTEGRATED** (legitimate architecture consolidation, FOOTPRINT-DELTA §1) |
| 20 | VC/UCRT/OpenSSL runtime zoo (4 VC generations, 4 OpenSSL generations, 200 UCRT shims) | statically-linked CRT + vcpkg `z.dll`/`lz4.dll`/`zstd.dll` beside `cr_ui.dll` (matched class for the compression family) | **ALREADY-INTEGRATED** (one modern toolchain; deliberate) |
| 21 | Installer: Inno unins000 trio + SetupUE 16-OS gate + InfoForSetup telemetry | `installer/recoverywizard.nsi` (outer NSIS + inner Tauri bundle; privilege page; ONE complete uninstaller `HeicFastUninstall.exe`); Win10 gate (WebView2 floor); no install telemetry | **ALREADY-INTEGRATED** (deliberate tech swap D-I1/D-I4/D-I7/D-I10) |
| 22 | DRWService SCM registration / setupdrvx64 on-demand driver install / EuShare install.bat | nothing: no service, no driver, no System32 writes, no cert-store changes — user-mode direct `\\.\PhysicalDriveN`/`HarddiskVolumeN` opens under the admin token; `ERR_ELEVATION_REQUIRED` refusal vocabulary | **ABSENT** (deliberate D-I5/D-I6-reversed/D-FALL-reversed: the product's divergence is the entire absence of kernel components) |
| 23 | CVersionCtrl auto-update chain (CheckUpdateThread, Upgrade.ini, AutoUpdate.dll, EuDownloader) | none — releases via GitHub, manual download | **ABSENT** (deliberate v1 scope cut D-I8) |
| 24 | Cloud recovery (cloud_session, EuICloudRecovery, firebase pair) | none | **ABSENT** (out of scope — no cloud surface claimed; FOOTPRINT-DELTA §1) |
| 25 | NAS stack (EuMrx.sys, EuShare, NasConfig, libssh2, connect.zip agent) | none | **ABSENT** (out of scope — no NAS claim; FOOTPRINT-DELTA §1) |
| 26 | Telemetry/monetization (Aliyun channel, firebasefetch, EuUserRate, EUinApp, retention experiments, DRWReport upload) | none (no telemetry, no activation theater — F-ACT-01 removed it) | **ABSENT** (deliberate; privacy + simplicity, D-I10/D-P3) |
| 27 | WinPE emergency media (BUILDPE 194 files, PE_Tools builder, dlconfig/PEtools) | none | **ABSENT** (out of scope — separate deliverable, recorded; FOOTPRINT-DELTA §1) |
| 28 | `PriorityDisplayType.xml` + FilterFolder SumaryFileType groups | `engine/src/core/query/cr_config_display_data.cpp` (N-132 — both layers verbatim) | **ALREADY-INTEGRATED** |
| 29 | `usefullostfiles.xml` (27-ext "worth recovering" whitelist) | none — dispositioned **OOS-UI** (the og's whitelist is a DRWUI display-tier promotion of carved RAW photos, a UI-layer concern the engine docs scope out) | **ABSENT** (recorded disposition, not a gap: `engine/docs/PARITY-OG-CORE-INVENTORY.md` n.xml/usefullostfiles row; `engine/docs/REFERENCE-API-GAP.md` R-31 "the one dispositioned remainder: usefullostfiles.xml whitelist promotion is OOS-UI (verified)") |
| 30 | `DRWReport.exe` crash reporter + dbghelp + DRWLoader supervision | in-app report (`cr_ui_recovery_report_json`, `engine/include/cr/cr_ui.h:1542`) + bridge logging seam (NG-15); no crash-restart supervision | **PARTIAL** (consolidated by design; crash-report parity vocabulary recorded in worklog) |
| 31 | `config/nType.xml` display-priority + "Unimportant files" bucket gate | the query/filefilter display layers (row 28's `cr_config_display_data.cpp`, N-132); the "Unimportant" bucket is R-20's residual PARTIAL — dispositioned OOS-UI with the N-215 re-truth | **PARTIAL** (R-20 residual, `engine/docs/REFERENCE-API-GAP.md`) |

Current product install footprint for comparison (v0.9.3): 8 files / ≈8.2 MiB in a flat
`$INSTDIR` + `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}` + 3 registry keys;
no service, no driver, no System32 writes, no certificates. Delta vs the reference live
tree: 1,276 vs 8 files (159.5×), 518.7 MB vs 8.6 MB (60.1×) — reconciled per module in
`worklog/FOOTPRINT-DELTA.md` §1 (this session re-verified its rows against HEAD).

---

## 6. corpus-UNKNOWN (what a complete footprint statement would require but the corpus does not document)

1. **A pristine installer payload** — the main Inno Setup EXE/script is not captured;
   silent/`VERYSILENT` flags, install-order, per-file `[Files]` mapping are inference
   only (`needcopy.txt` is the partial copy-list). Also the Inno `Uninstall` registry key
   DRW writes for itself (`{AppId}_is1` is the convention — inference, not evidence).
2. **A live-machine before/after registry diff** — only in-binary strings + the PE
   `regdrv.reg` exist; `C:\ProgramData` layout, Start Menu group, and the license-data
   registry layout (`writeActivateData` is named but its key/value layout is NOT-IN-CORPUS).
3. **Driver post-state on a real machine** — timestamps/DACLs inside a live
   `System32\Drivers`; the exact `CreateService` start-type argument for live eprdtdrv
   install (the corpus states on-demand deployment; the siblings are demand-start —
   labeled inference in the prior sheet §8).
4. **DiskHealth scheduled-task definitions** — the `.bat` trio drives schtasks-style
   management, but the resulting task set is not captured.
5. **`EuMrxnp.dll`** — the Network Provider shim referenced by `ProviderPath` is not
   shipped ("driver-only drop"); its content is unknowable from the corpus.
6. **`Ext2SearchDLL.dll`** — referenced in DRWUI LoadLibrary strings but absent from
   every tree copy (legacy stub).
7. **`VideoViewer/sfc`** (211,614 B, magic " cfs-…") — not a PE, not an archive; purpose
   UNKNOWN (player-skin/cipher-blob candidate). `auto.txt` (opaque GUID blob) and
   `lostpart.dat`/`partinfo.dat` full decodes also open.
8. **`EuSendMessage.exe`'s exact role** — the one PE-only binary with no decoded role in
   the census (queued for per-binary RE per PE census §5).
9. **Which component invokes `EuShare/install.bat` at runtime** (and whether its console
   window is suppressed) — not documented.
10. **The reference's disk-health refresh interval** (the product chose the
    CrystalDiskInfo-lineage 5-min default — VERIFICATION §10, corpus-UNKNOWN).
11. **BUILDPE bookkeeping convention** — whether the 194 PE-media files count toward
    "installed footprint" (this sheet keeps them separate from the 1,276-file live tree,
    per the T14 convention).

---

### Verification ledger (this session)

Re-measured at corpus HEAD `be0cfe95` via `git ls-tree -r -l HEAD -- og-codebase`
(names/sizes only): 1,470 files / 702,834,195 B total; BUILDPE 194 / 184,084,980 B;
per-directory counts/sizes all match the T14 census 1:1 (root 149/140,060,599 ·
diskhealth 818/55,371,700 · VideoViewer 76/104,424,473 · EUFileRepair 87/47,843,550 ·
PartitionRecovery 57/28,158,618 · language 37/6,386,275 · SaveScan 3/69,107,396 …).
Engine sizes verified: NTFSSearchDLL 577,256 · RAWSearchDLL 644,840 · etc. Driver sizes
verified (incl. the eprdtdrv 27,728 correction). 30 `.mo` files counted in-tree.
Every role claim cites a corpus research doc; no og binary was opened, and nothing was
copied or proposed for copying. Product paths verified by direct inspection
(`engine/src/**`, `ui/src/**`, `installer/**`, `engine/data/userdata-defaults.json`,
`engine/scripts/gen_{filefilter,filterfolder}.py`). The one file written this session:
this sheet.
