# REFERENCE-INSTALLER.md — EaseUS Data Recovery Wizard (DRW): Installer Behavior Record

> Task 2-d2 (Agent D2). READ-ONLY analysis of the reference corpus at
> `/home/z/my-project/work/easeus-drw-research` (commit 03ad46c). No repository file modified.
> Scope: installer technology, install sequence, elevation, registry writes, service/driver
> registration, uninstall/update/repair. Installed-directory layout is Agent D1's
> (REFERENCE-LAYOUT.md); the rebuilt rda.sys deployment contract is Agent C's
> (DRIVER-DEPLOYMENT.md) — referenced only where the two must not diverge.
>
> **Citation convention**: paths are relative to the corpus root
> (`/home/z/my-project/work/easeus-drw-research/`) unless absolute. Strings are quoted
> **verbatim** from the cited file/line. Every claim carries its corpus cite; the
> UNVERIFIED section lists what the corpus text does *not* answer.

**Reference product identity**: EaseUS Data Recovery Wizard for Windows, build stamped
`versionNumber=21.0.0.0`, `product-code=RE-DRW-FREE-WIN`, `channelName=easeus`,
`marketName=abroad` (`og-codebase/config/version.ini`); upgrade manifest says
`Product Version=20.8.0.0` (`og-codebase/Upgrade.ini`); DRWUI embedded build banner
"Oct 13 2026 13:33:43" (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:23235-23236`);
main EXEs built 2026-10 (`architecture/01-build-tree-intelligence.md:88`).

---

## 1. Installer technology

**The installer is Inno Setup (5.5.x) — NOT NSIS, NOT MSI.** The task brief assumed
NSIS ("unins000.exe naming is itself NSIS evidence"); the corpus strings disprove that
assumption — `uninsNNN.exe` + `uninsNNN.dat` + `uninsNNN.msg` is the **Inno Setup**
uninstaller signature, and the binary self-identifies as Inno repeatedly:

- `re-workspace/reversed/unins000.exe/strings_ascii.txt:3365-3368`:
  `Inno Setup Setup Data (5.5.7) (u)` / `Inno Setup Uninstall Log (b)` /
  `Inno Setup Uninstall Log (b) 64-bit` / `Inno Setup Messages (5.5.3) (u)`
- `re-workspace/reversed/unins000.exe/strings_utf16.txt:1255-1260`:
  `Inno Setup version 5.5.9 (u)` / `Copyright (C) 1997-2016 Jordan Russell` /
  `Portions Copyright (C) 2000-2016 Martijn Laan` / `http://www.innosetup.com/`
  (also `Setup version: Inno Setup version 5.5.9 (u)` at :1244, :1353)
- Embedded manifest: `name="JR.Inno.Setup"` … `<description>Inno Setup</description>`
  (`re-workspace/reversed/unins000.exe/rabin2_strings_data.txt:2922`)
- 64-bit installs from the 32-bit uninstaller go through the Inno helper pipe
  `\\.\pipe\InnoSetup64BitHelper-%.8x-%.8x-%.8x-%.8x%.8x`
  (`re-workspace/reversed/unins000.exe/strings_utf16.txt:640`; `HELPER_EXE_AMD64` at
  `strings_ascii.txt:1420`)
- `re-workspace/artifacts/per-binary/unins000.md:28`: "**Uninstaller.** Inno Setup
  uninstaller (3rd-party generated, customized)." (x86/1,259,408 B, 298 imports/12 DLLs,
  Authenticode-signed per `rabin2_info.txt:17 signed true`)
- `architecture/01-build-tree-intelligence.md:84`: "unins000.exe: 2016 (Inno Setup
  uninstaller, VS2008-era)" — i.e. the uninstaller runtime is ~10 years older than the
  2026 main build, a frozen installer-generation constant.
- `re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:15`:
  "unins000.exe + unins000.dat + unins000.msg — the **Inno Setup uninstaller trio**
  (unins000 naming + .dat/.msg data files are Inno's signature) — the installer itself
  is Inno Setup-based, with SetupUE as a pre-install bootstrap …"

No MSI database, no `msiexec`, no WiX/Orca artifacts anywhere in the corpus (negative
result of the mandated greps over `re-workspace/artifacts/` + `architecture/`).

### Bootstrap chain (SetupUE.exe → InfoForSetup.exe; EuSendMessage.exe → installer window)

The corpus ships a two-EXE installer-side bootstrap beside the Inno uninstaller at the
install root (`og-codebase/SetupUE.exe`, `og-codebase/InfoForSetup.exe`,
`og-codebase/unins000.{exe,dat,msg}`):

1. **SetupUE.exe** — x86 GUI, 177,384 B, 24,545-line decompile
   (`re-workspace/artifacts/per-binary/SetupUE.md`). Role: "Installation/uninstall, geo
   detection, retention triggers" (per-binary/SetupUE.md:40); capa: "get geographical
   location … get OS version, check OS version" (SetupUE.md:25).
   - **16-OS gate**: full Windows-version detection table "Microsoft Windows 2000"
     through "Server 2016" incl. Standard/Enterprise/Datacenter/Web/Compute Cluster/
     Storage/Itanium/x64 edition variants, via native `GetNativeSystemInfo` +
     `RtlGetNtVersionNumbers` (WOW64-aware) — "the bootstrap gates installation on OS
     version" (`re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:11`).
   - **Registry contract**: `SOFTWARE\EASEUS\DRW\Version` — "the installer writes/reads
     the product version key" (bootstrap-pair.md:12); verbatim in
     `re-workspace/reversed/ghidra-core/SetupUE_decompiled.c:1833`.
   - **Spawns InfoForSetup.exe** (literal `_wcscat_s(…, L"InfoForSetup.exe")`,
     `SetupUE_decompiled.c:1769`) with the telemetry verb
     `/SendInfo "Window" "Install" "Activity" "Info_Userinfo" "Attribute" "<JSON>" "vid"
     "<Testid>"` (`SetupUE_decompiled.c:1843-1851`), where the JSON embeds
     `"Country"`, `"Timezone"`, `"OS"`, `"Test_id"` fields built at
     `SetupUE_decompiled.c:1818-1837` (country/timezone/OS getters) — the A/B-test
     bucket id (`Testid`/`vid`) is read from `SOFTWARE\EASEUS\DRW\Version`
     (`SetupUE_decompiled.c:1832-1833`).
   - **Owns the `DRW_Installer` window** — "the titled window that EuSendMessage.exe
     targets with WM_COPYDATA command 999" (bootstrap-pair.md:14).
2. **InfoForSetup.exe** — x64 GUI, 117,776 B, 13,717-line decompile
   (`re-workspace/artifacts/per-binary/InfoForSetup.md`): "Pre-install environment
     probing" (InfoForSetup.md:35); install-time telemetry fields
     `Activity`/`Attribute`/`Country`/`Timezone`, `/SendInfo "..."`, `Info_Userinfo`,
     `Install`, `Test_id`/`Testid` (bootstrap-pair.md:20-23) — "the installer
     participates in the product's experiment framework".
3. **EuSendMessage.exe** — *PE-build-only* one-shot notifier: concatenates argv,
   converts to ANSI, `FindWindowW(NULL, L"DRW_Installer")`, then
   `SendMessageW(WM_COPYDATA)` with `dwData=999`
   (`re-workspace/artifacts/WinPE/eusendmessage--wm-copydata-messenger.md:26-31`).
   "The window title identifies the **DRW installer wizard** (SetupUE.exe-family —
   the live-tree installer that the PE build deliberately omits)" (same file §3).
   PDB: `E:/AWork/DRW_MAIN/EuSendMessage_toInstaller/Release`
   (`architecture/01-build-tree-intelligence.md:72` — note the project name
   "EuSendMessage_toInstaller").

