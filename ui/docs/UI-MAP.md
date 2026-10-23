# UI-MAP

Page inventory, transitions, controls, states, and dialogs for the Recovery Wizard UI.
Scope note: this re-implementation covers the **recovery workflow core** (device
selection → scan → results → recovery, plus health/repair/settings). Corpus surfaces
that are commercial funnel (retention gates, license activation, cross-sell product
pages, AI assistant) are deliberately excluded — see §0.

## 0. Documented divergences from the reference corpus

| Reference surface | Decision | Rationale |
|---|---|---|
| Retention/license/upsell gates | **Excluded** | Commercial funnel specific to the reference's monetization model; not part of a truthful recovery-tool IA. The connector never exposes them, so the UI cannot honestly render them. |
| AI assistant / AI recommend | **Excluded this session** | Would be a UI-fabricated capability — the Mock Connector contract exposes no ranking model. Roadmap item if the engine ever provides one. |
| Cloud/NAS/WeChat/iCloud targets | **Excluded this session** | Reference chains require network backends; connector exposes local devices + removable media only. Cloud/NAS marked as unavailable-with-reason in the UI (truthful degradation). |
| Sidebar product pages (backup, crashed PC, etc.) | **Excluded** | Separate products, not recovery wizard flow. |
| Auto-Recover after scan | **Excluded** | Requires engine orchestration the contract does not expose. |

Everything else follows the corpus IA: 4-zone wizard stack, 3-pane results, scan lives
inside the results page, celebration modal, destination dialog semantics, session
save/load, contextual CTAs.

## 1. Route graph

```
/                      Home (device selection)
/scan/:deviceId        Scan Configuration (mode, types, advanced)
/scan/:deviceId/live   Scan Progress + live results (3-pane, scan-in-progress banner)
/results/:scanId       Results Explorer (3-pane)
/recover/:scanId       Recovery Queue + Destination + Report
/health/:deviceId      Disk Health (SMART)
/repair/:scanId        Volume Repair (RawFixer surface)
/settings              Settings & About
```

Router: React Router v7 data mode, `createHashRouter` (ADR-001). Guards:
`/scan/:deviceId/live` and `/results/:scanId` require a live/loaded session via
connector `getScanState`/`getSessionMeta` — otherwise redirect `/` with a toast.
Back-navigation follows the wizard stack (Home ⇄ every page; results keeps a
"resume scanning" path while scan is live).

Transitions: cross-fade 160ms; scan→results is explicit via celebration modal CTA
(auto-dismiss optional); disconnect error → inline reconnect prompt, not a route change.

## 2. Pages, controls, and states

### 2.1 Home (Device Selection)

Zones: title bar / left sidebar (sections: Data Recovery rows — Hardware Disk,
Removable Media, Lost Partition Rescue; Advanced Features — Disk Health, Volume
Repair; footer: Settings, About) / content (page title, internal-drives group,
removable group, quick targets row: Recycle Bin / Desktop / Choose Folder,
primary CTA bottom-right).

Controls: device cards (select on click, blue border + checkmark), capacity bar +
used/total, health hint chip (Good/Caution via connector), lost-partition badge,
refresh devices, session import (file picker → connector loadSession), recent
sessions list (resume → results), per-card Scan button.

States:
- loading: skeleton cards, device enumeration spinner (corpus "early" capture)
- empty (no devices): coaching illustration + refresh CTA + guidance
- error: connector error with reason + retry
- permission-denied: banner asking for elevation, per-device lock icon + reason
- disconnected: card marked offline mid-listing, rescan offered
- success: populated grid, selection, CTA enabled on selection

### 2.2 Scan Configuration

Zones: header (device summary card), mode cards (Quick / Deep / Deep+Carve staged
model), file-type pre-selection grid (8 categories with icons + counts from
connector filter metadata), advanced disclosure (sector-size override if exposed,
bad-sector policy: stop / skip / zero-fill [zero-fill requires explicit opt-in],
retry policy visibility), footer (Back, Start Scan primary).

States: loading (device info), filesystem-unavailable (quick scan impossible →
truthful warning + deep-only), permission-denied, error (device gone → back to
home), success.

### 2.3 Scan Progress (live inside results layout)

Zones: 3-pane results layout with scan-in-progress banner ("results so far" marked
as partial); bottom status cluster: circular overall gauge (weighted monotonic),
stage line, bytes/throughput, ETA, files-found live counter, bad-sector counter +
map indicator; controls pause / resume / stop (confirm dialog); bottom-right
Recover-All CTA (enabled only when contract marks extent safe; otherwise disabled
with tooltip explaining deferral).

States: running (gauge + strip animate; tree fills progressively), paused (frozen
gauge + red strip), resuming, stopping, completed (celebration modal), cancelled
(partial results offered: "browse what was found" vs home), error-injection
scenarios: bad-sectors (error map + count), device-disconnected (pause + reconnect
prompt, no crash), permission-lost.

