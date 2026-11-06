# CORE-API — the engine core contract as a consumer sees it (T2-C)

Ground truth: `engine/include/cr/cr_ui.h` (1,602 ln) + `engine/src/ui/cr_ui.cpp` (5,086 ln),
`engine/src/core/orchestrator/cr_scan.cpp` + `engine/include/cr/orchestrator/cr_scan.h`,
`engine/src/app/rrecov.cpp`, `engine/src/win/cr_health.cpp`, `engine/src/win/cr_enumerate.cpp`,
cross-checked against `engine/docs/API.md` and `engine/docs/ABI-CONTRACT.md`. What IS, not what
should be. (Note: the UI header lives at `engine/include/cr/cr_ui.h` — there is no `cr/ui/` dir.)

## 1. The cr_ui ABI surface (v4)

**Version gate.** `CR_UI_ABI_VERSION 4u` (cr_ui.h:81); `cr_ui_abi_version()` returns it
(cr_ui.h:105, cr_ui.cpp:1377). Host must call it first and refuse on mismatch. Policy
(cr_ui.h:39-48): adding exports, appending struct fields, new event `type`s = compatible, NO
bump; removing/renaming exports, changing field type/meaning/order/ownership/threading = bump.
v3→v4 bump was the M-067 `cr_ui_user_sign` field resize (cr_ui.h:75-80) and the sanctioned
removal of the driver-era `cr_ui_driver_status` export (docs/API.md:110-116). 92 exports
(docs/API.md:111). `CR_UI_API` = dllexport/dllimport on Win32, default-visibility elsewhere
(cr_ui.h:63-73); MSVC additionally exports one core symbol via `engine/src/ui/cr_ui_exports.def:18-19`.

**Rules (cr_ui.h:23-48, ABI-CONTRACT §1-3).** C11, no C++ types, no exception escapes — every
entry wrapped in `guarded()` catch-all → `CR_UI_ERR_INTERNAL` (cr_ui.cpp:138-146). Strings in:
NUL-terminated UTF-8; out: caller buffers (two-call) or heap freed with `cr_ui_string_free`
(`free()` also valid, cr_ui.cpp:1385-1387). Zero-init configs always safe (0 = default). Handles
opaque; close order caller's. Threading: `run()`/`process()` synchronous on the caller's thread;
pause/resume/cancel/telemetry/snapshot safe from any thread while running (cr_ui.h:34-38,
622-639); progress + event callbacks fire on **engine worker threads** (never block, no cancel
semantics — token functions are the cancel path); heartbeat events fire on the shim's watchdog
thread (cr_ui.h:141-150). `cr_ui_scan_set_event_fn` is pre-run only (ERR_BAD_PARAMETER while
running; cr_ui.cpp:2477-2499). `cr_ui_last_error()` = thread-local text, stable until next
cr_ui call on that thread (cr_ui.cpp:117-136).

**Error codes** — same numeric space as `engine/include/cr/cr_error.h:25-121`; families:
- `0x5002` open/transport: 01 OPEN_FAILED, 02 READ_FAILED, 03 WRITE_FAILED, 04 RANGE_OUT_OF_BOUNDS,
  05 CANCELLED, 06 RETRY_EXHAUSTED, 07 DEVICE_DISCONNECTED (N-125).
- `0x5003` format/parse: 01 BAD_MAGIC, 02 BAD_CHECKSUM, 03 BAD_PARAMETER, 04 TRUNCATED,
  05 FIXUP_FAILED, 06 CYCLE_LIMIT, 07 DEPTH_LIMIT, 08 UNSUPPORTED, 09 NO_MEMORY.
- `0x5004` session: 01 FORMAT, 02 IO, 03 VERSION. `0x5005` writer/dest: 01 OVERLAP, 02 UNWRITABLE,
  03 HASH_MISMATCH, 04 FULL, 05 NAME_EXHAUSTED, 06-08 path taxonomy. `0x5006` FVE: 01 LOCKED,
  02 ELEVATION_REQUIRED. `0x5007` health: 01 UNAVAILABLE. `0x5008` repair/apply: 01-07.
- `0x5009` INTERNAL, `0x500A` IMAGE_CORRUPT, `0x500C0001` ELEVATION_REQUIRED (fresh value space;
  the 0x500B driver family was deleted with the driver, cr_error.h:108-120).

