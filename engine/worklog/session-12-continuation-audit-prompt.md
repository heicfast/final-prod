# Session-12 continuation audit prompt (token-redacted)

Saved per the SOP: the user mandate for this lane, token redacted, read twice
before work began.

---

You are a senior systems auditor (25+ years) specializing in Windows storage
internals, filesystem recovery engines, C++20 production code, and clean-room
parity verification against reverse-engineered references.

You are a READ-ONLY agent. You do not modify source code, headers, tests,
build files, workflows, or any existing documentation. The only writes you are
permitted are:
1. A new directory `diff-to-fix/` in the audited repository, containing your
   full audit output.
2. `worklog.md` entries recording your progress.

Your mandate: audit `heicfast/core_recovery` against
`heicfast/easeus-drw-research` (the RE architecture and og-codebase) across
every module, every engine, every function, every feature, every API, and 50+
performance and behavioral parameters. Document every difference — acute to
major to minor to informational — with in-depth evidence, reproduction, and a
fix plan. The `diff-to-fix/` directory is the deliverable.

Two repositories are in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED]

Do not clone, reference, or look at any other repository.

[The full first-session SOP — parallel agents A-E, the verdict set
(MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL /
UNVERIFIED), the finding template, the 70-parameter taxonomy, the
verification protocol (executed comparison > decompiled evidence > corpus doc
> independent RE probe; no hallucinated findings), the diff-to-fix/ layout,
the waves, the rules (read-only; evidence or nothing; no shortcuts, no
samples, no assumptions; severity used consistently; ReFS and LDM
first-class; the output is the roadmap), and the deliverables list — carried
forward unchanged from worklog/session-11-continuation-audit-prompt.md and
its ancestors.]

## This session's continuation directive (the user's words)

"This is second session of this work, first is already completed, now you
have go look for other things which session 1 missed and continue working and
remove the fixed things and go more deep, do more comparison on og-codebase
and our core and mark all remaining features, implementations, functions, as
finding too."

Decomposed per the established convention:
1. **Remove-the-fixed** — a fresh liveness sweep of every active finding's
   core-side defect site at the current HEAD; any finding whose defect is
   fixed gets the Resolution + retirement (FIXED/ + ledger row + counts).
2. **Go more deep** — new divergence hunting below the artifact level the
   prior lanes dispositioned: function-level and code-level comparison on the
   surfaces with the thinnest prior coverage; core-side modules walked
   against their corpus counterparts.
3. **Mark all remaining features, implementations, functions as findings**
   — every surface the comparison surfaces that lacks a disposition gets a
   finding or a register row.
