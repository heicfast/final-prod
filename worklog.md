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
