# CORE-CURRENT-STATE — the current, complete consumer contract of core_recovery @ 12f3edf7

> **Agent D (Task T2-d), consolidation session.** READ-ONLY investigation. Every claim
> below is cited to the tree at `core_recovery` main @ `12f3edf7` (verified:
> `git rev-parse HEAD` = `12f3edf7cb2411dcfd5bc18c9ca57d3705d79ec3`). The v2 reference
> for §3 is `final-prod/engine/include/cr/cr_ui.h` (the header the old Rust bridge
> compiles against) — see the provenance note there: that file is core@22e9657's
> cr_ui.h PLUS three final-prod-local patches, verified by blob diff against
> `git show 22e9657:include/cr/cr_ui.h` (only 3 hunks: the NG-15 log sink, the G-9
> `bus_type` field, the NG-9 `cr_ui_health_read_ex2` + `cr_ui_smart_attr_ex`).
> **The header `include/cr/cr_ui.h` is the normative machine surface**; where a doc
> disagrees with it, the header wins (docs/ABI-CONTRACT.md banner, :8-10). Doc-drift
> findings are flagged inline as [DRIFT].

---

## 1. Product identity & posture

**What it is** (README.md:3-15, verified against src/win/): a user-mode Windows
data-recovery stack, C++20, clean-room rewrite of the reference product's engine layer.
It reads raw disks **directly from user mode**: `\\.\PhysicalDriveN` opens with
`GENERIC_READ` (`| GENERIC_WRITE` only on the fix/apply write paths), share
`R|W|D`, through a **64 KiB read envelope with true byte counts**
(`io::direct_envelope`, include/cr/storage/cr_directalign.h; CONNECTION-GUIDE.md §3).
**There is no kernel driver and no service** — session-96 (v0.7.0) removed the
driver-era RDA transport entirely (RELEASE-NOTES.md:70-108). Engines: NTFS 3.x,
FAT12/16/32, exFAT, ext2/3/4, HFS+, Btrfs, ZFS, ReFS, LDM dynamic disks
(RAID-0/1/RAID-5 assembly), lost-partition discovery, grammar+signature carving
(139-format table + user-taught signs), damage audit + transactional repair, the
`.crsf` durable session container, the safe recovery writer, ATA/NVMe health.
Image targets: flat, VMDK, VHD, VHDX (README.md:57). MIT license.

**The privilege model (the honest elevation contract)** — README.md:10-15,
RELEASE-NOTES.md:83-94, cr_ui.h:318-329:

- Raw-disk verbs — scan / recover / tree `--disk` / health / fix / apply / bench /
  image-with-disk-targets, i.e. ABI `cr_ui_open_disk` / `cr_ui_open_volume[_with_key]` /
  `cr_ui_health_read` / `cr_ui_image_create` / `cr_ui_fsck_volume` (elevated) /
  `cr_ui_apply_lost_partition` — require an **elevated (administrator) process token**.
  The probe is `win::is_elevated()` (TokenElevation, include/cr/win/cr_elevate.h;
  src/win/cr_elevate.cpp). A non-elevated admin's group membership is NOT enough.
- A non-elevated caller gets **`ERR_ELEVATION_REQUIRED` = `0x500C0001`**
  (cr_error.h:120). On the CLI this is **exit code 4** and the refusal text is the
  whole response (src/app/rrecov.cpp:526-537, `require_privilege()` runs before any
  other output). At the ABI level the caller sees: the open/verb's **return code ==
  0x500C0001** and `cr_ui_last_transport_error()` mirrors it (M-030 advisory;
  ABI-CONTRACT §7e: "the returned code equals the advisory"). Nothing elevates
  silently; no helper process; no launch-time manifest — **the host triggers its own
  UAC flow and retries** (cr_ui.h:322-325).
- **No elevation needed**: image targets (`--image` files, `cr_ui_open_image`) and the
  session/report/preview/dictionary verbs (anything that only reads files).
- A failure under an elevated token is a **target error** (`ERR_OPEN_FAILED` etc.) —
  never a silent transport switch; there is no fallback leg (ABI-CONTRACT §7e).
- Quiesce (volume writes, fix/apply): LOCK → DeleteVolumeMountPoint → flush → DISMOUNT
  → OFFLINE, restore ONLINE+mount on release; OFFLINE failure degrades to
  dismount+lock and is logged (CONNECTION-GUIDE.md:81-84).
- BitLocker: legitimate unlock only — `ERR_FVE_LOCKED` 0x50060001 unless auto-unlock /
  user FVEK (`cr_ui_open_volume_with_key`, in-stack XTS-AES) / the explicit dictionary
  aid succeeds; no bypass, no derivation (SECURITY.md, CONNECTION-GUIDE.md §4).
  `ERR_FVE_ELEVATION_REQUIRED` 0x50060002 = the FVE auto-unlock store exists but this
  token cannot read it (M-105, no-service posture).

Latest release: **v0.7.1** (RELEASE-NOTES.md:3-68; git tags through `v0.7.1`, which is
an ancestor of HEAD with exactly 1 commit after it). [DRIFT] The build stamps were NOT
bumped for v0.7.x: CMakeLists.txt:4 `VERSION 0.6.1`, vcpkg.json `version-string 0.6.1`
(last bump f621b37f at the v0.6.1 cut) — a consumer packaging from HEAD gets
0.6.1-stamped binaries despite v0.7.1 notes. The CMake project DESCRIPTION also still
says "on the RDA driver" (stale, the driver is gone).

---

## 2. The cr_ui ABI v4 surface — exhaustive (92 exports)

`include/cr/cr_ui.h` (1603 lines) is the entire public ABI. **Count truth** (docs/API.md
:110-113, re-derived here): **92 exported functions = 95 `CR_UI_API` token occurrences
minus the 3 `#define CR_UI_API ...` macro lines (:67/:69/:72)**. The MSVC `.def`
(src/ui/cr_ui_exports.def) adds exactly ONE extra non-`CR_UI_API` export: the mangled
C++ symbol `?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z` = `cr::log::set_sink`
(the N-048 log seam — see §2.q). On Linux, `libcr_ui.so` statically embeds cr_core with
default visibility, so the full C++ core surface is reachable from `.dynsym` (def-file
comment :7-11).

**ABI rules** (cr_ui.h:23-48): C11, no C++ types cross, no exception escapes (every
entry has a catch-all returning `CR_UI_ERR_INTERNAL` 0x50090001); every function
returns a u32 from the SAME numeric space as cr_error.h (0 = OK); strings in are
NUL-terminated UTF-8; strings out are caller buffers (two-call size pattern) or heap
strings freed with `cr_ui_string_free` (plain `free()` also valid); **zero-initialized
config structs are ALWAYS safe** (0 = "default", never "nothing matches"); handles are
opaque, close everything you open; **no `#pragma pack` — natural alignment** (the
`cr_ui_event` POD carries explicit trailing pads to stay an 8-byte multiple for
transport bridges, :255-260/:268); structs are POD, C-layout stable, append-only.
`CR_UI_ABI_VERSION` = **4** (cr_ui.h:81). [DRIFT] The comment block directly above the
macro (:75-80) narrates the v3 rationale (M-067) but never states why the value is 4 —
the v3→v4 bump (the one sanctioned removal, `cr_ui_driver_status` + the ERR_DRIVER_*
family, session-96) is documented only in RELEASE-NOTES.md:96-108; ABI-CONTRACT.md §9
(:765) still says "`CR_UI_ABI_VERSION` is `3`" — stale vs the header. **The header
wins: the value is 4.**

Stability policy (ABI-CONTRACT §9, matching the header :39-48): ADDING exports,
APPENDING struct fields, adding event `type` values = compatible, NO bump (consumers
detect by export presence / zero-init defaults). Bump on: removing/renaming an export,
changing a field's type/meaning/order (an in-struct array resize included — that is why
v3 exists), changing ownership/threading of an existing symbol, renumbering the error
space. Fields are never reordered or removed within a version. Project semver is
separate; the ABI version is what a UI compiles against.

Grouped census of all 92 exports (signatures verbatim from cr_ui.h; line cites):

### (a) Lifecycle / versioning / diagnostics — 3
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_abi_version(void)` (:105) | returns CR_UI_ABI_VERSION (4). Call first; refuse to run on mismatch. |
| `const char* cr_ui_last_error(void)` (:110) | thread-local diagnostic text for the LAST failing cr_ui_* call on this thread; stable until the next cr_ui_* call on the same thread; never NULL. |
| `void cr_ui_string_free(char* s)` (:113) | frees a heap string returned by this API (free() also valid). |

### (b) Device layer — 9
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_open_image(const char* utf8_path, uint64_t offset, uint64_t bytes, cr_ui_device** out)` (:313) | read-only 512-byte-sector device over an image FILE bounded to [offset, offset+bytes); bytes==0 → to EOF. Portable (the ABI-check/test surface). flat/VMDK/VHD/VHDX. |
| `uint32_t cr_ui_open_disk(uint32_t disk, cr_ui_device** out)` (:330) | Windows only (ERR_UNSUPPORTED elsewhere). Direct `\\.\PhysicalDriveN`, no driver/service/fallback. Non-elevated token → **ERR_ELEVATION_REQUIRED 0x500C0001**; the host runs its own UAC flow and retries. S-051: a whole-disk target reads the partition list at scan setup and scans every table volume. |
| `uint32_t cr_ui_open_volume(char letter, cr_ui_device** out)` (:331) | Windows only. BitLocker probed FIRST: locked → auto-unlock attempt → `ERR_FVE_LOCKED` 0x50060001 (or `ERR_FVE_ELEVATION_REQUIRED` 0x50060002). Returns the volume's slice on its parent disk (kept alive internally). |
| `uint32_t cr_ui_last_transport_error(void)` (:340) | M-030 advisory: refusal code from the LAST open (or the surviving target error). Process-global, never gates an open's return code, never clears. |
| `uint32_t cr_ui_open_volume_with_key(char letter, const char* utf8_hex_fvek, cr_ui_device** out)` (:352) | M-033: locked-volume open with user FVEK — 64 hex chars (AES-128-XTS) or 128 (AES-256-XTS), key‖tweak-key concatenated. ERR_BAD_PARAMETER on malformed/weak key; ERR_UNSUPPORTED off-Windows. |
| `uint32_t cr_ui_unlock_volume_with_dictionary(char letter, const char* const* utf8_user_passwords, uint32_t count)` (:368) | N-064 **explicit-consent** dictionary unlock (shipped 228-entry table first, then user candidates); the host calls it only from its user-initiated unlock surface. OK when a candidate unlocked; ERR_FVE_LOCKED when the set failed; ERR_UNSUPPORTED off-Windows. Attempt lines carry the candidate index only, never the password. |
| `void cr_ui_device_close(cr_ui_device* dev)` (:372) | close; a device must outlive every scan/preview/recovery using it (they borrow, never own). |
| `uint32_t cr_ui_device_geometry(cr_ui_device* dev, cr_ui_geometry* out)` (:381) | `cr_ui_geometry { u64 total_bytes; u32 logical_sector, physical_sector; u64 sector_count; }` (:374-379). 4Kn-aware. |
| `uint32_t cr_ui_device_transport(cr_ui_device* dev, char* buf, uint32_t cap, uint32_t* needed)` (:387) | transport truth: `"direct"` or `"image"`, NUL-terminated into buf. Two-call: buf==NULL → *needed = size. |

