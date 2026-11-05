# UI-WIRING — gate §6 evidence: the UI is wired to the current core

Written: 2026-11-05 (consolidation release session), re-verified against
the tree at d166147b. The wiring landed in P4 (8e61f4a8 → e33dd819, the
commit trail below); the GREEN closure is db7f588a.

## The architecture (as built)

- **One connector seam** — `ui/src/connector/index.ts` is the only backend
  selector: `VITE_CONNECTOR=tauri` (default, shipped builds) → the real
  bridge; `mock` → the TEST DOUBLE (dev browser, vitest, e2e only).
- **The bridge** — `ui/src-tauri/src/bridge/` (abi, error, events, state,
  consent, filefilter, logging, util) + `commands/` (12 domains) +
  `lib.rs`; statically imports `cr_ui.dll` (ABI v4, 95 exports) via
  `cr_ui.lib` — the same import-lib contract the release bundle ships.
- **The TS connector** — `ui/src/connector/tauri/tauri-connector.ts`
  (453 lines): maps every command result and error (the numeric engine
  space + bridgeCodes) onto `contract.ts`'s vocabulary; events ride the
  Tauri event channel with the engine's CR_EVT payloads.
- **No mocks in shipped builds** — the mock is a TEST DOUBLE by name and
  by gate: the forensic no-mock gate asserts on the shipped DOM that
  `__TAURI_INTERNALS__` is present and the double's `__rwInjectConsent`
  hook is absent (drive-ui.mjs, run fails on violation).

## Per-surface wiring (spot-verified in-tree at d166147b)

| Surface | UI | Bridge | Proof |
|---|---|---|---|
| Devices + quick targets | home-page.tsx (`RECYCLE_BIN_TARGET='vol-recycle-bin'` :54) | commands/devices.rs (:253 resolves the virtual target) | e2e smoke "quick targets start real scans directly" |
| Scan lifecycle | /scan/:id/live | commands/scan.rs (CR_EVT stream → status/progress/found) | e2e golden path (scan → results) |
| Results explorer | results explorer feature | commands/results.rs (G-7 filter over NodeMeta; 1,481-entry filefilter mirror) | e2e filter/search/preview legs; filefilter:check fresh |
| Recovery | recover flow | commands/recovery.rs (F12 overlap refusal, enqueue, report) | e2e recover + report + export legs |
| Health | health-page.tsx | commands/health.rs (value/worst/raw/status; threshold column truthful-disabled, BF-03) | e2e responsive results-pane; unit suite |
| Consent (repair + apply) | consent-modal.tsx (shell-mounted, non-dismissable) | bridge/consent.rs (echo tokens, lock-release-before-block, force-deny on cancel) | e2e consent.spec (deny leg + re-hydration leg), unit state.pendingConsent |
| Sessions save/resume | backup screen + home resume | commands/sessions.rs (save/load/resume-last/verify-identity/autosave) | unit connector-tauri tests (397-line contract test) |
| Lost partition | lost-partition feature | commands/lost_partition.rs (apply consent S-029 + undo) | unit + consent-surface tests |
| Preview/thumbnails | preview pane | commands/preview.rs (hex/image/archive M-116; session thumbnails real, live fallback BF-13) | unit suite |
| Elevation posture | elevation-banner.tsx (role=alert) + restart-as-admin | commands/system.rs (TokenElevation probe + ShellExecuteW "runas") | forensic unelevated twin (banner + refusal assertions) |

## Threading / cancellation / lifecycle rules (encoded)

- Scan run threads belong to the engine; the bridge only mirrors state
  under snapshot locks (bridge/state.rs) — the UI's react-query polling
  never blocks the engine.
- Cancel resolves (never rejects): ERR_CANCELLED translates to status
  'cancelled' per contracts/error-codes.json.
- The consent gate: non-dismissable modal; scan Stop force-denies via
  cancel (the bridge closes the gate FIRST — no deadlock); re-hydration
  from getScanState().pendingConsent survives SPA navigation (the e2e
  history-navigation leg, db7f588a).
- Elevation: ERR_ELEVATION_REQUIRED (0x500C0001) crosses as an engine
  code → banner + relaunch; never a degraded mode.

## The mock demotion (honesty proof)

- `VITE_CONNECTOR` default flipped to `tauri` at P4 (connector/index.ts
  header comment); build.ps1's ui lane runs `pnpm tauri build` with no
  connector override → the shipped bundle is the real backend.
- The mock remains for: `pnpm dev` (browser dev), vitest (vite.config
  test.env), the e2e lane (explicit `VITE_CONNECTOR=mock` in ui-e2e.yml).
- The release lane's own gate: forensic drive-ui.mjs's no-mock assertion
  on the shipped DOM (above).

## Test inventory (what proves the wiring)

- `ui/tests/connector-tauri.test.ts` — 397 lines pinning the REAL
  connector's command/error/event mapping against a spied invoke.
- `ui/tests/consent-surfaces.test.tsx` — the G-1 consent surface proofs
  (+372 lines, 573fba98).
- vitest: **174/174** green (24 files), coverage 84.7/63.3/76.5/85.6
  all-files, connector/tauri 84.2/79.4/88.9/87.9 (run 42109647269).
- Playwright: **14/14** green incl. the consent re-hydration leg (run
  42122198275).
- cargo: clippy `-D warnings` ZERO + `cargo check --tests` ZERO (CI runs
  42122198423; locally cross-checked against x86_64-pc-windows-msvc).
- Forensic S1–S4 on the installed product: the release-validation
  dispatch (RELEASE-RUN.md records the run).

## Commit trail

8e61f4a8 (the bridge lands, 58 files +17,903) → 573fba98 (surface
proofs) → 70741eb7 (convergent-lane reconcile) → 0a43f678 (e2e nav fix)
→ 8c7b82f2 (7 Windows compile errors + 2 clippy) → e33dd819
(ShellExecuteW HWND) → **db7f588a** (the 5 clippy dead-code removals +
the history-navigation e2e fix — both lanes GREEN).
