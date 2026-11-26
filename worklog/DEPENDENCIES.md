# DEPENDENCIES — Phase-0 baseline inventory (current vs latest stable upstream)

Repo: `final-prod` @ `126ea398` (branch `main`) — audit date: **2026-11-26**. Agent D, release-engineering Phase-0.

Method: every pinned/locked version was read from the tracked manifest files (`ui/package.json`, `ui/src-tauri/Cargo.toml` + `Cargo.lock`, `.github/workflows/*.yml`, `engine/CMakeLists.txt` / `CMakePresets.json` / `vcpkg.json`, `engine/third_party/`, `engine/bench/og/FlaUiProbe/FlaUiProbe.csproj`) and compared against the latest **stable** upstream release fetched live on 2026-11-26 from npm/crates.io/NuGet/GitHub Releases/nodejs.org/vcpkg-ports/runner-images. Prerelease dist-tags (next/beta/rc/alpha) were ignored per policy; GitHub Actions are pinned by immutable SHA — the SHA was resolved to its exact release tag via the tags API and compared to the repo's latest release.

## Summary

- **Total dependencies inventoried: 90** (25 UI runtime + 30 UI dev + 7 Rust/Tauri + 13 Actions/runner-images + 7 toolchains + 8 vendored/other)
- CURRENT: **30**
- MAJOR-BEHIND: **24**
- MINOR-BEHIND: **18**
- PATCH-BEHIND: **13**
- UNKNOWN (unpinned, no fixed version to compare): **5**

### Notable MAJOR-BEHIND items

| component | current | latest stable |
| --- | --- | --- |
| `i18next` | 26.5.6 | 27.2.9 |
| `react-i18next` | 17.1.4 | 18.0.3 |
| `react-router` | 7.18.4 | 8.5.3 |
| `@testing-library/jest-dom` | 7.0.1 | 8.0.0 |
| `@types/node` | 24.19.2 | 26.6.11 |
| `@vitest/coverage-v8` | 4.1.12 | 5.1.2 |
| `jsdom` | 30.1.3 | 31.0.1 |
| `typescript` | 5.9.3 | 7.0.2 |
| `vitest` | 4.1.12 | 5.1.2 |
| `actions/checkout` | v-tag v4.4.0 (@11d5960a3267) | v7.0.1 |
| `actions/cache` | v-tag v4.3.0 (@0057852bfaa8) | v6.1.0 |
| `actions/upload-artifact` | v-tag v4.6.2 (@ea165f8d65b6) | v7.0.2 |
| `actions/download-artifact` | v-tag v4.3.0 (@d3f86a106a0b) | v8.0.2 |
| `actions/setup-node` | v-tag v4.4.0 (@49933ea5288c) | v7.1.0 |
| `pnpm/action-setup` | v-tag v4.3.0 (@b906affcce14) | v6.1.1 |
| `softprops/action-gh-release` | v-tag v2.6.2 (@3bb12739c298) | v3.0.4 |
| runner image: `windows-2022` | image 20261116.370.1 — OS 10.0.20348.5870, VS2022 17.14.37906.3, CMake 3.31.6, Rust 1.100.0, Node 22.23.5/24.22.3, vcpkg @0df93a05b9 | windows-latest = Windows Server 2025 image 20261116.315.1 (VS2026 18.12.12405.243, CMake 4.5.0) |
| runner image: `ubuntu-24.04` | Ubuntu 24.04 LTS (contracts-ci.yml) | ubuntu-latest = Ubuntu 26.04 LTS |
| Node.js (engines / CI) | >=24 (engines floor); CI node-version: 24 → 24.22.3 (LTS Krypton) | 26.13.1 (LTS Lithium, 2026-11-10) |
| Node.js (forensic-repro.yml only) | node-version: '20' → 20.20.2 (EOL 2026-04-30) | 26.13.1 |
| CMake | floor 3.25 (engine/CMakeLists.txt); runner CMake 3.31.6 | v4.5.1 (2026-11-19) |
| MSVC / Visual Studio | VS2022 17.14.37906.3 (windows-2022; generator 'Visual Studio 17 2022', v143) | VS2026 18.12.12405.243 (windows-latest image) |
| FlaUI.Core / FlaUI.UIA3 (engine/bench/og/FlaUiProbe.csproj — reference-only, not built by CI) | 4.0.0 | 5.0.0 |
| .NET target (FlaUiProbe — reference-only) | net8.0-windows | .NET 11.0.0 (latest; .NET 8 EOL 2026-11-10) |

