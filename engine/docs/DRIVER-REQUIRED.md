# DRIVER-REQUIRED — the driver-only architecture contract

> The RDA kernel driver (`rda-driver`, service `RDA` by default) is a **hard
> prerequisite** for every data-path operation in this core. There is no
> direct-access fallback, no silent degradation, no driverless mode: the product
> is either working through the driver or explicitly refusing. This document is
> the contract: the operation classification, the refusal taxonomy, the exact
> CLI stderr and exit codes per condition, the installer contract, and the UI
> presentation contract.
>
> Evidence chain: [CORE-DISK-ACCESS.md](CORE-DISK-ACCESS.md) (the pre-change
> fallback map), [DRIVER-CONTRACT.md](DRIVER-CONTRACT.md) (the wire surface the
> status query is built on), [REFERENCE-DRIVER-BEHAVIOR.md](REFERENCE-DRIVER-BEHAVIOR.md)
> (what the reference product does — and does not distinguish),
> [PLAN-DRIVER-ONLY.md](PLAN-DRIVER-ONLY.md) (the removal plan this implements),
> [PARITY-DELTAS.md](PARITY-DELTAS.md) (the registered divergences).

---

## 1. Operation classification

Every public operation is classified (a) **driver-required** (fails fast with
its `ERR_DRIVER_*` class when the driver is unusable) or (b)
**driver-independent** (never used the driver — og evidence per row; these
proceed normally, and that is by design, not a fallback).

### (a) Driver-required — the disk data path

| Operation | Surface | What it opens |
|---|---|---|
| scan (disk or volume target) | CLI `scan --disk/--volume`; ABI `cr_ui_open_disk` / `cr_ui_open_volume` / `cr_ui_open_volume_with_key` → scan attach | RDA grammar `\\.\RDA\<token>\<disk>\<part>` (GENERIC_READ) |
| recover (disk target) | CLI `recover --disk`; ABI recovery over an attached device | same (the device is (re)opened through the data path) |
| fix | CLI `fix` → `win::open_disk` + `open_disk_rw` | grammar path read + RW |
| apply | CLI `apply` → `win::open_disk` + `open_disk_apply_rw` | grammar path read + RW (S-029 IOCTL chain rides the bound handle) |
| bench | CLI `bench` → `win::open_disk` | grammar path (`--no-rda` is REMOVED) |
| LDM cross-disk member resolution | rrecov.cpp LDM resolver; `cr_ui` `arm_ldm_resolver` | `win::open_disk` per member |
| scan-time repair writer | `scan --repair-apply` hooks | `open_disk_rw` |
| image-create (S-040; PLAN-API-EXPANSION Wave 4) | CLI `image --create --disk N --out file.img`; ABI `cr_ui_image_create` (win::open_disk read view, 64 KiB envelope stream) | reads disk content |

### (b) Driver-independent — control path and file state (og evidence)

| Operation | Evidence (why it never needed the driver) |
|---|---|
| list / disk+volume enumeration | og enumeration is GetLogicalDrives/WMI topology, NO SCM/driver APIs (getdriverinfo:20-29; FB-T04 MATCH) |
| health / SMART / NVMe / TRIM | og DiskHealth is a direct multi-transport SMART client (DiskHealth:15-23; FB-H01/H02 MATCH) |
| BitLocker status / unlock / dictionary | og uses PS `Unlock-BitLocker` / `manage-bde` + volume handles (bitlockerlib:17,44; FB-B02/B03 MATCH) |
| destination arming (volume extents, path→volume) | og compares resolved placement via volume handles (N-013) |
| post-apply layout refresh (`refresh_disk_properties`) | og issues IOCTL_DISK_UPDATE_PROPERTIES on its own direct handle (PR:46873-46880) |
| disk-image open/scan (`--image`, `cr_ui_open_image`) | image files are driver-independent BY DESIGN (S-030; CORE-DISK-ACCESS §e) |
| tree / query / preview / report | snapshot/session-backed state; no device open |
| session save/load (CRSF) | file operations |
| `driver` (the status verb itself) | it is the diagnostic that REPORTS the prerequisite |