Install-flow synthesis as documented by the corpus
(`re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:27-34`):

```
user runs setup (Inno Setup wizard)
  └─ SetupUE.exe (bootstrap): OS-version gate (16-OS table, native APIs)
       ├─ reads/writes SOFTWARE\EASEUS\DRW\Version
       ├─ spawns InfoForSetup.exe (/SendInfo telemetry: Activity/Country/Timezone/TestId)
       └─ owns "DRW_Installer" window (EuSendMessage WM_COPYDATA cmd-999 target in PE contexts)
  └─ Inno Setup proper: copies tree, writes unins000.{exe,dat,msg}
  └─ PE edition alternative: PE_Tools/WAIK flow (no SetupUE involved — PE never installs)
```

**Caveat**: the Inno *setup* EXE itself (the wizard that generated `unins000.dat`) is
**not present** in the corpus — only its bootstrap helpers and uninstaller are shipped
(see UNVERIFIED §10).

### Bootable-media "installer" (the PE_Tools builder)

The bootable Emergency Disk is not installed by the Inno installer; it is built by the
live product: `PE_Tools.dll` (51 RTTI classes) + `PEtools.xml` + `dlconfig.xml` +
`EuDownloader.exe` implement "Create Emergency Disk":
"1) download WAIK objects (EuDownloader, resume + MD5 verify), 2) stage WAIK tree,
3) mount/customize WINPE.WIM (DISM APIs), 4) inject DRW app tree (BUILDPE/EaseUS-x64),
drivers (regdrv.reg + .sys files), auto.bat shell config, 5) write ISO/USB (Burn.dll —
IMAPI2)" (`architecture/03-winpe-boot-environment.md:61`). `dlconfig.xml` carves
BOOTMGR/WINPE.WIM/BCD/BOOT.SDI/BOOTX64.EFI/font CABs out of Microsoft's official
`waik_supplement_en-us.iso` by byte-range + MD5 (`architecture/03:59`,
`og-codebase/dlconfig.xml`); PE variants winpe3/4/5/win10pe x86+amd64
(`architecture/03:62`).

---

## 2. Install sequence

Documented order (corpus-synthesized; each step cited):

1. **SetupUE.exe bootstrap**: OS-version gate (16-OS table) — "gates installation on OS
   compat and collects info BEFORE the Inno wizard runs"
   (`re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:15`).
2. **Install telemetry**: SetupUE → `InfoForSetup.exe /SendInfo "Window" "Install"
   "Activity" "Info_Userinfo" "Attribute" "<Country/Timezone/OS/Test_id JSON>" "vid"
   "<Testid>"` (`re-workspace/reversed/ghidra-core/SetupUE_decompiled.c:1843-1851`).
3. **Inno wizard copies the tree** and writes `unins000.{exe,dat,msg}`
   (bootstrap-pair.md:33). The Inno runtime registers its standard data: uninstall key
   (§3), `Inno Setup: App Path`, `Inno Setup: Icon Group`, `Inno Setup: Selected
   Components/Tasks`, `Inno Setup: User Info: Name/Organization/Serial`, `Inno Setup:
   Language`, plus icons ("Creating the icon."), INI entries, RunOnce registrations and
   font-key writes as applicable
   (`re-workspace/reversed/unins000.exe/strings_utf16.txt:774-845, 943-978`).
   Shipped side-by-side-manifest carriers `Microsoft.VC90.CRT.manifest` +
   `Microsoft.VC90.MFC.manifest` at the install root (`og-codebase/`) and "VC runtimes
   9.0 (VS2008!) through 14.0 … mixed CRT generations in one install tree"
   (`architecture/01-build-tree-intelligence.md:81`).
4. **Elevation model is requireAdministrator at product level**: DRW.exe manifest =
   `<requestedExecutionLevel level='requireAdministrator' uiAccess='false' />`
   (`re-workspace/reversed/DRW.exe/strings_ascii.txt:355`,
   `rabin2_strings_data.txt:1183`) and DRWUI.exe carries the identical manifest
   (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:41023`,
   `rabin2_strings_data.txt:1728`). So the entire product chain (DRW.exe loader →
   DRWUI.exe child via `CreateProcessA`, `re-workspace/artifacts/DRW-exe--launcher.md`
   §2) runs elevated — UAC at every launch, not only at install.
5. **First launch (post-install) installs the runtime service on demand**: DRWUI
   strings `\DRWService.exe /install` / `\DRWService.exe /uninstall` (twice)
   (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:25749-25753`;
   `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:430709,430808`) —
   "`DRWService.exe` — a separate, on-demand service … installed/uninstalled via
   `\DRWService.exe /install` / `/uninstall` (strings in DRWUI.exe)"
   (`architecture/02-kernel-driver-architecture.md:104`).
6. **First raw-disk scan installs the driver on demand**: "`setupdrvx64.exe`
   (mod.WinDiskAccessDriverSetup) installs+starts eprdtdrv service on first scan
   needing raw access"
   (`re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:96`);
   "Kernel drivers … installed on demand by setupdrvx64.exe"
   (`architecture/01-build-tree-intelligence.md:98`).
