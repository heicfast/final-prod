# API surface — post-remediation re-stamp (HEAD 3b203fe1, main)

Task T2-B (Agent B): the complete public API surface audit (C ABI + C++).
Body first written at Session-84 Phase 0 (baseline `e8dcaba8`); re-verified
first-hand at the post-remediation coherence session, HEAD `3b203fe1` (main).
Every `file:line` below is re-derived against the CURRENT tree: full re-read
of `include/cr/cr_ui.h` (1659 lines), `include/cr/cr_error.h` (128 lines),
`src/ui/cr_ui_exports.def`, the declaration/definition maps of all 93 exports
in `src/ui/cr_ui.cpp` (5173 lines — every trampoline line re-pinned; inner
helper lines re-pinned where named), the 4 contract docs, `tests/ui_abi_check.c`
(1221 lines), and the per-module header sweep (now 85 module headers). Count
derivation reproduced independently (python, same regexes as
`tools/doc_truth_gate.py:313-329`; gate derives exports=93, error codes=44,
event types=12 at this tree). Session-84-era entries are kept and marked
**[corrected]** where the tree moved; nothing verified-true was deleted.

---

## Exported functions

Calling convention: every function is `extern "C"` and uses the default x64
convention (no `__stdcall` on x64; the ABI is C11 — cr_ui.h:24). Export macro:
`CR_UI_API` = `__declspec(dllexport)` under `CR_UI_BUILD` / `dllimport`
otherwise on Win32, `__attribute__((visibility("default")))` elsewhere
(cr_ui.h:63-73). Impl = `src/ui/cr_ui.cpp` unless noted. "Threading"/"ownership"
columns quote the header/ABI-CONTRACT contract; "errors (impl)" lists codes on
the actual return paths (incl. pass-through engine codes).

**Derived totals: 93 header declarations (96 `CR_UI_API` lines − 3 `#define`s at
cr_ui.h:68/70/73) vs 93 implementations in cr_ui.cpp — the balance is exact.**
`cr_ui_session_resume_last` (cr_ui.h:927), the Session-84 phantom (F1), landed
at a47419c7 (impl cr_ui.cpp:3752); the M-075/M-078 window added
`cr_ui_fsck_volume` (cr_ui.h:1635) and `cr_ui_turn_off_bitlocker`
(cr_ui.h:1653). The DLL therefore exports **93 `cr_ui_*` C symbols** (+1 C++
symbol via the .def).

### 1. Version & diagnostics (cr_ui.h:101-113)

| symbol | signature | threading (doc) | ownership (doc) | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_abi_version` | `uint32_t (void)` | any time, stateless | none | none — returns `CR_UI_ABI_VERSION` (3) | cr_ui.cpp:1377 |
| `cr_ui_last_error` | `const char* (void)` | thread-local; stable until next cr_ui_* call on same thread; never NULL (cr_ui.h:107-109) | library-owned buffer | none (returns TLS string, cr_ui.cpp:117,1381) | cr_ui.cpp:1381 |
| `cr_ui_string_free` | `void (char* s)` | any time | frees heap strings returned by the API; `free()` also valid (cr_ui.h:112) | none | cr_ui.cpp:1385 |

### 2. Devices & transport (cr_ui.h:305-445)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_open_image` | `uint32_t (const char* utf8_path, uint64_t offset, uint64_t bytes, cr_ui_device** out)` | synchronous; portable (all lanes) | caller closes via `cr_ui_device_close`; device must outlive scans/previews/recoveries (cr_ui.h:310-315) | `ERR_BAD_PARAMETER` (null args / window past EOF / sub-sector tail), `ERR_OPEN_FAILED` (open_image_impl:1395ff) | cr_ui.cpp:1865 → open_image_impl:1395 |
| `cr_ui_open_disk` | `uint32_t (uint32_t disk, cr_ui_device** out)` | synchronous; Windows only (cr_ui.h:318-319) | caller closes | Win: `ERR_BAD_PARAMETER` (:1424ff), classified `ERR_DRIVER_*` 0x500B0001..05 (driver_state_error, :1436ff), `ERR_OPEN_FAILED`/open rc (target error); non-Win: `ERR_UNSUPPORTED` (stub :1727) | cr_ui.cpp:1868 → open_disk_impl:1424 / stub:1727 |
| `cr_ui_open_volume` | `uint32_t (char letter, cr_ui_device** out)` | synchronous; Windows only (cr_ui.h:318-331) | caller closes; returned device is the volume slice on its parent disk, kept alive internally (cr_ui.h:329-331) | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED`, `ERR_FVE_LOCKED` (0x50060001) / **`ERR_FVE_ELEVATION_REQUIRED` (0x50060002, M-105: ACL diagnosis — cr_ui.cpp:1476-1481)**, `ERR_DRIVER_*`; non-Win `ERR_UNSUPPORTED` (stub :1736) | cr_ui.cpp:1879 → open_volume_impl:1463 / stub:1736 |
| `cr_ui_open_volume_with_key` | `uint32_t (char letter, const char* utf8_hex_fvek, cr_ui_device** out)` | synchronous; Windows only (M-033, cr_ui.h:398-406) | caller closes | `ERR_BAD_PARAMETER` (args, hex len, non-hex, non-FVE volume, weak FVEK), `ERR_OPEN_FAILED`, `ERR_DRIVER_*` (with_key impl:1547ff); non-Win `ERR_UNSUPPORTED` (stub :1741) | cr_ui.cpp:1883 → 1547 / stub:1741 |
| `cr_ui_last_transport_error` | `uint32_t (void)` | advisory, process-global, never clears (cr_ui.h:334-340) | none | returns the atomic `g_last_transport_error` (0 / `ERR_DRIVER_*` / target err) | cr_ui.cpp:1876 (state :1417) |
| `cr_ui_driver_status` | `uint32_t (cr_ui_driver_status_t* out)` | synchronous, thread-safe, stateless (cr_ui.h:365-371) | standalone query, no lifecycle | `ERR_BAD_PARAMETER`, query rc pass-through (driver_status_impl:1679ff, query at :1684); non-Win `ERR_UNSUPPORTED` (stub :1758) | cr_ui.cpp:1887 → 1679 / stub:1758 |
| `cr_ui_unlock_volume_with_dictionary` | `uint32_t (char letter, const char* const* utf8_user_passwords, uint32_t count)` | explicit-consent action; Windows only (cr_ui.h:415-424) | none | `ERR_BAD_PARAMETER` (non-ASCII candidate), `win::bitlocker::unlock_with_dictionary` rc (`OK`/`ERR_FVE_LOCKED`) (impl :1639ff); non-Win `ERR_UNSUPPORTED` (stub :1749) | cr_ui.cpp:1890 → 1639 / stub:1749 |
| `cr_ui_device_close` | `void (cr_ui_device* dev)` | caller's thread; close order caller's responsibility (cr_ui.h:310-311) | releases the handle (delete; try/catch :1892-1898) | none | cr_ui.cpp:1892 |
| `cr_ui_device_geometry` | `uint32_t (cr_ui_device* dev, cr_ui_geometry* out)` | synchronous | fills caller struct | `ERR_BAD_PARAMETER` (device_geometry_impl:1770ff) | cr_ui.cpp:1902 → 1770 |
| `cr_ui_device_transport` | `uint32_t (cr_ui_device* dev, char* buf, uint32_t cap, uint32_t* needed)` | synchronous; two-call pattern (cr_ui.h:437-444) | caller buffer | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` (cap < needed, :1859) | cr_ui.cpp:1905 → 1846 |

### 3. Enumeration (cr_ui.h:447-517; Windows platform layer)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_enumerate_disks` | `uint32_t (cr_ui_disk_info* out, uint32_t* count)` | no shared state; each call RE-QUERIES the OS (N-098, cr_ui.h:495-503) | caller rows (POD) | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED`, `ERR_UNSUPPORTED` (stub :2009) | cr_ui.cpp:2023 → enumerate_disks_impl:1916 (200-disk bound :1920) |
| `cr_ui_enumerate_disks_ex` | `uint32_t (cr_ui_disk_info* out, uint32_t* count, uint32_t flags)` | same; unknown flag bits refused pre-platform (M-076, cr_ui.h:485-489) | caller rows | `ERR_BAD_PARAMETER` (unknown bits), `ERR_TRUNCATED`, `ERR_UNSUPPORTED` | cr_ui.cpp:2026 → 1916 |
| `cr_ui_enumerate_volumes` | `uint32_t (cr_ui_volume_info* out, uint32_t* count)` | same | caller rows | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED`, `ERR_UNSUPPORTED` (stub :2014) | cr_ui.cpp:2039 → enumerate_volumes_impl:1973 / stub:2014 |

### 4. Scan lifecycle (cr_ui.h:519-906)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_scan_create` | `uint32_t (const cr_ui_scan_config* cfg, cr_ui_scan** out)` | one handle, one controlling thread (ABI-CONTRACT §3) | caller closes via `cr_ui_scan_close`; device attached later is BORROWED | `ERR_BAD_PARAMETER` (stage_mask unknown bits, config translation :1296-1298; invalid fs_hint :1303-1304) | cr_ui.cpp:3078 → scan_create_impl:2117 |
| `cr_ui_scan_close` | `void (cr_ui_scan* s)` | not while run() executes (impl joins heartbeat defensively) | releases handle | none | cr_ui.cpp:3081 |
| `cr_ui_scan_attach_device` | `uint32_t (cr_ui_scan* s, cr_ui_device* dev)` | before run(); identity card auto-fill on Windows (cr_ui.h:666-672, fill_identity_card:2143) | device borrowed, must outlive the scan | `ERR_BAD_PARAMETER` (scan_attach_impl:2315ff) | cr_ui.cpp:3112 → 2315 |
| `cr_ui_scan_run` | `uint32_t (cr_ui_scan* s)` | SYNCHRONOUS on caller's thread (cr_ui.h:521-527); one-shot (double-run refused) | tree stays browsable on error/cancel | `ERR_BAD_PARAMETER` (already ran :2413, no device :2415, running), `ERR_SESSION_IO` (checkpoint file), engine rc pass-through incl. `ERR_READ_FAILED` (total-blindness refusal), `ERR_CANCELLED` (:2491-2511, autosaves per S-057) | cr_ui.cpp:3115 → scan_run_impl:2408 |
| `cr_ui_scan_pause` | `uint32_t (cr_ui_scan* s)` | ANY thread while run() executes (cr_ui.h:679-682) | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3118 → 2527 |
| `cr_ui_scan_resume` | `uint32_t (cr_ui_scan* s)` | any thread | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3121 → 2533 |
| `cr_ui_scan_cancel` | `uint32_t (cr_ui_scan* s)` | any thread; run() → ERR_CANCELLED | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3127 → 2539 |
| `cr_ui_scan_busy` | `uint32_t (cr_ui_scan* s, uint32_t* out)` | any thread, any time (N-075, cr_ui.h:684-695) | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3131 → 2555 |
| `cr_ui_scan_stopping` | `uint32_t (cr_ui_scan* s, uint32_t* out)` | any thread, any time | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3134 → 2562 |
| `cr_ui_scan_set_event_fn` | `uint32_t (cr_ui_scan* s, cr_ui_event_fn fn, void* user)` | subscribe BEFORE run() (ERR_BAD_PARAMETER mid-run, cr_ui.h:300-303); one sink per handle | sink + user borrowed | `ERR_BAD_PARAMETER` | cr_ui.cpp:3124 → 2569 |
| `cr_ui_scan_telemetry` | `uint32_t (cr_ui_scan* s, cr_ui_telemetry* out)` | any thread, any time (cr_ui.h:717-718) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3137 → 2603 |
| `cr_ui_scan_get_stats` | `uint32_t (cr_ui_scan* s, cr_ui_scan_stats* out)` | post-run counters (any thread per ABI-CONTRACT §5) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3140 → scan_stats_impl:2629 |
| `cr_ui_scan_carve_tally_count` | `uint32_t (cr_ui_scan* s)` | count-as-return (N-140); null scan reads 0 | value return | none (value IS the return, :3143-3150) | cr_ui.cpp:3166 → 3143 |
| `cr_ui_scan_carve_tally` | `uint32_t (cr_ui_scan* s, uint32_t index, uint64_t* out)` | synchronous | fills caller u64 | `ERR_BAD_PARAMETER` (null / out-of-range, :3151ff) | cr_ui.cpp:3169 → 3151 |
| `cr_ui_scan_assistant` | `uint32_t (cr_ui_scan* s, cr_ui_assistant_stats* out)` | valid after run() OK (cr_ui.h:784-787) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3173 → 2664 |
| `cr_ui_scan_checkpoint_count` | `uint32_t (cr_ui_scan* s, uint64_t* out)` | any thread | fills caller u64 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3176 → 2898 |
| `cr_ui_scan_repair_save_undo` | `uint32_t (cr_ui_scan* s, const char* utf8_path)` | after run() when repair executed (cr_ui.h:796-804) | writes file | `ERR_BAD_PARAMETER` (not run / nothing applied), `ERR_OPEN_FAILED`, `ERR_SESSION_IO` (:2688ff) | cr_ui.cpp:3179 → 2688 |
| `cr_ui_scan_save_session` | `uint32_t (cr_ui_scan* s, const char* utf8_path)` | valid after run() incl. cancelled (cr_ui.h:879-889); fires CR_EVT_SAVE_PROGRESS from the calling thread | atomic tmp+rename (write_session_atomic:2340/:2954); suppresses later auto-save (S-044) | `ERR_BAD_PARAMETER`, `ERR_SESSION_IO` + container rc + `ERR_CANCELLED` (scan_save_session_impl:2984ff) | cr_ui.cpp:3196 → 2984 |
| `cr_ui_scan_save_cancel` | `uint32_t (cr_ui_scan* s)` | ANY thread (N-060, cr_ui.h:891-898); save-scoped | sets atomic save_cancel | `ERR_BAD_PARAMETER`; no-op if no save in flight | cr_ui.cpp:3199 → 3006 |
| `cr_ui_scan_snapshot` | `uint32_t (cr_ui_scan* s, cr_ui_snapshot** out)` | ANY thread while run() executes (partial=1) or after (cr_ui.h:894-898) | caller closes via `cr_ui_snapshot_close`; value copy | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:3202 → 3013 |
| `cr_ui_scan_verify_identity` | `uint32_t (cr_ui_scan* s)` | after resume+attach, before run() (cr_ui.h:900-906) | none | `ERR_BAD_PARAMETER` (not resumed / no device), `ERR_BAD_CHECKSUM` (mismatch), OK on unverifiable v1 session (scan_verify_identity_impl:3054ff) | cr_ui.cpp:3205 → 3054 |

