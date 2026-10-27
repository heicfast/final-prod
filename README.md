# final-prod — Unified Data Recovery Product

Single production repository containing the native recovery engine
(`engine/`), the application UI (`ui/`), the kernel driver (`driver/`), the
shared boundary contracts (`contracts/`), and the combined installer
(`installer/`). One product, one install: the NSIS installer puts the
driver, engine, and UI on the customer's machine in standard locations.
**The RDA driver is a REQUIRED component** — it is the engine's only
raw-disk transport (no fallback, no degraded mode); the installer ships it
development test-signed and imports its certificate so it can load
(`docs/DEV-SIGNING.md`).

**Status: integration shipped** — the UI's simulated backend is gone; the
UI talks to the real engine through the Rust bridge
(`ui/src-tauri/src/`). The remaining tracked work lives in
`docs/INTEGRATION-GAPS.md` (capability gaps with follow-ups) and the
worklog's next-actions section.

Merged with full history from (directory-rewriting merges, no squash):

| Component | Source repository | Merged commit |
|---|---|---|
| engine/ | heicfast/core_recovery (main) | 5ddccd8 → advanced to 22e9657 |
| ui/ | heicfast/recovery-wizard-ui-new (v3) | f4a8f8a |
| driver/ | heicfast/rda-driver (main) | da7efb9 |

## Layout

- `engine/` — C++20 recovery engine; `cr_ui` C ABI (`engine/include/cr/cr_ui.h`)
  is the UI's in-process boundary (78 exports, ABI version 2)
- `ui/` — React 19 + Tauri 2 shell + the Rust bridge
  (`ui/src-tauri/src/{bridge,commands}/`, 33 Tauri commands)
- `driver/` — `rda.sys` kernel-mode raw-disk transport (service `RDA`)
- `contracts/` — the single engine↔UI boundary truth: command surface,
  error-code map, event map, type maps — validated by `contracts/validate.mjs`
- `installer/` — the combined NSIS installer + the portable zip + the
  development signing tool (`installer/devsign.ps1`)
- `build.ps1` / `build.sh` — the single-command product build
- `docs/` — product documentation (see below)

## Build

```powershell
.\build.ps1                 # full product: driver + engine + ui + installer + portable + checksums
.\build.ps1 -Target engine  # one lane: contracts | engine | driver | ui | installer | portable | all
```

Toolchain prerequisites and every sub-build: `docs/BUILD.md`.
On a clean checkout the dev loop is: engine lane once (stages the engine
kit), then `pnpm tauri dev` in `ui/`.

## Documentation

| Doc | Contents |
|---|---|
| `docs/ARCHITECTURE.md` | The product: driver, engine, bridge, UI, installer — and how they relate |
| `docs/BUILD.md` | The single build command, every sub-build, toolchain prerequisites |
| `docs/RELEASE.md` | Tag flow, artifacts, signing truth |
| `docs/DEV-SIGNING.md` | Development test signing: what, why, machine policy matrix, production path |
| `docs/TESTING.md` | The test suites and the validation evidence |
| `docs/INTEGRATION-PLAN.md` | The ratified gate plan (historical — completion state inside) |
| `docs/INTEGRATION-GAPS.md` | Every capability gap with precise scope + follow-up |
| `docs/PARITY-DELTAS.md` | Divergences from the reference layout, with rationale |
| `docs/INTEGRATION-VALIDATION.md` | Validation evidence index |
| `docs/MSIX-ASSESSMENT.md` | Why MSIX is not feasible for this product |
| `contracts/README.md` | The boundary contracts and their validation gate |
| `worklog.md` | Chronological session record |
