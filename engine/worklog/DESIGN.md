# DESIGN — the direct raw I/O access layer (the drop-in driver replacement)

> The design document for session-96's architectural change (T5). Governs T6-T12
> (implementation) and T13-T15 (removal). Inputs: the five Phase-0 sheets
> (`ACCESS-LAYER.md`, `CONSUMERS.md`, `DRIVER-CONTRACT.md`, `REFERENCE-ACCESS.md`,
> `DESIGN-CONSTRAINTS.md` — the gate document). Every claim below is cited to
> code (path:line at the pre-change tree) or to a sheet section. The design rule:
> **the same logical contract, one layer down** — engines, orchestrators,
> sessions, writer, health, repair, apply, ABI, and CLI keep identical behavior;
> only the mechanism that obtains raw bytes changes.

---

## §1 The shape of the change

Today's data path:

```
engines → IStorageDevice (PartitionSlice → RetryingDevice → CachedReader)
        → cr_transport (RdaTargetDevice: \\.\SVC\<tok>\<disk>\<part> grammar open)
        → the RDA kernel driver (MDL forward-IRP) → the real storage stack
```

After the change:

```
engines → IStorageDevice (PartitionSlice → RetryingDevice → CachedReader)   [unchanged]
        → cr_transport (DirectDevice: \\.\PhysicalDriveN open)
        → the Windows volume manager / disk class driver (direct)
```

The driver's data path is a synchronous forward-IRP with zero caching, zero
queueing, zero retry, verbatim error propagation, and the F11 Information scale
(DRIVER-CONTRACT §3). A direct `CreateFileW(\\.\PhysicalDriveN)` + `ReadFile`
path provides the same semantics natively minus the extra kernel hop, minus the
grammar-path CREATE dispatch, minus the F11 arithmetic. The core ALREADY
CONTAINS the complete direct machinery in production shape:
`DirectDevice` (cr_transport.cpp:258-290), `open_path` (:292-317), the unified
64 KiB envelope `aligned_read_at` (:193-219), `probe_geometry`/`probe_length`
(:86-152), the heal `reopen_handle` (:241-253), and the direct `RwDevice`
staging tail (:843-849, currently dead under `via_rda_`) — all of it exercised
by the pre-e58bde10 era and by `open_physical_drive` (the control-path consumer)
today. **The implementation is a re-caller of proven code, not a new invention**
(ACCESS-LAYER §8).

---

## §2 The layer's public surface (cr_transport.h after the change)

| Function | Today | After | Notes |
|---|---|---|---|
| `open_disk(disk, service, err)` | grammar open, part 0 | **`open_disk(disk, err)`** — `\\.\PhysicalDriveN` | `service` param dies (driver concern); all callers updated; same access/share/disposition/flags as today's grammar open (GENERIC_READ, share R\|W\|D, OPEN_EXISTING, NORMAL) — the exact flags `open_path` already uses |
| `open_disk_rw(disk, service, err)` | grammar RW open | **`open_disk_rw(disk, err)`** — direct RW open | GENERIC_READ\|WRITE, share R\|W (staging goes live) |
| `open_disk_apply_rw(disk, service, err)` | same | **`open_disk_apply_rw(disk, err)`** | S-029 IOCTL chain issues directly on the direct handle |
| `open_rda_partition` / `open_rda_volume` | grammar opens | **deleted** | `open_rda_volume` has zero callers (ACCESS-LAYER §2.2); `open_rda_partition` is inlined into `open_disk` as the direct open |
| `open_physical_drive(disk, err)` | control-path direct open | **unchanged** (now shares the one device class with the data path) | |
| `query_driver_status(svc, out)` | SCM + IOCTL probe | **deleted** | replaced by the privilege probe (§6) |
| `image_file_io()` | image I/O | **unchanged** | driver-independent by classification |
| `refresh_disk_properties(disk)` | direct IOCTL | **unchanged** | already the direct pattern (cr_transport.cpp:985-1010) |
| `volume_extent(s)`, `destination_volume_letter` | direct Win32 | **unchanged** | |
| `disconnect::gle_is_disconnect` | GLE predicate | **unchanged** | N-125 classification preserved |
| `cr_driver_status.h` (DriverStatus/DriverState/…) | classifier | **deleted** | |

The signatures that lose `service` are internal to the repo (src/win + their
callers in src/app, src/ui); no ABI surface carries them (cr_ui.h passes only
`uint32_t disk` / `char letter` — cr_ui.h:314-331 comments notwithstanding, the
ABI signatures are driver-free; the COMMENTS change).

## §3 The device class (cr_transport.cpp after the change)

