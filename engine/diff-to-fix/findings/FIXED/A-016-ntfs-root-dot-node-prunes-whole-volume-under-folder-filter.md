# ACUTE A-016 — the folder filter's path walk treats the NTFS volume-root "." node as a path segment: every prefix turns incompatible and the WHOLE volume prunes — a filtered NTFS scan returns zero results

**Severity:** ACUTE (wrong recovery results: an opt-in scan-time filter silently reduces the NTFS result set to nothing — the exact "user asks for a sub-tree, gets an empty tree" failure)
**Module:** orchestration (the M-073 scan-time folder filter) / ntfs tree shape
**Parameters affected:** S31 (scan lifecycle — the filter's tree constraint), A11-adjacent (node fields the filter consumes), the M-073 landing's own contract
**Corpus references:** the M-073 landing's corpus (UIUX/31:142 — "the filter condition is pushed INTO the scan controller, so pre-set filters also constrain incremental tree insertion"): the og's tree renders the volume's content DIRECTLY (`\folder1`), the root directory's `.` $FILE_NAME being a structural marker, never a rendered path segment — the og's results tree carries no `.\` prefixes anywhere in the corpus's real sessions (rsf-format--scan-session.md's path forms; the real G/H/I sessions' path column shapes); `.` is a forbidden user name on Windows (the object-namespace reserve list), so no legitimate content node can collide with the transparency rule
**Core references:** the M-073 landing on main (`5291076`-lineage): `src/core/orchestrator/cr_folderfilter.cpp` `prune_children` — every directory node contributes `node_path_key(c)` as a path segment with no `.` special case; the NTFS walk emits record 5's node with $FILE_NAME `.` and reparents it to the Found root (`src/core/ntfs/cr_mftwalk.cpp:1162-1166`; the shape pinned by `tests/unit/test_ntfs_volume.cpp:634` `attr_file_name(5, u".", ...)` and asserted at `:1704` `posix[0]->parent()->name_utf8() == "."`); the wiring prunes every non-exempt root through the walk (`ScanController::apply_folder_filter_`). Reproduction first-hand this session: the RED test below fails at `5291076` — `walk_mft` over the standard NTFS fixture, then `folderfilter::prune_root(tree, found_root, {"\\folder1"})`: `readme.txt`/`deleted.dat` ABSENT (the `.` node's path key `.` is compatible with no prefix → `count_subtree` + `detach_child` on the whole volume content).
**Status:** NEW-FINDING (discovered at the convergent-verification pass over the racing window's M-073 landing) — fixed this session

## Summary

The scan-time folder filter (M-073, landed by the racing window this session) walks the tree accumulating path segments from every directory node's name. The NTFS engine's volume-root node — MFT record 5, $FILE_NAME `.`, reparented to the Found root — contributes a `.` segment, so every real content path becomes `./folder1/...` while the caller's prefixes normalize to `folder1/...`: `folder_path_compatible` fails, the `.` node (a directory) is detached WITH its entire subtree, and a filtered NTFS scan returns an EMPTY tree (stats.files_found collapses toward 0 through the same prune). The FAT family is unaffected (the FAT engines attach root-level entries directly to the category root — no `.` node exists), which is exactly why the landing's FAT-fixture test battery stayed green and the defect shipped.

## What the reference does

The og's filter path (`FilterPath=`, UIUX/31:142) addresses volume content by real path; its rendered tree never carries a `.` segment (the corpus's real sessions show `\`-rooted content paths directly). The root directory's `.` is the structural self-reference of the NTFS namespace, not a user-addressable folder.

## What our core does (at the defect)

`prune_children` evaluates EVERY directory node as one path segment — including the `.` root node. `folder_path_compatible(".", "folder1")` is false on both sides; the node detaches with the whole volume. Falsifiable: the RED test (below) at `5291076` — a `\folder1` filter over the standard NTFS fixture prunes `readme.txt` and `deleted.dat`; post-fix it keeps them and prunes only the out-of-scope `$`-meta children of `.`.

## Impact

Any host using the new `filter_folders` surface on an NTFS volume (the dominant Windows filesystem — the primary use case of a "scan only under \Users\alice" filter) gets an empty result tree and near-zero counters, silently. The scan completes "successfully".

## Reproduction / verification

RED first-hand: `NtfsVolume.FolderFilterRootDotIsTransparent` at `5291076` — walk_mft over the standard fixture, `folderfilter::prune_root(tree, ra, {"\\folder1"})`, assert `readme.txt`/`deleted.dat` survive: FAILED (kept_in_scope 0). Post-fix: GREEN with the `$`-meta children pruned (the correct scoped prune) and `ps.files < stats.emitted` (a partial prune, never the whole volume).

## Fix plan

1. `prune_children`: a directory node whose key is exactly `.` is TRANSPARENT — recurse with the PARENT's accumulated path (its children evaluate against `dir` unchanged); it never contributes a segment and never detaches. `.` cannot be a user name on Windows, so the transparency is total for directories.
2. Acceptance: the RED test above + the existing FAT fixtures stay green (their trees carry no `.` node, so the special case is inert there — the behavior-unchanged pin).

## Tests to add

- `NtfsVolume.FolderFilterRootDotIsTransparent` (the engine-shaped end-to-end over the standard NTFS fixture: the exact `apply_folder_filter_` seam).

## Related findings

- **M-073** (FIXED, the parent surface this defect rode) — the racing window's landing; this finding is the convergent-verification catch at the seat.
- **N-218** (the same verification pass's infra catch: the detach live-counter over-subtraction).

## Open questions

None — the corpus's real-session path forms corroborate the transparent-`.` semantics; a `.`-named FILE cannot exist on any supported filesystem.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S79 fix lane, 2026-10-29)
**Verified by:** `NtfsVolume.FolderFilterRootDotIsTransparent` (RED first-hand at `5291076`: walk_mft over the standard NTFS fixture + `folderfilter::prune_root(tree, ra, {"\\folder1"})` — readme.txt/deleted.dat ABSENT, kept_in_scope 0; GREEN post-fix: both present through the `.` node, the `$`-meta children of `.` correctly pruned as out-of-scope, `ps.files < stats.emitted` proving a partial prune). The landing's 13 FolderFilter/ScanTimeFolderFilter tests stay green (the FAT fixtures carry no `.` node — the transparency is inert there, the behavior-unchanged pin).
**Resolution notes:** CONFIRMED-DIVERGENCE (the M-073 landing's own corpus ground re-read at the fix: UIUX/31:142's rendered-path forms — the og's tree addresses volume content directly, the `.` $FILE_NAME is a structural marker; `.` is a forbidden user name on Windows, so the transparency rule cannot misfire on legitimate content). The fix is one surgical rule in `prune_children` (cr_folderfilter.cpp): a directory node whose key is exactly `.` recurses with the PARENT's accumulated path — it never contributes a segment, never evaluates its own compatibility, never detaches. Its children are then evaluated individually against the caller's prefixes (the in-scope subtree keeps, the out-of-scope `$`-meta siblings prune — the correct scoped behavior the landing intended). No other walk in the codebase consumes `.`-shaped nodes pathologically (the query walk materializes paths for ROWS only, and rows exclude the structural `.` node by the category filter's is_full/props gate — verified by read). Discovered at the convergent-verification pass over the racing window's M-073 seat (the verifier role the session mandate assigns this lane); announced in the commit message.
