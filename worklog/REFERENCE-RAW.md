# REFERENCE-RAW.md — Raw I/O parameters of the og (EaseUS Data Recovery Wizard)

Session: performance-diagnostic, Agent C (Task 2-C). Date: 2025-11-18.
Corpus: /home/z/my-project/easeus-drw-research @ be0cfe9, decompiled text under
`re-workspace/reversed/<group>/<Module>_decompiled.c` (Ghidra output; ~90 modules).

Citation convention: `file:LINE` = decompiled text line; `FUN_xxxxxxxx` = Ghidra
function label whose entry address is embedded in the name (VA). Every claim below
was re-verified in the decompiled text this session; anything not verified is
explicitly marked **UNVERIFIED**. Paths below are relative to
`re-workspace/reversed/`.

Key file map (verified layout):
- `ghidra-mo/Device_decompiled.c` (68,988 lines) — CWindowsStorageDevice / CVolumeDevice / CStorageDisk device layer
- `ghidra-searchdll/NTFSSearchDLL_decompiled.c` (62,394), `RAWSearchDLL_decompiled.c` (80,615) + 9 sibling FS search DLLs
- `ghidra-drwui/DRWUI_decompiled.c` (2,381,187) — UI + CStorageDeviceWithCache + SearchDataSource
- `ghidra-drivers/eprdtdrv_decompiled.c` (2,193) — EPRDTDRV kernel driver
- `ghidra-partition/{Boot,RecoveryModule,MainModule,ToolsModule,CloneModule}_decompiled.c`

---

## 1. CreateFile flags in the device layer (HUNT 1)

**No `FILE_FLAG_OVERLAPPED` (0x40000000) and no `FILE_FLAG_NO_BUFFERING`
(0x20000000) anywhere in the scan pipeline.** Verified by corpus-wide search over
all `*_decompiled.c` under ghidra-mo, ghidra-searchdll, ghidra-partition,
ghidra-drwui: zero CreateFile call sites carry either flag. Every device-layer
open uses `FILE_ATTRIBUTE_NORMAL` (0x80) or 0 as dwFlagsAndAttributes, and
`OPEN_EXISTING` (3) as dwCreationDisposition.

Exact call shapes (access, share, security, disposition, flags, template):

| Site | Call | Meaning |
|---|---|---|
| ghidra-mo/Device_decompiled.c:55949-55950 | `FUN_180063080((param_1+0x14), 0xc0000000, 3, 0, 3, 0x80, 0)` | reopen inside `CWindowsStorageDevice::Read` — GENERIC_READ\|WRITE, FILE_SHARE_READ\|WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| ghidra-mo/Device_decompiled.c:57101 | `CreateFileA(&local_88, 0xc0000000, 7, 0, 3, 0x80, 0)` | volume-path open (\\?\ style, `?`→`\\` fixup at :57095) — share adds FILE_SHARE_DELETE |
| ghidra-mo/Device_decompiled.c:55370-55371 | `CreateFileA((param_1+0xa0), *(DWORD*)(param_1+0x3b0), *(DWORD*)(param_1+0x3b4), 0, 3, 0x80, 0)` | `CWindowsDynamicVolumeDevice::Open` (FUN_18005f3c0 :55363) — access/share from object fields, always OPEN_EXISTING + 0x80 |
| ghidra-mo/Device_decompiled.c:56456-56458 | `FUN_180063080((param_1+0xa0), *(param_1+0x3b0), *(param_1+0x3b4), 0, 3, 0x80, 0)` | `CWindowsStorageDevice::Open` (FUN_180061740 :56448) — same shape via the open wrapper (see §6) |
| ghidra-mo/Device_decompiled.c:50975 | `CreateFileA("\\\\.\\X:", 0xc0000000, 3, 0, 3, 0x80, 0)` | WinPE/driver-availability probe (FUN_1800577e0 :50943) |
| ghidra-mo/DeviceManager_decompiled.c:13229/13349/27417 | `CreateFileA(..., 0xc0000000, 3, 0, 3, 0x80, 0)` | device-manager opens |
| ghidra-mo/DeviceManager_decompiled.c:23545-24056, :27301 | `CreateFile("\\\\.\\MountPointManager", 0x10000000, 3, 0, 3, 0x80, 0)` | mount-point queries, GENERIC_ALL |
| ghidra-searchdll/NTFSSearchDLL_decompiled.c:35522 | `CreateFileW(lpFileName, 0x10000000, 3, 0, 2, 0x80, 0)` | output-file path (CREATE_ALWAYS), not a device read |

