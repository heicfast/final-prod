# INTEGRATION-PLAN — the consolidated execution plan for the consolidation session

> Agent E (T3), Phase 0 gate document. Consolidates the five Phase 0 sheets:
> `worklog/FINAL-PROD-INVENTORY.md` (A), `worklog/UI-INTEGRATION-SURFACE.md` (B),
> `worklog/BUILD-PIPELINE.md` (C), `worklog/CORE-CURRENT-STATE.md` (D),
> `worklog/UI-REPO-STATE.md` (F). Every load-bearing claim was re-verified against
> the trees at final-prod@a6cdead9, core_recovery@12f3edf7,
> recovery-wizard-ui-new@dd2782c (v3). **Nothing below is implemented until this
> plan is committed and pushed.** Phase 0 deliverable per session prompt §10.1.

---

## 0. Executive summary & phase order

**The mandate in one line:** remove `driver/` entirely, advance `engine/` to
core_recovery@12f3edf7 (ABI v4), replace `ui/` with recovery-wizard-ui-new@dd2782c
and REWIRE it for real to ABI v4, update every workflow, cut release 0.7.0 and
validate — green main at every push, no force-push, findings filed not hidden.

**Phase order (decided by the lead engineer; verified against the sheets —
the order is correct, see the rationale):**

| Phase | Name | One-line scope | Why here |
|---|---|---|---|
| **P1** | UI re-base | Replace `ui/` with fresh UI@dd2782c + CI posture adoption + version lockstep 0.7.0 | The old `ui/` bridge is ABI-v2-pinned (`CR_UI_ABI_VERSION 2u`, engine/include/cr/cr_ui.h:75) and its 55 called exports die at the core merge; the fresh UI has **no engine dependency at all** (src-tauri = 3 shell commands, Cargo.toml has no engine crate — UI-REPO-STATE §5), so main stays green with only contracts/CI-posture edits riding along. |
| **P2** | Core advance | `engine/` → core_recovery@12f3edf7 (subtree merge, core wins conflicts) + re-prunes + engine-ci adoption + contracts v4 data | After P1 nothing in `ui/` binds the engine, so the ABI v2→v4 break (14 adds, 2 removals, 9 struct changes — CORE-CURRENT-STATE §3) has no consumer to break. |
| **P3** | Driver removal sweep | Delete `driver/` (63 files) + `driver-ci.yml` + the 41-file MANUAL-EDIT census + core-internal stale strings + context-sheet archiving + grep-to-zero | After P2 the engine is already driver-free (core session-96 `be90de5d`), so the sweep is pure deletion/edit with no engine rebuild needed. |
| **P4** | UI rewiring | Rebuild the bridge (ABI v4) + real tauri-connector + contract extensions (consent, elevation, lost-partition apply, unlock, thumbnails, archive, resume) + mock demotion + contracts/ regeneration | Needs P2's v4 header and P3's driver-free posture (elevation vocabulary replaces driver vocabulary). |
| **P5** | Workflows & CI finalization | SHA-pin every surviving action, benchmark posture, filefilter gate reinstated, final polish | Mechanical; done once the file set is stable. |
| **P6** | Release & validation | v0.7.0 tag, release.yml rewrite, forensic-repro + validation-evidence redesign, evidence under worklog/RELEASE-VALIDATION/ | Last, on the finished tree. |

**Verified load-bearing facts underpinning the order:**
- (a) **Deleting `driver/` breaks no CI build.** engine-ci.yml touches only
  `engine/**` (BUILD-PIPELINE §1.3); ui-ci/ui-e2e only `ui/**`; contracts-ci only
  runs `node contracts/validate.mjs` + the filefilter check (data-only);
  forensic-repro/validation-evidence are `workflow_dispatch`-only; release.yml is
  tag-only. No push-triggered workflow references `driver/` except driver-ci.yml
  itself (deleted in the same push). build.ps1 is not run by any push-triggered
  workflow.
- (b) **The fresh UI's CI posture is green on its own config.** Its own
  `.github/workflows/ci.yml` (v3 push) runs typecheck→lint→format→arch:lint→
  ast-grep(npx)→knip→jscpd→test:coverage→build→bundle:check→cargo check→clippy
  `-D warnings`; `ui-smoke.yml` runs build+e2e+tauri-debug-build with
  `VITE_CONNECTOR: mock`. final-prod's ui-ci.yml differs only by the two steps
  the fresh package.json lacks (`sg:lint`, `filefilter:check` — verified against
  recovery-wizard-ui-new/package.json:11-30) — both dropped/retimed in P1.
- (c) **contracts/validate.mjs stays inert (not red) while `bridge/abi.rs` is
  absent** — read in full (contracts/validate.mjs:99-131): phase 2 gates on
  `existsSync('ui/src-tauri/src/bridge/abi.rs')`; absent → prints "phase 2
  pending … activate automatically when it does" and passes. Phase 1 (checks 1-4,
  :39-97) validates engine error headers ↔ error-codes.json ↔ contract.ts
  vocabulary ↔ event-mapping.md ↔ manifest well-formedness — **phase 1 DOES run
  and must be kept green at P1 and P2** (see the per-phase contracts edits
  below).
- **Green-main traps verified and encoded per phase:** P1 must edit
  contracts-ci.yml (the `gen-filefilter.mjs --check` step dies with the fresh ui/
  which has no `scripts/gen-filefilter.mjs` — BUILD-PIPELINE §7) and
  ui-ci.yml (missing-script failures); P1 must also adapt
  `contracts/error-codes.json` uiCodes to the fresh 24-code vocabulary
  (`driver-required`/`consent-not-found` do not exist in the fresh
  ConnectorErrorCode — verified contract.ts:56-80); P2 must add the 5 new v4
  error rows + 2 new CR_EVT rows before push (checks 1/3 parse the merged
  headers in the same tree).

**Version line:** root `VERSION` → `0.7.0` at P1 (lockstep gate
build.ps1:36-45 verified: `ui/package.json` `.version` equality, `ui/src-tauri/
Cargo.toml` regex `version\s*=\s*"0.7.0"`, `ui/src-tauri/tauri.conf.json`
`.version` equality); tag `v0.7.0` only at P6. Cargo.lock: fresh repo does not
track one; P4 commits it (bridge adds deps; reproducibility, final-prod
precedent — FINAL-PROD-INVENTORY watch-item 6).

---

## 1. Phase 1 — UI re-base (fresh UI@dd2782c onto final-prod)

### 1.1 Merge mechanics (history preserved, driver-era modifications wiped)

1. Clone/recovery-wizard-ui-new @ dd2782c (branch v3); rewrite its history to the
   `ui/` prefix exactly as the original adoption did
   (`git-filter-repo --to-subdirectory-filter ui`, the 86bc3168 mechanic —
   FINAL-PROD-INVENTORY §2). The merge-base with final-prod's existing `ui/`
   lineage is the rewritten f4a8f8a (final-prod adopted v3@f4a8f8a at 86bc3168;
   dd2782c descends from it — UI-REPO-STATE §8), so this is a genuine 3-way
   merge, not an import.
2. `git merge --no-commit <rewritten-dd2782c>` into final-prod main. Resolve:
   - Every conflict under `ui/` resolves **toward the fresh UI** (theirs). This
     wipes the 40 final-prod wiring/driver-era commits (FINAL-PROD-INVENTORY §5)
     by construction.
   - Explicitly `git rm` the **37 final-prod-only ui/ paths** (they are ours-only
     additions a 3-way merge would otherwise keep — UI-REPO-STATE §8 list):
     `src-tauri/src/bridge/{abi,consent,error,events,filefilter,logging,state,util,mod}.rs` (9),
     `src-tauri/src/commands/{devices,health,preview,recovery,repair,results,scan,sessions,mod}.rs` (9 — `system.rs` conflicts, theirs wins),
     `src-tauri/Cargo.lock`, `scripts/gen-filefilter.mjs`,
     `tests/{connector-tauri,test-connector-fake→fakes/test-connector,health-truth,results-tree-label}` assets,
     `e2e/consent.spec.ts`, `.env.example`, `metadata.json`,
     `public/assets/aistudio/*.png` (11).
   - Explicitly `git rm -r ui/.github` — the fresh repo's 4 subtree workflows
     (ci/ui-smoke/visual-compare/release) land under `ui/.github/workflows/` and
     are inert-but-noisy; the 2a99173c precedent (root consolidation) deletes
     them (UI-REPO-STATE §6.7 note; FINAL-PROD-INVENTORY §1 watch-item).
   - Keep everything the fresh repo brings: the mock connector stack
     (`src/connector/mock/{fixtures,mock-connector,node-space,rng}.ts`),
     `src/connector/tauri/{tauri-connector,not-implemented}.ts`,
     `src/queries/{dev,repair}.ts`, `tests/{connector-mock,node-space}.test.ts`,
     its docs/ (171 files), worklog.
3. Commit the merge pure (no drive-by edits).

### 1.2 Same-push follow-up commit — CI posture + version lockstep (keeps main green)

1. **ui-ci.yml** (edit): drop the two steps whose scripts the fresh package.json
   lacks — `pnpm sg:lint` (:53-57 partially) and `pnpm filefilter:check`
   (:59-60). Replace the ast-grep half with the fresh repo's own invocation
   (npx `ast-grep scan --rule rules/no-raw-invoke.yml --globs '!src/connector/**'`
   + the no-connector-import rule with negation globs — fresh ci.yml "Architecture
   rules (ast-grep)" step, adopted verbatim). `cargo check` stays (fresh runs
   plain `cargo check`; final-prod's `--tests` flag may stay — both green on the
   3-command shell). Everything else (pnpm 12.15.0, node 24 cache, knip with
   `KNIP_DISABLE_RAW_TRANSFER`, jscpd, coverage 70/60/70/70, build, bundle
   budget 200/500, clippy `-D warnings`, web-dist upload) maps 1:1.
2. **ui-e2e.yml** (edit): adopt the fresh shape — add an explicit
   **build step with `VITE_CONNECTOR: mock`** before `pnpm e2e` (the fresh
   playwright webServer is `pnpm preview --port 4173 --strictPort` which serves
   an existing dist/; fresh ui-smoke.yml builds first). Playwright chromium
   install, screenshots/traces uploads unchanged.
