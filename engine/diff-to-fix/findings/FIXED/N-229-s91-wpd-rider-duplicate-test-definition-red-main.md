# MINOR N-229 — the S91 rider's WpdZeroSizeFieldDeclines duplicate definition: origin/main red at the racing window's tip (cr_tests unbuildable)

**Severity:** MINOR (red-main repair — the M-120/N-210/N-219/N-222/N-227 class's NINTH occurrence)
**Module / Surface:** tests (test_carve.cpp — the racing window's WPD convergence dedupe)
**Parameters affected:** Q68 (the build's green-tree invariant), Q66 (the suite's runability)
**Corpus references:** none needed — a build-integrity defect, not a parity divergence
**Core references (origin/main at `ddbab2a1`):** `tests/unit/test_carve.cpp:3438` (the S87 version-table battery's `TEST(Carve, WpdZeroSizeFieldDeclines)`) and `:3616` (the S91 rider battery's identical-name test, under the "The S91 lane's convergent riders" comment) — gtest's TEST macro expands each name to a class; the second definition is a redefinition compile error (gcc: `error: redefinition of 'class {anonymous}::Carve_WpdZeroSizeFieldDeclines_Test'`, exit 2), so cr_tests could not build at the tip and NO test of the whole suite was runnable there.
**Status:** FIXED (this push — the S93 lane's discovery at its merge-build)

## Summary

The racing window's own convergence dedupe (the S90 lane's `fce46d98`, which dropped its two WPD rider tests as converged duplicates) caught the pair-table/over-cap duplicates but missed the S91 rider block's `WpdZeroSizeFieldDeclines` — the S87 battery already carried the identical test at `:3438`. The remote tip `ddbab2a1` therefore committed a tree whose test binary cannot compile: the ninth recurrence of the gate-skipping class (a lane pushing without rebuilding the pulled head).

## What the reference does

N/A — build integrity, not parity.

## What our implementation does (the fix)

The S91 duplicate block (the comment header + the test, 24 lines) removed; the S91 block's NON-duplicated rider (`WpdZeroSizeFieldDeclines`'s sibling `WpdZeroProductWordWithGoodPairCarves` — the +4-product-word retirement pin) kept. The two duplicate bodies pinned the same behavior (plant_wpd declared 0 → declines through the generic child's walk); the S87 seat's copy holds. A repo-wide duplicate-name sweep ran over every test file: zero further duplicates (the class-sweep lesson of N-222's "sweep the whole class, not the first instance").

## Gap / Impact

Falsifiable pre-fix (executed first-hand): `cmake --build --preset linux-gcc` at the merged tree → the redefinition error, exit 2, 0 tests runnable. Post-fix: 100% build, the DWG battery + the full carve suite green.

## Reproduction / verification

Pre-fix: the gcc redefinition error quoted above (origin/main's own file — `git show origin/main:tests/unit/test_carve.cpp | grep -c 'TEST(Carve, WpdZeroSizeFieldDeclines)'` = 2). Post-fix: the count 1; the build green; the carve suite green (the numbers in this push's worklog entry).

## Resolution

**Status:** FIXED
**Fixed in commit:** this push (the S93 merge commit + rider)
**Verified by:** the pre-fix gcc exit-2 redefinition error (first-hand at the merge-build); the post-fix build 100% + the full carve suite green; the repo-wide duplicate-name sweep clean
**Resolution notes:** the S91 rider duplicate removed, its unique sibling kept; the S87 seat's test holds. The ninth gate-skipping recurrence — the standing lesson (every racing-lane push must rebuild the pulled head before pushing) rides again in the worklog.
