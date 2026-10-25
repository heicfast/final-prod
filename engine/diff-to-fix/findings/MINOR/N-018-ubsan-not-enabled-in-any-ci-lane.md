# MINOR N-018 — UBSan is not enabled in any CI lane (only ASan + TSan)

**Severity:** MINOR
**Module:** quality (cross-cutting)
**Parameters affected:** Q67 (sanitizer cleanliness — ASan/UBSan/TSan clean — UBSan is the missing third prong)
**Corpus references:** `diff-to-fix/PARAMETERS.md:134` (Q67 definition: "Sanitizer cleanliness — ASan/UBSan/TSan clean")
**Core references:** `.github/workflows/ci.yml:23-34` (linux-gcc preset enables ASan on cr_core/cr_ui/cr_tests/fuzz on every push); `.github/workflows/ci.yml:52-71` (linux-tsan runs TSan); grep across `CMakeLists.txt` and `src/*/CMakeLists.txt` and `tests/CMakeLists.txt` for `fsanitize` finds only `-fsanitize=address` and `-fsanitize=thread`; NO `-fsanitize=undefined` anywhere; no UBSan suppression file exists (good — but the absence of the flag is the gap) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The CI runs ASan on every push and TSan on a separate lane; UBSan is
not enabled anywhere. The `PARAMETERS.md` Q67 definition explicitly
calls out all three sanitizers as the standard; the audit prompt
question 67 was marked MATCH-with-note in session 1's cross-ref matrix
("ASan/TSan CI green; TSan separate job") but the UBSan component
was not actually verified. Session 1's "M" verdict for Q67 was
incomplete.

UBSan catches a different class of bugs than ASan (which catches
memory errors) and TSan (which catches data races): signed integer
overflow, shift count overflow, out-of-range enum casts, misaligned
loads, null pointer dereferences (well, UBSan catches the
non-dereferencing null arithmetic; ASan catches the actual deref),
and other undefined behavior. The parsers most likely to surface
UBSan findings: the runlist codec (`cr_runlist.cpp`), the USA fixup
(`cr_fixup.cpp`), the FAT/exFAT checksum loops (`cr_exfat_dentry.cpp`),
and the carve analyser math (`cr_analysers.cpp`).

## What the reference does

The corpus does not document the og's sanitizer posture (the og is a
Windows-only binary; the question of UBSan doesn't apply to the
og-codebase shipped product, only to the core's testing discipline).
This is a quality-parameter finding, not an og-parity finding.

## What our core does

```yaml
# .github/workflows/ci.yml (audited 5d6da5f)
jobs:
  linux-gcc:
    runs-on: ubuntu-latest
    env: { CXX: g++, CC: gcc }
    steps:
      - uses: actions/checkout@v4
      - run: cmake --preset linux-gcc -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
      - run: cmake --build --preset linux-gcc
      - run: ctest --preset linux-gcc

  linux-tsan:                       # <-- separate job
    runs-on: ubuntu-latest
    env: { CXX: g++, CC: gcc }
    steps:
      - uses: actions/checkout@v4
      - run: cmake --preset linux-tsan -DCMAKE_BUILD_TYPE=Debug
      - run: cmake --build --preset linux-tsan
      - run: ctest --preset linux-tsan
```

NO `linux-ubsan` job. Grep across `CMakeLists.txt` for
`fsanitize=undefined` returns empty.

## Gap

ASan + TSan are exercised on every push; UBSan is not exercised at all.
Q67's three-pronged definition (ASan/UBSan/TSan) is partially met.

## Impact

- **Quality:** UBSan-only bugs (signed overflow, shift overflow, enum
  casts, misaligned loads) slip through CI. These are real undefined-
  behavior bugs that ASan doesn't catch.
- **Parser robustness:** the runlist codec, FAT/exFAT checksum loops,
  and USA fixup are the most likely sites for UBSan findings (bit
  manipulation, signed arithmetic on attacker-controlled data).
- **No correctness impact today:** no known UBSan bug; the gap is the
  missing detection.

## Reproduction / verification

- **Core evidence:** first-hand read of `.github/workflows/ci.yml`,
  `CMakeLists.txt`, and `src/*/CMakeLists.txt`. No `-fsanitize=undefined`
  anywhere; no `linux-ubsan` job.
- **Test run (proposed):** build with
  `CMAKE_CXX_FLAGS=-fsanitize=undefined` and run the suite — many
  signed-overflow / shift-count findings would likely surface in the
  parsers. Not actually run in this audit (out of scope: this is a
  "should be enabled" finding, not a "should be fixed" finding).

## Fix plan (for the engineering team)

1. **Architectural change:** add a `linux-ubsan` CI lane mirroring the
   existing `linux-tsan` job.
2. **Files to modify:**
   - `.github/workflows/ci.yml` — add a `linux-ubsan` job:
     ```yaml
     linux-ubsan:
       runs-on: ubuntu-latest
       env: { CXX: g++, CC: gcc }
       steps:
         - uses: actions/checkout@v4
         - run: cmake --preset linux-ubsan -DCMAKE_BUILD_TYPE=Debug
         - run: cmake --build --preset linux-ubsan
         - run: ctest --preset linux-ubsan
     ```
   - `CMakePresets.json` — add a `linux-ubsan` preset:
     ```json
     {
       "name": "linux-ubsan",
       "inherits": "linux-gcc",
       "cacheVariables": {
         "CMAKE_CXX_FLAGS": "-fsanitize=undefined -fno-sanitize-recover=undefined -g",
         "CMAKE_EXE_LINKER_FLAGS": "-fsanitize=undefined"
       }
     }
     ```
3. **Algorithm/constant:** `-fsanitize=undefined` enables the full
   UBSan check set. `-fno-sanitize-recover=undefined` makes UBSan
   findings abort (rather than continue with the undefined behavior).
4. **Tests to add:**
   - The `linux-ubsan` job IS the test — every existing test runs
     under UBSan. Fix the UBSan findings that surface (likely in
     `cr_runlist.cpp`, `cr_fixup.cpp`, `cr_exfat_dentry.cpp`,
     `cr_analysers.cpp`).
5. **Acceptance criteria:** the `linux-ubsan` job passes (all
   existing tests run under UBSan without findings).

## Tests to add

- `linux-ubsan` CI job (the test IS the CI lane; no per-finding test
  needed beyond making the existing suite pass under UBSan).

## Related findings

- **M-006** — fuzz CI disabled. Both are quality-parameter findings
  about CI coverage. M-006 is MAJOR (fuzz coverage is a security
  surface); N-018 is MINOR (UBSan is a defensive layer, not a primary
  security gate).

## Open questions

- Should UBSan be combined with ASan in a single lane (the
  `-fsanitize=address,undefined` form), or kept separate? ASan +
  UBSan together is the standard "kitchen sink" combo, but separate
  lanes give cleaner error attribution. Engineering decision.