**Export inventory, grouped** (all `cr_ui_*`):
- *lifecycle*: `abi_version`, `last_error`, `string_free` (cr_ui.h:105-113).
- *devices/enumeration*: `open_image(path,offset,bytes)` (bytes==0 → to EOF, exposed as 512-B
  sector RO device, cr_ui.h:313-316); `open_disk`/`open_volume`/`open_volume_with_key` (Win-only
  direct `\\.\PhysicalDriveN` I/O; unelevated → ERR_ELEVATION_REQUIRED 0x500C0001; locked volume →
  ERR_FVE_LOCKED 0x50060001 unless auto-unlock; cr_ui.h:318-353; gate cr_ui.cpp:1424-1439);
  `unlock_volume_with_dictionary` (consent-only, N-064); `last_transport_error` (advisory,
  process-global, never cleared, cr_ui.h:340); `device_close/geometry/transport`
  (transport string "direct"|"image", cr_ui.h:372-388); `enumerate_disks`, `enumerate_disks_ex(flags)`,
  `enumerate_volumes` (cr_ui.h:447-449).
- *scan*: `scan_create` (config fill cr_ui.cpp:1294-1369, 2025-2047), `scan_close`,
  `scan_attach_device` (borrowed; identity card auto-filled on Win, cr_ui.h:615), `scan_run`
  (one-shot: `started` flag, second run → ERR_BAD_PARAMETER, cr_ui.cpp:2320-2321), `scan_pause/
  resume/cancel` (token, cr_ui.cpp:2435-2452), `scan_busy`/`scan_stopping` (N-075 synchronous
  state, cr_ui.cpp:2463-2475), `scan_set_event_fn`, `scan_telemetry`, `scan_get_stats`,
  `scan_assistant` (S-023 grading), `scan_checkpoint_count`, `scan_repair_save_undo`,
  `scan_carve_tally_count/_tally` (N-140), `scan_save_session` (atomic tmp+rename, N-060),
  `scan_save_cancel` (SAVE-scoped, never the scan token, cr_ui.h:835), `scan_snapshot`
  (mid-scan partial=1 allowed), `scan_verify_identity` (boot-hash CRC32 vs session,
  ERR_BAD_CHECKSUM on mismatch, cr_ui.cpp:2962-2980).
- *sessions*: `session_load`, `session_load_with_events` (W1b: caller-owned `cancel_flag`
  polled per node, torn load never returns partial, cr_ui.h:992-994), `session_close`,
  `session_resume`, `session_resume_last` (M-154 pointer sidecar `last_scan.info`),
  `session_get_info`, `session_stats`, `session_snapshot`, `session_regrade`, `session_list`
  (W1a light-records newest-first, 1024-byte path rows, cr_ui.h:933-955),
  `session_last_autosave_path` (ERR_RANGE_OUT_OF_BOUNDS 0x50020004 when dead pointer, cr_ui.h:972).
- *snapshots*: `snapshot_close/node_count/partial/node/dfs` (flat index rows, CR_UI_SNAP_NONE=
  0xFFFFFFFF, cr_ui.h:996-1052).
- *query*: `query_run`, `query_rows_free`, `query_apply_buckets` (G-12; sort 0..7, category bits,
  offset/limit host-side model, default limit 100, cr_ui.h:1054-1141).
- *carve registry / user signs*: `carve_format_count/_info` (grammar rows then raw-sign rows),
  `register_user_signs`, `user_sign_count/_info` (cr_ui.h:1143-1221).
- *disk map*: `disk_map`, `disk_map_free` (allocation/coverage/found/confidence overlays +
  bad_ranges pairs; free overlay absent for LIVE mid-scan snapshot path, cr_ui.h:1223-1272).
- *preview*: `preview_open_snapshot/_session`, `preview_close/size/read/metadata/unreadable`,
  `archive_entry_count/_at` (M-116 inventory cached per handle; read ≤4 MiB/call; unreadable
  spans zero-filled AND reported, cr_ui.h:1274-1347).
- *thumbnail*: `thumbnail_generate` (two-call size probe; ERR_NO_MEMORY on small buffer,
  ERR_TRUNCATED = decode refusal which still promotes grade 2, ERR_UNSUPPORTED; success promotes
  grade 2, M-083/M-115, cr_ui.h:1349-1418), `thumbnail_format_supported`.
- *recovery*: `recovery_create/close/set_destination/enqueue_node/enqueue_subtree/
  enqueue_snapshot/enqueue_subtree_snapshot/queue_count/process/cancel/get_stats/report_json`
  (cr_ui.h:1424-1492).