Raw open wrapper: `FUN_180063080` (Device_decompiled.c:57351-57400) — allocates a
16-byte handle box tagged `0x6b646d56` ("Vmdk", :57372); if the path is not
driver-mode → plain `CreateFileA` (:57380), else driver-mode open (§6).

## 2. ReadFile / ReadFileEx shapes — overlapped or not (HUNT 2)

**VERDICT: fully SYNCHRONOUS I/O in the entire scan/recovery pipeline.**

- Read thunk `FUN_180062ce0` (Device_decompiled.c:57224-57247):
  `local_28 = ReadFile(local_20, param_2, param_3, param_4, param_5);` (:57239)
  — plain kernel32 ReadFile; branches to the driver-mode path
  `FUN_18004bd80` (:57243) when the handle is driver-mode.
- The canonical read loop `CWindowsStorageDevice::Read` = `FUN_1800601b0`
  (Device_decompiled.c:55867-55997, uint Read(this, buf, sectorOffset, sectorCount)):
  - offsets/lengths multiplied by sector size first (:55936-55939),
  - `SetFilePointer` thunk `FUN_180062c30` (:55942/:55957, impl :57196-57219),
  - `FUN_180062ce0(handle, buf, (uint)len, bytesRead, (LPOVERLAPPED)0x0)`
    (:55964-55965) — **OVERLAPPED argument is literally NULL**,
  - partial-read rounding `local_88[0] = local_88[0] / 0x200` (:55966-55968),
  - loop `while (ReadFile ok && bytes > 0)` (:55984).
- Write twin `FUN_1800606b0` (Device_decompiled.c:56002-56204) uses the WriteFile
  thunk `FUN_180062da0` (:57252-57275) with `(LPOVERLAPPED)0x0` (:56156, :56178).
- DeviceIoControl thunk `FUN_180062e60` (:57280-57304) likewise takes
  `(LPOVERLAPPED)0x0` at every device-layer call site (e.g. :56523, :56526,
  :56532, :56301, :56304).

**Async-machinery audit (corpus-wide, all ~90 decompiled modules):**

| API | Hits |
|---|---|
| CreateIoCompletionPort | **0** |
| GetQueuedCompletionStatus | **0** |
| QueueUserWorkItem | **0** |
| CreateThreadpoolIo | **0** |
| RegisterWaitForSingleObject | 2, both in `ghidra-repair/EUImageEnhance_decompiled.c:306520/:318265` (image-repair module, NOT the scan pipeline) |
| ReadFileEx | **0** |
| LPOVERLAPPED sites total | 1076 across the corpus; 1055 are explicitly `(LPOVERLAPPED)0x0` |
| Genuine non-null OVERLAPPED | only `ghidra-misc/archive_decompiled.c:81562/:83133` — libarchive OVERLAPPED ring (0x50 stride, `GetOverlappedResult(...,bWait=1)`), used for archive extraction; and `ghidra-partition/CloneModule_decompiled.c:96910/:101569`, where the overlapped pointer slot is explicitly zeroed just before the call (`*(puVar14+0x20) = 0`) → effectively NULL |

So: the og's disk read path is **synchronous ReadFile after SetFilePointer, no
IOCP, no thread pool, no ReadFileEx**; overlapped I/O exists only in the
archive-extraction support DLL.

## 3. Chunk sizes and call sites (HUNT 3)

