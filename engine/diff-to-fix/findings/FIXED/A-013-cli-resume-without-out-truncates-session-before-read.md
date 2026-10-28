# ACUTE A-013 — the CLI's `--resume`-without-`--out` flow truncates the session file BEFORE reading it: the printed resume command destroys the user's only session copy

**Severity:** ACUTE
**Module:** cli (rrecov) / session checkpoint wiring
**Parameters affected:** session lifecycle (36), checkpoint (34), error recovery (35), cancellation (32), safety — silent data loss class
**Corpus references:** n/a for the defect itself (core-internal). Contrast: the og's import path opens the .rsf read-only (CRSDataEx reader chain, rsf-format §4, `FUN_14051b0b0`) and never truncates the source; the og writes every auto-save to a FRESH timestamped file (`auto_` + ts, DRWUI:442898-900; `\auto_` name composer `FUN_1402857c0` call sites :612583/:615166/:616764)
**Core references:** `src/app/rrecov.cpp:1565-1583` (the truncate), `:1596-1617` (the later read), `:1761-1769` (the save-on-cancel message printing the destructive command) — audited at core `8d0d88b8` (content; lineage `869da779`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When `rrecov scan` runs with `--resume <file>` and no `--out`, the CLI computes
`effective_out = resume_path` and immediately opens that path with mode `"wb"` as the
checkpoint sink — truncating the session file to zero bytes — BEFORE the resume read opens
the same path with `"rb"`. Every in-place resume therefore reads an empty container, fails,
and the interrupted scan's results are permanently destroyed. The save-on-cancel flow prints
exactly this command (`--resume <file>` without `--out`) as the documented recovery path.

## What the reference does

The og has no CLI; its session flows never truncate a source. The .rsf import path is
read-only (`FUN_14051b0b0`, the CRSDataEx reader chain per `rsf-format` §4), and every
auto-save writes a fresh timestamped file `auto_<vol>_<M>_<D>_<H>_<m>_<s>.rsf`
(DRWUI:442898-900, composer cited at the three `FUN_1402857c0` call sites :612583,
:615166, :616764), so no save or resume path can destroy a prior session. The
session-1-era crash-safety decode (N-014 lineage) records this as "the og's
fresh-file-per-save crash-safety".

## What our core does

`src/app/rrecov.cpp`:

```c
1565  /* session output: --out explicit; --resume without --out updates the
1566   * resumed file in place */
1567  const std::wstring effective_out = out_path.empty() ? resume_path : out_path;
...
1574  FILE* ck_file = nullptr;
1575  if (!effective_out.empty()) {
1576      if (_wfopen_s(&ck_file, effective_out.c_str(), L"wb") != 0 || !ck_file) {
```

The `"wb"` open truncates `effective_out` — which IS `resume_path` when `--out` is absent.
Only afterwards does the resume read run:

```c
1596  if (!resume_path.empty()) {
1598      if (_wfopen_s(&fin, resume_path.c_str(), L"rb") != 0 || !fin) {
```

The comment at :1565-1566 states the intended contract ("updates the resumed file in
place") — but the implementation truncates before reading, so nothing is ever resumed
in place. The save-on-cancel path prints the destructive command as the recovery path
(rrecov.cpp:1761-1769: `session saved (cancelled): %s` followed by the
`--resume <file>` invocation with no `--out`). Mechanism verified this session: a
non-empty file opened `"wb"` then `"rb"` reads as 0 bytes (trivial C semantics; the
source order makes it real).

## Gap

`rrecov scan --resume F.crsf` (no `--out`) zeroes F.crsf at :1576 before the :1598 read.
The in-place-update contract stated at :1565-1566 is not implemented.

## Impact

Silent, total loss of the user's only copy of an interrupted scan's session — triggered by
following the program's own printed instruction after a cancel. Anyone who cancels a long
scan (saved via `--out`), then runs the printed resume command, loses everything. This is
the highest-severity class in the audit's scale (silent data loss on the documented path).

## Reproduction / verification

Executed this session (mechanism level): create a 1000-byte file, open `"wb"` then `"rb"`
in the same order as rrecov.cpp:1576/:1598 — the `rb` stream reads 0 bytes. Source-level
verification: the checkpoint wiring block (:1569-1583) textually precedes the resume-read
block (:1596-1617) in the same function; `effective_out` aliases `resume_path` whenever
`out_path` is empty (:1567). No CLI end-to-end resume test exists (`tests/` carries unit +
ui_abi_check only; the CI junk-gate step runs scan/tree/recover, not resume-in-place).
Corpus contrast: og auto-save call sites + fresh-file naming (cites above) — the og
cannot exhibit this loss.

## Fix plan (for the engineering team)

1. Open the checkpoint sink on `<effective_out>.tmp` and commit by rename after each
   emit — i.e. adopt the cr_ui `CkSink` shape (cr_ui.cpp:2078-2092, the N-014 landing),
   which already solved this exact defect at the ABI layer; OR defer the ck-sink open
   until after a successful resume read (read first, then open the sink).
2. Files to modify: `src/app/rrecov.cpp` (the :1565-1583 block), reusing the existing
   tmp+rename helpers from the final-save path (rrecov.cpp:1731-1757 already uses
   tmp+rename for the final save and save-on-cancel — only the in-run checkpoint kept
   the torn shape).
3. Add an explicit guard: if `effective_out == resume_path`, never open the sink before
   the resume read returns OK.
4. Tests to add (see below) — an end-to-end CLI test that cancels, resumes in place,
   and asserts the tree survives.
5. Contract/doc: the `--resume` in-place semantics line in rrecov.cpp's header comment
   block (:35, :1407) stays truthful after the fix.
6. Acceptance criteria: `scan --out F` → cancel → `scan --resume F` (no `--out`)
   reproduces the pre-cancel tree with stage skipping; F is never observed as 0 bytes at
   any point in the sequence (assert on file size across the boundary in the test).

## Tests to add

- `Cli.ResumeInPlaceDoesNotTruncateSession` (fail-before: file is 0 bytes after the
  command starts; pass-after: byte-identical prefix + appended checkpoint)
- `Cli.ResumeWithoutOutKeepsStageMask` — resume skips completed stages
- `Cli.SaveOnCancelPrintedCommandRoundTrips` — run the exact printed command; the tree
  must survive (this test fails today)
- Unit: `CheckpointSink.TmpCommitOnResumePath` (the sink never opens "wb" on the resume
  target before the read)

## Related findings

- M-131 (the same wiring's truncate-in-place checkpoint shape — this finding is the
  acute special case where the victim is the resume target)
- N-014 lineage (the cr_ui CkSink tmp+commit landing — the shape to adopt)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** <this push> (S42 lane — the ACUTE priority interrupt over the
MAJOR-first directive; both board ACUTEs jump the queue per the fix-order rule)

**Verified by:** `CrUi.CliResumeSinkNeverTruncatesTheSessionSource` (RED pre-fix —
proven by reverting rrecov.cpp to the pre-fix tree and re-running: the wiring
assertions find no sink, the destructive-pattern assertion finds the old
`_wfopen_s(&ck_file, effective_out...)` truncating open; GREEN post-fix) + the
Windows CI junk-gate step's new 3b block (the end-to-end round-trip: scan --out F →
resume-without-out on F → exit 0, F never 0 bytes, `tree --in F` still carries
DOC.TXT — the exe is Windows-only so the behavioral proof rides CI, the N-094
source-pin pattern); full cr_ui suite 68/68.

**Resolution notes:** The fix adopts the plan's option 1 — the N-014/CkSink shape
(cr_ui.cpp:334-389, the ABI layer's own solution to this exact defect class) at the
CLI: `CliCkSink` stages every checkpoint emit into `<path>.tmp` and commits by
`MoveFileExW(..., MOVEFILE_REPLACE_EXISTING)` rename (the staging handle closed
first — the sharing-violation note from the cr_ui landing), reopening a fresh
staging tmp for the next emit. The final path — which IS the resume file whenever
--out is absent — is never opened for truncation AT ALL: the resume read (unchanged,
"rb" on the final path) always sees the intact source, a failed resume leaves it
byte-identical, and the first successful checkpoint/final save replaces it
wholesale (the og's fresh-file-per-save crash safety, rsf-format §5.5, at the CLI).
The G-13 comment block rewritten to the atomic semantics; the sink's error paths
(resume-open failure, resume-load failure, post-run) close the staging file and
remove the tmp (no litter, no torn final state). The plan's alternative (defer the
sink open until after a successful resume read) was evaluated and rejected: the
orchestrator copies the checkpoint callbacks at controller construction inside
`ScanController::resume`, so the sink must be wired before the call — the
tmp+rename shape is the only ordering-proof fix, and it also covers the
crash-mid-emit and failed-resume cases the deferral does not. The
`--resume`-in-place contract line (rrecov.cpp:35/:1407) is now true. No safety
hardening was touched; the fix IS one (silent-data-loss class).
