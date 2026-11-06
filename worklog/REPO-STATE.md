# REPO-STATE — final-prod at session T1 open (Agent A)

> HEAD = `108578a6` (main, origin/main), 2026-11-06 16:05, worklog-only commit.
> 2,363 commits since root `03fcad0a` (2026-10-26, "final-prod identity — VERSION 0.5.0").
> Every claim below cites file:line, run-id, or commit sha. Inferences are labeled.
> Produced for the debug-and-release session (worklog/session-debug-and-release-prompt.md §2, Agent A).

---

## 1. Commit graph of main — last 30 commits, grouped by phase

### Phase T1 — the debug session opens (2026-11-06 15:56–16:05, 2 commits, worklog only)
| sha | date | first line |
|---|---|---|
| `108578a6` | 11-06 16:05 | worklog(T1 lane B): coordination note — parallel session acknowledged, D4 decomposed (carve CPU wall, frozen bytes_scanned stamping, bytes_found ABI gap) |
| `c8aef05a` | 11-06 15:56 | worklog(T1): session opens — user's report recorded; four spot-check root causes named (missing requireAdministrator manifest; `_popen` console in cr_health; BF-22 is real; enumeration axes) |

### Phase "forensic hardening" — post-v0.8.4 probe fixes (2026-11-06 11:05–14:06, 7 commits; touched only `tools/forensic/**`, `.github/workflows/forensic-repro.yml`, `worklog*` — verified via `git log --name-only 4b297849..HEAD`)
| sha | date | first line |
|---|---|---|
| `4c522cd3` | 14:06 | evidence(gate §8+§9): FORENSIC #29 — the complete green run, every step, on v0.8.4 |
| `0d09bca7` | 13:34 | fix(forensic uninstall): the emptied install dir is the caller's to remove too (42220997177 post-mortem) |
| `2f89f833` | 12:52 | fix(forensic uninstall): the NSIS `_?=` self-survivor — the synchronous uninstaller cannot delete its own exe (42215611622) |
| `f48f0396` | 12:10 | fix(forensic): resumed-completion cap sized to measured carve variance — run 26 completed at 962 s vs a 960 s cap (42212963611) |
| `c0e590a4` | 11:48 | fix(forensic teardown): sweep WebView2 browser children before uninstall — NSIS silently skips locked files (42207694695) |
| `44d166e6` | 11:05 | fix(forensic twin): unelevated launcher gives WebView2 a real user-data folder — `Start-Process -Credential` never loads the profile (42201790752) |
| `4b297849` | 09:49 | **chore(release): v0.8.4** — the whole-disk session resume fix (tag) |

### Phase v0.8.x release train (2026-11-06 05:04–09:49, 10 commits) — each release = one product fix + one forensic probe fix
| sha | date | first line |
|---|---|---|
| `559080ae` | 09:49 | fix(session resume): whole-disk sessions re-attach by size among identical-model disks (42195967961) — bridge `ui/src-tauri/src/commands/sessions.rs` |
| `a5e6ae7a` | 09:23 | chore(forensic): resume-hang instrumentation — the S5 gate carries its own evidence (42185123973 + 42192131100) |
| `939dd483` | 08:26 | **chore(release): v0.8.3** — the invisible-resume fix (tag) |
| `1cd9bc64` | 08:26 | fix(backup resume): `/scan?scanId=` is not a route; the resumed scan ran behind the fallback page (42185123973) |
| `ff9e4c13` | 07:35 | **chore(release): v0.8.2** — the stopped-scan banner fix (tag) |
| `f76c9941` | 07:35 | fix(scan state): the state poller froze at 'stopping' forever (42178485694) |
| `23ec07c6` | 07:04 | fix(forensic): BF-25 generalized — drain window sized to the carve matcher (42176600928) |
| `97893c15` | 06:17 | **chore(release): v0.8.1** — the backup-list truth fix (tag) |
| `73419713` | 06:17 | fix(backup + forensic): BF-26 — a valid whole-disk session is not '(unreadable session)' (42170466748) |
| `f1c0418f` | 05:04 | **chore(release): v0.8.0** — the session-save visibility release (tag) |

