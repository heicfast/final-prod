# REFERENCE DRIVER BEHAVIOR — EaseUS Data Recovery Wizard (og corpus)

> **Scope**: how the reference product ("og") detects, starts, uses, installs, and reports on its
> kernel driver, and what its user sees under every driver condition. Evidence is drawn exclusively
> from the reverse-engineering corpus at `/home/z/my-project/easeus-drw-research` (git `03ad46c`).
> Every claim carries a file citation (`.c`/`.txt`/`.md` + line). Items the corpus does **not**
> answer are listed in §6 — they must not be invented.
>
> **Terminology**: "EPMNtDrv" = the raw-disk kernel driver family (`eprdtdrv.sys`,
> `eprdtdrv_win7older.sys`, PE variants `ebrntdrv.sys`/`epmdkdrv.sys`; PDB `epmntdrv.pdb`) —
> `architecture/02-kernel-driver-architecture.md:11-17`. The user-mode device-path grammar is
> `\\.\EPRDTDRV\<PID-token>\<disk#>\<part#>` and `\\.\EPRDTDRV\<PID-token>\Volume\<N>`
> (`re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md:91-99`).

---

## 1. Detection / query mechanisms

### 1.1 There is no driver-presence, version, or health *probe* — the og opens and lets it fail

The reference has **no equivalent of a `rda_available()` probe, no GET_VERSION call, no
QueryServiceStatus on its own driver**. Detection is implicit: user mode composes the
`\\.\EPRDTDRV\...` path and calls `CreateFile`; failure of that open *is* the detection result.

- The client-side open chain in MOM `Device.mo`
  (`re-workspace/reversed/ghidra-mo/Device_decompiled.c`):
  - path composed from the hardcoded templates `"\\\\.\\EPRDTDRV\\%d\\%d"` at :7793, :50628,
    :52414, :54621 and `"\\\\.\\EPRDTDRV\\%d\\Volume\\%d"` at :329 (also
    `MOM-modules/Device.md:39-40`).
  - the first `%d` is filled with the **PID-token** — `GetCurrentProcessId()` transformed
    byte-wise `b1+=b0; b2+=b1; b3+=b2` (decompile :342-351, :7806-7815, :52427-52436,
    :54634-54666) — matching the driver-side handshake
    `token = b0|(b1+b0)<<8|(b2+b1)<<16|(b3+b2)<<24`, mismatch ⇒ `STATUS_INVALID_PARAMETER`
    (`epmntdrv-trio--naming-pid-handshake.md:59-89`).
  - the raw open is `FUN_180063080` (:57350-57400): `CreateFileA` (:57380) with the composed
    path; if the returned handle is `INVALID_HANDLE_VALUE` the wrapper object is freed and an
    invalid sentinel is returned (:57393-57396). `FUN_180003350` (:1430-1450) then propagates a
    failed construction as a **NULL `IStorageDevice`**.
- **Consequence**: driver absent ⇒ `CreateFileA("\\\\.\\EPRDTDRV\\...")` fails
  (path-not-found class) ⇒ NULL device object. There is no retry of an alternate path at these
  sites (see §4).

### 1.2 The one environment query: the WinPE gate `FUN_1800577e0`

`Device_decompiled.c:50943-50992` implements the only pre-open environment check:

```c
GetSystemDirectoryA(buf, 0x104);                       // :50967
if (buf[0]=='X'||buf[0]=='x') {                        // :50968  (X: = WinPE boot volume)
    sprintf(path, "\\\\.\\%c:", buf[0]);               // :50974
    h = CreateFileA(path, 0xC0000000, 3, NULL, 3, 0x80, NULL);   // :50975
    DeviceIoControl(h, 0x560000, NULL, 0, buf2, 0x110, &n, NULL); // :50980
    // 0x560000 = IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS; 0x110-byte out buffer
}
```

Callers branch on its return: `if (FUN_1800577e0() == 0) → \\.\EPRDTDRV\... path else → direct
\\.\PhysicalDrive%d / \\.\HarddiskVolume%d` (:352-358, :7816-7822, :50653-50659, :52437-52443,
:54667-54672). I.e. the gate selects *driver path on live Windows, direct path in WinPE*
(polarity caveats in §4/§6.3). This is an environment probe, not a driver probe.

### 1.3 Service state: start-only, best-effort, result discarded

