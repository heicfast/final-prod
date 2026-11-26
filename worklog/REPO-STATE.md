# REPO-STATE — the complete CURRENT-STATE sheet of final-prod @ 126ea398 (the released 0.9.6 train + the unreleased W2/F-03 follow-up)

> Agent A (Phase 0 refresh, the **fix-upgrade-release** session,
> 2026-11-26). READ-ONLY survey; every claim cites a `path:line` or a
> commit SHA in the tree at HEAD
> `126ea398ffa3123c2ff643b196deb1a7b030baab` (branch `main`, verified
> `git rev-parse HEAD`; tracked tree CLEAN and up to date with
> `origin/main`; 2558 commits total; ONE untracked file —
> `worklog/session-fix-upgrade-release-prompt.md`, this session's saved
> prompt, deliberately not committed yet). This sheet was **refreshed
> 2026-11-26 at 126ea398** from the v0.9.2-era version @ `8f3bedd1`
> (2026-11-11); structure and section conventions carried over. That
> sheet's critical finding — "v0.9.2 was never released" — is now
> history: the train jumped to 0.9.3, and `v0.9.3`/`v0.9.4`/`v0.9.5`/
> `v0.9.6` all exist as tags LOCAL and REMOTE (verified this session:
> `git ls-remote --tags origin` → v0.9.6 = `f4dcc3e8`, a lightweight
> tag, 2026-11-18 20:30 UTC, an ancestor of HEAD). `VERSION:1` =
> `0.9.6` and the four product version sites are in lockstep (§5).
> **HEAD is 6 commits PAST the v0.9.6 tag** — the F-01 closed-loop and
> W2/F-03 lane is unreleased (§2). `node contracts/validate.mjs` was
> re-run this session: **GREEN** ("41 error codes; 27 ui codes; 12
> events; 59 commands"). Live Actions status was NOT queried (out of
> scope; no token used); last recorded lane evidence is cited per lane.
> Display caveat carried forward from the prior sheets: this console
> strips the literal two-character sequence bracket-m from captured
> output; verify branch filters with `od -c` / a YAML parser before
> "fixing" them.

---

## 1. Commit-graph summary — 158 commits since the last sheet (8f3bedd1, Nov 11)

Two big non-release work blocks, then FOUR releases cut on one day
(2026-11-18), then a six-commit unreleased follow-up lane:

- **The performance session (Nov 17–18)**: engine IO rework —
  CachedReader sharded LRU (585c5aa7), the overlapped IOCP read-ahead
  ring (18b918d5), first-byte carve dispatch table 5.55x (b90976d9) —
  measured in `worklog/PERFORMANCE.md`; the `benchmark-src` lane
  restored as the measurement vehicle (84459c0f).
- **The cross-repo harvest session (Nov 19)**: six reference repos
  pinned (ea4b8684), FEATURE-MATRIX/LICENSING sheets (aa4d87ea), then
  the implementation waves — W1 deleted-only reconstruction with
  directory structure (cb2c8e0a, +cr_ui exports 95→99), W5 vendor/
  archive surfaces (9ad991d6, 795f5328), W8 corpus filters + image
  scan targets (8d61644e, 47925a96), W6 rescue imaging (7c7b62b6),
  W9 imaging connector (76a6738c), and the CI red-main repairs they
  forced (5e57d8b2 NOMINMAX, c6c52265 SetFilePointerEx allowlist).
