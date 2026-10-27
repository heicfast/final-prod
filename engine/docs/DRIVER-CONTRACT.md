# RDA Driver — User-Mode Contract (DRIVER-CONTRACT)

> Evidence base: rda-driver repo @ `da7efb9e0c94046893c62b25695bb95edf519f8b` (branch `main`, tag `v0.2.2`), all 66 tracked files read end-to-end. Every claim below cites `file:line` relative to the rda-driver repository root. Consumer of this document: core_recovery's driver-status surface and CI prebuilt-artifact fetch (see sibling `DRIVER-RELEASE.md`).
>
> RDA is a Windows kernel-mode WDM driver ("Raw Disk Access") that merges, wire-compatibly, the contracts of three reverse-engineered reference drivers: EPMNtDrv (`eprdtdrv.sys`), EUEDKEPM (`EuDskAcs.sys`), EUDCPEPM (`EuBackup.sys`) — `src/shared/inc/rda_ioctl.h:6-12`.

---

## 1. Device naming

The driver creates **one named control device** plus a symbolic link; both names derive from the **service name** at load time (reference-exact behavior):

- Device object name: `\Device\<UPPERCASE(last path component of the DriverEntry RegistryPath)>`, fallback literal `EPMNTDRV` — `src/driver/rda_control.c:4-10`, `src/driver/rda_control.c:96-101`; prefixes `\Device\` and `\DosDevices\` and fallback constant at `src/driver/inc/rda_p.h:46-50`.
- DOS symbolic link: `\DosDevices\<same base>` created via `IoCreateSymbolicLink` — `src/driver/rda_control.c:162`; deleted at unload (`src/driver/rda_control.c:190-195`).
- Override: registry `DeviceName` (service-key root or `Parameters\`) replaces the derived base (`EUEDKEPM` parity) — `src/driver/rda_registry.c:120-127`, `src/driver/rda_registry.c:158-162`, `src/driver/rda_control.c:96-97`.
- Device type: `FILE_DEVICE_UNKNOWN`, `DO_DIRECT_IO` — `src/driver/rda_control.c:16-17`, `src/driver/rda_control.c:152-160`.

**User-mode-visible path**: `\\.\<BASE>` (NT ` \DosDevices\<BASE>` → `\Device\<BASE>`). Concrete instances (from `scripts/install.ps1:7-9`, `scripts/install.ps1:79-82`):

| Service name (install) | Device object | User-mode path |
|---|---|---|
| `eprdtdrv` | `\Device\EPRDTDRV` | `\\.\EPRDTDRV` (DRW Device.mo drop-in) |
| `ebrntdrv` / `epmdkdrv` | `\Device\EBRNTDRV` / `\Device\EPMDKDRV` | WinPE spare instances |
| `rda` | `\Device\RDA` | `\\.\RDA` (neutral default) |

The shipped client hard-codes `\\.\EPRDTDRV` (`src/client/rdacli.c:44`); the format templates are `L"\\\\.\\%ws"` and `L"\\\\.\\%ws\\%u\\%u\\%u"` (`src/client/rdacli.c:29-30`). Any service name works — the same `rda.sys` binary answers whatever name it is installed under, and `%s%d` hot-upgrade variants coexist (`src/driver/rda_control.c:8-10`). The core's CI installs it as service `rda` → `\\.\RDA` (cross-ref: core `.github/workflows/api-benchmark.yml` header comment).

Sub-device opens are expressed **in the file name relative to the control device**, not as separate devices: `\\.\EPRDTDRV\<PID-TOKEN>\<disk>\<part>` etc. (`src/client/rdacli.c:53-69`, `src/bench/rdabench.c:61-71`).

---

## 2. Access model (how user mode opens the driver)

**CreateFile semantics** (from the shipped client, which is the reference-grammar consumer):

```c
CreateFileW(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)
```
- `src/client/rdacli.c:45-46` (bare/global open), `src/client/rdacli.c:58-59` (disk open), `src/client/rdacli.c:67-68` (volume open); `src/bench/rdabench.c:67-68` uses `GENERIC_READ | GENERIC_WRITE` + `FILE_FLAG_NO_BUFFERING`.
- Desired access observed in-tree: `GENERIC_READ` for identity/info/read paths (`src/client/rdacli.c:74`), `GENERIC_READ | GENERIC_WRITE` for the session client (`src/client/rdacli.c:234`) and the benchmark tool (`src/bench/rdabench.c:67`).
- Sharing: `FILE_SHARE_READ | FILE_SHARE_WRITE` in every in-tree client; the driver imposes **no sharing restriction of its own** (no exclusivity flag — `Exclusive=FALSE` at `src/driver/rda_control.c:152`).
- Handles are **synchronous** (no `FILE_FLAG_OVERLAPPED` in rdacli; the core's transport uses OVERLAPPED per its own audit) — the IO manager adds SYNCHRONIZE for synchronous handles; the driver itself never checks access bits, it only forwards to the storage stack. READ/WRITE complete synchronously in-line; only GETNEXTBLOCK returns `STATUS_PENDING` (`src/driver/rda_ioctl.c:295-299`, `src/driver/rda_session.c:1500`).

**Handle kinds** (determined at `IRP_MJ_CREATE` by the file-name grammar, `src/driver/rda_create.c:43-121`):

1. **Global/bare handle** — `CreateFile("\\.\RDA")`, empty relative name: succeeds with no target context (`RDA_OPEN_CLASS_GLOBAL`, `src/driver/rda_create.c:60-65`). Serves identity IOCTLs, extended IOCTLs, session protocol, and legacy global-handle read/write modes.
2. **Target-bound handle** — name carries the PID-token grammar; context (referenced target device + file object, mode, probed sector size) is stored in `FsContext`/`FsContext2` (`src/driver/rda_create.c:112-116`; context layout `src/driver/inc/rda_p.h:81-95`).

**Handle count / per-process restrictions**: no limit on the number of handles; every open re-derives the PID token from the *calling* process, so a path string built by another process fails (`src/driver/rda_create.c:71-77`). Sessions are owned by the creating process and destroyed on control-device `IRP_MJ_CLEANUP` (sweep, `src/driver/rda_create.c:159-162`). Resources are released at CLEANUP (fix F9), CLOSE completes `STATUS_SUCCESS` (`src/driver/rda_create.c:138-179`).

**Dispatch table** (control device; everything else → `STATUS_INVALID_DEVICE_REQUEST`): `IRP_MJ_CREATE`@0x00, `IRP_MJ_CLOSE`@0x02, `IRP_MJ_READ`@0x03, `IRP_MJ_WRITE`@0x04, `IRP_MJ_DEVICE_CONTROL`@0x0E, `IRP_MJ_CLEANUP`@0x12 — `src/driver/rda_driver.c:62-95`, default completion at `src/driver/rda_driver.c:50-60`. A single master dispatcher routes control vs filter devices (`src/driver/rda_driver.c:97-111`).

---

## 3. Open grammar and PID token (auth handshake)

Grammar (relative to the control device; all compared over non-NUL-terminated ranges — `src/shared/inc/rda_compat.h:11-26`):

| Form | Class | Semantics |
|---|---|---|
| *(empty name)* | `RDA_OPEN_CLASS_GLOBAL` (1) | bare/global handle, no target — `src/shared/inc/rda_compat.h:177-195` |
| `\<token>\Volume\<N>` | `RDA_OPEN_CLASS_VOLUME` (2) | opens `\Device\HarddiskVolume<N>` top-of-stack, mode 0 — `src/driver/rda_target.c:129-167` |
| `\<token>\<disk>\<part>` | `RDA_OPEN_CLASS_DISK` (3) | EPMNtDrv 3-segment: `\Device\Harddisk{d}\Partition0` with `\Device\Harddisk{d}\DR{d}` fallback (disk number as **both** varargs), walk down to first `\Device\Harddisk`-named layer; **the part number is never used** — `src/driver/rda_target.c:169-245` |
| `\<token>\<disk>` | `RDA_OPEN_CLASS_DISK_EPM` (4) | EUEDKEPM 2-segment: `Partition0` only, walk to first `\Driver\Disk` layer — `src/driver/rda_target.c:247-300` |

**PID token transform** (`src/shared/inc/rda_ioctl.h:110-119`, implemented `src/shared/inc/rda_compat.h:49-57`):

```
b0..b3 = little-endian bytes of the caller's 32-bit PID
token  = b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24   (8-bit wrapping adds)
```

The driver re-derives it from `PsGetCurrentProcessId()` at CREATE time; mismatch → `STATUS_INVALID_PARAMETER` (`src/driver/rda_create.c:73-77`). It is explicitly *not* a security boundary — the device SDDL is (`src/shared/inc/rda_ioctl.h:117-119`).

**Numeric bounds**: DISK kind requires `disk# < MaxDisks` (default 200, registry `Parameters\MaxDisks` clamped 2..1024); DISK_EPM requires `disk# < 100` (hard-coded) — `src/driver/rda_create.c:79-85`, `src/driver/inc/rda_p.h:55-56`, `src/driver/rda_registry.c:137-141`. Volume-number parse failure falls back to **volume 0** (reference-exact) — `src/shared/inc/rda_compat.h:244-253`.

