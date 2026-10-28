> **S42-C pin-time + renumber note (added at the third reconciliation):** this
> sheet records the tree state as verified at the lane's pin **`869da779`** (core) /
> **`be0cfe9`** (corpus). THREE hot windows landed while this lane worked; the
> sheet's internal draft IDs are superseded as follows: this lane's "M-118"/"M-121"
> (the Stbio large-IDAT finding) → final **M-122**; "I-035"/"I-037" (the diskhealth
> register) → final **I-038**; "I-036"/"I-038" (the corpus-worklog register) → final
> **I-039**; the red-main draft "N-154" → folded as the N-157 corroboration addendum.
> The remote lanes' same-numbered findings (the S12 MountPoints2 M-118, the DBX
> chain's M-121, the decode register I-037, the S42 lane's config-layer I-036) are
> different subjects. The sheet's liveness verdicts for findings the windows have
> since FIXED (S-048, M-117/M-121-dbx, N-152, N-125, N-126, N-135, N-148,
> S-054-the-N-013-regression, N-149, M-076, the INFO wave, N-115's N-158 tail
> aside) are pin-time-historical (true at the pin, superseded at the reconciled
> tree). The landing-parity VERIFIED-PARITY verdicts are unaffected. This lane is
> the S42-C lane (rebranded twice: the remote's S42 and S42-B lanes pushed first).

# S42-B executed evidence — the red-main reproduction (the corroboration of N-157)

**Lane:** S42-B (rebranded at the hot-window reconciliation), 2026-10-29 · **Tree pins:** core `869da779` (the lane's open) and the landing commit `67de3276` (a clean `git worktree` build) · **Build:** `cmake --preset linux-release && cmake --build --preset linux-release -j` (cmake 4.4.4, gcc 14, Linux)

## RUN 1 — HEAD-at-open baseline (the T0 duty)

```
$ ./build/linux-release/tests/cr_tests
[==========] 1134 tests from 91 test suites ran. (51468 ms total)
[  PASSED  ] 1091 tests.
[  SKIPPED ] 42 tests. (Win-only env skips)
[  FAILED  ] 1 test: Carve.OoxmlRelsPathSplit
```

Isolated run:
```
$ ./build/linux-release/tests/cr_tests --gtest_filter='Carve.OoxmlRelsPathSplit'
tests/unit/test_carve.cpp:8453: Failure
Expected equality of these values:
  nm.compare(nm.size() - 5, 5, c.ext)
    Which is: -32      (the ASCII case delta — produced UPPERCASE vs expected lowercase)
```
`cr_ui_tests`: 61/61 PASSED.

## RUN 2 — the landing commit itself (the fail-before at the pushed tree)

```
$ git worktree add <path> 67de3276
$ cmake --preset linux-release && cmake --build --preset linux-release -j
$ ./build/linux-release/tests/cr_tests --gtest_filter='Carve.OoxmlRelsPathSplit'
[  FAILED  ] 1 test: Carve.OoxmlRelsPathSplit   (same :8453 compare, -32)
```

**Verdict:** the test was RED in the N-115 landing's own tree (S-049 `d4b487c8` is
an ancestor — `git merge-base --is-ancestor d4b487c8 67de3276` → exit 0),
falsifying the Resolution record's "Suite 1119/1119 (1077 pass / 42 env skips /
0 fail)" (findings/FIXED/N-115-…:58). **Convergent with the remote lanes' N-157**
(this lane's independent discovery + second-method reproduction folds as the
corroboration addendum on that file per extend-don't-refile; the remote's
b26017a7/d10e4aa2/1fb7603b/b00118b5 repairs stand and the reconciled tree runs
green — 1104/0/85 then 1152/1152 then 1215/1215 across the window's records).

## RUN 3 — the product-side sanity check (which side is right)

The same file's S-049-era pin at test_carve.cpp:319
(`EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 5), ".DOCX")`) is
GREEN at the pin — the product emits the uppercase contract; only the N-115
test's expectation table (:8443-8453) was stale. The corpus side (DRWUI
FUN_140c44770 ~:1878330-81, the S-049 decode re-verified by the W2 pass) holds
uppercase — see evidence/s42b-landing-parity.md §2 (the S-049 section).
