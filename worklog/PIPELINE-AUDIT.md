# PIPELINE-AUDIT — build/test/benchmark/package duplication across the 9 workflows

Session: ci-optimization · Subagent C · 2026-11-18 · tree at 9352c81d.
Method: every step below is read from `.github/workflows/*.yml`, `build.ps1`,
`build.sh`, `engine/CMakeLists.txt` + `CMakePresets.json` + `vcpkg.json`,
`ui/package.json` + `ui/src-tauri/tauri.conf.json`, `installer/`, `tools/`.
Durations are GitHub API measurements of the cited run ids (recent green runs;
the API call uses a token that lives only in the process env — never in this
sheet). Inference is tagged `[Inference]`.

---

## 1. Inventory — what each workflow does (per build/test/bench/package step)

### 1.1 engine-ci.yml (push+PR `main`, paths `engine/**`; workflow_call; dispatch)
- **lint** (win-2022, ~0.6 min): pip clang-format 18.1.8 dry-run over
  `engine/src|include|tests` (cache: pip wheel); grep lints N-041/driver-era;
  PS AST parse gate over `engine/.github/scripts` + `bench/*.ps1`; PyYAML parse
  gate over all 9 root workflows + inline pwsh run-blocks; doc-truth gate
  (`engine/tools/doc_truth_gate.py`). Produces: verdict only.
- **build-and-test** (win-2022, needs lint): `cmake --preset win-x64-release`
  (MSVC /W4 /WX, VS2022, vcpkg manifest zlib/gtest/lz4/zstd, binary cache keyed
  `hashFiles('engine/vcpkg.json')`) → `cmake --build --preset win-x64-release`
  → `ctest --preset win-x64-release -C Release` (1775 tests + cr_ui ABI) →
  Win7 posture gate (stamp_osversion_um + verify_pe_um on rrecov.exe /
  cr_tests.exe / cr_ui.dll) → G-09 report-schema → elevation probes → M-071
  junk-filter CLI contract. Produces: `engine/build/win-x64-release/**`
  (NOT uploaded on success; failure diagnostics only, 7-day retention).
- Measured (green runs 44334552871, 44275761807, 44272560834, 44334569972):
  build step 7m54s / 10m24s / 7m34s / 9m38s (mean ≈ 8m53s); ctest 2m23s /
  3m24s / 5m43s / 4m12s (mean ≈ 3m56s); lint ≈ 0.6 min.

### 1.2 bridge-integration.yml (push+PR `main`, paths `engine/**`, `ui/src-tauri/**`, `contracts/**`)
- **bridge** (win-2022, single job, 75 min timeout):
  rust toolchain + cargo cache (`ui/src-tauri/it/target`), pnpm 12.15 + node
  24 setup; **Frontend build**: `pnpm install --frozen-lockfile` + `pnpm build`
  in `ui/` (vite dist for `generate_context!`); [1/5] ABI freshness
  (gen_abi_layouts.py re-run + clang-format + `git diff --exit-code`); vcpkg
  binary cache (SAME key `vcpkg-bin-win2022-<hash(vcpkg.json)>` as engine-ci);
  [2/5] **engine build** `cmake --preset win-x64-release` + build (identical
  preset/toolchain to engine-ci, no ctest); [3/5] run
  `cr_ui_abi_layout_dump.exe` → `abi-layouts.json`; [4/5] diskpart VHDX
  create/attach + NTFS seed (120 live + 80 deleted + ~3 GiB deleted bulk);
  [5/5] `cargo test --manifest-path ui/src-tauri/it/Cargo.toml --features
  layout-gate` (copies cr_ui.dll kit into `it/target/debug`). Produces:
  `abi-layouts.json` + `cr-it-evidence` artifact (always, 30 days).
