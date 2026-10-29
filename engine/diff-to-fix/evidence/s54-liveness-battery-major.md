# Liveness battery — MAJOR tier (pin 2794da8b, agent B)

Snapshot taken at the audited pin `2794da8b` (verified: `git rev-parse --short HEAD` → `2794da8b`, clean working tree). 26/26 findings in `diff-to-fix/findings/MAJOR/` verified. Retirement sweep: **none of the 26 has a `findings/FIXED/` copy or a FIXED-LEDGER row** → no FALSELY-ACTIVE verdicts; every ACTIVE row is truthfully active at this pin (23 LIVE defects + 3 pushed-forward driver-repo residents). The parallel session-52 engine-math claim (M-126..M-136) has landed **nothing** in code or ledger as of this pin — all 11 of its findings in scope verify LIVE.

## Verdict table

| ID | Title (short) | Verdict | Current-code evidence (file:line) | Retirement bookkeeping? |
|----|----|----|----|----|
| M-073 | Scan-time folder filter absent | LIVE | `include/cr/orchestrator/cr_scan.h:186-345` (no `filter_folders` field); `include/cr/cr_ui.h:562-639` (no filter fields); no `--filter-folder` in `src/app/rrecov.cpp` | None owed — no FIXED copy, no ledger row |
| M-075 | chkdsk launch absent | LIVE | `src/win/` has no `cr_fsck_volume_win.cpp`; no `cr_ui_fsck_volume` export; `docs/PARITY-DELTAS.md:3044` "The chkdsk delegation stays OUT OF CORE" | None owed |
| M-078 | BitLocker turn-off absent | LIVE | 0 code hits for `turn_off|TurnOff` in src/include/tests; `src/win/cr_bitlocker.cpp` has unlock/auto-unlock only; no `--turn-off` CLI | None owed |
| M-086 | Driver FileVersion pin defeats Versioned layout | UNCERTAIN (defect site out-of-repo) | `src/driver/rda_version.h` + `scripts/install.ps1` live in heicfast/rda-driver (absent locally); core-side floor `-FloorVersion v0.2.4` sound; `diff-to-fix/tracked-open/OQ-S10-3-driver-repo-resident-fixes.md` still OPEN | PUSHED-FORWARD intact (OQ-S10-3); active-by-design |
| M-087 | regdrv-rda.reg UpperFilters invalid hex(7) | UNCERTAIN (defect site out-of-repo) | `scripts/winpe/regdrv-rda.reg` is a heicfast/rda-driver path, not in this tree (only an unrelated evidence snapshot); OQ-S10-3 still OPEN | PUSHED-FORWARD intact; active-by-design |
| M-089 | rdabench phantom-zero extended-IOCTL fuzz gap | UNCERTAIN (defect site out-of-repo) + core-side residue LIVE | `src/bench/rdabench.c` is driver-repo (absent); but `bench/ours/verify-driver.ps1:47,55` still calls `DeviceIoControl(h, 0x60002440/0x60002448, …)` — the phantom codes, vs the real `0x6002440/44/48` in `include/cr/win/cr_rda.h:55-57` | PUSHED-FORWARD intact (OQ-S10-3); active-by-design |
| M-101 | Results-tree Versions/Tags-Extracted branches absent | LIVE | `rg "version_group|VersionsNode|TagsExtracted" src/ include/ tests/` → 0; `docs/REFERENCE-API-GAP.md:214` still "Missing: 'Versions node' … and 'Tags Extracted'" | None owed |
| M-105 | DRWService install lifecycle absent/unscored | LIVE | 0 hits for `CreateService|StartServiceCtrlDispatcher` in src/include/installer; R-83 note still false at `docs/REFERENCE-API-GAP.md:425` ("install lives in driver-repo CI scripts") | None owed |
| M-107 | Batch-1 size-walk stand-in residue unowned | LIVE | All 11 sites standing in `src/core/carve/cr_analysers.cpp` (x3f :4854, crw :4878, wpg :4967, wpd :4984, psp :5006, sit :5023, ape :5099, eps :5472+5481, dxf :5516, dwg :4410, swf :5766) | None owed (N-146 registered FH11 as 12th site; N-148 re-pinned EPS/AI caps) |
| M-111 | Explorer partition browse capability absent | LIVE | 0 hits for `CWindowsFS|IFSInterface|LiveFsAdapter|ExplorerPartition` in src/include; `FindFirstFileExW` only at `src/ui/cr_ui.cpp:397` + `src/app/rrecov.cpp:1073` (session listings); no PARITY scope row | None owed |
| M-113 | Recovered-OS boot surgery absent | LIVE | `rg "TmpBCD|TempSystem|TmpSoftWare|bootstat|9dea862c|23000003|12000004" src/ include/ docs/` → 0; `cr_bootfix.cpp` still bcd_file_plan/apply/execute/undo + nvram only; R-69 still "PRESENT" (`REFERENCE-API-GAP.md:293`) | None owed |
| M-114 | System-Restore purge absent + undecided | LIVE | `rg "srclient|SRRemoveRestorePoint|SystemRestore|restore point" src/ include/ tests/ docs/` → 0; no D-row, no plan item | None owed |
| M-115 | Thumbnail grade promotion decode-gated, not read-gated | LIVE | `src/ui/cr_ui.cpp:4016-4029` (promotion only on `make_thumbnail == Ok`); `:3956-3957` (`DecodeFailed → ERR_TRUNCATED` before promotion) | None owed; note: the og-cite comment (:4025) now reads ":422507-514" — fix-plan step 6's cite correction appears applied |
| M-118 | Lost-part three-source assembly register channel absent | LIVE | `rg "MountPoints|CPC\\Volume|mountpoints" src/ include/ tests/` → 0; no `CandidateSource` seam; no `win::mountpoints2_volume_history()` | None owed (S-051's table channel landed; register channel still unowned) |
| M-119 | Partition-boundary read semantics clamp-vs-refuse | LIVE | `src/core/storage/cr_slice.cpp:28-32` (create refuses overrun), `:46-47` (read hard-error, no zero-fill); `src/app/rrecov.cpp:3245-3247` ("cannot slice partition" + `return 2`); `src/ui/cr_ui.cpp:1381/1466` | None owed |
| M-126 | ETA model absent (rate-EMA decoded) | LIVE | 0 hits for `eta_seconds|remaining_time|EMA|smoothed_rate` in src/include/tests; `rrecov.cpp:1931` prints elapsed only; O39 M-note stands (`modules/orchestration.md:51`) | None owed |
| M-127 | FAT/exFAT cycle guard O(n²) vs og range tree | LIVE | `src/core/fat/cr_chain.cpp:429/442-447` (vector + linear scan) and `:475/486-491`; exFAT twin `src/core/exfat/cr_exfat_quick.cpp:94-107` | None owed |
| M-128 | User-sign hex-case: 12 og-dead signs live in core | LIVE | `src/core/carve/cr_usersigns.cpp:77-91` (`hex_nibble` accepts 'a'-'f' AND 'A'-'F'), byte-compare at `:107-114`; no case registration in PARITY-DELTAS | None owed |
| M-129 | EUUserData parser all-or-nothing vs og tolerant skip | LIVE | `cr_usersigns.cpp:174` (exact-case `strcmp(key,"signs")`), `:207` (unknown key → reject), `:110` (odd hex → reject), `:221` (trailing garbage → reject) | None owed |
| M-130 | FAT dentry shape-validator family unported | LIVE | No `dentry_shape_ok|histogram|ext-tail` gates in `src/core/fat/`; only N-010c date gate (`cr_fat_quick.cpp:234`, default OFF at `cr_fat_engine.h:257`) | None owed |
| M-132 | GPT backup probe position + dead-LBA1 asymmetry | LIVE | `src/core/table/cr_table_parse.cpp:385-388` (primary OK → probe at `prim.backup_lba`, not device-last-first); `:403-418` (dead primary → dev_last, emits entries) | None owed (M-123's validation tightening landed around it; position unchanged) |
| M-133 | Twin-key device-family set wrong ({FAT12,16,32,exFAT} vs og {FAT16, EXT2}) | LIVE | `src/core/tree/cr_grading.cpp:138-140` (`fat_family = Fat12||Fat16||Fat32||Exfat`, −2 shift); ext family unshifted | None owed — NB `findings/FIXED/M-133-suite-red-at-head…` is a DIFFERENT finding (ID reuse after renumber), not this one's retirement |
| M-134 | Grading deep-read validation absent | LIVE | No `GradeReader|deep_read` anywhere; `cr_grading.cpp:338-346` (`regrade_node` = `solo_rules` only, no graded-bit); `cr_grading.h:27` still "UNDECODED" | None owed; OQ-S42-3 tracked-open present |
| M-135 | Recovery read-failure semantics (truncate+number+keep vs zero-fill+delete) | LIVE | `src/core/recovery/cr_safewriter.cpp:469-471` (read fail → memset zero + `had_unreadable` + continue); `include/cr/win/cr_destio.h:36-43` (temp deleted on every !renamed exit); no cause taxonomy, no "Failed to ReadSector" log family | None owed |
| M-136 | Recovery reads bypass retry/cache pipe | LIVE | `src/app/rrecov.cpp:2323` + `src/ui/cr_ui.cpp:4118` hand the RAW device to `SafeWriter`; `build_read_pipe` (`cr_scan.cpp:162`) scan-only | None owed |
| M-137 | Lost-part surfaced shape (naming/cap/Quick/512-fallback) | LIVE | `src/core/orchestrator/cr_scan.cpp:2054-2058` (`u"Candidate @"` + LBA); `cr_candidates.cpp:759-760` (`logical_sector == 0` → `ERR_BAD_PARAMETER`); no cap, no "Lost Partition" template, no Quick staging | None owed |

## Per-finding detail

### M-073 — LIVE
- Finding's claim: the og pushes a pre-set folder filter into the scan controller (`CDoScanPartition::PushFolderFilter`), constraining incremental tree insertion; the core has no scan-time folder-filter input in `ScanConfig` or its ABI twin — only post-hoc query filtering plus the fixed M-018 junk blocklist.
- Current code: `include/cr/orchestrator/cr_scan.h:186` `struct ScanConfig {` — full field census (run_quick…events, read this battery) contains no filter input; the only "filter" text in the struct region is the `filter_generalraw` comment. ABI twin `include/cr/cr_ui.h:562-639` `cr_ui_scan_config` — fields end at `autosave_dir_utf8`, no `filter_folders_utf8`. `src/app/rrecov.cpp` has `--filter` (name substring, query-side, e.g. :1484/:2173) but no `--filter-folder`. `rg "filter_folder|filter-folder" src/ include/ tests/` → 0 code hits (docs/plan only).
- Tests present? None — `rg "ScanTimeFolderFilter" tests/` → 0.
- Notes: line drift per the finding's own S11B addendum (ScanConfig now opens at :186, closes ~:345; junk pass now `cr_scan.cpp:1328-1330` region). `worklog.md:3674` lists M-073 under "Remaining of this lane's declared territory" — a PLAN, not a landing. A prior battery (`diff-to-fix/evidence/s12-liveness/liveness-1.md`) concurs STILL-ACTIVE.

### M-075 — LIVE
- Finding's claim: the og's check-escalation ladder ends in a real chkdsk launch (WinChkdsk.exe wrapping `\System32\` chkdsk); the core has only the in-engine 0x1001..0x100e audit (M-041) with no launch surface — the 0x100E advice is unactionable from the product.
- Current code: `src/win/` contains only `cr_bitlocker/cr_bootfix/cr_destio/cr_elevate/cr_enumerate/cr_health/cr_image_win/cr_transport` — no `cr_fsck_volume_win.cpp`. `rg "fsck_volume|FsckVolume" src/ include/ tests/` → 0. `include/cr/check/cr_fscheck.h:97` still just the taxonomy: `CHK_CHECK_EXCEPTION = 0x100E, /* catastrophic: "use Chkdsk /F" */`. `docs/PARITY-DELTAS.md:3044`: "The chkdsk delegation stays OUT OF CORE: spawning an external binary is the host's job…" — the D-90-class posture the finding says is superseded remains un-rewritten (fix-plan step 4 undone).
- Tests present? None — no `FsckVolume.*` tests.
- Notes: none beyond line drift of the PARITY row (was :2977).

### M-078 — LIVE
- Finding's claim: the og coordinates BitLocker state with a transactional `CBitlockerTurnOffOperation`; the core's surface is detect/unlock/auto-unlock only — no export or win-layer function turns BitLocker off.
- Current code: `rg "turn_off|TurnOff|turn-off" src/ include/ tests/` → 0 code hits (only `docs/PLAN-API-EXPANSION.md:70`, the finding, and `API-DELTA.md:260` "ABSENT (grep)"). `src/win/cr_bitlocker.cpp` census: `harvest_offline_keys` (:278), `try_auto_unlock_offline` (:496) — no turn-off/decrypt function; the command builder has no `-off` mode. CLI unlock path (`rrecov.cpp:589-636`): dictionary + auto-unlock only, no `--turn-off`.
- Tests present? None for `Bitlocker.TurnOff*`.
- Notes: none.

### M-086 — UNCERTAIN (defect site outside the audited repo)
- Finding's claim: the driver's `-Layout Versioned` tag derives from the binary's FileVersion resource, pinned at 1.0.0.0 since first release with no release-workflow stamping — every v0.2.x pair collides at install exit 2, defeating M-064's upgrade contract.
- Current code: the entire fix surface (`src/driver/rda_version.h`, `src/driver/rda.rc`, `scripts/install.ps1:99-174`, driver `release.yml`) is in heicfast/rda-driver. That repo is NOT present under `/home/z/my-project/repos/` (only `core_recovery` + `easeus-drw-research`), and no vendored copy exists in this tree (`rg --files | rg "rda_version"` → only evidence snapshots under `diff-to-fix/evidence/executions/devsign-run-…/release-scripts/`). Blocked from a first-hand liveness check of the defect site.
- Tests present? N/A (driver-repo CI drills).
- Notes: bookkeeping verified instead — the finding carries "Status: PUSHED-FORWARD (tracked-open OQ-S10-3)" and `diff-to-fix/tracked-open/OQ-S10-3-driver-repo-resident-fixes.md` is still OPEN at the pin (lists M-086 first, with the M-087→M-089→N-107/N-127→M-086 order). Active-by-design; no retirement owed. Core-side half verified sound at the pin: `release.yml` consumes the driver via `-FloorVersion v0.2.4`.

### M-087 — UNCERTAIN (defect site outside the audited repo)
- Finding's claim: `scripts/winpe/regdrv-rda.reg` writes the DiskDrive-class `UpperFilters` as `hex(7):r,d,a,\,0` (5 ASCII bytes) — an invalid REG_MULTI_SZ encoding, so the WinPE filter never attaches; the og's own regdrv.reg shows the correct 10-byte UTF-16LE form.
- Current code: the defect file is a heicfast/rda-driver path; not present in this tree (the only `install.ps1` hits are evidence snapshots). Blocked from first-hand verification of the live line.
- Tests present? N/A (driver-repo gate `RegHex7GrammarGate`).
- Notes: PUSHED-FORWARD bookkeeping intact (OQ-S10-3 OPEN, item 2, with the exact corrected byte sequence recorded in the finding body). Active-by-design. A future battery with the driver repo cloned should re-run `od -t x1z` on `scripts/winpe/regdrv-rda.reg:31` and check for the `.reg` parse gate.

### M-089 — UNCERTAIN (defect site out-of-repo) + core-side residue LIVE
- Finding's claim: the driver's fuzz-hammer (`rdabench.c:390`) probes the extended IOCTL family at phantom-zero values `0x60002440/44/48` instead of the real `0x6002440/44/48` — the live extended handlers get zero fuzz coverage and the gap is invisible in green runs.
- Current code: `src/bench/rdabench.c` is a heicfast/rda-driver path (absent locally; this repo's `bench/` is the perf-bench harness — no rdabench.c). Blocked on the primary site. HOWEVER the phantom-zero defect class is verifiably ALIVE inside this repo's own Windows verification script: `bench/ours/verify-driver.ps1:47` `bool ok = DeviceIoControl(h, 0x60002440, null, 0, b, 24, out ret, IntPtr.Zero);` and `:55` `DeviceIoControl(h, 0x60002448, …)` — both phantom codes, while the real constants sit in `include/cr/win/cr_rda.h:55-57` (`kIoctlGetInfo = 0x6002440u; kIoctlOpenTarget = 0x6002444u; kIoctlGetVersion = 0x6002448u`, with the 2026-10-27 wire-truth note at :44-49). The GET_INFO/GET_VERSION legs of that script therefore probe codes the dispatch rejects.
- Tests present? N/A for rdabench; no `BenchProbeSet-Covers-Dispatch-Table` gate in this repo.
- Notes: PUSHED-FORWARD intact (OQ-S10-3 item 3). The `verify-driver.ps1` residue is a core-repo instance of the same class the finding did not cite — flagged here for the fix lane (it would make the script's Info()/version legs always report "rejected").

### M-101 — LIVE
- Finding's claim: the og's results tree has five distinguished roots; core implements three — the "Versions" node (NTFS multi-version record siblings) and "Tags Extracted" (EXIF/tag grouping) have no tree/query concept at all.
- Current code: `rg "version_group|versions_node|VersionsNode|TagsExtracted|tags_extracted|multi_version" src/ include/ tests/` → 0 (the only `kVersions` hit is a Camera-RAW version list in `cr_analysers.cpp:4418`). Query axes (`include/cr/query/cr_query.h:128-157`) carry name/path/state only. The board row still records the absence: `docs/REFERENCE-API-GAP.md:214` "**Missing: 'Versions node' (NTFS multi-version siblings) and 'Tags Extracted' (EXIF tag grouping) — no tree concept for either.**"
- Tests present? None (`Ntfs.VersionsNodeGroupsTornSiblings`, `Query.TagsExtractedGrouping`, etc. — 0 hits).
- Notes: none.

### M-105 — LIVE
- Finding's claim: the og ships a complete transient SCM service lifecycle for BitLocker FVE key access ("Read FVE Server", AUTO_START/LocalSystem, /install + /uninstall, stop-gated delete); the core has no counterpart, the K71-K73 slice is unscored, and R-83's disposition note is factually wrong.
- Current code: `rg "CreateService|StartServiceCtrlDispatcher|SERVICE_AUTO_START" src/ include/ installer/` → 0 code hits (DRWService appears only in comments: `cr_bitlocker.cpp:22-24,368-371,445`, `cr_reghive.cpp:42` — the S-014 in-process data path). `docs/PACKAGING.md` grep for DRWService/FVE → 0. R-83's false note stands at `docs/REFERENCE-API-GAP.md:425`: "| service lifecycle (R-83) | DRWService:9-30 | none in core | GAP | install lives in driver-repo CI scripts |" (fix-plan step 3 undone) — and :321 repeats it.
- Tests present? None (`FveService.*` / `Bitlocker.AutoUnlockAclDeniedDiagnosis` — 0 hits).
- Notes: none.

### M-107 — LIVE
- Finding's claim: eleven batch-1 formats still carve at the DWG-pattern 64 MiB bounded extent where the og performs a per-format size walk; S-007's "recorded follow-ups" were never re-registered until this finding.
- Current code: every stand-in comment survives in `src/core/carve/cr_analysers.cpp`: `validate_x3f` :4854 ("the section walk is the recorded follow-up"), `validate_crw` :4878-4879 ("FUN_180052160 is the recorded follow-up; the DWG-pattern bounded extent stands in"), `validate_wpg` :4967, `validate_wpd` :4984, `validate_psp` :5006, `validate_sit` :5023, `validate_ape` :5099, `validate_eps` :5472 + `:5481-5483` (`r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));` with "N-148: 64 MiB - the M-107-verified stand-in shape"), `validate_ai` :5505-5507 (same 64 MiB N-148 pin), `validate_dxf` :5516 ("bounded extent stands in for the text walk"), `validate_dwg` :4410 ("a full section walk is a follow-up; the version gate + bounded extent is the honest floor"), `validate_swf` :5766 ("The RECT bit-walk past byte 8 is the recorded follow-up"). All still `64 << 20` / bounded-extent.
- Tests present? None of the plan's `plant_<fmt>_over_cap` / `plant_<fmt>_torn` family (the plan's per-format tests are for the fix landings).
- Notes: line drift (session-11B cites were :3465-4312; now :4406-5766 after later landings). N-146 (FIXED) registered the FH11 site as the twelfth member — comment at :4476 confirms ("stand-in is REGISTERED (M-107's enumeration, the twelfth site"). N-148 (FIXED) re-pinned EPS/AI from 16 MiB to 64 MiB — the stand-in substance (no size walk) is unchanged, so the verdict stays LIVE on all eleven batch-1 sites.

### M-111 — LIVE
- Finding's claim: the og offers per-partition in-app browsing of existing files via the ExplorerPartition→CWindowsFS live-mounted-FS adapter (plus used-space and root-openability consumers); the core has no counterpart seam and no scope registration.
- Current code: `rg "CWindowsFS|IFSInterface|LiveFsAdapter|ExplorerPartition" src/ include/` → 0. The only Win32 find uses remain the session listings: `src/ui/cr_ui.cpp:397` and `src/app/rrecov.cpp:1073` (`FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, …)`). The "browse" hits in the tree are M-116's archive browse-inside (different, landed capability). No PARITY-DELTAS row registers the mounted-volume browse scope decision (`rg "browse" docs/PARITY-DELTAS.md` → only browse-while-scanning/UIUX rows).
- Tests present? None (`Browse.MountedVolumeEnumeration` etc. — 0 hits).
- Notes: none.

### M-113 — LIVE
- Finding's claim: the og performs offline registry-hive surgery on the recovered OS (MountedDevices rewrite as "TempSystem", BCD device-element re-pointing via "TmpBCD" {9dea862c…}→23000003→12000004, SOFTWARE-hive "TmpSoftWare", SystemSetup/bootstat); the core has none of it and R-69's "PRESENT" verdict over-covers.
- Current code: `rg "TmpBCD|TempSystem|TmpSoftWare|bootstat|9dea862c|23000003|12000004" src/ include/ docs/` → **0 hits**. `src/win/cr_bootfix.cpp` still exposes only the cell-repoint family (`bcd_file_plan` :174, `bcd_file_apply` :202, `bcd_file_execute` :240, `bcd_file_undo` :247) + NVRAM (`nvram_plan/execute/undo` :279-303) + `force_unload_bcd_hive` — no `cr_reghive_write.*`, no hive-load surgery. `docs/REFERENCE-API-GAP.md:293` still reads "| R-69 boot repair | `bcdfix`/`nvramfix` (M-040a/b) + CLI `apply --boot-fix/--boot-fix-bcd` (§a/§c.8) | PRESENT |…" (fix-plan step 5 undone).
- Tests present? None (`Bootfix.MountedDevicesRewriteRoundTrip` etc. — 0 hits).
- Notes: none.

### M-114 — LIVE
- Finding's claim: around copy/repartition operations the og silently deletes ALL System Restore points on both involved drives (srclient.dll SRRemoveRestorePoint via WMI, 510-index fallback); the core has nothing and no posture decision is registered.
- Current code: `rg "srclient|SRRemoveRestorePoint|SystemRestore|restore point|RestorePurge" src/ include/ tests/ docs/` → **0 hits**. No plan item in `src/core/apply/cr_apply.cpp` / `include/cr/apply/cr_apply.h` (grep for host-visible/purge operations → 0). No PARITY-DELTAS D-row.
- Tests present? None (`Apply.PlanExposesRestorePurgeItem` — 0 hits).
- Notes: none.

### M-115 — LIVE
- Finding's claim: the og promotes a node's grade to 2 on clean recovery-READ success (decode-independent, tier-gated, TxtView/media triggers); core promotes only on decode success — damaged-but-readable images under-promote, text/video triggers don't exist, and ≤20-byte decodable images over-promote.
- Current code: `src/ui/cr_ui.cpp:4014-4029` — promotion strictly on the Ok path:
  ```cpp
  const preview::ThumbStatus st = preview::make_thumbnail(reader, ecaps, thumb);
  …
  if (st != preview::ThumbStatus::Ok) { … return ret_(thumb_status_to_abi(st), …); }
  /* M-083: the og's grading side-effect - thumbnail success promotes … */
  if (node->grade() != tree::Grade::High)
      node->set_grade(tree::Grade::High);
  ```
  and `:3956-3957` `case preview::ThumbStatus::DecodeFailed: return ERR_TRUNCATED;` — the return precedes the promotion. No `ThumbnailOutcome`/`read_ok` disclosure anywhere (`rg "ThumbnailOutcome|read_verdict" src/ include/ tests/` → 0); no ≤0x14 size gate in `thumbnail_generate_impl` (:3964-4039); `include/cr/preview/cr_thumbnail.h` still defers video key-frames and refuses-by-name text/WMF families (no text-snapshot trigger).
- Tests present? None — `CrUi.ThumbnailPromotesOnReadNotDecode` / `TinyFileDoesNotPromote` / `PreviewPaneNeverPromotes` → 0 hits.
- Notes: line drift (:3737-3742 → :4014-4029). One partial: the in-code og cite at :4025 now reads "FUN_1402636e0 :422507-514" — i.e. fix-plan step 6's comment-cite correction has been applied (the finding recorded the stale ":422498-518"). Cosmetic only; the trigger re-basing (the substance) is not implemented.

### M-118 — LIVE
- Finding's claim: the og assembles lost-partition candidates from three sources (mbr / filesystem / register); the register source — the Explorer `MountPoints2\CPC\Volume` history walk — has no core counterpart, so partitions whose on-disk remnants are destroyed but whose placement Windows remembers cannot be proposed.
- Current code: `rg -i "MountPoints|CPC.Volume|mountpoints" src/ include/ tests/` → 0. No `CandidateSource` seam or provenance field on `lost::Candidate` (`rg "CandidateSource|provenance" src/core/lost/` → 0); no `win::mountpoints2_volume_history()` (no `src/win/cr_mountpoints.cpp`). The reghive family remains FVE/offline-OS only.
- Tests present? None (`LostPart.MountPoints2WalkParsesPlacements` etc. — 0 hits).
- Notes: the table channel's vehicle (S-051) has since LANDED (`findings/FIXED/S-051-wholedisk-scan-targets-absent-silent-empty.md` exists; `cr_candidates.cpp:744/962-963` carries the `table_match` scoring) — consistent with the finding's own framing ("the table channel arrives via S-051's planned fix"). The register channel — this finding's defect — remains absent and unowned.

### M-119 — LIVE
- Finding's claim: the og's partition device clamps out-of-bounds reads and zero-fills the tail (read succeeds); the core's `PartitionSlice` hard-errors per-read and REFUSES the whole volume at creation when the table's declared length overruns the device.
- Current code: `src/core/storage/cr_slice.cpp` — unchanged strict shape:
  ```cpp
  :28  if (start_byte + length_bytes > parent->size()) {
  :30      *err = ERR_RANGE_OUT_OF_BOUNDS; return nullptr; }   // whole-slice refusal
  :46  if (offset > length_ || u64(length) > length_ - offset)
  :47      return ERR_RANGE_OUT_OF_BOUNDS;                      // per-read hard error
  ```
  CLI leg: `src/app/rrecov.cpp:3243-3247` `part_slice = cr::PartitionSlice::create(view.get(), part_base, u64(p.sector_count) * …, &prc); if (!part_slice) { wprintf(L"cannot slice partition %d (rc=%08x)\n", …); return 2; }` — the raw declared length, whole-scan abort. ABI legs: `src/ui/cr_ui.cpp:1381` and `:1466` (`PartitionSlice::create(whole.get(), start, len, nullptr)`). No clamp/zero-fill, no `declared_bytes` field (`include/cr/storage/cr_slice.h` read — no such member).
- Tests present? None (`Slice.CreateClampsOverlongDeclaredLength`, `Slice.ReadStraddleZeroFillsTail`, `Cli.ScanOverlongTableEntryScansVolume`, `CrUi.VolumeOpenClampedNotRefused`, `Apply.WriteBoundsStillHardRefused` — all 0 hits).
- Notes: line drift (rrecov :2973→:3243; cr_ui :1349/:1433→:1381/:1466; slice cites stable). The S42 addendum's "full ladder" (three refusals + clamp + error-path zero-fill) is likewise unimplemented.

### M-126 — LIVE
- Finding's claim: the og computes a remaining-time estimate on every progress message (whole-run base ETA, EMA 0.2/0.8, type-gated time-budget variant ×0.6, "3m00s" placeholder, 1 Hz countdown); the core has no ETA surface anywhere and O39's "UI-side formatting" disposition rests on a wrong premise.
- Current code: `rg -i "eta_seconds|remaining_time|smoothed_rate|rate_ema" src/ include/ tests/` → 0 (the EMA/semantics hits are unrelated comments). CLI prints elapsed only: `src/app/rrecov.cpp:1931` (`stats.elapsed_ms / 1000` … `u32(stats.elapsed_ms % 1000) / 100`). Telemetry inputs unchanged (`cr_scan.h` overall_permille/elapsed_ms). `diff-to-fix/modules/orchestration.md:51` still carries "O39 ETA — `M`-note: core exposes telemetry for ETA computation…" (with :7 pointing at M-126 as owner — tracked, not fixed).
- Tests present? None (`Eta.WholeRunModel` / `Eta.EmaDamping` / `Eta.TimeBudgetVariant` / `Eta.PlaceholderAndCountdownFormat` — 0 hits).
- Notes: part of the session-52 engine-math claim (M-126..M-136); nothing landed as of this pin (no ledger row, no code).

### M-127 — LIVE
- Finding's claim: the FAT/exFAT chain walkers push every cluster into a visited vector and linearly scan it per step (O(n²) up to ~2.1×10⁹ comparisons per maximal 65,536-cluster chain); the og uses a red-black range tree with run coalescing.
- Current code: `src/core/fat/cr_chain.cpp:429-447` —
  ```cpp
  std::vector<u64> visited; /* visited clusters (cycle guard, fix F7) */
  …
  for (u64 v : visited)
      if (v == c) { r.cycle = true; return r; }
  visited.push_back(c);
  ```
  and `reconstruct_deleted_chain` same shape at `:475/:486-491`. The exFAT twin: `src/core/exfat/cr_exfat_quick.cpp:94-107` (`std::vector<u64> visited;` … `for (u64 v : visited)` … `visited.push_back(c);`). No `VisitedRuns` helper anywhere; no run-coalescing structure. The 0x10000 caps and per-step cancel checks are intact (per fix-plan step 3, correctly untouched).
- Tests present? None (`FatChain.CycleGuardPerf` / `RunCoalescingDetectsOverlap` / `ExfatChain.CycleGuardPerf` — 0 hits).
- Notes: path drift — the finding cites the exFAT twin as `src/core/fat/cr_exfat_quick.cpp:91-133`; in the current tree the file lives at `src/core/exfat/cr_exfat_quick.cpp` (same content shape at :91-107). Session-52 claim; nothing landed.

### M-128 — LIVE
- Finding's claim: the og matches user signs as UPPERCASE-hex text memcmp (`%02X` only), making its own 12 shipped lowercase-hex signs dead; the core decodes hex case-insensitively and compares bytes, so all 190 signs are live — an unregistered superset.
- Current code: `src/core/carve/cr_usersigns.cpp:77-91` —
  ```cpp
  bool hex_nibble(char c, u8& v) {
      … if (c >= 'a' && c <= 'f') { v = u8(c - 'a' + 10); return true; }
      … if (c >= 'A' && c <= 'F') { … } …
  ```
  and `parse_sign` `:107-114` decodes to bytes (`us.bytes[i/2] = u8((hi << 4) | lo);`) — byte comparison, both cases accepted. No uppercase normalization at load; no registration: `rg "case-insensit|lowercase|%02X|dead sign" docs/PARITY-DELTAS.md` → only an unrelated FAT row (:1562).
- Tests present? None (`UserSigns.LowercaseHexSignDeadIfOgFaithful` / `UppercaseHexSignMatches` / `DefaultSetEffectiveCount` — 0 hits; the nearest existing tests are N-179's `UserSigns.ManifestEightCharExtParses` and N-168's `ShortFragmentMatchesInCore`, both different defects).
- Notes: the product decision (option a vs b) is still unmade — no PARITY-DELTAS D-104 addendum.

### M-129 — LIVE
- Finding's claim: the og's EUUserData loader is tolerant (unknown members ignored via `_stricmp`-dispatch on "signs", malformed signs skipped, odd hex kept-but-dead); the core's reader is all-or-nothing — any quirk rejects the whole file.
- Current code: `src/core/carve/cr_usersigns.cpp` — the strict reject sites all stand: `:174` `if (std::strcmp(key, "signs") == 0)` (exact-case, not stricmp), `:110` `if (n < 2 || n > kMaxSignBytes * 2 || (n & 1) != 0) return false;` (odd hex → whole-file reject via parse_sign), `:207` `return ERR_BAD_PARAMETER; /* unknown key: strict, the og's shape only */`, `:221` `return ERR_BAD_PARAMETER; /* trailing garbage: honest rejection */`; empty-signs and duplicate-mime rejects also present in the same reader.
- Tests present? None (`UserSigns.UnknownMemberIgnored` / `MemberNamesCaseInsensitive` / `MalformedSignSkippedNotFatal` / `OddHexSkipped` / `TornFileStillRejected` / `ShippedOgFileVerbatimLoads` — 0 hits).
- Notes: the comment at :207 still asserts "the og's shape only", which this finding disproved — a stale in-code claim riding the unfixed defect.

### M-130 — LIVE
- Finding's claim: the og runs every candidate FAT dentry through a shape validator (ext-tail >'?' gate, size/chain XOR gate, dir gate, >9-repeat byte histogram over +0x0B..+0x1F, walk-stop semantics); the core ports none of the four gates — only the config-gated date sub-gate.
- Current code: `rg -i "dentry_shape_ok|histogram|ext.tail|shape_valid" src/core/fat/ include/cr/fat/` → 0. The only validator remnant is N-010c's date gate: `src/core/fat/cr_fat_quick.cpp:234` `if (ctx.flags.strict_date_sanity) {` with `include/cr/fat/cr_fat_engine.h:257` `bool strict_date_sanity = false;` (default OFF, exactly as the finding describes). No walk-stop-on-validation semantics.
- Tests present? None (`Fat.ExtTailByteBeyondQuestionmarkRejected` / `SizeChainXorRejected` / `DirSizeAndHighClusterRejected` / `HistogramTenRepeatsRejected` / `ValidatorFailureStopsWalkAtGatedSites` / `VolumeLabelRejected` — 0 hits).
- Notes: none.

### M-132 — LIVE
- Finding's claim: two asymmetries — (A) the og ALWAYS probes the GPT backup at device-last while the core probes the primary's declared `backup_lba` when the primary verifies; (B) on an unreadable LBA1 the og bails with no entries while the core recovers from device-last (unregistered superset).
- Current code: `src/core/table/cr_table_parse.cpp:380-418` —
  ```cpp
  :385 /* informational backup probe at the TRUSTED primary's own
  :386  * backup LBA (healthy-disk case: both flags true) */
  :387 if (load_gpt_side(device, prim.backup_lba, true, bak, bent) == OK)
  :388     out.gpt_backup_ok = true;
  …
  :407 if (load_gpt_side(device, dev_last, true, bak, bent) == OK) {
  :408     out.gpt_backup_ok = true; … emit_gpt_entries(bent); …
  ```
  — the primary-OK path still probes the DECLARED `backup_lba` with no device-last-first fallback (Case A defect intact), and the dead-primary path still emits the backup's entries at dev_last (Case B superset intact, still unregistered). No probe-order change; the fix plan's device-first order not implemented.
- Tests present? None (`Gpt.BackupProbedAtDeviceLastWhenPrimaryOk` / `CorruptBackupLbaFieldStillFindsBackup` / `DeadLba1BackupRecoveryRegistered` — 0 hits).
- Notes: line drift (:323/:339-348 → :385-388/:403-418). M-123's stricter header validation (FIXED, `load_gpt_side` gates at :41-71) landed around this site but did not touch the probe position. Session-52 claim set; nothing landed for M-132 itself.

### M-133 — LIVE
- Finding's claim: both sides fold a per-device ordinal into the twin address key with a −2 shift, but for different families — the og shifts kinds {FAT16, EXT2-family code 4}; the core shifts {FAT12, FAT16, FAT32, exFAT} and leaves the ext family unshifted, so twin/overlap collisions fire on the wrong device pairs.
- Current code: `src/core/tree/cr_grading.cpp:130-142` —
  ```cpp
  const u32 fs = ctx.fs_kind_of(n);
  const bool fat_family = fs == u32(FsKindCode::Fat12) || fs == u32(FsKindCode::Fat16) ||
                          fs == u32(FsKindCode::Fat32) || fs == u32(FsKindCode::Exfat);
  const i64 index = i64(ctx.disk_ordinal_of(n)) - (fat_family ? 2 : 0);
  ```
  — identical to the finding's quote: all four FAT kinds shifted, ext family not.
- Tests present? None (`Grading.TwinKeyShiftsFat16AndExt2FamilyOnly` / `TwinKeyFat32DoesNotShift` / `TwinKeyExfatDoesNotShift` — 0 hits).
- Notes: **ID-collision hazard**: `findings/FIXED/M-133-suite-red-at-head-convergent-n157-m120.md` + its FIXED-LEDGER row are a DIFFERENT finding (a carve/tests/build-gate suite-red discovery renumbered into M-133), NOT this twin-key finding's retirement. This M-133 (twin-key) is correctly still ACTIVE. Session-52 claim set; nothing landed.

### M-134 — LIVE
- Finding's claim: the og's grading contains a deep-read rule (zeroed/0xFF first sector → grade 0; 16-zero-byte previous-sector tail → grade 2) and a full-tree per-selection re-grade; the core re-runs solo rules only and its grading pass cannot read the disk at all.
- Current code: `rg "GradeReader|deep_read" src/ include/ tests/` → 0. `src/core/tree/cr_grading.cpp:338-346` —
  ```cpp
  bool regrade_node(tree::FileNode* n, const GradeContext& ctx) {
      … if (n->grade() != Grade::Average) return false;
      solo_rules(n, ctx, nullptr); return true; }
  ```
  (solo-only, no table gate, no twin map, no graded-bit). The header still registers the non-adoption posture: `include/cr/tree/cr_grading.h:27` "…are UNDECODED (see Resolution notes)" and `cr_grading.cpp:206` "(vt+0xb8 / vt+0x100 sub-rules: UNDECODED - not adopted.)".
- Tests present? None (`Grading.DeepReadZeroedFirstSectorGradesLow` / `DeepRead0xFFHeadGradesLow` / `DeepReadPrevSectorTailZeroGradesHigh` / `RegradeRunsFullTreeOnce` / `DeepReadSkipsOverlapFlagged` — 0 hits).
- Notes: the fix-time sub-gate questions remain parked in `diff-to-fix/tracked-open/OQ-S42-3-grading-deep-read-subgates.md` (present at the pin). Session-52 claim set; nothing landed.

### M-135 — LIVE
- Finding's claim: on a mid-file read failure the og logs a three-parameter error, records error 9, breaks, and KEEPS the partial file with a numbered six-cause taxonomy and a runtime node mark; the core zero-fills the span, continues, commits Salvaged, and deletes failed items' temp output wholesale.
- Current code: `src/core/recovery/cr_safewriter.cpp` — read-failure path (compressed-extent leg quoted; the plain leg mirrors at :426-427): `:469-471`
  ```cpp
  std::memset(packed.data(), 0, size_t(psize)); /* Skip policy */
  std::memset(plain.data(), 0, size_t(lsize));
  it.had_unreadable = true; /* FB-R02: zero-filled span */
  ```
  (zero-fill + continue — no break). Salvaged commit semantics intact (:752 region "Salvaged (never silently Done)"; outcome enum at `include/cr/recovery/cr_report.h:26`). Whole-item failures still leave no file: `include/cr/win/cr_destio.h:36-43` "close() deletes the temp on every !renamed exit". No per-file cause taxonomy in `cr_report.h` (grep cause_id → 0); no "Failed to ReadSector" log family anywhere (`rg "ReadSector" src/ include/` → 0).
- Tests present? None (`Writer.HardReadFailureTruncatesAndKeeps` / `FailureCarriesNumberedCause` / `ReadSectorLogTriFormat` / `ReportPartialBytesWritten` — 0 hits).
- Notes: line drift (:498-503 → :426-427/:469-471). Session-52 claim set; nothing landed.

### M-136 — LIVE
- Finding's claim: the scan reads through SliceShim→RetryingDevice(3 attempts, Skip)→CachedReader, but the recovery pass hands the SafeWriter the bare device — one transient read error becomes a permanent zero-filled span of the user's recovered file.
- Current code: `src/app/rrecov.cpp:2323` `recovery::SafeWriter writer(io, src.device.get(), wcfg);` — the raw `IStorageDevice*`. ABI: `src/ui/cr_ui.cpp:4118` `r->writer.reset(new recovery::SafeWriter(*r->io, src, wcfg));` where `src` is `d->dev.get()`/`d->hold.get()` (the raw parent disk, F-1 basis) — no retry/cache wrapper. The pipe remains scan-only: `src/core/orchestrator/cr_scan.cpp:162` `ReadPipe build_read_pipe(IStorageDevice* slice, u64 cache_bytes, u32 block_bytes)` (SliceShim→RetryingDevice→CachedReader, :181-190), constructed for scans at :1152.
- Tests present? None (`Writer.ReadsRetryOnTransientFailure` / `Writer.CachedReaderSharedWithScan` — 0 hits; no fault-injection mock for first-attempt-fails).
- Notes: line drift (rrecov :2326→:2323; cr_ui :4027→:4118 — the M-116/archive landings shifted the ABI file). Session-52 claim set; nothing landed.

### M-137 — LIVE
- Finding's claim: the og surfaces lost partitions with a "Lost Partition %d(%s)" naming template, an "Unallocated" relabel, a 10-target insertion cap, per-candidate "Quick" registration, and a 512-byte geometry fallback; the core names roots `Candidate @ <LBA>`, has no cap, no staging, and refuses on zero geometry.
- Current code: `src/core/orchestrator/cr_scan.cpp:2054-2058` —
  ```cpp
  std::u16string role = u"Candidate @";
  char buf[32];
  snprintf(buf, sizeof(buf), " %llu", (unsigned long long)c.start_lba);
  for (const char* p = buf; *p; ++p) role.push_back(u16(u8(*p)));
  ```
  — the template unchanged; no "Lost Partition" string anywhere in `src/` (grep → 0). Geometry refusal intact: `src/core/lost/cr_candidates.cpp:759-760` `if (disk == nullptr || disk->geometry().logical_sector == 0) return ERR_BAD_PARAMETER;`. No insertion cap / Quick staging in the candidate pass (`rg "insert|cap|Quick" cr_scan.cpp` → no lost-candidate cap; the always-deep candidate semantics note stands at :1929 region "pass now with full deep semantics").
- Tests present? None (`LostPart.UnnamedCandidateNamedLostPartitionN` / `InsertionCapTenTargets` / `ZeroSectorSizeFallsBackTo512` / `CandidateListingCarriesLayoutFields` — 0 hits; `tests/unit/test_orchestrator.cpp:466/489` actively assert the ABSENCE of duplicate `Candidate @` roots — pins of the current shape, not the og's).
- Notes: line drift (:2052-2058 held). Session-52 claim set; nothing landed.

## Uncertainties

1. **M-086 / M-087 / M-089 (driver-repo-resident):** the entire defect surfaces live in `heicfast/rda-driver`, which is not cloned in this workspace (`/home/z/my-project/repos/` holds only `core_recovery` and `easeus-drw-research`). First-hand liveness of `rda_version.h`'s 1.0.0.0 pin, `regdrv-rda.reg:31`'s hex(7) bytes, and `rdabench.c:390`'s probe list could not be re-verified at the pin. Substitutes: (a) the push-forward bookkeeping is intact and open (`tracked-open/OQ-S10-3`, read this session — both the original record and the session-40 re-affirmation); (b) M-089's defect CLASS was verified alive inside this repo at `bench/ours/verify-driver.ps1:47,55` (phantom `0x60002440`/`0x60002448` vs the real codes in `cr_rda.h:55-57`) — a core-side residue the finding never cited. If a future lane has the driver repo, re-run: `git -C rda-driver log -p -- src/driver/rda_version.h`, `od -A d -t x1z scripts/winpe/regdrv-rda.reg`, and grep `0x6000244` in `src/bench/rdabench.c`.
2. **Freshness (per the mandate):** a parallel FIX lane (session-52) has claimed the engine-math set M-126..M-136 and is actively landing. At pin `2794da8b` none of M-126/M-127/M-128/M-129/M-130/M-132/M-133/M-134/M-135/M-136/M-137 shows any code or ledger trace of a fix — all verify LIVE. These eleven verdicts are the most likely to go stale within hours; they are the prime re-verification (retirement-candidate) targets for the next battery.
3. **Renumber-collision hazard (M-133):** `findings/FIXED/M-133-suite-red-at-head-convergent-n157-m120.md` is a different finding that consumed the M-133 ID after renumbering. Any automated retirement sweep matching on the bare ID would wrongly retire the ACTIVE twin-key M-133. Match on title/slug, not ID.
4. Execution-dependent aspects (bad-sector fixtures, Windows SCM/drill lanes, PE boot transcripts) were verified statically only — consistent with each finding's own UNVERIFIED-BY-EXECUTION markings; no execution environment is available in this container (Linux).
