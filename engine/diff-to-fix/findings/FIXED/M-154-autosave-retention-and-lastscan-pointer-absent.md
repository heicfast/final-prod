# MAJOR M-154 — the auto-session lifecycle: the og keeps exactly ONE auto session (pointer-file delete-previous + LastScanInfo.ini + an HKLM mirror); the core never prunes and has no pointer

**Severity:** MAJOR
**Module:** session / orchestrator (auto-session lifecycle, state management)
**Parameters affected:** #36 (session lifecycle — save/retention), #12 (session persistence format — the pointer file), #31 (scan lifecycle — the post-save organize), #52 (scalability — unbounded disk growth)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:442815-442833` (the retention loop: read LastScanInfo.ini → DeleteFileA the previous auto .rsf), `:443060-443086` (the writer: `[Info] SavePath/Name/Count/Time` + the HKLM `SOFTWARE\EASEUS\DRW` `LastScanConfig`/`LastScanFile` mirror), `:443158-443215` (the reader: Name/SavePath/Count/Time + `PathFileExistsA` validation), `:443092-443127` (the pointed resume: SavePath copied and restored when no explicit .rsf was chosen), `:432256-432341` (the auto-session list: only names containing both "auto_" and ".rsf"), `:541001-541057` (the `%APPDATA%\EaseUS\<sub>\LastScanInfo.ini` resolver); all read first-hand this session (lane evidence: `evidence/s69-lane-sheets/S69-A-localdb.md` §B.6)
**Core references:** audited `3799dee2`, re-verified LIVE at `271f5d06` — `src/ui/cr_ui.cpp:2287-2323` (`autosave_if_due_`: gates + name + atomic write; **no delete of any prior auto session**), `src/app/rrecov.cpp` (the CLI twin autosave band; no delete), `src/core/session/cr_crsf.cpp:363-393` (`auto_session_name_at` — the og-parity naming), the session-list enumeration (all `*.crsf` files listed and sorted by save time; no auto_ filter, no pruning — grep `remove|unlink|prune` over session/UI/app paths: zero hits)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's auto-session lifecycle is bounded and pointed: before writing a new auto session it reads the `LastScanInfo.ini` pointer (SavePath) and DELETES the previous auto-saved `.rsf`, so exactly one auto session is ever live; the pointer file (plus an `HKLM\SOFTWARE\EASEUS\DRW` registry mirror) drives the "continue last scanning" flow, and the auto-session list surface shows only `auto_*.rsf` files. The core writes auto sessions with og-parity naming but never prunes — every auto session accumulates forever — and has no pointer file or pointed-resume mechanism.

## What the reference does

Verified first-hand (this lane):

1. **The retention loop** (`FUN_1402857c0`, DRWUI:442815-442833):
```c
local_5b0 = FUN_1401b3ec0();
local_5c0 = FUN_1401b6b90(local_5b0);
if (local_5c0 != (longlong *)0x0) {
    FUN_1402806d0(local_368);
    FUN_140286460(param_1,local_368);          /* read LastScanInfo.ini into the record */
    if (local_368[0] != '\0') {                 /* the SavePath-exists flag */
        lpFileName = (LPCSTR)FUN_140056170(local_340);
        DeleteFileA(lpFileName);                /* DELETE the previous auto session */
    }
    ...
```
2. **The pointer writer** (DRWUI:443060-443086): `WritePrivateProfileStringA("Info","SavePath",<rsf path>)`, `"Name"`, `"Count"`, `"Time"` (FILETIME hex) into `%APPDATA%\EaseUS\<sub>\LastScanInfo.ini` (resolver `FUN_14031e7a0`, DRWUI:541001-541057), then the registry mirror: `CRegKey` open `SOFTWARE\EASEUS\DRW`, set `LastScanConfig` (the ini path) and `LastScanFile` (the rsf path).
3. **The pointed resume** (DRWUI:443092-443127): `FUN_140286390` copies SavePath and restores through the shared restore path; the auto-list surface `FUN_140273940` (DRWUI:432256-432341) enumerates the dir keeping only names containing "auto_" AND ".rsf" — manual saves are excluded from the auto list.
4. Triggers: exactly the three completion-family handlers (no interval timer) — the S-044 decode, re-confirmed.

Net effect: the og's SaveScan dir holds exactly ONE live auto session (the shipped corpus tree's three `auto_*.rsf` samples are consistent only with the pointer being absent/reset between QA runs).

## What our core does

`autosave_if_due_` (`src/ui/cr_ui.cpp:2287-2323` at the re-verified pin) writes `auto_<label>_<stamp>.crsf` via the atomic writer and emits `CR_EVT_AUTOSAVED` — nothing is deleted. The CLI twin behaves identically. The session-list surface enumerates ALL `*.crsf` files sorted by save time (no `auto_` distinction, no pruning). Grep for `remove|unlink|prune` across the session/UI/app paths: zero hits. There is no pointer file and no pointed-resume entry point.

