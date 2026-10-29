# API surface — Session-84 Phase 0 (baseline e8dcaba8)

Task T2-B (Agent B): the complete public API surface audit (C ABI + C++), read
first-hand at HEAD `e8dcaba8` (main). Every entry carries `file:line`. Method:
full read of `include/cr/cr_ui.h` (1600 lines), the 8 shared headers,
`src/ui/cr_ui_exports.def`, `src/ui/cr_ui.cpp` (4990 lines, impl-level reads of
every exported entry + the error-path extraction), the 4 contract docs, and the
per-module header sweep. Count derivation reproduced independently (python,
same regexes as `tools/doc_truth_gate.py:313-320`).

---

## Exported functions

Calling convention: every function is `extern "C"` and uses the default x64
convention (no `__stdcall` on x64; the ABI is C11 — cr_ui.h:24). Export macro:
`CR_UI_API` = `__declspec(dllexport)` under `CR_UI_BUILD` / `dllimport`
otherwise on Win32, `__attribute__((visibility("default")))` elsewhere
(cr_ui.h:63-73). Impl = `src/ui/cr_ui.cpp` unless noted. "Threading"/"ownership"
columns quote the header/ABI-CONTRACT contract; "errors (impl)" lists codes on
the actual return paths (incl. pass-through engine codes).

**Derived totals: 91 header declarations (94 `CR_UI_API` lines − 3 `#define`s,
cr_ui.h) vs 90 implementations in cr_ui.cpp — `cr_ui_session_resume_last`
(cr_ui.h:927) is declared but NEVER implemented (F1).** The DLL therefore
exports 90 `cr_ui_*` C symbols (+1 C++ symbol via the .def).

### 1. Version & diagnostics (cr_ui.h:101-113)

| symbol | signature | threading (doc) | ownership (doc) | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_abi_version` | `uint32_t (void)` | any time, stateless | none | none — returns `CR_UI_ABI_VERSION` (3) | cr_ui.cpp:1336 |
| `cr_ui_last_error` | `const char* (void)` | thread-local; stable until next cr_ui_* call on same thread; never NULL (cr_ui.h:107-109) | library-owned buffer | none (returns TLS string, cr_ui.cpp:115,1340) | cr_ui.cpp:1340 |
| `cr_ui_string_free` | `void (char* s)` | any time | frees heap strings returned by the API; `free()` also valid (cr_ui.h:112) | none | cr_ui.cpp:1344 |

### 2. Devices & transport (cr_ui.h:305-445)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_open_image` | `uint32_t (const char* utf8_path, uint64_t offset, uint64_t bytes, cr_ui_device** out)` | synchronous; portable (all lanes) | caller closes via `cr_ui_device_close`; device must outlive scans/previews/recoveries (cr_ui.h:310-311) | `ERR_BAD_PARAMETER` (null args / window past EOF / sub-sector tail, cr_ui.cpp:1355-1356, ImageFileDevice::open :546-568), `ERR_OPEN_FAILED` (:546,:552) | cr_ui.cpp:1812 → open_image_impl:1354 |
| `cr_ui_open_disk` | `uint32_t (uint32_t disk, cr_ui_device** out)` | synchronous; Windows only | caller closes | Win: `ERR_BAD_PARAMETER` (:1385), classified `ERR_DRIVER_*` 0x500B0001..05 (:1395-1400), `ERR_OPEN_FAILED`/open rc (target error, :1418); non-Win: `ERR_UNSUPPORTED` (:1682) | cr_ui.cpp:1816 → 1383 / stub 1675 |
| `cr_ui_open_volume` | `uint32_t (char letter, cr_ui_device** out)` | synchronous; Windows only | caller closes; returned device is the volume slice on its parent disk, kept alive internally (cr_ui.h:327-330) | `ERR_BAD_PARAMETER` (:1424), `ERR_OPEN_FAILED` (unknown BL state :1427, extent map :1441, slice :1476, rda open :1461), `ERR_FVE_LOCKED` (:1433), `ERR_DRIVER_*` (:1448-1453); non-Win `ERR_UNSUPPORTED` (:1688) | cr_ui.cpp:1827 → 1422 / stub 1685 |
| `cr_ui_open_volume_with_key` | `uint32_t (char letter, const char* utf8_hex_fvek, cr_ui_device** out)` | synchronous; Windows only (M-033) | caller closes | `ERR_BAD_PARAMETER` (args :1497, hex len :1501, non-hex :1517, non-FVE volume :1524, weak FVEK :1574), `ERR_OPEN_FAILED` (:1535,:1555,:1570), `ERR_DRIVER_*` (:1542-1547); non-Win `ERR_UNSUPPORTED` (:1695) | cr_ui.cpp:1830 → 1496 / stub 1691 |
| `cr_ui_last_transport_error` | `uint32_t (void)` | advisory, process-global, never clears (cr_ui.h:334-340) | none | returns the atomic `g_last_transport_error` (0 / `ERR_DRIVER_*` / target err) | cr_ui.cpp:1819 (state :1376) |
| `cr_ui_driver_status` | `uint32_t (cr_ui_driver_status_t* out)` | synchronous, thread-safe, stateless (cr_ui.h:365-371) | standalone query, no lifecycle | `ERR_BAD_PARAMETER` (:1630), query rc pass-through (:1635); non-Win `ERR_UNSUPPORTED` (:1714) | cr_ui.cpp:1834 → 1628 / stub 1707 |
| `cr_ui_unlock_volume_with_dictionary` | `uint32_t (char letter, const char* const* utf8_user_passwords, uint32_t count)` | explicit-consent action; Windows only | none | `ERR_BAD_PARAMETER` (non-ASCII candidate, :1608), `win::bitlocker::unlock_with_dictionary` rc (`OK`/`ERR_FVE_LOCKED`, :1615-1620); non-Win `ERR_UNSUPPORTED` (:1703) | cr_ui.cpp:1837 → 1588 / stub 1698 |
| `cr_ui_device_close` | `void (cr_ui_device* dev)` | caller's thread; close order caller's responsibility (cr_ui.h:310-311) | releases the handle (delete; try/catch) | none | cr_ui.cpp:1841 |
| `cr_ui_device_geometry` | `uint32_t (cr_ui_device* dev, cr_ui_geometry* out)` | synchronous | fills caller struct | `ERR_BAD_PARAMETER` (:1721) | cr_ui.cpp:1849 → 1719 |
| `cr_ui_device_transport` | `uint32_t (cr_ui_device* dev, char* buf, uint32_t cap, uint32_t* needed)` | synchronous; two-call pattern (cr_ui.h:439-443) | caller buffer | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` (cap < needed, :1808) | cr_ui.cpp:1852 → 1795 |

### 3. Enumeration (cr_ui.h:447-517; Windows platform layer)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_enumerate_disks` | `uint32_t (cr_ui_disk_info* out, uint32_t* count)` | no shared state; each call RE-QUERIES the OS (N-098, cr_ui.h:495-503) | caller rows (POD) | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED`, `ERR_UNSUPPORTED` (stub :1954) | cr_ui.cpp:1970 → 1865 (200-disk bound :1869) |
| `cr_ui_enumerate_disks_ex` | `uint32_t (cr_ui_disk_info* out, uint32_t* count, uint32_t flags)` | same; unknown flag bits refused pre-platform (M-076) | caller rows | `ERR_BAD_PARAMETER` (unknown bits), `ERR_TRUNCATED`, `ERR_UNSUPPORTED` | cr_ui.cpp:1973 → 1865 |
| `cr_ui_enumerate_volumes` | `uint32_t (cr_ui_volume_info* out, uint32_t* count)` | same | caller rows | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED`, `ERR_UNSUPPORTED` (stub :1960) | cr_ui.cpp:1986 → 1922 |

