# REPO-STATE — the complete CURRENT-STATE sheet of final-prod @ 8f3bedd1 (the unreleased 0.9.2 train)

> Agent A (Phase 0 refresh, the post-v0.9.2 debug session). READ-ONLY survey;
> every claim cites a commit SHA or `path:line` in the tree at HEAD
> `8f3bedd1a851da520c42622a7657cf03841a63f0` (branch `main`, verified
> `git rev-parse HEAD`; clean working tree; up to date with `origin/main`;
> 2400 commits total). `VERSION:1` = `0.9.2` — **but no `v0.9.2` tag exists
> (local or remote); the newest tag is `v0.9.1` at `cdf66ff8`** (see §2, the
> single most load-bearing finding of this sheet). Research corpus (not
> surveyed here; Agents E/F own it): `/home/z/my-project/work/easeus-drw-research`
> @ `be0cfe9` per worklog.md:2706. Live CI badge status was NOT checkable from
> this sandbox (no network); last recorded evidence is cited per lane.
> Display caveat for downstream readers: this console strips the literal
> two-character sequence bracket-m from captured output — `cat`/`rg` show
> `branches: ain]` where the file verifiably contains `branches: [main]`
> (verified `od -c` + PyYAML parse; the v0.8.4-era sheet recorded the same
> artifact). Do not "fix" workflow branch filters on visual evidence alone.

---

## 1. Commit-graph summary — the recent defect trains

### 1.1 The 0.9.x debug-and-release session (2026-11-06, two lanes)

Two concurrent sessions (lane A "heicfast-release" = shell/product surface;
lane B "Session Agent" = engine seam; the contract is
`worklog/COORDINATION.md`) took the tree from v0.8.4 to the 0.9.2 train in
one day. Chronological order (oldest first):

