# CORE-DISK-ACCESS — the current disk access path (v0.4.1-parity tree, HEAD 22e9657)

> Written by reading agent A (Task 2-a). Every claim carries a `file:line` citation against
> the tree at `22e9657`. Companion document: [CORE-API-CURRENT.md](CORE-API-CURRENT.md).
> Historic audit baseline: [AUDIT-FALLBACKS.md](../AUDIT-FALLBACKS.md) (pinned at `08b44ac`);
> this file reflects the **current** code, noting where the audit's REMOVE items have since
> landed (FB-K01 exit codes, FB-B01 Unknown status, FB-X01 required codecs, N-041 RdaClient
> deletion which moots FB-T06/FB-T07).

>
> **POST-REMOVAL STATE (driver-only session, commit e58bde1+):** the fallback
> map sections below are the HISTORICAL pre-change census (the mandate's
> deliverable). The driver-only architecture has since REMOVED the
> direct-access data path entirely (F1-F12 are dead; `rda_available` /
> `open_harddisk_volume` / the WinPE gate / `--no-rda` are deleted;
> classified ERR_DRIVER_* refusals replaced them) - the CURRENT contract is
> [DRIVER-REQUIRED.md](DRIVER-REQUIRED.md). Sections (e) images and (b) the
> grammar path survive unchanged.
---

## (a) Overview — the layers, as text

```
rrecov CLI (src/app/rrecov.cpp)                cr_ui C ABI (src/ui/cr_ui.cpp)
   |  cmd_scan / cmd_fix / cmd_apply /            |  cr_ui_open_disk / _open_volume /
   |  cmd_bench / cmd_recover                     |  _open_volume_with_key / _open_image
   |  open_disk_view / open_volume_view /         |  open_*_impl + g_last_transport_error
   |  open_image_view                             |  (M-030 transport diagnostic)
   v                                              v
cr::win platform layer (src/win/cr_transport.cpp)
   |  open_disk / open_disk_rw / open_disk_apply_rw  <- the driver-vs-direct DECISION point
   |  open_rda_partition / open_rda_volume (grammar \\.\RDA\<token>\<d>\<p>)
   |  open_physical_drive / open_harddisk_volume      (direct \\.\PhysicalDriveN / HarddiskVolumeN)
   |  rda_available (bare \\.\RDA CreateFile probe)   (the presence probe)
   |  win_pe_environment (M-030 gate, X: system drive)
   |  RwDevice (repair/apply writes + S-010 quiesce + S-029 IOCTL chain)
   |  probe_geometry / probe_length / aligned_read_at (64 KiB envelope, BOTH transports)
   v
IStorageDevice transports
   |  DirectDevice (cr_transport.cpp:242)      raw Win32 handle, true byte counts
   |  RdaTargetDevice (cr_transport.cpp:309)   grammar handle, F11 x512 unscale
   |  FlatImageDevice / VMDK / VHD / VHDX      (src/core/storage/cr_image.cpp) - DRIVER-INDEPENDENT
   |  DecryptingDevice (in-stack XTS-AES wrap, M-033)
   v
pure-core scan pipe (src/core/orchestrator/cr_scan.cpp)
   |  PartitionSlice (bounds) -> SliceShim -> RetryingDevice (retry+bisect+Skip) -> CachedReader
   |  build_read_pipe() (cr_scan.cpp:153) constructed at run() (cr_scan.cpp:1024, 8 MiB / 256 KiB)
   v
engines (ntfs/fat/exfat/ext4/hfsplus/btrfs/zfs/refs/lost/carve) read only ReadAt(bytes)
```

The one interface everything below the CLI/ABI sees is `cr::IStorageDevice`
(include/cr/storage/cr_device.h:117-134): `read_at(offset, buffer, length)` in bytes,
`geometry()`, `absolute_base()`, `is_static_media()`. Engines never know which transport
they sit on (include/cr/storage/cr_device.h:8-14).

---

## (b) The driver (RDA) path — exact control flow

### b.1 Grammar open (the only production driver surface)

