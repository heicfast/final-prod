# WARNINGS.md — Phase-0 baseline: every build/compile warning, deprecation and lint finding

- Session: release-engineering Phase-0, Agent C (warning catalog)
- Date: 2026-11-26
- Repo: `/home/z/my-project/final-prod`, branch `main`, HEAD `126ea398`
- Toolchains used: gcc/g++ 14.2.0 (Debian), CMake 4.4.5 + Ninja 1.13.2, Rust 1.100.0 (clippy 0.1.100 added via `rustup component add clippy`), pnpm 12.15.0, node 24.22.3, python 3.12.16
- Method: engine configured+built locally (Linux lane) with the repo's own strictness; UI gates run with the repo's own scripts; Rust surface attempted locally and, where blocked, judged from the archived GREEN CI logs under `/tmp/cilogs/`; deprecations grepped in sources; every line below is a real captured line (own run output or CI log), with the run/file it came from.

---

## 1. Configured strictness per surface (with evidence)

### 1.1 Engine C/C++ (`engine/CMakeLists.txt`)
| Setting | Value | Evidence |
| --- | --- | --- |
| `cmake_minimum_required` | 3.25 | engine/CMakeLists.txt:1 (no deprecation warning on CMake 4.4.5) |
| C++ standard | C++20, `STANDARD_REQUIRED ON`, no extensions | engine/CMakeLists.txt:13-15 |
| C standard | C11, `STANDARD_REQUIRED ON` | engine/CMakeLists.txt:16-17 |
| MSVC | `/W4 /WX /permissive- /Zc:preprocessor /utf-8 /external:W0 /wd28251` | engine/CMakeLists.txt:19-21 |
| MSVC deprecation suppression | `_CRT_SECURE_NO_WARNINGS` (plus `_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000`) | engine/CMakeLists.txt:29-30 |
| GCC/Clang | `-Wall -Wextra -Wpedantic -Werror -Wshadow` | engine/CMakeLists.txt:32 |
| GCC extra | `-Wno-missing-field-initializers` | engine/CMakeLists.txt:33-35 |
| Linux preset (repo's own path) | `linux-release`: Unix Makefiles, `CMAKE_BUILD_TYPE=Release`, in-repo `build/linux-release`; `build.sh` runs `cmake --preset linux-release` + ctest | engine/CMakePresets.json:25-31, build.sh:26-33 |

Local build deviation (declared): preset binaryDir is inside the repo, which this session's constraint forbids, so I configured `cmake -S engine -B /tmp/engine-build -G Ninja -DCMAKE_BUILD_TYPE=Release` — same source tree, same global strictness flags (they live in `CMakeLists.txt`, not the preset), Ninja instead of the preset's Unix Makefiles. GTest was not installed system-wide, so FetchContent downloaded v1.15.2 at configure time, as the repo's CMake does (engine/CMakeLists.txt:172-179).

### 1.2 Engine formatting / parse gates (CI-configured)
| Gate | Strictness | Evidence |
| --- | --- | --- |
| clang-format | `clang-format==18.1.8 --dry-run -Werror` over `src/ include/ tests/` | engine-ci 45814007501, job "Lint + parse gates", step "4_[1/2] clang-format check" |
| win-layer parse gate | `g++ -std=c++20 -fshort-wchar -D_WIN32 -D__declspec(x)= -fsyntax-only -Werror=comment` over `src/win/*.cpp`, `src/app/*.cpp`, `src/ui/*.cpp`, `tests/unit/test_*_win.cpp` | engine/tools/win_syntax_check.sh:71,84,110 (registered as ctest `win_syntax_check`, engine/tests/CMakeLists.txt:213-217) |
| CI API harness compile | `cl /nologo /std:c17 /O2 /W4 /DWIN32_LEAN_AND_MEAN` — /W4 but **no /WX** | .github/workflows/benchmark-src.yml:77-79 |

### 1.3 UI (`ui/`)
| Gate | Strictness | Evidence |
| --- | --- | --- |
| TypeScript | `strict: true`, `noUnusedLocals`, `noUnusedParameters`, `noUncheckedSideEffectImports` | ui/tsconfig.json:14-18; `pnpm typecheck` = `tsc --noEmit` (ui/package.json:15) |
| ESLint | `@eslint/js` recommended + `typescript-eslint` `recommendedTypeChecked` (type-checked) + `eslint-plugin-react-hooks` recommended | ui/eslint.config.js:23-25,40-41; `pnpm lint` = `eslint .` |
| Prettier | `prettier --check .` | ui/package.json:18 |
| arch-lint | `node scripts/arch-lint.mjs` (91 files) | ui/package.json:25 |
| knip | include: files, dependencies, unlisted, binaries, unresolved, exports, types, duplicates | ui/knip.json:10-19 |
| Unit tests | `vitest run` | ui/package.json:19 |
| e2e | `playwright test` — CI-owned (browsers), not run locally | ui/package.json:22 |

### 1.4 Rust (`ui/src-tauri`)
| Setting | Value | Evidence |
| --- | --- | --- |
| Toolchain | Rust 1.100.0 stable, edition 2021 | ui/src-tauri/Cargo.toml:5 |
| Lints configured | **None** — no `[lints]` table in Cargo.toml, no `clippy.toml`, no `.cargo/config` anywhere in the repo. Default rustc lint levels apply (`#[warn(unused)]` on by default) | ui/src-tauri/Cargo.toml (whole file), `find` for clippy.toml/.cargo = no hits |
| CI lane | bridge-integration runs `cargo test` (rustc default warnings surface in its log); no archived lane runs `cargo clippy -- -D warnings` | .github/workflows/bridge-integration.yml; /tmp/cilogs/45814007296 step "14_[5/5] cargo test" |

`cargo clippy` was attempted with `-D warnings` per the task, but see §5: it cannot run in this sandbox for system-library reasons, and no archived CI lane runs it either.

---

## 2. Findings

### 2.1 Real findings (warning-level, would show on the owning surface)

| # | Surface | file:line | Exact message | Severity |
| --- | --- | --- | --- | --- |
| F-1 | Rust (rustc, `#[warn(unused_assignments)]`; CI-owned Windows lane) | ui/src-tauri/it/tests/bridge_integration.rs:491:28 | `value assigned to \`frozen_run_s\` is never read` (note: `this value is reassigned later and never used`; overwritten at line 507 `frozen_run_s = f64::from(since_move_ms as u32) / 1000.0;`) | warning |
| F-2 | Rust (rustc, `#[warn(unused_assignments)]`) | ui/src-tauri/it/tests/bridge_integration.rs:498:13 | `value assigned to \`frozen_run_s\` is never read` (help: maybe it is overwritten before being read? — line 498 is `frozen_run_s = 0.0;`) | warning |
| F-3 | Rust (rustc, `#[warn(dead_code)]`) | ui/src-tauri/it/tests/bridge_integration.rs:580:9 | `fields \`node_id\`, \`name\`, and \`category\` are never read` (struct `OwnedRow` at line 579; fields at 580, 581, 585) | warning |
| F-4 | Rust build script (cargo:warning, informational by design) | ui/src-tauri/build.rs (emitted for target `recovery-wizard-ui@0.9.6`) | `linking cr_ui.lib (static import of cr_ui.dll, ABI v4); build.ps1 -Target engine stages the kit when CR_UI_LIB_DIR is unset` | info (build-script notice) |
| F-5 | UI vitest (React `act(...)` stderr warnings; tests pass) | ui/tests/consent-surfaces.test.tsx:181 (test `approves stay disabled for a refused plan (default-deny truth)`) | `An update to X inside a test was not wrapped in act(...).` — 10 instances: `Presence` (4), `Dialog` (2), `ForwardRef(FocusScope)` (1), `ForwardRef(DismissableLayer)` (1), `ForwardRef(DialogOverlayImpl)` (1), `ConsentModal` (1) | warning (test hygiene; radix dialog internals) |
| F-6 | Engine C, MSVC /W4 (CI harness file, not the engine targets) | engine/.github/scripts/api_harness.c:39 | `warning C4005: 'WIN32_LEAN_AND_MEAN': macro redefinition` — the workflow passes `/DWIN32_LEAN_AND_MEAN` (.github/workflows/benchmark-src.yml:77) AND the source re-`#define`s it at line 39; no `/WX` on that cl invocation, so the lane stays green. Captured in runs 44344598911, 44346817200, 44359663812 (2026-11-18); both the workflow flag and the source define are unchanged at HEAD, so the warning still fires on that lane | warning |
| F-7 | UI production bundle (vite/rolldown build output; surfaced on the e2e CI lane) | ui (chunk `dist/assets/index-DZWtaJbv.js`, 561.21 kB / gzip 160.02 kB) | `(!) Some chunks are larger than 500 kB after minification. Consider: - Using dynamic import() to code-split the application - Use build.rolldownOptions.output.codeSplitting to improve chunking... - Adjust chunk size limit for this warning via build.chunkSizeWarningLimit.` | warning (bundle size) |

`bridge-it` summary line confirming F-1..F-3: ``warning: `bridge-it` (test "bridge_integration") generated 3 warnings``.

### 2.2 CI-platform deprecations (real deprecation notices, but in GitHub Actions' own runtime — not repo code)

| # | Surface | Source | Exact message | Severity |
| --- | --- | --- | --- | --- |
| N-1 | CI infra (actions/cache + actions/checkout node runtime) | engine-ci 45814007501 steps "4_vcpkg binary cache" (line 11), "21_Post vcpkg binary cache" (line 2), "Lint + parse gates" (line 104) and others; also bridge-integration 45814007296 | `(node:8088) [DEP0040] DeprecationWarning: The \`punycode\` module is deprecated. Please use a userland alternative instead.` | noise (GH Actions internals) |
| N-2 | CI infra (same) | engine-ci 45814007501 "21_Post vcpkg binary cache" (line 5), "Lint + parse gates" (line 275) | `(node:3768) [DEP0169] DeprecationWarning: \`url.parse()\` behavior is not standardized and prone to errors that have security implications. Use the WHATWG URL API instead. CVEs are not issued for \`url.parse()\` vulnerabilities.` | noise (GH Actions internals) |
| N-3 | CI infra (GitHub platform notice about the pinned action SHAs) | engine-ci 45814007501 "23_Complete job" (line 3) and "Lint + parse gates/17_Complete job" (line 2) | `##[warning]Node.js 20 is deprecated. The following actions target Node.js 20 but are being forced to run on Node.js 24: actions/cache@0057852bfaa89a56745cba8c7296529d2fc39830, actions/checkout@11d5960a326750d5838078e36cf38b85af677262. For more information see: https://gi...` | noise (platform-level; actionable only by bumping action SHAs someday) |
| N-4 | git line-ending notice | bridge-integration 45814007296 step "9_[1/5] Generated ABI files freshness gate" (line 29) | `warning: in the working copy of 'ui/src-tauri/it/tests/abi_layouts.rs', CRLF will be replaced by LF the next time Git touches it` | noise (checkout-time, not a compiler) |

### 2.3 Environment-limited results in this sandbox (recorded for honesty, NOT repo defects)

| # | Item | What happened | Why it is environmental |
| --- | --- | --- | --- |
| E-1 | `FatChain.CycleGuardPerf` (engine/tests/unit/test_fat_volume.cpp:3267) | ctest 477/1822: `Subprocess killed`, exit 137 standalone; kernel log: `Out of memory: Killed process 13124 (cr_tests) ... anon-rss:3194512kB` | The fixture builds a `std::vector<u8> img(total_secs * 512, 0)` (~3.4 GB, test comment at test_fat_volume.cpp:3277-3279 says "lazy-mapped ~3.4 GB"). The sandbox cgroup caps memory at 4 GiB (`/sys/fs/cgroup/memory.max` = 4294967296) and value-initialization touches every page, so the test needs a CI-class runner. All other 1821 tests pass (see §3). |
| E-2 | knip raw OOM | First run: `RangeError: Array buffer allocation failed` at oxc-parser 0.154.0 `src-js/raw-transfer/common.js:294` (fixed ~6 GiB raw-transfer buffer) | Node >= 22 auto-enables raw transfer (oxc-parser `raw-transfer/supported.js`); the 4 GiB cgroup cannot allocate it. knip ships its own escape hatch `KNIP_DISABLE_RAW_TRANSFER=1` (knip 6.42.1 `dist/typescript/ast-nodes.js:8`); with it set, knip runs and is CLEAN — same parser, only the AST transfer mode differs. |
| E-3 | `win_syntax_check` initial FAIL | ctest 1821 failed with `no gtest include dir found (build one preset first - FetchContent drops it at build/*/_deps/googletest-src/googletest/include - or export GTEST_INCLUDE)` | The gate searches `engine/build/*/_deps/...` (in-repo build tree, win_syntax_check.sh:118-120); my build dir is /tmp/engine-build per the session constraint. Re-run with `GTEST_INCLUDE=/tmp/engine-build/_deps/googletest-src/googletest/include`: **Passed (16.52 s)** — all src/win, src/app, src/ui and test_*_win.cpp files parse. Harness-path artifact, not a code defect. |

### 2.4 Deprecation sweep (sources + config)

| Surface | Search | Result |
| --- | --- | --- |
| Engine C++ | `[[deprecated]]`, `__declspec(deprecated`, `DEPRECATED` in engine/src, engine/include, engine/tests | **0 hits** |
| Engine CRT-era deprecations | MSVC `strcpy`/`fopen`-class warnings are globally suppressed by `_CRT_SECURE_NO_WARNINGS` (engine/CMakeLists.txt:29) — nothing surfaces at /W4 /WX; on Linux nothing triggers under the configured flags | 0 findings (suppressed-by-config, recorded as config fact) |
| CMake | own deprecation warnings in my configure output (CMake 4.4.5 vs `cmake_minimum_required(VERSION 3.25)`; no `cmake_policy`/compatibility syntax anywhere) | **0 warnings** in /tmp/engine-configure.log |
| UI TS | `@deprecated` JSDoc in ui/src, ui/tests | **0 hits**; no eslint deprecation rule/plugin configured (eslint.config.js) |
| Rust | `#![deprecated]`/`deprecated` in ui/src-tauri/src, ui/src-tauri/it, build.rs | **0 hits** |
| Lockfile | `pnpm install --frozen-lockfile` | **clean — no mismatch** (install done in 8s, exit 0; pnpm-lock.yaml untouched, `git status` clean for it) |

---

## 3. Per-surface clean statements

1. **Engine C/C++ compile, Linux lane (the primary strictness surface): CLEAN.** GCC 14.2.0, CMake 4.4.5, Release, `-Wall -Wextra -Wpedantic -Werror -Wshadow` (+GNU `-Wno-missing-field-initializers`), C++20/C11: **0 warning lines** in `/tmp/engine-build.log` (176/176 ninja targets, exit 0) and **0 warning/deprecation lines** in `/tmp/engine-configure.log`. With `-Werror`, any warning would have failed the build; it did not.
2. **Engine MSVC /W4 /WX (judged from CI logs): CLEAN.** engine-ci 45814007501 ("Build + test (MSVC Release, vcpkg cache)", 2026-11-26) and named-feature-fixture 45811533301 ("Build rrecov (win-x64-release)", 2026-11-26): grep for `warning C[0-9]` over the build steps = **0 hits**; both runs carry 0 `##[error]` markers and completed green.
3. **Engine clang-format gate: CLEAN** — `clang-format clean (294 files)` (engine-ci 45814007501, "Lint + parse gates", step 4).
4. **Engine win-layer parse gate: CLEAN** — win_syntax_check Passed with GTEST_INCLUDE (see E-3); all 14 src/win + 2 src/app + 1 src/ui files and every `tests/unit/test_*_win.cpp` parse under `g++ -fsyntax-only -Werror=comment` with the win-stub SDK.
5. **Engine test suite: 1822 tests, 1820 green in-sandbox**; of the 2 non-greens, one is OOM (E-1) and one is the harness-path artifact that passes when the gate's own GTEST_INCLUDE override is used (E-3). Zero test-code warnings captured (compile of all test targets was warning-free under the global flags).
6. **UI typecheck: CLEAN** — `tsc --noEmit` exit 0, no output.
7. **UI lint: CLEAN** — `eslint .` exit 0, no output.
8. **UI format:check: CLEAN** — `All matched files use Prettier code style!`
9. **UI arch:lint: CLEAN** — `arch-lint: OK (91 files checked)`.
10. **UI knip: CLEAN** — exit 0, no findings (with `KNIP_DISABLE_RAW_TRANSFER=1`, see E-2).
11. **UI vitest: 225/225 tests pass (30 files)**; findings are the 10 act() stderr warnings (F-5) and nothing else.
12. **Rust engine-bridge crate (`ui/src-tauri`, the app crate itself): no warnings in the only evidence available** — bridge-integration 45814007296 compiles `recovery-wizard-ui v0.9.6` with zero rustc warnings; the 3 rustc warnings (F-1..F-3) are all in the `bridge-it` test crate (`ui/src-tauri/it/tests/bridge_integration.rs`). Clippy-level coverage does not exist anywhere I could check (see §5).

---

## 4. What each surface is configured to enforce but did NOT flag (the honest "nothing found" list)

- GCC `-Wshadow` (engine): no shadowing findings.
- `-Wpedantic` (engine): no pedantic findings.
- MSVC `/W4 /WX` incl. `/wd28251` carve-out and `/external:W0`: no C4xxx findings on the engine targets in the green CI logs; the only C4xxx anywhere in the archive is F-6 (C4005, harness file, /W4 without /WX).
- typescript-eslint `recommendedTypeChecked` (type-aware rules): no findings.
- eslint-plugin-react-hooks recommended: no findings.
- knip's full include set (files/dependencies/unlisted/binaries/unresolved/exports/types/duplicates): no findings.
- tsc strict + noUnusedLocals + noUnusedParameters + noUncheckedSideEffectImports: no findings.

---

## 5. Honest limits (what could not be checked here, and what covers it instead)

1. **MSVC is not runnable in this Linux sandbox.** All Windows-only compile surfaces (engine win/app/ui layers, the Rust Tauri shell, the installer) were judged from the archived GREEN CI logs only. Logs checked: `/tmp/cilogs/45814007501` (engine-ci: MSVC /W4 /WX build+test, lint+parse gates — 2026-11-26), `/tmp/cilogs/45814007296` (bridge-integration: engine build + Rust cargo test — 2026-11-26), `/tmp/cilogs/45811533301` (named-feature-fixture: rrecov win-x64-release build + scenario matrix — 2026-11-26), `/tmp/cilogs/44474393995` (ui-e2e — 2026-11-19, carries F-7), plus the older release/benchmark lanes 44344598911 / 44346817200 / 44359663812 (2026-11-18, carriers of F-6). Full sweep: all 33 archived runs grepped for `warning|deprecat|clippy|C[0-9]{4}:` (531 raw hits, classified: 7 code-level findings F-1..F-7, 4 platform noise N-1..N-4, remainder step-name echoes and repeats).
2. **Rust surface is CI-owned (Windows).** `cargo check` on `ui/src-tauri` fails in this sandbox on Tauri 2's Linux system deps. Precisely missing (pkg-config): `gdk-3.0` (first failure, from `gdk-sys v0.18.2` build script), and for the full Linux link set: `atk`, `gdk-3.0`, `gtk+-3.0`, `gtk+-unix-print-3.0`, `webkit2gtk-4.1`, `javascriptcoregtk-4.1`, `soup-3.0`, `dbus-1`, `libappindicator-0.1` (present: glib-2.0, gobject-2.0, gio-2.0, cairo, pango, gdk-pixbuf-2.0, x11). No sudo, so they cannot be installed. `cargo clippy` was installed locally (clippy 0.1.100) but cannot compile for the same reason. **No archived CI lane runs `cargo clippy -- -D warnings` either** — the bridge lane runs `cargo test` only — so clippy-level findings are covered by NO evidence at all; this is a gap in the repo's gate set worth knowing, not just a sandbox gap.
3. **The named ui-ci run 44475947005 is not present under /tmp/cilogs** (the archive holds 33 runs; none is a current ui-ci lane — the `ci` job dirs 44474393997/44475009006/44391744949 contain only system.txt). Coverage instead: I re-ran all ui-ci gates locally at HEAD (typecheck/lint/format:check/arch:lint/knip/test — §3 items 6-11), which is fresher than any archived log. e2e (playwright) was NOT run locally (no browsers) — CI owns it; the only e2e-derived finding is F-7 from the Nov-19 run.
4. **F-6's freshness:** the C4005 warning was captured on Nov-18 runs of the release/benchmark lane; no newer archive of that lane exists. Both the `/DWIN32_LEAN_AND_MEAN` workflow flag and the `#define WIN32_LEAN_AND_MEAN` at engine/.github/scripts/api_harness.c:39 are unchanged at HEAD `126ea398`, so the warning deterministically still fires there; it is recorded from the captured lines, per the no-invented-findings rule.
5. **Engine build dir & generator deviation:** /tmp/engine-build with Ninja instead of the preset's Unix Makefiles + in-repo binaryDir (session constraint: no repo writes). Flags identical (they come from CMakeLists.txt). Nothing in the preset changes warning flags.
6. **GTest 1.15.2** was fetched from the network at configure time (FetchContent), matching engine/CMakeLists.txt:174-178; its compile produced no warnings (it is not under the repo's global flags in a way that failed; build exited 0 with -Werror).

Raw logs kept for replay: `/tmp/engine-configure.log`, `/tmp/engine-build.log`, `/tmp/engine-ctest.log`, `/tmp/ui-typecheck.log`, `/tmp/ui-lint.log`, `/tmp/ui-format.log`, `/tmp/ui-archlint.log`, `/tmp/ui-knip.log`, `/tmp/ui-knip2.log`, `/tmp/ui-vitest.log`, `/tmp/rust-check.log`, `/tmp/cilogs-warning-hits.txt` (grep script: `/home/z/my-project/scripts/ci_warn_grep.sh`).

**Bottom line: the codebase is warning-clean on every locally-runnable strict surface (engine GCC -Werror lane, engine MSVC /W4 /WX per CI logs, UI typecheck/eslint/prettier/arch-lint/knip). The real outstanding findings are 3 rustc unused/dead-code warnings in the Rust integration-test crate (F-1..F-3), 10 React act() warnings in one vitest file (F-5), a MSVC C4005 macro redefinition in the CI API harness (F-6), and a 561 kB chunk-size warning on the production bundle (F-7). No deprecation usage exists in any source language; the only deprecations seen are GitHub-Actions-platform ones (N-1..N-3).**
