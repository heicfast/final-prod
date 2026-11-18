# PLATFORM-PERF — Windows platform I/O facts for the recovery engine (Agent D retry, Task 2-D-retry)

Agent D retry, performance-diagnostic session, final-prod · Research date 2026-11-18 (06:39 UTC). Mission: the
Windows-side facts that decide the I/O architecture for a user-mode raw-disk recovery engine (`\\.\PhysicalDriveN` /
`\\.\C:` handles, sequential scan + random MFT hits + CPU-bound parsing). Supersedes the 2-D sheet previously at this
path. Every URL below was fetched directly by this agent (plain GET, HTTP 200) and every quote verified against the
fetched copy — nothing is quoted from memory. Numbers are hardware-specific unless Microsoft states a rule;
unresolved items are **OPEN**. Feeds DIAGNOSIS.md and the Phase-2 I/O-model decision.

Method / fetch audit: 16 GETs = 15 distinct pages (11 learn.microsoft.com + 1 docs.github.com + 2 Wikipedia
+ 1 GitHub issue of engineering-measurement class, labeled as such) — 3 over the "~12 max" guidance, deliberately:
question 4 spans five device classes, question 3 needs two API pages; zero search engines used; every fetch maps to
a numbered sub-question. Pages the earlier 2-D sheet cited but **not** re-fetched (contents not re-asserted here):
KB 156932, the I/O Completion Ports overview, File Caching, CreateThreadpoolIo/StartThreadpoolIo function pages,
colibri #863, runner-images issues — see OPEN register + source index.

---

## 1. FILE_FLAG_OVERLAPPED semantics on volume/disk handles; concurrent reads; GetOverlappedResult; CancelIoEx

- What OVERLAPPED buys, verbatim: "If this flag is specified, the file can be used for simultaneous read and write
  operations. If this flag is not specified, then I/O operations are serialized, even if the calls to the read and
  write functions specify an OVERLAPPED structure." Concurrency on one handle exists **only** on OVERLAPPED handles.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew]
- Not a hard async guarantee: overlapped calls "generally return immediately but can also behave synchronously with
  respect to blocked execution"; both fetched API pages link the KB titled *Asynchronous disk I/O appears as
  synchronous on Windows* (KB not fetched this retry → its recommendations, incl. any NO_BUFFERING-guarantees-async
  claim, OPEN). [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]
- Offset handling: "The system does not maintain the file pointer on asynchronous handles … the file position must be
  passed … in the related offset data members of the OVERLAPPED structure"; the event is set/reset automatically but
  "the offset … is not automatically updated" (ReadFile).
  [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]
- Concurrent ReadFile from multiple threads on one handle: each call "must point to a valid and unique OVERLAPPED
  structure, otherwise the function can incorrectly report that the read operation is complete"; "neither the offset
  nor any other part of the structure should be modified, freed, or reused … until the event is signaled";
  buffers/OVERLAPPEDs untouched "until all asynchronous I/O operations to the file object have been completed".
  ReadFile's hFile may be "a file, file stream, physical disk, volume, …" handle.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile] ·
  [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o]
- OS-side ceiling on outstanding async I/O: "The ReadFile function may fail with ERROR_INVALID_USER_BUFFER or
  ERROR_NOT_ENOUGH_MEMORY whenever there are too many outstanding asynchronous I/O requests" — the documented
  user-mode QD cap; no numeric bound given.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]
- hEvent == NULL footgun: "the system uses the state of the hFile handle to signal when the operation has been
  completed. Use of file … handles for this purpose is discouraged … there is no way to know which operation caused
  the object's state to be signaled" — with >1 outstanding read, completions are unidentifiable; wait on per-operation
  events (or use IOCP). [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult]
