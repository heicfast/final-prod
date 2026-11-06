# CORE-API — the `cr_ui` C ABI as a consumer sees it (Phase 0, Agent C)

Scope: `engine/include/cr/cr_ui.h` (1,602 lines, **92 `CR_UI_API` exports**, ABI v4) plus the
implementing sources read in full or in the load-bearing parts:
`src/ui/cr_ui.cpp` (5,087 l), `src/core/orchestrator/cr_scan.{h,cpp}`, `src/core/carve/cr_carve.cpp`,
`src/win/cr_enumerate.cpp`, `src/win/cr_health.cpp`, `src/win/cr_fsck.cpp`, `src/win/cr_bitlocker.cpp`,
`include/cr/cr_error.h`, `include/cr/win/cr_enumerate.h`, `src/app/rrecov.cpp` (the CLI).
Consumer cross-refs into `ui/src-tauri/src/bridge/*` and `ui/src/connector/contract.ts` are cited only
where they pin the user-visible presentation of an engine fact.

---

## ABI overview + versioning

- Flat C11 ABI over cr_core + the cr_win platform layer; opaque handles (`cr_ui_device`,
  `cr_ui_scan`, `cr_ui_session`, `cr_ui_snapshot`, `cr_ui_preview`, `cr_ui_recovery`)
  — cr_ui.h:93-99. In-process by doctrine ("no CLI subprocess and NO IPC on the UI<->engine leg",
  cr_ui.h:5-21); the Rust bridge binds it via `libloading`-style externs (ui/src-tauri/src/bridge/abi.rs).
- **Every function returns u32 from the same numeric space as cr_error.h** (0 = OK); no exception
  escapes (each entry wrapped in `guarded()` with a catch-all → `CR_UI_ERR_INTERNAL` 0x50090001,
  cr_ui.cpp:138-143). Thread-local diagnostic text: `cr_ui_last_error` (cr_ui.h:107-110; cr_ui.cpp:117,1381).
- Strings: UTF-8 in, caller buffers out (two-call size pattern) or heap strings freed with
  `cr_ui_string_free` (cr_ui.h:28-29,112-113). Zero-init config structs are always safe
  (0 = "default", never "nothing matches") — cr_ui.h:30-31.
- **Threading contract** (cr_ui.h:34-38): scan/recovery run on the CALLER's thread (blocking);
  pause/cancel/telemetry/snapshot capture are safe from ANY thread; progress/event callbacks fire
  on worker threads, must not block, and carry NO cancellation semantics — the token functions
  (`cr_ui_scan_pause/_resume/_cancel`, `cr_ui_recovery_cancel`) are the cancel path.
- **Versioning** (cr_ui.h:39-48,81): `cr_ui_abi_version()` returns `CR_UI_ABI_VERSION = 4`.
  Policy (docs/ABI-CONTRACT.md §9): ADDING exports, APPENDING struct fields, adding event `type`
  values are compatible — NO bump (consumers detect by export presence / zero-init appended fields);
  removing/renaming an export or changing a field's type/meaning/order bumps. v3→v4 was the
  M-067 `cr_ui_user_sign` field resize (bytes 16→48, mime 64→81 — a 73-char MIME memcpy previously
  left the field non-NUL-terminated, cr_ui.h:75-80). The bridge refuses to bind on mismatch
  (scan.rs:124-130). All append-only tails carry explicit "zero-init safe" comments
  (e.g. trim fields M-076 cr_ui.h:403-415; session_info.label_full N-090 cr_ui.h:879-881).

## Exports by domain (92 total)

Error column lists the *notable* codes beyond OK/BAD_PARAMETER/INTERNAL; all are cr_error.h values.

### Lifecycle / diagnostics (3)
| Export | Semantics | Blocking | Errors |
|---|---|---|---|
| `cr_ui_abi_version()` | returns 4; call first | no | — |
| `cr_ui_last_error()` | TLS diagnostic of last failing call | no | — |
| `cr_ui_string_free(char*)` | frees heap strings | no | — |

### Devices / transport (10)
| Export | Semantics | Blocking | Errors |
|---|---|---|---|
| `cr_ui_open_image(path,off,bytes,out)` | portable image view, 512-byte-sector device bounded [off,off+bytes); bytes==0 = to EOF (cr_ui.h:313-316) | yes | ERR_OPEN_FAILED, ERR_IMAGE_CORRUPT |
| `cr_ui_open_disk(disk,out)` | `\\.\PhysicalDriveN` direct raw I/O; **non-elevated token refused ERR_ELEVATION_REQUIRED 0x500C0001** (cr_ui.h:318-329; cr_ui.cpp:1433-1435) | yes | ERR_ELEVATION_REQUIRED, ERR_OPEN_FAILED |
| `cr_ui_open_volume(letter,out)` | volume slice on parent disk; BitLocker probed first — locked → **ERR_FVE_LOCKED 0x50060001** unless auto-unlock succeeds; ACL case ERR_FVE_ELEVATION_REQUIRED (cr_ui.cpp:1456-1474) | yes | ERR_FVE_LOCKED, ERR_FVE_ELEVATION_REQUIRED, ERR_ELEVATION_REQUIRED, ERR_OPEN_FAILED |
| `cr_ui_open_volume_with_key(letter,hex_fvek,out)` | M-033: 64/128-hex FVEK, in-stack XTS-AES decrypt (cr_ui.h:342-353) | yes | ERR_BAD_PARAMETER, ERR_FVE_LOCKED |
| `cr_ui_unlock_volume_with_dictionary(letter,pwds,count)` | N-064 consent action: 228-entry dict + user candidates via PowerShell/manage-bde (cr_ui.h:355-370) | yes (2 min/command) | ERR_FVE_LOCKED, ERR_UNSUPPORTED |
| `cr_ui_device_close(dev)` | device must outlive scans/previews/recovery (borrowed, cr_ui.h:310-311) | no | — |
| `cr_ui_device_geometry(dev,out)` | total_bytes/logical_sector/physical_sector/sector_count (cr_ui.h:374-381) | no | BAD_PARAMETER |
| `cr_ui_device_transport(dev,buf,cap,needed)` | "direct" or "image", two-call (cr_ui.h:383-388) | no | TRUNCATED |
| `cr_ui_last_transport_error()` | M-030 advisory: last open's refusal code; process-global, never clears (cr_ui.h:333-340; cr_ui.cpp:1417) | no | — |
| `cr_ui_scan_set_event_fn(scan,fn,user)` | subscribe BEFORE run(); mid-run swap = ERR_BAD_PARAMETER; one sink per scan (cr_ui.h:300-303; cr_scan.cpp:380-385) | no | ERR_BAD_PARAMETER |

