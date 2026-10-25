# Session prompt — diff-to-fix close-out (focus: 33 MINORs), 2026-10-26

(Saved verbatim from the session prompt; the GitHub token is redacted per token hygiene.)

Mandate: senior systems engineer (25+ years; Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references). Execute the roadmap
in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR, MINOR,
INFORMATIONAL, TRACKED-OPEN — independently verify the finding, fix it in the codebase,
prove the fix with tests and benchmarks, and mark it fixed in the audit trail with evidence.
Do not trust the audit blindly; every finding is re-verified against the research corpus and
the running code before any change is made. A false finding is corrected in the audit trail
rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): `ghp_REDACTED`

Do not clone, reference, or look at any other repository. Do not guess reference behavior —
verify against the corpus.

Key SOPs:
1. Read before touching anything — audit output first, corpus line-by-line where a finding
   requires it, core repo in full. Record the audited commit in the worklog and every finding.
2. Independently verify each finding: reproduce evidence, re-verify against corpus, classify
   CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING.
   Only after classification proceed to fix. Every touched finding gets a Resolution section:
   Status (FIXED | FIXED-ALREADY | INVALID | SUPERSEDED), fixed-in commit, verified-by,
   resolution notes. Never delete finding files.
3. Fixing rules: re-implement from understanding (never paste decompiled code / proprietary
   strings; cite corpus artifact in commit message); fix the cause not the symptom; match
   architecture and wiring; every fix paired with a test that fails before and passes after;
   safety hardenings are preserved (destination overlap refusal, BitLocker legitimate-unlock-only,
   RawFixer explicit consent, 4Kn handling, cancellable IRP, SDDL gating); no shortcuts, no
   fallbacks masking failure, no stubs/mocks/TODOs; ReFS and LDM remain first-class; do not
   break contracts other agents consume without updating in the same change; pull frequently,
   never force-push, leave main green; token never committed or logged; one finding = one
   verified fix = one push = one finding-file update.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) → TRACKED-OPEN.
   User directive this session: fix ALL findings, **focus on the 33 MINORs**, move fixed
   findings into `findings/FIXED/`.
5. Performance fixes: measure, don't guess (baseline → fix → re-benchmark → accept or revert;
   Windows direct I/O must be 4 KiB-multiple and 64 KiB-multiple where possible).
6. Tests: unit for logic, integration on seeded fixtures, structure-aware fuzz for parsers,
   TSan for threading, ASan/UBSan for memory, CLI contract tests, ABI consumer test for C ABI
   changes, benchmarks for perf findings. Test names describe behavior proven.
7. Audit trail updates after every fix: finding Resolution, CROSS-REF-MATRIX (MATCH cells),
   modules/*.md verdict tables, SUMMARY.md counts, README audited-commit, core worklog.md.
8. Discovered divergences: new finding file + matrix row + SUMMARY + fix this session; announce
   in commit message. No silent fixes.
9. CI: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, integration, C consumer, fuzz
   smoke, Win7 import allowlist); Linux green (ASan/UBSan/TSan, fuzz smoke); no
   continue-on-error; Win7-inclusive build; signing stays disabled.
10. Docs: CAPABILITY-MATRIX, architecture.md, ground-truth-contracts.md, error-fix-registry.md
    updated in the same commit; remove resolved TODO/FIXME/HACK; PARITY-DELTAS.md carries every
    deliberate divergence.
11. Deliverables: every finding resolved to FIXED / FIXED-ALREADY / INVALID / SUPERSEDED;
    INFORMATIONALs ACKNOWLEDGED; TRACKED-OPENs resolved or pushed forward precisely; matrix
    fully MATCH-populated; SUMMARY counts current; modules updated; worklog with SHAs + test
    evidence; green CI; perf numbers recorded; fail-before/pass-after tests; all changes pushed
    with evidence-citing messages; no finding deleted; tagged release if clean; handoff.
12. First 20 TODOs as listed in the original prompt (T1..T20).
13. SOP: Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document →
    Push → Loop. The audit trail is the deliverable.

Final reminder: the corpus is the only source of truth for the reference. The audit is a
hypothesis; this session is the verifier. No shortcuts, no samples, no assumptions, no
hallucination, production grade only. Work long-horizon; commit and push often; keep
worklog.md current. Save this prompt (token redacted), read twice, start with T1.
