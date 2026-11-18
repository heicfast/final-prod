# CACHE-AUDIT — cache/artifact audit of the 9 workflows (T4)

Session: CI/CD optimization, subagent D. Date 2026-11-18. Tree at product commit
9352c81d (HEAD 25537feb, worklog-only). Sources: the 9 files under
`.github/workflows/` (line numbers cite that tree), GitHub REST API
(`/repos/heicfast/final-prod/actions/caches`, `/actions/artifacts`, run/job
logs), and git history for deleted workflows. Every claim cites one of those.
Scratch logs under `/home/z/my-project/ci-logs/`. Facts only; no ratio is
claimed beyond the runs actually read.

## 1. Inventory — cache uses, built-in caches, artifacts, uncached installs

### 1a. actions/cache uses (4 total, in 2 workflows)

| # | Workflow:job | Lines | path | key | restore-keys |
|---|---|---|---|---|---|
| C1 | engine-ci:lint | engine-ci.yml:86-91 | `~\AppData\Local\pip\Cache` | `pip-clangformat-win2022` (static, no hash) | `pip-clangformat-win2022` (identical to key) |
| C2 | engine-ci:build-and-test | engine-ci.yml:273-278 | `${{ runner.temp }}\vcpkg-binary-cache` | `vcpkg-bin-win2022-${{ hashFiles('engine/vcpkg.json') }}` | `vcpkg-bin-win2022-` |
| C3 | bridge-integration:bridge | bridge-integration.yml:65-73 | `~/.cargo/registry`, `~/.cargo/git`, `ui/src-tauri/it/target` | `bridge-it-rust-win2022-${{ hashFiles('ui/src-tauri/Cargo.lock') }}-${{ hashFiles('ui/src-tauri/it/Cargo.toml') }}` | `bridge-it-rust-win2022-` |
| C4 | bridge-integration:bridge | bridge-integration.yml:119-124 | `${{ runner.temp }}\vcpkg-binary-cache` | `vcpkg-bin-win2022-${{ hashFiles('engine/vcpkg.json') }}` | `vcpkg-bin-win2022-` |

### 1b. Built-in dependency caches (setup-* `cache:`)

| # | Workflow:job | Lines | action | node-version | cache | cache-dependency-path |
|---|---|---|---|---|---|---|
| B1 | bridge-integration:bridge | bridge-integration.yml:80-85 | setup-node@49933ea5 v4 | 24 | `pnpm` | `ui/pnpm-lock.yaml` |
| B2 | ui-ci:ci | ui-ci.yml:37-41 | setup-node@49933ea5 v4 | 24 | `pnpm` | `ui/pnpm-lock.yaml` |
| B3 | ui-e2e:e2e | ui-e2e.yml:35-39 | setup-node@49933ea5 v4 | 24 | `pnpm` | `ui/pnpm-lock.yaml` |

setup-node WITHOUT cache (no `cache:` input): contracts-ci.yml:35-37
(node-version 24, node contracts/validate.mjs only — no install, cache not
needed); forensic-repro.yml:63-65 (node-version '20'); release.yml:50-52
(node-version 24); validation-evidence.yml:161-163 (t15 job, node 24). No
setup-python / setup-go anywhere in the 9 files.

### 1c. upload-artifact uses (10 total)

