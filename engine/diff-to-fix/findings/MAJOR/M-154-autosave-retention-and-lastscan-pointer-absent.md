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
