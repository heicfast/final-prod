# VULNERABILITIES.md — Phase-0 baseline vulnerability scan

- **Repo:** `heicfast/final-prod`, branch `main`, HEAD `126ea398ffa3123c2ff643b196deb1a7b030baab`
- **Scan date:** 2026-11-26 (agent E, release-engineering Phase-0 baseline)
- **Scope:** every dependency/secret/static-analysis finding returned by a runnable scanner, each with severity, affected surface, and fix version. Zero findings are stated as *measured* zeros, with the scanner that measured them.
- **Raw outputs:** `/tmp/vuln/` (`dependabot-open.json`, `dependabot-all.json`, `codescanning-open.json`, `secretscanning-open.json`, `pnpm-audit.txt`, `pnpm-audit-clean.json`, `cargo-install.log`, `cargo-audit-main.txt`, `cargo-audit-it.txt`, `osv.txt`, `osv-stdout.json`).

## 1. Summary — severity counts

Severity convention (CVSS base score → band): 9.0+ critical, 7.0–8.9 high, 4.0–6.9 medium, 0.1–3.9 low. Advisories with no CVSS in any database keep their source label (unrated / unmaintained / yanked) and are shown in separate rows, not silently folded into a band.

### Deduplicated (unique advisories, counted once regardless of how many scanners/lockfiles report them)

| Severity | Shipped app (`ui/src-tauri/Cargo.lock`) | ui build/dev toolchain (`ui/pnpm-lock.yaml`, dev-deps) | CI-only test crate (`ui/src-tauri/it/Cargo.lock`) | Reference trees (engine/bench/og, diff-to-fix, third_party/stb) |
| --- | --- | --- | --- | --- |
| Critical | 0 | 0 | 0 | 0 |
| High | 0 | 0 | 0 | 0 |
| Medium | 2 (cookie, glib) | 1 (minimatch) | 2 (cookie, glib) | 0 |
| Low | 0 | 0 | 0 | 0 |
| Unrated vulnerability (no CVSS published) | 1 (url) | 0 | 1 (url) | 0 |
| Unmaintained-crate warning (RustSec info-class) | 5 | 0 | 5 | 0 |
| Yanked crate warning (not a security advisory) | 1 (async-executor) | 0 | 0 | 0 |
| **Total unique advisories** | **8 + 1 yank note** | **1** | **8 (same set as app lockfile)** | **0 (measured zero, osv-scanner full walk)** |

Notes on the split:

- `ui/src-tauri/Cargo.lock` is the dependency set of the shipped Tauri desktop app (`recovery-wizard-ui 0.9.6`). Every Rust finding below is **shipped-surface**.
- `ui/src-tauri/it/Cargo.lock` is the `bridge-it 0.9.4` integration-gate crate: per its own Cargo.toml it is "only built by `.github/workflows/bridge-integration.yml`" — a **CI/test surface, not shipped** in the installer. It duplicates the full app advisory set (its lockfile includes the app as a path dep) minus the `async-executor` yank.
- `minimatch` is a **devDependency** of `ui/` (reached only through the eslint toolchain: `eslint`, `typescript-eslint`, `eslint-plugin-import-x`, `eslint-plugin-react-hooks`, `@eslint/js`; pnpm reports the finding as `dev: true`, and `pnpm audit --prod` reports "No known vulnerabilities found"). It never enters the packaged app.
- Reference trees: osv-scanner's recursive walk visited 249 dirs / 3543 inodes across the whole repo and extracted the only manifests present there (`engine/bench/og/FlaUiProbe/FlaUiProbe.csproj` → 2 NuGet packages, 0 vulns; `engine/third_party/stb` → 0 packages). **Zero reference-only findings — a measured zero.** osv-scanner did not error on any vendored manifest (nothing to record as an error).

### As reported per scanner / per lockfile occurrence (verbatim scanner counts)

| Scanner | Result |
| --- | --- |
| osv-scanner (whole repo, both Cargo.locks + pnpm-lock) | 17 rows: "0 Critical, 0 High, 5 Medium, 0 Low, 12 Unknown … 7 vulnerabilities can be fixed" |
| pnpm audit (`ui/`, full tree) | "1 vulnerabilities found — Severity: 1 moderate" |
| pnpm audit --prod | "No known vulnerabilities found" |
| cargo-audit (`ui/src-tauri/Cargo.lock`, 471 deps) | "error: 2 vulnerabilities found!" + "warning: 7 allowed warnings found" |
| cargo-audit (`ui/src-tauri/it/Cargo.lock`, 484 deps) | "error: 2 vulnerabilities found!" + "warning: 6 allowed warnings found" |
| GitHub secret scanning | 0 open alerts (measured) |
| GitHub Dependabot | alerts **disabled** on the repo (API message; not a measured zero) |
| GitHub code scanning | "no analysis found" (never configured; not a measured zero) |

