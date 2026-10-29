# Session-84 (S84) — the user's fix-mandate prompt (token redacted)

Mandate: execute the roadmap in core_recovery/diff-to-fix/. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify, fix in the codebase, prove with tests/benchmarks,
mark fixed in the audit trail. Two repos in scope and only two:
core_recovery (modified) + easeus-drw-research (read-only corpus).
GitHub token: [REDACTED — env/git config only, never committed].

Key rules (abridged from the full prompt):
- Verify → Reproduce → Classify (CONFIRMED / ALREADY-FIXED / INVALID /
  SUPERSEDED) → Fix → Test → Benchmark → Document → Push → Loop.
- One finding, one verified fix, one push. Pull frequently, never
  force-push, leave main green.
- No stubs/mocks/fallbacks. Re-implement from understanding; cite the
  corpus artifact. Fix the cause, not the symptom.
- Safety hardenings preserved (destination-overlap refusal, BitLocker
  legitimate-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- Performance fixes measured (baseline → fix → re-bench → accept/revert);
  Windows direct I/O stays 4KB-multiple and 64KB-multiple where possible.
- Findings move to findings/FIXED/ with Resolution sections; matrix,
  modules/*.md, SUMMARY, worklog updated per fix; no finding deleted.
- Aim for 20+ fixes this session, priority to MAJOR first.
- Deliverables: every finding resolved to a terminal state; the audit
  trail (diff-to-fix/) is the final evidence set; worklog handoff with
  the exact commit and next steps for anything left open.