7. **WinPE edition never installs**: "PE_Tools/WAIK flow (no SetupUE involved — PE
   never installs)" (bootstrap-pair.md:34); the setup trio is live-only
   (`architecture/03-winpe-boot-environment.md:72`: "Live-only binaries (15): setup
   trio (SetupUE/unins000/InfoForSetup) …").

What requires admin, per corpus: SCM operations (setupdrvx64: `OpenSCManagerA` +
`CreateServiceA` imports,
`re-workspace/reversed/PartitionRecovery/setupdrvx64.exe/rabin2_imports.txt:90,93`;
"setupdrvx64.exe writes to System32\Drivers via SCM with admin rights",
`eprdtdrv-sys--driver-internals.md:90`), DRWService install
(`OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS 0xF003F)` → `CreateServiceA(…
SERVICE_ALL_ACCESS)`, `re-workspace/artifacts/DRWService-exe--bitlocker-service.md`
§1), HKLM writes (`SOFTWARE\EASEUS\DRW\…`), and the product EXEs themselves
(requireAdministrator manifests above). The Inno uninstaller checks elevation at
runtime: string `Uninstaller requires administrator: %s`
(`re-workspace/reversed/unins000.exe/strings_utf16.txt:916`).

---

## 3. Registry writes

### 3.1 Uninstall key (Inno)

Exact key paths carried by the uninstaller runtime (verbatim):

- `Software\Microsoft\Windows\CurrentVersion\Uninstall` +
  `%s\%s_is1`
  (`re-workspace/reversed/unins000.exe/strings_utf16.txt:772-773` and duplicates at
  `:829, :1038-1039`; used in code at
  `re-workspace/reversed/ghidra-misc/unins000_decompiled.c:131320,136521`)
  — the classic Inno layout
  `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{AppId}_is1`.
- Value names the Inno runtime writes/reads (verbatim list, strings_utf16.txt
  :830-868): `InstallLocation`, `DisplayName`, `DisplayIcon`, `UninstallString`,
  `QuietUninstallString` (with `" /SILENT` suffix, :848-850), `DisplayVersion`,
  `Publisher`, `URLInfoAbout`, `HelpTelephone`, `HelpLink`, `URLUpdateInfo`, `Readme`,
  `Contact`, `Comments`, `ModifyPath`, `NoModify`, `NoRepair`, `InstallDate`,
  `MajorVersion`, `MinorVersion`, `VersionMajor`, `VersionMinor`, `EstimatedSize` —
  plus the `Inno Setup: *` bookkeeping values (:830-845).
- **The DRW-specific `{AppId}` is NOT recoverable from the corpus**: it lives in the
  binary `unins000.dat` log (not decoded; the corpus ships it as a binary artifact
  `og-codebase/unins000.dat`). Two product-adjacent `_is1` keys ARE visible verbatim
  in DRWUI.exe strings:
  - `SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{201F83A9-F234-4144-871A-B65A3E6586CF}_is1`
    (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:23783`) — DRWUI opens it
    HKLM (`0xffffffff80000002`) and reads `DisplayVersion`
    (`ghidra-drwui/DRWUI_decompiled.c:268676-268682`) to version-check the embedded
    **FoxitRestore** component (adjacent strings
    `SOFTWARE\Foxit Software\FoxitRestore\Foxit Information`, `FoxitRestoreUpdate.exe`
    — `strings_ascii.txt:23777-23780`, `DRWUI_decompiled.c:268629`).
  - `{Tenorshare 4DDiG}_is1` (`DRWUI_decompiled.c:523795`;
    `strings_utf16.txt:14541`) — **competitor-detection** enumeration: DRWUI walks
    the subkeys of `SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` **and**
    `Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall`
    (`DRWUI_decompiled.c:523888,523892`), matching installed products by `_is1` key
    name and reading `ParentKeyName` (`:523790-523808`).

### 3.2 Product keys (HKLM\SOFTWARE\EASEUS\DRW…)

| Key | R/W | Who (evidence) |
|---|---|---|
| `SOFTWARE\EASEUS\DRW\Version` | write+read (installer), read (runtime) | SetupUE `SetupUE_decompiled.c:1833`; DRW.exe loader `ghidra-core/DRW_decompiled.c:869,1076`; DRWUI ≥12 sites incl. `RegOpenKeyExA(HKLM, "SOFTWARE\\EASEUS\\DRW\\Version")` `DRWUI_decompiled.c:554074` and value **`InstallParam`** read `DRWUI_decompiled.c:1561997` (`strings_utf16.txt:6461`); crash uploader `ghidra-misc/send_dmp_with_aliyun_decompiled.c:1766`; bootstrap-pair.md:12 "the installer writes/reads the product version key" |
| `SOFTWARE\EASEUS\DRW\Version\Testid` (a.k.a. `vid`) | read | SetupUE A/B id for `/SendInfo … "vid"` (`SetupUE_decompiled.c:1832-1851`) |
| `HKEY_LOCAL_MACHINE\SOFTWARE\EaseUS\DRW\Retention` | r/w (product) | DRWUI `strings_ascii.txt:9790` (and :9837, 9927, 10256, 10618, 11794, 12801, 15637, 16193, 16247, 16257, 20661, 20664) — win-back/retention state |
| `SOFTWARE\EASEUS\DRW\` + `SNExpired`, `register` | read | license/serial expiry + registration flags — `strings_ascii.txt:23185-23186, 23249-23253` |
| `SOFTWARE\EASEUS\DRW\ChrisWnd\` (`NeedTip`) | r/w | `strings_ascii.txt:23261-23265` |
| `SOFTWARE\EASEUS\DRW\UserGuidWnd\` (`NeedTip`) | r/w | `strings_ascii.txt:23322-23326` |
| `SOFTWARE\EASEUS\DRW\UserRecovered\` | r/w | `strings_ascii.txt:23332-23334` |
| `SOFTWARE\EASEUS\DRW\Language` (value `Language`) | read | DRW.exe launcher: "HKLM\SOFTWARE\EASEUS\DRW\Language → 'Language' value (RegOpenKeyExA line 311)" `DRW-exe--launcher.md:19`; DRWReport reads the same key (`re-workspace/reversed/DRWReport.exe/strings_ascii.txt`, SOFTWARE\EASEUS\DRW\Language hit); **PE seeds it**: `software.reg` single entry `HKLM\SOFTWARE\EASEUS\DRW\Language = "English"` (`re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md:124`) |
| `SOFTWARE\EASEUS\EPM\Version` (value `OEM`) | read | WinPE auto.bat Phase D OEM branch: `reg query "HKLM\SOFTWARE\EASEUS\EPM\Version" /v OEM` gates Mainland-vs-international desktop shortcuts (`boot-config--winpeshl-autobat-regdrv.md:59-66`); DiskHealth opens `Software\EASEUS\EPM\Version` (`ghidra-wave2/DiskHealth_decompiled.c:33189`) |

### 3.3 Driver/service keys (live OS)

Written by `setupdrvx64.exe` (EPM `mod.WinDiskAccessDriverSetup`):

- `SYSTEM\CurrentControlSet\Services\%s` — "Registry: `SYSTEM\CurrentControlSet\Services\%s`
  (write + verify)" (`re-workspace/artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md:38`);
  verbatim 4× at `re-workspace/reversed/PartitionRecovery/setupdrvx64.exe/strings_ascii.txt:143,152,167` and
  `rabin2_strings_data.txt:8,20,38`.
- `SYSTEM\CurrentControlSet\Control\SafeBoot\Minimal` +
  `SYSTEM\CurrentControlSet\Control\SafeBoot\Network` — SafeBoot registration carried by
  the deployer (`setupdrvx64.exe/strings_ascii.txt:139-140`,
  `rabin2_strings_data.txt:4-5`) — the driver is registered to load in Safe Mode too.
- Service config values: `DisplayName`, `ErrorControl`, `DeleteFlag` (UTF-16 ×2)
  (`strings_ascii.txt:154-155`, `strings_utf16.txt:1-2`), group `Boot File System`
  (`strings_ascii.txt:141`), error string `CreateService failed!  Error = %d`
  (`strings_ascii.txt:142`), `-Uninstall` mode (`strings_ascii.txt:174`,
  `rabin2_strings_data.txt:45`).
- Storage class keys carried by the deployer:
  `SYSTEM\CurrentControlSet\Control\Class\{71A27CDD-812A-11D0-BEC7-08002BE2092F}` and
  `SYSTEM\CurrentControlSet\Control\Class\{4D36E967-E325-11CE-BFC1-08002BE10318}`
  (`strings_ascii.txt:160-166`, `rabin2_strings_data.txt:31-37`).
  **Flag (cross-check note)**: the corpus labels `{71A27CDD-…}`
  "GUID_DEVCLASS_DISKDRIVE" (`boot-config--winpeshl-autobat-regdrv.md:109`), but the
  standard Microsoft device-class table maps `{4D36E967-…}` = DiskDrive and
  `{71A27CDD-…}` = Volume. Which class receives `UpperFilters` on the *live-OS* path is
  not decoded in the corpus — recorded as UNVERIFIED; do not copy the corpus label
  blindly into the rebuild (the rebuilt rda INF/PS1 writes the disk-drive class
  `{4d36e967-…}` per Agent C's DRIVER-DEPLOYMENT.md).

### 3.4 Service-related keys read at runtime

- `HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock` (+ per-volume-GUID subkeys,
  value `Data`) — enumerated by DRWService to collect BitLocker auto-unlock external
  keys (`DRWService-exe--bitlocker-service.md` §2, decompile lines cited there).
- `HKLM\SYSTEM\CurrentControlSet\Services\EuMrx\Parameters` — EuMrx NAS-redirector
  service parameters (`architecture/02-kernel-driver-architecture.md:94`).
- `System\CurrentControlSet\Control\MiniNT` — read by DRWUI as the WinPE-detection
  marker (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:23224`).

### 3.5 IE/TLS bootstrapper keys (EUinApp.exe)

- Write (upgrade-only, HKCU): `Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION`
  → `<HostExe> = 9000|10000|11000` (IE9→9000, IE10→10000, otherwise 11000 DWORD)
  (`re-workspace/artifacts/UIUX/23-euinapp-ie-emulation-bootstrapper.md:29-52`;
  `Licensing/euinapp--ie-bootstrapper.md:13`).
- Read: `SOFTWARE\Microsoft\Internet Explorer` values `version`/`svcVersion`
  (23-euinapp:30-31). Touch: `Software\Microsoft\Windows\CurrentVersion\Internet
  Settings` (`SecureProtocols` per Licensing artifact:15; "companion check" per
  UIUX 23:44).
- **Orphaned deployment**: "a full-corpus scan … finds **zero references to EUinApp** —
  not from DRW.exe, not from DRWUI.exe, not from SetupUE.exe/InfoForSetup.exe/unins000.exe.
  … Its function … is either performed by the external installer (out-of-tree), …"
  (`UIUX/23-euinapp-ie-emulation-bootstrapper.md:55-61`) — i.e. plausibly an
  install-time step in the out-of-tree installer, unproven (UNVERIFIED).

### 3.6 Inno runtime generic keys (capability, product-agnostic)

`Software\Microsoft\Windows\CurrentVersion\RunOnce` (:965-966),
`Software\Microsoft\Windows\CurrentVersion\App Paths\` (:943),
`Software\Microsoft\Windows\CurrentVersion\Fonts` / `…\Windows NT\CurrentVersion\Fonts`
(:823-824), `InnoSetupRegFile.%.10d` (:967), uninstall hive roots enumerated by name
(HKEY_CLASSES_ROOT…HKEY_DYN_DATA, :561-567) — all from
`re-workspace/reversed/unins000.exe/strings_utf16.txt`.

### 3.7 CORRECTION — the MOM "RegLib" and "CheckVersion" are NOT installer components

The task brief guessed RegLib = product registry library and CheckVersion =
product update check. The corpus says otherwise — both are **recovery-engine modules**
that parse **offline Windows registry hives** of the patient disk:

- `RegLib.mo` — "registry HIVE parser … reads Windows state without booting it"
  (`re-workspace/artifacts/MOM-modules/RegLib.md:1-4`; class census `Hive`, `Key`,
  `Node`, `IndxLeaf/FastLeaf/HashLeaf`, `BigData` — the on-disk hive format types).
- `CheckVersion.mo` — "Windows installation detection … finds installed Windowses in
  offline hives for boot repair"; strings `\\$$$PROTO.HIV`, `\\CONTROLSET00`,
  `\\CONTROL\\PRODUCTOPTIONS\\PRODUCTTYPE`, `\\SELECT\\CURRENT`
  (`MOM-modules/CheckVersion.md:1-45`).

Neither writes installer/product keys. Product update behavior lives in DRWUI's
`CVersionCtrl` (§6 below).

---

## 4. Services and drivers registered

### 4.1 DRWService.exe — "Read FVE Server" (BitLocker FVE key service)

Exact registration (decompile-cited, `DRWService-exe--bitlocker-service.md` §1 +
verification ledger §6):

```
OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS 0xF003F)
CreateServiceA(scm,
   lpServiceName  = "Read FVE Server",
   lpDisplayName  = "Read FVE Server",
   dwDesiredAccess= SERVICE_ALL_ACCESS (0xF01FF),
   dwServiceType  = 0x10  = SERVICE_WIN32_OWN_PROCESS,
   dwStartType    = 2     = SERVICE_AUTO_START,        ← auto-start at every boot
   dwErrorControl = 1     = SERVICE_ERROR_NORMAL,
   lpBinaryPath   = <full path of DRWService.exe>)