## 2. Scanner inventory (what ran, versions, evidence)

| Scanner | Version | Target(s) | Raw output |
| --- | --- | --- | --- |
| GitHub REST API (auth) | — | dependabot alerts (open + all), code-scanning (open), secret-scanning (open) | `/tmp/vuln/dependabot-open.json`, `dependabot-all.json`, `codescanning-open.json`, `secretscanning-open.json` |
| pnpm audit | pnpm 12.15.0 | `ui/pnpm-lock.yaml` (562 deps: 72 prod / 476 dev / 203 optional) | `/tmp/vuln/pnpm-audit.txt`, `pnpm-audit-clean.json` |
| cargo-audit | 0.23.0 (RustSec advisory DB, 1568 advisories, fetched 2026-11-26) | both `Cargo.lock` files in the repo | `/tmp/vuln/cargo-audit-main.txt`, `cargo-audit-it.txt` |
| osv-scanner (Google OSV) | v2.7.1 (osv-scalibr 0.5.3, built 2026-11-09) | recursive walk of the whole repo | `/tmp/vuln/osv.txt`, `osv-stdout.json` |

Reproducing commands (toolchain env: `PATH=/home/z/my-project/.toolchain/pnpm-global/bin` + `/home/z/my-project/.toolchain/cargo/bin`, `CARGO_HOME=/home/z/my-project/.toolchain/cargo`, `RUSTUP_HOME=/home/z/my-project/.toolchain/rustup`):

```bash
# pnpm
cd ui && pnpm audit            # and: pnpm audit --prod
# cargo-audit
cargo audit --file ui/src-tauri/Cargo.lock
cargo audit --file ui/src-tauri/it/Cargo.lock
# osv
/home/z/my-project/.toolchain/bin/osv-scanner -r /home/z/my-project/final-prod --format table
```

## 3. Findings — shipped surface (app + its build toolchain)

### 3.1 Rust — the shipped Tauri app (`ui/src-tauri/Cargo.lock`, `recovery-wizard-ui 0.9.6`)

All 8 advisories also appear identically in `ui/src-tauri/it/Cargo.lock` (bridge-it CI gate), because that lockfile contains the same resolved versions. Sources: cargo-audit + osv-scanner both, unless noted.