### 4. Scan lifecycle (cr_ui.h:519-804)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_scan_create` | `uint32_t (const cr_ui_scan_config* cfg, cr_ui_scan** out)` | one handle, one controlling thread (ABI-CONTRACT §3) | caller closes via `cr_ui_scan_close`; device attached later is BORROWED | `ERR_BAD_PARAMETER` (stage_mask unknown bits cr_ui.cpp:1257, invalid fs_hint :1292) | cr_ui.cpp:3025 → 2066 |
| `cr_ui_scan_close` | `void (cr_ui_scan* s)` | not while run() executes (impl joins heartbeat defensively, :3049) | releases handle | none | cr_ui.cpp:3028 |
| `cr_ui_scan_attach_device` | `uint32_t (cr_ui_scan* s, cr_ui_device* dev)` | before run(); identity card auto-fill on Windows (cr_ui.h:669-672) | device borrowed, must outlive the scan | `ERR_BAD_PARAMETER` (:2264ff) | cr_ui.cpp:3059 → 2264 |
| `cr_ui_scan_run` | `uint32_t (cr_ui_scan* s)` | SYNCHRONOUS on caller's thread (cr_ui.h:521-527); one-shot (double-run refused) | tree stays browsable on error/cancel | `ERR_BAD_PARAMETER` (null :2359, already ran :2362, no device :2364, running :2366), `ERR_SESSION_IO` (checkpoint file :2377), engine rc pass-through incl. `ERR_READ_FAILED` (total-blindness refusal), `ERR_CANCELLED` (:2439-2454, autosaves per S-057) | cr_ui.cpp:3062 → 2357 |
| `cr_ui_scan_pause` | `uint32_t (cr_ui_scan* s)` | ANY thread while run() executes (cr_ui.h:679-682) | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3065 → 2476 |
| `cr_ui_scan_resume` | `uint32_t (cr_ui_scan* s)` | any thread | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3068 → 2482 |
| `cr_ui_scan_cancel` | `uint32_t (cr_ui_scan* s)` | any thread; run() → ERR_CANCELLED | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:3075 → 2488 |
| `cr_ui_scan_busy` | `uint32_t (cr_ui_scan* s, uint32_t* out)` | any thread, any time (N-075, cr_ui.h:684-695) | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3078 → 2504 |
| `cr_ui_scan_stopping` | `uint32_t (cr_ui_scan* s, uint32_t* out)` | any thread, any time | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3081 → 2511 |
| `cr_ui_scan_set_event_fn` | `uint32_t (cr_ui_scan* s, cr_ui_event_fn fn, void* user)` | subscribe BEFORE run() (ERR_BAD_PARAMETER mid-run, cr_ui.h:300-303); one sink per handle | sink + user borrowed | `ERR_BAD_PARAMETER` | cr_ui.cpp:3071 → 2518 |
| `cr_ui_scan_telemetry` | `uint32_t (cr_ui_scan* s, cr_ui_telemetry* out)` | any thread, any time (cr_ui.h:717) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3084 → 2552 (locks ctrl_mu) |
| `cr_ui_scan_get_stats` | `uint32_t (cr_ui_scan* s, cr_ui_scan_stats* out)` | post-run counters (any thread per ABI-CONTRACT §5) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3087 → 2578 |
| `cr_ui_scan_carve_tally_count` | `uint32_t (cr_ui_scan* s)` | count-as-return (N-140); null scan reads 0 | value return | none (value IS the return, :3092-3099) | cr_ui.cpp:3113 → 3092 |
| `cr_ui_scan_carve_tally` | `uint32_t (cr_ui_scan* s, uint32_t index, uint64_t* out)` | synchronous | fills caller u64 | `ERR_BAD_PARAMETER` (:3102, out-of-range :3106) | cr_ui.cpp:3116 → 3100 |
| `cr_ui_scan_assistant` | `uint32_t (cr_ui_scan* s, cr_ui_assistant_stats* out)` | valid after run() OK (cr_ui.h:784-787) | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3120 → 2613 |
| `cr_ui_scan_checkpoint_count` | `uint32_t (cr_ui_scan* s, uint64_t* out)` | any thread | fills caller u64 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3123 → 2847 |
| `cr_ui_scan_repair_save_undo` | `uint32_t (cr_ui_scan* s, const char* utf8_path)` | after run() when repair executed (cr_ui.h:797-804) | writes file | `ERR_BAD_PARAMETER` (not run / nothing applied), `ERR_OPEN_FAILED`, `ERR_SESSION_IO` (:2637ff) | cr_ui.cpp:3126 → 2637 |
| `cr_ui_scan_save_session` | `uint32_t (cr_ui_scan* s, const char* utf8_path)` | valid after run() incl. cancelled (cr_ui.h:879-882); fires CR_EVT_SAVE_PROGRESS from the calling thread | atomic tmp+rename; suppresses later auto-save (S-044) | `ERR_BAD_PARAMETER` (:2934, not run :2938), `ERR_SESSION_IO` + container rc + `ERR_CANCELLED` via write_session_atomic (:2933-2952) | cr_ui.cpp:3143 → 2933 |
| `cr_ui_scan_save_cancel` | `uint32_t (cr_ui_scan* s)` | ANY thread (N-060, cr_ui.h:884-892); save-scoped | sets atomic save_cancel | `ERR_BAD_PARAMETER`; no-op if no save in flight | cr_ui.cpp:3146 → 2955 |
| `cr_ui_scan_snapshot` | `uint32_t (cr_ui_scan* s, cr_ui_snapshot** out)` | ANY thread while run() executes (partial=1) or after (cr_ui.h:894-898) | caller closes via `cr_ui_snapshot_close`; value copy | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` (:2969) | cr_ui.cpp:3149 → 2962 |
| `cr_ui_scan_verify_identity` | `uint32_t (cr_ui_scan* s)` | after resume+attach, before run() (cr_ui.h:900-906) | none | `ERR_BAD_PARAMETER` (not resumed :3008, no device :3012), `ERR_BAD_CHECKSUM` (mismatch), OK on unverifiable v1 session (:3009-3010) | cr_ui.cpp:3152 → 3003 |

### 5. Lost-partition apply (cr_ui.h:806-877; ABI-since 1.3)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_apply_lost_partition` | `uint32_t (cr_ui_device* dev, const cr_ui_apply_candidate* cand, uint32_t rebuild_table, cr_ui_apply_consent_fn consent, void* consent_user, cr_ui_apply_result* out)` | synchronous; consent fires exactly once before any write; default-deny | undo log retained on the device handle | `ERR_BAD_PARAMETER`, `ERR_APPLY_CONSENT_DENIED`, `ERR_APPLY_STALE`, `ERR_APPLY_REFUSED`, `ERR_APPLY_REFRESH_FAILED`, `ERR_VOLUME_LOCK_FAILED` (S-010), engine rc pass-through (:2721ff) | cr_ui.cpp:3130 → 2721 |
| `cr_ui_apply_undo_last` | `uint32_t (cr_ui_device* dev, cr_ui_apply_result* out)` | synchronous; idempotent on bytes | uses retained undo log | `ERR_BAD_PARAMETER` (no undo log), `ERR_APPLY_REFUSED`, `ERR_APPLY_REFRESH_FAILED` (:2801ff) | cr_ui.cpp:3136 → 2801 |
| `cr_ui_apply_save_undo` | `uint32_t (cr_ui_device* dev, const char* utf8_path)` | synchronous | writes CRUNDO01 container | `ERR_BAD_PARAMETER` (no undo log), `ERR_OPEN_FAILED`, `ERR_SESSION_IO` (:2829ff) | cr_ui.cpp:3140 → 2829 |

