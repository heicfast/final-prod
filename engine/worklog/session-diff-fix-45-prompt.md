# Session 45 — diff-fix execution prompt (token redacted)

Saved at session open (T1). The live token was provided via the user message only;
it is redacted here and never committed.

**Mandate:** execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify
the finding, fix it in the codebase, prove the fix with tests and benchmarks, and mark
it fixed in the audit trail with evidence. The audit is a hypothesis; this lane is the
verifier. False positives cost more than honest UNVERIFIEDs.

**Repos in scope (only two):**
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research

**Classifications:** CONFIRMED-DIVERGENCE → fix; ALREADY-FIXED → FIXED-ALREADY with
reproduction; FALSE-POSITIVE → INVALID with evidence; OUTDATED-EVIDENCE → SUPERSEDED;
NEW-FINDING → file + fix this session.

**SOP:** Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document →
Push → Loop. One finding, one verified fix, one push. Pull frequently, never force-push,
leave main green. Re-implement from understanding — never paste decompiled code, never
copy proprietary strings. Every fix paired with a test that fails before and passes after.

**Fix order:** ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) → TRACKED-OPEN.
User's session emphasis: "Fix all the audit findings (priority to major first) one by one …
Aim for at least 20+ fixes in this session."

**Session pins:** opened at core `98079347` (HEAD at clone), corpus `be0cfe9`.
Board at open (filesystem truth): 77 active — 1 ACUTE (A-014) · 9 SEVERE · 28 MAJOR ·
32 MINOR · 7 INFO / 64 files in findings/FIXED/ / 6 tracked-open.
