# WORKFLOW-INVENTORY — CI/CD optimization session (2026-11-18)

Fact sheet over the 9 workflows in `.github/workflows/`. YAML facts read
from the local clone at commit 9352c81d (+ worklog-only commits on top);
run facts from the GitHub Actions API (`/repos/heicfast/final-prod/actions/...`),
retrieved 2026-11-18 ~16:0x UTC. Duration basis: run level = `updated_at -
run_started_at`; job level = `completed_at - started_at` (both API fields,
shown per row). No logs quoted; failing runs cited by run ID.

Default-branch status as of 2026-11-18 15:46 UTC: **red = engine-ci,
bridge-integration, benchmark-src** (all runs on head 9352c81daf failed);
green = contracts-ci, ui-ci, ui-e2e. Dispatch-only lanes (forensic-repro,
validation-evidence) have no push trigger; their last dispatched runs were
red, so they are also counted red-on-latest-run below. release.yml runs on
tags only (last: v0.9.4, green).

## 1. Summary table

| Workflow | File | Trigger(s) | Concurrency group | cancel-in-progress | Jobs (runner) | Timeout(s) (min) | Last conclusion | Green baseline (mean) |
|---|---|---|---|---|---|---|---|---|
| benchmark-src | benchmark-src.yml | workflow_dispatch only | `benchmark-src` | **false** | benchmark (windows-2022) | 300 | failure | none yet (0 green runs) |
| bridge-integration | bridge-integration.yml | push[main]+paths, pull_request+paths, workflow_dispatch | `bridge-integration-${{ github.event_name }}-${{ github.ref }}` | true | bridge (windows-2022) | 75 | failure | 777 s (n=6) |
| contracts-ci | contracts-ci.yml | push[main]+paths, pull_request+paths | **absent** | — | validate (ubuntu-24.04) | default 360 | success | 12 s (n=10) |
| engine-ci | engine-ci.yml | push[main]+paths, pull_request+paths, workflow_call, workflow_dispatch | `engine-ci-${{ github.event_name }}-${{ github.ref }}` | true | lint (win-2022, 15); build-and-test (win-2022, 90) | 15 / 90 | failure | 867 s (n=5, run level) |
| forensic-repro | forensic-repro.yml | workflow_dispatch only | **absent** | — | repro (windows-2022) | 90 | failure | 2003 s (n=2) |
| release | release.yml | push tags `v*` | **absent** | — | full-product (windows-2022) | 90 | success (v0.9.4) | 1277 s (n=8) |
| ui-ci | ui-ci.yml | push[main]+paths, pull_request+paths | **absent** | — | ci (windows-latest) | default 360 | success | 301 s (n=9) |
| ui-e2e | ui-e2e.yml | push[main]+paths, pull_request+paths | **absent** | — | e2e (windows-latest) | default 360 | success | 367 s (n=10) |
| validation-evidence | validation-evidence.yml | workflow_dispatch only | **absent** | — | t14-measurements (win-2022, 60); t15-installer-matrix (win-2022, 100) | 60 / 100 | failure | 928 s (n=3, run level) |

Green baseline = mean of recent `conclusion: success` runs within the last
10 fetched per workflow (run-level duration). n = number of green runs in
that window.

## 2. Per-workflow detail

### 2.1 benchmark-src (benchmark-src.yml)

- **Triggers (exact YAML)**: `on: workflow_dispatch` with inputs
  `profile` (choice smoke|full, default smoke), `scan_modes` (default
  `'quick,deep,deepcarve'`), `reason` (required), `run_tests` (boolean,
  default true). No push/PR trigger.
- **Permissions**: `contents: read`.
- **Concurrency**: group `benchmark-src`, `cancel-in-progress: false`
  ("measurement runs are never dropped").
- **Jobs**:
  - `benchmark` — name "Build (release recipe) + the scenario matrix + the
    alignbench sweep", `runs-on: windows-2022`, `timeout-minutes: 300`.
    No needs/if/strategy. Steps carry `if: ${{ inputs.run_tests }}` on the
    optional full-ctest step; no continue-on-error anywhere (comment
    explicitly states "no masking").
- **Caches**: none. No actions/cache; no setup-* with cache.
- **Artifacts**: upload `benchmark-src-evidence-${{ inputs.profile }}-${{
  github.run_id }}` (paths `benchmark-results/`, `runner-context.json`;
  retention 90; if-no-files-found error). No downloads.
