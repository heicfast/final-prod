# FINAL-PROD-INVENTORY — the target product repo, exhaustively surveyed

Agent A (T2-a), read-only investigation. Every claim below was verified by
reading files / running `git` against the local clones at
`/home/z/my-project/final-prod` (HEAD `a6cdead9`, product baseline `aa388e2e`,
branch `main`; VERSION `0.6.0-rc2`), `/home/z/my-project/core_recovery`
(`12f3edf7`), and `/home/z/my-project/recovery-wizard-ui-new` (`dd2782c`, v3).
Sibling sheets: `worklog/UI-REPO-STATE.md` (Agent F), `worklog/BUILD-PIPELINE.md`
(Agent C). This sheet is the component/driver-census/divergence half.

Method note: all `git grep` counts below use word-boundary-aware patterns and
were manually de-noised — the naive substring `rda` matches "affo**rda**nce" and
"yeste**rda**y" (e.g. `ui/src/stores/filter-store.ts:10`, `ui/src/lib/runtime.ts:5`,
`tools/forensic/drive-ui.mjs:241`, `ui/tests/health-truth.test.tsx:12` are all
false positives, verified). Files excluded from the census per mandate:
`worklog.md`, `worklog/`, `engine/diff-to-fix/`, `docs/INTEGRATION-VALIDATION/`
(prior evidence, preserved as audit trail; `ui/docs/historical/` does not exist
in this repo — checked `git ls-files ui/docs`).

---

## 1. Component inventory

Tracked files per top-level component (`git ls-files | awk -F/ '{print $1}'`):

| Component | Files | One-line identity |
|---|---|---|
| `engine/` | 768 | C++20 recovery core, `cr_ui` C ABI (ABI v2, 80 exports) |
| `ui/` | 396 | React 19 + Tauri 2 shell + Rust bridge (34 Tauri commands) |
| `driver/` | 63 | `rda.sys` kernel raw-disk transport (service `RDA`) — TO BE REMOVED |
| `docs/` | 44 | Product docs + context sheets + INTEGRATION-VALIDATION evidence |
| `tools/` | 9 | Forensic-probe + validation harness scripts |
| `.github/` | 8 | Root workflows (contracts/driver/engine/forensic/release/ui-ci/ui-e2e/validation) |
| `contracts/` | 6 | Engine↔UI boundary truth + `validate.mjs` CI gate |
| `installer/` | 4 | NSIS installer + portable zip + dev-sign tool |
| `worklog/` + `worklog.md` | 3 | Session prompts (consolidation + debug-integration) + 1651-line session record |
| root files | 6 | `build.ps1`, `build.sh`, `VERSION`, `README.md`, `.gitignore`, `LICENSE` (MIT) |

**Provenance spine (verified via `git log --merges` and `--reverse`):** the
repo's root history *is* the rda-driver history — commit 1 of 1217 is
`1a6bfec7 "chore: bootstrap repository skeleton"` followed by driver-era commits
(`36c24ba5 "feat(driver): full kernel driver implementation..."`), positions
1..1100 are the driver lineage; `03fcad0a "root: final-prod identity"` (tree
before first component merge = just `.gitignore README.md VERSION worklog.md`,
verified via `git ls-tree 51701b14^1`) rebadged it, then the four
directory-rewriting merges (§2) layered engine/, ui/, driver/ on top. The
driver removal therefore deletes files whose history is the repo's own root
lineage — history stays, tree loses `driver/`.

### engine/ (768 files)

- **What**: clean-room rewrite of the reference product's recovery engine:
  device/partition model, nine FS engines, dynamic disks, lost-partition
  recovery, grammar-aware carving, durable scan sessions (`.crsf`),
  transactional repair, safe recovery writer. CMake project
  `core_recovery VERSION 0.4.0` (`engine/CMakeLists.txt:4`) — core_recovery
  is already at `VERSION 0.6.1`.
- **Provenance**: adopted wholesale by merge `51701b14` (core_recovery@a574ecf,
  990 commits, 769 files / 200,991 insertions), advanced by `87b9ccd9` to
  core_recovery@22e9657 (21 files, +453/−216; upstream commits 23739b2,
  394a102, 095aab5, 22e9657). Since then **12 final-prod commits** touched it
  (§4).
