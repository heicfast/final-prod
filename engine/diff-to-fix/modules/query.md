

## Session-93 fix row (2026-10-30 — the M-101 concession + this lane's two riders over the racing seat)

- **M-101: the SA-9 concession** — this lane's convergent implementation
  (u32 counter ids / phase F / node-level tags bit / bit-14 u32) was
  withdrawn at the merge; the racing seat holds (the FNV-1a
  namespace-identity hash at emission + the u64 wire + tags_only +
  version_groups()). **The riders this lane adds on top:** (1) the Tags
  Extracted PRODUCER — the seat's props flag had no stamping path (grep
  clean at its tree); the ABI preview metadata call now stamps the live
  node's props when the extraction produced tag data (the seat's
  documented semantics: EXIF orientation/datetime, media duration/codec,
  dimensions), through a session-liveness watch so the handle stays
  session-independent (post-close: nothing marked, metadata still
  served). (2) the ABI exposure the seat lacks:
  cr_ui_query.version_group (0 = no filter) + .tags_only,
  cr_ui_query_row.version_group + .tags_extracted,
  cr_ui_node_info.version_group (u64) + .tags_extracted — the N-090
  tail-append pattern, zero-init = safe default, plain-C driven in
  ui_abi_check.c. Pinned by CrUi.M101TagsExtractedMarkAndBranchAxes
  (RED first-hand via the block-neutralize dance at the pre-merge tree:
  the mark block removed → FAILED; restored → GREEN). The merged-tree
  suite: 1712/1712.

## S12 addendum (2026-10-29 — the session-2 deep-dive)

New active findings on this module: **N-167** (the found-size aggregate absent —
the og's "Files Found: N (X GB)" completion headline is unrenderable from core stats).



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


## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

New active findings on this module: **N-194** (the og's two byte-size display
formatters — log-side KB/MB/GB binary tiers with the +1 flip boundaries, UI-side
B..TB with caller precision — have no core counterpart; hosts cannot render
og-faithful sizes; the M-125 `progress_display_*` family is the landed precedent
for the fix).


## S66 lane (2026-10-29): N-194 landed

**N-194 FIXED** — the og's two byte-size display formatters landed beside the
M-125/M-126 display-math families: `query::display_size_log` (the log side:
KB/MB/GB only, `%.2f`, 1024-tier divisors under decimal labels, the +1 flip
boundaries — 1 MiB renders "1024.00 KB", 2 TiB renders "2048.00 GB", bytes < 1
writes nothing) and `query::display_size_ui` (the tr-family UI side: the full
B..TB ladder at 0x401/0x100001/0x40000001/0x10000000001, caller precision, the
zero-bytes forced-0 quirk, and the all-zero-full-precision fraction strip).
Both corpus-verified first-hand at DRWUI:264525-264583 and :1556295-1556400 at
the fix. The CLI's four size prints (image info, disk listing, volume listing,
unreadable-bytes diagnostic) render through the log-faithful form; throughput
rates stay rate-formatted. Pins: `Display.SizeLogFormatterBoundaries` /
`Display.SizeUiFormatterTiers` / `Display.SizeUiFormatterPrecisionAndStrip`
(RED: link-failure Not-Run with the implementation stashed; GREEN post-fix).

## S69 addendum (2026-10-29 — the ninth continuation lane's deep dives; core `3799dee2`, re-verified LIVE at `271f5d06`, corpus `be0cfe9`)

New findings touching this module: **N-215** (MINOR — R-20's mechanism
mis-attribution in REFERENCE-API-GAP.md:214: the og's "Unimportant files"
bucket is the usefullostfiles.xml extension gate at tree-build time
[DRWUI:635764-635910, first-hand — _strlwr + set-membership, strlen<3
folders, the recursive any-useful-descendant walk], NOT the M-018 junk-name
filter; doc-truth, the behavioral gap itself is OOS-UI-dispositioned);
**I-048A/B registers** (FileFilter.xml Selected=2 = dead data, no reader
anywhere — the core's omission correct; the og's summary-family groups are
remote-tunable at runtime [value_sumaryfiletype/value_fastgoodfile,
DRWUI:319007-319022] — the core's N-132 tables are the static ini snapshot,
remote layer REPORTING-OOS). Verified-MATCH this lane: the N-132 display
tables byte-exact vs the XML; the FileFilter membership data (1,481 unique
exts). Evidence: `evidence/s69-lane-sheets/S69-A-localdb.md`.

## Session-81 addendum — N-215: the R-20 "Unimportant" attribution re-truthed (2026-10-29)

**The doc-truth drift closed.** REFERENCE-API-GAP's R-20 rows attributed
the og's "Unimportant files" branch to the M-018 junk-name filter — the
wrong layer. The corpus re-verified first-hand this session: the og's
bucket is the **usefullostfiles.xml extension gate at tree-build** (the
reader FUN_1401b8380 — `config\usefullostfiles.xml`, a 27-ext set with a
101-entry cap, DRWUI:320396; the bucket builder FUN_1403bb970 at the
"Build tree finish." handler — the "Unimportant files" node at :635824,
`_strlwr` + set-membership for files, `strlen<3` folders, the recursive
any-useful-descendant walk, DRWUI:635764-635910). The M-018 FilterFolder
blocklist is the separate scan-time hide — two different data sources,
timing, and user visibility. Both R-20 rows re-truthed with the cites;
PARITY-OG-CORE-INVENTORY's usefullostfiles.xml row cross-linked so the
two documents cannot drift apart again. The behavioral gap itself stays
OOS-UI (display-tier demotion, never a drop — files are recovered
regardless). #16/#46 rows: MATCH (attribution layer).