| # | Workflow:job | Lines | name | path | retention-days | condition |
|---|---|---|---|---|---|---|
| A1 | benchmark-src:benchmark | benchmark-src.yml:150-158 | `benchmark-src-evidence-${{ inputs.profile }}-${{ github.run_id }}` | benchmark-results/, runner-context.json | 90 | always (implicit) |
| A2 | bridge-integration:bridge | bridge-integration.yml:298-307 | `bridge-it-evidence-${{ github.run_number }}` | ${{ runner.temp }}\cr-it-evidence, abi-layouts.json | 30 | `if: always()` |
| A3 | engine-ci:build-and-test | engine-ci.yml:496-505 | `engine-ci-failure-diagnostics-${{ github.run_id }}` | CMakeCache.txt, Testing/**/*.log | 7 | `if: failure()` |
| A4 | forensic-repro:repro | forensic-repro.yml:305-317 | `forensic-repro-evidence` | C:\forensic\evidence\**, ProgramData logs, twin-data | 30 | `if: always()` |
| A5 | release:full-product | release.yml:283-296 | `release-build-evidence` | build/out/* minus stage-*/devsign.* | 14 | `if: always()` |
| A6 | ui-ci:ci | ui-ci.yml:111-116 | `web-dist` | ui/dist/ | 7 | always (implicit) |
| A7 | ui-e2e:e2e | ui-e2e.yml:57-64 | `e2e-step-screenshots` | ui/test-results/*.png | 14 | `if: always()` |
| A8 | ui-e2e:e2e | ui-e2e.yml:66-73 | `e2e-traces` | ui/test-results/ | 7 | `if: failure()` |
| A9 | validation-evidence:t14 | validation-evidence.yml:141-152 | `t14-measurements` | t14-*.jsonl/log, runner-context.json | 30 | `if: always()` |
| A10 | validation-evidence:t15 | validation-evidence.yml:196-210 | `t15-installer`, `t15-runtime-matrix` | setup.exe; t15-evidence/ | 30 | always (implicit) / `if: always()` |

actions/download-artifact: zero uses in all 9 workflows (grep over
`.github/workflows/` returned nothing). Release assets are published via
softprops/action-gh-release (release.yml:273-281), not artifact download.

### 1d. Dependency installation with NO cache at all

| Workflow:job | Lines | What is installed uncached |
|---|---|---|
| forensic-repro:repro | forensic-repro.yml:67-73 | `npm init -y` + `npm install playwright-core@1.49.1` in tools/forensic (no lockfile, no cache; setup-node has no `cache:` input). Run 44249495727 job log: "npm install playwright-core@1.49.1" -> "added 1 package" — works, but re-resolves every dispatch |
| bridge-integration:bridge | bridge-integration.yml:107 | `pip install clang-format==18.1.8` inside step [1/5] — NO pip cache (engine-ci's C1 caches the pip wheel dir but bridge-integration does not; logs of runs 44334578460 / 44275761901 show the bare `pip install` line) |
| release:full-product | release.yml:62-67 (build.ps1 -Target all) | build.ps1 sets `VCPKG_ROOT` (build.ps1:60-62) but never `VCPKG_DEFAULT_BINARY_CACHE` — vcpkg falls back to the runner-local default archive. Run 44256566315 job log: "Restored 0 package(s) from C:\Users\runneradmin\AppData\Local\vcpkg\archives in 102 us", then Installing 6/6 with "Elapsed time to handle gtest: 36 s, zstd: 17 s" (~95 s total, C2/C4's 17 MB cache would have skipped this) |
| release:full-product | build.ps1 pnpm step | `pnpm install` inside build.ps1 (run log: "Progress: resolved 370, reused 0, downloaded 366/370" — full 366-package download; setup-node at release.yml:50-52 has NO `cache:` input, so no pnpm store cache in the release lane) |
| benchmark-src:benchmark | benchmark-src.yml:56-60 | `./build.ps1 -Target engine -SkipTests` — same uncached vcpkg; run 44339547294 job log: "Restored 0 package(s)", gtest 43 s, zstd 18 s |
| validation-evidence:t14 + t15 | validation-evidence.yml:47 / :177 | same `build.ps1` engine lane; run 44262457689 job 134658841919 log: "Restored 0 package(s)", gtest 43 s, zstd 17 s. t15 also runs build.ps1 all (job 134658842348, same restore-0) |
| ui-ci:ci | ui-ci.yml:99-109 | `cargo check --tests` + `cargo clippy` with NO cargo/registry cache and no `~/.cargo`/target caching (dtolnay/rust-toolchain installs the toolchain; the registry is cold). Run 44339512896 step log: "Downloading crates ..." then "Finished `dev` profile ... in 1m 17s", 204 "Compiling" lines — all of it re-downloaded and re-compiled on every run |
| contracts-ci:validate | contracts-ci.yml:35-40 | setup-node without cache, but the job only runs two `node` scripts — no package install, so nothing to cache (correct, not a gap) |

## 2. Correctness assessment per cache

### C1 pip-clangformat-win2022 (engine-ci:lint, engine-ci.yml:86-91)

- OS in key: yes, by literal (`win2022`) — the job is pinned `runs-on: windows-2022` (engine-ci.yml:78), so the literal is truthful.
- Content-addressed: NO. The key is a static string; the pinned version
  (`clang-format==18.1.8`, engine-ci.yml:102) is not in the key. A version
  bump reuses the old wheel cache (harmless for a pure wheel cache, but the
  cache can never be invalidated by content — it is only evicted by GitHub's
  7-day-unused policy).
- restore-keys: present but identical to the primary key
  (`pip-clangformat-win2022` both, lines 90-91) — a "partial hit" here equals
  a full hit; the restore-key adds nothing.
- Order: restore (step at line 86) precedes `pip install` (line 102). Correct.
- Stale-content risk: LOW. Path is the pip HTTP cache dir; pip verifies
  hashes when installing from it. 1.72 MB (API) — cheap either way.
- Verdict: works (see hit evidence below) but the key strategy is degenerate —
  effectively a permanent key.

### C2 vcpkg-bin-win2022-<hash(vcpkg.json)> (engine-ci:build-and-test, engine-ci.yml:273-278)

- OS in key: yes (`win2022` literal; job pinned windows-2022 at engine-ci.yml:254).
- Content-addressed: yes — `hashFiles('engine/vcpkg.json')` over the real
  manifest (engine/vcpkg.json exists, lists zlib/gtest/lz4/zstd). Any
  dependency change re-keys. Not included: the vcpkg toolchain version
  itself (runner-image pinned, so acceptable) and CMakeLists/build scripts
  (a port-version bump by the image would re-key nothing — the known blind
  spot, mitigated by the pinned runner image).
- restore-keys: `vcpkg-bin-win2022-` — correct partial-hit fallback
  (restores old binaries; vcpkg rebuilds only what is missing/ABI-mismatched).
- Order: cache step (273) precedes the Configure+build step (281) that sets
  `VCPKG_DEFAULT_BINARY_CACHE` (286) — correct.
- Stale-content risk: LOW-CONTROLLED. Binary packages are ABI-hash addressed
  inside the cache; a stale restore only helps, never serves wrong bits.
- Verdict: correct strategy; the one real defect is duplication — see C4.

### C3 bridge-it-rust-win2022-<hash(Cargo.lock)>-<hash(it/Cargo.toml)> (bridge-integration:bridge, bridge-integration.yml:65-73)

- OS in key: yes (`win2022`; job pinned windows-2022 at line 53).
- Content-addressed: partially. `hashFiles('ui/src-tauri/Cargo.lock')` is the
  real lockfile (exists in tree). `hashFiles('ui/src-tauri/it/Cargo.toml')`
  hashes the it-crate manifest — a reasonable proxy because no
  `ui/src-tauri/it/Cargo.lock` exists (it resolves through the parent
  workspace). MISSING from the key: the lib crate's own
  `ui/src-tauri/Cargo.toml` is only covered transitively through Cargo.lock
  (fine for deps), and rustc/toolchain version is not in the key —
  dtolnay/rust-toolchain@stable (bridge-integration.yml:63, comment "stable
  (2026-10-16)") floats with stable; a new stable over a stale
  `it/target` gives incremental rebuilds, not wrong outputs (cargo re-checks
  fingerprints), so correctness holds, only cache efficiency degrades.
- restore-keys: `bridge-it-rust-win2022-` — correct partial-hit prefix.
- Order: cache (65) precedes pnpm setup / frontend build (75-97) and cargo
  test (270-296). Correct.
- Stale-content risk: LOW (cargo fingerprints), but the cache is HUGE: the
  API shows two entries under the same key prefix at 733.38 MB and 584.55 MB
  (see §3) — the `it/target` dir includes debug test binaries; the key does
  not include a `--release`/debug discriminator or a cleanup of stale
  artifacts, so two near-duplicate entries coexist.
- Verdict: strategy broadly correct; flaws are size and missing toolchain
  fingerprint in key.

### C4 vcpkg-bin-win2022-<hash(vcpkg.json)> (bridge-integration:bridge, bridge-integration.yml:119-124)

- Identical key template to C2 (same manifest hash, same restore-key, same
  path under runner.temp, same env wiring at line 132). That is deliberate
  sharing: both jobs hit the SAME cache entry (API shows exactly one
  `vcpkg-bin-win2022-e95e...` entry, 17.46 MB) — cross-workload reuse works.
- All C2's correctness notes apply. Order: cache (119) precedes the engine
  build step (126). Correct.
- Verdict: correct; the shared-key design is the good pattern here.

### B1/B2/B3 setup-node cache: pnpm (bridge-integration.yml:80-85, ui-ci.yml:37-41, ui-e2e.yml:35-39)

- OS: implicit — setup-node's internal key embeds `Windows-x64`
  (API/log key `node-cache-Windows-x64-pnpm-...`). Correct.
- Content-addressed: yes — `cache-dependency-path: ui/pnpm-lock.yaml` hashes
  the real lockfile (exists; 547 entries per the install log "Verifying
  lockfile against supply-chain policies (547 entries)").
- restore-keys: handled internally by the action (no manual list).
- Order: setup-node precedes `pnpm install --frozen-lockfile`
  (bridge-integration.yml:94, ui-ci.yml:44, ui-e2e.yml:42). Correct.
- Shared key across all three workflows (same lockfile path) — one entry
  (100.86 MB) serves all lanes. Run evidence in §4: "reused 363" /
  "downloaded 0" in ui-ci run 44339512896.
- Verdict: correct, textbook use.

### Jobs with install and NO cache — correctness

- forensic-repro npm install (1d row 1): an un-pinned-in-lockfile
  `npm install playwright-core@1.49.1` — no lockfile exists to hash, so the
  only correct cache would be a version-templated key; today it re-downloads
  ~1 package/run (log: "added 1 package... in 1s"). Low cost, low priority;
  the missing piece is that `npm init -y` mutates the tree with a generated
  package.json each run (side-effect, not cache-related).
- bridge-integration pip install (1d row 2): the pip wheel cache exists as a
  pattern in engine-ci (C1) and is simply not replicated here. Cost per run:
  the step log shows the install between 15:22:42.21 and the generator output
  at 15:22:47.33 (run 44334578460) — a ~5 s window that includes the wheel
  download over the network on every bridge run.
- release / benchmark-src / validation-evidence vcpkg (1d rows 3-6): the C2
  cache exists and is proven (17.46 MB, hit on every engine-ci/bridge run),
  but the three build.ps1-based lanes never set
  `VCPKG_DEFAULT_BINARY_CACHE` and re-build all 6 ports from source each
  run: gtest 36-43 s, lz4 ~7 s, zlib ~7 s, zstd 17-18 s, ~95 s+ per lane per
  run (release run 44256566315 and validation run 44262457689 logs above).
  This is the largest single cache gap in the repo.
- release pnpm: setup-node without `cache:` (release.yml:50-52) while the
  same repo's other three lanes set it — the release run re-downloads 366+
  370 packages every tag (run 44256566315 log).
- ui-ci cargo: no cargo cache at all; every run downloads all crates and
  compiles 204 of them (~1m17s of pure compile + download; run 44339512896
  step log). The same it/target caching pattern (C3) is proven next door in
  bridge-integration.
- Is any cache keyed so broadly that it could serve stale/incorrect content?
  The broadest keys are C1 (static string) and C3's restore-key prefix
  (`bridge-it-rust-win2022-`). Neither can serve WRONG content (pip verifies
  wheel hashes; cargo/vcpkg verify fingerprints/ABI hashes on top of
  restored state), so the failure mode is only wasted storage/missed
  invalidation, not incorrect builds. No key includes the runner OS as a
  variable expression, but all caching jobs are pinned to windows-2022 /
  windows-latest for B2/B3 — note B2/B3 run on `windows-latest`
  (ui-ci.yml:26, ui-e2e.yml:24) while the setup-node key embeds Windows-x64,
  which stays valid across windows-latest image updates; if those lanes
  ever move to a non-Windows runner the built-in key adapts automatically.
  The one genuine cross-OS hazard does not exist today because no caching
  job runs on more than one OS.

## 3. Actual cache and artifact state (API, 2026-11-18)

### GET /actions/caches?per_page=100 — total_count 5, total 1437.97 MB (~1.4 GB)

| key | size | created_at | last_accessed_at | read |
|---|---|---|---|---|
| `node-cache-Windows-x64-pnpm-0e9d512facb51c19…` | 100.86 MB | 2026-11-05T16:34:44Z | 2026-11-18T15:48:14Z | B1/B2/B3 |
| `bridge-it-rust-win2022-066a39f4…-802b7afd…` | 733.38 MB | 2026-11-18T06:53:46Z | 2026-11-18T15:47:24Z | C3 |
| `pip-clangformat-win2022` | 1.72 MB | 2026-11-05T17:37:29Z | 2026-11-18T15:46:59Z | C1 |
| `vcpkg-bin-win2022-e95ef99dd…` | 17.46 MB | 2026-11-05T17:54:42Z | 2026-11-18T15:24:01Z | C2/C4 |
| `bridge-it-rust-win2022-066a39f4…-802b7afd…` | 584.55 MB | 2026-11-18T05:43:21Z | 2026-11-18T06:46:18Z | older duplicate of the 733 MB entry |

Stale/unused: none of the 5 is past GitHub's 7-day eviction — all were
accessed within the last day except the 584.55 MB rust entry, whose
last_accessed_at is 2026-11-18T06:46 (same day). The REAL waste is the
duplicate: TWO entries share the identical full key suffix
`…-802b7afd1f54b1293db27d4850f90537d5a25a5cb83b438be6bbe8d9d04d0133`
(584.55 MB from 05:43 and 733.38 MB from 06:53). The key is character-for-
character identical in the API listing, i.e. the same logical cache was
saved twice (the 06:53 save happened after a run that started with a
restore-key partial hit and re-saved; both remain because eviction of the
older one lags). 584.55 MB — 41% of the repo's entire cache storage — is a
redundant copy of a cache that the newest entry already supersedes.

### GET /actions/artifacts?per_page=100 — total_count 220, total 326.74 MB, none expired

Oldest: `rda-wdk26100-Release` 2026-10-26T15:30:16Z. Newest:
`e2e-step-screenshots` 2026-11-18T15:51:41Z.

| family | n | total size | oldest | newest | still produced? |
|---|---|---|---|---|---|
| e2e-step-screenshots | 69 | 91.63 MB | 11-05 | 11-18 | yes (ui-e2e A7) |
| forensic-repro-evidence | 30 | 130.93 MB | 10-27 | 11-18 | yes (A4) |
| t14-measurements | 19 | 0.10 MB | 10-26 | 11-18 | yes (A9) |
| t15-runtime-matrix | 17 | 0.07 MB | 10-26 | 11-06 | yes (A10; dispatch-only, idle since 11-06) |
| web-dist | 19 | 6.92 MB | 11-11 | 11-18 | yes (A6) |
| bridge-it-evidence-N | 26 | 0.05 MB | 11-11 | 11-18 | yes (A2) |
| engine-ci-failure-diagnostics-N | 8 | 0.54 MB | 11-17 | 11-18 | yes (A3, failure-only) |
| release-build-evidence | 10 | 67.21 MB | 11-05 | 11-18 | yes (A5) |
| t15-installer | 8 | 28.22 MB | 10-26 | 11-06 | yes (A10, dispatch-only) |
| rda-wdk26100-Debug/Release | 14 | 0.62 MB | 10-26 | 10-27 | NO — ORPHANED |

Orphaned names: `rda-wdk26100-Debug` (7) and `rda-wdk26100-Release` (7)
were produced by the DELETED `driver-ci` workflow (git: commit 318b9655
deleted `.github/workflows/driver-ci.yml` on 2026-11-05; its upload step was
`name: rda-${{ matrix.tag }}-${{ matrix.configuration }}` with tag
`wdk26100` and NO retention-days — default 90). Their expires_at is
2027-01-24/25 — they will keep occupying storage for ~2 more months. Small
(0.62 MB) but the only true orphans. Everything else maps to a live upload
step in the current 9 workflows.

Also notable: `bridge-it-evidence-N` uses `github.run_number` in the name
(bridge-integration.yml:302) and `benchmark-src-evidence-…-run_id` (90-day
retention, benchmark-src.yml:153-157) — one artifact PER RUN, never
overwritten; with 90-day retention the benchmark lane will accumulate one
artifact per dispatch indefinitely (none exist yet — the lane is new and
its runs 44339547294/44334634961 failed before the upload step).

## 4. Hit-ratio evidence — per recent run, exact keys (log lines actually read)

Workflows that use caching: engine-ci, bridge-integration, ui-ci, ui-e2e.
3 most recent completed runs each (success), logs downloaded via
runs/{id}/logs and jobs/{id}/logs. No "cache not found" / miss line appears
in ANY of the 12 runs — every cache restored was a primary-key full hit.

engine-ci (runs 44334569972, 44334552871, 44275761807 — all 2026-11-18):

| run | cache | verdict | exact key from the log |
|---|---|---|---|
| 61 (44334569972) | C1 pip | HIT + no-save | `pip-clangformat-win2022` |
| 61 | C2 vcpkg | HIT + no-save | `vcpkg-bin-win2022-e95ef99dd23f957b01bd371203ca55116a201a8df4e021adfe3130d8a2605a1b` |
| 60 (44334552871) | C1 pip | HIT + no-save | `pip-clangformat-win2022` |
| 60 | C2 vcpkg | HIT + no-save | same `vcpkg-bin-win2022-e95e…a1b` |
| 59 (44275761807) | C1 pip | HIT + no-save | `pip-clangformat-win2022` |
| 59 | C2 vcpkg | HIT + no-save | same `vcpkg-bin-win2022-e95e…a1b` |

bridge-integration (runs 44334578460, 44334552899, 44275761901):

| run | cache | verdict | exact key from the log |
|---|---|---|---|
| 43 (44334578460) | C3 rust | HIT + no-save | `bridge-it-rust-win2022-066a39f4bd7f47b94b57e6e9942a64b4c7142bd483f80ef8e6e971c39017805b-802b7afd1f54b1293db27d4850f90537d5a25a5cb83b438be6bbe8d9d04d0133` (~699 MB restored) |
| 43 | B1 pnpm | HIT + no-save | `node-cache-Windows-x64-pnpm-0e9d512facb51c19ae1bdf6b04509b9e1c6074734f49ac343e25f011e2c57be8` |
| 43 | C4 vcpkg | HIT + no-save | `vcpkg-bin-win2022-e95e…a1b` |
| 42 (44334552899) | C3/B1/C4 | all HIT + no-save | same three keys |
| 41 (44275761901) | C3/B1/C4 | all HIT + no-save | same three keys |

ui-ci (runs 44339512896, 44260156149, 44257909832): B2 pnpm — HIT +
no-save on the single key `node-cache-Windows-x64-pnpm-0e9d…57be8` in all
three; run 109's install log confirms the store reuse: "Progress: resolved
363, reused 363, downloaded 0, added 360".

ui-e2e (runs 44339513001, 44260156160, 44257909802): B3 pnpm — HIT +
no-save on the same `node-cache-…57be8` key in all three ("Cache Size:
~96 MB (100860856 B)").

Not measured: cache-save behavior on a key CHANGE (no run in the sample
saved a new cache — every Post Job line reads "Cache hit occurred on the
primary key …, not saving cache"). Miss evidence therefore CANNOT be given
from these 12 runs, and no claim is made about which specific run wrote the
two bridge-it-rust cache entries (05:43 and 06:53, per the API listing)
beyond their timestamps. No hit RATIO is claimed beyond: 12/12 runs, 100%
primary-key hits, 0 misses observed, 0 restore-key partial hits observed.

## 5. Findings summary (ranked)

1. Largest gap: release.yml, benchmark-src.yml, validation-evidence.yml run
   build.ps1's vcpkg engine build with NO binary cache — ~95 s of port
   rebuilding per lane per run (release run 44256566315, validation run
   44262457689, benchmark run 44339547294 all show "Restored 0 package(s)"),
   while a proven 17.46 MB cache with the exact right key shape (C2/C4)
   exists in the sibling workflows.
2. Second gap: release.yml setup-node has no `cache: pnpm` (release.yml:50-52)
   — 366/370 packages re-downloaded per release (run 44256566315 log) while
   the other three node lanes share one 100.86 MB store.
3. Third gap: ui-ci's cargo check/clippy (ui-ci.yml:99-109) has no cargo
   cache — 204 crates re-downloaded/re-compiled (~1m17s) every run (run
   44339512896 log); the C3 pattern from bridge-integration is the template.
4. Duplicate cache: two identical-key `bridge-it-rust-win2022-…-802b7afd…`
   entries (733.38 MB + 584.55 MB = 41% of all cache storage). Evicting the
   05:43 one (cache id 10706535329) frees 584.55 MB with zero hit-rate cost.
5. Static key C1 (`pip-clangformat-win2022`, no hash, restore-key identical
   to key): works, but it is an unversioned permanent cache — keying it on
   the pinned version (`pip-clangformat-win2022-18.1.8`) would be strictly
   more truthful; bridge-integration's copy of the same pip install (line
   107) has no cache at all.
6. Orphaned artifacts: 14 `rda-wdk26100-*` artifacts (0.62 MB) from the
   deleted driver-ci workflow linger until 2027-01-24/25 — deletable now
   (DELETE /actions/artifacts/{id}); every other artifact family maps to a
   live workflow.
7. Artifact accumulation: `benchmark-src-evidence-…-${{ github.run_id }}`
   at retention-days 90 (benchmark-src.yml:153,157) creates one
   never-overwritten artifact per dispatch for 3 months; same pattern
   (run-number suffix) at bridge-it-evidence but only 30 days and ~0.002 MB
   each.
8. What is already right: C2/C4 share one key across workflows (one 17.46
   MB entry serving both), all three pnpm lanes share one node-cache entry,
   every restore step precedes its consumer, every key that hashes a file
   hashes a real one (vcpkg.json, pnpm-lock.yaml, Cargo.lock), and all
   cacheable jobs are pinned to a single runner image so the win2022 literal
   in keys is truthful.
