# Event mapping — `CR_EVT_*` → connector progress events

Source of truth for the subscription-shaped events the UI consumes:
`ScanProgressEvent` (contract.ts) from scan handles, `RecoveryProgressEvent`
from recovery jobs, plus the consent/save surfaces the bridge adds. The engine
side is `cr_ui_event` (POD, `engine/include/cr/cr_ui.h:190-234`) delivered on
**engine worker threads** via `cr_ui_event_fn` — one sink per scan, subscribed
BEFORE `cr_ui_scan_run`.

## Marshaling rules (non-negotiable, from the engine's threading contract)

1. The sink runs on an engine worker thread (heartbeat: the shim's watchdog
   thread; save-progress: the save thread). It MUST NOT block, lock the
   bridge registry, or call back into the engine.
2. The sink does exactly one thing: `try_send` a serialized copy of the event
   into an mpsc channel owned by the scan's bridge entry. Channel-full means
   the UI is slow; the emitter thread coalesces (drops superseded progress
   samples, never stage transitions or error-range deltas).
3. A single emitter thread per scan drains the channel and calls
   `app_handle.emit("scan-progress:<scanId>", payload)` (Tauri event). The TS
   side subscribes via `listen()` inside `onScanProgress` and returns the
   unlisten function — the contract's unsubscribe shape is unchanged.
4. Unknown `cr_ui_event.type` values are ignored (forward compatibility,
   `cr_ui.h` contract).
5. Event emission NEVER synthesizes progress: `CR_EVT_HEARTBEAT` carries no
   progress payload and the bridge must not fabricate one from it.

## The map

| `cr_ui_event.type` | Value | Payload fields used (cr_ui_event) | Bridge effect |
|---|---|---|---|
| `CR_EVT_SCAN_STAGE` | 1 | `stage` (0 quick / 1 deep / 2 carve / 0xFFFFFFFF none) | Emits a stage-transition progress event. Stage names (contract `ScanStage`): pre-scan phases → `'volume-analysis'`; stage 0 → `'quick-index'`; 1 → `'deep-walk'`; 2 → `'carve'`; post-run → `'tree-build'` then `'done'`. |
| `CR_EVT_SCAN_PROGRESS` | 2 | `stage_permille`, `overall_permille`, `bytes_done`, `bytes_total`, `files_found/deleted_found/carved_found` (cumulative), `error_ranges`, `timestamp_ms` | Emits the periodic progress event: `stageProgress = stage_permille/1000`, `overallProgress = overall_permille/1000` **clamped monotonic** by the bridge (the engine guarantees monotonic `overall_permille`; the clamp is defense in depth), `bytesScanned` ← engine telemetry `bytes_scanned` (coalesced snapshot from `cr_ui_scan_telemetry` at emit time), `filesFound` ← `files_found + deleted_found + carved_found`. |
| `CR_EVT_FILES_FOUND` | 3 | cumulative `files_found`, `deleted_found`, `carved_found` | Emits a progress event carrying the bumped `filesFound`. |
| `CR_EVT_ERROR_RANGE` | 4 | `err_start_bytes`, `err_len_bytes`, `error_ranges`, `err_module` (`CR_ERRMOD_*`), `err_operation` (`CR_ERROP_*`), `err_attempts` | Appends `{startByte: err_start_bytes, sizeBytes: err_len_bytes}` to the scan entry's bad-sector map (bounded: the bridge keeps the map plus the count; payload carries the map) and bumps `errors` to the cumulative `error_ranges`. Module/op/attempts ride along in the bridge log only (M-049 diagnostic), not the UI payload. |
| `CR_EVT_CHECKPOINT` | 5 | `checkpoint_index` | No UI emission (internal checkpoint truth; `cr_ui_scan_checkpoint_count` is the queryable surface). |
| `CR_EVT_LOST_CANDIDATE` | 6 | `candidate_lba` | Increments the scan entry's lost-candidate counter; next progress event carries it in `detail.lostCandidates` (the Home/LostPartition badge surfaces read the count post-scan from `cr_ui_scan_stats.lost_partitions`). |
| `CR_EVT_REPAIR` | 7 | `repair_actions`, `repair_outcome` (0 plan-ready / 1 applied / 2 denied / 3 failed) | Emits `consent-request:<consentId>` semantics per the consent flow when outcome is plan-ready (pre-scan repair path); applied/denied/failed outcomes emit a repair-result toast event the Scan screen renders. |
| `CR_EVT_HEARTBEAT` | 8 | `timestamp_ms` | Liveness only: updates the scan entry's last-heartbeat instant (dead-scan detection after device yank). NO progress synthesis. |
| `CR_EVT_SAVE_PROGRESS` | 9 | `stage_permille`, `overall_permille` | Drives the saveSession spinner: emits `save-progress:<scanId>` with a 0..1 permille fraction. |
| `CR_EVT_FS_CHECK` | 10 | `check_errors` (0x1001..0x100e set count), `check_family` | Bumps the scan entry's damage-audit counter (surface: scan screen's truthful "X file-system issues found" note; the full detail lives in stats/session records). |

## Recovery progress (`RecoveryProgressEvent`)

`cr_ui_recovery_progress_fn` (`cr_ui.h:124`) is **bytes-level**: 
`(void* user, uint64_t bytes_done, uint64_t bytes_total)` — no per-item callback
exists (OQ-1 resolved from the header). The bridge therefore composes
`RecoveryProgressEvent` as:

- `overallProgress` ← `bytes_done / bytes_total` (0 when total==0), clamped
  monotonic;
- `completedCount`/`failedCount`/`skippedCount`/`totalCount` ← live deltas
  applied to the job's item ring as `cr_ui_recovery_report_json` items appear
  (the report is valid at any time; the bridge reads it on each drain cycle)
  and the live counters from `cr_ui_recovery_get_stats` — `done`, `errors +
  hash_mismatch` → failed, `refused` → skipped, `queued` → total;
- `currentFile` ← the report's most recent item `path` while `running`
  (bytes-level callback gives no filename; the report tail is the truthful
  live item — null when nothing has committed yet);
- `currentFileProgress` ← null-equivalent 0 (bytes-level truth; the UI's
  per-file bar renders the indeterminate state — matches the contract, which
  the mock also satisfied with 0 during bulk phases);
- `verifying` ← derived from the writer phase signal: when `bytes_done`
  stalls and items report hash verification (`hash_status=='verified'`
  appears), the flag is true; the bridge never fabricates a verify phase the
  report cannot back;
- `throughputBytesPerSecond` / `etaSeconds` ← bridge-derived from
  `bytes_done` deltas over wall time (bounded, smoothed);
- `status` ← the job status map (contracts/type-maps.md §Recovery).

Emitted over the same channel→emit discipline on `recovery-progress:<jobId>`.

## Consent events (bridge surface)

The consent callback (`cr_ui_repair_consent_fn`, `cr_ui.h:423`; apply consent
in the lost-partition flow) runs ON the engine's run thread. The bridge:

1. mints a `consentId`, registers a pending oneshot, and `try_send`s the
   request (never blocks the registry — deadlock analysis in
   docs/INTEGRATION-PLAN.md §4.5);
2. the emitter thread emits `consent-request:<consentId>` with the plan POD
   translated to JSON (`detected`, `fs_family`, `action_count`, `refused`,
   `reason`, `advisory_count`);
3. the UI renders the typed-consent prompt (the existing REPAIR token flow)
   and resolves via the `consent_respond` command;
4. default-deny holds twice: the engine denies when the callback is absent,
   and the bridge denies a consent whose window/subscription is gone.
