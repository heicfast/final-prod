# MINOR N-222 (RENUMBERED from the born-collided N-219 seat — the second N-219 file; the M-117 collision class) — committed conflict markers survive the M-151 landing: main is RED (the M-120/N-210/N-217 class, fourth recurrence) — the A-016 test and all four M-151 tests were committed inside live conflict hunks

> **Renumber note:** this finding was drafted as N-219 by a lane that did
> not see the racing S85 lane's same-window filing (f18ba226, 16:23) —
> the doc_truth_gate's retired-file ID uniqueness check caught the
> collision at this lane's push; the first N-219 keeps the seat, this
> one renumbers to N-222 per the M-153 precedent. The two filings cover
> the same event (the 27bda40e marker repair) from two convergent
> repair lanes; the S84 convergence addendum in the first N-219 records
> the corroboration.

**Severity:** MINOR (board-integrity: a red main blocks every other lane; no recovery-results defect — both sides' content was fully present, only the merge resolution was committed as markers)
**Module:** tests (test_ntfs_volume.cpp) + audit trail (CROSS-REF-MATRIX.md)
**Parameters affected:** Q68 (documentation truth — the matrix row), the never-push-a-red-commit rule (docs/CONTRIBUTING.md)
**Corpus references:** none (a core-process defect; the corpus is not implicated — the class citation is the trail's own M-120/N-210/N-217 lineage)
**Core references:** discovered at this session's open (core `842a587e`, the S83-close tree): `tests/unit/test_ntfs_volume.cpp:5780/5803/5860` + `:5865/5905/5987` (two live conflict regions, `<<<<<<< HEAD` / `=======` / `>>>>>>> 9647e8b0`); `diff-to-fix/CROSS-REF-MATRIX.md:38-44` (one more). The build fails `-Werror` at the first region (`error: version control conflict marker in file`, gcc 14.2); the N-210 conflict-marker lint class would also have caught it had it run pre-push.
**Status:** FIXED (filed + fixed this session at the open, per the discovered-divergence SOP — main was RED at the open; every lane's prerequisite)

## Summary

The M-151 landing commit `27bda40e` resolved its rebase against a racing
lane's `9647e8b0` (a garbage-collected lane-local SHA, absent from the
final history) by committing BOTH conflict regions as-is in
`tests/unit/test_ntfs_volume.cpp`, plus one in
`diff-to-fix/CROSS-REF-MATRIX.md`. Main was RED at this session's open:
gcc refuses the markers under `-Werror`, so `cr_tests` cannot build —
the fourth recurrence of the M-120/N-210/N-217 committed-marker class.
Both sides' content was fully present inside the hunks: HEAD carried the
A-016 `FolderFilterRootDotIsTransparent` test (complete at `e2ea4d50`,
the A-016 landing), the `9647e8b0` side carried M-151's four tests (the
run-growth battery) and its helper namespace.

## What was wrong

Two conflict regions, mapped first-hand:

1. `test_ntfs_volume.cpp` region 1 (:5780/:5803/:5860): HEAD = the A-016
   test start (`TEST(NtfsVolume, FolderFilterRootDotIsTransparent)` …
   `auto* rl = tree.add_root(u"l");`); 9647e8b0 = the M-151 comment block +
   the `patch_mft_runlist_declared` helper + the
   `TruncatedRunlistGrowsToValidRecords` test start. The four-line shared
   band (`WalkerFlags wf; … ASSERT_EQ(walk_mft(…), OK);`) sat between the
   regions as common context.
2. region 2 (:5865/:5905/:5987): HEAD = the A-016 test tail (the dot
   transparency + prune assertions); 9647e8b0 = the M-151 test tails +
   the three further growth tests. The A-016 test's closing brace
   (`e2ea4d50:5829`, the file's last line there) was dropped by the
   merge; the M-151 battery's closing brace survived as the file's last
   line.
3. `CROSS-REF-MATRIX.md` (:38-44): HEAD = the S69 audit lane's M-151
   ADDENDUM + N-182 option-2 rows; 9647e8b0 = the M-151 FIXED row + the
   N-182 pre-option-2 addendum row.

## The repair (this session, first push)

- `test_ntfs_volume.cpp`: both sides interleaved to their pre-conflict
  shapes — the complete A-016 test (start + shared walk band + tail +
  closing brace, byte-identical to `e2ea4d50:5764-5829`), then the
  complete M-151 block (comment + helper namespace + the four tests with
  the shared walk band restored inside `TruncatedRunlistGrowsToValidRecords`).
  5989 conflicted lines → 5987 clean; every `TEST(NtfsVolume, …)` from
  both sides present (96 tests).
- `CROSS-REF-MATRIX.md`: the M-151 row takes the 9647e8b0 side (the
  landing's own FIXED row — `grow_mft_runs_to_valid_records`, the S80
  lane's growth loop, verified present at `src/core/ntfs/cr_bootstrap.cpp:527`); the N-182 row takes the HEAD side (the option-2
  registered disposition, `868d1e85` — the later landing in main's
  history).

## Reproduction / verification

- RED at the open, first-hand: `cmake --build --preset linux-release`
  exit 2 with `error: version control conflict marker in file` at
  `test_ntfs_volume.cpp:5780` (+ the two `>>>>>>>` marker errors).
- GREEN post-repair: build clean; full suite **1648/1648 passed**
  (`ctest --output-on-failure -j`, 114 s) — including the repaired
  A-016 pin (`NtfsVolume.FolderFilterRootDotIsTransparent`), the four
  M-151 growth pins (`TruncatedRunlistGrowsToValidRecords`,
  `RunGrowthStopsAtFirstInvalidRecord`, `RunGrowthPartialWindowCountsValidPrefix`,
  `MirrorFallbackUnaffectedByGrowthPass`), and `win_syntax_check`.
- Repo-wide marker sweep post-repair: zero live `<<<<<<<`/`>>>>>>>`
  outside the M-120 evidence quote (the N-210 lint's sanctioned carrier).

## Resolution

**Status:** FIXED
**Fixed in commit:** `1d7d31a6` (the racing S81 lane's rebase landing — the test-file seat conceded per SA-9) + this lane's push (the trail-file half the racing repair left: the matrix M-154/M-151/N-182/I-048 rows, the ledger tail incl. the swallowed M-151/M-154 rows, the worklog union, the I-048 Resolution merge, PARITY-DELTAS' twice-landed I-048 registers deduplicated to the S82 trio D-160/D-161/D-162)
**Verified by:** build green (gcc 14.2 -Werror) on both convergent repairs; this lane: full suite 1648/1648 pre-rebase; the racing lane: 1647/1647 at its repaired tree; post-rebase re-verification on the merged tree this push; repo-wide line-start marker sweep clean outside the M-120 evidence quote — including the five files origin still carried markers in at the rebase.
**Resolution notes:** The M-151 lane committed its rebase resolution against a garbage-collected lane SHA with the markers live — the exact M-120/N-210/N-217 class, fourth recurrence, and the second red window in the racing window's own history (ed501caa was the first, repaired as N-217). Two convergent repairs executed in the same window: the racing S81 lane's (the test file reconstructed to semantic intent) and this lane's (both sides' content recovered from the hunks, the A-016 test byte-identical to e2ea4d50:5764-5829); per SA-9 the racing landing keeps the test-file seat, and this lane's repair extended to the five trail files the racing push still carried markers in (CROSS-REF-MATRIX, FIXED-LEDGER — whose M-151/M-154 rows were swallowed entirely —, worklog, the I-048 finding file, PARITY-DELTAS), resolving each to the parameter-truthful final state (per-parameter later-state selection; the twice-landed I-048 registers deduplicated to the S82 renumbered trio; the missing M-151/M-154 ledger rows restored). Prevention note (the recurrence driver): every recurrence shipped from a lane that did not run ci-local.sh's step-3 build before push — the -Werror marker refusal and the N-210 lint are both pre-push gates that were skipped, not missing. Recorded so the next audit counts the recurrence honestly.
