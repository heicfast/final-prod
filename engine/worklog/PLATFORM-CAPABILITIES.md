# PLATFORM-CAPABILITIES — the Windows user-mode raw-I/O capability set, and what the direct path exploits of it

> Agent E (T2-e), Phase-0 platform survey, **2026-11-05**. Core at `e6ee97da`; driver repo `rda-driver`
> at `323e75a`; corpus `easeus-drw-research` (read-only).
>
> **Citation discipline**: every platform/OS claim carries a Microsoft Learn URL that was **fetched
> this session** (HTTP 200, text quoted or paraphrased from the fetched page); every code claim carries
> `path:line` against the pinned trees. Claims that are widely repeated but NOT in the docs are labeled
> **"undocumented behavior — treat as risky"**; project-measured facts are labeled **[project-measured]**
> with the run ID. No credentials, no corpus mutations; this file is the only write.

---

## §1 The user-mode raw-I/O capability set (the menu)

### 1.1 Open flags & sharing — `\\.\PhysicalDriveN` vs volume GUID / `\\.\X:`

**Documented** (CreateFileW, "Physical Disks and Volumes" section —
<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew>):

- A physical-disk or volume open returns a **DASD handle** usable with `DeviceIoControl` (and
  `ReadFile`/`WriteFile`). Requirements for success: **caller must have administrative privileges**;
  `dwCreationDisposition` must be **OPEN_EXISTING**; and **when opening a volume or floppy disk the
  `dwShareMode` must include FILE_SHARE_WRITE**.
- `dwDesiredAccess` **may be zero** — a metadata-only open for querying device attributes "without
  accessing a device" (the core's busy-disk fallback open, §4).
- **"Direct access to the disk or to a volume is restricted"** (XP+). The doc's requirement list is
  the restriction; the *write* half of the restriction class is documented under WriteFile (§5).