### Enumeration (3) — Windows only, ERR_UNSUPPORTED elsewhere
`cr_ui_enumerate_disks`, `cr_ui_enumerate_disks_ex` (+flags; unknown bits → ERR_BAD_PARAMETER,
cr_ui.cpp:1937-1943), `cr_ui_enumerate_volumes`. Two-call pattern; enumeration failure returns its
error code (never a silent empty list); **no cache — every call re-queries the OS** (N-098,
cr_ui.h:438-446). See "Enumeration contract".

### Scan (17)
| Export | Semantics / contract |
|---|---|
| `cr_ui_scan_create(cfg,out)` | stage_mask bits 1\|2\|4 (0 = all 7); weights 0 = 40/35/25; threads 0 = auto; identity-card strings optional (auto-filled from enumeration on Windows); checkpoint_path; progress fn; repair consent fn; memory monitor; autosave_dir; folder filter (cr_ui.h:505-607; cr_ui.cpp:1296-1341) |
| `cr_ui_scan_close(s)` | frees handle |
| `cr_ui_scan_attach_device(s,dev)` | borrowed device; before run() (cr_ui.h:612-615) |
| `cr_ui_scan_run(s)` | **synchronous on caller thread**; returns 0, an engine error, or CR_UI_ERR_CANCELLED with the tree intact (cr_ui.h:617-620); second run → ERR_BAD_PARAMETER (cr_ui.cpp:2320-2325) |
| `cr_ui_scan_pause/_resume/_cancel` | token control from ANY thread; pause "blocks engines cooperatively" (cr_ui.h:622-625) |
| `cr_ui_scan_busy(s,out)` / `cr_ui_scan_stopping(s,out)` | N-075 synchronous live-state queries; busy=run() executing, stopping=cancel winding down (cr_ui.h:627-639) |
| `cr_ui_scan_telemetry(s,out)` | live snapshot, any thread: overall_permille + per-stage quick/deep/carve{permille,units_done/total,bytes_done,elapsed_ms,completed} + counters + bytes_scanned + error_ranges + paused/running/cancelled (cr_ui.h:641-661) |
| `cr_ui_scan_get_stats(s,out)` | final stats incl. repair/check outcome, bytes_unreadable, probe_unreadable, autosaved (cr_ui.h:663-701) |
| `cr_ui_scan_assistant(s,out)` | S-023 grading stats, valid after run() OK (cr_ui.h:703-730) |
| `cr_ui_session_regrade(sess,ids,count,out)` | per-selection re-grade, grade-1 only (sticky 0/2) (cr_ui.h:732-737) |
| `cr_ui_scan_checkpoint_count(s,out)` | G-13 diagnostics (cr_ui.h:738) |
| `cr_ui_scan_repair_save_undo(s,path)` | S-008 undo container; BAD_PARAMETER when nothing applied (cr_ui.h:740-747) |
| `cr_ui_scan_save_session(s,path)` | truthful .crsf; valid after run() incl. cancelled; atomic tmp+rename; save-scoped cancel via `cr_ui_scan_save_cancel` (cr_ui.h:822-835; cr_ui.cpp:2862-2904) |
| `cr_ui_scan_snapshot(s,out)` | immutable value tree; any thread WHILE run() (partial=1) or after (cr_ui.h:837-841) |
| `cr_ui_scan_verify_identity(s)` | boot-hash check for resumed scans → CR_UI_ERR_BAD_CHECKSUM on mismatch (cr_ui.h:843-849) |

### Lost-partition apply / repair (3)
`cr_ui_apply_lost_partition` (S-029: plan → consent callback → transactional write → undo; volume
slices refused; consent fires once before any write, default-deny — cr_ui.h:749-811),
`cr_ui_apply_undo_last`, `cr_ui_apply_save_undo`.

### Sessions (9)
`cr_ui_session_load`, `_close`, `cr_ui_session_resume_last` (M-154 pointer sidecar `last_scan.info`,
no directory scan; dead pointer → ERR_OPEN_FAILED, cr_ui.h:860-870), `cr_ui_session_get_info`
(fs_family, stage_done_mask vs configured_mask, slice offset/bytes, sectors, label[32]+label_full[100],
scanned_bytes, model[48]/serial[32], boot_hash — cr_ui.h:872-888), `cr_ui_session_stats`,
`cr_ui_session_snapshot`, `cr_ui_session_resume(sess,original_cfg,out)` (fresh tree, mask validated
⊆ configured; G-07), `cr_ui_session_list(dir,rows,count)` (W1a light-records-only listing, newest
first by embedded save time; unreadable files get rows with valid=0; TRUNCATED fills the newest
prefix — cr_ui.h:900-955), `cr_ui_session_last_autosave_path` (ERR_RANGE_OUT_OF_BOUNDS 0x50020004
when no live pointer — cr_ui.h:957-973), `cr_ui_session_load_with_events(path,fn,user,cancel_flag,out)`
(W1b: CR_EVT_SESSION_LOAD_PROGRESS from the calling thread; caller-owned flag polled per node; torn
load discarded — cr_ui.h:975-994).

