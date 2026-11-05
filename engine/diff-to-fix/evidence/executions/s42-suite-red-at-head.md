# S42 execution evidence — the suite is RED at HEAD `869da779` (Linux release build)

**Run date:** 2026-10-29 (session-42 lane) · **Machine:** the audit sandbox
(x86_64 Linux, gcc 14 / cmake 4.4.4, `-DCMAKE_BUILD_TYPE=Release
-DCR_ENABLE_TESTS=ON`, out-of-source at `/tmp/cr-build` — the repo tree
untouched; this is the standard Linux-release configuration the trail's
own fix lanes certify with).

## 1. The run

```
$ cmake /home/z/audit/core_recovery -DCMAKE_BUILD_TYPE=Release -DCR_ENABLE_TESTS=ON
$ cmake --build . -j$(nproc)          # EXIT 0 — the tree BUILDS clean
$ ./tests/cr_tests
[==========] Running 1134 tests from 91 test suites.
[==========] 1134 tests from 91 test suites ran. (52001 ms total)
[  FAILED  ] Carve.OoxmlRelsPathSplit     ← 3 assertion failures
[  SKIPPED ] 85 (the Win-only families)
 1 FAILED TEST
```

The failure (verbatim, ×3 — one per case row):

```
[ RUN      ] Carve.OoxmlRelsPathSplit
/home/z/audit/core_recovery/tests/unit/test_carve.cpp:8453: Failure
Expected equality of these values:
  nm.compare(nm.size() - 5, 5, c.ext)
    Which is: -32
  0
the typed extension
```

## 2. Root-cause (first-hand, both sides read)

- `-32` is the ASCII case delta: the carved name ends **`.DOCX/.XLSX/.PPTX`
  (UPPERCASE)** — the S-049 universal naming contract
  (`src/core/carve/cr_carve.cpp:1051-1090`, `default_namer`:
  `<UPPEREXT>_<%06d>.<UPPEREXT>`, `ext_up` fold at :1072-1075, verified
  read this session).
- The test — landed WITH the N-115 fix at commit `67de3276` (absent at the
  parent `75dd2c96`: `git show 75dd2c96:tests/unit/test_carve.cpp | grep -c
  OoxmlRelsPathSplit` → 0; present at `67de3276` → 1) — expects the
  **pre-S-049 lowercase** form: `tests/unit/test_carve.cpp:8431-8434`
  (`{".docx"} / {".xlsx"} / {".pptx"}` case rows) checked at `:8453`.
- The S-049 lane (d4b487c8) re-pinned the then-existing family tests to the
  uppercase contract ("the parallel lanes' mid-window tests (N-112 LZH,
  N-113 text school) updated for the uppercase contract in the same push")
  — but N-115's test was authored AFTER that landing, against the stale
  lowercase model, and its own lane's chain broke before the suite run
  (see §3).
- The product CODE is contract-correct on both halves this test probes:
  typing (`EXPECT_EQ(n0->props()->carve_format, c.fmt)` passes — the
  failure is only the extension-string check) and naming (the S-049
  uppercase form).

## 3. The broken gate (remote CI, read-only API check)

`67de3276` check-runs (GitHub API, this session):

```
Build + test (MSVC Release, vcpkg cache): completed / skipped
Lint + parse gates:                       completed / failure
```

The commit landed on `main` with the build+test job SKIPPED and the lint
gate FAILED. The follow-up bookkeeping commit `869da779`'s own message
confirms the broken chain: "the retirement bookkeeping the broken chain
step dropped — N-153 moved to FIXED/, the ledger rows and the worklog
entry that never ran land now (the fix itself is 67de3276)".

## 4. The lint violations (local clang-format 23.1.3, `--dry-run -Werror`)

Introduced in the `75dd2c96..869da779` window (= `67de3276`; the
bookkeeping commit touched no src/tests):

```
include/cr/carve/cr_signatures.h:325:98  (comment wrap)
src/core/carve/cr_analysers.cpp:1370:49  (whitespace)
src/core/carve/cr_registry.cpp:111:70    (double trailing comment)
tests/unit/test_carve.cpp:7501:42        (whitespace)
```

## 5. Scope notes

- Linux-only execution here (the sandbox has no Windows runner); the
  failing assertion is platform-independent string comparison — the
  Windows suite would fail identically (and its CI job was skipped).
- The 85 SKIPPED are the registered Win-only families — expected on Linux.
- Suite count context: 1134 ran here vs the trail's last certified green
  "1185/1185" (d4b487c8, Windows-inclusive) — the delta is the Win-only
  skips plus the N-115/N-153 test additions.

## 6. Commands (reproducible)

```
git clone https://github.com/heicfast/core_recovery && git checkout 869da779
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCR_ENABLE_TESTS=ON
cmake --build . -j$(nproc)
./tests/cr_tests 2>&1 | grep -E "FAILED|PASSED|tests ran"
clang-format --dry-run -Werror include/cr/carve/cr_signatures.h \
  src/core/carve/cr_analysers.cpp src/core/carve/cr_registry.cpp tests/unit/test_carve.cpp
```