### 2.4 Results Explorer

Zones: toolbar (Home/Back/Up cluster, breadcrumb address, view toggle
detail⇄icons, filter button, search box with type scope) / filter bar (toggle row:
Path, File Type, Date Modified, File Size dropdowns + Show toggles + Reset) /
3-pane: left tree (tabs: Path | Type; 5 special branches for raw/deleted groups),
center virtualized list (columns: checkbox, Name, Date Modified, Type, Size;
confidence badge per row sourced from connector; thumbnail async), right preview
pane / bottom status bar (selection summary + contextual CTA Recover N / Recover
All).

Controls: multi-select (click, ctrl+click, shift+range, select-all, invert),
context menu (Recover / Preview / Open location / Select unrecovered / Smart
select best / Clear), sort headers, save session, load session, icon/detail views.

States: loading (skeleton rows), empty-filter ("no matches" + reset CTA),
empty-results (coaching), partial (scan incomplete banner + resume link), error
(connector), disconnected (device gone → results still browsable from session;
recovery requires source reconnection — truthful message), session-corrupt on load
(clear failure, no partial load), success.

### 2.5 Preview (pane + modal)

Kinds: image (bounded decode, zoom/fit/1:1), hex (first 64 KiB + offset nav),
text (size-capped, encoding badge), PDF (page bitmaps from connector, pager),
audio/video (metadata + first-frame thumbnail), office (metadata + extracted
text). Failure = diagnostic reason from connector error code (never generic).

States: loading (bounded worker), ready, unsupported (reason), failed (reason),
partial (truncated notice when bounded).

### 2.6 Recovery (Queue + Destination + Report)

Zones: queue list (per-item state icons, collision-naming policy selector, folder
reconstruction + empty-folder preservation toggles), destination section (volume
rows with free-space bars + same-source warning, folder picker, manual path input
with live validation), progress panel (per-file + overall, cancel, hash-verify
indicator), report dialog (success/partial/failed with reasons, export).

States: validating destination, overlap-refused (hard gate — reason from
`validateDestination`; UI cannot override), insufficient-space (preflight from
connector: needed vs free; Continue-anyway only when connector allows), running,
cancelled, completed-partial (failures listed with reasons), completed-success,
error (destination unavailable).

### 2.7 Health

Zones: disk selector, health summary card (status Good/Caution, temperature,
power-on hours, class SSD/HDD, TRIM state for SSD), SMART attribute table
(ID / Attribute / Current / Worst / Threshold / Raw / Status), coaching for
caution.

States: loading, error, not-smart-available (reason), success.

### 2.8 Repair (volume structure repair — RawFixer surface)

Zones: damage summary (from dry-run: findings list), consent gate (typed/checkbox
acknowledgment with consequence text), apply progress, undo-log viewer (actions
taken, sector snapshots), result.

States: idle, dry-run loading, dry-run findings (none → nothing-to-do state),
consent required, applying, applied (with undo available), failed (reason), undo
log empty.

### 2.9 Settings & About

Zones: theme (system/light/dark/high-contrast), language (en/zh-CN), density,
default scan mode, default destination, log location + open-log-folder, about
(version, connector mode badge mock/tauri, engine status fields from connector,
open-source licenses, diagnostics log viewer).

States: persisted via connector-backed settings where the contract exposes them;
localStorage for pure UI prefs (documented split).

## 3. Dialog inventory

| Dialog | Trigger | Semantics |
|---|---|---|
| Stop-scan confirm | stop button | stats + Stop/Continue (destructive-first per corpus but ours: Continue default-safe) |
| Celebration | scan completes | 540×380, stats grid, primary CTA "Open results" |
| Destination | recover CTA | per §2.6 |
| Space gate | preflight fail | needed-vs-free, Continue/Reselect (Continue only if connector allows) |
| Context menu | right-click row | 6 actions per §2.4 |
| Preview modal | preview action | large preview surface |
| Session picker | import session | file list from connector |
| Repair consent | apply repair | checkbox + consequence text, disabled until acknowledged |
| Error dialog | unrecoverable errors | reason + retry/contact actions |
| Confirm exit during scan | window close while scanning | guard: keep scanning / stop |

## 4. Cross-cutting UX rules (from corpus interaction rules, adapted)

1. Contextual bottom CTA reflects selection (Recover All ⇄ Recover N).
2. Progressive disclosure: results fill during scan; pause/stop always available.
3. Modal scrims dim main window; no click-through.
4. Single-ESC closes menus/dialogs only; exit confirm guards scanning state.
5. Monotonic overall progress; per-stage progress may reset.
6. Every numeric uses tabular figures; every timestamp localized via Intl.
7. Focus visible on every control; full keyboard path documented in TESTING.
8. No dead controls: unavailable = disabled + truthful reason tooltip.
