# REFERENCE-ACCESS — How the og (EaseUS DRW) accesses disks: driver, direct, or both

> Agent D (T2-d) research report, evidence-cited against the corpus
> `/home/z/my-project/repos/easeus-drw-research` (paths below are relative to that repo root).
> Every claim carries `path:line`. Corrections/uncertainties are flagged inline and collected in §8.
> Scope: the og's kernel-driver access path, its direct user-mode raw-I/O path, the
> environment gate between them, the privilege model, edge-state behavior, and performance
> characteristics — as precedent for the core's driver→direct replacement (§7).
>
> Verification of the core's standing claim ("the og gates on environment (WinPE → direct)
> and silently NULLs failed driver opens — corpus Device.mo"): **CONFIRMED**, with one
> correction to a corpus artifact's *interpretation* of the gate (§8, item C-1).

---

## §1 The og's access architecture overall — BOTH paths, gated by environment

The og ships **one kernel-driver access subsystem AND full direct user-mode raw I/O**, and
the choice between them is made **per device-open, by a WinPE environment probe** — not by
driver availability, not by admin status.

### 1.1 The two access stacks

| Layer | Driver path (live Windows) | Direct path (WinPE / own-use sites) |
|---|---|---|
| Device namespace | `\\.\EPRDTDRV\{PID-token}\{disk}\{part}` or `\\.\EPRDTDRV\{PID-token}\Volume\{N}` served by eprdtdrv.sys (EPMNtDrv) | `\\.\PhysicalDrive%d`, `\\.\HarddiskVolume%d`, `\\.\%c:`, `\\?\Volume{GUID}` — plain Win32 |
| Who opens it | MOM `Device.mo` device layer (PartitionRecovery) | MOM `Device.mo` (PE branch) + DRWUI.exe's own disk-info/destination layers (always) |
| Read mechanics | IRP_MJ_READ on driver handle; length in 512-byte sectors; MDL async passthrough | `SetFilePointer` + synchronous `ReadFile`, chunked 4/7 MiB |
| Enumeration | WMI `ROOT\microsoft\windows\storage:MSFT_{Disk,PhysicalDisk,Volume,VirtualDisk,...}` — environment-independent | same |

Driver family and namespace evidence: `architecture/02-kernel-driver-architecture.md:9-17`
(7 binaries, 4 roles; `\Device\EPMNTDRV` + `\\.\EPRDTDRV\…`), `:29-43` (grammar
`\\.\EPRDTDRV\%d\%d`, `\\.\EPRDTDRV\%d\Volume\%d`, plus the two regex literals; internal
`\Device\Harddisk%u\DR%u`, `\Device\Harddisk%u\Partition0`, `\Device\HarddiskVolume%u`).
Enumeration-by-WMI: `re-workspace/reversed/ghidra-mo/Device_decompiled.c:17024,17196,17799`
(`\\.\ROOT\microsoft\windows\storage:MSFT_Volume/MSFT_Disk/MSFT_PhysicalDisk` etc.).
Direct-path literals in the same module: `re-workspace/artifacts/MOM-modules/Device.md:39-51`
(`\\.\EPRDTDRV\%d\%d`, `\\.\EPRDTDRV\%d\Volume\%d`, `\\.\HarddiskVolume%d`,
`\\.\PHYSICALDRIVE%d`, `\\.\PhysicalDrive%d`, `\\?\PhysicalDrive%d`).

### 1.2 The environment gate — `IsWinPE()` decides, at every device-open site

Device.mo contains a WinPE probe, `FUN_1800577e0`
(`re-workspace/reversed/ghidra-mo/Device_decompiled.c:50943-50992`):

```
GetSystemDirectoryA(buf, 0x104)                        // :50967
if buf[0] == 'X' || 'x':                               // :50968  (WinPE boots from X:)
    sprintf("\\\\.\\%c:", 'X')                          // :50974
    CreateFileA(\\.\X:, GENERIC_READ|GENERIC_WRITE, share 3, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL)  // :50975
    DeviceIoControl(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS /*0x560000*/, out 0x110)                    // :50980
```

The fully-typed twin in RecoveryModule.dll
(`re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:42737-42801`) shows the
complete semantics: **config override first** (`../res/UIConfig.ini` `[Public] IsWinPE`,
:42741-42747), then the X:-drive check, extents probe, and a `DiskNumber == -1` test on the
first extent (:42786) — i.e. "X: is the WinPE RAM disk". The same probe is inlined in at
least Device.mo (`Device_decompiled.c:50943`), DeviceManager.mo
(`re-workspace/reversed/ghidra-mo/DeviceManager_decompiled.c:13221`), RecoveryModule.dll
(`RecoveryModule_decompiled.c:42757`), and UILogic.dll (strings `IsWinPE`/`\\.\%c:`
adjacent at `re-workspace/reversed/PartitionRecovery/UILogic.dll/strings_ascii.txt:1040-1045`).

At every device-open site the gate forks identically (5 sites verified):

