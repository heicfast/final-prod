# AUDIT REPORT — Session 3 (parity audit against the reference corpus)

Audit performed **before any fix** (mandate §2). Every verdict cites corpus evidence
(`research-corpus/…` paths) or app file:line. Evidence hierarchy: runtime capture
(dynamic ground truth + screenshots) > decompiled wiring > QSS census > doc synthesis.

Cross-references:
- Corpus atlas: `re-workspace/artifacts/UIUX/35-redesign-reference-atlas.md`
- Flow decode: `…/UIUX/04-userflow-state-machine.md`, `08-scan-phase-deep-dive.md`
- Runtime proof: `…/UIUX/dynamic-ground-truth--scan-phase-captured.md` (§3, §7, §8)
- Engine flow: `architecture/04-recovery-workflow.md`, `05-scan-engine-architecture.md`
- Design system: `…/UIUX/05-styling-theming-layout.md`, `21-qss-property-census.md`

Scope decisions carried from session 1 (documented in UI-MAP §0): the reference's
monetization/retention/license-gate surfaces (retention gates, buy/activate dialogs,
free-tier caps, ad rails, AI assistant upsell) are commercial-funnel surfaces and are
deliberately not re-implemented. Everything functional IS in scope.

---

## 1. Axis verdicts

| Axis | Verdict | Summary |
|---|---|---|
| Behavior | **MISMATCH** | Invented scan-configuration step between device selection and scan start (§2.1); stubbed recover-all; no-op Preview/Smart-select context items; fake stats. |
| Structure | **MISMATCH (partial)** | Screen inventory otherwise matches the reference IA (home/scan-live/results/recover/health/repair/settings); the invented `/scan/:deviceId` config page and its route must go; sidebar "Lost Partition" item is a redirect stub. |
| Layout | **MATCH (with gaps)** | 3-pane results layout, bottom status cluster, sidebar IA, quick-target trio, bottom-right CTA — all present and corpus-shaped. Gaps: live "Searching:" ticker sits at top (corpus pins it above the bottom cluster); icons view mode is a row-height change only (no tile renderer). |
| Visual parameters | **MISMATCH (by design this session)** | Visual identity now comes from the adopted theme tokens (ADR-007); tokens must be re-mapped from the two source files, light+dark+3 custom families, with an accessibility-adjusted mapping documented in VISUAL-SPEC. |
| States | **MISSING (partial)** | Missing: real "used time" in celebration; real searched-path ticker; select-all over the full filtered set (currently loaded-rows only); recover-all as a real job. All error/empty/permission/disconnect states exist. |
| Interaction patterns | **MISMATCH (partial)** | Selection/filter/sort/search/paging present. Context menu: 2 of 6 items are stubs (Preview no-op, Smart select = Recover). Filter bar lacks the corpus's "Existing files" / "Hide 0 KB" toggles and the Custom type-suffix entry. |
| Motion & feedback | **MATCH** | Duration/easing tokens, press/hover feedback, pulse for live states, `prefers-reduced-motion` kill-switch — all present; motion tokens re-source from the adopted theme ladder. |
| Accessibility | **NEEDS-WORK** | Keyboard paths and ARIA largely present; contrast must be re-measured against the NEW theme pairs; HC theme must be re-derived from the adopted tokens; the `EmptyState` screen-reader "Main content" span is noise; several wrong aria-labels (size inputs labeled with date keys). |
| Strings & localization | **MISMATCH (minor)** | en/zh-CN parity exists (398/398) but new surfaces (flow correction, corpus status-line formats, filter toggles, About dialog) add keys; several key bugs (size inputs use date keys; export toast says "copied"; repair result concatenates counts outside i18n). |
| Data flow | **MATCH (with defects)** | Connector isolation is clean (UI → queries → connector; ast-grep enforced). Defects: `useValidateDestination(scanId)` passes a scan id as `sourceDeviceId` → overlap refusal unreachable (recovery-page.tsx:75 + queries/system.ts:29); preview-kind inference (extension lists) lives in the UI — engine knowledge that belongs behind the connector. |
| Code quality | **NEEDS-REFACTOR** | 55-item slop list (§3): dead branches, duplicated formatters (hex rows ×2, duration ×2, cnStrip), dead store surfaces, unused connector commands, format-then-regex hack, unit-base inconsistency in `format.ts`. |
| Performance | **WITHIN** | Budgets green (main 89.7/200 KB gzip, total 315/500 KB; filter <200 ms measured in tests; virtualized list). Re-verify after changes. |
| Test coverage | **GAPS** | 56 unit + 2 e2e; no coverage for: direct-start flow, quick→advanced automatic transition, recover-all, smart select, icons view, theme snapshot, lost-path origin, destination source-id. |

