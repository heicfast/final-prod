# API-SURFACE.md — the cr_ui C ABI context sheet (Phase 0 Agent B, 2026-11-26)

Repo: `/home/z/my-project/final-prod`, baseline commit `126ea398`. READ-ONLY research; every
claim carries `file:line` into this tree. Purpose: everything the next engineer needs to write
a native API harness that drives the engine DLL from CI and cross-checks it against the CLI.
Companion sheets: `worklog/CORE-API.md` (the earlier phase's ABI census — 93 exports then;
**99 at this baseline**), `engine/docs/ABI-CONTRACT.md` (the normative human contract, 908 lines).

---

## 1. Build artifact: the DLL

- CMake target `cr_ui`, SHARED, defined at `engine/src/ui/CMakeLists.txt:11`
  (`add_library(cr_ui SHARED cr_ui.cpp)`); impl is the single translation unit
  `engine/src/ui/cr_ui.cpp` (5502 lines). Windows product = **`cr_ui.dll`** (+ `cr_ui.lib`
  import lib, staged by `build.ps1:114`); other lanes = `libcr_ui.so`
  (`engine/src/ui/CMakeLists.txt:3-9`). Links `cr::core` (+ `cr::win`, `Psapi` on Windows;
  `engine/src/ui/CMakeLists.txt:17-25`).
- Windows Release output path: **`engine/build/win-x64-release/src/ui/Release/cr_ui.dll`**
  (preset `win-x64-release`, `binaryDir ${sourceDir}/build/${presetName}`,
  `engine/CMakePresets.json:12,44-52`; the capitalized `Release` config dir is asserted by
  `build.ps1:87-91`). CLI twin: `engine/build/win-x64-release/src/app/Release/rrecov.exe`.
- Exports: the 99 `CR_UI_API` declarations of `engine/include/cr/cr_ui.h` (MSVC dllexport
  via `CR_UI_BUILD`, `cr_ui.h:65-73`) plus ONE extra C++ symbol on MSVC —
  `cr::log::set_sink`, exported through `engine/src/ui/cr_ui_exports.def:18-19` so the shim's
  own test binary can drive the log seam. Doc-lag note: `docs/ARCHITECTURE.md:24` still says
  "C ABI v4 (92 exports)" and the prior sheet counted 93 — exports are additive without a
  version bump (`engine/docs/ABI-CONTRACT.md:785-797`), so the count drifts by design; pin
  per-baseline (99 unique names at `126ea398`, zero duplicates verified by grep).
- The DLL embeds cr_core statically ("libcr_ui embeds cr_core too", `engine/tests/CMakeLists.txt:161-165`)
  — a harness must NOT also link the static core into the same binary (ASan ODR; same file).
- No explicit library init/shutdown export exists. `cr_ui_abi_version()` is the handshake;
  process-global state: the log sink (`cr::log::set_sink`, `engine/include/cr/cr_log.h:59`),
  `cr_ui_register_user_signs` (process-global, thread-safe, `cr_ui.h:1269`), and the advisory
  `cr_ui_last_transport_error` (`cr_ui.h:349`).

## 2. ABI basics + version handshake

- C11 flat ABI, no C++ types, every entry catch-all → `CR_UI_ERR_INTERNAL` (`cr_ui.h:23-27`).
  Every function returns `uint32_t` from the SAME numeric space as `cr_error.h` (0 = OK).
- **Version handshake**: `CR_UI_ABI_VERSION == 4` (`cr_ui.h:81`; v4 = the driver-removal era
  — one sanctioned export removal, `cr_ui_driver_status`; history in
  `engine/docs/ABI-CONTRACT.md:780-797`). Call `cr_ui_abi_version()` FIRST and refuse the
  load on mismatch (`cr_ui.h:103-105`; `engine/tests/ui_abi_check.c:303`). `cr_ui_last_error()`
  gives thread-local diagnostics text, stable until the next cr_ui call on that thread
  (`cr_ui.h:107-110`; impl `engine/src/ui/cr_ui.cpp:119,1406-1408`).
- Strings: NUL-terminated UTF-8 in; caller buffers (two-call size pattern) or heap strings
  freed with `cr_ui_string_free` (plain `free()` also valid) out (`cr_ui.h:28-29`).
- Zero-initialized config structs are ALWAYS safe (0 = default, never "nothing matches";
  `cr_ui.h:30-33`). Structs only ever APPEND fields (stability policy §9,
  `engine/docs/ABI-CONTRACT.md:778-810`).

## 3. Export census (99, grouped; cr_ui.h line = declaration)