- Measured (green 44334552899, 44334578460, 44275761901): job 14m20s /
  12m55s / 12m54s; engine-build step 8m58s / 9m46s / 9m48s (mean ≈ 9m31s);
  frontend build 20s / 8s / 8s; cargo test 1m37s / 52s / 55s.

### 1.3 benchmark-src.yml (workflow_dispatch only, smoke|full matrix)
- **benchmark** (win-2022, 300 min timeout): **Engine kit via
  `./build.ps1 -Target engine -SkipTests`** (configure+build win-x64-release
  + PE gates + kit staging + devsign — no ctest); MSVC dev cmd; compile
  `api_harness.c` (cl /O2) linking staged `cr_ui.lib`; optional full ctest
  (input run_tests); runner-context json; scenario matrix via
  `engine/.github/scripts/Run-Benchmark.ps1` + `New-TestDisk.ps1`
  (fixed VHDX disks, `generate_seed.ps1` corpus, MBR+GPT FAT32/NTFS/exFAT)
  staged into `D:\cr-bench\core\bin`; alignbench sweep
  (`engine/bench/ours/run-alignbench.ps1`, `\\.\C:`, 512 MiB span); upload
  evidence artifact (90 days).
- Measured (run 44275941738, red mid-matrix): engine kit 9m19s; matrix
  103m44s (failed at cap); 44339547294: kit 10m56s then ctest failed 2s
  (red tree); 44272577458: kit 9m27s. No fully-green recent run — kit build
  ≈ 9m19s–10m56s every dispatch.

### 1.4 release.yml (tag push `v*`)
- **full-product** (win-2022, 90 min timeout): version gate (tag == VERSION);
  `node contracts/validate.mjs`; node/pnpm/rust setup; **`./build.ps1 -Target
  all`** = contracts → engine (configure+build+ctest 1775 + PE gates + kit +
  devsign) → ui (pnpm install + typecheck + lint + `pnpm test` (30 vitest
  files) + `pnpm tauri build` with `pnpm build` beforeBuild + D1 manifest
  gate) → installer (makensis NSIS + devsign) → portable zip → SHA256SUMS;
  verify artifacts + signatures; release notes; upload to GitHub release +
  build-evidence artifact (14 days).
- Measured (green 44256566315 = 0.9.4, 44246474638, 42188868989): job
  23m18s / 23m35s / 22m53s; the single build step 22m22s / 22m22s / 22m04s.
  Log timeline (44256566315): engine lane 07:09:21→07:25:03 (≈15m42s, incl.
  vcpkg rebuild of gtest/lz4/zlib/zstd 07:09:48–07:10:47 and ctest 367s
  1775/1775) → ui lane 07:25:04→07:31:40 (≈6m36s, incl. vitest 30 files
  07:25:40→07:26:58, cargo release profile 4m35s, NSIS bundle) → installer+
  portable 07:31:40→07:31:42 (2s).

### 1.5 ui-ci.yml (push+PR `main`, paths `ui/**`, `contracts/**`)
- **ci** (win-latest — NOT win-2022): pnpm install → typecheck → lint →
  format:check → arch-lint → ast-grep → filefilter mirror → knip → jscpd →
  `pnpm test:coverage` (vitest, 70/60/70/70) → **`pnpm build`** (vite prod
  dist) → bundle budget → cargo check --tests (no link) → clippy; uploads
  `web-dist` artifact (7 days, NEVER consumed by any workflow — verified: no
  `actions/download-artifact` anywhere in the 9 workflows).
- Measured (green 44339512896, 44260156149): job 4m13s / 5m11s; vitest
  1m22s / 1m29s; cargo check 1m19s / 1m43s; `pnpm build` 1s / 4s (warm vite
  cache); pnpm install 5s / 6s (pnpm store cache).

### 1.6 ui-e2e.yml (push+PR `main`, paths `ui/**`)
- **e2e** (win-latest): pnpm install → Playwright chromium install
  (2m20s / 3m29s) → **`pnpm build`** (VITE_CONNECTOR=mock) → `pnpm e2e`
  (12 specs vs vite preview, 2m11s / 2m16s) → screenshots artifact.
