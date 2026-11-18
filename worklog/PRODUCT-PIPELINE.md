# PRODUCT-PIPELINE — Current state of the result pipeline (Phase 0 read)

Repo: `/home/z/my-project/repos/final-prod`. All citations are `file:line` into this repo, current HEAD as read. "Absent" means the code does not exist.

---

## 1. Result-pipeline data flow (as it exists TODAY)

Chain: **sector read → read pipe → FS engine walk / carve window → detection → `tree.add_file` node → ResultTree → (filter, grade) → session/snapshot → query → cr_ui ABI → Tauri bridge → UI event**.

### 1.1 Read chain (sector → engine)

| Step | Function | Citation |
|---|---|---|
| Whole-disk/volume handle, 64 KiB envelope direct reads, handle self-heal | `DirectDevice::read_at` via `aligned_read_at` / `overlapped_read` / `reopen_handle` | engine/src/win/cr_transport.cpp:53,191,237,256 |
| Single construction point of scan read chain | `build_read_pipe` — slice → `SliceShim` → `RetryingDevice` (3 attempts, Skip-mode zero-fill) → `CachedReader` (8 MiB, 256 KiB blocks) | engine/src/core/orchestrator/cr_scan.cpp:191-230 |
| Retry/bisect + LRU cache classes | `RetryingDevice`, `CachedReader` (LRU move-to-front) | engine/src/core/storage/cr_retry.cpp; engine/src/core/storage/cr_cached.cpp:44,75 |

### 1.2 Orchestration (engine pass ordering)

`ScanController::run` (engine/src/core/orchestrator/cr_scan.cpp:1187) executes, in order:

1. Disk-truth pass: live table parse, persisted-layout compare, stale lost-candidate re-validation (cr_scan.cpp:1211-1254).
2. Pre-scan repair (S-008, consent-gated, cr_scan.cpp:1279-1336) and damage-audit check (M-041, cr_scan.cpp:1346-1360).
3. Read pipe build (cr_scan.cpp:1387) + `probe_family` (cr_scan.cpp:963-1185) with blind-probe flagging (cr_scan.cpp:1416-1441).
4. `run_quick` (cr_scan.cpp:1679) → single shared engine pass via `run_engine_pass` (cr_scan.cpp:1967) → `run_engine_dispatch` (cr_scan.cpp:1841-1963) switch over FsFamily: `ntfs::walk_mft`, `fat::scan_fat`, `exfat::scan_exfat`, `ext4::scan_ext4`, `hfsplus::scan_hfsplus`, `btrfs::scan_btrfs`, `zfs::scan_zfs`, `refs::scan_refs` (cr_scan.cpp:1849-1945). LDM dynamic volumes (cr_scan.cpp:2001-2050) and whole-disk table targets (cr_scan.cpp:2091-2196) route per-volume.
5. Carve-before-deep when quick already emitted (N-181, cr_scan.cpp:1556-1576); `run_carve_stage` (cr_scan.cpp:2607-2784) → `carve::carve_slice` (cr_scan.cpp:2762).
6. `run_deep` (cr_scan.cpp:2239-2541): NTFS remnant sweep `ntfs::scan_mft_remnants` (cr_scan.cpp:2290), lost-partition discovery `lost::discover` (cr_scan.cpp:2318), parallel candidate fan-out over `ThreadPool` + `parallel_for` (cr_scan.cpp:2473-2511), `scan_candidate` (cr_scan.cpp:2543-2605) with exFAT content-consistency rejection (cr_scan.cpp:2583-2603).
7. Post-scan: `tree_.rebase_new_extents` F-1 absolute basis (cr_scan.cpp:1532,1566,2164); `apply_folder_filter_` (cr_scan.cpp:877-903); `filterfolder::apply` junk pass (cr_scan.cpp:1609); `tree::grading::grade_tree` (cr_scan.cpp:1616-1631); read-truth completion gate refusing fully-blind scans with `ERR_READ_FAILED` (cr_scan.cpp:1648-1675).
8. Stage-boundary checkpoints `emit_checkpoint_` (cr_scan.cpp:905-961) — 'RKPP' container, atomic `.tmp`+rename commit.