### Results / snapshot / query / disk map / carve registry (17)
`cr_ui_snapshot_close/_node_count/_partial/_node/_dfs` (flat index-addressed rows,
CR_UI_SNAP_NONE=0xFFFFFFFF; name two-call — cr_ui.h:996-1052); `cr_ui_query_run/_rows_free/
_apply_buckets` (G-12 core-side filter/sort/paging over a STABLE session tree; sort keys 0..7 incl.
grade and type; category bits; state/origin masks; M-101 version-group + tags axes; buckets map the
og's Date/Size menu IDs — cr_ui.h:1054-1141); `cr_ui_carve_format_count/_info` (two-layer registry,
grammar rows then raw-sign rows, deterministic order — cr_ui.h:1143-1170); `cr_ui_scan_carve_tally_
count/_tally` (N-140 per-format counts + user-sign aggregate slot — cr_ui.h:1172-1191);
`cr_ui_register_user_signs` (M-048 atomic JSON install — cr_ui.h:1193-1209) + `cr_ui_user_sign_
count/_info`; `cr_ui_disk_map` (GAP-S6-11 geometry + allocation/coverage/found/confidence overlays
+ bad_ranges u64 pairs; ≥1 overlay required — cr_ui.h:1223-1268) + `cr_ui_disk_map_free`.

### Preview / archive / thumbnail (12)
`cr_ui_preview_open_snapshot/_open_session/_close/_size/_read` (bounded ≤4 MiB reads; unreadable
spans zero-filled AND reported; *placed = assembled — cr_ui.h:1274-1297), `cr_ui_preview_metadata`
(fixed-capacity fields, truncated flag — cr_ui.h:1299-1314), `cr_ui_archive_entry_count/_at`
(M-116 260-byte names, cached per handle — cr_ui.h:1316-1342), `cr_ui_preview_unreadable`
(cr_ui.h:1344-1347), `cr_ui_thumbnail_generate` (two-call size probe; 0x50030004 = decode refusal
also promotes grade to 2 on clean reads — M-115; ≤20-byte nodes refused UNSUPPORTED — cr_ui.h:1349-1418),
`cr_ui_thumbnail_format_supported`.

