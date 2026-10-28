

---

## Session-11 fix row (2026-10-26, DFX11 — three M-029 sub-items closed)

- **Type sort CLOSED**: `SortKey::Type` (ABI code 7, append-only) — the
  og's results-view Type column: category in the FileFilter.xml
  iconIndex order, then extension, then name (pinned with the full
  8-row expected sequence).
- **modified-unit doc lie FIXED**: ground-truth-contracts now says
  FILETIME 100-ns since 1601 (the engines' native unit; the old "Unix
  seconds" line was false — 369-year mis-parse for trusting consumers);
  pinned by Query.ModifiedUnitIsFiletimeEverywhere. The stale Category
  enum row (Email/Bookmarks) fixed in the same pass.
- **Paging documented as the deliberate difference**: no og page-size
  constant exists (UI-layer virtualization, ~1000-item batches); the
  core's offset+limit is host-side convenience, no parity claimed.
- REMAINING (decode-blocked, next steps in the finding): the Date/Size
  bucket-ID translation (the og's buckets select by node+0x89 date TAGS,
  not ranges — a range translation without the tag decode would invent
  constants) and the general-raw naming bucket.

---

## Session-32 fix row (2026-10-26, DFX32 — M-029's decode-blocked remainder closed; the finding is fully resolved)

- **Bucket-ID translation CLOSED** (the decode the finding demanded,
  first-hand from ghidra-drwui/DRWUI_decompiled.c): the CTimeFilterBtn
  menu FUN_140705c50 (0x100 Today / 0x101 Yesterday / 0x102 "Last 7
  days" / 0x103 "Last 1 month" / 0x104 SKIPPED / 0x105 Custom) and the
  CSizeFilterBtn menu FUN_140706060 (0x1000 "< 512KB" / 0x1001
  "512KB~1MB" / 0x1002 "1MB~512MB" / 0x1003 "512MB~1GB" / 0x1004
  "> 1GB" / 0x1005 SKIPPED / 0x1006 Custom) — the audit's "<128MB /
  <1GB / <5GB" class-table approximations are corrected by the decode.
  The semantics: day-truncated LOCAL windows from the cached
  today/yesterday/−7d/−30d bounds (FUN_140254670, COleDateTimeSpan
  arithmetic), exact inclusive size boundaries (FUN_140255820),
  inclusive local-day custom ranges (FUN_140256830).
- **API**: `query::apply_date_bucket` / `query::apply_size_bucket`
  (DateBucket/SizeBucket enums; the skipped IDs 0x104/0x1005 and any
  unknown value are REJECTED — the core never invents menu semantics) +
  the `cr_ui_query_apply_buckets` added export (ABI §9 no-bump) so a
  host can express every og bucket ID incl. Custom.
- **General-raw naming CLOSED**: `ScanConfig::raw_category_name` +
  `cr_ui_scan_config.raw_category_name_utf8` — the og's per-scan
  "filter_generalraw" display-name surface (OQ-S4-2's decode), with
  resume-safe root matching (default OR configured name).
- Pinned by Query.DateBucketsTranslateToRanges,
  Query.SizeBucketsTranslateToRanges, Query.BucketFiltersThroughRun,
  CrUi.QueryApplyBucketsFillsAndRejects, CrUi.RawCategoryNameNamesTheCarveRoot
  (all fail-before RED on the neutered tree). M-029 → FIXED/ — every
  sub-item of the finding is now closed (path group s6; Type sort +
  modified-unit + paging s11; buckets + naming s32).


## Session-8 section (2026-10-28, audited `35adcb9`)

**M-072** (MAJOR): the path axis lacks substring and the state dimension is a
boolean — the og's 7-group AdvanceFilterCondition lands as 5 (type/time/size +
name-search + path-prefix). Fix: `path_substring` + `state_mask` (4-bit
DeletedState×Origin mapping) on Filter, the ABI twin, and the CLI; the og's
state-bucket encodings are corpus-undecoded (registered interpretation).
**M-073** (MAJOR) is the scan-time half (PushFolderFilter — see
modules/orchestration.md).


## Session-8 lead-lane wave (audited `ff879f9`)
- **M-101**: the "Versions" node (NTFS multi-version record siblings) + "Tags Extracted"
  branches — the two R-20 roots with no tree concept.
- **N-126**: the "Unsaved File" category (the og's type-scope list).

---

## Session-10 lane-B addendum (2026-10-29, audited `84a1832`)

**N-132 (MINOR, NEW):** the config-layer triplet behind the query/summary model —
SumaryFileType (extension unification groups: `m4p_m4a_m4v_mp4` as one family,
`zip_docx_pptx_xlsx` as the office-in-zip family) and PriorityDisplayType.xml
(5-cat/35-ext within-category presentation order) — are absent while API-DELTA §1.7
row 45 claims them "landed (query/filefilter data layers)" (false; the FileFilter
1481-row membership table and the M-029 buckets ARE landed — the false claim covers
only the two absent models). The summary counts are the plain 8-bucket
`category_counts` (cr_grading.h:115). Disposition required: land the unification
helper or register the plain counter as a deliberate delta; PriorityDisplayType
needs an OOS-UI row (the usefullostfiles.xml precedent) unless the CLI `tree` wants
ordering parity. Grep record:
`evidence/decompiled-quotes/s10-config-layer-absence-greps.md`.
