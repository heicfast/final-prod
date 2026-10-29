# MINOR N-227 — the racing window's stash-abort sweep deleted a test file the newer base had registered and implemented: tests/CMakeLists.txt references unit/test_m101_branches.cpp at HEAD, cr_tests cannot build — main RED (the N-210 gate-skipping class's EIGHTH occurrence)

**Severity:** MINOR (main RED at the open — the build-stage class; every test on the tree unbuildable)
**Module / Surface:** tests / the M-101 landing's registration seam (tests/CMakeLists.txt + tests/unit/test_m101_branches.cpp)
**Parameters affected:** Q68 (audit-trail/CI truth — a lane pushed a tree it never built), Q61-adjacent (the M-101 test battery's presence)
**Corpus references:** none (a core-repo build-integrity defect)
**Core references (all first-hand this session):** the merged HEAD `d15d32a4` — `tests/CMakeLists.txt:5`'s explicit list carries `unit/test_m101_branches.cpp` (added by `b7d05b12`, the M-101 implementation landing, `git log -S test_m101_branches -- tests/CMakeLists.txt`), while the FILE is absent (deleted by `708fad32`, the "stash-abort leak sweep"). `cmake --preset linux-release` → `CMake Error at tests/CMakeLists.txt:5: Cannot find source file: unit/test_m101_branches.cpp` / `No SOURCES given to target: cr_tests` — cr_tests unbuildable, 0 of 1700 tests runnable. The racing sequence reconstructed from the two commits' own trees: `708fad32` (18:46:33Z) examined a tree WITHOUT the M-101 implementation (its message's own evidence: "grep clean for version_group in include/"), declared the leaked file "UNREGISTERED ... UNIMPLEMENTED", and deleted it — but `b7d05b12` (18:43:25Z, merged into the same window's base) had registered the file AND landed the implementation (version_group in cr_node.h/cr_query.h/cr_mftwalk.cpp/cr_snapshot.cpp/cr_records.cpp — all present at HEAD). The sweep's three-defect analysis was correct against ITS stale tree and destructive against the merged one: the lane never rebuilt after the pull.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

Two racing lanes crossed on the M-101 surface: the implementation lane registered and implemented `test_m101_branches.cpp` (b7d05b12, 18:43); the sweep lane — examining the pre-implementation tree where the file genuinely was an unregistered, unimplemented stash-abort leak (fe5d3af0) — deleted it (708fad32, 18:46) without rebuilding the merged tree. The merged HEAD carries the registration without the file: `cr_tests` fails at CMake generate, the entire suite (1700 tests, including the racing window's own new landings) is unbuildable. The exact class the trail's meta-lesson names: every red this session was a gate-skipping push; the sweep lane pushed a deletion whose blast radius it never built.

## Gap

Falsifiable: at `d15d32a4` (and at the racing window's own `f937081e` tip), `cmake --preset linux-release && cmake --build --preset linux-release` fails at generate with "Cannot find source file: unit/test_m101_branches.cpp". `grep test_m101_branches tests/CMakeLists.txt` → present; `ls tests/unit/test_m101_branches.cpp` → absent. First-hand this session: the fresh configure on the merged tree errored exactly so.

## Fix

Restore the file from `b7d05b12`'s tree (`git checkout b7d05b12 -- tests/unit/test_m101_branches.cpp`): the restored battery is exactly the implementation the landing registered it against — `FileProperties::version_group`, `tags_extracted`, `query::version_groups()`, the `Filter::version_group` axis — all present at HEAD (grep-verified). The 708fad32 sweep's own worklog record (its "design hints for that lane") is superseded by the landing it raced against; both histories stand.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S89 fix lane, 2026-10-30)
**Verified by:** RED first-hand at `d15d32a4`: `cmake --preset linux-release` → "Cannot find source file: unit/test_m101_branches.cpp", cr_tests unbuildable (0/1700 runnable). GREEN post-restore: the fresh build links `cr_tests` clean, `ctest -R M101` → 5/5 (the restored battery: VersionsNodeGroupsTornSiblings, the tags axis, the session round-trip, the three-landed-branches pin, the query axis), and the full suite **1700/1700** green (127.7 s).
**Resolution notes:** The file restored byte-exact from `b7d05b12` (the landing that registered it); no implementation change owed — the M-101 surface (version_group / tags_extracted / version_groups()) is complete at HEAD and the restored battery pins it. The racing window's sweep (708fad32) was correct against its own stale tree and is preserved as history; the merged-tree red it created is this finding's defect. The class recurrence count (the N-210 "push without building the pulled head" family) reaches EIGHT — the procedural fix remains the trail's standing lesson: rebuild AND rerun the suite against the pulled head immediately before every push.