| Chunk | Size | Verified site |
|---|---|---|
| Device read unit | 0x400000 / sectorSize sectors (4 MiB) | ghidra-mo/Device_decompiled.c:56545 — `*(int*)(param_1+0x10) = (int)(0x400000 / (longlong)*(int*)(param_1+0x38));` in FUN_180061890 (geometry init). Prior cite ":56545" CONFIRMED exact. Guard: sectorSize==0 → chunk=0 (:56541-56542) |
| Geometry sources for that init | IOCTL 0x70000 (DISK_GET_DRIVE_GEOMETRY, out 0x18), 0x74004 (PARTITION_INFO_EX, out 0x20), 0x7405C (GET_LENGTH_INFO, out 8) | :56523, :56526, :56532; sector default 0x200 at :56535-56536 |
| Volume read unit | 0x700000 / sectorSize (7 MiB) | ghidra-mo/Device_decompiled.c:56912 — `*(int*)(param_1+0x18) = (int)(0x700000 / (ulonglong)param_2);` in CVolumeDevice ctor FUN_180062480 (:56903-56914); copy ctor FUN_180062500 repeats it at :56928. Prior cite ":56912" CONFIRMED exact |
| MFT batch | 0x1000 sectors per read (2 MiB @512B), window buffer = sectorSize << 0xC | ghidra-searchdll/NTFSSearchDLL_decompiled.c:5649-5678 — `uVar12 = uVar5 << 0xc;` (:5652), read call `(**(code**)(*param_2+8))(param_2, lVar2, uVar11, 0x1000, 0);` (:5658, repeated :5678), MFT records stepped by 0x400 (:5663-5667), progress advances `(uVar10 / recordSize) / sectorSize` (:5671-5673). Prior cite ":5649-5678" CONFIRMED exact |
| $UsnJrnl window | 0x100000 (1 MiB) | ghidra-searchdll/NTFSSearchDLL_decompiled.c:19376 — `FUN_18002be40(0x100000)` in ctor FUN_180018570 (:19349-19380); window reset `memset(*(param_1+0x58), 0, 0x100000)` (:20036) and carry clamp `< 0x100000` (:20013, :20041); stream size field `plVar11[8] = 0x100000` (:21435, FUN_18001b190). $Extend/$UsnJrnl walk in FUN_180018d80 (:19668+, name compares :19730/:19761) |
| Boot region | 64 KiB claim | **UNVERIFIED / NOT FOUND**: no device-side 64 KiB read located. The only 0x10000 reads in ghidra-partition/Boot_decompiled.c are lazy log-export thunks `FUN_1800075a0` (:4364-4388, call at :4385) and `FUN_180017940` (:13202-13226, call at :13223) — log buffers, not disk I/O. Partition-table walk instead reads 1 sector at a time: ghidra-mo/Partition_decompiled.c:985, :1022 (6 sectors), :1034, :1043 (2 sectors) via vtable+8 with count=1 |
| Carve window (RAWSearchDLL) | chunk = 0x400000 / sectorSize (4 MiB); buffer = 2× chunk bytes + 0x1000 | ghidra-searchdll/RAWSearchDLL_decompiled.c:9245-9309 (FUN_18000c430): `*(int*)(param_1+200) = 0x400000 / param_2` (:9272 and :9285), `*(int*)(param_1+0xd8) = iVar3 * *(int*)(param_1+0xf0) * 2` (:9274-9275, :9287) — double buffer, alloc `FUN_1800020c0(iVar3 + 0x1000)` (:9293), zero-fill (:9296), plus 0x10048-byte side table (:9298). NEW finding — supersedes REFERENCE-SCAN.md §12 "carve window not decompiled" |
| RAWSearchDLL device I/O | none | RAWSearchDLL_imports.txt has no CreateFile/ReadFile/DeviceIoControl — carving rides caller buffers (consistent with DIAGNOSIS.md:71) |

### CStorageDeviceWithCache (DRWUI) — prefetch/cache parameters

