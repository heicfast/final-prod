# SB-E-01..05 fix evidence — the premature progress family (engine)

## Defect (before — reproduced on the pre-fix build, this session)

Five engines emitted a terminal-shaped progress tick at the WRONG SCOPE;
the telemetry consumer's `done >= total -> 1000 permille` + monotonic
clamp then held the stage at 100% for the rest of the work:

- SB-E-01 FAT quick (cr_fat_quick.cpp:294-295): `(done, done)` at the close
  of EVERY directory — plus the mid-loop form collapsing to done==total on
  the last buffer entry.
- SB-E-02 exFAT quick (cr_exfat_quick.cpp:1002-1003): same per-directory
  `(done, done)`.
- SB-E-03 ext4 (cr_ext4_engine.cpp:1254-1255): `(groups, groups)` after the
  live walk, BEFORE the deleted-inode sweep — the sweep's own honest
  `(ino, sweep)` ticks were clamped up to the stuck 1000.
- SB-E-04 btrfs (cr_btrfs_engine.cpp:1184): `(4, 4)` per SUBVOLUME.
- SB-E-05 HFS+ (cr_hfsplus_engine.cpp:1397-1398): hard-coded 1024-node
  denominator — ≤29% cap for short chains, premature 100% at node 1025.

The captured PRE-FIX reproduction (the new regression test's engine,
Linux build, ASAN preset — seq of overall permille samples):

    seq: 229 459 688 750 750 750 750 750 750 750 750 750 750

Two sibling subdirectories (A=200 entries, B=300) in a FAT16 root: when
sibling A's chain closed, the overall hit the 750 boundary (the weighted
quick+deep "75% of the whole scan") and STAYED there through sibling B's
entire walk — 10 boundary samples mid-walk. The user sees "75% done"
seconds into a scan and a frozen gauge for its remainder (the agent's
worked example: gauge "Quick 100%"/overall 75% stuck, ETA meaningless).

## Fix (after)

- FAT quick: the mid-loop tick's denominator carries the +1 margin
  (`done + (count - i)` — done < total ALWAYS mid-walk); the per-directory
  close tick REMOVED (the pass end's `(1,1)` is the true terminal).
- exFAT quick: the per-directory close tick REMOVED (mid-loop already
  carried the margin form).
- ext4: the pre-sweep `(groups, groups)` tick REMOVED — the sweep's
  `(ino, sweep)` ticks are the live signal, the stage-boundary stamp owns
  the terminal.
- btrfs: the per-subvolume `(4,4)` tick REMOVED — the phase markers
  (1..4) and the final `(4,4)` are the honest signals.
- HFS+: the phantom 1024 denominator replaced by the self-scaling
  `chain_len + 16` (the verified-clean ReFS pattern: strictly < 1000
  mid-walk, terminal at the pass end).

POST-FIX captured sequence (same fixture):

    seq: 228 456 685 685 685 685 685 750 750

No sample reaches the boundary until the true pass end (2 true-end
stamps). The mid-walk clamp plateau (685) is the pre-existing
buffer-local-estimate character (not new behavior); the DEFECT — premature
completion — is gone.

## Regression test

`engine/tests/unit/test_telemetry.cpp`:
- New fixture `build_fat16_sibling_subdirs` — two SIBLING subdirectories
  (the inline DFS means the first per-directory close tick fires with the
  second sibling's whole walk remaining — the defect's shape).
- New pin `Telemetry.FatQuickWalkHonestUntilTrueEnd`: monotonicity
  preserved, live values exist below the boundary, the TRUE end still
  lands exactly at the boundary, and the boundary value is sampled ONLY
  at the true end (≤4 terminal stamps). RED pre-fix (10 boundary samples),
  GREEN post-fix (2).

## Full-suite proof (this session, Linux, ASAN build)

`cr_tests --gtest_brief=1`: **1749 tests ran, 1707 passed, 42 skipped
(fixture data absent — e.g. TIFF thumbnails), 0 failed** — the five
engine changes regress nothing (FAT/exFAT/ext4/btrfs/HFS+ volume suites,
telemetry, pause/cancel, scan-truth included).

Windows CI: the pushed commit runs engine-ci (win-x64-release, /W4 /WX) on
GitHub's Windows runner — status recorded in the session worklog.

## Addendum: the concurrent byte-plan progress model landed mid-session

A parallel session landed `e9893088` (the F-B/F-C byte-plan model: 40/35/25
StageWeights retired, one cumulative byte-sweep axis, quick+deep metadata
passes own no percent) while this fix was in flight. Both changes rebase
cleanly onto one tree; interaction reconciled honestly:

- The premature-tick mechanism in the CONSUMER is unchanged
  (`tel_publish_`: `done >= total -> pm = 1000`, monotonic clamp) — the
  five engine-side tick removals remain valid and necessary.
- The overall axis is now byte-plan driven; for a quick-only scan
  (no byte sweep → `plan_bytes == 0`) `overall_from_plan` FALLS BACK to
  the stage permille — the walk ticks ARE the overall there, which is
  exactly the surface the premature tick polluted. The regression test
  drives that configuration (`cfg.run_deep = false; run_carve = false`)
  and pins: the 1000 completion value sampled ONLY at the true end.

Red/green on the MERGED tree (this session, Linux ASAN build):
- PRE-FIX (parent commit 5fbf39fb's engine files checked out): the pin
  FAILS — `completion_samples = 9` (the overall hit 1000 when sibling A's
  chain closed and held it through sibling B's whole walk).
- POST-FIX: PASSES (completion samples ≤ 4, all true-end stamps).
- Full merged-tree engine suite: **1750 tests ran, 1708 passed,
  42 skipped (absent fixture data), 0 failed** (171 s, ASAN preset).
