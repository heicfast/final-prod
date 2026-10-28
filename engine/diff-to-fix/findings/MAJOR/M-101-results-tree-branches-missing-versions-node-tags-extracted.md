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
