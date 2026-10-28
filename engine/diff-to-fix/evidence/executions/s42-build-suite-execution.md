# s42 execution log — the W4 build + full-suite run at the audited HEAD `b26017a7`

**Date:** 2026-10-29 · **Runner:** the S42 continuation audit lane · **Command transcript (fresh out-of-tree build):**

```
$ git clone … core_recovery && git rev-parse HEAD
b26017a7ae0f9b35d5b3a468297c4b64b067d5b4

$ cmake -S . -B build-s42 -DCMAKE_BUILD_TYPE=Release -DCR_BUILD_APP=ON
[cmake exit 0]

$ cmake --build build-s42 -j$(nproc)
[build exit 0 — 100% Built target cr_tests; zero warnings in the tail]

$ ./build-s42/tests/cr_tests
[==========] Running 1136 tests from 91 test suites.
[==========] 1136 tests from 91 test suites ran. (50933 ms total)
[  PASSED  ] 1093 tests.
[  SKIPPED  ] 42 tests (the Win-only set).
[  FAILED  ] 1 test, listed below:
[  FAILED  ] Carve.OoxmlRelsPathSplit
```

## The failure detail

```
tests/unit/test_carve.cpp:8463: Failure
Expected equality of these values:
  tail          Which is: ".docx"    (the actual name's tail, LOWERCASED by the loop at :8458-8463)
  c.ext         Which is: ".DOCX"    (the expectation table, uppercased by commit b26017a7)
… the same pair for ".xlsx"/".XLSX" and ".pptx"/".PPTX"
```

## Verdicts

1. **The engine behavior is CORRECT** — the carve emits the uppercase extension the S-049 contract requires (`c.ext` reflects the emitted name; the test's own lowercasing fold is what mismatches).
2. **The test is self-defeating at HEAD** — commit `b26017a7` (the session-42 opening hygiene commit) re-pinned the expectation table to uppercase but left the `ch + 32` fold loop, so the assertion can never pass. **Filed as N-185.**
3. **The record's green claims are refuted by execution**: the commit message's "T1 baseline recorded (1092/1092 green)" and the fix-lane worklog's "1134 ran / 1092 passed / 42 skipped / 0 failed on the repaired tree" — the executed truth at the same commit is **1136 / 1093 / 42 / 1 FAILED**. The +2 count drift (1134→1136) indicates two tests landed in the window between the recorded run and this one (or the recorded count was stale); N-185's fix lane should re-pin the canonical count.
4. **No finding regressed**: the failing test is the hygiene commit's own edit; the S12 lane's 1117/1117 record predates the N-115/N-153/S-049 landing window's test additions.

## The read-only guarantee (this execution)

The build directory `build-s42/` is out-of-tree and gitignored-equivalent (not committed — verified `git status` shows only `diff-to-fix/` + `worklog*` files as this lane's writes before the commit). No source, test, build, or workflow file was modified by this run.
