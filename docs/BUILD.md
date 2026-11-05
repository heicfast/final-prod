# BUILD — the single build command and every sub-build

`build.ps1` (Windows, the full product) and `build.sh` (Linux, the
developer subset: contracts validation + engine Linux presets) are the
only build entry points. A new engineer runs one command and gets a full
product build.

```powershell
.\build.ps1                 # all lanes: contracts → engine → ui+tauri → installer → portable → SHA256SUMS
.\build.ps1 -Target engine  # one lane: contracts | engine | ui | installer | portable | all
.\build.ps1 -Configuration Debug
.\build.ps1 -SkipTests      # engine ctest + ui vitest skipped
```

There is no driver lane and no WDK prerequisite: the product is user-mode
only (the kernel driver was removed in the consolidation session, commit
318b9655).

## Version policy

The root `VERSION` file is the product version source of truth.
`build.ps1`'s first gate (`Assert-VersionConsistency`) FAILS if any lockstep
consumer has drifted: `ui/package.json`, `ui/src-tauri/Cargo.toml`,
`ui/src-tauri/tauri.conf.json`. The release workflow additionally refuses a
tag that does not equal `v<VERSION>`. Component wire-visible versions
(the engine ABI number 4) are component contracts, documented as such —
they are NOT restamped.

## Lanes and sequencing

```
VERSION gate (all consumers in lockstep)
├── contracts: node contracts/validate.mjs          (boundary truth: phase 1 + 2)
├── engine:   cmake --preset win-x64-<cfg> (vcpkg manifest: zlib/gtest/lz4/zstd)
│             → ctest → stamp_osversion_um.py + verify_pe_um.py (Win7 gates)
│             → engine kit staged: cr_ui.dll + cr_ui.lib + rrecov.exe + the
│               vcpkg runtime DLLs the engine's own POST_BUILD step deployed
│             → devsign.ps1 embed-signs the staged cr_ui.dll + rrecov.exe
│               (user-mode self-signing; docs/DEV-SIGNING.md)
└── ui:       pnpm install → typecheck/lint/test → stage the engine kit into
              src-tauri/target/{release,debug} + CR_UI_LIB_DIR → pnpm tauri build
              (beforeBuildCommand runs pnpm build; cargo links cr_ui.lib —
              STATIC import: the DLLs must ride BESIDE the exe at runtime,
              which the staging + installer layout both honor)
└── installer: makensis recoverywizard.nsi (consumes the Tauri NSIS bundle +
              the staged engine kit + VERSION) → devsign.ps1 signs the setup.exe
└── portable:  portable.ps1 (app + kit BESIDE the exe)
└── checksums: SHA256SUMS over every artifact in build\out
```

## Toolchain prerequisites (Windows lanes)

- Visual Studio 2022 with MSVC v143 + Windows SDK (runner image default)
- CMake ≥ 3.21 + vcpkg (manifest mode; GitHub runners ship VCPKG_ROOT)
- Node 24 + pnpm 12.15 (`corepack enable`)
- Rust stable (`rustup`; `cargo check`/`clippy` need no engine kit —
  no link step — but `pnpm tauri build` needs the staged kit)
- NSIS (makensis): ships with the Tauri bundler's toolchain
  (`%LOCALAPPDATA%\tauri\NSIS\makensis.exe`) or system NSIS

## The engine kit (link + load truth)

The bridge statically imports `cr_ui.dll`:

- **Link time** needs `cr_ui.lib` on the search path — `build.rs` adds
  `$CR_UI_LIB_DIR` (set by build.ps1's ui lane) plus
  `src-tauri/target/{release,debug}` (where the lane stages the kit).
  `cargo check` / `cargo clippy` do not link and need none of this.
- **Load time** resolves static imports from the EXE's directory — the kit
  DLLs ride BESIDE `RecoveryWizard.exe` at `$INSTDIR` (installer), beside
  the built exe (dev), or beside the portable exe. An `engine\` subfolder
  is off the loader's search path and fails with 0xc0000135 at app start.

## CI mapping

`contracts-ci.yml` (validate.mjs + the filefilter freshness gate),
`ui-ci.yml` (every TS gate + cargo check/clippy — no kit needed),
`engine-ci.yml` (core's checks.yml posture: clang-format 18.1.8 +
architecture lints + MSVC /W4 /WX + ctest + the Win7 posture gates +
G-09 report-schema validation + the elevation refusal probes — the
`ERR_ELEVATION_REQUIRED` refusal is pinned for the non-elevated token —
+ M-071; Linux ASan/TSan/UBSan/fuzz included),
`ui-e2e.yml` (Playwright against the test-mode build),
`forensic-repro.yml` (dispatchable probe of the built product),
`release.yml` (tag → full build → artifacts + SHA256SUMS + truthful notes),
`validation-evidence.yml` (T14/T15, workflow_dispatch).
The driver-ci lane is deleted (no driver since commit 318b9655).
Every lane is paths-filtered so a component break cannot red another lane.
