# MAJOR M-053 — M-015 landing missed the C-consumer fixture: Linux CI gate red since 6dc1e32

**Severity:** MAJOR (NEW-FINDING, filed during the session-9 baseline verification)
**Module:** fat / test-infrastructure (the CI quality gate)
**Parameters affected:** the M-015 landing's fixture coverage (FAT whole-table
validation, `fat_table_validate` media-word gate)
**Corpus references:** the M-015 finding's corpus citation stands unchanged —
og CFAT16 `FUN_180041b00` :54052-141 (media word pair exact,
`u32@FAT[0] == 0xFFFFFFF8`); re-verified in-source at
`src/core/fat/cr_chain.cpp:181-199` (`fat_table_validate`, the Flavor::Fat16
exact media-word branch) this session.
**Core references:** `tests/ui_abi_check.c` `build_fat16()` (the plain-C
consumer fixture, "the test_orchestrator fixture, ported" per its header
comment) — the one FAT16 fixture builder in the tree that was NOT updated
when M-015 landed (`6dc1e32`).
**Status:** NEW-FINDING (filed during the session-9 build-baseline
verification) — CONFIRMED-REGRESSION (test infrastructure, not engine parity;
the engine code is og-correct)

## Summary

The M-015 landing (`6dc1e32`, "FAT whole-table validation + copy retry")
added the og's whole-table FAT validator, which rejects a FAT16 table whose
`u32@FAT[0] != 0xFFFFFFF8`. The landing updated the two C++ fixture builders
(`tests/unit/test_fat_volume.cpp:138-141`, `tests/unit/test_orchestrator.cpp:75-77`)
with the canonical media pair — but missed the third fixture builder,
`tests/ui_abi_check.c:build_fat16()`, whose header comment says it is the
test_orchestrator fixture ported to plain C. From `6dc1e32` onward:

