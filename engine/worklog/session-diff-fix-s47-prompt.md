# Session 47 (S47) — the diff-to-fix parity-fix lane (SEVERE-first)

**Opened:** 2026-10-29 (utc+8) · **Lane:** S47 · **Operator:** the fixing agent per the user's mandate.
**Core at open:** `fa654673` (HEAD of main; the audited pin for the live board was `869da779`/`b26017a7` per
finding lineage — every finding is re-verified at the CURRENT tree before classification).
**Corpus at open:** `easeus-drw-research` @ `be0cfe9` (read-only ground truth).

## Mandate (token redacted)

Execute `diff-to-fix/`: for every finding (ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN),
independently verify (reproduce → re-cite corpus → classify CONFIRMED / ALREADY-FIXED / INVALID / SUPERSEDED),
then fix with tests (fail-before/pass-after), record the Resolution in the finding file, move it to
`findings/FIXED/`, update CROSS-REF-MATRIX / SUMMARY / modules / FIXED-LEDGER / docs, and push one
commit per finding. Rules: re-implement from understanding, never paste decompiled code; fix the cause;
no stubs/mocks/TODOs; safety hardenings preserved; pull frequently, never force-push, keep main green;
corpus wins over finding text; false findings marked INVALID with evidence, not "fixed" by breaking good code.

Board at open (filesystem truth, recounted this session): **1 ACUTE (A-014) · 9 SEVERE
(S-040 S-050 S-055 S-056 S-057 S-058 S-059 S-060 S-061) · 28 MAJOR · 36 MINOR · 7 INFO active; 64 FIXED; 6 tracked-open.**

## Session plan (SEVERE-first per the user's directive)

T1 pull+record ✅ · T2 read diff-to-fix ✅ · T3 read core ✅ · T4..T19 the verify→classify→fix loop in
priority order · T20 close-out: board docs, handoff.

## Worklog (chronological)

- **[T1]** Repos pulled; core at `fa654673`, corpus at `be0cfe9`. Audited-commit lineage recorded. Mandate saved.
- **[T2/T3]** diff-to-fix/ read (README, SUMMARY, all live finding files, module docs); core tree walked.
  Baseline build + full test suite next.
