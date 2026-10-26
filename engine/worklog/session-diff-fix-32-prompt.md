# Session 32 prompt (DFX32) — saved token-redacted at bootstrap, read twice before work

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding against the research corpus (ground
truth, read-only: easeus-drw-research @ 03ad46c), fix it in the codebase,
prove the fix with tests and benchmarks, and mark it fixed in the audit
trail with evidence. The audit is a hypothesis; this session is the
verifier. False findings are corrected in the audit trail rather than
"fixed" by breaking good code.

Repos in scope (only two): core_recovery (modified) and
easeus-drw-research (read-only ground truth). GitHub token: [REDACTED —
env/git-config only, never committed, never logged].

SOP per finding: Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE /
ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix
→ Re-review → Test (fail-before/pass-after) → Benchmark (perf-tagged) →
Document (Resolution section, CROSS-REF-MATRIX, modules/*.md, SUMMARY,
docs/*) → Push → Loop. One finding, one verified fix, one push.

Key rules: re-implement from understanding (never paste decompiled code);
fix the cause not the symptom; safety hardenings preserved; no shortcuts,
no fallbacks masking failure, no stubs/TODOs; no hallucinated parity —
every FIXED has a proving test; pull frequently, never force-push, leave
main green; fixed findings moved to findings/FIXED/; commit messages cite
finding ID + corpus artifact; worklog.md updated every landing.

Session-32 lane (disjoint from DFX23's MINOR queue and DFX24's MAJOR
queue, both claimed in the same window): the tracked-open queue first
(OQ-S3-JPEG, OQ-FLT-1, OQ-S6-2, OQ-S6-3, OQ-S5-1, OQ-S6-1, OQ-Z1/Z2/Z4/
Z5, OQ-RS-5/6/7/8, OQ-E1, OQ-S2M-2, OQ-T6) — each resolved to a fix, an
evidence-backed closure, or a precise push-forward with NEW evidence from
first-hand corpus reads; then the deferred tails of the claimed queues
via first-landing-wins (N-064/N-071/N-032 win-layer MINORs DFX23 put
last; M-041/M-040/M-034 MAJORs DFX24 put last) with a pull before every
landing. Target: 20+ resolutions this session.

Baseline at bootstrap: linux-release 737/737 green at `02b950f`
(incl. win_syntax_check; 4 pre-existing env skips inside cr_tests:
QemuVmdk/Vhd/VhdxCompat + UpcaseAlgorithmMatchesRealMkfsExfat — no
qemu/mkfs tools in this container; unchanged from prior sessions).
Corpus `03ad46c` unmoved.
