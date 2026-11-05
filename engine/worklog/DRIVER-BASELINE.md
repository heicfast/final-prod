# DRIVER-BASELINE — the RDA driver's performance record (the direct path's TARGET)

> **Provenance.** Driver repo `rda-driver` read end-to-end at commit `323e75acf4cd745960813af4e7fd586a4a34d1a0` (tag `v0.2.4`, branch `main`; git-verified at read time), **READ-ONLY — never modified**. Analysis date 2026-11-05. Agent C / task T2-c of the Phase-0 driver-performance set.
>
> Companion sheets: `core_recovery/worklog/DRIVER-CONTRACT.md` (the wire-contract sheet, written against this exact driver commit — the semantic authority; where this sheet and that one disagree, the driver source wins for both) and `core_recovery/worklog/PERFORMANCE.md` (the session's measurement ledger — §1 holds the captured driver-era numbers this sheet consolidates and extends).
>
> **The rule:** this document is the performance **TARGET**. The direct user-mode raw-I/O path that replaced the driver must **meet or beat** every number in §3 and every guarantee in §4 on the same runner class with the same disk profile and the same verbs. No performance claim survives without a captured measurement; every number below carries its run ID and source.

---

## §1 The driver's data-path architecture, end to end

### 1.1 Device model and dispatch

- One WDM kernel driver installed as a demand-start kernel service; the service name derives the control device identity: `\\.\RDA` → `\Device\RDA` + `\DosDevices\RDA` (fallback `EPMNTDRV`), `rda-driver/src/driver/rda_control.c:4-10,96-101,110-136`. Control device is `FILE_DEVICE_UNKNOWN` (0x22) with **`DO_DIRECT_IO`** set — `rda_control.c:16-17,152-160` — so the IO manager probes/locks the caller's buffer and builds an MDL for every READ/WRITE. Created with `IoCreateDeviceSecure` + SDDL `D:P(A;;GA;;;BA)(A;;GA;;;SY)` (Administrators/SYSTEM only) — `rda_control.c:141-153`, `rda-driver/src/driver/inc/rda_p.h:59-68`.
- **Dispatch model: one master dispatcher on all 28 IRP majors** routes `DeviceObject == g_RdaControlDevice` through a 28-slot control table (CREATE@0x00, CLOSE@0x02, READ@0x03, WRITE@0x04, DEVICE_CONTROL@0x0E, CLEANUP@0x12 owned; everything else `STATUS_INVALID_DEVICE_REQUEST`), any other device object to the filter path — `rda-driver/src/driver/rda_driver.c:62-111`. The control device has **no attached lower device of its own** — it is a terminal dispatch point that *re-issues* IO to a separately resolved target (below).

### 1.2 Handle creation: the grammar path and target resolution (the "session open" cost)

- Sub-device opens are **file names relative to `\\.\RDA`**, classified at CREATE into GLOBAL / VOLUME / DISK / DISK_EPM / REJECT (`rda-driver/src/driver/rda_create.c:43-69`), gated by the PID-token handshake re-derived from `PsGetCurrentProcessId()` (`rda_create.c:73-77`) and disk-number bounds (DISK ≥ MaxDisks=200, DISK_EPM ≥ 100 — `rda_create.c:79-85`, `rda_p.h:55-56`).
- Target resolution **per open** (no cross-handle caching of grammar targets): VOLUME opens `\Device\HarddiskVolumeN` via `IoGetDeviceObjectPointer` then takes the **top of the volume stack** (`IoGetAttachedDeviceReference`) — `rda-driver/src/driver/rda_target.c:129-167`; DISK opens `\Device\HarddiskN\Partition0` (retry `\Device\HarddiskN\DRN`) then **walks DOWN** past filter devices to the first layer whose `ObQueryNameString` name contains `\Device\Harddisk` — `rda_target.c:169-245` (retry `:189`, walk `:216-228`); DISK_EPM walks down to the first `\Driver\Disk` layer — `rda_target.c:247-300`. The partition number is never used (whole-disk access; partition selection is client offset math).
- The resolved layer is stashed in a per-handle context `{TargetFileObject, TargetDevice, Mode, Kind, SectorSize}` in `FsContext`/`FsContext2` — `rda_p.h:81-95`, `rda_create.c:87-116` — released at **CLEANUP** (fix F9) together with a sweep of the opening process's copy-sessions — `rda_create.c:140-168`. CLOSE is a trivial success — `rda_create.c:170-179`. No handle-count limit, `Exclusive=FALSE`, unrestricted sharing (`rda_control.c:152`).
- A logical-sector-size probe (`IOCTL_DISK_GET_DRIVE_GEOMETRY` against the attached stack) runs per open, **cached per unique device pointer** (bounded 64 entries, oldest-evicted, ~100 ns hit path — the round-2 fix for a +10% open-latency gap) — `rda_target.c:302-445`.

### 1.3 The READ/WRITE path mechanics (the data path)

`RdaDispatchRead/Write` → `RdaDispatchReadWriteInner` (`rda-driver/src/driver/rda_readwrite.c:280-367,421-431`) → `RdaForwardSynchronous` (`:185-278`):

1. Fetch the per-handle context from `FsContext2` (`:290-291`); map the caller's MDL once with `MmGetSystemAddressForMdlSafe(NormalPagePriority)` — NULL → `STATUS_INSUFFICIENT_RESOURCES` (`:294-305`).
2. Extract `Length` (BYTES) and `ByteOffset` (absolute byte offset on the target) from the IRP stack (`:307-315`; contract header `rda_readwrite.c:4-15`, `rda-driver/src/shared/inc/rda_ioctl.h:131-145`). Zero-length or unmapped-MDL requests reject with `STATUS_INVALID_PARAMETER` (`:342-347`, sane divergence S10 — the reference NULL-derefenced).
3. `RdaForwardSynchronous`: negative-offset reject (`:196-198`); per-IO transfer context `{Wake, DoneFlag, Cancelled, CancelDone, CancelStarted, Iosb, ForwardIrp}` taken from a driver-wide **SLIST free-list pool** (16-byte aligned, depth-capped 32, heap fallback — `rda_readwrite.c:49-64,78-87,134-141`); two `KeInitializeEvent`s (`:204-205`); cancel routine armed BEFORE forwarding with an already-cancelled race check (`:210-222`).
4. **A fresh driver-built IRP** per request: `IoBuildAsynchronousFsdRequest(MajorFunction, TargetDevice, SystemAddress, Length, ByteOffset, &ctx->Iosb)` (`:224-225`), completion routine returning `STATUS_MORE_PROCESSING_REQUIRED` (`:154-167,234`), then `IoCallDriver(TargetDevice, forward)` (`:236`) — the IO enters the storage stack at the **resolved disk-class/volume layer**, i.e. a second IRP in addition to the caller's.
5. **Synchronous wait in the caller's thread**: flag-checked `KeWaitForSingleObject` loop (`:242-257`); on a cancel wake the dispatcher propagates cancellation INTO the storage stack with `IoCancelIrp(forward)` on the still-owned forward IRP (`:250-256`).
6. Single-completer teardown: disarm cancel (wait for `CancelDone` if it fired — `:261-263`), select status (`STATUS_CANCELLED` when cancelled, else the forward IRP's verbatim status — `:266`), **on success `Information = Length × 512`** (`RDA_LEGACY_INFORMATION_SCALE` = 512, `rda_ioctl.h:146`; applied at `rda_readwrite.c:267-273` — fix F11, the intentionally preserved legacy convention; clients divide by 512, `rda-driver/src/bench/rdabench.c:73-79`), `IoFreeIrp(forward)` (`:275`), context back to the pool (`:276`), caller IRP completed in-line (`:363-366`). On failure `Information = 0` and the storage stack's NTSTATUS propagates verbatim with error trace lines (`:352-361`).

**There is no data copy and no bounce buffer in the driver**: the system VA of the caller's own locked pages is handed to the forward IRP (`rda_readwrite.c:224-225`).

### 1.4 Request sizing, queue depth, completion model

- **No splitting, no chunking, no read sizing, no merging, no caching, no retry**: one forward IRP per request, exactly the client's byte length, immediately awaited. Buffer sizes are entirely client-determined — the in-tree bench uses up to 16 MiB (`rdabench.c:38`) with a default 1024 KiB block (`rdabench.c:430`); the driver imposes no maximum (the IO manager/MDL and the target stack bound it).
- **No queue, no queue depth**: the driver never queues a grammar-path request. Concurrency is purely whatever the client issues — any number of handles/threads, no driver-side serialization, no per-handle lock; the only shared state is the free-list and the global-handle slot. The session engine's queue machinery (worker thread, 2000-deep pump queue, kick cap 301, spb-sized chunks — `rda-driver/src/driver/rda_session.c:65-73`) applies **only** to the copy-session protocol, which no live core code uses (DRIVER-CONTRACT §0 D5).
- **Completion model exposed to user mode: synchronous in-line** — the caller IRP is completed before the dispatch returns; OVERLAPPED clients get the IO manager's synchronous-handle serialization, not driver async. The only `STATUS_PENDING` producer in the driver is the session GETNEXTBLOCK pump (`rda-driver/src/driver/rda_ioctl.c:295-299`; `rda_session.c`).

### 1.5 Buffer management summary

`DO_DIRECT_IO` → IO-manager MDL over the caller's pages (`rda_control.c:159-160`); one `MmGetSystemAddressForMdlSafe` map per request (`rda_readwrite.c:294-305`); the mapped system VA is the forward IRP's buffer — **zero copies of the data, zero bounce buffers**. Pages stay locked until the forward IRP completes (the cancel design depends on it — `rda_readwrite.c:40-42`).

### 1.6 Alignment preconditions: none (the verbatim-forward claim)

**No driver-side alignment enforcement exists on the grammar path**: byte offsets and byte lengths forward verbatim; the probed `SectorSize` is stored in the context but never consulted by READ/WRITE (DRIVER-CONTRACT §0 D2; set at `rda_target.c:152,238,293`, consumed only by the session/direct/RRWR family — `rda-driver/src/driver/rda_disk.c:191-193`). Sector alignment is the storage stack's job: an unaligned request is rejected by the target, and that status propagates. The sector-size value IS used by the session engine's LBA translation (`byteOffset = LBA × SectorSize` — `rda_disk.c:191-193`) and the direct/sized commands.

### 1.7 IOCTL surface

Dispatch order on the control device (`rda_ioctl.c:247-286`): (1) session-family codes (19 recognized, normalized by `IoctlBase`) on any handle when `SessionsEnabled != 0` (`rda_ioctl.c:38-72`); (2) bound handles: `0x60002408` GET_DISK_CONTEXT_MODE answered locally (`rda-driver/src/driver/rda_ident.c:163-174`), **everything else passed through**; (3) global handles: identity IOCTLs `0x60002400` GET_PROC_NAME / `0x60002404` GET_FILE_OBJ_NAME (`rda_ident.c:28-161`) plus extended `0x6002440` GET_INFO (`rda_ioctl.c:75-98`), `0x6002444` OPEN_TARGET (`rda_ioctl.c:115-184`), `0x6002448` GET_VERSION (`rda_ioctl.c:100-110`); anything else → `STATUS_INVALID_PARAMETER` (`rda_ioctl.c:282-285`). Pass-through proxy: `IoGetAttachedDeviceReference(Context->TargetDevice)` (the *current* top), `IoBuildDeviceIoControlRequest`, synchronous wait, `Information = iosb.Information`, lower status verbatim; METHOD_BUFFERED/DIRECT honored (fix F10), METHOD_NEITHER rejected — `rda_ioctl.c:186-245`. Nothing is special-cased per code — geometry, layout-modifying, storage-query and SMART IOCTLs all ride the same generic proxy.

### 1.8 Session/engine model (documented; not in the core's production surface)

OPEN_TARGET binds a target without the path-string grammar (`rda_ioctl.c:115-184`); the copy-session engine is a 64-slot generation-tagged table with per-session system worker threads servicing a spinlock-guarded pump queue (caps: 2000 queued, 301 kicks, 20 flush retries, 64 MiB COW, 10 s guard — `rda_session.c:65-73`). The core never issued a session command (DRIVER-CONTRACT §0 D5) — this machinery is replacement-irrelevant except as measured dispatch overhead (its codes are in the fuzz hammer's table).

### 1.9 Cancellation

Full `CancelIRP` support on the data path (fix F4 — the reference hung on dead drives): cancel routine armed before forwarding (`rda_readwrite.c:213`), already-cancelled race fail-fast (`:218-222`), cancel wake propagates `IoCancelIrp` into the storage stack (`:250-256`), `STATUS_CANCELLED` to the caller, single-completer invariant, buffer pages locked until the forward completes — `rda_readwrite.c:23-42,105-121,242-277`. The cancel routine releases the global cancel spinlock (fixes S1/S2 — `rda_readwrite.c:89-121`).

### 1.10 Error propagation

Verbatim, GetLastError-style passthrough: the lower stack's NTSTATUS **and** `Information` propagate unmodified; the driver adds no masking, no zero-fill, no retry, no remap, no timeout of its own — `rda_readwrite.c:266-273,352-366`; pass-through `rda_ioctl.c:236-244`. Full condition table (every class with citations) is DRIVER-CONTRACT §4.

---

## §2 The per-request cost model (kernel hops, locks, copies)

### 2.1 Every cost on the grammar-path READ, in order

Costs marked **[extra]** are what a direct `ReadFile` on `\\.\PhysicalDriveN` does **not** pay.

| # | Cost | Site | Class |
|---|---|---|---|
| 1 | User→kernel transition, IO-manager IRP build for the rda device, `DO_DIRECT_IO` buffer probe+lock + MDL build | IO manager; device contract `rda_control.c:159-160` | shared (the direct path pays the same shape for its own device) |
| 2 | Object-manager path resolution through the `\DosDevices\RDA` symlink to `\Device\RDA` | `rda_control.c:110-136,162` | **[extra]** (direct: one open of the disk device itself) |
| 3 | Master dispatcher + 28-slot table indirection | `rda_driver.c:97-111` | **[extra]**, ~ns |
| 4 | Context fetch, MDL presence check, params copy | `rda_readwrite.c:290-315` | **[extra]**, ~ns |
| 5 | `MmGetSystemAddressForMdlSafe` — build a system-VA mapping of the caller's locked pages | `rda_readwrite.c:294-305` | **[extra]** — the direct path's MDL is consumed by the disk stack without a driver-side system-VA map |
| 6 | Negative-offset + zero-length validation | `rda_readwrite.c:196-198,342-347` | **[extra]**, ~ns |
| 7 | Per-IO context: interlocked SLIST pop + `RtlZeroMemory` of the 7-field context (heap alloc on pool miss) | `rda_readwrite.c:78-87,200-203`; pool `:49-64` | **[extra]** — measured at −10% random IOPS before the pool existed (round-1 gap, §3.1 note) |
| 8 | 2 × `KeInitializeEvent` + `DriverContext` stash | `rda_readwrite.c:204-210` | **[extra]**, ~ns |
| 9 | Cancel-routine arm + already-cancelled race check (`IoSetCancelRoutine` ×2 under the cancel spinlock on the race path) | `rda_readwrite.c:213-222` | **[extra]** |
| 10 | **`IoBuildAsynchronousFsdRequest` — allocate + initialize a second IRP** (and its completion registration) | `rda_readwrite.c:224-225,234` | **[extra]** — the single largest fixed per-IO cost |
| 11 | `IoCallDriver` into the disk-class/volume layer — the storage stack sees one more IRP than the client issued | `rda_readwrite.c:236` | **[extra]** hop |
| 12 | Completion at DISPATCH_LEVEL: status copy, interlocked `DoneFlag`, `KeSetEvent`, `STATUS_MORE_PROCESSING_REQUIRED` | `rda_readwrite.c:154-167` | **[extra]** |
| 13 | Thread wake + flag-checked `KeWaitForSingleObject` wait loop | `rda_readwrite.c:242-257` | **[extra]** vs the direct path's native synchronous completion |
| 14 | Cancel disarm (+ conditional `CancelDone` wait), status select | `rda_readwrite.c:261-266` | **[extra]** |
| 15 | `Information = Length × 512` multiply (F11) + client-side ÷512 unscale | `rda_readwrite.c:267-273`; `rdabench.c:73-79` | **[extra]**, trivial arithmetic but per-IO |
| 16 | `IoFreeIrp` + SLIST push of the context | `rda_readwrite.c:275-276` | **[extra]** |
| 17 | `IoCompleteRequest` on the caller IRP | `rda_readwrite.c:363-366` | shared |

**No locks** are taken on the fast path (the SLIST uses interlocked operations, not a spinlock; the geometry cache's FAST_MUTEX is open-path only — `rda_target.c:380-390`). **No data copies** (§1.5). The theoretical statement: *the driver path = the direct path + one extra IRP lifecycle + one system-VA map + one pooled-context lifecycle + cancel arm/disarm + event wake + ×512 scale + the rda dispatch hop.*

### 2.2 The device-object stack (what the direct path faces instead)

The driver's DISK grammar binds, by walking down from `\Device\HarddiskN\Partition0`'s top, **the disk-class layer** — the same device object class that `\\.\PhysicalDriveN` opens natively (`rda_target.c:169-245`); the VOLUME grammar binds the **top of the volume stack** (volsnap and volume filters included) — equivalent to a volume device open (`rda_target.c:129-167`). A direct `\\.\PhysicalDriveN` handle therefore terminates at the same stack the driver forwarded into, minus the rda dispatch layer entirely; `\\.\C:`/`\\.\HarddiskVolumeN` handles traverse the volume manager/partition manager path the VOLUME grammar replicated. Pass-through IOCTLs took the *current* top (`IoGetAttachedDeviceReference` per IOCTL — `rda_ioctl.c:199`); the direct path issues them on its own handle with zero intermediate IRPs.

### 2.3 Measured anchors for the per-op overhead

- **Fuzz dispatch rate 841,698 IOCTL/s** = ~1.19 µs per complete user→kernel→driver→user round trip — the purest dispatch-path floor the driver ever measured (run 39807929475, §3.1); the +52% over the reference (553,752/s) proves dispatch-path cost is measurable and structural (leaner merged command table vs the reference's if-ladder — `rda-driver/docs/benchmarks/2026-10-22-runner-comparison.md:45-48`).
- **Random 4 KiB −2.9% IOPS / +3.0% latency vs the reference**, documented as the residual price of "cancel-safe, SDDL-secured IO machinery" after the SLIST pool fix (`2026-10-22-runner-comparison.md:39-44`) — the direct path removes exactly this machinery.
- **Open mean 8,217 ns vs reference 8,042 ns (+2.2% ≈ the SDDL check)** (`2026-10-22-runner-comparison.md:36-39`) — the direct path's device open replaces the grammar walk + SDDL with one object-manager open of the disk device.
- **Same-run driver-vs-direct legs (run 40315255368)**: core direct 64 KiB envelope 203.98 MiB/s vs the driver grammar leg 194.34 (−4.5%); direct 256 KiB 241.64 vs driver 237.54 (−1.7%) — same runner, span and QD1 (cross-target caveat noted in §3.4). At the 64 KiB shape this is ≈15 µs/read on the virtualized runner (~307 µs/read base) — consistent with the dispatch floor plus the virtualization's amplification of the extra IRP's completion path.
- The driver repo's own expectation baseline: "µs-class per-op dispatch overhead on 4 KiB random" and "≥90% of direct `\\.\PhysicalDriveN` sequential throughput" — `rda-driver/docs/benchmarking.md:64-71`. **The direct path is the 100% reference; the driver was the one budgeting to reach 90% of it.**

---

## §3 The captured performance record — the baseline of record

Runner class for every run: **windows-2022** (Windows Server 2022 Datacenter 10.0.20348, AMD EPYC 9V74 2C/4T @ 2596 MHz, hosted, elevated — `core_recovery/worklog/PERFORMANCE.md:6-8`). Disk profiles: **(P1)** 4 GiB seeded dynamic GPT VHDX, NTFS+FAT32, realistic JPEG/PDF/MP4/ZIP/text/1000-small-file corpus; **(P2)** 8 GiB VHDX, 512e (512 logical/4096 physical), NTFS clean volume, disk 4; **(P3)** the runner's live virtualized OS volume `\\.\C:`; **(P4)** seeded 3 GiB scenario VHDXs; **(P5)** `\\.\PhysicalDrive0` (the runner's OS physical device).

### 3.1 Run 39807929475 — the driver-vs-og parity table (driver repo bench lane)

Our test-signed `rda.sys` (service `rdatest` → `\\.\RDATEST`) vs the ORIGINAL WHQL-counter-signed `eprdtdrv.sys`, side by side, same `rdabench` binary, page-aligned buffers + `FILE_FLAG_NO_BUFFERING` — `rda-driver/docs/benchmarks/2026-10-22-runner-comparison.md:1-25`; `rda-driver/worklog.md:142-160`.

| Metric | RDA R2 (post-opt) | RDA R1 (pre-opt) | og (eprdtdrv) | Source |
|---|---:|---:|---:|---|
| seq 64 KiB throughput | **532.9 MiB/s** | 371.1 | 533.1 (worklog parity narrative; the table's 391.0 Original column mixes R1/R2 runs — ±5-10% runner variance noted) | `2026-10-22-runner-comparison.md:16`, `worklog.md:157-158,27-29` |
| seq 1 MiB throughput | (not recorded — see variance note) | 458.6 MiB/s | 500.6 MiB/s | `2026-10-22-runner-comparison.md:17` |
| rand 4 KiB IOPS (seeded LCG) | **7,932** | 7,932 | 8,170 (−2.9%) | `2026-10-22-runner-comparison.md:18` |
| rand 4 KiB latency mean | **126.1 µs** | 126.1 | 122.4 µs (+3.0%) | `2026-10-22-runner-comparison.md:19` |
| open mean (cold, grammar open) | **8,217 ns** | 10,993 | 8,042 (+2.2% ≈ SDDL) | `2026-10-22-runner-comparison.md:20,36-39` |
| open churn mean (100 open/close) | **8,532 ns** | 11,331 | 8,537 (parity) | `2026-10-22-runner-comparison.md:21` |
| fuzz hammer rate | **841,698 IOCTL/s** | 707,161 | 553,752 (+52%) | `2026-10-22-runner-comparison.md:22` |
| verify: 64/64 1 MiB SHA-256 ranges | **byte-identical, 0 mismatches** | — | byte-identical | `2026-10-22-runner-comparison.md:25` |
| geometry pass-through / Information scale | 1.0 / ×512 | 1.0 / ×512 | 1.0 / ×512 | `2026-10-22-runner-comparison.md:23-24` |
| hostile IOCTL survival | 50,000 survived, zero hangs | — | 50,000 survived | `2026-10-22-runner-comparison.md:47`; `docs/benchmarking.md:89-92` |

Round-1→round-2 fixes that moved these numbers (the driver's own measured performance lessons): per-open geometry probe **+10% open latency** → per-device cache, ~100 ns hit (`rda_target.c:302-445`; `worklog.md:148-151`); per-IO context heap alloc **−10% random IO** → SLIST pool (`rda_readwrite.c:49-64`; `worklog.md:149-151`). Live lesson: `ExAllocate/FreeFromLookasideListEx` are Win11-22H2-only real exports — importing them gave StartService error 127 on Server 2022 (`worklog.md:152-155`; `2026-10-22-runner-comparison.md:54-62`).

### 3.2 Run 41042267586 — the fresh pre-replacement baseline (the primary reference point)

test-benchmark smoke on the session-start tree `619acec2` (= release v0.6.1, driver installed and verified), reason `release-v0.6.1`, completed green — `core_recovery/worklog/PERFORMANCE.md:35-49`.

| Metric | Value | Source artifact |
|---|---|---|
| bench info (rda) | 8,589,934,592 B; 512/4096; 4Kn=false | clean-volume-benchmark.json |
| bench seq 1 MiB × 256 MiB (rda) | **536.92 MiB/s** | clean-volume-benchmark.json |
| bench rand 1 MiB × 1000 (rda) | **1993.22 µs mean; 502 IOPS; 1000/1000 hits** | clean-volume-benchmark.json |
| bench open × 1000 (rda) | **17,019 ns mean; 58,757 ops/s** | clean-volume-benchmark.json |
| scan quick (clean vol) | 150 ms wall | clean-volume-benchmark.json |
| scan deep (clean vol) | 16,505 ms wall; 1209 files; TTFR 0 ms | clean-volume-benchmark.json |
| scan deepcarve (clean vol) | 1,501,622 ms wall; 8,572,108,800 B scanned (≈5.6 MiB/s) | clean-volume-benchmark.json |
| API clean-benchmark (rda, threads 4, mask 7) | open 0.9 ms; scan 1,513,221.7 ms; save 10.8 ms; query 0.2 ms; recovery 49.2 ms (82 files); preview 4096/4096 | api-result.json |

Runner/disk profile: windows-2022, disk 4, 8 GiB VHDX 512e (**P2**), NTFS clean, 1209 live + 2138 carved files. **Honest record:** no full-profile driver-backed baseline exists — run 41082469649 was cancelled externally before publishing, and the re-dispatch 41111403030 correctly failed with the v0.6.1 driver-absent refusal (exit 4), proving the removal complete — `worklog/PERFORMANCE.md:51-69`. §3.2 + §3.1 are therefore the baseline of record.

### 3.3 Run 40227375329 — the alignbench sweep + the direct-legs gap

windows-2022, 512 MiB span per cell, two passes checksum-gated, vcpkg-release build, driver service `rda` from the rda-driver v0.2.1 release — `core_recovery/docs/PERFORMANCE-BASELINE.md:75-120`; `worklog/PERFORMANCE.md:94-101`; `worklog/CONSUMERS.md:494-503`.

| Metric | Value | Notes |
|---|---:|---|
| alignment demo, 128 KiB vs 132 KiB chunks (NO_BUFFERING, QD1, `\\.\C:`) | **240.0 vs 233.6 MiB/s (1.03×)** | the ~3.5× slow path does NOT manifest on the virtualized runner volume — recorded honestly (`PERFORMANCE-BASELINE.md:79-86`) |
| read-size × QD sweep, `\\.\C:` (MiB/s) | 64 KiB QD1 208.6 / QD8 235.5; 256 KiB 238.9/237.3; 1 MiB 224.4/241.3; 4 MiB 202.8/242.4 | plateau ~240 MiB/s, size-flat ≥256 KiB, QD>1 buys nothing (`PERFORMANCE-BASELINE.md:88-102`) |
| RDA grammar 64 KiB QD1 | **201.9 MiB/s** (8,192 reads, F11 unscale verified on every read) | production grammar `\\.\rda\<token>\<disk>\<part>` |
| RDA grammar 256 KiB QD1 | **169.3 MiB/s** (2,048 reads) | |
| RDA grammar 1 MiB QD1 | **160.5 MiB/s** (512 reads) | |
| direct `\\.\PhysicalDrive0` 256 KiB QD1 | **151.5 MiB/s** (2,048 reads) | **the one historical datapoint where direct measured SLOWER than the driver path on the same runner (−10.5% vs 169.3)** — the direct legs predate the core's current envelope implementation; explicitly flagged for re-measurement (`worklog/PERFORMANCE.md:96-101,127-130`) |

### 3.4 Run 40315255368 — the OQ-S4-3 transport verdict (the cleanest strategy comparison)

Push on `095aab5`, 2026-10-26T11:32Z, image win22, target `\\.\C:`, span 512 MiB, QD1 (the og's own dispatch), RDA service RUNNING with driver-info verified — `core_recovery/diff-to-fix/tracked-open/OQ-S4-3-chunking-relative-perf.md:137-174`; `worklog/CONSUMERS.md:519-524`.

| Pair (og buffered-sync vs core direct) | og MiB/s | og cpu_ms | core MiB/s | core cpu_ms | ratio og/core |
|---|---:|---:|---:|---:|---:|
| og 4 MiB vs core 64 KiB envelope | 197.32 | 609.4 | **203.98** | 593.8 | 0.967 |
| og 4 MiB vs core 256 KiB cache block | 197.32 | 609.4 | **241.64** | 562.5 | 0.817 |
| og 7 MiB vs core 64 KiB envelope | 196.15 | 515.6 | **203.98** | 593.8 | 0.962 |
| og 7 MiB vs core 256 KiB cache block | 196.15 | 515.6 | **241.64** | 562.5 | 0.812 |

Plus the driver-path legs on the same run: **RDA 64 KiB 194.34 / RDA 256 KiB 237.54 MiB/s** through `\\.\rda\…` (cross-target vs the `\\.\C:` direct legs — the driver leg targets the disk-class stack, the direct leg the volume stack; same runner/span/QD). `alignment_ratio_64k_over_4k` = 1.001.

### 3.5 Runs 40073093997 / 40079065019 — the scenario matrix (walls + quality context)

windows-2022, seeded 3 GiB scenario VHDXs (**P4**) — `core_recovery/bench/baseline-candidate.json` (`source_run_id: 40073093997-smoke`); `docs/PERFORMANCE-BASELINE.md:51-58`.

| Scenario (perf per mode, wall s) | quick | deep | deepcarve |
|---|---:|---:|---:|
| delete_partition (recall 42.99% / precision 19.81%) | 0.51 | 8.03 | 90.06 |
| permanent_delete (87.16% / 30.26%) | 0.52 | 7.05 | 59.87 |
| quick_format (0.0% / 0.0% — the FAT32 gap) | 0.53 | 2.01 | 41.98 |

Context rows: exFAT native delete family **99.94% recall / 99.16% precision** on the same matrix (`docs/PERFORMANCE-PROFILE.md:40`); og recycle 26/26 on the same volume (bench run **39924932944**, `docs/BENCHMARK-REPORT.md:21`); transport-relevant finding — every recovered file with `size % 512 != 0` was zero-filled "salvaged" before the sector-alignment shim landed in that run (`PERFORMANCE-BASELINE.md:53-58`); M-117 (carve-stage engine fix, tree `d7ba41f6`): 220 MB contiguous TS fixture **>600 s → 11.3 s** (`OQ-S4-3-chunking-relative-perf.md:255-264`).

### 3.6 Run 40228880804 — API cheap-op latency (the first fully green alignbench run)

windows-2022, late-bound cr_ui, 2M iterations/op — `docs/PERFORMANCE-BASELINE.md:122-141`.

| Operation | ns/op |
|---|---:|
| `cr_ui_abi_version` | **10.7** |
| `cr_ui_last_error` | **11.3** |
| `cr_ui_carve_format_count` | **12.6** (174 formats) |
| `cr_ui_carve_format_info` | **166.3** (struct fill + confidence scan) |

The run also re-confirms alignbench legs 1-3 within noise (alignment ratio 0.99; two-pass checksums recorded as `checksum_match` telemetry — `PERFORMANCE-BASELINE.md:138-141`).

### 3.7 CI-side (non-Windows) numbers from the driver repo

libFuzzer ~48.5M executions at ~800K exec/s, coverage plateau 86/265, 112-entry corpus, zero crashes; 4019-input ASan/UBSan selftest, zero findings — `rda-driver/docs/benchmarking.md:82-92`. (Dispatch-logic cost context only; not a data-path number.)

---

## §4 The guarantees the direct path must match or exceed

Labels: **C-T** = contractual, enforced by an automated test lane (rdabench/benchmark.yml assertions, unit tests, fuzz); **C-D** = contractual by design, verified against the reference decompile / structural code audit but without fault-injection automation; **INC** = incidental (reference-parity behavior, not deliberately tested).

| # | Guarantee | The driver's guarantee (citation) | Class |
|---|---|---|---|
| G1 | **Error semantics — verbatim storage-stack passthrough**: whole-IO semantics, no partial-read synthesis, failure → `Information = 0` + the lower stack's NTSTATUS unmasked (no zero-fill, no retry, no remap, no timeout); a bad sector surfaces exactly as the stack reports it | `rda_readwrite.c:266-273,352-366`; pass-through `rda_ioctl.c:236-244`; DRIVER-CONTRACT §4 | C-D (reference-decompile verified; observable error classes exercised by fuzz 50k + integration; no fault injection of media errors exists) |
| G2 | **Alignment tolerance — byte-granular, sector-agnostic forwarding**: offsets and lengths forward verbatim with no driver-side alignment, emulation or 4Kn translation; the stack enforces alignment; the client's envelope/bounce handles the rest | `rda_readwrite.c:4-15,307-315,342-347`; D2 (SectorSize never consulted on this path); contract `rda_ioctl.h:131-145` | INC (a consequence of no checks; the direct path must PRESERVE tolerance via its own 64 KiB envelope + bounce — `worklog/CONSUMERS.md:453-457`) |
| G3 | **Read-size independence / no splitting**: any length forwards as exactly one IRP — 512 B to 16 MiB observed (`rdabench.c:38,291-306`); throughput must not depend on driver-side chunking because there is none | `rda_readwrite.c:224-225`; no sizing logic anywhere in the path | C-T (rdabench info-mode `read_information_scale` + seq/rand at 64 KiB/1 MiB; the ×512 recovery is unit-frozen in `tests/unit/test_compat.c`) |
| G4 | **Information accounting**: on success `Information = Length × 512` (F11); clients recover true bytes by ÷512; on failure 0 | `rda_readwrite.c:267-273`; `rda_ioctl.h:146`; `rdabench.c:73-79` | C-T (rdabench info mode asserted ×512; the core's unscale keys on `via_rda_` — with the driver gone exactly one unscale site stops, DRIVER-CONTRACT §8.5) |
| G5 | **Synchronous per-request completion + QD1 client model**: every grammar-path request completes in-line before dispatch returns; ordering is the client's; the core's consumers are synchronous QD1 by design | `rda_readwrite.c:242-277,363-366`; only GETNEXTBLOCK ever pends (`rda_ioctl.c:295-299`); `worklog/CONSUMERS.md:347-349` | C-D (the wait-loop construction; consumers' QD1 validated by the alignbench QD sweep, run 40227375329) |
| G6 | **Concurrent-open policy**: unlimited handles, `Exclusive=FALSE`, unrestricted sharing, no dismount/exclusivity, no cross-handle state, no per-handle locks; concurrent IO from any thread count | `rda_control.c:152`; `rda_create.c:140-179`; DRIVER-CONTRACT §3.3, §10 | INC (reference parity) |
| G7 | **Handle lifecycle**: CLEANUP releases the target binding and sweeps process-owned state; CLOSE trivial; nothing client-visible survives a closed handle | `rda_create.c:140-179` (release `:150-157`, sweep `:162`); fix F9 | C-D (integration suite open/close; the driver's own churn bench, 8,532 ns) |
| G8 | **Geometry reporting fidelity**: pass-through IOCTLs return the disk stack's answer verbatim — status AND `Information` — for geometry, storage queries, layout-modifying and SMART IOCTLs alike, with transfer-method awareness | `rda_ioctl.c:186-245` (`:242` Information verbatim); proven by `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` pass-through (`rdabench.c:265-269`) | C-T (rdabench `geometry_passthrough` asserted 1.0 in the parity run; the core's bench info probe measured 8,589,934,592 B / 512/4096 through the driver, run 41042267586) |
| G9 | **Cancellation**: `CancelIo`/`CancelSynchronousIo`/handle close abort a stuck read promptly; `STATUS_CANCELLED` to the caller; cancellation propagates into the storage stack; buffer pages stay locked until the forward completes | `rda_readwrite.c:23-42,105-121,213-222,242-277` (fix F4, S1, S2) | C-D (audited in `docs/audit/2026-10-22-security-review.md`; no automated dead-drive test — the reference HUNG here, which is why the fix exists) |
| G10 | **Data fidelity**: bytes read through the transport are the bytes on the medium — no transformation anywhere in the driver (zero copies) | `rda_readwrite.c:224-225` + §1.5 | C-T (64/64 SHA-256 ranges byte-identical across drivers, run 39807929475; CI fails the job on any mismatch — `docs/benchmarking.md:32-35`) |
| G11 | **Hostile-input robustness**: every command validates before touching memory; arbitrary IOCTL codes/lengths always answer, never hang | `rdabench.c:381-419`; gates throughout `rda_ioctl.c`/`rda_session.c` | C-T (50,000 hostile IOCTLs survived, zero hangs, both drivers — run 39807929475) |
| G12 | **Session semantics**: not owed — no live core code ever issued a session command (DRIVER-CONTRACT §0 D5); the overlap guard (`STATUS_SHARING_VIOLATION`, `rda_session.c:751-754`) applies only to the unused protocol | `rda_ioctl.h:354-359`; DRIVER-CONTRACT §2.4 | INC (unused surface) |

**Performance guarantees** (the numeric targets, §3): seq 1 MiB ≥ 536.92 MiB/s (run 41042267586, P2); rand 1 MiB ≤ 1993.22 µs / ≥ 502 IOPS; open ≤ 17,019 ns mean; and the driver's own parity envelope as the floor: ≥ 532.9 MiB/s seq 64 KiB, ≥ 7,932 IOPS rand 4 KiB, ≤ 126.1 µs rand latency (run 39807929475, P1) — all on the same runner class and disk profiles, same verbs (`rrecov bench`).

---

## §5 The driver's known limits / headroom — where the direct path can BEAT it

1. **The grammar-path re-issue overhead (the second IRP)** — one `IoBuildAsynchronousFsdRequest` + `IoCallDriver` + completion + `IoFreeIrp` per IO, plus the pooled-context lifecycle, cancel arm/disarm, event wake and system-VA map (§2.1 items 5-16; `rda_readwrite.c:200-277`). Evidence it is real and measurable: the fuzz-rate delta (+52% for leaner dispatch walks — `2026-10-22-runner-comparison.md:45-48`), the documented −2.9% random-IO residual attributed to the cancel-safe machinery (`:39-44`), and the same-run 40315255368 legs (direct 203.98/241.64 vs driver 194.34/237.54 MiB/s). The driver's own expectations doc budgeted the *driver* to reach ≥90% of direct `\\.\PhysicalDriveN` throughput — the direct path IS the 100% reference (`docs/benchmarking.md:64-71`).
2. **The F11 Information scaling ×512** — a per-IO multiply in the driver plus a per-IO ÷512 unscale in every legacy client (`rda_readwrite.c:267-273`; `rdabench.c:73-79`; the core's `via_rda_` keyed unscale). Trivial arithmetic, but it vanishes entirely on the direct path (true byte counts), along with the risk of any residual unscale site.
3. **The pass-through IOCTL proxy hop** — every geometry/layout/storage-query IOCTL paid `IoGetAttachedDeviceReference` + `IoBuildDeviceIoControlRequest` + wait + deref through the driver (`rda_ioctl.c:186-245`); the direct path issues the same IOCTLs natively on its own raw handle with zero intermediate IRPs. The geometry-probe lesson transfers: the driver found per-open probing cost +10% open latency and cached it (`rda_target.c:302-445`, `worklog.md:148-151`) — the direct path should cache its geometry probe per handle likewise (it does — probe-once-at-open in the core's transport).
4. **The open-path resolution walk + SDDL** — per grammar open: `IoGetDeviceObjectPointer` + `IoGetAttachedDeviceReference` + a walk down with a 0x200-byte pool alloc + `ObQueryNameString` **per layer** (`rda_target.c:36-74,197-235`) + the geometry cache lookup + the SDDL check (measured +2.2% ≈ 175 ns of the 8,217 ns open). A direct `CreateFile("\\\\.\\.PhysicalDriveN")` is one object-manager open of the disk device. Measured driver open mean 8,217 ns (run 39807929475); core bench open through the full product stack 17,019 ns (run 41042267586) — the direct target is ≤ these.
5. **The cancel machinery** — the driver's forward-wait + cancel-routine + `CancelDone` handshake was pure overhead relative to a native OVERLAPPED read the OS cancels for free (documented as part of the −2.9% random-IO price — `2026-10-22-runner-comparison.md:39-44`; machinery at `rda_readwrite.c:105-121,242-277`).
6. **What is NOT headroom**: large-block sequential throughput was already at storage-stack parity (532.9 vs og 533.1 MiB/s — `worklog.md:157`; "the data proves the merged driver adds no measurable cost on the large IO path", `2026-10-22-runner-comparison.md:49-52`). The direct path should match, not beat, the plateau (~240 MiB/s on P3; 536.92 MiB/s on P2) — claiming large sequential wins would indicate a measurement error, not engineering.
7. **The counter-evidence that must be retired**: run 40227375329's direct `\\.\PhysicalDrive0` 256 KiB leg measured **151.5 MiB/s vs the driver's 169.3** (−10.5%) — the one historical datapoint where direct lost on the same runner. Its direct legs predate the core's current envelope implementation (`worklog/PERFORMANCE.md:96-101,127-130`). The replacement's acceptance re-measures this exact cell; if the gap persists on the runner's VHDX (OS disk contention penalizing either path differently), the measurement — not a claim — decides the diagnosis.

---

## §6 The og reference driver (eprdtdrv) — the wider reference frame

- **Measured og legs (run 39807929475, P1, side-by-side)**: seq 64 KiB 533.1 MiB/s (391.0 in the mixed R1-run column — ±5-10% runner variance); seq 1 MiB 500.6; rand 4 KiB 8,170 IOPS / 122.4 µs; open 8,042 ns; churn 8,537 ns; fuzz 553,752 IOCTL/s; 64/64 byte-identical; 50,000 hostile IOCTLs survived — `2026-10-22-runner-comparison.md:14-25`. The og is WHQL Microsoft-counter-signed (loads on stock Windows — `docs/benchmarking.md:15-17`).
- **What the og did NOT have** (the deltas are the R2 table's story): no cancellation (hung on dead drives — fix F4, `rda_readwrite.c:23-42`), no DACL (fix F2 — the +2.2% open price the og did not pay, `rda_control.c:141-153`), per-IO heap context alloc (the −10% R1 random gap), per-open geometry probe — same +10% class the rewrite had before caching (`worklog.md:146-155`). The og's numbers are therefore a *less-machine* floor: what the same storage stack yields with the thinnest possible kernel forwarder.
- **The og product's own chunking strategy (corpus decompile, measured in run 40315255368)**: the original product issued **4 MiB device chunks** (`Device_decompiled.c` ~:56545) and **7 MiB volume chunks** (~:56912) as buffered-synchronous QD1 ReadFiles — measured 197.32 / 196.15 MiB/s on P3 vs the core's 64 KiB envelope 203.98 and 256 KiB cache block 241.64 MiB/s (+18-19% at lower CPU) — `OQ-S4-3-chunking-relative-perf.md:137-174`. The og's session engine (EUDCPEPM) used a 1 MiB transfer buffer + ~2000 × 56 B IRP-context nodes — `rda-driver/docs/ground-truth-contracts.md:246-263`.
- **og quality reference**: recycle 26/26 on the same seeded volume (run 39924932944 — `docs/BENCHMARK-REPORT.md:21`); the og's own reference-driver reads returned `Information = Length << 9` identically (`ground-truth-contracts.md:128-129,141-144`).
- Frame: the direct path does not owe the og's driver anything (it is gone with the driver), but the og legs remain the honest "thinnest kernel path" and "original product strategy" references the corpus recorded — the direct path that beats §3.1's R2 column beats the og column a fortiori.

---

### Footer — sources read

Driver repo (complete, at `323e75ac` / v0.2.4): `src/driver/` — rda_driver.c, rda_control.c, rda_create.c, rda_readwrite.c, rda_ioctl.c, rda_ident.c, rda_target.c, rda_disk.c, rda_session.c (head + limits), rda_filter.c (referenced), rda_registry.c (referenced), inc/rda_p.h, inc/rda_trace.h (referenced); `src/shared/inc/` — rda_ioctl.h, rda_compat.h (referenced via DRIVER-CONTRACT); `src/client/rdacli.c` (bench-read mode `:330-401`); `src/bench/rdabench.c`; `docs/` — benchmarking.md, benchmarks/2026-10-22-runner-comparison.md, ground-truth-contracts.md (§3, §5.4-5.5), architecture/requirements/testing/win7-compat/signing (via DRIVER-CONTRACT); `worklog.md`; `scripts/`, `tests/unit/test_compat.c`, `fuzz/` (referenced). Core repo (read-only): `worklog/DRIVER-CONTRACT.md`, `worklog/PERFORMANCE.md`, `worklog/CONSUMERS.md`, `docs/PERFORMANCE-BASELINE.md`, `docs/PERFORMANCE-PROFILE.md`, `docs/BENCHMARK-REPORT.md`, `bench/baseline-candidate.json`, `diff-to-fix/tracked-open/OQ-S4-3-chunking-relative-perf.md`. Run IDs consolidated: **39807929475, 41042267586, 41082469649 (cancelled — no numbers), 41111403030 (refusal-proof, no numbers), 40227375329, 40315255368, 40073093997, 40079065019, 39924932944, 40228880804.**
