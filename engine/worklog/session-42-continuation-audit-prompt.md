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