### 5. Lost-partition apply (cr_ui.h:806-877; ABI-since 1.3)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_apply_lost_partition` | `uint32_t (cr_ui_device* dev, const cr_ui_apply_candidate* cand, uint32_t rebuild_table, cr_ui_apply_consent_fn consent, void* consent_user, cr_ui_apply_result* out)` | synchronous; consent fires exactly once before any write; default-deny | undo log retained on the device handle | `ERR_BAD_PARAMETER`, `ERR_APPLY_CONSENT_DENIED`, `ERR_APPLY_STALE`, `ERR_APPLY_REFUSED`, `ERR_APPLY_REFRESH_FAILED`, `ERR_VOLUME_LOCK_FAILED` (S-010), engine rc pass-through (apply_lost_partition_impl:2772ff) | cr_ui.cpp:3183 → 2772 |
| `cr_ui_apply_undo_last` | `uint32_t (cr_ui_device* dev, cr_ui_apply_result* out)` | synchronous; idempotent on bytes | uses retained undo log | `ERR_BAD_PARAMETER` (no undo log), `ERR_APPLY_REFUSED`, `ERR_APPLY_REFRESH_FAILED` (:2852ff) | cr_ui.cpp:3189 → 2852 |
| `cr_ui_apply_save_undo` | `uint32_t (cr_ui_device* dev, const char* utf8_path)` | synchronous | writes CRUNDO01 container | `ERR_BAD_PARAMETER` (no undo log), `ERR_OPEN_FAILED`, `ERR_SESSION_IO` (:2880ff) | cr_ui.cpp:3193 → 2880 |

### 6. Sessions, list, pointed resume (cr_ui.h:908-1049)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_session_load` | `uint32_t (const char* utf8_path, cr_ui_session** out)` | no shared state; any thread | caller closes via `cr_ui_session_close` | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED`, container rc pass-through (SESSION_FORMAT/IO/VERSION) (session_load_impl:3213ff) | cr_ui.cpp:3637 → 3213 |
| `cr_ui_session_load_with_events` | `uint32_t (const char* utf8_path, cr_ui_event_fn fn, void* user, volatile uint32_t* cancel_flag, cr_ui_session** out)` | events from the CALLING thread; cancel_flag settable from ANY thread (W1b/R-54, cr_ui.h:1032-1048) | same; cancel flag CALLER-OWNED | same as load + `ERR_CANCELLED` (torn load discards tree, :3273ff) | cr_ui.cpp:3640 → 3273 |
| `cr_ui_session_close` | `void (cr_ui_session* sess)` | after last borrower released | releases handle | none | cr_ui.cpp:3659 |
| `cr_ui_session_resume_last` | `uint32_t (const char* utf8_dir, cr_ui_session** out)` | no shared state; any thread (cr_ui.h:916-927) | caller closes via `cr_ui_session_close` | `ERR_BAD_PARAMETER` (null), `ERR_RANGE_OUT_OF_BOUNDS` (no live last-scan pointer, :3698 — same code the header documents for last_autosave_path), `ERR_OPEN_FAILED`/load rc pass-through via session_load_impl (:3732ff) | cr_ui.cpp:3752 → session_resume_last_impl:3732 **[corrected — F1 resolved at a47419c7; was header-only]** |
| `cr_ui_session_get_info` | `uint32_t (cr_ui_session* sess, cr_ui_session_info* out)` | stable tree; any thread per handle discipline | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3667 → session_info_impl:3371 |
| `cr_ui_session_stats` | `uint32_t (cr_ui_session* sess, cr_ui_scan_stats* out)` | same | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3675 → session_stats_impl:3394 |
| `cr_ui_session_snapshot` | `uint32_t (cr_ui_session* sess, cr_ui_snapshot** out)` | same | caller closes snapshot | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:3678 → 3461 |
| `cr_ui_session_regrade` | `uint32_t (cr_ui_session* sess, const uint64_t* node_ids, uint32_t count, uint32_t* regraded)` | grade-1-only re-grade (cr_ui.h:789-794) | fills caller u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3670 → 3418 |
| `cr_ui_session_resume` | `uint32_t (cr_ui_session* sess, const cr_ui_scan_config* original_cfg, cr_ui_scan** out)` | mask-validated resume (cr_ui.h:949-955) | returns a FRESH scan handle | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED`, load rc pass-through (:3592ff) | cr_ui.cpp:3681 → 3592 |
| `cr_ui_session_list` | `uint32_t (const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count)` | no shared state; any thread; two-call (W1a/R-56, cr_ui.h:957-1012) | caller rows (POD, no free export) | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED` (dir unreadable), `ERR_TRUNCATED` (capacity < needed) (session_list_impl:3299ff) | cr_ui.cpp:3686 → 3299 |
| `cr_ui_session_last_autosave_path` | `uint32_t (const char* utf8_dir, char* out_path, uint32_t cap)` | no shared state (M-154, cr_ui.h:1014-1028) | copies into caller buffer | `ERR_BAD_PARAMETER`, `ERR_RANGE_OUT_OF_BOUNDS` (no live pointer, :3698 — the header comment now names this code; **[corrected — F4 fixed at 957e6170/65e0e2bd]**), `ERR_TRUNCATED` | cr_ui.cpp:3708 → 3693 |

### 7. Snapshots (cr_ui.h:1050-1110)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_snapshot_close` | `void (cr_ui_snapshot* snap)` | after last row read | releases value copy | none | cr_ui.cpp:3846 |
| `cr_ui_snapshot_node_count` | `uint32_t (cr_ui_snapshot* snap, uint32_t* out)` | immutable value; any thread | fills caller u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3854 → 3760 |
| `cr_ui_snapshot_partial` | `uint32_t (cr_ui_snapshot* snap, uint32_t* out)` | same | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3857 → 3767 |
| `cr_ui_snapshot_node` | `uint32_t (cr_ui_snapshot* snap, uint32_t index, cr_ui_node_info* out, char* name_buf, uint32_t name_cap)` | same; two-call name pattern (cr_ui.h:1099-1103) | fills caller struct/buffer | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` (name cap) | cr_ui.cpp:3860 → 3774 |
| `cr_ui_snapshot_dfs` | `uint32_t (cr_ui_snapshot* snap, uint32_t* indices, uint32_t* count)` | same; two-call (cr_ui.h:1105-1109) | caller index array | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` | cr_ui.cpp:3865 → 3823 |

### 8. Query (cr_ui.h:1118-1197)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_query_run` | `uint32_t (cr_ui_session* sess, const cr_ui_query* q, cr_ui_query_row** rows, uint32_t* row_count, uint64_t* total_matched)` | STABLE session trees only (cr_ui.h:1121-1124) | heap rows — caller frees via `cr_ui_query_rows_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:3976 → query_run_impl:3874 |
| `cr_ui_query_rows_free` | `void (cr_ui_query_row* rows, uint32_t row_count)` | any time | frees row array + name/path heap strings | none | cr_ui.cpp:3981 |
| `cr_ui_query_apply_buckets` | `uint32_t (cr_ui_query* q, uint32_t date_bucket, uint32_t size_bucket, uint64_t date_custom_from, uint64_t date_custom_to, uint64_t size_custom_min, uint64_t size_custom_max)` | pure translation, in place (M-029, cr_ui.h:1179-1197) | mutates caller query | `ERR_BAD_PARAMETER` (skipped/unknown bucket IDs 0x104/0x1005) | cr_ui.cpp:4035 → query_apply_buckets_impl:3991 |

### 9. Carve registry + user signs (cr_ui.h:1210-1281)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_carve_format_count` | `uint32_t (void)` | any thread, any time — tables immutable (cr_ui.h:1213-1224) | value return | none — `carve::registry_size() + rawsigns::kFormatCount` | cr_ui.cpp:4063 |
| `cr_ui_carve_format_info` | `uint32_t (uint32_t index, cr_ui_carve_format* out)` | same | fills caller struct | `ERR_BAD_PARAMETER` (null out / index out of range, :4068ff) | cr_ui.cpp:4068 |
| `cr_ui_register_user_signs` | `uint32_t (const char* json_bytes, uint32_t json_len)` | process-global; thread-safe (cr_ui.h:1259-1266) | replaces set atomically | `ERR_BAD_PARAMETER` (null/empty :5072, parse rc :5075, install rc :5077) | cr_ui.cpp:5073 |
| `cr_ui_user_sign_count` | `uint32_t (void)` | any thread | value return | none | cr_ui.cpp:5082 |
| `cr_ui_user_sign_info` | `uint32_t (uint32_t index, cr_ui_user_sign* out)` | snapshot-consistent read | fills caller struct | `ERR_BAD_PARAMETER` (:5086ff) | cr_ui.cpp:5086 |

