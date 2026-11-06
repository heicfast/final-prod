# final-prod — Unified Data Recovery Product

Single production repository containing the native recovery engine
(`engine/`), the application UI (`ui/`), the shared boundary contracts
(`contracts/`), and the combined installer (`installer/`). One product, one
install: the NSIS installer puts the application and the engine on the
customer's machine in standard locations (Program Files + ProgramData) —
and nothing else.

**A user-mode product with direct raw-disk access.** There is no kernel
driver, no service, and no certificate-store change anywhere in this
product — nothing is loaded into the operating system. The engine reads
and writes disks and volumes directly from user mode
(`\\.\PhysicalDriveN` opens through the 64 KiB read envelope with true byte
counts). **The application requests the administrator token at launch**
(`requireAdministrator` manifest — `ui/src-tauri/app.manifest`, reference
parity with the og's DRW.exe/DRWUI.exe posture): one standard UAC prompt
when it starts, none mid-session. Scanning, recovering, health checks,
repair and lost-partition apply all need the elevated token; should a
non-elevated context ever occur (edge case), the process gets a truthful
`ERR_ELEVATION_REQUIRED` refusal (CLI exit 4) — never a degraded mode,
never a fallback, never a silent failure. Uninstalling
removes everything the installer placed; saved sessions are kept by
default (explicit consent required to delete them).

**Status: consolidation session complete at the tree level** — the kernel
driver is removed (the 0.6.x mandatory-driver posture is dead), the engine
is advanced to the current core_recovery line (ABI v4, 92 exports), and
the UI is re-based onto the current wizard base; the connector rewiring to
the real engine lands with the bridge (see `worklog/INTEGRATION-PLAN.md`
phase P4 — the mock connector is the interim dev/test backend, never the
shipped one).

Merged with full history from (directory-rewriting merges, no squash):

| Component | Source repository | Merged commit |
|---|---|---|
| engine/ | heicfast/core_recovery (main) | a574ecf → 22e9657 → **12f3edf7 (ABI v4)** |
| ui/ | heicfast/recovery-wizard-ui-new (v3) | f4a8f8a → **dd2782c (re-adopted)** |
| ~~driver/~~ | ~~heicfast/rda-driver (main)~~ | da7efb9 — **REMOVED** (tree-only; the git lineage is preserved in history) |

## Layout

- `engine/` — C++20 user-mode recovery engine; the `cr_ui` C ABI
  (`engine/include/cr/cr_ui.h`) is the UI's in-process boundary
  (92 exports, ABI version 4): nine filesystem engines, lost-partition
  recovery, grammar-aware carving, durable `.crsf` scan sessions,
  transactional repair, the safe recovery writer, SMART/NVMe health,
  BitLocker legitimate unlock
- `ui/` — React 19 + Tauri 2 shell (`ui/src-tauri/`) + the connector
  contract (`ui/src/connector/contract.ts`, 36 operations across 9 domains)
- `contracts/` — the single engine↔UI boundary truth: command surface,
  error-code map, event map, type maps — validated by `contracts/validate.mjs`
- `installer/` — the combined NSIS installer + the portable zip + the
  development self-signing tool (`installer/devsign.ps1`)
- `build.ps1` / `build.sh` — the single-command product build
- `docs/` — product documentation (see below)

## Build

```powershell
.\build.ps1                 # full product: engine + ui + installer + portable + checksums
.\build.ps1 -Target engine  # one lane: contracts | engine | ui | installer | portable | all
```

Toolchain prerequisites and every sub-build: `docs/BUILD.md`.
On a clean checkout the dev loop is: engine lane once (stages the engine
kit), then `pnpm tauri dev` in `ui/`.

## Documentation

| Doc | Contents |
|---|---|
| `docs/ARCHITECTURE.md` | The product: engine, bridge, UI, installer — and how they relate |
| `docs/BUILD.md` | The single build command, every sub-build, toolchain prerequisites |
| `docs/RELEASE.md` | Tag flow, artifacts, signing truth |
| `docs/DEV-SIGNING.md` | Development self-signing: what, why, production path |
| `docs/TESTING.md` | The test suites and the validation evidence |
| `docs/INTEGRATION-PLAN.md` | The prior session's plan (historical — completion state inside) |
| `docs/INTEGRATION-GAPS.md` | Every capability gap with precise scope + follow-up |
| `docs/PARITY-DELTAS.md` | Divergences from the reference layout, with rationale |
| `docs/INTEGRATION-VALIDATION.md` | Validation evidence index |
| `docs/MSIX-ASSESSMENT.md` | MSIX feasibility assessment (reopened post-driver-removal) |
| `contracts/README.md` | The boundary contracts and their validation gate |
| `worklog/` | The consolidation session's Phase 0 context sheets + the gate plan |
| `worklog.md` | Chronological session record |
