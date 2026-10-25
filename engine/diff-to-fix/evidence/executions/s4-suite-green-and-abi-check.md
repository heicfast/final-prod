# Executed evidence — session 4: full suite + C-ABI check green at the audited commit

**Audited commit:** `63a9167` (session-4 prompt commit on top of `abe26b8`; core lineage
`e6d5b01` ← `abe26b8` ← `c33236f` [A-004+S-016 fix] ← `7d0ffa6` [s3 close-out]).
**Corpus commit:** `03ad46c` (unmoved since session 3).

## Environment

- Linux container, g++ 13.x (`g++ --version`: 13.x), CMake 4.4.4 (pip-installed), nproc N.
- Configure: `cmake -S . -B build-s4 -DCR_BUILD_TOOLS=OFF -DCR_BUILD_APP=OFF -DCR_BUILD_UI=ON -DCR_ENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build-s4 -j$(nproc)` — 100%, zero warnings surfaced (gate is
  `-Wall -Wextra -Wpedantic -Werror -Wshadow` for GCC per CMakeLists.txt:30-33).

## Results (verbatim tails)

```
$ ./tests/cr_tests
[==========] 306 tests from 45 test suites ran. (3951 ms total)
[  PASSED  ] 306 tests.

$ ./tests/cr_ui_abi_check
cr_ui_abi_check: OK (4 progress calls, 0 recovery calls)
```

- `cr_tests`: **306/306 PASSED** (3.9 s). This is the post-A-004/S-016-fix count
  (session-3 close recorded 305/305; `abe26b8` added `Ldm.GptDynamicDiskRealCanonicalGuidsParse`
  → 306).
- `cr_ui_abi_check` (plain-C consumer, full lifecycle scan→session→snapshot→query→
  preview→recover→report over a synthetic FAT16 image): **OK**, 4 progress callbacks,
  0 recovery callbacks.

## What this evidence pins

1. The audited tree is buildable and green on Linux at the audited commit — every
   finding filed this session is filed against a working tree, not a broken one.
2. The C ABI shim loads and completes a full lifecycle from a plain-C consumer —
   the ABI surface audited in session 4 (61 exports) is exercised end-to-end.
3. Baseline for fix legs: any fix landing after this session must keep 306/306 +
   ABI check green, plus its own fail-before/pass-after pair.

Session-4 auditor, read-only leg. Harnesses and build tree live outside the repo
(`../scratch/`, `build-s4/`); no source was modified.
