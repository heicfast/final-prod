# CORE-API-CURRENT — the current public API surface (v0.4.1-parity tree, HEAD 22e9657)

> Written by reading agent A (Task 2-a). Original census cited `file:line`
> against `22e9657`. **Cite refresh (api-fix-2 lane):** the LIVE sections'
> cites were re-verified against the current tree — §(a) Win platform layer,
> §(b) ABI tables, §(c) CLI grammar, §(d).5 — and re-pinned where drifted;
> §(e)/§(f) keep their historical 22e9657-era cites under the banner below.
> Companion document: [CORE-DISK-ACCESS.md](CORE-DISK-ACCESS.md) (the transport decision map).
> This file inventories the surface as it EXISTS; historical/parity context lives in
> [API.md](API.md), [API-SURFACE-AUDIT.md](API-SURFACE-AUDIT.md), [ABI-CONTRACT.md](ABI-CONTRACT.md).

**Headline counts** (re-derived 2026-10-29, the S56 lane — the
doc_truth_gate count invariants guard these in CI): C++ public headers 82
under `include/cr/**`; C ABI **91 exports** (94 `CR_UI_API` lines minus 3
`#define` macros in cr_ui.h = 91 `extern "C" CR_UI_API` definitions in
src/ui/cr_ui.cpp, recount matches; +2 from the N-140 lane's carve-tally pair); CLI **13 verbs** (rrecov.cpp — list,
list-carve-formats, sessions, scan, tree, recover, archive-list, health,
fix, apply, image, bench, driver); error codes **43 + OK** (the enum-member
census: 43 ERR_ members in cr_error.h — the long-standing "45" was the
doc_truth_gate's file-wide 0x5-literal regex counting the truncated
prefixes of the two og-corpus comment literals at cr_error.h:6; the
Session-84 truth repair anchors the census on enum-member lines, and
the gate derives this). errmod modules **13**
(kNone sentinel + 12 attribution modules — the enum census re-derived with
the other counts);
ABI event types **12** (the CR_EVT_* census).

>
> **POST-EXPANSION STATE (driver-only session, commit e58bde1+):** this sheet
> is the pre-change census. Added since: the ERR_DRIVER_* family (0x500B),
> the `cr_ui_driver_status` ABI export + `cr_ui_scan_stats` truth fields
> (bytes_unreadable / probe_unreadable), the `rrecov driver` verb, exit code
> 4 (driver prerequisite), and the read-truth scan verdict (ERR_READ_FAILED
> on total blindness). Current contracts: [DRIVER-REQUIRED.md](DRIVER-REQUIRED.md),
> [cr_ui.h](../include/cr/cr_ui.h).
>
> **SUPERSEDED SECTIONS (do not read as current):** §(e) items 1-7 and §(f)
> in their entirety describe the PRE-driver-only status surface —
> `rda_available`/`win_pe_environment` were REMOVED by PLAN-DRIVER-ONLY
> (probe → `win::query_driver_status`); `cr_ui_device_transport` reports
> "rda"/"image" only (no "direct"); `cr_ui_last_transport_error` is the
> driver-only advisory (ABI-CONTRACT §7e); the wire IOCTLs are now ISSUED
> (D-DRV-5, GET_INFO 0x6002440 + GET_VERSION 0x6002448); every §(f) gap is
> closed by the 8-step classifier + `cr_ui_driver_status` + the ABI error
> family. The b.2 row "RDA-first with direct fallback" is likewise
> superseded (classified ERR_DRIVER_* refusal, no fallback). Current
> counts: 91 exports / 13 verbs / 43+OK error codes / 12 ABI event types
> (re-derived 2026-10-29, the S79 lane's N-140-rider count repair - the
> doc_truth_gate guards these).
> Current contract docs: [ABI-CONTRACT.md](ABI-CONTRACT.md),
> [DRIVER-REQUIRED.md](DRIVER-REQUIRED.md), [API.md](API.md).
---

## (a) C++ API inventory (grouped by module; one line per public operation)

### Foundation (`include/cr/`)

| Symbol | Purpose |
|---|---|
| `cr::u8..u64/i8..i64`, `u32_from_bytes_le` etc. | fixed-width types + LE/BE folds (cr_types.h:20-39, cr_bits.h:21-42) |
| `cr::FileTimes`, `unix_to_filetime(_ns)` | 100-ns-1601 time model (cr_types.h:45-66) |
| `cr::FsKindCode` | serialized fs-kind enum (cr_types.h:81+) |
| `cr::Err` + `err_text()` | the error space (cr_error.h:25-89; §(d) below) |
| `errmod::Module` (kNone..kLdm), `current/set_thread/Guard` | per-thread module attribution (cr_errmod.h:50-91; impl cr_errmod.cpp:14-25) |
| `log::set_sink/enabled/write/check_memory_pressure` | the N-048 log seam (cr_log.h:48-79) |
| `ThreadPool`, `PoolCancelToken`, `parallel_for`, `CancelPoolWatcher` | bounded worker pool (cr_parallel.h:43-112) |
| `recycle::Info/parse_record/token_of/is_bin_root` | $I/$R recycle pairing model (cr_recycle.h:42-127) |
| `text::utf8_to_u16(_lossless)/lossless_u16_to_utf8/wide_to_utf8/...` | the shared codec (cr_text.h:40-240) |

### Storage seam (`include/cr/storage/`)

| Symbol | Purpose |
|---|---|
| `IStorageDevice` | the one read interface engines see (cr_device.h:117-134) |
| `Geometry`, `is_4kn()` | probed byte-anchored geometry (cr_device.h:43-54) |
| `ReadErrorRange`, `ReadErrorMap` (+serialize/deserialize) | persistent read-failure map (cr_device.h:57-88) |
| `RetryMode`, `RetryPolicy` | Fail/Skip + max_attempts + backtrack (cr_device.h:90-108) |
| `CancelToken`, `PausableToken` | cooperative cancel + pause/resume with paused_ms (cr_device.h:136-234) |
| `PartitionSlice::create/read_at/refresh_geometry` | bounded virtual-volume view (cr_slice.h:30-68) |
| `SliceShim` | non-owning ownership adapter (cr_slice.h:75-91) |
| `CachedReader::create` (+hits/misses) | bounded true-LRU cache, 8 MiB registered divergence (cr_cached.h:50-82) |
| `RetryingDevice::create` (+error_map[_copy], set_owner_module) | retry/bisect/Skip wrapper (cr_retry.h:36-80) |
| `io::direct_envelope`, `kDirectAlignBytes` | the 64 KiB direct-I/O envelope math (cr_directalign.h:33-73) |
| `storage::ImageFileIo` | injected raw-file IO seam (cr_image.h:62-71) |
| `storage::open_image`, `classify_extension`, `classify_lba0`, `ImageFormat`, `Lba0Layout` | image-target open + classification (cr_image.h:75-109) |
| `storage::FreeMap` | engine allocation bitmap (cr_freemap.h:35-71) |
| `DecryptingDevice::create` | in-stack XTS-AES wrap over a slice (cr_decrypt_device.h:48-73) |

