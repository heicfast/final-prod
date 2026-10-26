# Session: review-and-validate (post-fix audit of the fix sessions)

Saved at session start, token redacted. Read twice before working.

## Mandate (verbatim summary)

Senior systems engineer role (25+ years, Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references). This session is
**not a fix session**: it is the review, confirmation, verification, and validation pass
over everything the parallel fix sessions (DFX lanes) landed.

Scope, repos, and only these:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged. [REDACTED]

## Session-specific directives (from the user, overriding the fix-session loop)

1. Review and audit the fixes: confirm, verify, validate every finding resolution
   the fix sessions recorded (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN).
2. Review multiple agents' collision commits (the union merges).
3. Check major removal commits — specifically named:
   - 53b7d68514f4e33eadd8bd198407bb5fe1bbf99e (M-052 convergence repair)
   - 83c796e275b9aab34af64f892916318588d70f55 (pwddict/D-80 reconciliation)
   - "etc." — find the others of the same class.
4. Verify and validate that each removal was valid and per the session principles
   (no hallucination, no lost tests, no lost safety hardenings, no lost first-class
   surfaces), and really worth it. If not — revert the removal.
5. Check for hallucinations in fixes/resolutions: tests that don't exist, claims that
   don't reproduce, corpus citations that don't say what's claimed. Fix them.
6. If a fix was not applied correctly for any finding — reopen the finding.
7. Assess everything, verify and validate.
8. Update ALL documentation in core_recovery (README, docs/, every doc), create new
   latest docs — API documentation, formats, connection guide, parameters, etc. —
   and update existing documentation with the latest stage, current behavior, specs.

## Inherited SOPs (unchanged from the fix sessions)

- Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark -> Document -> Push -> Loop.
- No shortcuts. No fallbacks that mask failure. No mock/stub/TODO. Production grade only.
- Every change proven with a test. No hallucinated parity.
- One coherent change, one verified push. Never force-push. Leave main green.
- The corpus is the only source of truth for reference behavior.
- Safety hardenings preserved (destination overlap refusal, BitLocker legitimate-unlock-only,
  RawFixer explicit consent, 4Kn handling, cancellable IRP, SDDL gating).
- ReFS and LDM remain first-class (never downgraded to report-only).
- Win7-inclusive build gate intact. Signing stays disabled.
- Token hygiene: never commit, never log.

## Fix-order priorities (for anything this session must repair)

ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL -> TRACKED-OPEN; correctness before
performance; data integrity before latency; silent-failure paths before observable
inefficiencies. Windows direct I/O: multiples of 4 KB, and 64 KB where possible.

## Session baseline (recorded at start)

- core_recovery HEAD at session start: `7ec544679fb32e174e9aa3278e9e9b3644dc67da`
  ("merge: union with origin/main — the parallel landings + DFX24's M-039 push absorbed")
- Corpus: `03ad46ccd6086000e53d7d3871ef43cd7f32e462` (unmoved from all prior sessions)
- Open finding folders at start: SEVERE 1, MAJOR 2, MINOR 1, FIXED 216, tracked-open 2.
- Known damage found at session start (pre-review): committed conflict markers in
  diff-to-fix/CROSS-REF-MATRIX.md, diff-to-fix/findings/FIXED/M-041-*.md,
  diff-to-fix/modules/fat.md, diff-to-fix/modules/exfat.md, worklog.md.
