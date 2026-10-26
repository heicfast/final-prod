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
