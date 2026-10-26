# ARCHITECTURE — the unified product

One product, four components, one contract boundary. The reference product
binds its UI to the engine IN-PROCESS through C-export factories
(corpus: searchdll-family--architecture.md §1); ours does the same through
the `cr_ui` C ABI — no subprocess, no IPC on the UI↔engine leg.

```
┌────────────────────────────────  RecoveryWizard.exe  ────────────────────────┐
│  WebView2 (React 19 UI)                                                       │
│    └─ src/queries/* ── connector contract (src/connector/contract.ts)         │
│         └─ src/connector/tauri/tauri-connector.ts  (36 methods, REASON_KEYS)  │
│              └─ invoke() ── 33 Tauri commands                                 │
│                   (manifest: contracts/command-surface.json;                 │
│                    validate.mjs cross-checks BOTH directions)                 │
│  Rust bridge  src-tauri/src/                                                  │
│    bridge/  abi.rs (pinned 1:1 to cr_ui.h) · state.rs (registries + the       │
│             snapshot metadata cache) · events.rs (sink→channel→emitter;      │
│             worker threads never block) · error.rs (code/bridgeCode truth)   │
│    commands/ devices · scan · results · preview · sessions · recovery ·      │
│             health · repair (G-1 gaps) · system                               │
│    ── static import ──▶  cr_ui.dll  (+ vcpkg runtime DLLs beside the exe)     │
└───────────────────────────────────────────────────────────────────────────────┘
                          │ C ABI v2 (78 exports)
              engine/  C++20 recovery core (scan/recover/repair/health,
                       sessions .crsf, the query layer)
                          │ RDA transport first, direct handles fallback
              driver/   rda.sys (kernel raw-disk transport, service `RDA`,
                        \\.\RDA\<token>\<disk>\<part> grammar)
```

## Component responsibilities

- **driver/** (`rda.sys`): raw sector access under admin/SYSTEM DACL;
  demand-start, no auto service; unsigned by design (release notes state
  the test-signing path). The engine falls back to direct handles
  (`\\.\PhysicalDriveN`) without it — degraded but functional.
- **engine/**: the recovery domain — staged scans (quick/deep/carve),
  lost-partition discovery + apply, pre-scan repair analysis (default-deny
  consent contract), bounded previews, the SafeWriter recovery path with
  the machine-readable report, .crsf sessions, SMART health. Persistence:
  caller-supplied paths only; the bridge supplies ProgramData.
- **bridge** (`ui/src-tauri/src/bridge`): registries with the engine's
  close order enforced; ONE metadata-cache filter path for results (G-7:
  the engine query cannot express the contract's 'existing' atom);
  never-blocking event marshaling (bounded sync channels, single emitter
  per handle, HEARTBEAT is liveness-only); errors carry the engine numeric
  + the UI vocabulary string.
- **UI** (`ui/src`): renders and requests — never re-implements the core.
  The connector seam (contract.ts) is the only backend; the labeled test
  double (`ui/tests/fakes/`) exists only in test-scope builds (arch-lint A7).

## Threading model (the load-bearing rules)

- Scan/recovery run on dedicated OS threads (`cr_ui_scan_run` /
  `cr_ui_recovery_process` are synchronous on the caller's thread); token
  calls (pause/resume/cancel) and telemetry probes are any-thread safe.
- Engine callbacks fire on worker threads: the bridge sinks `try_send`
  into bounded channels (never block, never lock, never re-enter); one
  emitter thread per handle drains, coalesces, and emits Tauri events.
- Terminal state is latched by the run/process thread exactly once; the
  final progress sample + `*-finished` events are emitted by that thread.

## Install layout (installer/recoverywizard.nsi)

| Component | Location |
|---|---|
| App + engine kit (DLLs BESIDE the exe — loader truth) | `%ProgramFiles%\HeicFast\Recovery Wizard\` |
| rrecov.exe (CLI diagnostic + the G-1 undo path) | beside the app |
| Driver (when the surfaced step is accepted) | `%SystemRoot%\System32\drivers\RDA.sys` + service `RDA` |
| Shared state (sessions, logs) | `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}\` |
| Per-user prefs | localStorage (`rw.ui.*`) |

Elevation: the installer launches asInvoker; per-machine/driver choices
elevate at the moment they are needed. The app manifest is
`requireAdministrator` (reference parity — both transport legs are
admin-only on stock Windows; documented in PARITY-DELTAS D-I3).

## Where the truth lives

- Boundary commands/events/errors/types: `contracts/` (validated by
  `validate.mjs` on every change; phase 2 cross-checks the Rust + TS sides).
- Capability gaps: `docs/INTEGRATION-GAPS.md` (G-1..G-11 + follow-ups).
- Layout divergences from the reference: `docs/PARITY-DELTAS.md`.
- The engine ABI itself: `engine/include/cr/cr_ui.h` (append-only policy).
