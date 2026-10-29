# MINOR N-219 — the M-120/N-210/N-217 class's FOURTH recurrence: `27bda40e` (the M-151 landing) pushed with live committed conflict markers in `test_ntfs_volume.cpp` — main failed `-Werror` again, and the landing's claimed test re-pins were never executed

**Severity:** MINOR (the class convention; the repair landed in the same session, before any consumer pulled the broken tree — but this recurrence is worse than N-217's: the push's commit message claims re-pins ("the MirrorCoversRunlistSize / MirrorResidentDataRejected re-pins above carry the same interplay") that were LOST in the uncommitted conflict — the tree never compiled, so the claims were never executed)
**Module:** audit-trail integrity + build (the M-120/N-210/N-217 class)
**Parameters affected:** Q68 (audit-trail truth), the M-120 regression surface, CI truth (the push again bypassed the N-210 marker gate)
**Corpus references:** none for the marker defect (a core-repo integrity defect); the repair's semantic half: the M-151 growth-probe landing itself (FUN_180007670, the racing lane's decode) — the three missed re-pins are growth-era consequences, corpus-consistent
**Core references:** found at core `27bda40e` (origin/main HEAD at this lane's pull): `tests/unit/test_ntfs_volume.cpp:5780/:5803/:5860/:5865/:5905/:5987` — two interleaved conflict blocks (the A-016 lane's `FolderFilterRootDotIsTransparent` test vs the M-151 lane's `TruncatedRunlistGrowsToValidRecords` + `MirrorFallbackUnaffectedByGrowthPass` tests, sharing a line-level middle chunk: both tests' bodies were interleaved as HEAD/inc1/shared/inc2/tail). Verified first-hand: `cmake --build` → error at the marker lines.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The M-151 lane's push rebased onto the A-016/N-218 lane's test-file changes, hit conflicts, and committed the unresolved interleaving — main failed `-Werror` for the fourth time in the class's history (M-120 `db228b75`, N-210 `f3da99f1`, N-217 `ed501caa`, now `27bda40e`). Worse: the push message claims mirror-test re-pins that the conflict had swallowed — the tree could not compile, so no claimed verification ever ran.

## The repair (executed this session)

1. The interleaved conflict was reconstructed programmatically: both tests' bodies recovered by stitching HEAD1+shared+HEAD2 (the A-016 test — identical to the `15f51121` parent's version) and INC1+shared+INC2 (the M-151 tests, appended complete with their helper namespace).
2. The build then exposed THREE missed growth-era re-pins (the M-151 landing's behavioral consequences, each corpus-consistent with the growth probe's decoded semantics):
   - `NonresidentAttrlistOnMft`: `max_record_no()` 20 → 23 — the growth annexed the physical tail's 3 validator-passing records past the extension's declared end (the fixture leaves them on disk; the og's exact growth population). Re-pinned with the note.
   - `MirrorCoversRunlistSize` / `MirrorResidentDataRejected`: the fixtures' un-zeroed physical hole let the growth annex build_image's physical records past run 0's end — silently bypassing the mirror-fallback path those tests exist to prove. Fixed with the M-151 lane's OWN fixture pattern (the zeroed hole, as their `MirrorFallbackUnaffectedByGrowthPass` uses): the mirror path stays testable, the original assertions hold unchanged.
3. Post-repair: the NTFS family 96/96; the full suite 1520 passed / 0 failed (85 pre-existing skips).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S80 parity fix lane, 2026-10-29)
**Verified by:** `cmake --build` clean at the post-repair tree; `rg -c '^<<<<<<< ' tests/ src/` → 0; the recovered tests green (`FolderFilterRootDotIsTransparent`, `TruncatedRunlistGrowsToValidRecords`, `MirrorFallbackUnaffectedByGrowthPass`); the three growth-era re-pins green (`NonresidentAttrlistOnMft`, `MirrorCoversRunlistSize`, `MirrorResidentDataRejected`); NTFS family 96/96; full suite 1520/0.
**Resolution notes:** The class protocol held again: semantic reconstruction for code (both racing lanes' tests preserved — the interleaving made manual resolution error-prone, so the pieces were extracted by marker positions and stitched), the growth-era consequences re-pinned per the M-151 decode's own semantics. The recurrence's root cause is unchanged since N-210 (a racing lane pushing without the ci-local marker gate) — now with the added hazard that unexecutable trees carry unverified claims in their commit messages. Recorded for the next audit's liveness battery.
