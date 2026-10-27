# API Reference — core_recovery

This document is the single entry point for every programming surface the
core exposes, at the current tree. Surfaces are listed from the outside in:
the `rrecov` CLI, the `cr_ui` C ABI (the host-binding surface), and the
C++20 core API (the engine layer hosts embed directly when they do not go
through the shim).

Companion documents:

- [PARAMETERS.md](PARAMETERS.md) — every tunable (CLI flags, scan config,
  caps, budgets) with defaults and bounds.
- [CONNECTION-GUIDE.md](CONNECTION-GUIDE.md) — how a host binds, links,
  drives the lifecycle, and connects devices (driver vs direct vs images).
- [FORMATS.md](FORMATS.md) — what is parsed/written (filesystems, carve
  formats, session container, report schema).
- [ABI-CONTRACT.md](ABI-CONTRACT.md) — the wire-level ABI rules
  (versioning, stability classes, export inventory).
- [error-fix-registry.md](error-fix-registry.md) + `include/cr/cr_error.h`
  — the error model.

---

## 1. The `rrecov` CLI

```
rrecov list
rrecov scan    --disk N | --volume C: [--whole-disk] [--quick-only] [--no-carve]
               [--threads N] [--out file.crsf] [--resume file.crsf] [--json]
               [--log file] [--csv] [--filter s] [--deleted-only]
               [--bitlocker-fvek hex] [--bitlocker-dict [file]]
               [--user-signs file] [--repair [plan]] [--repair-apply]
               [--check]
rrecov tree    --in file.crsf [--filter substr] [--deleted-only] [--csv] [--json]
rrecov recover --in file.crsf --disk N --to DIR [--deleted-only] [--filter s]
               [--report file.json]
rrecov health  --disk N
rrecov fix     --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
rrecov bench   --disk N [--mode seq|rand|open|read|info] [--mb N]
               [--block-kb N] [--iterations N] [--seed N] [--json]
rrecov driver  [--service NAME] [--json]
```

(The historical `rrecov info` line above was doc drift - no such verb ever
existed; `bench --mode info` is the geometry/transport report. `--no-rda`
is REMOVED with the driver-only architecture: bench requires the driver.)
The `driver` verb is the driver-status surface (docs/DRIVER-REQUIRED.md):
present/loaded/started/authorized/healthy/version facets; exit 0 = ready,
4 = the prerequisite is not met.

**Exit codes**

| Code | Meaning |
|---|---|
| 0 | success |
| 1 | failure (scan failure, enumeration failure, …) |
| 2 | recover: partial — item errors or hash mismatch in the write set |
| 3 | scan cancelled (session saved when --out was given) / health all-unavailable |
| 4 | the DRIVER prerequisite is not met — scan/recover/fix/apply/bench refuse before any other output with the ERR_DRIVER_* class on stderr (docs/DRIVER-REQUIRED.md) |
| 3 | cancelled (scan; the session file is still saved when `--out` was given) / health: every source unavailable |

Every command logs stage timings, per-stage counts and error maps
(`--log`). `scan --json` emits the run summary machine-readably;
`tree --json` the result tree; `--csv` the flat file list.

