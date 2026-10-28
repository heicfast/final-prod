# Liveness battery — SEVERE tier + tracked-open (pin 2794da8b, agent A)

Audited tree: `/home/z/my-project/repos/core_recovery` at `2794da8b` (verified via `git rev-parse --short HEAD`; working tree clean — no divergence between tree and pin). Read-only audit; no file inside either repo was modified.

## Verdict table

| ID | Title (short) | Verdict | Current-code evidence (file:line) | Retirement bookkeeping? |
|---|---|---|---|---|
| S-040 | Disk-image creation wholly absent (reader-only) | LIVE | `rg "image_create\|create_image\|cr_ui_image" include/ src/` → **0 hits**; `src/app/rrecov.cpp:3458-3492` (12 verbs, no `image`); `src/win/cr_image_win.cpp:37` (`GENERIC_READ` reader-only); `include/cr/cr_ui.h:313` (`cr_ui_open_image` is the only image export; census 89) | None owed (defect live; board Status: READ-ONLY FINDING) |
| S-050 | File-repair line absent + registration drift | PARTIAL (per the board's own session-11B addendum: false-citation leg fixed at filing; capability leg + formal disposition live) | `src/core/repair/` = `cr_rawfixer.cpp` only; `rg "imgrepair\|docrepair\|videorepair\|repair_file" src/ include/` → **0 hits**; no CLI verb (`rrecov.cpp:3458-3492`); `docs/CAPABILITY-MATRIX.md:77-92` (§3 has no repair row); `diff-to-fix/SURFACE-INVENTORY.md:98` (row now truthful, cites S-050) | None owed — S-050 stays ACTIVE (I-035's FIXED file explicitly records "S-050 remains ACTIVE … and owns the repair-line decision") |
| S-055 | User-sign layer inverts built-in-first collision rule | LIVE | `src/core/carve/cr_carve.cpp:884-886` (user-sign probe) runs BEFORE `:978` (`idx.probe_header`) in the same stride iteration, `continue` at `:953`; comment `:873-879` still asserts "built-in-first" | None owed (defect live) |
| S-056 | disabled_formats does not gate the user-sign layer | LIVE | `src/core/carve/cr_carve.cpp:884-956` (user-sign block — zero disabled-format references); gate for built-ins at `:984-992`; disable-map build `:1325-1339` builds only `trial_disabled` + `rawsign_disabled` | None owed (defect live) |
| S-057 | Og auto-saves stopped scans; core never saves cancelled | LIVE | `src/ui/cr_ui.cpp:2191-2192` (`if (rc != OK) return ret_(rc, "scan %s", err_text(rc));`) precedes the auto-save block at `:2211-2242`; false og-claim comment at `:2193-2200`; CLI completion-only at `src/app/rrecov.cpp:1942-1943` | None owed (defect live; I-041 FIXED file records "S-057 … — open") |
| S-058 | Grading extension-table gate polarity inverted | LIVE | `src/core/tree/cr_grading.cpp:275-276` — `if (!extension_in_grading_table(ext_of_name(pr))) continue;` (twins collected only for IN-table exts); pin test `tests/unit/test_grading.cpp:166-187` still pins the inverted reading | None owed (defect live) |
| S-059 | Sparse/hole export: left-shift or ERR_TRUNCATED refusal | LIVE | `src/core/ntfs/cr_runlist.cpp:190-191` (`if (r.sparse) continue;`); `src/core/ntfs/cr_mftwalk.cpp:262-263` (misleading "writer zero-fills" comment) + `:344-352` (holes emit no extents); `include/cr/tree/cr_node.h:114-119` (Extent = `{offset,length,codec,psize}`, no logical); `src/core/recovery/cr_safewriter.cpp:419-422` (cumulative walk) + `:442-444` (`ERR_TRUNCATED`) | None owed (defect live) |
| S-060 | Recovered-file timestamps/attributes never restored | LIVE | `rg "SetFileTime\|SetFileAttributes\|utimensat\|utimes\|set_times\|fchmod\|set_metadata" src/ include/` → **0 hits**; `include/cr/recovery/cr_safewriter.h:40-63` (DestIo: create/mkdir/write/commit/close/free_bytes/exists — no metadata op); `include/cr/win/cr_destio.h:17-46` (same); item body ends commit→close→Done at `cr_safewriter.cpp:548-561` | None owed (defect live) |
| S-061 | ReFS/Btrfs insert-time High grade absent | LIVE | `src/core/tree/cr_grading.cpp:217-219` — bare `return true` for Refs/Btrfs, no `set_grade`; no Refs/Btrfs grade test in `tests/unit/test_grading.cpp` | None owed (defect live) |

