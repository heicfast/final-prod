# Session prompt — the S80 diff-to-fix mandate (token redacted)

Saved at session open, 2026-10-29. The GitHub token supplied in the
original prompt is redacted here per the token-hygiene rule (never
committed, never logged).

## The mandate (verbatim summary)

Senior systems engineer mandate: execute the roadmap in
`core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR,
MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the
finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. The
audit is a hypothesis; this session is the verifier. Two repositories
only: core (`heicfast/core_recovery`, modified) and the corpus
(`heicfast/easeus-drw-research`, read-only ground truth).

Key SOPs: re-implement from understanding (never paste decompiled code);
fix the cause not the symptom; every fix paired with a test that fails
before and passes after; safety hardenings preserved; no stubs, no
mocks, no TODOs; one finding, one verified fix, one push; pull
frequently, never force-push, leave main green; the corpus wins over
the audit text; commit messages cite the finding ID, the divergence,
the fix, the test, and the corpus artifact.

Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL →
TRACKED-OPEN. Performance fixes measured, not guessed. Windows direct
I/O in 4 KiB multiples, 64 KiB where possible. CI green on Linux and
Windows; Win7-inclusive import gate intact; signing stays disabled.
Target: 20+ fixes this session.

## Session open state

- Core HEAD: `04fb5e88` (the S69 audit lane on top)
- Corpus HEAD: `be0cfe9`
- Board at open: 32 active (2 SEVERE · 17 MAJOR · 12 MINOR · 1 INFO) /
  169 FIXED / 4 tracked-open (per the S79/S69 lane blocks)
- The next free IDs: S-069+, M-155+, N-217+, I-049+, A-016+, D-150+

## CRITICAL at open: main is RED

The `ed501caa` push (the S71 X3F landing) committed UNRESOLVED merge
conflict markers in `src/core/carve/cr_analysers.cpp` (:5024/:5093/:5125)
AND in `diff-to-fix/findings/MAJOR/M-107-*.md`, with a false
"suite 1570/1570" claim in the commit message — the tree cannot compile.
The M-120/N-210 lint would have caught it; the gate was not run before
that push. Resolution decided first-hand against the corpus this session
(see M-155 and its fix commit): the S71 lane's FileX3FAnalyser
(FUN_1800535b0, SECd + 0x4C, loop-exit salvage) is the size authority —
the M-090 DMG precedent (the File*Analyser secondary of the dual
registration is the walk authority) + the full first-hand read of
FUN_1800535b0's three found-branches; the S78 lane's count-walk decode
(FUN_1800404a0, the Brance streaming callback) is superseded.
