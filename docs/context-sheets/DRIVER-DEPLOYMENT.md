# DRIVER-DEPLOYMENT.md — RDA (Raw Disk Access) kernel driver, exhaustive deployment sheet

> Produced by Agent C (Task 2-c) from a line-by-line read of **every** tracked file in
> `work/rda-driver` @ commit `da7efb9e0c94046893c62b25695bb95edf519f8b` (branch `main`, 66
> tracked files, working tree clean). READ-ONLY analysis; no repo file was modified.
> Every claim below cites its source file. This sheet is the sole driver input to the
> combined-installer integration plan.

---

## 1. Identity

| Property | Value | Source |
|---|---|---|
| Product | **RDA — Raw Disk Access Driver** | `README.md`, `src/driver/rda.rc` ("RDA Raw Disk Access Driver", CompanyName "heicfast / RDA contributors") |
| Binary | `rda.sys` (x64 kernel, WDM) | `README.md` "Quick facts", `src/driver/rda.vcxproj` (`DriverType>WDM`, `ConfigurationType>Driver`) |
| Model | **WDM, not WDF/KMDF** — "WDM, no framework dependency"; PlatformToolset `WindowsKernelModeDriver10.0`, `DriverType=WDM`, `DriverTargetPlatform=Desktop` | `README.md` line 8; `src/driver/rda.vcxproj` lines 45–58 |
| Purpose | One unified driver for raw disk/partition/volume sector I/O and sector-level copy sessions; clean-room merge of three EaseUS reference drivers: `EPMNtDrv` (`eprdtdrv.sys`, raw gate), `EUEDKEPM` (`EuDskAcs.sys`, 2-segment gate), `EUDCPEPM` (`EuBackup.sys`, disk-class filter + copy-session engine). `EuMrx.sys` excluded by decision. | `README.md` lines 3–6; `docs/architecture.md` §1; `docs/requirements.md` "Mission" |
| Targets | Windows 7 SP1 → Windows 11, **x64 only** (no x86, no ARM64 anywhere in the repo) | `README.md` line 8; `docs/win7-compat.md` "We do not run on Windows 7 x86 (x64 only)" |
| Language/toolchain | C17, WDK 10 via NuGet (pinned 10.0.26100.6584), MSVC v143 (VS2022), `/W4 /WX` zero-warning | `README.md` line 9; `packages.config`; `src/driver/rda.vcxproj` |
| Device class (control DO) | `FILE_DEVICE_UNKNOWN` (0x22, reference parity) with `DO_DIRECT_IO` flag | `src/driver/rda_control.c` header comment + line 152–160; `docs/ground-truth-contracts.md` §1.1 |
| Pool tag | `rda1` (`RDA_POOL_TAG ((ULONG)'rda1')`); reference used `esdr`/`ECLU` | `src/driver/inc/rda_p.h` line 45; `src/driver/rda_util.c` |
| Version | 1.0 (`RDA_VERSION_MAJOR 1`, `RDA_VERSION_MINOR 0`, FILEVERSION "1.0.0.0", INF DriverVer 01/01/2026,1.0.0.0) | `src/shared/inc/rda_ioctl.h` lines 106–107; `src/driver/rda_version.h`; `src/driver/rda.inf` line 22 |
| License | MIT ("heicfast and RDA contributors", (c) 2026) | `LICENSE` |

