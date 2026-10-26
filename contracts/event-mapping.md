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
| `CR_EVT_REPAIR` | 7 | `repair_actions`, `repair_outcome` (0 plan-ready / 1 applied / 2 denied / 3 failed) | The outcome record: latched into the scan's repair summary line + stats (0 plan-ready = the consent callback returned deny/NULL-armed; 1 applied = the prompt approved and the engine executed; 2 denied; 3 failed). The consent PROMPT itself is emitted by `cr_ui_repair_consent_fn` (bridge::consent), not this event — see the consent section below. |
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

## Consent events (bridge surface — shipped per G-1 (a))

The consent callback (`cr_ui_repair_consent_fn`, `cr_ui.h`) runs ON the
engine's run thread, at most ONCE per scan (cr_ui.h S-008: only when a
repairable plan exists, before any engine read). The bridge
(`ui/src-tauri/src/bridge/consent.rs`):

1. mints a requestId (an opaque echo token), registers a pending
   `ConsentGate` in the state registry, and — with the registry lock
   RELEASED — emits `consent-request:<scanId>` directly from the run
   thread, carrying the plan POD translated to JSON (`detected`,
   `fsFamily`, `actionCount`, `refused`, `reason`, `advisoryCount`) plus
   the requestId. The event is SCAN-scoped so the page can subscribe in
   advance; the minted requestId — carried in the payload, never the
   event name — is the only key `consent_respond` accepts. (Plan §4.3
   drafted `consent-request:<consentId>`; the shipped keying keeps the
   token in the payload because a listener cannot subscribe to an event
   name minted at request time — the deviation and its mechanics are
   recorded here, the ratified plan text stays frozen as history.)
2. the run thread then BLOCKS on the gate — NO user-facing timeout
   (plan §4.5: a non-answer is a user decision, not a timer);
3. the scan page renders the typed-consent prompt from the event, or
   from scan-state's `pendingConsent` re-hydration field (the event fired
   before the page's subscription landed, or the user navigated away and
   back), and answers via the `consent_respond` command. A stale id —
   double respond, cancel force-deny, scan ended — throws the typed
   `consent-not-found` and the prompt dismisses;
4. default-deny holds on every teardown path: the engine denies when the
   callback is absent, `scan_cancel` force-denies the scan's pending
   gates FIRST (a pending consent blocks the run thread inside the
   callback; the engine observes the cancel token only after it returns —
   plan §4.5's deadlock rule), and the run tail closes any survivor
   after `cr_ui_scan_run()` returns. An APPROVED repair executes
   transactionally through the shim-opened write view; the run tail
   persists the undo log for `rrecov fix --undo-apply <file>` (G-1 (c)).
