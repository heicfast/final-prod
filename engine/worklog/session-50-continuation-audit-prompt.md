# Session-49 continuation audit prompt (token redacted)

## IM Chat Context (gateway metadata)
- session_id: web-2e2234ed-2f4b-4010-90f9-34258184f31a
- chat_id: b5fb5697-23ec-4e1a-9659-d9efa794208a
- channel: zai-web
- trace_id: 1a184a0375675df1

## Mandate (abridged, token redacted)

Senior systems auditor (25+ years), READ-ONLY. Audit `heicfast/core_recovery`
against `heicfast/easeus-drw-research` (the RE architecture and og-codebase)
across every module, engine, function, feature, API, and 50+ performance and
behavioral parameters. The only writes permitted: `diff-to-fix/` (the audit
output) and `worklog.md` entries. No source code, headers, tests, build files,
workflows, or existing docs are modified.

Second-session continuation directive (the user's words): "This is second
session of this work, first is already completed, now you have go look for
other things which session 1 missed and continue working and remove the fixed
things and go more deep, do more comparison on og-codebase and our core and
mark all remaining features, implementations, functions, as finding too."

The three asks, as executed by the prior continuation lanes (S42/S42-B/S42-C/
S12, whose precedent this lane follows):
1. **Remove-the-fixed** — the liveness battery: every active finding re-verified
   against the pinned HEAD; falsely-active findings retired to findings/FIXED/
   with Resolution addenda.
2. **Go-deeper** — new comparison work on surfaces the prior lanes missed:
   the tracked-open OQ probes (OQ-S42-1/OQ-S42-2/OQ-S42-3), the newest fix
   landings' corpus parity (the 25-landing window since 869da779), and the
   under-cited artifacts.
3. **Mark-all-remaining** — every surfaced gap that is a divergence gets a
   finding file; every unresolvable question gets a tracked-open entry with a
   precise next probe.

Verdict set: MATCH / DIVERGENCE-ACUTE / DIVERGENCE-SEVERE / DIVERGENCE-MAJOR /
DIVERGENCE-MINOR / DIVERGENCE-INFORMATIONAL / UNVERIFIED. Evidence or nothing;
no hallucination; no shortcuts; no samples; no assumptions. Severity used
consistently; acute reserved for wrong recovery results, silent data loss,
crashes, security regressions. Fix plans actionable and test-scoped. ReFS and
LDM first-class. The output is the roadmap.

Repos in scope and only two:
- Core (audited, read-only): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed (REDACTED here).

House rules inherited: SA-9 earlier-push-wins on ID collisions; territory
claims posted before work; the audited commit hash pinned in README and every
finding; pull frequently; do not chase moving targets (board statements pinned
at this lane's pin; later landings recorded as convergence notes); read-only
guarantee verified per commit (the delta vs origin/main = diff-to-fix/ +
worklog* only).
