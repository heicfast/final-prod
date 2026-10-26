# Session 31 prompt (DFX31) — saved token-redacted at bootstrap, per protocol
# (re-stamped from the DFX30 collision; the ex-DFX26 lane holds DFX30)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix
with tests and benchmarks, and mark it fixed in the audit trail with
evidence. The audit is a hypothesis; this session is the verifier.
Classification before any change: CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE (INVALID) / OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING.

Two repositories in scope and only two:
- Core (modified): heicfast/core_recovery
- Research corpus (ground truth, read-only): heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ghp_<REDACTED>

SOPs (non-negotiable):
- Re-implement from understanding; never paste decompiled code; never copy
  proprietary strings; cite the corpus artifact in the commit message.
- Fix the cause, not the symptom. Match architecture and wiring.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings preserved (dest-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- No shortcuts, no fallbacks that mask failure, no stub/mock/TODO.
- One finding, one verified fix, one push. Pull before every landing;
  first-landing-wins on overlap; union-merge reconciliation; never
  force-push; leave main green.
- Windows CI gates intact (MSVC /W4 /WX, Win7-inclusive import gate,
  signing stays disabled); Linux suite + sanitizers green.
- Audit trail: Resolution section in every finding touched; FIXED/ move;
  RESOLVED-INDEX row; CROSS-REF-MATRIX cell -> MATCH; modules/*.md verdict;
  SUMMARY counts; worklog.md entry. Never delete a finding.
- Performance findings: baseline -> fix -> re-benchmark -> accept or revert;
  numbers recorded. Direct I/O: 4 KiB-multiple lengths, 64 KiB multiples
  where possible.
- Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFO -> TRACKED-OPEN,
  largest user impact first within tier.
- Discovered divergences: new finding file + matrix row + SUMMARY update +
  fix or precise tracked-open push-forward. No silent fixes.
- Target: 20+ fixes this session; MAJOR focus initially per the user.

Session-31 lane claim (disjoint, posted at bootstrap): the OQ front
remainder (OQ-S2M-2, OQ-S3-JPEG, OQ-RS) + M-029's decode-blocked half
(Date/Size bucket IDs + tag derivation) + the mid-MAJOR cluster
(M-025/M-039/M-020/M-048/M-018) that the DFX27 (MAJOR-head) and DFX30
(ex-DFX26, MAJOR-tail) lanes reach last; first-landing-wins, pull before
every landing.
