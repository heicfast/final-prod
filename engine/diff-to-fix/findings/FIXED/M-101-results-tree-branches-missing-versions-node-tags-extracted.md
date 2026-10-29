# MAJOR M-101 — the results tree lacks two og branches: the "Versions" node (NTFS multi-version file-record siblings) and "Tags Extracted" (EXIF/tag grouping) — core's tree has no concept for either (5 og roots vs 3)

**Severity:** MAJOR
**Module / Surface:** tree / query (modules/query.md, modules/ntfs.md-adjacent)
**Parameters affected:** A11-adjacent (node fields/branches), A16-adjacent (the branches are filter-bar targets), A42-adjacent (the tree-shape the host renders)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md:27-36` (the **5 distinguished roots**: Tags Extracted (EXIF/tag grouping), File Name Lost (raw-carved by content), File Path Lost (carved w/o reconstructable path), **Versions node (NTFS file-record multi-version)**, Unimportant (the collapsed junk bucket)); the R-20 row (docs/REFERENCE-API-GAP.md:214: "Missing: 'Versions node' (NTFS multi-version siblings) and 'Tags Extracted' (EXIF tag grouping) — no tree concept for either").
**Core references:** audited commit `ff879f9` — The landed halves: Origin/DeletedState enums + raw_category_name + the M-018 junk filter ("Unimportant" ≈ is_junk_name; "File Name/Path Lost" ≈ raw-category + Origin). **No Versions concept** (the NTFS walk deduplicates multi-version record siblings rather than grouping them) and **no Tags Extracted grouping** (EXIF/tag extraction exists for preview/thumbnail but not as a tree branch).
**Status:** FIXED (this session; the S84 fix lane)

---

## Session-93 S93 lane addendum (2026-10-30) — the concurrent-landing concession + the two riders this lane lands on top of the seat

**The concession (SA-9, earlier-push-wins):** this lane (opened at
`fc4ffdf4`, pushing as S93 after the racing window took S89-S92) landed
an independent M-101 fix concurrently: a u32 counter group id assigned
by a post-walk phase F over the (parent record, name) collision key, a
node-level runtime tags bit, `versions_only`/`tags_extracted_only` query
axes, and nflags bit-14 as a u32 block. The racing seat (above) holds:
its namespace-identity FNV-1a hash is the better carrier (identity at
emission, no second pass, deterministic per pathname), and its session
wire (u64 bit-14 block) is the pushed form. This lane's convergent
implementation is withdrawn at the merge; every duplicate surface was
removed (the u32 field, phase F, the duplicate query axes and Row
fields, the u32 write/read blocks, the four convergent-duplicate tests).

**The two riders this lane lands ON TOP of the seat (both unique, both
kept):**

1. **The Tags Extracted PRODUCER** — the seat's `tags_extracted` props
   flag had NO stamping path anywhere (its own comment: "stamped by the
   host's preview/metadata pass" - with no such pass in the code; grep
   clean at the seat's tree: zero setters). A carrier with no producer
   is the no-disabled-surface class. The rider: the ABI preview
   metadata call stamps the live node's props when the extraction
   produced tag data (the seat's own documented semantics: EXIF
   orientation/datetime, media duration/codec, dimensions), through a
   session-liveness watch so the preview handle stays session-independent
   (post-close: nothing marked, metadata still served from the private
   copy). Pinned by CrUi.M101TagsExtractedMarkAndBranchAxes.
2. **The ABI exposure** — the seat landed no ABI surface at all; the
   rider appends the branch axes to the C ABI (the N-090 tail pattern,
   zero-init = safe default): `cr_ui_node_info.version_group` (u64, the
   seat's hash) + `.tags_extracted`, `cr_ui_query.version_group` (0 = no
   filter) + `.tags_only`, `cr_ui_query_row.version_group` +
   `.tags_extracted` - plain-C driven in ui_abi_check.c (the A-ABI-3
   class swept). The merge repair on the racing window's own residue is
   recorded in the merge commit (the born-collided WPD test name and the
   format drift).

The seat's Resolution above stands unchanged; this addendum records the
concession and the riders only.
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

## Resolution

**Status:** FIXED
**Fixed in commit:** the code landed in `b7d05b12` (honest note: the lane's gate-repair commit swept the M-101 working tree in an `git add -A` mishap during the N-223 renumber window — the changes are the M-101 landing in full, verified below); THIS commit lands the trail (the finding Resolution, the matrix/ledger/SUMMARY/worklog records).
**Verified by:** the finding's own four-test battery, all green first-hand: `NtfsVolume.WalkStampsVersionGroup` (the walk-side falsification: a planted torn-overwrite sibling — a second MFT record with the same (parent 16, name readme.txt) — both records' nodes carry the SAME nonzero namespace-identity hash; distinct pathnames never collide) + `M101Branches.VersionsNodeGroupsTornSiblings` (the branch semantics: one group with count 2 and the shared name; the per-group Filter returns exactly the members; single-member ids never surface) + `M101Branches.VersionsGroupComposesWithOtherFilters` (the group axis ANDs with the M-072 state mask — the deleted prior version is the point of the branch) + `M101Branches.SessionVersionsRoundTrip` (the nflags bit-14 block round-trips; the group enumeration survives the reload; the tags flag does NOT ride the wire — the finding's own defer note) + `M101Branches.TagsExtractedGrouping` (the EXIF-bearing files group under the branch; the complement returns all) + `M101Branches.ExistingBranchesUnchanged` (the M-018 junk bucket still hides rows AND never joins a group — the three landed branches keep their shapes). Full Linux release suite 1550/0; the ASan lane 1549/0 LSAN-clean.
**Resolution notes:** CONFIRMED-DIVERGENCE (the corpus citation re-read first: UIUX/09-results-phase-deep-dive.md:27-36 — the FIVE distinguished roots with their tr-verified coaching copy; the Versions tooltip 'It is another version of the file in the same directory. You can view different versions of the same file'; the Tags Extracted tooltip 'We classify and organize the found files by extracted tags'). The landing, per the finding's fix plan: (1) `FileProperties::version_group` — the deterministic namespace-identity hash (FNV-1a 64 over the namespace-best link's parent record number and name; no map, no second pass; the multi-parent hard-link case carries the namespace-best link's identity, the same one-link tie-break M-007 documents) stamped by the NTFS walk at emission; PASSIVE datum (never consumed by read/recovery); persisted as the nflags bit-14 append-only block (8 bytes, fires only when nonzero — pre-M-101 sessions stay shape-identical); (2) the query exposure: `Row::version_group` + `Filter::version_group` (the per-group membership, composes with every M-072 axis) + `query::version_groups()` (the branch enumeration — one counting pass + one collecting pass, junk-filtered nodes never members); (3) `FileProperties::tags_extracted` — the RUNTIME flag (NOT persisted: the corpus shows the og persisting no tag state in the session — the finding's own defer note; a reloaded session re-derives through the host's preview pass where extract_metadata runs) + `Filter::tags_only` + `Row::tags_extracted`; (4) the snapshot carries both axes append-only (SnapNode fields, in-memory value snapshot — no wire bump). The Versions-node-for-non-NTFS open question: the corpus ties the branch to NTFS record multi-version (UIUX/09:31) — the id is stamped for structured NTFS nodes only; other engines carry 0 (not-a-member), the honest corpus-scoped reading.

---

## Session-93 S93 lane addendum (2026-10-30) — the concurrent-landing concession + the two riders this lane lands on top of the seat

**The concession (SA-9, earlier-push-wins):** this lane (opened at
`fc4ffdf4`, pushing as S93 after the racing window took S89-S92) landed
an independent M-101 fix concurrently: a u32 counter group id assigned
by a post-walk phase F over the (parent record, name) collision key, a
node-level runtime tags bit, `versions_only`/`tags_extracted_only` query
axes, and nflags bit-14 as a u32 block. The racing seat (above) holds:
its namespace-identity FNV-1a hash is the better carrier (identity at
emission, no second pass, deterministic per pathname), and its session
wire (the u64 bit-14 block) is the pushed form. This lane's convergent
implementation was withdrawn at the merge; every duplicate surface was
removed (the u32 field, phase F, the duplicate query axes and Row
fields, the u32 write/read blocks, the four convergent-duplicate tests).

**The two riders this lane lands ON TOP of the seat (both unique, both
kept):**

1. **The Tags Extracted PRODUCER** — the seat's `tags_extracted` props
   flag had NO stamping path anywhere (its own comment: "stamped by the
   host's preview/metadata pass" - with no such pass in the code; grep
   clean at the seat's tree: zero setters). A carrier with no producer
   is the no-disabled-surface class. The rider: the ABI preview
   metadata call stamps the live node's props when the extraction
   produced tag data (the seat's own documented semantics: EXIF
   orientation/datetime, media duration/codec, dimensions), through a
   session-liveness watch so the preview handle stays session-independent
   (post-close: nothing marked, metadata still served from the private
   copy). Pinned by CrUi.M101TagsExtractedMarkAndBranchAxes.
2. **The ABI exposure** — the seat landed no ABI surface at all; the
   rider appends the branch axes to the C ABI (the N-090 tail pattern,
   zero-init = safe default): `cr_ui_node_info.version_group` (u64, the
   seat's hash) + `.tags_extracted`, `cr_ui_query.version_group` (0 = no
   filter) + `.tags_only`, `cr_ui_query_row.version_group` +
   `.tags_extracted` - plain-C driven in ui_abi_check.c (the A-ABI-3
   class swept). The merge repair on the racing window's own residue is
   recorded in the merge commit (the born-collided WPD test name and the
   format drift).

The seat's Resolution above stands unchanged; this addendum records the
concession and the riders only.
