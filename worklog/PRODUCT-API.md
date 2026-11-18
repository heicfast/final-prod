# PRODUCT-API — the product's CURRENT complete API surface sheet

> Phase 0, Agent D (TASK 2-d), Installer Integration & API Surface Expansion session.
> READ-ONLY census: every count below was re-derived from source at the working
> tree (no doc claim was taken on faith; doc-vs-code drift found during the
> census is recorded in §9). Written before any code changes, per the charter.

## 0. Header

| Field | Value | Source |
|---|---|---|
| Product repo | `heicfast/final-prod`, worktree HEAD `0c2c3c52` (the T1 charter commit; the session's named frontier is `8217bc26`, the T19 footprint reconciliation — v0.9.3 tagged there) | `git log --oneline -3`; `worklog.md:3204-3214` |
| Date | 2026-11-18 (Phase 0) | — |
| **REAL ABI export count** | **93** — counted from the header myself: 93 function declarations matching `^CR_UI_API` in `engine/include/cr/cr_ui.h` (96 total `CR_UI_API` occurrences minus 3 `#define` macro lines at :67/:69/:72); cross-checked: `rg -c '^CR_UI_API'` = 93, and the Rust binding `ui/src-tauri/src/bridge/abi.rs` declares exactly 93 `extern "C" fn cr_ui_*` | cr_ui.h (full read); `rg -c '^CR_UI_API' include/cr/cr_ui.h` = 93; `rg -c 'fn cr_ui_'` over abi.rs = 93 |
| ABI version | **4** — `#define CR_UI_ABI_VERSION 4u` (cr_ui.h:81); the v3→v4 bump removed the driver-era `cr_ui_driver_status` export with the kernel driver (the one sanctioned removal) | cr_ui.h:75-81; engine/docs/API.md:108-118; engine/docs/CORE-API-CURRENT.md:11-28,55 |
| Doc-agreement on 93 | engine docs agree and CI-enforce it: `engine/docs/API.md:110` ("93 exported functions… the 93rd is `cr_ui_health_read_ex2`"), `engine/docs/CORE-API-CURRENT.md:13` (same, doc_truth_gate count invariants) | engine/tools/doc_truth_gate.py:296-331 (`derive_abi_counts`) |
| Stale-count note | `contracts/README.md:13` still says "92 `cr_ui_*` exports" and "9 domains, 36 methods" (:15) — both stale vs. HEAD (93 exports; connector is 12 domains / 67 methods, §4). The engine-side doc_truth_gate only scans `engine/docs/*` + engine README (doc_truth_gate.py:54-56,121), so the contracts README drifted unseen | contracts/README.md:13,15; §9 below |
| Event types | **12** `CR_EVT_*` (values 1–12) | cr_ui.h:151-223 |
| Error-code space | shared with `cr_error.h`; **40 `ERR_` members + OK** at HEAD | CORE-API-CURRENT.md:19-28; my `rg` census of cr_error.h = 40 |
| errmod attribution modules | 13 `CR_ERRMOD_*` (kNone + 12) + 2 `CR_ERROP_*` | cr_ui.h:292-307; CORE-API-CURRENT.md:25-26 |

---

## 1. ABI export inventory — 93 exports, grouped by domain

Source of truth: `engine/include/cr/cr_ui.h` (1,701 lines), normative companion
`engine/docs/ABI-CONTRACT.md` (§1-§10). ABI RULES block (cr_ui.h:23-48): C11,
no C++ types cross, no exception escapes (catch-all → `CR_UI_ERR_INTERNAL`
0x50090001), every fn returns a u32 from the `cr_error.h` numeric space
(0 = success), strings-in NUL-terminated UTF-8, strings-out two-call size
pattern or heap strings freed by `cr_ui_string_free` (:113), zero-initialized
config structs always safe, handles opaque (close every handle).

**Threading/cancellation/lifecycle contract (cr_ui.h:34-38; ABI-CONTRACT §3,
:55-75):** `scan_run`/`recovery_process` run SYNCHRONOUSLY on the caller's
thread; `pause`/`resume`/`cancel`/`telemetry`/`snapshot` capture are safe from
any thread while run() executes; progress/event callbacks fire on ENGINE
WORKER THREADS (and the 5 s heartbeat watchdog thread, N-043), must not block,
and carry NO cancellation semantics — the token functions are the cancel path
(the reference's separate abort-event discipline). One handle = one
controlling thread. Ownership/borrow table: ABI-CONTRACT §2 (:39-53) — device
borrows to scans/previews/recovery; preview owns a private copy of node props
but the DEVICE must outlive it; snapshot values are copies (source may close
after the last row is read); recovery borrows source device + enqueued
sessions/snapshots.

### 1.1 Version & diagnostics (3)
| Export (cr_ui.h:line) | Signature summary | Notes / error contract |
|---|---|---|
| `cr_ui_abi_version` :105 | `u32(void)` → `CR_UI_ABI_VERSION` | call first; refuse to bind on mismatch |
| `cr_ui_last_error` :110 | `const char*(void)` | thread-local diagnostic text of the LAST failing call on this thread; stable until next `cr_ui_*` call on that thread; never NULL |
| `cr_ui_string_free` :113 | `void(char* s)` | frees heap strings returned by the API (free() also valid) |

### 1.2 Event subscription (1)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_scan_set_event_fn` :312 | `u32(scan*, cr_ui_event_fn fn, void* user)` | subscribe BEFORE run() (the sink is read at scan start; calling while run() executes → ERR_BAD_PARAMETER); one sink per scan handle; pass NULL to unsubscribe. Payload is the `cr_ui_event` POD (cr_ui.h:225-281) — byte-serializable, unknown type values must be ignored (forward compat). Event types 1-12: SCAN_STAGE 1, SCAN_PROGRESS 2, FILES_FOUND 3, ERROR_RANGE 4 (M-049 module/op/attempts attribution: `err_module` CR_ERRMOD_*, `err_operation` CR_ERROP_*, `err_attempts`), CHECKPOINT 5, LOST_CANDIDATE 6, REPAIR 7 (S-008 pre-scan repair outcome 0-3), HEARTBEAT 8 (N-043, fired from the shim's watchdog thread every 5 s while run() is in flight and no progress arrived — standing telemetry only, never synthetic progress), SAVE_PROGRESS 9 (N-060, save-thread), FS_CHECK 10 (M-041 damage-audit phase done, taxonomy 0x1001..0x100e, never aborts the scan), SESSION_LOAD_PROGRESS 11 (W1b, calling thread, caller-owned cancel flag polled per node), AUTOSAVED 12 (W1c, fired once after run() completes when autosave_dir set; cancelled runs never auto-save). cr_ui.h:151-223 |

### 1.3 Devices (9)
| Export | Signature summary | Error/threading notes |
|---|---|---|
| `cr_ui_open_image` :322 | `u32(const char* utf8_path, u64 offset, u64 bytes, device** out)` | portable (the ABI-check/test surface); bytes==0 = to EOF; exposed as a 512-byte-sector RO device bounded [offset, offset+bytes) (:324-325) |
| `cr_ui_open_disk` :339 | `u32(u32 disk, device** out)` | Windows-only (ERR_UNSUPPORTED elsewhere); DIRECT raw I/O `\\.\PhysicalDriveN` — no kernel driver since session-96; single precondition is the process token: non-elevated → **ERR_ELEVATION_REQUIRED 0x500C0001** (the host triggers its own UAC flow and retries; nothing elevates silently); failure under an elevated token is a target error (ERR_OPEN_FAILED et al) (:327-338) |
| `cr_ui_open_volume` :340 | `u32(char letter, device** out)` | Windows; BitLocker probed first — locked volume → **ERR_FVE_LOCKED 0x50060001** unless auto-unlock succeeds; returned device = volume's slice on parent disk (:334-338) |
| `cr_ui_last_transport_error` :349 | `u32(void)` | M-030 advisory diagnostic of the LAST open (carries the elevation refusal code or surviving target error); never gates an open's rc, never cleared, process-global (:342-348) |
| `cr_ui_open_volume_with_key` :361 | `u32(char letter, const char* utf8_hex_fvek, device** out)` | M-033 in-stack XTS-AES decrypt; 64 hex (AES-128) or 128 hex (AES-256) key\|\|tweak-key; ERR_BAD_PARAMETER on malformed/weak key; ERR_UNSUPPORTED off-Windows; locked-without-key stays ERR_FVE_LOCKED (:351-360) |
| `cr_ui_unlock_volume_with_dictionary` :377 | `u32(char letter, const char* const* pwds, u32 count)` | N-064 explicit-consent unlock (shipped 228-entry dictionary + caller candidates tried after); OK on unlock, ERR_FVE_LOCKED when set failed, ERR_UNSUPPORTED off-Windows; log lines carry candidate INDEX only, never the password (:364-376) |
| `cr_ui_device_close` :381 | `void(device*)` | — |
| `cr_ui_device_geometry` :390 | `u32(device*, cr_ui_geometry* out)` | 4Kn-aware (logical/physical sector, sector_count) (:383-388) |
| `cr_ui_device_transport` :396 | `u32(device*, char* buf, u32 cap, u32* needed)` | "direct" or "image" (two-call); the CLI's `[transport]` lines (:392-395) |

### 1.4 Enumeration (3, Windows layer; ERR_UNSUPPORTED elsewhere)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_enumerate_disks` :467 | `u32(cr_ui_disk_info* out, u32* count)` | two-call (*count = IN capacity when out!=NULL, OUT needed when NULL); failure returns its error code — never a silent empty list (CLI G-20 parity); each call RE-QUERIES the OS — no cache, calling again IS the refresh (N-098/R-86 pull model, :458-466). Row = `cr_ui_disk_info` (:401-436): number/size/sectors/removable/layout (0 none,1 MBR,2 GPT,3 EBR N-057)/table_ok/model[128]/serial[128] + M-076 TRIM tail (`trim_known`/`trim_on`, filled only by the `_ex` opt-in) + D-ENUM-02 `bus_type` (raw STORAGE_BUS_TYPE, 0=unknown → degrade to removable) |
| `cr_ui_enumerate_disks_ex` :468 | `u32(out, count, u32 flags)` | the M-076 opt-in: `CR_ENUM_FLAG_TRIM 0x1` (:452) adds the per-row TRIM chip datum (one fresh fsutil read per invocation, OS-global, stamped on every row; failed query → trim_known==0, never guessed); unknown flag bits refused ERR_BAD_PARAMETER before the platform check (:448-451, ABI-CONTRACT §5 :146-153) |
| `cr_ui_enumerate_volumes` :469 | `u32(cr_ui_volume_info* out, u32* count)` | row = letter/disk/offset_on_disk/bytes/fs_name[16]/label[64]/bitlocker (:438-446) |

### 1.5 Scan lifecycle, telemetry, stats, repair-undo (15)
| Export | Signature summary | Notes |
|---|---|---|
| `cr_ui_scan_create` :629 | `u32(const cr_ui_scan_config* cfg, scan** out)` | config = `cr_ui_scan_config` (:525-627): stage_mask 0=default(quick\|deep\|carve=7), whole_disk, fs_hint (0 auto; serialized FsFamily values incl. 11 UDF M-044, 10 BitLocker probe-only never a hint), weights 0=40/35/25, threads 0=auto, volume label/model/serial + N-066 firmware/WWN/interface identity overrides, checkpoint_path, progress fn, pre_scan_repair (S-008 consent gate), memory_warn_mb (N-062 monitor, 0=OFF), raw_category_name (M-029), pre_scan_check (M-041, never aborts), autosave_dir (W1c; fires on completion AND on user-stopped-with-results per S-057/N-211; empty/resumed/already-saved runs never auto-save per S-044), M-073 `filter_folders_utf8`+count (the scan-time folder filter — out-of-prefix folders pruned at stage boundaries before checkpoints, persisted as REC_SCAN_FILTER). Zero-init = safe defaults everywhere |
| `cr_ui_scan_close` :630 | `void(scan*)` | — |
| `cr_ui_scan_attach_device` :635 | `u32(scan*, device*)` | borrowed; attach before run(); identity card auto-filled from enumeration on Windows (:632-635) |
| `cr_ui_scan_run` :640 | `u32(scan*)` | synchronous on the CALLER's thread; returns 0, an engine error, or ERR_CANCELLED with tree intact (take a snapshot / save the session afterwards — the caller's explicit choice) (:637-640, :483-490) |
| `cr_ui_scan_pause` :643 | `u32(scan*)` | any thread while running; blocks engines cooperatively |
| `cr_ui_scan_resume` :644 | `u32(scan*)` | releases a pause; pause-compensated elapsed |
| `cr_ui_scan_cancel` :645 | `u32(scan*)` | any thread; makes run() return ERR_CANCELLED |
| `cr_ui_scan_busy` :658 | `u32(scan*, u32* out)` | N-075 synchronous state queries (the og's sigIsBusy ×14 / sigIsStopping ×8 as ABI ops); any thread any time; busy = live run() execution (:647-657) |
| `cr_ui_scan_stopping` :659 | `u32(scan*, u32* out)` | the live cancel REQUEST on the token (distinct from telemetry's post-run `cancelled` flag) |
| `cr_ui_scan_telemetry` :712 | `u32(scan*, cr_ui_telemetry* out)` | any thread any time; live snapshot: overall/phase (D-P6 `phase` 0 quick/1 advanced + `phase_permille`, the og's per-phase gauge restart, 9618d9ed), per-stage `cr_ui_stage_tel` quick/deep/carve, files/deleted/carved/lost counters, `bytes_scanned` (P-DEEP-01: live reading position of the ACTIVE byte stage), elapsed pause-compensated, error_ranges, paused/running/cancelled, `bytes_found` (D4b live found-size), `current_offset_bytes` (P-TICK-01 the real sweep cursor, 0 = not sweeping) (:661-709) |
| `cr_ui_scan_get_stats` :752 | `u32(scan*, cr_ui_scan_stats* out)` | final counters + repair_* (S-008 outcome), check_* (M-041), read-truth `bytes_unreadable`/`probe_unreadable` (a scan that could read essentially nothing REFUSES with ERR_READ_FAILED — never fake-completes), `autosaved` (W1c) (:714-750) |
| `cr_ui_scan_assistant` :781 | `u32(scan*, cr_ui_assistant_stats* out)` | S-023 scan-completion grading statistics (the og's RecoveryAssistant block: total/raw/lost/quick/deep counts, category_counts[8], matched/total gauge with the <100 forced-zero small-result heuristic, graded_count) (:760-781) |
| `cr_ui_session_regrade` :787 | `u32(session*, const u64* node_ids, u32 count, u32* regraded)` | per-selection re-grade (the og's FUN_1401d7110); only grade-1 nodes eligible — grades 0/2 sticky (:783-786) |
| `cr_ui_scan_checkpoint_count` :789 | `u32(scan*, u64* out)` | G-13 checkpoints (queryable truth; CHECKPOINT events are internal) |
| `cr_ui_scan_repair_save_undo` :798 | `u32(scan*, const char* path)` | S-008: persist the pre-scan repair's undo log (CRC-checked container); BAD_PARAMETER when nothing applied or scan has not run; operator restores via `rrecov fix --undo-apply <file>` (:791-797) |

### 1.6 Lost-partition apply (3, consent-gated; ABI-since 1.3)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_apply_lost_partition` :859 | `u32(device* dev, const cr_ui_apply_candidate* cand, u32 rebuild_table, cr_ui_apply_consent_fn consent, void* user, cr_ui_apply_result* out)` | whole-disk view only (volume slices refused — table lives at disk LBA 0); plan(read-only refusals)→consent callback fires exactly ONCE with the POD plan before any write; zero/NULL consent = default-deny, nothing written (outcome 1); result outcome 0 applied/1 denied/2 refused/3 table-changed/4 refresh-tail-failed/5 error (:800-862). Candidate carries M-137 presentation tail `disk_layout`/`disk_flags` (:808-826) |
| `cr_ui_apply_undo_last` :866 | `u32(device*, cr_ui_apply_result* out)` | byte-exact sector restore + properties refresh; idempotent (:864-865) |
| `cr_ui_apply_save_undo` :871 | `u32(device*, const char* path)` | persist undo log; BAD_PARAMETER when no undo log exists; loadable by `rrecov fix --undo-apply` (:868-870) |

### 1.7 Save / snapshot / identity (4)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_scan_save_session` :876 | `u32(scan*, const char* path)` | the full truthful .crsf (same record set as the CLI); valid after run() including a cancelled one (:873-875). Save is ATOMIC (staging `<path>.tmp` + rename — a torn save never destroys the previous good file; ABI-CONTRACT §5 :208-221) |
| `cr_ui_scan_save_cancel` :886 | `u32(scan*)` | N-060 SAVE-SCOPED cancel (never touches the scan's cancel token — save-on-scan-cancel stays intact); aborted save returns ERR_CANCELLED, final path keeps previous content, no .tmp remains; not-in-flight = no-op; safe from any thread (:878-885) |
| `cr_ui_scan_snapshot` :892 | `u32(scan*, snapshot** out)` | G-02 immutable value snapshot; callable from ANY thread WHILE run() executes (partial=1 stamped — mid-scan truth) and after (partial=0) (:888-891) |
| `cr_ui_scan_verify_identity` :900 | `u32(scan*)` | boot-hash check for RESUMED scans (the CLI's warning): first 512 bytes vs session's stored hash; ERR_BAD_CHECKSUM on mismatch (different volume), 0 on match or unverifiable (legacy session); call after resume+attach, before run() (:894-899) |

### 1.8 Sessions (10)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_session_load` :908 | `u32(const char* path, session** out)` | a loaded .crsf = stable tree (no engine running against it) — queries/preview/recovery enqueue directly (:902-906) |
| `cr_ui_session_close` :909 | `void(session*)` | — |
| `cr_ui_session_resume_last` :921 | `u32(const char* dir, session** out)` | M-154 pointed resume (the og's continue-last without enumeration): reads the `last_scan.info` sidecar, loads the AUTO session it names; errors: BAD_PARAMETER (null args), ERR_OPEN_FAILED (no pointer / dead pointer); any thread, no shared state (:911-920) |
| `cr_ui_session_get_info` :939 | `u32(session*, cr_ui_session_info* out)` | fs_family (D-1 truth), stage_done/configured masks, slice offset/bytes, sectors, label[32]+N-090 label_full[100], scanned/elapsed, model/serial, boot_hash (:923-937) |
| `cr_ui_session_stats` :940 | `u32(session*, cr_ui_scan_stats* out)` | — |
| `cr_ui_session_snapshot` :941 | `u32(session*, snapshot** out)` | — |
| `cr_ui_session_resume` :948 | `u32(session*, const cr_ui_scan_config* original_cfg, scan** out)` | loads into a FRESH tree, validates completion mask, returns a scan handle that continues where the session stopped; attach ORIGINAL device, optionally verify_identity, then run() (:943-947) |
| `cr_ui_session_list` :1005 | `u32(const char* dir, cr_ui_session_row* rows, u32* count)` | W1a/R-56: every *.crsf yields ONE row from LIGHT records only (SESSION_STATE+VOLUME_INFO; heavy payloads skipped by TYPE before payload I/O); two-call; rows fill NEWEST-FIRST by embedded save time (deterministic tie-break by path bytes); capacity<needed → ERR_TRUNCATED with the newest prefix filled; unreadable files still get a row (valid=0 + error — a listing never hides damage); pre-v5 rows: save_unix=0 (display file mtime, never fabricated). Row carries N-090 label_full[100]; path[1024] truncation → row error ERR_TRUNCATED (:951-1006) |
| `cr_ui_session_last_autosave_path` :1023 | `u32(const char* dir, char* out_path, u32 cap)` | the pointer-resume query: OK + pointed path when pointer exists AND file present; **ERR_RANGE_OUT_OF_BOUNDS 0x50020004** when no live pointer names a present file (impl's actual code, ABI-test-pinned; enum carries no ERR_NOT_FOUND) (:1008-1024) |
| `cr_ui_session_load_with_events` :1043 | `u32(const char* path, cr_ui_event_fn fn, void* user, volatile u32* cancel_flag, session** out)` | W1b/R-54: load with CR_EVT_SESSION_LOAD_PROGRESS (coalesced, calling thread) + CALLER-OWNED cancel flag polled PER NODE even when fn==NULL; on cancel → ERR_CANCELLED, *out untouched (a torn load never half-claims state); with NULL fn+flag = exactly plain session_load (:1026-1045) |

### 1.9 Snapshot enumeration (5)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_snapshot_close` :1089 | `void(snapshot*)` | — |
| `cr_ui_snapshot_node_count` :1090 | `u32(snapshot*, u32* out)` | — |
| `cr_ui_snapshot_partial` :1091 | `u32(snapshot*, u32* out)` | 1 = captured mid-scan |
| `cr_ui_snapshot_node` :1097 | `u32(snapshot*, u32 index, cr_ui_node_info* out, char* name_buf, u32 name_cap)` | two-call name (NULL buf → needed size); capped copy returns ERR_TRUNCATED when it did not fit (:1093-1096). Node info = parent/child/sibling indices (CR_UI_SNAP_NONE 0xFFFFFFFF), node_id (stable; preview/recovery addressing), fs_kind, record_ref (NTFS hard-link grouping), origin (0 structured/1 carved/2 lost/3 virtual), deleted state (0 active/1 deleted/2 partial/3 damaged), confidence 0..4, grade 0..2 (S-023 CRecoverability verdict), overlap flag, logical/allocated sizes, 4×100ns-1601 timestamps, DOS attributes, extents count, carve provenance, sha128 content hash + M-101 `version_group` (NTFS namespace-identity) / `tags_extracted` (:1055-1087) |
| `cr_ui_snapshot_dfs` :1103 | `u32(snapshot*, u32* indices, u32* count)` | depth-first emission order (byte-parity with the live serializer); two-call, TRUNCATED on short capacity (:1100-1103) |

### 1.10 Query — session trees (3)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_query_run` :1167 | `u32(session*, const cr_ui_query* q, cr_ui_query_row** rows, u32* row_count, u64* total_matched)` | core-side filter/sort/paging over a LOADED session (G-12 — the improvement beyond the reference, whose filtering is client-side); mid-scan filtering = snapshot enumeration + client predicates (:1105-1109). Query = category_mask bits 1-8, size/date ranges (0=unbounded, 100ns-1601), confidence_min, extensions[] (lowercased with dot), name_substring, M-072 path_prefix/path_substring/state_mask, deleted_only, origin_mask, sort 0-7 (6 grade S-023 desc-first, 7 type M-029 FileFilter iconIndex order), dir, offset/limit (0=default 100 — a HOST-side model, no og paging parity claimed, ABI-CONTRACT :817-827), count_only, M-101 version_group/tags_only (:1123-1152). Rows are heap arrays — free with `cr_ui_query_rows_free` |
| `cr_ui_query_rows_free` :1170 | `void(cr_ui_query_row* rows, u32 count)` | — |
| `cr_ui_query_apply_buckets` :1189 | `u32(cr_ui_query* q, u32 date_bucket, u32 size_bucket, u64 date_custom_from/to, u64 size_custom_min/max)` | M-029: translates the og filter-bar's stable action IDs (Date 0x100/0x101/0x102/0x103/0x105 Custom; Size 0x1000..0x1004/0x1006 Custom) into range fields IN PLACE; day-truncated LOCAL windows from the host clock (og-exact); the SKIPPED ids (0x104, 0x1005) and unknown values REJECTED ERR_BAD_PARAMETER — the core never invents menu semantics the og lacks (:1172-1188, ABI-CONTRACT :829-843) |

### 1.11 Carve-format registry + tally (4)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_carve_format_count` :1218 | `u32(void)` | ops #7-#8 parity (the og's EUGetAnalyserList family): read-only enumeration of BOTH layers — grammar-validator registry rows first (FormatId order), then raw-sign rows (kFormats order); deterministic, stable across releases, append-only; no handle, safe from any thread (:1194-1217) |
| `cr_ui_carve_format_info` :1221 | `u32(u32 index, cr_ui_carve_format* out)` | BAD_PARAMETER out-of-range; row = layer/ext[24]/min/max size/base_confidence 0..1000/magic (:1204-1213) |
| `cr_ui_scan_carve_tally_count` :1241 | `u32(scan*)` | N-140 per-format carve tally (the og's GetRawUserInfoMap): live slot bound over the unified format-id space + ONE aggregate user-sign slot at the end; 0 = no carve stage ran / session predates the tally (:1223-1240) |
| `cr_ui_scan_carve_tally` :1242 | `u32(scan*, u32 index, u64* out)` | count at index; BAD_PARAMETER null/out-of-range |

### 1.12 User-signature database (3, ABI-since 2.1 M-048)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_register_user_signs` :1260 | `u32(const char* json_bytes, u32 json_len)` | the og's EUUserData JSON shape verbatim; parse+validate+install ATOMICALLY (bad payload leaves the previous set — never a silent partial load); re-registration REPLACES; process-global, thread-safe; registered signs probed by every carve pass AFTER built-ins (:1244-1259) |
| `cr_ui_user_sign_count` :1271 | `u32(void)` | enumeration for hosts rendering the active set |
| `cr_ui_user_sign_info` :1272 | `u32(u32 index, cr_ui_user_sign* out)` | v3/M-067 widths: ext[9], mime[81], offset 0..1024, sign_len 1..48, bytes[48] (the first struct FIELD change under the stability policy — why v3 exists; ABI-CONTRACT §9 :771-790) |

### 1.13 Disk map (2)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_disk_map` :1315 | `u32(session*, snapshot*, const cr_ui_disk_map_geometry* want, geometry* got, u8** allocation, u8** coverage, u8** found, u8** confidence, u64* bad_ranges, u32 bad_cap, u32* bad_count)` | GAP-S6-11 the visualization DATA PRODUCER (AnalysisGridWidget parity): exactly ONE of sess/snap; `want` optional for sessions (default = scanned slice), REQUIRED for snapshots; cell_bytes 0 = engine cluster or 64 KiB default; overlays each optional (at least one required): allocation 0 unknown/1 used/2 free (S-009 engine bitmap truth)/3 bad, coverage 0 unscanned/1 scanned, found (capped 255), confidence max 0..4; bad_ranges = u64 PAIR array, two-call, TRUNCATED after filling (:1274-1319) |
| `cr_ui_disk_map_free` :1323 | `void(u8* overlay)` | frees each overlay array separately; NULL no-op |

### 1.14 Preview (9)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_preview_open_snapshot` :1335 | `u32(device*, snapshot*, u32 index, preview** out)` | bounded chunked logical reads, NO recovery pass (G-10); reader owns a private props copy — source session/snapshot may close while preview lives; DEVICE must outlive it (:1325-1332) |
| `cr_ui_preview_open_session` :1337 | `u32(device*, session*, u64 node_id, preview** out)` | node addressing: snapshot row index vs session node_id (:1334) |
| `cr_ui_preview_close` :1339 | `void(preview*)` | — |
| `cr_ui_preview_size` :1341 | `u32(preview*, u64* out)` | — |
| `cr_ui_preview_read` :1347 | `u32(preview*, u64 offset, void* buf, u32 len, u32* placed)` | *placed = bytes actually assembled (short only at logical EOF; len capped **4 MiB per call**); unreadable spans ZERO-FILLED AND REPORTED — never silently swallowed (:1343-1346) |
| `cr_ui_preview_metadata` :1365 | `u32(preview*, cr_ui_metadata* out)` | bounded metadata: kind image/video/audio/document/archive/text/unsupported, format, dims, EXIF orientation/datetime, duration, codec, text_snippet[256], `truncated` flag (never silent) (:1350-1363) |
| `cr_ui_archive_entry_count` :1391 | `u32(preview*, u32* out)` | M-116 browse-inside: bounded 512-entry hostile-bound inventory extracted at the FIRST archive call and cached for the handle's lifetime; non-archive nodes report count 0 (never an error) (:1367-1390) |
| `cr_ui_archive_entry_at` :1392 | `u32(preview*, u32 index, cr_ui_archive_entry* out)` | row = name[260] (archive-native UTF-8 pass-through), size, compressed, crc_present, name_truncated |
| `cr_ui_preview_unreadable` :1397 | `u32(preview*, u64* total_bytes, u32* range_count)` | the truthful failure map (zero-filled spans) (:1395-1396) |

### 1.15 Thumbnail (2, M-102)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_thumbnail_generate` :1466 | `u32(device*, session*, u64 node_id, const cr_ui_thumbnail_caps* caps, u8* rgba, u32 rgba_cap, cr_ui_thumbnail_info* info)` | TWO-CALL pattern (rgba==NULL = size probe; too-small cap → ERR_NO_MEMORY with bytes_needed set, nothing partially written); display-oriented RGBA8 with EXIF orientation BEFORE downscale (S-043); status mapping engine-verbatim: ERR_TRUNCATED = decode REFUSAL (also sets info->truncated); in-band tolerated-short streams cross via info->truncated on Ok; ERR_UNSUPPORTED = phase-2 refusal families. Caps (:1422-1428): max_edge 0=256, max_pixels 0=64 MP, max_read 0=200 MB, max_work 0=4 Mi units, allow_embedded (0/1 — embedded EXIF fast path ALWAYS preferred, no off position by design N-137). M-083 grade-promotion: success promotes node grade→2 (monotonic, sticky); M-115: nodes ≤20 bytes refused ERR_UNSUPPORTED (no attempt, no promotion); a decode REFUSAL with clean reads STILL promotes (og promotes on read evidence) (:1400-1465) |
| `cr_ui_thumbnail_format_supported` :1473 | `u32(const char* utf8_format)` | probe: true for jpeg/png/bmp/gif/tiff/psd/pnm/hdr/raw (S-045 raw family) (:1471-1472) |

### 1.16 Recovery (12)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_recovery_create` :1493 | `u32(device* source, const cr_ui_recovery_config* cfg, recovery** out)` | config (:1483-1491): read_chunk 0=1 MiB, rename_attempts 0=64, skip_hash_verify 0=verify ON, skip_dir_reconstruct 0=ON (G-08), conflict 0 rename/1 skip/2 overwrite, progress fn |
| `cr_ui_recovery_close` :1495 | `void(recovery*)` | — |
| `cr_ui_recovery_set_destination` :1502 | `u32(recovery*, const char* path)` | normalized trailing separator; on Windows a drive-letter destination ARMS the F12 overlap refusal against disk-backed sources (image-file sources cannot overlap a volume — documented truth) (:1497-1501) |
| `cr_ui_recovery_enqueue_node` :1509 | `u32(recovery*, session*, u64 node_id)` | per-call errors (e.g. ERR_DEST_OVERLAP 0x50050001 — count refusals by return code) (:1504-1508) |
| `cr_ui_recovery_enqueue_subtree` :1511 | `u32(recovery*, session*, u64 node_id)` | recursive walk: every directory becomes a mkdir item (empty leaves materialize), then files |
| `cr_ui_recovery_enqueue_snapshot` :1515 | `u32(recovery*, snapshot*, u32 index)` | G-02b value path — mid-scan recovery without touching the live tree; race-free by construction (:1513-1514) |
| `cr_ui_recovery_enqueue_subtree_snapshot` :1517 | `u32(recovery*, snapshot*, u32 index)` | — |
| `cr_ui_recovery_queue_count` :1520 | `u32(recovery*, u32* out)` | live queue depth |
| `cr_ui_recovery_process` :1526 | `u32(recovery*)` | on the CALLER's thread (read→temp→verify→atomic commit); ERR_CANCELLED on cancel() — already-committed items stay committed; the report tells the truth about the rest (:1522-1525) |
| `cr_ui_recovery_cancel` :1527 | `u32(recovery*)` | safe from any thread |
| `cr_ui_recovery_get_stats` :1537 | `u32(recovery*, cr_ui_recovery_stats* out)` | queued/done/refused/errors/hash_mismatch/resumed/salvaged/dirs + `nothing_to_recover` (GAP-S6-7 metadata-only items skipped BEFORE any write) (:1529-1535) |
| `cr_ui_recovery_report_json` :1542 | `u32(recovery*, const char* source_utf8, char** json_out)` | the G-09 JSON report (schema `engine/docs/recovery-report-schema.md`) as a heap string (cr_ui_string_free); valid ANY time — unprocessed items report "pending" truthfully (:1539-1541) |

### 1.17 Health / SMART (2, Windows layer; ERR_UNSUPPORTED elsewhere)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_health_read` :1619 | `u32(u32 disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs, u32 attr_cap, u32* attr_count)` | one call fills the report AND up to attr_cap rows (*attr_count returns needed either way — two-call). Report (:1574-1615): available, transport ("ata-passthrough"/"nvme"/"legacy"), ATA versions, failing_now, reallocated/pending/uncorrectable, power-on/spin-retry/ssd-wear/temperature, NVMe health-log rows (M-036 full set: host reads/writes, controller busy, error-log entries, data-units→bytes ×512,000, power_cycles_nvme), critical_warning, ssd_like, trim_known/enabled, **D-P4 `grade`** (the og DiskHealth 4-state verdict — family-first grading: NVMe log via grade_nvme, else S-024 worst-of-attr ATA pass, else Unknown) (:1604-1614) |
| `cr_ui_health_read_ex2` :1622-1637 | `u32(u32 disk, rep, attrs, attr_cap, attr_count)` | BF-03 closure (the 93rd export, eb4bb5f3, additive — no bump): the SAME read with the vendor-threshold axis stamped on every attr row (`cr_ui_smart_attr.threshold` from 0xD1 READ_THRESHOLDS; 0 = unknown, never fabricated) + per-attr D-P4 `grade`; identical error contract + elevation gate; threshold-less engines: callers fall back to the base export on ERR_UNSUPPORTED (:1623-1634). The bridge calls ex2 FIRST with base fallback (ui/src-tauri/src/commands/health.rs:38,71) |

### 1.18 Imaging (1)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_image_create` :1656 | `u32(u32 disk, const char* path, cr_ui_recovery_progress_fn progress, void* user)` | S-040/R-93 flat raw disk-image producer: class (a) privilege-required (non-elevated → ERR_ELEVATION_REQUIRED, same gate as open_disk); progress = recovery-style cumulative bytes (may be NULL); streamed to `<path>.crimg-tmp`, atomic rename on completion + `<path>.sha256` sidecar; cancel/failure KEEPS `<path>.partial` (reported, never passed off as complete); off-Windows → ERR_UNSUPPORTED. **NO cancel export** — the win-layer CancelToken is not ABI-visible (D-IMG-1) (:1639-1655) |

### 1.19 Maintenance / privileged one-shots (2, Windows layer)
| Export | Signature | Notes |
|---|---|---|
| `cr_ui_fsck_volume` :1677 | `u32(char letter, u32 flags, i32* exit_code)` | M-075 chkdsk escalation: flags 0 check / 1 fix (`chkdsk X: /f`), other bits ignored (forward compat); exit_code RAW (0 clean/1 fixed/2 cleanup-scheduled/3 errors-remain rerun /f/-1 spawn failed/259 STILL_ACTIVE past the 15-min synchronous bound); OK when chkdsk ran; BAD_PARAMETER non-letter; ERR_OPEN_FAILED no volume at letter (loud refusal); ERR_UNSUPPORTED not elevated / off-Windows. The escalation ladder: in-engine pre_scan_check → this endpoint → host raw-carve fallback (:1659-1676) |
| `cr_ui_turn_off_bitlocker` :1695 | `u32(char letter, cr_ui_progress_fn progress, void* user)` | M-078 DESTRUCTIVE decrypt twin of the unlock surface: explicit opt-in only (nothing in the scan/open path ever calls it — D-16 legitimate-unlock-only untouched); dual chain PowerShell Disable-BitLocker primary / manage-bde -off fallback; **OK = ACCEPTED, not plaintext** (the OS decrypt continues in background); ERR_FVE_LOCKED when both forms failed; ERR_BAD_PARAMETER non-letter; ERR_UNSUPPORTED off-Windows; progress fires at request start + settle only (the OS decrypt percentage is not observable — honest contract) (:1679-1694) |

### 1.20 ABI version / lifecycle rules (the append-only policy)
From cr_ui.h:39-48 + ABI-CONTRACT §9 (:771-803):
- `CR_UI_ABI_VERSION` = 4 (cr_ui.h:81). **ADD-compatible (NO bump required):** adding exports, APPENDING struct fields, adding event `type` values, adding overlay kinds — consumers must already ignore unknowns. A bump is PERMITTED as a deliberate feature-batch marker (v2 = M-036's six smart_report fields shipped with a bump so hosts could branch on `>=2`).
- **Bump REQUIRED on:** removing/renaming an export, changing a field's type/meaning/order (an in-struct array resize included — why v3 exists: the user_sign resize), changing ownership or threading contract of an existing symbol, renumbering error-space symbols consumers must handle. The v3→v4 bump REMOVED driver-era `cr_ui_driver_status` with the kernel driver (session-96) — the one sanctioned removal (API.md:114-117).
- Fields never reordered/removed within a version; every appended field carries its landing (change id + era) in a header comment; no "v1.1/v1.2" ever shipped — the historical "ABI-since 1.x" annotations were doc defects rewritten to truthful era labels (ABI-CONTRACT :786-790).
- Stability classes (API.md §5 :298-307): C ABI append-only; CLI grammar additive, exit codes stable; C++ headers internal freedom (pin a commit); on-disk formats versioned records.
- **Layout pinning (BF-ABI-01 closure):** `tools/gen_abi_layouts.py` parses cr_ui.h's structs and emits BOTH halves of the layout gate from the SAME parse — `engine/tests/abi_layout_dump.c` (plain-C dumper; the MSVC-side offsetof TRUTH, run by ctest and by the bridge-integration lane) and `ui/src-tauri/it/tests/abi_layouts.rs` (GENERATED Rust mirror: 29 structs / 366 field offsets via `std::mem::offset_of!`, feature-gated `layout-gate`). The it-crate's `layouts_match_engine_manifest` test (abi_layouts.rs:469) diffs the engine manifest vs the Rust mirror field-for-field — a stale pin FAILS CI. Discipline: header gains a field → re-run the generator → re-pin `ui/src-tauri/src/bridge/abi.rs` in the SAME commit (the P-DEEP-01/P-TICK-01 commit 693d4d60 records the same-commit re-pin proof; abi.rs:8-18 tells the origin story — the eb4bb5f3 appends shipped without the re-pin: an 8×N-byte heap overflow in enumerate_disks + a 16-byte stack smash per health read).
- The MSVC export tail `engine/src/ui/cr_ui_exports.def` exports exactly one core symbol (`cr::log::set_sink`, mangled) for the test binary's log-seam use; on Linux the full core surface is reachable from libcr_ui.so's dynsym (cr_ui_exports.def:1-19).

---

## 2. CLI surface — `rrecov` (the engine's peer consumer)

Entry: `int wmain(int argc, wchar_t** argv)` — `engine/src/app/rrecov.cpp:3834`. Bare `rrecov` (argc<2) prints the usage block (:3840-3908) and exits 0 (:3909). Dispatcher: 13 verbs (:3911-3952); unknown verb → `unknown command`, exit 1 (:3951-3952). Zero `cr_ui_*` call sites — rrecov links only `cr::core`+`cr::win` and calls the core directly (the no-second-singleton rule; engine/worklog/cli-surface.md:18-23). Ctrl-C handler = cooperative cancel (token polled at loop heads; exit 3) (:3835-3838).

Consent-bearing flags (nothing write-side/unlock-side ever fires implicitly): `--repair-apply`, `--repair-undo`, `--undo-apply`, `--bitlocker-fvek`, `--bitlocker-dict`, `--apply`, `--boot-fix` (API.md:98-102; PARAMETERS.md §5 :120-136).

| Verb | Flags (usage block :3843-3903; PARAMETERS.md §1) | Notes |
|---|---|---|
| `list` | `--trim` | disk+volume census; --trim = per-row TRIM chip (M-076 opt-in, fsutil OS-global datum); enumeration failure is never exit-0 (FB-K01, :797) |
| `list-carve-formats` | — | the carve-capability projection of the ABI registry (the og's EUGetAnalyserList surface) (:3846-3848) |
| `sessions` | `--dir D` `--json` | the saved-session list (light summaries; unreadable files = per-row data; only enumeration failure exits 1) (:3849-3852) |
| `scan` | `--disk N` \| `--volume C:` \| `--image file(.vmdk/.vhd/.vhdx/.img, .dmg raw)`; `--deep` (deprecated alias of --whole-disk, prints a note), `--whole-disk`, `--quick-only`, `--no-carve`, `--threads N`, `--out file.crsf`, `--resume file.crsf`, `--log file`, `--autosave-dir D`, `--label s`, `--json`, `--csv`, `--filter s`, `--deleted-only`, `--pause-stdin` (N-178 keyboard pause: p/SPACE pause, r/Enter resume, Ctrl-C cancel; no-op when stdin redirected), `--filter-folder DIR` (repeatable, M-073), `--repair`, `--repair-apply`, `--repair-undo file`, `--check`, `--bitlocker-fvek hex`, `--bitlocker-dict [file]`, `--user-signs file` (:3853-3875) | deep stage always on; --quick-only disables it; --whole-disk = lost-partition discovery |
| `tree` | `--in file.crsf` `--filter s` `--deleted-only` `--csv` `--json` | load a session, print/export the result tree (:3876) |
| `archive-list` | `--in file.crsf` (`--disk N` \| `--image file`) `--node ID` `--json` | the M-116 browse-inside archive inventory (:3877-3878) |
| `recover` | `--in file.crsf` (`--disk N` \| `--image file`) `--to DIR` `--deleted-only` `--filter s` `--report file.json` (:3879-3881) | safe writer; exit 2 = partial |
| `health` | `--disk N` (:3882) | SMART/NVMe/TRIM; exit 3 = every source unavailable |
| `image` | `--create --disk N --out file.img` (:3883-3890) | flat raw producer + .sha256 sidecar; VHD/VHDX writers out of scope (D-147); cancel keeps `<out>.partial` |
| `fix` | `--disk N` `--part P` `--apply` `--undo file` `--undo-apply file`; `fix --chkdsk C:` (:3891-3894) | RawFixer analyze/dry-run by default + the M-075 chkdsk rung |
| `unlock` | `--turn-off C:` (:3895-3897) | M-078 consented BitLocker OFF (decrypt, never bypass; OS decrypt continues in background) |
| `apply` | `--disk N --session file.crsf --candidate N --apply --undo file --boot-fix old-lba [--boot-fix-bcd path]` (:3898-3901) | lost-partition MBR apply, dry-run by default; --boot-fix rewrites BCD offsets + UEFI NVRAM after the apply |
| `bench` | `--disk N --mode seq\|rand\|open\|read\|info --mb N --block-kb N --iterations N --seed N --json` (:3902-3903) | transport/device microbenchmarks; reports transport "direct"/"image" |

**Exit codes (API.md:72-81 table, code-verified):**
| Code | Meaning | Code cite |
|---|---|---|
| 0 | success (also bare-rrecov usage) | :3909; :2916 |
| 1 | failure — scan failure, enumeration failure (FB-K01), unknown option/verb | :3922, :3952, :797, :855 |
| 2 | recover: partial (item errors/hash mismatch); apply/fix: open/plan/execute/undo failure; image: FAILED (partial kept); bench: open/read failure or zero hits; unlock: refusal | :2927, :3073, :3239, :3263, :3306 |
| 3 | scan cancelled (session saved when --out given); image CANCELLED (partial kept); health: every source unavailable; fix: chkdsk errors-remain | :2065 ("cancelled is distinct from failure (0/1/3)"), :2922 |
| 4 | **the ELEVATION-refusal path** — `require_privilege()` (:526-537) runs BEFORE any other output in every raw-disk-requiring verb: non-elevated token → stderr `ERR_ELEVATION_REQUIRED (0x500C0001)` + the relaunch/elevated-prompt guidance; image targets and file-only verbs skip the gate (`argv_has_image_target` :542-547) | :536; :3905-3908 (usage block's privilege prerequisite); CAPABILITY-MATRIX §0 :18 |
| 5 | scan only: target device disconnected mid-scan (N-125) — distinct from 1/3; with --out the session checkpoint resumes after reattach | :2072-2082; API.md:81 |

Privilege prerequisite (usage tail :3905-3908 + PARAMETERS.md:71-74): scan/recover/tree --disk/fix/apply/bench/image with DISK targets refuse exit 4 unless elevated; `--image` scan targets are file-backed and need no elevation. Whole-disk scan semantics (S-051, API.md:87-96): `scan --disk N` always builds targets from the partition table (never a silent empty tree; unreadable FS → skipped and counted).

---

## 3. contracts/ surface — the boundary-truth directory

`contracts/README.md` is the ownership model (:9-16): the engine ABI, the error numeric space, and the UI seam are owned by their trees; **how the two map to each other** is owned here. Files:

- **`command-surface.json`** (version 4): every Tauri command the bridge exposes — **54 commands** + **12 events** + **4 removed** entries (my count; `rg '"name"'` + python len check). Threading classes: `sync` / `spawn-join` / `async-job` (:4). Each row carries name, threading, abiRefs (the cr_ui functions it calls), serves (the connector methods), args/returns, notes. Removed rows (:887-904): `system_driver_status` (driver gone, P3), `scan_close` UI surface (dropped from the fresh contract; the command stays as internal registry hygiene), `cr_ui_health_read_ex2 / CrUiSmartAttrEx` ("not on ABI v4" — STALE, see §9), `cr_ui_set_log_sink` flat-C (bridge binds the MSVC-mangled C++ seam, cr_ui_exports.def:19).
- **`error-codes.json`** (version 1): the complete engine↔UI error mapping — **40 engine `cr::Err` mapping rows** + **18 bridge-only codes** (my counts; python len check; `uiCodesWithNoEngineSource` is empty). Key semantics: ERR_CANCELLED is NEVER an error to the UI (translated to status 'cancelled', :47-54); ERR_DEVICE_DISCONNECTED = repurposed v4 value for device-gone + reattach-then-resume offer (:65-73); ERR_ELEVATION_REQUIRED → `elevation-required` with the banner/relaunch flow (:329-337); context refinements recorded in-row (e.g. device-gone, session-not-found, scan-scope-invalid vs destination-invalid). Invariants (:418-423): every cr::Err has exactly one mapping row; ERR_CANCELLED never reaches the UI as an error; every ConnectorErrorCode is mapped or bridge-only; the bridge passes the engine code numerically in `ConnectorError.detail.code`, the UI renders reasonKey strings only.
- **`event-mapping.md`**: the CR_EVT_* (1-12) ↔ connector progress-event map (:30-43) + the non-negotiable marshaling rules (:10-27: sink on worker thread → try_send into an mpsc channel → single emitter thread per scan → Tauri emit; coalesce superseded progress, never stage transitions/error-range deltas; unknown types ignored; heartbeat NEVER synthesizes progress) + recovery-progress composition (bytes-level callback + report/stats drain — OQ-1 resolved, :45-73) + consent events (bridge::consent, G-1 (a), :75-109) + imaging progress (:111-133, single-writer rule, NO cancel).
- **`type-maps.md`**: field-level maps for every struct conversion the bridge performs — 13 sections: §1 devices/volumes, §2 scan progress, §3 scan state, §4 tree/results, §5 query, §6 recovery, §7 health, §8 preview, §9 sessions, §10 system, §11 imaging, §12 diagnostics, §13 maintenance.
- **`validate.mjs`** — the CI gate (node, contracts-ci.yml:40): **phase 1** (always): (1) every `cr::Err` value in cr_error.h has a mapping row; (2) every uiCode (mapped or bridge-only) exists in `ConnectorErrorCode` AND every vocabulary member is accounted for; (3) every `CR_EVT_*` define 1..N in cr_ui.h is mapped in event-mapping.md (both header define forms parsed); (4) command-surface well-formedness (unique names, name/threading/serves, threading-class vocabulary). **phase 2** (auto-activated by `ui/src-tauri/src/bridge/abi.rs` existing): (5) every manifest command has a Rust `#[tauri::command]` implementation and every Rust command is in the manifest; (6) every command invoked from `ui/src/connector/` is in the manifest (and vice versa where it serves a connector method). Exit 0 green / 1 holes with file:line evidence (validate.mjs:1-23,39-131).
- **Cross-tree constants defined ONCE** (README.md:43-61): sessions dir `%ProgramData%\HeicFast\Recovery Wizard\sessions\`, logs dir `%ProgramData%\HeicFast\Recovery Wizard\logs\`, product version = root `VERSION` file (the only version constant; build.ps1 fails on drift), required `cr_ui` ABI version **4** (the bridge refuses to bind on any other value — bridge abi.rs startup gate). Portable-mode override (README.md:52-61): the `recovery-wizard.portable` marker beside the exe switches sessions/logs to `.\sessions`/`.\logs` (deterministic marker probe, never a writability probe).

---

## 4. Connector contract — `ui/src/connector/contract.ts` (1,223 lines)

The single source of truth for the UI seam. **REAL count at HEAD: 12 domains, 67 methods (55 request/response + 12 `on*` subscription methods)** — my count (`rg` over the interface method declarations = 67; 12 subscription methods). The documented figure "9 domains, 36 methods" (contracts/README.md:15) is the PRE-lane-C claim, stale since a536e131 (lane C added the imaging/maintenance domains + the diagnostics methods into scan/results/system).

ConnectorErrorCode vocabulary: **27 codes** (contract.ts:56-83; my regex count). Events (subscription surface): `onScanProgress`, `onScanFinished`, `onSaveProgress`, `onLostCandidate`, `onScanAutosaved`, `onConsentRequest` (Scan), `onSessionLoadProgress` (Sessions), `onRecoveryProgress`, `onRecoveryFinished` (Recovery), `onApplyConsent` (LostPartition), `onImageProgress`, `onImageFinished` (Imaging) — each returns an unsubscribe function.

| Domain (methods) | Operations |
|---|---|
| **Devices** (4) | `listDevices`, `getDeviceInfo(deviceId)` (device or volume id — resolves volume→owner), `refreshDevices`, `unlockVolume(volumeId, passwords)` (N-064 explicit-consent dictionary unlock) |
| **Scan** (17: 11 RR + 6 sub) | `startScan(params)` (targetId, optional pathScope G-11 view-scoping, optional bitlockerFvek M-033), `pauseScan` (transient 'pausing' + scan-not-running refusal), `resumeScan`, `cancelScan`, `getScanState` (params/target echo, progress, failure, pendingConsent re-hydration G-1(a), lostCandidates NG-11, autosaved W1c), `respondToConsent(requestId, allow)`, `getScanTelemetry` (lane C live stage axes), `getScanAssistantStats` (lane C S-023 grading), `listCarveFormats` (lane C static registry), `getCarveTally(scanId)` (lane C per-format counts + user-sign aggregate), `getDiskMap(scanId, options?)` (lane C sector-map producer; cell budget default 4096 cap 65536); subs: `onScanProgress` (the payload carries phase/phaseProgress D-P6, currentSector/currentSectorTotal P-TICK-01, dataFoundBytes D4b), `onScanFinished` (W-01 terminal), `onSaveProgress` (W-01), `onLostCandidate`, `onScanAutosaved`, `onConsentRequest` |
| **Results** (5) | `getResultTree(scanId, parentId, mode: path\|type\|raw)` (ONE filter implementation over the NodeMeta cache, G-7; lost-paths/raw synthetic groups), `queryResults(scanId, filter, sort, page)`, `getNodeDetail` (physical path only for structured rows on lettered volumes; confidenceReason from real evidence), `countResults`, `regradeNodes(scanId, nodeIds)` (lane C S-023 per-selection re-grade — session trees only) |
| **Preview** (2) | `requestPreview(scanId, nodeId)` → the bounded payload union (image/hex/text/pdf/av/office/archive — M-116 archive inventory with truncated flag; damage truth → preview-damaged), `getThumbnail(scanId, nodeId)` (M-102 server-side; loaded sessions via engine thumbnail_generate, live scans assemble from preview reads — BF-13 documented degradation) |
| **Sessions** (8: 7 RR + 1 sub) | `saveSession(scanId, path?)` (auto-name in the sessions store; refuses while running), `cancelSave` (N-060 save-scoped), `listSessions` (W1a light rows, newest-first; pre-v5 savedAt null), `loadSession(path)`, `getSessionMeta(path)` (light-row lookup), `resumeLastSession` (M-154 pointed resume; session-not-found → caller falls back to list), `resumeSession(path)` (load + resume + identity verify + run; session-corrupt on mismatch); sub: `onSessionLoadProgress(path)` |
| **Recovery** (7: 5 RR + 2 sub) | `startRecovery(scanId, selection: nodes\|all+filter, destination, options)` (config map: collision→conflict, verifyOnWrite→skip_hash_verify, rebuildFolders→skip_dir_reconstruct; keepEmptyFolders/fileRepair have NO engine equivalent — BF-14 truthful-disable), `cancelRecovery`, `getRecoveryState`, `getRecoveryItems(jobId, page)`, `getRecoveryReport` (per-list cap 500 + truncatedItemLists; processedTotal exact); subs: `onRecoveryProgress`, `onRecoveryFinished` |
| **Health** (1) | `getHealth(deviceId)` → DiskHealth (status 4-state 1:1 from the engine grade D-P4; SmartAttribute with threshold via ex2, per-row status incl. 'caution'; transport provenance; powerCycles; tableEmpty = unknown-never-good) |
| **Repair** (4) | `dryRunRepair`, `applyRepair`, `getUndoLog`, `undoRepairs` — **the G-1(b) typed not-implemented cluster** (the ONLY methods allowed to throw `not-implemented`; standalone repair is the scan's pre-scan hook + the CLI's `rrecov fix`) |
| **LostPartition** (6: 5 RR + 1 sub) | `applyLostPartition(deviceId, candidate, rebuildTable?)` (consent-gated; transcribes a scan-state candidate — never invents geometry), `undoLastApply`, `saveApplyUndo(deviceId, path)` (loadable by rrecov fix --undo-apply), `respondToApplyConsent(requestId, allow)`, `pendingApplyConsent(deviceId)` (re-hydration); sub: `onApplyConsent` |
| **System** (7) | `getAppInfo` (engineVersion = 'cr_ui ABI v<N>' — G-6 contract number; userSigns M-048 count), `loadUserSigns(content)` (atomic replace from webview file text), `validateDestination(path, sourceDeviceId, neededBytes)` (overlap/invalid/out-of-space/unwritable — real write probe; engine backstops 0x50050001/4), `pickDirectory(hint?)` (native dialog; hint roots/avoids a device), `listMountPoints`, `openPath(path)` (folders open, files revealed), `getDeviceTransport(deviceId)` (lane C: "direct"\|"image" + advisory last-transport-error) |
| **Imaging** (4: 2 RR + 2 sub) | `startImageCreation(deviceId, path)` (byte-for-byte whole physical disk; bridge pre-gates same-disk destination → destination-overlap, free-space < total+64 MiB headroom → out-of-space; NO cancel — cancelSupported always false, D-IMG-1), `getImageState(imageId)` (live counters + terminal latch + partialKept filesystem-truth probe); subs: `onImageProgress`, `onImageFinished` |
| **Maintenance** (2) | `runFsck(letter, fix?)` (M-075 escalation rung; raw exit code + verdict map incl. 259 STILL_ACTIVE at the 15-min bound; CREATE_NO_WINDOW — no console on the desktop), `turnOffBitlocker(letter)` (M-078 destructive decrypt twin; accepted≠plaintext) |

---

## 5. Tauri command inventory + the bridge consumption model

**54 `#[tauri::command]` functions across 13 modules** — my count from `rg -U '#\[tauri::command\]'` over `ui/src-tauri/src/commands/*.rs` (3+7+1+2+4+2+2+5+5+4+7+6+6 = 54), matching `contracts/command-surface.json`'s 54 rows 1:1 (validate.mjs check 5 enforces both directions).

| Module (file) | Commands |
|---|---|
| `commands/devices.rs` (3) | `devices_list`, `devices_refresh`, `volume_unlock_dictionary` |
| `commands/scan.rs` (7) | `scan_start`, `scan_pause`, `scan_resume`, `scan_cancel`, `scan_state`, `scan_close` (internal registry hygiene; the fresh contract dropped the UI surface), `scan_save_cancel` |
| `commands/results.rs` (4) | `results_tree`, `results_query`, `results_count`, `node_detail` |
| `commands/preview.rs` (2) | `preview_open`, `preview_thumbnail` |
| `commands/sessions.rs` (6) | `session_save`, `sessions_list`, `session_load`, `session_meta`, `session_resume_last`, `session_resume` |
| `commands/recovery.rs` (5) | `recovery_start`, `recovery_cancel`, `recovery_state`, `recovery_items`, `recovery_report` |
| `commands/health.rs` (1) | `health_read` (calls `cr_ui_health_read_ex2` first, base-export fallback: health.rs:38,71) |
| `commands/repair.rs` (5) | `consent_respond`, `repair_dry_run`, `repair_apply`, `repair_undo_log`, `repair_undo` (the four gap methods = typed not-implemented) |
| `commands/lost_partition.rs` (4) | `lost_partition_apply`, `lost_partition_undo`, `lost_partition_save_undo`, `apply_consent_pending` |
| `commands/system.rs` (6) | `system_app_info`, `system_load_user_signs`, `system_validate_destination`, `system_pick_directory`, `system_list_mounts`, `system_open_path` |
| `commands/imaging.rs` (2) | `image_create_start`, `image_create_state` |
| `commands/diagnostics.rs` (7) | `scan_telemetry`, `scan_assistant_stats`, `carve_catalog`, `scan_carve_tally`, `disk_map`, `device_transport`, `session_regrade` |
| `commands/maintenance.rs` (2) | `volume_fsck`, `volume_bitlocker_off` |

**The bridge (`ui/src-tauri/src/bridge/`) — how the ABI is consumed** (module map, bridge/mod.rs:1-27):
- `abi.rs` (1,035 lines): the FFI surface **pinned 1:1 from cr_ui.h** — all 93 externs (unconsumed declarations are EXPECTED: the capability map's DISABLE rows stay declared so the file is the complete, diffable binding; scoped allow(dead_code) at :45-54), every struct `#[repr(C)]` mirroring the header's declaration order (natural alignment reproduces the MSVC layout — no manual padding ever). ABI gate at startup: version must equal 4 (contracts README's required-version row).
- `state.rs`: the registries (live scans / recovery jobs / imaging jobs / loaded sessions), the snapshot metadata cache (NodeMeta), lifecycle discipline (the Drop-enforced close order: event unsub → sink box → snapshot cache → scan → device, command-surface.json:146).
- `events.rs`: worker-thread sinks → bounded mpsc channel → single emitter thread per handle → `app_handle.emit` (the event-mapping.md marshaling rules; the single-writer rule for terminal events).
- `consent.rs`: the S-008 pre-scan repair consent gate — mints requestId, registers the pending ConsentGate, emits `consent-request:<scanId>` with the registry lock RELEASED, then the run thread BLOCKS on the gate with NO user-facing timeout (plan §4.5: a non-answer is a user decision); cancel force-denies FIRST (the deadlock rule); the run tail closes survivors and persists the undo log after an approved repair.
- `error.rs`: `BridgeError` + the UI vocabulary bridge-side codes (the error-codes.json map).
- `util.rs`: two-call helpers, time/base64 conversion, owned strings, portable-mode probe.
- `filefilter.rs` (1,538 lines): GENERATED extension→category mirror of the engine table (ui/scripts/gen-filefilter.mjs --check gates it, contracts-ci.yml:43).
- `logging.rs`: the operational log — bridge lifecycle + the engine's diagnostic seam (NG-15) into `ProgramData\...\logs`.

Threading classes per the manifest: `sync` (registry-only / any-thread engine ops), `spawn-join` (the engine call is synchronous-on-caller; the command spawns a dedicated thread), `async-job` (long-running thread; completion via events + state queries). Event set emitted: the 12 manifest events (scan-progress/scan-finished/save-progress/lost-candidate/consent-request/apply-consent-request/recovery-progress/recovery-finished/session-load-progress/scan-autosaved/image-progress/image-finished, each `<scoped-id>`).

---

## 6. Test coverage map — per domain, who pins it

**Engine side (`engine/tests/`, GoogleTest):** `cr_tests` = 70 unit sources (tests/CMakeLists.txt:5-76, portable core, runs Linux AND Windows); 5 Windows-only executables (destio :110, transport_win :124, bitlocker :131, fsck_win :138, image_create :147); `cr_ui_tests` (test_cr_ui.cpp, 91 TESTs — the flat-C surface suite, :176-192); `cr_ui_abi_check` (plain-C consumer: header compiles under a plain C compiler AND drives the full lifecycle scan→session→snapshot→query→preview→recover→report over a synthetic FAT16 image, :164-169,194-212); `cr_ui_abi_layout_dump` (the generated layout-manifest dumper, ctest smoke :213-214). Lint gates: `win_syntax_check` (Linux parse gate over the win layer, :245-249), `test_name_uniqueness` (:258-260).

| Domain | Engine tests (one-line roles) | Where the ABI is pinned |
|---|---|---|
| Devices/enumeration | `test_enumerate_win.cpp` (win-layer census contracts), `test_enumerate_merge.cpp` (cfgmgr32+PhysicalDrive merge), `test_transport_win.cpp` (win transport seam, win-only exe), `test_transport_logic.cpp` (portable transport logic), `test_privilege.cpp` (the elevation gate), `test_slice_devmap.cpp` / `test_directalign.cpp` / `test_cached.cpp` / `test_retry.cpp` (slice/devmap, direct-I/O alignment, 8 MiB LRU cache, retry+bisection) | `test_cr_ui.cpp` (CrUi.* suites), `ui_abi_check.c` |
| Scan pipeline | `test_orchestrator.cpp` (72 TESTs — staged pipeline, weights, pause/cancel/checkpoint, M-049 attribution), `test_scan_truth.cpp` (read-truth/fake-scan refusal, 5 pins incl. fail-before), `test_telemetry.cpp` (P-DEEP-01/P-TICK-01 axes), `test_pause_resume.cpp` (token+checkpoint resume), `test_parallel.cpp`, `test_node_tree.cpp` (virtual roots/attributes model), `test_m101_branches.cpp` | `test_cr_ui.cpp` + it-gate `scan_progress_axes_move` (real-Windows axes) |
| Filesystems | `test_ntfs_volume.cpp`, `test_fat_volume.cpp`, `test_exfat_volume.cpp`, `test_ext4_volume.cpp` (+`_m144_large_volume`, `_m143_journal_inode_image`), `test_hfsplus_volume.cpp`, `test_btrfs_volume.cpp`, `test_zfs_volume.cpp`, `test_refs_volume.cpp`, `test_refs_ldm.cpp`, `test_ldm.cpp`, `test_ldm_recovery.cpp`, `test_vollabel.cpp` (per-family engine suites; CAPABILITY-MATRIX §1 cites the per-row test names) | via cr_ui paths in `test_cr_ui.cpp` |
| Carve | `test_carve.cpp` (grammar validators + raw-sign table + user signs), `test_dict.cpp` (the 228-entry dictionary), fixtures data/thumbnails+data/images | `cr_ui_carve_*` in `test_cr_ui.cpp` |
| Lost-partition | `test_lostpart.cpp` (discovery/scoring/S-019 BitLocker candidates) | `CrUi.LostApply*` in test_cr_ui.cpp |
| Apply/boot repair | `test_apply.cpp` (13 tests: refusals/race/rollback/GPT signature), `test_bcdfix.cpp`, `test_nvramfix.cpp` (M-040 stages a+b) | `cr_ui_apply_*` |
| Repair/check | `test_rawfixer.cpp` (analyze/dry-run/transactional apply/rollback), `test_fscheck.cpp` (15 tests, M-041 taxonomy, never-aborts), `test_fscheck_launch.cpp` + `test_fsck_win.cpp` (M-075 chkdsk launch), `test_no_console_spawn.cpp` (the no-_popen build-time guard) | `cr_ui_fsck_volume` |
| Sessions | `test_session.cpp` (CRSF round-trip/F10 salvage), `test_resume_revalidation.cpp` (identity/boot-hash), `test_reghive.cpp`/`test_reghive_edit.cpp`/`test_mountpoints2.cpp` (offline hive + mountpoint history for resume re-attach) | `cr_ui_session_*` + it-gate `results_filters_and_deleted_tree` |
| Query/results | `test_query.cpp`, `test_query_perf.cpp`, `test_config_display.cpp` (display formatters), `test_grading.cpp` (S-023 grading) | `cr_ui_query_run` |
| Preview/thumbnail | `test_preview.cpp` (budgeted reads, M-028/M-077 caps), `test_thumbnail.cpp` (the format matrix + M-115 gates) | it-gate `preview_surfaces` (real JPEG + genuine 43-entry zip) |
| Recovery | `test_writer_dirs.cpp`, `test_writer_readpipe.cpp` (safe writer, mkdir/collisions), `test_report.cpp` (JSON report, schema CI-validated by tools/validate_report.py), `test_destio_win.cpp` (win DestIo) | `cr_ui_recovery_*` + `ui_abi_check.c` |
| Health/SMART | `test_health_grade.cpp` (S-024 8 rule tests), `test_health_csmi.cpp`, `test_sat_stamps.cpp` (byte-exact SAT stamps, D-P4a), `test_nvme_health.cpp` | it-gate `health_read_per_disk_is_coherent` (canary-bracketed stack-smash guard) |
| Imaging | `test_image.cpp` (reader: flat/VMDK/VHD/VHDX), `test_image_create_win.cpp` (S-040 disposition: atomic rename, .sha256 sidecar, .partial) | it-gate lane (`cr_ui_image_create` covered on the seeded lane) |
| Crypto/BitLocker | `test_sha256.cpp`, `test_bitlocker_xts.cpp` (in-stack XTS), `test_bitlocker_win.cpp` (win seam), `test_fve_policy.cpp` | `cr_ui_open_volume_with_key` |
| Diagnostics | `test_telemetry.cpp`, `test_text.cpp`, `test_fixup.cpp`, `test_crc32_gpt.cpp`, `test_consistency.cpp`, `test_folderfilter.cpp` (M-073) | `cr_ui_disk_map` etc. via test_cr_ui.cpp |

**The it-crate (`ui/src-tauri/it/`, the real-bridge integration lane — BF-ABI-02 closure, bridge_integration.rs:1-27):** 5 runtime tests + 1 layout gate, all writing evidence BEFORE asserting: `enumerate_rows_coherent` (:60 — the disks-missing/row-stride reproduction), `health_read_per_disk_is_coherent` (:137 — the 16-byte stack-smash canary + per-disk outcomes), `scan_progress_axes_move` (:233 — real scan over the SEEDED disk; axes a-d: overall advances, found files+bytes grow, deep-stage bytes_scanned advances, no >15 s freeze; cancelled at window end), `results_filters_and_deleted_tree` (:513 — category/size/manifest filters + the 81/81 deleted-tree reconstruction), `preview_surfaces` (:662 — image metadata kind + the genuine zip's 43-entry inventory). Environment from `.github/workflows/bridge-integration.yml` (windows-2022, seeded 6 GiB NTFS VHDX + 512 MiB raw VHDX; CR_IT_EVIDENCE/MIN_DISKS/SCAN_DISK/SCAN_SECS/EXPECT_FILES/ABI_LAYOUTS). The layout gate `layouts_match_engine_manifest` (abi_layouts.rs:469, feature `layout-gate`) — 29 structs / 366 field offsets — is where the ABI is LAYOUT-pinned (§1.20). Evidence of the all-green run: `worklog/VALIDATION/it-gates/` (run 44244567592 @ be3b8256; scan-timeline.csv pins the D-P6 terminal numbers: overall_pm=750, phase=1, phase_permille=1000; deleted-tree.txt pins 81/81; health.json pins the truthful refusals; abi-layouts.json = the manifest dump).

**UI side:** vitest suite under `ui/tests/` (28 test files — connector-mock, connector-tauri, event-surface, scan-session-store, results-tree-label, consent-surfaces, etc.) + Playwright e2e (`ui/e2e/`: smoke, consent, responsive, lane-c-surfaces). The mock connector (`ui/src/connector/mock/`) is the dev double; the real one is `ui/src/connector/tauri/tauri-connector.ts`.

---

## 7. The documented-truth gap list — every deferred/not-implemented item the repo records

**A. PARITY-DELTAS rows named by the mission (docs/PARITY-DELTAS.md):**
- **D-IMG-1 (image-create CANCEL)** :68 — the og's DiskBackProgressPage has stop-confirm; ours: NO cancel exists (`cancelSupported` always false, dialog copy states the run completes) — the ABI exports no cancel (`cr_ui_image_create` runs its win-layer CancelToken as nullptr). "Engine scope for next session: a handle-ized `cr_ui_image_cancel`."
- **D-IMG-2 (image format)** :69 — the og writes `.edisk` (proprietary container); ours: flat `.img` + `.sha256` sidecar. "Engine-side scope (a .edisk writer); D-147's register stands."
- **D-IMG-3 (image sector range)** :70 — the og's "More Option" custom sector range; ours: whole-disk byte-for-byte only ("the engine export takes no range; same engine-scope family as D-IMG-1").
- **D-P4c (vendor bridge dialects)** :56 — DEFERRED: Si "SCSIDISK" miniport SRB, ASM1352R 0xE3/0xE0, the 23-branch AddDisk-USB VID/PID ladder, 7 vendor NVMe routes; generic SAT + STORAGE_QUERY NVMe cover standards-compliant bridges; payloads only partially decoded in the corpus (VERIFICATION V9). Precise scope (BEHAVIOR-FINDINGS :77-80): decode FUN_00419280 cases 5-10 + the NVMe vendor routes from the decompile, then implement per-bridge; no fabricated dialects meanwhile.
- **D-P8 (rendered video/document previews)** :60 — DEFERRED: ffmpeg-class (~99 MiB) + pdfium-class renderers not bundled (supply-chain + installer-size: current total 8.2 MiB); the truthful "preview not rendered" hint with REAL metadata ships. Precise scope (:60 + BEHAVIOR-FINDINGS :81-83): pick the renderer set, decide provenance (vcpkg vs vendored), wire bounded invisible workers with size/time caps, extend the preview pane's render surface. No stub ships meanwhile.
- **G-1(b)** (docs/INTEGRATION-GAPS.md:39) — the standalone repair-workbench ABI trio (dryRun/undoLog/undo) stays OPEN as an ABI gap: the typed `not-implemented` refusals are real and shipped (the four Repair-domain methods); undo is CLI-only today (`rrecov fix --undo-apply`; the bridge saves the undo log after an approved pre-scan repair). Also BEHAVIOR-FINDINGS :84 ("G-1(b) — the file-repair workbench (EUFileRepair, 45.6 MiB): a separate product surface; deferred").

**B. INTEGRATION-GAPS (docs/INTEGRATION-GAPS.md) — the prior-gap register still open (Part 1, :35-50):** G-2 `currentPath` ticker always null (engine has no per-item event; BF-29 re-registers it as engine+bridge scope), G-3 `dataFoundBytes` live counter absent (exact at completion — note: D4b landed the live `bytes_found` axis on the ABI/events; the row records the contract-side live-counter history), G-4 rendered previews null with truthful hints (= D-P8), G-5 `keepEmptyFolders`/`fileRepair` truthfully disabled (BF-14), G-6 engine version string null in About (ABI version is a contract number, G-6), G-7 bridge-side results query for live scans and sessions (G-7 migration recorded in PARITY-DELTAS D-B3 — one path, identical semantics), G-8 device-disconnect synthesis from heartbeat (BF-07: the dedicated reattach-then-resume affordance is the open UX gap), G-9 device bus kinds from the removable flag (CLOSED in code by D-ENUM-02 `bus_type` at cr_ui.h:435 + the bridge map type-maps.md §1 — the row predates the 0.9.3 re-pin), G-10 pre-scan lost-partition hints absent (superseded by scan_state lostCandidates + lost-candidate events), G-11 folder-scoped scans = bridge-side view scoping (documented semantics delta). The NG register (Part 2, :53-237) is the 2026-10-27 forensic audit; most rows closed in-session (NG-18/21/22 explicitly CLOSED :188-237); NG-20 (~20 ABI exports uncalled) was superseded by lane C (BF-08/09/10/11/17 "Surfaced").

**C. REFERENCE-API-GAP (engine/docs/REFERENCE-API-GAP.md) — the corpus-operation gap check:** summary (:344-347) "PRESENT 45 · PARTIAL 10 · GAP 27 (recounted 2026-10-29; ~14 of the GAPs are product-layer/licensing/monetization or media-render scope)". The standing GAP rows with scope (checked vs HEAD where cheap): R-02 pre-scan license/version gating (product layer), R-16 disconnect retry states (now partially served by ERR_DEVICE_DISCONNECTED + reattach-resume offer; BF-07 keeps the affordance open), R-29 AI recommend/dedup (product feature), R-38/R-79 media helper IPC (out of core scope — pairs with D-P8), R-39 external viewer processes (host concern), R-45/R-81 free-quota/upgrade (monetization), R-46 repair-during-recovery (= the Fixo line), R-47 auto-recover plan, R-48 ACL preservation on copy (recovered files lose NTFS DACLs — `SafeWriter`/`WinDestIo` write plain files, :254), R-49 cloud destination, R-50 recovery telemetry, R-57 config cache, R-59 crash-report/watchdog, R-68 EPM editing vocabulary (out of recovery scope), R-71 RAID-5 rebuild, R-72..78 the Fixo file-content repair chain (registered as a SEPARATE product line, S-050 disposition — CAPABILITY-MATRIX §3 :79-111, PARITY-DELTAS D-137 amendment set), R-80 activation, R-82 update/version check, R-87 hot-plug notify (pull-only enumeration), R-89/90/91 NAS/Linux/cloud connect, R-92 WinPE media creation, R-94 WeChat recovery. Closed since the census: R-14 scan-time filter (M-073 landed — cr_ui_scan_config.filter_folders + CLI --filter-folder), R-93 disk-image creation (S-040 PRESENT :338), R-65 disk check (M-041 + M-075 PRESENT :286), R-83 service lifecycle (REGISTERED-GAP by decision M-105/D-166), R-86 disk-metadata refresh (C++-only win::refresh_disk_properties, documented at R-86; the pull-model equivalence is N-098 in cr_ui.h:458-466).

**D. Other standing registers:** D-147 (VHD/VHDX writers out of scope — API.md:45, D-IMG-2), D-MAP-1 (the og's AnalysisGridWidget paint rules never corpus-decoded — our four-overlay form is honest to the ENGINE contract, PARITY-DELTAS :71), XPRESS/LZX NTFS compression (SHARED limitation — the og is LZNT1-only too; CAPABILITY-MATRIX :33), ZFS multi-vdev topology (PARTIAL, D-09/D-L, :47), the file-repair product line (S-050/D-137), BF-register open rows in `worklog/BEHAVIOR-FINDINGS.md`: BF-07 (reattach-resume affordance), BF-12 (image targets unsurfaced in the UI — also the ABI-check surface), BF-16 (core-side query engine unused — the NodeMeta port carries the UI), BF-21 (M-101 version_group/tags fields unsurfaced), BF-25/BF-30 (carve matcher drain latency + the 8.45 MiB/s throughput wall — upstream core scope), BF-28 (health path returns ERR_OPEN_FAILED rather than ERR_ELEVATION_REQUIRED unelevated — upstream uniformity), BF-31 (SD kind never fires — needs the upstream bus axis; NOTE: the 0.9.3 re-pin ba2b9347 landed bus_type + the bus-driven kind split, closing BF-02/BF-31's precondition), BF-32 (the scan-emitter teardown race — design sketch recorded for the next bridge session), plus the smart-scan session's deferred register (:71-93): physical-disk-class SMART validation (bare-metal host needed), D-P4c, D-P8, G-1(b), BF-30, BF-32, and the deep-in-flight-observability non-defect record.

---

## 8. CI state as documented at HEAD

**Workflow files that exist (product root `.github/workflows/`):** `bridge-integration.yml` (windows-2022; builds engine + links the real bridge + runs the it-crate with the seeded VHDX fixture + the layout gate; uploads cr-it-evidence), `contracts-ci.yml` (ubuntu-24.04; validate.mjs phase 1+2 + gen-filefilter check), `engine-ci.yml` (windows-2022 ×2 jobs; includes `python tools/doc_truth_gate.py` at :248 — the doc-count invariants incl. the 93-export claim), `forensic-repro.yml` (windows-2022; the unelevated/elevation forensic probes; default tag fixed v0.8.4→v0.9.3 at 1b9a4408), `release.yml` (windows-2022; validation-gated, the sole source of builds), `ui-ci.yml` (windows-latest), `ui-e2e.yml` (windows-latest; Playwright), `validation-evidence.yml` (windows-2022 ×2). (Engine repo's own `.github` exists but carries no workflow files — the engine CI runs from the product root.)

**Green at the v0.9.3 frontier, as the worklog records:** the T20 handoff (worklog.md:3169-3188) — "CI STATE AT CUTOFF: GREEN — ui-ci + ui-e2e + contracts on the code frontier (f84809bc/948164bc); engine-ci GREEN at 385d206e, the WMI import gate's final form runs on 93389402; bridge-integration: the axes/health gates PASS on every recent run; the two new gates mid first-execution" (the query-gate panic in run 44102634559 was closed by be3b8256 — see T1, worklog.md:3208-3214: "this session starts from the green v0.9.3 frontier", bridge run 44244567592 @ be3b8256 the first all-green run, evidence in `worklog/VALIDATION/it-gates/README.md`). v0.9.3 tagged (948164bc bump; the tag waits were over at the frontier). Static census only — no runs were executed for this sheet.

---

## 9. Doc-drift notes found during the census (recorded, not fixed — read-only session)

1. `contracts/README.md:13` says **92** exports — the header has **93** (verified twice; the 93rd is `cr_ui_health_read_ex2`, landed eb4bb5f3/BF-03; engine docs + doc_truth_gate were trued to 93 at ba2b9347 but the contracts README was not).
2. `contracts/README.md:15` says "9 domains, 36 methods" — `contract.ts` at HEAD is **12 domains / 67 methods** (lane C's a536e131 added imaging + maintenance + the diagnostics methods).
3. `engine/docs/ABI-CONTRACT.md §9 :773` still opens "`CR_UI_ABI_VERSION` is `3`" — the header is **4** (cr_ui.h:81; API.md §2 :108 says 4; the v3→v4 bump removed `cr_ui_driver_status`). The rest of §9's policy text is current.
4. `contracts/command-surface.json:434` (health_read note "BASE ABI only (no ex2 on v4…)") and the removed-row `cr_ui_health_read_ex2 / CrUiSmartAttrEx` (:897-899, "not on ABI v4") are **stale vs. code**: the header declares ex2 (cr_ui.h:1635), abi.rs binds it (:1000), and health.rs calls it FIRST with base fallback (:38,71) — the 0.9.3 re-pin (ba2b9347) landed after the manifest's last edit in this area. Same vintage for the devices_list note "BF-02: no bus_type on v4" (:22) — bus_type IS on v4 now (cr_ui.h:435; type-maps §1 :17-18 documents the live split).
5. `ui/src-tauri/src/bridge/abi.rs:46` has one stale inline phrase "the whole 92-export surface" in the allow-block comment — the same file's header (:4) says 93; cosmetic only.
6. `worklog/VALIDATION/it-gates/README.md` says "365 field paths" — the generated abi_layouts.rs currently pins **366** `FieldOff` entries (29 structs).
7. `contracts/error-codes.json:333` (ERR_ELEVATION_REQUIRED context) still carries the interim "INTERIM mapping to device-permission until the P4 bridge introduces the dedicated 'elevation-required' code" note, while the row's own uiCode and contract.ts:59 already carry `elevation-required` — the note describes superseded history.

None of these affect runtime behavior; they are exactly the class the integration matrix's "documented truth vs. code truth" column should carry.

---

### Count summary (the real numbers, all counted at HEAD)

| Surface | Count | Method |
|---|---|---|
| cr_ui ABI exports | **93** | `^CR_UI_API` declarations in cr_ui.h; abi.rs extern census |
| ABI version | 4 | cr_ui.h:81 |
| CR_EVT_* event types | 12 | cr_ui.h:151-223 |
| cr_error.h ERR_ members | 40 (+OK) | rg census; error-codes.json mapping rows = 40 |
| CR_ERRMOD / CR_ERROP | 13 / 2 | cr_ui.h:292-307 |
| rrecov CLI verbs / exit codes | 13 / 6 (0-5) | rrecov.cpp:3911-3952 + exit-code table |
| contracts: commands / events / removed | 54 / 12 / 4 | command-surface.json len() |
| contracts: engine error mappings / bridge-only | 40 / 18 | error-codes.json len() |
| Connector: domains / methods / subscriptions | 12 / 67 / 12 | contract.ts interface census |
| ConnectorErrorCode members | 27 | contract.ts:56-83 |
| Tauri commands (Rust) | 54 in 13 modules | #[tauri::command] census |
| Engine test sources | 76 unit .cpp (70 in cr_tests + 5 win-only + cr_ui_tests) + 2 plain-C consumers | tests/CMakeLists.txt |
| it-crate tests | 5 runtime + 1 layout gate (29 structs / 366 offsets) | bridge_integration.rs / abi_layouts.rs |
| CI workflows | 8 | .github/workflows/* |
