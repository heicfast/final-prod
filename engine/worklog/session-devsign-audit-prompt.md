# NOTE: this file is the SA (dev-sign + parity re-audit) lane's prompt.
# The S7 parity-audit lane's prompt lives at worklog/session-audit-prompt.md
# (it committed first; this file was renamed from the same path to resolve
# the concurrent-lane conflict without deleting either lane's record).

# Session: merged-product dev-sign validation + full parity re-audit (2026-10-27/28)

## Mandate (redacted prompt summary)

Senior systems auditor / release engineer posture. Two-phase mandate.

**Phase 1 — dev-sign validation (the only implementation allowed):**
- Validate the merged product's driver under development signing: verify the
  driver can be test-signed and installed on real Windows environments, the
  install/uninstall/load path is production-shaped, evidence captured.
- If the repo already documents a dev-sign path → verify it works and record
  reproduction in `worklog/DEV-SIGN.md`. If not → create isolated
  `tools/dev-sign/` (script: self-signed cert, signtool SHA256 + timestamp
  server, optional test-signing mode, README with commands/postconditions/
  rollback/environments) + `.github/workflows/dev-sign.yml`
  (workflow_dispatch-only, artifact upload, no auto-install).
- Validate on a real Windows runner: install transcript, `sc query`, device
  enumeration, control-interface round-trip, uninstall transcript.
- Gaps → findings in `diff-to-fix/findings/<severity>/<ID>-devsign-<slug>.md`,
  audited against the corpus's documented reference installer behavior.
- Phase 1 committed and pushed BEFORE Phase 2 begins.

**Phase 2 — full read-only parity audit (corpus-driven scope):**
- Read both repositories END TO END (parallel agents A–E: corpus inventory;
  corpus per-surface parameters; product public surface; product deep read;
  independent verification).
- Scope comes from the corpus, NOT the prompt: "If the corpus documents a
  surface, it is in scope. If the corpus does not document a surface, it is
  not in scope."
- Verdicts: MATCH / DIVERGENCE-ACUTE|SEVERE|MAJOR|MINOR|INFORMATIONAL /
  UNVERIFIED. Only MATCH + INFORMATIONAL are acceptable end states; every
  divergence is a finding; every UNVERIFIED resolved or moved to
  `tracked-open/`.
- Output structure extends existing `diff-to-fix/`: README, SUMMARY,
  SEVERITY-SCALE, PARAMETERS, SURFACE-INVENTORY, CROSS-REF-MATRIX,
  findings/{ACUTE,SEVERE,MAJOR,MINOR,INFORMATIONAL}, modules/, evidence/,
  tracked-open/.
- SURFACE-INVENTORY.md + PARAMETERS.md are GATE documents, committed/pushed
  before any findings.
- Finding template (severity, module/surface, parameters affected, corpus
  refs, core refs @ audited commit, status, prior-audit check, summary,
  reference behavior, our behavior, gap, impact, reproduction, fix plan,
  tests, related, open questions).
- Verification protocol strength order: executed comparison > decompiled
  evidence > corpus doc evidence > independent RE probe. Never assert an
  unverified reference behavior.

## Rules (non-negotiable)
- READ-ONLY on the audit side. Writes: `diff-to-fix/`, `worklog.md`,
  `worklog/DEV-SIGN.md`, and — only if no documented dev-sign path exists —
  `tools/dev-sign/` + `.github/workflows/dev-sign.yml`.
- Respect prior work: extend, never delete/rewrite prior findings or `fixed/`.
- Corpus defines scope. No invented surfaces, no invented parameters, no
  hallucination. Evidence or nothing. No shortcuts, no samples, no
  assumptions.
- Severity consistency: ACUTE = wrong results/silent data loss/crash/security
  regression/broken ABI/transport corruption/install-blocking. MINOR =
  cosmetics/doc drift.
- Long-horizon waves, commit+push after every coherent wave, detailed
  messages, each finding's commit carries its ID + one-liner. Record audited
  commit hash in README + every finding.

## In-scope repos (only these two)
- Merged product (audit + dev-sign target): confirmed by inspection =
  **https://github.com/heicfast/core_recovery** (core engine + win layer +
  driver-consuming transport + CI + the diff-to-fix audit trail; the driver
  binary ships as prebuilt `rda.sys` from `heicfast/rda-driver` releases per
  docs/PREBUILT-ARTIFACTS.md — the repo's own documented convention)
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- Token: env/git-config only, never committed, user rotates after session.
  [REDACTED FROM THIS FILE]
- Do not clone/reference/audit any other repository. (The driver release
  assets are consumed BY the merged repo's own documented install path; the
  audit target remains core_recovery.)

## First 20 TODOs
T1 clone+confirm+prompt+worklog · T2 Phase-1 dev-sign path check + Windows
runner validation · T2b evidence analysis · T2c DEV-SIGN.md + findings +
push · T3 parallel read agents A–E · T4 SURFACE-INVENTORY + PARAMETERS gate
docs · T5 README/SEVERITY/SUMMARY/MATRIX scaffolds · T6–T18 surface waves
(corpus-determined order; highest integration-risk first: driver/transport
post-wire-fix, then engines/sessions/orchestration/writer/health/UI/aux) ·
T19 close: resolve/move UNVERIFIED, finalize summaries · T20 final
verification sweep (no source modified outside dev-sign exception, no
duplicates, no invented scope, all pushed).