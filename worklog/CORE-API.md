# CORE-API.md — engine core CURRENT contract + defect deep-dives (Agent C, Phase-0)

Repo: `/home/z/my-project/work/final-prod` (engine in `engine/`). Research-only refresh of the
consumer-facing contract. Every claim carries file:line. Written against the CURRENT tree
(ABI header v4; **93 exports** — the task brief said 92; the 93rd is `cr_ui_health_read_ex2`,
cr_ui.h:1590).

---

## 1. ABI surface (engine/include/cr/cr_ui.h, ABI v4 = `CR_UI_ABI_VERSION`, cr_ui.h:81)

C11, no C++ types cross the boundary, every entry catch-all → `CR_UI_ERR_INTERNAL`
(cr_ui.h:23-27). Same numeric space as `cr_error.h` (§6). Strings: UTF-8 in, two-call
size pattern or `cr_ui_string_free` out. Zero-init structs always safe. Append-only
struct/event evolution; version bumps only on breaks (cr_ui.h:39-48).

### 1.1 Export groups (93 total)

| Group | Exports | Names |
|---|---|---|
| version/diagnostics | 3 | `cr_ui_abi_version` :105, `cr_ui_last_error` :110 (thread-local, stable until next call), `cr_ui_string_free` :113 |
| scan events | 1 | `cr_ui_scan_set_event_fn` :312 (subscribe BEFORE run; one sink/scan; NULL unsubscribes) |
| devices | 9 | `cr_ui_open_image` :322 (offset+bytes window, bytes==0→EOF), `cr_ui_open_disk` :339 (`\\.\PhysicalDriveN` direct; non-elevated → `ERR_ELEVATION_REQUIRED`), `cr_ui_open_volume` :340 (BitLocker probed first; locked → `ERR_FVE_LOCKED` unless auto-unlock), `cr_ui_last_transport_error` :349 (advisory, process-global, never clears), `cr_ui_open_volume_with_key` :361 (FVEK hex 64/128), `cr_ui_unlock_volume_with_dictionary` :377 (explicit consent only), `cr_ui_device_close` :381, `cr_ui_device_geometry` :390, `cr_ui_device_transport` :396 ("direct"/"image") |
| enumeration | 3 | `cr_ui_enumerate_disks` :467, `cr_ui_enumerate_disks_ex` :468 (+flags; only `CR_ENUM_FLAG_TRIM` 0x1 :452 — unknown bits → `ERR_BAD_PARAMETER`, refused pre-platform-check, cr_ui.cpp:1945), `cr_ui_enumerate_volumes` :469 |
| scan lifecycle | 18 | `cr_ui_scan_create` :629, `cr_ui_scan_close` :630, `cr_ui_scan_attach_device` :635 (borrow; identity card auto-fill on Windows, cr_ui.cpp:2059), `cr_ui_scan_run` :640 (synchronous, caller thread), `cr_ui_scan_pause`/`resume`/`cancel` :643-645 (any thread; cancel → run returns `ERR_CANCELLED`, tree stays browsable), `cr_ui_scan_busy`/`stopping` :658-659 (N-075 sync queries), `cr_ui_scan_telemetry` :687, `cr_ui_scan_get_stats` :727, `cr_ui_scan_assistant` :756, `cr_ui_session_regrade` :762, `cr_ui_scan_checkpoint_count` :764, `cr_ui_scan_repair_save_undo` :773, `cr_ui_scan_save_session` :851, `cr_ui_scan_save_cancel` :861 (SAVE-scoped), `cr_ui_scan_snapshot` :867, `cr_ui_scan_verify_identity` :875 (boot-hash; `ERR_BAD_CHECKSUM` on mismatch) |
| lost-partition apply | 3 | `cr_ui_apply_lost_partition` :834 (consent-gated, whole-disk views only), `cr_ui_apply_undo_last` :841, `cr_ui_apply_save_undo` :846 |
| sessions | 10 | `cr_ui_session_load` :883, `cr_ui_session_close` :884, `cr_ui_session_resume_last` :896 (last_scan.info pointer), `cr_ui_session_get_info` :914, `cr_ui_session_stats` :915, `cr_ui_session_snapshot` :916, `cr_ui_session_resume` :923, `cr_ui_session_list` :980 (newest-first, light records only), `cr_ui_session_last_autosave_path` :998, `cr_ui_session_load_with_events` :1018 (caller-owned `cancel_flag`) |
| snapshots | 5 | `cr_ui_snapshot_close` :1064, `node_count` :1065, `partial` :1066, `node` :1072, `dfs` :1078 |
| query | 3 | `cr_ui_query_run` :1142, `cr_ui_query_rows_free` :1145, `cr_ui_query_apply_buckets` :1164 (og menu IDs; unknown → `ERR_BAD_PARAMETER`) |
| carve registry | 6 | `cr_ui_carve_format_count` :1193, `carve_format_info` :1196, `cr_ui_scan_carve_tally_count` :1216, `cr_ui_scan_carve_tally` :1217, `cr_ui_register_user_signs` :1235, `cr_ui_user_sign_count` :1246, `cr_ui_user_sign_info` :1247 |
| disk map | 2 | `cr_ui_disk_map` :1290 (one of sess/snap; overlays freed individually), `cr_ui_disk_map_free` :1298 |
| preview | 8 | `cr_ui_preview_open_snapshot` :1310, `open_session` :1312, `close` :1314, `size` :1316, `read` :1322 (≤4 MiB/call; zero-fill + report), `metadata` :1340, `cr_ui_archive_entry_count` :1366, `archive_entry_at` :1367, `preview_unreadable` :1372 |
| thumbnail | 2 | `cr_ui_thumbnail_generate` :1441 (two-call; grade-2 promotion M-115), `cr_ui_thumbnail_format_supported` :1448 |
| recovery | 12 | `cr_ui_recovery_create` :1468, `close` :1470, `set_destination` :1477, `enqueue_node` :1484, `enqueue_subtree` :1486, `enqueue_snapshot` :1490, `enqueue_subtree_snapshot` :1492, `queue_count` :1495, `process` :1501, `cancel` :1502, `get_stats` :1512, `report_json` :1517 |
| health | 2 | `cr_ui_health_read` :1574, `cr_ui_health_read_ex2` :1590 (BF-03 threshold twin) |
| image/fsck/bitlocker | 3 | `cr_ui_image_create` :1611 (raw flat + .sha256; partial kept on cancel), `cr_ui_fsck_volume` :1632 (chkdsk; 15-min bound; exit codes −1 spawn-fail / 259 timeout), `cr_ui_turn_off_bitlocker` :1650 (consented decrypt; OK = accepted, not plaintext) |