StartServiceA(svc)                     ← started immediately after creation
```

- Description string: `"DRW read registry key value."`; uninstall =
  `OpenSCManager → OpenService → Control(SERVICE_STOP) → DeleteService` with log
  strings `Service stopping...`, `Uninstall service succeed/failed`
  (artifact §1). SCM lifecycle strings verbatim in
  `re-workspace/reversed/DRWService.exe/strings_ascii.txt:36-57`:
  `Service already installed`, `CreateService failed`, `StartService failed`,
  `Service not installed`, `OpenService failed`, `QueryServiceStatus failed`,
  `Stop Service failed`, `Uninstall service succeed/failed`, `/install`,
  `/uninstall`, `StartServiceCtrlDispatcher failed`; imports
  `OpenServiceA/ChangeServiceConfig2A/StartServiceA/ControlService/DeleteService/
  RegisterServiceCtrlHandlerA/SetServiceStatus/CreateServiceA` (:109-120).
- Runs as **LocalSystem**; pushes the FVEAutoUnlock key table over named pipe
  `\\.\Pipe\DRWBitlocker` (service = pipe CLIENT, one-shot message, 0x138-byte
  records: 50-char GUID + 256-byte key blob, ≤100 records) — artifact §3-4.
- Note the corpus' own nuance: the artifact's rationale says the service exists "so a
  standard-user DRW instance can obtain the keys without UAC" (artifact §0/§4), while
  DRWUI's manifest is requireAdministrator (§8 below) — the service additionally
  exists because FVEAutoUnlock is SYSTEM/admin-ACL'd.
- Install/uninstall is **DRWUI-initiated at runtime** (`\DRWService.exe /install`,
  `\DRWService.exe /uninstall` strings in DRWUI.exe, §2 item 5) — *and* the
  architecture doc lists it under "Driver installation lifecycle (user-mode
  orchestration)" step 2 (`architecture/02-kernel-driver-architecture.md:104`),
  i.e. installed when BitLocker volumes are first encountered, not by the Inno wizard
  (no Inno-side evidence — UNVERIFIED whether the installer also pre-installs it).

### 4.2 eprdtdrv.sys — EPMNtDrv raw-disk driver (live OS, on demand)

- Deployer: `PartitionRecovery/setupdrvx64.exe` (x64, 174,024 B, PDB
  `D:\Project_Application\_EPM_Main_\mod.WinDiskAccessDriverSetup\setupempdrv03\x64\Release\setupempdrvx64.pdb`,
  `re-workspace/artifacts/per-binary/setupdrvx64.md` + `rabin2_info.txt:13`).
  "Installs EPMNtDrv drivers (x64)" (setupdrvx64.md:28).
- Full SCM lifecycle: "`CreateService`/`OpenService`/`StartService`/`ControlService`
  with per-call error strings ('CreateService failed! Error = %d')"
  (`deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md:35`).
- **Copy destinations, verbatim** (`setupdrvx64.exe/strings_ascii.txt:151,153,168-170`):
  `\System32\Drivers\%s.sys`, `\SystemRoot\System32\drivers\%s.sys`,
  `%s\System32\Drivers\%s.sys`, and the numbered variant
  `%s\System32\Drivers\%s%d.sys` — "**the `%s%d` numbered variant is the driver
  versioning scheme** (eprdtdrv2.sys-style coexisting versions)"
  (deepening-batch1:36). Device names are composed at runtime from the service name
  (`\Device\<UPPERCASE(service-name)>`, fallback `EPMNTDRV`), so deploying
  `eprdtdrv2` creates `\Device\EPRDTDRV2` — "multiple instances coexist with
  independent device names, enabling **in-place driver upgrades without rebooting**"
  (`re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md:34-39`).
- Service config: group `Boot File System`, values `DisplayName`/`ErrorControl`/
  `DeleteFlag`, `-Uninstall` mode (deepening-batch1:37-38).
- Start mode: demand — "installed+starts eprdtdrv service on first scan needing raw
  access" (`eprdtdrv-sys--driver-internals.md:96`); in WinPE regdrv the family is
  Start=3 DEMAND with group "Boot File System" (§4.4). Access control is the
  PID-token handshake (`\\.\EPRDTDRV\<pid-token>\<disk>\<part>` /
  `\Volume\<N>`) + disk.sys stack verification, **not** a device ACL
  (`eprdtdrv-sys--driver-internals.md:90`; `epmntdrv-trio--naming-pid-handshake.md`
  §3-§4).
- At app exit / driver uninstall: "service is stopped and deleted"
  (`architecture/02-kernel-driver-architecture.md:105`).

### 4.3 ebrntdrv.sys / epmdkdrv.sys — WinPE spare instances of the same driver

- Byte-identical family (byte-diff 2,907/9,703 bytes, register allocation variance
  only — `epmntdrv-trio--naming-pid-handshake.md:127`);
  `og-codebase/BUILDPE/x64/Windows/system32/ebrntdrv.sys` and `epmdkdrv.sys`
  (27,272 B each, PDB epmntdrv.pdb — `architecture/02-kernel-driver-architecture.md:13-14`).
- Purpose: "The WinPE environment carries **two spare instances of the same driver**
  under different service names: if the primary instance is blocked …, the PE host
  (DRWLoader/Device.mo retry logic …) can start `ebrntdrv`/`epmdkdrv` instead and open
  `\\.\EBRNTDRV\…` / `\\.\EPMDKDRV\…` without a name collision" (trio artifact §5;
  failover policy marked INFERRED there).

### 4.4 EUDCPEPM.sys / EUEDKEPM.sys — WinPE hook-family pair (pre-registered, not SCM-installed)

Exact `regdrv.reg` rows (UTF-16 decoded, hex-verified —
`re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` §3):

| Service | Type | Start | ImagePath | Extra values |
|---|---|---|---|---|
| `epmdkdrv` | 1 KERNEL_DRIVER | 3 DEMAND_START | `\SystemRoot\System32\epmdkdrv.sys` | — |
| `ebrntdrv` | 1 KERNEL_DRIVER | 3 DEMAND_START | `\SystemRoot\System32\ebrntdrv.sys` | `Group="Boot File System"` |
| `EUDCPEPM` | 1 KERNEL_DRIVER | **0 BOOT_START** | `system32\drivers\EUDCPEPM.sys` | `DeviceName="epmdiskcopydrv"`, `IoctlBase=0x10000` |
| `EUEDKEPM` | 1 KERNEL_DRIVER | 1 SYSTEM_START | `system32\drivers\EUEDKEPM.sys` | `DeviceName="EUEDKEPM"`, `IoctlBase=0x10000` |

Plus the class filter
`HKLM\PE_Sys\ControlSet001\Control\Class\{71A27CDD-812A-11D0-BEC7-08002BE2092F}
UpperFilters = REG_MULTI_SZ "EUDCPEPM"` (boot-config §3) — the disk-copy hook attaches
to every disk device in the PE from boot. EUDCPEPM = EuBackup filter (PDB
`d:\epm\_epm_main\sharelib\dc\…\EuBackup.pdb`, `eudcpepm-euedkepm--hook-drivers.md:5`);
EUEDKEPM = EuDskAcs raw-disk door, "deployed-and-registered in the WinPE image but
**dormant** in this product — no in-tree opener" (hook-drivers.md:249-252).
DriverEntry reads `DisplayName`, `DeviceName`, `IoctlBase` from its service key
(`eudcpepm-euedkepm--hook-framework-urgent-cache.md:108-113`), i.e. registration is
registry-value-driven.

### 4.5 EuMrx.sys — NAS redirector (live tree only)

- Service parameters under `HKLM\SYSTEM\CurrentControlSet\Services\EuMrx\Parameters`
  (`architecture/02-kernel-driver-architecture.md:94`); device `\Device\EuRdr`;
  RDBSS mini-redirector; **absent from the PE image** (boot-config §3 placement facts:
  "EuMrx.sys (NAS redirector) is also absent — no NAS mount in PE").
- Install mechanism on live OS is NOT documented in the corpus (no setupdrv/INF for
  EuMrx found) — UNVERIFIED.

### 4.6 How drivers get loaded at runtime (summary)

- **Live OS**: demand-loaded. `setupdrvx64.exe` installs + `StartService`s the service
  at first raw access; thereafter Device.mo opens `\\.\EPRDTDRV\<pid-token>\…`
  (`eprdtdrv-sys--driver-internals.md:96`; trio artifact §3.2). Fallback without the
  driver: Device.mo "opens `\\.\HarddiskVolume%d` directly (GENERIC_READ) — plain
  volume access without the driver … the 'no-admin' degraded mode visible in the UI
  flows" (trio artifact §6).
- **WinPE**: pre-registered by `regdrv.reg` at media-build time; loaded at PE boot via
  the winpeshl.ini chain: `wpeinit.exe` → `ChangeRes.exe setmax` →
  `launch.exe 0 …auto.bat` → `%systemdrive%\Program Files\easeus\DRW\DRW.exe` →
  `launch.exe 2` (keep-alive) (`boot-config--winpeshl-autobat-regdrv.md` §1);
  demand-start drivers are "loaded only when DRW's Device.mo opens
  `\\.\EPRDTDRV\<disk#>\<partition#>` … the product triggers `NtLoadDriver` via the
  service SCM at first raw access" (boot-config:119).

