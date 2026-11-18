# PRODUCT-STATE — cross-repo harvest session baseline
> Agent A (Task 2-A), Phase-0. READ-ONLY audit at HEAD ea4b8684 (worklog-only commits after 201100aa code state), VERSION 0.9.5, date 2026-11-19. Prior perf-lens sheet archived at worklog/context-sheets/PRODUCT-STATE-perf-2026-11-18.md. Every claim cites file:line; UNVERIFIED marks unknowns.

## 1. Architecture (module map + ABI + UI + CI)

### 1.1 Repo layout, version, build
- Top level: `engine/` (C++20 core, CMake + vcpkg, MIT), `ui/` (React 19 + Vite frontend),
  `ui/src-tauri/` (Rust bridge + `it/` integration-test crate), `installer/` (NSIS + devsign +
  portable), `contracts/` (boundary-truth gate), `tools/` (forensic, validation,
  gen_abi_layouts.py), `docs/`, `worklog/`, `build.ps1`, `VERSION`.
- `VERSION` = `0.9.5`. `engine/RELEASE-NOTES.md` head = v0.7.1 notes (F12 recall-0 fix, N-044
  comment-swallow fix). ABI v4 = the driver-removal era (`engine/docs/ABI-CONTRACT.md:778+` §9).
- Engine build: `engine/CMakeLists.txt` (C++20, MSVC `/W4 /WX`, Win7 SP1 target macros); zlib +
  lz4 + zstd via vcpkg (`engine/vcpkg.json`). Targets: static `cr_core`
  (`engine/src/core/CMakeLists.txt`), `cr_win` (`src/win/CMakeLists.txt`), CLI `rrecov`
  (`src/app/CMakeLists.txt:1`), SHARED `cr_ui` (`src/ui/CMakeLists.txt:11` — links cr_core +
  cr_win; the MSVC export tail `src/ui/cr_ui_exports.def` carries exactly ONE symbol, the
  cr::log set_sink seam the shim's own test consumes).
- CLI verbs (`engine/src/app/rrecov.cpp:3991-4009`): `scan`, `tree`, `archive-list`, `recover`,
  `health`, `unlock`, `fix`, `apply`, `image`, `bench` (rrecov.cpp is 4,012 lines).
- Product is USER-MODE ONLY: no kernel driver, no service (release.yml header; direct
  `\\.\PhysicalDriveN` under an admin token; unelevated raw-disk verbs refuse
  ERR_ELEVATION_REQUIRED 0x500C0001 — CAPABILITY-MATRIX.md §0 row 1).

### 1.2 Engine module map
- 98 .cpp files under `engine/src`, 93 headers under `engine/include/cr`. 28 core modules
  (verified via find): apply, bcdfix, btrfs, carve, check, compress, crypto, err, exfat, ext4,
  fat, health, hfsplus, lost, ntfs, nvramfix, orchestrator, preview, query, recovery, refs,
  reghive, repair, session, storage, table, tree, zfs.
- `src/win/` (12 files): cr_transport (1,517), cr_health (632), cr_health_wmi (309),
  cr_bitlocker (575), cr_enumerate (328), cr_bootfix (311), cr_image_create (225),
  cr_destio (225), cr_fsck (121), cr_image_win (119), cr_mountpoints (91), cr_elevate (24).
- Largest core files: carve/cr_analysers.cpp 7,392; orchestrator/cr_scan.cpp 3,300;
  ntfs/cr_mftwalk.cpp 2,395; hfsplus 1,815; zfs 1,809; storage/cr_image.cpp 1,647;
  ext4 1,553; refs/cr_refs_engine.cpp 1,620; table/cr_ldm.cpp 1,591; carve/cr_registry.cpp 1,897;
  ui/cr_ui.cpp 5,356.
- Storage layering: `IStorageDevice` (include/cr/storage/cr_device.h) → `SliceShim` /
  `PartitionSlice` (cr_slice.h) → `RetryingDevice` + ReadErrorMap (cr_retry.h) →
  `CachedReader` 8 MiB / 256 KiB blocks (cr_cached.h); `FreeMap` (cr_freemap.h, S-009);
  `DecryptingDevice` in-stack XTS-AES (cr_decrypt_device.h, M-033); 64 KiB direct-I/O envelope
  `io::direct_envelope` (cr_winenv.h; used at src/win/cr_transport.cpp:247).
- Error model: single numeric space `include/cr/cr_error.h` (0 = OK,
  ERR_CANCELLED 0x50020005); per-module attribution `cr_errmod.h` (M-049); logging `cr_log.h`
  with a set_sink seam. The overlapped IOCP read-ahead ring is the transport's latency-hiding
  (cr_transport.h docblock, PERF-LEVERS F2/F10 — prior perf sheet's focus).

### 1.3 The cr_ui C ABI (include/cr/cr_ui.h, 1,760 lines; src/ui/cr_ui.cpp, 5,356 lines)
- **95 exported function declarations** (rg count of `CR_UI_API` minus 3 macro lines);
  **CR_UI_ABI_VERSION = 4** (cr_ui.h:81). Rules: C11, no C++ types, every entry catch-all →
  CR_UI_ERR_INTERNAL, u32 returns in the cr_error space, NUL-terminated UTF-8 in, two-call
  size pattern / `cr_ui_string_free` out, zero-initialized configs always safe
  (docs/ABI-CONTRACT.md §1).
- Domain sections (header line anchors):
  - version & diagnostics :101 (cr_ui_abi_version / last_error / string_free)
  - typed events :132-210 — CR_EVT_SCAN_STAGE 1, SCAN_PROGRESS 2, FILES_FOUND 3,
    ERROR_RANGE 4, CHECKPOINT 5, LOST_CANDIDATE 6, REPAIR 7, HEARTBEAT 8 (shim-side),
    SAVE_PROGRESS 9 (shim-side), FS_CHECK 10, SESSION_LOAD_PROGRESS, AUTOSAVED
  - devices :314 — open_image (S-030), open_disk, open_volume, open_volume_with_key (FVEK,
    M-033), unlock_volume_with_dictionary (N-064), device_geometry/transport/close
  - enumeration :399 — enumerate_disks / disks_ex / volumes (win layer; ERR_UNSUPPORTED on Linux)
  - scan lifecycle :482 — scan_create/attach_device/run/pause/resume/cancel/busy/stopping,
    telemetry, get_stats, assistant (S-023), checkpoint_count, verify_identity (boot-hash),
    snapshot, save_session / save_cancel (N-060)
  - pre-scan repair consent :497 (S-008); grading :767 (session_regrade);
    lost-partition apply :813 (apply/undo_last/save_undo, S-029)
  - sessions :915 — load/close/resume_last (M-154 pointed), resume, get_info/stats/snapshot,
    list (W1a), load_with_events (W1b), last_autosave_path
  - snapshot enumeration :1060 — node_count / partial / node / dfs
  - query :1118 — query_run / rows_free / apply_buckets (G-12)
  - carve-format registry :1207 + per-format tally :1236 (N-140);
    user-sign database :1257 (M-048; register/count/info)
  - disk map :1287 (GAP-S6-11); preview :1338 (snapshot/session open, read, metadata,
    unreadable); archive inventory :1380 (M-116 browse-inside); thumbnail :1413 (M-102)
  - recovery :1509 — create/set_destination/enqueue_node/subtree/snapshot/subtree_snapshot/
    process/cancel/queue_count/get_stats/report_json
  - health :1579 (health_read, health_read_ex2); image create :1690 (cr_ui_image_create/ex,
    D-IMG-3 sector range); fsck :1736 (M-075); bitlocker-off :1754 (M-078)
- Stability policy (ABI-CONTRACT.md §9:778+): ADDING exports / APPENDING struct fields / new
  event types / new overlay kinds need NO bump (consumers must ignore unknowns); a bump is
  required only on removal/rename/field-change/contract change. History: v4 = the one
  sanctioned removal (cr_ui_driver_status with the kernel driver), v3 = cr_ui_user_sign field
  resize, v2 = six cr_ui_smart_report appends.
- The Rust bridge mirrors every struct via generated layouts (`tools/gen_abi_layouts.py` →
  `ui/src-tauri/it/tests/abi_layouts.rs`); the bridge-integration lane links the real DLL and
  fails on drift (`.github/workflows/bridge-integration.yml` header, BF-ABI-01/02).

### 1.4 UI structure (React 19 + Tauri 2)
- `ui/src/connector/contract.ts` (1,225 lines) is the single source of truth. **12 domains**:
  Devices :141, Scan :300, Imaging :546, Maintenance :588, Results :734, Preview :818,
  Sessions :851, Recovery :952, Health :1017, Repair :1056, LostPartition :1098, System :1148.
  Errors carry machine codes + i18n reason keys (contract.ts:49-79 ConnectorErrorCode union).
  ESLint rules forbid connector imports outside src/queries and raw tauri invoke
  (ui/rules/no-connector-import-outside-queries.yml, ui/rules/no-raw-invoke.yml).
- Implementations: `src/connector/mock/mock-connector.ts` (2,713 lines, procedural mock with
  fixtures + node-space) and `src/connector/tauri/tauri-connector.ts` (645 lines).
- `src/queries/` — 13 files, one per domain plus client/dev (client, dev, devices, health,
  imaging, lost-partition, preview, recovery, repair, results, scan, sessions, system).
- Stores (src/stores/): ui-store, scan-session-store, selection-store, filter-store,
  consent-store.
- Features (12 route groups under src/features/): backup, consent, dev-panel,
  device-selection (home-page, sd-card-page, unlock-volume-dialog, create-image-dialog),
  health (health-page, disk-health-window), lost-partition, preview (preview-pane),
  recovery-queue (recovery-page), repair (repair-page), results-explorer (results-explorer,
  filter-bar, results-route, page-fallback), scan-progress (scan-progress-page, sector-map-panel,
  recoverability-card), settings-about (settings, about, activation dialogs). Shell/router/
  providers/theme/error-boundary/exit-guard in src/app/. i18n: en + zh-CN (src/i18n/).
- Tauri bridge: `src-tauri/src/bridge/` (abi.rs, events.rs, state.rs, util.rs, consent.rs,
  logging.rs, error.rs, filefilter.rs, mod.rs) + `src-tauri/src/commands/` (13 files).
  `lib.rs:40-115` registers **53 commands**: devices 3, scan 7, results 4, preview 2,
  sessions 6, recovery 5, health 1, repair 5, lost_partition 4, system 6, imaging 3,
  diagnostics 7, maintenance 2. Startup ABI gate (lib.rs:114-126) logs mismatch and refuses
  per-scan binding. Elevation = launch-time manifest requireAdministrator
  (src-tauri/app.manifest; build.ps1 D1 gate byte-scans the exe). Capabilities: default.json +
  disk-health.json.
- Event chain: engine CR_EVT_* → bridge events.rs → connector callbacks, documented in
  `contracts/event-mapping.md` (141 lines) + `contracts/type-maps.md` (258 lines).

### 1.5 Installer
- `installer/recoverywizard.nsi` (640 lines): NSIS, Unicode true, /SOLID lzma, custom
  privilege page (:192), `RequestExecutionLevel admin` (:174) — ONE UAC prompt; product SW key
  `HKLM\Software\HeicFast\Recovery Wizard` (:139); InstallDir `$PROGRAMFILES64\HeicFast\Recovery
  Wizard` (:158); sections: required app (:203), hidden -post registry/uninstaller (:342),
  Uninstall (:522). Output `RecoveryWizard-<ver>-setup.exe` (:157).
- `installer/devsign.ps1` — ephemeral per-build code-signing cert (nothing committed);
  `installer/portable.ps1` — portable zip. Self-signed Authenticode is an integrity marker,
  not a load prerequisite; production CA signing pending (release.yml header).

### 1.6 contracts/ (boundary truth)
- `contracts/validate.mjs` (134 lines, node) — 6 checks: (1) every cr::Err value in
  engine/include/cr/cr_error.h has a row in error-codes.json; (2) every uiCode exists in
  contract.ts ConnectorErrorCode; (3) every CR_EVT_* define in cr_ui.h mapped in
  event-mapping.md; (4) command-surface.json well-formed (unique names; serves/abiRefs/
  returns-or-args per row); (5) manifest ↔ #[tauri::command] parity (auto-active now that the
  bridge exists); (6) every connector invoke is in the manifest.
- `contracts/error-codes.json` (424 lines), `contracts/command-surface.json` (914 lines).
  contracts-ci runs on every push touching contracts/, engine/include/, ui connector/bridge.

### 1.7 CI lanes (9 workflows in .github/workflows/)
1. **engine-ci.yml** — jobs `lint` + `build-and-test` (windows-2022): clang-format + MSVC /W4/WX
   build + ctest + CLI contract + C-consumer ABI check + Win7-inclusive import gate. Linux
   sanitizer/fuzz lanes are local-only via ci-local.sh --san/--fuzz (header note).
2. **ui-ci.yml** — job `ci` (windows-latest): pnpm typecheck/lint/test + cargo check/clippy
   (no engine kit needed — no link step).
3. **ui-e2e.yml** — Playwright over `vite preview` with VITE_CONNECTOR=mock; 4 spec files /
   15 test cases (ui/e2e/*.spec.ts), step screenshots kept as evidence.
4. **contracts-ci.yml** — job `validate` (ubuntu-24.04): the validate.mjs boundary gate.
5. **bridge-integration.yml** — job `bridge` (windows-2022): builds the REAL engine, links the
   REAL bridge, runs it/ integration tests (abi_layouts.rs + bridge_integration.rs) against
   the real DLL on real disks; generated-files freshness gate first.
6. **release.yml** — job `full-product`: tag-triggered build.ps1 all lanes (contracts → engine
   → ui+tauri → NSIS → portable zip → SHA256SUMS) + the validation stack.
7. **benchmark-src.yml** — job `benchmark` (windows-2022, dispatch-only): measures the
   checked-out commit with the release recipe; scan-wall + extended alignbench
   (QD 1..32 × 64 KiB..16 MiB).
8. **forensic-repro.yml** — job `repro`: reproduces reported installed-release symptoms
   (installer console popups, SMART surface via WebView2 CDP, seeded 64 GB NTFS vdisk scan
   timeline) with captured evidence at every layer.
9. **validation-evidence.yml** — jobs `t14-measurements` (engine latency + bridge filter/
   memory budgets via tools/validation/measure-cr-ui.c) + `t15-installer-matrix`
   (install/uninstall/update/repair matrix on a clean runner, tools/validation/
   installer-matrix.ps1).
All lanes carry concurrency groups, per-job timeouts, and caches (commit 201100aa; plus two CI
fix commits 235d3c32/6a6e05cd at HEAD; worklog/OPTIMIZATION.md + CI-FAILURES.md).

## 2. Capability inventory by domain

### 2.1 Partition detection & lost-partition recovery — EXISTS (deep)
- Table parsing `engine/src/core/table/cr_table_parse.cpp` + header:
  - MBR: signature 0xAA55, 4 entries, extended-chain (0x05/0x0F) EBR walk with visited-set
    cycle guard; torn-chain SALVAGE — walked prefix kept, EbrChainStatus Ok/Partial/Failed +
    ebr_break_lba (M-002; cr_table_parse.h:56-68).
  - GPT: primary + backup headers, entry-array CRC32 (table/cr_crc32.cpp), protective MBR,
    per-entry span-wrap gate → gpt_invalid_entries (N-161), backup salvage from device-derived
    LBA when the primary is damaged (GAP-W2-19 closed 3267d26).
  - FsKind enum (cr_table_parse.h:37-53): Unknown/Ntfs/Fat12/Fat16/Fat32/Exfat/Refs/Ldm/
    Extended/Protective/Reserved/Linux/Other/EfiSystem. PartitionInfo carries gpt_attrs +
    UTF16 name[36].
- **LDM full decode** `table/cr_ldm.cpp` (1,591 lines): PRIVHEAD/TOCBLOCK/VMDB validation
  (copies at the spec LBAs), complete VBLK object graph incl. composite multi-fragment
  records, dynamic-volume reconstruction simple/spanned/striped/mirrored/**RAID-5**
  (left-symmetric rotation — og-parity match compared at cr_ldm.cpp:1346-59); damaged VBLKs
  counted+skipped (LdmStats); spanned volumes surface as read-only virtual devices; cross-disk
  member resolver `LdmDiskResolver` wired into ScanConfig (cr_scan.h) — empty = single-disk
  semantics. Multi-disk recovery basis persisted as REC_LDM_LAYOUTS (S-052).
- **Lost discovery** `lost/cr_candidates.cpp`:
  - discover() :942 — whole-disk stride in 64 KiB WindowScanner windows (:33), 512-byte
    geometry fallback (M-137), read-error tolerant.
  - Pass 1a boot-sector families (55AA + OEM gated, cr_candidates.cpp:993-1035): "NTFS    "
    → probe_ntfs :78; "EXFAT   " → probe_exfat :243; "EFI PART" → probe_gpt_header :326
    (backup GPT header candidates included); "ReFS" OEM → probe_refs :530 (page-signature
    window); "-FVE-FS-" → probe_bitlocker :289 (S-019, must run before the FAT catch-all);
    else FAT family → probe_fat :165 (BPB validation).
  - Pass 1b superblock families (:1038-1060): ext @+1024 (probe_ext :380, in-window),
    HFS+ @+1024 (probe_hfsplus :457), btrfs @+64 KiB (probe_btrfs :485) and ZFS @+128 KiB
    (probe_zfs :502) — out-of-window reads gated on 1 MiB grid alignment (PARITY-DELTAS D-50).
  - Candidate model (cr_lostpart.h:47-141): CandidateFs 11 values (Ntfs/Fat/Exfat/GptTable/
    Unknown/Ext2/HfsPlus/Btrfs/Zfs/Refs/BitLocker — wire-stable); dual extent (whole-volume +
    data-area, N-035); **og content scoring**: score = ln(count)/ln(max_count) permille over
    weighted file/folder counts + per-FS bonus + 500 table-match (M-017; score_ratio :705,
    weighted_count :695, fs_bonus :836, table_match :857, content_counts :745); structural
    points as tie-break; Evidence vector of tagged points.
  - Three-source assembly (M-118): signature sweep + live-table feed + Explorer MountPoints2
    `CPC\Volume` register history (Provenance::Register; decode_mountpoints2_placement :898;
    host-fed via ScanConfig.register_history_lbas — the win CLI fills from HKCU, offline via
    reghive::mountpoints2_placements).
  - Geometry payload bps/SPC/hidden captured by BPB probes (M-042); device-tail fallback size
    flagged size_from_device_end and EXCLUDED from containment dedup (GAP-S6-6 fix).
  - Post-discovery: dedup pass-2 (first-insert-wins); materialize() :581 → read-only
    PartitionSlice for the normal FS engines; revalidate_candidate :591 (resume-time drop of
    stale candidates, cr_scan.cpp:1247-1258).
- Orchestrator integration: whole_disk mode discovers in the deep stage (cr_scan.cpp:2314+),
  S-051 no-double-scan reconciliation vs already-scanned table targets (:2323+), M-137
  insertion cap of 10 scan targets (lost_candidates_inserted, cr_scan.h), parallel candidate
  scans. exFAT content-consistency scorer (lost/cr_consistency.cpp, M-035): majority-checkable
  + strict ratio < 0.3 → reject (candidate stays listed, files detached,
  lost_content_rejected stat).
- Edges: UDF has NO lost probe (classification only in probe_family, cr_scan.cpp:1101-1122,
  M-044 — carve-only on both sides); GPT-damaged layout is TableLayout::GptDamaged (reported,
  not reconstructed in-place — reconstruction is the apply module's consented MBR path only).

### 2.2 Filesystem engines — per-FS
- **NTFS** — see §2.3 (deepest domain).
- **FAT12/16/32** (`fat/`, 1,967 lines): FatTable with mirrored-FAT choose-healthy
  (CDoubleFATTable parity); chain walker with visited-range cycle guard, whole-run batched
  reads; LFN VFAT slot assembly before the 8.3 entry with checksum validation; **deleted-slot
  recovery** — deletion 0xE5 destroys the 8.3 first byte only, LFN slots intact, checksum
  brute-force recovers slot-set validation AND the short-name first char
  (lfn_deleted_checksum_recover, cr_lfn.cpp:76 — GAP-S6-1); CP437 boundary decode N-087;
  **deleted-chain reconstruction** — contiguous-candidate chain bounded by expect_clusters,
  validated cluster-by-cluster against FAT free-state, reused cluster cuts the run, graded
  PartialRisk (reconstruct_deleted_chain, cr_chain.cpp — GAP-S6-2); cancel polling at loop
  heads (GAP-W2-9).
- **exFAT** (`exfat/`, 1,501 lines): checksum32 ROL31 boot-region validation (main + backup),
  16-bit entry-set checksums; deleted-set handling with the **parked-set reconstruction
  oracle** (M-013: sets with destroyed validation parked during the walk, then same-shape
  head/tail combinations re-tried; reconstruct_parked_sets at cr_exfat_quick.cpp:1020,
  sets_reconstructed stat); upcase-table fixture (tests/data/exfat/).
- **ext2/3/4** (`ext4/cr_ext4_engine.cpp`, 1,553 lines): superblock gate (0xEF53@+0x38,
  block-size 1024<<log, 64-bit block count, group math); group descriptors 32/64-byte; inode
  tables; extent trees (0xF30A) with indirect-block fallback (ext2/3); directories linear +
  hashed treated as linear streams; live tree + **deleted-inode pass (i_dtime set)** graded
  against group block bitmaps; **journal salvage** (descriptor chain → directory-looking
  blocks); NODE_FLAG_SIZE_EXCEEDS_BLOCKS 0x200 truncation marker (N-177); large-volume
  (m144) and journal-inode-image (m143) tests.
- **ReFS** (`refs/`, 1,723 lines): full MSB-chain reconstruction engine (cr_refs_engine.cpp
  1,620 lines): VBR ("ReFS"@+3, "FSRS"@+0x10), superblock page at cluster 30 + backups in
  2nd/3rd-last clusters, checkpoint pages with **body-minor gate + dynamic body offset**
  (GAP-S6-3 fix — real Server-2022 3.7 volumes open), minstore B+ index pages, directory
  tables (root = fixed table ID 0x600; rows 0x10 dir descriptor / 0x00010030 file /
  0x00020030 dir link / 0x80000020 ID2 moved-deleted name evidence), attributes (0x80 $DATA
  embedded data-run table with startLCN/relVCN/numClusters, checksummed flags 0x80/0x100);
  deleted harvest via ID2 evidence + flags-4 deleted entries. Detection header
  cr_refs_detect.cpp.
- **Btrfs** (`btrfs/`, 1,360 lines): mirror superblock select (max generation); sys chunk
  array → full logical→physical chunk map → chunk-tree walk → root-tree discovery
  (self-validating) → per-subvolume FS trees (inode, dir_item/dir_index, extent_data regular
  + inline); subvolumes as named containers + default-subvolume pick (DIR_ITEM name "default"
  → ROOT_ITEM); **M-020 chunk profiles**: DUP/RAID1/SINGLE through rotating-mirror index;
  RAID0/RAID10 stripe math = REGISTERED SUPERSET (the og's read path fails both); only the
  NEWEST-generation tree walked (og parity, AUDIT.md row 20); btrfs_cache extent cache.
- **ZFS** (`zfs/cr_zfs_engine.cpp`, 1,809 lines): single-vdev pools; 4 vdev labels (0, 256K,
  end-512K, end-256K); XDR nvlist config; uberblock ring txg newest-wins; MOS objset →
  object directory ZAP → dataset directory chain → per-dataset objset_phys → master-node ZAP
  → ZPL directories (micro/macro ZAP any nlevels) → dnode bonus (SA layout 1 + legacy
  znode_phys); full indirect-block ladder, embedded block pointers, gang blocks; compressed
  blocks lzjb/lz4/zle/gzip/zstd tagged on the extent for writer-time decode (codec/psize on
  Extent, cr_node.h:129); big-endian pools refused ERR_UNSUPPORTED; multi-vdev pools detected
  and reported unsupported; damaged reads counted, never zero-filled.
- **HFS+** (`hfsplus/`, 1,815 lines): volume header @1024 BE ("H+"/"HX"), catalog B-tree
  (CNID 4) leaf chain, cnid-parent hierarchy, data + resource forks, extents-overflow B-tree
  (CNID 3) for fragmented forks, **deleted pass over freed catalog nodes** (orphaned cnids
  graded against the allocation bitmap).
- **BitLocker** (win/cr_bitlocker.cpp, 575 lines): detect via FVE metadata; DETECT +
  LEGITIMATE WINDOWS UNLOCK ONLY (F13): PowerShell Unlock-BitLocker → manage-bde →
  FVEAutoUnlock registry external keys (live hive AND offline hive — S-014, fed by
  reghive::fveautounlock walk); in-stack XTS-AES decryption with a user-supplied FVEK
  (DecryptingDevice, M-033; `--bitlocker-fvek` CLI / cr_ui_open_volume_with_key ABI); locked
  volumes refuse ERR_FVE_LOCKED; password-dictionary aid (§2.9). No key derivation (excluded).
- **UDF**: recognition only (probe_family AVDP→Main-VDS descriptor probe, cr_scan.cpp:
  1101-1122) — carve-only, matching the og (M-044).
- **Compression layer** (compress/cr_decomp.cpp, 286 lines): LZNT1 (MS-XCA tagged-group —
  GAP-P1-1 LZNT1 half closed), lzjb (source-bounded — GAP-P1-2), lz4/zle/gzip/zstd via vcpkg
  for ZFS/Btrfs extents. XPRESS/LZX NOT implemented (shared limitation with the og,
  CAPABILITY-MATRIX §1 row).

### 2.3 NTFS specifics — EXISTS (the deepest domain)
- **Deleted-record detection** (cr_mftwalk.h docblock + cr_mftwalk.cpp 2,395 lines): magic
  'FILE', mandatory fixup (cr_fixup.cpp), not-BAAD, header sanity; in-use bit → active vs
  deleted; deleted non-dir with size > 0x3FF (STRICT, NTFS-1) gets the viability walk with
  four-tier $Bitmap grading (Excellent/PartialRisk/Overwritten/MetadataOnly — cr_node.h:57-63);
  known-metafile name list subsumes the og-15 + $SI SYSTEM|HIDDEN gate (N-040);
  namespace preference POSIX > Win32+Dos > Win32 > Dos.
- **Two-pass scan parity** (S-012): pass 1 = live MFT walk emitting EVERY record (deleted
  included, destroy machinery OFF — destroy_fully_overwritten defaults false); pass 2 =
  GUID-dump walk over live `{GUID}{3808876b-c176-4e48-b7ae-04046e6cc752}`-named MFT-dump
  files: record stride 0x400, hole stride 512, claim-map viability at each run START cluster,
  destroy on all-claimed / size ≤ 0x3ff / $-name / system (WalkerFlags.guid_dump_pass = true);
  **size-0 rescue**: deleted size-0 records harvest resident $DATA from $MFTMirr (fixup +
  record# validated) and merge a same-name record's runlist + size (size_zero_rescue).
- **MFT corruption recovery**: torn/damaged primary DBR → open from the volume-end backup
  (boot_from_backup, M-043, report-only); record-0 bootstrap from the $MFTMirr LCN
  (boot_from_mftmirr, S-063 — the mirror's record-0 slot is a copy of $MFT's own record whose
  $DATA runlist describes the whole primary MFT); mirror LCN out-of-bounds → mirror_invalid
  demotion (M-141); read_record mirror fallback for records 0..3 (cr_bootstrap.h:60-67);
  fragmented $MFT via $ATTRIBUTE_LIST depth-limited (fix F6); **brute-force remnant scan**
  scan_mft_remnants (cr_mftwalk.h:169-183): strides the WHOLE slice at record_size outside
  the live MFT stream, 4 MiB block-buffered with record-size overlap, thread fan-out; emits
  records WITH content (resident $DATA or runlist extents, PartialRisk — GAP-S6-4 closed)
  and metadata-only for the rest. $MFT record-0 restore from $MFTMirr slot 0 exists ONLY as
  a consent-gated RawFixer action (S6 scope) — there is NO in-scan $MFT run-list
  patching/reconstruction stage.
- **$LogFile analysis**: deliberately NOT adopted — the og's own journal machinery is dead
  weight in its scan path (tables built, freed, never consulted;
  diff-to-fix/findings/FIXED/I-049-ntfs-logfile-dead-machinery-register.md; grep "LogFile"
  in engine/src hits only the metafile name lists cr_mftwalk.cpp:222/251). OQ-S79-1 tracked
  the open question and closed as the I-049 informational register.
- **$UsnJrnl analysis**: harvest_usnjrnl (WalkerFlags) walks $Extend/$UsnJrnl:$J runs,
  V2/V3 record parse filtered on USN_REASON_FILE_DELETE 0x00020000 (cr_mftwalk.h:112-115),
  recovering deleted names the MFT pass missed; damaged runs counted (usn_damaged_runs).
- **MFT slack carving**: no dedicated MFT-slack carver exists. Nearest surfaces:
  scan_mft_remnants (records outside the live MFT, incl. post-format salvage) and the pass-2
  GUID-dump files. UNVERIFIED whether any reference does more (mft-rescue reference is the
  harvest candidate).
- **Orphan/parent-reference handling**: orphan parents → the "File Path Lost" root, grouped
  per dead parent under synthetic LostDir<record> folders (M-103, post-format salvage R2.5);
  hard links: single_link_per_record default TRUE = og parity (one node per record under its
  namespace-best parent; M-007) with opt-in per-parent fan-out; $ATTRIBUTE_LIST chase default
  multi-hop BFS depth-64 cycle-guarded with og-parity depth-1 flag (M-008); named $DATA →
  ADS child nodes (emit_ads).
- Recycle Bin: $I metadata parsed, $R records remapped to original names (two-pass map —
  GAP-P1-3 fixed; WalkStats.recycle_info/recycle_remapped). Runlist codec: width nibbles
  low=length high=offset, signed deltas, sparse runs; **torn-prefix salvage** — a fragment
  torn after ≥1 valid run decodes to its valid prefix with `torn` set → PartialRisk (N-163,
  cr_runlist.h:29-45). LZNT1 compressed $DATA full ladder + MFT-walk inflation + codec-tagged
  extents the writer inflates (GAP-P1-1 LZNT1 closed; cr_decomp lznt1_payload_decode).

### 2.4 Carving (src/core/carve/, 12,248 lines) — EXISTS (two engine layers + user layer)
- Pipeline (cr_carve.h:1-8): 1 MiB sequential windows + 4 KiB overlap, per-format magic
  search, grammar validation, dedup against structured results (F9), 128-bit content hashing
  for ≤ 8 MiB files, host-provided naming callback; session-wide per-prefix name counters
  (CarveNameCounters, S-049 — the og's DRWUI counter map) and per-volume grouping
  (VolumeGroup, S-048 — "<label> (<X:>)" roots).
- **Grammar layer** (cr_signatures.h + cr_registry.cpp): **155 FormatId enum values** (FMT_
  count; appended-only for serialized stability). Table kTable rows carry {magic, ext, offset,
  min/max size, confidence, validator, label}; **load-time fail-closed validation** — a null
  validator rejects the registry (F8; validate_registry at cr_registry.cpp:1723).
  Validators are structural (7,392 lines in cr_analysers.cpp): JPEG SOF gates, ZIP 3-anchor
  LFH/CD/EOCD + CD-ownership + padding (CARVE-2), PST floor, TIFF BE variants (CARVE-3),
  ORF IIRS/MMOR, naked-moov MOV salvage (S-017), ASF structural walk (A-010), GIF/BMP/RTF/
  OLE/MPEG-PS/ES/DWG (S-007), headerless MP3 (12 sync keys) + AAC ADTS frame walkers,
  low-Word-doc/HLP/ICO/CUR/WMF/TTF/MDB/TAR list-layer probes, etc.
- **Raw-sign layer** (cr_rawsigns.h + cr_rawsigns_data.cpp): **kFormatCount = 139**
  (cr_rawsigns_data.cpp:14) header/tail byte patterns with (offset 0..46, confidence, min/max
  in KiB). Semantics: header probed at 512-byte-stride starts; tail walked forward from
  start+min within [min, max); header-only hit = extent min(max, device-end-start) at sign
  confidence; header+tail = (hdr+tail)/2 + 100 bonus capped 1000; tail scan bounded
  kRawSignTailScanLimit 8 MiB then header-only fallback (S-015); confidence field used for
  winner selection = deliberate superset over og dead data (N-030, pinned by tests).
- **User-sign layer** (cr_usersigns.h, M-048/D-104): og EUUserData JSON schema
  `{"<ext>":{"signs":["<offset>,<HEX>"],"mime":...}}`; caps: sign ≤ 48 bytes, ≤ 256 total
  signs, ≤ 12/entry, offset ≤ 1024, ext ≤ 8, mime ≤ 80 (S-039 resize = ABI v3); per-run
  re-registration via cr_ui_register_user_signs (install-tree config semantics, not session
  state); aggregate tally slot in the per-format table.
- Gates + stats (CarveStats, cr_carve.h:50-100): free-map cluster gate (M-005 — used-cluster
  candidate suppression), cross-engine occupancy interval gate (M-039), all-zero-stride
  fast-skip (N-031), unreadable windows NOT counted as scanned (FB-C01), per-format tally
  over the unified id space [grammar | rawsign | user-aggregate] (N-140, 512-slot array,
  exposed via cr_ui_carve_tally*).
- Dedup/merge: (first-cluster, size-class, hash) pre-attach dedup vs structured results AND
  vs prior carves; resume-safe disk-absolute extent-key dedup tree (GAP-S6-8 closed,
  test_carve.cpp:1565).
- Fragmentation: NO generic fragmented-carve reassembly — only format-specific structures
  (ZIP CD anchoring, naked-moov, OLE FAT walk). Grammar-aware aspects are per-format
  validators; there is no declarative grammar engine. This is a prime harvest surface.

### 2.5 Results tree (src/core/tree/) — EXISTS
- Node model (cr_node.h): dual classes — lightweight FileNode + FullNode for roots/virtuals
  (og CFileNode 0x93 / CFileSystemNode 0x290 parity); intrusive parent/child/sibling links,
  arena allocation with doubling arenas (cr_node.cpp:107-135); refcounted FileProperties
  shared by hard links: {record_ref (MFT ref), fs_kind, logical/allocated size, FileTimes,
  attributes, flags, name (u16), short_name (N-011a 8.3), name_raw (N-016b lossless raw
  bytes), resident_data (NTFS resident $DATA / ZFS embedded / Btrfs inline), extents[]
  {offset ABSOLUTE device bytes, length, codec, psize, logical, hole} (S-059 logical-start +
  hole extents), carve_format/carve_confidence, RAW provenance fields (M-051)}.
- Enums: Origin Structured/Carved/Lost/Virtual (:55); DeletedState Active/Deleted/
  PartiallyOverwritten/Damaged (:56); Confidence Excellent/PartialRisk/Overwritten/
  MetadataOnly (:57-63); Grade Low/Average/High (:71). Tree flags: NODE_FLAG_EXT_SHIFTED
  0x8000 (absolute-basis guard), NODE_FLAG_JUNK_FILTERED 0x4000 (M-018),
  NODE_FLAG_SIZE_EXCEEDS_BLOCKS 0x0200 (N-177 ext4 damage).
- Path reconstruction: per-parent attach; "File Path Lost" orphan root with LostDir<record>
  groups (M-103); volume-grouped carve roots (S-048/M-104 tier machinery for whole-disk
  scans); configurable raw-category root name (M-029, resume recognizes both names).
- Deleted/live classification + provenance: in-walk allocation-axis Confidence (NTFS $Bitmap
  grading etc.) PLUS the post-scan **CRecoverability grading pass** (cr_grading.cpp, S-023):
  og decision tree (size < 1 → Low; email category → High sticky; existing-file insert ≥ 1 →
  High; raw/carve device size < 0x400 → High else Low; …), monotonic-max, grade-2 sticky,
  on-demand regrade for grade-1 nodes via cr_ui_session_regrade.
- Snapshot (cr_snapshot.h, G-02): copy-on-write value snapshot under ResultTree::mutex_ —
  one O(n) pass into flat arrays (~150-300 ms at 1M nodes, paid at stage boundaries or on
  demand); SNAP_PARTIAL truth flag; recovery-from-snapshot value-enqueue is race-free
  (G-02b); cr_ui_snapshot_node/dfs enumeration.
- Disk map (cr_diskmap.h, GAP-S6-11): geometry + per-cell state — allocation USED (active
  structured extents only) / FREE (engine bitmap + no claims) / UNKNOWN (nothing invented);
  scan-coverage overlay; found-content density; confidence grading overlay; bad-sector
  overlay. FREE from engine allocation bitmaps (NTFS $Bitmap / FAT / exFAT) persisted
  cell-granular as REC_ALLOC_MAP (S-009). Rendering is host-side by design.
- Query (cr_query.h, G-12): pure layer over one tree — composable predicates, stable sort,
  count-only mode, offset/limit paging; performance contract CI-enforced: filter+count over
  1M nodes < 200 ms; filefilter vocabulary generated (query/cr_filefilter_data.cpp 1,513
  lines; engine scripts/gen_filefilter.py + ui scripts/gen-filefilter.mjs mirror, freshness-
  gated in CI).

### 2.6 Imaging — PARTIAL (rich reader, minimal producer)
- **Reader** (storage/cr_image.cpp, S-030; 1,647 lines): flat raw (any size, 512 logical);
  **VMDK** monolithicSparse + descriptor-with-extent-files (FLAT + SPARSE extents, sparse
  header v1/2/3, grain directory/table translation, differencing chains via
  parentFileNameHint, depth cap 32; streamOptimized refused ERR_UNSUPPORTED); **VHD** fixed +
  dynamic + differencing (footer/dynamic header/BAT, per-block sector bitmaps, W2KU/W2RU
  parent locators); **VHDX** dynamic (header pair, region table, metadata, BAT with
  chunk-ratio SB interleave) with a **read-only in-memory log-replay overlay** (qemu-refusing
  case served from the log region; differencing VHDX refused); `.edisk` refused
  ERR_UNSUPPORTED (registered exclusion); extension-first dispatch with content verification
  (a bad magic is an error, not a silent flat fallback — og parity, DRWUI:948237-948409);
  ImageFileIo injected seam (Win32 impl src/win/cr_image_win.cpp — thread-safe handle table).
- **Producer** (win/cr_image_create.cpp + storage::create_image_stream_range at
  cr_image.cpp:1584-1647): **flat-raw ONLY** (VHD/VHDX writers out of scope, D-147);
  single-pass ascending 64 KiB copy; SHA-256 over the emitted stream; disposition: write to
  `<final>.crimg-tmp` → on success flush + atomic rename + `.sha256` sidecar (`<hex>  <name>`
  form); on cancel/failure close and rename to the truthful `<final>.partial` (kept, named,
  reported; never deleted, never passed off as complete); ranged forms
  create_image_file_range + create_disk_image_ex (D-IMG-3: sector range validated —
  beyond-end refuses ERR_BAD_PARAMETER, never silent truncation); cancel at chunk heads.
- **What the producer does NOT have (the harvest gap)**: no two-pass pass, no bad-sector
  isolation (a read failure mid-stream ABORTS the image with the committed prefix —
  cr_image.cpp:1616-1622), no per-sector error map persisted beside the image, no pause/
  resume of an interrupted create (a `.partial` is kept but cannot be continued). The
  scan-side machinery (RetryingDevice bisection + ReadErrorMap) is not reused here.
- UI: create-image-dialog (features/device-selection) + ImagingDomain commands
  image_create_start/state/cancel (D-IMG-1 closed: caller-owned cancel + sector range).

### 2.7 Health (src/core/health/ + src/win/cr_health*.cpp) — EXISTS
- Transport chain (win/cr_health.cpp): ATA SMART command set 0xB0 (feature 0xD0 READ_DATA /
  0xD1 READ_THRESHOLDS / 0xEC IDENTIFY) issued in the pure seam (core/health/cr_smart_ata.cpp,
  N-032) over three routes: (1) IOCTL_ATA_PASS_THROUGH (ATA_PASS_THROUGH_EX), (2)
  SMART_RCV_DRIVE_DATA 0x7C088 legacy fallback, (3) CSMI CC-0x19 SCSI-miniport pass-through;
  NVMe health via log-page 2 (cr_ui_health_read_ex2 with vendor thresholds + the M-036 v2
  appends: host_reads/writes, controller_busy_time, num_error_log_entries, data bytes
  read/written); WMI layer (cr_health_wmi.cpp); SAT decoder (cr_sat.cpp) + CSMI parser
  (cr_csmi.cpp) pure-core.
- Grading (core/health/cr_grade.cpp, S-024): the og DiskHealth 4-status model
  (Unknown/Good/Caution/Bad): caution counts for attrs 5/197/198, SSD-life caution 10%,
  NVMe critical_warning≠0 → Bad, life ≤ 10 rules; temperature display-only (per-attr chain);
  vendor family decode cascade (cr_family.cpp — incl. the decoded 0x181c display family,
  commit 9ad991d6); SSD model-string heuristics; pure core so Linux CI executes the rules.
- Drive-inspector UX: features/health/health-page.tsx + disk-health-window.tsx
  (DiskHealthWindowApp at features/health/disk-health-window.tsx:107) + lib/disk-health-
  window.ts; capability file src-tauri/capabilities/disk-health.json. NG-5..10 SMART-truth
  fixes landed (v0.9.1 per prior smart sheet); TRIM state via direct fsutil.exe run, no
  shell, 10 s bounded wait (cr_health.cpp:476-527); FVE policy gate (cr_fve_policy.cpp).

### 2.8 Session/persistence (src/core/session/, 1,857 lines) — EXISTS
- CRSF container (cr_crsf.h): magic "CRSFMT01", version 1, 512-byte units, chunked deflate
  ≤ 4096 payload with stored-when-not-smaller, level-5; per-record CRC32 + per-chunk counts;
  skip-forward to the next well-formed record on corruption (F10). Container flag
  CRSF_FLAG_ABS_EXTENTS for the F-1 absolute-extent basis.
- Record types (cr_crsf.h:44-120 + cr_records.h): REC_SESSION_STATE 1, VOLUME_INFO 2
  (identity card GAP-W3-9a + N-066 firmware/WWN/interface + S-030 image path + N-090 100-byte
  label channel), PARTITION_CONTEXT 3 (fs_kind wire-stable), RESULT_TREE 4, ERROR_MAP 5,
  ASSISTANT_STATS 6 (S-023), ALLOC_MAP 7 (S-009), DISK_LAYOUT 8 (M-042 layout-changed flag),
  FS_CHECK 9 (M-041), LDM_LAYOUTS (S-052), CARVE_TALLY (N-140), SCAN_FILTER (M-073).
  Old readers skip unknown types; optional records degrade honestly.
- Checkpoint/resume: stage-boundary CheckpointSink (write/tell/seek + N-014 commit hook —
  file-backed sinks fflush+close+rename so a crash leaves the previous good checkpoint);
  serialized mask is truthful so resume skips finished stages by construction; failed sink
  write/commit ABORTS the scan (never a silent gap).
- Autosave + last-scan: autosave_dir on run() COMPLETION and on the STOP WINDOW (N-211 —
  stopped scans save partial results like finished runs) under session::auto_session_name;
  last_scan.info sidecar with the og's four fields + one-live-auto retention delete (M-154,
  cr_lastscan.h); session list (W1a) + pointed resume + load-with-events + load progress +
  save cancel (W1b/N-060). Identity: boot-sector hash — cr_ui_scan_verify_identity refuses
  with CR_UI_ERR_BAD_CHECKSUM on mismatch.

### 2.9 Recovery writing (src/core/recovery/, 1,762 lines) — EXISTS
- SafeWriter (cr_safewriter.h + 1,138-line impl): queue + injected DestIo (Win32 backend
  win/cr_destio.cpp); per-item temp with `.part-<id>` suffix (concurrent same-name safety);
  fsync-equivalent + atomic rename commit; **collision naming numeric suffix name_N, cap
  10000** (F11 corrected); SHA-256 hash verification per item; recursive directory
  reconstruction (G-08, idempotent); **destination-overlap HARD refusal** F12 — armed ONLY
  when a destination extent sits on the SOURCE disk (the RECALL0 root-cause fix;
  win::dest_overlap_spans; release notes v0.7.1 §1: post-fix 10,341/10,404 items, zero
  refusals); truthful outcomes Done / Salvaged (had_unreadable) / nothing-to-recover (no
  extents AND no resident data → no zero-file commit — GAP-S6-7); zero-fill skip policy only
  for PARTIAL spans of real files; post-commit metadata restore (SetFileAttributes +
  SetFileTime trio, non-fatal, S-060); free-bytes pre-check + ENOSPC fast-fail (N-015);
  LDM-basis nodes read through re-armed virtual devices with PHYSICAL member-span mapping
  (S-052); extents decoded through compress::decompress_block per codec.
- Report (cr_report.cpp, 322 lines): JSON recovery report with per-item evidence (duration,
  hash, outcome, path); schema CI-validated (engine/tools/validate_report.py; docs/
  recovery-report-schema.md); folder taxonomy incl. lost_files_reconstructed (og kind 7).
- Dictionary (cr_dict.cpp + cr_dict_data.cpp, N-064): the og's 228-entry common-password
  list; sequential attempts, user candidates after the shipped set, stop at first success,
  password material never logged; wired to BitLocker manage-bde/PowerShell unlock only,
  user-consented (cr_ui_unlock_volume_with_dictionary).

### 2.10 Orchestrator (src/core/orchestrator/cr_scan.cpp, 3,300 lines) — EXISTS
- Stages: quick (structured metadata walk) → deep (deleted extras + remnant sweep + lost
  discovery) → carve (dedup against structured), StageWeights 40/35/25 percent (cr_scan.h:
  72-75). **Carve runs BEFORE deep when quick completed** (F9: the carve dedup needs the
  structured tree first — cr_scan.cpp:1561-1566). Single-walk design: quick runs the engine
  pass with deep semantics when deep is coming (cr_scan.cpp:1684-1691).
- Pre-scan phases: optional pre_scan_repair (S-008: analyze → plan surfaced via events →
  execute ONLY on consent hook AND an openable write view; failed/denied repair never aborts
  the scan) then optional pre_scan_check (M-041: the og's order — repair writes first, the
  audit sees repaired bytes; report never aborts).
- fs_hint auto-probe probe_family (cr_scan.cpp:968; NTFS/FAT/exFAT/ext4/HFS+/Btrfs/ZFS/
  ReFS/BitLocker/UDF-recognize); read pipe = CachedReader(8 MiB, 256 KiB) over RetryingDevice
  over the slice with a pinned, tested bare-slice degrade path (build_read_pipe, M-058);
  threads = hw-1 clamped 1..8 (resolve_threads :58).
- Lost-partition integration (whole_disk): discover with live-table overlap scoring (audit
  row 31a), S-051 no-double-scan reconciliation, M-137 insertion cap 10, parallel candidate
  scans, LDM dynamic volumes scanned as virtual devices kept alive past run() (S-052
  controller registry).
- Progress model: weighted monotonic permille; stage-true bytes where counted (carve, the
  deep sweep's byte-denominated sub-passes — P-DEEP-01); pause-compensated timers; typed
  event stream (cr_scan.h:131-210): ScanStage/ScanProgress (coalesced ≤ 1000 + flips)/
  FilesFound (N-166 live-refreshed counters + D4b bytes_found)/ErrorRange (M-049 module +
  operation + attempts attribution)/Checkpoint/LostCandidate/Repair/FsCheck 10. Shim adds
  HEARTBEAT 8 (5 s liveness watchdog), SAVE_PROGRESS 9, SESSION_LOAD_PROGRESS, AUTOSAVED.
- Cancellation & pause: CancelToken checked at engine loop heads; cr_ui_scan_pause blocks
  engines cooperatively, resume releases (cr_ui.h:643-644); cancellation returns
  ERR_CANCELLED with the tree intact — saving afterwards is the caller's explicit choice.
- Filters: scan-time folder filter (M-073) prunes out-of-prefix subtrees at EVERY stage
  boundary BEFORE the checkpoint emit (sessions never carry out-of-scope folders; carve root
  and the pathless root exempt); FilterFolder junk hidden-set pass post-walk pre-grading
  (M-018) — the two compose without double-counting (ScanStats.junk_filtered /
  folder_filtered, cr_scan.h:448-460).
- Volume label (M-079/N-133): explicit --label > engine-decoded on-disk label > image stem;
  engine pass REPLACES a derived label; 100-byte host channel (N-090). Telemetry (N-166)
  carries per-stage wall-times, cache hits/misses, carve skip counters.

### 2.11 Repair family — EXISTS (multi-module, all transactional + consent-gated)
- **RawFixer** (repair/cr_rawfixer.cpp, 1,052 lines, S-008/S-010): analyze() read-only plan;
  execute() only with explicit plan + consent; safety S1-S7 — consent, clean-refusal,
  ambiguity-refusal, CRC'd durable undo log with sector snapshots before every write + failed
  action rolls back all prior, post-write revalidation with full rollback on mismatch,
  scoped writes (boot sectors, checksums, derived geometry, FAT-table MIRROR copies restored
  sector-granular from a signature-valid sibling, $MFT record 0 from $MFTMirr slot 0; directory
  content and FAT entries beyond the mirror are report-only advisories), volume quiesce
  (FSCTL_LOCK_VOLUME before the first write, ERR_VOLUME_LOCK_FAILED = zero writes). Wired
  pre-scan (CLI --repair/--repair-apply, cr_ui consent ABI, Repair events, undo-log
  persistence cr_ui_scan_repair_save_undo).
- **Damage audit** (check/cr_fscheck.cpp 1,092 lines + cr_fsck_launch.cpp, M-041):
  CCheckFAT + CCheckNTFSVolume parity — FAT DBR flavor ceilings, dead-zone/duplicate FAT
  entries, pairwise FAT-copy compare, FAT32 root-chain walk, dir-tree audit ('.'/'..'
  pairs, size-vs-chain, cycles), orphan sweep with host decision; NTFS $MFT record-0 +
  $MFTMirr proofs, version gate {1.2, 3.0, 3.1}, $Bitmap structural checks, MFT × $Bitmap
  cross-check (two records claiming one cluster); error taxonomy 0x1001..0x100e; never
  aborts the scan; REC_FS_CHECK session record + CR_EVT_FS_CHECK.
- **chkdsk escalation** (win/cr_fsck.cpp, M-075): launches the real
  %SystemRoot%\System32\chkdsk.exe elevated/synchronous/timeout-bounded with WoW64
  filesystem-redirection disabled; cr_ui_fsck_volume ABI; MaintenanceDomain runFsck.
- **Lost-partition apply** (apply/cr_apply.cpp, 760 lines, S-029): plan_apply read-only with
  refusals (overlap with live entry, past disk end, LBA 0, 32-bit MBR range, non-MBR layout);
  execute_apply = consent BEFORE lock, live-sector-0 revalidation before consent AND after
  lock (ERR_APPLY_STALE), CRUNDO01 undo container, verified write-back via raw sector-0
  rewrite + the kernel IOCTL chain (0x7C100 delete-layout / 0x7C010 set-layout / 0x7C058
  single-entry record / 0x70140 update-properties with 3× Sleep(1000) retry — the Windows RW
  transport implements IApplyDevice; file/RAM devices no-op). undo_apply + save_undo; GPT
  variant regenerates the 32-bit MBR disk signature (corpus-corrected semantics).
- **Boot-chain repair** (M-040 stages a+b): bcdfix (pure BCD hive cell decode — 0xffffffa0
  allocated-cell markers, partition byte-offset u64 at P+0x24 — new_lba<<9 rewrite +
  plan/execute/undo + P62 consent); nvramfix (UEFI Boot#### updater over an injected
  firmware-variable seam, MBR/GPT matching, update-only no create/delete); win/cr_bootfix.cpp
  (hive file rewrite + flush + \Registry\Machine\BCD00000000 force-unload with
  SeRestore/SeBackup; dynamic Get/SetFirmwareEnvironmentVariableW resolution +
  SeSystemEnvironmentPrivilege). CLI `apply --boot-fix <old-lba>`; boot.ini + chkdsk =
  registered dispositions (D-90).
- **Registry hive tooling** (reghive/, 1,131 lines): read-only regf parser (cr_reghive.cpp —
  FVEAutoUnlock S-014 source); **direct hive-file editor** for dead-system surgery
  (cr_reghive_edit.cpp — rewrite-only-never-create, ≤4-byte data inline with bit 31, larger
  data first-fit cell reuse else new hbin, sequence-number bumps); offline OS profile
  (cr_offlineos.cpp); surgery (cr_surgery.cpp); MountPoints2 placements (cr_mountpoints2.cpp,
  the M-118 register-history source).

## 3. Extension points for the harvest
1. **Orchestrator stage hooks**: ScanConfig (cr_scan.h:202-420) is the seam. A new pre/post
   phase slots in beside pre_scan_repair (S-008 pattern: hooks struct + consent + never
   abort) or pre_scan_check (M-041 pattern: after repair, before engines, surface via event
   + session record + stats). New core event types append at event::Type ≥ 10 (8/9 are
   shim-owned — cr_scan.h:132-143); new CR_EVT_* ≥ 11; consumers must ignore unknowns.
2. **Node/tree model**: every new result source emits tree::FileNode + FileProperties into
   ResultTree roots (root_active/deleted/lost/carved + volume-group roots); extents carry
   codec/psize/logical/hole so new compressed/fragmented formats flow to the writer and
   preview unchanged (cr_node.h:129-160).
3. **ABI export conventions** (how a new export is added, ABI-CONTRACT.md §9): declare in
   include/cr/cr_ui.h under a domain section (append-only, no bump); implement in
   src/ui/cr_ui.cpp with the catch-all → CR_UI_ERR_INTERNAL; add the command row to
   contracts/command-surface.json (serves/abiRefs); regenerate the Rust struct mirrors
   (tools/gen_abi_layouts.py — bridge-integration lane gate 1 enforces freshness); add the
   connector method + query; contracts-ci checks the chain end-to-end.
4. **contracts/validate.mjs** enforces: new cr::Err → error-codes.json row + uiCode; new
   CR_EVT → event-mapping.md row; new command → manifest + bridge fn + connector invoke.
5. **UI connector domains**: new capability = Domain interface in contract.ts + mock
   implementation + tauri-connector delegation to a new src-tauri/src/commands/<x>.rs
   registered in lib.rs generate_handler (53 commands today); ESLint rules confine coupling
   to src/queries.
6. **Carve registry**: append FMT_ enum values (serialized carve_format stability — always
   append); add the kTable row WITH a validator (fail-closed if null, F8); raw-sign table
   regenerated via engine/scripts/gen_rawsigns.py; the user-sign channel already exists for
   user-taught signatures without engine changes.
7. **Filesystem engines**: new engine = src/core/<fs>/ module + CMake entry + FsFamily enum
   append (wire values stable — Reserved5=5 must not renumber, cr_scan.h:37-70) +
   run_engine_pass dispatch case (cr_scan.cpp:1799+) + a lost-partition probe if applicable.
8. **Repair surfaces**: a new repairer follows rawfixer's S1-S7 seams (analyze/execute/undo,
   IWriteDevice, consent hooks) or apply's A1-A4 pattern; session persistence appends new
   REC_* types (readers skip unknown — cr_crsf.h).
9. **Imaging**: the producer's flat loop (cr_image.cpp:1584) is the insertion point for a
   two-pass/bad-sector-isolating imager; the existing ReadErrorMap/RetryingDevice machinery
   and the IStorageDevice seam are directly reusable; UI/ABI already expose cancel + ranges.

## 4. Test & fixture infrastructure
- **Engine unit tests**: 75 test_*.cpp files under engine/tests/unit (~79,206 lines) + main.cpp
  + test_fixtures.hpp/test_tmpdir.hpp; GTest via FetchContent; target `cr_tests`
  (tests/CMakeLists.txt:5) links cr::core; data fixtures copied next to the binary (:82-93).
  Domain map (file → coverage at a glance):
  - NTFS: test_ntfs_volume (walk, hardlinks, ADS, USN, mirror, LZNT1 vectors), test_fixup,
    test_runlist (N-163 torn prefix)
  - FAT/exFAT: test_fat_volume (deleted LFN/chain), test_exfat_volume (parked-set oracle)
  - Linux FS: test_ext4_volume + test_ext4_m143_journal_inode_image + test_ext4_m144_large_
    volume, test_btrfs_volume, test_zfs_volume, test_hfsplus_volume, test_refs_volume
  - Partitions: test_lostpart (dual-candidate, BitLocker S-019), test_crc32_gpt (MBR/EBR/
    GPT), test_ldm + test_ldm_recovery (byte-exact span/stripe/RAID-5 round-trips),
    test_refs_ldm
  - Carve: test_carve (registry fail-closed, grammar/raw-sign/user-sign layers, dedup,
    resume re-carve :1565)
  - Tree/query: test_node_tree, test_grading, test_diskmap, test_snapshot, test_query +
    test_query_perf (1M < 200 ms), test_enumerate_merge, test_parallel
  - Pipeline: test_orchestrator, test_scan_truth, test_pause_resume, test_resume_
    revalidation, test_session, test_consistency, test_telemetry, test_apply, test_vollabel,
    test_folderfilter, test_config_display
  - Storage/transport: test_image (qemu compat), test_image_create_win, test_directalign
    (64 KiB envelope), test_transport_logic + test_transport_win, test_destio_win,
    test_cached, test_retry
  - Recovery: test_writer_dirs, test_writer_readpipe, test_dict, test_report
  - Repair: test_rawfixer, test_fscheck, test_fsck_launch + test_fsck_win, test_bcdfix,
    test_nvramfix, test_reghive + test_reghive_edit, test_mountpoints2
  - Crypto/preview: test_bitlocker_win + test_bitlocker_xts, test_fve_policy, test_sha256,
    test_preview, test_thumbnail, test_sat_stamps, test_text
  - Health: test_health_grade, test_nvme_health, test_health_csmi
  - ABI/host: test_cr_ui, test_privilege, test_no_console_spawn, test_m101_branches
- **C consumers**: tests/ui_abi_check.c (plain-C full lifecycle, MSVC /W4 /WX, the executable
  form of ABI-CONTRACT) + tests/abi_layout_dump.c (+ abi_layout_dump.header.sha pin).
  **win-stub** headers (tests/win-stub/*.h) let the win layer compile under Linux CI.
- **Fixtures** (engine/tests/data/): images/ — qemu-disk1.{vmdk,vhd,vhdx} (4 MiB, written
  0xAB@0/0xCD@1M/0xEF@3M patterns + SHA256SUMS; the VHDX carries a post-close log entry so
  the overlay parse runs every suite) + junk-gate-fat16.raw; thumbnails/ — ~40 hostile/EXIF-
  orientation (1-9)/TIFF-variant/decompression-bomb files + SHA256SUMS; exfat/upcase-table-
  mkfs-exfat-1.2.9.bin; euuserdata-verbatim.json (user-signs corpus truth).
- **Local gate**: engine/ci-local.sh / ci-local.ps1 — 10 steps mirroring CI order and
  strictness (clang-format, configure, build, ctest, C consumer, bench scorer self-tests…)
  plus `--san` (TSan/UBSan lanes) and `--fuzz` (libFuzzer smoke ~11 min); documented in
  engine/docs/CI-LOCAL.md.
- **bench/** (comparative harness, bench/README.md): seed/generate_seed.ps1 (deterministic
  versioned corpus + seed-manifest.jsonl + per-file extents/record numbers), scenarios/
  apply-scenario.ps1 (deletion matrix incl. decay branches + the physical-ceiling probe),
  volume/volume.ps1 (hash-maps ground truth), ours/run-ours.ps1 + run-behavior.ps1
  (unelevated-refusal probe etc.) + run-alignbench.ps1 (QD×size sweep), og/ (run-og.ps1,
  og-driver.ps1, FlaUiProbe C# probe), metrics/ (make_baseline, compute_metrics, compare,
  merge_runs, regression_gate, inventory_recovered, vlm_extract/crosscheck), tools/pe_info.py,
  vlm-evidence/ JSONL captures, baseline.json, root-causes.json. CI vehicle: benchmark-src.yml.
- **tools/ at repo root**: forensic/ (seed-disks.ps1, install-probe.ps1, drive-ui.mjs CDP,
  io-probe.ps1, launch-app.ps1, collect-facts.ps1, unelevated-launch.ps1), validation/
  (measure-cr-ui.c, installer-matrix.ps1), gen_abi_layouts.py.
- **UI tests**: 31 files in ui/tests (stores, connector-mock, connector-tauri, scan-state-
  drain, node-space, health-window, i18n-parity/sweep, theme-snapshot/persist, dialog-
  choreography, exit-guard, press-states, consent-surfaces, home-states, confidence-pill,
  lane-c/reference-surfaces, sidebar, toaster, format, logger, route-fade, window-controls,
  event-surface, components, backup-resume-route, ui-store, scan-session-store,
  i18n-sweep-extraction + fixtures/adopted-tokens.snapshot.json), 4 Playwright e2e specs /
  15 cases (mock-pinned by design), and the Rust it/ crate (bridge_integration.rs +
  abi_layouts.rs, run by bridge-integration.yml).

## 5. Known gaps (documented)
- **engine/docs/CORE-GAP-LIST.md** (the live register): at this HEAD nearly all rows CLOSED —
  GAP-P1-1..5 (LZNT1, lzjb bound, recycle remap, salvaged outcome, elevation probe),
  W2/W3/W4 waves, G-01/G-02 (shim + snapshot), BENCH-1..5, W5 docs, W6 Win7 gate + release,
  S6-1..S6-8 (deleted LFN, deleted chains, ReFS gate, remnant content, dual-candidate,
  zero-file refusal, resume dedup). Three rows read **MISSING but are STALE** — the artifacts
  now exist in-tree: GAP-S6-9 (docs/ABI-CONTRACT.md present, 800+ lines normative),
  GAP-S6-13 (io::direct_envelope + test_directalign.cpp present; cr_transport.cpp:247),
  GAP-S6-14 (docs/PERFORMANCE-PROFILE.md present). CAPABILITY-MATRIX.md §4 still says
  "GAP-P1-1 … the only recovery-output P1 still open" — stale vs its own §1 LZNT1 DONE row.
- **docs/INTEGRATION-GAPS.md** (root, 264 lines — the 2026-10-27 installed-product audit;
  NG register C/H/M/L): verified-closed since filing: G-9 bus kinds (D-ENUM-02), NG-14 SD
  filter, NG-18, NG-21 (pre-scan repair guard present at ui/src-tauri/src/commands/scan.rs:
  216-220), NG-22, NG-10. Spot-check of **NG-1 (results tree vs the real engine)**: the
  premise is still structurally present — virtual roots are created with attributes == 0
  (ResultTree::add_root, cr_node.cpp:139-160 sets only name/role) while the bridge derives
  is_dir from `attributes & 0x10` (state.rs:732), though path assembly was reworked to DFS
  parent-chains (state.rs:714-730). Current real-engine rendering status: UNVERIFIED
  statically (needs runtime evidence; forensic-repro.yml is the vehicle). Still-open
  UI-side rows relevant to the harvest: G-2 (per-item currentPath ticker — the engine has
  no per-file event), G-3 (live found-bytes is progress-cadence sampled; exact value latched
  at completion), G-4 (rendered previews pdf/av/office null with truthful hints), G-5
  (keepEmptyFolders / fileRepair options truthfully disabled), G-11 (folder-scoped scans are
  bridge-side path scoping — documented delta), NG-20 (~20 ABI exports never called by the
  UI, e.g. image scan targets).
- **AUDIT registers** (engine root, 2026-10-23 era): AUDIT.md (253), AUDIT-ARCH.md (305),
  AUDIT-NTFS.md (291), AUDIT-LINUX-FS.md (237), AUDIT-FALLBACKS.md (305 — the fallback/
  degraded-path census with REMOVE/MATCH/RETAIN-AS-SAFETY verdicts). The diff-to-fix ledger
  is largely FIXED (diff-to-fix/FIXED-LEDGER.md); remaining tracked-open OQs in
  diff-to-fix/tracked-open/ (thumbnail codec-bound formats OQ-S10-1, chunking relative perf
  OQ-S4-3, driver-era residue OQ-S10-3).
- **Standing exclusions** (docs/CAPABILITY-MATRIX.md §3): the file-repair product line
  (Fixo-provenance photo/video/office repair — S-050 formal disposition; D-137 records the
  future seam: a future in-engine implementation would enter through cr/repair with
  tree::FileNode::extents as the integration point); cloud/NAS recovery; BitLocker key
  DERIVATION (user-FVEK decryption is live); .edisk; .rsf import (G-11); production CA
  signing (self-signed only).
- **Structural limitations to verify against the references** (harvest targets): image
  producer lacks bad-sector isolation/pause-resume/two-pass (§2.6); XPRESS/LZX compressed
  NTFS not implemented (shared with the og); ZFS multi-vdev unsupported; VHDX differencing +
  VMDK streamOptimized refused on read; carve has no generic fragmented-file reassembly;
  UDF structured engine absent (both sides); ext4/HFS+/Btrfs/ZFS/ReFS CI staging matrix not
  yet built (CAPABILITY-MATRIX §0 follow-up: mkfs-class staging script per filesystem).

## 6. Coverage log (honest)
- **Read fully / near-fully**: include/cr/cr_ui.h (all 95 export declarations + section
  anchors + ABI rules + event defines); include/cr/orchestrator/cr_scan.h (FsFamily →
  ScanConfig → ScanStats); the docblocks + key structs of every engine header listed in §1.2/
  §2 (lost, table, table_wire, ldm, ntfs×5, fat, exfat, ext4, refs, btrfs, zfs, hfsplus,
  carve×4, tree×4, storage×7, session×3, recovery×3, health×7, repair, check×2, apply,
  reghive×4, bcdfix, nvramfix, preview×2, query, win×9); src/core/lost/cr_candidates.cpp
  (discover + all probes + scoring); cr_scan.cpp run-flow regions (1400-1600, 2244-2470,
  resume revalidation 1240-1260); storage/cr_image.cpp (open_image dispatch + VHDX + the
  producer loop 1540-1647); win/cr_image_create.cpp (full disposition body); cr_node.cpp
  (arena + add_root/add_folder); contracts/validate.mjs + README; installer/
  recoverywizard.nsi structure; all 9 workflow headers + job maps; ui contract.ts domain
  inventory; lib.rs command registry; state.rs NG-1 region; docs/{CORE-GAP-LIST,
  CAPABILITY-MATRIX, INTEGRATION-GAPS, ABI-CONTRACT}.md key sections; tests/data/images
  README; ci-local.sh; bench/README.
- **Skimmed (docblocks + structure + targeted greps)**: cr_ui.cpp (section anchors only),
  cr_mftwalk.cpp, cr_registry.cpp/cr_analysers.cpp (counts + fail-closed gate), rrecov.cpp
  (verb map + autosave notes), cr_rawsigns_data.cpp (counts), health win files (transport
  chain), reghive/bcdfix/nvramfix implementations, ui features pages, e2e specs, engine/docs
  corpus (~60 files listed, ~10 read), engine/worklog.md (grep only).
- **Read as secondary evidence**: worklog/context-sheets/PRODUCT-STATE-smart-2026-11-17.md
  and PRODUCT-STATE-perf-2026-11-18.md (headers + section maps; version history 0.9.2→0.9.5).
- **Skipped**: engine/worklog/ + ui/worklog/ session prompts (out of scope per mandate),
  .git/, node_modules-equivalents, ui/docs/evidence VLM JSON blobs, bench metrics script
  bodies, diff-to-fix evidence trees (two findings read fully: I-049, OQ-S79-1).

## 7. Harvest-relevant conclusions
The product's spine — orchestrator, NTFS/FAT/exFAT engines, two-layer carving, session
persistence, the transactional repair family, and the contracts-governed cr_ui ABI — is
mature and heavily parity-pinned against the EaseUS reference, so the harvest should target
the edges where this repo is thin rather than the core: (1) **imaging** is the weakest
production surface (single-pass flat-raw producer; no bad-sector isolation, error-map
persistence, or pause/resume — sectorforge/testdisk-class DAO imagers can donate a two-pass
architecture that plugs into the existing IStorageDevice/ReadErrorMap seams and the
already-cancelable ABI); (2) **NTFS depth beyond ours** (ntfs-recover/mft-rescue: $MFT
run-list reconstruction/patching as an in-scan recovery stage — ours is consent-gated
RawFixer-only; richer $LogFile/$UsnJrnl journal analysis — our $LogFile is deliberately
absent per I-049, and MFT-slack carving does not exist); (3) **carving breadth** (sleuthkit's
signature/validator corpus and generic fragmented-file reassembly vs our 155+139+user
layers with no fragmentation story); (4) **lost-partition heuristics** (testdisk's deeper
boot-sector-backup probing state machines vs our 11-family probe set); (5) **filesystem long
tail** (halim-recovery references for ext4/HFS+/Btrfs deleted-inode edge cases our
single-pass engines may miss). Every insertion point is already contract-governed (ABI
append-only rules, event types ≥ 10, REC_* append, the connector-domain + command-surface
chain), so harvested code lands as new modules behind existing seams with CI gates that
enforce the boundary end-to-end.
