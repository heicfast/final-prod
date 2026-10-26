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
| contracts-ci | every push since `a790cf9b` | validate.mjs phase 1+2 + filefilter freshness (34 commands / 33 invoked since `f47463ec`) |
| ui-ci | `b671348f` (green) | typecheck, eslint, prettier, arch-lint A1–A7, ast-grep ×2, knip, jscpd, vitest 130/130, coverage gates, build + bundle budget, cargo check --tests (incl. the T14 measurement modules) + clippy -D warnings |
| driver-ci | `a790cf9b` (green, first run) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matrix + unsigned assertion |
| engine-ci | `a790cf9b` (6/6 green) | lint, Linux ASan full suite, UBSan, fuzz, Windows MSVC + Win7 PE gates + ctest, **TSan** — the original job wedged >9h on one step and was cancelled; the fresh attempt completed GREEN (same tree, no code change) |
| ui-e2e | `b671348f` (green) | Playwright 14 specs on the test-mode build (the G-1 consent prompt suite included; the keyboard-flow spec's latent mount-race fixed — its either-name regex now pins the page h1) |

## Pending evidence (tracked, not claimed)

- **Runtime install/uninstall/update/repair matrix on a clean Windows
  runner** (T15): installer transcript, before/after directory + registry
  captures, session-preservation proof across update, repair-over-corruption.
- **Measured latency/throughput vs the retired simulator's documented
  budgets** (T14): RECORDED — `t14-measurements/` (run 40385809220 at
  `55cb76f4`): enumerate p50 9.6 ms vs the 180±60 listDevices budget;
  preview sub-ms vs 240±160; queryResults within (the scoped+search
  worst case 137.7 ns/node = 138 ms at 10^6 — the window's top edge,
  recorded); health unsupported on virtual-disk runners (needs physical
  hardware); startScan prep timing added to the harness for the next run.
- **Memory bounds**: RECORDED — same run: NodeMeta cache 213 B/node
  (213.6 MB at 10^6), harness peak 2.42 GB over the 1.9 M-node tree, the
  static caps pinned (4,096-row bad-sector map, 512/64-deep channels).
- **RC release run**: `release.yml` artifacts + SHA256SUMS + the notes
  template rendered from a real tag.