---

## 2. Behavior findings (evidence per finding)

### 2.1 CRITICAL — Invented scan-configuration step (the reported divergence)

**Reference behavior (verified, three independent sources):**

1. Decompiled transition — `scanClickedSlot` (`FUN_140833a90`, artifact 04 §4):
   device tile scan click → `CScanMgr` prepared → stack flips directly to `CDetailPage`
   (index 3) → `FUN_1406bbfe0(detail)` **starts the scan immediately**. No intermediate
   screen; scan and results are the same page.
2. Runtime proof (dynamic ground truth, scan-phase capture): clicking
   "Search for Lost Data" at (922,662) lands **directly** in the scan-phase view
   (3-pane results layout already visible, gauge running). At t≈75 s the gauge read
   "Advanced Scanning, 9%" with 1.11 M files — the quick pass (~21 s, MFT walk) had
   already completed and advanced scanning had begun **without any user action**.
3. Engine wiring (artifact 25 §3.1): `sigQuickScanFinished(int) --×2-->
   onQuickScanCompleted(int)` — the two-phase gate; runtime log shows
   `bottomarea.cpp:838 m_iScanType=3` (quick) then `:856 m_iScanType=4` (advanced)
   back-to-back in one continuous 14.7-min run.
4. Absence proof: no scan-mode/config dialog exists in the 2,402 tr call sites
   (artifact 02 §5), 195 telemetry events (artifact 06 §2), 722 connect() edges
   (artifact 25), the 42-slot CDetailPage dispatcher (artifact 14), or the 1,710
   QSS targets (artifact 21/arch-UIUX §12).

The only quick→deep boundary surface is `CQuickScanCompleteWnd` — a Free-tier
upsell/coaching window ("Quick Scan Completed" / "You may now" / "Recover Found
Deleted %1 files %2" / "Tips: Advanced Scan is trying to find more other lost files
for you while recove[ring…]", artifact 08 §4; classified under Free-tier retention
surfaces in atlas §10). It is **not a gate** — advanced proceeds regardless — and it
was never caught on-screen in any dynamic run. As a retention/upsell surface it falls
under the documented out-of-scope decision (UI-MAP §0); the **automatic** transition
it coaches IS in scope and is implemented.

**App divergence:** `src/features/scan-configuration/scan-config-page.tsx` (526 lines)
inserts a configuration page between Home and the scan: a scan-mode radio group
(quick/deep/deep-carve — a "deep scan" choice), file-type category checkboxes,
sector-size select, bad-sector policy, retry count, and a scan-location radio.
Route `/scan/:deviceId` (router.tsx:31). None of these controls exist in the reference
between device selection and scan start. The folder-scope concept itself IS
reference-backed ("Specify a location" → Select Folder tile, atlas §5) — but it is
chosen on the landing page, not in an intermediate step.