- *health*: `health_read` (§5).
- *lost-partition apply (S-029)*: `apply_lost_partition` (consent-gated, default-deny),
  `apply_undo_last`, `apply_save_undo` (cr_ui.h:749-820).
- *system*: `image_create` (S-040 flat raw + .sha256 sidecar; cancel keeps `<path>.partial`,
  cr_ui.h:1540-1558), `fsck_volume` (M-075 chkdsk escalation; raw exit code, 15-min timeout
  yields 259), `turn_off_bitlocker` (M-078 destructive consent twin; OK = accepted not plaintext,
  cr_ui.h:1560-1596).

**Key structs (fields / units).**
- `cr_ui_geometry` (cr_ui.h:374-379): `total_bytes` u64; `logical_sector`, `physical_sector` u32 B;
  `sector_count` u64.
- `cr_ui_disk_info` (cr_ui.h:392-416): `number` u32; `size_bytes` u64; `logical_sector`/
  `physical_sector` u32 B; `removable` (1=USB/SD bus); `layout` (0 none, 1 MBR, 2 GPT, 3 EBR —
  N-057 append); `table_ok`; `model[128]`, `serial[128]` UTF-8; `trim_known` (0=unknown, only
  filled by `_ex`+`CR_ENUM_FLAG_TRIM`), `trim_on`.
- `cr_ui_volume_info` (cr_ui.h:418-426): `letter` char (0 when absent); `disk` u32;
  `offset_on_disk`, `bytes` u64; `fs_name[16]`, `label[64]`; `bitlocker` (1=FVE detected).
- `cr_ui_scan_config` (cr_ui.h:505-607): `stage_mask` (0=default 7; bits 1 quick/2 deep/4 carve;
  unknown bits refused, cr_ui.cpp:1296-1298); `whole_disk`; `fs_hint` (serialized FsFamily: 1 NTFS,
  2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS, 11 UDF; 5/10 never valid hints,
  cr_ui.h:472-476); `quick/deep/carve_weight` (0 = 40/35/25 percent); `threads` (0=auto hw-1 clamp
  1..8, 1=serial, cr_scan.cpp:58-68); `volume_label_utf8` (truncated 99), `volume_model/serial/
  firmware/wwn/interface_utf8` (identity-card overrides; shim fills from enumeration);
  `checkpoint_path_utf8` (stage-boundary auto-checkpoints); `progress`+`progress_user`;
  `pre_scan_repair`+`repair_consent`+user (S-008 default-deny); `memory_warn_mb` (0=OFF, N-062);
  `raw_category_name_utf8` (M-029); `pre_scan_check` (M-041); `autosave_dir_utf8` (W1c);
  `filter_folders_utf8`+`filter_folder_count` (M-073, NULL/0 = OFF).
- `cr_ui_event` (cr_ui.h:225-272): `type`, `stage`, `timestamp_ms` (run-elapsed, pause-compensated);
  progress: `bytes_done/bytes_total` (stage-true bytes where counted), `stage_permille`,
  `overall_permille`; counters `files_found/deleted_found/carved_found` (cumulative);
  error-range `err_start_bytes/err_len_bytes`, `error_ranges` (cumulative after append),
  `err_module/err_operation/err_attempts` (M-049, zero elsewhere); `checkpoint_index`;
  `candidate_lba`; `repair_actions/repair_outcome`; `check_errors/check_family`; `autosave_rc`.
- `cr_ui_stage_tel` (cr_ui.h:641-648): `permille` 0..1000 monotonic; `units_done/units_total`
  (engine work units, 0 total = unknown); `bytes_done` (only stages that count bytes = carve);
  `elapsed_ms` (stage wall, pause-compensated); `completed`.
- `cr_ui_telemetry` (cr_ui.h:650-658): `overall_permille` (weighted monotonic); `quick,deep,carve`
  stage_tel; `files_found,deleted_found,carved_found,lost_partitions`; `bytes_scanned`
  ("carve-true-bytes"); `elapsed_ms` pause-compensated; `error_ranges` live count;
  `paused,running,cancelled`.