`win::open_rda_partition(service_name, disk, part, err)` — src/win/cr_transport.cpp:376-401:

1. `tok = rda::compute_pid_token(GetCurrentProcessId())` (cr_transport.cpp:377; the
   byte-identical og transform at cr_transport.cpp:358-363; contract in
   include/cr/win/cr_rda.h:99-103).
2. `CreateFileW(L"\\\\.\\%s\\%u\\%u\\%u", service, tok, disk, part)` — GENERIC_READ,
   share R|W|D (cr_transport.cpp:378-381). The **3-segment EPMNtDrv grammar only** — no
   cascade (FB-L01 comment fix; include/cr/win/cr_transport.h:26-34).
3. Geometry via `probe_geometry(h, nullptr)` (cr_transport.cpp:87-135: IOCTL_DISK_GET_LENGTH_INFO
   + IOCTL_DISK_GET_DRIVE_GEOMETRY_EX + IOCTL_STORAGE_QUERY_PROPERTY access-alignment at
   +0x10/+0x14). `total_bytes == 0` → CloseHandle + `ERR_OPEN_FAILED` (cr_transport.cpp:386-391).
4. Returns `RdaTargetDevice` (cr_transport.cpp:309-335): reads go through
   `aligned_read_at(..., rda_scaled=true, ...)`; success accepts `got == len` OR
   `got == len * 512` — the **F11 Information=Length*512 contract**
   (cr_transport.cpp:48-71, `rda::kLegacyInformationScale` at include/cr/win/cr_rda.h:56).
5. Handle self-healing: on `ERR_READ_FAILED` + `gle == ERROR_INVALID_HANDLE (6)` the device
   re-CreateFile's the same grammar path once under `heal_mu_` and retries
   (cr_transport.cpp:314-323, `reopen_handle` cr_transport.cpp:225-238; og parity
   `Device_decompiled.c:55947-55959`, documented in docs/DRIVER-UTILIZATION.md §1.4).

`win::open_rda_volume(service, volume, err)` (cr_transport.cpp:403-427) is the
`\\.\%s\%u\Volume\%u` variant with identical posture. **Neither is called anywhere by
production except through the decision sites in §(f)** (verified: callers are
`open_disk` cr_transport.cpp:510, `open_disk_view` rrecov.cpp:319, `open_disk_impl`
cr_ui.cpp:1158/1225/1342, `open_rw_device` cr_transport.cpp:876).

### b.2 The write-capable driver path

`open_rw_device(disk, prefer_rda, service, err)` (cr_transport.cpp:872-923, anonymous ns):
when `prefer_rda && service != nullptr` it CreateFileW's the 3-segment grammar with
GENERIC_READ|GENERIC_WRITE (cr_transport.cpp:874-890) and returns an `RwDevice`
(`via_rda_=true`). Reads honor F11 on both count forms; writes stage through a
sector-aligned buffer only on the direct path (RDA MDL-locks any alignment —
`write_impl_` cr_transport.cpp:795-849). `RwDevice` IS-A `apply::IApplyDevice`
(cr_transport.cpp:524) so one class serves `open_disk_rw` (cr_transport.cpp:925-928)
and `open_disk_apply_rw` (cr_transport.cpp:930-933), including the S-010 volume-quiesce
chain (LOCK→DeleteMountPoint→flush→Sleep(2000)→DISMOUNT→OFFLINE terminal; M-030;
cr_transport.cpp:537-636) and the S-029 corpus IOCTL chain 0x7C100/0x7C010/0x7C058/
0x70140 + 3×Sleep(1000) (cr_transport.cpp:688-731, refresh_disk_properties
cr_transport.cpp:927-949).

### b.3 What the driver is NOT used for (today)

- **Scans never use the driver's session/copy engine.** The 18-code session protocol
  (`kSess*` constants, `OpenSession`/`StartBackupIn`/`GetNextBlock` structs,
  include/cr/win/cr_rda.h:105-218) is wire-documentation only: "no core code issues them"
  (include/cr/win/cr_rda.h:109-114). The client class was deleted by N-041 (zero callers;
  cr_transport.cpp header comment :4-11; docs/DRIVER-UTILIZATION.md §0 reconciliation —
  the reference's recovery path is stateless EPMNtDrv grammar + ReadFile, so this MATCHES
  the og).