| Group | Exports |
|---|---|
| version/diag (3) | `cr_ui_abi_version` :105, `cr_ui_last_error` :110, `cr_ui_string_free` :113 |
| scan events (1) | `cr_ui_scan_set_event_fn` :312 (subscribe BEFORE run; one sink/scan; while running → `ERR_BAD_PARAMETER`) |
| devices (9) | `cr_ui_open_image` :322 (offset+bytes window; bytes==0→EOF; 512-byte-sector read-only view), `cr_ui_open_disk` :339 (\\.\PhysicalDriveN direct; non-elevated → `ERR_ELEVATION_REQUIRED`), `cr_ui_open_volume` :340 (BitLocker probed first; locked → `ERR_FVE_LOCKED` unless auto-unlock), `cr_ui_last_transport_error` :349 (advisory, process-global, never clears), `cr_ui_open_volume_with_key` :361 (FVEK hex, 64/128 chars), `cr_ui_unlock_volume_with_dictionary` :377 (explicit consent only), `cr_ui_device_close` :381, `cr_ui_device_geometry` :390 (`cr_ui_geometry`: total_bytes/logical_sector/physical_sector/sector_count :383-388), `cr_ui_device_transport` :396 ("direct"/"image", two-call) |
| enumeration (3) | `cr_ui_enumerate_disks` :467, `cr_ui_enumerate_disks_ex` :468 (flags; only `CR_ENUM_FLAG_TRIM` 0x1 :452; unknown bits → `ERR_BAD_PARAMETER`), `cr_ui_enumerate_volumes` :469. Rows: `cr_ui_disk_info` :401-436 (number/size_bytes/sectors/removable/layout 0none-1MBR-2GPT-3EBR/table_ok/model[128]/serial[128]/trim_known/trim_on/bus_type), `cr_ui_volume_info` :438-446 (letter/disk/offset_on_disk/bytes/fs_name[16]/label[64]/bitlocker). Two-call pattern; each call RE-QUERIES the OS, no cache (`cr_ui.h:454-466`) |
| scan lifecycle (19) | `cr_ui_scan_create` :629, `cr_ui_scan_close` :630, `cr_ui_scan_attach_device` :635 (borrow), `cr_ui_scan_run` :640 (synchronous, caller thread), `cr_ui_scan_pause`/`_resume`/`_cancel` :643-645, `cr_ui_scan_busy`/`_stopping` :658-659, `cr_ui_scan_telemetry` :725, `cr_ui_scan_get_stats` :765, `cr_ui_scan_assistant` :794, `cr_ui_session_regrade` :800 (per-selection re-grade; grade-1 only, 0/2 sticky), `cr_ui_scan_checkpoint_count` :802, `cr_ui_scan_repair_save_undo` :811, `cr_ui_scan_save_session` :889, `cr_ui_scan_save_cancel` :899, `cr_ui_scan_snapshot` :905, `cr_ui_scan_verify_identity` :913, `cr_ui_scan_carve_tally_count`/`_tally` :1254-1255 (see carve group) |
| lost-partition apply (3) | `cr_ui_apply_lost_partition` :872 (consent-gated, whole-disk views only; volume slices refused), `cr_ui_apply_undo_last` :879, `cr_ui_apply_save_undo` :884 |
| sessions (10) | `cr_ui_session_load` :921, `_close` :922, `cr_ui_session_resume_last` :934, `cr_ui_session_get_info` :952, `cr_ui_session_stats` :953, `cr_ui_session_snapshot` :954, `cr_ui_session_resume` :961, `cr_ui_session_list` :1018, `cr_ui_session_last_autosave_path` :1036, `cr_ui_session_load_with_events` :1056 |
| snapshot enumeration (5) | `cr_ui_snapshot_close` :1102, `_node_count` :1103, `_partial` :1104, `cr_ui_snapshot_node` :1110, `cr_ui_snapshot_dfs` :1116 |
| query (3) | `cr_ui_query_run` :1180, `cr_ui_query_rows_free` :1183, `cr_ui_query_apply_buckets` :1202 |
| carve registry/user-signs (7) | `cr_ui_carve_format_count` :1231, `cr_ui_carve_format_info` :1234, `cr_ui_scan_carve_tally_count` :1254, `cr_ui_scan_carve_tally` :1255, `cr_ui_register_user_signs` :1273, `cr_ui_user_sign_count` :1284, `cr_ui_user_sign_info` :1285 |
| disk map (2) | `cr_ui_disk_map` :1328, `cr_ui_disk_map_free` :1336 |
| preview (9) | `cr_ui_preview_open_snapshot` :1348, `cr_ui_preview_open_session` :1350, `cr_ui_preview_close` :1352, `cr_ui_preview_size` :1354, `cr_ui_preview_read` :1360 (≤4 MiB/call), `cr_ui_preview_metadata` :1378, `cr_ui_archive_entry_count` :1404, `cr_ui_archive_entry_at` :1405, `cr_ui_preview_unreadable` :1410 |
| thumbnail (3) | `cr_ui_thumbnail_generate` :1479, `cr_ui_thumbnail_generate_snapshot` :1499, `cr_ui_thumbnail_format_supported` :1507 |
| recovery (12) | `cr_ui_recovery_create` :1535, `_close` :1537, `cr_ui_recovery_set_destination` :1544, `cr_ui_recovery_enqueue_node` :1551, `_enqueue_subtree` :1553, `_enqueue_snapshot` :1557, `_enqueue_subtree_snapshot` :1559, `cr_ui_recovery_queue_count` :1562, `cr_ui_recovery_process` :1568, `cr_ui_recovery_cancel` :1569, `cr_ui_recovery_get_stats` :1583, `cr_ui_recovery_report_json` :1588 |
| reconstruct-deleted F-01 (4) | `cr_ui_reconstruct_deleted_preview` :1623, `_preview_snapshot` :1625, `cr_ui_recovery_enqueue_deleted_tree` :1634, `_deleted_tree_snapshot` :1635 |
| health (2) | `cr_ui_health_read` :1712, `cr_ui_health_read_ex2` :1728 (vendor thresholds + power_cycles_nvme) |
| image/fsck/bitlocker (4) | `cr_ui_image_create` :1749, `cr_ui_image_create_ex` :1772 (sector range + cancel flag), `cr_ui_fsck_volume` :1795, `cr_ui_turn_off_bitlocker` :1813 |

