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
| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row ok — 15 modules laid down in `C:\Program Files\HeicFast\Recovery Wizard` + the driver set in `System32\drivers`, the development certificate imported into LocalMachine Root + TrustedPublisher, **the RDA service REGISTERED and RUNNING (status 4) with the dev-signed rda.sys**, the product key writes read back verbatim, a session marker survives reinstall, a corrupted `cr_ui.dll` hash is restored by rerun, and silent uninstall leaves ZERO leftovers (service, driver file, certificate from both stores, registry keys — sessions kept) | `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/` (validation-evidence run **40437334177** for the matrix rows; the fully-green re-run **40442056802** at `9e188154` confirms identically — install facts: cert Root+TP exit 0, `driverSigned: true`, `driverService: "Running"`, 15 files, 9.4 s)
| **T14 — measured latency/throughput vs the retired simulator's budgets, through the mandatory RDA driver**: run **40442056802** at `9e188154` — the driver development-signed, staged, RUNNING, `\\.\RDA` proven to open, the engine RDA-only (no fallback exists to this day). enumerate p50 7.9 ms vs the 180±60 budget; startScan prep **5.5 ms** vs 420±120; preview open+read+metadata **0.056 ms total** vs 240±160; queryResults within (scoped+search worst case 124 ns/node = 124 ms at 10^6 — the window's top edge, recorded); full quick-stage scan of the 274 GB volume **72.1 s / 1.9 M files through the driver**; health unsupported on virtual-disk runners (needs physical hardware). The pre-mandatory run 17 (direct transport, driver not loaded) measured within noise — the RDA path performs on par with the retired direct path | `docs/INTEGRATION-VALIDATION/t14-measurements/` (run 40442056802 primary; run 40422119022 kept as the pre-mandatory comparison inside `t14-measurements.md`) |
| **Memory bounds**: NodeMeta cache 213 B/node (213.6 MB at 10^6), harness peak 2.42 GB over the 1.9 M-node tree, the static caps pinned (4,096-row bad-sector map, 512/64-deep channels) | same run — `t14-measurements.md` "Memory bounds" table |
| **The shipped kit carries zero test runtimes** (the gtest/gmock exclusion in build.ps1's kit curation) | the current run's install facts row `testDlls: 0` (matrix.json, run 40442056802; the null-count artifact that once read 1 was fixed at run 15) |

## CI green state (verifiable in Actions history)

| Lane | State at | Notes |
|---|---|---|
| contracts-ci | green at `b9d1a366` (the `ERR_DRIVER_REQUIRED` vocabulary commit) | validate.mjs phase 1+2 + filefilter freshness (34 commands / 33 invoked) — the new error code cross-checked end to end |
| ui-ci | green at `b9d1a366` | typecheck, eslint, prettier, arch-lint A1–A7, ast-grep ×2, knip, jscpd, vitest, coverage gates, build + bundle budget, cargo check --tests (incl. the T14 measurement modules) + clippy -D warnings — the driver-required i18n string included |
| driver-ci | green at `00f5cae5` (the latest commit touching its domain) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matrix + **the dev-sign smoke leg** (devsign.ps1 signs the WDK-built rda.sys, signtool verify passes, nothing committed) |
| engine-ci | green at `1a7d05d5` (the latest commit touching engine/ — the mandatory-driver rewrite tree) | lint, Linux ASan full suite (926 tests), UBSan, fuzz, Windows MSVC + Win7 PE gates + ctest — RDA-only transport, `ERR_DRIVER_REQUIRED` refusal covered |
| ui-e2e | green at `b9d1a366` | Playwright specs on the test-mode build (the G-1 consent prompt suite included) |
| validation-evidence | **run 40442056802 at `9e188154` GREEN — the first fully-green validation on the mandatory-driver tree** (every earlier round root-caused: the devsign `-File` param, the WOW64 redirection that had broken EVERY driver start, the `0x80000000` literal, the dropped msvc-dev-cmd step) | T14 + T15 end to end on the same tree the release cuts from: driver dev-signed and RUNNING on the runner, measurements through it, full installer matrix green |
| release | **`v0.5.0` run 4 GREEN** (unsigned era, history); **`v0.6.0-rc1` — the first DEVELOPMENT TEST-SIGNED release, cut on the run-23-validated tree (tag at `2cc3c501`, code `9e188154`)** | the full build WITH tests, artifact verification (the setup.exe AND the staged `rda.sys` must carry the build certificate — `devsign.ps1 -Action verify` throws on absence), truthful dev-signing notes (mandatory driver, Secure Boot guidance, testsigning.cer asset), upload with SHA256SUMS + the public `-testsigning.cer` |

## Pending evidence (tracked, not claimed)

- **Health on physical hardware**: SMART passthrough needs a bare-metal
  runner; recorded as the honest hardware gap in
  `t14-measurements/t14-measurements.md`.

The `v0.6.0-rc1` release run (tag at `2cc3c501`) is the first
**development test-signed** release: the driver, the engine kit, and the
installer all carry the build certificate; the public certificate ships
as a release asset; the notes state the Secure-Boot/testsigning
requirement truthfully. The v0.5.0 rows above read as history (the
unsigned era).