Kernel API surface (what the driver actually consumes): `IoCreateDeviceSecure` (wdmsec.lib), `PsLookupProcessByProcessId` + `SeLocateProcessImageName` (identity IOCTLs, replacing the reference's `NtOpenProcess`+`ZwQueryInformationProcess`), `IoBuildAsynchronousFsdRequest`, `IoBuildDeviceIoControlRequest`, `IoBuildSynchronousFsdRequest`, `IoSetCancelRoutine`/`IoCancelIrp`/`IoReleaseCancelSpinLock`, `MmProbeAndLockPages`/`MmGetSystemAddressForMdlSafe`, `PsCreateSystemThread`, `PoStartNextPowerIrp`/`PoCallDriver`, `ExpInterlocked*EntrySList` (self-contained pool — see §12), `ExAllocatePoolWithTag` (never `ExAllocatePool2`). Full audited import allowlist in `tools/verify_pe.py` (base = 75-function union in `tools/ref_imports.json` + ~60 justified additions). `src/driver/rda_util.c` documents the Win7 rationale for `ExAllocatePoolWithTag`.

---

## 2. Wire contract (the exact on-the-wire protocol)

Authoritative source: `src/shared/inc/rda_ioctl.h` (shared kernel↔user↔tests, frozen with `C_ASSERT` offset checks) + `docs/ground-truth-contracts.md` + driver dispatch in `src/driver/rda_ioctl.c`, `rda_create.c`, `rda_readwrite.c`, `rda_session.c`.

### 2.1 Device naming

- Control device: `\Device\<UPPERCASE(last component of the service RegistryPath)>` and `\DosDevices\<same>`; fallback literal `EPMNTDRV` when the registry path is unusable. Derivation scans the path backward for the last `\` and upcases via `RtlUpcaseUnicodeChar` — `src/driver/rda_control.c` `RdaDeriveBaseName()` lines 27–71, `RDA_FALLBACK_BASE_NAME L"EPMNTDRV"` in `src/driver/inc/rda_p.h` line 48.
- **Consequence: the SAME `rda.sys` installed under service name `eprdtdrv` serves `\Device\EPRDTDRV` (drop-in for the DRW user-mode stack); `ebrntdrv`/`epmdkdrv` serve EBRNTDRV/EPMDKDRV; `%s%d` numbered variants coexist for hot-upgrade.** (`src/driver/rda_control.c` header, `README.md` Quick facts, `docs/ground-truth-contracts.md` §1.1)
- Optional override: `Parameters\DeviceName` REG_SZ (EUEDKEPM parity) — takes precedence over service-name derivation (`src/driver/rda_control.c` lines 96–101, `src/driver/rda_registry.c`).
- User-mode open path: `\\.\{SYM}` where `{SYM}` = uppercase service name.

### 2.2 Open grammar (IRP_MJ_CREATE) — `src/shared/inc/rda_compat.h` `RdaClassifyOpenPath()`

| Path (relative to `\\.\{SYM}`) | Class | Semantics |
|---|---|---|
| *(empty name — Length < 2 bytes)* | `RDA_OPEN_CLASS_GLOBAL` (1) | SUCCESS with no target ("global" handle: identity IOCTLs + global-handle modes). Reference: EPMNtDrv `FUN_000110b0`. A **1-wchar** name is REJECTED (reference gate is `FileName.Length < 2` BYTES). |
| `\<PID-TOKEN>\Volume\<N>` | `RDA_OPEN_CLASS_VOLUME` (2) | `\Device\HarddiskVolume<N>` → top-of-stack target, mode 0. Volume-number **parse failure falls back to volume 0** (reference-exact). Volume gate is separator-inclusive: remainder (`Chars - sepIdx`) must be `> 8` wchars. |
| `\<PID-TOKEN>\<disk>\<part>` | `RDA_OPEN_CLASS_DISK` (3) | EPMNtDrv 3-segment: `\Device\Harddisk{d}\Partition0`, on failure retry `\Device\Harddisk{d}\DR{d}` (disk number passed as BOTH varargs — instruction-verified); walk down to first layer whose name contains `\Device\Harddisk`; mode = (ServiceKeyName != "Disk"). Part number is **vestigial** (never used). Disk bound `< MaxDisks` (default 200). |
| `\<PID-TOKEN>\<disk>` | `RDA_OPEN_CLASS_DISK_EPM` (4) | EUEDKEPM 2-segment: Partition0 only (no DR retry), walk to first layer with ServiceKeyName == "Disk", that layer IS the target. Disk bound `< 100`. |
| anything malformed | `RDA_OPEN_CLASS_REJECT` (0) | `STATUS_INVALID_PARAMETER` |

**PID token** (wire-compat handshake, NOT a security boundary — the DACL is): `b0..b3` = LE bytes of `PsGetCurrentProcessId()`; `token = b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24` with 8-bit wrapping adds. Driver re-derives at IRP time; mismatch → `STATUS_INVALID_PARAMETER`. Implemented as inline `RdaComputePidToken()` in `rda_compat.h` (client computes the same from its own PID). Dispatch in `src/driver/rda_create.c` `RdaDispatchCreateInner()` lines 43–121.

Context stored: `FileObject->FsContext` = referenced target FILE_OBJECT; `FsContext2` = `RDA_OPEN_CONTEXT` `{TargetFileObject, TargetDevice, Mode, Kind, SectorSize}` (`src/driver/inc/rda_p.h` lines 81–95). Released at **IRP_MJ_CLEANUP** (fix F9; reference released at CLOSE) — `src/driver/rda_create.c` `RdaDispatchCleanup()`.

### 2.3 Control IOCTLs — exact codes (`src/shared/inc/rda_ioctl.h` lines 48–76)

Device type `RDA_DEVICE_TYPE 0x600`; all METHOD_BUFFERED, FILE_ANY_ACCESS.

| IOCTL code | Name | CTL_CODE | Buffer contract |
|---|---|---|---|
| `0x60002400` | `IOCTL_RDA_GET_PROC_NAME` | `CTL_CODE(0x600, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)` | In: exactly 4 bytes = u32 PID (`RDA_IDENT_IN_LEN 4`). Out: ≥ `0x208` bytes (`RDA_IDENT_MIN_OUT_LEN`) = WCHAR image path + NUL. `Information = pathBytes + 2`. Global handles only. |
| `0x60002404` | `IOCTL_RDA_GET_FILE_OBJ_NAME` | `CTL_CODE(0x600, 0x901, …)` | In: 4 bytes = u32 HANDLE. Out: ≥ 0x208 = full NT path (device name + FileName) + NUL; `Information = bytes + 2`. Global handles only. |
| `0x60002408` | `IOCTL_RDA_GET_DISK_CONTEXT_MODE` | `CTL_CODE(0x600, 0x902, …)` | In: none. Out: **exactly 4 bytes** = `LONG Mode` (0 = disk.sys-backed, 1 = otherwise); `Information = 4`. Bound handles only. |
| `0x60002440` | `IOCTL_RDA_GET_INFO` (RDA extension) | `CTL_CODE_RDA(0x910)` | Out ≥ `sizeof(RDA_DRIVER_INFO)` (24 bytes): `{UINT32 StructSize, MajorVersion, MinorVersion, Flags, MaxDisks, IoctlBase}`. Flags: `RDA_INFO_FLAG_FILTER_MODE 0x1`, `RDA_INFO_FLAG_SESSIONS 0x2`. Global handles. |
| `0x60002444` | `IOCTL_RDA_OPEN_TARGET` (RDA extension) | `CTL_CODE_RDA(0x911)` | In: `RDA_OPEN_TARGET_IN` (24 bytes): `{StructSize, Kind(1=VOLUME/2=DISK/3=DISK_PART), DiskNumber, PartNumber, VolumeNumber, Reserved}`; Out must be 0. Binds FsContext2 without the path grammar. Handle must not already be bound (`STATUS_INVALID_DEVICE_STATE`). |
| `0x60002448` | `IOCTL_RDA_GET_VERSION` (RDA extension) | `CTL_CODE_RDA(0x912)` | Out ≥ 8 bytes: `UINT64 = (Major << 32) | Minor` = `0x0000000100000000`. Global handles. |

**Dispatch routing** (`src/driver/rda_ioctl.c` `RdaDispatchDeviceControlInner`, lines 247–286):
1. If `SessionsEnabled` (default 1) and `(code − IoctlBase)` matches a session code → session engine (see §2.5). Served on ANY handle.
2. Else if handle is target-bound: `0x60002408` handled locally; **everything else (incl. 0x60002400/04) passes through** to the target's top-of-stack via `RdaPassThroughIoctl` (method-aware, fix F10: BUFFERED → SystemBuffer both ways; IN/OUT_DIRECT → out = `Irp->UserBuffer`; METHOD_NEITHER → `STATUS_NOT_SUPPORTED`).
3. Else (global handle): the 5 codes above; anything else → `STATUS_INVALID_PARAMETER`.

### 2.4 READ/WRITE contract (`src/driver/rda_readwrite.c`, `rda_ioctl.h` lines 129–143)

- Control DO is `DO_DIRECT_IO` → caller's buffer arrives as MDL; mapped with `MmGetSystemAddressForMdlSafe`.
- `Length` is **BYTES**; `ByteOffset` is absolute BYTES on the target; negative offset → `STATUS_INVALID_PARAMETER`; zero length → `STATUS_INVALID_PARAMETER` (documented divergence S10 — the reference crashed on a NULL MDL there).
- Forwarded via driver-built async FSD IRP to the resolved target; cancellable (fix F4: cancel routine flags + wakes, dispatcher propagates `IoCancelIrp` into the storage stack; single-completer invariant; SLIST-pooled transfer contexts).
- **On success `IoStatus.Information = Length × 512`** (`RDA_LEGACY_INFORMATION_SCALE 512`) — the legacy scaled convention, intentionally preserved (F11); legacy clients divide by 512. Clients verify this exact scaling (`src/bench/rdabench.c` `UnscaleInformation`, `BenchInfo`).
- Global-handle legacy modes (reference parity, kept): WRITE with no target and `Length == 4` parks the u32 handle into the driver global slot `g_RdaGlobalHandle` (`volatile LONG64`); READ with no target, slot non-zero, length `> 0x207` resolves the parked handle's object name into the buffer (GetDriverInfo handshake). `src/driver/rda_readwrite.c` lines 317–340. Note: `docs/ground-truth-contracts.md` §7 says the park mode is "additionally gated by SeAdminCheck", but the implemented code relies on the device DACL only (no SeAdminCheck call exists in `src/` — verified by grep; treat the DACL as the gate).
- Failure traces keep reference strings: `[epmntdrvDispatchWrite] Write error 0x%x,%u,%I64u,0x%x`, `[GetVolumeDeviceObejct] %ws open error 0x%x,that's it!` (vendor typo preserved).

### 2.5 Copy-session protocol (EUDCPEPM merge) — `src/driver/rda_session.c`

Code normalization: `custom = code − IoctlBase` where `IoctlBase` is the service-key DWORD (default 0; the WinPE reference deployment sets `0x10000`). Served when `Parameters\SessionsEnabled` ≠ 0 (default 1). Full verified command set — **19 codes** (G28 widened the earlier 15) (`rda_ioctl.h` lines 180–198):

| custom code | name | in-gate | out-gate | Semantics (driver implementation) |
|---|---|---|---|---|
| `0x800C2AC0` | DISCOVERY | — | ≥ 4 | **Filter stacks only**: answers bound diskId, `Information = 4`, re-probes sector size. On the control device: falls through to plain SUCCESS (reference `LAB_000129f9`). |
| `0x800C2AC8` | GET_VERSION | ≥ 0x84 | — | Writes `0x00010100` (`RDA_SESSION_PROTO_VERSION`) then ASCII product string `"RDA Disk Copy Driver (c) RDA contributors"` (≤ 0x2E bytes, `RDA_SESSION_PRODUCT_STRING_MAX`) at +4; `Information = 0`. |
| `0x800C2AD0` | OPEN_SESSION | ≥ 0x840 | — | Creates a session from the 0x840 wire struct (below). Overlap with an active session on the same disk → `STATUS_SHARING_VIOLATION` (`0xC0000043`). Writebacks: handle @ +0x834, effective spb @ +0x30, sector size @ +0x83C; `Information = 0`. |
| `0x800C2AD8` | SESSION_OP | ≥ 8 | — | Flush the protected file's volume: path = NUL-terminated WCHAR string at the head of the session's 0x800 bitmap buffer; `ZwCreateFile` + `IRP_MJ_FLUSH_BUFFERS` ≤ 20 retries (tolerates PENDING/`0xC0000010`/`0xC00000A2`); state 0→1→3; always returns STATUS_SUCCESS (0 even on open-fail). |
| `0x800C2AE0` | SESSION_OP2 (StartBackup) | ≥ 0x28 | — | Requires state == 3 (else `STATUS_UNSUCCESSFUL` `0xC0000001`); locks caller pump buffer via MDL (`MmProbeAndLockPages(UserMode, IoWriteAccess)`); validates `PumpBytes % (spb*sectorSize) == 0` and user-bitmap spb match; builds plan {start, blocks, skip bits}; state → 4. |
| `0x800C2AE8` | GETNEXTBLOCK | ≥ 0x1E | ≥ 0x1E | Async pump: IRP queued `STATUS_PENDING`; worker copies `min(MaxSec, pump capacity, plan remaining)` sectors **into the StartBackup-registered pump buffer** (NOT the IRP buffer), COW-serving externally-written ranges; completes IRP with the 0x1E record; **`Information = 0x1E` constant**. No plan → returns literal `0xC0000120` with progress fields filled. Queue cap 2000 nodes → `STATUS_NO_MEMORY`. |
| `0x800C2AF0` | SESSION_STATUS | ≥ 8 | — | Returns the block-map status **AS the NTSTATUS** (no buffer write). |
| `0x800C2AF8` | GET_PROGRESS (finalize) | ≥ 0xC | — | FINALIZE: state → 5, kick + drain queued IRPs (with `0xC0000120`), free plan + pump MDL + COW; percent (u32) at `SystemBuffer + 8`; `Information = 0`. |
| `0x800C2B00` | CLOSE_SESSION | ≥ 8 | — | Teardown; invalid handle → `STATUS_INVALID_HANDLE` (`0xC0000008`). |
| `0x800C2B0A` | DIRECT_READ | — | > 0x10 | METHOD_OUT_DIRECT: header+data in the MDL'd OUTPUT buffer `{u32 diskId@0, u32 sectorCount@+4, u64 LBA@+8, data@+0x10}`; bound = `sectorSize*count + 0x10 ≤ outLen`; **`Information = OutputBufferLength` always**. Session-free (diskId from header). |
| `0x800C2B12` | DIRECT_WRITE | — | > 0x10 | Twin of 0x800C2B0A, write direction. |
| `0x800C2B18` | MODE4 | ≥ 0x1C | ≥ 4 | Raw paging-IO file WRITE by handle (`RDA_FILEIO_IN`); sets `IRP_PAGING_IO` + caller's FileObject; `Information = 8` on success. |
| `0x800C2B20` | MODE3 | ≥ 0x1C | ≥ 4 | Twin: raw paging-IO file READ. |
| `0x800C2B2A` | SIZED_XFER | — | > 0x10 | Same shape as DIRECT_READ; sector size from per-disk cache. |
| `0x800C2B2E` | SIZED_XFER2 | — | > 0x10 | Same, write direction. |
| `0x800C2B38` | CANCEL | ≥ 8 | — | State := 3 (standby) under spinlock + kick; SUCCESS. |
| `0x800C2B3E` | RRWR raw disk WRITE | ≥ 0x1C | — | **Filter stacks only** ("RRWR" magic); control device → plain SUCCESS. |
| `0x800C2B40` | RRWR version probe | ≥ 4 | — | Filter only: `0x604` at buffer+8, `Information = 0xC`. |
| `0x800C2B42` | RRWR raw disk READ | ≥ 0x1C | — | Filter only. RRWR wire: `{magic 0x52455752 @0 ("RRWR"), sectorSize @+8 (must match disk), outLen @+0xC (must equal actual), LBA @+0x10, count @+0x18, data = whole MDL buffer}` (`RDA_RRWR_MAGIC 0x52455752`). |

**RRWR + DISCOVERY are served on the filter stacks** (`src/driver/rda_filter.c` `RdaFilterIoctl`, lines 185–311); on the control device these codes complete SUCCESS without effect (reference fall-through) — `rda_session.c` lines 1410–1422.

### 2.5.1 Exact wire struct layouts (packed, C_ASSERT-frozen)

`RDA_OPEN_SESSION` — overlaid in/out, **0x840 bytes total**, `#pragma pack(4)` (`rda_ioctl.h` lines 309–342):

```
+0x000  UINT32 DiskId
+0x004  UINT64 DataFirstSec      /* copy range start sector */
+0x00C  UINT64 TotalSecs         /* must be nonzero */
+0x014  UINT64 ExtentParam1
+0x01C  UINT64 ExtentParam2
+0x024  UINT64 BitmapSize        /* must be nonzero */
+0x02C  UINT32 BitmapCount       /* must be nonzero */
+0x030  UINT32 SpbInOut         /* IN 0 => driver picks (tSecs<0x20 ? 1 : 0x10), tail = tSecs&0xF
                                    OUT: overwritten with the effective spb (reference ctor writes
                                    back at +0x30; +0x838 is unused padding in the reference) */
+0x034  UINT8  BlockBitmap[0x800] /* bit per copy block; NUL pad at 0x7FE */
+0x834  UINT64 SessionHandle     /* OUT: opaque 64-bit handle (RDA: table ID, never a pointer) */
+0x83C  UINT32 SectorSize        /* OUT */
```

`RDA_GETNEXTBLOCK` — 0x1E bytes, `pack(1)`: `{UINT64 Handle@0, UINT64 CursorSec@8, UINT32 MaxSec@0x10, UINT32 DiskId@0x14(out), UINT32 Progress@0x18(out), UINT8 MapProgress@0x1C(out), UINT8 MapOk@0x1D(out)}`.

`RDA_STARTBACKUP_IN` — 0x28 bytes, `pack(4)`: `{UINT64 Handle@0, PVOID PumpBuffer@8, UINT32 PumpBytes@0x10, PVOID UserBitmap@0x14, UINT32 StartSec@0x1C, UINT32 BlockCount@0x20, UINT32 Spb@0x24}`.

`RDA_FILEIO_IN` — 0x1C bytes, `pack(4)`: `{HANDLE FileHandle@0, PVOID Buffer@8, UINT64 ByteOffset@0x10, UINT32 Length@0x18}`.

### 2.5.2 Session engine protocol (how a client drives a copy session)

Implemented lifecycle (verified in `src/driver/rda_session.c` + demonstrated by `src/client/rdacli.c` `CmdSession`):
1. Open the control device **bare** (`\\.\{SYM}`) with GENERIC_READ|GENERIC_WRITE → global handle.
2. `OPEN_SESSION (0x800C2AD0 + IoctlBase)` with the 0x840 struct (DiskId, DataFirstSec, TotalSecs, BitmapSize=0x800, BitmapCount, BlockBitmap bits) → returns `SessionHandle` @ +0x834, effective spb @ +0x30, sector size @ +0x83C. Sessions are **owned by the creating process** and swept at control-device IRP_MJ_CLEANUP/CLOSE (`RdaSessionSweepByProcess`); filter REMOVE kills sessions bound to the removed disk.
3. Optionally `SESSION_OP (0x800C2AD8)` flush of the protected file whose path heads the session bitmap buffer → state 3 (standby).
4. `SESSION_OP2 (0x800C2AE0)` StartBackup with the pump buffer (user VA, locked by the driver with an MDL for the session's lifetime) → state 4 (copying).
5. Repeated `GETNEXTBLOCK (0x800C2AE8)` — each IRP pends; the worker copies sectors into the pump buffer and completes with `Information = 0x1E` and `{DiskId, Progress%, MapProgress, MapOk}` in the 0x1E record.
6. `GET_PROGRESS (0x800C2AF8)` finalize → percent @ buffer+8, state 5, pump MDL unlocked/freed.
7. `CLOSE_SESSION (0x800C2B00)` or `CANCEL (0x800C2B38)` (state := 3, not an abort).
State machine: `0 created → 1 flushing → 3 standby → 4 copying → 5 done` (`RDA_SESSION_STATE_*`, `rda_ioctl.h` lines 215–222). Session table: 64 slots, generation-tagged opaque handles `((index+1) << 16) | generation` (F12: never a kernel pointer). Kick budget 301 (`RDA_SESSION_KICK_CAP`, reference 0x12D). COW cache capped 64 MiB/session (`RDA_SESSION_COW_MAX_BYTES`).

### 2.6 Error codes the driver returns (exact NTSTATUS map)

From the driver sources: `STATUS_INVALID_PARAMETER` (0xC000000D — bad grammar/token/offset/params), `STATUS_INSUFFICIENT_RESOURCES` (0xC0000017 — alloc/MDL failures; COW cap), `STATUS_SHARING_VIOLATION` (0xC0000043 — overlapping session ranges), `STATUS_INVALID_HANDLE` (0xC0000008 — CLOSE/PROGRESS on a bad handle), `STATUS_INVALID_DEVICE_STATE` (double-bind in OPEN_TARGET), `STATUS_UNSUCCESSFUL` (0xC0000001 — StartBackup outside state 3), `STATUS_BUFFER_TOO_SMALL` (short in/out gates), `STATUS_NOT_SUPPORTED` (METHOD_NEITHER pass-through), `STATUS_INVALID_DEVICE_REQUEST` (unowned majors, filter extension mismatch), `STATUS_NO_MEMORY` (pump queue cap 2000), `STATUS_CANCELLED`, `STATUS_PENDING` (0x103, GETNEXTBLOCK + parked writes), literal `0xC0000120` (GETNEXTBLOCK with no active plan — reference literal, no friendly name), `STATUS_DELETE_PENDING` (parked writes drained at teardown), `STATUS_OBJECT_PATH_NOT_FOUND`/`STATUS_OBJECT_NAME_NOT_FOUND` (registry/flush-file paths). No `DbgBreakPoint`/`KeBugCheckEx` on request paths (F7/F15).

### 2.7 Access model

- Device DACL (fix F2): `IoCreateDeviceSecure(DriverObject, 0, &g_RdaDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, sddl, &RDA_DEVICE_CLASS_GUID, &g_RdaControlDevice)` with default SDDL **`D:P(A;;GA;;;BA)(A;;GA;;;SY)`** (Administrators + SYSTEM full access only), overridable via `Parameters\Sddl` (`RDA_DEFAULT_SDDL` in `src/driver/inc/rda_p.h` line 62; applied in `src/driver/rda_control.c` lines 141–153). Class GUID `{22503FD1-90D4-4322-B62D-9C8FFE186E33}` lets admins tighten via the class key. **Standard users cannot open the device at all.**
- PID token retained for wire compat only ("a calculator-class process could compute its own token; the DACL is what stops it" — `docs/audit/2026-10-22-security-review.md`).

---

## 3. Artifacts produced

Build outputs (per `docs/build.md` + project files; `OutDir = $(SolutionDir)bin\$(Platform)\$(Configuration)\`):

| Artifact | Path | Notes |
|---|---|---|
| **`rda.sys`** | `bin\x64\Release\rda.sys` (and `bin\x64\Debug\rda.sys`) | The driver. **Unsigned by design** (see §7). Post-build: `python tools\stamp_osversion.py` (stamps PE OS fields 6.1/6.01 NATIVE + recomputes checksum) then `python tools\verify_pe.py` (import allowlist + header contract). |
| **`rdacli.exe`** | `bin\x64\Release\rdacli.exe` | User-mode client/contract harness (see §10). |
| **`rdabench.exe`** | `bin\x64\Release\rdabench.exe` | JSON-emitting benchmark/fuzz/verify client (built by the solution; **not** staged into CI/release artifacts — used by `benchmark.yml` in-place). |
| **`rda.inf`** | shipped **verbatim** from `src\driver\rda.inf` — "no INF tooling in build; shipped verbatim" (worklog.md; `rda.vcxproj` keeps it out of the `Inf` item group as `<None>`). Declares `CatalogFile = rda.cat` but **no .cat is ever generated** — no Inf2Cat/stampinf step exists. |
| `rda.cat` | **NOT produced** | The INF references it, but the build has no catalog step; the release bundle ships unsigned rda.sys + INF. |

Variants: **x64 only**; Debug and Release; no ARM64, no test-signed release artifact (CI *asserts* the artifact is unsigned — `ci.yml` "Verify unsigned artifact (no accidental test-signing)" reads the certificate-table directory RVA at `lfanew + 0x18 + 112 + 32` and fails if ≠ 0). Test-signing happens only inside the benchmark workflow (job-local cert, session-scoped).

**CI artifact** (`ci.yml` build job, per matrix Debug/Release): `rda-{wdk26100}-{Configuration}` containing exactly `rda.sys`, `rdacli.exe`, `rda.inf`.

**Release bundle** (`release.yml`, tags `v*` / manual dispatch; `dist\`):
- `rda.sys`, `rdacli.exe`, `rda.inf`
- `scripts\install.ps1`, `scripts\uninstall.ps1`, `scripts\run-integration.ps1`
- `LICENSE`, `README.md`, `docs\signing.md`, `docs\testing.md`, `docs\win7-compat.md`
- `SHA256SUMS` (SHA-256 over every file, generated via `Get-FileHash`)
- Uploaded as artifact `rda-release-<ref_name>`; on `v*` tags also attached to a GitHub release via `softprops/action-gh-release@v2` with body text: "Unsigned build. Windows kernel-mode code signing applies before production deployment …".

---

## 4. Registry requirements

The driver itself only **reads** registry (once, at DriverEntry — `src/driver/rda_registry.c`; it never writes). Install scripts and INF write it.

### 4.1 Values read by `RdaReadParameters()` (`src/driver/rda_registry.c` lines 90–184)

Reads the **service key root** first (reference parity — G10), then the `Parameters` subkey as override:

| Key (relative to `HKLM\SYSTEM\CurrentControlSet\Services\<ServiceName>`) | Type | Default | Meaning |
|---|---|---|---|
| `IoctlBase` | DWORD | 0 | Session-protocol code base (`custom = code − IoctlBase`). WinPE reference deployment sets `0x10000`. |
| `DeviceName` | REG_SZ | (absent) | Overrides the `\Device`/`\DosDevices` base name (EUEDKEPM parity). Both root and Parameters. |
| `Parameters\IoctlBase` | DWORD | (as above) | Override. |
| `Parameters\MaxDisks` | DWORD | 200 | Open-grammar disk bound (clamped 2..1024). |
| `Parameters\DebugLevel` | DWORD | 1 (errors only) | 0..5 trace verbosity (`g_RdaDebugLevel`, DbgPrintEx). |
| `Parameters\FilterMode` | DWORD | 0 | 1 = PnP disk-class filter role (AddDevice attach). |
| `Parameters\VolSnapAttach` | DWORD | 0 | 1 = also attach VolSnap devices (PE parity; currently trace-only in `rda_filter.c`). |
| `Parameters\SessionsEnabled` | DWORD | 1 | Serve the copy-session protocol. |
| `Parameters\DeviceName` | REG_SZ | (absent) | Override (takes precedence over root value). |
| `Parameters\Sddl` | REG_SZ | `D:P(A;;GA;;;BA)(A;;GA;;;SY)` | Device DACL override. |

Missing/malformed values keep defaults (reference behavior). Registry-read failure at DriverEntry is **non-fatal** except a NULL RegistryPath.

### 4.2 Values written by `src/driver/rda.inf`

`[Rda.Service]`: `DisplayName/Description = "RDA Raw Disk Access Driver"`, `ServiceType = 1` (SERVICE_KERNEL_DRIVER), `StartType = 3` (SERVICE_DEMAND_START), `ErrorControl = 1` (SERVICE_ERROR_NORMAL), `ServiceBinary = %12%\rda.sys` (i.e. `%windir%\system32\drivers\rda.sys`); `Class = System`, `ClassGuid = {4D36E97D-E325-11CE-BFC1-08002BE10318}`, `CatalogFile = rda.cat`, `DriverVer = 01/01/2026,1.0.0.0`, `PnpLockPolicy = 1`. `[Rda.Parameters]` (HKR = service key; also `DelReg`'d at uninstall): `HKR,"Parameters","IoctlBase",0x00010001,0x00000000`, `HKR,"Parameters","MaxDisks",0x00010001,0x000000C8` (200), `HKR,"Parameters","DebugLevel",0x00010001,0x00000001`, `HKR,"Parameters","SessionsEnabled",0x00010001,0x00000001`. Note: the INF installs the service under the name the INF is installed with (the file is `rda.inf` → service name `rda` → device `\Device\RDA`).

### 4.3 Values written by `scripts/install.ps1` (per §5): `Parameters\{IoctlBase, MaxDisks, DebugLevel, SessionsEnabled}` under `HKLM:\SYSTEM\CurrentControlSet\Services\<ServiceName>\Parameters`.

### 4.4 WinPE manual registration — `scripts/winpe/regdrv-rda.reg` (EVERY value)

Merged into the offline PE SOFTWARE/SYSTEM hive (or running PE) before launching the recovery host:

```reg
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\rda]
"Type"=dword:00000001          ; SERVICE_KERNEL_DRIVER
"Start"=dword:00000003         ; SERVICE_DEMAND_START
"ErrorControl"=dword:00000001  ; SERVICE_ERROR_NORMAL
"Group"="Boot File System"
"ImagePath"="\\SystemRoot\\System32\\drivers\\rda.sys"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\rda\Parameters]
"IoctlBase"=dword:00010000    ; session codes shift to 0x800C2AC0+0x10000 wire values
"MaxDisks"=dword:000000c8      ; 200
"DebugLevel"=dword:00000001
"SessionsEnabled"=dword:00000001
"FilterMode"=dword:00000001    ; activates the EUDCPEPM-style copy-filter role
"VolSnapAttach"=dword:00000000

; Disk-class upper filter (GUID_DEVCLASS_DISKDRIVE) — the documented mechanism
; replacing EUDCPEPM's AddDevice patching (fix F6):
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}]
"UpperFilters"=hex(7):r,d,a,\,0   ; REG_MULTI_SZ "rda"
```

Note the PE profile differs from the live-OS profile in exactly three ways: service name `rda`, `IoctlBase=0x10000`, and `FilterMode=1` + the disk-class `UpperFilters` REG_MULTI_SZ value `"rda"`. Comment in the file: "FilterMode=1 + the UpperFilters value activate the EUDCPEPM-style copy-filter role on every disk device in the PE."

### 4.5 No HKCU, no Run keys, no other registry writes exist anywhere in the repo.

---

## 5. Service registration — exact mechanics

### 5.1 `scripts/install.ps1` step by step (elevated)

Params: `-ServiceName` (mandatory, `^[A-Za-z0-9_]+$`), `-Source` (mandatory, must exist; typically `.\bin\x64\Release\rda.sys`), `-IoctlBase` (default 0), `-MaxDisks` (default 200), `-DebugLevel` (default 1), `-NoStart` (switch).

1. Elevation guard: `WindowsPrincipal(IsInRole(Administrator))` → `Write-Error "This script must run elevated (Administrator)."`
2. Preflight: `Get-Service -Name $ServiceName` — **fails if the service already exists** ("Remove it first (uninstall.ps1) or pick another name.").
3. Copy: `Copy-Item $sourcePath "$env:SystemRoot\System32\drivers\$ServiceName.sys" -Force` — **the driver file is RENAMED to the service name** (`$destDir = Join-Path $env:SystemRoot 'System32\drivers'`; `$destPath = "$ServiceName.sys"`).
4. Create service (exact command):
   ```powershell
   sc.exe create $ServiceName binPath= "$destPath" type= kernel start= demand error= normal group= "Boot File System"
   ```
   (binPath is the **absolute** `C:\Windows\System32\drivers\<ServiceName>.sys`; load group `"Boot File System"`; no dependencies set.)
5. Write `Parameters` values: `New-Item HKLM:\SYSTEM\CurrentControlSet\Services\<SN>\Parameters -Force`; `Set-ItemProperty` for `IoctlBase`, `MaxDisks`, `DebugLevel` (from params, DWord) and `SessionsEnabled = 1`.
6. Start unless `-NoStart`: `sc.exe start $ServiceName`; on failure prints: "StartService failed ($LASTEXITCODE). Check code-signing policy (testsigning), driver signature, and the System event log." (warning only, not fatal).
7. Verify/echo: `"Expected control device: \Device\<UPPER(ServiceName)>"`, `"User-mode path: \\.<UPPER(ServiceName)>\<pid-token>\<disk>\<part>"`, `"Next: .\bin\x64\Release\rdacli.exe grammar 0"`.

### 5.2 `scripts/uninstall.ps1` step by step (elevated)

Params: `-ServiceName` (mandatory). Steps: elevation guard → `Get-Service`; if running: `sc.exe stop $ServiceName` + `Start-Sleep -Seconds 1` → `sc.exe delete $ServiceName` → delete `%SystemRoot%\System32\drivers\<ServiceName>.sys` (`Remove-Item -Force`). Warns (does not fail) if the service is absent.

### 5.3 vs. plain `sc.exe create`

Functionally the same SCM primitive (`type= kernel start= demand error= normal`); the script adds: existence preflight, the file copy+rename to the service name, the `Parameters` values (which pure sc.exe does not write), the load **group** (`"Boot File System"` — sc.exe default is no group), start, and expected-device echo. **The benchmark workflow uses a different, thinner form** (`.github/workflows/benchmark.yml` lines 165/171): `sc.exe create <name> type= kernel start= demand binPath= "System32\drivers\<file>.sys" group= "FSFilter Boot File System"` — note the **relative binPath** and the **different group name** `"FSFilter Boot File System"` vs install.ps1's `"Boot File System"`. Both group names appear in the repo; if the integration plan normalizes one, `"Boot File System"` is what the shipped install.ps1 + regdrv-rda.reg use. The INF sets no group.

### 5.4 Service identity rules (what the driver does with it)

DriverEntry derives the device name from the SERVICE key path (not the file name); the service `DisplayName` is unused. So: `-ServiceName eprdtdrv` → `\Device\EPRDTDRV` (DRW drop-in); `ebrntdrv`/`epmdkdrv` → PE spares; `rda` → `\Device\RDA` (INF/PE profile). Commit `da7efb9` (HEAD) specifically fixed an `sc.exe.exe` typo in this pair.

---

## 6. Privilege requirements

| Operation | Privilege | Evidence |
|---|---|---|
| Install / uninstall service | **Administrator** (enforced: both scripts `Write-Error` if not elevated) | `scripts/install.ps1` lines 37–41; `scripts/uninstall.ps1` lines 15–17 |
| Load (`sc.exe start`) | Administrator + **signature policy**: test-signed builds need `bcdedit /set testsigning on` + reboot, or a provisioned test cert; properly signed builds need a Microsoft-accepted signature (see §7) | `README.md` "Install"; `docs/testing.md` Layer 3 |
| Open the control device (any mode) | **Administrators or SYSTEM only** (default SDDL `D:P(A;;GA;;;BA)(A;;GA;;;SY)`; overridable via `Parameters\Sddl`) | `src/driver/inc/rda_p.h` line 62; `src/driver/rda_control.c` lines 141–153 |
| Read/write sectors, identity IOCTLs, session engine, pass-through | No *additional* check beyond the device open — everything sits behind the DACL | `docs/audit/2026-10-22-security-review.md` "Access control posture" |
| PID-token | Not a privilege check; a client-binding handshake (must embed `RdaComputePidToken(GetCurrentProcessId())` as the first path segment for target opens) | `rda_ioctl.h` lines 109–126 |
| Bare (`\\.\{SYM}`) opens | Same DACL; succeed with no target (global handle) | `rda_create.c` `RdaDispatchCreateInner` |

WinPE note: "For WinPE deployments (the copy-engine profile), signature enforcement is relaxed — OSSign-signed (or even test-signed) binaries are operational immediately" (`docs/signing.md` §3 "Honest scope note").

---

## 7. Signing posture

- **The build never signs.** `SignMode=Off` in `rda.vcxproj` (both configs). CI has a dedicated step **asserting the artifact is unsigned** (certificate-table RVA must be 0) — `ci.yml` "Verify unsigned artifact (no accidental test-signing)"; same in `release.yml`. Reasons documented in `docs/signing.md` §2: reproducible artifacts; signing belongs to the release pipeline; no test-signing noise in the PE gate.
- **Current state: `rda.sys` ships unsigned.** The release bundle and GitHub releases carry an explicit "Unsigned build" body. It will NOT load on stock Win10/11 (or any Secure-Boot-enabled OS) without signing.
- **Test signing (expected flow)**: `bcdedit /set testsigning on` + reboot, or VS-provisioned test certificate (`README.md` Install; `docs/testing.md`). The **benchmark workflow's proven no-reboot alternative** (hosted runners, SecureBoot off): `New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=RDA-BENCH-TEST" …`, export `.cer`+`.pfx` (password `rda-bench`), `Import-Certificate` into `Cert:\LocalMachine\Root` AND `Cert:\LocalMachine\TrustedPublisher`, then `signtool sign /fd SHA256 /f rda-test.pfx /p rda-bench <rda.sys>` and `signtool verify /pa` (`benchmark.yml` lines 71–90). This is the model a CI/installer can replicate on test machines.
- **OS acceptance matrix** (`docs/signing.md` §1): Win10 1607+ (client, Secure Boot) = Hardware Dev Center dashboard-signed only (attestation with EV cert, or HLK); Win8/8.1 = HCK/HLK or SHA-256 Authenticode cross-signed; Win7 SP1 = HCK or SHA-1 cross-sign (SHA-256 needs KB4474419); Win7 unpatched = SHA-1 only. Cross-signing program retired (2021+) — "new Win7 retail distribution is effectively frozen".
- **OSSign pipeline: prepared but DISABLED.** `signing/ossign/review.yml` (gitleaks 8.21.2 + osv-scanner 1.9.1 on the pinned SHA, reports uploaded with OIDC to `$OSSIGN_SERVER/api/v1/reviews/<id>/uploads`) and `signing/ossign/sign.yml` (pinned-SHA rebuild: `nuget restore` → `msbuild … -p:Configuration=Release -p:Platform=x64` → `python src\tools\verify_pe.py` **before signing** → `ossign/ossign@1.3.1` pecoff signing of `rda.sys` and `rdacli.exe` → `ossign verify` → fetch review report → publish release). Activation requires: OSSign onboarding, mirror repo variables `SOURCE_REPO`/`OSSIGN_SERVER`/`OSSIGN_KEY`, protected environment `signing` on the mirror. **Honest scope** (`docs/signing.md`): an OSSign Authenticode signature is NOT a Microsoft kernel-mode signature — loads on Win7/8/8.1 (cross-sign-equivalent) and WinPE and test-signed Win10/11, but NOT retail Win10/11. Recommended onboarding order: OSSign first, Hardware Dev Center (EV) in parallel for Win10/11 retail, optional HLK.
- `rda.inf` declares `CatalogFile = rda.cat` but no catalog is built anywhere (no Inf2Cat step; INF ships verbatim). A combined installer must either add a catalog step or rely on the signing pipeline covering the .sys.