Parallelism: `ThreadPool` fixed workers (auto = min(hw-1, 8)), `parallel_for`, `CancelPoolWatcher` 50 ms abort watcher (engine/src/core/cr_parallel.cpp:23,104,152). Carve window fan-out (cr_carve.cpp:1630-1683); lost candidates fan-out (cr_scan.cpp:2457-2523).

### 1.3 FS result emission (engine → node)

All engines emit directly into the shared `tree::ResultTree` under its mutex:

| Engine | Emit site | Deleted/conf state |
|---|---|---|
| NTFS | `tree.add_file(default_root, link_props)` per record, ADS children, recycle-bin fixup queue | engine/src/core/ntfs/cr_mftwalk.cpp:1078,1155,1219,1411,1928,2190,2281 |
| FAT | set_origin Structured; deleted entries marked | engine/src/core/fat/cr_fat_quick.cpp:552,575,589,793 |
| exFAT | deleted entries / valid-deleted conf | engine/src/core/exfat/cr_exfat_quick.cpp:938,955,963,966,1177 |
| ext4 | deleted-inode harvest to `root_deleted`, orphans to `root_lost` with `Origin::Lost` | engine/src/core/ext4/cr_ext4_engine.cpp:1246,1358,1442-1447,1495-1541 |

Node attach: `ResultTree::add_file` (engine/src/core/tree/cr_node.cpp:188-229) — O(1) tail append, `live_files_/live_raw_/live_logical_` atomic bumps (N-166/D4b), `index_node_locked` builds `by_record_` and `by_extent_` dedup maps (cr_node.cpp:231-251).

### 1.4 Carve emission

`carve_slice` (engine/src/core/carve/cr_carve.cpp:1494-1722) → per-window `scan_window_index` (cr_carve.cpp:1409) → `scan_one_window` (cr_carve.cpp:613): per-byte first-byte dispatch over `kTrialOrder` (cr_carve.cpp:59-151), N-070 best-accumulated-score winner, M-005 free-map gate, M-039 occupancy gate, F9 extent dedup, `tree.add_file` emission with `og_carve_name` naming (cr_carve.cpp:424) and volume-group attach `carve_attach_parent` (cr_carve.cpp:532). Raw-sign layer rides at 512-byte strides after grammar claims (cr_carve.cpp:975-1051); user-sign layer after built-ins (cr_carve.cpp:1053-1193).

### 1.5 Tree → session / snapshot → query → UI

- Snapshot: `ResultTree::capture_snapshot` (engine/src/core/tree/cr_snapshot.cpp:17-85) — full copy under tree lock, `Snapshot::dfs` (cr_snapshot.cpp:91), `materialize_props` (cr_snapshot.cpp:114).
- Session save: `ScanController::save_session` (cr_scan.cpp:2786-2989) writes 12 record types; load `load_session_summary` (cr_scan.cpp:3042-3256), resume (cr_scan.cpp:3258-3292).
- Query: `query::run` (engine/src/core/query/cr_query.cpp:228-436) — single-pass filter walk, stable sort (Name/Size/Modified/Confidence/Grade/Path/Type), offset/limit paging, `Row.path_utf8` built by `emit_path` (cr_query.cpp:396-416).
- ABI: `cr_ui_scan_run` (engine/src/ui/cr_ui.cpp:3087 → `scan_run_impl` cr_ui.cpp:2378-2495), `cr_ui_scan_snapshot` (cr_ui.cpp:3174), `cr_ui_query_run` (cr_ui.cpp:3951 → `query_run_impl` cr_ui.cpp:3848), disk map `cr_ui_disk_map` (cr_ui.cpp:3618).
- Event path: `scan_event_trampoline_` (cr_ui.cpp:1066-1109) — core POD → `cr_ui_event` field-for-field; heartbeat watchdog thread (cr_ui.cpp:1142-1196); completion auto-save `autosave_if_due_` + `CR_EVT_AUTOSAVED` (cr_ui.cpp:2370).
- Windows transport carries NO scan events — cr_transport.cpp is raw I/O only; all results cross to the UI through the cr_ui ABI + the Rust bridge (grep of cr_transport.cpp shows zero event/scan functions).

### 1.6 Tauri bridge (cr_ui → UI)

