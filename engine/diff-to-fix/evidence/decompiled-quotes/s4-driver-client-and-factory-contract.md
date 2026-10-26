# Decompiled evidence — session 4: driver client pattern + engine factory contract (M-030 / I-016)

Sources (corpus `03ad46c`): `re-workspace/reversed/ghidra-drivers/eprdtdrv_decompiled.c`,
`re-workspace/reversed/ghidra-mo/Device_decompiled.c`,
`re-workspace/reversed/ghidra-partition/PartitionRecovery_decompiled.c`,
`re-workspace/reversed/ghidra-searchdll/*_decompiled.c`,
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`.
Quotes verbatim; line numbers in the named file.

## 1. Driver wire contract (eprdtdrv ≡ ebrntdrv ≡ epmdkdrv — identical decompiles)

Device name: `\Device\<upcased service name>` with fallback literal `L"EPMNTDRV"`
(DriverEntry `FUN_000121a4`, eprdtdrv:749-881, fallback :830/:847). CREATE
(`FUN_000110b0` :33-313) parses the tail grammar; anti-hijack PID check :260-290
(cascading byte-add mangling b3+=b2; b2+=b1; b1+=b0, :274-284 — matches the client-side
mangle). Tail: `\Volume<d>` → `\Device\HarddiskVolume%u` (:107-136); `<d>` →
`\Device\Harddisk%u\Partition0` + DR retry (:189-207); **disk# ≤ 199** (:184).

IRP_MJ_READ (`FUN_00011fd8` :671-744): length is in **512-byte sector units** —

```c
719:  lVar6 = IoBuildAsynchronousFsdRequest(3,lVar3,lVar6,*(undefined4 *)(lVar1 + 8),&local_res10,local_38);
733:  *(ulonglong *)(param_2 + 0x38) = (ulonglong)(uint)(*(int *)(lVar1 + 8) << 9);  // Information = Length << 9
```

IOCTLs (`FUN_00011bcc` :482-562): custom codes **0x60002400 / 0x60002404 / 0x60002408**
(GetProcName / GetFileObjName / GetDiskContextMode; CTL_CODE(0x6000, 0x900..0x902,
METHOD_BUFFERED, FILE_ANY_ACCESS)); **any other IOCTL is a pass-through proxy** to the
attached device (:539-546).

## 2. Client open path + WinPE gate (Device.mo)

```c
50628:  pcVar5 = "\\\\.\\EPRDTDRV\\%d\\%d";                       // disk open grammar
50643:  local_190[0] = GetCurrentProcessId();                     // mangled :50644-50652
50656:    FUN_1800179c0(local_138,0x104,local_160,local_190[0]); // \\.\EPRDTDRV\<mangled-pid>\<disk>
50659:    FUN_1800179c0(local_138,0x104,"\\\\.\\PhysicalDrive%d",...);  // FALLBACK
```
Volume variant (:329-358): `"\\\\.\\EPRDTDRV\\%d\\Volume\\%d"` vs `"\\\\.\\HarddiskVolume%d"`.

**WinPE probe** `FUN_1800577e0` (:50943-50992): `GetSystemDirectoryA` → if on drive
`X:`/`x:` → `CreateFileA("\\.\X:")` + `DeviceIoControl(h, 0x560000, NULL, 0, buf, 0x110)`;
callers branch driver-path vs PhysicalDrive. **On live Windows the driver path is used;
in WinPE the direct path.**

## 3. Read chunking + queue depth (the 64 KiB provenance question)

```c
56545:  *(int *)(param_1 + 0x10) = (int)(0x400000 / *(int *)(param_1 + 0x38));  // 4 MiB / sectorSize  (disk)
56915:  *(int *)(param_1 + 0x10) = (int)(0x700000 / ...);                        // 7 MiB / sectorSize  (volume)
```
Sector-aligned **synchronous** ReadFile loop (`FUN_180062600` :57010-57028,
`LPOVERLAPPED 0x0`) → **queue depth 1**; no OVERLAPPED/IOCP anywhere in Device.mo.
**No 0x10000 (64 KiB) multiple rule exists at the device layer** (only the >4 GiB
total-read reject :57005). The only 64 KiB-ish corpus constants are the NTFS 64 KB MFT
bootstrap buffer (ntfs-engine--deep-dive.md:21) and the FAT 0x10000-cluster deleted-chain
guard (fat-engine--deep-dive.md:34) — engine-local, not transport alignment.

## 4. Stale-handle ladder + volume quiesce terminal step

```c
// Device_decompiled.c:55942-55967 — SetFilePointer fail + GetLastError()==6
//   (ERROR_INVALID_HANDLE) → CloseHandle → re-CreateFile → retry once.
// RecoveryModule_decompiled.c:60440 — quiesce chain ends with
//   IOCTL_VOLUME_OFFLINE (0x56C00C) after DISMOUNT.
```

## 5. Engine factory contract (9/9 verified; two matrix corrections)

| DLL | Complete | Quick | Notes |
|---|---|---|---|
| NTFS | `EUCreateNTFSSearcher` :33 | `EUCreateNTFSQuickSearcher` :52 | 1-arg out-slot |
| FAT | `EUCreateFATSearcher` :22 | `EUCreateFATQuickSearcher` :37 | 1-arg |
| exFAT | `EUCreateEXFATSearcher` :11 | `EUCreateEXFATQuickSearcher` :26 | 1-arg |
| EXT4 | `EUCreateEXT4Searcher` :11 | `EUCreateEXT4QuickSearcher` :44 | 1-arg (matrix's "grep miss" note REFUTED) |
| HFS+ | `EUCreateHFSSearcher` :22 | `EUCreateHFSQuickSearcher` :41 | 1-arg; quick = triple-vftable MI |
| ReFS | `EUCreateReFSSearcher` :83 | `EUCreateReFSQuickSearcher` :54 | **Quick takes 3 args** (arity deviation) |
| Btrfs | `EUCreateBtrfsComplateSearcher` :11 | `EUCreateBtrfsQuickSearcher` :22 | **Complete = 8-byte stub returning NULL** |
| ZFS | `EUCreateZFSSearcher` :15407 | `EUCreateZFSQuickSearcher` :15453 | 1-arg |
| RAW | `EUCreateRAWSearcher` :72122 | (none; `EUCreateRAWSearcherForCellTest` :72142, 5-arg, cell-test only) | registry exports: EUGetAnalyserList/Map/Multimap + `GetRawUserInfoMap` (only this one consumed by DRWUI, :227538) |

Host invoker: `FUN_14056fe40` (DRWUI:906144-906162) — `GetProcAddress` then
`(*pFVar1)(&local_28)` (ONE out-slot argument). Engine interface consumed via vtable
slots (host call sites): +0x00 attach partition, +0x08 sub-partition list, +0x20 scan
mode, **+0x30 IAuxiliaryBitmap (global 10 MB+512 B singleton, DRWUI:897296-897335)**,
+0x40 path string, **+0x48 RAW per-file `std::function<string(string,string)>` hook
(DRWUI:917292-917296)**, **+0x58 Search(IStorageDevice*, callback*) → 64-bit
{facility<<32|code} status**, +0x70 complete-run, +0x80 pre/teardown, +0xe8 late ctx.

**Progress = poll/push hybrid + 5 s watchdog**: callback vt+8 progress, vt+38 phase,
**vt+10 cancel poll (engines poll per record batch)**; DRWUI watchdog thread every
5000 ms bumps coarse progress (+0xe8++) and re-fires vt+8 when the engine step counter
(+0x8fc) lags (+0x8f8) — progress advances **even when the engine is silent**
(DRWUI:910565-910585, 910864; cancel = flag at scanctx+200, join with 10 s wait).

**Engine-enable gates**: `config/version.ini` `scanext4`/`scanzfs` (DRWUI:913038-913046,
913104-913112 quick; 917169 ZFS complete), `Config.dat [register] bAddSpecifyDirFilter`
(917053-917060 complete set), settings+0x60 for RAW (917266-917270). Deep-scan complete
set = NTFS/FAT/exFAT/ZFS/HFS/ReFS/RAW — **never Btrfs** (917068-917262). ReFS DLL refuses
to load pre-Win8 (ReFSSearchDLL:37-52, 17821-17840). Dead codes: dispatcher maps 4/5 →
`EUCreateEXT2QuickSearcher` but `Ext2SearchDLL.dll` is unshipped (DRWUI:911145, 903488).

**FS-code spaces do NOT align** (og: 3=NTFS 9=EXT4 0xB=Btrfs 0xD=ZFS; core: 1=NTFS 4=ext4
7=Btrfs 8=ZFS) — any cross-reference table must not silently equate them.
