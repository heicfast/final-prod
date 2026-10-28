# MINOR N-154 — Bookkeeping-truth batch #2: the N-079 ID collision (an in-source comment cites a filing the ledger gives to a different finding), the GPT-salvage strictness difference unregistered, the module-row structure gaps (tree/err/reghive/apply), and the stale suite-count claims (1027 vs the 1117 the tree actually runs)

**Severity:** MINOR
**Module:** err (the log module's audit trail), table (GPT salvage), diff-to-fix board structure, worklog/doc truth
**Parameters affected:** Q68 (documentation accuracy — in-source audit-trail cites and board docs must be truthful), Q65 (the module-row completeness class), C20/A25-adjacent (the salvage strictness is an unregistered deliberate difference)
**Corpus references:** `re-workspace/reversed/ghidra-partition/Partition_decompiled.c` FUN_180006e20:4517-4560 (the og's two-step GPT salvage, step 1 — the hardcoded 0x80 entry stride the core's stricter `entry_size == 128` gate documents itself against, in `src/core/table/cr_table_parse.cpp:70-81`'s comment); `re-workspace/artifacts/MOM-modules/module-internals-wave2--ldm-reglib-ctbfile.md:73-78` (the RegLib hive-WRITE half — see I-033); `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:70-75` (the 5th record — see N-142)
**Core references:** audited `c7fa527c` — (1) `src/core/err/cr_log.cpp:30-34` (the comment now at :30-33 at the post-rebase HEAD) (the comment "…red on the ASan lane; filed as N-079" for the Site-leak deque fix) vs `diff-to-fix/FIXED-LEDGER.md:262` (N-079 = "ntfs-recycle-bin-scope-name-prefix-matched-everywhere") vs `diff-to-fix/SUMMARY.md:1326/:1341/:1644` (the N-079 references are the recycle-bin/FatScanStats context) — one number, two findings; (2) `src/core/table/cr_table_parse.cpp:70-81` (the salvage comment documents a deliberate strictness difference — "Stricter than the og on one point: we require the declared entry_size == 128 (the og hardcodes the 0x80 stride…)" — with no PARITY-DELTAS D-row registering it; `modules/ldm.md:53` says plain "M on two-step GPT salvage"); (3) `diff-to-fix/modules/` census: no `tree.md`, no `err.md`, no `reghive.md`, no `apply.md` (I-030's own "module-row structure gaps" class — the liveness sweep confirms the reghive/apply rows still absent at HEAD); (4) the worklog/SUMMARY blocks carrying "suite 1027/1027 green (Linux release)" as the standing claim vs the executed truth at this HEAD: **1117/1117** (`evidence/executions/s12-build-execution.md` — 9 targets, 0 warnings, -Werror active, ctest exit 0, 64.2 s)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Four bookkeeping-truth defects, one class (the N-135 precedent): (1) an in-source audit comment cites "filed as N-079" for the cr_log Site-leak fix, but the ledger's N-079 is the NTFS recycle-bin-scope finding — the Site-leak fix's real filing number is unrecorded anywhere; (2) the GPT salvage's deliberate strictness difference (core requires `entry_size == 128`; the og hardcodes the 0x80 stride) is documented only in a code comment, with no PARITY-DELTAS registration and a plain-M module row; (3) four core modules (tree, err, reghive, apply) still have no module doc rows — I-030 is partially fixed; (4) the standing suite-count claim (1027) is stale — the tree runs 1117 tests.

## What the reference does / What our core does

Per item, the two sides and the defect:

1. **N-079 collision.** Core: `src/core/err/cr_log.cpp:30-34` (the comment now at :30-33 at the post-rebase HEAD) — the deque/pool comment's "filed as N-079". Board: `FIXED-LEDGER.md:259` — `| N-079 | MINOR | … | findings/FIXED/N-079-ntfs-recycle-bin-scope-name-prefix-matched-everywhere.md |`; SUMMARY's N-079 mentions are the recycle-bin context. The Site-leak fix (the N-062 landing's LeakSanitizer red) has no discoverable filing under any other number — grep the trail for "Site leak"/"deque" finds only this comment. Defect: the in-source audit-trail cite is false (a reader auditing cr_log.cpp will chase the wrong finding).
2. **Salvage strictness unregistered.** Core: `cr_table_parse.cpp:72-81` comment states the deliberate difference in full. Board: `modules/ldm.md:53` — "`M` on two-step GPT salvage + backup-at-end" — no D-row, no finding, no PARITY-DELTAS entry. Defect: a reviewed-and-accepted difference that the board does not register (the exact class PARITY-DELTAS exists for).
3. **Module-row gaps.** Board: 28 module docs; `src/core/tree/` (4 files, 1155 LOC), `src/core/err/` (3 files, 231 LOC), `src/core/reghive/` (2 files, 670 LOC), `src/core/apply/` (586 LOC) have no rows — their verdicts are scattered across orchestration.md/visualization.md/ldm.md/session.md and findings (agent-verified COVERED-in-substance this session, but the board's own I-030 row admits the structural gap; the liveness sweep confirms the reghive/apply rows are still absent at HEAD).
4. **Stale suite count.** Board: the worklog parity-close entry and SUMMARY-era blocks cite "1027/1027 green (Linux release)" as the standing suite size. Executed truth this session (from-scratch configure+build+ctest at `c7fa527c`): **1117 tests, 1117 passed, 0 failed, 0 skipped** (`evidence/executions/s12-build-execution.md`). Defect: doc drift on the quality-gate number — future sessions re-verifying "1027" would mis-report.

## Gap

Falsifiable per item: (1) `grep -n "N-079" src/core/err/cr_log.cpp` hits the mis-cite while the ledger row names a different finding; (2) `grep -rn "entry_size\|0x80" docs/PARITY-DELTAS.md` finds no salvage row; (3) `ls diff-to-fix/modules/` lacks the four rows; (4) the executed count is 1117, not 1027.

## Impact

All four are board/doc-truth defects: they cost auditor and engineer time (wrong finding chased, difference unregistered, module coverage invisible, quality gate mis-stated) but no recovery behavior. The N-079 collision is the worst of them because it lives in SOURCE-code comments — the class the repo's own audit discipline treats as load-bearing.

## Reproduction / verification

Executed (this session, first-hand): every grep/cite above; the from-scratch build+ctest run recorded in `evidence/executions/s12-build-execution.md`; the module-doc census and the liveness sweep's I-030 confirmation (`evidence/s12-liveness-sweep.md`).

## Fix plan (for the engineering team)

1. N-079: renumber the Site-leak deque finding (the next free N-number at fix time) in `cr_log.cpp`'s comment, or reword to cite the N-062 landing commit if the fix was absorbed there — one source-comment edit + a ledger note recording the collision's resolution. (This is the single item requiring a source-code touch; it is a comment, not behavior.)
2. Salvage strictness: add the PARITY-DELTAS D-row (one paragraph: the difference, the rationale — mis-sized arrays on corrupt non-128 headers, the risk posture) and downgrade `modules/ldm.md:53`'s cell to "M (with D-<n> strictness register)".
3. Module rows: add `modules/tree.md`, `modules/err.md`, `modules/reghive.md`, `modules/apply.md` from this session's coverage census (the substance exists; the rows are transcription) — closes I-030's remaining half.
4. Suite count: replace the standing "1027/1027" claims with the count-at-HEAD convention (or better: "suite green at <commit>, N tests" pinned per session) in the worklog/SUMMARY live blocks.

## Tests to add

- A docs/CI check in the same class S-050's plan proposes: every "filed as <ID>" in-source cite resolves to a ledger row of the same title-class (grep-based; would have caught the N-079 collision), and every PARITY-DELTAS-requiring comment keyword ("Stricter than the og", "deliberate", "superset") has a D-row.

## Related findings

- N-135 (bookkeeping-truth batch #1 — same class), I-030 (the module-row gap owner), N-132 (its API-DELTA row-truth residuals — the liveness sweep's partial-fix set), N-138 (the OQ status-line residual), S-054 (the regression the green-suite claim masked — the count drift is its process-truth companion)

## Open questions

The Site-leak fix's true filing identity (the ledger has no row for it under any number — the fix-time disposition is either a renumber-in-comment or an N-062-absorption note; both are recorded options, the choice is the fixer's).

---

## Post-rebase renumber note (2026-10-29, the hottest parallel window)

This finding was filed under the N-143 number at the pre-rebase HEAD `c7fa527c`;
the parallel continuation/fix lanes pushed first with their own N-143 filing
(their N-143 landed earlier and keeps the number per the N-129 collision
convention) — renumbered to N-154 at the reconciliation. The defect site was
re-verified LIVE at the post-rebase HEAD `cd883754` (grep + direct read, cites
re-pinned where lines drifted); the finding's substance is unchanged.