Factory `FUN_1405846b0` (ghidra-drwui/DRWUI_decompiled.c:919039-919249):
- :919080-919092 — sector size from vtable+0x28 (fallback 0x200);
  `uVar19 = 0x400000 / uVar17; if (0 == uVar19) uVar19 = 0x2000;`
  `*(int*)(param_1+0x41) = uVar19;` — cache chunk = 4 MiB worth of sectors
  (fallback 0x2000 = 8192 sectors). Prior cites ":919080-:919092" CONFIRMED.
- :919093 — `FUN_14057fde0(uVar17, 0x400000 % uVar17)` (video-repair buffer, 4 MiB).
- :919109 — object typed `CStorageDeviceWithCache::vftable`;
  :919116 `InitializeCriticalSection((LPCRITICAL_SECTION)(puVar14 + 0x11));`
- :919129 — `FUN_1405c77d0(puVar14, 1000);` — ctor-default run size 1000 sectors.
- :919131 — `*(undefined4 *)(puVar14 + 0xe) = 10;` — lookahead = 10 runs.
- :919142 — `FUN_1405c77d0(param_1[0x25], uVar19);` — run size OVERRIDDEN by the
  factory to the 4 MiB/sector chunk; then :919144 `FUN_1405c78b0(...)` primes the
  ring and spawns the prefetch thread.

Ctor `FUN_1405c72c0` (DRWUI_decompiled.c:962250-962291):
:962268 `InitializeCriticalSection(param_1+0x11)`; :962281
`FUN_1405c77d0(param_1, 1000)`; :962283 `*(undefined4*)(param_1+0xe) = 10;`.
Prior cites ":962250-:962597" CONFIRMED (ctor + FUN_1405c77d0 :962487-962505 +
FUN_1405c78b0 :962530-962597).

`FUN_1405c77d0` (run-size setter, :962487-962505): stores run in sectors at
`param_1[7]`, allocates the consumer copy buffer of `sectorSize × run` bytes
(:962499-962502). Prime loop `FUN_1405c78b0` (:962530-962597): 10 slot buffers,
each `GetBytesPerSector() × run` (:562→:562569); creates two manual-reset events
(:962589-962592) and the prefetch thread (:962593).

So the verified cache budget: **ring of 10 runs × run size; run = 1000 sectors
(ctor default) → 4 MiB/sectorSize (factory override); ring total 10 × 4 MiB with
the override.** The "count 1000" in prior notes is the ctor-default run size in
sectors, not a slot count.

## 4. Threading in the scan pipeline (HUNT 4)

- DRWUI has exactly **45 CreateThread call sites** (rg count this session;
  matches the corpus worklog.md:461 "45 named threads"). All
  `CreateThread(0, 0, entry, param, flags, 0)` — no stack-size tuning, no
  _beginthreadex anywhere in DRWUI.
- **Prefetch thread** (one per CStorageDeviceWithCache):
  `FUN_1405c78b0` → `CreateThread((LPSECURITY_ATTRIBUTES)0x0, 0,
  FUN_1405c7ac0, param_1, 0, local_a8)` (DRWUI_decompiled.c:962593). Thread body
  FUN_1405c7ac0 (:962602-962682): waits `WaitForSingleObject(kick, 1)` — 1 ms poll
  (:962620); reads the next run of `param_1[7]` sectors through the underlying
  device `(*vtbl+8)(dev, slot, offset, runSectors, 1)` (:962660-962664);
  ring cursor wraps at the 10 lookahead (:962624, :962672);
  `EnterCriticalSection`/`LeaveCriticalSection` around counter+events
  (:962670, :962676); on device error 0x37 → disables prefetch
  (:962665-962667, LogInfo "CacheThread find read sector" :962634).
