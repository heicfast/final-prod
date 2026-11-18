# REFERENCE-ARCHITECTURE — the og's documented I/O architecture (corpus ground truth)

> Agent B, performance-diagnostic session (Task 2-B). Corpus: `/home/z/my-project/easeus-drw-research`
> @ HEAD `be0cfe9`. Scope: the **architecture-level view of the og's storage/device I/O** — what
> the corpus's architecture docs and deep artifacts *document* about how EaseUS Data Recovery
> Wizard 21.0.0.0_free (build Oct 13 2026, git `40504ccb…`, `og-codebase/log/DRWUI.log` line 2 per
> `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` §1) opens disks, sizes reads,
> threads, queues, caches, cancels, and treats transports.
>
> Discipline (non-negotiable): every claim carries `file:line` or `file §section` into the corpus.
> Tags: **[OBSERVED]** = stated in a corpus artifact/.md; **[CODE xref]** = first-hand read of
> corpus decompiled C / CI import tables, quoted with line numbers (raw-parameter census is Agent
> C's `worklog/REFERENCE-SCAN.md` — these are cross-references, kept to what this sheet needs);
> **[INFERRED]** = reading consistent with cited evidence; **[NOT-IN-CORPUS]**/**[UNKNOWN]** =
> absent — never invented. Product-side files (`final-prod/…`) are cited only as divergence
> registers, never as ground truth. CORRECTIONS.md rows supersede older artifact text where cited.

---

## 0. Division of labor

- This sheet = documented architecture (the og's I/O model as the corpus records it).
- Agent C's `REFERENCE-SCAN.md` = raw decompiled parameter census of the same pipeline (its §7
  "Sector access & performance" overlaps deliberately; where both sheets cite the same fact I
  cite the corpus artifact here and point there for the instruction-level walk).
- final-prod `engine/include/cr/storage/cr_cached.h` (N-045 / D-72 register) records our cache
  divergence against this architecture — referenced in §5/§6 as the product-side reading.

## 1. The documented I/O layering (who reads the disk)

**Volume IO layering** [OBSERVED, `architecture/05-scan-engine-architecture.md` §8, lines 116-125]:

```
Engine tree reader
  └─ CPartitionVirtualVolumeDevice (works on lost partitions)
       ├─ normal file API when volume is mounted & healthy
       ├─ raw IOCTL path → EPMNtDrv (\\.\EPRDTDRV\<d>\<p>) — MDL async sector IO, 512B sectors
       ├─ BitLocker: FVE keys (DRWService pipe) → XTS-AES decrypt (BitLockerLib/OpenSSL)
       └─ NAS: EuMrx redirector letter (X:\EuShare.com\... → \\127.0.0.1 tunnel)
```

- `CPartitionVirtualVolumeDevice` — "Virtual" = works for volumes the OS can't see
  (lost/deleted partitions); geometry read from partition context, then a volume reader
  `FUN_18000af90(device, 0, totalSectors, …)` [OBSERVED, `architecture/05` §4, lines 63-66].
- In front of the device abstraction sits the read cache: `CStorageDeviceWithCache` →
  `IStorageDevice`, "to survive MFT re-reads economically" [OBSERVED,
  `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §8, lines 143-149; class in RTTI
  `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md:24` and
  `re-workspace/artifacts/per-binary/NTFSSearchDLL.md:48`].
- Engines receive the device abstraction from the host; the NTFS engine *detects the cache
  wrapper at runtime*: `__RTDynamicCast(device, IStorageDevice → CStorageDeviceWithCache)` then
  branches into `FUN_18000fae0` (single-sector boot probe + geometry re-read) [CODE xref,
  `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:7165-7170`].
- RAW carving reads flow "through the same `CStorageDeviceWithCache`-style volume IO as FS
  engines (IOCTL/EPMNtDrv raw path on damaged volumes)" [OBSERVED,
  `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:80-81`].
- Recovery export: "source reads come from the engine/scan-session abstraction
  (vtable-mediated IFileProperties/IInWindowStream), which for damaged volumes read through
  EPMNtDrv raw sector IO; writes go to destination via normal file APIs" [OBSERVED,
  `architecture/04-recovery-workflow.md` §7.5, lines 147-151].
- Destination validation opens the raw volume `\\.\X:` (writability + same-volume detection),
  falling back to `PathFileExistsW`; network destinations take attribute checks [OBSERVED,
  `architecture/04` §7.3, lines 137-140].
- Repair rides the same device layer: video repair builds "a self-built sector bitmap
  ('bitmap create by self', 'set to bmp:start sector %lld count: %lld')" and MTS/M2TS does
  "raw stream finding at DEVICE offset — device-level scan" [OBSERVED,
  `architecture/06-repair-chain.md` §3, lines 42-48].

## 2. Device open & handle model (documented)

- User-mode opens `\\.\EPRDTDRV\{disk}\{part}` or `\\.\EPRDTDRV\{disk}\Volume\{N}`; the driver's
  IRP_MJ_CREATE resolves `\Device\Harddisk{d}\Partition0` (whole disk, absolute LBA) or
  `\Device\HarddiskVolume{N}` top-of-stack, 16-byte "esdr" context in FsContext2 [OBSERVED,
  `re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §2, lines 28-50;
  namespace regex `architecture/02-kernel-driver-architecture.md` §2.2, lines 29-43].
- **All sector addressing is caller-side absolute LBA on the whole disk** — the scan engines
  compute partition-relative offsets themselves [OBSERVED, eprdtdrv artifact §2, line 50].
- Driver device: `\Device\EPMNTDRV`, `DO_DIRECT_IO` (MDL-based direct I/O) [OBSERVED, eprdtdrv
  §1, lines 10-16]; read/write path: `IoBuildAsynchronousFsdRequest` with **length in sectors**,
  completion `FltReadWriteSectorsCompletion`, `Information = Length << 9` — **512-byte sector
  hardcoded; 4Kn/512e normalization happens above the driver** [OBSERVED, eprdtdrv §3, lines
  52-73].
- The async loop: submitter builds IRP → disk stack → completion frees MDL chain + bounce buffer,
  writes `{status, info}` back to the submitter slot and `KeSetEvent`-wakes the waiting thread —
  "the synchronous-looking submitter (DeviceIoControl from user mode) unblocks" [OBSERVED,
  `re-workspace/artifacts/PartitionRecovery/epmntdrv-internals-v2--completion-engine-identity-ioctls.md`
  §1, lines 9-40]. I.e. **driver IO is synchronous-from-caller, asynchronous at the disk stack**.
- First-hand import census (corpus CI multitool, per-DLL `rabin2_imports.txt`):
  - NTFSSearchDLL imports `CreateFileW`, `DeviceIoControl`, `SetFilePointerEx`,
    `CreateFileMappingW`, `MapViewOfFile`, plus events/waits and `InitializeCriticalSection{,AndSpinCount}`
    /`Enter`/`Leave`/`Delete` [CODE xref,
    `re-workspace/reversed/ci-multitool/searchdll/NTFSSearchDLL/rabin2_imports.txt:21-44`];
    FATSearchDLL adds `CreateFileA`/`SetFilePointer`/`WriteFile` (:119-139); exFATSearchDLL
    (:160-163), EXT4SearchDLL (:56-63), BtrfsSearchDLL (:46-72) carry the same direct-IO set.
  - **RAWSearchDLL (45 KERNEL32 imports), ReFSSearchDLL (16), HFSPlusSearchDLL (30),
    ZfsSearchDLL (18) have NO CreateFile*/DeviceIoControl/ReadFile imports** [CODE xref, zero
    matches in each `rabin2_imports.txt`] — their reads ride the device abstraction handed to
    them by the host, matching the §1 layering [INFERRED, same evidence as
    `worklog/REFERENCE-SCAN.md` §7 "first-hand import census"].
- The NTFS engine's mapped-file IO: a `PathFileExistsW` check → `CreateFileW(0x10000000, share
  3, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL)` → `DeviceIoControl(0x900c4 = FSCTL_LOCK_VOLUME)` →
  `SetFilePointerEx`/`SetEndOfFile` → `CreateFileMappingW` → `MapViewOfFile` sequence — the
  auxiliary-bitmap backing file (the `euAuxiliaryBitmaplog` artifact family) [CODE xref,
  `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:35506-35545`;
  aux-bitmap concept `re-workspace/artifacts/SearchDLL/exfat-engine--deep-dive.md:28-29` and NAS
  agent's `euAuxiliaryBitmaplog` `re-workspace/artifacts/EuShare/eumrx-nas-architecture.md:125`].
  No `ReadFile`/`WriteFile` imports in NTFSSearchDLL: the mounted-healthy path is
  **memory-mapped views**, not read loops [CODE xref, KERNEL32 block of
  `rabin2_imports.txt` — reading INFERRED].
- `MainWndExitThread` performs "clean exit of engines + threads" [OBSERVED,
  `re-workspace/artifacts/DRWUI-exe--thread-architecture.md:52`].

## 3. Read sizing per stage — every documented size

| Stage | Size | Citation |
|---|---|---|
| Partition scan setup | sector size **default 512**; lost partitions inserted with StartSector/TotalSector | [OBSERVED] `architecture/04` §3.4, lines 66-69; QA log quote `config-files/qa-logs--session-forensics.md:27-28` |
| MFT bootstrap | **64 KB buffer** (0x10000; 16×4 KB or 64×1 KB records); fixup per record | [OBSERVED] `SearchDLL/ntfs-engine-internals.md:33`; `SearchDLL/ntfs-engine--deep-dive.md:44` |
| MFT record size | clamped **≥ 0x400 (1024 B)** — NTFS default | [OBSERVED] `SearchDLL/ntfs-engine--deep-dive.md` §3, lines 60-62 |
| MFT batch walk | **4 KiB pages** via `device.vt[0x08](device, offset, buf, 0x1000, 0)`, record-size-strided fixup re-validation; MFT reads **cluster-strided/aligned, not per-record** | [OBSERVED] `SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §5, lines 125-128; `ntfs-engine--deep-dive.md` §8, lines 150-151 |
| Device/prefetch run (cache) | block count **0x400000/bytes_per_sector** (4 MiB), fallback **0x2000**; ctor default **1000 blocks** | [CODE xref] `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:919078-919089, :962281`; register reading `final-prod/engine/include/cr/storage/cr_cached.h` N-045 |
| Whole-device pass | a first pass `(0, totalSectors-1)` through the cache wraps the deep-scan device setup | [CODE xref] `DRWUI_decompiled.c:919146-919155` |
| $Bitmap/USN-class stream sizing | **2 MiB floor** (0x200000), ~100 MiB region cap (0x6400001), 1 MiB record-stream buffer (0x100000) | [CODE xref] `NTFSSearchDLL_decompiled.c:11539-11546, :20036` — attribution INFERRED (stream-loader family) |
| FAT engine | **0x200 (512 B) sector staging buffer**; sector-size defaulting `param_3 == 0 ? 512 : …` | [OBSERVED] `SearchDLL/fat-engine--deep-dive.md:13-17` |
| Btrfs probe | superblock at **0x10000 (64 KB)** + mirrors 0x40000/0x4000000/… | [OBSERVED] `SearchDLL/btrfs-engine--deep-dive.md:29-31` |
| ZFS probe | vdev labels **4 × 128 KB**; label read `offset 0x1C000, size 0x4000`; uberblocks 0x80 stride | [OBSERVED] `SearchDLL/zfs-engine--deep-dive.md:19-24`; `SearchDLL/zfs-vdev--mftmirr--calcpartition.md:13,25-26` |
| Carve initial guess (ZIP) | `sector_size << 7` = **128 sectors (64 KB @512)** initial size estimate | [OBSERVED] `SearchDLL/rawsearchdll--zip-analyser-carving.md:26` |
| Carve zero-pad extension (ZIP) | sector-aligned zero-padding extension capped **5.24 MB** (0x500001) | [OBSERVED] same artifact, lines 49-56 |
| PST carve | **1 MiB floor** size accept (`0x100000`) | [OBSERVED] `SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md:18` |
| Driver contract | lengths in **sectors of 512 bytes**, `Information = Length << 9` | [OBSERVED] eprdtdrv §3, lines 60-71 |

