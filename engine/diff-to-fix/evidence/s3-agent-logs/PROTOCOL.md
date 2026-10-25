# S3 PROTOCOL — shared rules for session-3 deep-pass agents (READ FIRST, THEN YOUR TASK PROMPT)

**Session:** 3 (continuation diff-audit). **Core HEAD:** `34b3441` (repo at
`/home/z/my-project/audit/core_recovery`). **Corpus HEAD:** `03ad46c` (repo at
`/home/z/my-project/audit/easeus-drw-research`). All file paths below are relative
to those two roots. Prior sessions filed 45+ findings — **read your module file(s)
under `diff-to-fix/modules/` BEFORE filing anything, and never re-file an existing
finding** (IDs and coverage are listed there; also check
`diff-to-fix/findings/FIXED/RESOLVED-INDEX.md` for closed items).

## 1. Read-only discipline (non-negotiable)

- You do NOT modify any source code, header, test, build file, workflow, or existing doc.
- You may write ONLY:
  1. New finding files: `diff-to-fix/findings/<SEVERITY>/<ID>-<slug>.md`
  2. New evidence files: `diff-to-fix/evidence/decompiled-quotes/s3-<slug>.md` and
     `diff-to-fix/evidence/executions/s3-<slug>/…` (fixtures + logs for anything you execute)
  3. NEW-module verdict rows APPENDED to `diff-to-fix/modules/<module>.md` (append only,
     never rewrite existing rows; mark new rows `**(S3)**` so the consolidator can merge)
  4. Your scratch report: `diff-to-fix/evidence/s3-agent-logs/<your-task-id>.md`
- Do NOT `git commit`, `git push`, or touch `worklog.md` (the lead consolidates).
- Do NOT modify the corpus repo at all — it is read-only ground truth.

## 2. Verification protocol (evidence or nothing)

Every claim about the og (reference) must be established by ONE of, in strength order:
1. **Executed comparison** — build/run the core CLI or unit pieces on a fixture you
   construct; store fixture + commands + output under `evidence/executions/s3-…/`.
   (cmake+ninja+ASan build is fine; the suite is known green 317/317 at HEAD.)
2. **Decompiled evidence** — quote the exact snippet from
   `re-workspace/reversed/ghidra-*/<DLL>_decompiled.c` with `file:line` (the .c files are
   line-numbered by Ghidra export; cite the line you see in the editor) and, when
   possible, the function name (`*_functions.txt` indexes) + virtual address.
3. **Corpus documentation** — `architecture/` docs, quoted with file + section.
4. **Independent RE probe** — `strings`/`objdump`/python PE-walks on the binaries in
   `og-codebase/` (e.g. `og-codebase/PartitionRecovery/*.mo`, the SearchDLLs,
   `DRW.exe`). Cite the command + the bytes/strings found.

If none of these can establish og behavior, do NOT guess: record it in your scratch
report as `UNVERIFIED: <precise question> + <the corpus artifact to examine next>`.
**A false finding costs the team more than a missing one.** Known corpus-doc errors are
cataloged in `diff-to-fix/CORPUS-ERRATA.md` — the decompiled binaries beat the docs.
Claiming core behavior: cite `file:line` you read yourself at HEAD `34b3441`.

## 3. Verdicts

MATCH / DIVERGENCE-ACUTE / DIVERGENCE-SEVERE / DIVERGENCE-MAJOR / DIVERGENCE-MINOR /
DIVERGENCE-INFORMATIONAL / UNVERIFIED. Definitions: `diff-to-fix/SEVERITY-SCALE.md`
(read it). Severity discipline: ACUTE = wrong recovery results / silent data loss /
crashes / security regressions ONLY. INFORMATIONAL = deliberate documented superset or
scope exclusion. Mark deliberate supersets INFO, not MAJOR.

## 4. Finding file template (MANDATORY, exact structure)