### Orchestrator (`include/cr/orchestrator/`)

| Symbol | Purpose |
|---|---|
| `orchestrator::FsFamily` (0..11, 5 reserved, 10 BitLocker, 11 UDF) | family enum, serialized values (cr_scan.h:36-69) |
| `StageWeights` (40/35/25), `weighted_progress` | monotonic weighted progress (cr_scan.h:71-75, 443) |
| `ReadPipe`, `build_read_pipe(slice, cache, block)` | the testable scan pipe constructor (cr_scan.h:86-96; cr_scan.cpp:153) |
| `event::Type` (ScanStage..FsCheck, 8 values) + `event::Event` + `EventFn` | the typed event stream (cr_scan.h:122-183) |
| `ScanConfig` | stage enables, whole_disk, fs_hint, threads, repair hooks (S-008), pre_scan_check (M-041), identity card fields, raw_category_name, CheckpointSink, ldm_resolver, event_fn (cr_scan.h:185-315) |
| `ScanStats` | counters, free_map, repair/check outcomes, grading stats, resume-truth signals (cr_scan.h:317-407) |
| `StageTelemetry`, `ScanTelemetrySnapshot` | live per-stage snapshots (cr_scan.h:418-440) |
| `ScanController` | the staged pipeline controller: ctors (fresh/resumed), `probe_family`, `run`, `save_session`, `load_session`, `resume`, `load_session_summary`, `telemetry`, `checkpoint_count`, `set_event_sink`, `config` (cr_scan.h:455-668; run at cr_scan.cpp:827, dispatch 1356, save 1816, load 1955/1961, resume 2116) |
| `filterfolder::is_junk_name` | the M-018 junk-name filter (cr_filterfolder.h:70) |

### Engines (`include/cr/<family>/`)

| Symbol | Purpose |
|---|---|
| `ntfs::NtfsVolume::open`, `walk_mft`, `scan_mft_remnants`, `VolumeBitmap`, `WalkerFlags/WalkStats` | NTFS MFT walk + remnant sweep (cr_mftwalk.h:166,182; dispatch cr_scan.cpp:1245) |
| `ntfs::fixup/runlist/attributes/bitmap/bootstrap` wire+logic headers | per-concern NTFS primitives (cr_fixup.h, cr_runlist.h, cr_attributes.h, cr_bitmap.h, cr_bootstrap.h) |
| `fat::scan_fat`, `FatTable`, `follow_chain`, `reconstruct_deleted_chain`, `reconstruct_contiguous_chain`, `flavor_from_clusters`, `short_name_to_u16` | FAT12/16/32 engine (cr_fat_engine.h:30-256) |
| `exfat::scan_exfat`, `validate_boot_region`, `ExfatParams/Flags/Stats` | exFAT engine (cr_exfat_engine.h:35-127) |
| `ext4::scan_ext4`, `validate_superblock` | ext4 engine (cr_ext4_engine.h:84,107) |
| `hfsplus::scan_hfsplus`, `validate_volume_header` | HFS+ engine (cr_hfsplus_engine.h:90,98) |
| `btrfs::scan_btrfs`, `validate_superblock`, `ChunkMap` | Btrfs engine (cr_btrfs_engine.h:124,126) |
| `zfs::scan_zfs`, `LabelConfig` | ZFS engine (cr_zfs_engine.h:75,85) |
| `refs::scan_refs`, `validate_boot`, `refs_detect` | ReFS engine + detection (cr_refs_engine.h:110-115, cr_refs_detect.h:31) |
| `table::parse_disk_table`, `DiskTable/PartitionInfo/TableLayout/FsKind`, `mbr_type_to_kind` | pure partition-table parse (cr_table_parse.h:23-104) |
| `table::LdmInfo`, `parse_ldm_database`, LDM object structs, `LdmDiskResolver`, volume assembly | full LDM decode (cr_ldm.h:66-221+) |
| `lost::discover`, `Candidate`, `Evidence`, `classify_overlap`, `score_ratio`, `weighted_count`, `materialize`, `revalidate_candidate` | lost-partition discovery + scoring (cr_lostpart.h:37-162) |
| `consistency::` (cr_consistency.h) | M-035 content-consistency scorer |

### Carve / check / repair / apply / bootfix (`include/cr/`)

| Symbol | Purpose |
|---|---|
| `carve::carve_slice` + `CarveStats/CarveFlags/CarveCandidateInfo/CarvePostProcessor/default_namer` | the carve engine entry + config (cr_carve.h:27-154) |
| `carve::Signature`, `registry()`, `registry_size()`, `FormatId` | grammar-validator registry (cr_signatures.h:23-185) |
| `carve::rawsigns::kFormats/kHeaderSigns/SignIndex` | 139-format raw-sign prefilter (cr_rawsigns.h:76-102) |
| `carve::usersigns::parse/install_set/active_count` + `UserSign(Set)` | user-extensible signatures (M-048) (cr_usersigns.h:43-51) |
| `check::run_fscheck/check_fat/check_ntfs` + `FsCheckReport/CheckIssue/CheckCallbacks` (taxonomy 0x1001..0x100e) | read-only damage audit (M-041) (cr_fscheck.h:79-180) |
| `repair::analyze/execute/undo/rebase_plan` + `IWriteDevice/RepairPlan/UndoLog` | transactional RawFixer (S-008) (cr_rawfixer.h:67-179) |
| `apply::plan_apply/execute_apply/undo_apply/mbr_type_for_family` + `IApplyDevice/ApplyPlan/ApplyConsentFn` | lost-partition table apply (S-029) (cr_apply.h:97-261) |
| `bcdfix::` plan/execute/undo + `BcdRewrite` | BCD offset surgery (M-040a) (cr_bcdfix.h:62) |
| `nvramfix::` + `TargetPartition/NvramRewrite` | UEFI NVRAM Boot#### updater (M-040b) (cr_nvramfix.h:96-124) |

### Query / preview / tree / grading / diskmap

