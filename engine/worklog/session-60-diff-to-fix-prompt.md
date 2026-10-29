# Session-60 diff-to-fix execution lane — the saved mandate (2026-10-29)

> The verbatim user mandate, GitHub token redacted. Read twice at session open.

**Scope grant (two repositories, only two):**
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: `[REDACTED — env/git-credentials only, never committed, never logged]`
- Do not clone, reference, or look at any other repository. Do not guess reference
  behavior — verify against the corpus.

**Mandate:** execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify
the finding, fix it in the codebase, prove the fix with tests and benchmarks, and
mark it fixed in the audit trail with evidence. Do not trust the audit blindly; every
finding is re-verified against the research corpus and the running code before any
change is made. A false finding is corrected in the audit trail rather than "fixed"
by breaking good code.

**This session's user override on order:** fix all the audit findings
**(priority to MINOR first)** one by one, with full context; move fixed findings into
`findings/FIXED/`; update all docs and counts; aim for at least 20+ fixes.

## Operating loop (per finding, per §2/§13 of the mandate)

1. Reproduce the evidence on current main.
2. Re-verify against the corpus artifact (read the cited decompile/doc lines).
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING.
4. Fix the cause, not the symptom; re-implement from understanding; cite the corpus
   artifact in the commit message; never paste decompiled code or proprietary strings.
5. Every fix paired with a test that fails before and passes after.
6. Record the Resolution section in the finding file (FIXED / FIXED-ALREADY / INVALID / SUPERSEDED
   + commit + verification + notes), move the file to findings/FIXED/, add the
   FIXED-LEDGER row, flip the CROSS-REF-MATRIX cell to MATCH, update the module doc
   verdict, update SUMMARY counts, update worklog.md.
7. One finding, one verified fix, one push. Pull frequently, never force-push, leave
   main green.

## Non-negotiables

- No shortcuts, no fallbacks that mask failure, no stubs/mocks/TODOs/placeholders.
- No hallucinated parity — no FIXED without a proving test.
- Safety hardenings preserved (destination-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- ReFS and LDM stay first-class (never downgraded to report-only).
- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, C consumer, fuzz smoke,
  Win7-inclusive import allowlist); Linux CI green (ASan/UBSan/TSan on touched
  tests, fuzz smoke). Signing stays disabled.
- Direct I/O on Windows: read lengths multiple of 4 KB and of 64 KB where possible.
- Performance fixes measured baseline → fix → re-benchmark, numbers recorded.
- The audit trail is the deliverable. No finding file deleted. Discovered divergences
  filed as new findings in the correct severity folder.
- Token hygiene: never commit the token; never log it.

## Session baseline (recorded at T1)

- core_recovery HEAD at session open: `98a5b139` (2026-10-29 03:43:59 +0000)
- corpus HEAD: `be0cfe9`
- Active board at open (filesystem truth): 1 SEVERE (S-040) · 23 MAJOR · 22 MINOR ·
  2 INFO; 136 files in findings/FIXED/; 4 tracked-open.
- Baseline suite (linux-release): 1338 passed / 0 failed (42 skipped: win-lane TIFF
  variants etc.).
- doc_truth_gate: green (52 active, 133 retired, 389 ledger-resolved, 135 D-rows).
- Audited-commit pins carried by the newest findings: S55 filings verified at
  `9633d94f`; S54 findings at `34d6cf10`; liveness battery pinned `1fccb0a4`.
