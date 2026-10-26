# Session DFX26 — diff-to-fix execution prompt (token redacted)

**Role:** senior systems engineer (25+ years) — Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered references.
Closed audit findings on commercial recovery engines; knows the difference between
"the auditor says X" and "I proved X, fixed it, and proved the fix."

**Mandate:** execute the roadmap in `core_recovery/diff-to-fix/`. For every finding
— ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify
the finding, fix it in the codebase, prove the fix with tests and benchmarks, and
mark it fixed in the audit trail with evidence. The audit is not trusted blindly;
every finding is re-verified against the research corpus and the running code
before any change. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

**Repos (only two, in scope):**
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged (redacted here).

No other repository is cloned, referenced, or inspected. Reference behavior is
never guessed — it is verified against the corpus.

## Session-specific directive (this leg)

- Fix all the audit findings, **focus on MAJOR initially**, one by one, with full
  context and knowledge; update and put fixed findings in `findings/FIXED/`.
- Update all docs, counts, and matrices properly.
- Target at least 20+ fixes before session end.
- Standard SOPs from all prior sessions apply verbatim (see §13 of the source
  prompt): verify → reproduce → classify → fix → re-review → test → benchmark →
  document → push → loop. One finding, one verified fix, one push. Pull
  frequently; never force-push; leave main green.

## Key procedure excerpts (verbatim from the source prompt)

1. **Read before touching anything** — diff-to-fix/README.md, SUMMARY.md,
   SEVERITY-SCALE.md, PARAMETERS.md, CROSS-REF-MATRIX.md, every finding file,
   every module overview, every evidence artifact; the corpus line-by-line where a
   finding requires it; the core repo in full at current main. Record the audited
   commit in the worklog and in every finding touched.

2. **Independently verify each finding before fixing** — reproduce the evidence on
   current main; re-verify against the corpus artifact in full; classify:
   CONFIRMED-DIVERGENCE (fix), ALREADY-FIXED (FIXED-ALREADY), FALSE-POSITIVE
   (INVALID with evidence, no code change), OUTDATED-EVIDENCE (SUPERSEDED,
   re-audit), NEW-FINDING (new file in correct severity folder, fix this session).

3. **Resolution section template** (appended to every finding touched; findings
   are never deleted):
   Status FIXED | FIXED-ALREADY | INVALID | SUPERSEDED; fixed-in commit; verified
   by (tests + benchmark IDs / command evidence); resolution notes paragraph with
   corpus and code citations.

4. **Fixing rules** — re-implement from understanding (never paste decompiled
   code, never copy proprietary strings, cite the corpus artifact in the commit
   message); fix the cause not the symptom; match architecture and wiring; every
   fix paired with a test that fails before and passes after (unit / integration /
   structure-aware fuzz / TSan for threading / ASan-UBSan for memory); safety
   hardenings preserved (dest-overlap refusal, BitLocker legitimate-unlock-only,
   RawFixer explicit consent, 4Kn handling, cancellable IRP, SDDL gating); no
   shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs; ReFS and LDM
   remain first-class; contracts updated in the same change; pull frequently;
   token hygiene; commit and push after every coherent finding fix.

5. **Fix order** — ACUTE → SEVERE → MAJOR (largest user impact first: correctness
   before performance, data integrity before latency, silent-failure paths before
   observable inefficiencies) → MINOR (batch per module) → INFORMATIONAL (verify
   deliberate-difference rationale, mark ACKNOWLEDGED; safety hardenings left
   unchanged with one-line preservation note) → TRACKED-OPEN (resolve or push
   forward precisely).

6. **Performance** — baseline first on audit fixtures, fix, re-benchmark, accept
   only if no parameter regresses; Windows direct I/O reads multiple of 4 KiB and
   64 KiB where possible; record numbers in Resolution + SUMMARY performance
   section.

7. **Audit trail updates after every fix** — Resolution in the finding file; move
   the finding to findings/FIXED/; CROSS-REF-MATRIX cell → MATCH with commit link;
   modules/<module>.md verdict row; SUMMARY.md counts; README audited-commit
   navigation if needed; worklog.md entry with fix, SHA, proving test.

8. **Discovered divergences** — new finding file + matrix row + SUMMARY update +
   fix this session or precise tracked-open; announced in the commit message. No
   silent fixes.

9. **CI** — Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, seeded
   integration, C consumer, fuzz smoke, Win7-inclusive import allowlist gate);
   Linux green (pure-core, CLI, ASan/UBSan/TSan on touched tests, fuzz smoke); no
   continue-on-error; signing stays disabled.

10. **Docs** — CAPABILITY-MATRIX, architecture, ground-truth-contracts,
    error-fix-registry updated in the same commit as behavior changes; resolved
    TODO/FIXME/HACK/future-work notes removed; PARITY-DELTAS documents every
    deliberate divergence.

## Deliverables (from §11 of the source prompt)

