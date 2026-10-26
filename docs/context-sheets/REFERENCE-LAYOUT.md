# REFERENCE-LAYOUT — EaseUS Data Recovery Wizard (DRW): Installed Directory Layout

> Agent D1 (task 2-d1) evidence sheet. Reference product = **EaseUS Data Recovery Wizard (DRW)**,
> the product captured in the research corpus `work/easeus-drw-research` @ 03ad46c.
> Primary evidence: `og-codebase/` (git tree of the reference product's install dir — 167
> top-level entries, **1,470 tracked files** by `git ls-tree -r | wc -l`), the
> `architecture/` docs, and `re-workspace/artifacts/*.md` (which quote decompiled/strings
> evidence). No repository file was modified.
> Companion sheet: D2 covers installer/registry/service-registration behavior in a separate
> file (merged later); this sheet is layout-on-disk.

---

## 1. Product identity

| Fact | Value | Citation |
|---|---|---|
| Product | EaseUS Data Recovery Wizard ("DRW"); window title observed live: "EaseUS Data Recovery Wizard Free" | corpus `README.md` ("Data Recovery Wizard — Research Repository"); `re-workspace/artifacts/UIUX/dynamic-ground-truth--landing-page-windows.md` (process list + verbatim strings) |
| Version (shipped build) | **21.0.0.0**, edition `free`, channel `easeus`, market `abroad`, `product-code=RE-DRW-FREE-WIN`, `SetupBuildTime=1013` | `og-codebase/config/version.ini` `[ver]` |
| Build fingerprint | `DRWUI [21.0.0.0_free easeus] build at[Oct 13 2026 13:33:43] [40504ccb1923e316c9c9a1ffb06907c8307930de]` — build git-commit hash; environment string `abroad__free__easeus__21000__1013` | quoted in `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` §1 (from `og-codebase/log/DRWUI.log` line 2) |
| Prior-release marker | `Upgrade.ini`: "Product Version=20.8.0.0", `killDRWUI=1`, upgrade URL `http://www.easeus.com/download/data-recovery-software.html` | `og-codebase/Upgrade.ini` |
| Repair-line dev version | `strutureReg.log`: `"appversion": "19.3"`, moduleversion "19.2回退分支" | `og-codebase/strutureReg.log`; `artifacts/config-files/vendor-logs--qa-session-forensics.md` §3 |
| PE build | Same 21.0.0.0 build-pair; `version=free`, `product-code=RE-DRW-FREE-WIN` (pristine vs the live copy's runtime-mutated fields e.g. `lastUpdateTs`) | `og-codebase/BUILDPE/EaseUS-x64/config/version.ini`; `artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md` §4 |
| Process topology | DRW.exe (DRWLoader supervisor) → DRWUI.exe (main UI) → DRWReport.exe on crash; DRWService.exe = "Read FVE Server" service | `artifacts/DRW-exe--launcher.md`; `artifacts/DRWService-exe--bitlocker-service.md` |
| Vendor QA leftovers | The shipped tree contains the vendor's own 2026-10-14/15 QA-night logs + 3 .rsf sessions | `artifacts/config-files/vendor-logs--qa-session-forensics.md` |

## 2. Main install directory layout (`og-codebase/` = the installed tree)

1,470 tracked files under 167 top-level entries. Organized by file class (all paths below are
relative to the install root; evidence per class):

### 2.1 Core product executables (install root)
| File | Purpose | Evidence |
|---|---|---|
| `DRW.exe` | **DRWLoader** — thin supervisor/watchdog; entry point. Boots `<exe_dir>\log\`, 2 MB shared log mapping, launches `DRWUI`, restarts on crash, launches `DRWReport.exe` on crash events. PDB `DRWIntelligentScan\DRWLoader\x64\Release\DRWLoader.pdb` | `artifacts/DRW-exe--launcher.md` §1-2 |
| `DRWUI.exe` | **Main UI application** (Qt; 119,182 functions / 2.38M lines decompiled; 1,073 RTTI classes; 126 windows) | `artifacts/DRWUI-exe--main-ui-architecture.md` (index row), `architecture/00-INDEX.md` |
| `DRWService.exe` | **BitLocker key service** — SCM service "Read FVE Server" (SERVICE_AUTO_START, OWN_PROCESS, LocalSystem); pipe client to `\\.\Pipe\DRWBitlocker`, enumerates `HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock`; installed/uninstalled via `\DRWService.exe /install` `/uninstall` | `artifacts/DRWService-exe--bitlocker-service.md` §1 |
| `DRWReport.exe` | **Crash reporter** — customized XCrashReport (CodeProject); collects minidumps + loader telemetry JSON; upload via `send_dmp_with_aliyun.exe` (Aliyun OSS) | `artifacts/DRWReport-exe--crash-reporter.md` |
| `EUinApp.exe` | In-app purchase / store bridge; "orphaned IE-emulation bootstrapper" (209 fns; GlobalSign/DigiCert CRL+OCSP URLs embedded) | `artifacts/Licensing/licensing-activation-chain.md`; `artifacts/Licensing/euinapp--ie-bootstrapper.md` |
| `InfoForSetup.exe`, `SetupUE.exe` | Installer bootstrap pair (live-tree only, absent from PE): SetupUE gates 16-OS table + reads/writes `SOFTWARE\EASEUS\DRW\Version`; InfoForSetup = `/SendInfo` install telemetry (Activity/Country/Timezone/TestId) | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` |
| `unins000.exe`, `unins000.dat`, `unins000.msg` | **Inno Setup uninstaller trio** (`unins000.dat` header: "Inno Setup Uninstall Log (b) 64-bit") | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1; direct `od` of `unins000.dat` |
| `EuDownloader.exe` | Resumable range-downloader w/ MD5 verify (WAIK ISO carving for the PE builder, `connect.zip` NAS agent) | `artifacts/config-files/config-data-reference.md` (dlconfig.xml consumer) |
| `send_dmp_with_aliyun.exe`, `AliyunWrap.dll`, `AliyunWrapExe.exe` | Crash-dump/log upload channel (Aliyun OSS; `AliyunConfig.ini` = its config) | `config-data-reference.md` §Install/misc |
| `EuUserRate.dll` | NPS rating popup component (config.ini `[User] UserRated*`) | og-codebase listing + `config-data-reference.md` |
| `PubLog.dll` | Shared logging library (14 exports: `InitLog`, `Log`, `LogError/Info/Warning`, `DebugTrace`; classes `CDirectLog`, `CFileMappingLog`, `IEsLog`) | `artifacts/per-binary/PubLog.md` |
| `legacy.dll`, `GDIPLUS.DLL`, `dbghelp.dll`, `Japanese.dll` | Legacy/support DLLs; Japanese.dll = MUI resource DLL (launcher language fallback lists Chinese.dll/Deutsch.dll/Japanese.dll — **only Japanese.dll ships in this tree**) | `artifacts/DRW-exe--launcher.md` §1 step 5; tree listing |

### 2.2 FS scan engines (install root) — the "SearchDLL" plugin family (9)
`NTFSSearchDLL.dll`, `FATSearchDLL.dll`, `exFATSearchDLL.dll`, `EXT4SearchDLL.dll`,
`HFSPlusSearchDLL.dll`, `BtrfsSearchDLL.dll`, `ReFSSearchDLL.dll`, `ZfsSearchDLL.dll`,
`RAWSearchDLL.dll` — all at install root, loaded by DRWUI under the EUCreate* plugin
contract (controller=tree-builder multiple inheritance; RAWSearchDLL has 106 analyser
classes/70 magic keys). Evidence: `architecture/00-INDEX.md` (05 row, SearchDLL rows);
`artifacts/SearchDLL/*`. Each engine's deep-dive artifact confirms it lives in the app root
(e.g. `SearchDLL/ntfs-engine--deep-dive.md` targets `og-codebase/NTFSSearchDLL.dll`).

### 2.3 `PartitionRecovery/` — lost-partition module set (57 items)
- **30 MOM `.mo` modules** (in-house COM-like object model; loaded by bare name →
  `LoadLibraryA("<Name>.mo")` → `GetModule`/`CreateObject` exports): Device.mo,
  DeviceManager.mo, LdmManager.mo, Partition.mo, NTFSLib.mo, RegLib.mo, bitlockerhelp.mo,
  EFIBoot.mo, Log.mo, Thread.mo, Mom.mo, etc. Evidence:
  `artifacts/PartitionRecovery/mom-module-system--architecture.md` (loading contract) +
  `artifacts/PartitionRecovery/mo-module-map.md` (all 30 mapped).
- **20 DLLs**: MainModule.dll (268 op-class exports), PartitionRecovery.dll (59 IObject
  symbols), RecoveryModule.dll, ToolsModule.dll, UILogic.dll, CloneModule.dll, RawFixer.dll
  (transactional FS repair), BitLockerLib.dll, Boot.dll, BootDriver.dll, Burn.dll (IMAPI2),
  Win32Bcd.dll, libvmdk.dll, GetDriverInfo.dll, Logger.dll, AutoUpdate.dll, peformat.dll,
  Qt5Core.dll, zlib.dll, zlib1.dll. Host-resolution: path tries
  `<exe_dir>\RecoveryModule.dll`, falls back to `<exe_dir>\PartitionRecovery\RecoveryModule.dll`
  (`mom-module-system--architecture.md`).
- **2 kernel drivers**: `eprdtdrv.sys` (live-OS raw-disk proxy, EPMNtDrv) and
  `eprdtdrv_win7older.sys` (Win7- build) — see §4.
- **Installer**: `setupdrvx64.exe` (PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`) +
  `setupdrv.ini` (section `[eprdtdrv]`) — installs the driver at first run.
- **Data**: `lostpart.dat`, `partinfo.dat` (shipped data blobs), `WinChkdsk.exe`.

### 2.4 Repair tools (two worlds: DRW-native + "Fixo" subtree)
- Top level (DRW-native chain): `EUImgRepair.exe`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`,
  `EURawImg.exe`, `EuOfficeRepairWin32.exe`, `EuOfficeTools.exe`, `EuRepairHost.exe`
  (out-of-proc activation bridge to activation.easeus.com[.cn]), `EUUnZip.exe` (generic
  child-process unzip launcher), `EUHwpFilePathCheckerModule.dll` + `EUHwpViewer.exe`
  (Korean HWP viewer chain), `EUOfficeViewer.exe`, `PDFViewer.exe` (pdfium-based preview
  viewers), `EUImageEnhance.dll`, `EUVideoRepair.dll`. Evidence:
  `architecture/06-repair-chain.md` (per index) + `artifacts/FileRepair/*`.
- `EUFileRepair/` — Fixo engine subtree, self-contained: repair EXEs
  `EUImgRepairFile.exe`, `EuOfficeRepairFile.exe`, `EuPDFRepairFile.exe`, `EUArchiveRepair.exe`,
  `EUAdobeRepair.exe` + ImageMagick delegates (`CORE_RL_*` 25, `IM_MOD_RL_*` 7) + own UCRT
  (api-ms-win-* + ucrtbase) + libcurl/openssl-3/heif. (Live-tree only — absent from PE.)
- `VideoViewer/` — video preview/repair subtree: `EaseUS_VideoViewer.exe`,
  `VideoFileRepairer.exe`, `EaseUS_MediaGraph.dll`, `ffmpeg.exe` + avcodec-62/avdevice-62/
  avfilter-11/avformat-62/avutil-60/swresample-6/swscale-9/postproc-58, `libzmq-mt-4_3_5.dll`,
  UI pngs/cur, `sfc` (211,614-byte data file). Preview bridge ports 127.0.0.1:20171/20172
  (VAPreviewListener). Evidence: `artifacts/DRWUI-exe--main-ui-architecture.md` (§9 per index);
  `artifacts/per-binary/EaseUS_VideoViewer.md`.
- Viewer launch paths are **module-dir-relative**: DRWUI strings
  `%s\PDFViewer.exe %p "%s" "%s" %I64d %d`, `EUOfficeViewer.exe`, `EUHwpViewer.exe`,
  `%s\EuOfficeTools.exe "%s" "%s"` (`re-workspace/reversed/DRWUI.exe/strings_utf16.txt`
  ~lines 14875-14890) — the `%s` is the app dir.
- Password dictionary for encrypted-archive/HWPX repair: top-level file `Dict`
  (2,742 B, 228 comma-terminated password entries) — `artifacts/config-files/config-data-reference.md` §Dict.

### 2.5 Cloud / NAS subsystem
- Install root: `cloud_session.dll` (OAuth2 loopback: Dropbox/GDrive/OneDrive),
  `EuICloudRecovery.dll` (es_icloud full iCloud client), `firebasefetch.exe` +
  `firebaseonlineconfig.dll` + `google-services.json` (Firebase remote config = A/B flags) +
  `remote_config_data` (61,624 B fetched remote-config snapshot) + `icloud_logger`
  (88 B iCloud log stub: "[esicloud::ICloudRecovery::logout:367] User login out.").
  Evidence: `artifacts/Cloud/cloud-recovery-architecture.md` (per index).
- `NasConfig/` = `NasModel.xml` (Synology model → CPU-arch catalog) + `ssh_config.ini`
  (`PushPath=/tmp/EaseUS/`, `[SSH]` state slots). `connect.zip`/`connect.exe` are NOT
  shipped — downloaded at runtime (see §6).
- `EuShare/` = `EuMrx.sys` (NAS redirector kernel mini-driver) + `install.bat` +
  `uninstall.bat` (driver service scripts). Evidence: `artifacts/EuShare/eumrx-nas-architecture.md`.

### 2.6 `diskhealth/` — SMART companion app (self-contained Qt5/QML copy)
`DiskHealth.exe` (CrystalDiskInfo lineage), `DiskHealth.rcc`, `addtask.bat` /
`deltask.bat` / `querytask.bat` (scheduled-task helpers), full private Qt5 stack
(Qt5Qml/Qt5Quick/Qt5QuickControls2/... + own platforms/styles/imageformats/bearer +
api-ms-win-* + ucrtbase). Evidence: `artifacts/DiskHealth/diskhealth--smart-companion.md`
(per index); git tree of `og-codebase/diskhealth/`.

### 2.7 Qt runtime + runtimes + codec/crypto/network libs (install root)
- Qt5: `Qt5Core/Gui/Network/Svg/Widgets/WinExtras.dll` + plugin dirs
  `platforms/{qwindows,qoffscreen,qminimal}.dll`, `styles/qwindowsvistastyle.dll`,
  `imageformats/{qjpeg,qgif,qsvg}.dll`, `iconengines/qsvgicon.dll`
  (`artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md` §1 table).
- C/C++ runtimes, three compiler generations: VC9 (`msvcp90/msvcr90/msvcm90` +
  `Microsoft.VC90.{CRT,MFC}.manifest`), VC12 (`msvcp120/msvcr120`), VC14
  (`msvcp140/msvcp140_1/vcruntime140/concrt140`), UCRT (`ucrtbase.dll` + ~49
  `api-ms-win-*` forwarders) — same census §1 ("engine codebases of different ages").
- Crypto: OpenSSL 3 generations — `libeay32/ssleay32` (+`MD` variants, `libssl32`),
  `libcrypto-1_1`/`libssl-1_1` (+`-x64`), `libcrypto-3-x64`/`libssl-3-x64`
  (`artifacts/runtime-families/*` per index).
- Media: `de265.dll`, `heif.dll`, `libx265.dll`, `pdfium.dll`.
- Network/compression: `cpprest141_2_10.dll`, `libzmq-mt-4_3_5.dll`, `libssh2.dll`
  (SSH-2.0-libssh2_1.11.0 — connect.exe's runtime dependency),
  `7za.exe`, `archive.dll`, `zlib.dll`, `zlib1.dll`, `liblzma.dll`.

### 2.8 Config/data files shipped in the install dir
| File | Purpose | Citation |
|---|---|---|
| `config/version.ini` | product/build identity ([ver] section, §1 above) | read directly |
| `config/config.ini` | 22-language table, fonts, window 1240x840 min 800x600, `[User]` state (showRaw, FileRepairSavePath, UserRated...), ENS buy-button coords | read directly; `artifacts/config-files/config-data-reference.md` |
| `config/FilterFolder.ini` | noise/junk filter (folder/file blocklists, 10-locale "New Folder" filter, TxtViewFiles, SumaryFileType) | `artifacts/config-files/filtering-priority--config-layer.md` §3 |
| `config/usefullostfiles.xml` | 27-ext "valuable formats" whitelist | same §1 |
| `config/PriorityDisplayType.xml` | type-tree ordering (alias sort keys) | same §2 |
| `config/RawConfig.ini` | per-extension RAW-carve on/off (37 types) | same §4 |
| `config/shellexe.ini` | 25-locale Explorer shell-extension taglines ("Recover any deleted data with EaseUS") | same §5 |
| `config/nasconfig.ini` | NAS defaults (busenas, vendor→logo map) | read directly |
| `config/themdark.ini` | dark-theme config | tree listing |
| `FileFilter.xml` | result-category tree: 8 FileCategories / 1489 `<Extention>` entries | `config-data-reference.md` |
| `raw_signs.json` (35,342 B) | RAW signature DB #1: 139 extensions, header/tail signs + confidence | `config-data-reference.md` |
| `EUUserData` (16,891 B, **JSON file, not a directory**) | RAW signature DB #2: ext → `{signs:[offset,hex], mime}` libmagic-style map | read directly; `config-data-reference.md` |
| `PEtools.xml` + `dlconfig.xml` | WinPE builder manifests (WAIK deployment + download-carving) | `config-data-reference.md` §WinPE builder |
| `DRWReport.ini` | crash-reporter file manifest: `[FilesToAdd] File001=log\DelFat.log … File004=Drwdump0.DMP … File010=log\BuildRaw.log` | read directly (§8) |
| `AliyunConfig.ini` | Aliyun OSS upload config (encrypted endpoint/keys; AllowSendUrl `http://track.easeus.com/product/index.php?...`) | read directly |
| `DataFile.ini` | UTF-16 stub `[HEAD_DATA] TOTALCOUNT=0 STARTINDEX=0` (data-file index, empty pristine) | read directly |
| `recoveryFree.ini` | free-edition quota state (mostly binary/empty in pristine copy) | read directly |
| `Upgrade.ini` | upgrade config (20.8.0.0, killDRWUI=1, URL) | read directly |
| `style.css` (427 KB) | CSS for embedded web views (EUinApp windows) | `config-data-reference.md` |
| `auto.txt` (316 B) | GUID-tagged binary blob `{429E92F9-5908-42A7-AD61-655349DAE2F1}` | read directly |
| `needcopy.txt` | 55-name runtime-DLL list the packager must carry (InfoForSetup/AliyunWrap/api-ms-*/d3d11/msvcr100...) | read directly |
| `google-services.json` | Firebase project config | tree listing |
| `Resource/` = `ImageResource.xml` + `drw_light.rcc`; `ResourceDark/` = `ImageResource.xml` + `drw_dark.rcc` | light/dark theme resource bundles | git tree |
| `language/` | **37 .qm** files (26 product/OEM incl. Mainland, Mainland_Foxit, Mainland_JZ, Mainland_LC + 11 qtbase_*.qm) | git tree; `artifacts/Languages/language-packs--family-census.md` |
| `License/` | 19 third-party license texts (openssl, qt, libraw, opencv, ...) | git tree |
| `SaveScan/` | 3 auto-saved `.rsf` sessions (vendor QA leftovers, §7) | git tree; `artifacts/SaveScan/rsf-format--scan-session.md` |
| `log/` | runtime logs (§8) | git tree |
| Shipped log leftovers at root | `EPMLOG.log`, `Eaolog.log`, `strutureReg.log`, `reglog_win.txt` (§8) | `artifacts/config-files/vendor-logs--qa-session-forensics.md` |

## 3. Install root path (where this tree lives on Windows)

**Evidence found (all of it):**
1. **bitlockerhelp.mo literal strings** (rabin2, .rdata UTF-16LE):
   `C:\Program Files\EaseUS`, `C:\Program Files\EaseUS\BL`, `x:\Program Files\EaseUS\BL`
   (`re-workspace/reversed/PartitionRecovery/bitlockerhelp.mo/rabin2_strings_data.txt`
   lines 161-166; `artifacts/MOM-modules/bitlockerhelp.md` §Notable string evidence).
   → the product family installs under `C:\Program Files\EaseUS\...`.
2. **DRWUI PE-builder strings** (UTF-16): `\Program Files\EaseUS\DRW`,
   `\Program Files\EaseUS\DRW\PartitionRecovery`, `\Program Files\EaseUS\DRW\`,
   `\Program Files\EaseUS\BootRepair` — xcopy targets for the **bootable media**
   (`re-workspace/reversed/DRWUI.exe/strings_utf16.txt` lines 15424-15615).
3. **winpeshl.ini / auto.bat (PE)**: launches
   `%systemdrive%\Program Files\easeus\DRW\DRW.exe`
   (`artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` lines 18, 137;
   `architecture/03-winpe-boot-environment.md` §2).
4. **Per-user echo of the same convention**: `%AppData%\EaseUS\DRW\connect.zip`
   (`artifacts/EuShare/eumrx-nas-architecture.md` §6 note 2).
5. Registry key naming `SOFTWARE\EASEUS\DRW` (HKLM; Language/Version keys) mirrors the
   `EaseUS\DRW` folder convention (`artifacts/DRW-exe--launcher.md` §1 step 5;
   `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` §1).

**Conclusion**: Every runtime path in the corpus is **module-relative**
(`<exe_dir>`, GetModuleFileName-derived — see §6/§7/§8), so the product works from any root.
The corpus proves the family prefix `C:\Program Files\EaseUS` (bitlockerhelp.mo) and the
`EaseUS\DRW` subfolder convention (PE builder + registry + AppData), but **no single literal
full path of the live-OS DRW install dir** appears anywhere in the corpus text.
**Most probable install root: `C:\Program Files\EaseUS\DRW\`** (consistent across PE-deploy
target, registry key, and AppData sibling) — the exact live subfolder name (e.g. whether the
live installer uses `EaseUS Data Recovery Wizard` vs `DRW`) is **UNVERIFIED-BY-CORPUS**
(D2's installer/registry evidence may settle it; Inno `unins000.dat` embeds the {app} path but
is compressed binary, not decoded in the corpus).

## 4. Kernel drivers on disk (the 7 binaries — where each lives)

Source: `architecture/02-kernel-driver-architecture.md` §1-§3, §5 + git trees.

| # | Driver binary | Ships in install dir at | Ends up on a live machine at | Service name / start |
|---|---|---|---|---|
| 1 | `eprdtdrv.sys` (EPMNtDrv, raw-disk proxy; device `\Device\EPMNTDRV`, namespace `\\.\EPRDTDRV\{d}\{p}`) | `PartitionRecovery\eprdtdrv.sys` | **copied at runtime to `%SystemRoot%\System32\Drivers\%s.sys`** by `PartitionRecovery\setupdrvx64.exe` (strings: `\System32\Drivers\%s.sys`, `\SystemRoot\System32\drivers\%s.sys`, `SYSTEM\CurrentControlSet\Services\%s`, CreateServiceA) | name pinned by `setupdrv.ini` `[eprdtdrv]`; group "Boot File System"; optional class-UpperFilters install paths (diskdrive + volume class GUIDs, strings `DiskFlt`, `VolFlt`, `PnP Filter`) |
| 2 | `eprdtdrv_win7older.sys` (same source, Win7- build) | `PartitionRecovery\eprdtdrv_win7older.sys` | same mechanism, chosen on Win7- | same |
| 3 | `ebrntdrv.sys` | **`BUILDPE/x64/Windows/system32/ebrntdrv.sys`** (PE media only) | n/a — WinPE deployment only (`\SystemRoot\System32\ebrntdrv.sys`, Type 1, Start 3, Group "Boot File System") | `ebrntdrv` (regdrv.reg) |
| 4 | `epmdkdrv.sys` | **`BUILDPE/x64/Windows/system32/epmdkdrv.sys`** (PE media only) | n/a — WinPE only (`\SystemRoot\System32\epmdkdrv.sys`, Type 1, Start 3) | `epmdkdrv` (regdrv.reg) |
| 5 | `EUDCPEPM.sys` (device-stack HOOK; all-28-slot hook + `\Driver\VolSnap` attach) | **`BUILDPE/x64/Windows/system32/drivers/EUDCPEPM.sys`** (PE media only) | n/a — WinPE only; BOOT_START; disk-class `UpperFilters = EUDCPEPM` on GUID_DEVCLASS_DISKDRIVE | `EUDCPEPM`, DeviceName `epmdiskcopydrv`, IoctlBase 0x10000 |
| 6 | `EUEDKEPM.sys` (named raw-disk door, PID-token gated) | **`BUILDPE/x64/Windows/system32/drivers/EUEDKEPM.sys`** (PE media only) | n/a — WinPE only; SYSTEM_START | `EUEDKEPM`, DeviceName `EUEDKEPM` |
| 7 | `EuMrx.sys` (NAS RDBSS redirector; device `\Device\EuRdr`) | `EuShare\EuMrx.sys` (app dir subtree) | copied by `EuShare\install.bat`: `copy /Y "%~dp0EuMrx.sys" "%systemroot%\system32\DRIVERS"` | `sc create EuMrx binPath= "system32\DRIVERS\EuMrx.sys" type= filesys start= demand group= Network` + NetworkProvider entries (ProviderPath `System32\EuMrxnp.dll`) |

Also a **user-mode service** (not a driver): `DRWService.exe` — SCM service
"Read FVE Server", SERVICE_AUTO_START, binary path = full path of `DRWService.exe`
(itself — registered from wherever it was installed; §2.1).

**Rule**: only `eprdtdrv(_win7older).sys` (raw disk) and `EuMrx.sys` (NAS) are copied into
`System32\drivers` on a live machine, both on demand from the app dir; the four WinPE
drivers live only on the bootable media (`architecture/02` §2.1: "ebrntdrv/epmdkdrv —
preloaded inside the WinPE boot environment (BUILDPE)"; §3: "Deployed only in WinPE
(BUILDPE/.../drivers/)... NOT loaded during normal Windows operation of DRW").

## 5. ProgramData / shared machine data

- **No ProgramData path appears in any corpus document.** Grep of `architecture/` +
  `re-workspace/artifacts/` + all `.md/.txt/.json/.ini/.xml` for `ProgramData|ALLUSERSPROFILE`
  returns only: (a) a bare token **"ProgramData"** in DRWUI.exe UTF-16 strings
  (`re-workspace/reversed/DRWUI.exe/strings_utf16.txt` line 14976 — adjacent to
  `CEaoActiveOnline` activation/`"Program Files (x86)"` strings, i.e. path-building material
  in the activation component, with no full path composed anywhere), and (b) unreferenced
  hits in raw windows_census/Inno-internal strings.
- Shared machine state documented in the corpus lives in **HKLM registry**, not the
  filesystem: `HKLM\SOFTWARE\EASEUS\DRW\Language` (launcher §1), `SOFTWARE\EASEUS\DRW\Version`
  (installer contract), `SOFTWARE\EASEUS\DRW\{UserRecovered,UserGuidWnd,ChrisWnd,ExitShowHelp}`
  (DRWUI strings census), `HKLM\SOFTWARE\EASEUS\EPM\Version\OEM` (auto.bat OEM branch),
  service keys `SYSTEM\CurrentControlSet\Services\{eprdtdrv,EuMrx,Read FVE Server}`.
- **All shared writable data (logs, sessions, config cache) is stored in the install
  directory itself**, module-dir-relative (§6-§8) — i.e., the product assumes a writable
  install dir, not a ProgramData layout.
- **UNVERIFIED-BY-CORPUS**: any concrete `C:\ProgramData\EaseUS\...` content (e.g.
  activation/license files) — the "ProgramData" token hints the activation component may
  build one, but no path is composed in strings or artifacts.

## 6. Per-user data

- **HKCU registry** (hive explicit in artifacts): `HKCU\SOFTWARE\EASEUS\EaseUS Data
  Recovery Wizard\EUImgRepair` — image-repair invocation-chain/state key
  (`artifacts/FileRepair/image-repair--format-grammar.md` lines 9, 136;
  `artifacts/UIUX/04-userflow-state-machine.md` line 121: "Scenario/product state via
  registry `SOFTWARE\EASEUS\EaseUS Data Recovery Wizard` (+ FoxitRestore codename keys)").
  DRWUI also reads `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize`
  (dark-mode detection; strings lines 12937/15498). Hive for the `SOFTWARE\EASEUS\DRW\*`
  subkeys is not always stated per key (launcher's Language read is HKLM) — see UNVERIFIED.
- **%AppData%**: `%AppData%\EaseUS\DRW\connect.zip` — the NAS agent downloaded by
  `CLinuxDownloadConnectZipThread`, extracted by `EUUnZip.exe` to `NasConfig\connect.exe`
  **inside the install dir**; version gated by QSettings `[VERSION] connectver`
  (`artifacts/EuShare/eumrx-nas-architecture.md` §6 note 2). This is the only AppData path
  in the corpus.
- **Install-dir per-user-ish state files** (module-dir-relative, so nominally shared but
  per-machine): `config/config.ini [User]` (showRaw, `FileRepairSavePath=`, UserRated*,
  VideoVolume), `config/version.ini` runtime-mutated fields (SelLanguage, qq1/qq2,
  InitActive, lastUpdateTs — census artifact §4 "runtime-mutated file"),
  `%s\Configure.dat` (license/serial-expiry state; DRWUI strings `%s\Configure.dat`,
  `SOFTWARE\EASEUS\DRW\` + `SNExpired` cluster),
  `DRWCfg.bin` (§7).
- **OfficeRepairTmp**: QA log shows `OfficeTmp=D:\OfficeRepairTmp`
  (`artifacts/config-files/qa-logs--session-forensics.md` §2 quoting `log/DRWUI.log`) — the
  office-repair temp dir path is configurable/on-drive; default not documented in corpus.
- **HKCU residue of components**: `HKCU\Software\CodeProject\XCrashReportTest` (DRWReport
  provenance, `artifacts/DRWReport-exe--crash-reporter.md`).

## 7. Sessions and results (.rsf)

- **Location**: `<install dir>\SaveScan\` — "The SaveScan folder path is derived from the
  module directory (GetModuleFileNameW → strip filename → append `"SaveScan"`) — FUN_1402737f0"
  (`artifacts/SaveScan/rsf-format--scan-session.md` §5.5). DRWUI contains the literal
  string `SaveScan\` (`reversed/DRWUI.exe/strings_ascii.txt`). **Sessions are saved in the
  install directory, not per-user AppData.**
- **Naming**: `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` for auto-saves; enumerator scans
  `auto_*` + `.rsf` in the SaveScan folder (FUN_140273940); user saves via QFileDialog with
  filter `Recovery State File(*.rsf)|*.rsf`; volume-name part sanitized `/ * : ? " < > |` → `_`
  (§5.5). Shipped examples: `SaveScan/auto_Local Disk (G_)10_15_00_44_10.rsf` (7.37 MB),
  `auto_Local Disk (H_)10_14_23_42_58.rsf` (52.7 MB), `auto_Local Disk (I_)10_14_23_13_21.rsf`
  (9.0 MB) — the vendor's own QA sessions of 2026-10-14/15.
- **Format**: container magic `EA F4 B3 1B 05 43 B6 4A B4 B8 24 4A 33 D9 13 BB`; 36-byte
  record headers chaining zlib-chunked payloads; 4 records per session (state counts, volume
  identity + SYSTEMTIME + stats, partition/volume context incl. 512-byte boot-sector
  snapshot, full found-file tree; record-ID GUIDs from .rdata). Fully reversed + validated
  (`artifacts/SaveScan/rsf-format--scan-session.md` §2-§5).
- **UI**: "Save Scan Session" / "Load Scan Session"; `LastScanRecordView` page lists past
  sessions; "(1-Import Scan Result" import-from-file (rsf artifact header + §5.5).
- **Results (recovered files)**: destination is **user-chosen** via
  `ChooseDestinationPathDialog` ("We highly recommend saving the found data to a different
  drive or a network drive"); destination validated by opening `\\.\X:` raw volume
  (writability + same-volume detection); failures reported via a recovery log
  (`architecture/04-recovery-workflow.md` §7). Cloud destinations exist for cloud scans
  (onedrive_cloud_stream / ICloudDriveUploadTask, quota-checked). **No default
  recovery-destination folder is documented in the corpus** (config.ini
  `FileRepairSavePath=` ships empty).

## 8. Logs (every documented class and its location)

All logs are **module-dir-relative** (`<exe_dir>\log\...`) unless noted:

| Log | Writer | Location + citation |
|---|---|---|
| `DRWLog.log` | DRW.exe (DRWLoader): 5-second flusher copies the 2 MB shared mapping `DRW_{1E956C11-...}` into `<exe_dir>\log\DRWLog.log` (CreateDirectoryA/CreateFileA CREATE_ALWAYS) | `artifacts/DRW-exe--launcher.md` §1-2 |
| `DRWUI.log` | DRWUI main log; shipped QA copy has the build-fingerprint line | `<exe_dir>\log\DRWUI.log` — DRWUI strings `%s/log/DRWUI.log`, `%s\log\DRWUI.log` (`reversed/DRWUI.exe/strings_ascii.txt`); `artifacts/config-files/qa-logs--session-forensics.md` |
| `DefaultLog.txt` | Tree-builder stage machine (`CDirectoryTreeBuilder`, 3-branch tree creation) | `<exe_dir>\log\DefaultLog.txt` — qa-logs artifact §4 + `DRWReport.ini` File003 |
| `CalcPartition.log` | Partition-search (PartitionRecoveryD candidate rounds) | `<exe_dir>\log\CalcPartition.log` — qa-logs artifact §3; DRWUI string `\log\CalcPartiton.log` (typo'd variant in binary) |
| `DelFat.log`, `BuildRaw.log` | referenced by crash-reporter manifest | `log\DelFat.log`, `log\BuildRaw.log` — `DRWReport.ini [FilesToAdd] File001/File010` |
| `CHKLog.log`/`CHKlog.log`, `FileNodeDataRun.log`, `FileStart.log`, `euAuxiliaryBitmaplog.log`, `euFileMatchinglog.log`, `euMajorKeylog.log`, `eufatlog.log` | engine/FS-check logs | `\log\...` string census in `reversed/DRWUI.exe/strings_ascii.txt` |
| `Drwdump0-5.DMP` (a.k.a. `%s/Drwdump%d.dmp`) | crash minidumps, rotating pair per crash-reporter | **app dir root** — `DRWReport.ini [FilesToAdd] File004-009`; DRWUI string `%s/Drwdump%d.dmp`; `artifacts/DRWReport-exe--crash-reporter.md` §role ("rotating pair, app dir") |
| `EPMLOG.log` | PartitionRecovery device-manager boot trace (CDeviceManagerBuilder) | install root (shipped QA leftover) — `artifacts/config-files/vendor-logs--qa-session-forensics.md` §1 |
| `Eaolog.log` | CEaoActiveOnline (activation) init log | install root — same artifact §2; file read: "2026-10-14 22:57:02 [T/P:18688/2116] [Liaolanjun] CEaoActiveOnline::Init success." |
| `strutureReg.log` | video-repair regroup JSON debug logger | install root — same artifact §3 |
| `reglog_win.txt` | video-algorithm dev traces ("integrationalg.cpp[444] m_progress is set...") | install root — direct read |
| `icloud_logger` | iCloud client log | install root — direct read |
| `/tmp/log/FileStart.log`, `/tmp/log/euAuxiliaryBitmaplog.log`, `/tmp/log/euFileMatchinglog.log` | NAS agent on-box logs (written on the NAS under /tmp/EaseUS) | `artifacts/EuShare/eumrx-nas-architecture.md` §3.3 |
| `DFRLog.log`, `PRLog.log` | name-only strings in DRWUI (writer/role not decoded) | `reversed/DRWUI.exe/strings_ascii.txt` |
| `log/testData.log` | test-only path string | same |
| Shared log plumbing | `PubLog.dll` exports InitLog/Log/LogError/Info/Warning + classes `CFileMappingLog` (the 2 MB mapping class) / `CDirectLog` | `artifacts/per-binary/PubLog.md` |
| Upload path | dumps/logs → `send_dmp_with_aliyun.exe` → Aliyun OSS per `AliyunConfig.ini` (LOGENDPOINT/PROJECTNAME encrypted; AllowSendUrl `track.easeus.com`) | `AliyunConfig.ini` direct read + `artifacts/DRWReport-exe--crash-reporter.md` |

## 9. WinPE bootable-media layout (secondary product copy)

From `architecture/03-winpe-boot-environment.md` (full doc) + git trees +
`artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md` (180-file census) +
`artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md`:

```
BUILDPE/
├── EaseUS-x64/            (180 files — staging tree the media builder injects into the WIM)
│   ├── DRW.exe, DRWUI.exe, DRWService.exe, DRWReport.exe, all 9 SearchDLLs, repair tools,
│   │   config/ (9 files), language/, Resource/, platforms/ (incl. PE-only qoffscreen), ...
│   └── NO: PartitionRecovery/ EuShare/ NasConfig/ License/ SaveScan/ EUFileRepair/ VideoViewer/ diskhealth/
└── x64/                   (14 files — PE OS overlay)
    ├── auto.bat  regdrv.reg  software.reg  wallpaper.bmp
    └── Windows/
        ├── system32/  ebrntdrv.sys  epmdkdrv.sys  msi.dll  oledlg.dll  winpeshl.ini  Unattend.xml
        ├── SysWOW64/ msi.dll  oledlg.dll
        └── drivers/   EUDCPEPM.sys  EUEDKEPM.sys
```

- Deploy target on the booted media: `%systemdrive%\Program Files\easeus\DRW\DRW.exe`
  (winpeshl.ini launch; auto.bat "Data Recovery Wizard" desktop shortcut →
  `x:\program files\EaseUS\DRW\DRW.exe`) — `architecture/03` §2, `boot-config` artifact
  lines 18/137.
- Boot sequence: wpeinit → ChangeRes setmax → launch.exe auto.bat (~30 .reg imports,
  Chrome-as-viewer registration, shortcuts, OEM branch via `HKLM\SOFTWARE\EASEUS\EPM\Version\OEM`)
  → DRW.exe (same loader supervision as live OS) → launch.exe 2 keep-alive.
- PE-vs-live diff (census §2-§3 + `architecture/03` §6): PE-only = EuSendMessage.exe,
  d3d8thk/d3d9/d3d11/dxgi shims, qoffscreen, mfc90u; live-only = setup trio
  (SetupUE/unins000/InfoForSetup), 7za.exe, Aliyun channel, cloud_session.dll +
  cpprest141_2_10.dll, EuUserRate.dll, msvcm90.dll; live-only **directories** (8):
  `PartitionRecovery/` (31 .mo + 19 DLLs; zero .mo anywhere under BUILDPE), `EuShare/`,
  `NasConfig/`, `License/`, `SaveScan/` (zero .rsf on media), `EUFileRepair/`,
  `VideoViewer/`, `diskhealth/`. Consequences: bootable edition cannot do lost-partition
  recovery (no MOM modules), no NAS redirector, no licensing blobs (FREE by construction),
  self-carried UCRT (49 forwarders + ucrtbase).
- Media builder in the live product: `PE_Tools.dll` + `PEtools.xml` + `dlconfig.xml` +
  `EuDownloader.exe` + `Burn.dll`; downloads Microsoft WAIK supplement ISO and carves
  boot files by byte-range; BUILDPE/EaseUS-x64 is the injected payload.

## 10. UNVERIFIED-BY-CORPUS (layout questions the corpus text does not answer)

1. **Exact live-OS install directory full path** — no literal `C:\Program Files\EaseUS\<DRW-folder>`
   string exists; `C:\Program Files\EaseUS` (bitlockerhelp.mo) + `\Program Files\EaseUS\DRW`
   (PE-builder strings) converge on `C:\Program Files\EaseUS\DRW` but the live subfolder
   name is not byte-proven. Inno `unins000.dat` would contain {app} but is not decoded in
   the corpus. → D2 installer evidence may resolve.
2. **Any ProgramData (`C:\ProgramData\EaseUS\...`) content** — only a bare "ProgramData"
   token in DRWUI activation code; no composed path, no documented files.
3. **Hive (HKLM vs HKCU) for each `SOFTWARE\EASEUS\DRW\*` subkey** (Language=HKLM per
   launcher doc; `EaseUS Data Recovery Wizard\EUImgRepair`=HKCU per image-repair artifact;
   UserRecovered/UserGuidWnd/ChrisWnd hive not stated).
4. **Default recovery-destination folder** (recalled files) — destination is
   user-chosen via ChooseDestinationPathDialog; no default path in corpus.
5. **Whether logs/sessions/DRWCfg.bin are ALSO mirrored to any per-user or shared
   location** — corpus shows only module-dir-relative writes (log\, SaveScan\, app root);
   no alternate-location fallback documented.
6. **Where activation/license data files live on disk** (registry vs `%s\Configure.dat` vs
   ProgramData) — "writeActivateData (registry/files)" in the licensing artifact is not
   path-specific; Configure.dat location is module-dir-relative but its content/format is
   not decoded.
7. **`auto.txt` content/purpose** (GUID-tagged 316-byte blob) — not decoded anywhere.
8. **`remote_config_data` blob layout** (Firebase remote-config snapshot; 61 KB) — role
   known, format not decoded in corpus docs.
9. **`VideoViewer/sfc` (211,614-byte data file) role** — not documented in artifacts.
10. **`lostpart.dat` / `partinfo.dat` formats** — shipped in PartitionRecovery/ but not
    decoded (unlike raw_signs.json/EUUserData which are).
11. **`Japanese.dll` vs missing Chinese.dll/Deutsch.dll** — launcher lists three language
    fallback DLLs but only Japanese.dll ships; whether the others are download-time or
    build-variant artifacts is undocumented.
12. **PE drivers' absence on live systems**: architecture/02 asserts EUDCPEPM/EUEDKEPM are
    "NOT loaded during normal Windows operation" — no live-OS install path for them is
    documented (consistent, but absence can't be byte-proven).
13. **DiskHealth.exe scheduled-task installation** (addtask/deltask/querytask.bat exist;
    what task name/trigger they register is not decoded).
14. **`EUHwpViewer`/`EUOfficeViewer`/`PDFViewer` working-dir assumptions** (they're
    launched with `%s\`-prefixed absolute paths, so likely self-contained, but no
    per-viewer layout doc).
15. **Whether `SaveScan\` auto-cleans or caps old sessions** — enumeration is documented
    (auto_* + .rsf) but retention policy is not.