3. **contracts-ci.yml** (edit): drop the `node ui/scripts/gen-filefilter.mjs
   --check` step (:41-42) — the script no longer exists; it returns in P4 when
   the mirror is regenerated. Trigger paths stay valid (`ui/src/connector/**`
   exists in the fresh tree).
4. **contracts/error-codes.json** (edit, phase-1 green requirement): re-point
   uiCodes onto the fresh 24-code vocabulary (contract.ts:56-80): drop the
   `consent-not-found` bridgeOnly row (no consent surface until P4); re-map
   `ERR_DRIVER_REQUIRED 0x50020007 → device-permission` **interim** (the v2
   engine still emits it until P2; row context notes the P2 successor
   ERR_DEVICE_DISCONNECTED→device-gone). All 24 fresh vocabulary members remain
   referenced (repair-consent-required via ERR_APPLY_CONSENT_DENIED; device-gone
   via bridgeOnly) — check 2 green.
5. **Version lockstep 0.7.0** (4 files): root `VERSION` → `0.7.0`;
   `ui/package.json` `.version`; `ui/src-tauri/Cargo.toml` `version = "0.7.0"`;
   `ui/src-tauri/tauri.conf.json` `.version` (fresh repo is 0.3.4 in all three —
   verified). The Assert-VersionConsistency gate (build.ps1:36-45) is then green.