- **Consumer side** (cache-hit path `FUN_1405c7dd0`, :962729-962794):
  `WaitForSingleObject(param_1[0x17], 0xffffffff)` (:962764) — blocks until the
  prefetch thread fills slots; `EnterCriticalSection` (:962772); `memcpy` from
  ring slot into the copy buffer (:962776); cursor mod 10 (:962778-962780);
  `SetEvent(kick)` (:962785); `LeaveCriticalSection` (:962787). Classic
  single-producer/single-consumer ring gated by a CRITICAL_SECTION + two events.
- **SearchDataSource engine threads** (DRWUI_decompiled.c):
  `CreateThreadPool2Search` FUN_140230ed0 (:388693-388730) spawns one thread
  (entry `DAT_140231020`, :388714). `SearchAvoidLockUiThread` FUN_1402310a0
  (:388735-388854) then spawns one thread per engine "CHN" channel:
  `CreateThread(0, 0, FUN_14022f9c0, chan, 4, 0)` (:388833 — flag 4 =
  CREATE_SUSPENDED), **`SetThreadAffinityMask(h, (longlong)(1 << (local_d8 & 0x1f)))`
  (:388835) — each engine thread is hard-pinned to core `i`**, then
  `ResumeThread` (:388836). Channel count = `FUN_1402321c0` (:389350-389359,
  vector length); distribution round-robin `local_d8 % channelCount` (:388808).
- **SearchDLL engines are internally single-threaded**: rg for CreateThread /
  _beginthreadex in all 11 ghidra-searchdll decompiles → ZERO hits (NTFSSearchDLL
  has exactly one CreateEventW, :49907). Parallelism comes entirely from the
  DRWUI-side channel threads above.
- PartitionRecovery side: `ghidra-partition/RecoveryModule_decompiled.c` and
  `MainModule_decompiled.c` each have 3 CreateThread sites.
- No thread-pool APIs corpus-wide (see §2 audit table).

## 5. Driver-era path: eprdtdrv.sys (HUNT 5)

**Namespace grammar** (ghidra-mo/Device_decompiled.c):
- `\\.\EPRDTDRV\%d\%d` (:7793, CDiskDriveLayoutReadWriter ctor FUN_180063??;
  formatted at :7819) and `\\.\EPRDTDRV\%d\Volume\%d` (:329, volume variant,
  formatted at :355) — includes `GetCurrentProcessId()` mixed into the args
  (:7806/:342). This is the og's "grammar path" (NOT `\\.\rda`).
- Fallbacks when driver absent: `\\.\PhysicalDrive%d` (:7822, :50659, :52443) and
  `\\.\HarddiskVolume%d` (:358). Switch: `iVar1 = FUN_1800577e0(); if (iVar1 ==
  0) → EPRDTDRV path; else → PhysicalDrive` (:7816-7823, :352-358, :50653,
  :52437). FUN_1800577e0 (:50943-50992): `GetSystemDirectoryA`; if the system
  root is drive X:/x: (WinPE), opens `\\.\X:` (:50975) and issues
  `DeviceIoControl(h, 0x560000, ...)` (IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
  :50980) — i.e. EPRDTDRV paths are chosen in the WinPE boot environment.
- **.vmdk routing**: `FUN_1800633a0` (:57456-57494) lowercases the path and tests
  the ".vmdk" suffix (:57486-57488); driver-mode handles come from
  `FUN_18004b900` (:44918-44967) → `FUN_18004bb60` (:44982-45013) which calls the
  RPC layer `FUN_18004e630(0x1800aa100, name)` and `FUN_18004f070` (:45002-45007).
  Driver-mode seek: `FUN_18004bce0` (:45018-45041) sets a virtual 64-bit offset at
  client+0x48. Driver-mode read: `FUN_18004bd80` (:45046-45077):
  `FUN_18004ea80(0x1800aa100, name, uVar3/uVar1 /*offset/bytesPerSector*/,
  param_3/uVar2 /*length/bytesPerSector*/, buffer)` (:45066) — the cross-boundary
  contract is **sector-addressed (offset/512, length/512)**; the virtual pointer
  advances by bytes (:45072).