Headline: the Rust/Tauri side is essentially current (serde/serde_json exact-current, tauri stack only patch-behind, Rust stable 1.100.0). The JavaScript side is split: all Radix UI primitives, react/react-dom and the build core are current, but the test stack broke new ground upstream (vitest 5, jsdom 31, jest-dom 8, TypeScript 7) and the i18n/router trio is one major behind (i18next 27, react-i18next 18, react-router 8). The CI action layer is the oldest stratum: 7 of 10 actions are major-behind (checkout v4 vs v7, upload/download-artifact v4 vs v7/v8, setup-node v4 vs v7, cache v4 vs v6, pnpm/action-setup v4 vs v6, action-gh-release v2 vs v3), and most jobs still pin `windows-2022` while `windows-latest` now resolves to Windows Server 2025 + VS2026.

## (a) UI runtime dependencies (ui/package.json — dependencies)

25 entries, exact pins (no ranges); `ui/pnpm-lock.yaml` is in sync with every specifier. All latest values are `dist-tags.latest` (stable) from the npm registry.

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| `@radix-ui/react-checkbox` | 1.4.1 | 1.4.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-checkbox/latest |
| `@radix-ui/react-context-menu` | 2.4.1 | 2.4.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-context-menu/latest |
| `@radix-ui/react-dialog` | 1.3.1 | 1.3.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-dialog/latest |
| `@radix-ui/react-dropdown-menu` | 2.2.1 | 2.2.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-dropdown-menu/latest |
| `@radix-ui/react-label` | 2.1.17 | 2.1.17 | CURRENT | https://registry.npmjs.org/@radix-ui/react-label/latest |
| `@radix-ui/react-popover` | 1.3.1 | 1.3.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-popover/latest |
| `@radix-ui/react-progress` | 1.2.1 | 1.2.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-progress/latest |
| `@radix-ui/react-radio-group` | 1.5.1 | 1.5.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-radio-group/latest |
| `@radix-ui/react-slot` | 1.5.0 | 1.5.0 | CURRENT | https://registry.npmjs.org/@radix-ui/react-slot/latest |
| `@radix-ui/react-switch` | 1.4.1 | 1.4.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-switch/latest |
| `@radix-ui/react-tooltip` | 1.4.1 | 1.4.1 | CURRENT | https://registry.npmjs.org/@radix-ui/react-tooltip/latest |
| `@tanstack/react-query` | 5.104.5 | 5.108.4 | MINOR-BEHIND | https://registry.npmjs.org/@tanstack/react-query/latest |
| `@tanstack/react-virtual` | 3.14.22 | 3.14.28 | PATCH-BEHIND | https://registry.npmjs.org/@tanstack/react-virtual/latest |
| `@tauri-apps/api` | 2.12.4 | 2.12.5 | PATCH-BEHIND | https://registry.npmjs.org/@tauri-apps/api/latest |
| `class-variance-authority` | 0.7.1 | 0.7.1 | CURRENT | https://registry.npmjs.org/class-variance-authority/latest |
| `clsx` | 2.1.1 | 2.1.1 | CURRENT | https://registry.npmjs.org/clsx/latest |
| `i18next` | 26.5.6 | 27.2.9 | MAJOR-BEHIND | https://registry.npmjs.org/i18next/latest |
| `lucide-react` | 1.61.0 | 1.81.0 | MINOR-BEHIND | https://registry.npmjs.org/lucide-react/latest |
| `react` | 19.3.1 | 19.3.1 | CURRENT | https://registry.npmjs.org/react/latest |
| `react-dom` | 19.3.1 | 19.3.1 | CURRENT | https://registry.npmjs.org/react-dom/latest |
| `react-i18next` | 17.1.4 | 18.0.3 | MAJOR-BEHIND | https://registry.npmjs.org/react-i18next/latest |
| `react-router` | 7.18.4 | 8.5.3 | MAJOR-BEHIND | https://registry.npmjs.org/react-router/latest |
| `sonner` | 2.0.8 | 2.0.8 | CURRENT | https://registry.npmjs.org/sonner/latest |
| `tailwind-merge` | 3.7.0 | 3.7.1 | PATCH-BEHIND | https://registry.npmjs.org/tailwind-merge/latest |
| `zustand` | 5.0.16 | 5.1.0 | MINOR-BEHIND | https://registry.npmjs.org/zustand/latest |

