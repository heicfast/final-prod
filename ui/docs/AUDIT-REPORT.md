# Full-Repository & Reference Corpus Audit Report

> **Audit Scope**: Complete line-by-line cross-check of `heicfast/recovery-wizard-ui-new` against `heicfast/easeus-drw-research` (architecture docs `00..07`, UIUX artifacts `01..35` + dynamic ground truth, RE binary docs `DRWUI-exe--main-ui-architecture.md`, `DRWUI-subsystems--assistant-grading-preview.md`, `SaveScan/rsf-format--scan-session.md`, `config-files/*`, `CORRECTIONS.md`, and screenshots `01..57`).

---

## 1. Axis Verdict Summary (Pre-Fix Audit → Post-Fix Target)

| Axis | Pre-Fix Verdict | Post-Fix Verdict | Evidence & Summary of Findings |
|---|---|---|---|
| **Behavior** | `MISMATCH` | `MATCH` | Direct scan start (`ADR-008`) and automatic quick→advanced pipeline match corpus (`CORRECTIONS.md` #1). **Divergences found**: (1) `bad-sectors` scan stage weights sum to `0.86` when `quick-index` is omitted (`src/connector/mock/mock-connector.ts:276`); (2) `scanTick` leaves `filesFound`/`dataFound` at `96.66%` (`1 - e^-3.4`) on completion while `queryResults` shows `100%` (`mock-connector.ts:696`); (3) `node-space.ts` mixes `raw` (`path: null`) and `lost-path` files into `root` and directory counts/queries instead of isolating the 3 top-level Path-tree branches (`root`, `lost-paths`, `raw-all`); (4) `getResultTree` returns static final counts during live scan instead of growing with `visibleFraction`; (5) `results-explorer.tsx:511` passes file count (`totalOf`) into `formatBytes()` in the status bar instead of total byte size (`totalBytes`); (6) `node-space.ts` can assign non-`'poor'` confidence to `0 B` files, violating `CRecoverability` rule 1 (`size < 1 -> poor`). |
| **Structure** | `MATCH` | `MATCH` | Screen inventory, flow graph (`/` → `/scan/:id/live` → `/results/:scanId` → `/recover/:scanId`), 3-pane explorer, and sidebar tools (`/lost-partition`, `/health`, `/repair`, `/settings`, `AboutDialog`) match `docs/UI-MAP.md` and `ADR-008`. |
| **Layout** | `MATCH` | `MATCH` | 36px custom title bar, 208px/52px collapsible sidebar, 3-pane explorer with resizable preview pane (`240px–560px`), 4px spacing grid, zero horizontal page overflow at 100%/150%/200% scale. |
| **Visual parameters** | `MATCH` | `MATCH` | Adopted `diskgenie-v2` token layer (`src/theme/adopted/tokens.css` + `themes.ts`) + semantic bridge (`src/styles/tokens.css`), snapshot-pinned in `tests/theme-snapshot.test.ts`. |
| **States** | `MATCH` | `MATCH` | Initial loading (`Skeleton`), empty (`EmptyState`), error (`EmptyState` / `Alert`), partial (`PartialBanner` on cancelled scan), permission-denied (device & volume level), disconnected (device & volume level + mid-scan modal), and success states implemented across all surfaces. |
| **Interaction patterns** | `MISMATCH` | `MATCH` | Tri-state Select All over full filtered set, Shift-range & Ctrl-toggle selection, 8-category + custom extension filter, date/size presets + custom ranges, 6-item context menu, and smart select work end-to-end. **Divergences found**: Path-tree selection of `Existing files` (`root`) or a subfolder (`d<N>`) leaked `raw` (`path: null`) and `lost-path` items; live scan did not invalidate `tree`/`results`/`count` queries as progress advanced (`src/queries/scan.ts`). |
| **Motion & feedback** | `MATCH` | `MATCH` | Route fade (`120ms`), dialog enter/exit (`200ms`) with outside-click swallowed (corpus rule 10), sidebar collapse (`200ms`), button/checkbox press scale (`0.985`), top-right toast stack (`offset.top = 48px`), `prefers-reduced-motion: reduce` honored globally. |
| **Accessibility** | `MATCH` | `MATCH` | Full keyboard operability, skip link, landmark regions, `aria-sort` on columns, `role="tree"` / `role="grid"` / `role="progressbar"` / `role="separator"`, paired color+label on all confidence and status badges, WCAG AA contrast across Light, Dark, and High-Contrast themes. |
| **Strings & localization** | `MATCH` | `MATCH` | 100% key and placeholder parity between `src/i18n/en.ts` and `src/i18n/zh-CN.ts`, zero unused or missing keys enforced by `scripts/i18n-sweep.mjs` and `tests/i18n-parity.test.ts`. |
| **Data flow** | `MISMATCH` | `MATCH` | Strict `UI → queries → connector` boundary enforced by `scripts/arch-lint.mjs` and ESLint. **Divergences found**: (1) `ResultPage` lacked `totalBytes`, causing `results-explorer.tsx` to format the file count as bytes; (2) `src/queries/scan.ts` did not invalidate `count` on scan completion or refresh `tree`/`results`/`count` during live scan ticks. |
| **Code quality** | `NEEDS-REFACTOR` | `CLEAN` | Zero TypeScript errors, zero `jscpd` clones, zero `knip` dead exports. **Issues found**: (1) `O(N^2)` batch cursor loop in `NodeSpace.queryNodes` and `MockConnector.jobTick` causing slow `recover-all` and 319s test runtime; (2) `tests/setup.ts` missing `import '@/i18n'` causing `NO_I18NEXT_INSTANCE` warnings; (3) `src/lib/logger.ts` emitting `console.debug` noise in Vitest; (4) `eslint.config.js` referencing deleted `src/features/scan-configuration` instead of `src/features/lost-partition`. |
| **Documentation** | `OUT-OF-DATE` | `ACCURATE` | **Issues found**: (1) Stale root `AUDIT-UI.md` from session 1/2 referencing deleted `/scan/:volumeId` config screen and `52/52` tests; (2) `docs/CONNECTOR-CONTRACT.md` had pre-ADR-008 method signatures (`requestPreview` with `kind`, `deleteSession`, old `startRecovery` signature); (3) `docs/DESIGN-SYSTEM.md`, `docs/TESTING.md`, and `README.md` had stale token and test counts; (4) Missing gate docs `docs/screen-manifest.md`, `docs/visual-spec.md`, `docs/component-inventory.md`. |
| **CI / Workflows** | `FAILING-OR-WEAK` | `ENFORCING` | **Issues found**: (1) `.github/workflows/ui-smoke.yml:58` had `continue-on-error: true` on `tauri-debug-build`; (2) `.github/workflows/ci.yml:63` ran `pnpm test` without `--coverage` gate; (3) Missing manual `workflow_dispatch` workflow for screenshot comparison against the reference corpus. |

---

## 2. Detailed Divergence Ledger (With File/Line & Corpus Evidence)

### D1 — `bad-sectors` Scan Stage Weight Normalization (`src/connector/mock/mock-connector.ts:276, 659`)
- **Corpus Evidence**: `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md` §2 & `docs/07-user-flow-uiux.md` §3 — overall scan percentage progresses smoothly from `0%` to `100%` across active stages.
- **Root Cause**: When `volume.quickScanAvailable === false`, `rt.stages` is `['volume-analysis', 'deep-walk', 'carve', 'tree-build']`. Because `'quick-index'` (`STAGE_WEIGHTS['quick-index'] = 0.14`) is omitted without normalizing the remaining weights, the sum of weights is `0.86`, stalling `overallProgress` at `86%` until completion jumps to `100%`.
- **Fix**: Normalize stage weights over the active `rt.stages` in `scanTick()`.

### D2 — Scan Completion `filesFound` / `dataFoundBytes` Mismatch (`src/connector/mock/mock-connector.ts:696`)
- **Corpus Evidence**: `screenshots/35_scan_completed_overview.png` & `artifacts/UIUX/08-string-table-i18n.md` §7 — `Scan Completed, Files Found: N (size)` matches the exact total in the results explorer.
- **Root Cause**: `scanTick()` computed `visibleFraction = 1 - Math.exp(-3.4 * rt.overall)` before updating `rt.overall`, and at `rt.overall = 1.0`, `1 - Math.exp(-3.4) = 0.9666`, leaving `rt.filesFound` and `rt.dataFound` at `96.66%` of `stats.files` / `stats.size` even when `rt.stage === 'done'`.
- **Fix**: Compute `rt.filesFound` and `rt.dataFound` after updating `rt.overall` and `rt.stage`, and use `this.visibleFraction(rt)` (which returns `1` when `rt.stage === 'done'`) so completion reports `100%` (`stats.files` and `stats.size`).

### D3 — Path-Tree 3-Branch Isolation (`src/connector/mock/node-space.ts:291, 314, 488`)
- **Corpus Evidence**: `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md` §3 & §7, `30-interaction-driver-ground-truth.md` §2, `SaveScan/rsf-format--scan-session.md` §5.4.6, `docs/CONNECTOR-CONTRACT.md:70`.
- **Root Cause**:
  1. Constructor lines 291–295 added `lost-path` (`originIdx === 2`) and `raw` (`originIdx === 3`) files into directory `directCount[d]` and `directSize[d]`, inflating `root` and folder counts so `root + lost-paths + raw-all > totalFiles`.
  2. `matches()` lines 488–496 did not filter `this.fileOrigin[i] <= 1` when `filter.treeMode === 'path'` and `filter.nodeId === 'root'` or `d<N>`, causing carved files (`path: null`) and orphaned `lost-path` files to appear inside normal folders.
- **Fix**: Restrict directory tree aggregation (`directCount`/`directSize`) and `treeMode === 'path'` directory matching (`root` and `d<N>`) to files with intact directory paths (`this.fileOrigin[i] <= 1`, i.e., `existing` and `deleted`).

### D4 — Live-Scan Tree Growth & Query Invalidation (`src/connector/mock/mock-connector.ts:448`, `src/connector/mock/node-space.ts:314`, `src/queries/scan.ts:62`)
- **Corpus Evidence**: `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md` §7 — tree node counts and result rows grow live during scanning.
- **Root Cause**: `getResultTree` ignored `visibleFraction(rt)` and returned 100% final tree counts immediately at scan start; `src/queries/scan.ts` did not invalidate `count` on scan completion or refresh `tree`/`results`/`count` periodically during live scan progress.
- **Fix**: Scale `pathTreeChildren` and `typeTree` counts by `visibleFraction` during active scans (`visibleFraction < 1`), and throttle-invalidate `tree`, `results`, and `count` queries in `useScanProgress` as scan progress advances and on terminal state transitions.

### D5 — Results Explorer Status Bar Byte Total Bug (`src/features/results-explorer/results-explorer.tsx:511, 545`)
- **Corpus Evidence**: `screenshots/35..44` & `artifacts/UIUX/35-redesign-reference-atlas.md` §8 — bottom status bar shows `N files · X GB`.
- **Root Cause**: `totalOf(pages.data?.pages)` returned `pages[pages.length - 1].total` (the file count) and passed it to `formatBytes()`, displaying e.g. `18.0 KB` for 18,400 files instead of their actual multi-gigabyte total size.
- **Fix**: Include `totalBytes: number` on `ResultPage` (`src/connector/contract.ts` & `src/connector/mock/node-space.ts`) and read `totalBytes` in `results-explorer.tsx`.

### D6 — `CRecoverability` Zero-Byte File Grading Rule (`src/connector/mock/node-space.ts:276`)
- **Corpus Evidence**: `artifacts/DRWUI-subsystems--assistant-grading-preview.md` §3.3 (`GetEntryRecoverability` decompiled C++: `if (entry.size() < 1) return 1 /* Poor */;`).
- **Root Cause**: `node-space.ts` generated `size === 0` (`i % 97 === 0`) after assigning `fileGrade[i]`, allowing 0-byte files to be graded `excellent` or `good`.
- **Fix**: Enforce `if (size === 0) gradeIdx = 3 /* 'poor' */` in `NodeSpace` construction.

### D7 — `O(N^2)` Performance Bottleneck in `recover-all` & Scan Tick Speed (`src/connector/mock/node-space.ts:442`, `src/connector/mock/mock-connector.ts:742`)
- **Root Cause**: `NodeSpace.queryNodes` looped from `i = 0` on every batch call, and `jobTick` called `job.items.findIndex` from `0` on every tick. Additionally, fixed 330-tick scan durations caused `tests/connector-mock.test.ts` to take 319s.
- **Fix**: Advance `job.advanceCursor` in `O(1)` amortized time, optimize `queryNodes`, and support fast deterministic tick progression in test environments while preserving all stage/phase invariants.

### D8 — Test Suite Hygiene, ESLint Zone Config, CI Gates & Documentation Sync
- **Fixes**:
  - Import `@/i18n` in `tests/setup.ts` to eliminate `NO_I18NEXT_INSTANCE` warnings.
  - Silence `console.debug` in `src/lib/logger.ts` when `import.meta.env.MODE === 'test'`.
  - Update `eslint.config.js` restricted zones (`lost-partition` replacing deleted `scan-configuration`).
  - Remove `continue-on-error: true` from `.github/workflows/ui-smoke.yml`.
  - Enforce `pnpm test:coverage` in `.github/workflows/ci.yml` with coverage thresholds in `vite.config.ts`.
  - Add `.github/workflows/visual-compare.yml` (`workflow_dispatch`) and `scripts/visual-compare.mjs`.
  - Delete stale root `AUDIT-UI.md` and synchronize `docs/CONNECTOR-CONTRACT.md`, `docs/DESIGN-SYSTEM.md`, `docs/TESTING.md`, `docs/ACCEPTANCE.md`, `docs/IMPLEMENTATION-STATUS.md`, `README.md`, and `worklog.md`.