## 4. Lifecycle: required call order

Canonical flows (`engine/docs/ABI-CONTRACT.md:135-174`), all mirrored 1:1 in
`engine/tests/ui_abi_check.c` (the executable form of the contract, runs on every lane):

```
scan:   open_* -> scan_create -> scan_attach_device -> [set_event_fn + progress]
        -> scan_run -> scan_snapshot* / scan_get_stats / scan_save_session
        -> scan_close -> device_close
resume: session_load -> session_resume(orig_cfg) -> scan_attach_device
        -> scan_verify_identity -> scan_run -> ... -> scan_close
browse: session_load -> session_snapshot -> snapshot_node/dfs
        -> preview_open_snapshot -> preview_read/metadata -> preview_close
        -> snapshot_close -> session_close
recover: recovery_create(source_dev, cfg) -> set_destination
        -> enqueue_node/subtree*  -> recovery_process -> report_json/get_stats
        -> recovery_close
```
Rules: `scan_run` is one-shot (`ui_abi_check.c:399` double-run refusal; impl
`cr_ui.cpp:2402-2403` "scan already ran"); a device must outlive every scan/preview/recovery
borrowing it (`cr_ui.h:319-320`); close order is the caller's responsibility
(`engine/docs/ABI-CONTRACT.md:51-53`). `scan_run` blocks on the CALLER's thread; call it from
a worker thread like the reference UI does (`cr_ui.h:482-490`).

## 5. Threading contract (one paragraph)

`cr_ui_scan_run` and `cr_ui_recovery_process` run synchronously on the caller's thread;
`cr_ui_scan_pause/_resume/_cancel`, `cr_ui_recovery_cancel`, `cr_ui_scan_telemetry`,
`cr_ui_scan_snapshot`, `cr_ui_scan_busy/_stopping` are safe from ANY thread while they
execute; one handle = one controlling thread (create/run/close on a single thread, the token
calls are the only cross-thread entries); all other entry points: one thread at a time per
handle (`engine/docs/ABI-CONTRACT.md:55-68`, `cr_ui.h:34-38`). Snapshots are immutable value
copies (readers never block writers). Progress callbacks fire on the thread running
run()/process(); EVENT callbacks may additionally fire from engine worker threads AND the
shim's heartbeat watchdog thread (5 s cadence, started only when an event sink is subscribed
or `memory_warn_mb` armed, `cr_ui.cpp:2446-2456`, joined at run end and defensively at
`scan_close`, `cr_ui.cpp:3098-3108`); the shim does NOT serialize event deliveries —
single-flight is the callback's own discipline (`cr_ui.h:140-150`). Engine worker pools
(`cr::ThreadPool`, auto hw−1 clamp 1..8, cap 64) fan out internally — the run() caller sees
only the synchronous call (`engine/AUDIT-ARCH.md:108`).

## 6. Callbacks, events, cancellation

- **Progress** `cr_ui_progress_fn` (`cr_ui.h:125-127`):
  `void (*)(void* user, uint32_t stage, uint32_t stage_permille, uint32_t overall_permille, uint64_t bytes_done, uint64_t bytes_total, uint32_t error_ranges)`.
  stage 0 quick / 1 deep / 2 carve / 0xFFFFFFFF none. Fired on the worker thread; must not
  block; **NO cancellation semantics — the token functions are the cancel path** (`cr_ui.h:115-124`).
- **Typed events** `cr_ui_event_fn` (`cr_ui.h:283`): `void (*)(void* user, const cr_ui_event* evt)`.
  The POD `cr_ui_event` (`cr_ui.h:225-281`) carries: type, stage, timestamp_ms, bytes_done/total,
  stage/overall permille, files/deleted/carved counters, err span + module/op/attempts,
  checkpoint_index, candidate_lba, repair_actions/outcome, check_errors/family, autosave_rc,
  bytes_found. Types (`cr_ui.h:151-223` + `contracts/event-mapping.md:30-43`):
  1 SCAN_STAGE, 2 SCAN_PROGRESS, 3 FILES_FOUND, 4 ERROR_RANGE, 5 CHECKPOINT, 6 LOST_CANDIDATE,
  7 REPAIR, 8 HEARTBEAT, 9 SAVE_PROGRESS, 10 FS_CHECK, 11 SESSION_LOAD_PROGRESS, 12 AUTOSAVED.
  Unknown types MUST be ignored (forward compat; `ui_abi_check.c:178-222`).
- **Subscribe before run**: `cr_ui_scan_set_event_fn(scan, fn, user)` — the sink is read at
  scan start; calling while run() executes is `ERR_BAD_PARAMETER`; one sink per scan; NULL
  unsubscribes (`cr_ui.h:309-312`; round-trip proof `ui_abi_check.c:360-372`).