**One class: `DirectDevice`** (the existing one, promoted):
- `read_at`: bounds check (`off > size() || len > size() - off` →
  `ERR_RANGE_OUT_OF_BOUNDS`) → `aligned_read_at` (the 64 KiB envelope,
  **`rda_scaled` parameter deleted** — acceptance is `got == len` exactly) →
  heal-on-`ERROR_INVALID_HANDLE` (reopen + one retry) — identical to today's
  DirectDevice (cr_transport.cpp:266-279) minus the parameter.
- `RdaTargetDevice` deleted entirely; `rda::compute_pid_token`,
  `rda::control_device_path`, and the `rda` namespace deleted;
  `include/cr/win/cr_rda.h` deleted.

**`RwDevice`** (repair/apply writer — cr_transport.cpp:596-936):
- `via_rda_` member deleted; the staged-write tail (:843-849) becomes
  unconditional (sector-aligned `_aligned_malloc(len, ssec)` staging when the
  caller's buffer is unaligned — the direct-write rule, ACCESS-LAYER §8.5);
  write acceptance `put == len` exactly (F11 tolerance deleted).
- The S-010 quiesce chain (LOCK → GUID capture → DeleteVolumeMountPoint →
  Flush → settle → DISMOUNT → OFFLINE, restore on release) is UNCHANGED — it
  already operates on direct volume handles (:633-698).
- The S-029 device-control chain (`0x7C100` DELETE_DRIVE_LAYOUT, `0x7C010`
  SET_DRIVE_LAYOUT, `0x7C058` SET_PARTITION_INFO_EX, `0x70140`
  UPDATE_PROPERTIES, `settle_refresh`) issues `DeviceIoControl` on the direct
  `\\.\PhysicalDriveN` handle — the driver merely passed these through
  (DRIVER-CONTRACT §5); `refresh_disk_properties` is the in-tree proof the
  direct handle accepts them (cr_transport.cpp:1001).

## §4 The error contract

Preserved verbatim (DESIGN-CONSTRAINTS §2.1 table): the GLE→ERR_* mapping,
N-125 disconnect precision (21/55/1110/1112/1619 → ERR_DEVICE_DISCONNECTED;
1117/23/87 stay ERR_READ_FAILED), no masking, no zero-fill at the transport,
`ERR_OPEN_FAILED` on open failure/zero-size, `ERR_VOLUME_LOCK_FAILED` clean
refusal, `ERR_NO_MEMORY` staging failure, heal-on-6 once.

**The refusal taxonomy** (DESIGN-CONSTRAINTS §2.2, adopted):
- The `ERR_DRIVER_*` family (0x500B0001-05) is **deleted** from cr_error.h,
  err_text, ABI-CONTRACT §4, and every consumer. Driver service states
  (absent/not-started/version-mismatch/errored) describe a component that no
  longer exists.
- One new append-only code, **`ERR_ELEVATION_REQUIRED`** (family 0x500C, value
  0x500C0001 — fresh, never a re-used 0x500B slot): the privilege refusal, the
  direct layer's only precondition state (elevated / not elevated — probed by
  `is_elevated()`, cr_elevate.cpp:12-21, the TokenElevation test).
- `rrecov.cpp require_driver()` becomes **`require_privilege()`**: same shape
  (FIRST in every disk-requiring verb, before any other output), same exit
  code (**4**), stderr refusal naming what requires elevation, what to do
  (relaunch elevated / let the host trigger UAC), and what works without it
  (image targets — classified (b), images never touch raw disks). The refusal
  text is the honest privilege model (mandate §6): no generic error, no fake
  result.
- The `driver` CLI verb is **deleted** (driver-status surface dies with the
  driver; `rrecov driver` describes a component that no longer exists).
  `--service` flags die everywhere they appear.
- The ABI export **`cr_ui_driver_status` is deleted** (cr_ui.h:396), the
  `CR_UI_DRIVER_STATE_*` constants and `cr_ui_driver_status_t` with it.
  `CR_UI_ABI_VERSION` bumps **3 → 4** (the removal is a breaking ABI change;
  hosts recompile against v4 — the only honest way to remove a driver surface
  without leaving a stub, which the mandate bans).
- `cr_ui_device_transport` reports **"direct"** or **"image"** (the "rda"
  value dies; cr_ui.h:439-441 comment rewritten).
- `open_disk_impl` / `open_volume_impl` / `open_volume_with_key_impl`
  (cr_ui.cpp:1424-1633): the query_driver_status precondition is replaced by
  the elevation precondition — non-elevated token → `ERR_ELEVATION_REQUIRED`
  through `ret_` + `g_last_transport_error` (identical propagation shape);
  elevated → the direct open; open failure → the target error
  (`ERR_OPEN_FAILED` + the is_elevated()-aware guidance), never a silent
  switch.

