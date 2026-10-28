# MINOR N-186 — the M-119 window committed a red main: the app parse broke at `rrecov.cpp:3253` (IStorageDevice-typed holder erased PartitionSlice's clamp surface) and `cr_ui.cpp:1387/:1481` used the nonexistent `Level::Warning` — win_syntax_check failed at the pushed tree

**Severity:** MINOR (build-integrity / committed-red class — the N-133/M-120 family; no behavior defect on Linux where the app target was not part of the lane's suite run)
**Module:** cli / ui (the app + UI translation units the win_syntax_check gate parses)
**Parameters affected:** #65 test coverage (the suite's truthfulness — the gate that failed is itself the repo's committed-red detector), #68 documentation accuracy (the M-119 trail commits claim green suites)
**Corpus references:** n/a (core-internal build defect)
**Core references:** discovered at `64fc2d8d` (the merged tree after the M-119/S-055/S-056/M-128/M-129/M-133/N-175 window): `src/app/rrecov.cpp:3225-3260` — `std::unique_ptr<IStorageDevice> part_slice` holds the `PartitionSlice::create` result (cr_slice.h:48 returns `unique_ptr<PartitionSlice>`), then `:3253` calls `part_slice->was_clamped()`, `:3257` `->length()`, `:3259` `->clamp_delta()` — IStorageDevice has no such members; `src/ui/cr_ui.cpp:1387/:1481` — `cr::log::Level::Warning` (the enum carries `Warn`, cr_log.h:50). Executed proof: `ctest -R win_syntax_check` at the merged tree = **Failed** with exactly those five errors (rrecov ×3, cr_ui ×2); the linux-release app/library targets that DO compile rrecov/cr_ui were not part of the window's `--target cr_tests` runs.
**Status:** FIXED (this session — the repair rides the N-158 convergence push)

## Summary

The M-119 landing (033b9cdb) wired the clamp-delta surfacing into the CLI through an interface-typed holder, and the same window's cr_ui volume-open logging used a nonexistent log-level enumerator. The win_syntax_check gate — the suite member whose entire purpose is catching exactly this — failed at the pushed tree while the window's trail commits recorded green suites (their runs targeted cr_tests only, which excludes the app TU parse).

## Reproduction / verification

Executed this session: `ctest -j2` at `64fc2d8d` = 1339/1340 with win_syntax_check Failed; the five parse errors quoted above; after the two-line repair (below) the full suite = **1340/1340 green** including win_syntax_check.

## Fix plan (executed)

1. `rrecov.cpp`: the holder retyped `std::unique_ptr<PartitionSlice>` (the create signature's own return; the `slice = part_slice.get()` upcast is unchanged) — the M-119 clamp surfacing compiles as designed.
2. `cr_ui.cpp`: `Level::Warning` → `Level::Warn` at both sites.

## Tests to add

The gate itself is the test (win_syntax_check); the process defect it exposed — lanes recording green suites from `--target cr_tests` runs that skip the app TU — is recorded here for the CI note: the local-mirror gate must build the full default target set, not a filtered one.

## Related findings

M-119 (the landing that introduced the break); N-133/M-120 (the committed-red family precedent); the S-055/S-056/M-128/M-129/M-133/N-175 window rides the same merge.

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-186 repair, riding the N-158 convergence push — post-push SHA in the worklog entry)
**Verified by:** win_syntax_check green at the repaired tree (was Failed with the five quoted errors at `64fc2d8d`); the full suite 1340/1340 (1339/1340 pre-repair).
**Resolution notes:** The two-line repair landed exactly as the fix plan: the unique_ptr holder retyped to `PartitionSlice` (the create factory's own return type — the interface erasure was the defect, not the calls) and the two `Level::Warning` uses corrected to the enum's actual `Warn`. No behavior change on any platform — the Linux app binary was unbuildable at the window, so the repair restores the M-119 design intent rather than altering it. The process lesson (green-suite claims from target-filtered runs) is recorded in the finding for the CI lane.