### Phase P6 tail — behavior findings + probe hardening (2026-11-06 00:16–05:04)
| sha | date | first line |
|---|---|---|
| `95d5d1de` | 05:04 | fix(forensic): BF-25 root-caused + save/resume gates re-vehicled (42148493806) |
| `d3982b01` | 02:15 | style: prettier the save onError (ui-ci Format check red on db600f4c, run 42148477718) |
| `db600f4c` | 02:09 | fix(session save): the refused save is now VISIBLE — onError toast + probe retries through drain window (42143819243) |
| `8edb721c` | 01:26 | worklog(P6 in flight): **BF-22 = 76.1% deep-scan stall (identical pre/post merge, environmental/known-perf)**; BF-23 SMART absent on Azure VM disks; release run 42126551916: 1741/1741 ctest |
| `eaec57b5` | 01:25 | fix(forensic): report wait hardens against WebView2 rejection (42138026793) |
| `ff5b337b` | 00:36 | fix(forensic): mount rows render VOLUME LABEL not drive letter — `system_list_mounts` returns `fixed_str(&v.label)` (run 42135600496 cancelled pre-burn) |
| `9ba50525` | 00:34 | evidence(gate 1+9): P6 driver-sweep re-proof — 76 files/1,024 raw hits, zero unclassified live references |
| `db5f0b81` | 00:16 | fix(forensic v2): unelevated twin gets a genuinely restricted token + recover/session surfaces join gate set (42129256087) |

### Phase P4/P5/P6-open — UI wiring, CI finalization, first driverless release (2026-11-05)
| sha | date | first line |
|---|---|---|
| `68ab3fcd` | 22:58 | **chore: v0.7.0** (tag) — evidence(P6 prep): RELEASE-VALIDATION sheets + BEHAVIOR-FINDINGS + forensic uninstall leg |
| `d166147b` | 22:42 | worklog(P4+resume): P4 entry reconstructed; red-main closure (db7f588a) + P5 landing |
| `a5cfa11f` | 22:41 | chore(P5): workflows + CI finalization — every action SHA-pinned, forensic lane redesigned for the driver-free product |
| `db7f588a` | 22:31 | fix(P4 tail): five clippy errors + e2e re-hydration leg — both red lanes on e33dd819 go green (42109647269, 42109647145) |
| `e33dd819` | 21:06 | fix(P4): ShellExecuteW's HWND is `*mut c_void` — null_mut, not null (42107935888) |
| `8c7b82f2` | 20:55 | fix(P4): seven Windows compile errors + two clippy warnings (42105861785) |
| `0a43f678` | 20:49 | fix(e2e): consent re-hydration navigates client-side — page.goto() resets the TEST DOUBLE's module state |
| `70741eb7` | 20:42 | fix(P4 reconcile): convergent-lane landing — parallel P4 reconciled, three real defects + missing CI gates |

Earlier (still in the last ~50): P4's `8e61f4a8` ("the UI is WIRED to the real engine — the v4 bridge lands, the mock demoted to TEST DOUBLE"), P3's driver REMOVAL (`318b9655` "the kernel driver is REMOVED from the product"), P2's engine advance (`283a5d90` merge to core_recovery@12f3edf7, ABI v4), P1 re-base (`0eaec2ee`).

### Known-good release points (tags, all CI-green when cut)
| tag | sha | what it fixed (commit subjects + release.yml:119-157 notes) |
|---|---|---|
| v0.7.0 | `68ab3fcd` | the driverless product: kernel driver removed (P3a `318b9655`), UI wired to the real ABI v4 engine (P4 `8e61f4a8`), CI lanes finalized (P5 `a5cfa11f`) |
| v0.8.0 | `f1c0418f` | refused session save made VISIBLE (onError toast; drain-window retry) — BF-25 vehicle |
| v0.8.1 | `97893c15` | Backup list truth: valid whole-disk sessions no longer "(unreadable session)" (BF-26) |
| v0.8.2 | `ff9e4c13` | stopped-scan banner: state poller froze at 'stopping' (partial results unlabelled) |
| v0.8.3 | `939dd483` | invisible resume: `/scan?scanId=` not a route — resumed scans ran behind a fallback page |
| v0.8.4 | `4b297849` | whole-disk session resume re-attach by SIZE among identical-model disks (VM disk sets) |