### 1.2 Callbacks
- `cr_ui_progress_fn` (cr_ui.h:125): `(user, stage, stage_permille, overall_permille, bytes_done, bytes_total, error_ranges)`. stage 0 quick / 1 deep / 2 carve / 0xFFFFFFFF none. Fired on worker threads only, must not block, NO cancellation semantics (token functions are the cancel path).
- `cr_ui_recovery_progress_fn` :130: cumulative bytes.
- `cr_ui_event_fn` :283: POD `cr_ui_event` (cr_ui.h:225-281). Event types: `CR_EVT_SCAN_STAGE` 1, `SCAN_PROGRESS` 2, `FILES_FOUND` 3, `ERROR_RANGE` 4, `CHECKPOINT` 5, `LOST_CANDIDATE` 6, `REPAIR` 7, `HEARTBEAT` 8 (shim watchdog thread, standing truth — never synthetic progress, cr_ui.cpp:1111-1194), `SAVE_PROGRESS` 9, `FS_CHECK` 10, `SESSION_LOAD_PROGRESS` 11, `AUTOSAVED` 12. Unknown types MUST be ignored.
- `cr_ui_repair_consent_fn` :523 and `cr_ui_apply_consent_fn` :815: default-deny (NULL/0 ⇒ nothing written).

### 1.3 Key structs (field-level, units)
- `cr_ui_disk_info` (cr_ui.h:401-436): `number` (PhysicalDriveN), `size_bytes`, `logical/physical_sector`, `removable` (descriptor RemovableMedia@+10 ∨ bus∈{USB,SD,MMC}, cr_enumerate.h:118-119), `layout` (0/1 MBR/2 GPT/3 EBR), `table_ok`, `model[128]`, `serial[128]`, `trim_known`/`trim_on` (M-076, _ex+flag only), **`bus_type`** (D-ENUM-02 append; SDK STORAGE_BUS_TYPE raw value, 0 unknown).
- `cr_ui_volume_info` :438-446: letter, disk, offset_on_disk, bytes, fs_name[16], label[64], bitlocker.
- `cr_ui_scan_config` :525-627: `stage_mask` (0 = 7 = quick|deep|carve; bits 1/2/4), `whole_disk` (lost-partition discovery; needs disk view), `fs_hint` (0 auto; serialized FsFamily values — see cr_scan.h:37-70), `quick/deep/carve_weight` (0 = 40/35/25 percent-of-total), `threads` (0 = auto hw−1 clamp 1..8, cr_scan.cpp:58-68), label/model/serial/firmware/wwn/interface overrides, `checkpoint_path_utf8`, `progress`+user, `pre_scan_repair`+consent (S-008), `memory_warn_mb` (N-062, 0 off), `raw_category_name_utf8`, `pre_scan_check` (M-041), `autosave_dir_utf8` (W1c), `filter_folders_utf8[]` (M-073).
- `cr_ui_stage_tel` :661-668: permille 0..1000, units_done/total (engine work units; 0 total = unknown), bytes_done (carve only), elapsed_ms pause-compensated, completed.
- `cr_ui_telemetry` :670-684: overall_permille weighted monotonic; per-stage tel; files/deleted/carved/lost counters; `bytes_scanned` = **carve-true bytes only** (D4a: live during carve = base+done clamped); `bytes_found` (D4b live tree logical sum); paused/running/cancelled.
- `cr_ui_scan_stats` :689-725: final counters + `cached_hits/misses`, `error_ranges`, repair_*, check_*, `bytes_unreadable` (read-truth), `probe_unreadable`, `autosaved`.
- `cr_ui_node_info` :1030-1062: tree row (parent/child/sibling indices, node_id, fs_kind, origin 0..3, deleted 0..3, confidence 0..4, grade 0..2, sizes, 100ns-1601 times, DOS attributes, extents count, carved provenance, sha128, version_group, tags_extracted).
- `cr_ui_query` :1098-1127 / `cr_ui_query_row` :1129-1140: category bits 1..8, size/time ranges, extensions, name/path filters, state_mask (M-072), origin_mask, sort 0..7, dir, offset/limit (0 = 100), count_only.
- `cr_ui_smart_report` :1540-1570: `available` (ATA table decoded), `transport[24]` ("ata-passthrough"/"smart-rcv-drive-data"/"csmi-miniport"/"nvme"), attribute rows, failing_now, derived counters (attrs 5/197/198/9/10/231|233/194|190), NVMe block (nvme_available, temperature_c_x10, spares, percentage_used, data units, host r/w, busy time, error log, power cycles), `ssd_like` (seek-penalty heuristic cr_health.cpp:461-478), `trim_known/trim_enabled` (per-device, `query_trim_state_handle` — no subprocess, cr_health.cpp:431-459).

