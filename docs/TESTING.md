# TESTING — the suites and the validation evidence

## Suite map (what runs where)

| Suite | Scope | Lane |
|---|---|---|
| Engine: 900+ unit/integration tests | pure core: parsers, carve, sessions, report, query | engine-ci (Linux gcc ASan; Windows MSVC + ctest) |
| Engine: TSan + UBSan lanes | threading + UB discipline | engine-ci |
| Engine: 15 fuzz harnesses | crash-freedom smoke (45s/target) | engine-ci (fuzz) |
| Engine: ui_abi_check.c | the C-consumer proof of the cr_ui ABI | engine-ci (Windows) |
| Engine: Win7 PE gates | OS 6.1 stamp + import allowlist | engine-ci (Windows) |
| Engine: elevation refusal probes | raw-disk verbs refuse `ERR_ELEVATION_REQUIRED` (0x500C0001) under a non-elevated token; `rrecov` exits 4 naming the remedy | engine-ci |
| Installed product: the exact module set laid down in `C:\Program Files\HeicFast\Recovery Wizard` (flat — app + engine kit + uninstallers), ProgramData state, product registry keys read back; uninstall leaves ZERO leftovers (no service, no driver file, no certificates, no registry keys — sessions kept) | the installer | validation-evidence (T15) |
| Contracts: validate.mjs (phase 1 + 2) | boundary truth; command manifest ↔ Rust ↔ TS cross-check | contracts-ci |
| Contracts: filefilter freshness | the bridge's 1,481-entry mirror == the engine's table | contracts-ci |
| UI: vitest 174 tests (24 files) | unit + component + i18n parity/sweep + the connector contract test (incl. the G-1 consent surface) | ui-ci |
| UI: coverage thresholds | 70/60/70/70 gates; measured on the P4 tree — all files 84.7/63.3/76.5/85.6, connector/tauri 84.2/79.4/88.9/87.9 (run 42109647269 step 14) | ui-ci |
| UI: cargo check + clippy -D warnings | the bridge compiles clean under strict lints | ui-ci |
| UI: arch-lint A1–A7 + ast-grep x2 + knip + jscpd | layering, no raw invoke outside connector, no test-double imports, no dead code, no duplication | ui-ci |
| UI: Playwright 14 specs | golden path (scan → results → recover), chrome, dialogs, responsive, the G-1 consent prompt (blocked-walk deny + state re-hydration approve) | ui-e2e (test-mode build) |

## The connector contract test

`ui/tests/connector-tauri.test.ts` pins the REAL connector against a spied
IPC boundary: every method invokes the exact manifest command with the
exact arguments; rejections surface the mapped `ConnectorError` (code +
reasonKey + detail.code for engine numerics); `REASON_KEYS` is checked
against the locale catalogs; subscriptions return the contract's
unsubscribe shape. Engine BEHAVIOR (real scans, real recoveries) is the
Windows bridge lane + the release validation's business — this test guards
the seam.

## Test doubles (clearly labeled, never shipped)

`ui/tests/fakes/test-connector.ts` implements the contract against small
deterministic fixtures. It is wired ONLY through the Vite test-mode alias
(vitest runs, `vite build --mode test`); production builds cannot reach it
(arch-lint A7; CI verifies the bundle contents both ways). The double
fails EXACTLY where production fails — the G-1 repair gaps refuse
identically — never better.

## Validation evidence

`docs/INTEGRATION-VALIDATION.md` indexes the evidence artifacts under
`docs/INTEGRATION-VALIDATION/` (installer compile validation is committed;
the runtime install/uninstall/update matrix and the measured
latency/throughput comparisons land with the clean-runner validation run).
Rules: every "validated" statement cites the artifact that proves it.