Memory-adaptive behavior is documented: NTFS engine logs `GlobalMemoryStatus` ("Total physical
memory… Available physical memory… page files…") — "batch sizes adapt to RAM" [OBSERVED,
`SearchDLL/ntfs-engine-internals.md:70-71`; import `GlobalMemoryStatusEx` CODE xref
`rabin2_imports.txt:35`].

## 4. Threading discipline (documented)

- **45 named threads** in DRWUI, grouped by subsystem; "every non-trivial operation
  (sort/filter/classify/select/thumbnail/navigation) is off-thread — the Qt event loop stays
  responsive; browse-while-scanning is a direct consequence" (UI-thread purity) [OBSERVED,
  `re-workspace/artifacts/DRWUI-exe--thread-architecture.md` §1-§2, lines 9-64].
- Scan-side workers: `CStopScanThread` (cancellation worker), `CCheckNodeThread(s)`,
  `CEnumPartitionThread`/`CheckPartitionThread` (device discovery), `CSetClassifyThread`,
  `CSetFilterThread`, `CSortNodeThread`, `CSelNodeThread`, `CGetListThumbnailThread`,
  `SaveRestoreThread` (.rsf writer off-thread) [OBSERVED, same artifact, lines 11-27].
- `InitThread` = "startup worker (device enumeration behind the CStartWindow splash)" [OBSERVED,
  same artifact, line 50]; `CommonThreadPoolMgr` = "the shared thread pool — general-purpose
  work-stealing pool the one-off threads defer to" [line 56]; "45 creation sites vs 40 named
  classes — a few anonymous workers remain (Qt-internal + pool submissions)" [line 64].
