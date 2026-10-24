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
pnpm dev              # browser dev mode (mock connector, port 1420)
pnpm test             # 52 unit/contract/component tests
pnpm e2e              # Playwright golden path (serves dist/ via preview)
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

| Doc                           | Contents                                               |
| ----------------------------- | ------------------------------------------------------ |
| `docs/SCREENSHOT-MANIFEST.md` | The 63-capture corpus inventory                        |
| `docs/VISUAL-SPEC.md`         | Design tokens with per-value sources                   |
| `docs/UI-MAP.md`              | Route graph, per-page state matrices, dialog inventory |
| `docs/DESIGN-SYSTEM.md`       | Living system doc                                      |
| `docs/ARCHITECTURE.md`        | Layers, boundaries, data flow                          |
| `docs/adr/`                   | Five accepted architecture decisions                   |
| `docs/TESTING.md`             | Test strategy + verification evidence                  |
| `docs/VISUAL-ACCEPTANCE.md`   | The 55-parameter grid per surface                      |
| `docs/ROADMAP.md`             | Next session: wiring the real engine                   |

## Quality gates (all green at v0.1.0)

`tsc --noEmit` · ESLint (0 errors) · Prettier · arch-lint + ast-grep · knip ·
jscpd · 52/52 tests · production build · bundle budget (main 86 KB / 200 KB
gzip; total 301 KB / 500 KB) · Playwright golden path with console-error
assertions · browser-verified end-to-end (evidence screenshots in worklog).

CI runs the full matrix on `windows-latest` (`.github/workflows/`), including
`cargo check` + `clippy` for the Tauri shell.

## License

TBD — no license granted yet; all rights reserved by the owner until decided.
Third-party runtime components ship under their own licenses (Radix MIT,
Lucide ISC, TanStack MIT, React MIT, i18next MIT, Tailwind MIT).