- **Cancellation**: `cr_ui_scan_cancel(scan)` from any thread → `run()` returns
  `CR_UI_ERR_CANCELLED` with the tree intact/browsable (`cr_ui.h:640-645,637-639`).
  Recovery: `cr_ui_recovery_cancel` → `process()` returns ERR_CANCELLED, committed items stay
  (`cr_ui.h:1564-1569`). Save-scoped: `cr_ui_scan_save_cancel` (:899, never touches the scan
  token). Load-scoped: caller-owned `volatile uint32_t* cancel_flag` on
  `cr_ui_session_load_with_events` (:1056). Image: `cancel_flag` on `cr_ui_image_create_ex`
  (:1772). ERR_CANCELLED is NOT an error for the bridge — translated to status, never thrown
  (`contracts/error-codes.json:47-54`).
- Consent callbacks (synchronous, on the run thread, default-deny on zero/NULL):
  `cr_ui_repair_consent_fn` (:523) fires once pre-engines with a `cr_ui_repair_plan` POD
  (:513-521); `cr_ui_apply_consent_fn` (:853) fires once with `cr_ui_apply_plan` (:841-851)
  before any table write. Bridge marshaling rules (never block/lock/re-enter; channel
  try_send; stale-id handling) are pinned in `contracts/event-mapping.md:10-27,75-109`.

## 7. Error model + CLI exit-code mapping

Enum `cr::Err` (`engine/include/cr/cr_error.h:25-127`), single numeric space shared by ABI,
core, and bridge (`contracts/error-codes.json:5-9`). Top codes:

| Code | Symbol | Value |
|---|---|---|
| OK | `cr::Err::OK` | 0x00000000 |
| ERR_OPEN_FAILED | open/transport | 0x50020001 |
| ERR_READ_FAILED | read | 0x50020002 |
| ERR_CANCELLED | cooperative cancel | 0x50020005 |
| ERR_DEVICE_DISCONNECTED | device gone mid-run (N-125) | 0x50020007 |
| ERR_BAD_MAGIC | session/volume mismatch | 0x50030001 |
| ERR_BAD_PARAMETER | invalid argument | 0x50030003 |
| ERR_TRUNCATED | buffer too small (two-call) | 0x50030004 |
| ERR_UNSUPPORTED | Windows-only surface on Linux | 0x50030008 |
| ERR_FVE_LOCKED | BitLocker locked | 0x50060001 |
| ERR_ELEVATION_REQUIRED | raw disk needs elevated token | 0x500C0001 |
| ERR_INTERNAL | catch-all bug | 0x50090001 |

(Full 40-code table incl. dest family 0x50050001-09, session 0x5004xxxx, apply
0x50080001-07, health 0x50070001, image 0x500A0001: `cr_error.h:29-126`; every code mapped
1:1 to a UI vocabulary word in `contracts/error-codes.json:10-347`.)
**CLI exit codes** (rrecov.cpp:57-71): scan 0 ok / 1 failure (`rrecov.cpp:2113`) / 3
cancelled — session saved when `--out` was given (`rrecov.cpp:2077`) / 5 device disconnected
(`rrecov.cpp:2094`) / **4 = non-elevated raw-disk verb** (`require_privilege`,
`rrecov.cpp:538-548`); recover 2 partial = item errors or hash mismatch
(`rrecov.cpp:2755`), 0 complete; health 3 when every source unavailable; list/sessions 1 on
enumeration failure; image 3 cancelled / 2 failed (partial kept as `<out>.partial`).
Elevation facts for CI: image targets (`--image` / `cr_ui_open_image`) are exempt — the ABI
harness can run the full synthetic-fixture lifecycle unprivileged; disk/volume/health/image verbs
need an elevated token and refuse with 0x500C0001 (`docs/ARCHITECTURE.md:66-78`;
`cr_ui.cpp:1452-1479`).

## 8. Scan result rows — exact field names

Two surfaces; record these per file in the harness:

- **Snapshot row** `cr_ui_node_info` (`cr_ui.h:1068-1100`): `node_id` (stable id; preview/
  recovery addressing), `fs_kind` (cr_types.h `FsKindCode`: 0/1/2 FAT12/16/32, 3 NTFS, 4 ext2,
  6 Raw, 7 exFAT, 8 HFS+, 9 ext4, 10 ReFS, 11 Btrfs, 13 ZFS — `engine/include/cr/cr_types.h:81-101`),
  `record_ref` (NTFS record; 0 carved), `origin` (0 structured, 1 carved, 2 lost, 3 virtual),
  `deleted` (0 active, 1 deleted, 2 partial, 3 damaged), `confidence` (ordinal 0..4),
  `grade` (S-023 recoverability: 0 low, 1 average, 2 high — sticky), `overlap`, `is_full`
  (roots/virtuals carry role+stats), `logical_size`, `allocated_size`, `created`/`modified`/
  `changed`/`accessed` (100ns since 1601), `attributes` (DOS; 0x10 = dir), `flags`,
  `extent_count`, `resident_bytes`, `carve_format`, `carve_confidence`, `hash_valid` +
  `sha128[16]` (truncated content hash), `name_len` (+ name via `cr_ui_snapshot_node`
  two-call name_buf), `version_group`, `tags_extracted`. Tree topology: `parent`,
  `first_child`, `next_sibling`, `prev_sibling` as INDICES, `CR_UI_SNAP_NONE` = 0xFFFFFFFF
  (:1066); DFS order via `cr_ui_snapshot_dfs` (byte-parity with the live serializer, :1113).