### (c) Enumeration (Windows layer; ERR_UNSUPPORTED elsewhere) — 3
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_enumerate_disks(cr_ui_disk_info* out, uint32_t* count)` (:447) | two-call (out==NULL → *count = needed); failure returns its error code, NEVER a silent empty list (G-20). **Always fresh** — every call re-queries the OS, no cache (N-098; calling again IS the refresh). |
| `uint32_t cr_ui_enumerate_disks_ex(cr_ui_disk_info* out, uint32_t* count, uint32_t flags)` (:448) | M-076 opt-in flags form. `CR_ENUM_FLAG_TRIM 0x1` (:432) adds the per-row TRIM datum (one fsutil spawn per invocation, OS-global value stamped on every row; failed query leaves trim_known==0). **Unknown bits refused with ERR_BAD_PARAMETER.** |
| `uint32_t cr_ui_enumerate_volumes(cr_ui_volume_info* out, uint32_t* count)` (:449) | letter/disk correlation + FVE flag. |

`cr_ui_disk_info` (:392-416): `u32 number; u64 size_bytes; u32 logical_sector,
physical_sector; u32 removable /*1=USB/SD bus*/; u32 layout /*0 none,1 MBR,2 GPT,3 EBR
(N-057)*/; u32 table_ok; char model[128], serial[128] /*UTF-8, may be empty*/;
u32 trim_known, trim_on /*M-076 append; valid only when trim_known==1*/`.
`cr_ui_volume_info` (:418-426): `char letter /*'C' style; 0 absent*/; u32 disk;
u64 offset_on_disk, bytes; char fs_name[16], label[64] /*UTF-8*/; u32 bitlocker /*1=FVE*/`.

### (d) Scan lifecycle — 19
One `cr_ui_scan` = one staged pipeline (quick → deep → carve) over one attached device
view, mirroring the CLI `scan` flow (identity card, LDM cross-disk resolver, checkpoint
sink, save-on-demand). `run()` is synchronous on the CALLER's thread; pause/cancel/
telemetry/snapshot are safe from any thread while it runs (cr_ui.h:462-470).

| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_scan_create(const cr_ui_scan_config* cfg, cr_ui_scan** out)` (:609) | cfg zero-init = defaults. |
| `void cr_ui_scan_close(cr_ui_scan* s)` (:610) | |
| `uint32_t cr_ui_scan_attach_device(cr_ui_scan* s, cr_ui_device* dev)` (:615) | attach BEFORE run(); borrowed. Windows: identity card auto-filled from enumeration unless cfg supplied one. |
| `uint32_t cr_ui_scan_run(cr_ui_scan* s)` (:620) | runs to completion on the calling thread. Returns 0, an engine error, or CR_UI_ERR_CANCELLED — **tree stays browsable** (snapshot or save afterwards). |
| `uint32_t cr_ui_scan_pause(cr_ui_scan* s)` (:623) | blocks engines cooperatively. |
| `uint32_t cr_ui_scan_resume(cr_ui_scan* s)` (:624) | releases a pause. |
| `uint32_t cr_ui_scan_cancel(cr_ui_scan* s)` (:625) | run() → ERR_CANCELLED. |
| `uint32_t cr_ui_scan_busy(cr_ui_scan* s, uint32_t* out)` (:638) | N-075: live "run() executing" state; any thread, any time; synchronous. |
| `uint32_t cr_ui_scan_stopping(cr_ui_scan* s, uint32_t* out)` (:639) | N-075: the live cancel REQUEST (distinct from telemetry's post-run `cancelled`). |
| `uint32_t cr_ui_scan_set_event_fn(cr_ui_scan* s, cr_ui_event_fn fn, void* user)` (:303) | subscribe BEFORE run() (mid-run = ERR_BAD_PARAMETER); fn==NULL unsubscribes; ONE sink per scan handle. |
| `uint32_t cr_ui_scan_telemetry(cr_ui_scan* s, cr_ui_telemetry* out)` (:661) | live snapshot, any thread any time. |
| `uint32_t cr_ui_scan_get_stats(cr_ui_scan* s, cr_ui_scan_stats* out)` (:701) | final counters. |
| `uint32_t cr_ui_scan_assistant(cr_ui_scan* s, cr_ui_assistant_stats* out)` (:730) | S-023 grading stats (the og RecoveryAssistant block). Valid after run() returns OK or after resume of a completed session. |
| `uint32_t cr_ui_scan_checkpoint_count(cr_ui_scan* s, uint64_t* out)` (:738) | G-13 checkpoints emitted. |
| `uint32_t cr_ui_scan_repair_save_undo(cr_ui_scan* s, const char* utf8_path)` (:747) | S-008: persist the pre-scan repair's undo log (durable CRC-checked container). BAD_PARAMETER when nothing was applied / scan not run. Restore via `rrecov fix --disk N --undo-apply <file>`. |
| `uint32_t cr_ui_scan_save_session(cr_ui_scan* s, const char* utf8_path)` (:825) | full truthful .crsf (same record set as CLI). Valid after run() — **including a cancelled one** — or after a resumed run. |
| `uint32_t cr_ui_scan_save_cancel(cr_ui_scan* scan)` (:835) | N-060 **SAVE-SCOPED** cancel: never touches the scan's cancel token; aborted save returns ERR_CANCELLED, final path keeps previous content, no .tmp remains; no-op if no save in flight; any thread. |
| `uint32_t cr_ui_scan_snapshot(cr_ui_scan* s, cr_ui_snapshot** out)` (:841) | G-02 immutable value snapshot; callable mid-run (partial=1) and after (partial=0). |
| `uint32_t cr_ui_scan_verify_identity(cr_ui_scan* s)` (:849) | resume revalidation: compares the device's first 512 bytes against the session's boot hash; **ERR_BAD_CHECKSUM 0x50030002 on mismatch**; 0 on match or unverifiable (legacy). Call after resume+attach, before run(). |

`cr_ui_scan_config` (:505-607) — the full field set (zero-init = all defaults):
`u32 stage_mask` (0 = quick|deep|carve = 7; bits 1/2/4), `u32 whole_disk` (1 =
lost-partition discovery; needs a disk view), `u32 fs_hint` (0 auto-probe; values =
orchestrator::FsFamily: 1 NTFS, 2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS,
11 UDF; 5 reserved-on-wire, 10 BitLocker probe-only — :472-476), `u32 quick_weight/
deep_weight/carve_weight` (0 = 40/35/25), `u32 threads` (0 = auto), `const char*
volume_label_utf8` (truncated to 99 chars, N-090), `volume_model_utf8 /
volume_serial_utf8 / volume_firmware_utf8 / volume_wwn_utf8 / volume_interface_utf8`
(identity-card overrides; auto-filled from enumeration when absent), `const char*
checkpoint_path_utf8` (stage-boundary auto-checkpoints, overwrite-in-place),
`cr_ui_progress_fn progress` + `void* progress_user`, `u32 pre_scan_repair` (S-008) +
`cr_ui_repair_consent_fn repair_consent` + `void* repair_consent_user`, `u32
memory_warn_mb` (N-062; 0 = OFF; the 5 s watchdog Warns through the cr::log seam),
`const char* raw_category_name_utf8` (M-029; NULL/"" = default), `u32 pre_scan_check`
(M-041 damage audit; report never aborts the scan), `const char* autosave_dir_utf8`
(W1c/R-52; completion auto-save, see CR_EVT_AUTOSAVED), `const char* const*
filter_folders_utf8` + `u32 filter_folder_count` (M-073 scan-time folder filter;
persists as REC_SCAN_FILTER so resume keeps scope).

`cr_ui_repair_plan` (:493-501): `u32 detected; char fs_family[8]; u32 action_count;
u32 refused; char reason[160]; u32 advisory_count`. The consent callback
`uint32_t (*cr_ui_repair_consent_fn)(void* user, const cr_ui_repair_plan* plan)`
(:503) fires ONCE on the run thread before any engine read when
`pre_scan_repair==1` and a repairable plan exists. **Nonzero return = execute the
transactional repair** (through the shim-opened write view); zero / NULL callback /
no write view = surfaced via CR_EVT_REPAIR + stats and NEVER executed (default-deny).
Undo retrievable via `cr_ui_scan_repair_save_undo`.

### (e) Lost-partition apply (consent-gated) — 3
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_apply_lost_partition(cr_ui_device* dev, const cr_ui_apply_candidate* cand, uint32_t rebuild_table, cr_ui_apply_consent_fn consent, void* consent_user, cr_ui_apply_result* out)` (:808) | S-029: plan (read-only) → consent callback (fires exactly once, full POD plan, BEFORE any write; zero/NULL = denied, nothing written) → transactional write → undo. Works on a WHOLE-DISK view only (image at offset 0 or a disk device; volume slices refused — the table lives at LBA 0). |
| `uint32_t cr_ui_apply_undo_last(cr_ui_device* dev, cr_ui_apply_result* out)` (:815) | byte-exact sector restore of the last apply on this device + properties refresh; idempotent on bytes. |
| `uint32_t cr_ui_apply_save_undo(cr_ui_device* dev, const char* utf8_path)` (:820) | persist the undo log (durable CRC-checked container, loadable by `rrecov fix --undo-apply`). BAD_PARAMETER when no undo log exists. |

`cr_ui_apply_candidate` (:757-775): `u64 start_lba; u64 sector_count; u32 fs`
(lost::CandidateFs: 0 NTFS, 1 FAT, 2 exFAT, 4 GPT-table (refused), 5 unknown
(refused), 6 ext2, 7 HFS+, 8 Btrfs, 9 ZFS, 10 ReFS, 11 BitLocker); M-137 tail:
`u32 disk_layout` (0 none, 1 MBR, 2 EBR, 3 GPT, 4 GPT-damaged), `u32 disk_flags`
(bit0 target is dynamic/LDM, bit1 table parsed). `cr_ui_apply_plan` (:777-787):
`u32 refused; u32 scope /*0 single-entry, 1 rebuild-table*/; u64 start_lba,
sector_count; u32 mbr_type; u32 slot /*0..3*/; u32 regenerate_signature; u32
signature; char reason[160]`. `cr_ui_apply_result` (:791-799): `u32 applied; u32
outcome` (0 applied, 1 consent denied, 2 refused, 3 table changed since plan,
4 applied-but-refresh-tail-failed (undo valid), 5 error — see return code);
`u64 undo_entries; char reason[160]`.

### (f) Sessions (.crsf) — 11
A loaded session = stable browsable tree + stats + volume info; queries/previews/
recovery enqueue directly against it (cr_ui.h:851-855).

| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_session_load(const char* utf8_path, cr_ui_session** out)` (:857) | |
| `void cr_ui_session_close(cr_ui_session* sess)` (:858) | |
| `uint32_t cr_ui_session_resume_last(const char* utf8_dir, cr_ui_session** out)` (:870) | M-154 pointed resume: reads the `last_scan.info` sidecar in utf8_dir and loads the AUTO session it names (no directory scan). ERR_OPEN_FAILED on dead pointer (no sidecar / missing file / not an auto session). |
| `uint32_t cr_ui_session_get_info(cr_ui_session* sess, cr_ui_session_info* out)` (:888) | family, stage masks, slice, sectors, label + label_full, identity card, boot_hash. |
| `uint32_t cr_ui_session_stats(cr_ui_session* sess, cr_ui_scan_stats* out)` (:889) | |
| `uint32_t cr_ui_session_snapshot(cr_ui_session* sess, cr_ui_snapshot** out)` (:890) | |
| `uint32_t cr_ui_session_resume(cr_ui_session* sess, const cr_ui_scan_config* original_cfg, cr_ui_scan** out)` (:897) | the CLI `scan --resume` flow: fresh tree, validates completion mask, returns a scan handle continuing where the session stopped. Attach the ORIGINAL device, optionally verify_identity, then run(). original_cfg mirrors the interrupted run's config. |
| `uint32_t cr_ui_session_regrade(cr_ui_session* sess, const uint64_t* node_ids, uint32_t count, uint32_t* regraded)` (:736) | S-023 per-selection re-grade; only grade-1 (average) nodes eligible (0 and 2 are sticky/final). |
| `uint32_t cr_ui_session_list(const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count)` (:954) | W1a (R-56): one row per *.crsf, decoded from LIGHT records only (SESSION_STATE + VOLUME_INFO; heavy payloads skipped BY TYPE before any payload I/O). Two-call. Rows fill NEWST-FIRST by embedded save time (deterministic path-byte tiebreaks); capacity < needed → ERR_TRUNCATED with the newest prefix filled. Unreadable files still get a row (valid=0 + error). ERR_OPEN_FAILED only when the directory itself cannot be read. |
| `uint32_t cr_ui_session_last_autosave_path(const char* utf8_dir, char* out_path, uint32_t cap)` (:972) | the pointer-sidecar query: OK + path when the pointer exists AND the file is present; **ERR_RANGE_OUT_OF_BOUNDS 0x50020004** when no live pointer names a present file (host falls back to session_list). |
| `uint32_t cr_ui_session_load_with_events(const char* utf8_path, cr_ui_event_fn fn, void* user, volatile uint32_t* cancel_flag, cr_ui_session** out)` (:992) | W1b (R-54): CR_EVT_SESSION_LOAD_PROGRESS fires coalesced from the CALLING thread; cancel_flag is CALLER-OWNED, polled per node even when fn==NULL; on cancel → ERR_CANCELLED and *out untouched (a torn load never half-claims state). fn=NULL + cancel_flag=NULL == plain session_load. |

`cr_ui_session_info` (:872-886): `u32 fs_family; u32 stage_done_mask; u32
configured_mask; u64 slice_offset, slice_bytes; u32 logical_sector, physical_sector;
char label[32]; char label_full[100] /*N-090 append; == label on v1..v5*/; u64
scanned_bytes, elapsed_ms; char model[48]; char serial[32]; u32 boot_hash /*0 =
unverifiable (v1)*/`. `cr_ui_session_row` (:933-952): `u32 valid, error, fs_family,
stage_done_mask, configured_mask; u64 files_found, deleted_found, carved_found,
bytes_scanned, elapsed_ms, save_unix /*0 = pre-v5: show file mtime, never fabricated*/;
char label[32]; char path[CR_UI_SESSION_ROW_PATH=1024]; char label_full[100]`.

### (g) Snapshot enumeration (the mid-scan browse surface) — 5
Flat, index-addressed rows; parent/child/sibling links as indices, `CR_UI_SNAP_NONE =
0xFFFFFFFF` when absent (:1002). The UI filters client-side on live scans (the og has
no core-side live query); core-side query is session-only (§h).

| Export (line) | Contract |
|---|---|
| `void cr_ui_snapshot_close(cr_ui_snapshot* snap)` (:1038) | the source scan/session may close after the LAST row has been READ, not while rows are being read (values are copied — G-02). |
| `uint32_t cr_ui_snapshot_node_count(cr_ui_snapshot* snap, uint32_t* out)` (:1039) | |
| `uint32_t cr_ui_snapshot_partial(cr_ui_snapshot* snap, uint32_t* out)` (:1040) | 1 = mid-scan capture. |
| `uint32_t cr_ui_snapshot_node(cr_ui_snapshot* snap, uint32_t index, cr_ui_node_info* out, char* name_buf, uint32_t name_cap)` (:1046) | row `index` into *out. name_buf NULL → *out->name_len = needed; non-NULL → copies min(cap-1, name_len) + NUL, sets name_len to the FULL length (compare with cap-1 to detect truncation); capped copy still returns 0, full-truncation returns ERR_TRUNCATED. |
| `uint32_t cr_ui_snapshot_dfs(cr_ui_snapshot* snap, uint32_t* indices, uint32_t* count)` (:1052) | depth-first emission order (byte-parity with the live serializer). Two-call; capacity < needed → ERR_TRUNCATED. |

`cr_ui_node_info` (:1004-1036) — the row shape (field-by-field):
`u32 parent, first_child, next_sibling, prev_sibling` (indices); `u64 node_id`
(stable id; recovery/preview addressing); `u32 fs_kind` (FsKindCode, cr_types.h:81+);
`u64 record_ref` (NTFS record for hard-link grouping; 0 carved); `u32 origin`
(tree::Origin: 0 structured, 1 carved, 2 lost, 3 virtual); `u32 deleted` (0 active,
1 deleted, 2 partial, 3 damaged); `u32 confidence` (ordinal 0..4); `u32 grade`
(S-023: 0 low, 1 average, 2 high — the og's CRecoverability verdict, sticky-2);
`u32 overlap` (twin/overlap rule flag); `u32 is_full` (roots/virtuals carry role +
stats); `u64 logical_size, allocated_size`; `u64 created, modified, changed,
accessed` (100ns since 1601); `u32 attributes` (DOS; 0x10 = directory); `u32 flags`;
`u32 extent_count` (info only — the writer resolves extents); `u32 resident_bytes`;
`u32 carve_format, carve_confidence`; `u32 hash_valid; u8 sha128[16]`; `u32 name_len`
(UTF-8 bytes excl. NUL); M-101 tail: `u64 version_group` (NTFS namespace-identity
hash; 0 = none), `u32 tags_extracted`.

### (h) Query (session trees) — 3
Core-side filter/sort/paging over a loaded session (G-12) — the improvement beyond
the reference (whose filtering is client-side). **Mid-scan filtering is snapshot
enumeration + client-side predicates.**

| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_query_run(cr_ui_session* sess, const cr_ui_query* q, cr_ui_query_row** rows, uint32_t* row_count, uint64_t* total_matched)` (:1116) | heap rows; free with cr_ui_query_rows_free. |
| `void cr_ui_query_rows_free(cr_ui_query_row* rows, uint32_t row_count)` (:1119) | |
| `uint32_t cr_ui_query_apply_buckets(cr_ui_query* q, uint32_t date_bucket, uint32_t size_bucket, uint64_t date_custom_from, uint64_t date_custom_to, uint64_t size_custom_min, uint64_t size_custom_max)` (:1138) | M-029: translates the og filter-bar action IDs into range fields in place. Date 0x100 Today, 0x101 Yesterday, 0x102 Last 7 days, 0x103 Last 1 month, 0x105 Custom; Size 0x1000 <512KB, 0x1001 512KB~1MB, 0x1002 1MB~512MB, 0x1003 512MB~1GB, 0x1004 >1GB, 0x1006 Custom. 0 leaves the axis untouched. **Skipped IDs (0x104, 0x1005) and unknown values are REJECTED with ERR_BAD_PARAMETER.** Date windows are day-truncated LOCAL time from the host clock. |

`cr_ui_query` (:1072-1101), zero-init = match-all: `u32 category_mask` (bits 1 image,
2 video, 3 audio, 4 document, 5 archive, 6 other, 7 email, 8 bookmarks); `u64
size_min, size_max` (0 = unbounded); `u64 modified_from, modified_to` (0 = unbounded;
100ns-1601 units); `u32 confidence_min` (0..4); `const char* const* extensions`
(lowercased WITH dot) + `u32 extension_count`; `const char* name_substring` (UTF-8,
ASCII-case-insensitive); `const char* path_prefix` (full-path prefix,
ASCII-case-insensitive); `const char* path_substring` (M-072); `u32 state_mask`
(M-072: bit0 Active, bit1 Deleted, bit2 Partial, bit3 Damaged; 0 = all); `u32
deleted_only`; `u32 origin_mask` (0 = all); `u32 sort` (0 none/emission, 1 name,
2 size, 3 modified, 4 confidence, 5 path, 6 grade (desc = High first), 7 type
(M-029: FileFilter.xml iconIndex category order, then extension, then name)); `u32
dir` (0 asc, 1 desc); `u64 offset; u32 limit` (0 = default 100 — HOST-side model,
no og paging parity claimed); `u32 count_only`; M-101: `u64 version_group` (nonzero =
only that group's members), `u32 tags_only`.
`cr_ui_query_row` (:1103-1114): `u64 node_id; char* name; char* path` ('/'-joined;
both heap, freed by rows_free); `u64 size, modified; u32 origin, deleted, confidence,
category; u32 grade`; M-101: `u64 version_group; u32 tags_extracted`.

### (i) Carve registry + user signs — 7
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_carve_format_count(void)` (:1167) | total enumerable formats: grammar-validator rows first (FormatId order), then raw-sign rows. Deterministic, stable across releases (both tables only ever APPEND). No handle; any thread; immutable for the library's lifetime. |
| `uint32_t cr_ui_carve_format_info(uint32_t index, cr_ui_carve_format* out)` (:1170) | BAD_PARAMETER on out-of-range. |
| `uint32_t cr_ui_scan_carve_tally_count(cr_ui_scan* s)` (:1190) | N-140: live per-format carve tally slot bound (0 = no carve stage ran / session predates the tally). |
| `uint32_t cr_ui_scan_carve_tally(cr_ui_scan* s, uint32_t index, uint64_t* out)` (:1191) | count at index; the LAST slot (== carve_format_count()) is the user-sign aggregate. No tree walk required. |
| `uint32_t cr_ui_register_user_signs(const char* json_bytes, uint32_t json_len)` (:1209) | M-048: parse + validate + install the og's EUUserData-shaped JSON ATOMICALLY (bad payload leaves the previous set). Process-global, thread-safe. Re-registration REPLACES. |
| `uint32_t cr_ui_user_sign_count(void)` (:1220) | |
| `uint32_t cr_ui_user_sign_info(uint32_t index, cr_ui_user_sign* out)` (:1221) | |

`cr_ui_carve_format` (:1153-1162): `u32 layer` (0 grammar validator, 1 raw-sign
prefilter); `char ext[24]` (lowercase, no dot); `u64 min_size, max_size` (0 =
unbounded); `u16 base_confidence` (0..1000); `u32 magic_len` (0..16), `magic_offset`;
`u8 magic[16]`. `cr_ui_user_sign` (:1211-1218): `char ext[9]; char mime[81]; u32
offset` (0..1024); `u32 sign_len` (1..48); `u8 bytes[48]` — the v3/M-067 widths (the
og's longest MIME is 73, longest sign 44 B, max offset 526).

### (j) Disk map (visualization data producer) — 2
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_disk_map(cr_ui_session* sess, cr_ui_snapshot* snap, const cr_ui_disk_map_geometry* want, cr_ui_disk_map_geometry* got, uint8_t** allocation, uint8_t** coverage, uint8_t** found, uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap, uint32_t* bad_count)` (:1264) | GAP-S6-11: geometry + per-cell overlays. Exactly ONE of sess/snap. `want` optional for sessions, REQUIRED for snapshots (snapshots are value objects with no view); `got` always reports the geometry actually used. Each overlay is an optional heap array of got->cell_count bytes (at least one required). allocation: 0 unknown, 1 used, 2 free (S-009 engine-bitmap truth), 3 bad; coverage: 0 unscanned, 1 scanned; found: distinct files covering the cell (capped 255); confidence: max ordinal 0..4. bad_ranges: u64 PAIRS [start,len] absolute bytes; bad_cap < needed → ERR_TRUNCATED after filling. |
| `void cr_ui_disk_map_free(uint8_t* overlay)` (:1272) | frees ONE overlay array (call per array; NULL is a no-op). |

`cr_ui_disk_map_geometry` (:1255-1262): `u64 start_byte, total_bytes, cell_bytes`
(in: 0 = auto → engine cluster size or 64 KiB default), `cell_count` (out),
`u32 cluster_bytes` (out; 0 = no cluster model), `u32 fs_family` (out).

### (k) Preview + archive inventory — 9
Bounded, chunked logical reads of one node with NO recovery pass (G-10). The reader
owns a private copy of the node's properties, so the source session/snapshot may
close while a preview lives; **the DEVICE must outlive it** (it supplies the bytes).

| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_preview_open_snapshot(cr_ui_device* dev, cr_ui_snapshot* snap, uint32_t index, cr_ui_preview** out)` (:1284) | snapshot row index addressing. |
| `uint32_t cr_ui_preview_open_session(cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, cr_ui_preview** out)` (:1286) | session node_id addressing. |
| `void cr_ui_preview_close(cr_ui_preview* pv)` (:1288) | |
| `uint32_t cr_ui_preview_size(cr_ui_preview* pv, uint64_t* out)` (:1290) | |
| `uint32_t cr_ui_preview_read(cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t len, uint32_t* placed)` (:1296) | reads [offset, offset+len); *placed = bytes actually assembled (short only at logical EOF); **len capped at 4 MiB per call**. Unreadable spans are zero-filled AND reported — never silently swallowed. |
| `uint32_t cr_ui_preview_metadata(cr_ui_preview* pv, cr_ui_metadata* out)` (:1314) | bounded metadata: kind image\|video\|audio\|document\|archive\|text\|unsupported; truncation reported via `truncated`, never silent. |
| `uint32_t cr_ui_archive_entry_count(cr_ui_preview* pv, uint32_t* out)` (:1340) | M-116: browse-inside inventory. Extracted at the FIRST archive call on the handle, cached for the handle's lifetime (one stable listing per preview; non-archive nodes report count 0, never an error). 512-entry hostile bound; damaged-zip partial salvage. |
| `uint32_t cr_ui_archive_entry_at(cr_ui_preview* pv, uint32_t index, cr_ui_archive_entry* out)` (:1341) | out-of-range → ERR_RANGE_OUT_OF_BOUNDS. |
| `uint32_t cr_ui_preview_unreadable(cr_ui_preview* pv, uint64_t* total_bytes, uint32_t* range_count)` (:1346) | the truthful failure map (zero-filled spans). |

`cr_ui_metadata` (:1302-1312): `char kind[24], format[24]; u32 width, height; char
orientation[8]` (EXIF "1".."8"; "" absent); `char datetime[40]` (EXIF
DateTimeOriginal); `u64 duration_ms; char codec[32]; char text_snippet[256]; u32
truncated`. `cr_ui_archive_entry` (:1332-1338): `char name[260]` (archive-native
UTF-8 pass-through); `u64 size, compressed; u32 crc_present` (CD CRC-32 != 0 — the
damaged-entry marker); `u32 name_truncated`.

### (l) Thumbnail — 2
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_thumbnail_generate(cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, const cr_ui_thumbnail_caps* caps, uint8_t* rgba, uint32_t rgba_cap, cr_ui_thumbnail_info* info)` (:1415) | M-102: one bounded, SYNCHRONOUS generation per node; display-oriented RGBA8 (EXIF orientation transform BEFORE downscale, S-043). TWO-CALL: rgba==NULL is the SIZE PROBE (generation runs; info->bytes_needed carries the answer; pixels discarded); rgba_cap too small → ERR_NO_MEMORY with bytes_needed set. Status mapping verbatim: decode refusal → ERR_TRUNCATED (info->truncated set); in-band truncation is NOT an error (crosses via info->truncated on OK); phase-2 refusals → ERR_UNSUPPORTED (source_format carries the family). M-115: nodes of size <= 20 refused 0x50030008 (no grade promotion); a decode REFUSAL with clean reads still promotes grade to 2 (the og promotes on read evidence); span-damaged/budget-exhausted/unsupported never promote. M-083: successful generation promotes the node's grade to 2 (monotonic, sticky). |
| `uint32_t cr_ui_thumbnail_format_supported(const char* utf8_format)` (:1422) | true for jpeg/png/bmp/gif/tiff/psd/pnm/hdr/raw. |

`cr_ui_thumbnail_caps` (:1371-1377): `u32 max_edge` (0 = 256); `u64 max_pixels`
(0 = 64 MP); `u32 max_read` (0 = 200 MB / 0xC800000); `u32 max_work` (0 = 4 Mi);
`u32 allow_embedded` (0 = ON — the EXIF fast path is ALWAYS preferred; there is no
off position by design, N-137). `cr_ui_thumbnail_info` (:1379-1389): `u32 width,
height, bytes_needed; char source_format[16]; u8 from_embedded, truncated; u8 _pad[2]`.

### (m) Recovery (the safe writer) — 12
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_recovery_create(cr_ui_device* source, const cr_ui_recovery_config* cfg, cr_ui_recovery** out)` (:1442) | SafeWriter over an injected DestIo (Win32 atomic temp+rename; portable stdio elsewhere). |
| `void cr_ui_recovery_close(cr_ui_recovery* rec)` (:1444) | |
| `uint32_t cr_ui_recovery_set_destination(cr_ui_recovery* rec, const char* utf8_path)` (:1451) | destination root (normalized with trailing separator). Windows drive-letter destinations arm the **F12 destination-overlap refusal** against disk-backed sources (v0.7.1 fix: armed ONLY when a destination extent sits on the SOURCE disk — cross-disk numeric comparisons no longer refuse); image-file sources have file-internal extents and cannot overlap. |
| `uint32_t cr_ui_recovery_enqueue_node(cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` (:1458) | one node (file, or directory as a structure item). Returns per-call errors (e.g. 0x50050001 ERR_DEST_OVERLAP — count refusals by return code). |
| `uint32_t cr_ui_recovery_enqueue_subtree(cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id)` (:1460) | whole subtree: every directory on the path becomes a mkdir item (empty leaves materialize), then every file. |
| `uint32_t cr_ui_recovery_enqueue_snapshot(cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` (:1464) | G-02b value path — mid-scan recovery, race-free by construction. |
| `uint32_t cr_ui_recovery_enqueue_subtree_snapshot(cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index)` (:1466) | |
| `uint32_t cr_ui_recovery_queue_count(cr_ui_recovery* rec, uint32_t* out)` (:1469) | live queue depth. |
| `uint32_t cr_ui_recovery_process(cr_ui_recovery* rec)` (:1475) | processes to completion on the CALLING's thread (read extents → temp → verify → atomic commit). ERR_CANCELLED on cancel(); **already-committed items stay committed; the report tells the truth about the rest** (pending items). |
| `uint32_t cr_ui_recovery_cancel(cr_ui_recovery* rec)` (:1476) | any thread. |
| `uint32_t cr_ui_recovery_get_stats(cr_ui_recovery* rec, cr_ui_recovery_stats* out)` (:1486) | |
| `uint32_t cr_ui_recovery_report_json(cr_ui_recovery* rec, const char* source_utf8, char** json_out)` (:1491) | the G-09 JSON report (schema: docs/recovery-report-schema.md) as a heap string (cr_ui_string_free). Valid ANY time — unprocessed items report "pending" truthfully. |

`cr_ui_recovery_config` (:1432-1440): `u64 read_chunk` (0 = 1 MiB); `u32
rename_attempts` (0 = 64); `u32 skip_hash_verify` (0 = verify ON); `u32
skip_dir_reconstruct` (0 = reconstruct ON, G-08); `u32 conflict` (0 rename, 1 skip,
2 overwrite); `cr_ui_recovery_progress_fn progress; void* progress_user`.
`cr_ui_recovery_stats` (:1478-1484): `u64 queued, done, refused, errors,
hash_mismatch, resumed, salvaged, dirs, nothing_to_recover`.

### (n) Health (Windows layer) — 1
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_health_read(uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs, uint32_t attr_cap, uint32_t* attr_count)` (:1536) | the CLI `health` flow: SMART via ATA passthrough, NVMe health log, SSD heuristic, TRIM state, one flat struct. Read-only. One call fills the report AND up to attr_cap attribute rows; *attr_count returns the needed size either way. |

`cr_ui_smart_attr` (:1498-1504): `u32 id; u32 value, worst; u64 raw` (6 vendor
bytes); `u32 failing; char name[40]`. `cr_ui_smart_report` (:1506-1532): `u32
available; char transport[24]` ("ata-passthrough"/"nvme"/"legacy"); `u32 ata_version,
smart_version, attribute_count, failing_now; u64 reallocated_sectors, pending_sectors,
uncorrectable, power_on_hours, spin_retry, ssd_wear_level, temperature_c;` NVMe block:
`u32 nvme_available, temperature_c_x10, available_spare, available_spare_threshold;
u64 percentage_used, data_units_read, data_units_written, power_on_hours_nvme,
unsafe_shutdowns, media_errors; u32 critical_warning;` media class: `u32 ssd_like
(seek-penalty heuristic), trim_known, trim_enabled;` M-036 (ABI v2) tail: `u64
host_reads, host_writes, controller_busy_time, num_error_log_entries,
data_bytes_read, data_bytes_written`. The **4-status grading model** (Unknown/Good/
Caution/Bad) is the C++-side `health::HealthGrade` (include/cr/health/cr_grade.h:52-57
— pure core, rule-by-rule decoded from og DiskHealth.exe); the ABI row carries the
raw inputs the host grades from.

### (o) Image producer — 1
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_image_create(uint32_t disk, const char* utf8_path, cr_ui_recovery_progress_fn progress, void* user)` (:1557) | S-040 (R-93): flat raw image of physical disk N. **Class (a) privilege-required** (same ERR_ELEVATION_REQUIRED gate as open_disk). Progress = cumulative-bytes callback (may be NULL). Streamed to `<path>.crimg-tmp`, atomically renamed on completion + `<path>.sha256` sidecar; on cancel/failure the partial is KEPT as `<path>.partial` — reported, never passed off as complete. Flat raw only (VHD/VHDX writers out of scope, D-147). ERR_UNSUPPORTED off-Windows. |

### (p) Volume-level check/repair escalation + BitLocker off — 2
| Export (line) | Contract |
|---|---|
| `uint32_t cr_ui_fsck_volume(char letter, uint32_t flags, int32_t* exit_code)` (:1578) | M-075: launch Windows chkdsk on the volume at `letter`. flags: 0 = check posture (`chkdsk X:`), 1 = fix posture (`/f`); other bits ignored for forward compat. *exit_code = the RAW chkdsk exit (0 no errors, 1 fixed, 2 cleanup/scheduled, 3 errors remain — rerun /f, −1 spawn failed, 259 the 15-min timeout left it running). OK when chkdsk ran; ERR_BAD_PARAMETER non-letter; ERR_OPEN_FAILED no volume mounted at the letter (loud refusal); ERR_UNSUPPORTED when not elevated or off-Windows. The escalation ladder: in-engine pre_scan_check → this endpoint → the host's raw-carve fallback. |
| `uint32_t cr_ui_turn_off_bitlocker(char letter, cr_ui_progress_fn progress, void* user)` (:1596) | M-078: turn BitLocker OFF — the DESTRUCTIVE consent twin of unlock (decrypt, never bypass). Explicit opt-in only; nothing in the scan/open path ever calls it. Dual chain (PowerShell Disable-BitLocker primary, manage-bde -off fallback); the OS decrypt continues in background — **OK means ACCEPTED, not plaintext**. progress fires at request start and settle (the OS decrypt percentage is NOT observable through this surface). ERR_FVE_LOCKED when both forms fail; ERR_UNSUPPORTED off-Windows. |

### (q) The engine-log seam (not a cr_ui_* export on core)
The core's diagnostic seam is C++: `cr::log::set_sink(SinkFn, void*)`
(include/cr/cr_log.h:59). Levels: Error 0, Warn 1, Info 2. The sink may fire from
ANY thread, writes serialized by an internal mutex, must be cheap or internally
deferred (a blocking sink blocks the emitting worker). On MSVC the DLL exports the
mangled symbol `?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z` via
src/ui/cr_ui_exports.def:19 — a C consumer can GetProcAddress it; on Linux it is
reachable from libcr_ui.so's dynsym. **The v2 engine copy's `cr_ui_set_log_sink`
flat-C wrapper does NOT exist on core v4** (see §3).

---

## 3. The v2→v4 delta (THE REWIRING SPEC)

Reference pair: `final-prod/engine/include/cr/cr_ui.h` (v2, ABI 2, **83 `CR_UI_API`
token lines / 80 actual exports**) vs `core_recovery/include/cr/cr_ui.h` (v4, ABI 4,
**95 token lines / 92 exports**). Programmatic full-signature diff (every
`CR_UI_API … cr_ui_name(…);` compared textually after whitespace normalization):

### 3.1 Headline numbers
- **Added: 14 exports. Removed: 2. Signature-changed: 0.** Net 80 → 92.
- Version macro: `CR_UI_ABI_VERSION` 2 → 4. New macro `CR_UI_SESSION_ROW_PATH 1024`.
- New event types: `CR_EVT_SESSION_LOAD_PROGRESS 11` (W1b), `CR_EVT_AUTOSAVED 12`
  (W1c). v2 had 1..10; v4 has 1..12.
- Error space: v2 engine copy has 35 ERR_ members; v4 has **40** (+OK). Details §3.4.

### 3.2 Exports ADDED in v3/v4 (new capabilities the UI may surface)
All 14, signatures verbatim (grouped by capability):

1. `uint32_t cr_ui_enumerate_disks_ex(cr_ui_disk_info* out, uint32_t* count, uint32_t flags)` — M-076 TRIM chip (opt-in `CR_ENUM_FLAG_TRIM 0x1`; unknown bits refused).
2. `uint32_t cr_ui_session_list(const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count)` — W1a (R-56) saved-session directory listing (newest-first, light records only).
3. `uint32_t cr_ui_session_resume_last(const char* utf8_dir, cr_ui_session** out)` — M-154 pointed resume (sidecar pointer, no enumeration).
4. `uint32_t cr_ui_session_last_autosave_path(const char* utf8_dir, char* out_path, uint32_t cap)` — the pointer query (ERR_RANGE_OUT_OF_BOUNDS when dead).
5. `uint32_t cr_ui_session_load_with_events(const char* utf8_path, cr_ui_event_fn fn, void* user, volatile uint32_t* cancel_flag, cr_ui_session** out)` — W1b load progress + caller-owned cancel.
6. `uint32_t cr_ui_archive_entry_count(cr_ui_preview* pv, uint32_t* out)` — M-116 browse-inside.
7. `uint32_t cr_ui_archive_entry_at(cr_ui_preview* pv, uint32_t index, cr_ui_archive_entry* out)` — M-116.
8. `uint32_t cr_ui_thumbnail_generate(cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id, const cr_ui_thumbnail_caps* caps, uint8_t* rgba, uint32_t rgba_cap, cr_ui_thumbnail_info* info)` — M-102 (+ M-083 grade promotion).
9. `uint32_t cr_ui_thumbnail_format_supported(const char* utf8_format)` — M-102.
10. `uint32_t cr_ui_scan_carve_tally_count(cr_ui_scan* s)` — N-140 per-format carve tally.
11. `uint32_t cr_ui_scan_carve_tally(cr_ui_scan* s, uint32_t index, uint64_t* out)` — N-140.
12. `uint32_t cr_ui_image_create(uint32_t disk, const char* utf8_path, cr_ui_recovery_progress_fn progress, void* user)` — S-040 the "Create Image" producer.
13. `uint32_t cr_ui_fsck_volume(char letter, uint32_t flags, int32_t* exit_code)` — M-075 chkdsk escalation.
14. `uint32_t cr_ui_turn_off_bitlocker(char letter, cr_ui_progress_fn progress, void* user)` — M-078 BitLocker off.

### 3.3 Exports REMOVED (the bridge must stop binding them)
Both removals are **final-prod-local engine patches that never landed in core**
(verified: `git log --all -S` finds neither symbol in core's cr_ui.h history; the
engine copy = core@22e9657 + the NG-15/G-9/NG-9 patch trio):

1. `void cr_ui_set_log_sink(cr_ui_log_fn fn, void* user)` (NG-15) — **no flat-C log
   sink on core v4.** Replacement: bind the C++ seam `cr::log::set_sink` (exported
   from cr_ui.dll as the mangled symbol `?set_sink@log@cr@@...`, def file :19), or
   run silent. Callback shape: `void (*)(uint32_t level /*0 Err,1 Warn,2 Info*/,
   const char* line, void* user)`, any thread, must not block.
2. `uint32_t cr_ui_health_read_ex2(uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr_ex* attrs, uint32_t attr_cap, uint32_t* attr_count)` (NG-9, with
   `cr_ui_smart_attr_ex { cr_ui_smart_attr base; uint32_t threshold; }`) — **the
   per-attribute vendor-threshold capability does not exist on core v4**; only plain
   `cr_ui_health_read` / `cr_ui_smart_attr` (no threshold field). A UI feature built
   on the "Current + Threshold" health view loses its data source unless re-landed
   upstream.

Note on the core-side v4 removal: `cr_ui_driver_status` + the ERR_DRIVER_* family
(0x500B0001-05) were added to core AFTER 22e9657 (driver era) and REMOVED again at
session-96 — **neither ever appears in the v2 engine copy nor in v4**, so the v2→v4
net delta does not include them (RELEASE-NOTES.md:96-108 documents the v3→v4 bump).

### 3.4 Struct deltas (the layout truth — recompile is MANDATORY)
No shared export changed its signature, but **9 shared structs changed layout** (8
append-only, 1 destructive). A bridge compiled against the v2 engine header but
loaded against the v4 DLL is ABI-unsafe:

| Struct | v2 (engine copy) → v4 (core) | Nature |
|---|---|---|
| `cr_ui_disk_info` | v2 tail field `uint32_t bus_type` (G-9: raw STORAGE_BUS_TYPE, 0 unknown/7 USB/11 SATA/12 SD/13 MMC/14 virtual/17 NVMe) → **REPLACED** by `uint32_t trim_known, trim_on` (M-076). Same offset, +4 bytes total size. | **DESTRUCTIVE**: a v2-compiled reader of `bus_type` reads `trim_known` under v4; passing v2-sized arrays to `cr_ui_enumerate_disks` mis-strides every row after the first. Core v4 has NO bus-type field (only `removable`). |
| `cr_ui_user_sign` | `mime[64]→[81]`, `bytes[16]→[48]`, `sign_len` 1..16→1..48, `offset` bounded 0..1024. | **v3/M-067 FIELD RESIZE** — the bump-worthy change (a 73-char MIME memcpy'd into mime[64] left it non-NUL-terminated). |
| `cr_ui_scan_config` | + `const char* autosave_dir_utf8` (W1c), + `const char* const* filter_folders_utf8`, + `uint32_t filter_folder_count` (M-073). | append-only (v2-compiled struct is SHORTER — core reads past it). |
| `cr_ui_scan_stats` | + `uint64_t bytes_unreadable`, + `uint32_t probe_unreadable` (read-truth), + `uint32_t autosaved` (W1c). | append-only. |
| `cr_ui_event` | + `uint64_t autosave_rc` at the tail (W1c). | append-only — **POD stride change; a v2-compiled event sink reads garbage tail / the bridge's CrUiEvent must be re-derived.** |
| `cr_ui_query` | + `const char* path_substring`, + `uint32_t state_mask` (M-072), + `uint64_t version_group`, + `uint32_t tags_only` (M-101). | append-only. |
| `cr_ui_query_row` | + `uint64_t version_group`, + `uint32_t tags_extracted` (M-101). | append-only — row-array stride change. |
| `cr_ui_session_info` | + `char label_full[100]` (N-090; label[32] stays the 31-char prefix view). | append-only. |
| `cr_ui_apply_candidate` | + `uint32_t disk_layout`, + `uint32_t disk_flags` (M-137). | append-only. |

NEW structs in v4 (no v2 counterpart): `cr_ui_session_row` (+`CR_UI_SESSION_ROW_PATH`
1024), `cr_ui_archive_entry`, `cr_ui_thumbnail_caps`, `cr_ui_thumbnail_info`.
UNCHANGED (field-identical, verified by diff): `cr_ui_geometry`, `cr_ui_volume_info`,
`cr_ui_recovery_config`, `cr_ui_recovery_stats`, `cr_ui_telemetry`, `cr_ui_stage_tel`,
`cr_ui_carve_format`, `cr_ui_metadata`, `cr_ui_smart_attr`, `cr_ui_smart_report`,
`cr_ui_repair_plan`, `cr_ui_apply_plan`, `cr_ui_apply_result`, `cr_ui_assistant_stats`,
`cr_ui_disk_map_geometry`. (The 9th changed struct is `cr_ui_node_info`: M-101 tail
append `u64 version_group; u32 tags_extracted` — append-only, same class as the
tabulated rows; the table lists the other eight.)

### 3.5 Error-code deltas (v2 engine copy → v4)
| Change | Code | Notes |
|---|---|---|
| RENAMED/REPURPOSED | `0x50020007` | v2: `ERR_DRIVER_REQUIRED` (mandatory-driver era) → v4: **`ERR_DEVICE_DISCONNECTED`** (N-125: yanked USB/ejected media mid-scan; drives the reattach-then-resume flow; CLI exit 5). **Same numeric value, changed meaning — the bridge's mapping table must be updated.** |
| ADDED | `0x50050006` | `ERR_DEST_PATH_TOO_LONG` (M-074). |
| ADDED | `0x50050007` | `ERR_DEST_PATH_INVALID` (M-074). |
| ADDED | `0x50050008` | `ERR_DEST_NETWORK` (M-074). |
| ADDED | `0x50060002` | `ERR_FVE_ELEVATION_REQUIRED` (M-105, the ACL diagnosis). |
| ADDED | `0x500C0001` | **`ERR_ELEVATION_REQUIRED`** (session-96; FRESH value slot — never a re-used 0x500B one). |

(The driver-era 0x500B0001-05 family came and went entirely between the two
snapshots — absent from both.) `CR_ERRMOD_*`/`CR_ERROP_*` constants: identical in
both headers (13 modules + 2 ops, append-only, pinned by static_asserts in cr_ui.cpp).

### 3.6 Narrative (verified against RELEASE-NOTES.md)
- v2 = M-036 (six appended `cr_ui_smart_report` NVMe fields) — already in the engine
  copy at 22e9657.
- v3 = M-067: the `cr_ui_user_sign` field resize (the first struct-FIELD change).
- v4 = session-96 / v0.7.0: the direct-access release — kernel driver removed,
  `cr_ui_driver_status` + ERR_DRIVER_* removed, ERR_ELEVATION_REQUIRED added,
  transport strings now `"direct"`/`"image"` (v2's engine copy reports `"rda"` for
  live disks — the bridge must expect the string change). v0.7.1 kept ABI v4 and
  fixed two defects (the F12 false cross-disk ERR_DEST_OVERLAP storm — 10,248/10,405
  items refused on the bench runner; and the `--deep` N-044 comment-swallow).

---

## 4. Data structures & enums (cr_types.h and friends)

- **Fixed-width scalars**: `cr::u8..u64/i8..i64` (cr_types.h:20-27). LE byte folds
  `u16/u32/u64_from_bytes_le` (:29-41); BE readers in cr_bits.h (le16/32/64, be16/32/64).
- **Time model**: NTFS-style 100ns since 1601 everywhere (`FileTimes`
  {created,modified,changed,accessed}; `unix_to_filetime[_ns]` folds, :45-66).
- **`cr::FsKindCode`** (cr_types.h:81-101, session/fs_kind wire values): Fat12=0,
  Fat16=1, Fat32=2, Ntfs=3, **Ext2=4** (OQ-S52-1/M-133: the EXT2/3 family),
  Raw=6 (carved), Exfat=7, Hfsplus=8, Ext4=9, Refs=10, Btrfs=11, Zfs=13. (5 skipped
  on the wire; legacy remap on load: 12→13, Btrfs-7 indistinguishable from exFAT-7 =
  documented F16 defect.)
- **`orchestrator::FsFamily`** (cr_scan.h:37+): Unknown=0, Ntfs=1, Fat=2, Exfat=3,
  Ext4=4, (5 reserved — value NOT renumbered, FB-L03), Hfsplus=6, Btrfs=7, Zfs=8,
  Refs=9, BitLocker=10 (probe-only), UDF=11 (M-044). This is `fs_hint` and
  `session_info.fs_family`'s value space.
- **Stage/origin/deleted/confidence/grade enums** (consumed as plain u32 at the ABI,
  values in cr_ui.h comments): stage 0 quick, 1 deep, 2 carve, 0xFFFFFFFF none;
  origin 0 structured, 1 carved, 2 lost, 3 virtual; deleted 0 active, 1 deleted,
  2 partial, 3 damaged; confidence ordinal 0..4; grade 0 low, 1 average, 2 high
  (sticky-2). `tree::Grade` re-grade: only grade-1 nodes eligible.
- **Health grading** (cr_grade.h:52-57): `HealthGrade` Unknown=0, Good=1, Caution=2,
  Bad=3 — the og DiskHealth 4-status model; pure-core rules (per-disk caution counts
  for attrs 5/197/198, SSD-life 10%, NVMe: critical_warning!=0→Bad, life>10→Good,
  ==10→Caution, <10→Bad; temperature is DISPLAY-ONLY).
- **Device descriptors / scan params / result rows / recovery / health / repair /
  session / preview payloads**: all specified field-by-field in §2 (they ARE the ABI
  structs — the ABI deliberately has no duplicate C++-side UI shapes).
- **String encoding**: every string at the ABI is NUL-terminated UTF-8 in, and UTF-8
  out (fixed-capacity fields are NUL-padded; truncation is reported, never silent).
  Internally names are UTF-16; the codec policy (cr_text.h:1-25): strict RFC 3629
  decode, ONE U+FFFD per ill-formed maximal subpart (Windows MB2WC semantics); the
  LOSSLESS variant maps raw bytes to U+E000+byte (ext4/Btrfs round-trip; ZFS
  deliberately keeps the replacement form); `wide_to_utf8` for host args. DO NOT
  "clean" private-use units downstream.
- **Versioned-struct patterns**: no numeric size/version fields in ABI structs —
  versioning is the append-only policy + `CR_UI_ABI_VERSION`; the .crsf container and
  its records carry their own explicit lengths/versions instead (§6).

## 5. Error model (cr_error.h + cr_errmod.h)

Single u32 numeric space, shared verbatim by the ABI (cr_ui.h:26-27). Success = 0.
The complete registry (cr_error.h:25-121 — 40 ERR_ members + OK):

| Value | Symbol | Semantics |
|---|---|---|
| 0x50020001 | ERR_OPEN_FAILED | open/transport failure |
| 0x50020002 | ERR_READ_FAILED | read failure (also the read-truth refusal: a scan that could read essentially nothing REFUSES instead of fake-completing) |
| 0x50020003 | ERR_WRITE_FAILED | |
| 0x50020004 | ERR_RANGE_OUT_OF_BOUNDS | also the dead-pointer answer of `cr_ui_session_last_autosave_path` |
| 0x50020005 | ERR_CANCELLED | cooperative cancel completed; tree/queue state preserved |
| 0x50020006 | ERR_RETRY_EXHAUSTED | |
| 0x50020007 | ERR_DEVICE_DISCONNECTED | N-125 target gone mid-scan → reattach-then-resume (CLI exit 5) |
| 0x50030001 | ERR_BAD_MAGIC | volume/boot/signature mismatch |
| 0x50030002 | ERR_BAD_CHECKSUM | identity mismatch on resume (cr_ui_scan_verify_identity); torn .crsf record |
| 0x50030003 | ERR_BAD_PARAMETER | invalid argument |
| 0x50030004 | ERR_TRUNCATED | two-call buffer too small / torn read / thumbnail decode refusal |
| 0x50030005 | ERR_FIXUP_FAILED | |
| 0x50030006 | ERR_CYCLE_LIMIT | |
| 0x50030007 | ERR_DEPTH_LIMIT | |
| 0x50030008 | ERR_UNSUPPORTED | surface exists only on the Windows platform layer (or named phase-2 refusals) |
| 0x50030009 | ERR_NO_MEMORY | allocation failure; thumbnail size-probe mismatch |
| 0x50040001 | ERR_SESSION_FORMAT | torn/refused session record (F10 salvage first) |
| 0x50040002 | ERR_SESSION_IO | |
| 0x50040003 | ERR_SESSION_VERSION | |
| 0x50050001 | ERR_DEST_OVERLAP | destination overlaps the source (F12; v0.7.1: armed only for true same-disk) |
| 0x50050002 | ERR_DEST_UNWRITABLE | |
| 0x50050003 | ERR_HASH_MISMATCH | committed but stored hash mismatched |
| 0x50050004 | ERR_DEST_FULL | N-015 ENOSPC; writer fast-fails the remaining queue |
| 0x50050005 | ERR_DEST_NAME_EXHAUSTED | N-065 collision rename space exhausted (og cap-10000; file FAILED, never a synthesized name) |
| 0x50050006 | ERR_DEST_PATH_TOO_LONG | M-074 |
| 0x50050007 | ERR_DEST_PATH_INVALID | M-074 |
| 0x50050008 | ERR_DEST_NETWORK | M-074 |
| 0x50060001 | ERR_FVE_LOCKED | BitLocker locked (legitimate unlock failed/absent) |
| 0x50060002 | ERR_FVE_ELEVATION_REQUIRED | M-105: FVE store exists but token cannot read it |
| 0x50070001 | ERR_HEALTH_UNAVAILABLE | |
| 0x50080001 | ERR_REPAIR_REFUSED | |
| 0x50080002 | ERR_REPAIR_UNDO_FAILED | |
| 0x50080003 | ERR_VOLUME_LOCK_FAILED | S-010 volume busy, quiesce refused |
| 0x50080004 | ERR_APPLY_CONSENT_DENIED | A1 |
| 0x50080005 | ERR_APPLY_STALE | A2: sector 0 changed since planning |
| 0x50080006 | ERR_APPLY_REFUSED | A3: overlap/scope/capacity |
| 0x50080007 | ERR_APPLY_REFRESH_FAILED | A4: written+verified; the 3x refresh tail failed (undo valid) |
| 0x500A0001 | ERR_IMAGE_CORRUPT | structural nonsense inside a container |
| 0x50090001 | ERR_INTERNAL | internal invariant failure (bug — report it); the catch-all |
| 0x500C0001 | ERR_ELEVATION_REQUIRED | raw-disk privilege gate (CLI exit 4) |

- **Error context**: `cr_ui_last_error()` thread-local text (stable until the next
  cr_ui_* call on that thread). Read-failure ranges additionally carry the M-049
  module/operation/attempts triple on `CR_EVT_ERROR_RANGE` (err_module =
  CR_ERRMOD_* of the sub-pass that was reading; kNone for pre-M-049 session reloads).
- **Partial success**: the recovery report is per-item truth (outcome
  done/refused/error/hash_mismatch/resumed/pending/salvaged/nothing_to_recover/
  read_error — every item reports its state, valid at any time). A cancelled
  scan/recovery keeps completed work; scan stats carry bytes_unreadable +
  probe_unreadable so "these regions were NOT scanned" is presentable without logs.
- **CLI exit-code mapping** (src/app/rrecov.cpp): 0 success; 1 operational/usage
  failure (unknown option, enumeration failure FB-K01, scan engine failure); 2
  target-level failure (cannot open disk, session load failed, image failed; recover
  errors-or-mismatches per report truth rule 4 — the report is still emitted first);
  3 cancelled (Ctrl-C is cooperative: token polls at loop heads, session saves); 4
  elevation required; 5 device disconnected (N-125).
- **The UI MUST surface distinctly**: ERR_ELEVATION_REQUIRED (trigger UAC + retry),
  ERR_DEVICE_DISCONNECTED (reattach-then-resume offer), ERR_FVE_LOCKED vs
  ERR_FVE_ELEVATION_REQUIRED (different user actions), ERR_CANCELLED (keep results),
  ERR_DEST_OVERLAP + the M-074 destination taxonomy, ERR_APPLY_CONSENT_DENIED,
  ERR_BAD_CHECKSUM (resume identity mismatch), ERR_READ_FAILED with
  bytes_unreadable>0 (the read-truth refusal).

## 6. Output & persistence formats

- **Recovery report JSON** (docs/recovery-report-schema.md, G-09 v1): emitted by
  `cr_ui_recovery_report_json` / `rrecov recover --report`. Top level
  `{schema_version:1, engine_version, source, destination, elapsed_ms, totals{...},
  items[]{item_id, path, name, kind, outcome, reason, bytes, duration_ms, sha256,
  hash_status, tree_class?, cause_id?, first_failed_offset?, bytes_written?}}`,
  optional `scan_analysis` block (S-023 fields + per-format carve_formats tally,
  N-140). Write-only for the core; deterministic field order; strict JSON escaping
  (invalid UTF-8 → U+FFFD so it always parses). Truth rules: hash reported only where
  computed; unreached items are `pending`; reconciliation
  queued == done+dirs+refused+errors+resumed+nothing_to_recover+read_error+pending.
  CI-validated by tools/validate_report.py.
- **`.crsf` session container** (docs/crsf-format.md, v1): 64-byte header
  (magic "CRSFMT01", version u32 = 1, flags bit0 ABS_EXTENTS, record_count,
  unit_size 512, session_id, reserved); 24-byte record headers (magic "CRSR",
  type u32, session_id, chain, crc32 of DECOMPRESSED payload); chunked-zlib payload
  (≤4096 B/chunk, deflate level 5 when it strictly shrinks). Record types:
  1 SESSION_STATE, 2 VOLUME_INFO, 3 PARTITION_CONTEXT, 4 RESULT_TREE, 5 ERROR_MAP,
  6 ASSISTANT_STATS, 7 ALLOC_MAP, 8 DISK_LAYOUT (M-042), 9 FS_CHECK (M-041).
  Reader behavior: inflate failure → raw fallback for that chunk; CRC mismatch →
  record delivered empty + BAD_CHECKSUM, later records continue; broken header →
  skip forward in 512-byte steps (resumable by construction). VOLUME_INFO is
  length-versioned v1..v6 (v4 adds firmware/wwn/interface + 512 B boot snapshot;
  v5 adds save_unix; v6 adds label_full[100]). Resume: `cr_ui_session_resume` +
  `cr_ui_scan_verify_identity` (boot hash) + DISK_LAYOUT comparison ("layout
  changed" is a surfaced warning; stale lost candidates DROPPED with counts).
- **Undo containers**: pre-scan repair undo (`cr_ui_scan_repair_save_undo`) and
  lost-partition apply undo (`cr_ui_apply_save_undo`) both persist "the durable
  CRC-checked container" restorable by `rrecov fix --disk N --undo-apply <file>`
  (cr_ui.h:740-747, :817-820).
- **User-signature dictionary**: the og's EUUserData JSON shape verbatim —
  `{ "<ext>": { "signs": ["<offset>,<HEX>"], "mime": "<type>" }, ... }`
  (cr_ui.h:1193-1200). Caps (S-039): signs ≤48 B, ≤12 signs/entry, offsets ≤1024,
  MIME ≤80 chars. A shipped default set (`data/userdata-defaults.json`, 111 entries)
  rides the release zip beside rrecov.exe (M-065; absent file = empty registry + one
  log line, never an error). User-sign record tag space 0xFFFE0000 (distinct from
  raw-sign 0xFFFF0000+fmt).
- **Report/JSON elsewhere**: `rrecov sessions --json`, `tree --json/--csv`,
  `list --trim`, `bench --json` (CLI-side renderings of the ABI data above).

## 7. Threading, cancellation & lifecycle guarantees

(ABI-CONTRACT §3 + cr_ui.h:23-48 — header text is the operative contract.)

- `cr_ui_scan_run` / `cr_ui_recovery_process` run SYNCHRONOUSLY on the caller's
  thread. `cr_ui_scan_pause/resume/cancel`, `cr_ui_recovery_cancel`,
  `cr_ui_scan_telemetry`, `cr_ui_scan_snapshot` are safe from ANY thread while they
  execute. One handle, one controlling thread: create/run/close on a single thread;
  the token functions are the ONLY cross-thread entries. All other entry points: one
  thread at a time per handle. `cr_ui_scan_busy/stopping`: any thread, any time.
- **Callback threading**: progress callbacks (`cr_ui_progress_fn`,
  `cr_ui_recovery_progress_fn`) fire on the worker thread running run()/process()
  only; must not block; carry NO cancellation semantics (returning is the only
  contract — cancel via the token functions, mirroring the og's separate abort
  event). EVENT callbacks (`cr_ui_event_fn`) may fire from **engine worker threads
  OR the shim's heartbeat watchdog thread**; the shim does NOT serialize deliveries
  (single-flight is the sink's own discipline); must not block; unknown `type` values
  MUST be ignored (forward compat). `CR_EVT_HEARTBEAT` (N-043): 5 s watchdog fires
  from its own thread only when no SCAN_PROGRESS arrived in the window — honest
  standing telemetry, never a synthetic permille advance.
- **Cancellation semantics**: cancel makes run()/process() return ERR_CANCELLED with
  all completed work intact (tree browsable, committed items committed, report
  truthful about the rest). Save-on-cancel is the caller's explicit choice
  (`cr_ui_scan_save_session` works on a cancelled scan; the CLI's --out policy).
  The save has its OWN scoped cancel (`cr_ui_scan_save_cancel` — never touches the
  scan token); the load has the caller-owned `cancel_flag` (polled per node, even
  when fn==NULL; a torn load never half-claims state). Pause blocks engines
  cooperatively; resume releases. Ctrl-C in the CLI is the same cooperative token.
- **Session resume + identity revalidation**: `cr_ui_session_resume` → attach the
  ORIGINAL device → `cr_ui_scan_verify_identity` (first-512-bytes vs stored boot
  hash; ERR_BAD_CHECKSUM on mismatch — the session came from a different volume; 0
  on match or unverifiable legacy) → run(). The DISK_LAYOUT record comparison
  surfaces layout drift as a warning; stale candidates are dropped with counts.
- **Parallelism** (cr_parallel.h): fixed `ThreadPool` (workers = hw-1 clamped 1..8 —
  disks, not CPUs, are the bottleneck); cooperative abort via `abort()` from any
  thread; `parallel_for` returns ERR_CANCELLED when aborted mid-run; exceptions
  captured per item → ERR_INTERNAL. `CancelPoolWatcher` polls the token every 50 ms
  and aborts the pool so QUEUED work never starts after a cancel; running jobs
  finish to their own poll point (never preempted). ABI surface: `threads = 0` auto,
  `threads = 1` fully single-threaded (deterministic testing).
- **Snapshots** are immutable value copies: readers never block writers; mid-scan
  captures are partial=1 and safe to browse AND recover from. A mid-scan disk map
  overlay stays absent (a live map would be racy — documented, not guessed).
- **Reentrancy**: no handle is internally reference-counted across threads;
  `cr_ui_register_user_signs` and the carve-format registry are process-global and
  thread-safe/immutable. `cr_ui_last_error` is thread-local.

## 8. Build, CLI, tests, bench

- **CMake targets** (CMakeLists.txt + src/*/CMakeLists.txt): `cr_core` (pure C++
  core, static), `cr_win` (Windows platform layer, static), `cr_ui` (the C-ABI shim
  → **`cr_ui.dll` + `cr_ui.lib`** import lib), `rrecov` (the CLI, Windows-only,
  `CR_BUILD_APP`), the GTest suite `cr_tests`, the plain-C ABI consumer
  (`tests/ui_abi_check.c`). Options: `CR_ENABLE_ASAN`, `CR_BUILD_TOOLS`,
  `CR_BUILD_APP`, `CR_BUILD_UI`, `CR_ENABLE_TESTS`. C++20 / C11; MSVC
  `/W4 /WX /permissive-`; gcc/clang `-Wall -Wextra -Wpedantic -Werror -Wshadow`.
  **Win7 SP1 posture**: `_WIN32_WINNT=0x0601`, PE OS version stamped
  (tools/stamp_osversion_um.py) + import-allowlist-gated (tools/verify_pe_um.py) in CI.
- **vcpkg.json**: zlib, gtest, lz4, zstd — zlib/lz4/zstd are REQUIRED in every build
  (a build without the decompressors silently zero-fills compressed pools; the
  CMake configure fails loudly instead, CMakeLists.txt:138-147).
- **Platform lanes** (the two-lane truth): MSVC (windows-2022, production) and
  Linux gcc/clang (the pure core + tests; **the win layer is parse-gated, not
  compiled** — `tools/win_syntax_check.sh` parse-covers every `tests/unit/test_*_win.cpp`
  and the win layer with a win-stub SDK; `-Werror=comment` there is what kills the
  N-044 class forever). Sanitizers ASan/UBSan/TSan green, zero suppressions (local
  via `./ci-local.sh --san` / `--fuzz` after the owner's windows-only CI directive).
- **CLI** (`rrecov`, src/app/rrecov.cpp): **13 verbs** — list [--trim],
  list-carve-formats, sessions [--dir D] [--json], scan (--disk N | --volume C: |
  --image file; --deep/--quick-only/--no-carve/--whole-disk/--threads/--out/--resume/
  --log/--autosave-dir/--label/--json/--csv/--filter/--deleted-only/--pause-stdin/
  --filter-folder/--repair[--apply][--undo][--check]/--bitlocker-fvek/
  --bitlocker-dict/--user-signs), tree --in, archive-list --in (--disk|--image)
  --node ID [--json], recover --in (--disk|--image) --to DIR [--report], health
  --disk, image --create --disk N --out, fix (--disk [--part] [--apply] [--undo]
  [--undo-apply] | --chkdsk C:), apply --disk --session --candidate [--apply]
  [--undo] [--boot-fix], unlock --turn-off C:, bench --disk. Exit codes: §5.
  The `driver` verb was removed with the driver.
- **Tests**: `tests/unit/` — 76 files, 1,717 TEST/TEST_F macros (docs/testing.md),
  latest executed green counts 1740/1740 MSVC / 1737/1737 Linux (RELEASE-NOTES
  v0.7.0:106-108). ctest via presets (`linux-release`, `win-x64-release`).
  `tests/ui_abi_check.c` is the executable form of the ABI contract: plain C, no
  C++ runtime, drives list→scan→save→tree/snapshot→query→preview→recover→report on a
  synthetic FAT16 image. Coverage per subsystem in docs/testing.md (all engines
  end-to-end on synthetic volumes, LDM with real-parity reconstruction, CRSF
  round-trip + corruption, writer overlap/collisions/hashing/resume, etc.).
- **Fuzz**: `fuzz/` — 12 libFuzzer harnesses (fuzz_pure, fuzz_engines, fuzz_carve,
  fuzz_preview, fuzz_container, fuzz_image, fuzz_thumbnail + per-FS harnesses),
  `-Werror`, seeded corpus committed under `fuzz/corpus/{fat,ntfs,exfat,ext4,btrfs,
  zfs,refs,hfsplus,image,container,pure}`. Local: `make run` (45 s smoke) /
  `make run-long`.
- **Bench**: `bench/` — evidence-grade ours-vs-og comparison on staged VHDX disks
  (seed manifest = ground truth; 12-scenario deletion matrix: recycle_bin,
  permanent_delete, quick/full_format, delete_partition, decay_50/80/100, …).
  `bench/baseline.json` (schema 2) = the perf regression gate: **10 driver-parity
  rows with tolerances** (bench-seq-1m ≥536.92 MiB/s ±3%, bench-rand-1m ≤1993.22 µs
  ±10%, bench-open, scan-quick/deep/deepcarve walls, api-open/scan-run/save-session/
  recovery) — failing on violation or on a metric that stops being captured.
  v0.7.1 posture (re-measured, direct vs driver baseline): seq 1 MiB 554.83 MiB/s
  (+3.3%), rand 1 MiB 1943.47 µs (−2.5%), open −41%, deepcarve −31%, API scan −22%,
  CPU/GiB −39% (RELEASE-NOTES.md:57-62; full ledger worklog/PERFORMANCE.md §2).
- **CI lanes** (.github/workflows, docs/CI-CURRENT.md — all windows-2022): **Checks**
  (push/PR: format, lints, /W4 /WX build, full ctest incl. the C ABI consumer, Win7
  posture, G-09 report-schema validation, the unelevated-refusal exit-4 probes, CLI
  junk-gate), **Release** (tag-driven: validate→build core→assemble+publish; core
  zip + MANIFEST.json + SHA256SUMS only), **Test & Benchmark** (dispatched per
  publish: prebuilt-release consumer, VHDX scenario matrix through CLI AND API, the
  perf regression gate), **Benchmark (source)** (session-97: measures the checked-out
  commit through the release recipe), **Installer gates** (S-038 silent round-trip +
  version key + OS-gate matrix). [DRIFT] CI-CURRENT's title says "4-lane" but the
  table lists five (benchmark-src postdates the title).
- **Engine kit staging** (how consumers get binaries): the release zip
  `core-recovery-<tag>.zip` is assembled by release.yml:190-247 —
  **`bin/`**: rrecov.exe, cr_ui.dll, userdata-defaults.json, VC143 CRT + UCRT DLLs
  (+ pdbs); **`lib/`**: cr_ui.lib (import), cr_core.lib, cr_win.lib (static);
  **`include/`** (the full header tree), **`docs/`**, **`data/`**, README/LICENSE/
  RELEASE-NOTES, in-zip SHA256SUMS. Verified by `.github/scripts/fetch-release.ps1`
  (SHA256SUMS + MANIFEST cross-check + the exit-4 refusal probe doubles as the ABI
  regression pin). The final-prod side then rebuilds its own kit in build.ps1:98-118
  (cr_ui.dll + cr_ui.lib + the vcpkg runtime DLLs the POST_BUILD deploy step placed
  beside cr_ui.dll) and stages it so the Tauri bridge links (cr_ui.lib) and loads
  (cr_ui.dll + runtime DLLs beside the exe).
- **Installer** (installer/): Inno Setup (6.3+), core-only payload since session-96 —
  no driver component, no SCM step; teardown gate asserts NO kernel service is
  installed. 13-parameter contract from the og corpus (silent flags, ARP
  UninstallString + QuietUninstallString ending " /SILENT", admin-required,
  version key `HKLM\SOFTWARE\heicfast\core_recovery\Version` write+verified,
  OS gate Win7-SP1→Server-2025 x64, single frozen AppId, telemetry omitted).
  [DRIFT] installer/README.md still narrates the driver deployer and cites the
  deleted docs/DRIVER-RELEASE.md — the .iss itself is updated (git f6a605d9).

## 9. Consumer integration checklist (ordered)

1. **ABI gate first**: load `cr_ui.dll`; `cr_ui_abi_version() == 4` (compile against
   the v4 header) — refuse to bind on mismatch (CONNECTION-GUIDE §2). Recompile the
   bridge's `#[repr(C)]` structs from the v4 header — v2 layouts are unsafe
   (§3.4: disk_info tail, user_sign widths, event/query_row/scan_config/appends).
2. **Optional log sink**: bind the exported mangled `cr::log::set_sink` if engine
   logs are wanted (any thread, cheap, level 0/1/2). Default is silent.
3. **Device census**: `cr_ui_enumerate_disks` (always-fresh; two-call). Opt into
   `cr_ui_enumerate_disks_ex(..., CR_ENUM_FLAG_TRIM)` only if the TRIM chip is worth
   one fsutil spawn. `cr_ui_enumerate_volumes` for letters/FVE.
4. **Elevation handling**: expect `0x500C0001` from open_disk/open_volume/
   health/image_create/fsck(fix posture) when non-elevated. Treat it as "trigger
   UAC, retry" — never as a fatal error. Check `cr_ui_last_transport_error()` for
   the advisory twin. Image targets and session/preview/dictionary verbs need no
   elevation — keep those paths usable unelevated.
5. **Open the device**: `cr_ui_open_disk` (whole disk, whole_disk scans) /
   `cr_ui_open_volume` (slice; BitLocker probed first) / `cr_ui_open_image`
   (portable). `cr_ui_device_geometry` / `cr_ui_device_transport` ("direct"|"image").
6. **Scan wiring**: zero-init `cr_ui_scan_config` (memset 0 = all defaults — never
   "nothing matches"); set stage_mask/whole_disk/fs_hint/threads as needed;
   `cr_ui_scan_set_event_fn` BEFORE run (one sink; mid-run subscribe is
   ERR_BAD_PARAMETER); `cr_ui_scan_attach_device`; run `cr_ui_scan_run` on a dedicated
   worker thread; drive UI from telemetry (`cr_ui_scan_telemetry`, any thread) +
   events; pause/resume/cancel from the UI thread; `busy`/`stopping` for live state.
7. **Results**: after run (or mid-run) take `cr_ui_scan_snapshot` (immutable;
   partial=1 mid-scan) and enumerate `snapshot_node`/`snapshot_dfs`; or
   `cr_ui_scan_save_session` → `cr_ui_session_load[_with_events]` →
   `cr_ui_session_snapshot` + `cr_ui_query_run` (filter/sort/page, count_only for
   totals, `cr_ui_query_apply_buckets` for the og's date/size menus — unknown IDs
   throw). Pagination: offset+limit, default limit 100, `total_matched` out.
8. **Preview/thumbnails**: preview borrows the DEVICE (must outlive);
   `preview_read` ≤4 MiB/call, short only at EOF, unreadable spans zero-filled AND
   reported via `preview_unreadable`; `archive_entry_count/at` for browse-inside;
   `thumbnail_generate` two-call (NULL probe → bytes_needed → alloc → call; decode
   refusal = ERR_TRUNCATED with info->truncated; success promotes grade to 2).
9. **Recovery**: `recovery_create` (zero-init cfg = 1 MiB chunks, 64 renames, verify
   ON, reconstruct ON, conflict=rename) → `set_destination` (F12 overlap arms only
   same-disk) → enqueue node/subtree/…_snapshot → `process` on a worker thread →
   `get_stats` + `report_json` (valid any time; pending items are truthful).
10. **Teardown**: close in reverse: preview → snapshot (after last row read) →
    recovery → scan → session → device. The device outlives everything borrowing it.
11. **Consent discipline**: repair_consent / apply_consent callbacks are
    default-deny (NULL/zero = nothing written); the dictionary unlock and
    turn_off_bitlocker are explicit user actions only.
12. **Top pitfalls** (from the header/contract text):
    - Passing v2-compiled structs (disk_info arrays, event POD, query rows) against
      the v4 DLL — silent misreads (§3.4).
    - Subscribing events after run() started (ERR_BAD_PARAMETER) or expecting
      callback-return to cancel (it never does — token functions only).
    - Blocking in progress/event/log callbacks (they fire on engine workers; a
      blocking sink blocks the worker — marshal out, don't work inside).
    - Treating `0x50020007` as driver-required (v2 meaning) — it is
      ERR_DEVICE_DISCONNECTED now; offer reattach-then-resume.
    - Assuming enumeration caches (it never does — every call re-queries; refresh by
      calling again) or that an empty list means failure (enumeration failure returns
      its error code; an empty-but-successful list is OK).
    - Forgetting `cr_ui_string_free` / `cr_ui_query_rows_free` / per-overlay
      `cr_ui_disk_map_free`, or closing a snapshot's source while rows are still
      being read (values are copied — close after the LAST row READ).
    - Expecting `cr_ui_set_log_sink` / `cr_ui_health_read_ex2` / `bus_type` (the
      engine-copy locals) on core v4 — they do not exist (§3.3); health thresholds
      and the C log sink need re-landing upstream if still required.
    - Version-stamp trust: the DLL's project version still reads 0.6.1 at this tree
      despite v0.7.1 notes ([DRIFT] §1) — gate on `cr_ui_abi_version()`, not the
      file version.

— End of sheet. Provenance: core_recovery@12f3edf7 (read-only); v2 reference
final-prod/engine (core@22e9657 + NG-9/NG-15/G-9 local patches, blob-verified).