| Symbol | Purpose |
|---|---|
| `query::run` + `Query/Filter/Row/Result/SortKey/SortDir/Category`, `category_for_extension`, `apply_date_bucket/apply_size_bucket`, `filefilter_entry_count` | server-side filter/sort/page (G-12/M-029) (cr_query.h:34-182) |
| `preview::PreviewReader` (+read_at/size/unreadable_spans/read_cost) + `extract_metadata` + `Caps/Metadata` | bounded content preview (G-10) (cr_preview.h:54-133) |
| `tree::ResultTree`, `FileNode/FullNode`, `FileProperties`, `Origin/DeletedState/Confidence/Grade` enums | the dual-class result graph (cr_node.h:35-189+) |
| `tree::Snapshot` + `SnapNode` | mid-scan immutable value capture (G-02) (cr_snapshot.h:39-73) |
| `tree::grading::grade_tree` + `GradeContext/AssistantStats` | S-023 CRecoverability pass (cr_grading.h:103-142) |
| `tree::build_disk_map` (tree + snapshot overloads) + `downsample_free_map` | GAP-S6-11 disk-map producer (cr_diskmap.h:88-107) |

### Session / recovery / report / dict

| Symbol | Purpose |
|---|---|
| `session::Writer/Reader` (CRSF magic/version/flags; `CRSF_FLAG_ABS_EXTENTS`) | chunked-zlib container IO (cr_crsf.h:30-150) |
| `session::serialize/deserialize_*` (tree, error_map, free_map, volume_info, session_state, partition_context, disk_layout, assistant_stats, fs_check, records) + `VolumeInfo/SessionState/PartitionContext/DiskLayoutEntry` | the record set (cr_records.h:46-195) |
| `recovery::SafeWriter` + `DestIo`, `WriterConfig/WriterStats/QueueItem/ItemState/ConflictPolicy`, `enqueue`, `set_destination(_spans)`, `process` | the safe writer (cr_safewriter.h:31-152) |
| `recovery::report::build_report/escape_json/engine_version` + `Item/Totals/ScanAnalysis/Outcome/HashStatus` | G-09 JSON report (cr_report.h:26-121) |
| `dict::parse/try_passwords` + `kDictEntryCount` (228) | N-064 password-dictionary aid (cr_dict.h:43-77) |

### Win platform layer (`include/cr/win/` + health)

| Symbol | Purpose |
|---|---|
| `win::query_driver_status(service_name, out)` | the driver-status diagnostic query — fills every facet, never fails on driver condition, it REPORTS it (cr_transport.h:53) |
| `win::open_rda_partition/open_rda_volume` | grammar-path driver opens (cr_transport.h:64-66) |
| `win::image_file_io()` | the Win32 ImageFileIo singleton (cr_transport.h:71) |
| `win::open_physical_drive` | CONTROL-PATH whole-disk open (\\.\\PhysicalDriveN), enumeration-only — NOT a data-path open; `open_harddisk_volume` and the direct data-path fallbacks are REMOVED (cr_transport.h:80) |
| `win::open_disk(disk, service, err)` | the whole-disk data-path open over the RDA grammar path — driver REQUIRED, the grammar open is the only leg (cr_transport.h:89) |
| `win::open_disk_rw` / `open_disk_apply_rw` | write-capable views (repair/apply), RDA handles GENERIC_READ|WRITE (cr_transport.h:100/:121) |
| `win::refresh_disk_properties` | S-029 IOCTL_DISK_UPDATE_PROPERTIES + settle (cr_transport.h:110) |
| `win::volume_extent/volume_extents/destination_volume_letter` | N-013 destination arming (cr_transport.h:127-152) |
| `win::enumerate_disks/enumerate_disk_interfaces/merge_disk_enumeration/enumerate_volumes` + `PhysicalDisk/VolumeEntry` + `decode_device_descriptor/decode_device_id_descriptor/bus_interface_name` | N-071 two-stack enumeration (cr_enumerate.h:37-258) |
| `win::WinDestIo` | Win32 atomic temp+rename DestIo (cr_destio.h:17-45) |
| `win::is_elevated()` | GAP-P1-5 elevation probe (cr_elevate.h:23) |
| `win::health::read_smart/read_nvme_health/query_trim_state/looks_like_ssd/smart_attr_name` + `SmartReport/SmartAttribute/NvmeHealth` | health surface (cr_health.h:45-98) |
| `win::bitlocker::is_fve_volume/status/unlock/try_auto_unlock(_offline)/unlock_with_dictionary` (+detail seams) | R8/S-014/N-064 BitLocker surface (cr_bitlocker.h:31-114) |
| `win::bootfix::bcd_file_plan/bcd_file_execute/nvram_plan/nvram_execute/force_unload_bcd_hive/...` | M-040 win runtime (cr_bootfix.h:72-95) |
| `rda::compute_pid_token/control_device_path` + all wire constants/structs | the driver wire contract mirror (cr_rda.h:38-225) — structs doc-only, no client issues them |
| `health::parse_nvme_health_page/grade_smart/grade_nvme/classify_family/...` (cr_nvme.h, cr_grade.h, cr_smart_ata.h, cr_csmi.h) | pure health parsing/grading seams |

---

## (b) The C UI ABI — every export (78; `CR_UI_ABI_VERSION 2`, cr_ui.h:79)

Rules (cr_ui.h:38-68): C11, u32 returns from the cr::Err space, UTF-8 in / two-call or
heap-out strings, zero-init configs always safe, no exception escapes (catch-all returns
`CR_UI_ERR_INTERNAL` 0x50090001), append-only policy.

### b.1 Version & diagnostics (3)

| Export | Contract |
|---|---|
| `cr_ui_abi_version()` | returns 2; call first, refuse on mismatch (cr_ui.h:120-122) |
| `cr_ui_last_error()` | thread-local text of the LAST failing call; stable until next call on that thread (cr_ui.h:125-128) |
| `cr_ui_string_free(char*)` | frees heap strings (free() also valid) (cr_ui.h:128) |

### b.2 Devices & transport truth (9)

