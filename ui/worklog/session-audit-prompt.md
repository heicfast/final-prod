# Session 3 — Audit & Completion Prompt (token redacted)

Role: senior product engineer / design systems lead. Clean-room parity audit
against a documented reference product, then fix and complete.

Scope — exactly two repositories:
- Research corpus (reference source of truth): github.com/heicfast/easeus-drw-research
- Application under audit (modify): github.com/heicfast/recovery-wizard-ui-new
- GitHub token: REDACTED (env/git config only, never committed, never logged)

## Mandate (ordered)
1. Read the entire research corpus line by line — architecture docs, RE raw
   sources, artifacts, all 63 screenshots (VLM extraction). No guessing.
2. Read the entire application repository line by line.
3. Cross-check app vs reference on every behavioral, structural, visual
   parameter; record divergences with evidence in docs/AUDIT-REPORT.md.
4. Fix every divergence and all slop (duplication, dead code, invented
   abstractions, inconsistent patterns, stubs).
5. Complete every remaining implementation. No TODO, no stubs, no placeholders.
6. Meet the 50+ parameter quality grid per surface, verified with evidence.

## Known divergence to verify and correct first
- Reference lost-data flow: select device → scan starts immediately (quick
  pass → automatic deep/advanced pass). NO intermediate setup/config step with
  a mode selector (e.g. a "deep scan" choice) between device selection and
  scan start.
- Current build inserts such an intermediate step. Correct it to match the
  reference exactly (sequence, transitions, visible state, controls, copy).
  Keep evidence-backed affordances the reference does expose (cancel/pause).
  Add automated coverage for the corrected sequence.

## Theme adoption (visual identity source of truth)
Fetch ONLY these two raw files (do not browse/clone the origin repo):
- https://raw.githubusercontent.com/heicfast/diskgenie-v2/refs/heads/main/src/theme/themes.ts
- https://raw.githubusercontent.com/heicfast/diskgenie-v2/refs/heads/main/src/theme/tokens.css
Adopt as the project's design tokens: map into the app's token system; layout/
structure/behavior follow the reference; visual identity (colors, gradients,
typography, radii, elevation, motion) follows the adopted tokens. Adjust the
MAPPING for accessibility, never the source files; document choices. Light +
dark variants verified; high-contrast derived if not defined. Single token
source of truth in-repo + snapshot test that fails on drift.

## Audit axes
Behavior / Structure / Layout / Visual parameters / States / Interaction
patterns / Motion & feedback / Accessibility / Strings & localization (en +
zh-CN) / Data flow / Code quality / Performance / Test coverage.

## SOP
Research → Extract → Plan → Audit → Review → Refactor → Fix → Re-test →
Validate → Compare → Loop. Async discipline at the data layer. Accessibility
mandatory. Perf budgets measured in CI. Commit and push every coherent unit
with evidence-citing messages. worklog.md maintained continuously. Token
hygiene. Never force-push; keep main green. Dependencies pinned.

## Deliverables
docs/AUDIT-REPORT.md, docs/ACCEPTANCE.md (50+ grid per surface), docs/ADR/
(incl. corrected lost-data flow + theme adoption), corrected flow with tests,
adopted theme with snapshot test, refactored components, complete states,
green Windows CI (tsc/eslint/prettier/ast-grep/knip/jscpd/vitest+coverage/
Playwright smoke/axe), docs updated + stale docs deleted, NSIS release
workflow, worklog handoff, everything pushed.
