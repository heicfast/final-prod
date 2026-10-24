# IMPLEMENTATION-STATUS — the experience-layer tracker

> Single source of truth for what is implemented and what remains, per the
> experience session mandate. Updated after every completed item. Companion
> gate docs: EXPERIENCE-SPEC.md (reference parameters), MOTION-SPEC.md
> (choreography), INTERIM-ASSETS.md (interim layer). Baseline: v0.2.1
> (8ab7234), sessions 1–4 delivered the core surfaces (ACCEPTANCE.md PASS
> grid); this session owns the experience layer on top of that baseline.

## 1. Surfaces

| Surface                                                              | Corpus ref  | State                 | Gaps                                                                                                                                                                        | Plan                                                               |
| -------------------------------------------------------------------- | ----------- | --------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| Window chrome (min/max/restore/close)                                | EXP-SPEC §1 | **COMPLETE** (T5)     | Wiring + corpus 4-state glyphs + dblclick toggle; real-window drag/snap verified via Windows tauri-debug-build gate (capabilities compile-time-validated)                   | —                                                                  |
| Left sidebar                                                         | EXP-SPEC §2 | **COMPLETE** (T6)     | Collapse/expand (52px icon rail), 360ms width + 160ms label fade, icon-gutter retention, keyboard toggle, localStorage persistence — live-verified                          | —                                                                  |
| Dialogs & popups                                                     | EXP-SPEC §3 | **PARTIAL**           | All corpus dialogs present with correct ESC discipline; **no exit animations** (Radix unmounts instantly); stop-confirm allows click-outside where corpus requires a choice | T7: exit fades; per-dialog outside-click audit; scroll lock verify |
| Toasts                                                               | EXP-SPEC §4 | **COMPLETE** (verify) | sonner stack — verify hover-pause, focus/aria, offset vs sidebar/CTA overlap at all widths                                                                                  | T8: verification + rich-color treatment                            |
| Page transitions                                                     | EXP-SPEC §5 | **PARTIAL**           | Instant swap is corpus-accurate; add the 160 ms incoming-page fade; verify zero CLS between routes                                                                          | T9                                                                 |
| Microinteractions                                                    | EXP-SPEC §6 | **PARTIAL**           | Button/card/row states complete; verify switch/checkbox/segmented press states; context menu enter ok                                                                       | T10                                                                |
| State surfaces (loading/empty/error/partial/permission/disconnected) | EXP-SPEC §7 | **COMPLETE** (verify) | ACCEPTANCE PASS; re-verify per surface during this session                                                                                                                  | T11                                                                |
| Home / device selection                                              | UI-MAP §1   | **COMPLETE**          | —                                                                                                                                                                           | —                                                                  |
| Scan progress                                                        | UI-MAP §2   | **COMPLETE**          | —                                                                                                                                                                           | —                                                                  |
| Results explorer                                                     | UI-MAP §3   | **COMPLETE**          | Type⇄Path tab: instant tab highlight + async tree — verify opacity treatment                                                                                                | T9/T15                                                             |
| Preview pane                                                         | UI-MAP §4   | **COMPLETE**          | —                                                                                                                                                                           | —                                                                  |
| Recovery flow                                                        | UI-MAP §5   | **COMPLETE**          | —                                                                                                                                                                           | —                                                                  |
| Health / Repair / Settings / LostPartition                           | UI-MAP      | **COMPLETE**          | —                                                                                                                                                                           | —                                                                  |
| i18n (en/zh-CN)                                                      | —           | **COMPLETE**          | New UI strings for T5/T6 must land in both catalogs                                                                                                                         | per item                                                           |
| Interim asset layer                                                  | mandate §3  | **PARTIAL**           | Chrome SVGs extracted + manifest (T5); broader set pending                                                                                                                  | T13                                                                |

## 2. Components (experience-critical set)