### Recovery (11)
`cr_ui_recovery_create` (read_chunk 0=1 MiB, rename_attempts 0=64, conflict 0/1/2 — cr_ui.h:1432-1443),
`_close`, `cr_ui_recovery_set_destination` (drive-letter destination arms the F12 overlap refusal —
ERR_DEST_OVERLAP 0x50050001 at enqueue — cr_ui.h:1446-1461), `cr_ui_recovery_enqueue_node/_subtree/
_enqueue_snapshot/_enqueue_subtree_snapshot`, `cr_ui_recovery_queue_count`,
`cr_ui_recovery_process` (**caller's thread**; cancel → CR_UI_ERR_CANCELLED, committed items stay),
`cr_ui_recovery_cancel` (any thread), `cr_ui_recovery_get_stats` (queued/done/refused/errors/
hash_mismatch/resumed/salvaged/dirs/nothing_to_recover — cr_ui.h:1478-1486),
`cr_ui_recovery_report_json` (G-09 schema, valid any time — cr_ui.h:1488-1492).

### Health / image / fsck / bitlocker-off (5)
`cr_ui_health_read` (below), `cr_ui_image_create` (S-040 flat raw image + .sha256 sidecar; cancel
keeps `<path>.partial`; elevated-token gate — cr_ui.h:1540-1558; cr_ui.cpp:1725-1727),
`cr_ui_fsck_volume(letter,flags,exit_code)` (M-075 chkdsk; flags 0 check / 1 fix; exit_code raw
chkdsk verdict, 259 = 15-min timeout; ERR_UNSUPPORTED when not elevated — cr_ui.h:1560-1578;
cr_fsck.cpp:80-94), `cr_ui_turn_off_bitlocker(letter,progress,user)` (M-078 consented decrypt;
OK = ACCEPTED, not plaintext — cr_ui.h:1580-1596).

## Enumeration contract

**Structs.** `cr_ui_disk_info` (cr_ui.h:392-416): `number` (u32, the \\.\PhysicalDriveN index),
`size_bytes`, `logical_sector`, `physical_sector` (bytes), `removable` (1 = bus USB/SD/MMC),
`layout` (0 none, 1 MBR, 2 GPT, 3 EBR — N-057 append), `table_ok`, `model[128]`, `serial[128]`
(UTF-8, may be empty), `trim_known/trim_on` (M-076 append, only filled by `_ex` with
CR_ENUM_FLAG_TRIM). `cr_ui_volume_info` (cr_ui.h:418-426): `letter` (0 when absent), `disk`,
`offset_on_disk`, `bytes`, `fs_name[16]`, `label[64]`, `bitlocker` (1 = FVE detected).
Note the ABI row exposes **only number+model+serial** as identity — the engine's richer axes
(firmware, WWN, interface_desc — PhysicalDisk, cr_enumerate.h:37-60) never cross the ABI except as
the scan config's identity-card strings (cr_ui.h:519-529, N-066).

**The two stacks** (cr_enumerate.cpp:139-159):
1. **Numbered PhysicalDrive namespace**: loop n = 0..max_disks-1; the shim passes **200**
   (cr_ui.cpp:1828; `ENUMERATE_DISK_BOUND = 200`, cr_winenv.h:45 — the old driver grammar's
   disk# ≤ 199 space). Per disk: `open_disk_for_query` (GENERIC_READ, full share; on
   ERROR_ACCESS_DENIED / ERROR_SHARING_VIOLATION only, retry a **0-access metadata open** and mark
   busy — N-071 hardening so AV/backup-held disks stay listed — cr_enumerate.cpp:98-113), then
   `query_disk_handle`: IOCTL_DISK_GET_DRIVE_GEOMETRY_EX (size/sectors), StorageAccessAlignment
   (logical@+0x10, physical@+0x14, accepted only in [256,65536]), StorageDeviceProperty
   (decode_device_descriptor with SDK-exact offsets: product@+16, revision@+20, serial@+24, bus@+28
   — N-080 fixed the pre-fix off-by-fields — cr_enumerate.h:62-91), StorageDeviceIdProperty
   (first device-associated identifier → WWN/serial-2, cr_enumerate.h:133-178). Table parsed via
   the pure core (`table::parse_disk_table`). A disk that yields **no geometry is dropped**
   (cr_enumerate.cpp:26-27,123-124).
2. **cfgmgr32 device-interface stack** (N-071): CM_Get_Device_Interface_ListW over
   GUID_DEVINTERFACE_DISK, PRESENT instances only; each opened with the same share/open policy;
   disk number from IOCTL_STORAGE_GET_DEVICE_NUMBER (else `kUnknownDiskNumber` 0xFFFFFFFF — such
   entries dedup by instance path and get no table parse); geometry/descriptor via the shared
   query path. cfgmgr failure degrades silently to stack 1 (cr_enumerate.cpp:168-212).

**Merge** (pure, header-inline — cr_enumerate.h:228-255): every loop disk kept (richer entry);
an interface entry appended only if its **number** is absent from the loop set AND its
**interface_path** unseen (kUnknownDiskNumber entries dedup by path only). Order: loop order then
pass order.

**Dedup/identity axes**: disk NUMBER and PnP instance path — **never model/serial**. Two identical
internal drives get distinct PhysicalDriveN numbers and BOTH surface engine-side; if the shipped UI
shows one row, the collapse is host-side (the BF-27 note "every Azure VM disk reports model
'Virtual Disk', serial empty" — worklog.md:2201-2204 — makes model+serial a weak host-side key;
Agent D audits `ui/src-tauri/src/commands/devices.rs`).

**Every known way a real disk can be MISSING from the result:**
1. Open failure not in {ACCESS_DENIED, SHARING_VIOLATION} → entry skipped silently
   (cr_enumerate.cpp:102-109).
2. Even the 0-access metadata open fails ("cannot even be metadata-opened stays absent — the honest
   limit, D-85", cr_enumerate.cpp:94-97).
3. No geometry from the handle (IOCTL_DISK_GET_DRIVE_GEOMETRY_EX fails) → dropped
   (cr_enumerate.cpp:28-38,123-124).
4. Interface-stack entry that does not open at all is skipped (cr_enumerate.cpp:166-167,186).
5. Disk number ≥ 200 misses stack 1 but is caught by stack 2 (device-number IOCTL), so the bound
   is effectively compensated — a miss requires BOTH stacks to fail.
6. `busy` is NOT exposed in cr_ui_disk_info — a listed-but-unopenable disk is indistinguishable
   from a scannable one until `cr_ui_open_disk` fails.
7. An SD card via USB reader enumerates as its own PhysicalDriveN (bus 7/11/12 → removable=1);
   it disappears only through 1-4 (e.g., a reader driver rejecting our share modes) — or because
   no media is present (no disk object at all).
8. **Volume enumeration wholesale failure**: `GetLogicalDriveStringsW(64, letters)` — the 64-wchar
   buffer holds ≤15 "X:\" strings; with 16+ drive letters (incl. CD-ROM/network letters, which all
   appear in the string) the API returns the required size > 64 and the code returns
   **ERR_BAD_PARAMETER for the entire volume list** (cr_enumerate.cpp:216-218). Card-reader
   multi-letter machines are the realistic trigger.
9. Volume rows only for DRIVE_FIXED/DRIVE_REMOVABLE (cr_enumerate.cpp:224-225); only the FIRST
   extent of VOLUME_DISK_EXTENTS is attributed (spanned/dynamic volumes → first disk only,
   cr_enumerate.cpp:232-240) and `VolumeEntry.disk` defaults to 0 — a volume whose extents query
   fails reports disk 0 (cr_enumerate.h:186).

## Health contract

Exports: `cr_ui_health_read` (the only health export; SMART + NVMe + SSD heuristic + TRIM in one
call, one struct). `cr_ui_smart_report` (cr_ui.h:1498-1532): available, transport[24]
("ata-passthrough"/"smart-rcv-drive-data"/"csmi-miniport"/"nvme"/"legacy"), ata/smart versions,
attribute_count (two-call attrs), failing_now, reallocated/pending/uncorrectable sectors,
power_on_hours, spin_retry, ssd_wear_level, temperature_c; NVMe block (nvme_available,
temperature_c_x10, spare + threshold, percentage_used, data_units_read/written, power_on_hours_nvme,
unsafe_shutdowns, media_errors, critical_warning); M-036 append (host_reads/writes,
controller_busy_time, num_error_log_entries, data_bytes_read/written — data units ×512,000);
ssd_like, trim_known/trim_enabled.

**Query path** (cr_ui.cpp:4879-4958): opens `\\.\PhysicalDriveN` GENERIC_READ|GENERIC_WRITE
(unelevated → **ERR_OPEN_FAILED 0x50020001** with an elevate hint — health does NOT use the
ERR_ELEVATION_REQUIRED gate, unlike open_disk; cr_ui.cpp:4887-4894). SMART via a transport CHAIN,
first that answers IDENTIFY serves the set (cr_health.cpp:216-262): (1) IOCTL_ATA_PASS_THROUGH
ATA_PASS_THROUGH_EX (cr_health.cpp:38-64); (2) SMART_RCV_DRIVE_DATA 0x7C088 legacy with the og's
exact SMART IDEREGS magic (cr_health.cpp:73-109); (3) CSMI CC-0x19 SCSI-miniport for Intel RST /
AMD rcraid (IOCTL_SCSI_GET_ADDRESS → \\.\ScsiN: port — cr_health.cpp:143-208). NVMe health log page 2
via IOCTL_STORAGE_QUERY_PROPERTY + ProtocolTypeNvme (cr_health.cpp:264-299). SSD heuristic =
model-string check ∪ seek-penalty probe (cr_health.cpp:325-342). No source → **ERR_HEALTH_UNAVAILABLE
0x50070001** (cr_ui.cpp:4953-4956).

**Every shell-out in the engine** (grep of engine/src for `_popen|CreateProcess|WinExec|system(`):

| Site | Command | Spawn mechanism | Visible console on a GUI desktop today? |
|---|---|---|---|
| cr_health.cpp:306 | `fsutil behavior query disabledeletenotify 2>nul` | **`_popen(..., "r")`** | **YES — VISIBLE.** MSVC `_popen` spawns `cmd.exe /c …` WITHOUT CREATE_NO_WINDOW/DETACHED_PROCESS; a GUI-subsystem parent has no console, so the child allocates one — the flash the user reported ("popup shells … probably cmd"). Called UNCONDITIONALLY by `cr_ui_health_read` (cr_ui.cpp:4948) and by `cr_ui_enumerate_disks_ex` only under CR_ENUM_FLAG_TRIM (cr_ui.cpp:1845-1846). This is defect D-CONS (worklog.md:2184-2191). |
| cr_fsck.cpp:50-51 | chkdsk (via build_chkdsk_command, Wow64-redirection-disabled) | `CreateProcessW(..., CREATE_NO_WINDOW, ...)` | **No — hidden.** Correct in-tree pattern. |
| cr_bitlocker.cpp:64-65 (`run_command`) | `powershell -NoProfile -NonInteractive -Command "Unlock-BitLocker/Disable-BitLocker …"` primary, `manage-bde …` fallback (cr_bitlocker.cpp:358-381); reached from unlock / dictionary unlock / auto-unlock / turn-off | `CreateProcessW(..., CREATE_NO_WINDOW, ...)` | **No — hidden.** (Both command forms, incl. every dictionary attempt, ride the same hidden spawn; 2-min unlock / 10-min turn-off timeouts.) |

No `WinExec`, no `system()`, no `ShellExecute*` anywhere in engine/src (grep clean; the elevation
relaunch ShellExecuteW "runas" is host-side, ui/src-tauri/src/commands/system.rs:85). The one
visibility defect is the single `_popen`; the fix shape (CREATE_NO_WINDOW CreateProcessW or
`_popen` with `cmd /c` + STARTF_USESHOWWINDOW) already exists in-tree in cr_fsck/cr_bitlocker.

## Scan contract

**Start exports.** One scan lifecycle (`cr_ui_scan_create` → attach → optional event sink → run →
snapshot/save); whole-disk vs volume is a property of the ATTACHED DEVICE VIEW
(`cr_ui_open_disk` + `cfg.whole_disk=1` for lost-partition discovery vs `cr_ui_open_volume`'s
slice — cr_ui.h:505-509), not a separate export. `fs_hint` values = orchestrator::FsFamily
serialized (0 auto, 1 NTFS, 2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS, 11 UDF;
5 reserved, 10 BitLocker probe-only — cr_ui.h:472-476; cr_scan.h:37-70).

**Stage model.** Three stages, ids **0 quick / 1 deep / 2 carve** (0xFFFFFFFF none — cr_ui.h:121-122;
event type values 1..12 — cr_ui.h:151-223). Weights default **quick 40 / deep 35 / carve 25
percent-of-total** (cr_scan.h:72-76); `weighted_progress = Σ(wᵢ·pmᵢ)/Σw` in permille, exact
single-division (cr_scan.cpp:131-146). Execution ORDER in run() (cr_scan.cpp:1436-1502):
pre-scan repair (S-008) → check phase (M-041) → **quick** (the single structured engine pass —
one MFT walk with deep semantics, "single-walk design", cr_scan.cpp:1592-1595) → **carve**
(N-181: `carve_before_deep = cfg_.run_carve && quick_done_`, cr_scan.cpp:1467 — landed in v0.7.0,
commit 75959e8d) → **deep extras** (NTFS MFT-remnant sweep + whole-disk lost-partition discovery
+ ≤10 candidate scans, cr_scan.cpp:2150-2446) → folder-filter/junk/grading passes → read-truth gate
(a fully-blind scan REFUSES ERR_READ_FAILED instead of fake-completing, cr_scan.cpp:1543-1578).

**Progress surfaces.** (a) Legacy `cr_ui_progress_fn(user, stage, stage_permille, overall_permille,
bytes_done, bytes_total, error_ranges)` (cr_ui.h:125-127); the shim derives the stage from whichever
telemetry is nonzero — carve checked FIRST (cr_ui.cpp:1204-1217). (b) Typed event stream
`cr_ui_event` (cr_ui.h:225-272): stage/permille fields, cumulative files_found/deleted_found/
carved_found, error-range deltas (err_start/len BYTES + M-049 module/operation/attempts), checkpoint
index, lost-candidate LBA, repair/check summaries, autosave rc. Events coalesce by overall permille;
files-found are stage-boundary samples LIVE-refreshed from tree attach counters (N-166,
cr_scan.cpp:555-575); **bytes_done/bytes_total only meaningful in the carve stage** (cr_ui.h:122-124,
cr_scan.cpp:596-597). (c) Polling `cr_ui_scan_telemetry` (any thread). (d) **Heartbeat** N-043: the
shim's watchdog thread fires CR_EVT_HEARTBEAT every 5,000 ms while run() is in flight and no
progress event arrived in the window — standing truth only, "never a synthetic permille advance"
(cr_ui.h:163-173; cr_ui.cpp:1150-1188).

**Cancellation / pause / resume.** `cr_ui_scan_cancel` from any thread → run() returns
CR_UI_ERR_CANCELLED, tree stays browsable, save-on-cancel is the caller's explicit choice;
`cr_ui_scan_save_cancel` is SAVE-scoped and never touches the scan token (cr_ui.h:827-835).
Resume: `cr_ui_session_resume` (mask-⊆-configured validation; completed stages skipped,
cr_scan.cpp:388-438) + `cr_ui_scan_verify_identity` (boot-hash; BAD_CHECKSUM on wrong volume).
Sessions: `.crsf` container "CRSFMT01" v1, chunked-zlib framing, per-record CRC32, forward-skip on
corruption (cr_crsf.h:1-35); records SESSION_STATE/VOLUME_INFO/PARTITION_CONTEXT/RESULT_TREE/
ERROR_MAP + optional 6..10 (assistant, alloc map, disk layout, fs-check, LDM layouts, carve tally —
cr_crsf.h:44-90). Autosave on completion AND user-stop (S-057), gated by S-044 (empty/resumed/
already-saved runs never auto-save); auto name `auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf` +
`last_scan.info` pointer with one-live-auto retention delete (cr_ui.cpp:2258-2314).

### THE ~75% BOUNDARY ANALYSIS (BF-22, the "75% advanced scanning" stall)

The overall permille the UI displays is the ENGINE's `overall_permille` (bridge: events.rs:268
`overall = evt.overall_permille / 1000.0`; telemetry co-sample events.rs:359-361), and both
deep-walk and carve map to the UI phase **"Advanced Scanning {{percent}}"** (contract.ts:169-175;
en.ts:155; stage mapping events.rs:256-262).

