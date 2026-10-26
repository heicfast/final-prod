# MINOR N-019 — Missing per-parameter regression tests: 13 specific edge-case scenarios not pinned

**Severity:** MINOR
**Module:** quality (cross-cutting, per-engine)
**Parameters affected:** Q65 (test coverage — every parameter above has a test)
**Corpus references:** `diff-to-fix/PARAMETERS.md:132` (Q65 definition: "Test coverage — every parameter above has a test")
**Core references:** `tests/unit/` (26 test files, 287+ TEST() cases); grep across them for the 13 specific scenarios below returns 0 hits each — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Q65's definition ("every parameter above has a test") is partially
met. The core has 26 unit-test files with 287+ TEST() cases covering
the happy path of every engine. But 13 specific edge-case scenarios
the audit prompt identified as critical for parity verification have
NO isolated test pinning them. Some have indirect coverage (the
engine module tests exercise the happy path), but the specific
edge-case behavior is not pinned — meaning a regression could land
without a test failing.

## What the reference does

The corpus's Q65 definition is aspirational; the og has its own
internal test suite (not in the corpus) and the corpus doesn't
document the og's per-parameter test coverage. This is a
quality-parameter finding, not an og-parity finding.

## What our core does

| # | Scenario | Test exists? | Evidence |
|---|---|---|---|
| 1 | hard-link destroy behavior | NO | S-001 fix added `destroy_fully_overwritten` flag but no isolated hard-link test in `test_ntfs_volume.cpp` (the `EndToEndWalk` test exercises a hard-link record but doesn't pin the destroy-rule interaction with hard-links specifically) |
| 2 | ADS emission | NO | grep `alternate.*stream\|ADS` across `tests/unit/` returns no isolated test (the `ResidentAdsCarriesContent` test at `:894-933` tests resident ADS content capture, but not the emission boundary or the parity-mode drop) |
| 3 | sparse file accounting | NO | S-001 fix's sparse-run sub-fix has no isolated regression test (the `holes.dat` test in `EndToEndWalk` exercises a sparse-but-allocated file but doesn't pin the ratio exclusion) |
| 4 | FAT cross-linked chain | NO | grep `cross.link\|crosslink` across `tests/unit/` returns no test (a chain where two files share a cluster — the cycle guard should detect this) |
| 5 | exFAT multi-fork | N/A | exFAT has no fork concept (single-stream filesystem); not applicable |
| 6 | ext4 extent tree depth > 2 | NO | grep `extent.*tree.*depth\|depth.*3` returns no test (the engine handles arbitrary depth but the depth-3 path is untested) |
| 7 | HFS+ resource fork recovery | NO | S-002 finding about HFS+ attributes-tree-decmpfs was filed but the resource-fork path itself has no isolated test |
| 8 | Btrfs subvolume rooting | NO | grep `subvolume.*root\|Btrfs.*Subvol` returns no test (the engine handles subvolumes but the rooting path is untested) |
| 9 | ZFS snapshot | NO | OQ-T6 tracked-open; no test |
| 10 | ReFS integrity stream | NO | grep `integrity.*stream\|ReFS.*integrity` returns no test |
| 11 | LDM RAID-0 stripe alignment | NO | OQ-T7 tracked-open; no test |
| 12 | LDM RAID-1 mirror read | NO | OQ-T7 tracked-open; no test |
| 13 | lost-partition overlap scoring | NO | `cr_scan.cpp:920-925` `known` table feeds the Inside/Overlaps penalties; no test exercises them with overlapping candidates |

## Gap

12 of 13 applicable scenarios lack an isolated regression test (item 5
is N/A). The engine code paths exist and likely work; the gap is the
lack of pinned regression coverage.

## Impact

- **Regression risk:** a refactor of any of these paths could silently
  break the edge-case behavior without a test failing.
- **Parity verification:** without isolated tests, og-parity
  comparisons on these scenarios are ad-hoc (per-bench-run) rather
  than CI-pinned.
- **No current bug:** the engines likely work correctly today; the gap
  is the missing test discipline.

## Reproduction / verification

- **Core evidence:** file-by-file grep across `tests/unit/` confirms
  absence for each of the 12 applicable scenarios.
- **Test fixtures:** the existing `tests/unit/test_fixtures.hpp` makes
  most of these ~20-line additions per scenario.

## Fix plan (for the engineering team)