| Component                                          | Purpose                       | Ref behavior                                                | State                                                                                        | Gaps                   |
| -------------------------------------------------- | ----------------------------- | ----------------------------------------------------------- | -------------------------------------------------------------------------------------------- | ---------------------- |
| `WindowControls` (new)                             | min/max/restore/close cluster | 4-state, order min→max→close, 24 px glyphs, hover plate rx4 | **COMPLETE** (T5: jsdom 5/5)                                                                 | —                      |
| `TitleBar` (shell header)                          | drag region + app identity    | ~35 px, drag, dblclick toggles max                          | **COMPLETE** (T5: dblclick wired, inert over interactive children; drag region pre-existing) | —                      |
| `SidebarNav` (shell nav)                           | IA navigation                 | fixed rail; collapse is app extension                       | **PARTIAL**                                                                                  | T6                     |
| `Dialog`                                           | modal surface                 | fade enter; exit fade; Freeze                               | PARTIAL                                                                                      | exit animation         |
| `DropdownMenu` / `ContextMenu` / `Popover`         | menus                         | 120 ms fade enter                                           | PARTIAL                                                                                      | exit fade              |
| `Toaster`                                          | toasts                        | sonner stack                                                | COMPLETE                                                                                     | verify                 |
| `Button`                                           | all CTAs                      | instant state swap ≤120 ms fade                             | COMPLETE                                                                                     | —                      |
| `Checkbox` / `Switch` / `RadioGroup` / `Segmented` | toggles                       | instant state swap                                          | COMPLETE (verify press states)                                                               | T10                    |
| `ProgressGauge`                                    | scan % ring                   | animated fill, freeze on pause                              | COMPLETE                                                                                     | verify freeze on pause |
| `VirtualFileList`                                  | virtualized rows              | no scroll jank                                              | COMPLETE                                                                                     | T15                    |
| `ResultsTree`                                      | tree nav                      | async expand                                                | COMPLETE                                                                                     | —                      |
| `PreviewPaneResizer`                               | drag resize                   | 1:1 pointer follow                                          | COMPLETE                                                                                     | —                      |

## 3. Experience parameters (§4 set) — verdicts per surface