### 6. Sessions, list, pointed resume (cr_ui.h:908-1048)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_session_load` | `uint32_t (const char* utf8_path, cr_ui_session** out)` | no shared state; any thread | caller closes via `cr_ui_session_close` | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED` (:3166), container rc pass-through (SESSION_FORMAT/IO/VERSION, :3174-3176) | cr_ui.cpp:3584 → 3162 |
| `cr_ui_session_load_with_events` | `uint32_t (const char* utf8_path, cr_ui_event_fn fn, void* user, volatile uint32_t* cancel_flag, cr_ui_session** out)` | events from the CALLING thread; cancel_flag settable from ANY thread (W1b/R-54, cr_ui.h:1029-1048) | same; cancel flag CALLER-OWNED | same as load + `ERR_CANCELLED` (torn load discards tree, :3222-3245) | cr_ui.cpp:3587 → 3222 |
| `cr_ui_session_close` | `void (cr_ui_session* sess)` | after last borrower released | releases handle | none | cr_ui.cpp:3607 |
| `cr_ui_session_resume_last` | `uint32_t (const char* utf8_dir, cr_ui_session** out)` | doc: no shared state; any thread (cr_ui.h:917-927) | **NO IMPLEMENTATION — phantom export (F1)** | doc'd: `CR_UI_ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED` (dead-pointer class) — unreachable | **declared cr_ui.h:927 only; zero defs in src/** |
| `cr_ui_session_get_info` | `uint32_t (cr_ui_session* sess, cr_ui_session_info* out)` | stable tree; any thread per handle discipline | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3615 → 3320 |
| `cr_ui_session_stats` | `uint32_t (cr_ui_session* sess, cr_ui_scan_stats* out)` | same | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:3622 → 3343 |
| `cr_ui_session_snapshot` | `uint32_t (cr_ui_session* sess, cr_ui_snapshot** out)` | same | caller closes snapshot | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:3625 → 3410 |
| `cr_ui_session_regrade` | `uint32_t (cr_ui_session* sess, const uint64_t* node_ids, uint32_t count, uint32_t* regraded)` | grade-1-only re-grade (cr_ui.h:789-794) | fills caller u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3618 → 3367 |
| `cr_ui_session_resume` | `uint32_t (cr_ui_session* sess, const cr_ui_scan_config* original_cfg, cr_ui_scan** out)` | mask-validated resume (cr_ui.h:949-955) | returns a FRESH scan handle | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED`, load rc pass-through (:3541ff) | cr_ui.cpp:3628 → 3541 |
| `cr_ui_session_list` | `uint32_t (const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count)` | no shared state; any thread; two-call (W1a/R-56, cr_ui.h:957-1012) | caller rows (POD, no free export) | `ERR_BAD_PARAMETER`, `ERR_OPEN_FAILED` (dir unreadable :3253), `ERR_TRUNCATED` (capacity < needed) | cr_ui.cpp:3633 → 3248 |
| `cr_ui_session_last_autosave_path` | `uint32_t (const char* utf8_dir, char* out_path, uint32_t cap)` | no shared state (M-154, cr_ui.h:1014-1027) | copies into caller buffer | `ERR_BAD_PARAMETER` (:3644), **`ERR_RANGE_OUT_OF_BOUNDS`** for the dead-pointer case (:3647) — header documents `ERR_NOT_FOUND`, which does not exist (F4); `ERR_TRUNCATED` (:3649) | cr_ui.cpp:3655 → 3642 |

### 7. Snapshots (cr_ui.h:1050-1098)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_snapshot_close` | `void (cr_ui_snapshot* snap)` | after last row read | releases value copy | none | cr_ui.cpp:3748 |
| `cr_ui_snapshot_node_count` | `uint32_t (cr_ui_snapshot* snap, uint32_t* out)` | immutable value; any thread | fills caller u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:3756 → 3666 |
| `cr_ui_snapshot_partial` | `uint32_t (cr_ui_snapshot* snap, uint32_t* out)` | same | fills flag | `ERR_BAD_PARAMETER` | cr_ui.cpp:3759 → 3673 |
| `cr_ui_snapshot_node` | `uint32_t (cr_ui_snapshot* snap, uint32_t index, cr_ui_node_info* out, char* name_buf, uint32_t name_cap)` | same; two-call name pattern (cr_ui.h:1088-1093) | fills caller struct/buffer | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` (name cap) | cr_ui.cpp:3762 → 3680 |
| `cr_ui_snapshot_dfs` | `uint32_t (cr_ui_snapshot* snap, uint32_t* indices, uint32_t* count)` | same; two-call (cr_ui.h:1095-1098) | caller index array | `ERR_BAD_PARAMETER`, `ERR_TRUNCATED` | cr_ui.cpp:3767 → 3727 |

### 8. Query (cr_ui.h:1100-1177)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_query_run` | `uint32_t (cr_ui_session* sess, const cr_ui_query* q, cr_ui_query_row** rows, uint32_t* row_count, uint64_t* total_matched)` | STABLE session trees only (cr_ui.h:1100-1104) | heap rows — caller frees via `cr_ui_query_rows_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:3874 → 3778 |
| `cr_ui_query_rows_free` | `void (cr_ui_query_row* rows, uint32_t row_count)` | any time | frees row array + name/path heap strings | none | cr_ui.cpp:3879 |
| `cr_ui_query_apply_buckets` | `uint32_t (cr_ui_query* q, uint32_t date_bucket, uint32_t size_bucket, uint64_t date_custom_from, uint64_t date_custom_to, uint64_t size_custom_min, uint64_t size_custom_max)` | pure translation, in place (M-029, cr_ui.h:1157-1177) | mutates caller query | `ERR_BAD_PARAMETER` (skipped/unknown bucket IDs 0x104/0x1005) | cr_ui.cpp:3933 → 3891 |

### 9. Carve registry + user signs (cr_ui.h:1179-1257)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_carve_format_count` | `uint32_t (void)` | any thread, any time — tables immutable (cr_ui.h:1185-1203) | value return | none — `carve::registry_size() + rawsigns::kFormatCount` (:3962-3963) | cr_ui.cpp:3961 |
| `cr_ui_carve_format_info` | `uint32_t (uint32_t index, cr_ui_carve_format* out)` | same | fills caller struct | `ERR_BAD_PARAMETER` (null out / index out of range :3969,:4004) | cr_ui.cpp:3966 |
| `cr_ui_register_user_signs` | `uint32_t (const char* json_bytes, uint32_t json_len)` | process-global; thread-safe (cr_ui.h:1238-1245) | replaces set atomically | `ERR_BAD_PARAMETER` (null/empty :4949, parse rc :4952, install rc :4954) | cr_ui.cpp:4948 |
| `cr_ui_user_sign_count` | `uint32_t (void)` | any thread | value return | none | cr_ui.cpp:4957 |
| `cr_ui_user_sign_info` | `uint32_t (uint32_t index, cr_ui_user_sign* out)` | snapshot-consistent read | fills caller struct | `ERR_BAD_PARAMETER` (:4962,:4969,:4975) | cr_ui.cpp:4961 |

### 10. Disk map (cr_ui.h:1259-1308)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_disk_map` | `uint32_t (cr_ui_session* sess, cr_ui_snapshot* snap, const cr_ui_disk_map_geometry* want, cr_ui_disk_map_geometry* got, uint8_t** allocation, uint8_t** coverage, uint8_t** found, uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap, uint32_t* bad_count)` | data producer; exactly ONE of sess/snap (cr_ui.h:1259-1304) | overlay arrays are HEAP, freed with `cr_ui_disk_map_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY`, `ERR_RANGE_OUT_OF_BOUNDS`, `ERR_TRUNCATED` (bad_cap < needed) | cr_ui.cpp:3593 → 3434 |
| `cr_ui_disk_map_free` | `void (uint8_t* overlay)` | any time; NULL no-op | frees ONE overlay array | none | cr_ui.cpp:3603 |

### 11. Preview / archive (cr_ui.h:1310-1383)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_preview_open_snapshot` | `uint32_t (cr_ui_device* dev, cr_ui_snapshot* snap, uint32_t index, cr_ui_preview** out)` | synchronous | private props copy; DEVICE must outlive preview (cr_ui.h:1310-1317) | `ERR_BAD_PARAMETER` (args :4018, index :4023, full node :4026), `ERR_NO_MEMORY` (:4029) | cr_ui.cpp:4188 → 4016 |
| `cr_ui_preview_open_session` | `uint32_t (cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, cr_ui_preview** out)` | synchronous | same | `ERR_BAD_PARAMETER` (args :4071, node :4077, no props :4080, full :4082), `ERR_NO_MEMORY` (:4086) | cr_ui.cpp:4192 → 4069 |
| `cr_ui_preview_close` | `void (cr_ui_preview* pv)` | any time | releases handle | none | cr_ui.cpp:4196 |
| `cr_ui_preview_size` | `uint32_t (cr_ui_preview* pv, uint64_t* out)` | synchronous | fills u64 | `ERR_BAD_PARAMETER` | cr_ui.cpp:4204 → 4103 |
| `cr_ui_preview_read` | `uint32_t (cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t len, uint32_t* placed)` | synchronous; 4 MiB/call cap; short only at EOF (cr_ui.h:1328-1333) | caller buffer | `ERR_BAD_PARAMETER`; reader rc pass-through (ERR_RANGE_OUT_OF_BOUNDS / ERR_READ_FAILED; zero-fill + report for unreadable spans) | cr_ui.cpp:4207 → 4110 |
| `cr_ui_preview_metadata` | `uint32_t (cr_ui_preview* pv, cr_ui_metadata* out)` | synchronous | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:4211 → 4120 |
| `cr_ui_preview_unreadable` | `uint32_t (cr_ui_preview* pv, uint64_t* total_bytes, uint32_t* range_count)` | synchronous | fills two outs | `ERR_BAD_PARAMETER` | cr_ui.cpp:4214 → 4140 |
| `cr_ui_archive_entry_count` | `uint32_t (cr_ui_preview* pv, uint32_t* out)` | cached at first archive call (M-116, cr_ui.h:1352-1376) | fills u32 (0 for non-archives, never an error) | `ERR_BAD_PARAMETER` | cr_ui.cpp:4218 → 4153 |
| `cr_ui_archive_entry_at` | `uint32_t (cr_ui_preview* pv, uint32_t index, cr_ui_archive_entry* out)` | same | fills caller struct | `ERR_BAD_PARAMETER`, `ERR_RANGE_OUT_OF_BOUNDS` | cr_ui.cpp:4221 → 4165 |