- **Session query row** `cr_ui_query_row` (`cr_ui.h:1167-1178`): `node_id`, `name` (heap),
  `path` (heap, '/'-joined), `size`, `modified`, `origin`, `deleted`, `confidence`,
  `category`, `grade`, `version_group`, `tags_extracted`; freed with
  `cr_ui_query_rows_free(rows, row_count)` (:1183).
- **What the CLI prints** (comparison target): scan summary line `scan complete: N file(s)
  (X GB), N deleted, N carved, N lost partition(s) in T s` (`rrecov.cpp:2143-2147`); per-file
  CSV `"path",size,origin,deleted,confidence,first_extent_offset` (`rrecov.cpp:1000-1004`);
  per-file JSON `{"path","name","size","modified","origin","deleted","confidence","category"}`
  under `{"total":N,"rows":[...]}` (`rrecov.cpp:2173-2222`); human tree `name (size)
  [deleted|risk] [carved|lost]` (`rrecov.cpp:1005-1015`). Scan `--json` summary fields:
  files, deleted, carved, lost_partitions, bytes_scanned, bytes_unreadable, bytes_found,
  error_ranges, probe_unreadable, elapsed_ms (`rrecov.cpp:2128-2136`).

## 9. Scan config: modes, weights, filters

`cr_ui_scan_config` (`cr_ui.h:525-627`) — zero-init = default pipeline:
`stage_mask` (0 = default quick|deep|carve = 7; bits 1 quick, 2 deep, 4 carve — a quick-only
scan = 1), `whole_disk` (1 = lost-partition discovery; needs a DISK view), `fs_hint`
(0 auto-probe; 1 NTFS, 2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS, 11 UDF;
10 BitLocker probe-only invalid, `cr_ui.h:492-496`), `quick_weight`/`deep_weight`/
`carve_weight` (0 = 40/35/25), `threads` (0 = auto), identity-card overrides
(`volume_label_utf8` truncated to 99 chars, `volume_model_utf8`, `volume_serial_utf8`,
`volume_firmware_utf8`, `volume_wwn_utf8`, `volume_interface_utf8` — auto-filled from
enumeration on Windows when absent), `checkpoint_path_utf8` (stage-boundary checkpoints,
overwrite-in-place, = CLI `--out`), `progress`/`progress_user`, `pre_scan_repair` +
`repair_consent`/`repair_consent_user` (S-008), `memory_warn_mb` (0 = off),
`raw_category_name_utf8` (default "Other Deleted Files (Carved)" role), `pre_scan_check`
(M-041 damage audit), `autosave_dir_utf8` (completion auto-save; empty = off; S-044 gates:
empty/resumed/already-saved runs never auto-save), `filter_folders_utf8` +
`filter_folder_count` (M-073 scan-time folder filter, pruned at stage boundaries,
persisted as REC_SCAN_FILTER).
Post-scan filters/sort/paging: `cr_ui_query` (`cr_ui.h:1136-1165`) — `category_mask`
(1 image…8 bookmarks), `size_min/max`, `modified_from/to` (100ns-1601; 0 = unbounded),
`confidence_min`, `extensions[]` (lowercased WITH dot), `name_substring`,
`path_prefix`, `path_substring`, `state_mask` (DeletedState bits), `deleted_only`,
`origin_mask`, `sort` (0 none, 1 name, 2 size, 3 modified, 4 confidence, 5 path, 6 grade,
7 type) + `dir` (0 asc/1 desc), `offset`/`limit` (0 = default 100), `count_only`,
`version_group`, `tags_only`. Og filter-bar bucket IDs translate via
`cr_ui_query_apply_buckets` (:1202; Date 0x100..0x103/0x105, Size 0x1000..0x1004/0x1006,
unknown → ERR_BAD_PARAMETER).

## 10. Target addressing (letter vs PhysicalDriveN vs image)

- **Image file**: `cr_ui_open_image(utf8_path, offset, bytes, &dev)` — read-only
  512-byte-sector device bounded to [offset, offset+bytes); bytes==0 → to EOF (`cr_ui.h:322-326`).
  The ABI-check/test surface (also .vmdk/.vhd/.vhdx container parse, S-030; CLI
  `--image file.vmdk|.vhd|.vhdx|.img [.dmg raw]`, `rrecov.cpp:4096`). No elevation needed.
- **Physical disk N**: `cr_ui_open_disk(u32 disk, &dev)` — direct raw I/O, opens
  `\\.\PhysicalDriveN` under an administrator token; non-elevated → ERR_ELEVATION_REQUIRED
  (0x500C0001); no driver, no service, no fallback leg (`cr_ui.h:327-339`,
  `cr_ui.cpp:1449-1479`, `docs/ARCHITECTURE.md:46-49`).