- **Actions**: checkout (pinned SHA, v4 comment), ilammy/msvc-dev-cmd
  (pinned SHA, v1 comment), upload-artifact (pinned SHA, v4 comment).
- **Run history (last 5, run-level duration basis updated_at - run_started_at)**:
  | run | # | conclusion | started (UTC) | duration |
  |---|---|---|---|---|
  | 44339547294 | 4 | failure | 2026-11-18T15:46:45Z | 689 s |
  | 44334634961 | 3 | failure | 2026-11-18T15:21:10Z | 383 s |
  | 44275941738 | 2 | failure | 2026-11-18T09:26:32Z | 6811 s |
  | 44272577458 | 1 | failure | 2026-11-18T09:04:59Z | 588 s |
  All 4 dispatch runs to date are red; **no green baseline exists**.
- **Latest-run job detail (run 44339547294)**: job "Build (release recipe)
  + the scenario matrix + the alignbench sweep" (id 134925426642),
  conclusion failure, started 15:46:48, completed 15:58:14 → **686 s**.

### 2.2 bridge-integration (bridge-integration.yml)

- **Triggers**: `push: branches: [main]` paths `engine/**`,
  `ui/src-tauri/**`, `contracts/**`, `tools/gen_abi_layouts.py`,
  `.github/workflows/bridge-integration.yml`; `pull_request` same minus
  the workflow file; `workflow_dispatch: {}`.
- **Concurrency**: `bridge-integration-${{ github.event_name }}-${{ github.ref }}`,
  `cancel-in-progress: true`.
- **Jobs**:
  - `bridge` — name "Build engine + link bridge + run integration tests",
    `runs-on: windows-2022`, `timeout-minutes: 75`. No needs/strategy.
    Step-level `if: always()` on the evidence upload (red runs still
    upload). No continue-on-error.
- **Caches**:
  - actions/cache: path `~/.cargo/registry`, `~/.cargo/git`,
    `ui/src-tauri/it/target`; key `bridge-it-rust-win2022-${{
    hashFiles('ui/src-tauri/Cargo.lock') }}-${{ hashFiles('ui/src-tauri/it/Cargo.toml') }}`;
    restore-keys `bridge-it-rust-win2022-`.
  - actions/cache (vcpkg): path `${{ runner.temp }}\vcpkg-binary-cache`,
    key `vcpkg-bin-win2022-${{ hashFiles('engine/vcpkg.json') }}`,
    restore-keys `vcpkg-bin-win2022-`.
  - Built-in: setup-node `cache: pnpm`, `cache-dependency-path: ui/pnpm-lock.yaml`.
- **Artifacts**: upload `bridge-it-evidence-${{ github.run_number }}`
  (paths `${{ runner.temp }}\cr-it-evidence`, `${{ runner.temp }}\abi-layouts.json`;
  retention 30; if-no-files-found warn; `if: always()`). No downloads.
- **Actions**: checkout (SHA, v4), dtolnay/rust-toolchain (SHA, stable
  2026-10-16), actions/cache x2 (SHA, v4), pnpm/action-setup (SHA, v4),
  setup-node (SHA, v4), upload-artifact (SHA, v4).
- **Run history (last 5)**:
  | run | # | event | conclusion | started (UTC) | duration |
  |---|---|---|---|---|---|
  | 44339558061 | 45 | workflow_dispatch | failure | 15:46:48 | 110 s |
  | 44339513235 | 44 | push | failure | 15:46:33 | 107 s |
  | 44334578460 | 43 | workflow_dispatch | success | 15:20:53 | 780 s |
  | 44334552899 | 42 | push | success | 15:20:45 | 864 s |
  | 44275761901 | 41 | push | success | 09:25:23 | 777 s |
  Older: 40 push success 780 s; 39 push success 745 s; 38 push success
  718 s; 37 push cancelled; 36 push failure.
  **Green baseline: 777 s** (780, 864, 777, 780, 745, 718).
- **Latest-run job detail (run 44339558061)**: job "Build engine + link
  bridge + run integration tests" (id 134925460014), failure, started
  15:46:51, completed 15:48:37 → **106 s**.
