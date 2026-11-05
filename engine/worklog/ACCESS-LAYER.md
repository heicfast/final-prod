# ACCESS-LAYER — the complete driver-access-layer contract (session driver-replacement, Task 2-a / Agent A)

> READ-ONLY excavation of the tree at HEAD `b6418420`. Every claim cites `path:line`
> against that tree; historical claims cite `e58bde10~1:path:line`, where `e58bde10~1`
> resolves to `0f26ed23` ("docs(plans): the three surgical plans", 2026-10-27) — the
> last tree before `e58bde10` ("feat(driver-only)!: the direct-access fallback is
> REMOVED"). The purpose of this document: the COMPLETE contract of the current
> RDA-driver access layer, so the planned direct user-mode raw-I/O replacement
> (\\.\PhysicalDriveN / Volume-GUID paths under admin privilege) preserves it exactly.
> Companion documents: docs/DRIVER-REQUIRED.md (the operation classification +
> refusal contract), docs/DRIVER-CONTRACT.md (the rda-driver wire surface),
> docs/CORE-DISK-ACCESS.md (the pre-removal census, now carrying its own
> POST-REMOVAL banner at docs/CORE-DISK-ACCESS.md:11-18).

---

## §1 The layer map — every file in the access layer

| File | Role (one line) |
|---|---|
| `src/win/cr_transport.cpp` (1086 lines) | THE transport: grammar opens, device classes (DirectDevice / RdaTargetDevice / RwDevice), geometry probing, the 64 KiB read envelope, handle self-healing, the S-010 quiesce + S-029 IOCTL chains, driver-status query, destination arming (src/win/cr_transport.cpp:36-1086). |
| `include/cr/win/cr_transport.h` | Public transport surface: every open signature, `image_file_io()`, extent/arming helpers, the N-125 `disconnect` classification namespace (include/cr/win/cr_transport.h:53-183). |
| `include/cr/win/cr_rda.h` | The RDA wire mirror: IOCTL codes, `DriverInfo`/`OpenTargetIn`/session structs (size-frozen), the PID-token transform declaration, `kLegacyInformationScale = 512` (include/cr/win/cr_rda.h:38-234). |
| `include/cr/win/cr_driver_status.h` | The pure, platform-independent facet struct + 6-state classifier + error/state-name mapping (include/cr/win/cr_driver_status.h:39-142). |
| `include/cr/win/cr_elevate.h` + `src/win/cr_elevate.cpp` | The definitive TokenElevation probe `is_elevated()` (include/cr/win/cr_elevate.h:23; src/win/cr_elevate.cpp:12-21). |
| `include/cr/win/cr_winenv.h` | Platform-independent corpus pins: the PE predicate, `ENUMERATE_DISK_BOUND = 200`, the quiesce/restore hex constants (include/cr/win/cr_winenv.h:36-66). |
| `include/cr/win/cr_enumerate.h` | Enumeration contracts: PhysicalDisk/VolumeEntry records, the pure two-stack merge (include/cr/win/cr_enumerate.h:37-255). |
| `src/win/cr_enumerate.cpp` | The classified-(b) control path: numbered \\.\PhysicalDriveN loop + cfgmgr32 interface pass, table parse through `open_physical_drive` (src/win/cr_enumerate.cpp:115, 148, 201). |
| `include/cr/win/cr_destio.h` + `src/win/cr_destio.cpp` | Win32 destination writer (atomic temp+rename) — driver-independent by classification (include/cr/win/cr_destio.h:17-49). |
| `src/win/cr_image_win.cpp` | The Win32 `ImageFileIo` (`WinImageFileIo`) + the `image_file_io()` singleton — the S-030 image seam, driver-independent (src/win/cr_image_win.cpp:21, 113-116). |
| `src/win/cr_image_create.cpp` | The S-040 image producer: `create_disk_image()` opens the source through `open_disk` (src/win/cr_image_create.cpp:175-181). |
| `src/win/cr_health.cpp` | Direct SMART/NVMe/CSMI health — its own \\.\PhysicalDriveN / \\.\Scsi%u: handles, never the driver (src/app/rrecov.cpp:2769-2774; src/win/cr_health.cpp:169). |
| `src/core/storage/` (cr_retry.cpp, cr_cached.cpp, cr_slice.cpp, cr_devmap.cpp, cr_decrypt_device.cpp, cr_image.cpp) | The pure-core IStorageDevice stack above the transport (§3). |
| `include/cr/storage/*.h` (cr_device.h, cr_retry.h, cr_cached.h, cr_slice.h, cr_decrypt_device.h, cr_image.h, cr_directalign.h) | The contracts of that stack, including the 64 KiB envelope math (include/cr/storage/cr_directalign.h:33-73). |
| `include/cr/cr_error.h` + `src/core/err/cr_error.cpp` | The error taxonomy incl. the 0x500B driver family (include/cr/cr_error.h:112-120) and the driver-era `err_text` lines (src/core/err/cr_error.cpp:94-105). |
| `src/app/rrecov.cpp` | The CLI: every driver-requiring verb gated by `require_driver()` (§5). |
| `src/ui/cr_ui.cpp` + `include/cr/cr_ui.h` | The C ABI over the same layer, incl. `cr_ui_driver_status` and `cr_ui_last_transport_error` (§4-§5). |
| `tests/unit/test_driver_status.cpp`, `test_transport_logic.cpp`, `test_directalign.cpp`, `test_retry.cpp`, `test_cached.cpp`, `test_slice_devmap.cpp`, `test_cr_ui.cpp` | The Linux-lane pins (§7). |
| `tests/win-stub/` (windows.h, winioctl.h, …) + `tools/win_syntax_check.sh` | The stubbed Win32 surface the Linux lane parse-compiles the win/app/ui layers against (§7). |
| `docs/DRIVER-REQUIRED.md`, `docs/PLAN-DRIVER-ONLY.md`, `docs/DRIVER-CONTRACT.md`, `docs/CORE-DISK-ACCESS.md`, `docs/DRIVER-UTILIZATION.md`, `docs/ABI-CONTRACT.md` | The documented contract (§7, §9). |
| `installer/` (core-recovery.iss, build-installer.ps1) | Driver artifact + service install/uninstall wiring (§9). |
| `bench/` (run-alignbench.ps1, verify-driver.ps1, run-ours.ps1) | The driver round-trip / F11-unscale / transport-assertion benches (§7). |

Layering (data path, top to bottom):

```
engines (ntfs/fat/exfat/ext4/hfsplus/btrfs/zfs/refs/lost/carve)
  -> CachedReader (8 MiB / 256 KiB, D-72 divergence)
  -> RetryingDevice (3 attempts, N-004 backtrack, bisect, Skip zero-fill, ReadErrorMap)
  -> SliceShim -> PartitionSlice (byte-bounded window over the parent)
  -> [DecryptingDevice, opt-in FVEK]           (cr_slice.h:98-114; cr_decrypt_device.h:48-77)
  -> IStorageDevice transport: RdaTargetDevice / RwDevice (grammar \\.\RDA\<tok>\<d>\<p>)
     |  DirectDevice survives ONLY for the classified-(b) control path (open_physical_drive)
  -> kernel: RDA driver -> storage stack
```
(cited per layer at src/core/orchestrator/cr_scan.cpp:153-200; docs/CORE-DISK-ACCESS.md:23-50.)

---

## §2 Every operation the transport provides

All public entry points are declared in include/cr/win/cr_transport.h:53-152 and defined in
src/win/cr_transport.cpp. Signatures below are the CURRENT (post-e58bde10) forms.

### 2.0 Shared primitives (anonymous namespace)

**`u32 overlapped_read(HANDLE h, u64 off, void* buf, u32 len, bool rda_scaled, DWORD* gle_out)`** — src/win/cr_transport.cpp:53-82.
- Zero-length returns OK immediately (:54-55). One OVERLAPPED per call, offset split
  `ov.Offset`/`ov.OffsetHigh` (:56-58) — position-independent reads, so a shared
  synchronous HANDLE is safe under the ABI's per-handle single-flight contract
  (:188-191).
- `ReadFile` → on immediate failure: `e != ERROR_IO_PENDING` → classify
  `disconnect::gle_is_disconnect(e) ? ERR_DEVICE_DISCONNECTED : ERR_READ_FAILED`
  and export the GLE via `gle_out` (:60-69). On `ERROR_IO_PENDING` →
  `GetOverlappedResult(..., TRUE)` blocks; failure classifies identically (:70-75).
- Success count: `got == len` → OK; `rda_scaled && got == len * rda::kLegacyInformationScale`
  → OK (**the F11 contract**: the driver reports `Information = Length * 512`,
  include/cr/win/cr_rda.h:63-65; docs/DRIVER-CONTRACT.md §5); anything else →
  `ERR_READ_FAILED` (:77-81).
- Threading: no lock of its own — callers serialize (see per-device `heal_mu_`).

**`bool probe_length(HANDLE h, u64* out)`** — src/win/cr_transport.cpp:86-94.
`IOCTL_DISK_GET_LENGTH_INFO` (exact byte length, works on volume AND disk handles;
DISK_GEOMETRY_EX rounds — comment :84-85).

**`Geometry probe_geometry(HANDLE h, u64* out_size)`** — src/win/cr_transport.cpp:96-152.
Sequence: `IOCTL_DISK_GET_LENGTH_INFO` (exact size preferred) →
`IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` (BytesPerSector into logical AND physical,
:102-105) → `IOCTL_STORAGE_QUERY_PROPERTY` with `StorageAccessAlignmentProperty`,
reading **BytesPerLogicalSector@+0x10 / BytesPerPhysicalSector@+0x14** (the 183dfcd3
fix — the pre-fix code read the CACHE-LINE fields +8/+0xC and every VHD reported
logical_sector 0, killing slice creation; comment :113-122), sanity-gated to
[256, 65536] (:126-129), else geometry-EX value stands, else physical = logical
(:130-132). `total_bytes` = exact length when available, else `dg.DiskSize`
(:133). **N-206 CEIL**: `sector_count = (total_bytes + ss - 1) / ss` — the og's
geometry-EX form, the final partial sector is addressable/driver-padded
(:134-139; pinned by Geometry::valid() include/cr/storage/cr_device.h:49-60 and
tests/unit/test_image.cpp:163). Fallback when geometry IOCTL is not forwarded
(some volume stacks): 512/512 + exact length + ceil (:140-148).

**`u32 aligned_read_at(HANDLE h, u64 off, void* buf, u32 len, bool rda_scaled, u64 dev_size, DWORD* gle_out)`** — src/win/cr_transport.cpp:193-219. THE 64 KiB envelope, unified on BOTH transports (comment block :154-192):
1. `io::direct_envelope(off, len, dev_size)` (include/cr/storage/cr_directalign.h:48-73):
   offset rounded DOWN, end rounded UP to 64 KiB multiples, clamped at the device
   end (the documented ragged-tail exception); `skip`/`take` locate the request
   inside the envelope; `direct` = the zero-copy fast path (aligned offset,
   64-KiB-multiple length, fully inside the device).
2. `!e.ok` → `ERR_RANGE_OUT_OF_BOUNDS` (:198-199). `e.direct` → read straight
   into the caller's buffer (:200-201).
3. Bounce path: `issued_len > 1 MiB + 64 KiB` → `ERR_BAD_PARAMETER` (the writer
   chunks at 1 MiB; anything bigger means caller-side chunking broke — :205-206).
   `thread_local std::vector<u8> bounce` (per-thread, tolerates concurrent
   callers — :209-210), read the envelope, `take != len` → `ERR_READ_FAILED`
   (envelope did not cover the request), else memcpy out (:211-218).
- Why 64 KiB: (a) sector rejection — the Windows disk stack rejects ReadFile IRPs
  whose length/offset is not a logical-sector multiple (bench run 40073093997 —
  every non-512-multiple file was zero-filled "salvage"); 64 KiB is a multiple of
  every real logical sector size; (b) GAP-S6-13 — 4-KiB-multiple-but-not-64-KiB
  lengths take the ~3.5x slow path (~830 vs ~2900 MB/s NVMe) (:158-177). The F11
  compensation keys on the ISSUED length, orthogonal to the envelope (:185-186).

**`bool reopen_handle(const wchar_t* path, DWORD access, DWORD share, HANDLE* h_inout)`** — src/win/cr_transport.cpp:241-253. The self-heal primitive: `CreateFileW` the SAME
path/access/share (a fresh handle to the same target — og parity
`Device_decompiled.c:55947-55959`), swap under the caller's lock, close the old
handle (:244-252). One heal attempt per failing call; geometry is NOT re-probed
(:223-239 comment).

### 2.1 `open_rda_partition(service_name, disk, part, err)` — src/win/cr_transport.cpp:483-508

The DATA-PATH open. Exact sequence:
1. `tok = rda::compute_pid_token(GetCurrentProcessId())` (:485; transform at :374-378 —
   `b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24`, 8-bit wraps; the driver re-derives
   it from the calling process at CREATE, include/cr/win/cr_rda.h:107-112).
2. `_snwprintf_s L"\\\\.\\%s\\%u\\%u\\%u"` — the 3-segment EPMNtDrv grammar
   `\\.\SVC\<token>\<disk>\<part>` (:486-487). No cascade, no 2-segment or
   `Volume\N` variant on this path (FB-L01; include/cr/win/cr_transport.h:55-59).
   Grammar semantics on the driver side: `\Device\Harddisk{d}\Partition0` with
   `\Device\Harddisk{d}\DR{d}` fallback, walk down to first `\Device\Harddisk`
   layer, **the part number is never used** (docs/DRIVER-CONTRACT.md §3 table).
3. `CreateFileW(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)` (:488-490). Failure →
   `*err = ERR_OPEN_FAILED`, null (:491-495).
4. `probe_geometry` — `total_bytes == 0` → CloseHandle + `ERR_OPEN_FAILED` (:496-502).
5. Returns `RdaTargetDevice(h, g, path, GENERIC_READ, R|W|D)` (:503-507).

`RdaTargetDevice::read_at` (src/win/cr_transport.cpp:333-346): bounds
(`off > size() || len > size()-off` → `ERR_RANGE_OUT_OF_BOUNDS`), then
`aligned_read_at(..., rda_scaled=true, ...)`; on `ERR_READ_FAILED` with
`gle == ERROR_INVALID_HANDLE (6)`: under `heal_mu_`, `reopen_and_rebind_()`
(= `reopen_handle` on the same grammar path — the driver re-binds the target at
IRP_MJ_CREATE; comment :350-353) and one retry (:338-345). Destructor closes the
handle (:329-332). Geometry snapshot is immutable after open (N-046's refresh
lives on PartitionSlice, not here — include/cr/storage/cr_slice.h:58-72).

### 2.2 `open_rda_volume(service_name, volume, err)` — src/win/cr_transport.cpp:510-534

Identical posture, grammar `\\.\%s\%u\Volume\%u` (:513). Driver side opens
`\Device\HarddiskVolume<N>` top-of-stack, mode 0 (docs/DRIVER-CONTRACT.md §3).
**Zero production callers** (documented grammar branch kept for the wire
contract; the only callers of grammar opens are the decision sites listed in §5,
all of which use `open_rda_partition` / `open_disk`). Volume-number parse failure
falls back to volume 0 driver-side (docs/DRIVER-CONTRACT.md §3).

### 2.3 `open_disk(disk, service, err)` — src/win/cr_transport.cpp:574-586

`open_disk` = `open_rda_partition(service ? service : kDefaultDriverService, disk, 0, err)`
(:585). The direct fallback and the WinPE gate were REMOVED by PLAN-DRIVER-ONLY R2
(:575-584 comment): a failure here with a Ready driver is a TARGET error
(ERR_OPEN_FAILED), not a driver-state refusal; callers that need the classified
refusal check `query_driver_status` first. `kDefaultDriverService = L"RDA"`
(include/cr/win/cr_driver_status.h:39).

### 2.4 `open_disk_rw(disk, service, err)` / `open_disk_apply_rw(disk, service, err)` — src/win/cr_transport.cpp:948-983

Both are typed views of the anonymous `open_rw_device(disk, service, err)` (:948-974):
grammar `\\.\%s\%u\%u\%u` with `GENERIC_READ|GENERIC_WRITE`,
share `R|W|DELETE` (:951-955); CreateFile failure → `ERR_OPEN_FAILED`; geometry
zero → close + `ERR_OPEN_FAILED` (:956-967); returns
`RwDevice(h, g, via_rda_=true, disk, path, GENERIC_READ|GENERIC_WRITE, R|W|DELETE)`
(:970-972). `open_disk_rw` returns `cr::repair::IWriteDevice` (:976-978);
`open_disk_apply_rw` returns `cr::apply::IApplyDevice` (:980-983). The direct
\\.\PhysicalDriveN RW tail is REMOVED (R3; comment :940-946).

### 2.5 The `RwDevice` class — src/win/cr_transport.cpp:596-936

`cr::apply::IApplyDevice` (IS-A `repair::IWriteDevice`), so one class carries the
repair write path, the apply chain, and the S-010 quiesce. Members: `h_`, `geom_`,
`via_rda_`, `disk_`, `path_`, `access_`, `share_`, `write_gle_`, `heal_mu_`,
`locked_`, `locked_volumes_`, `restores_` (:923-935).

**`read_at`** (:706-723): bounds → `aligned_read_at(..., via_rda_, ...)` (M-009: the
write path rides the same 64 KiB envelope as the scan path, comment :711-713) →
`ERROR_INVALID_HANDLE` heal-and-retry-once under `heal_mu_` (:716-721).

**`write_at`** (:725-741): bounds → `write_impl_`; on `ERR_WRITE_FAILED` with
`write_gle_ == ERROR_INVALID_HANDLE`: heal + retry once (:733-739; og
`Device_decompiled.c:56182-56183`).

**`write_impl_`** (:838-877): the staged WriteFile.
- **Direct-path alignment rule** (the seed constraint for the replacement): when
  `!via_rda_` and the caller's buffer is not sector-aligned
  (`(uintptr_t)buf % ssec != 0`), stage through `_aligned_malloc(len, ssec)` +
  memcpy (:843-849); the RDA path MDL-locks any alignment (:833-835 comment).
  `_aligned_malloc` failure → `ERR_NO_MEMORY` (:845-846).
- OVERLAPPED WriteFile; immediate failure `!= ERROR_IO_PENDING` → `write_gle_ = e`,
  `ERR_WRITE_FAILED` (:854-861); `GetOverlappedResult` failure → same (:862-868).
- Success counts: `via_rda_` accepts `put == len` OR `put == len * 512` (F11 on
  writes too); `!via_rda_` requires `put == len` (:872-875).

**`lock_volume()`** (S-010 + M-030, :633-698) — the quiesce chain, per MOUNTED
volume on `disk_` with a letter:
1. `enumerate_volumes` unavailable → return OK (nothing to quiesce, :636-638).
2. `CreateFileW("\\\\.%c:", GENERIC_READ|GENERIC_WRITE, R|W)` per volume —
   open failure → `ERR_VOLUME_LOCK_FAILED` (in use: refuse, never race, :644-648).
3. `FSCTL_LOCK_VOLUME` failure → close + `ERR_VOLUME_LOCK_FAILED` (:649-654).
4. Capture the volume's GUID path via `GetVolumeNameForVolumeMountPointW` BEFORE
   the mount point goes away (the OFFLINE step removes the volume from the
   mountable set; the restore needs `\??\Volume{...}\`, :655-668).
5. `DeleteVolumeMountPointW` (the OS must not hand the volume back out) →
   `FlushFileBuffers` → `Sleep(2000)` settle → `FSCTL_IS_VOLUME_MOUNTED` (verify,
   result ignored) → `FSCTL_DISMOUNT_VOLUME` (:669-677).
6. **`IOCTL_VOLUME_OFFLINE` (0x56c00c) — the M-030 terminal step**: failure is
   recorded through the `cr::log` Warn seam ("degraded to dismount+lock posture"),
   never fatal (:678-692).
7. Handles are HELD for the device lifetime; `locked_volumes_`/`restores_` grow in
   lockstep (:693-694). Idempotent via `locked_` (:634-635).
Corpus provenance: RecoveryModule_decompiled.c:60401-60447, pinned constants in
include/cr/win/cr_winenv.h:47-66.

**`unlock_volume()` / `release_locked_volumes_()`** (:699-704, :896-921): closing
each held handle releases its lock first (:901), then the M-030 restore chain:
reopen the volume by GUID, `IOCTL_VOLUME_ONLINE` (0x56c008),
`SetVolumeMountPointW` (the saved letter), `Sleep(1000)` (:902-916; corpus
FUN_1800459b0 :34465-34502). A volume with no captured GUID degrades to
handle-close only (the pre-M-030 behavior, never worse — :888-890).

**The S-029 apply chain** (corpus PartitionRecovery device-control; error
convention: OK = DeviceIoControl succeeded, any failure → `ERR_WRITE_FAILED` so
`execute_apply` treats it as a hard device error and rolls back — :746-751):
- `delete_drive_layout()` — `IOCTL_DISK_DELETE_DRIVE_LAYOUT` raw 0x7C100 (:752-758).
- `set_drive_layout_mbr(signature, entry_count)` — `IOCTL_DISK_SET_DRIVE_LAYOUT`
  0x7C010 with a 0xA8-byte DRIVE_LAYOUT_INFORMATION: count (default 4) + MBR
  signature, per-entry payload zero except RewriteBoot on the first — the ioctl's
  job is the layout-signature commit; the sector-0 write carried the real bytes
  (:759-781).
- `set_partition_info_ex(rec)` — 0x7C058 with the 0x18-byte record
  {u32 partition number, 16-byte identity, u32 boot flag} (:782-798).
- `update_properties()` — `IOCTL_DISK_UPDATE_PROPERTIES` 0x70140 (:799-805).
- `settle_refresh()` — `Sleep(1000)` (corpus PR:47015-47025, :806-808).
- `legacy_os_path()` — constant false (the build envelope is Win7 SP1+; the
  corpus GetVersionExA gate's only live branch, :809-814).
- `generate_disk_signature()` — fresh 32-bit signature from
  QueryPerformanceCounter ^ GetTickCount<<32 ^ disk<<48, FNV-mixed, never 0
  (:815-830).
All of these are issued **through the RDA bound handle — the driver passes
non-session IOCTLs through to the bound target** (include/cr/win/cr_transport.h:112-120;
docs/DRIVER-CONTRACT.md §4.3: METHOD_BUFFERED out = SystemBuffer, METHOD_NEITHER
rejected STATUS_NOT_SUPPORTED). A direct replacement must issue them on
\\.\PhysicalDriveN itself (exactly what `refresh_disk_properties` already does, §2.7).

### 2.6 `open_physical_drive(disk, err)` — src/win/cr_transport.cpp:542-546

CONTROL-PATH only (classified (b)): `open_path(L"\\\\.\\PhysicalDrive%u")`.
`open_path` (:292-317): CreateFileW GENERIC_READ share R|W|D → probe_geometry →
zero-size exotic-handle fallback `GetFileSizeEx` with 512/512 geometry
(:302-311, FB-T09) → `DirectDevice`. `DirectDevice::read_at` (:266-279) is
`aligned_read_at(..., rda_scaled=false, ...)` with the same ERROR_INVALID_HANDLE
heal. Callers: enumeration only (src/win/cr_enumerate.cpp:148, :201). NOT a
data-path leg (comment :536-541; DRIVER-REQUIRED.md §1 table row "list").

### 2.7 `refresh_disk_properties(disk)` — src/win/cr_transport.cpp:985-1010

The (b)-classified layout refresh: opens `\\.\PhysicalDrive%u` with
GENERIC_READ|GENERIC_WRITE share R|W **directly** (:993-996) — the one surviving
data-bearing direct RW open in the file — then `IOCTL_DISK_UPDATE_PROPERTIES`
with up to 3 attempts and `Sleep(1000)` between (corpus PR:46873-46880, :1000-1007).
CreateFile failure → `ERR_OPEN_FAILED`; 3 failed IOCTLs → `ERR_WRITE_FAILED`
(:997-1009). Best-effort by contract: the caller surfaces the code without
failing the apply (include/cr/win/cr_transport.h:102-110).

### 2.8 `volume_extent(letter, disk, start, len)` — src/win/cr_transport.cpp:548-572

Opens `\\.\%c:` GENERIC_READ share R|W|D, `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`
into `VOLUME_DISK_EXTENTS ext[8]`, reports the FIRST extent only (N-013: kept for
source-disk lookup; destination arming must use volume_extents —
include/cr/win/cr_transport.h:123-127). Any failure → false.

### 2.9 `volume_extents(letter, out)` (N-013) — src/win/cr_transport.cpp:1019-1046

ALL extents (a spanned/RAID destination has more than one). Opens `\\.\%c:` with
**access mask 0** (metadata-only open, :1025-1026), same IOCTL; `ERROR_MORE_DATA`
(>8 extents) is tolerated — the first 8 are used (documented cap, :1034-1037);
fills `VolExtent{disk, start, len}` (include/cr/win/cr_transport.h:136-141);
false when unopenable or none.

### 2.10 `destination_volume_letter(path, letter)` (N-013) — src/win/cr_transport.cpp:1048-1082

1. `PathIsNetworkPathW` → false (network destinations never opened raw, never
   armed, :1051-1054).
2. `CreateFileW(path, 0, R|W|D, FILE_FLAG_BACKUP_SEMANTICS)` +
   `GetFinalPathNameByHandleW(VOLUME_NAME_DOS)` resolves reparse points to the
   FINAL path (a junction into another volume yields the TARGET's volume);
   `\\?\` prefix stripped (:1058-1071).
3. Fallback (destination folder may not exist yet): `GetVolumePathNameW` on the
   path itself (:1073-1077). Root on a network root → false (:1078-1079).
   Success: `*letter = root[0]` (:1080-1081).

### 2.11 `image_file_io()` — include/cr/win/cr_transport.h:68-71; src/win/cr_image_win.cpp:113-116

The platform `storage::ImageFileIo` singleton: `WinImageFileIo` — CreateFileW
GENERIC_READ share R|W|D + GetFileSizeEx + position-independent OVERLAPPED reads
behind a locked handle table (src/win/cr_image_win.cpp:20-110). Feeds
`storage::open_image` (driver-independent by classification; S-030).

### 2.12 `query_driver_status(service_name, out)` — src/win/cr_transport.cpp:411-481

See §4.

### 2.13 The `disconnect` classification (N-125) — include/cr/win/cr_transport.h:154-183

Pure predicate over numeric GLEs (no windows.h, both CI lanes compile it):
`ERROR_DEV_NOT_EXIST(55)`, `ERROR_NOT_READY(21)`, `ERROR_MEDIA_CHANGED(1110)`,
`ERROR_NO_MEDIA_IN_DRIVE(1112)`, `ERROR_DEVICE_REMOVED(1619)` →
`ERR_DEVICE_DISCONNECTED` (0x50020007). Media errors (1117 IO_DEVICE, 23 CRC)
and 87 stay read failures — the dying-disk storm keeps its semantics
(include/cr/cr_error.h:35-42; consumed at src/win/cr_transport.cpp:68/:74).

### 2.14 The `rda` namespace helpers — src/win/cr_transport.cpp:372-386

`compute_pid_token` (:374-378) and `control_device_path(service)` →
`L"\\\\.\\" + (service ? service : L"RDA")` (:380-384). The wire structs in
include/cr/win/cr_rda.h (DriverInfo :70-82, OpenTargetIn :92-105, the 19 session
codes :118-142, OpenSession/StartBackupIn/GetNextBlock :156-218) are
**documentation-only** — N-041 deleted the RdaClient with zero callers; the wire
mirror stays as the driver's documented protocol (include/cr/win/cr_rda.h:84-90,
:118-123).

### 2.15 Error-mapping table (GLE → ERR_*), consolidated

| Site | Win32 condition | ERR_* | Source |
|---|---|---|---|
| any open (grammar/direct/RW/volume/quiesce) | CreateFileW fails | ERR_OPEN_FAILED | cr_transport.cpp:491-494, :956-959 |
| any open | probe_geometry total_bytes == 0 | ERR_OPEN_FAILED | :497-501, :961-966 |
| read (both classes) | ReadFile/GetOverlappedResult fails, GLE ∈ {21,55,1110,1112,1619} | ERR_DEVICE_DISCONNECTED | :68, :74 |
| read | ReadFile/GetOverlappedResult fails, any other GLE | ERR_READ_FAILED (+gle_out) | :68, :74 |
| read | count mismatch (≠ len, ≠ len×512 when rda_scaled) | ERR_READ_FAILED | :77-81 |
| read | envelope out of bounds | ERR_RANGE_OUT_OF_BOUNDS | :198-199; device bounds :269-270, :336-337, :709-710, :728-729 |
| read | bounce issued_len > 1 MiB + 64 KiB | ERR_BAD_PARAMETER | :205-206 |
| read | envelope take != len | ERR_READ_FAILED | :215-216 |
| read/write | ERROR_INVALID_HANDLE after a read/write failure | one heal + retry (same code if still failing) | :273-277, :340-344, :716-721, :733-739 |
| write | WriteFile/GetOverlappedResult fails | ERR_WRITE_FAILED (write_gle_ recorded) | :855-868 |
| write | put ≠ len (direct) / ≠ len or len×512 (RDA) | ERR_WRITE_FAILED | :872-875 |
| write staging | _aligned_malloc fails | ERR_NO_MEMORY | :844-846 |
| query_driver_status | null out / null-check fail | ERR_BAD_PARAMETER | :412-413 |
| query_driver_status | any driver condition | OK (state reported in facets) | :407-410, :446-449 |
| lock_volume | volume CreateFileW or FSCTL_LOCK_VOLUME fails | ERR_VOLUME_LOCK_FAILED | :647-654 |
| S-029 chain | any DeviceIoControl fails | ERR_WRITE_FAILED | :755-757, :778-780, :795-797, :802-804 |
| refresh_disk_properties | 3× UPDATE_PROPERTIES fails | ERR_WRITE_FAILED (open: ERR_OPEN_FAILED) | :997-1009 |
| volume_extent(s)/destination_volume_letter | any failure | false (no ERR code — boolean surface) | :554-571, :1027-1045, :1049-1081 |

### 2.16 Threading / lifetime / buffering summary

- Per-handle `std::mutex heal_mu_` serializes healers; the winning reopen swaps the
  handle, losers' retries ride the fresh handle (comment :236-239). Per-thread
  bounce buffer for envelope reads (:209-210). Engine-pipe reads arrive serialized
  under the CachedReader mutex; per-handle single-flight is the ABI contract
  (:188-191).
- Lifetime: every device owns exactly its HANDLE; destructors CloseHandle
  (:262-265, :329-332, :603-610 — RwDevice's destructor additionally runs
  `release_locked_volumes_()` so held volume locks release, :603-607).
- Buffering: no transport-level cache (the cache is the CachedReader layer, §3);
  `FILE_FLAG_NO_BUFFERING` is NOT set on any core handle (the RDA path is a
  kernel IRP path; the direct control-path opens are plain buffered handle opens).
- IOCTLs issued by the transport: IOCTL_DISK_GET_LENGTH_INFO, 
  IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, IOCTL_STORAGE_QUERY_PROPERTY
  (StorageAccessAlignmentProperty), IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
  FSCTL_LOCK_VOLUME, FSCTL_IS_VOLUME_MOUNTED, FSCTL_DISMOUNT_VOLUME,
  IOCTL_VOLUME_OFFLINE, IOCTL_VOLUME_ONLINE, IOCTL_DISK_DELETE_DRIVE_LAYOUT
  (0x7C100), IOCTL_DISK_SET_DRIVE_LAYOUT (0x7C010), 0x7C058, 
  IOCTL_DISK_UPDATE_PROPERTIES (0x70140); on the control device:
  kIoctlGetInfo (0x6002440) and kIoctlGetVersion (0x6002448)
  (include/cr/win/cr_rda.h:50-57).
- What the grammar path ADDS over a plain handle: (1) the PID token in the path
  (auth handshake, not a security boundary — the device DACL is;
  docs/DRIVER-CONTRACT.md §3); (2) F11 Information=Length×512 compensation on
  every read AND write; (3) IOCTL pass-through to the bound target (the S-029
  chain rides it); (4) no driver-side alignment requirement on READ/WRITE
  (byte lengths, absolute byte offsets; MDL-locked user buffers —
  docs/DRIVER-CONTRACT.md §5) — which is why the direct-path staging rule in
  write_impl_ could be skipped for via_rda_.

---

## §3 The storage stack above the transport

**`IStorageDevice`** (include/cr/storage/cr_device.h:125-142): `read_at(offset,
buffer, length)` fills exactly `length` bytes or returns an error; bounds checked
(ERR_RANGE_OUT_OF_BOUNDS); safe from engine worker threads; `geometry()`,
`size()`, `absolute_base()` (0 for whole-device views — feeds session VolumeInfo
and destination-overlap math, :131-134), `is_static_media()` (N-072: images skip
robustness retries, :135-141). Engines never know the transport
(include/cr/storage/cr_device.h:8-14).

**`Geometry`** (include/cr/storage/cr_device.h:43-63): byte-anchored; 
`sector_count` is the N-206 CEIL; `valid()` checks the ceil window
((count-1)*ss < total ≤ count*ss); `is_4kn()`.

**The scan pipe** — `build_read_pipe(slice, 8 MiB, 256 KiB)`
(src/core/orchestrator/cr_scan.cpp:153-200; constructed at run(),
docs/CORE-DISK-ACCESS.md:45-47):
`slice -> SliceShim (non-owning) -> RetryingDevice{max_attempts=3, mode=Skip,
backtrack=true} -> CachedReader (8 MiB / 256 KiB)`. Production ALWAYS opts into
Skip (cr_scan.cpp:168-171) — one dead spot never aborts a scan. Degradation
contract (M-058, pinned): a CachedReader::create failure degrades to the BARE
SLICE — no cache AND no retry isolation, surfaced never hidden
(cr_scan.cpp:174-194).

**`RetryingDevice`** (src/core/storage/cr_retry.cpp:42-141):
- `try_read` retries only `ERR_READ_FAILED`/`ERR_RETRY_EXHAUSTED` — deterministic
  failures (bounds, bad parameter) pass through unretried (:47-50).
- **N-004 backtrack** (the og's CEXT4CatchBufferDeviceProxy salvage): one
  shifted-window second chance before bisecting — re-read
  `[off - max(0x400, sector), ...)` into a fresh heap buffer, keep the overlap
  (:83-99). Step = max(0x400, logical_sector) so the shifted request stays
  sector-aligned on 4Kn (the og's hardcoded 0x400 is misaligned there —
  include/cr/storage/cr_device.h:107-115).
- Bisection to sector granularity, depth budget 24 (:15-18, :100-101, :131-134).
- Permanent failure: recorded in `ReadErrorMap` with attempts + M-049 module
  attribution (thread stamp, else owner fallback, :108-114), Error-level log seam
  (:120-123), then Skip zero-fills and returns OK (:124-127) / Fail returns
  `ERR_RETRY_EXHAUSTED` (:128).
- Thread-safe: error map mutex-guarded; `error_map_copy()` is the G-06 race-free
  snapshot (include/cr/storage/cr_retry.h:50-54).

**`ReadErrorMap`** (src/core/storage/cr_devmap.cpp:14-30; include/cr/storage/cr_device.h:83-96):
sorted-disjoint run list; `add` push-then-normalize (sort + absorb overlapping/
touching runs, max attempts); the incoming range's module wins on coalesce
(newest-failure semantics, :22-29). Serialized into sessions as the geometric trio
(module not serialized — wire stays readable both directions,
include/cr/storage/cr_device.h:74-78). Travels with scan results and sessions
(R1.6); `ScanStats.bytes_unreadable` is its coverage — the read-truth gate
(DRIVER-REQUIRED.md:178-189).

**`CachedReader`** (src/core/storage/cr_cached.cpp:12-24; include/cr/storage/cr_cached.h:50-82):
reactive true-LRU, `cache_bytes` rounded down to `block_bytes`; REGISTERED
DIVERGENCE D-72: 8 MiB@scan-pipe vs the og's 4 MiB prefetch engine (og evidence
DRWUI_decompiled.c:919080-919092 — pinned by
tests/unit/test_cached.cpp:109 `ScanCacheBudgetIsTheRegisteredDivergence`). Failed
blocks are not cached forever (tests/unit/test_cached.cpp:68).

**`PartitionSlice`** (include/cr/storage/cr_slice.h:46-91; src/core/storage/cr_slice.cpp):
read-only byte window [start, start+len) over a parent; child offset 0 =
partition start; bounds in BYTES (fix F1); start must be parent-sector-aligned,
ragged END allowed (:26-29). M-119 tolerant ladder: create() clamps an over-long
DECLARED length to the device end (only the length — a start beyond the device
refuses honestly, N-046), read_at clamps a straddling read to the in-bounds
prefix and ZERO-FILLS the unread tail on success (:30-45; pinned by
tests/unit/test_slice_devmap.cpp:76, :315-363). `refresh_geometry()` re-derives
from the parent, REFUSING an invalidating change (:58-72; pinned :219-275).
`absolute_base()` feeds overlap math.

**`DecryptingDevice`** (include/cr/storage/cr_decrypt_device.h:44-77): opt-in
AES-XTS wrap over the volume slice; tweak = LE sector index from the ABSOLUTE
inner offset / logical_sector (BitLocker convention); unaligned reads decrypt
whole spanned sectors and copy out; FVEK 32/64 bytes (M-033/F13/S61).

**`storage::open_image`** (include/cr/storage/cr_image.h:60-109;
src/core/storage/cr_image.cpp): extension dispatch (.vmdk/.vhd/.vhdx with magic
verification — ERR_BAD_MAGIC never a silent flat fallback), flat fallback for
.img/.dmg/.raw/unknown, .edisk refused ERR_UNSUPPORTED; FlatImageDevice serves
the N-206 driver-padded tail (zero-filled reads past EOF within the padded
sector, src/core/storage/cr_image.cpp:156-185) — the "driver-padded" semantics
the direct replacement must also honor through its own geometry ceil.
`create_image_stream` (S-040): 64 KiB stride ascending reads, SHA-256 over the
emitted stream, cancel per chunk, truth contract = committed prefix
(include/cr/storage/cr_image.h:111-150).

**Error flow up to engines**: transport ERR_READ_FAILED → retry (3×) → backtrack
→ bisect → ReadErrorMap + Skip zero-fill (production) → engines see OK with
zeroed spots; ERR_DEVICE_DISCONNECTED is NOT retried by try_read (not in the
retry set, cr_retry.cpp:49) — it propagates up as a distinct code for hosts to
drive the reattach-then-resume flow (session checkpoints); the engine retry
window stays a registered difference (PARITY-DELTAS D-121;
include/cr/win/cr_transport.h:154-166).

---

## §4 The driver-status surface

**Facets** (include/cr/win/cr_driver_status.h:64-78): `service_present`,
`service_running`, `device_openable`, `authorized`, `info_valid`, `major`/`minor`,
`flags`, `max_disks`, `ioctl_base`, `last_gle`.

**Mechanism per facet** (query_driver_status, src/win/cr_transport.cpp:392-481):
1. present/loaded: `OpenSCManagerW(CONNECT)` + `OpenServiceW(QUERY_STATUS)` +
   `QueryServiceStatus` → `service_running = dwCurrentState == SERVICE_RUNNING`
   (:421-438). SCM failures are themselves diagnostic (last_gle; 1060 =
   ERROR_SERVICE_DOES_NOT_EXIST = absent, :433).
2. started/authorized: `CreateFileW(\\.\RDA, GENERIC_READ, R|W)` — the exact
   production posture the deleted `rda_available` probe used, minus the boolean
   collapse (:440-452). Open failure → last_gle + return OK (classified upstream,
   :446-449).
3. healthy/version: `kIoctlGetInfo` (0x6002440, 24-byte DriverInfo) with the
   `br >= sizeof && struct_size == sizeof` gate, then `kIoctlGetVersion`
   (0x6002448, u64 (major<<32)|minor) CROSS-CHECKED against GET_INFO — both must
   agree for `info_valid` (:453-478). FB-T06 contract: GET_INFO failure on an
   OPENED device = wrong/ancient driver → info_valid false → Errored.
Every handle is closed before return; the query retains no state; thread-safe
under its own handles (:407-410; include/cr/win/cr_transport.h:42-52).

**The classifier** (include/cr/win/cr_driver_status.h:91-104) — PURE,
first-match, fail-fast:
```
!present -> Absent; !running -> NotStarted;
!openable -> (last_gle==5 ? Unauthorized : Errored);
!info_valid -> Errored; major != kDriverWireMajorRequired(1) -> VersionMismatch; else Ready
```
`driver_state_error` maps 1:1 to the ERR_DRIVER_* family (0x500B0001..0x500B0005,
include/cr/cr_error.h:112-120; mapping at cr_driver_status.h:107-123);
`driver_state_name` is total (never an unhandled arm, :126-142). Zero-init
safety: a default DriverStatus classifies Absent, never a false Ready
(cr_driver_status.h:76-78; pinned tests/unit/test_driver_status.cpp:153-160).

**Exposure**: CLI `rrecov driver [--service NAME] [--json]`
(src/app/rrecov.cpp:3885-3942; exit 0 Ready / 4 not-Ready, :3921, :3940-3941);
ABI `cr_ui_driver_status(cr_ui_driver_status_t*)` (src/ui/cr_ui.cpp:1679-1722,
export at :1885-1887; struct + state enum at include/cr/cr_ui.h:343-396; Linux
stub honestly returns CR_UI_ERR_UNSUPPORTED, src/ui/cr_ui.cpp:1758-1766).

---

## §5 Every caller and the contract it expects

| Caller | Calls | Contract expected |
|---|---|---|
| `require_driver()` (rrecov.cpp:525-574) | `query_driver_status` + `driver_state` + `driver_state_error` | Query returns OK with the state in facets; non-Ready → the exact 2-line stderr + `[err 0x…]` + **exit 4**, before ANY other output (DRIVER-REQUIRED.md:106-131). Runs in scan (:1338), recover (:2454), archive-list (:2271), image (:2887), bench (:2982), apply (:3218), fix (:3608), driver's own tail (:3940). `--image` targets skip the gate (argv_has_image_target, :579-584). |
| `open_disk_view` (rrecov.cpp:586-617) | `query_driver_status` (in-path truth layer / TOCTOU window) + `open_rda_partition(L"RDA", n, 0)` | Driver Ready → grammar open; failure prints the GLE + elevation-specific guidance via `is_elevated()` and returns false — a TARGET error, never a silent switch (:603-616). |
| `open_volume_view` (rrecov.cpp:688-810) | bitlocker status/unlock/dict → `volume_extent` → `open_disk_view(disk)` → `PartitionSlice::create` → optional `DecryptingDevice` | Volume = slice on the parent disk; the parent view stays alive (out.hold, :797-799); BitLocker refusals (F18) come BEFORE any open (:697-763). |
| `scan_repair_open_writer` (rrecov.cpp:388-408) | `open_disk_rw(c->disk, L"RDA", &e)` | Null writer = "repair unavailable this scan" (orchestrator contract cr_scan.h:212-216); failure prints elevation guidance (:396-401); the writer is UNMAPPED (the orchestrator rebases onto the parent basis, A-002, :403-405). |
| LDM resolver, CLI (rrecov.cpp:1746-1779) | `enumerate_disks(32)` + `open_disk(pd.number, L"RDA", &oerr)` per member | A member that will not open simply does not resolve (continue, :1757-1759); reads LBA6 PRIVHEAD through its own retry+cache pipe (M-148, :1768-1770). |
| `cmd_bench` (rrecov.cpp:3037-3129) | `open_disk(disk, L"RDA", &err)` (mode open: N× `open_disk`, :3101) | Transport label constant "rda" (:3045, :3034-3035); `bench info` prints geometry + is_4kn from `dev->geometry()` (:3044-3067); `bench read` reads one sector at LBA0 aligned via `_aligned_malloc` (:3070-3091); seq/rand use aligned buffers of 4 KiB..16 MiB (:3116-3129). |
| `cmd_apply` (rrecov.cpp:3218-3425) | `open_disk(disk, L"RDA")` for the candidate re-validation view (:3284) + `open_disk_apply_rw(disk, L"RDA")` for the transactional write (:3417) | The read view and the write view are SEPARATE opens; apply IOCTL chain + quiesce ride the RW device inside the transaction; failure → exit 2 with guidance (:3418-3424). |
| `cmd_fix` (rrecov.cpp:3608-3860) | `open_disk_rw` for undo (:3720) and execute (:3846); `open_disk` for analyze (:3747) | Read view for planning, RW view for writes; undo is journaled (inflight_begin/end with the `\\.\PhysicalDriveN` identity string, :3733). |
| `cmd_image` (rrecov.cpp:2883-2982) | `require_driver` then `create_disk_image` → `open_disk` (src/win/cr_image_create.cpp:178) | Classification (a): the stream reads the disk data path; 64 KiB envelope stride; truth contract = committed prefix + `.partial` on cancel/failure (cr_image.h:111-150). |
| `cmd_driver` (rrecov.cpp:3885-3942) | `query_driver_status(service)` | The diagnostic itself never refuses on driver state; JSON/text per facet; exit 0/4. |
| health (rrecov.cpp:2765-2785) | own `CreateFileW(\\.\PhysicalDrive%u, GENERIC_READ|GENERIC_WRITE, R|W)` | Classified (b): NEVER the driver; elevation guidance on failure. |
| `open_disk_impl` (cr_ui.cpp:1425-1461) | `query_driver_status` → `open_rda_partition(kDefault, disk, 0)` | Non-Ready → the ERR_DRIVER_* code is BOTH the return value and `g_last_transport_error` (:1436-1442); Ready-but-target-fails → target error, never a switch (:1455-1460); transport string "rda" (:1450). |
| `open_volume_impl` (cr_ui.cpp:1463-1539) | bitlocker status/auto-unlock → `volume_extent` → `query_driver_status` → `open_rda_partition(disk, 0)` → `PartitionSlice::create(whole, start, len)` | Parent disk grammar open, slice kept alive on CrDevice.hold (:1530); M-119 clamp surfaces via the log seam (:1517-1525). |
| `open_volume_with_key_impl` (cr_ui.cpp:1547-1615) | same + `DecryptingDevice` wrap | The DecryptingDevice wrap survives on the RDA-opened parent slice (R6). |
| `driver_status_impl` (cr_ui.cpp:1679-1722) | `query_driver_status` | Maps the 6 states to CR_UI_DRIVER_STATE_*; fills every facet incl. required_major=1 (:1716); query failure is the only non-zero return. |
| `ui_repair_open_writer` (cr_ui.cpp:2087-2099) | `open_disk_rw(d->disk, kDefault)` | Null = repair unavailable, never guessed (:2098). |
| `arm_ldm_resolver` (cr_ui.cpp:2161-2200) | `enumerate_disks(ENUMERATE_DISK_BOUND)` + `open_disk(pd.number)` per member | Same resolve-or-skip contract as the CLI; the 200-disk bound (cr_winenv.h:45) because the driver CREATE grammar accepts disk# ≤ 199 (eprdtdrv_decompiled.c:184, comment :2167-2171). |
| `ui_apply_open_writer` (cr_ui.cpp:2742-2759) | `open_disk_apply_rw(d->disk, kDefault)` | The S-029 chain issues through the RDA bound handle (comment :2752-2754); images reopen r+b instead (:2743-2748). |
| `cr_ui_image_create` (cr_ui.cpp:1808+) | `query_driver_status` then the same `open_disk` read view | Class (a) refusal contract identical to open_disk_impl (comment :1782-1785). |
| enumeration (cr_enumerate.cpp:140-160, :168-215) | `open_physical_drive(n)` for table parse | Control-path only; view dropped after `parse_disk_table`; busy disks stay listed via the metadata-only open (N-071, include/cr/win/cr_enumerate.h:44-46). |
| orchestrator scan pipe (cr_scan.cpp:153-200) | consumes any IStorageDevice | Transport-agnostic: bounds + retry + cache; production Skip mode; ReadErrorMap surfaces. |

---

## §6 The elevation / access-control flow today

- **`is_elevated()`** (src/win/cr_elevate.cpp:12-21): `OpenProcessToken(TOKEN_QUERY)`
  → `GetTokenInformation(TokenElevation)` → `TokenIsElevated`. FALSE for standard
  users AND non-elevated admins (the distinction raw \\.\PhysicalDriveN access
  needs — the ELEVATED token, not group membership,
  include/cr/win/cr_elevate.h:5-8). Any probe failure → FALSE (the message stays
  honest: "requires elevation", :19-22). Used ONLY for error-message selection at
  open failures (rrecov.cpp:398, :613-615, :3040-3041, :3418-3423, :3720-3725,
  :3846-3851) — it never gates an attempt.
- **The real access gate is the driver's device DACL**: default SDDL
  `D:P(A;;GA;;;BA)(A;;GA;;;SY)` — Administrators + SYSTEM only
  (docs/DRIVER-CONTRACT.md §9). A non-elevated CreateFile on \\.\RDA fails
  ERROR_ACCESS_DENIED (5) → classified Unauthorized (cr_driver_status.h:96-98).
  The PID token is wire compatibility only, "NOT a security boundary"
  (include/cr/win/cr_rda.h:107-112; docs/DRIVER-CONTRACT.md §3).
- **`require_driver()`** is the CLI's only precondition gate (§5); there is no
  auto-start of the service (deliberate divergence: the installer owns the
  lifecycle, `sc start RDA` is the documented user action — DRIVER-REQUIRED.md:85-87).
- **The manifest**: the CLI/app embeds an admin manifest (requireAdministrator —
  the installer's own posture, installer/README.md:12); the elevation PROBE merely
  decides which guidance sentence prints.
- For the replacement: today user-mode NEVER opens \\.\PhysicalDriveN for data —
  the elevation story is "the driver's DACL + an elevated process". Direct access
  moves the gate to the raw-device object's own ACL (also admin-only) plus
  volume-handle semantics (GENERIC_READ on a mounted volume needs sharing
  cooperation; the S-010 chain exists precisely because direct writes to a
  mounted volume are unsafe without quiesce).

---

## §7 Tests, fixtures, benchmarks that pin this behavior

| Pin | What it pins |
|---|---|
| tests/unit/test_driver_status.cpp:35-166 (13 tests) | Every classifier state, the 8-step order (fail-fast first-match, :116-135), GLE routing 5 vs 87 (:64-89), GET_INFO-fails-on-opened-device → Errored (:91-97), minor-only divergence stays Ready (:108-114), zero-init → Absent (:153-160), required major == 1 (:162-166). |
| tests/unit/test_transport_logic.cpp:23-38 | The PE predicate (og FUN_1800577e0 decision input = the system-drive first byte). |
| tests/unit/test_transport_logic.cpp:40-54 | The quiesce chain hex constants (0x90018/0x90020/0x9001c/0x56c00c/0x56c008). |
| tests/unit/test_transport_logic.cpp:56-63 | ENUMERATE_DISK_BOUND == 200 (the driver grammar's disk# ≤ 199 space). |
| tests/unit/test_transport_logic.cpp:65-88 | The N-125 disconnect set + precision (1117/23/87 stay read failures) + the 0x50020007 code value. |
| tests/unit/test_directalign.cpp:15-139 (9 tests) | The 64 KiB envelope: fast path, 4K-not-64K takes the envelope, unaligned offset+length, multi-block coverage, device-tail clamp exception, 1 MiB writer chunk stays direct, out-of-bounds/empty fail, issued-length property, and `UnifiedEnvelopeExhaustivePropertyBothDevices` (:104) — BOTH transports, one envelope. |
| tests/unit/test_retry.cpp:53-272 (14 tests) | Pass-through, Fail-mode exhausted+map, Skip zero-fills only bad sectors, unaligned bad range rounds to whole sectors, backtrack salvage (:146), backtrack-off legacy (:165), backtrack alignment on 4Kn (:177), bounds before inner (:191), deterministic failures not retried (:202), geometry forwarded (:212), module attribution (:221-271). |
| tests/unit/test_cached.cpp:36-109 | Hit/miss/bridging, bounds reject, failed blocks not cached forever, and the D-72 budget pin (:109). |
| tests/unit/test_slice_devmap.cpp:36-363 | Slice bounds, M-119 tail clamp+zero-fill, start alignment, ragged tail, ReadErrorMap merge/serialize (module not serialized — wire trio), geometry refresh accept/refuse, create clamps overlong declared length, start-beyond-device refused, orchestrator ragged-table clamp. |
| tests/unit/test_cr_ui.cpp:3990-4008 | The ABI open path on Windows: driver-attached host → open succeeds + advisory resets to 0; driverless host → `ERR_DRIVER_ABSENT` (0x500B0001) return AND advisory; Linux stub → UNSUPPORTED + advisory 0. |
| tests/unit/test_cr_ui.cpp:4036-4100 | Doc-truth pins: no phantom error macros, the transport contract carries no "direct" value (:4044), ABI-CONTRACT carries the driver-era truth (:4036), wire codes free of the phantom zero (:4062), bench verify IOCTL codes match cr_rda.h (:4092). |
| tests/unit/test_image.cpp:163 | N-206: a truncated flat image CEILS its sector count (the padded-tail contract). |
| tests/unit/test_destio_win.cpp | The Win32 destination-writer surface (path-too-long vs unwritable GLE mapping — the M-074 family; N-015 ENOSPC). |
| tests/win-stub/windows.h:216-229, :358, :589-598; tests/win-stub/winioctl.h:111-158 | The stubbed Win32 surface: CreateFileW/ReadFile/WriteFile/GetOverlappedResult/GetSystemDirectoryA/SCM trio all parse-only (bodies never linked); the IOCTL numeric constants incl. 0x7405C GET_LENGTH_INFO, 0x700A0 GEOMETRY_EX, 0x2D1400 QUERY_PROPERTY, 0x560000 EXTENTS, 0x56C00C/8 OFFLINE/ONLINE, 0x70140 UPDATE_PROPERTIES. |
| tools/win_syntax_check.sh:50-83 + tests/CMakeLists.txt:218-225 | The Linux parse gate: every src/win, src/app, src/ui file compiled `-fsyntax-only` against the stubs (the M-055/N-228 "Windows-only breakage invisible on Linux" gate). |
| bench/ours/run-alignbench.ps1:14, :38, :104, :156/:193, :330-340 | The alignment bench: direct-handle chunk sweeps + the optional RDA grammar round-trip leg that VERIFIES the F11 unscale (`want = chunk * 512` when rdaScaled) and the direct-vs-driver comparison at the same chunk. |
| bench/ours/verify-driver.ps1:1-19, :47, :111 | The driver-attached CI proof: service install as `eprdtdrv`, GET_INFO/GET_VERSION codes byte-match cr_rda.h, rdacli info integration, PE version resource. |
| bench/ours/run-ours.ps1:142-144 | GAP-BENCH-5: with the rda service present the scan MUST have used \\.\RDA — a silent direct fallback would measure the wrong transport (asserted from the `[transport]` log lines). |
| docs/ABI-CONTRACT.md:94-98, :875-897 | The ABI-side pins: the 0x500B family in the normative error table and §7e's driver-era rewrite (the advisory reports the ONE leg there is). |
| Windows-runner staged scenario S-DRV-REFUSE (docs/PLAN-DRIVER-ONLY.md:223-228) | The real-driver matrix: Phase 1 absent → exit 4 + exact stderr; Phase 2 started → Ready; Phase 3 stopped → NOT_STARTED. |

Fixtures: tests/data/images/qemu-disk1.{vhd,vhdx,vmdk} + SHA256SUMS (the image
containers); tests/data/images/junk-gate-fat16.raw; the win-stub headers are the
fixture for the parse gate.

---

## §8 The HISTORICAL direct path (the seed evidence, from e58bde10~1 = 0f26ed23)

`e58bde10` ("feat(driver-only)!") removed a TESTED direct-access fallback. The
pre-removal file is 1027 lines; every removed element below is cited
`e58bde10~1:src/win/cr_transport.cpp:line` (extracted via
`git show e58bde10~1:src/win/cr_transport.cpp`).

### 8.1 `open_harddisk_volume(volume, err)` — REMOVED, never re-added

e58bde10~1:src/win/cr_transport.cpp:435-439:
```cpp
std::unique_ptr<IStorageDevice> open_harddisk_volume(u32 volume, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\HarddiskVolume%u", volume);
    return open_path(path, err);
}
```
Declared at e58bde10~1:include/cr/win/cr_transport.h:44. This is the exact
template for a Volume-GUID-style direct volume open (substitute the
`\\.\Volume{GUID}` path for `\\.\HarddiskVolume%u`).

### 8.2 `open_path` / `open_physical_drive` / `DirectDevice` — the direct read path

- `open_path` (e58bde10~1:src/win/cr_transport.cpp:276-306): CreateFileW GENERIC_READ
  share R|W|D → `probe_geometry` → exotic-handle `GetFileSizeEx` fallback (512/512)
  → `DirectDevice`. **Identical to the surviving code** (current :292-317) — only
  the CALLER set changed.
- `DirectDevice::read_at` (e58bde10~1:242-274): bounds →
  `aligned_read_at(..., rda_scaled=false, ...)` → ERROR_INVALID_HANDLE heal
  (reopen_handle on the same path). Same in the current tree (:258-290).
- `open_physical_drive` (e58bde10~1:429-433) = `open_path("\\\\.\\PhysicalDrive%u")`
  — SURVIVES today (:542-546) but demoted to the control path.

### 8.3 `rda_available(service, last_error)` — the deleted presence probe

e58bde10~1:src/win/cr_transport.cpp:441-457: bare `CreateFileW(\\.\SVC,
GENERIC_READ, R|W)`; failure → `*last_error = GLE`, false; success → close, true.
Deleted by R11 ("a boolean view of a five-state world is the silent-degradation
primitive itself", docs/PLAN-DRIVER-ONLY.md:168). Its exact posture survives as
facets 2-3 of query_driver_status (current :440-452).

### 8.4 `win_pe_environment()` — the deleted environment gate

e58bde10~1:src/win/cr_transport.cpp:466-473: `GetSystemDirectoryA` into 8 bytes,
decide on the FIRST BYTE ('X'/'x' = WinPE, the og's FUN_1800577e0 probe;
comment :458-465 cites Device_decompiled.c:50943-50992 and the four corpus
callers :352/:7816/:50653/:52437). Deleted by R12; the pure predicate
`pe_environment_by_system_drive` survives in include/cr/win/cr_winenv.h:36-38 as
the corpus pin (its own header comment :30-35 documents the removal).

### 8.5 The direct legs of `open_disk` / `open_rw_device` — the removed decision points

- `open_disk(disk, prefer_rda, service, err)` — e58bde10~1:src/win/cr_transport.cpp:498-516:
  ```cpp
  if (prefer_rda && !win_pe_environment()) {
      u32 e = 0;
      if (auto dev = open_rda_partition(service, disk, 0, &e))
          return dev;
  }
  return open_physical_drive(disk, err);
  ```
  (declared e58bde10~1:include/cr/win/cr_transport.h:47). Three-way decision:
  caller preference + environment gate + failure fallback.
- `open_rw_device(disk, prefer_rda, service, err)` — e58bde10~1:src/win/cr_transport.cpp:872-914:
  RDA RW grammar first (`if (prefer_rda && service != nullptr)`, :873-890), then
  the DIRECT TAIL: `CreateFileW("\\\\.\\PhysicalDrive%u", GENERIC_READ|GENERIC_WRITE,
  FILE_SHARE_READ|FILE_SHARE_WRITE)` (:893-897), probe_geometry, zero-size →
  close + ERR_OPEN_FAILED (:898-908), success → `RwDevice(..., via_rda_=false, ...)`
  (:909-913). NOTE the share mode difference: the direct RW tail used share R|W
  (no DELETE), while the grammar RW open used R|W|DELETE (:878-880).
- `open_disk_rw`/`open_disk_apply_rw` carried the extra `bool prefer_rda` parameter
  (e58bde10~1:src/win/cr_transport.cpp:917-925).

### 8.6 The direct-write alignment rule (the replacement's hardest constraint)

`RwDevice::write_impl_` (e58bde10~1:src/win/cr_transport.cpp:766-805): on the
direct path (`!via_rda_`) an unaligned caller buffer is staged through
`_aligned_malloc(len, ssec)` + memcpy before WriteFile; success requires
`put == len` exactly (no F11 tolerance) (:797-800, :800-803). This code SURVIVES
verbatim in the current tree (src/win/cr_transport.cpp:838-877) but is currently
DEAD (`via_rda_` is always true — open_rw_device only opens the grammar,
:970-972). **It is the proven direct-write staging the replacement reactivates.**

### 8.7 The historical caller posture (removed by e58bde10)

- CLI `open_disk_view` (e58bde10~1:src/app/rrecov.cpp:313-341): `rda_available`
  → grammar → on failure print `[transport] RDA open failed (%hs) - direct path`
  → `open_physical_drive`; probe failure → print the CreateFile GLE → direct.
- CLI bench `--no-rda` (e58bde10~1:src/app/rrecov.cpp:1758, :1791-1792, :1810-1820):
  `use_rda` cleared the preference; transport label `"rda"`/`"direct"`.
- ABI `open_disk_impl` (e58bde10~1:src/ui/cr_ui.cpp, the pre-removal form):
  `!win_pe_environment() && rda_available(...)` → grammar → direct fallback with
  the M-030 error preservation (`rda_err` kept when the direct leg had nothing
  more specific; `transport = "direct"` on the surviving leg).
- The LDM resolvers and `scan_repair_open_writer` carried two-step
  open-retry-with-`prefer_rda=false` (docs/PLAN-DRIVER-ONLY.md:164-166; the
  CORE-DISK-ACCESS census rows F1-F12, docs/CORE-DISK-ACCESS.md:181-204).

### 8.8 What the historical tree proves for the replacement

The direct DATA path was fully implemented and tested through the SAME
`open_path`/`DirectDevice`/`aligned_read_at` machinery that survives today for
the control path — i.e. the replacement's read side is a RE-CALLER of existing
proven code, not a new implementation. The write side's staging rule (§8.6) also
survives, dead. The deltas the removal introduced that a replacement must now
consciously re-decide: the ERR_DRIVER_* refusal contract (keep: elevation/
admin-privilege failures must stay classified, not silent), the transport-string
surface ("rda" | "image" — cr_ui.h:439-444 forbids "direct" from returning), and
the F11 compensation (drop when direct).

---

## §9 The installer / build / driver-artifact touchpoints

- **installer/core-recovery.iss:53** `#define DriverService "RDA"`; :93-101 the
  `driver` component (Full install = engine + kernel driver; checkable-alone);
  :109-113 payload `driver\{rda.sys, rda.inf, install.ps1, uninstall.ps1,
  SHA256SUMS}`; :194-206 the post-install step runs
  `install.ps1 -ServiceName RDA -Source {app}\driver\rda.sys` ELEVATED and a
  failed driver step FAILS the whole app install (the M-062 never-exit-0 lesson);
  :217-229 the uninstall-time service teardown (a reported failure is a MsgBox,
  never silent).
- **installer/build-installer.ps1:25, :44-46, :70-76**: stages a FETCHED AND
  VERIFIED rda-driver release directory (fetch via fetch-release.ps1 -Repo
  heicfast/rda-driver); a missing asset is a hard failure.
- **Driver release contract** (docs/DRIVER-RELEASE.md:32-66): assets `rda.sys`,
  `rdacli.exe`, `rda.inf`, `SHA256SUMS`, `install.ps1`, `uninstall.ps1`,
  `run-integration.ps1`; `sc create <service> type= kernel start= demand` +
  registry Parameters + `sc start`; WinPE media register via
  `scripts/winpe/regdrv-rda.reg` (DRIVER-REQUIRED.md:133-145). Signing posture:
  unsigned by design; retail Win10/11 needs test-signing or attestation
  (DRIVER-RELEASE.md:66).
- **Build wiring**: src/win/CMakeLists.txt:3,5,11 compile cr_transport.cpp,
  cr_image_win.cpp, cr_elevate.cpp into the Windows platform layer; the Linux
  lane never links them (parse-only via tools/win_syntax_check.sh).
- **CI**: the driver-attached lane installs service `rda` → \\.\RDA
  (docs/DRIVER-CONTRACT.md:26); bench/ours/verify-driver.ps1 installs the alias
  service `eprdtdrv` and proves GET_INFO/GET_VERSION/rdacli info
  (bench/ours/verify-driver.ps1:111); the S-DRV-REFUSE staged scenario proves the
  refusal matrix (docs/PLAN-DRIVER-ONLY.md:223-228).
- **Wire-truth pin**: the extended IOCTL codes were corrected 0x6000244x →
  0x600244x (c169332e; the classifier caught the mistyped codes as Errored —
  include/cr/win/cr_rda.h:43-49). test_cr_ui.cpp:4062/:4092 pin the docs and the
  bench to the corrected values.

---

## §10 Open questions / risks for the replacement

1. **The refusal taxonomy must survive as SOMETHING.** The ERR_DRIVER_* family,
   exit code 4, the `cr_ui_driver_status` surface, and the "refusal is the whole
   response" CLI contract (DRIVER-REQUIRED.md:106-131) are pinned by tests and
   ABI docs. A driverless direct build must decide: does the family become
   ERR-driver-absent-only-when-privileged-open-fails, or is the surface retired
   with an ABI-compatible stub that always reports Ready? `cr_ui_open_disk`'s
   contract text (cr_ui.h:319-331) says REFUSES with ERR_DRIVER_* — the header,
   ABI-CONTRACT §7e (docs/ABI-CONTRACT.md:875-897), and test_cr_ui.cpp:4036 all
   pin the current story; every one of them moves with the replacement.
2. **F11 compensation must be keyed off cleanly.** `overlapped_read`'s
   `rda_scaled` flag and `write_impl_`'s `put == len*512` acceptance
   (cr_transport.cpp:79-80, :872-873) are the ONLY RDA-specific data-path
   semantics; direct handles return true counts (the historical DirectDevice
   proved it, e58bde10~1:250-260). Removing the flag also changes
   run-alignbench.ps1's `want = chunk * 512` RDA leg (:156, :193).
3. **The 64 KiB envelope + sector-alignment rules are transport-mandatory for
   direct handles.** The GAP-S6-13 slow path and the sector-rejection evidence
   (cr_transport.cpp:158-177) apply to DIRECT reads specifically; the envelope is
   already unified on both transports and MUST stay. The direct WRITE path
   additionally needs the §8.6 staging rule (buffer alignment) — note the
   historical direct RW tail also lacked FILE_SHARE_DELETE (§8.5) while the
   grammar open had it; the replacement should keep R|W|D for reads and decide
   the write-tail share deliberately (a mounted volume refuses share-delete
   writes).
4. **Volume targeting semantics.** The grammar's `\<token>\<disk>\<part>` opens
   the DISK (Partition0), part number unused (DRIVER-CONTRACT.md §3) — every
   volume view today is volume_extent → whole-disk open → PartitionSlice
   (cr_ui.cpp:1491-1516; rrecov.cpp:772-786). A \\.\\<Volume GUID> replacement
   changes the GEOMETRY SOURCE: volume handles may not forward
   IOCTL_DISK_GET_DRIVE_GEOMETRY_EX (the 512/512 fallback exists for exactly
   that, cr_transport.cpp:140-148) and their lengths come from
   IOCTL_DISK_GET_LENGTH_INFO — the slice-bounding math (M-119 clamp) and the
   N-206 ceil window must be re-proven on GUID handles.
5. **The S-029 IOCTL chain + refresh must move to the direct handle.** They
   currently ride the driver's pass-through (cr_transport.h:112-120;
   DRIVER-CONTRACT.md §4.3). refresh_disk_properties (cr_transport.cpp:993-1009)
   is the in-tree proof that 0x7C100/0x7C010/0x7C058/0x70140 work on a direct
   \\.\PhysicalDriveN RW handle. METHOD_NEITHER IOCTLs would be rejected by the
   driver but are the raw device's own contract — none of the four use it.
6. **The S-010 quiesce + M-030 offline/restore chain is transport-independent
   (it already uses direct \\.\X: volume handles) but is REQUIRED for direct
   writes** — the driver's bound-handle writes bypassed the volume stack's
   mounted-state concerns at the disk level; direct writes to a mounted disk
   without LOCK→DISMOUNT→OFFLINE are the corruption risk the chain exists for
   (cr_transport.cpp:612-632).
7. **Authorization observability.** Today a non-elevated caller gets the
   classified Unauthorized from the DACL denial on \\.\RDA. Direct
   \\.\PhysicalDriveN access denials (ERROR_ACCESS_DENIED / ERROR_SHARING_VIOLATION)
   must map into an equally honest classification — the historical tree collapsed
   everything into "probe failed → direct" and that silence was the registered
   product failure (PLAN-DRIVER-ONLY.md:20-22). The `is_elevated()` guidance
   selection (§6) is the current mitigation and stays relevant.
8. **Bench/test surface churn**: run-ours.ps1:142-144 asserts the transport label
   came from \\.\RDA when the service runs; test_cr_ui.cpp:3990-4008's
   driver-attached branch; verify-driver.ps1; the win-stub parse gate (a new SDK
   symbol in src/win grows the stub — tools/win_syntax_check.sh's contract).
   cr_ui.cpp:1874-1880's own comment still says "surviving transport leg"
   (stale vs ABI-CONTRACT §7e) — fix or carry forward knowingly.
9. **The Win7-SP1 import posture**: cr_transport.cpp imports are gated by the
   Win7 allowlist (3b203fe1 fixed PathIsNetworkPathW at cr_transport.cpp:1053;
   cc6312ca the kernel32 consumers). Any new direct-path API (e.g. GetVolumeNameForVolumeMountPointW
   is already used at :665) must stay Win7-SP1-available or join the registered
   baseline.
10. **The driver-padded tail (N-206)**: the ceil window means reads may be issued
    past EOF within the final partial sector and are expected to succeed
    zero-filled through the driver (cr_image.cpp:169-185 mirrors it for images).
    A direct handle READ past EOF fails; the replacement must reproduce the
    zero-fill semantics in its own envelope/tail handling or the
    TruncatedFlatImageCeilsSectorCount-class guarantees shift for real disks.
11. **Session-protocol surface is unused but documented**: no core code issues
    the 19 session codes (cr_rda.h:118-142); nothing to migrate, but the wire
    structs document capabilities (exclusive sector ranges, TRIM swallowing while
    a session is active — DRIVER-CONTRACT.md §7) whose closest direct-mode
    equivalents are the S-010 quiesce; confirm no bench depends on them
    (run-alignbench.ps1 uses plain ReadFile only).
