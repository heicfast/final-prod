# Session DFX17 — diff-to-fix roadmap execution (closing the parity loop)

Date: 2026-10-26 · Core HEAD at bootstrap: `5d63124` · Corpus: `03ad46c` (unmoved)
Baseline: linux-gcc RelWithDebInfo+ASan — **584/584 green** at bootstrap.

## Mandate (token redacted)

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence.
Do not trust the audit blindly; every finding is re-verified against the
research corpus and the running code before any change is made. A false
finding is corrected in the audit trail rather than "fixed" by breaking
good code.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ghp_<REDACTED>
- Do not clone, reference, or look at any other repository.
- Do not guess reference behavior — verify against the corpus.

## Verification loop (per finding)

1. Reproduce the audit's evidence on current main.
2. Re-verify against the corpus (read the cited artifact in full).
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE (INVALID) /
   OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING.
4. Fix only after classification. Re-implement from understanding; never paste
   decompiled code; cite the corpus artifact in the commit.
5. Every fix paired with a test that fails before and passes after.
   Unit for logic, integration on seeded fixtures, structure-aware fuzz for
   parsers, TSan for threading, ASan/UBSan for memory.
6. One finding, one verified fix, one push, one finding-file update.
7. Resolution section in the finding file; move to findings/FIXED/; update
   CROSS-REF-MATRIX.md, modules/<module>.md, SUMMARY.md, README.md, worklog.md.

## Fix order

ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) → TRACKED-OPEN.
Within each tier: correctness before performance, data integrity before
latency, silent-failure paths before observable inefficiencies.
User directive this session: fix all major ones first; target ≥20 fixes.

## Safety hardenings preserved

Destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer
explicit consent, 4Kn handling, cancellable IRP, SDDL gating — documented
improvements over reference weaknesses. Keep them; note preservation in
Resolutions.

## Quality gates

- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, C consumer,
  fuzz smoke, Win7-inclusive import allowlist). Signing stays disabled.
- Linux CI green (ASan/UBSan/TSan, fuzz smoke). No continue-on-error.
- Windows direct I/O: reads multiple of 4 KB and 64 KB where possible.
- Pull frequently; never force-push; leave main green.
- Token hygiene: never commit or log the token.
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs.
- ReFS and LDM remain first-class.
- Audit trail is the deliverable: no finding files deleted.

## Deliverables checklist

1. Every finding FIXED / FIXED-ALREADY / INVALID / SUPERSEDED — no
   DIVERGENCE-* or UNVERIFIED left.
2. INFORMATIONAL findings ACKNOWLEDGED.
3. TRACKED-OPEN resolved or pushed forward precisely.
4. CROSS-REF-MATRIX.md fully MATCH where fixed.
5. SUMMARY.md counts + performance numbers.
6. modules/*.md verdicts current.
7. worklog.md chronological with SHAs and test evidence.
8. Full green CI both platforms; sanitizers clean.
9. Performance findings with before/after numbers.
10. Tests fail-before/pass-after, named for behavior proven.
11. Detailed evidence-citing commit messages.
12. Tagged release if audit closes cleanly.
13. worklog handoff with next steps.