```c
// disk open (CDiskDriveLayoutReadWriter ctor and kin)
if (FUN_1800577e0() == 0)                                   // NOT WinPE (live Windows)
    sprintf(path, "\\\\.\\EPRDTDRV\\%d\\%d", pid_token, disk);   // DRIVER path
else                                                        // WinPE
    sprintf(path, "\\\\.\\PhysicalDrive%d", disk);               // DIRECT path
```
`Device_decompiled.c:7816-7823` (disk), `:50653-50660`, `:52437-52444`, `:54667-54673`
(same), `:352-359` (volume variant: probe==0 → `\\.\EPRDTDRV\%d\Volume\%d`; else →
`\\.\HarddiskVolume%d`).

**Decision map (verified):**

```
                    ┌─ live Windows (IsWinPE()==0)
  device open ──────┤    → \\.\EPRDTDRV\{PID-token}\...   [kernel driver eprdtdrv.sys]
                    │       (service ensured/started first: OpenSCManager/StartService)
                    │
                    └─ WinPE (IsWinPE()!=0: X: + RAM-disk extents + config override)
                         → \\.\PhysicalDrive%d  /  \\.\HarddiskVolume%d   [DIRECT user-mode]
                    
  independent of the gate (both environments):
    DRWUI.exe own opens → \\.\PHYSICALDRIVE%d, \\.\%c:  (identity/geometry/destination)
    enumeration        → WMI MSFT_* storage namespace
    BitLocker FVE keys → \\.\Pipe\DRWBitlocker (DRWService.exe, SCM service)
    NAS                → EuMrx.sys RDBSS redirector (\\.\EuRdrDN)
```

### 1.3 "Silently NULLs failed driver opens" — confirmed

On live Windows the composed `\\.\EPRDTDRV\...` string is handed to the
`CWindowsStorageDevice` factory chain. `CWindowsStorageDevice::Open`
(`FUN_180061740`, `Device_decompiled.c:56455-56477`) calls `CreateFileA` via the
universal open wrapper (§2.1); on `INVALID_HANDLE_VALUE` it stores `GetLastError()` into
`+0x18` and **returns 0 — no retry, no fallback path, no exception** (`:56460-56463`).
The MOM query wrapper `FUN_180003350` (`:1430-1450`) then returns **NULL** when the
query/`ConvertVolume` fails, and the caller stores that NULL into its device slot without
propagation — e.g. `CDiskDriveLayoutReadWriter` ctor `:7828-7829`
(`*(param_1 + 0x10) = FUN_180003350(...)`). A device object whose open failed stays in a
"closed" state where its `DeviceIoControl` stub reports error 5 (ACCESS_DENIED)
(`FUN_18005fb20`, `:55635-55637`: `if (!opened) { this->err = 5; return 0; }`).

---

## §2 The direct-access pattern in exact detail

### 2.1 The universal open wrapper — plain `CreateFileA`, no buffering flags

`FUN_180063080` (`Device_decompiled.c:57351-57400`) is the single open funnel for the MOM
device layer:

```c
// pseudo from decompile
box = new(0x10); box->magic = 'Vmdk' (0x6b646d56)                  // :57372
if (IsVmdkPath(path))  box->obj = OpenVmdk(path, ...)               // :57377,57389
else                   box->h   = CreateFileA(path, access, share, NULL,
                                               disposition, flags, NULL)  // :57380
if (handle == -1) { free(box); return -1; }                         // :57393-57396
```

The **Vmdk magic box** is why later `SetFilePointer/ReadFile` wrappers fork on
`*(int*)h == 0x6b646d56` (`FUN_1800634c0`, `:57499-57513`) — a handle-or-virtual-disk
abstraction, not a driver fork. All MOM-layer direct opens go through this box; the
share/access come from object fields (`+0x3b0` desired access, `+0x3b4` share; set by the
`CWindowsStorageDevice` ctor `FUN_180061580`, `:56399-56413`, params 4/5 → `+0x3b8/+0x3bc`
relative base).

**Observed flag combinations (verbatim constants):**