Retirement cross-check: none of the 9 IDs appears as a row in `diff-to-fix/FIXED-LEDGER.md` (grep `^\| S-0(40|50|55|…)` → no rows; the only mention is I-035's row, which references S-050 as enrichment). None of the 9 has a copy under `findings/FIXED/`. All 9 remain in `findings/SEVERE/` with `**Status:** READ-ONLY FINDING — no fix applied` — **no FALSELY-ACTIVE cases** (every live defect matches an active finding; no fix landed un-retired).

## Per-finding detail

### S-040 — LIVE

- **Finding's claim:** the og ships a home-page "Create Image" flow (DiskBackupDialog chain, ToolsModule/libvmdk) producing the image files its own scan path consumes; the core implements only the reader half (flat/VMDK/VHD/VHDX via `storage::open_image`, `--image` scan targets) and has no producer: no `cr_ui_image_create` ABI export, no win-layer writer, no CLI `image` verb.
- **Current code:**
  - `rg -c "image_create|create_image|cr_ui_image" include/ src/` → **0 hits** (exit 1).
  - `src/win/cr_image_win.cpp:37`: `HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,` — still reader-only.
  - `src/app/rrecov.cpp:3458-3492` — the verb dispatch: `if (cmd == L"list")` … `L"driver"` — 12 verbs (list, list-carve-formats, sessions, scan, tree, archive-list, recover, health, fix, apply, bench, driver); **no `image` verb**. Header comment `rrecov.cpp:4-38` and usage block `:3408-3450` list the same 12.
  - `include/cr/cr_ui.h` — `CR_UI_API` census now 89; the only image export is the reader `cr_ui_open_image` (`:313`).
  - Plan-board rows still mark it absent: `diff-to-fix/API-DELTA.md:259` (S3-W4b: "`cr_ui_image_create` ABSENT; `storage::open_image` is the reader only") and `docs/PLAN-API-EXPANSION.md:64` (R-93 PARTIAL, still a plan row).
- **Tests present?** None — `rg "ImageCreate\." tests/` → 0 hits (the fix plan's `ImageCreate.HashMatchesSource` / `CancelMidStream` / `NoDriverRefuses` are absent).
- **Notes:** line drift vs the finding's citations — verb table was `:3027-3048` at filing, `:3127-3148` at the session-11B re-triage, now the dispatch chain is at `:3458-3492` and the usage block at `:3408-3450`; export census 81 → 86 (session-11B) → **89** now; absence unchanged. Session-11B cite-refresh addendum (in the finding file) remains accurate.

### S-050 — PARTIAL (capability leg LIVE; registration leg fixed at filing; formal disposition pends)

- **Finding's claim:** (1) the og bundles a complete file-repair product line (photo/video/office/HWP/raw/7z-archive repair — 9+ binaries, `enable_imgrepair`/`enable_docrepair` remote-config flags, DRWUI CHK/`ln` integration) that the core wholly lacks (its `src/core/repair/` is the different RawFixer disk-repair contract); (2) the only trail disposition was a SURFACE-INVENTORY row citing a CAPABILITY-MATRIX §3 exclusion list that did not contain the repair line.
- **Current code:**
  - `ls src/core/repair/` → **`cr_rawfixer.cpp` only** (the RawFixer disk-repair contract).
  - `rg -c "imgrepair|docrepair|videorepair|archive_repair|repair_file" src/ include/` → **0 hits**.
  - No file-repair CLI verb: the only repair flags are the RawFixer pre-scan pair `--repair`/`--repair-apply`/`--repair-undo` (`rrecov.cpp:17-18`, `:1500-1511` region); verb dispatch `:3458-3492` has no repair-file verb.
  - Registration leg (leg 2) — fixed as the session-11B addendum records: `diff-to-fix/SURFACE-INVENTORY.md:98` now reads "**AUDITED this session (SA-10B): S-050** — unimplemented AND the prior row's OUT-OF-SCOPE (CAPABILITY-MATRIX §3) citation was FALSE (§3 does not list the repair line)…". The formal disposition still pends: `docs/CAPABILITY-MATRIX.md:77-92` (§3 "Explicit exclusions (standing)") still lists only Cloud/NAS/remote, BitLocker FVE-key derivation, signing, `.rsf` import — **no repair-line row** (the only "repair" rows in the matrix are RawFixer `:71` and boot-chain repair `:60/:72` — disk-repair surfaces).
- **Tests present?** None — `rg "RepairImage|Repair7z|RepairOffice" tests/` → 0 hits.
- **Notes:** verdict PARTIAL matches the board's own session-11B addendum ("PARTIAL — the false-citation leg was fixed at filing; the capability leg stands"). Enrichment addenda (session-11-lead per-chunk feed, S41 UIUX/32, session-48 I-035 constants fold-in) are all present in the finding file; the I-035 register is retired as ACKNOWLEDGED (`findings/FIXED/I-035-chk-repair-chain-decode-addendum-for-s050.md`, ledger row 502) with "S-050 stays ACTIVE and owns the repair-line decision" — consistent with this audit.

### S-055 — LIVE

- **Finding's claim:** the code contract (comment + `cr_usersigns.h` header) says a user sign colliding with a built-in raw sign fires only when the built-ins did not match (built-in-first), but the code probes `usersigns::match_at` before `idx.probe_header` in the same loop iteration and `continue`s on hit — so in the default-on set the 8 overlap formats (amr, cap, class, cpl, flac, msf, pdb, wpl) flip to device-end user-sign records instead of bounded built-in candidates.
- **Current code** (`src/core/carve/cr_carve.cpp`):
  ```cpp
  871            if (emitted_at.count(start_abs))
  872                continue; /* a grammar-validated file starts here */
  873            /* M-048: the user-extensible signature layer (the og's
  ...
  878             * BUILT-IN fires only when the built-ins did not match -
  879             * built-in-first, the collision rule the finding specifies).
  ...
  884            if (usersigns::active_count() != 0) {
  885                if (const carve::usersigns::UserSign* us =
  886                        usersigns::match_at(win + off, want - off)) {
  887                    const u64 len = ctx.total - start_abs;   /* device end */
  ...
  922                        props->carve_format = 0xFFFE0000u; /* user-sign tag */
  ...
  953                        continue;                  /* built-in never runs here */
  ...
  978            idx.probe_header(win + off, want - off, matches);   /* the built-in table */
  ```
  The header contract is still the (false) built-in-first claim: `include/cr/carve/cr_usersigns.h:88-92` — "First match in registration order wins (deterministic; built-ins were already probed by the earlier layers at the same stride)."
- **Tests present?** The pre-existing pin test `CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition` at `tests/unit/test_carve.cpp:7557-7592` — still grammar-layer-only (plants a valid PNG, a grammar format; asserts `FMT_PNG` claims the position, `0xFFFE0000` count 0). The finding's fix-plan tests (`Carve.UserSignYieldsToRawSignTableAtSamePosition`, per-overlap-format variants, `UserSignStillFiresWhenNoBuiltinMatches`, `GrammarRejectedMagicNotReclaimedByUserSign`) — **none exist** (`rg` → 0 hits).
- **Notes:** line drift — the block was `:725-808` (probe `:738-740`, `continue` `:805`, comment `:727-733`) at filing; now comment `:873-883`, probe `:884-886`, tag `:922`, `continue` `:953`, `probe_header` `:978`. The defect shape is byte-identical. `data/userdata-defaults.json` still ships (the default-on set the finding's impact analysis uses), and the CLI default-loads it (I-036's registered posture, `rrecov.cpp:1269+`).

### S-056 — LIVE

- **Finding's claim:** `disabled_formats` gates the grammar layer and the built-in raw-sign layer but not the user-sign block — a host disabling "cpl"/"jpg"/… still gets those formats carved through the user-sign channel, contradicting D-104's registration claim ("gated by the same format-toggle rules as every other carve source").
- **Current code** (`src/core/carve/cr_carve.cpp`):
  - The user-sign block `:884-956` contains **zero** references to `rawsign_disabled`, `trial_disabled`, `disabled_formats`, or any user-sign disable map (verified by reading the whole block).
  - The built-in layer's gate, `:984-992`:
    ```cpp
    984            if (!ctx.rawsign_disabled.empty()) {
    985                size_t w = 0;
    986                for (size_t r = 0; r < matches.size(); ++r)
    987                    if (!ctx.rawsign_disabled[matches[r]->fmt])
    988                        matches[w++] = matches[r];
    ```
  - The disable-map build `:1325-1339` still builds exactly two maps (`ctx.trial_disabled` for grammar, `ctx.rawsign_disabled` for built-ins) — no user-sign map; `usersigns::match_at` (`cr_usersigns.h:92`) still takes no predicate: `const UserSign* match_at(const u8* p, u64 avail);`.
- **Tests present?** None — `Carve.DisabledFormatsGatesUserSignLayer` / `…CaseInsensitive` / `DisabledOtherFormatsLeaveUserSignsOn` → 0 hits.
- **Notes:** line drift — disable-map build was `:1118-1131`, now `:1325-1339` (the grammar leg gained the N-149 exempt-set clause at `:1326-1334`); raw-sign gate was `:836-844`, now `:984-992`. I-036's FIXED file (session register) explicitly lists S-055/S-056 as "the channel's defects — all CLI-default-path" and still open — a parallel lane confirmed, not resolved, them.

### S-057 — LIVE

- **Finding's claim:** the og's stop path runs its own organize phase and feeds the same completion handler whose auto-save gates (not-import, results-found) contain no stopped-check — stopped scans' partial results land in SaveScan; the core's cancel path returns `ERR_CANCELLED` before the completion auto-save block ever runs, and the CLI mirrors completion-only.
- **Current code:**
  - `src/ui/cr_ui.cpp:2191-2192`:
    ```cpp
    if (rc != OK)
        return ret_(rc, "scan %s", err_text(rc));
    ```
    followed by the still-false comment `:2193-2196`: "ONLY a completed run auto-saves (a cancelled run never does - the og's flow is the completion window, and save-on-cancel is the explicit --out/ cr_ui_scan_save_session surface)" — the exact claim S-057's stop-organize decode (DRWUI:1549399-1549421 → :339078-79 → :612571-587) refutes.
  - The auto-save gate block at `:2211-2212`: `if (s->cfg.autosave_dir[0] != 0 && !s->is_resumed && !s->user_saved && (s->stats.files_found != 0 || s->stats.lost_partitions != 0))` — unreachable for `rc == ERR_CANCELLED` (exits at `:2192`).
  - CLI leg, `src/app/rrecov.cpp:1941-1943`: "W1c (R-52): the completion auto-save (…; cancelled runs return before this point, so ONLY completed runs auto-save)" — completion-only, no stop-window parity leg.
- **Tests present?** None — `CrUi.CancelledRunWithResultsAutoSaves` / `CancelledRunZeroFoundWritesNothing` / `CancelledAutoSaveLoadsPartialTree` / `Cli.AutosaveDirStopWindowWrites` → 0 hits.
- **Notes:** line drift — cancel return was `:2147-2148`, now `:2191-2192`; comment was `:2152-2156`, now `:2193-2200`; gate block was `:2166+`, now `:2211+`; the CLI block cited at `:1831-1838` is now `:1941-1982`. I-041's FIXED resolution notes and M-131's related-findings row both record S-057's stop-window leg as still open — parallel lanes landed adjacent work (state-mask register, checkpoint integrity) without touching this defect.

### S-058 — LIVE

- **Finding's claim:** the og's completion pass runs the twin/deep-validation branch for extensions NOT in the 169-key grading table (`FUN_1401bebe0` is an INEQUALITY comparator: `if (cVar1 == '\0')` enters when the key IS FOUND… i.e. the block runs when NOT found); the core runs twin detection only for IN-table extensions — the exact inverse — and its test pins the wrong polarity.
- **Current code** (`src/core/tree/cr_grading.cpp:272-281`):
  ```cpp
  /* generic path (FS not in {NTFS, ReFS, Btrfs}): the extension
   * table gates the twin detection (found in the 169-entry map at
   * DRWUI :605740-605741 -> the deep path runs). */
  if (!extension_in_grading_table(ext_of_name(pr)))
      continue;
  TwinKey tk;
  ...
  ```
  Still collects twin candidates only for in-table extensions; the comment still carries the s5 misreading. Twin pass (overlap flags) at `:284-296`.
- **Tests present?** The inverted pin is still there, `tests/unit/test_grading.cpp:166-187` (`Grading.ExtensionTableGatesDeepValidation`): `EXPECT_TRUE(a1->overlap()) << "in-table (DOC) twins must be flagged"; … EXPECT_FALSE(c1->overlap()) << "out-of-table extensions never enter the deep path";`. The fix-plan tests (`Grading.TableGatePolarityNotInTableEnters` / `InTableSkips` / `DotlessSkipsTwins`) → 0 hits.
- **Notes:** no line drift — gate at `:275-276` and test at `:166-187` match the finding's citations exactly. The s5 evidence erratum the fix plan asks to file (`evidence/decompiled-quotes/s5-grading-extension-table-dump.md` §5 item 4) has no correction note appended (the finding's own worksheet `s42-grading-gate-polarity.md` remains the corrected record).

### S-059 — LIVE

- **Finding's claim:** for NTFS sparse/compressed files with holes, the og writes every logical byte in order (holes zero-filled, full logical size); the core drops hole runs from the extent list, its writer places extents cumulatively with no logical offset (interior holes left-shift content), and when the extent sum falls short of `logical_size` it fails the whole file with `ERR_TRUNCATED` and deletes the temp.
- **Current code:**
  - `src/core/ntfs/cr_runlist.cpp:187-198` — `RunList::extents()`:
    ```cpp
    189        for (const auto& r : runs_) {
    190            if (r.sparse)
    191                continue;
    ```
    (sparse runs dropped; adjacent real runs coalesced `:192-196`).
  - `src/core/ntfs/cr_mftwalk.cpp:262-263` — the comment still claims `header == 0 -> hole: the remaining blocks of the unit are sparse (no extent; the writer zero-fills)`; and `:344-348` + `:351-352` — the expander `break`s on holes, emitting no extents:
    ```cpp
    351            if (hdr == 0) {
    352                break; /* hole: the rest of the unit is sparse */
    ```
  - `include/cr/tree/cr_node.h:114-119` — `struct Extent { u64 offset; u64 length; u32 codec = 0; u64 psize = 0; }` — **no logical-offset / hole field**.
  - `src/core/recovery/cr_safewriter.cpp:408-445` — the cumulative walk (`while (done < total)`, extent located by cumulative `ext_pos` sum at `:419-422`) and the refusal:
    ```cpp
    440            io_.close(it.item_id);
    441            it.state = ItemState::Error;
    442            it.error = ERR_TRUNCATED;
    443            ++stats.errors;
    444            return ERR_TRUNCATED;
    ```
  - The metadata-only enqueue skip at `:276-291` (`logical_size > 0 && extents.empty() && resident_data.empty()` → `nothing_to_recover`) — still no hole accounting.
- **Tests present?** None for the export side — `tests/unit/test_runlist.cpp:36-49` still only proves sparse runs PARSE (decode + `vcn_to_lcn`); `test_safewriter.cpp` does not exist; `test_writer_dirs.cpp` has no sparse/hole test. Fix-plan tests (`Runlist.SparseHoleEmitsExtent`, `Writer.InteriorHoleZeroFilledFullSize`, `Writer.CompressedZeroUnitIsHole`, `Session.ExtentLogicalRoundTrip`) → 0 hits.
- **Notes:** minimal drift — `RunList::extents` still starts at `:187`; mftwalk hole-emission drifted `:340-352` → `:344-352`; `Extent` struct `:109-120` → `:114-120`; writer walk `:405-455` → `:403-445` with `ERR_TRUNCATED` at `:442-444` (matches the finding's own merged-HEAD re-pin `:442-445`).

### S-060 — LIVE

- **Finding's claim:** after every successfully copied item the og restores DOS attributes (`SetFileAttributesW`, mask `& ~0x2`) and mask-gated timestamps (`SetFileTime` per bits 0x8/0x10/0x20); the core has no surface at any layer — no DestIo metadata operation, no writer call, no report field — so every recovered file gets "now" timestamps and default attributes.
- **Current code:**
  - `rg "SetFileTime|SetFileAttributes|utimensat|utimes|set_times|fchmod|set_metadata" src/ include/` → **0 hits** (exit 1).
  - `include/cr/recovery/cr_safewriter.h:40-63` — `class DestIo` exposes exactly `create_temp`/`mkdir`/`write`/`commit`/`close`/`free_bytes`/`exists` — no metadata operation.
  - `include/cr/win/cr_destio.h:17-46` — `WinDestIo` implements exactly that surface (no time/attribute mapping).
  - The item body (`src/core/recovery/cr_safewriter.cpp:323-561`) ends content → hash → `io_.commit` (`:548`) → `io_.close` → `ItemState::Done` — no post-commit metadata step.
  - The report carries no per-file times: `include/cr/recovery/cr_report.h` (the header **moved** from `src/core/recovery/` to `include/cr/recovery/` since filing) — the only time field is `duration_ms` (`:60`, per-item processing time).
- **Tests present?** None — `Writer.RestoresTimestampsAndAttributes` / `MetadataFailureIsNonFatal` / `WinDestIo.SetMetadataMapsVtableGates` → 0 hits.
- **Notes:** refactor to flag — `cr_report.h` relocated `src/core/recovery/cr_report.h` → `include/cr/recovery/cr_report.h` (with `cr_report.cpp` remaining in src/core/recovery/); the finding's citation is stale on path only, the content claim holds. The N-009d refutation target (`diff-to-fix/modules/writer.md:67-75`) was not re-checked this pass (doc-truth item, not the code defect).

### S-061 — LIVE

- **Finding's claim:** at tree-insert time the og grades every non-existing ReFS and Btrfs file High (grade 2) unconditionally with gauge-counter ticks (DRWUI:605481-605497); the core's `solo_rules` returns early for ReFS/Btrfs without setting any grade, leaving the whole population at engine-default Average.
- **Current code** (`src/core/tree/cr_grading.cpp:217-219`):
  ```cpp
  if (fs == u32(FsKindCode::Refs) || fs == u32(FsKindCode::Btrfs))
      return true; /* excluded from the generic path (the og's device
                    * walk covers these; no further decoded rule) */
  ```
  No `set_grade` call on this path (contrast the NTFS rule at `:208-215` which does set grade, and the carve rule at `:200-202`); the comment still carries the disproven "device walk covers these" premise.
- **Tests present?** None — no Refs/Btrfs grade test exists in `tests/unit/test_grading.cpp` (the file tests setter semantics, table membership `:212-219`, twins, counters — never these two FS kinds); fix-plan tests (`Grading.RefsDeletedFilesGradeHigh` / `BtrfsDeletedFilesGradeHigh` / `RefsExistingStillHigh`) → 0 hits.
- **Notes:** no line drift — `:217-219` matches the finding's citation exactly.

## Tracked-open status snapshot

| OQ | Current Status line (verbatim) | Resolution notes? |
|---|---|---|
| OQ-S10-1 | `**Status:** OPEN (session-10, this lane) — spun out of S-045's Resolution (the TIFF-RAW half landed at 3aba383)` | None landed. Session-40 disposition note (2026-10-29): "Still OPEN, scope unchanged and still honest" — refusals remain BY NAME (`thumbnail_format_supported` false for HEIC/WebP/PDF/video); next step (libheif first) stands verbatim. |
| OQ-S10-3 | `**Status:** OPEN (session-10, this lane) — spun out of the two-repo grant boundary` | None landed. Session-40 note: boundary re-affirmed — the five driver-repo findings (M-086/M-087/M-089/N-107/N-127) remain in heicfast/rda-driver, untouchable under the grant; core-side halves re-verified. |
| OQ-S4-3 | `**Status:** OPEN (session 4) · **Module:** transport · **Parameters:** P47 (throughput), P51 (I/O behavior)` — plus a status-update header at line 5: "the transport-chunking half is now MEASURED … **Remaining open: the scan-level time-to-first-result half only**" | Substantial progress, not resolution: the transport-chunking half was MEASURED (alignbench run 40315255368, 2026-10-26 — core 64 KiB envelope meets og 4/7 MiB at qd=1, 256 KiB cache block exceeds it 18-19%); the TTFR half remains open (probe harness landed — `TtfrProbe` in rrecov + Run-Benchmark.ps1 extraction — awaiting a green Windows-lane dispatch to transcribe). Session-40 note: still OPEN. |
| OQ-S42-1 | `**Status:** OPEN (this session) — the core asserts parity without og decode` | None — no addenda beyond the original record (NTFS $SI-vs-$FN time precedence; core pins `cr_mftwalk.cpp:868-877` SI-first without og decode). |
| OQ-S42-2 | `**Status:** OPEN (this session)` | None — no addenda (Btrfs nsec fold-vs-truncate; core folds via `unix_to_filetime_ns`, og side undecoded). |
| OQ-S42-3 | `**Status:** OPEN (this session)` | None — no addenda (the six grading deep-read sub-gates: `FUN_1401bc5a0`, +0x72 bit 7, +0x74 bit 2, vt+0x28, vt+0xb8/vt+0x100, `local_b8[0]`). |

All six remain in `diff-to-fix/tracked-open/` (none moved to FIXED/); OQ-S4-3 is the only one with a materially progressed half (transport measured) and it explicitly remains OPEN on the TTFR leg.

## Uncertainties

- **S-050 verdict boundary:** "PARTIAL" follows the board's own session-11B addendum classification (leg 2 fixed at filing, leg 1 live). Both remaining aspects (the capability gap and the pending implement-vs-§3-register decision) are decision-owner items, not code the audit could verify further; I confirmed no code/verb/ABI surface exists and §3 still lacks the row.
- **S-060 report-path drift:** `cr_report.h` moved from `src/core/recovery/` to `include/cr/recovery/`; I verified the new location's fields (no per-file times). The finding's `modules/writer.md:67-75` (N-009d row) refutation target was not re-read this pass — it is a doc-truth leg, not the code defect.
- **S-058 erratum append:** the fix plan's step 4 (append the polarity correction to `evidence/decompiled-quotes/s5-grading-extension-table-dump.md` §5) has not landed in the evidence tree — noted from the s42 worksheet's existence and §5's un-corrected state as read via the finding; I did not exhaustively diff the evidence file (it is board material, not core code).
- **Export census drift (S-040):** `cr_ui.h` now carries 89 `CR_UI_API` exports (81 at filing, 86 at session-11B). The absence of an image-create export holds regardless of the count; the count sites named in the fix plan (81→82) would need re-basing if/when the fix lands.
- No execution-based verification was performed (static read/grep only, per the read-only mandate); every LIVE verdict above is code-proven at the pin, matching each finding's own "verified first-hand" record. Working tree = pin `2794da8b`, clean.
