# Worklog — recovery-wizard-ui-new

Multi-session work log. Append-only; newest at the bottom. Format per SOP:
what / why / evidence / next.

---

## Session 1 — 2026-10-23 (UI Mock session)

### T1 — Corpus pull, repo skeleton, governance docs — DONE

- Cloned research corpus `easeus-drw-research` (1.7 GB, 11,495 files) to
  `/home/z/my-project/easeus-drw-research` (read-only reference; never integrated).
- Verified toolchain: Node 24.21, npm 11.19, git 2.47, Python 3.12 + PIL (icon generation).
  No cargo locally — Rust side compiles in Windows CI only; Node side builds locally.
- Created private GitHub repo `heicfast/recovery-wizard-ui-new` (default branch `main`).
- Local skeleton at `/home/z/my-project/recovery-wizard-ui-new/`: `docs/`, `docs/adr/`,
  `worklog/`, `scripts/`. Remote authenticated via token stored in `.git/config` only.
- Saved redacted session prompt to `worklog/session-ui-mock-prompt.md`.

### Research extraction (reading pass, evidence → docs later)

Read, in full or in targeted depth:

- `architecture/00-INDEX.md` — master index; scale of extraction.
- `architecture/07-user-flow-uiux.md` — screen-by-screen flows, scan modes, results/preview/
  recovery/repair surfaces, 4-page stacked wizard (QStackedWidget: Home / LostDisk /
  PeripheralHome / DetailPage — scan runs INSIDE the results page).
- `architecture/05-uiux-architecture.md` — design system facts: brand blue #0560fd light /
  #5094ff dark; CTA gradients #0c99ff→#0560fd (light) / #059afd→#0ccdff (dark); success
  #30b373; warn #f72c2c / #ffc10e; 4-step neutral ladder #e7e7e7/#dee1ed/#c1ccdf/#a0a6b1;
  shadows rgba(4,5,43,.5) light / rgba(28,28,28,.5) dark; Segoe UI 14px base + zh/ja/ko CJK
  stacks; window 1240×840 normal / 800×600 min, opens maximized; dialog 540×240; icon
  standards 48/56/72/80; 118-slot dual palette (light/dark) hot-swappable; dark surfaces
  #1b1d1f/#202428/#32383e, text #f1f5ff, orange accent #ff7342, borders #404c63/#596680.
- `artifacts/UIUX/35-redesign-reference-atlas.md` — THE rebuild reference: title-bar strip,
  landing map (sidebar sections Data Recovery / Advanced Features / File Backup; device tiles
  with capacity bars; Specify-a-location trio; Search CTA bottom-right), scan-phase controls
  (circular % gauge bottom-left, pause/stop, Searching strip, Recover All CTA bottom-right),
  celebration modal (stats grid + Go to Recover), results 3-pane map (toolbar nav cluster +
  breadcrumb + view toggle + filter + search; filter bar row; select-all + sortable columns;
  bottom status + contextual CTA), context menu 6 items, filter dropdown option sets
  (Type/Date/Size/custom + toggles), destination dialog anatomy (Local/Cloud tabs, drive rows
  with space stats, Choose folder, repair toggle ON default, Save disabled until selection,
  overwrite warning, insufficient-space gate), preview surfaces, 15 dynamically-proven
  interaction rules.
- `artifacts/UIUX/05-styling-theming-layout.md` — QSS/theming layer facts (three-layer style
  system, tooltip spec, scrollbar specs, page margins, data-driven configs:
  PriorityDisplayType.xml, usefullostfiles.xml, RawConfig.ini per-extension carve flags).
- `architecture/04-recovery-workflow.md` — engine-side flow: scan setup factories (FS codes),
  quick vs advanced (full searcher set + RAW carver), tree building → classify (FileFilter.xml
  8 categories / 1,489 extensions) → check → filter → sort; preview subsystem; CRecoveryMgr
  export chain with destination validation (raw volume open, same-volume detection); .rsf
  session persistence (zlib-chunked records: flags → volume name + timestamp → scan metadata
  → full tree); failure paths.
- `artifacts/UIUX/19-diskhealth-quick-ui.md` — SMART table shape: ID / Attribute / Current /
  Worst / Threshold / Raw / Status + health states Good/Caution; disk selector + caution card.
- `artifacts/PartitionRecovery/rawfixer-fs-repair.md` — transactional repair model: Check*
  probes → FixDBR / FixFromDBR / FixByScanSector → UpdateDBR → verify → Undo; per-FS fixer
  matrices (NTFS/FAT/exFAT); CopySectors undo snapshots.

Screenshot corpus inventoried: 63 files across 7 areas (splash 2, landing 21, scan-phase 7,
scan-complete 2, results 14, preview 2, recovery 15). VLM extraction next (T2).

### Decisions (pre-ADR, to be formalized)

- Router: React Router v7 data mode, `createHashRouter` (hash routing works identically in
  WebView2 custom protocol, `tauri://localhost`, and browser dev/preview) — ADR-001.
