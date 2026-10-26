# INTEGRATION-VALIDATION — evidence index

Every "validated" claim cites the artifact that proves it. Evidence lives
under `docs/INTEGRATION-VALIDATION/`; the CI lanes' green runs are
verifiable via the repository's Actions history (per-run URLs cited from
the worklog).

## Committed evidence

| Claim | Artifact |
|---|---|
| The NSIS installer compiles clean (zero warnings) with its define-contract `!error` guards negative-tested | `docs/INTEGRATION-VALIDATION/installer-compile-check.md` (cross-compiled with makensis 3.12; runtime install matrix = the clean-runner validation below) |
| The bridge's extension→category mirror is exact (1,481 entries, byte-parity with the engine's generated table incl. the `dv-avi`/`dvr-ms`/`mp+` entries) | `ui/scripts/gen-filefilter.mjs --check` green in contracts-ci on every push since `a790cf9b` |
| The command manifest matches BOTH implementations | `contracts/validate.mjs` phase 2 green: "34 command implementations cross-checked", "33 invoked commands cross-checked" (contracts-ci runs; the manifest's `removed` array records the two dropped commands with rationale — `consent_respond` was re-added when the G-1 (a) prompt landed) |
| The test double never ships; production builds resolve the real connector | ui-ci bundle assertions + local verification: test-mode build contains the double's fixtures and zero real-connector strings; production build the reverse |
| The G-1 (a) consent flow (bridge gate + scan-page prompt + consent_respond round-trip) | `ui/src-tauri/src/bridge/consent.rs` + the connector contract tests (invoke shape, subscription, consent-not-found mapping) + `ui/e2e/consent.spec.ts` (blocked-walk deny + state re-hydration approve) + contracts/event-mapping.md's consent section |
| The Rust bridge compiles clean under strict clippy on the Windows toolchain | ui-ci green at `4c9f115a` (10-round iteration recorded in the worklog; every red's root cause fixed, zero suppressions beyond the documented abi.rs pinned-surface allow) |

## CI green state (verifiable in Actions history)

| Lane | State at | Notes |
|---|---|---|
| contracts-ci | every push since `a790cf9b` | validate.mjs phase 1+2 + filefilter freshness |
| ui-ci | `4c9f115a` (green) | typecheck, eslint, prettier, arch-lint A1–A7, ast-grep ×2, knip, jscpd, vitest 127/127, coverage 96.4/84.4/100/98.6, build + bundle budget, cargo check + clippy -D warnings |
| driver-ci | `a790cf9b` (green, first run) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matrix + unsigned assertion |
| engine-ci | `a790cf9b` (5/6 green at press; tsan in flight) | lint, Linux ASan full suite, UBSan, fuzz, Windows MSVC + Win7 PE gates + ctest |
| ui-e2e | first run pending | Playwright 12 specs on the test-mode build |

## Pending evidence (tracked, not claimed)

- **Runtime install/uninstall/update/repair matrix on a clean Windows
  runner** (T15): installer transcript, before/after directory + registry
  captures, session-preservation proof across update, repair-over-corruption.
- **Measured latency/throughput vs the retired simulator's documented
  budgets** (T14): listDevices 180±60 ms, startScan 420±120 ms,
  queryResults 90±50 ms, preview 240±160 ms, health 280±120 ms (its
  table was the comparison baseline); plus the G-7 filter cost over the
  metadata cache at 10^5/10^6 node scales.
- **Memory bounds**: snapshot O(n) copy, the metadata cache, the bounded
  event channels (512/64-deep), the 4,096-row bad-sector map cap.
- **RC release run**: `release.yml` artifacts + SHA256SUMS + the notes
  template rendered from a real tag.