The decisive wiring is **`covers_both`**: the quick stage's single engine pass runs with deep
semantics and its progress bridge publishes **BOTH quick AND deep permilles simultaneously**
(cr_scan.cpp:722-724 `if (b->covers_both) tel_publish_(1, done, total, …)`; set by run_quick's
`run_engine_pass(..., 0, deep_coming=true)` at cr_scan.cpp:1597). Therefore, as the MFT walk
completes, quick_pm = deep_pm = 1000 and

  overall = (40·1000 + 35·1000 + 25·0)/100 = **750‰ = exactly 75.0%** — the end of the engine walk
  IS the 75% boundary. The orchestrator's own comment pins it: *"with covers_both the walk already
  fed the deep stage, so the bridge has forwarded values up to 750"* (cr_scan.cpp:1603-1606).

With the default stage set (the bridge zero-inits → mask 7, scan.rs:229-231), N-181 then runs
**carve as the SECOND stage**, so the displayed percent after 75% is `750 + 0.25·carve_pm` — i.e.
**75%..100% is the carve stage**, and the UI label at that moment flips to "Advanced Scanning"
(the CR_EVT_SCAN_STAGE(2) event). The forensic stall values decode exactly: 76.1% ⇒ carve_pm = 44‰
(4.4% into carve); run 19's stop "0.6% into CARVE" ⇒ carve_pm = 6‰ (worklog.md:1960-1963).
The old ABI-v2 engine (quick→deep→carve order) showed the same stall at 76.1% = ~1% into its
carve — both eras stall in EARLY CARVE; the worklog's "76.1% deep-scan stall" name is a misnomer
for the stage, though "advanced" is the correct UI label.