- Measured (green 44339513001, 44260156160): job 5m10s / 6m37s.

### 1.7 contracts-ci.yml (push+PR, paths `contracts/**`, `engine/include/**`, `ui/...`)
- **validate** (ubuntu-24.04): `node contracts/validate.mjs` +
  `node ui/scripts/gen-filefilter.mjs --check`. No build. Measured: whole
  job 0m18s (44339512886).

### 1.8 forensic-repro.yml (workflow_dispatch, installs a RELEASE)
- **repro** (win-2022, 90 min): downloads the tagged release setup.exe +
  SHA256SUMS (curl; no build of its own); npm playwright-core; seed-disks
  (64GB NTFS + 8GB exFAT VHDX); install/uninstall probes + CDP UI drive.
- Measured (green 42255492260): job 36m46s; the S2/S3/S4 step 32m40s.

### 1.9 validation-evidence.yml (workflow_dispatch)
- **t14-measurements** (win-2022): **`./build.ps1 -Target engine -SkipTests`**
  (engine configure+build+kit+devsign, contracts gate inside); elevation
  facts; `cl` compile of `tools/validation/measure-cr-ui.c`; `cargo test
  --release -- --ignored` (T14 bridge numbers).
- **t15-installer-matrix** (win-2022): **`./build.ps1 -Target all
  -SkipTests`** (full product: engine + ui/tauri + installer + portable);
  installer-matrix.ps1 (install/update/repair/uninstall).
- Measured (green 42143983955): t14 job 10m30s (engine kit 6m26s, ignored
  tests 3m30s); t15 job 15m26s (full-product build 14m40s, matrix 11s).
  [Inference] the 6m26s vs 14m40s split implies the ui/tauri+installer
  lanes ≈ 8m of the t15 build.

---

## 2. Duplicated-work register (the core finding)

### 2.1 Clean engine (MSVC win-x64-release, vcpkg manifest) builds per run

| # | Where | Command | ctest? | Kit/devsign? |
|---|---|---|---|---|
| 1 | engine-ci `build-and-test` | `cmake --preset win-x64-release` + build | yes (1775) | no |
| 2 | bridge-integration `[2/5]` | same preset, same toolchain | no | no |
| 3 | benchmark-src "Engine kit" | `build.ps1 -Target engine -SkipTests` (same preset inside) | optional input | yes |
| 4 | release `full-product` (inside `build.ps1 -Target all`) | same preset inside build.ps1 | yes | yes |
| 5 | validation-evidence t14 | `build.ps1 -Target engine -SkipTests` | no | yes |
| 6 | validation-evidence t15 (inside `all`) | same preset inside build.ps1 | no (-SkipTests) | yes |

- **On a single push to `main` touching `engine/**`**: engine-ci AND
  bridge-integration both fire (both list `engine/**`) → **2 separate clean
  engine builds** (≈8m53s + ≈9m31s ≈ **18–19 min of duplicated compile per
  engine push**, plus a second vcpkg bootstrap). benchmark-src/release/
  validation-evidence are dispatch/tag-only, so a push never adds them.
- On a tag push (release): build #4 runs while build #1/#2 may already have
  proven the same tree — release re-builds rather than downloading (§4 Q4).
- Note: vcpkg *binary cache* is shared by the SAME key in engine-ci and
  bridge-integration (`vcpkg-bin-win2022-<hash(vcpkg.json)>`), so dependency
  DLLs are cache-hits after the first job; but the engine C++/C object
  compile (~9 min) has NO cache in any lane — `engine/build/**` is never
  cached or uploaded.
- `build.ps1` engine lane re-runs contracts validation; engine-ci does not.
- release and validation-evidence t15 both run `build.ps1 -Target all` —
  when a release tag and a dispatch happen on the same tree, the full
  product (engine + tauri + NSIS + portable) is built twice more.