- FAT quick search is **threaded per-worker**: nested `CFATQuickSearch::CFATQuickSearchWorker`
  with "thread control at +0x38, progress flags" — "the quick scan is threaded per-worker,
  matching DRWUI's async model" [OBSERVED, `SearchDLL/fat-engine--deep-dive.md:14-17, :74-75`].
- NTFS engine worker coordination uses **Vista+ condition variables**
  (`InitializeConditionVariable`/`SleepConditionVariableCS`/`WakeAllConditionVariable`,
  GetProcAddress-resolved, pre-Vista fallback) [OBSERVED,
  `SearchDLL/ntfs-engine-internals.md:72-74`]; imports confirm the event/wait primitives
  (`CreateEventW`, `WaitForSingleObjectEx`, `ResetEvent`, `SetEvent`) [CODE xref,
  `NTFSSearchDLL/rabin2_imports.txt:21-24`]. Property objects are interlocked AddRef/Release —
  "thread-safe for the UI's async workers" [OBSERVED, `ntfs-engine--deep-dive.md` §3, lines
  84-86].
- Advanced scan composition = a `std::list` of full searchers + RAW searcher created by
  `FUN_140580720` [OBSERVED, `architecture/04` §3.3, lines 61-65] — **the concurrency/overlapped
  fan-out across engines is NOT documented anywhere; [UNKNOWN]** (same call in Agent C's
  `REFERENCE-SCAN.md` §7/§12).
