# MAJOR M-029 — Query model divergences beyond M-023: sort keys, paging semantics, filter-bucket IDs, server-side filter push, noise filters, and carve toggles

**Severity:** MAJOR
**Module:** query (new module row — modules/query.md)
**Parameters affected:** P16 (filter model — bucket IDs + noise filters + carve toggles), P17 (sort model), P18 (paging model), P10-adjacent (name normalization in substring match), P13 (report — category counts), P37-adjacent (filters pushed into scan vs query-side)
**Corpus references:** `og-codebase/FileFilter.xml` (8 categories / 1,489 extension elements / 1,481 distinct; "Other" = IsMutex fallback, 0 extensions), `og-codebase/RawConfig.ini` (34 carve toggles, STL=0/VPJ=0), `og-codebase/config/FilterFolder.ini` (folder/file blocklists, 13-locale "New Folder", BitLocker artifacts) + `TxtViewFiles` 119-ext whitelist, `re-workspace/artifacts/config-files/filtering-priority--config-layer.md` (PriorityDisplayType 5-cat/35-ext, 2-digit alias keys), `re-workspace/artifacts/UIUX/` results-view captures (filter bar: Show/File Type/Date Modified/File Size + AI Recommend; two filter-bar modes Path/Type; sort columns; virtualized list), `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — AdvanceFilterCondition 0x48-byte/7-group struct, Date bucket IDs 0x100-0x103+0x105 (Custom), Size bucket IDs 0x1000-0x1004+0x1006 (Custom), date tags from node+0x89 bits 11-14 with 1600/1601/>2100 special cases, filters pushed into the scan controller at scan start, `sigResetStartIdx` virtualized paging with ~1000-item tree batches
**Core references:** `include/cr/query/cr_query.h:32-60` (Category 6-value enum None..Other; Filter predicate set), `src/core/query/cr_query.cpp:22-45` (64-entry hard-coded extension map), `:100-135` (predicate evaluation, query-side only), `include/cr/query/cr_query.h:44-47` (100-ns units), `docs/ground-truth-contracts.md:148-152` (frozen query/tree contract — "Unix epoch SECONDS" claim, false), audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied (M-023's category-count row gets a correction addendum from this session's recount)

## Summary

Beyond M-023's category/extension gap (which this session re-verified: the core ships a 6-category/64-extension hard-coded map vs the og's 8-category/1,481-distinct-extension runtime XML), the whole query model diverges in five more dimensions: (1) the og's filter buckets are stable numeric IDs (Date 0x100-0x105, Size 0x1000-0x1006, both with Custom entries and skipped IDs) with a 7-group advance-condition struct, and filters are pushed INTO the scan at start (server-side); the core filters query-side only with raw ranges. (2) The og's sort model is 6 columns (Name/Date Modified/Size/Type/Path/None) keyed by column-name strings; the core's sort keys are its own set (Path/Name/Size/Modified/Confidence) with no Type column. (3) The og's paging is a virtualized list (~1000-item incremental batches, `sigResetStartIdx`); the core is offset/limit with default 100 — no og counterpart constant. (4) The og layers noise filters (FilterFolder.ini blocklists, TxtViewFiles preview whitelist) and 34 carve toggles (RawConfig.ini); the core has neither surface. (5) The frozen `tree --json` contract doc misdocuments the `modified` unit (says Unix seconds; the engines store FILETIME 100-ns-since-1601 — verified in the FAT engine).

## What the reference does

(Every numeric claim below from FileFilter.xml/RawConfig.ini/FilterFolder.ini direct reads
plus the DRWUI decompile; citations in §Corpus references and the context sheets.)

1. **Filter buckets with stable IDs**: Date = 0x100/0x101/0x102/0x103 + **0x105 Custom**
   (0x104 skipped); Size = 0x1000..0x1004 + **0x1006 Custom** (0x1005 skipped); date-tag
   derivation from node+0x89 bits 11-14 with year-string fallback special-casing
   1600/1601/>2100. `AdvanceFilterCondition` = 0x48-byte struct, 7 sub-condition groups,
   all-enable-bits-empty → clear. TWO filter-bar instances (Path-mode and Type-mode)
   persisting last-filter-per-mode in CAdvanceFileFilterMgr's 3 slots.
2. **Filters are scan inputs**: the active filter is pushed into the scan controller at
   scan start (the engines see it; the RAW engine additionally receives a per-file
   `std::function<string(string,string)>` naming/filter hook — vtable +0x48). The
   results view is a filtered projection, not a post-hoc query.
3. **Sort model**: columns 0-5 = Name / Date Modified / Size / Type / Path / None; sort
   keys are the literal column-name strings; "Recovery Chances" is a gauge widget, NOT a
   sort column.
4. **Paging**: virtualized list with `sigResetStartIdx(int)` reset and ~1000-item
   incremental tree batches; no Qt fetchMore machinery and no page-size constant —
   there is no og "page size" to parity-check against.
5. **Noise filters**: FilterFolder.ini blocklists (folders, files, 13-locale
   "New Folder" variants, BitLocker artifacts); TxtViewFiles = 119-extension
   text-preview whitelist; SumaryFileType unification groups;
   PriorityDisplayType = 5-category/35-extension presentation ordering with 2-digit
   alias sort keys.
6. **Carve toggles**: RawConfig.ini ships **34** toggles (not 37 — CORRECTIONS.md #5),
   STL=0/VPJ=0 disabled by default; usefullostfiles.xml = 27 extensions (13/7/7 split);
   the RAW binary's own valuable-map = 14 extensions.
7. **Category truth**: 8 categories (Graphic/Document/Email/Audio/Video/Other/Archive/
   Bookmarks, iconIndex 0-7); 1,489 extension elements / 1,481 distinct (8 cross-listed
   duplicates); "Other" = the IsMutex fallback with zero extensions of its own.

## What our core does

- `cr_query.h:32-40`: `Category` = None/Image/Video/Audio/Document/Archive/Other —
  **6 real categories** (M-023's "7-cat" figure is corrected by this session's recount:
  the seventh value is `None=0`, not a category), 64 extensions hard-coded
  (`cr_query.cpp:22-45`), category derived per-row from the name at query time
  (`cat_of`, `:62-70`) — no Email, no Bookmarks, no "Graphics" naming, no runtime
  filter file, no per-category counts cache.
- Filter predicates (`cr_query.h:48-58`): `category_mask`, `size_min/max`,
  `modified_from/to` (raw 100-ns ranges — no bucket IDs, no Custom bucket, no
  date-tag derivation), `confidence_min`, `deleted_only`, `origin_mask`,
  `extensions`, `name_substring` — **query-side only**; nothing is pushed into the
  scan; no per-file carve naming hook.
- Sort (`cr_query.h`, `cr_query.cpp` sort block): Path / Name / Size / Modified /
  Confidence, all `std::stable_sort` — no **Type** column (the og's column 3), and
  Confidence sort has no og counterpart (the og's gauge is not sortable).
- Paging: `offset` + `limit` (limit 0 = all, default 100), `count_only` — the default
  100 has no corpus anchor (the og has no page-size constant).
- No FilterFolder-equivalent blocklist, no TxtViewFiles-equivalent preview whitelist,
  no per-format carve toggles (carve format set is compile-time; N-031 records the
  dispatch semantics side).
- `docs/ground-truth-contracts.md:152` — the FROZEN `tree --json` contract claims
  `modified` is "Unix epoch SECONDS"; the engines store FILETIME
  (verified: `cr_fat_quick.cpp:256-259` `unix*10000000 + 116444736000000000`;
  `cr_query.h:46-47` documents 100-ns units). A consumer trusting the frozen doc
  mis-parses every timestamp by 369 years.

## Gap

Falsifiable claims:
(a) No core API accepts or represents the og's Date/Size bucket IDs or the 7-group
advance condition; a host porting the og's filter bar cannot express its state.
(b) The core cannot push filters into the scan (no server-side filtering); the og does.
(c) The core's sort set lacks the og's Type column and adds Confidence (no og
counterpart); no sort-key mapping table exists.
(d) The core's default page size 100 and offset/limit model have no og counterpart to
check against (the og virtualizes at the UI layer) — the core must not claim paging
parity it cannot have.
(e) No blocklist/whitelist/carve-toggle surface exists in the core.
(f) The frozen contract doc's `modified` unit claim is false (FILETIME in reality).

## Impact

- **Host-application parity**: a UI built on the core cannot reproduce the reference
  filter bar (buckets, two modes, persisted filters) or the reference results-table
  column set without client-side reimplementation of the entire bucket model.
- **Performance**: category-per-row derivation at query time (vs the og's assistant-
  precomputed category map — S-023) makes every category-filtered query O(n) in name
  conversion; the 1M-node <200 ms budget is preserved by lazy conversion, but bucket-
  filtered queries on category still convert.
- **Wire-consumer correctness**: the frozen `tree --json` doc actively misleads
  integrators on the `modified` unit.
- **Scan-scope control**: without carve toggles, hosts cannot disable individual carve
  formats the way RawConfig.ini does (34 toggles, STL/VPJ default-off).

## Reproduction / verification

- **Corpus documentation + file evidence (method 3)**: direct reads of
  `og-codebase/FileFilter.xml` (category/extension census — 8/1,489/1,481),
  `og-codebase/RawConfig.ini` (34 toggles), `og-codebase/config/FilterFolder.ini`
  (blocklists + TxtViewFiles 119-ext) by the corpus-reading leg; convergent with
  `re-workspace/artifacts/config-files/filtering-priority--config-layer.md` and the
  CORRECTIONS.md #5 recount.
- **Decompiled evidence (method 2)**: bucket IDs / AdvanceFilterCondition / two
  filter-bar modes / filter-push-at-scan-start / sort columns / `sigResetStartIdx`
  extracted from `ghidra-drwui/DRWUI_decompiled.c` (UIUX + config-layer context
  sheets, citations in §Corpus references).
- **Core-side verification (executed greps + reads)**: category enum recount
  (`cr_query.h:32-40`), extension-map count (64 entries at `cr_query.cpp:22-45`),
  sort-set enumeration, paging defaults, the FILETIME store path
  (`cr_fat_quick.cpp:256-259`) vs `ground-truth-contracts.md:152`.
- Og-execution comparison not performed (UI-layer behaviors); the decompile + config
  files are the ground truth for this layer.

## Fix plan (for the engineering team)

1. **Architecture**: decide the layer split — recommend implementing bucket IDs as a
   thin core-side translation (`cr_ui_query_filter_from_buckets(date_id, size_id,
   custom_from, custom_to, ...)`) rather than pushing buckets into `tree::Filter`
   (keeps the engine layer clean while giving hosts the og-expressible surface).
   Server-side filter push: add an optional scan-time name/extension predicate to
   `ScanConfig` (the og's scan-input filter) — mark it INFORMATIONAL-superset if the
   core chooses to stay query-side-only, but then the ABI doc must say so.
2. **Files to modify**: `include/cr/query/cr_query.h` (+ bucket ID enum with the exact
   og values incl. skipped IDs; + optional `type_sort_key` derivation), `src/core/query/
   cr_query.cpp` (bucket translation; Type-column sort key = category-then-extension;
   optional scan-push hook), `include/cr/cr_ui.h` + `src/ui/cr_ui.cpp` (bucket-flavored
   query entry; per-format carve toggle surface reading an optional RawConfig-like
   list), `docs/ground-truth-contracts.md` (fix the `modified` unit line to FILETIME
   100-ns-since-1601 — one-line doc fix, do FIRST).
3. **Constants**: Date 0x100..0x103, 0x105; Size 0x1000..0x1004, 0x1006; date-tag
   special cases 1600/1601/>2100; FilterFolder blocklist import (config-file reader,
   not hard-coded); TxtViewFiles whitelist for preview eligibility.
4. **Tests to add**: `Query.DateBucketsTranslateToRanges` (each ID → expected range,
   Custom passthrough), `Query.SizeBucketsTranslateToRanges`,
   `Query.TypeSortKeyOrdersByCategoryThenExtension`,
   `Query.ModifiedUnitIsFiletime Everywhere` (pins the unit after the doc fix),
   `Query.CraftTogglesDisableFormats` (STL/VPJ off → not carved),
   `Query.BlocklistFiltersJunkFolders` (New Folder 13 locales + BitLocker artifacts).
5. **Contract/doc updates**: `ground-truth-contracts.md` §11 unit fix;
   ABI-CONTRACT query section documents the bucket translation + the paging model's
   non-parity (no og page-size constant — deliberate difference, record it);
   CAPABILITY-MATRIX query row.
6. **Acceptance criteria**: a host can express every og bucket ID incl. Custom and get
   the same predicate; the frozen-doc unit line matches the wire bytes (verified by a
   JSON-tree round-trip test); blocklisted folders never appear in query rows when the
   blocklist is loaded; 306/306 + new tests green.

## Tests to add

`Query.DateBucketsTranslateToRanges`, `Query.SizeBucketsTranslateToRanges`,
`Query.TypeSortKeyOrdersByCategoryThenExtension`, `Query.ModifiedUnitIsFiletimeEverywhere`,
`Query.CraftTogglesDisableFormats`, `Query.BlocklistFiltersJunkFolders`,
`Query.FilterPersistedPerMode` (if the per-mode persistence lands host-side, pin the
core translation instead).

## Related findings

- **M-024 (parallel S3/S4 leg — RawConfig filter surface)** — the carve-toggle axis
  (RawConfig.ini 34 toggles) is that finding's subject; this finding cross-references
  it rather than duplicating (the toggle surface belongs to the carve module row).
- **M-023** — the category/extension gap this finding extends (its "7-cat" figure gets
  this session's correction addendum: 6 real categories + None sentinel).
- **S-023** — the assistant's precomputed category map is the og-side counterpart of
  the core's per-row `cat_of`; fixing one should fix the other's data source.
- **N-031** — carve dispatch semantics (u32→u16→linear→sign-DB); the toggle surface
  here is the config layer over that dispatch.
- **N-040** — the `modified` unit doc lie is also filed there as part of the doc-
  truthfulness bundle; the canonical fix lands in this finding's plan.

## Open questions

1. Whether the og's per-file RAW naming hook (`std::function<string(string,string)>`)
   reaches beyond naming into emission gating — the decompile shows registration, not
   the engine-side consumption sites; tracked in
   `tracked-open/OQ-S4-2-raw-naming-hook-consumption.md`.
2. The 8 cross-listed duplicate extensions' precedence rules in FileFilter.xml
   (which category wins when an extension appears twice) — needs a 5-minute config
   read during the fix leg; recorded so the port does not guess.

---

## Session-5 addendum (2026-10-26) — Gap (b) corrected: the RAW naming hook is a single-site category display-name mapping, not a per-file filter

OQ-S4-2 is RESOLVED this session (tracked-open updated; full decode at
`evidence/decompiled-quotes/s5-oq-s4-2-raw-naming-hook.md`):

- Registration confirmed (DRWUI:917271-917296 → controller vtable slot
  +0x48 setter FUN_18000b8d0, RAW:8695-8705; std::function stored at
  controller+0x30, impl at +0x68).
- **The single consumption site is RAW:9090** (inside vtable-slot-13
  method FUN_18000beb0): the hook is invoked ONCE with the reserved key
  **"filter_generalraw"** + empty string, and the returned string NAMES the
  general-RAW filter/category node (tree node type 6) in the RAW result
  tree (FUN_180057b80/FUN_1800579b0/FUN_180057e80 attach it).
- DRWUI side maps the key via `<appdir>/firebase.json`
  (FUN_140161460 :263298-263340) with an external callback fallback
  (FUN_140160ab0 :263193-263290) and an "url_" special case; the hook
  ALWAYS returns a name (no drop/empty path).
- **Fix-plan step 1 correction:** the "server-side filter push" decision is
  narrower than assumed — what the core needs is (a) a stable, nameable
  general-raw category bucket (the og's display name is localized/
  remote-config-driven), and (b) optionally a name-override callback on the
  scan API for UI hosts. No per-file rename or filter gating is required
  for parity.