---

## 5. Uninstall behavior

### 5.1 unins000.exe (Inno uninstaller)

What the corpus proves it does (strings + decompile, all from
`re-workspace/reversed/unins000.exe/`):

- Data-driven rollback: reads `Inno Setup Uninstall Log (b)` /
  `Inno Setup Uninstall Log (b) 64-bit` (`strings_ascii.txt:3366-3367`); reports
  `Original Uninstall EXE: `, `Uninstall DAT: `, `Uninstall command line: `
  (`strings_utf16.txt:1354-1356`); "Deleting Uninstall data files." (:1335);
  `Removed all? %s` (:1361).
- File/directory deletion with in-use handling: "The existing file appears to be in
  use (%d). Will replace on restart." + `MoveFileEx` (:884, :987) — delayed deletion
  on reboot.
- **Shared-file accounting**: `TUninstSharedFileForm`, "Remove shared file %s? User
  chose %s%s" (:1332), "Incrementing shared file count (64-bit/32-bit)"
  (:924-925) — SharedDLLs-style refcount prompts.
- Registry: deletes logged keys/values (`RegDeleteKeyExA`, `RegDeleteValueW` in
  imports; `rabin2_imports.txt` advapi block), incl. the uninstall key + `Inno
  Setup: *` values and RunOnce entries (:965-966).