- Volume GUID form `\\?\Volume{GUID}\` is documented as the unique-volume name
  (<https://learn.microsoft.com/en-us/windows/win32/FileIO/naming-a-volume>).
- Sharing consequences: omitting FILE_SHARE_WRITE blocks concurrent writers (open fails with a
  sharing violation if anyone holds write access or a write mapping); FILE_SHARE_WRITE **allows**
  subsequent write opens — on volume handles it is *mandatory* anyway per the requirement above.
- Byte-offset semantics: volume↔disk mapping is documented through
  IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS / `DISK_EXTENT.StartingOffset` = "the offset from the
  beginning of the disk to the extent, in bytes"
  (<https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-disk_extent>,
  <https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-volume_disk_extents>).
  **Undocumented behavior — treat as risky**: no fetched page states outright that offset 0 on a
  volume handle is the VBR/partition-first-sector; it is *implied* by FSCTL_ALLOW_EXTENDED_DASD_IO's
  "hidden sectors ... between the first sector of the partition (the boot parameter block) and the
  first useful sector" (see 1.7). The core avoids the question by reading everything through
  `\\.\PhysicalDriveN` and computing partition extents itself (§4).

**Requirement**: admin token, OPEN_EXISTING, FILE_SHARE_WRITE for volumes. **Benefit**: raw
sector-level visibility (partition tables, deleted structures) with zero kernel component of ours.

### 1.2 Buffered vs unbuffered — FILE_FLAG_NO_BUFFERING (0x20000000), FILE_FLAG_WRITE_THROUGH

**Documented**:

- CreateFileW flag text: NO_BUFFERING = "no system caching for data reads and writes. This flag does
  not affect hard disk caching or memory mapped files. There are strict requirements ... see File
  Buffering." WRITE_THROUGH = write-through to disk, no cached-write delay (multi-sector writes "may
  not always be written to the disk right away" without it — WriteFile remarks,
  <https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile>).
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew>)
- **The alignment contract** (File Buffering —
  <https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering>): with NO_BUFFERING,
  (a) file **access sizes, including the optional file offset in OVERLAPPED**, must be an integer
  multiple of the **volume sector size**; (b) **buffer addresses "should be physical sector-aligned"**
  — "Depending on the disk, this requirement may not be enforced"; (c) align to the **physical**
  sector (the atomic unit), query it via **IOCTL_STORAGE_QUERY_PROPERTY →
  STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR.BytesPerPhysicalSector** ("Microsoft strongly recommends");
  (d) allocate buffers with **VirtualAlloc** (page-aligned ⇒ usually sector-aligned) or
  **`_aligned_malloc`**.
- **Volume handles**: CreateFileW — "Volume handles can be opened as noncached at the discretion of
  the particular file system ... **You should assume that all Microsoft file systems open volume
  handles as noncached**. The restrictions on noncached I/O for files also apply to volumes. ... if
  the noncached option is specified when opening a volume, **buffer alignment is enforced**
  regardless of the file system ... **It is recommended on all file systems that you open volume
  handles as noncached**, and follow the noncached I/O restrictions."
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew>)
- **512e vs 4Kn** (Advanced Format cookbook —
  <https://learn.microsoft.com/en-us/windows/win32/w8cookbook/advanced-format--4k--disk-compatibility-update>;
  support policy —
  <https://learn.microsoft.com/en-us/troubleshoot/windows-server/backup-and-storage/support-policy-4k-sector-hard-drives>):
  two sector sizes (logical = smallest write unit / LBA unit; physical = single-operation unit,
  "unit of atomic write"); "it is no longer safe to make any assumptions regarding the sector size";
  most APIs (IOCTL_DISK_GET_DRIVE_GEOMETRY) return the *logical* size; 4Kn is inbox-supported from
  Win8/Server 2012.

**Benefit / trade**: unbuffered removes the cache-manager copy and its read-ahead machinery
(direct DMA into your buffer), at the price of the full alignment contract (offset+length+buffer).
**[project-measured, not in docs]**: GAP-S6-13 — reads whose length is a 4 KiB multiple but not a
64 KiB multiple take a ~3.5× slow path (~830 vs ~2900 MB/s NVMe; run 40073093997;
include/cr/storage/cr_directalign.h:4-9). No Microsoft page documents a 64 KiB threshold; the
*existence* of an alignment-sensitive slow path is consistent with the cache-manager behavior
documented in the async-appears-sync KB (1.3), but the specific 64 KiB granularity is **undocumented
behavior — treat as risky** (we only have our own measurements).

### 1.3 Synchronous vs overlapped — FILE_FLAG_OVERLAPPED (0x40000000)

**Documented**:

- CreateFileW flag text: "**If this flag is specified, the file can be used for simultaneous read
  and write operations. If this flag is not specified, then I/O operations are serialized, even if
  the calls to the read and write functions specify an OVERLAPPED structure.**"
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew>)
- ReadFile, "Considerations for working with synchronous file handles": with a synchronous handle and
  a non-NULL OVERLAPPED, "the read operation starts at the offset that is specified in the OVERLAPPED
  structure and **ReadFile does not return until the operation is complete**. The system updates the
  OVERLAPPED offset and the file pointer before ReadFile returns." (i.e. the OVERLAPPED is a position
  carrier only; ERROR_IO_PENDING cannot occur.)
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile>)
- Asynchronous handles: OVERLAPPED must be **valid and unique** per outstanding operation; the
  buffer/OVERLAPPED must not be touched until completion; `GetOverlappedResult` (optionally
  `bWait=TRUE`) or the OVERLAPPED event yields the byte count
  (<https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult>).
- **IOCP binding**: CreateIoCompletionPort requires a handle "opened for overlapped I/O completion"
  (FILE_FLAG_OVERLAPPED); multiple handles may share one port; completions dequeued via
  GetQueuedCompletionStatus
  (<https://learn.microsoft.com/en-us/windows/win32/FileIO/createiocompletionport>,
  <https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus>).
- **Thread-pool file I/O**: StartThreadpoolIo "must [be] call[ed] before initiating each
  asynchronous I/O operation on the file handle bound to the I/O completion object. Failure to do so
  will cause the thread pool to ignore an I/O operation ... and will cause memory corruption";
  FILE_SKIP_COMPLETION_PORT_ON_SUCCESS interacts (immediate-success completions skip the callback →
  CancelThreadpoolIo).
  (<https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-startthreadpoolio>)
- ReadFileEx: completion-callback form, requires FILE_FLAG_OVERLAPPED handle
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfileex>).
- Folklore guard: "If a file or device is opened for asynchronous I/O, subsequent calls ... generally
  return immediately but can also behave synchronously with respect to blocked execution"
  (<https://learn.microsoft.com/en-us/windows/win32/FileIO/synchronous-and-asynchronous-i-o>; the
  KB <https://learn.microsoft.com/en-us/previous-versions/troubleshoot/windows/win32/asynchronous-disk-io-synchronous>
  documents the mechanism: cached reads that miss get handed to "a limited pool of worker threads
  (currently three ...)" — "if you issue numerous I/O operations for data that is not in the cache,
  the cache manager and memory manager become saturated and your requests are made synchronous").

**Benefit**: per-handle parallelism (QD>1), non-blocking waits, IOCP/TP completion, and the
prerequisite for ReadFileScatter/WriteFileGather.

### 1.4 Scatter/gather reads — ReadFileScatter / WriteFileGather

**Documented** (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfilescatter>,
<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefilegather>):

- **Preconditions**: handle created with GENERIC_READ (write for Gather) **plus FILE_FLAG_OVERLAPPED
  plus FILE_FLAG_NO_BUFFERING**; a segment array of page-sized, page-aligned buffers; total length a
  multiple of the sector size; a **required** OVERLAPPED (async-only — may return ERROR_IO_PENDING;
  completion via GetOverlappedResult / HasOverlappedIoCompleted / GetQueuedCompletionStatus).
- FILE_SEGMENT_ELEMENT is a PVOID64 union (32-bit sign-extension caveat documented).

**Benefit**: one large device read landing discontiguously in user memory — zero re-copy when the
consumer's destination layout is page-structured (e.g. decode pipelines, image reassembly).

### 1.5 Queue depth & parallelism

**Documented**:

- **Storport per-LU queues**: "all requests are queued in the port driver in per-logical-unit queues.
  Without extended SRB support, each logical unit can have a maximum of **255 outstanding requests**.
  Otherwise, the queue depth is only limited by available system resources or the capabilities of
  the adapter ... There are no predefined limits from Storport on the number of outstanding requests
  that an adapter can have" (e.g. 55 LUs × 255 = 14,025).
  (<https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/storport-queue-management>)
- **The Win32-side ceilings are documented as errors, not numbers**: "The ReadFile function may fail
  with ERROR_INVALID_USER_BUFFER or ERROR_NOT_ENOUGH_MEMORY whenever there are **too many outstanding
  asynchronous I/O requests**", and "may fail with ERROR_NOT_ENOUGH_QUOTA, which means the calling
  process's buffer could not be page-locked" (→ SetProcessWorkingSetSize)
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile>).
- Sequential-streaming guidance: no page prescribes a QD for sequential raw reads. The async-KB's
  worker-pool-of-3 note is the documented hazard for *cached* async fan-out (1.3). Anything more
  specific (e.g. "QD8 saturates NVMe") is **undocumented behavior — treat as risky**.

**[project-measured]** CONSUMERS.md:498-500 (alignbench run 40227375329, `\\.\C:`, runner VHDX):
64 KiB QD1 208.6 → QD2 233.1 → QD4 231.1 → QD8 235.5 MiB/s; 256 KiB–4 MiB QD-insensitive ~240
plateau. I.e. on this runner only the small-block leg still benefits from QD2, +12%.

### 1.6 Cancellation — CancelIoEx / CancelSynchronousIo; TerminateThread anti-pattern

**Documented**:

- **CancelIoEx** "cancels only outstanding I/O on the handle ... only I/O operations in the current
  process, regardless of which thread created the I/O operation. ... Most types of operations can be
  canceled immediately; other operations can continue toward completion before they are actually
  canceled ... does not wait for all canceled operations to complete." Completion is one of three
  statuses: completed normally / canceled (ERROR_OPERATION_ABORTED) / failed otherwise. With a
  completion port attached, a *synchronous* operation successfully canceled queues no packet;
  pending *asynchronous* cancels do queue one.
  (<https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex>)
- **CancelSynchronousIo**: "cancels a pending synchronous I/O operation issued by the specified
  thread" — the documented tool for interrupting a blocking ReadFile on a **synchronous** handle
  (requires THREAD_TERMINATE access to the thread; ERROR_NOT_FOUND if nothing to cancel)
  (<https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelsynchronousio>).
- **TerminateThread** is the documented anti-pattern: "a dangerous function ... If the target thread
  owns a critical section, the critical section will not be released" (plus DLL/state corruption
  list) (<https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminatethread>).

**Benefit for cancellation-bound UI scans**: an OVERLAPPED handle + CancelIoEx gives cross-thread,
prompt, race-free abort of in-flight raw reads (the exact contract the driver implemented in kernel —
§2). Nothing volume/disk-specific is documented: the guarantees are handle-generic.

### 1.7 Geometry / identity IOCTLs, and FSCTL_ALLOW_EXTENDED_DASD_IO

**Documented**:

- **IOCTL_DISK_GET_DRIVE_GEOMETRY_EX**: geometry for MBR *and* GPT media (unlike the old
  IOCTL_DISK_GET_DRIVE_GEOMETRY which reads MBR only); "Only callers above Partmgr.sys may call this
  IOCTL"; built on GET_DRIVE_GEOMETRY + GET_LENGTH_INFO.
  (<https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ni-ntdddisk-ioctl_disk_get_drive_geometry_ex>)
- **IOCTL_STORAGE_GET_DEVICE_NUMBER**: FILE_DEVICE_XXX type + device number + partition number
  assigned when the device started.
  (<https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_get_device_number>)
- **IOCTL_STORAGE_QUERY_PROPERTY**: device/adapter properties incl. the access-alignment descriptor
  (physical sector size — the File Buffering page's recommended query).
  (<https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_query_property>)
- **FSCTL_ALLOW_EXTENDED_DASD_IO**: "Signals the file system driver not to perform any I/O boundary
  checks on partition read or write calls. Instead, boundary checks are performed by the device
  driver." **What it actually changes on volume handles**: access to "hidden sectors ... between the
  first sector of the partition (the boot parameter block) and the first useful sector", and "lost
  clusters ... between the last useful cluster and the end of the partition"; requests beyond the
  partition boundary still fail at the device driver. CreateFile must use FILE_SHARE_READ |
  FILE_SHARE_WRITE on the volume. "Should only be used with great caution" (write-side hazard).
  (<https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_allow_extended_dasd_io>)

**Benefit**: without it, the documented boundary checks clamp volume-handle I/O inside the FS's
notion of the partition — the classic "last few sectors unreadable through `\\.\X:`" failure
(CreateFileW's own note: "To read or write to the last few sectors of the volume, you must ...
specify FSCTL_ALLOW_EXTENDED_DASD_IO").

### 1.8 Memory — VirtualLock, large pages, PrefetchVirtualMemory, no-touch allocation

**Documented**:

- **VirtualLock**: locks pages into physical memory; **the lockable maximum is the minimum working
  set minus a small overhead** — "Applications that need to lock larger numbers of pages must first
  call SetProcessWorkingSetSize"; locked pages "are guaranteed not to be written to the pagefile
  while ... locked"; locking may degrade overall system performance.
  (<https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtuallock>)
- **PrefetchVirtualMemory** (Win8+): "efficiently bring in those address ranges from disk using
  large, concurrent I/O requests where possible"; "purely a performance optimization ... treated as
  a strong hint ... can completely or partially fail under low-memory conditions"; prefetched memory
  is physical-cached, **not** added to the working set.
  (<https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-prefetchvirtualmemory>)
- **Allocate-no-touch / commit charge**: VirtualAlloc MEM_COMMIT "allocates memory charges (from the
  overall size of memory and the paging files on disk) ... **Actual physical pages are not allocated
  unless/until the virtual addresses are actually accessed**"; MEM_RESERVE reserves VA without
  physical/paging storage; MEM_RESET discards interest without decommitting; MEM_LARGE_PAGES exists
  (0x20000000) but its privilege story (SeLockMemoryPrivilege) is **not** on the fetched page —
  verify before use.
  (<https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc>)
- Interaction with I/O: ReadFile's ERROR_NOT_ENOUGH_QUOTA page-lock failure (1.5) is the flip side —
  the OS charges locked-page quota for direct-IO buffers.

### 1.9 Hints — FILE_FLAG_SEQUENTIAL_SCAN on a volume/DASD handle

**Documented**:

- For **files**: "Access is intended to be sequential ... The system can use this as a hint to
  optimize file caching"; gains for "applications that read large files using sequential access";
  mixing RANDOM_ACCESS + SEQUENTIAL_SCAN is "self-defeating"
  (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew>). The
  async-KB adds: the cache manager "will optimize the cache for this type of access"
  (<https://learn.microsoft.com/en-us/previous-versions/troubleshoot/windows/win32/asynchronous-disk-io-synchronous>).
- For **volume handles**: the hint steers the *cache manager*, but "You should assume all Microsoft
  file systems open volume handles as noncached" (1.2) — there is no cache to steer on the volume
  data path, and the docs make **no** promise of any effect on DASD handles. Any claimed effect of
  SEQUENTIAL_SCAN on `\\.\PhysicalDriveN` is **undocumented behavior — treat as risky**.

---

## §2 What the DRIVER exploited (rda-driver @ 323e75ac)

- **Device-object stack position**: a named control device (`IoCreateDeviceSecure`, FILE_DEVICE_UNKNOWN,
  admin-gated SDDL — src/driver/rda_control.c:152-153) **plus**, in filter mode, a transparent filter
  DO attached into every disk stack (`IoAttachDeviceToDeviceStack`, matching the stack's
  DO_BUFFERED_IO|DO_DIRECT_IO convention — src/driver/rda_filter.c:397-421). Targets resolved
  *inside* the kernel by name: `\Device\HarddiskVolume%u` → **top-of-stack** via
  IoGetAttachedDeviceReference (src/driver/rda_target.c:121, :143-149), disk →
  `\Device\Harddisk{d}\Partition0` with the walk-down-to-disk.sys discipline (src/driver/rda_target.c:7-27).
  I/O issued at the *top of the storage stack* — above volume-manager filters for volumes, at the
  disk class layer for disks.
- **Direct I/O + MDL, no buffering**: control device `Flags |= DO_DIRECT_IO` (src/driver/rda_control.c:159-160);
  the caller's buffer mapped once with `MmGetSystemAddressForMdlSafe` (src/driver/rda_readwrite.c:293-305)
  and forwarded verbatim via `IoBuildAsynchronousFsdRequest` → `IoCallDriver`
  (src/driver/rda_readwrite.c:224-236). Zero copy, zero cache-manager involvement, no alignment
  contract at the Win32 edge (byte lengths/offsets — docs/DRIVER-CONTRACT.md §5, quoted in
  worklog/ACCESS-LAYER.md:386-388). The internal session path even forced `IRP_PAGING_IO`
  semantics for file-object IO (src/driver/rda_session.c:1180-1181).
- **Synchronous forward**: dispatcher waits on the completion event (`KeWaitForSingleObject`,
  src/driver/rda_readwrite.c:242-257) — the QD1 model, inherited by the core (CONSUMERS.md:465-468).
  The per-IO cost was tuned with an SLIST context pool after a measured ~10% IOPS gap
  (src/driver/rda_readwrite.c:49-64).
- **Cancellation**: a full cancellable forward-wait design — cancel routine never completes the IRP,
  flags + wakes the dispatcher, which propagates `IoCancelIrp` into the storage stack on the
  driver-owned forward IRP; single-completer invariant (src/driver/rda_readwrite.c:23-42, :105-121,
  :250-263).
- **The F11 Information oddity (driver-era contract)**: on success the driver reported
  `Information = Length × 512` — the legacy sector convention reference clients divide by
  (src/driver/rda_readwrite.c:266-272; `RDA_LEGACY_INFORMATION_SCALE 512UL`,
  src/shared/inc/rda_ioctl.h:146). **Bought**: wire compatibility with the og client grammar.
  **Cost**: a fake byte count on every read (4Kn drives must normalize above the driver), and a
  client-side unscale. Deleted with the driver (src/win/cr_transport.cpp:8-10, :48-52).
- **What the posture cost overall**: driver install/signing/SCM lifecycle, Win7 ABI hazards
  (StartService 127 — rda-driver docs/benchmarks/2026-10-22-runner-comparison.md:58-64), and the
  measured ~3-10% parity tax (fuzz +52%, rand −2.9%, open +2.2% vs the og driver — same doc, §Round-1/2).

## §3 What the og REFERENCE exploited (corpus)

- **The WinPE-gated direct leg**: every MOM device-open forks on `IsWinPE()` — live Windows →
  `\\.\EPRDTDRV\{token}\...` (driver), WinPE → `\\.\PhysicalDrive%d` / `\\.\HarddiskVolume%d`
  (direct, no driver at all). First-hand: `re-workspace/reversed/ghidra-mo/Device_decompiled.c:7816-7823`
  (the gate + PhysicalDrive sprintf), probe at `:50943-50992`. The og's **own PE edition is the
  product-mode precedent for a driverless direct core** (worklog/REFERENCE-ACCESS.md:433-437).
- **Buffered + synchronous**: the MOM data path opens with `CreateFileA(path, access, share, 0, 3,
  0x80 /*FILE_ATTRIBUTE_NORMAL*/, 0)` — **no OVERLAPPED, no NO_BUFFERING** — verified first-hand at
  `Device_decompiled.c:56456`; the read loop is `SetFilePointer` + synchronous `ReadFile`
  (CVolumeDevice::ReadSectors, `Device_decompiled.c:56961-57038`, loop :57018-57028). The **only**
  `FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING` handle in the corpus is DRWUI's disk-info handle
  (`0x60000080` at `DRWUI_decompiled.c:176880`, verified first-hand) and it is **IOCTL-only** — the
  og never does unbuffered bulk reads (REFERENCE-ACCESS.md:149-159, :425-427).
- **The chunking constants**: disk-level 4 MiB per ReadFile (`0x400000/sectorSize`,
  `Device_decompiled.c:56545`, verified) and volume-level 7 MiB (`0x700000/sectorSize`,
  `:56912`, verified); engine-side 2 MiB MFT batches (NTFSSearchDLL_decompiled.c:5649-5678, cited
  src/win/cr_transport.cpp:174) and a cluster-aligned engine cache over the host device
  (REFERENCE-ACCESS.md:406-413).
- **The F11 ×512 scaling oddity (og side)**: the og driver reports `IoStatus.Information = Length
  << 9` with **512 hardcoded** — "user-mode must adapt to 4Kn/512e drives" above the driver
  (`re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:68, :71`;
  REFERENCE-ACCESS.md:299-301).
- **No parallel raw-read engine**: concurrency is engine/UI-level; sector reads serialize through
  the single per-volume device object (REFERENCE-ACCESS.md:416-424) — the og is QD1 by design too.
- The 7-driver family map: `architecture/02-kernel-driver-architecture.md:7-17` (epmntdrv×4 names,
  EUDCPEPM hook, EUEDKEPM door, EuMrx redirector).

## §4 What the CURRENT direct path exploits (HEAD e6ee97da) — and the GAP LIST

### 4.1 What it uses

- **Opens**: data path `CreateFileW(\\.\PhysicalDriveN, GENERIC_READ, FILE_SHARE_READ|
  FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)`
  (src/win/cr_transport.cpp:291-293, `open_path`; public entry `open_disk` :359-371; control view
  `open_physical_drive` :327-331). RW legs: GENERIC_READ|GENERIC_WRITE share R|W|D (:732-734);
  metadata-only 0-access fallback opens for busy disks (src/win/cr_enumerate.cpp:98-113, and
  :803-804). Quiesce/restore volume opens GENERIC_READ|GENERIC_WRITE share R|W (:428-430, :686-688).
  Image files: GENERIC_READ, share R|W|D, FILE_ATTRIBUTE_NORMAL (src/win/cr_image_win.cpp:37-39).
- **Completion model**: one stack `OVERLAPPED` per call as a **position carrier on a synchronous
  handle** — ReadFile blocks, GetOverlappedResult(bWait=TRUE) closes the dance
  (src/win/cr_transport.cpp:53-80). The ERROR_IO_PENDING branch (:60-69) is dead code by
  documentation: on a handle without FILE_FLAG_OVERLAPPED "I/O operations are serialized, even if
  the calls ... specify an OVERLAPPED structure" (L-CREATEFILE) and ReadFile "does not return until
  the operation is complete" (L-READFILE). Writes identical, plus sector-aligned staging via
  `_aligned_malloc` when the caller buffer is unaligned (:622-659, staging :627-633).
- **The 64 KiB envelope**: offset-down/length-up to 64 KiB multiples with device-tail clamp, copy
  out of a `thread_local std::vector` bounce only when unaligned, no copy on the aligned fast path
  (src/win/cr_transport.cpp:191-215; pure math include/cr/storage/cr_directalign.h:33-73). Applied
  to both DirectDevice (:264-277) and RwDevice (:490-507) reads.
- **Geometry/identity**: IOCTL_DISK_GET_LENGTH_INFO (:84-92), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
  (:100-103) + IOCTL_STORAGE_QUERY_PROPERTY StorageAccessAlignmentProperty for 512e/4Kn
  (:105-130; enumeration twin cr_enumerate.cpp:28-53), StorageDeviceProperty/DeviceIdProperty
  (cr_enumerate.cpp:54-83), IOCTL_STORAGE_GET_DEVICE_NUMBER (cr_enumerate.cpp:189),
  IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS (:344, cr_enumerate.cpp:234, :809), N-206 geometry-EX ceil
  (:136-137).
- **Health/control passthroughs** (src/win/cr_health.cpp): IOCTL_ATA_PASS_THROUGH (:59), legacy
  SMART_RCV_DRIVE_DATA (:105), IOCTL_SCSI_GET_ADDRESS + `\\.\Scsi%u:` CSMI miniport (:164-170),
  IOCTL_STORAGE_QUERY_PROPERTY protocol-specific NVMe health log (:290), seek-penalty SSD probe
  (:337). Quiesce chain: FSCTL_LOCK_VOLUME / FSCTL_DISMOUNT_VOLUME / IOCTL_VOLUME_OFFLINE /
  IOCTL_VOLUME_ONLINE + mount point surgery (cr_transport.cpp:417-482, :678-703). Apply chain:
  IOCTL_DISK_DELETE_DRIVE_LAYOUT / SET_DRIVE_LAYOUT / 0x7C058 / UPDATE_PROPERTIES
  (:536-592, :763-788).
- **Cache**: engine reads go through `slice → SliceShim → RetryingDevice(Skip×3) → CachedReader`
  at 8 MiB / 256 KiB blocks (src/core/orchestrator/cr_scan.cpp:164-203, constants :1298; the LRU
  itself under one mutex — src/core/storage/cr_cached.cpp:28-97).
- **Healing**: one in-place reopen on ERROR_INVALID_HANDLE (cr_transport.cpp:237-249, :271-275).

### 4.2 The GAP LIST — §1 capabilities the current path does NOT exploit

Grep-verified over src/ at HEAD: **zero occurrences** of FILE_FLAG_OVERLAPPED,
FILE_FLAG_NO_BUFFERING, FILE_FLAG_SEQUENTIAL_SCAN, FILE_FLAG_WRITE_THROUGH, ReadFileScatter,
WriteFileGather, CreateIoCompletionPort/GetQueuedCompletionStatus/StartThreadpoolIo, CancelIoEx,
CancelSynchronousIo, VirtualLock, PrefetchVirtualMemory, SetFilePointer, SetFileValidData,
SetFileInformationByHandle, FSCTL_ALLOW_EXTENDED_DASD_IO, TerminateThread. The only `_aligned_malloc`
uses are write staging (cr_transport.cpp:628) and the bench/probe helpers (rrecov.cpp:3025, :3071).

| # | Gap | Why it might matter (consumer pattern) |
|---|-----|----------------------------------------|
| G1 | **No FILE_FLAG_OVERLAPPED anywhere** — every read is serialized QD1 per handle (documented serialization; the carve/candidate worker fan-out collapses onto one in-flight read) | seq-bulk and deep scans on fast media leave device queue slots idle; [project-measured] 64 KiB QD1→QD2 = 208.6→233.1 MiB/s on the runner (CONSUMERS.md:498-500) |
| G2 | **No FILE_FLAG_NO_BUFFERING** — data-path reads traverse the cached path; the 64 KiB envelope is our private mitigation of its alignment sensitivity, not the documented bypass | removes the cache-manager copy + the GAP-S6-13 slow-path *class*; also the documented posture MS *recommends* for volume handles; costs the full offset+length+buffer alignment contract, and the CachedReader's heap `std::vector` blocks would need aligned allocation to stay copy-free |
| G3 | **No cancellation primitive** — a UI Stop cannot interrupt an in-flight 4 MiB read; the driver had a full cancellable forward design (rda_readwrite.c:23-42), the direct path has none | cancellation-bound UI scans on dying/USB media; documented tools: CancelIoEx (needs OVERLAPPED handle) / CancelSynchronousIo (works today, per-thread) |
| G4 | **No IOCP / thread-pool I/O** — completion is thread-blocking | duplex imaging (read source + write destination) and multi-target scans could share one completion queue with bounded threads |
| G5 | **No ReadFileScatter/WriteFileGather** | duplex imaging / decode pipelines could land device reads discontiguously, zero re-copy (preconditions: G1+G2 first) |
| G6 | **No FSCTL_ALLOW_EXTENDED_DASD_IO** on any volume-handle leg | tail-sector recovery on `\\.\X:`-style legs: the documented boundary checks clamp the last few sectors (we currently avoid volume data reads, so this only bites if a volume-handle data path is ever added) |
| G7 | **No memory-side strategy**: buffers are plain heap (`std::vector` in CachedReader/Bounce; no VirtualAlloc/VirtualLock/PrefetchVirtualMemory) | unbuffered G2 needs aligned+locked-adjacent buffers; working-set/quota sizing avoids ReadFile's documented ERROR_NOT_ENOUGH_QUOTA page-lock failure; PrefetchVirtualMemory could pre-warm the image-file source side |
| G8 | **No SEQUENTIAL_SCAN / WRITE_THROUGH hints**; no SetFileValidData/SetFileInformationByHandle on the destination write path | SEQUENTIAL_SCAN is documented for files (image destination could gain cache behavior); WRITE_THROUGH/SetFileValidData govern crash-consistency and zero-fill cost of image-file destinations (SE_MANAGE_VOLUME_NAME needed) |
| G9 | **0x40000000+ chunk guards only in the image layer** (cr_image_win.cpp:73): the raw path has no documented large-I/O split | none today (envelope caps at 1 MiB + 64 KiB — cr_transport.cpp:202-203); matters only if a consumer issues >2 GiB reads |

## §5 The constraint boundary — what user mode CANNOT do that the driver could

1. **The mounted-volume write refusal class (documented, hard)**: "A write on a disk handle will
   succeed if ... the sectors do not fall within a volume's extents; ... you have explicitly locked
   or dismounted the volume (FSCTL_LOCK_VOLUME / FSCTL_DISMOUNT_VOLUME); ... no mounted file system
   other than RAW." Same for volume handles (plus boot-sector / outside-FS-space exceptions).
   (<https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile>)
   No user-mode flag lifts this — the S-010 lock/dismount/OFFLINE chain (cr_transport.cpp:417-482)
   IS the documented route. FSCTL_LOCK_VOLUME fails on system volumes/page files/open files
   (<https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume>);
   dismount-without-lock harms bystanders (<https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_dismount_volume>).
   The kernel *could* (and the og's PE hook driver did — EUDCPEPM suppresses foreign IO,
   architecture/02-kernel-driver-architecture.md:85-89); user mode cannot.
2. **Admin privilege + share-mode are hard requirements** on DASD opens, and volume-handle alignment
   is *enforced* once NO_BUFFERING is chosen (L-CREATEFILE, 1.1/1.2) — unbuffered is not free-form;
   the alignment contract travels with it.
3. **Queue ceilings**: Storport per-LU 255 without extended SRB; Win32 fan-out fails with
   ERROR_INVALID_USER_BUFFER / ERROR_NOT_ENOUGH_MEMORY beyond "too many" outstanding asyncs, and
   page-locked buffers can fail quota (ERROR_NOT_ENOUGH_QUOTA) — plus the documented worker-pool
   degradation for *cached* async fan-out (1.3/1.5). The kernel driver could pump its own queue with
   IRQL-level control (the corpus EUDCPEPM data pump; our driver's session engine
   rda_session.c:376-419) — user mode only shapes demand, never the device queue.
4. **Kernel priority/IRQL and fairness**: no user-mode equivalent of DISPATCH_LEVEL completion,
   priority boosts, or filter-stack positioning (top-of-stack attach — rda_filter.c:410). A user-mode
   reader cannot preempt or reorder another process's IO on the same device; mandatory fairness does
   not exist at the Win32 layer. **[undocumented]** anything claiming user-mode QoS/priority control
   over device queues — treat as risky.
5. **Oplocks/volume-manager coordination**: locking is cooperative (FSCTL_LOCK_VOLUME semantics
   above); bypassing the volume manager's own synchronization (snapshot/oplock interplay) is not a
   documented user-mode capability. The og solved this in PE with hook drivers, not Win32.

## §6 The recommendation matrix

Capability × consumer pattern — **exploit / ignore / harmful**, one line each.
Consumers: **seq-bulk** (deep scan stream, imaging read side) · **rand-meta** (MFT/FAT/small random
metadata walk) · **latency** (interactive preview/TTFR) · **cancel-UI** (user-stopped scans) ·
**duplex** (image source read + destination write).

| Capability | seq-bulk | rand-meta | latency | cancel-UI | duplex |
|---|---|---|---|---|---|
| FILE_FLAG_OVERLAPPED + QD2-4 (G1) | **exploit** — [measured] +12% at 64 KiB, flat ≥256 KiB; must rework per-handle single-flight ABI (documented serialization is the current safety net) | **exploit** — pipelining hides seek latency on slow media | exploit (small) | **exploit** — prerequisite for CancelIoEx | exploit |
| FILE_FLAG_NO_BUFFERING + VirtualAlloc 64 KiB buffers (G2+G7) | **exploit** — kills the GAP-S6-13 class by construction; needs aligned CachedReader blocks or every read bounces | ignore — 256 KiB cache blocks stay, alignment at block level | ignore | ignore (neutral) | exploit — destination-side WRITE_THROUGH pairing |
| IOCP / thread-pool I/O (G4) | ignore at QD≤4 (events suffice) | ignore | ignore | ignore | **exploit** — one queue both directions |
| ReadFileScatter/Gather (G5) | ignore (contiguous stream) | ignore | ignore | ignore | exploit late — only after G1+G2, decode-shaped consumers |
| CancelIoEx / CancelSynchronousIo (G3) | ignore | ignore | ignore | **exploit** — CancelSynchronousIo works on today's handles; CancelIoEx after G1 | exploit (abort legs) |
| FSCTL_ALLOW_EXTENDED_DASD_IO (G6) | ignore (PhysicalDrive path unaffected) | ignore | ignore | ignore | ignore — only if a volume-handle data leg appears |
| Geometry/identity IOCTLs | already exploited (:84-150, cr_enumerate.cpp:28-85) | ditto | ditto | ditto | ditto |
| VirtualLock / working-set sizing (G7) | exploit with G2 (lock the bounce/cache blocks; size WS first) | ignore | ignore | ignore | exploit — destination staging buffers |
| PrefetchVirtualMemory (G7) | ignore on raw device legs (not a file mapping) | ignore | ignore | ignore | exploit — image-file source side (it IS a mapping-shaped read) |
| SEQUENTIAL_SCAN hint (G8) | ignore on DASD (undocumented effect; volume handles noncached) | ignore | ignore | ignore | **exploit on the image-file destination handle** (documented for files) |
| WRITE_THROUGH / SetFileValidData (G8) | ignore (read side) | ignore | ignore | ignore | **exploit** — valid-data extension avoids zero-fill on pre-sized images; SE_MANAGE_VOLUME_NAME (admin present) |
| QD>8 fan-out (1.5) | harmful — 255/LU ceiling is far away but quota/pool errors begin; measured plateau | harmful — IOPS do not help metadata walks below cache block size | neutral | harmful — more to cancel | harmful unless IOCP-shaped |

**Sequencing note**: G1 and G2 interact — OVERLAPPED-first (small, measured, QD2) is the cheap
experiment; NO_BUFFERING changes buffer ownership across the CachedReader boundary and must be a
deliberate contract change. Every switch must be re-measured with the §0 bench verbs
(worklog/PERFORMANCE.md:18-27) — the runner's VHDX already showed the direct-vs-driver gap is
device-profile-sensitive (worklog/PERFORMANCE.md:94-99).

---

### Doc-claim verification appendix (§8 discipline)

All URLs above returned HTTP 200 on 2026-11-05 and were text-extracted for quoting. Not verified /
not documented — flagged inline and repeated here:

1. The **64 KiB alignment fast-path threshold** (GAP-S6-13, ~3.5×) is project-measured (runs
   40073093997, 40227375329); no Microsoft page documents it. The docs' *only* stated alignment
   contract is sector-multiple sizes/offsets + physical-sector-aligned buffers (L-BUFFERING).
2. Whether `\\.\PhysicalDriveN` reads traverse the cache manager (the mechanism our envelope
   mitigates) is **not documented either way**; the documented noncached-by-default statement covers
   **volume** handles only (L-CREATEFILE).
3. "Offset 0 on a volume handle = VBR/partition first sector" — implied by L-DASD + L-DISKEXTENT,
   never stated outright.
4. Any specific per-handle outstanding-I/O number for Win32 (beyond the Storport 255/LU device-side
   figure and the ReadFile error codes) — undocumented.
5. FILE_FLAG_SEQUENTIAL_SCAN effect on DASD handles — undocumented (files only).
6. Large-page (MEM_LARGE_PAGES) privilege prerequisites — not on the fetched VirtualAlloc page;
   verify SeLockMemoryPrivilege docs before any use.

*Corpus and core untouched except this file. End of sheet.*
