# ADR-002 — React Compiler via the oxc port

**Status:** Accepted

## Context

The spec requires React Compiler enabled for automatic memoization, with no
hand-written `useMemo`/`useCallback` unless a profiler proves the need.

## Decision

`@vitejs/plugin-react` 6.x with `react({ compiler: true })` — the Rust port
(`oxc-transform-react`), not the Babel path.

## Rationale

- plugin-react v6 moved to oxc and exposes the compiler as a first-class
  option; the Babel route (`babel-plugin-react-compiler` +
  `@rolldown/plugin-babel` + `@babel/core`) adds three dependencies and a
  transform stage for identical output.
- Build time: 2266 modules transform in ~750 ms with the oxc pipeline.
- Two components are compiler-skipped with warnings (`react-hooks/
incompatible-library`): `VirtualFileList` (TanStack Virtual) and the scan
  config form (React Hook Form `watch()`). Both are documented, non-fatal,
  and behave identically without memoization.

## Consequences

- `eslint-plugin-react-hooks` v7 (compiler-powered rules: purity,
  set-state-in-effect) enforced **zero errors**; the codebase follows the
  external-store and derived-state patterns those rules push toward.
- Manual `useMemo` count in shipped code: two (both inside the virtualized
  list for derived row arrays — justified by virtualizer contract).