- The dedicated prefetch thread: `CreateThread(0, 0, FUN_1405c7ac0, cache, 0, …)` inside the
  cache ring init [CODE xref, `DRWUI_decompiled.c:962530-962600` — see §5].
- Tree building is incremental: BuildTree cycles in "~1000-item incremental batches
  (show-node/build-raw cycles)" over a 223M-item cumulative workload [OBSERVED,
  `config-files/qa-logs--session-forensics.md` §4, lines 48-56].
- Loader↔UI log protocol runs on named mutex + shared memory (2 MB buffer flushed every 5 s by
  DRWLoader) with read/write completion events [OBSERVED,
  `DRWUI-exe--thread-architecture.md:57` + `architecture/04` §1, lines 26-29].

## 5. Queue depth & concurrency — the prefetch ring

- **No IOCP / no async OVERLAPPED in the scan path**: zero `CreateIoCompletionPort` /
  `GetQueuedCompletionStatus` / `PostQueuedCompletionStatus` matches in the DRWUI + engine
  decompiles; every `OVERLAPPED` argument that appears is `(LPOVERLAPPED)0x0` — NULL =
  synchronous `DeviceIoControl`/`ReadFile` (e.g. `DRWUI_decompiled.c:110002-110025, :169965,`
  `:171000`; `NTFSSearchDLL_decompiled.c:35529`) [CODE xref]; engines import
  `WaitForSingleObjectEx` + events (sync waits), and the driver path blocks in
  `KeWaitForSingleObject` [NOT-IN-CORPUS for async user-mode IO; the only "outstanding" budget
  documented elsewhere is the WinPE copy-engine's "301 outstanding kicks" in EUDCPEPM
  `re-workspace/artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md:122` — a
  different subsystem].
