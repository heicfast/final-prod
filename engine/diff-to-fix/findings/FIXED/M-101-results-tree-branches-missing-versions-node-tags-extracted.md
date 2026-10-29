# MAJOR M-101 — the results tree lacks two og branches: the "Versions" node (NTFS multi-version file-record siblings) and "Tags Extracted" (EXIF/tag grouping) — core's tree has no concept for either (5 og roots vs 3)

**Severity:** MAJOR
**Module / Surface:** tree / query (modules/query.md, modules/ntfs.md-adjacent)
**Parameters affected:** A11-adjacent (node fields/branches), A16-adjacent (the branches are filter-bar targets), A42-adjacent (the tree-shape the host renders)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md:27-36` (the **5 distinguished roots**: Tags Extracted (EXIF/tag grouping), File Name Lost (raw-carved by content), File Path Lost (carved w/o reconstructable path), **Versions node (NTFS file-record multi-version)**, Unimportant (the collapsed junk bucket)); the R-20 row (docs/REFERENCE-API-GAP.md:214: "Missing: 'Versions node' (NTFS multi-version siblings) and 'Tags Extracted' (EXIF tag grouping) — no tree concept for either").
**Core references:** audited commit `ff879f9` — The landed halves: Origin/DeletedState enums + raw_category_name + the M-018 junk filter ("Unimportant" ≈ is_junk_name; "File Name/Path Lost" ≈ raw-category + Origin). **No Versions concept** (the NTFS walk deduplicates multi-version record siblings rather than grouping them) and **no Tags Extracted grouping** (EXIF/tag extraction exists for preview/thumbnail but not as a tree branch).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** recorded as R-20 PARTIAL — never formalized. The three landed branches trace to M-018/M-029; the two missing ones had no finding ID.

## Summary

The og's results tree distinguishes five special roots; core implements three (junk/Unimportant, Name-Lost, Path-Lost). The "Versions" branch groups NTFS multi-version file records (the same file's older MFT versions surfaced as siblings — torn-write and overwritten-file recovery gold), and "Tags Extracted" groups files by their EXIF/tag payload. Both are user-visible tree shapes the og renders and core cannot express.

## What the reference does

The five roots are decoded at UIUX/09:27-36 — the Versions node carries the NTFS file-record multi-version family (the og surfaces prior record versions of the same pathname), and Tags Extracted is the EXIF/tag-grouping root the filter bar targets.

## What our core does

No node field, category, or branch encodes multi-version grouping — the NTFS walk's duplicate-record handling (clone fan-out aside) does not preserve the "versions of one file" relation as a queryable branch; tag extraction exists only in the preview/thumbnail paths, not as tree structure.

## Gap

Falsifiable: a volume with two MFT records for the same $FILE_NAME (a torn overwrite) renders a Versions node in the og; core's tree renders them as unlinked siblings with no grouping axis. No query returns a tags-extracted grouping.

## Impact

Recovery value: the Versions branch is how the og exposes recoverable PRIOR versions of overwritten files (a headline deep-scan capability); Tags Extracted drives the photo recovery UX. Both are host-renderable only if the engine carries the concept.

## Reproduction / verification

Corpus documentation evidence (UIUX/09:27-36 + the R-20 row); core state verified this session (no version-grouping field in cr_node.h; no tag-grouping category in the query model).

## Fix plan (for the engineering team)

1. Versions: add a version-group id to FileProperties (the NTFS walk already sees sibling records for one $FILE_NAME — group them under a synthetic parent or a sibling-set id; persist as an nflags/record extension per the session-wire discipline). 2. Expose in query: a Versions grouping flag or the group id as a filter axis (composes with M-072). 3. Tags Extracted: group by the extracted EXIF presence (the thumbnail/preview tag reader is the source) — a query-time grouping over an extracted-at-scan flag (defer full tag persistence unless the corpus shows the og persisting tags in the session — it does not; the branch is runtime grouping). 4. ABI: the snapshot_dfs enumeration carries the branch ids (append-only).

## Tests to add

- `Ntfs.VersionsNodeGroupsTornSiblings` — two records, one pathname → one group.
- `Query.TagsExtractedGrouping` — EXIF-bearing files group under the branch.
- `Session.VersionsRoundTrip` — the group id persists (the nflags extension).
- `Snapshot.ExistingBranchesUnchanged` — the three landed branches keep their shapes.

## Related findings

M-072 (the filter axes that target these branches); R-20 (the recorded row); M-018/M-029 (the landed three branches).

## Open questions

Whether the og's Versions node also appears for non-NTFS engines (the corpus ties it to NTFS record multi-version — treated as NTFS-scoped here; one UIUX decode question if hosts want it wider).

---

## Session-89 S89 fix-lane addendum (2026-10-30) — the finding is FIXED; the classification trail

**Classification: CONFIRMED-DIVERGENCE.** The corpus evidence re-verified
first-hand this session (not from the finding text): UIUX/09:27-36 read in
full (the five distinguished roots with their coaching copy); the
branch-type tooltip decode `FUN_140aeaec0` (DRWUI_decompiled.c :1678423-
1678519, read this session): `QModelIndex::data` → `QVariant::toInt` —
0 = Tags Extracted, 1 = File Name Lost, 2 = File Path Lost, 3 = Versions,
4 = Unimportant; the breadcrumb builder `FUN_1403c6fc0` (:642755-642822:
`local_58 == 6` → "Tags Extracted", `local_58 == 7` → "Reconstructed",
`local_58 == 4` → "Existing Files"); the rsf session grammar grep — no
version/tag field persisted anywhere (SaveScan/rsf-format + rsf-record4:
grep -i version/tag → only the format-version dword and the
type_flags_again serialization artifact). Core side re-verified at
`fc4ffdf4` (line-cites current): no version-group field in
FileProperties, no tag mark on FileNode, no branch axis in
query::Filter, no branch fields on SnapNode/cr_ui_node_info/cr_ui_query
— the divergence stood exactly as filed.

**The landed fix (engine carries the two branch concepts; the UI split
stays the og's — the corpus derives both branches at model-build time
from the live tree, so the core's parity surface is the queryable
relation, not a UI):**

1. **Versions** — `FileProperties::version_group` (u32; 0 = no sibling),
   stamped by the NTFS walk's new phase F (cr_mftwalk.cpp, after the
   S-012 rescue): every emitted node whose (parent record, name) key
   collides with another record's carries the same group id
   (deterministic, emission-order, 1-based). Hard links never collide
   on the key (one record's links carry distinct parents), so the groups
   are exactly the torn-overwrite record families. NTFS-scoped per the
   finding's own open question (the corpus ties the branch to NTFS
   record multi-version). Persisted as nflags bit-14 (4-byte append-only
   block; fires only when nonzero, pre-M-101 sessions shape-identical).
2. **Tags Extracted** — `FileNode`'s runtime-only `tags_` bit (the
   graded_/failed_ discipline: nothing persisted — the og persists no
   tag field; a reloaded session re-derives on re-read). Stamped by the
   ABI preview tag reader: `preview_open_session` binds a liveness watch
   (the handle stays session-independent — the private props copy is the
   content source; the watch removes the use-after-free class for
   close-then-call), and `preview_metadata` marks the live node when the
   extraction yields EXIF tags (orientation/datetime non-empty — the
   reader's tag set; snapshot previews carry no live node, the honest
   posture).
3. **Exposure** — query::Filter gains `versions_only` /
   `tags_extracted_only` (zero-init = off, compose with the M-072 axes);
   Row gains `version_group` / `tags_extracted`; SnapNode gains both
   (append-only; `materialize_props` carries the group); the ABI twins:
   `cr_ui_query.versions_only` / `tags_extracted_only`,
   `cr_ui_query_row.version_group` / `tags_extracted`,
   `cr_ui_node_info.version_group` / `tags_extracted` (the N-090
   tail-append pattern, no version bump; zero-init = safe default).

**Tests (fail-before RED first-hand via the block-neutralize dance —
all three behavioral blocks removed → 3/3 RED; restored → GREEN; full
suite 1697/1697, the suite grew 1692→1697):**
- `NtfsVolume.VersionsNodeGroupsTornSiblings` — the finding's named test:
  live + deleted-prior "doc.txt" under root share ONE group; a same-name
  record under folder1 does NOT join; unrelated names stay 0; the torn
  siblings sit in the same directory node.
- `Query.M101VersionsAndTagsBranchAxes` — the axes select only group
  members / only tag-marked nodes; rows carry both; composition with the
  state mask; zero-init = all rows.
- `Crsf.VersionsRoundTrip` — the group id survives the session wire
  (nflags bit-14); the tag mark un-marks on reload (runtime-only, the
  og's posture).
- `Snapshot.M101BranchAxesAndExistingBranchesUnchanged` — the finding's
  named test: the new axes ride the snapshot rows AND the three landed
  branches keep their exact shapes (junk flag / Origin::Carved /
  Origin::Lost); `materialize_props` carries the group.
- `CrUi.M101TagsExtractedMarkAndBranchAxes` — the full ABI path: a real
  EXIF read (Orientation 6 from a crafted APP1) marks the live session
  node; the query axis + row fields + snapshot node_info observe it; the
  non-EXIF neighbor stays unmarked; session close invalidates the mark
  path while the preview's private copy still serves metadata; the
  versions axis maps and zero-inits safely on a FAT session.
- `tests/ui_abi_check.c` — plain-C visibility of every appended field
  and both axes (the A-ABI-3 class swept: the C consumer compiles and
  reads them; no export-count change — fields only, the N-090 pattern).

**The finding's open questions, closed:** (1) NTFS-scoped — the corpus
ties the Versions node to NTFS record multi-version (UIUX/09:34); no
other engine stamps groups. (2) Tag persistence — the corpus shows the
og persisting NOTHING (rsf grep clean); the branch is runtime grouping,
exactly as the fix plan's step 3 recorded.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push, the S89 fix lane)
**Verified by:** RED first-hand via the block-neutralize dance (phase F
removed → NtfsVolume.VersionsNodeGroupsTornSiblings FAILED; the preview
mark block removed → CrUi.M101TagsExtractedMarkAndBranchAxes FAILED; the
nflags bit-14 blocks removed → Crsf.VersionsRoundTrip FAILED; all three
restored → GREEN) + full suite 1697/1697 + ci-local.sh GREEN (format,
dead-surface, conflict-marker, warnings-as-errors build, ctest, report
schema, Win7 PE gate self-test, vcpkg, doc links, secrets, commit
convention) + ui_abi_check.c extended (plain-C compile + drive of every
appended field and both axes).
**Resolution notes:** Both missing branches land as engine-carried,
host-rendered concepts — the og's own split (its UI derives both at
model-build time from the live tree; the rsf grammar persists neither).
Versions: the (parent record, name) collision key matches the og's
"another version of the file in the same directory" frame exactly —
hard links never collide, so the groups are the torn-overwrite families.
Tags Extracted: the preview tag reader is the source (the fix plan's
recorded seam), runtime-only by design; the mark rides the node like
graded/failed. No safety hardening was adjacent (the finding touches no
consent/overlap surface); no existing behavior changed — the three
landed branches and every existing query axis keep their shapes (pinned
by Snapshot.M101BranchAxesAndExistingBranchesUnchanged and the unchanged
M-072/M-029 pins in the full suite). Corpus citations: UIUX/09:27-36,
DRWUI FUN_140aeaec0 :1678423-1678519 (branch-type ids), FUN_1403c6fc0
:642755-642822 (breadcrumb node-kind ids), the SaveScan grammar's
no-tag/no-version persistence (grep this session).
