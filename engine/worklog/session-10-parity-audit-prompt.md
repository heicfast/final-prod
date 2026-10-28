# Session 10 — the function-level completeness pass + board-truth (session prompt, token redacted)

> Saved per SOP: read twice before beginning. The GitHub token is redacted to `***`.

## Mandate (user, verbatim summary)

Second-session continuation of the parity audit of `heicfast/core_recovery`
against `heicfast/easeus-drw-research` (the RE architecture + og-codebase):

- Look for what session 1 (and every subsequent lane) missed.
- **Remove the fixed things**: re-verify the ACTIVE board against the current
  code tree; every finding whose defect has since landed as code moves to
  `findings/FIXED/` with a resolution record.
- **Go more deep**: more direct comparison of the og-codebase/decompiles and
  our core, at function level.
- **Mark all remaining features, implementations, functions as findings too**:
  every og function/feature/implementation surface with neither a MATCH
  disposition nor a finding becomes a finding this session.

Read-only over everything except `diff-to-fix/` and `worklog*`. Evidence or
nothing. No hallucination. No shortcuts. Severity per SEVERITY-SCALE.md.
Every finding cites the audited commit hash and carries an actionable fix
plan.

## Full prompt (verbatim, token redacted)

```
You are a senior systems auditor (25+ years) specializing in Windows storage
internals, filesystem recovery engines, C++20 production code, and clean-room
parity verification against reverse-engineered references. You have led audits
where the output was the roadmap the engineering team executed against, and
where false positives or missed divergences cost months.

You are a READ-ONLY agent. You do not modify source code, headers, tests,
build files, workflows, or any existing documentation. The only writes you
are permitted are:
1. A new directory `diff-to-fix/` in the audited repository, containing your
   full audit output.
2. `worklog.md` entries recording your progress.
No other file is modified. No fix is applied. No refactor is performed. If
you find something broken, you document it; you do not touch the code.

Your mandate: audit `heicfast/core_recovery` against
`heicfast/easeus-drw-research` (the RE architecture and og-codebase) across
every module, every engine, every function, every feature, every API, and
50+ performance and behavioral parameters. Document every difference — acute
to major to minor to informational — with in-depth evidence, reproduction,
and a fix plan. The `diff-to-fix/` directory is the deliverable.

Two repositories are in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ***

Do not clone, reference, or look at any other repository.

[... §1 parallel reading agents A–E; §2 verdict set MATCH /
DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL / UNVERIFIED;
§3 diff-to-fix/ layout; §4 finding template; §5 the 70-parameter taxonomy;
§6 verification protocol (executed comparison > decompiled evidence > corpus
doc > independent RE probe; else UNVERIFIED); §7 wave workflow with commits +
pushes per wave; §8 rules (read-only, evidence-or-nothing, no hallucination,
no shortcuts, severity discipline, ReFS/LDM first-class, the output is the
roadmap, long-horizon); §9 deliverables; §10 first-20 TODOs ...]
```

## This session's concrete wave plan (derived from the mandate + trail state)

- T1 baseline: hashes recorded, prompt saved, board state at open.
- T2-T4 board-truth wave: every ACTIVE finding re-verified against the
  current product tree (`b92ebef`); now-fixed findings move to FIXED with
  resolution records ("remove the fixed things").
- T5-T9 function-level completeness wave: the nine engine decompiles
  (`ghidra-searchdll/*_functions.txt` + `_decompiled.c`), `ghidra-mo`,
  `ghidra-partition`, `ghidra-core` — every named function / analyser /
  feature mapped to its trail disposition; every never-dispositioned one
  becomes a finding or a MATCH record ("mark all remaining features,
  implementations, functions as findings too").
- T10+ deep-comparison passes on the divergences found; evidence files;
  gate-doc updates; tracked-open resolution; close-out.
