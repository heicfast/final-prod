# ROADMAP

## v0.1.0-ui-mock (this session) — DONE

- Research gate: screenshot manifest, visual spec, UI map, design system.
- Full Tauri 2 + React 19 + TS strict + Vite 8 scaffold, CI on windows.
- Mock Connector: 9 domains, deterministic fixtures (1.2 M-node index),
  timing budgets, error injection, dev panel.
- All 9 wizard surfaces with loading/empty/error/partial states.
- Virtualized results (tree + list), bounded preview workers, recovery queue
  with hard overlap refusal, SMART + transactional repair, sessions.
- 52 tests green, Playwright golden path, browser-verified with evidence.

## Next: engine integration session

1. **Wire the real connector** (`src/connector/tauri/`): implement the
   contract over `invoke()` + `emit`/`listen`; delete the throwing stub.
   Map the scan engine's staged progress to `ScanProgressEvent` (weighted,
   monotonic) and its IOCTL-backed enumeration to the Devices domain.
2. **Engine command surface** (Rust): thin commands mirroring the contract
   (start/pause/resume/cancel, tree/page queries with server-side filters,
   preview reads with bounds, session save/load to real `.rsf`-equivalents,
   recovery job orchestration, SMART passthrough, RawFixer dry-run/apply/undo).
3. **Real validation**: replace the JS-side `validateDestination` with
   engine volume checks (same-letter + raw volume open semantics).
4. **Measured budgets on real hardware**: re-run grid #34–44 (1 M rows,
   filter <200 ms, preview <500 ms, cold start <2 s, DPI ladders) against
   the engine, not the simulator.

## v0.2 candidates

- Icon set v2: purpose-drawn two-tone filetype/device icons (see
  INTERIM-ASSETS.md).
- AV bounded playback behind a stability flag.
- Additional locales beyond en/zh-CN (infrastructure already locale-safe).
- Cloud/NAS targets — only when the engine exposes them in the contract
  (UI stays truthful until then).
- RHF→stable-8 / React Router 8 / Vitest 5 upgrades (ADR-005 notes the
  mapping).
- Per-route automated axe audit in CI.

## Standing decisions to keep

- No monetization/retention surfaces (documented divergence, UI-MAP §0).
- Preview rendering boundary per ADR-003 (engine renders document pages).
- The UI never re-grades confidence or health — connector data is final.
