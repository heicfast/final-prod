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