| Export | Contract |
|---|---|
| `cr_ui_open_image(utf8, offset, bytes, out)` | flat/VMDK/VHD/VHDX read-only device bounded to [offset, offset+bytes); bytes=0 → to EOF (cr_ui.h:296) |
| `cr_ui_open_disk(disk, out)` | Windows only: REFUSES with the classified ERR_DRIVER_* code when the RDA driver is absent/not-started/unauthorized/version-incompatible/errored — NO direct-handle fallback (PLAN-DRIVER-ONLY); transport (always "rda") reported by device_transport (cr_ui.h:314). S-051: scanning a whole-disk target reads the partition list at scan setup and makes every used, non-container partition a scan target (the og's CDoScanPartition flow; architecture/04-recovery-workflow.md §3.4) — a host-set fs_hint keeps the single-dispatch slice assertion, an unasserted target with a table never silently yields nothing; per-volume skips are surfaced in ScanStats.table_volumes* |
| `cr_ui_open_volume(letter, out)` | BitLocker probe first; locked → auto-unlock attempt → `CR_UI_ERR_FVE_LOCKED` (0x50060001); returns the volume slice on its parent disk (cr_ui.h:315) |
| `cr_ui_last_transport_error()` | M-030: surviving transport leg's error from the LAST cr_ui_open_disk on this process; advisory, process-global, never clears (cr_ui.h:309; impl cr_ui.cpp:1505) |
| `cr_ui_open_volume_with_key(letter, hex_fvek, out)` | M-033: locked-volume open with user FVEK (64/128 hex chars → AES-128/256-XTS in-stack decrypt) (cr_ui.h:391) |
| `cr_ui_unlock_volume_with_dictionary(letter, passwords, count)` | N-064 explicit-consent dictionary unlock (shipped 228 + user candidates) (cr_ui.h:407) |
| `cr_ui_device_close(dev)` | closes; caller-owned close order (cr_ui.h:411) |
| `cr_ui_device_geometry(dev, out)` | 4Kn-aware geometry (cr_ui.h:420) |
| `cr_ui_device_transport(dev, buf, cap, needed)` | "rda" / "image" two-call string (cr_ui.h:427) |

### b.3 Enumeration (2)

| Export | Contract |
|---|---|
| `cr_ui_enumerate_disks(out, count)` | PhysicalDrive census ≤ 200 disks; failure returns error (never silent empty); ALWAYS FRESH — every call re-queries the OS, no cache, no retained state (calling again IS the refresh; the og's modal refresh flow re-runs the same machinery, the modal chrome is host-side; the update-disk-metadata case is the C++-only `win::refresh_disk_properties`, S-029) — the N-098 contract sentence, cr_ui.h:472-484 |
| `cr_ui_enumerate_volumes(out, count)` | letter/disk correlation + FVE flag (cr_ui.h:459) |

### b.4 Scan lifecycle (18)

| Export | Contract |
|---|---|
| `cr_ui_scan_create(cfg, out)` / `cr_ui_scan_close` | handle lifecycle; zero-init cfg = defaults (cr_ui.h:566-567) |
| `cr_ui_scan_attach_device` | bind device BEFORE run; identity card auto-fill (Windows) (cr_ui.h:572-574) |
| `cr_ui_scan_run` | synchronous on caller's thread; 0 / engine error / `CR_UI_ERR_CANCELLED` with tree intact (cr_ui.h:577-579) |
| `cr_ui_scan_pause` / `_resume` / `_cancel` | token control, any thread (cr_ui.h:580-582) |
| `cr_ui_scan_busy` / `cr_ui_scan_stopping` | N-075 synchronous state queries (live run state / live cancel request) (cr_ui.h:595-596) |
| `cr_ui_scan_set_event_fn` | subscribe pre-run only (ERR_BAD_PARAMETER mid-run) (cr_ui.h:286-288) |
| `cr_ui_scan_telemetry` | any-thread live snapshot (cr_ui.h:618) |
| `cr_ui_scan_get_stats` | post-run counters (cr_ui.h:650) |
| `cr_ui_scan_assistant` | S-023 grading statistics (cr_ui.h:679) |
| `cr_ui_scan_checkpoint_count` | G-13 emitted checkpoints (cr_ui.h:687) |
| `cr_ui_scan_repair_save_undo` | persist S-008 repair undo log (cr_ui.h:696) |
| `cr_ui_scan_save_session(path)` / `cr_ui_scan_save_cancel` | N-060 atomic save + save-scoped cancel (cr_ui.h:773-779, 703-712) |
| `cr_ui_scan_snapshot` | G-02 immutable mid-scan/after value capture (cr_ui.h:779-782) |
| `cr_ui_scan_verify_identity` | boot-hash mismatch → `CR_UI_ERR_BAD_CHECKSUM` (cr_ui.h:787-791) |

### b.5 Apply (3) — S-029 lost-partition table writes

| Export | Contract |
|---|---|
| `cr_ui_apply_lost_partition(dev, cand, rebuild, consent, user, out)` | plan → consent callback → transactional write + undo; default-deny (cr_ui.h:746-754) |
| `cr_ui_apply_undo_last(dev, out)` | byte-exact restore of last apply (cr_ui.h:753) |
| `cr_ui_apply_save_undo(dev, path)` | persist CRUNDO01 container (cr_ui.h:758) |

### b.6 Sessions & snapshots (14)

| Export | Contract |
|---|---|
| `cr_ui_session_load` / `cr_ui_session_close` | .crsf container lifecycle (cr_ui.h:796-798) |
| `cr_ui_session_get_info` | family, masks, slice, identity card, boot_hash (cr_ui.h:811) |
| `cr_ui_session_stats` | the stats block (cr_ui.h:812) |
| `cr_ui_session_regrade` | S-023 per-selection re-grade (grade-1 only) (cr_ui.h:685-687) |
| `cr_ui_session_snapshot` | post-scan value capture (cr_ui.h:813) |
| `cr_ui_session_resume(sess, original_cfg, out)` | mask-validated resume flow (cr_ui.h:820-821) |
| `cr_ui_session_list(dir, rows, count)` | W1a (R-56) the saved-session directory list: the light per-file summary (LastScanRecordView), two-call size pattern, error rows for unreadable files (cr_ui.h:823-865) |
| `cr_ui_session_load_with_events(path, fn, user, cancel_flag, out)` | W1b (R-54) the load with progress events + the caller-owned cancel flag; NULL/NULL == plain load; cancel -> ERR_CANCELLED, no session out (cr_ui.h:867-886) |
| `cr_ui_snapshot_close` / `_node_count` / `_partial` / `_node(index, info, name_buf, cap)` / `_dfs(indices, count)` | index-addressed immutable rows; TRUNCATED two-call pattern (cr_ui.h:922-938) |

### b.7 Query (3)

| Export | Contract |
|---|---|
| `cr_ui_query_run(sess, q, rows, count, total)` | G-12 filter/sort/page; heap rows (cr_ui.h:985-987) |
| `cr_ui_query_rows_free` | frees the row array (cr_ui.h:988) |
| `cr_ui_query_apply_buckets` | M-029 og date/size bucket-id translation, unknown IDs rejected (cr_ui.h:1007-1013) |

### b.8 Carve registry + user signs (5)

| Export | Contract |
|---|---|
| `cr_ui_carve_format_count()` / `cr_ui_carve_format_info(index, out)` | N-033 two-layer enumeration, stable order (cr_ui.h:1036-1043) |
| `cr_ui_register_user_signs(json, len)` | M-048 atomic install (cr_ui.h:1057) |
| `cr_ui_user_sign_count()` / `cr_ui_user_sign_info` | enumerate active set (cr_ui.h:1068-1070) |

### b.9 Disk map (2)

| Export | Contract |
|---|---|
| `cr_ui_disk_map(sess, snap, want, got, alloc/coverage/found/confidence, bad_ranges...)` | GAP-S6-11 overlay producer; TRUNCATED two-call for bad ranges (cr_ui.h:1112-1117) |
| `cr_ui_disk_map_free(overlay)` | frees one overlay array (cr_ui.h:1120) |

### b.10 Preview (7)

| Export | Contract |
|---|---|
| `cr_ui_preview_open_snapshot(dev, snap, index, out)` / `cr_ui_preview_open_session(dev, sess, node_id, out)` | private-props copy; DEVICE must outlive (cr_ui.h:1132-1135) |
| `cr_ui_preview_close` / `cr_ui_preview_size` | lifecycle (cr_ui.h:1136-1141) |
| `cr_ui_preview_read(pv, off, buf, len, placed)` | 4 MiB/call cap; short only at EOF; unreadable spans zero-filled AND reported (cr_ui.h:1144-1149) |
| `cr_ui_preview_metadata(pv, out)` | capped kind/format/EXIF/codec/text extraction (cr_ui.h:1162) |
| `cr_ui_preview_unreadable(pv, total, count)` | truthful failure map (cr_ui.h:1166) |

### b.11 Recovery (12)

| Export | Contract |
|---|---|
| `cr_ui_recovery_create(source, cfg, out)` / `cr_ui_recovery_close` | writer session over injected DestIo (cr_ui.h:1187-1191) |
| `cr_ui_recovery_set_destination(path)` | arms F12 overlap refusal on Windows (cr_ui.h:1196) |
| `cr_ui_recovery_enqueue_node` / `_subtree` / `_snapshot` / `_subtree_snapshot` | queue building (session node_id or snapshot index) (cr_ui.h:1203-1211) |
| `cr_ui_recovery_queue_count` | live depth (cr_ui.h:1214) |
| `cr_ui_recovery_process` / `cr_ui_recovery_cancel` | synchronous process on caller's thread; ERR_CANCELLED keeps committed items (cr_ui.h:1220-1223) |
| `cr_ui_recovery_get_stats` | per-outcome counters incl. salvaged/nothing_to_recover (cr_ui.h:1187) |
| `cr_ui_recovery_report_json(rec, source, json_out)` | G-09 report, valid any time (cr_ui.h:1192) |

### b.12 Health (1)

| Export | Contract |
|---|---|
| `cr_ui_health_read(disk, rep, attrs, cap, count)` | SMART + NVMe + SSD heuristic + TRIM in one call; Windows-only (ERR_UNSUPPORTED elsewhere) (cr_ui.h:1237-1240) |

### b.13 Events (the payload contract, not exports)

`cr_ui_event` POD with `type` 1..10 (cr_ui.h:146-239): `CR_EVT_SCAN_STAGE` 1,
`SCAN_PROGRESS` 2, `FILES_FOUND` 3, `ERROR_RANGE` 4 (with err_module/err_operation/
err_attempts M-049 triple), `CHECKPOINT` 5, `LOST_CANDIDATE` 6, `REPAIR` 7,
`HEARTBEAT` 8 (N-043 watchdog, shim-side), `SAVE_PROGRESS` 9 (N-060, shim-side),
`FS_CHECK` 10 (M-041). Unknown types MUST be ignored (forward compat, cr_ui.h:135-136).

### b.14 Threading / errors (cross-cutting)

- run()/process() synchronous on caller's thread; pause/resume/cancel/telemetry/snapshot/
  busy/stopping any thread (cr_ui.h:47-51; ABI-CONTRACT §3).
- Progress + event callbacks fire on worker threads; must not block; NO cancellation
  semantics in callbacks (cr_ui.h:106-110).
- Every export is wrapped by `guarded()` (cr_ui.cpp) → catch-all → `CR_UI_ERR_INTERNAL`
  with `cr_ui_last_error` text.
- Export mechanics: MSVC exports only `__declspec(dllexport)` symbols plus exactly ONE
  C++ symbol via cr_ui_exports.def (`?set_sink@log@cr@@...`, src/ui/cr_ui_exports.def:19)
  for the shim's own test; Linux .so keeps default visibility (def header comment :1-16).

---

## (c) CLI grammar — every verb, flag, exit code (src/app/rrecov.cpp)

Verbs dispatched in `wmain` (rrecov.cpp:3043-3063): **list, list-carve-formats, sessions, scan,
tree, recover, health, fix, apply, bench, driver** (11). `rrecov` with no args prints usage, exit 0.
Unknown verb → `unknown command: %s`, exit 1. All output is wprintf (stdout); errors are
printed to stdout too — the ONE deliberate stderr surface is the driver-refusal path
(`require_driver`'s `fwprintf(stderr)` lines, rrecov.cpp:332-383: exit 4 carries the
ERR_DRIVER_* class on stderr by contract, docs/DRIVER-REQUIRED.md) — "Everything is logged in
detail ... for testing, validation and benchmarking workflows", rrecov.cpp:39-40; the
`--log FILE` receives the detailed seam log, N-048 wiring rrecov.cpp:148-183).

### c.1 `list` (rrecov.cpp:674-702)
Enumerates disks (200-disk bound) + volumes. Flags: none.
Exit: 0 ok / **1** enumeration failure (FB-K01 fix, rrecov.cpp:676-680).

### c.2 `list-carve-formats` (rrecov.cpp:711-773)
Enumerates the two-layer carve registry (grammar + rawsign). Flags: none.
Exit: 0 / 1 (empty registry is never exit-0, rrecov.cpp:723-726).

### c.3 `sessions` (rrecov.cpp:901-1058)
W1a (R-56): the saved-session list (the og's LastScanRecordView). Flags:
`--dir D` (default: the current directory), `--json` (FROZEN schema:
path/valid/error/fs_family/stage_done_mask/configured_mask/files_found/
deleted_found/carved_found/bytes_scanned/elapsed_ms/save_unix/label).
Exit: 0 listed (unreadable files are per-row data, never a failure) /
1 enumeration failure.

### c.4 `scan` (rrecov.cpp:1064-1687)
| Flag | Semantics |
|---|---|
| `--disk N` | whole-disk target; driver-only transport (open_disk_view — the grammar path is the only leg, PLAN-DRIVER-ONLY R1). S-051: a whole-disk scan ALWAYS builds its volume targets from the partition table (the og's CDoScanPartition flow — every used non-container entry probed per-target, scanned under per-volume display-name roots; a volume with no readable filesystem is skipped honestly and counted in table_volume_skipped; a table-less disk keeps the single-slice dispatch) |
| `--volume C:` | volume target: BitLocker gate (fvek/dict/auto-unlock), extent map, parent disk, PartitionSlice, optional XTS wrap |
| `--image file` | S-030 image target (.vmdk/.vhd/.vhdx/.img/.dmg/raw; .edisk refused) |
| `--whole-disk` | ADDS lost-partition discovery on top of the always-on table-volume targets (N-044 honest name; needs --disk; S-051's reconciliation drops discovery candidates whose start LBA matches an already-scanned table volume — no double scan) |
| `--deep` | deprecated alias of --whole-disk with a note (rrecov.cpp:1256) |
| `--quick-only` | disables deep + carve (deep is ALWAYS on otherwise) |
| `--no-carve` | disables carve stage |
| `--threads N` | 0 = auto (hw-1, clamp 1..8); 1 = single-threaded |
| `--out file.crsf` | save session (atomic tmp+rename) + stage-boundary auto-checkpoints |
| `--autosave-dir D` | W1c (R-52): the completion auto-save directory (the og's auto_<vol>_<ts> SaveScan flow, rsf-format section 5.5) - on a COMPLETED run only; the N-061 naming; never a cancelled run |
| `--resume file.crsf` | resume (mask truth, boot-hash identity warning, image-path warning, stale-candidate drops) |
| `--log file` | N-048 detailed run log |
| `--json` | one-line machine summary `{"files":..,"deleted":..,"carved":..,"lost_partitions":..,"bytes_scanned":..,"elapsed_ms":..}` |
| `--csv` / `--filter s` / `--deleted-only` | tree print projections (frozen csv grammar) |
| `--repair` / `--repair-apply` / `--repair-undo file` | S-008 pre-scan repair: dry-run default, --apply = consent, undo persisted |
| `--check` | M-041 pre-scan damage audit (never aborts) |
| `--bitlocker-fvek hex` | M-033 in-stack XTS decrypt (64/128 hex) |
| `--bitlocker-dict [file]` | N-064 consented dictionary unlock (bare = shipped 228; file = og-format loader) |
| `--user-signs file` | M-048 EUUserData JSON signature install (honest failure) |

Exit: **0** ok / **1** failure (unknown option, no target, open failure, scan rc≠OK,
session save failure) / **3** cancelled (session saved when --out/--resume given,
rrecov.cpp:1616). Pre-pass flags may appear anywhere (rrecov.cpp:1073-1104).

### c.5 `tree` (rrecov.cpp:1744-1795)
`--in file.crsf` (required), `--filter substr`, `--deleted-only`, `--csv`, `--json`
(frozen G-12 schema `{"total":N,"rows":[{path,name,size,modified,origin,deleted,confidence,category}]}`,
print_tree_json rrecov.cpp:1702). Exit 0/1.

### c.6 `recover` (rrecov.cpp:1801-2022)
`--in file.crsf` + (`--disk N` | `--image file`) + `--to DIR` (all required);
`--filter s`, `--deleted-only`, `--report file.json` (G-09; includes S-023 ScanAnalysis).
Source opened BEFORE session load (M-042 revalidation). Destination overlap → per-item
`ERR_DEST_OVERLAP` refusals; `[F12] WARNING` when dest == source disk (rrecov.cpp:1891).
Exit: **0** / **1** hard failure / **2** partial (item errors or hash mismatch,
rrecov.cpp:1962). Destination-full is completed-with-failures, not abort (N-015).

### c.7 `health` (rrecov.cpp:2027-2136)
`--disk N` required. Direct `\\.\PhysicalDriveN` GENERIC_READ|WRITE open
(rrecov.cpp:2051-2053) with elevation-aware messaging. SMART → NVMe → SSD heuristic →
TRIM. Exit: 0 / 1 (open/flag) / **3** every source unavailable (FB-H04 fix).

### c.8 `fix` (rrecov.cpp:2674-2880)
`--disk N` (required, even for undo-apply — F-B-4), `--part P` (slice view; A-002 disk-absolute
rebase), `--apply` (consent), `--undo file`, `--undo-apply file` (`--no-rda` is REMOVED —
driver-only, rrecov.cpp:2470).
Dry-run default (plan print). Exit 0 (incl. REFUSED/clean) / 1 (usage) / 2 (analyze/execute/undo failure).

### c.9 `apply` (rrecov.cpp:2375-2672)
`--disk N --session file.crsf` required (`--in` alias accepted); `--candidate N` (1-based; required once >1),
`--apply` (consent), `--undo file` (`--no-rda` is REMOVED — driver-only), `--boot-fix old-lba`,
`--boot-fix-bcd path` (M-040 post-apply BCD+NVRAM repair, advisory failures).
Exit 0 (incl. dry-run/REFUSED) / 1 (usage) / 2 (open/plan/execute failure).

### c.10 `bench` (rrecov.cpp:2138-2339)
`--disk N` required; `--mode seq|rand|open|read|info` (default seq), `--mb N` (≤65536),
`--block-kb N` (4..16384), `--iterations N`, `--seed N`, `--json` (`--no-rda` is REMOVED —
driver-only). Transport label is "rda" only — the grammar path is the one leg
(PLAN-DRIVER-ONLY R10, rrecov.cpp:2195). Exit 0 / 1 (usage/unknown mode)
/ 2 (open failure, zero hits, read failure).

**Doc-drift note (HISTORICAL, both halves since fixed):** docs/API.md once listed
`rrecov info --disk N | --image file` — no `info` verb ever existed ("info" is only a
bench MODE); that line was removed and API.md's scan usage line gained
`--image`/`--deep`/`--repair-undo` (this lane, the three-site CLI truth pass). CLI
usage text in-binary remains the truth; the wmain dispatch is rrecov.cpp:2787-2808.

---

## (d) Cross-cutting contracts

### d.1 Error classes — the FULL enumerated list (include/cr/cr_error.h:25-86)

| Family | Code | Symbol | Meaning (err_text, cr_error.cpp:10-83) |
|---|---|---|---|
| — | 0x00000000 | `OK` | success |
| open/transport 0x5002 | 0x50020001 | `ERR_OPEN_FAILED` | open failed |
| | 0x50020002 | `ERR_READ_FAILED` | read failed |
| | 0x50020003 | `ERR_WRITE_FAILED` | write failed |
| | 0x50020004 | `ERR_RANGE_OUT_OF_BOUNDS` | range out of bounds |
| | 0x50020005 | `ERR_CANCELLED` | cancelled |
| | 0x50020006 | `ERR_RETRY_EXHAUSTED` | retries exhausted |
| format/parse 0x5003 | 0x50030001 | `ERR_BAD_MAGIC` | bad magic |
| | 0x50030002 | `ERR_BAD_CHECKSUM` | bad checksum |
| | 0x50030003 | `ERR_BAD_PARAMETER` | bad parameter |
| | 0x50030004 | `ERR_TRUNCATED` | truncated data |
| | 0x50030005 | `ERR_FIXUP_FAILED` | USA fixup failed |
| | 0x50030006 | `ERR_CYCLE_LIMIT` | chain cycle limit |
| | 0x50030007 | `ERR_DEPTH_LIMIT` | depth limit |
| | 0x50030008 | `ERR_UNSUPPORTED` | unsupported |
| | 0x50030009 | `ERR_NO_MEMORY` | out of memory |
| session 0x5004 | 0x50040001 | `ERR_SESSION_FORMAT` | session format error |
| | 0x50040002 | `ERR_SESSION_IO` | session I/O error |
| | 0x50040003 | `ERR_SESSION_VERSION` | session version unsupported |
| writer 0x5005 | 0x50050001 | `ERR_DEST_OVERLAP` | destination overlaps source |
| | 0x50050002 | `ERR_DEST_UNWRITABLE` | destination not writable |
| | 0x50050003 | `ERR_HASH_MISMATCH` | hash mismatch after write |
| | 0x50050004 | `ERR_DEST_FULL` | destination full |
| | 0x50050005 | `ERR_DEST_NAME_EXHAUSTED` | collision rename cap (og 10000) |
| FVE 0x5006 | 0x50060001 | `ERR_FVE_LOCKED` | volume locked (FVE) |
| health 0x5007 | 0x50070001 | `ERR_HEALTH_UNAVAILABLE` | health data unavailable |
| repair 0x5008 | 0x50080001 | `ERR_REPAIR_REFUSED` | repair refused by policy |
| | 0x50080002 | `ERR_REPAIR_UNDO_FAILED` | repair undo failed |
| | 0x50080003 | `ERR_VOLUME_LOCK_FAILED` | volume busy, quiesce refused |
| | 0x50080004 | `ERR_APPLY_CONSENT_DENIED` | apply consent denied |
| | 0x50080005 | `ERR_APPLY_STALE` | table changed since planning |
| | 0x50080006 | `ERR_APPLY_REFUSED` | apply refused by policy |
| | 0x50080007 | `ERR_APPLY_REFRESH_FAILED` | applied but refresh tail failed |
| image 0x500A | 0x500A0001 | `ERR_IMAGE_CORRUPT` | image container structurally corrupt |
| internal 0x5009 | 0x50090001 | `ERR_INTERNAL` | internal error |

**Driver-condition representation today:** there is NO code for driver-absent /
not-started / version-mismatch / unauthorized. Absent/stopped/forbidden all collapse
into "rda_available probe CreateFile failed" → silent direct fallback (the GLE is printed
by the CLI or available via `rda_available`'s `*last_error`); an errored driver grammar
open collapses into `ERR_OPEN_FAILED` 0x50020001; read-time driver failures collapse into
`ERR_READ_FAILED` 0x50020002 (with the ERROR_INVALID_HANDLE self-heal first,
cr_transport.cpp:252-261/314-323). The ABI advisory `cr_ui_last_transport_error` carries
the surviving leg's cr::Err code (M-030). See CORE-DISK-ACCESS.md §(g).

**Error-context attribution (M-049):** `errmod::Module` ids 0..12 (kNone, kOrchestrator,
kNtfs, kFat, kExfat, kExt4, kBtrfs, kHfsplus, kRefs, kZfs, kCarve, kLost, kLdm —
cr_errmod.h:50-64; ABI mirrors CR_ERRMOD_* cr_ui.h:267-283; ops op_none/op_read
cr_errmod.h:68-69). Every ErrorRange event carries module/operation/attempts.

### d.2 Lifecycle states

- **Scan**: created → attached → running (run()) → {ok | cancelled} → save/snapshot →
  closed. Double-run refused ERR_BAD_PARAMETER (cr_ui.cpp:1752-1760). Resume: loaded →
  mask-validated → controller continues (cr_scan.h:493-507).
- **Stages**: quick → deep → carve with `stage_done_mask` bits 1/2/4 stamped from ACTUAL
  completion (cr_scan.h:447-449; resume truth).
- **Recovery**: created → destination set → enqueued → process() → {done | cancelled |
  dest-full} → report → closed.
- **Session**: Writer open → add_record* → finalize (chain patch); Reader index → records;
  corrupt records skipped forward (F10/G-01..G-03 salvage semantics).

### d.3 Threading model

- run()/process() on caller's thread; pause/resume/cancel/telemetry/snapshot/busy/stopping
  from any thread (cr_ui.h:47-51; ABI-CONTRACT.md:55-70).
- Events + progress may fire from ENGINE WORKER THREADS; core does not serialize
  deliveries; sinks must not block (cr_scan.h:109-121; cr_ui.h:127-136).
- Cancellation is cooperative: engines poll `CancelToken::checkpoint()` at loop heads;
  **transports do NOT poll the token** (cr_device.h:136-147 honest scope note); bounded
  response = one loop iteration + one in-flight read. PausableToken parks waiters
  (cr_device.h:168-234).
- `threads = 1` = deterministic single-threaded scan; 0 = hw-1 clamp 1..8
  (cr_scan.h:192).

### d.4 Persistence — CRSF session format

Own container (D-01): magic "CRSFMT01", version 1, 512-byte units, 64-byte header,
24-byte record headers, ≤4096-byte zlib chunks (stored-when-not-smaller),
`CRSF_FLAG_ABS_EXTENTS` (F-1 absolute basis) (cr_crsf.h:30-42). Record roles: session
state, volume info (identity card incl. N-066 axes + S-030 image path), partition
context, disk layout (M-042), result tree, read-error map, free map (S-009), assistant
stats, fs-check report, lost candidates. Per-record CRC32, skip-forward on corruption,
atomic saves (tmp+rename; N-014/N-060). CLI: `--out` + auto-checkpoints at stage
boundaries via CheckpointSink (cr_scan.h:272-299); `--resume` revalidation (M-042).

### d.5 Configuration

- C++: `ScanConfig` (cr_scan.h:185-315), `WriterConfig` (cr_safewriter.h:98-111),
  `CarveFlags` (cr_carve.h:59), `RetryPolicy` (cr_device.h:96-108), `Caps` (cr_preview.h:108).
- ABI: `cr_ui_scan_config` (zero-init defaults; stage_mask 0=7; weights; threads;
  identity strings; checkpoint_path; progress; pre_scan_repair+consent; memory_warn_mb;
  raw_category_name; pre_scan_check — cr_ui.h:429-475), `cr_ui_recovery_config`
  (read_chunk/rename_attempts/skip flags/conflict/progress — cr_ui.h:1114-1122),
  `cr_ui_query` (cr_ui.h:841-856).
- CLI: PARAMETERS.md registry; every flag listed in §(c).
- Transport config: service name (hardcoded `L"RDA"` at every call site;
  the CLI `driver --service NAME` overrides for the status surface only).
  No `prefer_rda` bools, no `--no-rda` flags (both REMOVED with the
  driver-only architecture). No env vars, no config files.

---

## (e) Driver-related surfaces that exist today

1. **`win::rda_available(service, last_error)`** (cr_transport.h:110-114;
   cr_transport.cpp:441-457) — presence probe, GLE out. The only driver-status query
   primitive in the core.
2. **`cr_ui_device_transport`** — "rda"/"direct"/"image" (cr_ui.h:355-357).
3. **`cr_ui_last_transport_error`** — M-030 surviving-leg error (cr_ui.h:283-295).
4. **CLI `[transport]` lines** — the CLI prints probe GLE / open error / chosen path
   (rrecov.cpp:317-340) and bench's `transport` field (rrecov.cpp:1820).
5. **`win_pe_environment`** gate (cr_transport.h:116-124) + `cr_winenv.h` pins.
6. **Wire contract documentation** (cr_rda.h): identity IOCTLs 0x60002400/04/08,
   `kIoctlGetInfo` 0x6002440 / `kIoctlOpenTarget` 0x6002444 / `kIoctlGetVersion`
   0x6002448, `DriverInfo` (major/minor/flags/max_disks/ioctl_base), `OpenTargetIn`,
   PID-token transform, the 18-code session protocol, F11 scale — all mirrored,
   static_assert-frozen, **and issued by nothing** (N-041 deleted the client;
   cr_rda.h:77-82, 109-114).
7. **Bench lane** (bench/ours/verify-driver.ps1, run-alignbench.ps1; DRIVER-UTILIZATION
   pinning addendum) — CI-side driver install/open verification per run; release pin
   v0.2.2, floor >= v0.2.1 (v0.2.0 fails every bare \\.\RDA open with error 87 —
   silently disables the transport via the probe).

---

## (f) Gaps vs. a "driver status" surface (factual notes)

- **No version query anywhere**: GET_INFO/GET_VERSION structs exist (cr_rda.h:46-48,
  61-73) but no core code issues them; driver-version-mismatch is undetectable from the
  core today (the only mitigation is the CI release pin + the v0.2.0 error-87 probe
  failure mode, which is indistinguishable from absent).
- **No loaded/started/authorized distinction**: the probe collapses stopped service,
  SDDL-denied open, and missing driver into one boolean + a Win32 GLE.
- **No driver-health surface**: nothing queries driver error state; read failures are
  transport-agnostic `ERR_READ_FAILED`.
- **No ABI event** for transport choice/degradation (events are scan-scoped; the
  transport decision happens at open time before any scan handle exists).
- **No session/config field** records which transport a scan used (the CLI prints it;
  the ABI exposes it per-device only while the device handle lives).
- **Inconsistent PE-gate application**: `open_disk_impl` gates on
  `!win_pe_environment() && rda_available` (cr_ui.cpp:1157) but `open_volume_impl` /
  `open_volume_with_key_impl` probe `rda_available` only (cr_ui.cpp:1224-1227,
  1340-1343); the CLI's `open_disk_view` (rrecov.cpp:318) has no PE gate at all while
  `win::open_disk` (cr_transport.cpp:505) does — three call sites, two policies.
- **`docs/API.md:41` lists a `rrecov info` verb that does not exist** (bench `--mode
  info` is the only "info" surface) — doc drift to fix when touching API.md.

---

## Appendix: directory inventory (one-line classifications)

- **diff-to-fix/** (344 files): the session-2..38 RE-parity audit trail — README/SUMMARY/
  PARAMETERS/SEVERITY-SCALE/CROSS-REF-MATRIX/CORPUS-ERRATA protocol docs;
  modules/ (27 per-module parity worksheets); findings/FIXED/ (223 resolved finding
  reports: N-/S-/M-/I-/A-/OQ- families); tracked-open/ (1 open item OQ-S4-3);
  evidence/ (87: decompiled-quotes/ corpus citation notes, executions/ probe scripts +
  outputs, s3-agent-logs/).
- **worklog/** (47 files): saved per-session prompt markdown files (the chronological
  record of the diff-audit / diff-fix-N / core-api / parity sessions).
- **bench/** (29 files): the Windows benchmark harness — baseline.json +
  baseline-candidate.json + root-causes.json (metric baselines); metrics/ (python
  compute/merge/regression-gate + VLM crosscheck); ours/ (run-ours / verify-driver /
  run-behavior / run-alignbench ps1); og/ (FlaUiProbe C# UI-automation probe +
  run-og/og-driver ps1); scenarios/apply-scenario.ps1; seed/generate_seed.ps1;
  volume/volume.ps1; vlm-evidence/ (jsonl screen-capture extractions);
  tools/pe_info.py; README.md.
- **fuzz/** (18 files): standalone libFuzzer harnesses — one per parser surface
  (fuzz_ntfs/fat/exfat/ext4/btrfs/zfs/refs/hfsplus/carve/image/preview/reghive/
  engines/pure/container) + fuzz_parser_common.h + Makefile (pkg-config standalone
  build) + corpus/pure/ seed file.
- **tests/** (73+ files): tests/unit/ (62 gtest suites, ~1000 TEST cases: engine
  end-to-end fixtures per FS, orchestrator/session/pause-resume/resume-revalidation,
  transport logic pins, directalign, enumerate merge + win pins, destio win,
  bitlocker win + xts, health csmi/grade/nvme, carve 82, query, preview, report,
  apply/bcdfix/nvramfix, cr_ui 34, ui_abi_check.c plain-C consumer);
  tests/win-stub/ (7 headers: DECLARATION-ONLY Win32 SDK stubs so the Linux lane can
  `g++ -fsyntax-only` the whole src/win layer — the M-055 parse gate);
  tests/data/ (qemu-disk1.vhd/.vhdx/.vmdk images + exfat upcase-table binary).