`PartitionRecovery.dll` (export table: `re-workspace/reversed/PartitionRecovery/PartitionRecovery.dll/rabin2_exports.txt:62`,
export #59 `StartDriverFunction`) contains the whole live-Windows driver lifecycle:

```c
// FUN_1800044d0, ghidra-partition/PartitionRecovery_decompiled.c:2593-2632
hSCManager = OpenSCManagerA(NULL, NULL, 0xF003F);      // :2606 SC_MANAGER_ALL_ACCESS
if (!hSCManager) return 0;                             // no SCM ⇒ silent fail
hService = OpenServiceA(hSCManager, name, 0xF01FF);    // :2611 SERVICE_ALL_ACCESS
if (hService) {
    ok = (StartServiceA(hService, 0, NULL) == 1);      // :2613
    if (!ok && GetLastError() == 0x420)                // :2620 ERROR_SERVICE_ALREADY_RUNNING
        return 1;                                      // already-running ⇒ SUCCESS
}
```

- `StartDriverFunction(LPCSTR serviceName)` (:932-938) is a thin export of this.
- The internal caller is the device-enumeration initializer `FUN_180004b80` (:2811), which first
  serializes via a global counter (`while (0 < g) Sleep(1000);` :2897-2899), then calls
  `FUN_1800044d0("eprdtdrv")` — **hardcoded service name, and the return value is discarded**
  (:2901, no branch follows) — then refreshes the `DeviceManager` MOM module and enumerates
  disks/volumes (:2904-2999).
- **No `CreateService`** in `PartitionRecovery.dll` (install belongs to `setupdrvx64.exe`, §3) and
  **no `QueryServiceStatus`** on the product driver anywhere in user mode (corpus-wide grep: only
  `DiskHealth.exe` — for the *WMI* service, §5 — and `DRWService.exe` use it).

### 1.4 What the og does NOT query (negative findings, corpus-verified)

- **No version query.** EPMNtDrv's custom IOCTL set is exactly three codes — `0x60002400`
  (set/resolve global target), `0x60002404` (GetFileObjName), `0x60002408` (GetDiskContextMode) —
  everything else passes through to the disk stack
  (`artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:75-84`). There is no GET_VERSION.
  User mode issues only `0x60002408` (`Device_decompiled.c:52598`, 4-byte mode out) — a handle
  mode check, not a version check.
- **No authorization/license query.** `og-codebase/License/` is 19 *open-source* license texts
  (qt/openssl/zlib…), not product licensing; the activation chain artifact couples activation to
  product features only (`artifacts/Licensing/licensing-activation-chain.md`). The only
  "authorization" is the PID-token in the device path grammar (§1.1) plus admin-required install.
- **Naming trap**: `GetDriverInfo.dll` sounds like a driver-info query but is a drive-*letter*
  topology module (`GetLogicalDrives`/`GetDriveTypeW`, **no SCM imports** —
  `artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md:18-29`).
- `Device.mo` also enumerates storage via WMI namespaces (`\\.\ROOT\microsoft\windows\storage:
  MSFT_Disk/MSFT_PhysicalDisk/MSFT_Volume/...` — `MOM-modules/Device.md:45-50`) — the device
  *inventory* never depends on the driver; only raw sector IO does.

---

## 2. Per-condition behavior table

Legend for "surface": **U** = user-facing string/dialog; **L** = log/console/trace only;
**—** = nothing (silent). All conditions on **live Windows** unless noted.

| Condition | How the og distinguishes it | User-facing refusal language (verbatim) | Error codes / classes | Which surfaces disable vs stay enabled |
|---|---|---|---|---|
| **Driver ABSENT** (service never installed / file deleted) | `OpenServiceA("eprdtdrv")` fails → `FUN_1800044d0` returns 0, **discarded** (`PartitionRecovery_decompiled.c:2611,2901`); later `CreateFileA("\\\\.\\EPRDTDRV\\...")` fails → NULL `IStorageDevice` (`Device_decompiled.c:57380-57396, 1430-1450`) | **—** no driver-specific string exists in the extracted English UI strings (`re-workspace/reversed/DRWUI.exe/qt_strings_english.txt` — grep for driver yields only the PE "Add Driver" feature, §3.4). Nearest generic device strings: `"*Storage device error."` (:373), `"Unable to continue scanning. Reconnect your drive and try again."` (:1361), `"4Partition Cannot Be Opened"` (:822) — none mention the driver | Win32 `ERROR_SERVICE_DOES_NOT_EXIST` (1060) at OpenService (unconsumed); `ERROR_FILE_NOT_FOUND`/`ERROR_PATH_NOT_FOUND` (2/3) at CreateFile (unconsumed, collapses into NULL object) | Device *listing* stays enabled (WMI/GetDriverInfo topology, §1.4); the raw-IO device objects silently fail to construct. Scans that need raw sectors fail downstream as generic device/scan failures |
| **Driver NOT STARTED** (installed, stopped) | `StartServiceA` attempted at every device-enumeration refresh (`:2613`, `:2901`); `ERROR_SERVICE_ALREADY_RUNNING` (0x420) ⇒ treated as success (`:2620-2622`) | **—** (same as absent) | 0x420 handled; other StartService failures return 0, discarded | Same as absent. If the start *succeeds* mid-session, the very next `\\.\EPRDTDRV` open succeeds — no re-init dance is evidenced |
| **START FAILURE** (e.g. load blocked, image bad) | `StartServiceA` returns 0 with a non-0x420 `GetLastError()` → function returns 0 (`:2618-2625`); `setupdrvx64.exe` additionally prints to its console `"StartService failure! Error = %d \n"` (`re-workspace/reversed/PartitionRecovery/setupdrvx64.exe/strings_ascii.txt:145`, decompile `setupdrvx64_decompiled.c:246-254`) | **U/L**: console-only inside the setup tool; **no product dialog** is evidenced | L: raw Win32 error printed (`Error = %d`); product side: none | Product continues into enumeration with the result discarded (`:2901`) |
| **WRONG VERSION** | **NOT DISTINGUISHED — no version surface exists.** EPMNtDrv exposes no version IOCTL (§1.4); the client issues none. OS-based build selection instead: `eprdtdrv.sys` (Win8+) vs `eprdtdrv_win7older.sys` (Win7−) shipped side by side (`artifacts/per-binary/eprdtdrv_win7older.md:28-29`, `architecture/02-kernel-driver-architecture.md:11-12`); upgrades use *new service names* (`eprdtdrv2`, `eprdtdrv3`… ⇒ new device names `EPRDTDRV2`…), so versions coexist rather than negotiate (`epmntdrv-trio--naming-pid-handshake.md:34-39`) | **—** | — | — |
| **BLOCKED** (signature policy / Safe Boot mismatch / stale instance) | No dedicated check. Load failures surface only as StartService failure (row above). Mitigations are *deployment-side*: service also registered under `SafeBoot\Minimal` and `SafeBoot\Network` so Safe Mode loads it (`setupdrvx64_decompiled.c:29-50`, FUN_140001000); numbered variants sidestep stale in-use instances (`epmntdrv-trio--naming-pid-handshake.md:34-39`); WinPE avoids signing friction entirely (§3.3) | **—** | StartService error only | Same as start failure |
| **UNAUTHORIZED** (another process reusing the path) | Driver-side PID-token handshake: re-derived from `PsGetCurrentProcessId()` of the *opening* process; token mismatch ⇒ `STATUS_INVALID_PARAMETER` at IRP_MJ_CREATE (`epmntdrv-trio--naming-pid-handshake.md:59-89`; user-side transform `Device_decompiled.c:342-351`). No license/activation coupling (§1.4). DRWUI itself always runs elevated — embedded manifest `<requestedExecutionLevel level='requireAdministrator' uiAccess='false'/>` (`DRWUI.exe/strings_ascii.txt:41023`) | **—** for the DRW process (it is the authorized caller). Non-DRW abusers get kernel error 87 (`STATUS_INVALID_PARAMETER`) — invisible to DRW's UI | `STATUS_INVALID_PARAMETER` (0xC000000D / Win32 87) at CREATE; elevation failure would be `ERROR_ACCESS_DENIED` (5) at OpenSCManager (`:2607-2609` returns 0 silently) | Nothing — the DRW UI has no notion of driver authorization |
| **DRIVER ERRORED** (runtime IO failure) | Driver-side trace logging only: `[FltReadWriteSectorsCompletion] Io error 0x%x,%p` and `[epmntdrvDispatchWrite] Write error 0x%x,%u,%I64u,0x%x` (`re-workspace/reversed/PartitionRecovery/eprdtdrv.sys/strings_ascii.txt:25,32`); target-resolution failures: `[GetDiskDeviceObejct] %ws open error 0x%x,retry` → `...that's it!` (:20,22) and `[GetVolumeDeviceObejct] %ws open error 0x%x,that's it!` (:24) — internal retry is `\Device\Harddisk{D}\Partition0` → `\Device\Harddisk{D}\DR{P}` → give up (`eprdtdrv-sys--driver-internals.md:40-43`) | **—** (DbgPrint traces, checked builds visible only with a kernel debugger) | NTSTATUS values logged; failed reads complete the user IRP with the error status | Read/Write of that handle fails; no UI state change |

**Summary row**: the reference product **never renders a driver-condition message**. Every
driver condition is absorbed as either a discarded boolean (`:2901`), a NULL device object
(`:57393-57396`), or a failed IRP. The English Qt string table contains *zero* driver-failure
strings (`qt_strings_english.txt` — only "Add Driver"/PE-media strings match "driver").

---

## 3. Driver install UX, failure handling, and WinPE

### 3.1 Live Windows: `setupdrvx64.exe` — the WinDiskAccessDriverSetup installer

Binary: `og-codebase/PartitionRecovery/setupdrvx64.exe` (174 KB, PDB
`mod.WinDiskAccessDriverSetup\setupempdrvx64.pdb` — `artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md:32-38`).
Decompiled behavior (`re-workspace/reversed/ghidra-partition/setupdrvx64_decompiled.c`):

- **Create** (`FUN_1400011c0`, :58-93): `CreateServiceA(scm, name, name, 0xF01FF, type=1
  (KERNEL_DRIVER), startType, errorControl=1, imagePath, "Boot File System", ...)` (:72-73).
  `ERROR_SERVICE_EXISTS` (0x431) ⇒ treated as success (:76-78). On success it additionally
  registers the service under `HKLM\SYSTEM\CurrentControlSet\Control\SafeBoot\Minimal` **and**
  `...\SafeBoot\Network` (`FUN_140001000`, :29-50) so the driver loads in Safe Mode/Safe Mode
  with Networking.
- **Start** (`FUN_1400015c0`, :226-265): `OpenServiceA` + `StartServiceA`;
  `ERROR_SERVICE_ALREADY_RUNNING` (0x420) ⇒ returns 0x401 success (:248-250).
- **Uninstall** (`FUN_140001420` mode 2, :182-187): `ControlService(SERVICE_CONTROL_STOP)`
  (`FUN_140001670`, :290) + `DeleteService` (`FUN_140001540`, :214) + set registry `DeleteFlag=1`
  (`FUN_1400012d0`, :129) — a boot-time delete marker for in-use images; install clears it
  (`DeleteFlag` deleted, :126). Command line: `-Uninstall` (:869).
- **Layouts** (strings): `\System32\Drivers\%s.sys`, `\SystemRoot\System32\drivers\%s.sys`,
  `%s\System32\Drivers\%s.sys`, and the versioning variant `%s\System32\Drivers\%s%d.sys`
  (`setupdrvx64.exe/strings_ascii.txt:151,153,168,169,170`) — the `%s%d` name is the coexisting
  numbered-instance scheme (`epmntdrv-trio--naming-pid-handshake.md:34-39`).
- **Failure language** — console `printf`s only, never product dialogs:
  `"CreateService failed!  Error = %d \n"` (:142), `"OpenService failed!  Error = %d \n"`
  (:144,146), `"StartService failure! Error = %d \n"` (:145), `"ControlService failed!  Error =
  %d \n"` (:146 in strings; decompile sites :80, :239, :252, :293).
- **Reboot**: no reboot is required or prompted anywhere in the corpus. DriverUnload is supported
  (`[EPMNtDrv]DriverUnload called`, `eprdtdrv.sys/strings_ascii.txt:18`; internals v2 §4 per trio
  artifact :37-39), and in-place upgrades ride the numbered-service mechanism; the only
  boot-deferred action is the `DeleteFlag` marker for a busy image.
- `og-codebase/PartitionRecovery/setupdrv.ini` contains exactly one line — `[eprdtdrv]`
  (service-name section header; its consumer is not identified in the corpus, §6.2).

### 3.2 When is it installed? (partly corpus-silent)

The runtime code path (`PartitionRecovery.dll`) only *starts* an already-installed service
(§1.3) — it never installs. The corpus contains **no caller** of `setupdrvx64.exe` from any
reversed user-mode binary (grep over `re-workspace/reversed/`: zero references outside the
Ghidra report). `eprdtdrv-sys--driver-internals.md:96` claims "installs+starts eprdtdrv service
on first scan needing raw access" — that "first scan" timing is the artifact's inference, not
code-cited. See §6.1.

### 3.3 WinPE (BUILDPE): no install UX at all — registry-baked services

`BUILDPE/x64/regdrv.reg` (decoded in `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md:93-120`):

| Service | Start | ImagePath | Notes |
|---|---|---|---|
| `epmdkdrv` | 3 DEMAND_START | `\SystemRoot\System32\epmdkdrv.sys` | EPMNtDrv family, PE name |
| `ebrntdrv` | 3 DEMAND_START, Group `"Boot File System"` | `\SystemRoot\System32\ebrntdrv.sys` | EPMNtDrv family, PE name |
| `EUDCPEPM` | 0 BOOT_START | `system32\drivers\EUDCPEPM.sys` | disk-class **UpperFilter** (hook), `DeviceName="epmdiskcopydrv"`, `IoctlBase=0x10000` |
| `EUEDKEPM` | 1 SYSTEM_START | `system32\drivers\EUEDKEPM.sys` | raw-disk door, `DeviceName="EUEDKEPM"`, `IoctlBase=0x10000` |

- **`eprdtdrv.sys` is absent from the PE image** (boot-config artifact :114) — the live-Windows
  name simply isn't deployed there.
- No dialogs, no elevation, no signing prompts: winpeshl.ini launches `wpeinit.exe` → shell
  chrome (auto.bat) → `DRW.exe` (boot-config artifact :11-32); nothing in `auto.bat` touches DRW
  files or drivers (:139).
- **No user-mode opener exists for the PE instances**: corpus-wide grep for `EBRNTDRV|EPMDKDRV`
  in `re-workspace/reversed/` (excluding the driver binaries themselves) returns **zero** hits.
  Same "dormant deployment" verdict as EUEDKEPM
  (`artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md:280`; `architecture/02-kernel-driver-architecture.md:129`).
  Combined with §4's PE gate (direct paths in PE), the PE driver instances are carried by the
  *shared EaseUS emergency-media image* (EPM/Todo-Backup lineage — boot-config artifact :66, :140)
  rather than opened by DRW components.

### 3.4 The only *user-facing* driver UI: "Add Driver" (third-party INF loading)

`DRWUI.exe` main-window More-menu, action ID 0xC, icon `menu_ico_Add driver.svg`, appearing in
scan/results states (`artifacts/UIUX/35-redesign-reference-atlas.md:85-89`). Implementation
(`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:1320046-1320118`):

```
GetOpenFileNameW(filter "*.inf", defExt "inf")            // :1320062-1320069
ShellExecuteW(NULL, L"Open", L"drvload.exe", <inf path>, NULL, 1)   // :1320073
  success (ret > 32): tr("Load Driver %1 Success")         // :1320097
  failure (ret <= 32): FormatMessageW(err) + tr("Load Driver failed,Error Message:%1")  // :1320076-1320078
```

Plus the resource-table pair `"4Successfully add driver %1"` / `"NFailed to add driver. Error
Message: %1"` (`DRWUI.exe/qt_strings_english.txt:687-688`) and the section header
`"Hard Disk Drivers"` (`strings_ascii.txt:11843`). `drvload.exe` is the WinPE driver-loading
tool — this feature exists to load *missing storage/RAID drivers* (so disks become visible in
PE), **not** to install the product's own driver. Failure language = the raw Windows
`FormatMessageW` text substituted into `%1`.

The PE-media *builder* flows (`pet_install_driver`, `pet_download_packet`, … —
`DRWUI.exe/strings_ascii.txt:14671`, `:26580`, `:35409`; PDB `pe_maker\grabdriver.cpp` :35466)
install driver packets into the **bootable USB being created**, not into the running system
(strings context: "Please connect a USB flash drive", `PEDeviceItemTitle`).

### 3.5 Adjacent service lifecycle (for contrast, not the driver)

`DRWService.exe` ("Read FVE Server", BitLocker key service) is the one service with visible
lifecycle strings: installed `SERVICE_AUTO_START`, started immediately, uninstall logs
`"Service stopping..."`, `"Uninstall service succeed/failed"`
(`artifacts/DRWService-exe--bitlocker-service.md:7-30`). The kernel driver gets none of this
visibility.

---

## 4. The reference's OWN driver-vs-direct duality (fallback behavior)

This is the behavior our product will deliberately diverge from (fallback removal), so it is
pinned to exact code.

### 4.1 The switch: environment-gated, one-shot, silent

Every raw device construction site in `Device.mo` picks **one path per environment** via
`FUN_1800577e0()` (§1.2):

| Site (Device_decompiled.c) | Driver path (gate==0, live Windows) | Direct path (gate!=0, WinPE) | Constructed object |
|---|---|---|---|
| :329-365 | `\\.\EPRDTDRV\<token>\Volume\<N>` (:355) | `\\.\HarddiskVolume%d` (:358) | volume `IStorageDevice` |
| :7793-7829 | `\\.\EPRDTDRV\<token>\<d>\<p>` (:7819) | `\\.\PhysicalDrive%d` (:7822) | `CDiskDriveLayoutReadWriter` device |
| :50628-50666 | same (:50656) | `\\.\PhysicalDrive%d` (:50659) | second layout-readwriter variant |
| :52414-52459 | same (:52440) | `\\.\PhysicalDrive%d` (:52443) | `CWindowsStorageDevice` |
| :54621-54679 | same (:54669) | `\\.\PhysicalDrive%d` (:54672) | `CMomAggObject<CWindowsStorageDevice>` |

Both branches request `GENERIC_READ` (0x80000000) for volume opens (:360, :370) — i.e. the
direct fallback is a *read*-sufficient path ("plain volume access without the driver … the driver
is only required for raw/locked-volume and disk-level IO" —
`epmntdrv-trio--naming-pid-handshake.md:145-151`).

### 4.2 What happens when the chosen path FAILS — the critical nuance

- **There is no failure-triggered fallback at any of the five sites.** The choice is made *before*
  the open; a failed `CreateFile` on the chosen path produces an invalid handle wrapper
  (`Device_decompiled.c:57393-57396`) → NULL `IStorageDevice` (`:1430-1450`). The og does **not**
  retry `\\.\PhysicalDriveN` after a failed `\\.\EPRDTDRV` open on live Windows, and does not
  retry the driver path after a failed direct open in PE.
- Therefore the reference's "fallback" is properly described as an **alternate primary path
  selected by environment (WinPE), not a runtime degradation path**. On live Windows with a
  dead driver, the og's evidenced behavior is *silent device-construction failure*, not silent
  transport switch.
- **User visibility of the switch: zero.** No message, no log string, no UI state is emitted at
  the gate or the open sites. The trio artifact's phrase "the 'no-admin' degraded mode visible
  in the UI flows" (`epmntdrv-trio--naming-pid-handshake.md:151`) is that artifact author's
  inference; no UI string tying any state to the direct path exists in the corpus (and DRWUI is
  `requireAdministrator` anyway, `strings_ascii.txt:41023`, so a no-admin mode cannot arise from
  DRWUI itself).
- **Fallbacks that DO exist are inside the kernel driver**, not between transports: target
  resolution tries `\Device\Harddisk{D}\Partition0` then `\Device\Harddisk{D}\DR{P}` then gives
  up (`eprdtdrv-sys--driver-internals.md:40-43`, trace strings `...retry` / `...that's it!`,
  `eprdtdrv.sys/strings_ascii.txt:20-24`); the stack walk verifies the target is driven by
  `disk.sys` and stores the resulting mode, queryable via IOCTL `0x60002408`
  (`epmntdrv-trio--naming-pid-handshake.md:103-119`).
- Ancillary direct-path machinery in `Device.mo` confirms the direct family is first-class:
  `\\.\PhysicalDrive%d` name builders (:52518, :52976), `\\?\PhysicalDrive%d` opens (:53223,
  :53416), `\\.\PHYSICALDRIVE` prefix parsing (:50914), plus `\\.\%c:` drive-letter opens
  (`MOM-modules/Device.md:38-51`).

### 4.3 Divergence note (for our fallback-removal work)

Removing the direct fallback diverges from the reference in a specific, citable way: the og (a)
uses direct handles as the **primary path in WinPE** and (b) tolerates driver-open failure by
**silently producing a NULL device object** — never by telling the user, and never by
re-routing a live-Windows open to `\\.\PhysicalDriveN`. Our driver-only architecture must
therefore define behavior the og leaves undefined (explicit refusal on driver failure), and the
reference evidence supports that no user-facing string or error-class precedent is being broken.

---

## 5. Driver-status surface (the diskhealth module)

**The reference exposes no driver status/health surface for EPMNtDrv.** No API, UI, telemetry
event, or tray state reports running/stopped/version/authorization of the product driver (§1.4,
§2). What the corpus actually contains under "health":

### 5.1 `diskhealth/DiskHealth.exe` — a SMART disk-health companion, not a driver monitor

- Separate Qt Quick/QML app (840×590 frameless window, "EaseUS DiskHealth"), CrystalDiskInfo
  lineage — engine classes `DiskInfoMgr`, `CAtaSmart`, `RefreshThread`, `CTrayIcon`,
  `AttrModel`; per-disk SMART polling with SAT/CSMI/NVMe-Realtek transport accessors
  (`artifacts/DiskHealth/diskhealth--smart-companion.md:14-23`).
- **States it distinguishes are disk health states, not driver states**: "Good" / "Caution!" /
  "Normal" / "Your disk may have a physical failure:" — full 32-string census in
  `artifacts/UIUX/19-diskhealth-quick-ui.md:24-30` (SMART table: ID / Attribute / Current /
  Worst / Threshold / RawValue / Status / "Temprature" [sic]).
- Tray-resident guarding, launched from Windows notifications
  (`"Click_Open_DiskHealth_By_Notification"`, window names `EaseUS_DiskHealth`/
  `EaseUS_DiskHealthConsole` — companion artifact :28).
- **Its only SCM activity is ensuring WMI runs**: `OpenSCManagerW(NULL,NULL,0x20000000)` →
  `OpenServiceW(name, 0x14)` → `QueryServiceStatus` → if not RUNNING(4): `StartServiceW`,
  poll (≤400 ms in 100 ms steps), then `ShellExecuteW("sc config Winmgmt start= auto")`, then a
  ≤10×500 ms start-poll loop, success trace `"StartService Completed : SERVICE_RUNNING"`
  (`re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c:9441-9546`; the `sc config`
  call at :9508). The serviced name service is `Winmgmt` — WMI, needed for SMART queries.
- Deployed from the boot USB by `auto.bat` Phase E and shipped in the live tree; DRWUI pairs it
  with home-page SSD/TRIM warnings (`diskhealth--smart-companion.md:25-29`).

### 5.2 The driver's own introspection surface (minimal)

The full extent of runtime driver introspection available to og user mode: IOCTL `0x60002408`
GetDiskContextMode (4-byte mode; the only custom IOCTL user mode actually issues —
`Device_decompiled.c:52598`), `0x60002404` GetFileObjName, `0x60002400` global-target set
(`eprdtdrv-sys--driver-internals.md:75-84`). A GET_VERSION-style identity IOCTL exists **only**
on the WinPE/Todo-Backup-lineage driver EUDCPEPM (`0x800C2AC8` → `0x00010100` + "EASEUS Todo
Backup Driver (c) Ea…" — `eudcpepm-euedkepm--hook-drivers.md:61`), which has **no in-tree DRW
opener** (:280). Nothing in the DRW stack queries it.

---

## 6. Corpus-silence list (do not invent)

1. **Who invokes `setupdrvx64.exe`** (installer-time vs first-run vs on-demand) — no caller
   found in any reversed binary or shipped config; `eprdtdrv-sys--driver-internals.md:96`'s
   "on first scan" is inference (§3.2).
2. **Who calls the `StartDriverFunction` export** and what reads `setupdrv.ini` (`[eprdtdrv]`)
   — no import/reference in DRWUI/DRW/MainModule decompiles or string dumps; the only evidenced
   internal call hardcodes `"eprdtdrv"` (`PartitionRecovery_decompiled.c:2901`).
3. **Exact return polarity of `FUN_1800577e0`** — Ghidra types it `void`; callers read EAX
   (`:352`, `:7816`, `:50653`, `:52437`, `:54667`). The X:-system-drive + volume-extents probe
   strongly implies nonzero = WinPE (⇒ direct paths), consistent with the branch code
   (`==0 → EPRDTDRV`), but the decompile alone cannot prove the register's live value; the
   trio artifact §6 words it the opposite way ("when the probe fails … direct") — treat the
   polarity as high-confidence inference, not byte-proof.
4. **Whether any higher layer re-enumerates with direct paths after a failed `\\.\EPRDTDRV`
   open on live Windows** — the five construction sites never retry (§4.2), but the
   `DeviceManager.mo`/`MainModule` reaction to NULL devices was not traced end-to-end.
5. **Driver-condition strings in non-English `.qm` catalogs** — only the English Qt table is
   extracted (`DRWUI.exe/qt_strings_english.txt`); the 37 shipped `.qm` files are binary and
   were not searched (no "driver" refusal string found in English; other languages unverified).
6. **Any driver version/ABI negotiation** — no version surface exists at all for EPMNtDrv
   (§2 "wrong version" row); nothing to cite beyond the negative finding.
7. **License/activation gating of the driver** — no evidence of any coupling (§1.4).
8. **Reboot prompts on install failure** — none evidenced; `setupdrvx64.exe` failure output is
   console-only, and no product dialog references it (§3.1).
9. **Live-Windows behavior when driver load is blocked by signing policy** (e.g. Win7 x64
   enforcement) — the corpus shows only the discarded StartService result; no dialog/string.
10. **Whether DRW ever demand-starts the PE instances (`ebrntdrv`/`epmdkdrv`)** — zero in-tree
    openers for `EBRNTDRV`/`EPMDKDRV` device paths (corpus-wide grep); the boot-config
    artifact's "the product triggers NtLoadDriver via the service SCM at first raw access"
    (`boot-config--winpeshl-autobat-regdrv.md:119`) remains an inference contradicting §4's
    PE-gate reading.
11. **The Win7 build-selection mechanism** (who chooses `eprdtdrv_win7older.sys` on Win7) —
    both files ship side-by-side; no selector code was found.

---

## 7. Evidence index (primary sources)

| Topic | File |
|---|---|
| Client open sites + PID token + WinPE gate | `re-workspace/reversed/ghidra-mo/Device_decompiled.c` (:329-365, :7793-7829, :50628-50666, :50943-50992, :52414-52459, :52598, :54621-54679, :57350-57400, :1430-1450) |
| Service start, enumeration trigger | `re-workspace/reversed/ghidra-partition/PartitionRecovery_decompiled.c` (:932-938, :2593-2632, :2811, :2897-2901); `rabin2_exports.txt:62` |
| Driver internals + IOCTLs + trace strings | `re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md`; `re-workspace/reversed/PartitionRecovery/eprdtdrv.sys/strings_ascii.txt:18-35` |
| Naming/PID handshake/trio | `re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md` |
| Installer | `re-workspace/reversed/ghidra-partition/setupdrvx64_decompiled.c` (:1-322, :869); `setupdrvx64.exe/strings_ascii.txt:142-170`; `artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3 |
| WinPE registration | `re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` §1-§3; `og-codebase/BUILDPE/x64/regdrv.reg` |
| Add Driver UI | `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:1320046-1320118`; `DRWUI.exe/strings_ascii.txt:12804-12849`; `qt_strings_english.txt:687-688`; `artifacts/UIUX/35-redesign-reference-atlas.md:85-89` |
| UI strings (device errors, admin) | `DRWUI.exe/qt_strings_english.txt:142,161,373,822,1361`; `strings_ascii.txt:41023` (requireAdministrator) |
| DiskHealth | `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c:9441-9546`; `artifacts/DiskHealth/diskhealth--smart-companion.md`; `artifacts/UIUX/19-diskhealth-quick-ui.md` |
| Hook drivers / GET_VERSION | `re-workspace/artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (:61, :280) |
| Naming trap | `re-workspace/artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md` |
| Architecture syntheses | `architecture/02-kernel-driver-architecture.md` (§2, §5, §6); `architecture/04-recovery-workflow.md` |
| Errata check | `re-workspace/artifacts/CORRECTIONS.md` (no driver-behavior corrections) |