- GetOverlappedResult blocking equivalence: "If [bWait] is TRUE, and the Internal member of the lpOverlapped structure
  is STATUS_PENDING, the function does not return until the operation has been completed" — a blocking wait with the
  latency profile of a synchronous read; bWait=FALSE returns FALSE/ERROR_IO_INCOMPLETE while pending. Do not build a
  parallel pipeline on GetOverlappedResult(bWait=TRUE) polling.
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult]
- CancelIoEx scope: "The function only cancels I/O operations in the current process, regardless of which thread
  created the I/O operation"; lpOverlapped NULL ⇒ "all I/O requests for the hFile parameter are canceled", non-NULL ⇒
  "only those specific I/O requests … with the specified lpOverlapped" (CancelIo only cancels the calling thread's
  requests). [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex] ·
  [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o]
- CancelIoEx latency/semantics: "Most types of operations can be canceled immediately; other operations can continue
  toward completion before they are actually canceled and the caller is notified. The CancelIoEx function does not
  wait for all canceled operations to complete"; "you cannot know whether the operation was completed successfully or
  canceled" (handle state not changed). No documented numeric worst-case → **OPEN**. Device-side hint (HDD): NCQ can
  delay a queued I/O "up to 1–2 seconds" under I/O pressure (Google tests, 2008, via Wikipedia).
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex] ·
  [https://en.wikipedia.org/wiki/Native_Command_Queuing]
- Cancel completion statuses (handle all three): completed normally (cancel arrived too late),
  ERROR_OPERATION_ABORTED, or another error. "If the file handle is associated with a completion port, an I/O
  completion packet is not queued to the port if a synchronous operation is successfully canceled. For asynchronous
  operations still pending, the cancel operation will queue an I/O completion packet."
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex]
- OVERLAPPED lifetime after cancel: "The application must not free or reuse the OVERLAPPED structure associated with
  the canceled I/O operations until they have completed. The thread can use the GetOverlappedResult function to
  determine when the I/O operations themselves have been completed." Nothing to cancel ⇒ 0 + ERROR_NOT_FOUND (benign).
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex]

## 2. IOCP model: CreateIoCompletionPort + GetQueuedCompletionStatus; completion-thread count; Vista+ threadpool I/O

- Association rules: a handle "has to have been opened for overlapped I/O completion … you must specify the
  FILE_FLAG_OVERLAPPED flag when using the CreateFile function"; "A handle can be associated with only one I/O
  completion port, and after the association is made, the handle remains associated with that I/O completion port
  until it is closed"; "Multiple file handles can be associated with a single I/O completion port"; the port "is
  associated with the process that created it and is not sharable between processes. However, a single handle is
  sharable between threads in the same process". Min supported client: Windows XP.
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-createiocompletionport]
- IOCP excludes the Ex APIs and dislikes duplication: "After an instance of an open handle is associated with an I/O
  completion port, it cannot be used in the ReadFileEx or WriteFileEx function"; "It is best not to share a file
  handle associated with an I/O completion port by using either handle inheritance or a call to the DuplicateHandle
  function. Operations performed with such duplicate handles generate completion notifications. Careful consideration
  is advised." [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-createiocompletionport]
- Completion-thread count, documented on the API page: NumberOfConcurrentThreads is "the maximum number of threads
  that the operating system can allow to concurrently process I/O completion packets … If this parameter is zero, the
  system allows as many concurrently running threads as there are processors in the system" — the classic "completion
  concurrency = processor count" guidance, verbatim. The overview page's longer prose ("one thread per processor is
  the best overall maximum", "≥2× threads per CPU" rule of thumb) was not re-fetched → **OPEN**; the API-page rule is
  the cited core. [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-createiocompletionport]