**eprdtdrv.sys IRP table** (ghidra-drivers/eprdtdrv_decompiled.c DriverEntry
setup :868-875; MajorFunction base 0x70 on x64 DRIVER_OBJECT):
- IRP_MJ_CREATE = FUN_000110b0 (slot 0x70), IRP_MJ_CLOSE = FUN_000117b8
  (slot 0x80, frees client context), **IRP_MJ_READ = FUN_00011fd8 (slot 0x88)**,
  IRP_MJ_WRITE = FUN_00011de4 (slot 0x90), IRP_MJ_DEVICE_CONTROL = FUN_00011bcc
  (slot 0xe0), IRP_MJ_CLEANUP = FUN_00011dbc (slot 0x100).

**"F11" read handler FUN_00011fd8** (eprdtdrv_decompiled.c:671-744) — the exact
IOCTL/IRP contract:
- maps the caller's buffer directly from the IRP MDL:
  `MmMapLockedPagesSpecifyCache(...)` (:695, and :711 via the MDL at IRP+8),
  or uses the pre-mapped system address (IRP->MdlAddress->MappedSystemAddress at
  MDL+0x18, :698/:714) — **direct MDL I/O confirmed**;
- builds the downstream request with
  `IoBuildAsynchronousFsdRequest(3 /*IRP_MJ_READ*/, dev, mappedBuf,
  *(DWORD*)(client+8) /*length*/, &offset /*client+0x18*/, event)` (:719-720);
- completion routine `FUN_00011844` registered (:724) — its DbgPrint is
  `"[FltReadWriteSectorsCompletion] ..."` (:346-348, at :356-361 it does
  MmUnlockPages + IoFreeMdl per MDL, then KeSetEvent);
- waits synchronously: `KeWaitForSingleObject` after `IofCallDriver` returns
  0x103 STATUS_PENDING (:727-730);
- **completion scaling: `*(ulonglong *)(param_2 + 0x38) = (ulonglong)(uint)
  (*(int *)(lVar4 + 8) << 9);` (:733)** — IRP->IoStatus.Information = Length × 512
  (<<9). The hunt-list claim "F11 Information=Length*512" is CONFIRMED at
  eprdtdrv_decompiled.c:733. The WRITE handler mirrors it: same `<< 9` at :653
  inside FUN_00011de4 (DbgPrint "[epmntdrvDispatchWrite]" :658).
  Dimension note (from the user-mode side §5): the client length field is
  sector-scaled — the user-mode wrapper divides by bytes-per-sector before the
  RPC, and completion Information multiplies back by 512.
- Private IOCTLs decoded in FUN_00011bcc (:482-531): 0x60002400 (GetProcName →
  FUN_00011908, :496), 0x60002404 (GetFileObjName → FUN_00011a90, :501-503),
  **0x60002408 = "[GetDiskContextMode]"** (:513-531, DbgPrint :527). The og's
  device layer calls 0x60002408 back from user mode at
  ghidra-mo/Device_decompiled.c:52598.
- Which paths use the driver: EPRDTDRV namespace opens (WinPE environment per
  FUN_1800577e0) and .vmdk image handles; PhysicalDrive/HarddiskVolume opens go
  direct user-mode CreateFileA + synchronous ReadFile (§1-§2). The thunks switch
  on the handle-box flag: `FUN_1800634c0` (:57499-57513) validates the "Vmdk" tag
  and returns mode+handle; FUN_180062c30/FUN_180062ce0/FUN_180062da0/FUN_180062e60
  branch: mode 0 → kernel32 API, mode 1 → driver RPC (:57208-57216, :57236-57244,
  :57264-57272, :57293-57301).

## 6. Per-transport behavior (HUNT 6)

