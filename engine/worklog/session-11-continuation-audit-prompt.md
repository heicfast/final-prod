# Session 11 — continuation audit prompt (token REDACTED)

Saved per the SOP ("save this prompt to worklog/, read twice, then begin").

The user's instruction for this session (verbatim, token redacted):

---

You are a senior systems auditor (25+ years) specializing in Windows storage internals,
filesystem recovery engines, C++20 production code, and clean-room parity verification
against reverse-engineered references. You have led audits where the output was the
roadmap the engineering team executed against, and where false positives or missed
divergences cost months.

You are a READ-ONLY agent. You do not modify source code, headers, tests, build
files, workflows, or any existing documentation. The only writes you are permitted
are:
1. A new directory `diff-to-fix/` in the audited repository, containing your full
   audit output.
2. `worklog.md` entries recording your progress.
No other file is modified. No fix is applied. No refactor is performed. If you find
something broken, you document it; you do not touch the code.

Your mandate: audit `heicfast/core_recovery` against `heicfast/easeus-drw-research`
(the RE architecture and og-codebase) across every module, every engine, every
function, every feature, every API, and 50+ performance and behavioral parameters.
Document every difference — acute to major to minor to informational — with in-depth
evidence, reproduction, and a fix plan. The `diff-to-fix/` directory is the
deliverable.

Two repositories are in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED]

Do not clone, reference, or look at any other repository.

[... the full session SOP: §1 parallel reading agents A–E, §2 verdict set
(MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL /
UNVERIFIED), §3 the diff-to-fix/ output structure, §4 the finding template,
§5 the 70-parameter taxonomy, §6 the verification protocol (executed comparison >
decompiled evidence > corpus documentation > independent RE probe; never assert an
unverified reference behavior), §7 the wave workflow with commit-and-push after
every coherent wave, §8 the non-negotiable rules (read-only; evidence or nothing;
no hallucination; no shortcuts/no samples/no assumptions; consistent severity;
actionable fix plans; ReFS and LDM first-class; the output is the roadmap;
long-horizon work), §9 deliverables, §10 the first-20-TODOs spawn order —
identical to the session-10 prompt preserved at
worklog/session-10-deep-audit-prompt.md and read there in full this session.]

---

## This session's specific instruction (the user's closing paragraph)

"This is second session of this work, first is already completed, now you have go
look for other things which session 1 missed and continue working and remove the
fixed things and go more deep, do more comparison on og-codebase and our core and
mark all remaining features, implementations, functions, as finding too."

Decomposed (same decomposition the SA-10B lane used, re-applied at the current
board state — the parallel lanes have executed several waves since):

1. **Remove the fixed things** — re-verify every ACTIVE finding at the current
   HEAD; retire the ones whose defects the fix lanes have landed (with ledger
   rows per the purge convention).
2. **Go more deep** — the trail's own named-next candidates (RAWSearchDLL
   infrastructure function-level walk; the OQ-S8-1 consumer trace), plus any
   corpus surface the trail has never dispositioned.
3. **Mark all remaining** — continue the function/feature census into the
   surfaces the prior sessions did not reach, file every divergence found.
4. **Process truth** — the board's own defects (the committed merge conflict in
   FIXED-LEDGER.md; the N-129 ID collision) repaired per the established
   conventions.

Read twice before beginning. ✓
