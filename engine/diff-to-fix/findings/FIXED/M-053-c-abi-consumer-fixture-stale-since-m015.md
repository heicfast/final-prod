# MAJOR M-053 — C-ABI consumer check broken since M-015: fixture lacks the FAT16 media words, every run dies BAD_MAGIC + NULL deref

**Severity:** MAJOR
**Module:** quality (cross-cutting) — the C ABI consumer gate (`cr_ui_abi_check`)
**Parameters affected:** Q66/Q67 family (gate health — the gate is REGISTERED in ctest and FAILS on every push since M-015), R56 (C ABI consumer contract — the check is the MSVC `/W4 /WX` plain-C proof), P42 (public surface — the C lifecycle the check proves is currently unproven)
**Core references:** `tests/ui_abi_check.c` `build_fat16()` (:99-131 at filing) — the fixture writes FAT entries for clusters 5/10/11/12/20 on both copies but **never writes the FAT[0]/FAT[1] media words** (both stay zeroed from `calloc`); `src/core/fat/cr_chain.cpp:196-204` — the M-015 whole-table validator rejects FAT16 when `u32@FAT[0] != 0xFFFFFFF8` (exact pair), and `FatTable::load` returns `ERR_BAD_MAGIC` (:162-172); `tests/CMakeLists.txt:104-105` — `add_test(NAME cr_ui_abi_check ...)` is in the default ctest set (`ctest --preset linux-gcc` runs it on CI)
**Corpus references:** N/A (quality-parameter finding, same class as M-006 — the repo's own gate is the reference; the og-parity FAT validator itself is M-015's fixed behavior and is CORRECT)
**Status:** FIXED this session (see Resolution)

## Summary

The M-015 fix (commit `6dc1e32`, "FAT whole-table validation + copy retry") taught `FatTable::load` to reject a FAT16 table whose `u32@FAT[0]` is not the canonical `0xFFFFFFF8` media pair — the og's exact `CFAT16` factory behavior. The same commit updated the C++ fixtures to carry the canonical pair (`tests/unit/test_cr_ui.cpp` "M-015: canonical media words (real-volume shape)"), but **missed the plain-C consumer fixture** in `tests/ui_abi_check.c`. Since that commit, every `cr_ui_abi_check` run — local and CI, release and ASan — fails: the scan of the zero-media-word image returns `ERR_BAD_MAGIC`, every downstream CHECK fails in cascade, and the check dies with a NULL dereference at the query section (`rows[0].name` with `rows == NULL`, ui_abi_check.c:576 at filing) because its CHECK macros record failures without halting. On the release lane the crash exits 139 with the buffered FAIL output lost; on the ASan lane it surfaces as the SEGV report.

## What the reference does

The repo's own contract: the C consumer check exists to prove the flat-C ABI under a plain C compiler (ABI-CONTRACT; `tests/CMakeLists.txt:78-88` — "PLAIN-C consumer: proves the header compiles/links under a C compiler and drives the full lifecycle"), and CI runs it via ctest on every push. A registered gate that crashes on every invocation is a red CI lane masking every real regression it was built to catch — the same defect class as M-006 (fuzz job `if: false`), except this one fails loudly-and-blindly instead of silently.

## What our core does (at filing)

`build_fat16` in ui_abi_check.c writes a FAT16 image whose FAT sectors start at zero (calloc) — the exact shape the M-015 validator rejects as non-canonical. The scan fails; the check's non-halting CHECK macros let execution continue into sections that dereference outputs the failed scan never produced.

## Gap

Falsifiable claim (executed this session, both lanes): at HEAD, `./tests/cr_ui_abi_check` exits 139 (release, silent) / SEGV (ASan) on every run. `ctest --preset linux-gcc` includes `cr_ui_abi_check` — the Linux CI lane cannot be green at any commit since `6dc1e32` unless something else masks it.

## Impact

The C ABI consumer gate — the only plain-C proof of the exported surface, and a Windows-CI-paired contract — has been dead since M-015 landed. Real ABI regressions on the C surface (the exact class the check exists to catch: struct layouts, calling conventions, lifecycle contracts) pass unnoticed while the gate crashes on a stale fixture. All session-5/6 "green" claims that included the abi check in their verification were actually green only because the crash's exit code was mis-read through pipes or the lane was not run.

## Reproduction / verification

Executed first-hand this session (2026-10-26, tree `4a924d1`+):

