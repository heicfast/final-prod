# SEVERE S-044 — Completion auto-save diverges: missing results-found and not-already-saved gates; config modeled as a directory where the og uses an enable flag

**Severity:** SEVERE
**Module:** session / orchestration (W1c R-52 surface)
**Parameters affected:** A31 (scan lifecycle — completion semantics), D12 (session persistence — auto-save contract), S35 (error/failure propagation — empty-scan sessions), B4-adjacent (result-set semantics)
**Corpus references:** `DRWUI_decompiled.c` — auto-save `FUN_1402857c0` :442747 with its ONLY three call sites :612583, :615166, :616764 (each inside scan-phase completion handlers, each gated: not-import `+0x1c8==0`, results-found `FUN_1403964b0 != 0`, and at two sites not-already-saved `+0x388==0`); entry gate :442822-24 `FUN_1400b5270(FUN_1400b3c60()) == '\0'` = byte flag on a config singleton (:165951-5 — `return *(u8*)(param_1+0x10)`), no char[512] directory field anywhere in the path; overwrite `DeleteFileW` :442903-07; folder-scan `L"Folder"` name suffix :442883-886; stale-file `DeleteFileA` :442830-33 — decode in `evidence/decompiled-quotes/s8-autosave-and-stamps.md` §1-§3
**Core references:** audited `ff879f9` — `src/ui/cr_ui.cpp:1999-2019` (the completion leg: fires when `rc == OK && cfg.autosave_dir[0] != 0` — no results-found gate, no already-saved gate), `src/app/rrecov.cpp:1700-1718` (CLI twin), `include/cr/orchestrator/cr_scan.h:282` (`char autosave_dir[512] = {0}` — zero-init = OFF), landed `c95335f` (W1c)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's auto-save fires from completion handlers only when the scan
found results and (at two of three sites) the user has not already
saved; its enable surface is a one-byte flag on a config singleton.
The core's W1c landing fires on every completed scan with a non-empty
`autosave_dir` — including empty scans and scans the user already
saved explicitly — and models the enable surface as a 512-byte
directory string.

## What the reference does

Three trigger sites, all in scan-phase completion handlers, each with
the gating chain above. The `FUN_1403964b0` results-found check means
an empty scan NEVER auto-saves in the og (no session file for a
nothing-found run). The `+0x388` already-saved check (2 of 3 sites)
prevents double-saving after an explicit save. Inside the function,
the sole gate is the singleton byte flag (0 = enabled) — the save path
is composed at the call site. The write overwrites via DeleteFileW
first, the volume name gains a `Folder` suffix on folder scans, and a
stale path is cleaned up (DeleteFileA).

## What our core does

`scan_run_impl`'s completion window (cr_ui.cpp:1999-2019): if
`rc == OK` and `autosave_dir[0] != 0`, write
`auto_<label>_<ts>.crsf` via `write_session_atomic`, set
`stats.autosaved`, emit `CR_EVT_AUTOSAVED`. No results-found gate (an
empty completed scan writes a session); no not-already-saved gate (a
user who explicitly saved gets a second file); the enable surface is
the directory field itself. The CLI twin (rrecov.cpp:1700-1718) has
the same shape. (The write path itself — tmp+finalize+rename, atomic
— is sound, and `write_session_atomic` overwrites, matching the og's
overwrite intent.)

## Gap

Falsifiable: (1) complete a scan that finds zero files with
autosave-dir set — og: no session file; core: an `auto_*.crsf` lands.
(2) Complete a scan, explicitly save via `cr_ui_scan_save_session`,
with autosave-dir set — og (2 of 3 sites): only the explicit save;
core: two files. (3) The config surface: og = byte flag + composed
path; core = directory string (functionally equivalent enable/dest
semantics — this half is a registered-shape difference, not a defect
by itself).

## Impact