## §5 Geometry, sector size, alignment

- `probe_geometry` / `probe_length` unchanged (IOCTL_DISK_GET_LENGTH_INFO +
  DISK_GEOMETRY_EX + StorageAccessAlignment at +0x10/+0x14 — the corrected
  offsets, cr_transport.cpp:96-152). These IOCTLs answer identically on direct
  disk handles (the driver forwarded the same requests).
- N-206 CEIL `sector_count` unchanged (computed, not probed).
- The 64 KiB envelope unchanged (`io::direct_envelope`, cr_directalign.h) —
  the sector-rejection defense (run 40073093997) and the GAP-S6-13 ~3.5×
  slow-path defense. Direct reads NEED it more than the grammar path did.
- 512/512e/4Kn: the envelope is a multiple of every logical sector size;
  probe reports both sector sizes as today. 4Kn direct handles require
  sector-multiple offsets/lengths — the envelope guarantees them.
- The N-206 ceil-window tail: the device-level bounds check
  (`len > size() - off` → ERR_RANGE_OUT_OF_BOUNDS) is byte-identical on both
  paths today (RdaTargetDevice:336-337 ≡ DirectDevice:269-270) — no delta to
  preserve. The FlatImageDevice tail-clamp (cr_image.cpp:156-185) is the
  image-path behavior and stays as is.

## §6 The privilege model (mandate §6, honestly implemented)

- **The core (cr_ui.dll) never elevates itself.** A DLL cannot relaunch its
  host; the host owns the elevation UX. The core's job is to REFUSE precisely:
  `ERR_ELEVATION_REQUIRED` with guidance text, at the moment a raw-disk open
  is attempted, never at load time.
- **The CLI (rrecov.exe) refuses, never silently elevates.** A console
  process relaunching itself through ShellExecute "runas" detaches from the
  console and hides its output — a deceptive shape. Instead:
  `require_privilege()` prints the refusal (what needs elevation, how to
  relaunch elevated, what works unelevated: image targets, session/report/
  preview verbs that only read files) and exits 4 — the same refusal SHAPE
  the product has today for the driver prerequisite (rrecov.cpp:525-574), with
  the honest new reason. The user runs the elevation action themselves; the
  UAC prompt they see is the standard OS flow, triggered by their own action.
