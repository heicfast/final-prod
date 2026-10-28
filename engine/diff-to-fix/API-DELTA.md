# API INVENTORY / DELTA — core_recovery vs the research corpus (API scope)

> Session: api-fix (2026-10-28). Method per the context-recovery protocol:
> three parallel read-legs (corpus CTX-1, core CTX-2, driver CTX-3) produced
> `/home/z/my-project/api-ctx/{corpus-api-context,core-api-surface,driver-api-surface}.md`;
> this report consolidates them into the present|missing|inconsistent|hallucinated
> board and is then executed item-by-item. Corpus commit `03ad46c`; core at
> `d1fba5d` → synced through `025dee7` (parallel lanes landing concurrently).

## 0. Surfaces in scope (API layer only)

1. **cr_ui C ABI** — `include/cr/cr_ui.h` (79 exports, `CR_UI_ABI_VERSION 3`
   after the M-067 landing) + `src/ui/cr_ui.cpp`.
2. **rrecov CLI** — 10 verbs, exit codes, machine-readable JSON shapes.
3. **Session/report formats** — CRSF v1 (`docs/crsf-format.md`),
   recovery-report JSON v1 (`docs/recovery-report-schema.md`).
4. **Driver transport contract** — `include/cr/win/cr_rda.h`,
   `src/win/cr_transport.cpp`, `docs/DRIVER-CONTRACT.md`,
   `docs/DRIVER-REQUIRED.md` (user-mode half of the rda-driver wire).
5. **Normative API docs** — `docs/ABI-CONTRACT.md`, `docs/API.md`,
   `docs/API-SURFACE-AUDIT.md`, `docs/CORE-API-CURRENT.md` (claims, not truth).

