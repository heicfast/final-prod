# ACCESS-LAYER-CURRENT — the DIRECT access layer as it exists today (session-97 perf, Task T2-b / Agent B)

> READ-ONLY excavation of `core_recovery` at HEAD **e6ee97da** (main; parents
> `42971fb6` v0.7.0 release notes, `04f8869f` the probe fix). Date: 2026-11-05.
> Every claim cites `path:line` at that tree; inferences are labeled
> "inferred:". No source file was modified; this file is the session's only
> output for this task.
>
> Companion sheets: `worklog/ACCESS-LAYER.md` (Agent A, the DRIVER-era
> contract, written at tree `b6418420` — PRE-removal; the transport it
> documents no longer exists), `worklog/CONSUMERS.md` (Agent B, the driver-era
> consumer census at `619acec2`), `worklog/DRIVER-CONTRACT.md` (Agent C, the
> rda-driver wire surface), `worklog/PERFORMANCE.md` (the measurement ledger —
> §2 still "TO FILL"), `worklog/session-performance-parity-prompt.md` (the
> session-97 mandate). The driver-era grammar path
> (`\\.\SVC\<token>\<disk>\<part>`), `query_driver_status`, the ERR_DRIVER_*
> family and F11 `Information=Length*512` scaling were REMOVED in
> be90de5d..04f8869f (v0.7.0) — none of them exist at HEAD (verified: zero
> hits for `open_rda_partition`/`query_driver_status`/`rda_scaled` in
> src/win/cr_transport.cpp, which is 863 lines, down from 1086).

---

## §1 The layer map — every file performing raw disk access today

| File (lines at HEAD) | Role (one line) |
|---|---|
| `src/win/cr_transport.cpp` (863) | THE transport: `overlapped_read` (sync QD1 ReadFile), `probe_geometry` (3 IOCTLs), `aligned_read_at` (the 64 KiB envelope + thread-local bounce), `DirectDevice` (the data-path device), `RwDevice` (repair/apply writes + S-010 quiesce + S-029 IOCTL chain), `open_disk`/`open_physical_drive`/`open_disk_rw`/`open_disk_apply_rw`/`refresh_disk_properties`, destination arming (`volume_extent(s)`, `destination_volume_letter`) — all on `\\.\PhysicalDriveN`. |
| `include/cr/win/cr_transport.h` (170) | The public surface: the privilege-precondition contract (:45-53), every open signature (:60-103), VolExtent/N-013 (:105-134), the N-125 `disconnect` GLE set (:136-165). |
| `include/cr/storage/cr_directalign.h` (78) | Pure 64 KiB envelope math: `kDirectAlignBytes` (:33), `DirectEnvelope` (:37-46), `direct_envelope()` (:48-73). |
| `include/cr/win/cr_elevate.h` (28) + `src/win/cr_elevate.cpp` (24) | `is_elevated()` — TokenElevation probe (cr_elevate.cpp:12-21); false on any probe failure (:20). |
| `src/core/storage/cr_slice.cpp` (104) + `include/cr/storage/cr_slice.h` (118) | `PartitionSlice` byte-bounded read-only window over the parent device (cr_slice.cpp:16-83); M-119 clamp+zero-fill (:38-46, :72-79). |
| `src/core/storage/cr_retry.cpp` (144) | `RetryingDevice`: 3× retry (only ERR_READ_FAILED/RETRY_EXHAUSTED, :49), N-004 backtrack (:83-99), bisect to sector (:100-141), ReadErrorMap + Skip zero-fill (:111-127). |
| `src/core/storage/cr_cached.cpp` (99) | `CachedReader`: reactive LRU, **one mutex spanning the inner transport call** (:31), 256 KiB block fills (:52-68), LRU move-to-front (:76-82). |
| `src/core/storage/cr_devmap.cpp` (106) | `ReadErrorMap` sorted-disjoint run list, M-049 newest-module-wins merge (:15-44). |
| `src/core/storage/cr_decrypt_device.cpp` (76) | `DecryptingDevice`: opt-in AES-XTS wrap; per-read heap staging + per-sector decrypt (:57-72). |
| `src/core/storage/cr_image.cpp` (1613) | Image READ side (flat/VHD/VHDX/VMDK devices) + `create_image_stream` — the S-040 producer loop, 64 KiB chunks (:1553-1610). |
| `src/win/cr_image_win.cpp` (119) | `WinImageFileIo` (`image_file_io()`): CreateFileW-backed image handle table; reads serialized under `mu_` (:61-84). |
| `src/win/cr_image_create.cpp` (185) | S-040 flat-image producer: `create_disk_image` → `open_disk` (:178), FileSink temp+rename atomic commit (:122-172). |
| `src/win/cr_enumerate.cpp` (257) | Control-path enumeration: numbered `\\.\PhysicalDriveN` loop + cfgmgr32 interface pass, table parse through `open_physical_drive` (:148, :201); busy-disk 0-access retry open (:98-113). |
| `src/win/cr_health.cpp` (346) | SMART/NVMe/CSMI: its OWN `\\.\PhysicalDriveN` handle (GENERIC_READ\|WRITE, share R\|W — rrecov.cpp:2725-2726), ATA/SCSI passthrough IOCTLs (cr_health.cpp:59, :105, :164-197, :290, :337). Never the transport. |
| `src/win/cr_destio.cpp` (225) | `WinDestIo`: destination writer (temp+atomic rename), synchronous OVERLAPPED WriteFile per chunk (:54-75), ENOSDC classification (:46-48, :69-70). |
| `src/win/cr_bitlocker.cpp` (575) | Volume-letter fveapi state/unlock — no raw-disk data path. |
| `src/win/cr_bootfix.cpp` (311) | Registry-hive file I/O (`open_hive` CreateFileW on hive FILES, :126-139) — no raw disk. |
| `src/win/cr_fsck.cpp` (121) | chkdsk launcher (volume letter, Wow64 redirection) — no raw disk. |
| `src/win/cr_mountpoints.cpp` (91) | HKCU MountPoints2\CPC\Volume history walk (M-118 register channel) — registry only. |
| `src/core/orchestrator/cr_scan.cpp` (3192) | `build_read_pipe` (:164-203) — the pipe every engine reads through; the scan staging, probe, table flow, discovery, carve boundaries, cancellation checkpoints. |
| `src/app/rrecov.cpp` (3953) | The CLI: `require_privilege` (:526-537), `open_disk_view` (:549-573), `open_volume_view` (:644+), the verbs (§3), the bench verb (:2930-3130). |
| `src/ui/cr_ui.cpp` (5081) | The C ABI over the same layer: `open_disk_impl` (:1424-1454, transport "direct" :1445), `open_volume_impl` (:1456+), `scan_run_impl` (:2316-2404), recovery pipes (:2100, :2159, :4485), repair writer (:1999), apply writer (:2663), `image_create_impl` (:1718-1750). |
| `.github/scripts/api_harness.c` | The ABI bench harness: reads `cr_ui_device_transport` into api-result.json `transport` (:240-242, :494). |

Layering (data path, top→bottom, unchanged in shape from the driver era):

