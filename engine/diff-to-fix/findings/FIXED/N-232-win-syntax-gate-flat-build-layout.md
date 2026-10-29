# MINOR N-232 (renumbered from N-231 at the collision repair per SA-9: b4bb09f6's earlier push holds N-231) — the win_syntax_check gate's gtest-include discovery misses the flat build layout: the N-228 test-file section hard-fails on a direct `cmake build/` configure (`build/_deps/...`), while the gate only globs preset subdirectories (`build/*/_deps/...`)

**Severity:** MINOR (a gate false-red on a legitimate build layout — the gate that exists to catch pushed-red was itself red on a healthy tree; the inverse of N-116's missing-stub class)
**Module / Surface:** CI / the M-055 win-layer parse gate (tools/win_syntax_check.sh, the N-228 test-file section)
**Parameters affected:** Q68 (audit-trail/CI truth — the gate's coverage silently depended on the build-directory naming convention)
**Corpus references:** none (a core-repo gate robustness defect)
**Core references:** found at this lane's post-landing verification (2026-10-30, first-hand): `ctest` → `win_syntax_check ... ***Failed` with `FAIL: no gtest include dir found (build one preset first - FetchContent drops it at build/*/_deps/googletest-src/googletest/include - or export GTEST_INCLUDE). The N-228 test-file section cannot run; a missing gate is a failure, never a skip.` — on a tree whose 13 src/win files ALL parse (the gate's own `ok:` lines) and whose full suite is otherwise green. The gate's candidate loop (tools/win_syntax_check.sh:103) globs ONLY `"$SRC_DIR"/build/*/_deps/googletest-src/googletest/include` — the preset-subdir layout (build/<preset>/_deps/..., the CI lanes' `cmake --preset` convention). A direct `cmake -B build` configure drops FetchContent at `build/_deps/googletest-src/googletest/include` — one directory level shallower — which the glob never matches. The GTEST_INCLUDE escape hatch exists but is a manual workaround, not a fix: any lane or contributor configuring the documented plain way gets a false red.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The N-228 section hard-depends on gtest's headers (GTEST_OS_WINDOWS pulls `<direct.h>`/`<io.h>` — the win-stub surface), so the gate must locate the FetchContent include dir from the build tree. The discovery loop assumed the preset layout (`build/*/_deps/...`) and treated "not found" as a failure (the correct posture — a missing gate is never a skip). The flat layout (`build/_deps/...`) — the CMake documentation's own default form and what a plain `cmake -B build && cmake --build build` produces — was never a candidate, so the gate false-reds on exactly the trees that followed the simplest documented path. The failure mode is pernicious in the other direction too: a lane that sees the false red learns to export GTEST_INCLUDE locally, and the gate's coverage quietly stops being exercised the documented way.

## Gap

Falsifiable: configure with `cmake -B build` (no preset), build, then run `bash tools/win_syntax_check.sh` → the `FAIL: no gtest include dir found` line and exit 2, while `ls build/_deps/googletest-src/googletest/include` shows the directory exists. First-hand this session.

## Fix

Add the flat-layout candidate to the discovery loop: the glob list now covers `build/*/_deps/googletest-src/googletest/include` (the preset lanes) AND `build/_deps/googletest-src/googletest/include` (the direct configure). The GTEST_INCLUDE override and the not-found-is-a-failure posture are unchanged.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S89 fix lane, 2026-10-30)
**Verified by:** `bash tools/win_syntax_check.sh` on the flat-layout tree → **18/18 ok** (the 13 src/win files + the 5 tests/unit/test_*_win.cpp files of the N-228 section — the section now runs to completion instead of aborting at discovery), zero FAIL lines; the full ctest re-run green with the gate in-suite.
**Resolution notes:** RED first-hand (the false FAIL + exit 2 on a tree whose every gated file parses), GREEN after the one-line candidate addition. The gate's own not-found-is-failure discipline is exactly right — the defect was the discovery's layout assumption, not the posture. The N-116/N-224 stub-growth contract is untouched (no stub content changed); the fix is discovery-only. Lesson recorded for the gate family: every path a contributor or lane can legitimately produce must be a candidate — a gate that assumes one layout convention makes every other layout a false red, and false reds teach lanes to bypass gates.
