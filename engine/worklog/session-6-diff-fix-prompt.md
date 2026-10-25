# Session: diff-to-fix full-roadmap execution (session 6)

**Session start:** 2026-10-26
**Core repo:** https://github.com/heicfast/core_recovery (modified)
**Corpus (read-only):** https://github.com/heicfast/easeus-drw-research @ `03ad46c`
**Token:** REDACTED (env/git config only, never committed)

## Mandate (abridged from the session prompt)

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE,
SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the
finding against the corpus and the running code, classify it
(CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
NEW-FINDING), fix it with tests that fail before and pass after, prove
performance fixes with benchmarks, and record the Resolution in the finding
file + CROSS-REF-MATRIX + SUMMARY + modules/*.md + worklog.

SOPs: verify → reproduce → classify → fix → re-review → test → benchmark →
document → push → loop. One finding, one verified fix, one push. No stubs, no
mocks, no TODO leave-behinds, no hallucinated parity. Safety hardenings
(destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer
consent, 4Kn, cancellable IRP, SDDL gating) preserved. Win7-inclusive import
gate intact. Signing stays disabled. Never force-push; pull frequently.

Fixed findings move to `diff-to-fix/findings/FIXED/` with resolutions recorded;
audit trail files updated each fix.

## T1 — bootstrap record

- Core cloned fresh: current `main` = `bf7df39` ("verify: S-012 convergence —
  hostile-input smoke (400 iters ASan/UBSan) + claim-gate evidence addendum").
- Audited commit per `diff-to-fix/README.md`: session 4 wave-1 `204137b`
  (lineage `63a9167` rebased onto `ed00020`); fix legs since then landed
  S-013/S-014/S-015/S-017/S-006/S-011/S-012/S-019/S-022/N-021/N-022/N-023/N-034/
  A-002/A-004+S-016 and the M-030-partial transport wave `a4053c4`.
- Corpus cloned fresh: `03ad46c` — matches the audit's pinned corpus commit.
- Prior session handoff (164fd78): suite 396/396 at `bf7df39`'s parent lineage;
  remaining SEVERE per handoff: S-003/S-007/S-009/S-020/S-021 (+ S-023..S-028
  integrated later per 85cc08c... actually all still OPEN per file inventory).

## Session-6 file inventory (direct count at `bf7df39`)

- SEVERE active: 11 (S-003, S-007, S-009-remaining, S-020, S-021, S-023,
  S-024, S-025, S-026, S-027, S-028)
- MAJOR active: 36 (M-002 .. M-039 minus the 3 fixed)
- MINOR active: 38 files
- INFORMATIONAL active: 18 files (4 already ACKNOWLEDGED: I-001/I-002/I-003 +
  partial N-007/N-009 groups)
- tracked-open: 11 OQ files
- FIXED/: 33 findings + RESOLVED-INDEX

Next: baseline build + full suite on `bf7df39` before touching anything.