- **Volume by letter**: `cr_ui_open_volume(char letter, &dev)` — BitLocker status probed
  first (locked → ERR_FVE_LOCKED 0x50060001 unless auto-unlock succeeds; ERR_OPEN_FAILED when
  the boot sector is unreadable); then maps letter → (disk, start, len) via
  `win::volume_extent`, opens the PARENT PhysicalDrive and returns the volume's
  `PartitionSlice` (over-long windows clamp; parent held alive internally)
  (`cr_ui.cpp:1481-1550`). Locked with a key: `cr_ui_open_volume_with_key(letter,
  utf8_hex_fvek)` (:361; 64/128 hex chars, in-stack XTS-AES). Dictionary unlock (explicit
  consent only): `cr_ui_unlock_volume_with_dictionary(letter, const char* const*, count)`
  (:377-379).
- **Unallocated regions / lost partitions**: NOT directly addressable as a device. The
  whole-disk scan (`whole_disk=1` on a disk/image-at-offset-0 view) discovers lost
  partitions (stride sweep + probes, `engine/AUDIT-PIPELINE.md:104-113`); candidates surface
  as events (CR_EVT_LOST_CANDIDATE, `candidate_lba`) + stats.lost_partitions + virtual
  "Candidate @ LBA" roots. Restoring one to the table = the consent-gated
  `cr_ui_apply_lost_partition(dev, &cr_ui_apply_candidate{start_lba, sector_count, fs,
  disk_layout, disk_flags}, rebuild_table, consent_fn, user, &result)` on a WHOLE-DISK view
  (volume slices refused — the table lives at LBA 0; `cr_ui.h:865-875`). fs values:
  0 NTFS, 1 FAT, 2 exFAT, 4 GPT-table (refused), 5 unknown (refused), 6 ext2, 7 HFS+,
  8 Btrfs, 9 ZFS, 10 ReFS, 11 BitLocker (`cr_ui.h:824-827`).
- Health/image verbs address by physical-disk NUMBER (`cr_ui_health_read(disk, …)`,
  `cr_ui_image_create(disk, path, …)`); fsck/bitlocker-off by volume LETTER
  (`cr_ui_fsck_volume(letter, flags, &exit)`, `cr_ui_turn_off_bitlocker(letter, …)`).

## 11. Buffer ownership + memory rules

Ownership table (`engine/docs/ABI-CONTRACT.md:39-53`): device/session/scan/snapshot/preview/
recovery handles created by their open/create/export, freed by their `_close`; a scan
borrows its device (caller keeps it alive until after scan_close); a recovery borrows the
source device + every session/snapshot whose nodes were enqueued; a preview borrows the
DEVICE (properties are private copies — the session/snapshot may close first). Heap strings
(report_json, …) → `cr_ui_string_free`; query row arrays → `cr_ui_query_rows_free`;
disk-map overlays → `cr_ui_disk_map_free` (each array separately, NULL no-op); session-list
rows are caller-owned PODs (no free export); `cr_ui_session_row.path` is a fixed 1024-byte
field (`cr_ui.h:997-1016`). Two-call size patterns (call with NULL/0 buffer → needed count;
then capacity IN, filled OUT; too small → ERR_TRUNCATED): device_transport, snapshot_node
name, snapshot_dfs, session_list, health attrs, thumbnail (rgba==NULL probe), disk-map
bad_ranges.

## 12. Sessions (.crsf) API