**What the carve stage does at that boundary and why it stalls** (cr_carve.cpp):
- The sweep walks **1 MiB windows** (`WINDOW = 1<<20`, +4,096 overlap — cr_carve.cpp:45-46) with a
  **per-byte dispatch loop** `for (u64 off = 0; off + 12 <= want; ++off)` (cr_carve.cpp:597) —
  per position: free-map cluster gates, occupancy-interval gates, first-byte magic dispatch,
  multi-row trial validation, raw-sign KiB-window tail scans.
- **Progress and bytes only tick at window granularity**: single-thread path reports
  `progress(wi*WINDOW, total)` per window (cr_carve.cpp:1530-1537); parallel path reports
  `done*WINDOW` only when a window COMPLETES (cr_carve.cpp:1560-1566); `bytes_scanned` accumulates
  per completed window (cr_carve.cpp:1388). A window whose candidate validation grinds produces
  **zero permille advance, zero bytes_scanned advance, zero new files** — the frozen UI.
- **Reads go quiet while the matcher grinds**: candidate validators read through the
  window-serving `WindowDevice` — in-window reads are memcpys off the already-read buffer,
  "zero device traffic" (cr_carve.cpp:445-456). Hence the io-timeline "reads idle from +6 min"
  (worklog.md:1876-1888) while CPU grinds on stale free-space content — dense on a real disk
  (used clusters are gated by the $Bitmap/occupancy gates; FREE clusters full of stale file
  content are scanned byte-by-byte).