- The read-ahead engine [CODE xref, `DRWUI_decompiled.c:962530-962683`]:
  - Ring init `FUN_1405c78b0` (spans :962530-962598): a start-position override read from
    `GetPrivateProfileStringA("main","pos",…,"C:\\config.ini")` (:962552); a validity bitmap of
    `(count+7)/8` bytes memset `0xFF` (:962561-962564); **10 pre-allocated run buffers**
    (`sector_size × count` each — alloc :962569, push :962579, loop bound :962582), each filled
    by one device read; two manual-reset events (kick initially non-signaled :962589, ready
    initially signaled :962591); `CreateThread` for the prefetch thread (:962593).
  - Thread proc `FUN_1405c7ac0` (spans :962602-962683): polls the kick event with **1 ms**
    `WaitForSingleObject(kick, 1)` (:962620); on kick: advance cursor mod 10 (:962623-962628),
    clamp at end-of-device + vendor log `"----------CacheThread find read sector = %d"`
    (:962631-962635), read `count` sectors at the advancing position into the cursor's ring
    buffer through the device vtable read slot (:962660-962664), `EnterCriticalSection` (:962670),
    ring-fill counter++ (:962671), **`ResetEvent(kick)` when the 10-deep window is full**
    (:962672-962674), `SetEvent(ready)` (:962675), `LeaveCriticalSection` (:962676). Device-gone
    error 0x37 stops the thread (:962665-962667).
  - Reading: the queue model is **up to 10 outstanding run-reads ahead of the consumer, one
    synchronous device read in flight per cache thread, gated by events** [INFERRED from the
    event flow at :962620-962676; final-prod's register words it "10-run look-ahead window,
    1000-sector default runs" — N-045 in `cr_cached.h`].
- The corpus `.md` artifacts do not narrate this ring — the only artifact-level statement is the
  cache's role ("survive MFT re-reads economically", §1) [OBSERVED for the role; mechanics
  CODE xref]. Nothing documents per-transport queue-depth tuning [NOT-IN-CORPUS].

## 6. Buffering & caching (documented inventory)

- **`CStorageDeviceWithCache`** — the IO cache layer [OBSERVED class, `ntfs-engine-internals.md:24`]:
  - ctor `FUN_1405c72c0`: `InitializeCriticalSection` at +0x88 (:962269); block-count setter
    `FUN_1405c77d0(count)` allocates a `sector_size × count` cache buffer (:962487-962501) —
    **default count 1000** (:962281); **ring depth 10** (`param_1[0xe] = 10`, :962284); device
    type code from `FUN_1401f9110(inner)` (:962294) [CODE xref].
  - DRWUI deep-scan device setup `FUN_1405846b0` overrides the count with
    **`0x400000 / bytes_per_sector` (4 MiB), fallback `0x2000`** (:919086-919089, :919142),
    attaches a change-detection compare buffer (:919143), starts the ring (:919144), then runs
    the whole-device pass `(0, totalSectors-1)` through it (:919149-919155) [CODE xref; final-prod N-045 registers
    the same reads as "og budget 4 MiB … constructed once per physical disk, shared across
    partition scans" — the once-per-disk reading is the register's, not a corpus artifact
    statement].
  - Read path `FUN_1405c7650`: cache-hit `memcpy` from the cache buffer; miss falls through to
    the inner device read [CODE xref, :962419-962478].
  - Locking: one `CRITICAL_SECTION` guards ring bookkeeping (:962269 init; :962670 enter /
    :962676 leave in the thread) [CODE xref].
- **Other documented caches/layers**:
  - MFT-stream + record-cache helper objects created by `EUCreateNTFSSearcher` at
    `obj[0x1a]/obj[0x1b]` [OBSERVED, `SearchDLL/ntfs-engine--deep-dive.md` §1, lines 30-31].
  - `CNTFSBuildVolumeBitmap` — quick-scan used-space map [OBSERVED, `ntfs-engine-internals.md:59-64`];
    its backing file is the mapped, FSCTL_LOCK_VOLUME-locked auxiliary-bitmap file (§2).
  - exFAT `CQuickSearchAndBuildBitmap` shared base keeps the FS bitmap in memory "same
    auxiliary-bitmap concept as NTFS $Bitmap / the NAS agent's euAuxiliaryBitmaplog" [OBSERVED,
    `SearchDLL/exfat-engine--deep-dive.md:25-29`].
  - `CHFSPlusFileCache` [OBSERVED RTTI, `SearchDLL/hfsplus-engine--deep-dive.md:13`].
  - EXT4 **CatchBuffer device proxy**: wraps the volume device; "on read failure the proxy
    allocates a fresh 0xE00-byte buffer object and retries with 0x400 backtrack" — bad-sector
    resilience unique to that engine [OBSERVED, `SearchDLL/ext4-engine--deep-dive.md:25-29, :47`].
  - EUDCPEPM urgent COW cache (`CacheSrcData`/`ReadFromCache`/`CacheTransmitData`) — WinPE
    backup/copy chain, not the live scan path [OBSERVED,
    `PartitionRecovery/eudcpepm-euedkepm--hook-framework-urgent-cache.md` §1.3, lines 71-93].

