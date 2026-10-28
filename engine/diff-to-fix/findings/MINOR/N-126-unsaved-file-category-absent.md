# MINOR N-126 — the "Unsaved File" category is absent: the og's search/type scope lists it as a first-class category (office unsaved-doc recovery) — core's category model has no equivalent

**Severity:** MINOR
**Module / Surface:** query / tree (modules/query.md)
**Parameters affected:** A16-adjacent (category model), A42-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md:49` (the search's Optional Type scope list: Pictures/Videos/Documents/Audio/Archive/Emails/Bookmarks/**Unsaved File**); `UIUX/25-connect-wiring-graph.md:146-151` (the search funnel wiring).
**Core references:** audited commit `ff879f9` — The Query Category enum (type/time/size axes landed; cr_query) carries no unsaved-doc category; grep for unsaved across src/: the only adjacency is the office-temp-file carve names if any. R-21's row: "no 'Unsaved File' (office unsaved-doc) category" (docs/REFERENCE-api-gap gap row).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** R-21 PARTIAL (name/substring/type present); the missing category never formalized.

## Summary

('The og\'s type-scope list includes "Unsaved File" — the office unsaved-document family (Word/Excel autosave .asd/.wbk, the OART temp forms) as a searchable category. Core\'s category model has no equivalent axis, so hosts cannot surface the unsaved-doc story the og\'s UI promises. The engine-side prerequisite is a category mapping over the autosave-extension family (a carve-naming or query-category job, not a new parser).',)

## What the reference does

As cited — the type-scope list with Unsaved File among the seven categories.

## What our core does

No unsaved category in the Query model; no autosave-family grouping anywhere.

## Gap

Falsifiable: the og's search can scope to Unsaved File; no cr_ui query parameter expresses it.

## Impact

A user-facing recovery category (unsaved docs are a top support scenario); small engine-side (a category mapping), larger content-side (the autosave format family's carve coverage is a separate question — see Open questions).

## Reproduction / verification

Corpus documentation evidence; core state grep-verified this session.

## Fix plan (for the engineering team)

1. Add the category: a Query category value (append-only enum) mapping the autosave extension family (.asd, .wbk, .tmp office forms, ~-prefixed locks — the og's list needs one decode pass, see Open questions) + the carve-naming plumbing so recovered autosaves land in it. 2. FROZEN-JSON + three-site lock updates. 3. The category is only as good as the carve reach for those formats — cross-file the extension-family carve coverage as the dependency.

## Tests to add

- `Query.UnsavedFileCategory` — a planted .asd file lands in the category.
- `Query.CategoriesUnchanged` — the landed list keeps its values (append-only guard).

## Related findings

M-072 (the query-axes family); M-101 (the tree-branch family).

## Open questions

The og's exact Unsaved File membership list (which extensions/classes the category covers) — the UIUX artifact shows the category label, not the membership; one read of the category table (the R-27 type-ID space) before landing.
