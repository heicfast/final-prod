# DRIVER-CONTRACT — the RDA driver's complete contract (driver-replacement context sheet)

> **Provenance.** Driver repo `rda-driver` read end-to-end at commit `323e75acf4cd745960813af4e7fd586a4a34d1a0` (tag `v0.2.4`, branch `main`), READ-ONLY. Every claim cites `rda-driver/<path>:<line>` (paths relative to the rda-driver repo root). Cross-read: the core's consumer-side view `core_recovery/docs/DRIVER-CONTRACT.md` (evidence base: v0.2.2 @ `da7efb9`) and `core_recovery/include/cr/win/cr_rda.h`. **Where the two views diverge, the driver source wins** — divergences are listed in §0.
>
> Consumer of this sheet: the driver-replacement session (direct user-mode raw I/O under admin). Agent C / task T2-c.

---

## §0 Divergences resolved (driver source vs core docs / core header)

| # | Claim in core docs / cr_rda.h | Driver source truth (v0.2.4) | Resolution |
|---|---|---|---|
| D1 | `docs/DRIVER-CONTRACT.md:105` + `cr_rda.h:43-49`: extended IOCTL codes "0x6002440/44/48 — the driver header's comments carried a phantom extra zero" | The header comments were FIXED at commit `163e3cc` ("the IOCTL comments' phantom extra zero - CTL_CODE_RDA(0x910) is 0x6002440, not 0x60002440"); current `rda-driver/src/shared/inc/rda_ioctl.h:77-79` reads `/* 0x6002440 */`, `/* 0x6002444 */`, `/* 0x6002448 */` and the macro `(0x600 << 16) \| (fn << 2)` at `rda_ioctl.h:75` evaluates exactly to those values; the dispatch switch cases match (`rda-driver/src/driver/rda_ioctl.c:276-281`) | **Codes confirmed: GET_INFO 0x6002440, OPEN_TARGET 0x6002444, GET_VERSION 0x6002448.** The identity codes 0x60002400/04/08 are RAW reference wire values, deliberately not derivations (`rda_ioctl.h:54-62`) |
| D2 | `docs/DRIVER-CONTRACT.md:193` claims the per-handle probed sector size is "used for 4Kn correctness at the forwarding boundary" (inherited from `rda_p.h:91-94` and `rda_target.c:26-27` comments) | The grammar-path READ/WRITE never consults `Context->SectorSize` — zero references in `rda_readwrite.c` (verified by search; the field is set at `rda_target.c:152,238,293` and consumed only by the session engine / disk cache / filter). Offsets and lengths forward verbatim as bytes; `Information = Length × 512` regardless of media | **No 4Kn translation exists on the grammar path.** Byte transparency is the contract; the storage stack enforces alignment. Direct `\\.\PhysicalDriveN` handles behave the same way (byte offsets, stack-side alignment enforcement) |
| D3 | `docs/DRIVER-CONTRACT.md:197` already flags it: header comment (`rda_ioctl.h:366`) + `docs/ground-truth-contracts.md:319` claim the global-handle store mode is "gated to admin callers" / `SeAdminCheck` | No admin check exists on that path (`rda_readwrite.c:317-326`); the DACL at device open is the only gate | Driver source wins: the SDDL is the sole boundary (see §6) |
| D4 | `rda_ioctl.h:155` comment says the session set was "decoded ... (18 codes)" | The define list (`rda_ioctl.h:183-201`) and the dispatch switch (`rda_ioctl.c:47-67`) contain **19** codes | 19 recognized session-family codes (the "18" is a stale prose count) |
| D5 | Core `cr_rda.h:87-91,118-124`: OPEN_TARGET and the session codes are documented as wire mirrors only — "RdaClient ... deleted - zero callers since introduction" | Confirmed against `core_recovery/src/win/cr_transport.cpp`: the core's production surface uses only the **grammar-path opens + GET_INFO/GET_VERSION (query_driver_status) + pass-through IOCTLs**; no session command is issued by any live core code | The session engine is documented here for completeness (§2.4, §3.4) but is **not** part of the core's replacement-critical surface |

---

## §1 The driver's user-mode surface

### 1.1 Service, control device, naming