### 10. Disk map (cr_ui.h:1300-1331)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_disk_map` | `uint32_t (cr_ui_session* sess, cr_ui_snapshot* snap, const cr_ui_disk_map_geometry* want, cr_ui_disk_map_geometry* got, uint8_t** allocation, uint8_t** coverage, uint8_t** found, uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap, uint32_t* bad_count)` | data producer; exactly ONE of sess/snap (cr_ui.h:1300-1321) | overlay arrays are HEAP, freed with `cr_ui_disk_map_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY`, `ERR_RANGE_OUT_OF_BOUNDS` (want view outside slice, :3524/:3540), `ERR_TRUNCATED` (bad_cap < needed) | cr_ui.cpp:3644 → disk_map_impl:3485 |
| `cr_ui_disk_map_free` | `void (uint8_t* overlay)` | any time; NULL no-op | frees ONE overlay array | none | cr_ui.cpp:3654 |

### 11. Preview / archive (cr_ui.h:1341-1404)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_preview_open_snapshot` | `uint32_t (cr_ui_device* dev, cr_ui_snapshot* snap, uint32_t index, cr_ui_preview** out)` | synchronous | private props copy; DEVICE must outlive preview (cr_ui.h:1341-1345) | `ERR_BAD_PARAMETER` (args / index / full node), `ERR_NO_MEMORY` (:4116ff) | cr_ui.cpp:4313 → 4116 |
| `cr_ui_preview_open_session` | `uint32_t (cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, cr_ui_preview** out)` | synchronous | same | `ERR_BAD_PARAMETER` (args / node / no props / full), `ERR_NO_MEMORY` (:4169ff) | cr_ui.cpp:4317 → 4169 |
| `cr_ui_preview_close` | `void (cr_ui_preview* pv)` | any time | releases handle | none | cr_ui.cpp:4321 |
| `cr_ui_preview_size` | `uint32_t (cr_ui_preview* pv, uint64_t* out)` | synchronous | fills u64 | `ERR_BAD_PARAMETER` | cr_ui.cpp:4329 → 4207 |
| `cr_ui_preview_read` | `uint32_t (cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t len, uint32_t* placed)` | synchronous; 4 MiB/call cap; short only at EOF (cr_ui.h:1353-1361) | caller buffer | `ERR_BAD_PARAMETER`; reader rc pass-through (ERR_RANGE_OUT_OF_BOUNDS / ERR_READ_FAILED; zero-fill + report for unreadable spans) | cr_ui.cpp:4332 → 4214 |
| `cr_ui_preview_metadata` | `uint32_t (cr_ui_preview* pv, cr_ui_metadata* out)` | synchronous | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:4336 → 4224 |
| `cr_ui_preview_unreadable` | `uint32_t (cr_ui_preview* pv, uint64_t* total_bytes, uint32_t* range_count)` | synchronous | fills two outs | `ERR_BAD_PARAMETER` | cr_ui.cpp:4339 → 4263 |
| `cr_ui_archive_entry_count` | `uint32_t (cr_ui_preview* pv, uint32_t* out)` | cached at first archive call (M-116, cr_ui.h:1397) | fills u32 (0 for non-archives, never an error) | `ERR_BAD_PARAMETER` | cr_ui.cpp:4343 → 4276 |
| `cr_ui_archive_entry_at` | `uint32_t (cr_ui_preview* pv, uint32_t index, cr_ui_archive_entry* out)` | same | fills caller struct | `ERR_BAD_PARAMETER`, `ERR_RANGE_OUT_OF_BOUNDS` | cr_ui.cpp:4346 → archive_entry_impl:4288 |

### 12. Thumbnail (cr_ui.h:1428-1481; M-102/M-083/M-115/N-144)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_thumbnail_generate` | `uint32_t (cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, const cr_ui_thumbnail_caps* caps, uint8_t* rgba, uint32_t rgba_cap, cr_ui_thumbnail_info* info)` | synchronous, one bounded generation (cr_ui.h:1448-1472); grade→2 promotion side effect | caller pixel buffer; two-call probe | `ERR_BAD_PARAMETER` (null out / caps range / size ≤ 20 nodes), `ERR_NO_MEMORY` (small buffer, bytes_needed set), `ERR_TRUNCATED` (decode refusal 0x50030004), `ERR_UNSUPPORTED` (phase-2 families + size ≤ 20 refusal, cr_ui.h:1459), `ERR_READ_FAILED`, `ERR_INTERNAL` — mapped at thumb_status_to_abi:4357 | cr_ui.cpp:4511 → thumbnail_generate_impl:4376 |
| `cr_ui_thumbnail_format_supported` | `uint32_t (const char* utf8_format)` | stateless probe (N-144: `txt` routes to the text-snapshot path) | value return | `ERR_BAD_PARAMETER` (null) | cr_ui.cpp:4519 → 4501 |