- `cr_ui_scan_stats` (cr_ui.h:663-699): `files_found,deleted_found,carved_found,lost_partitions`;
  `bytes_scanned,elapsed_ms`; `cached_hits/cached_misses`; `error_ranges` final; `repair_ran/
  planned/applied/denied`; `check_ran/check_errors/check_detected`; `bytes_unreadable` (error-map
  coverage; Skip-mode zero-fill hides it from engines, never from this field), `probe_unreadable`;
  `autosaved`. (NOTE: core `ScanStats.bytes_found` — N-167 found-bytes — has NO ABI field.)
- `cr_ui_recovery_stats` (cr_ui.h:1478-1484): `queued,done,refused,errors,hash_mismatch,resumed,
  salvaged,dirs,nothing_to_recover` (all u64 counts).

## 2. The event stream

Wire values (cr_ui.h:151-223). Core emits 1-7 and 10 (cr_scan.h:135-151); 8/9/11/12 are shim-side.
- **1 CR_EVT_SCAN_STAGE**: stage transitions, in scan order, once each (tel_stage_begin_,
  cr_scan.cpp:450-466). stage = 0/1/2; counters ride via emit_event_.
- **2 CR_EVT_SCAN_PROGRESS**: coalesced progress. `emit_progress_event_` fires ONLY when overall
  permille advanced (CAS on `evt_last_overall_`; stale out-of-order worker deliveries dropped;
  cr_scan.cpp:623-642) ⇒ ≤ ~1000/scan + stage flips. bytes_done/bytes_total are stage-true bytes
  for stage 2 only (emit_event_ cr_scan.cpp:596-597), read from live `carve.bytes_done/units_total`.
  N-166: on each sample the live tree counters' delta over the boundary total is attributed to the
  active stage (`carved_found` for carve, `files_found` else, cr_scan.cpp:607-618).
- **3 CR_EVT_FILES_FOUND**: cumulative counters at stage boundaries (emit_boundary_events_
  cr_scan.cpp:644-648) — but N-166 live refresh means files/carved counters MOVE mid-stage
  (refresh_live_counts_ cr_scan.cpp:567-575); `deleted_found` stays boundary-only (engines assign
  deleted-state after attach; cr_scan.cpp:564-566).
- **4 CR_EVT_ERROR_RANGE**: delta = newest appended span (absolute device bytes) + cumulative
  count, with `err_module` (CR_ERRMOD_* 0..12, cr_ui.h:283-295), `err_operation` (CR_ERROP_READ),
  `err_attempts`. Fires only when the count grew (cr_scan.cpp:644-695).
- **5 CR_EVT_CHECKPOINT**: per committed G-13 checkpoint (emit_checkpoint_ cr_scan.cpp:858-873).
- **6 CR_EVT_LOST_CANDIDATE**: per discovered candidate, stage=1, `candidate_lba` (cr_scan.cpp:697-718).
- **7 CR_EVT_REPAIR**: outcome 0 plan-ready-for-consent / 1 applied / 2 denied / 3 failed-or-
  unavailable; `repair_actions` planned-or-applied count (cr_scan.cpp:1200-1245).
- **8 CR_EVT_HEARTBEAT** (shim): fired from the watchdog thread every 5000 ms while run() is in
  flight AND no CR_EVT_SCAN_PROGRESS arrived in the window (cr_ui.cpp:1106-1189); every progress
  arrival resets the clock (cr_ui.cpp:1095-1102). Payload = STANDING telemetry truth: stage
  0xFFFFFFFF or first-unclosed stage; `bytes_total=0` (honest unknown); `bytes_done =
  t.bytes_scanned` — the boundary-stamped stat (see §3); never a synthetic permille advance.
  Watchdog armed only when an event sink or memory_warn_mb exists (cr_ui.cpp:2364-2374).
- **9 CR_EVT_SAVE_PROGRESS** (shim): from the thread running `cr_ui_scan_save_session`; coalesced
  first-tick/permille-step/terminal; stage=0xFFFFFFFF, stage_permille = save 0..1000,
  overall=1000, bytes = nodes serialized/total (cr_ui.cpp:2828-2855).
- **10 CR_EVT_FS_CHECK**: once per scan after the check phase; `check_errors` count (0x1001..0x100e
  taxonomy), `check_family` ('f'/'n'/0) (cr_scan.cpp:1261-1270).
- **11 CR_EVT_SESSION_LOAD_PROGRESS** (shim): load twin of 9; overall=0xFFFFFFFF; cancel via the
  caller-owned `cancel_flag` polled per node (cr_ui.h:195-209).