- **Internal layout** (`git ls-files engine | awk -F/ '{print $2}'`):
  - `src/` 85 files — `app/rrecov.cpp` (CLI), `core/` (28 module dirs: apply,
    bcdfix, btrfs, carve, check, compress, crypto, err, exfat, ext4, fat,
    health, hfsplus, lost, ntfs, nvramfix, orchestrator, preview, query,
    recovery, refs, reghive, repair, session, storage, table, tree, zfs +
    cr_parallel.cpp), `ui/cr_ui.cpp` + `cr_ui_exports.def` (ABI shim),
    `win/` (bitlocker, bootfix, destio, elevate, enumerate, health,
    image_win, transport)
  - `include/cr/` 79 files — mirrors core/ + `win/` platform headers
    (incl. the driver wire contract `win/cr_rda.h`, deleted upstream)
  - `tests/` 73 files — 58 unit-test .cpps, `ui_abi_check.c`, `win-stub/`
    (Windows SDK header stubs for the Linux parse gate), `data/`
  - `bench/` 29 files — metrics tooling (compare/compute/regression_gate,
    vlm cross-checks) + `ours/` runner scripts incl. `verify-driver.ps1`
    (driver-only; **absent from core_recovery@12f3edf7** — verified:
    core's `bench/ours/` has only run-alignbench/run-behavior/run-ours)
  - `fuzz/` 18 files — libFuzzer targets + corpus
  - `scripts/` 33 files — repro/generation scripts (gen_filefilter.py,
    gen_rawsigns.py, bench seeds, hostile smokes)
  - `docs/` 35 files — ABI-CONTRACT, API, DRIVER-UTILIZATION (**deleted
    upstream**), PARITY-*, ground-truth-contracts, etc.
  - `diff-to-fix/` **344 files** — the parity audit trail (223 findings,
    87 evidence, 27 module registers, tracked-open; preserved as history,
    merged away with the core advance)
  - `worklog/` 47 files + `worklog.md` — core's session records
  - root: AUDIT*.md (7), CMakeLists/Preset, vcpkg.json, LICENSE, README,
    RELEASE-NOTES, CONTRIBUTING, SECURITY
- **Current state**: driver-era. `include/cr/win/cr_rda.h` is the RDA wire
  contract (IOCTL grammar, PID-token handshake, F11 512× byte-count quirk);
  `cr_error.h:41` defines `ERR_DRIVER_REQUIRED = 0x50020007`; transport is
  RDA-grammar-only after `e63cdaa1` removed every direct fallback. ABI v2
  (`cr_ui.h:75` `#define CR_UI_ABI_VERSION 2u`), 80 exported functions
  (README says 78 — the +2 are the post-adoption NG appends
  `cr_ui_set_log_sink` and `cr_ui_health_read_ex2`, see §4). core_recovery
  @12f3edf7 is ABI **v4** (`include/cr/cr_ui.h:81`), **92 exports**
  (counted), no driver, `ERR_ELEVATION_REQUIRED = 0x500C0001`
  (`include/cr/cr_error.h:120`).

### ui/ (396 files)

- **What**: React 19 + Vite + Tauri 2 shell (`recovery-wizard-ui` v0.6.0-rc2),
  real wired UI: `src/connector/contract.ts` (686 lines, 9 domains, 38
  methods) + `src/connector/tauri/tauri-connector.ts` (real invoke-based
  adapter) + the Rust bridge `src-tauri/src/bridge/` (10 modules) +
  `src-tauri/src/commands/` (9 domain modules, 34 `#[tauri::command]`s
  registered in the `invoke_handler` block `lib.rs:32-77`). No mock backend
  (the simulated backend was deleted in `02d153af`; test double lives in
  `tests/fakes/test-connector.ts`).
- **Provenance**: adopted by `86bc3168` (recovery-wizard-ui-new v3@f4a8f8a,
  385 files / 34,556 insertions); **40 commits** modified it since (§5).
- **Current state**: driver-era modifications throughout (driver status
  surfaces, `driver-required` error code, mandatory-driver copy), plus the
  NG-2..NG-22 truth fixes. The fresh UI repo (dd2782c, v0.3.4, 372 files)
  diverged: its `tauri-connector.ts` is a 35-line NotImplementedError stub,
  its backend is the 1602-line `mock/mock-connector.ts`, and its contract.ts
  (662 lines) lacks the driver-era + consent additions (diff = 98 lines:
  no `driver-required`/`consent-not-found` codes, no `pendingConsent`,
  `RepairPlanSummary`, `ConsentRequestEvent`, repair-workbench types —
  verified by diffing the two contract.ts files).

### driver/ (63 files) — REMOVAL SUBJECT

- **What**: `rda.sys` kernel-mode raw-disk transport, service `RDA`, device
  `\\.\RDA\<pid-token>\<disk>\<part>` grammar. Layout: `src/` 24, `docs/` 13
  (architecture, ground-truth-contracts, signing, win7-compat, audit),
  `scripts/` 6 (install/uninstall/winpe), `fuzz/` 5 (incl. two committed
  binaries: `fuzz/selftest`, `fuzz/unit_asan`), `tools/` 3
  (stamp_osversion.py, verify_pe.py, ref_imports.json), `signing/` 2,
  tests 1, rda.sln/packages.config/Directory.Build.props, worklog.md.
- **Provenance**: the repo's root lineage (positions 1..1100) + re-adoption
  merge `840201ba` (rda-driver@da7efb9, 43 commits, 66 files / 11,098
  insertions).
- **Current state**: untouched since adoption except round-13 fixes
  (`790bf5b3` touched `driver/scripts/install.ps1` elevation guard —
  verified in its commit body); builds via WDK 26100.6584 pinned through
  `packages.config`; embed-signing happens at packaging time
  (build.ps1:134-181), never in the WDK build (driver-ci asserts unsigned).

### contracts/ (6 files)

Engine↔UI boundary truth — see §6 for detail.

### installer/ (4 files)

- `recoverywizard.nsi` (808 lines): combined NSIS installer — app (inner
  Tauri bundle, silent) + engine kit + **driver Section `SEC_DRIVER`
  (SectionIn RO, lines 281-484)** + cert import (`certutil -addstore
  Root/TrustedPublisher`, lines 320-333) + service create/start
  (`sc.exe create ... type= kernel start= demand group= "Boot File
  System"`, lines 399-440) + driver page (lines 508-535) + uninstall
  removal (673-700) + `RMDir /r "$INSTDIR\driver"` (778).
- `portable.ps1`: portable zip — stages `driver\` (lines 44-56) + README
  text explaining the no-driver-installed limitation (63-87).
- `devsign.ps1`: development test-signing tool (new-cert / sign / verify /
  install-cert). Signs rda.sys, the engine kit, and the installer; driver
  examples/comments throughout but the tool itself stays useful for engine +
  installer signing.
- `README.md`: installer rationale (driver install semantics, NG-16 notes).

### docs/ (44 files)

25 top-level + `context-sheets/` (6) + `INTEGRATION-VALIDATION/` (evidence
subtree, 19 files incl. t14-measurements, t15-runtime-matrix). Key living
docs: ARCHITECTURE, BUILD, RELEASE, DEV-SIGNING, TESTING, INTEGRATION-PLAN
(historical, 1156+ lines), INTEGRATION-GAPS, PARITY-DELTAS (incl. **D-FALL** —
the mandatory-driver directive row, `docs/PARITY-DELTAS.md:33`), PRODUCT-WIRING,
INSTALLER*, MSIX-ASSESSMENT, FORENSIC-PROBE, REFERENCE-* (corpus research).
`context-sheets/` are frozen evidence (their README: "frozen evidence, not
living documentation") produced by read-only agents 2026-10-26.

### tools/ (9 files)

`forensic/` (7): collect-facts.ps1, drive-ui.mjs (CDP UI driver),
install-probe.ps1, io-probe.ps1, launch-app.ps1, seed-disks.ps1, .keep —
the S1-S4 forensic reproduction suite. `validation/` (2):
installer-matrix.ps1 (T15 runtime matrix), measure-cr-ui.c (T14 harness).

### .github/ (8 workflows)

`contracts-ci.yml`, `driver-ci.yml`, `engine-ci.yml`, `forensic-repro.yml`,
`release.yml`, `ui-ci.yml`, `ui-e2e.yml`, `validation-evidence.yml`.
Sub-tree workflows (engine/.github, ui/.github, driver/.github) were deleted
in `2a99173c "build(ci): remove inert sub-tree .github/workflows
(consolidation to root)"` — `git ls-files engine/.github` is empty today.
**Merge watch-item:** core_recovery@12f3edf7 carries its own
`.github/{workflows,scripts,scenarios}` (12 files: checks.yml, release.yml,
benchmark-src.yml, installer.yml, test-benchmark.yml + scripts); the
engine/ advance will re-introduce `engine/.github/*` unless pruned again.

### Root files

- `build.ps1` (306 lines): single product build; lanes
  contracts/engine/driver/ui/installer/portable/all; `Invoke-Driver`
  (134-181) is the whole driver lane. Version lockstep gate
  `Assert-VersionConsistency` (36-45): VERSION must equal
  `ui/package.json` version, `ui/src-tauri/Cargo.toml` version, and
  `ui/src-tauri/tauri.conf.json` version — all verified currently
  `0.6.0-rc2`. Component wire-visible versions (driver FILEVERSION, engine
  ABI number) are documented as NOT restamped (build.ps1:14-17).
- `build.sh` (54 lines): Linux subset — contracts + engine (linux-release
  presets) + ui static checks only; comment line 3 mentions "driver,
  installer, Tauri bundle is Windows-only".
- `VERSION`: `0.6.0-rc2` (single line).
- `README.md` (68 lines): product identity; **driver claims at lines 4-11**
  ("The RDA driver is a REQUIRED component... no fallback, no degraded
  mode"), merge table (21-25), layout (29-37).
- `.gitignore`: build outputs + session scratch; `!engine/tests/**/fixtures/**/*.log`.
- `LICENSE`: MIT, "(c) 2026 heicfast / final-prod contributors".

---

## 2. Merge & release history

### The four directory-rewriting merges (`git show --stat` verified)

| Merge | Date (author) | What was adopted | Source | Size |
|---|---|---|---|---|
| `51701b14` | 2026-10-26 11:10 | engine/ ← core_recovery history (a574ecf, 990 commits) | github.com/heicfast/core_recovery (main) | 769 files, +200,991 |
| `86bc3168` | 2026-10-26 11:10 | ui/ ← recovery-wizard-ui-new v3 (f4a8f8a) | github.com/heicfast/recovery-wizard-ui-new (v3) | 385 files, +34,556 |
| `840201ba` | 2026-10-26 11:10 | driver/ ← rda-driver (da7efb9, 43 commits) | github.com/heicfast/rda-driver (main) | 66 files, +11,098 |
| `87b9ccd9` | 2026-10-26 12:34 | engine/ advanced to core_recovery@22e9657 (4 upstream commits: 23739b2 K32GetProcessMemoryInfo win7-gate, 394a102 bitlocker test fix, 095aab5 fuzz-warning zeroing, 22e9657 OQ-S4-3 docs) | core_recovery (main) | 21 files, +453/−216 |

All four used `git-filter-repo --to-subdirectory-filter` (stated in each
commit body); lineage traversable via `git log --follow engine/<file>`.
`87b9ccd9`'s body records the pre-merge ABI check: `cr_ui.h` unchanged
across 5ddccd8..22e9657 (78-export ABI v2 surface identical) and conflicts
in `engine/.github/workflows/api-benchmark.yml` + `engine/tools/verify_pe_um.py`.

### Release history (`git tag -l` + log)

Tags: `v0.5.0-rc1` → `rc2` → `rc3` → `v0.5.0` (466e41b9 / 66279f51 /
719041b2 / 6fb8f658 "the final tag on the validated tree", 2026-10-26/27)
then `v0.6.0-rc1` (a83484a2 "the first development TEST-SIGNED release —
shipped, live, byte-verified") and `v0.6.0-rc2` (e7cec9bd "the
fixes-confirmed RC — version lockstep + forensic default"; HEAD~1).
The v0.5.0 line was the pre-mandatory-driver product; v0.6.0-rc* are the
test-signed mandatory-driver RCs. rc2 validation: `aa388e2e` worklog entry
"RC validation run 40490926558 dispatched" (forensic-repro against the
tagged release).

### Version lockstep enforcement

- Source of truth: root `VERSION` (build.ps1:11-14, contracts/README.md:50).
- `build.ps1 Assert-VersionConsistency` (36-45) hard-fails on drift across
  `ui/package.json`, `ui/src-tauri/Cargo.toml`, `ui/src-tauri/tauri.conf.json`.
- `release.yml` "Version gate" (36-45) refuses a tag ≠ `v<VERSION>`.
- Wire-visible component versions are deliberately NOT restamped
  (build.ps1:14-17: driver FILEVERSION, engine ABI number 2) — after the
  re-merge the engine ABI becomes 4 (core's `cr_ui.h:81`), which is a
  *contract* change, not a version-drift issue; docs/BUILD.md:22 says the
  same.

---

## 3. Driver reference census — the removal surface (exhaustive)

Scope searched: `.github/workflows`, `build.ps1`, `build.sh`, `contracts/`,
`installer/`, `docs/`, `tools/`, `README.md`, `ui/src`, `ui/src-tauri`,
`ui/scripts`, `ui/package.json`, `ui/tests`, `ui/e2e`. Patterns: `rda`,
`RDA`, `rda.sys`, `rdacli`, `driver`, `ERR_DRIVER_REQUIRED`, `RDA service`,
`testsigning`, `\\.\RDA`. Excluded as audit trail (preserved, not edited):
`worklog.md`, `worklog/`, `engine/diff-to-fix/`, `docs/INTEGRATION-VALIDATION/`.
False positives de-noised: "affordance"/"yesterday" substring hits
(`ui/src/lib/runtime.ts:5`, `ui/src/stores/filter-store.ts:10,78`,
`ui/src/features/results-explorer/filter-bar.tsx:178,185`,
`ui/src/features/device-selection/home-page.tsx:60`,
`ui/src/theme/adopted/tokens.css:13,43,204,401`,
`ui/tests/health-truth.test.tsx:12`, `tools/forensic/drive-ui.mjs:241`) and
"CDP driver" (Chrome DevTools Protocol, `forensic-repro.yml:103` — contextual,
not the kernel driver).

### 3a. .github/workflows

| File | Reference (file:line, nature) | Classification |
|---|---|---|
| `driver-ci.yml` | entire file: the driver CI lane — lint/fuzz/unit jobs, WDK build matrix, rda.sys stamp/verify (136-163), dev-sign smoke leg (173-198), artifact upload of rda.sys/rdacli.exe/rda.inf (199-207) | **[DELETE-FILE]** |
| `engine-ci.yml` | :37-45 "dead-surface absence lint (N-041)" greps `RdaClient::`/`class RdaClient`/`volume_letter_to_disk` — a driver-era guard; core's own checks.yml:113-134 has the evolved version incl. session-96 driver-era lint (`ERR_DRIVER_`, `open_rda_partition`, ...) | **[MANUAL-EDIT]** |
| `forensic-repro.yml` | :18 "rdacli liveness during the scan", :20-21 machine-facts comment (services, driver file, certs, testsigning), :103 CDP-driver (false pos., context) | **[MANUAL-EDIT]** |
| `release.yml` | :4 lane order comment; :8-15 test-signing header; :82 `-testsigning.cer` expected artifact; :89-95 devsign verify of `$stage\driver\rda.sys`; :108-156 release-notes text (MANDATORY driver section, bcdedit testsigning instructions, portable "staged driver" copy) | **[MANUAL-EDIT]** |
| `validation-evidence.yml` | :9-16 header (T14 loads RDA on runner); :44-56 WDK setup + `build.ps1 -Target driver`; :58-123 "Load the development-signed RDA driver" step (cert import :80, install.ps1 :86, `\\.\RDA` CreateFileW probe :112, service Running assertion :91-99) | **[MANUAL-EDIT]** |
| `contracts-ci.yml` | no direct driver refs (validates contracts/ which carry them — indirect) | — |
| `ui-ci.yml` | no driver refs (cargo check/clippy of the bridge is indirectly driver-adjacent) | — |
| `ui-e2e.yml` | no driver refs | — |

### 3b. Root files

| File | Reference | Classification |
|---|---|---|
| `build.ps1` | :6-8 header comments (driver lane); :20 `ValidateSet('all','contracts','engine','driver',...)`; :134-181 `Invoke-Driver` (nuget restore, msbuild rda.sln, stamp/verify, devsign, stage rda.sys/rdacli.exe/rda.inf/install.ps1/uninstall.ps1/cert); :236 installer comment; :250 `/DDRIVER_STAGE=` define; :291 `'driver' { Invoke-Driver }`; :297 `Invoke-Driver` in the all-lane | **[MANUAL-EDIT]** |
| `README.md` | :4-11 "RDA driver is a REQUIRED component… no fallback"; :25 merge table row; :33 layout bullet `driver/ — rda.sys kernel-mode raw-disk transport (service RDA)`; :44-45 build lane text | **[MANUAL-EDIT]** |
| `build.sh` | :3 comment "The FULL product (driver, installer, Tauri bundle) is Windows-only" | **[MANUAL-EDIT]** (one line) |
| `VERSION`, `.gitignore`, `LICENSE` | no driver refs (verified) | — |

### 3c. contracts/

| File | Reference | Classification |
|---|---|---|
| `contracts/command-surface.json` | :29 command `system_driver_status` (serves system.getDriverStatus; returns "SCM probe + \\\\.\\RDA open + IOCTL GET_VERSION") | **[MANUAL-EDIT]** |
| `contracts/error-codes.json` | :65-70 mapping row `ERR_DRIVER_REQUIRED 0x50020007 → driver-required` (+ refinement note vs ERR_OPEN_FAILED) | **[MANUAL-EDIT]** |
| `contracts/type-maps.md` | :207-209 §10 `DriverStatus` (SCM service `RDA` exists; `\\.\RDA` opens; version from IOCTL) | **[MANUAL-EDIT]** |
| `contracts/README.md` | :47 cross-tree constant "Driver service name `RDA` (device `\\.\RDA`…)"; :51 "Required cr_ui ABI version 2" (superseded by v4) | **[MANUAL-EDIT]** |
| `contracts/event-mapping.md` | no driver refs (verified) | — |
| `contracts/validate.mjs` | no direct driver refs; validates the above files (indirect — regenerates with the contracts) | — |

### 3d. installer/

| File | Reference | Classification |
|---|---|---|
| `installer/recoverywizard.nsi` | :6-42 header (RDA component, DDRIVER_STAGE, all-users rationale); :59-60 `!error` if DRIVER_STAGE undefined; :137 `DRIVER_SERVICE "RDA"`; :281-484 `Section "RDA raw-disk kernel driver (required)" SEC_DRIVER` (stage files 297-304, cert import 320-341, service update 350-360, System32\drivers copy 383-396, sc create/start 399-450, post-install readback 464-484); :508 `DESC_DRIVER` LangString; :512 MUI description; :519-535 driver page (NSD labels); :618 Win-gate copy; :673-700 uninstall service removal; :704+ cert-store cleanup; :778 `RMDir /r "$INSTDIR\driver"` | **[MANUAL-EDIT]** (the section + page + defines + uninstall legs) |
| `installer/portable.ps1` | :14 layout comment `driver\`; :44-46 driver stage requirement check; :51-56 copy `driver\*` into the zip; :63-87 README text (ERR_DRIVER_REQUIRED, certutil + install.ps1 instructions, `\\.\RDA` admin note) | **[MANUAL-EDIT]** |
| `installer/devsign.ps1` | :28-49 header (driver test-signing rationale, WDK SignMode=Off, attestation future); :53-54 examples signing rda.sys; :120-123 `rda-testcert.cer`/`-thumbprint.txt` outputs; :200-201 thumbprint file lookup | **[MANUAL-EDIT]** (tool itself retained for engine/installer signing; driver-specific copy/steps edited) |
| `installer/README.md` | :41 ERR_DRIVER_REQUIRED refusal truth; :114 decision table row 6 (driver load policy vs ERR_DRIVER_REQUIRED) | **[MANUAL-EDIT]** |

### 3e. docs/ (excl. INTEGRATION-VALIDATION/ — preserved evidence)

| File | Nature of reference | Classification |
|---|---|---|
| `docs/context-sheets/DRIVER-DEPLOYMENT.md` | entire sheet is the driver deployment record (128 driver mentions — highest density in the repo); frozen-evidence context sheet | **[DELETE-FILE]** (or archive-mark; it documents only the removed component) |
| `docs/INTEGRATION-PLAN.md` | 111 mentions: §7 install plan, §8.3 lane table, D-FALL, ERR_DRIVER_REQUIRED (0x50020007) rows | **[MANUAL-EDIT]** (historical plan — completion-state notes) |
| `docs/INSTALL-FOOTPRINT-CURRENT.md` | 70: current install matrix incl. `System32\drivers\RDA.sys`, service, cert stores, ERR_DRIVER_REQUIRED (:209) | **[MANUAL-EDIT]** |
| `docs/DEV-SIGNING.md` | 29: what/why of test signing — rda.sys row in the signature table, bcdedit/testsinging matrix, attestation path | **[MANUAL-EDIT]** (retains meaning for engine+installer signing; driver rows go) |
| `docs/PRODUCT-WIRING.md` | 26: wiring truth — `devices.rs:77-83` ERR_DRIVER_REQUIRED, `cr_ui_open_disk` RDA-grammar-only (:130,:232,:291), error map `0x50020007 → driver-required` (:434) | **[MANUAL-EDIT]** |
| `docs/BUILD.md` | 11: lane list (:9-10), version policy (:22 driver FILEVERSION), driver sub-build (:30-32), WDK prerequisite (:54-55), driver-ci reference (:81) | **[MANUAL-EDIT]** |
| `docs/RELEASE.md` | 12: tag flow, artifacts, "build.ps1 embed-signs rda.sys" (:43) | **[MANUAL-EDIT]** |
| `docs/TESTING.md` | 4: driver CI lanes table rows (:12-14, incl. "Driver LOADED on CI" T14 row) | **[MANUAL-EDIT]** |
| `docs/ARCHITECTURE.md` | 8: component diagram `driver/ rda.sys` (:28,:35), install-location table (:77), ERR_DRIVER_REQUIRED (:41) | **[MANUAL-EDIT]** |
| `docs/INSTALLER.md` | 16: installer spec incl. driver section semantics | **[MANUAL-EDIT]** |
| `docs/INSTALL-FOOTPRINT-DELTA.md` | 7: T9 delta manifest vs reference (driver rows) | **[MANUAL-EDIT]** |
| `docs/INSTALLER-DECISION.md` | 7: NSIS-vs-MSI-vs-WiX decision matrix (driver SCM install column, install.ps1 console-window data :16) | **[MANUAL-EDIT]** |
| `docs/INTEGRATION-GAPS.md` | 9: NG/G gap register (driver-status related rows) | **[MANUAL-EDIT]** |
| `docs/INTEGRATION-VALIDATION.md` | 12: lane-status table incl. driver-loaded T14 row (:30) | **[MANUAL-EDIT]** (index; evidence subtree preserved) |
| `docs/PARITY-DELTAS.md` | 6: **D-FALL row (:33)** — the mandatory-driver directive register; D-I6/D-I10 driver-install rows | **[MANUAL-EDIT]** (D-FALL must be rewritten to record the reversal) |
| `docs/FORENSIC-PROBE.md` | 16: S1-S4 symptom records incl. rdacli/driver facts | **[MANUAL-EDIT]** (historical record — addenda) |
| `docs/MSIX-ASSESSMENT.md` | 12: §1 "A Windows kernel driver cannot be installed by MSIX" — the assessment's primary reason | **[MANUAL-EDIT]** (conclusion may flip) |
| `docs/REFERENCE-FOOTPRINT.md`, `REFERENCE-MODULES.md`, `REFERENCE-BEHAVIOR.md`, `context-sheets/REFERENCE-INSTALLER.md`, `REFERENCE-LAYOUT.md`, `CORE-SURFACE.md` | reference-product research: the *reference's* eprdtdrv/EuMrx drivers, `\\.\RDA` grammar of the og, D-FALL superseded marker in CORE-SURFACE (:287) | **[MANUAL-EDIT]** (light — corpus facts stay true; our-posture claims change) |
| `docs/context-sheets/README.md`, `UI-SURFACE.md` | sheet index + UI surface (driverStatus query, driver 60ms budget :505) | **[MANUAL-EDIT]** (light) |

### 3f. tools/

| File | Reference | Classification |
|---|---|---|
| `tools/forensic/collect-facts.ps1` | :1-2 header (services, driver file, rdacli, testsigning); :29 `rdaService` (Get-Service RDA); :30 `rdaServiceReg` (HKLM Services\RDA); :31 `driverFile` (System32\drivers\RDA.sys); :36 `rdacli` probe; :39 System-event filter `'RDA|rda\.sys'`; :45 note | **[MANUAL-EDIT]** |
| `tools/validation/installer-matrix.ps1` | :15-24 header (driver rows, mandatory-driver assertion); :57 `$DriverSvc = 'RDA'`; :136/:141/:199 driver service checks; :196 `driverDirFiles`; :207 `driverSigned` (Authenticode on `driver\rda.sys`); :245-246 install fails unless service Running + signed; :251-252; :264-265 uninstall file inventory (rda.sys/rdacli.exe/rda.inf/…) | **[MANUAL-EDIT]** |
| other tools (drive-ui.mjs, install-probe, io-probe, launch-app, seed-disks, measure-cr-ui.c, .keep) | no real driver refs (verified — only the de-noised false positives) | — |

### 3g. ui/ — wiped wholesale by the UI re-merge (§5); listed for the rewire phase

13 files with real driver references in final-prod's ui/ (all disappear with
the `ui/` replacement; the fresh UI must NOT re-acquire them):

| File | Reference |
|---|---|
| `ui/src-tauri/src/commands/system.rs` | :3-8 doc comments (SCM `RDA`, `\\.\RDA`, IOCTL_RDA_GET_VERSION 0x60002448, ERR_DRIVER_REQUIRED); :20-23 constants `RDA_SERVICE`/`RDA_DEVICE`/`IOCTL_RDA_GET_VERSION`/`DRIVER_REQUIRED_NOTE`; :34-119 `driver_probe()`; :123 `system_driver_status` command |
| `ui/src-tauri/src/lib.rs` | :71 `commands::system::system_driver_status` registration |
| `ui/src-tauri/Cargo.toml` | :21 comment "Win32 surface for the bridge's own probes (disk usage, SCM/\\.\RDA driver…)" |
| `ui/src/connector/contract.ts` | :58 `'driver-required'` ConnectorErrorCode; :618 `DriverStatus` interface; :651 `getDriverStatus()` |
| `ui/src/connector/tauri/tauri-connector.ts` | :60 error map `'driver-required': 'errors.driverRequired'`; :93 `0x50020007: 'driver-required'`; :365-366 getDriverStatus → `invoke('system_driver_status')` |
| `ui/src/features/settings-about/about-dialog.tsx` | :4,:27,:37-46 driver row (about.driver / driverOk / driverMissing) |
| `ui/src/features/settings-about/settings-page.tsx` | :4,:64,:197-198 driver status row |
| `ui/src/i18n/en.ts` | :12-16 about.driver* keys; :156 whole-disk copy "through the recovery driver"; :464 errors.driverRequired |
| `ui/src/i18n/zh-CN.ts` | :14-18, :448 same keys in Chinese |
| `ui/src/queries/system.ts` + `ui/src/queries/client.ts` | `useDriverStatus` hook / `driverStatus` query key |
| `ui/tests/connector-tauri.test.ts` | :285-292 getDriverStatus / system_driver_status invocation pin |
| `ui/tests/fakes/test-connector.ts` | :38,:938-943 DriverStatus fixture |

**Rewire note:** the *fresh* UI also carries a `DriverStatus` surface
(`src/connector/contract.ts:573-605`, `src/queries/system.ts:13-16`,
`src/queries/client.ts:38`, mock returns installed/running —
`mock-connector.ts:1461-1467`), plus `about.driver*` i18n keys. With the
driver gone, the rewiring phase decides: stub it truthful (installed: false)
or delete the surface from the fresh contract (a UI-side change).

### 3h. engine/ — superseded by the core advance (verified against core_recovery@12f3edf7)

19 code/build files carry real driver references today; the core merge
replaces engine/ wholesale toward core's tree, which has **already removed
the driver** (commit `be90de5d "feat(direct-access)!: the kernel driver is
replaced by direct raw i/o"`, session-96):

| final-prod engine file | Driver content | core_recovery@12f3edf7 state (verified) |
|---|---|---|
| `include/cr/win/cr_rda.h` | whole file: RDA wire contract (IOCTLs, PID token, F11) | **deleted** (not in `include/cr/win/` listing) |
| `include/cr/cr_error.h` + `src/core/err/cr_error.cpp` | `ERR_DRIVER_REQUIRED = 0x50020007` (:41) | gone; replaced by `ERR_ELEVATION_REQUIRED = 0x500C0001` (cr_error.h:120) |
| `include/cr/win/cr_transport.h`, `src/win/cr_transport.cpp` | RDA grammar transport, `rda_available` probe | rewritten direct (`\\.\PhysicalDriveN`, "The RDA driver era is GONE" — cr_transport.h:5-8); residual historical *comments* only |
| `include/cr/cr_ui.h`, `src/ui/cr_ui.cpp` | transport-truth strings "rda"/"direct", open-disk doc comments (:281-356) | no RDA refs in core's cr_ui.h (verified by grep); cr_ui.cpp keeps 3 historical comments (:1413,:2087,:2661) |
| `src/app/rrecov.cpp`, `src/win/cr_enumerate.cpp`, `src/core/*` (btrfs/carve/err/exfat/ext4/query/zfs touchpoints), `include/cr/storage/cr_directalign.h`, `src/ui/CMakeLists.txt`, `engine/CMakeLists.txt` (:5 DESCRIPTION), `scripts/dfx7_info_resolutions.py`, `bench/ours/run-*.ps1`, `tests/unit/test_cr_ui.cpp`, `test_transport_logic.cpp` | transport labels, doc comments, bench wiring | superseded (core equivalents exist, driver-free); **residual stale strings even in core**: CMakeLists.txt:5 still says "on the RDA driver" (identical in both trees — diff of the two CMakeLists.txt = VERSION line only), cr_directalign.h:19 comment, run-ours.ps1:174 stale-label note |
| `bench/ours/verify-driver.ps1` | driver-only bench verification script | **deleted** upstream |
| `docs/DRIVER-UTILIZATION.md` | driver utilization audit | **deleted** upstream (not in core's docs/) |

Plus engine docs/audit mentions (AUDIT*.md, diff-to-fix findings, worklog)
that ride along with the merge — history, not edits.

### Census totals

| Classification | Count | Files |
|---|---|---|
| **[DELETE-FILE]** | **2** (+ `driver/` itself, 63 files, the removal subject; +2 engine files vanish via the core merge: `verify-driver.ps1`, `DRIVER-UTILIZATION.md`) | `driver-ci.yml`, `docs/context-sheets/DRIVER-DEPLOYMENT.md` |
| **[MANUAL-EDIT]** | **41** | workflows 4 (engine-ci, forensic-repro, release, validation-evidence) + root 3 (build.ps1, README.md, build.sh) + contracts 4 (all except event-mapping/validate) + installer 4 (all) + docs 24 + tools 2 (collect-facts, installer-matrix) |
| **[SUPERSEDED-BY-CORE-MERGE]** | **19** engine code/build files (+ engine docs/audit trail) | table in 3h |
| **wiped by UI re-merge** | **13** ui files (+ the 37-file final-prod-only ui asset set, §5) | table in 3g |
| preserved audit trail (no edits) | worklog.md, worklog/, engine/diff-to-fix/ (344), docs/INTEGRATION-VALIDATION/, engine+ui+driver worklogs | — |

Cross-check: the T1 baseline (worklog.md, a6cdead9 commit body) measured
"154 files" via loose `git grep -il rda` — that number includes the false
positives and the audit-trail trees; the de-noised actionable surface is the
table above (2 delete-file + 41 manual-edit outside engine/ui, which are
themselves wholesale-replaced).

---

## 4. Engine divergence — final-prod-specific changes on core_recovery@22e9657

`git log --oneline 87b9ccd9..HEAD -- engine/` → **12 commits**, oldest first,
each verified with `git show --stat`:

| Commit | Subject (essence) | engine/ footprint | core_recovery@12f3edf7 equivalent? (verified) |
|---|---|---|---|
| `3c8147dc` | chore(build): win-x64-release testPreset carries `configuration: Release` (the ctest -C fix for the VS multi-config generator) | CMakePresets.json +1 | **NO** — core's win-x64-release testPreset has no `configuration` key (python-checked). Without it `ctest --preset` from build.ps1 hits the v0.5.0-rc1 "Not Run" failure class |
| `790bf5b3` | fix: round-13 product bugs + release-run-2 deploy race — `VCPKG_APPLOCAL_DEPS=OFF` in both win presets (single-deployer) + 4×250 ms copy retries in `cr_deploy_vcpkg_binaries` | CMakePresets.json, cmake/DeployVcpkgBinaries.cmake | **NO** — core's presets have no VCPKG_APPLOCAL_DEPS; its DeployVcpkgBinaries.cmake has no retry loop (grep-verified). The MSB3075 race (z-applocal vs backstop under `--parallel`) will come back |
| `e63cdaa1` | **feat(engine)!: the RDA driver is MANDATORY** — every direct-handle fallback removed; ERR_DRIVER_REQUIRED added; cr_transport rewritten RDA-only (276+/381− across 8 files) | 8 files | **INVERTED** — core session-96 (`be90de5d` + `f6a605d9` + `02ddcad0`) removed the driver entirely (direct user-mode I/O + ERR_ELEVATION_REQUIRED). Merge policy "core wins" is exactly right here; nothing to preserve |
| `ef8cd49c` | feat: mandatory driver install + dev test-signing end-to-end (engine/ touches are docs only: DRIVER-UTILIZATION.md, PARITY-DELTAS.md) | 2 docs files | docs superseded (DRIVER-UTILIZATION deleted upstream) |
| `2f4695ce` | fix: MSVC first-run findings — rpcndr.h `small` macro + format | cr_ui.cpp, cr_transport.cpp, test_cr_ui.cpp | superseded by core's own MSVC lane (its history carries the same class of fixes; files rewritten) |
| `1a7d05d5` | fix: clang-format 18, not 23 (CI-pinned formatter) | cr_ui.cpp | superseded (core pins 18.1.8 in its own CI, `4d42f64a` rider notes) |
| `6e12f167` | **fix(health): NG-5/6/7/9/10 + NG-8** — SMART surface tells the truth: `cr_ui_smart_attr_ex{base,threshold}` + `cr_ui_health_read_ex2` ABI append; `query_trim_state` → native IOCTL (fsutil `_popen` console-flash removed); health_read_impl templated; win-stub grown | cr_ui.h, cr_health.h/.cpp, cr_ui.cpp, rrecov.cpp, win-stub/winioctl.h (87+/35−) | **PARTIAL** — core has the *data* (smart_report carries transport/nvme/trim fields, ABI-v2-M-036 appends) but **NOT**: attr `base`/`threshold` (core's cr_ui_smart_attr has id/value/worst/raw/failing/name only — verified struct read), no `health_read_ex2`, and **core's `query_trim_state` still uses fsutil `_popen`** (cr_health.cpp:301-324 — the NG-8 console flash would regress). Re-port needed |
| `6c156321` | **fix(scan): NG-3 deep-scan progress liveness** — aggregated per-block sweep cursor in `RemnantCtx` (atomic `scanned`, 64 MiB `REMNANT_PROGRESS_STEP`, whole-sweep denominator); also NG-2/11/13 (UI-side halves) | cr_mftwalk.cpp (42+/7−) | **NO** — core's RemnantCtx (cr_mftwalk.cpp:1748-1759) has no progress fields; its remnant_range_scan (:2283-2313) still never ticks progress mid-sweep. Core evolved mftwalk heavily (N-213 record-header pre-gate etc.) so this is a **re-port onto a much-changed file**, not a cherry-pick |
| `41bf3b4d` | style: clang-format the health/TRIM/progress edits | cr_ui.cpp, cr_health.cpp | n/a (format-only) |
| `1dfb358d` | **fix(surfaces): NG-14 bus-type ABI append** — `cr_ui_disk_info.bus_type` tail field (STORAGE_BUS_TYPE raw; SD vs USB); NG-12/13a/17 are UI/installer halves | cr_ui.h, cr_enumerate.h/.cpp, cr_ui.cpp (13+/1−) | **NO** — core's cr_ui_disk_info has no bus_type (struct read verified; it instead added `trim_known`/`trim_on` M-076 + `layout` N-057). G-9 SD-card surface needs the append re-done on ABI v4 |
| `a5455985` | **feat(logging): NG-15 operational log seam** — `cr_ui_set_log_sink(cr_ui_log_fn, void*)` ABI append (engine cr::log seam → host sink; level 0/1/2) | cr_ui.h +11, cr_ui.cpp +18 | **NO** — `git log -S cr_ui_set_log_sink` over core's history: empty. Core's ABI v4 (92 exports) has no log sink. The bridge's ProgramData app.log wiring depends on this seam |
| `3e4d2a10` | style: clippy doc-indent + clang-format drift on a5455985 | cr_ui.cpp, cr_health.cpp | n/a (format-only) |
| `8a48b1cb` | **fix(scan): NG-21 whole-disk repair misfire** — per-partition analyze scope in cr_scan.cpp (whole_disk → per-partition from the parsed table, plans rebased) + partition-table primary guard in cr_rawfixer.cpp (GPT protective 0xEE / "EFI PART" at LBA1 / plausible MBR ⇒ refuse, never propose FixByScanSector over the disk's LBA 0); NG-22 UI label half | cr_scan.cpp, cr_rawfixer.cpp, test_orchestrator.cpp, test_rawfixer.cpp (318+/1−) | **NO** — core's cr_rawfixer.cpp analyze() has no partition-table guard (grep for "EFI PART"/0xEE/partition-table: empty); core's cr_scan.cpp whole_disk path (:2206+) is lost-partition discovery, not per-partition repair analysis. **Safety-critical: without a re-port, a whole-disk scan on a healthy GPT disk can again propose overwriting the partition table** (the S3 forensic finding, run 40470191039) |

**Net divergence risk list (what "core wins conflicts" will drop):**
1. `NG-21` repair scope + partition-table guard — **safety-critical, no upstream equivalent** → must re-port onto core's (heavily evolved) orchestrator/rawfixer.
2. `NG-15` log seam (`cr_ui_set_log_sink`) — no upstream equivalent → re-append on ABI v4 or redesign the bridge logging against core's own facilities.
3. `NG-14` `bus_type` — no upstream equivalent → re-append on v4's cr_ui_disk_info.
4. `NG-3` remnant-sweep progress — no upstream equivalent → re-port (file diverged massively; 1092 commits of core evolution).
5. `NG-5..10` health truth — partially covered; **missing attr thresholds + native TRIM** (core still fsutil-popen) → re-port `cr_ui_health_read_ex2`-equivalent + IOCTL TRIM.
6. Build fixes `3c8147dc` + `790bf5b3` (testPreset configuration, VCPKG_APPLOCAL_DEPS=OFF, deploy retries) — no upstream equivalent → **re-apply to engine/CMakePresets.json + engine/cmake/DeployVcpkgBinaries.cmake post-merge** or the Windows product build hits known failure classes.
7. `e63cdaa1`/`ef8cd49c` (mandatory driver) — deliberately inverted upstream; nothing to keep.

Everything else engine-side is core Recovery's own 1092-commit evolution
(22e9657..12f3edf7), which the merge brings in wholesale: direct transport,
ERR_ELEVATION_REQUIRED refusal contract, 95→92-export ABI v4, sessions
regrade/assistant, disk map, user signs, fsck, mountpoints, image_create,
installer/ (Inno Setup, 5 files — new `engine/installer/` arrives with the
merge), .github/ (12 files — see §1 watch-item), fuzz corpus growth
(1485 files), tests 926→~1648 (core's own suite count from its log).

---

## 5. UI delta — what gets wiped by the re-merge

`git log --oneline 86bc3168..HEAD -- ui/` → **40 commits** (integration +
driver-era + NG-fix sessions). One line each (subject-derived):

1. `2a99173c` build(ci): remove inert sub-tree .github/workflows (root consolidation)
2. `7ebd3c08` build: engine-lane artifact paths + PE-gate invocation + version sync 0.5.0
3. `1efb8d32` bridge(1/3): FFI surface pinned 1:1 from cr_ui.h + error/util/state/events/filefilter
4. `02d153af` bridge(2/3)+connector: the 33-command bridge + real TS connector; simulated backend GONE
5. `728b0baa` wiring(3/3): consumers on the real bridge + labeled test double, all gates green
6. `a790cf9b` ci+installer+gaps: root workflow consolidation (4 lanes), kit-staging truth, gaps register
7. `cdaf7e53`→`0684ad7d` bridge compile rounds 1-4 (CI reds: imports, channels, borrows)
8. `d701b74a` fix(bridge): bounded sync channels for the sinks
9. `f2ae9f59` fix(bridge): round-4 (buf mut, unused imports)
10. `521371ec` docs+e2e: shipped-reality sweep (T16) + e2e test-mode story
11. `6b4c7ebd`,`60937e45`,`61c0646a`,`39187ba7`,`4c9f115a` bridge compile/clippy rounds 5-9
12. `ec3e50b5` fix(test double): resolve virtual quick-targets like the real bridge
13. `f47463ec` feat(G-1a): pre-scan repair consent flow — prompt + consent_respond + undo-log persistence
14. `22639336`,`fcdde5b4` ci rounds: e2e root causes + prettier
15. `d87c7d3c` feat(T14+T15): validation-evidence pipeline + silent-uninstall data fix
16. `b671348f` fix(t14 tests): E0382 struct-update move ordering
17. `466e41b9`,`66279f51`,`6fb8f658`,`719041b2` releases v0.5.0-rc1/rc2/rc3/final (ctest -C fix; installer facts file)
18. `ad27cc0d` feat(ui): driver status tells the mandatory truth ('required component', test-signed)
19. `ef8cd49c` feat: mandatory driver + dev test-signing end-to-end (ui share)
20. `b9d1a366` fix(contracts+ui): ERR_DRIVER_REQUIRED flows truthfully — 'driver-required' ConnectorErrorCode
21. `6e12f167` fix(health): SMART truth (UI half of NG-5..10)
22. `6c156321` fix(scan+lost-partition): NG-2 live refetch, NG-11 real lost-partition launcher, NG-13b sector counts
23. `1dfb358d` fix(surfaces): NG-12 truthful repair page, NG-13a native folder picker, NG-17 PFX leak
24. `6c3aa9f6` chore(ui): drop dead repair query hooks (knip)
25. `a5455985` feat(logging): NG-15 bridge lifecycle → ProgramData app.log
26. `3e4d2a10` style: clippy + prettier drift
27. `8a48b1cb` fix(scan): NG-22 real tree-root label (UI half)
28. `4b609ef9` chore: ignore tsconfig.tsbuildinfo
29. `25dfafff` style: prettier NG-22 pin test
30. `b93e6be8` feat(installer) NG-16 production installer (ui share: zero consoles)
31. `a4dbcfc1` fix(portable): NG-18 portable-mode marker probe
32. `e7cec9bd` release v0.6.0-rc2 (version lockstep + forensic default)
(+ the remaining style/release-tail commits to aa388e2e.)

**These 40 commits are wiped** when ui/ is replaced by
recovery-wizard-ui-new@dd2782c. Their *concepts* are the rewiring backlog:
the 3-stage bridge build pattern, the consent flow (G-1a), truthful surfaces
(NG-2/3/5-13/18/21/22), the logging seam, the test double.

### final-prod-only ui/ assets (37 files; `comm -23` of the two `ls-files` lists)

The real-wiring assets whose concepts must be re-implemented on the fresh base:

- **`src-tauri/src/bridge/` (10 files)**: abi.rs (FFI pinned 1:1 to cr_ui.h),
  state.rs (registries + snapshot metadata cache), events.rs
  (worker-thread sink → channel → emitter), error.rs (code/bridgeCode truth),
  consent.rs, filefilter.rs, logging.rs, util.rs (portable-mode probe),
  mod.rs — **all must be rebuilt against ABI v4** (the old abi.rs is ABI v2;
  cannot be carried over).
- **`src-tauri/src/commands/{devices,scan,results,preview,sessions,recovery,health,repair}.rs` (8 files)** — the domain commands (system.rs exists in both; final-prod's is the driver-probe version).
- **`scripts/gen-filefilter.mjs`** — mirrors the engine's filter-folder table into the bridge (fresh repo lacks it; engine has the source of truth `engine/scripts/gen_filefilter.py` / core equivalent).
- **`e2e/consent.spec.ts`**, **`tests/connector-tauri.test.ts`**, **`tests/fakes/test-connector.ts`** (the labeled test double), **`tests/health-truth.test.tsx`**, **`tests/results-tree-label.test.tsx`** — the wiring-era test assets.
- **`src-tauri/Cargo.lock`** (final-prod commits the lockfile; fresh repo does not — build reproducibility decision), `metadata.json`, `.env.example`, 13 `public/assets/aistudio/*.png` screenshots.

Conversely, the fresh repo brings 13 files final-prod never had: the **mock
connector stack** (`src/connector/mock/{fixtures,mock-connector,node-space,rng}.ts`,
1602-line mock), `src/connector/tauri/not-implemented.ts`, `src/queries/dev.ts`
+ `repair.ts`, `tests/{connector-mock,node-space}.test.ts`, and its own 4
`.github/workflows` (which 2a99173c-style consolidation will need to handle
again — they will land under `ui/.github/workflows/` and are inert-but-noisy).

---

## 6. contracts/ — the shared boundary truth

- **`README.md`** (62 lines): ownership model — engine owns the ABI + error
  numeric space, ui owns the connector seam, this directory owns the mapping.
  Cross-tree constants defined once: **Driver service name `RDA` (device
  `\\.\RDA`) — line 47** (delete/replace), sessions dir
  `%ProgramData%\HeicFast\Recovery Wizard\sessions\`, logs dir `...\logs\`,
  product version = root VERSION, **required ABI version 2** (line 51 →
  becomes 4). Portable-mode marker documented here too.
- **`command-surface.json`** (60 lines): **34 commands** (verified by node
  parse), naming `<domain>_<verb>` across 9 domains (devices_list …
  system_driver_status … repair_undo), 6 events; each row: name, threading
  (sync | spawn-join | async-job), abiRefs, serves, returns. Driver-era row:
  `system_driver_status` (:29).
- **`error-codes.json`** (362 lines): **35 engine mappings + 15 bridge-only
  codes** (verified by parse). Driver-era row: `ERR_DRIVER_REQUIRED
  0x50020007 → driver-required` (:65-70). Bridge-only vocabulary:
  scan-not-found, session-not-found, scan-not-running, scan-not-paused,
  scan-scope-invalid, destination-invalid, device-gone, preview-* (7),
  consent-not-found, not-implemented.
- **`event-mapping.md`** (107 lines): CR_EVT_* 1..10 → connector progress
  events; marshaling rules (sink on worker thread must not block;
  channel + emitter; no progress synthesis from heartbeats). No driver refs.
- **`type-maps.md`** (220 lines): field-level struct conversion maps
  (Device/Volume, ScanProgressEvent, ResultNode/TreeNode/NodeDetail,
  ResultsFilter, RecoveryOptions, RecoveryJob/Report, DiskHealth,
  PreviewPayload, DriverStatus §10 :207-209).
- **`validate.mjs`** (134 lines): the CI gate (node). Phase 1: parses
  `engine/include/cr/cr_error.h` + `cr_ui.h`, cross-checks error-codes.json
  (every cr::Err mapped, values equal), ConnectorErrorCode vocabulary
  completeness, CR_EVT_* table. Phase 2 (auto-activates when
  `ui/src-tauri/src/bridge/abi.rs` exists): every manifest command has a
  Rust `#[tauri::command]`, every Rust command is manifested, every
  TS-invoked command is manifested. Run by: contracts-ci.yml:39, build.ps1
  Invoke-Contracts (:47-53), build.sh run_contracts, release.yml:48.
- **Regeneration need**: all of this describes **ABI v2** (78→80 exports,
  ERR_DRIVER_REQUIRED, 34 commands). Against core_recovery's ABI v4 (92
  exports; new verbs like enumerate_disks_ex, open_volume_with_key,
  unlock_volume_with_dictionary, session_regrade, scan_assistant, disk_map,
  user signs, fsck, image_create, turn_off_bitlocker) the maps need full
  regeneration (Agent D owns the v4 contract; this sheet records the
  mechanism only).

---

## 7. tools/ and misc

- **tools/forensic/** (7 files): the S1-S4 reproduction suite driven by
  `forensic-repro.yml` — install-process/window capture (install-probe),
  CDP UI driving (drive-ui.mjs), IO counters (io-probe), app launch
  (launch-app), vdisk seeding (seed-disks: 64 GiB NTFS + 8 GiB exFAT with
  real signatures + deletions), machine facts (collect-facts — the only
  driver-referencing one, §3f). `.keep` preserves the npm install dir for
  playwright-core.
- **tools/validation/**: installer-matrix.ps1 (T15: install/update/repair/
  uninstall runtime matrix; driver rows §3f), measure-cr-ui.c (T14 engine
  latency harness; links cr_ui.lib, compiled by validation-evidence.yml).
- **.gitignore**: build/dist/out/target/node_modules/vcpkg_installed/ +
  IDE dirs + `*.log` (negated for engine test fixtures). Note:
  `4b609ef9` added `tsconfig.tsbuildinfo` to `ui/.gitignore:14` (the
  component subtree carries its own ignore file from the fresh repo), not
  the root one.
- **LICENSE**: MIT (final-prod contributors). Each merged component also
  carries its own MIT LICENSE (engine/, ui/, driver/).
- **worklog.md** (1651 lines): the chronological session record —
  driver-adoption session, integration sessions, the mandatory-driver
  session, NG fix sessions, T1 consolidation baseline (the "154 files /
  ABI v2→v4 / 1092 commits ahead" measurements quoted in §3 and §4 are from
  its tail). Preserved untouched per mandate.
- **worklog/**: session-consolidation-prompt.md (this session's mandate,
  token-redacted), session-debug-integration-prompt.md (prior session), plus
  sibling agent sheets (UI-REPO-STATE.md, BUILD-PIPELINE.md).
- **Notable absences**: no `engine/.github`, no `ui/docs/historical/`, no
  CHANGELOG (release notes are generated by release.yml), no submodules.

---

## Cross-cutting watch-items for the integration plan (facts, not opinions)

1. **Engine build fixes must be re-applied post-merge** (§4 items 6):
   testPreset configuration, VCPKG_APPLOCAL_DEPS=OFF, deploy retries —
   verified absent in core@12f3edf7.
2. **NG-21 safety guard has no upstream home** — a whole-disk scan on a
   healthy GPT disk regresses to the catastrophic false repair offer
   without a re-port (§4 item 1).
3. **The merge re-introduces `engine/.github/` (12 files) and
   `engine/installer/` (5 Inno-Setup files)** — both were deliberately
   absent from final-prod's engine subtree; decide keep-or-prune.
4. **Stale driver strings survive even in core** (`engine/CMakeLists.txt:5`
   DESCRIPTION "on the RDA driver" — identical both sides;
   `cr_directalign.h:19`; `run-ours.ps1:174` note) — cosmetic residue the
   removal phase may want to sweep upstream or locally.
5. **The fresh UI's DriverStatus surface exists in its contract** (§3g
   rewire note) — an explicit decision point, not an oversight.
6. **Cargo.lock policy flips**: final-prod commits `ui/src-tauri/Cargo.lock`;
   the fresh repo does not track it (§5).
7. **Version lockstep machinery survives the re-merge intact** (VERSION →
   package.json/Cargo.toml/tauri.conf.json gate in build.ps1:36-45 +
   release.yml tag gate) — the fresh UI's versions must be restamped to the
   next product version on adoption.