---

## 8. Lifecycle (as the repo's own docs/scripts describe it)

- **Install (test environments only, live-OS profile)** — `README.md`:
  ```powershell
  # elevated; installs rda.sys as service 'eprdtdrv' → device \Device\EPRDTDRV
  .\scripts\install.ps1 -ServiceName eprdtdrv -Source .\bin\x64\Release\rda.sys
  ```
  Alternative: `rda.inf` `DefaultInstall` (CopyFiles + Service sections; service name = INF name `rda`). Prereq: test-signing enabled or a signed binary.
- **Load**: demand start — `sc.exe start <ServiceName>` (install.ps1 does it unless `-NoStart`). DriverEntry: read Parameters → create control device + symlink → install master dispatch on all 28 majors → init session table + disk cache → if `FilterMode ≠ 0`, register `AddDevice` (filter role). No auto-start anywhere (StartType 3 in INF and reg file).
- **Filter-role activation** (WinPE profile): merge `scripts/winpe/regdrv-rda.reg` (service + `FilterMode=1` + disk-class `UpperFilters="rda"`); every disk stack gets a filter DO at AddDevice; REMOVE destroys disk-bound sessions and detaches.
- **Unload**: `sc.exe stop <ServiceName>` → `RdaDriverUnload` tears down in reverse order: `RdaSessionShutdown()` (destroy all active sessions) → `RdaDiskCacheShutdown()` → `RdaReadWriteShutdown()` (drain xfer pool) → `RdaFilterStop()` → `RdaDeleteControlDevice()` (delete symlink, free name buffers, `IoDeleteDevice`). Reference trace string: `"[EPMNtDrv]DriverUnload called"`. (`src/driver/rda_driver.c` lines 33–48.)
- **Uninstall**: `.\scripts\uninstall.ps1 -ServiceName <name>` (stop → sleep 1s → `sc.exe delete` → remove `%SystemRoot%\System32\drivers\<name>.sys`). INF alternative: `DefaultUninstall` (DelFiles + ServiceUninstall + `DelReg Rda.Parameters`).
- **Update / hot-upgrade**: not scripted; supported implicitly — "install the *same binary* under any service name and it answers that name's identity … `%s%d` hot-upgrade variants coexist without conflicts" (`README.md`, `docs/architecture.md` §2). i.e., run the new build as service `eprdtdrv2` → `\Device\EPRDTDRV2` side by side, switch clients, remove the old service. No reboot required for the raw-access role (the disk-filter role requires PnP re-enumeration/reboot in PE).
- **Integration test run** (post-install, elevated): `.\scripts\run-integration.ps1 -Client .\bin\x64\Release\rdacli.exe -Disk 0` → cases `info`, `grammar <disk>`, `ident <pid>`, `read <disk> 0 512`; non-zero exit on failure.
- **Recommended host matrix** (`docs/testing.md`): Win11+Hyper-V testsigning (primary), Win10 22H2 + Driver Verifier (special pool/IRQL/deadlock/force-pending), Win7 SP1 x64 VM (validates the PE contract end-to-end), WinPE (filter-role attach).

