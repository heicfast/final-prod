# PLAN-DRIVER-ONLY — remove the direct-access fallback; the driver becomes a hard prerequisite

> Session: driver-only mandate. Tree base: `dda4ed4` (docs layer). Evidence base:
> [CORE-DISK-ACCESS.md](CORE-DISK-ACCESS.md) (the F1–F22 fallback map), [DRIVER-CONTRACT.md](DRIVER-CONTRACT.md)
> (the wire surface the status query is built on), [REFERENCE-DRIVER-BEHAVIOR.md](REFERENCE-DRIVER-BEHAVIOR.md)
> (what the og does — and does NOT distinguish), [CORE-API-CURRENT.md](CORE-API-CURRENT.md) (the error space
> and CLI/ABI contracts being extended). Every claim below carries its evidence. Nothing here is gated,
> flagged, or deprecated — the fallback is REMOVED.

---

## 0. The decision, and its evidence base

The reference product's transport duality is **environment-gated, not failure-gated** (REFERENCE-DRIVER-BEHAVIOR §4:
`FUN_1800577e0()` returns 0 on live Windows → `\\.\EPRDTDRV\<token>\<d>\<p>`; nonzero (WinPE) → direct
`\\.\PhysicalDrive%d`; **no site retries the other transport on open failure** — a failed driver open becomes a
silent NULL device). Our core ADDED a failure-gated fallback on top (probe-fails → direct; grammar-fails → direct;
RW-fails → direct) at three independent deciders (CORE-DISK-ACCESS §f). That superset — plus the product decision
that this core ships driver-gated capabilities — is what gets removed. The removal is a **deliberate, registered
divergence** from reference behavior (PARITY-DELTAS D-class entries; rationale: the product does not market a
non-driver path, and silent degradation was the direct cause of the "product installed, nothing works" failure
class the user experienced).

The refusal contract targets the semantic taxonomy mandated by the session brief: driver-absent,
driver-not-started, driver-version-mismatch, driver-unauthorized, driver-errored — each a distinct error class.
The og distinguishes NONE of these (REFERENCE-DRIVER-BEHAVIOR §2: service-start result discarded, NULL device
silent, no version negotiation); our taxonomy is a **truthfulness superset**, registered as a divergence, not
as parity.

## 1. Operation classification — driver-required vs. driver-independent

Every public operation is classified (a) requires-the-driver/fails-fast or (b) never-needed-the-driver
(og evidence per row). This table becomes the spine of docs/DRIVER-REQUIRED.md.

| Operation | Surface(s) | Classification | Evidence (why (b) never needed the driver) |
|---|---|---|---|
| scan (disk) | CLI `scan --disk`, `cr_ui_scan_attach_device` → `cr_ui_open_disk` | **(a) driver-required** | CORE-DISK-ACCESS §b: grammar path is the recovery read path |
| recover (disk) | CLI `recover`, `cr_ui_open_disk`/`_open_volume` | **(a)** | same |
| fix | CLI `fix` → `win::open_disk_rw` | **(a)** | repair-write data path |
| apply | CLI `apply` → `win::open_disk_apply_rw` | **(a)** | table-write data path |
| bench | CLI `bench` | **(a)** | disk-I/O measurement path; `--no-rda` is REMOVED |
| LDM member resolve | rrecov.cpp:1046-1073; cr_ui.cpp:1679-1714 | **(a)** | resolves SCAN TARGET disks |
| image-create (new, PLAN-API-EXPANSION) | future `image` verb | **(a)** | reads disk content |
| list / enumerate disks+volumes | CLI `list`; `cr_ui_enumerate_*` | **(b)** | FB-T04 MATCH: og enumeration is GetLogicalDrives/WMI topology (getdriverinfo:20-29) — "enumeration never needs the driver" |
| health / SMART / NVMe / TRIM | CLI `health`; `cr_ui_health_read` | **(b)** | FB-H01/H02 MATCH: og DiskHealth is direct multi-transport SMART (DiskHealth:15-23) |
| BitLocker status/unlock | CLI unlock chain; `cr_ui_unlock_volume_with_dictionary` | **(b)** | FB-B02/B03 MATCH: og uses PS/manage-bde + volume handles (bitlockerlib:17,44) |
| destination arming / extents | `volume_extent(s)`, `destination_volume_letter` | **(b)** | og compares resolved placement via volume handles (N-013) |
| post-apply layout refresh | `refresh_disk_properties` | **(b)** | og PR:46873-46880 does IOCTL_DISK_UPDATE_PROPERTIES on its own direct handle |
| image open/scan | CLI `--image`; `cr_ui_open_image` | **(b)** | S-030: image files are driver-independent BY DESIGN (CORE-DISK-ACCESS §e) |
| tree / query / preview / report | snapshot/session-backed ops | **(b)** | operate on in-memory or persisted state; no device open |
| session save/load/list | CRSF file I/O | **(b)** | file operations |
| driver status (new) | CLI `driver`; `cr_ui_driver_status` | **diagnostic** | the surface that REPORTS the prerequisite; never refuses on driver state |