- **12 CR_EVT_AUTOSAVED** (shim): once after run() completes (or the S-057 stop window) with
  `autosave_dir` set; `autosave_rc` = 0 or write error (cr_ui.cpp:2305-2312). Gates (S-044): an
  EMPTY run (no files, no lost partitions), a RESUMED run, or an explicitly-saved run never
  auto-save (cr_ui.cpp:2258-2260).

## 3. Scan lifecycle + telemetry semantics (cr_scan.cpp)

`ScanController::run` (cr_scan.cpp:1101-1588), in order: (a) M-042 disk-truth pass — capture live
table, compare vs persisted (layout_changed), revalidate restored lost candidates (whole-disk
resumes only) (:1125-1168); (b) telemetry reset (:1170-1177); (c) pre-scan repair S-008
(:1190-1247); (d) pre-scan check M-041 (:1257-1271); (e) `build_read_pipe` = slice → SliceShim →
RetryingDevice (3 attempts, Skip-mode zero-fill, error map) → CachedReader 8 MiB/256 KiB
(:1298, cr_scan.cpp:164-203); (f) probe_family + probe_unreadable truth (a recorded range covering
offset 0 = blind probe, :1312-1353); (g) volume facts + boot CRC32 (:1355-1389); (h) stages;
(i) junk filter M-018 (:1520), grading S-023 (:1527-1542); (j) read-truth gate: fully-blind or
nothing-read ⇒ REFUSES `ERR_READ_FAILED` instead of fake-completing (:1543-1586); (k) `stamp()`
finalizes stats/telemetry/events (:1391-1432).

**Stage sequence + weights.** Default weights 40/35/25 percent-of-total (cr_scan.h:72-76).
Quick = one `run_engine_pass` with deep settings when deep will run (single MFT walk,
cr_scan.cpp:1590-1610). N-181: when quick ran and carve is on, **carve runs BEFORE deep**
(`carve_before_deep = run_carve && quick_done_`, :1467, 1485-1487); with quick disabled the order
is deep → carve (:1488-1502). Deep = (engine pass if !quick_done_) + NTFS remnant sweep +
whole-disk lost discovery + candidate fan-out (insertion cap 10, M-137) (:2150-2446). Carve =
`run_carve_stage` with 1 MiB windows + 4 KiB overlap (cr_carve.cpp:45-46), occupancy + bitmap
gates, byte-unit progress (:2512-2681).

**weighted_progress** (cr_scan.cpp:131-146): exact single division
`Σ(w_i·pm_i)/Σw` with per-stage clamps ≤1000; 500/500/500 @ 40/35/25 = exactly 500. Overall is
kept monotonic: `tel_publish_`/`tel_stage_end_` only raise it (cr_scan.cpp:518-519, 481-482).

**Telemetry plumbing and the live-vs-boundary truth:**
- `tel_stage_begin_` (:450-466): zeroes the stage's StageTelemetry, stamps stage_t0/pause base,
  fires ScanStage.
- `tel_publish_(stage, done, total, bytes_units, cancel)` (:485-520): called from ENGINE WORKER
  THREADS via `engine_progress_bridge_` (:720-735) and the scan thread. Under `tel_.m`: permille
  = done*1000/total (monotonic clamp), `units_done/units_total` set when total≠0; **`bytes_done`
  set only when `bytes_units` — true only for stage 2** (:508-509); pause-compensated stage
  elapsed; recomputed overall.
- `tel_stage_end_` (:468-483): permille=1000, completed=true, elapsed final.
- `tel_counts_` (:522-549): copies `stats.files_found/deleted_found/carved_found/lost_partitions/
  bytes_scanned` into the snapshot + merges the LIVE retry-pipe error map. Called ONLY at stage
  ends and in `stamp()` (cr_scan.cpp:1601, 2440, 2675, 1401).
- **Fields that update LIVE mid-stage:** stage permille; quick/deep `units_done/units_total`
  (per engine progress callback); carve `bytes_done` + `units_done/units_total` (per carve
  window); stage elapsed; overall permille; `error_ranges` and the error map (telemetry() merges
  the live pipe on every read, cr_scan.cpp:744-759); `paused/running` (from the token);
  `files_found`/`carved_found` on EVENTS (N-166 live atomics, cr_scan.cpp:567-575).
