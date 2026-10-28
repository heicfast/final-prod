# S12/D — BUILD + EXECUTION VERIFICATION LOG

**Sub-agent:** D (build & test execution verification)
**Date:** 2026-10-28 (UTC)
**Repo:** `/home/z/my-project/audit-work/core_recovery`
**Result headline: BUILD OK — TESTS 1117/1117 PASSED (100%), 0 failed, 0 skipped.**

---

## 1. Tree identity (audit note: prompt/actual mismatch)

- Prompt claimed HEAD = `c7fa527c`. **Actual HEAD = `36c1d3e5`.**
- `36c1d3e5` is a worklog-only commit on top of `c7fa527c`:

  ```
  36c1d3e5 worklog(S12/T1): session-12 continuation lane open — core c7fa527c / corpus be0cfe9; ...
   worklog.md                                      | 19 ++++++
   worklog/session-12-continuation-audit-prompt.md | 62 +++++++++++++++++
   2 files changed, 81 insertions(+)
  ```

  → Source tree is identical to `c7fa527c`; only worklog docs differ. Not a problem for parity,
  but the prompt's "current HEAD c7fa527c" was off by one (worklog) commit.
- `git status --short` after build: clean (only `build/`, which is in `.gitignore`).
  **No source files were modified by this sub-agent.**

## 2. Environment

| Item | Value |
|---|---|
| OS | Debian (g++ 14.2.0-19) |
| Compilers | `/usr/bin/gcc`, `/usr/bin/g++` (GNU 14.2.0) |
| CMake | **4.4.4 — installed via `pip install cmake`** (system had NO cmake/ctest; venv `/home/z/.venv/bin`) |
| Generator | Unix Makefiles (per preset) |
| Jobs | 2 cores, built with `--parallel 2` |
| zlib | present (`pkg-config --exists zlib` → yes) |
| lz4 | present (`pkg-config --exists liblz4` → yes) |
| zstd | present (`pkg-config --exists libzstd` → yes) |
| GTest | NOT installed system-wide → CMake `FetchContent` fetched **googletest v1.15.2** from GitHub (network available) |

No apt packages needed to be installed; only `pip install cmake` was required.

## 3. Presets (from `CMakePresets.json`, schema version 6, cmakeMinimumRequired 3.25)

- Configure presets: `linux-gcc` (RelWithDebInfo + ASAN), `linux-release` (Release),
  `linux-ubsan`, `win-x64-debug`, `win-x64-release`.
- Build presets exist for all of the above.
- **Test presets exist ONLY for `linux-gcc`, `win-x64-release`, `linux-ubsan` — there is NO
  `linux-release` test preset.** Therefore ctest was invoked directly against the build dir
  (step 4 below).
- `linux-release` preset: `binaryDir = ${sourceDir}/build/linux-release`, `CMAKE_BUILD_TYPE=Release`.
  Note: `linux-release` does NOT disable tools/app (`CR_BUILD_APP` defaults ON but is
  Windows-only-gated in src; on Linux it builds `cr_core`, `cr_ui`, tools, tests).

## 4. Exact commands used

```bash
cd /home/z/my-project/audit-work/core_recovery
export PATH=/home/z/.venv/bin:$PATH        # cmake 4.4.4 from pip

# fresh from-scratch verification (no pre-existing build/ dir; confirmed "no build dir")
cmake --preset linux-release               # exit 0, 1.8s configure

cmake --build --preset linux-release --parallel 2   # exit 0 (see §5)

ctest --test-dir build/linux-release -N              # count only: Total Tests: 1117
ctest --test-dir build/linux-release --output-on-failure   # exit 0 (see §6)
```

No `build/linux-release` existed before (prior-session build dir was absent → genuinely
from-scratch). Nothing inside the repo was written except `build/` (gitignored).

## 5. Build result

- **Exit code 0.** Targets built (9): `gtest`, `gmock`, `gtest_main`, `gmock_main`,
  `cr_core`, `cr_ui`, `cr_ui_tests`, `cr_ui_abi_check`, `cr_tests`.
- **Warnings: 0** (`grep -ci warning` over the full build log = 0). Project compiles with
  `-Wall -Wextra -Wpedantic -Werror -Wshadow` (plus `-Wno-missing-field-initializers` on GCC),
  so this is consistent — no warning could have passed anyway.
- Wall time ≈ 3 minutes (first object 18:49:02 → final link 18:51:36) on 2 cores.
- Configure log tail:

  ```
  -- The C compiler identification is GNU 14.2.0
  -- The CXX compiler identification is GNU 14.2.0
  -- Could NOT find GTest (missing: GTEST_LIBRARY GTEST_INCLUDE_DIR GTEST_MAIN_LIBRARY)
  -- Found Threads: TRUE
  -- Configuring done (1.8s)
  -- Generating done (0.0s)
  -- Build files have been written to: .../build/linux-release
  ```

## 6. Test run result

- Command: `ctest --test-dir build/linux-release --output-on-failure`
- **Total Tests: 1117**
- **Passed: 1117 (100%) — `100% tests passed out of 1117`**
- **Failed: 0. Not Run / Skipped: 0** (grep counts: "Passed" = 1117, "Not Run|Skipped" = 0)
- Total test time (real): **64.23 sec** (single-threaded ctest scheduling)
- ctest exit code: 0
- First/last tests for the record:

  ```
  1/1117 Test #1: Fixup.ValidRecordApplies ....................... Passed
  1116/1117 cr_ui_abi_check ..................................... Passed  0.34 sec
  1117/1117 win_syntax_check .................................... Passed  8.22 sec
  ```

### Failure listing

*(none — empty by result)*

## 7. Audit-relevant discrepancies vs prior-session claims

1. **Claim: "suite 1027/1027 green (Linux release)". Actual at this tree: 1117 tests, 1117 green.**
   The count 1027 does not match this tree under the `linux-release` preset (1117). Either the
   claim is stale (tree has grown by ~90 tests since) or it was measured under a different
   preset/tooling. The *green* status itself reproduces; the *count* does not.
2. Prompt's HEAD `c7fa527c` vs actual `36c1d3e5` (worklog-only delta — see §1).
3. There is no `linux-release` **test preset** in CMakePresets.json — "ctest --preset
   linux-release" as suggested in the task would fail; direct `--test-dir` invocation was used.

## 8. Raw log locations ( ephemeral, this session )

- configure: `/tmp/cr-configure.log`
- build: `/tmp/cr-build.log`
- ctest: `/tmp/cr-ctest.log`

**Bottom line: build OK (0 warnings, -Werror clean), full suite 1117/1117 PASSED, 0 failed,
0 skipped, at actual HEAD 36c1d3e5 (= c7fa527c + worklog-only commit). No source modifications
were made.**