### 12. Thumbnail (cr_ui.h:1385-1458; M-102/M-083/M-115)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_thumbnail_generate` | `uint32_t (cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, const cr_ui_thumbnail_caps* caps, uint8_t* rgba, uint32_t rgba_cap, cr_ui_thumbnail_info* info)` | synchronous, one bounded generation (cr_ui.h:1427-1454); grade→2 promotion side effect | caller pixel buffer; two-call probe | `ERR_BAD_PARAMETER` (null out / caps range / size ≤ 20 nodes), `ERR_NO_MEMORY` (small buffer, bytes_needed set), `ERR_TRUNCATED` (decode refusal 0x50030004), `ERR_UNSUPPORTED` (phase-2 families, size ≤ 20 refusal), `ERR_READ_FAILED` (unreadable span), `ERR_INTERNAL` — mapped at thumb_status_to_abi:4234 | cr_ui.cpp:4386 → 4253 |
| `cr_ui_thumbnail_format_supported` | `uint32_t (const char* utf8_format)` | stateless probe | value return | `ERR_BAD_PARAMETER` (null) | cr_ui.cpp:4394 → 4378 |

### 13. Recovery (cr_ui.h:1460-1528)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_recovery_create` | `uint32_t (cr_ui_device* source, const cr_ui_recovery_config* cfg, cr_ui_recovery** out)` | synchronous setup | caller closes via `cr_ui_recovery_close`; source device + every enqueued session/snapshot borrowed (ABI-CONTRACT §2) | `ERR_BAD_PARAMETER` | cr_ui.cpp:4781 → 4404 |
| `cr_ui_recovery_close` | `void (cr_ui_recovery* rec)` | after process ends | releases handle | none | cr_ui.cpp:4786 |
| `cr_ui_recovery_set_destination` | `uint32_t (cr_ui_recovery* rec, const char* utf8_path)` | synchronous; arms F12 overlap refusal | normalized internally | `ERR_BAD_PARAMETER` | cr_ui.cpp:4794 → 4465 |
| `cr_ui_recovery_enqueue_node` | `uint32_t (cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` | synchronous | enqueues borrowed node | `ERR_BAD_PARAMETER` (incl. node not found) | cr_ui.cpp:4798 → 4531 |
| `cr_ui_recovery_enqueue_subtree` | `uint32_t (cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` | synchronous | recursive walk | `ERR_BAD_PARAMETER`, `ERR_DEST_OVERLAP` (per-item, 0x50050001) | cr_ui.cpp:4802 → 4585 |
| `cr_ui_recovery_enqueue_snapshot` | `uint32_t (cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` | synchronous; race-free value path (G-02b) | borrowed snapshot row | `ERR_BAD_PARAMETER` | cr_ui.cpp:4807 → 4621 |
| `cr_ui_recovery_enqueue_subtree_snapshot` | `uint32_t (cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` | synchronous | same | `ERR_BAD_PARAMETER`, `ERR_DEST_OVERLAP` | cr_ui.cpp:4812 → 4646 |
| `cr_ui_recovery_queue_count` | `uint32_t (cr_ui_recovery* rec, uint32_t* out)` | any time | fills u32 | `ERR_BAD_PARAMETER` | cr_ui.cpp:4817 → 4713 |
| `cr_ui_recovery_process` | `uint32_t (cr_ui_recovery* rec)` | SYNCHRONOUS on caller's thread (cr_ui.h:1507-1511) | committed items stay committed on cancel | `ERR_BAD_PARAMETER`; writer rc pass-through (ERR_CANCELLED, ERR_DEST_*, ERR_HASH_MISMATCH, ERR_READ_FAILED …) | cr_ui.cpp:4820 → 4721 |
| `cr_ui_recovery_cancel` | `uint32_t (cr_ui_recovery* rec)` | ANY thread while process() executes | token state | `ERR_BAD_PARAMETER` | cr_ui.cpp:4823 → 4735 |
| `cr_ui_recovery_get_stats` | `uint32_t (cr_ui_recovery* rec, cr_ui_recovery_stats* out)` | any time | fills caller struct | `ERR_BAD_PARAMETER` | cr_ui.cpp:4826 → 4742 |
| `cr_ui_recovery_report_json` | `uint32_t (cr_ui_recovery* rec, const char* source_utf8, char** json_out)` | valid any time (G-09, cr_ui.h:1524-1528) | HEAP string — caller frees with `cr_ui_string_free` | `ERR_BAD_PARAMETER`, `ERR_NO_MEMORY` | cr_ui.cpp:4830 → 4759 |

### 14. Health & image producer (cr_ui.h:1530-1594)

| symbol | signature | threading | ownership | errors (impl) | impl |
|---|---|---|---|---|---|
| `cr_ui_health_read` | `uint32_t (uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs, uint32_t attr_cap, uint32_t* attr_count)` | synchronous; Windows only; read-only | fills report + caller attr rows; *attr_count = needed either way | `ERR_BAD_PARAMETER` (:4846), `ERR_OPEN_FAILED` (:4855, elevation-aware text), `ERR_HEALTH_UNAVAILABLE` (:4918, no source); non-Win `ERR_UNSUPPORTED` (:4933) | cr_ui.cpp:4940 → 4843 / stub 4926 |
| `cr_ui_image_create` | `uint32_t (uint32_t disk, const char* utf8_path, cr_ui_recovery_progress_fn progress, void* user)` | synchronous; driver-gated class (a) (S-040, cr_ui.h:1576-1594) | streamed to `<path>.crimg-tmp`, atomic rename + `.sha256` sidecar; partial KEPT as `<path>.partial` on cancel/failure | `ERR_BAD_PARAMETER` (:1750,:1771), `ERR_DRIVER_*` (:1760-1766), `ERR_CANCELLED` (:1778), create rc pass-through; non-Win `ERR_UNSUPPORTED` (:1791) | cr_ui.cpp:4987 → 1748 |

**Guarded/catch-all coverage (implemented):** 75/90 exports route through the
`guarded()` catch-all (cr_ui.cpp:136-144 → `ERR_INTERNAL` + last_error text);
6 void close/free entries use inline `try{...}catch(...){}` (device_close:1841,
scan_close:3028, session_close:3607, snapshot_close:3748, preview_close:4196,
recovery_close:4786); the remaining 9 (abi_version, last_error, string_free,
last_transport_error, disk_map_free, query_rows_free, register_user_signs,
user_sign_count, user_sign_info) are non-throwing by construction or delegate
to internally-guarded core seams (`carve::usersigns`). The "every entry has a
catch-all" rule (cr_ui.h:24-25; ABI-CONTRACT.md:24-25) therefore holds via
delegation for those 9.

---

## Structs

All structs are POD, C-layout stable, append-only (ABI-CONTRACT §9). No struct
documents a byte size in-header; only array field widths are pinned. Zero-init
is always safe (cr_ui.h:30-31).

