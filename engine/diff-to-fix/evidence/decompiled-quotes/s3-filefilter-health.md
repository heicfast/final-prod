# S3 evidence — FileFilter.xml engine-side consumption + DiskHealth grading decode

Quotes: `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c`
(exFAT), `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (DRWUI),
`re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` (DiskHealth),
`og-codebase/FileFilter.xml` (md5-verified parse this session). Decoded by
the session-3 filter/health agent, 2026-10-26.

## PART A — FileFilter.xml

### A1. Artifact structure (mechanical parse, verified)

8 categories / 1,489 `<Extention>` entries / **1,481 globally unique**
(cross-category dupes: .insp .md .ari .insv .gz .jar .xz .uzip). Categories
(iconIndex): Graphic(0, 320 entries incl. camera-RAW group "TIFF Graphics
file(.tif.tiff.epx.nef.arw)"), Document(1, 432), Email(2, 19), Audio(3, 281),
Video(4, 217), Other(5, 0 entries, `IsMutex="1"` fallback), Archive(6, 219),
Bookmarks(7, 1: `.bookmarks`). No size gates, no codec detection, no magic
overrides in the XML.

### A2. ENGINE-SIDE consumption (exFATSearchDLL — previously undocumented)

- Parse `FUN_1800098d0`-adjacent loader at :47102-47560: builds
  `<moduledir>FileFilter.xml` (sprintf "%sFileFilter.xml" :47165), TinyXml;
  processes ONLY FileCategory with iconIndex==0 (Graphic, :47279-47290) and
  ONLY FileType whose Description contains "TIFF" (:47324); collects
  dot-stripped _strupr extensions into global RAW set (:47428-47440).
- Classifier `FUN_180024fe0` (:30785-30960): given type idx (<0xAA=170) and
  name: (a) `strstr(name, ext)` over a **hardcoded 170-entry x9-byte dotless
  uppercase table** `DAT_18005a090` (:30824); (b) RAW-set membership
  (:30843-30854); (c) MOV/MP4/JPG prefix + JPG/JPEG memicmp specials
  (:30858-30920). SUBSTRING matching, not exact.
- **Lost-partition validation scorer `FUN_180025390` (:30973-31156)**: per
  file, data-start = `cluster_size*(start_cluster-2)+base` (:31057-31063),
  looked up in a signature map offset->type-id (`FUN_1800225a0` :28535-28566);
  counts name<->content matches; `ratio = matched/(considered-unmatched)`
  (:31150); **reject when `considered/2 <= failed && ratio <= 0.3 && failed
  != 0`** (:31152-31155). Called from >= 5 sites (:38364, :38665, :39688,
  :40096, :42733). FileFilter.xml therefore affects ENGINE scan decisions
  (lost-partition acceptance), not only display.

### A3. UI-side (DRWUI, 4 parse sites)

- `FUN_14031ee30` (:541281-541388): full 8-cat parse -> map iconIndex ->
  internal id (0->3, 1->7, 2->2, 3->4, 4->5, 5->8, default->9); values =
  dotless exts joined "%s;". Membership = exact-token `";"+ext+";"` strstr
  (:541417-541424). isGraphic :541476, isVideo :541492.
- `FUN_1403ed150` (:665106): Graphic/TIFF-only, EXCLUDES .tif/.tiff — the
  camera-RAW list.
- `FUN_140440340` (:714662): category-extension accessor for the advanced
  filter UI (ids 2,3,4,5,7).
- Site 4 (:827690): Graphic/TIFF-only global set (UI copy of the engine set).
- Advanced filter: `AdvanceFilterCondition` 0x48 bytes, **7 sub-condition
  groups: type/time/size/state/tag/path/search**
  (ui-resources/UIUX/31-filterbar-family-decode.md:122); pre-set filters
  push into the scan controller (PushFolderFilter, :142 of that doc).

### A4. Core side (include/cr/query/cr_query.h, src/core/query/cr_query.cpp)

6 categories (None/Image/Video/Audio/Document/Archive/Other, cr_query.h:32-40),
**64 extensions** (kMap :22-45 — PIPE-2's "57" is stale), exact-lowercase
semantics, post-scan query only; no tag/path predicate; lost-partition
validation is structure-only (cr_candidates.cpp:69-116).

## PART B — DiskHealth grading

### B1. Grading model (the previously-UNVERIFIED layer)

- **4 statuses: Unknown / Good / Caution / Bad** (status list :37087-37100).
- Per-disk threshold defaults (:37004-37019): field 0x1808 = 50 or 60 (by
  disk flag 0x1724 — INFERRED is-SSD); caution counts for attrs 5/197/198
  = 1; SSD-life caution = 10%.
- HDD bad-sector attrs 5/197/198 (`FUN_00458350` :33540-33582): value <
  vendor threshold -> Bad; raw >= caution threshold (default 1) -> Caution.
- Attr 1 read-error-rate (:33527-33551): value==0&&raw==0 or threshold<=value
  -> Good, else Bad.
- **SSD remaining-life attrs {169,173,177,187,202,209,230,231,232,233}**
  gated by drive-interface family (aggregation :5930-6130): 100 -> Good;
  >10 -> Good; <=10 -> Caution; 0 -> Bad (:6010-6016, :6076-6090).
- Vendor thresholds read via **SMART READ_THRESHOLDS 0xD1** (:14108-14118)
  — a required input to the Bad rule.
- NVMe transport: IOCTL 0x2D1400 + {ProtocolType 3, DataType 2, page 2,
  512 B} (:13351-13375) — **byte-parity identical to our cr_health.cpp:252-278**
  (positive MATCH record; data units x512,000 conversion :5600-5615).

### B2. Core side (src/win/cr_health.cpp)

- Transports: IOCTL_ATA_PASS_THROUGH 0xEC/0xB0-0xD0 + legacy 0x7C088
  fallback (:59-85, :172-187); **0xD1/0xD8 never issued**; 30x12 attr table
  (:195-240); `failing = (flags & 8) || ((flags & 1) && value==0)` (:207) —
  no 4-level grading, no vendor thresholds, no SSD-wear vendor mapping
  (231/233 unconditional :233-236).
- NVMe parser cr_nvme.cpp:23-39 — drops host_reads/host_writes/
  controller_busy_time/num_error_log_entries; data units unconverted;
  percentage_used read u16 (N-009a, known).
- TRIM: first-"= " match, `break` (:299) — code is og-parity but
  AUDIT-PIPELINE.md:218 and the :283-285 comment still describe last-wins.