"Known-good" = CI-good (all push lanes + release + forensic #29 green at v0.8.4 — §2, §5). It is **not** user-good: the T1 spot-checks (commit `c8aef05a`) name the elevation-manifest and `_popen` defects as real in this same code, and the user's report is against this release train.

Older tags: v0.5.0 (+rc1–rc3), v0.6.0-rc1/rc2 (the driver era, pre-P3 removal).

---

## 2. Workflow inventory + LIVE status (GitHub Actions API, fetched this session)

No `continue-on-error` exists in any of the 7 workflows (grep: only `if: always()`/`if: failure()` artifact-upload steps — engine-ci.yml:497, ui-e2e.yml:58,67, forensic-repro.yml:191,298, validation-evidence.yml:141,204, release.yml:201). All actions are SHA-pinned (P5 `a5cfa11f`).

### 2.1 contracts-ci (.github/workflows/contracts-ci.yml, 43 ln)
- **What**: ubuntu-24.04, single job. Runs `node contracts/validate.mjs` (phase 1: error-code map vs `engine/include/cr/cr_error.h`, every uiCode vs `ui/src/connector/contract.ts`, CR_EVT_* events vs `cr_ui.h`, command-surface well-formedness; phase 2 auto-activates once `ui/src-tauri/src/commands/` exists — validate.mjs:10-16 — the bridge landed in P4, so phase 2 IS live; the header comment "inert while the bridge is absent" at contracts-ci.yml:5-7 is stale) + `ui/scripts/gen-filefilter.mjs --check` (engine table ↔ bridge mirror freshness, :42-43).
- **Triggers**: push/PR on `contracts/**, engine/include/**, ui/src/connector/**, ui/src-tauri/src/**, ui/scripts/gen-filefilter.mjs, .github/workflows/contracts-ci.yml` (:8-24).
- **LIVE (38 runs total)**: `42197913793` @ `4b2978497` success (11-06 09:49) · `42194741122` @ `a5e6ae7a2` success · `42187695975` @ `939dd483a` success. Not run since v0.8.4 — post-tag commits touched none of its paths (§1 note).
- **Verdict**: green, current through v0.8.4; phase-2 skip comment stale but the skip itself no longer applies.

### 2.2 engine-ci (.github/workflows/engine-ci.yml, 505 ln)
- **What**: 2 jobs, both windows-2022 (pinned, :38-40). `lint` (:76-249): clang-format 18.1.8 pinned (:99-113), N-041 dead-surface + direct-access + session-96 driver-era lints (:116-142), PowerShell AST parse gate over every CI-consumed script (:144-167), ROOT workflow YAML parse gate + inline pwsh run-block AST pass with `${{ }}` substitution (:169-239), doc-truth gate M-088 (:241-249). `build-and-test` (:251-505): vcpkg binary cache, `cmake --preset win-x64-release` MSVC /W4 /WX (:281-293), full ctest incl. cr_ui ABI (:295-300), Win7 posture gate stamp+import-allowlist on 3 PEs (:304-319), G-09 report-schema (:324-334), **elevation refusal-contract probes** — unelevated LUA-token probe must get exit 4 ERR_ELEVATION_REQUIRED, elevated `bench --mode info` exit 0 (:343-382), CLI junk-filter contract M-071/A-013/M-079 (:388-494), failure diagnostics upload (:496-505).
- **Triggers**: push/PR `engine/**` + own yml, negation-globs exclude `*.md`, `engine/docs/**`, `engine/worklog*` (:42-59); `workflow_call` (:60) — **stale comment**: release.yml does NOT call it (grep `uses: ./.github` over workflows = empty; release runs its own `build.ps1 -Target all` with ctest inside, release.yml:62-67 + build.ps1:72-81); `workflow_dispatch`.
- **LIVE (22 runs)**: `42123593814` @ `a5cfa11f4` success (11-05 22:41) · `42076322446` @ `bb69f3d53` success · `42075907522` @ `0629555f7` **failure** (the engine-ci comment-syntax red fixed by `bb69f3d5`).
- **Not run since `a5cfa11f`** — justified: `git log a5cfa11f..HEAD -- engine/ ':!engine/worklog' ':!*.md'` is EMPTY (engine tree unchanged since P5). Caveat: its YAML parse gate also covers root `.github/workflows/*.yml`, and forensic-repro.yml was edited 5× after a5cfa11f (`44d166e6`..`0d09bca7`) — those edits were parse-gated only by the successful dispatch, not by this lane.
- **Verdict**: green and still covering HEAD's engine tree (no engine change since the last run).

### 2.3 forensic-repro (.github/workflows/forensic-repro.yml, 309 ln)
- **What**: `workflow_dispatch` ONLY (deliberately — :31-39), input `release_tag` default v0.8.4 (:41-47). windows-2022, 90 min. Downloads the shipped release setup.exe + SHA256SUMS verification (:72-88), machine facts pre/post (:90-104), seeds 64GB NTFS + 8GB exFAT vdisks with real signatures/deletions (seed-disks.ps1, :94-96), S1 install with full process/window capture (:98-100), then ONE combined step (job-object lifetime lesson, run 40464495050 — :106-140): launch app, io-probe 5400 s, `node drive-ui.mjs` drives the REAL shipped UI over WebView2 CDP (health/scan/every surface); unelevated twin via restricted token + CDP 9223 asserting the elevation banner + scan-start refusal (:142-188); app.log + sessions snapshot `if: always()` (:190-209); silent uninstall with no-leftover assertions, incl. the documented `_?=` self-survivor and the "caller removes emptied dir" rules (:215-295); evidence upload `if: always()` (:297-309). No masking: every failure fails the run (:38-39).
- **LIVE (29 runs)**: `42226613660` @ `0d09bca79` success (11-06 13:34) — **FORENSIC #29, the first complete green run** (worklog/RELEASE-VALIDATION/RELEASE-RUN.md:386-427) · `42220997177` @ `2f89f833f` failure (empty-dir survivor) · `42215611622` @ `f48f0396e` failure (NSIS `_?=` self-survivor).
- **Verdict**: green at last dispatch, but historically **flaky**: 29 dispatches for one complete green; the two most recent reds were probe-teardown defects (fixed in-tree, post-mortem'd in commits `2f89f833`, `0d09bca7`). Both reds are AFTER v0.8.4 was cut — the shipped installer itself is unchanged since the tag.

### 2.4 release (.github/workflows/release.yml, 213 ln)
- **What**: `push: tags v*` (:21-24), windows-2022, 90 min, `contents: write`. Version gate: tag MUST equal VERSION file (:36-45); contracts gate (:47-48); `build.ps1 -Target all` — contracts→engine (with ctest, build.ps1:72-81)→ui+tauri→NSIS installer→portable zip→checksums (:62-67); artifact presence + dev-signature verification on installer and `cr_ui.dll` (:69-90); truthful release notes incl. "development self-signed, not production-signed", elevation expectations (:92-188); upload setup.exe/portable.zip/SHA256SUMS with `fail_on_unmatched_files` (:190-198); build-log artifact `if: always()` excluding signing material (NG-17, :200-213).
- **LIVE (12 runs)**: `42199111948` @ tag v0.8.4 success (11-06 09:58) · `42188868989` @ v0.8.3 success · `42182876659` @ v0.8.2 success. All 5 v0.8.x releases green.
- **Verdict**: green. Note it validates the product it builds but its tests ride inside build.ps1, not the engine-ci lane (see 2.2 stale-comment note).

### 2.5 ui-ci (.github/workflows/ui-ci.yml, 116 ln)
- **What**: windows-latest, working-dir ui/. pnpm 12.15.0 + node 24; typecheck, lint, format:check, arch-lint A1-A7, ast-grep rules (raw invoke confined to connector; connector factory imports confined — :58-70), filefilter mirror freshness (:72-73), knip dead-code (:75-82), jscpd (:84-85), vitest coverage thresholds 70/60/70/70 (:87-88), production build + bundle budget 200/500 KB (:90-97), `cargo check --tests` + clippy -D warnings on the bridge (:99-109), web-dist artifact.
- **Triggers**: push/PR `ui/**, contracts/**` + own yml (:9-19).
- **LIVE (60 runs)**: `42197913795` @ `4b2978497` success · `42194741077` @ `a5e6ae7a2` success · `42187695792` @ `939dd483a` success.
- **Verdict**: green through v0.8.4.

### 2.6 ui-e2e (.github/workflows/ui-e2e.yml, 73 ln)
- **What**: windows-latest; Playwright 12-spec suite against `vite preview` with the build **pinned to `VITE_CONNECTOR=mock`** (:47-51) — "the e2e specs are mock-pinned by design in this state; the real connector needs the Tauri shell + engine kit, which is the release lane's business" (:3-8). Screenshots `if: always()`, traces `if: failure()`.
- **LIVE (45 runs)**: `42197913860` @ `4b2978497` success · `42194741132` @ `a5e6ae7a2` success · `42187695816` @ `939dd483a` success.
- **Verdict**: green, but **green-by-design-on-mock**: it never exercises the real connector/engine path. Documented, not silent — still, any real-connector UI breakage is invisible to this lane (the forensic probe is the only installed-UI check).

### 2.7 validation-evidence (.github/workflows/validation-evidence.yml, 210 ln)
- **What**: `workflow_dispatch` ONLY (numbers are runner-dependent; artifacts curated by hand, :17-25). Two jobs. `t14-measurements` (win-2022, 60 min): engine kit via `build.ps1 -Target engine -SkipTests`, elevation facts recorded and REQUIRED (:50-71), MSVC harness compile+run vs documented latency budgets (:73-96), bridge-side ignored tests serialized for G-7 filter cost + memory bounds (:98-124), runner-context JSON, evidence upload `if: always()`. `t15-installer-matrix` (win-2022, 100 min): full product build `-SkipTests`, then installer-matrix.ps1 install/update/repair/uninstall rows — note the `$?`-not-`$LASTEXITCODE` null-comparison fix (:180-193), installer + evidence uploads (:195-210).
- **LIVE (25 runs)**: `42143983955` @ `8edb721c0` success (11-06 01:27) · `40474514493` @ `b93e6be8a` success (10-27) · `40442056802` @ `9e1881541` success (10-27).
- **Verdict**: green at 8edb721c (pre-v0.8.x); not re-run against v0.8.4 (dispatch-only lane).

---

## 3. Directory map (what lives where)

| path | contents |
|---|---|
| `build.ps1` / `build.sh` | the single Windows product build / Linux-subset build (§4) |
| `VERSION` | `0.8.4` — product version source of truth |
| `contracts/` | boundary truth: `command-surface.json` (45 commands × serves/abiRefs/returns + events + removed), `error-codes.json` (40 engine→uiCode mappings + bridgeOnlyCodes + invariants), `event-mapping.md`, `type-maps.md`, `validate.mjs` (the CI gate), README |
| `engine/src/app/` | `rrecov.cpp` — the CLI executable (scan/tree/recover/bench/list/list-carve-formats verbs; the CI probes drive it directly) |
| `engine/src/win/` | Windows-only platform layer: `cr_enumerate.cpp` (disk/volume enumeration), `cr_health.cpp` (**SMART/health queries; the `_popen` fsutil call at :306**), `cr_transport.cpp`, `cr_elevate.cpp`, `cr_bitlocker.cpp`, `cr_destio.cpp`, `cr_fsck.cpp`, `cr_bootfix.cpp`, `cr_image_win.cpp`/`cr_image_create.cpp`, `cr_mountpoints.cpp` |
| `engine/src/core/` | the portable recovery core, one subdir per module: `health/` (SMART ATA/NVMe/CSMI/grading/fve-policy), `orchestrator/` (scan, vollabel, folder filters), `query/`, `tree/` (diskmap/node/snapshot/grading), `ntfs/ fat/ exfat/ ext4/ btrfs/ zfs/ refs/ hfsplus/` (per-FS engines), `carve/` (signature scan — **the BF-22 CPU wall**), `preview/` (thumbnails), `storage/` (image/slice/cached/retry/decrypt), `session/` (crsf records/lastscan), `repair/` (rawfixer), `reghive/` (offline registry surgery), `recovery/` (report/safewriter/dict), `table/` (LDM/GPT), `crypto/`, `compress/`, `check/`, `apply/`, `lost/`, `bcdfix/`, `nvramfix/`, `err/`, `cr_parallel.cpp` |
| `engine/src/ui/` | `cr_ui.cpp` + `cr_ui_exports.def` — the C-ABI shim (cr_ui.h, ABI v4) the Rust bridge statically imports |
| `engine/include/cr/` | public headers mirroring core modules + `win/` (cr_elevate.h, cr_enumerate.h, cr_health.h, …) + the contract headers `cr_ui.h`/`cr_error.h`/`cr_types.h` that validate.mjs parses |
| `engine/tests/` | GoogleTest suites (60+ unit test files incl. test_enumerate_win, test_health_*, test_scan_truth, test_transport_win), `win-stub/` headers for Linux builds, data fixtures (thumbnail corpus, qemu disk images, junk-gate-fat16.raw) |
| `engine/{worklog,diff-to-fix,bench,docs,tools,cmake}/` | the merged core-repo research corpus: ~150 session prompts/evidence sheets, decompiled-reference quotes, benchmarks, audits, PE-stamp tools |
| `ui/src/` | React 19 app: `app/`, `components/`, `features/`, `hooks/`, `i18n/`, `lib/`, `queries/` (react-query hooks per surface), `stores/`, `styles/`, `theme/`, `workers/`, `assets/` |
| `ui/src/connector/` | the UI↔shell boundary: `contract.ts` (error/event vocabulary), `mock/` (the TEST DOUBLE, demoted in P4 `8e61f4a8`), `tauri/` (the real connector) |
| `ui/src-tauri/src/` | Rust shell: `main.rs`/`lib.rs`, `bridge/` (abi.rs ABI-v4 bindings, events, error map, consent, state, filefilter mirror, logging), `commands/` (12 files: devices, health, scan, recovery, repair, results, preview, lost_partition, sessions, system, mod) — `commands/system.rs:36-83` is the TokenElevation probe + `system_relaunch_elevated` (ShellExecuteW "runas") |
| `ui/src-tauri/{tauri.conf.json,Cargo.toml,build.rs,capabilities/}` | shell build config (§4) |
| `installer/` | `recoverywizard.nsi` (the combined wrapper installer), `devsign.ps1` (ephemeral self-sign cert), `portable.ps1`, README |
| `tools/forensic/` | the 8 probe scripts: collect-facts.ps1 (facts JSON incl. the app-manifest probe :29-32), seed-disks.ps1, install-probe.ps1, launch-app.ps1, drive-ui.mjs (CDP driver), io-probe.ps1, unelevated-launch.ps1, unelevated-probe.mjs |
| `tools/validation/` | `installer-matrix.ps1` (T15), `measure-cr-ui.c` (T14 harness) |
| `docs/` | product docs: ARCHITECTURE, BUILD, DEV-SIGNING, FORENSIC-PROBE (installed-product probe ledger), INSTALLER(-DECISION), INSTALL-FOOTPRINT-(CURRENT/DELTA), INTEGRATION-(PLAN/GAPS/VALIDATION), MSIX-ASSESSMENT, PARITY-DELTAS, PRODUCT-WIRING, REFERENCE-*, RELEASE, TESTING |
| `worklog/` | session sheets: `RELEASE-VALIDATION/` (RELEASE-RUN.md = the run ledger; BUILDS/CORE-MERGE/UI-WIRING/CAPABILITY-MAP/DRIVER-REMOVAL), `BEHAVIOR-FINDINGS.md` (BF-22..BF-26), `REPRODUCTION/` (**empty — T1 phase 1 pending**), `context-sheets/`, session prompts |
| `.github/workflows/` | the 7 lanes (§2) |

---

## 4. Config inventory that determines product behavior

- **VERSION** = `0.8.4` (file, 1 line). Lockstep gate: build.ps1:37-46 asserts ui/package.json, ui/src-tauri/Cargo.toml, tauri.conf.json all equal it (engine CMake's `0.6.1` is the core component version, deliberately not restamped — build.ps1:12-18). Release tag gate: release.yml:36-45.
- **build.ps1** (252 ln): lanes `contracts|engine|ui|installer|portable|all` (:21,:236-251). Engine lane: vcpkg preset build + ctest with `-C Release` (the rc1 lesson, :72-81), Win7 PE stamp+verify on rrecov.exe/cr_ui.dll (:84-97), engine kit staging beside-the-exe with gtest/gmock excluded (:99-121), dev-sign of kit (:123-131). UI lane stages the kit into target/{release,debug} + `CR_UI_LIB_DIR` then `pnpm tauri build` (:134-172). Installer lane: makensis with native backslash defines (:186-213) + dev-sign of setup.exe. No manifest/mt.exe step anywhere (see NSIS below).
- **build.sh** (53 ln): Linux subset — contracts validate, engine linux-release + ctest, ui typecheck/lint/test. No installer/tauri lanes (Windows-only).
- **engine/CMakeLists.txt** (180 ln): project core_recovery 0.6.1, C++20/C11, MSVC `/W4 /WX /permissive-` + Win7 envelope `_WIN32_WINNT=0x0601` (:19-30); zlib REQUIRED fatal (:48-53); lz4+zstd REQUIRED fatal — "no silent codec-less builds" (:138-147); vcpkg runtime DLL POST_BUILD deploy backstop `cr_deploy_vcpkg_binaries` (:156-167); GTest 1.15.2 FetchContent fallback (:170-180).
- **engine/CMakePresets.json**: `win-x64-release` = VS2022 x64 + `$env{VCPKG_ROOT}` toolchain, `VCPKG_APPLOCAL_DEPS=OFF` (:44-53); also win-x64-debug, linux-gcc (ASAN), linux-release, linux-ubsan. These presets are what engine-ci:289-291 and build.ps1:66-71 drive.
- **ui/src-tauri/tauri.conf.json** (54 ln): Tauri 2; productName "Recovery Wizard", version 0.8.4, id com.heicfast.recoverywizard; frameless maximized 1240×840 window (:14-27); `csp: null` (:29); bundle targets `["nsis"]` (:34), nsis installMode "both", EN+zh-CN, WebView2 downloadBootstrapper (:43-52). **No manifest/UAC config key exists in this file.**
- **ui/src-tauri/Cargo.toml** (41 ln): recovery-wizard-ui 0.8.4; tauri 2, dialog+opener plugins; `windows-sys 0.59` features Win32_Foundation/Storage_FileSystem/Security/System_Threading/UI_Shell/UI_WindowsAndMessaging — for the TokenElevation probe and the relaunch verb (:21-34); release profile `opt-level="s"`, `panic="abort"`, lto. **No `embed-manifest`/`tauri-build` manifest feature; build.rs only wires cr_ui.lib linking then `tauri_build::build()`** — which embeds Tauri's DEFAULT manifest (asInvoker, no requireAdministrator).
- **ui/src-tauri/capabilities/default.json**: minimal — core:default, dialog:default, opener:default, 5 window-chrome permissions (:6-15). No fs/shell-wide permissions.
- **ui/package.json** scripts (:11-31): dev/build/preview, typecheck, lint, format(:check), test(:coverage), e2e, tauri, arch:lint, knip, jscpd, bundle:check, visual:compare, filefilter:check. `dev` pins `VITE_CONNECTOR=mock` (:12). Deps: React 19.3, react-router 7, TanStack query/virtual, zustand, i18next, sonner, Radix.
- **installer/recoverywizard.nsi** (531 ln): wrapper installer. `RequestExecutionLevel admin` (:151) — the INSTALLER elevates (one UAC). Drives the inner Tauri NSIS bundle silent `/S /allusers /D=$INSTDIR` (:210) after seeding the inner install-location key (:208); adopts the inner's location if it lands elsewhere (:217-231); engine kit DLLs BESIDE app exe for loader resolution (:233-262); ProgramData sessions/logs (:129-131, :265-271); two-level SW key + owned uninstall key with read-backs (:273-301); AbortWithReason + InstallFact dual logging (:78-111); uninstaller: close-app prompt, sessions kept by default with explicit consent only (:446-469), inner uninstaller silent, kit DLL catch-all delete, registry cleanup, no leftovers (:423-531).
  - **THE CLAIM AT :27-29** — "Elevation: RequestExecutionLevel admin — ONE UAC prompt at installer launch … and the app itself runs elevated (**requireAdministrator**)."
  - **VERIFICATION — the parenthetical is FALSE in the tree**: (a) `Glob **/*.manifest` over the repo = **zero** .manifest files; (b) `requestedExecutionLevel|requireAdministrator|asInvoker|mt.exe` appear ONLY in docs (.md), the .nsi comment itself, and the probe `tools/forensic/collect-facts.ps1:29-32` — never in any build script, CMake, build.rs, Cargo.toml, or tauri.conf.json; (c) no `mt.exe` invocation in build.ps1/build.sh/installer//tools//engine/tools/ (grep empty); (d) build.rs ends with plain `tauri_build::build()` (no `WindowsAttributes::app_manifest` override), so recovery-wizard-ui.exe ships with Tauri's default asInvoker-level manifest; (e) the app's actual elevation posture is: launch unelevated → in-app banner + `system_relaunch_elevated` (ShellExecuteW "runas") — ui/src-tauri/src/commands/system.rs:81-83 ("nothing elevates silently"), which contradicts the session mandate "must request elevation by default at launch" (worklog/session-debug-and-release-prompt.md §1). This is the #1 user-reported defect, root-caused by the parallel T1 spot-check (commit `c8aef05a`) and consistent with collect-facts.ps1:32's probe (the run-#29 facts artifact records the truth; inference: it read "as-invoker-or-no-match" since no manifest is embedded).
- **contracts/command-surface.json** (737 ln): 45 commands, each with `name/threading/abiRefs (cr_ui_* symbols)/serves (UI methods)/returns/notes`, plus `events` and `removed` (the driver-era surface deleted in P3c `e432a8c9`). Examples: `devices_list` → cr_ui_enumerate_disks/volumes/open_disk/…; `system_relaunch_elevated` is the shell-owned verb.
- **contracts/error-codes.json** (409 ln): 40 `mappings` rows `{code,value(0x5…),uiCode,context refinement rules,surface}` binding every `cr::Err` value (cr_error.h) to a ConnectorErrorCode, plus `bridgeOnlyCodes`, `uiCodesWithNoEngineSource`, `invariants`.
- **Who validates them**: `contracts/validate.mjs` phase 1 (header↔map↔vocabulary completeness) + phase 2 (manifest ↔ `#[tauri::command]` fns in ui/src-tauri/src/commands/ ↔ connector invocations) — run by contracts-ci (every relevant push/PR), build.ps1's contracts lane (build.ps1:48-54), and release.yml:47-48 before any product build.

---

## 5. Baseline CI health verdict

**Is main green at HEAD?** No run exists on HEAD `108578a6` itself — the last 8 commits after v0.8.4 touch only `tools/forensic/**`, `.github/workflows/forensic-repro.yml`, and `worklog*`, which no push lane watches (verified per-path via `git log --name-only 4b297849..HEAD`). **Every lane's latest conclusion is success**, and the gaps are path-filter-justified:

| workflow | latest run | head sha | conclusion | coverage caveat |
|---|---|---|---|---|
| contracts-ci | 42197913793 | `4b2978497` (v0.8.4) | success | none material (no contract/engine-header/connector change since) |
| engine-ci | 42123593814 | `a5cfa11f4` (P5) | success | engine tree unchanged since (git log empty); forensic-repro.yml edits after P5 not re-gated by its YAML gate |
| forensic-repro | 42226613660 | `0d09bca79` | success (#29, first complete green) | dispatch-only; 2 of previous 3 red (probe teardown bugs, fixed) |
| release | 42199111948 | tag v0.8.4 | success | all 5 v0.8.x green |
| ui-ci | 42197913795 | `4b2978497` | success | none material |
| ui-e2e | 42197913860 | `4b2978497` | success | mock-pinned by design — real connector untested here |
| validation-evidence | 42143983955 | `8edb721c0` | success | dispatch-only, last run pre-v0.8.x |

**Verdict: main is CI-green (green-by-path-filter at HEAD, fully green at v0.8.4), yet the product is functionally broken per the user** — the lanes that are green do not cover the failing surfaces: elevation-at-launch (no lane asserts an app manifest; only the unelevated-twin REFUSAL is asserted), console popups (engine-ci's probes run headless where `_popen` windows are invisible; the S1 window capture covers INSTALL time only), enumeration completeness on real multi-disk hardware (probe seeds vdisks — #29 showed 4 selectable drives), and the 75% stall (recorded as BF-22 at 76.1%, "environmental/known-perf", deliberately NOT gated — worklog/BEHAVIOR-FINDINGS.md:36). CI greenness is therefore necessary but not sufficient — exactly the "green-for-the-wrong-reason" pattern this sheet exists to flag.

---

## 6. Facts that matter most for the debugging session (pointers, not diagnosis)

1. **No elevation manifest anywhere** — installer/recoverywizard.nsi:27-29 claims requireAdministrator for the app; zero .manifest files, zero mt.exe, default Tauri manifest (§4). App relies on banner + runas relaunch (commands/system.rs:81-83).
2. **`_popen("fsutil behavior query disabledeletenotify 2>nul")` at engine/src/win/cr_health.cpp:306** — a visible console spawn on the health path (matches "console popups on disk health"; T1 commit `c8aef05a` names it).
3. **BF-22**: deep scan of large volumes freezes at **76.1%** with found-count frozen (worklog/BEHAVIOR-FINDINGS.md:36) — identical pre/post the ABI v4 merge and on v0.7.0 (runs 42072191860, 42129256087); reads idle from +6 min; the carve matcher grinds in memory (BF-25, :39) with no cancel checkpoint. The user's "75% stuck, files found (0 B)" is plausibly this (inference).
4. **Forensic #29 (42226613660) is green on v0.8.4**: 4 drives selectable, stop→partial→recover works, unelevated twin shows banner+refusal, uninstall clean (RELEASE-RUN.md:386-427). The user's "only 1 of 3 disks" was NOT reproduced on the seeded runner (runner disks differ from user hardware — enumeration axes named by `c8aef05a` as an open question).
5. **`worklog/REPRODUCTION/` is empty** — T1 phase 1 (reproduce with evidence) had not started when this sheet was cut; the four spot-check root causes in `c8aef05a` are working hypotheses with file-level evidence, not yet gated reproductions.