| struct | fields (types) | semantics / notes | cite |
|---|---|---|---|
| `cr_ui_event` | `uint32_t type, stage; uint64_t timestamp_ms; uint64_t bytes_done, bytes_total; uint32_t stage_permille, overall_permille; uint64_t files_found, deleted_found, carved_found; uint64_t err_start_bytes, err_len_bytes; uint32_t error_ranges; uint64_t checkpoint_index; uint64_t candidate_lba; uint32_t repair_actions, repair_outcome; uint32_t err_module, err_operation, err_attempts, err_context_pad; uint64_t check_errors; uint32_t check_family, check_pad; uint64_t autosave_rc` | the typed event POD; serializeable byte-for-byte; trailing pads keep it an 8-byte multiple (always 0); v1-era append = repair pair; v2-era = err_context + check pair; W1c append = autosave_rc | cr_ui.h:225-272 |
| `cr_ui_driver_status_t` | `uint32_t state, service_present, service_running, device_openable, authorized, info_valid, major, minor, required_major, flags, max_disks, ioctl_base, last_gle` | 6-facet driver status; state = CR_UI_DRIVER_STATE_*; last_gle = diagnostic Win32 error | cr_ui.h:379-394 |
| `cr_ui_geometry` | `uint64_t total_bytes; uint32_t logical_sector, physical_sector; uint64_t sector_count` | 4Kn-aware geometry out | cr_ui.h:430-435 |
| `cr_ui_disk_info` | `uint32_t number; uint64_t size_bytes; uint32_t logical_sector, physical_sector, removable, layout, table_ok; char model[128]; char serial[128]; uint32_t trim_known, trim_on` | census row; layout 0 none/1 MBR/2 GPT/3 EBR (N-057); M-076 TRIM append (zero-init = unknown) | cr_ui.h:449-473 |
| `cr_ui_volume_info` | `char letter; uint32_t disk; uint64_t offset_on_disk, bytes; char fs_name[16]; char label[64]; uint32_t bitlocker` | census row; bitlocker 1 = FVE detected | cr_ui.h:475-483 |
| `cr_ui_repair_plan` | `uint32_t detected; char fs_family[8]; uint32_t action_count, refused; char reason[160]; uint32_t advisory_count` | S-008 consent-gate POD summary (fired via callback) | cr_ui.h:550-558 |
| `cr_ui_scan_config` | `uint32_t stage_mask, whole_disk, fs_hint, quick_weight, deep_weight, carve_weight, threads; const char* volume_label_utf8, volume_model_utf8, volume_serial_utf8, volume_firmware_utf8, volume_wwn_utf8, volume_interface_utf8, checkpoint_path_utf8; cr_ui_progress_fn progress; void* progress_user; uint32_t pre_scan_repair; cr_ui_repair_consent_fn repair_consent; void* repair_consent_user; uint32_t memory_warn_mb; const char* raw_category_name_utf8; uint32_t pre_scan_check; const char* autosave_dir_utf8; const char* const* filter_folders_utf8; uint32_t filter_folder_count` | zero-init = defaults; stage_mask 0 = 7; label truncated to 99 chars (N-090); v1/v2-era + W1c + M-073 appends all tail-appends | cr_ui.h:562-664 |
| `cr_ui_stage_tel` | `uint32_t permille; uint64_t units_done, units_total, bytes_done, elapsed_ms; uint32_t completed` | per-stage telemetry; elapsed pause-compensated | cr_ui.h:698-705 |
| `cr_ui_telemetry` | `uint32_t overall_permille; cr_ui_stage_tel quick, deep, carve; uint64_t files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms; uint32_t error_ranges, paused, running, cancelled` | live pull snapshot; bytes_scanned = carve-true bytes | cr_ui.h:707-715 |
| `cr_ui_scan_stats` | `uint64_t files_found, deleted_found, carved_found, lost_partitions, bytes_scanned, elapsed_ms, cached_hits, cached_misses; uint32_t error_ranges; uint32_t repair_ran, repair_planned, repair_applied, repair_denied; uint32_t check_ran, check_errors, check_detected; uint64_t bytes_unreadable; uint32_t probe_unreadable; uint32_t autosaved` | final counters; v1-era repair + v2-era check + driver-only read-truth + W1c autosaved appends | cr_ui.h:720-756 |
| `cr_ui_assistant_stats` | `uint64_t total_file_count, raw_count, lost_partition_exist_file_count, lost_partition_delete_file_count, quick_scan_exist_file_count, quick_scan_delete_file_count, deep_scan_file_count, quick_scan_delete_extend; uint64_t category_counts[8]; uint64_t matched, total; uint32_t small_result; uint64_t graded_count` | S-023 grading statistics (og RecoveryAssistant); matched forced 0 when total < 100 | cr_ui.h:766-782 |
| `cr_ui_apply_candidate` | `uint64_t start_lba, sector_count; uint32_t fs; uint32_t disk_layout, disk_flags` | S-029 planner input; fs = lost::CandidateFs values; M-137 presentation tail (zero-init whole struct) | cr_ui.h:814-832 |
| `cr_ui_apply_plan` | `uint32_t refused, scope; uint64_t start_lba, sector_count; uint32_t mbr_type, slot, regenerate_signature, signature; char reason[160]` | planner output; scope 0 single-entry / 1 rebuild-table | cr_ui.h:834-844 |
| `cr_ui_apply_result` | `uint32_t applied, outcome; uint64_t undo_entries; char reason[160]` | outcome 0..5 (applied / denied / refused / stale / refresh-failed / error) | cr_ui.h:848-856 |
| `cr_ui_session_info` | `uint32_t fs_family, stage_done_mask, configured_mask; uint64_t slice_offset, slice_bytes; uint32_t logical_sector, physical_sector; char label[32]; char label_full[100]; uint64_t scanned_bytes, elapsed_ms; char model[48]; char serial[32]; uint32_t boot_hash` | loaded-session summary; N-090 label_full append (== label on v1..v5); boot_hash 0 = unverifiable | cr_ui.h:929-943 |
| `cr_ui_session_row` | `uint32_t valid, error, fs_family, stage_done_mask, configured_mask; uint64_t files_found, deleted_found, carved_found, bytes_scanned, elapsed_ms, save_unix; char label[32]; char path[1024]; char label_full[100]` | W1a listing row; `CR_UI_SESSION_ROW_PATH 1024`; N-090 label_full append; save_unix 0 = pre-v5 | cr_ui.h:990-1009 |
| `cr_ui_node_info` | `uint32_t parent, first_child, next_sibling, prev_sibling; uint64_t node_id; uint32_t fs_kind; uint64_t record_ref; uint32_t origin, deleted, confidence, grade, overlap, is_full; uint64_t logical_size, allocated_size; uint64_t created, modified, changed, accessed; uint32_t attributes, flags, extent_count, resident_bytes, carve_format, carve_confidence, hash_valid; uint8_t sha128[16]; uint32_t name_len` | snapshot row; links are indices (CR_UI_SNAP_NONE 0xFFFFFFFF); grade = tree::Grade 0..2 (S-023); timestamps 100ns-1601 | cr_ui.h:1058-1082 |
| `cr_ui_query` | `uint32_t category_mask; uint64_t size_min, size_max, modified_from, modified_to; uint32_t confidence_min; const char* const* extensions; uint32_t extension_count; const char* name_substring, path_prefix, path_substring; uint32_t state_mask, deleted_only, origin_mask, sort, dir; uint64_t offset; uint32_t limit, count_only` | zero-init = match-all; M-072 path_substring/state_mask appends; limit 0 = default 100 | cr_ui.h:1118-1140 |
| `cr_ui_query_row` | `uint64_t node_id; char* name; char* path; uint64_t size, modified; uint32_t origin, deleted, confidence, category, grade` | HEAP name/path — free whole array with `cr_ui_query_rows_free` | cr_ui.h:1142-1150 |
| `cr_ui_carve_format` | `uint32_t layer; char ext[24]; uint64_t min_size, max_size; uint16_t base_confidence; uint32_t magic_len, magic_offset; uint8_t magic[16]` | layer 0 grammar / 1 raw-sign; magic fields grammar-layer only | cr_ui.h:1189-1198 |
| `cr_ui_user_sign` | `char ext[9]; char mime[81]; uint32_t offset, sign_len; uint8_t bytes[48]` | v3/M-067 widths (bytes 16→48, mime 64→81; offset ≤ 1024, sign_len 1..48) — the field resize that justified ABI v3 | cr_ui.h:1247-1254 |
| `cr_ui_disk_map_geometry` | `uint64_t start_byte, total_bytes, cell_bytes, cell_count; uint32_t cluster_bytes, fs_family` | in: view hints (want); out: actually-used geometry (got); cell_bytes 0 = auto | cr_ui.h:1291-1298 |
| `cr_ui_metadata` | `char kind[24]; char format[24]; uint32_t width, height; char orientation[8]; char datetime[40]; uint64_t duration_ms; char codec[32]; char text_snippet[256]; uint32_t truncated` | fixed-capacity UTF-8; truncation reported via `truncated` | cr_ui.h:1338-1348 |
| `cr_ui_archive_entry` | `char name[260]; uint64_t size, compressed; uint32_t crc_present, name_truncated` | M-116 inventory row; archive-native UTF-8 pass-through | cr_ui.h:1368-1374 |
| `cr_ui_thumbnail_caps` | `uint32_t max_edge; uint64_t max_pixels; uint32_t max_read, max_work, allow_embedded` | 0-field = engine default (256 / 64 MP / 200 MB / 4 Mi / embedded ON — no OFF position by design, N-137) | cr_ui.h:1407-1413 |
| `cr_ui_thumbnail_info` | `uint32_t width, height, bytes_needed; char source_format[16]; uint8_t from_embedded, truncated; uint8_t _pad[2]` | output dims + two-call probe answer; truncated crosses Ok-but-truncated decodes | cr_ui.h:1415-1425 |
| `cr_ui_recovery_config` | `uint64_t read_chunk; uint32_t rename_attempts, skip_hash_verify, skip_dir_reconstruct, conflict; cr_ui_recovery_progress_fn progress; void* progress_user` | defaults 1 MiB / 64 / verify ON / reconstruct ON / conflict 0 = rename | cr_ui.h:1468-1476 |
| `cr_ui_recovery_stats` | `uint64_t queued, done, refused, errors, hash_mismatch, resumed, salvaged, dirs; uint64_t nothing_to_recover` | v1-era `nothing_to_recover` append (zero-init safe) | cr_ui.h:1514-1520 |
| `cr_ui_smart_attr` | `uint32_t id, value, worst; uint64_t raw; uint32_t failing; char name[40]` | one SMART attribute row (raw = 6 vendor bytes) | cr_ui.h:1534-1540 |
| `cr_ui_smart_report` | `uint32_t available; char transport[24]; uint32_t ata_version, smart_version, attribute_count, failing_now; uint64_t reallocated_sectors, pending_sectors, uncorrectable, power_on_hours, spin_retry, ssd_wear_level, temperature_c; uint32_t nvme_available, temperature_c_x10, available_spare, available_spare_threshold; uint64_t percentage_used, data_units_read, data_units_written, power_on_hours_nvme, unsafe_shutdowns, media_errors; uint32_t critical_warning; uint32_t ssd_like, trim_known, trim_enabled; uint64_t host_reads, host_writes, controller_busy_time, num_error_log_entries, data_bytes_read, data_bytes_written` | ABI v2 (M-036) tail: the last six NVMe fields + data-unit→byte conversion | cr_ui.h:1542-1568 |