- **Fields stamped ONLY at stage boundaries (the frozen-bytes defect):** snapshot `bytes_scanned`
  (`tel_.snap.bytes_scanned = stats.bytes_scanned`, cr_scan.cpp:535) — `stats.bytes_scanned` is
  incremented only when `carve_slice` returns (`stats.bytes_scanned += cs.bytes_scanned`,
  cr_scan.cpp:2662), so **`cr_ui_telemetry.bytes_scanned` is 0 for the entire carve and jumps to
  the final value at the carve boundary**; snapshot `files_found/deleted_found/carved_found/
  lost_partitions` (N-166 refreshes only the two live ones on the EVENT path, not the pull path);
  `deleted_found` everywhere (boundary-only by construction); `cr_ui_event.bytes_done` for
  HEARTBEAT (reads the frozen `t.bytes_scanned`, cr_ui.cpp:1177). Also **found-bytes never cross
  the ABI**: `ScanStats.bytes_found` (N-167, tree total_logical, stamped at every tel_counts_,
  cr_scan.cpp:529, cr_scan.h:579-586) exists only on the C++/CLI/JSON surface
  (rrecov.cpp:2118-2133); no cr_ui struct or event carries it.
- `stamp(rc)` (:1391-1432): elapsed pause-compensated; error map merged into stats on EVERY exit
  (save-on-cancel carries it); one boundary-event emit; under the telemetry lock finalizes the
  snapshot (cancelled flag, paused=false, running=false) and clears `tel_retry_`/`tel_cancel_`
  before the pipe/token can die.

**Pause/resume/cancel.** `PausableToken` (cr_device.h:156-242): `pause()`/`resume()`/`cancel()`
any thread; `checkpoint()` is the cooperative PAUSE+cancel point — blocks while paused, returns
cancelled; `paused_ms()` compensates elapsed. `cr_ui_scan_pause/resume/cancel` drive it
(cr_ui.cpp:2435-2452). **Checkpoint sites:** 43 `cancel->checkpoint()` polls across 13 core
files at engine loop heads (MFT walk ×3, FAT ×7, exFAT ×2, ext4 ×5, HFS+ ×6, Btrfs ×3, ZFS ×5,
ReFS ×1, carve ×2, lost candidates ×2, orchestrator ×6, safewriter ×1); between stages
(cr_scan.cpp:1433, 1469, 1489); per whole-disk table target (:2022). **Inside the carve:
per-window only** — serial loop head once per 1 MiB window (cr_carve.cpp:1530-1536) and once per
worker per window (:1550); there is NO checkpoint inside `scan_one_window` (no per-byte pause/
cancel — one window's magic-dispatch/validate runs to completion). `CancelPoolWatcher` polls the
token every 50 ms and aborts the pool so QUEUED windows/candidates never start (cr_parallel.h:105-127;
cr_carve.cpp:1579-1586; cr_scan.cpp:2391); running jobs finish to their own poll point — that is
the drain-after-cancel behavior. After cancel: run() returns ERR_CANCELLED with the tree intact;
`stamp()` merges/finishes telemetry; saving the session is the caller's explicit choice (CLI saves
when `--out` was given, exit 3, rrecov.cpp:2041-2065; shim auto-saves on the S-057 stop window if
the S-044 gates pass, cr_ui.cpp:2398-2413). **Session checkpoints (G-13)** exist only at stage
boundaries (cr_scan.cpp:1450, 1483, 1497) plus one forced full checkpoint when lost discovery
first produces candidates (:2281-2282); a failed checkpoint write ABORTS the scan with its error
(cr_scan.h:363-372); sinks stage to `<path>.tmp` + atomic rename (cr_ui.cpp:2327-2343).

## 4. CLI (`rrecov`, engine/src/app/rrecov.cpp)

Verbs (rrecov.cpp:4-52, dispatch :3912-3951): `list [--trim]`, `list-carve-formats`,
`sessions [--dir D] [--json]`, `scan`, `tree --in F [--filter s] [--deleted-only] [--csv] [--json]`,
`archive-list --in F (--disk N|--image f) --node ID [--json]`, `recover --in F (--disk N|--image f)
--to DIR [--deleted-only] [--filter s] [--report file.json]`, `health --disk N`,
`fix --disk N [--part P][--apply][--undo f][--undo-apply f]` / `fix --chkdsk C:`,
`unlock --turn-off C:`, `apply --disk N --session F [--candidate N][--apply][--undo f]
[--boot-fix lba][--boot-fix-bcd f]`, `bench --disk N [--mode seq|rand|open|read|info] ...`,
`image --create --disk N --out f.img` (+ .sha256 sidecar).