### 2.2 UI/tauri builds per run

| # | Where | What builds | Full tauri (cargo link)? |
|---|---|---|---|
| 1 | ui-ci | `pnpm build` (vite dist only) + `cargo check --tests` | no (check only) |
| 2 | ui-e2e | `pnpm build` (vite, mock connector) | no |
| 3 | bridge-integration "Frontend build" | `pnpm install` + `pnpm build` (dist for tauri context macro) | no (it/ crate debug build later) |
| 4 | release (inside `build.ps1 -Target all` → Invoke-Ui) | `pnpm tauri build` (pnpm build + cargo release + NSIS) | yes |
| 5 | validation-evidence t15 (inside `all`) | same | yes |

- On a push to `main` touching `ui/**`: ui-ci + ui-e2e both fire →
  **2 separate `pnpm build` dist builds** (plus a 3rd dist build in
  bridge-integration when the push also touches `engine/**` or
  `ui/src-tauri/**` or `contracts/**` — path sets overlap; e.g. commit
  9352c81d triggered ui-ci + ui-e2e + contracts-ci, and 47cfeae9 triggered
  engine-ci + bridge-integration).
- **Full tauri/cargo release link builds**: only in `build.ps1 -Target all`
  consumers = release.yml (tag) and validation-evidence t15 (dispatch). Not
  duplicated on ordinary pushes.