Notes: `@tanstack/react-query` 5.104.5 → 5.108.4, `lucide-react` 1.61.0 → 1.81.0 and `zustand` 5.0.16 → 5.1.0 are minor-bumps only. The three majors-behind are the i18n/router trio (`i18next` 26→27, `react-i18next` 17→18, `react-router` 7→8).

## (b) UI dev dependencies (ui/package.json — devDependencies)

30 entries, exact pins. All latest values are `dist-tags.latest` (stable).

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| `@ast-grep/cli` | 0.51.0 | 0.51.3 | PATCH-BEHIND | https://registry.npmjs.org/@ast-grep/cli/latest |
| `@eslint/js` | 10.0.1 | 10.0.1 | CURRENT | https://registry.npmjs.org/@eslint/js/latest |
| `@playwright/test` | 1.64.1 | 1.65.1 | MINOR-BEHIND | https://registry.npmjs.org/@playwright/test/latest |
| `@rolldown/plugin-babel` | 0.2.4 | 0.3.1 | MINOR-BEHIND | https://registry.npmjs.org/@rolldown/plugin-babel/latest |
| `@tailwindcss/vite` | 4.3.3 | 4.3.5 | PATCH-BEHIND | https://registry.npmjs.org/@tailwindcss/vite/latest |
| `@tauri-apps/cli` | 2.12.4 | 2.12.6 | PATCH-BEHIND | https://registry.npmjs.org/@tauri-apps/cli/latest |
| `@testing-library/jest-dom` | 7.0.1 | 8.0.0 | MAJOR-BEHIND | https://registry.npmjs.org/@testing-library/jest-dom/latest |
| `@testing-library/react` | 16.3.3 | 16.3.3 | CURRENT | https://registry.npmjs.org/@testing-library/react/latest |
| `@types/node` | 24.19.2 | 26.6.11 | MAJOR-BEHIND | https://registry.npmjs.org/@types/node/latest |
| `@types/react` | 19.3.0 | 19.3.3 | PATCH-BEHIND | https://registry.npmjs.org/@types/react/latest |
| `@types/react-dom` | 19.3.0 | 19.3.0 | CURRENT | https://registry.npmjs.org/@types/react-dom/latest |
| `@vitejs/plugin-react` | 6.1.2 | 6.3.0 | MINOR-BEHIND | https://registry.npmjs.org/@vitejs/plugin-react/latest |
| `@vitest/coverage-v8` | 4.1.12 | 5.1.2 | MAJOR-BEHIND | https://registry.npmjs.org/@vitest/coverage-v8/latest |
| `babel-plugin-react-compiler` | 1.0.0 | 1.0.0 | CURRENT | https://registry.npmjs.org/babel-plugin-react-compiler/latest |
| `esbuild` | 0.28.2 | 0.28.2 | CURRENT | https://registry.npmjs.org/esbuild/latest |
| `eslint` | 10.13.0 | 10.15.0 | MINOR-BEHIND | https://registry.npmjs.org/eslint/latest |
| `eslint-plugin-import-x` | 4.17.1 | 4.18.1 | MINOR-BEHIND | https://registry.npmjs.org/eslint-plugin-import-x/latest |
| `eslint-plugin-react-hooks` | 7.1.1 | 7.1.1 | CURRENT | https://registry.npmjs.org/eslint-plugin-react-hooks/latest |
| `globals` | 17.13.0 | 17.14.0 | MINOR-BEHIND | https://registry.npmjs.org/globals/latest |
| `jscpd` | 5.4.2 | 5.4.3 | PATCH-BEHIND | https://registry.npmjs.org/jscpd/latest |
| `jsdom` | 30.1.3 | 31.0.1 | MAJOR-BEHIND | https://registry.npmjs.org/jsdom/latest |
| `knip` | 6.42.1 | 6.48.0 | MINOR-BEHIND | https://registry.npmjs.org/knip/latest |
| `oxc-transform-react` | 0.155.0 | 0.158.0 | MINOR-BEHIND | https://registry.npmjs.org/oxc-transform-react/latest |
| `prettier` | 3.9.10 | 3.10.1 | MINOR-BEHIND | https://registry.npmjs.org/prettier/latest |
| `tailwindcss` | 4.3.3 | 4.3.5 | PATCH-BEHIND | https://registry.npmjs.org/tailwindcss/latest |
| `tw-animate-css` | 1.4.0 | 1.4.0 | CURRENT | https://registry.npmjs.org/tw-animate-css/latest |
| `typescript` | 5.9.3 | 7.0.2 | MAJOR-BEHIND | https://registry.npmjs.org/typescript/latest |
| `typescript-eslint` | 8.72.0 | 8.74.0 | MINOR-BEHIND | https://registry.npmjs.org/typescript-eslint/latest |
| `vite` | 8.3.5 | 8.4.0 | MINOR-BEHIND | https://registry.npmjs.org/vite/latest |
| `vitest` | 4.1.12 | 5.1.2 | MAJOR-BEHIND | https://registry.npmjs.org/vitest/latest |

