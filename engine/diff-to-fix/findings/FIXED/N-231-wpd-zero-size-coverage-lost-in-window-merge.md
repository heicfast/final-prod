# MINOR N-231 — the WPD zero-size-decline COVERAGE lost in the racing window's final merge state: N-230's own recorded resolution ("the S87 seat's test kept") does not match the tip it shipped (count 0)

**Severity:** MINOR (a coverage/bookkeeping-truth repair — the record-vs-tree disagreement class)
**Module / Surface:** tests (test_carve.cpp — the WPD battery's zero-size member); the N-229/N-230 arbitration records
**Parameters affected:** Q66 (the zero-size-decline behavior's coverage — the extent!=0 engine gate, corpus-decoded at 0x18000d7e1; the pending-child framing 0x18000b760)
**Corpus references:** the same two decodes N-229/N-230 record (the engine acceptance gate + the generic pending child's decline) — no new corpus claim
**Core references (origin/main at `84a3670e`, first-hand):** `git show origin/main:tests/unit/test_carve.cpp | grep -c 'TEST(Carve, WpdZeroSizeFieldDeclines)'` → **0**. Neither the S87 seat's test nor the S91 rider's renamed form exists at the tip — despite the three arbitration records stating the coverage preserved: `8301c72b` ("the S87 seat's test keeps its name, the S91 rider's renamed to its own decode angle ... both batteries' coverage preserved"), `47c849bb` ("the second renamed WpdZeroSizePendingChildWalkDeclines, both green"), a98392d1 ("the racing pending-child form keeps the dedup seat"), and N-230's own Resolution ("the racing fix, their form: the S91 rider's colliding test DELETED, the S87 seat's kept" — count 1 claimed). The landed validator still implements the behavior (`validate_wpd`'s `if (end <= hit) return r;` — the decline on a zero declared size), so the behavior is real, corpus-decoded, and UNPINNED at the tip.
**Status:** FIXED (filed+fixed this push — the S94 lane's convergence-verification catch)

## Summary

The racing window's duplicate-test arbitration (N-229 superset / N-230 renumbered seat) was fought over WHICH zero-size test survives; the window's final merge tree answered "neither". Three commit messages and N-230's Resolution all describe a one-or-two-test final state; the tip they actually shipped carries zero tests for the behavior — the extent!=0 gate's decline (a corpus-decoded acceptance rule) has no pin anywhere, so a future regression of `end <= hit` would pass the whole suite green.

## What the repair does

The S87 seat's `WpdZeroSizeFieldDeclines` is restored verbatim against the landed validator (the seat-holder's own recorded intent — "the S87 seat's kept" — re-implemented; the S94 lane's restoration uses the S87 seat's engine-gate framing and the current `plant_wpd(img, at, 0)` helper). The restored test sits with the rest of the WPD battery.

## Reproduction / verification

- RED (by inspection + grep, first-hand): the count-0 grep at the tip; the behavior implemented at `validate_wpd`'s `end <= hit` decline with no test exercising it anywhere in the suite.
- GREEN: the restored test passes on the merged tree; the DWG+WPD battery green; the full-suite numbers in this push's worklog entry.

## Resolution

**Status:** FIXED
**Fixed in commit:** this push (the S94 lane's merge + riders commit)
**Verified by:** the restored `Carve.WpdZeroSizeFieldDeclines` passing on the merged tree (first-hand); the count-1 grep post-restoration; the full-suite green run recorded in the worklog
**Resolution notes:** The record-vs-tree disagreement is the finding: N-230's Resolution claims count 1, the tip shipped count 0. The restoration makes the tree match the recorded intent. The deeper lesson rides the worklog: three lanes arbitrated the SAME duplicate and the final merge still lost the whole behavior pin — arbitration records are not tree state; the pin must be re-verified against the shipped tree (the same lesson as the lane's own N-229 discovery: counts in messages are claims, not evidence).
