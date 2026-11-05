# REFERENCE-ARCHITECTURE — the og (EaseUS DRW) access & recovery architecture as the corpus documents it

> **Provenance.** Read-only excavation of the research corpus at
> `/home/z/my-project/repos/easeus-drw-research` (commit `be0cfe9`). Every claim cites
> `path:line` relative to that repo root (decompiled code = `re-workspace/reversed/…`,
> curated docs = `architecture/` + `re-workspace/artifacts/…`). Date: 2026-11-05.
> Session: performance-parity (post v0.7.0 driver removal) — **Agent A / T2-a**.
>
> **Companion sheets** (core repo, pre-removal unless noted): `worklog/REFERENCE-ACCESS.md`
> (the access-path evidence sheet — still accurate, re-verified here where cited),
> `worklog/ACCESS-LAYER.md` (the core's pre-removal access layer), `worklog/CONSUMERS.md`
> (per-engine read patterns of the core), `worklog/DRIVER-CONTRACT.md` (the RDA driver wire
> surface), `worklog/PERFORMANCE.md` (the driver-era baselines + alignbench runs).
> Core-side pointers in §5 are relative to the core repo at HEAD `e6ee97da` (v0.7.0,
> direct-only).
>
> **Errata applied**: `re-workspace/artifacts/CORRECTIONS.md` rows 1–9 were checked against
> every claim below; none of the 9 rows concerns the raw-access path itself, but rows 3, 4,
> 5, 6, 7, 8, 9 correct per-engine/filter claims cited in §2/§3/§5 and are folded in.
>
> Corpus untouched. One file written: this one.

---

## §1 The reference's I/O access architecture

### 1.1 Two access stacks, gated per device-open by a WinPE probe

The og ships **one kernel-driver access subsystem AND full direct user-mode raw I/O**; the
choice is made per device-open by `IsWinPE()` — X:-system-dir probe + RAM-disk extents +
`[Public] IsWinPE` config override (`re-workspace/reversed/ghidra-mo/Device_decompiled.c:50943-50992`,
typed twin `ghidra-partition/RecoveryModule_decompiled.c:42737-42801`). Live Windows →
`\\.\EPRDTDRV\{PID-token}\{disk}\{part}` (eprdtdrv.sys); WinPE →
`\\.\PhysicalDrive%d` / `\\.\HarddiskVolume%d` direct. Full decision map, flag tables, and
the 5 verified gate sites: `worklog/REFERENCE-ACCESS.md` §1-§2 (re-verified this session,
no corrections needed). The core's direct-only posture (v0.7.0) is therefore
behaviorally equal to the og's own WinPE deployment mode.

**Driver read contract** (`eprdtdrv.sys`, decompiled):
- `ReadFile` on the EPRDTDRV handle: **file offset = absolute byte offset (LBA×512),
  length = count of 512-byte sectors**; the driver maps the user buffer MDL
  (`MmMapLockedPagesSpecifyCache`, DO_DIRECT_IO), builds an async FSD IRP, waits on event;
  on success `IoStatus.Information = Length << 9` — **512 hardcoded**
  (`re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:52-73`,
  decompile `ghidra-drivers/eprdtdrv_decompiled.c:671-744`).
- Any non-custom IOCTL passes **transparently through to the disk stack**
  (`eprdtdrv-sys--driver-internals.md:75-84`) — geometry/SCSI queries without re-open.
- Device-side: no queueing, no retry, no caching in the driver — it is a pure
  synchronous forward-IRP sector pipe (same artifact; core `worklog/DRIVER-CONTRACT.md` §3
  confirms the RDA clone inherited exactly this).
- PID-token anti-abuse bind at CREATE; disk# must be < 200; disk-stack walk requires a
  driver literally named `Disk` (`re-workspace/artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md:61-123`).
- Through the driver path, Device.mo **emulates disk IOCTLs in user mode** and
  **fake-succeeds `FSCTL_DISMOUNT_VOLUME`** while LOCK/UNLOCK fail
  (`Device_decompiled.c:45138-45220`, esp. `:45216`).

### 1.2 The direct path's read loop — sizing, buffering, batching

`CVolumeDevice::ReadSectors` (`Device_decompiled.c:56961-57038`): 64-bit
`SetFilePointer(FILE_BEGIN)` + **synchronous `ReadFile` loop** that continues across
partial reads, converts the byte count back to **sectors** as its return value.

**The og's two transport chunk constants** (the "4/7 MiB" model):
- Disk-level `CWindowsStorageDevice`: `+0x10 = 0x400000 / sectorSize` — **4 MiB** per
  ReadFile, recomputed after the geometry IOCTLs
  (`Device_decompiled.c:56503-56551`, constant at `:56545`).
- Volume-level `CVolumeDevice`: `+0x18 = 0x700000 / sectorSize` — **7 MiB** per ReadFile
  (`Device_decompiled.c:56903-56914`, constant at `:56912`).
- Both are **sector-size-normalized**: on 4Kn the sector count shrinks so the byte chunk
  stays 4/7 MiB.

**Buffering posture**: the scan/data path is plain **buffered, synchronous** I/O — no
`FILE_FLAG_NO_BUFFERING`, no `FILE_FLAG_OVERLAPPED` anywhere in the MOM read path. The
single `NO_BUFFERING|OVERLAPPED` open in the product is DRWUI's disk-info handle
(`DRWUI_decompiled.c:176880`), used for IOCTLs only (geometry 0x70000 at `:176887`,
STORAGE_QUERY_PROPERTY 0x2d1400 at `:178061-178072`) — never bulk reads
(`worklog/REFERENCE-ACCESS.md` §2.1-§2.2, flag table verified).

**All issued chunk sizes are 64 KiB multiples by construction** — 4 MiB device, 7 MiB
volume, 2 MiB MFT growth batches (`ghidra-searchdll/NTFSSearchDLL_decompiled.c:5649-5678`),
1 MiB $UsnJrnl-class reads, 64 KiB boot region (`aiStack_10038`,
`re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:44`). (The 1 MiB $UsnJrnl
site is carried from the core's own citation `src/win/cr_transport.cpp:174`; the exact
decompile site was not re-pinned this pass — flagged, not corpus-verified here.)

### 1.3 The og's cache + prefetch model — corpus-verified (the D-72 story)

The og wraps the `IStorageDevice` handed to the scan in **`CStorageDeviceWithCache`**,
implemented in the DRWIntelligentScan `storagedevicemodule` and linked into DRWUI.exe:

- **Budget**: `cache_count = 0x400000 / sector_size` (**4 MiB** worth of sectors),
  fallback `0x2000` blocks when the sector query fails —
  `ghidra-drwui/DRWUI_decompiled.c:919080-919092` (verified this session, line-exact).
- **Ring geometry**: run length **1000 sectors** (`FUN_1405c77d0(param,1000)` at
  `:962281`), look-ahead window of **10 runs** (`*(u32*)(this+0xe) = 10` at `:962283`),
  per-slot buffers of `sector_size × 1000` allocated in the init loop
  (`:962566-:962583`). Construction once per scan setup (the `:919039` ctor chain), with
  the 0x1a8-byte engine-side cluster retune object built when the engine recognizes the
  wrapper (`ghidra-searchdll/NTFSSearchDLL_decompiled.c:7165-7169` → `FUN_18000fae0`
  at `:11752-11811`).
- **Dedicated prefetch thread** — `FUN_1405c78b0` creates two events (wake + ready) and
  `CreateThread(NULL, 0, FUN_1405c7ac0, …)` (`DRWUI_decompiled.c:962589-962594`). The
  thread proc (`:962602-962682`) polls its wake event with a **1 ms** timeout, advances
  the ring index modulo 10, clamps the run at the device end
  (`LogInfo("----------CacheThread find read sector = %d")` — the literal "CacheThread"
  log string at `:962634`), **zero-fills slots whose read fails** (memset 0,
  `:962642-:962647`), issues the read with a prefetch flag
  (`device.read(slot, pos, run, 1)` at `:962664`), then under a **CRITICAL_SECTION**
  (`:962268`, `:962670-:962676`) bumps the filled count, `ResetEvent(wake)` when the
  window is full, `SetEvent(ready)`.
- **Consumer path** (`FUN_1405c7650`, `:962419-962482`): in-window requests are served by
  `memcpy` out of the run buffer; out-of-window requests fall through to the underlying
  device read (again with the prefetch flag).
- A bitmap-gated "should prefetch this range" check (`FUN_1405c7cd0`, `:962687-962703`)
  memcmps a used-space bitmap — the ring skips ranges the bitmap marks unchanged
  (reading: prefetch only where the FS bitmap says data lives; the bitmap source
  `FUN_1404ecb50` was not decoded further — **hypothesis, flagged**).

This is the factual basis of registered divergence **N-045 / PARITY-DELTAS D-72** ("og
4 MiB + prefetch thread vs core 8 MiB LRU") — the corpus side of the register is now
line-cited end-to-end. Core mirror comment: `include/cr/storage/cr_cached.h:9-35`.

**Engine-internal caches** (second layer, per-DLL): the NTFS engine RTTI-casts the host
device to `CStorageDeviceWithCache` and re-tunes it from the volume boot sector (512-byte
read at LBA 0, BPB parse, cluster-sized units, min 8 sectors, critical-section guarded —
`NTFSSearchDLL_decompiled.c:7165-7169` + `:11752-11846`). HFS+ keeps a hot-node
`CHFSPlusFileCache`; Btrfs persists `BtrfsCache.txt` next to the DLL to speed re-scans
(`re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md:66-67`).

### 1.4 Concurrency model

- **No parallel raw-read engine exists in the og.** Sector reads are serialized through
  the single per-volume device object; the only synchronization is the cache wrapper's
  critical section (`worklog/REFERENCE-ACCESS.md` §6, conclusion re-verified — nothing
  in the corpus contradicts it).
- Concurrency is at the **engine/UI level**: structured scan and RAW carving "run
  concurrently" (`architecture/04-recovery-workflow.md:73-74`); engines run on
  `CDoScan*` strategy threads with cooperative `CStopScanThread`
  (`re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:51-55`); the 45-thread census
  contains no IO/reader pool (`re-workspace/artifacts/DRWUI-exe--thread-architecture.md`).
- Engine-side worker coordination uses **Vista+ condition variables**
  (InitializeConditionVariable / SleepConditionVariableCS / WakeAllConditionVariable,
  GetProcAddress-resolved with pre-Vista fallback) —
  `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md:72-74`.
- **Memory-adaptive batching**: the NTFS engine logs GlobalMemoryStatus ("Total physical
  memory… Available… page files…") and its batch sizes adapt to RAM
  (`ntfs-engine-internals.md:70-71`). No numeric table is documented — mechanism only.
- The FAT quick scan is **threaded per-worker** (`CFATQuickSearch::CFATQuickSearchWorker`,
  13+ ctor params incl. thread control at +0x38, sector-size defaulting
  `param_3 == 0 ? 512 : …`) — `re-workspace/artifacts/SearchDLL/fat-engine--deep-dive.md:14-17`.

### 1.5 Geometry discovery and sector-size handling

- Geometry: `IOCTL_DISK_GET_DRIVE_GEOMETRY (0x70000)` → `GET_PARTITION_INFO_EX (0x74004)`
  → `GET_LENGTH_INFO (0x7405c)` with **512 fallback when BytesPerSector == 0**
  (`Device_decompiled.c:56523-56545`; base class default `*(uint*)(this+0x38)=0x200` at
  `:55587`).
- Enumeration is **environment-independent WMI** `ROOT\microsoft\windows\storage:
  MSFT_{Disk,PhysicalDisk,Volume,…}` (`Device_decompiled.c:17024,17196,17799`); the QA
  boot log adds Storage Spaces + VDS enumeration and per-partition capacity via
  `CNTFSLib::GetSpaceInformation` for NTFS vs Win32 API otherwise
  (`re-workspace/artifacts/config-files/vendor-logs--qa-session-forensics.md:13-30`).
- Drive-layout buffer grows **1000 B + 640 B per ERROR_INSUFFICIENT_BUFFER retry**
  (`Device_decompiled.c:50691-50730`).
- **512e/4Kn**: the direct path is fully geometry-driven (chunk recompute §1.2); the
  driver path **hardcodes 512** (`len<<9` + `×0x200` in Device.mo dispatch
  `Device_decompiled.c:46429`), so 4Kn must be normalized above the driver — no corpus
  artifact demonstrates end-to-end 4Kn through the driver (REFERENCE-ACCESS C-7).
  RAWSearchDLL carvers **query the storage device's real sector size (vtable slot 5)
  rather than assuming 512** — "correct on 4Kn disks"
  (`re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md:71`).
- Disks capped at 200 in the driver namespace; DRWUI's info layer caps at 32
  (`epmntdrv-trio--naming-pid-handshake.md:107`; `DRWUI_decompiled.c:176873-176874`).

### 1.6 The scan staging model (quick / advanced / lost-partition)

1. **Quick scan**: one engine chosen by the volume's FS code (0-2 FAT, 3 NTFS, 4-5 EXT2,
   7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 13 ZFS, +Btrfs) via `EUCreate<FS>QuickSearcher`
   (`architecture/05-scan-engine-architecture.md:28-43`; DRWUI `FUN_140577d40`,
   `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md:239-253`). NTFS quick =
   direct MFT enumeration incl. deleted records; `GetPrivateProfileIntW("search",
   "fastsearch", 1, Config.dat)` — **default ON** (`NTFSSearchDLL_decompiled.c:7163`).
2. **Advanced/deep scan**: `FUN_140580720` creates the **full searcher set**
   (NTFS/FAT/exFAT/ZFS/HFS/ReFS Searchers) **plus `EUCreateRAWSearcher`** gated on a
   settings flag (+0x60) — "Advanced scan = ALL FS engines in full mode + RAW carving
   engine, in parallel" (`DRWUI-exe--main-ui-architecture.md:255-261`; UI copy
   "Advanced Scan is searching files sector by sector…", `architecture/07-user-flow-uiux.md:42-46`).
   The quick pass completes first and **advanced continues in the background while the
   user may already recover** (`re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md:34-36`).
3. **Lost-partition scan**: `CDoScanLostPartition`; PartitionRecovery .mo analysers emit
   candidates, `CGenericRecoveryController` inserts them as scan targets with
   StartSector/TotalSector (sector default 512)
   (`architecture/04-recovery-workflow.md:66-69`); real QA output shows 15+ overlapping
   candidates scored by recoverable-file counts and resolved by `AdjustPartitionInfoOrder`
   (`re-workspace/artifacts/config-files/qa-logs--session-forensics.md:37-42`).
4. **RAW carving pass**: signature engine over unallocated/free space (volume bitmap from
   the FS engine feeds it — `searchdll-family--architecture.md:98`).

### 1.7 Volume IO layering (who reads the disk, per engine)

```
Engine tree reader
  └─ CPartitionVirtualVolumeDevice (works on lost partitions)
       ├─ normal file API when volume is mounted & healthy
       ├─ raw IOCTL path → EPMNtDrv (\\.\EPRDTDRV\<d>\<p>) — MDL async sector IO, 512B sectors
       ├─ BitLocker: FVE keys (DRWService pipe) → XTS-AES decrypt (BitLockerLib/OpenSSL)
       └─ NAS: EuMrx redirector letter (X:\EuShare.com\... → \\127.0.0.1 tunnel)
```
`architecture/05-scan-engine-architecture.md:116-125` (§8), cross-verified by
`re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:143-149` (capa IOCTL site
0x18002C0BA) and `rawsearch-carving--deep-dive.md:80-81` (carvers use the same volume IO).

---

## §2 Per-engine documented read patterns

All engines share the "engine SDK" (CFileNode/CFileSystemNode/IFileProperties/IDataRunList
statically compiled into every DLL — `remaining-engines--ext4-hfs-refs-btrfs-zfs.md:88-93`)
and speak **sectors** to the device abstraction; the host supplies the cached wrapper (§1.3).

| Engine | Documented read pattern | Citation |
|---|---|---|
| **NTFS** | DBR parse → $MFT datarun; first MFT-record region read into a **64 KB stack buffer** (`aiStack_10038` = 16,388 dwords); records read in **64 KB batches** (0x10000 buffers); **$MFT growth probe reads 0x1000 sectors (2 MiB @512) per device call** in a loop that extends the run while fixup validates (record-size-strided `FUN_180041bb0` re-validation inside the page); record-size clamp ≥ 0x400; MFT reads "cluster-strided … aligned blocks, not per-record" | `ntfs-engine--deep-dive.md:42-62,143-151`; `ntfs-engine-internals.md:28-41`; `NTFSSearchDLL_decompiled.c:5649-5678`; `ntfs-engine-algorithms-v2--deleted-viability.md:125-128` (the ~21100 batch loader: **0x1000-granular reads** `device.vt[0x08](device, offset, buf, 0x1000, 0)`, 4 KiB pages unpacked into record-size slots) |
| **NTFS $MFTMirr** | Fixed-location estimate, **fallback = read MFT record 1's own $DATA runlist** (two-layer resilience); NTFSLib `InitMFTMirror` | `zfs-vdev--mftmirr--calcpartition.md:34-60` |
| **NTFS deleted phase** | Per-batch walker `FUN_18001acc0`: cancel poll **per record** (`cancel_callback.vt[0x10]()`), fixup chain (min 0x400 parse capacity), strict `0x3ff < size` viability-walk gate (CORRECTIONS #7), bitmap cross-check per run | `ntfs-engine-algorithms-v2--deleted-viability.md:8-66` |
| **FAT** | Quick-search worker embeds a **0x200 sector staging buffer**; FAT tables behind `CFAT12/16/32Table` + `CDoubleFATTable` (FAT1+FAT2 mirrored, choose-healthy); directory walk **batches whole FAT runs into one device call** (`device.read(buf+filled, sector, spc*next)`), visited-range **red-black tree cycle guard**, `.`-entry signature check; deleted 0xE5 entries kept with a **64 KB chain-length sanity guard** (`< 0x10000` clusters) | `fat-engine--deep-dive.md:12-55`; `fat-exfat-engine-algorithms-v2.md:29-58`; `fat-exfat-engine-internals.md:27-45` |
| **FAT12 quirk** | Og follows FAT12 BAD cluster 4087 **raw** (garbage-tolerant continuation) — no canonicalization at the 12-bit boundary (CORRECTIONS #3) | `CORRECTIONS.md` row 3 |
| **exFAT** | `CQuickSearchAndBuildBitmap` builds the allocation bitmap during the quick walk; dentry **triples** validated by `(type-X)&0x7F` InUse-mask classes, backward 0x40-strided triple reconstruction; NoFatChain contiguous runs resolved without FAT walking; FAT-filter ini (`GetPrivateProfileStringA("Filter","FATFilterName",…)`) | `exfat-engine--deep-dive.md:23-38`; `fat-exfat-engine-algorithms-v2.md:60-78`; `fat-exfat-engine-internals.md:59-73` |
| **exFAT FAT windows** | **Not documented in corpus** — no 1 MiB FAT-window constant appears in the exFAT artifacts or greps of `exFATSearchDLL_decompiled.c` summaries. The corpus only documents whole-run batching (FAT family, above). | (absence — explicit) |
| **EXT4** | 0x200-byte read at **offset 0x400** (fixed SB position), 0x400-byte SB window copied into the volume object; `2 × block_size` GDT read buffer read through the device **block API (vtable+0x78, count × block_size)**; `CEXT4CatchBufferDeviceProxy` — on read failure allocates a fresh **0xE00-byte** buffer and retries with a **0x400 (1 KiB) backtracked shifted window** (the skip-bad-sectors salvage); strided reads `(block + param_2) * geometry` | `ext4-hfs-engine-algorithms-v2.md:34-48`; `ext4-engine--deep-dive.md:25-33`; `remaining-engines--ext4-hfs-refs-btrfs-zfs.md:11` |
| **EXT4 journal** | Gates **ONLY** on jsb magic 0xC03B3998 + blocktype {3,4}; every block in [s_first, s_maxlen) feeds the directory parser — no commit/revoke handling (CORRECTIONS #6). Backup-superblock positions are **UNVERIFIED for the engine** (CORRECTIONS #4). Journal traversal traces "GotJournalDir/TraverseJournalDir". | `CORRECTIONS.md` rows 4, 6; `remaining-engines--ext4-hfs-refs-btrfs-zfs.md:14-19` |
| **HFS+** | BE-decode volume header; gates: sig `H+`/`HX`, **block size multiple of 512**, version < 0x11, block-count invariants; catalog/extents/attributes B-trees walked leaf-by-leaf with per-node cancel+EOF virtual checks; `CHFSPlusFileCache` hot-node cache; full hfspluslib incl. journal | `ext4-hfs-engine-algorithms-v2.md:50-83`; `hfsplus-engine--deep-dive.md:13-75` |
| **ReFS** | `refs::` implementation layer chains **SUPB/CHKP/MSB+ReFSFSRS** pages (page_type 1/2/3 signatures, min 0x20 size) through `RefsBaseDataImplManager<T>`; small controller surface, OS-version gating | `refs-btrfs-zfs-engine-algorithms-v2.md:7-33`; `refs-engine--deep-dive.md` |
| **Btrfs** | Superblock probes at **0x10000** + mirrors (0x40000/0x4000000/…): per candidate, size check then **read 0x1000 (4 KiB) at offset**, `_BHRfS_M` magic, pow2 sectorsize, nodesize band 0x86..0x86+0xFF7B, **newest-generation wins**; zlib inflate for compressed extents, RC4 for encrypted datasets, crc32c/adler32 checksums; **BtrfsCache.txt** persisted metadata cache; **no chunk-tree-uuid validation** (CORRECTIONS #8) | `btrfs-engine--deep-dive.md:18-38`; `refs-btrfs-zfs-engine-algorithms-v2.md:35-53`; `CORRECTIONS.md` row 8 |
| **ZFS** | vdev label read at **0x1C000 length 0x4000** (nvlist region), hand-rolled nvlist parse (0x40-byte entries, type-9 recursion into vdev_tree, BE u64 accumulate); uberblock ring walked with **0x80-qword (1 KiB) stride**, 0xBAB10C magic, **txg newest-wins**; delay-execution polling during long walks | `zfs-vdev--mftmirr--calcpartition.md:8-32`; `refs-btrfs-zfs-engine-algorithms-v2.md:55-83`; `zfs-engine--deep-dive.md` |
| **RAWSearchDLL** | Carvers read through the same volume IO; **query device sector size via vtable slot 5** (4Kn-correct); ZIP analyser initial size estimate `sector_size << 7` (128 sectors = 64 KiB @512) with sector-aligned zero-padding extension capped 5.24 MB; BruteForce-family walks forward to EOF; per-extension runtime gating via `config/RawConfig.ini` (**34 toggles** after CORRECTIONS #5) | `rawsearch-carving--deep-dive.md:70-91`; `rawsearchdll--zip-analyser-carving.md:26,65-71`; `CORRECTIONS.md` row 5 |
| **RAW signature layers** | (1) `raw_signs.json` 139 extensions via hand-rolled istream tokenizer (NOT tinyxml2/CJsonObject — artifact's own correction); (2) EUUserData — **never reaches the carving engine**; sole consumer is DRWUI's CHKRepairFileSign repair chain; `GetRawUserInfoMap` = per-scan **carve-statistics** map (insert-or-increment from ~70 analyser detection paths, cleared at every searcher init, read post-scan by the report builder) — scan OUTPUT, never input (CORRECTIONS #9) | `rawsearch-carving--deep-dive.md:52-68`; `CORRECTIONS.md` row 9 |

**Engine error convention**: HRESULT-style `0x5002xxxx` (0x5002200000004 open-fail,
0x5002c0000004 read-fail family); release-built diagnostics `"Return Error:0x%x
File:%s Line:%d Fuction:%s"`; fixup failure → record skipped, walk continues
(`architecture/05-scan-engine-architecture.md:59-70`; `ntfs-engine--deep-dive.md:153-160`).

---

## §3 Session-level and downstream behavior

### 3.1 The scan-mode state machine (UI-level, decoded + dynamically verified)

- Scan runs **inside `CDetailPage`** (stack idx 3): `scanClickedSlot` prepares `CScanMgr`
  (vft+0x1f0/+0x1f8), hands the target to the detail page, switches the stack, starts the
  scan — scan and results are the SAME page
  (`re-workspace/artifacts/UIUX/04-userflow-state-machine.md:42-68`).
- Progress chrome: `ScaningRecoverMessage` ("%1 file chunks scanned..."),
  `AnalysisGridWidget` sector-grid, `CGenericRecoveryProgress`; strategy objects
  `CDoScanPartition/CDoScanLostPartition/CDoScanCloudDevice/CDoScanRemoteDevice`
  (44 v-methods each) drive `CUIScanProgressInfo`
  (`UIUX/08-scan-phase-deep-dive.md:6-17,49-56`).
- **Live status format (dynamic capture)**: `Advanced Scanning , Files Found: 1139157
  (143.74 GB)` + `Remaining time: 07m43s | Reading sector: 134316032/31413598` — a
  **sector-ticker progress model** with a linear remaining-time estimate
  (`UIUX/dynamic-ground-truth--scan-phase-captured.md:84-107`).
- Quick→Advanced: `CQuickScanCompleteWnd` offers "Recover Found Deleted %1 files %2"
  while advanced continues; auto-recover option engages `CAutoRecoverMgr` state 5
  "Auto_Trigger_Recover" (`08-scan-phase-deep-dive.md:34-36`;
  `architecture/04-recovery-workflow.md:113-115`).

### 3.2 Results/tree-building sequencing relative to reads

1. Engine controllers ARE tree builders (multiple inheritance; `DetourDirectoryTree`
   decorator intercepts node insertion to feed progress + dedup + raw/structured merge)
   — `architecture/05-scan-engine-architecture.md:45-57`.
2. Results stream incrementally: the vendor's own tree-builder log shows
   `FinishSearch → "prepare to create 3 new file system tree" → CreateNewFileSystemTree →
   BuildTree cycles ("show file system node" ⇄ "build raw file node")` in **~1000-item
   incremental batches**; a 223M-item cumulative workload processed this way; the
   **3-branch tree (existing / "File Path Lost" / "Reconstructed")** named by the vendor
   log, matched byte-exact in .rsf and live in the screenshot — three evidence channels
   (`config-files/qa-logs--session-forensics.md:44-57`;
   `UIUX/dynamic-ground-truth--scan-phase-captured.md:43-61`).
3. Browse-while-scanning: UI fully interactive mid-scan; the Reconstructed branch grows
   live (1→16 during 6 min of advanced scanning) — RAW carving results stream in as found
   (`dynamic-ground-truth--scan-phase-captured.md:55-57, 104-107`).
4. Post-organize pipeline order (all off-thread): **classify** (`CSetClassifyThread`,
   FileFilter.xml — **8 categories / 1,481 extensions** after CORRECTIONS #5) →
   **validate** (`CCheckNodeThread`s / `CheckAnsyncToListThread`, per-node recoverability)
   → **filter** (`CSetFilterThread` + `CAdvanceFileFilterMgr`) → **sort** (`CSortNodeThread`)
   → selection/thumbnails (`architecture/04-recovery-workflow.md:97-114`).
   `CCheckNodeThread` runs at **TimeCriticalPriority** (corpus worklog.md:609).
5. **Grading**: `CRecoverability` — a background-thread pipeline
   (`_LaunchPad<tuple<void(__cdecl*)(void*), CRecoverability*>>`, log "Recoverability
   analyze begin.") grading per node: size<1 → 0; type 0xC (ZFS) → 2; FS-code family
   {3,10,0xB} mapping; device.kind {1,4} → offset −2; pluggable `ICheckRecoverable`
   checkers; **per-selection re-grading** at UI time (the "Recovery Chances" gauge =
   CCheckNodeThread visualized) —
   `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §2,
   `UIUX/dynamic-ground-truth--scan-phase-captured.md` worklog cross-refs.
   The engine already computed the **readable-extent ratio at scan time** (NTFS bitmap
   cross-check, §2 above) — the og grades twice: engine-time extent ratio, post-scan
   CRecoverability pass.
6. Scan finish: `sigScanFinished` → view-manager slot 15; the completion modal + stats
   grid ("Files found · Files scanned · Used Time · Size · Organized · …");
   telemetry chain `bottomarea.cpp:838/856 (nStatus=4, m_iScanType 3->4)` then
   recoverability passes (corpus worklog.md:602-604).

### 3.3 Session persistence (.rsf) — format + autosave behavior

- Container: 16-byte magic `EA F4 B3 1B…`, 0x24-byte record headers chaining
  **zlib-chunked** records (u16 chunk header: bit15 = compressed, bits0-11 = size−1,
  ≤4096 B payloads; CZlibCompressor level 5, 512 KB staging, 8,190 B output; decompress
  failure → raw memcpy fallback). **Not encrypted** (corrections register §1 of the
  artifact). 4 records: session state / volume info / partition context (incl. **512-byte
  boot-sector snapshot** + disk identity) / the full result tree (CFileNodeRS::SaveFileTree
  BFS, 34-byte base node grammar, 10 per-FS IFileProperties classes, NTFS-style datarun
  persistence) — `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:24-107,241+`
  and `rsf-record4--tree-node-grammar.md`.
- **Autosave**: `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` written by the auto-save
  path (all 3 shipped samples are auto-saves; `FUN_140273940` enumerates `auto_*`;
  SaveScan folder = module dir + "SaveScan"; filename sanitizer `/ * : ? " < > |` → `_`)
  — `rsf-format--scan-session.md:443-451`. Dynamic telemetry: **Freeze → auto_*.rsf
  auto-save → Unfreeze → CScanMgr::ScanFinished:636** (corpus worklog.md:602-604).
- Record 2 carries elapsed-ms + a TSC/QPC-style tick counter (stat1 239,480 ms ≈ 4 min;
  stat2/stat1 = 3.431 GHz) — the og records per-session wall time in the session file
  (`rsf-format--scan-session.md:171-176`).
- Load/import: "Load Scan Session" / "Import Scan Result" re-materializes the tree
  without rescanning (`architecture/04-recovery-workflow.md:156-165`).

### 3.4 Recovery (read-side) sequencing

`CRecoveryMgr::ExportThread`: cancel checks → destination validation (network vs local;
local opens **`\\.\X:` raw** for writability + same-volume detection) → per-target export
loop resolving nodes through the global item manager, streaming content **from the
engine/scan-session abstraction (IFileProperties/IInWindowStream), which for damaged
volumes reads through EPMNtDrv raw sector IO**; failures collected for the recovery log;
progress forced to 100.0 on completion (`architecture/04-recovery-workflow.md:127-154`).

---

## §4 Cancellation and error behavior as documented

| Behavior | og (documented) | Citation |
|---|---|---|
| Engine cancellation | **Cooperative per-record/cluster polls**: NTFS deleted walker `cancel_callback.vt[0x10]()` per record; HFS+ B-tree walker `cancel.vt[0x10]()` per node + EOF check; FAT32 chain loop `cancel.vt[0x88]()` per cluster step | `ntfs-engine-algorithms-v2--deleted-viability.md:18`; `ext4-hfs-engine-algorithms-v2.md:71-75`; `fat-exfat-engine-algorithms-v2.md:37` |
| UI stop flow | `CStopScanThread` cooperative cancel + `CScanCommonPopDlg` stop-confirm modal ("Stop searching for lost data?" + risk framing); **Pause/Stop controls** in the scan bottom bar; PAUSED + resumed states dynamically captured | `UIUX/08-scan-phase-deep-dive.md:27-31,55`; corpus worklog.md:744-746 |
| Pause | Pause (❚❚) / Stop (■) controls observed live; pause-resume cycle captured | `UIUX/dynamic-ground-truth--scan-phase-captured.md:99`; worklog.md:744 |
| Mid-record failure | Fixup failure → **skip record, never abort the walk**; bad FAT chains bounded by the RB-tree cycle guard + 64 KB chain guard; EXT4 CatchBuffer retries with 1 KiB-backtrack shifted window then salvages | `ntfs-engine--deep-dive.md:158-160`; `fat-exfat-engine-algorithms-v2.md:44-45`; `ext4-engine--deep-dive.md:25-33` |
| Bad sectors | UI copy **"Program is trying to skip bad sectors to find data."** (BadSectorTip + nTipsDlg coaching); prefetch slots zero-filled on read failure | `config-files/config-data-reference.md:55`; `DRWUI_decompiled.c:962642-962647` |
| Drive removed mid-scan | "Drive Disconnected — Unable to continue scanning. Reconnect your drive and try again." Retry/Cancel modal; parallel NAS/Linux/Cloud variants | `UIUX/08-scan-phase-deep-dive.md:38-47` |
| Device absent at enumeration | `ERROR_NOT_READY (0x15)` on `\\.\PHYSICALDRIVE%d` open → close + bail out of disk-info init | `DRWUI_decompiled.c:176880-176891` |
| Driver-open failure (live) | Silent NULL: error stashed at +0x18, device closed, IOCTLs on it → error 5; no retry/fallback at this layer | `Device_decompiled.c:56460-56463`, `:55635-55637` (REFERENCE-ACCESS §1.3) |
| Write-protected media | `IOCTL_DISK_IS_WRITABLE (0x70024)`; ERROR_WRITE_PROTECT (0x13) short-circuits + closes | `Device_decompiled.c:52986-52997`; `DRWUI_decompiled.c:318413-318417` |
| Recovery cancellation | `CRecoveryMgr::doCancelRecovery` (RTTI trace name) | `DRWUI-exe--main-ui-architecture.md:226` |
| Volume in use | Scan opens are **shared** (share 3/7) — coexists with the running OS, no lock during scan; real locks only in the pending-op engine / PE hook driver | `Device_decompiled.c:57101`,`:56456` (REFERENCE-ACCESS §5) |
| Crash | DRWLoader watchdog → DRWReport (minidump + Elapsed/Error/Result JSON) → DRWUI restart; 2 MB shared log flushed every 5 s | `architecture/04-recovery-workflow.md:176-179` |

---

## §5 First-pass parity checklist (core @ e6ee97da, direct-only v0.7.0)

Status legend: **[M]** MATCHES-CORE · **[D]** DIVERGES · **[U]** UNKNOWN (needs core check)
· **[DD]** DELIBERATE-DIVERGENCE (registered / by design). Corpus cite first, core
pointer second. This feeds the session's PARITY-DELTAS work.

### Access & transport

1. **[DD]** Two-path architecture (driver live / direct WinPE, IsWinPE gate) — core is
   direct-only (v0.7.0 removal). Corpus: REFERENCE-ACCESS §1.2; core:
   `src/win/cr_transport.cpp:359-370` (open_disk direct comment).
2. **[M]** Direct opens `\\.\PhysicalDriveN` GENERIC_READ, share R|W|D, OPEN_EXISTING,
   FILE_ATTRIBUTE_NORMAL, buffered — matches the og's direct-branch posture (og data path
   also requests GENERIC_WRITE on volume objects; read-path equivalent).
   Corpus: `Device_decompiled.c:56456`, REFERENCE-ACCESS §2.1; core:
   `src/win/cr_transport.cpp:290-293`.
3. **[D]** Positioning mechanism: og = `SetFilePointer`+synchronous `ReadFile` (loop
   continues across partial reads, returns sectors); core = one OVERLAPPED per call
   (position-independent, shared-handle safe), envelope must cover the request exactly.
   Behaviorally equivalent; partial-read-tolerance semantics differ. Corpus:
   `Device_decompiled.c:56961-57038`; core: `cr_transport.cpp:53-82`, `:191-218`.
4. **[DD]** 4 MiB disk / 7 MiB volume transport chunking — core issues caller-length
   reads (engine path 256 KiB cache fills; ≤1 MiB+64 KiB bounce guard). Registered
   OQ-S4-3 (core measured faster at 256 KiB). Corpus: `Device_decompiled.c:56545`,
   `:56912`; core: `cr_scan.cpp:1298`, `cr_transport.cpp:199-207`.
5. **[M]** All reads 64 KiB-multiple in size and offset (og by construction; core by the
   envelope). Corpus: `NTFSSearchDLL_decompiled.c:5649-5678` + chunk census (§1.2);
   core: `cr_transport.cpp:152-190`, `include/cr/storage/cr_directalign.h:48-73`.
6. **[M]** Geometry discovery: GET_LENGTH_INFO preferred, geometry-EX, storage-query
   access-alignment, 512 fallback, ceil sector count. Corpus:
   `Device_decompiled.c:56503-56551`; core: `cr_transport.cpp:96-152`.
7. **[U]** Layout-IOCTL growing-buffer retry (1000 B + 640/retry) — core enumeration
   table parse not re-checked this pass. Corpus: `Device_decompiled.c:50691-50730`.
8. **[M]** 200-disk namespace cap. Corpus: `epmntdrv-trio--naming-pid-handshake.md:107`;
   core: `include/cr/win/cr_winenv.h` ENUMERATE_DISK_BOUND=200.
9. **[D]** Enumeration by WMI MSFT_* — core uses its own PhysicalDrive loop + cfgmgr32
   two-stack merge (deliberate clean-room control path). Corpus:
   `Device_decompiled.c:17024+`; core: `src/win/cr_enumerate.cpp:115+`.
10. **[M]** ERROR_NOT_READY → device-absent bail in the info layer. Corpus:
    `DRWUI_decompiled.c:176880-176891`; core: enumerate path (check `cr_enumerate.cpp`
    NOT_READY handling).
11. **[U]** Writability probe + ERROR_WRITE_PROTECT short-circuit (destination arming).
    Corpus: `Device_decompiled.c:52986-52997`; core: `src/win/cr_destio.cpp` (verify).
12. **[DD]** F11 Information scaling (`Information = Length << 9`, driver reports
    bytes-for-sectors) — the RDA wire quirk is GONE in direct mode by construction.
    Corpus: `eprdtdrv-sys--driver-internals.md:68-71`; core history:
    `include/cr/win/cr_rda.h:63-65` (kLegacyInformationScale), removed from
    `overlapped_read` (`cr_transport.cpp:53`).
13. **[DD]** Driver-path IOCTL emulation + fake-success FSCTL_DISMOUNT — N/A; the core
    does a REAL quiesce chain (lock → dismount-point-delete → flush → settle → offline).
    Corpus: `Device_decompiled.c:45138-45220`; core: `cr_transport.cpp:396-445` (RwDevice
    lock_volume, M-030).
14. **[M]** Handle self-healing (fresh CreateFileW of the same path on ERROR_INVALID_HANDLE,
    swap under lock). Corpus: `Device_decompiled.c:55947-55959` (og parity note);
    core: `cr_transport.cpp:264-277`, `:241-253`.

### Caching & prefetch

15. **[DD]** Cache budget: og **4 MiB** (`0x400000/ss`, fallback 0x2000) vs core **8 MiB**
    (32×256 KiB). REGISTERED N-045/D-72. Corpus: `DRWUI_decompiled.c:919080-919092`;
    core: `cr_scan.cpp:1298`, register comment `cr_cached.h:9-35`.
16. **[DD]** Prefetch: og **dedicated CacheThread** (CreateThread, 1 ms event poll,
    10-run × 1000-sector ring, zero-fill failed slots, CRITICAL_SECTION, bitmap-gated
    prefetch) vs core **reactive true-LRU, no prefetch thread**. Same register.
    Corpus: `DRWUI_decompiled.c:962250-962682`; core: `cr_cached.cpp:30-89`.
17. **[D]** Cache construction scope: og once per physical disk shared across partition
    scans + engine-side cluster retune (`NTFSSearchDLL_decompiled.c:7165-7169`,
    `:11752-11846`); core builds a read pipe **per slice** with fixed 256 KiB blocks.
    Core: `cr_scan.cpp:164-203`.
18. **[D]** Failed-block policy: og zero-fills the failed prefetch slot and continues;
    core does NOT cache failed blocks — the error propagates to the retry layer and the
    next read re-attempts. Corpus: `DRWUI_decompiled.c:962642-962647`; core:
    `cr_cached.cpp:62-68` (audit row 38).

### Engine read patterns

19. **[D]** NTFS MFT acquisition: og 64 KiB bootstrap buffer + 64 KiB record batches +
    **2 MiB growth-probe reads** (`0x1000` sectors/call); core 512 B boot probe + 4 KiB
    run-growth windows + record-granular walk through the 256 KiB cache. Functionally
    equivalent (the cache converts record-granular to block-granular); read-size profile
    differs. Corpus: `ntfs-engine-internals.md:28-41`, `NTFSSearchDLL_decompiled.c:5649-5678`;
    core: `src/core/ntfs/cr_bootstrap.cpp:526-581`, `cr_mftwalk.cpp:415-524`.
20. **[M]** NTFS record-size clamp ≥ 0x400. Corpus: `ntfs-engine--deep-dive.md:62`;
    core: bootstrap clamp (CONSUMERS §2.2).
21. **[M]** NTFS deleted-viability strict `0x3ff < size` fast-path boundary (CORRECTIONS
    #7 folded). Corpus: `CORRECTIONS.md` row 7; core: pinned by AUDIT-NTFS NTFS-1.
22. **[M]** $ATTRIBUTE_LIST fragmented-MFT fork. Corpus: `ntfs-engine--deep-dive.md:50-58`;
    core: `cr_mftwalk.cpp:579`, `cr_attributes.cpp:204`.
23. **[M]** $Bitmap whole-volume load for quick scan. Corpus:
    `ntfs-engine-internals.md:57-61`; core: `cr_bitmap.cpp` (S-009).
24. **[U]** $MFTMirr record-1 $DATA-runlist fallback (og two-layer MFT resilience).
    Corpus: `zfs-vdev--mftmirr--calcpartition.md:34-60`; core: mirror handling not
    confirmed this pass.
25. **[M]** FAT whole-table load FAT1→FAT2 with mirror validation/promote. Corpus:
    `fat-exfat-engine-internals.md:31-32`; core: `cr_chain.cpp:130-169`.
26. **[M]** FAT whole-run batched directory reads + cycle guard. Corpus:
    `fat-exfat-engine-algorithms-v2.md:36-58`; core: `cr_fat_quick.cpp:26-52` (4 MiB
    DirBuffer cap — core-specific bound; og bound not documented).
27. **[DD]** FAT12 BAD-cluster handling: og follows 4087 raw; core canonicalizes
    (registered delta, not an og-behavior claim — CORRECTIONS #3).
28. **[M]** exFAT dentry-triple InUse-mask reconstruction. Corpus:
    `fat-exfat-engine-algorithms-v2.md:60-78`; core: exFAT engine (parity-audited).
29. **[U]** exFAT "1 MiB FAT windows" — **not documented in corpus** (see §2); if the
    core implements 1 MiB FAT windowing it is a core-side choice, not og parity.
30. **[M]** EXT4 CatchBuffer semantics: retry with fresh buffer + **1 KiB backtrack
    shifted window** on read failure. Corpus: `ext4-engine--deep-dive.md:25-33`; core:
    RetryingDevice backtrack step `max(0x400, logical_sector)` — `cr_scan.cpp:179-183`,
    `src/core/storage/cr_retry.cpp:83-99`.
31. **[DD]** Bad-sector retry scope: og CatchBuffer is **EXT4-engine-specific**; the core
    applies backtrack+skip to ALL engines (deliberate generalization).
32. **[U]** EXT4 backup-superblock probing — corpus says the positions are UNVERIFIED for
    the engine (CORRECTIONS #4); core must not cite og parity for any backup-SB walk.
33. **[U]** EXT4 journal gate = magic + blocktype {3,4} only (CORRECTIONS #6) — core
    journal salvage parity not re-checked this pass.
34. **[M]** HFS+ gates (H+/HX, 512-multiple block size, version<0x11). Corpus:
    `ext4-hfs-engine-algorithms-v2.md:50-61`; core: hfsplus bootstrap.
35. **[M]** Btrfs newest-generation superblock selection across mirror offsets (0x10000
    + mirrors), 4 KiB probe reads. Corpus: `refs-btrfs-zfs-engine-algorithms-v2.md:35-53`;
    core: btrfs engine.
36. **[M]** Btrfs: **no chunk-tree-uuid validation** (CORRECTIONS #8 — core must not add
    one). Corpus: `CORRECTIONS.md` row 8.
37. **[D]** Btrfs `BtrfsCache.txt` persistent metadata cache for re-scans — no core
    equivalent (feature gap, low priority). Corpus:
    `remaining-engines--ext4-hfs-refs-btrfs-zfs.md:66-67`.
38. **[M]** ZFS label read 0x1C000/0x4000, 1 KiB uberblock stride, txg newest-wins.
    Corpus: `zfs-vdev--mftmirr--calcpartition.md:8-32`; core: zfs engine.
39. **[M]** ReFS SUPB/CHKP/MSB page-signature chain. Corpus:
    `refs-btrfs-zfs-engine-algorithms-v2.md:7-33`; core: refs engine.
40. **[M]** RAW carving: signature layers + per-format grammar walks + confidence
    ranking; carvers query the device's real sector size (4Kn-correct). Corpus:
    `rawsearch-carving--deep-dive.md`; core: carve engine + raw_signs port (64 KiB
    envelope subsumes the sector-size query).
41. **[M]** EUUserData is NOT a carving input; `GetRawUserInfoMap` is scan-output
    statistics (CORRECTIONS #9 — core must keep userdata out of carving). Corpus:
    `CORRECTIONS.md` row 9.
42. **[U]** RAW per-extension runtime toggles (og RawConfig.ini, 34 toggles after
    CORRECTIONS #5) — core carve-format gating surface not re-checked this pass.
43. **[D]** `fastsearch` ini default-ON config gate (og quick pass is configurable via
    Config.dat) — core has no runtime config file gate. Corpus:
    `NTFSSearchDLL_decompiled.c:7163`.
44. **[U]** Memory-adaptive batch sizing (GlobalMemoryStatus-driven) — no core
    equivalent documented. Corpus: `ntfs-engine-internals.md:70-71`.

### Concurrency

45. **[D]** og serializes all sector reads through one per-volume device (QD1
    per-volume); core runs a worker pool (auto = hw−1 clamp 1..8) fanning out carve
    windows / remnant chunks / lost candidates — concurrent callers on one shared
    direct handle (tolerated by design, thread_local bounce). Deliberate performance
    architecture; the per-read contract stays synchronous. Corpus:
    REFERENCE-ACCESS §6; core: `cr_scan.cpp:58-68`, `include/cr/cr_parallel.h:40-87`,
    `cr_transport.cpp:204-207`.
46. **[M]** Engine-level concurrency posture (structured + carving concurrent; no
    reader-pool in the og; UI never blocks). Corpus: `architecture/04-recovery-workflow.md:73-74`;
    core: pool model + cooperative abort.

### Session & downstream

47. **[M]** Two-phase staging: quick (FS metadata) → advanced (all engines + RAW,
    background continuation). Corpus: `08-scan-phase-deep-dive.md:34-36`; core:
    quick/deep/deepcarve modes.
48. **[M]** 3-branch results tree (existing / File Path Lost / Reconstructed).
    Corpus: `qa-logs--session-forensics.md:44-57`; core: session tree roots.
49. **[D]** Incremental ~1000-item tree batches + browse-while-scanning — core is
    library/CLI: results materialize per-stage, no live UI tree. Downstream-parity item
    only if a UI lane is built. Corpus: same as 48.
50. **[D]** Autosave `auto_*.rsf` at scan finish (Freeze→autosave→ScanFinished) — core
    session save is explicit (verb/API); no auto-save on completion confirmed. Corpus:
    `rsf-format--scan-session.md:443-451`, worklog.md:602-604; core: docs/crsf-format.md.
51. **[DD]** Session file format: og .rsf (zlib-chunked, 4 records, boot-sector
    snapshot, BFS tree grammar) vs core CRUNDO/crsf clean-room formats — deliberate
    divergence (documented in core docs).
52. **[D]** Grading sequencing: og grades engine-time (bitmap extent ratio) AND
    post-scan (CRecoverability background pass + per-selection re-grade,
    ICheckRecoverable pluggable); core grades during scan (check module + bitmap ratio),
    no post-scan re-grade pass. Corpus:
    `DRWUI-subsystems--assistant-grading-preview.md` §2; core: `src/core/check/`.
53. **[D]** Organize-pipeline order: og classify→check→filter→sort after tree build;
    core runs the check module pre-scan and classifies during scan. Corpus:
    `architecture/04-recovery-workflow.md:97-114`; core: `cr_scan.cpp` staging.
54. **[M]** Lost-partition candidates inserted as scan targets with
    StartSector/TotalSector; cap 10 (og cap matched). Corpus:
    `architecture/04-recovery-workflow.md:66-69` + core `cr_scan.cpp:2300-2310`
    (kLostInsertCap=10, "the og's cap").
55. **[U]** Candidate scoring by recoverable-file count + `AdjustPartitionInfoOrder`
    ranking — core candidate scoring parity not re-checked this pass. Corpus:
    `qa-logs--session-forensics.md:37-42`.
56. **[D]** RAW naming callback (`std::function` via vtable+0x48 — naming policy lives in
    the host UI, not the engine) — core names carves internally (no host/UI split).
    Corpus: `rawsearch-carving--deep-dive.md:70-77`.
57. **[M]** Recovery source reads stream through the engine/session abstraction (extent
    reads on demand), destination validated by raw `\\.\X:` open. Corpus:
    `architecture/04-recovery-workflow.md:127-154`; core: SafeWriter + destio
    (`cr_destio.cpp`).
58. **[D]** Error surface: og HRESULT-style 0x5002xxxx engine codes + silent-NULL device
    opens; core has its own error taxonomy + explicit codes (deliberate improvement).
    Corpus: `05-scan-engine-architecture.md:61-63`; core: `include/cr/cr_error.h`.

### Cancellation & errors

59. **[M]** Cooperative cancel polled at loop granularity (per record / per cluster /
    per node). Corpus: §4 row 1; core: `CancelToken::checkpoint()` at engine loop heads
    (`include/cr/storage/cr_device.h:144-167`), per-block/per-record polls
    (`cr_mftwalk.cpp:482-483`).
60. **[M]** Cancel prevents QUEUED work from starting (og: engine-chain stop; core:
    CancelPoolWatcher 50 ms poll → pool abort). Corpus: `08-scan-phase-deep-dive.md:55`;
    core: `include/cr/cr_parallel.h:106-127`.
61. **[D]** Pause/resume: og has user-facing Pause (❚❚) mid-scan; core is pause-aware at
    checkpoints (G-07) but exposes no CLI pause verb — verify surface.
62. **[M]** Bad-sector skip-and-continue with zero-fill salvage + UI tip copy. Corpus:
    §4; core: RetryingDevice Skip mode (zero-fill surface, never abort).
63. **[M]** Fixup/record failure → skip, never abort the walk. Corpus:
    `ntfs-engine--deep-dive.md:158-160`; core: retry/skip chain.
64. **[M]** Disconnect-classified failure (drive removed → abort with dialog; core maps
    GLE→ERR_DEVICE_DISCONNECTED in `overlapped_read`, `cr_transport.cpp:60-75`).
    Corpus: `08-scan-phase-deep-dive.md:42`.

**Tally: 64 items — [M] 31 · [D] 15 · [U] 9 · [DD] 9.** (Items may be re-graded by the
PARITY-DELTAS session; [U] items are check-tasks, not gaps.)

---

## §6 Performance-relevant corpus facts

### 6.1 Measured og performance (corpus-captured)

| Fact | Value | Citation |
|---|---|---|
| Quick pass wall, NTFS partition (270,977,023 sectors ≈ 129 GiB) | **~21-26 s** (`22:57:57 doScan → 22:58:23 UpdateScanFinishedInfo`) | `qa-logs--session-forensics.md:25-30` |
| Full scan session elapsed (vendor QA I:) | **239,480 ms ≈ 4 min** (record-2 stat1) + 3.431 GHz tick counter (stat2) | `rsf-format--scan-session.md:171-176` |
| Full scan, 149.45 GB runner disk | **880 s (14.7 min)**, 1,139,286 files / 147.12 GB (≈98.5% of capacity enumerated) | `UIUX/dynamic-ground-truth--scan-phase-captured.md:111-138` |
| Scan progress curve (same run) | 9% @ t=75 s → 34% @ t≈460 s; remaining-time 07m43s @ 34% — **linear sector-throughput model** ("Reading sector: 134316032/…") | same, `:84-107` |
| Tree-builder scale | 223M-item cumulative workload processed in ~1000-item incremental batches (stress-scale QA disk) | `qa-logs--session-forensics.md:44-57` |
| Quick→Advanced handoff | Quick done (1.11M files) while advanced at 9%; Reconstructed branch grows 1→16 over 6 min (carve streaming rate ≈ 2.5/min on that workload) | `dynamic-ground-truth--scan-phase-captured.md:43-56, 94` |

### 6.2 The og's I/O shape (what the optimizer is matching against)

- Transport chunks: **4 MiB (disk) / 7 MiB (volume)**, sector-normalized
  (`Device_decompiled.c:56545`, `:56912`).
- Engine-side: **64 KiB** MFT bootstrap/batches; **2 MiB** MFT growth probes; whole-FAT
  and whole-run batched reads; 4 KiB Btrfs SB probes; 16 KiB ZFS label reads; every
  issued size a **64 KiB multiple** (census in §1.2).
- Cache: **4 MiB budget / 1000-sector runs / 10-run look-ahead ring + dedicated
  prefetch thread (1 ms poll) / CRITICAL_SECTION** (`DRWUI_decompiled.c:919080-919092`,
  `:962250-962682`) — the D-72 counterpart.
- Concurrency: **serialized per-volume QD1 reads**; parallelism only at engine/UI level.
- F11: driver returns `Information = Length << 9` — the sector-vs-byte reporting quirk
  the core's F11 compensation historically keyed on (`eprdtdrv-sys--driver-internals.md:68-71`);
  gone in direct mode.

### 6.3 Core-side measured evidence (for cross-reference; core repo)

- **Run 40315255368** (alignbench, OQ-S4-3): og-model 4/7 MiB chunking 197.32/196.15 MiB/s
  vs core 64 KiB envelope 203.98 and **256 KiB 241.64 MiB/s (+18-19% at lower CPU)** —
  the core's smaller reads are already ahead of the og's shape
  (`worklog/PERFORMANCE.md:103-105`).
- **Run 40227375329**: read-size sweep plateau ~240 MiB/s (128/132 KiB ≈ flat); the one
  historical direct-vs-driver gap (151.5 @ 256 KiB, pre-envelope) — retired-by-remeasure
  duty (`PERFORMANCE.md:94-101`).
- **GAP-S6-13**: 4-KiB-multiple-but-not-64-KiB lengths take the ~3.5× storage-stack slow
  path (~830 vs ~2900 MB/s NVMe) — the reason both the og's and the core's sizes are
  64 KiB multiples (`src/win/cr_transport.cpp:166-171`).
- Driver-era reference (run 41042267586): seq 1 MiB 536.92 MiB/s; deepcarve 8 GiB
  1,501,622 ms — the direct comparison rows are pending in `PERFORMANCE.md` §2.

### 6.4 Optimization-plan implications (first-pass, from corpus facts only)

1. The og's prefetch thread exists to hide **sequential read-ahead latency under its
   serialized QD1 volume device**. The core already gets equivalent-or-better
   sequential service from 256 KiB aligned reads (6.3) plus pool parallelism — the D-72
   divergence is measured, not theoretical; keep it benchmark-gated as registered.
2. The og's 2 MiB MFT growth probes and 64 KiB record batches ride the same prefetch
   ring; the core's 4 KiB probes + 256 KiB cache fills issue smaller reads — if the
   runner shows the core's MFT phase slower than ~21 s per 129 GiB equivalent, the
   growth-probe size (4 KiB → 2 MiB-class) is the corpus-documented knob.
3. The og never reads in non-64-KiB multiples; any new core read path must preserve the
   envelope (sector-rejection evidence, run 40073093997).
4. The og grades recoverability post-scan on a background thread; scan-time throughput
   is therefore not paying the grading cost — the core's scan-time check module should
   be compared against that split if scan walls regress.

---

## §7 Coverage / not-covered register

- **Covered**: architecture/00-07 docs; SearchDLL family + all per-engine deep-dives and
  algorithm-v2 artifacts; eprdtdrv internals + trio + hook drivers; DRWUI main/thread/
  subsystems artifacts; UIUX 04/08 + scan-phase ground truth; SaveScan rsf + record-4 +
  IFileProperties parts; config-files (raw_signs/RawConfig/EUUserData/FileFilter/
  filtering-priority); qa-logs + vendor-logs forensics; CORRECTIONS.md rows 1-9; corpus
  worklog.md tail; first-hand re-verification of the D-72 cache/prefetch decompile sites
  and the 2 MiB MFT growth probe.
- **Not covered this pass** (with reason): the 17 FileRepair binaries and repair chain
  (out of scope: post-recovery, no disk-read parity impact); Cloud/NAS/EuMrx chains
  beyond the access map (remote transports, not local raw I/O); per-binary stubs for
  ~110 binaries (identity-level only, no I/O content); the 1 MiB $UsnJrnl decompile site
  (core-cited, not re-pinned — flagged in §1.2); the prefetch bitmap source
  `FUN_1404ecb50` (hypothesis flagged in §1.3); ghidra-mo/DeviceManager &
  ghidra-partition beyond the sites cited by REFERENCE-ACCESS (relied on that sheet's
  line-cited verification, which this session found no reason to overturn).
- **Explicit corpus absences** (stated so nobody hunts them): no throughput/latency
  benchmarks of the og's own transport (only wall-clock scan timings §6.1); no parallel
  read engine; no exFAT "1 MiB FAT window" constant; no numeric memory-adaptive batch
  table; no 4Kn end-to-end driver-path demonstration; no "RDA" term (core-internal
  vocabulary, REFERENCE-ACCESS C-6).

*Corpus untouched. One file written: this one.*