- `scan_start` (ui/src-tauri/src/commands/scan.rs:92) → `scan_start_blocking` (scan.rs:116) creates scan, subscribes `cr_ui_scan_set_event_fn` with `scan_event_sink` (ui/src-tauri/src/bridge/events.rs:50-60 — `try_send` into bounded channel; full channel = drop, emitter coalesces).
- `launch_run_thread` (scan.rs:339-460): dedicated run thread calls `cr_ui_scan_run`, latches terminal state, refreshes final snapshot, emits `scan-finished:{id}`.
- Emitter thread `spawn_scan_emitter` (events.rs:335-460) drains channel, updates `ScanLive`, maps CR_EVT_* to stage labels ("quick-index"/"deep-walk"/"carve"), monotonic-clamps overall.
- Progress payload `compose_scan_progress` (events.rs:172-231) — camelCase contract shape with badSectorMap, etaSeconds, throughput, recoverableDuringScan, currentSector; **`currentPath` is always null** (events.rs:218-220, G-2).
- Snapshot cache: `refresh_snapshot` (ui/src-tauri/src/bridge/state.rs:631-677) — generation-numbered `SnapshotCache`, min refresh interval, frozen at terminal; `ensure_metas` (state.rs:696) builds a `Vec<NodeMeta>` bridge-side metadata cache per generation.
- Result queries: `query_page` (ui/src-tauri/src/commands/results.rs:434) — **the bridge filters its own NodeMeta cache, not `cr_ui_query_run`** ("the engine's cr_ui_query_run cannot express the contract's … treeMode semantics", results.rs:11-17).

### 1.7 TS consumption

- Connector events: `scan-progress:{scanId}`, `scan-finished`, `save-progress`, `lost-candidate`, `scan-autosaved`, `consent-request`, `session-load-progress`, `recovery-progress/finished`, `image-progress/finished`, `apply-consent-request` (ui/src/connector/tauri/tauri-connector.ts:264-340,428-620).
- Progress store: one external store per scanId in `getScanStore` (ui/src/queries/scan.ts:32-90); on each event it invalidates React-Query caches for tree/results/count at **10%-bucket cadence** (scan.ts:41-47) — the result list is re-fetched via `useInfiniteQuery` `queryResults` (ui/src/queries/results.ts:42-50).
- `useScanState` polls every 500 ms while running/starting/stopping (scan.ts:106-120).

**Streaming model verdict: batched snapshot-pull, not row streaming.** The engine streams progress *counters* (permille, bytes, files-found, error ranges, lost candidates) but never streams result rows; the UI's result list is refreshed by re-capturing an immutable `tree::Snapshot` and re-running the bridge-side filter over the cached NodeMeta vector. Ordering: monotonic clamped permille (cr_scan.cpp:721-726 stale-drop, events.rs:356-360); backpressure: bounded `SyncSender` channel with `try_send` drop + coalescing (events.rs:53-60); cancellation: `PausableToken`/`CancelToken` polled at engine loop heads + 50 ms `CancelPoolWatcher` (cr_parallel.cpp:152-179; cr_carve.cpp:627 cancel checkpoint per 4 KiB).

---

## 2. File-type detection (current)

Three layers, all in the carve module:

