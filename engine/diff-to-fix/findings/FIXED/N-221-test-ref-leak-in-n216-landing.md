# MINOR N-221 — the N-216 landing's test leaks its FileProperties references: LeakSanitizer fails the suite at exit (the ASan lane's exit code is red behind a green gtest summary)

**Severity:** MINOR (test hygiene: the product code is correct — the ref-counting contract is the test's to honor; the red exit code masks every later-failing check a CI consumer would run)
**Module:** orchestration (the filterfolder size-cap surface) / test
**Parameters affected:** none (a test-side defect on the N-216 landing's own battery)
**Corpus references:** none needed (not a parity divergence — the N-216 parity content is correct and its pins pass)
**Core references:** `tests/unit/test_orchestrator.cpp` `SizeCapHidesOversizeNode` (the `add_file` lambda at :3350-3358): three `new tree::FileProperties()` whose references are never released. The contract: `ResultTree::add_file` "Takes a REFERENCE (caller releases its own ref when done; each add_file call adds one node-held ref)" (`include/cr/tree/cr_node.h:473-476`). Every sibling site releases (`test_folderfilter.cpp:281`, `test_orchestrator.cpp:2220`, `:2741`).
**Status:** NEW-FINDING (discovered at this lane's session-open ASan baseline run) — fixed this session

## ID note (the seat renumber)

This lane originally drafted the filing as N-219 locally; the push window
collided with the racing S85 lane's own N-219 (the fourth marker-recurrence
repair) and N-220 (the fifth-recurrence class record) — renumbered to N-221
(the next free seat) at the landing, per the born-collided-seat convention
(the M-153/D-150 precedent: the gate's uniqueness check is the arbiter, the
renumber note preserves the history).

## Summary

The N-216 landing's test constructs three `FileProperties` through `add_file` and returns `tree.add_file(root, pr)` directly — the caller-side reference is never released. LeakSanitizer (enabled by default with ASan) reports at process exit: "Direct leak of 1080 byte(s) in 3 object(s) … operator new … test_orchestrator.cpp:3351" plus the indirect u16-string payloads (1170 bytes in 6 allocations) — the ASan suite exits 1 despite every test passing. The racing S85 lane's same-window repairs (c7037bf3/f18ba226/1d7d31a6 — the marker class) touched `test_ntfs_volume.cpp` and the matrix but not this file; the leak survived every one of them (re-verified RED at `eeddec57` first-hand before the fix).

## What the reference does

Not applicable (test-side defect).

## What our core does (at the defect)

The ref contract is documented and honored everywhere else; this one landing missed it. The product `filterfolder::apply` path is not implicated (the leak is the test's own refs).

## Impact

Every ASan/LSAN CI lane that runs this suite exits red on a green tree — masking real failures behind a hygiene leak, and burning CI trust (the "exit code is the deliverable" rule).

## Reproduction / verification

`./tests/cr_tests --gtest_filter='FilterFolder.*'` on the ASan build at `eeddec57`: `[  PASSED  ] 5 tests.` followed by `ERROR: LeakSanitizer: detected memory leaks` → exit 1. Post-fix: exit 0, 5/5 green, LSAN clean.

## Fix plan

Release the caller-side reference inside the lambda, exactly like the sibling sites: `tree::FileNode* n = tree.add_file(root, pr); pr->release(); return n;`

## Tests to add

None — the proof is the ASan lane's exit 0 with the test's three pins still green.

## Related findings

- **N-216** — the landing whose test battery carried the leak (its parity content is unaffected).
- **N-219 / N-220** — the racing S85 lane's same-window marker-class filings; this lane's independent marker repair converged with theirs and was conceded per SA-9 (their landing stands; the convergence addendum is in their N-219 file).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S84 fix lane, 2026-10-29)
**Verified by:** RED first-hand at `eeddec57` (`FilterFolder.*` on the ASan build: 5/5 gtest-green, LSAN report, exit 1) → GREEN post-fix (exit 0, LSAN clean, 5/5). The one-line release honors the documented ref contract (cr_node.h:473-476) the way every sibling site does.
**Resolution notes:** CONFIRMED (reproduced at the current remote tip — the leak survived the racing window's marker repairs because it lives in a file none of them touched). No product code touched — the leak was purely the test's caller-side reference. The lesson recorded for future landings: the ASan lane's EXIT CODE (not the gtest summary line) is the truth — a "suite green" claim with an LSAN report riding behind it is a red suite.
