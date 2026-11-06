# ARCHITECTURE — the unified product

One product, four components, one contract boundary. The reference product
binds its UI to the engine IN-PROCESS through C-export factories
(corpus: searchdll-family--architecture.md §1); ours does the same through
the `cr_ui` C ABI — no subprocess, no IPC on the UI↔engine leg.

```
┌────────────────────────────────  RecoveryWizard.exe  ────────────────────────┐
│  WebView2 (React 19 UI)                                                       │
│    └─ src/queries/* ── connector contract (src/connector/contract.ts)         │
│         └─ src/connector/tauri/tauri-connector.ts  (38 methods, REASON_KEYS)  │
│              └─ invoke() ── 34 Tauri commands                                 │
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
                          │ C ABI v4 (92 exports)
              engine/  C++20 recovery core (scan/recover/repair/health,
                       sessions .crsf, the query layer) — USER-MODE,
                       no kernel component of ours: the transport opens
                       \\.\PhysicalDriveN / \\.\HarddiskVolumeN directly
                       (engine/src/win/cr_transport.cpp; the kernel driver
                       was removed in the consolidation session,
                       commit 318b9655)
```

The UI is being re-based onto the recovery-wizard-ui-new v3 wizard base
(mock connector is the interim state; P4 rewires it to the real engine
through this same connector seam — the ABI, bridge, and command-surface
described here are the target wiring the contract already pins).

## Component responsibilities

- **engine/**: the recovery domain — staged scans (quick/deep/carve),
  lost-partition discovery + apply, pre-scan repair behind the consent
  callback (the scan-page prompt approves; deny/cancel/teardown all
  default-deny), bounded previews, the SafeWriter recovery path with
  the machine-readable report, .crsf sessions, SMART health. The
  transport is **user-mode direct**: raw sector I/O on
  `\\.\PhysicalDriveN` / `\\.\HarddiskVolumeN` handles under an
  administrator token (no driver, no service — the kernel component
  was removed in the consolidation session, commit 318b9655).
  Persistence: caller-supplied paths only; the bridge supplies
  ProgramData.
- **bridge** (`ui/src-tauri/src/bridge`): registries with the engine's
  close order enforced; ONE metadata-cache filter path for results (G-7:
  the engine query cannot express the contract's 'existing' atom);
  never-blocking event marshaling (bounded sync channels, single emitter
  per handle, HEARTBEAT is liveness-only); errors carry the engine numeric
  + the UI vocabulary string.
- **UI** (`ui/src`): renders and requests — never re-implements the core.
  The connector seam (contract.ts) is the only backend; the labeled test
  double (`ui/tests/fakes/`) exists only in test-scope builds (arch-lint A7).
- **installer** (`installer/recoverywizard.nsi`): NSIS, all-users, one UAC
  prompt; lays down the app + engine kit under Program Files and prepares
  the ProgramData sessions/logs area. No service, no driver, no
  certificate-store changes — nothing is loaded into the OS.

## Privilege model (the load-bearing rule)

Raw-disk operations — scan, recover, health, repair, lost-partition apply —
open physical drives and volumes directly, which requires the process to
hold an **administrator token**. When it does not, every raw-disk verb
refuses with `ERR_ELEVATION_REQUIRED` (0x500C0001; FVE unlock paths carry
`ERR_FVE_ELEVATION_REQUIRED`); the CLI (`rrecov.exe`) exits 4 and names the
remedy. There is no degraded mode, no fallback, and no silent
partial-function: the refusal is the product's truthful answer — the
exe itself requests elevation at launch (requireAdministrator manifest,
D-PRIV-01: one UAC, reference parity; the retired in-app banner +
restart surface no longer exists). A device that was open and is physically gone mid-run is the
separate `ERR_DEVICE_DISCONNECTED` (0x50020007) — reattach and resume.

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

The machine footprint is exactly two locations — nothing is loaded into
the OS, and uninstall removes everything it placed:

| Component | Location |
|---|---|
| App + engine kit (DLLs BESIDE the exe — loader truth) | `%ProgramFiles%\HeicFast\Recovery Wizard\` |
| rrecov.exe (CLI diagnostic + the G-1 undo path; raw-disk verbs exit 4 when not elevated) | beside the app |
| Shared state (sessions, logs) | `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}\` |
| Per-user prefs | localStorage (`rw.ui.*`) |

Elevation: the installer carries an `admin` manifest (all-users install,
Program Files + HKLM + the ProgramData shared state; one UAC prompt at
launch) and shows an informational privilege page — the product is
user-mode, and it is the APPLICATION that needs the administrator token
for raw-disk verbs, per the privilege model above. The driver-era
`System32\drivers` row, the `RDA` service, and the certificate-store
imports are gone (consolidation session, commit 318b9655).

## Where the truth lives

- Boundary commands/events/errors/types: `contracts/` (validated by
  `validate.mjs` on every change; phase 2 cross-checks the Rust + TS sides).
- Capability gaps: `docs/INTEGRATION-GAPS.md` (G-1..G-11 + follow-ups).
- Layout divergences from the reference: `docs/PARITY-DELTAS.md`.
- The engine ABI itself: `engine/include/cr/cr_ui.h` (append-only policy).