1. `cr_ui_abi_check` (the plain-C ABI consumer proof, ctest #518 on Linux)
   fails with a SEGV — not because of an ABI defect but because the scan
   itself now returns `ERR_BAD_MAGIC` on the fixture's invalid FAT16 volume
   (both FAT copies carry FAT[0]=FAT[1]=0), so the session never materializes
   and the test's `rows[0]` dereference at :576 reads NULL+8.
2. The Linux CI gate (`ctest --preset linux-gcc`, ci.yml:33) has been red on
   every push since `6dc1e32` — the "leave main green" SOP has been violated
   by every landing in that window, none of which ran the full ctest set
   (the parallel legs ran the `cr_tests` binary only and reported
   "suite N/N green", which is true but incomplete: `cr_ui_abi_check` is a
   separate ctest entry).

## What the reference does

Unchanged from M-015: the og's CFAT16 validator requires the exact media-word
pair; a real FAT16 volume (any mkfs/mkfs.vfat/Windows format) always writes
`FAT[0]=0xFFF8` (media descriptor 0xF8 + 0xFF fill) and `FAT[1]=0xFFFF`.
The engine's rejection of the fixture is **correct og parity** — the fixture
was never a valid FAT16 volume; it only scanned because the pre-M-015 core
did not validate.

## What our core does

The engine (correctly) rejects the volume; the fixture (incorrectly) omits
the media pair. The defect is the incomplete landing: one of three fixture
sites missed, the C-consumer proof broken, the CI gate red for the entire
window `6dc1e32..c231f2f` (15 commits).

## Gap

Falsifiable claim: at `6dc1e32` the Linux ctest suite is red (cr_ui_abi_check
SEGV) while at its parent `63d9067` it is green — executed both ways this
session (worktree builds):

- `63d9067`: `ctest -R cr_ui_abi_check` → **100% tests passed** (0.23 s)
- `6dc1e32`: `ctest -R cr_ui_abi_check` → **0% tests passed** (SEGV,
  `ui_abi_check.c:576`, ASan: READ at 0x8)
- HEAD pre-fix (`c231f2f`): same SEGV (517/518, the one red)

## Impact

- The plain-C ABI consumer proof — the gate that proves the header compiles
  and links as C and drives the full lifecycle — has been dead since
  `6dc1e32`, so ABI regressions on that surface would not have been caught.
- The Linux CI gate has been red on every push in the window; landings were
  proceeding on a partial suite run (`cr_tests` only), which is exactly the
  failure mode the "no skipped stages" CI SOP exists to prevent.

## Reproduction / verification

Executed this session (see Gap): worktree build at the parent and at the
landing; HEAD reproduction via the standalone repro
(`scripts/repro_name_filter.c`, session scratch): scan returns
"scan bad magic" (`ERR_BAD_MAGIC` from `fat_table_validate` → the
`fat_validate_(p)` gate at `cr_chain.cpp:164`), `files_found=0`; with the
media pair added, the same fixture scans (`files_found=2, deleted_found=1`)
and the name-substring query returns `total=1, rows[0].name="DOC.TXT"` —
exactly what the abi test asserts at :574-576.

## Fix plan

1. `tests/ui_abi_check.c:build_fat16()`: write the canonical media pair into
   both FAT copies (`FAT[0]=0xFFF8`, `FAT[1]=0xFFFF`), matching the two C++
   fixture builders updated by M-015 — the fixture becomes a valid FAT16
   volume for the first time.
2. Re-run the FULL ctest set (not just `cr_tests`) — the lesson of this
   finding: "suite N/N green" claims must name the runner scope.

## Tests to add

None beyond the fix itself: `cr_ui_abi_check` IS the test; it fails before
(SEGV, executed) and passes after (518/518, executed). The fail-before is
intrinsic — the fixture could not scan at all.

## Related findings

- M-015 (the landing that missed this fixture — its own Resolution stands;
  this finding is the landing's missed tail, not a reversal).
- N-019 (missing per-parameter regression tests — same class: proof coverage
  gaps between what a fix changes and what the suite actually runs).

## Open questions

None.

---

## Resolution

**Status:** FIXED-ALREADY (the fixture repair landed on the remote inside the parallel DFX7 lane's `b1f9210` — "fix(N-018): UBSan CI lane ... first run caught three real defects, all fixed" — whose defect set carried the identical `build_fat16()` media-pair repair, with the in-code comment "recovered by the DFX7 session"; this session had independently root-caused and fixed the same defect in parallel, and the rebase union kept the remote's bytes)
**Fixed in commit:** `b1f9210` (fixture repair; the finding FILE and its evidence are this session's contribution — the parallel lane fixed the code without filing the regression)
**Verified by:** this session's independent before/after chain, executed BEFORE seeing the remote fix: worktree at `63d9067` (abi check green), worktree at `6dc1e32` (abi check SEGV — the fail-before proof pinned at the regression point), HEAD pre-fix `c231f2f` 517/518 with the SEGV; this session's fixture repair ran full ctest **518/518 green** (28.7 s). Standalone repro (session scratch): scan "bad magic" with 0 files pre-fix; `files_found=2 deleted_found=1`, name-substring query `total=1 rows[0].name="DOC.TXT"` post-fix. Post-rebase verification on the union tree below.
**Resolution notes:** CONFIRMED-REGRESSION (test infrastructure; the engine's media-word validation is og-correct M-015 parity and is preserved untouched — the remote's fix and this session's fix are byte-equivalent in effect: the canonical media pair into both FAT copies). The finding stands as the formal audit-trail record of the `6dc1e32..b1f9210` red-window: the plain-C ABI proof was dead and the Linux CI gate red on every push, because "suite N/N green" claims in that window named only the `cr_tests` runner — `cr_ui_abi_check` is a separate ctest entry and was the only red. Lesson recorded for future landings: a suite-green claim must name its runner scope. The S-008 repair block still exercises the torn-FAT1→mirror-promotion→rawfixer-restore chain on the now-valid volume.

### Concurrent-filing reconciliation (session 9)

The parallel FF lane landed the same defect's fix + their own M-053 filing
(`M-053-c-abi-consumer-fixture-stale-since-m015.md`, landing `7c44bdc` —
fixture bytes + a loud-failure guard, root-caused with a deterministic
plain-C reproducer) while this session's rebase was in flight. Both files
stand side-by-side as the two legs' records of one defect; main carries
their fixture bytes + guard. This file's unique contribution is the
regression-point proof (worktree bisect: parent green / landing SEGV) in
`evidence/executions/s9-m053-abi-fixture-regression.md`.