| Commit | Claim |
|---|---|
| `c8aef05a` | (T1 lane A) session opens; four spot-check root causes named |
| `108578a6` | (T1 lane B) coordination note; D4 decomposed (a-d) |
| `a99869e4`/`58c8f5b9` | (T2/T3 both lanes) the Phase-0 context sheets + DIAGNOSIS.md |
| `be28db44` | the two-lane seam posted |
| `76bdf056` | **D-PRIV-01 FIXED**: requireAdministrator manifest via tauri-build `app_manifest` (+5 doc-truth sites) |
| `c6ff7c10` | **D-ENUM-01 UI + D-UI-01 FIXED**: Home renders Internal+External groups (the `kind==='internal'`-only filter removed); elevation surfaces re-themed |
| `21f33f32` | **W-02+U-01+U-02+U-03 FIXED**: close-requested save guard (ExitGuard); real `connectorMode`; live Open-location; content-based user-signs import |
| `68460a4c` | **D1+D2 FIXED**: privilege model + console shells (`_popen` → hidden direct fsutil via CreatePipe+CREATE_NO_WINDOW) |
| `34e7332e` | **W-01 FIXED**: six dead-letter bridge events get listeners end to end (contract+tauri+mock+queries+UI) |
| `22bae5be`,`6565c112` | mechanical clang-format repairs after lane B's pushes |
| `8c7120ae` | (T4) all four reported defects REPRODUCED on shipped v0.8.4 (forensic run 42255492260) → `worklog/REPRODUCTION/` |
| `d8fbef64` | forensic probe re-vehicled: gates now assert the FIXED posture (manifest, OS-refusal twin, home census, D-SCAN-01 progress) |
| `ce71143e` | **D-SCAN-01/D4a+b+c FIXED**: live sector counter, live found-size (bytes_found ABI append), bounded cancel — engine+ABI+bridge, 12 files |
| `571d588a`,`65e50e5f`,`22ea9564`,`1a21a8a9` | docs trued (DOC-AUDIT), PARITY-DELTAS, BF register update, 0.9.0 notes |
| `3d03eec2` | T14 persistence: results view mode + preview pane width persist |
| `0eeff0ec` | **D3 engine FIXED**: never drop a PRESENT device (ABSENT vs PRESENT-but-unopenable discrimination, `engine/src/win/cr_enumerate.cpp` +83) |
| `6c9c36d9` | **BF-28 FIXED**: unelevated health ACCESS_DENIED carries ERR_ELEVATION_REQUIRED |
| `b8553e71` | BF-32 recorded (emitter teardown race), deliberately held out of the train |
| `ba48a7b0` | test determinism fix (Windows red 42266802645); product code unchanged |
| `4aa33d62` | Win7 import allowlist: CreatePipe+SetHandleInformation (the D2 vehicle's imports) |
| `8fe16ab8` | **v0.9.0 cut** (tag exists) — "every reported defect fixed at the root" |
| `7623a36a`,`710bd35c` | release/forensic evidence: v0.9.0 run 42274682840 16/16 green; forensic 42278088695 **RED at a NEW freeze → D4e** (655 s deep-remnant-sweep) |
| `62fb7670` | **D4e + E-01 FIXED**: 4 MiB block-level remnant-sweep ticks; 64-wchar GetLogicalDriveStringsW → exact-size retry (16+ letters) |
| `e69e39c5`→`cdf66ff8` | **v0.9.1 cut** (first attempt failed the version gate — ui/package.json missing from lockstep; tag re-cut at `cdf66ff8`) |
| `0e87689d` | **D4f FIXED**: the 605 s zero-region freeze — user-sign stride probe made lock-free (snapshot-once + precomputed disable map + zero-span fast skip), caught by the v0.9.1 forensic run 42291160236 |
| `3fe8403c`,`87034a7d`,`12d824a3` | mechanical repairs (test window widen, prettier ×2) |
| `1ef7aa7c` | **"chore(release): v0.9.2"** — version lockstep bumped (VERSION, package.json, Cargo.toml/lock, tauri.conf.json) + release-notes template updated. **The v0.9.2 tag was never pushed** (§2) |
| `eb4bb5f3` (Nov 9) | **D-ENUM-02 + D-NVMe-01 FIXED + BF-03 closure**: the bus-type table was SHIFTED BY ONE (bus 11 is SATA, not SD → every SATA disk shipped `removable=true`, UI sorted internal SATA drives into the external group); `read_nvme_health`'s pack(1) buffer placed STORAGE_PROTOCOL_SPECIFIC_DATA at offset 9 instead of 8 → **every real NVMe controller refused → NVMe-only drives surfaced no health at all**; RemovableMedia byte honored; `bus_type` ABI axis appended; `cr_ui_health_read_ex2` + `cr_ui_smart_attr.threshold` (8 files, +360/−44) |
| `0c80036f` | clang-format over eb4bb5f3's two files — **main was RED at this session's open** (engine-ci #34) |
| `8f3bedd1` (HEAD, Nov 11) | (T1) this session re-opens: "the user's machine still says NO" |

### 1.2 Pre-0.9 history in one paragraph each

- **v0.8.0–v0.8.4 (2026-11-05/06, ~17 h)**: one session of forensic-driven
  UI/UX truth fixes — visible refused save (`db600f4c`), backup-list truth
  (`73419713`), stopped-scan banner (`f76c9941`), the invisible-resume route
  (`1cd9bc64`), whole-disk session re-attach (`559080ae`) — all discovered by
  the forensic-repro lane on real runners. Engine/ untouched in this window.
- **v0.7.0 (`68ab3fcd`, 2026-11-05)**: the consolidation release — kernel
  driver REMOVED, engine re-merged to core_recovery@12f3edf7 (ABI v4), UI
  re-based to recovery-wizard-ui-new@dd2782c, the bridge (P4) wired the UI to
  the real engine. ~1123 commits beneath it.
- **v0.5.0/v0.6.0-rc1-rc2 (2026-10-26/27)**: the integration + mandatory-
  driver-era releases (driver later removed).

### 1.3 Last-commit-per-directory (drift map)

- `engine/` → `0c80036f` (the D-ENUM-02 format repair). **engine/ changed
  heavily in the 0.9.x train** (D2/D3/D4a-f/E-01/D-ENUM-02/D-NVMe-01) — the
  v0.8.4-era "engine untouched" observation is long obsolete.
- `ui/` → `12d824a3` (prettier over tauri.conf.json).
- `contracts/` → `ce71143e`-era (event-mapping.md updated in the D4 train).
- `installer/` → `571d588a` (docs truth only; the .nsi itself predates).
- `.github/workflows/` → `1ef7aa7c` (release.yml notes for 0.9.2).
- `tools/`, `docs/`, `worklog/` → the 0.9.0/0.9.1 evidence train.

## 2. THE CRITICAL FINDING — v0.9.2 was never released

Verified facts:

1. `git tag -l "v0.9*"` → `v0.9.0`, `v0.9.1` only. `git ls-remote --tags
   origin` (27 tags) → newest is `v0.9.1` → `cdf66ff8`. **No `v0.9.2` tag
   exists locally or on the remote.**
2. The release workflow (`.github/workflows/release.yml:21-24`) triggers ONLY
   on `push: tags: v*` — without the tag push, no v0.9.2 release object,
   build, or artifact ever existed.
3. `worklog/RELEASE-VALIDATION/RELEASE-RUN.md` ends at the v0.9.0 section
   (+ the "v0.9.1 follows" note at its tail). **There is no v0.9.1 release-
   workflow section and no v0.9.2 section at all.** The v0.9.1 forensic run
   (42291160236, RED at D4f) is recorded only in `worklog.md:2684-2701`.
4. Yet the session-opening entry (`worklog.md:2703`, commit `8f3bedd1`)
   says "session re-opens post-v0.9.2", and `1ef7aa7c`'s release-notes
   template (release.yml:148-157) already documents "(0.9.2) Zero-heavy
   volumes no longer freeze the scan."

Consequences for this session (facts, not speculation):

- Whatever build the user has installed, it **cannot contain** the three
  post-v0.9.1 fixes: `0e87689d` (D4f zero-region freeze), `eb4bb5f3`
  (D-ENUM-02 SATA-mislabeled-removable + D-NVMe-01 dead NVMe health query +
  BF-03 threshold column), `0c80036f`.
- The user's three headline symptoms — scan stuck in sector scanning, SMART
  only-first-disk, not all disks visible — map exactly onto the defect
  classes those UNRELEASED commits claim to fix (D4f freeze; D-NVMe-01
  "switch to another disk and it shows nothing"; D-ENUM-02 "the disks page
  looked incomplete"). Whether the user runs v0.9.1 or older, and whether
  the eb4bb5f3 fixes actually work on real hardware, are the first things
  to establish (§7).

## 3. Workflows — the 7 CI/validation lanes (all files read in full)

| Lane | Trigger / paths | Runner | What it runs | Goes RED when | Skip / caveat |
|---|---|---|---|---|---|
| **contracts-ci.yml** (44 ln) | push(main)+PR on `contracts/**`, `engine/include/**`, `ui/src/connector/**`, `ui/src-tauri/src/**`, `gen-filefilter.mjs` | ubuntu-24.04, node 24 | `node contracts/validate.mjs` (error-code map ↔ `cr_error.h`; event map ↔ `CR_EVT_*` 1..10; command manifest ↔ Rust `#[tauri::command]` AND TS invokes) + `ui/scripts/gen-filefilter.mjs --check` | any boundary drift, unmapped code, manifest hole | Proves boundary truth only, nothing runtime. **Re-run locally this session: green** ("43 commands, 10 events"). |
| **engine-ci.yml** (506 ln) | push(main)+PR on `engine/**` (negates `*.md`, `engine/docs/**`, `engine/worklog*`) + `workflow_call` + `workflow_dispatch` | windows-2022 (pinned) | Job `lint` (15 min): clang-format **18.1.8 pinned** over src/include/tests; N-041 dead-surface + direct-access + driver-era lints; vcpkg.json presence; PowerShell AST parse over every CI-consumed script; ROOT workflow YAML parse + inline-pwsh run-block AST; doc-truth gate (`engine/tools/doc_truth_gate.py`). Job `build-and-test` (needs lint, 90 min): vcpkg binary cache; `cmake --preset win-x64-release` MSVC /W4 /WX; full ctest (GoogleTest + cr_ui ABI check, 1752+ cases at 0.9.0); Win7 posture gate (stamp_osversion_um.py + verify_pe_um.py vs `ref_imports_um.json`) on rrecov.exe/cr_tests.exe/cr_ui.dll; G-09 report schema; **elevation refusal probes** (unelevated `rrecov scan` must exit 4 = ERR_ELEVATION_REQUIRED via `engine/.github/scripts/Probe-Unelevated.ps1`; elevated `bench --disk 0 --mode info` exit 0); M-071 CLI junk-filter contract (tree+session round-trip+A-013 resume+M-079 auto-save naming) | format drift (the repeated killer), lint hit, build/test failure, PE import outside allowlist, missing report sample, wrong exit taxonomy, junk leak | docs-only pushes ignored; concurrency event-scoped cancel-in-progress. **Stale comment** at :60 claims "release.yml calls this lane as its validation gate" — release.yml contains no `workflow_call` of it (it runs build.ps1 directly). Proves the ENGINE; **says nothing about the Tauri exe's surfaces**. |
| **forensic-repro.yml** (317 ln) | **workflow_dispatch ONLY**, input `release_tag` (default `'v0.8.4'` — stale; latest release is v0.9.1) | windows-2022, 90 min | Downloads the SHIPPED release assets (checksum-verified), machine facts, seeds 64 GB NTFS + 8 GB exFAT vdisks, installs silently with full process/window capture (S1), **manifest gate** (installed exe must read requireAdministrator, D-PRIV-01), disk census, ONE-step CDP drive of the real UI via `tools/forensic/drive-ui.mjs` (S2 health/SMART DOM, S3 scan timeline up to 28 min with IO probe, S4 all surfaces; **home census gate**: every OS disk renders; **D-SCAN-01 gates**: no 180 s fully-frozen stretch, advanced phase advances ≥1.0 pt, live found-size non-"0 B"), unelevated twin (OS must REFUSE, win32 740), app-log snapshot, silent uninstall + no-leftover assertions, evidence upload `if: always()` | any gate failure, checksum mismatch, uninstall leftover, missing artifact | **No masking by design**; but SMART is inherently untestable on Azure VHDX (BF-23) — the health gate exercises the honest-failure path, not real SMART data. This is the ONLY lane that validates the installed product E2E. |
| **release.yml** (258 ln) | **push tags `v*`** | windows-2022, 90 min, contents:write | Version gate (tag == VERSION file); contracts gate; `./build.ps1 -Target all` (contracts → engine+ctest+PE gates+kit staging+devsign → ui+tauri build+**D1 manifest byte-scan** → NSIS installer → portable zip → SHA256SUMS); artifact presence + devsign verify; truthful release-notes composition; upload via softprops/action-gh-release; build-log artifacts (signing material excluded, NG-17) | version lockstep drift, any build/test failure, **missing requireAdministrator in the exe** (D1 gate), missing dev signature, missing artifact | none — every step is a hard gate. **Never ran for v0.9.2 (no tag — §2).** |
| **ui-ci.yml** (117 ln) | push(main)+PR on `ui/**`, `contracts/**` | **windows-latest (UNPINNED)** | pnpm 12.15.0/node 24 frozen install; typecheck; eslint; prettier --check; arch-lint A1-A7; ast-grep rules (`rules/no-raw-invoke.yml`, `rules/no-connector-import-outside-queries.yml`); filefilter mirror check; knip (dead code, KNIP_DISABLE_RAW_TRANSFER=1); jscpd (min-tokens 70); vitest --coverage (thresholds 70/60/70/70); vite build; bundle budget (MAIN 200 KB / TOTAL 500 KB); `cargo check --tests` (no link); clippy `-D warnings` | any of the above | The two prettier reds of the 0.9.x train (12d824a3, 87034a7d) landed HERE. No real-engine link (by design — release lane's business). |
| **ui-e2e.yml** (74 ln) | push(main)+PR on `ui/**` | **windows-latest (UNPINNED)** | Builds frontend with **`VITE_CONNECTOR: mock`**; playwright chromium; `pnpm e2e` vs vite preview (12 tests in 3 specs: `ui/e2e/smoke.spec.ts` 6, `responsive.spec.ts` 4, `consent.spec.ts` 2); screenshots + traces artifacts | any e2e failure | **Mock-pinned by design** — the e2e lane NEVER exercises the real bridge/engine. |
| **validation-evidence.yml** (211 ln) | **workflow_dispatch ONLY** | windows-2022 | T14: `build.ps1 -Target engine -SkipTests`, elevation facts recorded, `tools/validation/measure-cr-ui.c` harness (latency/throughput/G-7 filter cost/memory at 10⁵/10⁶), ignored T14 cargo tests serialized. T15: full product build + `tools/validation/installer-matrix.ps1` (install/update/repair/uninstall rows) | hard phase failure, failing matrix row, missing evidence artifact | No masking ("unavailable phases are RECORDED, never skipped"); artifacts curated by hand into `docs/INTEGRATION-VALIDATION/`. |

## 4. Directory map (top 3 levels, `.git` excluded; one-line purposes from actual contents)

```
final-prod/                       2400 commits; VERSION=0.9.2 (unreleased train)
├── .github/workflows/            the 7 CI lanes (§3)
├── contracts/                    engine↔UI boundary truth (validate.mjs gate, command-surface.json
│                                 43 commands/10 events/4 removed, error-codes.json, event-mapping.md,
│                                 type-maps.md) — CI-validated against BOTH implementations
├── docs/                         20 product docs (ARCHITECTURE, BUILD, DEV-SIGNING, FORENSIC-PROBE,
│                                 PARITY-DELTAS, RELEASE, INTEGRATION-VALIDATION/ w/ t14+t15 evidence)
├── engine/                       C++20 user-mode recovery engine (core_recovery lineage, ABI v4)
│   ├── src/app/                  rrecov.cpp — the CLI (scan/tree/recover/bench/list/list-carve-formats)
│   ├── src/core/                 the pure recovery core, ~30 subsystems: ntfs, fat, exfat, refs, btrfs,
│   │                             zfs, ext4, hfsplus, carve, orchestrator, tree, session, health, crypto,
│   │                             repair, reghive, preview, query, recovery, table, storage, compress,
│   │                             lost, bcdfix, nvramfix, apply, check
│   ├── src/ui/                   cr_ui.cpp + cr_ui_exports.def — the 92-export flat-C ABI shim (G-01)
│   ├── src/win/                  Windows platform layer: cr_enumerate, cr_health (SMART/NVMe),
│   │                             cr_transport, cr_bitlocker, cr_fsck, cr_elevate, cr_destio,
│   │                             cr_image*, cr_mountpoints, cr_bootfix
│   ├── include/cr/               92 public headers mirroring core+win incl. cr_ui.h (1,600+ ln) and
│   │                             win/cr_enumerate.h (the D-ENUM-02 battleground)
│   ├── tests/unit/               77 GoogleTest files, 1752+ cases (ABI check, telemetry, enumerate,
│   │                             nvme_health, no_console_spawn, scan_truth, …)
│   ├── tests/win-stub/           Windows SDK stub headers (byte-faithful since eb4bb5f3) for Linux lanes
│   ├── tests/data/               fixtures/images (junk-gate-fat16.raw etc.)
│   ├── scripts/                  46 dev/bench/parity scripts (gen_* code generators, sync-push, bench)
│   ├── tools/                    CI gate tools: doc_truth_gate.py, stamp_osversion_um.py,
│   │                             verify_pe_um.py + ref_imports_um.json (Win7 import allowlist),
│   │                             validate_report.py, gen_junk_gate_fixture.py, win_syntax_check.sh
│   ├── third_party/stb/          stb_image.h (preview/thumbnail decode)
│   ├── .github/scripts/          CI-consumed probes (Probe-Unelevated.ps1, benchmark, test-disk)
│   ├── .githooks/                pre-commit / pre-push
│   ├── bench/                    benchmarking lab (scenarios, og-vs-ours, vlm-evidence, root-causes.json)
│   ├── fuzz/                     fuzz harnesses (carve, btrfs, …) + corpus
│   ├── cmake/                    DeployVcpkgBinaries.cmake (runtime DLL deploy backstop)
│   ├── data/                     userdata-defaults.json
│   ├── diff-to-fix/              the engine's own diff-audit ledger (findings/FIXED-LEDGER/modules)
│   ├── docs/                     40+ engine docs (ABI-CONTRACT, CI-CURRENT, PARITY-*, PERF-LEVERS, adr/)
│   └── worklog/                  ~150 pre-merge-era session prompts/ledgers (historical, engine repo's own)
├── ui/                           React 19 + Tauri 2 shell
│   ├── src/app|components|hooks|lib|stores|styles|theme|workers|i18n|assets|queries/
│   ├── src/connector/            contract.ts (9 domains, 36 methods) + mock/ + tauri/ implementations
│   ├── src/features/             12 feature dirs: device-selection, health, scan-progress,
│   │                             results-explorer, backup, recovery-queue, repair, lost-partition,
│   │                             preview, consent, settings-about, dev-panel
│   ├── src-tauri/                Rust shell: src/bridge/ (abi, events, state, error, consent,
│   │                             filefilter, logging, util), src/commands/ (11 domains), app.manifest
│   │                             (requireAdministrator), build.rs (app_manifest wiring), tauri.conf.json,
│   │                             capabilities/, icons/
│   ├── e2e/                      3 Playwright specs, 12 tests — MOCK-pinned
│   ├── tests/                    28 vitest files + fixtures (connector-mock/tauri, event-surface,
│   │                             exit-guard, home-states, scan-state-drain, …)
│   ├── rules/                    2 ast-grep architecture rules
│   ├── scripts/                  arch-lint, bundle-check, gen-filefilter, i18n-sweep, visual-compare
│   ├── docs/, worklog/           fresh-UI-repo-era docs/prompts (pre-merge, partially stale)
├── installer/                    recoverywizard.nsi (542 ln combined NSIS wrapper: app bundle driven
│                                 silent + engine kit + ProgramData state + complete uninstall),
│                                 devsign.ps1 (ephemeral self-signing), portable.ps1, README.md
├── tools/forensic/               the 7 forensic probe scripts (collect-facts, seed-disks, install-probe,
│                                 launch-app, io-probe, drive-ui.mjs, unelevated-launch)
├── tools/validation/             measure-cr-ui.c (T14 harness), installer-matrix.ps1 (T15)
├── worklog/                      the session sheet archive (§6)
├── build.ps1 / build.sh          the build entry points (§5)
└── VERSION                       0.9.2
```

## 5. Product-determining configs

| Config | Value / role |
|---|---|
| `VERSION` | `0.9.2` — the product version source of truth; release tag must equal it (release.yml:36-45) |
| `build.ps1` (275 ln) | Single Windows build entry. Targets `all\|contracts\|engine\|ui\|installer\|portable`, `-SkipTests`. **Version-lockstep gate** (VERSION == ui/package.json == Cargo.toml == tauri.conf.json — this gate caught the v0.9.1 first attempt, `cdf66ff8`). Engine lane: cmake preset + ctest + PE gates (Win7 stamp + import allowlist) + kit staging (cr_ui.dll/.lib + rrecov.exe + vcpkg runtime DLLs, gtest/gmock excluded) + devsign. UI lane: pnpm gates + kit staged into `src-tauri/target/{release,debug}` + `pnpm tauri build` + **D1 gate: byte-scan for `requireAdministrator` in the built exe** (build.ps1:169-190). Installer lane: makensis with native defines + devsign. Portable + SHA256SUMS. |
| `build.sh` (53 ln) | Linux subset: contracts + engine (linux-release preset + ctest) + ui static checks. No Tauri/installer lanes. |
| `ui/src-tauri/tauri.conf.json` | Recovery Wizard 0.9.2, `com.heicfast.recoverywizard`, NSIS-only bundle, installMode "both", languages EN+zh-CN, WebView2 downloadBootstrapper, frameless 1240×840 maximized main window, CSP null |
| `ui/package.json` | 0.9.2; pnpm 12.15.0, node ≥24; React 19.3.1, Tauri api/cli 2.12.4, vite 8.3.5, vitest 4.1.12, playwright 1.64.1; scripts: arch:lint, knip, jscpd, bundle:check, filefilter:check, e2e |
| `engine/CMakeLists.txt` (180 ln) | project **core_recovery VERSION 0.6.1** (documented drift, BF-19 — the product gates on `cr_ui_abi_version()`=4 only); C++20/C11; MSVC `/W4 /WX /permissive-`; Win7SP1 macros `_WIN32_WINNT=0x0601`; zlib REQUIRED; **lz4+zstd REQUIRED** (FB-X01: no codec-less builds); `cr_deploy_vcpkg_binaries` POST_BUILD; GTest FetchContent 1.15.2 |
| `installer/recoverywizard.nsi` (542 ln) | Combined installer; `RequestExecutionLevel admin`; all-users only; engine kit + Tauri app bundle (driven silent) + ProgramData sessions/logs; silent uninstall keeps sessions by design (interactive consent to delete); uninstall removes everything else; header carries the v0.8.x hallucination truth-note |
| `ui/src-tauri/app.manifest` (49 ln) | `requireAdministrator` + Common-Controls v6 dependency + Win10/11 & 8.1 supportedOS — the whole D-PRIV-01 privilege model, wired via tauri-build `WindowsAttributes::app_manifest` (build.rs) |
| `.gitignore` (16 ln) | build/, dist/, out/, target/, node_modules/, vcpkg_installed/, `ui/src-tauri/gen/`, `*.log` (engine fixtures excepted) |

ABI verification (this session): `engine/include/cr/cr_ui.h:81` `#define
CR_UI_ABI_VERSION 4u`; exactly **92 `CR_UI_API` function declarations**;
`contracts/command-surface.json` = 43 commands / 10 events / 4 removed,
version 4; `node contracts/validate.mjs` → **green**.

## 6. Worklog inventory — what exists, and staleness

**Current (this session / 0.9.x era):**
- `worklog.md` (2,730 ln) — the chronological handoff through every session;
  ends with the 8f3bedd1 re-open entry (the prior sessions' claims digest is
  at :2156-:2730). CURRENT.
- `worklog/session-debug-and-release-prompt.md` — this session's prompt +
  the user's addendum VERBATIM (the new defect register seed, :180-:225).
  CURRENT — the authoritative statement of what the user reports.
- `worklog/COORDINATION.md` — the two-lane seam (file ownership, release
  protocol, the clang-format pre-push discipline). Current through 0.9.0.
- `worklog/BEHAVIOR-FINDINGS.md` — the BF register (BF-01..BF-32) with
  status annotations; updated through the 0.9.0 session. Predates
  eb4bb5f3 (Nov 9).
- `worklog/REPO-STATE.md` — **THIS sheet** (refreshed from the v0.8.4-era
  version @ `c8aef05a`, last touched by `58c8f5b9`).

**v0.8.4-era Phase-0 set (the debug-and-release session's baseline; STALE
vs the 0.9.x tree — refresh mandated this session):**
- `worklog/DIAGNOSIS.md` — the canonical defect register (D-PRIV-01,
  D-CONS-01, D-ENUM-01, D-SCAN-01 four-root stack, D-UI-01 + W/U/E
  secondary). Still the defect-ID vocabulary; its "fix directions" are now
  historical claims to re-verify.
- `worklog/UI-MAP.md`, `CORE-API.md`, `WIRING-AUDIT.md`,
  `REFERENCE-BEHAVIOR.md`, `INSTALLER-AND-PRIVILEGE.md` — lane A's
  canonical Phase-0 sheets (@ v0.8.4).
- `worklog/lane-b/{CORE-API,DIAGNOSIS,INSTALLER-AND-PRIVILEGE,
  REFERENCE-BEHAVIOR,REPO-STATE,UI-MAP,WIRING-AUDIT}.md` — lane B's
  verbatim set (same vintage, preserved as audit trail).
- `worklog/REPRODUCTION/D-{PRIV,CONS,ENUM,SCAN}-01/` — v0.8.4 reproduction
  evidence (forensic run 42255492260) with per-defect READMEs.
- `worklog/RELEASE-VALIDATION/` — RELEASE-RUN.md (the release/forensic
  campaign ledger v0.7.0→v0.9.0; **ends at v0.9.0** — no v0.9.1/v0.9.2
  sections), BUILDS.md, CAPABILITY-MAP.md, CORE-MERGE.md,
  DRIVER-REMOVAL.md, UI-WIRING.md.
- `worklog/DOC-AUDIT.md` — doc-truth register (11 surfaces trued at 571d588a).

**Consolidation-era (v0.6.0-rc2/v0.7.0; STALE):**
- `worklog/BUILD-PIPELINE.md` (@ a6cdead9), `UI-INTEGRATION-SURFACE.md`,
  `FINAL-PROD-INVENTORY.md`, `INTEGRATION-PLAN.md` (the P1-P6 plan),
  `CORE-CURRENT-STATE.md` (core_recovery@12f3edf7), `UI-REPO-STATE.md`
  (recovery-wizard-ui-new@dd2782c).
- `worklog/context-sheets/` — explicitly **ARCHIVED 2026-11-05** driver-era
  evidence (CORE-SURFACE, DRIVER-DEPLOYMENT, REFERENCE-INSTALLER,
  REFERENCE-LAYOUT, UI-SURFACE).
- `worklog/session-consolidation-prompt.md`,
  `worklog/session-debug-integration-prompt.md` — prior session prompts.

## 7. The prior sessions' FIXED claims — the re-verification list

Everything below is CLAIMED in `worklog.md`/`RELEASE-RUN.md`. This session's
standing rule (worklog.md:2726-2728): "the prior sessions' 'FIXED' claims
are hypotheses, not facts, until re-proven on a real Windows runner with the
installed product."

**Released in v0.9.0 (tag `8fe16ab8`; forensic run 42278088695 partially
validated, then caught D4e):**
1. D-PRIV-01 `76bdf056` — requireAdministrator manifest + OS-refusal twin +
   11 docs trued + build.ps1 D1 gate + forensic manifest gate. Forensic
   v0.9.0: manifest gate GREEN.
2. D-CONS-01 `68460a4c` — hidden direct-fsutil TRIM query + tree-wide
   no-`_popen` build gate (`engine/tests/unit/test_no_console_spawn.cpp`) +
   import allowlist (`4aa33d62`).
3. D-ENUM-01 (two layers) `c6ff7c10` (UI: Internal+External groups render)
   + `0eeff0ec` (engine: never drop a PRESENT device). Forensic v0.9.0:
   home census gate GREEN ("4 OS disks, 0 missing") — **but on an
   all-internal-bus Azure runner; the SATA-mislabeled-removable defect
   (D-ENUM-02) was still live and INVISIBLE to that census** (eb4bb5f3).
4. D-SCAN-01 (four roots) `ce71143e` D4a live bytes_scanned / D4b bytes_found
   ABI append + live stamping / D4c bounded cancel; + W-01 instant-completion
   & live-tree bridges `34e7332e`. Forensic v0.9.0: live found-size GREEN,
   smooth carve GREEN — then RED at the NEW 655 s D4e freeze.
5. D-UI-01 resolved (banner surface removed); W-02 exit guard, U-01/02/03
   truthfulness trio `21f33f32`; T14 persistence `3d03eec2`.

**Released in v0.9.1 (tag `cdf66ff8`; forensic run 42291160236 RED at D4f):**
6. D4e + E-01 `62fb7670` — 4 MiB remnant-sweep ticks; exact-size
   GetLogicalDriveStringsW retry. (v0.9.1's own forensic catch: D4f.)

**ON MAIN BUT NEVER RELEASED (§2 — the top re-verification items):**
7. D4f `0e87689d` — lock-free user-sign stride probe (the 605 s zero-region
   freeze: registry mutex + per-sign token scan × ~134 M zero strides).
   Claims output-identical; pin `D4fUserSignProbe.TheStrideLoopIsLockFree`.
8. D-ENUM-02 `eb4bb5f3` — the SHIFTED bus table (bus 11 = SATA not SD →
   every SATA disk `removable=true` → internal drives sorted into the
   external group; "the disks page looked incomplete"); RemovableMedia byte
   as authority; `bus_type` ABI axis (BF-02/BF-31 closure).
9. D-NVMe-01 `eb4bb5f3` — the pack(1) offset-9 STORAGE_PROTOCOL_SPECIFIC_DATA
   misplacement that made EVERY real NVMe controller refuse the health query
   ("switch to another disk and it shows nothing"). **Invisible on Azure
   VHDX seeds** (no NVMe passthrough) — never validated on real NVMe
   hardware.
10. BF-03 closure `eb4bb5f3` — `cr_ui_health_read_ex2` +
    `cr_ui_smart_attr.threshold` + `power_cycles_nvme` (the SMART threshold
    column).

**BF register state (BEHAVIOR-FINDINGS.md):** FIXED: BF-04, BF-22 (=
D-SCAN-01), BF-25 (bounded cancel), BF-28. OPEN by record: BF-06 (NG-21
engine-side guard, mitigated bridge-side), BF-07, BF-08..BF-12, BF-16,
BF-17, BF-21, **BF-29 (the "Searching:" path ticker is dead on the real
backend — `currentPath` always null)**, **BF-30 (the carve throughput wall:
8.45 MiB/s effective, 76-124 s/GiB — a multi-TB user disk means the last
25% of a scan runs hours-to-days EVEN WITH every D4 fix)**, BF-31 (SD/USB
kind fidelity — partially addressed by eb4bb5f3's bus_type axis),
**BF-32 (the scan-emitter teardown race — a latent use-after-free window at
bridge teardown, deliberately held out of every train; "next bridge
session's first item")**, BF-23 (SMART untestable on Azure runners — the
exact surface the user reports broken).

## 8. Red / broken in the repo state itself (facts found this survey)

1. **The v0.9.2 release does not exist** (no tag local/remote; no release
   run; RELEASE-RUN.md ends at v0.9.0) while the tree and worklog claim
   "post-v0.9.2". The version-lockstep files say 0.9.2; any tag push now
   would build a 0.9.2 containing eb4bb5f3 — but as of HEAD nothing shipped.
2. **Main was red at session open** — engine-ci #34 failed clang-format on
   eb4bb5f3's own files; repaired as `0c80036f` (whitespace only). Pattern
   recurrence: 6 of the last ~30 commits are mechanical format repairs
   (`0c80036f`, `12d824a3`, `87034a7d`, `6565c112`, `22bae5be`,
   `3fe8403c`'s parent-fix) — the pre-push format discipline keeps failing
   in exactly the way COORDINATION.md:60-70 warned about.
3. **forensic-repro.yml:50 default `release_tag` is still `v0.8.4`** — a
   careless dispatch without the input probes the wrong release.
4. **engine-ci.yml:60 stale claim**: "release.yml calls this lane as its
   validation gate" — release.yml has no `workflow_call` use; it re-runs
   build.ps1 itself. (Doc-truth gate M-088 does not cover this comment.)
5. **Runner-class drift**: ui-ci/ui-e2e run `windows-latest` (unpinned)
   while engine-ci/forensic/release pin `windows-2022` (the
   execution-validated class, engine-ci.yml:38-40).
6. **The UI e2e lane is mock-pinned** (ui-e2e.yml:50 `VITE_CONNECTOR:
   mock`) — 12 tests prove the mock connector, never the bridge.
7. **No end-to-end validation exists for the current main tree**: the last
   forensic runs are v0.9.0 (42278088695, RED→D4e) and v0.9.1 (42291160236,
   RED→D4f, recorded in worklog.md only). The D4f/D-ENUM-02/D-NVMe-01 fixes
   have NEVER been through forensic-repro, and D-NVMe-01/BF-03 cannot be
   validated on Azure runners at all (BF-23) — a bare-metal or
   unit-integration strategy is required for the SMART surface.
8. **BF-32** — the scan-emitter teardown race (raw SendPtr vs entry Arc
   Drop) remains an unfixed latent use-after-free in
   `ui/src-tauri/src/bridge/events.rs:349-398` / `state.rs:291-327`.
9. **BF-29** — the "Searching: <path>" ticker is dead on the real backend
   (`currentPath: null` always); the reference runs a live path ticker.
10. **BF-30** — the carve throughput wall (8.45 MiB/s) is open; on the
    user's real (multi-TB?) disks the scan's final phase takes hours-to-days
    by measurement — a plausible physical substrate for "stuck in sector
    scanning" even after every freeze fix.
11. **engine/CMakeLists.txt:4 project VERSION 0.6.1** vs product 0.9.2 —
    documented drift (BF-19), harmless but trip-wire-y for doc-truth gates.

## 9. What this sheet does NOT establish (honest limits)

- Which build the user actually runs (v0.9.1 is the newest installable
  release; the worklog claims "post-v0.9.2" but no such artifact exists —
  resolving this contradiction is Phase 1's first question).
- Live CI badge status (no network from this sandbox).
- Whether the eb4bb5f3 enumeration/NVMe fixes work on real hardware (never
  run anywhere but the Linux parse lane + MSVC unit suite).
- Anything about the research corpus (Agents E/F own it).

— Agent A, 2026-11-11. Evidence: git log/show/od at HEAD 8f3bedd1; all 7
workflow files read in full; configs read in full; worklog.md:2156-2730
read in full; contracts/validate.mjs executed (green).
