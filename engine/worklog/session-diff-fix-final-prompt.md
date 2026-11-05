# The diff-to-fix closing session — the fix mandate (token redacted)

Baseline recorded at open:
- Core HEAD `b478a4b` (trail-only commits on top of product tip `b92ebef`;
  the session-8 audit's cited product code `35adcb9` is an ancestor).
- Corpus (read-only ground truth) `be0cfe9` — unchanged, verified by fetch.
- Board at open: **75 active (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) /
  250 FIXED / 2 tracked-open (OQ-S4-3, OQ-S8-1)**.
- Baseline build: linux-release preset; ctest **1023/1023 green**.

## Mandate (abridged, full text in the session transcript)

Execute the roadmap in `diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the
finding against the research corpus (the only source of truth for the
reference) and the running code, fix it, prove the fix with tests, and
record the Resolution in the audit trail. A false finding is corrected
(INVALID) rather than "fixed" by breaking good code.

Classification before any fix: CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING.

Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN;
within a tier, correctness before performance, data integrity before
latency, silent-failure paths before observable inefficiencies.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research

The rda-driver repo is OUT of this session's grant — driver-repo findings
are dispositioned in-file, not fixed here.

Non-negotiables: re-implement from understanding (never paste decompiled
code); fix the cause not the symptom; every fix paired with a test that
fails before and passes after; safety hardenings preserved; no stubs, no
mocks, no TODOs; one finding = one verified fix = one push; pull
frequently, never force-push, leave main green; token never committed
or logged.
