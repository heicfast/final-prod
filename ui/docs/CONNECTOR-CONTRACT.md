# CONNECTOR-CONTRACT

The typed command/event surface between the UI and the recovery backend.
Single source of truth: `src/connector/contract.ts`. This document is the
human-readable companion — where the contract came from, what each domain
guarantees, and exactly how the real bridge replaces the simulator.

## Contract principles

1. **The UI renders and requests; the connector provides and grades.**
   Confidence, health status, error reasons, and destination validation are
   backend decisions. The UI never re-grades, re-interprets, or overrides.
2. **Errors carry machine codes + i18n reason keys.** The UI maps
   `ConnectorErrorCode` and `reasonKey` to localized copy — never string-matches
   message text.
3. **Events are subscription-shaped.** `onScanProgress` / `onRecoveryProgress`
   return unsubscribe functions; the UI consumes them via
   `useSyncExternalStore` external stores (`src/queries/scan.ts`,
   `src/queries/recovery.ts`).
4. **Bounded payloads.** Preview payloads are bounded by the backend (hex
   first 64 KiB, text capped, PDF page bitmaps, AV first-frame). The UI never
   requests unbounded data.

## Domains

| Domain   | Commands                                                                                                                                         | Events                          |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------- |
| Devices  | `listDevices`, `getDeviceInfo(id)` (id may be a device or volume id), `refreshDevices`                                                           | —                               |
| Scan     | `startScan(params) → {scanId}` (params include optional `pathScope`), `pauseScan`, `resumeScan`, `cancelScan`, `getScanState`                    | `onScanProgress(scanId, cb)`    |
| Results  | `getResultTree(scanId, parentId, mode)`, `queryResults(scanId, filter, sort, page)`, `getNodeDetail`, `countResults`                             | —                               |
| Preview  | `requestPreview(scanId, nodeId, kind)` → bounded payload union, `getThumbnail`                                                                   | —                               |
| Sessions | `saveSession(scanId, path)`, `listSessions`, `loadSession(path) → {scanId}`, `getSessionMeta`, `deleteSession`                                   | —                               |
| Recovery | `startRecovery(scanId, nodeIds, destination, options) → {jobId}`, `cancelRecovery`, `getRecoveryState`, `getRecoveryItems`, `getRecoveryReport`  | `onRecoveryProgress(jobId, cb)` |
| Health   | `getHealth(deviceId)` → SMART table, class, TRIM, wear                                                                                           | —                               |
| Repair   | `dryRunRepair(deviceId)`, `applyRepair(deviceId, {consentToken})`, `getUndoLog`, `undoRepairs`                                                   | —                               |
| System   | `getDriverStatus`, `getAppInfo`, `validateDestination(path, sourceDeviceId, neededBytes)`, `pickDirectory(hint?)`, `listMountPoints`, `openPath` | —                               |

### Key payload shapes

- `ScanProgressEvent`: weighted-monotonic `overallProgress` (never regresses,
  and normalizes active stage weights when `quick-index` is omitted on a volume
  with `quickScanAvailable: false`), per-stage `stageProgress` (may reset
  between stages), live counters (`filesFound` and `dataFoundBytes` reach 100%
  of the volume/scope total upon completion), bad-sector map, ETA,
  `recoverableDuringScan`, `deviceConnected`.
- `ResultsPage`: `{ nodes, total, totalBytes, offset, pageSize, elapsedMs }` —
  carries both the filtered file count (`total`) and the filtered byte total
  (`totalBytes`) so the status bar renders accurate filtered byte sums.
- `ResultsFilter`: pushed server-side — tree scope, search, categories, date
  range, size range, confidence, extensions, origins. The connector resolves it
  over typed-array scans; the real engine will resolve it in Rust.
- `ScanParams.pathScope`: optional folder restriction. An absolute path on
  the target volume; `null`/omitted = whole volume. (G-11: the engine has no
  folder-scoped scan — the bridge scans the whole volume and applies
  path-prefix scoping to every result view.) An
  unresolvable folder is a hard `scan-scope-invalid` error — the UI cannot
  override the connector verdict. Scoped scans list only files inside the
  subtree; the path-mode tree presents the scanned folder itself as root.
- `pickDirectory(hint?)`: the optional hint roots the native dialog at a
  volume (`{ volumeId }`) — used by the Home "Select Folder" quick target
  (picker runs first, then the scan starts directly on the chosen scope).
  Destination picking (recovery) calls it without a hint.
- `RecoveryOptions`: collision policy (`rename`/`overwrite`/`skip`), folder
  reconstruction, empty-folder preservation, file repair, hash-on-write verify.
- `DestinationValidation`: `{ allowed, reason?, reasonKey?, freeBytes?,
neededBytes? }` — **the overlap refusal is hard**: the UI cannot proceed
  when `allowed === false` with reason `overlap`.

### Semantic notes for the real adapter (from the corpus research)

- Scan modes map to the reference's staged model: quick = FS index pass;
  deep = full structural walk; deep-carve adds the signature scanner.
- `.rsf`-style sessions: save serializes the found-file tree + scan metadata;
  load returns a resumable `scanId`; a corrupt session must fail with
  `session-corrupt` (never a partial load).
- Path-mode tree partitions files into three disjoint top-level branches
  (`root` for existing + normal deleted files with intact paths, `lost-paths`
  for `origin === 'lost-path'`, and `raw-all` for `origin === 'raw'` with
  `path: null`).
- Zero-byte files (`sizeBytes === 0`) are always graded `confidence: 'poor'`
  per `CRecoverability::GetEntryRecoverability` (`size < 1 → Poor`).
- Repair is transactional: every `applyRepair` must be undo-able via the
  undo log; `consentToken` must be the user's explicit acknowledgment
  (`REPAIR` typed confirmation in the UI).

## The real bridge (shipped)

The seam the contract promised held: `src/connector/tauri/tauri-connector.ts`
implements all 36 methods over the 33 Tauri commands of
`contracts/command-surface.json` (repository root), and the Rust bridge
(`src-tauri/src/{bridge,commands}/`) binds those commands to the cr_ui
engine ABI. `contracts/validate.mjs` cross-checks the manifest against BOTH
implementations on every change (phase 2).

- Error model: the bridge rejects with `{ code, bridgeCode, message }`;
  `bridgeCode` maps directly to `ConnectorErrorCode`, `code` maps through
  the engine numeric table (`contracts/error-codes.json`). The UI renders
  `reasonKey` strings only. `ERR_CANCELLED` never crosses as an error —
  cancel paths resolve with status 'cancelled'.
- Events: `scan-progress:<scanId>` / `recovery-progress:<jobId>` via Tauri
  `emit`/`listen`; the subscriptions keep the contract's unsubscribe shape.
- The simulated backend is GONE (no env gate, no fallback): the factory
  (`src/connector/index.ts`) exports the real connector singleton only.
- Integration gaps are recorded truthfully in `docs/INTEGRATION-GAPS.md`
  (repository root) — e.g. the G-1 repair workbench throws the typed
  `not-implemented` from the bridge, and the UI renders the message.

## The test double (never shipped)

`tests/fakes/test-connector.ts` implements the same contract against small
deterministic fixtures so the component and e2e suites run without the
engine. It is wired ONLY through the Vite test-mode alias (vitest runs,
`vite build --mode test`); production builds can never reach it (arch-lint
A7; CI asserts the bundle contents). The double fails EXACTLY where the
real bridge fails — the G-1 repair gaps refuse identically — never better.
