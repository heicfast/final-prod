# Recovery Wizard UI

Production-grade **Tauri 2 + React 19 + TypeScript (strict) + Vite** desktop
UI for a data-recovery wizard — wired to the REAL recovery engine through
the Rust bridge (`src-tauri/src/`, manifest: `contracts/command-surface.json`
at the repository root). The simulated backend this UI was born with is
gone; a small labeled test double (`tests/fakes/`) serves the component and
e2e suites through a test-mode-only build alias and never ships.

> Clean-room notice: all code, UI copy, and assets in this repository are
> original. The visual/interaction design was reconstructed from factual
> research parameters (layout structures, geometry, color roles) documented
> in `docs/VISUAL-SPEC.md` with per-token sources. No proprietary assets,
> code, or string tables were copied.

## Quick start

```bash
pnpm install          # pnpm 12.15.0, Node 24
pnpm dev              # browser dev mode (port 1420 / 3000; the real connector
                      # needs the Tauri shell + the engine kit - use tauri dev)
pnpm test             # 127 unit/contract/component/governance tests (21 files)
pnpm test:coverage    # unit tests with enforced v8 coverage thresholds
pnpm e2e              # 12 Playwright e2e tests (builds in test mode - the
                      # labeled double - then serves dist/ via preview)
pnpm visual:compare   # verify captured e2e step screenshots against corpus geometry
pnpm build            # production bundle (budget-gated)
pnpm tauri dev        # full Tauri shell (needs Rust toolchain)
pnpm tauri build      # NSIS installer (unsigned; see release workflow)
```

## The connector

The UI talks to exactly ONE backend: `src/connector/tauri/tauri-connector.ts`
over the 33 Tauri commands of `contracts/command-surface.json`. The Rust
bridge (`src-tauri/src/{bridge,commands}/`) binds those commands to the
cr_ui engine ABI (`engine/include/cr/cr_ui.h` at the repository root). Test
suites run against the labeled double (`tests/fakes/test-connector.ts`)
through a Vite alias that activates ONLY in test mode (`vitest` runs and
`vite build --mode test`) — production builds can never reach it
(enforced by arch-lint A7 + verified in CI).

The contract is specified in `src/connector/contract.ts` and documented in
`docs/CONNECTOR-CONTRACT.md`; integration gaps are recorded truthfully in
`docs/INTEGRATION-GAPS.md` at the repository root.

## Documentation

| Doc                           | Contents                                                            |
| ----------------------------- | ------------------------------------------------------------------- |
| `docs/AUDIT-REPORT.md`        | Full corpus-to-application parity audit + 55-parameter grid + fixes |
| `docs/screen-manifest.md`     | Screen & flow manifest derived from the corpus                      |
| `docs/visual-spec.md`         | Visual parameters specification with corpus citations               |
| `docs/component-inventory.md` | Complete component inventory derived from the reference             |
| `docs/ACCEPTANCE.md`          | Session verdicts: the 55-parameter grid per surface                 |
| `docs/SCREENSHOT-MANIFEST.md` | The 63-capture corpus inventory + VLM evidence index                |
| `docs/VISUAL-SPEC.md`         | Design tokens with per-value sources + §A11 mapping                 |
| `docs/UI-MAP.md`              | Route graph, per-page state matrices, dialog inventory              |
| `docs/DESIGN-SYSTEM.md`       | Living system doc                                                   |
| `docs/ARCHITECTURE.md`        | Layers, boundaries, data flow                                       |
| `docs/adr/`                   | Seven accepted architecture decisions                               |
| `docs/TESTING.md`             | Test strategy + verification evidence                               |
| `docs/INTERIM-ASSETS.md`      | Interim assets + replacement plan                                   |
| `docs/ROADMAP.md`             | Roadmap (engine integration: shipped)                               |

The visual identity comes from the adopted theme pair (`src/theme/adopted/`,
ADR-007 — five palettes + derived high-contrast, every color token-driven).
Layout, structure, and flow come from the reference corpus: selecting a
device starts the search directly, and the quick pass continues into the
advanced pass automatically (ADR-008). The custom title bar keeps the
Buy-Now pill and activation key trigger; Settings, Theme, and About live
behind the three-lines header Menu, while language switching is its own
header dropdown.

## Quality gates (all green at HEAD)

`tsc --noEmit` · ESLint (0 errors) · Prettier · arch-lint + ast-grep · knip
(0 findings) · jscpd (0.03% duplicated lines, 1 clone) · 145/145
unit/contract/component tests · v8 coverage thresholds 70/60/70/70 at
85.2% lines / 62.38% branches / 74.58% functions / 86% statements ·
theme-drift + i18n-surface governance tests · production build · bundle
budget (main 124.6 KB / 200 KB gzip; total 309.2 KB / 500 KB) · 12/12
Playwright e2e tests with console-error assertions · live axe audit: 0
violations across all palettes + HC (`docs/evidence/a11y/`).

CI runs the full matrix on `windows-latest` (`.github/workflows/`), including
`cargo check` + `clippy` for the Tauri shell and `visual-compare.yml` for
step-screenshot verification.

## License

TBD — no license granted yet; all rights reserved by the owner until decided.
Third-party runtime components ship under their own licenses (Radix MIT,
Lucide ISC, TanStack MIT, React MIT, i18next MIT, Tailwind MIT).