- Preview rendering boundary: connector returns bounded rendered page bitmaps for PDF/Office/
  AV thumbnails; UI decodes raw bytes only for image/hex/text (bounded) — ADR-003.
- Mock data strategy: procedural deterministic node space + typed-array aggregate index for
  1.2M nodes (filters scan typed arrays, never materialize full objects) — ADR-004.
- Skip the reference's monetization/retention/license-gate surfaces: they are commercial
  funnel specific to the reference product, not part of a truthful recovery-tool information
  architecture. Documented in UI-MAP as deliberate divergence.
- Product identity: neutral "Recovery Wizard" branding, original strings, original icons
  (Lucide interim base, documented in INTERIM-ASSETS.md).

### Next

T2 — VLM extraction across all 63 screenshots → SCREENSHOT-MANIFEST.md + VISUAL-SPEC.md draft;
T3 QSS cross-check; T4 UI-MAP.md; push gate docs.

---

## Session 1 continued — 2026-10-24 (build → verification → handoff)

### T5–T8 — Scaffold, tokens, primitives, connector (DONE)

- Versions verified live; ADR-005 records the three spec/registry conflicts
  (Vite 7 nonexistent → 8.3.5; TS 7 exists → kept spec-named 5.9.3; RR8/Vitest5
  exist → kept spec-named 7.18.4/4.1.12; RHF beta → 7.91.0; 3 packages repinned
  past the 24 h minimumReleaseAge policy).
- pnpm 12: build approvals via `allowBuilds` map in pnpm-workspace.yaml.
- Tailwind v4 four-step architecture (tokens.css → @theme inline → base).
- Connector: contract.ts (9 domains), mock (mulberry32-seeded; NodeSpace
  typed-array index for 1.2 M nodes with two-pass subtree aggregates),
  tauri stub, factory.

### T9–T17 — All feature surfaces (DONE)

Home (device cards, quick targets, session resume), ScanConfig (staged modes,
8 categories, advanced policies, zero-fill warning), ScanProgress (live 3-pane
results, gauge cluster, pause/stop/celebration, device-lost prompt), Results
(toolbar, filter bar, lazy tree, virtualized list, context menu, contextual
CTA), Preview (bounded kinds, worker hex, diagnostics), Recovery (destination
validation with hard overlap refusal, queue, report export), Health (SMART
table), Repair (REPAIR-typed consent, undo log), Settings (theme/density/
locale, diagnostics, about).

### T19 — Tests, enforcement, CI (DONE)

- 52 Vitest tests green (contract timing budgets asserted: cancel ≤300 ms,
  pause/resume <600 ms, filter <200 ms; determinism; injections; overlap
  refusal; repair consent round-trip).
- eslint recommendedTypeChecked + react-hooks v7 (compiler rules): 0 errors;
  external-store pattern (useSyncExternalStore) for progress streams.
- arch-lint + ast-grep + eslint zones: connector boundary clean (A1–A5).
- CI/ui-smoke/release workflows complete (windows-latest).

### Live verification (agent-browser, evidence screenshots verify-01..08)

Full golden path PASS with 0 page errors: home → select → config → scan →
completion modal → results (filter, tree, 200-row pages) → row select →
preview pane → recover → destination → job → report. Two real defects found
and fixed during verification:

1. Row checkbox double-toggle (span wrapper + Radix onCheckedChange both
   toggled) — root-caused via Radix source read; fixed in virtual-file-list.
2. RadioGroupItem used outside RadioGroup in DestinationRow — surfaced by
   the error boundary; replaced with a self-contained radio indicator.
   Also fixed: query-client thunk invocation in the scan progress store;
   one-shot disconnect injection; completed-scan sidebar cleanup.

### T20 — Docs, acceptance, handoff (DONE)

- docs/: CONNECTOR-CONTRACT, ARCHITECTURE, adr/001–005, TESTING, ROADMAP,
  INTERIM-ASSETS, VISUAL-ACCEPTANCE (55-grid), README, AUDIT-UI.
- Budget gates PASS: main 86/200 KB gzip, total 301/500 KB.

### Handoff — next session

Wire the real engine: implement RecoveryConnector in connector/tauri over
invoke()/emit, add Rust commands mirroring the contract, keep every UI file
untouched (factory swap via VITE_CONNECTOR=tauri). See docs/ROADMAP.md for
the full ordered list, including re-measuring the PARTIAL grid items on
Windows hardware.

---

## Session 2 — 2026-10-24 (CI red → green: full re-verification)

User report: release failing, all CI failing, "looks like incomplete fast
implementation — give more time and attention, verify all again."

### Diagnosis (from downloaded Actions logs)

Five failed runs, four distinct root causes:

1. **CI Format check**: Windows runners default `core.autocrlf=true` → the
   checkout rewrites every text file to CRLF → Prettier (endOfLine=lf) failed
   on 121 files at once. Typecheck/lint had PASSED before it; everything after
   (arch-lint, ast-grep, knip, jscpd, tests, build, cargo) never ran — the
   "all checks failing" impression was mostly this cascade.
2. **UI Smoke tauri-debug-build**: `tauri build --debug --bundles none` is
   invalid CLI (possible values msi/nsis) → `--no-bundle`.
