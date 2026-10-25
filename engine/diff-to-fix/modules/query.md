# modules/query.md — query module audit verdicts (NEW in session 4; extends visualization.md's filter coverage)

Reference: og's results-layer query model — FileFilter.xml (8 categories / 1,489
elements / 1,481 distinct; "Other" = IsMutex fallback, 0 extensions),
PriorityDisplayType (5-cat/35-ext presentation order, 2-digit alias keys), FilterFolder.ini
blocklists + TxtViewFiles 119-ext preview whitelist, RawConfig.ini 34 carve toggles,
Date bucket IDs 0x100-0x103+0x105(Custom), Size bucket IDs 0x1000-0x1004+0x1006(Custom),
AdvanceFilterCondition (0x48 bytes / 7 groups), two filter-bar modes (Path/Type) with
per-mode persistence, **filters pushed into the scan controller at scan start**
(server-side), sort columns 0-5 = Name/Date Modified/Size/Type/Path/None (sort keys =
column-name strings; Recovery Chances is a gauge, NOT a sort column), virtualized paging
(`sigResetStartIdx`, ~1000-item tree batches, no page-size constant). Evidence: session-4
context sheets (corpus config-file reads + `ghidra-drwui/DRWUI_decompiled.c` extracts);
companion finding **M-029** (+ M-023 for the category axis).

Core (audited `63a9167`): `src/core/query/cr_query.cpp` (309 LOC) +
`include/cr/query/cr_query.h` — 8 AND-ed predicates (category_mask 6-bit, size range,
modified range, confidence_min, deleted_only, origin_mask, extensions, name_substring),
5 sort keys (all `std::stable_sort`), offset/limit paging (limit 0 = all, default 100),
`count_only`; category derived per-row from the name (`cat_of`, 64-entry hard-coded
map); recursive walk over the tree; the frozen `tree --csv`/`tree --json` CLI
projections. Tests: 3 unit tests + a perf test (1M-node < 200 ms budget). Prior
sessions covered the category axis only (M-023 via visualization.md); the model axes
are session 4's.

## Verdicts

- **P16 (filter model)** — finding **M-029** (bucket IDs, Custom buckets, 7-group
  advance condition, two-mode persistence, server-side push, noise filters, carve
  toggles all absent) + **M-023 FIXED** (findings/FIXED/M-023: Category enum
  +Email(7)/+Bookmarks(8); the generated 1,481-entry table from the corpus
  FileFilter.xml replaces the 64-entry hand table — scripts/gen_filefilter.py,
  binary-search lookup, category_mask default 0x1FF; the runtime-editable XML
  surface is the documented deliberate non-adoption).
- **P17 (sort model)** — finding **M-029**: core sort set = Path/Name/Size/Modified/
  Confidence (stable) vs og's 6 columns; **Type column missing**; Confidence sort has
  no og counterpart.
- **P18 (paging model)** — finding **M-029**: og virtualizes at the UI layer with no
  page-size constant; core offset/limit default 100 — no og counterpart to check
  against (must be recorded as deliberate, not parity-claimed).
- **P15-adjacent (confidence filter)** — `M`-note: `confidence_min`
  filters the core's own 5-tier model (S-023 now FIXED: the og's grade model
  exists alongside — rows carry `grade`, sort key 6 = grade; the og has no
  confidence filter; its gauge is display-only).
- **P10-adjacent (name normalization)** — `M`-note: `contains_ci` is ASCII-only
  case-folding (matches the og's `_strnicmp`-family semantics for the results search);
  M-010 covers the UTF-8 storage axis.
- **P13-adjacent (report/category counts)** — finding **S-023**: no precomputed
  per-category counts (the og's RecoveryAssistant map); every category-filtered query
  converts names per-row.
- **P65 (test coverage)** — finding **M-029**: Path/Modified/Confidence sorts and the
  bucket-translation surface unpinned; the 1M-node perf budget IS pinned.
- **P46 (contract docs)** — findings **M-029/N-040**: the frozen
  `ground-truth-contracts.md:152` "Unix epoch SECONDS" line is false — the wire is
  FILETIME 100-ns-since-1601 (verified in the FAT engine store path).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-029 | MAJOR | Query model beyond categories: sort keys, paging semantics, bucket IDs, server-side filter push, noise filters, carve toggles, frozen-doc unit lie |
| M-023 | MAJOR (s1/s3, amended) — **FIXED** (FIXED/M-023) | Category/extension coverage gap (counts corrected: 8 vs 6 cats, 1,481 vs 64 ext) → generated 1,481-ext table + 2 new categories |
| N-040 | MINOR (shared) | The frozen-contract `modified` unit falsehood is canonically fixed in M-029's plan |
| S-023 | SEVERE (shared) | No precomputed category map / assistant statistics (query re-derives per row) |

---

## Session-5 audit (audited commit `7cf6754`)

- **M-029 addendum (filterbar decode, UIUX 31 first read):** the 7-dimension
  AdvanceFilterCondition layout confirmed (tag + path dimensions have NO core field); 3
  filter-slot persistence (CAdvanceFileFilterMgr) absent; CSetFilterThread refilter
  worker vs synchronous core query; CCheckNodeThread re-grade wiring (TimeCritical) now
  cited for S-023's per-selection re-grade. OQ-S4-2's naming correction folded: the og has
  no FileNNNN scheme (N-060); the +0x48 hook is the filter-push config source, i.e. it
  belongs to this module's server-push section.
