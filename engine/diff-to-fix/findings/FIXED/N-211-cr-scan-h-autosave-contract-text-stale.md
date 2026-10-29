# MINOR N-211 — the cr_scan.h autosave_dir contract comment still says "a cancelled run never auto-saves," contradicting the landed S-057 stop-window auto-save in rrecov.cpp

**Severity:** MINOR
**Module:** orchestrator (API contract documentation)
**Parameters affected:** P46 (documentation of the contract), S32-adjacent (cancel-path persistence semantics as documented)
**Corpus references:** (n/a — a core-internal doc-truth defect; the og-side flow is S-057's decode: the stop path's own organize into the shared completion handler, DRWUI :1549399-421 → :339076-79 → FUN_140395e50, gates :612571-587 with no stopped-check)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` — `include/cr/orchestrator/cr_scan.h:316-326` (the re-verified pin at `0da3490b`) (the autosave_dir field contract: "cr_ui + the CLI honor this field on run() COMPLETION (rc == OK; **a cancelled run never auto-saves** - the og's flow is the completion window)"); `src/app/rrecov.cpp:1927-1936` (S-057's landing: "the og's stop window auto-saves … the stopped scan's partial results land in the auto session exactly like a finished run's" — `autosave_if_due(true)` on the cancel path)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The `autosave_dir` field's contract comment in `cr_scan.h` states that a cancelled run never auto-saves and that the field is honored "on run() COMPLETION (rc == OK)". The CLI has auto-saved cancelled scans into the auto session since S-057 landed (the og's stop-window flow: partial results land exactly like a finished run's). An engineer implementing a host against the header contract would disable or mis-handle the stop-window save. The doc_truth_gate's doc-truth checks cover `docs/*.md` and README, not header comments — this drift class has no gate, and no finding owns it.

## What the reference does

n/a (core-internal doc drift; the og-side behavior is the S-057 decode cited above and already landed faithfully in the CLI).

## What our core does

The code is correct and og-faithful (`rrecov.cpp:1927-1936`); only the header text is wrong. The comment's parenthetical "(rc == OK; a cancelled run never auto-saves - the og's flow is the completion window)" predates S-057's landing and was never re-truthed; the same comment block's "The WRITE is host-side file I/O by architecture" sentence remains accurate.

## Gap

Falsifiable by reading: `cr_scan.h:321-322` asserts cancelled-never-auto-saves; `rrecov.cpp:1927-1936` implements cancelled-auto-saves-when-configured-and-results-found. The two statements cannot both be true.

## Impact

API-contract trust: a host integrator reading the public header implements the wrong cancel-path persistence behavior (dropping the user's partial results on stop instead of saving them). No runtime behavior is wrong — this is documentation-of-contract drift only, the M-120 committed-truth class applied to a header.

## Reproduction / verification

Both sites read first-hand this session at `3799dee2` (quoted in the references). The S-057 provenance: its landing is cited in the rrecov comment itself; the header comment's git blame predates it (the field was introduced by the W1c/R-52 autosave landing, before S-057's stop-window decode). No trail row covers the contradiction (grep "never auto-saves" across diff-to-fix/: zero hits).

## Fix plan (for the engineering team)

1. Re-truth the `cr_scan.h` autosave_dir comment: the field is honored on completion AND on the stop window (rc == OK or the cancelled-with-results path; the og's stop path runs its own organize into the shared completion handler — cite S-057 and the rrecov site).
2. Sweep the same comment family for sibling drift: the `raw_category_name` and auto-checkpoint blocks in the same header describe cancel-path behavior — verify each against the current rrecov/cr_ui behavior in the same pass (the auto-checkpoint text at :329+ describes stage-boundary invocation; confirm the stop-window checkpoint statement, if any, matches S-057's flow).
3. Files: `include/cr/orchestrator/cr_scan.h` (comment only — no code change).
4. Tests: none (a comment); the guard is procedural — consider extending `tools/doc_truth_gate.py` with a header-contract grep for the specific retired phrase ("never auto-saves") if the class recurs (the gate's check_deleted_citations pattern generalizes).
5. Acceptance criteria: the header text and the rrecov behavior agree; no other statement in the header's cancel-path contracts contradicts a landed finding.

## Tests to add

None — no behavioral surface. (If the gate extension in step 4 is adopted, the gate run itself is the test.)

## Related findings

- S-057 (FIXED — the stop-window auto-save landing whose contract this comment predates).
- M-120 (FIXED — the committed-conflict-marker/doc-truth repair family; the same discipline applied to headers).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-211 landing (the S81 MINOR-first lane)
**Verified by:** the reproduction at the open pin (both contradiction sites read first-hand: `cr_scan.h:321-322`'s "a cancelled run never auto-saves" vs `rrecov.cpp`'s `autosave_if_due(true)` on the cancel path — the S-057 landing) + the corpus spot-check this session (DRWUI :1549399-421: the stop window's own "Organizing Found Files" / "Scan stopped and found %1 files." messaging feeding FUN_140395e50 at :339079 — the S-057 decode re-verified) + the NEW gate check as the executed guard: `tools/doc_truth_gate.py`'s `check_header_contract_phrases()` — **RED at the pre-fix tree (it caught a SECOND drift site the finding missed: `include/cr/cr_ui.h:636`'s "a cancelled run never auto-saves" in the shim's own config comment, whose code at `cr_ui.cpp:2418` has auto-saved the stop window since S-057)** and GREEN at the post-fix tree (`doc_truth_gate: header-contract phrases clean (N-211 class)`), plus the linux-release build green after both comment re-truths.
**Resolution notes:** Both header comments re-truthed to the landed behavior: the field is honored on completion AND on the stop window (the og's stop path runs its own organize into the shared completion handler — DRWUI :1549399-421 → :339076-79 → FUN_140395e50, gates :612571-587 with no stopped-check; S-057's landing, cited at both code sites). The sibling-comment sweep the fix plan ordered: the `raw_category_name` and auto-checkpoint blocks carry no cancel-path claims (verified by read + the gate's include-wide sweep); the sweep's real find was cr_ui.h's twin drift (the gate caught it, not the eye — the class needed the gate, exactly the fix plan's step-4 prediction). The gate extension: `RETIRED_HEADER_CONTRACT_PHRASES` + `check_header_contract_phrases()` wired into `main()` — the retired-phrase class now fails CI the moment it recurs in any public header. No code change (the code was already og-faithful at both sites); no behavioral surface to test — the gate run IS the test, per the fix plan's own step 4.