**Fix (T6):** delete the ScanConfigPage and its route; Home's "Search for Lost Data"
CTA, per-volume links, and card scan buttons call `startScan` directly and navigate to
the live scan page. "Select Folder" opens the folder picker first, then starts.
Contract: `ScanParams` reduces to `{ targetId, pathScope? }` (mode/categories/
sectorSize/badSectorPolicy/retries removed — engine-internal per
architecture/04 §3: FS-type probe → quick searcher → full searcher set + RAW carver,
sector size default 512). Mock runs the full stage chain automatically
(volume-analysis → quick-index → deep-walk → carve → tree-build → done).

### 2.2 Recover-all is a stub

`recovery-page.tsx:58-63,343` — `?all=1` sends an **empty** `nodeIds` array; the mock
builds a 0-item job that "completes" instantly with a 0/0 report. The reference's
Recover All is a real export loop over the full result set (architecture/04 §7).
**Fix:** selection model gains an all-mode with count (wires `countResults`); recovery
runs a real bounded-lazy job over the whole space.

### 2.3 Context-menu stubs

- "Preview" item is a self-assignment no-op (`results-explorer.tsx:347`). Reference:
  opens the preview (modal in the reference; our architecture maps it to the preview
  pane + focus).
- "Smart select best files" runs the same handler as Recover (`:366`). Reference
  (atlas §8): magic-wand item selects the best files. **Fix:** implement best-per-name
  selection over the filtered set (confidence-ranked, non-existing preferred).

### 2.4 Fake/incorrect data displays

- Celebration "used time" is `formatDuration(((eta ?? 0) + 180) * 0)` → always
  00:00 (`scan-progress-page.tsx:349`). `ScanState.startedAt` exists — use it.
- Scan-start ISO effect contains dead math `(progress.overallProgress > 0 ? 1 : 0) * 0`
  (`:82`).
- Health page renders wear as a bare number without `%` (`health-page.tsx:188-191`).

### 2.5 Corpus status-line formats not matched

Reference (verbatim, dynamic): `Quick Scanning%1, Files Found:` /
`Advanced Scanning , Files Found: N (size)` / `Scan Completed, Files Found: N (size)` /
`Remaining time: 07m43s | Reading sector: X/Y` / full-width `Searching: <path>` strip
above the bottom cluster (atlas §6; GT-scan §2/§7). App shows a top banner with
generic stage labels and a different counter layout. **Fix:** move the live ticker to
the bottom cluster, use corpus label formats, add `currentPath` to
`ScanProgressEvent`, derive the sector readout from bytesScanned/512.

### 2.6 Destination validation source-id bug

`useValidateDestination(scanId)` passes the scan id where the connector expects the
source device id → the same-volume overlap refusal can never fire through the UI
(recovery-page.tsx:75; queries/system.ts:29-34). Reference: destination validation
opens the destination's raw volume and detects same-volume writes
(architecture/04 §7.3). **Fix:** pass the scan target id from the session store.

---

## 3. Code-quality findings (slop list)

Bugs / dead logic:
1. recovery-page.tsx:75 — scanId passed as sourceDeviceId (see §2.6).
2. recovery-page.tsx:105-110 — `estimatedBytes` depends on a jobId that cannot exist
   before recovery starts; always null.
3. recovery-page.tsx:58-63,343 — recover-all stub (see §2.2).
4. node-space.ts:239 — `Math.floor(rng() * 5)` over a 6-entry ORIGINS array:
   `lost-path` never generated; the "lost paths" tree group is permanently empty.
5. scan-progress-page.tsx:349 — fake used-time (see §2.4).
6. scan-progress-page.tsx:82 — dead math.
7. mock-connector.ts:239-243 — `listDevices` if/else with identical branches.
8. mock-connector.ts:188,211 — `const rng = mulberry32(…); … void rng;`.
9. mock-connector.ts:456 — impossible `typeof rt.target === 'string'` branch.
10. mock-connector.ts:1017 — `* 2.1) / 1` dead division.
11. mock-connector.ts:796,804 — `session-not-found` code with `errors.sessionCorrupt`
    reasonKey (mismatched semantics).