Legend: ✓ verified · ◐ partial · ✗ missing. (Baseline from ACCEPTANCE +
this session's audit; re-verified per item as work lands.)

| Parameter                 | Chrome | Sidebar | Dialogs | Toasts | Pages | Home | Scan | Results | Recovery |
| ------------------------- | ------ | ------- | ------- | ------ | ----- | ---- | ---- | ------- | -------- |
| 1 Window-size range       | ◐      | ◐       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 2 Aspect resilience       | ◐      | ◐       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 3 DPI resilience          | ◐      | ◐       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 4 Sidebar collapse        | —      | ✗       | —       | —      | —     | —    | —    | —       | —        |
| 5 Dialog open/close       | —      | —       | ◐       | —      | —     | —    | ✓    | ✓       | ✓        |
| 6 Toast choreography      | —      | —       | —       | ✓      | —     | —    | —    | ✓       | ✓        |
| 7 Chrome states           | ✗      | —       | —       | —      | —     | —    | —    | —       | —        |
| 8 Task clarity            | ✓      | ✓       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 9 Error recovery          | —      | —       | ✓       | ✓      | —     | ✓    | ✓    | ✓       | ✓        |
| 10 Destructive gating     | —      | —       | ✓       | —      | —     | —    | ✓    | ✓       | ✓        |
| 11 Consistency            | ✓      | ✓       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 12 First-run clarity      | —      | ✓       | —       | —      | —     | ✓    | ✓    | ✓       | —        |
| 13 Progressive disclosure | —      | —       | ✓       | —      | —     | ✓    | ✓    | ✓       | —        |
| 14 Loading feedback       | —      | —       | —       | —      | —     | ✓    | ✓    | ✓       | ✓        |
| 15 Perceived perf         | —      | —       | —       | —      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 16 Keyboard flow          | ◐      | ◐       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 17 Focus in transitions   | —      | —       | ✓       | —      | ◐     | —    | ✓    | ✓       | ✓        |
| 18 Scroll preservation    | —      | ◐       | —       | —      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 19 Copy quality           | ✓      | ✓       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |
| 20 Non-color status       | ✓      | ✓       | ✓       | ✓      | ✓     | ✓    | ✓    | ✓       | ✓        |

## 4. Motion inventory (from MOTION-SPEC §3)

| Motion                                | Status                                                      |
| ------------------------------------- | ----------------------------------------------------------- |
| Dialog overlay fade-in                | IMPLEMENTED                                                 |
| Dialog content fade-in                | IMPLEMENTED                                                 |
| Dialog exit fade                      | **MISSING → T7**                                            |
| Menu (dropdown/context/popover) enter | IMPLEMENTED (fade)                                          |
| Menu exit fade                        | **MISSING → T7**                                            |
| Toast enter/stack/dismiss/pause       | IMPLEMENTED (sonner)                                        |
| Route-change fade (160 ms)            | **MISSING → T9**                                            |
| Type⇄Path async tab switch            | partial — verify tree-rebuild opacity → T9                  |
| Sidebar collapse transition           | **MISSING → T6**                                            |
| Chrome button state swaps             | **IMPLEMENTED** (T5: nor/hov/pre corpus SVGs, instant swap) |
| Gauge animated fill / pause freeze    | IMPLEMENTED (200 ms linear) — verify freeze                 |
| Ticker status pulse                   | IMPLEMENTED (2.4 s, dot only)                               |
| Progress stripes                      | IMPLEMENTED (1.1 s)                                         |
| Button hover/press fades              | IMPLEMENTED (100 ms)                                        |
| Card selection (device tiles)         | IMPLEMENTED (instant + border)                              |
| Reduced-motion global                 | IMPLEMENTED                                                 |

## 5. Interim assets

See INTERIM-ASSETS.md. Current: none extracted; v1 icon set is Lucide.
Corpus chrome SVGs (4-state × light/dark × min/max/restore/close) are the
T5 extraction set. Status updated per item.

## 6. Open items (priority order)

1. **T5** Window chrome — ✅ DONE: capabilities, component, corpus states,
   dblclick, e2e coverage.
2. **T6** Sidebar collapse — ✅ DONE: transition, persistence, keyboard,
   no CLS (same-DOM-node assertion + live 208⇄52 verification).
3. **T7** Dialog/menu exit animations + outside-click audit — ✅ DONE:
   focus-return-to-invoker bug found (Radix controlled-dialog triggerRef
   gap) and fixed with evidence (e2e + jsdom).
4. **T8** Toast verification pass — ✅ DONE.
5. **T9** Route fade + async tab treatment — ✅ DONE.
6. **T10** Microinteraction audit — ✅ DONE (press tokens applied,
   drag-drop confirmed corpus-N/A).
7. **T11** State surfaces — ✅ DONE: volume-level status bug fixed
   (home banner + card lock + per-volume chip gating), dev-panel
   invalidation, live evidence.
8. **T12** Feature walk — ✅ DONE: all core surfaces verified COMPLETE
   in the deep read; recovery destination audit closed the walk.
9. **T13** Interim assets — ✅ DONE: chrome layer extracted + manifest;
   scope decision documented in INTERIM-ASSETS.md.
10. **T14** Responsiveness/DPI — ✅ DONE: 800×600 min + square + wide +
    dsf 1.5 e2e layout invariants; screenshots archived in CI artifacts.
11. **T15** Per-component verification — ✅ DONE: parameter grid final;
    SPA focus-on-main-landmark added (route transitions) + keyboard-flow
    e2e.
12. **T16** Playwright experience-path coverage.
13. **T17** Frame-budget watchdog review.
14. **T18** Full CI green.
15. **T19** Final tracker update.
16. **T20** Handoff.