## 7. Cancellation & stop (documented)

- `CStopScanThread` — "scan cancellation worker (cooperative stop of the engine chain)"
  [OBSERVED, `DRWUI-exe--thread-architecture.md:14`].
- Scan-abort event: `CScanMgr` ctor creates `CreateEventA(manual-reset, initially-signaled)`
  at **+0x108 = the scan-abort event** [OBSERVED, `re-workspace/artifacts/UIUX/13-ui-core-wiring-state.md:42, :63`].
- Per-record responsive cancel: the deleted-record batch loop checks
  `controller.cancel_callback.vt[0x10]()` **every record** and breaks [OBSERVED,
  `SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1, lines 16-18].
- Salvage-over-purity: "Fixup failure → record skipped (never aborts the whole walk — a recovery
  product maximizes salvage over purity)" [OBSERVED, `SearchDLL/ntfs-engine--deep-dive.md` §9,
  lines 159-161]. No explicit sector-read retry loop is documented below the cache [NOT-IN-CORPUS].
- Stop UI: confirm dialog "Stop searching for lost data?" with risk framing "Remaining data can
  be rescanned…" + `CStopScanThread` cooperative cancel + `CScanCommonPopDlg` confirm [OBSERVED,
  `UIUX/08-scan-phase-deep-dive.md:28, :55`]; zh-CN copy coaches "中止扫描会导致部分数据无法找到"
  [OBSERVED, `Languages/language-packs--family-census.md:24`]. Scan can also be **paused/resumed**
  (atlas scan-phase states incl. paused) [OBSERVED, `worklog.md` uiux-6 entry — 5 states
  incl. paused; `UIUX/35-redesign-reference-atlas.md`].
- Recovery export checks cancel between phases (`ExportThread` "PID/TID trace → cancel checks →
  destination validation") [OBSERVED, `architecture/04` §7.3, lines 136-140].
- Driver-level abort exists only in the WinPE copy chain: a CANCEL/ABORT state setter among
  EUDCPEPM's 15 private IOCTL commands [OBSERVED, `architecture/02-kernel-driver-architecture.md` §3, lines 67-77].
- Coordinated completion: the Freeze protocol ("`-------------Start Freeze--------------` …
  `m_nFreeze=0, bUnfreeze=1` … `Unfreeze`") wraps tree-build finish + auto-`.rsf` save at scan
  end [OBSERVED, `UIUX/30-interaction-driver-ground-truth.md:64-68, :105`; freeze signal table
  `UIUX/20-moc-surface-sink-architecture.md:51-53`].
- Cache thread teardown: the setup path clears the running flag (`+0x48 = 0`) after the
  whole-device pass (:919156), and the thread self-stops on device-gone (§5) [CODE xref,
  `DRWUI_decompiled.c:919156, :962665-962667`].

## 8. Per-transport / device-class behavior (documented)

- Home list rows carry per-partition **TRIM status chips "TRIM OFF"/"TRIM ON"** — SSD-aware
  recovery-chance indicator [OBSERVED, `UIUX/07-home-phase-deep-dive.md:17`]; at boot DRW shells
  out to `fsutil behavior` twice — candidate purpose `disabledeletenotify` (TRIM status) [OBSERVED
  behavior, reading flagged, `UIUX/dynamic-ground-truth--wine-loader-run.md:62`].
- Device enumeration via WMI (`Win32_DiskDrive` family; `wbem_locator_ConnectServer` ×2 rounds)
  and `RegisterPowerSettingNotification` power monitoring ("disk-scan safety on laptops; pairs
  with the USB-unplug guard") [OBSERVED, `dynamic-ground-truth--wine-loader-run.md:35, :37`].
- Removable/SD handling: `CameraDeviceManager: Insert Camera Device SD/MMC/MS PRO
  (size=62607327232) — type=12/13 notices; nMaxRemovebleDiskSize=549755813888 (512GB threshold)`
  [OBSERVED, `config-files/qa-logs--session-forensics.md:21-24`]; SD/camera flow via
  `CSDCardDeviceListPage` + peripheral device flow [OBSERVED, `architecture/04` §2, lines 48-49];
  drive-type icon codes 22-25 = physical-disk pages (`CDoScanLostPartition`) [OBSERVED,
  `UIUX/07-home-phase-deep-dive.md:46`].
- Transport-specialized paths: BitLocker (FVE keys via `\\.\Pipe\DRWBitlocker` DRWService +
  XTS-AES content decrypt) [OBSERVED, `architecture/04` §4a, lines 78-80]; NAS (EuMrx redirector
  + libssh2 agent pushed to the NAS at /tmp/EaseUS; the agent runs "a bitmap-based scanner +
  file-matching engine ON the NAS" with `euAuxiliaryBitmaplog`) [OBSERVED,
  `EuShare/eumrx-nas-architecture.md:125, :159`]; Linux RAID/Cloud/Linux-recovery share the
  triple-connector pattern [OBSERVED, `DRWUI-exe--thread-architecture.md` §2, lines 32-35].
- 4Kn/512e: normalization above the driver — `InitSectorSize` in EUDCPEPM "normalizes sector
  geometry between target device objects"; EPMNtDrv is 512-hardcoded [OBSERVED, eprdtdrv §3
  note line 73 + `architecture/02` §3 strings, line 88].
- SMART companion DiskHealth (CrystalDiskInfo lineage) does multi-transport access:
  `GetSmartAttributeSat -1A/-2/-2A` (SAT/ATA), `GetSmartAttributeCsmi` (CSMI/Intel RST),
  `GetSmartAttributeNVMeRealtek` (Realtek NVMe bridge), USB bridge tables, "Parallels Virtual
  NVMe Disk" virtual-disk awareness — companion telemetry, not the scan path [OBSERVED,
  `DiskHealth/diskhealth--smart-companion.md:12, :19, :23`].
- WinPE edition: drivers preloaded (ebrntdrv/epmdkdrv demand-start; EUDCPEPM BOOT_START as
  disk-class `UpperFilters` on every disk; EUEDKEPM SYSTEM_START) [OBSERVED,
  `architecture/03-winpe-boot-environment.md` §3, lines 45-54]; PE capability diff — no
  `PartitionRecovery/` .mo (no lost-partition search), no EuShare redirector, no licensing, PE =
  FREE by construction [OBSERVED, `architecture/03` §6, lines 67-75].
- No documented per-transport change to scan I/O sizes/threads (all sizes in §3 are
  transport-agnostic; only TRIM advice and enumeration differ) [NOT-IN-CORPUS].

## 9. Kernel-driver era vs direct user-mode I/O — what ships today

- The corpus describes a **mixed model, per volume condition**: "normal file API when volume is
  mounted & healthy; raw IOCTL path → EPMNtDrv for damaged/locked volumes" [OBSERVED,
  `architecture/05` §8 lines 116-125; `architecture/04` §4a line 76-77]. Four engines carry
  direct Win32 IO imports; four ride the abstraction only (§2 import census).
- Live Windows: `eprdtdrv.sys` is **installed on demand** — "setupdrvx64.exe
  (mod.WinDiskAccessDriverSetup) installs+starts eprdtdrv service on first scan needing raw
  access" [OBSERVED, eprdtdrv §7.1, lines 95-99; `architecture/02` §5, lines 99-106]. The
  driver is hot-unloadable (`DriverUnload` registered), enabling no-reboot numbered-variant
  upgrades (`eprdtdrv2/3…`) [OBSERVED, `epmntdrv-internals-v2` §4, lines 104-116].
- One driver source, four filenames: eprdtdrv (live DRW), ebrntdrv + epmdkdrv (WinPE),
  eprdtdrv_win7older (Win7 ABI) — same 39 functions [OBSERVED, eprdtdrv §8, lines 102-106].
- Hook/redirector drivers are NOT part of the live DRW scan path: "These are NOT loaded during
  normal Windows operation of DRW — they are for the bootable environment (and EPM's disk-copy
  product)" [OBSERVED, `architecture/02` §3, lines 89-90]; EUEDKEPM has "no in-tree opener —
  dormant deployment from the EPM/Todo-Backup disk-copy lineage" [OBSERVED, `architecture/02`
  §7 table, line 129].
- Anti-abuse: the driver family validates callers (PID/process checks; EUEDKEPM open-path
  PID-token gate) [OBSERVED, `architecture/02` §2.4, lines 53-55 + §7 line 129; identity IOCTLs
  0x60002400/04/08 in `epmntdrv-internals-v2` §2, lines 42-81].
- Product-side note (not ground truth): final-prod's kernel driver was removed and ABI advanced
  (ARCHIVED `worklog/context-sheets/DRIVER-DEPLOYMENT.md` header) — i.e. our product ships the
  **direct user-mode I/O era only**, with the og's driver-era behavior preserved in this sheet.

## 10. NOT-IN-CORPUS register (explicitly absent — do not invent)

1. **IOCP / async-OVERLAPPED / queued-completion** in the scan path — zero IOCP APIs in the
   decompiles; all OVERLAPPED args are NULL (§5) [NOT-IN-CORPUS].
2. **Per-read retry loop below the cache** — no doc; engine level is skip-and-continue [NOT-IN-CORPUS].
3. **Engine-fan-out thread counts for the advanced scan** (how many engines run concurrently) [UNKNOWN].
4. **Cache hit-rate / IOPS / throughput numbers for the og** — corpus has timings (quick ≈21 s
   on 131 GB NTFS; full session 14.7 min / 1,139,286 files / 147.12 GB on 149 GB ≈98.5%
   enumerated [OBSERVED, `UIUX/dynamic-ground-truth--scan-phase-captured.md:113, :138`]) but no
   queue/latency measurements [NOT-IN-CORPUS].
5. **Total prefetch-ring memory figure as a vendor-stated number** — ring is 10 × (sector_size ×
   count) buffers + the count-sized cache buffer by construction (:962569-962582, :962487-962501);
   the "4 MiB budget" phrasing is final-prod's register reading [CODE xref + INFERRED].
6. **Per-transport queue-depth or read-size tuning** [NOT-IN-CORPUS].

## 11. Verification ledger (selected)

| Claim | Evidence |
|---|---|
| 4-backend volume IO layering + cache in front | `architecture/05` §8; `ntfs-engine--deep-dive.md` §8 |
| 512-byte driver contract, sectors<<9, MDL async, sync-from-caller | eprdtdrv §3; internals-v2 §1 |
| 64 KB MFT bootstrap / 4 KiB pages / ≥0x400 records / cluster-strided | ntfs-engine-internals:33; algorithms-v2 §5; deep-dive §2-3, §8 |
| Prefetch thread + 10-ring + count default 1000 + CS lock + kick/ready events | `DRWUI_decompiled.c:962250-962683` (CODE xref) |
| 4 MiB / 0x2000 override at deep-scan device setup | `DRWUI_decompiled.c:919078-919144` (CODE xref) |
| 45-thread census, UI purity, pool, condition variables | `DRWUI-exe--thread-architecture.md`; ntfs-engine-internals:72-74 |
| Cancellation (abort event, per-record check, CStopScanThread, stop UI) | UIUX/13:42; algorithms-v2 §1; thread-architecture:14; UIUX/08:28,55 |
| Transport matrix (TRIM, WMI, NAS/BitLocker/Linux, PE diff, SMART) | UIUX/07:17; wine-loader-run:35-62; 04 §4a; EuShare artifact; 03 §3, §6; DiskHealth artifact |
| Mixed driver/direct model; on-demand install; WinPE-only hook drivers | 05 §8; eprdtdrv §7; 02 §3, §5 |
| Engine import split (4 direct-IO vs 4 abstraction-only) | `ci-multitool/searchdll/*/rabin2_imports.txt` (first-hand) |

## 12. Cross-links

- Sibling sheets: `worklog/REFERENCE-SCAN.md` (Agent C — raw parameter census, §7 overlaps),
  `worklog/REFERENCE-BEHAVIOR.md`, `worklog/REFERENCE-FOOTPRINT.md`,
  `worklog/context-sheets/DRIVER-DEPLOYMENT.md` (archived product driver decision).
- Product divergence registers citing this architecture:
  `final-prod/engine/include/cr/storage/cr_cached.h` (N-045/D-72), `final-prod/engine/docs/PERFORMANCE-BASELINE.md` §5.
- Corpus: `architecture/00-INDEX.md` (master index), `re-workspace/artifacts/CORRECTIONS.md`
  (errata supersede older artifact text), `worklog.md` (methodology, stages 0→5).
