# SEVERE S-001 — Fully-overwritten deleted files are emitted; the reference destroys them

**Severity:** SEVERE
**Module:** ntfs
**Parameters affected:** B1 (detection correctness — result-set membership), B4 (deleted-file classification), D15 (confidence grading model)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1; decompiled `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_18001acc0, lines ~21128–21300 (destroy branch ~:21250, agent-B spot-verified)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:649–691` (grading), `:674` (Overwritten), `:808–812` (emission + DeletedState::PartiallyOverwritten), `include/cr/tree/cr_node.h:39–46` (Confidence enum) — audited commit `67193b6`
**Status:** CLOSED — FIXED in `e93decf` (verified independently by the session-2 audit: destroy rule + FUN_18001acc0 :21249-21262 cite re-read first-hand, suite 288/288; see Resolution)

## Summary

When a deleted file's every cluster has been re-allocated, the reference engine **drops the file
from the result set entirely** (`props.Release()` without adding to the controller's list). Our
core instead **emits the node**, graded `Confidence::Overwritten` and
`DeletedState::PartiallyOverwritten`. The observable result sets therefore differ on every volume
that contains fully-overwritten deleted files: ours is a superset.

## What the reference does

The deleted-record loop `FUN_18001acc0` (NTFSSearchDLL) computes, per deleted file that passes
the `0x3ff < size` viability gate, a per-cluster allocation test against the volume bitmap
(`FUN_18002c2e0`, controller+0x40 with fallback controller+0x48, plus a secondary map — see
tracked-open OQ-T1). The decompiled decision is:

```
if (total > 0 && intact == total) → props.Release() — DESTROYED
```

(agent-B context sheet B-ntfs.md §3.1, quoting `NTFSSearchDLL_decompiled.c` ~:21250; the
`Release()` path skips the list-insert at controller+0x18 that all other records take at
LAB_18001afe6). The artifact `ntfs-engine-algorithms-v2--deleted-viability.md` §1 documents the
same semantics: destroy only when every cluster is re-allocated; partially-intact files are kept.
A fully-destroyed deleted file never reaches the tree, the .rsf session, or the UI.

## What our core does

`cr_mftwalk.cpp:649–691` grades instead of destroying:

```cpp
if (probe_size > flags.viability_min_size && probe.total_clusters() > 0) {
    if (bitmap && bitmap->valid()) {
        const double ratio = bitmap->reused_ratio(probe);
        if (ratio >= 1.0)
            conf = tree::Confidence::Overwritten;
        else if (ratio > 0.0)
            conf = tree::Confidence::PartialRisk;
        else
            conf = tree::Confidence::Excellent;
    ...
```

and `:808–812` still emits:

```cpp
if (!in_use) {
    n->set_deleted_state(tree::DeletedState::Deleted);
    if (conf == tree::Confidence::Overwritten)
        n->set_deleted_state(tree::DeletedState::PartiallyOverwritten);
}
...
emitted.push_back({n, p, false});
++stats.emitted;
```

No comment on this path documents the deviation from the og's destroy rule (the adjacent
comments document other deliberate choices, e.g. the metafile filter at `:585`).

## Gap

Falsifiable claim: for a deleted NTFS file whose $Bitmap probe finds all clusters re-allocated,
the og result set contains no entry and the core result set contains an entry graded
Overwritten / PartiallyOverwritten.

## Impact

- **Recovery correctness:** no wrong bytes are written (the grade is honest), but users are
  offered files that can never produce content — recovering them yields zeros.
- **Result-set parity:** every comparative benchmark inventory (recall/precision vs og) counts
  these as extra candidates; precision metrics diverge mechanically.
- **UI contract:** the reference UI never shows such entries; downstream consumers (filtering,
  "Recovery Chances" gauge, counts per category) see a superset.
- **Session size:** .crsf files carry nodes the reference would never have persisted.

## Reproduction / verification

- Corpus-derived (strength 2/3): decompiled quote above with file+line; artifact §1 cross-cite.
- Core-derived: code quotes with file:line at audited commit `67193b6`.
- Executed comparison: not feasible in this environment (no Windows/og run); the divergence is
  fully determined by the two code paths above, both quoted verbatim.

## Fix plan (for the engineering team)

1. Decide the product semantic (two options, pick one and register it):
   a. **Strict parity:** drop fully-overwritten deleted files at walk time (add a
      `destroy_fully_overwritten` walk flag, default on for og-parity scans).
   b. **Keep-and-grade (current):** register as a reviewed deviation in
      `docs/parity/PARITY-DELTAS.md` with rationale, and exclude
      `Confidence::Overwritten` nodes from og-comparison inventories in bench metrics.