### 1.4 Lifecycle / threading contract
- One `cr_ui_scan` = one staged pipeline over one borrowed device view; run() synchronous on caller thread; pause/cancel/telemetry/snapshot any-thread (cr_ui.h:482-490).
- run() is single-shot: second run → `ERR_BAD_PARAMETER` (cr_ui.cpp:2328-2329).
- Session handles are stable trees (no engine running); queries/previews/recovery enqueue directly.
- Progress callbacks: worker threads, non-blocking; event sink: worker threads OR the shim heartbeat watchdog thread (cr_ui.h:140-150). The shim does NOT serialize event deliveries (single-flight is the host's job).
- Pause: `PausableToken` (cr_ui.cpp:837); cooperative; elapsed pause-compensated (cr_scan.cpp:1425-1430).
- Cancellation: `cr_ui_scan_cancel` → token → run() returns `CR_UI_ERR_CANCELLED`; auto-save still fires on the stop window if results exist (S-057, cr_ui.cpp:2406-2421).

---

## 2. CLI — `rrecov` (engine/src/app/rrecov.cpp, entry `wmain` :3838+)

Subcommands (usage block rrecov.cpp:3840-3908; dispatch :3911-3952):
- `list [--trim]` — disk table (+TRIM chip; fsutil, one spawn per run). Exit 1 on enumeration failure (FB-K01).
- `list-carve-formats` — ABI registry projection.
- `sessions [--dir D] [--json]` — session list; only enumeration failure exits 1.
- `scan --disk N | --volume C: | --image f.vmdk/.vhd/.vhdx/.img/.dmg` `[--deep] [--whole-disk] [--quick-only] [--no-carve] [--threads N] [--out f.crsf] [--resume f.crsf] [--log f] [--autosave-dir D] [--label s] [--json] [--csv] [--filter s] [--deleted-only] [--pause-stdin] [--filter-folder DIR]… [--repair] [--repair-apply] [--repair-undo f] [--check] [--bitlocker-fvek hex] [--bitlocker-dict [f]] [--user-signs f]` (parse :1429-1590). Deep default ON; `--quick-only` disables.
- `tree --in f.crsf [--filter s] [--deleted-only] [--csv]` :3876.
- `archive-list --in f.crsf (--disk N|--image f) --node ID [--json]` :3877.
- `recover --in f.crsf (--disk N|--image f) --to DIR [--deleted-only] [--filter s] [--report f.json]` :3879.
- `health --disk N` :3882 (prints ATA attrs + NVMe block + TRIM).
- `unlock --turn-off C:` :3895 (M-078).
- `fix --disk N [--part P] [--apply] [--undo f] [--undo-apply f] | fix --chkdsk C:` :3891.
- `apply --disk N --session f.crsf --candidate N [--apply] [--undo f] [--boot-fix lba [--boot-fix-bcd path]]` :3898.
- `image --create --disk N --out f.img` :3883.
- `bench --disk N [--mode seq|rand|open|read|info] ...` :3902.

Exit codes (tally): 0 success; 1 usage/engine error (dominant); 2 second failure class
(e.g. destination/report errors); **3 = cancelled** (Ctrl-C: session saved, distinct from
failure, rrecov.cpp:3834-3838); **4 = elevation required** (raw-disk verbs, :3905-3908);
5 (one site). Output: human text + `--json`/`--csv` machine modes per subcommand.

---

## 3. DISK ENUMERATION subsystem (audit)

Files: `engine/include/cr/win/cr_enumerate.h` (311 ln), `engine/src/win/cr_enumerate.cpp` (328 ln).

**Physical disks** — two stacks, pure merge:
1. Numbered namespace loop `\\.\PhysicalDrive0..max_disks-1` (cr_enumerate.cpp:174-188; callers pass 200 = `ENUMERATE_DISK_BOUND`, cr_winenv.h:45; cr_ui.cpp:1833, rrecov.cpp:795).
   - `open_disk_for_query` (:99-120): `CreateFileW` GENERIC_READ + full share; on ACCESS_DENIED/SHARING_VIOLATION retries a **0-access metadata open** (FILE_ANY_ACCESS IOCTLs still answer) and marks `busy` (N-071 — a backup/AV-held disk stays listed).
   - `query_disk` (:137-162): open failure is a namespace gap ONLY for FILE_NOT_FOUND / PATH_NOT_FOUND / INVALID_NAME; every other failure (NOT_READY, IO_DEVICE, GEN_FAILURE, denied) = PRESENT-but-unopenable → **listed with honest empties + busy** (D3 fix, :122-136). Geometry failure after open → still listed (D3, :153-159).
   - Per-disk queries: `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX`, `StorageAccessAlignmentProperty`, `StorageDeviceProperty` (descriptor decode at SDK offsets product@+16/rev@+20/serial@+24/bus@+28/removable@+10 — `decode_device_descriptor`, cr_enumerate.h:95-121, N-080 + D-ENUM-02), `StorageDeviceIdProperty` → WWN (cr_enumerate.h:185-222). Table parse via `open_physical_drive` + pure `parse_disk_table` (:183-186) — non-elevated ⇒ table_ok=false, disk still listed.
2. cfgmgr32 interface pass `GUID_DEVINTERFACE_DISK` (`enumerate_disk_interfaces`, :205-263): CM_Get_Device_Interface_List(PRESENT); every instance opened with the same share policy; number via `IOCTL_STORAGE_GET_DEVICE_NUMBER` (else `kUnknownDiskNumber` = 0xFFFFFFFF, dedup by path); unopenable → still listed (D3).
3. `merge_disk_enumeration` (cr_enumerate.h:272-299, pure): loop disks kept; interface-only appended when number/path unseen.

**Bus-type decode** (cr_enumerate.h:130-175): SDK STORAGE_BUS_TYPE order, corrected by D-ENUM-02
(pre-fix table was shifted-by-one from bus 11 up: SATA labeled "SD", SD labeled "MMC").
Removable classification: descriptor flag OR bus ∈ {7 USB, 12 SD, 13 MMC} (:118-119).

**Volumes** — `enumerate_volumes` (:265-325): `GetLogicalDriveStringsW` (512-wchar stack + exact-size heap retry — E-01 fix), filter `DRIVE_FIXED|DRIVE_REMOVABLE` only (:296; DRIVE_REMOTE/RAMDISK excluded by design), `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` (first extent) → disk correlation, `GetVolumeInformationW` label/FS, `bitlocker::is_fve_volume`.

**Where a disk could still be dropped on a real machine (2 internal + 1 SD):**
- Engine: I found **no remaining silent drop** in the enumerate path itself — D3/N-071/D-ENUM-01/02 landed. Residual engine-side nits: (a) numbered loop capped at 200 (fine); (b) volumes limited to FIXED/REMOVABLE drive types; (c) an empty card reader with no media usually exposes no PhysicalDriveN at all (OS-level, not ours).
- **Interface gap**: `fill_identity_card` (cr_ui.cpp:2059-2074; CLI twin rrecov.cpp:417-428) does `enumerate_disks(disk + 1)` then `disks[disk]` — it assumes **list index == disk number**. With a numbering gap (e.g. disk 1 absent, disks = [0,2]) the bound check `disks.size() <= disk` spuriously fails and `disks[disk]` would pick the WRONG disk when size happens to exceed index. Display-only (identity card), but it is the one remaining index/number conflation.
- **ABI drift (top candidate)**: the Tauri bridge's `CrUiDiskInfo` (ui/src-tauri/src/bridge/abi.rs:174-189) **lacks `bus_type`** and its comment claims "BF-02: v4 carries NO bus_type" (abi.rs:186; devices.rs:210-214 repeats it). The engine DID append `bus_type` (cr_ui.h:425-435, D-ENUM-02). Consequence: `list_devices` maps `kind = removable ? "usb" : "internal"` (devices.rs:215) — **an SD card is misclassified as "usb"** and renders in "External drives" with USB kind; contract.ts:28 defines `'sd'` but nothing can produce it. A user with 2 internal + 1 SD sees the SD in the wrong group/kind and reports it "not visible".

---

## 4. HEALTH/SMART subsystem (audit)

Files: `engine/include/cr/win/cr_health.h` (113 ln), `engine/src/win/cr_health.cpp` (482 ln),
pure seam `cr_smart_ata.cpp` (223 ln) + `cr_nvme.cpp` (54 ln).

**Query mechanism — no WMI, no per-disk PowerShell.** Per call (cr_ui.cpp:4888-4991):
1. Open `\\.\PhysicalDriveN` GENERIC_READ|GENERIC_WRITE (cr_ui.cpp:4894-4897). Non-elevated ACCESS_DENIED → `ERR_ELEVATION_REQUIRED` (:4910-4916, BF-28); other failures → `ERR_OPEN_FAILED`.
2. **ATA SMART** — `read_smart` (cr_health.cpp:216-262) over a 3-transport chain, first that answers IDENTIFY (0xEC) serves the set (cr_smart_ata.cpp:104-116):
   - `IOCTL_ATA_PASS_THROUGH` (ATA_PASS_THROUGH_EX verbatim, cr_health.cpp:38-64);
   - `SMART_RCV_DRIVE_DATA` 0x7C088 legacy with og-exact IDEREGS SMART magic (4F/C2, :73-109);
   - CSMI CC-0x19 SCSI-miniport (IOCTL_SCSI_GET_ADDRESS → `\\.\Scsi%u:`; Intel RST/rcraid route, :143-208).
   Then READ DATA (0xB0/D0), READ THRESHOLDS (0xD1, non-fatal), 30×12 attr table parse, derived counters, model string, S-024 grading (cr_smart_ata.cpp:118-219).
3. **NVMe** — `read_nvme_health` (cr_health.cpp:264-355): `IOCTL_STORAGE_QUERY_PROPERTY` + `StorageDeviceProtocolSpecificProperty`, protocol data at `offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)` == 8 (D-NVMe-01 fix with static_asserts :300-317), log page 2 (health), 512-byte payload re-derived from the ANSWER's `ProtocolDataOffset` (:339-350); pure parse at spec offsets (cr_nvme.cpp:23-51, F-A4-17/M-036/N-009a fixes).
4. `looks_like_ssd` (seek-penalty, cr_health.cpp:461-478); `query_trim_state_handle` (per-device `StorageDeviceTrimProperty`, NO subprocess, :431-459).
5. If neither ATA nor NVMe answered: **`ERR_HEALTH_UNAVAILABLE`** (cr_ui.cpp:4986-4989) with the dual rc in the message.

**Disk selection**: caller passes the PHYSICAL DRIVE NUMBER (`cr_ui_disk_info.number`); handle opened fresh per call and closed per call (cr_ui.cpp:4894/4985) — **no handle reuse, no per-disk cache**. The Tauri bridge passes `disk-{number}` parsed from the device id built as `disk_id(d.number)` (devices.rs:209, health.rs:21-29) — index/number mismatch is NOT present on this leg.

**Behavior by class**: SATA → ATA table (any of 3 transports); NVMe → NVMe log (no ATA); USB/UASP → ATA passthrough usually refused by the bridge stack → ERR_HEALTH_UNAVAILABLE (no SAT layer in this engine); **SD/MMC → no SMART, no NVMe → ERR_HEALTH_UNAVAILABLE** (honest refusal; the UI renders the "not available" EmptyState, health-page.tsx:215-222).

**Subprocess spawns in the whole engine (3 sites, all direct CreateProcessW, CREATE_NO_WINDOW, no cmd.exe):**
1. `win::health::query_trim_state` — `fsutil behavior query disabledeletenotify` (cr_health.cpp:392-394; 10 s bounded wait; OS-global datum; only on `CR_ENUM_FLAG_TRIM` / `list --trim` / `health`).
2. `win::fsck::fsck_volume` — `%SystemRoot%\System32\chkdsk.exe` (cr_fsck.cpp:50; command built cr_fsck_launch.cpp:23; 15-min bound).
3. `win::bitlocker::unlock/turn_off` — `powershell -NoProfile -NonInteractive Unlock-BitLocker/Disable-BitLocker` primary, `manage-bde` fallback (cr_bitlocker.cpp:64, :358-381; consent-gated only).

---

## 5. SCAN PIPELINE (audit)

Files: `engine/include/cr/orchestrator/cr_scan.h` (967 ln), `engine/src/core/orchestrator/cr_scan.cpp` (3234 ln), carve `cr_carve.cpp` (1725 ln), pool `cr_parallel.cpp` (181 ln), remnant sweep `cr_mftwalk.cpp:2342+`.

**Pipeline** (run(), cr_scan.cpp:1132-1622):
1. Search-start disk-truth pass (layout compare + lost-candidate revalidation, :1156-1199).
2. Optional pre-scan repair (S-008, consent-gated, :1213-1281) and damage-audit check (M-041, :1283-1305).
3. Read pipe: slice → SliceShim → RetryingDevice (3 attempts, Skip-mode zero-fill; error map) → CachedReader (8 MiB LRU, 256 KiB blocks; mutex-guarded, cr_cached.h:81; degraded bare-slice fallback pinned, cr_scan.cpp:164-203).
4. FS probe (`probe_family`) with blind-probe detection (`probe_unreadable`, :1346-1387); boot-sector CRC32 identity (:1411-1423).
5. **Stage order (default mask 7)**: quick → **carve → deep** when quick completed (`carve_before_deep = run_carve && quick_done_`, :1501; N-181 — og starts RAW controllers before deep). If quick disabled: quick-skipped → deep(engine pass) → carve.
6. Per stage: `tel_stage_begin_` → engine work → `tel_stage_end_` + `tel_counts_` + `emit_boundary_events_` + folder-filter prune + checkpoint emit (stage boundaries; a failed checkpoint write ABORTS the scan).
7. Post: FilterFolder junk pass, S-023 grading, **read-truth gate** (fully-blind or nothing-read scan REFUSES `ERR_READ_FAILED` instead of fake-completing, :1577-1620).

**Stage weights & the 75%**: quick 40 / deep 35 / carve 25 (cr_scan.h:72-76); `weighted_progress` = Σ(wᵢ·pmᵢ)/100 (cr_scan.cpp:131-146). Boundaries with the default order: quick ends at **40%**, carve ends at **65%**, deep carries 65→100%. **Overall 75% = mid-DEEP** (deep permille ≈ 285/1000). The UI's "Advanced Scanning {pct}" label renders for stage `deep-walk` OR `carve` (contract.ts:171-175, scan-progress-page.tsx:186) — so "75% advanced scanning" = the deep stage, post-carve.

**Progress reporting contract**:
- Engine→orchestrator: per-engine `progress(done, total)` callbacks at engine cadence; carve counts BYTES (window fan-out, per-window tick, cr_carve.cpp:1650-1655); deep's bridge passes `bytes_units=false` (cr_scan.cpp:2208) — **the deep stage contributes NO byte axis by design**.
- `tel_publish_` (cr_scan.cpp:485-530): monotonic permille, live `bytes_scanned = base + done` during carve (D4a).
- Events coalesce: fire when overall permille advances OR bytes moved ≥16 MiB (cr_scan.cpp:642-673). `bytes_found` (D4b) rides PROGRESS and FILES_FOUND from the tree's atomic `live_logical` (cr_node.cpp:226; refresh cr_scan.cpp:581-593).
- The cr_ui shim adds the 5 s **heartbeat watchdog** (cr_ui.cpp:1142-1194): standing truth only, `CR_EVT_HEARTBEAT`, stage 0xFFFFFFFF, bytes_total 0.
- The bridge polls `cr_ui_scan_telemetry` at every emitted event (events.rs:364-403) and maps stage/permille/files/bytes to the UI payload; the frontend renders "Reading sector: X/Y" from `bytesScanned/bytesTotal` (scan-progress-page.tsx:131-132).

**Where "75% advanced, 21 files (0 B), no sector progress, stuck" comes from** — see §8.3.

**0-byte files**: tree `live_logical`/`total_logical` sum `props->logical_size` (cr_node.cpp:226, :341-346). Nodes attach with size 0 when the engine could not recover a size/content: NTFS MFT **remnants** whose $DATA is compressed or absent stay metadata-only with size 0 (cr_mftwalk.cpp:1770-1780, GAP-S6-4 honest refusal); FAT/exFAT deleted entries with zeroed size fields; skeleton/category FOLDERS are attach_folder'd and do NOT tick live_files (cr_node.cpp:220 is add_file-only).

**Results tree / completion**: merged ResultTree per scan; roots pre-created; grading pass stamps tree::Grade; completion autosaves when `autosave_dir` set + results found + not resumed + not already saved (S-044 gates, cr_ui.cpp:2266-2322); stop-window (cancel) autosaves too (S-057, cr_ui.cpp:2406-2421).

---

## 6. Error model

`engine/include/cr/cr_error.h` (enum `cr::Err`, u32): open/transport 0x5002 0001-0007 (`OPEN_FAILED`, `READ_FAILED`, `WRITE_FAILED`, `RANGE_OUT_OF_BOUNDS`, `CANCELLED` 0x50020005, `RETRY_EXHAUSTED`, `DEVICE_DISCONNECTED` N-125); parse 0x5003 0001-0009 (`BAD_MAGIC`, `BAD_CHECKSUM`, `BAD_PARAMETER` 0x50030003, `TRUNCATED`, `FIXUP_FAILED`, `CYCLE_LIMIT`, `DEPTH_LIMIT`, `UNSUPPORTED`, `NO_MEMORY`); session 0x5004 0001-3; recovery-writer 0x5005 0001-8 (`DEST_OVERLAP`, `DEST_UNWRITABLE`, `HASH_MISMATCH`, `DEST_FULL`, `DEST_NAME_EXHAUSTED`, path/network taxonomy); FVE 0x5006 0001-2 (`FVE_LOCKED`, `FVE_ELEVATION_REQUIRED`); health/repair 0x5007/0x5008 (`HEALTH_UNAVAILABLE` 0x50070001, repair refused/undo/volume-lock, apply consent/stale/refused/refresh); image 0x500A0001; `ERR_INTERNAL` 0x50090001; `ERR_ELEVATION_REQUIRED` 0x500C0001 (session-96 direct-arch single precondition; the 0x500B driver family was removed).

Propagation: engines → orchestrator rc (stage aborts run, stats stamped via `stamp()` cr_scan.cpp:1425-1466) → cr_ui shim `guarded()` catch-all → u32 + thread-local `cr_ui_last_error`. CLI maps to exit codes (§2).

**Where errors get swallowed (deliberate, documented):**
- Retry layer Skip-mode: bad reads return zero-filled OK to engines; truth preserved in the error map / `bytes_unreadable` / ErrorRange events (cr_scan.h:484-502).
- Candidate-mode engine failures never abort a scan (run_engine_dispatch cr_scan.cpp:1800, :1906; table-volume engine errors counted `table_volumes_skipped`, :2123-2130).
- Unreadable carve windows: skipped, not counted as scanned (cr_carve.cpp:1439-1445).
- Repair/fscheck outcomes never abort the scan (S-008/M-041).
- Shim: `ret_()` always fills last_error; `autosave` pointer write is best-effort (cr_ui.cpp:2311).
- Bridge event channel full → try_send drop (events.rs:52) — progress coalesces by design; a stalled UI can drop events.

---

## 7. Threading / cancellation

- Worker model: fixed `ThreadPool` (cr_parallel.h:43-87; auto hw−1 clamp 1..8, cr_parallel.cpp:13-20; cap 64). Used by: carve window fan-out (cr_carve.cpp:1631), remnant sweep chunks (cr_mftwalk.cpp:2376), lost-candidate scans (cr_scan.cpp:2412), recovery writer. `parallel_for` blocks the caller, captures exceptions per item → `ERR_INTERNAL` (cr_parallel.cpp:104-150). `CancelPoolWatcher` polls the external token every 50 ms and aborts the pool so queued work never starts post-cancel (cr_parallel.cpp:152-179).
- Cancellation: `CancelToken`/`PausableToken` polled at engine loop heads and per-4 KiB carve candidate (D4c, cr_carve.cpp:622-633); pause freezes workers at gates (cr_carve.cpp:1639).
- Progress cadence: permille-step or 16 MiB byte-step coalescing (cr_scan.cpp:646-672); remnant sweep ticks every 4 MiB (D4e, cr_mftwalk.cpp:2328-2340); heartbeat every 5 s when no progress in window.
- Locking: telemetry mutex guards all snapshot mutation (cr_scan.cpp:490); CachedReader mutex (cr_cached.h:81) + RetryingDevice map mutex (cr_retry.h:77) make the shared pipe worker-safe; event emitters never fire under a core lock (cr_scan.cpp:565-567).

---

## 8. DEFECT DEEP-DIVE

### 8.1 Defect 1 — "not all disks visible (2 internal + 1 SD, only some shown)"

Call chain: `usePhysicalDevices` (ui/src/hooks/use-physical-devices.ts:21) → `list_devices` (ui/src-tauri/src/commands/devices.rs:107) → `enumerate_disks()` (bridge/util.rs:156) → `cr_ui_enumerate_disks` (cr_ui.cpp:1937→1829) → `win::enumerate_disks(200)` (cr_enumerate.cpp:174: numbered loop :177 + cfgmgr pass :192 + merge :193) → rows to `CrUiDiskInfo` (cr_ui.cpp:1854-1884) → bridge JSON (devices.rs:208-229, `kind` at :215) → home page groups internal vs non-internal (home-page.tsx:83-90, renders external group :276-298).

Ranked candidates:
1. **[UI bridge ABI drift — HIGH] `bus_type` missing from the bridge's `CrUiDiskInfo`** (abi.rs:174-189; stale claims abi.rs:186 + devices.rs:210-214). Engine appends `bus_type` (cr_ui.h:425-435) and fills it (cr_ui.cpp:1863-1865); the bridge never reads it, so `kind` degrades to `removable→"usb"` (devices.rs:215). The SD card renders in "External drives" labeled USB — and `DeviceKind 'sd'` (contract.ts:28) is unreachable. To a user with 2 internal + 1 SD this reads as "the SD is missing/wrong". Fix: add `bus_type: u32` to the Rust struct (append-only, safe) and map 12/13 → "sd".
2. **[Engine latent — LOW] index/number conflation in `fill_identity_card`** (cr_ui.cpp:2059-2062; rrecov.cpp:419-422): `enumerate_disks(disk+1)` + `disks[disk]` breaks when PhysicalDrive numbering has gaps (wrong-disk identity or silent no-card). Display-only today.
3. **[Engine — VERY LOW] residual enumerate drops**: none found — D3 (cr_enumerate.cpp:122-162) keeps present-but-unopenable, N-071 (:87-120) keeps share-blocked, E-01 (:265-290) fixes the volume-list overflow. Only DRIVE_FIXED/REMOVABLE volumes are correlated (:296) — RAM/network volumes never appear (by design).
4. **[Environment — check] non-elevated process**: disks still list but every `cr_ui_open_disk` probe returns `ERR_ELEVATION_REQUIRED` → all rows `status:"permission-denied"` (devices.rs:84-90) + banner (home-page.tsx:203-241). If the user runs unelevated, scans also fail — but disks remain visible, so this is not the "missing" symptom.

### 8.2 Defect 2 — "SMART only shows for the FIRST disk; switching to others shows nothing"

Call chain: health page (health-page.tsx:51 `useHealth(deviceId)`, per-device query key, queries/health.ts:7) → `health_read` cmd (health.rs:19) → parse `disk-{n}` (:21-29) → `cr_ui_health_read(n,…)` (cr_ui.cpp:5010) → `health_read_impl` (cr_ui.cpp:4888): open `\\.\PhysicalDriveN` R/W (:4894-4897, elevation gate :4910-4916) → `read_smart` (cr_health.cpp:216, 3-transport ATA chain) → `read_nvme_health` (cr_health.cpp:264, D-NVMe-01-fixed layout) → `looks_like_ssd` + per-device TRIM → refuse `ERR_HEALTH_UNAVAILABLE` when both sources absent (cr_ui.cpp:4986-4989) → bridge maps to HEALTH_UNAVAILABLE (health.rs:43-48) → EmptyState "not available" (health-page.tsx:215-222).

Ranked candidates:
1. **[CASCADE — HIGH] Defect 1 narrows the health page's device list**. The health page's picker only offers devices the device list returned (usePhysicalDevices). If internal disk #2 is absent/misclassified there, the only "other" disk is the SD card — which has NO SMART and NO NVMe → the engine's honest `ERR_HEALTH_UNAVAILABLE` renders as "nothing". Expected-behavior gap, not an engine bug: the og shows a graceful "no SMART on this device" state; ours shows an error EmptyState.
2. **[UI bridge ABI drift — HIGH, memory-unsafe] `CrUiSmartReport` in the bridge is 8 bytes short** (abi.rs:648-682 lacks the trailing `power_cycles_nvme`, cr_ui.h:1569): the engine's `std::memset(rep, 0, sizeof(*rep))` (cr_ui.cpp:4892) writes 8 bytes past the Rust stack object in `spawn_blocking` — UB; field reads are at correct offsets (append-only tail) so symptoms would be sporadic stack corruption, a plausible contributor to flaky "shows nothing" behavior. Related: bridge abi.rs:20 asserts `health_read_ex2` is GONE while the engine ships it (cr_ui.h:1590) — the two sides disagree about the v4 surface (93 vs "92-export" abi.rs:684).
3. **[Engine — verify on hardware] NVMe second disk**: the D-NVMe-01 buffer-placement fix is present and static_asserted (cr_health.cpp:300-317). Remaining failure modes on real NVMe: descriptor `Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)` (:340-342), `ProtocolDataLength < 512` (:344-346) — some older stornvme stacks return exactly these shapes; the ATA chain also burns time on IDENTIFY x3 before NVMe is tried (no bus-type short-circuit: the engine could route NVMe-first when `bus_type==17`). Also `CreateFileW` GENERIC_WRITE on a disk with mounted volumes can be refused by filter drivers → `ERR_OPEN_FAILED` for THAT disk only (works for disk 0, fails for disk 1 depending on which volumes are mounted).
4. **[Engine — expected] USB/SCSI disks**: no SAT translation layer ⇒ `ERR_HEALTH_UNAVAILABLE` by design (only ATA passthrough/legacy/CSMI + NVMe exist).

No handle reuse, no cross-call cache, no per-disk capability probe — each call is independent; "first works, rest don't" cannot come from shim state.

### 8.3 Defect 3 — "scan jumps to 75% 'advanced scanning', files found 21 (0 B), no sector progress, stuck"

Call chain: `scan_start` → bridge zero-init config (stage_mask 0 ⇒ 7, weights 40/35/25, threads auto; scan.rs:229-231) → `cr_ui_scan_run` (cr_ui.cpp:2324: controller, heartbeat thread :2378-2382, `ctrl->run` :2383) → run() order quick(:1470)→carve(:1519)→deep(:1522) → UI events (scan_event_sink → channel → emitter events.rs:252+, telemetry poll :364-403) → frontend "Advanced Scanning {overall%}" + "Files Found: N (size)" + "Reading sector: X/Y" (scan-progress-page.tsx:131-132, 175-195).

Decode of the symptom:
- **75%**: quick(40) + carve(25) completed = 650; +~100 permille into deep = 750. The "jump" is quick+carve finishing fast (small/unknown-FS volume ⇒ quick engine no-ops, carve finds nothing quickly), then the DEEP stage dominating wall time. "Advanced Scanning" label covers deep+carve (contract.ts:171-175).
- **"no sector progress"**: `bytes_scanned` is carve-true-bytes ONLY (cr_scan.h:619-621; deep's StageBridge passes bytes_units=false, cr_scan.cpp:2208). During deep the "Reading sector: X/Y" line freezes **by design** — deep reports permille/units, never bytes.
- **"files found 21 (0 B)"**: D4b `bytes_found` = tree `live_logical` sum of `logical_size` (cr_node.cpp:226). 21 attached nodes with logical_size 0 = the NTFS remnant sweep's metadata-only nodes (compressed/absent $DATA honestly refused, cr_mftwalk.cpp:1770-1780) and/or deleted FAT entries with zeroed size fields.
- **"stuck"**: the deep stage not completing. The engine DOES fire `CR_EVT_HEARTBEAT` every 5 s with standing truth (cr_ui.cpp:1142-1194) — but the bridge consumes it WITHOUT emitting a progress payload (`emit = false`, events.rs:337-340), and telemetry is only polled when some other event arrives (events.rs:364-403). During a deep-stage lull the UI therefore renders a frozen 75% with no elapsed-sector motion — a stall is INDISTINGUISHABLE from a hang at the presentation layer even when the engine is alive.

Ranked root-cause candidates:
1. **[Presentation contract — HIGH] Deep stage has no byte axis + heartbeat not rendered** (engine cr_scan.cpp:2208 bytes_units=false; bridge events.rs:337-340 emit=false; telemetry poll gated on events :364). Even a healthy long deep sweep looks exactly like the user's report. Fix candidates: render heartbeat (elapsed/standing permille) in the UI; or poll `cr_ui_scan_telemetry` on a timer; or give the remnant sweep a byte-coverage axis it already computes (`ctx.scanned`, cr_mftwalk.cpp:2328) — it is fed to `deep_cb` as units, not bytes.
2. **[Engine genuine stall — MEDIUM] NTFS remnant sweep (D4e class residual)**: `scan_mft_remnants` (cr_mftwalk.cpp:2342) fans `total/threads` chunks over the pool (:2375-2382), each reading 4 MiB+overlap windows through the SHARED 8 MiB LRU CachedReader (single mutex, cr_cached.h:81) — 8 workers × 256 KiB block fills serialize on the cache mutex and evict each other's blocks (thrash): throughput collapses with the disk mostly idle; the 655 s frozen stretch that motivated D4e was exactly this shape. D4e added ticks, not throughput. On a multi-TB NTFS volume the sweep alone is hours.
3. **[Engine genuine stall — MEDIUM] whole-disk deep extras**: `lost::discover` signature sweep (cr_scan.cpp:2257) + up to 10 candidate volume scans (cap, :2334-2393) — a candidate whose engine loops on corrupt metadata without hitting a cancel poll head stalls deep indefinitely (candidate errors are swallowed by design, :2431).
4. **[Counter truth — LOW] "0 B"**: metadata-only remnant/deleted nodes carry logical_size 0 (cr_mftwalk.cpp:1770-1780); the og displays a size for such rows from its own heuristics. If ALL 21 are 0-sized, the whole found-set is remnant-class — consistent with a quick pass that found nothing structured (e.g., FS probe unknown) and a deep sweep emitting orphan records.
5. **[UI event starvation — LOW] sync_channel(512) try_send drop** (events.rs:52): only matters if the emitter thread is blocked; it is a plain loop on `rx` — not observed.

---

## 9. Unfinished / stubbed / drifted (search TODO/FIXME/stub/mock over engine/src)

- No TODO/FIXME/XXX/HACK markers exist in engine/src (clean). One honest "NOT implemented" note: AES-XTS CTS extension deliberately absent (cr_aes.cpp:12).
- **ABI drift engine↔bridge (the real "hallucinated contract" class):**
  - Bridge `CrUiDiskInfo` missing `bus_type` (abi.rs:174-189) though engine ABI v4 ships it (cr_ui.h:435) — see §8.1.
  - Bridge `CrUiSmartReport` missing trailing `power_cycles_nvme` (abi.rs:648-682 vs cr_ui.h:1569) — 8-byte stack overflow per health call, see §8.2.
  - Bridge header claims "92-export surface" and "`health_read_ex2` ... GONE (BF-03)" (abi.rs:20, :684) while the engine exports 93 including `cr_ui_health_read_ex2` (cr_ui.h:1590) — the bridge was written against a different ABI-era belief.
- The `CsmiMiniportChannel` constructor silently degrades (port open failure → unusable channel) — intended transport miss (cr_health.cpp:149-175).
- `cr_ui_scan_run` deletes a leftover checkpoint .tmp at run end (cr_ui.cpp:2392-2404) — note the stray-indent comment block :2393-2397 (cosmetic).
- Devices with no media (empty card readers) may not appear at all — OS-level; documented behavior of the numbered namespace.

## 10. Next actions (for the fix squad)
1. Bridge ABI refresh: append `bus_type` (CrUiDiskInfo) + `power_cycles_nvme` (CrUiSmartReport) in ui/src-tauri/src/bridge/abi.rs; re-expose ex2 or delete the stale claims; map kind 'sd' for bus 12/13. (Fixes the visible face of defects 1 & 2.)
2. Health UX: render an explicit "no SMART on this device (SD/USB)" state instead of the generic error EmptyState; consider NVMe-first routing when bus_type==17.
3. Scan presentation: poll telemetry on a 1-2 s timer in the emitter (or render heartbeat) so deep-stage liveness is visible; consider exposing the remnant sweep's coverage bytes as `bytes_scanned` (units already ticked).
4. Remnant-sweep throughput: per-worker read contexts or a larger shared cache to stop 8-worker LRU thrash (cr_mftwalk.cpp:2375 + cr_cached.h).
5. Fix `fill_identity_card` index/number assumption (cr_ui.cpp:2061, rrecov.cpp:419) — search by `.number`, not index.