1. **Grammar layer (precision)**: 154 registry rows, one per `FormatId` (155 `FMT_*` enum members incl. `FMT_COUNT` sentinel — engine/include/cr/carve/cr_signatures.h:458; `kTable` in engine/src/core/carve/cr_registry.cpp ends at :1692, `registry_size()` :1716). Each row: `Signature{id, ext, header[16], header_len, header_offset, min_size, max_size, base_confidence, validator, name, suppress_overlap}` (cr_signatures.h:550-568). Validators live in cr_analysers.cpp (7,392 lines) — e.g. `validate_jpeg` reads SOF dims + EXIF (cr_analysers.cpp:701-714), `validate_pdf` (cr_analysers.cpp:156 registry row; :261 `pdf_name_prefix`). Trial order `kTrialOrder` puts specific-before-generic (DOCX>ZIP, CR2>DNG>TIFF, RAR5>RAR4, HIK before MPEGPS) — cr_carve.cpp:59-151. Overlapping candidates ranked by accumulated `base_confidence + validator confidence`, ties by trial order (N-070, cr_carve.cpp:672-789). No header-only rows allowed (F8 gate `validate_registry`, cr_registry.cpp:1723-1750; `self_probe_registry` F8/S-017 self-reachability probe, cr_signatures.h:603).
2. **Raw-sign layer (breadth)**: 139 formats, 199 header signatures, 62 tail signatures generated from corpus raw_signs.json — engine/src/core/carve/cr_rawsigns_data.cpp:14-16. `SignIndex::probe_header` bucket dispatch by (offset, first byte), offsets 0..46 (engine/src/core/carve/cr_rawsigns.cpp:34-49); tail scan at 512-strides, `(hdr+tail)/2+100` blend, header-only fallback with tail-walk bound `kRawSignTailScanLimit` 8 MiB (cr_rawsigns.h:46-64).
3. **User-sign layer (extensible)**: og EUUserData JSON shape, `usersigns::parse`/`install_set`/`snapshot` (engine/src/core/carve/cr_usersigns.cpp:207,337,350); CLI default `userdata-defaults.json` load (engine/src/app/rrecov.cpp:1346-1380); ABI `cr_ui_register_user_signs`.

Extension inference: query-side `category_for_extension` over the generated 1,481-extension FileFilter.xml table (engine/src/core/query/cr_filefilter_data.cpp:23-26; binary search cr_query.cpp:197-222) + N-126 "Unsaved" overlay for .asd/.wbk (cr_query.cpp:212-213). Volume FS family inference: `probe_family` (cr_scan.cpp:963-1185) — NTFS content-verified via MFT record 0 + USA bounds (cr_scan.cpp:969-1034); exFAT boot-region validation (cr_scan.cpp:1036-1055); ReFS OEM+"FSRS" pair (cr_scan.cpp:1063-1067); BitLocker "-FVE-FS-" (cr_scan.cpp:1074); HFS+/HFSX at byte 1024 with version gate (cr_scan.cpp:1079-1086); UDF AVDP at LBA 256 (cr_scan.cpp:1107-1120); FAT 55AA + root-directory readability proof (cr_scan.cpp:1126-1165); ext4 0x53EF at 1024+0x38 (cr_scan.cpp:1167-1170); Btrfs "_BHRfS_M" (cr_scan.cpp:1171-1176); ZFS 0x00BAB10C (cr_scan.cpp:1177-1183).

---

## 3. Size / recovery-chance / name / metadata / path logic

| Dimension | What exists | Citation |
|---|---|---|
| Size (structured) | `logical_size` from FS metadata ($DATA real size / FAT dir-entry size / inode i_size); `allocated_size` | e.g. cr_mftwalk.cpp:941-942 |
| Size (carved) | validator-returned end − start gated by min/max; raw-sign layer uses table min/max (KiB-converted) | cr_carve.cpp:749-755; cr_rawsigns_data.cpp:18-89 |
| Recovery chance | 3-value `Grade` {Low, Average, High} via post-scan `grade_tree`: solo rules (size<1→Low; Email→High; Active→High), twin detection (same extent-end key + category → overlap flag + upgrade), one-sector deep-read validation (all-zero head→Low, 16-byte zero tail→High), 169-entry extension gate | cr_grading.cpp:331-389,395-551,230-294,29-47 |
| Recoverability gauge | AssistantStats population: 20-format candidate set + auto-High classes; total<100 → matched forced 0 | cr_grading.cpp:219-228,553-559 |
| Name (structured) | FS-native names u16; short names; raw bytes preserved | cr_node.cpp:63-69 |
| Name (carved) | og contract `<PREFIX>_<%06d>.<UPPEREXT>` — content-derived prefixes (JPEG EXIF Model/Software, MP4/MOV duration, PDF Title/Creator, DOCX date, ZIP first entry, WAV INAM); two-class session-wide counter map | cr_carve.cpp:424-465,825-829; cr_analysers.cpp:549-714 |
| Metadata | times (carved nodes stamped with scan-time FILETIME S-031, cr_carve.cpp:855), attributes ($SI flags N-012, cr_mftwalk.cpp:949-956), `carve_format`, `carve_confidence` 0..1000, CRC32 header checksum M-051 (cr_carve.cpp:860-865), `carve_note` evidence string N-089, sha128 for files ≤ `hash_threshold` (cr_carve.cpp:874-884), archive inner-entry inventory as child nodes S-032 (cr_carve.cpp:927-955) |
| Path | query `Row.path_utf8` composed from FullNode roles + names (cr_query.cpp:396-416); NTFS parent linkage in-tree; carve grouped `Reconstructed/<volume-display>/files` with 7 empty category folders S-048 (cr_scan.cpp:239-330) |
| Missing / absent | No numeric per-file recovery percentage (grade is 3-valued); no `currentPath` per-item scan position anywhere in the ABI (G-2 — events.rs:218 hard null); no deep-content sampling beyond the single tail sector in grading; AES-CTS units intentionally not implemented (engine/src/core/crypto/cr_aes.cpp:12); UDF recognized but has no engine dispatch (cr_scan.cpp:1104-1106 comment: "No engine dispatch follows") |

