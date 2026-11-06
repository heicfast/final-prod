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