2. If (a): modify `src/core/ntfs/cr_mftwalk.cpp` after the grading block — when
   `ratio >= 1.0` and the node is deleted, skip emission (mirror the og's Release path);
   keep a `stats.destroyed_overwritten` counter for telemetry parity with the og's counters.
3. If (b): update `docs/CAPABILITY-MATRIX.md` and bench `compare.py` to filter
   `PartiallyOverwritten` from the og-side comparison set.
4. Either way: add the decision to `docs/ground-truth-contracts.md` §deleted-classification.

## Tests to add

- `test_ntfs_volume.cpp::Overwritten_Dropped_From_Results` (option a): deleted file with all
  clusters marked allocated in $Bitmap → walk emits 0 nodes for that record; fails before fix,
  passes after.
- `test_ntfs_volume.cpp::Overwritten_Graded_And_Flagged` (option b): same fixture → node present
  with `Confidence::Overwritten`, `DeletedState::PartiallyOverwritten`, and the bench filter
  excludes it from og comparisons.
- Bench: re-run `permanent_delete` scenario; expect precision to move toward the og number
  under (a).

## Related findings

- OQ-T1 (tracked-open): secondary claim map — could shift which files reach the destroy branch.
- S-014 (summary counts): bench `MetadataOnly zero-fill FPs` interacts with the same honesty
  cap family.

## Open questions

None beyond OQ-T1 (tracked separately).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: S-001`)
**Verified by:** runtime fail-before/pass-after — with only the `cr_bitmap.cpp`
sparse-run change stashed, `NtfsVolume.EndToEndWalk` fails at the
`stats.destroyed_overwritten == 1` and `holes.dat != nullptr` assertions
(the old ratio counted sparse runs as re-used, graded hole-only files
Overwritten, and the destroy rule then dropped them); restored, it passes.
The destroy rule's fail-before state is the PREVIOUS pinned expectation of
the same test (`overwrit->confidence() == Overwritten` + node present —
the exact keep-and-grade divergence this finding describes, still in git
history at the audit baseline) plus the first-hand decompile read
(FUN_18001acc0 :21249-21262: `0 < total && intact == total -> props
Release()`, skipping the list insert at LAB_18001afe6). Updated/new tests:
`NtfsVolume.EndToEndWalk` (overwrit.dat destroyed + counter + folder kept +
hole-only file kept Excellent), `NtfsVolume.OverwrittenHonestModeKeepsAndGrades`
(the flag-off mode pins the old keep-and-grade behavior). Suite 282/282
(Linux ASan).
**Resolution notes:** implemented option (a) strict parity per the fix plan:
`WalkerFlags::destroy_fully_overwritten` (default ON = og destroy rule;
OFF preserves the previous keep-and-grade visibility for honest-recovery
scans — the reviewed-deviation mode is one flag away, not deleted). Folders
are never destroyed (the og's `(flags & 0x10) == 0` gate — verified :21190);
live files are unaffected (they never take the viability path); recycle-bin
$R records are in-use records and cannot hit the rule. Two sub-bugs found
during verification and fixed in the same commit: (1) `VolumeBitmap::
reused_ratio` counted SPARSE runs fully into both counters, while the og's
viability iterator skips `-1` entries (:21220) — sparse runs are excluded
now (an all-sparse deleted file is Excellent, not Overwritten — and was
about to be destroyable on bogus accounting); (2) a `stats.destroyed_
overwritten` counter was added (telemetry parity). The viability map source
difference (og: scan-built claimed-sector set, ctor FUN_18002c330, bits set
by FUN_180017190 at :18409; core: volume $Bitmap) is equivalent on healthy
volumes and only diverges on MFT-damaged media (og's claimed-set is smaller
→ og keeps files the $Bitmap would mark re-used) — recorded in the OQ-T1
tracked-open write-up, not a behavioral fix target this session. The
secondary claim map (OQ-T1) is RESOLVED: same-direction (re-allocated) but
ctor-NULLed and never assigned anywhere in NTFSSearchDLL — the secondary
test never fires in the og build; our single-map grading is not missing
any og input. Downstream: bench comparisons should re-run `permanent_delete`
(the finding's own note) — the og-side inventory no longer needs the
PartiallyOverwritten exclusion under default flags.