- **Enumeration never uses the driver** — always `\\.\PhysicalDriveN`
  (cr_enumerate.cpp:115-127 `query_disk`; FB-T04 MATCH).
- **Health never uses the driver** — direct `\\.\PhysicalDriveN` GENERIC_READ|WRITE
  CreateFileW (rrecov.cpp:1661-1666; src/win/cr_health.cpp ATA passthrough chain).
- **Images never use the driver** (§(e) below).

---

## (c) The direct-access path(s) — exact control flow

### c.1 `open_path` / `open_physical_drive` / `open_harddisk_volume`

`open_path(path, err)` (cr_transport.cpp:275-306): CreateFileW GENERIC_READ share R|W|D
→ `probe_geometry` → (exotic handles with zero IOCTL size: GetFileSizeEx last resort,
512-byte sectors — cr_transport.cpp:285-297, FB-T09 delta-candidate) → `DirectDevice`
(cr_transport.cpp:242-273): reads via `aligned_read_at(..., rda_scaled=false, ...)` with
ERROR_INVALID_HANDLE self-heal (cr_transport.cpp:252-261).

`open_physical_drive(disk)` (cr_transport.cpp:429-433) = `open_path("\\\\.\\PhysicalDrive%u")`;
`open_harddisk_volume(volume)` (cr_transport.cpp:435-439) = `open_path("\\\\.\\HarddiskVolume%u")`.

### c.2 The unified 64 KiB envelope (both transports, one code path)

`aligned_read_at(h, off, buf, len, rda_scaled, dev_size, gle_out)`
(cr_transport.cpp:177-221) computes `io::direct_envelope(off, len, dev_size)`
(include/cr/storage/cr_directalign.h:48-73): offset rounded DOWN, length rounded UP to
64 KiB multiples, clamped at the device end; already-aligned requests read straight into
the caller's buffer (zero-copy fast path); otherwise a per-thread bounce
(`e.issued_len > 1 MiB + 64 KiB` refused `ERR_BAD_PARAMETER`, cr_transport.cpp:188-190).
Rationale: the Windows stack rejects non-sector-multiple IRPs (bench run 40073093997)
and 4-KiB-multiple-but-not-64-KiB lengths take the ~3.5× slow path (GAP-S6-13);
unified on BOTH transports (cr_transport.cpp:126-174 comment block). Exhaustively unit-
proven (tests/unit/test_directalign.cpp, 9 tests incl.
`UnifiedEnvelopeExhaustivePropertyBothDevices`).

### c.3 The scan read pipe above the transport

`ScanController::run(slice, cancel, progress, stats)` (cr_scan.cpp:827) wraps the caller's
device view once (cr_scan.cpp:1024):

```
build_read_pipe(slice, 8 MiB cache, 256 KiB blocks)   cr_scan.cpp:153-194
   slice -> SliceShim (non-owning, cr_slice.h:75-91)
         -> RetryingDevice (RetryPolicy{max_attempts=3, mode=Skip, backtrack=true})
            cr_scan.cpp:168-171 — production ALWAYS opts into Skip
         -> CachedReader (8 MiB / 256 KiB; D-72 registered 8-vs-4 MiB divergence)
```

`RetryingDevice::read_at` (cr_retry.cpp:56-62) bounds-checks, then `read_isolated`
(cr_retry.cpp:64-141): retry ×3 → N-004 CatchBuffer shifted-window salvage
(cr_retry.cpp:83-99) → bisection to sector granularity (depth cap 24, cr_retry.cpp:18)
→ permanent failure recorded in `ReadErrorMap` with M-049 module attribution
(cr_retry.cpp:108-114) → **Skip mode zero-fills and returns OK** (cr_retry.cpp:124-127;
Fail mode would return `ERR_RETRY_EXHAUSTED`, but production never runs Fail —
cr_scan.cpp:170, header honesty note include/cr/storage/cr_retry.h:8-14).