- **The installer-api lane (Nov 18 evening)**: the 0.9.4
  shipped-installer exit-2 root cause closed and PROVEN on a foreign
  machine (5798cf39 — the installer probed a compile-time build-machine
  path at runtime; kit payload now $PLUGINSDIR-extracted, 02e62afb
  CopyFiles); the phantom zero-job push failures closed (cc201bff,
  235d3c32); v0.9.5 released green end to end (e0e1c6ff: "1224 s,
  assets verified").

### 1.1 The release trains (all tags verified local + remote)

| Tag | Commit | Cut (UTC) | Train (per release.yml notes + commits) |
|---|---|---|---|
| `v0.9.3` | `b04cad16` | 2026-11-18 05:31 | The smart-scan/parity train: SAT-over-USB + WMI SMART transports, the engine's 4-state health verdict end to end, health its own window, the per-phase scan gauge, seeded-fixture integration gates, preview truthfulness (release.yml:163-206) |
| `v0.9.4` | `342fd87f` | 2026-11-18 06:45 | The honesty patch: file-backed disks (VHDX/Storage Spaces) render again after 0.9.3's 'virtual' kind filtered them off every scanning surface (release.yml:148-162) |
| `v0.9.5` | `b4bf9ea0` | 2026-11-18 18:01 | The CI/release performance train: concurrency groups, timeouts, vcpkg/pnpm/cargo caches on every lane, measured in worklog/OPTIMIZATION.md (201100aa) |
| `v0.9.6` | `f4dcc3e8` | 2026-11-18 20:30 | W1 deleted-only reconstruction (F-01, SF parity), W6 ddrescue-style rescue imaging, W8 corpus filters + image-file scan targets, red-main repairs; version-lockstep repairs during the cut (2bab7c9a package.json/tauri.conf 0.9.5→0.9.6, run 44390728388; bed4d22c Cargo 0.9.5→0.9.6, run 44391177507; f4dcc3e8 Cargo.lock app entry was 0.9.4-stale) |

### 1.2 The unreleased follow-up lane (v0.9.6..HEAD, 6 commits)

| Commit | Claim |
|---|---|
| `be7966c5` | F-01b root causes fixed: engine dot-transparency ('.'/'..' volume-root names sanitized into `_` path components — every real-volume reconstruct nested under `_\`), fixture durability (WriteThrough recreate), fixture rooting (seeds at X:\ volume root) |
| `1a1f6877` | null-safe magic lookup for magic-less rows + prettier (runs 44474398913/44474393997) |
| `20efb62a` | ui-ci knip: tests/timing-budget-proof.ts is standalone evidence — knip-ignored (run 44475009006) |
| `8c86a484` | worklog: the 44391734922 root-cause session record; F-01 closed-loop lane re-dispatched; ui-ci M31 timing budget (two serial mock latencies 700±200+600±200 ms vs RTL's 1000 ms default) |
| `d65a5d12` | F-01 closed-loop red (run 44475017206) root-caused to a VERIFIER defect: deleted-DIRECTORY rows hit the I2 file-exactness branch |
| `126ea398` (HEAD) | **W2/F-03**: the NTFS parent-sequence gate — a reused parent record is not a deleted entry's parent |

### 1.3 Last-commit-per-directory (drift map, at HEAD)

- `engine/` → `126ea398` (W2/F-03, the HEAD commit itself).
- `ui/` → `20efb62a` (knip ignore); `ui/tests/` grew the timing proof.
- `contracts/` → `05c1104f` (W8/M22 reasonKeys).
- `installer/` → `02e62afb` (CopyFiles $PLUGINSDIR payload fix).
- `tools/` → `d65a5d12` (the F-01 verifier fix).
- `docs/` → `5a374ce5` (W3 doc-truth sweep — 93 exports/12-67 seam rows).
- `.github/` → `d65a5d12`; `worklog/` → `8c86a484`.

## 2. Release posture at HEAD — what shipped, what did not

Verified facts:

1. `git tag -l` + `git ls-remote --tags origin` → newest tag is
   `v0.9.6` at `f4dcc3e8`, present local AND remote. The release
   workflow ran for it: the cut's own repair commits cite the release
   runs (2bab7c9a → run 44390728388, bed4d22c → run 44391177507 — the
   build.ps1 VERSION gate refusing the stale sites, then green).
2. **HEAD (126ea398) is 6 commits past the tag** (§1.2). Any release
   cut from HEAD would be a NEW version: the W2/F-03 parent-sequence
   gate, the F-01b dot-transparency fix, the fixture-rooting/durability
   fixes, and the F-01 verifier fix are in NO shipped build.
3. The named-feature F-01 closed loop: the last recorded state is in
   `worklog.md:4554-4580` — the 44391734922 engine-flow red root-caused
   to three defects (engine F-01b + fixture durability + fixture
   rooting), all fixed at `be7966c5`; at handoff, lanes were in flight
   (named-feature-fixture(14) on 1a1f6877, engine-ci(73)/bridge(68) on
   be7966c5, ui-ci(129)/ui-e2e(114) on 20efb62a). Their CURRENT status
   at 126ea398 is UNKNOWN from this sandbox (§9).
4. The v0.9.6 GitHub release NOTES were composed from the release.yml
   template AS-IS at the tag (verified `git show
   v0.9.6:.github/workflows/release.yml`): the newest "What changed"
   section is **"(0.9.5)"** and the body says "0.9.5 carries this
   train" — the W1/W6/W8 content of 0.9.6 is not narrated anywhere in
   the shipped notes (§8 item 2).

## 3. Workflows — the 10 CI/validation lanes (all files read in full)

Seven lanes existed at the last refresh; THREE are new since
(`benchmark-src`, `bridge-integration`, `named-feature-fixture`), and
the original seven changed (caches/concurrency/timeouts, the 0.9.5
train).

| Lane (ln) | Trigger / paths | Runner | Jobs → what it runs | Purpose in one line / caveats |
|---|---|---|---|---|
| **contracts-ci.yml** (48) | push(main)+PR on `contracts/**`, `engine/include/**`, `ui/src/connector/**`, `ui/src-tauri/src/**`, `ui/scripts/gen-filefilter.mjs` | ubuntu-24.04, node 24 | `validate`: `node contracts/validate.mjs` (error-code map ↔ `cr_error.h`; event map ↔ `CR_EVT_*`; command manifest ↔ Rust `#[tauri::command]` AND TS invokes) + `gen-filefilter.mjs --check` | Boundary-truth gate, phase 1 + 2. **Re-run this session: green** (41 codes, 12 events, 59 commands). Nothing runtime. |
| **engine-ci.yml** (505) | push(main)+PR on `engine/**` (negates `*.md`, `engine/docs/**`, `engine/worklog*`) + `workflow_call` + `workflow_dispatch` | windows-2022 (pinned) | Job `lint` (15 min): clang-format **18.1.8 pinned** over src/include/tests; N-041 dead-surface + direct-access + driver-era lints; vcpkg.json presence; PowerShell AST parse over every CI-consumed script; ROOT workflow YAML parse + inline-pwsh run-block AST; doc-truth gate (`tools/doc_truth_gate.py`, M-088). Job `build-and-test` (needs lint, 90 min): vcpkg binary cache; `cmake --preset win-x64-release` MSVC /W4 /WX; full ctest; Win7 posture gate (stamp_osversion_um.py + verify_pe_um.py vs `ref_imports_um.json`) on rrecov.exe/cr_tests.exe/cr_ui.dll; G-09 report schema; elevation refusal probes (unelevated `rrecov scan` exit 4 via Probe-Unelevated.ps1; elevated `bench --disk 0 --mode info` exit 0); M-071 CLI junk-filter contract (tree+session round-trip+A-013 resume+M-079 auto-save naming) | Proves the ENGINE. Stale comment at :60 ("release.yml calls this lane as its validation gate") — release.yml runs build.ps1 directly (§8 item 4). |
| **bridge-integration.yml** (307, NEW) | push(main)+PR on `engine/**`, `ui/src-tauri/**`, `contracts/**`, `tools/gen_abi_layouts.py` + dispatch | windows-2022 | One job `bridge` (75 min): pnpm/node/rust setup; frontend dist build (tauri context macro needs dist/ at LIB compile); **[1/5] generated-ABI freshness gate** (gen_abi_layouts.py re-run + clang-format 18.1.8 + `git diff --exit-code` over dump/table/sha); [2/5] engine build; [3/5] layout-manifest dump; [4/5] attach 2 VHDX (6144 MB seeded NTFS: 120 live + 80 deleted + 3 GiB deleted bulk + real JPEGs/zip; 512 MB raw) → 3+ disk shape; [5/5] `cargo test --manifest-path ui/src-tauri/it/Cargo.toml --features layout-gate -- --test-threads=1`; evidence uploaded ALWAYS | **BF-ABI-02 closure**: the only lane that LINKS the real bridge against the real engine and runs integration tests on real disks (ui-ci is cargo-check-no-link; the layout gate catches the eb4bb5f3 stale-mirror class). |
| **named-feature-fixture.yml** (244, NEW) | workflow_dispatch ONLY | windows-latest (fixture job) / windows-2022 (engine-flow job) | Job `fixture` (20 min): build a 2 GB GPT/NTFS VHDX, generate the fixture TWICE (determinism proof, run 2 on RUNNER_TEMP — D-LANE-01), apply the deletion matrix (double flush), verifier self-test, detach, upload VHDX+manifest+logs. Job `engine-flow` (needs fixture, 60 min): build rrecov; full scan of the detached VHDX as an image (whole-disk quick+deep+carve); `recover --reconstruct-deleted`; byte-for-byte verifier I1–I5 (deleted-only, exactness, collision-set, accountability, containment); output-listing captured runner-side (dot-file artifact-glob gap, run 44475017206) | The F-01 named-feature closed loop on seeded ground truth. The VHDX is never remounted after the deletion flush. |
| **benchmark-src.yml** (172, NEW) | workflow_dispatch ONLY (inputs: profile smoke/full, scan_modes, reason, run_tests) | windows-2022 | One job `benchmark` (300 min): build.ps1 engine lane (release recipe); compile api_harness.c; optional full ctest before measuring; runner-context JSON; the scenario matrix (Run-Benchmark.ps1 over a seeded VHD, kit bridged into the release-zip bin layout); the alignbench sweep (16 MiB × QD32, `\\.\C:`); evidence artifact (90 d) | The measured before/after vehicle for engine perf work (PERFORMANCE.md's contract); numbers runner-dependent, recorded deliberately, never pushed. |
| **forensic-repro.yml** (321) | workflow_dispatch ONLY, input `release_tag` (**default 'v0.9.3'**; description still says v0.8.4 — §8 item 3) | windows-2022, 90 min | Downloads the SHIPPED release assets (checksum-verified), machine facts, seeds 64 GB NTFS + 8 GB exFAT vdisks, silent install with full process/window capture (S1), manifest gate (requireAdministrator, D-PRIV-01), disk census, one-step CDP drive of the real UI (S2 health/SMART, S3 scan timeline to 28 min with IO probe, S4 all surfaces; home census + D-SCAN-01 gates), unelevated twin (OS refusal, win32 740), app-log snapshot, silent uninstall + no-leftover assertions, evidence upload `if: always()` | The ONLY lane that validates the INSTALLED product E2E. SMART untestable on Azure VHDX (BF-23). Default tag is two releases stale. |
| **release.yml** (317) | push tags `v*` | windows-2022, 90 min, contents:write | Version gate (tag == VERSION); contracts gate; `./build.ps1 -Target all`; artifact presence + devsign verify (setup.exe + cr_ui.dll); release-notes composition; softprops upload; build-log artifacts (signing material excluded, NG-17) | The full-product release lane. Ran green for v0.9.5/v0.9.6 (per worklog). Notes template's newest section is "(0.9.5)" (§8 item 2). |
| **ui-ci.yml** (124) | push(main)+PR on `ui/**`, `contracts/**` | **windows-latest (UNPINNED)**, 30 min | pnpm 12.15.0/node 24 frozen install; typecheck; eslint; prettier --check; arch-lint A1-A7; ast-grep rules (2); filefilter mirror; knip (KNIP_DISABLE_RAW_TRANSFER=1); jscpd (min-tokens 70); vitest --coverage (70/70/70/60); vite build; bundle budget (MAIN 200 KB / TOTAL 500 KB); `cargo check --tests` (no link); clippy `-D warnings`; web-dist artifact | The UI quality lane. No engine link (bridge-integration owns that now). The M31 timing-budget red and the knip ignore landed here. |
| **ui-e2e.yml** (78) | push(main)+PR on `ui/**` | **windows-latest (UNPINNED)**, 20 min | Builds frontend with **`VITE_CONNECTOR: mock`**; playwright chromium; `pnpm e2e` vs vite preview — **4 specs / 16 tests**: smoke 6, responsive 4, consent 2, lane-c-surfaces 4; screenshots + traces artifacts | Mock-pinned by design — the e2e lane NEVER exercises the real bridge/engine. |
| **validation-evidence.yml** (231) | workflow_dispatch ONLY | windows-2022 ×2 | Job `t14-measurements` (60 min): build.ps1 engine -SkipTests, elevation facts, measure-cr-ui.c harness (cl /O2), bridge G-7 filter/memory via `cargo test --release -- --ignored`, runner context. Job `t15-installer-matrix` (100 min): build.ps1 all -SkipTests; `tools/validation/installer-matrix.ps1` (install/update/repair/uninstall rows) | Measured evidence lanes (T14/T15); artifacts curated by hand into `docs/INTEGRATION-VALIDATION/`; no masking. |

### 3.1 Action pins (every `uses:` across the 10 files — all SHA-pinned)

| Action | Pinned SHA | Used by |
|---|---|---|
| `actions/checkout` | `11d5960a326750d5838078e36cf38b85af677262` (v4) | all 10 |
| `actions/setup-node` | `49933ea5288caeca8642d1e84afbd3f7d6820020` (v4) | contracts-ci, forensic-repro (**node 20** there; node 24 elsewhere), release, ui-ci, ui-e2e, validation-evidence |
| `actions/cache` | `0057852bfaa89a56745cba8c7296529d2fc39830` (v4) | engine-ci, bridge-integration, named-feature-fixture, validation-evidence (both jobs) |
| `actions/upload-artifact` | `ea165f8d65b6e75b540449e92b4886f43607fa02` (v4) | engine-ci, bridge, named-feature-fixture, release, ui-ci, ui-e2e, validation-evidence, benchmark-src |
| `actions/download-artifact` | `d3f86a106a0bac45b974a628896c90dbdf5c8093` (v4) | named-feature-fixture |
| `pnpm/action-setup` | `b906affcce14559ad1aafd4ab0e942779e9f58b1` (v4, pnpm 12.15.0) | bridge, release, ui-ci, ui-e2e, validation-evidence |
| `dtolnay/rust-toolchain` | `b725eda525362e7cf34af9a7243cac1e3f01a7ec` (stable 2026-10-16) | bridge, release, ui-ci, validation-evidence (both jobs) |
| `ilammy/msvc-dev-cmd` | `0b201ec74fa43914dc39ae48a89fd1d8cb592756` (v1) | benchmark-src, validation-evidence t14 |
| `Swatinem/rust-cache` | `d1527c8f67ef9af357290475d4af4b0fab8f12e0` | ui-ci only |
| `softprops/action-gh-release` | `3bb12739c298aeb8a4eeaf626c5b8d85266b0e65` (v2) | release only |

Un-pinned-in-manifest deps installed ad hoc by lanes: `clang-format==18.1.8` and `pyyaml` via pip (engine-ci, bridge), `playwright-core@1.49.1` via npm in `tools/forensic` (forensic-repro).

## 4. Directory map (top 3 levels, `.git` excluded; one-line purposes from actual contents)

```
final-prod/                       2558 commits; VERSION=0.9.6 (released train + 6 unreleased commits)
├── .github/workflows/            the 10 CI lanes (§3)
├── contracts/                    engine↔UI boundary truth: command-surface.json (v4, 59 commands/
│                                 12 events/3 removed), error-codes.json (41), event-mapping.md (12),
│                                 type-maps.md, validate.mjs, README.md — CI-validated BOTH sides
├── docs/                         19 top-level product docs + INTEGRATION-VALIDATION/ (t14/t15 evidence):
│                                 ARCHITECTURE, BUILD, DEV-SIGNING, FORENSIC-PROBE, RELEASE, PARITY-DELTAS…
├── engine/                       C++20 user-mode recovery engine (core_recovery lineage, ABI v4, 99 exports)
│   ├── CMakeLists.txt            project core_recovery 0.6.1 (documented drift, BF-19); C++20/C11;
│   │                             MSVC /W4 /WX /permissive- /utf-8 /external:W0; Win7SP1 macros
│   │                             (_WIN32_WINNT=0x0601); zlib REQUIRED; lz4+zstd REQUIRED (FB-X01);
│   │                             cr_deploy_vcpkg_binaries POST-BACK; GTest FetchContent 1.15.2
│   ├── CMakePresets.json         5 configure presets: linux-gcc(ASAN), linux-release, win-x64-debug,
│   │                             win-x64-release (VS2022+vcpkg toolchain), linux-ubsan
│   ├── src/app/                  rrecov.cpp — the CLI (scan/tree/recover/bench/list/…, incl.
│   │                             --reconstruct-deleted F-01 and the W6 rescue verbs)
│   ├── src/core/                 the pure recovery core (~30 subsystem dirs): ntfs, fat, exfat, refs,
│   │                             btrfs, zfs, ext4, hfsplus, carve, orchestrator, tree, session, health,
│   │                             crypto, repair, reghive, preview, query, recovery, table, storage,
│   │                             compress, lost, bcdfix, nvramfix, apply, check, err (NEW since the
│   │                             last sheet), cr_parallel.cpp
│   ├── src/ui/                   cr_ui.cpp + cr_ui_exports.def — the 99-export flat-C ABI shim (G-01)
│   ├── src/win/                  Windows platform layer, 13 modules: cr_enumerate, cr_health,
│   │                             cr_health_wmi (NEW, the WMI SMART fallback), cr_transport,
│   │                             cr_bitlocker, cr_fsck, cr_elevate, cr_destio, cr_image_create,
│   │                             cr_image_win, cr_mountpoints, cr_rescue_win (NEW, W6 rescue sink),
│   │                             cr_bootfix
│   ├── include/cr/               37 entries: 9 root cr_*.h (cr_ui.h 1,600+ ln, cr_error.h) + 27
│   │                             subsystem dirs + win/ (12 headers incl. cr_enumerate.h, cr_rescue_win.h)
│   ├── tests/unit/               80 files (78 .cpp + test_fixtures.hpp + test_tmpdir.hpp);
│   │                             1847 TEST( macros (grep); Linux suite
│   │                             1682/0 per worklog.md:4574 (CycleGuardPerf excluded locally)
│   ├── tests/win-stub/           Windows SDK stub headers (15 files: windows.h, winioctl.h, wbemidl.h,
│   │                             ntddscsi.h, sys/…) for the Linux parse/ASAN lanes
│   ├── tests/                    + abi_layout_dump.c, abi_layout_dump.header.sha, ui_abi_check.c, data/
│   ├── bench/                    the benchmark lab: og/ (og-driver.ps1, run-og.ps1, FlaUiProbe/ —
│   │                             reference tooling, does NOT ship), ours/ (run-ours, run-behavior,
│   │                             run-alignbench), scenarios/, metrics/ (9 py/mjs), seed/, tools/,
│   │                             vlm-evidence/, volume/, baseline.json, root-causes.json
│   ├── scripts/                  45 dev/bench/parity scripts (gen_* generators, sync-push, bench)
│   ├── tools/                    11 CI gate tools: doc_truth_gate.py, stamp_osversion_um.py,
│   │                             verify_pe_um.py + ref_imports_um.json (Win7 import allowlist),
│   │                             validate_report.py, gen_junk_gate_fixture.py, win_syntax_check.sh…
│   ├── third_party/stb/          stb_image.h + LICENSE (the ONLY vendored code)
│   ├── .github/scripts/          CI-consumed probes (Probe-Unelevated.ps1, Run-Benchmark.ps1,
│   │                             api_harness.c, scenarios/)
│   ├── .githooks/                pre-commit (secrets scan + staged format fast subset), pre-push
│   │                             (full ci-local gate, pwsh/Unix split)
│   ├── ci-local.ps1 / .sh        the developer full-gate entry points
│   ├── fuzz/                     fuzz harnesses (carve, btrfs, fat, exfat, ext4, hfsplus, engines,
│   │                             container) + corpus + Makefile
│   ├── cmake/                    DeployVcpkgBinaries.cmake
│   ├── vcpkg.json                manifest: zlib, gtest, lz4, zstd (version-string 0.6.1)
│   ├── data/                     userdata-defaults.json (111 user-sign defaults, ships in the kit)
│   ├── diff-to-fix/, docs/ (39), worklog/ (167), AUDIT-*.md, worklog.md — engine's own ledgers
├── ui/                           React 19 + Tauri 2 shell
│   ├── src/app|components|hooks|lib|stores|styles|theme|workers|i18n|assets|queries/ (13 query domains)
│   ├── src/connector/            contract.ts (12 *Domain interfaces, 75+ methods) + mock/ (4 files)
│   │                             + tauri/tauri-connector.ts — the connector seam
│   ├── src/features/             12 feature dirs: device-selection, health, scan-progress,
│   │                             results-explorer, backup, recovery-queue, repair, lost-partition,
│   │                             preview, consent, settings-about, dev-panel
│   ├── src-tauri/                Rust shell: src/bridge/ (abi, consent, error, events, filefilter,
│   │                             logging, state, util, mod), src/commands/ (14 domains + mod),
│   │                             lib.rs, main.rs, it/ (the bridge-it integration crate, §6),
│   │                             app.manifest (requireAdministrator), build.rs, tauri.conf.json,
│   │                             Cargo.toml/lock, capabilities/, icons/, nsis-header.bmp
│   ├── e2e/                      4 Playwright specs, 16 tests — MOCK-pinned
│   ├── tests/                    32 entries (30 test files + setup.ts + timing-budget-proof.ts)
│   ├── rules/                    2 ast-grep architecture rules; sgconfig.yml
│   ├── scripts/                  arch-lint, bundle-check, gen-filefilter, i18n-sweep, visual-compare
│   ├── eslint.config.js, tsconfig.json (strict ES2022), vite.config.ts (react compiler,
│   │                             tailwindcss, manualChunks, vitest+coverage 70/70/70/60),
│   │                             knip.json, playwright.config.ts, pnpm-workspace.yaml
│   ├── docs/ (20), worklog/ (4), worklog.md — fresh-UI-repo-era docs (pre-merge, partially stale)
├── installer/                    recoverywizard.nsi (672 ln combined NSIS installer), devsign.ps1
│                                 (177 ln ephemeral self-signing), portable.ps1 (86 ln), README.md
├── tools/forensic/               the 7 forensic probe scripts (collect-facts, seed-disks, install-probe,
│                                 launch-app, io-probe, drive-ui.mjs, unelevated-launch)
├── tools/validation/             measure-cr-ui.c (T14), installer-matrix.ps1 (T15),
│                                 named-feature/ (generate-fixture.ps1, apply-deletions.ps1,
│                                 verify-reconstruction.py, README.md)
├── tools/gen_abi_layouts.py      the ABI layout generator (freshness-gated by bridge-integration)
├── worklog/                      the session sheet archive (§7); worklog.md (4,580 ln) at repo root
├── build.ps1 (299 ln) / build.sh (53 ln)  the build entry points (§5)
└── VERSION                       0.9.6
```

## 5. Product-determining configs (version lockstep + build entries)

**Version lockstep — GREEN at HEAD**: `VERSION`=0.9.6 = `ui/package.json:4`
= `ui/src-tauri/Cargo.toml:3` = `ui/src-tauri/tauri.conf.json:4` =
`ui/src-tauri/Cargo.lock` app entry (repaired at f4dcc3e8). Remaining
drift is component-level only (§8 items 5–7).

| Config | Value / role |
|---|---|
| `build.ps1` (299 ln) | Single Windows build entry. Targets `all\|contracts\|engine\|ui\|installer\|portable`, `-SkipTests`, `-Configuration`. **Version-lockstep gate** (build.ps1:37-46; this gate refused the first v0.9.1 AND v0.9.6 cut attempts). Engine lane: cmake preset + ctest + PE gates (Win7 stamp + import allowlist, rrecov.exe + cr_ui.dll) + kit staging (cr_ui.dll/.lib + rrecov.exe + vcpkg runtime DLLs, gtest/gmock excluded + `engine/data/userdata-defaults.json`, M-065) + devsign. UI lane: pnpm gates + kit staged into `src-tauri/target/{release,debug}` + CR_UI_LIB_DIR + `pnpm tauri build` + **D1 gate: byte-scan for `requireAdministrator` in the built exe** (build.ps1:182-203). Installer lane: makensis with version-locked bundle pick (I-STALE-01) + devsign. Portable + SHA256SUMS. |
| `build.sh` (53 ln) | Linux subset: contracts + engine (linux-release preset + ctest) + ui static checks (typecheck/lint/test). No Tauri/installer lanes. |
| `ui/src-tauri/tauri.conf.json` | Recovery Wizard 0.9.6, `com.heicfast.recoverywizard`, NSIS-only bundle, installMode "both", languages EN+SimpChinese, WebView2 downloadBootstrapper, frameless 1240×840 (min 800×600) maximized main window, CSP null |
| `ui/package.json` | 0.9.6; pnpm 12.15.0, node ≥24; React 19.3.1, Tauri api/cli 2.12.4, vite 8.3.5, vitest 4.1.12, playwright 1.64.1, TS 5.9.3, eslint 10.13.0, tailwind 4.3.3, react-compiler (babel-plugin 1.0.0 + oxc-transform-react 0.155.0); scripts incl. arch:lint, knip, jscpd, bundle:check, e2e, visual:compare, filefilter:check |
| `ui/src-tauri/Cargo.toml` | 0.9.6; tauri 2, tauri-plugin-dialog 2, tauri-plugin-opener 2, serde/serde_json, windows-sys 0.59 (6 Win32 feature groups); release profile: codegen-units 1, lto, opt-level "s", panic abort, strip |
| `engine/CMakeLists.txt` (180 ln) | project **core_recovery VERSION 0.6.1** (BF-19 drift, documented); C++20/C11; MSVC `/W4 /WX /permissive-`; gcc/clang `-Wall -Wextra -Wpedantic -Werror -Wshadow`; Win7SP1 target macros; zlib REQUIRED; **lz4+zstd REQUIRED** (FB-X01); `cr_deploy_vcpkg_binaries` POST_BUILD; GTest FetchContent v1.15.2; options ASAN/TOOLS/APP/UI/TESTS |
| `installer/recoverywizard.nsi` (672 ln) | Combined installer; `RequestExecutionLevel admin`; all-users; engine kit extracted via $PLUGINSDIR (5798cf39, build-machine independent) + Tauri app bundle + ProgramData sessions/logs; silent uninstall keeps sessions by design |
| `ui/src-tauri/app.manifest` (53 ln) | `requireAdministrator` + Common-Controls v6 + Win10/11 & 8.1 supportedOS (D-PRIV-01), wired via tauri-build `WindowsAttributes::app_manifest` |
| `.gitignore` (18 ln) | build/, dist/, out/, target/, node_modules/, vcpkg_installed/, `ui/src-tauri/gen/`, IDE dirs, `*.log` (engine fixtures excepted) |

**ABI verification (this session)**:
`engine/include/cr/cr_ui.h:81` `#define CR_UI_ABI_VERSION 4u`; **99
`CR_UI_API` function declarations** (grep `^\s*CR_UI_API\s+\w`); 41
error codes in `cr_error.h` (validate.mjs: "all 41 parsed; 41
mapped"); **12 `CR_EVT_*` events**; `contracts/command-surface.json`
version 4 = **59 commands / 12 events / 3 removed** (59 Rust impls,
57 TS-invoked — green). The 93→99 export growth (W1/W5) is doc-truthed
at `5a374ce5`.

## 6. Manifest inventory — every dependency manifest file

| Manifest | Path | Contents / version |
|---|---|---|
| Product version | `VERSION` | `0.9.6` — the source of truth (release tag must equal it; release.yml:41-53) |
| vcpkg (engine) | `engine/vcpkg.json` | zlib, gtest, lz4, zstd; name `core-recovery`, version-string `0.6.1` |
| Engine fetched dep | `engine/CMakeLists.txt:174-177` | GTest **1.15.2** via FetchContent URL tarball (no lockfile — network fetch when not found) |
| Vendored code | `engine/third_party/stb/` | stb_image.h + LICENSE (the only bundled third-party source) |
| npm (app) | `ui/package.json` | 0.9.6; 22 runtime deps (React 19.3.1, Tauri api 2.12.4, radix, tanstack, i18next, zustand…) + 33 devDeps (vite 8.3.5, vitest 4.1.12, eslint 10.13.0, TS 5.9.3, playwright 1.64.1, knip 6.42.1, jscpd 5.4.2) |
| npm lockfile | `ui/pnpm-lock.yaml` | frozen-install source for all pnpm lanes |
| pnpm workspace | `ui/pnpm-workspace.yaml` | allowBuilds: @ast-grep/cli, unrs-resolver, esbuild, @tauri-apps/cli; minimumReleaseAgeExclude: react-i18next@17.1.4 |
| cargo (shell) | `ui/src-tauri/Cargo.toml` | 0.9.6 (§5) |
| cargo lock | `ui/src-tauri/Cargo.lock` | app entry `recovery-wizard-ui` 0.9.6 (repaired f4dcc3e8; was stale 0.9.4 before 0.9.5) |
| cargo (integration crate) | `ui/src-tauri/it/Cargo.toml` | `bridge-it` **0.9.4** (CI-only, `publish = false`; path-dep on the parent crate; features: layout-gate) |
| cargo lock (it) | `ui/src-tauri/it/Cargo.lock` | bridge-it crate tree |
| .NET (reference tooling) | `engine/bench/og/FlaUiProbe/FlaUiProbe.csproj` | net8.0-windows WinForms exe; FlaUI.UIA3/Core 4.0.0. **Does NOT ship**: referenced only by `engine/bench/og/run-og.ps1`, `engine/bench/README.md:37`, `engine/docs/CORE-API-CURRENT.md`, `worklog/PRODUCT-STATE.md` — no build.ps1 lane, no CMake target, no workflow builds it. Reference-product (og) benchmark probe. |
| Ad hoc (not committed) | forensic-repro.yml:71-77 | `playwright-core@1.49.1` npm-installed inside `tools/forensic` at run time |

## 7. Worklog inventory — what exists, and staleness

**Current (this session / 0.9.3–0.9.6 era):**
- `worklog.md` (4,580 ln) — the chronological handoff; ends with the
  2026-11-19 continuation entry (the 44391734922 root causes, lanes in
  flight, `worklog/SESSION-2026-11-19-F01B-ROOT-CAUSE.md` referenced).
  CURRENT through the F-01b session; nothing yet for the 2026-11-26
  fix-upgrade-release session.
- `worklog/session-fix-upgrade-release-prompt.md` — **UNTRACKED** (the
  only dirty file in the tree): this session's prompt, saved
  token-redacted at session start.
- `worklog/SESSION-2026-11-19-F01B-ROOT-CAUSE.md` — the F-01b
  three-root-cause session record. CURRENT.
- `worklog/PERFORMANCE.md`, `worklog/OPTIMIZATION.md` (87 ln),
  `worklog/CI-FAILURES.md` (170 ln, 12 sections) — the perf/CI campaign
  ledgers of the 0.9.5/0.9.6 trains. CURRENT.
- `worklog/COORDINATION.md`, `worklog/BEHAVIOR-FINDINGS.md` (207 ln,
  BF-01..BF-32; last touched `76a6738c` 2026-11-18 — see §8 item 10),
  `worklog/REPO-STATE.md` — **THIS sheet** (refreshed from the 8f3bedd1
  version, 2026-11-11).
- `worklog/FEATURE-MATRIX.md` (58 rows), `worklog/LICENSING.md`,
  `worklog/SAFETY-SURFACE.md`, `worklog/PRODUCT-PIPELINE.md`,
  `worklog/REFERENCE-*.md` (SCAN/ALGORITHMS/EDGE-CASES + the older
  set), `worklog/SECTORFORGE|HALIM|TESTDISK|SLEUTHKIT|NTFS-RECOVER|
  MFT-RESCUE-FEATURES.md`, `worklog/SMART-MATRIX.md`, `PRODUCT-STATE.md`
  — the cross-repo harvest session's Phase-0 corpus (Nov 18–19).
- `worklog/lane-c/` (API-EXPANSION.md), `worklog/VALIDATION/`
  (fixtures/, it-gates/) — the lane C / W-wave records.
- `worklog/CACHE-AUDIT.md`, `PIPELINE-AUDIT.md`, `PIPELINE-MATRIX.md`,
  `WORKFLOW-INVENTORY.md`, `PLATFORM-PERF.md`, `API-DELTA.md`,
  `BASELINE.md`, `FOOTPRINT-DELTA.md`, `VERIFICATION.md` — the CI-opt
  session's sheets.

**STALE (pre-0.9.3 vintage, kept as audit trail):**
- `worklog/RELEASE-VALIDATION/RELEASE-RUN.md` — **last touched
  `710bd35c` 2026-11-06; ends at the v0.9.0 section** + "v0.9.1
  follows" note. The v0.9.3–v0.9.6 release evidence lives ONLY in
  worklog.md / CI-FAILURES.md / OPTIMIZATION.md and commit messages.
- `worklog/DIAGNOSIS.md`, `UI-MAP.md`, `CORE-API.md`, `WIRING-AUDIT.md`,
  `REFERENCE-BEHAVIOR.md`, `INSTALLER-AND-PRIVILEGE.md` — the v0.8.4-era
  Phase-0 set (defect-ID vocabulary still in use; claims historical).
- `worklog/REPRODUCTION/D-*` — v0.8.4 reproduction evidence;
  `worklog/context-sheets/` — archived 2026-11-05 driver-era evidence
  (+ two PRODUCT-STATE snapshots: smart-2026-11-17, perf-2026-11-18);
  `worklog/lane-b/` — lane B's verbatim v0.8.4 set; the consolidation-era
  sheets (BUILD-PIPELINE, INTEGRATION-PLAN, CORE-CURRENT-STATE,
  UI-REPO-STATE, FINAL-PROD-INVENTORY, DOC-AUDIT,
  INTEGRATION-MATRIX, INSTALLER-CURRENT, UI-INTEGRATION-SURFACE).
- `ui/worklog/` (4 prompts), `ui/docs/` (20) — pre-merge UI-repo era.

## 8. Inconsistent / stale observations (facts found this survey)

1. **README.md status claims are stale**: `README.md:28` and `:46` say
   "**92 exports**" (actual: 99 — grep cr_ui.h; doc-truthed at 5a374ce5);
   `README.md:51` says the connector contract is "**36 operations
   across 9 domains**" (actual: 59 commands per
   `contracts/command-surface.json`; 12 `*Domain` interfaces in
   contract.ts). The status block (`README.md:22-28`, "consolidation
   session complete at the tree level… the connector rewiring to the
   real engine lands with the bridge… the mock connector is the
   interim") describes the 0.7-era posture — the bridge has shipped
   since v0.9.0 and the mock is the dev/test double only.
2. **release.yml's notes template lags the product**: the newest "What
   changed" section is "(0.9.5)" and the body says "0.9.5 carries this
   train" (release.yml:139-147; verified identical at the v0.9.6 tag).
   The v0.9.6 release shipped with notes that never narrate W1
   deleted-only reconstruction, W6 rescue imaging, or the W8 wave; any
   NEXT tag would ship the same stale body unless edited.
3. **forensic-repro.yml:50 default `release_tag` is `v0.9.3`** while
   the input description still says "default: v0.8.4" — both stale;
   newest release is v0.9.6. A careless dispatch probes two releases
   back.
4. **engine-ci.yml:60 stale comment**: "release.yml calls this lane as
   its validation gate" — release.yml contains no `workflow_call` of it
   (it runs build.ps1 directly). Carried from the prior sheet, still
   untrue at HEAD.
5. **engine/CMakeLists.txt:4 project VERSION 0.6.1** and
   **engine/vcpkg.json version-string 0.6.1** vs product 0.9.6 —
   documented drift (BF-19: the product gates on `cr_ui_abi_version()`
   = 4 only); trip-wire-y for doc-truth gates and version audits.
6. **`ui/src-tauri/it/Cargo.toml` bridge-it version = 0.9.4** — stale
   since before 0.9.5 (the app crate is 0.9.6). CI-only,
   `publish = false`, no functional impact; inconsistent for any
   version sweep.
7. All four PRODUCT version sites are consistent (0.9.6 — §5); the
   only 0.9.x lockstep repair needed at the v0.9.6 cut was the
   Cargo.lock app entry (f4dcc3e8: it had been stale at 0.9.4 since
   before 0.9.5 — evidence the lock is not regenerated by every lane).
8. **Runner-class drift**: ui-ci, ui-e2e and named-feature-fixture's
   `fixture` job run `windows-latest` (unpinned) while every engine/
   bridge/release/validation lane pins `windows-2022` (the
   execution-validated class, engine-ci.yml:38-40). forensic-repro
   pins node 20 while every other node lane pins 24.
9. **The e2e lane is mock-pinned** (ui-e2e.yml:55 `VITE_CONNECTOR:
   mock`) — 16 tests over 4 specs prove the mock connector, never the
   bridge; the bridge's own real-disk proof is bridge-integration only
   (since the 0.9.3-era BF-ABI-02 closure).
10. **The BF register lags the lane closures**: `47925a96`'s message
    claims "BF-12 closed" (image-file scan targets) and `05c1104f`
    claims "BF-15 closed at the connector layer", but at HEAD the
    register rows still read BF-12 "**Open** — also the ABI-check
    surface" (BEHAVIOR-FINDINGS.md:26) and BF-15 "**Mitigated** —
    distinct reasonKeys deferred" (:29); the register's last commit
    (76a6738c, 2026-11-18) predates 05c1104f. BF-32 (scan-emitter
    teardown race, the latent UAF) and BF-30 (the 8.45 MiB/s carve
    throughput wall) remain the register's open heavyweight rows;
    BF-29's current_path ticker axis landed engine-side in 9352c81d
    (W5b/M13) but the register row still says "Open, engine+bridge
    scope" (:54) — another row to re-verify rather than trust.
11. **RELEASE-RUN.md is four releases behind** (ends v0.9.0; last
    touched 2026-11-06) while v0.9.3–v0.9.6 shipped — the release
    campaign ledger moved into worklog.md/CI-FAILURES.md/OPTIMIZATION.md
    without the canonical file being extended (§7).

## 9. What this sheet does NOT establish (honest limits)

- Live Actions status for ANY lane at HEAD 126ea398 (the Actions API
  was not queried; no token used). The last recorded in-flight state
  (worklog.md:4576-4580) predates d65a5d12 and 126ea398: whether
  engine-ci/bridge/named-feature-fixture are green on the W2/F-03
  parent-sequence gate is UNKNOWN here.
- Whether the v0.9.6 GitHub release OBJECT carries all three assets
  (the tag exists local+remote; the green run is recorded in commit
  messages e0e1c6ff/9653e9d9-era notes — not re-verified against the
  API this session).
- Whether the F-01b/W2 engine changes pass the named-feature
  engine-flow closed loop on a real runner (the F-01 verdict gate,
  I1–I5) — dispatched but unrecorded at HEAD.
- Real-hardware behavior of the SMART/NVMe surfaces (BF-23: Azure
  runners cannot test them; unchanged since the prior sheet).
- Anything about the research corpus (`/home/z/my-project/work/…`) —
  not surveyed; other agents own it.

— Agent A, 2026-11-26. Evidence: git log/show/ls-remote at HEAD
126ea398; all 10 workflow files read in full; engine CMakeLists +
presets + hooks + bench + third_party read; ui package.json +
tauri.conf.json + Cargo.toml + app.manifest + configs read;
contracts/validate.mjs executed (green); build.ps1/build.sh read in
full; README.md, worklog.md:4554-4580, BEHAVIOR-FINDINGS.md rows read.