---

## 4. Streaming model (summary; detail in §1.5-1.7)

- **Batched snapshot-pull, not row streaming.** Engine→UI events carry counters/deltas only.
- Event types (engine): `CR_EVT_SCAN_STAGE/SCAN_PROGRESS/FILES_FOUND/ERROR_RANGE/CHECKPOINT/LOST_CANDIDATE/REPAIR/HEARTBEAT/SAVE_PROGRESS/FS_CHECK/SESSION_LOAD_PROGRESS/AUTOSAVED` — engine/include/cr/cr_ui.h:151-210; struct `cr_ui_event` cr_ui.h:225.
- Coalescing: engine fires when overall permille advanced OR bytes moved ≥16 MiB (cr_scan.cpp:697-728); ≤1000 progress events + stage flips per scan; heartbeat 5 s watchdog (cr_ui.cpp:883-891,1142).
- Queue: bounded channel in the Rust bridge (`try_send`, drop-on-full — events.rs:50-60); single emitter thread per handle; monotonic overall clamp (events.rs:356).
- Cancellation: `PausableToken` pause/resume/cancel (cr_ui.cpp:2497-2513); per-4-KiB carve checkpoint (cr_carve.cpp:627); `ThreadPool::abort` + watcher (cr_parallel.cpp:78,152).
- UI poll fallback: `useScanState` 500 ms poll while running/stopping (scan.ts:106-120).

---

## 5. Local persistence

- **Session format: CRSF container** — magic `CRSR`, version, flags (`CRSF_FLAG_ABS_EXTENTS` F-1), 64-bit session id, records framed in ≤4 KiB chunks, deflate-when-smaller, per-record CRC32 of decompressed data, record_count + chain patched in `finalize` (engine/src/core/session/cr_crsf.cpp:21-200; header engine/include/cr/session/cr_crsf.h:38-101).
- Record codecs in cr_records.cpp: `serialize_tree`/`deserialize_tree` with per-node progress + cancel hooks (cr_records.cpp:89,355). 12 record types: REC_SESSION_STATE, REC_VOLUME_INFO, REC_PARTITION_CONTEXT, REC_RESULT_TREE, REC_ERROR_MAP, REC_ASSISTANT_STATS, REC_ALLOC_MAP, REC_DISK_LAYOUT, REC_FS_CHECK, REC_LDM_LAYOUTS, REC_CARVE_TALLY, REC_SCAN_FILTER (cr_crsf.h:45-101).
- Checkpoints: same record set under 'RKPP' tag, staging `.tmp` + atomic rename commit (cr_scan.cpp:905-961).
- Last-scan pointer: `cr_lastscan.cpp` (UTF-8→wide file ops, rename-with-replace, retention delete — cr_lastscan.cpp:22-107).
- **No SQLite/WAL anywhere in the engine** (grep: zero matches for sqlite outside the carve signature `sqlite` format row). Persistence is the CRSF binary container + lastscan pointer only.
- Cache layers: in-memory `CachedReader` LRU (8 MiB/256 KiB, hits/misses surfaced in stats — cr_scan.cpp:1595-1596); `viz::downsample_free_map` persisted alloc map (cr_scan.cpp:2980-2987); thumbnail generation on demand with no disk cache (engine/src/core/preview/cr_thumbnail.cpp); preview handle's in-memory archive inventory cache (M-116, cr_ui.cpp:1022-1028); bridge-side `SnapshotCache`/`NodeMeta` per-generation in-memory (state.rs:631-776).