- GetQueuedCompletionStatus: "Attempts to dequeue an I/O completion packet from the specified I/O completion port. If
  there is no completion packet queued, the function waits for a pending I/O operation associated with the completion
  port to complete"; "This function associates a thread with the specified completion port. A thread can be associated
  with at most one completion port." Failure semantics are contractual: *lpOverlapped == NULL ⇒ nothing dequeued
  (timeout, or ERROR_ABANDONED_WAIT_0 if the port handle closed while the call was outstanding); *lpOverlapped != NULL
  + FALSE ⇒ a packet for a **failed** I/O was dequeued. Batched dequeue: "To dequeue multiple I/O completion packets
  at once, use the GetQueuedCompletionStatusEx function."
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus]
- Event/IOCP interplay: an hEvent "whose low-order bit is set prevents the completion of the overlapped I/O from
  enqueing a completion packet to the completion port" — pick one notification mechanism deliberately. ReadFile: "If
  the hFile parameter is associated with an I/O completion port, you can also get the number of bytes read by calling
  the GetQueuedCompletionStatus function."
  [https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]
- Inline-completion double-free hazard, documented: "if the application frees resources after WriteFile returns TRUE
  with ERROR_SUCCESS in addition to in the I/O completion port routine, it will have a double-free error condition …
  allow the completion port routine to be solely responsible for all freeing operations for such resources." And: "If
  the handle is deallocated prematurely, ReadFile or WriteFile may incorrectly report that the I/O operation is
  complete." [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o]
- Managed alternative (CreateThreadpoolIo/StartThreadpoolIo) availability: "The original thread pool has been
  completely rearchitected in Windows Vista. The new thread pool is improved because it provides a single worker
  thread type (supports both I/O and non-I/O) … and a new thread pool API" — the CreateThreadpoolIo /
  StartThreadpoolIo family is that Vista+ API (Win7 SP1 covered; per-function requirement tables not re-fetched). Pool
  sizing, same page: "By default, each thread pool has a maximum of 500 worker threads. The thread pool attempts to
  create more worker threads when the number of worker threads in the ready/running state must be less than the number
  of processors." [https://learn.microsoft.com/en-us/windows/win32/ProcThread/thread-pools]