A (b) classification is a documented og-evidenced fact, not a permission to
grow: adding a new (b) operation requires the same evidence row in this table.

## 2. The refusal taxonomy — error family `0x500B`

| Class | Code | Condition | Detection mechanism |
|---|---|---|---|
| `ERR_DRIVER_ABSENT` | `0x500B0001` | service not installed | `OpenSCManagerW` + `OpenServiceW` fail |
| `ERR_DRIVER_NOT_STARTED` | `0x500B0002` | installed, not running | `QueryServiceStatus.dwCurrentState != SERVICE_RUNNING` |
| `ERR_DRIVER_UNAUTHORIZED` | `0x500B0003` | calling context denied by the device DACL | control-device `CreateFileW` fails `ERROR_ACCESS_DENIED` (5) |
| `ERR_DRIVER_VERSION_MISMATCH` | `0x500B0004` | wire major != required (1) | `GET_INFO` (0x6002440) `major != kDriverWireMajorRequired` |
| `ERR_DRIVER_ERRORED` | `0x500B0005` | running but unusable | open fails with any other GLE (the v0.2.0 error-87 class); or `GET_INFO` fails / `struct_size != 24` / `GET_VERSION` (0x6002448) disagrees with `GET_INFO` |

The classification state machine (pure, platform-independent,
`cr/win/cr_driver_status.h`; pinned by `tests/unit/test_driver_status.cpp`):

```
1. SCM: OpenSCManager + OpenService        fail -> ABSENT
2. QueryServiceStatus != RUNNING           -> NOT_STARTED
3. CreateFile \\.\RDA  ERROR_ACCESS_DENIED -> UNAUTHORIZED
4. CreateFile \\.\RDA  any other failure   -> ERRORED (last_gle recorded)
5. GET_INFO fails / struct_size != 24      -> ERRORED
6. GET_INFO.major != 1                     -> VERSION_MISMATCH
7. GET_VERSION != (major<<32|minor)        -> ERRORED
8. else                                    -> READY
```

A **target error is not a driver refusal**: with a Ready driver, a grammar open
that fails (`ERR_OPEN_FAILED`, GLE surfaced) means that disk/partition could
not be opened — an actionable target problem, never a silent transport switch.

The core deliberately does NOT auto-start the service (the og demand-starts it,
PartitionRecovery_decompiled.c:2620-2622 — registered divergence: the installer
owns the service lifecycle; `sc start RDA` is the documented user action).

## 3. The driver-status surface

- **C++**: `win::query_driver_status(service, DriverStatus*)` +
  `win::driver_state()` / `win::driver_state_error()` (cr_transport.h /
  cr_driver_status.h). Facets: `service_present` (installed),
  `service_running` (loaded), `device_openable` (started), `authorized`,
  `info_valid` (healthy: GET_INFO + GET_VERSION consistent round-trip — the
  driver has no health/heartbeat IOCTL, DRIVER-CONTRACT §8), `major`/`minor`
  (version).
- **ABI**: `cr_ui_driver_status(cr_ui_driver_status_t*)` — additive export,
  ABI-since 2.x per the additive policy; fixed-layout C struct, zero-init safe
  (a default/ignored status classifies Absent, never a false Ready).
- **CLI**: `rrecov driver [--service NAME] [--json]` — one line per facet,
  the classified state, and remediation; exit 0 = Ready, 4 = not Ready.
- **Threading**: synchronous, stateless, thread-safe (own handles, closed
  before return). **Lifecycle**: none — a standalone query.

## 4. The CLI refusal contract

Every driver-requiring verb checks the driver FIRST — before any other output,
progress event, or partial result. **The refusal is the whole response.** Exit
code **4** (distinct from 0 ok / 1 hard failure / 2 recover-partial / 3
cancelled or health-all-unavailable).

Exact stderr per condition (`rrecov scan --disk 0` with the driver absent):

```
rrecov: driver required - the RDA driver is not installed (service "RDA" not found).
rrecov: install it from the product installer or the rda-driver release (install.ps1), then retry.
rrecov: [RDA driver not installed 0x500B0001] - no operation was attempted
```

