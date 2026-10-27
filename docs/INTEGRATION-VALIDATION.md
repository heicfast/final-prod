# INTEGRATION-VALIDATION — evidence index

Every "validated" claim cites the artifact that proves it. Evidence lives
under `docs/INTEGRATION-VALIDATION/`; the CI lanes' green runs are
verifiable via the repository's Actions history (per-run URLs cited from
the worklog).

## Committed evidence

| Claim | Artifact |
|---|---|
| The NSIS installer compiles clean (zero warnings) with its define-contract `!error` guards negative-tested | `docs/INTEGRATION-VALIDATION/installer-compile-check.md` (cross-compiled with makensis 3.11; re-validated after the /SD + registry fixes) |
| The bridge's extension→category mirror is exact (1,481 entries, byte-parity with the engine's generated table incl. the `dv-avi`/`dvr-ms`/`mp+` entries) | `ui/scripts/gen-filefilter.mjs --check` green in contracts-ci on every push since `a790cf9b` |
| The command manifest matches BOTH implementations | `contracts/validate.mjs` phase 2 green: "34 command implementations cross-checked", "33 invoked commands cross-checked" (contracts-ci runs; the manifest's `removed` array records the two dropped commands with rationale — `consent_respond` was re-added when the G-1 (a) prompt landed) |
| The test double never ships; production builds resolve the real connector | ui-ci bundle assertions + local verification: test-mode build contains the double's fixtures and zero real-connector strings; production build the reverse |
| The G-1 (a) consent flow (bridge gate + scan-page prompt + consent_respond round-trip) | `ui/src-tauri/src/bridge/consent.rs` + the connector contract tests (invoke shape, subscription, consent-not-found mapping) + `ui/e2e/consent.spec.ts` (blocked-walk deny + state re-hydration approve) + contracts/event-mapping.md's consent section |
| The Rust bridge compiles clean under strict clippy on the Windows toolchain | ui-ci green at `4c9f115a` (10-round iteration recorded in the worklog; every red's root cause fixed, zero suppressions beyond the documented abi.rs pinned-surface allow) |
| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row ok — the app + engine kit land in `C:\Program Files\HeicFast\Recovery Wizard`, the RDA service registers (Stopped: unsigned on stock runners, the documented degradation), the product key writes read back verbatim, a session marker survives reinstall, a corrupted `cr_ui.dll` hash is restored by rerun, and silent uninstall leaves ZERO leftovers with sessions kept | `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/` (validation-evidence run **40422119022**, GREEN, at `10da4cd6`; `matrix.json` + the per-phase JSON + transcripts + the installer's own lifecycle facts file `02-install-facts.txt` / `08-uninstall-facts.txt`) |
| **T14 — measured latency/throughput vs the retired simulator's budgets**: enumerate p50 6.6 ms vs the 180±60 budget; startScan prep **4.1 ms** vs 420±120 (the prep-timer gap CLOSED across runs 10/15/17); preview sub-ms vs 240±160; queryResults within (the scoped+search worst case 137.7 ns/node = 138 ms at 10^6 — the window's top edge, recorded); health unsupported on virtual-disk runners (needs physical hardware) | `docs/INTEGRATION-VALIDATION/t14-measurements/` (run 40422119022; earlier runs 40385809220 / 40413941883 within noise) |
| **Memory bounds**: NodeMeta cache 213 B/node (213.6 MB at 10^6), harness peak 2.42 GB over the 1.9 M-node tree, the static caps pinned (4,096-row bad-sector map, 512/64-deep channels) | same run — `t14-measurements.md` "Memory bounds" table |
| **The shipped kit carries zero test runtimes** (the gtest/gmock exclusion in build.ps1's kit curation) | run 17's install facts row `testDlls: 0` (runs 14/15 proved the exclusion; run 15's "1" was the null-count artifact, fixed) |

## CI green state (verifiable in Actions history)

| Lane | State at | Notes |
|---|---|---|
| contracts-ci | every push since `a790cf9b` | validate.mjs phase 1+2 + filefilter freshness (34 commands / 33 invoked since `f47463ec`) |
| ui-ci | `719041b2` (green) | typecheck, eslint, prettier, arch-lint A1–A7, ast-grep ×2, knip, jscpd, vitest 130/130, coverage gates, build + bundle budget, cargo check --tests (incl. the T14 measurement modules) + clippy -D warnings |
| driver-ci | `790bf5b3` (green) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matrix + unsigned assertion (the fixed elevation guards parse + the WDK build passes) |
| engine-ci | `790bf5b3` (green, run 3) | lint, Linux ASan full suite, UBSan, fuzz, Windows MSVC + Win7 PE gates + ctest — under the single-deployer backstop (VCPKG_APPLOCAL_DEPS=OFF + retry-hardened copies) and the `-C Release` test preset |
| ui-e2e | `719041b2` (green) | Playwright 14 specs on the test-mode build (the G-1 consent prompt suite included) |
| validation-evidence | **run 17 at `10da4cd6` GREEN** (also run 16 at `f97a8964`) | T14 + T15 end to end — the matrix's 11 failure rounds each root-caused and fixed for cause (the worklog records every round: silent MessageBox blocks, registry-key form, dead elevation guards, gtest in the kit, null-count checks, the -File exit-code propagation) |
| release | **run 3 at `v0.5.0-rc3` GREEN**; **run 4 at `v0.5.0` GREEN** | the full build WITH tests, artifact verification, truthful unsigned notes, upload with SHA256SUMS — the v0.5.0-rc3 release (prerelease-labeled) and the FINAL v0.5.0 release both exist with setup.exe + portable.zip + SHA256SUMS |

## Pending evidence (tracked, not claimed)

- **Health on physical hardware**: SMART passthrough needs a bare-metal
  runner; recorded as the honest hardware gap in
  `t14-measurements/t14-measurements.md`.

The final `v0.5.0` release run (run 4, at `6fb8f658`) completed GREEN —
its artifacts and SHA256SUMS are published on the release; the RC row in
the table above now reads as history (rc1: engine ctest gate; rc2:
deployer race; rc3: green).