User-observable: spurious session files for empty scans clutter the
session list (W1a's list reads every `auto_*.crsf`), and double-saves
duplicate user action. The og's gating is part of the SaveScan
contract the W1c landing cited (rsf-format 5.5 + UIUX/30:64-67) — the
landing implemented the naming/trigger placement but not the gating
chain, and its commit message asserts a "cancelled never auto-saves"
gate the corpus does not show (no IsCancelled check exists at any
trigger site; only the completion-handler placement bounds
cancellation behavior — the core's `rc == OK` placement happens to
match that placement, so the cancel half is equivalent in effect).

## Reproduction / verification

Corpus: `evidence/decompiled-quotes/s8-autosave-and-stamps.md` §1-§3
(agent V-B decode with the call-site/gate citations; the singleton
flag function read directly). Core: cr_ui.cpp:1999-2019 and
rrecov.cpp:1700-1718 read directly this session. The empty-scan and
double-save behaviors follow from the code paths (no execution needed
to establish the absent gates — the conditionals are absent).

## Fix plan (for the engineering team)

1. Add the results-found gate: skip the auto-save when
   `stats.files_found == 0` (and no lost partitions) — mirror the og's
   `FUN_1403964b0` check.
2. Add the not-already-saved gate: track whether an explicit save
   happened for this scan handle (`cr_ui_scan_save_session` sets a
   flag; the completion leg skips when set).
3. Add the folder-scan naming variant (`Folder` suffix) if/when a
   folder-scan flag exists in the core's scan config (record as
   deferred if the core has no folder-scan mode).
4. Record the config-shape difference (byte-flag vs directory field)
   as a deliberate registered delta in PARITY-DELTAS.
5. Files: `src/ui/cr_ui.cpp`, `src/app/rrecov.cpp`,
   `include/cr/orchestrator/cr_scan.h` (doc comment),
   `tests/unit/test_cr_ui.cpp`, preview/orchestration doc rows.
6. Acceptance: empty completed scan → no file, no event, stats
   autosaved==0; found-results scan + explicit save → exactly one
   file; normal completion → exactly one auto file + event.

## Tests to add

AutoSaveSkipsEmptyResults; AutoSaveSkipsAfterExplicitSave;
AutoSaveFiresOnCompletionOnly (existing — keep); AutoSaveCancelNever
(existing — keep); AutoSaveFolderSuffix (when the flag exists).

## Related findings

N-105 (the Folder-suffix naming variant); W1a/W1b/W1c landing records
(worklog API-EXP-1); N-100 (the +0x60 session-wire sibling from the
same landing family).

## Open questions

What the og's `+0x1c8` "not-import" flag maps to in the core's model
(imported/resumed scans) — the core's resume path should be checked
for the same suppression when implementing the gates.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** `9496f3c`
**Verified by:** `CrUi.AutoSaveSkipsEmptyResults` (a blank-image completed scan: no auto file, no CR_EVT_AUTOSAVED, stats.autosaved == 0), `CrUi.AutoSaveSkipsAfterExplicitSave` (a mid-scan explicit save through the pause window — the og's own mid-scan save posture: exactly one session file total, no event), `CrUi.AutoSaveSkipsResumedRun` (a resumed/imported run never re-auto-saves), the retained `CrUi.AutoSaveOnCompletion` (the normal completion still auto-saves exactly once), and the full suite 1030/1030 green on the Linux release lane. TSan lane clean on the threading surface (AutoSave*/Pause/Cancel family, 6 tests).
**Resolution notes:** The corpus evidence (s8-autosave-and-stamps.md §1-§3, DRWUI FUN_1402857c0 + its three call sites) was re-verified first-hand before the fix. All three decoded trigger-site gates landed: **results-found** (`stats.files_found != 0 || stats.lost_partitions != 0` mirrors FUN_1403964b0 — an empty scan never auto-saves, so the W1a session list never sees nothing-found runs), **not-import** (the `is_resumed` flag on the UI resume path — session_open_scan adopts resumed controllers, and the CLI's `--resume`/`--out` explicit save covers its import case through the not-already-saved gate), and **not-already-saved** (a new `user_saved` flag set by a successful `cr_ui_scan_save_session`; 2 of the og's 3 trigger sites carry this gate — the core's single completion site adopts it). The gates live on both the ABI completion window (cr_ui.cpp) and the CLI twin (rrecov.cpp), and the ABI contract docs (CR_EVT_AUTOSAVED, cr_ui_scan_config::autosave_dir_utf8, cr_ui_scan_stats::autosaved) were updated in the same change. The **config-shape difference** (byte flag on a config singleton vs the directory field) is functionally equivalent enable semantics and is registered as deliberate delta D-105 in docs/PARITY-DELTAS.md; the **Folder-scan naming variant** (N-105's `L"Folder"` suffix) has no trigger surface in the core (no folder-scan mode) and is registered as deferral D-106. The write path itself (tmp+finalize+rename, atomic overwrite) was already sound and is untouched.
