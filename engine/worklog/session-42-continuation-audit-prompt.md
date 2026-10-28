# worklog/session-42-continuation-audit-prompt.md — TWO parallel S42 lanes saved prompts under this filename

> **[S42-B merge note (2026-10-29):]** two lanes ran the session-42 continuation
> mandate in parallel on the same baseline (core `869da779` / corpus `be0cfe9`):
> the S42-MAIN lane (their record first pushed — the S-048/M-117/N-152 fixes,
> the INFO restoration, the HFS+ resolutions, N-154..N-157, M-118/M-119, N-142,
> I-035/I-036, M-120) and the S42-B lane (this lane — the liveness battery, the
> five read agents, N-158/159/160 [renumbered from N-154/155/156 at the merge],
> I-037 [from I-035], the N-150 addendum, the M-117 collision record). Both
> prompts are preserved below, the S42-B one first. Both lanes' worklog entries
> and SUMMARY blocks record their own product per the both-sides-kept convention.

---

## The S42-B lane's prompt (this lane)

# Session-42 continuation audit prompt (token redacted)

Saved at session open per the standing SOP. The user's mandate (verbatim,
token redacted — the token lives only in env/git config, never committed):

> You are a senior systems auditor (25+ years) specializing in Windows storage
> internals, filesystem recovery engines, C++20 production code, and clean-room
> parity verification against reverse-engineered references. [...]
>
> Your mandate: audit `heicfast/core_recovery` against
> `heicfast/easeus-drw-research` (the RE architecture and og-codebase) across
> every module, every engine, every function, every feature, every API, and
> 50+ performance and behavioral parameters. Document every difference — acute
> to major to minor to informational — with in-depth evidence, reproduction,
> and a fix plan. The `diff-to-fix/` directory is the deliverable.
>
> [.. the full SOP: read-only, evidence-or-nothing, no hallucination, no
> shortcuts/samples/assumptions, severity used consistently, ReFS and LDM
> first-class, the output is the roadmap, work long-horizon, commit and push
> often ..]
>
> This is second session of this work, first is already completed, now you
> have go look for other things which session 1 missed and continue working
> and remove the fixed things and go more deep, do more comparison on
> og-codebase and our core and mark all remaining features, implementations,
> functions, as finding too.

## The three asks (the continuation-audit mandate, same shape as the S41/S11/S10 lanes)

1. **Remove-the-fixed** — the liveness battery over every active finding at
   the session's pinned core commit; retire/verify each.
2. **Go-deeper** — comparison surfaces no prior lane has covered.
3. **Mark-all-remaining** — every remaining feature/implementation/function
   surface either has a finding, a MATCH cell, or a deliberate-delta register
   row; nothing undispositioned.

## Pins

- core: `869da77958d9d41df44ca15dc8e45f5e5f0af9c4` (HEAD at session open)
- corpus: `be0cfe9` (unchanged since the prior lanes)

## Hygiene

- READ-ONLY on product code: only `diff-to-fix/` and `worklog*` are written.
- The GitHub token is env/git-config only; this file carries none.

---

## The S42-MAIN lane's prompt (the parallel lane, pushed first)

# Session-42 continuation-audit prompt (saved per the mandate; token redacted; IM gateway metadata stripped)
# Baseline at save: core main = 869da779 (the N-153 retirement-bookkeeping tip), corpus = be0cfe9. Lane: session-42, the user's continuing "second session" directive re-applied at the current board — find what prior sessions missed, remove the fixed things, go deeper, more og-vs-core comparison, mark all remaining features/implementations/functions as findings too.

You are a senior systems auditor (25+ years) specializing in Windows storage internals, filesystem
recovery engines, C++20 production code, and clean-room parity verification against
reverse-engineered references. You have led audits where the output was the roadmap the engineering
team executed against, and where false positives or missed divergences cost months.

You are a READ-ONLY agent. You do not modify source code, headers, tests, build files, workflows, or
any existing documentation. The only writes you are permitted are:
1. A new directory `diff-to-fix/` in the audited repository, containing your full audit output.
2. `worklog.md` entries recording your progress.
No other file is modified. No fix is applied. No refactor is performed. If you find something
broken, you document it; you do not touch the code.

Your mandate: audit `heicfast/core_recovery` against `heicfast/easeus-drw-research` (the RE
architecture and og-codebase) across every module, every engine, every function, every feature,
every API, and 50+ performance and behavioral parameters. Document every difference — acute to
major to minor to informational — with in-depth evidence, reproduction, and a fix plan. The
`diff-to-fix/` directory is the deliverable.

Two repositories are in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED]

Do not clone, reference, or look at any other repository.

[The full standing SOP text — verdict set, output directory layout, finding template, the 70-parameter
taxonomy, verification protocol, waves, rules, deliverables, first-20-TODOs — is preserved verbatim in
worklog/session-diff-audit-prompt.md and session-41-continuation-audit-prompt.md; not re-duplicated
here. The operative deltas for this lane follow.]

Continuation directive (this session, per the user):
"This is second session of this work, first is already completed, now you have go look for other
things which session 1 missed and continue working and remove the fixed things and go more deep, do
more comparison on og-codebase and our core and mark all remaining features, implementations,
functions, as finding too."

Session-42 lane reading of the directive (the S41 precedent applied at the new board):
1. Remove-the-fixed battery over every then-active finding at the current HEAD.
2. The deeper comparison passes no prior lane executed (un-dispositioned surfaces first: the
   config-layer residue — shellexe.ini / TreeType / themdark.ini / the empty DRWLog channel —
   the UNVERIFIED matrix residuals, and the executed-verification pass at HEAD).
3. Mark-all-remaining: every surface the trail still leaves without a disposition row becomes a
   register entry or finding.
4. Evidence or nothing; no hallucination; no shortcuts; severity consistent; read-only guarantee
   per-commit.
