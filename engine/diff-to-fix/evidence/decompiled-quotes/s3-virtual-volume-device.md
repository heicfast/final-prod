# S3 evidence — CPartitionVirtualVolumeDevice decode (DRWUI + HFSPlusSearchDLL)

Quotes: `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c`
(HFS) and `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (DRWUI);
RTTI census `re-workspace/rtti_classes.json`; vtable map
`ui-resources/drwwui_vtable_map.json:38427-38462`. Decoded by the session-3
virtual-device agent, 2026-10-26. Both binary copies are field-for-field
identical.

## 1. Class shape (ctor HFS `FUN_180009130` :7113-7138; DRWUI `FUN_140643760` :1044826-1044847)

Object 0x50 bytes: `+0x00 vftable`, `+0x28 last-error`, `+0x30 parent
IStorageDevice*`, `+0x38 bytes/sector (default 512, inherited from parent via
vtable[5])`, `+0x40 base START SECTOR`, `+0x48 SIZE IN SECTORS`. Sector
units throughout.

## 2. Read semantics (HFS `FUN_180009290` :7219-7264; DRWUI twin :1044939-1044979)

```
if (parent == 0)  { err = -1; return 0; }
if (buf == 0)     { err = -2; return 0; }
if (0 <= offset && offset < slice_sectors && count <= slice_sectors) {
    n = count;
    if (slice_sectors < offset + count) n = slice_sectors - offset;   // CLAMP
    ok = parent->Read(buf, offset + base, n);                          // REBASE
    if (!ok) { err = -3; return 0; }                                   // no zero-fill on parent failure
    memset(buf + n*bps, 0, count - n);                                 // ZERO-FILL TAIL
    return ok; }
err = -1; return 0;                                                    // out of range
```
- Reads crossing the slice end are **clamped, tail zero-filled, and the call
  still succeeds** — the og engines were written against this contract.
- **Apparent og defect (both binaries)**: the tail `memset` length is
  `count - n` — a SECTOR count, not `(count-n)*bps` bytes (:7258) — the tail
  is under-zeroed by the sector-size factor on clamped reads.

## 3. Construction points

- **Lost-partition path (engine, heap)**: HFS :3567-3577 — when a searched
  partition node has its lost marker (`*(lVar3+0x110) != 0`) and `0 < start &&
  1 < size`: `new(0x50)` + ctor(parent, startSector, sectorCount), owned by
  the partition node (+0xc0), then partition context `FUN_18000b410`.
- **DRWUI HFS+ analysis (stack RAII)**: :981990-982036 — `local_f8[80]` slot,
  size = end - start, feeds a statically-linked `HfsPlusVolumeAnalyer`.
- **Orchestration**: `CGenericRecoveryController` "Insert lost partition %s,
  %d, StartSector=%I64d, TotalSector=%I64d" (:904076, :904198, :904404,
  :905106, :905938).
- **Cache order**: physical -> `CStorageDeviceWithCache` (4 MiB budget =
  `0x400000 / bps`, fallback 0x2000 blocks; :919080-:919092, ctor :962250)
  -> virtual volume device -> engine. Cache is disk-wide, shared across
  partitions.

## 4. No write path

All 21 shared-base `IStorageDevice` vtable methods (:176238-:176461) are
no-ops / trivial getters; no Write virtual exists anywhere in the family.
Recovery writes go to destination files; repair is a separate product world.

## 5. Sibling device classes (rtti_classes.json)

DRWUI: `CBasicPartitionDevice`, `CCacheDeviceProxy`, `CDoScanRemoteDevice`,
`CPartitionVirtualVolumeDevice`, `CRemoteDevice` (NAS/EuMrx),
`CSimulateStorageDevice`, `CStorageDeviceWithCache`. EXT4SearchDLL:
`CEXT4CatchBufferDeviceProxy` (read-failure retry with 1 KiB backtrack).
RAWSearchDLL: `CRawFileCellTestStorageDevice`. PartitionRecovery legacy:
`CDiskStorageDeviceBuilder`, `CWindowsStorageDevice`, `Win32VolumeStorageDevice`.

## 6. Architecture doc confirmation

`architecture/05-scan-engine-architecture.md:63-65`: "CPartitionVirtualVolumeDevice
— the volume abstraction; 'Virtual' = works for volumes the OS can't see
(lost/deleted partitions)". §8 (:116-125): engine -> virtual device -> file
API / raw IOCTL / EPMNtDrv / **BitLocker FVE keys via DRWService pipe ->
XTS-AES decrypt in-stack** / NAS EuMrx redirector.

## 7. Core comparison summary (include/cr/storage/cr_slice.h, src/core/storage/cr_slice.cpp)

| semantic | og | core |
|---|---|---|
| bounds | clamp + zero-fill tail + succeed | strict ERR_RANGE_OUT_OF_BOUNDS refusal (:46-47) |
| units | sectors | bytes, sector-aligned starts, ragged tails allowed |
| ctor validation | none (errors deferred) | eager (null/zero/align/overflow) |
| write path | none (interface) | none (interface) — MATCH |
| cache | 4 MiB disk-wide below slice | 8 MiB / 64 KiB pipe below candidate slices (cr_scan.cpp:610) |
| BitLocker | in-stack XTS-AES decrypt | pre-unlock gate only (rrecov.cpp:271-293) |
| NAS | CRemoteDevice/EuMrx | absent |
| geometry refresh | RefreshSectorSize from parent (:1044889) | snapshot at create |
| errors | sticky last-error (-1/-2/-3) | per-call u32 codes + persistent ReadErrorMap |