---

## 6. Classification / prioritization

- **Junk filter (post-walk)**: `filterfolder::apply` flags NODE_FLAG_JUNK_FILTERED by name-pool sets + 1 TiB default size cap (N-216), hidden from results but subtree kept (engine/src/core/orchestrator/cr_filterfolder.cpp:91-151).
- **Scan-time folder filter (prune)**: `folderfilter::prune_root` prefix-normalized prune, carve root + pathless "File Path Lost" exempt (engine/src/core/orchestrator/cr_folderfilter.cpp:165-177; boundary application cr_scan.cpp:877-903).
- **Grading/prioritization**: `grade_tree` S-023 (cr_grading.cpp:395) — see §3; query sort by Grade puts High first (cr_query.cpp:286-294).
- **FileFilter.xml equivalent**: generated 1,481-extension / 8-category table (cr_filefilter_data.cpp) + `FilterFolder.ini` SumaryFileType 11 unification groups + `PriorityDisplayType.xml` 35-extension ranked type-display order (engine/src/core/query/cr_config_display_data.cpp:44-63).
- **Carve confidence ranking**: N-070 accumulated score; suppress-overlap stream family; raw-sign confidence blend.
- UI-side classification: file-icon/type mapping via `Category` + bridge `extension_of` (state.rs).

---

## 7. Known weaknesses (marker search + observed gaps)

- **Zero TODO/FIXME/XXX/HACK markers in engine/src and ui/src+src-tauri/src** (grep counts: 0 and 0). Remaining "placeholder/stub" word hits are corpus-parity commentary, not unfinished work (e.g. FAT '#' placeholder name N-172, cr_fat_quick.cpp:225; X3F stub citation in cr_analysers.cpp:5185).
- Gaps observed while reading (not marker-based):
  1. **No per-result streaming**: result rows reach the UI only via snapshot + bridge-local filter (results.rs:11-17 acknowledges the engine query cannot express the contract — the Rust side re-implements filtering, a duplicated filter semantics surface).
  2. **`currentPath` always null** in ScanProgressEvent — no ABI event carries the item being scanned (events.rs:218-220).
  3. **Grading skips populations**: extension-in-table nodes never reach twin detection (S-058 inverted gate, cr_grading.cpp:480-494); deep-read grade only for non-twin, index==0 nodes; undecoded og sub-gates deliberately not adopted (cr_grading.cpp:8-10).
  4. **Tail-scan bound 8 MiB vs og 200 MiB** — raw-sign tail walk falls back to header-only beyond it (cr_rawsigns.h:46-49).
  5. **`kMaxTrackedRejections` 1 Mi cap** — user-sign no-reclaim degrades on garbage-saturated media (cr_carve.cpp:48-54).
  6. **AES-CTS not implemented** (cr_aes.cpp:12) — limits some BitLocker configurations.
  7. **UDF: recognition only, no engine** (cr_scan.cpp:1104-1106).
  8. **Session file remains the only persistence**; nothing caches derived artifacts (thumbnails, previews, per-format tallies) across process restarts (tally is reloaded only via REC_CARVE_TALLY).
  9. **Bridge-side result filtering duplication**: engine `query::run` and Rust `matches_pred`/`sort_matches` (results.rs:285,369) are two implementations of the same semantics; drift risk documented as "future engine-side query migration as the follow-up" (results.rs:17).

---

## 8. How tests exercise the pipeline today

Engine tests (engine/tests/unit, GoogleTest, run by ctest in engine-ci):

