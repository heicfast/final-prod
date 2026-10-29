# MINOR N-218 — the detach paths subtract FullNode intermediates from live_files_: the unsigned telemetry counter wraps (N-166's own "never negative" contract violated)

**Severity:** MINOR (a telemetry-counter corruption, not recovery data — but the wrap poisons the mid-stage progress numbers the host UI observes, and the class was N-166's exact contract)
**Module:** tree (ResultTree's detach paths)
**Parameters affected:** progress/telemetry counters (the N-166 surface), the M-073 folder-filter prune path that made mid-tree detaches load-bearing
**Corpus references:** none needed — the defect is internal-consistency (N-166's own contract text in `include/cr/tree/cr_node.h:487-495`: "every add_file ticks it... detach_subtree subtracts"); the live counter models the og's results-grid live tracking (the M-073/M-018 neighborhood)
**Core references:** `src/core/tree/cr_node.cpp` — `ResultTree::detach_child` (`live_files_.fetch_sub(dead.size())` — the M-073 landing's new primitive) and `ResultTree::detach_subtree` (the same shape at the roots-only call sites); `add_folder`/`add_root` NEVER tick the counter (grep clean — only `add_file` ticks, cr_node.cpp:213); the session-load path `deserialize_tree` RE-CREATES intermediate folders as FullNodes under the structured roots (cr_records.cpp `tree.add_folder`, the marker=2 records — S-048), so a resumed scan's tree legitimately contains FullNode intermediates under the prunable roots. Reproduction first-hand: `NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles` at `5291076` — a root + one FullNode folder + one file: `live_files()` reads 1, then `detach_child(root, folder)` leaves `live_files()` == 18446744073709551615 (the wrapped u64).
**Status:** NEW-FINDING (discovered at the convergent-verification pass over the racing window's M-073 landing) — fixed this session

## Summary

`live_files_` is ticked by `add_file` alone — `add_folder` and `add_root` never tick it (N-166's documented contract). Both detach paths subtracted the WHOLE subtree's node count, FullNodes included, so any detached subtree holding an intermediate folder over-subtracted and WRAPPED the unsigned atomic: mid-stage telemetry (`ScanController::refresh_live_counts_` samples) reports 18-quintillion-file counts until the next boundary stamp re-truths. The M-073 folder-filter prune made mid-tree detaches routine (a filtered scan detaches on every boundary), and a RESUMED session's tree carries the session-loaded marker=2 FullNode folders under the very roots the filter prunes — the wrap became reachable in the standard flow, not just in theory.

## What the reference does / what our core does

Internal-consistency defect (no corpus counterpart): the counter's contract says "detach subtracts what attach added" — the code subtracted more than was ever added.

## Impact

Mid-stage progress/telemetry corruption for filtered scans over session trees with intermediate folders; the boundary stamps (authoritative) stay correct, hence MINOR.

## Reproduction / verification

RED first-hand: `NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles` at `5291076` — `live_files()` == 18446744073709551615 after one detach. Post-fix: 0, node_count 1.

## Fix plan

1. Both detach paths (`detach_child`, `detach_subtree`) subtract ONLY non-full nodes (the ones `add_file` could have ticked).
2. Acceptance: the RED test above; `detach_subtree`'s existing callers (lost-candidate rejects) unaffected — their subtrees are FS content whose counts are unchanged by the fix.

## Tests to add

- `NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles`.

## Related findings

- **N-166** (the counter's own contract — the "clamps at zero" comment was aspirational; the real clamp is now the correct subtraction).
- **M-073 / A-016** (the same verification pass's context).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S79 fix lane, 2026-10-29)
**Verified by:** `NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles` (RED first-hand at `5291076`: live_files() == 18446744073709551615 after one detach of a {FullNode folder + file} subtree; GREEN post-fix: 0 with node_count 1). The full NodeTree/Detach suites green post-fix.
**Resolution notes:** CONFIRMED-DIVERGENCE (internal-consistency: N-166's documented add/remove contract — "every add_file ticks it... detach subtracts" — violated by subtracting FullNode intermediates that add_folder/add_root never ticked). Both detach paths now count only non-full nodes (`detach_child` — the M-073 landing's primitive, and `detach_subtree` — the same shape at its roots-only call sites, fixed identically in the same change since the carve skeleton's FullNode folders and the session-loaded marker=2 intermediates make the wrap reachable there too). The "clamps at zero" comment is gone (it was aspirational — the real clamp is the correct subtraction). Discovered at the convergent-verification pass over the racing window's M-073 seat; announced in the commit message.
