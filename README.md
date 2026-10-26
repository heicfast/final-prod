# final-prod — Unified Data Recovery Product

Single production repository containing the native recovery engine
(`engine/`), the application UI (`ui/`), the kernel driver (`driver/`),
the shared boundary contracts (`contracts/`), and the combined installer
(`installer/`).

**Status: consolidation in progress — see `docs/INTEGRATION-PLAN.md`.**

Merged with full history from:
| Component | Source repository | Merged commit |
|---|---|---|
| engine/ | heicfast/core_recovery (main) | 5ddccd8912b81d971970f84adf53674d6c6be55d |
| ui/ | heicfast/recovery-wizard-ui-new (v3) | f4a8f8aebab625e9048d833cd9c3501741d7c436 |
| driver/ | heicfast/rda-driver (main) | da7efb9e0c94046893c62b25695bb95edf519f8b |

Layout:
- `engine/` — C++20 recovery engine, `cr_ui` C ABI (the UI's in-process boundary)
- `ui/` — React + Tauri application shell
- `driver/` — rda.sys kernel-mode raw disk transport
- `contracts/` — the single engine↔UI boundary truth (error map, event map, command surface)
- `installer/` — NSIS combined installer (+ portable build)
- `docs/` — unified product documentation
- `tests/` — cross-component integration tests
- `build/` — top-level build orchestration (single command product build)

Build (once the consolidation completes): see `docs/BUILD.md`.