- GAC handling: "Uninstalling from GAC: %s" (:713), `UninstallAssembly` (:684).
- Restart handling: `UninstallNeedRestart`, "Will restart because
  UninstallNeedRestart returned True.", "Restarting Windows." (:1362-1368);
  RestartManager interplay: "RestartManager found an application using one of our
  files: %s" / "Can use RestartManager to avoid reboot? %s (%d)" (:786-787).
- **Silent modes**: `/SILENT`, `/VERYSILENT`, `/NoRestart`, `/SuppressMsgBoxes`
  (:1339-1343); QuietUninstallString ends `" /SILENT` (:849).
- Elevation: "Uninstaller requires administrator: %s" (:916); two-phase elevation via
  `/SECONDPHASE="%s" /FIRSTPHHASEWND=$%x` (:1349); 64-bit helper pipe
  `InnoSetup64BitHelper` (:640) since the shipped uninstaller is x86
  (`rabin2_info.txt: arch x86 / machine i386`).
- Multi-uninstall support: `unins???.*` / `unins%.3d` enumeration (:980-981).
- **What it removes exactly (file list) is inside `unins000.dat` — binary, not
  decoded in the corpus**; whether it removes the driver services/DRWService is
  UNVERIFIED (the corpus documents product-side uninstalls, below).

### 5.2 Product-side uninstalls at runtime (DRWUI / setupdrvx64 / DRWService)

- DRWUI invokes `\DRWService.exe /uninstall` (twice — likely app-exit and
  uninstall-trigger paths) — `DRWUI_decompiled.c:430709,430808`;
  `DRWUI.exe/strings_ascii.txt:25751,25753`.
- setupdrvx64 has a `-Uninstall` mode (`setupdrvx64.exe/strings_ascii.txt:174`);
  "At app exit / driver uninstall, service is stopped and deleted"
  (`architecture/02-kernel-driver-architecture.md:105`).
- DRWService self-uninstall: `Control(SERVICE_STOP) → DeleteService` with
  "Uninstall service succeed/failed" (`DRWService-exe--bitlocker-service.md` §1;
  `DRWService.exe/strings_ascii.txt:46-47`).

### 5.3 Uninstall-time marketing (retention)

- DRWUI string `Uninstall_Retain` (`DRWUI_decompiled.c:217011`); remote-config flags
  `value_uninstallretain172`, `value_inappuninstallretain`,
  `value_uninstallretain__abroad` and trigger `UninstallDlgRetentionTrigger`
  (`DRWUI_decompiled.c:1265608, 1265747, 1610998, 1611154, 1611607`);
  `UninstallDlgRetentionTrigger` class per
  `re-workspace/artifacts/Licensing/licensing-activation-chain.md:45`
  ("UninstallDlgRetentionTrigger" — win-back offer at uninstall).
- AliyunWrap install/uninstall telemetry hooks consumed via GetProcAddress by DRWUI:
  `AliyunInstallConfigFilePath`, `AliyunInstallInitUid`, `AliyunUninstallStart`,
  `AliyunUninstallEnd` (`DRWUI_decompiled.c:497427-497433`) — install/uninstall
  events are instrumented into the Aliyun analytics channel.

---

## 6. Update / upgrade behavior

(CORRECTION carried over from §3.7: the MOM `CheckVersion.mo` is offline-hive
boot-repair logic, **not** the updater. The real update machinery:)

- **DRWUI `CVersionCtrl`** (strings verbatim,
  `DRWUI.exe/strings_ascii.txt:23199-23222`): `CVersionCtrl::doCheckUpdate`; reads
  `%s/Upgrade.ini` (line 23201); reads `config/version.ini` values `version`,
  `versionNumber`, `channelName`, `versionFrom`, `marketName`, `versionCustom`,
  `lastUpdateTs` (update-check throttle), `url_updatecheck`/`UpdateCheckUrl`,
  `url_updatedownload`/`UpdateDownloadUrl`; default endpoints hardcoded:
  `http://recovery.easeus.jp/update/data-recovery-wizard/drw_pro.ini` (update-check
  manifest, line 23206) and
  `http://recovery.easeus.jp/recovery/lp/recoverywizrad_pro_current_autoupdate.html`
  (update-landing page, line 23213); purchase-UtM composition
  `%s&paymentTypeId=%s&PAY_TYPE=%s` (23216). Upgrade dialog class
  `CheckUpgradeDlg` (`DRWUI_decompiled.c:1534094-1534116, 1783408`; UIUX window map
  `UIUX/03-window-class-map.md:125`).
- **`og-codebase/Upgrade.ini`** (verbatim, GBK comment decoded):
  `[VERSION] Product Version=20.8.0.0 / Description=Current version 20.8.0.0 /
  ;下载器，用户确定重装或升级后，是否强杀运行中的DRWUI进程 [downloader: after the
  user confirms reinstall or upgrade, whether to force-kill the running DRWUI process] /
  killDRWUI=1 / URL=http://www.easeus.com/download/data-recovery-software.html /
  NeedPanelViewExpend = 0` — i.e. the updater/downloader compares versions and, with
  user confirmation of reinstall/upgrade, force-kills the running DRWUI (killDRWUI=1),
  then sends the user to the download URL.
