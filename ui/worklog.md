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