- Threadpool cancel discipline (best practices, same page): "An I/O request can run on any thread in the thread pool.
  Canceling I/O on a thread pool thread requires synchronization … always provide the OVERLAPPED structure with which
  an I/O request was initiated when calling CancelIoEx for asynchronous I/O"; never TerminateThread pool threads /
  ExitThread from callbacks; restore thread state per callback.
  [https://learn.microsoft.com/en-us/windows/win32/ProcThread/thread-pools]

## 3. FILE_FLAG_NO_BUFFERING alignment rules; physical vs logical sector (512e)

- Where the rules live: the File Buffering page, linked from CreateFile's NO_BUFFERING entry ("There are strict
  requirements … for details see File Buffering"); CreateFile: "The file or device is being opened with no system
  caching for data reads and writes. This flag does not affect hard disk caching or memory mapped files."
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew]
- Rule 1 — sizes and offsets: "File access sizes, including the optional file offset in the OVERLAPPED structure, if
  specified, must be for a number of bytes that is an integer multiple of the volume sector size. For example, if the
  sector size is 512 bytes, an application can request reads and writes of 512, 1,024, 1,536, or 2,048 bytes, but not
  of 335, 981, or 7,171 bytes." [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- Rule 2 — buffer addresses: "File access buffer addresses for read and write operations should be physical
  sector-aligned … Depending on the disk, this requirement may not be enforced." (Logical-sector multiples = hard
  rule; physical-sector buffer alignment = "should" — enforce it ourselves.)
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- Volume handles specifically (CreateFile remarks): "if the noncached option is specified when opening a volume,
  buffer alignment is enforced regardless of the file system on the volume. It is recommended on all file systems that
  you open volume handles as noncached, and follow the noncached I/O restrictions"; "You should assume that all
  Microsoft file systems open volume handles as noncached. The restrictions on noncached I/O for files also apply to
  volumes"; volume tail: "To read or write to the last few sectors of the volume, you must call DeviceIoControl and
  specify FSCTL_ALLOW_EXTENDED_DASD_IO."
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew]
- 512e (Advanced Format), Microsoft's documented model: devices "emulate a regular 512-byte sector storage device but
  make available information about the true sector size"; **Logical Sector** = "the smallest unit of write that the
  storage can accept. This is the 'emulation'"; **Physical Sector** = "The unit for which read and write operations to
  the device are completed in a single operation. This is the unit of atomic write, and what unbuffered I/O will need
  to be aligned to in order to have optimal performance and reliability characteristics."
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- Query API: "the physical sector size can be retrieved through the IOCTL_STORAGE_QUERY_PROPERTY control code, with
  the relevant information contained in the BytesPerPhysicalSector member in the STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR
  structure … Microsoft strongly recommends that developers align unbuffered I/O to the physical sector size as
  reported by the IOCTL_STORAGE_QUERY_PROPERTY control code." Members: BytesPerLogicalSector, BytesPerPhysicalSector,
  BytesOffsetForSectorAlignment, BytesPerCacheLine, BytesOffsetForCacheAlignment; "Minimum supported client — Windows
  Vista" ("introduced with Windows Vista and Windows Server 2008").
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_access_alignment_descriptor]
- Documented 64 KiB-multiple length effect: none in Microsoft docs. Single engineering source (colibri #867):
  unbuffered lengths not 64 KiB multiples "took a ~3.5x slow path in our measurements", with the same page
  self-caveating: "Row 3's slow path is real on that box as measured, but its exact trigger is under re-measurement —
  see issue #863 … before quoting the 3.5x figure." Treat 64 KiB length-multiplicity as free hygiene, not an invariant
  → cross-box magnitude **OPEN**. [https://github.com/JustVugg/colibri/issues/867]

## 4. Read-size and queue-depth optima per device class

- Authoritative status: Microsoft documents **no** per-class optimum read size or QD. What exists: device-side queue
  ceilings, one engineering QD sweep, virtualized-platform specs. Per-class optima on real fixtures stay **OPEN** (the
  session's Phase-2 sweep is the missing measurement).
- NVMe ceiling (documented, via the NCQ page): "For PCIe-based NVMe SSDs, the queue depth was even increased to
  support a maximum of 65,535 queues with up to 65,535 commands each" — 64Ki-scale, never the bottleneck for QD≤64
  designs (NVMe spec primary not fetched → last-digit precision OPEN).
  [https://en.wikipedia.org/wiki/Native_Command_Queuing]
- NVMe QD sweep + read size (engineering datapoint; single box: Windows 11, Gen4 NVMe; credibility = self-reported
  measurement with explicit protocol): 4K random QD1 = **75 MB/s** ("measured latency-bound small blocks"); unbuffered
  64K-multiple **QD4+ = ~2900–3000 MB/s** ("the honest number for this box"); buffered 128K QD32 = **3683 MB/s**
  ("page-cache warm — measured RAM, not disk"); unbuffered 4K-aligned non-64K-multiple = **675 MB/s** (slow-path row,
  trigger under re-measurement per the same page). Same source: "'4K random' and '1 MiB random' differ by 40x on the
  same drive; neither is 'the disk's bandwidth'." Read-size plateau: ≥64 KiB multiples at QD4+ on the ~3 GB/s plateau;
  no systematic 64K→4M sweep fetched (#863 not fetched) → **OPEN**. [https://github.com/JustVugg/colibri/issues/867]
- SATA SSD: "SATA solid-state drives profit significantly from being able to queue multiple commands for parallel
  workloads" (NCQ ceiling 32, below); concrete QD4–16 optima: no authoritative source fetched → **OPEN**.
  [https://en.wikipedia.org/wiki/Native_Command_Queuing]
- HDD (NCQ): "Both NCQ and TCQ have a maximum queue length of 32 outstanding commands"; NCQ "allows the drive itself
  to determine the optimal order in which to retrieve outstanding requests … reduce the amount of unnecessary drive
  head movement" — the mechanism that makes QD>1 pay on random MFT-hit patterns. Measured-effect ranges, same page: a
  2010 review found "improvements on the order of 9% (on average) … in a series of Windows multitasking tests" (a 2004
  first-gen test found desktop performance *decreased*); "On some drives' firmware … read-ahead is disabled when NCQ
  is enabled, resulting in slower sequential performance" — sequential HDD scanning may prefer shallow queues.
  Often-quoted QD8–32 optimum: per-drive, unfetched → **OPEN**. Reorder worst case: "a queued request can be delayed …
  up to 1–2 seconds" (Google tests, 2008). Host side: "Windows Vista and Windows 7 also natively support AHCI" (NCQ
  enabler). [https://en.wikipedia.org/wiki/Native_Command_Queuing]
- USB mass-storage: BOT has "an inability to perform command queueing or out-of-order command completions" ⇒ effective
  QD ≈ 1; UAS/UASP was created to fix exactly that — "Enables command queuing and out-of-order completions for USB
  mass-storage devices … Up to 64K commands may be queued"; "When used with an SSD, UAS is considerably faster than
  BOT for random reads and writes given the same USB transfer rate"; Windows: "Microsoft added native support for UAS
  to Windows 8. Drives supporting UAS load Uaspstor.sys instead of the older Usbstor.sys." Win7+BOT bridge ⇒ design to
  QD1–2; precise optima **OPEN**. [https://en.wikipedia.org/wiki/USB_Attached_SCSI]
- SD: nothing authoritative fetched — treat as USB-class media behind a reader → **OPEN**.
- User-mode Windows-side QD ceiling: ReadFile fails with ERROR_INVALID_USER_BUFFER / ERROR_NOT_ENOUGH_MEMORY "whenever
  there are too many outstanding asynchronous I/O requests" — the OS-side cap any deep-window design must respect.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]

## 5. Azure / GitHub-hosted Windows runner storage (why ~240 MiB/s + QD-flat is expected)

- Runners are Azure VMs, fresh per job: "GitHub hosts Windows runners and Linux runners that don't use multi-cloud
  compute on virtual machines in Azure"; "With the exception of single-CPU runners, each GitHub-hosted runner is a new
  virtual machine (VM) hosted by GitHub." [https://docs.github.com/en/actions/concepts/runners/github-hosted-runners]
- Azure per-VM caps are published at a *defined* queue depth (Dv2-series local temp disk, best case): Standard_D2_v2 =
  1 temp disk / 100 GiB / **6,000 random-read IOPS / 93 MBps sequential read / 46 MBps sequential write**; D3_v2 =
  12,000 / 187 / 93; D4_v2 = 24,000 / 375 / 187; D5_v2 = 48,000 / 750 / 375. Footnote verbatim: "Temp disk performance
  depends on many factors including block size, workload patterns of read/writes, queue depth (QD), and others. Temp
  disk performance specifications should be viewed as best case performance numbers, assuming 4k block sizes and
  QD=256 for IOPS, and 256k block sizes with QD=64 for throughput." (Uncached premium-SSD column: D2_v2 = 8×500 IOPS.)
  [https://learn.microsoft.com/en-us/azure/virtual-machines/sizes/general-purpose/dv2-series]
- Why that predicts QD-flat + a hard ceiling: the platform's published numbers are already measured at QD64–256; a
  per-VM/per-disk cap set below hardware capability converts extra outstanding requests into latency, not throughput —
  a ~240 MiB/s plateau at which QD>1 buys nothing is exactly the signature of a capped virtualized disk, and matches
  the session's own runner measurement (128 KiB chunks 240.0 MiB/s at QD1, size×QD sweep flat; internal
  worklog/context-sheets/CORE-SURFACE.md, run 40227375329). Units: Azure "MBps = 10^6 bytes/sec" (93 MBps ≈ 88.6
  MiB/s; 240 MiB/s ≈ 252 MBps) — same order as D2_v2-class caps. No fetched page names the exact runner disk
  class/spec (the runners page no longer carries a storage-spec table) → external corroboration of the exact 240
  figure is **OPEN**; our CI baseline is its source.
  [https://learn.microsoft.com/en-us/azure/virtual-machines/sizes/general-purpose/dv2-series] ·
  [https://docs.github.com/en/actions/concepts/runners/github-hosted-runners]

## 6. FILE_FLAG_NO_BUFFERING working-set/cache benefit; buffer pool sizing (VirtualAlloc); N×S memory

- Cache pollution avoided by construction: NO_BUFFERING = "opened with no system caching for data reads and writes"
  (CreateFile) / "complete and direct control over data I/O buffering" (File Buffering) — a multi-GB sequential scan
  does not cycle gigabytes through the system cache; FILE_FLAG_SEQUENTIAL_SCAN / FILE_FLAG_RANDOM_ACCESS are
  cache-manager hints with "no effect … FILE_FLAG_NO_BUFFERING"; on-board hard-disk caching is unaffected by the flag.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew] ·
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- Async-serialization angle: the fetched pages document "can also behave synchronously" and link the KB titled
  *Asynchronous disk I/O appears as synchronous on Windows*; the KB's specific NO_BUFFERING-is-the-best-guarantee
  statement was not re-fetched → **OPEN** (not re-asserted).
  [https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o] ·
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile]
- Buffer pool allocation, documented: "One way to sector-align buffers is to use the VirtualAlloc function to allocate
  the buffers … VirtualAlloc allocates memory that is aligned on addresses that are integer multiples of the system's
  page size. Page size is 4,096 bytes on x64 and x86 … Both page and sector sizes are powers of 2. Therefore, in most
  situations, page-aligned memory will also be sector-aligned, because the case where the sector size is larger than
  the page size is rare." CRT alternative: `_aligned_malloc`; "Sector size is typically 512 to 4,096 bytes for
  direct-access storage devices (hard drives) and 2,048 bytes for CD-ROMs."
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- N outstanding reads of size S: pool = N×S (8×1 MiB = 8 MiB … 8×4 MiB = 32 MiB class) of VirtualAlloc'd page-aligned
  buffers, one unique OVERLAPPED each, untouched until completion (§1); N bounded above by the
  ERROR_INVALID_USER_BUFFER/ERROR_NOT_ENOUGH_MEMORY ceiling (§4) and by working-set sanity (design reasoning, not a
  doc claim). VirtualLock/working-set rules not fetched → **OPEN**.
  [https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile] ·
  [https://learn.microsoft.com/en-us/windows/win32/FileIO/file-buffering]
- Cache-state labeling is part of the number (engineering protocol): "A warm number without the label is a lie by
  omission" — e.g. 6.72 GB/s cold vs 20.46 warm on the same shard; every benchmark this session adopts must carry
  cache state + request size + alignment + QD + a control measurement.
  [https://github.com/JustVugg/colibri/issues/867]

## OPEN register (unresolved this retry)

- O1 — KB 156932 content: linked by two fetched pages, page itself not fetched; recommendations not re-asserted.
- O2 — I/O Completion Ports overview page: "one thread per processor" prose + "≥2× threads per CPU" rule +
  modern CreateThreadpoolIo-vs-raw-IOCP comparison not re-fetched; the API-page concurrency rule stands.
- O3 — Cancel worst-case latency per transport: no documented number (only the HDD NCQ 1–2 s datapoint); measure.
- O4 — Per-class QD/read-size optima (NVMe size sweep, SATA SSD, USB, SD): no authoritative sources; one-box NVMe
  datapoints only; the session's sweep is the missing measurement.
- O5 — "~3.5x non-64K-multiple slow path": single engineering source, self-flagged under re-measurement.
- O6 — Exact runner-disk class behind the ~240 MiB/s cap: runners are documented Azure VMs, caps documented at
  defined QD, but no fetched page names the runner disk/spec; 240 MiB/s stays our own CI measurement.
- O7 — VirtualLock / working-set-enlargement guidance for pinned pools: not fetched.
- O8 — File Caching page (lazy-writer/cache-sizing specifics): cited by the earlier 2-D sheet, not re-fetched.

## Design implications for a Windows recovery engine

1. Open raw handles as admin + OPEN_EXISTING + FILE_SHARE_WRITE (volumes) + FILE_FLAG_OVERLAPPED |
  FILE_FLAG_NO_BUFFERING — Microsoft explicitly recommends noncached volume handles; OVERLAPPED is the *only* mode in
  which "the file can be used for simultaneous read and write operations". [createfilew]
2. Architecture baseline: overlapped reads + IOCP sliding window of N outstanding reads refilled on completion — vs
  sync QD1 the measured gap on one NVMe is 75 MB/s (4K QD1) vs ~2.9–3.0 GB/s (unbuffered 64K-multiple QD4+); on the
  virtualized CI target expect QD-flatness above the cap, so QD1+large-read is adequate *there* but must not be the
  shipping default. [colibri #867; dv2-series; createiocompletionport]
3. Completion thread count: IOCP NumberOfConcurrentThreads = CPU count (0 ⇒ processor count, documented); or the
  Vista+ threadpool I/O objects (Win7-safe) with their cancel-synchronization discipline; the 2×-threads rule of thumb
  stays OPEN (overview page unfetched). [createiocompletionport; thread-pools]
4. One unique OVERLAPPED per outstanding read, real hEvent (never the handle-signaled state), completion path solely
  owns frees (inline-completion double-free hazard); don't gate the hot path on GetOverlappedResult (bWait=TRUE) — a
  blocking wait equal in latency to sync I/O; drain via GetQueuedCompletionStatus(Ex). [readfile; syncasync;
  getoverlappedresult; gqcs]
5. Read-size defaults per transport class (starting points; sweep on fixtures): NVMe ≥ 1 MiB unbuffered 64K-multiple
  (plateau at QD4+, ~3 GB/s one-box); virtualized/runner 256 KiB–1 MiB (Azure spec basis 256k @ QD64); HDD 256 KiB–1
  MiB sequential with shallow queue (firmware read-ahead can be disabled by NCQ); USB BOT 64–256 KiB at QD1–2 (UASP on
  Win8+ may queue); SD = USB-class, OPEN. [colibri #867; dv2-series; w:NCQ; w:UAS]
6. Alignment guarantees: lengths+offsets multiples of volume sector size (hard rule); buffer addresses physical-sector
  aligned (should-rule); query BytesPerPhysicalSector via IOCTL_STORAGE_QUERY_PROPERTY +
  STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR (Vista+) per handle, never hardcode; VirtualAlloc pool ⇒ page-aligned ⇒
  sector-aligned "in most situations". [file-buffering; storage_access_alignment_descriptor]
7. Round unbuffered lengths to 64 KiB multiples as free hygiene (clamp at device size; FSCTL_ALLOW_EXTENDED_DASD_IO
  for the volume tail) — but the 3.5x claim is single-source and self-flagged as under re-measurement; not an
  invariant. [file-buffering; createfilew; colibri #867]
8. Queue-depth defaults per class: NVMe 8–16 (device ceiling 64Ki-scale; the pool and the OS-side
  ERROR_INVALID_USER_BUFFER ceiling bind first); SATA SSD 4–16 within NCQ 32 (OPEN); HDD small-to-16 within NCQ 32
  (reorder benefit documented, sequential caution documented); USB BOT ≈ 1, UASP small; virtualized runner: expect
  flat above the VM cap. [w:NCQ; readfile; w:UAS; dv2-series]
9. Cancellation design: CancelIoEx(handle, specific OVERLAPPED) per in-flight read; handle all three terminal
  statuses; never free/reuse the OVERLAPPED until completion (GetOverlappedResult/GQCS says when); user-abort =
  cancel-all → drain completions → only then close handles and free buffers; ERROR_NOT_FOUND benign; expect up to ~1–2
  s device-side drag on deep HDD queues; on threadpool threads always pass the exact OVERLAPPED. [cancelioex; gqcs;
  thread-pools; w:NCQ]
10. Buffer pool: VirtualAlloc N×S page-aligned (8×1–4 MiB = 8–32 MiB class), each paired with its OVERLAPPED,
  untouched until completion; N bounded by the ERROR_INVALID_USER_BUFFER/ERROR_NOT_ENOUGH_MEMORY ceiling;
  pinned-buffer policy OPEN. [file-buffering; readfile]
11. Benchmark protocol for every number this session produces: cache state + request size/alignment + actual access
  pattern + control measurement, or the number is not citable (warm numbers measured RAM at 3683 MB/s on one box; 6.72
  GB/s cold vs 20.46 warm on another). [colibri #867]
12. CI reality: GitHub-hosted Windows runners are per-job Azure VMs; Azure caps are defined at QD64/256 — a ~240 MiB/s
  QD-flat plateau is the expected signature, so runner-relative numbers never validate a bare-metal design; keep the
  two result classes labeled apart. [docs.github runners; dv2-series]

## Relevance caveat — what the bar to beat actually is

The reference product (EaseUS DRW, the og-codebase this session reverse-engineers) uses **no overlapped I/O and
no IOCP anywhere in its scan path**: the corpus decompiles show "zero CreateIoCompletionPort /
GetQueuedCompletionStatus / PostQueuedCompletionStatus matches … every OVERLAPPED argument that appears is
(LPOVERLAPPED)0x0 — NULL = synchronous DeviceIoControl/ReadFile" (worklog/REFERENCE-ARCHITECTURE.md §5, lines
175–186), and its read-ahead engine is one dedicated prefetch thread filling a 10-slot ring of 4 MiB-class runs —
"10 pre-allocated run buffers", "up to 10 outstanding run-reads ahead of the consumer, one synchronous device read
in flight", gated by two events; run size 0x400000/sector = 4 MiB with a 1000-sector default
(worklog/REFERENCE-ARCHITECTURE.md §4 line 121 and §5 lines 187–205; worklog/REFERENCE-RAW.md line 317: "prefetch
run size (1000 sectors) and lookahead (10 runs) — CONFIRMED"). So the number to beat is **prefetch-hidden
synchronous read-ahead — effective device QD≈1 with ~10 runs (~40 MiB) of look-ahead absorbing consumer latency**
— not an IOCP engine: a plain synchronous reader with the same 4 MiB-run/10-run window already matches it, and any
genuine concurrency (2–8 outstanding) plus per-class tuning is upside. It also means the CI-measured ~240 MiB/s
QD-flat runner plateau cannot distinguish QD1-sync from IOCP designs — only bare-metal NVMe/HDD fixtures can — so
the Phase-2 I/O-model decision should be gated on the fixture sweep, with virtualized-runner numbers labeled as
their own class.

## Source index (all fetched directly, HTTP 200, 2026-11-18; scratch copies in /tmp, not committed)

- learn.microsoft.com (11 pages): nf-fileapi-createfilew · nf-fileapi-readfile · nf-ioapiset-getoverlappedresult ·
  nf-ioapiset-cancelioex · nf-ioapiset-createiocompletionport · nf-ioapiset-getqueuedcompletionstatus ·
  fileio/synchronous-and-asynchronous-i-o · FileIO/file-buffering · api/winioctl/ns-winioctl-storage_access_alignment_descriptor
  · azure/virtual-machines/sizes/general-purpose/dv2-series · win32/ProcThread/thread-pools.
- docs.github.com: actions/concepts/runners/github-hosted-runners (redirect target of
  using-github-hosted-runners/about-github-hosted-runners; both URLs fetched — one page).
- en.wikipedia.org: Native_Command_Queuing · USB_Attached_SCSI.
- github.com: JustVugg/colibri/issues/867 (engineering-measurement class, self-caveated).
- Not fetched this retry (so not asserted above): KB 156932 · I/O Completion Ports overview · File Caching ·
  CreateThreadpoolIo/StartThreadpoolIo function pages · colibri #863 · runner-images issues · nvmexpress.org — see
  OPEN register.