Notes: `@rolldown/plugin-babel` 0.2.4 → 0.3.1 and `oxc-transform-react` 0.155.0 → 0.158.0 are 0.x packages where a minor bump is breaking by convention. The majors: `@vitest/coverage-v8`+`vitest` 4 → 5, `jsdom` 30 → 31, `@testing-library/jest-dom` 7 → 8, `@types/node` 24 → 26 (matching Node LTS line), and `typescript` 5.9.3 → 7.0.2 (the TypeScript 7 native-compiler line is now the registry's stable `latest`).

## (c) Rust / Tauri dependencies (ui/src-tauri/Cargo.toml, locked in Cargo.lock)

The Tauri crate itself is **2.12.5 locked** (req `"2"`); the lockfile is the authoritative version — Cargo.toml uses floating caret requirements.

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| tauri (crate) | 2 (locked 2.12.5) | 2.12.7 | PATCH-BEHIND | https://crates.io/api/v1/crates/tauri |
| tauri-build (build-dep) | 2 (locked 2.7.4) | 2.7.6 | PATCH-BEHIND | https://crates.io/api/v1/crates/tauri-build |
| tauri-plugin-dialog | 2 (locked 2.8.3) | 2.8.4 | PATCH-BEHIND | https://crates.io/api/v1/crates/tauri-plugin-dialog |
| tauri-plugin-opener | 2 (locked 2.7.2) | 2.7.3 | PATCH-BEHIND | https://crates.io/api/v1/crates/tauri-plugin-opener |
| serde | 1 (locked 1.0.229) | 1.0.229 | CURRENT | https://crates.io/api/v1/crates/serde |
| serde_json | 1 (locked 1.0.154) | 1.0.154 | CURRENT | https://crates.io/api/v1/crates/serde_json |
| windows-sys (cfg(windows)) | 0.59 (locked 0.59.0) | 0.61.2 | MINOR-BEHIND | https://crates.io/api/v1/crates/windows-sys |

Notes: `windows-sys` req is `0.59` with lockfile 0.59.0; latest is 0.61.2 — for a 0.x crate a two-step minor bump is breaking. All four Tauri-family crates are patch-behind only (tauri 2.12.5→2.12.7, tauri-build 2.7.4→2.7.6, dialog 2.8.3→2.8.4, opener 2.7.2→2.7.3); `serde`/`serde_json` are exactly current. No rust-toolchain.toml exists anywhere in the repo — the Rust version comes from CI (see (e)).

## (d) GitHub Actions + runner images (.github/workflows/*.yml — 10 workflow files)

Every `uses:` ref in the repo is an immutable SHA pin (no floating tags). `engine/.github/` contains no workflows of its own (only scripts/scenario data consumed by the root workflows).

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| `actions/checkout` | v-tag v4.4.0 (@11d5960a3267) | v7.0.1 | MAJOR-BEHIND | https://api.github.com/repos/actions/checkout/releases/latest |
| `actions/cache` | v-tag v4.3.0 (@0057852bfaa8) | v6.1.0 | MAJOR-BEHIND | https://api.github.com/repos/actions/cache/releases/latest |
| `actions/upload-artifact` | v-tag v4.6.2 (@ea165f8d65b6) | v7.0.2 | MAJOR-BEHIND | https://api.github.com/repos/actions/upload-artifact/releases/latest |
| `actions/download-artifact` | v-tag v4.3.0 (@d3f86a106a0b) | v8.0.2 | MAJOR-BEHIND | https://api.github.com/repos/actions/download-artifact/releases/latest |
| `actions/setup-node` | v-tag v4.4.0 (@49933ea5288c) | v7.1.0 | MAJOR-BEHIND | https://api.github.com/repos/actions/setup-node/releases/latest |
| `pnpm/action-setup` | v-tag v4.3.0 (@b906affcce14) | v6.1.1 | MAJOR-BEHIND | https://api.github.com/repos/pnpm/action-setup/releases/latest |
| `Swatinem/rust-cache` | v-tag v2.9.3 (@d1527c8f67ef) | v2.9.3 | CURRENT | https://api.github.com/repos/Swatinem/rust-cache/releases/latest |
| `ilammy/msvc-dev-cmd` | v-tag v1.13.0 (@0b201ec74fa4) | v1.13.0 | CURRENT | https://api.github.com/repos/ilammy/msvc-dev-cmd/releases/latest |
| `softprops/action-gh-release` | v-tag v2.6.2 (@3bb12739c298) | v3.0.4 | MAJOR-BEHIND | https://api.github.com/repos/softprops/action-gh-release/releases/latest |
| `dtolnay/rust-toolchain` | @b725eda52536 (stable snapshot, 2026-10-16) | master (2 commits newer, housekeeping only) | CURRENT | https://api.github.com/repos/dtolnay/rust-toolchain/commits |
| runner image: `windows-2022` | image 20261116.370.1 — OS 10.0.20348.5870, VS2022 17.14.37906.3, CMake 3.31.6, Rust 1.100.0, Node 22.23.5/24.22.3, vcpkg @0df93a05b9 | windows-latest = Windows Server 2025 image 20261116.315.1 (VS2026 18.12.12405.243, CMake 4.5.0) | MAJOR-BEHIND | https://github.com/actions/runner-images (README + image readmes) |
| runner image: `windows-latest` | = Windows Server 2025 image (VS2026, CMake 4.5.0) | windows-latest label (newest stable Windows image) | CURRENT | https://github.com/actions/runner-images (README + image readmes) |
| runner image: `ubuntu-24.04` | Ubuntu 24.04 LTS (contracts-ci.yml) | ubuntu-latest = Ubuntu 26.04 LTS | MAJOR-BEHIND | https://github.com/actions/runner-images (README + image readmes) |

Notes:
- SHA→tag resolution via the GitHub tags API: checkout `11d5960a…` = **v4.4.0**, cache `0057852b…` = **v4.3.0**, upload-artifact `ea165f8d…` = **v4.6.2**, download-artifact `d3f86a10…` = **v4.3.0**, setup-node `49933ea5…` = **v4.4.0**, pnpm/action-setup `b906affc…` = **v4.3.0**, action-gh-release `3bb12739…` = **v2.6.2**, msvc-dev-cmd `0b201ec7…` = **v1.13.0** (identical to latest release), rust-cache `d1527c8f…` = **v2.9.3** (identical to latest release).
- `dtolnay/rust-toolchain` is a rolling action (its only GitHub release is a 2022 `v1`); the pin is a 2026-10-16 snapshot, 2 commits behind master — both newer commits are test housekeeping, so the action is functionally current, and `toolchain: stable` resolves the current stable Rust (1.100.0) at run time regardless.
- Commit distance to latest release: checkout 36 behind (diverged), cache 98, upload-artifact 55, download-artifact 78, setup-node 64, pnpm/action-setup 40, action-gh-release 61.
- Runners: `windows-2022` in 9 of 13 jobs (benchmark-src, bridge-integration, engine-ci ×2, forensic-repro, named-feature-fixture lane 2, release, validation-evidence ×2); `windows-latest` in 3 jobs (ui-ci, ui-e2e, named-feature-fixture lane 1); `ubuntu-24.04` in contracts-ci only. `windows-latest` = Windows Server 2025 image with VS2026; `ubuntu-latest` = Ubuntu 26.04.
- setup pins used: `actions/setup-node` (node-version 24 in 6 workflows; node 20 only in forensic-repro.yml), `pnpm/action-setup` (version 12.15.0), `dtolnay/rust-toolchain` (toolchain: stable), `ilammy/msvc-dev-cmd` (default latest installed VS). `lukka/run-cmake` and `actions-rust-lang/setup-rust-toolchain` are NOT used in this repo.
- MSVC toolset expectation: CMakePresets.json win presets use generator 'Visual Studio 17 2022' (v143 toolset via VS 17.14 on the windows-2022 image).

## (e) Toolchains (declared in-repo and used by CI)

Declared: `ui/package.json` engines `node >=24` + `packageManager pnpm@12.15.0`; no rust-toolchain.toml; `engine/CMakeLists.txt` `cmake_minimum_required(VERSION 3.25)`; build.sh/build.ps1 expect cmake/node/pnpm/cargo on PATH and drive `cmake --preset win-x64-release` (VS 2022 generator) plus `pnpm tauri build`.

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| Node.js (engines / CI) | >=24 (engines floor); CI node-version: 24 → 24.22.3 (LTS Krypton) | 26.13.1 (LTS Lithium, 2026-11-10) | MAJOR-BEHIND | https://nodejs.org/dist/index.json |
| Node.js (forensic-repro.yml only) | node-version: '20' → 20.20.2 (EOL 2026-04-30) | 26.13.1 | MAJOR-BEHIND | https://nodejs.org/dist/index.json + https://endoflife.date/api/node.json |
| pnpm | 12.15.0 (packageManager + CI) | 12.23.0 | MINOR-BEHIND | https://registry.npmjs.org/pnpm/latest |
| Rust (stable, dtolnay action) | toolchain: stable — resolves at run time; windows-2022 image preinstalls Rust 1.100.0 | 1.100.0 (2026-11-12) | CURRENT | https://api.github.com/repos/rust-lang/rust/releases/latest |
| CMake | floor 3.25 (engine/CMakeLists.txt); runner CMake 3.31.6 | v4.5.1 (2026-11-19) | MAJOR-BEHIND | https://api.github.com/repos/kitware/cmake/releases/latest |
| MSVC / Visual Studio | VS2022 17.14.37906.3 (windows-2022; generator 'Visual Studio 17 2022', v143) | VS2026 18.12.12405.243 (windows-latest image) | MAJOR-BEHIND | https://github.com/actions/runner-images (README + image readmes) |
| vcpkg (engine/vcpkg.json manifest) | no builtin-baseline — floats with runner checkout (commit 0df93a05b9) | vcpkg master (rolling) | UNKNOWN — unpinned | https://github.com/microsoft/vcpkg |

Notes: Node 24 is still an active LTS (Krypton, EOL 2028-04-30) but is two even-major lines behind 26.13.1 (Lithium). Node 20 in forensic-repro.yml is past EOL (2026-04-30). pnpm 12.15.0 → 12.23.0 is minor-only. CMake: the repo floor is 3.25 and the windows-2022 image ships 3.31.6, one major behind 4.5.1 (released 2026-11-19). The engine vcpkg manifest pins no `builtin-baseline`, so zlib/lz4/zstd/gtest versions float with whichever vcpkg checkout the runner image carries (windows-2022 image: vcpkg built from commit 0df93a05b9).

## (f) Vendored / FetchContent / manifest-external sources (engine/ + bench)

engine/third_party/stb is vendored in-tree; googletest is a FetchContent fallback tarball (only used when find_package(GTest) fails — the vcpkg manifest is the primary path); zlib/lz4/zstd/gtest come from the vcpkg manifest. engine/bench/og/FlaUiProbe is an out-of-tree, reference-only UIA3 probe (not built by any workflow or CMake target — referenced only in docs/bench README).

| component | pinned/locked version | latest stable | status | source URL checked |
| --- | --- | --- | --- | --- |
| stb_image.h (engine/third_party/stb, vendored) | 2.30 (2024-05-31) | 2.34 (master, rolling — no tags; sha256 differs) | MINOR-BEHIND | https://github.com/nothings/stb |
| googletest (FetchContent fallback, engine/CMakeLists.txt) | v1.15.2 tarball | v1.18.0 (2026-08-10) | MINOR-BEHIND | https://api.github.com/repos/google/googletest/releases/latest |
| zlib (vcpkg manifest) | unpinned — resolved by vcpkg baseline | port 1.4 (vcpkg master) | UNKNOWN — unpinned | https://github.com/microsoft/vcpkg/tree/master/ports/zlib |
| lz4 (vcpkg manifest) | unpinned — resolved by vcpkg baseline | port 1.11.1 (vcpkg master) | UNKNOWN — unpinned | https://github.com/microsoft/vcpkg/tree/master/ports/lz4 |
| zstd (vcpkg manifest) | unpinned — resolved by vcpkg baseline | port 1.6.0 (vcpkg master) | UNKNOWN — unpinned | https://github.com/microsoft/vcpkg/tree/master/ports/zstd |
| gtest (vcpkg manifest) | unpinned — resolved by vcpkg baseline | port 1.18.0 (vcpkg master) | UNKNOWN — unpinned | https://github.com/microsoft/vcpkg/tree/master/ports/gtest |
| FlaUI.Core / FlaUI.UIA3 (engine/bench/og/FlaUiProbe.csproj — reference-only, not built by CI) | 4.0.0 | 5.0.0 | MAJOR-BEHIND | https://api.nuget.org/v3-flatcontainer/flaui.core/index.json |
| .NET target (FlaUiProbe — reference-only) | net8.0-windows | .NET 11.0.0 (latest; .NET 8 EOL 2026-11-10) | MAJOR-BEHIND | https://endoflife.date/api/dotnet.json |

Notes: vendored `stb_image.h` is v2.30 (2024-05-31); upstream master is v2.34 and its sha256 differs from the vendored copy — stb has no release tags, master is the rolling reference. googletest FetchContent pins the v1.15.2 tarball while the vcpkg master port and the latest GitHub release are 1.18.0. No other ExternalProject/CPM/vendor directories exist under engine/ (checked: only third_party/stb).

## Source index (checked 2026-11-26)

- npm: `https://registry.npmjs.org/<pkg>/latest` (dist-tags.latest; prerelease tags ignored)
- crates.io: `https://crates.io/api/v1/crates/<crate>` (max_stable_version; UA dep-audit/1.0)
- GitHub Releases API: `https://api.github.com/repos/<owner>/<repo>/releases/latest`, `…/tags`, `…/compare/<pin>...<tag>` for actions, rust-lang/rust, kitware/cmake, google/googletest
- Node: `https://nodejs.org/dist/index.json`; EOL windows: `https://endoflife.date/api/node.json`
- .NET: `https://endoflife.date/api/dotnet.json`; NuGet: `https://api.nuget.org/v3-flatcontainer/<pkg>/index.json`
- Runner images: `https://github.com/actions/runner-images` README + `images/windows/Windows2022-Readme.md` + `images/windows/Windows2025-VS2026-Readme.md`
- vcpkg ports: `https://raw.githubusercontent.com/microsoft/vcpkg/master/ports/<port>/vcpkg.json`
- stb: `https://raw.githubusercontent.com/nothings/stb/master/stb_image.h` (sha256-compared against `engine/third_party/stb/stb_image.h`)

No credentials, tokens or auth material are stored in this file; the GitHub API token used for rate-limited reads existed only in curl command lines and is not reproduced here.
