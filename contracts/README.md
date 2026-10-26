# contracts/ — the engine↔UI boundary truth

This directory is the **single source of truth** for everything that crosses
the boundary between the native engine (`engine/`, C ABI `cr_ui`) and the
application UI (`ui/`, TypeScript `RecoveryConnector` seam). Both sides'
build/CI checks validate against the files here; nothing else may define
these mappings.

Ownership model (no duplication):

| Truth | Owned by | Location |
|---|---|---|
| The engine ABI (78 `cr_ui_*` exports, structs, event POD) | engine | `engine/include/cr/cr_ui.h` — frozen ABI, append-only |
| The engine error numeric space | engine | `engine/include/cr/cr_error.h` |
| The UI seam (9 domains, 36 methods, `ConnectorErrorCode` vocabulary) | ui | `ui/src/connector/contract.ts` |
| **How the two map to each other** | **this directory** | `error-codes.json`, `event-mapping.md`, `type-maps.md`, `command-surface.json` |

## Files

- `error-codes.json` — every `cr::Err` numeric code ↔ the `ConnectorErrorCode`
  the bridge surfaces, with context overrides and bridge-only codes.
- `event-mapping.md` — `CR_EVT_*` (types 1–10) ↔ connector progress-event
  fields; the marshaling rules (worker-thread → channel → Tauri emit).
- `type-maps.md` — field-level maps for every struct conversion the bridge
  performs (device/volume → `Device`/`Volume`, telemetry → `ScanProgressEvent`,
  node info → `ResultNode`/`TreeNode`/`NodeDetail`, query → `ResultsFilter`,
  recovery config → `RecoveryOptions`, stats → `RecoveryJob`/`RecoveryReport`,
  SMART → `DiskHealth`, preview metadata → `PreviewPayload`).
- `command-surface.json` — every Tauri command the bridge exposes: name,
  arguments, return shape, the `RecoveryConnector` method(s) it serves, the
  cr_ui ABI functions it calls, and its threading class.
- `validate.mjs` — the CI gate (node): parses `engine/include/cr/cr_error.h`
  and `engine/include/cr/cr_ui.h`, cross-checks `ui/src/connector/contract.ts`
  and `ui/src-tauri/src/**`, and fails on any hole or drift:
  1. every `cr::Err` value is mapped (no unmapped engine code),
  2. every mapped UI code exists in `ConnectorErrorCode`,
  3. every `CR_EVT_*` 1..10 is mapped,
  4. every command in `command-surface.json` has a Rust `#[tauri::command]`
     implementation and every Rust command is in the manifest,
  5. every command the TS connector invokes is in the manifest (and vice
     versa where the manifest says `invokedBy`).

## Cross-tree constants (defined ONCE — here; nowhere else)

| Constant | Value | Consumers |
|---|---|---|
| Driver service name | `RDA` (device `\\.\RDA`; installer pins; matches `engine`'s RDA transport probe and `driver`'s service-name-derived device grammar with `UPPER(SN)`) | installer (§installer/recoverywizard.nsi), bridge `get_driver_status`, `contracts/command-surface.json` |
| Sessions directory | `%ProgramData%\HeicFast\Recovery Wizard\sessions\` | bridge `session_save`/`sessions_list`/`session_load`, installer (creates + ACLs) |
| Logs directory | `%ProgramData%\HeicFast\Recovery Wizard\logs\` | bridge logger, installer (creates + ACLs) |
| Product version | root `VERSION` file (`0.5.0`) — the ONLY version constant; engine CMake, ui Cargo/tauri.conf/package.json, driver rda_version.h/INF, and the NSIS installer all derive from it and `build.ps1` fails on drift | everything |
| Required `cr_ui` ABI version | `2` (`CR_UI_ABI_VERSION` in `engine/include/cr/cr_ui.h`) — the bridge refuses to bind on any other value | bridge `abi.rs` startup gate |

Portable-mode override (documented deviation, `docs/PARITY-DELTAS.md`): when
the app directory is not the installed Program Files tree (portable zip), the
sessions/logs directories resolve beside the executable (`.\sessions`, `.\logs`)
— matching the reference's module-relative model for its portable/boot-media
modes. The probe is an app-dir writability check at startup; there is no
configuration surface for it (one behavior, decided by install mode).