| # | Crate + version | Advisory | Severity | Fix / mitigation | Reverse dependency (entry path into the app) |
| --- | --- | --- | --- | --- | --- |
| R1 | `cookie 0.18.2` | [RUSTSEC-2026-0594](https://rustsec.org/advisories/RUSTSEC-2026-0594) = [GHSA-8wcq-xq25-7fhm](https://github.com/advisories/GHSA-8wcq-xq25-7fhm), published 2026-11-23 | **Medium** (cargo-audit: 6.5 medium; CVSS:3.1 AV:N/AC:H/PR:N/UI:N/S:U/C:L/I:H/A:N) | Upgrade to `>=0.18.3` | `tauri 2.12.5` / `tauri-runtime 2.12.2` / `wry 0.57.1` (WebView layer). RustSec scope note: only applications that `fork()` then add encrypted cookies (`PrivateJar::add`) are affected — a Tauri desktop app does not fork, so exploitability here is low, but the pin is still out of policy. |
| R2 | `url 2.5.8` | [RUSTSEC-2026-0592](https://rustsec.org/advisories/RUSTSEC-2026-0592) = [GHSA-hrj5-67jp-wq8g](https://github.com/advisories/GHSA-hrj5-67jp-wq8g), published 2026-11-20 | **Unrated vulnerability** (no CVSS published in RustSec or OSV; cargo-audit counts it as one of its 2 "vulnerabilities", osv-scanner classes it "Unknown") | Upgrade to `>=2.5.9` | `reqwest 0.13.5` (via `tauri 2.12.5`) and `schemars 0.8.22` / `serde_with 3.24.0` (via `tauri-utils 2.10.2`). Stack overflow in `Url::origin()` on attacker-controlled nested `blob:` URLs — relevant if the app ever parses untrusted URLs. |
| R3 | `glib 0.18.5` | [RUSTSEC-2024-0429](https://rustsec.org/advisories/RUSTSEC-2024-0429) = [GHSA-wrw7-89jp-8q8g](https://github.com/advisories/GHSA-wrw7-89jp-8q8g), published 2024-03-30 | **Medium** (OSV/GitHub DB: MODERATE, CVSS:4.0 6.9; cargo-audit class: "unsound" warning) | Upgrade to `>=0.20.0` | Linux GUI stack: `gtk 0.18.2` / `libappindicator 0.9.0` / `tray-icon 0.25.1` / `muda 0.20.0` / `tao 0.37.2` (all via `tauri 2.12.5`). Unsound `Iterator` impl for `VariantStrIter` (UB). |
| R4 | `chrono 0.4.45` | [RUSTSEC-2026-0543](https://rustsec.org/advisories/RUSTSEC-2026-0543), published 2026-09-14 | **Unmaintained** (RustSec info-class; no CVSS) | No fixed upgrade. RustSec recommends migrating to `jiff` or `time`. | `serde_with 3.24.0` → `tauri-utils 2.10.2` → `tauri 2.12.5`. |
| R5 | `proc-macro-error 1.0.4` | [RUSTSEC-2024-0370](https://rustsec.org/advisories/RUSTSEC-2024-0370), published 2024-09-01 | **Unmaintained** (no CVSS) | No fixed upgrade. | `glib-macros 0.18.5` → `glib 0.18.5` (Linux GUI stack, as R3). |
| R6 | `zbus_names 4.3.5` | [RUSTSEC-2026-0371](https://rustsec.org/advisories/RUSTSEC-2026-0371), published 2026-10-16 | **Unmaintained** (no CVSS) | No fixed upgrade; crate merged into `zbus >=6.0.0` (`names` module). | `zbus 5.19.0` → `tauri-plugin-opener 2.7.2` (Linux portal opener). |
| R7 | `zvariant 5.15.1` | [RUSTSEC-2026-0370](https://rustsec.org/advisories/RUSTSEC-2026-0370), published 2026-10-16 | **Unmaintained** (no CVSS) | No fixed upgrade; merged into `zbus >=6.0.0` (`wire` module). | `zbus 5.19.0` → `tauri-plugin-opener 2.7.2`; also `zbus_macros 5.19.0`, `zbus_names 4.3.5`. |
| R8 | `zvariant_derive 5.15.1` | [RUSTSEC-2026-0372](https://rustsec.org/advisories/RUSTSEC-2026-0372), published 2026-10-16 | **Unmaintained** (no CVSS) | No fixed upgrade; derive macros provided by `zbus >=6.0.0`. | Same chain as R6/R7. |
| N1 | `async-executor 1.14.0` | no advisory — **yanked** from crates.io (cargo-audit warning; osv-scanner does not flag yanks) | Not a security advisory | Use a non-yanked version (latest on crates.io) | Build/runtime executor pulled in by the tauri/async stack; present only in the app lockfile, not in bridge-it's. |

cargo-audit's exact bottom line per lockfile (verbatim): main — `error: 2 vulnerabilities found!` / `warning: 7 allowed warnings found`; bridge-it — `error: 2 vulnerabilities found!` / `warning: 6 allowed warnings found`. ("Allowed warnings" = RustSec warning-class findings not denied by default; no `audit.toml`/ignore config exists anywhere in the repo — verified.)

### 3.2 npm — ui build/dev toolchain (`ui/pnpm-lock.yaml`)

| # | Package + version | Advisory | Severity | Fix | Notes |
| --- | --- | --- | --- | --- | --- |
| N2 | `minimatch 10.2.6` | [GHSA-g3hv-75wg-m249](https://github.com/advisories/GHSA-g3hv-75wg-m249) = CVE-2026-118764 | **Medium** (pnpm: "moderate"; OSV/GitHub DB: MODERATE, CVSS:4.0 6.9; CWE-405) | `>=11.0.1` | DevDependency only (43 dependency paths, all through the eslint toolchain: `eslint` → `@eslint/config-array` → `minimatch`, plus `typescript-eslint`, `eslint-plugin-import-x`, `eslint-plugin-react-hooks`). DoS via globparts amplification in `firstPhasePreProcess()` with `optimizationLevel >= 2`. Never ships in the app bundle; `pnpm audit --prod` = clean. Discovered by **both** pnpm audit and osv-scanner (deduped to one). |

### 3.3 Reference trees (engine/bench/og, engine/diff-to-fix, engine/third_party, tools, installer, contracts)

**Zero findings — measured, not assumed.** osv-scanner walked the entire repo (249 dirs, 3543 inodes) and the only package manifests outside `ui/` were:

- `engine/bench/og/FlaUiProbe/FlaUiProbe.csproj` — 2 NuGet packages (`FlaUI.UIA3 4.0.0`, `FlaUI.Core 4.0.0`), 0 known vulnerabilities;
- `engine/third_party/stb` — 0 packages (header-only C/C++, no versioned manifest).

No osv-scanner errors on vendored manifests occurred. Note the reference `og` tree is reverse-engineering evidence that does not ship, so even its manifests are outside the shipped surface.

## 4. Cross-check / dedupe notes

- `minimatch` / GHSA-g3hv-75wg-m249: reported by pnpm audit **and** osv-scanner — one advisory, two sources.
- The 8 Rust advisories: reported by cargo-audit **and** osv-scanner, each in **both** Cargo.locks — one advisory each, counted once. `glib` appears under two IDs (RUSTSEC-2024-0429 and GHSA-wrw7-89jp-8q8g) in OSV; they are aliases of the same advisory.
- `async-executor` yank: cargo-audit only (OSV has no yank data). Main lockfile only.
- osv-scanner's raw row count (17 = 8 Rust advisories x 2 lockfiles + 1 npm) double-counts lockfiles; its "7 fixable" = cookie (x2) + glib (x2) + url (x2) + minimatch (x1).
- Discrepancies kept honest: cargo-audit treats `glib` as a warning ("unsound") while OSV/GitHub rate it MODERATE 6.9 — both labels are recorded above. `url` is a real vulnerability but carries no CVSS anywhere, so it stays "unrated" rather than being forced into a band.

## 5. Scanner-coverage statement (honest)

**Ran successfully (4 independent sources):**

1. GitHub REST API, authenticated — dependabot alerts (`state=open` and `state=all`), code-scanning alerts, secret-scanning alerts.
2. `pnpm audit` (pnpm 12.15.0) against `ui/pnpm-lock.yaml` — live npm registry advisories; also `pnpm audit --prod`.
3. `cargo-audit 0.23.0` (installed via `cargo install cargo-audit --locked`, log at `/tmp/vuln/cargo-install.log`) with the RustSec advisory database fetched 2026-11-26 (1568 advisories) — run against **every** `Cargo.lock` in the repo (exactly two: `ui/src-tauri/Cargo.lock`, `ui/src-tauri/it/Cargo.lock`; neither contains only path deps — 471 and 484 resolved crates respectively).
4. `osv-scanner v2.7.1` (linux-amd64 binary from the official GitHub release, installed at `/home/z/my-project/.toolchain/bin/osv-scanner`) — full-repo recursive walk covering npm, crates.io, and NuGet manifests.

**GitHub-native results (as measured):**

- Dependabot alerts: **disabled for this repository** (API returns "Dependabot alerts are disabled for this repository." for both open and all states). The "zero Dependabot alerts" is therefore *not* a measured zero — GitHub SCA is simply not running on this repo.
- Code scanning: API returns **"no analysis found"** — no CodeQL or other SARIF analyses have ever been uploaded. Zero static-analysis findings is *not configured*, not measured.
- Secret scanning: **enabled, 0 open alerts** — a genuine measured zero as of 2026-11-26.

**Could not run:**

- **OWASP Dependency-Check**: requires a Java runtime, which is not available in this sandbox (no `java`, no sudo). It is also **not used by this repo's CI** — none of the 11 workflows under `.github/workflows/` reference it (verified by grep). Impact is low: its npm/crates.io advisory feeds overlap heavily with what pnpm audit + cargo-audit + OSV already covered.
- **CodeQL**: not runnable here (needs the GitHub Actions setup or codeql CLI analysis of a build); the API confirms no results exist on the platform side either, so there are no platform CodeQL findings to catalog.

**Known blind spots (no scanner available):**

- **C/C++ engine dependencies** declared in `engine/vcpkg.json` (`zlib`, `gtest`, `lz4`, `zstd`) — no vcpkg lockfile or pinned versions exist in the repo (versions resolve at build time from the vcpkg registry), so no SCA tool has anything to audit. These were **not** scanned by anything and carry no measured status.
- The repo's **CI has no vulnerability-audit lane at all**: no workflow runs pnpm audit, cargo-audit, osv-scanner, CodeQL, or OWASP DC (grep over `.github/workflows/`). This sheet is therefore the first recorded dependency-vulnerability baseline for the repo.

## 6. Practical read for the release lane

- **Zero critical, zero high** across all runnable scanners, shipped and reference alike.
- The two findings with concrete patched versions inside the shipped app (`cookie 0.18.2 → 0.18.3`, `url 2.5.8 → 2.5.9`) are young advisories (2026-11-23 / 2026-11-20) with low exploitability in a non-forking desktop app; a routine `cargo update -p cookie -p url` closes both.
- `glib 0.18.5 → 0.20.0` needs the gtk stack (tao/gtk/libappindicator pins from tauri) to move first — typical of Linux-only tauri transitive pins.
- The five unmaintained crates and the yanked `async-executor` are hygiene items (RustSec warning class), not exploitable bugs; four of the five (zbus_names/zvariant/zvariant_derive + the zbus 5 line) resolve together by moving `tauri-plugin-opener`'s zbus stack to zbus 6.
- `minimatch` is lint-toolchain only; bump when the eslint stack refreshes (fix `>=11.0.1`).