`probe_family` (cr_scan.cpp:635) runs through the same pipe (cr_scan.cpp:1039) with the
orchestrator's errmod stamp (cr_scan.cpp:1040-1043). Engine dispatch in
`run_engine_pass` (cr_scan.cpp:1356; per-family calls at cr_scan.cpp:1245-1338:
`ntfs::walk_mft`, `fat::scan_fat`, `exfat::scan_exfat`, `ext4::scan_ext4`,
`hfsplus::scan_hfsplus`, `btrfs::scan_btrfs`, `zfs::scan_zfs`, `refs::scan_refs`).

---

## (d) THE FALLBACK MAP — every path that can proceed without the driver or degrade

Decision-degraded sites first (driver → direct), then in-scan degradations.

| # | Site (file:line) | Condition that triggers it | Behavior | Observable? |
|---|---|---|---|---|
| F1 | `open_disk_view` rrecov.cpp:313-341 | `rda_available(L"RDA", &gle)` false (control-device CreateFile fails; cr_transport.cpp:441-457) | prints `[transport] RDA not available (\\.\RDA CreateFile error %lu) - direct path`, opens `\\.\PhysicalDriveN` directly | stdout line; exit code unchanged; scan proceeds on direct |
| F2 | `open_disk_view` rrecov.cpp:318-328 | driver present but the grammar open fails (`ERR_OPEN_FAILED` from open_rda_partition) | prints `[transport] RDA open failed (%hs) - direct path`, falls to `open_physical_drive` | stdout line (FB-T02 / D-37 registered superset: og gates on probe only) |
| F3 | `win::open_disk` cr_transport.cpp:498-514 | `prefer_rda == false` OR `win_pe_environment()` true (X: system drive, M-030; cr_transport.cpp:466-473, predicate include/cr/win/cr_winenv.h:29-31) OR grammar open fails | `open_physical_drive(disk, err)` — **silent** in the library; the CLI/ABI layer prints | silent at this layer |
| F4 | `open_rw_device` cr_transport.cpp:872-923 | RDA RW grammar open fails (INVALID_HANDLE) or geometry 0 | direct `\\.\PhysicalDriveN` GENERIC_READ\|WRITE (cr_transport.cpp:894-923) | silent; callers print on total failure only |
| F5 | `cr_ui_open_disk` → `open_disk_impl` cr_ui.cpp:1152-1205 | PE environment OR `rda_available` false | direct `open_physical_drive`; `g_last_transport_error` = the surviving leg's error (0 when direct-only path succeeded) | `cr_ui_last_transport_error` (cr_ui.cpp:1437-1445) + `cr_ui_device_transport` = "direct" |
| F6 | `cr_ui_open_disk` cr_ui.cpp:1165-1183 | grammar open fails with driver present | direct fallback; **RDA leg's error preserved** in `g_last_transport_error` (M-030 — the pre-fix silent overwrite was the finding's claim (f)) | as above |
| F7 | `cr_ui_open_volume` → `open_volume_impl` cr_ui.cpp:1209-1258 | `rda_available` false or grammar open fails | whole-disk direct open, then `PartitionSlice::create(whole, start, len)` (cr_ui.cpp:1230-1240) | transport string "direct"; NOTE: **no PE gate on this path** (see §(f) inconsistency note) |
| F8 | `cr_ui_open_volume_with_key` → `open_volume_with_key_impl` cr_ui.cpp:1260-1363 | same as F7 | same duality + `DecryptingDevice` wrap (cr_ui.cpp:1352-1358) | same |
| F9 | CLI LDM cross-disk resolver rrecov.cpp:1046-1073 | `open_disk(pd.number, true, L"RDA")` fails | retry `open_disk(pd.number, false, L"RDA")` (rrecov.cpp:1058-1061); unresolved disks simply don't resolve | silent (FB-K04 DELTA-CANDIDATE, low) |
| F10 | ABI LDM resolver `arm_ldm_resolver` cr_ui.cpp:1679-1714 | same | same two-step open | silent |
| F11 | `cmd_bench` rrecov.cpp:1806-1820 | `--no-rda` or `rda_available` false | `open_disk(disk, use_rda, L"RDA")`; transport label `"rda"`/`"direct"` in output (rrecov.cpp:1820) | printed + `--json` `"transport"` field |
| F12 | `scan_repair_open_writer` rrecov.cpp:231-251 | `open_disk_rw(disk, true, L"RDA")` fails | retry `open_disk_rw(disk, false, nullptr)` (rrecov.cpp:234-236); else prints elevate guidance and returns null writer = "repair unavailable this scan" (orchestrator contract, include/cr/orchestrator/cr_scan.h:212-216) | printed; scan proceeds without repair |
| F13 | read-pipe degrade (M-058) cr_scan.cpp:153-194 | `CachedReader::create` fails (unreachable with run()'s 8 MiB/256 KiB constants) | pipe = **bare slice**: no cache AND no retry isolation; `rp.retry` null so no error map (cr_scan.cpp:1028-1033) | silent at runtime; pinned by test (the M-058 UAF fix contract) |
| F14 | retry Skip-mode zero-fill cr_retry.cpp:124-127 | persistent read failure after 3 attempts + backtrack + bisection | range recorded in ReadErrorMap (stats/session/events), buffer zero-filled, **returns OK** | ReadErrorMap + ErrorRange events (M-049 triple) + log seam (cr_retry.cpp:120-123); never aborts the scan |
| F15 | carve window read failure | window skipped post-retry | counted as scanned, `return OK` (FB-C01 — DELTA-CANDIDATE, still open) | only via the retry pipe's error map |
| F16 | lost-candidate engine failure cr_scan.cpp:1644+ (`scan_candidate`) | candidate open/parse fails | rc discarded, `rc = OK` — "candidate misses never abort the scan" (FB-O07/O08 MATCH) | silent by design (speculative candidates) |
| F17 | fs_hint Unknown after probe | boot probe can't classify | quick yields nothing; deep/carve still run (FB-O02 MATCH — og registers RAW regardless) | `recognition:` log lines (N-048) |
| F18 | BitLocker locked/unknown volume rrecov.cpp:411-509; cr_ui.cpp:1209-1219 | boot sector unreadable or FVE-locked, no user key | **REFUSES** (never scans blind): CLI prints unlock guidance, returns false → exit 1; ABI returns `ERR_FVE_LOCKED`/`ERR_OPEN_FAILED` (FB-B01 fixed — Unknown no longer misclassified as Clear) | printed + error code |
| F19 | geometry IOCTL soft-failures cr_transport.cpp:87-135 | alignment query fails / geometry-EX fails | physical=logical fallback; 512 + GET_LENGTH_INFO fallback (FB-T08 RETAIN-AS-SAFETY, D-04) | silent (hardening) |
| F20 | quiesce OFFLINE failure cr_transport.cpp:611-621 | IOCTL_VOLUME_OFFLINE rejected | degrades to dismount+lock posture, Warn line through the cr::log seam ("never silent") | log seam only |
| F21 | health sources unavailable rrecov.cpp:1715-1753 | SMART fails → NVMe fails | prints, continues, exit **3** when EVERY source unavailable (FB-H04 fixed: `return any_health_source ? 0 : 3` rrecov.cpp:1751) | printed + exit code |
| F22 | `list` enumeration failure rrecov.cpp:560-568 | `enumerate_disks != OK` | prints `enumeration failed`, exit **1** (FB-K01 fixed) | printed + exit code |

**Not fallbacks (explicitly cleared):** the driver/direct duality itself is verified
reference behavior (AUDIT-FALLBACKS §8; docs/DRIVER-UTILIZATION.md §1.9 MATCH);
destination-overlap refusal, consent gates, CRSF salvage are registered safety deltas.

---

## (e) Disk-image access — how images are opened; driver-independent by design

`storage::open_image(io, path_utf16, err)` — src/core/storage/cr_image.cpp:1512-1515 →
`open_image_impl` (cr_image.cpp:1455-1510):

1. **Extension dispatch** (`classify_extension`, cr_image.cpp:1417-1448, mirroring the og's
   factory DRWUI:948237-948409): `.vmdk` → `open_vmdk`, `.vhd` → `open_vhd`,
   `.vhdx` → `open_vhdx` (cr_image.cpp:1464-1470).
2. **Flat fallback bucket**: `.img`/`.dmg`/`.raw`/unknown extensions → `FlatImageDevice`
   (cr_image.cpp:152-172) over `ImageFileIo` — "exactly the og's CreateFileW +
   GetFileSizeEx path" (cr_image.cpp:1471-1473). `.edisk` is REFUSED `ERR_UNSUPPORTED`
   (registered exclusion; cr_image.cpp:1486-1490). A magic-mismatched .vmdk/.vhd/.vhdx is
   an error (ERR_BAD_MAGIC), never a silent flat fallback (include/cr/storage/cr_image.h:21-24).
3. The Win32 `ImageFileIo` is `WinImageFileIo` (src/win/cr_image_win.cpp:21-110):
   CreateFileW GENERIC_READ share R|W|D + GetFileSizeEx + OVERLAPPED reads; singleton
   `image_file_io()` (cr_image_win.cpp:113-116).
4. Image devices report `is_static_media() == true` (cr_image.cpp:155; contract
   include/cr/storage/cr_device.h:127-133) — robustness retries are skipped for static media.

**Classification: image access is a legitimate, first-class, DRIVER-INDEPENDENT path**
(the og's "Import a disk image" flow; S-030). It is not a fallback: no RDA code is ever
consulted on this path. Entry points: CLI `--image` → `open_image_view` (rrecov.cpp:277-311)
which prints format + LBA0 MBR/GPT classification; ABI `cr_ui_open_image`
(cr_ui.cpp:1123-1148, transport "image"). The CLI marks the session's image identity
(cfg.volume_image, rrecov.cpp:1006-1017) and the resume path warns on path mismatch
(rrecov.cpp:1186-1196). Tests use `tests/data/images/qemu-disk1.vhd|.vhdx|.vmdk`
(+ raw `.img` fixtures built in-memory) — see tests/unit/test_image.cpp (20 tests).

---

## (f) The decision logic — who decides driver vs direct, and where

There are **three independent deciders**, all ultimately keyed on the same primitives
(`rda_available` + optional `win_pe_environment`):

1. **CLI scan path** — `open_disk_view(n, out)` (rrecov.cpp:313-341):
   `if (win::rda_available(L"RDA", &rda_gle))` → try `open_rda_partition` → on failure
   print + fall to `open_physical_drive`; else print the CreateFile GLE + go direct.
   **Flags**: none — always RDA-first for `scan --disk` / `recover --disk`. It does NOT
   consult `win_pe_environment` (the PE gate rides `win::open_disk` only, see #2).
2. **Library whole-disk open** — `win::open_disk(disk, prefer_rda, service, err)`
   (cr_transport.cpp:498-514): `if (prefer_rda && !win_pe_environment())` → try grammar,
   fall to direct. **Flags**: `prefer_rda` (bool) + `service` name (default deployment
   `L"RDA"`). Callers: cmd_fix (rrecov.cpp:2377), cmd_apply (rrecov.cpp:2058),
   cmd_bench (rrecov.cpp:1812, `--no-rda` clears `use_rda`), both LDM resolvers
   (rrecov.cpp:1058/1060; cr_ui.cpp:1698-1700).
3. **ABI device open** — `open_disk_impl` (cr_ui.cpp:1152-1205):
   `if (!win_pe_environment() && win::rda_available(L"RDA", &gle))` → grammar →
   direct fallback with the M-030 error preservation. `open_volume_impl` /
   `open_volume_with_key_impl` (cr_ui.cpp:1209-1363) apply the same duality for the
   parent-disk open but **without** the PE gate.

Additional posture rules:

- **WinPE** (M-030): in a PE environment the corpus callers open
  `\\.\PhysicalDrive%d`/`\\.\HarddiskVolume%d` directly and never attempt the grammar
  (predicate `pe_environment_by_system_drive`, include/cr/win/cr_winenv.h:29-31;
  `win_pe_environment()` cr_transport.cpp:466-473; pinned by
  tests/unit/test_transport_logic.cpp:22-37).
- **The presence probe** `rda_available(service, last_error)` (cr_transport.cpp:441-457)
  is a bare CreateFileW on `\\.\Rda` GENERIC_READ share R|W; `*last_error` receives the
  CreateFile GLE (the "runner mystery" diagnostic). "same probe semantics as the og;
  no GET_INFO" (docs/DRIVER-UTILIZATION.md §1.9).
- **Driver release floor**: v0.2.0 of rda-driver fails every bare `\\.\RDA` open with
  error 87 (classifier ordering bug), which **silently disables the whole RDA transport
  via the probe** — functional floor >= v0.2.1, bench pin v0.2.2
  (docs/DRIVER-UTILIZATION.md:241-252).

**Refusal points** (driver absent is NEVER itself a refusal — the direct path is the
answer): the only refusals are (i) both legs fail (`cannot open disk %u (elevate?)`
rrecov.cpp:336-339 / `cannot open disk %u (RDA and direct paths failed)` cr_ui.cpp:1171-1174,
1179-1183), (ii) BitLocker locked/unknown without key material (F18), (iii) write-path
open failures with elevate guidance (rrecov.cpp:237-249, 2149-2156, 2358-2365, 2458-2465).

---

## (g) What happens today when the driver is absent — exact behavior

1. **CLI scan/recover (`--disk`)**: `open_disk_view` prints
   `[transport] RDA not available (\\.\RDA CreateFile error <GLE>) - direct path`
   (rrecov.cpp:328-331) and opens `\\.\PhysicalDriveN`. The scan runs identically
   (stages, retry pipe, sessions); exit code is unaffected by transport choice. The
   `[transport] direct path (\\.\PhysicalDriveN)` line follows (rrecov.cpp:337-340).
2. **CLI fix/apply/bench**: `win::open_disk` silently takes the direct path (no print at
   the library layer); `bench` explicitly reports `transport: direct`
   (rrecov.cpp:1820) unless `--no-rda` forced it anyway.
3. **ABI**: `cr_ui_open_disk` returns OK with `cr_ui_device_transport() == "direct"`
   (cr_ui.cpp:1185-1190) and `cr_ui_last_transport_error() == 0` on the clean direct-only
   path (nothing attempted/lost — cr_ui.cpp:1367-1372 comment), or the RDA leg's
   `ERR_OPEN_FAILED` when the grammar open itself failed first (cr_ui.cpp:1165-1183).
4. **No error, no warning, no telemetry event** exists for "driver absent" as a distinct
   condition: there is no error code for it (see CORE-API-CURRENT.md §(d) error table),
   no event type, and no session field. The only observable surfaces are the printed
   transport lines, the `device_transport` string, and the advisory
   `cr_ui_last_transport_error`. This matches the reference ("silent fallback, the
   'no-admin' degraded mode", docs/DRIVER-UTILIZATION.md §1.9, AUDIT-FALLBACKS FB-T01
   MATCH) with the documented truthfulness superset (D-24/ADR-5 transport labels).
5. **Driver-not-started / driver-unauthorized / driver-version-mismatch** are NOT
   distinguished from absent today: a stopped service, an SDDL-denied open, or a foreign
   driver all collapse into "probe CreateFile failed → direct" (the GLE is the only
   differentiator, surfaced in the CLI print and `rda_available`'s `*last_error`).
   The wire structs that COULD distinguish them (`kIoctlGetInfo` 0x6002440 /
   `kIoctlGetVersion` 0x6002448, `DriverInfo` with major/minor/flags —
   include/cr/win/cr_rda.h:46-73) are documentation-only: "no core code issues them"
   (cr_rda.h:109-114; the querying client was deleted by N-041, cr_rda.h:77-82).
