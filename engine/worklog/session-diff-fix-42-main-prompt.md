# Session fix roadmap prompt (2026-10-29, session-42) — token redacted

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` for every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify,
fix, prove with tests, mark fixed in the audit trail with evidence.

Repos (only these two):
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed)

Key SOP rules:
1. Read audit output first; read cited corpus artifacts in full; read core repo in full.
2. Re-verify every finding before fixing: CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING.
3. Resolution section template appended to every finding touched; never delete findings.
4. Fix cause not symptom; re-implement from understanding; no pasted decompiled code.
5. Every fix paired with test that fails before / passes after.
6. Preserve safety hardenings (destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
7. No stubs/mocks/TODOs/fallbacks; leave TRACKED-OPEN with precise scope if unfixable this session.
8. Priority: INFORMATIONAL first (user directive this session), then ACUTE → SEVERE → MAJOR → MINOR → TRACKED-OPEN.
9. One finding, one verified fix, one push. Pull frequently. Never force-push. main stays green.
10. Update CROSS-REF-MATRIX, SUMMARY, modules/*.md, FIXED-LEDGER, worklog.md after every fix.
11. Fixed findings go to findings/FIXED/ (user directive).
12. Windows CI green (MSVC /W4 /WX, Win7-inclusive import gate); Linux CI green (ASan/UBSan/TSan, fuzz).
13. Performance fixes: baseline → fix → re-benchmark → accept/revert with numbers recorded.
14. Docs updated in same commit; resolved TODO/FIXME/HACK removed; PARITY-DELTAS for deliberate divergences.

Opening state (T1 record):
- Audited commit per latest S41 lane: core c7fa527c (pre-rebase pin), corpus be0cfe9
- Current main at session open: 67de3276 (2026-10-28 19:44:56 +0000)
- Corpus HEAD: be0cfe95ecf22ac67e0f3a823dd003163b196d38 (matches audit pin)
- Active findings on disk: 3 SEVERE (S-040/S-048/S-050) · 21 MAJOR · 18 MINOR · 8 INFO · 3 tracked-open
- Target: 20+ fixes this session, INFORMATIONAL tier first per user directive.
