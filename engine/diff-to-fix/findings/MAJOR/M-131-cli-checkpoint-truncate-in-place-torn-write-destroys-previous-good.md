# MAJOR M-131 — the CLI's in-run checkpoint sink is the truncate-in-place shape N-014 fixed in cr_ui: a crash mid-emit destroys the previous good checkpoint, and `--out` over a prior session truncates it at scan start

**Severity:** MAJOR
**Module:** cli (rrecov) / session checkpoint integrity
**Parameters affected:** checkpoint (34), error recovery (35), safety — data-integrity class
**Corpus references:** the og writes every save to a FRESH timestamped file (`auto_` + ts composer, DRWUI:442898-900; the three call sites :612583/:615166/:616764) — a torn new file never destroys the previous auto-save (the registered crash-safety model, N-014 lineage: "the og's fresh-file-per-save crash-safety")
**Core references:** `src/app/rrecov.cpp:1574-1583` (`"wb"` on the FINAL path), `src/core/orchestrator/cr_scan.cpp:585-600` (`emit_checkpoint_` seek-0 + overwrite, no commit hook), the fixed cr_ui twin `src/ui/cr_ui.cpp:2078-2092` (`CkSink` tmp+rename, pinned by `tests/unit/test_cr_ui.cpp:2650-2656` whose comment describes this exact pre-fix shape); the CLI's own final-save already uses tmp+rename (`rrecov.cpp:1731-1757`); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The CLI's checkpoint sink opens the FINAL `--out` path with `"wb"` (truncating any prior
session there, before the scan even starts) and each stage-boundary emit seeks to 0 and
overwrites in place — no tmp+commit. A crash mid-emit leaves a torn container with the
previous good checkpoint already overwritten. The cr_ui layer fixed this exact shape
(N-014: `CkSink` stages `.tmp` and renames); the CLI's final save and save-on-cancel use
tmp+rename too — only the in-run checkpoint kept the torn shape. A-013 is the acute
special case where the truncate victim is the resume target; this finding is the general
integrity defect of the same wiring.

## What the reference does

Every og save path creates a fresh timestamped file (composer cites above); no og save
or auto-save ever opens an existing session for in-place truncation. The registered
crash-safety model (N-014's corpus decode) is "fresh-file-per-save": a torn write
produces one bad NEW file; every prior save survives.

## What our core does

`rrecov.cpp:1574-1583`: `_wfopen_s(&ck_file, effective_out.c_str(), L"wb")` on the final
path. `cr_scan.cpp:585-600` (`emit_checkpoint_`): seek(0) + overwrite per emit, no
commit/rename hook. The in-code comment (rrecov.cpp:1569-1573) concedes "a torn one is
no worse than no checkpoint — the reader re-indexes forward by construction", but a torn
OVERWRITE of a previously-good checkpoint IS worse than no checkpoint: the reader
re-indexes forward over a corrupt mixture (half new, half stale) that may parse to a
wrong partial state or fail outright. The cr_ui twin's test
(test_cr_ui.cpp:2650-2656) documents the pre-fix shape this CLI block still carries.

## Gap

The CLI checkpoint lacks the tmp+commit shape its own cr_ui layer landed as N-014.
Falsifiable: run `scan --out S.crsf`, kill the process during a stage-boundary emit →
S.crsf is torn (previous good checkpoint destroyed); with the CkSink shape it would hold
the last complete checkpoint.

## Impact

- Crash/power-loss mid-emit destroys the previous good checkpoint → resume salvages less
  than the last committed stage, or fails (the A-013 flow compounds: the resume then
  truncates what's left).
- `--out` pointing at an existing session (overwrite intent) destroys it at scan START,
  before any new checkpoint exists — a crash in the first minutes leaves nothing.
- Inconsistent with the core's own cr_ui contract (two layers, two integrity shapes).

## Reproduction / verification

Core-derived (lead auditor read :1574-1583 + the cr_scan emit shape + the cr_ui CkSink
contrast; the test comment at test_cr_ui.cpp:2650-2656 names this exact pre-fix shape).
Corpus contrast (the fresh-file composer cites). No execution needed — the file-mode
semantics are deterministic.

## Fix plan (for the engineering team)

1. Adopt the CkSink shape in the CLI: open `<effective_out>.tmp`, emit checkpoints
   there, rename over the final path after each complete emit (the cr_ui.cpp:2078-2092
   pattern; the rename is atomic on both Windows and POSIX).
2. Files: `src/app/rrecov.cpp` (the :1574-1583 block) — reuse the tmp+rename helpers
   from the final-save path (:1731-1757).
3. This fixes A-013's truncate leg simultaneously (the resume target is never opened
   "wb" directly); A-013's read-before-sink ordering guard should STILL land as defense
   in depth.
4. Tests: below.
5. Acceptance criteria: kill-during-emit (SIGKILL at the emit window) leaves the file
   holding the last COMPLETE checkpoint (assert parseable + stage mask == last
   committed); a pre-existing file at `--out` is untouched until the first checkpoint
   commits.

## Tests to add

- `Checkpoint.CrashMidEmitKeepsPreviousGoodCheckpoint` (inject a kill between seek(0)
  and write completion; parse the file — fails today)
- `Checkpoint.OutPathPreexistingFileNotTruncatedAtStart` (fails today)
- `Checkpoint.EveryEmitCommitsAtomically` (watcher observes only complete files)

## Related findings

- A-013 (the acute resume-target special case — fix together)
- S-057 (the stop-window auto-save that would write through this sink)
- N-014 lineage (the CkSink landing to reuse)

## Open questions

None.