- pnpm install happens per-job in every UI-touching job (ui-ci, ui-e2e,
  bridge-integration, release's ui lane, t15) — pnpm store cache makes each
  ~5–7s, so the measured waste is small, but the count per ui push = 2 (or 3
  with bridge-integration).

### 2.3 vcpkg bootstraps/dependency installs per push (engine/**)
- engine-ci: 1 (vcpkg manifest install, binary cache)
- bridge-integration: 1 (same manifest, same cache key)
- → **2 vcpkg dependency installs per engine push** (cache-assisted;
  first-of-run pays the build of gtest/lz4/zlib/zstd if the binary cache is
  cold — the release log shows vcpkg rebuilding all four ports on the
  release runner because release.yml has NO vcpkg binary cache step:
  07:09:48→07:10:47 ≈ 1 min).
- release/t14/t15/benchmark use build.ps1, which also has **no**
  `VCPKG_DEFAULT_BINARY_CACHE` wiring → every release rebuilds the 4 vcpkg
  ports from source (~1 min measured in 44256566315).

### 2.4 Fixture generation duplication
- Engine fixture generation: NOT re-run per job. `junk-gate-fat16.raw` etc.
  are committed files (engine/tests/data/images/, with SHA256SUMS); the
  CMake POST_BUILD copies them beside the test binaries in each build tree
  (tests/CMakeLists.txt:81–95). So each engine build copies fixtures, but
  generation itself is not duplicated.
- VHDX disk seeding IS re-created per job in THREE independent lanes with
  different shapes: bridge-integration `[4/5]` (2 VHDX + NTFS seed + ~3 GiB
  bulk, ~20s), benchmark-src Run-Benchmark/New-TestDisk (full matrix disks +
  seed corpus — minutes inside the 103 min matrix), forensic-repro
  seed-disks.ps1 (64GB NTFS + 8GB exFAT, 35s). Different purposes (integration
  vs benchmark vs forensic), so tagged `legitimately different`, but the
  seed-writing code lives in 3 places.
- `gen_abi_layouts.py` regeneration: bridge-integration [1/5] only. Not
  duplicated.

### 2.5 Test-run duplication
- engine ctest (1775 tests): engine-ci (mandatory) + release (via build.ps1
  `all`, not -SkipTests) + benchmark-src (optional input, default true) —
  a tag+dispatch on the same tree can run the full suite up to 3 times.
- ui vitest suite: ui-ci (coverage) + release ui lane (`pnpm test`, 30 files
  ≈ 1m18s) — duplicated only across push lanes vs release.
- contracts validate.mjs: contracts-ci (push) + release version-gate step +
  inside build.ps1 every lane (t14: "contracts gate runs inside") — runs
  2–3 times per release run alone (release.yml step 4 + build.ps1
  Invoke-Contracts). Cheap (sub-second), but pure duplication.

---

## 3. Dependency graphs + critical paths

### engine-ci (push)
```
lint (0.6 min) ──> build-and-test (build 9m + ctest 4m ≈ 13–15 min)
```
Critical path ≈ 0.6 + 14.2 ≈ 15 min (run 44334569972 = 15m13s wall).
Two jobs, strictly serial (`needs: lint`). No parallel legs.

### bridge-integration (push)
Single job; internal sequence: setup(≈2.5 min incl. rust/pnpm/node) →
frontend dist (≈10s) → ABI freshness (≈10s) → engine build (≈9.5 min) →
layout dump (1s) → VHDX attach/seed (≈20s) → cargo it-suite (≈1.5 min).
Critical path = the whole job ≈ 13–14 min; the engine build is 74% of it.

### release (tag)
Single job `full-product`: setup (≈1 min) → build.ps1 all:
contracts(1s) → engine(≈15.7 min incl. vcpkg + ctest 6m07s) → ui
(≈6.6 min incl. vitest + cargo release 4m35s + NSIS) → installer (2s) →
portable/checksums (2s) → verify/upload (≈30s). Critical path ≈ 23 min,
fully serial inside ONE step (`Full product build` = 22m22s of the 23m18s
run). engine lane 67%, ui lane 28% of the build.

### ui-ci / ui-e2e / contracts-ci (push; run in PARALLEL as separate workflows)
- ui-ci critical path: pnpm install → vitest(1m22s) → cargo check
  (1m19s) → clippy ≈ 4–5 min.
- ui-e2e: playwright browsers (2m20s–3m29s) ‖ nothing → e2e suite (2m11s)
  ≈ 5–6.5 min.
- contracts-ci ≈ 0m18s.
- On a `ui/**` push all three start together; ui-e2e and ui-ci are
  independent workflows (no shared job), so they parallelize across
  runners, not within.

### benchmark-src / forensic-repro / validation-evidence (dispatch-only)
- benchmark-src single job: kit(≈9.5 min) → harness(1s) → ctest (optional)
  → matrix (103 min+ capped) → alignbench. Critical path ≈ kit + matrix.
- validation-evidence: t14 and t15 are two jobs with NO `needs` → they run
  in PARALLEL; each independently re-runs build.ps1 (t14 engine lane 6m26s;
  t15 full product 14m40s) — the same tree gets the engine built twice more
  inside ONE workflow run.
- forensic-repro: no build; downloads release; S2/S3/S4 step 32m40s is the
  critical path.

---

## 4. The five questions (exact counts + evidence)

**Q1 — clean engine builds on a single push to main (all triggered
workflows):** Read the steps: engine-ci `build-and-test` (1) and
bridge-integration `[2/5]` (2). Both trigger on `push.branches[main]` with
`engine/**` in paths. **Answer: 2** (≈18–19 min duplicated compile; plus 2
vcpkg installs). If the push also touches `ui/**`, no additional engine
build; if it only touches `docs`, none of these fire (path filters).

**Q2 — separate UI/tauri builds on a single push to main:** ui-ci
(`pnpm build` vite dist + `cargo check`, no link) and ui-e2e (`pnpm build`
vite dist, mock) — plus bridge-integration's frontend dist build when its
paths match. **Answer: 2 dist builds per `ui/**` push (3 when
bridge-integration also triggers); 0 full tauri link builds** (tauri NSIS
builds only in release.yml and validation-evidence t15, which are
tag/dispatch events, never push).

**Q3 — benchmark-src builds vs. what it could download:** It builds the
full engine kit from the checked-out commit via
`./build.ps1 -Target engine -SkipTests` — configure + vcpkg (4 ports
rebuilt; no binary-cache step in build.ps1) + MSVC compile + PE gates + kit
staging + devsign (measured 9m19s–10m56s per recent dispatch run
44275941738 / 44272577458 / 44339547294), then compiles api_harness.c
(1s). The measurement intent is "the checked-out COMMIT" (workflow header),
so source-freshness is deliberate; but a `workflow_dispatch` pinned to a
tagged commit could download the release's already-built engine kit
(`RecoveryWizard-*-portable.zip` / release assets, verified by SHA256SUMS —
the repo's own `engine/docs/PREBUILT-ARTIFACTS.md` documents exactly this
convention and `engine/.github/scripts/fetch-release.ps1` implements it)
instead of re-compiling ≈9.5 min; the only parts that genuinely need a
build are uncommitted source states. [Inference] for tag-pinned dispatches
the kit download would cut ~9–10 min per benchmark run; for
head-of-main dispatches the source build is the point.

**Q4 — release builds vs. reuses:** release.yml builds EVERYTHING from the
tag: contracts → engine (vcpkg 4 ports rebuilt — no binary cache step, no
engine/build cache) + full ctest (1775, 6m07s) + PE gates + kit + devsign →
ui (pnpm install + typecheck + lint + vitest 1m18s + vite dist + cargo
release 4m35s + NSIS) → installer → portable → checksums. It reuses
NOTHING: no `actions/download-artifact` anywhere in the 9 workflows, no
consumption of the `web-dist` artifact ui-ci uploads (dead artifact, 7-day
retention, zero consumers), and the tag's push-triggered engine-ci/ui-ci
runs on the same commit are ignored (engine-ci even declares
`workflow_call: {}` "release.yml calls this lane as its validation gate" —
**a stale comment: release.yml contains no workflow_call invocation**, it
re-implements the whole validation itself inside build.ps1). Measured:
release job 22m53s–23m35s, of which the build step is 22m04s–22m22s;
engine lane ≈15m42s (vcpkg ≈1m, compile ≈5m, ctest 6m07s, gates/stage
rest), ui lane ≈6m36s. It could reuse: the engine-kit from a
build-and-upload job on the same tag (compile ≈9 min + ctest can be a
separate gate), the frontend dist from ui-ci's web-dist, and skip its own
typecheck/lint/vitest (already run by ui-ci on the same commit in the push
lane). [Inference] artifact-passing (engine kit + web-dist) would put
release near ≈10–12 min (cargo link + NSIS + portable + upload).

**Q5 — pure duplicates vs. legitimately different configs:**
- PURE duplicates (same inputs, same outputs, different job):
  1. engine-ci build vs bridge-integration engine build — same preset
     (`win-x64-release`), same toolchain, same vcpkg manifest/cache key,
     same source; only difference: ctest runs in engine-ci only. The
     compile itself (~9 min) is done twice per engine push.
  2. `pnpm install --frozen-lockfile` + `pnpm build` (vite dist) — ui-ci,
     ui-e2e, bridge-integration, release ui-lane: three dist builds of the
     same frontend on overlapping pushes (mock vs default connector differs
     only via `VITE_CONNECTOR` env in ui-e2e — the other two are identical
     invocations).
  3. contracts `validate.mjs`: contracts-ci (push) AND release step AND
     inside every build.ps1 lane (t14/t15/release) — 2–3 runs of the same
     script on the same tree per release run.
  4. vitest suite: ui-ci (coverage mode) vs release `pnpm test` — same
     tests, one with coverage thresholds.
  5. engine ctest 1775: engine-ci vs release (via build.ps1 without
     -SkipTests) vs benchmark-src (optional, default true) — same suite.
- LEGITIMATELY different configs: benchmark-src's release-recipe kit
  (staged/devsigned product shape vs engine-ci's raw build tree);
  validation-evidence t14's `cargo test --release -- --ignored` (T14
  measurement tests, not run anywhere else); forensic-repro (installs the
  shipped release, builds nothing); bridge-integration's `it/` debug-profile
  cargo tests + layout-gate feature (unique); engine-ci's sanitizer/Linux
  presets exist in CMakePresets.json but no workflow uses them (build.sh
  linux-release is the developer/Linux path, not wired to any workflow);
  ui-e2e's mock-connector dist (different env) — borderline, listed above
  as duplicate since the artifact content is the mock-pin only.

---

## 5. Measured cost table (recent runs, GitHub API)

| Workflow / step | run ids (green unless noted) | measured |
|---|---|---|
| engine-ci lint job | 44334569972 / 44334552871 / 44275761807 / 44272560834 | 51s / 54s / 35s / 39s |
| engine-ci build step | same | 9m38s / 7m54s / 10m24s / 7m34s (mean 8m53s) |
| engine-ci ctest step | same | 4m12s / 2m23s / 3m24s / 5m43s (mean 3m56s) |
| bridge-integration job | 44334578460 / 44334552899 / 44275761901 | 12m55s / 14m20s / 12m54s |
| bridge engine-build step | same | 9m46s / 8m58s / 9m48s (mean 9m31s) |
| bridge frontend dist | same | 8s / 20s / 8s |
| release job | 44256566315 / 44246474638 / 42188868989 | 23m18s / 23m35s / 22m53s |
| release build.ps1 step | same | 22m22s / 22m22s / 22m04s |
| benchmark-src engine kit | 44275941738(red) / 44272577458(red) / 44339547294(red) | 9m19s / 9m27s / 10m56s |
| validation t14 job | 42143983955 | 10m30s (kit 6m26s) |
| validation t15 job | 42143983955 | 15m26s (full build 14m40s) |
| ui-ci job | 44339512896 / 44260156149 | 4m13s / 5m11s |
| ui-e2e job | 44339513001 / 44260156160 | 5m10s / 6m37s |
| contracts-ci job | 44339512886 | 0m18s |
| forensic-repro job | 42255492260 | 36m46s (S2/S3/S4 step 32m40s) |

Duplicated-work totals:
- engine push to main: 2 engine builds ≈ 18m24s duplicated compile (8m53s +
  9m31s means) + 2 vcpkg installs + 2 full checkouts.
- ui push to main: 2 pnpm installs + 2 vite dist builds + 2 vitest runs
  (ui-ci 1m22s–1m29s; ui-e2e re-builds dist for mock) ≈ mostly small; the
  big UI duplication is vs release/t15, not per push.
- release run: ≈22 min that re-proves what engine-ci + ui-ci already proved
  on the same commit at push time, plus rebuilds vcpkg ports (≈1 min) that
  engine-ci's binary cache already holds (different runner, no cache step).

## 6. Dead / stale wiring found while auditing

- `web-dist` artifact (ui-ci): uploaded, 7-day retention, zero consumers
  (no download-artifact in the repo).
- engine-ci `workflow_call: {}` + comment "release.yml calls this lane as
  its validation gate": release.yml does NOT call it (grep: no `uses:
  ./.github/workflows/engine-ci.yml` anywhere).
- `engine/.github/scripts/fetch-release.ps1` + PREBUILT-ARTIFACTS.md
  convention: implemented, documented, consumed by NO workflow in this repo
  (benchmark-src and release both build from source instead).
- benchmark-src `cancel-in-progress: false` with group `benchmark-src`:
  measurement runs are never dropped (deliberate, documented).
- concurrency groups are event-scoped (engine-ci/bridge-integration): push
  storms cancel each other but never a nested release validate — consistent
  with the comments; no cross-workflow cancellation between engine-ci and
  bridge-integration (separate groups), so both full lanes always run.
