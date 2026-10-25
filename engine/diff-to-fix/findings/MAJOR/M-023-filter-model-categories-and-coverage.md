# MAJOR M-023 — filter model: og is a runtime 8-category/1,481-extension XML filter; core is a hard-coded 7-category/64-extension table (Email and Bookmarks missing entirely)

**Severity:** MAJOR
**Module:** query/filter (visualization row), visualization
**Parameters affected:** B16 (filter model — predicate set and semantics), D13 (report fields), B10-adjacent (filename handling via extension table)
**Corpus references:** `og-codebase/FileFilter.xml` (first-hand parse this session: root `All types` Selected=2; 8 `<FileCategory>` nodes = Graphic, Document, Email, Audio, Video, Other, Archive, Bookmarks; 9 `<FileType>` nodes; 1,489 `<Extention>` entries, 1,481 unique case-insensitively; per-node `<Selected>` state; iconIndex/Description/Label metadata); `re-workspace/artifacts/config-files/` (the FileFilter census artifacts; CORRECTIONS.md row 5 confirms the recount 8/1,481); `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :9766-9842 (`config/config.ini [User] only_common` — the carve-side runtime config gate; same runtime-config pattern)
**Core references:** audited commit `34b3441` — `include/cr/query/cr_query.h:32-40` (`enum class Category : u8 { None, Image, Video, Audio, Document, Archive, Other }` — no Email, no Bookmarks), `src/core/query/cr_query.cpp:27-61` (`kMap` = 64 entries, first-hand count this session; `cat_of` → unknown extensions fall to `Category::Other`); the visualization module file's "(project GAP) MAJOR 7-cat/85-ext" row (the 85 figure is stale — the in-tree count is 64)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's user-facing filter is a **runtime-loaded XML document**: 8 categories (including **Email** — 19 extensions: pst, eml, emlx, dbx, nsf, edb, mbx, pab, … — and **Bookmarks**), 1,481 unique extensions, per-category/per-type/per-extension `Selected` toggles, and UI metadata (icons, descriptions, labels). The core's filter model is a **compile-time 64-entry table** with 7 categories; the Email and Bookmarks categories do not exist in the `Category` enum, so every email artifact (PST/OST/DBX/EML…) and bookmark file classifies as `Other`. The visualization module has carried this as an unnumbered "(project GAP)" row since session 1; per the session-3 mandate (mark all remaining features as findings), it is now filed with evidence.

## What the reference does

`FileFilter.xml` ships beside the product binary and is read at UI startup: the tree carries `Selected` state at every level (root default 2 = on), `iconIndex`, human `Description`/`Label` strings ("common image files or digital camera photos, such as JPG, etc."), 9 FileType subgroupings inside the categories, and 1,489 extension entries (1,481 unique). The og's pattern of runtime-configurable product behavior extends to the carve engine (`config/config.ini [User] only_common`, decoded this session in N-031) and the per-extension carve toggles (`RawConfig.ini`, 34 toggles per corpus CORRECTIONS row 5).

## What our core does

`cr_query.h` defines a 7-value category enum; `cr_query.cpp` maps 64 extensions in a hard-coded table; `cat_of()` sends every unknown extension to `Other`. `passes()` (cr_query.cpp:110+) applies `category_mask`, `extensions`, `name_substring` as name-dependent predicates evaluated last (a documented perf ordering). There is no runtime filter document, no user-editable category membership, no Email/Bookmarks categories, and no per-category UI metadata.

## Gap

Falsifiable: (a) a `.pst` in results gets `Category::Other` in the core and `Email` in the og; (b) any of the ~1,417 extensions the og classifies but the core does not (e.g. `.srw`, `.3fr`, `.x3f`, `.cr3`, `.jfx`, `.nsf`) land in `Other`, so a "Graphic only" filter in a parity UI built on our query layer drops camera-RAW files the og keeps; (c) no runtime extension exists to add categories without a rebuild.

## Impact

User-observable (any UI/CLI consumer of the filter model): category-filtered views mis-bucket a large fraction of real-world file populations — the email class entirely, and the long tail of camera/document/archive formats. Report consumers (JSON report schema) get a coarser category field than the og's UI presents. Not a recovery-correctness issue (files are still found; only their classification and filterability change), hence MAJOR not SEVERE.

## Reproduction / verification

Corpus-derived (first-hand XML parse this session + CORRECTIONS row 5 recount) and core-derived (enum + table read at `34b3441`; 64-entry count by mechanical parse). Executable check: `category_for_extension("pst") == Category::Other` on the current tree — one-line test below.

## Fix plan (for the engineering team)

1. **Extend the enum**: add `Email = 7`, `Bookmarks = 8` to `Category` (ABI: the enum is `u8`; adding values is backward-compatible if `Other` keeps its numeric slot — it does, since appends go after 6).
2. **Grow the table to data**: replace the 64-entry hard-coded `kMap` with a generated table from the corpus `FileFilter.xml` (a `scripts/gen_filefilter.py` in the rawsigns pattern — parse the XML, emit `{ext, cat}` pairs; keep the generator in-repo so corpus updates regenerate). This closes the 64→1,481 gap mechanically and keeps case-folding semantics.
3. **Runtime filter document (optional, parity-plus)**: load `FileFilter.xml`-shaped overrides at startup if product requirements demand user-editability; otherwise document the compile-time choice as a deliberate divergence (the INFORMATIONAL pattern).
4. **Tests**: `Filter.EmailCategoryExists` (`category_for_extension("pst") == Category::Email` — fails today), `Filter.BookmarksCategory` (`.bookmarks`), `Filter.CameraRawClassified` (`.cr3`/`.x3f`/`.srw` → Image, fails today), `Filter.TableCount` (generated table size == corpus unique count, guards regeneration drift).
5. **Docs**: update the visualization module GAP row to M-023 and refresh the stale 85-ext figure to the generated count.

## Tests to add

See fix plan §4 (four named tests; the first two fail-before by construction).

## Related findings

N-031 (the only_common runtime-config gate — the same runtime-config family), S-007 (carve format coverage — the Email formats like PST are also carve-relevant), N-020 (doc drift — the 85-ext figure).

## Open questions

Whether the product brief wants the og's runtime-editable filter (XML beside the binary) or accepts compile-time parity of CONTENT only — a product decision recorded here; the fix plan supports either.

---

## Session-4 addendum (2026-10-26, commit `63a9167`) — recount correction + model decode extension

1. **Core category count corrected: 6 real categories, not 7.** The Category enum
   (`include/cr/query/cr_query.h:32-40`) is None=0 (sentinel) + Image/Video/Audio/
   Document/Archive/Other — the "7-cat" figure in this finding's original text
   counted the sentinel. The extension map is 64 entries
   (`src/core/query/cr_query.cpp:22-45`; this session's mechanical recount). The og
   remains 8 categories / 1,489 elements / 1,481 distinct ("Other" = the IsMutex
   fallback with zero extensions; 8 cross-listed duplicates) — the gap stands, now
   with exact numbers on both sides: **8 vs 6 categories, 1,481 vs 64 extensions,
   Email + Bookmarks missing.**
2. **The filter/sort/paging model is now decoded beyond the category gap** — see the
   companion finding **M-029** (renumbered from M-025 after the parallel S3/S4 legs filed their own M-024/M-025) (sort keys, paging semantics, Date/Size bucket IDs,
   the 7-group AdvanceFilterCondition, two filter-bar modes, server-side filter push,
   FilterFolder/TxtViewFiles noise filters, the 34 RawConfig carve toggles). This
   finding remains the category/extension-coverage axis; M-029 carries the model axes.
3. **Grading linkage**: the og's RecoveryAssistant precomputes per-category counts
   (IDs 5/6/7/8/9/0xB) at scan completion (S-017) — the category model the fix ports
   should be the single source for both the query filter and the assistant statistics.
