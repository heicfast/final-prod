# PLAN-API-EXPANSION — close the reference-documented API gaps

<!-- session-96: the driver era ended - the kernel driver was replaced by direct raw I/O; the driver-era rows below are historical records. -->

> Session: driver-only mandate §3. Worklist source: [REFERENCE-API-GAP.md](REFERENCE-API-GAP.md)
> (88 corpus-documented operations; 41 PRESENT / 15 PARTIAL / 29 GAP, ~14 flagged product-layer).
> Rule of engagement, verbatim from the mandate: **no operation is added that the reference does not
> document**; every addition cites the corpus row; product-scope additions are registered as divergences;
> deferred items get precise justification in PARITY-DELTAS.md — never silence.

---

## 0. Scope decisions (evidenced, not assumed)

1. **Driver-status surface** — mandated by the session brief; the corpus is SILENT on any og UI
   driver-status/version query (REFERENCE-API-GAP R-84 + §d corpus-silence list; the og-access-architecture record §3, worklog/reference/:
   the og exposes no driver-status surface — service-start result literally discarded). It is implemented
   as a **product-scope truthfulness divergence** (shape per the driver-era removal plan §3, deleted with the driver), NOT as parity. This is the
   single sanctioned non-corpus addition; everything else below is corpus-driven.
2. **C ABI surface** — the merged UI consumes the core via the cr_ui C ABI today (cr_ui.h:5-22 documents the
   lineage: the og binds its UI in-process through C-export factories; our equivalent is the flat-C shim).
   The C++ surface and the C ABI therefore move in lockstep: every operation below lands in
   `include/cr/**` (C++ contract) AND `cr_ui.h` (C ABI export) AND — where operability demands it — the CLI.
   ABI bumps v2 → v3 (additive).
3. **CLI exposure rule** — the reference has no CLI (GUI product; cr_ui.h:12-13). CLI verbs are added only
   where (i) the operation is essential to core operability (driver, sessions) or (ii) an existing verb
   naturally absorbs the facet (flag/subcommand on scan/health/fix). Nothing ad-hoc.
4. **Session pause/resume** — corpus-silent (§d list) and ALREADY ours (cr_ui_scan_pause/_resume,
   test_pause_resume.cpp): kept, registered as an ours-superset delta (no change).

## 1. The worklist — every in-scope operation, its shape, its tests