## 2. The refusal taxonomy — error family `0x500B` (cr_error.h)

New family, following the registered core-internal value-space convention (D-68; cr_error.h:8-13):

| Code | Value | Condition (detection mechanism, from DRIVER-CONTRACT.md) |
|---|---|---|
| `ERR_DRIVER_ABSENT` | `0x500B0001` | `OpenServiceW(service)` fails (service not installed) |
| `ERR_DRIVER_NOT_STARTED` | `0x500B0002` | service exists, `QueryServiceStatus.dwCurrentState != SERVICE_RUNNING` |
| `ERR_DRIVER_UNAUTHORIZED` | `0x500B0003` | control-device CreateFile fails with `ERROR_ACCESS_DENIED` (DACL SDDL `D:P(A;;GA;;;BA)(A;;GA;;;SY)` — DRIVER-CONTRACT §7) |
| `ERR_DRIVER_VERSION_MISMATCH` | `0x500B0004` | `GET_INFO` (0x60002440) reports `major != kVersionMajor` (1) — cr_rda.h:221 |
| `ERR_DRIVER_ERRORED` | `0x500B0005` | service RUNNING + control-device open fails with any other GLE (the v0.2.0 error-87 class), or `GET_INFO` fails / `struct_size != 24` / `GET_VERSION` (0x60002448) disagrees with `GET_INFO` (FB-T06's contract: GET_INFO failure on an opened device = wrong/ancient driver) |

`err_text()` gains one line per code (cr_error.cpp). Every code is user-facing: CLI stderr, ABI return,
`cr_ui_last_error` string.

**Ordering contract (the classification state machine, fail-fast, first-match):**

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

The og demand-starts its service on enumeration (PartitionRecovery_decompiled.c:2620-2622); we deliberately
do NOT auto-start — the core refuses and names the remediation (divergence registered: the installer owns
service lifecycle; a demand-start `sc start RDA` is the documented user action).

## 3. The driver-status surface

**Shape decision (evidence-driven):** the og exposes NO driver-status surface to its UI (REFERENCE-API-GAP
R-84 + corpus-silence list; REFERENCE-DRIVER-BEHAVIOR §3). The mandate requires a per-facet surface
(present / loaded / started / version / authorized / healthy). Resolution: implement the mandated facets as a
**single synchronous query call returning a structured status** (one call, per-facet fields), because that is
the minimum shape that answers every mandated facet, and richer shapes (subscriptions, per-facet calls) would
be invention beyond both the mandate's text and the corpus. Registered as a product-scope divergence
(PARITY-DELTAS), NOT as parity.

**C++ surface (`include/cr/win/cr_transport.h`):**

```cpp
struct DriverStatus {          /* every facet a distinct, queryable field */
    bool service_present;      /* installed            (SCM)             */
    bool service_running;      /* loaded               (SCM)             */
    bool device_openable;      /* started              (CreateFile)      */
    bool authorized;           /* authorized           (not ERROR_ACCESS_DENIED) */
    bool info_valid;           /* healthy              (GET_INFO+GET_VERSION round-trip) */
    u32  major, minor;         /* version              (GET_INFO, cross-checked) */
    u32  flags, max_disks, ioctl_base;   /* capability echo (DriverInfo) */
    u32  last_gle;             /* diagnostic (CreateFile/IOCTL GLE)      */
};
enum class DriverState { Absent, NotStarted, Unauthorized, VersionMismatch, Errored, Ready };
DriverState driver_state(const DriverStatus&);        /* PURE — Linux-lane testable */
u32  query_driver_status(const wchar_t* service, DriverStatus* out); /* Win32; returns ERR_DRIVER_* */
```

Facet ↔ mechanism mapping (honest, from DRIVER-CONTRACT §8 — "open succeeds (loaded) + GET_INFO
(capabilities/flags)" is the driver's entire surface; there is no health/heartbeat IOCTL):
present = SCM; loaded = SCM state; started = control-device open; version = GET_INFO/GET_VERSION;
authorized = open GLE != ACCESS_DENIED; healthy = GET_INFO+GET_VERSION consistent round-trip.

**The probe upgrade that makes this real:** `rda_available()` (cr_transport.cpp:441-453) today is a bare
CreateFile that collapses absent/not-started/unauthorized/errored into one boolean
(CORE-DISK-ACCESS §g.5). It is superseded by `query_driver_status` and REMOVED (no caller keeps a boolean
view of a five-state world). The GET_INFO/GET_VERSION structs in cr_rda.h:46-73/220-222 stop being
documentation-only: the status query issues both IOCTLs on the global control-device handle (dispatch order
per DRIVER-CONTRACT §3: session codes → bound-handle → global-handle switch — GET_INFO/GET_VERSION are
global-handle codes).

**ABI surface (`include/cr/cr_ui.h` + `src/ui/cr_ui_exports.def`):**

```c
typedef struct cr_ui_driver_status_t {   /* fixed-layout, C11, zero-init safe */
    u32 state;            /* CR_UI_DRIVER_STATE_* enum value */
    u32 service_present, service_running, device_openable, authorized, info_valid; /* 0/1 */
    u32 major, minor, flags, max_disks, ioctl_base, last_gle;
} cr_ui_driver_status_t;
u32 cr_ui_driver_status(cr_ui_driver_status_t* out);   /* 0 = query itself succeeded */
```

ABI version bumps v2 → v3 (additive exports; existing signatures unchanged — per docs/ABI-CONTRACT.md's
additive policy, verified at implementation time). `tests/ui_abi_check.c` grows the new symbols.

**CLI surface:** new verb `driver` (single-word, matching the `health` grammar):

```
rrecov driver [--json] [--service NAME]
  text: one line per facet + final state + remediation when not Ready
  json: the full DriverStatus
  exit: 0 = Ready; 4 = any non-Ready state
```

## 4. The removal diff — every code path, file by file

Sources: CORE-DISK-ACCESS §d fallback map (F1–F12 are the removal set; F13–F22 are in-scan degradations
that are either registered safety deltas or already-fixed truth bugs — NOT transport fallbacks; only the
driver→direct legs die here).

| # | Site | Change |
|---|---|---|
| R1 | `open_disk_view` rrecov.cpp:313-341 | Probe→grammar→**direct fallback** becomes: `require_driver()` → refusal (stderr class + exit 4) → `open_rda_partition` only. Grammar-open failure with a Ready driver stays `ERR_OPEN_FAILED` (a target error, not a driver refusal) with the GLE printed. |
| R2 | `win::open_disk` cr_transport.cpp:498-514 | `prefer_rda && !win_pe_environment()` + direct fallback REMOVED. Signature loses `prefer_rda` (callers updated: rrecov.cpp:2377/2058/1812, resolvers). Body = grammar open only. |
| R3 | `open_rw_device` cr_transport.cpp:872-923 | Direct `\\.\PhysicalDriveN` RW tail REMOVED. RDA grammar RW only. |
| R4 | `open_disk_impl` cr_ui.cpp:1152-1205 | PE gate + probe + direct fallback REMOVED → classified refusal codes returned; `g_last_transport_error` carries the class (the M-030 channel becomes the refusal diagnostic). |
| R5 | `open_volume_impl` cr_ui.cpp:1209-1258 | Same; the whole-disk direct open + `PartitionSlice::create` fallback leg REMOVED. |
| R6 | `open_volume_with_key_impl` cr_ui.cpp:1260-1363 | Same (the `DecryptingDevice` wrap survives on the RDA-opened parent). |
| R7 | CLI LDM resolver rrecov.cpp:1046-1073 | The `open_disk(pd, false, …)` direct retry (1058-1061) REMOVED — resolver resolves via RDA or the member stays unresolved (printed, as today). |
| R8 | ABI LDM resolver `arm_ldm_resolver` cr_ui.cpp:1679-1714 | Same two-step removal. |
| R9 | `scan_repair_open_writer` rrecov.cpp:231-251 | Direct-RW retry (234-236) REMOVED; with a Ready driver an RW-open failure prints "repair unavailable this scan" (existing orchestrator contract cr_scan.h:212-216). |
| R10 | `cmd_bench` rrecov.cpp:1806-1820 | `--no-rda` flag REMOVED; bench is driver-required; transport label always the grammar path. |
| R11 | `rda_available` cr_transport.cpp:441-453 | REMOVED — superseded by `query_driver_status` (a boolean view of a five-state world is the silent-degradation primitive itself). All callers migrated. |
| R12 | PE gate | `win_pe_environment()` call-sites in the DATA path removed (R2/R4). The probe function itself survives only if non-data-path callers exist (implementation-time grep decides keep-for-logging vs delete); the M-030 doc pins update. **In WinPE the driver is now REQUIRED** — rda-driver ships `scripts/winpe/regdrv-rda.reg` (DRIVER-RELEASE §6) so the prerequisite is installable there. Divergence from og PE-direct registered in PARITY-DELTAS. |
| R13 | `open_physical_drive` / `open_harddisk_volume` / `open_path` / `DirectDevice` cr_transport.cpp:242-306/429-439 | Callers after R1-R10: enumeration/health/refresh do their own inline CreateFileW (cr_enumerate.cpp:115-127, cr_health.cpp, refresh_disk_properties:935-938) — these classes become dead on the data path. Implementation-time grep: if zero callers remain, DELETE them (mandate: removed, not kept-dormant). The inline control-path opens stay, classified (b), with header contract comments added. |
| R14 | `cr_ui_device_transport` | The "direct" enum value dies with the fallback; "rda" and "image" remain. Header contract text updated. |

**What is NOT removed (and why — each is classified, not overlooked):** the in-scan truth surfaces F13-F22
(retry Skip-mode zero-fill with ReadErrorMap, quiesce OFFLINE Warn seam, health exit-3, etc.) are
registered safety deltas or fixed truth bugs (AUDIT-FALLBACKS verdicts RETAIN-AS-SAFETY/MATCH), not
driver-fallback legs. FB-C01 (skipped carve windows counted as scanned) remains the one open truth defect —
fixed under this plan's quality bar if the carve stats seam allows it without scope creep; otherwise it stays
registered open (it is not a transport fallback).

## 5. The CLI refusal contract (docs/DRIVER-REQUIRED.md gets the full table)

Every driver-requiring verb (`scan --disk`, `recover`, `fix`, `apply`, `bench`, plus future driver-dependent
ops) executes `require_driver()` BEFORE any other output: no "starting scan", no progress events, no partial
tables. The refusal is the whole response:

```
rrecov: driver required - <specific message> [ERR_DRIVER_xxxxxxxx 0x500B00xx]
```

Per-condition stderr text (exact strings, shipped in DRIVER-REQUIRED.md with examples):

- ABSENT: `RDA driver is not installed (service "RDA" not found). Install it from the product installer or the rda-driver release (install.ps1), then retry.`
- NOT_STARTED: `RDA driver service "RDA" is installed but not running. Start it (elevated: sc start RDA) or re-run the installer, then retry.`
- UNAUTHORIZED: `RDA driver denied access (security descriptor). Run elevated / from an administrator account, then retry.`
- VERSION_MISMATCH: `RDA driver version X.Y is incompatible with this core (requires major 1). Update the driver via the installer, then retry.`
- ERRORED: `RDA driver service is running but its control device is not usable (CreateFile/GET_INFO error %lu). Reinstall the driver, then retry.`

Exit code: **4** (new — distinct from 0 ok / 1 hard failure / 2 recover-partial / 3 scan-cancelled /
health-all-unavailable). `driver` verb: 0 Ready / 4 not-Ready. `--image` scans and (b)-class verbs are
unaffected (no driver check — that is their documented classification).

## 6. Installer contract + UI coordination (documented, one change-set)

- docs/DRIVER-REQUIRED.md carries the installer contract: the product installer installs the driver
  (rda-driver `install.ps1` is the reference implementation — elevated, `sc create type= kernel start= demand`,
  service start, failure = installation failure with the reason). No "skip driver" path exists; a failed driver
  step fails the installation.
- The UI coordination contract: cr_ui refusal codes (`ERR_DRIVER_*` returned by `cr_ui_open_disk/_volume/
  _volume_with_key`) + `cr_ui_driver_status()` are the presentation contract. The UI surfaces each class through
  its existing error presentation; the UI-side change is tracked as a precise dependency
  (docs/DRIVER-REQUIRED.md §"UI contract") — not implemented this session (UI repo out of scope).
- The ABI open functions' contract text in cr_ui.h is rewritten in the same commit as the removal (contract,
  implementation, and docs move together).

## 7. Test plan (no mocks on the truth path — Linux lane pins logic, Windows runner proves behavior)

1. **Unit (Linux lane, win-stub):** `tests/unit/test_driver_status.cpp` — the PURE `driver_state()` classifier:
   every state, GLE routing (87 vs 5 vs 2), GET_INFO struct_size/major checks, GET_VERSION disagreement,
   v0.2.0-error-87 regression pin (RUNNING + open GLE 87 → Errored). Stub additions for SCM APIs where the
   win layer compiles against tests/win-stub.
2. **Unit updates:** `test_transport_logic.cpp` PE-gate pins updated to the new posture (predicate survives as
   environment probe if kept; the DATA-path gate is gone — the pins test what remains).
3. **Contract (Windows runner, real driver, real disk — no mock):** the staged-scenario matrix
   (PLAN-CI-REBUILD §4) gains the **driver-refusal scenario**: Phase 1 (driver NOT installed): `rrecov driver`
   exits 4 with ABSENT; `rrecov scan --disk 0` exits 4 with the exact stderr class and NO scan output.
   Phase 2 (driver installed + started): `rrecov driver` exits 0 Ready with all facets; scan proceeds on the
   grammar path. Phase 3 (service stopped): NOT_STARTED refusal. Evidence artifact: captured stdout/stderr +
   exit codes per phase.
4. **ABI contract:** `test_cr_ui.cpp` grows `cr_ui_driver_status` plumbing tests (Linux-lane-safe: the query's
   Win32 seam returns a stubbed status on non-Windows builds — the CLASSIFIER is the tested logic; the real
   query is proven on the Windows runner).

## 8. Doc updates riding the same change-set

- ADD `docs/DRIVER-REQUIRED.md` (taxonomy, per-condition stderr/exit codes, the §1 classification table,
  installer contract, UI contract, remediation strings).
- UPDATE `docs/architecture.md`, `docs/ground-truth-contracts.md`, `README.md`, `docs/DRIVER-UTILIZATION.md`,
  `docs/CORE-DISK-ACCESS.md` (post-removal state), `docs/CORE-API-CURRENT.md` (new codes/verb/export),
  `docs/API.md` (the `rrecov info` drift fix rides along), root `AUDIT-FALLBACKS.md` (verdict column updated:
  the driver→direct legs move from MATCH-superset to REMOVED-divergence with D-numbers).
- UPDATE `docs/PARITY-DELTAS.md`: three new divergence entries — (1) failure-gated fallback removed (og has
  none; our superset deleted), (2) WinPE gate removed / driver required in PE (og: PE-direct by design), (3)
  driver-status surface + refusal taxonomy added (og distinguishes nothing; product-scope truthfulness).

## 9. Acceptance criteria (all must be green before the next phase)

1. `rg "\\\\\\.\\PhysicalDrive" src/win src/app src/ui` → hits ONLY in classified-(b) control paths
   (enumerate/health/refresh/volume-handles) — zero data-path hits.
2. No `prefer_rda` parameter, no `--no-rda` flag, no `rda_available` symbol, no PE gate on any data path.
3. `ERR_DRIVER_*` family + err_text entries + ABI `cr_ui_driver_status` + CLI `driver` verb shipped.
4. Every driver-requiring verb refuses pre-output with class + exit 4 (Windows scenario evidence).
5. Linux-lane classifier tests green; full unit suite green; MSVC /W4 /WX build green.
6. DRIVER-REQUIRED.md + the §8 doc set consistent (grep for "fallback"/"direct path" returns only
   historical/deliberate references).
