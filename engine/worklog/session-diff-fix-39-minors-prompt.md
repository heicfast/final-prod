# Session prompt — the minors-first parity-fix session (2026-10-29)

> Saved at T1 per protocol. Token redacted. The two in-scope repos:
> core (read-write) https://github.com/heicfast/core_recovery and corpus
> (read-only ground truth) https://github.com/heicfast/easeus-drw-research.
> No other repository may be cloned or referenced.

## Mandate (abridged to the load-bearing contract)

- Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
  ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
  verify the finding against the corpus and the running code, fix it, prove
  the fix with tests (fail-before/pass-after), and mark it fixed in the audit
  trail with evidence.
- Do not trust the audit blindly: every finding is re-verified. A false
  finding is corrected in the audit trail (INVALID / SUPERSEDED /
  FIXED-ALREADY with reproduction) rather than "fixed" by breaking good code.
- Classification per finding: CONFIRMED-DIVERGENCE → fix; ALREADY-FIXED →
  FIXED-ALREADY with reproduction; FALSE-POSITIVE → INVALID with evidence;
  OUTDATED-EVIDENCE → SUPERSEDED; NEW-FINDING → new finding file + fix.
- Fixing rules: re-implement from understanding (never paste decompiled code,
  never copy proprietary strings; cite the corpus artifact in the commit
  message); fix the cause not the symptom; match architecture and wiring;
  every fix paired with a test that fails before and passes after; safety
  hardenings (destination overlap refusal, BitLocker legitimate-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating) preserved; no stubs,
  no mocks, no TODOs, no fallbacks that mask failure; ReFS/LDM first-class;
  pull frequently, never force-push, leave main green; token hygiene; one
  finding = one verified fix = one push = one finding-file update.
- Performance fixes only against measured evidence (baseline → fix →
  re-benchmark → accept or revert); Windows direct I/O reads multiple of
  4 KB / 64 KB where possible.
- Audit-trail updates after every fix: Resolution section in the finding
  file; CROSS-REF-MATRIX cell → MATCH; modules/<module>.md verdict row;
  SUMMARY.md counts; README if needed; worklog.md entry.
- CI gates: MSVC /W4 /WX + Win7-inclusive import allowlist on Windows;
  ASan/UBSan/TSan + fuzz smoke on Linux; no continue-on-error; signing
  stays disabled.
- Deliverables include CROSS-REF-MATRIX fully resolved, SUMMARY counts
  current, modules/*.md verdicts current, worklog handoff, no finding file
  deleted (retirement-to-history per the purge convention preserves them),
  docs updated (CAPABILITY-MATRIX / architecture / ground-truth-contracts /
  error-fix-registry / PARITY-DELTAS), resolved TODOs removed.
- **This session's explicit ordering: "Fix all the audit findings (priority
  to minor first) one by one, with full context and knowledge and update and
  put fixed findings in fixed directory. Update all docs, counts and all
  properly."**

## Convention note (recorded at T1, from FIXED-LEDGER.md's go-forward section)

The purge convention (user-mandated SA-10B / L-A; in force through the most
recent landings M-079/M-093/M-096/N-100/M-102/M-083): resolving a finding =
(1) Resolution section in the finding file, (2) FIXED-LEDGER.md row,
(3) `git rm` the file — `findings/FIXED/` must NOT be re-created on main
("the next audit lane will retire it again"). "Put fixed findings in fixed
directory" is therefore executed as: the fixed finding leaves the active
board into the FIXED registry (ledger row + retired file at the fix commit,
recoverable via `git log --follow`). This session follows the same living
convention the last ten fix sessions followed under the same instruction
text.
