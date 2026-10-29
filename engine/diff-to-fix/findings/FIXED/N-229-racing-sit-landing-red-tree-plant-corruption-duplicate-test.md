# MINOR N-229 — the racing S89 window's SIT landing (2214af90) committed a red tree: the plant_sit marker writes lost their `[m]` subscripts (`img] = 0x02;` — eight syntactically invalid lines) AND the WPD merge left a duplicate WpdZeroSizeFieldDeclines test — main could not build

**Severity:** MINOR (the N-210 push-without-gate class's recurrence #8 — the lane's own commit message claims "the full suite 1711/1711" over a tree that does not compile; every downstream lane pulling that head inherits the break)
**Module:** CI / the carve test battery (tests/unit/test_carve.cpp)
**Parameters affected:** Q68 (audit-trail/CI truth — the claimed suite count vs the non-compiling tree)
**Corpus references:** none (a core-repo integrity defect)
**Core references:** found at this lane's merge over origin/main `98bd30d6` (2026-10-30; first-hand): `git show origin/main:tests/unit/test_carve.cpp | sed -n '3683,3712p'` → the plant_sit body carries `img] = 0x02;` / `img + 1] = 0xD7;` / … / `img + 7] = 0x88;` — eight lines with the `[m` subscript eaten (a merge-resolution corruption: the `const size_t m = at + bytes - 12;` declaration survives above them, proving the intended form); the same tree carries TWO `TEST(Carve, WpdZeroSizeFieldDeclines)` definitions (their :3434 and :3612 — the S89 merge over this lane's WPD push resolved append-both instead of dedup) → `redefinition of 'Carve_WpdZeroSizeFieldDeclines_Test'`. The build: `g++ ... error: redefinition` + the parse errors on the img] lines — cr_tests does not link at `98bd30d6`.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The racing S89 lane's SIT member landing (`2214af90`) pushed a tree that cannot compile: the new plant_sit's eight marker-write lines lost their `[m]` subscripts in a merge resolution (`img] = 0x02;` — the `m` variable is declared but never used, the exact shape of a mechanical conflict-resolution loss), and the same push's WPD test merge kept both sides' `WpdZeroSizeFieldDeclines` batteries — a gtest redefinition. The commit message records "the full suite 1711/1711" — a count the pushed tree cannot have produced. This is the eighth recurrence of the N-210 class (a lane pushing without running the local build/test gate against the pulled head).

## Fix

1. The plant_sit marker writes repaired to `img[m] = 0x02;` … `img[m + 7] = 0x88;` (the racing lane's own declared intent — the comment "the 0x19-stride marker at bytes-12" and the `const size_t m` line both survive; the repair note rides the plant's comment).
2. The duplicate `WpdZeroSizeFieldDeclines` deduped (this lane's 2026-10-30 form dropped; the racing lane's richer pending-child form kept — the same observable behavior, the better cite).
3. This lane's withdrawn SIT battery (the concession record in M-107's S87 addendum 3) swept from the tree — the tests pinned the superseded processor-level translation and conflicted with the seat's marker+stride walk.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S87 fix lane's repair over `98bd30d6`, 2026-10-30)
**Verified by:** the full rebuild clean (0 errors); the Sit*/Wpd* batteries 15/15 green; the full ctest suite 1715/1716 with the single known parallel-load perf flake (FatChain.CycleGuardPerf) passing serially at 1.70 s.
**Resolution notes:** RED first-hand at `98bd30d6` (the gtest redefinition + the eight parse errors — cr_tests cannot build), GREEN after the three-part repair. The recurrence root cause is unchanged since N-210: the lane pushed without the local gate. The claimed-1711-vs-broken-tree delta is recorded here so the next audit weighs commit-message suite counts against the tree's actual buildability — the count in a message is a claim, not evidence.