- **No manifest change on rrecov.exe**: launch-time requireAdministrator
  would elevate at process start even for `rrecov --help` / image sessions —
  exactly what mandate §6 forbids ("must not elevate at process start if the
  user is only browsing"). asInvoker + per-operation refusal is the honest
  model. (The og shipped requireAdministrator on its GUI exes —
  REFERENCE-ACCESS §4 — but the og's GUI has no image-only mode; our CLI
  does, and the mandate's rule wins.)
- **Everything not touching raw disks stays unelevated**: image scans, session
  load/save, previews, report viewing, dictionary, tree queries on loaded
  sessions — all classified (b)/(c) today and untouched.
- **Safety hardening unchanged**: no writes to source media outside the
  consent-gated repair/apply paths; destination-overlap refusal (N-013
  volume_extents arming) unchanged; BitLocker legitimate-unlock only;
  consent gates before every RAW write. Higher privilege in the token does
  not weaken any gate — they are all in core code, not in the driver.

## §7 OS support matrix (Win7 SP1 → Win11 + WinPE)

Direct raw access is the OS-native mechanism: `\\.\PhysicalDriveN` +
`IOCTL_DISK_GET_LENGTH_INFO`/`GET_DRIVE_GEOMETRY_EX`/
`IOCTL_STORAGE_QUERY_PROPERTY`/`IOCTL_DISK_UPDATE_PROPERTIES`/layout IOCTLs
all exist since Vista. Win7 SP1 through Win11: one code path, no version
gates (the `legacy_os_path()` corpus constant stays as the documented dead
branch it already is — cr_transport.cpp:809-814). WinPE: direct access needs
NOTHING installed (the driver needed regdrv-rda.reg registration —
DRIVER-RELEASE §6); PE runs as SYSTEM so `is_elevated()` is true by
construction. The Win7 PE posture gate (tools/verify_pe_um.py) is untouched —
it pins the import surface, and the direct layer imports NOTHING new (the SCM
trio OpenSCManagerW/OpenServiceW/QueryServiceStatus/CloseServiceHandle is
DELETED, a net import-surface reduction).

## §8 The consumers after the change

Every consumer keeps its exact access pattern (CONSUMERS §1-§4): the
PartitionSlice bounding, the RetryingDevice ×3/Skip/backtrack policy, the
CachedReader 8 MiB/256 KiB window, the QD1 synchronous model, the
cr_parallel worker pool — all unchanged. The ONLY behavioral deltas a
consumer can observe:

1. `cr_ui_device_transport()` returns "direct" instead of "rda".
2. The refusal code for an unelevated process is `ERR_ELEVATION_REQUIRED`
   (0x500C0001) instead of an ERR_DRIVER_* code; the CLI stderr text names
   elevation; the exit code stays 4.
3. The `driver` verb and `cr_ui_driver_status` no longer exist.

All three ARE the driver removal (mandate: "except the removal of the driver
dependency itself"). Every other output — scan results, trees, JSON schemas,
reports, sessions, recovery byte streams, health text, repair behavior — is
byte-identical.

## §9 Implementation order (T6-T12, each pushed green)

1. **T6** — cr_transport.cpp: the direct-only rewrite (delete RdaTargetDevice/
   rda namespace/query_driver_status; drop `service` params; F11 flip;
   staging live). cr_transport.h: the new surface. cr_error.h: ERR_DRIVER_*
   family out, ERR_ELEVATION_REQUIRED in. cr_rda.h deleted. cr_elevate
   promoted to the precondition probe. Both lanes compile; the Linux unit
   lane's transport tests updated (test_transport_logic: the PE-gate and
   grammar-space pins die, elevation/quiesce/disconnect pins stay);
   test_driver_status.cpp replaced by the elevation-refusal pins.
2. **T7** — edge states: device absent (CreateFile 2/3 → ERR_OPEN_FAILED +
   guidance), disconnect mid-read (N-125 family → ERR_DEVICE_DISCONNECTED,
   the heal path), mounted/unmounted volume targets, in-use volumes
   (ERR_VOLUME_LOCK_FAILED clean refusal on the quiesce path), bad sectors
   (verbatim GLE → ERR_READ_FAILED + RetryingDevice backtrack unchanged),
   unaligned reads (envelope bounce), 512e/4Kn (geometry probe), the
   non-elevated refusal. Each pinned by a Linux-lane test where pure, and by
   the Windows runner scenario lane otherwise (tests/e2e + the CI probe set).
3. **T8** — the privilege model: require_privilege() in every disk verb
   (scan/recover/tree/health-disk/fix/apply/bench, image targets exempt),
   the ABI elevation refusals, the refusal text, both-path tests.
4. **T9-T11** — wire the consumers: engines/orchestrator (they consume
   IStorageDevice — zero engine changes expected; the wiring is the
   orchestrator's transport calls), session/writer/health/repair/
   visualization, then the full suites.
5. **T12** — ABI/CLI: the v4 bump, ui_abi_check.c re-pinned, API.md/ABI
   docs updated (the doc sweep is T15).

## §10 Performance plan (correctness first, then measure)

The physics: the driver added a grammar-path CREATE dispatch + a forward-IRP
hop + the F11 scale; direct removes all three. The consumers are QD1
synchronous (CONSUMERS §3) — no queue-depth redesign is needed or wanted
(changing it would change engine timing behavior). The measurement plan
(PERFORMANCE.md §2): same bench verbs, same runner class, same 8 GiB VHDX
profile, run IDs recorded; targets per DESIGN-CONSTRAINTS §7 (seq 1 MiB
≥ 536.92 MiB/s, rand 1 MiB ≤ 1993.22 µs, open ≤ 17,019 ns, scan walls
within noise, byte-identical recovery output). The one known risk — the
historical runner direct-vs-driver gap (run 40227375329: 151.5 vs 169.3
MiB/s @ 256 KiB) — is retired by measurement: if it reproduces, the cause is
diagnosed (VHDX-on-OS-disk contention affects both paths; the gap predates
the unified envelope) and the result documented; the acceptance gate is the
FRESH §1.1 baseline on the same runner class, not the historical sweep.

## §11 What this design does NOT do (and why)

- No OVERLAPPED queue-depth pipelining (consumers are QD1 by design; changing
  it changes observable timing semantics — mandate: behavior identical).
- No FILE_FLAG_NO_BUFFERING (the envelope already guarantees alignment; the
  og's buffered reads matched the driver — run 40315255368 — and NO_BUFFERING
  would change partial-read semantics).
- No volume-GUID data-path opens (volume targets are whole-disk + slice
  today, ACCESS-LAYER §2.2; the grammar volume variant had zero callers).
- No helper process, no service, no auto-elevation (mandate §6/§7).
- No stubs: every removed surface is either deleted outright or replaced by
  its honest equivalent (ERR_ELEVATION_REQUIRED, "direct" transport label).
