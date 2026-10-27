# CORE-SURFACE.md — core_recovery exhaustive context sheet

> Produced by Agent A (Task 2-a) from a line-by-line read of
> `/home/z/my-project/work/core_recovery` @ `a574ecf6cc06a374e1ee0f893e574efef4904639` (branch `main`, "v0.4.0-parity" tree).
> READ-ONLY analysis; no repository file was modified.
> Every claim cites the file it was verified in. Companion repo context:
> the core's kernel transport is `rda-driver`; its ground truth is the
> `easeus-drw-research` RE corpus.

---

## 1. Identity

- **What it is**: a user-mode **Windows data-recovery engine stack** — clean-room rewrite of the reference product's engine layer (device/partition model, 9 filesystem engines, LDM dynamic disks, lost-partition discovery, grammar-aware carving, durable scan sessions, transactional repair, safe recovery writer, SMART/NVMe health, BitLocker legitimate unlock, boot-chain repair), built on the `rda-driver` kernel transport. (`README.md:1-53`)
- **Version state**: `project(... VERSION 0.4.0)` (`CMakeLists.txt:4`); release line `v0.4.0-parity` (`RELEASE-NOTES.md:1`). The parity audit is **closed**: "ACUTE 0 · SEVERE 0 · MAJOR 0 · MINOR 0 · INFO 0 open · tracked-open 1 (OQ-S4-3…)" (`worklog.md:17069-17074`, `diff-to-fix/SUMMARY.md:74-78`). 926 tests on Linux lanes (885+4 skipped core + 35 UI + C consumer); release tag gate was "one green Windows CI + the OQ-S4-3 bench artifact" at last worklog entry (`worklog.md:17076-17080`).
- **Language standard**: C++20 (`CMAKE_CXX_STANDARD 20`, extensions OFF) for engines; **C11** for the `cr_ui` ABI boundary (`CMAKE_C_STANDARD 11`, `CMakeLists.txt:13-16`; `include/cr/cr_ui.h` "C11, no C++ types cross the boundary"). MSVC `/W4 /WX /permissive-`; gcc `-Wall -Wextra -Wpedantic -Werror -Wshadow` (`CMakeLists.txt:19-36`).
- **Build system**: CMake ≥ 3.25 + CMakePresets (5 configure presets: `linux-gcc` [ASan], `linux-release`, `linux-ubsan`, `win-x64-debug`, `win-x64-release`) + vcpkg manifest mode on Windows (`CMakePresets.json:8-65`).
- **Dependencies (vcpkg.json)**: `zlib`, `gtest`, `lz4`, `zstd` (`vcpkg.json:4-9`). zlib = session-container compression; lz4+zstd = ZFS/Btrfs compressed extents — **both REQUIRED, build fails without them** (FB-X01: "no silent codec-less builds", `CMakeLists.txt:138-147`).
- **Platform targets**: Windows 7 SP1 → Win11, x64 (machine-enforced: `_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000` compile definitions; PE OS-version stamped 6.1 post-link by `tools/stamp_osversion_um.py`; import allowlist gate `tools/verify_pe_um.py` — both run in CI and release, `.github/workflows/ci.yml:144-165`, `.github/workflows/release.yml:26-42`). Linux = development/CI platform for the pure core (win layer is parse-gated via `tools/win_syntax_check.sh` instead of compiled).
- **Target envelope detail**: every shipped PE carries the "Win7 SP1 loader contract"; `CodeQL` CI is deliberately deferred (`if: false` — recorded, `docs/testing.md:43-48`).
- **Repo stats**: 88 `.h` files total (76 in `include/cr/`, 9 test win-stubs + 2 scripts refs + fuzz common), 79 `.cpp` under `src/` (66 core + 9 win + 1 app + 1 ui + 4 CMakeLists), 61 test `.cpp` + `tests/ui_abi_check.c`, 15 libFuzzer harnesses under `fuzz/`, 420+ markdown docs.

---

## 2. Public API surface

There are exactly **three consumption surfaces** (docs/API.md `docs/API.md:1-8`):

