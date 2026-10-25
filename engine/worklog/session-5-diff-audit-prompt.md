# Session 5 (diff-audit continuation) Prompt — token redacted

Saved at session start. Original user prompt verbatim except the GitHub token
(`ghp_[REDACTED]`) and IM gateway metadata. Read twice before beginning.

---

Role: senior systems auditor (25+ years; Windows storage internals, filesystem
recovery engines, C++20 production code, clean-room parity verification against
reverse-engineered references). READ-ONLY agent: the only writes permitted are
(1) a new `diff-to-fix/` directory in the audited repository, (2) `worklog.md`
entries. No source, headers, tests, build files, workflows, or existing docs are
modified. No fix is applied.

Mandate: audit `heicfast/core_recovery` against `heicfast/easeus-drw-research`
(the RE architecture and og-codebase) across every module, engine, function,
feature, API, and 50+ performance and behavioral parameters. Document every
difference — acute to major to minor to informational — with in-depth evidence,
reproduction, and a fix plan. `diff-to-fix/` is the deliverable.

Two repositories in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ghp_[REDACTED]

## Session-5 continuation mandate (this session)

"This is continuous session of this work, multiple agents are already working and
completed, now you have go look for other things which earlier sessions missed and
continue working and update the fixed things and go more deep, do more comparison on
og-codebase and our core and mark all remaining features, implementations, functions,
as finding too. Focus on every minor detailing too."

Operationalized:
1. Verify fixes that landed after the session-4 audited commit; update dispositions.
2. Gap-analysis the corpus citation map (artifact files vs findings/evidence cites);
   every in-scope artifact never cited by any prior session gets read this session.
3. Gap-analysis the core tree the same way (never-cited core files get audited).
4. Decode the tracked-open OQs where feasible (S4-1 grading extension table dump,
   S4-2 RAW naming-hook consumption, plus older OQs).
5. Mark every remaining og feature/function/implementation as a finding (including
   INFORMATIONAL scope-exclusion registers) — "mark all remaining ... as finding too".
6. Minor detailing: constants, naming, doc drift, ordering, units.

## Core methodology (unchanged from sessions 1-4)

- Verdicts: MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL /
  UNVERIFIED. Only MATCH and INFORMATIONAL are acceptable end states.
- 70-parameter taxonomy (diff-to-fix/PARAMETERS.md), per-module scoring.
- Verification ladder: executed comparison > decompiled evidence (file+address+quote)
  > corpus doc citation (file+line+quote) > independent RE probe. No hallucinated
  findings; UNVERIFIED goes to tracked-open with a precise question.
- Finding template exactly as sessions 1-4 (see any findings/ file).
- ReFS and LDM are first-class.
- Severity discipline: ACUTE = wrong recovery results / silent data loss / crashes /
  security regressions. MINOR = cosmetics, doc drift, low-impact inefficiency.
- Commit and push after every coherent wave; every finding cites the audited commit
  hash; README states the audited commit.
- worklog.md append-only with Task IDs.

## First TODOs (session 5)

- S5-T1: Bootstrap — pull both repos, record hashes (core 7cf6754 / corpus 03ad46c),
  save this prompt, worklog entry, baseline suite evidence.
- S5-T2: Reading wave — parallel agents on the never-cited corpus artifacts
  (RAWSearchDLL family-brance/bruteforce/filegen/rawspeed/special/xjq,
  per-engine-function-map, rsf-record4, ifileproperties-part3, config-files
  never-cited, UIUX never-cited, MOM-modules never-cited, WinPE, og-codebase data
  files) and never-cited core files (cr_elevate, cr_destio, cr_snapshot, cr_cached,
  cr_decomp, crypto, err).
- S5-T3..Tn: per-lane audits, findings, evidence, matrix/module/SUMMARY updates.
- S5-close: resolve or track every UNVERIFIED; finalize SUMMARY/worklog; push.
