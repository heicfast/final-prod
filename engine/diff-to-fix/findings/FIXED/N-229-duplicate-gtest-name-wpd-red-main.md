# MINOR N-229 — the duplicated gtest name left origin/main build-red: two racing lanes' WPD batteries both defined `Carve.WpdZeroSizeFieldDeclines` (cr_tests unbuildable at the 98bd30d6 head)

**Severity:** MINOR (the class convention — M-120 `db228b75`, N-210 `f3da99f1`, N-217 `ed501caa`, N-219 `27bda40e`, N-222 `f3da99f1`, N-227 `d15d32a4`, now `98bd30d6`'s parent window; the class's ninth occurrence, and the third to reach main build-red)
**Module:** tests / the never-push-a-red-commit gate (the N-210 class)
**Parameters affected:** Q68 (audit-trail/CI truth — the pushed tree did not compile)
**Corpus references:** none (a core-repo build-integrity defect)
**Core references:** found at this lane's rebase over origin/main `98bd30d6` (2026-10-30, first-hand): `git show origin/main:tests/unit/test_carve.cpp | grep -n 'TEST(Carve, Wpd'` → `WpdZeroSizeFieldDeclines` at BOTH `:3434` (the racing S87 landing's battery — the engine-gate framing, 0x18000d7e1) AND `:3612` (the racing S91 lane's convergent-rider battery — the pending-child framing, 0x18000b760). The gtest `TEST()` macro derives one class per name (`Carve_WpdZeroSizeFieldDeclines_Test`); two definitions = the redefinition error: `tests/unit/test_carve.cpp:3620:6: error: redefinition of 'class {anonymous}::Carve_WpdZeroSizeFieldDeclines_Test'` — cr_tests unbuildable, zero tests runnable. The 98bd30d6 style sweep (the window's own last commit, "16 violations ... swept") reformatted BOTH copies without compiling between them.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The racing S91 lane appended its convergent WPD rider battery directly on top of the racing S87 seat's battery without checking the name set: its zero-size test reused the S87 battery's exact name with a DIFFERENT decode framing (the pending-child walk decline 0x18000b760 vs the engine acceptance gate 0x18000d7e1 — two decodes of the same observable behavior). The gtest macro's class-per-name derivation makes the file unbuildable, and the window's final style sweep pushed the broken file reformatting both copies. The S87 seat's claim "the 191-test carve suite green" was true of ITS tree; the S91 rider window's claim "full suite 1711/1711" was not true of the tree it actually left behind.

## The repair (executed this session)

The S87 seat's test keeps its name (the seat holder). The S91 rider's test is renamed to its OWN decode angle — `WpdZeroSizePendingChildDeclines` — with the born-collided note inline (the convention: test names describe the behavior proven; the two framings both hold and both batteries' coverage is preserved). RED first-hand at the rebase head (build exit 2, the redefinition error, 0 tests runnable) → GREEN: the full suite **1717/1717** (the WPD battery 9/9 including both zero-decline framings).

## Resolution

**Status:** FIXED
**Fixed in commit:** (the racing window's own merge — 5ed0f07f's tree carries the count-1 form, first-hand; this lane's parallel rename repair conceded per SA-9 and recorded here)
**Verified by:** `git show origin/main:tests/unit/test_carve.cpp | grep -c 'TEST(Carve, WpdZeroSizeFieldDeclines)'` → 1 (the racing fix, their form: the S91 rider's colliding test DELETED, the S87 seat's kept); this lane's convergent repair renamed it instead (WpdZeroSizePendingChildDeclines — both framings preserved) and proved GREEN first-hand at its own tree (full suite 1717/1717, the WPD battery 9/9) before the rebase found their landing; the merged tree builds clean and the suite is green (the final count recorded in this lane's closing worklog entry).
**Resolution notes:** The ninth occurrence of the push-without-gate class, the third to reach main build-red. The recurrence root cause is unchanged since N-210: a lane pushing without running the local gate (`ci-local.sh` or at minimum a build) against the PULLED head immediately before the push. The style-sweep commit is the aggravator here: it ran a FORMATTER over a file it never compiled — formatting is not validation. Both racing batteries' coverage is preserved verbatim; the only delta is the colliding name.