- **No intra-window cancellation checkpoint**: cancel is checked only at window START
  (cr_carve.cpp:1531,1550); the 50 ms CancelPoolWatcher aborts QUEUED windows only. A running
  `scan_one_window` grinds to completion — BF-25's unbounded drain ("the carve matcher grinds in
  memory with no cancellation checkpoint until it finishes", worklog BEHAVIOR-FINDINGS.md:39),
  `cr_ui_scan_busy` stays 1 until the matcher finishes, and the bridge's save is refused.
- During the grind the event stream goes **fully silent** (progress events coalesce by permille
  advance — none; the 5 s heartbeat updates liveness but the bridge does NOT re-emit scan-progress
  on heartbeats, events.rs:326-329 leaves `emit=false`) — so percent, files, bytes, ETA and
  throughput all freeze while deviceConnected stays true. That is precisely the user's
  "jumps to 75% advanced scanning … never moves" presentation.

Secondary presentation defects of the same model (worth Phase 1 instrumentation):
- With carve disabled (CLI `--no-carve`; the bridge never does this), the deep stage's weighted
  value maxes at 750 ⇒ the UI sits at **exactly 75.0% for the entire deep stage** (remnant sweep +
  lost discovery + candidate scans) then jumps to 100%.
- With carve enabled, the deep stage (which runs LAST) executes at a displayed overall already
  pinned at 100% (tel_stage_begin_ resets deep telemetry to 0 — cr_scan.cpp:456-459 — but
  overall_permille is monotonic-max, cr_scan.cpp:481-482/518-519): minutes of real work at "100%".
- The old-run shape "43%→76.1%" (drive-ui.mjs:648) is the pre-N-181 order's same early-carve stall.

## Results contract (units audit)

- **Snapshot rows** `cr_ui_node_info` (cr_ui.h:1004-1036): link indices; `node_id` (stable,
  recovery/preview addressing); fs_kind; record_ref (NTFS record number, 0 carved); origin
  (0 structured/1 carved/2 lost/3 virtual); deleted (0 active/1 deleted/2 partial/3 damaged);
  confidence 0..4; grade 0..2 (S-023); overlap; logical_size / allocated_size — **BYTES**;
  created/modified/changed/accessed — **100ns units since 1601** (FILETIME); attributes (0x10 =
  dir); extent_count/resident_bytes; carve_format/carve_confidence; hash_valid + sha128[16];
  name two-call. `cr_ui_query_row.size` = logical bytes; `size_min/max`, `modified_from/to`
  (100ns-1601) in `cr_ui_query` (cr_ui.h:1072-1101). Disk-map bad_ranges = absolute-byte
  [start,len] pairs; carve-format min/max_size bytes (raw-sign KiB-converted once, S-015).
- **Every size/offset field in the ABI is BYTES; every progress fraction is permille 0..1000;
  no sector-unit field crosses the boundary** (image open bounds, geometry, volume extents,
  node sizes, preview offsets/`placed`, recovery read_chunk, error spans, tally counts). NVMe
  data-units are converted ×512,000 before crossing (cr_ui.h:1525-1531). **A units-conversion bug
  at the ABI boundary is ruled out for "0 B".**
- **"Files found 21 (0 B)" is a MISSING-FIELD gap, not a units bug**: `cr_ui_event` carries
  files_found/deleted_found/carved_found but **no found-bytes**; `cr_ui_telemetry` and
  `cr_ui_scan_stats` likewise expose only `bytes_scanned` (carve-true). The engine tracks
  `stats.bytes_found = tree_.total_logical()` (N-167, cr_scan.cpp:529) and persists it in the
  session (SessionState.bytes_found, cr_scan.cpp:2699) — but no live ABI surface carries it, so
  the bridge can only fill `dataFoundBytes` at TERMINAL time by summing the frozen snapshot's
  metas (scan.rs:419-441; live init 0 at state.rs:136). Mid-scan the headline is structurally
  "N (0 B)". Additionally `bytes_scanned` counts **only the carve stage's true reads**
  (cr_scan.h:594-596 "carve-true-bytes only"; only `run_carve_stage` adds to it,
  cr_scan.cpp:2662) — so "sector progress" (bytesScanned/bytesTotal, total = device geometry,
  scan.rs:288) is 0 during quick+deep and frozen during a carve grind; and the session-row
  bridge maps **bytes_scanned → dataFoundBytes** (sessions.rs:217) — a semantic mismatch
  (scanned ≠ found) on the sessions screen.
- Mid-scan browse: `cr_ui_scan_snapshot` (partial=1) + client-side filtering; core-side query is
  session-only (cr_ui.h:1054-1058). Counts: telemetry counters (files/deleted/carved/lost) +
  `cr_ui_scan_get_stats` at terminal; `cr_ui_scan_assistant` for the recoverability block
  (small_result heuristic: matched forced 0 when total < 100 — cr_ui.h:718-723).

## Error model

Full enum (cr_error.h:25-121), single numeric space shared by every cr_ui return:
- 0x5002 transport: **ERR_OPEN_FAILED 0x50020001**, ERR_READ_FAILED …02, ERR_WRITE_FAILED …03,
  ERR_RANGE_OUT_OF_BOUNDS …04, **ERR_CANCELLED 0x50020005**, ERR_RETRY_EXHAUSTED …06,
  ERR_DEVICE_DISCONNECTED 0x50020007 (N-125).
- 0x5003 parse: BAD_MAGIC …01, **ERR_BAD_CHECKSUM 0x50030002** (identity mismatch),
  BAD_PARAMETER …03, TRUNCATED …04, FIXUP_FAILED …05, CYCLE/DEPTH_LIMIT …06/07,
  **ERR_UNSUPPORTED 0x50030008**, NO_MEMORY …09.