| Suite | Coverage |
|---|---|
| test_orchestrator.cpp (72 TESTs) | whole-disk target sets, lost-partition naming/cap, LDM, junk gate, spoofed-OEM rejection, UDF probe, weighted progress, log brackets, memory monitor (engine/tests/unit/test_orchestrator.cpp:175-1344) |
| test_carve.cpp (338 TESTs) | grammar/raw-sign/user-sign layers, naming contract, dedup, gates |
| test_cr_ui.cpp (91 TESTs) | ABI version, carve registry enumeration, per-format tally, scan lifecycle over image, cancel-save-resume, auto-save gates (not-import/results-found/not-already-saved), session list, scan-time folder filter at ABI, heartbeat during stall (test_cr_ui.cpp:476-1229) |
| test_session.cpp (48 TESTs) | CRSF round-trips (extents, identity card, carve tree), corruption skip-forward (F10), torn checkpoint salvage, hostile length bombs, save progress/cancel, auto naming (test_session.cpp:59-1200) |
| test_telemetry.cpp (12 TESTs) | monotonic progress, live counters, mid-scan visibility (files/bytes/errors), cancel inside carve window, deep byte axes (test_telemetry.cpp:215-618) |
| test_query.cpp (23 TESTs) + test_query_perf.cpp | filter/sort/paging semantics + the 1M-node <200 ms CI budget |
| test_scan_truth.cpp | fake-scan refusal: fully-unreadable volume → ERR_READ_FAILED; partial damage salvage; blind probe flagged (test_scan_truth.cpp:80-166) |
| test_grading.cpp, test_folderfilter.cpp, test_vollabel.cpp, test_diskmap.cpp, test_snapshot.cpp, test_node_tree.cpp, test_parallel.cpp, test_pause_resume.cpp, per-FS volume tests (ntfs/fat/exfat/ext4/btrfs/hfsplus/refs/zfs) | component-level |

CI (`.github/workflows`):
- **engine-ci.yml**: Windows-2022 only. Jobs: (1) lint — clang-format 18.1.8 pinned, dead-surface/architecture lints, PowerShell AST + workflow YAML parse gates, doc-truth gate; (2) build+test — MSVC /W4 /WX Release via vcpkg, full ctest, Win7 posture gate, report-schema validation, elevation refusal probes. Sanitizer/fuzz lanes run only locally via `engine/ci-local.sh --san/--fuzz` (engine-ci.yml:12-15).
- **ui-ci.yml**: windows-latest single job: pnpm typecheck/lint/format, arch-lint A1-A7 + ast-grep, filefilter mirror freshness, knip dead code, jscpd duplication, vitest coverage thresholds 70/60/70/70, production build, bundle budget, `cargo check --tests` + clippy -D warnings for the bridge, web-dist artifact (ui-ci.yml:26-114).

**Not covered by tests/CI today**: no per-result-row streaming assertions (the emitter coalescing path is compile-checked via `cargo check --tests` but the vitest lane runs against the mock connector, not the Rust bridge); no Linux/macOS engine CI; no sanitizer/fuzz in CI; no e2e/browser lane in ui-ci (ui/e2e exists in-tree but is not in the workflow's job steps); the bridge's NodeMeta filter/sort (results.rs) is exercised only by Rust unit tests under `cargo check --tests`, not against engine fixtures; engine-side `query::run` is tested but the bridge bypasses it (§7.9) so the shipped result-list path has no engine-side contract test.

---

## Appendix: event name → payload quick map (Tauri surface)

| Tauri event | Source | Payload keys |
|---|---|---|
| `scan-progress:{scanId}` | events.rs:172 compose | scanId, status, stage, stageProgress, overallProgress, bytesScanned, bytesTotal, filesFound, dataFoundBytes, errors, badSectorMap, etaSeconds, throughputBytesPerSecond, recoverableDuringScan, deviceConnected, currentPath (null), currentSector, currentSectorTotal, sectorSize, phase, phaseProgress |
| `scan-finished:{scanId}` | scan.rs:452 run tail | status, failure, autosaved |
| `save-progress:{scanId}` | events.rs:418 | fraction |
| `lost-candidate:{scanId}` | events.rs:390 | lba, sectorCount, fs, score, count |
| `scan-autosaved:{scanId}` | events.rs:467 (CR_EVT_AUTOSAVED arm) | autosave outcome |
| `consent-request:{scanId}` | bridge/consent.rs | repair plan consent |
| `session-load-progress:{path}` | events.rs:86 | permille, nodesDone, nodesTotal |
| `recovery-progress/finished:{jobId}` | events.rs recovery emitter | bytes, counts, currentFile |