---

## 9. Build system

- **Solution**: `rda.sln` — 3 projects, Debug|x64 + Release|x64 only: `rda` (`src\driver\rda.vcxproj`, GUID {8E4B7C2A-…}), `rdacli` (`src\client\rdacli.vcxproj`), `rdabench` (`src\bench\rdabench.vcxproj`).
- **WDK via NuGet (pinned)**: `nuget restore .\packages.config -PackagesDirectory .\packages\` restores `Microsoft.Windows.SDK.CPP`/`SDK.CPP.x64`/`WDK.x64` all at **10.0.26100.6584**; `Directory.Build.props` imports the package `.props` for `Platform == x64`, redirecting `WindowsSdkDir`/`WDKContentRoot` into the package; `rda.vcxproj` pins `WindowsTargetPlatformVersion=10.0.26100.0` so stampinf-class tools resolve inside the package (worklog: "TRK0005" hazard). VS component provides toolset plumbing only. **No EWDK ISO required**; EWDK alternative documented in `docs/build.md` for air-gapped labs (mount ISO, `LaunchBuildEnv.cmd`, same project files).
- **Build command** (identical in CI, release, benchmark, and OSSign sign.yml):
  ```powershell
  nuget restore .\packages.config -PackagesDirectory .\packages\
  msbuild .\rda.sln -m -v:minimal -nologo -p:Configuration=Release -p:Platform=x64
  ```
- **Driver project specifics** (`src/driver/rda.vcxproj`): PlatformToolset `WindowsKernelModeDriver10.0`; `TargetVersion Windows10` (only modern WDK target); `NTDDI_VERSION=0x06010000;_WIN32_WINNT=0x0601;WINVER=0x0601` (Win7 header gating); C17; `/W4 /WX` + `/external:W0`; `DisableSpecificWarnings 4391` (wdm.h interlocked prototype mismatch — the only suppression); `BufferSecurityCheck true` (/GS), `ControlFlowGuard false`; linker `/osversion:6.1 /subsystem:native`; `wdmsec.lib` (IoCreateDeviceSecure); `SignMode=Off`.
- **Post-build, always**: `python tools\stamp_osversion.py <rda.sys>` — the modern WDK linker stamps OS (10,0) regardless of flags, so this tool rewrites PE optional header: MajorOperatingSystemVersion/Minor = 6.1, MajorSubsystemVersion/Minor = 6.01, Subsystem = NATIVE(1), recomputes the PE checksum; then `python tools\verify_pe.py <rda.sys>` enforces machine 0x8664 (AMD64), OS (6,1), subsystem NATIVE(1), imports ⊆ `tools/ref_imports.json` "union" (75 Win7-SP1 ntoskrnl exports from the six reference binaries) ∪ ~60 audited additions (each with a stated reason, incl. the wdmsec static-link closure and the `ExpInterlocked*EntrySList` pool), import DLLs ⊆ {ntoskrnl.exe, hal.dll, bootvid.dll, kdcom.dll, wmilib.sys}.
- **CI jobs** (`ci.yml`): `lint` (ubuntu: clang-format `--dry-run --Werror`, markdown trailing-newline check, gcc unit tests via `scripts/run-unit-tests.sh`), `fuzz` (ubuntu: libFuzzer 60 s round + gcc selftest 4019 adversarial inputs + unit tests under ASan/UBSan via `fuzz/Makefile`), `unit` (windows-2022: MSVC `/W4 /WX /wd4127 /std:c11` build+run of `tests\unit\test_compat.c`), `build` (windows-2022 matrix Debug+Release: NuGet WDK → msbuild → stamp → PE gate → unsigned assertion → upload `rda-{tag}-{config}` with rda.sys/rdacli.exe/rda.inf), `codeql` (windows-2022, c).
- **Release pipeline**: see §3 (dist bundle + SHA256SUMS + GitHub release on `v*` tags).
- **Benchmark pipeline** (`benchmark.yml`, windows-2022, on `src/**` pushes + manual): NuGet build → stamp+gate → fetch reference drivers at run time from `https://raw.githubusercontent.com/heicfast/easeus-drw-research/main` (eprdtdrv.sys from `og-codebase/PartitionRecovery/`, EUEDKEPM.sys + EUDCPEPM.sys from `og-codebase/BUILDPE/x64/Windows/system32/drivers/`) → test-sign rda.sys (job-local cert, §7) → create+seed a 4 GiB GPT VHDX (NTFS+FAT32, JPEG/PDF/MP4/ZIP/text/1000-small-files corpus) → install BOTH drivers under distinct service identities (`eprdtdrv` original WHQL-signed, `rdatest` ours) → run `rdabench` matrix (info/open/seq 1 MiB+64 KiB/rand 4 KiB+64 KiB/verify/fuzz 50k) → markdown comparison + cross-driver SHA-256 byte-equality assertion (any mismatch fails the job) → cleanup (`sc.exe stop|delete` both, detach+delete VHD, remove driver files + test cert) → upload `bench-results/**` (30-day retention).
- Reference binaries are fetched at RUN time, never committed (also true of `tools/ref_imports.json`, which merely lists the 75-export union).

---

## 10. The client CLI — `rdacli.c` (the model consumer for the core engine)

`src/client/rdacli.c` (443 lines, console app, `/W4 /WX`, links only the shared headers). **The core engine should mimic these consumption patterns**:

1. **Token computation client-side**: `TokenFromPid()` = `RdaComputePidToken((ULONG)GetCurrentProcessId())` — the same shared inline function the kernel uses (byte-sum transform in `rda_compat.h`).
2. **Open paths** (all `CreateFileW`, `FILE_SHARE_READ|FILE_SHARE_WRITE`, `OPEN_EXISTING`):
   - Global/bare: `\\.\EPRDTDRV` (device name HARDCODED to the eprdtdrv drop-in name in the shipped client — `OpenDevice()` uses `L"EPRDTDRV"`).
   - 3-segment: `\\.\EPRDTDRV\<token>\<disk>\<part>` (`RDA_OPEN_FMT L"\\\\.\\%ws\\%u\\%u\\%u"`).
   - Volume: `\\.\EPRDTDRV\<token>\Volume\<N>`.
   - 2-segment (EUEDKEPM): `\\.\EPRDTDRV\<token>\<disk>`.
   - Negative test: token XOR 0x5A5A must fail the open.
3. **Subcommands** (usage in `Usage()`): `info`, `grammar <disk>`, `read <disk> <offset> <len>` (write command exists in the header comment but is not wired in `main`), `ident <pid>`, `session <disk> <startSec> <secs>`, `bench read <disk> <secs> [chunkSectors]`.
4. **IOCTL usage**: `DeviceIoControl(h, IOCTL_RDA_GET_INFO, NULL, 0, &info, sizeof(info), &bytes, NULL)`; `IOCTL_RDA_GET_VERSION` → `ULONG64`; `IOCTL_RDA_GET_PROC_NAME` with `&pid, sizeof(pid)` in / `RDA_IDENT_MIN_OUT_LEN`-sized buffer out; `IOCTL_RDA_GET_DISK_CONTEXT_MODE` with 4-byte `LONG` out.
5. **Raw read**: `SetFilePointer` (64-bit split) + `ReadFile`; **the client checks the legacy ×512 Information scaling** (`got != len * RDA_LEGACY_INFORMATION_SCALE` → NOTE).
6. **Session flow** (CmdSession): open global R|W → `calloc(1, 0x840)` → fill `{DiskId, DataFirstSec, TotalSecs, BitmapSize=0x800, BitmapCount=0x100, BlockBitmap memset 0xFF for 0x100, SpbInOut=0}` → `DeviceIoControl(h, RDA_SESSION_CODE_OPEN_SESSION /* + IoctlBase, 0 here */, in, 0x840, in, 0x840, …)` → read back `SessionHandle`, `SpbInOut`, `SectorSize` → up to 8 `RDA_SESSION_CODE_GETNEXTBLOCK` rounds (in = handle + pad to 0x1E; out = 64 KiB stack buffer) → `RDA_SESSION_CODE_GET_PROGRESS` (in 0xC bytes, percent at out+8) → `RDA_SESSION_CODE_CLOSE_SESSION` (in = 8-byte handle).
7. **Bench**: driver path vs `\\.\PhysicalDriveN` baseline, chunk = 128 sectors default.
8. `run-integration.ps1` drives `info`, `grammar`, `ident $PID`, `read 0 512` and gates on exit codes.

`rdabench.c` additionally demonstrates: `--device` is parameterized (drives ANY wire-compatible driver by symlink name — used to hit `EPRDTDRV` and `RDATEST` side by side), `FILE_FLAG_NO_BUFFERING` + page-aligned buffers for kernel-honest timing, `UnscaleInformation(scaled) = scaled / 512` on every read, SHA-256 per 1 MiB range via BCrypt for cross-driver equality, and a hostile IOCTL fuzz hammer (fixed code table incl. valid/invalid codes, random in/out lengths) that must survive 50k iterations.

---

## 11. Windows 7 compatibility & OS-version gating

(`docs/win7-compat.md`, enforced end-to-end.)

- One binary Win7 SP1 → Win11, proved by 4 machine-verified properties: (1) source discipline (only XP/2003-era APIs; `ExAllocatePoolWithTag`, never `ExAllocatePool2`; no Win8+ kernel APIs); (2) `NTDDI_VERSION=0x06010000` (`NTDDI_WIN7`), `_WIN32_WINNT=0x0601`, `WINVER=0x0601` pinned in the vcxproj — WDK 26100 headers gate to Win7 semantics (verified to reject only *below* Win7); (3) PE contract `/osversion:6.1 /subsystem:native` + `stamp_osversion.py` post-fix (the WDK toolset links OS=(10,0) regardless — worklog "THE BIG ONE" item 6); (4) `verify_pe.py` import allowlist gate (75-function reference union + audited additions; "A single stray import fails the build").
- The approach mirrors the reference: `eprdtdrv.sys` built 2022, PE OS 6.1, 41 Win7-era imports, runs Win7→Win11.
- **Explicitly NOT claimed**: Win7 x86 (x64 only); the binary still needs per-OS signature policy (§7); runtime Win7 testing is a self-hosted VM task, hosted CI "compiles and gates but cannot load".
- Live hazard captured (benchmark lesson): `ExAllocate/FreeFromLookasideListEx` are FORCEINLINE before NTDDI_WIN10_NI and real exports only from Win11 22H2 — importing them caused StartService error 127 on Server 2022; replaced with the Win2000-era `ExpInterlocked*EntrySList` (allowed in the gate with justification).
- Other Win7 nods: `PoStartNextPowerIrp` always called (required Win7, no-op later); legacy power IRP discipline in the filter role.
- Modern-track compile check (VS2026 + WDK NuGet 10.0.28000.2526, runner `windows-2025-vs2026`) is planned but "added once the runner label is publicly available" (`ci.yml` comment); shipped artifact always from the conservative VS2022/26100 toolchain.

---

## 12. Known issues / caveats (from worklog.md + error-fix-registry.md + audits)

**Fixed reference-driver defects this rewrite addresses (F-series, `docs/error-fix-registry.md`)** — wire contracts preserved unless noted:
F1 512-hardcode→geometry probe (per-handle cached); F2 no ACL→SDDL admin/SYSTEM; F3 open identity IOCTLs→behind DACL; F4 no IRP cancellation→cancellable forward-wait; F5 kernel string parsing→bounded shared pure-logic + explicit OPEN_TARGET; F6 AddDevice patching of foreign drivers→documented `UpperFilters` class registration; F7 DbgBreakPoint on request path→`STATUS_INVALID_HANDLE`; F8 power IRP pairing→full PnP/Power + `PoStartNextPowerIrp`; F9 release at CLOSE→CLEANUP; F10 METHOD-blind pass-through→method-aware (NEITHER rejected); F11 misleading `Information = Length × 512` → **preserved intentionally** (legacy clients rescale); F12 raw kernel-pointer session handles→opaque generation-tagged IDs; F13 reference leak claim reworded (G6: both references were actually balanced); F14 pagefile "urgent IRP" heuristic not reimplemented (documented divergence); F15 no request-path bugchecks.

**Self-audit defects found & fixed (S-series, `docs/audit/2026-10-22-security-review.md`)**: S1/S2 (P0) cancel-routine global-spinlock bugs that would deadlock the machine on first cancel; S3 control-device zero-extension deref; S4 filter REMOVE use-after-free window (event-wait added); S5 OpenSession reference leak; S6 overlap-check TOCTOU; S7 truncating cast; S8 buffer-underflow guard; S9 IRQL violation; S10 zero-length IO (rejected deliberately); S11 power path status; S12 dead code. **All fixed** — verified against `src/driver/*.c`.

**Contract corrections (G1..G28, `docs/audit/2026-10-22-decompile-reverification.md`)** — notable: session protocol is 19 codes (discovery + RRWR families had been missed); GETNEXTBLOCK pumps into the StartBackup-registered buffer with `Information = 0x1E`; GET_PROGRESS is FINALIZE; direct/sized are METHOD_OUT_DIRECT with LBA offsets and `Information = outLength`; CANCEL sets state 3; G11 master dispatch (control handlers can never run on filter stacks).

**Current open items / residual risks:**
- Signing: disabled pending OSSign onboarding + Hardware Dev Center evaluation (§7). `rda.sys` cannot load on stock Secure-Boot Win10/11 today.
- `rda.cat`: INF declares it; nothing builds it.
- 56/57 checks green in `scripts/verify/verify_reference_binaries.py`; the remaining one is a documented compiler-encoding artifact.
- COW cache capped at 64 MiB/session → external writers flooding an active session faster than the pump drains get `STATUS_INSUFFICIENT_RESOURCES` (deliberate, documented).
- `DbgPrintEx` tracing compiled-in (level-gated, default errors-only); WPP/ETW is a roadmap item.
- Session engine served whenever `SessionsEnabled=1` (default) — hostile-input surface bounded but present (fuzzed in CI).
- VolSnapAttach is parameter-gated but currently trace-only (no actual VolSnap attach implemented — `rda_filter.c` `RdaFilterStart` logs the request only).
- Head-commit fixes to know about: `da7efb9` fixed `sc.exe.exe`→`sc.exe` in install/uninstall; `7e88e38` fixed the empty-name/null-buffer classifier ordering (error 87 on every bare open — the exact hazard a combined installer's smoke test would hit).
- `docs/ground-truth-contracts.md` §7 mentions SeAdminCheck gating on the global-handle park mode, but no such call exists in the implementation — the DACL is the actual gate (code truth over doc prose).
- Load-group name differs between install.ps1/reg file (`"Boot File System"`) and benchmark.yml (`"FSFilter Boot File System"`) — see §5.3.
- fuzz/selftest + fuzz/unit_asan are tracked compiled ELF binaries (dev artifacts committed; harmless but noteworthy for repo hygiene in a merge).

---

## 13. Quick-reference: what a combined installer must reproduce

1. **Ship** `rda.sys` (unsigned until signing lands) + `rda.inf` + `rdacli.exe` (+ optionally `rdabench.exe`) from the release bundle layout (`dist/`).
2. **Pick a service name per profile**: `eprdtdrv` for DRW drop-in (device `\Device\EPRDTDRV`); any `^[A-Za-z0-9_]+$` name works; `rda` for the PE profile.
3. **Copy** the .sys to `%SystemRoot%\System32\drivers\<ServiceName>.sys`.
4. **Create service**: `sc.exe create <SN> binPath= "<absolute path>" type= kernel start= demand error= normal group= "Boot File System"`.
5. **Write Parameters**: `IoctlBase` (0 live / 0x10000 PE), `MaxDisks` 200, `DebugLevel` 1, `SessionsEnabled` 1; PE adds `FilterMode` 1, `VolSnapAttach` 0, and disk-class `UpperFilters` REG_MULTI_SZ `"rda"` on `{4d36e967-e325-11ce-bfc1-08002be10318}`.
6. **Start**: `sc.exe start <SN>`; verify `\Device\<UPPER(SN)>` exists; signature policy must be satisfied first (test-signing or signed binary; relaxed in WinPE).
7. **Smoke test** (elevated): `rdacli.exe info` → GET_INFO/GET_VERSION on the bare device; `rdacli.exe grammar <disk>` → all open grammars + wrong-token rejection; `read` verifies the ×512 Information contract.
8. **Uninstall**: stop → `sc.exe delete` → remove the .sys file.