| Condition | First stderr line | Remediation line |
|---|---|---|
| ABSENT | `the RDA driver is not installed (service "RDA" not found).` | `install it from the product installer or the rda-driver release (install.ps1), then retry.` |
| NOT_STARTED | `service "RDA" is installed but not running.` | `start it (elevated: sc start RDA) or re-run the installer, then retry.` |
| UNAUTHORIZED | `the driver denied access (its security descriptor allows administrators and SYSTEM only).` | `run elevated / from an administrator account, then retry.` |
| VERSION_MISMATCH | `driver version X.Y is incompatible with this core (requires major 1).` | `update the driver via the installer, then retry.` |
| ERRORED | `service "RDA" is running but its control device is not usable (CreateFile/GET_INFO error %lu).` | `reinstall the driver, then retry.` |

`--image` targets skip the gate (classification (b)); `list`, `health`,
`sessions`, `tree` never had it. Nothing between the gate and the operation
prints: no "starting scan", no partial tables.

## 5. Installer contract

The product installer installs the driver. There is **no "skip driver" path**
— an installation without a working driver leaves the product in a state where
every data-path operation refuses with its exact class, which is precisely the
outcome the product must never silently avoid. The reference implementation is
rda-driver's `install.ps1` (elevated): copy `rda.sys` to
`System32\drivers\<service>.sys`, `sc create <service> type= kernel start=
demand`, registry `Parameters`, `sc start`. **A failed driver step fails the
installation** with the reason (install.ps1's own exit path; the service-start
failure surfaces with the test-signing hint). WinPE media register the driver
via rda-driver's `scripts/winpe/regdrv-rda.reg` — the prerequisite is
installable everywhere the product runs.

## 6. UI contract (the presentation layer this core hands the host)

The UI never sees a partial or "working-but-degraded" data path. Its contract:

1. **On startup or before any data-path action**, call `cr_ui_driver_status()`.
   `state != READY` → present the refusal through the UI's existing error
   presentation, keyed by state (each maps 1:1 to an `ERR_DRIVER_*` code and to
   the remediation text in §4 — the host localizes/renders, the semantics come
   from the core).
2. **Every ABI open** (`cr_ui_open_disk/_volume/_volume_with_key`) returns the
   classified `ERR_DRIVER_*` code directly (and `cr_ui_last_transport_error()`
   carries the same class for hosts that surface transport diagnostics) — no
   fallback ever executes inside the ABI, so the UI cannot accidentally render
   a "successful" session that is silently driverless.
3. **No direct-access mode exists to remove from the UI**: any UI toggle that
   previously selected a transport is dead by contract. The follow-up UI-side
   change (remove such a path if one exists, wire the refusal presentation) is
   tracked as a dependency on this document and lands in the UI repository —
   out of this session's scope, tracked precisely here.

## 7. Truth guarantees (what this architecture makes impossible)

- No code path opens `\\.\PhysicalDriveN` / `\\.\HarddiskVolumeN` for the data
  path (grep-verified: direct opens survive only in the classified (b)
  control paths — enumeration, health, layout refresh, volume handles).
- No probe-boolean gates any transport decision (`rda_available` is deleted;
  the five-state classification replaced it).
- No WinPE exemption (the og's environment-gated duality is a registered
  divergence — the driver is required in PE too, and ships a PE registration
  file).
- Every refusal names its class and code; every remediation is actionable.
- **The scan cannot fake-complete on an unreadable target** (the read-truth
  gate, PARITY-DELTAS D-DRV-6): Skip-mode zero-fill still hides individual
  read failures from the engines (og-parity salvage — bad sectors are
  skipped, the map records them), but the VERDICT is truthful —
  `ScanStats.bytes_unreadable` (the error map's coverage) and
  `probe_unreadable` (family Unknown because sector 0 was unreadable, not
  because a filesystem was absent) are surfaced in the CLI summary, the JSON
  output, the ABI stats (`cr_ui_scan_stats`) and the log; and a scan whose
  recorded failure coverage spans (essentially) the whole slice with nothing
  found **REFUSES with ERR_READ_FAILED** instead of printing
  "scan complete: 0 file(s)" and exiting 0. There is no state in which the
  scan appears to have scanned data it could not read.
