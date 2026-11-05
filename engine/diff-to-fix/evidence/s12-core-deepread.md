# S12/B — CORE DEEP-READ parity-disposition report (audit sub-agent B)

**Scope:** the under-walked core modules named in the S12/B mandate.
**Method:** full source read of every assigned file; per-file grep of
`diff-to-fix/modules/*.md`, `diff-to-fix/findings/`, `FIXED-LEDGER.md`,
`CROSS-REF-MATRIX.md`, `SUMMARY.md`, `API-DELTA.md`; verification (ls/grep) that every
in-code corpus citation resolves to a real artifact in
`easeus-drw-research/re-workspace/`; git-history checks where a call-site truth question
arose. No files inside `core_recovery/` were modified.

**Trail disposition vocabulary:** COVERED = a module doc and/or finding records a verdict
for the file's behaviors. PARTIAL = only sub-behaviors are dispositioned. NOT-COVERED =
no trail row names the file/behavior.

---

## 1. `src/core/recovery/` — 4 files, 1,243 LOC

### cr_dict.cpp — 88 LOC, 2 public fns (`dict::parse`, `dict::try_passwords`)
- **Trail:** **COVERED** — N-064 (FIXED-LEDGER.md:244, fix `97382b7`,
  `findings/FIXED/N-064-dict-password-dictionary-absent.md` deleted per purge
  convention); API-DELTA.md rows 25 + 43 (`cr_ui_unlock_volume_with_dictionary`,
  "Dict 228-entry password dictionary | shipped (cr_dict_data.cpp) | corpus-api-context
  §4" — MATCH+). The D-80 dead-pwddict adjudication (SUMMARY.md:1331-1334, `28e9510`)
  and the N-064-union repair wave (SUMMARY.md:1759-1770) are the standing record.
- **Corpus ground truth:** `re-workspace/artifacts/config-files/config-data-reference.md`
  — "Dict (2,742 B, 228 entries — byte-verified) … comma-terminated password lines …
  numeric-first then common word passwords" (verified present, §Dict). Consumer rows:
  `DRWService-exe--bitlocker-service.md` (pipe caps), `bitlockerlib--bitlocker-implementation.md`.
- **Core-first (informational):** `parse()` accepts both comma-terminated and
  unterminated lines ("keeps the grammar total for hand-edited dictionaries",
  cr_dict.cpp:34-36) and rejects non-printable-ASCII/oversized/hostile blobs — superset
  validation, no og-side decode exists to contradict.

### cr_dict_data.cpp — 214 LOC, generated (kDictPool[2289], kDictEntries[228])
- **Trail:** **COVERED** by the same N-064 row; the generated-table provenance
  (scripts/gen_dict.py, "byte-verified shape") is the finding's own landing form.
- **Corpus:** same `config-data-reference.md` Dict section (2742 B file vs 2289-B
  NUL-pool = the comma/CRLF overhead dropped at generation — consistent with the
  "byte-verified 228 entries" claim, not a divergence).

### cr_report.cpp — 264 LOC, ~8 fns (`engine_version`, `escape_json`, `append_analysis`, `build_report` + helpers)
- **Trail:** **COVERED** — writer.md D13 row ("the og has no JSON report (GUI funnel);
  the core's schema is its own contract, validated by CI (validate_report.py)");
  `docs/recovery-report-schema.md` (G-09, issue #6 — write-only for the core); the
  `scan_analysis` block is the S-023 landing (writer.md:235-237 cites the finding's fix
  plan; FIXED-LEDGER S-023 row).
- **Corpus ground truth for the scan_analysis half:**
  `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §1
  (RecoveryAssistant counters: TotalFileCount/rawCount/lostPartitionExist/Delete/
  QuickScanExist/Delete/DeepScanFileCount/QuickScanDeleteExtend + category buckets) —
  exactly the field set `append_analysis()` emits (cr_report.cpp:166-201).
- **Core-first (informational):** the whole JSON report surface is core-first; og's
  nearest artifacts are DRWReport.exe (crash reporter — different object) and the
  ExportThread "%1 files/%2 failed; … check the recovery log" message
  (architecture/04-recovery-workflow.md:150-151).
- **PARTIAL residue:** `escape_json`'s U+FFFD policy is documented in
  recovery-report-schema.md ("invalid UTF-8 bytes become U+FFFD") but no N-016a-style
  decode-parity verdict exists for the *report* escaper (N-016a covers the restore-name
  path only). No og-side counterpart to contradict → informational.

### cr_safewriter.cpp — 677 LOC, ~15 fns (SafeWriter: set_destination[_spans],
sanitize_name, resolve_collision, enqueue ×2 + enqueue_common, ensure_dir_,
recover_item[_impl_], process, report_totals, report_items)
- **Trail:** **COVERED** — writer.md is a dense per-function disposition: B2 recovery
  correctness, F62 overlap refusal (R7.2/F12, G-08 dirs), D13 report, name sanitization,
  collision (A27 → N-065 MATCH after DFX34), S32 cancellation, X49 memory; findings
  N-009b (progress honesty — fixed in-tree), N-009d (ACL-only divergence, ACTIVE),
  N-013 (FIXED — see Divergence Candidate #1 below for the call-site regression),
  N-015 (FIXED 915bce3), N-016a/b (FIXED), N-056 (FIXED DFX17), GAP-S6-7
  (nothing_to_recover, CLOSED 03ddcd2), M-074 (FIXED 09b311d), S-052 (ACTIVE — the
  LDM-basis half that lives in this file's `source_->read_at` contract).
- **Corpus:** architecture/04-recovery-workflow.md §7.3-7.4 (ExportThread,
  `\\.\X:` destination validation, per-file error collection); DRWUI_decompiled.c
  :357116-357131 (collision), :264664-694/:357529 (restore-name MB2WC),
  RecoveryModule_decompiled.c:47033-47038 (free-space pre-check),
  `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c` exists (verified).
- **Core-first (informational):** always-computed SHA-256 + hash_status truth
  (G-09/FB-R02), `salvaged` sub-bucket, `pending` truthful-abort accounting,
  compressed-extent zero-fill "never wrong bytes" policy — all registered in
  CORE-GAP-LIST GAP-P1-4 / writer.md.

**Module verdict:** recovery/ is fully dispositioned. The one live defect found is the
N-013 call-site regression (see §10, Candidate #1) — a *consumer* of this module, not
the module itself.

---

## 2. `src/core/tree/` — 4 files, 1,155 LOC — **NO dedicated module doc** (thin coverage zone)

### cr_diskmap.cpp — 225 LOC, ~9 fns (build_disk_map ×2 overloads, downsample_free_map, choose_cell_bytes/mark_extents/cell_of helpers)
- **Trail:** **COVERED** — visualization.md (S-009 SEVERE → FIXED with the free-overlay
  wiring; GAP-S6-11 CLOSED in CORE-GAP-LIST.md:135 with "RESIDUAL: none open").
  Residuals already recorded in visualization.md:66-69 (budget-growth paths untested;
  fold the diskmap fuzz into S-009's harness ask).
- **Corpus:** UIUX/08-scan-phase-deep-dive.md (AnalysisGridWidget; rendering host-side
  by design) + SearchDLL/searchdll-family--architecture.md:31 (IBuildVolumeBitmap) —
  both verified present.

### cr_grading.cpp — 350 LOC, ~11 fns (extension_in_grading_table, grading_table_entry_count, GradeContext::{slot_for,fs_kind_of,disk_ordinal_of}, solo_rules, grade_tree, regrade_node + twin-key/ext helpers)
- **Trail:** **COVERED** — orchestration.md:102-136 (S-023 FIXED verdicts P15/P38, the
  deliberate-non-adoption register for the undecoded vt+0xb8/vt+0x100 sub-rules);
  evidence/decompiled-quotes/s4-crecoverability-grading-engine.md + s5-grading-
  extension-table-dump.md both exist (verified); consumers wired (cr_scan.cpp:1227
  grade_tree, cr_ui.cpp:2947 regrade_node).
- **Corpus:** DRWUI .data VA 0x141bb0500 169×9-byte table (dumped byte-exact);
  DRWUI-subsystems--assistant-grading-preview.md §2 (CRecoverability, FUN 14038ac90,
  605660+).
- **PARTIAL residue (ACTIVE finding):** **N-132** — the AssistantStats use the plain
  8-bucket `category_counts` (cr_grading.h:115) while the og has the SumaryFileType
  extension-unification groups + PriorityDisplayType 5-cat order
  (modules/query.md:70-90; evidence/decompiled-quotes/s10-config-layer-absence-greps.md).
  "Disposition required: land the unification helper or register the plain counter as a
  deliberate delta" — still open.

### cr_node.cpp — 445 LOC, ~17 fns (FileProperties::clone, FileNode::set_props/name_utf8, ResultTree::add_root/add_file/index_node_locked/find_by_record/find_by_extent/rebase_{new,subtree,props}/refresh_stats/reparent/detach_subtree/reset, arena_alloc, extent_len_class_)
- **Trail:** **COVERED by scattered findings, no module doc** — M-047 FIXED
  (recursive stats, rsf-format :285 vtable+0x58), OQ-SA-1 FIXED (anchor datums),
  N-100 FIXED (+0x60 carrier), M-101 ACTIVE (versions/tags branches),
  N-131 ACTIVE (FS-native parent anchors), S-052 ACTIVE (LDM layout-relative basis,
  cr_node.h:400-407). CROSS-REF-MATRIX.md:520-521 carries the IFileProperties
  23-slot/session-wire field verdicts (28-of-32 MATCH etc.).
- **Corpus:** SaveScan/rsf-record4--tree-node-grammar.md + ifileproperties-part1..3
  (all verified present).

### cr_snapshot.cpp — 135 LOC, 3 public fns (capture_snapshot/capture, dfs, materialize_props)
- **Trail:** **COVERED as core-first** — GAP-G-02/G-02b CLOSED (fc656d2,
  CORE-GAP-LIST.md:66-67), ADR-UI-SNAPSHOT.md, session-5 first-time-cited read
  (SUMMARY.md:4565), visualization.md:88 acceptance tests
  (DiskMapFreeCellsSurviveSessionReloadAndSnapshot). No og counterpart exists to
  diverge from (the og's mid-scan consistency model is its threading architecture —
  DRWUI-exe--thread-architecture.md) → informational core-first.
- **N-016b raw-name survival through snapshots is pinned in-source (cr_snapshot.cpp:44,
  :124) with the DRWUI:264664-694 cite.**

**Module verdict:** tree/ has no module row of its own; every file is nevertheless
dispositioned through other modules' rows + findings. The thin spot is bookkeeping, not
coverage. Grading carries the one active behavioral residue (N-132).

---

## 3. `src/core/table/` — 3 files, 1,981 LOC

### cr_crc32.cpp — 41 LOC, 1 public fn (crc32)
- **Trail:** **COVERED implicitly** — ldm.md:9 names all three table/ files as the
  audited unit ("src/core/table/ (cr_table_parse, cr_ldm, cr_crc32 — 1,900 LOC)") and
  :33 dispositions "GPT header/entry CRC validation" as M. The algorithm itself is
  standard zlib/Linux crc32 (core-first; GPT requirement) — no og-side analog needed.

### cr_table_parse.cpp — 427 LOC, ~6 public fns (mbr_type_to_kind, parse_ebr_chain, parse_disk_table + load_gpt_side/salvage_primary_entries helpers)
- **Trail:** **COVERED** — ldm.md B1/B6/C19/C20 rows: GPT CRC validation, backup-GPT
  salvage at device end, GPT_ENTRIES_READ_CAP, two-step GPT salvage "M"; M-002 FIXED
  (torn EBR keeps walked prefix, ebr_status/ebr_break_lba); N-057 (Ebr layout
  first-class); A-001/A-002 GUID constants saga; P0-2 header-CRC-length gate.
- **Corpus:** `reversed/ghidra-mo/Partition_decompiled.c` exists (38,246 lines;
  FUN_180006e20 verified present); mo-module-map.md; s2-ldm-gpt-gate.md evidence.
- **NEW-DIVERGENCE-CANDIDATE (small, informational):** `salvage_primary_entries()`
  requires `entry_size == sizeof(GptEntry)` (128) where "the og hardcodes the 0x80
  stride, which mis-sizes the array for corrupt non-128 headers"
  (cr_table_parse.cpp:78-81). This stricter-than-og gate is documented **only
  in-source** — no PARITY-DELTAS row and no ldm.md verdict names it. Both sides citable
  (core comment vs. the og's hardcoded stride at Partition_decompiled.c:4517-4560).
  Recommend registering (1-line D-row), not changing code.

### cr_ldm.cpp — 1,513 LOC, ~59 defs (BE readers, vnum/vstr, parse_privhead/tocblock/vmdb, ldm_parse_guid_text, LdmGuid::to_string, ldm_volume_kind_name, VBLK parsers ×7, parse_vblk_view, walk_vblks, frag_commit, resolve_layouts/resolve_one_volume, find_privhead, parse_ldm_database, LdmVolumeDevice ctor/read_at/read_spanned_copy, spanned_find/translate_spanned, probe_disk_lds, xor_into)
- **Trail:** **COVERED** — ldm.md in full (B1/B2/B3/B6/B8/C19/C20/A27/A30/S32/X51/F59/
  F62/Q65 + A-001/A-002 FIXED, M-002 FIXED, M-016 FIXED, N-025 FIXED, M-046 FIXED
  (4Kn), I-008, OQ-T7 RESOLVED left-symmetric, S-052 ACTIVE recovery-side). The
  kernel/libldm decode is verified against in-repo scripts/ldm_reference.{c,h}.
- **Corpus:** `reversed/ghidra-mo/LdmManager_decompiled.c` exists (91,475 lines; the
  :59525/:59748/:68631 cites resolve); PartitionRecovery/mo-module-map.md.
- **Registered deliberate supersets (already dispositioned):** damaged-primary backup
  replacement (kernel aborts), config-region clamp, TOCBLOCK forgiving per-copy walk,
  VBLK disk_v4 floor-vs-equality (AUDIT.md row 30) — all in-source + ldm.md:127-132.

**Module verdict:** table/ is the densest-covered module in this sweep. Only the
entry_size-strictness registration (above) is new.

---

## 4. `src/core/apply/cr_apply.cpp` — 586 LOC, ~14 fns (lba_to_chs, mbr_type_for_family, plan_apply, execute_apply, undo_apply, refresh_tail, IApplyDevice::generate_disk_signature + helpers)

- **Trail:** **COVERED** — lost-partition.md:175-182 (S-029 FIXED s10: "plan_apply
  (read-only; refusals …), execute_apply (… 3x Sleep(1000) refresh retry tail), and
  undo_apply (byte-exact restore + refresh replay)") + the session-16 verdict update
  (lost-partition.md:202-215, ABI exports + CLI verb). FIXED-LEDGER.md:321.
- **Corpus:** `reversed/ghidra-partition/PartitionRecovery_decompiled.c` (70,706
  lines — **verified**: FUN_180038b10 at :32548, FUN_1800392f0 at :32784; the 3×
  {Sleep(1000); 0x70140} refresh tail at :47015-47025; the 0x7C058/0x18 record at
  :47012-47014) + `artifacts/PartitionRecovery/pending-operation-engine.md` (the EPM
  transactional engine — Controller/Operation/PendingOperation, undo semantics; the
  "Recovery tie-ins" section). All in-code PR: cites verified against the actual file.
- **Registered deliberate divergence:** D-72c — refresh-tail failure returns
  ERR_APPLY_REFRESH_FAILED where "the og swallows this silently" (cr_apply.cpp:78-90,
  :552-556) — registered (PARITY-DELTAS per the S-029 resolution).
- **Core-first (informational):** A1-A6 safety standard (double revalidate, consent
  before lock, sector-coalesced verified transaction with rollback-all) — a superset
  of the og's funnel, documented in cr_apply.h.

---

## 5. `src/core/check/cr_fscheck.cpp` + `src/core/err/` — 1,323 LOC

### cr_fscheck.cpp — 1,092 LOC, ~22 defs (check_fat, check_ntfs, run_fscheck, serialize/deserialize_fs_check + FAT validators/chain/audit-dir/orphan-sweep + NTFS record readers/cross-check)
- **Trail:** **COVERED** — modules/check-phase.md is a complete per-gate disposition
  (taxonomy 0x1001-0x100E with 0x100C preserved gap, phase order, termination
  semantics, the 8-event callback contract, wiring) + M-041 FIXED
  (FIXED-LEDGER.md:143) + registered deltas D-87 (slack term not guessed; CheckIssue
  superset context; partial $Bitmap load fails) + M-075 ACTIVE (WinChkdsk launch
  endpoint absent) + the session-8 addendum (FileSystemCheck_decompiled.c:7099/:7134/
  :7217 CreateProcessA cites).
- **Corpus:** `reversed/ghidra-mo/FileSystemCheck_decompiled.c` **verified** —
  FUN_1800015b0 at :462, FUN_1800045a0 at :2757, FUN_180004d80 at :3061 (the module
  doc's own cites). MOM-modules/FileSystemCheck.md also present.

### cr_errmod.cpp — 28 LOC, 2 fns (errmod::current, errmod::set_thread)
- **Trail:** **COVERED** — M-049 FIXED (FIXED-LEDGER.md:151, "error-model divergence
  and parity claim"); the ABI coupling is pinned by static_asserts in cr_ui.cpp:90-104.

### cr_error.cpp — 107 LOC, 1 fn (err_text — ~40-code table)
- **Trail:** **PARTIAL** — the M-074 landing added the og's numbered destination
  taxonomy (ERR_DEST_PATH_TOO_LONG/INVALID/NETWORK with the og's string shapes,
  cr_error.cpp:60-65; UIUX/11:16 + architecture/04:136-140 cites) and S-029/N-065
  appended their codes. The table as a whole is core-first (the og's error surface is
  Error.mo's codes + UI strings); no dedicated module row exists for `err/`, but
  session-5 read it first-hand (SUMMARY.md:4565/4688) with no finding beyond M-049.
  Informational.

### cr_log.cpp — 96 LOC, ~9 fns (set_sink, enabled, write, check_memory_pressure + write_va/site pool)
- **Trail:** **COVERED** — N-048 FIXED (logging seam), N-062 FIXED (memory monitor,
  the CMemMonitorManager/Log.mo census cite), D-75 registered divergence
  (single-process synchronous seam).
- **NEW-DIVERGENCE-CANDIDATE (trail hygiene, informational):** the in-source comment
  "filed as N-079" for the deque-pool Site-leak fix (cr_log.cpp:32-33) collides with
  the CURRENT FIXED-LEDGER.md:259 N-079 row, which is titled
  "ntfs-recycle-bin-scope-name-prefix-matched-everywhere"; the log-leak filing lives
  at SUMMARY.md:1906-1914 ("N-079 NEW-FINDING (MINOR) filed + FIXED — the ASan lane
  back to green"). One number, two findings — the code comment's cite is ambiguous at
  HEAD. (Cf. N-135, the fix-wave bookkeeping-truth batch, which covers adjacent
  ledger defects but not this collision.)

---

## 6. `src/core/storage/` — 6 files, 2,014 LOC

| file | LOC | public surface | Trail disposition | Corpus ground truth |
|---|---|---|---|---|
| cr_slice.cpp | 71 | PartitionSlice::create/read_at/refresh_geometry | **COVERED** — storage.md (M-016/M-032→D-74 tail-read refusal registered; N-046 FIXED geometry-refresh revalidation hardening) | s3-virtual-volume-device.md (FUN_180009130/:7219-7264); DRWUI FUN_140643870 |
| cr_cached.cpp | 99 | CachedReader::create/read_at | **COVERED** — N-045 FIXED → D-75 (prefetch read-ahead vs true-LRU registered; 8-vs-4 MiB budget) | DRWUI_decompiled.c:919080-:919092, ctor :962250-:962597 (CStorageDeviceWithCache) |
| cr_retry.cpp | 144 | RetryingDevice::create/read_at/try_read/read_isolated, error_map[_copy] | **COVERED** — N-004 catch-buffer backtrack (og CEXT4CatchBufferDeviceProxy, ext4-engine--deep-dive.md §3 — **verified present**, "fresh 0xE00-byte buffer object and retries"), M-049 attribution, N-048 exhaust logging | ext4-engine--deep-dive.md; PartitionRecoveryD PRD:21962/:22121 |
| cr_devmap.cpp | 106 | ReadErrorMap::add/merge/serialize/deserialize/total_bytes | **COVERED** — clean-room R1.6 addition ("reference had no persistent error map at all", cr_devmap.cpp:3-5) + M-049 module-attribution rule; I-012 superset register | none required (core-first); consumed by S-009 bad-overlay + session records |
| cr_decrypt_device.cpp | 76 | DecryptingDevice::create/read_at | **COVERED** — M-033 FIXED (DFX20; storage.md:22); OQ-S20-1 tracked-open (recovery-password/BEK→VMK→FVEK unwrap out-of-corpus) | bitlockerlib--bitlocker-implementation.md; DRWService artifact §"Volume content decrypt: XTS-AES via OpenSSL" |
| cr_image.cpp | 1,518 | open_image/classify_extension/classify_lba0 + VMDK/VHD/VHDX/Flat device classes | **COVERED** — imaging.md (S-030 FIXED + session-10 convergence table: VHDX dynamic/log-replay D-64d, VMDK parent chains, VHD differencing, .dmg flat-fallback MATCH, refusal families; D-64 registered exclusions edisk/compressed/ESX) + S-040 ACTIVE (image CREATE absent) | DRWUI_decompiled.c:948237-948409/:275877-80 (extension dispatch); Device.mo libvmdk :49106-49124/:48965-49100; qemu-img fixtures in tests/data/images |

**Module verdict:** all six dispositioned; nothing new surfaced. The in-code deliberate
improvements (catch-buffer 4Kn alignment at cr_retry.cpp:80-83; static-media N-072
markers) are each tagged to their findings.

---

## 7. `src/win/` — 8 files, 2,842 LOC

| file | LOC | public surface | Trail disposition | Corpus ground truth |
|---|---|---|---|---|
| cr_transport.cpp | 1,074 | overlapped_read/probe_geometry/aligned_read_at (64 KiB envelope), DirectDevice, RdaTargetDevice, rda::compute_pid_token/control_device_path, query_driver_status, open_rda_partition/volume, open_disk, open_disk_rw/apply_rw, refresh_disk_properties, volume_extent[s], destination_volume_letter, RwDevice (S-010 quiesce + S-029 device-control chain) | **COVERED** — transport.md (P51/P47/P35/P59/P60/P8/P46/P69/P62/C22 verdicts; M-030 → resolved by the PLAN-DRIVER-ONLY posture; N-041 RdaClient deleted; I-016 wire register); M-089 ACTIVE (RDA-bench phantom zero-extended IOCTL fuzz gap); GAP-S6-13 CLOSED (unified 64 KiB envelope) | Device_decompiled.c (grammar :50628, chunking :56545/:56912, stale-handle :55947-59); ghidra-partition/RecoveryModule_decompiled.c (quiesce :60401-60447, restore :34465-34502); DRIVER-CONTRACT.md §8 |
| cr_elevate.cpp | 24 | win::is_elevated | **COVERED** — GAP-P1-5 CLOSED (837a543, CORE-GAP-LIST.md:46); storage.md:74-75 ("byte-shape twin of DRWUI's elevation check … deliberate, no finding") | corpus architecture/02 (service elevation model) |
| cr_destio.cpp | 183 | WinDestIo::create_temp/write/free_bytes/commit/close/exists/mkdir + classify_dest_error | **COVERED** — writer.md (N-056 FIXED rename-failure temp cleanup; N-015 ENOSPC classification; M-074 path taxonomy; G-08 mkdir EEXIST contract, the MSVC #227 incident) | UIUX/11 (no orphaned temps; numbered destination strings); RecoveryModule :47033-47038 |
| cr_enumerate.cpp | 257 | enumerate_disks, enumerate_disk_interfaces, enumerate_volumes, query_disk, open_disk_for_query | **COVERED** — storage.md session-6/30 rows (N-071 FIXED two-stack merge → D-79/D-80; N-080 SDK-exact descriptor offsets; N-066 WWN/interface axes; busy-disk metadata retry) + **M-076 ACTIVE** (TRIM facet rows) | mom-module-system--architecture.md :57-59 (WMI/SetupAPI stacks); getdriverinfo--tb-module-decode.md |
| cr_health.cpp | 346 | read_smart, read_nvme_health, query_trim_state, looks_like_ssd, smart_attr_name + 3 AtaChannel transports | **COVERED** — health.md / CROSS-REF-MATRIX.md:230 (N-032 FIXED CSMI CC-0x19; S-024 FIXED grading decode; M-036 FIXED NVMe fields; N-042 FIXED temperature; N-055 FIXED; N-009a) | DiskHealth_decompiled.c (FUN_00425450 CSMI, :14166-14176 SMART IDEREGS, FUN_00417750 SCSI address); diskhealth--smart-companion.md |
| cr_bitlocker.cpp | 508 | is_fve_volume, status, unlock, unlock_with_dictionary ×2, try_auto_unlock[_offline], detail::enum_auto_unlock_keys/apply_auto_unlock_keys/build_unlock_command/dictionary_unlock_seam | **COVERED** — health.md F61/B1 cells → MATCH (CROSS-REF-MATRIX.md:230/417-418: S-014 FIXED live+offline FVEAutoUnlock; N-064 FIXED dictionary); **M-078 ACTIVE** (BitLocker turn-off absent); OQ-S20-1 tracked-open | DRWService_decompiled.c:104-140 + artifacts/DRWService-exe--bitlocker-service.md:56-74 (0x138 stride, 50-char GUID, 256-B blob, 100 records — verified); bitlockerhelp_decompiled.c:3128; RegLib_decompiled.c:2376-77 |
| cr_bootfix.cpp | 311 | bcd_file_plan/execute/undo, force_unload_bcd_hive, nvram_plan/execute/undo, firmware_seam_available, enable_system_environment_privilege | **COVERED** — boot-repair.md (M-040 FIXED stages a+b: the BCD00000000 force-unload ZwUnloadKey under SeRestore/SeBackup, the dynamic firmware seam :3281-3290, P62 default-deny consent); **N-121/N-122 ACTIVE** (ESP auto-mount; NVRAM create/delete exclusion) | Fixup.mo :533/:542/:243-255; EFIBoot :3281-3290/:3248-3270 |
| cr_image_win.cpp | 119 | WinImageFileIo (open/read/size/close) + image_file_io() | **COVERED** — imaging.md ("Win32 implementation src/win/cr_image_win.cpp"; the CreateFileW+GetFileSizeEx flat-fallback primitives DRWUI:948389-948393) | DRWUI_decompiled.c:948389-93 |

**Module verdict:** all eight dispositioned. The driver-only architecture (removed
WinPE gate / direct fallbacks, cr_transport.cpp:563-575) is a *deliberate* registered
divergence (PLAN-DRIVER-ONLY R1-R14; transport.md:138 "annotate rather than re-audit").

---

## 8. `src/app/rrecov.cpp` (3,160 LOC) + `src/ui/cr_ui.cpp` (4,325 LOC, heads + recovery/arming paths read)

### rrecov.cpp — 11 verbs (wmain dispatch verified: list, list-carve-formats, sessions, scan, tree, recover, health, fix, apply, bench, driver), ~48 defs
- **Trail:** **COVERED** — cli.md (own-contract CLI — "the og has NO CLI"; C24 exit-code
  table incl. exit 4 driver prerequisite; the N-123 11-verb recount with the four locked
  doc sites; N-020 doc-drift FIXED; N-009c --filter ACTIVE). The CLI is cited by many
  active findings as the exposure surface (S-040 image-create, S-051 wholedisk targets,
  S-052 LDM, M-076 TRIM, M-111 explorer-browse, N-121 ESP, N-133 volume label).
- **Corpus:** none required (core-first surface); og-side analogs are the GUI funnel
  (04-recovery-workflow.md) and the og's own log/progress strings cited in-source
  ("%1 files %2 have been…" architecture/04:113).

### cr_ui.cpp — C-ABI shim, ~83 exports in-file (header census 208 `cr_ui_` refs), ABI_VERSION 3
- **Trail:** **COVERED** — abi.md (GAP-G-01 CLOSED 2a93c45; 81-export census at v3
  re-verified, abi.md:99; P42-P46/C22 verdicts) + N-137 ACTIVE (thumbnail ABI header
  contradiction) + N-075/N-090 rows. Layer rules ("BINDING layer — no engine logic")
  pinned in the file head.
- **Corpus:** og's plugin contract (EUCreate<FS>Searcher factories, EUGetAnalyserList/
  Map/Multimap) — searchdll-family--architecture.md; MOM GetModule/CreateObject.

---

## 9. Corpus ground-truth index (verified exists, per module)

report/dictionary/safewriter (mission ask #1): `config-files/config-data-reference.md`
(Dict §) · `DRWUI-subsystems--assistant-grading-preview.md` §1-2 ·
`architecture/04-recovery-workflow.md` §7.3-7.5 · `reversed/ghidra-partition/
RecoveryModule_decompiled.c` · `reversed/ghidra-mo/{LdmManager,Partition,
FileSystemCheck,Error,Log via ghidra-mo}_decompiled.c` · `SaveScan/rsf-record4--
tree-node-grammar.md` + ifileproperties-part1-3 · `SearchDLL/ext4-engine--deep-dive.md`
§3 · `UIUX/08-scan-phase-deep-dive.md` · `SearchDLL/searchdll-family--architecture.md`
:31 · `PartitionRecovery/pending-operation-engine.md` · `DRWService-exe--bitlocker-
service.md` · `DiskHealth` family · `MOM-modules/{Error,Log,FileSystemCheck,LdmManager,
mom-module-system--architecture}.md` · `PartitionRecovery/mo-module-map.md`.

---

## 10. NEW-DIVERGENCE-CANDIDATES (evidence or nothing)

### #1 (STRONG — a FIXED finding whose landing was clobbered at both consumer call sites): N-013 multi-extent destination arming
- **Core at HEAD:** `rrecov.cpp:2015-2041` — cmd_recover arms the writer with the
  drive-letter gate + `win::volume_extent()` (FIRST EXTENT ONLY) +
  `writer.set_destination(dstart, dlen)`; `cr_ui.cpp:3863-3870` —
  `recovery_set_destination_impl` does the same. Meanwhile the N-013 machinery exists
  and is dead at these sites: `SafeWriter::set_destination_spans()` (cr_safewriter.cpp:29-35),
  `win::volume_extents()` all-8-extents (cr_transport.cpp:1008-1035), and the header
  contract states "destination arming **must use volume_extents() (all spans)**"
  (include/cr/win/cr_transport.h:124-126). Grep: zero call sites of
  `set_destination_spans`/`volume_extents` in rrecov.cpp or cr_ui.cpp at HEAD.
- **Git proof:** commit `3b5397a1` (fix N-013) landed the spans call sites
  (rrecov@3b5397a1:984-1004 uses destination_volume_letter + volume_extents +
  set_destination_spans; cr_ui@3b5397a1 has 2 spans-shape matches). The very next
  commit touching these files, **`1ebd1bcc` ("fix(M-036): NVMe health …" — unrelated),
  reverted both hunks** back to the pre-N-013 singular form (its diff deletes the
  "first-extent-only missed overlap there" block and re-adds the `':'`-gated singular
  form). Never restored through HEAD. Same clobber class the trail itself documents
  (c4e37d13's "repair the 5aaf044 'style' clobber"; M-071's N-064-wave gate drop).
- **Trail disposition status:** N-013 is recorded FIXED (FIXED-LEDGER.md:193;
  writer.md:81-82 "all-extent arming + any-resolving path arming") — that verdict is
  **false at HEAD for the consumer sites** (the core machinery + the DestArming tests
  survive; the tests exercise the win:: helpers, not the rrecov/cr_ui call sites,
  which is why the clobber was invisible).
- **og side:** N-013's own corpus cite — "the og ALWAYS opens `\\.\X:` for drive-letter
  destinations and uses the full multi-extent result
  (RecoveryModule_decompiled.c:60381-60444)"; architecture/04-recovery-workflow.md:229.
- **Impact:** a spanned/RAID destination volume's second+ extents are unarmed for the
  source/destination overlap refusal (F12/R7.2) — silent-wrong-bytes-class exposure on
  exactly the shape S-052 already flags for LDM sources.

### #2 (Trail-hygiene): N-079 ID collision (cr_log.cpp:32-33 vs FIXED-LEDGER.md:259 vs SUMMARY.md:1906-1914) — see §5 cr_log. Informational; suggest a ledger renumber/annotation row.

### #3 (ACTIVE, already filed — listed for completeness): N-132 grading category_counts
vs og SumaryFileType unification (query.md:70-90; core cr_grading.h:115) — the only
tree/-module behavioral residue.

### #4 (Small, informational): cr_table_parse.cpp:78-81 stricter salvage gate
(entry_size == 128 vs og's hardcoded 0x80 stride) — documented in-source only; no
PARITY-DELTAS row. Suggest registering.

### #5 (Informational, no og-side contradiction): canonical `cr::dict` (N-064 wave)
lacks the dedup/caps sequencer features of the superseded DFX34 `cr::pwddict` landing
(CROSS-REF-MATRIX.md:418 describes "dedup, caps"; cr_dict.cpp's try_passwords has no
dedup). The og's BitLockerLib dictionary-loop semantics are not decoded in the corpus,
so there is no ground truth to diverge from — observation only.

---

## 11. CORE-FIRST features identified (informational, per mission item e)

1. The recovery-report JSON + always-computed SHA-256/hash_status (cr_report.cpp,
   cr_safewriter.cpp) — G-09/issue #6; "the og has no JSON report" (writer.md D13).
2. `ReadErrorMap` persistent error geometry (cr_devmap.cpp) — "reference had no
   persistent error map at all … clean-room addition required by R1.6".
3. `err_text` error-string table + `errmod` thread attribution (err/) — M-049's model
   on a core-first table.
4. Mid-scan immutable `Snapshot` + G-02b writer value-enqueue (cr_snapshot.cpp) —
   no og counterpart.
5. LDM recovery supersets: damaged-PRIVHEAD backup replacement, config clamp,
   forgiving TOCBLOCK walk, VBLK v4 floor (cr_ldm.cpp) — registered.
6. fscheck D-87 hardening: partial-$Bitmap-load refusal; CheckIssue superset context
   (cr_fscheck.cpp) — registered.
7. N-046 slice geometry-refresh revalidation (refuse over silent adoption) —
   registered FIXED.
8. N-004 catch-buffer backtrack generalized to all transports with 4Kn-aligned step
   (cr_retry.cpp:80-83 "deliberate improvement") — registered via N-073/N-004 family.
9. The whole rrecov CLI + exit-code contract (og has no CLI) — cli.md.
10. Dictionary grammar totality + hostile-blob bounds (cr_dict.cpp) — superset.
11. apply A1-A6 consent/transaction safety standard (cr_apply.cpp) — superset of the
    og's funnel, D-72c the one registered honesty divergence.
12. 64 KiB unified direct-I/O envelope (cr_transport.cpp) vs og 4/7 MiB chunks —
    GAP-S6-13, deliberate, corpus-grounded.

---

## 12. Bottom line

- **Under-dispositioned modules:** none of the assigned files is truly
  NOT-COVERED. The thin zone is **tree/ (no dedicated module doc — coverage is
  scattered across orchestration/visualization/ldm/session rows + findings)** and
  **err/ (no module doc; M-049/N-048/N-062 findings only)**. Both are adequately
  dispositioned in substance; a bookkeeping module row each would complete the board.
- **The one real code-level defect found:** N-013's consumer regression
  (Candidate #1) — the FIXED verdict is falsifiable at HEAD with a two-grep +
  one-git-show proof.
- **Top follow-ups:** (1) re-land the N-013 spans call sites in rrecov.cpp +
  cr_ui.cpp (or register why not); (2) register the cr_table_parse salvage strictness
  as a D-row; (3) resolve the N-079 ledger collision; (4) close or register N-132's
  category_counts decision; (5) consider modules/tree.md + modules/err.md rows for
  board completeness.
