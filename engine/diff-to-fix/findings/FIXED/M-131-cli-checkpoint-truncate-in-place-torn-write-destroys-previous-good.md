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

## Resolution

**Status:** FIXED-ALREADY
**Fixed in commit:** 4f0fc160 (the parallel lane's A-013 landing — "the resume-without-out flow never truncates the session source (the N-014/CkSink atomic shape at the CLI)") — verified current at this lane's HEAD `c98eb7a7`; the M-131 scope was fixed by that landing in full
**Verified by:** the reproduction at HEAD: `src/app/rrecov.cpp:991-1058` — the `CliCkSink` (the N-014/CkSink shape adopted at the CLI layer): every emit stages into `<path>.tmp` (`cli_ck_write_cb`/`tell`/`seek` on the STAGING handle), the commit closes the staging handle and replaces the final path by `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` (`cli_ck_commit_cb`), then re-opens the tmp fresh — **the sink never opens the final path for truncation**; `src/core/orchestrator/cr_scan.cpp:705-739` (`emit_checkpoint_`): the seek-0 rewind + the complete emit + the `cfg_.checkpoint.commit` hook (the tmp+commit shape, the torn-emit comment at :734-735); the acceptance test green at HEAD: `CrUi.CliResumeSinkNeverTruncatesTheSessionSource` (test_cr_ui.cpp:4160+, "the A-013 atomic checkpoint sink (the N-014/CkSink shape)") + the N-014 staging pins (test_cr_ui.cpp:2814+); suite 1187/1187 green at this lane's HEAD.
**Resolution notes:** M-131's whole scope — the `"wb"` truncate-in-place on the final path (the old rrecov.cpp:1574-1583), the seek-0 overwrite per emit with no commit hook (the old cr_scan.cpp:585-600), and the `--out`-over-a-prior-session destroy-at-scan-start leg — is closed by the A-013 landing's `CliCkSink`: the prior session at `--out` survives byte-identical until the first SUCCESSFUL checkpoint replaces it wholesale (the og's fresh-file-per-save crash-safety model, the finding's own corpus reference), and a crash mid-emit leaves the final path holding the last COMPLETE checkpoint (the staging tmp is the only torn artifact). The fix plan's step-3 defense-in-depth (A-013's read-before-sink ordering guard) landed with the same commit (the test's resume-flow assertion). The finding's classification as the "general integrity defect of the same wiring" is confirmed; the specific shape it demanded (tmp+commit+rename, reusing the final-save helpers) is exactly what landed. No further code change needed; this Resolution records the verification per §2's ALREADY-FIXED class.