Out of scope: engine internals, driver internals, packaging/deployer scripts
(except where a doc's claim about an API is falsifiable), UI.

## 1. Board — every corpus API surface vs core status

Legend: MATCH = contract-equivalent | INCONSISTENT = present but drifting
(doc vs code vs corpus) | MISSING = no counterpart | HALLUCINATED = claimed
but not real (invented) | SUPERSET = documented deliberate divergence.

### 1.1 Engine factories & scan lifecycle

| # | Corpus surface | Core equivalent | Status | Evidence |
|---|---|---|---|---|
| 1 | `EUCreate<FS>{Quick,}Searcher` pair × 9 DLLs; FS-code dispatch 0-2/3/4-5/7/8/9/10/13 (+Btrfs); Btrfs "Complate" typo | `cr_ui_scan_create` `stage_mask`/`fs_hint` (1/2/3/4/6/7/8/9/11 valid) | **MATCH** (binding-model equivalence documented) | corpus-api-context §1.1; core-api-surface b.4 |
| 2 | RAWSearchDLL extra exports: `EUGetAnalyserList/Map/Multimap`, `GetRawUserInfoMap`, `EUCreateRAWSearcherForCellTest` | `cr_ui_carve_format_count/_info` (both layers, append-only) | **MATCH** (enumeration ops #7/#8 landed prior session); `GetRawUserInfoMap` = per-scan carve-statistics map (OQ-SA-3 RESOLVED) — telemetry only, no gap | driver-agnostic; corpus-api-context §2.2 item 11 |
| 3 | `RegisterCarvedFileNamer` (RAW vt+0x48, UI-side naming policy) | not exposed; names synthesized; host can rename post-hoc via node_info | **MATCH (registered divergence D-API-1)** | API-SURFACE-AUDIT §2.1 #5 |
| 4 | `SetScanMode(int)` | `whole_disk` + stage weights in scan config | **MATCH** | audit #6 |
| 5 | Engine pause/resume interface | **NOT IN CORPUS** (UI-level only) | **MATCH+ (documented superset D-18)** | corpus-api-context §6 Q1 |
| 6 | Event bus `(int msg, void* data, int&)` MOM/Qt | `cr_ui_scan_set_event_fn` + 10 typed events | **MATCH+** (typed superset, ABI-CONTRACT §6) | audit #15 |
| 7 | Progress: pull-polled + sigPercentChange; emission throttle NOT IN CORPUS | push `cr_ui_progress_fn` + pull `cr_ui_scan_telemetry`, permille-CAS throttle | **MATCH+** | audit #16 |
| 8 | StopScan (manual-reset event + reason) | `cr_ui_scan_cancel` (PausableToken, tree intact) | **MATCH** | audit #13 |

### 1.2 Node metadata, query, filter

| # | Corpus surface | Core equivalent | Status | Evidence |
|---|---|---|---|---|
| 9 | IFileProperties 23-slot vtable; 10 persisted type codes; FLAGS 0x10/0x20; DataRunList | `cr_ui_node_info` (name/sizes/4 ts/attrs/extents/deleted/origin/confidence/carve format/hash) | **MATCH** (field set ⊇ persisted set) | audit #18 |
| 10 | FileFilter.xml: 8 categories (Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks), 1,489 Extention elements / 1,481 unique | 6 hard-coded categories / ~90 ext (M-023 engine scope) | **MISSING (engine-scope taxonomy, tracked M-023; the audit's row is stale on counts: "1,481" = unique, 1,489 = elements)** | corpus-api-context §2.1 #19 (mechanically verified) |
| 11 | AdvanceFilterCondition 7 groups (type/time/size/state/tag/path/search); date buckets 0x100-0x103/0x105, size 0x1000-0x1004/0x1006 | `cr_ui_query` + `cr_ui_query_apply_buckets` (og bucket IDs exact; skipped/unknown rejected) | **MATCH on 6/7** (tag NOT-EXPOSED, UI state) | core-api-surface b.7; corpus-api-context §1.11 |
| 12 | Search dropdown "Optional Type" 8 classes | `name_substring` | **MATCH** | audit #21 |
| 13 | Sort (Name/Size/Type/Date + recovery-chance) | query sort 0..7 (7=Type) | **MATCH** | audit #22 |
| 14 | Tree branches: Tags Extracted / File Name Lost / File Path Lost / Versions / Unimportant | origin/confidence grading + roots | **PARTIAL — engine taxonomy scope (M-023 lineage)** | corpus-api-context §2.2 item 16 |

### 1.3 Preview, recovery, session

| # | Corpus surface | Core equivalent | Status | Evidence |
|---|---|---|---|---|
| 15 | Preview open per-type; **IInWindowStream = LZMA SDK name, NOT a preview contract** (M-028 falsified the audit's attribution); caps 100 MB/15 s = UI policy; 1 MiB payload gate | `cr_ui_preview_*` family, 4 MiB/call cap, short only at EOF | **MATCH + doc tail FIXED this session** (6d0f16c landed the enumeration; N-088 tail swept) | corpus-api-context §1.9/§2.1 #26 |
| 16 | ArchivePreviewModel browse-inside-damaged-zip | preview ArchiveEntry inventory (512 cap + truncated flag) | **MATCH — landed (6d0f16c, M-066)** | M-066 finding → FIXED |
| 17 | Media thumbnail IPC TCP 20171/20172 + zmq | n/a — UI-process internals | **N/A-by-layer** | corpus-api-context §1.9 |
| 18 | Recovery: `\\.\X:` same-volume block; 0x5c→0x2f path normalization; ACL preservation; overwrite policy NOT IN CORPUS | `cr_ui_recovery_*`; `ERR_DEST_OVERLAP`; conflict modes rename/skip/overwrite; ACL = N-009d tracked | **MATCH+** | audit #28/#29 |
| 19 | .rsf container (16-B magic, GUID-keyed records 1-4, chunked zlib lvl 5, fallback memcpy) | CRSF v1 (8 record types, chunked zlib, CRC32, skip-forward) | **MATCH (container semantics equivalent; G-09/F10 supersets)** | audit #32/#33 |
| 20 | Auto-save orchestration (Freeze→save→Unfreeze) | host-side/CLI save | **N/A-by-layer** | audit #34 |
| 21 | Recovery log FILE format | JSON report schema v1 | **MATCH (clean-room equivalent; og format NOT IN CORPUS)** | audit #37 |

### 1.4 Lost partitions, health, disk map

| # | Corpus surface | Core equivalent | Status | Evidence |
|---|---|---|---|---|
| 22 | Lost-partition: insert-as-scan-target; `CPartitionVirtualVolumeDevice`; partition engine modules | `whole_disk` + `CR_EVT_LOST_CANDIDATE` + lostpart engine + `cr_ui_apply_lost_partition/_undo_last/_save_undo` | **MATCH (API surface complete; scoring parity M-017 engine scope)** | audit #35 |
| 23 | `IBuildVolumeBitmap` | `cr_ui_disk_map` (4 overlays) | **MATCH+ (GAP-S6-11)** | audit §2.5 |
| 24 | DiskHealth companion app (SMART SAT/CSMI/NVMe) | `cr_ui_health_read` (SMART+NVMe+SSD heuristic+TRIM; ABI v2 NVMe tail) | **MATCH+** | audit §2.5 |
| 25 | BitLocker: FVEAutoUnlock registry + `\\.\Pipe\DRWBitlocker` (0x138-stride, ≤100 records, one-shot push); manage-bde fallback | `cr_ui_open_volume_with_key` (FVEK XTS-AES) + `cr_ui_unlock_volume_with_dictionary` (228-entry Dict) | **MATCH+ (documented divergences: explicit-consent unlock vs service pipe — the pipe contract is corpus ground truth for the SERVICE shape, not required in-core)** | corpus-api-context §1.14 |

### 1.5 Driver transport (user-mode contract)

| # | Corpus surface | Core equivalent | Status | Evidence |
|---|---|---|---|---|
| 26 | Open grammar `\\.\{SYM}\<pidtoken>\Volume\<N>` / `\<disk>\<part>`; PID-token both-sides-exact; disk#≤199 (disk.sys-verified walk) | `src/win/cr_transport.cpp` same grammar + token transform | **MATCH** | driver-api-surface §2; core-api-surface §4 |
| 27 | 512-byte-sector semantics; `Information` legacy ×512 unscale (F11) | F11 unscale mirrored `cr_rda.h` | **MATCH** | driver-api-surface §5 |
| 28 | Identity IOCTLs 0x60002400/04/08 (RAW values); extended GET_INFO/OPEN_TARGET/GET_VERSION = `0x600244x` (CTL_CODE_RDA) | `cr_rda.h` constants correct; **INCONSISTENT (doc tail)**: `DRIVER-REQUIRED.md:64-65`, `PLAN-DRIVER-ONLY.md:63-64`, `CORE-API-CURRENT.md:499-500`, `CORE-DISK-ACCESS.md:313-314`, `PARITY-DELTAS.md:3046-47`, `CAPABILITY-MATRIX.md:19`, `DRIVER-RELEASE.md:196`, `cr_driver_status.h:42-43`, `cr_transport.cpp:392-393` still carry the phantom-zero `0x6000244x` literals | **FIX ITEM 1** | this session's grep; commit 3bda8e2 fixed the wire, not the doc tail |
| 29 | Driver-status classifier (absent/not-started/unauthorized/version-mismatch/errored → 0x500B) | `cr_ui_driver_status` + 8-step classifier + CLI exit 4 | **MATCH** | DRIVER-REQUIRED.md §4 |
| 30 | EPMNtDrv pass-through IOCTLs on bound handles | method-aware pass-through (F10) | **MATCH** | driver-api-surface §3.3 |
| 31 | EUDCPEPM 19-code copy-session protocol; EUEDKEPM 2-segment door | merged in rda-driver; core consumes grammar path only (copy-session = documented wire surface, no in-core consumer — correct: the og's consumer is the WinPE copy chain, not DRW's engine) | **MATCH (scope-correct)** | driver-api-surface §3.4 |

### 1.6 ABI infrastructure (the C ABI's own contract)

| # | Claim site | Status | Evidence |
|---|---|---|---|
| 32 | `cr_ui.h:916` names `CR_UI_ERR_INVALID_PARAM` — **macro does not exist** (impl returns `ERR_BAD_PARAMETER`) | **INCONSISTENT (hallucinated identifier in a normative header comment) — FIX ITEM 2** | grep: single site; cr_ui.cpp:3108-3117 |
| 33 | `cr_ui.h:407` documents transport `"rda", "direct" or "image"` — `"direct"` unreachable since driver-only | **INCONSISTENT (stale doc) — FIX ITEM 2** | cr_ui.cpp sets only "rda"/"image" |
| 34 | `ABI-CONTRACT.md §4` error table lacks the `ERR_DRIVER_*` 0x500B family ABI opens return | **INCONSISTENT — FIX ITEM 3** | ABI-CONTRACT.md:72-88 vs cr_error.h:91-99 |
| 35 | `ABI-CONTRACT.md §6` event block stops at type 9 (no `CR_EVT_FS_CHECK` 10 / M-041 struct tail) | **INCONSISTENT — FIX ITEM 3** | ABI-CONTRACT.md:202-257 vs cr_ui.h:168-233 |
| 36 | `ABI-CONTRACT.md §7e` still documents the removed RDA→direct fallback + WinPE gate | **INCONSISTENT — FIX ITEM 3** | ABI-CONTRACT.md:562-600 vs cr_ui.cpp:1152-1189 (driver-only) |
| 37 | `API.md` says 78 exports (actual 79); omits `cr_ui_driver_status`/`cr_ui_last_error`/`cr_ui_snapshot_close`; §2.2 fallback-era wording | **INCONSISTENT — FIX ITEM 3** | core-api-surface §7 item 6 |
| 38 | `CORE-API-CURRENT.md` pre-driver-only census body (§e/§f, b.2 row) describes removed mechanisms as current | **INCONSISTENT (historical census banner'd; body not rewritten) — FIX ITEM 3 (point edits)** | core-api-surface §7 item 7 |
| 39 | `API-SURFACE-AUDIT.md` headline "61 exports" (now 79) + counts | **INCONSISTENT (stale headline) — FIX ITEM 3** | audit :31-36 |
| 40 | `ABI-CONTRACT.md §3` "progress callbacks fire on the thread running run()/process()" vs §6 worker-thread events | **Clarify (one sentence) — FIX ITEM 3** | core-api-surface §7 item 12 |

### 1.7 Data assets behind API surfaces

| # | Corpus asset | Core status | Evidence |
|---|---|---|---|
| 41 | EUUserData 111-entry default user-sign DB | **landed (37c48c6, M-065)**; consumption mode resolved (CHK repair chain; core's load = SUPERSET D-104) | 025dee7 board |
| 42 | raw_signs.json 139-extension table (engine-init `std::ifstream` load) | compiled-in table (D-103 registered, reproducibility-over-mutability decision) | e81b98f |
| 43 | Dict 228-entry password dictionary | shipped (cr_dict_data.cpp) | corpus-api-context §4 |
| 44 | FilterFolder.ini noise blocklist + FatFilterName 10 locales + TxtViewFiles 119 ext | landed (cr_filterfolder_data.cpp) | corpus-api-context §4 |
| 45 | PriorityDisplayType.xml / usefullostfiles.xml / SumaryFileType groups | landed (query/filefilter data layers) | corpus-api-context §4 |

### 1.8 Deliberate divergences (registered, no action)

D-01/F10 session hardening, D-14 grade-at-scan, D-16 BitLocker legitimate-unlock
only, D-18 pause/resume superset, D-29 client-side filtering, D-53 tag not exposed,
D-104 default user-sign set, D-103 compiled-in raw-signs, D-DRV-1..5 driver-only
transport + classifier, D-API-1 host-side naming. All documented in
docs/PARITY-DELTAS.md lineage.

## 2. Executed this session (fix ledger)

| Item | Action | Commit |
|---|---|---|
| 0 | CI-red repair: MSVC C4244 (FH11 extent check) + clang-format sweep (9 files) — the parallel lanes' two breakages; 965/965 green on gcc-13+ASan (CI lane's exact config) | `1117ac9` |
| 1 | Phantom-zero IOCTL literal doc tail (DRIVER-REQUIRED, PLAN-DRIVER-ONLY, CORE-API-CURRENT, CORE-DISK-ACCESS, PARITY-DELTAS, CAPABILITY-MATRIX, DRIVER-RELEASE, cr_driver_status.h, cr_transport.cpp) → `0x600244x` | this session |
| 2 | cr_ui.h:916 phantom macro → ERR_BAD_PARAMETER; :407 "direct" removal | this session |
| 3 | ABI-CONTRACT §4 (0x500B family rows), §6 (FS_CHECK type 10 + struct tail), §7e (driver-only rewrite), §3 sentence; API.md (79 exports, 3 missing rows, driver-era §2.2); CORE-API-CURRENT point edits; API-SURFACE-AUDIT headline | this session |

## 3. Remaining board (after parallel-lane landings + that session)

- **S-038** app-installer surface (packaging milestone; step 1 = PACKAGING.md
  landed `e75d4df`; steps 2-4 = installer implementation + tests — a
  Windows-lane deliverable, queued for the packaging lane).
- **M-064** driver deployer versioned/verified contract (rda-driver scripts
  lane — S-036 batch landed the core side; the upstream v0.2.3 deployer batch
  remains).
- **N-089** FCP-event subtype relabel (MINOR, labeling-only — carve lane).
- **I-026** unsigned-by-design posture (INFO, deliberate).
- **OQ-SA-1** anchor-datum fields probe (tracked-open, engine scope).
- **OQ-S4-3** chunking perf probe (tracked-open, engine scope).
- **A-ABI-6** codeql lane `if: false` vs RELEASE-NOTES claim (policy decision
  for the repo owner, flagged in the audit).

---

# Session 2 (2026-10-28, api-fix continuation) — the post-A-011 truth pass

> Method: context-recovery protocol re-run, API scope only. Three parallel
> read legs produced `/home/z/my-project/api-remediation/context/{research,
> core,driver}-api-inventory.md`; every item below was then verified
> FIRST-HAND against the tree before fixing (zero-invention bar). Corpus
> `03ad46c`; core base `c899998`; driver `f569836`.

## 4.1 Re-verification of the session-1 board (wire + counts)

| Surface | Verification | Result |
|---|---|---|
| Driver wire, user-mode half | cr_rda.h vs rda-driver `src/shared/inc/rda_ioctl.h` line-by-line: identity 0x60002400/04/08, extended 0x6002440/44/48, all 19 session codes 0x800C2AC0..0x800C2B42, F11 Information x512 | **MATCH (byte-exact)** |
| cr_ui export count | 82 `CR_UI_API` header hits - 3 macro defines = 79; API.md:77 claims 79 | **MATCH** |
| Error model | `rg -c "^\s+ERR_\w+\s*=" include/cr/cr_error.h` = 39 + OK; CORE-API-CURRENT banner claims 39+OK | **MATCH** |
| CLI dispatch (code truth) | wmain rrecov.cpp:2779-2800 — 10 verbs: list, list-carve-formats, scan, tree, recover, health, fix, apply, bench, driver | **MATCH (code is the truth; docs below drifted)** |
| M-064 upstream status | REBASE RE-VERIFICATION: rda-driver advanced to v0.2.4 (`323e75a`) while this session ran — the M-064 remainder landed in full: `-Layout Versioned` (default, the `%s%d` coexistence scheme, in-place upgrades, exit codes 8/9), `-Uninstall` verb (delegates to uninstall.ps1), versioned-aware teardown, PS AST parse gate in CI; v0.2.3 (`6b50bb8`) had already landed Parameters write+verify. Finding moved to FIXED by the remote lane (`dfb2761`); core's driver-status discovery verified UNAFFECTED (the service carries the binPath) | **CLOSED upstream (v0.2.4) — no core action; reconciled into this board at rebase** |

## 4.2 NEW drift found and verified this session (fix items A-D)

| # | Site | Drift (verified) | Status |
|---|---|---|---|
| A | `docs/crsf-format.md:27-29` (+ no payload section) | Record-type list stops at `8 DISK_LAYOUT (M-042)` — omits `REC_FS_CHECK = 9` (cr_crsf.h:75, M-041); no FS_CHECK payload grammar anywhere in the doc | **FIXED — `5c3fb32`** |
| B | `src/app/rrecov.cpp:4-17` (header comment) | Lists 8 of 10 verbs (missing `list-carve-formats`, `apply`); scan line omits `--image`/`--deep`/`--repair-undo`/`--user-signs`; recover line omits `--image`; stray `*` artifact mid-continuation line 10 | **FIXED — `3c9a747` (B+C share the CLI truth-pass commit)** |
| C | `docs/API.md §1` (:27-41, :44-60) | 8 of 10 verbs (missing `list-carve-formats`, `apply`); scan line omits `--image`/`--deep`/`--repair-undo`; `[--repair [plan]]` carries a phantom `[plan]` value (`--repair` is a bare `has_flag`, rrecov.cpp:1089); recover line omits `--image` (rrecov.cpp:1611); exit-code table has a DUPLICATE row 3 (:58 and :60) | **FIXED — `3c9a747` (B+C share the CLI truth-pass commit)** |
| D | `docs/CORE-API-CURRENT.md §c` | :313-314 says 9 verbs — omits `driver` — with stale wmain cite 2495-2551 (actual 2779-2800); c.7/c.8/c.9 document the REMOVED `--no-rda` flag (rrecov.cpp:1926/:2462 comments mark it removed); c.9 claims transport label `"rda"/"direct"` reported — only `"rda"` exists (rrecov.cpp:1992, PLAN-DRIVER-ONLY R10); c.9's doc-drift note cites API.md's already-fixed `info`-verb line as current; §c function-range cites stale throughout (print_disks 560-590 -> 636-664, print_carve_formats 597-655 -> 673-735, scan 781-1365 -> 857-1486, tree 1368-1423 -> 1530-1581, recover 1425-1641 -> 1587-1808, health 1643-1753 -> 1813-1921, bench 1754-1874 -> 1924-2125, apply 1989-2285 -> 2161-2458, fix 2287-2493 -> 2460-2666) | **FIXED — `8ff17fc` (grew into the §a/§b/§d truth-and-cite refresh: phantom prefer_rda param, removed rda_available/win_pe_environment rows, 51 stale cr_ui.h cites re-pinned)** |

Grounding for item A's payload section (verified first-hand):
`serialize_fs_check` (src/core/check/cr_fscheck.cpp:1009-1044) /
`deserialize_fs_check` (:1046-1089); the og lineage is M-041's
FileSystemCheck error set 0x1001..0x100e (finding
`findings/FIXED/M-041-damage-audit-check-phase-missing.md`).

## 4.3 Board after session 2

API-surface code and its normative docs are now consistent at
`c899998`+this session. Remaining items are all OUTSIDE the core API
surface: S-038 steps 2-4 (packaging lane, pending the owner's two
PACKAGING.md decisions), M-064 CLOSED upstream at rda-driver v0.2.4,
A-ABI-6 (owner policy), OQ-SA-1/OQ-S4-3 (engine-scope probes), I-026
(deliberate posture). N-089 FIXED at `c899998`; A-011/N-089 convergence
reconciled by the remote lane (`fc74677`).

**Pin added:** `CliVerbListsAreThreeSiteLocked` — any verb/flag/export
change must land docs/API.md §1, CORE-API-CURRENT §c AND the rrecov.cpp
header comment in the same change (the three sites this session proved
drift independently).

---

# Session 3 (2026-10-28, api-fix continuation — the corpus-parity expansion board)

> Method: context-recovery protocol run 3, API scope only. Three parallel
> read legs (corpus / core / driver) re-ran; every item below was then
> verified FIRST-HAND against the tree before filing (zero-invention bar).
> Corpus `be0cfe9`; core base `89dd7ad`; driver `323e75a` (v0.2.4). A fresh
> local verification lane was bootstrapped in this container (cmake 4.4.4
> via pip, gtest 1.15.2 via FetchContent, gcc 14.2.0 Release): full suite
> **977/977 green** at the base tree (the Werror/-Wall -Wextra -Wpedantic
> flags are global in CMakeLists — this is the Linux build lane's config).

## 5.1 Standing state re-verified (no re-audit drift since session 2)

| Surface | Verification | Result |
|---|---|---|
| Session-1/2 boards | 79 exports / `CR_UI_ABI_VERSION 3` / 39+OK error codes / 10 CLI verbs recounted at `89dd7ad` (wmain rrecov.cpp:2789-2810) | all stand |
| Driver wire, user-mode half | `cr_rda.h` vs rda-driver `rda_ioctl.h`: identity 0x60002400/04/08 + extended 0x6002440/44/48 + session codes 0x800C2AC0..0x800C2B42 — the SAME 19 values both sides (`kSessDiscovery..kSessRrwrRead` cr_rda.h:118-142 == `RDA_SESSION_CODE_DISCOVERY..RRWR` rda_ioctl.h:159-188; the driver worklog's "18-code" phrasing counts the dispatch set minus the AC0 discovery probe — same wire set) | **MATCH (byte-exact)** |
| S-036 fail-loud + FloorVersion close-out (the rda-driver worklog's named "core_recovery-side close-out") | already landed in this tree: `.github/scripts/fetch-release.ps1` `-FloorVersion` semver gate (:27, :59-65), install-driver composite default fail-loud + `FloorVersion v0.2.3` (:45, :68, :71, :108), `release.yml:171` + `staged-scenarios.yml:49` floors, `DRIVER-RELEASE.md:210` mapping | **CLOSED — no core action**; the driver lane's worklog line is stale (recorded for the next driver-lane sync) |
| A-ABI-6 | RELEASE-NOTES.md carries NO CodeQL claim at this tree (grep: zero hits); the deferral IS the recorded decision (ci.yml:173-175 "recorded, not silent" + `CI-CURRENT.md:115`); the stale half was the audit row itself — `API-SURFACE-AUDIT.md:132` cited a "RELEASE-NOTES.md:71 'CodeQL clean'" that line 71 no longer carries (it is now the Win7 import-gate bullet) | **FIXED — S3-1 this session** (audit row + closing summary + remaining-items list flipped; no false claim stands) |
| M-068 | finding filed+fixed in one landing (605d682; the composite now RUNNING-query-first); **REBASE UPDATE**: the hygiene move landed by the board-hygiene lane at `2a556e5` (FIXED/ move + Resolution + RESOLVED-INDEX row) while this session filed — folded, no double-owner | **CLOSED at the rebase** (no api-exp action) |
| S-038 steps 2-4 | **REBASE UPDATE**: while this session filed, the S-038 lane landed steps 2-3 (`3e6a8fa` — the Inno Setup installer against the 13-parameter PACKAGING.md contract + the three test gates + the CI lane, 11 files); step 4 (acceptance flips on the lane's first installer.yml dispatch) remains theirs | step 4 pending the installer lane |
| OQ-SA-1 / OQ-S4-3 | **REBASE UPDATE**: SCAN-2f decoded OQ-SA-1's NTFS +0x64 half first-hand (`7bda022` — the low dword of the FIRST $DATA extent's starting LCN; fix queued in their F-lane); OQ-S4-3 unchanged | engine lanes own both |

## 5.2 The remaining corpus-parity API work — the plan-of-record board

`docs/PLAN-API-EXPANSION.md` is the plan of record; its acceptance criteria
("every PARTIAL row is either PRESENT or moved to the deferred table — zero
silent rows") are still OPEN. Its Waves 1-5 are the remaining API-surface
gaps vs the corpus. Verified ABSENT first-hand at `89dd7ad` (grep over
include/cr + src/ui + src/app; the sole near-hit is
`cr_ui_smart_report`'s `trim_known/trim_enabled` (cr_ui.h:1189) — the
HEALTH facet, NOT the enumeration-row facet R-63 wants):

| # | Op (corpus row) | Corpus evidence (the plan's cites, path-mapped) | Verified state at `89dd7ad` | Lane |
|---|---|---|---|---|
| S3-W1a | session list + summary export (R-56 PARTIAL) | SaveScan/rsf-format--scan-session.md:11; UIUX/07-home-phase-deep-dive.md:21 (LastScanRecordView + Continue-last banner) | **FIXED this session** — core half `78a6552` (the v5 save_unix tail + `Reader::peek_record` + `ScanController::read_summary` + the missing crsf-format VOLUME_INFO grammar section; 4 unit tests) + surface half `eb16335` (`cr_ui_session_list` cr_ui.h:808-850 + the `rrecov sessions [--dir D] [--json]` verb + the three-site lock + ABI-CONTRACT §7f + all count sites 79→80/10→11 + 39 cite re-pins + 2 ABI tests + the C-gate block + the parse-gate stub growth); suite 1009/1009 green locally | **DONE** |
| S3-W1b | session load progress + cancel (R-54 PARTIAL) | SaveScan/rsf-format--scan-session.md:344-376 (load progress 40-65% + per-child cancel via IUIProgressInfo) | **FIXED this session** — `ce6bcb0`: `session::DeserializeProgress` (per-node, ERR_CANCELLED propagates - never flattened into SESSION_FORMAT) threaded through deserialize_tree/load_session/load_session_summary/resume; `cr_ui_session_load_with_events` + `CR_EVT_SESSION_LOAD_PROGRESS` (type 11) + the caller-owned cancel flag (the IsCancelled mirror, polled per node even silent); docs ABI-CONTRACT S7g + S6 row + 81-export counts + the cite re-mechanization; 3 tests; suite 1012/1012 | **DONE** |
| S3-W1c | auto-save-on-completion (R-52 PARTIAL) | SaveScan/rsf-format--scan-session.md:443-451 (auto_<vol>_<ts>.rsf); UIUX/30-interaction-driver-ground-truth.md:64-67 | no `autosave_dir` anywhere (grep ABSENT) | this session |
| S3-W2a | query axes path_substring + state_mask (R-22 PARTIAL) | UIUX/31-filterbar-family-decode.md:55-125 (AdvanceFilterCondition 7 groups) | `path_prefix` landed (the path axis, M-029); substring/state axes ABSENT; tag axis = carve tag surface (D-53 registered) | this session |
| S3-W2b | scan-time folder filter (R-14 GAP) | UIUX/31-filterbar-family-decode.md:140-143 (PushFolderFilter) | `ScanConfig` has no filter input (grep ABSENT) | this session |
| S3-W3a | destination validation codes (R-41 PARTIAL) | architecture/04-recovery-workflow.md:136-141 | `ERR_DEST_*` family has no path-too-long / invalid-path / network-classification codes (cr_error.h 0x5005 family) | this session |
| S3-W3b | chkdsk launch (R-65 PARTIAL) | MOM-modules/mom-module-system--architecture.md:65 (FileSystemCheck.mo + WinChkdsk) | `cr_ui_fsck_volume` ABSENT; `check::run_fscheck` is the in-engine checker, not the og's chkdsk launch | this session (win-layer) |
| S3-W3c | enumeration refresh export (R-86 PARTIAL) | UIUX/13-ui-core-wiring-state.md:14 | behavior already always-fresh; the plan's remaining half is the header contract sentence | this session (doc) |
| S3-W3d | TRIM facet in enumeration rows (R-63 PARTIAL) | UIUX/07-home-phase-deep-dive.md:17 (TRIM chip per row) | `cr_ui_disk_info` rows lack trim fields; health report has them (cr_ui.h:1189); opt-in flag + the plan's DEFERRED-if-slow measurement still open | this session |
| S3-W4a | preview caps shipped defaults (R-34 PARTIAL) | UIUX/10-preview-subsystem.md:22-27 (100 MB / 15 s) | Caps seam exists (`preview::Caps`), no shipped defaults (zero-init = uncapped today) | this session |
| S3-W4b | image create (R-93 PARTIAL) | UIUX/07-home-phase-deep-dive.md:36-37 (disk -> image) | `cr_ui_image_create` ABSENT; `storage::open_image` is the reader only | next |
| S3-W5a | BitLocker turn-off (R-88 tail) | DRWService-exe--bitlocker-service.md:50-89 (unlock chain incl. turn-off) | `cr_ui_turn_off_bitlocker` ABSENT (grep) | next |

## 5.3 Execution rules this session obeys

- Landing order = the plan's (Wave 1 -> 5), one op per landing; each op lands
  as C++ + ABI export + CLI (where the plan rules) + contract text + tests.
- `CliVerbListsAreThreeSiteLocked`: docs/API.md §1 + CORE-API-CURRENT §c +
  the rrecov.cpp header comment move together on every verb/flag change.
- ABI stability policy (ABI-CONTRACT §9): append-only exports/fields = no bump.
- Read-on-demand: each op's corpus counterpart is read FIRST-HAND before
  implementation; nothing lands without its citation re-verified.
- Verification per landing: local gcc-14 lane (977/977 base) + clang-format-18
  + the dead-surface/architecture/secrets/commit-convention gates.
