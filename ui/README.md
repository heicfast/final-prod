# Recovery Wizard UI

Production-grade **Tauri 2 + React 19 + TypeScript (strict) + Vite** desktop
UI for a data-recovery wizard — a clean-room re-implementation driven
entirely by a **Mock Connector** that implements the same command/event
contract the real recovery engine will expose. Swapping the simulator for a
real IPC bridge is a one-line environment change; no component or view
changes.

> Clean-room notice: all code, UI copy, and assets in this repository are
> original. The visual/interaction design was reconstructed from factual
> research parameters (layout structures, geometry, color roles) documented
> in `docs/VISUAL-SPEC.md` with per-token sources. No proprietary assets,
> code, or string tables were copied.

## Quick start

```bash
pnpm install          # pnpm 12.15.0, Node 24
pnpm dev              # browser dev mode (mock connector, port 1420 / 3000)
pnpm test             # 110 unit/contract/component/governance tests (16 test files)
pnpm test:coverage    # unit tests with enforced v8 coverage thresholds
pnpm e2e              # 12 Playwright e2e tests (serves dist/ via preview)
pnpm visual:compare   # verify captured e2e step screenshots against corpus geometry
pnpm build            # production bundle (budget-gated)
pnpm tauri dev        # full Tauri shell (needs Rust toolchain)
pnpm tauri build      # NSIS installer (unsigned; see release workflow)
```

## Connector mode

- `VITE_CONNECTOR=mock` (default) — deterministic simulator: 2 TB NTFS with
  1.2 M result nodes, 64 GB SD card, 4 TB Btrfs, lost-partition scenario,
  100–250 ms progress ticks, error injection via the dev panel (DEV builds:
  flask icon in the title bar).
- `VITE_CONNECTOR=tauri` — the real adapter. Currently a **loud stub** that
  throws `not-implemented` on every method, per the no-silent-fallback rule.

The contract is specified in `src/connector/contract.ts` and documented in
`docs/CONNECTOR-CONTRACT.md`.

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
| `docs/adr/`                   | Eight accepted architecture decisions                               |
| `docs/TESTING.md`             | Test strategy + verification evidence                               |
| `docs/INTERIM-ASSETS.md`      | Interim assets + replacement plan                                   |
| `docs/ROADMAP.md`             | Next: wiring the real engine                                        |

The visual identity comes from the adopted theme pair (`src/theme/adopted/`,
ADR-007 — five palettes + derived high-contrast, every color token-driven).
Layout, structure, and flow come from the reference corpus: selecting a
device starts the search directly, and the quick pass continues into the
advanced pass automatically (ADR-008).

## Quality gates (all green at HEAD)

`tsc --noEmit` · ESLint (0 errors) · Prettier · arch-lint + ast-grep · knip
(0 findings) · jscpd (0 clones) · 110/110 unit/contract/component tests ·
theme-drift + i18n-surface governance tests · production build · bundle
budget (main 98 KB / 200 KB gzip; total 283 KB / 500 KB) · 12/12 Playwright
e2e tests with console-error assertions · live axe audit: 0 violations across
all palettes + HC (`docs/evidence/a11y/`).

CI runs the full matrix on `windows-latest` (`.github/workflows/`), including
`cargo check` + `clippy` for the Tauri shell and `visual-compare.yml` for
step-screenshot verification.

## License

TBD — no license granted yet; all rights reserved by the owner until decided.
Third-party runtime components ship under their own licenses (Radix MIT,
Lucide ISC, TanStack MIT, React MIT, i18next MIT, Tailwind MIT).