```markdown
# <SEVERITY> <ID> — <One-line title>

**Severity:** ACUTE / SEVERE / MAJOR / MINOR / INFORMATIONAL
**Module:** <engine/module>
**Parameters affected:** <parameter IDs from diff-to-fix/PARAMETERS.md>
**Corpus references:** <file paths + line/address in RE corpus>
**Core references:** <file paths + line/symbol in core_recovery @ 34b3441>
**Status:** READ-ONLY FINDING — no fix applied

## Summary
<2–4 sentences.>
## What the reference does
<In-depth, every claim cited. Quote decompiled snippets exactly with file:line.>
## What our core does
<In-depth, every claim cited file:line.>
## Gap
<Falsifiable claim.>
## Impact
<Recovery correctness? Performance? API contract? User-observable?>
## Reproduction / verification
<Commands + outputs, or citations, or both.>
## Fix plan (for the engineering team)
<Numbered, actionable: architectural change, files to modify, algorithm/constants,
tests to add, contract/doc updates, acceptance criteria.>
## Tests to add
<Concrete test names/scenarios that fail before, pass after.>
## Related findings
<IDs + short reason.>
## Open questions
<Or "None.">
```

## 5. ID ranges (pre-assigned to avoid cross-agent collisions — use the NEXT FREE ID in YOUR range, in order)

| Agent | Range (in order of use) |
|---|---|
| S3-CARVE | S-012, S-013, M-011, M-012, N-023, N-024, N-025, N-026, I-009 |
| S3-LOST  | S-014, M-013, M-014, N-027, N-028, I-010 |
| S3-REFS  | S-015, M-015, M-016, N-029, N-030, N-031, I-011 |
| S3-NTFS  | S-016, M-017, M-018, N-032, N-033, N-034, N-035, I-012 |
| S3-EHB   | S-017, S-018, M-019, M-020, N-036, N-037, N-038, N-039, I-013 |
| S3-FEZL  | S-019, S-020, M-021, M-022, N-040, N-041, N-042, N-043, I-014 |
| S3-ORCH  | S-021, M-023, M-024, N-044, N-045, N-046, N-047, I-015 |
| S3-CLIABI| S-022, M-025, N-048, N-049, N-050, I-016 |

If you find more divergences than IDs in your range, file the most important ones and
list the rest in your scratch report as candidates (the lead assigns follow-up IDs).

## 6. Your scratch report (`diff-to-fix/evidence/s3-agent-logs/<task-id>.md`)

Required sections: (a) what you read (files + line ranges, so coverage stats are
honest); (b) findings filed (ID, title, 1-line gap, evidence type); (c) parameters
newly verified MATCH (parameter ID + 1-line evidence — these update the matrix);
(d) UNVERIFIED residuals (precise question + next artifact); (e) verdicts appended to
which module files. Keep it dense and factual — the lead turns it into the worklog.

## 7. Reference indexes you will need

- Core source: `src/core/<module>/`, `include/cr/`, tests `tests/unit/`, CLI `src/app/rrecov.cpp`,
  UI `src/ui/cr_ui.cpp`, win layer `src/win/`. Docs: `docs/` (CAPABILITY-MATRIX, ABI-CONTRACT).
- Corpus decompiles: `re-workspace/reversed/ghidra-searchdll/<Engine>_decompiled.c` +
  `_functions.txt` (symbol→address index) + `_imports.txt` (import census).
  Also `ghidra-partition/` (RecoveryModule, PartitionRecovery, RawFixer, Boot, BitLockerLib,
  CloneModule, MainModule…), `ghidra-mo/` (Device, Data, NtfsLib-class .mo modules…),
  `ghidra-core/` (DRW, DRWReport, DRWService), `ghidra-repair/` (file-repair modules).
- Corpus docs: `architecture/` (flow/state docs), `re-workspace/` artifacts
  (`.rsf` session dumps, QA logs, driver references), `og-codebase/` (installed product tree).
- Prior-session evidence to BUILD ON (don't redo): `diff-to-fix/evidence/decompiled-quotes/`,
  `diff-to-fix/evidence/executions/`, `diff-to-fix/tracked-open/`.
