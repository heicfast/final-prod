# Session prompt — diff-to-fix execution (DFX-37; the DFX36 name collided with a parallel lane)

Saved token-redacted at session start. Corpus `03ad46c` (unmoved). Core at `ea1a3c7` (post pull, main green, suite reportedly 865/865 at DFX32 close).

## Mandate (condensed, token redacted)

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the finding against the research corpus (`easeus-drw-research` @ `03ad46c`, read-only), fix it in the codebase, prove the fix with tests and benchmarks, and mark it fixed in the audit trail with evidence.

Two repos in scope and only two:
- Core (modified): heicfast/core_recovery
- Research corpus (ground truth, read-only): heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

## Classification per finding (before any fix)
- CONFIRMED-DIVERGENCE → fix
- ALREADY-FIXED → FIXED-ALREADY with reproduction
- FALSE-POSITIVE → INVALID with evidence, no code change
- OUTDATED-EVIDENCE → SUPERSEDED, re-audit
- NEW-FINDING discovered during verification → new finding file, fix same session

## Resolution template (appended to every finding file touched; file then moved to findings/FIXED/)

```markdown
## Resolution

**Status:** FIXED | FIXED-ALREADY | INVALID | SUPERSEDED
**Fixed in commit:** <sha>
**Verified by:** <test names + benchmark run IDs, or command evidence for INVALID/SUPERSEDED>
**Resolution notes:** <one paragraph — what was changed, or why the finding was invalid/superseded, with corpus and code citations>
```

## Non-negotiables
- Re-implement from understanding; never paste decompiled code; never copy proprietary strings; cite corpus artifact in commit message.
- Fix the cause, not the symptom. Match architecture and wiring.
- Every fix paired with a test that fails before and passes after. Unit / integration on seeded fixtures / structure-aware fuzz for parsers / TSan for threading / ASan-UBSan for memory.
- Safety hardenings preserved (destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- No stubs, no mocks, no TODOs, no fallbacks masking failure. Production grade only.
- One finding, one verified fix, one push, one finding-file update. Pull frequently, never force-push, keep main green.
- Windows direct I/O: reads multiple of 4 KiB, and of 64 KiB when possible.
- Win7-inclusive import allowlist gate intact. Signing stays disabled (no OSSign).
- Performance fixes: baseline → fix → re-benchmark → accept-or-revert; numbers in Resolution + SUMMARY.
- New deliberate divergences → PARITY-DELTAS.md rows.
- Update CROSS-REF-MATRIX.md, SUMMARY.md, modules/*.md, docs/* in the same commits.
- No finding file deleted — the audit trail is the deliverable.

## Session targets
- All remaining open findings resolved: S-034, M-030, M-034, M-039, M-040, M-048, N-007, N-065, N-066, N-072, OQ-S4-3, OQ-Z-E-FLT.
- ≥20 fixes this session (open items + NEW-FINDINGs discovered during verification + any audit-trail completeness gaps found).
- Tag release if clean; worklog handoff.
