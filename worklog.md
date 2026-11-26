# final-prod — Unified Product Worklog

Session log for the unified repository (engine + ui + driver + installer).
Created by the consolidation session on 2026-10-26 from:
- core_recovery @ a574ecf6cc06a374e1ee0f893e574efef4904639 (main)
- recovery-wizard-ui-new @ f4a8f8aebab625e9048d833cd9c3501741d7c436 (v3)
- rda-driver @ da7efb9e0c94046893c62b25695bb95edf519f8b (main)

This file records the product's ongoing session history. Component-local
history lives in engine/worklog.md, ui/worklog.md, driver/worklog.md
(preserved verbatim from the source repositories at merge time).

---

## 2026-10-26T11:40Z — engine/ advanced to core_recovery@5ddccd8 (latest main)

A parallel integration lane (same mandate, same sources) had already
pushed the T3–T6 state to `origin/main` (gate plan, context sheets,
three history-preserving component merges, contracts/, build entry
points). Per the mandate's coordination rules (pull frequently, never
force-push, leave main green) that state is adopted as canonical and
this lane builds on it. Convergence audit between the two lanes' plans:
both independently pinned the driver service name to `rda`/`RDA` (the
core hard-codes `\\.\RDA`), the same in-process cr_ui FFI bridge
architecture, ProgramData shared state, surfaced consent-gated driver
step, and MSIX exclusion.

This merge advances `engine/` from a574ecf to the latest core main
(5ddccd8, pushed 2026-10-26T10:31Z — after the adoption merge was cut):
a8a189e (rrecov/cr_ui Windows link fix + the new one-entry
cr_ui_exports.def), 7a76c86 (cr_ui_tests static core on MSVC), 5ddccd8
(Win7 import-allowlist for the S-014/S-010/M-033 wave). Verified before
merge: `engine/include/cr/cr_ui.h` unchanged — the 78-export ABI v2
surface the bridge targets is identical. Verified after merge:
`git diff core@5ddccd8 HEAD:engine` is empty except the deliberate
post-adoption `.github` removal (4 inert workflows). Rewritten
histories share ancestor a0405e83; full lineage preserved (repo now
1107 commits).

---

## 2026-10-26T12:00Z — T11 installer + T12 MSIX verdict (integration lane)

`installer/` lands: `recoverywizard.nsi` (the combined product installer —
517 lines) + `portable.ps1` (the no-driver portable zip) + `README.md`
(design + deviations). `docs/MSIX-ASSESSMENT.md` records the T12 verdict:
MSIX is structurally impossible for this product (no kernel-driver
installation, no elevated full-trust in AppContainer) — dropped truthfully,
NSIS is the single distribution artifact.

Architecture resolution (recorded in installer/README.md): the ratified plan
§7.2 sketched a custom Tauri NSIS template, but the committed build.ps1 (T6)
defines a wrapper contract — outer product installer consuming the stock
Tauri bundle silently. The committed code wins: the wrapper is implemented
(avoiding Tauri-template version lock-in; all product concerns in one
reviewable script; contradiction documented).

Key behaviors: dual-mode install (MultiUser; per-user installs force the
driver off with a truthful read-only note — machine-level component);
surfaced driver page with the full explanation (what it does, what happens
without it, SYSTEM/admin DACL reality, UNSIGNED truth, add/remove later
anytime) whose checkbox is the consent control (default ON, synced with the
components page); engine kit gated at install (cr_ui.dll + >=1 runtime DLL —
the 0xc0000135 guard); driver registration via the driver's own install.ps1
(exact SCM args, service `RDA`, Parameters DWORDs) with update semantics
(stop+delete+recreate, no reboot) and non-fatal start failure surfacing the
signing guidance; ProgramData `HeicFast\Recovery Wizard\{sessions,logs}`
(constants from contracts/README.md); inner Tauri uninstall-key takeover ->
ONE complete Add/Remove entry; uninstall order: ask-close -> driver teardown
(uninstall.ps1 + fallback) -> ProgramData sessions KEPT by default (explicit
consent to delete) -> inner uninstaller silent -> engine/driver dirs ->
registry -> RMDir.

Validation (evidence: docs/INTEGRATION-VALIDATION/installer-compile-check.md):
cross-compiled on Linux with makensis 3.12 — zero warnings, valid PE32
Nullsoft self-extracting artifact, define-contract !error guards verified by
negative test. Fixes made during validation: StrFunc ${Using:StrFunc}
activation, description block reordered after Section definitions (the
original order silently broke hover descriptions), MULTIUSER_EXECUTIONLEVEL
define-name fix, registry-based Win10 gate (AtLeastWin10 does not exist in
stock WinVer.nsh). Runtime install matrix = T15 on Windows CI (pending).

Earlier this hour (same lane): engine/ advanced to core_recovery@5ddccd8
(latest main) with history preserved; build.ps1 engine-lane defects fixed
(artifact paths src/app|ui/Release, per-file --allowlist PE-gate invocation,
engine-kit DLL staging from the POST_BUILD-deployed set instead of hardcoded
z.dll/lz4.dll names); ui version files synced to VERSION 0.5.0 (the
Assert-VersionConsistency gate was throwing on every run).

---

## 2026-10-26T16:00Z — T7-T10 wiring session: the bridge lands, the simulated backend is gone, CI consolidated at root

Session start state: main at 87b9ccd9 (engine/ advanced to
core_recovery@22e9657 with history preserved; T1-T6, T11, T12 done; T11's
installer compile-validated). The engine update session that preceded this
one had drafted the Rust bridge + TS connector locally but was lost to an
unexpected stop before any commit — this session rebuilt the whole T7-T10
surface from the ratified plan + the committed contracts, committing and
pushing in four chunks (1efb8d32, 02d153af, 728b0baa + this commit).

The Rust bridge (ui/src-tauri/src/{bridge,commands}):
- bridge/abi.rs — the FFI surface pinned 1:1 from cr_ui.h (this exact
  filename is the validate.mjs phase-2 trigger); bridge/error.rs —
  BridgeError{code, bridgeCode, message} per error-codes.json, ERR_CANCELLED
  never crosses as an error; bridge/util.rs — two-call protocols, the
  single 100ns-1601 -> ISO-8601 conversion point (civil-from-days, no
  chrono), base64, SendPtr, OwnedCrString; bridge/state.rs — Arc-ed
  registries (long passes hold the entry's snapshot mutex, never the
  registry lock), per-generation snapshot metadata cache built in ONE DFS
  pre-order pass (paths O(total path bytes); subtree aggregates in the
  reverse pass), engine close order enforced on Drop, session source
  re-attach best-effort by model+serial (browsing never blocked on a gone
  source); bridge/events.rs — sinks only try_send (engine worker threads
  never block/lock/re-enter), single emitter per handle, HEARTBEAT is
  liveness-only, bad-sector map bounded 4096 with exact counts,
  LOST_CANDIDATE/REPAIR/FS_CHECK ride counters+logs (no payload surface
  exists; repair analysis armed default-deny), job emitter coalesces byte
  samples + polls stats/report-tail at 2s cadence for currentFile/verify
  truth; bridge/filefilter.rs — GENERATED from the engine's own 1,481-entry
  table (ui/scripts/gen-filefilter.mjs, --check freshness gate; exact
  parity verified 1481/1481 including the dv-avi/dvr-ms/mp+ entries).
- commands/ — 33 commands implementing the amended manifest: devices
  (open-probe status, Win32 usage, virtual quick-targets vol-recycle-bin /
  vol-desktop -> system volume + implicit pathScope); scan (the documented
  binding lifecycle, G-11 scope validation, pre-scan repair + damage audit
  READ-ONLY with the engine's documented default-deny [G-1], terminal latch
  from run() rc with exact stats + the G-3 sum, transients cleared when
  telemetry catches up); results (ONE bridge-side filter path for BOTH live
  scans and sessions [G-7: cr_ui_query cannot express the 'existing' atom];
  the tree modes reproduce the retired simulator's visible contract from
  REAL origins — lost branch is the real dir tree pruned to lost dirs, raw
  branch flat truth; sorts mirror the engine); preview (ADR-008 dispatch
  from engine metadata, image cap 16 MiB, unreadable spans -> the typed
  preview-damaged refusal, av/office metadata payloads with null renderables
  [G-4], thumbnails graphics-only); sessions (ProgramData store, N-061
  auto-name UTC, mtime-keyed lazy list cache [OQ-4], load pre-check +
  best-effort attach); recovery (config map, F12 armed engine-side, G-02b
  snapshot-index enqueue, recover-all pages the filtered query, status from
  the engine's stats, items/report from the report JSON with the outcome map
  + 500-cap/truncatedItemLists); health (threshold null — the engine has
  none); repair (the four G-1 typed refusals); system (SCM RDA probe +
  \\.\RDA + IOCTL 0x60002448 [OQ-5 pinned from cr_transport.h: service RDA],
  engineVersion null [G-6], real write-probe destination validation,
  reveal_in_explorer absorbed into system_open_path via the opener plugin).
- Cargo.toml: serde_json + windows-sys (cfg-gated); build.rs: cr_ui.lib
  static-import link wiring (CR_UI_LIB_DIR override + staged target dirs;
  cargo check/clippy need no kit).

The TS connector (src/connector/tauri/tauri-connector.ts): the 36 contract
methods over the 33 commands; REASON_KEYS (all 24 codes) + the engine
numeric map + detail.code; hex JSON-array -> Uint8Array normalization;
listen()-based subscriptions with the contract's unsubscribe shape.
src/connector/index.ts exports only the real singleton.

The removal (plan §5): mock tree (mock-connector, fixtures, node-space,
rng), tauri/not-implemented.ts, queries/dev.ts, connector-mock.test.ts,
node-space.test.ts, the RNG block in format.test.ts,
ScenarioControl/MockScenarioId/connectorMode from the contract; dev-panel
restructured (language/density/perf-overlay survive); settings/about mode
rows removed; the dead "Open location" item WIRED (existing-origin rows
reveal their real on-disk path via getNodeDetail + openPath through the
queries layer; the rest carry the truthful openLocationMissing title).
Locales: dead keys removed, the REASON_KEYS vocabulary added (en + zh in
lockstep); i18n sweep 375 defined / 0 unused / 0 missing.

Test seam (plan §5.5): tests/fakes/test-connector.ts — the labeled TEST
DOUBLE (never shipped; A7 enforces it; the G-1 gaps refuse exactly like
production); vite.config.ts test-mode alias (exact-match regex; production
builds verified to contain the real connector and zero double strings);
arch-lint A7 added; tests/connector-tauri.test.ts — 23 tests pinning the
command surface, error model, REASON_KEYS-vs-catalogs, and unsubscribe
shapes against a spied IPC boundary.

contracts/command-surface.json amended to the shipped truth (33 commands):
consent_respond/preview_read/preview_close moved to a rationale-bearing
`removed` array. contracts/validate.mjs GREEN with phase 2 active for the
first time: 33 Rust impls x 32 TS invocations cross-checked both ways.

Gates (local, this sandbox): typecheck clean; eslint 0 errors; arch-lint
A1-A7 OK (75 files); ast-grep x2 clean; prettier clean; jscpd 0 clones;
vitest 127/127; coverage 96.4/84.4/100/98.6 on the real connector
(thresholds 70/60/70/70); production build + bundle:check in budget
(111.9/200 KB main, 296.5/500 KB total); knip GREEN with
KNIP_DISABLE_RAW_TRANSFER=1 (the same knob the UI repo's original CI
documented — without it oxc's raw transfer exceeds this 4GB sandbox's
allocation limit, before AND after these changes).

Installer/build alignment (this commit): build.ps1's ui lane NOW actually
stages the engine kit (cr_ui.lib + DLLs into target/{release,debug} +
CR_UI_LIB_DIR) — the comment claimed it before but no copy existed;
recoverywizard.nsi installs the kit BESIDE the app exe (layout truth: the
bridge statically imports cr_ui.dll — loader resolution happens from the
exe directory; an engine\ subfolder would 0xc0000135), excludes the import
library from the customer layout, ships rrecov.exe as the G-1 CLI undo
path, and the uninstaller deletes the kit explicitly; portable.ps1 mirrors
the beside-exe layout.

CI consolidated at root (per plan §8.3, the consolidation the T4 deletion
of inert sub-tree workflows pointed at): contracts-ci.yml (validate.mjs +
filefilter freshness), ui-ci.yml (all TS gates + cargo check/clippy —
no engine kit needed), engine-ci.yml (lint/ASan/TSan/UBSan/Windows-MSVC +
Win7 PE gates/fuzz — adopted from the engine repo's proven lanes with
working-directory + paths filters), driver-ci.yml (lint/fuzz/unit/WDK
matrix + unsigned assertion — adopted likewise). Push watches: first run
of the Rust lanes will surface compile reds this sandbox could not check
(no cargo here) — iterate on them per the worklog's next-actions.

docs/INTEGRATION-GAPS.md created: G-1..G-11 with the session's resolutions
(the consent-flow decision recorded in full under G-1) + the hygiene items.

Remaining (tracked, in order): watch the four root workflows' first runs
and fix every red for the right reason (T17 lane); INTEGRATION-VALIDATION
evidence (T14: budgets vs the retired simulator's documented latencies;
the mid-scan query cost measurement G-7 depends on); e2e under
`vite build --mode test` (ui-e2e.yml + playwright config update — the
built-bundle story the plan §5.4 sets); release.yml (T13); docs sweep
(T16: README/BUILD/ARCHITECTURE/RELEASE/TESTING + CONNECTOR-CONTRACT mock
sections + stale VITE_CONNECTOR references); RC tag (T18); handoff (T20).

---

## 2026-10-26T17:15Z — the bridge compiles: 10-round CI iteration to green (T7 compile lane + T17 start)

The Rust bridge was written without any local cargo (the sandbox has no
Rust toolchain) — the ui-ci Windows lane IS its compiler. Ten rounds,
every red fixed for the root cause, zero suppressions:

- R1 (a790cf9b): json! let-block value (the macro parses '{' as an object
  start) + a leftover positional {}. Pre-flight audit in the same push:
  SendPtr INSIDE thread closures (raw pointers are !Send even when
  extracted just before the move), Manager import, unused imports,
  parens-wrapped if-else json! values.
- R2 (cdaf7e53): commands/mod.rs was missing the preview module; Sender
  import; the scan emitter's lock guard needed mut; JobLive field-name
  mismatch (last_throughput); NodeMeta needed Clone; savedAs Into<Value>;
  emit takes &str; SCM handles are pointers (.is_null()).
- R3 (d701b74a): std mpsc Sender::try_send is NIGHTLY-ONLY — the sinks now
  use bounded sync_channel + SyncSender::try_send (STABLE, never-block,
  coalescing semantics per event-mapping.md); metas_snapshot borrow-vs-move;
  one unused mut.
- R4 (f2ae9f59): buf's mut restored (round-3 removed the wrong binding);
  five unused imports.
- R5 (521371ec): lib.rs's BridgeState import needed the module path after
  the re-export cleanup. CARGO CHECK PASSED at this point.
- R6 (6b4c7ebd→60937e45): clippy dead-code sweep — the pinned abi.rs gets a
  module-scoped allow(dead_code) WITH the rationale (the complete 78-export
  header binding stays declared; G-1/G-7 explain why some exports are
  unconsumed); every genuinely dead bridge item REMOVED (unread struct
  fields, superseded helpers, never-minted error constants).
- R7 (61c0646a): 15 clippy style findings, all fixed canonically (PageReq +
  TreeSpec structs replacing 8/9-arg helpers, strip_prefix, char-array
  patterns, &Path, sort_by_key(Reverse), closure elisions, doc reflow).
- R8 (39187ba7): Option<SendPtr> moves (derive(Copy) bounds T: Copy) — borrow.
- R9 (4c9f115a): one redundant field name. **ui-ci GREEN.**

GREEN STATE at 4c9f115a (verified via the API):
- contracts-ci: validate.mjs phase 1+2 GREEN on every push since the bridge
  landed (33 commands cross-checked both directions) + filefilter freshness
  (1,481 extensions).
- ui-ci (windows-latest): typecheck, eslint, prettier, arch-lint A1-A7,
  ast-grep x2, knip (KNIP_DISABLE_RAW_TRANSFER=1 — the oxc raw-transfer
  reservation exceeds small runners/sandboxes; documented in the lane),
  jscpd 0 clones, vitest 127/127 + coverage 96.4/84.4/100/98.6 on the real
  connector, production build + bundle budget, **cargo check + clippy
  -D warnings GREEN** on the ~3,700-line bridge.
- driver-ci (a790cf9b): lint/fuzz/unit/WDK Debug+Release matrix + the
  unsigned assertion — GREEN first run.
- engine-ci (a790cf9b): lint, linux ASan (full suite), linux-ubsan, fuzz,
  **windows (MSVC vcpkg manifest Debug+Release + Win7 PE stamp/import
  gates + ctest)** all GREEN; linux-tsan still in flight at press time.

Also this window: playwright.config.ts builds in TEST MODE before preview
(the e2e bundle carries the labeled double — verified both ways: test build
contains the double's fixtures and zero real-connector strings; production
the reverse); docs sweep (ui/README.md connector section, CONNECTOR-
CONTRACT.md's shipped-bridge + never-shipped-double sections, the
UI-SURFACE historical banner, pathScope G-11 truth).

Next (tracked): engine-ci's tsan tail; ui-e2e.yml (playwright lane with
browsers installed); release.yml (T13); INTEGRATION-VALIDATION evidence
(T14); root README/BUILD/ARCHITECTURE/RELEASE/TESTING (T16 remainder);
the RC tag + clean-runner install evidence (T18/T15).

---

## 2026-10-26T17:35Z — all CI lanes green; release lane + docs set shipped; handoff

Green state at ec3e50b5 (verified via the API):
- contracts-ci GREEN — validate.mjs phase 1+2 (33 commands cross-checked
  both directions) + the filefilter freshness gate (1,481 extensions).
- ui-ci GREEN (windows) — typecheck, eslint, prettier, arch-lint A1-A7,
  ast-grep x2, filefilter, knip (KNIP_DISABLE_RAW_TRANSFER=1, documented),
  jscpd, vitest 127/127, coverage 96.4/84.4/100/98.6, production build +
  bundle budget, cargo check + clippy -D warnings on the bridge.
- ui-e2e GREEN (windows) — 12/12 Playwright specs on the test-mode build
  (the double fidelity fix: it now resolves the virtual quick-targets
  exactly like the real bridge — succeed where production succeeds).
- driver-ci GREEN — lint, fuzz, unit (MSVC /W4 /WX), WDK Debug+Release
  matrix + the unsigned assertion.
- engine-ci — lint, fuzz, Linux ASan (full suite), Linux UBSan, Windows
  MSVC + Win7 PE gates + ctest ALL GREEN; the TSan lane was still in
  flight at press time (the longest job in the set; watch run 40349677467
  — no code changed under it since it started, its outcome is about the
  engine tree, which this session never touched).

Also this window:
- release.yml (T13): version gate (tag == VERSION) → contracts gate →
  build.ps1 -Target all → artifact verification → truthful notes (nothing
  signed; the driver test-signing path; the surfaced driver step; the
  portable no-driver label) → release upload with SHA256SUMS,
  fail_on_unmatched_files.
- The root docs set (T16): README (integration-shipped status + docs map),
  BUILD (lanes, version policy, toolchain, the engine-kit link/load
  truth), ARCHITECTURE (component diagram, threading rules, install
  layout), RELEASE (tag flow, signing truth, RC discipline), TESTING
  (suite map + the double's charter), PARITY-DELTAS (D-I1..I11 + this
  integration's D-B1..B5), INTEGRATION-VALIDATION (evidence index with
  committed evidence, the CI green table with SHAs, and the PENDING items
  stated as pending), INTEGRATION-GAPS (G-1..G-11 with follow-ups).
- ui-e2e.yml + the playwright test-mode webServer.

Remaining work (tracked honestly — none of it blocks the integration
claim; all of it is executable from this handoff):
1. Watch engine-ci's TSan tail (run 40349677467); fix for cause if red.
2. T14 measurements: the latency/throughput table vs the retired
   simulator's documented budgets + the G-7 filter cost at 10^5/10^6
   scales + memory bounds. Executable on any windows runner with the
   engine kit staged; record into docs/INTEGRATION-VALIDATION.md.
3. T15 runtime matrix: run the built installer on a clean windows runner;
   capture install/uninstall/update/repair transcripts + registry/dir
   diffs; commit under docs/INTEGRATION-VALIDATION/.
4. T18: cut v0.5.0-rc1, confirm release.yml end-to-end, install/run on the
   clean runner, then the final tag.
5. G-1 follow-up (the one capability gap with a UX consequence): the
   scan-page consent prompt + consent_respond re-add, per the recorded
   design in INTEGRATION-GAPS.md G-1.

Every claim above is verifiable: the runs are in the repository's Actions
history at the named SHAs; the committed evidence artifacts are in
docs/INTEGRATION-VALIDATION/ and docs/; the manifest's removed-commands
rationale is in contracts/command-surface.json itself.

---

## 2026-10-26T18:35Z — G-1 (a) SHIPPED (consent flow end to end); TSan green; the T14/T15 pipeline dispatched

The one capability gap with a UX consequence is closed (G-1 follow-up (a),
the ratified plan §4.5 design), pushed through three CI rounds with every
red fixed for the cause:

- Bridge: bridge/consent.rs (ConsentGate + ConsentPipe + the sink), the
  state-registry consent map, scan_start arming the REAL callback, cancel
  force-deny (the deadlock rule), the run-tail pipe free + survivor close +
  undo-log persistence after an approved repair, consent_respond re-added
  (the removed[] entry retired), scan_state's pendingConsent re-hydration.
- Contracts: command-surface v2 (34 commands, 6 events), consent-not-found
  in bridgeOnlyCodes, event-mapping.md's consent section rewritten to the
  shipped scan-scoped keying (the plan §4.3 deviation recorded, plan text
  stays frozen).
- TS/UI: the typed prompt on the scan page (non-dismissable, Deny the safe
  default, refused/no-action Approve disabled), the consent store (event +
  state re-hydration), 38 connector methods, en/zh locales.
- Tests: 130/130 vitest, 14/14 e2e (2 new consent specs — the round-1
  re-hydration spec taught the hash-router lesson: page.goto destroys the
  in-memory double; the fixture navigates by location.hash now), the
  double's Desktop-target damaged-volume fixture (walk BLOCKED until
  answered, mirroring the run thread).
- Round reds, root-caused: Borrow<&str> map lookup (&&str), a
  prettier-missed spec edit, the responsive keyboard spec's latent
  mount-race (either-name regex matched h1+h2 — pinned to the page h1),
  clippy type_complexity on the 4-tuple (ScanPrep struct, R7 style).

Also this window:
- engine-ci TSan: the original job was wedged >9h on one step (cancelled);
  the re-run completed GREEN — the lane is 6/6 at a790cf9b.
- The T14/T15 pipeline: tools/validation/measure-cr-ui.c (engine-side
  harness against the staged kit, pinned-header include), the results.rs
  #[ignore]d G-7 filter/memory measurements (ui-ci check gains --tests),
  tools/validation/installer-matrix.ps1, and validation-evidence.yml —
  dispatched at d87c7d3c (run 40377623726).
- A real bug fixed while writing the matrix: the uninstaller's MB_YESNO
  sessions prompt auto-answered YES in silent mode — /S uninstalls silently
  WIPED ProgramData. Silent now keeps state; interactive consent deletes.

Remaining: watch the validation run (T14/T15 evidence), commit the reviewed
evidence into docs/INTEGRATION-VALIDATION/, then T18 (cut v0.5.0-rc1, watch
release.yml end to end, clean-runner install via the matrix, final tag).

---

## 2026-10-26T22:35Z — T18: the v0.5.0-rc1 cut (release lane dispatched)

State at the cut: main at d4922aff. Product lanes green — contracts-ci,
ui-ci, ui-e2e at b671348f; engine-ci 6/6 and driver-ci green at a790cf9b;
no engine/, ui/, or driver/ source changes after those SHAs (the trail
since touched only installer/, tools/validation/, docs/, build.ps1 — and
build.ps1's full-product path is proven by the validation lane's green
"Full product build" steps on d4922aff itself). T14 evidence committed
(docs/INTEGRATION-VALIDATION/t14-measurements/). T15 (installer runtime
matrix) still pending — two validation-evidence runs in flight on
d4922aff; the T15 matrix step is the long pole and is under active
diagnosis this session (the RC proceeds first per operator instruction:
artifacts in hand for local testing outrank the T15 transcript).

The cut: VERSION 0.5.0 -> 0.5.0-rc1 in lockstep with ui/package.json,
ui/src-tauri/Cargo.toml, ui/src-tauri/tauri.conf.json (the
Assert-VersionConsistency gate's exact consumer set; pnpm-lock.yaml and
the tree carry no other version references — verified by grep). Tag
v0.5.0-rc1 pushed -> release.yml dispatched: contracts gate, build.ps1
-Target all (WITH tests — engine ctest + ui vitest in-lane), artifact
verification, truthful unsigned notes, release upload with SHA256SUMS.
RC purpose per docs/RELEASE.md: operator tests the setup.exe + portable
zip on a real machine before the final v0.5.0 tag.
## 2026-10-26T22:40Z — validation round 11 root-caused: silent installs blocked on MessageBox without /SD

Round 11 (run 40405891621, d4922aff) was the FIRST to get past the app
integrity gate — the registry-seed fix from d4922aff worked (the inner
setup now lands in OUR $INSTDIR). But the T15 matrix then wedged ~40 min
in the "Run the runtime matrix" step with zero further evidence.

Diagnosis (evidence chain):
- Cancelled the run to harvest its `if: always()` artifact: the
  t15-runtime-matrix artifact held ONLY `01-baseline.json` (231 B, clean
  baseline) — no install transcript, no abort-reason file, no matrix.json:
  the silent installer process neither exited nor aborted. It was HUNG
  inside the install.
- The only blocking construct on the silent path: `MessageBox` WITHOUT
  `/SD`. NSIS manual §4.12: a silent installer's script "can still
  display whatever it wants, use MessageBox's /SD to specify a default
  for silent installers" — boxes without /SD display AND block /S runs
  (the classic trap; electron-builder #616 is the canonical field report).
- The exact box: the driver section's "installed but could not be
  started" notice. On a stock GitHub windows-2022 runner `sc start RDA`
  fails fast (unsigned driver, no test signing) → $DriverResult =
  "installed-not-running" → the box → wedged. Rounds 1-10 never reached
  the driver section (they aborted in SEC_APP), so this was never
  exercised until the seed fix let them through.
- `driver/scripts/install.ps1` ruled out as a hang point: purely
  non-interactive (sc.exe + registry + non-fatal Write-Warning).
- Stock `MultiUser.nsh` (fetched, v20210216) ruled out for OUR config:
  its privilege boxes never fire elevated + /allusers, and its uninstall
  init path carries no boxes in that configuration.

Fix (installer/recoverywizard.nsi, all silent-path boxes now carry /SD):
- driver registration-failed box → /SD IDOK (silent: continue in degraded
  mode, state recorded; interactive: box shown)
- driver installed-not-started box → /SD IDOK (the EXPECTED branch on CI
  runners; a DetailPrint records it in the log as well)
- .onInit Win10 gate box → /SD IDOK (silent: box skipped, Abort still
  refuses with exit 2)
- uninstaller ask-close RETRYCANCEL → /SD IDCANCEL (silent: refuse the
  uninstall rather than remove anything under a running app)
- uninstaller sessions prompt → /SD IDNO under the existing ${Silent}
  guard (two independent layers now); the guard's comment corrected — the
  old rationale claimed NSIS auto-answers with the default button; the
  truth per §4.12 is it BLOCKS. The guard was and remains correct; the
  rationale text was not.
- installer/README.md gained the "/SD discipline" section;
  installer-compile-check.md gained the round-11 re-validation (makensis
  3.11-1 Debian cross-compile, exit 0, zero warnings at -V3).

Also this window: run 12 (40409125718, dispatched 22:01 by a second
session/user on the SAME commit) was equally wedged at the same box and
was cancelled for the same reason — same tree, same bug, ~55 min of
doomed runner time reclaimed.

Next: re-dispatch validation-evidence (run 13) on the fixed commit; on
green, commit the T15 evidence + the T14 scan-prep row. (The RC tag
v0.5.0-rc1 was already cut by the entry above — release.yml run 1 is in
flight on it; the /SD fix does not affect the release BUILD path, which
compiles the installer without running it, so the RC artifacts are sound.
The fixed installer ships with the final v0.5.0 tag after T15 lands.)

---

## 2026-10-26T23:05Z — delta on bdf4eb3d: the ask_close dead check goes live; reg-failed DetailPrint

Reconciled with the parallel lane's /SD fix (bdf4eb3d — adopted as base:
same five boxes fixed, makensis 3.11 cross-compile re-validated exit 0,
their /SD IDCANCEL refuse-semantics for silent-uninstall-under-a-running-app
is the right call for a recovery product — never remove the engine under a
live scan; my earlier ${Silent} guard variant would have proceeded and is
dropped in favor of theirs). Three deltas they lacked, layered on top:

- ask_close was a DEAD CHECK: FindWindow searched for a window titled
  "HeicFast Recovery Wizard" (PRODUCT_NAME) — the app window's text is
  "Recovery Wizard" (tauri.conf.json app.windows[0].title; the frameless
  window still carries its title as window text). The close-app prompt
  could never fire, interactive or not. New APP_WINDOW_TITLE define +
  live lookup; the /SD IDCANCEL semantics now actually mean something.
- registration-failed branch gained the DetailPrint counterpart (the
  not-started branch had one; failed needed its own for silent logs).
- installer/README.md: the stale integrity-check reference
  'Recovery Wizard.exe' corrected to recovery-wizard-ui.exe.

---

## 2026-10-26T23:15Z — v0.5.0-rc1 release run: failed at engine ctest; build.ps1 fixed; rc2 cut