- **AutoUpdate.dll** (`og-codebase/PartitionRecovery/AutoUpdate.dll`, x64, export
  `GetAutoUpdateInstance`): "PR auto-update imp"; build path
  `i:\_drw_main\partitionrecovery\autoupdate\autoupdateimp.cpp`; update manifests
  `epm_update.ini` and `epr_update.ini`; WinINet `HttpOpenRequestA`/`HttpSendRequestA`;
  strings "EaseUS Partition Master", "EaseUS Partition Recovery", `AutoUpdate error:
  %s %u err = %u`; reads `../res/UIConfig.ini` + `Product Version`
  (`re-workspace/reversed/PartitionRecovery/AutoUpdate.dll/strings_ascii.txt` hits;
  `re-workspace/artifacts/per-binary/AutoUpdate.md`). Its caller is not documented
  (UNVERIFIED).
- **EuDownloader.exe** (`og-codebase/EuDownloader.exe`, x86 console, 1,381,608 B):
  "Component/file downloader … PE tools, agent zips, holiday res"
  (`per-binary/EuDownloader.md:74`); 62 `curl_*` exports (resumable range downloader
  with MD5 verify — classes `CEudDownload/CEudRunable/CEudThread`); PDB
  `E:\AWork\DRW15.8AB3\sharelib\EuDownloadPublicLibrary\x86\exe\EuDownload.pdb`
  (`rabin2_info.txt:13`). Used by the PE media builder (dlconfig.xml, `TRY times=3`)
  and runtime downloads (`\EaseUS\DRW\connect.zip` NAS-agent zip strings in DRWUI,
  `strings_ascii.txt:12522-12542`; per-binary/EuDownloader.md:74 "agent zips"). It is
  a **downloader**, not a version-checker — no Upgrade.ini/update-URL strings inside
  (grep negative).
- **No-reboot driver upgrades**: numbered `%s%d` service/device variants
  (§4.2) — "in-place driver upgrades without rebooting (each new service name = new
  device name; the old instance keeps serving existing handles until unload)"
  (`epmntdrv-trio--naming-pid-handshake.md:36-39`;
  `epmntdrv-internals-v2--completion-engine-identity-ioctls.md:115-116`).
- Update-check participates in the A/B framework: Testid/vid from
  `SOFTWARE\EASEUS\DRW\Version` (§1/§3.2).

---

## 7. Repair behavior

- **No product "repair install" (maintenance mode) is documented anywhere in the
  corpus.** The Inno runtime *supports* Modify/Repair semantics generically
  (`ModifyPath`, `NoModify`, `NoRepair` value names in
  `unins000.exe/strings_utf16.txt:860-862`), but whether DRW's script sets them is
  inside `unins000.dat` (UNVERIFIED). No repair command line, no setup re-run repair
  path appears in any artifact.
- The product's own "repair" features are *data* repair, unrelated to installer
  maintenance: WinChkdsk.dll (chkdsk wrapper for the check-filesystem feature,
  `deepening-batch1:25-30`), EUFileRepair/Fixo family, RawFixer — all out of scope.
- The only "reinstall" evidence is the Upgrade.ini `killDRWUI` reinstall-or-upgrade
  flow (§6), which re-downloads the installer rather than repairing in place
  (URL points to the product download page).

---

## 8. Elevation / privilege model (consolidated)

| Component | Elevation | Evidence |
|---|---|---|
| DRW.exe (DRWLoader) | **requireAdministrator** (UAC every launch) | `re-workspace/reversed/DRW.exe/strings_ascii.txt:355` + `rabin2_strings_data.txt:1183` |
| DRWUI.exe | **requireAdministrator** (manifest) | `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:41023` + `rabin2_strings_data.txt:1728` |
| setupdrvx64.exe | SCM admin (driver/service install) | `eprdtdrv-sys--driver-internals.md:90`; advapi imports `setupdrvx64.exe/rabin2_imports.txt:90,93` |
| DRWService.exe | installs with SC_MANAGER_ALL_ACCESS; runs **LocalSystem** | `DRWService-exe--bitlocker-service.md` §1 |
| unins000.exe | runtime check "Uninstaller requires administrator: %s"; shipped manifest asInvoker (JR.Inno.Setup), two-phase relaunch + 64-bit helper pipe | `strings_utf16.txt:916`; `rabin2_strings_data.txt:2922`; `:1339-1349`, `:640` |
| DRWReport.exe, EuDownloader.exe, firebasefetch.exe, EUAdobeRepair.exe, EURawImg.exe, viewer/repair/search DLLs | **asInvoker** manifests (spawned by elevated parents in practice) | e.g. `DRWReport.exe/strings_ascii.txt:8084`; `EuDownloader.exe/strings_ascii.txt:8163`; grep ledger in session notes |
| DRWUI without driver (degraded) | "no-admin degraded mode": direct `\\.\HarddiskVolume%d` opens when the driver probe fails | `epmntdrv-trio--naming-pid-handshake.md` §6 |

Net: the reference product is **admin-elevated end-to-end** (loader + UI manifests);
the only standard-user surface the corpus describes is the *fallback* volume path and
the DRWService broker rationale ("Runs elevated so a standard-user DRW instance can
obtain the [FVE] keys without UAC" — `DRWService-exe--bitlocker-service.md:5` — which
predates/parallels the requireAdministrator manifests; keep both facts).

---

## 9. WinPE driver registration (secondary — boot media)

Already detailed in §4.4; the essentials for the rebuild:

- `BUILDPE/x64/regdrv.reg` (UTF-16 LE) registers `epmdkdrv`, `ebrntdrv` (Start=3,
  ebrntdrv `Group="Boot File System"`), `EUDCPEPM` (Start=0 BOOT_START,
  DeviceName=`epmdiskcopydrv`, IoctlBase=0x10000), `EUEDKEPM` (Start=1
  SYSTEM_START, DeviceName=`EUEDKEPM`, IoctlBase=0x10000); plus disk-class
  `UpperFilters = REG_MULTI_SZ "EUDCPEPM"` on
  `HKLM\PE_Sys\ControlSet001\Control\Class\{71A27CDD-812A-11D0-BEC7-08002BE2092F}`
  (`boot-config--winpeshl-autobat-regdrv.md` §3; note the GUID-label caveat in §3.3).
- Placement: `ebrntdrv.sys`+`epmdkdrv.sys` at `Windows/system32/` **root** (matching
  `\SystemRoot\System32\…` ImagePaths); `EUDCPEPM.sys`+`EUEDKEPM.sys` at
  `Windows/system32/drivers/`; **eprdtdrv.sys and EuMrx.sys are absent from the PE
  image** (boot-config §3 placement facts; `architecture/03:114-115`).
- `software.reg` seeds exactly one product value:
  `HKLM\SOFTWARE\EASEUS\DRW\Language = "English"` (boot-config §4).
