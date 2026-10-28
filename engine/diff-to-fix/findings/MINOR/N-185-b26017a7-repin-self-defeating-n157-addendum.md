# MINOR N-185 — the b26017a7 "repair" of the OoxmlRelsPathSplit red was itself self-defeating: executed proof the tree stayed red at that commit, its "green" claim false — the N-157 record's addendum

**Severity:** MINOR (record-truth addendum — the defect is RESOLVED at the current HEAD by the parallel window's later repairs)
**Module:** tests / build (the executed-verification pass, W4)
**Parameters affected:** #65 Test coverage (the suite's truthfulness); #68 Documentation accuracy (the commit message's claim)
**Corpus references:** n/a (core-internal defect; the executed evidence is the verification)
**Core references:** `tests/unit/test_carve.cpp` — at `b26017a7`: the case table uppercased to `.DOCX/.XLSX/.PPTX` while the comparison loop still lowercased the actual name's tail (`ch + 32`) at :8455-8463; **RESOLVED** at the current HEAD by `d10e4aa2` (the N-126 lane's case-insensitive comparator) + `1fb7603b` (the both-sides fold cleanup) — the test now lowercases BOTH sides and passes; commit `b26017a7`'s message claims "T1 baseline recorded (1092/1092 green…)" and the "Fix session 42" worklog entry claims "1134 ran / 1092 passed / 42 Win-only skipped / 0 failed on the repaired tree" — both refuted by this lane's executed proof at exactly `b26017a7`: **1136 ran / 1093 passed / 1 FAILED (`Carve.OoxmlRelsPathSplit`) / 42 skipped** — Linux gcc, Release, fresh out-of-tree build. Evidence: `diff-to-fix/evidence/executions/s42-build-suite-execution.md`
**Status:** READ-ONLY FINDING — no fix applied (the record-truth addendum to `findings/FIXED/N-157-ooxml-rels-extension-case-push-window.md`)

## Summary

The parallel window's N-157 record documents the OoxmlRelsPathSplit red at `67de3276` and credits `b26017a7` as one of the three convergent repairs ("`b26017a7` re-pinned the expectation to the same uppercase forms"). This lane's executed proof adds the missing link in that chain: **`b26017a7` itself never repaired anything** — it uppercased the expectation table while leaving the tail-lowercasing loop, a self-defeating edit that left the test failing unconditionally at exactly that commit (1136/1093/1/42, first-hand execution). The actual repair arrived with `d10e4aa2`/`1fb7603b` (the both-sides fold). The `b26017a7` commit message's "1092/1092 green" claim and the same-session fix-lane worklog's "0 failed on the repaired tree" are both false as of the tree they describe — the record-truth defect this finding preserves.

## What the reference does

n/a — this is a core-internal test-hygiene defect. (The S-049 naming contract itself — the carved name's extension being the format's own uppercase form — is corpus-verified and unchanged; the test's *intent* follows it.)

## What our core does

At `b26017a7` (`tests/unit/test_carve.cpp:8455-8463`):

```cpp
std::string tail = nm.substr(nm.size() - 5);
for (auto& ch : tail)
    if (ch >= 'A' && ch <= 'Z')
        ch = char(ch + 32);          /* tail forced lowercase */
EXPECT_EQ(tail, c.ext) << "the typed extension";   /* c.ext now ".DOCX" — can never match */
```

The commit `b26017a7` changed only the `cases[]` table (`.docx` → `.DOCX` etc.) — the fold loop made the re-pin self-defeating. At the CURRENT HEAD the parallel window's `d10e4aa2`+`1fb7603b` repairs restored green with the both-sides fold (both the table and the tail lowercased — the case-insensitive comparator the test comment documents); the engine's behavior was correct throughout (the failure output shows `c.ext` = `.DOCX`, i.e. the carve emits the uppercase form the contract wants).

## Gap

Stated falsifiably: `Carve.OoxmlRelsPathSplit` failed on every run at exactly `b26017a7` (executed proof), while that commit's message and the same-session worklog record claimed a green suite — the record's repair attribution is false for `b26017a7`; the true repair sequence is `67de3276` (red, per the remote's N-157) → `b26017a7` (STILL RED — this finding) → `d10e4aa2` (green, case-insensitive) → `1fb7603b` (cleanup). The +2 count drift (1134 recorded vs 1136 executed) also lands here.

## Impact

Record-truth only at the current HEAD (the test is green); the historical false-green claims remain in the pushed commit message and the worklog — the baseline-of-truth convention requires the erratum this finding carries. The N-157 record's "b26017a7 re-pinned the expectation to the same uppercase forms" line reads as a successful repair; this addendum corrects it.

## Reproduction / verification

**Executed (this session, first-hand):** fresh out-of-tree CMake build at `b26017a7` (`cmake -DCMAKE_BUILD_TYPE=Release -DCR_BUILD_APP=ON` + `cmake --build -j`), then `./tests/cr_tests`: `1136 tests ran, 1093 PASSED, 1 FAILED (Carve.OoxmlRelsPathSplit), 42 skipped`; the failure detail shows the three expectation pairs (`.docx` vs `.DOCX`, `.xlsx` vs `.XLSX`, `.pptx` vs `.PPTX`) with `tail` lowercased and `c.ext` uppercased. Full log: `diff-to-fix/evidence/executions/s42-build-suite-execution.md`.

## Fix plan (for the engineering team)

1. The code defect is already resolved by the parallel window (`d10e4aa2`/`1fb7603b`) — no further test change needed (optionally strengthen to the case-sensitive compare per the original N-157 discussion; the both-sides fold that stands is acceptable).
2. The RECORD erratum remains owed: a one-line note in the worklog's session-42 records (the 1233b4a0 message-truth precedent) correcting the b26017a7 green claims, plus the N-157 fixed-record's attribution line pointing at this addendum.
3. Acceptance: the worklog notes landed; nothing else owed.

## Related findings

S-049 (FIXED — the naming contract the test pins), N-115/N-153 (FIXED — the landing whose test this is), the S42 fix-lane records (their green claims need the erratum).

## Open questions

Where the +2 test count drift came from (1134 recorded vs 1136 executed) — likely two tests landing between the fix-lane's run and the hygiene commit; harmless, but the fix lane's re-run should pin the new canonical count.
