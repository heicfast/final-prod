# SEVERE S-057 — the og auto-saves STOPPED scans' partial results; the core never auto-saves a cancelled run — a user who stops a long scan keeps the results in the og and loses them in the core

**Severity:** SEVERE
**Module:** session (auto-save) / orchestration (scan lifecycle)
**Parameters affected:** session lifecycle (36), scan lifecycle (31), cancellation (32), checkpoint (34), progress semantics boundary
**Corpus references:** stop-organize path "Organizing Found Files" / "Scan stopped and found %1 files." + `FreezeScanStopArbitrate` (DRWUI_decompiled.c:1549399-1549421, case 3; the completed twin case 2 at :1549360-1549390); "Build tree finish." → completion handler chain (`FUN_1401d37d0` :339076, call at :339078-79 → `FUN_140395e50` :612479); the auto-save block :612572-587 with `FUN_1402857c0` call at :612583 gated ONLY on not-import (`m_bImportResult` :612571-572) + results-found (`FUN_1403964b0` :612573-576) — no stopped/cancelled check; the early-finalize twin `FUN_14039b470` (:615148-166); user-facing contract "Remaining data can be rescanned and recovered anytime later" (UIUX/35:182)
**Core references:** `src/ui/cr_ui.cpp:2147-2148` (`if (rc != OK) return ret_(rc, ...)` precedes the completion auto-save block at :2166+), the comment :2152-2156 asserting "a cancelled run never does [auto-save] - the og's flow is the completion window"; `src/app/rrecov.cpp:1831-1838` ("on run() completion only - never a cancelled run"); S-044's own evidence conceding no cancelled-never gate in the corpus (`diff-to-fix/evidence/decompiled-quotes/s8-autosave-and-stamps.md:18-21`); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

In the og, pressing Stop does not discard the scan: the stop path runs its own organize
phase ("Scan stopped and found %1 files."), feeds the same build-tree/completion handler
as a finished scan, and that handler's auto-save gates (not-import, results-found,
SD/NAS) contain no stopped-check — so stopped scans' partial results land in SaveScan as
`auto_<vol>_<ts>.rsf` and appear in the "Continue last scanning" surface. In the core, a
cancelled run returns `ERR_CANCELLED` before the completion auto-save block ever runs
(cr_ui.cpp:2147-48), and the CLI's auto-save is likewise completion-only — a user who
stops a long scan (without `--out`) loses everything.

## What the reference does

The stop path (DRWUI case 3, :1549399-1549421):

```c
1549400    local_198 = QObject::tr(...,"Organizing Found Files",0);
1549404    local_180 = QObject::tr(...,"Scan stopped and found %1 files.",0);
1549419    QString::QString(local_2c0,"FreezeScanStopArbitrate");
```

The organize completes → "Build tree finish." (`FUN_1401d37d0` :339076) → the shared
completion handler:

```c
339078    if (*(longlong *)(param_1 + 8) != 0) {
339079        FUN_140395e50(*(longlong *)(param_1 + 8));
```

Inside `FUN_140395e50`, the auto-save block at :612572-587:

```c
612571    FUN_140162e00("m_bImportResult=%d\r\n",*(undefined1 *)(param_1 + 0x1c8));
612572    if (*(char *)(param_1 + 0x1c8) == '\0') {          /* not-import */
612573        ... FUN_1403964b0(param_1,&local_220,&local_1d8);  /* results found? */
612576        if (local_220 != 0) {
...
612583                FUN_1402857c0(local_158);              /* the auto-save */
612584                *(undefined1 *)(param_1 + 0x388) = 1;  /* already-saved flag */
```

No stopped/cancelled/reason check exists in the gate chain — the handler cannot
distinguish finish from stop. The CStopScanThread stop passes reason 0
(`FUN_1406a6ff0` :1096667-75 → engine-mgr vtable+0x28(0)), leaving the `+0x6c` gate at 0 —
the completion handler runs for user stops. The UI contract makes the persistence
user-visible: "Remaining data can be rescanned and recovered anytime later"
(UIUX/35:182).

## What our core does

`src/ui/cr_ui.cpp` — the cancel path returns before the auto-save block:

```c
2147        if (rc != OK)
2148            return ret_(rc, "scan %s", err_text(rc));
2149        /* W1c (R-52): the completion auto-save - ...
2152         * ONLY a completed run auto-saves (a cancelled run never does - the og's
2153         * flow is the completion window, and save-on-cancel is the explicit --out/
2154         * cr_ui_scan_save_session surface). ... */
2166        if (s->cfg.autosave_dir[0] != 0 && !s->is_resumed && !s->user_saved &&
2167            (s->stats.files_found != 0 || s->stats.lost_partitions != 0)) {
```

A cancelled run (`rc == ERR_CANCELLED`) exits at :2148 — the S-044 gate block at :2166+
(equivalent to the og's not-import/results-found gates) is unreachable for it. The CLI
mirrors this (rrecov.cpp:1831-1838, completion only; the `--out` save-on-cancel is a
separate, explicit-sink surface). The comment :2152-2156 asserts the og's flow is
completion-window-only — the stop-organize decode above contradicts it.

## Gap

A stopped scan with results found: og → auto-saved session in SaveScan (resumable,
listed); core → nothing persisted at the cr_ui layer (and at the CLI layer without
`--out`). Falsifiable: stop a scan after files are found, with `autosave_dir` set — the
og writes `auto_*.rsf`; the core writes no file and `stats.autosaved` stays 0.

## Impact

User-observable data loss by divergence: the og user who stops a 2-hour scan at 500 K
files keeps them; the core user re-scans from zero. The core's `--out` save-on-cancel
mitigates only for CLI users who passed an explicit session sink — the auto-save surface
the og exposes (and the core's own S-044 landing implements for completions) silently
omits the stop window. The comment's false og-claim also misleads future sessions.

## Reproduction / verification

Verified first-hand this session (lead auditor): read DRWUI:1549399-1549421 (stop case),
:339076-79 (the build-tree-finish → handler call), :612571-587 (the auto-save gates —
quote above: not-import + results-found only); `FUN_140395e50`'s only other callers
checked (`grep FUN_140395e50(` → :339079 single call site; `FUN_1402857c0` call sites
:612583/:615166/:616764 all inside completion-path handlers). Core side read directly
(cr_ui.cpp:2147-2166). S-044's evidence file concedes the corpus shows no
cancelled-never gate (s8-autosave-and-stamps.md:18-21).

## Fix plan (for the engineering team)

1. Land stop-window auto-save: in the cancel path, when `rc == ERR_CANCELLED` and the
   S-044 gates pass (results found, not-import, not-already-saved, autosave_dir set),
   run the same auto-save routine as the completion path (the organize/tree at cancel
   time is already coherent — the tree is live).
2. Files to modify: `src/ui/cr_ui.cpp` (the cancel branch before :2148), reusing the
   existing auto-save block (factor the :2166+ block into a helper called from both
   paths); CLI parity decision: rrecov's `--autosave-dir` flow gains the same stop-window
   behavior (the cancel handler at :1761).
3. Keep the results-found gate exactly (the og never writes a nothing-found session).
4. Tests: below.
5. Contract/doc: the :2152-2156 comment rewritten to the decoded og truth (stop-organize
   → shared handler, no stopped gate); SUMMARY/module session.md note.
6. Acceptance criteria: cancel-after-found with autosave_dir set → an auto_*.crsf
   appears (naming per N-061 convention), `stats.autosaved == 1`, and the session loads
   with the partial tree; a cancel-with-zero-found writes nothing (og parity).

## Tests to add

- `CrUi.CancelledRunWithResultsAutoSaves` (fails today: no file, autosaved==0)
- `CrUi.CancelledRunZeroFoundWritesNothing` (the results-found gate holds)
- `CrUi.CancelledAutoSaveLoadsPartialTree` (session round-trip of the stop window)
- `Cli.AutosaveDirStopWindowWrites` (rrecov parity leg, if taken)

## Related findings

- A-013 (the resume path that would consume the stop-window session — fix together)
- M-131 (checkpoint integrity for the same CLI wiring)
- S-044 lineage (the completion-window gates this extends to the stop window)
- N-135 residual (the doc-truth family the :2152 comment belongs to)

## Open questions

One decode residual (does not change the verdict): the `+0x6c` stop-reason gate at
:612528-530 — the stop thread passes 0, leaving the handler running; a deeper probe of
`FUN_1401ae610`'s writer would close the alternative that some stop class sets it. The
three auto-save call sites and the stop-organize chain are already sufficient evidence
for the divergence.