12. toaster.tsx:9 — `mode === 'high-contrast' ? resolved : resolved` dead ternary.
13. results-explorer.tsx:347 — Preview no-op (see §2.3).
14. results-explorer.tsx:366 — Smart select = Recover (see §2.3).
15. results-explorer.tsx:54-63 — `targetId` prop accepted then discarded.
16. shell.tsx:150-158 — About button fires a "learn more" toast (placeholder).
17. results-explorer.tsx:207-212 — breadcrumb count built by
    `t(...).replace(/ · .*$/, '')` format-then-strip hack.
18. scan-progress-page.tsx:399-401 — `cnStrip()` reimplements `cn()`.
19. hex-worker-client.ts vs hex.worker.ts — row-formatting algorithm duplicated.
20. preview-pane.tsx:59-63 — hex fallback duplicates the branch list it follows.
21. preview-pane.tsx:361-364 — inline mm:ss math duplicates `formatDuration`.
22. preview-pane.tsx:28-63 — extension→kind classification in the UI (engine
    knowledge; also overlaps fixtures' CATEGORY_BY_EXT).

Dead surfaces / unused exports:
23. ui-store.ts:36-38 — `togglePreviewPane`/`setPreviewPaneWidth` never called (no
    resizer exists; corpus has a QSplitter → implement the resizer).
24. filter-store.ts — confidence/extension filter state has no UI (corpus filter bar
    has Custom type suffix entry → wire extensions; confidence filter is not a
    reference surface → remove).
25. scan-session-store.ts:41 — `clearActive` never called.
26. selection-store.ts:38-43 — `remove` unused.
27. contract countResults — implemented, never consumed (wire via all-mode counts).
28. contract deleteSession — never consumed, not reference-evidenced (remove).
29. contract openPath — mock no-op + menu item disabled (keep contract for the real
    connector; disabled item gets a truthful reason).
30. virtual-file-list.tsx:88-110 — `onVisibleRangeChange`/`viewportRef` dead props.
31. error-boundary.tsx:39 — `ErrorScreen.onRetry` never supplied.
32. i18n/index.ts:45 — `export { i18next }` unused.
33. queries/dev.ts:22-23 — dead `const qc = useQueryClient(); void qc;`.
34. queries/scan.ts:108-109 — pointless `useRef(() => qc)` indirection.

Inconsistent patterns / typing:
35. recovery-page.tsx:561 — `state: string` instead of the item-state union.
36. preview-pane.tsx:178 — untyped error cast.
37. connector/index.ts:38 — `as unknown as MockConnector` (documented, keep with a
    type guard).
38. scan-progress-page.tsx:299 — raw `['sessions']` key bypasses the keys factory.
39. home-page.tsx:45-47 — manual hash parse instead of `useSearchParams`.
40. format.ts:21/37 — 1000-based vs 1024-based byte steps in one file (corpus shows
    binary GiB-scale values labeled GB, e.g. 147.12 GB found on a 149.45 GB volume —
    unify on 1024).
41. filter-bar.tsx:208,218 — size inputs labeled with date i18n keys.
42. filter-bar.tsx — raw native checkboxes instead of the shared Checkbox component.
43. repair-page.tsx:163,190 — i18n string concatenation.
44. recovery-page.tsx:545 — export success toast says "copied".
45. shell.tsx:196-202 — Settings link rendered twice in results flow.
46. empty-state.tsx:58 — sr-only "Main content" span on every empty state.
47. recovery-page.tsx:621-626 — conflicting `border-transparent border-border` classes.
48. mock-connector.ts:1350-1355 — over-abstracted `ReturnType<…>` conditional type.
49. recovery-page.tsx:373,497 — silent caps (200 queue rows / 20 failed) with no
    affordance.
50. results-explorer.tsx:222-224 — icons view changes row height only (no renderer).
51. mock-connector.ts:991 — recovery failure reuses a preview error code/reasonKey.
52. select-all semantics select loaded rows only while aria-rowcount declares the
    filtered total.
53. Recycle Bin / Desktop quick targets fire "learn more" toasts (dead targets; the
    reference scans them — atlas §5/GT-landing §4).
54. Home page: `showRemovable` via manual URL parsing (see 39).
55. Stepper component exists but is unused-by-surface (verify after flow fix: the
    reference has no wizard stepper between selection and scan — remove if dead).

---

## 4. Per-surface verdicts

| Surface | Behavior | Structure | States | Notes / fix |
|---|---|---|---|---|
| Home / device selection | MISMATCH | MATCH | MATCH | Start scan directly (§2.1); wire quick targets (§3.53); useSearchParams. |
| Scan configuration page | **MISSING from reference** | — | — | Delete (§2.1). |
| Scan progress (live) | MISMATCH | MATCH (gaps) | MATCH | Corpus status formats + bottom ticker (§2.5); real used-time; keep pause/stop/celebration. |
| Results explorer | MISMATCH (partial) | MATCH | GAPS | Context menu stubs (§2.3); icons view (§3.50); select-all total (§3.52); search breadcrumb+chip; filter toggles + Custom suffix. |
| Filter bar | MISMATCH (partial) | MATCH | MATCH | Add Existing/Hide-0KB toggles + Custom type entry; fix labels (§3.41); Checkbox component. |
| Preview pane | MATCH | MATCH | MATCH | Move kind inference to connector (`requestPreview(scanId, nodeId)`); dedupe formatters. |
| Recovery (destination → queue → report) | MISMATCH | MATCH | GAPS | Source-id bug (§2.6); recover-all (§2.2); caps affordance; paged/virtualized queue. |
| Sessions | MATCH | MATCH | MATCH | reasonKey fix; remove deleteSession; keep save/load/resume. |
| Health | MATCH | MATCH | GAPS | wear %; never-'bad' status unreachable → add honest 'bad' scenario coverage in mock. |
| Repair | MATCH | MATCH | MATCH | i18n concat fix. |
| Settings / About | GAPS | MATCH | MATCH | About dialog (real); zoom control (reference DPI menu 100/150/200); diagnostics stay. |
| Shell / sidebar | MATCH | MATCH | MATCH | Duplicate Settings link; About placeholder; theme system re-map (ADR-007). |
| Dev panel | MATCH | — | MATCH | Scenario set updates with contract changes. |

---

## 5. Theme adoption audit (ADR-007 summary)

Adopted sources (fetched, immutable in-repo copies under `src/theme/adopted/`):
`themes.ts` (5 palettes: light/dark/ember/tide/blossom; two-layer `data-theme` +
`data-scheme` attribute system) and `tokens.css` (surfaces/text/control/brand/status/
pastels/age/shape/motion/spacing/focus/elevation/typography/layout metrics; dark-family
layer; per-theme blocks). The app's semantic token layer maps onto these; structural
geometry the corpus pins (window sizes, title-bar ≈35 px, sidebar IA) stays
reference-derived. High-contrast is not defined by the sources → derived from the
tokens and documented. Accessibility-driven mapping choices (measured pairs, the
brand CTA exception, HC alternatives) are recorded in VISUAL-SPEC §A11. A snapshot
test fails if the adopted token surface drifts.

---

## 6. Fix order (tracked as T5–T20)

1. T5 theme adoption + snapshot test (visual identity foundation for everything after).
2. T6 lost-data flow correction + contract slimming + mock improvements (architecture-
   derived) + automated coverage.
3. T7–T9 waves: foundation components → feature surfaces → remaining surfaces (all
   §3 items).
4. T10 state completion; T11 dead-code/architecture enforcement; T12 localization;
   T13 accessibility; T14–T15 DPI/perf; T16 CI green; T17 acceptance grid; T18 docs;
   T19 release; T20 final sweep.