Grouped by landing order. Corpus row IDs (R-##) are REFERENCE-API-GAP.md's. "ABI" = new cr_ui export;
"CLI" = rrecov surface. Threading/cancellation contracts inherit the cr_ui house rules (cr_ui.h ABI RULES:
C11, same numeric error space, zero-init-safe structs, no escaping exceptions).

### Wave 1 — session surface completion (SaveScan parity)

| Op | Corpus | Shape | Tests |
|---|---|---|---|
| **session list + summary** (R-56 PARTIAL) | SaveScan:11; UIUX/07:21 (LastScanRecordView + Continue-last banner) | C++ `load_session_summary` already exists unexported → `cr_ui_session_list(dir, **out_rows)` returning per-file summary (path, volume identity, timestamp, counts, stage mask); CLI verb `sessions [--dir D]` (single-word grammar; `--json` supported like `list`). | unit: summary rows from a saved CRSF fixture; CLI golden output; ABI two-call size pattern |
| **load progress + cancel** (R-54 PARTIAL) | SaveScan:344-376 (save/load progress 40-65% + per-child cancel) | **LANDED (W1b, ce6bcb0) — the shipped shape differs from this plan row:** `cr_ui_session_load_with_events(path, fn, user, volatile uint32_t* cancel_flag, out)` — the cancel is a polled flag argument on the events variant (mirroring the save-side cancel seam), NOT the `cr_ui_session_load_cancel(h)` export this plan sketched; `CR_EVT_SESSION_LOAD_PROGRESS` fires with stage permille + byte fields. | unit: progress events fire during load of a large fixture; cancel mid-load aborts cleanly, no partial claims |
| **auto-save-on-completion** (R-52 PARTIAL) | SaveScan:443-451 (auto_<vol>_<ts>.rsf on completion); UIUX/30:64-67 | `ScanConfig.autosave_dir` (empty = off, zero-init safe); on scan completion the controller writes `auto_<vol-id>_<timestamp>.crsf` via the existing save path; surfaced as an event + stats field. CLI: `scan --autosave-dir D`. | unit: completion triggers exactly one auto-save; naming pattern pinned; off by default |

### Wave 2 — query/filter surface completion

| Op | Corpus | Shape | Tests |
|---|---|---|---|
| **filter axes: path + state** (R-22 PARTIAL) | UIUX/31:55-125 (AdvanceFilterCondition 7 groups: path/state/tag among them) | `Query` filter gains `path_substring` (UTF-8) and `state_mask` (Existing/Deleted/Lost/Carved) axes; `cr_ui_query_run` passes them through; `tree --filter` grammar extended with the same axes. Tag axis: our raw_category is the carve tag surface — exposed as the tag axis (corpus "Tags Extracted" grouping, R-20 PARTIAL, is satisfied by the same field in grouping mode). | unit: each axis filters the snapshot fixture correctly; combined-axes test; CLI golden |
| **scan-time folder filter** (R-14 GAP) | UIUX/31:140-143 (PushFolderFilter) | `ScanConfig.filter_folders` (list of UTF-8 path prefixes; empty = no filter); the orchestrator prunes tree construction outside the prefixes (engines still read metadata — the og's filter is a results filter riding the scan, and ours matches that: post-parse prune before node insertion). CLI: `scan --filter-folder P` (repeatable). | unit: seeded tree with out-of-prefix nodes → absent from snapshot; session round-trip preserves the filter |

### Wave 3 — health / enumeration / destination polish

| Op | Corpus | Shape | Tests |
|---|---|---|---|
| **destination validation codes** (R-41 PARTIAL) | arch/04:136-141 | `ERR_DEST_PATH_TOO_LONG` (0x50050006), `ERR_DEST_PATH_INVALID` (0x50050007); network-path branch returns the refusal code with guidance (og: PathIsNetworkPathW never armed — cr_transport.cpp:993-995 today returns false; the writer surface now classifies it for the caller). | unit: >MAX_PATH destination, malformed path, UNC path → specific codes |
| **chkdsk launch** (R-65 PARTIAL → **LANDED M-075, 2026-10-30**) | mom-module-system:65 (FileSystemCheck.mo + WinChkdsk) | `cr_ui_fsck_volume(letter, flags, **exit_code)` launching `chkdsk.exe <vol> /f` (elevated, synchronous with timeout); CLI `fix --chkdsk C:`. Refuses (not silently skips) when the volume cannot be checked. | LANDED: FsckVolume.* unit battery (test_fscheck_launch.cpp, both lanes) + FsckWin.* seam battery (test_fsck_win.cpp, Windows lane) + the CI-SCENARIOS row |
| **enumeration refresh export** (R-86 PARTIAL) | UIUX/13:14 | `cr_ui_enumerate_disks/_volumes` documented as always-fresh (each call re-queries — implementation verified; if any caching exists it is removed); header contract text states it. No new verb: `list` IS the refresh (og's refresh re-runs the same enumeration). | unit: two consecutive enumerate calls reflect an intervening fixture change (where the lane allows) |
| **TRIM/SSD facet in enumeration rows** (R-63 PARTIAL) | UIUX/07:17 (TRIM chip per row) | `cr_ui_disk_info` gains `trim_known`/`trim_on` filled from the existing health TRIM query when cheap (cache-inhibited: only when the caller opts in via a flag — enumeration latency is the og's own concern; the og fills it from DiskHealth's periodic data). CLI `list --trim`. DEFERRED-if-slow decision recorded at implementation with the measurement. | unit: flag off = unknown; flag on = queried (stub lane pins the field plumbing) |

### Wave 4 — preview / image surface completion

| Op | Corpus | Shape | Tests |
|---|---|---|---|
| **preview caps shipped defaults** (R-34 PARTIAL) | UIUX/10:22-27 (100 MB / 15 s caps) | The existing caps seam gets shipped defaults: 100 MiB max bytes, 15 s max decode — the corpus numbers; config-overridable, zero-init = defaults ON (the og's caps are product policy, not user opt-in). | unit: oversized preview request is truncated + flagged in metadata; slow-decode guard pinned via the seam |
| **image create** (R-93 PARTIAL) | UIUX/07:36-37 (disk → image) | CLI `image --create --disk N --out file.img` + `cr_ui_image_create(disk, path_utf8, progress_fn)`: streams the driver-required device to a flat `.img` via the destination-I/O layer (64 KiB envelope reads, safewriter-style progress/cancel, SHA256 sidecar). Raw-flat only — vhd/vhdx WRITERS are out of scope (documented; the og's create ships flat images to its own reader). Reads go through the driver (classification (a), the driver-era plan §1 — historical; session-96 replaced the driver with direct raw I/O). | Windows scenario: create image of the staged disk → hash equals source device hash; resume-after-cancel unit test |

### Wave 5 — BitLocker tail

| Op | Corpus | Shape | Tests |
|---|---|---|---|
| **BitLocker turn-off** (R-88 tail) | DRWService:50-89 (unlock chain incl. turn-off) | `cr_ui_turn_off_bitlocker(letter, **progress)` via `manage-bde -off <vol>:` (the same PS/manage-bde duality as unlock, FB-B02 evidence class); CLI `unlock --turn-off C:`. Destructive-long-running: explicit opt-in only, progress surfaced, refusal on failure — never silent. | unit: command construction + rc mapping; Windows scenario optional (BitLocker provisioning on runners is documented as out-of-reach → PARITY-DELTAS notes the untested-on-CI caveat honestly) |

## 2. The deferred list — precise justifications (PARITY-DELTAS.md rows, verbatim intent)

| Op (R-##) | Corpus | Why deferred (precise, not vague) |
|---|---|---|
| ACL preservation (R-48) | pending-op:45-51 | requires an NTFS `$Secure` parser (security-ID → descriptor stream) that the engines do not have; a fake (copy-from-destination) would be a silent-degradation counterfeit — the exact class this session eliminates. Deferred WITH design notes: parse $SDH/$SII during scan, store per-node security_id, SetNamedSecurityInfoW at write time. |
| RAID-5 rebuild (R-71) | pending-op:64-65 | reconstruction engine = new analysis surface (stripe geometry, parity math, degraded reads); no shared substrate with the FS engines. Product-scope decision required (mission: recovery of files, not array rebuild). |
| EPM partition-editing vocabulary (R-68) | pending-op:30-43 | corpus itself frames it as the partition-EDITOR product, adjacent to but outside the recovery mission; our apply/undo transactional surface covers the recovery-relevant subset (R-67 PRESENT). |
| Fixo file-content repair (R-72..78) | UIUX/12:22-55 | a separate engine family (video/PDF/office codec repair) shipped in the og as separate binaries (EUFileRepair/VideoViewer dirs); not the recovery core. Product-scope. |
| archive browse-inside preview (R-37) | UIUX/10:17 | requires zip/rar/7z enumeration parsers (new dependency family); preview of archive FILES already works (R-32/33 PRESENT). |
| thumbnails async protocol (R-28) | UIUX/13:50 | the og's thumbnail layer is a UI-side image pipeline over the same preview reads; the host owns async (cr_ui is already free-threaded per ABI rules). Exposing an invented async protocol = richer-than-documented. **The engine itself landed (05e6786, `cr_thumbnail` — see modules/preview.md); the cr_ui ABI exposure is the open Agent-1 handoff (M-102), not this row's protocol question.** |
| AI recommend/dedup (R-29), quota/license/update/telemetry/crash-report (R-02/45/50/57/59/80/81/82) | various | product/monetization layer — no engine contract in the corpus beyond existence; adding them would be invention. |
| hot-plug notify (R-87) | pending-op:43 | RegisterDeviceNotification is a windowed-process concern; the core's contract is pull enumeration (R-86). Host responsibility. |
| NAS/Linux/cloud/WeChat (R-89/90/91/94), WinPE media creation (R-92) | various | remote transports + product scenarios; corpus documents them as product directions, not engine APIs. |
| service lifecycle in core (R-83) | DRWService:9-30 | the og's DRWService is its product service; our driver's install lifecycle lived in rda-driver's install.ps1/uninstall.ps1 (the driver-era release contract §5) and the product installer's then-driver step (both historical — session-96 removed the driver). Core-side duplication would fork the install path. |
| media-helper IPC / external viewers (R-38/39) | DRWUI arch:291-313 | UI-shim concerns (helper processes are host-owned). |
| Versions node (R-20 part) | UIUX/09:27-36 | VSS previous-versions enumeration is a Windows-shell surface (\\?\shadow copies); requires VSS integration outside the scan engine. Tags-Extracted grouping IS delivered via the tag filter axis (Wave 2). |
| periodic health monitor loop (R-61) | DiskHealth:20-28 | tray/notify/loop is host UI; the pull API (health_read) is the core contract. |
| quick→advanced gate verb (R-06) | UIUX/25:69-78 | reachable today by composition (stage events + mid-scan snapshot + stage_mask); a dedicated gate verb would be API surface without new capability. Registered as present-by-composition. |

## 3. Cross-cutting contracts every wave obeys

- **Driver precondition:** Waves 4's image-create and Wave 5's turn-off touch volumes/disks: image-create is
  class (a) (privilege check first, the driver-era plan §5); turn-off/chkdsk/list/health are class (b) (documented
  in the deleted driver-era refusal contract's table — no driver check, the og evidence says these never used it).
- **Error space:** new codes registered in cr_error.h with err_text; no numeric reuse; the ABI returns the
  same numeric space (cr_ui.h:26-27).
- **Contract text:** every new header symbol ships with full contract text (what/inputs/outputs/errors/
  threading/lifecycle) — the cr_ui.h house style (cr_ui.h:23-40 ABI RULES block).
- **Tests before green:** each wave's push includes unit tests (Linux lane) and — where the surface is
  disk-truth — a Windows scenario row in CI-SCENARIOS.md; no wave lands without its tests.

## 4. Acceptance criteria

1. Every Wave-1..5 operation present in C++ + ABI + (where ruled) CLI, with contract text and tests.
2. REFERENCE-API-GAP.md's status column re-audited at close: every PARTIAL row is either PRESENT or
   moved to the §2 deferred table with its justification; every in-core GAP row likewise. Zero silent rows.
3. PARITY-DELTAS.md carries: the driver-status divergence, the deferred table above, and the ours-superset
   notes (pause/resume, hash verification, structured report).
4. ABI v3 exported-symbol list verified by tests/ui_abi_check.c on both lanes.
5. Full unit suite + MSVC /W4 /WX + the staged Windows scenarios green with the new surfaces exercised.