**Structured alternative (no string grammar)**: `IOCTL_RDA_OPEN_TARGET` binds a target explicitly (see §4.2) — `src/driver/rda_ioctl.c:112-184`.

---

## 4. IOCTL surface

Device type for custom codes: `RDA_DEVICE_TYPE 0x600` (`src/shared/inc/rda_ioctl.h:51`). All custom/identity/extended IOCTLs are `METHOD_BUFFERED`/`FILE_ANY_ACCESS` (macro `CTL_CODE_RDA` at `src/shared/inc/rda_ioctl.h:72`; reference codes derived as `CTL_CODE(0x600, 0x900..0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)` at `src/shared/inc/rda_ioctl.h:54-58`).

**Dispatch order** in `RdaDispatchDeviceControlInner` (`src/driver/rda_ioctl.c:247-286`):
1. Session-protocol codes (normalized `code - IoctlBase`) — served on **any** handle when `SessionsEnabled != 0` (`src/driver/rda_ioctl.c:257-260`).
2. Target-bound handles: `IOCTL_RDA_GET_DISK_CONTEXT_MODE` or **pass-through** of everything else to the target's top-of-stack (`src/driver/rda_ioctl.c:263-268`).
3. Global handles: identity + extended IOCTLs; anything else → `STATUS_INVALID_PARAMETER` (`src/driver/rda_ioctl.c:271-285`).

### 4.1 Reference identity IOCTLs (EPMNtDrv contract — exact codes)