- One WDM kernel driver, installed as a demand-start kernel service (`type= kernel start= demand error= normal group= "Boot File System"` — `rda-driver/scripts/install.ps1:185`; INF equivalent `rda-driver/src/driver/rda.inf:45-51`). Default service name `rda` → control device `\\.\RDA` (`rda-driver/scripts/install.ps1:7-9`; PE profile `rda-driver/scripts/winpe/regdrv-rda.reg:13-18`).
- **Service-name-derived identity**: device = `\Device\<UPPERCASE(last component of RegistryPath)>`, DOS symlink = `\DosDevices\<same>`, fallback literal `EPMNTDRV` — `rda-driver/src/driver/rda_control.c:4-10,27-71,96-101`; prefixes/fallback at `rda-driver/src/driver/inc/rda_p.h:46-51`. Consequence: the same `rda.sys` under service `eprdtdrv` answers `\\.\EPRDTDRV` (reference drop-in), `%s%d` hot-upgrade variants coexist. Optional registry `DeviceName` (service root or `Parameters\`) overrides the derived base (EUEDKEPM parity) — `rda-driver/src/driver/rda_registry.c:120-127,158-162`, `rda_control.c:96-97`.
- Device type `FILE_DEVICE_UNKNOWN` (0x22), **`DO_DIRECT_IO`** flag set — `rda_control.c:16-17,152-160` (IO manager builds MDLs for READ/WRITE). Symlink created at `rda_control.c:162`, deleted at unload `:190-204`.
- Dispatch: one master dispatcher on all 28 majors routes `DeviceObject == g_RdaControlDevice` to the control table, everything else to the filter path — `rda-driver/src/driver/rda_driver.c:97-111` (G11 fix). Control majors owned: CREATE@0x00, CLOSE@0x02, READ@0x03, WRITE@0x04, DEVICE_CONTROL@0x0E, CLEANUP@0x12 (release, fix F9); all other majors complete `STATUS_INVALID_DEVICE_REQUEST` — `rda_driver.c:62-95` and `:50-60`.

### 1.2 The grammar path (sub-device opens are file names on the control device)

All forms are the FileObject name **relative to** `\\.\RDA`, parsed over non-NUL-terminated ranges (`rda-driver/src/shared/inc/rda_compat.h:23-26`):

| User-mode path | Class | Target resolved |
|---|---|---|
| `\\.\RDA` (empty name) | GLOBAL (1) | none — identity IOCTLs, extended IOCTLs, session protocol, legacy global-handle R/W modes (`rda_compat.h:177-195`, accepted only for the EMPTY name: the gate is `FileName.Length < 2` BYTES; a 1-wchar name REJECTS) |
| `\\.\RDA\<token>\Volume\<N>` | VOLUME (2) | `\Device\HarddiskVolume<N>` via `IoGetDeviceObjectPointer`, then **top-of-stack** (`IoGetAttachedDeviceReference`), Mode 0 — `rda-driver/src/driver/rda_target.c:129-167` |
| `\\.\RDA\<token>\<disk>\<part>` | DISK (3) | EPMNtDrv 3-segment: `\Device\Harddisk{d}\Partition0`, on failure retry `\Device\Harddisk{d}\DR{d}` (disk number as BOTH varargs — instruction-verified, G4), then walk DOWN from top-of-stack to the first layer whose `ObQueryNameString` name contains `\Device\Harddisk`; that layer is the IO target (skips storage filters above). **The part number is never parsed or used** (reference-vestigial) — `rda_target.c:169-245` (retry at `:189`, walk `:216-228`), classifier `rda_compat.h:257-287` |
| `\\.\RDA\<token>\<disk>` | DISK_EPM (4) | EUEDKEPM 2-segment: `Partition0` only (no DR fallback), walk DOWN to the first layer whose driver `ServiceKeyName == L"Disk"`; that layer is the target, Mode 0 — `rda_target.c:247-300` |

Grammar edge rules (reference-exact, `rda_compat.h:161-288`): separator scan starts at index 1; empty token → REJECT; token must parse as decimal (leading whitespace/`+` tolerated, matching `RtlUnicodeStringToInteger`); volume branch requires the remainder **separator-inclusive** > 8 wchars and matches `\Volume` AT the separator; **volume-number parse failure falls back to volume 0** (`rda_compat.h:244-253`); token-only path (no target) → REJECT.

### 1.3 PID-token handshake

`token = b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24` over the little-endian bytes of the **calling process's own 32-bit PID**, 8-bit wrapping adds — `rda_compat.h:49-57`, spec `rda_ioctl.h:113-122`. The driver re-derives it at CREATE from `PsGetCurrentProcessId()`; mismatch → `STATUS_INVALID_PARAMETER` — `rda-driver/src/driver/rda_create.c:73-77`. Explicitly NOT a security boundary (the DACL is); retained for wire compatibility.

### 1.4 Numeric bounds

DISK kind: `DiskNumber >= MaxDisks` (default 200, registry-clamped 2..1024) → `STATUS_INVALID_PARAMETER`; DISK_EPM kind: `>= 100` hard-coded — `rda_create.c:79-85`, limits `rda_p.h:55-56`, clamp `rda-driver/src/driver/rda_registry.c:137-141`.

### 1.5 Handle kinds and lifecycle at open

- Global/bare open: succeeds with NULL FsContext/FsContext2 — `rda_create.c:60-65`.
- Bound open: context `{TargetFileObject, TargetDevice, Mode, Kind, SectorSize}` (target refs adopted) into `FsContext`/`FsContext2` — `rda_create.c:87-116`, layout `rda_p.h:74-95`. `Mode` = 0 when the chosen layer's driver ServiceKeyName is `Disk`, 1 otherwise (the comparison can never equal the bare `L"Disk"`, so mode is effectively constant 1 for disk opens — reference-exact, G5, `rda_target.c:76-98`).
- **CLEANUP** releases the context (fix F9; the reference released at CLOSE) **and** destroys every copy-session owned by the calling process; **CLOSE** just completes SUCCESS — `rda_create.c:140-179` (sweep call at `:162`).
- No handle-count limit, no sharing restriction (`Exclusive=FALSE`, `rda_control.c:152`); every open re-walks the object namespace (no cross-handle caching of grammar targets).

---

## §2 The wire contract

Device type for custom codes: `0x600` (`rda_ioctl.h:51`). All identity/extended IOCTLs are `METHOD_BUFFERED`/`FILE_ANY_ACCESS`.

### 2.1 IOCTL dispatch order (control device) — `rda-driver/src/driver/rda_ioctl.c:247-286`

1. **Session-family codes** (normalized `code - IoctlBase`, matched explicitly over the 19-code set, `rda_ioctl.c:38-72`): served on **any** handle when `SessionsEnabled != 0` (default 1).
2. **Bound handles**: `0x60002408` (GET_DISK_CONTEXT_MODE) is answered locally; **everything else passes through** to the target's top-of-stack (§5).
3. **Global handles**: identity + extended IOCTLs; anything else → `STATUS_INVALID_PARAMETER` (`rda_ioctl.c:282-285`).

### 2.2 Identity IOCTLs (EPMNtDrv contract — raw codes)

| IOCTL | Code | Gates | Semantics |
|---|---|---|---|
| GET_PROC_NAME | `0x60002400` | in == 4 (u32 PID), out ≥ 0x208 | Image path of any PID: `PsLookupProcessByProcessId` + `SeLocateProcessImageName` (substitutes the reference's `NtOpenProcess(0x20000)`), NUL-terminated WCHAR into SystemBuffer, `Information = pathBytes + 2` — `rda_ioctl.h:63,68-69`, `rda-driver/src/driver/rda_ident.c:46-82` (copy helper `:28-44`) |
| GET_FILE_OBJ_NAME | `0x60002404` | in == 4 (u32 HANDLE), out ≥ 0x208 | Fully-qualified NT name of a file handle: `ObReferenceObjectByHandle(..., *IoFileObjectType, KernelMode)` → `ObQueryNameString(DeviceObject)` + append `FileName`; `Information = bytes + 2` — `rda_ident.c:84-148,150-161` |
| GET_DISK_CONTEXT_MODE | `0x60002408` | out == 4 exactly | Writes the per-handle `Mode` LONG, `Information = 4`; bound handles only — `rda_ident.c:163-174` |

All gate failures → `STATUS_INVALID_PARAMETER` (`rda_ident.c:55-58,155-158,167-170`). On bound handles, GET_PROC_NAME/GET_FILE_OBJ_NAME fall through to pass-through (reference behavior).

### 2.3 RDA extended IOCTLs (function codes 0x910-0x912)

`CTL_CODE_RDA(fn) = (0x600 << 16) | (fn << 2) | METHOD_BUFFERED` — `rda_ioctl.h:75`.

| IOCTL | Code | In/Out | Semantics |
|---|---|---|---|
| GET_INFO | `0x6002440` | out ≥ 24 | `RDA_DRIVER_INFO` (24 bytes, all u32): `StructSize`, `MajorVersion`=1, `MinorVersion`=0, `Flags` (`0x1` filter role active, `0x2` sessions active), `MaxDisks`, `IoctlBase`; `Information = 24` — `rda_ioctl.h:82-92`, `rda_ioctl.c:75-98` |
| OPEN_TARGET | `0x6002444` | in ≥ 24, out == 0 | Structured bind: `RDA_OPEN_TARGET_IN` `{StructSize, Kind (1=VOLUME/2=DISK/3=DISK_PART), DiskNumber, PartNumber, VolumeNumber, Reserved}` — `rda_ioctl.h:95-106`. Validation: already-bound handle → `STATUS_INVALID_DEVICE_STATE`; DISK ≥ 100 / DISK_PART ≥ MaxDisks → `STATUS_INVALID_PARAMETER`; no PID token required (DACL is the boundary) — `rda_ioctl.c:115-184` |
| GET_VERSION | `0x6002448` | out ≥ 8 | `ULONG64 = (Major << 32) \| Minor` = `0x0000000100000000`; `Information = 8` — `rda_ioctl.c:100-110`, `rda_ioctl.h:109-110` |

**Version negotiation: none is enforced.** The driver never refuses a client on version. Five independent version surfaces exist (extended GET_VERSION, GET_INFO fields, session GET_VERSION dword `0x00010100` + product string `"RDA Disk Copy Driver (c) RDA contributors"` ≤ 0x2E bytes at `rda_ioctl.h:347-352`, PE resource 1.0.0.0 at `rda-driver/src/driver/rda.rc:19-43` + `rda_version.h:3-5`, INF DriverVer `01/01/2026,1.0.0.0` at `rda.inf:22`). A consumer probes GET_INFO itself and compares (the `rdacli info` model — `rda-driver/src/client/rdacli.c:73-99`). `GET_INFO.IoctlBase` is echoed so clients can compute session codes (`rda_ioctl.c:94-95`).

### 2.4 Copy-session protocol (EUDCPEPM contract — 19 recognized codes)

Wire code seen by the driver = `custom + IoctlBase` (service-key DWORD at root or `Parameters\`; default 0, **WinPE sets 0x10000** — `rda_ioctl.h:149-153`, `rda_registry.c:120-127,134-136`, `regdrv-rda.reg:21`). Buffer gates: `rda_ioctl.h:203-216`. Command table (control-device behavior; `rda-driver/src/driver/rda_session.c:1365-1508`):

| Code | Name | Gates | Semantics |
|---|---|---|---|
| 0x800C2AC0 | DISCOVERY | out ≥ 4 (filter) | Control device: **no-op SUCCESS** (`rda_session.c:1410-1419`). Filter stacks: writes bound diskId, `Information = 4`, re-probes sector size — `rda-driver/src/driver/rda_filter.c:210-231` |
| 0x800C2AC8 | GET_VERSION | in ≥ 0x84 | dword `0x00010100` + product string at +4; `Information = 0` — `rda_session.c:1376-1393` |
| 0x800C2AD0 | OPEN_SESSION | in ≥ 0x840 | Full 0x840 overlaid struct (below); overlap guard; outputs handle@+0x834, spb@+0x30, sectorSize@+0x83C; `Information = 0` — `rda_session.c:731-838` |
| 0x800C2AD8 | SESSION_OP | in ≥ 8 | Flush the protected file's volume (path = WCHAR string at head of the session's 0x800 bitmap buffer), `IRP_MJ_FLUSH_BUFFERS` ≤ 20 retries; state 0→1→3; **always returns 0** — `rda_session.c:840-941` |
| 0x800C2AE0 | SESSION_OP2 (StartBackup) | in ≥ 0x28 | Requires state 3 else `STATUS_UNSUCCESSFUL`; locks pump-buffer MDL; `Information = 0` — `rda_session.c:944-1054` |
| 0x800C2AE8 | GETNEXTBLOCK | in ≥ 0x1E, out ≥ 0x1E | **Async copy pump**: IRP queued (cap 2000 → `STATUS_NO_MEMORY`), returns `STATUS_PENDING`; worker copies sectors into the StartBackup-registered pump buffer and completes the IRP with the 0x1E record; `Information = 0x1E` constant; no active plan → literal `0xC0000120` with progress fields filled — `rda_session.c:1472-1501,435-571` (literal at `:465`) |
| 0x800C2AF0 | SESSION_STATUS | in ≥ 8 | Returns the block-map status **as the NTSTATUS** (no buffer write) — `rda_session.c:1455-1457` |
| 0x800C2AF8 | GET_PROGRESS (FINALIZE) | in ≥ 0x0C | State → 5, frees plan + pump MDL + COW, percent at buffer +8, `Information = 0` — `rda_session.c:1057-1085` |
| 0x800C2B00 | CLOSE_SESSION | in ≥ 8 | Destroys the session; `Information = 0` — `rda_session.c:1462-1465` |
| 0x800C2B0A / 0x800C2B12 | DIRECT_READ / DIRECT_WRITE | out > 0x10 | `METHOD_OUT_DIRECT`; header+data in the MDL'd output buffer `{u32 diskId@+0, u32 sectorCount@+4, u64 startLba@+8, data@+0x10}`; offsets are **LBA × probed sector size**; session-FREE (diskId from header); `Information = OutputBufferLength` **always** — `rda_session.c:1091-1139`, dispatch `:1398-1408` |
| 0x800C2B18 / 0x800C2B20 | MODE4 / MODE3 | in ≥ 0x1C, out ≥ 4 | Raw **paging-IO** file write/read by handle `{HANDLE@0, PVOID Buffer@+8, u64 ByteOffset@+0x10, u32 Length@+0x18}`; `IRP_PAGING_IO` flag set, caller's file object injected; `Information = 8` on success — `rda_session.c:1143-1194,1425-1431` |
| 0x800C2B2A / 0x800C2B2E | SIZED_XFER / SIZED_XFER2 | out > 0x10 | Same shape as direct; sector size from the per-disk cache — `rda_ioctl.h:169-170` |
| 0x800C2B38 | CANCEL | in ≥ 8 | State := 3 (standby) + worker kick; `STATUS_SUCCESS` — `rda_session.c:1196-1200,1467-1470` |
| 0x800C2B3E / 0x800C2B40 / 0x800C2B42 | RRWR write-probe/read family | filter-only | Control device: no-op SUCCESS (`rda_session.c:1410-1419`). Filter stacks: magic `0x52455752` ("RRWR") required; version probe answers `0x604 @+8, Information = 0xC`; read/write wire `{magic@0, sectorSize@+8 must match, declaredOutLen@+0xC must equal actual, lba@+0x10 (u64), count@+0x18}`, data = whole MDL buffer, `Information = outLength` on success — `rda_filter.c:233-309`, magic `rda_ioctl.h:228` |

Handle-taking commands (SESSION_OP, SESSION_OP2, SESSION_STATUS, GET_PROGRESS, CLOSE_SESSION, GETNEXTBLOCK, CANCEL): first 8 bytes = session handle. `inLength < 8` or NULL buffer → `STATUS_INVALID_HANDLE`; invalid handle value: CLOSE_SESSION/GET_PROGRESS → `STATUS_INVALID_HANDLE`, others → `STATUS_INVALID_PARAMETER` (`rda_session.c:1433-1445`). **Unknown session codes → `STATUS_SUCCESS`** (reference fall-through, `rda_session.c:1503-1506`).

**Wire structs** (compile-time frozen with `C_ASSERT`s — `rda_ioctl.h:245-254,272-281,292-298,332-345`; runtime-frozen by `rda-driver/tests/unit/test_compat.c`):

- `RDA_OPEN_SESSION` (0x840, pack(4)): `+0x000 u32 DiskId` · `+0x004 u64 DataFirstSec` · `+0x00C u64 TotalSecs` (≠0) · `+0x014/+0x01C u64 ExtentParam1/2` · `+0x024 u64 BitmapSize` (≠0) · `+0x02C u32 BitmapCount` (≠0) · `+0x030 u32 SpbInOut` (IN 0 → driver picks `TotalSecs < 0x20 ? 1 : 0x10`, tail = `TotalSecs & 0xF` only in that case — G13; OUT overwritten with effective spb) · `+0x034 u8 BlockBitmap[0x800]` (NUL pad @0x7FE; the buffer head doubles as the SESSION_OP protected-file path — reference storage) · `+0x834 u64 SessionHandle` (OUT, opaque) · `+0x83C u32 SectorSize` (OUT) — `rda_ioctl.h:300-345`, handler `rda_session.c:731-838` (spb rules `:779-792`).
- `RDA_STARTBACKUP_IN` (0x28, pack(4)): `{u64 Handle@0, PVOID PumpBuffer@+8 (user VA, MDL-locked), u32 PumpBytes@+0x10 (must be spb×sectorSize-aligned), PVOID UserBitmap@+0x14 (optional exclusion bitmap, ProbeForRead), u32 StartSec@+0x1C, u32 BlockCount@+0x20, u32 Spb@+0x24 (must match when bitmap supplied)}` — `rda_ioctl.h:256-281`, handler `rda_session.c:944-1054`.
- `RDA_GETNEXTBLOCK` (0x1E, pack(1)): in `{u64 Handle@0, u64 CursorSec@+8, u32 MaxSec@+0x10}`; out `{u32 DiskId@+0x14, u32 Progress@+0x18, u8 MapProgress@+0x1C, u8 MapOk@+0x1D}`. **The copied data lands in the pump buffer, NOT the IRP buffer** — `rda_ioctl.h:230-254`.
- `RDA_FILEIO_IN` (0x1C, pack(4)): `{HANDLE FileHandle@0, PVOID Buffer@+8, u64 ByteOffset@+0x10, u32 Length@+0x18}` — `rda_ioctl.h:283-298`.

**Session engine limits** (`rda_session.c:65-73`): 64-slot table, generation-tagged opaque handles `(slot+1) << 16 | generation` (never kernel pointers — F12, `rda_session.c:151-167`); worker kicks capped 301 (`0x12D`, reference); pump queue cap 2000; flush retries 20; COW snapshot memory capped 64 MiB/session; worker guard timeout 10 s. State machine (spinlock-guarded): `0 created → 1 flushing → 3 standby → 4 copying → 5 done`; CANCEL forces 3; StartBackup requires 3 — `rda_ioctl.h:218-225`.

### 2.5 Legacy global-handle modes (bare opens) — `rda_ioctl.h:361-368`, `rda_readwrite.c:317-340`

- WRITE with `Length == 4`, no target: parks the caller's u32 handle value in the driver-global slot (reference GetDriverInfo handshake). No per-request admin check (see D3).
- READ with slot nonzero and out length ≥ 0x208: resolves the parked handle's object name into the buffer (same service as GET_FILE_OBJ_NAME).

---

## §3 The data path (grammar-path READ/WRITE, and what lies beneath)

### 3.1 Buffering: DO_DIRECT_IO, MDL, no extra copy of the data

- The control device sets `DO_DIRECT_IO` (`rda_control.c:159-160`), so the IO manager probes/locks the caller's buffer and builds an MDL for READ/WRITE.
- The driver maps it once with `MmGetSystemAddressForMdlSafe(NormalPagePriority)`; NULL → `STATUS_INSUFFICIENT_RESOURCES` — `rda_readwrite.c:294-305` (equivalent to the reference's `MmMapLockedPagesSpecifyCache` minus the bugcheck path).
- `Parameters.Read/Write.Length` is **BYTES**; `ByteOffset` is the **absolute byte offset on the target** (clients use `SetFilePointer`) — contract header `rda_readwrite.c:4-15`, `rda_ioctl.h:132-145`. **There is no data copy and no bounce buffer in the driver**: the system VA of the caller's own pages is handed to the forward IRP (`rda_readwrite.c:224-225`).

### 3.2 IRP flow and completion model (per request)

`RdaDispatchReadWriteInner` (`rda_readwrite.c:280-367`) → `RdaForwardSynchronous` (`:185-278`):

1. Negative offset → `STATUS_INVALID_PARAMETER` (`:196-198`). Zero length or unmapped MDL → `STATUS_INVALID_PARAMETER` (`:342-347`; documented sane divergence S10 — the reference NULL-dereferenced its MDL there).
2. Per-IO transfer context (`{Wake event, DoneFlag, Cancelled, CancelDone event, Iosb, ForwardIrp}`) from a driver-wide **SLIST free-list pool** (16-byte aligned, depth-capped 32; heap fallback) — `rda_readwrite.c:49-64,78-87,134-141`. (Round-2 optimization: the per-IO heap alloc/zero cost ~10% random IOPS before this.)
3. Cancel routine armed BEFORE forwarding (`:213-213`), with an already-cancelled race check (`:218-222`).
4. A **fresh driver-built IRP** via `IoBuildAsynchronousFsdRequest(MajorFunction, TargetDevice, SystemAddress, Length, ByteOffset, &ctx->Iosb)` (`:224-225`), completion routine returns `STATUS_MORE_PROCESSING_REQUIRED` (`:154-167`), `IoCallDriver` (`:236`).
5. **Synchronous wait** in the caller's thread: flag-checked `KeWaitForSingleObject` loop; on a cancel wake the dispatcher propagates cancellation INTO the storage stack with `IoCancelIrp(forward)` on the still-owned forward IRP (`:242-257`) — cancellable forward-wait, fix F4 (the reference waited with no cancel support and hung on dead drives).
6. Single-completer invariant: only the dispatcher completes the caller IRP; `STATUS_CANCELLED` when cancelled; disarm + `CancelDone` handshake before freeing the context; `IoFreeIrp(forward)` — `:261-277`.
7. On success: **`Information = Length × 512`** (`RDA_LEGACY_INFORMATION_SCALE` = 512, `rda_ioctl.h:146`; applied at `rda_readwrite.c:267-273` with `ScaleInformation = TRUE` from `:349-350`). This is fix F11 — *intentionally preserved misleading metric*; legacy clients divide by 512 to recover the true byte count (`rda-driver/src/bench/rdabench.c:73-79`). On failure: `Information = 0`, the storage stack's NTSTATUS propagates verbatim, error trace lines emitted (`rda_readwrite.c:352-366`).

**Completion model exposed to user mode: synchronous in-line** — the IRP is completed before the dispatch returns; only GETNEXTBLOCK ever returns `STATUS_PENDING` (`rda_ioctl.c:295-299`, `rda_session.c:1500`). OVERLAPPED clients get the IO manager's synchronous-handle serialization, not driver async.

### 3.3 Concurrency, queue depth, read sizing, alignment

- **No queue, no queue depth, no read sizing, no merging, no caching, no retry**: one forward IRP per request, immediately awaited. Concurrency is purely whatever the client issues (any number of handles/threads; no driver-side serialization, no per-handle lock — the only shared state is the free-list and the global-handle slot).
- **No driver-side alignment enforcement** on this path: byte offsets and byte lengths forward verbatim (§0 D2). Sector alignment is the storage stack's job; the core's own `aligned_read_at` bounce logic already handles unaligned requests for direct handles (`core_recovery/src/win/cr_transport.cpp:193-212`) and the driver-side equivalent is "the stack rejects it".
- **Sector-size handling (512 / 512e / 4Kn)**: at open, every context probes the target's logical sector size via `IOCTL_DISK_GET_DRIVE_GEOMETRY` against the attached stack, accepting 512 / 4096 / multiples of 512, falling back to 512 when the target does not answer geometry — `rda_target.c:302-370`. The probe is **cached per unique device pointer** (bounded 64 entries, oldest-evicted, ~100 ns hit path — the round-2 fix for the +10% open-latency gap) — `rda_target.c:310-445`. On the grammar READ/WRITE path the probed value is stored but **not consulted** (D2): `Information = Length × 512` is reported regardless of media. The value IS used by: the session engine's disk cache (`rda_disk.c:191-193`: `byteOffset = LBA × SectorSize`, `length = SectorCount × SectorSize`), the direct/sized commands (`rda_session.c:1122`), and the filter's RRWR/write-intercept paths (`rda_filter.c:281,295,331-332`).
- Buffer sizes seen in-tree: bench uses up to 16 MiB (`rdabench.c:38`), default block 1024 KiB (`rdabench.c:430`); no driver maximum (the IO manager/MDL and the target stack bound it).

### 3.4 The session engine's data path (documented; not in the core's production surface)

Per-session system worker thread services a spinlock-guarded pump queue (`rda_session.c:573-621`); GETNEXTBLOCK copies `min(MaxSec, pump capacity, plan remaining)` sectors from the source disk into the MDL-locked user pump buffer in spb-sized chunks, skipping skip-bit blocks, COW-serving ranges saved from external writers (`:435-571`). Direct/sized transfers are single-shot synchronous sector IO through the per-disk target cache (32 entries, `rda_disk.c:23`) with LBA→byte translation (`rda_disk.c:175-198`). MODE3/4 issue paging-IO read/write against a caller file object's volume stack (`rda_session.c:1143-1194`).

### 3.5 What the grammar-path driver does that a direct `CreateFile` + `ReadFile` path does NOT

1. **Target resolution with layer skipping**: the DISK grammar binds the *disk-class layer* found by walking DOWN past filter devices (name-contains-`\Device\Harddisk` rule), and DISK_EPM binds the first `\Driver\Disk` layer — on a live OS this is effectively `\Device\HarddiskN\Partition0`'s disk stack, i.e. what `\\.\PhysicalDriveN` gives directly. The VOLUME grammar binds the **top of the volume stack** (including filters like volsnap) — equivalent to a volume device open. The part number is ignored (whole-disk access; partition selection is the client's offset math).
2. **The ×512 Information scaling** (F11) — a direct handle returns true byte counts.
3. **The PID-token gate** and grammar classification at CREATE.
4. **Pass-through IOCTL proxy with transfer-method awareness** (§5) — with direct access the core issues these IOCTLs to its own raw handle; the driver's proxy adds only the IRP hop.
5. **Identity services** (GetProcName/GetFileObjName) — privileged information-disclosure primitives with no direct user-mode equivalent of the same shape.
6. **Legacy global-handle read/write modes**.
7. **The session engine + filter role** (park/COW/TRIM-swallow) — WinPE copy-filter semantics with no user-mode equivalent.

---

## §4 The error model

User mode sees the `RtlNtStatusToDosError` mapping of the driver's NTSTATUS (via the IO manager). Full condition table (all from source):

| Driver condition | NTSTATUS | Site |
|---|---|---|
| Malformed grammar / token mismatch / disk bound exceeded / zero-length or unmapped-MDL IO / unknown IOCTL on global handle / bad identity-IOCTL lengths / OPEN_TARGET bad struct·kind·outLen | `STATUS_INVALID_PARAMETER` (0xC000000D → Win32 87) | `rda_create.c:67-85`, `rda_readwrite.c:342-347`, `rda_ioctl.c:124-154,282-285`, `rda_ident.c:55-58,155-158,167-170` |
| Output/input below command minimum (GET_INFO, GET_VERSION, OpenSession, StartBackup, GETNEXTBLOCK, GET_PROGRESS, direct-family out ≤ 0x10) | `STATUS_BUFFER_TOO_SMALL` (0xC0000023 → 122) | `rda_ioctl.c:79-81,103-105`, `rda_session.c:744-746,954-956,1063-1065,1104-1106,1378-1380,1476-1478` |
| Any allocation failure (context, bitmap, worker, forward IRP, pump node, name buffers) / session table full (64) / COW cap | `STATUS_INSUFFICIENT_RESOURCES` (0xC0000017 → 14) | `rda_create.c:87-90`, `rda_ioctl.c:156-159,231-234`, `rda_readwrite.c:200-203,226-231,295-300`, `rda_session.c:753,986-989,1008-1011,1483-1486,302-304` |
| Pump queue exhausted (2000 queued GETNEXTBLOCK) | `STATUS_NO_MEMORY` (0xC000009A → 14) | `rda_session.c:1480-1482` |
| Invalid session handle on CLOSE_SESSION/GET_PROGRESS; handle-command input < 8 bytes | `STATUS_INVALID_HANDLE` (0xC0000008 → 6) | `rda_session.c:1434-1444` |
| Invalid session handle on other handle-taking commands | `STATUS_INVALID_PARAMETER` (→ 87) | `rda_session.c:1438-1445` |
| OPEN_SESSION with TotalSecs/BitmapSize/BitmapCount == 0 | `STATUS_INVALID_DEVICE_REQUEST` (0xC0000018 → 1) | `rda_session.c:747-749` |
| OPEN_TARGET on already-bound handle; pass-through with no attachable top | `STATUS_INVALID_DEVICE_STATE` (0xC0000184) | `rda_ioctl.c:132-134,199-202` |
| Unsupported major on control device | `STATUS_INVALID_DEVICE_REQUEST` (→ 1) | `rda_driver.c:50-60` |
| METHOD_NEITHER pass-through | `STATUS_NOT_SUPPORTED` (0xC00000BB → 50) | `rda_ioctl.c:220-224` |
| Overlapping session range on same disk | `STATUS_SHARING_VIOLATION` (0xC0000043 → 32) | `rda_session.c:751-754`, `rda_ioctl.h:354-359` |
| StartBackup when state ≠ 3 | `STATUS_UNSUCCESSFUL` (0xC0000001 → 31 ERROR_GEN_FAILURE) | `rda_session.c:961-964` |
| GETNEXTBLOCK with no active plan | literal `0xC0000120` with progress fields filled (the source deliberately uses the raw "reference literal"; the core maps it to ERROR_GEN_FAILURE as the drain signal — `core_recovery/AUDIT-FALLBACKS.md:61`) | `rda_session.c:456-466` |
| Cancelled READ/WRITE | `STATUS_CANCELLED` (0xC0000122 → 1223) | `rda_readwrite.c:218-222,266` |
| Drained parked writes at teardown | `STATUS_DELETE_PENDING` | `rda_session.c:663-665` |
| Volume/disk target absent | resolution status propagates from `IoGetDeviceObjectPointer` (typically `STATUS_OBJECT_NAME_NOT_FOUND` → Win32 2) | `rda_target.c:121-126,181-195,254-259` |
| Non-admin/non-SYSTEM opens the device | `STATUS_ACCESS_DENIED` at the object-manager open (from the SDDL) → Win32 5 | `rda_control.c:141-153`, `rda_p.h:59-62` |
| Driver not loaded (no symlink) | `STATUS_OBJECT_NAME_NOT_FOUND` → Win32 2; the core also treats SCM `ERROR_SERVICE_DOES_NOT_EXIST` (1060) as absent | `core_recovery/src/win/cr_transport.cpp:433` |
| GetProcName PID-not-found / image query failure | lookup NTSTATUS propagates | `rda_ident.c:62-78` |
| GetFileObjName bad/non-file handle | `ObReferenceObjectByHandle` status propagates (`STATUS_OBJECT_TYPE_MISMATCH` / `STATUS_INVALID_HANDLE`) | `rda_ident.c:97-107` |
| Name longer than caller's buffer | `STATUS_BUFFER_TOO_SMALL` | `rda_ident.c:34-36` |
| **Storage-stack failures (the important class)**: bad sectors, dead devices, unaligned offsets rejected by the target, offline media | the lower stack's NTSTATUS **and** `Information` propagate verbatim — the driver adds no masking, no zero-fill, no retry, no remap | `rda_readwrite.c:266-273` (status = `ctx->Iosb.Status`), pass-through `rda_ioctl.c:236-244` |

Success conventions: READ/WRITE `Information = Length × 512`; GET_INFO 24; GET_VERSION 8; identity IOCTLs `nameBytes + 2`; GETNEXTBLOCK `0x1E`; direct-family `OutputBufferLength` always; MODE3/4 `8`; OPEN_SESSION / session GET_VERSION / GET_PROGRESS / CLOSE / CANCEL `0`.

---

## §5 The control path (pass-through IOCTLs on bound handles)

Every non-session, non-`0x60002408` IOCTL on a **bound** handle is forwarded to `IoGetAttachedDeviceReference(Context->TargetDevice)` (the current top of the target's stack) via `IoBuildDeviceIoControlRequest`, synchronously waited, `Information = iosb.Information`, lower status returned verbatim — `rda_ioctl.c:186-245`.

- `METHOD_BUFFERED`: out = SystemBuffer (reference parity).
- `METHOD_IN_DIRECT` / `METHOD_OUT_DIRECT`: out = `Irp->UserBuffer` (fix F10 — the reference passed SystemBuffer both ways).
- `METHOD_NEITHER`: rejected `STATUS_NOT_SUPPORTED` (`rda_ioctl.c:220-224`).
- No attachable top → `STATUS_INVALID_DEVICE_STATE`; IRP build failure → `STATUS_INSUFFICIENT_RESOURCES`.

**Nothing is special-cased**: `IOCTL_DISK_UPDATE_PROPERTIES`, `IOCTL_DISK_DELETE_DRIVE_LAYOUT`, `IOCTL_DISK_SET_DRIVE_LAYOUT`, `IOCTL_DISK_GET_DRIVE_GEOMETRY(_EX)`, `IOCTL_STORAGE_GET_DEVICE_NUMBER`, `IOCTL_STORAGE_QUERY_PROPERTY`, SMART/ATA/NVMe pass-throughs, etc. all ride the same generic proxy — there is no per-code table (`rda_ioctl.c:263-268` is the entire decision). This is how the core's geometry queries reach the disk (bench probe `rdabench.c:265-269` proves `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` pass-through). A direct raw handle provides the identical surface natively — the driver's proxy adds exactly one IRP build + wait + reference per IOCTL.

---

## §6 Lifecycle & security

### 6.1 Install / start / upgrade / uninstall

- `scripts/install.ps1` (v0.2.4 contract, `install.ps1:14-32`): elevation guard (exit 1); versioned layout `<ServiceName><MajorMinor>.sys` from the image's version resource — the reference `%s%d` coexistence scheme, in-use images never overwritten; different-version install over an existing service = in-place upgrade (stop → `sc config binPath` → shared Parameters write+verify → start; exits 8/9 on upgrade legs). Fresh install: copy (exit 3), `sc.exe create` (exit 4, per-call SCM error), `Parameters` values `IoctlBase/MaxDisks/DebugLevel/SessionsEnabled` written **and read back** (exit 5), start (exit 6 — FATAL on failure, testsigning guidance), RUNNING confirmation (exit 7). `-NoStart`, `-Uninstall` (delegates to `uninstall.ps1`).
- `uninstall.ps1`: 10×500 ms STOP_PENDING poll, checked stop/delete, post-delete registration check, ImagePath-aware file removal + versioned-leftover sweep, loud failures.
- DriverEntry sequence: parameters read → control device create → master dispatch install → session/disk-cache init → optional filter role (`FilterMode != 0`) — `rda_driver.c:113-175`. Unload tears down in reverse (sessions, disk cache, transfer pool, filter, device) — `rda_driver.c:33-48`.

### 6.2 DACL / security descriptor

- `IoCreateDeviceSecure` (wdmsec.lib, `rda.vcxproj:91-92`) with default SDDL **`D:P(A;;GA;;;BA)(A;;GA;;;SY)`** — Administrators + SYSTEM full access only — and a fixed class GUID `{22503FD1-90D4-4322-B62D-9C8FFE186E33}` so admins can tighten via the class key — `rda_control.c:141-153`, `rda_p.h:59-68`. Override via `Parameters\Sddl` (`rda_registry.c:163-167`).
- Consequence: a non-elevated `CreateFile` on `\\.\RDA...` fails at the object-manager open with `STATUS_ACCESS_DENIED` → Win32 5. The measured price: +2.2% open latency vs the reference, which had no DACL (`rda-driver/docs/benchmarks/2026-10-22-runner-comparison.md:36-39`).
- The PID token is retained but demoted ("NOT a security boundary — device SDDL is", `rda_ioctl.h:117-122`). No image allow-listing, no per-request ACL checks (fix set F2/F3; `rda-driver/docs/audit/2026-10-22-security-review.md:89-99`).

### 6.3 WinPE

`scripts/winpe/regdrv-rda.reg`: service `rda` (Type 1, demand start, group "Boot File System", `\SystemRoot\System32\drivers\rda.sys`) + `Parameters` `{IoctlBase=0x10000, MaxDisks=200, DebugLevel=1, SessionsEnabled=1, FilterMode=1, VolSnapAttach=0}` + disk-class **UpperFilters** on GUID_DEVCLASS_DISKDRIVE (`{4d36e967-...}`) = `rda` — the documented mechanism replacing the reference's AddDevice patching (F6). In filter mode: AddDevice attaches a transparent DO per disk (magic `0xD2C8E4C5` handshake value in the extension, `rda_filter.c:40,42-49,389-425`); WRITEs coordinated with sessions (standby → parked; copying → COW-save then forward, `rda_filter.c:314-348`); TRIM (`IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES` action=Trim) swallowed while a session is active on the disk (`rda_filter.c:195-207`); discovery + RRWR served on filter stacks (`rda_filter.c:210-309`); PnP REMOVE kills sessions bound to the disk (`rda_filter.c:164-180`).

### 6.4 Registry parameters (read once at DriverEntry; service-root first, `Parameters\` overrides)

`IoctlBase` (default 0; PE 0x10000), `MaxDisks` (default 200, clamp 2..1024), `DebugLevel` (0..5, default 1 = errors), `FilterMode`, `VolSnapAttach`, `SessionsEnabled` (default 1), `DeviceName` (REG_SZ), `Sddl` (REG_SZ) — `rda_registry.c:8-23,115-171`. Tracing is `DbgPrintEx` only, no WPP/ETW — `rda-driver/src/driver/inc/rda_trace.h:4-15,32-38`.

### 6.5 Signing / build / CI surface (what the core's driver-artifact removal must cover)

- Build: VS2022 v143 + WDK/SDK NuGet `10.0.26100.6584` pinned via `packages.config` + `Directory.Build.props`; solution `rda.sln` = 3 projects (rda.sys, rdacli.exe, rdabench.exe) into `bin\x64\<cfg>\`; `NTDDI_VERSION=0x06010000` Win7 gating; `/W4 /WX`; `/osversion:6.1 /subsystem:native` + post-build `tools/stamp_osversion.py`; `SignMode=Off` (artifacts intentionally unsigned; CI asserts no signature table) — `rda.vcxproj:9-14,49,58,64,73,89,92`, `rda-driver/docs/win7-compat.md:19-41`.
- CI (`.github/workflows/`): `ci.yml` (lint + clang-format + PowerShell AST gate + MSVC builds + PE/import Win7 allowlist gate + CodeQL + unit tests), `benchmark.yml` (windows-2022; build → test-sign with job-local cert → seeded 4 GiB VHDX → both drivers side-by-side → matrix → SHA-256 cross-equality assertion), `release.yml` (tag builds, staged bundle + SHA256SUMS, unsigned).
- Signing: OSSign pipeline prepared but DISABLED (`signing/ossign/sign.yml`, `review.yml`; requires repo vars + protected environment). Honest scope: OSSign Authenticode is not a Microsoft kernel signature; Win10/11 retail needs Hardware Dev Center attestation; WinPE is relaxed — `rda-driver/docs/signing.md:29-76`.

---

## §7 The performance baseline (every captured measurement)

**Run ID 39807929475** — the benchmark workflow's first fully green run (windows-2022 hosted runner, 4 GiB seeded dynamic GPT VHDX with NTFS + FAT32 and a realistic JPEG/PDF/MP4/ZIP/text/small-file corpus; our test-signed `rda.sys` as `rdatest` vs the ORIGINAL WHQL-counter-signed `eprdtdrv.sys`, side by side, same `rdabench` binary, page-aligned buffers + `FILE_FLAG_NO_BUFFERING`) — `rda-driver/docs/benchmarks/2026-10-22-runner-comparison.md:1-10`, `rda-driver/worklog.md:140-145`. Raw JSONL in that run's artifact.

| Metric | Original (eprdtdrv) | Ours R1 (pre-opt) | Ours R2 (post-opt) | R2 same-run delta |
|---|---:|---:|---:|---:|
| seq 64 KiB throughput | 391.0 MiB/s (R1 run) | 371.1 MiB/s | **532.9 MiB/s** | **−0.0%** (worklog: parity vs ref 533.1 MiB/s — `worklog.md:157-158`; the table's "Original" column mixes R1/R2 runs, noted there as ±5-10% runner variance) |
| seq 1 MiB throughput | 500.6 MiB/s | 458.6 MiB/s | (see note) | — |
| rand IOPS (4 KiB, seeded LCG) | 8,170 | 7,932 | **7,932** | **−2.9%** |
| rand latency mean | 122.4 µs | 126.1 µs | **126.1 µs** | **+3.0%** |
| open mean (cold, grammar open) | 8,042 ns | 10,993 ns | **8,217 ns** | **+2.2%** |
| open churn mean (100 open/close) | 8,537 ns | 11,331 ns | **8,532 ns** | **−0.1%** |
| fuzz hammer rate | 553,752 IOCTL/s | 707,161 | **841,698 IOCTL/s** | **+52.0%** |
| verify (SHA-256 per 1 MiB range) | — | — | **64/64 byte-identical, 0 mismatches** | byte-exact |
| geometry pass-through | 1.0 | 1.0 | 1.0 | contract ✓ |
| Information scaling | ×512 | ×512 | ×512 | contract ✓ |

Both drivers survived 50,000 hostile IOCTLs (fuzz mode) with zero hangs. Round-1 gaps and their fixes (`2026-10-22-runner-comparison.md:31-53`, `worklog.md:146-155`): per-open geometry probe cost **+10% open latency** → per-device cache (64 entries, ~100 ns hit); per-IO context alloc cost **−10% random IO** → SLIST free-list pool (depth 32); remaining +2.2% open ≈ the SDDL check; remaining −2.9% random ≈ cancel-safe, DACL-secured IO machinery. Live lesson: `ExAllocate/FreeFromLookasideListEx` are Win11-22H2-only real exports — importing them gave StartService error 127 on Server 2022 (import-gate hazard, `worklog.md:151-155`).

CI-side numbers: libFuzzer ~48.5M executions at ~800K exec/s, cov plateau 86/265, 112-entry corpus, zero crashes; 4019-input ASan/UBSan selftest, zero findings — `rda-driver/docs/benchmarking.md:82-92`. Expectations baseline: ≥90% of direct `\\.\PhysicalDriveN` sequential throughput, µs-class per-op dispatch overhead on 4 KiB random — `benchmarking.md:64-71`. `rdacli bench read` compares the driver path against a direct `\\.\PhysicalDriveN` baseline side by side (`rdacli.c:339-401`) — no committed numbers from that mode.

**Design implications for the replacement** (`rda_readwrite.c:49-64,185-278`): the driver's "queue model" is *none* — one synchronous forward IRP per request, per-IO context from an interlocked free-list; the session engine's queue design (worker thread, 2000-deep pump queue, 301 kicks, spb-sized chunks, 1 MiB-class pump buffers per the reference layout `rda-driver/docs/ground-truth-contracts.md:246-262`) applies only to the (unused-by-core) copy protocol. The replacement's throughput target is therefore the driver's R2 numbers above — which the reference-parity analysis says is essentially "the storage stack's own speed" plus one IRP build + MDL map per IO.

---

## §8 Guarantees the direct replacement must preserve

**Semantic**
1. Byte-granular, absolute device offsets; whole-IO semantics (no partial-read synthesis; failure → `Information = 0` + verbatim status). No masking, no zero-fill, no silent retry, no remap — a dead/bad sector surfaces the storage stack's error exactly as the driver did.
2. Target equivalence: disk grammar ≡ `\\.\PhysicalDriveN` (whole-disk device, disk-class stack); volume grammar ≡ volume-stack top; part# vestigial. The core passes part 0 in practice (`cr_transport.cpp:585`).
3. Pass-through IOCTL surface with verbatim status + `Information` (geometry, storage queries, layout-modifying IOCTLs, SMART passthroughs) — now issued directly on the raw handle. METHOD_BUFFERED semantics preserved trivially.
4. Synchronous completion per request; concurrent IO from any number of handles/threads with no cross-handle state.
5. The F11 unscale boundary: the core's transport already keys its `÷512` on `via_rda_` (`cr_transport.cpp:51-79,715,872`) — with the driver gone, exactly one code site must stop unscaling, and direct handles' true byte counts flow through.

**Error**
6. Every condition class in §4 must be reproducible from the direct path with the same user-mode-visible codes: absent device (Win32 2/3), access denied for non-admin (Win32 5 — the SDDL posture maps to the raw device's own admin requirement), invalid parameter classes (87), sharing violation class only if a replacement keeps range exclusivity (the core does not need it — sessions unused), storage-stack verbatim failures (CRC / device-not-connected / bad-blocks NTSTATUS → GLE).

**Threading / cancellation**
7. Cancellable synchronous reads: `CancelIo`/`CancelSynchronousIo`/handle close must abort a stuck read promptly (the driver invested F4 + S1 + S2 for exactly this; an OVERLAPPED direct path gets it natively).
8. CLEANUP-time resource release ≡ handle close releases everything (no client-visible state survives a closed handle; sessions died with the process at CLEANUP — `rda_create.c:159-162`).

**Lifecycle**
9. No driver install/start/service-presence probing in the product; the driver-status query (`query_driver_status`, `cr_transport.cpp:411-477`) is removed with the driver, not emulated.
10. Win7 SP1 → Win11 x64 coverage (the driver's PE contract was 6.1/NATIVE; the direct path needs only documented Win7-era user-mode APIs).

---

## §9 Where direct access can EXCEED the driver

- **One IRP build + one completion + one wait per IO, plus a per-IO context alloc** — `rda_readwrite.c:200-276` (context from pool `:78-87`, `IoBuildAsynchronousFsdRequest` `:224-225`, completion handshake `:242-277`). A direct `ReadFile` on `\\.\PhysicalDriveN` performs zero intermediate IRPs (the IO manager builds the FSD IRP directly). The driver's own fuzz-rate advantage (+52%) shows dispatch-path cost is measurable; the data path pays the same shape of overhead per IO.
- **The MDL map per request**: `MmGetSystemAddressForMdlSafe` over the caller's locked pages (`rda_readwrite.c:294-305`) — with DO_DIRECT_IO the direct path also gets an MDL, but the driver adds a system-VA mapping + the forward IRP referencing it.
- **The ×512 Information scaling** (multiply on success, `rda_readwrite.c:267-273`) — trivial arithmetic, but its removal also removes the client-side ÷512.
- **Per-open resolution walk**: every grammar open does `IoGetDeviceObjectPointer` + `IoGetAttachedDeviceReference` + a walk down with an `ObQueryNameString` (0x200-byte pool alloc + query) **per layer** — `rda_target.c:36-74,197-235` — plus the geometry probe (cached per device after round 2, `rda_target.c:310-445`). A direct `CreateFile("\\\\.\\PhysicalDriveN")` is one object-manager open. Measured driver open mean: 8,217 ns R2.
- **The SDDL check** (+2.2% open, the documented price of the DACL — `2026-10-22-runner-comparison.md:36-39`); the raw device's own security check replaces it with standard semantics.
- **The cancel machinery** (−2.9% random IO documented as the price of cancel-safe IO — `2026-10-22-runner-comparison.md:39-44`): the driver's forward-wait + cancel-routine + `CancelDone` handshake (`rda_readwrite.c:105-121,242-277`) is pure overhead relative to a native OVERLAPPED read the OS cancels for free.
- **The pass-through IOCTL hop**: every geometry/layout IOCTL pays `IoGetAttachedDeviceReference` + `IoBuildDeviceIoControlRequest` + wait + deref — `rda_ioctl.c:186-245`.
- **The master dispatch indirection** (`rda_driver.c:97-111`) and grammar classification per CREATE — negligible but nonzero.
- Net expectation (from the driver's own parity data): sequential throughput was already at storage-stack parity (532.9 vs 533.1 MiB/s), so the direct path's headroom there is small; random-IO latency and open cost are where the driver's own documented overheads (−2.9% IOPS, +2.2% open) can be recovered.

---

## §10 Known driver edge-state behaviors (with citations)

| Edge state | Driver behavior |
|---|---|
| **Device absent** (disk/volume number has no device) | Resolution status propagates from `IoGetDeviceObjectPointer` — typically `STATUS_OBJECT_NAME_NOT_FOUND` (Win32 2) at CREATE; volume-number parse failure falls back to **volume 0** and then fails the same way — `rda_target.c:121-126,181-195,254-259`, `rda_compat.h:244-253` |
| **Disconnect / dead drive mid-read** | The forward IRP's status propagates verbatim on completion (`rda_readwrite.c:266-273`); a hung lower stack is escapable because the driver arms a cancel routine and propagates `IoCancelIrp` into the storage stack (fix F4 — the *reference* hung here; `rda_readwrite.c:23-42,105-121,250-256`). Driver-side there is no timeout of its own (the session engine's internal sync path relies on stack timeouts — `rda_readwrite.c:369-375`) |
| **Mounted vs unmounted volumes** | No exclusivity or dismount logic anywhere: the volume grammar opens the volume stack top (`rda_target.c:129-167`) and forwards reads/writes regardless of mount state; sharing is unrestricted (`Exclusive=FALSE`). Raw writes to a mounted volume go straight through (the driver adds no lock — the reference's consistency came from the PE filter role's park/COW, which is inactive in the live-OS profile). The core's own mountpoint/offline handling sits above this layer |
| **Volume in use / locked by others** | No driver-level arbitration; conflicts surface as the storage stack's status on the forwarded IRP. Session-engine overlap guard (`STATUS_SHARING_VIOLATION`, `rda_session.c:751-754`) applies only to the (core-unused) session protocol |
| **Bad sectors / media errors** | Verbatim propagation of the storage stack's NTSTATUS with `Information = 0` on failure (`rda_readwrite.c:352-366`); no remap, no zero-fill, no retry. Error trace strings `[epmntdrvDispatchRead] Read error 0x%x,%u,%I64u` / `[epmntdrvDispatchWrite] ...` preserved for tooling parity (`:353-359`) |
| **Unaligned reads** | No driver-side alignment enforcement or emulation: byte offset/length forward verbatim (§0 D2); a target that requires sector alignment rejects the IRP itself (status propagates). Zero-length requests are rejected `STATUS_INVALID_PARAMETER` (S10 divergence — the reference crashed) — `rda_readwrite.c:342-347` |
| **Sector-size variants (512 / 512e / 4Kn)** | Probed at open (`IOCTL_DISK_GET_DRIVE_GEOMETRY`, 512/4096/multiples accepted, 512 fallback), cached per device — `rda_target.c:302-445`. Grammar-path IO is byte-transparent; `Information = Length × 512` is reported on **all** media (the ÷512 client unscale recovers true bytes on 4Kn too). LBA-translated paths exist only in the session/direct/RRWR family (`rda_disk.c:191-193`) |
| **Negative offsets** | `STATUS_INVALID_PARAMETER` — `rda_readwrite.c:196-198` |
| **1-wchar relative names / empty token / nondecimal token** | REJECT with `STATUS_INVALID_PARAMETER` (the global-mode gate is `Length < 2` BYTES, i.e. empty name only) — `rda_compat.h:177-215` |
| **Wrong PID token** (path built by another process) | `STATUS_INVALID_PARAMETER` (Win32 87) at CREATE — `rda_create.c:73-77`; the wrong-token negative test is part of the integration suite (`rdacli.c:150-163`) |
| **Disk number out of bounds** | DISK ≥ MaxDisks (200 default), DISK_EPM ≥ 100 → `STATUS_INVALID_PARAMETER` — `rda_create.c:79-85` |
| **Unknown IOCTL** | Global handle → `STATUS_INVALID_PARAMETER`; bound handle → passed through to the target; unknown *session-family* code → **`STATUS_SUCCESS`** (reference fall-through) — `rda_ioctl.c:282-285`, `:263-268`, `rda_session.c:1503-1506` |
| **Session handle abuse** (stale/garbage handle) | CLOSE_SESSION/GET_PROGRESS → `STATUS_INVALID_HANDLE`; other handle commands → `STATUS_INVALID_PARAMETER`; handles are opaque IDs into a generation-guarded 64-slot table, never pointers (F12) — `rda_session.c:151-167,1433-1445` |
| **Client death with open sessions** | CLEANUP sweeps and destroys the owner's sessions (workers stopped, pump MDLs unlocked) — `rda_create.c:159-162`, `rda_session.c:1222-1237` |
| **Disk removal (filter role)** | REMOVE waits for the lower stack (S4), kills sessions bound to the disk, purges the disk cache — `rda_filter.c:164-180`, `rda_disk.c:35-57` |
| **Hostile input** | Every command validates lengths/handles before touching memory; fuzz harness survived 50,000 random IOCTL codes/lengths per run; libFuzzer 48.5M executions zero crashes — `rdabench.c:381-419`, `docs/benchmarking.md:82-92` |
| **Cancellation** | `STATUS_CANCELLED` to the caller + `IoCancelIrp` propagated into the storage stack; single-completer invariant; buffer pages stay locked until the forward IRP completes — `rda_readwrite.c:23-42,105-121,242-277` |

---

### Footer — sources read (driver repo, complete)

`src/driver/`: rda_driver.c, rda_control.c, rda_create.c, rda_readwrite.c, rda_ioctl.c, rda_ident.c, rda_target.c, rda_disk.c, rda_session.c, rda_filter.c, rda_registry.c, rda_util.c, inc/rda_p.h, inc/rda_trace.h, rda_version.h, rda.inf, rda.rc, rda.vcxproj. `src/shared/inc/`: rda_ioctl.h, rda_compat.h. `src/client/`: rdacli.c + vcxproj. `src/bench/`: rdabench.c + vcxproj. `docs/`: architecture, ground-truth-contracts, benchmarking, benchmarks/2026-10-22-runner-comparison, error-fix-registry, requirements, research-digest, signing, testing, win7-compat, build, audit/2026-10-22-decompile-reverification, audit/2026-10-22-security-review. `scripts/`: install.ps1, uninstall.ps1, winpe/regdrv-rda.reg, run-integration.ps1, run-unit-tests.sh. `signing/ossign/`: sign.yml, review.yml. `packages.config`, `Directory.Build.props`, `rda.sln`, `.github/workflows/{ci,benchmark,release}.yml`, `worklog.md`, `tests/unit/test_compat.c` (referenced), `fuzz/` (referenced). Cross-read: `core_recovery/docs/DRIVER-CONTRACT.md`, `core_recovery/include/cr/win/cr_rda.h`, `core_recovery/src/win/cr_transport.cpp`, `core_recovery/worklog/session-driver-replacement-prompt.md`, `core_recovery/AUDIT-FALLBACKS.md` (mapping cross-checks only).