- Save: `cr_ui_scan_save_session(scan, path)` — valid after run() INCLUDING a cancelled one
  (the caller's explicit save-on-cancel; `ui_abi_check.c:430-436`); atomic tmp+rename; emits
  CR_EVT_SAVE_PROGRESS; `cr_ui_scan_save_cancel` is save-scoped (:886-899).
- Load: `cr_ui_session_load(path, &sess)` / `cr_ui_session_load_with_events(path, fn, user,
  cancel_flag, &sess)` (NULL/NULL == plain load, `ui_abi_check.c:631-638`). Container magic
  CRSFMT01; corrupt → 0x50040001/0x50030001 family.
- Info/stats: `cr_ui_session_get_info` (`cr_ui_session_info` :936-950: fs_family,
  stage_done_mask, configured_mask, slice_offset/bytes, sectors, label[32]+label_full[100],
  scanned_bytes, elapsed_ms, model[48], serial[32], boot_hash) and `cr_ui_session_stats`
  (reuses `cr_ui_scan_stats`).
- Resume: `cr_ui_session_resume(sess, &orig_cfg, &scan)` → fresh scan handle; attach the
  ORIGINAL device; `cr_ui_scan_verify_identity` (boot-hash compare → ERR_BAD_CHECKSUM on
  mismatch); run() skips completed stages (`ui_abi_check.c:1022-1044`).
- List/pointed-resume: `cr_ui_session_list(dir, rows, &count)` — every *.crsf in the dir,
  light records only, NEWEST-FIRST by embedded save time, unreadable files get
  valid=0+error rows; `cr_ui_session_resume_last(dir, &sess)` + `cr_ui_session_last_autosave_
  path(dir, out_path, cap)` read the `last_scan.info` pointer sidecar (dead pointer →
  ERR_RANGE_OUT_OF_BOUNDS 0x50020004; `ui_abi_check.c:586-594`).
- Auto-save: config `autosave_dir_utf8` → `auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf` on run()
  completion (or user-STOPPED with results, S-057); CR_EVT_AUTOSAVED carries the rc
  (`cr_ui.h:588-615`).
- Export: the .crsf itself is the interchange (CLI `tree --in`, `rrecov sessions` list it);
  no separate export format.

## 13. Health API (Windows-only; ERR_UNSUPPORTED elsewhere)

`cr_ui_health_read(disk, &cr_ui_smart_report, attrs, attr_cap, &attr_count)` — ONE call
fills the report and (attrs != NULL) up to attr_cap rows; attr_count returns the needed size
either way (two-call, `cr_ui.h:1710-1714`; the CLI equivalent prints
"…attribute(s)" + rows, `rrecov.cpp:2791-2812`). Report struct `cr_ui_smart_report`
(:1667-1708): available, transport[24] ("ata-passthrough"/"nvme"/"legacy"), ata/smart
versions, failing_now, reallocated/pending/uncorrectable sectors, power_on_hours,
ssd_wear_level, temperature_c, NVMe block (nvme_available, temperature_c_x10, spares,
percentage_used, data units, unsafe_shutdowns, media_errors, critical_warning), ssd_like,
trim_known/trim_enabled, host_reads/writes, controller_busy_time, data_bytes read/written,
power_cycles_nvme, disk `grade` (0 Unknown/1 Good/2 Caution/3 Bad, D-P4). Attribute rows
`cr_ui_smart_attr` (:1642-1665): id, value, worst, raw (6 vendor bytes), failing, name[40],
threshold (ex2 only), grade. `cr_ui_health_read_ex2` (:1728) = same call + vendor thresholds
(0xD1) + power_cycles; callers tolerant of a threshold-less engine fall back on
ERR_UNSUPPORTED. Requires elevation like every disk verb (CLI opens
`\\.\PhysicalDrive%u` itself, `rrecov.cpp:2779-2789`).

## 14. API-only vs CLI-only surface

**API exposes, CLI does not** (CLI = `rrecov.exe`, a C++-core consumer, see §15):
pause/resume tokens (CLI has only Ctrl-C + `--pause-stdin` keyboard surface,
`rrecov.cpp:19-22`), the typed event stream + heartbeat, live telemetry pull
(`cr_ui_telemetry`/`scan_busy`/`scan_stopping`), mid-scan snapshots + snapshot enqueue
(recover-while-scanning), the full query layer (CLI `tree --json` uses query::run with
name/deleted filters only, `rrecov.cpp:2186-2191`), preview reads/metadata, thumbnails,
disk-map overlays, assistant stats + regrade, session_list/load-with-events/
resume-last, `cr_ui_recovery_config.conflict` skip/overwrite (CLI has NO --conflict flag —
WRITER-7, `engine/AUDIT-PIPELINE.md:203`), `sf_collision` variants via F-01 pair,
`cr_ui_last_transport_error` (PRODUCT-WIRING.md:504-509 confirms the UI bridge leaves
several of these unwired too).
**CLI exposes, ABI does not**: `bench` (drive benchmark), `fix --part/--apply/--undo-apply`
(offline undo-container restore; the ABI only saves undo + `apply_undo_last` in-session),
`apply --boot-fix/--boot-fix-bcd` (N-121 BCD repair), `image --rescue` (F-33/F-34 ddrescue
mode, `rrecov.cpp:2907-3036`), output formats (`--csv`, `--json`, `--log`, pause keyboard),
the CLI's own exit-code vocabulary (§7). Both share: scan/recover/health/image/fsck/
bitlocker verbs, user-signs, dictionary unlock, FVEK unlock, repair/apply, sessions.

## 15. How the CLI drives the engine (critical architecture fact)

`rrecov.exe` does **NOT** call the cr_ui ABI. It includes and links the C++ core directly
(`engine/src/app/rrecov.cpp:79-110` — `cr/orchestrator/cr_scan.h`, `cr/win/*`, …; zero
`cr_ui.h` usage, grep-verified) — deliberately, to avoid two core copies in one process
(`rrecov.cpp:857-861` no-second-singleton rule). Reference call sequence for `scan`
(`rrecov.cpp:1301-2170`): privilege gate → BitLocker pre-pass (--bitlocker-fvek/--dict) →
user-signs install → open target (`open_disk_view`/`open_volume_view`/`open_image_view`) →
parse flags into `orchestrator::ScanConfig` (whole_disk/quick-only/no-carve/threads/
filter-folder/--out checkpoint sink) → resume-or-fresh `ScanController` → `ctrl->run(device,
&g_cancel_token, progress_fn, &prog, stats)` on the main thread → stats/tree printing →
save session/checkpoint → autosave-if-due. The ABI shim wraps THE SAME core
(`cr_ui.cpp:2457` `s->ctrl->run(...)`), so an API harness that replays the ui_abi_check
lifecycle is driving the identical engine path the CLI drives — result trees, stats, and
sessions are directly comparable (CLI `--json`/`--csv` vs harness query rows; CLI .crsf vs
harness `cr_ui_scan_save_session` output).

## 16. Minimal harness sequences (the runner recipe)

1. **Gate**: `cr_ui_abi_version() == 4` else abort; `cr_ui_last_error()` sanity.
2. **Targets**: `cr_ui_enumerate_disks(NULL, &n)` → alloc → `cr_ui_enumerate_disks(rows, &n)`
   (Windows only; on Linux lanes expect ERR_UNSUPPORTED, `ui_abi_check.c:1159-1168`).
   CI fixture: build the synthetic FAT16 image (`ui_abi_check.c:60-144`) → `cr_ui_open_image(img, 0, 0, &dev)`.
3. **Quick scan**: zero-init `cr_ui_scan_config cfg` (memset), set `cfg.stage_mask = 1`
   (quick only; 0 = full 7-stage default), `cfg.progress`/`progress_user` optional,
   `cfg.checkpoint_path_utf8` optional → `cr_ui_scan_create(&cfg, &scan)` →
   `cr_ui_scan_attach_device(scan, dev)` → `cr_ui_scan_set_event_fn(scan, evt_cb, NULL)` →
   `cr_ui_scan_run(scan)` (0 or 0x50020005) → `cr_ui_scan_get_stats` (`files_found`,
   `deleted_found`, `carved_found`, `lost_partitions`, `bytes_scanned`, `elapsed_ms`,
   `error_ranges`, `bytes_unreadable`, `autosaved`).
4. **Enumerate results**: EITHER mid/post-run `cr_ui_scan_snapshot(scan, &snap)` →
   `cr_ui_snapshot_node_count` → per index `cr_ui_snapshot_node(snap, i, &info, namebuf, cap)`
   (+`cr_ui_snapshot_dfs` for order) → `cr_ui_snapshot_close`; OR save →
   `cr_ui_session_load(crsf, &sess)` → `cr_ui_query_run(sess, &q, &rows, &n, &total)` →
   `cr_ui_query_rows_free`.
5. **Recover**: zero-init `cr_ui_recovery_config rcfg` (+ `rcfg.progress`) →
   `cr_ui_recovery_create(dev, &rcfg, &rec)` → `cr_ui_recovery_set_destination(rec, dir)` →
   `cr_ui_recovery_enqueue_node(rec, sess, node_id)` (or subtree/snapshot variants) →
   `cr_ui_recovery_process(rec)` → `cr_ui_recovery_get_stats` (`done/refused/errors/
   hash_mismatch/…`) + `cr_ui_recovery_report_json(rec, src, &json)` →
   `cr_ui_string_free(json)` → `cr_ui_recovery_close`.
6. **Session save+resume**: `cr_ui_scan_save_session(scan, "x.crsf")` → close scan →
   `cr_ui_session_load("x.crsf", &sess)` → `cr_ui_session_resume(sess, &orig_cfg, &scan2)`
   (zero-init orig_cfg mirrors the interrupted run) → `cr_ui_scan_attach_device(scan2, dev2)`
   → `cr_ui_scan_verify_identity(scan2)` → `cr_ui_scan_run(scan2)` → close.
7. **Health** (Windows, elevated): `cr_ui_health_read(disk, &rep, NULL, 0, &need)` →
   alloc → `cr_ui_health_read(disk, &rep, attrs, need, &need2)`.
8. **Teardown**: reverse-open order — recovery_close, snapshot_close, session_close,
   scan_close, `cr_ui_device_close(dev)` (`ui_abi_check.c:1192-1208`).
Logging for CI: register the core sink `cr::log::set_sink(fn, user)` (exported via
`cr_ui_exports.def:19`) — Error/Warn/Info lines from any thread, serialized internally
(`engine/include/cr/cr_log.h:26-34`).

## 17. Harness pitfalls (pinned by tests)

- Enumeration/two-call: `*count` is IN capacity when out != NULL, OUT needed when NULL;
   capacity < needed → ERR_TRUNCATED with *count = filled (`cr_ui.h:454-456,972-984`).
- `cr_ui_snapshot_node` name truncation: returns OK when everything fit; a capped copy
  returns ERR_TRUNCATED with `name_len` = FULL length (`cr_ui.h:1106-1109`,
  `ui_abi_check.c:680-682`).
- Cancel-before-run is legal: `stopping` reads 1, `run()` returns ERR_CANCELLED, the
  cancelled scan still saves a truthful session (`ui_abi_check.c:401-439`).
- The event sink must handle HEARTBEAT (8) without synthesizing progress; unknown types are
  ignored not fatal (`ui_abi_check.c:215-221`).
- ERR_CANCELLED / ERR_TRUNCATED / ERR_RANGE_OUT_OF_BOUNDS are contract answers, not bugs
  (`contracts/error-codes.json:47-54,104-111`).
- Progress overall_permille is monotonic (engine-guaranteed; the test asserts it,
  `ui_abi_check.c:150-168`).
- On Linux lanes every Windows-layer verb returns ERR_UNSUPPORTED — the ABI-check pins
  exactly which (`ui_abi_check.c:1152-1186`); a CI harness should branch on platform.
- The .def-extra symbol is the ONLY non-cr_ui export; a GetProcAddress-based harness
  resolves names individually anyway.