Opaque handle types (cr_ui.h:93-99): `cr_ui_device`, `cr_ui_scan`,
`cr_ui_session`, `cr_ui_snapshot`, `cr_ui_preview`, `cr_ui_recovery` — opaque
by design; internal impl structs `CrDevice`(cr_ui.cpp:504), `CrScan`(:832),
`CrSession`(:918), `CrSnapshot`(:942), `CrPreview`(:972), `CrRecovery`(:990).

---

## Enums

### The error-code enum — `cr::Err` (include/cr/cr_error.h:25-114), u32 values

**True census: `OK` + 43 error codes = 44 values.** (The doc claim "45 + OK"
counts 2 phantom regex artifacts — see "Doc claims vs tree".)

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
| health 0x5007 | 0x50070001 | `ERR_HEALTH_UNAVAILABLE` | :83 |
| repair/apply 0x5008 | 0x50080001 | `ERR_REPAIR_REFUSED` | :84 |
| | 0x50080002 | `ERR_REPAIR_UNDO_FAILED` | :85 |
| | 0x50080003 | `ERR_VOLUME_LOCK_FAILED` (S-010) | :86 |
| | 0x50080004 | `ERR_APPLY_CONSENT_DENIED` (S-029 A1) | :89 |
| | 0x50080005 | `ERR_APPLY_STALE` (A2) | :90 |
| | 0x50080006 | `ERR_APPLY_REFUSED` (A3) | :91 |
| | 0x50080007 | `ERR_APPLY_REFRESH_FAILED` (A4) | :92 |
| internal 0x5009 | 0x50090001 | `ERR_INTERNAL` | :99 |
| image 0x500A | 0x500A0001 | `ERR_IMAGE_CORRUPT` (S-030) | :96 |
| driver 0x500B | 0x500B0001 | `ERR_DRIVER_ABSENT` | :105 |
| | 0x500B0002 | `ERR_DRIVER_NOT_STARTED` | :106 |
| | 0x500B0003 | `ERR_DRIVER_UNAUTHORIZED` | :107 |
| | 0x500B0004 | `ERR_DRIVER_VERSION_MISMATCH` | :108 |
| | 0x500B0005 | `ERR_DRIVER_ERRORED` | :109 |

`const char* err_text(u32)` (cr_error.h:117) is the shared text surface.
Header comment pins the space as CORE-INTERNAL (D-68; cr_error.h:4-13).

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
| error-context modules | `CR_ERRMOD_NONE 0 … CR_ERRMOD_LDM 12` (13 values; pinned to `cr::errmod::Module` by static_asserts cr_ui.cpp:93-105) | cr_ui.h:283-295 |
| error-context ops | `CR_ERROP_NONE 0`, `CR_ERROP_READ 1` | cr_ui.h:297-298 |
| enumeration flags | `CR_ENUM_FLAG_TRIM 0x1` | cr_ui.h:489 |
| snapshot sentinel | `CR_UI_SNAP_NONE 0xFFFFFFFF` | cr_ui.h:1056 |
| session-row path width | `CR_UI_SESSION_ROW_PATH 1024` | cr_ui.h:990 |
| core module enum | `errmod::Module kNone 0 … kLdm 12` | cr_errmod.h:50-64 |
| log levels | `log::Level Error 0 / Warn 1 / Info 2` | cr_log.h:48-52 |
| fs-kind codes | `FsKindCode Fat12 0, Fat16 1, Fat32 2, Ntfs 3, Ext2 4, Raw 6, Exfat 7, Hfsplus 8, Ext4 9, Refs 10, Btrfs 11, Zfs 13` | cr_types.h:81-101 |

---

## Callbacks

| callback | signature | when invoked | threading | cite |
|---|---|---|---|---|
| `cr_ui_progress_fn` | `void (*)(void* user, uint32_t stage, uint32_t stage_permille, uint32_t overall_permille, uint64_t bytes_done, uint64_t bytes_total, uint32_t error_ranges)` | live scan progress (quick/deep/carve); stage 0/1/2/0xFFFFFFFF; permilles 0..1000 | worker thread(s) only; must not block; NO cancellation semantics; impl serializes under `progress_mu` + monotonic CAS dedup (cr_ui.cpp:1150-1161) | cr_ui.h:115-127 |
| `cr_ui_recovery_progress_fn` | `void (*)(void* user, uint64_t bytes_done, uint64_t bytes_total)` | cumulative bytes during `cr_ui_recovery_process` and `cr_ui_image_create` (64 KiB chunks) | thread running process()/image_create; must not block | cr_ui.h:129-130 |
| `cr_ui_event_fn` | `void (*)(void* user, const cr_ui_event* evt)` | the typed event stream (12 types); SAVE/LOAD progress fire from the calling thread of save/load | engine worker threads, the run() thread, the shim's heartbeat watchdog thread (N-043), or the save/load caller; no shim-side serialization (single-flight is the sink's job); must not block; no cancellation; unknown types MUST be ignored | cr_ui.h:132-150, 274; impl trampoline cr_ui.cpp:1025-1063, heartbeat :1096-1148 |
| `cr_ui_repair_consent_fn` | `uint32_t (*)(void* user, const cr_ui_repair_plan* plan)` | exactly ONCE per run() when `pre_scan_repair` set and a repairable plan exists | on the run() thread, before any engine read; nonzero return = execute transactional repair (default-deny on NULL/zero) | cr_ui.h:534-560 |
| `cr_ui_apply_consent_fn` | `uint32_t (*)(void* user, const cr_ui_apply_plan* plan)` | exactly ONCE per apply, before any write | synchronous on the caller's thread; zero/NULL = denied, nothing written | cr_ui.h:846 |
| `cr::log::SinkFn` | `void (*)(cr::log::Level, const char* line, void* user)` (C++, exported via .def) | every engine log line (Error/Warn/Info) | ANY thread, called under the seam's internal mutex; sink must be cheap or internally deferred; `set_sink` any time | cr_log.h:54-59 |

---

## Versioning/lifecycle

- **`CR_UI_ABI_VERSION` = 3** (cr_ui.h:81). v2 = M-036 (six `cr_ui_smart_report`
  tail fields); v3 = M-067 (`cr_ui_user_sign` field resize: bytes 16→48, mime
  64→81, sign_len 1..48, offset ≤ 1024 — the first FIELD change under the
  policy). Compatibility policy (cr_ui.h:39-48 = ABI-CONTRACT §9, corrected by
  A-ABI-1): adding exports / appending struct fields / adding event types are
  COMPATIBLE (no bump); removing/renaming exports, changing field
  type/meaning/order, changing an existing symbol's ownership or threading
  contract, renumbering error symbols → bump. There is no v1.1/v1.2 (N-040
  doc-defect cleanup).