1. **Add one regression test per missing scenario** to the relevant
   engine test file. Each test should be ~20-50 lines, fixture-driven
   (using `test_fixtures.hpp`), and pin the specific edge-case
   behavior.
2. **Files to modify:**
   - `tests/unit/test_ntfs_volume.cpp` — add scenarios 1, 2, 3.
   - `tests/unit/test_fat_volume.cpp` — add scenario 4.
   - `tests/unit/test_ext4_volume.cpp` — add scenario 6.
   - `tests/unit/test_hfsplus_volume.cpp` — add scenario 7.
   - `tests/unit/test_btrfs_volume.cpp` — add scenario 8.
   - `tests/unit/test_zfs_volume.cpp` — add scenario 9.
   - `tests/unit/test_refs_volume.cpp` — add scenario 10.
   - `tests/unit/test_ldm.cpp` — add scenarios 11, 12.
   - `tests/unit/test_lostpart.cpp` — add scenario 13.
3. **Tests to add (one per scenario):**
   - `NtfsVolume.HardLinkDestroyRuleInteraction` — pin the destroy
     rule's interaction with hard-link fan-out (does destroying one
     link destroy the props shared with other links?).
   - `NtfsVolume.AdsEmissionBoundary` — pin the ADS emission parity
     flag's behavior (default ON, opt-in OFF).
   - `NtfsVolume.SparseRunRatioExclusion` — pin the sparse-run
     exclusion in `reused_ratio` (the S-001 sub-fix).
   - `FAT.CrossLinkedChainDetectedByCycleGuard` — fixture: two files
     sharing a cluster; assert cycle guard detects this and one file
     is graded lower (or both).
   - `Ext4.ExtentTreeDepth3` — fixture: a file with depth-3 extent
     tree (root → index → index → leaf); assert full content
     recovered.
   - `HfsPlus.ResourceForkRecovered` — fixture: a file with a
     resource fork; assert fork content recovered (if S-002 is fixed).
   - `Btrfs.SubvolumeRooting` — fixture: a Btrfs filesystem with 2
     subvolumes; assert files in each subvolume recover under the
     correct root.
   - `Zfs.SnapshotRecovered` — fixture: a ZFS pool with a snapshot;
     assert snapshot files recover (if OQ-T6 is resolved).
   - `Refs.IntegrityStreamRecovered` — fixture: a ReFS file with
     integrity stream; assert content recovered (the integrity
     checksums validated or skipped per policy).
   - `Ldm.Raid0StripeAlignment` — fixture: a 2-disk RAID-0 stripe;
     assert the stripe alignment is correct (each member reads the
     right sectors).
   - `Ldm.Raid1MirrorRead` — fixture: a 2-disk RAID-1 mirror; assert
     the read prefers the primary (or whichever side og prefers —
     OQ-T7).
   - `LostPartition.OverlapScoringPenalizesOverlap` — fixture: two
     lost-partition candidates overlapping by 50%; assert the
     Overlaps penalty fires and the lower-overlap candidate wins.
4. **Acceptance criteria:** the 12 new tests pass; the
   `tests/CMakeLists.txt` registers them.

## Tests to add

(See the 12 specific tests above.)

## Related findings

- **M-006** — fuzz CI disabled. Both are quality-parameter findings.
  N-019 is the per-parameter regression test gap; M-006 is the fuzz
  harness gap. They complement each other (regression tests pin
  known-good behavior; fuzz harnesses probe unknown-bad behavior).
- **OQ-T6 / OQ-T7** — the tracked-open items for ZFS snapshots and
  LDM RAID-5 rotation. Tests for those scenarios can't be written
  until the RE questions are resolved (the og's behavior isn't
  fully known).

## Open questions

- **Is there an audit-trail check that EVERY parameter has a test, or
  is Q65 aspirational?** The honest framing is "every CODEPATH has a
  test" not "every PARAMETER has a test" — and that's what core
  actually achieves (26 test files, 287+ tests). Filing Q65 as a real
  divergence reflects the audit-prompt's literal interpretation; the
  practical posture is better than the literal finding suggests.
  Engineering decision: enforce Q65 literally (add the 12 tests) or
  re-scope Q65 to "every codepath" (already met). The fix plan above
  implements the literal interpretation.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-019 commit — `git log` "fix(N-019)")