`scan` targets `--disk N | --volume C: | --image file` (vmdk/vhd/vhdx/img/dmg-raw). Flags:
`--whole-disk` (N-044: adds lost-partition discovery; table volumes are ALWAYS scanned — S-051),
`--deep` (deprecated alias for --whole-disk, prints a note, :1483-1491), `--quick-only`
(disables deep AND carve, :1493-1496), `--no-carve`, `--threads N`, `--out F` (also the G-13
checkpoint sink), `--resume F`, `--autosave-dir D`, `--label s`, `--log f`, `--json`, `--csv`,
`--filter s`, `--deleted-only`, `--pause-stdin` (N-178: 'p'/SPACE pause, 'r'/Enter resume),
`--filter-folder DIR` (repeatable, M-073), `--repair/--repair-apply/--repair-undo f/--check`,
`--bitlocker-fvek hex`, `--bitlocker-dict [f]`, `--user-signs f` (:1427-1600). Deep stage is
always on; --quick-only is the only way off (:1474).

Output: scan completion headline `"scan complete: N file(s) (X GB), D deleted, C carved, L lost
partition(s) in S s"` where GB = bytes_found/1024³ (rrecov.cpp:2126-2135); `--json` emits
{files, deleted, carved, lost_partitions, bytes_scanned, bytes_unreadable, bytes_found,
error_ranges, probe_unreadable, elapsed_ms} (:2116-2124); read-truth warnings always printed
(:2103-2114). `tree --csv/--json` the result tree; `sessions --json` the light list.

**Exit codes** (rrecov.cpp:54-68; docs/API.md:72-81): 0 ok; 1 failure (scan failure, unknown
option, enumeration failure); 2 recover-partial / apply+fix open-plan-execute-undo failure /
bench open-read failure or zero hits / unlock refusal; 3 scan-cancelled (session saved when
--out given) / health all-sources-unavailable / image cancelled (partial kept as `<out>.partial`);
4 the ELEVATION prerequisite not met (raw-disk verbs unelevated — image targets exempt); 5 scan
only: target disconnected mid-scan (N-125, reattach-then-resume, :2072-2083). Ctrl-C is
cooperative: token polls at loop heads, session saves, exit 3 (:3834-3837).

## 5. Health surface

`cr_ui_health_read(disk, rep, attrs, attr_cap, attr_count)` (cr_ui.h:1498-1538; impl
cr_ui.cpp:4879-4958): opens `\\.\PhysicalDriveN` GENERIC_READ|WRITE; open failure → ERR_OPEN_FAILED
with text distinguishing "admin rights required" vs "busy/locked even elevated" (:4889-4894).
Then, per disk:
- **ATA SMART** via `win::health::read_smart` (cr_health.cpp:216-262): transport chain tried in
  order — (1) `IOCTL_ATA_PASS_THROUGH` ATA_PASS_THROUGH_EX (:38-64), (2) legacy
  `SMART_RCV_DRIVE_DATA` 0x7C088 pre-Vista (:66-109), (3) CSMI CC-0x19 SCSI-miniport (Intel RST/
  AMD rcraid; IOCTL_SCSI_GET_ADDRESS → `\\.\ScsiN:`) (:135-208). First transport that answers
  IDENTIFY serves the command set. Fills available, transport name, ata/smart version, per-attr
  rows (id/value/worst/6-vendor-byte raw/failing/name), failing_now, reallocated/pending/
  uncorrectable sectors, power_on_hours, spin_retry, ssd_wear_level, temperature_c.
- **NVMe health log page 2** via IOCTL_STORAGE_QUERY_PROPERTY ProtocolTypeNvme (:264-299);
  fills nvme_available, temperature_c_x10, available_spare(+threshold), percentage_used, data
  units read/written, power_on_hours_nvme, unsafe_shutdowns, media_errors, critical_warning, and
  the M-036 tail (host_reads/writes, controller_busy_time, num_error_log_entries, data bytes =
  units × 512,000).
- **ssd_like** = seek-penalty heuristic `IOCTL_STORAGE_QUERY_PROPERTY` StorageDeviceSeekPenalty
  (:325-342). **TRIM** = `query_trim_state` ALWAYS called (cr_ui.cpp:4947-4951).
- Neither SMART nor NVMe available → `ERR_HEALTH_UNAVAILABLE` 0x50070001 (CLI health exit 3).