| Site | Path | Access | Share | Disposition | Flags |
|---|---|---|---|---|---|
| `Device_decompiled.c:56456` (CWindowsStorageDevice::Open) | composed (EPRDTDRV or PhysicalDrive/HarddiskVolume) | object field | object field | 3 `OPEN_EXISTING` | 0x80 `FILE_ATTRIBUTE_NORMAL` |
| `Device_decompiled.c:55370` (CWindowsDynamicVolumeDevice::Open) | `+0xa0` | field | field | 3 | 0x80 |
| `Device_decompiled.c:57101` (CVolumeDevice::Open by device-object name — `\??\Volume{GUID}` → `\\?\...` via `'?'→'\'` rewrite `:57090-57091`, trailing-`\` strip `:57094-57099`) | volume GUID/NT name | 0xc0000000 R\|W | **7** R\|W\|DELETE | 3 | 0x80 |
| `Device_decompiled.c:50975` (IsWinPE probe) | `\\.\X:` | 0xc0000000 | 3 | 3 | 0x80 |
| `Device_decompiled.c:52986` (writability probe) | `\\.\PHYSICALDRIVE%d` (upper) | 0xc0000000 | 3 | 3 | **0** |
| `DRWUI_decompiled.c:176880` (disk-info layer) | `\\.\PHYSICALDRIVE%d` | 0xc0000000 | 3 | 3 | **0x60000080 = FILE_FLAG_OVERLAPPED \| FILE_FLAG_NO_BUFFERING \| FILE_ATTRIBUTE_NORMAL** |
| `DRWUI_decompiled.c:178062` (identity query) | `\\.\PhysicalDrive%d` | 0xc0000000 | 3 | 3 | 0x80 |
| `DRWUI_decompiled.c:177643` (query, lowercase) | `\\.\physicaldrive%d` | 0xc0000000 | 3 | 3 | 0x80 |
| `DRWUI_decompiled.c:318410` (destination picker) | `\\.\%c:` | 0xc0000000 | 3 | 3 | 0x80 |
| `RecoveryModule_decompiled.c:42769` (IsWinPE twin) | `\\.\X:` | 0xc0000000 | 3 | 3 | 0x80 |
| `Device_decompiled.c:25481→25484` (property refresh) | volume name from map | 0xc0000000 | **1, retry 3** | 3 | 0x80 |

Notes:
- **Buffering**: the scan/data path (`CVolumeDevice`, `CWindowsStorageDevice`) is **plain
  buffered, synchronous** I/O — no `FILE_FLAG_NO_BUFFERING`, no `FILE_FLAG_OVERLAPPED`
  anywhere in the MOM read path. The only `NO_BUFFERING|OVERLAPPED` open is DRWUI's
  disk-info handle (`DRWUI_decompiled.c:176880`), which is used for IOCTLs (geometry
  `0x70000` at `:176887`, `STORAGE_QUERY_PROPERTY` `0x2d1400` via `FUN_1400c82e0`
  `:178061-178072`), not bulk reads.
- **Alignment**: because the data path is un-buffered-flag-free, no buffer/size/offset
  alignment constraints are imposed at the Win32 layer; alignment is purely
  sector-granularity at the API level (§2.2).
- **Share retry**: `Device_decompiled.c:25481-25484` tries share=1 first, then relaxes to
  share=3 on failure (`FUN_18002b890`).

### 2.2 The read loop — `SetFilePointer` + chunked synchronous `ReadFile`

`CVolumeDevice::ReadSectors` (`FUN_180062600`, `Device_decompiled.c:56961-57038`):

```c
// args: (this, buffer, startSector, sectorCount); sectorSize = this->+0x14
byteOff = startSector * sectorSize;                     // :57007
if (high32(byteLen_for_count) != 0) return 0;           // :57004-57006  32-bit length guard
if (!SetFilePointer(h, low32(byteOff), &high32(byteOff), FILE_BEGIN))  // :57010
    { GetLastError(); return 0; }                       // :57013-57015
do {                                                    // :57018-57028
    chunk = min(remaining, maxSectors * sectorSize);    // maxSectors = this->+0x18  :57020
    ok = ReadFile(h, buf, chunk, &n, NULL);             // SYNCHRONOUS, no OVERLAPPED :57023
    total += n; remaining -= n; buf += n;
} while (ok && n != 0);
GetLastError();                                         // :57029 (post-loop error capture)
return total / sectorSize;                              // :57031 — returns SECTORS read
```

- Positioning is 64-bit (`SetFilePointer` with high-dword pointer), `FILE_BEGIN` semantics.
- The loop **continues across partial reads** and stops on hard failure or EOF.
- The return value converts bytes back to sectors — callers speak sectors, the Win32 layer
  speaks bytes.

**Chunk sizing (the og's 4/7 MiB constants):**

- `CVolumeDevice` ctor (`FUN_180062480`, `:56903-56914`): sector size stored at `+0x14`;
  **`+0x18 = 0x700000 / sectorSize`** (`:56912`) — max sectors per `ReadFile` =
  **7 MiB / sector size** (14,336 × 512B). Copy-ctor identical (`:56928`).
- `CWindowsStorageDevice` geometry init (`FUN_180061890`, `:56503-56551`): after
  `IOCTL_DISK_GET_DRIVE_GEOMETRY (0x70000, out 0x18)` `:56523`,
  `IOCTL_DISK_GET_PARTITION_INFO_EX (0x74004)` `:56526`,
  `IOCTL_DISK_GET_LENGTH_INFO (0x7405c, out 8)` `:56532`, with **512 fallback when
  BytesPerSector==0** (`:56535-56537`), it sets **`+0x10 = 0x400000 / sectorSize`**
  (`:56545`) — **4 MiB** chunking for the disk-level device.
- Base `CWindowsDevice` defaults sector size to 512 (`FUN_18005f7f0`, `:55587`:
  `*(uint*)(this+0x38) = 0x200`).

### 2.3 Volume opens by letter / GUID (mounted volumes)

- Mounted-healthy volumes are read via **normal file API**
  (`architecture/05-scan-engine-architecture.md:120-124` §8 "Volume IO layers": "normal
  file API when volume is mounted & healthy"). Engine-side example: FAT engine seeks a
  file handle to EOF for sizing (`re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c:3387`).
- Volume-by-device-object name: `FUN_1800628e0` (`:57067-57110`) queries the device
  object's name through the storage stack (vtable chain `+0x38→+0x40→+0x68→+0x98`,
  `:57083-57087`), rewrites `\??\…` to `\\?\…` (`'?'→'\'` at `:57090-57091`), strips the
  trailing separator, opens with **GENERIC_READ|WRITE, share 7 (R|W|DELETE),
  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL** (`:57101`).
- Destination validation opens the destination **volume letter raw**:
  `architecture/04-recovery-workflow.md:137-138` ("opens `\\.\X:` raw volume of destination
  (writability + same-volume detection)"), plus the writable-most-free-space picker at
  `DRWUI_decompiled.c:318409-318427` (`\\.\%c:` + `IOCTL_DISK_IS_WRITABLE 0x70024` +
  `GetDiskFreeSpaceExA`).

### 2.4 Error handling and retry behavior (direct path)

- Open failure → error code stashed, object marked closed, **NULL propagated upward**
  (§1.3); IOCTLs on a closed device return error 5 (`:55635-55637`).
- `IOCTL_DISK_GET_DRIVE_LAYOUT (0x7400c)` retry-on-buffer-too-small:
  `FUN_180056f40` (`:50691-50730`) starts at **1000 bytes** and grows by
  **0x280 (640 = one `DRIVE_LAYOUT_INFORMATION_EX` entry)** while
  `GetLastError()==0x7a (ERROR_INSUFFICIENT_BUFFER)` (`:50722-50723`).
- Geometry failure path: if the handle open succeeded but geometry IOCTLs fail, the device
  self-closes (`FUN_180061740`, `:56469-56472`: reopen-guard resets `+0x1c=0`).
- Writability probe short-circuits on `ERROR_WRITE_PROTECT (0x13)`:
  `Device_decompiled.c:52986-52997`.

---

## §3 The driver-access pattern in exact detail

### 3.1 The driver family and deployment

One driver source (`mod.windiskaccessdriver`, PDB `epmntdrv.pdb`), four filenames:
`eprdtdrv.sys` (live, installed by `PartitionRecovery/setupdrvx64.exe`),
`eprdtdrv_win7older.sys` (legacy build for Win7−), `ebrntdrv.sys`/`epmdkdrv.sys`
(WinPE preloads, demand-start via `regdrv.reg`)
(`architecture/02-kernel-driver-architecture.md:9-27`;
`re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:102-106`).
Control device `\Device\EPMNTDRV` + `\DosDevices\EPMNTDRV`; device names are
**runtime-composed from the service name** (uppercased last registry-path component), so
`eprdtdrv2` → `EPRDTDRV2` — the `%s%d` numbered-variant scheme for in-place upgrades
(`re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md:9-39`).

### 3.2 Path grammar + PID-token handshake

```
\\.\{SYM}\<PID-TOKEN>\Volume\<N>             → \Device\HarddiskVolume<N> (top of stack)
\\.\{SYM}\<PID-TOKEN>\<disk#>\<partition#>   → \Device\Harddisk{d}\Partition0 (whole disk)
```
`epmntdrv-trio--naming-pid-handshake.md:91-101`. `{SYM}` ∈ {EPRDTDRV, EBRNTDRV, EPMDKDRV, …}.

**PID token** (anti-abuse bind — the driver re-derives it from the *opening* process):
```
b0..b3 = bytes of GetCurrentProcessId()
token  = b0 | (b1+b0)<<8 | (b2+b1)<<16 | (b3+b2)<<24     // 8-bit wraps
```
Driver side: CREATE dispatch, first numeric path component must equal the token else
`STATUS_INVALID_PARAMETER` (`epmntdrv-trio--naming-pid-handshake.md:61-73`).
Client side (Device.mo): same arithmetic in place, then
`sprintf(path, "\\\\.\\EPRDTDRV\\%d\\Volume\\%d", token, volumeN)` —
`Device_decompiled.c:342-355` (volume site) and `:7806-7819` (disk site); byte-exact
both sides per the artifact (`:75-89`).

Disk-branch resolution in the driver: disk# must be < 200; open
`\Device\Harddisk{d}\Partition0`, on failure retry `\Device\Harddisk{d}\DR{p}` ("retry"),
then give up ("that's it!"); then walk **down** the stack with `IoGetLowerDeviceObject` +
`ObQueryNameString` requiring a `\Device\Harddisk` name and a driver named **`Disk`**
(disk.sys) — refuses non-disk stacks (`epmntdrv-trio--naming-pid-handshake.md:103-123`).

### 3.3 IOCTLs

Custom codes (all `CTL_CODE(0x600, f, METHOD_BUFFERED, FILE_ANY_ACCESS)`)
(`re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:75-84`):

| IOCTL | Meaning |
|---|---|
| `0x60002400` | set/resolve the *global* target (no-file-object mode) |
| `0x60002404` | GetFileObjName: DWORD handle in → ≥0x208 name out (`ObReferenceObjectByHandle`+`ObQueryNameString`) |
| `0x60002408` | GetDiskContextMode: 4-byte mode out |
| anything else | **transparent pass-through** to the disk stack (geometry, SCSI passthrough, …) |

Plus an undocumented `IRP_MJ_DIRECTORY_CONTROL` stub that swallows directory queries
(`epmntdrv-trio--naming-pid-handshake.md:56-57`).

### 3.4 The read contract through the driver

`IRP_MJ_READ` (`eprdtdrv_decompiled.c:671-744`, summarized in
`eprdtdrv-sys--driver-internals.md:52-73`):

- User-mode `ReadFile` on the EPRDTDRV handle: **file offset = absolute byte offset**
  (LBA×512), **length = count of 512-byte sectors**.
- Driver maps the user buffer via `MmMapLockedPagesSpecifyCache` (MDL, DO_DIRECT_IO),
  builds `IoBuildAsynchronousFsdRequest(IRP_MJ_READ, target, buffer, Length, &ByteOffset)`,
  sets completion routine, `IofCallDriver`, waits on event if pending
  (`eprdtdrv_decompiled.c:719-731`).
- On success: `IoStatus.Information = Length << 9` (`:732-733`) — the 512-byte sector is
  **hardcoded**; 4Kn/512e normalization must happen above the driver
  (`eprdtdrv-sys--driver-internals.md:73`).
- Client-side dispatch in Device.mo: `FUN_18004bd80` (`:45046-45077`) → `FUN_18004ea80`
  (`:46378-46450`) — a name→context cache (global manager at `0x1800aa100`, map at
  `+0x20`), volume/disk backend objects at `+0x38`/`+0x40`, invoked with
  `(startSector × 0x200, count × 0x200)` in the disk branch (`:46429`, `:46507` — again
  hardcoded 512).
- **Driver-path IOCTL emulation**: when IO goes through the driver, Device.mo emulates the
  disk IOCTLs in user mode — geometry/partition/layout/length from cached state
  (`FUN_18004c0f0`, `:45138-45220`: 0x70000, 0x70024, 0x74004, 0x7400c, 0x7405c, 0x7c010
  SET_DRIVE_LAYOUT, 0x7c054/0x7c058, 0x2d1400 STORAGE_QUERY_PROPERTY). Notably
  **`FSCTL_DISMOUNT_VOLUME (0x90020)` returns fake success** (`:45216`: `local_24 = 1`)
  while `FSCTL_LOCK_VOLUME (0x90018)` / `FSCTL_UNLOCK_VOLUME (0x9001c)` fall through to
  failure (`:45194-45207`) — through the raw-driver path you cannot really lock a volume,
  so the layer pretends dismount worked.

### 3.5 Driver lifecycle on live Windows (who starts it, when)

- The PartitionRecovery entry path serializes driver setup with a global gate
  (`while (0 < flag) Sleep(1000); flag++;` then `FUN_1800044d0("eprdtdrv")`) —
  `re-workspace/reversed/ghidra-partition/PartitionRecovery_decompiled.c:2897-2901`.
- `FUN_1800044d0` (`:2593-2632`): `OpenSCManagerA(NULL,NULL,SC_MANAGER_ALL_ACCESS 0xf003f)`
  → `OpenServiceA(scm,"eprdtdrv",SERVICE_ALL_ACCESS 0xf01ff)` → `StartServiceA`;
  `ERROR_SERVICE_ALREADY_RUNNING (0x420)` counts as success; **if the service does not
  exist it returns 0 and the caller proceeds anyway** (no install, no rethrow).
- The actual **installer** is `PartitionRecovery/setupdrvx64.exe` (PDB
  `mod.WinDiskAccessDriverSetup\setupempdrv03`): full SCM lifecycle, copies to
  `\System32\Drivers\%s.sys` / `\SystemRoot\System32\drivers\%s.sys` /
  `%s\System32\Drivers\%s%d.sys` (numbered coexisting variants), service group
  "Boot File System", `-Uninstall` mode
  (`re-workspace/artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md:32-38`).
- In WinPE the drivers are pre-registered by `regdrv.reg` (epmdkdrv/ebrntdrv demand-start;
  EUDCPEPM BOOT_START disk-class UpperFilter; EUEDKEPM SYSTEM_START)
  (`architecture/03-winpe-boot-environment.md:45-54`).
- **When is the driver preferred?** Only on live Windows (the IsWinPE==0 branch, §1.2) —
  where raw `\Device\Harddisk*` access still needs elevation and the driver provides (a)
  a stable namespace independent of mount state, (b) the PID-token anti-abuse bind, (c)
  IOCTL pass-through without re-opening devices. In WinPE the og **bypasses its own
  driver entirely** and goes direct.
- EUDCPEPM/EUEDKEPM (the WinPE hook pair) are for the EPM/Todo-Backup **disk-copy**
  lineage (volume lock/copy); EUEDKEPM has **no in-tree opener** — dormant in this product
  (`re-workspace/artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md:250,280`;
  `architecture/02-kernel-driver-architecture.md:87-90`).

---

## §4 The privilege / elevation model

- **DRW.exe (loader): `requireAdministrator`** — manifest literal at
  `re-workspace/reversed/DRW.exe/strings_ascii.txt:355`.
- **DRWUI.exe (main UI): `requireAdministrator`** —
  `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:41023`.
- DRWUI is launched by DRW.exe via `CreateProcessA("DRWUI", inheritHandles=TRUE)`
  (`re-workspace/artifacts/DRW-exe--launcher.md:31`) — the whole user-mode stack
  (Device.mo opens, PhysicalDrive opens, SCM calls, EPRDTDRV opens) inherits the elevated
  token. **One UAC prompt at launch; nothing else prompts.**
- **DRWService.exe: `asInvoker`** (`re-workspace/reversed/DRWService.exe/strings_ascii.txt:201`)
  — it runs as SYSTEM via the SCM (AUTO_START "Read FVE Server" for BitLocker keys;
  `re-workspace/artifacts/DRWService-exe--bitlocker-service.md`).
- Setup pair: SetupUE.exe and InfoForSetup.exe are `asInvoker`
  (`re-workspace/reversed/SetupUE.exe/strings_ascii.txt:354`,
  `re-workspace/reversed/InfoForSetup.exe/strings_ascii.txt:396`) — installer elevation is
  handled by the Inno bootstrap chain, out of scope of the runtime access path.
- The SCM calls in the driver-ensure path demand `SC_MANAGER_ALL_ACCESS`/`SERVICE_ALL_ACCESS`
  (`PartitionRecovery_decompiled.c:2606-2611`) — feasible only elevated.
- **No in-app elevation prompt, no runas relaunch, no "not elevated" refusal code was
  found anywhere in the corpus.** The product's model is: the OS refuses to start the app
  at all without admin (manifest gate), and in WinPE it runs as SYSTEM where UAC is absent
  (`architecture/03-winpe-boot-environment.md:40-43` boot chain). There is **no degraded
  non-admin mode in the live product** — see §8 C-1 for the artifact-phrasing that
  suggested otherwise.

---

## §5 Edge-state behavior table

| Edge state | og behavior | Citation |
|---|---|---|
| Device absent / not ready | Direct open of `\\.\PHYSICALDRIVE%d` + `IOCTL_DISK_GET_DRIVE_GEOMETRY`; on `GetLastError()==0x15 (ERROR_NOT_READY)` → close handle, bail out of disk-info init | `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:176880-176891` |
| Drive removed mid-scan | UI: **"Drive Disconnected" — "Unable to continue scanning. Reconnect your drive and try again."** with Retry/Cancel (modal `CScanCommonPopDlg` family, CDetailPage region) | `re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:42` |
| NAS offline / Linux box offline / cloud session lost | Parallel dialogs ("NAS Disconnected", "Disconnect with Linux", "Cloud Drive Disconnected") with Retry/Cancel/Disconnect | `re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:43-46` |
| No devices at all (home) | Empty groups + **"Drive Not Show Up"** tile + Refresh | `re-workspace/artifacts/UIUX/07-home-phase-deep-dive.md:53` |
| Volume mounted & healthy | Normal file API reads (no raw path needed) | `architecture/05-scan-engine-architecture.md:120-124` |
| Volume unmounted / damaged | Raw path: EPRDTDRV `\Volume\{N}` (live) or `\\.\HarddiskVolume%d` (PE) — volume device objects, bypasses FS stack; lost partitions via `CPartitionVirtualVolumeDevice` | `eprdtdrv-sys--driver-internals.md:32-38`; `epmntdrv-trio--naming-pid-handshake.md:94`; `architecture/05-scan-engine-architecture.md:120` |
| Volume in use (scan) | Scan opens are **shared** (share 3 / share 7) — coexists with the running OS, no lock during scan | `Device_decompiled.c:57101`, `:56456`; `DRWUI_decompiled.c:318410` |
| Volume lock/dismount needed (partition ops, writes) | Real lock via pending-op engine ("[==dbg==]lock volume start/end", op 0x2003); in PE, EUDCPEPM hook suppresses foreign IO; through the driver path `FSCTL_DISMOUNT_VOLUME` is **faked success**, LOCK/UNLOCK fail | `re-workspace/reversed/ghidra-partition/MainModule_decompiled.c:79123-79159`; `architecture/02-kernel-driver-architecture.md:87`; `Device_decompiled.c:45194-45216` |
| Bad sectors | Engine-level skip-and-salvage: EXT4 `CEXT4CatchBufferDeviceProxy` retries with fresh 0xE00 buffer and **1 KB-backtrack shifted window**; UI copy **"Program is trying to skip bad sectors to find data."** (BadSectorTip) + `nTipsDlg` coaching | `re-workspace/artifacts/SearchDLL/ext4-engine--deep-dive.md:25-35`; `re-workspace/artifacts/config-files/config-data-reference.md:55` |
| Write-protected media | `IOCTL_DISK_IS_WRITABLE (0x70024)`; `ERROR_WRITE_PROTECT (0x13)` short-circuits and closes | `Device_decompiled.c:52986-52997`; `DRWUI_decompiled.c:318413-318417` |
| BitLocker locked volume | Unlock dialogs (`BitlockerUnlockDialog` etc.), FVE keys via `\\.\Pipe\DRWBitlocker` (DRWService) + manage-bde fallback, XTS-AES decrypt in BitLockerLib; icons lock→unlock; scan proceeds on unlocked volume | `architecture/04-recovery-workflow.md:182-183`; `re-workspace/artifacts/UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:33` |
| Sector-size variants (512e/4Kn) | Geometry-driven: sector size from `IOCTL_DISK_GET_DRIVE_GEOMETRY`, **512 fallback when zero**; chunk sizes recomputed (0x400000/0x700000 ÷ sectorSize); **driver path and driver read contract hardcode 512** (len<<9) — 4Kn must be normalized above the driver | `Device_decompiled.c:56523-56545`, `:55587`; `eprdtdrv-sys--driver-internals.md:73`; `Device_decompiled.c:46429` |
| Disk # ≥ 200 | Driver rejects (`disk# > 199`); DRWUI info layer caps at 32 disks (`0x1f` check) | `epmntdrv-trio--naming-pid-handshake.md:107`; `DRWUI_decompiled.c:176873-176874` |
| Win7 vs Win8+ vs Win11 | Separate legacy driver build `eprdtdrv_win7older.sys` for Win7−; WMI `MSFT_*` storage namespace used for enumeration (Win8+/Server2012+ storage mgmt API); install-time 16-OS gate in SetupUE; WinPE edition has zero .mo modules (feature-scoped, not OS-gated) | `architecture/02-kernel-driver-architecture.md:12,27`; `Device_decompiled.c:17024+`; `re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md:29`; `re-workspace/artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md:49-53` |
| Driver open fails (live) | Silent NULL (§1.3): error recorded, device closed, IOCTLs → error 5; no fallback to direct at this layer | `Device_decompiled.c:56460-56463`, `:55635-55637`, `:1430-1450` |
| Cross-process abuse of raw path | PID-token handshake rejects foreign processes at CREATE (`STATUS_INVALID_PARAMETER`) | `epmntdrv-trio--naming-pid-handshake.md:59-89` |

---

## §6 Performance characteristics

- **Chunking — the og's two constants:**
  - Disk-level reads (`CWindowsStorageDevice`): **4 MiB** per `ReadFile`
    (`0x400000 / sectorSize` max sectors) — `Device_decompiled.c:56545`.
  - Volume-level reads (`CVolumeDevice`): **7 MiB** per `ReadFile`
    (`0x700000 / sectorSize`) — `Device_decompiled.c:56912`, read loop `:57018-57028`.
  - Both are *sector-size normalized*: on a 4Kn device the sector count shrinks so the
    byte chunk stays 4/7 MiB.
- **Engine-side read amplification control**: `CStorageDeviceWithCache` (NTFS engine RTTI)
  wraps the host-provided `IStorageDevice` with a **cluster-aligned cache** built from the
  boot sector (512-byte boot read, BPB parse, cluster-sized units, critical-section
  guarded) — `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:7165-7169`,
  init `:11752-11846`.
- **MFT bootstrap**: first MFT record region read into a **64 KB** stack buffer
  (`aiStack_10038` = 16,388 dwords) —
  `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:44`.
- **Layout IOCTL adaptivity**: drive-layout buffer starts at 1000 B, grows +640 B per
  `ERROR_INSUFFICIENT_BUFFER` retry — `Device_decompiled.c:50704-50728`.
- **Parallelism**: no parallel raw-read engine found. Concurrency is at the *engine/UI*
  level — structured scan and RAW carving "run concurrently"
  (`architecture/04-recovery-workflow.md:73-74`), engines run on `CDoScan*` strategy
  threads with a cooperative `CStopScanThread`
  (`re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:51-55`;
  `re-workspace/artifacts/DRWUI-exe--thread-architecture.md:12-21` — the 45-thread
  census contains no IO/reader pool). Sector reads are serialized through the single
  per-volume device object (its only synchronization is a critical section in the cache
  wrapper).
- The only `FILE_FLAG_NO_BUFFERING|FILE_FLAG_OVERLAPPED` handle (DRWUI disk-info) is
  IOCTL-only — the og never does buffered-flag direct bulk reads
  (`DRWUI_decompiled.c:176880` + `:176887`, `:178061-178072`).

---

## §7 What this means for the core's direct-only replacement

1. **The og itself is the precedent.** Its PE edition runs *entirely* on direct
   `\\.\PhysicalDrive%d` / `\\.\HarddiskVolume%d` user-mode I/O under admin privilege —
   the same product, same engines, same UI — with the driver path simply not taken
   (`Device_decompiled.c:7816-7823`). A direct-only core is behaviorally faithful to a
   real og deployment mode, not a novelty.
2. **Copy the og's open recipe for the data path**: `CreateFileA/W(path,
   GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
   FILE_ATTRIBUTE_NORMAL, NULL)` — buffered, synchronous, no OVERLAPPED, no NO_BUFFERING
   (`Device_decompiled.c:56456`, `:57101`; `DRWUI_decompiled.c:178062`). This works
   un-mounted and in-use (share modes allow coexistence) and imposes no alignment
   constraints beyond sector granularity.
3. **Chunk at 4 MiB (disk) / 7 MiB (volume)**, sector-size-normalized, with the
   partial-read-continuing loop and bytes→sectors return contract of `FUN_180062600`
   (`:56961-57038`) — matching the core worklog's "4/7 MiB og chunking" note.
4. **Keep a writability probe** (`IOCTL_DISK_IS_WRITABLE`, short-circuit on
   ERROR_WRITE_PROTECT) and the **ERROR_NOT_READY → device-absent** bail for the
   device-enumeration layer (`:52986-52997`; `DRWUI_decompiled.c:176887-176891`).
5. **Volume-vs-disk opens**: volumes via `\\.\HarddiskVolume%d` / device-object GUID names
   (share 7 when opened by object name, `:57101`); disks via `\\.\PhysicalDrive%d`.
   The og's `\\?\Volume{GUID}` rewrite (`:57090-57099`) is the template for GUID opens.
6. **What the core loses by dropping the driver** (be explicit in docs): the PID-token
   anti-abuse bind, the 200-disk namespace, IOCTL pass-through without re-open, and the
   fake-success `FSCTL_DISMOUNT_VOLUME` emulation (`:45216`). The core's direct path must
   decide its own story for lock/dismount (the og's real locking lives in the pending-op
   engine + PE hook driver, not in the access layer).
7. **Edge-state parity targets**: Drive-Disconnected dialog copy, bad-sector skip+
   1 KB-backtrack retry, ERROR_NOT_READY handling, 512 fallback sector size, layout-IOCTL
   growing-buffer retry (§5, §6).
8. **Privilege parity**: single launch-time `requireAdministrator` (inherited by children)
   with no runtime re-prompt and no degraded mode is the og's whole model — the core's
   "direct under admin privilege" matches it exactly, minus the SCM dependency
   (§4; `DRW.exe/strings_ascii.txt:355`, `DRWUI.exe/strings_ascii.txt:41023`).

---

## §8 Corrections / uncertainty register

**CORRECTIONS.md status**: the corpus errata file
(`re-workspace/artifacts/CORRECTIONS.md`, 9 entries) contains **no rows about disk
access** — none of the corrected claims (NTFS/FAT/ext4/Btrfs/FileFilter/EUUserData)
touch the access paths in this report, so no citation above is superseded.

| # | Item | Status / correction |
|---|---|---|
| C-1 | `epmntdrv-trio--naming-pid-handshake.md:145-151` (§6) calls the direct fallback "the 'no-admin' degraded mode visible in the UI flows" triggered "when the driver-probe fails" | **Corrected reading (this report)**: `FUN_1800577e0` is an `IsWinPE()` probe (X: system dir + RAM-disk extents + `[Public] IsWinPE` config override — fully-typed twin at `RecoveryModule_decompiled.c:42737-42801`). The direct branch fires when `IsWinPE() != 0` — i.e. **in WinPE**, where the process is SYSTEM. There is no no-admin live mode: DRWUI is `requireAdministrator` (`DRWUI.exe/strings_ascii.txt:41023`). The *code* cited by the artifact is correct; its *interpretation* is not. |
| C-2 | Same artifact §5 (`:137-143`) infers PE "failover" to ebrntdrv/epmdkdrv via retry logic | Marked INFERRED by its own §8. In tension with the verified gate (PE → direct, no driver open at all) and with the PE payload census (zero `.mo` modules under BUILDPE — `re-workspace/artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md:49`). Treat the "PE spare-instance failover" story as **unverified**; the two PE driver copies more plausibly serve the EPM-lineage PE builds that do carry PartitionRecovery (shared-monorepo inference). |
| C-3 | Ghidra mis-types `FUN_1800577e0` as `void` (`Device_decompiled.c:50943`) while callers test its return | Direction of the gate established from three independent lines: (a) the fully-decompiled RecoveryModule twin (return-variable structure), (b) DeviceManager's `if (FUN_180012aa0() != 0) { PE drive-letter logic }` (`DeviceManager_decompiled.c:12757-12758` + `RegFlushKey` context), (c) the X:-disk check body itself. Confidence high; the exact truth-table of the extents `DiskNumber == -1` test is not fully pinned. |
| C-4 | DRW's own PE build ships no `.mo` modules — so Device.mo's WinPE→direct branch is dead code *in DRW's PE edition* | Fact (`pe-payload-census--buildpe-easeus-x64.md:49-53`). The branch executes in EPM/partition-master-lineage PE environments that include PartitionRecovery (inference from shared PDB/build tree `i:\_drw_main\partitionrecovery\...`, `Device.md:33`). The gate's *existence and mechanics* remain verified code in the shipped Device.mo. |
| C-5 | Driver-path disk-open access/share modes | The disk-path `\\.\EPRDTDRV\{token}\{d}\{p}` open goes through MOM functor indirection (`FUN_180006a50`→clone-object ctor) whose access/share binding could not be fully resolved; the **volume** driver-path open is verified GENERIC_READ/share-0 (`Device_decompiled.c:360-363` + artifact `:82`). Direct-path flags are fully verified (§2.1 table). |
| C-6 | "rda / RDA" (task vocabulary) | No hits anywhere in the corpus (`rg -w rda|RDA` over artifacts + architecture → 0). Presumably core-repo internal terminology; no corpus claim depends on it. |
| C-7 | 4Kn behavior above the driver | `Device_decompiled.c:46429` hardcodes `×0x200` in the driver-path disk read dispatch and the driver does `len<<9`; user-mode geometry normalization exists (`:56523-56545`), but no corpus artifact demonstrates an end-to-end 4Kn read through the driver path. Treat og-on-4Kn-through-driver as **unverified**; the direct path is geometry-driven and clean. |
| C-8 | Volume-in-use semantics through the driver path | `FSCTL_DISMOUNT_VOLUME` fake-success / LOCK-UNLOCK failure is verified code (`:45194-45216`) but the artifact set never names it; noted here as a first-reading, flagged for re-verification if the core ever emulates it. |

---

*Corpus untouched. One file written: this one.*