**Verified by:** 4 new regression tests, each fail-before-proven by executed MUTATION (the guarded behavior broken in the engine source → RED, reverted → green): `NtfsVolume.HardLinkDestroyRuleInteraction` (mutation: destroy rule disabled → 5 failures), `Ext4Engine.ExtentTreeDepth3` (mutation: depth cap at level 1 → 1 failure; plus the construction-time RED run first-hand: a root-depth mistake produced zero resolved extents — the exact silent-loss mode this test exists to catch), `Btrfs.TwoSubvolumesRootSeparately` (mutation: user-subvol gate dropped → 4 failures), `Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk` (mutation: unknown row breaks the attribute walk — the pre-S-020 first-match-break shape → 1 failure). Suite **753/753** linux-release.
**Resolution notes:** Full re-verification against the current tree (the finding was filed at `5d6da5f`, ~450 tests ago; every scenario re-checked before writing anything). The 13-scenario table resolves as follows — **6 ALREADY-FIXED by later landings**, each with its covering test(s) re-cited: (2) ADS emission boundary = `AdsDroppedInParityMode` + `AdsEmittedByDefaultSuperset` (N-011's pins); (3) sparse ratio exclusion = the all-sparse `holes.dat`→Excellent pin in EndToEndWalk ("sparse runs never degrade the grade") + `PartialRisk` at 1/2 reused (zfn.dat) + `ViabilityBoundaryExactFastPath`; (4) FAT cross-linked chain = `CrossLinkedFat1RejectedAndFat2Used` (M-015: two chains claiming one cluster, the og's wholesale claims-walk rejection) + `ChainCycleIsCut`; (7) HFS+ resource fork = `RecordBit29SelectsResourceFork` + `DecmpfsInlineAndRsrcContentRecovered`; (11) LDM RAID-0 stripe alignment = `StripedVolumeInterleave` (per-chunk column/row byte-exact reads across both members); (12) LDM RAID-1 mirror read = `MirroredReadFailover` + `MirroredBothCopiesFail`. **1 SUPERSEDED:** (13) lost-partition overlap scoring — the finding's premise ("cr_scan.cpp:920-925 known table feeds the Inside/Overlaps penalties") is outdated: M-017's landing replaced the penalty model with the og's content-driven scoring (weighted count per FS + per-FS bonus + ln(count)/ln(max) permille + the +500 table-match; containment-drop for Inside candidates), pinned by `LostScoring.MatchesLogRows`, `TableMatchBonus500`, `OverLargeDuplicateRescoredToFront`, `FileRichCandidateOutranksCleanBoot`, `OverlapClassification`. **1 N/A:** (5) exFAT multi-fork — single-stream filesystem, per the finding's own table. **1 BLOCKED-pending-OQ:** (9) ZFS snapshot — routed to OQ-T6 per the finding's own text; this session's re-verification ADDS evidence to OQ-T6 (corpus zfs-engine--deep-dive.md §3 capa digest: "ZFS per-dataset snapshots: the engine surfaces snapshot roots when present in the DSL tree"; core negative-space verified: no origin_obj/ds_snapnames chase in cr_zfs_engine.cpp) — the og-parity decode target is now precise. **4 tests written this session:** (1) `NtfsVolume.HardLinkDestroyRuleInteraction` — the destroy decision is per-RECORD upstream of the fan-out: a deleted hard-linked record with fully-reused runs is destroyed ONCE (parity mode: no node; fan-out mode: BOTH links gone, counter still 1 — the shared props released with the record, never per-link); partial reuse keeps it graded PartialRisk; honest mode keeps it visible in both emission modes. (6) `Ext4Engine.ExtentTreeDepth3` — root(depth 3)→index→index→leaf, kernel depth semantics (root depth = levels below root; the engine decrements per level), two disjoint data runs resolve to the exact two-extent shape with byte-exact content markers. (8) `Btrfs.TwoSubvolumesRootSeparately` — subvol 5 (the FS tree) + subvol 256 (first legal user id) on one filesystem: every file under its OWN root, no cross-subvol leakage either direction, the second subvol's file byte-exact. (10) honestly rescoped — the corpus is SILENT on ReFS integrity streams (0 hits across the refs artifacts, verified first-hand), so an "integrity-stream parity" test would freeze a guess; the real robustness property landed instead: `Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk` — the og's property-flag marker rows (0x108/0xf8, the surface an attributed/integrity-enabled file rides) and unknown-type rows are walked past without breaking the data-run resolution or tripping the granularity discard, data recovered byte-exact. Q65's literal interpretation is now satisfied for every scenario whose og behavior is corpus-pinned; the two that are not (9, 10) carry precise evidence instead of frozen guesses.