- 0x5004 session: SESSION_FORMAT/IO/VERSION …01..03.
- 0x5005 recovery: **ERR_DEST_OVERLAP 0x50050001**, DEST_UNWRITABLE …02, HASH_MISMATCH …03,
  DEST_FULL …04, DEST_NAME_EXHAUSTED …05, DEST_PATH_TOO_LONG/INVALID/NETWORK …06..08.
- 0x5006 bitlocker: **ERR_FVE_LOCKED 0x50060001**, ERR_FVE_ELEVATION_REQUIRED 0x50060002 (M-105 ACL).
- 0x5007/0x5008: **ERR_HEALTH_UNAVAILABLE 0x50070001**; REPAIR_REFUSED/UNDO_FAILED/VOLUME_LOCK_
  FAILED 0x50080001..03; APPLY_CONSENT_DENIED/STALE/REFUSED/REFRESH_FAILED 0x50080004..07.
- 0x500A ERR_IMAGE_CORRUPT; 0x5009 ERR_INTERNAL.
- **ERR_ELEVATION_REQUIRED = 0x500C0001** (NOT 0x50020001 — that is ERR_OPEN_FAILED; the task
  brief's guess is corrected here). Fresh value space after the driver-era 0x500B family was
  removed (session-96); every raw-disk verb fails fast with exactly this code on a non-elevated
  token (cr_error.h:108-120). Used by cr_ui_open_disk/open_volume*/image_create (cr_ui.cpp:1433,
  1488, 1575, 1725); `cr_ui_health_read` instead returns ERR_OPEN_FAILED unelevated, and
  `cr_ui_fsck_volume` returns ERR_UNSUPPORTED unelevated — an inconsistency hosts must absorb.
- Propagation: engines return codes up through the orchestrator; the shim wraps everything in
  `guarded()` (catch-all → ERR_INTERNAL) and stamps TLS last_error; two-call truncation uses
  ERR_TRUNCATED with partial fills; the read-truth gate converts a fully-blind "success" into
  ERR_READ_FAILED (cr_scan.cpp:1543-1578). Engine codes → UI vocabulary mapping at
  scan.rs:465-473 (0x500C0001 → "elevation-required", 0x50060001 → "device-permission",
  0x50020007 → "device-gone").

## CLI

The engine ships ONE console target: `rrecov` (engine/src/app/rrecov.cpp, 3,952 lines;
engine/tools is build/test tooling, not a CLI). Subcommands (usage block rrecov.cpp:3838-3907,
dispatch 3910-3951):

| Command | Args (essentials) | Notes |
|---|---|---|
| `list` | `[--trim]` | disks; --trim pays the fsutil _popen once (M-076) |
| `list-carve-formats` | — | the ABI registry projection |
| `sessions` | `[--dir D] [--json]` | light-record listing |
| `scan` | `--disk N \| --volume C: \| --image file` + `--quick-only/--no-carve/--whole-disk/--threads/--out/--resume/--log/--autosave-dir/--label/--json/--csv/--filter/--deleted-only/--pause-stdin/--filter-folder/--repair/--repair-apply/--repair-undo/--check/--bitlocker-fvek/--bitlocker-dict/--user-signs` | deep always on; Ctrl-C → exit 3 |
| `tree` | `--in file.crsf [--filter/--deleted-only/--csv]` | browse a session |
| `archive-list` | `--in --disk/--image --node ID [--json]` | M-116 inventory |
| `recover` | `--in (--disk/--image) --to DIR [--deleted-only/--filter/--report]` | G-09 JSON report |
| `health` | `--disk N` | SMART/NVMe/TRIM; all sources dead → non-zero (FB-H04) |
| `unlock` | `--turn-off C:` | M-078 consented decrypt |
| `fix` | `--disk N [--part/--apply/--undo/--undo-apply]` \| `--chkdsk C:` | RawFixer + chkdsk escalation |
| `apply` | `--disk N --session --candidate N [--apply/--undo/--boot-fix]` | S-029 lost-partition entry |
| `image` | `--create --disk N --out file.img` | + .sha256 sidecar |
| `bench` | `--disk N [--mode seq\|rand\|open\|read\|info] …` | direct-transport measurement |

**Exit codes**: 0 success; 1 failure/usage (enumeration failure never exit-0, FB-K01,
rrecov.cpp:797); 2 image-create failure / open failure (rrecov.cpp:2926,2993); **3 = cancelled**
(Ctrl-C; distinct from failure — rrecov.cpp:2065); **4 = elevation required**
(`require_privilege()` prints the ERR_ELEVATION_REQUIRED refusal and returns 4 for every raw-disk
verb — rrecov.cpp:519-537; image targets exempt, rrecov.cpp:539-547); **5 = device disconnected
mid-scan** (N-125, rrecov.cpp:2072-2082, with the reattach-then-resume hint). A fully-unreadable
target refuses with the read-truth text (rrecov.cpp:2084-2097).

---

### Phase-1 hooks this sheet hands forward
1. Instrument the carve stage's per-window timing (window index, duration, hit density) — the
   75%→76.1% freeze is early-carve; confirm on real hardware which window class grinds
   (raw-sign tail scans vs grammar validation vs tree emission).
2. Add a live found-bytes field (or expose `bytes_found` in telemetry/stats) — the "(0 B)" headline.
3. Replace `query_trim_state`'s `_popen` with the in-tree CREATE_NO_WINDOW pattern (D-CONS).
4. Reproduce the 16+ drive-letter volume-enumeration failure (cr_enumerate.cpp:216-218) and the
   model+serial host-side dedup hypothesis for "1 of 3 disks" (Agent D's surface).
5. Re-examine the deep-stage display inversion (work at 100% / 75% pinned) — a stage-model
   presentation defect distinct from the stall itself.
