# Session-86 (S86) — the diff-to-fix fix lane prompt (saved per T1; token REDACTED)

> The S86 seat HELD: the racing window's own records renumbered S86→S89→S90
> in deference to this lane's earlier push (the premature renumber
> retracted; saved at session open from the user's mandate).

> with the earlier claim; this lane takes S89 — the M-153 renumber
> precedent). Saved at session open from the user's mandate; the S89 plan
> section below stands as written except the lane id.


Saved at session open from the user's mandate. Corpus `be0cfe9` (read-only),
core opened at `32441353` (the S84/S85-window head: the image `--create` flag
landing). Two repos in scope and only two: core_recovery (modified) +
easeus-drw-research (read-only). GitHub token env/git-config only, never
committed — REDACTED here.

## Mandate (verbatim structure)

Role: senior systems engineer (25+ years) — Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered
references. Execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with
tests and benchmarks, mark it fixed in the audit trail with evidence. Do not
trust the audit blindly; every finding re-verified against the research corpus
and the running code before any change. A false finding is corrected in the
audit trail rather than "fixed" by breaking good code.

1. Read before touching anything: diff-to-fix README/SUMMARY/SEVERITY-SCALE/
   PARAMETERS/CROSS-REF-MATRIX; every finding file (findings/** incl. FIXED),
   tracked-open/**; every module overview; evidence/** (reproducible before
   fixing against it). Read the corpus line-by-line where a finding requires;
   use Ghidra/objdump/strings/etc. on corpus binaries when ambiguous. Read the
   core repo line by line; every commit since the audited commit. Record the
   audited commit in the worklog and in every finding touched.
2. Independently verify each finding before fixing (priority ACUTE → SEVERE →
   MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN): reproduce the evidence;
   re-verify against the corpus; classify CONFIRMED-DIVERGENCE /
   ALREADY-FIXED (mark FIXED-ALREADY) / FALSE-POSITIVE (mark INVALID) /
   OUTDATED-EVIDENCE (mark SUPERSEDED) / NEW-FINDING (add finding file, fix
   this session). Only after classification, proceed to the fix. Resolution
   section template at the bottom of each finding file (Status / Fixed in
   commit / Verified by / Resolution notes). Never delete findings.
3. Fixing rules: re-implement from understanding, never paste decompiled code,
   never copy proprietary strings, cite the corpus artifact in the commit
   message. Fix the cause not the symptom. Match architecture and wiring. Every
   fix paired with a test that fails before and passes after (unit /
   integration on seeded fixtures / structure-aware fuzz for parsers / TSan
   threading / ASan-UBSan memory). Safety hardenings preserved (destination
   overlap refusal, BitLocker legitimate-unlock-only, RawFixer explicit
   consent, 4Kn, cancellable IRP, SDDL gating) — note preservation in
   Resolution. No shortcuts, no fallbacks that mask failure, no stub/mock/
   TODO/placeholder. If a finding cannot be fixed this session, leave it
   TRACKED-OPEN with precise remaining scope + next concrete step. No
   hallucinated parity. ReFS and LDM remain first-class. Do not break contracts
   others consume without updating them in the same change. Pull frequently,
   rebase carefully, re-run full suite after every pull, main green, never
   force-push. Token hygiene: never commit, never log. Commit and push after
   every coherent finding fix — one finding, one verified fix, one push, one
   finding-file update. Do not batch unrelated fixes.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL →
   TRACKED-OPEN. Within tier: largest user impact first (correctness before
   performance, data integrity before latency, silent-failure before visible
   inefficiency).
5. Performance fixes: baseline first, fix, re-benchmark same fixtures; accept
   only if the metric moves the right way without regressing others; record
   numbers in Resolution + SUMMARY's performance section. Windows direct I/O
   reads: multiple of 4 KB and of 64 KB where possible.
6. Tests: unit for pure logic; integration on seeded fixtures (from
   diff-to-fix/evidence/ where possible); structure-aware fuzz harnesses into
   the existing fuzz suite; TSan for threading; ASan/UBSan for memory; CLI
   contract tests for grammar/exit-code changes; ABI consumer test under MSVC
   /W4 /WX for ABI surface changes; benchmark comparison for
   performance-tagged findings. Test names describe behavior, not finding IDs.
7. Audit-trail updates after every fix: Resolution in finding file;
   CROSS-REF-MATRIX cell → MATCH (link to fix commit); modules/<module>.md
   verdict table row → MATCH + finding link FIXED; SUMMARY.md counts;
   README.md audited-commit/navigation; worklog.md in the core repo.
   Never delete resolved findings.
8. Discovered divergences during fixing: new finding file in the correct
   severity folder with full evidence; row in CROSS-REF-MATRIX; SUMMARY.md
   update; fix this session or track precisely in tracked-open/; announce in
   the commit message. No silent fixes.
9. CI: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, seeded
   integration, C consumer, fuzz smoke, import allowlist Win7-inclusive);
   Linux green (pure-core, CLI, ASan/UBSan/TSan on touched tests, fuzz smoke).
   No continue-on-error; no skipped stages without recorded reason. Pin
   toolchains/actions where flaky. Pull frequently; main green. Win7-inclusive
   loadability gate intact. Signing stays disabled.
10. Documentation: every fix in the finding's Resolution + worklog.md;
    docs/CAPABILITY-MATRIX.md where capability status changes;
    architecture.md / ground-truth-contracts.md / error-fix-registry.md in the
    same commit as behavior changes. Remove resolved TODO/FIXME/HACK/future-
    work notes. PARITY-DELTAS.md documents deliberate divergences only.
11. Deliverables: every ACUTE/SEVERE/MAJOR/MINOR finding FIXED (tests + SHA) /
    FIXED-ALREADY / INVALID / SUPERSEDED; INFORMATIONAL ACKNOWLEDGED;
    TRACKED-OPEN resolved or pushed forward precisely; CROSS-REF-MATRIX fully
    MATCH with links; SUMMARY counts + top remaining risks; modules/*.md
    current; worklog.md chronological; green CI; perf numbers before/after;
    fail-before/pass-after tests; detailed commit messages; no finding deleted;
    tagged release (v0.5.0-parity or next semver) if clean; worklog handoff
    with exact commit + remaining items + next steps; diff-to-fix/ preserved
    as final evidence set.
12. First 20 TODOs: T1 pull repos + record commits + save prompt; T2 read
    diff-to-fix in full; T3 read core repo in full; T4/T5 ACUTE reproduce +
    fix; T6/T7 SEVERE; T8/T9 MAJOR; T10 perf-tagged; T11/T12 MINOR (batch per
    module); T13 INFORMATIONAL acknowledge; T14 TRACKED-OPEN resolve or push;
    T15 matrix module by module; T16 SUMMARY counts + performance; T17
    modules/*.md + docs, remove resolved TODOs; T18 full CI sweep; T19
    discovered tests + coverage; T20 tag release + handoff + final sweep.
13. SOPs: Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark →
    Document → Push → Loop. No shortcuts/fallbacks/compromises/mock/stub.
    Every finding independently verified before fixed; every fix proven with a
    test; every resolution recorded; no silent fixes/findings. One finding, one
    verified fix, one push. Pull frequently; never force-push; main green.
    Commit messages state finding ID, divergence, fix, test, corpus citation.
    Re-read prompt + SUMMARY + worklog periodically. Every intermediate push
    updates worklog.md. The audit trail is the deliverable.

User's closing instruction: fix all audit findings (priority to major first)
one by one, with full context and knowledge; put fixed findings in the FIXED
directory; update all docs and counts properly. Aim for at least 20+ fixes
this session; don't stop before that.

## Session-86 plan (recorded at open)

Board at open (filesystem truth): 8 MAJOR active (M-075 M-078 M-086 M-087
M-089 M-101 M-105 M-107) · 2 MINOR (N-107 N-127) · 0 INFO / ~198-200 FIXED
(ledger census at open) / 4 tracked-open. Of the MAJORs, M-086/M-087/M-089 and
the MINORs N-107/N-127 are driver-repo-resident per OQ-S10-3 — outside the
two-repo grant (rda-driver untouched; pushed forward per the standing
disposition). In-scope fix surface this session: M-075, M-078, M-101, M-105
(option (a) posture per the finding's own recommended default), M-107's
remaining members (X3F/DWG/WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT — CRW already
landed), the 4 tracked-open dispositions, and NEW findings discovered during
verification (the 20+ target: 5 MAJOR-level closures + ~10 M-107 member
landings + 2-4 tracked-open resolutions + newly-filed divergences).