### 13. Recovery (cr_ui.h:1489-1550)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_recovery_create` | `uint32_t (cr_ui_device* source, const cr_ui_recovery_config* cfg, cr_ui_recovery** out)` | synchronous setup | caller closes via `cr_ui_recovery_close`; source device + every enqueued session/snapshot borrowed (ABI-CONTRACT §2) | `ERR_BAD_PARAMETER` | cr_ui.cpp:4906 → recovery_create_impl:4527 |
| `cr_ui_recovery_close` | `void (cr_ui_recovery* rec)` | after process ends | releases handle | none | cr_ui.cpp:4911 |
| `cr_ui_recovery_set_destination` | `uint32_t (cr_ui_recovery* rec, const char* utf8_path)` | synchronous; arms F12 overlap refusal | normalized internally | `ERR_BAD_PARAMETER` | cr_ui.cpp:4919 → 4588 |
| `cr_ui_recovery_enqueue_node` | `uint32_t (cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` | synchronous | enqueues borrowed node | `ERR_BAD_PARAMETER` (incl. node not found) | cr_ui.cpp:4923 → 4654 |
| `cr_ui_recovery_enqueue_subtree` | `uint32_t (cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` | synchronous | recursive walk | `ERR_BAD_PARAMETER`, `ERR_DEST_OVERLAP` (per-item, 0x50050001) | cr_ui.cpp:4927 → 4708 |
| `cr_ui_recovery_enqueue_snapshot` | `uint32_t (cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` | synchronous; race-free value path (G-02b) | borrowed snapshot row | `ERR_BAD_PARAMETER` | cr_ui.cpp:4932 → 4744 |
| `cr_ui_recovery_enqueue_subtree_snapshot` | `uint32_t (cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` | synchronous | same | `ERR_BAD_PARAMETER`, `ERR_DEST_OVERLAP` | cr_ui.cpp:4937 → 4769 |
| `cr_ui_recovery_queue_count` | `uint32_t (cr_ui_recovery* rec, uint32_t* out)` | any time | fills u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:4942 → 4836 |
| `cr_ui_recovery_process` | `uint32_t (cr_ui_recovery* rec)` | SYNCHRONOUS on caller's thread (cr_ui.h:1532) | committed items stay committed on cancel | `ERR_BAD_PARAMETER`; writer rc pass-through (ERR_CANCELLED, ERR_DEST_*, ERR_HASH_MISMATCH, ERR_READ_FAILED …) | cr_ui.cpp:4945 → recovery_process_impl:4844 |
| `cr_ui_recovery_cancel` | `uint32_t (cr_ui_recovery* rec)` | ANY thread while process() executes | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:4948 → 4858 |
| `cr_ui_recovery_get_stats` | `uint32_t (cr_ui_recovery* rec, cr_ui_recovery_stats* out)` | any time | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:4951 → recovery_stats_impl:4865 |
| `cr_ui_recovery_report_json` | `uint32_t (cr_ui_recovery* rec, const char* source_utf8, char** json_out)` | valid any time (G-09, cr_ui.h:1545-1548) | HEAP string — caller frees with `cr_ui_string_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:4955 → 4882 |

### 14. Health, image producer, Windows-only escalation (cr_ui.h:1551-1657)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_health_read` | `uint32_t (uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs, uint32_t attr_cap, uint32_t* attr_count)` | synchronous; Windows only; read-only (cr_ui.h:1551-1554) | fills report + caller attr rows; *attr_count = needed either way | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED` (elevation-aware text), `ERR_HEALTH_UNAVAILABLE` (no source) (health_read_impl:4966ff); non-Win `ERR_UNSUPPORTED` (stub :5056) | cr_ui.cpp:5065 → 4966 / stub:5056 |
| `cr_ui_image_create` | `uint32_t (uint32_t disk, const char* utf8_path, cr_ui_recovery_progress_fn progress, void* user)` | synchronous; driver-gated class (a) (S-040, cr_ui.h:1595-1614) | streamed to `<path>.crimg-tmp`, atomic rename + `.sha256` sidecar; partial KEPT as `<path>.partial` on cancel/failure | `ERR_BAD_PARAMETER`, `ERR_DRIVER_*`, `ERR_CANCELLED`, create rc pass-through (image_create_impl:1799ff); non-Win `ERR_UNSUPPORTED` (stub :1842) | cr_ui.cpp:5112 → 1799 / stub:1842 |
| `cr_ui_fsck_volume` | `uint32_t (char letter, uint32_t flags, int32_t* exit_code)` | synchronous; Windows only; 15-min internal timeout bound (M-075, cr_ui.h:1616-1635) | exit_code (may be NULL) = RAW chkdsk exit (0/1/2/3, −1 spawn-fail, 259 timeout) | OK when chkdsk ran (verdict in exit_code), `ERR_BAD_PARAMETER` (non-letter), `ERR_OPEN_FAILED` (no volume at letter — the loud refusal), `ERR_UNSUPPORTED` (not elevated or non-Windows) (fsck_volume_impl:5121ff / stub :5133) | cr_ui.cpp:5139 → 5121 / stub:5133 |
| `cr_ui_turn_off_bitlocker` | `uint32_t (char letter, cr_ui_progress_fn progress, void* user)` | synchronous; Windows only; DESTRUCTIVE consent twin of unlock (M-078, cr_ui.h:1637-1653) | none; OK means request ACCEPTED, not plaintext (OS decrypt continues in background) | `ERR_BAD_PARAMETER` (non-letter), `ERR_FVE_LOCKED` (both command forms failed — never a silent success), engine rc pass-through (turn_off_bitlocker_impl:5148ff / stub :5166); progress fires at request start + settle (stage 0xFFFFFFFF sentinels) | cr_ui.cpp:5172 → 5148 / stub:5166 |

**Guarded/catch-all coverage (re-derived): 78/93 exports route through the
`guarded()` catch-all (cr_ui.cpp:138-146 → `ERR_INTERNAL` + last_error text);
6 void close/free entries use inline `try{...}catch(...)` (device_close:1892,
scan_close:3081, session_close:3659, snapshot_close:3846, preview_close:4321,
recovery_close:4911); the remaining 9 (abi_version, last_error, string_free,
last_transport_error, disk_map_free, query_rows_free, register_user_signs,
user_sign_count, user_sign_info) are non-throwing by construction or delegate
to internally-guarded core seams (`carve::usersigns`). The "every entry has a
catch-all" rule (cr_ui.h:24-25) therefore holds via delegation for those 9.**
**[corrected — was 75/90 at Session-84; +resume_last, +fsck_volume,
+turn_off_bitlocker all use guarded()]**

**Platform split — the Linux `ERR_UNSUPPORTED` set (12 exports, per the
header's own statements):** `cr_ui_open_disk`/`cr_ui_open_volume`/
`cr_ui_open_volume_with_key` ("Windows only (platform layer): ERR_UNSUPPORTED
elsewhere", cr_ui.h:318-319), `cr_ui_driver_status` ("CR_UI_ERR_UNSUPPORTED
off-Windows", cr_ui.h:368), `cr_ui_unlock_volume_with_dictionary`
(cr_ui.h:421-422), the enumeration trio ("Windows platform layer;
ERR_UNSUPPORTED elsewhere", cr_ui.h:447), `cr_ui_health_read` (cr_ui.h:1551),
`cr_ui_image_create` ("Win-backed: on non-Windows lanes returns
ERR_UNSUPPORTED", cr_ui.h:1612), `cr_ui_fsck_volume` (cr_ui.h:1632), and
`cr_ui_turn_off_bitlocker` (cr_ui.h:1652). Impl stubs: cr_ui.cpp:1727, 1736,
1741, 1749, 1758, 1842, 2009, 2014, 5056, 5133, 5166 (enumerate_disks_ex
shares the enumerate_disks_impl stub path). Everything else is portable
(image-backed, session/query/preview/recovery, registry/user-signs).

---

## Structs

All structs are POD, C-layout stable, append-only (ABI-CONTRACT §9). No struct
documents a byte size in-header; only array field widths are pinned. Zero-init
is always safe (cr_ui.h:30-31).

| struct | fields (types) | semantics / notes | cite |
|---|---|---|---|
| `cr_ui_event` | `uint32_t type, stage; uint64_t timestamp_ms; uint64_t bytes_done, bytes_total; uint32_t stage_permille, overall_permille; uint64_t files_found, deleted_found, carved_found; uint64_t err_start_bytes, err_len_bytes; uint32_t error_ranges; uint64_t checkpoint_index; uint64_t candidate_lba; uint32_t repair_actions, repair_outcome; uint32_t err_module, err_operation, err_attempts, err_context_pad; uint64_t check_errors; uint32_t check_family, check_pad; uint64_t autosave_rc` | the typed event POD; serializeable byte-for-byte; trailing pads keep it an 8-byte multiple (always 0); v1-era append = repair pair; v2-era = err_context + check pair; W1c append = autosave_rc (:269-271) | cr_ui.h:225-272 |
| `cr_ui_driver_status_t` | `uint32_t state, service_present, service_running, device_openable, authorized, info_valid, major, minor, required_major, flags, max_disks, ioctl_base, last_gle` | 6-facet driver status; state = CR_UI_DRIVER_STATE_*; last_gle = diagnostic Win32 error | cr_ui.h:379-394 |
| `cr_ui_geometry` | `uint64_t total_bytes; uint32_t logical_sector, physical_sector; uint64_t sector_count` | 4Kn-aware geometry out | cr_ui.h:430-435 |
| `cr_ui_disk_info` | `uint32_t number; uint64_t size_bytes; uint32_t logical_sector, physical_sector, removable, layout, table_ok; char model[128]; char serial[128]; uint32_t trim_known, trim_on` | census row; layout 0 none/1 MBR/2 GPT/3 EBR (N-057); M-076 TRIM append (zero-init = unknown) | cr_ui.h:449-473 |
| `cr_ui_volume_info` | `char letter; uint32_t disk; uint64_t offset_on_disk, bytes; char fs_name[16]; char label[64]; uint32_t bitlocker` | census row; bitlocker 1 = FVE detected | cr_ui.h:475-483 |
| `cr_ui_repair_plan` | `uint32_t detected; char fs_family[8]; uint32_t action_count, refused; char reason[160]; uint32_t advisory_count` | S-008 consent-gate POD summary (fired via callback) | cr_ui.h:550-558 |
| `cr_ui_scan_config` | `uint32_t stage_mask, whole_disk, fs_hint, quick_weight, deep_weight, carve_weight, threads; const char* volume_label_utf8, volume_model_utf8, volume_serial_utf8, volume_firmware_utf8, volume_wwn_utf8, volume_interface_utf8, checkpoint_path_utf8; cr_ui_progress_fn progress; void* progress_user; uint32_t pre_scan_repair; cr_ui_repair_consent_fn repair_consent; void* repair_consent_user; uint32_t memory_warn_mb; const char* raw_category_name_utf8; uint32_t pre_scan_check; const char* autosave_dir_utf8; const char* const* filter_folders_utf8; uint32_t filter_folder_count` | zero-init = defaults; stage_mask 0 = 7; label truncated to 99 chars (N-090); v1/v2-era + W1c + M-073 filter appends all tail-appends | cr_ui.h:562-664 |
| `cr_ui_stage_tel` | `uint32_t permille; uint64_t units_done, units_total, bytes_done, elapsed_ms; uint32_t completed` | per-stage telemetry; elapsed pause-compensated | cr_ui.h:698-705 |
| `cr_ui_telemetry` | `uint32_t overall_permille; cr_ui_stage_tel quick, deep, carve; uint64_t files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms; uint32_t error_ranges, paused, running, cancelled` | live pull snapshot; bytes_scanned = carve-true bytes | cr_ui.h:707-715 |
| `cr_ui_scan_stats` | `uint64_t files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms, cached_hits, cached_misses; uint32_t error_ranges; uint32_t repair_ran, repair_planned, repair_applied, repair_denied; uint32_t check_ran, check_errors, check_detected; uint64_t bytes_unreadable; uint32_t probe_unreadable; uint32_t autosaved` | final counters; v1-era repair + v2-era check + driver-only read-truth + W1c autosaved appends | cr_ui.h:720-756 |
| `cr_ui_assistant_stats` | `uint64_t total_file_count, raw_count, lost_partition_exist_file_count, lost_partition_delete_file_count, quick_scan_exist_file_count, quick_scan_delete_file_count, deep_scan_file_count, quick_scan_delete_extend; uint64_t category_counts[8]; uint64_t matched, total; uint32_t small_result; uint64_t graded_count` | S-023 grading statistics (og RecoveryAssistant); matched forced 0 when total < 100 | cr_ui.h:766-782 |
| `cr_ui_apply_candidate` | `uint64_t start_lba, sector_count; uint32_t fs; uint32_t disk_layout, disk_flags` | S-029 planner input; fs = lost::CandidateFs values; M-137 presentation tail (zero-init whole struct) | cr_ui.h:814-832 |
| `cr_ui_apply_plan` | `uint32_t refused, scope; uint64_t start_lba, sector_count; uint32_t mbr_type, slot, regenerate_signature, signature; char reason[160]` | planner output; scope 0 single-entry / 1 rebuild-table | cr_ui.h:834-844 |
| `cr_ui_apply_result` | `uint32_t applied, outcome; uint64_t undo_entries; char reason[160]` | outcome 0..5 (applied / denied / refused / stale / refresh-failed / error) | cr_ui.h:848-856 |
| `cr_ui_session_info` | `uint32_t fs_family, stage_done_mask, configured_mask; uint64_t slice_offset, slice_bytes; uint32_t logical_sector, physical_sector; char label[32]; char label_full[100]; uint64_t scanned_bytes, elapsed_ms; char model[48]; char serial[32]; uint32_t boot_hash` | loaded-session summary; N-090 label_full append (== label on v1..v5); boot_hash 0 = unverifiable | cr_ui.h:929-943 |
| `cr_ui_session_row` | `uint32_t valid, error, fs_family, stage_done_mask, configured_mask; uint64_t files_found, deleted_found, carved_found, bytes_scanned, elapsed_ms, save_unix; char label[32]; char path[1024]; char label_full[100]` | W1a listing row; `CR_UI_SESSION_ROW_PATH 1024`; N-090 label_full append; save_unix 0 = pre-v5 | cr_ui.h:992-1009 **[corrected +2]** |
| `cr_ui_node_info` | `uint32_t parent, first_child, next_sibling, prev_sibling; uint64_t node_id; uint32_t fs_kind; uint64_t record_ref; uint32_t origin, deleted, confidence, grade, overlap, is_full; uint64_t logical_size, allocated_size; uint64_t created, modified, changed, accessed; uint32_t attributes, flags, extent_count, resident_bytes, carve_format, carve_confidence, hash_valid; uint8_t sha128[16]; uint32_t name_len; uint64_t version_group; uint32_t tags_extracted` | snapshot row; links are indices (CR_UI_SNAP_NONE 0xFFFFFFFF); grade = tree::Grade 0..2 (S-023); timestamps 100ns-1601; **M-101 tail append: `version_group` = the NTFS namespace-identity hash (0 = none), `tags_extracted` = the preview/metadata pass's mark — the merge (69b2ff9a) settled the racing window's uint32 form to uint64 + expanded comments** | cr_ui.h:1061-1093 **[corrected — M-101]** |
| `cr_ui_query` | `uint32_t category_mask; uint64_t size_min, size_max, modified_from, modified_to; uint32_t confidence_min; const char* const* extensions; uint32_t extension_count; const char* name_substring, path_prefix, path_substring; uint32_t state_mask, deleted_only, origin_mask, sort, dir; uint64_t offset; uint32_t limit, count_only; uint64_t version_group; uint32_t tags_only` | zero-init = match-all; M-072 path_substring/state_mask appends; limit 0 = default 100; **M-101 tail append: `version_group` (0 = no filter; nonzero = that group's members — enumerate live groups first) + `tags_only` (the Tags Extracted branch); the racing window's `versions_only`/`tags_extracted_only` u32 pair was re-shaped to this u64+u32 form at the 69b2ff9a merge** | cr_ui.h:1129-1158 **[corrected — M-101]** |
| `cr_ui_query_row` | `uint64_t node_id; char* name; char* path; uint64_t size, modified; uint32_t origin, deleted, confidence, category, grade; uint64_t version_group; uint32_t tags_extracted` | HEAP name/path — free whole array with `cr_ui_query_rows_free`; M-101 branch-axis tail (u64 hash + u32 mark) | cr_ui.h:1160-1171 **[corrected — M-101]** |
| `cr_ui_carve_format` | `uint32_t layer; char ext[24]; uint64_t min_size, max_size; uint16_t base_confidence; uint32_t magic_len, magic_offset; uint8_t magic[16]` | layer 0 grammar / 1 raw-sign; magic fields grammar-layer only | cr_ui.h:1210-1219 |
| `cr_ui_user_sign` | `char ext[9]; char mime[81]; uint32_t offset, sign_len; uint8_t bytes[48]` | v3/M-067 widths (bytes 16→48, mime 64→81; offset ≤ 1024, sign_len 1..48) — the field resize that justified ABI v3 | cr_ui.h:1268-1275 |
| `cr_ui_disk_map_geometry` | `uint64_t start_byte, total_bytes, cell_bytes, cell_count; uint32_t cluster_bytes, fs_family` | in: view hints (want); out: actually-used geometry (got); cell_bytes 0 = auto | cr_ui.h:1312-1319 |
| `cr_ui_metadata` | `char kind[24]; char format[24]; uint32_t width, height; char orientation[8]; char datetime[40]; uint64_t duration_ms; char codec[32]; char text_snippet[256]; uint32_t truncated` | fixed-capacity UTF-8; truncation reported via `truncated` | cr_ui.h:1359-1369 |
| `cr_ui_archive_entry` | `char name[260]; uint64_t size, compressed; uint32_t crc_present, name_truncated` | M-116 inventory row; archive-native UTF-8 pass-through | cr_ui.h:1389-1395 |
| `cr_ui_thumbnail_caps` | `uint32_t max_edge; uint64_t max_pixels; uint32_t max_read, max_work, allow_embedded` | 0-field = engine default (256 / 64 MP / 200 MB / 4 Mi / embedded ON — no OFF position by design, N-137) | cr_ui.h:1428-1434 |
| `cr_ui_thumbnail_info` | `uint32_t width, height, bytes_needed; char source_format[16]; uint8_t from_embedded, truncated; uint8_t _pad[2]` | output dims + two-call probe answer; truncated crosses Ok-but-truncated decodes | cr_ui.h:1436-1446 |
| `cr_ui_recovery_config` | `uint64_t read_chunk; uint32_t rename_attempts, skip_hash_verify, skip_dir_reconstruct, conflict; cr_ui_recovery_progress_fn progress; void* progress_user` | defaults 1 MiB / 64 / verify ON / reconstruct ON / conflict 0 = rename | cr_ui.h:1489-1497 |
| `cr_ui_recovery_stats` | `uint64_t queued, done, refused, errors, hash_mismatch, resumed, salvaged, dirs; uint64_t nothing_to_recover` | v1-era `nothing_to_recover` append (zero-init safe) | cr_ui.h:1535-1541 |
| `cr_ui_smart_attr` | `uint32_t id, value, worst; uint64_t raw; uint32_t failing; char name[40]` | one SMART attribute row (raw = 6 vendor bytes) | cr_ui.h:1555-1561 |
| `cr_ui_smart_report` | `uint32_t available; char transport[24]; uint32_t ata_version, smart_version, attribute_count, failing_now; uint64_t reallocated_sectors, pending_sectors, uncorrectable, power_on_hours, spin_retry, ssd_wear_level, temperature_c; uint32_t nvme_available, temperature_c_x10, available_spare, available_spare_threshold; uint64_t percentage_used, data_units_read, data_units_written, power_on_hours_nvme, unsafe_shutdowns, media_errors; uint32_t critical_warning; uint32_t ssd_like, trim_known, trim_enabled; uint64_t host_reads, host_writes, controller_busy_time, num_error_log_entries, data_bytes_read, data_bytes_written` | ABI v2 (M-036) tail: the last six NVMe fields + data-unit→byte conversion | cr_ui.h:1563-1589 |

Opaque handle types (cr_ui.h:93-99): `cr_ui_device`, `cr_ui_scan`,
`cr_ui_session`, `cr_ui_snapshot`, `cr_ui_preview`, `cr_ui_recovery` — opaque
by design; internal impl structs `CrDevice`, `CrScan`, `CrSession`,
`CrSnapshot`, `CrPreview`, `CrRecovery` (cr_ui.cpp, anonymous namespaces).

---

## Enums

### The error-code enum — `cr::Err` (include/cr/cr_error.h:25-117), u32 values

**True census: `OK` + 44 error codes = 45 values.** **[corrected — was 43
codes at Session-84; M-105 (c4d8dcee) added `ERR_FVE_ELEVATION_REQUIRED`;
doc_truth_gate's enum-member census now derives 44 (tools/doc_truth_gate.py:313-329)]**

| family | code | symbol | cr_error.h:line |
|---|---|---|---|
| — | 0x00000000 | `OK` | :26 |
| open/transport 0x5002 | 0x50020001 | `ERR_OPEN_FAILED` | :29 |
| | 0x50020002 | `ERR_READ_FAILED` | :30 |
| | 0x50020003 | `ERR_WRITE_FAILED` | :31 |
| | 0x50020004 | `ERR_RANGE_OUT_OF_BOUNDS` | :32 |
| | 0x50020005 | `ERR_CANCELLED` | :33 |
| | 0x50020006 | `ERR_RETRY_EXHAUSTED` | :34 |
| | 0x50020007 | `ERR_DEVICE_DISCONNECTED` (N-125) | :42 |
| format/parse 0x5003 | 0x50030001 | `ERR_BAD_MAGIC` | :45 |
| | 0x50030002 | `ERR_BAD_CHECKSUM` | :46 |
| | 0x50030003 | `ERR_BAD_PARAMETER` | :47 |
| | 0x50030004 | `ERR_TRUNCATED` | :48 |
| | 0x50030005 | `ERR_FIXUP_FAILED` | :49 |
| | 0x50030006 | `ERR_CYCLE_LIMIT` | :50 |
| | 0x50030007 | `ERR_DEPTH_LIMIT` | :51 |
| | 0x50030008 | `ERR_UNSUPPORTED` | :52 |
| | 0x50030009 | `ERR_NO_MEMORY` | :53 |
| session 0x5004 | 0x50040001 | `ERR_SESSION_FORMAT` | :56 |
| | 0x50040002 | `ERR_SESSION_IO` | :57 |
| | 0x50040003 | `ERR_SESSION_VERSION` | :58 |
| writer 0x5005 | 0x50050001 | `ERR_DEST_OVERLAP` | :61 |
| | 0x50050002 | `ERR_DEST_UNWRITABLE` | :62 |
| | 0x50050003 | `ERR_HASH_MISMATCH` | :63 |
| | 0x50050004 | `ERR_DEST_FULL` (N-015) | :67 |
| | 0x50050005 | `ERR_DEST_NAME_EXHAUSTED` (N-065) | :71 |
| | 0x50050006 | `ERR_DEST_PATH_TOO_LONG` (M-074) | :75 |
| | 0x50050007 | `ERR_DEST_PATH_INVALID` (M-074) | :76 |
| | 0x50050008 | `ERR_DEST_NETWORK` (M-074) | :77 |
| bitlocker 0x5006 | 0x50060001 | `ERR_FVE_LOCKED` | :80 |
| | 0x50060002 | `ERR_FVE_ELEVATION_REQUIRED` (M-105, c4d8dcee) | :87 **[new]** |
| health 0x5007 | 0x50070001 | `ERR_HEALTH_UNAVAILABLE` | :90 |
| repair/apply 0x5008 | 0x50080001 | `ERR_REPAIR_REFUSED` | :91 |
| | 0x50080002 | `ERR_REPAIR_UNDO_FAILED` | :92 |
| | 0x50080003 | `ERR_VOLUME_LOCK_FAILED` (S-010) | :93 |
| | 0x50080004 | `ERR_APPLY_CONSENT_DENIED` (S-029 A1) | :96 |
| | 0x50080005 | `ERR_APPLY_STALE` (A2) | :97 |
| | 0x50080006 | `ERR_APPLY_REFUSED` (A3) | :98 |
| | 0x50080007 | `ERR_APPLY_REFRESH_FAILED` (A4) | :99 |
| internal 0x5009 | 0x50090001 | `ERR_INTERNAL` | :106 |
| image 0x500A | 0x500A0001 | `ERR_IMAGE_CORRUPT` (S-030) | :103 |
| driver 0x500B | 0x500B0001 | `ERR_DRIVER_ABSENT` | :112 |
| | 0x500B0002 | `ERR_DRIVER_NOT_STARTED` | :113 |
| | 0x500B0003 | `ERR_DRIVER_UNAUTHORIZED` | :114 |
| | 0x500B0004 | `ERR_DRIVER_VERSION_MISMATCH` | :115 |
| | 0x500B0005 | `ERR_DRIVER_ERRORED` | :116 |

`const char* err_text(u32)` (cr_error.h:124) is the shared text surface.
Header comment pins the space as CORE-INTERNAL (D-68; cr_error.h:4-13).
Family order in source: 0x5009 INTERNAL is declared at :106 AFTER 0x500A
IMAGE_CORRUPT at :103 (cosmetic; values are what matter).

### ABI event-type macros — `CR_EVT_*` (include/cr/cr_ui.h:151-223) — 12 values

| value | symbol | cite | note |
|---|---|---|---|
| 1 | `CR_EVT_SCAN_STAGE` | cr_ui.h:151 | stage transition |
| 2 | `CR_EVT_SCAN_PROGRESS` | cr_ui.h:152 | coalesced progress sample |
| 3 | `CR_EVT_FILES_FOUND` | cr_ui.h:153 | cumulative counters |
| 4 | `CR_EVT_ERROR_RANGE` | cr_ui.h:154 | delta span + M-049 module/op/attempts triple |
| 5 | `CR_EVT_CHECKPOINT` | cr_ui.h:155 | G-13 checkpoint written |
| 6 | `CR_EVT_LOST_CANDIDATE` | cr_ui.h:156 | lost-partition candidate |
| 7 | `CR_EVT_REPAIR` | cr_ui.h:157-162 | S-008 outcome 0..3 + repair_actions |
| 8 | `CR_EVT_HEARTBEAT` | cr_ui.h:163-173 | N-043 watchdog, 5 s, standing truth |
| 9 | `CR_EVT_SAVE_PROGRESS` | cr_ui.h:174-187 | N-060 save-path progress |
| 10 | `CR_EVT_FS_CHECK` | cr_ui.h:188-194 | M-041 damage-audit done (fires once) |
| 11 | `CR_EVT_SESSION_LOAD_PROGRESS` | cr_ui.h:195-209 | W1b/R-54 load-path progress |
| 12 | `CR_EVT_AUTOSAVED` | cr_ui.h:210-223 | W1c/R-52 auto-save outcome |

(Core-side `orchestrator::event::Type` (cr_scan.h:135-151) carries only 8 of
these — 1..7 and 10; types 8, 9, 11, 12 are shim-side per the note at
cr_scan.h:143-146.)

### Other public constant sets

| set | values | cite |
|---|---|---|
| ABI error macros | `CR_UI_OK 0`, `CR_UI_ERR_INTERNAL 0x50090001`, `CR_UI_ERR_BAD_PARAMETER 0x50030003`, `CR_UI_ERR_CANCELLED 0x50020005`, `CR_UI_ERR_TRUNCATED 0x50030004`, `CR_UI_ERR_UNSUPPORTED 0x50030008`, `CR_UI_ERR_NO_MEMORY 0x50030009`, `CR_UI_ERR_BAD_CHECKSUM 0x50030002` | cr_ui.h:84-91 |
| driver states | `CR_UI_DRIVER_STATE_READY 0 … ERRORED 5` (6 values) | cr_ui.h:372-377 |
| error-context modules | `CR_ERRMOD_NONE 0 … CR_ERRMOD_LDM 12` (13 values; pinned to `cr::errmod::Module` by static_asserts) | cr_ui.h:283-295; cr_errmod.h:50-64 |
| error-context ops | `CR_ERROP_NONE 0`, `CR_ERROP_READ 1` | cr_ui.h:297-298 |
| enumeration flags | `CR_ENUM_FLAG_TRIM 0x1` | cr_ui.h:489 |
| snapshot sentinel | `CR_UI_SNAP_NONE 0xFFFFFFFF` | cr_ui.h:1056 |
| session-row path width | `CR_UI_SESSION_ROW_PATH 1024` | cr_ui.h:990-992 |
| core module enum | `errmod::Module kNone 0 … kLdm 12` | cr_errmod.h:50-64 |
| log levels | `log::Level Error 0 / Warn 1 / Info 2` | cr_log.h:48-52 |
| fs-kind codes | `FsKindCode Fat12 0, Fat16 1, Fat32 2, Ntfs 3, Ext2 4, Raw 6, Exfat 7, Hfsplus 8, Ext4 9, Refs 10, Btrfs 11, Zfs 13` | cr_types.h:81-101 |

---

## Callbacks

| callback | signature | when invoked | threading | cite |
|---|---|---|---|---|
| `cr_ui_progress_fn` | `void (*)(void* user, uint32_t stage, uint32_t stage_permille, uint32_t overall_permille, uint64_t bytes_done, uint64_t bytes_total, uint32_t error_ranges)` | live scan progress (quick/deep/carve); stage 0/1/2/0xFFFFFFFF; permilles 0..1000 | worker thread(s) only; must not block; NO cancellation semantics (cr_ui.h:115-119); impl serializes under `progress_mu` + monotonic CAS dedup | cr_ui.h:121-127 |
| `cr_ui_recovery_progress_fn` | `void (*)(void* user, uint64_t bytes_done, uint64_t bytes_total)` | cumulative bytes during `cr_ui_recovery_process` and `cr_ui_image_create` (64 KiB chunks) | thread running process()/image_create; must not block | cr_ui.h:128-130 |
| `cr_ui_event_fn` | `void (*)(void* user, const cr_ui_event* evt)` | the typed event stream (12 types); SAVE/LOAD progress fire from the calling thread of save/load | engine worker threads, the run() thread, the shim's heartbeat watchdog thread (N-043), or the save/load caller; no shim-side serialization (single-flight is the sink's job); must not block; no cancellation; unknown types MUST be ignored (cr_ui.h:141-150, :274) | cr_ui.h:132-150, 274; impl trampoline cr_ui.cpp:1066, heartbeat :1137 |
| `cr_ui_repair_consent_fn` | `uint32_t (*)(void* user, const cr_ui_repair_plan* plan)` | exactly ONCE per run() when `pre_scan_repair` set and a repairable plan exists | on the run() thread, before any engine read; nonzero return = execute transactional repair (default-deny on NULL/zero) | cr_ui.h:534-560 |
| `cr_ui_apply_consent_fn` | `uint32_t (*)(void* user, const cr_ui_apply_plan* plan)` | exactly ONCE per apply, before any write | synchronous on the caller's thread; zero/NULL = denied, nothing written | cr_ui.h:846 |
| `cr::log::SinkFn` | `void (*)(cr::log::Level, const char* line, void* user)` (C++, exported via .def) | every engine log line (Error/Warn/Info) | ANY thread, called under the seam's internal mutex; sink must be cheap or internally deferred; `set_sink` any time (cr_log.h:54-59) | cr_log.h:54-59 |

---

## Versioning/lifecycle

- **`CR_UI_ABI_VERSION` = 3** (cr_ui.h:81). v2 = M-036 (six `cr_ui_smart_report`
  tail fields); v3 = M-067 (`cr_ui_user_sign` field resize: bytes 16→48, mime
  64→81, sign_len 1..48, offset ≤ 1024 — the first FIELD change under the
  policy). Compatibility policy (cr_ui.h:39-48 = ABI-CONTRACT §9 at
  ABI-CONTRACT.md:768-782): adding exports / appending struct fields / adding
  event types are COMPATIBLE (no bump); removing/renaming exports, changing
  field type/meaning/order, changing an existing symbol's ownership or
  threading contract, renumbering error symbols → bump. There is no v1.1/v1.2
  (N-040 doc-defect cleanup).
- **No global init/shutdown**: the library is initialization-free
  (`cr_ui_carve_format_count` etc. callable "before any other call — no init
  ordering", ABI-CONTRACT §7a). The gate is `cr_ui_abi_version()` call-first
  (cr_ui.h:103-104).
- **Scan lifecycle** (ABI-CONTRACT §5, cr_ui.h:521-527): `scan_create` →
  `scan_attach_device` (borrow; must outlive) → `scan_run` (synchronous,
  one-shot; double-run = ERR_BAD_PARAMETER, cr_ui.cpp:2413) →
  snapshot/save → `scan_close` → `device_close`. Cancellation returns
  ERR_CANCELLED with the tree browsable; save-on-cancel is the caller's choice.
- **Session lifecycle**: `session_load[_with_events]` → query/snapshot/preview/
  regrade → `session_close`; resume: `session_load` → `session_resume` →
  attach → `verify_identity` → `run`; pointed resume (M-154):
  `session_resume_last(utf8_dir)` reads the last_scan.info pointer sidecar
  (session/cr_lastscan.h) and delegates to session_load (cr_ui.cpp:3732-3747).
- **Recovery lifecycle**: `recovery_create` → `set_destination` → `enqueue_*`
  → `process` (caller thread) → `report_json` → `close`.
- **Save/auto-save**: `cr_ui_scan_save_session` is atomic (staging `.tmp` +
  rename; write_session_atomic cr_ui.cpp:2340/:2954, N-014/N-060);
  `cr_ui_scan_save_cancel` is save-scoped; completion auto-save (W1c) fires
  CR_EVT_AUTOSAVED under the S-044 gates and — since S-057 — also on a
  user-STOPPED run with results (cr_ui.cpp:2491-2511).
- **Threading model (ABI-CONTRACT §3 = header cr_ui.h:34-38; doc restatement
  ABI-CONTRACT.md:55-75)**: run()/process() synchronous on the caller's
  thread; pause/resume/cancel/telemetry/snapshot/busy/stopping/save_cancel
  safe from any thread; one handle = one controlling thread otherwise; events
  may arrive on worker/watchdog threads (ABI-CONTRACT.md:72, :345-346).
  Implemented with per-scan `ctrl_mu`/`progress_mu`/`heartbeat_mu` +
  `running`/`save_cancel` atomics, a heartbeat watchdog thread
  (cr_ui.cpp:1137), and a process-global `g_last_transport_error` atomic
  (cr_ui.cpp:1417).

---

## Export list vs header

- `src/ui/cr_ui_exports.def` lists **exactly one symbol** (cr_ui_exports.def:19):
  `?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z` — the MSVC x64 decorated
  form of `void cr::log::set_sink(void (*)(cr::log::Level, const char*),
  void*)`, declared at cr_log.h:59. This is deliberate (the .def header comment
  :1-16: the shim's own test drives the log sink seam; on Linux the statically
  linked cr_core objects keep default visibility). It is added to the build
  only under MSVC (src/ui/CMakeLists.txt:31-33).
- **No C ABI symbol is .def-listed** — the 93 implemented `cr_ui_*` functions
  export via `__declspec(dllexport)` (CR_UI_BUILD) / visibility("default").
  Cross-check of all 93 at this tree: header signature ↔ cpp definition
  name/arity identical (extraction: 93 decls, 93 defs, set difference empty
  both ways). **[corrected — the Session-84 mismatch (phantom
  `cr_ui_session_resume_last`) is gone; F1 resolved at a47419c7]**
- The single .def symbol IS declared in a public header (cr_log.h:59) — no
  .def-only phantom. No header-declared C symbol is missing from the .def
  (none are expected there under the dllexport mechanism).

---

## The contract docs (scope + currency, re-read at 3b203fe1)

| doc | scope (its own words) | claims current? | function-name spot-check vs header |
|---|---|---|---|
| docs/ABI-CONTRACT.md (896 lines) | "Status: normative… the human contract behind every symbol: ownership rules, threading rules, the error model, the typed event stream (§6), the disk-map geometry model (§7), the carve-format registry (§7a), and the stability policy (§9). Where a statement here and in the header disagree, the HEADER wins" (:3-10) | Yes — normative, living | Not a function index (54/93 names appear; families via patterns like `preview_open_*`, `recovery_enqueue_[node|subtree][_snapshot]*`, `enumerate_disks[_ex]` at :135-162). Embedded signature copies verified exact: set_event_fn (:315), disk_map (:418-425), carve pair (:465-466), session_list (:516), load_with_events (:584-585), image_create (:691). Error table is "Selected codes" (:79) and includes the new 0x50060002 (:93). **Drift: 9 exports have ZERO ABI-CONTRACT coverage** — `cr_ui_fsck_volume`, `cr_ui_turn_off_bitlocker`, `cr_ui_session_resume_last`, `cr_ui_session_last_autosave_path`, `cr_ui_scan_carve_tally(_count)`, `cr_ui_unlock_volume_with_dictionary`, `cr_ui_driver_status`, `cr_ui_health_read` (M-075/M-078/M-154/N-104-era surfaces; the doc's "behind every symbol" preamble over-promises). **Stale cite: :585 "cr_ui.h:926-944" for load_with_events — current decl is cr_ui.h:1049** (comment 1032-1048). §6 event copy now carries `autosave_rc` (:307) ✓ (D6 fixed). |
| docs/API.md (293 lines) | "the single entry point for every programming surface the core exposes, at the current tree" (:3-5) | Yes — claims currency | §2 ABI count: "93 exported functions… 96 CR_UI_API lines minus 3 #define macros" (:99-102) — matches the tree exactly ✓. §2.1-2.6 tables cover the surface by family; genuinely name-absent (covered only via CLI verbs or not at all): the archive ABI pair, `cr_ui_image_create` (CLI `image` verb documented :51-52 instead), `cr_ui_recovery_cancel`, carve-tally pair, `resume_last`, `last_autosave_path`, fsck/turn-off (no rows yet). |
| docs/CORE-API-CURRENT.md (611 lines) | "the current public API surface (v0.4.1-parity tree, HEAD 22e9657)" + "This file inventories the surface as it EXISTS" (:1, :7-8) | Partially — headline counts current, LIVE banner + superseded banners, but title pins an old HEAD and several inner sections are stale (below) | Headline: "93 exports (96 CR_UI_API lines minus 3 #define… recount matches)" ✓ current. §(b) is STILL the pre-driver-only census: "every export (78; CR_UI_ABI_VERSION 2, cr_ui.h:79)" (:175) and b.13 "type 1..10" (:304) — F7 persists. §(d).1 "FULL enumerated list" added ERR_FVE_ELEVATION_REQUIRED (:457) but still omits ERR_DEVICE_DISCONNECTED, the M-074 ERR_DEST_PATH_* trio, and all five ERR_DRIVER_* codes, and still asserts "there is NO code for driver-absent…" (~:469) — F8 persists. ":13 C++ public headers 82 under include/cr/**" — stale, tree has 85 module + 9 top-level = 94 (F9 worsened by 3). |
| docs/API-SURFACE-AUDIT.md (190 lines) | "Session: core-api (2026-10-26). Baselines: core e6d5b01, driver 7e88e38, corpus 03ad46c… nothing here is projected from memory" (:3-7) — a point-in-time og-vs-core parity AUDIT | No (historical by design); one re-derived count line inserted later | :32-33 "93 exports (re-derived 2026-10-30, the M-075 lane: **95** CR_UI_API lines - 3 #defines…)" — headline 93 ✓ but the derivation is stale AND self-inconsistent (95−3=92; the tree has 96 lines; API.md's same-lane note says 96). The F5 class survives here. |

---

## Doc claims vs tree (re-verified at 3b203fe1)

| claim | doc cite | tree truth | verdict |
|---|---|---|---|
| "93 exports" | CORE-API-CURRENT.md:14-15; API.md:99-101; API-SURFACE-AUDIT.md:32 | 93 declared = 93 implemented (96−3; extraction cross-check empty both ways) | MATCHES ✓ (header AND DLL) **[corrected — F2 resolved]** |
| "…= 93 `extern "C" CR_UI_API` definitions in src/ui/cr_ui.cpp, recount matches" | CORE-API-CURRENT.md:15-16 | true — 93 definitions (verified: 93 defs, 93 unique symbols) | MATCHES ✓ **[corrected — was false at Session-84]** |
| "96 `CR_UI_API` lines minus 3 `#define` macros" | API.md:101; CORE-API-CURRENT.md:14 | tree has 96 CR_UI_API lines (96−3=93) | MATCHES ✓ **[corrected — API.md's derivation fixed by the N-150 pass]** |
| "95 CR_UI_API lines - 3 #defines" (deriving 93) | API-SURFACE-AUDIT.md:32-33 | tree has 96 lines; 95−3=92 contradicts the 93 in the same sentence | STALE derivation (F5 class persists HERE only) |
| "error codes **44 + OK**" | CORE-API-CURRENT.md:18; :47 | enum census = 44 codes + OK (45 values) — M-105's ERR_FVE_ELEVATION_REQUIRED (cr_error.h:87) is the 44th; gate derives 44 (doc_truth_gate.py:327) | headline MATCHES ✓ **[corrected]** — but the same line's parenthetical "43 ERR_ members in cr_error.h" is now stale by one (should be 44) |
| "ABI event types **12**" | CORE-API-CURRENT.md:22; API-SURFACE-AUDIT.md §1 | 12 `CR_EVT_*` macros, values 1..12 (cr_ui.h:151-223) | MATCHES ✓ |
| `CR_UI_ABI_VERSION` = 3 | API.md:98; ABI-CONTRACT.md:770 | cr_ui.h:81 = 3u | MATCHES ✓ |
| ABI-CONTRACT §6 inline `cr_ui_event` copy | ABI-CONTRACT.md:261-311 | copy carries `uint64_t autosave_rc` (:307) = header (:269-271) | MATCHES ✓ **[corrected — F6 fixed at 65e0e2bd]** |
| §(b) "every export (78; CR_UI_ABI_VERSION 2, cr_ui.h:79)"; b.13 "type 1..10" | CORE-API-CURRENT.md:175, :304 | 93 decls / 93 defs, ABI 3 (cr_ui.h:81), events 1..12 | STALE (F7 persists — pre-driver-only residue in a section billed cite-refreshed) |
| §(d).1 "the FULL enumerated list" | CORE-API-CURRENT.md:428-472 | table omits ERR_DEVICE_DISCONNECTED (cr_error.h:42), ERR_DEST_PATH_TOO_LONG/INVALID/NETWORK (:75-77), ERR_DRIVER_* ×5 (:112-116); ~:469 asserts NO driver codes exist; ERR_FVE_ELEVATION_REQUIRED WAS added (:457) | stale "FULL" list (F8 persists, minus the FVE row) |
| "C++ public headers 82 under include/cr/**" | CORE-API-CURRENT.md:13 | 85 per-module headers + 9 top-level = 94 files under include/cr/** (M-075 added check/cr_fsck_launch.h + win/cr_fsck.h; M-105 added health/cr_fve_policy.h) | STALE by 3 (F9 worsened) |
| events fire "on worker threads + watchdog" | ABI-CONTRACT.md:72, :345-346; cr_ui.h:141-150 | matches impl (heartbeat_loop_ cr_ui.cpp:1137) | MATCHES ✓ |
| §5 phantom names `session_disk_map`/`scan_disk_map` (A-ABI-4) | — | ABI-CONTRACT.md:170 names `cr_ui_disk_map` | FIXED ✓ |
| SQLite sidecar claim (A-ABI-2) | — | ABI-CONTRACT.md:751-766 is the REJECTED-BY-REFERENCE closure; zero sqlite in src/ | FIXED ✓ |
| `cr_ui_session_last_autosave_path` errors | cr_ui.h:1019-1023 | header comment now names `ERR_RANGE_OUT_OF_BOUNDS` (0x50020004); impl returns it (cr_ui.cpp:3698); C gate pins 0x50020004 (tests/ui_abi_check.c:592-593) | FIXED ✓ **[corrected — F4 resolved at 957e6170]** |
| "each call re-queries the OS — no cache" (enumerate) | API.md:118; ABI-CONTRACT.md:150-156 | enumerate_disks_impl constructs a fresh list per call (cr_ui.cpp:1916-1920) | MATCHES ✓ |
| driver-only refusal taxonomy (open_disk etc.) | ABI-CONTRACT.md:87-97, §7e | open_disk/open_volume/with_key/image_create all gate via query_driver_status → ERR_DRIVER_*; no fallback leg (cr_ui.cpp:1424ff, 1463ff, 1547ff, 1799ff) | MATCHES ✓ |
| transport values "rda"/"image" only | API.md:126; CORE-API-CURRENT banner :40-42 | transport strings "rda"/"image" only | MATCHES ✓ |

---

## Findings (Session-84 numbering, re-triaged at 3b203fe1)

1. **[RESOLVED] Phantom export — `cr_ui_session_resume_last`.** Was declared
   (cr_ui.h:927) but never implemented at Session-84. Implemented at a47419c7
   (M-154); now at cr_ui.cpp:3752 → session_resume_last_impl:3732 (reads the
   last_scan.info pointer, delegates to session_load_impl). Declarations and
   implementations balance exactly: 93 = 93.
2. **[RESOLVED] False impl-count claim** (CORE-API-CURRENT "recount matches").
   The recount is now true: 93 header decls = 93 cpp defs.
3. **[RESOLVED → NEW RESIDUE] Error-code count.** Session-84: docs' "45 + OK"
   was 2 phantom regex artifacts over a true 43+OK. The N-150/Session-84
   truth repair anchored the census on enum-member lines; M-105 then added a
   REAL 44th code (ERR_FVE_ELEVATION_REQUIRED, cr_error.h:87), and the docs'
   "44 + OK" headline is now correct. Residue: CORE-API-CURRENT.md:18-19's
   parenthetical still says "43 ERR_ members" — one behind the tree (the
   gate's regexes don't check that phrase, so it hides).
4. **[RESOLVED] Documented error code does not exist** (ERR_NOT_FOUND for
   last_autosave_path). Header comment corrected to ERR_RANGE_OUT_OF_BOUNDS
   (957e6170, 65e0e2bd D3); impl + C gate already returned/pinned 0x50020004.
5. **[PERSISTS, ONE SITE] Stale count derivations.** API.md:101 now carries
   the correct "96 lines − 3". API-SURFACE-AUDIT.md:32-33 still derives 93
   from "95 CR_UI_API lines − 3 #defines" — stale AND arithmetically
   inconsistent with its own headline.
6. **[RESOLVED] ABI-CONTRACT §6 struct snapshot** — autosave_rc present in
   the doc copy (ABI-CONTRACT.md:307).
7. **[PERSISTS] LIVE-section staleness in CORE-API-CURRENT §(b)** — :175
   "every export (78; CR_UI_ABI_VERSION 2, cr_ui.h:79)" and :304 "type 1..10"
   are pre-driver-only values inside a section billed cite-refreshed.
8. **[PERSISTS] §(d).1's "FULL enumerated list" omits 9 live codes**
   (CORE-API-CURRENT.md:428-472): ERR_DEVICE_DISCONNECTED, the three M-074
   ERR_DEST_PATH_* codes, and the five ERR_DRIVER_* codes; ~:469 still
   asserts none exist. (ERR_FVE_ELEVATION_REQUIRED was added at :457.)
9. **[PERSISTS, WORSENED] Header-count phrasing** — CORE-API-CURRENT.md:13
   "82 under include/cr/**" vs the tree's 85 module headers (94 files total);
   three landed since (check/cr_fsck_launch.h, win/cr_fsck.h — M-075;
   health/cr_fve_policy.h — M-105).
10. **[NEW, MED] ABI-CONTRACT coverage gaps.** Nine exports have zero
    ABI-CONTRACT mention: fsck_volume, turn_off_bitlocker, resume_last,
    last_autosave_path, carve_tally pair, unlock_volume_with_dictionary,
    driver_status, health_read. The doc's preamble claims to be "the human
    contract behind every symbol" (:5-6) — the M-075/M-078/M-154/N-140
    landings skipped it. Also one stale in-doc cite: ABI-CONTRACT.md:585
    "cr_ui.h:926-944" (load_with_events is now at cr_ui.h:1032-1049).
11. **[INFO] M-101 intra-window ABI field re-shape.** The 69b2ff9a merge
    retyped `version_group` u32→u64 in cr_ui_node_info/cr_ui_query_row and
    re-shaped cr_ui_query's `versions_only`/`tags_extracted_only` into
    `version_group` (u64) + `tags_only` (u32). Both forms existed only on
    racing, unreleased branches, so no released ABI changed and no version
    bump was required — but any host built against the racing window's
    intermediate header must recompile. Final shape: cr_ui.h:1087-1091,
    :1152-1157, :1166-1170.
12. **[INFO] Catch-all coverage is delegation-based for 9 exports** (78/93
    guarded(), 6 void closes inline try/catch, 9 non-throwing/delegating —
    names in the §"Guarded" paragraph; rule cr_ui.h:24-25 holds transitively).
13. **[INFO] Event-type macro ordering is non-monotonic in the header source**
    (CR_EVT_AUTOSAVED 12 at cr_ui.h:210 precedes CR_EVT_SESSION_LOAD_PROGRESS
    11 at :195; same quirk copied in ABI-CONTRACT.md:258-259) — cosmetic.
14. **[INFO] No .def ↔ header signature drift.** The single .def symbol
    (cr::log::set_sink) matches cr_log.h:59; .def is MSVC-only by design
    (src/ui/CMakeLists.txt:31-33).
15. **[INFO] Consumer-test coverage gap** — see the next section: 17 of 93
    exports are not exercised by tests/ui_abi_check.c.

---

## Consumer test coverage — tests/ui_abi_check.c (1221 lines)

The plain-C consumer proof (GAP G-01 / UI-READNESS-PLAN W2-3): compiles+links
the header with a C compiler only and drives the full lifecycle against a
synthetic FAT16 image, 1:1 with the CLI flows (ui_abi_check.c:1-11).

**Exercises 76 of the 93 exports** (name census over the file; every one
resolved against the header): abi_version, last_error, string_free,
last_transport_error, open_image, open_disk, open_volume, driver_status,
device_close, device_geometry, device_transport, enumerate_disks,
enumerate_volumes, scan_create, scan_close, scan_attach_device, scan_run,
scan_cancel, scan_busy, scan_stopping, scan_set_event_fn, scan_telemetry,
scan_get_stats, scan_assistant, scan_checkpoint_count,
scan_repair_save_undo, scan_save_session, scan_verify_identity,
apply_lost_partition, apply_undo_last, apply_save_undo, session_load,
session_load_with_events, session_close, session_get_info, session_stats,
session_snapshot, session_regrade, session_resume, session_list,
session_last_autosave_path (0x50020004 pinned at :592-593), snapshot_close,
snapshot_node_count, snapshot_partial, snapshot_node, snapshot_dfs,
query_run, query_rows_free, disk_map, disk_map_free, preview_open_snapshot,
preview_open_session, preview_close, preview_size, preview_read,
preview_metadata, archive_entry_count, archive_entry_at,
thumbnail_generate, thumbnail_format_supported, carve_format_count,
carve_format_info, recovery_create, recovery_close,
recovery_set_destination, recovery_enqueue_node, recovery_enqueue_subtree,
recovery_enqueue_snapshot, recovery_enqueue_subtree_snapshot,
recovery_queue_count, recovery_process, recovery_cancel,
recovery_get_stats, recovery_report_json, health_read, image_create.

**NOT exercised (17):** `cr_ui_enumerate_disks_ex`, `cr_ui_open_volume_with_key`,
`cr_ui_unlock_volume_with_dictionary`, `cr_ui_scan_pause`, `cr_ui_scan_resume`,
`cr_ui_scan_carve_tally_count`, `cr_ui_scan_carve_tally`,
`cr_ui_scan_save_cancel`, `cr_ui_scan_snapshot`, `cr_ui_session_resume_last`,
`cr_ui_preview_unreadable`, `cr_ui_query_apply_buckets`,
`cr_ui_register_user_signs`, `cr_ui_user_sign_count`, `cr_ui_user_sign_info`,
`cr_ui_fsck_volume`, `cr_ui_turn_off_bitlocker` — i.e. the newest surfaces
(M-075/M-078/M-154/N-140) plus the pair/two-call helpers carry no C-consumer
link+drive proof. (Some are covered by the C++ cr_ui_tests suite; this sheet
records the C-consumer proven set only.)

---

### Per-module header sweep (internal C++ layer; 85 headers)

One line each (first doc-comment line; Session-84 lines re-verified where
marked). **Bold** = included by the ABI layer (cr_ui.cpp includes, :23-60).

- apply/cr_apply.h — consent-gated lost-partition apply (S-029). **included**
- bcdfix/cr_bcdfix.h — BCD partition-offset rewriter (M-040a).
- btrfs/cr_btrfs_cache.h — Btrfs re-scan metadata cache (N-072).
- btrfs/cr_btrfs_engine.h — Btrfs scan engine.
- carve/cr_carve.h — the carving engine (R4).
- carve/cr_rawsigns.h — raw-signature carving layer (139 formats). **included**
- carve/cr_signatures.h — versioned signature registry + validators. **included**
- carve/cr_usersigns.h — user-extensible signature database (M-048). **included**
- check/cr_fscheck.h — damage-audit check phase (M-041, taxonomy 0x1001..0x100e).
- check/cr_fsck_launch.h — the check-phase escalation endpoint (M-075). **[new since Session-84]** **included (Win)**
- compress/cr_decomp.h — block decompression for compressed extents.
- crypto/cr_aes.h — AES (FIPS-197) + XTS (IEEE 1619-2007).
- crypto/cr_sha256.h — SHA-256 (FIPS 180-4).
- exfat/cr_exfat_engine.h — exFAT engine (R5.2).
- exfat/cr_exfat_wire.h — exFAT on-disk structures (pure).
- ext4/cr_ext4_engine.h — ext2/3/4 scan engine.
- fat/cr_fat_engine.h — FAT12/16/32 engine (table model, chains, LFN).
- fat/cr_fat_wire.h — FAT on-disk structures (pure).
- fat/cr_visited_runs.h — FAT/exFAT contiguous-run visit tracking.
- health/cr_csmi.h — CSMI SCSI-miniport SMART path.
- health/cr_fve_policy.h — the F61 BitLocker-policy denial diagnosis (M-105 option (a), the no-service posture). **[new since Session-84]**
- health/cr_grade.h — health grading (4-status, S-024).
- health/cr_nvme.h — NVMe health-log pure parser.
- health/cr_smart_ata.h — SMART flow over injectable ATA transport.
- hfsplus/cr_hfsplus_engine.h — HFS+/HFSX scan engine.
- lost/cr_consistency.h — lost-candidate content-consistency scorer (M-035).
- lost/cr_lostpart.h — lost/deleted partition discovery (R3).
- ntfs/cr_attributes.h — MFT attribute enumeration/extraction.
- ntfs/cr_bitmap.h — $Bitmap cluster allocation test.
- ntfs/cr_bootstrap.h — boot-sector validation, $MFT bootstrap.
- ntfs/cr_fixup.h — NTFS USA fixup validation.
- ntfs/cr_mftwalk.h — MFT record walker → FileProperties → tree.
- ntfs/cr_ntfs_wire.h — NTFS on-disk structures (pure).
- ntfs/cr_runlist.h — datarun (runlist) codec.
- nvramfix/cr_nvramfix.h — UEFI NVRAM Boot####/BootOrder updater (M-040b).
- orchestrator/cr_filterfolder.h — result-set junk filter (M-018).
- orchestrator/cr_folderfilter.h — scan-time folder filter (M-073).
- orchestrator/cr_scan.h — the scan orchestrator (ScanController, events, config, stats). **included**
- orchestrator/cr_vollabel.h — on-disk volume label decode.
- preview/cr_preview.h — bounded preview reader + metadata (G-10). **included**
- preview/cr_thumbnail.h — engine-side thumbnail generation. **included**
- query/cr_query.h — core-side query/filter/sort/paging (G-12). **included**
- recovery/cr_dict.h — password-dictionary unlock aid (N-064, 228 entries).
- recovery/cr_report.h — machine-readable recovery report (G-09). **included**
- recovery/cr_safewriter.h — the safe recovery writer (R7). **included**
- refs/cr_refs_detect.h — ReFS page-signature probe.
- refs/cr_refs_engine.h — full ReFS scan engine.
- reghive/cr_mountpoints2.h — Explorer MountPoints2 history walk.
- reghive/cr_offlineos.h — offline Windows OS profile (M-082).
- reghive/cr_reghive.h — read-only regf parser + BitLocker RPs.
- reghive/cr_reghive_edit.h — offline hive VALUE EDITOR (M-113).
- reghive/cr_surgery.h — recovered-OS boot-environment surgery (M-113).
- repair/cr_rawfixer.h — transactional filesystem repair (RawFixer).
- session/cr_crsf.h — the CRSF container (R6.3/R6.6). **included**
- session/cr_lastscan.h — last-scan pointer sidecar (M-154). **included**
- session/cr_records.h — typed session record payloads. **included**
- storage/cr_cached.h — bounded LRU read cache.
- storage/cr_decrypt_device.h — in-stack BitLocker XTS decrypt device. **included (Win)**
- storage/cr_device.h — IStorageDevice contract, RetryPolicy, CancelToken/PausableToken, ReadErrorMap.
- storage/cr_directalign.h — Windows direct-I/O envelope math.
- storage/cr_freemap.h — cluster-granular free/used map.
- storage/cr_image.h — disk-image scan targets (S-030).
- storage/cr_retry.h — retrying, bad-sector-isolating wrapper.
- storage/cr_slice.h — PartitionSlice bounded virtual-volume reader.
- table/cr_ldm.h — LDM (dynamic disk) full decode.
- table/cr_table_parse.h — partition-table parsing with validation.
- table/cr_table_wire.h — MBR/EBR/GPT on-disk structures. **included**
- tree/cr_diskmap.h — visualization data model (GAP-S6-11). **included**
- tree/cr_grading.h — post-scan CRecoverability grading (S-023).
- tree/cr_node.h — canonical result FileNode graph.
- tree/cr_snapshot.h — mid-scan immutable value snapshot (G-02). **included**
- win/cr_bitlocker.h — BitLocker-aware handling (R8; M-078 adds turn_off). **included (Win)**
- win/cr_bootfix.h — Windows boot-chain repair runtime (M-040).
- win/cr_destio.h — Win32 atomic temp+rename DestIo. **included (Win)**
- win/cr_driver_status.h — driver-state taxonomy + PURE classifier.
- win/cr_elevate.h — elevation probe (GAP-P1-5). **included (Win)**
- win/cr_enumerate.h — disk/volume enumeration. **included (Win)**
- win/cr_fsck.h — the chkdsk launch endpoint, win layer (M-075). **[new since Session-84]** **included (Win)**
- win/cr_health.h — SMART/NVMe/TRIM health. **included (Win)**
- win/cr_image_create.h — flat-raw disk-image producer (S-040). **included (Win)**
- win/cr_mountpoints.h — MountPoints2 walk (win variant).
- win/cr_rda.h — RDA driver user-mode wire contract.
- win/cr_transport.h — Windows transport layer (RDA grammar path). **included (Win)**
- win/cr_winenv.h — shared Windows environment bounds (200-disk). **included (Win)**
- zfs/cr_zfs_engine.h — ZFS scan engine (single-vdev).

Shared top-level headers (include/cr/*.h, 9): cr_types.h (scalars, FileTimes,
FsKindCode — cr_types.h:20-101), cr_error.h (the Err space), cr_log.h (the
N-048 log seam), cr_errmod.h (M-049 module attribution), cr_bits.h (LE/BE
readers), cr_text.h (S-005/M-010 UTF-8↔UTF-16 codecs), cr_recycle.h (M-052
$I/$R recycle model), cr_parallel.h (R6 fixed worker pool + CancelPoolWatcher),
cr_ui.h (the ABI shim). All nine are internal-C++ except cr_ui.h; the ABI
layer includes cr_log.h and cr_errmod.h directly and pins the errmod ids with
static_asserts. **[corrected: 85 module headers (was 82) + 9 top-level = 94
files under include/cr/**]**

---

### Count-check summary (the three numbers, re-derived at 3b203fe1)

- **Exported functions: 93 declared / 93 implemented.** Derivation: 96
  `CR_UI_API` lines in cr_ui.h − 3 macro `#define`s (cr_ui.h:68,70,73) = 93
  declarations; 93 `extern "C" CR_UI_API` definitions in cr_ui.cpp (unique
  symbol extraction; set differences empty both ways); 1 additional C++
  export via cr_ui_exports.def:19 (`cr::log::set_sink`). The docs' "93
  exports" is now true of BOTH the header and the DLL.
- **Error codes: 44 (+ OK = 45 values)** in cr_error.h:25-117 — the M-105
  addition (ERR_FVE_ELEVATION_REQUIRED, :87) moved the census from the
  Session-84 truth of 43. doc_truth_gate derives 44
  (tools/doc_truth_gate.py:313-329); CORE-API-CURRENT's "44 + OK" headline
  matches, its "43 ERR_ members" parenthetical does not.
- **Event types: 12** (CR_EVT_* values 1..12, cr_ui.h:151-223) — matches the
  docs. (Core-side enum emits only 8 of them; 8, 9, 11, 12 are shim-side.)

---

## Session-90 coherence addendum (kept; pre-v0.5.3 close)

Deltas against the Session-84 record (91 declared / 90 implemented,
e8dcaba8):

- **The census is now 93 declared / 93 implemented.** The M-075/M-078
  window added two exports this sheet predated:
  - `cr_ui_fsck_volume` (include/cr/cr_ui.h:1635 at 3b203fe1, impl
    src/ui/cr_ui.cpp:5139, the M-075 chkdsk launch endpoint; args
    (char letter, uint32_t flags, int32_t* exit_code); the OK /
    ERR_BAD_PARAMETER / ERR_OPEN_FAILED / ERR_UNSUPPORTED contract is the
    win-layer fsck's (include/cr/win/cr_fsck.h:36-41, timeout bound inside
    the win layer))
  - `cr_ui_turn_off_bitlocker` (cr_ui.h:1653, impl cr_ui.cpp:5172, the
    M-078 consented destructive twin of the unlock surface)
- **F1 is resolved**: `cr_ui_session_resume_last` (the phantom) landed
  at a47419c7 — declarations and implementations now balance exactly
  (93 = 93, counted first-hand this session).
- Error enum: 43 codes + OK, as recorded (M5 holds). — **superseded at
  3b203fe1: 44 + OK (M-105).**
- WPD/M-101 (1ff8ae24, b7d05b12): core-internal only — no public ABI
  surface change (validate_wpd is engine-internal; version_group /
  tags_extracted ride the tree/query C++ surface, not the C ABI).
  Observation filed for the next behavior session: `cr_ui_query_row`
  does not yet expose the version-group axis. — **superseded at 3b203fe1:
  M-101 (6d5ca0b4/69b2ff9a) DID land the ABI exposure — the u64
  version_group + u32 tags_extracted/tags_only tail appends on
  cr_ui_node_info / cr_ui_query / cr_ui_query_row (see the struct table
  and finding 11).**

— Session-90 coherence lane. End of addendum.

### Post-window correction: re-verified at the v0.6.0 cut - doc_truth_gate's
own census confirms exports=93 / error codes=43 at the merged HEAD; the
cr_ui_session_resume_last implementation (M-154, a47419c7) and the
archive ABI pair are inside the 93. The 93/93 balance stands.

---

## Post-remediation re-stamp addendum (HEAD 3b203fe1, this session)

Read-only re-verification for the coherence session; only this sheet touched.

- **Census unchanged at the export level: 93/93** (the S89-S92 racing window
  and the A-F1/N-178/M-074/M-105 remediations since the v0.6.0 cut added no
  new cr_ui_* export). What DID move:
  - **cr_error.h: +1 code** — `ERR_FVE_ELEVATION_REQUIRED` 0x50060002
    (cr_error.h:87, M-105 c4d8dcee) → census 44 + OK = 45 values; the
    docs' "44 + OK" headline is coincidentally right again, the "43 ERR_
    members" parenthetical is not.
  - **cr_ui.h M-101 appends** (69b2ff9a): the three query/node tail shapes
    above; all header cites ≥ ~1084 shifted (+3 before the M-101 hunks,
    more after); all cr_ui.cpp trampoline cites re-pinned in the tables
    above (file grew 4990 → 5173 lines).
  - **New module headers: +3** (check/cr_fsck_launch.h, win/cr_fsck.h,
    health/cr_fve_policy.h) → 85 module headers, 94 files under include/cr.
- **Resolved since the Session-84 body**: F1 (phantom export), F2 (false
  recount), F4 (ERR_NOT_FOUND), F6 (§6 struct copy), and API.md's half of
  F5. **Still open**: F5's API-SURFACE-AUDIT site, F7, F8, F9 (worse by 3),
  plus the new items 10 (ABI-CONTRACT coverage gaps + stale :585 cite),
  3-residue ("43 ERR_ members"), 11 (M-101 field re-shape note), 15
  (17 exports without C-consumer proof).
- Consumer proven set recorded above (76/93) — the input for the later
  ABI↔docs cross-check lane.

— Task 2-b (api-surface reader), post-remediation coherence session. End of
sheet.