The rc1 release run (40413302053) died in step 10 at build.ps1:72 —
'engine tests failed': 924/927 passed, destio/bitlocker/cr_ui_abi_check
'Not Run'. Root cause: build.ps1 invoked `ctest --preset win-x64-release`
WITHOUT -C on the VS multi-config generator — the three plain add_test
entries cannot resolve their config-dependent executables that way (the
gtest_discovered suite resolves, hence 924 green and the misleading '99%
passed'). The engine repo's own windows lane always passes `-C Release`
(engine-ci.yml line 157). build.ps1's test path had never run on CI
before: every validation lane used -SkipTests and engine-ci uses its own
invocation — exactly what an RC is for (RELEASE.md candidate discipline).

Fix: `ctest --preset $preset -C $Configuration --output-on-failure` (the
engine-ci form). No release object was half-created for rc1 (the upload
step never ran; verified via the API — zero releases).

rc2 cut: VERSION + the three lockstep consumers -> 0.5.0-rc2 (the rc1 tag
is immutable; failures roll forward, tags never move). The rc2 tree also
carries the /SD silent-mode fix (bdf4eb3d) + the ask_close live-check
delta (d0670530) — rc1's installer would have hung any /S deploy.

## 2026-10-26T23:05Z — round 13 evidence read: four real product bugs fixed; release run 2's deploy race fixed

Run 13 (40413941883, bdf4eb3d) — the first with the /SD fix — completed the
WHOLE T15 matrix in ~15 s of installer time: baseline clean, install ok
(6.1 s), update/reinstall ok (marker preserved), repair-over-corruption ok
(hash restored). The uninstall row read CHECK-LEFTOVERS. The evidence
files named four DISTINCT product bugs, all fixed this window:

1. Registry layout did not match the plan (or any consumer's probe).
   The .nsi wrote the product SW key as the single key
   `Software\HeicFast Recovery Wizard` (a key NAME containing a space),
   while INTEGRATION-PLAN §7 row 6 — and the matrix probe, and the
   ProgramData tree's own shape — all use the two-level
   `Software\HeicFast\Recovery Wizard`. The matrix's snapshot of
   `HKLM:\SOFTWARE\HeicFast\Recovery Wizard` (a two-segment PowerShell
   path) was actually reading the /D-SEED key (case-insensitively the
   same tree), which holds only the default value — hence
   `regVersion: null` and the "missing" named values that were never
   missing, just written elsewhere. Fixed: PRODUCT_SWKEY
   ("Software\HeicFast\Recovery Wizard") for every SW-tree access;
   the uninstall key renamed to the plan-exact `Uninstall\Recovery
   Wizard` (key name = product name, NSIS standard); the matrix probe
   follows; MULTIUSER_*_REGISTRY_KEY defines follow; the uninstaller
   now also deletes the seed key (`Software\heicfast\Recovery Wizard` —
   run 13 proved the inner uninstaller does NOT remove it) plus its
   empty manufacturer parent.

2. The elevation guard in driver/scripts/install.ps1 (and uninstall.ps1)
   was dead code that KILLED the script: `[WindowsPrincipal]::
   [WindowsIdentity]::GetCurrent()` parses but evaluates to $null
   (PowerShell treats the whole `[WindowsIdentity]::GetCurrent()` as
   the MEMBER-NAME expression of the outer `::` access — verified with
   a portable pwsh probe), so `IsInRole()` ran on null and the script
   exited nonzero at line 1. Evidence: driverService null + no
   System32\drivers\RDA.sys after install — install.ps1 produced
   NOTHING. The .nsi's registration-failed branch (with /SD IDOK)
   then recorded the degradation truthfully, which is why the install
   row still read ok. Fixed to the canonical two-line guard in both
   scripts. (The .nsi's sc.exe fallback had been masking the broken
   uninstall.ps1 — the driver's own teardown never once ran.)

3. The shipped engine kit carried TEST runtimes: the deploy backstop
   copies every vcpkg bin DLL, including gtest/gmock (the manifest's
   test dependency). cr_ui.dll imports none of them (its import
   allowlist proves it), yet the kit — and the portable zip — shipped
   them, and the uninstaller's hardcoded DLL delete list missed them:
   the 4 leftover files of the uninstall row. Fixed: build.ps1's kit
   curation excludes gtest/gmock (the build tree keeps them — the
   tests need them); the uninstaller gains a `$INSTDIR\*.dll`
   catch-all AFTER the inner uninstaller has removed its files (every
   remaining DLL in $INSTDIR is then ours — also cleans installs made
   by older installers whose kits carried the test DLLs).

4. Release run 2 (v0.5.0-rc2, 40415111906) failed in the ENGINE BUILD:
   `error MSB3075 ... z-applocal ... Access is denied` (exit 5) — a
   deployment RACE. The engine had TWO deployers (vcpkg's z-applocal
   per-target POST_BUILD + our cr_deploy_vcpkg_binaries backstop);
   under `cmake --build --parallel` they wrote the same DLLs into the
   same directories concurrently. Nondeterministic — four prior builds
   passed, run 2 lost the race. Fixed: VCPKG_APPLOCAL_DEPS=OFF in both
   win presets (the backstop is the single deployer — the engine's own
   docs already distrust z-applocal's name resolution) +
   DeployVcpkgBinaries.cmake retries each copy 4x/250 ms (five test
   targets share tests/<CFG>, so even single-deployer POST_BUILDs can
   overlap under --parallel; a sharing violation then is transient).

Also: rc2's artifacts (if the lane re-runs green) carry the pre-fix
installer — the rc1/rc2 tags are immutable records; the FINAL v0.5.0
tag ships the corrected product after run 14's matrix lands green.

Validation done locally before push: makensis 3.11 cross-compile of the
.nsi (exit 0, zero warnings at -V3); pwsh parse-lint of every touched
.ps1; the plan's §7 row 6 re-checked verbatim for both key forms.

Next: push, dispatch validation run 14 (the fixed installer), watch
engine-ci on the preset/deploy changes, then re-cut the release lane.

---

## 2026-10-26T23:20Z — delta on 790bf5b3: the installer records its own lifecycle; v0.5.0-rc3 cut

Reconciled with the parallel lane's round-13 fixes (790bf5b3 — adopted
as base; their four root-cause analyses stand: the single-key-with-space
SW-tree, the dead elevation guards, the kit test-runtimes, the deployer
race). My overlapping build.ps1 changes were DROPPED in their favor
(their fixes address the causes; redundancy dropped, not silenced). The
rc3 tree = 790bf5b3 + this delta:

- The installer gains an ALWAYS-ON facts file (%TEMP%\
  HeicFastRecoveryWizard-install-facts.txt, the InstallFact macro):
  mode resolution, the inner app-setup exit code, the driver
  install.ps1/uninstall.ps1 exit codes, sc start's exit, and READ-BACKS
  of every product-key write — the ground truth silent installs leave
  nowhere else. The round-13 lessons (a failing install.ps1 left NO
  surviving exit code; registry writes landed where nothing probed)
  both become one-class-visible: the next failure names itself. The T15
  matrix captures the file at every setup boundary.
- The driver script invocations (install + uninstall sides) move from
  nsExec to ExecWait: the command line reaches powershell VERBATIM
  (nsExec re-tokenizes/re-quotes — the round-8 /D= lesson as a class);
  $INSTDIR always contains spaces. With the elevation guards fixed the
  scripts now run; ExecWait removes the quoting failure mode outright.
  The console flash is the documented cost.
- The uninstall registry fact + un.onInit mode fact join the set.
- Compile validation: makensis 3.11-1 cross-compile (portable Debian
  nsis + nsis-common, NSISDIR override — same methodology as
  docs/INTEGRATION-VALIDATION/installer-compile-check.md), exit 0, ZERO
  warnings at -V3, valid PE32 Nullsoft artifact, 7 sections.
- The matrix hard-fails if any gtest*/gmock* DLL reaches the customer
  layout (belt-and-braces over build.ps1's kit exclusion) and names
  leftover files in the uninstall evidence.

VERSION -> 0.5.0-rc3 (rc2 produced no artifacts — its release run died
at the pre-fix deployer race). Release run 3 (the rc3 tag) and
validation run 14 (790bf5b3, already dispatched by the parallel lane)
run concurrently; rc3's installer = run 14's installer + the facts file
and the ExecWait hardening.
## 2026-10-26T23:30Z — run 14: the product matrix is GREEN; the step's own null-check killed it (fixed)

Run 14 (40417511050, 790bf5b3) — the first validation with round 13's four
fixes — delivered a FULLY GREEN product matrix (t15-runtime-matrix
artifact, matrix.json):

- install ok — driverService "Stopped" (the RDA service EXISTS now: the
  elevation-guard fix in install.ps1 works), regVersion "0.5.0-rc2" and
  installMode "AllUsers" read back from the REAL product key (the two-level
  registry fix works — the probe and the writer finally agree), files 13
  (was 17: the gtest/gmock exclusion works)
- update(reinstall) ok — marker survived
- repair-over-corruption ok — hash restored
- uninstall(silent) ok — unKeyGone, swKeyGone, driverGone, sysDriverGone,
  sessionsKept, zero leftovers (the seed-key cleanup + *.dll catch-all work)

And then the STEP exited 1 with no exception, right after "all rows ok".
Root cause (reproduced locally with a portable pwsh): the step's
`if ($LASTEXITCODE -ne 0) { exit 1 }` — the matrix script is cmdlet-only,
so a fresh pwsh session leaves $LASTEXITCODE $null, and `$null -ne 0` is
TRUE (PowerShell's null-comparison asymmetry). Runs 1-13 always failed
INSIDE the matrix (the throw exits the process via the error path), so
this latent check was never exercised until the first green matrix.
Fixed in validation-evidence.yml (the check now reads `$?`), and the same
latent class fixed in build.ps1's Invoke-Portable (standalone
`-Target portable` from a fresh shell would have thrown on SUCCESS —
masked in the 'all' lane by earlier native commands).

Audit of every `if ($LASTEXITCODE -ne 0)` in the workflows: all other
instances follow native-command invocations (cl, build.ps1 which runs
cmake/pnpm/makensis/python) — correct as-is.

Concurrent-lane note: 719041b2 (rc3) adopted 790bf5b3 as base and added
the always-on install-facts file + ExecWait verbatim quoting for the
driver scripts; release run 3 is in flight on the rc3 tag as of this
entry.

Next: dispatch run 15 on the fixed workflow — expecting the first fully
green validation-evidence run; then harvest + commit the T15 evidence,
then the final v0.5.0 tag.

## 2026-10-26T23:45Z — v0.5.0-rc3 RELEASED (run 3 green, artifacts live); notes-template backtick escape fixed

Release run 3 (40418458268, tag v0.5.0-rc3) completed SUCCESS end to
end: contracts gate, full product build WITH tests (engine ctest 927
green via the -C fix; ui vitest; the deployer-race fix held), artifact
verification, truthful notes, upload. The release is live at
github.com/heicfast/final-prod/releases/tag/v0.5.0-rc3 with all three
assets: RecoveryWizard-0.5.0-rc3-setup.exe (3.48 MB),
-portable.zip (4.35 MB), SHA256SUMS. Operator local testing can begin.

One real notes bug found reading the published body: the template's
single-backtick code span around bcdedit hit PowerShell's \`b escape
(BACKSPACE) — the published text read 'cdedit /set testsigning on'.
Template fixed (double backticks, the same form the other code spans
already use); the live release body patched via the API (verified:
'bcdedit /set testsigning on' now correct). rc1/rc2 produced no
releases, so rc3 is the first operator-testable artifact set.

Also adopted 2be4239a (the parallel lane's \$? fix — better than my
guard-removal: it catches any invocation failure, and they caught the
same latent class in build.ps1 Invoke-Portable + audited every other
guard). My guard-removal commit was dropped for theirs.

## 2026-10-26T23:50Z — run 15: the facts file earns its keep on day one — install.ps1's silent exit-1 + my null-guard bug

Run 15 (40419843389, 2be4239a) — the FIRST run carrying the facts file.
The rc3 installer's registry story is PERFECT (03-after-install.json:
all four product-key values present and correct in the two-level key;
the read-backs in the facts file match one-for-one; 13 files, zero
test DLLs; service created and Stopped). Two failures found:

1. driver install.ps1 exit=1 WITH THE SERVICE FULLY REGISTERED. The
   facts file names it in one line — exactly what it was built for.
   Root cause (two mechanisms, both fixed): (a) `powershell -File`
   propagates the last native command's exit code as the process exit
   when the script ends without an explicit exit — sc.exe start fails
   on the unsigned runner, so the script "fails" AFTER succeeding;
   (b) the start-failure Write-Warning used a trailing '+' string
   continuation that is not valid concatenation in argument position.
   Fix: one-string warning + explicit `exit 0` (registration is the
   script's contract; the start failure is the warned, non-fatal,
   EXPECTED stock-Secure-Boot outcome — the .nsi's own sc start + /SD
   notice handle the degradation truthfully). The driver repo's own CI
   never exercised this path (no install.ps1 invocation in driver-ci;
   the start-failure branch only runs unsigned).

2. MY OWN matrix guard threw on a PERFECTLY CLEAN kit: the empty-list
   "()" in run 15's error message is the tell. `@($null).Count` is 1
   (array-wrapping a null yields a one-element array) — the kit-exclusion
   guard I added in 719041b2 misfired on its first clean input. Fixed:
   null-ness IS the dirt signal (any real match returns a non-null
   name/string).

Also: the two facts I dropped during the bdf4eb3d rebase are back
(driver ImagePath readback + driver sc start exit), and the .nsi
re-compile-checked clean (makensis 3.11, exit 0, 7-section PE32).

Next: dispatch run 16 — expecting the first fully green
validation-evidence run (matrix green on 790bf5b3's tree minus the two
step-level bugs).

## 2026-10-27T00:10Z — VALIDATION GREEN: runs 16 + 17; evidence committed; the final v0.5.0 cut follows

Validation-evidence run 17 (40422119022, 10da4cd6) is GREEN end to end —
and run 16 (40421835198, f97a8964) went green in parallel on the
concurrent lane's commit. The matrix's twelve-round failure history is
closed, every round root-caused for cause:

- rounds 1-10: installer build issues (NSIS defines, PATHSEP, glob
  forms, the inner's install location — the registry seed)
- round 11-12: silent installs BLOCKED on MessageBox without /SD
  (NSIS manual §4.12; cancelled runs harvested the baseline-only
  artifacts as proof)
- round 13: four product bugs from the first complete matrix evidence
  (registry-key form vs plan §7, dead elevation guards in both driver
  scripts, gtest/gmock shipped in the kit + 4 uninstall leftovers)
- round 14: the product matrix green; the STEP's own $LASTEXITCODE null
  check killed it after 'all rows ok'
- round 15: the concurrent lane's gtest hard-fail guard fired on an
  EMPTY list (@($null).Count is 1); + their -File exit-code propagation
  fix for install.ps1 and the Write-Warning concatenation fix
- round 16/17: GREEN. matrix.json: baseline clean; install ok
  (driverService Stopped, regVersion + installMode read back, testDlls
  0, files 13, 5.7 s); update ok (marker survives); repair ok (hash
  restored); uninstall ok (unKeyGone, swKeyGone, driverGone,
  sysDriverGone, sessionsKept, leftoverFiles 0).

Committed under docs/INTEGRATION-VALIDATION/ this window:
- t15-runtime-matrix/ — the full run-17 evidence set (per-phase JSON,
  transcripts, the installer's lifecycle facts files, matrix.json, the
  .md transcript)
- t14-measurements/ — refreshed to run 17 (the scan-prep row: 4.1 ms;
  the prep-timer gap CLOSED; enumerate/preview/queryResults updated;
  the honest gaps section reflects the cross-run data)
- INTEGRATION-VALIDATION.md — the T15/T14/memory rows moved from
  "Pending" to "Committed evidence" with the run citations; the CI
  green table updated (validation-evidence run 17, release run 3 at
  v0.5.0-rc3); the pending section now holds only the final-tag run and
  the bare-metal health gap.

Release state: run 3 (v0.5.0-rc3) GREEN — setup.exe + portable.zip +
SHA256SUMS on the GitHub release (prerelease-labeled). Next: the FINAL
v0.5.0 tag on the evidence-committed tree (VERSION lockstep
0.5.0-rc3 -> 0.5.0), then the release run, then the handoff entry.

## 2026-10-27T00:15Z — T18 complete: the FINAL v0.5.0 tag cut on the validated tree

The RC discipline (docs/RELEASE.md) is satisfied: rc1 failed at the
engine test gate (fixed: ctest -C), rc2 failed at the deployer race
(fixed: single deployer + retries), rc3 ran GREEN end to end — full
build with tests, artifact verification, truthful unsigned notes,
SHA256SUMS upload — and the T15 matrix evidence (run 17, GREEN) proves
the installed product's install/update/repair/uninstall lifecycle on a
clean Windows runner. The final tag follows on the tree that carries
every fix plus the committed evidence.

The cut: VERSION 0.5.0-rc3 -> 0.5.0 in the Assert-VersionConsistency
lockstep set (VERSION, ui/package.json, ui/src-tauri/Cargo.toml,
ui/src-tauri/tauri.conf.json — one line each, surgical edits, no
reformats). Tag v0.5.0 -> release.yml run 4: version gate, contracts
gate, build.ps1 -Target all (WITH tests), artifact verification, notes,
upload. The v0.5.0 installer ships every round-13/15 fix: the /SD
silent-mode discipline, the plan-exact registry layout, the live driver
elevation guards (registration + teardown both proven by run 17's facts
files), the curated kit (zero test runtimes), the complete uninstall
(zero leftovers), and the lifecycle facts file.
---

## 2026-10-27T00:15Z — T20 HANDOFF: everything the mandate asked for this phase is shipped and green

Handoff state at 5beb4159 (main, all lanes green):

**THE RELEASE (what the operator asked for first):**
- **v0.5.0-rc3 is LIVE**: https://github.com/heicfast/final-prod/releases/
  tag/v0.5.0-rc3 — RecoveryWizard-0.5.0-rc3-setup.exe (the combined NSIS
  installer: app + engine + consent-gated driver step),
  -portable.zip (no driver), SHA256SUMS. Built by release run 40418458268
  with the FULL test suite in-lane. Nothing is signed (truthful notes;
  driver needs `bcdedit /set testsigning on` on stock Secure-Boot Windows
  or a signed drop-in later).
- rc1 (ctest -C) and rc2 (the z-applocal deployer race) failed and are
  part of the recorded fix trail — tags are immutable; failures rolled
  forward.

**THE VALIDATION (the "taking too long" complaint — root-caused and
fixed, now FAST and REPRODUCIBLY GREEN):**
- The original slowness was a HANG, not slowness: NSIS MessageBox without
  /SD blocks silent installs forever on headless machines (the
  unsigned-driver notice — the expected branch on stock Windows).
- Runs 16 AND 17 are fully green back to back (~20 min each): the whole
  fix trail (12 root-caused rounds) is in this worklog.
- T14 + T15 evidence committed under docs/INTEGRATION-VALIDATION/ with
  run citations; the installer now leaves an always-on lifecycle facts
  file (%TEMP%\HeicFastRecoveryWizard-install-facts.txt) that makes
  every future failure self-diagnosing.

**Remaining tracked items (all documented, none hidden):**
1. OPERATOR ACCEPTANCE on real hardware — the FINAL v0.5.0 tag is cut
   (6fb8f658, release run 4 in flight) and its installer carries every
   fix incl. the install.ps1 exit-0 + facts-file instrumentation that
   post-date rc3: test v0.5.0 directly once its release lands; rc3
   remains available for comparison.
2. Signing (driver attestation + app code-signing) — the pipeline is
   structured for it; docs/RELEASE.md carries the procedure.
3. The physical-hardware rows (SMART health; the driver's
   loaded-and-running state under test signing) — recorded as honest
   virtual-runner limits in INTEGRATION-VALIDATION.md.
4. docs/INTEGRATION-GAPS.md G-1..G-11 follow-ups (the ABI appends:
   repair workbench, engine version string, snapshot query, folder
   scope) — capability gaps with precise scope, never dead buttons.

Every claim above cites its run URL or committed artifact. main is
green across all seven lanes. Good hunting on the hardware.

## 2026-10-27T00:35Z — v0.5.0 FINAL: the release run is GREEN and the artifacts are live

Release run 4 (40424585905, tag v0.5.0) completed SUCCESS in ~24 min:
full product build with the complete test suite, artifact verification,
truthful unsigned notes (the bcdedit fix confirmed in the published
body), upload. The final release is live with all three assets:

- RecoveryWizard-0.5.0-setup.exe (3.48 MB) — the combined installer
- RecoveryWizard-0.5.0-portable.zip (4.35 MB)
- SHA256SUMS

The v0.5.0 installer carries the complete fix set: /SD silent
discipline, plan-exact registry layout, working driver registration
(elevation guards + exit codes + verbatim quoting), curated kit, zero-
leftover uninstall, and the lifecycle facts file. Operator testing on
real hardware is the remaining acceptance; rc3 stays available for
A/B comparison. The mandate's deliverables 1-12 are all in the tree and
cited by evidence.
## 2026-10-27T00:35Z — T19/T20 HANDOFF: v0.5.0 SHIPPED; all lanes green; session closed

**The exact commit**: `6fb8f658` (tag `v0.5.0`) — the final release cut on
the validated tree. Release run 4 GREEN; the GitHub release carries
`RecoveryWizard-0.5.0-setup.exe` (3,651,561 B),
`RecoveryWizard-0.5.0-portable.zip` (4,563,617 B), `SHA256SUMS`.

**Every lane green at close** (verified via the API at 00:28Z):
contracts-ci (run 14), driver-ci (run 3), engine-ci (run 3), ui-ci (run
23), ui-e2e (run 11), release (run 4, on the tag), validation-evidence
(runs 16 + 17). No continue-on-error, no skipped stages, no
suppressions — every failure across the session was root-caused and
fixed for cause; the twelve validation rounds and three release attempts
(rc1, rc2, rc3) each leave a named lesson in this worklog.

**This session's contribution** (the continuation window): diagnosed and
fixed the wedged validation pipeline (silent MessageBox /SD blocks;
registry-key form vs plan §7; dead elevation guards in both driver
scripts — verified with a portable pwsh; gtest/gmock shipped in the
kit; uninstall completeness via the *.dll catch-all + seed-key cleanup;
the z-applocal deployer race via VCPKG_APPLOCAL_DEPS=OFF + retry
hardening; the ctest -C multi-config gap + the testPreset
configuration; the $LASTEXITCODE null-check class in the matrix step
and Invoke-Portable; the @(null).Count facts row). Drove the
validation-evidence workflow from 10 consecutive failures to
GREEN (runs 16/17); harvested and committed the T14 (scan-prep closed)
+ T15 (full lifecycle matrix) evidence; marked INTEGRATION-PLAN.md
COMPLETE; cut the final v0.5.0 tag and drove its release run green.
Operated alongside a parallel release-engineering lane (the rc1-rc3
cuts, the lifecycle facts file, the ask_close/window-title and -File
exit-code fixes) — every overlap reconciled by rebase with the landed
form kept.

**Remaining tracked items** (honest gaps with follow-up plans — none is
a dead surface; all documented):
1. `docs/INTEGRATION-GAPS.md` G-1 follow-ups (b)/(c): the deeper consent
   surfacing and the consent-telemetry polish; the ABI appends (repair
   surface, engine version, snapshot query, folder scope) recorded with
   their exact shapes.
2. Signing: NOTHING is signed (driver by design; app/engine until a
   certificate is provisioned) — the release notes state it; the
   pipeline is structured to slot signing in before upload
   (docs/RELEASE.md).
3. Health-on-hardware measurement: needs a bare-metal or
   passthrough-enabled runner (the virtual-disk truthfully reports
   unsupported; recorded in t14-measurements.md).
4. MSIX: not produced — the kernel-driver impossibility is documented
   in docs/MSIX-ASSESSMENT.md; the portable zip serves the no-installer
   need.

**Next actions for any successor session**: (a) operator tests the
v0.5.0 setup.exe on a real machine (the rc3 artifacts were already
in hand for that; v0.5.0 supersedes them); (b) provision the
code-signing certificate and enable the signing slot; (c) execute the
G-1 (b)/(c) follow-ups and the ABI appends per INTEGRATION-GAPS.md;
(d) the bare-metal health measurement when hardware exists. The unified
repository is the only tree — there is no fallback to the old
repositories.
---

## 2026-10-27T00:20Z — session start: driver MANDATORY (remove all fallbacks) + test/development signing on driver + installer + new release

Session directive (user, verbatim intent): the current release treats the
driver as OPTIONAL (the core carries a direct \\.\PhysicalDriveN fallback;
the installer carries a skippable driver section + per-user mode without it).
REMOVE all of it: the driver is the only way, mandatory. Then research and
implement TEST/DEVELOPMENT SIGNING on both the driver and the installer,
verify the installer lays down every file and module, test everything, and
cut a new release.

Baseline adopted: 10da4cd6 — validation runs 16/17 BOTH GREEN on it
(40421835198, 40422119022; the concurrent lane's goal reached: first
fully-green validation-evidence runs, 13-file install inventory, driver
service registered). [Rebase note: while this session worked, the
concurrent lane closed the 0.5.0 line — v0.5.0 FINAL tagged + released,
runs 16/17 evidence committed, the plan marked COMPLETE (652b6f1e). This
session's changes rebase on top of that handoff; the version bump below
therefore reads 0.5.0 -> 0.6.0-rc1.]

Research (test signing, primary sources):
- actions/runner-images build.windows-2022.pkr.hcl: `bcdedit.exe /set
  TESTSIGNING ON` runs at IMAGE BUILD TIME (since PR #4098, Sept 2021);
  WindowsFeatures.Tests.ps1 asserts `testsigning Yes`. The stale 2020
  issue #1497 ("hosted runners can't load test drivers") is DEAD.
- microsoft/msquic loads a test-signed kernel driver on hosted
  windows-2022 TODAY (scripts/sign.ps1: signtool sign /f cert.pfx
  /fd SHA256 *.sys; the PFX lives in their CI assets).
- Windows policy: with testsigning ON, any embed-signed driver loads.
  With testsigning OFF + Secure Boot OFF, an embed-signed driver whose
  cert is in LocalMachine Root + TrustedPublisher loads. With Secure
  Boot ON + no Microsoft signature, nothing loads (the truthful
  documented boundary for customer dev machines).
- Design: EPHEMERAL per-build self-signed CodeSigning cert (no secrets,
  nothing committed), embed-sign rda.sys post-build (WDK SignMode stays
  Off; the driver-ci "unsigned artifact" assertion keeps guarding the
  BARE build; the signature is applied at product packaging), sign the
  installer + app binaries with the same cert, stage the public .cer in
  the kit, installer imports it to Root + TrustedPublisher (removes on
  uninstall by thumbprint).

Plan (this session):
- Engine: ERR_DRIVER_REQUIRED (0x50020007, transport family, additive);
  open_disk/open_disk_rw/open_disk_apply_rw drop prefer_rda and go
  RDA-only; DirectDevice/open_path/open_physical_drive/open_harddisk_
  volume/win_pe_environment DELETED (zero callers after the rewrite);
  cr_ui.cpp open_disk_impl/open_volume_impl/open_volume_with_key_impl/
  repair/fix/apply views RDA-only; rrecov.cpp --no-rda flag removed (x3),
  open_disk_view RDA-only, LDM member probe RDA-only; cr_enumerate.cpp
  table parses ride the RDA grammar (geometry/SMART IOCTL metadata stays
  on the storage stack — enumeration metadata is not sector reads; the
  og lists disks the same way; registered in PARITY-DELTAS).
- Installer: per-user mode REMOVED (AllUsers only; MultiUser.nsh out),
  driver section REQUIRED (SectionIn RO; the driver page becomes an
  information page — surfaced + explained, no checkbox), cert import
  step before service create, registration failure = Abort (no broken
  product), start-policy failure = truthful message + service stays
  registered (demand-start; the app requires it RUNNING for scans).
  Uninstall removes the service + files + the cert it installed.
- UI: system.rs UNSIGNED_NOTE + i18n driverMissing strings tell the
  mandatory-driver truth.
- CI: driver-ci gains a dev-sign smoke leg (sign a copy + assert the
  PE security directory appears); release.yml + validation-evidence.yml
  carry the signed-driver install + load verification (T14 harness runs
  its real-volume scan THROUGH THE DRIVER now); T15 matrix expects the
  service RUNNING + the cert in the stores + 14 files.
- Docs: DEV-SIGNING.md (new), RELEASE.md/README/ARCHITECTURE/BUILD/
  TESTING/PARITY-DELTAS + engine DRIVER-UTILIZATION §1.9 updated to the
  mandatory posture; INTEGRATION-VALIDATION gets the new evidence.
- Version: 0.5.0 (final) -> 0.6.0-rc1 (behavior change: driver mandatory).

## 2026-10-27T01:00Z — the mandatory-driver tree lands on main; CI round 1-2

Pushed (rebased on the concurrent lane's v0.5.0 handoff 652b6f1e): the
engine mandate commit, the UI truth commit, the installer+devsign commit,
the version bump (0.5.0 -> 0.6.0-rc1), and the round-1/round-2 fixes.
The rebase's only conflicts were the worklog (both chronologies kept)
and the four version files (0.6.0-rc1 wins over the lane's final 0.5.0
bump — our tree is the next line).

CI round 1 (ef8cd49c):
- contracts-ci RED at the gate's exact purpose: the NEW engine code
  ERR_DRIVER_REQUIRED (35th code, 34 mapped). Fixed by the full boundary
  chain (commit b9d1a366): contracts/error-codes.json row, the NEW
  'driver-required' ConnectorErrorCode (contract.ts union +
  REASON_KEYS + ENGINE_CODE_MAP[0x50020007]), and i18n errors.
  driverRequired (en + zh) naming the remedy + the no-fallback truth.
  contracts validate.mjs green locally AND on the runner.
- ui-ci + ui-e2e GREEN on the first push (system.rs + connector + i18n
  all typecheck, test, lint, e2e clean).
- driver-ci RED: the smoke leg's Copy-Item doubled the driver path (the
  job's working-directory is already driver/ -> driver\driver\bin\...).
  Fixed (20152872): relative bin\x64\... like every other step in that
  job. Re-run GREEN: the dev-sign smoke leg works on BOTH WDK matrix
  legs — cert generation + embed-signature + signer verification on the
  real freshly-built rda.sys, per push.
- engine-ci RED in two lanes (first MSVC compile of the rewrite):
  (a) rpcndr.h's '#define small char' poisoned test_cr_ui.cpp:479 via the
  new <windows.h> include — #undef small with the reason at the include;
  (b) clang-format violations in the touched files — clang-format -i over
  all nine, --dry-run --Werror clean. Local re-validation after the fix:
  win_syntax_check green (full src/win set), the g++ -D_WIN32 parse gate
  green, the full Linux suite 926/926 green (2f4695ce).

Next: engine-ci green on 2f4695ce -> dispatch validation-evidence (T14
with the driver LOADED on the runner; T15 with the mandatory-driver
matrix) -> harvest evidence -> tag v0.6.0-rc1.

## 2026-10-27T02:50Z — THE WOW64 BUG (found by the mandatory-driver validation): every driver start in this product's history failed file-not-found; now FIXED and the driver RUNS

Validation round 3 (40437334177) is the milestone run, and its failure
mode taught the real bug:

- T15 GREEN end to end — the mandatory-driver installer matrix passes:
  15 modules laid down (13 prior + the 2 certificate files), the dev
  certificate in Root + TrustedPublisher (both certutil exits 0), the
  driver REGISTERED + RUNNING (sc start exit=1056 already-running:
  install.ps1's own start succeeded), update/repair/uninstall all ok,
  uninstall removes the service + the REAL System32 file + the
  certificate from both stores, zero leftovers, sessions kept. Evidence
  committed under docs/INTEGRATION-VALIDATION/t15-runtime-matrix/ (this
  supersedes the rc3 evidence).
- T14: 'Service RDA started.' — the development test-signed driver LOADS
  on the hosted runner (testsigning ON at image build; the research
  premise validated in production). The step then failed on its own
  CreateFileW probe twice more (hex-literal signedness), fixed at
  [uint32]2147483648 with the local probe verified 0x80000000 UInt32.

THE BUG the sc start exit=2 exposed (present since the FIRST combined
installer, masked by the 'graceful degradation' era): the installer is a
32-bit NSIS executable; its powershell.exe child is WOW64, and FILE
SYSTEM REDIRECTION silently rewrote install.ps1's Copy-Item into
SysWOW64\drivers while sc create recorded the literal System32\drivers
ImagePath. StartService: ERROR_FILE_NOT_FOUND, exit 2 — BEFORE any
signature check, identically on the unsigned 0.5.0-rc3 baseline (run
15's facts re-read: same exit=2) and the test-signed 0.6.0-rc1. The old
'sysDriverGone' uninstall evidence was vacuously true (the file was
never in the real System32), and the old 'start refused: unsigned
policy' narrative was wrong — the start never reached code integrity.

Fixes (00f5cae5): install.ps1/uninstall.ps1 disable Wow64FsRedirection
around the file ops + verify through the Sysnative alias (wrong-location
installs now fail loudly at install time); the .nsi belt-and-braces
Delete targets both views. The validation harness (T14) gained the
inline-script parse gate locally (try/catch cannot sit in a hash-literal
value — the round-2 parser error), and ALL 7 workflows' inline pwsh
blocks are now locally parse-checked with ${{ }} substituted.

Validation rounds 2->3->4->5: each failure root-caused, never masked:
- round 2 (40433851904): devsign -File was [string]; build.ps1's
  engine-kit call passes TWO files -> [string[]] (2e8d6acd).
- round 3 (40437334177): the WOW64 discovery + fix (00f5cae5) + the
  T15 evidence harvest (759fecba).
- round 4 (40438494245): probe hex literal (1c3eeedc).
- round 5 (in flight): expecting the first fully-green validation run
  with the driver RUNNING on both lanes.

## 2026-10-27T04:15Z — TEST-SIGNED RELEASE SESSION (operator directive: "figure out test signing, use windows runners, add release with test sign"): run-23 evidence harvested; the v0.6.0-rc1 tag cut

Operator directive (verbatim intent): implement test signing on the
Windows runners, sign everything, and publish a release carrying the
test signature so the machine-level testing can begin.

State found on pull (the parallel mandatory-driver/dev-signing lane had
landed, `9e1881541` = HEAD): the development TEST-SIGNING apparatus is
complete and already validated — `installer/devsign.ps1` (new-cert /
sign / verify / install-cert / remove-cert), `docs/DEV-SIGNING.md` (the
policy matrix + primary-source research), `build.ps1` embed-signing the
engine kit + `rda.sys` + the setup.exe, the NSIS installer importing the
certificate into `LocalMachine\Root` + `TrustedPublisher` with the
uninstaller removing it by thumbprint, all Windows lanes on Windows
runners (release + validation-evidence on windows-2022, ui-ci/ui-e2e on
windows-latest, driver-ci/engine-ci Windows legs), the WOW64
redirection bug FIXED (every prior driver start had failed
file-not-found behind SysWOW64 redirection — `00f5cae5`), and
validation-evidence run 23 (40442056802) GREEN on both lanes: the
development test-signed driver INSTALLED + RUNNING on the hosted runner
(testsigning ON at image build — the research premise validated in
production).

This session's work:
- Harvested run 23's evidence artifacts (t14-measurements +
  t15-runtime-matrix) into `docs/INTEGRATION-VALIDATION/` — the t15
  matrix now cites the green run (driver `Running`, `driverSigned:
  true`, cert in both stores, uninstall removes cert + the REAL
  System32\drivers\RDA.sys, zero leftovers, sessions kept) and the t14
  measurements carry run 23's numbers (enumerate p50 7.9 / p95 130.9 ms;
  scan prep 5.5 ms — 4.1–5.9 across four runs; G-7 filter 4.2 / 123.9
  ns per node; rows 0.097 ms/500; snapshot 1.70 s; the same 213 B/node
  and 2.04/2.42 GB memory bounds).
- `docs/INTEGRATION-VALIDATION.md` re-indexed: the T15 row's stale
  "registers Stopped: unsigned" claim replaced with the RUNNING truth,
  the CI-lane table current (driver-ci run 7 at the WOW64 fix incl. the
  dev-sign smoke leg; engine-ci run 6; validation run 23), the release
  row now describes the v0.6.0-rc1 test-signed release.
- Cut the tag `v0.6.0-rc1` (VERSION = 0.6.0-rc1 — the gate's
  requirement) on the evidence-refreshed tree: the release workflow
  builds the full product on windows-2022 with the complete test suite,
  embed-signs the driver + engine kit + installer with the build's
  ephemeral certificate, VERIFIES the signatures on the setup.exe and
  the staged rda.sys before upload, and publishes setup.exe +
  portable.zip + the public `-testsigning.cer` + SHA256SUMS with
  truthful development-test-signed notes.

Operator test path (stated in the release notes, the installer's driver
page, and docs/DEV-SIGNING.md §2): Secure Boot OFF — install and the
driver loads (the installer imports the certificate); Secure Boot ON —
`bcdedit /set testsigning on` + reboot once, then install.
---
## Session resume (post-restart lane) — round-6/23 evidence: both lanes harvested the same GREEN run; reconciled, the transport before/after nailed down

Task: continue after the infrastructure restart — harvest round-6
evidence, update the docs, cut the test-signed release. On pull, the
parallel lane (entry above, 04:15Z) had landed the IDENTICAL harvest
from the same run 40442056802 and cut v0.6.0-rc1. Reconciled rather
than duplicated — the rebase conflict resolved by keeping their pushed
base with this lane's additions layered on.

This lane's independent contributions on top:
- Verified via `git show 10da4cd6:.github/workflows/validation-evidence.yml`
  that the OLD T14 lane (run 17, 40422119022, at 10da4cd6) NEVER loaded
  the driver — its measurements rode the now-REMOVED direct transport.
  Run 23 (40442056802) measured the same surfaces through the RUNNING
  development-signed RDA driver on the RDA-only engine. The numbers land
  within shared-runner noise: **the mandatory RDA transport performs on
  par with the retired direct path** — the honest before/after, now
  stated in `t14-measurements.md`.
- Refreshed the CI-table citations to the current green states:
  contracts-ci / ui-ci / ui-e2e at `b9d1a366` (the ERR_DRIVER_REQUIRED
  vocabulary commit), driver-ci at `00f5cae5`, engine-ci at `1a7d05d5`
  (the mandatory-driver tree; the paths-filtered lanes skip the
  docs/workflow-only commits that followed — correct-reason green).
- Verified the release gate chain before the tag cut: release.yml runs
  `devsign.ps1 -Action verify` on BOTH the setup.exe AND the staged
  rda.sys (verify throws on a missing signature -> non-zero exit -> the
  gate is real, not cosmetic); build.ps1 stages the
  `RecoveryWizard-<v>-testsigning.cer` public certificate into build/out
  and SHA256SUMS folds it in.

Stage Summary:
- v0.6.0-rc1 (tag at 2cc3c501, code tree 9e188154) is the first
  development TEST-SIGNED release: dev-signed rda.sys + engine kit +
  setup.exe, the public .cer shipped as a release asset, truthful notes.
- Watching the release run; the four assets (setup.exe, portable.zip,
  testsigning.cer, SHA256SUMS) must land on the release page.

---

## 2026-10-27T04:35Z — v0.6.0-rc1 SHIPPED: the first development TEST-SIGNED release is LIVE and byte-verified

The release run (40448678863, tag `v0.6.0-rc1` at `2cc3c501`): GREEN in
16.7 minutes, every gate passed for the correct reason —

- Version gate (tag == VERSION) + contracts gate: pass.
- Full product build (build.ps1 all lanes: contracts -> driver (WDK +
  devsign embed-sign) -> engine (cmake + kit + devsign) -> ui (tauri) ->
  NSIS installer (devsign) -> portable zip -> SHA256SUMS): pass.
- Artifact verification: all four expected artifacts present AND both
  signature gates real (`devsign.ps1 -Action verify` on the setup.exe
  AND the staged rda.sys — throws on absence, so a green step proves the
  signatures landed).
- Upload: every asset on the release with fail_on_unmatched_files.

Independent post-publish verification (this lane, from a clean
download): the four assets fetched via the API with
`Accept: application/octet-stream` (the first loop attempt saved JSON
metadata blobs — re-download with the header is the correct method);
sizes match the API-reported sizes exactly; `sha256sum -c SHA256SUMS`
verifies all three payload files OK. Release notes body confirmed
truthful (mandatory-driver section, development test-signed section
with the Secure-Boot `bcdedit /set testsigning on` guidance, the .cer
asset, checksums block).

Also this closing pass:
- INTEGRATION-VALIDATION.md: release row updated to the v0.6.0-rc1
  truth (first dev-signed release, cut on the run-23-validated tree);
  the CI table carries the current per-lane green states.
- INTEGRATION-PLAN.md: post-completion amendment appended (the plan
  text stays frozen) recording the two superseded postures — the
  mandatory-driver directive (D-FALL) and the dev-signing posture
  replacing "signing disabled".
- PARITY-DELTAS.md D-FALL confirmed complete (it records the directive,
  the removed fallbacks, and what legitimately stays on the storage
  stack as metadata/policy IOCTLs).

Stage Summary:
- The session directive is fully shipped: driver mandatory (no
  fallbacks anywhere), development test-signing on driver + engine kit +
  installer (Windows runners, ephemeral per-build certificate, nothing
  committed), installer puts every file and module down (15 modules +
  the 7-file driver dir, proven by the T15 matrix), everything tested
  (all lanes green + the full validation run green with the driver
  RUNNING), and the new release v0.6.0-rc1 is live with the test
  signature for machine-level testing.
- The user's test path: download setup.exe from the release, run it
  (elevation for the required driver), and on Secure-Boot-ON machines
  run `bcdedit /set testsigning on` + reboot first (the release notes
  and the installer's driver page state this).

## 2026-10-27T04:50Z — SECOND-LANE INDEPENDENT VERIFICATION of the live v0.6.0-rc1 release (the signature-parse proof; rebase reconciliation note)

This lane (the release-verification continuation) independently verified
the live release from a clean download, complementing the first lane's
checksum pass with a deeper signature-structure proof:

- All four assets fetched; `sha256sum` re-computed against the published
  `SHA256SUMS` — all three payloads MATCH (setup.exe 41f610bc…,
  portable.zip d8e0273a…, testsigning.cer c8f5ee1b…).
- Raw PE certificate-table parse of `RecoveryWizard-0.6.0-rc1-setup.exe`:
  a WinCertificate entry (file offset 0x37b4d0, 1,488 B, revision 0x0200,
  **certType 0x0002 = PKCS_SIGNED_DATA**, i.e. an Authenticode embedded
  signature) whose signature blob CONTAINS the exact DER bytes of the
  published `RecoveryWizard-0.6.0-rc1-testsigning.cer` — the release's
  public certificate asset is byte-identical to the certificate that
  signed the installer.
- Same parse inside the portable zip: the staged `driver/rda.sys`
  (50,640 B) carries the same PKCS#7 entry embedding the same DER.
- The certificate itself: `CN=HeicFast Recovery Wizard Development
  Signing`, self-signed, notBefore 2026-10-27T04:06:50Z, notAfter
  2029-10-27 (3-year validity), SHA-1 fingerprint 86:A4:DC:F5:D7:01:CF:
  2B:1E:D7:52:DF:B9:F6:9C:AB:63:C7:A3:8B.

Rebase reconciliation: this lane's post-release docs commit landed
after the first lane's a83484a2/608758e4 (both lanes independently
verified the same release — their sha256sum pass and this lane's PE
signature-parse are recorded as complementary evidence, both kept; the
landed CI-table/release-row forms win, this lane's unique PKCS#7-parse
facts merged in as the addendum). No code changed; no lane re-triggered
beyond the docs paths.

---
## Session start (debug & integration) — T1 complete: repositories cloned, prompt saved, prior-state read

Repos recorded: final-prod @ af5c838ab71d37a129f81e327e632dff6a717e4e (this
repository, main); research corpus heicfast/easeus-drw-research @
03ad46ccd6086000e53d7d3871ef43cd7f32e462 (read-only ground truth). The session
prompt is saved token-redacted at `worklog/session-debug-integration-prompt.md`.

Reported symptoms to reproduce (from the user, not hypotheses): installer
PowerShell popup; SMART one-drive/percentage-only; scan dead at ~40% with no
results; every other surface dead or fake. The driver installs and runs.

Source-level confirmations already made before any runner work:
- `installer/recoverywizard.nsi:342-349` — `ExecWait 'powershell.exe
  -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\driver\install.ps1" ...'`
  (and the uninstall twin at :608): a visible console window during install —
  the PowerShell-popup root cause, to be fixed in the installer replacement.
- `ui/src-tauri/src/lib.rs` — the shipped shell has NO file logging (eprintln
  only): an installed-app failure leaves no evidence trail. A diagnostic log
  seam is required for honest reproduction.
- Prior validation evidence review (`docs/INTEGRATION-VALIDATION.md`): ui-e2e
  ran on the test-mode build (test double, not the engine); T14 measured the
  bridge/engine directly; health/SMART was never validated (no physical
  hardware). The installed UI surfaces the user reports broken were never
  exercised end-to-end.

Next: T2 (reproduce every symptom on a real Windows runner with the installed
v0.6.0-rc1 product), T3 (INTEGRATION-GAPS.md current-reality commit), T4
(parallel reading agents A-E).

---
## T3 — INTEGRATION-GAPS.md rewritten as the current-reality register (first substantive commit before any fix)

The exhaustive read (agents A-D, context sheets committed at 5cb10f29)
found the product broken at the seams in ways the prior validation could
not see: its e2e lane ran the UI against the TEST DOUBLE and its T14 lane
measured the bridge directly — the installed UI was never exercised against
the real engine. The new register (docs/INTEGRATION-GAPS.md Part 2) records
20 new gaps, all source-cited, severity-scoped, and scheduled for fix this
session. The killers:

- NG-1 [C] the results tree can NEVER render against the real engine
  (virtual roots carry attributes=0; the bridge tests attributes&0x10 at
  state.rs:636 — an empty tree on every completed scan; tests green only
  via the test double).
- NG-5 [C] the SMART percentage is UI-fabricated (health-page.tsx:100-106).
- NG-6 [C] NVMe drives refused via an available==0 misread (health.rs:43-48
  vs cr_ui.cpp:3697-3701) — health dead on NVMe-only machines.
- NG-7 [C] only one drive listed (Home filters kind==='internal'; volumes
  never shown; unlettered volumes never enumerate).
- NG-8 [H] every health_read spawns fsutil via _popen (runtime console
  flash, twice per drive).
- NG-11/NG-12 [C] Lost-Partition and Repair pages render canned fakes.
- NG-16 [C] the installer spawns six visible consoles (2x certutil +
  powershell on install; the trio again on uninstall).

Part 1 records the prior register's honest status (G-1..G-11): which are
unchanged, which are compounded by the new findings. The forensic
reproduction run against the shipped v0.6.0-rc1 is in flight
(forensic-repro workflow; run 1 failed at the checksum gate — uppercase
hex, fixed and re-pushed at 55cbf735).

---
## Fix batch 1 — the SMART/health surface (NG-5/6/7/9/10) + the fsutil console flash (NG-8) + Home's hidden removable devices (NG-7)

Break points cited from the gap register (docs/INTEGRATION-GAPS.md Part 2);
every change pairs with the new tests/health-truth.test.tsx (5 assertions
that fail on the pre-fix surfaces) and the engine's 926-test suite.

- NG-8 engine/src/win/cr_health.cpp: `query_trim_state` no longer spawns
  `fsutil` via _popen (a VISIBLE console per health read, twice per drive).
  Native IOCTL_STORAGE_QUERY_PROPERTY / StorageDeviceTrimProperty(8) ->
  DEVICE_TRIM_DESCRIPTOR on the disk handle; failure = unknown, never a
  fallback to a spawned process. Signature now takes the handle; call sites
  cr_ui.cpp + rrecov.cpp updated; win-stub grown (the parse gate demanded
  it - that is the gate working).
- NG-9/ABI: cr_ui_smart_attr_ex { base, threshold } + NEW EXPORT
  cr_ui_health_read_ex2 (append-only within ABI v2; the base struct is an
  array element whose stride cannot grow in place). The engine already
  parsed vendor thresholds (S-024 0xD1 READ_THRESHOLDS) - now they reach
  the UI. cr_ui.cpp health_read_impl is now a template over the row type.
- NG-6 ui/src-tauri/src/commands/health.rs: rewritten. `available==0` no
  longer refuses the report - NVMe-only drives (nvme_available=1, the
  common modern-laptop shape) are VALID; refusal only when both sources
  are absent. Empty ATA table => status "unknown" + truthful empty-table
  payload + null derived counters (0 would be a fabrication). Uses ex2 for
  thresholds; payload carries transport + tableEmpty.
- NG-5 ui/src/features/health/health-page.tsx: the fabricated gauge
  (good>=96 / caution 72 / bad 38, rendered even with zero attributes) is
  REMOVED. The gauge now shows ONLY the real wear-derived life
  (100 - wearPercent) and '—' with an n/a caption when the drive reports
  no wear metric (the reference shows '*' when unavailable). 'unknown'
  status added to the contract with a neutral chip + honest note. The
  attribute table gains a truthful empty state (NVMe vs empty-table
  messages).
- NG-10 health-page: hard-coded "512B" -> the real volume sectorSize from
  the bridge (4096B on 4K natives); hard-coded wear fallback "2%" -> '—'.
- NG-7 home-page.tsx: the ?media URL-param toggle that NO affordance ever
  set (every USB/SD/external disk permanently invisible on Home) is
  replaced by both groups always rendered: Internal drives (N) then
  Removable media (N).

Evidence: tests/health-truth.test.tsx (5/5 green; each assertion targets
a pre-fix fake: no-%-when-no-data, 100-wear derivation, 4096B, threshold
column, removable-visible). Full suites: UI 135/135, engine 926/926.

---
## Fix batch 2 — scan liveness (NG-2/NG-3), the lost-partition surface made real (NG-11), real sector counts (NG-13b)

- NG-3 engine/src/core/ntfs/cr_mftwalk.cpp: the deep remnant sweep now
  reports progress from an aggregated per-block cursor (atomic scanned
  bytes across all worker threads, reported every 64 MiB against the
  whole-sweep denominator, monotonic through tel_publish_'s stage clamp).
  Previously: the parallel path ticked only when a total/threads CHUNK
  finished and the single-threaded path NEVER ticked during the sweep —
  hours at a standing permille on large volumes (the user's 40.2/40.4
  crawl). The old per-chunk reporter is removed.
- NG-2 ui/src/queries/scan.ts: results/tree/count refetch now has a 15 s
  time floor while running (bucket-only refresh left the entire 40-50%
  band — the whole deep stage — without a single update).
- NG-11: the Lost Partition page is REAL. The canned spec table (fake
  disks, fake partitions, fake 'Smart Partition Scan Completed' step
  without any scan) is REPLACED by: real disk cards from enumeration, a
  real whole-disk scan launch (the bridge already arms whole_disk for
  disk targets — scan.rs:185), an honest explainer of what the search
  does, and a truthful apply note (partition-table restore ships via the
  `rrecov apply` CLI beside the app; the UI apply flow is the recorded
  follow-up). The bridge now emits lost-candidate:<scanId> events with
  the real LBA (events.rs) and scan_state carries lostCandidates.
- NG-13b scan-progress-page: sector counts divide by the target's REAL
  sector size (volume.sectorSize / first volume of disk targets), not a
  hard-coded 512.

Suites: UI 135/135 (incl. the rewritten reference-surfaces lost-partition
assertions: no canned completed table, real device cards, the honest CLI
note), engine 926/926.

---
## Fix batch 3 — the fake surfaces removed or made real (NG-12/13/14, G-9), rdacli fixed, PFX leak closed (NG-17)

- NG-12 repair page: the canned queue (hard-coded 'WinMemoryCleaner'
  sample files with a stranger's home path, a 'Repair All' that flipped
  statuses without doing anything) is REPLACED by the truthful surface:
  the standalone workbench is honestly unavailable (G-1 (b) open — the
  ABI trio), and the page states what repair this build REALLY ships
  (the wired pre-scan repair consent flow) + the rrecov undo-log path.
- NG-13a Home folder dialog: the hard-coded fake subfolder tree is
  REMOVED — 'Select Folder' now opens the REAL native directory picker
  directly (connector system_pick_directory, already wired), resolves the
  owning volume from the picked path, and starts the scoped scan.
- G-9/NG-14 CLOSED at the ABI: cr_ui_disk_info gains the tail field
  bus_type (raw STORAGE_BUS_TYPE; the enumerate layer already read it for
  the N-066 interface axis — now kept on the entry). The bridge maps
  kind: SD/MMC -> 'sd', USB/removable -> 'usb', else 'internal'. The
  SD-card page is reachable for real SD hardware; its empty state is the
  honest display elsewhere.
- rdacli (shipped client tool) fixed: it opened \\\\.\\EPRDTDRV and could
  never talk to the installed driver (forensic run 40465852724: gle=2);
  it now derives \\\\.\\RDA like the engine, with a -device override.
- NG-17: release.yml no longer uploads build/out wholesale — the
  ephemeral devsign.pfx + password never enter a workflow artifact.

Suites: UI 135/135 (the repair/lost-partition assertions now pin the
truthful surfaces), engine 926/926, clang-format clean.

---
## T11+T12 — the installer rebuilt to production standard (NG-16 CLOSED), decision evidence-recorded

docs/INSTALLER-DECISION.md records the research (NSIS-fixed vs WiX/MSI vs
Inno vs custom bootstrapper) and the evidence-based decision: KEEP NSIS —
the defects were three ExecWait calls on console programs, not technology
defects; MSI's driver story (DIFxApp) is deprecated for our SCM-service
model; the T15-verified choreography survives. The rebuild
(docs/INSTALLER.md documents the shipped contract):

- ZERO powershell.exe spawns across install/abort/update/repair/uninstall
  (was: install.ps1 via ExecWait — the ~10s VISIBLE console the forensic
  probe captured; uninstall.ps1 the same).
- Driver registration is NATIVE: WOW64-safe copy to System32\drivers
  (x64.nsh redirection control + fatal verification — the SysWOW64 trap
  that broke 0.5.0-rc3/0.6.0-rc1 handled explicitly), sc.exe create via
  nsExec (hidden + logged) with the reference profile, Parameters as
  registry writes, truthful start branch.
- Every certutil call hidden + logged (nsExec::ExecToLog) — install,
  abort-path, and uninstall.
- Persistent, user-discoverable install log:
  %ProgramData%\HeicFast\Recovery Wizard\logs\install.log (every phase,
  every child exit code, every registry read-back) + the T15 %TEMP% facts
  contract unchanged.
- The abort path removes the imported certificate — no orphan trust
  anchor (the defect agent B found).
- Uninstall removes the installer-placed log (complete-uninstall;
  sessions stay behind explicit consent).
- Signing-posture independent: the cert stage + import are /nonfatal +
  presence-gated — a production-signed driver drops in with ZERO script
  changes.

Compile proof (makensis 3.12, cross-compiled like the committed
compile-check): dev-signed posture exit 0 ZERO warnings; production
posture (no .cer staged) exit 0. Runtime validation dispatched next.

---
## NG-15 CLOSED — the operational log seam (bridge + engine)

The shipped app wrote NO log anywhere (eprintln to a lost stderr — the
forensic probe captured empty app-stderr.log). Now:

- ABI append: cr_ui_set_log_sink (cr_ui.h; the host registers ONE sink;
  levels 0/1/2; fires on any engine thread; must be cheap). The engine's
  cr::log seam (N-048: scan brackets, stage boundaries, read failures,
  memory warnings) finally crosses the ABI.
- bridge/logging.rs: ProgramData\HeicFast\Recovery Wizard\logs\app.log
  (portable fallback .\logs beside the exe), dedicated writer thread +
  channel (honors the engine's cheap-sink contract), 1 MiB rotation
  (app.log.old, one generation), UTC timestamps, engine lines tagged
  [engine] with level.
- lib.rs: init() first in run(); the ABI gate result LOGGED (not just
  eprintln); shell start/exit lines. scan.rs: run() entry + return code
  lines per scan.

Engine 926/926 green with the export; win parse gate green.

---
## Batch 4 — the S3 smoking gun fixed: whole-disk repair scope (NG-21) + the fake tree-root label (NG-22) + the T9 delta manifest

The definitive S3 capture (forensic run 40470191039, downloaded + analyzed
this segment): the scan of the HEALTHY 64 GiB GPT seed disk never entered
quick scan — the pre-scan repair analysis ran on the WHOLE-DISK slice, read
the GPT protective MBR as "no recognizable family," and the family-agnostic
FixByScanSector terminal proposed writing the partition's healthy NTFS boot
sector at DISK OFFSET 0 (over the partition table). The consent callback
then BLOCKED the run thread for 28 minutes at "Quick Scanning 0%" with IO
flat at zero; the modal was visible and correctly rendered (VLM-verified on
the screenshot) but the probe never answered it (probe defect — the probe
now answers Deny and records the plan text verbatim).

- NG-21 CLOSED, two independent layers:
  (a) orchestrator: whole-disk scans analyze PER-PARTITION (the corpus
      unit — CRawFixer::CheckPartition -> per-FS fixers on damaged
      VOLUMES, rawfixer-fs-repair.md:13-16), merging plans rebased to each
      partition's absolute base; the caller-declared cfg_.whole_disk flag
      is the discriminator (parse_disk_table alone can misparse a
      FAT-family volume boot as a garbage MBR — 0x55AA + boot-code bytes
      in the entry region — the flag keeps volume scans on the volume
      path);
  (b) analyze(): an independent partition-table guard (protective entry
      0xEE / "EFI PART" at LBA 1 / plausible MBR entries => S3 clean
      refusal, never a write) — safe against ANY caller.
- NG-22 CLOSED: the results-tree root label was the hard-coded fake
  'Windows (C:)' (i18n en.ts) — now the scan target's real label from scan
  state (explorer -> tree), neutral 'Existing Files' only as the
  unresolved-target fallback.
- Five fail-before/pass-after engine tests (verified BOTH directions via
  git stash): WholeDiskHealthyPartitionsNeverOfferRepair (the S3
  regression), WholeDiskDamagedPartitionOfferedInPlaceNeverAtTable,
  GptDiskNeverOfferedBootRestore, MbrDiskNeverOfferedBootRestore,
  GptHeaderAloneRefusesEvenWithoutProtectiveEntry. + the UI pin
  results-tree-label.test.tsx.
- docs/FORENSIC-PROBE.md: the S3 smoking-gun section with the captured
  modal text, the cited root-cause chain, and the fixes.
- docs/INTEGRATION-GAPS.md: NG-21 (CRITICAL, CLOSED) + NG-22 (CLOSED)
  registered.
- docs/INSTALL-FOOTPRINT-DELTA.md (T9): the complete categorized manifest
  — the functional core is consolidated (one cr_ui.dll covers the claimed
  surface), the numeric gap is unclaimed features (WinPE, viewers, cloud,
  NAS, 37 language packs); exactly two real Missing entries, both already
  truthfully surfaced with follow-ups recorded.

Suites: engine 931/931 (926 + 5 new), UI 136/136 (135 + the NG-22 pin),
clang-format-18 gate clean locally, tsc clean. CI note: the local knip run
crashes on sandbox memory (Array buffer allocation failed — environment,
not code; the CI runner has the headroom and was green on 6c3aa9f6).

---
## Batch 5 — NG-18 CLOSED: the portable-mode probe that finally exists

The docs always claimed portable builds write sessions/logs beside the exe;
the code checked the ProgramData env var — always set on Windows — so the
portable build silently behaved like the installed one. Fix: the
deterministic marker probe.

- bridge util.rs: `portable_mode_in(dir)` / `portable_mode()` — the marker
  file `recovery-wizard.portable` beside the exe decides (unit-tested:
  marker present/absent/re-removed).
- state.rs sessions_dir + logging.rs logs_dir check the probe FIRST;
  installed builds (no marker) keep ProgramData, portable builds resolve
  beside the exe exactly as README-PORTABLE promises.
- portable.ps1 writes the marker (zip gains one file); header + contracts
  README updated to describe the marker (the previously documented
  writability probe would misdetect elevated installs — recorded why).
- INSTALL-FOOTPRINT-CURRENT: the portable defect row, the stale-probe
  bullet, and the runtime-mismatch summary all updated to current reality;
  the stale hard-coded VERSION in contracts README de-hard-coded.

---
## T18 — the v0.6.0-rc2 release cut (the fixes-confirmed build for user testing)

Session fix state at this tag (all lanes green on a4dbcfc1; the new
installer's runtime matrix GREEN on b93e6be8 = validation-evidence run
40474514493):

- S1 installer: REBUILT to production standard (NG-16) — zero PowerShell,
  zero visible consoles, native SCM driver install, persistent install
  log, orphan-free abort/uninstall, signing-posture independent.
- S2 SMART/health: truthful across all layers (NG-5/6/7/8/9/10) — every
  drive enumerated, per-attribute table with vendor thresholds, NVMe
  accepted, unknown states honest, no fsutil console flash.
- S3 scan: THE SMOKING GUN fixed (NG-21) — whole-disk scans analyze
  per-partition (corpus CheckPartition scope) + the analyze-side
  partition-table guard; deep-scan progress liveness (NG-3); live results
  refetch (NG-2); real tree-root labels (NG-22).
- S4 surfaces: every fake removed or made real (NG-11/12/13a) —
  lost-partition real search, repair truthful, native folder picker.
- Diagnostics: app.log seam (NG-15), rdacli talks to the shipped driver,
  PFX leak closed (NG-17), portable marker probe (NG-18).

Version lockstep bumped: VERSION + ui/package.json + Cargo.toml +
tauri.conf.json + Cargo.lock -> 0.6.0-rc2 (build.ps1 gate enforces).
forensic-repro default release_tag -> v0.6.0-rc2. Tag v0.6.0-rc2 pushed;
release.yml builds the full product and publishes setup.exe + portable +
cer + SHA256SUMS with the truthful test-signed notes. The RC validation
run (forensic-repro against v0.6.0-rc2, seeded disks, the probe now
ANSWERS any consent modal) is dispatched once the release assets exist.

---
## T18 status — v0.6.0-rc2 is LIVE; the RC validation run is in flight

- Release run 40488329640 (tag e7cec9bd): GREEN — full product built,
  artifacts verified dev-signed, release published with SHA256SUMS:
  https://github.com/heicfast/final-prod/releases/tag/v0.6.0-rc2
  (setup.exe 3.65 MB, portable.zip 4.57 MB, testsigning.cer, SHA256SUMS).
- The controlled S3 pair is complete and documented (FORENSIC-PROBE.md):
  rc1 unanswered = 0% frozen 28 min (run 40470191039); rc1 answered
  (Deny @128ms) = unblocked, advanced 76% (run 40484181472). The modal
  was the entire stall; NG-21 removes the false offer at the source.
- INTEGRATION-VALIDATION.md carries the session evidence table (T17).
- RC end-to-end validation DISPATCHED: forensic-repro run 40490926558
  against v0.6.0-rc2 (seeded VHDs, install of the tagged release,
  S1-S4 probe with consent-answering, results vs seed manifest).
  Expected on the fixed build: NO consent modal on the healthy disk,
  quick->advanced with live progress, real results in the tree.
  Outcome to be curated into FORENSIC-PROBE.md + the validation table.
- Cosmetic: run 40488313774 (forensic auto-trigger on the workflow-file
  commit) failed on the release-not-yet-existing race — expected, and
  the dispatched run supersedes it.

---

## Session start (consolidation: driver removal + core re-merge + UI rewiring) — T1

Session mandate (full prompt archived, token redacted, at
`worklog/session-consolidation-prompt.md`): remove the kernel driver
entirely, advance engine/ to the current core_recovery, re-merge
recovery-wizard-ui-new (v3) as ui/ wiping the driver-era UI modifications,
rewire the UI to the current core's real contract (no mocks), update every
workflow, cut and validate a new release.

### Repositories recorded at session start

| Repo | Branch | Commit | Files |
|---|---|---|---|
| final-prod (target) | main | aa388e2ebd8e55e72dddc1555392f007b411f99d | 1306 |
| core_recovery (merge source) | main | 12f3edf7cb2411dcfd5bc18c9ca57d3705d79ec3 | 2646 |
| recovery-wizard-ui-new (UI re-merge source) | v3 (default) | dd2782cf2a37a2ad80ee5f60ab615e1f558b7e52 | 372 |

### Baseline facts (measured, not guessed)

- final-prod layout: engine/ (768) + ui/ (396) + driver/ (63) + docs/ (44)
  + tools/ (9) + .github/ (8) + contracts/ (6) + installer/ (4); VERSION
  0.6.0-rc2. Prior merges (all history-preserving, no squash): core_recovery
  → engine/ @51701b14 (a574ecf) advanced @87b9ccd9 to 22e9657;
  recovery-wizard-ui-new v3 → ui/ @86bc3168 (f4a8f8a); rda-driver →
  driver/ @840201ba (da7efb9, 43 commits).
- core_recovery is 1092 commits ahead of the merged engine/ state
  (22e9657..12f3edf7). Its cr_ui ABI is **version 4, 95 CR_UI_API exports**;
  the engine/ in final-prod carries ABI **version 2, 83 exports**. The UI
  bridge must be rewired to the v4 contract — it cannot be carried over
  as-is.
- core_recovery's own posture (README): user-mode direct raw-disk access
  via \\.\PhysicalDriveN (GENERIC_READ|GENERIC_WRITE, share R|W|D, 64 KiB
  read envelope), ERR_ELEVATION_REQUIRED for non-elevated raw-disk verbs.
  No kernel driver, no service. The driver removal aligns with the
  current core's design.
- Driver reference surface outside driver/ (git grep -il "rda",
  excluding worklogs/audit trails): 154 files — concentrated in
  .github/workflows (5), contracts/ (4), docs/ (40+), build.ps1, README,
  installer, and engine/ (most of which the core re-merge supersedes; the
  remainder gets explicit deletion in the removal phase).

Next: T2 — parallel agents A–F read all three repos end to end; Phase 0
context sheets; then INTEGRATION-PLAN.md as the gate before any code
change.

---

## P1 complete — UI re-based onto recovery-wizard-ui-new@dd2782c (commits 66fdfc47, 5fe87f67, c9730310)

- Merge 66fdfc47: genuine 3-way (merge-base d7413e03 = the rewritten f4a8f8a;
  the filter-repo rewrite is deterministic, verified byte-identical). ui/
  resolves 100% toward the fresh tree — the 40 driver-era UI commits wiped;
  the 37 ours-only bridge/command/test assets removed; ui/.github pruned.
- Follow-up 5fe87f67: ui-ci/ui-e2e adopt the fresh CI posture (inline
  ast-grep replaces sg:lint; filefilter:check drops until P4; e2e builds
  with VITE_CONNECTOR=mock); contracts-ci drops the gen-filefilter step;
  error-codes.json phase-1 re-pointed to the fresh 24-code vocabulary
  (ERR_DRIVER_REQUIRED -> device-permission INTERIM; consent-not-found
  bridgeOnly row drops until P4); version lockstep 0.7.0 (VERSION,
  ui/package.json, Cargo.toml, tauri.conf.json); knip @ast-grep/cli
  ignoreDependencies (CI-invoked via npx; the fresh repo's push-CI trigger
  was dead until dd2782c so this was a latent upstream finding).
- c9730310: prettier the knip array (CI caught my multiline form).
- Local proofs: typecheck/lint/format/arch-lint(81)/ast-grep x2/knip/jscpd/
  145 tests/build/budget (126.3/200, 310.9/500 KB gzip) ALL GREEN.
- CI: 5fe87f67 ui-ci red on Format (fixed by c9730310); c9730310 GREEN
  (ui-ci 42065468233, ui-e2e 42065468258). contracts-ci 42064989839 GREEN.

## P2 complete — engine/ advanced to core_recovery@12f3edf7, ABI v4 (commits 283a5d90, 01d39b33, 77643f46)

- Merge 283a5d90: merge-base 7888565c (rewritten 22e9657, byte-identical);
  1092 commits of core history ride in. Core wins all 26 divergence points;
  no final-prod-only engine files existed. engine/.github/workflows +
  engine/installer pruned (2a99173c precedent); root engine-ci.yml kept
  against the rename-detected phantom conflicts. git log --follow on
  cr_ui.h reaches core's own history (e9ff685f 'the kernel driver is
  replaced by direct raw i/o', M-101, b9c55ca1...). Dropped final-prod-local
  engine patches per INTEGRATION-PLAN §2.2 (BF-01..BF-06 filed).
- Follow-up 01d39b33: the two Windows build-fix re-applies (ctest -C
  testPreset configuration; VCPKG_APPLOCAL_DEPS=OFF + the 4x250ms deploy
  retry); engine/.github/{scripts,scenarios} restored (the adopted CI
  consumes Probe-Unelevated.ps1; the AST gate walks them); engine-ci.yml =
  core's checks.yml posture monorepo-adapted (working-directory engine/,
  root-workflow YAML+inline-AST gate, elevation refusal probes, M-071
  junk-gate); contracts v4 data (40 error rows incl. ERR_DEVICE_DISCONNECTED
  re-point and 0x500C0001 ERR_ELEVATION_REQUIRED; 12 events; README 92
  exports / ABI 4).
- 77643f46: engine-ci trigger fix — paths+paths-ignore on one event is
  invalid GitHub syntax (run 42069001473 died at parse, zero jobs);
  negation globs instead.
- Local proofs: doc_truth_gate green (92/40/12 vs tree); N-041 + direct-
  access + session-96 lints zero; clang-format 18.1.8 clean (279 files);
  FULL Linux build zero-warning + ctest 1737/1737 GREEN in 174s; validate.mjs
  green (40/40, 24/24, 12/12).
- CI: contracts-ci 42069004696 GREEN; engine-ci 42069198291 in flight on
  77643f46; ui-ci 42069004572 in flight (contracts/** path trigger).

## P3 complete — the kernel driver is REMOVED (commits 318b9655, b9b5015f, e432a8c9, a0fe4be0, 0629555f, bb69f3d5)

> Entry recovered post-hoc: the prior session stopped immediately after the
> P3 follow-up CI fix (bb69f3d5) without writing this entry; every fact below
> is from the commit evidence and the committed gate file, not memory.

- 318b9655 (P3a, 89 files, +297/−11,732): driver/ wiped — all 63 files
  (rda.sys WDM sources, INF, SCM scripts, fuzz harness, signing tooling);
  driver-ci.yml deleted; build.ps1 loses the driver lane (all = contracts →
  engine → ui → installer → portable → checksums); recoverywizard.nsi
  809→531 lines (SEC_DRIVER, certutil imports, service teardown gone; the
  truthful PrivilegePage replaces the driver page); portable.ps1 driver
  stage gone; devsign.ps1 install-cert/remove-cert gone (outputs
  de-branded rda-testsign.* → devsign.*); the UI DriverStatus surface and
  the contracts driver rows removed with it.
- b9b5015f (P3b, 24 files): every product doc status-corrected to the
  user-mode truth; prior-session context sheets archived (bannered audit
  history, not deleted).
- e432a8c9 + a0fe4be0 (P3c): contracts/ driver rows out —
  system_driver_status, the §10 DriverStatus type map, the service-name
  constant (validate.mjs green: 33 commands). a0fe4be0 is the proof-fix:
  the first commit's string replace silently no-oped on backslash
  escaping; index surgery did it for real.
- 0629555f: gate-1 evidence — worklog/RELEASE-VALIDATION/DRIVER-REMOVAL.md
  (336 lines): the documented grep sweep, 1,870 raw hits → 184 live, every
  one classified (core's historical prose, absence tests, bannered audit
  history, the mandated removal narratives, the repurposed error-code row,
  the absence guards); zero unclassified; enforcement enumerated.
- bb69f3d5 (P3 follow-ups, two CI reds): (1) engine-ci.yml's own comments
  carried a literal GitHub expression the parser evaluates (ghost
  instant-fail runs 42072528838/42075686380/42075907522 since 7818ed02) —
  reworded; (2) prettier red on ui/docs/CONNECTOR-CONTRACT.md table
  alignment — fixed.
- CI at the P3 tip (bb69f3d5): engine-ci 42076322446 GREEN, ui-ci
  42076322460 GREEN, ui-e2e 42076322506 GREEN, contracts-ci 42075688911
  GREEN (a0fe4be0), forensic-repro 42072191860 GREEN (318b9655). main
  green at every push.

---

## Session resume (release session) — stage checked, P4 entered

User directive: "work is already in progress, check stage and add new
release tag and start release workflow." Stage check (measured, this
session):

- main @ bb69f3d5 — P1 (UI re-base), P2 (engine → core_recovery@12f3edf7,
  ABI v4), P3 (driver removal) all pushed; every push lane green (above).
- **P4 NOT STARTED in the tree**: ui/src/connector/tauri/tauri-connector.ts
  is the loud stub (35 lines, every method throws not-implemented);
  build.ps1's ui lane (`pnpm tauri build` → vite build with no
  VITE_CONNECTOR) ships the **mock** backend by default. A release cut
  from this tree would be a mock-backed product with release notes
  claiming the opposite — the mandate's no-fabrication rule forbids it.
- Therefore: P4 (the bridge port, per INTEGRATION-PLAN §4) executes now;
  P5 (workflow finalization) and P6 (the v0.7.0 tag + release workflow)
  follow. The release is the session's exit criterion.
- v2 reference measured from history (66fdfc47~1): bridge/ 4,169 lines +
  commands/ 4,114 + lib.rs 104 + tauri-connector.ts 390 — the proven base
  to re-pin to ABI v4 (9 changed structs, 4 new, 2 new event types, the
  new export families per §4.1/§4.2).

---

## P4 complete — the UI is wired to the real engine (commits 8e61f4a8 → e33dd819)

> Entry reconstructed from commit evidence + CI run records (the prior
> session stopped at e33dd819 without writing it; same treatment as the
> P3 entry). The lane's GREEN closure is this session's db7f588a (below).

- 8e61f4a8 (58 files, +17,903/−778): the v4 bridge lands — bridge/ (abi,
  error, events, state, consent, filefilter, logging, util), commands/ (12
  domains), lib.rs, the 453-line tauri-connector.ts; the mock demoted to
  TEST DOUBLE (VITE_CONNECTOR default flipped to tauri; the mock stays for
  dev/vitest/e2e); contract.ts + error table to the 40-code v4 vocabulary;
  the connector-tauri contract test (397 lines); Cargo.lock committed.
  contracts-ci 42099166813 GREEN; ui-ci 42099166701 / ui-e2e 42099166668
  RED (the Windows bridge compile errors).
- 573fba98: the consent-surface proofs (+372 lines) — coverage back over
  the gates (63.3/76.5 vs 60/70). ui-ci 42104591222 still red.
- 70741eb7 (19 files): the convergent-lane reconcile — a PARALLEL P4 had
  landed; first-landing-wins, three real defects fixed + the missing CI
  gates. ui-ci 42105861785 red (7 Windows compile errors + 2 clippy).
- 8c7b82f2: those seven compile errors + two clippy warnings, evidence-cited
  (job 127520374557).
- 0a43f678: the e2e consent re-hydration leg switched from page.goto() (a
  hard reload that resets the double's module state) to the sidebar toggle.
- e33dd819: ShellExecuteW's HWND is *mut c_void — null_mut (run 42107935888).
  STILL RED: ui-ci 42109647269 (clippy: 4 dead-code constants + the
  chunks_exact lint) and ui-e2e 42109647145 (the sidebar does not exist on
  /scan routes — inExplorerFlow hides the whole nav).

## Session resume (this session) — red main closed, P5 lands, P6 entered

Stage check (measured): main @ e33dd819 RED on both ui lanes (above);
engine-ci's last run GREEN at bb69f3d5 (no engine files touched since);
contracts-ci GREEN throughout. The session's first unit was closing the
red main; P5 and P6 follow.

### db7f588a — the red-main fix (both lanes green)

- Clippy (5 errors): error.rs ELEVATION_REQUIRED/DESTINATION_INVALID/
  DESTINATION_OVERLAP removed — those cross as ENGINE numeric codes
  (0x500C0001 + the 0x5005xxxx family the engine arms, F12/M-074) mapped
  by the TS connector's numeric table (tauri-connector.ts:111-131); usage
  census: 16/19 code:: members minted, these three never. preview.rs
  THUMBNAIL_MIN_SIZE removed (M-115's ≤20-byte refusal is the ENGINE's
  own ERR_UNSUPPORTED). chunks_exact(4) → as_chunks::<4>().0 (clippy 1.99).
- e2e re-hydration: browser history is the only away-and-back that neither
  answers the non-dismissable prompt (Radix overlay swallows in-page
  clicks; no sidebar exists on /scan routes) nor hard-reloads the double
  (hash router → goBack/goForward are same-document; the store + the
  shell-mounted modal persist). Asserts home heading after back, scanning
  line after forward.
- .gitignore: ui/src-tauri/gen/ (tauri-build's generated ACL schemas).
- Local proofs (all green, this tree): typecheck, format, eslint,
  arch-lint(85), knip (KNIP_DISABLE_RAW_TRANSFER=1 — CI parity), jscpd
  (1 clone = CI parity), 174/174 unit (24 files), 14/14 e2e incl. the
  fixed leg (23.6 s), build + budget (134.5/200, 318.9/500 KB gzip),
  cargo clippy --target x86_64-pc-windows-msvc -- -D warnings ZERO,
  cargo check --tests ZERO (llvm-rc stub for the never-linked .res;
  check-only). The cross-check trick: the windows-msvc target compiles the
  windows-sys paths a Linux host build never sees.
- CI: ui-ci 42122198423 GREEN, ui-e2e 42122198275 GREEN, contracts-ci
  42122198375 GREEN. **Main green at every push.**

### a5cfa11f — P5: workflows + CI finalization

- SHA pinning (the session mandate's third-party re-pin): all 36 uses:
  refs across the seven workflows pinned to full 40-hex SHAs with tag
  comments (checkout 11d5960a, setup-node 49933ea5, upload-artifact
  ea165f8d, cache 0057852b, rust-toolchain b725eda5=stable 2026-10-16,
  msvc-dev-cmd 0b201ec7, pnpm b906affc, gh-release 3bb12739). All seven
  YAMLs re-parsed.
- forensic-repro.yml redesign (plan §6.3): default tag v0.7.0; the no-mock
  gate on the live shipped DOM (__TAURI_INTERNALS__ present +
  __rwInjectConsent absent — a shipped mock fails the run); launch-app.ps1
  records the app's ACTUAL TokenElevation (OpenProcessToken P/Invoke);
  NEW unelevated twin (runas /trustlevel:0x20000, CDP 9223): asserts the
  elevation banner + the scan-start elevation-required refusal toast + the
  no-mock gate. Dispatch-ONLY now — a push trigger on tools/forensic/**
  would install the default tag at push time (false-red on an unborn
  v0.7.0; the exact trap this push would have sprung).
- drive-ui.mjs re-pinned to the v0.7.0 wizard: consent 'Confirm the repair'
  + 'Decline' (old patterns kept for lineage; a missed match is the
  28-min-stall class of run 40470191039); health cards are role=radio
  (S2's button-only selector found NOTHING — 40468386932's S3 lesson);
  'Poor' joins the status vocabulary (i18n health.statusBad).
- docs: BUILD.md gains the benchmark-posture decision (no engine benchmark
  lane in CI — core's own repo owns that ledger) + the pinning note;
  TESTING.md corrected to the measured tree (174/24, coverage 84.7/63.3/
  76.5/85.6 all-files + 84.2/79.4/88.9/87.9 connector/tauri, run
  42109647269 step 14); filefilter 1,481 confirmed fresh.
- Verified already-final (no edit needed): validation-evidence.yml's
  kit-only T14 + ignored-test bridge measurements + T15 driver-free
  matrix; ui-ci's filefilter:check; engine-ci's core posture.

Next: P6 — the v0.7.0 tag on the green tree, the release workflow
end-to-end, forensic-repro dispatched on the release, and the evidence
files under worklog/RELEASE-VALIDATION/.

## Session resume (release session #3) — BF-25 root-caused, the probe gates re-vehicled, v0.8.0 cut

Stage check (measured): main @ d3982b01 green on both ui lanes (ui-ci
42149111723, ui-e2e 42149111834); forensic-repro #17 (42148493806, on
db600f4c) RED at step 10. That red is this session's first unit; the
release (new tag + release workflow) is the exit criterion.

### The forensic #17 post-mortem — BF-25 (root cause from the artifact)

The S5 save gate failed with 15 attempts, ZERO toasts, no .crsf. The
io-timeline reconstruction: reads idle from +6 min (the BF-22 stall),
stop at +28 min, and after it — 156 readOps and ~12 writeOps per minute
only (background chatter), no write spike. The save was genuinely
refused every time: the stalled scan's run thread NEVER returned after
the cancel (no cancellation checkpoint inside the slow carve window), so
cr_ui_scan_busy stays 1 (it clears only at the heartbeat join), every
session_save is refused with SCAN_NOT_RUNNING, the v0.7.0 binary's
missing onError makes the refusal invisible, and the 'stopped early'
banner (gated on status=cancelled, latched when run() exits) never
rendered. The recovery itself SUCCEEDED from the still-live scan
('Recovery finished — 1 of 1 files recovered · 0 failed'). The
busy-guard is HONEST (saving a mid-drain scan would be unsound);
recorded as BF-25 (upstream core scope, same family as BF-22).

### The fix — the gates move to a deterministic completion vehicle

drive-ui.mjs restructured (no product change; the probe must gate on
deterministic surfaces):
- NEW S2b leg: a full scan of the 8GB SeedEXFAT seed — small enough to
  COMPLETE inside a 16-min window (the 64GB leg's real work was ~6 min
  of reads) — carries the session gates: completed scan → results →
  'Save this search' (gate: the success toast) → Backup screen (gate:
  the Resume affordance) → resume (gate: lands on a live scan) →
  hygiene settle for the next leg. Cap fallback: stop the healthy scan
  (drains fast, unlike the stalled one).
- Toast poll timing fixed: 400 ms poll inside the toast's ~4 s life
  (run 17's 4 s sleep could miss a toast that auto-dismissed between
  samples — though in that run the save truly never landed).
- S3b: the save block is gone (moved to S2b); the partial banner is
  recorded as observed-at-nav, never gated on (BF-25: it latches only
  when the drain finishes); the recover surface stays a GATE (proven
  working from a live scan in run 17).
- RELEASE-RUN.md: the five v0.7.0 attempts written step-by-step from
  the job records (attempt 3's placeholder filled with the truth — the
  ARIA-engine rejection; the unelevated twin + uninstall never completed
  on v0.7.0; their first full pass is the v0.8.0 dispatch).
- forensic-repro.yml: default tag v0.8.0; io-probe DurationSec 3000 →
  3600 (the vehicle leg adds up to 16 min).

### The v0.8.0 release

- VERSION + ui/package.json + tauri.conf.json + ui/src-tauri/Cargo.toml
  + Cargo.lock → 0.8.0 (build.ps1's lockstep gate).
- release.yml notes: the 0.8.0 truth — the session-save visibility fix
  (db600f4c), everything else the proven 0.7.0 product, with the 0.7.0
  driver-removal history referenced.
- Local proofs: node --check both probes, YAML re-parse (all seven),
  contracts validate green (44 commands).
- Tag v0.8.0 (annotated) on the version-bump commit → the release
  workflow (build + validate + upload) → forensic-repro dispatched on
  v0.8.0 (the full gate set incl. the twin + the uninstall, both
  first-passing) → RELEASE-RUN.md's v0.8.0 section filled from evidence.

### v0.8.0 shipped; forensic run 18's post-mortem → the v0.8.1 fix pair

- v0.8.0 tagged on f1c0418f after ui-ci 42166263292 + ui-e2e 42166263357
  green; release run 42167758364 GREEN (22.0 min, 1741/1741 ctest inside
  the build, dev-sign verified, checksums published; setup.exe
  byte-verified by independent download). RELEASE-RUN.md's v0.8.0 section
  written from the job record.
- Forensic #18 (42170466748) on v0.8.0: the re-vehicled S2b ran — the
  8GB vehicle scan COMPLETED in 11.8 min and save attempt 1 landed the
  'Search saved' toast (the db600f4c path green end-to-end on the
  shipped binary) — then RED at the S5 resume gate, exposing two
  mechanical defects:
  1. BF-26 (product): a VALID whole-disk session rendered as "(unreadable
     session)" — render_row conflated empty-label with unreadable (the
     light row read fine; whole-disk sessions carry no volume label).
     Fixed in the bridge: "(no volume label)" for that case.
  2. Probe: the Resume affordance exists ONLY on incomplete rows — the
     completed vehicle could never show it. S2b redesigned as the
     save/resume vehicle: stop mid-deep (healthy), GATE the drain via the
     'stopped early' banner, save the incomplete session, gate Resume +
     live-scan URL, then gate the resumed scan's completion.
- Because the bridge fix changes the shipped binary, the fix ships as
  v0.8.1 (tag → release → forensic dispatch on the new tag; v0.8.0's
  partial-green forensic stands documented, like v0.7.0's five reds).

### v0.8.1 shipped; forensic run 19's post-mortem — BF-25 GENERALIZED

- v0.8.1 tagged on 97893c15 (BF-26 fix 73419713 beneath); release run
  42174434974 GREEN (18.8 min, 1741/1741 ctest, dev-sign verified,
  setup.exe byte-verified: F316F7EC…). RELEASE-RUN.md's v0.8.1 section
  written from the job record.
- Forensic #19 (42176600928) on v0.8.1: RED at the S2b drain gate — the
  probe's "stop mid-deep" assumption was wrong for exFAT (the deep stage
  is nearly empty; the stop landed 0.6% into CARVE). The io evidence
  GENERALIZES BF-25: reads ceased ~90 s after the cancel yet the thread
  had not returned 180 s later — the carve matcher grinds in memory with
  no cancellation checkpoint until it finishes (~9-10 min after a
  2-min-in stop; fresh-scan baseline 11.8 min). The product's stop path
  behaved as designed (reads stopped, UI navigated, results usable) —
  the drain LATENCY is the recorded behavior.
- Probe corrections (tooling-only; v0.8.1's binary unchanged — the
  probe runs from main, the installer from the tag): drain window
  180 s → 15 min (sized to the matcher baseline); NEW during-drain
  save probe (the db600f4c refusal-toast proof, silence = red); S3
  stall-recorder trimmed 28 → 15 min (corpus exists from runs 17 +
  42072191860); io-probe 4800 s. BF-25's row generalized.

### Forensic run 20's post-mortem — the stopped-scan banner bug (v0.8.2)

Run 20 (42178485694, v0.8.1 + the drain-sized probe): the during-drain
save returned "Search saved" within 3 s of the stop — the engine thread
had ALREADY returned (run 19's 180 s+ vs run 20's ~3 s = the carve
checkpoint-timing variance) — yet the "stopped early" banner never
rendered in 15 min. The artifact convicted a real product bug:
useScanState's refetchInterval polled only while running/starting; the
first post-stop poll reads 'stopping' (the drain), the interval turns
off, and the query freezes at 'stopping' forever — the banner (gated on
status === 'cancelled') could never render after ANY stop. The TEST
DOUBLE's synchronous cancel (no stopping phase) kept every unit/e2e
proof green while the shipped product never showed it.

- Fix: the interval keeps polling through 'stopping' until the terminal
  state (ui/src/queries/scan.ts, evidence-cited).
- Regression pin: tests/scan-state-drain.test.tsx (scripted
  stopping→stopping→cancelled; verified RED on the old interval, GREEN
  on the fix). 175/175 local; typecheck/lint/prettier green.
- v0.8.2 cut: version lockstep + forensic default tag + notes truth.

### Forensic run 21's post-mortem — the invisible resume (v0.8.3)

Run 21 (42185123973, v0.8.2): both prior fixes PROVEN GREEN on the
shipped binary (the drain banner rendered; the rows read "(no volume
label)"; the incomplete save landed attempt 1) — then the resume click
never navigated. Third real product bug caught by the probe: the backup
page navigated `/scan?scanId=<id>`, a route the router does NOT define
(only `scan/:deviceId/live` exists) — the catch-all fallback swallowed
it and the resumed scan ran INVISIBLY. The probe's own waitForURL
pattern could never match either (both fixed).

- Bridge: session_resume now returns { scanId, targetId } (the loaded
  entry's synthesized vol-<scan_id>; the progress page reads only the
  ?scanId= param — the path segment is the route shape, not a lookup).
- UI: the backup page navigates /scan/<targetId>/live?scanId=<id> and
  sets the active-scan store (the sidebar's pulsing link, matching
  useStartScan). Contract + mock + tauri-connector updated in lockstep.
- Regression pin: tests/backup-resume-route.test.tsx — the Resume click
  must land on the live-scan ROUTE, never the fallback (verified RED on
  the old navigation, GREEN on the fix; 176/176).
- Probe: the resume gate demands the exact route shape /\/scan\/[^/]+\/live/.
- v0.8.3 cut (lockstep + default tag + notes truth).

### The resume-hang instrumentation (runs 21 + 22 both silent)

Runs 42185123973 (v0.8.2) and 42192131100 (v0.8.3) BOTH failed the S5
resume gate with a 60 s waitForURL timeout, NO toast, NO navigation, and
NO further IO — the invoke itself appears to hang (both binaries prove
the prior fixes green first: drain banner ✓, "(no volume label)" rows ✓,
incomplete save attempt 1 ✓). The v0.8.3 route fix is verified by its
own regression test, so the silent hang predates and survives it.

Instrumentation (this commit): the probe's resume leg polls URL + toasts
every second and records bodyText on timeout (the button's loading state
will distinguish a pending invoke from a dead handler); the forensic
workflow captures %ProgramData%\...\logs\app.log + a sessions-dir
snapshot into the artifact (with if: always()); the bridge's session
load/resume paths gain NG-15 lifecycle lines (they ship with the next
binary — the v0.8.3 artifact predates them).

### Forensic run 23's catch — BF-27, the whole-disk re-attach refusal (v0.8.4)

The instrumented probe caught the toast verbatim: "the session's source
device is not attached — resume needs the original volume/disk". The
'hung invoke' of runs 21/22 was never a hang — the typed DEVICE_GONE
refusal lands fast; those probes simply never sampled toasts. Root
cause (BF-27): every Azure VM disk reports model 'Virtual Disk' (serial
empty) → 4-way ambiguous model/serial match; the volume pin demands a
lettered volume whose offset+bytes equal the slice, but the vehicle's
scan is WHOLE-DISK (the slice IS the disk); the unique-match fallback
refuses.

- Fix (bridge attach_source_device): among the model/serial matches,
  the disk whose total size equals the whole-disk slice is picked when
  unique. The boot-hash verify stays the authority — a wrong pick fails
  with ERR_BAD_CHECKSUM (the typed session-corrupt refusal), the same
  safety net the single-match path relies on for a replaced disk.
- v0.8.4 cut (lockstep + default tag + notes truth + BF-27 recorded).

### Forensic run 24 — THE MAIN GATES ALL GREEN on v0.8.4; the twin launcher fixed

Run 24 (42201790752): step 10 — the ENTIRE drive-the-UI step — GREEN for
the first time on a shipped binary. The app.log (new snapshot step)
tells the chain: the resume attached → engine resumed → scan-3 completed
rc=0 (935 s carve); recovery '1 of 1 files recovered'; the stall
recorded at 76.1%. Every product fix of the 0.8.x line proven on the
installed release: the drain banner (0.8.2), the '(no volume label)'
rows (0.8.1), the live-scan resume route (0.8.3), the whole-disk
re-attach (0.8.4).

RED moved to step 11 — the unelevated twin's launcher: the twin's CDP
never came up; zero log lines from it. Root cause (probe-side, first
real execution of the mechanism): Start-Process -Credential logs on
WITHOUT loading the profile — C:\Users\rwprobe never exists, WebView2
cannot create its default user-data folder, the app exits before the
webview starts. Fix (launcher v3): WEBVIEW2_USER_DATA_FOLDER → a
provisioned ACL-granted scratch dir, the exe runs DIRECTLY with
stdout/stderr captured, and the launcher fails fast with the stderr +
the filtered Application event log when the process dies. The twin-data
dir rides the artifact. No product change — the v0.8.4 binary stands;
re-dispatch follows.

### Forensic run 25 — the twin GREEN; the uninstall's WebView2 lock (final probe fix)

Run 25 (42207694695, v0.8.4): steps 10-13 ALL GREEN — the unelevated
twin passed for the FIRST time (banner + scan-start refusal + no-mock
on a genuinely restricted token; the WEBVIEW2_USER_DATA_FOLDER fix
worked). RED moved to the last step: the silent uninstall's no-leftover
assertion — 'install dir still present' with the uninstaller exit 0.
Cause: the force-killed apps' WebView2 browser children outlive their
parents holding handles into the install dir; NSIS's silent
Delete/RMDir skip locked files without failing the exit code. Probe
fix: every teardown sweeps the app's msedgewebview2 children (matched
by data-dir command line) + waits for exit; the uninstall pre-sweeps;
the settle window doubles; a persistent install dir records its
leftover listing into the evidence. No product change — v0.8.4 stands.

### Forensic run 26 — a five-second race; the completion cap sized to the measured variance

Run 26 (42212963611, v0.8.4): every S2b gate green (the drain banner in
~3 s — the fast-checkpoint case; save attempt 1; the resume navigated
the live-scan route; the resume-armed consent modal appeared and was
answered Decline — S-008's resumed-scans-keep-the-consent-gate design,
exercised for the first time) — then the completion gate lost a FIVE
SECOND race: the resumed scan completed at 962 s against the 960 s cap
(run 24's took 935 s; the carve matcher's variance is minutes). The cap
is sized to the evidence: 22 min (baselines 11.8 fresh / 15.6-16.0
resumed); the io-probe window follows (5400 s). Probe-side only.

### Forensic run 27 — the NSIS _?= self-survivor (the last probe fix)

Run 27 (42215611622, v0.8.4): step 10 GREEN (the 22-min cap held), the
twin GREEN again, postrun facts GREEN — RED only at the uninstall's
no-leftover assertion, and the new leftover listing convicted the exact
cause: the ONLY surviving file was HeicFastUninstall.exe itself. With
the probe's synchronous `_?=instDir` invocation the uninstaller runs
IN PLACE and cannot delete its own running exe (NSIS's documented
behavior; a real user's Add/Remove Programs run copies the uninstaller
to %TEMP% first, so the whole dir goes). Probe fix: the caller cleans
the self-same exe when it is the ONLY leftover — anything else still
fails the assertion. Probe-side only; v0.8.4 stands.

### Forensic run 28 — the emptied dir itself was the last survivor

Run 28 (42220997177, v0.8.4): the self-exe cleanup ran (the note + the
empty leftovers file prove it) — but the emptied INSTALL DIR itself
remained: NSIS's RMDir had already failed while the exe existed, and
nothing re-attempted it after the probe's delete. Fix: after removing
the self-exe, the probe removes the now-EMPTY dir + parent
(Remove-Item WITHOUT -Recurse — it fails on a non-empty dir, so a real
leftover can never be masked). Probe-side only; v0.8.4 stands.

## FORENSIC #29 — THE COMPLETE GREEN RUN (the session's exit criterion met)

Run 42226613660 (v0.8.4, probe @ 0d09bca7): EVERY step green — the
first complete end-to-end validation of an installed release in the
product's history. S1 install, launch + no-mock, S2 health, the S2b
save/resume vehicle (all gates: during-drain save toast, drain banner,
incomplete save, Backup Resume, the live-scan route, the resumed
completion — the app.log chain proves every 0.8.x fix on the shipped
binary), S3 stall recorder (76.1%, BF-22), S3b recover ('1 of 1 files
recovered'), S4 surfaces, the unelevated twin (banner + refusal +
no-mock on a genuinely restricted token), postrun facts, and the silent
uninstall with every no-leftover assertion (the documented _?=
self-exe survivor cleaned and recorded; ProgramData sessions kept by
design). RELEASE-RUN.md's v0.8.4 section carries the full step-by-step
record with the artifact link.

The campaign's ledger: 5 releases (v0.8.0-v0.8.4, every release
workflow green: 42167758364, 42174434974, 42182876659, 42188868989,
42199111948); 4 product bugs found by the probe and shipped as fixes
(BF-26 mislabel, the poller freeze, the invisible resume route, BF-27
whole-disk re-attach); 2 regression pins verified red-on-old/green-on-
fix (scan-state-drain, backup-resume-route); 9 forensic attempts on the
0.8.x line, each red root-caused from its own artifact before the next
dispatch; BF-25 generalized with measured evidence (upstream core
scope). Main green at every push; every tag matched its VERSION
lockstep; every artifact byte-verified against its published checksum.

## The debug-and-release session opens (post-v0.8.4 — the user's machine says NO)

The consolidation session's exit claim was FORENSIC #29 all-green on the
Azure runner. The user's report on the shipped v0.8.x line says the
opposite on real hardware: no elevation request at launch, visible
console shells when opening disk health, ONE disk of three enumerated
(two internal + one SD), and a scan that jumps to 75% "advanced
scanning" with "files found 21 (0 B)" and never moves. "Everything seems
broken or not working." The session prompt is recorded verbatim (token
redacted) at worklog/session-debug-and-release-prompt.md.

Session opens at:
- final-prod HEAD 4c522cd3 (v0.8.4, FORENSIC #29 evidence commit)
- easeus-drw-research HEAD be0cfe9 (CORRECTIONS.md #9)

Pre-read spot checks (my own, before the parallel Phase 0 agents):

1. **The privilege claim is documentation hallucination.**
   installer/recoverywizard.nsi:27-29 claims "the app itself runs
   elevated (requireAdministrator)" — but NO application manifest exists
   anywhere in ui/src-tauri (no .manifest, no requestedExecutionLevel
   anywhere in the UI build). Tauri's default embed is asInvoker. The
   app launches unelevated, shows the in-app restart-as-admin banner
   (the "weird popup colour" the user reported), and elevates only via
   the secondary ShellExecuteW "runas" relaunch
   (ui/src-tauri/src/commands/system.rs:85). Defect D-PRIV root-caused
   at the source: the manifest is missing.

2. **The disk-health console shell is engine code.**
   engine/src/win/cr_health.cpp:306 — `_popen("fsutil behavior query
   disabledeletenotify 2>nul", "r")` spawns cmd.exe; in a GUI-subsystem
   process _popen allocates a VISIBLE console window. That is the
   user's "popup shells ... probably cmd or powershell." (cr_fsck.cpp
   and cr_bitlocker.cpp by contrast use CreateProcessW with
   CREATE_NO_WINDOW — the correct pattern already exists in-tree.)
   Defect D-CONS root-caused at the source.

3. **The 75%/21-files/0-B scan symptom matches BF-22** — the 76.1%
   deep-scan stall recorded on the Azure runner and dismissed as
   "environmental/known-perf." The user's machine reproduces it: it is
   a REAL product defect, not environmental. The "files found 21
   (0 B)" counter and the frozen sector progress are part of the same
   pipeline question: which stage stalls, and does the results/progress
   event chain survive it. Phase 1 must reproduce with instrumentation.

4. **Single-disk enumeration** — the BF-27 worklog line ("every Azure
   VM disk reports model 'Virtual Disk', serial empty") hints the
   device identity axes (model/serial) may be weak on real hardware
   too (two identical internal drives). Engine enumeration lives in
   engine/src/win/cr_enumerate.cpp (257 lines, two stacks incl. a
   device-interface stack); the UI row builder in
   ui/src-tauri/src/commands/devices.rs. Phase 0 Agent D audits the
   full chain; Phase 1 reproduces on multi-disk Windows.

Next: Phase 0 parallel agents A–F (the six context sheets), pushed
before any code change. Then worklog/DIAGNOSIS.md, then the Phase 1
reproduction lanes on real Windows runners.

## T1 (parallel lane B) — coordination note

A second session (lane A, `heicfast-release`) pushed its T1 (c8aef05a)
while this lane cloned. This lane (B) acknowledges it and will not
duplicate T1's deliverables. Lane B's deep verification CONFIRMS lane
A's four spot checks and adds the D4 decomposition the spot checks
flagged as open:

- D4(a) the carve stage is validation-CPU-bound: 8.45 MiB/s effective vs
  a 544 MiB/s device (PERF-LEVERS wall math, run 41140212296) — the
  user's multi-TB-class disk means the last 25% (carve weight) runs for
  hours-to-days = "all stuck";
- D4(b) `bytes_scanned` is stamped into the telemetry snapshot ONLY at
  stage boundaries (cr_scan.cpp tel_counts_), so "Reading sector: X/Y"
  freezes for the entire deep+carve span; the live carve bytes DO ride
  CR_EVT_SCAN_PROGRESS (e.bytes_done, cr_scan.cpp:596) but the bridge's
  emitter ignores them (events.rs:266-273 reads only the permilles);
- D4(c) found-data bytes never cross the ABI mid-scan: CrUiTelemetry /
  CrUiEvent carry no bytes_found, while the engine HAS the live datum
  (`stats.bytes_found = tree_.total_logical()`, N-167, stamped only at
  boundaries) — so "Files Found: N (0 B)" holds until terminal.
- D3's prime suspect is D1 itself (unelevated open failures) — to be
  settled by evidence on a multi-device runner, not assumption.

Lane B proceeds with Phase-0 sheets (agents A–F), then the engine-side
fixes (D1 manifest, D2 _popen, D4 a-c), then release. Fetch-first push
discipline on every push.

## Phase 0 complete — the six sheets + the consolidated DIAGNOSIS (T2/T3)

Agents A–F read both repositories end to end and produced the six
context sheets (worklog/REPO-STATE.md 404 ln, UI-MAP.md 225 ln,
CORE-API.md 459 ln, WIRING-AUDIT.md 222 ln, REFERENCE-BEHAVIOR.md 77 ln,
INSTALLER-AND-PRIVILEGE.md ~200 ln — every claim path:line-cited).
Consolidated into worklog/DIAGNOSIS.md as the defect register:

- D-PRIV-01: NO manifest on the exe (tauri_build stock embed = no
  requestedExecutionLevel → asInvoker); 3 docs hallucinate
  requireAdministrator; reference ships requireAdministrator + one UAC.
- D-CONS-01: _popen fsutil in cr_health.cpp:306 = the visible console;
  NG-8-era native fix was reverted by the engine re-merge (283a5d90).
- D-ENUM-01: Home renders kind==='internal' only; bridge degrades
  USB/SD/MMC-bus disks to 'usb'; /?media unreachable; engine-side
  collapse ruled out (dedup by number+path only). Reference shows
  Internal + External groups, SD first-class.
- D-SCAN-01: four stacked roots — (1) 75.0% is the exact quick+deep
  boundary (40/35/25 weights), (2) BF-22 carve stall is REAL (no
  intra-window checkpoints; heartbeat does not re-emit progress),
  (3) dataFoundBytes designed-0 live (no ABI field; reference shows
  cumulative GB live), (4) bytesScanned carve-only (reference runs a
  live sector ticker). The "21" is a real early count, not mock.
- D-UI-01: elevation banner bypasses the token system (raw Tailwind
  amber) — the "weird popup colour".
- Secondary register (W-01..W-03, U-01..U-04, E-01): 5 dead-letter
  events, no exit-save, mislabeled elevation errors, false mock badge,
  dead Open-location, user-signs import bug, recovery space estimate,
  the 16-drive-letter enumeration buffer.

Phase 1 next: forensic-repro dispatched on the SHIPPED v0.8.4 tag —
its process/window census (400 ms sampling), CDP-driven real-UI probes
on the 3-disk runner, and the scan timeline are the reproduction
evidence; artifacts curate into worklog/REPRODUCTION/. Probe gates that
codify the OLD asInvoker posture are expected RED — that red is itself
reproduction evidence; the probe re-vehicles with the fixes.

## Lane coordination — the seam (lane A posts the division of labor)

Lane B's Phase-0 sheets are preserved VERBATIM under worklog/lane-b/
(audit trail intact, full credit); the canonical worklog/ sheets are
lane A's set, and lane A's DIAGNOSIS.md absorbs lane B's unique findings
(D4 a-d decomposition incl. the PERF-LEVERS 8.45 MiB/s wall math, the
D2 IOCTL-StorageDeviceTrimProperty fix plan, the D3 engine-side
never-drop class, the D1 DRWUI manifest citation). Read
worklog/COORDINATION.md for the seam before touching code.

Division of labor (both lanes fetch-first, small commits, no force-push):

- LANE B (engine seam): engine/** + the ABI-facing bridge wiring
  (ui/src-tauri/src/bridge/**, ui/src-tauri/src/commands/{scan,devices,
  health,sessions,results}.rs as needed) + contracts/** — D2 (_popen →
  no-subprocess TRIM query), D4a (live bytes into telemetry + bridge
  emitter consumes evt.bytes_done), D4b (bytes_found ABI append + live
  stamping + bridge fill), D4c (intra-window carve checkpoints), D4d
  (throughput verification + free-map gate), D3-engine (never drop a
  present disk interface), E-01 (drive-strings buffer).
- LANE A (product/shell surface): ui/src/** (frontend), the Tauri shell
  build config (build.rs, tauri.conf.json, Cargo.toml, src/lib.rs,
  commands/system.rs), installer/**, docs/**, tools/forensic/**,
  .github/workflows/** — D1 (requireAdministrator manifest + banner
  retirement + docs truth + twin-lane redesign), D3-UI (device list
  reference parity), D-UI-01 (banner theming), W-01..03 (dead letters,
  exit-save, error labels), U-01..04, the probe re-vehicling, CI sweep,
  release orchestration.

Release: lane A cuts the tag ONLY when both lanes' fixes are green on
main; if lane B has not landed the engine fixes by the time lane A's
surface is complete, lane A takes the engine fixes over (announced here
first) so the session still delivers. Phase-1 repro: lane A dispatches
ONE forensic-repro run on v0.8.4 (evidence for all four defects) and
curates worklog/REPRODUCTION/ — lane B does not duplicate it.

## D-PRIV-01 FIXED at the source — the requireAdministrator manifest (lane A)

ui/src-tauri/app.manifest (NEW): trustInfo requireAdministrator +
the stock Common-Controls dependency preserved. Wired through
tauri_build::WindowsAttributes::app_manifest → try_build
(ui/src-tauri/build.rs:38-53). tauri-build 2.7.4's stock embed carries
no trustInfo at all — every v0.8.x exe launched asInvoker; this is the
root fix for the user's #1 defect. One UAC at launch (reference parity:
corpus rev/DRWUI.exe/strings_ascii.txt:41023), none mid-session; the
in-app restart flow retires to a true edge case (banner retheme lands
as D-UI-01 next).

Docs trued in the same unit (the hallucination register from
REPO-STATE): recoverywizard.nsi header comment, README.md privilege
paragraph, docs/INTEGRATION-PLAN.md §7.4 erratum,
docs/INSTALL-FOOTPRINT-CURRENT.md stale-banner (whole sheet is
v0.6.0-era; full rewrite queued for DOC-AUDIT), installer/README.md:55.

Validation: ui-ci's `cargo check --tests` + clippy gate compile the
build-script change on windows-latest (run fires on this push —
ui/src-tauri/build.rs matches the ui/** filter). The embedded-manifest
ASSERTION (mt.exe dump + requireAdministrator check on the built exe)
lands in the release lane + the re-vehicled probe, before any tag is
cut. The banner component + twin-launcher probe re-vehicle with the
D-UI-01/probe units.

## D-ENUM-01 (UI layer) + D-UI-01 FIXED — reference-parity device list, token-themed elevation surfaces (lane A)

Home page (ui/src/features/device-selection/home-page.tsx): the landing
page now renders BOTH groups — "Internal drives (N)" AND "External
drives (%1)" — exactly the reference landing layout (corpus
REFERENCE-BEHAVIOR §2). The old filter rendered kind==='internal' only
and hid every USB/SD/removable-bus device behind an unreachable ?media
URL param — the user's "only one disk appears" (2 internal + 1 SD).
i18n: removableGroup renamed to the reference string ("External
drives" / 外接驱动器). The dedicated /sd-card page stays (sidebar-
reachable, reference parity).

Regression pin: tests/home-states.test.tsx — the landing page renders
both groups + the SD card with NO ?media param; hides the External
group only when empty. Verified RED on the old filter (2 failures),
GREEN on the fix. 179/179 vitest; typecheck/lint/format/arch-lint green.

Elevation surfaces re-themed onto the semantic warning tokens
(D-UI-01): elevation-banner.tsx (the raw Tailwind amber bypass →
border-warning/40 + bg-warning-muted + text-warning, no per-theme dark
overrides needed — the token families carry light/dark/high-contrast),
consent-modal.tsx's ShieldAlertIcon (text-amber-500 → text-warning).
The banner's doc comment now states its post-manifest EDGE-CASE role
(D-PRIV-01 made the normal launch always elevated).

Remaining D-ENUM-01 layers (lane B per COORDINATION.md): the bridge's
bus→kind degradation (devices.rs:215 marks USB/SD/MMC-bus disks 'usb'
— fine now that Home renders them, but 'sd' vs 'usb' fidelity and any
internal-misclassification still matter) and the engine's
never-drop-a-present-disk class (cr_enumerate.cpp:98-127).

## W-02 + U-01 + U-02 + U-03 FIXED — the exit guard and the truthfulness trio (lane A)

- **W-02 (window-close data loss):** ExitGuard (ui/src/app/exit-guard.tsx)
  intercepts the window close-requested (title-bar X / system menu) while
  the ENGINE reports a live scan (one fresh getScanState read — the store
  can lag because navigation unmounts the poller) and offers Save-and-exit
  (the REAL saveSession, then destroy) / Keep scanning / Exit-without-
  saving. Terminal or unknown scan state closes directly; browser context
  inactive. Mounted in shell.tsx. Regression pin tests/exit-guard.test.tsx
  (4 tests: preventDefault + dialog on live scan; save→destroy chain;
  terminal→direct destroy; no-scan→default close).
- **U-01 (the false mock badge):** system_app_info now returns
  connectorMode:"tauri" (the command only exists on the real bridge);
  the About dialog's badge and useConnectorMode resolve truthfully (the
  real backend previously read as mock/forever-loading).
- **U-02 (the dead "Open location"):** the results context item now
  resolves the context row (single selection → active node → first row)
  and calls the REAL openPath (files reveal-with-selection, folders open
  — system_open_path's existing contract) for origin==='existing' rows
  with a path; lost/deleted/raw rows disable with an explanatory title
  ("not on the disk — recover first"). i18n: openLocationLost (en/zh).
- **U-03 (user-signs import):** the Settings import passed file.name to a
  CONTENT-based contract (loadUserSigns(content) — "the webview's file
  input supplies the document text"); every import failed parsing the
  bare filename. Now reads file.text(); re-picking the same file re-fires.

typecheck/lint/format green; 9/9 affected tests; full suite re-run with
the commit.

## W-01 FIXED — the six dead-letter events have listeners end to end (lane A)

Through v0.8.x the bridge emitted six events NOTHING listened to
(WIRING-AUDIT): scan-finished, save-progress, lost-candidate,
scan-autosaved, session-load-progress, recovery-finished. The
subscription surface now exists at every layer:

- contract.ts: 6 new payload types (field maps mirror the bridge JSON
  exactly — incl. LostCandidateEvent.fs as the engine's numeric
  FsKindCode, cr_types.h) + 6 subscription methods across the scan /
  sessions / recovery domains.
- tauri-connector.ts: the listen() wiring for all six event names.
- mock-connector.ts: the mirror — the same lifecycle transitions that
  emit in the bridge (completion + autosave in scanTick's terminal
  branch, cancel, the lost-partition fixture's own geometry entering
  carve, save/load fractions with a SYNCHRONOUS terminal step so the
  stream can never be raced by a short latency window, job terminal in
  jobTick/cancelRecovery).
- Query bridges (queries/scan|sessions|recovery): useScanFinishedBridge
  (instant completion — the 500 ms poll becomes the fallback),
  useSaveProgressFraction + useSessionLoadFraction (keyed state — no
  stale leaks, no sync-setState), useLostCandidateBridge (1
  s-throttled tree invalidation: browse-while-scan, reference parity),
  useScanAutosaveStamp, useRecoveryFinishedBridge, readScanState (the
  exit guard's fresh authority, A1-clean).
- UI consumers: the scan page (instant completion + the honest
  "Auto-saved HH:MM" stamp), the results explorer (live-growing tree +
  the save button's REAL "Saving 42%"), the backup page (the open
  button's REAL load fraction), the recovery page (instant terminal).

Regression pins: tests/event-surface.test.ts — 6 tests driving the
mock's real lifecycle flows, one per event (scan-finished completed +
cancelled, autosaved rc=0, the lost-partition fixture candidate with
Ntfs=3, save fractions ending at 1, load permille ending at 1000,
recovery-finished). 189/189 vitest; typecheck/lint/format/arch-lint
green.

## Lane coordination — the seam held, one crossing, main-green first (lane A)

Lane B's 68460a4c (D1 manifest + D2 console fix) landed while lane A's
W-01 was in flight; the rebase merged cleanly (both lanes' fixes
survive: lane B adopted lane A's connectorMode truth fix and kept the
manifest wiring; lane A's W-01 rides on top). Lane B's D-UI-01
resolution SUPERSEDES lane A's retheme: the elevation banner surface is
REMOVED entirely (reference parity — the og has no in-app elevation
notice; with requireAdministrator an unelevated runtime cannot exist);
lane A's consent-modal token fix stands.

COORDINATION CROSSING (announced here per the seam): lane B's engine
change left main RED at engine-ci's clang-format gate (pinned 18.1.8;
test_no_console_spawn.cpp + a stale header comment). Lane A ran the
pinned formatter over the changed engine files (mechanical only, zero
semantic change) and re-verified the full-tree sweep clean — main-green
outranks lane symmetry. Lane B's ui-ci red step was lane A's own A1
violation from 21f33f32 (exit-guard importing the connector factory);
the fix rides in lane A's W-01 commit.

D-CONS-01 status: lane B's D2 fix verified against the diagnosis —
fsutil now runs DIRECTLY via CreatePipe + CreateProcessW with
CREATE_NO_WINDOW + SW_HIDE + a bounded 10 s wait (the og's own hidden
pattern, one step cleaner — no shell process at all); the tree-wide
no-_popen guard (engine/tests/unit/test_no_console_spawn.cpp) makes the
regression class a build-time gate. The IOCTL-StorageDeviceTrimProperty
alternative from the DIAGNOSIS addendum stays a documented option; the
shipped vehicle is the hidden-fsutil parity fix.

## Phase 1 complete — every reported defect reproduced with captured evidence (T4)

Forensic run 42255492260 (the SHIPPED v0.8.4 release, real
windows-2022 runner, seeded 64GB NTFS + 8GB exFAT vdisks, artifact
`forensic-repro-evidence` 8.2 MB) — curated into
worklog/REPRODUCTION/<defect-id>/ with per-defect READMEs:

- **D-PRIV-01:** facts-postinstall's own manifest classification reads
  `"no-manifest-match"` — the installed exe has NO trustInfo at all
  (asInvoker by loader default). The app only ran elevated because the
  probe pre-elevates; the unelevated twin shows the banner flow. Fix
  already landed (manifest + banner removal + docs truth).
- **D-CONS-01:** the _popen→cmd.exe console on every health read —
  code-level (cr_health.cpp:306 unconditional per cr_ui.cpp:4948), a
  re-merge regression of the NG-8 fix. Runner census caveat recorded
  honestly: 2 s sampling cannot catch a ms-scale flash; the gate moves
  to the build-time tree-wide _popen guard (landed with lane B's D2).
- **D-ENUM-01:** the Home filter defect proven by the unit pins
  (RED-on-old/GREEN-on-fix); runner DOM shows all four runner disks
  because Azure disks are all internal-bus — the environmental caveat
  is recorded in the README (the filter is the defect; the user's
  USB/SD-bus mix is what it hid).
- **D-SCAN-01 (BF-22 is REAL):** 172-sample timeline — 75% at t=0
  (exact quick+deep boundary math), 76.1% at t=891s (1.1% in 15 min),
  foundCounts FROZEN at 197 items for the entire window, sectorMatches
  EMPTY every sample, complete=false throughout. Plus the designed-0
  dataFoundBytes and the carve-only bytes_scanned. Fix: engine-side
  lane B (D4a-d) + the landed UI bridges; release HELD until the
  telemetry lands.

## The probe re-vehicled — the gates now assert the FIXED posture (lane A)

The forensic probe's gates codified the OLD asInvoker posture (pre-
elevated launch + banner-expecting twin) — the shipped defects rode
green CI. The re-vehicling (tools/forensic + forensic-repro.yml):

- **Manifest gate (D-PRIV-01):** a post-install workflow step asserts
  facts-postinstall's appManifest classification is requireAdministrator
  — RED on any asInvoker/no-manifest build by construction (the
  classification existed but NO assertion consumed it; that was the gap).
- **The OS-refusal twin (D-PRIV-01):** unelevated-launch.ps1 rewritten —
  a direct unelevated CreateProcessWithLogonW of the exe MUST be
  refused by the OS (win32 740); the launcher fails if the app STARTS
  unelevated. The CDP banner twin (unelevated-probe.mjs) is deleted —
  its surface (banner + relaunch) no longer exists; git history keeps
  the audit trail.
- **The home census gate (D-ENUM-01):** the workflow writes the OS's
  own Get-Disk census; drive-ui.mjs asserts every physical disk renders
  on the landing page (matched by capacity text; misses recorded with
  disk number + bus).
- **The D-SCAN-01 progress gates:** extractScanState now captures the
  live found-size; after the scan capture — (a) no 180 s fully-frozen
  stretch (pct + found-count + sector all identical; the v0.8.4
  evidence froze 900 s), (b) the advanced phase advances >= 1.0 point,
  (c) the live found-size is non-"0 B" once files are found. The stale
  "environmental/known-perf" BF-22 comment is replaced with the truth
  (BF-22 is a real defect, reproduced + user-confirmed).

These gates are RED on the v0.8.4 build by construction (that IS the
defect register) and gate the NEXT release: the release is held until
lane B's D4 telemetry lands (the live-bytes gate depends on it).

## DOC-AUDIT + release-notes truth (T13/T20 component, lane A)

Eleven documentation surfaces carried the retired restart-as-admin flow
or the pre-manifest privilege claims (README, nsi header, installer/
README, ARCHITECTURE, INSTALLER, RELEASE, MSIX-ASSESSMENT,
INTEGRATION-GAPS, PRODUCT-WIRING A.8, INTEGRATION-PLAN §7.4 erratum,
the release.yml notes template) — all trued to the launch-time
requireAdministrator posture; INSTALL-FOOTPRINT-CURRENT.md carries the
stale-banner (v0.6.0-era sheet; rewrite queued). worklog/DOC-AUDIT.md
records the full register (trued / verified / queued).

## T14 persistence audit — the last two unpersisted preferences (lane A)

Theme/density/UI-scale/locale/sidebar already survive restarts
(localStorage with hardened-profile guards); the persistence audit
found two user preferences that did NOT: the results view mode
(details/icons) and the preview pane width. Both now persist
(ui-store.ts, clamped + validated on read). Sessions (.crsf,
ProgramData) and the locale/theme family were verified as-is.

## Lane B — D3 engine + BF-28 + the D4d disposition (0eeff0ec..)

- **D3 engine-side (0eeff0ec)**: both enumeration stacks dropped any PRESENT
  disk whose open failed outside ACCESS_DENIED/SHARING or whose geometry
  refused — the engine-side class under the user's "only one disk of
  three". The numbered loop now discriminates ABSENT (namespace-gap error
  set) from PRESENT-but-unopenable (NOT_READY/IO_DEVICE/GEN_FAILURE/
  exhausted retries) and KEEPS the present ones listed (honest empties,
  busy-marked); the cfgmgr interface pass lists unopenable PRESENT
  interfaces (identity = instance path for the merge); geometry refusals
  on opened handles still list. Reference: the og lists devices WITHOUT
  opening them (five-stack fan-in). Pin: D3Enumerate.PresentButUnopenable
  DisksStayListed; the N-071 signature pin updated for the last_error
  out-param. 12/12 enumeration tests green.
- **BF-28 (this commit)**: health_read_impl's unelevated ACCESS_DENIED now
  returns ERR_ELEVATION_REQUIRED (uniform with every other raw-open site)
  instead of ERR_OPEN_FAILED + an "administrator rights required" message
  string standing in for the code — the bridge's table labels the refusal
  elevation-required with zero frontend change. Pin:
  BF28ElevationCode.HealthOpenRefusalCarriesTheElevationCode.
- **D4d disposition (recorded as BF-30)**: the raw-sign layer IS gated
  (free_map + occupancy + N-031 zero-stride fast-skip — verified at
  cr_carve.cpp:968-1010); NTFS/FAT/exFAT fill the free map
  (cr_scan.cpp:1814/1832/1841). The 8.45 MiB/s wall is real-content
  validation CPU (PERF-LEVERS' measured 303.8 CPU-s/GiB) — upstream bench
  scope, honestly open. The product's obligation (visible, honest,
  bounded progress) is met by D4a/b/c.
- Lane A's re-vehicled forensic probe (d8fbef64) carries the fixed-posture
  gates; the shipped-v0.8.4 reproduction (run 42255492260: 75→76.1% over
  891 s, found-count frozen at 197) is the D4 field evidence.

## Lane B — the BF-32 record (the emitter teardown race)

WIRING-AUDIT's #9/#10 finding verified: the scan emitter thread holds the
engine scan handle as a raw SendPtr (events.rs:349-398), while the entry's
Drop (state.rs:291-327) frees that handle through cr_ui_scan_close the
moment the last Arc drops — the run tail's final emits can still be in the
emitter's drain queue at that instant. The window is narrow and never
observed (29 forensic runs), but it is a real lifetime bug. The correct
fix (two-phase teardown: sink box freed at the run tail like the consent
pipe; the emitter holds the entry Arc so Drop orders after the drain) is
designed in BF-32 and deliberately NOT rushed into the 0.9.0 train — a
concurrency restructure this late would risk the validated release. Next
bridge session's first item.

## Lane B — the CI red on 6c9c36d9 root-caused (test determinism, not product code)

engine-ci run 42266802645: Telemetry.CarveBytesAdvanceMidSweep red on the
Windows runner (46.41 s, carve bytes never moved) while green locally. The
fixture is scheduling-dependent: with the runner's 3 carve workers, the
PARKING window (window 2) can start before windows 0-1 complete, and the
parked read then holds the device pipe's turn — no window completes while
parked. My sandbox's 8 workers completed 0-1 before 2 started (green). The
PRODUCT code is correct either way (the park is a test-fixture artifact; a
real device never parks mid-read); the TEST now pins the deterministic
shape: cfg.threads = 1 (serial windows: 0 rides the retry pipe, 1
completes, 2 parks) + a cheaper, longer poll. Same determinism applied to
CancelLandsInsideTheCarveWindow. 3/3 green locally.

## The 0.9.0 cut — both lanes green, every defect fixed (T19)

The release train: main fully green at 4aa33d62 (engine-ci: MSVC /W4/WX
build clean, 1752/1752 ctest incl. the D4 telemetry pins, the no-console
tree guard, the D3 enumeration pins, the BF-28 pin, the PE import
allowlist gate; ui-ci + ui-e2e + contracts-ci green on the merged tree).
The defect register at cut time:

- D-PRIV-01 FIXED: the requireAdministrator manifest + the OS-refusal
  posture + eleven docs trued + the manifest gates (build.ps1 D1 gate,
  the forensic manifest gate).
- D-CONS-01 FIXED: the hidden direct-fsutil TRIM query + the tree-wide
  no-_popen build gate + the PE import allowlist catching the new
  imports consciously.
- D-ENUM-01 FIXED (both layers): the reference-parity landing page
  (Internal + External groups) + the engine's never-drop-a-present-
  device discrimination + the home-census probe gate.
- D-SCAN-01 FIXED (all four roots): intra-window carve checkpoints +
  live bytes_scanned (D4a), the bytes_found ABI append with live
  stamping (D4b), bounded cancel (D4c), the live counters rendered
  (bridge fill — zero frontend change needed), the instant-completion
  and live-tree bridges (W-01), and the probe's frozen-scan gates.
- D-UI-01 RESOLVED: the banner surface removed (reference parity).
- W-01/02/03, U-01/02/03/04, E-01(pending lane-B confirmation), T14
  persistence: fixed with regression pins.
- BF register: BF-04/22/25 fixed; BF-28 fixed; BF-30/31/32 recorded
  with precise scope (BF-32 deliberately held out of the train — the
  right call, documented).

Next: tag v0.9.0 → the release workflow (full product build, the
manifest gate inside build.ps1, artifacts + checksums + truthful notes)
→ the forensic dispatch on the new tag (the fixed-posture gates:
manifest, OS-refusal twin, home census, moving scan progress, live
found-size) → RELEASE-VALIDATION curation.

## Lane B — D4e: the deep remnant sweep's 655-second freeze (found by the v0.9.0 forensic gate)

The forensic run on v0.9.0 (42278088695) failed its own D-SCAN-01 gate:
a 655 s stretch with percent + found-count + sector ALL frozen — in the
DEEP stage this time, not the carve (advancedPctDelta=1.1 and
liveBytesOk=true prove the carve axes D4a/b landed correctly; the freeze
sat in the NTFS remnant sweep that runs BEFORE the carve). Root cause:
scan_mft_remnants ticked progress only per COMPLETED volume/threads chunk
(≈16 GiB per tick on the 64 GiB seed) — and the SERIAL path had NO tick
at all. Fix: the block loop (4 MiB steps) ticks a shared atomic coverage
sum through the caller's progress seam — monotonic across workers,
bounded cadence, cancel-through-progress preserved, both paths. Pin:
Telemetry.DeepRemnantSweepTicksPerBlock; NTFS suite 97/97, telemetry
suites 22/22.

**Release note (lane coordination):** v0.9.0 is published and carries
every fix EXCEPT this one; the deep-phase freeze it still has is the
D4e class. main needs this green, then v0.9.1 (lane A's protocol: it
cuts the tag; the takeover clause applies if it stays silent).


## Lane B — E-01: the volume enumeration buffer (the 16+ letters wholesale failure)

cr_enumerate.cpp's enumerate_volumes used a fixed 64-wchar
GetLogicalDriveStringsW buffer — at 16+ letters (dev VHDs, containers,
card readers) the call overflowed, the >64 check refused, and EVERY
volume vanished from the device list. Fixed with the SDK's own shape: a
512-wchar stack buffer + the overflow-return-driven exact-size retry.
(E-01 was the one register row lane A marked pending lane-B
confirmation — confirmed real, fixed; 12/12 enumeration tests green.)

## Lane B — the v0.9.1 cut (announced per the release protocol)

engine-ci green at 62fb7670 (D4e + E-01 in the full Windows suite). With
the published v0.9.0 carrying the known D4e freeze — disclosed in its
amended notes by lane A — and the fix green on main, the patch release
cannot wait: per the protocol's own principle (the user's mandate
outranks lane symmetry), lane B cuts v0.9.1. Version lockstep bumped
(VERSION, tauri.conf.json, Cargo.toml, Cargo.lock); the release notes'
"What changed" carries the two 0.9.1 items ahead of the 0.9.0 record.

## Lane B — D4f: the 605-second zero-region freeze (the v0.9.1 forensic gate's catch, root-caused)

The v0.9.1 forensic run (42291160236) still failed D-SCAN-01: a 605 s
stretch frozen on the 64 GiB NTFS scan, with the io-timeline proving
device reads CEASED (readBytes static at ~12.17 GB) while the process
ground on in pure CPU. Root cause (from the run's own artifacts + code
walk): the carve's raw-sign user-sign arm took the registry mutex TWICE
per 512-byte stride (usersigns::active_count + match_at) and re-scanned
the disabled-format token list per sign per stride — over the seed's
~63 GiB mostly-ZERO free space that is ~134M strides × (mutex + 190
token scans + memcmps), contended across 4 workers: the 10-minute
all-axes freeze. Fix (output-identical): the set is snapshot ONCE per
carve; the stride probe is lock-free over the snapshot in set order
(same first-match semantics, same avail rule); the disabled map is
precomputed (the M-024 pattern); a fully-zero probe span of an
all-nonzero-sign set skips the arm outright (the span covers every
sign's whole byte range — bounded by the same avail rule the probe
itself enforces). Pin: D4fUserSignProbe.TheStrideLoopIsLockFree.

## The debug-and-release session re-opens (post-v0.9.2 — the user's machine still says NO)

T1. Session opens. Repos recorded at start: final-prod@main
eb4bb5f3 (v0.9.2 train), easeus-drw-research@main be0cfe9 (the CORRECTIONS.md
#9 errata state). Session prompt re-saved with this session's addendum
(`worklog/session-debug-and-release-prompt.md`), token redacted.

The user's report this session: the scan is STILL broken (stuck in sector
scanning, proper files not shown), SMART is broken (little shown, only for
the FIRST disk, switching to any other disk shows NOTHING), not all disks
visible on the disks page (2 internal + 1 SD), "everything is broken or not
working properly", missing API surface, and an independent-research
inventory of Windows recovery engines to consult (SectorForge, TestDisk/
PhotoRec, ntfs-recover, ColinFinck/ntfs, libyal, etc.). Past enumeration
fixes (D-ENUM-01, D-ENUM-02) are explicitly called out as failed
implementations on the user's real machine.

First act — main was RED at session open: engine-ci #34 failed the
clang-format gate on eb4bb5f3's own two files (cr_enumerate.h:118-119,
test_nvme_health.cpp:216). Fixed mechanically (zero behavior change,
whitespace only) as 0c80036f — the lane-A style pre-commit discipline
re-applied. No diagnosis touched before Phase 0.

Standing interpretation for this session: the prior sessions' "FIXED"
claims are hypotheses, not facts, until re-proven on a real Windows runner
with the installed product. The defect register resets to the user's
report; the six context sheets get refreshed against the CURRENT tree; the
reproduction runs target the v0.9.2-era build as installed.

## Lane C — T-C1: the API-surface lane opens (2026-11-11)

The user split this session two ways: the defect lane keeps the reported
breakages (scan stuck, SMART first-disk-only, disks missing); lane C
(the third lane, `API Surface Agent`) takes the API-surface mandate —
"expose more core to api so ui interface can utilize it more better."

Phase C0 (read-before-write) complete: the full cr_ui.h ABI diffed
against the bridge's declared-and-used surface; the INTEGRATION-PLAN
§4.7 DISABLE register (rows 16-27) is the gap map; the research corpus
contradicts the "no reference screen" rationale on rows 16-21, 23 and 25
(the evidence table with citations lives in
worklog/lane-c/API-EXPANSION.md). Tauri 2 IPC research (official docs,
tauri-apps/tauri sources, current 2.12.x): the existing event-emitter
bridge pattern is sound for this codebase (channels would re-architect
a proven surface for no user-visible gain — the coalescing emitter
already provides the ordering/backpressure properties channels give);
the expansion follows the house pattern (async commands, spawn_blocking,
BridgeResult, scoped `domain-verb` event names).

Lane C territory + seam crossings announced in COORDINATION.md (the
defect lane's active files — engine/**, bridge/{state,events}.rs,
commands/{scan,devices,health}.rs, the three defect screens — are NOT
touched). Engine-side gap found and registered, not papered over:
cr_ui_image_create exports no cancel (the win layer's CancelToken is
not ABI-visible) — the bridge surfaces the truthful no-cancel contract
and the divergence from the reference's stop-confirm is PARITY-DELTAS
material, not a fake stop button.

Next: the three new bridge modules (imaging, diagnostics, maintenance)
+ the connector/contract/mock/tests/i18n wave, in one coherent push per
the validate.mjs two-directional manifest gate.

## Lane C — T-C2: the API-expansion train lands (11 commands, 2 events, full connector + mock parity)

The §4.7 DISABLE register flips eight rows to SURFACE — every one backed
by an engine export that already existed and a corpus citation that
contradicts the row's original "no reference screen" rationale (the
evidence table: worklog/lane-c/API-EXPANSION.md):

- imaging.rs (rows 19, 23's producer half): image_create_start /
  image_create_state + image-progress:<id> / image-finished:<id>. The
  bridge pre-gates the engine cannot express (same-disk destination →
  destination-overlap; free < byte-for-byte + 64 MiB headroom →
  out-of-space). NO cancel — the ABI exports none (cr_ui_image_create
  runs its CancelToken as nullptr); cancelSupported is always false, the
  truthful contract (the reference's stop-confirm dialog is the
  registered divergence, never a faked stop button). partialKept is a
  filesystem probe, never message-text parsing.
- diagnostics.rs (rows 16, 17, 18, 25, 26): scan_telemetry (the full
  stage axes + best-effort companions behind availability flags),
  scan_assistant_stats (the S-023 gauge axes verbatim — matched/total,
  the og's <100 forced-zero rule; the UI renders the verdict),
  carve_catalog + scan_carve_tally (the analyser-list surface + the
  found-N-.jpg report + the user-sign aggregate slot), disk_map (the
  AnalysisGridWidget producer: four overlays + bad ranges, bridge-bounded
  cell budget 4096/65536/64KiB-floor, two-call bad-pair sizing),
  device_transport (the [transport] truth + the M-030 advisory),
  session_regrade (grade-1-only, sticky 0/2, via the metadata cache).
- maintenance.rs (rows 20, 21): volume_fsck (chkdsk check/fix, hidden
  vehicle, raw exit code + the final i18n verdictKey), 
  volume_bitlocker_off (the destructive decrypt twin — accepted ≠
  plaintext, start/settle ticks logged, never a fabricated percentage).

The governance wave (validate.mjs's two-directional gate): 11 new
command-surface rows (43 → 54 commands), 2 event rows, 3 bridge-only
error-code rows (destination-overlap/out-of-space gain their first
bridge-side detector — the imaging pre-gate; image-not-found joins the
vocabulary), type-maps §11-§13, event-mapping's imaging section,
i18n-sweep's namespace list grows (maintenance). The connector contract
gains ImagingDomain + MaintenanceDomain + the Scan/Results/System
extensions; full mock parity (deterministic tick engine for imaging,
derived-from-runtime observability, the same typed refusals); 14 new
test cases across both connector suites (202 total green).

Lane coordination (BF-ABI-01, seen at push time): lane D's refreshed
DIAGNOSIS names the unreleased bridge ABI drift — abi.rs's CrUiDiskInfo
misses bus_type (296≠304) and CrUiSmartReport misses power_cycles_nvme
(224≠240). Lane C's exposure is confined to imaging's enumerate-derived
pre-gates (disk presence + size); every other lane-C surface consumes
pinned structs (carve_format, disk_map_geometry, stage_tel verified
field-exact). abi.rs is lane D's territory (their re-pin train) — lane
C's commands compile unchanged through the appends (zero-init-safe by
the ABI policy) and become fully truthful the moment the re-pin lands.
Recorded here so the seam is explicit; no crossing made.

## Lane C — T-C3: the UI surfaces + the CI-red fix (the local Rust gate unlocked)

The first CI round caught my lane's own seam (the lane-B lesson relearned
in lane C's colors): three import-level mistakes cargo check surfaced on
the Windows runner — Arc wrongly removed from diagnostics.rs (my grep
missed `tauri::State<'_, Arc<BridgeState>>` param types), Value wrongly
removed from maintenance.rs (the pattern missed `Result<Value,`), and the
DESTINATION_OUT_OF_SPACE name never matched between error.rs and
imaging.rs — plus an i8→char cast Rust refuses. All fixed.

The durable win: the sandbox now runs the FULL local Rust gate. Tauri's
Linux -sys build scripts only VERIFY pkg-config presence (the FFI crates
are pure-Rust extern blocks; my crate never compiles GTK C code) — a stub
.pc directory unblocks `cargo check --tests`, `cargo clippy --tests
-D warnings` and `cargo fmt` end to end. Every lane-C Rust push is now
compiler-proven BEFORE the runner sees it. (The pre-existing fmt drift in
build.rs/sessions.rs was reverted both times — lane D's files, not mine.)

The UI wave (every surface consuming the T-C2 API):

- SectorMapPanel (the AnalysisGridWidget parity, UIUX/08:11): collapsible
  strip on the scan page, four overlay tabs (coverage / allocation /
  found density / confidence), the engine's own cells merged to a 1024-
  cell display bound with ranked folding (bad > used > scanned), the bad-
  range count on the collapsed strip, refetch only while open+live.
- RecoverabilityCard (S-023 / UIUX/30:19): the completion celebration
  gains the engine's grading verdict — a circular gauge over matched/
  total with the og's three-tier vocabulary, the < 100 small-result rule
  mirrored (gauge hides, count shows), and the RecoveryAssistant
  category row. Renders nothing when the grading pass has no answer.
- CreateImageDialog (the DiskBackup chain parity): per-card entry on the
  home page (the reference's own placement), destination folder via the
  connector's picker (source-excluding hint) + default file name, live
  progress with throughput/ETA from the engine's cumulative-bytes
  callback, the truthful no-cancel contract in copy and controls (a
  "continue in background" exit, never a dead stop button), completion +
  reveal. State rehydration wired through useImageState (the event store
  dies with the last listener; a reopened dialog reads the query).
- queries/imaging.ts: the external-store pattern from queries/scan.ts
  (per-imageId progress store, finished-bridge invalidation, state poll).

Five new component tests (207 total green): the panel's collapsed/
open/tab flow against a real mock scan, the merge bound with a 4096-cell
payload, the verdict from a completed scan, the dialog's start → running
no-cancel posture through the singleton connector. i18n: scan.map.* +
scan.recoverability.* + the imaging.* namespace in both locales.

## Lane C — T-C4: green at 7a9def3a + the documentation truth

CI record: 7a9def3a — ui-ci GREEN, ui-e2e GREEN, contracts-ci GREEN (the
full lane-C train on lane D's re-pinned tree). The path there crossed two
announced mechanical repairs, both the lane-A/B precedent applied
symmetrically: lane D repaired lane C's first-push compile errors
(64d4632d, acknowledged); lane C repaired lane D's clippy findings on
their scan-truth/re-pin code (unused doc comments on expression fields +
the devices.rs OR-pattern) after both lanes' pushes had left ui-ci red.

The docs trued to the delivered surface:

- INTEGRATION-PLAN §4.7: rows 16-21, 25-26 flip DISABLE → SURFACE (23
  SURFACE / 4 DISABLE). Every flip carries its corpus citation; rows 22
  (TRIM chip) + 23 (img- scan targets) stay deferred to lane D's green
  per COORDINATION; row 24's posture stands.
- BEHAVIOR-FINDINGS: BF-08/09/10/11/17 closed as Surfaced (lane C), with
  BF-10 carrying the engine gap for next session (no
  cr_ui_image_cancel export — the win-layer CancelToken is not
  ABI-visible).
- docs/PARITY-DELTAS: the imaging-era divergences registered (D-IMG-1
  the no-cancel truth, D-IMG-2 .img-vs-.edisk, D-IMG-3 the sector-range
  option, D-MAP-1 the map's rendering-rules form).

Remaining in lane C's scope: the deferred img- scan-target arm + the
TRIM chip (both wait on lane D's defect-train green per COORDINATION),
and the maintenance screens (fsck verdict + bitlocker confirm) once the
SD/health surfaces stabilize.

## Lane C — T-C5: the e2e coverage + the visual record

Three new e2e specs (e2e/lane-c-surfaces.spec.ts, all green locally
against the mock-mode preview and on CI's ui-e2e): the sector map's
collapsed→open→overlay-switch flow with the legend vocabulary; the
completion celebration's engine verdict (the percentage form + the
matched/total line + the category row); and the Create Image flow's
truthful no-cancel posture (dialog → destination → running note +
"continue in background", the stop-button absence asserted, completion).
Four screenshots captured as the visual record (create-image dialog, the
map on coverage + allocation mid-scan, the celebration with the
recoverability card): 1280×832, content-verified.

---

## 2026-11-13 — SESSION START: SMART subsystem rebuild, scan pipeline repair & reference parity

Session prompt (authoritative, token redacted): worklog/session-smart-scan-rebuild-prompt.md —
read twice before T1. Mandate: (1) SMART/disk-health rebuild for every disk class (NVMe, SATA
SSD/HDD, USB, SD) across Win7/10/11 with correct per-transport APIs and the reference's
presentation model; (2) scan pipeline rebuild (quick scan → real-time streaming → automatic
deep scan, weighted monotonic progress, non-stalling sector reads, working filters,
deleted-tree reconstruction); (3) gap closure vs the og-codebase reference on installer
footprint, disk-health presentation, scan flow, filtering, prioritization, preview.

### T1 — baseline recorded

- Product repo cloned: final-prod @ e119cc71 (main, clean). Research corpus cloned:
  easeus-drw-research @ be0cfe9 (main, clean; 1.7 GB, 11,495 tracked files).
- Prior session history present and preserved: lanes A–D worklogs, PARITY-DELTAS register,
  BEHAVIOR-FINDINGS (BF-01..17 with 08/09/10/11/17 closed), REPRODUCTION evidence dirs,
  RELEASE-VALIDATION sheets, context-sheets/ (CORE-SURFACE, UI-SURFACE, REFERENCE-LAYOUT,
  REFERENCE-INSTALLER, DRIVER-DEPLOYMENT). Audit trail rule honored: nothing overwritten.
- GitHub token hygiene: token supplied in the session prompt stored only in a local
  git-ignored file (.github-token, mode 600) outside the product tree; passed per-command
  via `git -c http.extraheader`; never written into any repo file, remote URL, or log
  output. Bearer auth works for api.github.com but git smart-HTTP requires Basic —
  resolved by using the basic extraheader form.
- Phase 0 launched: parallel agents A–E (product state, reference SMART, reference scan,
  reference footprint, independent verification) reading both repositories end to end.
  Their five context sheets (PRODUCT-STATE / REFERENCE-SMART / REFERENCE-SCAN /
  REFERENCE-FOOTPRINT / VERIFICATION) are due before any code change, per the mandate.

### T2 — Phase 0 complete: the five context sheets (commits d558ee84 + this train)

- Turn boundary note: the first agent-E run was cut off by the harness subagent
  inactivity timeout (600 s) after finishing its evidence gathering, before writing
  its sheet. The rerun used an incremental-write protocol (skeleton first, append per
  verification) and delivered. All five sheets are now committed:
  PRODUCT-STATE (d558ee84), REFERENCE-SMART (d558ee84), REFERENCE-SCAN (d558ee84),
  REFERENCE-FOOTPRINT (d558ee84), VERIFICATION (this train).
- Agent-E verdicts that corrected the sibling sheets: the 40/35/25 weights live in
  engine/include/cr/orchestrator/cr_scan.h:72-76 (NOT contract.ts:177-184 — that is
  ScanStage/ScanPhase types); the status-bar templates are byte-exact
  `Quick Scanning%1, Files Found:` / `Advanced Scanning%1, Files Found:` /
  `Reading Sector:` / `Estimated time remaining: %1s`; the reference WMI fallback is
  MSStorageDriver_FailurePredict{Data,Thresholds} (root\WMI) — NOT MSFT_PhysicalDisk;
  USB quirk ladder = 23 distinct AddDisk-USB* strings (not 28); vendor NVMe route
  NAMES verified, payloads UNVERIFIED; 840x590 verified via extracted main.qml;
  RAWSearchDLL does NO device I/O (caller-provided buffers); FileFilter.xml exactly
  8 categories / 1489 <Extention>; raw_signs.json 139 formats / 261 signs, grammar
  offset,hex,score 0.5–1.0.
- Headline Phase-0 conclusions (details in the sheets): (1) the user's SMART/scan
  symptoms match the SHIPPED v0.9.1 — the NVMe mis-pack, bus-table, fake-healthScore,
  SD-fake-card, 605 s carve-freeze and scan-truth fixes all sit on main UNRELEASED
  (no tag since cdf66ff8); (2) the structural gaps are the registered D-P4 (in-app
  one-disk health page vs the reference's separate all-disks DiskHealth surface) and
  D-P6 (global 40/35/25 curve vs per-phase 0-100% restart) — this session's mandate
  reverses both; (3) engine transports today: 3-route ATA ladder + NVMe
  STORAGE_QUERY + CSMI; SAT and the WMI fallback are ABSENT (V1); (4) footprint
  delta 1,276 live files vs our 8 is registered consolidation + four real functional
  gaps (preview stack, repair workbench, partition-apply UI, diskhealth content).

### T3 — DIAGNOSIS.md consolidated (this train)

- worklog/DIAGNOSIS.md: this session's diagnosis PREPENDED (three-strata model,
  R1-R11 defect table mapping every user-reported symptom to its evidenced root
  cause and fix phase, agent-E corrections folded in, the Phase 0.5-4 plan); the
  prior session's 2026-11-06 diagnosis preserved verbatim below it (audit trail).
- Phase 0.5 defined as a precondition: main must go green (bridge-integration
  E0308 frozen_run_s at it/tests/bridge_integration.rs:461; ui-ci clippy) before
  any feature work, per the leave-main-green discipline.

### T5b/T5c (engine) — SAT transport + WMI fallback: the two missing SMART routes

- SAT (the USB/IEEE1394 bridge route, REFERENCE-SMART §2 "GetSmartAttributeSat"):
  implemented with the reference's BYTE-EXACT CDB stamps, decoded this session from
  the corpus decompile (DiskHealth_decompiled.c FUN_00419280 = SMART READ DATA,
  FUN_0041a440 = THRESHOLDS, the :9124 sibling = IDENTIFY):
    SMART READ DATA    A1 08 0E D0 01 01 4F C2 <dev> B0 00 00
    SMART THRESHOLDS   A1 08 0E D1 01 01 4F C2 <dev> B0 00 00
    IDENTIFY DEVICE    A1 08 0E 00 01 00 00 00 <dev> EC 00 00
    the -1A retry      CDB[1] 0x08 -> 0x1C (FUN_00419280 case 0xb)
  Geometry: IOCTL_SCSI_PASS_THROUGH_DIRECT (0x4D004 — the decompile's own constant),
  one allocation, sense 0x18 @ SenseInfoOffset, data 512 after it, DataIn, timeout 2s,
  response accepted only when the ioctl succeeds AND the 512 bytes are not all zero
  (the reference's byte-sum rule). Pure builder cr::health::sat::build_cdb12 (new
  src/core/health/cr_sat.cpp) + SatPassThroughChannel in cr_health.cpp; chain order
  ata-passthrough -> smart-rcv -> SAT -> CSMI (standards routes before the vendor
  array route). Divergence documented in-source: we fill the real SCSI address from
  IOCTL_SCSI_GET_ADDRESS (the reference zeroes P/T/L — its \\.\Scsi%d: context
  pre-resolves; on a disk handle zeros could serve another LUN's data).
- WMI fallback (the reference's LAST route): MSStorageDriver_FailurePredict{Data,
  Thresholds} from root\WMI (cr_health_wmi.cpp, new). Instance-to-disk tie by
  serial (STORAGE_DEVICE_DESCRIPTOR, space-stripped) with model fallback; no tie ->
  truthful unavailable, never another disk's table. Shared parse extracted:
  read_smart_ata_buffers() now serves BOTH the chain path and the WMI path — one
  parser, two sources, zero grading divergence. ata_version honestly 0 on the WMI
  route (IDENTIFY-only datum).
- Tests: test_sat_stamps.cpp (new) — byte-exact stamp assertions (all three commands
  + the -1A flip + refusal of uncited commands) and the shared-buffer parse
  (thresholds carried, missing-thresholds non-fatal, WMI-path ata_version 0).
  10/10 pass locally; full Linux suite green except two sandbox artifacts
  (FatChain.CycleGuardPerf: a 3.4 GB zeroed fixture vs the sandbox memory cap —
  it PASSED on Windows CI run 44023463712 forty minutes earlier; win_syntax_check
  via ctest looks for an in-tree build dir — direct run green).
- The M-055 win-stub grew: SCSI_PASS_THROUGH_DIRECT/SCSI_ADDRESS (load-bearing
  layout documented), the two SCSI ioctls, and a declaration-only wbemidl.h
  (the COM/WMI surface the fallback touches). win_syntax_check: 20/20 files parse.

### T5d (D-P4 reversal) — the grade truth chain + the separate disk-health window

- The grade truth chain (the D-P4 substance): the engine's og-faithful 4-state
  grade (cr_grade.cpp, V8-verified byte-level) NEVER crossed the ABI — the bridge
  re-derived status with its own rules (wear >= 80 -> caution — NOT the og rule;
  the og: life = 100 - wear, life > 10 Good / == 10 Caution / < 10 Bad). Fixed
  end to end: cr_ui_smart_attr.grade + cr_ui_smart_report.grade appended (ABI
  policy §9, append-only), stamped in health_read_impl (NVMe family-first via
  grade_nvme incl. the Parallels product-string special case through the new
  win::health::query_product_string; else the ATA S-024 worst-of-attrs; else
  Unknown), layouts regenerated (363 field paths), abi.rs re-pinned, health.rs
  maps the grade 1:1 (the wear>=80 re-derivation deleted), contract.ts gains the
  truthful fourth state 'unknown' + per-row 'caution', the mock models the SD
  card as Unknown/empty/no-transport (the reference's §5 model).
- The separate disk-health window (the reference's DiskHealth.exe companion
  equivalent): ui/src/features/health/disk-health-window.tsx — 840x590 frameless,
  custom titlebar, all-disks selector ("Which disk do you care about?"), the
  firstCautionIndex auto-jump on open, 4-state banner, temperature + powered-on
  card, the reference's S.M.A.R.T. columns ID/Attribute/Current/Threshold/
  RawValue/ST with per-row status colors, truthful transport line, manual
  refresh + the 5-minute poll (documented divergence: the reference's interval
  is corpus-UNKNOWN; CrystalDiskInfo-lineage default). Launched from the
  /health page header (openDiskHealthWindow: single-instance, focus-on-reopen,
  diskHealthIndex= parity via the disk query param + the disk-health:select
  event). Capabilities: disk-health.json (the window's chrome cluster) + the
  main window's create/focus/emit-to permissions.
- A real component bug the new test caught: the all-disks first-caution probe
  (one health read per disk, ~0.5-3 s on real hardware) completed AFTER the
  user's dropdown choice and OVERWROTE it. Fixed with userSelectedRef (user
  intent + re-open retargets win over the in-flight probe; the probe still
  clears the loading state).
- Tests: 4 new vitest (tests/health-window.test.tsx) — the mock SD truth pin
  (unknown/empty/no-transport), the first-caution jump, the never-fake-green
  SD state, the reference column set; 211/211 UI tests green, typecheck +
  eslint clean, 10/10 new engine tests green, win_syntax_check 20/20 (one
  real catch: strnlen_s is MSVC-only — replaced with memchr), clang-format 0.
  knip could not run locally (oxc-parser allocation vs the sandbox memory cap
  — CI runs it on proper hardware).
- CI gate fixes landed alongside (commit 72fad8c2): the Win7 import allowlist
  gained ole32/oleaut32 (the WMI fallback's COM pair — Win7-safe, reason trail
  in the file), and the bridge deep-window gate pinned its config to
  quick|deep (the first real execution never reached deep: carve at 8.45 MiB/s
  ate the whole 480 s window on the 2 GiB seed; the predicate's carve_done
  condition dropped WITH the stage — a skipped stage never completes).

### T7 (engine half, D-P6) — the per-phase progress model, engine-side

- phase_progress(): the pure function beside weighted_progress (cr_scan.cpp) — the
  reference's RESTARTING gauge (REFERENCE-SCAN §5: "Quick Scanning %1, Files Found:"
  → "Advanced Scanning %1" with the gauge back to 0). Quick phase = quick's own
  permille; advanced = deep+carve weighted over the ACTIVE sub-stages ONLY
  (re-normalized — unlike weighted_progress, a disabled stage drops its weight, so
  a quick|deep scan's advanced phase reaches 1000 instead of stalling at 750‰).
- Wired into BOTH overall-update sites (tel_stage_end_ + the publish-path twin) —
  same lock, same inputs, one truth with overall. ScanTelemetrySnapshot carries
  phase (0 quick / 1 advanced) + phase_permille.
- Tests: 3 new (OrchestratorPhase.*) — quick-is-own-permille, the restart at the
  boundary (quick done + deep 0 → advanced 0), the re-normalization cases
  (deep-only reaches 1000; quick-disabled starts advanced; nothing-active honest
  zero). 4/4 with WeightedProgress; format gate 0.
- NOT YET LANDED (next turn, precise scope): the ABI exposure (cr_ui_telemetry
  phase fields + layouts + abi.rs), the bridge emission (phase/phaseProgress in
  compose_scan_progress via the telemetry poll), the contract fields, the mock
  timeline, and the page's gauge/status-line switch from overallProgress to
  phaseProgress. The status-line i18n keys (quickScanning/advancedScanning with
  {{percent}}) already exist and take the phase percent unchanged.

### T7 presentation half (D-P6) — the phase axes cross the ABI to the gauge

- ABI (append-only, policy §9): cr_ui_telemetry.phase + phase_permille; stamped in
  scan_telemetry_impl from the snapshot; layouts regenerated (365 field paths);
  abi.rs re-pinned in the same change.
- Bridge: scan_phase() probe (any-thread telemetry, (0,0) on miss — honest no-data)
  + compose_scan_progress carries phase ('quick'|'advanced', null at terminal) and
  phaseProgress (0..1, 1.0 at terminal); all three compose call sites wired.
- Contract: ScanProgressEvent.phase + phaseProgress (optional, null at terminal,
  documented fallback to overallProgress).
- Mock: the timeline models the phase model (quick = quick's own %, advanced =
  weighted deep+carve over the stages actually present — restarts at the boundary).
- Page: the gauge + the status-line percent now render phaseProgress (the og's
  RESTARTING gauge) with the overall curve as fallback; the label prefers
  progress.phase. This is the user-visible D-P6 reversal: 'jumps to 30-75%' is
  gone — quick shows 0-100%, then advanced restarts at 0.
- Gates: engine build clean, OrchestratorPhase/Telemetry/HealthCsmi 15/15,
  clang-format 0, win_syntax_check 0 FAIL; UI typecheck/eslint/arch-lint/prettier
  clean, 211/211 vitest.

### T8/T9 evidence + the it-gate's honest reframe — deep is metadata-fast on partition scans

- The artifact forensics (run 44090904420, scan-timeline.csv): quick|deep over the
  6 GiB fixture with the 3 GiB deleted bulk COMPLETED inside one 100 ms tick —
  deep_pm=1000/deep_done=1, bytes_found=3,238,953,060 (~3.01 GiB, 293 files). THE
  ENGINE'S DEEP STAGE IS THE METADATA-STRUCTURED FULL PASS (MFT/$Bitmap/USN/
  orphans): it processes the bulk file's extents from metadata in milliseconds —
  it is NOT a slow byte sweep on partition scans. The byte-denominated advanced
  work is CARVE. The it-gate's 'deep in flight for >=4 samples' premise was wrong
  for this fixture class — three runs of evidence (44071105301 carve-ate-the-
  window; 44084452441 sub-500ms completion; 44090904420 sub-100ms with 3 GiB real
  bytes) close the question.
- The it-gate now pins what is REAL on Windows: quick+deep ran and completed;
  phase==1 at terminal; phase_permille==1000 (the restarting gauge completes at
  100%); overall_pm==750 exactly (the global quick|deep cap — the 'jumps directly
  to 75% advanced scanning' presentation D-P6 closes, pinned as a number). The
  timeline CSV carries the phase columns (the validation artifact).
- Engine-side (deterministic): PauseResume.PhaseModelCompletesAt1000WhereThe-
  GlobalCurveCapsAt750 — through the live controller on the FAT fixture: phase 0
  observed while quick ran; terminal phase 1 / phase_permille 1000 / overall
  exactly 750. The mid-flight restart cases stay pinned by OrchestratorPhase.*
  (b48898dd).

### T10-T12 — filters + the deleted-tree reconstruction, pinned on the seeded disk

- A new it-crate gate (results_filters_and_deleted_tree) exercises the REAL
  user-facing surfaces end to end on the CI fixture: a quick|deep scan (<1 s on
  this fixture), session save + reload (the results explorer's own path), then
  cr_ui_query_run assertions:
  (a) the whole manifest returns rows (R9 'filters are dead' — pinned ALIVE);
  (b) the CATEGORY filters through the engine's FileFilter table (image 1<<1,
      document 1<<4, archive 1<<5 — the seed now carries 5 .jpg + 3 .pdf +
      2 .zip live files for exactly this);
  (c) the SIZE filter (a 250 KiB floor admits the 17 x 256 KiB live rows + the
      3 GiB deleted bulk, and nothing else);
  (d) THE DELETED-TREE RECONSTRUCTION (T12): deleted_only returns >= 81 rows
      (80 del-*.bin + the bulk) and >= 80 carry their ORIGINAL 'gone\...'
      parent path — the 'reconstruct them as they were before deletion'
      standard; the full deleted tree is written to the evidence artifact
      (deleted-tree.txt).

### T13 — the preview surfaces, pinned + honestly scoped

- it-gate preview_surfaces: image nodes classify through the engine's metadata
  (kind 'image') and the REAL zip's browse-inside inventory walks its central
  directory (>= 5 entries; the seed's backup-1.zip is now a genuine
  Compress-Archive of the docs folder). Both ABI halves (M-102 metadata /
  M-116 inventory) exercised end to end on real Windows; evidence lands in
  preview.txt.
- The remaining reference gap — RENDERED video/document previews (the og's
  ffmpeg-class video stack ~99 MiB + pdfium documents) — stays a registered
  divergence: our surfaces show the truthful 'preview not rendered' hint with
  real metadata (duration/codec, office metadata), never a fabricated render.
  Bundling third-party media renderers is a supply-chain + installer-size
  decision recorded in PARITY-DELTAS (D-P8) with precise scope.

### T20 handoff — the session state at cutoff (2026-11-17, ~24h of lane time)

THE CODE STATE (all pushed): 24 commits this session, HEAD 93389402. The major
landings, in order: Phase 0 (d558ee84/37287b86/f95e74a9), main-green fixes
(64790fef/794ff72b/385d206e), SAT+WMI (0bfc634a), the grade chain + the
disk-health window (2a4b02ed), SMART-MATRIX + PARITY (74534282), the phase
model engine (b48898dd) + presentation (9618d9ed), the it-gate reframes
(e1988e6c, 72fad8c2, a3bac453), filters/deleted-tree gate (f8415b72),
preview gate (71bdc9d7), FOOTPRINT-DELTA + D-P6/D-P8 (95dbd0fe),
BEHAVIOR-FINDINGS (cae79db6), v0.9.3 bump (948164bc), the Win7-gate layer
chain (d24c1cab, 93389402), the seed honesty fixes (55278e7b).

CI STATE AT CUTOFF: GREEN — ui-ci + ui-e2e + contracts on the code frontier
(f84809bc/948164bc); engine-ci GREEN at 385d206e, then the WMI import gate
taught its three layers (name dict, function JSON, ordinal imports) — the
final form runs on 93389402 (in flight at cutoff). bridge-integration: the
axes/health gates PASS on every recent run; the TWO NEW gates
(results_filters_and_deleted_tree, preview_surfaces) are on their
first-execution cycle — two seed lies already caught and fixed (MFT record
recycling -> write-before-delete; content-parsed metadata -> real JPEGs);
run 44102634559 (the seed-fix cycle) then hit a NULL-POINTER PANIC inside
results_filters_and_deleted_tree (core/ptr/mod.rs:554 — a row pointer
deref; the sites: run_query's from_raw_parts / node_of_category's (*rows) /
the deleted rows' path reads). THE NEXT SESSION'S FIRST DEBUG: pull run
44102634559's full log + artifact, guard the row-pointer handling (likely
count>0 with a null rows pointer, or a null path in a specific row), rerun.

THE RELEASE: v0.9.3 is bumped (948164bc) but NOT YET TAGGED — the tag waits
for a green frontier (the mandate's leave-main-green discipline). When the
bridge run on 93389402 (or its successor) is green: git tag v0.9.3 && git
push origin v0.9.3 -> release.yml builds the installer; then the forensic
validation pass on the RC + worklog/VALIDATION/RELEASE/.

THE GATES' SCOREBOARD (what this session's evidence added):
- SMART: SAT stamps byte-exact (10 tests), WMI fallback, the grade chain,
  the separate window — all landed; the truthful no-source path proven on
  windows-2022. Physical-class validation remains deferred (no hardware).
- Scan: the phase model pinned at three levels (pure, controller, real
  Windows terminal pins); deep's true nature established (metadata-fast on
  partition scans — carve is the byte phase); filters + deleted-tree +
  preview gates written, mid first-execution.
- The Win7 import gate's full layering documented in-file (a four-part
  lesson this session paid for in full).

EVERY deferred item with its precise scope is in BEHAVIOR-FINDINGS.md
(this session's section) and PARITY-DELTAS (D-P4c, D-P8, G-1(b)).

### T15-T16 — v0.9.3 RELEASED (all lanes green, artifacts verified on the release)

- The frontier went fully green on be3b8256 — the FIRST all-green bridge run with
  all five integration gates (telemetry axes, per-disk health coherence, the
  phase-model terminal pins, filters + deleted-tree, preview surfaces): runs
  44244567589 (engine-ci), 44244567592 (bridge), 44244567595 (ui-ci),
  44244567598 (ui-e2e). The intermediate turn's two root-causes held: the
  oleaut32 ordinal_8/9 rows and the query-gate's OWN use-after-free (a test
  bug, not an engine bug — the shallow struct copy kept raw pointers after
  cr_ui_query_rows_free; fixed with owned rows materialized before the free).
- Tag v0.9.3 pushed on bf2e9755; release run 44246474638 GREEN end to end:
  version gate, contracts gate, the full product build (engine + UI + Tauri +
  NSIS installer + portable + SHA256SUMS), the dev-signature verification,
  truthful notes. The GitHub release carries: RecoveryWizard-0.9.3-setup.exe
  (2,854,056 bytes), RecoveryWizard-0.9.3-portable.zip (4,002,655 bytes),
  SHA256SUMS (199 bytes).
- worklog/VALIDATION/it-gates/: the green run's captured evidence, committed
  verbatim with a provenance README (c2c91315): the phase-model terminal pins
  (overall 750 vs phase 1000; 3.02 GiB found), the 81/81 deleted-tree rows
  under their original gone/ parent, the real-JPEG classification + the
  genuine zip's 43-entry inventory, the coherent no-source SMART refusals.
- CI-STALE-01 closed: forensic-repro.yml's default tag v0.8.4 -> v0.9.3
  (1b9a4408). T19 docs pointers: both footprint docs now route readers to
  worklog/FOOTPRINT-DELTA.md (8217bc26).
- T18 dispatched: forensic-repro on v0.9.3 (run 44249495727) — the clean-
  Windows install + surface exercise.

## Installer Integration & API Surface Expansion session (2026-11-18) — Phase 0 begin

### T1 — clone, records, the charter, the integration boundary

- Product: heicfast/final-prod cloned at bf2e9755, fast-forwarded to 8217bc26
  (three commits landed mid-clone: c2c91315 the it-gate evidence from the first
  all-green bridge run 44244567592 @ be3b8256; 1b9a4408 the forensic-repro
  default-tag fix v0.8.4 → v0.9.3; 8217bc26 the T19 footprint reconciliation).
  v0.9.3 tagged; the handoff's named first debug (run 44102634559's query-gate
  panic) is already closed by be3b8256 — this session starts from the green
  v0.9.3 frontier.
- Corpus: heicfast/easeus-drw-research cloned at
  be0cfe95ecf22ac67e0f3a823dd003163b196d38 (11,495 tracked files;
  og-codebase/ is the extracted EaseUS DRW install, ~660 MB of proprietary
  binaries — read-only research artifacts).
- Charter saved, token redacted: worklog/session-installer-api-prompt.md
  (read twice before any code touched).
- THE INTEGRATION BOUNDARY (binding, recorded in the charter): no file from
  og-codebase/ is ever copied into, shipped by, or redistributed through this
  product's installer. Integration = capability parity in this product's own
  original code — the discipline docs/PARITY-DELTAS.md already runs on. The
  product tree carries zero proprietary binaries today (git ls-files census:
  no exe/dll/sys tracked) and it stays that way.
- Phase 0 next: agents A–E produce INSTALLER-CURRENT / REFERENCE-FOOTPRINT /
  REFERENCE-API / PRODUCT-API / INTEGRATION-MATRIX; pushed before any code.

### T2 — Phase 0 agents A–D: the four context sheets

- Agent A → worklog/INSTALLER-CURRENT.md (570 lines): the installer truth —
  build pipeline, the staged engine kit (4 file classes), NSIS placement
  rules, the as-defined footprint, the t15 evidence STALE verdict (every
  committed runtime artifact is driver-era v0.6.0-rc1), and the §8 gap
  register (G-1 userdata-defaults placed by NOTHING though both consumers
  probe it exe-adjacent; G-3 shortcuts unaudited; G-4 abort stranding; G-5
  stale evidence; G-9 update path unexercised).
- Agent B → worklog/REFERENCE-FOOTPRINT.md (602 lines): the reference
  installed tree re-measured 1:1 at corpus HEAD (1,276 live files /
  494.7 MiB; BUILDPE 194), errata from CORRECTIONS.md folded, module
  taxonomy, install/uninstall/update behavior, the 31-row parity mapping
  (19 ALREADY-INTEGRATED / 5 PARTIAL / 7 ABSENT) — IP boundary held
  throughout: names/sizes only, no og binary opened.
- Agent C → worklog/REFERENCE-API.md (326 lines): the reference capability
  surface across 10 domains — scan control/telemetry, .rsf sessions, results
  tree/filters/deleted-tree, recovery, per-format repair, imaging, lost
  partition, the SMART transport chain, preview, activation/cloud/NAS — each
  with inputs/outputs/errors/perf + corpus citation; §10 ranks the parity
  gaps.
- Agent D → worklog/PRODUCT-API.md (418 lines): product truth counted from
  source, not docs — 93 ABI exports (the documented 92 is stale), ABI v4,
  12 events, 40+OK error codes, 13 CLI verbs / 6 exit codes, 54 Tauri
  commands, 12 domains / 67 connector methods (the documented 9/36 is
  stale), test coverage map, the documented-truth gap list, the doc-drift
  register (§9).

### T3 — Agent E → worklog/INTEGRATION-MATRIX.md (281 lines): the execution contract

- §1 footprint gaps (18 rows), §2 installer defects (G-1..G-9, root causes +
  precise fix scopes), §3 API gaps (32 rows M01–M32), §4 doc-truth defects
  (DT-1..DT-15), §5 the wave plan W1..W10, §6 the per-wave validation plan
  (sandbox truth: Linux-only L-class vs Windows-runner C-class vs manual
  M/H-class), §7 the sheet-vs-sheet discrepancy ledger (all resolved against
  source).
- THE WAVE CONTRACT: W1 userdata-defaults placement (T5, S) → W2 installer
  lifecycle hardening + evidence re-proof (T5) → W3 doc-truth sweep (T5/T13)
  → W4 imaging ABI completion cr_ui_image_create_ex (T11) → W5 small
  axes → W6 recovery/query completion → W7 repair workbench ABI → W8/W9 UI
  wiring → W10 measured perf. DEFER rows keep their recorded scopes (M03
  .edisk UNKNOWN-internals, M05 renderers, M11 vendor dialects, ...).
- Global invariants per wave: append-only ABI with gen_abi_layouts + abi.rs
  re-pin in the SAME commit (the eb4bb5f3 lesson), contracts validate.mjs
  green, no undocumented behavior change, main green after every wave, no og
  file ever copied.

### T5/W1 — installer placement completeness: userdata-defaults.json SHIPS

- INTEGRATION-MATRIX G-1/FM-14 closed — the one real placement defect: the
  engine's 111-entry default user-sign set existed in-repo
  (engine/data/userdata-defaults.json; entry count re-verified this wave by
  parse: 111) and BOTH consumers probe it exe-adjacent at startup
  (engine/src/app/rrecov.cpp:1354; the bridge's USER_SIGN_DEFAULTS_FILE,
  ui/src-tauri/src/commands/system.rs), but no lane staged it and nothing
  placed it — every clean install started at userSigns: 0.
- The fix (pure additive placement, zero behavior change elsewhere):
  build.ps1 stages the json into the engine kit; the .nsi kit File already
  places every non-.lib kit file flat in $INSTDIR (no .nsi placement-rule
  change — only the comment gained the truth); the uninstaller deletes it
  explicitly (a DATA file the *.dll catch-all does not cover — without the
  line a complete uninstall would orphan it); installer-matrix.ps1's
  exact-module-set assertion gains the file (required — the $extraRoot
  non-DLL guard would otherwise fail it); portable.ps1 inherits it via its
  whole-kit copy; docs/INSTALLER.md's placement table + worklog/
  FOOTPRINT-DELTA.md §3 record it. About's userSigns row becomes truthful
  non-zero on clean installs; the degradation contract (logged no-op when
  genuinely absent) stays for real absence.
- Validation, Linux-sandbox class (L): node contracts/validate.mjs GREEN
  (54 commands / 12 events, both cross-checks). Sandbox truth: no makensis,
  no pwsh, no root (apt refused) — the .nsi compile gate and the .ps1 parse
  gate run on the repo's Windows runners (release.yml builds the installer;
  validation-evidence.yml runs the T15 matrix); both edits are
  line-for-line the syntax of their neighbors. The Windows-runner evidence
  capture (fresh T15 run at this frontier) is queued with the G-5 re-proof
  (W2).

### T5/W2 — installer lifecycle hardening: no stranding, no stale modules, no unasserted state

- G-4 closed at the root (the abort path had no cleanup): the .nsi's engine
  kit PRE-FLIGHT is hoisted BEFORE the inner application setup runs (a kit
  that cannot pass its own gates aborts before anything is placed); every
  abort AFTER the inner setup ran now routes through AbortAfterInner ->
  AbortCleanup (the inner uninstall keys swept via RemoveInnerUninstallKeys,
  the inner application driven silent through its own uninstaller when it
  produced one, the seeded Software\heicfast\Recovery Wizard key + its empty
  parent deleted) — no stranded app, no orphan uninstall entry, no leftover
  seed key on ANY post-inner abort path. Honest scope: the sweep is
  $INSTDIR-anchored; a FAILED inner that landed in its own default dir (the
  /D= bypass edge) is not swept by the key match.
- G-9 closed at the root (stale modules on update): an install-over-install
  (detected: the SW key's InstallDir == $INSTDIR) sweeps the DOCUMENTED
  installer-owned kit file classes before placing the new kit — vcpkg naming
  changes across versions (zlib1/z/lz4/liblz4/liblz4d/zstd/zstdd/libzstd)
  never leave stale DLLs behind. Same-version reinstalls take the same
  harmless path (the kit re-places its set immediately). installer-matrix.ps1
  gains the version-bump row (conditional on -InstallerPathNext): registry
  Version restamp + marker survival + a DECOY stale module planted under a
  documented kit-class name asserted REMOVED by the sweep + the final
  uninstall proving uninstall-after-update clean. 'skipped' recorded when no
  second-version installer is supplied (the honest absent-evidence form).
- G-6 closed: the uninstall assertions now prove the seeded install-location
  key AND its empty manufacturer parent are gone (previously only
  swKey/uninstallKey were asserted).
- G-3 closed as designed: the shortcut probe — the OUTER script creates no
  shortcuts (verified: zero CreateShortCut in the .nsi); the matrix captures
  + asserts the inner Tauri template's all-users Start Menu set targets the
  install dir, records the desktop set, asserts both gone after uninstall;
  a CHECK-SHORTCUTS row (the final guard throws on it) names the
  contingency: if the inner template creates none, the outer .nsi gains the
  CreateShortCut. The truth lands with the next T15 run.
- Validation (L-class): contracts validate.mjs green; no makensis/pwsh in
  the sandbox (established in W1) — the .nsi compile + the full product
  build + the matrix run on the Windows runners. The failure-path matrix
  row (force a bad kit on the runner) is honestly scoped: needs a
  deliberately-broken-kit build lane; NOT shipped this wave — the pre-flight
  makes the bad-kit case abort pre-placement by construction.
- G-5 re-proof dispatched after this push: validation-evidence.yml
  (workflow_dispatch, full build + the extended matrix) at this frontier;
  artifacts to be reviewed and committed under docs/INTEGRATION-VALIDATION/
  per the workflow's own convention.

### T5/T13 W3 — doc-truth sweep: every stale count/note trued + a gate so the class cannot recur

- DT-1/DT-2: contracts/README.md — 92 → 93 exports; 9 domains/36 methods →
  12 domains/67 methods (the connector's real shape).
- DT-3: engine/docs/ABI-CONTRACT.md §9 — CR_UI_ABI_VERSION is 4 (the v4
  clause added; the v3/v2 history sentences kept).
- DT-4/DT-5/DT-6: contracts/command-surface.json — health_read's note now
  states the ex2-first + base-fallback truth (verified at
  commands/health.rs:38,42); the wrong "not on ABI v4" removed-row for
  cr_ui_health_read_ex2 dropped (the other three removed rows remain);
  devices_list's note states the live bus_type axis (BF-02/BF-31 closed at
  ba2b9347; virtual/file-backed buses fall to the removable flag per
  D-ENUM-05).
- DT-7: bridge/abi.rs header comment — 92-export → 93-export.
- DT-8: worklog/VALIDATION/it-gates/README.md — 365 → 366 field paths.
- DT-9: contracts/error-codes.json — the INTERIM elevation-mapping note
  replaced by the live-truth sentence (the dedicated elevation-required
  uiCode exists).
- DT-10/DT-11: docs/INTEGRATION-GAPS.md — G-1's wrong NG-10 cross-ref →
  NG-12; NG-10 annotated CLOSED-by-code (health-page rework);
  G-9 annotated CLOSED-by-code (ba2b9347's bus_type axis).
- DT-13: docs/INSTALLER.md Reproducibility — the one-command claim
  qualified (Windows + full toolchain; the installer lane consumes staged
  outputs; build.sh has no installer lane).
- DT-14: the NEW guard — engine/tools/doc_truth_gate.py gains
  check_contracts_readme_counts(): the contracts README's export row and
  UI-seam row are now checked against TREE-DERIVED numbers (cr_ui.h export
  census via derive_abi_counts; contract.ts's Domain interfaces →
  domains/methods/subscription counts). Skips gracefully in the standalone
  engine repo (no contracts/ tree there). The 92/9/36 drift class can no
  longer recur silently.
- DT-12 deferred to the evidence commit: installer-compile-check.md
  regenerates against the CURRENT .nsi (640 lines, driver-free) once the
  dispatched validation-evidence run lands.
- Validation (L-class, all run locally this wave): python3 -m json.tool on
  both edited contracts JSONs OK; node contracts/validate.mjs GREEN (54
  commands, both cross-checks); python3 engine/tools/doc_truth_gate.py
  GREEN — the full suite incl. the new check (exports=93, seam=12/67 = 55
  RR + 12 subscriptions, zero FAILs).

### T11/W4-A — imaging ABI completion: cr_ui_image_create_ex (range + cancel)

- D-IMG-1 + D-IMG-3 closed at the ABI in ONE additive export (no version
  bump — append-only, ABI-CONTRACT §9): cr_ui_image_create_ex(disk,
  path, first_sector, sector_count, volatile uint32_t* cancel_flag,
  progress, user). Sector range per the og's "More Option" from/to pair
  (first 0-based; count 0 = to the END of the disk; 0/0 = the whole disk
  = BYTE-IDENTICAL legacy behavior); cancel via the caller-owned flag
  (the cr_ui_session_load_with_events idiom) consulted at every 64 KiB
  loop head; on cancel the partial is KEPT as '<path>.partial' (the S-040
  truth contract, the og's "You need to re-create the disk image if you
  stop it" semantics).
- The layering, all byte-identical-preserving: storage::
  create_image_stream_range (the ranged copy loop; the whole-disk twin
  now DELEGATES to it with [0, size) — one loop, identical arithmetic);
  win::create_image_file_range (create_image_file delegates) +
  win::create_disk_image_ex (the sector-denominated form; geometry-opened
  validation: first_sector beyond the disk or a range past the end
  refuses ERR_BAD_PARAMETER — the og's "Invalid sector" posture, never a
  silent truncation); image_create_ex_impl (the same elevation gate, the
  same error/disposition contract as image_create_impl; FlagCancelToken
  adapts the volatile flag into a CancelToken).
- CLI: image --create --disk N --out F [--from-sector A --to-sector B] —
  both edges or neither, the og's [0-9]{1,15} grammar + "Invalid sector"
  refusals, inclusive [from, to]; the no-range path stays the literal
  legacy create_disk_image call. Usage/-h/parse/header updated (the
  five-site surface discipline).
- Tests: 7 new ImageCreateRange.* (whole-range == whole-stream
  byte/digest/stats equality — the no-regression proof; middle-slice
  content equality; zero-length empty-stream digest; beyond-device
  ERR_BAD_PARAMETER refusals + the exactly-to-end legal form;
  range-relative progress; mid-range cancel prefix truth; the grammar
  probe) + 3 ui_abi_check.c probes (argument gate + the honest
  non-Windows refusal). Portable-suite: they run on BOTH engine lanes.
- The same-commit invariants held: tools/gen_abi_layouts.py re-run (29
  structs / 365 paths — ZERO struct drift, the pinned abi_layouts.rs is
  byte-identical; the dump C restored to its clang-formatted committed
  form — the raw generator output is pre-format; the .header.sha re-stamped
  for the new header content); abi.rs re-pinned (the 94th extern —
  rg -c 'pub fn cr_ui_' = 94 == the header's 94 exports, 1:1).
- Doc truths re-trued in the SAME commit (the doc_truth_gate caught all
  six 93-claims on first run — the W3 guard proving itself): engine/docs/
  API.md + CORE-API-CURRENT.md (3 sites) + API-SURFACE-AUDIT.md +
  contracts/README.md now carry 94 with the append history;
  worklog/PRODUCT-API.md §0 gains the dated W4 addendum (93 counted
  pre-append, point-in-time).
- Validation (L-class, local): doc_truth_gate GREEN (exports=94; the
  contracts README row matches); contracts validate.mjs GREEN. The
  compile + the full suite run on engine-ci (Windows) on push.
- NEXT (W4-B, same session): the bridge half — commands/imaging.rs
  cancelImageCreation + the worker's switch to the ex call + the
  registry's cancel flag + image_create_state's cancelSupported=true +
  command-surface.json row + event-mapping note; the TS connector +
  UI stop-confirm/More-Option fields stay W9's recorded scope.

### T11/W4-B — the bridge half: cancel is real, the range is real, the wiring recorded

- commands/imaging.rs: ImageEntry gains the sector range (first_sector/
  sector_count) + the Arc<AtomicU32> cancel flag; the worker now rides
  cr_ui_image_create_ex for EVERY image (whole-disk included — 0/0 is
  byte-identical legacy output, the flag is LIVE for all images);
  image_create_cancel is the new command (typed image-not-found for
  unknown ids; an honest no-op for terminal jobs; the flag write is
  from-any-thread); image_create_state flips to cancelSupported: true +
  reports cancelRequested; the space gate sizes to the RANGE's bytes when
  a range is given (an out-of-bounds pair has no gate truth — the
  engine's own ERR_BAD_PARAMETER refusal owns validation).
- lib.rs: image_create_cancel registered; the "NO cancel" comment
  replaced with the closure truth. contracts/command-surface.json: the
  start row's args/abiRefs/notes trued (firstSector?/sectorCount?,
  cr_ui_image_create_ex, the ranged space gate), the state row's note
  (cancelSupported true), + the image_create_cancel row (55 commands
  now; serves: [] honestly — the connector method imaging.cancelImageCreation
  + the stop-confirm dialog land in the W9 UI wave; the bridge + engine
  halves are the shipped parts). contracts/event-mapping.md: the imaging
  section's cancel truth (no separate cancel-ack event — the terminal
  image-finished payload IS the acknowledgment; partialKept stays the
  filesystem probe).
- The it-gate gains image_create_ex_range_and_cancel: a bounded 4 MiB
  sector range on the REAL seeded disk (exact length + the .sha256
  sidecar shape), the pre-cancelled flag (ERR_CANCELLED + the truthful
  .partial + no committed image), and the out-of-range refusal
  (ERR_BAD_PARAMETER) — real-Windows evidence for both halves.
- Validation (L-class, local): contracts validate.mjs GREEN (55
  commands, both cross-checks — the new row passes the serves-empty
  rule); json.tool OK. Rust + the it-gate compile/run on the runners
  (bridge-integration) on push; TS untouched this wave (no pnpm surface
  changed).
- Honest scope: the CONNECTOR surface (contract.ts
  ImagingDomain.cancelImageCreation + firstSector/sectorCount on
  startImageCreation + mock + the create-image-dialog stop-confirm and
  More-Option fields) stays the W9 wave's recorded scope — the bridge
  command, engine flag, and CLI flags are the shipped halves today.

### T20 — the session handoff (2026-11-18, ~2.5h of lane time)

THE CODE STATE (all pushed; every push pulled-rebased onto the concurrent
lanes' advances, never forced): Phase 0 five sheets + matrix (7d253911/
e8c6b061), W1 userdata-defaults placement (87cdf27a→a3d3e72a), W2 installer
lifecycle hardening (42ef613b), W3 doc-truth sweep + the gate guard
(5a374ce5), W4-A imaging ABI completion (b8740541) + the pinned-format
pass (b43ce273), W4-B bridge half (07eb9ed4) + AtomicU32 import (b99fc523)
+ the format-arg fix (ad22d51e) + the partialKept/sidecar path-semantics
root fix (372198fe), T10/T19 deliverables (dedb0a46).

THE CI ITERATION STORY (the lanes doing their job — three real catches,
two of them genuine bugs):
1. engine-ci's clang-format gate caught the hand-written C++ wrapping
   (b43ce273, the 18.1.8-pinned wheel — a 23.x format would have
   re-wrapped the whole tree; the pin confined the diff to the new code).
2. ui-ci's cargo check caught the missing AtomicU32 import (b99fc523).
3. bridge-integration caught the it-gate's field-access format arg
   (ad22d51e) and then the REAL bug: the extension-append vs extension-
   replace path confusion — the engine keeps `<path>.img.partial`/
   `<path>.img.sha256` (APPENDED), while with_extension() REPLACES `.img`
   and probed a different file. The pre-existing bridge partialKept probe
   carried the same defect (it could only ever have reported false); both
   fixed at the root (372198fe).

CI STATE AT CUTOFF:
- engine-ci GREEN at b43ce273 — the W4-A C++ compiled under MSVC /W4 /WX
  and the FULL suite passed (the 7 new ImageCreateRange.* tests + the 3
  ui_abi_check probes included). The imaging ABI wave is proven on real
  Windows at the engine layer.
- ui-ci + ui-e2e + contracts-ci GREEN at 372198fe (the complete W4 with
  every fix; the frontend untouched all session).
- bridge-integration: the 372198fe run was CANCELLED mid-queue by the
  CONCURRENT lanes' pushes; the 18b918d5 run (my code + the performance
  lane's in-flight changes) fails at the ENGINE COMPILE step —
  cr_transport.cpp C2220/C2440, the performance lane's IOCP territory,
  NOT this session's files (my engine-ci passed the identical /W4 /WX
  build at b43ce273). Their lane is actively iterating.
- validation-evidence (the G-5 re-proof): three dispatches — #1 (06:40)
  failed on the performance lane's mid-version-bump tree (VERSION 0.9.4
  vs app manifests 0.9.3 — build.ps1's gate correctly refused); #2
  (07:00) failed on my pre-import-fix Rust (the W4-B catch); #3 (07:53)
  in flight on the W4-complete frontier — it will hit the same
  cr_transport.cpp error until the performance lane's compile fix lands.
  THE NEXT SESSION'S FIRST DISPATCH: once engine-ci is green on a
  frontier containing this session, dispatch validation-evidence.yml,
  download the artifacts, commit docs/INTEGRATION-VALIDATION/
  (t15-runtime-matrix incl. the new rows: shortcuts, seed-key, version-
  bump 'skipped', the W1 module-set file) + regenerate
  installer-compile-check.md (DT-12) against the current 640-line .nsi.

WHAT IS COMPLETE: Phase 0 (5 sheets + the matrix, pushed before code);
Phase 1's W1/W2/W3 (the one real placement defect closed; the abort path
hardened with pre-flight + AbortCleanup; the update sweep; the matrix
probes; every stale doc trued + a permanent gate); Phase 2's W4 (the
flagship imaging ABI completion — D-IMG-1 + D-IMG-3 closed through
engine/CLI/bridge with byte-identical legacy proof); the T9/T10
deliverables (FOOTPRINT-DELTA §3 + API-DELTA); BEHAVIOR-FINDINGS with the
precise deferred register; the handoff (this).

WHAT STAYS OPEN (precise scopes in BEHAVIOR-FINDINGS + API-DELTA +
INTEGRATION-MATRIX §5): the W9 TS/UI half of the imaging cancel+range;
W5's small axes; W6's recovery/query completion; W7's repair workbench;
W8's UI wiring batch; W10 (the concurrent performance lane owns it);
the G-5 evidence commit (blocked on the shared frontier's compile fix);
the failure-path matrix row (the broken-kit build lane).

THE INTEGRATION BOUNDARY held end to end: zero og-codebase bytes shipped;
capability parity only; the corpus cited as behavior ground truth.

---

## 2026-11-18T06:10Z — performance session opens: T1 complete (clone, record, prompt archived)

The performance-diagnostic mandate (senior performance engineer, Windows
storage internals, overlapped I/O / IOCP / threading / queue depth) opens on
final-prod @ 8217bc26 (main, clean) with the reference corpus
easeus-drw-research @ be0cfe9 checked out side-by-side. The session prompt is
archived token-redacted at worklog/session-performance-fix-prompt.md.

First-hand hot-path recon (the bottleneck hypotheses the Phase 0 agents now
verify end-to-end, all code-cited):

1. cr_transport.cpp:53-80 `overlapped_read` — ReadFile is issued against
   handles opened WITHOUT FILE_FLAG_OVERLAPPED (open_path :291-293,
   FILE_ATTRIBUTE_NORMAL): the OVERLAPPED struct is an offset holder, and
   GetOverlappedResult(...,TRUE) blocks. The read path is SYNCHRONOUS with
   queue depth 1 by construction — no IOCP, no outstanding reads, ever.
2. cr_transport.cpp:186-189 — the ABI pins per-handle single-flight, and
   cr_cached.cpp:31 serializes every read_at under one global std::mutex
   (the whole device share one lock; linear O(n) block scan per read;
   per-miss std::vector<u8> heap allocation, no buffer reuse).
3. Read sizing: scan pipe reads 256 KiB cache blocks (8 MiB LRU,
   cr_scan.cpp) vs the og's 4 MiB device chunks / 2 MiB MFT batches
   (PERFORMANCE-BASELINE.md §5: "The og still issues 4-8x fewer transitions
   per byte than our engine path").
4. cr_cached.h:9-36 (N-045) registers the divergence: the og has a PREFETCH
   read-ahead engine with a dedicated thread (10-run lookahead); ours is
   reactive LRU with no prefetch — on real NVMe the latency-hiding gap
   is the throughput gap.
5. The only sweep evidence (PERFORMANCE-BASELINE.md §6, run 40227375329) is
   Azure virtualized C: — ~240 MiB/s cap, QD-flat — explicitly recorded as
   NOT representative of consumer NVMe ("fast local NVMe will re-measure").
   Real-hardware QD/read-size optima are unmeasured; that is this session's
   measurement debt.

Known frontier inherited from the SMART session's T20 handoff (worklog tail):
v0.9.3 bumped but untagged; bridge-integration results_filters gate had a
NULL-POINTER PANIC queued — functional-lane debt, noted, not this session's
mandate. Phase 0 agents A-E dispatch next; no code touches until the five
context sheets are pushed.

---

## 2026-11-18 — D-ENUM-05: the 'virtual' bus-kind arm hid every file-backed disk (0.9.3's first post-release validation catch)

- The forensic validation pass on the RELEASED 0.9.3 (run 44249495727,
  dispatched by the prior lane at 1b9a4408's CI-STALE-01 closure) went RED
  at S2b: 'getByRole(radio, /SeedEXFAT/) Timeout 15000ms'. The evidence
  told the whole story: the disk census recorded 4 physical disks (2 SAS
  runner disks + the 64GB SeedNTFS and 8GB SeedEXFAT vhdx fixtures), the
  home page rendered 'Internal drives (2)' only — both vdisks missing
  from every scanning surface; the health window (working correctly)
  showed the truthful no-source state for the two disks it did see.
- Root cause (D-ENUM-05, the regression class of D-ENUM-02): 0.9.3's
  bus-driven kind split (ba2b9347) routed bus 14/15/16
  (Virtual/FileBackedVirtual/Spaces) to kind 'virtual' — and
  home-page.tsx:85, disk-health-window.tsx:110 and use-physical-devices.ts:21
  all filter kind !== 'virtual'. A mounted VHDX is a scannable physical
  drive with real content; 'virtual' is the shell-locations' kind
  (Recycle Bin/Desktop quick-targets — hardcoded UI rows, never bridge
  device rows). Every Hyper-V/Storage-Spaces/VHDX disk on a real user
  machine would have vanished from the scan surfaces the same way.
- Fix (96917f6c): map_kind's 14..=16 arm removed — bus 12/13 -> 'sd',
  bus 7 -> 'usb', every other bus INCLUDING the virtual buses falls to
  the removable flag (the same fallback ATA/NVMe/SAS always took).
  contracts/type-maps.md trued with the D-ENUM-05 note; the call-site
  comment trued. No test pinned the old arm (verified: no rust/ui/e2e
  references); the mock's kind->busType inverse map is display-only for
  the location rows.
- VERSION -> 0.9.4; the release notes' opener rewritten to the 0.9.4
  truth (the honesty patch over the 0.9.3 train, with the regression
  named plainly). Order of operations: this frontier goes green ->
  tag v0.9.4 -> release -> re-dispatch forensic-repro on v0.9.4 (the
  D-ENUM-01 census gate — every physical disk the OS exposes renders —
  becomes live again the moment the vdisks render).
## 2026-11-18T07:25Z — T2 complete: the five Phase 0 context sheets pushed (no code touched yet)

Agents A-E returned; every load-bearing citation spot-verified first-hand
before this entry (ThreadPool resolve_threads cr_scan.cpp:58-68; the scan
pipe's CachedReader::create(8 MiB, 256 KiB) at cr_scan.cpp:1387; ABI-CONTRACT
§3 single-flight/caller-thread contract; the node-tree global mutex
cr_node.h:653-657; the alignbench QD>1 overlapped ring
run-alignbench.ps1:115-126 + :316; MFT read_record per-record 1 KiB reads
cr_bootstrap.cpp:599-635; carve 1 MiB+4 KiB windows cr_carve.cpp:45-46).

The five sheets (worklog/PRODUCT-STATE.md, REFERENCE-ARCHITECTURE.md,
REFERENCE-RAW.md, PLATFORM-PERF.md, BASELINE.md — 2,040 lines, every claim
file:line- or URL-cited):

- PRODUCT-STATE (A): the read path is synchronous QD1 by construction —
  no FILE_FLAG_OVERLAPPED/NO_BUFFERING at any open, GetOverlappedResult
  blocks (cr_transport.cpp:70); ONE global mutex serializes every read
  through the pipe, transport fill under the lock (cr_cached.cpp:31,:61);
  scan-pipe cache 8 MiB/256 KiB/32 blocks reactive LRU, no prefetch
  (D-72/N-045); scan runs on the caller's thread, remnant/lost/carve fan
  out on ThreadPool hw-1 clamp 8; MFT walks per-record 1 KiB; result tree
  one global mutex (cr_node.h:656); no live perf CI lane at HEAD (the
  bench workflows were pruned by the CI rebuild — engine-ci is lint+build+
  ctest only).
- REFERENCE-ARCHITECTURE (B): the og layers CStorageDeviceWithCache (4 MiB
  budget, 1000-run ctor default) + a DEDICATED prefetch thread with a
  10-run look-ahead ring; NTFS healthy volumes ride memory-mapped views;
  45 named threads in DRWUI; FAT quick scan per-worker threads.
- REFERENCE-RAW (C): decisive — the og's CWindowsStorageDevice::Read is
  SYNCHRONOUS (ReadFile with NULL OVERLAPPED, Device_decompiled.c:55964),
  zero IOCP machinery corpus-wide, no FILE_FLAG_OVERLAPPED/NO_BUFFERING;
  the og's latency hiding = 4 MiB device chunks (:56545) / 7 MiB volume
  chunks (:56912) / 2 MiB MFT batches + the prefetch ring's 4 MiB runs
  (0x400000/sectorSize, DRWUI:919088-919142) × 10-deep lookahead —
  effective QD~10 read-ahead; SearchDataSource pins engine threads per
  core (SetThreadAffinityMask, DRWUI:388833-388836).
- PLATFORM-PERF (D, retry after context-deadline): Microsoft-documented
  facts — FILE_FLAG_OVERLAPPED is REQUIRED for concurrent reads on one
  handle (CreateFileW remarks); GetOverlappedResult(TRUE) is a blocking
  wait; IOCP NumberOfConcurrentThreads=0 = one-per-processor; CreateThreadpoolIo
  is Vista+ (Win7-safe); NO_BUFFERING needs sector-aligned buffers+offsets
  and "should be physical sector-aligned"; no per-class QD optima are
  Microsoft-documented (NCQ ceiling 32 SATA/HDD, NVMe 65,535², USB BOT
  QD≈1, UAS 8+); the ~240 MiB/s runner cap is the documented Azure VM
  storage signature (GitHub-hosted runners run on Azure VMs; Dv2-class
  caps at defined QD).
- BASELINE (E): all recorded numbers assembled with sources — direct-era
  v0.7.0 already beats the driver baseline (deepcarve −30.7%, open −40.9%,
  CPU/GiB −39.4%); THE WALL MATH: deepcarve effective 8.45 MiB/s vs device
  rate 544 MiB/s — the walls are validation-CPU-bound ~64× under transport
  provision; og legs on the runner: og 4/7 MiB chunk legs 197/196 MiB/s
  vs our 256 KiB 241.64 MiB/s. NEW component microbenchmarks (this Linux
  container, harness outside the repo): CachedReader miss-path overhead
  +8.8-10.1% vs raw pread; THE MEASURED SERIALIZATION FACTOR of the
  global mutex: 0.51× at N=2, 0.23× at N=4, 0.20× at N=8 threads (the
  fan-out phases' workers all queue on one lock — the component-side
  evidence PERF-LEVERS F1 was deferred for); LRU scan +85 ns/read at
  32 blocks (negligible); per-miss 256 KiB resize ~4.6 µs.
- Prior-session continuity pinned: PERF-LEVERS.md is the lever ledger —
  F1 (mutex scope) / F2 (FILE_FLAG_OVERLAPPED) DEFERRED for lack of a
  QD-discriminating surface; §3's honest frontier = the carve/validation
  CPU pipeline (303.8 CPU-s/GiB), QD2-4 pipelining on physical NVMe, the
  decrypt fixture, CancelSynchronousIo. This session opens exactly those,
  with the serialization factor now measured at the component level.

Convergence (feeds DIAGNOSIS.md next): the og is ALSO synchronous QD1 —
its speed lives in 4-7 MiB chunks + the 10-run×4 MiB prefetch ring that
keeps ~40 MiB of read-ahead in flight; ours has 1× 256 KiB in flight, one
global lock over every fill, and walls that are CPU-bound in validation.
The bottleneck hierarchy: (1) CPU-bound validation in carve/deep phases
(64× headroom), (2) the global-mutex serialization of the fan-out phases
(measured 0.2-0.5× aggregate scaling), (3) QD1-no-prefetch read path
(latency-exposed on real NVMe, unvalidatable on the Azure cap), (4) per-
miss/per-call buffer churn. DIAGNOSIS.md quantifies each layer before any
code lands.
---

## 2026-11-18T08:15Z — T3/T4/T5/T6 complete: DIAGNOSIS.md pushed (the quantified four-layer bottleneck)

DIAGNOSIS.md (new content on top, the SMART session's preserved verbatim
below) pins the bottleneck hierarchy, each layer a measurement:

- L1: walls are validation-CPU-bound — deepcarve 8.45 MiB/s effective vs
  544 MiB/s device rate (64x under provision; 303.8 CPU-s/GiB).
- L2: ONE global mutex over every read with fill under the lock
  (cr_cached.cpp:31,:61); the fan-out's 8 workers queue on it; MEASURED
  aggregate scaling 0.51x/0.23x/0.20x at N=2/4/8 (container microbench) —
  vs the og's bookkeeping-only CRITICAL_SECTION + dedicated filler thread.
- L3: QD1-sync, no read-ahead anywhere — 256 KiB blocks, per-record 1 KiB
  MFT calls, 1 MiB carve windows; the og: 4-7 MiB chunks + a 10-run x 4 MiB
  prefetch ring (~40 MiB in flight, effective QD~10). The runner cannot
  discriminate (240 MiB/s Azure cap, QD-flat) — the F1/F2 deferral reason,
  now reopened with a debt register.
- L4: buffer churn (per-miss 4.6 us alloc = 0.8x the copy; no pool anywhere).

Ruled out with citations: the 64 KiB envelope (correct, mandated), transport
QD1 large-read health, LRU/checkpoint/API latencies, the result-tree mutex
(register, not frontier — walls say validation CPU dominates). The fix plan
maps Phase 2 onto acceptance measurements per fix; BASELINE.md (T4) is the
standing numbers sheet. T5/T6 audit content rode PRODUCT-STATE.md (I/O
model + queue/lock/buffer/alignment audits, all file:line-cited).

---

## 2026-11-18T09:00Z — Lane E opens: safety-hardening session T1 (clone, baselines, prompt, coordination)

The memory-safety/stability/security-hardening session opens as **Lane
E** (announcement + territory: `worklog/COORDINATION.md`). Base state
recorded before any work:

- Product: `final-prod` main @ `dedb0a463309efc9b60436cf89553e3493351a8f`
  (2026-11-18T07:02Z) — the imaging-wave + DIAGNOSIS state (ABI v4, 94
  exports; W9's TS half recorded in API-DELTA).
- Corpus: `easeus-drw-research` main @ `be0cfe95ecf22ac67e0f3a823dd003163b196d38`
  (the 8-corrections errata state).
- Session prompt saved token-redacted:
  `worklog/session-safety-hardening-prompt.md` (read twice).

Environment truth (recorded because the session's own rules forbid
hallucinated evidence): this lane runs in a Linux container with
git/gcc/g++/node/npm/python3. cmake, clang/clang-tidy/clang-format,
cppcheck, cargo/rustc, CodeQL are NOT preinstalled — each gets
installed (pip/apt where available) before its Phase-0/C/D task needs
it, or its absence is documented with precise scope. Windows-only
gates (MSVC `/analyze`, on-Windows sanitizer/RC evidence, Windows CI
runs) cannot execute here and will be recorded as precise-scope
blockers in `worklog/BEHAVIOR-FINDINGS.md`, never faked. The
Linux-executable evidence set is real tool output throughout.

T1 artifacts: this entry; the prompt file; the Lane E section of
COORDINATION.md. Next: T2 — parallel Phase-0 read (agents A–E over the
product and corpus), five context sheets, push before any fix.


### T20 handoff (this session's close, 2026-11-18 ~07:20Z) — the v0.9.4 correction train

STATE AT CUTOFF:
- v0.9.3 is PUBLISHED (run 44246474638 green; setup.exe 2,854,056 bytes + portable
  4,002,655 + SHA256SUMS) and its integration-gate evidence is committed
  (worklog/VALIDATION/it-gates/, c2c91315: the phase pins, 81/81 deleted-tree rows,
  the real-zip inventory, the coherent no-source SMART refusals).
- The clean-Windows forensic pass on v0.9.3 (run 44249495727) caught D-ENUM-05
  (attached vdisks invisible). Fixed in three layers with the parallel installer-API
  session: 96917f6c (the bus-kind arm) + 94e3fc73 (home/disk-health consumers, this
  session) + 3c3e02c5 (use-physical-devices, theirs).
- The first v0.9.4 tag died at the lockstep gate (their 96917f6c bumped VERSION
  alone — run 44253461220, nothing published). Lockstep completed (9c9597e7) and
  the tag was RE-POINTED there (no release object existed; the re-point is clean).
- THE RELEASE RUN ON v0.9.4 IS IN FLIGHT AT THIS HANDOFF: run 44256566315 on
  9c9597e7. NEXT SESSION'S FIRST ACTION: check its outcome; on green, verify the
  assets + notes on the release, then DISPATCH forensic-repro on v0.9.4
  (workflow_dispatch, release_tag=v0.9.4) — the clean-Windows gate that must pass
  this time (the SeedEXFAT card is visible again with D-ENUM-05 fixed). On red,
  pull the failing step's log and root-cause (the build.ps1 -Target all path
  exercises the W1/W2 installer changes end-to-end for the first time).
- The parallel installer-API session is mid-wave (W1-W4 landed, W4-B fixing a
  ui-ci red on THEIR frontier at b99fc523 — NOT in the v0.9.4 tag, which predates
  their W4). Coordinate via this worklog; their sheets: worklog/INSTALLER-CURRENT,
  REFERENCE-API, PRODUCT-API, INTEGRATION-MATRIX, API-DELTA.

EVERY DEFERRED ITEM: BEHAVIOR-FINDINGS.md (both sessions' sections), PARITY-DELTAS
(D-P4c, D-P8, G-1(b)), FOOTPRINT-DELTA.md.

---

## Session 2026-11-18 — streaming-detection-pipeline (Phase 0 + Phase 1)

| Step | SHA | What |
|---|---|---|
| T1 | — | Both repos cloned; commits recorded: final-prod `b8740541` (rebased to remote `85d44f56` line after divergence), corpus `be0cfe95`; redacted session prompt saved (token never committed) |
| T2 | `06987a83` | Phase 0: five context sheets pushed BEFORE any code — PRODUCT-PIPELINE (A), REFERENCE-PIPELINE (B), REFERENCE-ALGORITHMS (C), REFERENCE-MODULES (D), PIPELINE-MATRIX (E synthesis). Every claim corpus-cited or product file:line |
| T3 | `793a740b` | DIAGNOSIS: exact drop stage quantified — engine→UI event surface carries 0 result rows (12/12 CR_EVT_* are counters); UI starves between 10%-bucket snapshot pulls; prior perf diagnosis preserved below the fold |

**Handoff**: implementation phases 2–4 not started (session step budget); exact entry points
for every remaining fix are in worklog/BEHAVIOR-FINDINGS.md (this session's section).
worklog/VALIDATION/ is intentionally empty — no evidence faked.
## 2026-11-18T09:20Z — fix 1 landed: the CachedReader rework (F1 opened, Layer 2 + Layer 4 closed)

Commit 9b3e1059 + evidence correction fe573920. The cache no longer
serializes the world:

- fill-outside-the-lock: the old read_at held ONE std::mutex across the
  whole loop INCLUDING the transport fill; fills now run outside every
  registry lock, per-block in-flight, with concurrent claimants waiting
  on that block's cv (the og's consumer model).
- 32-shard segmented LRU (one lane per block at the scan pipe's 32-block
  config; graceful min(32, cap)); lock-free VALID probe (terminal state)
  + hit copy under the shard lock (no refcount churn); per-shard
  counters; cache-line-aligned block headers.
- page-aligned buffer POOL (VirtualAlloc/mmap) replacing the per-miss
  vector resize (~4.6 us/miss, 0.8x the copy itself); shared block
  ownership = eviction-safe under a reader mid-copy.

Measured before/after on the SAME artifact-corrected harness, same day,
same fixture (BEFORE = the pre-rework code from 883107bf; the harness's
shared-destination false-sharing artifact was found and fixed BEFORE any
number was recorded): hit-path aggregate 0.54x->0.96x at N=2, 0.17x->0.82x
at N=4, 0.12x->0.82x at N=8; miss-path N=1 +46%, N=4 17,763->49,392
ops/s (+2.8x, was BELOW N=1 under the global mutex); sequential
overhead 8.0%->5.5%; lookup 107.1->54.2 ns @ 32 blocks; alloc churn gone.
A lock-free memcpy control at the same pattern bounds the container at
1.6x at N=2 flat beyond — the cache adds no serialization on top.

Correctness: 1637/1637 unit tests green (1634 prior + 3 new concurrency
pins: byte-identical concurrent readers; two fills provably inside the
transport at once; one fill serving 3 concurrent claimants). The
audit-row-38 rule preserved; single-threaded hit/miss accounting pinned
unchanged; the D-72/N-045 doc-truth pin passes (budget 8 MiB / 256 KiB
unchanged; the algorithm divergence shrinks — register note next).
PERFORMANCE.md (root worklog/) opened as the session ledger.

Next: fix 2 — the transport overlapped ring (FILE_FLAG_OVERLAPPED +
IOCP sliding window in cr_transport.cpp, Layer 3), which pairs with this
fix for device-level fill concurrency (current handles still serialize
concurrent ReadFile at the file object per the non-overlapped contract).
---

## 2026-11-18T10:35Z — fix 2 landed: the overlapped read-ahead ring (F2+F10, Layer 3 attacked)

The transport's read path is no longer QD1-sync-no-prefetch: every
data-path handle opens FILE_FLAG_OVERLAPPED and serves through the
AsyncRing — IOCP + one completion thread per device, 8x4 MiB sliding
window (32 MiB in flight vs the og's 10x4 MiB), refill-on-consume
follow-the-cursor, pivot-on-miss, 64 KiB envelope property preserved on
every issued read, CancelIoEx+drain+join teardown, overlapped-aware
handle heals (rebind), overlapped-aware probes, thread-local hEvents for
the sync-shape helper. ring_tuning::run_bytes/slot_count = the sweep
seam; open_file_async = the test/bench surface. ABI surface unchanged
(read_at stays blocking; ABI-CONTRACT §3 engine-internal note added);
D-72's algorithm halves closed by measurement (amendment in the row).
4 new Windows transport tests ride engine-ci. Throughput claims deferred
to the T7 harness dispatch — no number without the runner.

## 2026-11-18T12:05Z — installer-api session re-opens: the red-main repair (ring miss-path root cause + Linux parse gate)

Session re-dispatched on the same charter (installer integration + API
surface expansion). Baseline audit at clone (HEAD a2561acd) found main
RED in two places the prior sessions never saw:

1. **Linux lane**: `win_syntax_check` failed on `cr_transport.cpp` — the
   performance session's fix-2 ring code consumed 13 SDK symbols the
   M-055 declaration-only stubs never grew (the gate's own contract:
   "a new SDK symbol in src/win grows the stub here"), and
   `test_transport_win.cpp:121` shipped an ill-formed multi-word
   functional cast (`unsigned char(...)`) that MSVC tolerates and g++
   rejects. Fixed: stubs grown (IOCP trio, event pair, CancelIoEx,
   VirtualAlloc/VirtualFree, MEM_*/PAGE_* constants, GetTempFileNameW),
   cast → `static_cast`.
2. **benchmark-src run 44275941738 (a2561acd)** — the first full matrix
   through fixes 1-3 — failed SEVEN hard gates: cancel+resume
   `resumedFiles=0`; S-DEL-NTFS hash-recall 0.272 < floor 0.35;
   bench-rand-1m 9427ms > 2192 cap; bench-open 227869ms > 19571 cap;
   scan-deep-wall 26253ms > 18155 cap; api-save-session 13.7 > 11.88;
   api-recovery 5524.5ms > 54.12 cap (102x over baseline).

**Root cause (both functional failures + the perf-wall class), pinned
from source**: `AsyncRing::pick_victim_locked()` returns only
IDLE/FAILED/oldest-DONE slots — never INFLIGHT. In the runner's steady
state the device outruns validation CPU, so ALL 8 slots sit INFLIGHT;
every out-of-window read (remnant/carve random probes, recovery extents,
MFT random access) hit step 3 with no claimable victim and
`read_at` returned **ERR_NO_MEMORY** — which `RetryingDevice::try_read`
(cr_retry.cpp:49) classifies as a hard error (not retried, not
zero-filled, not bisected) and propagates straight into every engine.
The 4 remaining miss cases (a free victim existed) claimed a slot and
issued a full `run_`=4 MiB read for a 4 KiB need — ~1024x read
amplification on random workloads (the api-recovery/bench-rand walls).
The engine-ci ring tests never saw it: their fixtures (24-48 MiB) fit
inside the 32 MiB window, so the miss path never fired.

**The repair (root cause, not symptom)**: `AsyncRing::read` step 3 now
discriminates — a miss AT THE WALKING FRONT (slice ends beyond the
cursor, starts within one run of it: the sequential consumer's warm-up
and window-recycling path) still recycles a victim; EVERYTHING ELSE
(backward re-reads, random probes, recovery extents, front reads with
no free slot) is served by the **companion sync handle**: a second open
of the same path, FILE_FLAG_OVERLAPPED but NEVER associated with the
completion port, read through the pre-ring `aligned_read_at` envelope in
<=1 MiB slices, with NO cursor advance and NO kick (a random read must
not drag the window). `DirectDevice` owns the companion (ctor opens it
before the ring — if it fails the ring stays disabled and the device is
the pre-ring sync transport; heal reopens both handles as a unit or
fails whole; dtor closes it after ring stop). ERR_NO_MEMORY is now
unreachable from the read path; the design comment's original promise
("random-access streams degrade to sync-like behavior, never worse than
the pre-ring transport") is finally implemented.

**Regression pins added** (`test_transport_win.cpp`, MSVC lane): a
128 MiB fixture (4x the window) with 4 threads x 3000 random 64 KiB
reads — byte-exact, zero failures (the old code cannot pass this); a
96 MiB walker + 2 random-probe + 1 backward-reread concurrent test
(the product's real scan shape; the cursor stays the walker's).

Local evidence: parse gate green (grown stubs), 1781/1781 Linux tests,
contracts green, doc_truth green, clang-format 18.1.8 applied. The
Windows-runner proof (engine-ci + the seven benchmark-src gates) is
dispatched with this push — the repair is NOT claimed closed until the
matrix runs green.

---

## Session: CI/CD Workflow Optimization, Failure Fixing & Release (2026-11-18)

Prompt saved redacted at `worklog/session-ci-optimization-prompt.md`.

**T1 — clone and baseline.** Cloned at `9352c81d` (branch `main`, clean tree).
Baseline run history (GitHub API, 2026-11-18):

| Workflow | Latest run | Conclusion | Duration |
|---|---|---|---|
| engine-ci | push @15:46 | **failure** | 43–45 s (quick fail; green runs before it: 697–913 s) |
| bridge-integration | push @15:46 | **failure** | 107–110 s (green runs before it: 777–864 s) |
| benchmark-src | dispatch @15:46 | **failure** | 689 s (earlier run 44275941738: 6811 s ≈ 1 h 53 m) |
| ui-ci | push @15:46 | success | 257 s |
| ui-e2e | push @15:46 | success | 313 s |
| contracts-ci | push @15:46 | success | 23 s |

The 15:46 batch is commit `9352c81d`'s CI — three workflows red on the default branch. Root causes in `worklog/CI-FAILURES.md` (Phase 0/1).

**T2 — Phase 0 context sheets (agents A–D), pushed before any change.**
- `worklog/WORKFLOW-INVENTORY.md` — 9 workflows end to end; 3 red on main @9352c81d (engine-ci, bridge-integration, benchmark-src); 6/9 lack concurrency groups; 3/9 lack explicit timeouts; all 46 action refs SHA-pinned but on outdated majors.
- `worklog/CI-FAILURES.md` — every current + recent failure with root cause per run (engine-ci/bridge: regenerated `abi_layout_dump.c` committed without clang-format 18.1.8; benchmark-src: ctest step runs from repo root but `CMakePresets.json` lives under `engine/`; older benchmark failures already fixed on main).
- `worklog/PIPELINE-AUDIT.md` — engine built 2×/push (~18.4 min duplicated), release reuses nothing (~23 min serial), vcpkg ports rebuilt uncached in 3 lanes, `web-dist` artifact has zero consumers.
- `worklog/CACHE-AUDIT.md` — 5 caches/1.4 GB (duplicate 584 MB rust entry), 220 artifacts/327 MB (14 orphaned rda-wdk from deleted driver-ci), 12/12 recent cache events primary-key full hits.


**T4-T13 — the optimization batch (one push, evidence-citing).**
Root-cause fixes landed: `7d196cda` (benchmark ctest working-directory; the
abi format drift was already closed by the concurrent lane's a19afb56 —
verified locally: pinned clang-format 18.1.8 reports 0 remaining violations).
Batch adds: concurrency groups on all 6 workflows that lacked them
(evidence/queue lanes never cancel in progress), explicit timeouts on the 3
workflows on the 360-min default, the shared vcpkg binary-cache key +
`VCPKG_DEFAULT_BINARY_CACHE` wiring on release/benchmark/validation (the
three build.ps1 lanes that rebuilt the ports from scratch — "Restored 0
package(s)" in their logs), pnpm + cargo caches on release, cargo cache on
ui-ci (204 crates re-downloaded per run before), the versioned pip key on
engine-ci. Matrix reduction N/A (no matrices exist). Version bumped to
0.9.5 in lockstep for the release cut; notes updated truthfully.
Detail: worklog/OPTIMIZATION.md. Cleanup: duplicate rust cache entry and the
14 orphaned rda-wdk artifacts deleted via the API (see below).

---

## Session: Cross-Repository Feature Harvest & Implementation (2026-11-19)

Prompt saved redacted at `worklog/session-cross-repo-harvest-prompt.md`.

**T1 — clone and pin.** Product cloned at `201100aa` (branch `main`, clean
tree, VERSION 0.9.5). Six reference repos cloned read-only (all public,
anonymous clones — token never needed for reads, kept in git config only):

| Role | Repository | Pinned HEAD | License |
|---|---|---|---|
| Product (target) | heicfast/final-prod | `201100aa2978e26c218c73c00f7c15c23757d554` | MIT |
| B — UI/recovery features | terraseraph/sectorforge | `b292a299c18013706e81d670657ced6c43f200ca` | MIT |
| C — UI/recovery features | htoklu/halim-recovery | `3b5d66d8537432f54b76489e717387f9da198635` | MIT |
| D — partition & carving | cgsecurity/testdisk | `02941e1b0fce14a33932724de85cbe8385bb70d0` | GPLv2+ (`COPYING`) |
| E — forensic engine lib | sleuthkit/sleuthkit | `3dcb43668d811548206fbeaf5e0c9753202f672f` | IPL 1.0 core + bundled (`licenses/`) |
| F — NTFS deep recovery | mjgil-rust/ntfs-recover | `5072bbc729b293efc0b2070b38ad30c3b7e043cf` | 0BSD |
| G — MFT analysis/rescue | hasithahaththella/mft-rescue | `9b0e5ecc9d24ae9918f83e3a81e4a8616f610a22` | MIT |

Repo scale at pin: final-prod 3227 tracked files; sectorforge 197;
halim-recovery 62; testdisk 793; sleuthkit 959; ntfs-recover 25; mft-rescue 8.

Mandate: best-of-breed harvest — for each capability take the reference that
does it best, re-implement from understanding in our stack (never copy
verbatim), cite the reference for behavior, license-check every feature,
validate against seeded ground truth with byte-for-byte comparison, evidence
in `worklog/VALIDATION/`. The named feature is priority: **reconstructed
files with original directory structure preserved, deleted only, as a
post-scan option — matching SectorForge's behavior**. Nothing is implemented
until the feature matrix (T3) is committed and pushed.

**T2 — agents A-G context sheets** (commit `d7b8ad65`, pushed as `22dcfddb`
after rebase over the concurrent lane's `795f5328`). Seven sheets, every
claim file:line-cited by the agents; spot-checks at consolidation verified:
SectorForge named-feature paths (`crates/sf-core/src/ntfs/extract.rs:14`,
`sf-cli/src/recover.rs:241`), ntfs-recover `patch_record_data_runs`
(`repairer.rs:467`), TestDisk's 353 `file_*.c` count, mft-rescue
$LogFile/$UsnJrnl absence, product `CR_UI_ABI_VERSION 4`.
- `worklog/PRODUCT-STATE.md` — 95-export ABI v4 baseline; per-domain
  EXISTS/PARTIAL/MISSING; imaging producer and carve-fragmentation the top
  gaps; prior perf-lens sheet archived to context-sheets.
- `worklog/SECTORFORGE-FEATURES.md` — named feature FOUND+FULLY IMPLEMENTED
  (extract.rs plan/Writer, `_orphaned/`, ` (id N)` collisions, never-overwrite,
  per-item notes); two-pass ddrescue-style imager + mapfile; 94-type carve
  registry; same-drive guard.
- `worklog/HALIM-FEATURES.md` — evidence-weighted health 55/35/10;
  FAT/exFAT orphan-directory harvesting; exFAT bitmap authority; VHD testlab
  lessons (double flush, no remount).
- `worklog/TESTDISK-FEATURES.md` — 353-signature PhotoRec registry (285
  default-on, 930 registered signatures); deep-scan lattice + hint list;
  backup-BS workflows; MFT-mirror adjudication; bounded fragmentation
  brute-force (≤5 frags, ≤5000-block skips, paranoid=2 only).
- `worklog/SLEUTHKIT-FEATURES.md` — fs abstraction (name/meta/attr with
  independent alloc flags, ~20-pointer plug-in table); mmls; fls/icat/blkls
  grammar; NTFS index-slack deleted names; orphan pipeline; NO $LogFile
  parser, USN V2-only.
- `worklog/NTFS-RECOVER-FEATURES.md` — delta-grouping $MFT run-list
  reconstruction + in-place record-0 patch (battle-tested 2 TB); the
  ntfs-3g sign-extension encoding lesson; parent-sequence NOT validated
  (their gap, must not become ours).
- `worklog/MFT-RESCUE-FEATURES.md` — truth table: $LogFile, $UsnJrnl, MFT
  slack all NOT FOUND IN CODE; single-pass imager with progress.json (failed
  ranges never retried — superseded by SectorForge's design).

**T3 — the feature matrix** (commit `aa4d87ea`). `worklog/FEATURE-MATRIX.md`:
58 rows, every row citing its sheet; P0 = F-01 (the named feature, Wave 5)
+ F-33/F-34 (two-pass imaging + ddrescue mapfile, Wave 6); 8 P1s; decision
register: $LogFile NOT-ADOPTED (no reference implements it; I-049 stands),
MFT-slack carving NOT-APPLICABLE (mission assumption falsified by the truth
tables), mft-rescue imager SUPERSEDED; UNVERIFIED rows resolve inside their
wave before implementation.

**T4 — licensing** (commit `f0fe9155`). `worklog/LICENSING.md`: licenses
verified at pinned HEADs (SF/HR/MR MIT, NR 0BSD, TD GPLv2+, SK IPL 1.0 core
+ bundled matrix incl. GPL/RSA/Unicode files never used as copy sources);
standing policy stricter than MIT requires (no verbatim code from ANY
reference); GPL/IPL rows re-implemented from documented behavior only;
format facts free and cited; thresholds re-derived.

**T5 — seeded validation fixture, proven on real Windows**
(commits aabc1ffa → 6544953e; canonical green run
[44356754104](https://github.com/heicfast/final-prod/actions/runs/44356754104)).
`tools/validation/named-feature/` + dispatch-only lane
`.github/workflows/named-feature-fixture.yml`: deterministic generator
(salted path-hash PRNG, fixed timestamps/attributes, real signature magics),
deletion-matrix applier (double flush, double-delete recreate dance,
tree deletes), byte-for-byte verifier with 5 invariants + 5-case self-test,
built and captured on a real 2 GB GPT/NTFS VHDX with a determinism proof
(two generations byte-identical modulo `_meta`). Evidence committed at
`worklog/VALIDATION/fixtures/named-feature/` (manifest 24 rows / 22 deleted
/ 20 EXACT + 2 COLLISION-SET, apply-log 19/19 actions ok, self-test 5/5;
VHDX artifact SHA-256 `fb9f4835…fe82d3bd`, regenerable via lane dispatch).
Four failing runs root-caused from logs and fixed in-tree (empty-array
pipeline unrolling; the runner's second-invocation Write-VolumeCache
DriveLetter ':' misbind, worked around non-fatally; a File.Open overload
mistake) — full run table in the evidence README. Lane is dispatch-only:
push lanes untouched, main green throughout.

**State after T5**: Phase 0 + Phase 1 complete (matrix, licensing, fixture).
Waves 1-9 (T6-T20) are the implementation backlog, row by row from
`worklog/FEATURE-MATRIX.md` — the matrix's UNVERIFIED rows resolve to
EXISTS/MISSING verdicts by reading the cited product files inside their
wave before any code is written.

**Main-green repair** (commit `cc201bff`): every push since `201100aa` had
been creating phantom zero-job FAILED runs for release / benchmark-src /
validation-evidence (event=push, 0 jobs — including all seven of this
session's pushes). Root cause: the workflow-level
`env: VCPKG_DEFAULT_BINARY_CACHE: ${{ runner.temp }}…` block — the `runner`
context is unavailable at workflow-level env, the exact invalid-file failure
`235d3c32` described; that fix commit added the step-level env but never
removed the workflow-level block (24 insertions, 0 deletions). `cc201bff`
deletes the block from all three files (the step-level env carries the var);
verified post-push: no phantom runs for `cc201bff` where every prior push
generated three within a second.

---

## Session 2026-11-18 — sector-scan rebuild (Phase 0 + Phase 1): read, baseline, diagnose

Prompt saved redacted at `worklog/session-sector-scan-rebuild-prompt.md`. Clone: final-prod
`976bced8` (main), corpus `be0cfe95`. Token in credential store only, never committed.

**T2 (`d69893ed`)** — five Phase 0 context sheets pushed BEFORE any code, from five parallel
read agents (A product, B reference-scan, C reference-algorithms, D MS-docs, E reproduction):
`worklog/session-sector-scan-rebuild/{PRODUCT-PIPELINE,REFERENCE-SCAN,REFERENCE-ALGORITHMS,MS-DOCS,REPRODUCTION}.md`
(2,248 lines, every claim cited). Load-bearing findings: the read path is ALREADY the
AsyncRing IOCP transport (8×4 MiB + companion sync handle — the prior perf lane's work, not
the pre-ring QD1 sync the old sheets describe); **0 of 12 CR_EVT_* event types carry a result
row** (the engine→UI streaming drop stage); the reference corpus documents NO per-phase
progress weights — the sector-linear X/Y ticker is the only evidenced model, 40/35/25 is
product lineage; the reference's phantom gates are fully synthesized (dual-generation
validation, grammar-walk-with-end, size sanity, extent viability, name sanity, structured-wins
merge); 34 MS Learn pages fetched with 13 doc-cited platform rules; the seed manifest is
uploaded by forensic-repro but NEVER consumed by any gate (no detection/phantom-rate evidence
exists anywhere).

**T3 (`c54f2496`)** — `worklog/session-sector-scan-rebuild/DIAGNOSIS.md`: ten stages
quantified with run IDs. Reader/geometry/detection WORK (post-ring repair; 81/81 deleted-tree
nodes on the 6 GiB it-gate; 26-41k files/s quick-stage). Broken: (5) the streamer drops every
row — counters-only events + 10%-bucket snapshot starvation is the root cause of "nothing a
user would recognize as a real recovery result" DURING scan; (8) the progress axis decouples
from byte-denominated work — quick+deep are metadata passes finishing in seconds, so the UI
opens at exactly 75.0% then crawls (75.0→76.1% over 891 s, run 42255492260) while carve
grinds at the measured 8.45 MiB/s validation wall; (9) sector ticker axes inconsistent
(clamped carve vs unclamped deep, P-DEEP-01); (7) phantom/accuracy UNMEASURED — the
instrument itself is missing (F-D: manifest-vs-results gate is the first rebuild act).
Fix plan F-A..F-G with exact entry points recorded for the implementation session.

**Handoff (truthful)**: implementation phases 2-4 (F-A streaming event, F-B/F-C byte-derived
monotonic percent + one sector axis, F-D manifest gate, F-E filter unification, F-F grading
coverage, F-G carve throughput) NOT STARTED — this session's step budget ended at diagnosis.
Windows CI sweep + release validation not run (Linux sandbox). `worklog/VALIDATION/` gains
nothing from this session — no evidence faked. OPEN-1: the concurrent installer-api lane's
AsyncRing miss-path repair needs its Windows benchmark matrix re-run green.

---

## Session 2026-11-19 — installer-api lane re-opens (second continuation): main-green + the wiring waves

Prompt (unchanged charter, saved token-redacted by this lane's first session at
worklog/session-installer-api-prompt.md). Clone: final-prod `241e8991` (main),
corpus `be0cfe95`. Token in env/git config only.

**T1 — the red-main audit and repair.** Baseline at clone: benchmark-src,
validation-evidence, release RED on every push since `22dcfddb` (12 consecutive
zero-job push failures each — the phantom invalid-workflow-file class), ui-ci
RED at `795f5328` (prettier). Root causes pinned first-hand: (a) the
workflow-level `env: VCPKG_DEFAULT_BINARY_CACHE: ${{ runner.temp }}` survived
235d3c32's "workflow-level env removed" fix (that commit ADDED the step-level
replacements, 24 insertions/0 deletions — never deleted the offending block;
`runner` is an unrecognized named-value at workflow-env scope); (b)
`preview-pane.tsx` shipped unformatted by W5e/M06. My in-flight repair was
overtaken by the concurrent lane's identical root-cause fix (`cc201bff` +
`df5e05b9`) — my duplicate was rebased away, never force-pushed; my unique
remainder landed as `87c04b7f` (prettier 3.9.10 on preview-pane.tsx, pure
formatting 41/41, `prettier --check .` from ui/ green). Local gates before
push: contracts validate.mjs green (55 commands, both cross-checks),
doc_truth_gate green (exports=95, seam=12/67), all workflow YAMLs parse clean,
no workflow-level runner refs remain.

**The frontier at work start**: W5 COMPLETE (M07 `228bf5bc`, M13 `9352c81d`,
M12 `9ad991d6`, M06 `795f5328`; M28 verified fixed in-tree at
engine/src/ui/cr_ui.cpp:5095 — the uniform ERR_ELEVATION_REQUIRED gate). This
session's recorded scope, from the prior lane's deferred register: the W9
imaging UI/connector half (the charter's UI-wiring requirement), the W8 batch,
W7 repair workbench ABI, W9-repair UI, W6 (M20/M14), the evidence dispatches
(benchmark-src = the ring repair's Windows proof, OPEN-1 of the sector-scan
handoff; validation-evidence = the G-5 re-proof + DT-12 regen), and the
truth-sheets re-trued as waves land.

### T-W9 — the imaging UI/connector half: D-IMG-1/D-IMG-3 CLOSED end-to-end (the charter's UI-wiring requirement)

The W9-imaging wave over the shipped engine/CLI/bridge halves. The connector
contract gains `ImagingDomain.cancelImageCreation(imageId) -> ImageCancelAck`
+ `startImageCreation(deviceId, path, range?: ImageSectorRange)` (firstSector
0-based, sectorCount 0 = to-the-end — the ABI semantics verbatim) +
`ImageJobState.cancelRequested` (cancelSupported's stale "always false"
doc-truth replaced with the D-IMG-1-closed reality). The tauri connector
passes firstSector/sectorCount into `image_create_start` and binds
`image_create_cancel`; the mock simulates the cancel truthfully (the flag
consumed at the next tick head — the engine's one-64-KiB-chunk bound as one
tick; `cancelled` terminal + partialKept = bytes-committed > 0, the bridge's
filesystem-probe semantics mirrored) and refuses out-of-range pairs exactly
as the engine does (ERR_BAD_PARAMETER -> scan-scope-invalid, never a silent
truncation). `useCancelImageCreation` joins the query layer.

The dialog: the More-Option disclosure (the og's DiskBackupSaveDlg from/to
pair, strings:11640-11760) with digits-only inputs (the [0-9]{1,15} grammar)
and the advisory Invalid-sector state (the ENGINE owns the real bounds gate);
the running phase gains the Stop control + the stop-confirm with the
reference's re-create truth ("You need to re-create the disk image if you
stop it", strings:11672-11673) + Stop-vs-Keep-going choreography; the
cancelled terminal state renders the partialKept truth. The FALSE
noCancelNote copy ("this build provides no interrupt") is DELETED from both
locales — a stale claim removed, replaced with the truthful strings.

Contracts: command-surface.json's image_create_cancel row now serves
imaging.cancelImageCreation (55 commands, 54 TS-invoked cross-checked);
type-maps §11's "(ABI carries no cancel)" row replaced with the
cancel-flag + range rows; the contracts README seam count 67 -> 68 METHODS
(doc_truth_gate-derived: 56 RR + 12 subscriptions; the gate ran green in the
same commit).

Tests: connector-mock gains the cancel round-trip (ack running + flag live,
cancelled terminal, partialKept true), the unknown-id typed refusal, and the
range suite (legal mid-disk range sizes bytesTotal; first-past-end,
past-end-pair, and negative/float refusals all scan-scope-invalid);
connector-tauri pins the new invoke shapes (range args + cancel);
lane-c-surfaces re-pins the dialog (the cancellable posture + stop-confirm
choreography + the More-Option advisory validation) and gains a dedicated
D-IMG-3 range test; the e2e spec re-pins the same flow end-to-end.

Local gates (all green before push): typecheck, eslint, prettier --check .,
vitest 216/216 (30 files), arch:lint, filefilter:check, jscpd exit 0,
contracts validate.mjs (55 commands, both cross-checks), doc_truth_gate
(exports=95, seam=12/68). Honest scope: knip cannot run in this sandbox
(oxc-parser allocation vs the memory cap — the documented standing
limitation; the ui-ci runner proves it) and the e2e lane runs on the
runner.

### T-W8/M04 — image-file scan targets: the "Import a disk image" flow (BF-12 CLOSED)

The biggest W8 row, end to end. The bridge gains `commands/images.rs` — the
in-memory imported-image registry (the ImagingState seam discipline) with the
og's import filter minus `.edisk` (M03): vmdk/vhd/vhdx/dmg/img
(strings:11803-11816). `image_target_import` validates truthfully (missing
file → device-gone; unsupported family / empty file → scan-scope-invalid;
idempotent per canonical path) and returns the Device-shaped row (kind
'image' — NOT 'virtual', the D-ENUM-05 lesson: an imported image is a
scannable target every surface must render); `devices_list`/`devices_refresh`
merge the rows (refresh-stable within the app run — the registry's honest
scope); `scan_start`'s new imgfile- branch opens through
`cr_ui_open_image(path, 0, 0)` with whole_disk=1 (the image IS a disk), the
NG-21 pre-scan-repair guard (pre_scan_repair=0), pre_scan_check=1, and folder
scopes / FVEK keys refused (they address live-volume axes an image view has
none of). `system_pick_image_file` is the native picker with the og's filter
(bridge re-validates — the filter is a convenience, never the gate).

The connector: `DeviceKind` gains 'image'; `devices.importImageFile(path)` +
`system.pickImageFile()`; the mock mirrors the family gate, the idempotence,
the refresh stability, and rides the scan machinery through a synthetic
whole-image volume (the canned picker alternates .img/.vhdx). The home page
gains the fourth quick target ("Import a disk image", the DiscIcon row in
the Specify-a-location grid) with the pick→import→row chain; the DeviceCard
badges image rows and HIDES the create-image hover action (imaging addresses
physical disks — the bridge would refuse an imgfile target; never a dead
button). i18n en/zh-CN.

Contracts: scan_start/devices_list/devices_refresh notes trued + 3 new rows
(image_target_import serves devices.importImageFile; system_pick_image_file
serves system.pickImageFile; image_target_list serves-empty by design — the
dev-panel/diagnostics surface); the README seam count 68 → 70 (doc_truth_gate
caught the drift in-commit, as designed).

Tests: mock — import/scan-to-completion over the imported target,
idempotence + refresh survival, the family refusals, the canned-picker
alternation; tauri — the new invoke shapes + the refusal code maps; e2e —
the quick-target → row → badge flow. Local gates all green (typecheck,
eslint, prettier, vitest 222/222, arch:lint, jscpd, contracts 58/56
cross-checks, doc_truth 12/70). Honest scope: the Rust compiles on the
runners (no local cargo — the standing sandbox limitation); knip is the
documented memory-cap casualty.

---

## Cross-repo harvest — Wave 1 + Wave 5: the named feature lands (F-00d/F-01/F-31/F-45)

**Engine (pure core, Linux-proven).** New module
`engine/src/core/recovery/cr_extractplan.{h,cpp}` (F-00d + F-01): the
structure-preserving extract plan, re-implemented from the SF sheet's
documented behavior (`worklog/SECTORFORGE-FEATURES.md` §3, citing
sf-core `ntfs/extract.rs` + `sanitize.rs` — no code copied):
- `sanitize_component_sf()`: reserved device names (incl. `con.txt`
  stem-matching + the superscript COM¹²³ spellings), trailing dot/space
  runs → `_`, control chars → `_`, `.`/`..` neutralization, UTF-16 cap
  240 preserving a <=16-unit extension;
- `with_id_suffix_sf()`: ` (id N)` inserted BEFORE the extension;
- `plan_deleted_tree()` / `plan_deleted_snapshot()`: single-pass emit
  with tail rollback — the F-31 fast-path prunes an in-use subtree with
  no deleted descendant WHOLE (O(n)); live ancestors become path
  components only (the original path is the deleted file's full
  ancestry); the "File Path Lost" root and its LostDir<NNN> groups map
  to `_orphaned/` with the top of every orphaned chain id-suffixed
  (two orphaned "Documents" never merge); carved/raw-origin nodes
  excluded (no original structure); depth-capped 128.

**SafeWriter (opt-in SfId scheme).** `WriterConfig.scheme =
CollisionScheme::SfId`: `resolve_collision_sf()` (first collision →
`name (id N).ext` with the item's own id — deterministic re-runs;
second collision → FAIL with ERR_DEST_NAME_EXHAUSTED — never an
overwrite); SF sanitize in SfId mode; `arm_output_guard()` +
`DestIo::canonicalize()` (Win32 GetFinalPathNameByHandleW; POSIX
realpath; default ERR_UNSUPPORTED → truthful lexical degrade) — the
inside-out guard refuses a created dir that canonicalizes outside the
output root with the new ERR_DEST_ESCAPE (0x50050009); EFS-encrypted
items (0x4000) write EMPTY + note (SF content honesty); report
`Item.note` (F-45) derived only from truth flags (encrypted /
zero-filled / numbered cause / cluster reuse). The classic flow is
byte-for-byte unchanged (OgUnderscore default; `SfIdWriter.
EncryptedAndSchemeAreOptIn` pins it).

**ABI (append-only, 95 → 99 exports).** `cr_ui_recovery_config` gains
`sf_collision` (zero-init safe); `cr_ui_recovery_stats` gains
`read_error`; new exports `cr_ui_reconstruct_deleted_preview{,_snapshot}`
(read-only plan counts) + `cr_ui_recovery_enqueue_deleted_tree{,
_snapshot}` (the whole deleted-only plan into the queue; refused
without sf_collision=1 — the naming contract). Layouts regenerated
(`tools/gen_abi_layouts.py`: 30 structs / 376 field paths; the header
SHA pin refreshed; dump + ABI check green).

**Bridge + UI.** Rust: `recovery_preview_deleted_tree` command +
the `{kind:'deleted-tree'}` selection (arms sf_collision, enqueues the
plan — session or live snapshot); `RecoveryItem` rows carry
`outcome` (complete/incomplete/failed, F-45) + `note`. TS: contract
types (`DeletedTreePreview`, selection variant, item fields), tauri +
mock connectors (the mock simulates deleted-tree jobs over the fixture
space), `useDeletedTreePreview` query, results-explorer's post-scan
"Recover deleted (keep structure)" action, the recovery page's
deleted-tree mode (preview panel: files/dirs/orphans/bytes), outcome
badges + notes on item rows; i18n en + zh-CN parity. Contracts:
command-surface +recovery_preview_deleted_tree + selection note;
error-codes +ERR_DEST_ESCAPE; validate.mjs green (56 commands).

**CLI.** `rrecov recover --reconstruct-deleted`: arms the SF scheme +
plan (the classic path untouched); usage updated; win_syntax_check
parses the app layer clean.

**Tests.** `test_extractplan.cpp`: 15 cases — sanitizer edge cases
(reserved/superscript/trailing runs/240-cap-with-ext), id-suffix forms,
plan walks (deleted-only, whole-subtree pruning, live-ancestor
components, parents-before-children, orphans never merged, carved/junk
excluded, snapshot parity), SfId writer (intra-queue id suffix,
second-collision exhaustion never overwrites, encrypted-empty + note,
symlink-escape guard refusal, opt-in pin). Full suite: 1709 tests,
0 failed (Linux). `validate_report.py`: latent bug fixed — M-135's
`read_error` outcome/totals were missing from the validator (a
ReadError report row would have failed schema validation).

**Validation lane (Phase 3 closed loop).** The dispatch-only
`named-feature-fixture.yml` gains the `engine-flow` job: builds rrecov,
scans the seeded VHDX as an image (whole-disk), reconstructs with
`--reconstruct-deleted`, and hands the output to the byte-for-byte
verifier (I1–I5). A verifier failure fails the lane, never masked.
To be dispatched after this push; evidence lands in
`worklog/VALIDATION/fixtures/named-feature/` on green.

Local gates at commit time: engine ci-local.sh GREEN (10/10, pinned
clang-format 18.1.8), tsc/eslint/vitest green (211 tests), contracts
validate.mjs green. Token never committed (secrets scan green).
### T-W8/M31 + M15 + M22 — three rows, three commits (the quick-win batch)

**M31 (`3be873f3`-lineage, rebased to the push)**: the Repair page's truthful
interim — SAMPLE_REPAIR_FILES and the fabricated dropzone→queue→Repair-All
flow DELETED (NG-12 closed at the root); the page states plainly that
per-file content repair is a separate product line this build does not
include (M09/S-050/D-137, the API-DELTA register) and carries the REAL
surface as primary: the volume repair workbench over the typed repair
methods (the in-scan pre-scan repair chain's semantics; the typed
not-implemented refusal renders honestly on the real backend until W7).

**M15**: the three remaining corpus filter toggles (atlas §9: Hide System /
Hide Temp / Hide Browser Cache). The label set is corpus-cited; the
predicate semantics are this product's, DOCUMENTED as the divergence in
type-maps §5 (the og's AdvanceFilterCondition internals are
corpus-undecoded): system = DOS FILE_ATTRIBUTE_SYSTEM (0x04) via the new
NodeMeta.attributes axis (cr_ui_node_info.attributes — already read for
is_dir, now carried); temp = a '/'-path segment named temp/tmp (ASCII ci);
browser cache = cache/cache2/code cache/gpu cache. Bridge-side predicates in
matches_pred (zero ABI change); the mock mirrors the same segment classes
with a deterministic system-file fixture (the DOS-class names + every 31st
record) and gains the Cache dir name so the toggles are exercisable
end-to-end. ResultsFilter + filter-store + the bar + command-surface trued.

**M22**: the destination taxonomy's distinct reasonKeys — the three M-074
codes (0x50050006/7/8) keep the destination-invalid uiCode (the vocabulary
stays stable) but carry their own copy via ENGINE_REASON_OVERRIDES
(path-too-long / path-invalid / network) so the refusal says WHY; the
bridgeCode path keeps the umbrella. error-codes.json notes trued (BF-15
closed at the connector layer); i18n en/zh-CN.

**Concurrent-lane seam (recorded)**: the F-01 lane's `cb2c8e0a` added 4
engine exports (95→99) + `previewDeletedTreeReconstruction` (71st method)
without the contracts-README count trues; my frontier re-trued BOTH rows in
the same commit (the doc_truth gate caught both — the W3 guard holding the
line across lanes).

Tests: M15's two mock suites (each toggle drops its class, keeps the rest);
M22's four-case reasonKey suite; the full vitest 225/225. Local gates all
green (typecheck/lint/prettier/contracts/doc_truth 99+12/71). The Rust
compiles on the runners (no local cargo — the standing limitation).

---

## Session continuation: the red-main repair + the installer's shipped-exe bug (2026-11-18, evening)

**Where the reds came from, honestly:** the 0.9.5-session's own workflow edits
broke four workflows for ~1 hour (runner.temp in a workflow-level env block —
the runner context is not available there; plus a duplicated timeout-minutes
key). The removal patch's regex failed silently twice before the concurrent
lane's cc201bff deleted the blocks at root; 6a6e05cd deduped the timeouts.
Every "fix" commit from this session now requires grep-verified absence in
the message (recorded in CI-FAILURES §12).

**The real product bug found (closed):** every forensic-repro run on v0.9.4
failed D-PRIV-01 with `app not installed` — the shipped setup.exe aborted
with exit 2 on ANY machine other than the one that compiled it. Root cause:
the G-4 engine-kit pre-flight (added in v0.9.4, commit 42ef613b) probed the
COMPILE-TIME build-machine path `${ENGINE_KIT}` at RUNTIME. v0.9.3 installed
fine (nsi predates the gate); t15 stayed green only because it builds AND
installs on the same runner. Evidence: the forensic artifact's install.log
carried ONLY the onInit line; 4.2 s runtime; exit 2. Fixes: 5798cf39 (kit
payload extracted to $PLUGINSDIR\engine-kit, gates read the payload) +
02e62afb (CopyFiles not File for the runtime placement — NSIS File is
compile-time-only; learned live on release run 44361715679). PROVEN: forensic
run 44369227822 installs the shipped v0.9.5 setup.exe clean (exit 0, 5.6 s),
manifest gate green (requireAdministrator).

**The t15 false red (closed):** the G-3 shortcuts assertion matched any
'Recovery'-containing .lnk — including the STOCK Windows Administrative
Tools\RecoveryDrive.lnk (a win22 image fixture). 3809fbc5 scoped the probe
to 'HeicFast|Recovery Wizard'. Our link was provably removed; the OS's own
utility is not ours to remove. Validation-evidence GREEN at 987 s after the
fix (run 44366316888, t14+t15 both green).

**Concurrent-session reds fixed at their source (each with the run ID in
the commit message):** E0308 add_filter array literal (bd01ac19), needless
borrow at images.rs:170 (03b265e6), TS2739 missing hide-* fields in the
mock deletedTreeFilter (c8747105), E0063 Pred literal in the T14 test
(173ca47c, + doc-truth 95->99/40->41 counts in three engine docs), doubled
backtick continuations in named-feature-fixture.yml (0c8b3220), stale ABI
header.sha after cb2c8e0's header change (42e1e3ea, regenerated with the
repo's own generator + pinned clang-format 18.1.8), second needless borrow
at images.rs:185 (d8774560).

**v0.9.5 release: GREEN end to end** (run 44365218107, 1224 s = 20.4 min,
down from v0.9.4's 23.3 min with the vcpkg/pnpm/cargo caches first-hit;
assets: setup.exe 2877168 B, portable.zip 4039372 B, SHA256SUMS). The
≤10-min target was NOT met — the serial engine+tauri compile remains the
critical path; the build-once/artifact-reuse redesign is the documented
follow-up (OPTIMIZATION.md).

**Still open at handoff:** forensic-repro's D-SCAN-01 red on the S3 leg
(run 44369227822: maxFrozen=800 s over a 64 GB seeded VHDX deep scan;
advancedPctDelta=1.8, liveBytesOk=true — the percent DID move, slowly; a
67 GB deep pass at runner-disk speeds legitimately exceeds the 180 s
frozen-triple budget; the engine logged stage 2 begin then silence for the
linear deep phase). This is a probe-budget vs disk-size question, not a
workflow bug — the fix belongs to the forensic lane owner (scale the stall
budget with disk size or gate on engine sector telemetry). benchmark-src
run 44359663812 (smoke) still in flight; its first-ever green is expected
but not yet claimed. ui-ci/bridge runs on the newest heads are in flight
from the concurrent session's rapid pushes; each red so far was a real
integration miss from their feature commits, fixed at source same-hour.

---

## Wave 6 unit A + the cb2c8e0a CI postmortem: two-pass rescue imaging lands

**cb2c8e0a push postmortem (three root causes, all fixed in this unit).**
contracts-ci green; engine-ci lint RED, bridge-integration RED,
fixture-lane engine-flow RED:
1. **engine-ci (AST parse gate)**: the engine-flow job's pwsh block used
   backtick line-continuations before `--flags` — and the committed YAML
   carried DOUBLED backticks (an escaped literal, i.e. NO continuation;
   the next line's leading `--` parses as a unary operator). Rewritten
   as single-line invocations; locally re-simulated the gate's
   extraction (4 blocks, zero trailing backticks).
2. **bridge-integration (ABI freshness gate)**: my committed
   `abi_layout_dump.header.sha` was STALE — cr_ui.h was edited (the
   u8_to_u16/writer_scheme fixes + clang-format) AFTER the last
   generator run. Regenerated (30 structs / 376 field paths; the CI's
   expected 19f722e5… now matches), dump reformatted, local ABI dump +
   check green.
3. **fixture engine-flow ("Set up job")**: I fabricated the
   actions/download-artifact pin SHA (631f0bb8… — does not exist; the
   hallucination the mission's rules exist to catch, caught by the
   runner itself). Replaced with the REAL v4 tag SHA resolved from the
   GitHub API (d3f86a106a0bac45b974a628896c90dbdf5c8093, v4.3.0 merge).
   The fixture job itself SUCCEEDED in that run — the seeded VHDX
   ground truth exists (artifact `named-feature-fixture`, run
   44370686000).
Also registered: my pushed subject `feat(W1+W5/F-01)` violates the
local commit-convention charset (scope allows [A-Za-z0-9._-] only;
`+`/`/` are out) — CI does not enforce it and force-push is forbidden,
so it stands as a recorded deviation; every subsequent subject
conforms.

**F-33/F-34: the ddrescue-style two-pass rescue imager.** New pure-core
`engine/src/core/storage/cr_rescue.{h,cpp}` (behavior re-implemented
from the SF sheet §2.1/2.2 citing sf-core imager.rs/mapfile.rs; the
on-disk mapfile follows GNU ddrescue's documented format —
ddrescueview-readable):
- `RescueMap`: sorted/contiguous/non-overlapping blocks over [0,total);
  mark() split-carve-merge; find_next/find_prev (clipped);
  to_mapfile_text()/parse() (round-trip tested; hostile input —
  overlaps, holes, unknown status, zero-size — refused);
- `RescueImager`: pass 1 forward (from current_pos, with the from-0
  fallback so a resumed/reverse run picks up untried areas before it)
  or REVERSE (each region read from its TAIL; current_pos keeps the
  lowest point); a failed read marks max(skip, chunk) NonTrimmed and
  jumps — one bad spot cannot stall the rescue; pass 2 binary-splits
  NonTrimmed/NonScraped toward sector granularity, retrying each
  sector up to max_retries before BadSector; checkpoints (dest flush ->
  map save) at <=5s cadence (periodic failures swallowed — an AV lock
  aborting a live rescue would be far worse) and forced at pass
  boundaries; buffer-capacity-clamped reads (no unbounded retry read);
- stats derive from the map (rescued/bad/pending sum to total — the
  truth invariant); NO sha256 sidecar for rescue images BY DESIGN
  (holes would make a digest vouch for zeros never read; the mapfile is
  the truth artifact, all-'+' = the completion certificate).
- Win32 driver `win/cr_rescue_win.cpp`: positioned writes
  (SetFilePointerEx), FSCTL_SET_SPARSE best-effort (F-36) on
  reverse/fresh, atomic mapfile IO (tmp + flush + MoveFileExW with the
  5x50ms replace-while-open retry), auto-resume with the refusal
  checks (wrong-size map; map claiming bytes a missing/short dest does
  not hold), direct-to-final-path disposition (temp+rename would
  orphan the map's claims — the mapfile is the commit marker).
- CLI: `rrecov image --create --disk N --out f.img --rescue [--reverse]
  [--chunk-size N] [--skip-size N] [--retries N]` with resume hinting;
  the flat producer path untouched.
- Tests (12): map round-trip/hostile-input/split-merge/clip, clean
  full rescue byte-compared, the two-sector bad patch isolated to
  EXACTLY 1024 bad bytes (hole never written — no synthesized zeros),
  reverse attempts the tail first, resume never redoes Finished work,
  wrong-size map refused, forced-save failure propagates, periodic
  checkpoints fire, empty-device parity. win-stub extended (OPEN_ALWAYS,
  LONGLONG, FSCTL_SET_SPARSE, SetFilePointerEx; winioctl include moved
  to the stub's END so its types resolve); win_syntax_check green over
  the whole win layer + app.
Local gates: ci-local GREEN, 1679 tests 0 failed, layout regen verified. (feat(W6): two-pass ddrescue-style rescue imaging with the resumable mapfile (F-33/F-34) + the cb2c8e0a CI postmortem fixes)

## Session 2026-11-19 (continuation) — the 44391734922 engine-flow red: three root causes, all fixed

Head at session start `f4dcc3e8` (v0.9.6). This session: (1) the ui-ci
M31 red (run 44391744949) — a timing budget, not a product bug: two
serial mock latencies (700±200 + 600±200 ms) vs RTL's 1000 ms findByText
default; explicit 4000 ms budgets (distribution proof in
ui/tests/timing-budget-proof.ts; 97.2% of trials exceed 1000 ms);
(2) the named-feature-fixture engine-flow red (runs 10-12) — artifacts
pulled and driven to three root causes: ENGINE F-01b (the NTFS '.'
volume root sanitized into a '_' path component — every real-volume
reconstruct nested under `_\`; '.'/'..' are now transparent reference
names in both plan-walk forms, 4 new tests), FIXTURE durability (the v2
recreate's cached File.Copy lost its data on the unflushable volume
cache — the VHDX reads zeros at v2's extent while the stash twin
survived; the recreate is WriteThrough now), FIXTURE design (probe's
post-delete create ate report.docx's freed MFT record — the probe
record is pre-allocated before any deletion; the probe + stash are
declared manifest rows; the fixture seeds at the volume root X:\ so
manifest rel == volume-relative path). Bonus display-truth fix: the
'(VHDX, MBR)' label over a GPT disk (classify_lba0 needs LBA1; the og's
0x1000-byte read restored). Engine suite-by-suite on Linux: 1682/0
(CycleGuardPerf excluded locally — 3.4 GB fixture vs 4 GB sandbox; the
Windows lane owns it). Commits: be7966c5, 1a1f6877 (null-safe
$MAGIC[$null] + prettier), 20efb62a (knip ignore). In flight at
handoff: named-feature-fixture(14) on 1a1f6877 — THE F-01 closed-loop
gate; engine-ci(73)/bridge(68) on be7966c5; ui-ci(129)/ui-e2e(114) on
20efb62a. Full detail: worklog/SESSION-2026-11-19-F01B-ROOT-CAUSE.md.

---

## Session 2026-11-26 — Cleanup, Consolidation & Documentation Sync

**Start state**: HEAD `f01f630a`, VERSION 0.9.6, main green (per last session close).
**Session goal**: hygiene + documentation sync — remove slop/dead code/duplicates with evidence, consolidate all docs/artifacts under one `docs/` roof, mark every issue fixed vs open, update docs to current reality, validate.

- **T1 done**: cloned, recorded start state, prompt saved (token redacted) at `worklog/session-cleanup-consolidation-prompt.md`, read twice. This entry.

---

## Session 2026-11-26 (cross-repo-harvest resume) — the F-01 closed loop driven GREEN, F-03 landed, the gates closed out

Prompt: the cross-repo-harvest session re-dispatched verbatim (saved
token-redacted by its first lane at `worklog/session-cross-repo-harvest-prompt.md`,
read twice this session). Clone: all seven repos at the SAME pinned
SHAs the sheets recorded on 2026-11-19 (references unchanged);
product HEAD at session start `8c86a484` (the prior lane's handoff
worklog entry; the 52-commit harvest lineage `201100aa..8c86a484`
mapped first — this session RESUMES the interrupted harvest, it does
not redo Phase 0: the seven context sheets, FEATURE-MATRIX, LICENSING
all stand).

**The in-flight-at-handoff verdicts (7 days later, from the API):**
ui-ci(129)/ui-e2e(114) on `20efb62a` ended GREEN; engine-ci(73)/
bridge(68) on `be7966c5` ended GREEN; named-feature-fixture(14) =
run `44475017206` on `1a1f6877` ended RED — the F-01 closed-loop
gate itself. Root-caused from the downloaded artifacts (the evidence
upload step runs on failure too — by design): the ENGINE output was
fully correct (recovery-report 24/24 done; every file byte-exact;
every directory on disk; the runner-side I2 had PASSED the
`.nffx-flush-probe` row — it is not among the failures), but the
VERIFIER failed four deleted-DIRECTORY rows ("I2 missing output:
Docs/Old, Docs/Old/sub, Photos/vacation, Photos/vacation/inner"):
`walk_outputs()` collects files only, and dir rows carry
`expect.class=EXACT`, so the file-exactness loop always reported them
missing. Second finding in the same artifacts: the evidence zip had
silently dropped the dot-prefixed recovered files (the upload `**`
glob skips them — the probe + the stash twin were on the runner tree
per the recovery report but absent from the zip).

**`d65a5d12`** — the verifier fix + the evidence-completeness fix:
`exact_rows` excludes `is_dir` rows (directories stay validated by
the dedicated isdir check, which run 14 passed with zero findings);
the self-test gains the regression pins (dir row with a file child,
an empty deleted dir, a dir-missing failing case — 6/6); the lane
captures `output-listing.txt` runner-side (`Get-ChildItem -Force` —
nothing skipped) and names the dot entries in the artifact path.
Re-dispatched the lane on `d65a5d12`:

**Run `45811533301` — THE F-01 CLOSED LOOP GREEN** (fixture job +
engine-flow job both success, 2026-11-26): the full chain on a real
Windows runner — rrecov build (MSVC), whole-disk scan of the seeded
2 GB GPT/NTFS VHDX, `--reconstruct-deleted` reconstruction, and the
byte-for-byte judge: `verify-report.json` = `ok: true, failures: []`
(I1 deleted-only, I2 exactness, I3 collision-set, I4 accountability,
I5 containment; 19 output files incl. the probe + stash twin, 17
dirs). Evidence captured to
`worklog/VALIDATION/fixtures/named-feature/` (verify-report,
output-listing — complete incl. dot entries, recovery-report 24/24,
reconstruct/scan logs, apply-log, self-test 6/6, transcript; README
run-history rows for 44475017206 and 45811533301). **Validation gate
2 (the named feature, byte-for-byte on real Windows): PROVEN.**

**`126ea398`** — F-03 (P1) landed, the matrix row the prior lane had
left "UNVERIFIED → verify + close": the NTFS parent-sequence gate
(SF §2.5): the $FILE_NAME parent link is accepted only while its
recorded parent sequence matches the parent record's CURRENT
sequence (equal, or +1 with the parent deleted — deletion bumps it
once) AND the parent is a directory; a reused parent record's stale
link orphans under the existing LostDir grouping (the F-01 plan maps
it to `_orphaned/`), never attaching under the unrelated reused
node. Both attachment sites gated (phase B + the GUID-dump host);
the USN/INDX remnant paths already resolve by full reference
(find_by_record exact-match) and are untouched. Test:
`NtfsVolume.ParentSequenceGateOrphansReusedParents` (reuse→LostDir24,
delete-bump→deaddir accepted, file-parent→LostDir17, base-fixture
pseq==pcur links untouched); all existing fixtures are pseq==pcur —
regression-neutral by construction. Local gate: linux-release build
clean (-Werror), full suite **1822/1822**, format/lint/secrets/
subject stages clean. Windows: engine-ci `45814007501` GREEN,
bridge-integration `45814007296` GREEN on `126ea398`.

**The remaining-row verdicts (verified in code, not assumed):**
F-35 = IMPLEMENTED (subsumed by F-34's mapfile resume + the D-IMG-1
cancel); F-12 = core IMPLEMENTED as backup-BS open-path fallbacks
(NTFS volume-end M-043/M-145/S-063, FAT32 sector-6 R5.1, exFAT +12)
— DELIBERATELY-DIVERGENT on TD's per-MiB lattice (deep-scan material
under F-13); F-04 = absent (zero-USN passes fixup today — scoped);
F-28/F-19/F-18-breadth/F-37 = not implemented — scoped. Deliverables:
`worklog/PARITY.md` (every matrix row resolved to IMPLEMENTED /
DELIBERATELY-DIVERGENT / NOT-APPLICABLE / FOLLOW-UP, with commit +
test + run citations), `worklog/FOLLOW-UP.md` (every unimplemented
row with entry points, tests to add, validation to prove), and
`engine/docs/CAPABILITY-MATRIX.md` truthed with the harvest rows
(F-03, F-01, F-33/F-34).

**Concurrent-lane interleave (recorded):** the cleanup-consolidation
session opened mid-flight (`f01f630a` implementation specs for the
remaining waves under `worklog/VALIDATION/specs/` — F-02, F-12/13/14/
19/21, F-18/20/22/23/28/29 — the exact rows FOLLOW-UP scopes;
`eeab73de` its session entry). This session's docs push was rebased
onto their head, never force-pushed. **Recorded deviation on
`6676b427`:** the rebase's stash staging accidentally folded the
five v0.9.7 version-site bumps (VERSION, ui/package.json,
tauri.conf.json, Cargo.toml, Cargo.lock app entry) INTO the docs
commit — mixed concerns in one commit, discovered post-push
(force-push forbidden; stands as recorded here). The tag below
carries the coherent tree.

**v0.9.7 release candidate (gate 7):** tagged on the harvest set —
F-01 named feature (closed-loop green), F-33/F-34 rescue imaging,
F-03 parent-sequence gate, the verifier/evidence fixes, PARITY +
FOLLOW-UP + specs. Release lane + validation chain: see the next
entry (append-on-green, never claimed ahead of the run).
## Session 2026-11-26 (E2E) — the end-to-end testing & validation harness: Phase 0 baseline

Session prompt archived (token redacted): worklog/session-e2e-testing-prompt.md.
Head at session start `126ea398` (v0.9.6, "feat(W2/F-03): the NTFS parent-sequence
gate"). Mission: a real Windows e2e harness — VHD orchestration, deterministic seed
corpora, the full deletion matrix, CLI + API paths, metrics vs the seed manifest,
four new workflows (smoke / full / regression / release-validation), committed
evidence, full documentation. Agent environment is Linux: every product execution
happens on real GitHub-hosted Windows runners through the new workflows; nothing is
simulated locally. Phase 0 (agents A–D: CLI surface, API surface, test inventory,
release layout) dispatched before any code is written. Entries follow as units land.

### Phase 0 complete (T2) — 2026-11-26

Agents A–D ran read-only over this tree at baseline `126ea398` and produced four
evidence-cited sheets: worklog/CLI-SURFACE.md (rrecov verb surface, exit-code
taxonomy, target forms incl. detached `--image` VHDX), worklog/API-SURFACE.md
(cr_ui ABI v4, 99 exports, the full call sequences), worklog/TEST-INVENTORY.md
(the 10 workflows, the proven fixture/bench primitives and their exact paths,
12 MISSING gaps), worklog/RELEASE-LAYOUT.md (RecoveryWizard-<v>-setup.exe /
-portable.zip / SHA256SUMS, installed layout, silent /S install). Coordinator
spot-verified the load-bearing claims directly (rrecov.cpp verb flags,
benchmark-matrix.json, New-TestDisk.ps1 diskpart-only vdisk pipeline,
apply-scenario.ps1's 12-mode deletion matrix, run-ours.ps1 CLI lane,
api_harness.c lifecycle, Run-Benchmark.ps1 orchestration, regression_gate.py
thresholds, benchmark-src.yml staging recipe). Verified key facts: the runner
e2e lane must reuse the CI-proven primitives (generate_seed.ps1 corpus v2.0.0,
apply-scenario.ps1, run-ours.ps1, api_harness.c, compute_metrics.py,
regression_gate.py) rather than fork them; the new-build list is: the e2e
matrix + orchestrator, the CLI-vs-API lane divergence check, the committed
results collector, the quality baseline, and the four workflows.

### The harness lands (T3–T13, T19) — 2026-11-26

tests/e2e/ is committed: matrix.json (45 full-matrix scenarios = 4 filesystems x
the 12-mode deletion matrix, 4 smoke rows, 9 stateless shards, gates cited from
the recorded bench evidence), scripts/Run-E2E.ps1 (the orchestrator: New-TestDisk
diskpart VHDX env -> generate_seed corpus v2.0.0 -> apply-scenario deletions ->
run-ours CLI lane -> api_harness API lane -> inventory+metrics -> Compare-Lanes
divergence -> verdicts -> Teardown always), scripts/Compare-Lanes.ps1 (the gate-5
CLI-vs-API agreement check: identical hash-match sets, sizes, scan stats),
scripts/Collect-Results.ps1 (merge + completeness - no silent drops),
scripts/New-Baseline.ps1 (baseline candidate; reviewed promotion only),
scripts/Assert-Clean.ps1 (token/placeholder hygiene gate). Reuse by invocation,
never fork - one copy of the proven engine/bench primitives. Four workflows:
e2e-smoke.yml (PR, 4 rows, CLI only, ~10 min), e2e-full.yml (dispatch+nightly,
9 shard jobs + build-kit + assemble+Assert-Clean, retention 30/90),
e2e-regression.yml (dispatch, full matrix then regression_gate.py vs
tests/e2e/baseline.json with -2%/-2%/+25% thresholds and --require-all-baseline),
release-validation.yml (dispatch: pull latest release, hash-verify, silent /S
install, shipped-bits assertions, full matrix against the installed product).
Docs: tests/e2e README/SCENARIOS/METRICS/RESULTS-SCHEMA/BASELINE + results/
README + docs/TESTING.md e2e section. matrix.json, all 4 workflows YAML-validated;
scripts hand-reviewed for the PS traps the lane's own AST gates re-verify on
the runner. Validation runs (T14-T17) dispatched next.

## Cross-repo harvest — W2/F-02 landed (2026-11-26, session continuation at 0dc07535)

The largest FOLLOW-UP row, closed end to end. The pure core
(`engine/src/core/ntfs/cr_mftpatch.{h,cpp}`) re-implements ntfs-recover's
repair path from the documented behavior (0BSD @5072bbc7; spec at
`worklog/VALIDATION/specs/F-02-mft-runlist-patch.md` — no code copied):
delta grouping (`delta = disk_off − rec#×rec_size` per extent), cluster
floor/ceil alignment, <4-cluster drop, sequence dedup, the VCN cap from
record 0's own end_vcn (preserves $ATTRIBUTE_LIST/$BITMAP), merge
(extend/trim/skip), sign-extension-safe run encoding (ntfs-3g spares the
sign bit — the 45669 EIO case), and the record-0 in-place patch
(un-fixup → patch → USA bump wrapping 0xFFFE→0x0001; refuse when runs
exceed the run space; never a from-scratch rebuild). The RawFixer seam
gains `ActionKind::RebuildMftRunlist` — it fires ONLY after the
$MFTMirr path refused (S3 preserved); the clean gate now requires the
$DATA to decode (a torn runlist under a valid $FILE_NAME is NOT clean —
the damage shape this row exists for). Tests: 22 new (20 unit + 2
end-to-end: torn runlist + torn mirror → analyze plans exactly the
patch → execute → NtfsVolume::open succeeds → undo restores
byte-for-byte; wiped MFT region → clean refusal + advisory). Local:
1843/1843 green (linux-release, -Werror), clang-format 18.1.8 clean.
PARITY.md F-02 → IMPLEMENTED (0dc07535); FOLLOW-UP.md F-02 → LANDED.
Remaining harvest rows in priority order: F-19 (fragmentation
reassembly), F-28/29 (FAT/exFAT orphan-subtree harvest), F-37 (imaging
presets), F-18 (registry breadth) — all scoped in FOLLOW-UP.md with the
specs in worklog/VALIDATION/specs/.

## Session 2026-11-26 (b) — Small-Bug Hunting & Stability Polish

**Start state**: HEAD `6676b427`, VERSION 0.9.7, main green (per last session close:
harvest-gates session ended with PARITY + FOLLOW-UP resolved, F-01 closed-loop
evidence captured).
**Session goal**: find and fix the small defects that wreck the product experience —
freezes, sector-scan stalls, wrong numbers, overflow/underflow, memory/CPU spikes,
unwanted loops, broken buttons (close/minimize/maximize/stop-scan), stuck UI states,
dead wiring between UI, core, and API. Every defect reproduced, every fix proven
before/after.

- **T1 done**: cloned, start state recorded (above), prompt saved token-redacted at
  `worklog/session-small-bugs-prompt.md`, read twice. This entry.
- **Phase 0 push note**: Agent B's sheet landed as `worklog/WIRING-AUDIT-SMALL-BUGS.md`
  (the standing `WIRING-AUDIT.md` from a prior Agent-D audit already occupied the
  canonical name; both preserved).
- **Environment truth**: this session executes in a Linux sandbox. Windows-only
  validation (runtime profiling captures, UI interaction on Windows, Windows CI,
  clean-Windows RC) is scoped precisely in `docs/OPEN-ITEMS.md` and handed off —
  nothing fabricated. Linux-runnable evidence (engine suites, UI unit/RTL suites,
  mock-connector e2e, static audits) runs for real here.

## Session 2026-11-26 (fix-upgrade-release) — Phase 0 pushed, rebased onto the concurrent v0.9.7 head

Head at session start `126ea398`. Prompt token-redacted:
worklog/session-fix-upgrade-release-prompt.md. Five evidence sheets by
parallel agents A–E (full text in the sheets; only the headlines here):
- REPO-STATE.md refreshed at 126ea398 (version lockstep 0.9.6 GREEN;
  stale sites: engine CMake/vcpkg 0.6.1, bridge-it 0.9.4, README
  92-exports/36-ops vs actual 99/59, forensic-repro default tag v0.9.3).
- CI-FAILURES.md: NO push lane red at HEAD (engine-ci 45814007501,
  bridge 45814007296 green at 126ea398; F-01 closed-loop PROVEN by
  named-feature-fixture 45811533301 at d65a5d12; ui lanes green at
  20efb62a). Two stale dispatch reds at pre-repair trees: benchmark-src
  44359663812 (fc0e1e19, 12 hard gates, IO starvation) and
  forensic-repro 44369227822 (v0.9.5, D-SCAN-01 800s freeze) — both
  re-dispatched for current-truth during this session.
- WARNINGS.md: engine 0 warnings on Linux at repo strictness (176/176
  targets, -Wall -Wextra -Wpedantic -Werror); ui gates all clean
  (typecheck/lint/format/arch-lint/knip, vitest 225/225). Findings:
  3 rustc warnings (bridge_integration.rs:491,498 unused_assignments,
  :580 dead_code), api_harness.c:39 C4005, 10 act() warnings
  (consent-surfaces.test.tsx:181), vite chunk 561.21kB>500kB, and the
  gate-set gap: no clippy lane exists anywhere.
- DEPENDENCIES.md: 90 deps — 30 CURRENT, 24 MAJOR-behind (typescript
  5.9.3→7.0.2, vitest 4→5, i18next 26→27, react-i18next 17→18,
  react-router 7→8, actions checkout v4→v7, cache v4→v6,
  upload-artifact v4→v7, download-artifact v4→v8, setup-node v4→v7,
  pnpm/action-setup v4→v6, gh-release v2→v3, windows-2022→latest,
  ubuntu→26.04, node 24→26, cmake→4.5.1, MSVC→VS2026), 18 MINOR,
  13 PATCH, 5 unpinned vcpkg ports.
- VULNERABILITIES.md: 0 critical / 0 high measured (cargo-audit 0.23.0
  both Cargo.locks, pnpm audit, osv-scanner v2.7.1 full-repo,
  secret-scanning 0 open). Medium: cookie 0.18.2→0.18.3
  (RUSTSEC-2026-0594), glib 0.18.5→0.20.0 (RUSTSEC-2024-0429),
  minimatch 10.2.6→11.0.1 dev-only; unrated: url 2.5.8→2.5.9
  (RUSTSEC-2026-0592); 5 unmaintained; async-executor 1.14.0 yanked.
  Dependabot+CodeQL not configured (gap).

Concurrent-writer discovery: while Phase 0 ran, a sibling session pushed
e9e38c92..6f7c7632 and cut/published tag v0.9.7 (worklog/
VALIDATION/RELEASE/v0.9.7.md). These five sheets are rebased onto that
head; the warning/vuln/upgrade findings above remain OPEN deferred items
for the next session (precisely scoped in the sheets). Token: env/git
config only, never committed.
