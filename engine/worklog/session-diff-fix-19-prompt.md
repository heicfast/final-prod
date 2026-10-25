# diff-to-fix session 17 prompt (saved at bootstrap; token REDACTED)

Mandate: execute the roadmap in core_recovery/diff-to-fix/. For every finding
(ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN) — independently verify,
fix, prove with tests, mark fixed with evidence. The corpus is the only source
of truth (easeus-drw-research, read-only, commit 03ad46c). False findings are
corrected, not "fixed".

Repos: core (modified) github.com/heicfast/core_recovery; corpus (read-only)
github.com/heicfast/easeus-drw-research. Token: REDACTED (env/git config only).

SOPs: verify -> reproduce -> classify (CONFIRMED / ALREADY-FIXED / INVALID /
SUPERSEDED) -> fix cause -> test fail-before/pass-after -> benchmark for
perf-tagged -> update audit trail (finding Resolution, CROSS-REF-MATRIX,
modules/*.md, SUMMARY.md, RESOLVED-INDEX) -> push one finding per push.
Safety hardenings preserved. No stubs/mocks/fallbacks. Win7 gate intact.
Pull frequently; never force-push; leave main green. CI: MSVC /W4 /WX,
ASan/UBSan/TSan, fuzz smoke.

Fix order this session (user directive): MAJOR findings first, target 20+
fixes. Priority: unclaimed MAJORs — M-016, M-017, M-019, M-020, M-021, M-024,
M-025, M-028, M-030, M-033, M-034, M-038, M-039, M-040, M-041, M-042, M-044,
M-048 — then unclaimed MINORs/INFORMATIONAL/TRACKED-OPEN. Lanes already
claimed by parallel agents (avoid): M-005 (DFX14), M-007/M-008/M-032/M-050/
M-029/M-049 (DFX11), M-052/M-035/M-018/M-013/M-014 (DFX9), S-029 (DFX10),
M-045/M-043 (DFX15 next queue), N-067/N-068/N-063/N-065 (DFX16 next queue).
