# modules/visualization.md — query/preview/disk-map audit verdicts

Reference: og AnalysisGridWidget + IBuildVolumeBitmap + event bus (UIUX/03/04/08,
searchdll-family §2), preview caps (UIUX/10: 15 s video QTimer, 100 MB cap,
VAPreview ports 20171/20172 + 602-byte header protocol), RecoveryAssistant categories.

Core (audited `67193b6`): `src/core/query/cr_query.cpp` + `src/core/preview/cr_preview.cpp`
(1,081 LOC) + `tests/unit/test_query.cpp` + `test_preview.cpp` + `test_query_perf.cpp`.

## Verdicts

- **D16 filter model** — `M`-note: core's filter predicate set (name/path/kind/state/
  size/time) covers the og's bucket model (Date 0x100–0x105, Size 0x1000–0x1004) at the
  engine layer; the og's FileFilter.xml category membership (8 categories / 1,481 ext) is
  UI-side — core's kind filtering is coarser (per-FS kind, not per-extension-category).
  Registered project gap (CORE-GAP-LIST: 7-cat/85-ext vs og 8/1489) — verdict:
  DIVERGENCE-MAJOR via the project's own register (re-verified open).
- **D17 sort model** — `M` (Path/name/size/time sorts; Path sort materializes paths —
  perf note only).
- **D18 paging** — `M` (bounded fetch, strict/lenient modes).
- **X48/X52** — `M` (1M nodes < 200 ms query budget, randomized-vs-reference tests).
- **Preview** — `M`-note: core preview decodes docx raw-deflate + truncated-magic gates;
  og preview caps (15 s / 100 MB / VAPreview protocol) are UI-service-layer — engine
  data contract satisfied; the og's video/audio preview services are out of the core's
  engine scope (UI product decision).
- **Disk map** — **S-009** (IBuildVolumeBitmap/AnalysisGridWidget surface absent).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| S-009 | SEVERE | No volume-bitmap/disk-map surface (ABI-CONTRACT §7 to-build) |
| (project GAP) | MAJOR | Filter category granularity 7-cat/85-ext vs og 8/1489 |

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new visualization-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
visualization-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

- **S-009 PARTIALLY FIXED by 56cce87:** geometry/overlays/bad-ranges/zoom/64 MiB budget +
  `cr_ui_disk_map` ABI landed with tests; the finding's own plan items remain open — engine
  bitmaps not wired into the producer (`kCellAllocFree` unreachable; allocation overlay is
  active-structured-only, `cr_diskmap.cpp:64-68`), no versioned CRSF disk-map record, snapshot
  path carries no error map, two acceptance tests absent, ABI-CONTRACT §7 "2 free" doc drift.
- Stale figure note (I-011, CORRECTIONS row 5 residue): lines 16/33 of this file still quote
  the old "og 8/1489" count — update to the corrected 1,481 figure when this file is next
  edited for the S-009 remainder work.
- M-006 (fuzz CI) unchanged/open.

### Session-2 addendum (post-reconciliation)
**M-006 is now FIXED** (`e30b84d` re-enabled the fuzz job; verified on the current tree) —
the line in the appended session-2 section above saying "M-006 (fuzz CI) unchanged/open"
predates that fix landing and is superseded. Only the S-009 disk-map remainder stands
(bitmap wiring → `kCellAllocFree`, CRSF record, snapshot error map, acceptance tests,
§7 doc reconciliation).