**The `_popen` (cr_health.cpp:306).** `query_trim_state` runs
`_popen("fsutil behavior query disabledeletenotify 2>nul", "r")`, parses the FIRST "= " line
(og parity; the HEALTH-6 note records last-line-wins ambiguity on localized output), returns
ERR_HEALTH_UNAVAILABLE when no line parses (:301-323). Call chain from the UI's health command:
`cr_ui_health_read` (cr_ui.cpp:4976) → `guarded(health_read_impl)` (:4879) →
`win::health::query_trim_state` (cr_ui.cpp:4948) → `_popen` (cr_health.cpp:306) — a console
child process per health read. The SAME primitive is invoked from the enumeration path when
`cr_ui_enumerate_disks_ex` is called with `CR_ENUM_FLAG_TRIM` (cr_ui.cpp:1845-1846; one query
per invocation, OS-global datum stamped on every row; failure ⇒ trim_known=0 everywhere) — the
CLI `list --trim` leg (rrecov.cpp:3912-3924).

## 6. Enumeration (cr_enumerate.cpp / cr_enumerate.h)

**Two-stack merge.** Stack 1: the numbered `\\.\PhysicalDrive0..max-1` loop (max 200 at the ABI,
cr_ui.cpp:1828) — open, query geometry/alignment/descriptor, parse the on-disk table through the
pure core (cr_enumerate.cpp:139-159). Stack 2 (N-071): the cfgmgr32 `GUID_DEVINTERFACE_DISK`
present-instance pass — number from IOCTL_STORAGE_GET_DEVICE_NUMBER (kUnknownDiskNumber 0xFFFFFFFF
when it fails), same shared query path (:168-212). `merge_disk_enumeration` (pure,
cr_enumerate.h:228-255): every loop disk kept; an interface entry appended only when its number is
absent from the loop set AND its instance path unseen; unknown-number entries dedup by path only.

**LISTED vs DROPPED.** `open_disk_for_query` (:98-113): data open is `GENERIC_READ` with full
share; on failure only **ERROR_ACCESS_DENIED (5)** and **ERROR_SHARING_VIOLATION (32)** fall
through to a **0-access metadata-only open** (FILE_ANY_ACCESS IOCTLs still answer) — disk stays
LISTED with `busy=true` (scans must re-open later). **Every other GetLastError value returns
INVALID_HANDLE ⇒ the disk is silently dropped** (query_disk `continue`, :119-124). Also dropped:
a handle on which `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` fails entirely (`query_disk_handle` ok=false,
:28-38, 121-124); interface entries that do not open at all (:186). A disk that cannot even be
metadata-opened stays absent (honest limit, D-85).

**Identity fields.** `removable` = bus type 7 USB / 11 SD / 12 MMC (STORAGE_DEVICE_DESCRIPTOR
BusType@+28, cr_enumerate.h:88-90); model@+16, firmware@+20, serial@+24 (N-080 SDK-exact decode,
:70-91); WWN from StorageDeviceIdProperty first device identifier (:141-178); interface text
`bus_interface_name` (:96-131). Volumes: `GetLogicalDriveStringsW`, only DRIVE_FIXED and
DRIVE_REMOVABLE, first extent via IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, GetVolumeInformationW
fs/label, `bitlocker::is_fve_volume` (cr_enumerate.cpp:214-254).

**Unelevated vs elevated.** Enumeration itself needs no elevation: the 0-access metadata open
works unelevated (the GENERIC_READ first attempt fails ERROR_ACCESS_DENIED unelevated, which is
exactly the retry path). But the table parse calls `open_physical_drive(n)` (cr_enumerate.cpp:148,
201) — a raw direct open that requires elevation; unelevated the view is null and `table_ok`
stays 0 (disks listed, layout unknown). Elevated: geometry + table + identity all filled. Scans
(`cr_ui_open_disk`), health reads (GENERIC_READ|WRITE open) and the CLI's raw-disk verbs refuse
unelevated with ERR_ELEVATION_REQUIRED / exit 4. Refresh contract: no cache anywhere — each
cr_ui_enumerate_* call re-queries the OS (cr_ui.h:434-446, N-098). Capacity semantics: `out==NULL`
probes *count; `out!=NULL` treats *count as IN capacity; short capacity fills the prefix and
returns ERR_TRUNCATED (cr_ui.cpp:1847-1878). `_ex` refuses unknown flag bits BEFORE the platform
check so all lanes see the same grammar (cr_ui.cpp:1932-1944).
