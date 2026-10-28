# S42 — the from-scratch build at HEAD (the W4 executed verification): cr_core/cr_ui clean, cr_ui_tests 63/63 green, cr_tests BLOCKED by committed conflict markers (M-120)

**Lane:** S42 (session-42 continuation audit) · **Tree:** origin/main through `0bd04cf9` (the lane tip; the code state identical to `869da779` + the parallel window's product commits — S-048/S-054/N-115-repin included) · **When:** 2026-10-29.

## Environment

- Linux container, gcc 14 (`/usr/bin/g++` → g++-14), make, cmake 4.4.4 (pip-installed this session — the sandbox had none; no other toolchain changes).
- Preset: `linux-release` (`CMAKE_BUILD_TYPE=Release`, ASan OFF).
- GTest: fetch-content v1.15.2 (the CMakeLists' own pinned URL) — resolved and built.

## Result matrix

| Target | Outcome |
|---|---|
| `cr_core` (the product library) | **BUILT 100% CLEAN** |
| `cr_ui` + `cr_ui_abi_check` | **BUILT CLEAN** |
| `cr_ui_tests` (63 tests, executed) | **63/63 PASSED** (21.93 s) |
| `cr_tests` (the main suite) | **CANNOT COMPILE** — committed git conflict markers at `tests/unit/test_carve.cpp:8588-8600` (→ **M-120**) |

## The cr_tests compile failure (verbatim, gcc 14, -Werror)

```
/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8600:280: error: invalid digit "9" in octal constant
/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8600:302: error: invalid digit "9" in octal constant
/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8600:343: error: multi-character literal with 37 characters exceeds 'int' size of 4 bytes [-Werror]
/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8588:1: error: version control conflict marker in file
/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8590:25: error: 'tail' was not declared in this scope
```

The markers are COMMITTED (the working tree is clean; `git show origin/main:tests/unit/test_carve.cpp` carries them; introduced by `1fb7603b`). Full detail + fix plan: `findings/MAJOR/M-120-committed-conflict-markers-red-main.md`.

## The executed cr_ui_tests run (63/63)

The full pass list (every test OK): AbiVersionAndLastError, CarveFormatRegistryEnumeration, ScanLifecycleOverImage, CancelSaveAndResume, AutoSaveOnCompletion, AutoSaveSkipsEmptyResults, AutoSaveSkipsAfterExplicitSave, AutoSaveSkipsResumedRun, SessionListTwoCallPatternAndRowTruth, SessionLoadProgressEventsAndCancelFlag, SessionListUnreadableFileGetsAnErrorRow, SessionListOgTimeOrdering, SessionListTieBreakDeterministic, EnumerationRefreshContractPinned, SessionListCliAndAbiOrderAgree, PauseBlocksAndResumes, ScanBusyStoppingQueries, SnapshotEnumeration, QueryFiltersSortPaging, PreviewSessionAndSnapshot, RecoverySubtreeAndRename, RecoverAllSkipsJunkFlaggedNodes, RecoverAllSnapshotWalkSkipsJunkFlaggedNodes, RecoverExplicitSelectionStillAllowsJunk, ErrorPathsAndPlatformTruth, TypedEventStreamFullLifecycle, EventStreamUnsubscribeAndResubscribe, HeartbeatDuringStall, SaveProgressEventsAndAtomicCancel, EventThreadContractPinned, SessionSaveWritesCrsfOnly, AbiVersionAnnotationsAreTruthful, DiskMapSessionGeometryAndOverlays, DiskMapSnapshotRequiresViewGeometry, DiskMapFreeOverlayFromFatTable, DiskMapFreeCellsSurviveSessionReloadAndSnapshot, DiskMapScanSnapshotCarriesFreeMap, PreScanRepairConsentGateAndUndoLog, PreScanRepairDeniedLeavesFileUntouched, ImageSourceDestinationNearImageAccepted, CheckpointAtomicStagingAndCommit, CheckpointPreviousGoodSurvivesTornEmit, LostApplyConsentGateAndUndo, LostApplyRefusesOverlapAndSlices, QueryApplyBucketsFillsAndRejects, RawCategoryNameNamesTheCarveRoot, UserSignRegistrationSurface, UserSignAbiCarriesTheFullS039Widths, TransportErrorAdvisoryZeroOnCleanState, AbiHeaderNamesNoPhantomErrorMacros, TransportDocCarriesNoDirectValue, AbiContractCarriesDriverEraTruth, DriverWireCodesFreeOfPhantomZeroInDocs, CliHelpListsEveryDispatchedVerb, ThumbnailTwoCallPattern, ThumbnailTruncationDisclosed, ThumbnailUnsupportedFormat, ThumbnailSuccessPromotesGradeTo2, CliScanFillsAutoSaveVolumeLabel, ThumbnailCapsAllowEmbeddedValidated, ThumbnailHeaderMatchesContract + **DestArming.ProductionWiringCallsSpansApi, DestArming.HeaderContractStillClaimsAllSpans** (the S-054 fence pair — the N-013 regression fix's fences verified green audit-side, convergent with the `a3741812` landing's own record).

`[  PASSED  ] 63 tests.` — the tail verdict verbatim.

## What this verifies (the audit-side value)

1. **The parallel fix window's product claims hold where executable:** the S-054 fences are green first-hand; the ABI surface's contract tests (AbiVersionAnnotationsAreTruthful, AbiHeaderNamesNoPhantomErrorMacros, DriverWireCodesFreeOfPhantomZeroInDocs — the doc-truth test class) all pass at HEAD.
2. **The main suite is NOT executable at HEAD** — the trail's post-`1fb7603b` green claims are unverifiable until M-120's one-line resolution lands (the 43-minors lane's 1203/1203 was honestly executed at the pre-red `632518b7`).
3. The product library and the ABI layer are unaffected by the red — the defect is confined to the test source file.

---

## Post-repair re-run (the M-120 closure, same lane, tree `7585a52a` with `b00118b5` folded in)

The parallel repair lane landed `b00118b5` (the conflict-marker residue removed, the
both-sides-folded case-insensitive typed-extension compare kept) within the same window as
M-120's filing. This lane's verification at the post-repair tree:

- `make -j$(nproc)` → **cr_tests BUILDS** (the conflict-marker errors gone).
- `./tests/cr_tests` → **[  PASSED  ] 1104 tests.** / **85 SKIPPED** (the Win-only lane:
  ExifOrientation parameterization, ThumbnailTiff variants, the other Win-gated suites) /
  **0 FAILED**.

The full suite is green, first-hand, at the current origin/main. M-120 retired FIXED
(convergent) — the resolution detail (the folded-compare choice vs the introducing
message's direct-compare declaration) recorded in the finding's Resolution.