| IOCTL | Code | In / Out gates | Semantics |
|---|---|---|---|
| `IOCTL_RDA_GET_PROC_NAME` | `0x60002400` | in == 4 (u32 PID); out ≥ 0x208 | Image path of any PID: `PsLookupProcessByProcessId` + `SeLocateProcessImageName`, NUL-terminated WCHAR into SystemBuffer; `Information = pathBytes + 2` — `src/shared/inc/rda_ioctl.h:60,65-66`, `src/driver/rda_ident.c:46-82`, copy helper `src/driver/rda_ident.c:28-44`. Lookup failure propagates the lookup NTSTATUS (`src/driver/rda_ident.c:62-66`). |
| `IOCTL_RDA_GET_FILE_OBJ_NAME` | `0x60002404` | in == 4 (u32 HANDLE); out ≥ 0x208 | Fully-qualified NT name of a file HANDLE: `ObReferenceObjectByHandle(..., *IoFileObjectType, KernelMode)` → `ObQueryNameString(DeviceObject)` + append `FileName`; `Information = bytes + 2` — `src/driver/rda_ident.c:150-161`, core at `src/driver/rda_ident.c:84-148`. |
| `IOCTL_RDA_GET_DISK_CONTEXT_MODE` | `0x60002408` | out == 4 exactly | Writes the per-handle `Mode` LONG (0 when the chosen layer's driver is `\Driver\Disk`, 1 otherwise — comparison is against `DriverExtension->ServiceKeyName`, reference-exact, effectively constant 1 for disk opens) — `src/driver/rda_ident.c:163-174`, `src/driver/inc/rda_p.h:87-89`, `src/driver/rda_target.c:76-98`. Only meaningful on **bound** handles. |

Gate failures on all three return `STATUS_INVALID_PARAMETER` (`src/driver/rda_ident.c:55-58`, `:155-158`, `:167-170`).

### 4.2 RDA extended IOCTLs (new, clean interfaces; function codes 0x910-0x912 — no collision with 0x900-0x902)

| IOCTL | Code | In / Out | Semantics |
|---|---|---|---|
| `IOCTL_RDA_GET_INFO` | `0x6002440` (wire-truth fix 2026-10-27: the extended codes are CTL_CODE_RDA(fn) = (0x600 << 16) | (fn << 2) macros evaluating to 0x600244x - the driver header's own comments carried a phantom extra zero, which this doc and the core's cr_rda.h inherited; the identity codes 0x600024xx are RAW EPMNtDrv-exact values, correctly NOT derivations) (`CTL_CODE_RDA(0x910)`, `src/shared/inc/rda_ioctl.h:74`) | in: none; out ≥ 24 bytes | `RDA_DRIVER_INFO` (below). `Information = sizeof(RDA_DRIVER_INFO)` — `src/driver/rda_ioctl.c:75-98`. |
| `IOCTL_RDA_OPEN_TARGET` | `0x6002444` (`:75`) | in ≥ 24 (`RDA_OPEN_TARGET_IN`); out == 0 | Structured open; binds target on the current (global) handle. See below — `src/driver/rda_ioctl.c:115-184`. |
| `IOCTL_RDA_GET_VERSION` | `0x6002448` (`:76`) | out ≥ 8 | Writes `ULONG64 = (Major << 32) \| Minor`; `Information = 8` — `src/driver/rda_ioctl.c:100-110`. |

**`RDA_DRIVER_INFO`** (all `UINT32`, 24 bytes — `src/shared/inc/rda_ioctl.h:79-86`):

| Field | Meaning |
|---|---|
| `StructSize` | `sizeof(RDA_DRIVER_INFO)`, for versioning |
| `MajorVersion` / `MinorVersion` | `RDA_VERSION_MAJOR`=1 / `RDA_VERSION_MINOR`=0 (`src/shared/inc/rda_ioctl.h:106-107`) |
| `Flags` | `RDA_INFO_FLAG_FILTER_MODE` 0x1 (PnP filter role active); `RDA_INFO_FLAG_SESSIONS` 0x2 (session engine active) — `src/shared/inc/rda_ioctl.h:88-89`, filled `src/driver/rda_ioctl.c:87-93` |
| `MaxDisks` | configured disk-number bound (default 200) |
| `IoctlBase` | session-protocol code base in effect (default 0; WinPE sets 0x10000) |

**`RDA_OPEN_TARGET_IN`** (all `UINT32`, 24 bytes — `src/shared/inc/rda_ioctl.h:92-99`): `StructSize`, `Kind` (`RDA_TARGET_KIND_VOLUME`=1 / `RDA_TARGET_KIND_DISK`=2 / `RDA_TARGET_KIND_DISK_PART`=3, `src/shared/inc/rda_ioctl.h:101-103`), `DiskNumber`, `PartNumber`, `VolumeNumber`, `Reserved`. Validation (`src/driver/rda_ioctl.c:124-154`): `inLength < sizeof` or `outLength != 0` or NULL SystemBuffer → `STATUS_INVALID_PARAMETER`; `StructSize < sizeof` → `STATUS_INVALID_PARAMETER`; handle already bound → `STATUS_INVALID_DEVICE_STATE`; `DISK` kind ≥ 100 or `DISK_PART` kind ≥ `MaxDisks` → `STATUS_INVALID_PARAMETER`; allocation failure → `STATUS_INSUFFICIENT_RESOURCES`; resolution failures propagate. No PID token required by design — "the device DACL is the security boundary" (`src/driver/rda_ioctl.c:112-114`).

### 4.3 Pass-through IOCTLs (bound handles)

Every non-session, non-`0x60002408` IOCTL on a **bound** handle is forwarded to `IoGetAttachedDeviceReference(Context->TargetDevice)` via `IoBuildDeviceIoControlRequest`, synchronously waited, `Information = iosb.Information` (`src/driver/rda_ioctl.c:186-245`). Transfer-method awareness (fix F10, `src/driver/rda_ioctl.c:25-31`):
- `METHOD_BUFFERED`: out = SystemBuffer (reference parity).
- `METHOD_IN_DIRECT` / `METHOD_OUT_DIRECT`: out = `Irp->UserBuffer`.
- `METHOD_NEITHER`: rejected `STATUS_NOT_SUPPORTED` (`src/driver/rda_ioctl.c:220-224`).
- No attachable top device → `STATUS_INVALID_DEVICE_STATE` (`src/driver/rda_ioctl.c:199-202`); IRP build failure → `STATUS_INSUFFICIENT_RESOURCES` (`src/driver/rda_ioctl.c:231-234`).

This is how `IOCTL_DISK_GET_DRIVE_GEOMETRY(_EX)` etc. reach the disk (used by the bench "geometry pass-through" probe, `src/bench/rdabench.c:265-269`).

### 4.4 Copy-session protocol (EUDCPEPM contract — 19 recognized codes)

Wire code seen by the driver = `custom_code + IoctlBase`, where `IoctlBase` is the service-key DWORD (root or `Parameters\`; default 0, WinPE sets 0x10000) — `src/shared/inc/rda_ioctl.h:146-152`, `src/driver/rda_ioctl.c:38-72`, `src/driver/rda_registry.c:120-127,134-136`. Codes are matched **explicitly** (non-contiguous set, G28 — `src/driver/rda_ioctl.c:42-46`).

| Code (custom) | Name | Gates (in/out) | Semantics (source) |
|---|---|---|---|
| `0x800C2AC0` | DISCOVERY | out ≥ 4 (filter); in ≥ 4 on filter | Control device: no-op `STATUS_SUCCESS` (`src/driver/rda_session.c:1410-1419`). Filter stacks: writes bound diskId, `Information = 4`, re-probes sector size (`src/driver/rda_filter.c:210-231`). |
| `0x800C2AC8` | GET_VERSION | in ≥ 0x84 | Writes `0x00010100` + ≤0x2E-byte ASCII product string `"RDA Disk Copy Driver (c) RDA contributors"` at +4; `Information = 0` — `src/shared/inc/rda_ioctl.h:344-349`, `src/driver/rda_session.c:1376-1393`; gate `:1378`. |
| `0x800C2AD0` | OPEN_SESSION | in ≥ 0x840 | Full 0x840 overlaid struct (below). `Information = 0`; outputs at +0x834 handle, +0x30 spb, +0x83C sectorSize — `src/driver/rda_session.c:731-838`. |
| `0x800C2AD8` | SESSION_OP | in ≥ 8 | Flush the protected file's volume (path = WCHAR string at head of the 0x800 bitmap buffer), `IRP_MJ_FLUSH_BUFFERS` ≤ 20 retries; state 0→1→3; **always returns 0** — `src/driver/rda_session.c:840-941`. |
| `0x800C2AE0` | SESSION_OP2 / StartBackup | in ≥ 0x28 | Requires state 3 else `STATUS_UNSUCCESSFUL`; locks pump buffer MDL; `Information = 0` — `src/driver/rda_session.c:944-1054`, state gate `:962-964`. |
| `0x800C2AE8` | GETNEXTBLOCK | in ≥ 0x1E, out ≥ 0x1E | **Async copy pump**: IRP queued, returns `STATUS_PENDING`; worker copies sectors into the StartBackup-registered pump buffer and completes the IRP with the 0x1E record; `Information = 0x1E` constant — `src/driver/rda_session.c:1472-1501`, worker completion `:610-616`. No active plan → literal `0xC0000120` with progress fields filled (`src/driver/rda_session.c:456-466`). Queue cap 2000 → `STATUS_NO_MEMORY` (`:1480-1482`). |
| `0x800C2AF0` | SESSION_STATUS | in ≥ 8 | Returns the block-map status **as the NTSTATUS** (no buffer write) — `src/driver/rda_session.c:1455-1457`. |
| `0x800C2AF8` | GET_PROGRESS (finalize) | in ≥ 0x0C | State → 5 (done), frees plan + pump MDL + COW, percent at buffer +8; `Information = 0` — `src/driver/rda_session.c:1057-1085`. |
| `0x800C2B00` | CLOSE_SESSION | in ≥ 8 | Destroys the session; invalid handle → `STATUS_INVALID_HANDLE` — `src/driver/rda_session.c:1462-1465`, `:1440-1444`. |
| `0x800C2B0A` | DIRECT_READ | out > 0x10 | `METHOD_OUT_DIRECT`; header+data in the MDL'd output buffer: `{u32 diskId@+0x00, u32 sectorCount@+0x04, u64 startLba@+0x08, data@+0x10}`; offsets are LBA × probed sector size; **`Information = OutputBufferLength` always** — `src/driver/rda_session.c:1087-1139`, wire `src/driver/rda_ioctl.h:165-166`. Session-free (diskId from header) — `src/driver/rda_session.c:1398-1408`. |
| `0x800C2B12` | DIRECT_WRITE | out > 0x10 | Twin of DIRECT_READ, write direction — `src/driver/rda_session.c:1398-1408`. |
| `0x800C2B18` | MODE4 | in ≥ 0x1C, out ≥ 4 | Raw paging-IO **write** by file handle — `src/driver/rda_session.c:1425-1431`, handler `:1143-1194`; `Information = 8` on success (`:1190-1192`). |
| `0x800C2B20` | MODE3 | in ≥ 0x1C, out ≥ 4 | Raw paging-IO **read** by file handle — same citations. |
| `0x800C2B2A` | SIZED_XFER | out > 0x10 | Same shape as direct; sector size from the per-disk cache — `src/driver/rda_ioctl.h:169-170`, `src/driver/rda_session.c:1398-1408`. |
| `0x800C2B2E` | SIZED_XFER2 (write) | out > 0x10 | Twin — `src/driver/rda_session.c:1404-1408` (IsWrite for DIRECT_WRITE/SIZED_XFER2). |
| `0x800C2B38` | CANCEL | in ≥ 8 | State := 3 (standby) + worker kick; `STATUS_SUCCESS`, `Information = 0` — `src/driver/rda_session.c:1467-1470`, `:1196-1200`. |
| `0x800C2B3E` | RRWR raw disk WRITE | filter-only | Control device: no-op `STATUS_SUCCESS` (`src/driver/rda_session.c:1410-1419`). Filter: magic `0x52455752` `"RRWR"` required (`src/shared/inc/rda_ioctl.h:224-225`); wire `{magic@0, sectorSize@+8 must match disk, declaredOutLen@+0xC must equal actual out, lba@+0x10 (u64), sectorCount@+0x18}`; data in the whole MDL buffer; `Information = outLength` on success — `src/driver/rda_filter.c:247-309`. |
| `0x800C2B40` | RRWR version probe | filter-only | `{0x604 written at +8, Information = 0xC}` — `src/driver/rda_filter.c:233-245`, constants `src/shared/inc/rda_ioctl.h:212-213`. |
| `0x800C2B42` | RRWR raw disk READ | filter-only | Twin of 0x800C2B3E — `src/driver/rda_filter.c:247-309`. |

**Handle-taking commands** (`SESSION_OP`, `SESSION_OP2`, `SESSION_STATUS`, `GET_PROGRESS`, `CLOSE_SESSION`, `GETNEXTBLOCK`, `CANCEL`): first 8 bytes of input = session handle; `inLength < 8` or NULL buffer → `STATUS_INVALID_HANDLE`; invalid handle value: `CLOSE_SESSION`/`GET_PROGRESS` → `STATUS_INVALID_HANDLE`, others → `STATUS_INVALID_PARAMETER` (`src/driver/rda_session.c:1433-1445`). **Unknown session codes → `STATUS_SUCCESS`** (reference fall-through, `src/driver/rda_session.c:1503-1506`).

**Wire structs** (compile-time offset-frozen with `C_ASSERT`s):

- `RDA_OPEN_SESSION` (0x840 bytes, `#pragma pack(4)`) — `src/shared/inc/rda_ioctl.h:309-342`:
  `+0x000 u32 DiskId` · `+0x004 u64 DataFirstSec` · `+0x00C u64 TotalSecs` (≠0) · `+0x014 u64 ExtentParam1` · `+0x01C u64 ExtentParam2` · `+0x024 u64 BitmapSize` (≠0) · `+0x02C u32 BitmapCount` (≠0) · `+0x030 u32 SpbInOut` (IN 0 → driver picks: TotalSecs < 0x20 → 1 else 0x10; OUT overwritten with effective spb) · `+0x034 u8 BlockBitmap[0x800]` (NUL pad @0x7FE) · `+0x834 u64 SessionHandle` (OUT, opaque ID) · `+0x83C u32 SectorSize` (OUT).
  Open gates: `TotalSecs==0 || BitmapSize==0 || BitmapCount==0` → `STATUS_INVALID_DEVICE_REQUEST` (`src/driver/rda_session.c:747-749`); overlapping sector range on the same disk → `STATUS_SHARING_VIOLATION` (0xC0000043) (`:751-754`, `src/shared/inc/rda_ioctl.h:351-356`); table full → `STATUS_INSUFFICIENT_RESOURCES` (`:753`).
- `RDA_GETNEXTBLOCK` (0x1E bytes, pack(1)) — `src/shared/inc/rda_ioctl.h:234-251`: in `{u64 Handle@+0x00, u64 CursorSec@+0x08, u32 MaxSec@+0x10}`; out `{u32 DiskId@+0x14, u32 Progress@+0x18, u8 MapProgress@+0x1C, u8 MapOk@+0x1D}`. **The copied data lands in the pump buffer registered at StartBackup, not in the IRP buffer.**
- `RDA_STARTBACKUP_IN` (0x28 bytes, pack(4)) — `src/shared/inc/rda_ioctl.h:261-278`: `{u64 Handle@+0x00, PVOID PumpBuffer@+0x08 (user VA, MDL-locked), u32 PumpBytes@+0x10 (must be spb×sectorSize-aligned), PVOID UserBitmap@+0x14 (optional exclusions), u32 StartSec@+0x1C, u32 BlockCount@+0x20, u32 Spb@+0x24 (must match session spb when a user bitmap is supplied)}`. Violations → `STATUS_INVALID_PARAMETER` (`src/driver/rda_session.c:966-972`); zero non-skipped sectors → `STATUS_INVALID_PARAMETER` (`:1040-1043`).
- `RDA_FILEIO_IN` (0x1C bytes, pack(4)) — `src/shared/inc/rda_ioctl.h:284-295`: `{HANDLE FileHandle@+0x00, PVOID Buffer@+0x08, u64 ByteOffset@+0x10, u32 Length@+0x18}`.

---

## 5. READ / WRITE contract (IRP_MJ_READ / IRP_MJ_WRITE on bound handles)

Reference-exact semantics — `src/shared/inc/rda_ioctl.h:128-143`, `src/driver/rda_readwrite.c:4-15`:

- `Parameters.Read/Write.Length` is in **BYTES**; `ByteOffset` is the absolute BYTE offset on the target (clients use `SetFilePointer`).
- **Negative offset → `STATUS_INVALID_PARAMETER`** (`src/driver/rda_readwrite.c:196-198`).
- **Zero length (or unmappable MDL) → `STATUS_INVALID_PARAMETER`** (`src/driver/rda_readwrite.c:342-347`; documented sane divergence S10 — the reference crashes here).
- MDL system-address mapping failure → `STATUS_INSUFFICIENT_RESOURCES` (`src/driver/rda_readwrite.c:294-300`).
- **On success, `Information = Length × 512`** (`RDA_LEGACY_INFORMATION_SCALE` = 512 — `src/shared/inc/rda_ioctl.h:143`, applied at `src/driver/rda_readwrite.c:267-273`). Clients must divide by 512 (unscale helper: `src/bench/rdabench.c:77-79`; observed contract check `src/bench/rdabench.c:291-305`). This is intentional wire-visible parity (fix F11 — `docs/error-fix-registry.md:19`).
- Partial reads: the driver forwards exactly `Length` bytes and reports the scaled count only on success; on failure `Information = 0` and the storage stack's NTSTATUS propagates (`src/driver/rda_readwrite.c:352-366`). There is no short-read reporting mechanism beyond the underlying stack's own behavior.
- Cancellation is supported (fix F4): a cancelled request completes `STATUS_CANCELLED` and cancellation is propagated into the storage stack via `IoCancelIrp` on the driver-owned forward IRP (`src/driver/rda_readwrite.c:212-266`).
- Alignment/granularity: no driver-side alignment requirement on the READ/WRITE path (byte lengths, absolute byte offsets); the per-handle sector size is probed at open (`IOCTL_DISK_GET_DRIVE_GEOMETRY`, cached per unique device, 64-entry bound — `src/driver/rda_target.c:302-445`) and used for 4Kn correctness at the forwarding boundary (`src/driver/inc/rda_p.h:91-94`). No maximum transfer size is imposed by the driver (the IO manager/MDL and the storage stack bound it); the benchmark uses up to 16 MiB buffers (`src/bench/rdabench.c:38`).
- Caching: no driver-level caching — plain forwarding over the caller's MDL; the bench uses `FILE_FLAG_NO_BUFFERING` for honest kernel-path measurement (`src/bench/rdabench.c:68`).

**Legacy "global-handle" modes on bare opens** (reference parity — `src/shared/inc/rda_ioctl.h:358-365`, `src/driver/rda_readwrite.c:317-340`):
- WRITE with `Length == 4` and no target: stores the caller's u32 handle value into the driver-global slot (the reference GetDriverInfo handshake). *Note: the header comment (`src/shared/inc/rda_ioctl.h:364`) says RDA "gates the store half to admin callers", and `docs/ground-truth-contracts.md:319` claims an `SeAdminCheck` — the shipped code has no explicit admin check on this path; the admin gate is effected by the default device SDDL (§9). Consumers should not rely on a per-request admin check.*
- READ with the slot nonzero and out length ≥ 0x208: resolves the parked handle's object name into the buffer (`src/driver/rda_readwrite.c:327-335`).

---

## 6. Wire-contract versioning

There is **no single `RDA_ABI_VERSION` macro**. Version is reported through five independent surfaces, all currently **1.0**:

| Surface | Value | Evidence |
|---|---|---|
| `IOCTL_RDA_GET_VERSION` (ULONG64) | `(1 << 32) \| 0` = `0x0000000100000000` | `src/driver/rda_ioctl.c:106-107`, `RDA_VERSION_MAJOR/MINOR` `src/shared/inc/rda_ioctl.h:106-107` |
| `IOCTL_RDA_GET_INFO` Major/Minor fields | 1 / 0 (+ Flags/MaxDisks/IoctlBase capability report) | `src/driver/rda_ioctl.c:84-95` |
| Session `GET_VERSION` (0x800C2AC8) | dword `0x00010100` + product string `"RDA Disk Copy Driver (c) RDA contributors"` (≤ 0x2E bytes) | `RDA_SESSION_PROTO_VERSION` `src/shared/inc/rda_ioctl.h:347-349`; served `src/driver/rda_session.c:1376-1393` |
| PE file-version resource | `FILEVERSION 1,0,0,0`; `FileVersion`/`ProductVersion` strings `"1.0.0.0"`; `FileDescription "RDA Raw Disk Access Driver"`; `InternalName`/`OriginalFilename "rda.sys"`; `CompanyName "heicfast / RDA contributors"` | `src/driver/rda.rc:19-21`, `:36-43`; `src/driver/rda_version.h:3-5` |
| INF `DriverVer` | `01/01/2026,1.0.0.0` | `src/driver/rda.inf:22` |

**Compatibility tiers** (what actually exists, per `src/shared/inc/rda_compat.h:1-28` and `src/shared/inc/rda_ioctl.h:1-15`): the "contract" is the union of the three reference drivers' wire protocols, preserved byte-for-byte — (a) EPMNtDrv open grammar + identity IOCTLs + sector-IO semantics + IOCTL pass-through; (b) EUEDKEPM 2-segment grammar variant; (c) EUDCPEPM IoctlBase-normalized session protocol (19 codes). On top: three RDA-extended IOCTLs (function codes 0x910-0x912). `RDA_DRIVER_INFO.StructSize` and `RDA_OPEN_TARGET_IN.StructSize` exist "for versioning" (`src/shared/inc/rda_ioctl.h:80,93`) — future-proofing hooks, currently exact-size.

**Version-mismatch behavior: none is enforced.** The driver never refuses a client based on version; a consumer that needs a contract guarantee must call `GET_INFO`/`GET_VERSION` itself and compare (the rdacli `info` subcommand is the model — `src/client/rdacli.c:73-99`). `GET_INFO.Flags` is the capability probe: bit 0 = filter role active, bit 1 = session engine active (`src/driver/rda_ioctl.c:87-93`). `IoctlBase` is echoed back so a client can compute session codes correctly (`src/driver/rda_ioctl.c:94-95`).

---

## 7. Session engine (rda_session.c)

**Lifecycle / identity**:
- Fixed table of **64 session slots** (`RDA_SESSION_TABLE_SIZE` 64, `src/driver/rda_session.c:66,145`); each slot has a generation counter.
- Session handle = opaque 64-bit ID: `(slotIndex + 1) << 16 | generation` (`src/driver/rda_session.c:151-167,223`). Handles are **never kernel pointers** (deviation F12 — `src/driver/rda_session.c:57-58`, `docs/error-fix-registry.md:20`).
- Created by OPEN_SESSION; owned by the creating process (`session->Owner = PsGetCurrentProcess()` with a formal reference — `src/driver/rda_session.c:772-777`).
- Destroyed by: CLOSE_SESSION (`src/driver/rda_session.c:1462-1465`), control-device CLEANUP sweep of the owner's sessions (`src/driver/rda_session.c:1222-1237`, called from `src/driver/rda_create.c:159-162`), filter REMOVE killing sessions bound to a removed disk (`src/driver/rda_session.c:1239-1254`, `src/driver/rda_filter.c:172-174`), and driver unload (`src/driver/rda_session.c:1210-1220`).

**State machine** (spinlock-guarded `State` field; `src/shared/inc/rda_ioctl.h:215-222`): `0` created → `1` flush-in-progress → `3` standby/cancel → `4` copying → `5` done. CANCEL forces 3 from any state; StartBackup requires 3 (`src/driver/rda_session.c:962-964,1047,1196-1200`).

**Concurrency limits** (all at `src/driver/rda_session.c:65-73`): 64 concurrent sessions; overlap-guarded exclusive sector ranges per disk (`STATUS_SHARING_VIOLATION`); worker kicks capped at 301 (`RDA_SESSION_KICK_CAP`, reference 0x12D); pump queue cap 2000 (`STATUS_NO_MEMORY` when exhausted, `src/driver/rda_session.c:1480-1482`); COW snapshot memory capped at 64 MiB per session (`STATUS_INSUFFICIENT_RESOURCES` past the cap — `src/driver/rda_session.c:302-304`, residual risk documented `docs/audit/2026-10-22-security-review.md:152-157`).

**What a session gates**: exclusive access to a sector range on one disk (source-disk snapshot semantics). While a session on a disk is **standby (3)**, external WRITEs through the filter stack are *parked*; while **copying (4)**, writes overlapping the plan are COW-saved first; TRIMs are swallowed while a session is active — `src/driver/rda_session.c:1282-1360`, `src/driver/rda_filter.c:192-207,314-348`. Each session owns one system worker thread that services the pump queue (`src/driver/rda_session.c:573-621`).

---

## 8. State reporting / health surface

**There is no dedicated health/heartbeat IOCTL and no WMI.** The complete user-mode-visible state surface is:

1. `IOCTL_RDA_GET_INFO.Flags`: `RDA_INFO_FLAG_FILTER_MODE` (filter role active) and `RDA_INFO_FLAG_SESSIONS` (session engine served) — `src/driver/rda_ioctl.c:87-93`. This is the closest thing to a "driver capabilities/health" probe; the rdacli `info` case is the reference pattern (`src/client/rdacli.c:73-99`).
2. Driver *loaded/started* is observable only indirectly: the `\DosDevices\<NAME>` symlink exists (openable) once DriverEntry succeeded — `src/driver/rda_control.c:162`; `scripts/install.ps1:78-84` prints the expected device after `sc.exe start`; `scripts/run-integration.ps1:22-25` treats a successful `rdacli info` as the loaded-and-healthy check.
3. Session progress/state: `SESSION_STATUS` (map status as NTSTATUS), `GETNEXTBLOCK` progress fields, `GET_PROGRESS` percent — §4.4.
4. No "paused"/"errored" driver-level states exist; the driver is either serving or failed-to-load (DriverEntry failure paths: control-device create failure returns the NTSTATUS to the SCM — `src/driver/rda_driver.c:136-140`; filter start failure tears down — `src/driver/rda_driver.c:162-170`).
5. Tracing: `DbgPrintEx` with levels 0-5, default 1 (errors), registry `Parameters\DebugLevel` — `src/driver/inc/rda_trace.h:28-59`; explicitly no WPP/ETW ("ETW can be layered later without contract impact", `src/driver/inc/rda_trace.h:12-15`).

For the core's driver-status surface, the supported probe sequence is: (a) `CreateFile("\\.\RDA")` succeeds → driver loaded; (b) `IOCTL_RDA_GET_INFO` succeeds → dispatch alive; (c) check `MajorVersion/MinorVersion` + `Flags` → capability tier. This is exactly what `rdacli info` does (`src/client/rdacli.c:73-99`).

---

## 9. Authorization / access control (how "unauthorized" is signalled)

**The device-object DACL is the only real boundary** (`src/driver/inc/rda_p.h:59-62`; `docs/ground-truth-contracts.md:310-319`):

- Default SDDL: `D:P(A;;GA;;;BA)(A;;GA;;;SY)` — **Administrators and SYSTEM full access only**, applied via `IoCreateDeviceSecure` (wdmsec.lib, linked at `src/driver/rda.vcxproj:91-92`) with a fixed device class GUID `{22503FD1-90D4-4322-B62D-9C8FFE186E33}` so admins can tighten security through the class key — `src/driver/rda_control.c:141-153`, `src/driver/inc/rda_p.h:64-68`.
- Override: registry `Parameters\Sddl` (REG_SZ) replaces the default — `src/driver/rda_registry.c:163-167`, applied `src/driver/rda_control.c:141-147`.
- Consequence for user mode: a non-elevated `CreateFile` on `\\.\RDA...` fails at the object-manager open with `STATUS_ACCESS_DENIED` → Win32 **`ERROR_ACCESS_DENIED` (5)**. Everything the driver exposes (raw disk IO, identity IOCTLs, sessions, pass-through) sits behind this gate (`docs/audit/2026-10-22-security-review.md:89-99`).

**Other authorization elements** (all secondary):
- PID-token handshake — retained for wire compatibility only, "NOT a security boundary" (`src/shared/inc/rda_ioctl.h:117-119`); mismatch → `STATUS_INVALID_PARAMETER` (Win32 87) on the sub-device open (`src/driver/rda_create.c:73-77`).
- **No image-name/PID allow-listing, no ACL checks per request, no signature checks on the calling process** anywhere in the driver (the whole of `rda_registry.c` is configuration, not authorization — `src/driver/rda_registry.c:8-16`).
- Identity IOCTLs are privileged information-disclosure primitives reachable only through the (admin-only) device (fix F3 — `src/driver/rda_ident.c:17-20`).
- Registry-based **configuration** (all optional; read once at DriverEntry; malformed values keep defaults — `src/driver/rda_registry.c:18-23`): `IoctlBase`, `MaxDisks` (2..1024 clamp), `DebugLevel` (0..5), `FilterMode`, `VolSnapAttach`, `SessionsEnabled`, `DeviceName`, `Sddl`. Service-key **root** values `IoctlBase`/`DeviceName` are read first (reference parity, G10), then `Parameters\` overrides — `src/driver/rda_registry.c:115-171`. Parameter-read failure is non-fatal except a NULL RegistryPath (`src/driver/rda_driver.c:127-134`).

**Unauthorized-signalling summary for the core** (exact conditions → NTSTATUS → Win32):

| Condition | NTSTATUS | Win32 error user mode sees |
|---|---|---|
| Non-admin/non-SYSTEM opens the device | `STATUS_ACCESS_DENIED` (object manager, from SDDL) | 5 `ERROR_ACCESS_DENIED` |
| Token mismatch / malformed grammar / disk# out of bounds | `STATUS_INVALID_PARAMETER` (`src/driver/rda_create.c:67-85`) | 87 `ERROR_INVALID_PARAMETER` |
| Driver not loaded (no symlink) | `STATUS_OBJECT_NAME_NOT_FOUND` | 2 `ERROR_FILE_NOT_FOUND` |
| Unknown IOCTL on a global handle | `STATUS_INVALID_PARAMETER` (`src/driver/rda_ioctl.c:282-285`) | 87 (DeviceIoControl fails) |
| Overlapping session range | `STATUS_SHARING_VIOLATION` (`src/driver/rda_session.c:751-754`) | 32 `ERROR_SHARING_VIOLATION` |

---

## 10. Error model (driver condition → NTSTATUS → user mode)

All from source, not guessed. Win32 codes are the standard `RtlNtStatusToDosError` mappings of the listed NTSTATUS values.

| Driver condition | NTSTATUS | Site |
|---|---|---|
| Malformed open path / token mismatch / disk bound exceeded / zero-length IO / bad IOCTL on global handle / bad identity-IOCTL lengths / OPEN_TARGET bad struct or kind | `STATUS_INVALID_PARAMETER` (0xC000000D) | `rda_create.c:67-85`, `rda_readwrite.c:342-347`, `rda_ioctl.c:124-154,282-285`, `rda_ident.c:55-58,155-158,167-170` |
| Output/input buffer below command minimum (GET_INFO, GET_VERSION, OpenSession, StartBackup, GETNEXTBLOCK, GET_PROGRESS, direct-family out ≤ 0x10) | `STATUS_BUFFER_TOO_SMALL` (0xC0000023) | `rda_ioctl.c:79-81,103-105`, `rda_session.c:744-746,954-956,1063-1065,1104-1106,1378-1380,1476-1478` |
| Any allocation failure (context, session bitmap, worker thread, forward IRP, pump node, name buffers) | `STATUS_INSUFFICIENT_RESOURCES` (0xC0000017) | `rda_create.c:87-90`, `rda_ioctl.c:156-159,231-234`, `rda_readwrite.c:200-203,226-231,295-300`, `rda_session.c:753,986-989,1008-1011,1483-1486` |
| Session table full (64) | `STATUS_INSUFFICIENT_RESOURCES` | `rda_session.c:753` |
| Pump queue exhausted (2000 queued GETNEXTBLOCK) | `STATUS_NO_MEMORY` (0xC000009A) | `rda_session.c:1480-1482` |
| Session handle invalid on CLOSE_SESSION / GET_PROGRESS; handle-command input < 8 bytes | `STATUS_INVALID_HANDLE` (0xC0000008) | `rda_session.c:1434-1444` |
| Session handle invalid on other handle-taking commands | `STATUS_INVALID_PARAMETER` | `rda_session.c:1438-1445` |
| OPEN_SESSION with TotalSecs/BitmapSize/BitmapCount == 0 | `STATUS_INVALID_DEVICE_REQUEST` (0xC0000018) | `rda_session.c:747-749` |
| OPEN_TARGET on an already-bound handle; pass-through with no top-of-stack | `STATUS_INVALID_DEVICE_STATE` (0xC0000184) | `rda_ioctl.c:132-134,199-202` |
| Unsupported major function on the control device | `STATUS_INVALID_DEVICE_REQUEST` | `rda_driver.c:50-60` |
| METHOD_NEITHER IOCTL pass-through | `STATUS_NOT_SUPPORTED` (0xC00000BB) | `rda_ioctl.c:220-224` |
| Overlapping session sector range on same disk | `STATUS_SHARING_VIOLATION` (0xC0000043) | `rda_session.c:751-754` |
| StartBackup when session state ≠ standby (3) | `STATUS_UNSUCCESSFUL` (0xC0000001) | `rda_session.c:962-964` |
| GETNEXTBLOCK with no active plan (pre-StartBackup or finalized) | literal `0xC0000120` (STATUS_REQUEST_ABORTED; source deliberately uses the raw "reference literal" — `rda_session.c:456-466`) with progress fields filled | `rda_session.c:456-466` |
| Cancelled READ/WRITE | `STATUS_CANCELLED` (0xC0000122) | `rda_readwrite.c:218-222,266` |
| Volume/disk target device absent | resolution status propagates from `IoGetDeviceObjectPointer` (typically `STATUS_OBJECT_NAME_NOT_FOUND`) | `rda_target.c:121-126,181-195,254-259` |
| GetProcName: PID not found / image-name query failure | lookup NTSTATUS propagates | `rda_ident.c:62-66,68-78` |
| GetFileObjName: handle is not a file object / bad handle | `ObReferenceObjectByHandle` status propagates (typically `STATUS_OBJECT_TYPE_MISMATCH` / `STATUS_INVALID_HANDLE`) | `rda_ident.c:97-102` |
| Name longer than caller's buffer | `STATUS_BUFFER_TOO_SMALL` | `rda_ident.c:34-36` |
| Pass-through / storage-stack failures | the lower stack's NTSTATUS and `Information` propagate verbatim | `rda_ioctl.c:236-244` |
| Filter WRITE blocked (COW cap exhausted) | `STATUS_INSUFFICIENT_RESOURCES` | `rda_filter.c:339-343` |
| RRWR/discovery on filter stacks with bad magic/lengths | `STATUS_INVALID_PARAMETER` | `rda_filter.c:211-215,234-239,256-271` |
| Parked writes at teardown | `STATUS_DELETE_PENDING` | `rda_session.c:663-665` |
| Unknown *session-family* code (post-normalization) | `STATUS_SUCCESS` (reference fall-through) | `rda_session.c:1503-1506` |

Success conventions: READ/WRITE `Information = Length×512` (§5); GET_INFO 24; GET_VERSION 8; identity IOCTLs `nameBytes+2`; GETNEXTBLOCK `0x1E`; direct-family `OutputBufferLength` always; MODE3/4 `8`; OPEN_SESSION / GET_VERSION(session) / GET_PROGRESS / CLOSE / CANCEL `0`.

---

## 11. Build outputs (what the driver build produces)

- Solution `rda.sln` builds three projects — driver `rda`, client `rdacli`, bench `rdabench` (`rda.sln:5-10`), Debug|x64 and Release|x64 only (`rda.sln:12-15`).
- Output directory: `$(SolutionDir)bin\$(Platform)\$(Configuration)\` → **`bin\x64\Release\rda.sys`**, **`bin\x64\Release\rdacli.exe`**, **`bin\x64\Release\rdabench.exe`** (`src/driver/rda.vcxproj:63-67`, `src/client/rdacli.vcxproj:42-44`, `docs/build.md:22-26`).
- PDB: `/Zi` ProgramDatabase is configured (`src/driver/rda.vcxproj:84`) — symbol files are produced into the intermediate `obj\` tree but are **not** part of any published artifact (the release staging list has no `.pdb` — `.github/workflows/release.yml:43-57`).
- The driver links `wdmsec.lib` (`src/driver/rda.vcxproj:91-92`) and pins `/osversion:6.1 /subsystem:native` (`src/driver/rda.vcxproj:86-89`); post-build `tools/stamp_osversion.py` enforces PE OS fields 6.1/NATIVE + recomputed checksum (`tools/stamp_osversion.py:7-17,73-80`).
- `SignMode=Off` — artifacts are **unsigned by design** (`src/driver/rda.vcxproj:48-58`, `docs/signing.md:31-37`); CI asserts the binary carries no signature table (`release.yml:34-41`, `.github/workflows/ci.yml:151-162`).
- `rda.inf` ships as **static content, verbatim** (no stampinf/Inf2Cat in the build) — `src/driver/rda.vcxproj:121-126`. No `.cat` is produced (the INF references `CatalogFile = rda.cat`, `src/driver/rda.inf:21`, which does not exist in-tree — signing-stage concern only).
- CI build-matrix artifacts: `rda-wdk26100-Debug` / `rda-wdk26100-Release` containing `rda.sys`, `rdacli.exe`, `rda.inf` (`.github/workflows/ci.yml:164-172`).
- Toolchain: VS2022 v143 + WDK/SDK NuGet `10.0.26100.6584` (`packages.config:6-8`, `Directory.Build.props:8-10`), `NTDDI_VERSION=0x06010000` Win7-SP1 header gating (`src/driver/rda.vcxproj:73`), `/W4 /WX` zero-warning policy (`src/driver/rda.vcxproj:70-72`).

---

## 12. Contract freeze enforcement (what a consumer can rely on)

- The wire tables are compile-time frozen with `C_ASSERT`s in `src/shared/inc/rda_ioctl.h:245-251,272-278,292-295,332-342` and runtime-frozen by `tests/unit/test_compat.c:221-253` (IOCTL codes, all 19 session codes, struct sizes/offsets), which runs on every push on both Linux gcc and Windows MSVC (`.github/workflows/ci.yml:43-46,90-101`).
- The open-grammar logic is shared kernel/user-mode source (`src/shared/inc/rda_compat.h:1-10`), fuzzed with libFuzzer + ASan/UBSan in CI (`fuzz/Makefile:14-39`, `.github/workflows/ci.yml:49-72`).
- Cross-driver byte-equality with the original reference binaries is asserted in the benchmark workflow (SHA-256 over sampled 1 MiB ranges; any mismatch fails the job) — `.github/workflows/benchmark.yml:203-235`, results in `docs/benchmarks/2026-10-22-runner-comparison.md:14-25` (64/64 ranges byte-identical).
- Known deliberate wire-visible divergences: admin/SYSTEM-only DACL (opens that used to succeed for non-admins now fail — `docs/error-fix-registry.md:10`), session GET_VERSION product string (trademark hygiene, length-compatible — `docs/error-fix-registry.md:27-29`), zero-length READ/WRITE rejected instead of crashing (`docs/audit/2026-10-22-security-review.md:76-79`).