3. **UI Smoke smoke**: Playwright strict-mode violation —
   `getByText(/internal drives/i)` matched both the sidebar nav item and the
   section heading → role-scoped locator.
4. **Release**: tag `v0.1.0-ui-mock` vs app version `0.1.0` mismatch → all
   three manifests bumped to `0.1.0-ui-mock` (the task-brief tag).
5. **knip would have failed anyway**: never ran, but found 60+ findings when
   run locally (dead code, misconfigured project globs).

### Fixes and completion work

- `.gitattributes` (`* text=auto eol=lf`, CRLF for .bat/.cmd/.ps1, binary
  marks) — makes Windows checkouts byte-stable.
- Completed the dangling `ScanParams.pathScope` feature (contract field was
  added but nothing consumed it):
  - NodeSpace: `resolveScope` (drive-prefix/case/separator normalization),
    per-scan Uint8Array dir mask threaded through query/count/trees/aggregates
    (state kept in ScanRuntime — the space instance is shared per volume);
    scoped path-tree presents the scanned folder as root; `suggestedScopePath`.
  - Mock connector: `scan-scope-invalid` hard error with reason key; scope-aware
    found-file curve via scoped stats; `pickDirectory(hint?)` roots at a volume
    (contract change, documented) and returns a folder that actually resolves.
  - UI: "Scan location" section in scan-config (whole drive vs folder, input +
    Browse + hint + disabled-Start guard); Home "Choose a folder…" passes
    `?folder=1`; i18n keys en/zh-CN (parity 398/398).
  - 3 new contract tests (invalid scope error; absolute+relative forms list
    only in-scope files; path-tree root) — suite now 55/55.
- **Real bug found & fixed (pre-existing)**: `/scan/:volumeId` routes (device-card
  volume links, choose-folder) failed with "device not found" — mock
  `getDeviceInfo` only resolved device ids. Now resolves volume → owning
  device, matching startScan semantics; contract doc updated.
- **Real bug found & fixed (pre-existing, dev-only)**: DevPanel rendered
  unconditionally — its Close button toggled a store flag nothing consumed.
  Panel now gated on `devPanelOpen`.
- Accessibility upgrade: the three scan-progress modals (stop confirm,
  celebration, device-lost) were hand-rolled `role="dialog"` divs without
  focus trap or Esc handling → rebuilt on Radix Dialog (device-lost is
  non-dismissable: reconnect is the only honest action). Live-verified: Esc
  closes stop-confirm, dialog names announced, zero console errors.
- Dead-code purge (knip → 0 findings): removed Stepper, Textarea,
  ScrollArea/Separator (scroll-area.tsx renamed skeleton.tsx — only Skeleton
  was consumed), tabs.tsx, 30+ unused re-exports/variants/helpers
  (expect/hashString/clamp, formatByteUnit, setLogLevel, EXTENSIONS,
  HomeStats, raw Tooltip/DropdownMenu re-exports); wired `exportLogText`
  into Settings→Diagnostics (was duplicated inline); wired Radix Select into
  scan-config sector size (was a native `<select>`). Dependencies removed:
  @radix-ui/react-{tabs,scroll-area,separator,slider,toggle-group},
  @testing-library/user-event.
- knip.json: tests tsx entry (was `tests/**/*.ts` — .tsx invisible), CSS in
  project (resolves tw-animate-css `@import`), `ignoreExportsUsedInFile`
  (router/ErrorScreen/ConnectorMode class of false positives);
  `src/connector/contract.ts` types stay via in-file usage — no ignores needed.
- CI hardening: `KNIP_DISABLE_RAW_TRANSFER=1` on the knip step (oxc raw
  transfer reserves a 6 GiB buffer per core — fragile on runners, and crashes
  this 4 GB sandbox outright).
- Rust clippy pre-fix: `format!("/select,{}", path)` → `{path}` inline
  (uninlined_format_args is warn-by-default; CI runs clippy -D warnings, and
  clippy had NEVER run — the format check blocked it). serde_json already
  removed from Cargo.toml matches the (serde-only) sources.

### Re-verification (local, sandbox-representable parts)

typecheck ✓ · eslint 0 errors (2 informational react-compiler notices on
RHF/TanStack APIs — inherent to spec'd libraries) · prettier ✓ · arch-lint ✓ ·
ast-grep both rules ✓ (negative-tested: injected violations are caught) ·
knip 0 findings ✓ · jscpd 0.42% ✓ · vitest 55/55 ✓ · build ✓ · bundle budget
89.7/200 KB main, 315.2/500 KB total ✓ · i18n parity 398/398 ✓ ·
agent-browser live pass: choose-folder → folder-mode preselect → Browse fills a
resolvable path → scoped scan → completion dialog → scoped results (Config as
tree root) → stop-confirm Esc/Keep/Stop — zero console errors, evidence
screenshots verify2-01..05.

Windows-only gates (NSIS, cargo, Playwright-on-Windows, DPI) re-armed on the
tagged commit via Release/UI-Smoke/CI runs.