- PE boots: winpeshl.ini → wpeinit → ChangeRes → launch.exe 0 auto.bat (≈25 .reg
  chrome imports + `regsvr32 /s` MMC snap-ins + OEM branch on
  `HKLM\SOFTWARE\EASEUS\EPM\Version\OEM`) → `DRW.exe` → launch.exe 2 keep-alive
  (boot-config §1-§2). PE is FREE-edition by construction: `version.ini
  version=free`, `product-code=RE-DRW-FREE-WIN` (`architecture/03:74`).
- Rebuild cross-check: the rda-driver repo ships the parallel
  `scripts/winpe/regdrv-rda.reg` (service `rda`, Type 1/Start 3/ErrorControl 1/Group
  "Boot File System"/ImagePath `\SystemRoot\System32\drivers\rda.sys` + Parameters +
  disk-class UpperFilters on `{4d36e967-…}`) — Agent C's DRIVER-DEPLOYMENT.md; the
  reference's own demand-start + "Boot File System" group + System32\drivers placement
  semantics match (§4.2/§4.4).

---

## 10. UNVERIFIED-BY-CORPUS list (installer-behavior open questions)

Every installer question the corpus text does **not** answer:

1. **The exact Inno AppId / final uninstall subkey** for DRW (e.g.
   `…\Uninstall\EaseUS Data Recovery Wizard_is1`): not in any string; lives in the
   binary `og-codebase/unins000.dat`. Only the generic format
   `…\Uninstall\%s\%s_is1` and the two embedded-component/competitor keys
   (`{201F83A9-…}_is1` FoxitRestore, `{Tenorshare 4DDiG}_is1`) are visible.
2. **The Inno setup EXE itself** (wizard/bootstrap before SetupUE, `[Setup]`
   PrivilegesRequired, AppId, DefaultDirName, components, tasks, `[Run]`/`[UninstallRun]`
   entries) — absent from the corpus; only SetupUE/InfoForSetup/unins000 shipped.
   Install-time actions beyond what those binaries' strings prove are unknown.
3. Whether the **installer pre-installs** DRWService/eprdtdrv at install time, or
   only the app does on demand at first run (corpus documents only the runtime path).
4. Whether unins000 removes the **driver service, System32\Drivers\*.sys copies,
   SafeBoot entries, DRWService, and SOFTWARE\EASEUS keys** — unins000.dat is
   binary-undecoded; no artifact lists its file/registry removal set or leftovers.
5. **Repair/maintenance mode** specifics for this product (ModifyPath/NoModify/NoRepair
   values; any "Modify/Repair" button behavior) — generic Inno capability only.
6. **UAC specifics of setup**: whether the Inno script is PrivilegesRequired=admin
   (only the runtime string "Uninstaller requires administrator: %s" + the shipped
   asInvoker uninstaller manifest are known — the two coexist in Inno 5.5.x with the
   elevation being performed by a relaunch; exact behavior for THIS script unknown).
7. The **DRW_Installer window's WM_COPYDATA command-999 semantics** on the receiving
   side ("documented as installer-side detail; the messenger's contract above is
   complete" — `eusendmessage--wm-copydata-messenger.md` §3).
8. Who invokes **EUinApp.exe** ("either performed by the external installer
   (out-of-tree), already-absorbed elsewhere, or simply not wired in this build" —
   `UIUX/23-euinapp-ie-emulation-bootstrapper.md:61`).
9. **EuMrx.sys install mechanism** on the live OS (no INF/setupdrv/SCM evidence for
   it; only its Services\Parameters key and PE absence are documented).
10. Whether **VC redistributables** (VC90 CRT/MFC etc.) are installed by the installer
    or merely shipped side-by-side (SxS manifests at og-codebase root; no vcredist
    evidence).
11. Shortcut creation on the live OS (PE uses createshort.exe/auto.bat; DRWUI carries
    `%s\%s.lnk` + shell-folder strings, but the creator/invoke path of live-tree
    shortcuts is not decoded).
12. **AutoUpdate.dll's caller** and whether it (vs DRWUI CVersionCtrl) performs the
    actual product self-update; also whether EuDownloader ever fetches product
    updates (evidence shows only WAIK/tools/agent-zip downloads).
13. License/activation interactions at install time (Configure.dat, License/ dir,
    euactiveonline runtime activation are documented; whether the installer seeds
    license registry data is not).
14. Why unins000's rabin2 `compiled` stamp reads `Tue Feb 25 06:28:05 2031` while
    `architecture/01:84` dates it 2016 (Inno 5.5.9 era) — metadata inconsistency
    within the corpus; harmless but unresolved.
15. The class-GUID→UpperFilters target on the **live-OS** path (setupdrvx64 carries
    both `{71A27CDD-…}` and `{4D36E967-…}`; the live write target is not decoded; see
    the GUID-label caveat in §3.3).
16. Whether the PE ebrntdrv/epmdkdrv failover is automatic (marked **INFERRED** in
    `epmntdrv-trio--naming-pid-handshake.md` §5 — mechanics verified, policy not).
17. `AliyunInstall*` / `AliyunUninstall*` DRWUI call sites: instrumented
    (GetProcAddress imports found) but *when* they fire (true install/uninstall hooks
    vs every run) is not decoded.

---

## Appendix — evidence-file quick index (for T3 integrator)

- Installer bootstrap: `re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`
- Uninstaller raw strings: `re-workspace/reversed/unins000.exe/{strings_ascii.txt, strings_utf16.txt, rabin2_strings_data.txt, rabin2_imports.txt}`
- Uninstaller stub: `re-workspace/artifacts/per-binary/unins000.md`
- Driver deployer (live): `re-workspace/artifacts/per-binary/setupdrvx64.md`,
  `deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3,
  raw `re-workspace/reversed/PartitionRecovery/setupdrvx64.exe/strings_ascii.txt:139-174`
- Driver family + lifecycle: `architecture/02-kernel-driver-architecture.md` (§5 install lifecycle)
- Trio naming/no-reboot upgrades: `re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md`
- Hook drivers (PE): `re-workspace/artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md`,
  `eudcpepm-euedkepm--hook-framework-urgent-cache.md`
- Service: `re-workspace/artifacts/DRWService-exe--bitlocker-service.md`,
  raw `re-workspace/reversed/DRWService.exe/strings_ascii.txt:36-57`
- Launcher/elevation manifests: `re-workspace/reversed/DRW.exe/strings_ascii.txt:355`,
  `DRWUI.exe/strings_ascii.txt:41023`
- PE registration: `re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md`,
  `architecture/03-winpe-boot-environment.md`
- Update/upgrade: `og-codebase/Upgrade.ini`, `og-codebase/config/version.ini`,
  DRWUI `strings_ascii.txt:23170-23334`, `per-binary/{AutoUpdate,EuDownloader,CheckVersion}.md`,
  `MOM-modules/CheckVersion.md` (offline-hive correction)
- IE/TLS bootstrapper: `re-workspace/artifacts/UIUX/23-euinapp-ie-emulation-bootstrapper.md`,
  `Licensing/euinapp--ie-bootstrapper.md`
- Media builder: `architecture/03-winpe-boot-environment.md` §4, `og-codebase/dlconfig.xml`,
  `re-workspace/artifacts/config-files/config-data-reference.md` ("WinPE builder" section)