- Baseline (S-009 changes stashed): release `./tests/cr_ui_abi_check` → exit 139, zero stdout (buffer lost in the crash); ASan lane → `SEGV ... ui_abi_check.c:576 in main`.
- With stdout forced unbuffered (`LD_PRELOAD` asan+setvbuf ctor): the true first failure is `FAIL ...:327: cr_ui_scan_run(scan) == CR_UI_OK (1342373889 != 0)` — `0x50030001` = `ERR_BAD_MAGIC`; `cr_ui_last_error()` reads `"scan bad magic"`.
- Minimal deterministic reproducer (plain C, links `libcr_ui`): the C++ fixture's image with the media words scans OK (`files=2 deleted=1`); the same image with `FAT[0]=FAT[1]=0` returns `0x50030001` — on release AND ASan. Byte-diff of the two images: exactly the two 4-byte media-word pairs differ.
- Cross-check: `git log -- tests/ui_abi_check.c` shows the fixture was last touched by `d374064` (pre-M-015); `git log -- src/core/fat/cr_chain.cpp` shows the validator landing at `6dc1e32` (M-015).

## Fix plan (for the engineering team)

1. Root cause: add the canonical media-word pair to the C fixture (`FAT[0]=0xFFF8, FAT[1]=0xFFFF` on both FAT copies), matching the C++ fixtures M-015 itself updated — the fixture then describes a real FAT16 volume, which is what a "real-volume shape" validator demands.
2. Robustness (the cascade): a failed scan is a failed PRECONDITION — bail out cleanly after the scan section (`if (g_failed) { print last_error; cleanup; return 1; }`) instead of walking into NULL derefs. A real C consumer checks the return code and stops; the check must model that.
3. Acceptance: `cr_ui_abi_check` exits 0 with the full lifecycle green on release AND ASan; `ctest` reports it passing.

## Tests to add

The fixed gate IS the test (CI-level, like M-006). The fixture now also implicitly pins the M-015 media-word contract through the C surface.

## Related findings

M-015 (the fix whose fixture update was missed — its Resolution's test list covered the C++ fixtures only); M-006 (the same gate-health class); N-018 (UBSan-not-in-CI — same family of gate-coverage questions); S-009 (this session's fix that surfaced the crash during its verification pass).

## Open questions

None — root cause executed and proven deterministic on both lanes.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-053 commit — see `git log` for "fix: M-053")
**Verified by:** `cr_ui_abi_check` exits 0 with the full C lifecycle green ("cr_ui_abi_check: OK (4 progress calls, 0 recovery calls)") on BOTH the release lane and the ASan lane (`ASAN_OPTIONS=detect_leaks=0`, local execution this session); fail-before executed and recorded above (exit 139 / SEGV at baseline, first true failure `cr_ui_scan_run` → `ERR_BAD_MAGIC` with the deterministic plain-C reproducer isolating the media-word pair as the sole differing bytes).
**Resolution notes:** Root cause was the fixture, not the engine — M-015's exact-pair validator is correct og parity (`CFAT16` FUN_180041b00), so the fix adds the canonical real-volume media words to `tests/ui_abi_check.c build_fat16()` (both FAT copies), exactly matching the C++ fixtures the M-015 commit itself updated. The second half hardens the check's control flow: a failed `cr_ui_scan_run` now halts the check with the diagnostic (`cr_ui_last_error()`) and a clean exit 1 instead of cascading CHECK failures into the `rows[0]` NULL deref — modeling what a real C consumer does with a failed precondition. No production code changed. The C ABI consumer gate is live again on every push (ctest); future C-surface regressions will be caught.

### Concurrent-filing reconciliation (session 9, both legs preserved)

A parallel session (this file's author, landing `7c44bdc`) and the session-9
diff-fix leg independently root-caused and fixed this same regression in the
same window — the parallel lane via a deterministic plain-C reproducer and
this landing (fixture + guard), the session-9 leg via a worktree bisect
pinned at the regression point (`63d9067` green / `6dc1e32` SEGV) with the
full causal-chain repro. The session-9 leg filed the same defect as
`M-053-m015-landing-missed-c-consumer-fixture-ci-red.md` (kept side-by-side,
append-only) before seeing this landing; both files stand as the two legs'
records of one defect. Fix attribution: this landing's fixture bytes +
guard are what main carries; the session-9 file's evidence
(`evidence/executions/s9-m053-abi-fixture-regression.md`) adds the
regression-point worktree proof. No content conflict — the repairs were
byte-equivalent in effect.