6. **README.md root** — NOT yet edited (that is P3's driver sweep); the merge
   table line for ui (README.md:21-25) may be updated to record the re-adoption
   (dd2782c) — optional here, mandatory by P6 doc-truth.

### 1.3 Risks (P1)

- **Mock becomes the shipped default temporarily.** After P1 the production
  build runs the mock connector (`VITE_CONNECTOR=mock` default in
  src/connector/index.ts:17-20). Sanctioned intermediate state (lead's decision);
  the P6 release gate must prove `AppInfo.connectorMode === 'tauri'` (forensic
  S-probe) — encoded in §6.
- Merge conflicts in shared files (contract.ts diverged 662 vs 686 lines;
  tauri-connector 35 vs 390) — resolved mechanically toward theirs.
- pnpm lockfile: the fresh `pnpm-lock.yaml` replaces final-prod's (the fresh
  repo's own lock is the frozen-lockfile source; verified present).
- `tests/` aliasing: the fresh vitest setup runs the REAL factory (mock default),
  not final-prod's old `tests/fakes` vite alias — no action needed; the old
  alias trick returns in P4 only if the double needs isolating (it does not —
  vitest env pinning is cleaner, see §4.9).

**P1 file-ops tally:** 1 merge commit (≈37 ours-only deletions, ≈13 fresh-only
arrivals minus 4 subtree workflows, ~350 theirs-resolution files) + 8 follow-up
file edits (ui-ci.yml, ui-e2e.yml, contracts-ci.yml, contracts/error-codes.json,
VERSION, ui/package.json, ui/src-tauri/Cargo.toml, ui/src-tauri/tauri.conf.json).

---

## 2. Phase 2 — Core advance merge (engine/ → core_recovery@12f3edf7)

### 2.1 Mechanics (the 87b9ccd9 pattern, scaled)

1. Rewrite core_recovery@12f3edf7 history to the `engine/` prefix
   (`git-filter-repo --to-subdirectory-filter engine`, the 51701b14/87b9ccd9
   mechanic). Merge into final-prod main with **core wins conflicts**
   (`-X theirs` for engine/** content; commit body records the policy, as
   87b9ccd9's did).
2. **Pre-merge ABI note for the commit body** (the 87b9ccd9 precedent recorded
   the surface delta): v2 80 exports → v4 92 exports; `CR_UI_ABI_VERSION` 2→4
   (core include/cr/cr_ui.h:81, verified); +14/−2 exports and 9 struct changes
   enumerated in CORE-CURRENT-STATE §3 (the −2 are final-prod-local patches that
   never landed upstream: `cr_ui_set_log_sink`, `cr_ui_health_read_ex2`).
3. **Conflicts expected** (final-prod's 12 engine commits, FINAL-PROD-INVENTORY
   §4): `cr_ui.h/.cpp` (NG-9/NG-15 patches), `cr_health.*`, `cr_mftwalk.cpp`,
   `cr_scan.cpp`, `cr_rawfixer.cpp`, `cr_enumerate.*`, `test_cr_ui.cpp`,
   `test_transport_logic.cpp`, `CMakePresets.json`, `cmake/DeployVcpkgBinaries.cmake`
   — all resolve toward core EXCEPT the two build fixes re-applied post-merge
   (below). The 87b9ccd9 precedent also saw conflicts in
   `engine/.github/workflows/api-benchmark.yml` + `engine/tools/verify_pe_um.py`.
4. **Re-prune (2a99173c precedent):** `git rm -r engine/.github` (12 files: 5
   workflows checks/release/benchmark-src/test-benchmark/installer + 6 scripts +
   1 scenario json — verified listing) and `git rm -r engine/installer` (5 Inno
   Setup files: README/build-installer.ps1/core-recovery.iss/osgate.ps1/
   run-gates.ps1 — verified). final-prod's root workflows own CI; the NSIS
   wrapper owns installation. Record both prunes in the commit body.
5. **final-prod-only engine files preserved** (mandate): `engine/diff-to-fix/`
   (344 files, the parity audit trail) and any other ours-only paths survive the
   merge untouched (3-way keeps ours-only files).
6. **Post-merge re-applies (the two Windows build fixes with no upstream
   equivalent — FINAL-PROD-INVENTORY §4 item 6, verified absent in core):**
   - `engine/CMakePresets.json`: win-x64-release testPreset gains
     `"configuration": "Release"` (the ctest `-C` fix, 3c8147dc) — core's
     checks.yml:294 runs `ctest --preset win-x64-release -C Release`; build.ps1
     lane needs the preset fix to match.
   - `engine/cmake/DeployVcpkgBinaries.cmake` + both win presets:
     `VCPKG_APPLOCAL_DEPS=OFF` + the 4×250 ms copy-retry loop (790bf5b3 — the
     MSB3075 z-applocal race under `--parallel`).
   Both re-applies are final-prod-local divergence — noted in the commit body +
   BEHAVIOR-FINDINGS (upstream recommendation).

### 2.2 Dropped final-prod-local engine patches — dispositions (all verified vs core@12f3edf7)

| Patch | Disposition |
|---|---|
| `e63cdaa1` mandatory-driver inversion (cr_transport RDA-only) | **Dropped deliberately** — core session-96 inverted it (direct `\\.\PhysicalDriveN`); nothing to keep. |
| `ef8cd49c` DRIVER-UTILIZATION.md docs | Dropped — deleted upstream. |
| `2f4695ce`/`1a7d05d5` MSVC/clang-format fixes | Dropped — core's own lanes carry the class. |
| `6e12f167` NG-5..10 health truth (attr base/threshold, health_read_ex2, native TRIM) | **Dropped at merge; bridge-side degradation + BF-03/BF-04** — core has the data but not `cr_ui_smart_attr.threshold` nor `health_read_ex2` (CORE-CONTRACT §3.3.2), and `query_trim_state` still fsutil-`_popen`s. UI shows value/worst/raw/status only. |
| `6c156321` NG-3 deep-scan progress liveness | Dropped; **BF-05** (re-port onto core's heavily-evolved cr_mftwalk.cpp). |
| `1dfb358d` NG-14 `bus_type` tail field | Dropped; **BF-02** (v4 `cr_ui_disk_info` has `removable` + `trim_known/trim_on`, no bus_type — DeviceKind mapping degrades: removable→usb/sd approximation, documented). |
| `a5455985` NG-15 `cr_ui_set_log_sink` | Dropped; **P4 binds the mangled export instead** (§4.4) + **BF-01** (upstream flat-C re-land recommended). |
| `8a48b1cb` NG-21 per-partition repair scope + partition-table guard | Dropped; **P4 bridge-side mitigation** (§4.6) + **BF-06** (safety-critical engine re-port). |

### 2.3 engine-ci.yml adoption (core's checks.yml posture)

Rewrite `.github/workflows/engine-ci.yml` to mirror core's
`.github/workflows/checks.yml` (read in full), adjusted for the subtree:
- **Job `lint`** (windows-2022, timeout 15): clang-format==18.1.8 pip-pinned
  dry-run over `engine/src,include,tests`; the N-041 dead-surface lint
  (`RdaClient::`, `class RdaClient`, `volume_letter_to_disk`) + the
  direct-access architecture lints (`rda_available(`, `win_pe_environment(`,
  `open_harddisk_volume(`, `prefer_rda`, non-comment) + the session-96 driver-era
  lint (`open_rda_partition`, `open_rda_volume`, `query_driver_status`,
  `RdaTargetDevice`, `compute_pid_token`, `kDefaultDriverService`,
  `cr_ui_driver_status`, `ERR_DRIVER_`) + vcpkg.json presence + the PowerShell
  AST parse gate + the workflow-YAML parse gate + the doc-truth gate — all
  path-prefixed `engine/`. **This replaces final-prod's current N-041-only lint
  (engine-ci.yml:37-45) with core's evolved version** (checks.yml:113-134).
- **Job `build-and-test`** (needs lint, windows-2022, timeout 90, vcpkg binary
  cache keyed on `hashFiles('engine/vcpkg.json')`): git longpaths; cmake
  `--preset win-x64-release` + build (MSVC /W4 /WX); full ctest `-C Release`;
  Win7 posture gate (stamp_osversion_um.py + verify_pe_um.py over rrecov.exe,
  cr_tests.exe, cr_ui.dll); G-09 report-schema validation; **the elevation
  refusal-contract probes** (usage/list/list-carve-formats exit 0;
  Probe-Unelevated.ps1 exit-4 refusal; elevated `bench --disk 0 --mode info`
  passes) — ported with `engine/` path prefixes.
- **Dropped:** final-prod's ubuntu linux/linux-tsan/linux-ubsan/fuzz jobs (core's
  windows-only consolidation directive, checks.yml:3-8; sanitizers/fuzz remain
  locally runnable via `engine/ci-local.sh --san|--fuzz`). The drop is recorded
  in the commit body (final-prod loses Linux CI build coverage of engine/; the
  win layer was parse-gated only anyway — CORE-CURRENT-STATE §8).
- Triggers: push/PR `engine/**` + workflow, `paths-ignore` docs-only, event-scoped
  concurrency cancel-in-progress (checks.yml:63-72 pattern).

### 2.4 Contracts data edits (same push — phase 1 green)

- `contracts/error-codes.json`: +5 rows (v4 additions): `ERR_DEST_PATH_TOO_LONG
  0x50050006 → destination-invalid`, `ERR_DEST_PATH_INVALID 0x50050007 →
  destination-invalid`, `ERR_DEST_NETWORK 0x50050008 → destination-invalid`,
  `ERR_FVE_ELEVATION_REQUIRED 0x50060002 → device-permission` (context: FVE
  store ACL), `ERR_ELEVATION_REQUIRED 0x500C0001 → device-permission`
  (**interim**; P4 introduces the dedicated `elevation-required` code and
  re-points the row — mandate decision). Rename the 0x50020007 row
  ERR_DRIVER_REQUIRED→**ERR_DEVICE_DISCONNECTED**, uiCode `device-gone`, context
  updated (N-125 reattach-then-resume; v2's driver meaning is gone with the old
  engine).
- `contracts/event-mapping.md`: +2 rows — `CR_EVT_SESSION_LOAD_PROGRESS 11`
  (session-load progress, coalesced from the calling thread),
  `CR_EVT_AUTOSAVED 12` (completion auto-save notification).
- `contracts/README.md`: required ABI version 2 → **4** (:51).
- `contracts/type-maps.md`: unchanged at P2 (its v2 struct maps are rewritten
  wholesale at P4); driver-era §10 DriverStatus row removed at P3.

**P2 file-ops tally:** 1 merge (768-file subtree rewritten toward core's ~900+
tree; 12+5 re-prunes; 2 build-fix re-applies) + engine-ci.yml rewrite + 3
contracts data files + docs/BUILD.md engine-CI row (may ride P3).

---

## 3. Phase 3 — Driver removal sweep (the complete worklist)

Everything below is from Agent A's census (FINAL-PROD-INVENTORY §3, file:line
cited there) re-verified for phase-appropriateness. **Order within P3: tree
deletions first, then workflows/build, then contracts/installer/docs/tools, then
the UI DriverStatus removal, then the engine stale strings, then the archive
moves, then the grep sweep.**

### 3.1 Deletions

1. `git rm -r driver/` — **63 files** (src 24, docs 13, scripts 6, fuzz 5 incl.
   the two committed binaries, tools 3, signing 2, tests 1, rda.sln,
   packages.config, Directory.Build.props, worklog.md). History stays (the root
   lineage is the driver lineage — positions 1..1100; the tree loses it).
2. `git rm .github/workflows/driver-ci.yml` — 100% driver lane; its dev-sign
   smoke leg's proof moves to release.yml's verify gate (BUILD-PIPELINE §1.2).
3. `docs/context-sheets/DRIVER-DEPLOYMENT.md` — not deleted outright; moves per
   §3.6.

### 3.2 Workflow edits (3 files; engine-ci was done at P2)

- **forensic-repro.yml**: header :18 "rdacli liveness during the scan" removed;
  :20-21 machine-facts comment rewritten to the elevation posture; the CDP-driver
  mention at :103 stays (Chrome DevTools Protocol — de-noised false positive).
- **release.yml**: lane-order comment :4; test-signing header :8-15; the
  `-testsigning.cer` expected artifact :82; devsign verify of
  `stage-<v>\driver\rda.sys` :89-95; release-notes text :108-156 (mandatory
  driver section, bcdedit guidance, portable staged-driver copy). Full rewrite
  shape specified in §6 (P6 finalizes; P3 removes the driver references so the
  workflow is driver-free even if not yet re-run).
- **validation-evidence.yml**: header :9-16; WDK setup + `build.ps1 -Target
  driver` :44-56; the entire "Load the development-signed RDA driver" step
  :58-123 (cert import, install.ps1, `\\.\RDA` CreateFileW probe, service
  Running assertion) — replaced by an **elevation-facts step** (assert the
  harness process is elevated; record `whoami /groups` TokenElevation evidence;
  the T14 harness then measures the real user-mode transport directly —
  measure-cr-ui.c links only cr_ui.lib and needs no change).

### 3.3 Root + build files

- **build.ps1**: delete `Invoke-Driver` (:134-181), the `driver` ValidateSet
  member (:20), the `/DDRIVER_STAGE` define (:250), the `driver` switch case
  (:291), and the driver step in the `all` lane (:297 — `all` becomes contracts
  → engine → ui → installer → portable → checksums). Header comments :6-8 and
  the FILEVERSION note :14-16 rewritten (wire-visible versions note now covers
  the engine ABI number 4 only).
- **build.sh**: one-line comment :3 ("The FULL product (driver, installer,
  Tauri bundle) is Windows-only" → drop "driver,").
- **README.md**: :4-11 mandatory-driver paragraph (replaced by the user-mode +
  administrator-privilege truth), :25 merge-table driver row (records the
  removal), :33 layout bullet, :44-45 build-lane text.

### 3.4 contracts/ + installer/ + tools/

- **contracts/command-surface.json**: delete the `system_driver_status` row
  (:29). (The manifest as a whole is regenerated at P4; P3 only removes the
  driver row so no contract file references the driver.)
- **contracts/type-maps.md**: delete §10 DriverStatus (:207-209).
- **contracts/README.md**: delete the driver service-name constant (:47).
  (error-codes.json + ABI version were done at P2.)
- **installer/recoverywizard.nsi**: delete `SEC_DRIVER` (:281-484 — 7 File
  instructions, cert import, WOW64 System32 copy, sc create/start, Parameters
  block, abort path), the DRIVER_STAGE `!error` guard (:59-61), `DRIVER_SERVICE`
  (:137), DriverResult/DriverCertThumbprint vars (:169-170), the driver page
  (:185, :519-543), DESC_DRIVER (:508), DriverInstalled/DriverCertThumbprint
  registry values (:467, :471-473), the uninstall driver teardown + cert removal
  (:673-720), `RMDir /r "$INSTDIR\driver"` (:778). **Keep** SEC_APP, the -post
  section, kit-beside-exe layout, ProgramData sessions/logs, the
  inner-uninstaller choreography (BUILD-PIPELINE §3.1). Add a truthful
  "administrator required" install-time note (RequestExecutionLevel admin stays
  :164).
- **installer/portable.ps1**: drop the driver-stage hard-requirement (:44-47),
  the `driver\*` copy (:51-56), the driver README text (:63-87 → replaced with
  run-as-administrator + marker + WebView2 notes).
- **installer/devsign.ps1**: keep `new-cert/sign/verify` (payload-agnostic,
  still signs the engine kit + installer); delete `install-cert`/`remove-cert`
  (:188-218 — existed only for the driver load policy); de-brand the outputs
  (`rda-testcert.cer` → `devsign.cer` etc., :120-123, :200-201); header rewrite
  (:28-49).
- **installer/README.md**: driver flow/deviation rows removed (:41, :114).
- **tools/forensic/collect-facts.ps1**: drop rdaService/rdaServiceReg/
  driverFile/rdacli rows + the RDA System-log filter (:28-31, :34-36, :39); add
  `elevated` (TokenElevation of the probe shell), `integrityLevel`, and (post-P6
  tag) the installed app's requireAdministrator manifest fact.
- **tools/validation/installer-matrix.ps1**: new expected-module set (drop the
  7 driver files + driverCount==7 + DriverCertThumbprint + service-Running +
  cert-store + signed-rda.sys asserts :245-253, :261-276); uninstall row asserts
  files+registry+sessions only (:362-386).

### 3.5 docs/ (24 MANUAL-EDIT files per Agent A §3e)

Status-correcting edits, not deletions (audit trails preserved): ARCHITECTURE
(component diagram, install table, ERR_DRIVER_REQUIRED→elevation), BUILD (lanes,
WDK prerequisite, driver-ci reference), RELEASE (artifacts, embed-sign claim
:43), TESTING (driver CI rows :12-14), INTEGRATION-VALIDATION.md index (driver
T14 row :30 — the evidence subtree itself is untouched), INSTALLER,
INSTALL-FOOTPRINT-CURRENT (System32\drivers rows :70, :209),
INSTALL-FOOTPRINT-DELTA, INSTALLER-DECISION (driver SCM column),
INTEGRATION-GAPS (driver-status rows), PARITY-DELTAS (**D-FALL row :33 rewritten
to record the reversal: driver removed, user-mode elevation posture**),
FORENSIC-PROBE (dated addenda), MSIX-ASSESSMENT (conclusion re-evaluated: the
kernel-driver reason is gone; MSIX still declined for other reasons or reopened
— decision recorded with the edit), DEV-SIGNING (driver rows out; retained as
the installer/engine-kit signing record + successor note), PRODUCT-WIRING
(ERR_DRIVER_REQUIRED wiring :26 → ERR_ELEVATION_REQUIRED mapping),
REFERENCE-*/CORE-SURFACE/UI-SURFACE (light: corpus facts stay true; our-posture
claims corrected), INTEGRATION-PLAN.md historical (completion-state notes).

### 3.6 Context-sheet archiving (status-correcting, not deleted)

`git mv docs/context-sheets worklog/context-sheets` (all 6 files:
README, DRIVER-DEPLOYMENT, CORE-SURFACE, UI-SURFACE, REFERENCE-INSTALLER,
REFERENCE-LAYOUT — verified listing). Prepend to each a status banner:
"ARCHIVED <date>: frozen Phase-(-1) evidence of the DRIVER-ERA product. The
kernel driver was removed and the core advanced to ABI v4 in the consolidation
session — see worklog/INTEGRATION-PLAN.md. Preserved as audit trail." Update
references to the old path (docs/ index files, cross-links in the moved README).
docs/INTEGRATION-VALIDATION/ stays where it is (prior release evidence,
untouched).

### 3.7 Fresh-UI DriverStatus removal (P3's ui/ edit — arrives fully at P4)

- `ui/src/connector/contract.ts`: delete `DriverStatus` + `getDriverStatus()`
  (:573-605 area) from SystemDomain.
- `ui/src/queries/system.ts` + `ui/src/queries/client.ts`: drop
  `useDriverStatus`/`driverStatus` key.
- `ui/src/features/settings-about/{about-dialog,settings-page}.tsx`: drop the
  driver rows (About keeps name/version/connectorMode/engineVersion/platform).
- `ui/src/i18n/{en,zh-CN}.ts`: remove `about.driver*` keys (+ any orphaned
  `errors.driverRequired` — i18n-parity + i18n-sweep gates must stay green).
- `ui/src/connector/mock/mock-connector.ts`: delete the constant status
  (:1461-1468); `tests/connector-mock.test.ts`: drop the getDriverStatus pin.
- No Rust change (the fresh system.rs never had a driver probe).

### 3.8 Engine stale driver strings (final-prod-local cosmetic divergence, upstream note)

- `engine/CMakeLists.txt:5` DESCRIPTION "…on the RDA driver" → direct-access
  wording (identical stale string in both trees — verified by Agent A; we fix
  our copy, BF notes the upstream fix).
- `engine/include/cr/storage/cr_directalign.h:19` comment.
- `engine/bench/ours/run-ours.ps1:174` stale-label note.
- Sweep for any other `\brda\b|\\.\RDA|ERR_DRIVER_` in engine/ production code
  (core's own session-96 lint already guards the symbol classes; comments are
  the residue).

### 3.9 Grep-sweep protocol (gate §1 evidence)

Patterns (word-boundary-aware, Agent A's method): `\brda\b`, `rda\.sys`,
`rdacli`, `\\.\RDA`, `\bRDA\b`, `ERR_DRIVER_REQUIRED`, `testsigning`,
`bcdedit`, `DriverStatus`, `system_driver_status`, `DRIVER_STAGE`,
`Win32_System_Services` (the SCM feature). Scope: the whole tree minus the
**enumerated audit-trail exemptions**: `worklog.md`, `worklog/**` (incl. the
archived context sheets + this plan + the Phase 0 sheets, which quote driver
facts by design), `engine/diff-to-fix/**`, `engine/worklog*` (core's own
records), `docs/INTEGRATION-VALIDATION/**`, `ui/docs/historical/**` if present.
De-noised false positives on record: "affordance"/"yesterday" substring hits
(ui/src/lib/runtime.ts:5, ui/src/stores/filter-store.ts:10,78, ui/src/features/
results-explorer/filter-bar.tsx:178,185, ui/src/features/device-selection/
home-page.tsx:60, ui/src/theme/adopted/tokens.css:13,43,204,401,
ui/tests/health-truth.test.tsx:12, tools/forensic/drive-ui.mjs:241) and
"CDP driver" (forensic-repro.yml:103). Result recorded (patterns + zero-match
confirmation) in `worklog/RELEASE-VALIDATION/DRIVER-REMOVAL.md`.

**P3 file-ops tally:** 63 (driver/) + 1 (driver-ci.yml) + 3 (workflows) + 3
(root/build) + 3 (contracts) + 4 (installer) + 2 (tools) + 24 (docs) + 6 (git mv
context sheets) + ~8 (ui DriverStatus removal) + 3 (engine stale strings) ≈
**120 files**.

---

## 4. Phase 4 — UI rewiring (the real bridge on the fresh base, against ABI v4)

### 4.1 Architecture (ported from the proven v2 bridge, re-pinned to v4)

Same layer stack as UI-INTEGRATION-SURFACE §1 (the only pattern that satisfies
the engine's worker-thread callback contract):

```
features → queries (only consumer; arch-lint A1/A2 + ast-grep no-raw-invoke)
  → connector: contract.ts (extended) + tauri/tauri-connector.ts (real invoke/listen)
  → Tauri IPC (#[tauri::command]s in src-tauri/src/commands/*)
  → bridge/ (FFI + registries + sinks + consent + logging)
  → cr_ui.dll ABI v4 (static import link via cr_ui.lib; build.rs CR_UI_LIB_DIR)
```

**Bridge modules** (src-tauri/src/bridge/, rebuilt — the old abi.rs is ABI v2
and cannot be carried over):
- `abi.rs` — every `#[repr(C)]` mirror re-pinned 1:1 from
  `engine/include/cr/cr_ui.h` @ v4: `CR_UI_ABI_VERSION: u32 = 4`; the 9 changed
  structs re-derived (disk_info tail = trim_known/trim_on; event POD + autosave_rc
  tail; query/query_row M-101 tails; scan_config autosave/filter_folders tails;
  session_info label_full; apply_candidate M-137 tail; user_sign v3 widths) and
  the 4 new structs (session_row, archive_entry, thumbnail_caps,
  thumbnail_info). The mangled log-seam extern (§4.4).
- `state.rs` — scans/jobs/consents registries, `SendPtr`, NodeMeta cache, the
  enforced close order (event-fn unsub → sink box → snapshot cache → scan/
  session → device), sessions-dir + portable marker probe (NG-18: marker file,
  never the ProgramData env var).
- `error.rs` — BridgeError `{code, bridgeCode, message}`; ERR_CANCELLED is a
  status, never an error.
- `events.rs` — sink = one `try_send` POD copy into bounded sync_channel (512
  scan / 64 job), single emitter thread per handle, coalesced telemetry
  snapshot, monotonic overall clamp, unknown event types ignored, HEARTBEAT
  never fabricates progress; run/process threads latch terminal state
  (single-writer). + the two new event types (11 load-progress, 12 autosaved).
- `consent.rs` — the G-1(a) machinery ported verbatim-in-design: echo-token
  requestIds (`consent-N`), gate registered with the registry lock RELEASED
  before the run thread blocks, no user-facing timeout, cancel force-denies
  FIRST, run-tail hygiene close, re-hydration through scan_state.pendingConsent.
  **Extended to two consent families**: `cr_ui_repair_consent_fn` (pre-scan
  repair) and `cr_ui_apply_consent_fn` (lost-partition apply) — same gate
  design, separate registries keyed by handle.
- `logging.rs` — writer thread + channel + rotation (1 MiB, one generation) to
  ProgramData/logs (portable: beside exe); init FIRST in run() before the ABI
  gate. Engine lines ride the **mangled C++ seam** (§4.4).
- `util.rs` — two-call helpers, FILETIME→ISO (exact `Date.toISOString()` shape),
  OwnedCrString, TRUNCATED exact-size retry.
- `filefilter.rs` — GENERATED from `engine/src/core/query/cr_filefilter_data.cpp`
  (1,481 entries — verified present in core) by a ported
  `ui/scripts/gen-filefilter.mjs` with `--check` freshness gate.
- `mod.rs`.

**Commands** (src-tauri/src/commands/): devices, scan, results, preview,
sessions, recovery, health, repair, system (+ mod). lib.rs: startup ABI gate
(logs, does not abort) + per-scan hard refusal on `cr_ui_abi_version() != 4`.

### 4.2 Command manifest (contract-op ↔ Tauri command ↔ cr_ui v4 exports)

Base set = the fresh contract's 33-34 commands; extensions marked **[NEW]**;
internal-only marked [int]. Threading classes as the old manifest (sync |
spawn-join | async-job).

| Domain | Contract op | Tauri command | cr_ui v4 exports used |
|---|---|---|---|
| devices | listDevices / refreshDevices / getDeviceInfo | `devices_list` / `devices_refresh` / (client-derived from list) | `enumerate_disks`, `enumerate_volumes`, `open_disk`+`open_volume` (probe), `device_close`, `device_geometry` |
| scan | startScan | `scan_start` | `abi_version`, `open_volume` / `open_disk` / **`open_volume_with_key`** (fvek param), `scan_create`, `scan_attach_device`, `scan_set_event_fn` |
| | pauseScan/resumeScan/cancelScan | `scan_pause`/`scan_resume`/`scan_cancel` | `scan_pause/resume/cancel` (+ busy/stopping/telemetry flags); cancel force-denies consents FIRST |
| | getScanState | `scan_state` | `scan_busy`, `scan_stopping`, `scan_telemetry` (+ pendingConsent, lostCandidates re-hydration) |
| | — [int] | `scan_close` | registry Drop (close order); no UI surface (fresh contract dropped it) |
| | — [NEW] save-cancel | `scan_save_cancel` | `scan_save_cancel` (N-060 save-scoped cancel for the save spinner) |
| | onScanProgress | (event `scan-progress:<id>`) | `scan_set_event_fn` sink → channel → emit |
| results | getResultTree/queryResults/countResults/getNodeDetail | `results_tree`/`results_query`/`results_count`/`node_detail` | `scan_snapshot`/`session_snapshot`, `snapshot_node_count/_partial/_node/_dfs` via NodeMeta cache (G-7 port; one filter impl for live+session) |
| preview | requestPreview | `preview_open` | `preview_open_snapshot/_open_session`, `preview_size/_read/_metadata/_unreadable`, `preview_close` (transient handles; ≤4 MiB chunks; damage = typed refusal) |
| | getThumbnail | `preview_thumbnail` | **Session-kind: `thumbnail_generate` (two-call probe→alloc→generate) + `thumbnail_format_supported`**; Live-kind: preview-read assembly (documented degradation; BF-13) |
| | — [NEW] archive browse-inside | `preview_archive` | **`archive_entry_count`, `archive_entry_at`** (M-116; 512-entry bound) |
| sessions | saveSession | `session_save` | `scan_save_session`, `scan_get_stats` (+ save-progress events) |
| | listSessions | `sessions_list` | **`session_list`** (W1a light records, newest-first — replaces the per-file load loop) |
| | loadSession | `session_load` | **`session_load_with_events`** (progress + caller-owned cancel), `session_get_info`, `session_stats`, best-effort re-attach by model+serial |
| | getSessionMeta | `session_meta` | `session_get_info`/`session_stats` (from the registry) |
| | — [NEW] resume last | `session_resume_last` | **`session_resume_last`** (M-154; ERR_RANGE_OUT_OF_BOUNDS → fall back to list) |
| | — [NEW] resume incomplete | `session_resume` | **`session_resume`** + attach + **`scan_verify_identity`** (boot-hash; ERR_BAD_CHECKSUM surfaced) → run |
| recovery | startRecovery/cancel/getState/getItems/getReport | `recovery_start`/`recovery_cancel`/`recovery_state`/`recovery_items`/`recovery_report` | `recovery_create/_set_destination/_enqueue_node/_subtree/_enqueue_snapshot/_subtree_snapshot/_queue_count/_process/_cancel/_get_stats/_report_json`; selection `all` pages the bridge query at 500/page |
| | onRecoveryProgress | (event `recovery-progress:<id>`) | progress fn sink + stats/report drain (2 s poll) |
| health | getHealth | `health_read` | `health_read` (base only — no ex2 on v4; thresholds BF-03) |
| repair | — [NEW] consent respond | `consent_respond` | none (registry + Condvar; sync command) |
| | dryRun/apply/undoLog/undo (workbench) | `repair_dry_run`/`repair_apply`/`repair_undo_log`/`repair_undo` | typed `not-implemented` refusals (no standalone ABI repair surface — CORE: repair exists only in-scan; BF-06) |
| lost-partition [NEW] | applyLostPartition | `lost_partition_apply` | **`apply_lost_partition`** (whole-disk view; consent callback; `apply_result.outcome` surfaced verbatim) |
| | undoLastApply | `lost_partition_undo` | **`apply_undo_last`** |
| | saveUndo | `lost_partition_save_undo` | **`apply_save_undo`** |
| system | getAppInfo | `system_app_info` | none (package info; connectorMode 'tauri'; engineVersion = `abi_version().to_string()` — contract number, not product version, G-6) |
| | — [NEW] security posture | `system_security_posture` | none — Win32 TokenElevation probe (mirrors core `win::is_elevated`, include/cr/win/cr_elevate.h:23): `{ administrator: bool }` replaces getDriverStatus |
| | — [NEW] relaunch elevated | `system_relaunch_elevated` | none — ShellExecuteW "runas" on own exe + exit (the host-triggers-its-own-UAC contract, cr_ui.h:322-325) |
| | validateDestination | `system_validate_destination` | `enumerate_volumes` (+ same-letter overlap; engine backstop 0x50050001) |
| | pickDirectory | `system_pick_directory` | tauri-plugin-dialog blocking_pick_folder on the blocking pool |
| | listMountPoints | `system_list_mounts` | `enumerate_volumes` + GetDiskFreeSpaceExW |
| | openPath | `system_open_path` | tauri-plugin-opener (reveal with selection) |
| unlock [NEW] | unlockVolume (dictionary) | `volume_unlock_dictionary` | **`unlock_volume_with_dictionary`** (N-064 explicit-consent; user-initiated dialog only) |
| user-signs [NEW] | loadUserSigns | `system_load_user_signs` | **`register_user_signs`** (atomic replace) + `user_sign_count/_info` for the About count; startup auto-load of exe-adjacent `userdata-defaults.json` (bridge init reads + registers — the CLI does this at rrecov.cpp:1346-1383; the DLL does NOT auto-load) |

Events (per-handle scoped): `scan-progress:<id>`, `scan-finished:<id>`,
`save-progress:<id>`, `lost-candidate:<id>`, **`consent-request:<id>` [NEW]**,
`recovery-progress:<jobId>`, `recovery-finished:<jobId>`, **`session-load-progress:
<path>` [NEW]** (CR_EVT 11), **`scan-autosaved:<id>` [NEW]** (CR_EVT 12 →
state refresh). Autosave wiring: `scan_config.autosave_dir_utf8` = the sessions
dir (W1c) so completions auto-save; the auto session feeds
`session_resume_last`.

### 4.3 Contract extensions (contract.ts, op-by-op and type-by-type)

1. **Error vocabulary**: +`'elevation-required'` (+ `device-disconnected`
   already covered by `device-gone`). `error-codes.json` re-points
   `ERR_ELEVATION_REQUIRED → elevation-required`; `ERR_DEVICE_DISCONNECTED →
   device-gone`.
2. **Consent surface [NEW]** (ported from final-prod's G-1(a) additions,
   UI-INTEGRATION-SURFACE §3.3): `ConsentRequestEvent { scanId, requestId,
   plan: { detected, fsFamily, actionCount, refused, reason, advisoryCount } }`;
   `onConsentRequest(scanId, cb) → unsubscribe`; `respondToConsent(requestId,
   allow)`; `ScanState.pendingConsent` re-hydration field; ConnectorErrorCode
   `consent-not-found` re-added (bridgeOnly). Scan page renders the
   non-dismissable modal (Escape/outside-click prevented; Approve disabled when
   refused or zero actions; stale-id clears).
3. **Lost-partition apply [NEW]**: `LostPartitionCandidate` (from scan state:
   lba, count, fs family), `ApplyConsentPlan { refused, scope, startLba,
   sectorCount, mbrType, slot, regenerateSignature, reason }`,
   `onApplyConsent(candidateId, cb)` + `respondToApplyConsent`, `applyLostPartition(
   deviceId, candidate)` → outcome verbatim (0 applied … 5 error),
   `undoLastApply(deviceId)`, `saveApplyUndo(deviceId, path)`. The
   lost-partition screen (S2) gains the apply flow behind two-factor consent.
4. **Unlock [NEW]**: `unlockVolumeWithPasswords(volumeId, passwords[])` (typed
   consent-gated call; ERR_FVE_LOCKED on failure — candidate index only, never
   the password), `ScanParams.bitlockerFvek?: string` (64/128 hex) consumed by
   `scan_start` via `open_volume_with_key`.
5. **Elevation [NEW]**: `getSecurityPosture(): { administrator: boolean }` +
   `relaunchElevated(): void`; the Home permission-denied banner (existing
   `home.permissionDenied` copy) becomes the primary not-elevated signal with a
   "Restart as administrator" action; a startup posture query primes it.
6. **Thumbnails**: `getThumbnail` semantics documented (server-side generation
   for sessions; preview-read fallback live; null + `truncated` flag on decode
   refusal).
7. **Archive preview**: `PreviewPayload` union + `'archive' { entries:
   {name, size, compressed, crcPresent, nameTruncated}[], truncated }`.
8. **Session resume**: `resumeLastSession(): {scanId}`, `resumeSession(path)`
   on the Backup screen for incomplete sessions (identity-mismatch error
   surfaced via `session-corrupt` + detail).
9. **Recovery options truth**: map `RecoveryOptions` (collision rename/skip/
   overwrite → `cr_ui_recovery_config.conflict`; rebuildFolders →
   skip_dir_reconstruct; verifyOnWrite → skip_hash_verify; keepEmptyFolders/
   fileRepair have NO engine equivalent — truthful-disable with reasonKey,
   BF-14).

### 4.4 The log seam (NG-15 successor)

Core exports the C++ seam via the MSVC def file — **verified**:
`src/ui/cr_ui_exports.def:19` exports
`?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z` =
`void cr::log::set_sink(void (*)(cr::log::Level, const char*, void*), void*)`
(signature verified against include/cr/cr_log.h:55-59). The bridge declares:

```rust
type CrLogFn = unsafe extern "system" fn(level: u32, line: *const c_char, user: *mut c_void);
#[link_name = "?set_sink@log@cr@@YAXP6AXW4Level@12@PEBDPEAX@Z2@Z"]
unsafe extern "system" { fn cr_log_set_sink(fn_: Option<CrLogFn>, user: *mut c_void); }
```

Sink discipline: fires on ANY engine thread; one `try_send` into the logging
channel; levels 0/1/2 = Error/Warn/Info. (Linux dynsym reachability is
irrelevant — the product links the MSVC import lib.) **BF-01** files the
upstream recommendation to re-land a flat-C `cr_ui_set_log_sink`.

### 4.5 Error mapping table (v4 → ConnectorError, complete 40-code registry)

| Engine code | value | uiCode (final) | Notes |
|---|---|---|---|
| ERR_OPEN_FAILED | 0x50020001 | device-permission | refined device-gone when fresh enumeration lacks it |
| ERR_READ_FAILED | 0x50020002 | recovery-item-failed | scan-time = badSectorMap events, never errors |
| ERR_WRITE_FAILED | 0x50020003 | recovery-item-failed | |
| ERR_RANGE_OUT_OF_BOUNDS | 0x50020004 | unknown | also session_resume_last dead pointer → fall back to list |
| ERR_CANCELLED | 0x50020005 | null (status) | never an error |
| ERR_RETRY_EXHAUSTED | 0x50020006 | recovery-item-failed | |
| **ERR_DEVICE_DISCONNECTED** | **0x50020007** | **device-gone** | v2 meaning (driver-required) is GONE; reattach-then-resume offer (BF-07) |
| ERR_BAD_MAGIC | 0x50030001 | session-corrupt | session-load context |
| ERR_BAD_CHECKSUM | 0x50030002 | session-corrupt | resume identity mismatch |
| ERR_BAD_PARAMETER | 0x50030003 | scan-scope-invalid / destination-invalid | by failing-param refinement |
| ERR_TRUNCATED | 0x50030004 | unknown | bridge bug if seen (two-call protocol) |
| ERR_FIXUP_FAILED | 0x50030005 | repair-failed | |
| ERR_CYCLE_LIMIT / ERR_DEPTH_LIMIT | 0x50030006/7 | unknown | informational; scan completes partial |
| ERR_UNSUPPORTED | 0x50030008 | health-unavailable / preview-unsupported / not-implemented | by call-site |
| ERR_NO_MEMORY | 0x50030009 | unknown | |
| ERR_SESSION_FORMAT/IO/VERSION | 0x50040001-3 | session-corrupt | missing file pre-checked → session-not-found |
| ERR_DEST_OVERLAP | 0x50050001 | destination-overlap | F12; v0.7.1 arms only true same-disk |
| ERR_DEST_UNWRITABLE | 0x50050002 | destination-unwritable | |
| ERR_HASH_MISMATCH | 0x50050003 | recovery-item-failed | report hash_status |
| ERR_DEST_FULL | 0x50050004 | out-of-space | writer fast-fails queue |
| ERR_DEST_NAME_EXHAUSTED | 0x50050005 | recovery-item-failed | |
| ERR_DEST_PATH_TOO_LONG/INVALID/NETWORK | 0x50050006-8 | destination-invalid | M-074 taxonomy umbrella (BF-15 for distinct reasons) |
| ERR_FVE_LOCKED | 0x50060001 | device-permission | volume status 'locked' + unlock flow |
| ERR_FVE_ELEVATION_REQUIRED | 0x50060002 | device-permission | distinct reasonKey (FVE store ACL) |
| ERR_HEALTH_UNAVAILABLE | 0x50070001 | health-unavailable | |
| ERR_REPAIR_REFUSED/UNDO_FAILED/VOLUME_LOCK_FAILED | 0x50080001-3 | repair-failed | |
| ERR_APPLY_CONSENT_DENIED | 0x50080004 | repair-consent-required | |
| ERR_APPLY_STALE/REFUSED/REFRESH_FAILED | 0x50080005-7 | repair-failed | A2/A3/A4 outcomes also verbatim in apply result |
| ERR_IMAGE_CORRUPT | 0x500A0001 | unknown | no image surface (BF-12) |
| ERR_INTERNAL | 0x50090001 | unknown | |
| **ERR_ELEVATION_REQUIRED** | **0x500C0001** | **elevation-required [NEW]** | banner + relaunch-as-admin; `cr_ui_last_transport_error()` advisory twin |

### 4.6 NG-21 bridge-side guard (verified feasibility)

**OQ2 answer (verified in code):** ABI v4 exposes **no standalone repair/check
entry point** — repair exists only as (a) the scan's `pre_scan_repair` hook,
whose `repair::analyze(slice, rplan)` runs on **the scan's whole slice**
(core src/core/orchestrator/cr_scan.cpp:1190-1192), and (b) the lost-partition
apply family. Partition scoping exists **only at the CLI layer** — `rrecov fix
--disk N --part P` slices the device before analyze and rebases the plan before
execute (src/app/rrecov.cpp:3742-3746, 3770-3778). Core's `analyze()` has **no
partition-table guard** (grep for EFI PART/0xEE/protective: empty;
cr_rawfixer.cpp:543+ is boot-signature family detection only).

**Bridge policy (encoded):**
- `pre_scan_repair = 1` armed **only when the scan target resolved to a volume
  slice** (`cr_ui_open_volume` / quick targets / FVEK unlock) — analysis is then
  naturally partition-scoped (the slice's LBA 0 is the volume boot sector).
- Whole-disk targets (`whole_disk = 1`, lost-partition discovery, disk cards):
  `pre_scan_repair = 0` — **repair analysis is truthfully unavailable for
  whole-disk targets** (surface copy + capability-map row); the hazard (an
  unguarded LBA-0 plan on a healthy table) is closed at the bridge.
- `pre_scan_check = 1` always (M-041 read-only damage audit is safe on any
  slice).
- **BF-06** files the engine-side re-port (per-partition scoping in
  cr_scan.cpp's whole-disk path + the partition-table primary guard in
  cr_rawfixer.cpp — GPT protective 0xEE / "EFI PART" at LBA1 / plausible MBR ⇒
  refuse), with the S3 forensic incident (run 40470191039) as the standing
  justification.

### 4.7 Capability map (every capability → disposition)

Legend: **SURFACE** = wired to a real UI surface this session; **DISABLE** =
truthful-disable with documented reason (+ BEHAVIOR-FINDINGS entry). From
Agent D §3.2 (+14) + Agent B §6.3 (25 unused v2) + the fresh UI's screens.

| # | Capability (v4 exports) | Disposition | Where / why |
|---|---|---|---|
| 1 | Lost-partition apply + undo (`apply_lost_partition`, `apply_undo_last`, `apply_save_undo`) | **SURFACE** | S2 lost-partition screen exists; consent-gated apply + undo + undo-file save |
| 2 | Sessions: list/load/save (`session_list`, `session_load_with_events`, `scan_save_session`, `save_cancel`) | **SURFACE** | Backup screen + results save spinner (cancel wired) |
| 3 | Session resume (`session_resume`, `resume_last`, `verify_identity`, `last_autosave_path`) | **SURFACE** | Backup "resume" on incomplete + Home resume-last; identity mismatch surfaced |
| 4 | Autosave (`autosave_dir_utf8` + CR_EVT_AUTOSAVED) | **SURFACE** | completion auto-save to the sessions dir (W1c) |
| 5 | Thumbnails (`thumbnail_generate`, `thumbnail_format_supported`) | **SURFACE** | sessions: real generation (EXIF-oriented, grade-promoting); live scans: preview-read fallback (BF-13) |
| 6 | Archive browse-inside (`archive_entry_count/_at`) | **SURFACE** | preview pane 'archive' payload (M-116) |
| 7 | Dictionary unlock (`unlock_volume_with_dictionary`) | **SURFACE** | locked-volume unlock dialog (explicit consent; N-064) |
| 8 | FVEK unlock (`open_volume_with_key`) | **SURFACE** | same dialog, advanced input; scan_start param |
| 9 | Consent surface (repair + apply consent callbacks) | **SURFACE** | non-dismissable modal + re-hydration + force-deny (ported G-1(a)) |
| 10 | Elevation posture (bridge TokenElevation + relaunch) | **SURFACE** | replaces DriverStatus; banner + restart-as-admin |
| 11 | Query filters/sort/paging over real rows (snapshot enumeration + NodeMeta) | **SURFACE** | G-7 port; one impl for live+session; date/size presets bridge-computed |
| 12 | Recycle-bin / Desktop quick targets | **SURFACE** | resolve_target virtual quick-targets port (`vol-recycle-bin` → `<SystemDrive>:\$Recycle.Bin` implicit scope) |
| 13 | User-taught signatures — defaults + file import (`register_user_signs`, `user_sign_count/_info`) | **SURFACE** | startup `userdata-defaults.json` load + Settings import-file action; count in About |
| 14 | Health (`health_read` base) | **SURFACE** | S6 health page; threshold column degraded (BF-03) |
| 15 | Pre-scan repair consent (volume targets) | **SURFACE** | consent modal; whole-disk targets disabled (§4.6) |
| 16 | Carve format catalog (`carve_format_count/_info`) | **SURFACE** (lane C) | `scan.listCarveFormats()` — the og's EUGetAnalyserList surface; no dedicated screen yet (data surface for the results/diagnostics consumers); the corpus citation: RAWSearchDLL exports (rawsearch-carving--deep-dive.md §1) |
| 17 | Live carve tally (`scan_carve_tally_count/_tally`) | **SURFACE** (lane C) | `scan.getCarveTally(scanId)` — the og's GetRawUserInfoMap (the found-N-.jpg report, DRWUI :227538); no dedicated screen yet (the data surface feeds the completion report) |
| 18 | Disk map (`disk_map`, `disk_map_free`) | **SURFACE** (lane C) | SectorMapPanel on the scan page — the AnalysisGridWidget parity (UIUX/08:11): four overlays + bad ranges, cell-budget bounded, collapsible |
| 19 | Image create (`image_create`) | **SURFACE** (lane C) | CreateImageDialog from the home page — the reference's own DiskBackup chain placement (strings:14534); bridge pre-gates (same-disk overlap, byte-for-byte space); NO cancel (the ABI exports none — PARITY-DELTAS) |
| 20 | fsck escalation (`fsck_volume`) | **SURFACE** (lane C) | `maintenance.runFsck` — the check/fix posture, hidden vehicle (CREATE_NO_WINDOW), raw exit code + final i18n verdictKey; no dedicated screen yet (the connector surface is the escalation rung for the defect-screen consumers) |
| 21 | BitLocker off (`turn_off_bitlocker`) | **SURFACE** (lane C) | `maintenance.turnOffBitlocker` — the destructive decrypt twin, explicit opt-in only, accepted-not-plaintext contract; no dedicated screen yet (gated for the SD/unlock surfaces' consumers) |
| 22 | TRIM chip at enumerate (`enumerate_disks_ex`) | **DISABLE** | one fsutil spawn per refresh; health already carries trim truth |
| 23 | Image targets (`open_image`) | **DISABLE** | no image-file scan surface (would also enable unelevated scans — future feature, BF-12) |
| 24 | Core-side query engine (`query_run`, `query_rows_free`, `query_apply_buckets`) | **DISABLE** | bridge-side filter implemented over real rows; engine-side adoption needs T14 cost re-measure (BF-16) |
| 25 | Assistant stats + regrade (`scan_assistant`, `session_regrade`) | **SURFACE** (lane C) | RecoverabilityCard on the completion celebration (the S-023 verdict + gauge + category row — UIUX/30:19) + `results.regradeNodes` (the per-selection re-grade) |
| 26 | Transport diagnostics (`device_transport`, `last_transport_error`) | **SURFACE** (lane C) | `system.getDeviceTransport(deviceId)` — the [transport] truth + the M-030 advisory; no dedicated screen (the diagnostics axis for error surfaces) |
| 27 | Checkpoint count (`scan_checkpoint_count`) | **DISABLE** (internal) | checkpoints are internal; autosave surfaces instead |

**Counts: 23 SURFACE / 4 DISABLE** (27 capability rows; the lane C
API-expansion session flipped rows 16-21, 25-26 — every flip backed by a
corpus citation contradicting the original "no reference screen"
rationale; the evidence table lives in worklog/lane-c/API-EXPANSION.md).
Still DISABLE: 22 (the TRIM chip — deferred to lane D's green per
COORDINATION), 23 (image targets — the `img-` scan-target arm, same
deferral), 24 (core-side query engine — the T14 re-measure posture
stands). Row 27 (checkpoint count) now crosses as a flagged companion on
`scan_telemetry` (internal axis, surfaced as data not screen).

### 4.8 i18n + mock demotion + test strategy

- **i18n**: every new surface gets en + zh-CN keys (parity 1:1 enforced by
  tests/i18n-parity.test.ts; sweep must find zero unused). New key groups:
  elevation (banner, restart-as-admin, posture row), consent (modal title/body/
  actions/plan rows), lost-partition apply (plan summary, outcomes 0-5, undo),
  unlock (dialog, FVEK hint, failure), archive preview, session resume, save
  cancel, user-signs import, truthful-disable reasons for the DISABLE rows the
  UI renders (repair workbench not-implemented copy already exists).
- **Mock demotion**: `src/connector/index.ts` default flips — production default
  `tauri`; explicit `VITE_CONNECTOR=mock` for: the `dev` script (browser dev),
  vitest (pin via vite.config `test.env` / setup), e2e builds (ui-e2e.yml +
  ui-smoke-style step), and the tauri-debug-build job. The mock directory gets a
  header banner: TEST DOUBLE — never the shipped backend. `AppInfo.connectorMode`
  remains the truth surface (About renders it).
- **Tests (per suite)**:
  - `tests/connector-tauri.test.ts` [NEW, ported]: mocks `@tauri-apps/api/
    core`+`/event` at module boundary; asserts every contract method invokes the
    exact manifest command with exact args; error normalization (bridgeCode →
    reasonKey; numeric code → ENGINE_CODE_MAP incl. 0x500C0001 →
    elevation-required, 0x50020007 → device-gone); subscriptions return the
    unsubscribe shape.
  - `tests/connector-mock.test.ts`: keeps 27 mock-double tests (labeled); the
    "Tauri stub adapter throws" test (:563-567) is **re-authored** — the Proxy
    stub shrinks to nothing; the test converts to "the tauri connector exposes
    every domain method as a function" (wired-domains enumeration per Agent F
    §9 note).
  - `tests/node-space.test.ts` (11): unchanged (tests the double itself).
  - Component tests: unchanged (mock-pinned via vitest env).
  - `e2e/consent.spec.ts` [NEW, ported]: deny-completes, re-hydration after
    navigate-away, stale double-respond (runs against the mock double with an
    injected consent scenario — the mock gains a consent scenario hook).
  - Rust: `cargo test` in src-tauri for bridge units (close order, gate
    semantics, error mapping, two-call helpers); the T14 #[ignore] filter-cost
    and NodeMeta-memory tests re-landed.
  - `contracts/validate.mjs` phase 2 activates (bridge/abi.rs lands) — the
    regenerated manifest + every TS invoke cross-checked; `removed[]` documents
    `system_driver_status` and `scan_close`→internal.
- **contracts/ regeneration (same push as the bridge)**: command-surface.json
  (the ~45-command manifest + events + removed[]), error-codes.json (final
  vocabulary incl. elevation-required, consent-not-found), event-mapping.md
  (1..12 + marshaling rules), type-maps.md (v4 struct maps incl. the 9 changed
  + 4 new), README.md (constants: sessions/logs dirs, portable marker, ABI 4).
  gen-filefilter.mjs ported + regenerated filefilter.rs; the
  `filefilter:check` steps reinstated in contracts-ci.yml + ui-ci.yml +
  package.json.

### 4.9 Lifecycle / cancellation / threading (encoded rules)

- run()/process() on dedicated threads; token commands from spawn_blocking;
  one sink per scan; subscribe before run (ERR_BAD_PARAMETER mid-run);
  callbacks never block (one try_send); unknown event types ignored; terminal
  state latched by the run/process thread only.
- Cancel: `scan_cancel` force-denies pending gates FIRST; recovery cancel
  leaves committed items committed (report truth); save has its own scoped
  cancel (`scan_save_cancel`); session load takes the caller-owned cancel flag.
- Teardown order: preview → snapshot (after last row read) → recovery → scan →
  session → device; device outlives borrowers; `ScanEntry::Drop` enforces.
- Elevation retry: 0x500C0001 → banner + relaunch (never silent, never fatal);
  image/session paths stay usable unelevated (contract §1).

**P4 file-ops tally:** ~10 bridge modules + ~10 command modules + lib.rs +
build.rs + Cargo.toml + Cargo.lock + tauri-connector.ts + not-implemented.ts +
contract.ts + index.ts + ~6 queries + ~8 features + 2 i18n + mock banner +
~6 test files + gen-filefilter.mjs + package.json + 5 contracts files ≈ **55-60
files**.

---

## 5. Phase 5 — Workflows & CI finalization

Per-workflow final state (all push-triggered lanes re-verified against the
final tree; SHA pinning applied to every surviving `uses:`):

| Workflow | Final state |
|---|---|
| contracts-ci.yml | phase 1 + phase 2 (bridge present) + filefilter freshness (reinstated). Node 24. |
| engine-ci.yml | core checks.yml posture (P2) + SHA pins. |
| ui-ci.yml | fresh CI posture (P1) + `filefilter:check` reinstated (P4) + SHA pins. |
| ui-e2e.yml | fresh smoke shape (P1) + SHA pins. |
| forensic-repro.yml | driver-free (P3); P5: default `release_tag` input → v0.7.0; evidence upload unchanged; SHA pins. |
| release.yml | rewritten per §6 (P3 removes driver refs; P6 finalizes notes/artifacts); SHA pins incl. softprops/action-gh-release. |
| validation-evidence.yml | T14: engine kit only (no driver lane/load); bridge measurements via cargo `--ignored`; T15 matrix re-baselined (5-file module set); SHA pins. |
| driver-ci.yml | **deleted** (P3). |

**SHA-pinning list** (all 52 current `uses:` are tag-pinned — BUILD-PIPELINE
action-pinning note): `actions/checkout@v4`, `actions/setup-node@v4`,
`actions/upload-artifact@v4`, `actions/cache@v4` (new, from core's checks.yml),
`dtolnay/rust-toolchain@stable`, `ilammy/msvc-dev-cmd@v1` (if retained by
validation-evidence's harness compile), `microsoft/setup-msbuild@v2`,
`nuget/setup-nuget@v2` (only if any lane still needs msbuild — after the driver
goes, engine uses cmake; likely removable), `pnpm/action-setup@v4`,
`softprops/action-gh-release@v2`. Each pinned to the full 40-hex SHA of the
tag's commit, with a comment recording the tag.

**Benchmark posture:** final-prod keeps NO engine benchmark lane in CI (core's
benchmark-src/test-benchmark run on core's own repo; the v0.7.1 direct-vs-
driver ledger is core's evidence). The final-prod bench scripts under
`engine/bench/` ride with the merge (minus verify-driver.ps1, already absent
upstream). Decision recorded in docs/BUILD.md.

**Also in P5:** docs/BUILD.md + docs/TESTING.md CI tables updated to the final
lane set; `.github/workflows` YAML parse discipline (adopt core's inline-block
AST gate idea into engine-ci at P2 already); `worklog/RELEASE-VALIDATION/`
skeleton created.

**P5 file-ops tally:** ~8 workflows + 2 docs + skeleton ≈ **11 files**.

---

## 6. Phase 6 — Release & validation (v0.7.0)

1. **Version + tag**: VERSION already 0.7.0 (P1); lockstep re-verified (4
   files + Cargo.lock package version). Tag `v0.7.0` on the green tree;
   release.yml version gate (tag == `v<VERSION>`) passes.
2. **release.yml shape**: keep tag trigger, version gate, contracts gate
   (phase 1+2), `build.ps1 -Target all` (now contracts → engine → ui →
   installer → portable → checksums). Artifact set: `RecoveryWizard-0.7.0-
   setup.exe`, `-portable.zip`, `SHA256SUMS` — **no `-testsigning.cer` asset**
   (no kernel code-integrity gate exists to satisfy). **Honest signing posture
   (decision):** devsign `new-cert`+`sign`+`verify` continues for the engine kit
   (cr_ui.dll, rrecov.exe) and the setup.exe — self-signed, documented as such;
   the notes state plainly: user-mode binaries, no kernel signatures, SmartScreen
   will warn on the unsigned-by-a-CA installer, no certificate import is needed
   or performed, uninstall removes no certs. (devsign.ps1's install/remove-cert
   died at P3.) Notes outline: (a) what the product is now — user-mode data
   recovery, raw-disk access requires running as administrator (elevation
   banner + relaunch flow in-app); (b) what changed — kernel driver REMOVED
   (the 0.6.x mandatory-driver posture is dead), engine advanced 1092 commits
   to ABI v4 (direct `\\.\PhysicalDriveN` transport, ERR_ELEVATION_REQUIRED
   refusal contract), UI re-based to recovery-wizard-ui-new v3 and rewired to
   the real engine (no mock backend in shipped builds); (c) artifacts + hashes;
   (d) validation evidence links.
3. **forensic-repro.yml redesign** (validates the tagged release): S1-S4
   mechanics unchanged (install probe, seeded 64 GB NTFS + 8 GB exFAT VHDXs,
   CDP-driven UI exercise incl. the consent gate answered Deny). Facts layer
   swapped: drop all RDA service/file/cert/rdacli probes; add elevation facts
   (app manifest requireAdministrator, launched-process TokenElevation, the
   S3 scan run proves the elevated path end-to-end) + `connectorMode == 'tauri'`
   assertion (the no-mock gate) + an **unelevated-launch probe** (launch the
   app with a filtered token; expect the elevation banner + refusal — the
   product-side twin of core's exit-4 probes).
4. **validation-evidence.yml redesign**: T14 measures the direct transport
   (kit-only build; no WDK/driver steps); bridge filter/memory measurements
   against the real NodeMeta port. T15 matrix asserts the driver-free install
   footprint. Both write to artifacts; hand-curated into
   docs/INTEGRATION-VALIDATION/ per the existing convention.
5. **Evidence capture plan** (worklog/RELEASE-VALIDATION/):
   - `DRIVER-REMOVAL.md` — the P3 grep-sweep protocol + zero-match confirmation.
   - `UI-WIRING.md` — per-surface validation (forensic S1-S4 artifacts + the
     seam-test inventory) — gate §6.
   - `CAPABILITY-MAP.md` — §4.7 table exported with per-row evidence — gate §7.
   - `RELEASE-RUN.md` — release workflow run link, install/launch/exercise/
     uninstall on a clean Windows runner (T15 + forensic artifacts) — gate §8.
   - `CORE-MERGE.md` — merge SHA + `git log --follow` lineage proofs — gate §2.
   - `BUILDS.md` — CI green links + local build logs — gates §3/§4/§5.

**P6 file-ops tally:** release.yml + forensic-repro.yml + validation-evidence.yml
final edits + tag + ~6 evidence files ≈ **9-10 files**.

---

## 7. BEHAVIOR-FINDINGS seed list (worklog/BEHAVIOR-FINDINGS.md)

| ID | Finding | Precise scope |
|---|---|---|
| BF-01 | No flat-C log sink on core v4 | Bridge binds the mangled `?set_sink@log@cr@@…` (cr_ui_exports.def:19). Upstream: re-land `cr_ui_set_log_sink` flat-C wrapper so hosts don't link C++ mangles. |
| BF-02 | `bus_type` absent on v4 disk_info | G-9/NG-14 SD-vs-USB detection degrades to `removable` (usb/sd merged). Upstream: re-append bus_type or expose an encoding of `removable`+layout. |
| BF-03 | SMART attr thresholds absent (no `health_read_ex2`) | Health table shows value/worst/raw/status; threshold column truthful-disabled. Upstream: re-land the ex2 wrapper + `cr_ui_smart_attr_ex`. |
| BF-04 | `query_trim_state` still fsutil `_popen` on core (cr_health.cpp:301-324) | NG-8 console-flash class regressed upstream; native IOCTL re-port recommended. Bridge unaffected (no console in Tauri). |
| BF-05 | No mid-sweep progress ticks in core's RemnantCtx (cr_mftwalk.cpp:1748-1759) | NG-3 re-port onto the heavily-evolved file; deep-stage progress remains stage-level until then. |
| BF-06 | **NG-21 engine-side guard missing (safety-critical)** | Core `analyze()` has no partition-table primary guard; whole-disk pre-scan repair is unguarded at LBA 0. Bridge mitigates (§4.6: pre_scan_repair=0 on whole-disk targets). Upstream re-port: per-partition scoping in the whole-disk scan path + GPT-protective/MBR-plausibility refusal in cr_rawfixer.cpp. Justification: S3 forensic run 40470191039. |
| BF-07 | ERR_DEVICE_DISCONNECTED (0x50020007) reattach-then-resume flow | v4 drives a resume flow (CLI exit 5); UI surfaces device-gone + user restarts. Dedicated reattach-resume affordance (pause state + resume on reconnect) deferred. |
| BF-08 | Carve catalog + live tally unsurfaced | `carve_format_count/_info`, `scan_carve_tally_count/_tally` — no reference screen; scope a "what can be recovered" info surface if wanted. |
| BF-09 | Disk map unsurfaced | `disk_map`/`disk_map_free` — net-new visualization screen (allocation/coverage/found/confidence overlays + bad ranges); scoped for a future session. |
| BF-10 | image_create unsurfaced | Long-running elevated flat-image producer with .partial/cancel semantics; needs a Tools screen + progress; CLI covers today. |
| BF-11 | fsck_volume + turn_off_bitlocker unsurfaced | Privileged/destructive volume ops; escalation ladder documented (pre_scan_check → endpoint → raw-carve); surfacing needs consent + exit-code/outcome UX design. |
| BF-12 | Image targets unsurfaced | `open_image` would enable unelevated scans of VMDK/VHD/VHDX/flat files — a Home "scan image file" entry; also the ABI-check surface. |
| BF-13 | Live-scan thumbnails use preview-read fallback | `thumbnail_generate` is session/node_id-addressed only; snapshot-indexed thumbnails don't exist upstream. |
| BF-14 | RecoveryOptions truth gaps | keepEmptyFolders/fileRepair have no engine equivalent — truthful-disabled with reasonKey; collision/rebuildFolders/verifyOnWrite map to `cr_ui_recovery_config`. |
| BF-15 | M-074 destination taxonomy umbrella | path-too-long/invalid/network all → destination-invalid; distinct reasonKeys deferred. |
| BF-16 | Core-side query engine unused | `query_run`/`apply_buckets` (incl. exact bucket semantics: day-truncated local windows, rejected IDs 0x104/0x1005) deferred behind a T14 cost re-measure vs the NodeMeta port. |
| BF-17 | Assistant stats + regrade unsurfaced | S-023 grading stats + grade-1-only re-grade; no reference screen. |
| BF-18 | Windows build fixes are final-prod-local | testPreset `-C` fix, VCPKG_APPLOCAL_DEPS=OFF, deploy retries — recommend upstreaming to core's CMakePresets/DeployVcpkgBinaries. |
| BF-19 | Engine version-stamp drift | Core CMake VERSION 0.6.1 vs v0.7.1 notes (Agent D [DRIFT] §1); gate on `cr_ui_abi_version()` only; upstream stamp bump recommended. |
| BF-20 | Health grading duplicated at the bridge | The 4-status model is C++-side (cr_grade.h:52-57); the bridge re-implements the rules — keep in sync (or upstream a grading export). |
| BF-21 | M-101 version_group/tags fields unsurfaced | NTFS namespace-identity grouping + tag extraction ride in node_info/query; no UI surface. |

---

## 8. Risk register (top risks + mitigations)

| # | Risk (source) | Mitigation |
|---|---|---|
| R1 | **The release.yml ↔ build.ps1 ↔ NSIS triad** unwinds together; any one left driver-aware reds every tag build and poisons forensic-repro downstream (BUILD-PIPELINE §7 "single riskiest transformation") | P3 edits all three in one push; P6 release.yml dry-verified (NSIS compile check) before tagging; portable.ps1's driver hard-requirement deleted at P3. |
| R2 | **ABI v2→v4 struct strides** (disk_info tail, event POD, query_row, user_sign) — a stale mirror misreads silently (CORE §3.4, pitfall 1) | abi.rs re-pinned from the v4 header only; the old abi.rs is `git rm`ed at P1 (cannot leak); startup + per-scan ABI gates. |
| R3 | **NG-21 whole-disk repair misfire regresses** (Agent A §4 item 1) | Bridge-side closure (§4.6) + BF-06 engine re-port; forensic S3 keeps probing the consent gate. |
| R4 | **Consent deadlock class** (S3 28-minute stall, UI-INTEGRATION-SURFACE §3.3) | Port the gate machinery exactly (echo tokens, lock-release-before-block, force-deny on cancel, run-tail close, re-hydration); e2e consent.spec.ts re-landed. |
| R5 | **Mock ships as the backend by accident** (P1-P3 intermediate state) | P6 release gate asserts `connectorMode === 'tauri'` in forensic probes; index.ts default flipped at P4; mock labeled TEST DOUBLE. |
| R6 | **Version lockstep drift at re-merges** (three files per UI swap) | P1 bumps all four in the same push; build.ps1 gate + release.yml tag gate re-run at P6. |
| R7 | **Windows build-fix regressions** (testPreset `-C`, applocal race — v0.5.0-rc1 failure classes) | Re-applied immediately post-merge (§2.1.6) + BF-18 upstreaming. |
| R8 | **Sub-tree workflow resurrection** at both merges (engine/.github 12, ui/.github 4) | Explicit `git rm -r` in both merge resolutions (2a99173c precedent). |
| R9 | **knip dead-code gate** trips on a half-wired bridge (Agent F §9) | Wire domain-by-domain with the Proxy retaining unwired domains until each lands; contract coverage include list (70/60) tracked per domain. |
| R10 | **validate.mjs phase-2 activation ordering** — contracts must regenerate in the SAME push as the bridge | Single P4 push contains bridge + connector + regenerated contracts; CI order (contracts-ci on the merged push) proves it. |
| R11 | **Elevation UX is net-new** (no retry-as-admin ever existed; old UI had only a passive banner) | §4.3.5 design (posture op + relaunch + banner); forensic unelevated-launch probe validates the refusal path. |
| R12 | **Grep-sweep false positives** (affordance/yesterday/CDP-driver) | Word-boundary patterns + the recorded de-noise list (§3.9). |
| R13 | **Cargo.lock policy flip** (fresh repo untracked) | P4 commits the lockfile; lockstep gate unchanged (it never checked the lock). |
| R14 | **Core's own CI is windows-only**; final-prod loses Linux engine lanes at P2 | Documented consequence (§2.3); `engine/ci-local.sh --san|--fuzz` remains the local sanitizer/fuzz path. |

---

## 9. Verification-gate mapping (session prompt §9 → producing phase + evidence)

| Gate | Produced by | Evidence |
|---|---|---|
| 1. Driver fully removed | **P3** (sweep), re-proven at P6 | worklog/RELEASE-VALIDATION/DRIVER-REMOVAL.md (§3.9 protocol + zero-match) |
| 2. Core merged, history preserved | **P2** | RELEASE-VALIDATION/CORE-MERGE.md: merge SHA, `git log --follow` on representative files both directions |
| 3. Full product builds cleanly | **P2** (engine), **P4** (bridge/ui), **P6** (product) | RELEASE-VALIDATION/BUILDS.md + green engine-ci/ui-ci links |
| 4. Every test passes | **P2** (core ctest via engine-ci), **P4** (145+ UI suite, seam tests, cargo test), **P6** | CI links in BUILDS.md; test inventory in UI-WIRING.md |
| 5. Every workflow green for the right reason | **P1/P2/P3/P5** (each lane edited in its phase), verified **P6** | workflow list + latest-run statuses in RELEASE-RUN.md |
| 6. UI fully rewired (per-op integration proof + per-surface real-Windows validation) | **P4** (tests) + **P6** (forensic S1-S4) | RELEASE-VALIDATION/UI-WIRING.md |
| 7. Every capability surfaced or truthfully disabled | **P4** (§4.7 map) | RELEASE-VALIDATION/CAPABILITY-MAP.md |
| 8. Release cut + end-to-end validation | **P6** | RELEASE-VALIDATION/RELEASE-RUN.md (run link, install/launch/exercise/uninstall artifacts) |
| 9. Documentation accurate | **P3** (docs sweep) + **P4/P5/P6** (living docs) | doc-truth re-grep at P6 (no driver refs, no stale core refs, no false capability claims) |
| 10. Worklog complete | all phases | worklog.md chronological record (commit SHAs, CI links, measurements, deferred items) |

---

*Plan ends. Total change surface: ~250 files across six phases (P1 ≈ 1 merge +
8 edits; P2 ≈ 1 merge + re-prunes + 2 re-applies + 4 edits; P3 ≈ 120 files;
P4 ≈ 55-60 files; P5 ≈ 11; P6 ≈ 10 + evidence). Nothing here is implemented
until this document is committed and pushed.*
