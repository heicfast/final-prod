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
| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row ok — the app + engine kit land in `C:\Program Files\HeicFast\Recovery Wizard`, the development certificate imports into `LocalMachine\Root` + `TrustedPublisher` (both certutil exits 0), the RDA service registers AND **RUNS** (the test-signed driver loads on the runner; `driverService: "Running"`, `driverSigned: true`), the product key writes read back verbatim, a session marker survives reinstall, a corrupted `cr_ui.dll` hash is restored by rerun, and silent uninstall leaves ZERO leftovers with sessions kept — including the certificate removed from BOTH stores and the real `System32\drivers\RDA.sys` gone (the WOW64 fix made that row non-vacuous) | `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/` (validation-evidence run **40442056802**, GREEN, at `9e1881541`; `matrix.json` + the per-phase JSON + transcripts + the installer's own lifecycle facts file `02-install-facts.txt` / `08-uninstall-facts.txt` with the cert + ImagePath + `sc start exit=1056` rows) |
| **T14 — measured latency/throughput vs the retired simulator's budgets**: enumerate p50 7.9 ms (p95 130.9 ms — inside the 120–240 ms window) vs the 180±60 budget; startScan prep **5.5 ms** vs 420±120 (4.1–5.9 ms across runs 10/15/17/23 — the prep-timer gap CLOSED); preview sub-ms vs 240±160; queryResults within (the scoped+search worst case 123.9 ns/node = 124 ms at 10^6, the wide pass 4.2 ns/node; rows paged 0.097 ms/500 rows; snapshot build 1.70 s once per generation); health unsupported on virtual-disk runners (needs physical hardware) | `docs/INTEGRATION-VALIDATION/t14-measurements/` (run **40442056802** at `9e1881541`; earlier runs 40385809220 / 40413941883 / 40422119022 within noise) |
| **Memory bounds**: NodeMeta cache 213 B/node (213.6 MB at 10^6), harness peak 2.42 GB over the 1.9 M-node tree, the static caps pinned (4,096-row bad-sector map, 512/64-deep channels) | same run — `t14-measurements.md` "Memory bounds" table |
| **The shipped kit carries zero test runtimes** (the gtest/gmock exclusion in build.ps1's kit curation) | runs 17 AND 23's install facts row `testDlls: 0` (runs 14/15 proved the exclusion; run 15's "1" was the null-count artifact, fixed) |

## CI green state (verifiable in Actions history)

| Lane | State at | Notes |
|---|---|---|
| contracts-ci | every push since `a790cf9b` | validate.mjs phase 1+2 + filefilter freshness (34 commands / 33 invoked since `f47463ec`) |
| ui-ci | `719041b2` (green) | typecheck, eslint, prettier, arch-lint A1–A7, ast-grep ×2, knip, jscpd, vitest 130/130, coverage gates, build + bundle budget, cargo check --tests (incl. the T14 measurement modules) + clippy -D warnings |
| driver-ci | `00f5cae5` (green, run 7 — incl. the WOW64 driver-script fix) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matrix + unsigned assertion + the dev-sign smoke leg (cert generation + embed-signature + signer verification on the freshly-built `rda.sys`, both WDK legs) |
| engine-ci | `1a7d05d5` (green, run 6) | lint, Linux ASan full suite, UBSan, fuzz, Windows MSVC + Win7 PE gates + ctest — under the single-deployer backstop (VCPKG_APPLOCAL_DEPS=OFF + retry-hardened copies) and the `-C Release` test preset |
| ui-e2e | `719041b2` (green) | Playwright 14 specs on the test-mode build (the G-1 consent prompt suite included) |
| validation-evidence | **run 23 at `9e1881541` GREEN** (runs 16/17 GREEN on the 0.5.0 tree; rounds 18–22 are the mandatory-driver saga, each failure root-caused: the WOW64 redirection bug, the probe's uint cast, the `GENERIC_READ` literal, the dropped msvc-dev-cmd step) | T14 + T15 end to end on the dev-signing tree: the driver INSTALLED + RUNNING on the runner, the certificate in both stores, the whole lifecycle matrix green |
| release | **run 4 at `v0.5.0` GREEN** (unsigned era); **`v0.6.0-rc1` — the first DEVELOPMENT TEST-SIGNED release, cut on the run-23-validated tree** | the full build WITH tests, artifact verification (the setup.exe AND the staged `rda.sys` must carry the build certificate), truthful dev-signing notes, upload with SHA256SUMS + the public `-testsigning.cer` asset |

## Pending evidence (tracked, not claimed)

- **Health on physical hardware**: SMART passthrough needs a bare-metal
  runner; recorded as the honest hardware gap in
  `t14-measurements/t14-measurements.md`.

The final `v0.5.0` release run (run 4, at `6fb8f658`) completed GREEN;
its artifacts and SHA256SUMS are published on the release (the RC rows in
the table above read as history: rc1 engine ctest gate, rc2 deployer
race, rc3 green). The `v0.6.0-rc1` tag continues the line as the first
release whose driver, engine kit and installer are DEVELOPMENT
TEST-SIGNED, cut on the tree that run 23 validated end to end.