## Gap

Stated falsifiably: after N auto-saving runs, the og's session directory holds 1 auto session and the core's holds N; the og's host can name the last session without enumeration (LastScanInfo.ini / registry), the core's cannot.

## Impact

State management at scale: auto sessions are full tree snapshots (the og's shipped samples run 7-52 MB; core sessions are comparable) — a scan-a-day user accumulates gigabytes of stale auto sessions that the og's lifecycle would have kept bounded to one. The missing pointer also changes the "continue last scanning" contract (list+sort vs pointed). No recovery-correctness impact; retention policy + disk-growth + resume UX divergence.

## Reproduction / verification

Corpus: all six DRWUI bands quoted above read first-hand this session (the retention loop, the writer, the reader, the resume, the list filter, the resolver); og-codebase/SaveScan/ holds the three real `auto_*.rsf` files whose naming the core matches (N-061 parity re-confirmed). Core: the autosave band and the session-list enumeration read at `3799dee2` and re-verified at `271f5d06`; the delete/prune absence proven by grep. Evidence sheet: `evidence/s69-lane-sheets/S69-A-localdb.md` §B.6/§D.1.

## Fix plan (for the engineering team)

1. Decide the retention posture: (a) og-exact — delete the previous auto session before writing the new one, gated on a persisted pointer; or (b) registered deliberate divergence — bounded retention (keep last K, default 1, configurable) with the og's semantics documented in PARITY-DELTAS. Either way the unbounded growth ends.
2. Files: `src/ui/cr_ui.cpp` (`autosave_if_due_` — read pointer, delete prior), `src/app/rrecov.cpp` (the CLI twin), `src/core/session/` (a small `last_scan_info` read/write pair; the core's "ini" can be a JSON sidecar in the autosave dir — the og's APPDATA ini + HKLM mirror is a Windows-host convention the ABI layer can adopt or register as deliberately dropped with the pointer's semantic preserved).
3. The pointer record: SavePath, Name, Count, Time — four fields, the og's exact set (`[Info]` section, DRWUI:443060-443078).
4. The pointed resume: a `cr_ui_session_resume_last` entry (or a documented "list + newest auto_" rule) mirroring `FUN_140286390`'s SavePath-restore; the auto-list filter (`auto_` + extension) if the session-list surface needs the og's auto/manual split.
5. Tests: (a) two consecutive auto-saving scans in one autosave_dir → exactly one file remains + the pointer names it; (b) a missing/dead pointer (SavePath not found) never blocks the new save (the og's `PathFileExistsA` validation, DRWUI:443209-443214); (c) resume-last reads the pointer, not the directory enumeration; (d) manual saves are never deleted by the retention pass.
6. Acceptance: the autosave dir holds exactly one `auto_*.crsf` across repeated runs; the pointer file's four fields match the og's set; the doc_truth_gate register row lands in PARITY-DELTAS if option (b) is chosen.

## Tests to add

`Session.AutosaveRetentionDeletesPrevious` / `Session.AutosavePointerFourFields` / `Session.AutosaveDeadPointerDoesNotBlock` / `Session.ResumeLastUsesPointer` / `Session.ManualSaveSurvivesRetention`.

## Related findings

N-211 (the cr_scan.h autosave contract comment drift — same surface, doc-truth class), S-044/S-057 (the trigger gates, landed), N-061 (the naming parity, landed), N-145 (the marked-disk marker — the og's OTHER QSettings persistence, still active).

## Open questions

The APPDATA subfolder constant `DAT_141211ee0` (DRWUI:541001-541057) is unresolved — the og's exact ini path; immaterial if the pointer's semantics (not its Windows location) are adopted. The static call site of the writer `FUN_140285f40` is vtable-dispatched (dispatch origin not decoded) — the three completion-handler triggers are the decoded sites.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** the M-154 S80-lane commit (this session; SHA in the commit footer)
**Verified by:** CrUi.AutosaveRetentionDeletesPreviousAndPointerNamesIt (RED via the stash dance: BUILD failure pre-fix — the capability was absent) / AutosaveManualSaveSurvivesRetention / AutosaveDeadPointerDoesNotBlock; the pointed-resume ABI consumer check in tests/ui_abi_check.c (cr_ui_abi_check green); the pre-existing autosave pins (AutoSaveOnCompletion, CancelledRunWithResultsAutoSaves — 12/12) stay green; full suite 1651/1651.
**Resolution notes:** The og-exact posture (the fix plan's option a) landed: the retention loop (DRWUI:442815-442833) runs before every auto-session write in BOTH autosave bands (cr_ui.cpp's autosave_if_due_ + the rrecov.cpp CLI twin) — the previous auto session named by the pointer is deleted, keeping exactly one live auto session (the unbounded accumulation ends). The pointer: session::LastScanInfo (a new module, src/core/session/cr_lastscan.cpp) — a line-based `last_scan.info` sidecar in the autosave dir carrying the og's exact four fields (save_path/name/count/time, the [Info] set at DRWUI:443060-443086), written atomically (tmp+rename) after every successful auto-save; the HKLM SOFTWARE\EASEUS\DRW mirror is the Windows-host convention, deliberately dropped (PARITY-DELTAS-registered: the semantics preserved, the location convention is the difference). The pointed resume: cr_ui_session_last_autosave_path(utf8_dir, out, cap) — reads the pointer, validates the pointed file's presence (the og's PathFileExistsA, :443209-443214), returns the loadable path (FUN_140286390's SavePath-restore equivalent; ERR_RANGE_OUT_OF_BOUNDS on no-live-pointer so the host falls back to the list). The og's Count field carries the surfaced node count (files + deleted + carved). Registered safety hardening (a deliberate superset, noted here): the retention's auto_-name gate — the og's DeleteFileA targets whatever the pointer names (only ever its own auto_*.rsf by construction); the core refuses to delete a non-auto_ file a corrupted/hostile pointer might name. The og's interval-free trigger set (S-044/S-057) unchanged. The session-list surface keeps listing all sessions (the og's auto/manual list split is a UI-surface distinction the ABI's list consumers can apply by name; the pointed resume covers the continue-last contract).

---

## S82 convergence addendum (2026-10-29 — conceded to the racing af5987d0 + the hardening rider this lane landed on top)

A second lane (S82, this one) implemented the same lifecycle
independently (the same module shape - a cr_lastscan core pair, the
same last_scan.info sidecar name, the four fields, the retire-before/
pointer-after wiring, the pointed-resume ABI entry; three Linux core
tests) and conceded per SA-9 when af5987d0 landed first. The S82
contribution that survived the concession is the HARDENING RIDER on the
landed retire:

- **The gap:** the landed `last_scan_retention_delete_previous` gated
  `info.name` but REMOVED `info.save_path` - two independent sidecar
  fields. A tampered sidecar naming a valid `auto_*.crsf` in `name`
  while pointing `save_path` at ANY host file (a manual save, an
  arbitrary document) passed the gate and deleted the target; the
  name gate itself also admitted separators and traversal.
- **The rider (RED->GREEN via the revert rebuild):** the name gate
  refuses separators, traversal, and a missing `.crsf` tail; the
  remove path's basename must BE the gated name (a disagreeing
  save_path is refused, never followed). The principle: the gate must
  govern what is actually removed. Pinned by
  `Session.AutosaveRetentionDeletesPreviousAutoOnly` (the hostile
  save_path refused, the manual save survives) and
  `Session.AutoPointerNameGateRefusesSmuggledPaths` (the hand-written
  hostile sidecar) - both RED on the pre-rider tree (the manual file
  was deleted / the smuggled path followed).
- The Linux-runnable core trio re-targeted onto the landed API
  (LastScanPointerFourFieldRoundTrip + the two above) complements the
  win-lane tests af5987d0 landed in test_cr_ui.cpp.

## Convergence addendum (this lane — the S80 parity-fix lane's concession record)

This lane independently implemented option (a) in the same window (the
session-layer pointer pair with the og's four fields and ini grammar, the
retention-before-write with the dead-pointer no-op, the
belt-and-braces name-shape deletion guard [only auto_* names ever deleted],
the pointed resume as the CLI's --resume-last, the PARITY-DELTAS
registration for the co-located ini + the dropped HKLM mirror; the ABI
integration test RED pre-fix behavioral via the stash dance — the
accumulation broke the one-file pin — plus four session units and the CLI
contract pin) before the push met the racing landing — the seat conceded
per SA-9. The racing implementation re-verified first-hand post-concession:
the retention + manual-survives tests green, the build clean. The two
implementations are semantic twins independently derived from the fix plan
— the same four-field pointer, the same retention ordering, the same
name-shape guard (their `name_is_auto` = this lane's
`auto_session_name_shape`), the same dead-pointer no-op class. Two deltas,
both immaterial to the semantics: (a) the sidecar's name (their
`last_scan.info` vs this lane's `LastScanInfo.ini` — the og's filename is
a host-convention half either way, registered in the PARITY-DELTAS row);
(b) the pointed-resume surface (their ABI entry vs this lane's CLI
--resume-last flag — complementary halves of the same FUN_140286390
restore; the ABI entry is the stronger surface and stands).