- **IOCTL_STORAGE_QUERY_PROPERTY (0x2d1400)** usage:
  - ghidra-mo/Device_decompiled.c:52586 — vtable+0x28 DeviceIoControl with a
    12-byte STORAGE_PROPERTY_QUERY in-buffer and a 0x227-byte
    STORAGE_DEVICE_DESCRIPTOR out-buffer (:52570-52577).
  - Descriptor BusType at +0x1C is read (:52591) and compared `== 0xf`
    (:52599-52604) → sets flag at this+0x14. (0xF in STORAGE_BUS_TYPE =
    FileBackedVirtual — i.e. a file-backed/virtual disk; semantic label of the
    og's flag: **UNVERIFIED**, the branch is real).
  - Same IOCTL appears in ghidra-wave2/DiskHealth_decompiled.c:8917, :10804,
    :13362, :13366, :16257, :16317, :17770, :17831, :20241, :20302, :22336,
    :22397 (health module queries descriptors/serials);
    ghidra-partition/Boot_decompiled.c:8909; ghidra-partition/ToolsModule_decompiled.c:74957,
    :131208, :131229; ghidra-partition/MainModule_decompiled.c:114630, :114652.
- **WMI bus type collection** (not a sizing switch): `CStorageDisk` reads WMI
  property `L"BusType"` (Device_decompiled.c:17226, stored at this+100; same at
  :17644); `CStoragePhysicalDisk` reads `L"BusType"` (:17825, stored at this+0xC),
  plus `L"Usage"`, `L"IsPartial"`, `L"DeviceId"`, `L"CanPool"` (:17831-17852).
  (The `== 0xb` comparisons at :17838/:17852/:18200/:18214/:23084/:23136 are
  VARIANT-type checks (VT_BOOL) on the WMI results, NOT SATA checks.)
- GetDriveType: imported only by repair/misc modules (EUUnZip, unins000,
  EuDownloader, archive.dll, EUOfficeViewer — verified via *_imports.txt); NOT in
  the device/scan layer.
- **No chunk size, queue depth, or thread count keyed on bus type anywhere**:
  every size is a sector-normalized constant (0x400000/sector, 0x700000/sector,
  0x1000 sectors, 0x100000 bytes, 1000 sectors) independent of transport. The
  only device-class branches found are geometry (sectorSize) and the
  FileBackedVirtual-style flag above. Verified by corpus-wide constant audit.
- Volume-lock prep (all transports): FUN_180061250 (Device_decompiled.c:56290-
  56314) issues 0x90083 (FSCTL_ALLOW_EXTENDED_DASD_IO, :56301), then
  FlushFileBuffers (:56302), then retries FSCTL 0x90018 (FSCTL_LOCK_VOLUME) up
  to 3 times with `Sleep(100)` between attempts (:56303-56309).

## 7. Alignment (HUNT 7)

- All device reads are **sector-aligned by construction**: the Read API takes
  sector offsets/counts and multiplies by bytes-per-sector before SetFilePointer
  (Device_decompiled.c:55936-55939); read lengths are capped to the remaining
  chunk (`if (local_78 < local_70) local_70 = local_78;` :55961-55963); partial
  reads are truncated to whole sectors (`/ 0x200`, :55966-55968 and :56183).
- No `_aligned_malloc` in any read path: the only occurrence is
  DRWUI_decompiled.c:644338 `_aligned_malloc(8, 0x10)` (8-byte payload, non-I/O).
- Buffers are plain heap: cache ring slots via FUN_140053400
  (DRWUI:962562/:962569), SearchDLL windows via FUN_18002be40
  (NTFSSearchDLL:19376). DRWUI VirtualAlloc sites (0x100000 pages at
  :653564/:800009/:804027/:812232, e.g. FUN_1404d06c0 :799990-800030) are
  generic 1-MiB scratch allocators, not device-read buffers.
- No SECTOR-aligned buffer-address requirement is imposed anywhere (no
  FILE_FLAG_NO_BUFFERING ⇒ no alignment contract — consistent with §1).

## 8. Error-path / retry (HUNT 8)

- **ERROR_INVALID_HANDLE reopen-retry** — CONFIRMED at
  ghidra-mo/Device_decompiled.c:55944-55960 inside FUN_1800601b0: if
  SetFilePointer fails (0xFFFFFFFF), `FUN_180062c10()` (GetLastError, :55945);
  if error == 6 → `FUN_180062f90` (CloseHandle thunk, :55948), reopen
  `FUN_180063080(path, 0xc0000000, 3, 0, 3, 0x80, 0)` (:55949-55951), retry
  SetFilePointer once (:55957-55959), else bail. Prior cite ":55947-55959"
  CONFIRMED (block actually starts :55944).
- Read loop termination: `while ((ReadFile != 0) && (bytesRead != 0))` (:55984)
  — plain partial-read loop, no backoff, no retry counter.
- Other retries in the device layer: the 3×100 ms FSCTL lock retry (§6,
  Device:56303-56309). No other Sleep-based retry loops exist in
  Device_decompiled.c (single `Sleep(100)` in the file, at :56306).
- Prefetch-thread degradation: underlying device error 0x37 (55) → prefetch
  disabled (DRWUI_decompiled.c:962665-962667).
- Scan factory special case: volume handle == 0 → `Sleep(3000)`
  (DRWUI:919147-919148).

## 9. Corrections vs prior citations (audit deltas)

1. Prior "Device_decompiled.c:55947-56183 for CWindowsStorageDevice::Read" —
   corrected: Read = FUN_1800601b0, :55867-:55997 (the ERROR_INVALID_HANDLE block
   is :55944-:55960). Lines :56145-:56183 belong to the WRITE twin FUN_1800606b0
   (:56002-:56204).
2. ":56545 (4 MiB)", ":56912 (7 MiB)", "NTFSSearchDLL:5649-5678 (2 MiB MFT)",
   "DRWUI:919080-919092 / :962250-:962597" — all CONFIRMED at the exact lines.
3. "prefetch run size (1000 sectors) and lookahead (10 runs)" — CONFIRMED
   (1000 at :962281/:919129; 10 at :962283/:919131), with the caveat that the
   scan factory overrides the run to 0x400000/sectorSize at :919142 before
   priming the ring.
4. "cache budget (0x400000 = 4 MiB, count 1000)" — nuance: 0x400000 is the cache
   chunk/run (in sectors) and the 4-MiB consumer buffer; 1000 is the ctor-default
   run size in sectors; the ring holds 10 runs.
5. "64 KiB boot region" — UNVERIFIED / not present in the decompiled text; the
   only 0x10000 disk-adjacent reads are log-export thunks (Boot:4385/:13223), and
   the MBR/EBR walk reads single sectors (Partition:985/:1022/:1034/:1043).
6. "\\.\rda style paths" — corrected: the actual grammar is
   `\\.\EPRDTDRV\<disk>\<part>` / `\\.\EPRDTDRV\<disk>\Volume\<vol>` with a
   PhysicalDrive/HarddiskVolume fallback when not in the WinPE environment.
7. "the exact carve window is not decompiled" (REFERENCE-SCAN.md §12) —
   superseded: carve window verified at RAWSearchDLL:9271-9296 (4 MiB chunk,
   2× double buffer + 0x1000 tail).

## 10. Summary verdicts

- Overlapped-or-not: **synchronous everywhere in the scan path** — decisive
  evidence: `(LPOVERLAPPED)0x0` at Device_decompiled.c:55964-55965 (Read),
  :56156/:56178 (Write), plus zero IOCP/threadpool/ReadFileEx hits corpus-wide.
- Queue behavior: no I/O queues at all; the only producer-consumer structure is
  the CStorageDeviceWithCache 10-run ring with one prefetch thread, one consumer
  wait, one CRITICAL_SECTION, two manual-reset events (DRWUI:962530-962794).
- Thread budget: 45 CreateThread sites in DRWUI (1 prefetch thread per scanned
  device; N affinity-pinned engine channel threads from SearchDataSource;
  remainder UI/preview/workers). SearchDLL engines themselves single-threaded.
- Per-transport: BusType collected (WMI + IOCTL 0x2d1400 descriptor) but no
  size/depth parameter varies by transport class.