- **No global init/shutdown**: the library is initialization-free
  (`cr_ui_carve_format_count` etc. callable "before any other call — no init
  ordering", ABI-CONTRACT §7a). The gate is `cr_ui_abi_version()` call-first.
- **Scan lifecycle** (ABI-CONTRACT §5, cr_ui.h:519-527): `scan_create` →
  `scan_attach_device` (borrow; must outlive) → `scan_run` (synchronous,
  one-shot; double-run = ERR_BAD_PARAMETER, cr_ui.cpp:2361-2362) →
  snapshot/save → `scan_close` → `device_close`. Cancellation returns
  ERR_CANCELLED with the tree browsable; save-on-cancel is the caller's choice.
- **Session lifecycle**: `session_load[_with_events]` → query/snapshot/preview/
  regrade → `session_close`; resume: `session_load` → `session_resume` →
  attach → `verify_identity` → `run`.
- **Recovery lifecycle**: `recovery_create` → `set_destination` → `enqueue_*`
  → `process` (caller thread) → `report_json` → `close`.
- **Save/auto-save**: `cr_ui_scan_save_session` is atomic (staging `.tmp` +
  rename; cr_ui.cpp:2289-2356, N-014/N-060); `cr_ui_scan_save_cancel` is
  save-scoped; completion auto-save (W1c) fires CR_EVT_AUTOSAVED under the
  S-044 gates and — since S-057 — also on a user-STOPPED run with results
  (cr_ui.cpp:2439-2454).
- **Threading model (ABI-CONTRACT §3 = header cr_ui.h:34-38)**: run()/process()
  synchronous on the caller's thread; pause/resume/cancel/telemetry/snapshot/
  busy/stopping/save_cancel safe from any thread; one handle = one controlling
  thread otherwise; events may arrive on worker/watchdog threads. Implemented
  with per-scan `ctrl_mu`/`progress_mu`/`heartbeat_mu` + `running`/`save_cancel`
  atomics (cr_ui.cpp:846-906), a heartbeat watchdog thread (:1096-1148), and a
  process-global `g_last_transport_error` atomic (:1376).

---

## Export list vs header

- `src/ui/cr_ui_exports.def` lists **exactly one symbol** (cr_ui_exports.def:19):
  `?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z` — the MSVC x64 decorated
  form of `void cr::log::set_sink(void (*)(cr::log::Level, const char*),
  void*)`, declared at cr_log.h:59. This is deliberate (the .def header comment
  :1-16: the shim's own test drives the log sink seam; on Linux the statically
  linked cr_core objects keep default visibility). It is added to the build
  only under MSVC (src/ui/CMakeLists.txt:31-33).
- **No C ABI symbol is .def-listed** — the 90 implemented `cr_ui_*` functions
  export via `__declspec(dllexport)` (CR_UI_BUILD) / visibility("default").
  Cross-check of all 90: header signature ↔ cpp definition identical (name +
  arity verified by extraction; comm -13/-23 empty except the one below).
- **Mismatch found — 1 header declaration with no definition and no .def
  entry**: `cr_ui_session_resume_last` (cr_ui.h:927, with a full contract
  comment at cr_ui.h:917-926). Zero occurrences in `src/` (repo-wide grep:
  only cr_ui.h:927 and two diff-to-fix citations). Introduced header-only by
  commit `b2bc1970` (M-154 rider; that commit touched cr_ui.h +12 lines and
  cr_lastscan.cpp but NOT cr_ui.cpp). Consequence: the DLL exports **90** C
  functions, while the header (and every doc count) says **91**.
- The single .def symbol IS declared in a public header (cr_log.h:59) — no
  .def-only phantom. No header-declared C symbol is missing from the .def
  (none are expected there under the dllexport mechanism).

---

## Doc claims vs tree

| claim | doc cite | tree truth | verdict |
|---|---|---|---|
| "91 exports" (header-derived: 94 `CR_UI_API` lines − 3 `#define`s) | CORE-API-CURRENT.md:14-15; API.md:95; API-SURFACE-AUDIT.md:32 | header declares 91 ✓, but cr_ui.cpp defines only **90** (`cr_ui_session_resume_last` unimplemented) | count matches the HEADER, not the DLL |
| "…= 91 `extern "C" CR_UI_API` definitions in src/ui/cr_ui.cpp, recount matches" | CORE-API-CURRENT.md:15-16 | **false** — 90 definitions (verified: 90 `extern "C" CR_UI_API` defs, 90 unique symbols) | MISMATCH (F2) |
| "the count re-derived …: 92 `CR_UI_API` lines minus 3 `#define` macros" | API.md:96; API-SURFACE-AUDIT.md:32-33 | tree has **94** CR_UI_API lines (94−3=91); 92−3=89 contradicts the "91" in the same sentence | STALE derivation (F5); CORE-API-CURRENT.md:14 carries the corrected 94 |
| "error codes **45 + OK** (case-normalized census …; the doc_truth_gate derives this)" | CORE-API-CURRENT.md:18; :43 | enum census = **43 codes + OK (44 values)**. The 45 comes from doc_truth_gate `derive_abi_counts` (tools/doc_truth_gate.py:319) matching ANY `0x5` + 7 hex chars in cr_error.h — it double-counts the og-corpus comment literals `0x5002200000004`/`0x5002c0000004` (cr_error.h:6-7) as `0x50022000`/`0x5002c000` | INFLATED by 2 phantom matches (F3). ("six codes uppercase" is accurate: 0x500A0001 + 0x500B0001..05.) |
| "ABI event types **12**" | CORE-API-CURRENT.md:22; API-SURFACE-AUDIT.md §1 | 12 `CR_EVT_*` macros, values 1..12 (cr_ui.h:151-223) | MATCHES ✓ |
| `CR_UI_ABI_VERSION` = 3 | API.md:93; ABI-CONTRACT.md:766 | cr_ui.h:81 = 3u | MATCHES ✓ |
| ABI-CONTRACT §6 inline `cr_ui_event` copy | ABI-CONTRACT.md:260-307 | header struct has the W1c append `uint64_t autosave_rc` (cr_ui.h:269-271) — the doc copy omits it while §7h (:628-631) documents it | doc struct snapshot STALE (F6); header wins per ABI-CONTRACT.md:9-10 |
| §(b) "every export (78; `CR_UI_ABI_VERSION 2`, cr_ui.h:79)"; b.13 "type 1..10" | CORE-API-CURRENT.md:175, :304 | 91 decls / 90 defs, ABI 3 (cr_ui.h:81), events 1..12 | pre-driver-only census residue inside a section billed LIVE-re-verified (:4-7); superseded banner covers only §(e)/§(f) (F7) |
| §(d).1 "the FULL enumerated list" | CORE-API-CURRENT.md:425-463 | table omits 9 live codes: ERR_DEVICE_DISCONNECTED (cr_error.h:42), ERR_DEST_PATH_TOO_LONG/INVALID/NETWORK (:75-77), ERR_DRIVER_* ×5 (:105-113); :465-472 asserts NO driver codes exist | stale "FULL" list (F8) |
| "C++ public headers 82 under include/cr/**" | CORE-API-CURRENT.md:13 | 82 per-module headers (include/cr/*/*.h) + 9 top-level = 91 files under include/cr/** (90 C++ excluding cr_ui.h) | ambiguous/undercounted phrasing (F9) |
| events fire "on the thread running run() only" (A-ABI-5 class) | — (API-SURFACE-AUDIT.md:134 records it as fixed) | ABI-CONTRACT.md:341-346 now says worker threads + watchdog; header cr_ui.h:140-150 same | FIXED ✓ |
| §5 phantom names `session_disk_map`/`scan_disk_map` (A-ABI-4) | — (API-SURFACE-AUDIT.md:133) | ABI-CONTRACT.md:169 now names `cr_ui_disk_map` | FIXED ✓ |
| SQLite sidecar claim (A-ABI-2) | — | ABI-CONTRACT.md:747-762 is now the REJECTED-BY-REFERENCE closure; zero sqlite in src/ | FIXED ✓ |
| `cr_ui_session_last_autosave_path` errors "ERR_NOT_FOUND" | cr_ui.h:1021-1023 | no ERR_NOT_FOUND in the error space (cr_error.h); impl returns ERR_RANGE_OUT_OF_BOUNDS (cr_ui.cpp:3647); C gate pins 0x50020004 (tests/ui_abi_check.c:592-593) | header doc drift (F4) |
| "each call re-queries the OS — no cache" (enumerate) | API.md:114; ABI-CONTRACT.md:149-156 | enumerate_disks_impl constructs a fresh PhysicalDiskList per call (cr_ui.cpp:1865-1869) | MATCHES ✓ |
| driver-only refusal taxonomy (open_disk etc.) | ABI-CONTRACT.md:87-97, §7e | open_disk_impl/open_volume_impl/with_key/image_create all gate via query_driver_status → ERR_DRIVER_*; no fallback leg (cr_ui.cpp:1383-1419, 1442-1455, 1536-1548, 1752-1768) | MATCHES ✓ |
| transport values "rda"/"image" only | API.md:120; CORE-API-CURRENT banner :36-38 | transport strings "rda"/"image" only (cr_ui.cpp:1363, :1409, :1464) | MATCHES ✓ |

---

## Findings

1. **[HIGH] Phantom export — `cr_ui_session_resume_last` declared, never
   implemented.** cr_ui.h:927 (contract comment :917-926); zero definitions in
   src/ (repo grep: only the header + diff-to-fix citations); introduced
   header-only in commit b2bc1970 (M-154 rider touched cr_ui.h but not
   cr_ui.cpp). The DLL exports 90 `cr_ui_*` C symbols, not 91. A host binding
   by the header gets an unresolved symbol / NULL GetProcAddress; the
   documented error contract (BAD_PARAMETER / ERR_OPEN_FAILED dead-pointer
   class) is unreachable. Fix: implement it (load the pointed auto session via
   `session::last_scan_info_read` + `session_load`) or remove the declaration
   and re-derive the doc counts.
2. **[HIGH] False impl-count claim.** CORE-API-CURRENT.md:14-16 — "94
   `CR_UI_API` lines minus 3 `#define` macros in cr_ui.h = 91 `extern "C"
   CR_UI_API` definitions in src/ui/cr_ui.cpp, recount matches": the header
   arithmetic is right, the cpp recount is wrong (90 definitions).
3. **[MED] Error-code count inflated by a derivation artifact.** The
   "45 + OK" claims (CORE-API-CURRENT.md:18, :43) inherit
   tools/doc_truth_gate.py:319's regex, which matches truncated prefixes of
   the og-corpus literals in the cr_error.h header comment (:6-7:
   0x5002200000004, 0x5002c0000004 → counted as 0x50022000/0x5002c000). True
   census: 43 error codes + OK (44 values). The gate passes while overstating
   by 2 — the count invariant itself needs a comment-stripping fix.
4. **[MED] Documented error code does not exist.** cr_ui.h:1021-1023 says
   `cr_ui_session_last_autosave_path` returns `ERR_NOT_FOUND` on a dead
   pointer; no such code exists in cr_error.h:25-114; the implementation
   returns ERR_RANGE_OUT_OF_BOUNDS (cr_ui.cpp:3647) and the C consumer gate
   pins 0x50020004 (tests/ui_abi_check.c:592-593). Re-truth the header comment
   (ERR_RANGE_OUT_OF_BOUNDS is a strained fit for "not found" — an
   ERR_NOT_FOUND code or ERR_OPEN_FAILED would be the honest spelling).
5. **[MED] Stale count derivations in two of the three gate docs.** API.md:96
   and API-SURFACE-AUDIT.md:32-33 both derive the 91 as "92 `CR_UI_API` lines
   − 3 `#define`s" — the tree has 94 lines, and 92−3=89 contradicts the "91"
   claimed in the same sentence. CORE-API-CURRENT.md:14 carries the corrected
   94-line form (commit 497b1277 fixed only one of the three sites' derivations).
6. **[LOW] ABI-CONTRACT §6 struct snapshot missing the W1c append.** The
   inline `cr_ui_event` copy at ABI-CONTRACT.md:260-307 ends at `check_pad`;
   the header's `uint64_t autosave_rc` (cr_ui.h:269-271) is absent, while the
   doc's own §7h (:628-631) documents `autosave_rc`. Per the doc's preamble
   (:9-10) the header wins and the doc must be fixed in the same change.
7. **[LOW] LIVE-section staleness in CORE-API-CURRENT §(b).** :175 "every
   export (78; CR_UI_ABI_VERSION 2, cr_ui.h:79)" and :304 "type 1..10" are
   pre-driver-only census values; the superseded banner (:33-47) covers only
   §(e)/§(f), while the preamble (:4-7) bills §(b) as cite-refreshed.
8. **[LOW] §(d).1's "FULL enumerated list" omits 9 live codes**
   (CORE-API-CURRENT.md:425-463): ERR_DEVICE_DISCONNECTED, the three M-074
   ERR_DEST_PATH_* codes, and the five ERR_DRIVER_* codes; :465-472 still
   asserts "there is NO code for driver-absent / not-started /
   version-mismatch / unauthorized" — false at this tree (cr_error.h:101-113).
9. **[INFO] Header-count phrasing ambiguity.** CORE-API-CURRENT.md:13 "C++
   public headers 82 under `include/cr/**`" — 82 is the per-module header
   count; include/cr/** holds 91 files (9 top-level + 82 module).
10. **[INFO] Catch-all coverage is delegation-based for 9 exports.** 75/90
    route through `guarded()` (cr_ui.cpp:136-144), 6 void closes use inline
    try/catch, and 9 trivially-non-throwing entries (abi_version, last_error,
    string_free, last_transport_error, disk_map_free, query_rows_free,
    register_user_signs, user_sign_count, user_sign_info) rely on internal core
    guards — the "every entry has a catch-all" rule (cr_ui.h:24-25) holds, but
    only transitively for those.
11. **[INFO] Event-type macro ordering is non-monotonic in the header source**
    (CR_EVT_AUTOSAVED 12 at cr_ui.h:210 precedes CR_EVT_SESSION_LOAD_PROGRESS
    11 at :195) — cosmetic, values are correct.
12. **[INFO] No .def ↔ header signature drift for the 90 implemented
    functions.** The single .def symbol (cr::log::set_sink) matches its
    declaration (cr_log.h:59) exactly; the .def is MSVC-only by design
    (src/ui/CMakeLists.txt:31-33).

### Per-module header sweep (internal C++ layer; 82 headers)

One line each (first doc-comment line, verified). **Bold** = included by the
ABI layer (cr_ui.cpp:26-53).

- apply/cr_apply.h — consent-gated lost-partition apply (S-029). **included**
- bcdfix/cr_bcdfix.h — BCD partition-offset rewriter (M-040a).
- btrfs/cr_btrfs_cache.h — Btrfs re-scan metadata cache (N-072).
- btrfs/cr_btrfs_engine.h — Btrfs scan engine.
- carve/cr_carve.h — the carving engine (R4).
- carve/cr_rawsigns.h — raw-signature carving layer (139 formats). **included**
- carve/cr_signatures.h — versioned signature registry + validators. **included**
- carve/cr_usersigns.h — user-extensible signature database (M-048). **included**
- check/cr_fscheck.h — damage-audit check phase (M-041, taxonomy 0x1001..0x100e).
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
- win/cr_bitlocker.h — BitLocker-aware handling (R8). **included (Win)**
- win/cr_bootfix.h — Windows boot-chain repair runtime (M-040).
- win/cr_destio.h — Win32 atomic temp+rename DestIo. **included (Win)**
- win/cr_driver_status.h — driver-state taxonomy + PURE classifier.
- win/cr_elevate.h — elevation probe (GAP-P1-5). **included (Win)**
- win/cr_enumerate.h — disk/volume enumeration. **included (Win)**
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
layer includes cr_log.h and cr_errmod.h directly (cr_ui.cpp:23-24) and pins
the errmod ids with static_asserts (cr_ui.cpp:93-107).

---

### Count-check summary (the task's three numbers)

- **Exported functions: 91 declared / 90 implemented.** Derivation: 94
  `CR_UI_API` lines in cr_ui.h − 3 macro `#define`s (cr_ui.h:67,69,72) = 91
  declarations; 90 `extern "C" CR_UI_API` definitions in cr_ui.cpp (unique
  symbol extraction); 1 additional C++ export via cr_ui_exports.def:19
  (`cr::log::set_sink`). The docs' "91 exports" counts the header, and
  CORE-API-CURRENT.md:15's "recount matches" against cr_ui.cpp is false.
- **Error codes: 43 (+ OK = 44 values)** in cr_error.h:25-114 — NOT 45. The
  doc/gate figure of 45 includes two regex artifacts from the og-facility
  comment literals at cr_error.h:6-7 (reproduced with the gate's own regex).
- **Event types: 12** (CR_EVT_* values 1..12, cr_ui.h:151-223) — matches the
  docs. (Core-side enum emits only 8 of them; 8, 9, 11, 12 are shim-side.)

---

## Session-90 coherence addendum (re-stamped at the pre-v0.5.3 close)

Deltas against the Session-84 record (91 declared / 90 implemented,
e8dcaba8):

- **The census is now 93 declared / 93 implemented.** The M-075/M-078
  window added two exports this sheet predated:
  - `cr_ui_fsck_volume` (include/cr/cr_ui.h:1617, impl src/ui/cr_ui.cpp:5058,
    the M-075 chkdsk launch endpoint; args (char letter, uint32_t flags,
    int32_t* exit_code) per cr_ui.h:1617; the OK / ERR_BAD_PARAMETER /
    ERR_OPEN_FAILED / ERR_UNSUPPORTED contract is the win-layer
    fsck_volume's (include/cr/win/cr_fsck.h:36-41, timeout bound inside
    the win layer))
  - `cr_ui_turn_off_bitlocker` (cr_ui.h:1635, impl cr_ui.cpp:5091, the
    M-078 consented destructive twin of the unlock surface)
- **F1 is resolved**: `cr_ui_session_resume_last` (the phantom) landed
  at a47419c7 — declarations and implementations now balance exactly
  (93 = 93, counted first-hand this session).
- Error enum: 43 codes + OK, as recorded (M5 holds).
- WPD/M-101 (1ff8ae24, b7d05b12): core-internal only — no public ABI
  surface change (validate_wpd is engine-internal; version_group /
  tags_extracted ride the tree/query C++ surface, not the C ABI).
  Observation filed for the next behavior session: `cr_ui_query_row`
  does not yet expose the version-group axis.

— Session-90 coherence lane. End of addendum.

### Post-window correction: re-verified at the v0.6.0 cut - doc_truth_gate's
own census confirms exports=93 / error codes=43 at the merged HEAD; the
cr_ui_session_resume_last implementation (M-154, a47419c7) and the
archive ABI pair are inside the 93. The 93/93 balance stands.
