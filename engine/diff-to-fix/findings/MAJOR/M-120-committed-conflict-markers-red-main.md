# MAJOR M-120 — main is RED: committed git conflict markers in tests/unit/test_carve.cpp (the 1fb7603b repair push) — the build gate broken at HEAD while the introducing commit claims "suite 1096/1096 green"

**Severity:** MAJOR
**Module:** tests / build gate (process-infrastructure; the trail's verification convention)
**Parameters affected:** Q65 (test coverage — the suite cannot run), Q68 (documentation/message accuracy — the introducing commit's green claim is false at the pushed tree), Q67-adjacent (the sanitizer/CI lanes cannot execute)
**Corpus references:** none (this is a core-repo process defect, not a parity divergence — the corpus-side analog class is the trail's own "merge-window red" precedent, session-39's test_fixtures.hpp repair)
**Core references:** audited `869da779`-lineage at the current origin/main (through `0bd04cf9`). `tests/unit/test_carve.cpp:8588-8600` (the three committed markers, verbatim below); the introducing commit `1fb7603b` ("repair(tests)+worklog(S40-ML/converge): the N-115 OoxmlRelsPathSplit re-pin repaired … now the direct uppercase compare per the S-049 contract … suite 1096/1096 green"); the red window: every commit on origin/main from `1fb7603b` through the current HEAD.
**Status:** READ-ONLY FINDING — no fix applied (the read-only mandate; the one-line resolution belongs to the next fix lane)

## Summary

The parallel S40-ML lane's test-repair commit `1fb7603b` pushed **git conflict markers committed into `tests/unit/test_carve.cpp`** (lines 8588/8598/8600: `<<<<<<< HEAD` / `=======` / `>>>>>>> 59bd5208`). The main test binary `cr_tests` cannot compile at HEAD, so the full suite cannot run and every subsequent "suite green" claim is unverifiable until repaired — while the introducing commit's own message claims "suite 1096/1096 green" (the message-truth class, N-129, at whole-commit scale). The product library itself is unaffected: `cr_core`, `cr_ui`, and `cr_ui_abi_check` build 100% clean, and `cr_ui_tests` executes **63/63 green** at the same tree (verified by this lane's from-scratch build — the executed evidence below).

## What the reference does

N/A (core-repo process defect). The trail's own precedent for the class: session-39's merge-window red (the lost `cr/table/cr_table_wire.h` include in `test_fixtures.hpp` — repaired in the N-132 landing's commit with the "verified red at origin/main HEAD with my changes stashed" proof), and the never-push-a-red-commit rule itself (`ci-local.sh` header: "the never-push-a-red-commit rule, docs/CONTRIBUTING.md").

## What our core does

The committed file at origin/main HEAD (verified via `git show origin/main:tests/unit/test_carve.cpp`):

```cpp
/* :8588-8600 — the markers are IN the committed file (the working tree is clean) */
        const std::string nm = n0->name_utf8();
        ASSERT_GE(nm.size(), size_t(5));
<<<<<<< HEAD
        std::string tail = nm.substr(nm.size() - 5);
        for (auto& ch : tail)
            if (ch >= 'A' && ch <= 'Z')
                ch = char(ch + 32);
        std::string want = c.ext;
        for (auto& ch : want)
            if (ch >= 'A' && ch <= 'Z')
                ch = char(ch + 32);
        EXPECT_EQ(tail, want) << "the typed extension";
=======
        EXPECT_EQ(nm.substr(nm.size() - 5), c.ext) << "the typed extension";
>>>>>>> 59bd5208 (repair(tests)+worklog(S40-ML/converge): the N-115 OoxmlRelsPathSplit re-pin repaired - the S42 interim re-pin lowercased the name tail while its own case table went uppercase (the two halves disagreed; main was red) - now the direct uppercase compare per the S-049 contract; ...)
```

The compile failure (this lane's from-scratch build, gcc 14, `-Werror`):
`error: version control conflict marker in file` (:8588), `error: 'tail' was not declared in this scope` (:8590/:8597), plus the marker line's own lexical errors (:8600 — "invalid digit 9 in octal constant", "multi-character literal with 37 characters").

## Gap

`tests/unit/test_carve.cpp` at origin/main HEAD contains unresolved conflict markers; the `cr_tests` target cannot build; the full suite cannot execute; the introducing commit's green claim is false at the tree it pushed. Falsifiable: `git show origin/main:tests/unit/test_carve.cpp | grep -c '^<<<<<<<'` → 1 (three markers total); `make cr_tests` → the errors above.

## Impact

- **The verification convention is broken at HEAD:** every lane's "suite N/N green" claim from `1fb7603b` onward is unverifiable (the 43-minors lane's 1203/1203 was executed at the pre-red `632518b7` — their record is honest, but their NEXT push must merge over the red).
- **CI is red** (or would be, if the CI ran — the local-mirror gate `ci-local.sh` step 2 builds the suite and fails).
- **The N-115 re-pin is stranded mid-conflict:** the two sides disagree on the compare semantics (HEAD's side: lowercase both halves — case-insensitive; the 59bd5208 side: the direct compare per the S-049 uppercase contract). The introducing commit's message states the DIRECT COMPARE is the intended contract — so the resolution is known, just unlanded.
- **No product-code impact:** `cr_core`/`cr_ui` build clean; the recovery engine binaries are unaffected (verified — the executed evidence).

## Reproduction / verification

**EXECUTED this session** (the from-scratch audit-side build, the W4 pass):
1. `cmake --preset linux-release` (cmake 4.4.4, gcc 14) → configures clean (GTest fetch-content resolved).
2. `make -j$(nproc)` → `cr_core` **100% Built target cr_core**; `cr_tests` **fails** with the conflict-marker errors quoted above.
3. `make cr_ui_tests cr_ui_abi_check` → both build clean; `./tests/cr_ui_tests` → **63/63 PASSED** (21.9 s; including the S-054 fence pair `DestArming.ProductionWiringCallsSpansApi` + `DestArming.HeaderContractStillClaimsAllSpans` — the N-013 regression fix's fences verified green audit-side).
4. `git status` clean; `git show origin/main:tests/unit/test_carve.cpp | grep -c '^<<<<<<<\|^>>>>>>>\|^=======$'` → **3** (the markers are committed, not a local artifact).
Full log: `diff-to-fix/evidence/executions/s42-build-at-head.md`.

## Fix plan (for the engineering team)

1. Resolve the conflict at `tests/unit/test_carve.cpp:8588-8600` keeping the **59bd5208 side** (the direct compare `EXPECT_EQ(nm.substr(nm.size() - 5), c.ext)`) — the introducing commit's own message declares it the S-049 contract ("now the direct uppercase compare"); the HEAD side's lowercase-both-halves form is the variant the message says it was replacing.
2. Verify the N-115 case table's expectation still matches (the table's extensions are uppercase per the S-049 contract — if any row is lowercase, fix the row, not the compare).
3. Run the full suite (`ctest` from `build/linux-release`) — expect ≥1203 green (the 43-minors lane's count at `632518b7` plus the S-048/S-054 landings' tests).
4. The gate repair (prevention): the trail's rule is already "never push red" — the miss here is that the lane's local suite ran pre-rebase and the rebase conflict was committed without re-running. The ci-local.sh gate is sufficient IF run post-rebase; register the "re-run the gate after every rebase, not just before the push" discipline in the next worklog handoff (no code change needed).
5. Acceptance: `git show origin/main:tests/unit/test_carve.cpp | grep -c '^<<<<<<<'` → 0; `make cr_tests` builds; full suite green; the commit message truthfully states the tree it tested.

## Tests to add

None (the finding is the broken test file itself; its resolution restores the existing tests).

## Related findings

**N-115** (the re-pin whose conflict is stranded — its fix commit `67de3276` is sound; only the S40-ML lane's re-pin of it is conflicted) · **S-049** (the naming contract the conflicted compare implements) · **N-129** (the message-truth class — this finding is its whole-commit instance) · **S-054** (whose fence pair this lane verified green in the same build — the fix itself is unaffected by the red)

## Open questions

None. (Which side of the compare to keep is answered by the introducing commit's own message; if the engineering team disagrees, the case-table audit in fix-plan step 2 decides it either way.)
