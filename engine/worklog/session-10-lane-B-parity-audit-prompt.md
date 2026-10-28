# session-10-lane-B-parity-audit-prompt.md (token redacted)

Saved per the SOP. This is the session-10 lane-B mandate prompt with the GitHub token
removed.

---

You are a senior systems auditor (25+ years) specializing in Windows storage internals,
filesystem recovery engines, C++20 production code, and clean-room parity verification
against reverse-engineered references. […]

READ-ONLY agent. Only `diff-to-fix/` and `worklog` entries are written.

Mandate: audit `heicfast/core_recovery` against `heicfast/easeus-drw-research` across
every module, engine, function, feature, API, and 50+ performance and behavioral
parameters. Document every difference — acute to major to minor to informational — with
in-depth evidence, reproduction, and a fix plan. `diff-to-fix/` is the deliverable.

Two repositories in scope and only two (token in env/git config only, never committed):
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research

[… the full 10-section SOP: parallel agents A–E; the verdict set (MATCH /
DIVERGENCE-ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL / UNVERIFIED); the diff-to-fix/
layout; the finding template; the 70-parameter taxonomy; the verification protocol
(executed > decompiled > doc > RE-probe; never assert unverified reference behavior);
the wave workflow with commit-and-push per wave + the moving-target/pull-frequently
rules; the rules (read-only, evidence or nothing, no hallucination, no shortcuts,
severity discipline, ReFS/LDM first-class, the output is the roadmap, long-horizon);
the first-20-TODO seed.]

Session-10 addendum (this lane's user instruction): "This is second session of this
work, first is already completed, now you have go look for other things which session 1
missed and continue working and remove the fixed things and go more deep, do more
comparison on og-codebase and our core and mark all remaining features, implementations,
functions, as finding too."

Executed as lane B: (1) the full-board re-verification wave ("remove the fixed things"
— convergent with the parallel lane's sweep, the unique verdicts preserved);
(2) three deep-comparison lanes (the config-layer triplet; the 1470-file og-tree
completeness sweep; the ifileproperties field-parity table); (3) executed ground-truth
probes (the preview eligibility probe; the real-.rsf parse); (4) the push-time
reconciliation with the four concurrent fix lanes + the SA-10B purge convention +
the remote's own S-050 ID collision. Read twice before beginning, per the SOP.
