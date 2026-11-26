# SB-B-02 fix evidence — the dead X button (P0)

## Defect (before)

Two stacked root causes on the close path of the frameless main window
(`decorations: false` — the custom X in `window-controls.tsx` is the ONLY
close affordance):

1. **Missing capability grant.** `ui/src-tauri/capabilities/default.json`
   granted allow-close/minimize/toggle-maximize/is-maximized/start-dragging/
   set-focus but NOT `core:window:allow-destroy`. The X button's `close()`
   emits close-requested; with a JS `onCloseRequested` listener registered,
   closing is entirely up to the JS side — and every close path funnels into
   `destroy()` (`plugin:window|destroy`, ACL-gated). Denied invoke → rejected
   promise → `void`-swallowed → **the window can never close via the X in any
   state**.
2. **The exit-guard race.** The guard registered
   `onCloseRequested((event) => { void onRequested(event); })` — a
   fire-and-forget async handler. The installed @tauri-apps/api 2.12.4
   wrapper (verified verbatim in `node_modules/@tauri-apps/api/window.js:2030-2036`)
   does `await handler(evt); if (!evt.isPreventDefault()) await this.destroy()`.
   A fire-and-forget handler returns `undefined` to that await — the wrapper
   checks the flag BEFORE the guard's async engine-state read resolves, so:
   mid-scan, the wrapper would destroy the window while the save dialog is
   still being decided (had the permission been granted). preventDefault()
   landed after the check — a no-op.

Verification that allow-destroy is NOT in `core:window:default` (i.e. that
the explicit grant is required): the Tauri 2 core-permissions reference
(core:window:default includes get-all-windows/is-maximized/read-only getters;
destroy/close/minimize/toggle-maximize are NOT included) — fetched from
tauri.app/reference/acl/core-permissions during this session.

## Fix (after)

- `ui/src-tauri/capabilities/default.json`: + `core:window:allow-destroy`.
- `ui/src/app/exit-guard.tsx`: the handler now RETURNS its promise to the
  wrapper — `.onCloseRequested((event) => onRequested(event))` — so the
  wrapper's isPreventDefault() check runs only after the guard decided. The
  redundant `void win.destroy()` in the not-prevented branch was removed
  (the wrapper destroys; a second destroy would race into a rejected invoke
  on a gone window). The dialog paths (Save and exit / Exit without saving)
  keep their own destroy — the wrapper is not involved there.

## Regression tests (before-red / after-green)

- `ui/tests/exit-guard-close-race.test.tsx` — mock faithfully transcribed
  from the installed @tauri-apps/api 2.12.4 wrapper semantics. Pre-fix: the
  mid-scan case destroys the window while the dialog should win (red).
- `ui/tests/window-capability-gate.test.ts` — static gate: scans `ui/src`
  for every window-API method call and asserts the permission is granted in
  the capability of the window that renders it (shared chrome must be
  granted in both main + disk-health). P0 pin: destroy usage ⇒ allow-destroy.
  Pre-fix: red on exactly the shipped defect.
- `ui/tests/exit-guard.test.tsx` — updated to the faithful wrapper mock
  (assertions unchanged in intent; the close is now asserted through the
  wrapper's destroy — the real mechanism).

## Captured runs (this session, Linux sandbox, real vitest)

- BEFORE (worktree with the two fix files reverted via git stash):
  `Tests 3 failed | 4 passed (7)` — the P0 pin + the race cases red.
- AFTER: `tests/exit-guard*.tsx + window-capability-gate: 11/11 green`;
  full suite `32 files / 232 tests` green (no adjacent regression).
- Windows runtime proof (the 30-second X-click check from the audit):
  `ui-ci` / `ui-e2e` CI lanes on the pushed commit carry the static gates;
  the live click-through remains a Windows-runner smoke item (OPEN-ITEMS),
  since no Windows desktop exists in this sandbox.