Every finding FIXED / FIXED-ALREADY / INVALID / SUPERSEDED; INFORMATIONAL
ACKNOWLEDGED; TRACKED-OPEN resolved or pushed forward; matrix fully MATCH;
SUMMARY counts + risks; modules/*.md verdicts; worklog.md with SHAs + evidence;
green CI; performance numbers recorded; fail-before/pass-after tests; all changes
pushed; no finding deleted; tagged release if audit closes cleanly; worklog
handoff; diff-to-fix/ is the final evidence set.

## First 20 TODOs (from §12)

T1 pull repos + record commits + save prompt; T2 read diff-to-fix in full; T3
read core repo in full; T4 classify ACUTEs; T5 fix ACUTEs; T6 classify SEVEREs;
T7 fix SEVEREs; T8 classify MAJORs; T9 fix MAJORs; T10 performance
baseline/fix/rebench; T11 classify MINORs; T12 fix MINORs per-module batches;
T13 ACKNOWLEDGE INFORMATIONALs; T14 resolve/push-forward TRACKED-OPEN; T15
CROSS-REF-MATRIX; T16 SUMMARY; T17 modules/*.md + docs + TODO removal; T18 full
CI sweep; T19 discovered tests; T20 tag release + handoff + final sweep.

# Session DFX25 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus)

Saved at session start, token redacted. Date: 2026-10-26.

## Mandate (abridged, token redacted)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it, prove the fix with tests, and mark it
in the audit trail with evidence.

Two repos in scope only:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

User directive for this session: focus on INFORMATIONAL and tracked-open
initially, then continue one by one. Target at least 20+ fixes before session
end. Put fixed findings in the fixed directory. Update all docs, counts.

## Core SOPs

- Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document → Push → Loop.
- One finding, one verified fix, one push.
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs.
- Every fix paired with a test that fails before, passes after.
- Safety hardenings preserved (dest overlap refusal, BitLocker legit-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- Pull frequently, never force-push, keep main green.
- INFORMATIONAL: verify deliberate-difference rationale, mark ACKNOWLEDGED, no
  code change unless rationale wrong.
- Track open items: resolve or push forward with precise next step + evidence.

## Session start state

- Core HEAD: 716d121 (worklog(DFX16-M013): rebase reconciliation)
- Corpus HEAD: 03ad46c (unmoved throughout history)
- Open: 0 ACUTE, 0 SEVERE, 14 MAJOR, 19 MINOR, 22 INFORMATIONAL, 11 tracked-open
- FIXED: 149
=======
# Session 30 prompt (saved token-redacted at bootstrap; re-stamped 25→30 past same-window squats)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for every
finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN)
independently verify against the research corpus and the running code
before any change; fix confirmed divergences with fail-before/pass-after
tests; correct false findings in the audit trail rather than breaking
good code. Repos in scope (only these two):

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

Session emphasis (user instruction): fix all audit findings — minor
first, one by one, with full context and knowledge; move fixed findings
into `findings/FIXED/`; update all docs and counts properly; target at
least 20+ fixes before session end.

Lane note at bootstrap: two parallel lanes are claimed in the same
window — DFX-front claimed the full open MINOR tier (354c555, front-first
order: N-009, N-007/I-004, N-025, N-026, N-027, N-031, N-036, N-049,
N-059, N-065, N-066, N-075, N-019, N-050, N-033, N-072, N-032, N-071,
N-064) and DFX-major claimed the open MAJOR tier (02b950f). This lane
(DFX30) claims the DISJOINT TAIL of the MINOR queue, worked back-to-front
to maximize distance from DFX-front's front-first cursor: N-064, N-071,
N-032, N-072, N-033, N-019, N-050. First-landing-wins on any overlap;
pull before every fix; union-merge if a parallel landing arrives mid-fix.
After the tail slice drains, pull and absorb whatever MINORs remain
unclaimed (DFX-front front cursor may have advanced or stalled).

Key SOPs (unchanged from all prior sessions):

1. Read before touching: audit output, corpus citations (full artifact,
   not the excerpt), current main (pull — other agents may have pushed).
2. Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review →
   Test → Benchmark → Document → Push → Loop.
3. One finding, one verified fix, one push, one finding-file update.
4. Every fix paired with a test that fails before and passes after.
5. Safety hardenings preserved (destination overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable
   IRP, SDDL gating). ReFS and LDM stay first-class.
6. No shortcuts, no fallbacks that mask failure, no stubs, no mocks in
   production paths, no TODO left behind by a fix. Production grade only.
7. Win7-inclusive import gate intact; signing stays disabled; CI green
   on every push; ASan/UBSan/TSan on touched code; fuzz for parsers.
8. Audit trail is the deliverable: finding files keep their Resolutions,
   CROSS-REF-MATRIX cells go to MATCH, SUMMARY counts updated,
   modules/*.md verdicts updated, worklog.md chronological.
# Session 30 prompt (re-stamped from local 25→26→27 bootstraps after same-window ID collisions; queue unchanged: tracked-open tail) (diff-to-fix fix session — DFX27) — saved token-redacted at bootstrap

# (Re-stamped twice from the local DFX25/DFX26 bootstraps after same-window ID
# collisions with parallel lanes — see worklog DFX30-T0.)

# (This lane originally bootstrapped as DFX25; re-stamped after a same-window ID
# collision with the parallel INFO-closure lane, per the first-landing-wins protocol.)

Saved: 2026-10-26 ~05:05 UTC. Baseline: HEAD `02b950f` (origin/main at fetch =
claim tree; DFX24's MAJOR-lane claim + DFX23's MINOR-lane claim absorbed at
clone). Corpus `03ad46c` (verified at first corpus read). Baseline suite
executed green FIRST-HAND before the claim: linux-release **737/737**.

## Mandate (verbatim summary — full text in this file's history)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding against the research corpus and the running
code before any change. A false finding is corrected in the audit trail rather
than "fixed" by breaking good code.

Repos in scope (only two):
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — env/git use only, never committed, never logged]

Core protocol:
1. Read audit output first (README, SUMMARY, SEVERITY-SCALE, PARAMETERS,
   CROSS-REF-MATRIX, every finding, every module doc, evidence/).
2. For each finding: reproduce evidence → re-verify against corpus → classify
   (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
   NEW-FINDING) → only then fix.
3. Fixing rules: re-implement from understanding (never paste decompiled code);
   fix the cause not the symptom; match architecture and wiring; every fix
   paired with a test that fails before and passes after; safety hardenings are
   preserved; no shortcuts/stubs/mocks/fallbacks that mask failure; no
   hallucinated parity; ReFS and LDM stay first-class; don't break contracts
   other agents consume without updating in the same change; pull frequently,
   never force-push, leave main green; commit+push after every coherent finding
   fix (one finding, one verified fix, one push).
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (mark ACKNOWLEDGED
   with rationale confirmed; no code change unless the rationale is wrong) →
   TRACKED-OPEN (resolve or push forward precisely).
5. Performance fixes: baseline first, fix, re-benchmark, accept or revert.
   Windows direct I/O reads: multiple of 4 KiB, multiple of 64 KiB when possible.
6. Tests: unit for logic, integration on seeded fixtures, structure-aware fuzz
   for parsers, TSan for threading, ASan/UBSan for memory, CLI contract tests,
   ABI consumer tests. Test names describe behavior proven.
7. Audit trail updates after every fix: Resolution section in the finding,
   CROSS-REF-MATRIX → MATCH, modules/*.md verdict, SUMMARY counts, README if
   needed, worklog.md entry.
8. Discovered divergences: new finding file + matrix row + SUMMARY + fix or
   precise tracked-open. No silent fixes.
9. CI green both lanes; Win7-inclusive import gate intact; signing stays
   disabled.
10. Docs updated in the same commit; resolved TODO/FIXME removed;
    PARITY-DELTAS.md records deliberate divergences.
11. Deliverables: every finding terminal-state (FIXED / FIXED-ALREADY / INVALID /
    SUPERSEDED / ACKNOWLEDGED), matrix fully MATCH-populated, SUMMARY counts
    current, worklog handoff, green CI, no finding file deleted.
12. Target: 20+ fixes this session (user emphasis: severe/major initially —
    both tiers are claimed by active parallel lanes; see lane claim below).

Corpus is the only source of truth. When a finding's evidence conflicts with
the corpus, the corpus wins. The audit is a hypothesis; this session is the
verifier. No shortcuts, no samples, no assumptions, no hallucination,
production grade only.

## Session-29 lane adaptation (queue unchanged from the 27 claim: tracked-open tail) (recorded at bootstrap, per the lane protocol)

At fetch, two active lanes hold claims pushed within the last ~3 minutes:
- DFX23 (`354c555`, 04:57 UTC): the full open MINOR tier.
- DFX24 (`02b950f`, 04:58 UTC): the full open MAJOR tier.

First-landing-wins lane discipline (the protocol every prior session in
worklog.md followed) forbids claiming the same findings. This session (DFX25)
claims the DISJOINT remainder — deliverables #2 and #3 of the mandate, both
unowned at fetch:
- The INFORMATIONAL tier: 24 findings, 15 of which carry no ACKNOWLEDGED
  resolution (I-006, I-007, I-008, I-012, I-013, I-015, I-016, I-017, I-018,
  I-019, I-020, I-021, I-022, I-023, I-024) — verify each rationale against
  the corpus first-hand, mark ACKNOWLEDGED (or code-change if the rationale is
  wrong), plus the 4 thin "ACKNOWLEDGED"-only rows needing verification depth.
- The tracked-open queue: 11 OQ items — resolve to a fix this session where
  feasible, or push forward precisely with new evidence and a concrete next
  step.
- Any NEW-FINDING discovered during verification gets filed + fixed this
  session.
- If the parallel MAJOR/MINOR lanes stall (no landing observed over a long
  window), pull and absorb their remainder per first-landing-wins.
>>>>>>> 3f23f3fd4172262e0f40a1786d9366308cb62d44
