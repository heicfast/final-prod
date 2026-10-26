# Session 26 prompt (diff-to-fix fix session — DFX26) — saved token-redacted at bootstrap

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

## Session-26 lane adaptation (recorded at bootstrap, per the lane protocol)

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