- **Green job baseline (run 44334552899, #42)**: same single job, success,
  started 15:20:48, completed 15:35:08 → **860 s**.

### 2.3 contracts-ci (contracts-ci.yml)

- **Triggers**: `push: branches: [main]` paths `contracts/**`,
  `engine/include/**`, `ui/src/connector/**`, `ui/src-tauri/src/**`,
  `ui/scripts/gen-filefilter.mjs`, `.github/workflows/contracts-ci.yml`;
  `pull_request` same minus the workflow file.
- **Concurrency**: **absent**. **Timeouts**: none → default 360 min.
- **Jobs**: `validate` — `runs-on: ubuntu-24.04`. No name/needs/if/
  strategy/continue-on-error.
- **Caches**: setup-node with `node-version: 24`, no `cache:` key (comment
  notes the lane is tiny) → no cache at all.
- **Artifacts**: none.
- **Actions**: checkout (SHA, v4), setup-node (SHA, v4).
- **Run history (last 5)**: 69 push success 23 s; 68 success 10 s; 67
  success 14 s; 66 success 18 s; 65 success 9 s. All 10 fetched green.
  **Green baseline: 12 s**.
- **Latest-run job detail (run 44339512886, #69)**: job "validate" (id
  134925282630), success, started 15:46:37, completed 15:46:55 → **18 s**.

### 2.4 engine-ci (engine-ci.yml)

- **Triggers**: `push: branches: [main]` paths `engine/**`,
  `.github/workflows/engine-ci.yml` with negations `!**.md`,
  `!engine/docs/**`, `!engine/worklog.md`, `!engine/worklog/**`;
  `pull_request` same; `workflow_call: {}` (called by release.yml as its
  validation gate — comment); `workflow_dispatch: {}`.
- **Permissions**: `contents: read`.
- **Concurrency**: `engine-ci-${{ github.event_name }}-${{ github.ref }}`,
  `cancel-in-progress: true` (event-scoped so a release's workflow_call
  validate is never cancelled by mid-release pushes — run-40688552776
  lesson in comments).
- **Jobs**:
  - `lint` — name "Lint + parse gates", `runs-on: windows-2022`,
    `timeout-minutes: 15`, `defaults.run.working-directory: engine`. Steps
    include the pip-cache and YAML-parse gates.
  - `build-and-test` — name "Build + test (MSVC Release, vcpkg cache)",
    `needs: lint`, `runs-on: windows-2022`, `timeout-minutes: 90`,
    same working-directory default. Step-level `if: failure()` on the
    diagnostics upload. **No strategy/matrix, no continue-on-error**
    anywhere.
- **Caches**:
  - lint job: actions/cache path `~\AppData\Local\pip\Cache`, key
    `pip-clangformat-win2022`, restore-keys `pip-clangformat-win2022`.
  - build-and-test job: actions/cache path `${{ runner.temp }}\vcpkg-binary-cache`,
    key `vcpkg-bin-win2022-${{ hashFiles('engine/vcpkg.json') }}`,
    restore-keys `vcpkg-bin-win2022-`.
- **Artifacts**: upload (on `if: failure()`) `engine-ci-failure-diagnostics-${{ github.run_id }}`
  (paths `engine/build/win-x64-release/CMakeCache.txt`,
  `engine/build/win-x64-release/Testing/**/*.log`; retention 7;
  if-no-files-found ignore). No downloads.
- **Actions**: checkout (SHA, v4) x2, actions/cache (SHA, v4) x2,
  upload-artifact (SHA, v4).
- **Run history (last 5)**:
  | run | # | event | conclusion | started (UTC) | duration |
  |---|---|---|---|---|---|
  | 44339552858 | 63 | workflow_dispatch | failure | 15:46:46 | 45 s |
  | 44339513142 | 62 | push | failure | 15:46:33 | 43 s |
  | 44334569972 | 61 | workflow_dispatch | success | 15:20:51 | 913 s |
  | 44334552871 | 60 | push | success | 15:20:45 | 697 s |
  | 44275761807 | 59 | push | success | 09:25:23 | 892 s |
  Older: 58 success 865 s; 57 success 872 s; 56 failure; 55 failure;
  54 failure. **Green baseline (run level): 867 s** (913, 697, 892, 865, 972).
- **Latest-run job detail (run 44339552858, #63)**:
  - "Lint + parse gates" (id 134925445094): failure, started 15:46:50,
    completed 15:47:26 → **36 s**.
  - "Build + test (MSVC Release, vcpkg cache)" (id 134925926066):
    skipped (needs-gate), 0 s.
- **Green job baselines**: run 44334552871 (#60) — lint 54 s (15:20:48→15:21:42),
  build-and-test 636 s (15:21:45→15:32:21). Run 44275761807 (#59) —
  lint 35 s, build-and-test 853 s. Critical-path lint+build ≈ 690–888 s.

### 2.5 forensic-repro (forensic-repro.yml)

- **Triggers**: `workflow_dispatch` only, input `release_tag` (default
  `v0.9.3`).
- **Concurrency**: **absent**. **Permissions**: contents: read.
- **Jobs**: `repro` — name "Reproduce all reported symptoms (installed
  release)", `runs-on: windows-2022`, `timeout-minutes: 90`. No
  needs/strategy/continue-on-error. Step-level `if: always()` on the
  log/sessions snapshot and the evidence upload.
- **Caches**: none (setup-node `node-version: '20'`, no cache).
- **Artifacts**: upload `forensic-repro-evidence` (paths
  `C:\forensic\evidence\**`, ProgramData logs, `C:\forensic\twin-data\**`,
  `seed-manifest.json`, `disk-map.json`; retention 30; if-no-files-found
  error; `if: always()`). No downloads.
- **Actions**: checkout (SHA, v4), setup-node (SHA, v4), upload-artifact
  (SHA, v4).
- **Run history (last 5)**: 33 failure 127 s (2026-11-18 06:11); 32
  failure 1918 s (11-06); 31 failure 1793 s; 30 success 2211 s; 29
  success 1796 s. **Green baseline: 2003 s** (2 runs).
- **Latest-run job detail (run 44249495727, #33)**: job "Reproduce all
  reported symptoms (installed release)" (id 134616412887), failure,
  started 06:11:24, completed 06:13:27 → **123 s**.
- **Green job baseline (run 42255492260, #30)**: same job, success,
  16:46:35→17:23:21 → **2206 s**.

### 2.6 release (release.yml)

- **Triggers**: `push: tags: ['v*']`. No branch filter, no PR, no dispatch.
- **Permissions**: `contents: write` (release upload).
- **Concurrency**: **absent**.
- **Jobs**: `full-product` — `runs-on: windows-2022`, `timeout-minutes: 90`.
  No needs/if/strategy/continue-on-error. Step-level `if: always()` on the
  build-log evidence upload.
- **Caches**: **none** — no actions/cache anywhere despite the same vcpkg
  engine build being invoked via `build.ps1` (runs ~23 min per green run).
- **Artifacts**: softprops/action-gh-release upload to the GitHub release
  (`RecoveryWizard-*-setup.exe`, `*-portable.zip`, `SHA256SUMS`,
  `body_path: release-notes.md`, `fail_on_unmatched_files: true`);
  actions/upload-artifact `release-build-evidence` (paths `build/out/*`
  excluding `stage-*/` and `devsign.*`; retention 14; `if: always()`).
  No downloads.
- **Actions**: checkout (SHA, v4), setup-node (SHA, v4),
  pnpm/action-setup (SHA, v4), dtolnay/rust-toolchain (SHA, stable),
  softprops/action-gh-release (SHA, v2), upload-artifact (SHA, v4).
- **Run history (last 5)**: 18 push (tag v0.9.4) success 1400 s;
  17 failure 57 s; 16 (v0.9.3) success 1419 s; 15 (v0.9.1) success
  1307 s; 14 failure 41 s. **Green baseline: 1277 s** (n=8).
- **Latest-run job detail (run 44256566315, #18)**: job "full-product"
  (id 134639554312), success, started 07:08:35, completed 07:31:53 →
  **1398 s**.

### 2.7 ui-ci (ui-ci.yml)

- **Triggers**: `push: branches: [main]` paths `ui/**`, `contracts/**`,
  `.github/workflows/ui-ci.yml`; `pull_request` paths `ui/**`,
  `contracts/**`.
- **Concurrency**: **absent**. **Timeouts**: none → default 360 min.
- **Jobs**: `ci` — `runs-on: windows-latest`, `defaults.run.working-directory: ui`.
  No name/needs/if/strategy/continue-on-error.
- **Caches**: setup-node `cache: pnpm`,
  `cache-dependency-path: ui/pnpm-lock.yaml` (pnpm/action-setup 12.15.0
  installed first). No actions/cache.
- **Artifacts**: upload `web-dist` (path `ui/dist/`; retention 7). No
  downloads.
- **Actions**: checkout (SHA, v4), pnpm/action-setup (SHA, v4),
  setup-node (SHA, v4), dtolnay/rust-toolchain (SHA, stable),
  upload-artifact (SHA, v4).
- **Run history (last 5)**: 109 push success 257 s; 108 success 315 s;
  107 success 242 s; 106 success 341 s; 105 failure 304 s. Older in
  window: 104 success 246 s; 103 success 313 s; 102 success 269 s; 101
  success 361 s. **Green baseline: 301 s** (n=9).
- **Latest-run job detail (run 44339512896, #109)**: job "ci" (id
  134925281948), success, started 15:46:36, completed 15:50:49 → **253 s**.

### 2.8 ui-e2e (ui-e2e.yml)

- **Triggers**: `push: branches: [main]` paths `ui/**`,
  `.github/workflows/ui-e2e.yml`; `pull_request` paths `ui/**`.
- **Concurrency**: **absent**. **Timeouts**: none → default 360 min.
- **Jobs**: `e2e` — `runs-on: windows-latest`, `defaults.run.working-directory: ui`.
  No name/needs/strategy/continue-on-error. Step-level `if: always()` on
  the screenshot upload; `if: failure()` on the traces upload.
- **Caches**: setup-node `cache: pnpm`,
  `cache-dependency-path: ui/pnpm-lock.yaml`. No actions/cache.
- **Artifacts**: upload `e2e-step-screenshots` (path
  `ui/test-results/*.png`; retention 14; if-no-files-found ignore;
  `if: always()`); upload `e2e-traces` (path `ui/test-results/`;
  retention 7; `if: failure()`). No downloads.
- **Actions**: checkout (SHA, v4), pnpm/action-setup (SHA, v4),
  setup-node (SHA, v4), upload-artifact (SHA, v4) x2.
- **Run history (last 5)**: 94 push success 313 s; 93 success 401 s; 92
  success 362 s; 91 success 425 s; 90 success 394 s. All 10 fetched
  green. **Green baseline: 367 s** (n=10).
- **Latest-run job detail (run 44339513001, #94)**: job "e2e" (id
  134925283087), success, started 15:46:36, completed 15:51:46 → **310 s**.

### 2.9 validation-evidence (validation-evidence.yml)

- **Triggers**: `workflow_dispatch` only (no inputs).
- **Concurrency**: **absent**. **Permissions**: contents: read.
- **Jobs** (both no needs/if/strategy/continue-on-error; step-level
  `if: always()` on each evidence upload):
  - `t14-measurements` — name "T14 engine latency + bridge filter/memory
    measurements", `runs-on: windows-2022`, `timeout-minutes: 60`.
  - `t15-installer-matrix` — name "T15 installer runtime matrix (clean
    runner)", `runs-on: windows-2022`, `timeout-minutes: 100`.
- **Caches**: **none** — t15 runs the full `build.ps1 -Target all`
  (same lanes as release) with zero caching.
- **Artifacts**: upload `t14-measurements` (t14 jsonl/log files;
  retention 30; if-no-files-found error), upload `t15-installer`
  (`build/out/RecoveryWizard-*-setup.exe`; retention 30), upload
  `t15-runtime-matrix` (`t15-evidence/`; retention 30). No downloads.
- **Actions**: checkout (SHA, v4) x2, setup-node (SHA, v4) x2,
  pnpm/action-setup (SHA, v4) x2, dtolnay/rust-toolchain (SHA, stable)
  x2, ilammy/msvc-dev-cmd (SHA, v1), upload-artifact (SHA, v4) x3.
- **Run history (last 5)**: 28 failure 570 s; 27 failure 793 s; 26
  failure 57 s; 25 success 930 s; 24 success 913 s (older: 23 success
  943 s). **Green baseline (run level): 928 s** (n=3).
- **Latest-run job detail (run 44262457689, #28)**: t14 job (id
  134658841919) failure 07:53:55→08:03:22 = **567 s**; t15 job (id
  134658842348) failure 07:53:56→08:01:37 = **461 s**.
- **Green job baselines (run 42143983955, #25)**: t14 success
  01:27:41→01:38:11 = **630 s**; t15 success 01:27:42→01:43:08 = **926 s**.

## 3. External actions inventory (all 9 workflows)

All 46 `uses:` steps are pinned to full 40-char commit SHAs — **no
unpinned tag/branch refs anywhere**. Major-version currency (latest
release tags queried via API on 2026-11-18):

| Action | Ref used (SHA-pinned, with comment) | Comment claims | Latest release (API) | Outdated? |
|---|---|---|---|---|
| actions/checkout | 11d5960a326750d5838078e36cf38b85af677262 | v4 | v7.0.1 | Yes — 3 majors behind |
| actions/upload-artifact | ea165f8d65b6e75b540449e92b4886f43607fa02 | v4 | v7.0.2 | Yes — 3 majors behind |
| actions/setup-node | 49933ea5288caeca8642d1e84afbd3f7d6820020 | v4 | v7.1.0 | Yes — 3 majors behind |
| actions/cache | 0057852bfaa89a56745cba8c7296529d2fc39830 | v4 | v6.1.0 | Yes — 2 majors behind |
| pnpm/action-setup | b906affcce14559ad1aafd4ab0e942779e9f58b1 | v4 | v6.1.1 | Yes — 2 majors behind |
| dtolnay/rust-toolchain | b725eda525362e7cf34af9a7243cac1e3f01a7ec | stable (2026-10-16) | v1 | Ref = dated "stable" pin; major current |
| ilammy/msvc-dev-cmd | 0b201ec74fa43914dc39ae48a89fd1d8cb592756 | v1 | v1.13.0 | Current major, old SHA |
| softprops/action-gh-release | 3bb12739c298aeb8a4eeaf626c5b8d85266b0e65 | v2 | v3.0.4 | Yes — 1 major behind |

Deprecated-runtime note: forensic-repro.yml installs
`playwright-core@1.49.1` via npm inside a step (not an action ref), and
runs `setup-node node-version: '20'` while every other Node lane pins
Node 24.

## 4. Top 5 slowest jobs (measured, green or latest, by job-level API duration)

| # | Job (workflow) | Run | Conclusion | Duration basis (API timestamps) |
|---|---|---|---|---|
| 1 | Reproduce all reported symptoms (installed release) — forensic-repro | 42255492260 | success | 2026-11-06 16:46:35 → 17:23:21 = **2206 s** |
| 2 | full-product — release | 44256566315 | success | 2026-11-18 07:08:35 → 07:31:53 = **1398 s** |
| 3 | T15 installer runtime matrix (clean runner) — validation-evidence | 42143983955 | success | 2026-11-06 01:27:42 → 01:43:08 = **926 s** |
| 4 | Build engine + link bridge + run integration tests — bridge-integration | 44334552899 | success | 2026-11-18 15:20:48 → 15:35:08 = **860 s** |
| 5 | Build + test (MSVC Release, vcpkg cache) — engine-ci | 44275761807 | success | 2026-11-18 09:26:02 → 09:40:15 = **853 s** |

(Run-level wall times are longer where multiple jobs ran: green engine-ci
runs measured 697–913 s run-level vs 853 s for the build job alone at
09:26; validation-evidence #25 run-level 930 s covers both jobs in
parallel. t14 in run 42143983955 measured 630 s.)

## 5. Optimization-relevant observations (facts, no recommendations)

- 6 of 9 workflows have **no concurrency group** (contracts-ci,
  forensic-repro, release, ui-ci, ui-e2e, validation-evidence) — only
  benchmark-src, bridge-integration, engine-ci declare one; benchmark-src
  is the only one with `cancel-in-progress: false`.
- 3 of 9 workflows have **no explicit job timeout** (contracts-ci,
  ui-ci, ui-e2e → default 360 min).
- No workflow uses `strategy`/matrix at all; no `continue-on-error: true`
  anywhere (one comment in benchmark-src explicitly forbids masking).
- Every action ref is SHA-pinned, but the v3/v4-era majors trail current
  releases by 2–3 majors (Section 3 table).
- Caching exists only in engine-ci, bridge-integration, and the pnpm
  built-in caches of ui-ci/ui-e2e/bridge-integration; release.yml and
  validation-evidence.yml run the identical full `build.ps1` Windows build
  with **zero caching** (t15 green job 926 s; release green job 1398 s).
- The default branch is currently red on engine-ci (lint job 36 s →
  build skipped), bridge-integration (106 s fail) and benchmark-src (686 s
  fail) at head 9352c81daf — all three failing runs recorded above.
- The only cancelled run in the fetched window is bridge-integration
  #37 (run 44264525026, cancelled at 08:09 — consistent with its
  cancel-in-progress: true on the next push).
- engine-ci lint failure is cheap (36–43 s to red), but the lane's green
  critical path is ~690–888 s, dominated by the uncached-in-part build job
  (636–853 s measured on green runs #60 and #59).