**Consent-bearing flags** (the safety architecture): `--repair-apply`,
`--repair-undo`, `--undo-apply`, `--bitlocker-dict`, `--bitlocker-fvek`,
`--apply`. Each one is an explicit operator action; nothing write-side or
unlock-side ever fires implicitly. See [CONNECTION-GUIDE.md](CONNECTION-GUIDE.md#consent).

---

## 2. The `cr_ui` C ABI (`cr_ui.dll` / `libcr_ui.so`)

Version: `cr_ui_abi_version()` returns `CR_UI_ABI_VERSION` (**2**).
Call it first and refuse to bind on mismatch. The surface is
append-only-flat-C: 78 exported functions in the current header, all
`cr_ui_*`, opaque handles, C callbacks, no exceptions across the boundary,
thread-safety class documented per group ([ABI-CONTRACT.md](ABI-CONTRACT.md)).

### 2.1 Lifecycle and versioning

| Export | Purpose |
|---|---|
| `cr_ui_abi_version` | the ABI version gate (call first) |
| `cr_ui_string_free` | free every returned string |

### 2.2 Device layer

| Export | Purpose |
|---|---|
| `cr_ui_enumerate_disks` / `cr_ui_enumerate_volumes` | the device census (PhysicalDrive namespace, ≤ 200 disks, D-85 scope) |
| `cr_ui_open_disk` / `cr_ui_open_volume` / `cr_ui_open_image` | open by number / letter / image file (flat, VMDK, VHD, VHDX) |
| `cr_ui_open_volume_with_key` | open a BitLocker volume with a user-supplied FVEK (in-stack XTS-AES decrypt, M-033) |
| `cr_ui_device_geometry` / `cr_ui_device_transport` / `cr_ui_device_close` | geometry (4Kn-aware), transport name (`rda`/`direct`), close |
| `cr_ui_last_transport_error` | the surviving transport leg's error after an RDA→direct fallback (M-030; the silent-fallback closure) |

### 2.3 Scanning

| Export | Purpose |
|---|---|
| `cr_ui_scan_create` / `cr_ui_scan_close` | scan handle lifecycle |
| `cr_ui_scan_attach_device` | bind an open device handle |
| `cr_ui_scan_run` / `cr_ui_scan_cancel` / `cr_ui_scan_pause` / `cr_ui_scan_resume` | run control (pause is freeze-and-resume with pause-compensated elapsed) |
| `cr_ui_scan_busy` / `cr_ui_scan_stopping` | synchronous state queries (the og's `sigIsBusy`/`sigIsStopping` reference-out answers, N-075) |
| `cr_ui_scan_set_event_fn` | subscribe to events (see §2.6) |
| `cr_ui_scan_telemetry` / `cr_ui_scan_get_stats` | the pull surfaces (any thread, any time) |
| `cr_ui_scan_snapshot` | a mid-scan immutable result snapshot (G-02) |
| `cr_ui_scan_save_session` / `cr_ui_scan_save_cancel` / `cr_ui_scan_checkpoint_count` | durable sessions (.crsf) |
| `cr_ui_scan_verify_identity` | device-identity revalidation for resume (M-037-class) |
| `cr_ui_scan_assistant` | the post-scan grading statistics (S-023) |
| `cr_ui_scan_repair_save_undo` | persist the repair undo log |
| `cr_ui_disk_map` / `cr_ui_disk_map_free` | the visual disk map (used/free/bad cells; engine-bitmap free overlay, S-009) |

### 2.4 Sessions, snapshots, query, preview

| Export | Purpose |
|---|---|
| `cr_ui_session_load` / `cr_ui_session_close` / `cr_ui_session_resume` | session container lifecycle |
| `cr_ui_session_get_info` / `cr_ui_session_stats` / `cr_ui_session_regrade` / `cr_ui_session_snapshot` | summary, per-root stats, re-grading, snapshotting |
| `cr_ui_snapshot_*` (7) | the read-only result view: DFS walk, node detail, partial window |
| `cr_ui_query_run` / `cr_ui_query_rows_free` / `cr_ui_query_apply_buckets` | the server-side query (filter/sort/page/bucket IDs, M-029) |
| `cr_ui_preview_open_session` / `cr_ui_preview_open_snapshot` / `cr_ui_preview_read` / `cr_ui_preview_size` / `cr_ui_preview_metadata` / `cr_ui_preview_close` / `cr_ui_preview_unreadable` | budgeted content preview (caps honest, M-028) |

### 2.5 Recovery, repair, apply, unlock

| Export | Purpose |
|---|---|
| `cr_ui_recovery_create` / `cr_ui_recovery_close` | the writer session |
| `cr_ui_recovery_set_destination` / `cr_ui_recovery_enqueue_node` / `cr_ui_recovery_enqueue_subtree` / `cr_ui_recovery_enqueue_snapshot` (+ `_subtree_snapshot` variants) / `cr_ui_recovery_queue_count` / `cr_ui_recovery_process` / `cr_ui_recovery_get_stats` / `cr_ui_recovery_report_json` | the safe recovery writer (queue, collision policy, atomic writes, hash verify) |
| `cr_ui_health_read` | SMART/NVMe health with the 4-status grading (Unknown/Good/Caution/Bad) |
| `cr_ui_apply_lost_partition` / `cr_ui_apply_undo_last` / `cr_ui_apply_save_undo` | lost-partition table apply + undo (consent-gated, D-98a..d safety registers) |
| `cr_ui_unlock_volume_with_dictionary` | the consented BitLocker password-dictionary aid (N-064, D-80 canonical: cr::dict's 228-entry shipped table first, then user candidates) |
| `cr_ui_register_user_signs` / `cr_ui_user_sign_count` / `cr_ui_user_sign_info` | the user-extensible signature database (M-048) |
| `cr_ui_carve_format_count` / `cr_ui_carve_format_info` | carve-format enumeration (N-033) |

### 2.6 Events and threading contract

Subscribe with `cr_ui_scan_set_event_fn`. The event stream carries
`ScanProgress` (coalesced, monotonic, ≤ ~1000/scan), `FilesFound`,
`ErrorRange` (with module/operation/attempt attribution, M-049),
check-phase reports (`FsCheck`, wire 10), repair outcomes, candidate
discoveries, checkpoints, and pause/resume boundaries.

**Threading contract** (documented at `include/cr/orchestrator/cr_scan.h`
:110-113): events may fire from **engine worker threads** — the same
threads the progress callbacks use — and the core does **not** serialize
deliveries. A sink must be thread-safe and must not block. State queries
(`cr_ui_scan_busy`, telemetry, snapshots) are safe from any thread at any
time.

---

## 3. The C++ core API

Headers under `include/cr/`. The layering rule: pure core (no OS headers)
← engines ← orchestration; the platform layer implements the
`IStorageDevice` seam.

### 3.1 Storage seam

```cpp
struct cr::IStorageDevice {          // include/cr/storage/cr_device.h
    virtual u32 read_at(u64 off, void* buf, u32 len) = 0;
    virtual const Geometry& geometry() const = 0;   // logical/physical sector, sizes
};
```

Everything above this seam (engines, carving, check, repair, sessions,
writer) reads through it. Hosts provide it directly for exotic transports;
the shipped implementations are the RDA grammar + direct PhysicalDrive/
HarddiskVolume handles (Windows), `MemDevice` (tests), and the image
stack (flat/VMDK/VHD/VHDX, any platform). The retry pipe wraps a device
with bounded retry + bisection and produces the `ReadErrorMap` that
surfaces through stats/events/ABI.

### 3.2 Orchestration

```cpp
cr::orchestrator::ScanController ctrl{tree, cfg};   // include/cr/orchestrator/cr_scan.h
ctrl.set_event_sink(&on_event, user);
ctrl.run(&dev, cancel_token, progress_fn, user, stats);
```

`ScanConfig` fields: stage enables (`run_quick` / `run_deep` /
`run_carve`), `whole_disk` (lost-partition discovery), `threads` (0 =
auto), the stage weights, the pre-scan repair/check stages with their
consent callbacks, and the naming/post-processor hooks. Full field
reference in [PARAMETERS.md](PARAMETERS.md#scanconfig). The run is
staged quick → deep → carve with a monotonic weighted progress model,
per-range error attribution, checkpoints at stage boundaries, and
cancellable IRPs throughout (the cancel token is polled inside every
engine loop).

### 3.3 Engines

One header per family under `include/cr/<family>/`: `ntfs` (quick +
complete via MFT walk), `fat` (FAT12/16/32), `exfat`, `ext4`, `hfsplus`,
`btrfs`, `zfs`, `refs`, `ldm` (dynamic disks), `lost` (lost-partition
discovery + scoring). All emit into the shared
`cr::tree::ResultTree` (`FileNode`/`FullNode` dual-class graph) with
four-tier deleted grading, per-node provenance, and the recycle-bin
$I/$R pairing shared model (`cr_recycle.h`) across NTFS/FAT/exFAT.

### 3.4 Carving

`cr::carve::carve_slice(...)` with `CarveFlags` (see
[PARAMETERS.md](PARAMETERS.md#carveflags)): the grammar-validator layer
(JPEG/PNG/ZIP/7z/PST/OFFICE/TIFF/WAV/MP3/PDF/…), the 139-format raw-sign
table, user-registered signs (M-048), the occupancy gate
(`occupied_ranges`, M-039), the window-aware read view (M-025), the
overlap scoring + two-generation policy (N-070), archive inner-entry
inventories (S-032), and the post-processor hook (M-021).

### 3.5 Check / repair / apply

- `cr::check` (`cr_fscheck.h`) — the read-only damage-audit pass
  (FileSystemCheck parity, taxonomy 0x1001..0x100e, never aborts).
- `cr::repair` (`cr_rawfixer.h`) — analyze/dry-run/transactional
  apply/rollback; consent callback required before any write.
- `cr::apply` (`cr_apply.h`) — lost-partition table apply: plan (read-only,
  refusals) → execute (consent-gated) → undo.
- `cr::bcdfix` / `cr::nvramfix` — the boot-chain repair pair (M-040
  stages a+b: BCD cell offset surgery, UEFI Boot#### NVRAM updates).

### 3.6 Sessions, writer, report

- `cr::session` — the `.crsf` container (chunked-zlib records, per-record
  CRC32, versioned, skip-forward; [FORMATS.md](FORMATS.md#the-crsf-session-container)).
- `cr::recovery` — the safe writer (queue, collision naming, atomic
  temp+rename, hash verify, directory tree materialization) and the JSON
  report builder (`cr_report.h`, schema in
  [recovery-report-schema.md](recovery-report-schema.md)).
- `cr::dict` — the password-dictionary aid (D-80 canonical: generated
  228-entry corpus-shape table + the og-format loader).

---

## 4. Error model

- `cr::u32` status codes; `OK == 0`; families are contiguous and
  documented (`include/cr/cr_error.h`, registry in
  [error-fix-registry.md](error-fix-registry.md)).
- Module attribution: `CR_ERRMOD_*` + operation + attempt count on every
  `ErrorRange` event (M-049) — the same triple the report carries.
- Transport errors surface through `cr_ui_last_transport_error` after
  fallback (M-030); scan never aborts on unreadable ranges — they become
  error-map spans that grade the affected nodes.

## 5. Stability classes

1. **The C ABI** — append-only; additive exports bump nothing (the
   version gate is the contract); struct fields only append.
2. **The CLI grammar** — additive only; exit codes stable.
3. **The C++ headers** — internal versioning freedom, coordinated per
   the audit trail (`docs/ground-truth-contracts.md`); hosts binding C++
   pin a commit.
4. **On-disk formats** (`.crsf` records, report schema) — versioned
   records, readers skip forward.