1. the `rrecov` CLI (Windows EXE),
2. the **`cr_ui` C ABI** — `cr_ui.dll` / `libcr_ui.so`, the host-binding surface (this is the UI's boundary),
3. the C++20 core headers under `include/cr/` (pin a commit; not ABI-stable).

### 2.1 The `cr_ui` C ABI — 78 exports (THE UI surface)

Header: `include/cr/cr_ui.h` (1131 lines). `CR_UI_ABI_VERSION = 2` (`cr_ui.h:75`). Version policy: **append-only** — adding exports / appending struct fields / adding event types is compatible (no bump); removing/renaming exports, changing a field's type/meaning/order, changing ownership or threading contracts bumps the version. There is no v1.1/v1.2 (historical annotations were doc defects; `docs/ABI-CONTRACT.md:471-497`). Every function returns `uint32_t` from the same numeric space as `cr_error.h` (0 = success); no exception ever escapes (catch-all → `CR_UI_ERR_INTERNAL` 0x50090001).

The **78 exports**, grouped by lifecycle (verified against `cr_ui.h` CR_UI_API declarations; count also stated in `RELEASE-NOTES.md:58` and `docs/API.md:68-69`):

**Version & diagnostics (3)**
| Export | Signature / contract |
|---|---|
| `cr_ui_abi_version` | `uint32_t cr_ui_abi_version(void)` — call first; refuse to bind on mismatch (`cr_ui.h:99`) |
| `cr_ui_last_error` | `const char* cr_ui_last_error(void)` — thread-local text for the last failing call on this thread; stable until next `cr_ui_*` call on same thread; never NULL (`cr_ui.h:104`) |
| `cr_ui_string_free` | `void cr_ui_string_free(char* s)` — frees heap strings (free() also valid) (`cr_ui.h:107`) |

**Devices (8)** — opaque `cr_ui_device*`
| Export | Notes |
|---|---|
| `cr_ui_open_image(const char* utf8_path, uint64_t offset, uint64_t bytes, cr_ui_device** out)` | portable; `bytes==0` = to EOF; 512-byte-sector read-only view; flat/VMDK/VHD/VHDX (`cr_ui.h:275`) |
| `cr_ui_open_disk(uint32_t disk, cr_ui_device** out)` | Windows only (`ERR_UNSUPPORTED` elsewhere); prefers RDA driver, falls back `\\.\PhysicalDriveN`; chosen transport reported by `cr_ui_device_transport` (`cr_ui.h:287`) |
| `cr_ui_open_volume(char letter, cr_ui_device** out)` | BitLocker probed first; locked → `CR_UI_ERR_FVE_LOCKED` (0x50060001) unless auto-unlock succeeds; returns the volume's slice on its parent disk (`cr_ui.h:288`) |
| `cr_ui_open_volume_with_key(char letter, const char* utf8_hex_fvek, cr_ui_device** out)` | M-033: 64 hex chars (AES-128-XTS) or 128 (AES-256-XTS), full-volume-encryption-key bytes `key||tweak-key`; in-stack XTS-AES decrypt (`cr_ui.h:314`) |
| `cr_ui_unlock_volume_with_dictionary(char letter, const char* const* utf8_user_passwords, uint32_t count)` | N-064 consent action; 228-entry shipped dictionary first, then user candidates; OK on unlock, `CR_UI_ERR_FVE_LOCKED` if all fail (`cr_ui.h:330`) |
| `cr_ui_last_transport_error(void)` | M-030: process-global advisory — the surviving transport leg's error from the LAST `cr_ui_open_disk`; 0 = last open succeeded on first-choice leg (`cr_ui.h:302`) |
| `cr_ui_device_close(cr_ui_device*)` / `cr_ui_device_geometry(cr_ui_device*, cr_ui_geometry* out)` | geometry = `{total_bytes, logical_sector, physical_sector, sector_count}` (`cr_ui.h:336-343`) |
| `cr_ui_device_transport(cr_ui_device*, char* buf, uint32_t cap, uint32_t* needed)` | two-call; returns "rda" / "direct" / "image" (`cr_ui.h:348`) |

**Enumeration (2)** — Windows only
- `cr_ui_enumerate_disks(cr_ui_disk_info* out, uint32_t* count)` — two-call (`*count` IN capacity when out≠NULL, OUT needed when NULL); `cr_ui_disk_info{number, size_bytes, logical_sector, physical_sector, removable, layout (0 none/1 MBR/2 GPT/3 EBR), table_ok, model[128], serial[128]}`; ≤ 200 disks bound; enumeration failure returns its error code, never a silent empty list (`cr_ui.h:353-380`)
- `cr_ui_enumerate_volumes(cr_ui_volume_info* out, uint32_t* count)` — `{letter, disk, offset_on_disk, bytes, fs_name[16], label[64], bitlocker}` (`cr_ui.h:366-380`)

**Scan lifecycle (18)** — opaque `cr_ui_scan*`
| Export | Notes |
|---|---|
| `cr_ui_scan_create(const cr_ui_scan_config* cfg, cr_ui_scan** out)` | zero-init config = documented defaults. Config fields: `stage_mask` (0 = 7 = quick|deep|carve; bits 1/2/4), `whole_disk`, `fs_hint` (0 auto, 1 NTFS, 2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS, 11 UDF; 5 reserved/never produced; 10 BitLocker probe-only), `quick_weight`/`deep_weight`/`carve_weight` (0 = 40/35/25), `threads` (0 = auto), `volume_label_utf8` (≤31 chars), `volume_model_utf8`/`volume_serial_utf8`/`volume_firmware_utf8`/`volume_wwn_utf8`/`volume_interface_utf8` (identity-card overrides), `checkpoint_path_utf8` (stage-boundary auto-checkpoints), `progress` + `progress_user`, `pre_scan_repair` + `repair_consent` + `repair_consent_user`, `memory_warn_mb` (0 = OFF), `raw_category_name_utf8` (NULL/"" = "Other Deleted Files (Carved)"), `pre_scan_check` (`cr_ui.h:425-485`) |
| `cr_ui_scan_close` / `cr_ui_scan_attach_device(cr_ui_scan*, cr_ui_device*)` | device borrowed, must outlive scan; identity card auto-filled on Windows (`cr_ui.h:488-493`) |
| `cr_ui_scan_run(cr_ui_scan*)` | **synchronous on the caller's thread**; returns 0, an engine error, or `CR_UI_ERR_CANCELLED` with tree intact (`cr_ui.h:498`) |
| `cr_ui_scan_pause` / `cr_ui_scan_resume` / `cr_ui_scan_cancel` | any thread; token functions — the cancel path (`cr_ui.h:501-503`) |
| `cr_ui_scan_busy(cr_ui_scan*, uint32_t*)` / `cr_ui_scan_stopping(cr_ui_scan*, uint32_t*)` | N-075 synchronous state queries: busy = run() executing; stopping = cancel requested and winding down; any thread, any time (`cr_ui.h:516-517`) |
| `cr_ui_scan_telemetry(cr_ui_scan*, cr_ui_telemetry*)` | live snapshot any thread: `{overall_permille, quick/deep/carve: {permille, units_done, units_total, bytes_done, elapsed_ms, completed}, files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms, error_ranges, paused, running, cancelled}` (`cr_ui.h:528-539`) |
| `cr_ui_scan_get_stats(cr_ui_scan*, cr_ui_scan_stats*)` | `{files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms, cached_hits, cached_misses, error_ranges, repair_ran/planned/applied/denied, check_ran/check_errors/check_detected}` (`cr_ui.h:541-560`) |
| `cr_ui_scan_assistant(cr_ui_scan*, cr_ui_assistant_stats*)` | S-023 post-scan grading stats (og RecoveryAssistant block): 8 counters + `category_counts[8]` + `matched`/`total` (total<100 forces matched 0) + `small_result` + `graded_count` (`cr_ui.h:568-589`) |
| `cr_ui_session_regrade(cr_ui_session*, const uint64_t* node_ids, uint32_t count, uint32_t* regraded)` | per-selection re-grade; only grade-1 nodes eligible (`cr_ui.h:595`) |
| `cr_ui_scan_checkpoint_count(cr_ui_scan*, uint64_t*)` | G-13 (`cr_ui.h:597`) |
| `cr_ui_scan_repair_save_undo(cr_ui_scan*, const char* utf8_path)` | S-008 undo log (CRUNDO01 container); `BAD_PARAMETER` when nothing applied (`cr_ui.h:606`) |
| `cr_ui_scan_save_session(cr_ui_scan*, const char* utf8_path)` | full truthful .crsf; valid after run() incl. cancelled (`cr_ui.h:673`) |
| `cr_ui_scan_save_cancel(cr_ui_scan*)` | N-060 SAVE-SCOPED cancel (never touches the scan token); aborted save returns `CR_UI_ERR_CANCELLED`, final path keeps previous content (`cr_ui.h:683`) |
| `cr_ui_scan_snapshot(cr_ui_scan*, cr_ui_snapshot**)` | G-02 immutable value snapshot; callable any thread WHILE run() executes; `partial=1` stamped mid-scan (`cr_ui.h:689`) |
| `cr_ui_scan_verify_identity(cr_ui_scan*)` | boot-sector hash check for resumed scans → `CR_UI_ERR_BAD_CHECKSUM` on mismatch (`cr_ui.h:697`) |
| `cr_ui_scan_set_event_fn(cr_ui_scan*, cr_ui_event_fn, void*)` | subscribe BEFORE run(); one sink per handle; NULL unsubscribes (`cr_ui.h:265`) |

**Lost-partition apply (3)** — S-029, consent-gated
- `cr_ui_apply_lost_partition(cr_ui_device* dev, const cr_ui_apply_candidate* cand, uint32_t rebuild_table, cr_ui_apply_consent_fn, void*, cr_ui_apply_result* out)` — whole-disk views only; candidate `{start_lba, sector_count, fs}` where fs = lost::CandidateFs (0 NTFS, 1 FAT, 2 exFAT, 4 GPT-table [refused], 5 unknown [refused], 6 ext2, 7 HFS+, 8 Btrfs, 9 ZFS, 10 ReFS, 11 BitLocker); plan `{refused, scope (0 single-entry, 1 rebuild-table), start_lba, sector_count, mbr_type, slot, regenerate_signature, signature, reason[160]}`; result `{applied, outcome (0 applied; 1 consent denied; 2 refused; 3 table changed; 4 applied but refresh tail failed; 5 error), undo_entries, reason[160]}` (`cr_ui.h:616-668`). Default-deny: NULL/zero consent writes NOTHING.
- `cr_ui_apply_undo_last(cr_ui_device*, cr_ui_apply_result*)` — restores last apply's undo log, idempotent (`cr_ui.h:663`)
- `cr_ui_apply_save_undo(cr_ui_device*, const char* utf8_path)` — CRUNDO01 container loadable by `rrecov fix --undo-apply` (`cr_ui.h:668`)

**Sessions (6)** — opaque `cr_ui_session*`
- `cr_ui_session_load(const char* utf8_path, cr_ui_session**)` / `cr_ui_session_close` (`cr_ui.h:705-706`)
- `cr_ui_session_get_info(cr_ui_session*, cr_ui_session_info*)` — `{fs_family (FsFamily), stage_done_mask, configured_mask, slice_offset, slice_bytes, logical_sector, physical_sector, label[32], scanned_bytes, elapsed_ms, model[48], serial[32], boot_hash}` (`cr_ui.h:708-721`)
- `cr_ui_session_stats(cr_ui_session*, cr_ui_scan_stats*)` (`cr_ui.h:722`)
- `cr_ui_session_snapshot(cr_ui_session*, cr_ui_snapshot**)` (`cr_ui.h:723`)
- `cr_ui_session_resume(cr_ui_session*, const cr_ui_scan_config* original_cfg, cr_ui_scan**)` — fresh tree, validates completion mask, controller continues where session stopped (`cr_ui.h:730`)

**Snapshots (6)** — opaque `cr_ui_snapshot*`
- `cr_ui_snapshot_close` / `cr_ui_snapshot_node_count` / `cr_ui_snapshot_partial` (`cr_ui.h:767-769`)
- `cr_ui_snapshot_node(cr_ui_snapshot*, uint32_t index, cr_ui_node_info* out, char* name_buf, uint32_t name_cap)` — node row: `{parent/first_child/next/prev (indices, CR_UI_SNAP_NONE=0xFFFFFFFF), node_id, fs_kind (FsKindCode), record_ref, origin (0 structured/1 carved/2 lost/3 virtual), deleted (0 active/1 deleted/2 partial/3 damaged), confidence (0..4), grade (0 low/1 average/2 high), overlap, is_full, logical_size, allocated_size, created/modified/changed/accessed (100ns since 1601), attributes (0x10=dir), flags, extent_count, resident_bytes, carve_format, carve_confidence, hash_valid, sha128[16], name_len}` (`cr_ui.h:741-776`)
- `cr_ui_snapshot_dfs(cr_ui_snapshot*, uint32_t* indices, uint32_t* count)` — depth-first emission order, byte-parity with serializer; two-call (`cr_ui.h:781`)

**Query (3)** — session trees only (stable trees)
- `cr_ui_query_run(cr_ui_session*, const cr_ui_query*, cr_ui_query_row** rows, uint32_t* row_count, uint64_t* total_matched)` — query `{category_mask (bits 1 image…8 bookmarks), size_min/max, modified_from/to (100ns-1601), confidence_min, extensions[] (lowercased WITH dot), extension_count, name_substring (ASCII-case-insensitive), path_prefix, deleted_only, origin_mask, sort (0 none/1 name/2 size/3 modified/4 confidence/5 path/6 grade/7 type), dir (0 asc/1 desc), offset, limit (0 = default 100), count_only}`; row `{node_id, name (heap), path ('/'-joined, heap), size, modified, origin, deleted, confidence, category, grade}`; free with `cr_ui_query_rows_free` (`cr_ui.h:801-833`)
- `cr_ui_query_apply_buckets(cr_ui_query*, uint32_t date_bucket, uint32_t size_bucket, uint64_t date_custom_from/to, uint64_t size_custom_min/max)` — og filter-bar IDs: Date 0x100 Today/0x101 Yesterday/0x102 Last 7 days/0x103 Last 1 month/0x105 Custom; Size 0x1000 <512KB/0x1001 512KB~1MB/0x1002 1MB~512MB/0x1003 512MB~1GB/0x1004 >1GB/0x1006 Custom; skipped IDs (0x104, 0x1005) and unknowns REJECTED with `CR_UI_ERR_BAD_PARAMETER` (`cr_ui.h:852-855`)

**Carve-format registry (2)** — no handle, immutable, any thread
- `cr_ui_carve_format_count(void)` — 174 rows: 35 grammar-validator registry entries (FMT_JPG..FMT_CAB_LC, `include/cr/carve/cr_signatures.h:23-122`) then 139 raw-sign formats (`cr_ui.h:881`)
- `cr_ui_carve_format_info(uint32_t index, cr_ui_carve_format*)` — `{layer (0 grammar/1 raw-sign), ext[24] lowercase-no-dot, min_size, max_size (0 = unbounded), base_confidence (0..1000), magic_len (0..16), magic_offset, magic[16]}` (`cr_ui.h:867-884`)

**User signatures (3)** — M-048
- `cr_ui_register_user_signs(const char* json_bytes, uint32_t json_len)` — og EUUserData JSON shape `{"<ext>": {"signs": ["<offset>,<HEX>"], "mime": "<type>"}}`; atomic parse+validate+install; process-global; thread-safe; re-registration REPLACES (`cr_ui.h:902`)
- `cr_ui_user_sign_count(void)` / `cr_ui_user_sign_info(uint32_t, cr_ui_user_sign*)` — `{ext[9], mime[64], offset, sign_len (1..16), bytes[16]}` (`cr_ui.h:912-913`). Tag space `0xFFFE0000` (built-in raw-sign = `0xFFFF0000 + fmt`); caps 256 entries / 8 signs per ext / 16 bytes / offset ≤ 512 (`docs/PARAMETERS.md:79-80`)

**Disk map (2)** — GAP-S6-11 visualization data producer
- `cr_ui_disk_map(cr_ui_session*, cr_ui_snapshot*, const cr_ui_disk_map_geometry* want, cr_ui_disk_map_geometry* got, uint8_t** allocation, uint8_t** coverage, uint8_t** found, uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap, uint32_t* bad_count)` — exactly one of sess/snap; overlays: allocation (0 unknown/1 used/2 free/3 bad), coverage (0 unscanned/1 scanned), found (distinct files, cap 255), confidence (max ordinal 0..4); bad_ranges = u64 [start,len] pairs ABSOLUTE bytes; memory bound: total overlay bytes ≤ 64 MiB (cell_bytes grows by powers of 2) (`cr_ui.h:947-960`)
- `cr_ui_disk_map_free(uint8_t* overlay)` — frees each array separately (`cr_ui.h:964`)

**Preview (7)** — opaque `cr_ui_preview*`
- `cr_ui_preview_open_snapshot(cr_ui_device*, cr_ui_snapshot*, uint32_t index, cr_ui_preview**)` / `cr_ui_preview_open_session(cr_ui_device*, cr_ui_session*, uint64_t node_id, cr_ui_preview**)` — device must outlive the preview; source session/snapshot may close (private props copy) (`cr_ui.h:974-977`)
- `cr_ui_preview_close` / `cr_ui_preview_size` (`cr_ui.h:978-980`)
- `cr_ui_preview_read(cr_ui_preview*, uint64_t offset, void* buf, uint32_t len, uint32_t* placed)` — len capped **4 MiB**/call; short only at EOF; unreadable spans zero-filled AND reported (`cr_ui.h:986`)
- `cr_ui_preview_metadata(cr_ui_preview*, cr_ui_metadata*)` — `{kind[24] (image|video|audio|document|archive|text|unsupported), format[24], width, height, orientation[8] ("1".."8"), datetime[40], duration_ms, codec[32], text_snippet[256], truncated}` (`cr_ui.h:992-1004`)
- `cr_ui_preview_unreadable(cr_ui_preview*, uint64_t* total_bytes, uint32_t* range_count)` (`cr_ui.h:1008`)

**Recovery (12)** — opaque `cr_ui_recovery*`
- `cr_ui_recovery_create(cr_ui_device* source, const cr_ui_recovery_config*, cr_ui_recovery**)` — config `{read_chunk (0 = 1 MiB), rename_attempts (0 = 64), skip_hash_verify (0 = verify ON), skip_dir_reconstruct (0 = ON), conflict (0 rename/1 skip/2 overwrite), progress, progress_user}` (`cr_ui.h:1019-1030`)
- `cr_ui_recovery_close` / `cr_ui_recovery_set_destination(cr_ui_recovery*, const char* utf8_path)` — drive-letter destinations arm the F12 overlap refusal (`cr_ui.h:1031-1038`)
- `cr_ui_recovery_enqueue_node(cr_ui_recovery*, cr_ui_session*, uint64_t node_id)` / `cr_ui_recovery_enqueue_subtree(...)` — subtree = every dir becomes mkdir item then every file; per-call errors (e.g. 0x50050001 `ERR_DEST_OVERLAP`) (`cr_ui.h:1045-1048`)
- `cr_ui_recovery_enqueue_snapshot(cr_ui_recovery*, cr_ui_snapshot*, uint32_t index)` / `cr_ui_recovery_enqueue_subtree_snapshot(...)` — G-02b mid-scan value path, race-free (`cr_ui.h:1051-1054`)
- `cr_ui_recovery_queue_count(cr_ui_recovery*, uint32_t*)` (`cr_ui.h:1056`)
- `cr_ui_recovery_process(cr_ui_recovery*)` — synchronous on caller's thread; `CR_UI_ERR_CANCELLED` on cancel, committed items stay committed (`cr_ui.h:1062`)
- `cr_ui_recovery_cancel(cr_ui_recovery*)` — any thread (`cr_ui.h:1063`)
- `cr_ui_recovery_get_stats(cr_ui_recovery*, cr_ui_recovery_stats*)` — `{queued, done, refused, errors, hash_mismatch, resumed, salvaged, dirs, nothing_to_recover}` (`cr_ui.h:1065-1073`)
- `cr_ui_recovery_report_json(cr_ui_recovery*, const char* source_utf8, char** json_out)` — G-09 report (schema §4 below); valid any time; free with `cr_ui_string_free` (`cr_ui.h:1078`)

**Health (1)**
- `cr_ui_health_read(uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs, uint32_t attr_cap, uint32_t* attr_count)` — one call fills report + attribute rows; report = `{available, transport[24] ("ata-passthrough"/"nvme"/"legacy"), ata_version, smart_version, attribute_count, failing_now, reallocated_sectors, pending_sectors, uncorrectable, power_on_hours, spin_retry, ssd_wear_level, temperature_c, nvme_available, temperature_c_x10, available_spare, available_spare_threshold, percentage_used, data_units_read/written, power_on_hours_nvme, unsafe_shutdowns, media_errors, critical_warning, ssd_like, trim_known, trim_enabled, host_reads, host_writes, controller_busy_time, num_error_log_entries, data_bytes_read, data_bytes_written}`; attr = `{id, value, worst, raw (6 vendor bytes), failing, name[40]}` (`cr_ui.h:1085-1125`). Windows only; `ERR_UNSUPPORTED` elsewhere. 4-status grading model: Unknown/Good/Caution/Bad (`include/cr/health/cr_grade.h:55-60`).

**Event stream** — `cr_ui_event` POD (`cr_ui.h:190-234`), types:
`CR_EVT_SCAN_STAGE=1`, `CR_EVT_SCAN_PROGRESS=2`, `CR_EVT_FILES_FOUND=3`, `CR_EVT_ERROR_RANGE=4`, `CR_EVT_CHECKPOINT=5`, `CR_EVT_LOST_CANDIDATE=6`, `CR_EVT_REPAIR=7` (outcome 0 plan-ready/1 applied/2 denied/3 failed), `CR_EVT_HEARTBEAT=8` (N-043, fired from the shim's own watchdog thread every 5000 ms while run() is in flight and no progress arrived — the honest liveness variant, never synthetic progress), `CR_EVT_SAVE_PROGRESS=9` (N-060, fired from the thread running save_session; stage=0xFFFFFFFF, stage_permille = save's own 0..1000, bytes = nodes serialized/total), `CR_EVT_FS_CHECK=10` (M-041 damage-audit summary; `check_errors` count + `check_family` byte 'f'/'n'/0). Error-range events carry the M-049 context triple `err_module` (CR_ERRMOD_*: 0 none, 1 orchestrator, 2 NTFS, 3 FAT, 4 exFAT, 5 ext4, 6 Btrfs, 7 HFS+, 8 ReFS, 9 ZFS, 10 carve, 11 lost, 12 LDM), `err_operation` (CR_ERROP_NONE=0/READ=1), `err_attempts`. Unknown type values MUST be ignored (forward compat). Progress callbacks (`cr_ui_progress_fn`, `cr_ui_recovery_progress_fn`) fire on worker threads only, must not block, carry NO cancellation semantics.

### 2.2 The `rrecov` CLI (the peer consumer)

Grammar (usage text `src/app/rrecov.cpp:2456-2484`, flags verified in the parser; canonical doc `docs/API.md:26-42`):
```
rrecov list
rrecov list-carve-formats
rrecov scan --disk N | --volume C: | --image file.vmdk|.vhd|.vhdx|.img[.dmg raw]
            [--whole-disk] [--quick-only] [--no-carve] [--part P]
            [--threads N] [--out file.crsf] [--resume file.crsf]
            [--json] [--csv] [--log file] [--filter s] [--deleted-only]
            [--bitlocker-fvek hex] [--bitlocker-dict [file]]
            [--user-signs file] [--repair] [--repair-apply]
            [--repair-undo file] [--check]
rrecov tree    --in file.crsf [--filter s] [--deleted-only] [--csv] [--json]
rrecov recover --in file.crsf (--disk N | --image file) --to DIR [--deleted-only]
               [--filter s] [--report file.json]
rrecov health  --disk N
rrecov fix     --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
rrecov apply   --disk N --session file.crsf --candidate N [--apply] [--undo file]
               [--boot-fix old-lba [--boot-fix-bcd path]]
rrecov bench   --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
               [--iterations N] [--seed N] [--json] [--no-rda]
```
Verified in-parser flags: `--volume` (rrecov.cpp:827), `--bitlocker-dict` (:763, :839), `--user-signs` (:780, :846), `--image` (:853, :1396), `--threads` (:888), `--check` (:925), `--part` (:2256), `--boot-fix`/`--boot-fix-bcd` (:1990/:1995).
**Exit codes** (`docs/API.md:44-52`): 0 success; 1 failure; 2 recover partial (item errors or hash mismatch); 3 cancelled (session still saved when `--out` given) / health: every source unavailable. CLI stdout grammar is frozen byte-identical for UI-parsed lines (`docs/UI-READINESS-PLAN.md:339-341`).

### 2.3 The C++ core API (engine layer)

- **Storage seam** `cr::IStorageDevice` (`include/cr/storage/cr_device.h:117-134`): `virtual u32 read_at(u64 off, void* buf, u32 len)`, `geometry()`, `size()`, `absolute_base()`, `is_static_media()`. Everything above reads through it.
- **Orchestration** `cr::orchestrator::ScanController` (`include/cr/orchestrator/cr_scan.h:455-535`): ctor `(tree, cfg)` or resume ctor `(tree, cfg, VolumeSessionInfo)`; `run(IStorageDevice*, CancelToken*, progress, user, ScanStats&)`; `save_session(session::Writer&, stats, SerializeProgress, user)`; statics `probe_family`, `load_session`, `resume`, `load_session_summary`; `telemetry()`, `checkpoint_count()`, `set_event_sink()` (pre-run only). `ScanConfig` fields: `run_quick/run_deep/run_carve`, `whole_disk`, `fs_hint`, `weights{40/35/25}`, `threads` (0 = auto hw−1 clamp 1..8), `pre_scan_repair` + `RepairHooks{consent, open_writer, close_writer}`, `pre_scan_check`, `volume_label/model/serial/firmware/wwn/interface/image`, `raw_category_name`, `CheckpointSink{write/tell/seek/commit}`, `ldm_resolver`, `event_fn/event_user`.
- **Engine scan entry points** (free functions, dispatched by the orchestrator; all emit into `tree::ResultTree`):
  `cr::ntfs::walk_mft(...)` + `scan_mft_remnants(...)` (`include/cr/ntfs/cr_mftwalk.h:166,182`); `cr::fat::scan_fat(...)` (`include/cr/fat/cr_fat_engine.h:256`); `scan_exfat` (`include/cr/exfat/cr_exfat_engine.h:127`); `scan_ext4` (`include/cr/ext4/cr_ext4_engine.h:107`); `scan_hfsplus` (`include/cr/hfsplus/cr_hfsplus_engine.h:98`); `scan_btrfs` (`include/cr/btrfs/cr_btrfs_engine.h:126`); `scan_zfs` (`include/cr/zfs/cr_zfs_engine.h:85`); `scan_refs` (`include/cr/refs/cr_refs_engine.h:115`); `cr::lost::discover(...)` + `materialize` + `revalidate_candidate` (`include/cr/lost/cr_lostpart.h:119-162`); `cr::carve::carve_slice(...)` (`include/cr/carve/cr_carve.h:166`); `cr::table::parse_disk_table`/`parse_ebr_chain` (`include/cr/table/cr_table_parse.h:93-100`); `cr::table::parse_ldm_database` (`include/cr/table/cr_ldm.h:296`).
- **Check/repair/apply**: `cr::check::run_fscheck/check_fat/check_ntfs` (`include/cr/check/cr_fscheck.h:159-172`); `cr::repair::analyze/execute/undo/rebase_plan` (`include/cr/repair/cr_rawfixer.h:158-179`); `cr::apply::plan_apply/execute_apply/undo_apply/mbr_type_for_family` (`include/cr/apply/cr_apply.h:228-261`); `cr::bcdfix` + `cr::nvramfix` (BCD cell offset surgery + UEFI `Boot####` NVRAM updates, plan/execute/undo with default-deny consent — `include/cr/bcdfix/cr_bcdfix.h`, `include/cr/nvramfix/cr_nvramfix.h`).
- **FsFamily enum** (orchestrator, serialized as fs_kind): `Unknown=0, Ntfs, Fat, Exfat, Ext4, Reserved5=5 (never produced), Hfsplus=6, Btrfs=7, Zfs=8, Refs=9, BitLocker=10 (probe-only, locked-volume surfacing), Udf=11 (recognition + carve-only)` (`cr_scan.h:36-69`).
- **FsKindCode** (per-file session wire codes, cr_types.h:81-93): `Fat12=0, Fat16=1, Fat32=2, Ntfs=3, Raw=6 (carved), Exfat=7, Hfsplus=8, Ext4=9, Refs=10, Btrfs=11, Zfs=13` — the corpus FACTORY map is the single source of truth.

**Public-operation count**: 78 cr_ui exports + 10 CLI commands (`list`, `list-carve-formats`, `scan`, `tree`, `recover`, `health`, `fix`, `apply`, `bench`, plus usage) — the ABI is the surface a UI wires to.

---

## 3. How the core expects to be consumed — THE TRANSPORT/BOUNDARY

**The core is consumed IN-PROCESS. There is NO IPC, NO subprocess, NO JSON-over-stdio, NO socket, NO service.** The UI binds `cr_ui.dll` (Windows) / `libcr_ui.so` (Linux) directly — LoadLibrary + GetProcAddress / static import — and drives the full scan/recover lifecycle through the 78 flat-C exports with opaque handles and C callbacks.

Evidence (exact quotes):
- `README.md:75-78`: "Hosts bind in-process through `cr_ui.dll` / `libcr_ui.so` — the connection guide walks the full lifecycle".
- `include/cr/cr_ui.h:5-21`: "the reference product binds its UI to the engines IN-PROCESS through C-export factories … There is NO CLI subprocess and NO IPC on the UI<->engine leg of the reference. This header is our equivalent of that factory surface … a C11 flat ABI over cr_core (+ the cr_win platform layer on Windows) so a UI (or any host …) can drive the full scan/recover lifecycle in-process without linking C++ classes: opaque handles stand in for the controller interfaces, `cr_ui_*` factories stand in for `EUCreate*Searcher`, and C callbacks stand in for the engine event hooks."
- `docs/CONNECTION-GUIDE.md:29-35` (embedding-choices table): "a C/C++ host, wizard-style UI, service → `cr_ui.dll` / `libcr_ui.so` — the flat-C ABI (G-01)"; "a one-shot tool / scripting → the `rrecov` CLI"; "a host that wants engine-level control → the C++ headers (pin a commit; not ABI-stable)".
- `docs/ABI-CONTRACT.md:504-511` (§10 "What this ABI deliberately does NOT do"): "No subprocess spawning: the UI binds in-process exactly like the og's factory surface (the CLI is a peer consumer, not the bus). No remote/cloud/NAS surfaces."
- The binding lifecycle is spelled out in `docs/CONNECTION-GUIDE.md:39-60`: `cr_ui_abi_version()` gate → `cr_ui_enumerate_disks` → `cr_ui_open_disk` → `cr_ui_scan_create` (zero-init config) → `cr_ui_scan_attach_device` → `cr_ui_scan_set_event_fn` → `cr_ui_scan_run` (blocks; cancel/pause from other threads) → `cr_ui_scan_get_stats` → `cr_ui_scan_save_session` → close handles.
- The typed event stream is explicitly transport-ready but transport-agnostic: "a POD … a transport bridge (streaming channel, queue, FFI pipe) can serialize the struct byte-for-byte. The core does NOT assume a transport; it emits the clean typed stream." (`docs/ABI-CONTRACT.md:196-200`; also `cr_ui.h:134-135` "POD payload (no pointers to free - a transport bridge can serialize it byte-for-byte)"). So if a UI process wants to host the core in a helper process, the UI itself must build that bridge — the core ships none.

**What this means for a Tauri/React UI**: the documented boundary is a native FFI bind to `cr_ui.dll` (e.g., Rust `libloading`/static import of the 78 `cr_ui_*` symbols, or a C shim) with the `cr_ui.h` header as the single contract; alternatively drive the frozen-grammar CLI. `docs/UI-READINESS-PLAN.md` (§#1) is the landing record of this surface ("`cr_ui` C-ABI shim DLL … Coverage: scan lifecycle … telemetry … snapshot + node enumeration, query/filter/sort/paging, bounded preview read, session save/load, recovery lifecycle + per-item progress + JSON report, health … disk/volume enumeration").

---

## 4. Persistence formats

### 4.1 The `.crsf` session container (the session format)
Header `include/cr/session/cr_crsf.h`; format doc `docs/crsf-format.md`.
- Magic `"CRSFMT01"` (8 bytes) + `version u32 = 1` + `flags u32` (bit 0 `CRSF_FLAG_ABS_EXTENTS` = RESULT_TREE extents are ABSOLUTE device offsets; absent = pre-F-1 slice-relative, loaders normalize via VolumeInfo.slice_offset) + `record_count u32` + `unit_size u32 = 512` + `session_id lo/hi u32` + reserved 32 → **64-byte header** (`cr_crsf.h:30-42`, `docs/crsf-format.md:9-20`).
- Record header 24 bytes: magic `"CRSR"` + `type u32` + `session_id u64` + `chain u32` + `crc32 u32` (of DECOMPRESSED payload) + reserved. Chunk framing: `u16 header` bit 15 = deflate (zlib level 5), bits 0-11 = payload_size−1, payload ≤ 4096/chunk; stored form when deflate doesn't shrink; reader falls back to raw payload on inflate failure; record CRC mismatch → record delivered empty + `ERR_BAD_CHECKSUM`, caller continues; broken header → skip forward in 512-byte steps (resumable by construction) (`docs/crsf-format.md:22-50`).
- Record types (`cr_crsf.h:44-76`): `REC_SESSION_STATE=1, REC_VOLUME_INFO=2, REC_PARTITION_CONTEXT=3, REC_RESULT_TREE=4, REC_ERROR_MAP=5, REC_ASSISTANT_STATS=6 (S-023), REC_ALLOC_MAP=7 (S-009 free map, downsampled ≤ 8 MiB), REC_DISK_LAYOUT=8 (M-042 partinfo.dat semantics), REC_FS_CHECK=9 (M-041 damage-audit report)`. Old readers skip unknown types; OPTIONAL records degrade honestly.
- RESULT_TREE node grammar (DFS, parents always earlier): `u32 parent_id (0xFFFFFFFF = root), u16 reserved, u8 marker (0 light/1 full), u8 origin, u8 deleted_state, u8 confidence, u64 record_ref, u32 fs_kind, u64 logical_size, u64 allocated_size, u64 created/modified/changed/accessed, u32 attributes, u16 flags, u16 name_len + UTF-16LE name, u32 extent_count + {u64 offset, u64 length}*, u32 carve_format, u16 carve_confidence, u8 hash_valid + 16B sha-128, full: u16 role_len + role + 4×u64 stats` (`docs/crsf-format.md:51-69`, `include/cr/session/cr_records.h:24-48`).
- `REC_VOLUME_INFO` payload = `VolumeInfo{fs_kind, slice_offset, slice_bytes, logical_sector, physical_sector, label[32], scanned_bytes, elapsed_ms, flags (bit0 quick, bit1 deep, bit2 carved), model[48], serial[32], boot_hash (CRC32 of first 512 bytes), image[256] (S-030), firmware[32], wwn[32], interface_desc[32], boot_snapshot[512] (N-066)}` — versioned by tail-append, all v1..v4 sessions loadable (`include/cr/session/cr_records.h:65-98`).
- `REC_PARTITION_CONTEXT` = `PartitionContext{start_lba, sector_count, fs_kind, score, kind (0 live/1 lost candidate), bytes_per_sector, sectors_per_cluster, hidden_sectors, flags (bit0 size_from_device_end, bit1 geometry valid)}`; record LENGTH carries the version (25 B v1 / 41 B v2) (`cr_records.h:119-141`, `docs/crsf-format.md:71-83`).
- `REC_DISK_LAYOUT` = `u32 count` + per-entry `{u64 start_lba, u64 sector_count, u32 mbr_type}` (`cr_records.h:153-159`).
- Session auto-naming (N-061): `auto_<label sanitized>_<M>_<D>_<H>_<m>_<s>` local time (`cr_crsf.h:148-149`).
- **Atomicity**: session saves are temp-then-rename (`<path>.tmp` + commit); a torn save never destroys the previous good file (N-060; `docs/ABI-CONTRACT.md:167-180`).
- **Undo logs** (repair/apply): durable CRC-checked container, magic `"CRUNDO01"` (`include/cr/repair/cr_rawfixer.h:147-149`); produced by `fix --apply` / `scan --repair-apply` / `apply`; consumed by `--undo-apply`.

### 4.2 Recovery report JSON schema (v1)
Doc `docs/recovery-report-schema.md`; builder `include/cr/recovery/cr_report.h`. Top-level keys (deterministic order, compact separators, trailing newline): `"schema_version": 1`, `"engine_version"`, `"source"`, `"destination"`, `"elapsed_ms"`, `"totals"`, `"items"`. Totals keys: `queued, done, dirs, refused, errors, hash_mismatch, resumed, salvaged, nothing_to_recover, pending, bytes`. Item keys: `item_id, path, name, kind ("file"|"dir"), outcome, reason, bytes, duration_ms, sha256, hash_status`. `outcome` ∈ {`done, refused, error, hash_mismatch, resumed, pending, salvaged, nothing_to_recover`}; `hash_status` ∈ {`verified, computed, mismatch, not_computed`}; `sha256` = lowercase hex 64 or empty. Optional `scan_analysis` object after totals (S-023, when the writer had a scan context). Invalid UTF-8 → U+FFFD so the document always parses. Write-only for the core (never parsed back). CI-validated by `tools/validate_report.py`.

### 4.3 Session directory layout / where data is written
- **No implicit config/state directory exists.** The core writes NOTHING except paths the caller supplies: the session file (`--out`/`cr_ui_scan_save_session`), the checkpoint file (same path, overwrite-in-place via .tmp+rename), the report file (`--report`), the undo files, and the recovery destination. Checkpoints: stage-boundary auto-checkpoints written to `checkpoint_path_utf8` (the CLI reuses the `--out` path). The BitLocker dictionary aid uses a shipped 228-entry compiled table (no file), optionally a user dictionary file. The Btrfs re-scan cache is caller-supplied-directory, OFF by default (D-89).
- **Configuration file format/locations: none.** There is no .ini/.json config file for the core itself — all settings are CLI flags or config structs (zero-init defaults). (The og's RawConfig.ini/FileFilter.xml are host-side policy; the core's equivalents are `CarveFlags::disabled_formats` string + the generated tables.)

### 4.4 Repair artifacts
`CRUNDO01` undo containers for RawFixer and lost-partition apply; BCD rewrite plans (cr_bcdfix: `BcdRewrite{field_offset, old_bytes[8], new_bytes[8]}`); NVRAM updates via an injected firmware-variable store seam (cr_nvramfix).

---

## 5. Error model

Single `cr::Err` enum (`include/cr/cr_error.h:25-86`), same numeric space across C++/ABI/CLI. Success = 0. Families:
- **0x5002 open/transport**: `ERR_OPEN_FAILED 0x50020001`, `ERR_READ_FAILED 0x50020002`, `ERR_WRITE_FAILED 0x50020003`, `ERR_RANGE_OUT_OF_BOUNDS 0x50020004`, `ERR_CANCELLED 0x50020005`, `ERR_RETRY_EXHAUSTED 0x50020006`
- **0x5003 format/parse**: `ERR_BAD_MAGIC 0x50030001`, `ERR_BAD_CHECKSUM 0x50030002`, `ERR_BAD_PARAMETER 0x50030003`, `ERR_TRUNCATED 0x50030004`, `ERR_FIXUP_FAILED 0x50030005`, `ERR_CYCLE_LIMIT 0x50030006`, `ERR_DEPTH_LIMIT 0x50030007`, `ERR_UNSUPPORTED 0x50030008`, `ERR_NO_MEMORY 0x50030009`
- **0x5004 session**: `ERR_SESSION_FORMAT 0x50040001`, `ERR_SESSION_IO 0x50040002`, `ERR_SESSION_VERSION 0x50040003`
- **0x5005 writer/policy**: `ERR_DEST_OVERLAP 0x50050001`, `ERR_DEST_UNWRITABLE 0x50050002`, `ERR_HASH_MISMATCH 0x50050003`, `ERR_DEST_FULL 0x50050004` (N-015 ENOSPC fast-fail), `ERR_DEST_NAME_EXHAUSTED 0x50050005` (N-065 cap-10000)
- **0x5006 BitLocker policy**: `ERR_FVE_LOCKED 0x50060001`
- **0x5007 health**: `ERR_HEALTH_UNAVAILABLE 0x50070001`
- **0x5008 repair/apply**: `ERR_REPAIR_REFUSED 0x50080001`, `ERR_REPAIR_UNDO_FAILED 0x50080002`, `ERR_VOLUME_LOCK_FAILED 0x50080003` (S-010), `ERR_APPLY_CONSENT_DENIED 0x50080004`, `ERR_APPLY_STALE 0x50080005`, `ERR_APPLY_REFUSED 0x50080006`, `ERR_APPLY_REFRESH_FAILED 0x50080007`
- **0x500A images**: `ERR_IMAGE_CORRUPT 0x500A0001`
- **0x5009 generic**: `ERR_INTERNAL 0x50090001`

`const char* cr::err_text(u32)` gives human text (`cr_error.h:89`). ABI-side: every entry returns these u32s; `cr_ui_last_error()` carries thread-local detail; `CR_UI_*` selects are aliases of the same values (`cr_ui.h:77-85`). Propagation rules: scan **never aborts on unreadable ranges** — they become ReadErrorMap spans (with module/operation/attempts attribution, M-049) that travel in stats/events/session and grade affected nodes; "an empty result is OK with zero files, not an error; a torn session record is repaired (F10) or refused with 0x50040001 — never silently truncated" (`docs/ABI-CONTRACT.md:116-119`). Display guidance: the damage-audit taxonomy 0x1001..0x100e (`CHK_VOLUME_TOO_SMALL 0x1001 … CHK_CHECK_EXCEPTION 0x100E`, `include/cr/check/cr_fscheck.h:79-98`) is report-only and NEVER aborts a scan. The og facility-code mapping table (N-011e) is registration-only for parity tooling (`docs/ABI-CONTRACT.md:94-114`).

---

## 6. Threading and cancellation model

- **Who runs what**: `cr_ui_scan_run` and `cr_ui_recovery_process` are **synchronous on the caller's thread** (call from a UI worker thread). `pause/resume/cancel/recovery_cancel`, telemetry, snapshot capture, `busy`/`stopping`, checkpoint-count are **safe from any thread while run() executes**. One handle, one controlling thread. Snapshots are immutable value copies — readers never block writers (`docs/ABI-CONTRACT.md:55-70`).
- **Callbacks**: progress + events fire on engine worker threads (and the shim's heartbeat-watchdog thread for CR_EVT_HEARTBEAT; the save thread for CR_EVT_SAVE_PROGRESS). The core does NOT serialize event deliveries — the sink must be thread-safe and non-blocking (single-flight discipline is the consumer's job; the shim's progress bridge serializes/monotonizes the plain progress callback under `progress_mu` — `src/ui/cr_ui.cpp:705-713`).
- **Worker pool**: `cr::ThreadPool` — fixed workers, MPMC FIFO, auto = hw−1 clamped **1..8** ("disks, not CPUs, are the bottleneck"), abort drops the queue, `parallel_for` maps item indices; `CancelPoolWatcher` polls the token every 50 ms and aborts the pool so queued work never starts post-cancel (`include/cr/cr_parallel.h:43-127`). Fan-out points: per-candidate lost-partition scans, carve windows, remnant-sweep chunks, recovery items.
- **Cancellation**: cooperative `CancelToken` (`cancelled()`, `checkpoint()`, `paused_ms()`, `paused()`) polled at every engine loop head; **transports do NOT poll** — an in-flight read completes or fails on its own (F-B-16 honest scope, `cr_device.h:136-147`). `PausableToken` parks engine threads in `checkpoint()` on a condvar while paused; cancel wakes parked waiters; pause duration excluded from elapsed (RE parity). Cancellation makes run() return `ERR_CANCELLED` (0x50020005) with the tree intact and browsable; save-on-cancel is the caller's explicit choice. The progress/event callbacks carry **no cancellation semantics** — the token functions are the only cancel path (mirrors the reference's separate abort event).
- **ResultTree**: arena-allocated, all mutation serialized on `mutex_`; hashed dedup indexes (record_ref → node; first-extent+size-class → node). Mid-scan reads must go through `Snapshot` (one O(n) copy under the mutex; ~150-300 ms at 1M nodes) (`include/cr/tree/cr_node.h:286-398`, `cr_snapshot.h:1-18`).
- **Memory monitor** (N-062): `cr_ui_scan_config.memory_warn_mb` — the shim's 5 s watchdog samples the process working set (GetProcessMemoryInfo) and emits Warn through the `cr::log` seam at/above the threshold; 0 = OFF (`cr_ui.h:458-467`, `src/ui/cr_ui.cpp:883-891`).

---

## 7. Configuration surface (every setting, default, persistence)

**CLI flags with defaults** (`docs/PARAMETERS.md:13-53`): scan: `--disk/--volume/--image` (one required), `--part` (whole disk), `--whole-disk` (off), `--quick-only` (off), `--no-carve` (off), `--threads N` (0 = auto 1..8; 1 = single-threaded deterministic), `--out` (—; also arms stage-boundary checkpoints), `--resume`, `--filter`, `--deleted-only`, `--json/--csv/--log`, `--bitlocker-fvek hex` (**consent**), `--bitlocker-dict [file]` (**consent**; shipped 228-entry table + user file), `--check` (off — read-only damage-audit pre-pass), `--repair` (off — analyze only), `--repair-apply` (**consent**), `--repair-undo file`, `--user-signs file` (invalid input aborts honestly). tree/recover/health/fix/apply/bench per §2.2. None persisted to disk.

**cr_ui_scan_config** zero-init defaults: stage_mask 0→7 (quick+deep+carve); weights 40/35/25; fs_hint 0 auto; threads 0 auto; label/model/serial/firmware/wwn/interface NULL→filled from enumeration (Windows); checkpoint_path NULL→none; pre_scan_repair 0; repair_consent NULL→never execute (default-deny); memory_warn_mb 0→OFF; raw_category_name NULL→"Other Deleted Files (Carved)"; pre_scan_check 0→off (`cr_ui.h:425-485`).

**cr_ui_recovery_config** zero-init defaults: read_chunk 1 MiB; rename_attempts 64; hash verify ON; dir reconstruct ON; conflict = Rename (`cr_ui.h:1019-1027`).

**Engine-side constants (load-bearing)** (`docs/PARAMETERS.md:70-97`): NTFS remnant 4 MiB blocks; carve windows 4–8 MiB (+4 KiB overlap, `cr_carve.h:6`); raw-sign table 139 formats (+user signs; caps 256/8/16/512); user-sign tag 0xFFFE0000, confidence 600; scan cache **8 MiB / 256 KiB** reactive LRU (`build_read_pipe(slice, 8<<20, 256*1024)`, `cr_scan.h:94-96`); direct-I/O envelope **64 KiB multiples** on every direct read; read chunking 256 KiB through retry pipe; enumerate bound **200 disks**; checkpoint cadence = stage boundaries; dictionary loader caps 4096 candidates / 32 chars / 256 KiB; preview caps (4 MiB read / 4 MiB work / 256 text bytes, `cr_preview.h:108-112`); carve max extent 256 MiB; session free-map ≤ 8 MiB persisted; writer collision `%s_%d` ladder cap 10,000; query default limit 100. Retry: max_attempts 3, mode Fail (Skip = opt-in zero-fill), backtrack ON (N-004 shifted-window second chance, `cr_device.h:96-108`).

**Consent-gated operations** (the safety architecture, `docs/PARAMETERS.md:99-115`): RawFixer apply, lost-partition apply, BitLocker FVEK/dictionary, boot-chain repair (BCD/NVRAM), plus destination-overlap refusal — "No write-side or unlock-side operation ever fires implicitly."

---

## 8. Performance characteristics and resource bounds

From `docs/PERFORMANCE-BASELINE.md` (measured, cited) and `docs/PERFORMANCE-PROFILE.md` (session-6 historical baseline):
- **Driver transport microbench** (run 39807929475): our driver vs original eprdtdrv — seq 64 KiB 532.9 vs 391.0 MiB/s; random 4 KiB 7,932 vs 8,170 IOPS (−2.9% = cancel-safety + DACL price); hostile fuzz dispatch +52%.
- **Windows CI alignbench** (run 40227375329): runner C: caps ~240 MiB/s — 128 KiB chunks 240.0 vs 132 KiB 233.6 MiB/s (the documented ~3.5× slow path does NOT manifest on virtualized Azure storage; the 64 KiB envelope stays mandated by construction). Size×QD sweep: ≥256 KiB saturates at QD1; QD>1 buys nothing → the og-matched synchronous QD1 model is measurement-justified. RDA grammar path: 64 KiB 201.9 MiB/s, 256 KiB 169.3, 1 MiB 160.5; direct PhysicalDrive 256 KiB 151.5 — driver beats direct on this runner. **cr_ui cheap-op latency** (run 40228880804): `cr_ui_abi_version` 10.7 ns/op, `cr_ui_last_error` 11.3, `cr_ui_carve_format_count` 12.6 (count=174), `cr_ui_carve_format_info` 166.3.
- **Transitions per GiB** (sequential): engine path 256 KiB blocks ≈ 4,096; carve 1 MiB windows ≈ 1,024; og ≈ 256-512 (4-8× fewer — the registered residual).
- **Query perf contract** (CI-enforced): filter+count over 1M nodes < 200 ms on CI hardware (`include/cr/query/cr_query.h:13-15`; measured 362 ms cold / 31-36 ms in-suite).
- **Local suites** (session-6 profile): full unit suite 3.04 s (release), 8.46 s (ASan); engine fixture suites 22 ms. **Recovery-quality matrix** (runs 40073093997/40079065019): exFAT native 99.94% recall / 99.16% precision; NTFS permanent_delete 87.2%; recycle 26/26 name recall post-GAP-P1-3.
- **Memory bounds**: disk-map overlays ≤ 64 MiB total; snapshot = one O(n) copy; tree arena bulk-freed; writer bounded queues; carve window-bounded; session free-map ≤ 8 MiB; carve max extent 256 MiB/file.
- **Carve efficiency**: window-aware read view measured **9.9× fewer device reads** (M-025); occupancy gate + bitmap gate + zero-stride skip counters in ScanStats (`cr_scan.h:326-339`).

---

## 9. Packaging footprint (build artifacts)

Targets (`src/*/CMakeLists.txt`): `cr_core` STATIC (76 sources, `src/core/CMakeLists.txt:5-75`), `cr_win` STATIC (9 sources, Windows-only, links cfgmgr32/Advapi32/Shlwapi), `cr_ui` SHARED (`cr_ui.dll`/`libcr_ui.so`, links cr::core [+ cr::win on Windows] + Psapi; exports ONLY the CR_UI_API surface, `src/ui/CMakeLists.txt:11-32`), `rrecov` executable (links core+win, `src/app/CMakeLists.txt:1-2`). Build tree layout: `build/win-x64-release/src/app/Release/rrecov.exe`, `build/win-x64-release/src/ui/Release/cr_ui.dll` + `cr_ui.lib`, `.../src/core/Release/cr_core.lib`, `.../src/win/Release/cr_win.lib`; vcpkg runtime DLLs (z.dll, lz4.dll, zstd.dll) deployed next to targets at POST_BUILD (`cr_deploy_vcpkg_binaries`, `CMakeLists.txt:156-167`).

**Release bundle** (`.github/workflows/release.yml:44-138`; staged SDK): `bin/` — `rrecov.exe`, `cr_ui.dll`, z/lz4/zstd (vcpkg-built), **app-local VC143 CRT + UCRT + api-ms-win-crt-*** (Win7 RTM ships no UCRT — the bundle does not load without them), pdbs when present; `lib/` — `cr_ui.lib` (import lib), `cr_core.lib`, `cr_win.lib` (statics); `include/` (the C/C++ surface: `cr/cr_ui.h` + full internal tree); `docs/`; `SHA256SUMS`. Historical bundle: 117 entries / 3.94 MB (v0.3.0-parity; `docs/CORE-GAP-LIST.md` GAP-W6-RELEASE). **A consumer must ship**: `cr_ui.dll` + `cr_ui.lib` + the vcpkg runtime DLLs + the app-local CRT/UCRT (all in bin/) — that is the complete host-binding kit; the driver (rda.sys) is a SEPARATE repo/artifact and optional (see §10).

---

## 10. Driver relationship

- **[SUPERSEDED 2026-10-27 — the mandatory-driver directive removed every direct-handle fallback; the driver is required (`ERR_DRIVER_REQUIRED`); see `docs/PARITY-DELTAS.md` D-FALL. The text below is the historical pre-directive state.]** The core preferred the RDA kernel driver, worked without it, and never required it. `cr_ui_open_disk`/`open_disk` probe `\\.\RDA` availability (`rda_available` — a bare CreateFile probe, same semantics as the og), open the grammar path `\\.\RDA\<PID-token>\<disk>\<part>` (3-segment EPMNtDrv grammar; PID token = `b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24` over LE PID bytes, `include/cr/win/cr_rda.h:99-104`), and fall back to direct `\\.\PhysicalDriveN` / `\\.\HarddiskVolumeN` handles. The fallback is observable via `cr_ui_device_transport` ("rda"/"direct"/"image") and diagnosable via `cr_ui_last_transport_error` (`docs/DRIVER-UTILIZATION.md` §1.9).
- **WinPE gate** (M-030): inside a WinPE environment (system drive 'X') the RDA leg is SKIPPED entirely — the og's callers go direct by design (`cr_transport.h:116-124`, `include/cr/win/cr_winenv.h:16-31`).
- **Recovery never uses the driver's session engine** — the og's recovery path is stateless grammar-open + ReadFile; the session client (18-code EUDCPEPM protocol, wire structs frozen in `cr_rda.h:109-168` with static_asserts) is production-dark, an imaging/verification surface only (`docs/DRIVER-UTILIZATION.md` §0).
- Wire facts: F11 — driver reports `Information = Length × 512`; the client divides (`kLegacyInformationScale 512`, `cr_rda.h:54-56`). Every direct read rides the unified **64 KiB envelope** (`io::direct_envelope`), exhaustively unit-proven (`tests/unit/test_directalign.cpp`). Error-6 self-healing reopen landed on every device class (og parity). 200-disk enumeration bound mirrors the driver CREATE grammar (disk# ≤ 199).
- Access control: our driver adds SDDL admin/SYSTEM-only (`D:P(A;;GA;;;BA)(A;;GA;;;SY)`) where the og had none — **deliberate divergence, kept** (`docs/DRIVER-UTILIZATION.md` §1.6).
- Driver-absent behavior = silent-capable fallback with truthful surfacing (§1.9) — matching the reference.

---

## 11. Platform support (Linux vs Windows state)

- **Everything under `src/core` (66 .cpp) is pure and cross-platform** — compiles identically on gcc (Linux CI, ASan/UBSan/TSan lanes) and MSVC; **no OS headers** (verified: zero include violations, `AUDIT-ARCH.md:60-68`). This includes ALL engines (ntfs/fat/exfat/ext4/hfsplus/btrfs/zfs/refs/ldm), carving, sessions, writer, report, query, preview, check, repair, apply, bcdfix/nvramfix pure modules, health grading rules (pure) — the health *transports* are win-side.
- **`src/win` (9 .cpp) compiles only on Windows**: transports (RDA grammar + direct handles), enumeration, BitLocker, health transports (ATA passthrough/CSMI/NVMe), `WinDestIo`, elevation probe, bootfix runtime. On Linux these surfaces return `ERR_UNSUPPORTED` (0x50030008) through the ABI — "the honest platform answer" (`src/ui/CMakeLists.txt:4-6`). The win layer is parse-gated on Linux by `tools/win_syntax_check.sh` (a CI lane).
- **`src/app` (rrecov CLI) builds on Windows only** (`CR_BUILD_APP`, default ON; the `linux-*` presets set it OFF — `CMakePresets.json:20,58`).
- **`cr_ui` builds everywhere**: `libcr_ui.so` on Linux links cr_core only; every win-backed export (open_disk/open_volume/enumerate/health) returns `ERR_UNSUPPORTED` there; the C consumer (`tests/ui_abi_check.c`) exercises the full cross-platform lifecycle on every lane, and non-Windows platform-truth probes are part of it (`docs/API-SURFACE-AUDIT.md:129-155`).
- Note: `AUDIT-LINUX-FS.md` is the ext4/HFS+/Btrfs/ZFS/ReFS **filesystem** parity audit (5,108 LOC read line-by-line vs the corpus), not a Linux-platform doc. Platform truth: **Windows 7 SP1→11 x64 is the production envelope** (PE OS-version 6.1 stamped, import allowlist `tools/ref_imports_um.json` enforced); Linux is dev/CI.
- Device-layer scope (N-071/D-85): enumeration is the `\\.\PhysicalDriveN` namespace + cfgmgr32 disk-interface pass (two-stack merge, `include/cr/win/cr_enumerate.h:198-255`); the og's WMI/VDS/Storage-Spaces fan-in is out of scope with a revisit trigger. NAS/remote/cloud recovery is a **standing exclusion** (`README.md:52-53`, D-88).

---

## 12. Open gaps / known issues (registered, not hidden)

From `docs/CORE-GAP-LIST.md`, `README.md:113-122`, `RELEASE-NOTES.md:122-129`, `worklog.md:17069-17080`:
- **GAP-P1-1**: NTFS XPRESS/LZX compressed $DATA not implemented (the og is LZNT1-only too; no corpus surface exists). LZNT1 is DONE.
- **N-036**: ReFS container-map translation tracked (attribute layer is done).
- **ZFS multi-vdev topologies**: single-vdev only (D-05/D-09, honest PARTIAL).
- **HFS+ attributes items the corpus itself leaves UNVERIFIED** (journal replay, decmpfs partially); HFS+ recognition analyser bodies corpus-undecoded keep magic-gate probes (registered honest gap, `docs/ground-truth-contracts.md:31-33`).
- **OQ-S4-3** (the only tracked-open item at close): the 4-vs-7-MiB chunking relative-perf measurement — needs one live Windows harness run (transcribe `bench-artifacts/alignbench.json`).
- Minor registered residuals: og's carve-naming extension point not exported (host renames post-hoc via query rows, API-SURFACE-AUDIT §2.1 #5); ACL restore on recovery N-009d (open, tracked); og facility codes mapped by category not verbatim (N-011e); `RegisterCarvedFileNamer`-style naming hook not in the ABI (names + carve_format queryable per node instead); tag-group filter NOT-EXPOSED (D-53 lineage); CodeQL deferred (`if: false`).
- Historical now-closed items a UI agent may find in old docs: SQLite sidecar REJECTED-BY-REFERENCE (never existed; the reference has none either — ABI-CONTRACT §8); dead `RdaClient` deleted (N-041; a CI lint fails if it returns); GAP-CI-1 (MSVC /tmp) fixed; all 223-226 diff-to-fix findings dispositioned FIXED/INVALID/SUPERSEDED (`diff-to-fix/SUMMARY.md`).
- **PARITY-DELTAS.md = 97 deliberate-divergence rows, zero collisions** (D-01..D-102 reconciled) — the register to consult before "fixing" any divergence.

---

## Appendix A — quick wiring checklist for a UI (from the read)

1. Call `cr_ui_abi_version()`; require `2`.
2. Windows host: `cr_ui_enumerate_disks`/`_volumes` → render device list (≤ 200 disks; enumeration failure is an error code, never an empty list). Check `cr_ui_last_error()` on any nonzero return.
3. `cr_ui_open_disk`/`_volume`/`_image`; check `cr_ui_device_transport` (rda/direct/image) and `cr_ui_last_transport_error` for degraded-transport diagnostics.
4. Locked volume → surface `ERR_FVE_LOCKED` (0x50060001); offer the three consent paths: FVEK (`cr_ui_open_volume_with_key`), dictionary (`cr_ui_unlock_volume_with_dictionary`), or tell the user to unlock in Windows and retry.
5. `cr_ui_scan_create` with zero-init config (memset-0 = all defaults); set `whole_disk=1` for lost-partition discovery; attach device; `cr_ui_scan_set_event_fn` BEFORE run; run on a worker thread; drive pause/resume/cancel from UI threads.
6. Events arrive on worker threads (plus the shim watchdog thread for HEARTBEAT and the save thread for SAVE_PROGRESS): marshal to the UI thread yourself; never block in the sink; ignore unknown `type` values.
7. Browse: `cr_ui_scan_snapshot` mid-scan (respect `partial`) or `cr_ui_session_load` + `cr_ui_query_run` post-scan. Preview via `cr_ui_preview_*` (4 MiB/call cap; zero-filled unreadable spans are reported, not hidden).
8. Recover: `cr_ui_recovery_create` → `set_destination` (overlap refusal armed automatically for disk-backed sources) → enqueue node/subtree/session/snapshot → `process` on a worker thread → `report_json` for per-item truth (outcomes incl. `salvaged`/`nothing_to_recover`/`pending`).
9. Repair/apply flows are default-deny: wire real consent UI to `cr_ui_repair_consent_fn` / `cr_ui_apply_consent_fn`; persist undo logs (`cr_ui_scan_repair_save_undo` / `cr_ui_apply_save_undo`).
10. Sessions: `cr_ui_scan_save_session` is atomic; `cr_ui_scan_save_cancel` is save-scoped; resume = `session_load` → `session_resume` → attach → `scan_verify_identity` → run.

## Appendix B — file map (for verification)

- ABI header: `include/cr/cr_ui.h` (1131 L) · shim impl: `src/ui/cr_ui.cpp` (3803 L)
- Orchestrator: `include/cr/orchestrator/cr_scan.h` (673 L) + `src/core/orchestrator/cr_scan.cpp` (2139 L)
- CLI: `src/app/rrecov.cpp` (2509 L)
- Errors: `include/cr/cr_error.h` · module attribution: `include/cr/cr_errmod.h`
- Sessions: `include/cr/session/cr_crsf.h`, `cr_records.h` · format doc: `docs/crsf-format.md`
- Report: `include/cr/recovery/cr_report.h` · schema: `docs/recovery-report-schema.md`
- Writer: `include/cr/recovery/cr_safewriter.h` · Win dest: `include/cr/win/cr_destio.h`
- Transport: `include/cr/win/cr_transport.h`, `cr_rda.h` · driver contract doc: `docs/DRIVER-UTILIZATION.md`
- Query: `include/cr/query/cr_query.h` · Preview: `include/cr/preview/cr_preview.h`
- Carve: `include/cr/carve/cr_carve.h`, `cr_signatures.h`, `cr_rawsigns.h`, `cr_usersigns.h`
- Check: `include/cr/check/cr_fscheck.h` · Repair: `cr_rawfixer.h` · Apply: `cr_apply.h` · Boot chain: `cr_bcdfix.h`, `cr_nvramfix.h`
- Health: `include/cr/health/cr_grade.h`, `cr_nvme.h`, `cr_csmi.h`, `cr_smart_ata.h` + `include/cr/win/cr_health.h`
- BitLocker: `include/cr/win/cr_bitlocker.h` · dict: `include/cr/recovery/cr_dict.h`
- Tree/grading/snapshot: `include/cr/tree/cr_node.h`, `cr_grading.h`, `cr_snapshot.h`, `cr_diskmap.h`
- Threading: `include/cr/cr_parallel.h` · devices/cancel: `include/cr/storage/cr_device.h`
- Truth docs: `docs/API.md`, `docs/ABI-CONTRACT.md`, `docs/CONNECTION-GUIDE.md`, `docs/PARAMETERS.md`, `docs/FORMATS.md`, `docs/CAPABILITY-MATRIX.md`, `docs/CORE-GAP-LIST.md`, `docs/UI-READINESS-PLAN.md`, `docs/PARITY-DELTAS.md` (97 rows), `diff-to-fix/SUMMARY.md` (the board), `diff-to-fix/CROSS-REF-MATRIX.md`, `diff-to-fix/PARAMETERS.md` (the audit taxonomy B1..Q70)
- CI: `.github/workflows/ci.yml` (lint + linux ASan + linux-tsan + linux-ubsan + windows + fuzz + codeql-deferred), `api-benchmark.yml`, `release.yml`, `comparative-bench.yml`
