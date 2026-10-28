# Session-49 fix mandate (2026-10-29) — saved token-redacted per house convention

Role: senior systems engineer, 25+ years (Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered
references). Mandate: execute the roadmap in `core_recovery/diff-to-fix/`.

Scope (only these two repositories):
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: supplied via env at runtime only; never committed, never logged.

Workflow per finding (ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL →
TRACKED-OPEN): reproduce the audit evidence → re-verify the cited corpus
artifact in full (never trust the finding text alone; the corpus wins on
conflict) → classify CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING → fix the cause, not the symptom → pair every
fix with a test that fails before and passes after → update the audit trail
(finding file Resolution section, CROSS-REF-MATRIX cell to MATCH with commit
link, modules/*.md verdict row, SUMMARY.md counts, worklog.md entry) → one
finding, one verified fix, one push.

Non-negotiables: re-implement from understanding (never paste decompiled code,
never copy proprietary strings; cite the corpus artifact in the commit
message); no stubs/mocks/TODOs/fallbacks that mask failure; safety hardenings
(destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer
explicit consent, 4Kn handling, cancellable IRP, SDDL gating) are preserved
and their preservation noted; ReFS and LDM stay first-class; performance fixes
are measured (baseline → fix → re-benchmark → accept-or-revert with numbers
recorded); Win7-inclusive import gate stays intact; signing stays disabled;
pull frequently (parallel lanes active: S45 claimed ACUTE/SEVERE, S46 claimed
MINOR, S47/S48 claimed INFO/tracked-open/MAJOR — convergence per
earlier-push-wins with independent verification documented); never force-push;
leave main green.

Performance note (platform I/O): any Windows direct I/O introduced or touched
must use read lengths that are multiples of 4 KB (and 64 KB where possible) —
the documented non-64K-aligned direct-read slow path; verified on Windows CI.

Deliverables: every finding FIXED / FIXED-ALREADY / INVALID / SUPERSEDED with
evidence; CROSS-REF-MATRIX fully resolved; SUMMARY counts current;
modules/*.md verdicts current; worklog.md chronological log with SHAs and test
evidence; no finding file deleted (the audit trail is the deliverable);
diff-to-fix/findings/FIXED/ receives each retired finding file.

Session directive from the user: priority to ACUTE first, one by one, with
full context; update all docs and counts; aim for 20+ fixes this session.