```
engines (ntfs/fat/exfat/ext4/hfsplus/btrfs/zfs/refs/lost/carve/check/repair/apply)
  -> CachedReader (8 MiB / 256 KiB, reactive LRU)         cr_scan.cpp:1298
  -> RetryingDevice (3x, Skip, backtrack)                  cr_scan.cpp:179-183
  -> SliceShim -> PartitionSlice (byte window)             cr_scan.cpp:182
  -> [DecryptingDevice, opt-in FVEK]                       cr_ui.cpp:1588+
  -> DirectDevice / RwDevice  ==  \\.\PhysicalDriveN, GENERIC_READ[|WRITE], share R|W|D
                                                            cr_transport.cpp:290-315, :729-752
  -> Windows storage stack (no kernel component of ours)
```

---

## §2 The exact I/O contract

### 2.1 Handle types and open flags (verified per site)

| Site | Path | Access | Share | Disposition/flags |
|---|---|---|---|---|
| `open_path` (data path: `open_disk`, `open_physical_drive`) cr_transport.cpp:290-293 | `\\.\PhysicalDriveN` | GENERIC_READ | R\|W\|D | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| `open_rw_device` (repair/apply) :729-734 | `\\.\PhysicalDriveN` | GENERIC_READ\|GENERIC_WRITE | R\|W\|D | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| `refresh_disk_properties` :771-774 | `\\.\PhysicalDriveN` | GENERIC_READ\|GENERIC_WRITE | R\|W (no DELETE) | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| `volume_extent` :333-338 | `\\.\X:` | GENERIC_READ | R\|W\|D | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| `volume_extents` (N-013) :797-804 | `\\.\X:` | **0** (metadata-only) | R\|W\|D | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| `destination_volume_letter` :837-838 | any path | 0 | R\|W\|D | OPEN_EXISTING, **FILE_FLAG_BACKUP_SEMANTICS** |
| S-010 quiesce per volume :428-430 | `\\.\X:` | GENERIC_READ\|WRITE | R\|W | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| M-030 restore :686-688 | `\??\Volume{...}\` | GENERIC_READ\|WRITE | R\|W | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
| health (own handle) rrecov.cpp:2725-2726 | `\\.\PhysicalDriveN` | GENERIC_READ\|WRITE | R\|W | OPEN_EXISTING, **flags 0** |
| enumeration query cr_enumerate.cpp:99-107 | `\\.\PhysicalDriveN` | GENERIC_READ, retry 0 | R\|W\|D | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |

**No core handle sets `FILE_FLAG_OVERLAPPED` and none sets
`FILE_FLAG_NO_BUFFERING`** — every open is `FILE_ATTRIBUTE_NORMAL`
(buffered semantics; on a raw disk handle the storage stack still requires
sector-aligned lengths, which is what the envelope provides).
ReadFileScatter/WriteFileGather are never used (verified: zero hits).

### 2.2 The read path

`DirectDevice::read_at` (cr_transport.cpp:264-277): bounds check
(:267-268) → `aligned_read_at(h_, off, buf, len, size(), &gle)` → on
ERR_READ_FAILED with `gle == ERROR_INVALID_HANDLE(6)`: heal under
`heal_mu_` (one `reopen_handle` on the same path/access/share, :271-275;
:237-249) and ONE retry. `RwDevice::read_at` is identical (:490-507).

`aligned_read_at` (:191-215) — THE 64 KiB envelope:
1. `io::direct_envelope(off, len, dev_size)` (cr_directalign.h:48-73):
   offset rounded DOWN, end rounded UP to 64 KiB multiples, clamped at the
   device end (the ragged-tail exception, :58-59); `skip`/`take` locate the
   request inside the envelope; `direct` = aligned offset AND 64-KiB-multiple
   length AND fully inside the device (:69-70).
2. `!e.ok` → `ERR_RANGE_OUT_OF_BOUNDS` (:195-196). `e.direct` →
   `overlapped_read` straight into the caller's buffer — **no copy**
   (:197-198).
3. Bounce path: `issued_len > 1 MiB + 64 KiB` → `ERR_BAD_PARAMETER`
   (:202-203); `thread_local std::vector<u8> bounce` (:206-207), read the
   envelope, `take != len` → ERR_READ_FAILED, else `memcpy` out (:211-213).

`overlapped_read` (:53-80) — the completion model:
- one stack `OVERLAPPED` per call with `Offset`/`OffsetHigh` (:56-58) —
  used ONLY for position (the handle is synchronous);
- `ReadFile` → immediate failure `!= ERROR_IO_PENDING` → GLE exported,
  N-125 classification: `disconnect::gle_is_disconnect(e)` {21,55,1110,
  1112,1619} → `ERR_DEVICE_DISCONNECTED`, else `ERR_READ_FAILED` (:60-69);
- `ERROR_IO_PENDING` → `GetOverlappedResult(..., TRUE)` — **blocking wait**
  (:70-75); same classification;
- success requires `got == len` EXACTLY (true byte counts — the F11
  `len*512` acceptance is deleted, :77-79; header comment :48-52).
- **Queue depth is 1 per handle, synchronous.** No ReadFileEx, no I/O
  completion ports, no cancellation handle in the OVERLAPPED
  (`ov.hEvent` is null). Inferred consequence: a stalled device blocks the
  reader thread indefinitely; cooperative cancellation cannot interrupt an
  in-flight read (engines poll at loop heads only — cr_device.h:144-155).

### 2.3 The write path (RwDevice)

`write_at` (:509-525): bounds → `write_impl_` → on ERR_WRITE_FAILED with
`write_gle_ == ERROR_INVALID_HANDLE`: heal + one retry (:517-523).
`write_impl_` (:622-659) — **the staged-write rule**: if the caller's buffer
is not logical-sector-aligned, `_aligned_malloc(len, ssec)` + memcpy
(:627-633, failure → ERR_NO_MEMORY :629-630); OVERLAPPED WriteFile
(:634-638); immediate failure `!= ERROR_IO_PENDING` or
`GetOverlappedResult` failure → `write_gle_` recorded + ERR_WRITE_FAILED
(:639-652); success requires `put == len` exactly (:656-657). Note the WRITE
path has NO length/offset envelope — the caller (writer chunking at 1 MiB,
cr_safewriter.h:158) is responsible for aligned extents (the read-side
bounce cap comment says exactly this, :199-201).

### 2.4 Geometry probing + sector size handling

`probe_geometry` (:94-150), run at every open:
1. `IOCTL_DISK_GET_LENGTH_INFO` — exact bytes, volume AND disk handles
   (:84-92);
2. `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` — BytesPerSector into logical AND
   physical (:100-104);
3. `IOCTL_STORAGE_QUERY_PROPERTY(StorageAccessAlignmentProperty)` — reads
   `BytesPerLogicalSector@+0x10` / `BytesPerPhysicalSector@+0x14` (the
   183dfcd3 fix; comment :111-120), sanity-gated to [256, 65536]
   (:124-127); failure → physical = logical (:128-129);
4. `total_bytes` = exact when available else `dg.DiskSize` (:131);
5. **N-206 CEIL**: `sector_count = (total_bytes + ss - 1) / ss` (:136-137);
   geometry-IOCTL-unavailable fallback: 512/512 + exact + ceil (:138-145).
`open_path`'s exotic-handle fallback: zero-size → `GetFileSizeEx` with
512/512 geometry and `total_bytes / 512` (floor, :300-308 — inferred: this
floor is a minor divergence from the N-206 ceil, reachable only for exotic
handles whose geometry IOCTLs fail).
`Geometry::valid()` enforces the ceil window (cr_device.h:49-60 per the
driver-era sheet; unchanged at HEAD).

### 2.5 Buffering choices

- Transport: no transport-level cache; **no `FILE_FLAG_NO_BUFFERING`**
  (§2.1). The CachedReader is the only cache (8 MiB / 256 KiB reactive LRU,
  cr_scan.cpp:1298).
- Bounce buffer: `thread_local` per thread (:206) — tolerates concurrent
  callers without a lock; grows to the max envelope ever requested.
- Destination writes: buffered FILE_ATTRIBUTE_NORMAL, per-item exclusive
  temp + atomic rename (cr_destio.cpp:40-41, :91-114); flush before rename
  (:95).

### 2.6 Per-request overhead sources (the honest list)

1. CachedReader mutex held across the ENTIRE read_at INCLUDING the inner
   transport call (cr_cached.cpp:31) — see §4-F1.
2. One LRU linear scan over ≤32 blocks per block touch (cr_cached.cpp:46-51)
   + move-to-front shift (:76-82).
3. One `memcpy` per served block range (cr_cached.cpp:87) — 256 KiB on the
   engine path.
4. ReadFile syscall per 256 KiB block (the block fill, cr_cached.cpp:61).
5. Unaligned/short reads (probe, table parse, boot hash, grading, LDM
   probes, bench rand): 64 KiB envelope read + memcpy of the small request
   (cr_transport.cpp:206-213) — read amplification up to 128× per probe
   read (see §4-F4).
6. RetryingDevice wrappers: pure passthrough on success (no lock taken on
   the OK path — the map mutex is only touched on permanent failure,
   cr_retry.cpp:111-114).
7. PartitionSlice: pure offset shift (cr_slice.cpp:82); memcpy only on the
   M-119 straddle tail (:75-79).
8. SliceShim: non-owning forward (cr_slice.h:98 per driver-era sheet).
9. DecryptingDevice (BitLocker only): a heap `std::vector` per read_at +
   per-sector XTS + final memcpy (cr_decrypt_device.cpp:57-72).
10. Handle heal: `heal_mu_` taken only on ERROR_INVALID_HANDLE
    (cr_transport.cpp:271-275, :502-505) — never on the OK path.

### 2.7 IOCTL inventory (the transport's own)

Reads/geometry: `IOCTL_DISK_GET_LENGTH_INFO`, `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX`,
`IOCTL_STORAGE_QUERY_PROPERTY(StorageAccessAlignment)`. Volume/extents:
`IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` (:344, :809). Quiesce (S-010/M-030):
`FSCTL_LOCK_VOLUME`, `FSCTL_IS_VOLUME_MOUNTED`, `FSCTL_DISMOUNT_VOLUME`,
`IOCTL_VOLUME_OFFLINE` (0x56c00c) / `IOCTL_VOLUME_ONLINE` (0x56c008)
(:434-476, :691). Apply (S-029): `IOCTL_DISK_DELETE_DRIVE_LAYOUT` (0x7C100,
:539), `IOCTL_DISK_SET_DRIVE_LAYOUT` (0x7C010, :562), 0x7C058 partition-info
record (:579), `IOCTL_DISK_UPDATE_PROPERTIES` (0x70140, :586, :779). Plus
`DeleteVolumeMountPointW`/`SetVolumeMountPointW`,
`GetVolumeNameForVolumeMountPointW`, `FlushFileBuffers`, `Sleep`s (§4-F9).
Enumeration adds `IOCTL_STORAGE_GET_DEVICE_NUMBER`
(cr_enumerate.cpp:189).

---

## §3 The per-caller request pattern table (all 27 consumers, re-verified at HEAD)

Legend — entry points at HEAD: **OD** = `open_disk` (whole-disk direct
read view); **OPD** = `open_physical_drive` (control path);
**ODRW/ODARW** = `open_disk_rw`/`open_disk_apply_rw`; **VE** =
`volume_extent(s)`/`destination_volume_letter`; **IMG** =
`image_file_io()`/`storage::open_image`; **direct** = caller's own
CreateFileW; **pipe** = `build_read_pipe(slice, 8 MiB, 256 KiB)`
(cr_scan.cpp:164-203, constructed once per scan at :1298, per LDM member at
:1724, and per recovery at cr_ui.cpp:2100/:2159/:4485).

Unless noted, every pipe consumer issues **256 KiB, 64-KiB-aligned block
reads at the transport (QD1, sequential or random per engine), cancellation
polled at engine loop heads (CancelToken::checkpoint — cr_device.h:156-167),
errors accumulating into the pipe's ReadErrorMap (cr_retry.cpp:111-114 →
merged into ScanStats.errors, cr_scan.cpp:1399-1400)**.

| # | Consumer (entry point at HEAD) | Read sizes / alignment | Seq vs rand | QD / threads | Buffering | Cancel poll | Error map |
|---|---|---|---|---|---|---|---|
| 1 | Scan orchestrator probe (cr_scan.cpp:1328 `probe_family(pipe)`) | 512 B at LBA0 via pipe → one 256 KiB block fill + further 4-64 B probes served from the same block (cr_scan.cpp:977-1098) | random (one block) | QD1 | cache | no (pre-stage) | yes (probe_unreadable gate, :1327-1352) |
| 1b | boot-hash + identity (cr_scan.cpp:1377-1389) | 512 B at slice 0 — **raw slice** (bounce path) | random | QD1 | none | no | no (best-effort) |
| 1c | table capture / re-read (cr_scan.cpp:1125-1138, :2211-2218) | 512 B-per-sector reads — raw slice (bounce each) | sequential short | QD1 | none | no | no |
| 2 | NTFS engine walk (cr_mftwalk.cpp:1425-1479) | MFT runs in `0x8000`-sector chunks (16 MiB requests, :1459) → pipe splits into 256 KiB block fills | sequential | QD1 (walk is single-threaded per volume) | cache | yes (walk loop) | yes |
| 3 | NTFS remnant sweep (cr_scan.cpp:2179-2202, `scan_mft_remnants`, `threads` fan-out) | chunk fan-out via pipe | sequential chunks | multi-thread CPU, **serialized I/O** (cache mutex) | cache | yes | yes |
| 4 | FAT engine (fat::scan_fat via dispatch cr_scan.cpp:1793-1801) | FAT tables + dir clusters via pipe | mixed | QD1 | cache | yes | yes |
| 5 | exFAT engine (cr_exfat_quick.cpp:27-88) | **1 MiB aligned FAT windows** (`win`, :38) via pipe | sequential per window | QD1 | engine window + cache | yes | yes |
| 6 | ext4 engine | GDT/inode/bitmap/journal via pipe | mixed | QD1 | cache | yes | yes |
| 7 | Btrfs engine | chunk tree / nodes via pipe | mixed | QD1 | cache | yes | yes |
| 8 | ReFS engine (cr_refs_engine.cpp:80-93) | page reads (4×4 KiB pages) via an engine-side bounded page cache over the pipe | mixed | QD1 | engine page cache + pipe | yes | yes |
| 9 | HFS+ engine | B-tree nodes via pipe | random | QD1 | cache | yes | yes |
| 10 | ZFS engine | labels/uberblocks/block pointers via pipe | random | QD1 | cache (N-072 static-media skip) | yes | yes |
| 11 | Lost-partition discovery (cr_candidates.cpp:942-1080) | **64 KiB WindowScanner windows** (:33-58) via pipe → 4 windows per 256 KiB block fill; per-candidate probe re-reads 512 B-4 KiB **raw** (:81, :233, :291...) | sequential sweep, single-threaded | QD1 | window + cache | yes (:976-979) | read_errors counter (:970-982) |
| 12 | Carve engine (cr_carve.cpp) | **1 MiB windows + 4096 B overlap** (:45-46); ONE read_at per window (:445-489 window-serving device); tail 1 MiB reads (:1164) | sequential | ThreadPool fan-out when threads>1 (:1524-1588) — **I/O still serialized at the cache mutex** | window buffer | yes (worker contract, :1554) | yes (FB-C01 skipped windows) |
| 13 | Table parser / LDM (cr_table_parse.cpp read_sectors; cr_ldm.cpp:989-1042) | 512 B raw reads (bounce path); LDM config region up to 1 MiB raw (cr_ldm.cpp:1144-1146) | sequential short | QD1 | none | no | no (parse errors) |
| 14 | Check module pre-scan (cr_scan.cpp:1257-1259) | **raw slice, pre-cache** (deliberate: sees repaired bytes uncached) | mixed | QD1 | none | yes (gate :1257) | no |
| 15 | Recovery writer (cr_safewriter.cpp:461-776) | source reads in **1 MiB chunks** (cr_safewriter.h:158) through the M-136 pipe (cr_ui.cpp:4485; CLI recover same shape) | sequential per file | QD1 (single-threaded queue loop) | cache | yes (per chunk) | yes (M-136 salvage accounting, :465-471) |
| 16 | Preview / archive list (cr_preview.cpp:19-105) | capped at **4 MiB per call** (kMaxRead, cr_preview.h:59), assembled from extents | random (head windows) | QD1 | none (session device or pipe) | yes | passthrough |
| 17 | Thumbnail (cr_thumbnail.cpp) | via PreviewReader | random | QD1 | none | yes | passthrough |
| 18 | Image creation (cr_image.cpp:1553-1610 via cr_image_create.cpp:174-182 → **OD**, NO pipe) | **64 KiB chunks** (kChunk :1560), sync read + buffered WriteFile + SHA-256 update per chunk | sequential | QD1 | none on source | yes (chunk heads, :1574) | hard-fail (committed-prefix contract) |
| 19 | Bench verb (rrecov.cpp:2930-3130 → **OD**, NO pipe) | info: none; read: 1 sector; open: N× full open/close churn (1000 default, :3049-3054); seq: 1 MiB blocks × 256 MiB span (:3069-3082); rand: 1 MiB × 1000 LCG offsets — **mostly unaligned → the bounce path** (:3097-3112) | seq / rand | QD1 | `_aligned_malloc` buffer | no | hard-fail (breaks/counts) |
| 20 | Repair engine — fix (rrecov.cpp:3698 analyze OD; :3671/:3797 ODRW) | analyze via raw slice reads; execute writes ≤1 MiB staged (sector-aligned staging, cr_transport.cpp:627-633) | mixed | QD1 | none | yes (consent + undo journal) | surfaced via plan |
| 21 | Apply engine (rrecov.cpp:3236 plan OD; :3369 ODARW; :3397 refresh) | plan via raw slice; execute: table write + S-029 IOCTL chain on the RW handle + `refresh_disk_properties` (3× Sleep(1000) retry, :778-785) | mixed | QD1 | none | yes (consent) | hard device errors → rollback |
| 22 | DecryptingDevice (cr_ui.cpp:1588+ wrap; cr_decrypt_device.cpp:33-74) | whole-spanned-sector staging per read (heap vector :57), per-sector XTS (:65-69) | follows inner | QD1 | none | inner | passthrough verbatim (:59-61) |
| 23 | Health (rrecov.cpp:2723-2733 own handle; cr_health.cpp) | IOCTL passthrough only (ATA/SCSI/NVMe/CSMI) — no read_at stream | n/a | QD1 | n/a | no | exit-code surface (0/3) |
| 24 | Enumeration (cr_enumerate.cpp:139-212) | **OPD** + table parse per disk (512 B reads); 0-access retry open for busy disks (:98-113) | sequential | QD1 | none | no | honest-absent |
| 25 | BitLocker unlock (cr_bitlocker.cpp) | fveapi via volume letter — no raw data path | n/a | n/a | n/a | no | refusal surface |
| 26 | Destination arming (cr_transport.cpp:797-860) | **VE** — IOCTL only, access-0 opens, closed immediately | n/a | QD1 | n/a | no | boolean surface |
| 27 | Session/lastscan/reghive/bcdfix/nvramfix/bootfix/fsck-launch/query/tree/compress/crypto | **none** — file/registry/volume-letter only (verified: cr_bootfix.cpp opens hive FILES :126-139; cr_fsck.cpp launches chkdsk; zero `read_at`/PhysicalDrive refs in the others) | n/a | n/a | n/a | n/a | n/a |

CLI verb → transport map at HEAD (dispatch rrecov.cpp:3913-3950): `scan`
disk/volume → OD via `open_disk_view` (:1429-1443), `--image` → IMG
(privilege gate skipped, :1293-1297 + :542-547); `recover` → OD or IMG
(:2472); `health` → direct (:2723); `image` → OD (:2910);
`bench` → OD (:2989); `fix` → OD/ODRW (:3698/:3797/:3671); `apply` →
OD/ODARW (:3236/:3369) + refresh (:3397). The UI ABI mirrors all of these
(cr_ui.cpp:1440/:1495/:1582/:1737/:1999/:2663). Privilege gate: every
raw-disk verb calls `require_privilege()` FIRST (scan :1293, recover :2225,
archive-list :2408, image :2841, bench :2934, apply :3170, fix :3559) — exit
4 on a non-elevated token (rrecov.cpp:526-537); the ABI open impls return
`ERR_ELEVATION_REQUIRED` (0x500C0001, cr_error.h:120) instead
(cr_ui.cpp:1433-1435, :1488-1490, :1575-1577, :1725-1727).

---

## §4 Where performance is left on the table (ranked)

Baseline context (worklog/PERFORMANCE.md §1.1, run 41042267586 driver v0.6.1
vs the v0.7.0 numbers found in failed run 41140212296's evidence, per
worklog.md session-97 entry 1): seq@1 MiB 536.92 → **544.17 MiB/s (+1.4%)**;
rand@1 MiB 1993.22 → **1976.72 µs / 506 IOPS (parity)**; open 17,019 →
**9,213 ns (−46%)**; quick 150 → **27 ms**; deep 16,505 → 16,467 ms;
deepcarve 8 GiB 1,501,622 → **1,014,235 ms (−32%)**. The wall-clock gaps are
already favorable; the remaining opportunities are about CPU/byte, latency
tails, and the paths the current bench does not measure.

**F1 — CachedReader mutex spans the transport call: the whole scan is
effective QD1 (the dominant structural limit).**
`std::lock_guard<std::mutex> guard(mtx_)` at cr_cached.cpp:31 is held for
the ENTIRE `read_at` including `inner_->read_at(...)` (:61) — the
synchronous ReadFile. Every consumer in §3 rows 1-15 funnels through this
one lock: the multi-threaded carve fan-out (cr_carve.cpp:1541-1588), the
NTFS remnant sweep fan-out, and the candidate fan-out
(cr_scan.cpp:2362+) parallelize CPU only; each window/chunk fill waits on
the mutex. Cost mechanism: per-256 KiB lock/unlock + zero I/O overlap —
on the deepcarve workload the device is idle between block fills while
workers hash/validate. This is parity-preserving to remove for sequential
stages (one reader at a time anyway) and a strict win for the carve sweep
(multiple outstanding reads would need the handle to be safe for
concurrent ReadFile — see F2). Impact: rand throughput / deepcarve wall /
CPU-starvation of workers.

**F2 — Synchronous QD1 transport, no FILE_FLAG_OVERLAPPED: no read
pipelining is even expressible.** `overlapped_read` blocks in
`GetOverlappedResult(..., TRUE)` (cr_transport.cpp:70-75) on handles opened
without FILE_FLAG_OVERLAPPED (:291-293). The driver-era path had the same
QD1 shape (PERFORMANCE.md §1.3 point 2 blesses "reads stay synchronous QD1
(the consumers are QD1 by design)"), so this is parity, not regression —
but the direct era could cheaply exceed it: open with
FILE_FLAG_OVERLAPPED + N outstanding ReadFiles (the ABI's per-handle
single-flight contract, cr_transport.cpp:186-189 comment, would need a
per-handle slot array). Impact: seq throughput ceiling on fast NVMe (the
current 544 MiB/s @1 MiB is far below NVMe QD1 capability of the runner's
VHDX; the measured plateau ~240 MiB/s at 128-256 KiB in run 40227375329
suggests the VHDX/runner is the limiter there, inferred), rand latency
(506 IOPS @1 MiB ≈ 1.97 ms — mechanical-class for a virtual disk, likely
dominated by the VHDX block layer, inferred).

**F3 — One 256 KiB block per fill: no read coalescing across cache block
boundaries.** A 1 MiB carve window (cr_carve.cpp:45) is served as 4
misses → 4 separate 256 KiB `inner_->read_at` + 4 memcpys
(cr_cached.cpp:52-68, :87); the og issued 4 MiB device chunks
(Device_decompiled.c:56545, cited at cr_scan.cpp:1283-1286). The comment at
cr_scan.cpp:1286-1291 documents the 64→256 KiB tuning history; the next step
(large sequential fills for known-sequential consumers) is untaken. Cost
mechanism: 4× the syscalls + 4× the LRU traffic per MiB, 1× the device
bytes. Impact: seq throughput CPU/byte; small on wall time (measured
deepcarve already −32%).

**F4 — The 64 KiB bounce on every unaligned read: up to 128× read
amplification on probe-sized reads.** Every 512-byte raw-slice read (table
parse, boot hash, LDM probes, M-148 PRIVHEAD reads rrecov.cpp:1716/cr_ui.cpp:2092,
M-134 grading's one-sector-per-node reads, cr_grading.cpp:257/:280/:321)
issues a 64 KiB envelope read into the thread-local bounce and memcpy's out
512 bytes (cr_transport.cpp:206-213). On a healthy disk this is latency,
not throughput; on a dying disk each probe read drags 64 KiB through the
retry/bisect machinery. The envelope is mandatory (sector rejection + GAP-
S6-13, :152-190) but a SECTOR-granular envelope (512-multiple) would satisfy
constraint (a) without the 64 KiB amplification for tiny reads; constraint
(b) only binds lengths that are 4-KiB-multiples-not-64-KiB — a 512-byte
read is neither. Inferred opportunity: keep 64 KiB for ≥4 KiB requests,
sector-align smaller ones. Impact: latency of probe/table/check phases;
CPU/byte on grading (per-node sector reads over thousands of nodes).

**F5 — The rand bench measures the bounce path, not the device.** `bench
--mode rand` picks LCG offsets that are generally NOT 64-KiB-aligned
(rrecov.cpp:3104-3112) → every such read takes the envelope bounce (issued
length rounded up to 64 KiB + memcpy of the 1 MiB request). The reported
1976.72 µs / 506 IOPS therefore includes ~2× the bytes moved and an extra
1 MiB memcpy per op (inferred: the direct predicate fails unless
`off % 64 KiB == 0`, ~1/64 of draws). The driver baseline had the same
shape (the grammar path shared the envelope), so the comparison is honest,
but the number is not the device's random-read capability. Impact:
measurement fidelity for rand latency/IOPS; any QD/alignment sweep must
control this.

**F6 — Image create is a 64 KiB-chunk QD1 loop with buffered writes and
per-chunk SHA-256.** `create_image_stream` (cr_image.cpp:1553-1610): one
64 KiB sync read + one buffered `WriteFile` + `hash.update` per chunk —
16,384 iterations per GiB; the sink is FILE_ATTRIBUTE_NORMAL
(cr_image_create.cpp:56-58) with no write buffering flags, no larger
chunks, no async. The og used 4 MiB chunks (corpus cite at
cr_scan.cpp:1283-1284). Impact: image-create wall + CPU/byte (measured
image round-trip exists in evidence 05-feature-probes; no baseline delta
recorded — gap).

**F7 — WinImageFileIo serializes image reads under one mutex.**
`WinImageFileIo::read` holds `mu_` across the whole ReadFile loop
(cr_image_win.cpp:61-84) — every image-backed scan/recover/preview is QD1
even across threads, same class of limit as F1 but for the IMG transport.
Impact: image-lane throughput parity work.

**F8 — DecryptingDevice allocates per read and decrypts single-threaded.**
A heap `std::vector<u8> stage(span_bytes)` per `read_at`
(cr_decrypt_device.cpp:57) + per-sector `xts_.decrypt` loop (:65-69) + final
memcpy (:72). No buffer reuse, no SIMD batching, no parallel sectors.
Impact: BitLocker-volume scan throughput (unmeasured — no bench lane
exercises it; gap).

**F9 — Hard-coded Sleeps on the write/apply/quiesce paths.** S-010
quiesce: `Sleep(2000)` per volume (cr_transport.cpp:457); restore
`Sleep(1000)` per volume (:697); `settle_refresh` `Sleep(1000)`
(cr_apply.h chain, :590-592); `refresh_disk_properties` up to 3×
`Sleep(1000)` (:778-785). Corpus-pinned (og parity), but they dominate the
fix/apply/unlock verb walls. Impact: repair/apply latency only — not scan
throughput; any improvement must be justified against the corpus contract.

**F10 — No read-ahead / prefetch anywhere.** The cache is reactive
(CachedReader fills on miss only, cr_cached.cpp:52-68); the og had a
prefetch thread (DRWUI:962250-962597, cited at cr_scan.cpp:1292-1296 as
registered divergence D-72/N-045). For the sequential dominators
(discovery sweep, carve sweep, MFT walk) an 8 MiB-ahead fetch would hide
the per-block latency. Impact: seq throughput / deepcarve wall; already
−32% vs driver without it, so headroom is uncertain (inferred).

**F11 — The scan's checkpoint emission rewrites the whole session per
stage boundary** (emit_checkpoint_ at quick/carve/deep boundaries,
cr_scan.cpp:1450, :1497, :2281) — session-size-proportional I/O at each
boundary. Minor at current session sizes (save 9.3 ms measured); grows
with file count. Impact: latency spikes at boundaries; memory unchanged.

**F12 — probe_geometry costs 3 IOCTLs per open; the bench-open churn
measures them every time** (rrecov.cpp:3046-3066): already 46% better than
the driver (9,213 vs 17,019 ns) — remaining headroom is caching geometry
per disk number per process (enumeration already queried it). Impact: open
latency only; low priority.

**F13 — Volume-open dance for volume targets.** `open_volume_view` does
bitlocker status (fveapi) → `volume_extent` (open+IOCTL+close,
cr_transport.cpp:333-357) → `open_disk_view` (whole disk) →
`PartitionSlice::create` (rrecov.cpp:644-810, :733) — 3 opens + 2 IOCTL
chains before first read; every recovery/preview re-runs it per process.
Impact: TTFR/API-open (0.5 ms measured — already fine); cosmetic.

**F14 — Discovery's per-sector CPU probe is single-threaded**
(cr_candidates.cpp:975-1029): the 64 KiB window fetches are I/O-bound
through the pipe (F1) but the 55AA/OEM scan is a scalar byte loop per
sector on ONE thread. SIMD/multi-window parallelism (with F1 fixed) is
available. Impact: deepcarve CPU/byte — the measured −32% deepcarve win
came mostly from... (inferred: the direct open + envelope; decomposition
not recorded — gap).

**Metrics that are currently UNMEASURED by any harness** (gaps feeding the
ranking): CPU/byte on any workload, memory (working-set samples exist in
run-ours.ps1 but are not summarized into verdicts), cancellation latency,
queue-depth sweeps, write-path throughput, BitLocker path, image-create
baselines vs driver era, and the alignbench direct-vs-driver re-measure
that PERFORMANCE.md §1.2/§3 mandates for run 40227375329's −10.5% datapoint.

---

## §5 The privilege + error model

**`require_privilege()`** (rrecov.cpp:526-537): `is_elevated()`
(cr_elevate.cpp:12-21: OpenProcessToken(TOKEN_QUERY) →
GetTokenInformation(TokenElevation) → TokenIsElevated; FALSE on any probe
failure) — non-elevated → the 3-line stderr refusal + `ERR_ELEVATION_REQUIRED
(0x500C0001, cr_error.h:109-120 — a FRESH value space, deliberately not a
re-used 0x500B slot)` + **exit 4**, before any other output. Image targets
are exempt (`argv_has_image_target`, rrecov.cpp:542-547). The ABI
equivalents return `ERR_ELEVATION_REQUIRED` as BOTH the return value and
`g_last_transport_error` (cr_ui.cpp:1417-1419, :1433-1435, :1781-1787).
`open_disk_view` re-checks in-path (the TOCTOU truth layer,
rrecov.cpp:555-559). Nothing elevates silently; no helper is spawned
(cr_transport.h:12-18).

**Refusal contract**: under an elevated token, an open failure is a TARGET
error — `ERR_OPEN_FAILED` with the GLE printed + elevation-specific
guidance (rrecov.cpp:566-572; cr_transport.cpp:294-297, :735-745); there is
no fallback transport to switch to (the header says "the label is the
truth, not a choice", rrecov.cpp:2986-2987).

**Error mapping, Win32 → core (consolidated at HEAD)**:

| Site | Win32 condition | Core code | Cite |
|---|---|---|---|
| any open | CreateFileW fails | ERR_OPEN_FAILED | cr_transport.cpp:294-297, :735-738, :775-776 |
| any open | probe_geometry total_bytes == 0 | ERR_OPEN_FAILED | :300, :741-745 (exotic GetFileSizeEx fallback :302-308) |
| read | ReadFile/GetOverlappedResult fails, GLE ∈ {21,55,1110,1112,1619} (N-125 set) | ERR_DEVICE_DISCONNECTED | :68, :74; set at cr_transport.h:155-164 |
| read | any other GLE | ERR_READ_FAILED (+gle_out) | :68, :74 |
| read | got != len | ERR_READ_FAILED | :77-79 |
| read | envelope out of bounds | ERR_RANGE_OUT_OF_BOUNDS | :195-196; device bounds :267-268, :493-494 |
| read | bounce issued_len > 1 MiB + 64 KiB | ERR_BAD_PARAMETER | :202-203 |
| read | envelope take != len | ERR_READ_FAILED | :211-212 |
| read/write | ERROR_INVALID_HANDLE after failure | one heal + retry (same code if still failing) | :271-275, :500-505, :517-523 |
| write | WriteFile/GetOverlappedResult fails | ERR_WRITE_FAILED (write_gle_ kept) | :639-652 |
| write | put != len | ERR_WRITE_FAILED | :656-657 |
| write staging | _aligned_malloc fails | ERR_NO_MEMORY | :628-630 |
| quiesce | volume open or FSCTL_LOCK_VOLUME fails | ERR_VOLUME_LOCK_FAILED | :431-437 |
| S-029 chain | any DeviceIoControl fails | ERR_WRITE_FAILED (→ apply rollback) | :539-540, :562-563, :579-580, :586-587 |
| refresh | 3× UPDATE_PROPERTIES fails | ERR_WRITE_FAILED (open: ERR_OPEN_FAILED) | :775-787 |
| dest writes | ERROR_DISK_FULL/HANDLE_DISK_FULL | ERR_DEST_FULL (N-015); EXCED_RANGE/INVALID_NAME/UNC → the M-074 trio | cr_destio.cpp:23-48, :62-72 |
| VE sites | any failure | false (boolean surface) | :339-356, :805-823, :827-859 |

**Retry semantics** (cr_retry.cpp): only ERR_READ_FAILED/ERR_RETRY_EXHAUSTED
retry (3×, :42-54 — deterministic failures pass through unretried);
ERR_DEVICE_DISCONNECTED is NOT in the retry set and NOT recorded in the
ReadErrorMap (read_isolated returns it verbatim, :71-72) — it propagates
distinctly for the host's reattach-then-resume flow (N-125 contract,
cr_transport.h:136-148; PARITY-DELTAS D-121 keeps the engine retry window a
registered difference). Permanent failures: N-004 backtrack
(:83-99) → bisect to sector (depth 24, :100-141) → ReadErrorMap entry with
M-049 module attribution (:108-114) → Skip zero-fill (:124-127).
**Disconnect classification (N-125)**: pure numeric predicate over
{ERROR_DEV_NOT_EXIST(55), ERROR_NOT_READY(21), ERROR_MEDIA_CHANGED(1110),
ERROR_NO_MEDIA_IN_DRIVE(1112), ERROR_DEVICE_REMOVED(1619)}; 1117/23/87 stay
read failures (cr_transport.h:155-164). **Error-map accumulation**: merged
into ScanStats at every stamp (cr_scan.cpp:1399-1400) and serialized as the
geometric trio (cr_devmap.cpp:58-77; module not serialized).
**Read-truth gates** (cr_scan.cpp:1559-1586): fully-blind or nothing-read
runs REFUSE with ERR_READ_FAILED (the fake-scan fix) — CLI prints the
coverage and exits 1 (rrecov.cpp:2087-2097).

**CLI exit codes**: 0 ok; 1 failure (open/refusal/engine rc/session-save
fail); 3 cancelled (session saved when --out given, rrecov.cpp:2040-2065);
4 privilege refusal (:526-537); 5 device disconnected mid-scan — session NOT
auto-saved (rrecov.cpp:2072-2083); bench 2 on open/read failure
(rrecov.cpp:2990-2994, :3043, :3094, :3125).

---

## §6 The bench verb surface (`rrecov bench`, rrecov.cpp:2930-3130)

What it measures today (all through `open_disk` — the RAW direct device,
NO retry/cache pipe, QD1, transport label constant "direct" :2997):

| Mode | Command shape | What it does | Sizes at HEAD |
|---|---|---|---|
| info | `bench --disk N --mode info [--json]` | prints geometry: total bytes, logical/physical sector, is_4kn (:3008-3019) | no I/O |
| read | `--mode read` | ONE sector read at LBA0 through the transport; prints rc + first 16 bytes (:3022-3044) — a contract probe | 1 sector |
| open | `--mode open [--iterations N]` | N× full open_disk churn (CreateFileW + 3 geometry IOCTLs + CloseHandle), mean ns + ops/s (:3046-3066) | N default 1000, clamp [1,10000] |
| seq | `--mode seq [--mb M] [--block-kb K]` | ascending `read_at` loop from offset 0 into an `_aligned_malloc` buffer; MiB/s (:3075-3095) | span default 256 MiB (clamp 1..65536), block default 1024 KiB (clamp 4..16384 KiB) |
| rand | `--mode rand [--mb M] [--block-kb K] [--iterations N] [--seed S]` | LCG-distributed offsets inside the span; mean µs + IOPS (:3097-3126) — offsets are byte-granular → mostly the ENVELOPE BOUNCE path (§4-F5) | 256 MiB span, 1 MiB block, 1000 iterations (clamp 1..100000) |

How the benchmark lane drives it (.github/scripts/Run-Benchmark.ps1:362-371):
`bench --disk N --mode {info|seq --mb 256|rand|open} --json` — i.e. ALL
defaults (1 MiB blocks, 256 MiB span, 1000 iterations), plus one
`bench --disk 0 --mode info` for the OS disk (:372-373).

What it does NOT measure: **queue depth** (no QD sweep — everything is
QD1 synchronous by construction); **CPU per byte** (no CPU sampling inside
the verb; the harness samples working set + CPU at 500 ms granularity in
run-ours.ps1:63-67 but only for scan/recover, not bench, and never
summarizes); **memory** (same); **cancellation latency** (the verb has no
cancel path at all — no token plumbed); the **pipe** (retry+cache —
bench opens the bare device, so 8 MiB/256 KiB cache behavior, mutex
serialization and block-fill costs are invisible to it); **alignment
sweeps** (no --block-kb matrix is driven by any harness today; the
standalone bench/ours/run-alignbench.ps1 exists but is not part of
test-benchmark.yml's matrix — inferred from its absence in
Run-Benchmark.ps1); **write throughput** (no bench mode touches the write
path); **BitLocker/image/decrypt paths**; **warm-vs-cache behavior**. The
rand mode's offset distribution also conflates bounce-path cost with device
latency (§4-F5) — a QD/alignment-controlled rand would need new flags.

---

## §7 The known-failure forensics — S-PART-DELETE (v0.7.0 run 41140212296)

### 7.1 The exact command line (verified)

- Scenario matrix entry `.github/scenarios/benchmark-matrix.json`:
  S-PART-DELETE, mode `delete_partition`, disk `multi-gpt` (disk **3**, a
  5,120 MB GPT VHDX with CRNTFS=partition 2 + CREXFAT=partition 3 —
  evidence `02-disk-setup/disk-state.json`), volume CREXFAT,
  `target_mode: "disk"`, `scan_modes: "deepcarve"`, gate `discovery`.
- Run-Benchmark.ps1:498-501 passes `DiskNumber`/`PartitionNumber=@(3)` to
  apply-scenario.ps1, which runs `Remove-Partition -DiskNumber 3
  -PartitionNumber 3` (apply-scenario.ps1:206-226; log line 520:
  "GPT entries removed on disk 3: partitions 3 ... disk left unallocated")
  then captures a DISK-level hash map (:328-331 → volume.ps1:637-643,
  `BenchRaw::HashDisk` — opens `\\.\PhysicalDrive3` GENERIC_READ share 0x3,
  volume.ps1:70-76 — **proof the disk was raw-openable at 00:27:22.108,
  ~35 ms before the scan**).
- The CLI lane (Run-Benchmark.ps1:508-522) invokes
  `bench/ours/run-ours.ps1` with `TargetMode='disk'`,
  `DriverInfo='transport=direct'` (:518), `ScanModes=@('deepcarve')`.
- run-ours.ps1:79-89 builds, for `TargetMode -eq 'disk'`:
  **`rrecov scan --disk 3 --deep --threads 4 --out
  D:\cr-bench\work\scenarios\S-PART-DELETE\cli\session-deepcarve.crsf --log
  ...\stage-deepcarve.log --json`** (deepcarve = no extra stage flags;
  `--deep` is the deprecated alias that sets `cfg.whole_disk = true` and
  prints a note — rrecov.cpp:1483-1492).
- The API lane (never reached — the CLI lane threw first) would have run
  `.github/scripts/api_harness.exe --scan --target disk --disk 3
  --whole-disk --dest ... --out ... --label S-PART-DELETE`
  (Run-Benchmark.ps1:528-531).

### 7.2 The failure signature (verified from the runner log)

logs-41140212296/0_test-and-benchmark.txt:521-522: `=== ours scan mode:
deepcarve ===` at 00:27:22.1443 → `##[error]scenario S-PART-DELETE threw:
scan deepcarve produced NO session and was not capped - refusing to continue
(no silent skip)` at 00:27:22.6546 — **the rrecov process lived ≤ ~500 ms**
(run-ours.ps1:44-73 polls HasExited at 500 ms; wall 0.51 s = one poll +
overhead; the process exited on its own, `capped=false`). The throw site is
**bench/ours/run-ours.ps1:120-121** (no session file + not capped).
Evidence gap (session-97's own note, worklog.md): the exception path
archived NO scan stdout/stderr/exit code — `Copy-Evidence`
(Run-Benchmark.ps1:688-695) runs only after the lanes, and `ours-run.json`
(which records `exit_code`, run-ours.ps1:104) is written only at script
end — both postdate the throw. verdict.json (evidence 04-scenarios/
S-PART-DELETE) confirms `lanes: {}` — nothing was captured.

### 7.3 The code trace — every fast-exit path, and what the timing rules out

`cmd_scan` (rrecov.cpp:1289) in invocation order, with the exit that
would produce "no session, ≤0.5 s, not capped":

1. **exit 4** — `require_privilege()` (rrecov.cpp:1293-1297 → :526-537).
   RULED OUT: volume-lane scenarios minutes earlier (same process class,
   same elevation; e.g. S-FMT-FAT32's `[transport] direct raw I/O
   (\\.\PhysicalDrive2)` at evidence 04-scenarios/S-FMT-FAT32/cli/
   scan-deepcarve.stdout).
2. **exit 1** — `--disk` parse → `open_disk_view(3)` failure
   (rrecov.cpp:1429-1431 → :549-573: is_elevated ok → `win::open_disk(3)`
   → CreateFileW `\\.\PhysicalDrive3` GENERIC_READ share R|W|D
   (cr_transport.cpp:359-371, :290-298) → probe_geometry). NOT RULED OUT
   (see 7.4-H1).
3. **exit 1** — checkpoint sink create failure (`cannot create session
   file`, rrecov.cpp:1754-1759). RULED OUT: `$scCli` is created before the
   lane (Run-Benchmark.ps1:460) and worked for every earlier scenario.
4. **exit 1/5** — `ctrl->run()` returns non-OK: `scan failed: ...` (return
   1, rrecov.cpp:2098-2101) or `ERR_DEVICE_DISCONNECTED` (return 5,
   :2072-2083) — **neither path saves a session** (only ERR_CANCELLED
   does, :2040-2065; OK saves at :2140-2144). PARTIALLY RULED OUT by
   timing: run()'s pre-quick phase (table capture :1125-1138, pipe
   :1298, probe :1328, boot hash :1377-1389) is a handful of reads, but
   the FIRST stage on this disk is the S-051 table-target flow
   (cr_scan.cpp:2002-2107): the post-deletion GPT still carries partition
   1 (MSR, `FsKind::Reserved` — passes the target filter :2013-2016) and
   partition 2 (CRNTFS, a 3.2 GB NTFS volume) — scanning CRNTFS takes
   minutes (S-DEL-NTFS's deepcarve took ~4 min for the same volume class,
   log :473-474), and the whole-disk discovery sweep
   (cr_scan.cpp:2206-2282 → cr_candidates.cpp:975-1029) and the carve
   sweep would follow. **No legitimate engine-path error rc can arise
   before minutes of scanning on this state** (candidate-mode engine
   errors are swallowed, cr_scan.cpp:1872; the fully-blind gate
   :1559-1577 requires unreadability the disk-level hash disproves).
5. **crash** (exit 0xC0000005 etc.) — NOT RULED OUT (see 7.4-H2). Note
   this was the FIRST raw-disk `--disk --deep` invocation of the entire
   run (clean-volume scans are `--volume` only, Run-Benchmark.ps1:326; the
   image-lane whole-disk scan `scan --image` ran WITHOUT --deep
   (Run-Benchmark.ps1:210) and on the PRE-deletion disk image) — i.e. the
   whole-disk discovery path (mountpoints2 arming rrecov.cpp:1622-1632,
   LDM pre-scan :1702-1738, table-target flow, discovery) had ZERO
   successful prior executions in this run on a raw disk.

### 7.4 Root-cause hypotheses (ranked, with the discriminating test)

- **H1 (most probable): `open_disk(3)` failed post-Remove-Partition —
  exit 1 with `[transport] cannot open disk 3 (rc=...): ... device busy,
  locked, protected...`** (rrecov.cpp:566-572). Mechanism (inferred): the
  partition deletion triggers volume-stack/PnP reconfiguration; a raw
  CreateFileW on `\\.\PhysicalDrive3` in that window can fail with
  ERROR_SHARING_VIOLATION(32)/ACCESS_DENIED(5) or the disk can briefly
  report not-ready — the S-PART-DELETE lane is the only one that opens the
  disk ~30 ms after a table mutation (the volume lanes scan minutes after
  their mutations). The failure is a TARGET error by the direct-era
  contract (cr_transport.h:60-67) → exit 1, no session, ~0.1-0.2 s wall —
  consistent with the ≤0.5 s signature. The required fix is a bounded
  open-retry (or the lane sleeping past the reconfiguration), not a
  transport change.
- **H2: a native crash in the whole-disk-only code path** (first raw
  execution — §7.3.5). No candidate defect was found by reading the path
  (GPT name copy is u16[36]-safe, cr_table_parse.h:50; zeroed entries are
  skipped, cr_table_parse.cpp:319; MSR/CRNTFS targets are the same shapes
  the image lane scanned successfully) — but a data-dependent fault in the
  discovery/grading/writer interaction cannot be excluded without the
  exit code.
- **H3 (low): a fast engine rc** — requires an error the trace says
  cannot happen this early on this disk state (§7.3.4).

**Discriminating evidence to capture (next actions):** (1) archive
`scan-deepcarve.stdout/.stderr` + the process exit code BEFORE the
run-ours.ps1:121 throw (one-line Copy-Evidence + `$sr.exit` in the throw
message); (2) run the API lane first for this scenario (api_harness
reports `transport` + process rc in api-result.json even on failure,
api_harness.c:240-242/:494); (3) locally: `rrecov scan --disk N --deep
--out x.crsf` on a VHD immediately after Remove-Partition, capturing the
`[transport]` line — H1 predicts it prints `cannot open disk N`.

### 7.5 The stale transport expectation in the harness (exact locations)

- **`.github/scripts/Run-Benchmark.ps1:628-630`** — the API-lane gate:
  `if ($apiResult -and $apiResult.transport -and ([string]
  $apiResult.transport) -ne 'rda') { ... "api transport was
  '$($apiResult.transport)' (expected rda)" }`. THE stale assertion: the
  transport has been `"direct"` since v0.7.0 (cr_ui.cpp:1445, :1501,
  :1588; rrecov.cpp:2997; api_harness.c merely reports it). This gate
  added a spurious failure reason to EVERY scenario in run 41140212296
  (log lines 479, 495, 511, 538-543: "api transport was 'direct'
  (expected rda)").
- **`.github/scripts/Run-Benchmark.ps1:617` + `bench/ours/run-ours.ps1:146`**
  — the CLI-lane assertion's FIELD NAME `rda_transport_assertion` is stale
  (driver-era name), but its BEHAVIOR was updated for the direct era: it
  is armed by `DriverInfo = 'transport=direct'` (Run-Benchmark.ps1:518)
  and matches the `[transport] direct raw I/O` line (run-ours.ps1:144-149;
  printed by rrecov.cpp:561) — it PASSED in run 41140212296 (evidence:
  S-FMT-FAT32/cli/ours-run.json `rda_transport_assertion.hits = 1`). A
  rename would be cosmetic; the API-lane :628-630 comparison is the
  functional defect.
- No other `rda` expectations exist in the harness scripts (verified by
  exhaustive search of .github/scripts and bench/ours: only :617/:628/:630
  and run-ours.ps1:146).

### 7.6 The other failure classes in the same run (for context, not this task's scope)

Per session-97's T1 decomposition (worklog.md): recall-0 on floor-gated
scenarios is PRE-EXISTING (the driver-era baseline 41042267586 shows the
same recall-0 / 0-byte placeholder recovery shape — a recovery-pipeline
defect, not a direct-path regression); the transport-expectation failures
are §7.5; S-PART-DELETE is the only NEW, undiagnosed failure (§7.4).

---

## §8 Next actions this sheet feeds

1. §4-F1/F2 (cache mutex + sync QD1) are the two structural levers for
   rand/latency/parallel-sweep metrics — both need a concurrency-safe
   transport (FILE_FLAG_OVERLAPPED + per-slot OVERLAPPED) first.
2. §4-F5 + §6: before any optimization is measured, extend the bench verb
   (alignment-controlled rand, optional QD, CPU/byte, cancel latency) or
   every "faster" claim will measure the bounce path.
3. §7.4-H1: reproduce + fix (lane-side sleep or transport-side bounded
   open retry) and close the evidence gap (archive stdout/stderr/exit
   before the throw).
4. §7.5: fix Run-Benchmark.ps1:628-630 to expect 'direct' (one-line).
5. Fill PERFORMANCE.md §2 from the NEXT run's evidence (the v0.7.0 numbers
   in 41140212296's 07-performance/ are candidate rows but were captured
   under a harness with the §7.5 defect and the §7.4 abort).
