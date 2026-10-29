# Session prompt — diff-to-fix execution lane S63 (2026-10-29)

Saved verbatim per the mandate (token redacted; the S59-S62 lane seats
belong to the parallel windows per SA-9 — this lane takes S63). GitHub
token redacted — env/git config only, never committed.

---

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix
with tests and benchmarks, and mark it fixed in the audit trail with
evidence. The audit is not trusted blindly; every finding is re-verified
against the research corpus and the running code before any change. A
false finding is corrected in the audit trail rather than "fixed" by
breaking good code.

Two repositories in scope, only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — credential store only]

SOPs (abridged; full text in the live prompt):
1. Read before touching anything; record the audited commit in every
   finding touched.
2. Independently verify each finding: reproduce, re-verify against the
   corpus, classify CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
   OUTDATED-EVIDENCE / NEW-FINDING; only then fix.
3. Fixing rules — re-implement from understanding; fix the cause; match
   architecture and wiring; every fix paired with a failing-then-passing
   test; safety hardenings preserved; no shortcuts/stubs/TODOs; no
   hallucinated parity; ReFS/LDM first-class; contracts updated in the
   same commit; pull frequently, never force-push, leave main green;
   token never committed/logged; one finding, one verified fix, one push.
4. Fix order: SEVERE → MAJOR → MINOR → INFO → tracked-open (user
   emphasis: priority to major; largest user impact first in each tier).
5. Performance fixes measured, not guessed; Windows direct I/O length
   rules (4 KB multiple, 64 KB multiple where possible).
6. Tests per surface (unit/integration/fuzz/TSan/ASan-UBSan/CLI/ABI/bench);
   test names describe behavior.
7. Audit-trail updates after every fix (Resolution, matrix → MATCH,
   module verdict, SUMMARY counts, worklog).
8. Discovered divergences → new findings + announce in commits.
9. CI green (Linux locally; Windows via CI); no continue-on-error;
   Win7-inclusive gate intact; signing stays disabled.
10. Docs updated in the same commit (CAPABILITY-MATRIX, architecture,
    ground-truth-contracts, error-fix-registry, PARITY-DELTAS).
11. Deliverables per the mandate; aim for 20+ fixes; fixed findings into
    findings/FIXED/; no finding file deleted.
12. SOP loop: Verify → Reproduce → Classify → Fix → Re-review → Test →
    Benchmark → Document → Push → Loop.

---

## T63 record (this lane)

- Opened AFTER absorbing the parallel hot window: this lane's first
  local commit (M-147) was reset in favor of the remote S62 lane's
  convergent `b09b46c8` landing per SA-9 (their push held the seat; this
  lane's independent corpus verification of all four default sites and
  the NTFSLib store-guard structure CONFIRMED their reading — recorded
  here as the convergence note, no duplicate code).
- Opening HEAD: `9b92e3d9` (corpus `be0cfe9` throughout).
- Filesystem board at open: **1 SEVERE [S-040] · 20 MAJOR · 15 MINOR ·
  0 INFO active / 148 FIXED / 4 tracked-open**.
- Lane priorities (collision-aware: the S62 MAJOR-first lane has claimed
  M-142 next per its worklog; the S59-S61/S60 windows are absorbing the
  bounded NTFS/exFAT/carve minors): **S-040 first** (the only SEVERE,
  unclaimed), then the bounded MAJORs outside the S62 family — M-140,
  M-143, M-144, M-146, M-136, M-135, M-073, M-075, M-078, M-101 — then
  the decision-class (M-105/M-111/M-114) and the remaining MINORs, then
  the tracked-open items.
