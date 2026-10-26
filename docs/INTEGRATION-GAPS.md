# Integration gaps — capabilities the UI expected that the real core does not provide

Honest gaps, not failures: a truthful disabled surface beats a false one.
Every row records the precise scope, the shipped behavior, and the follow-up
plan. Cross-references: `docs/INTEGRATION-PLAN.md` §3.11 (the ratified
register), `contracts/error-codes.json` (the error vocabulary),
`docs/PARITY-DELTAS.md` (deliberate divergences from the reference layout).

---

## G-1 — Standalone repair workbench (dryRunRepair / getUndoLog / undoRepairs); the consent flow SHIPPED

**Scope.** The 78-export cr_ui ABI exposes NO standalone repair surface:
RawFixer analyze/execute/undo exist in the C++ core and cross the ABI only
as pre-scan repair inside a scan (`cr_ui_scan_config.pre_scan_repair` +
`cr_ui_repair_consent_fn`, outcomes via `CR_EVT_REPAIR`, undo log via
`cr_ui_scan_repair_save_undo`). The RepairPage workbench's three methods
(dryRunRepair, getUndoLog, undoRepairs) therefore throw the typed
`not-implemented` from the bridge (`repair_dry_run` / `repair_undo_log` /
`repair_undo` in contracts/command-surface.json) and the UI renders the
truthful message — never a dead button.

**applyRepair.** The standalone apply is refused identically (same typed
`not-implemented` — `repair_apply`). The REAL repair path is the scan's
pre-scan analysis, and it now carries the FULL consent flow:

**The consent surface — SHIPPED (follow-up (a) landed).** `scan_start` arms
`pre_scan_repair = 1` WITH the consent callback (`bridge::consent`, plan
§4.5): when the engine finds a repairable plan it calls the callback ONCE
on the run thread, the bridge mints a requestId, emits
`consent-request:<scanId>` (the payload carries the plan + requestId), and
blocks the run thread on the registered gate — NO user-facing timeout (a
non-answer is a user decision). The scan-progress page renders the
non-dismissable prompt (from the event, or re-hydrated from
`scan_state`'s `pendingConsent` field), and `consent_respond(requestId,
allow)` — re-added to contracts/command-surface.json; the removed[] entry
retired — is the only answer path. Approve = the engine executes the
transactional repair through the shim-opened write view and the run tail
persists the undo log (`<sessions>/repair-undo/<scanId>.undo`, consumable
by `rrecov fix --undo-apply <file>`); deny = the scan stays read-only.
Default-deny holds on every teardown route: `scan_cancel` force-denies the
scan's pending gates FIRST (a pending consent blocks the run thread; the
engine observes the cancel token only after the callback returns), and the
run tail closes any survivor after `run()` returns. The typed
`consent-not-found` covers stale ids (double respond, cancel race, ended
scan). The event-keying deviation from plan §4.3
(`consent-request:<scanId>` + payload requestId, not
`consent-request:<consentId>`) is recorded in contracts/event-mapping.md —
a listener cannot subscribe to a name minted at request time.

**Follow-up.** (a) SHIPPED (the prompt + listener + `consent_respond` +
undo-log persistence above). (b) OPEN: propose the append-only ABI
additions the plan §3.8 named: `cr_ui_repair_analyze` /
`cr_ui_repair_apply` / `cr_ui_repair_undo` (ABI policy allows appends
without a version bump) — the standalone workbench trio stays
`not-implemented` until they exist. (c) OPEN (partial): undo is CLI-only:
`rrecov.exe` ships beside the app; `rrecov fix --undo-apply <file>`
restores from the undo log the bridge now saves after every approved
repair.

---

## G-2 — `currentPath` ticker

No cr_ui event carries the item being scanned. `ScanProgressEvent.currentPath`
is always null; the UI hides the "Searching: …" ticker and shows the stage
label (the scan-progress page already handles null truthfully).
**Follow-up:** none proposed — adding it would require per-item engine
callbacks the reference itself does not expose to its UI at this granularity.

## G-3 — `dataFoundBytes` live counter

Telemetry carries no aggregate found-bytes. Live progress reports 0; the
exact value is computed once at completion (the snapshot metadata sum) and
latched into the terminal state — `getScanState`/terminal events carry the
exact number. Session LIST rows report the stat-derived value without the
snapshot sum (the cheap pass OQ-4 sanctioned); save/load rows carry the
exact latched value.
**Follow-up:** a `bytes_found` telemetry field (append-only, compatible).

## G-4 — Rendered previews (pdf pages / av thumbnails / office extracted text)

The engine provides bytes + metadata only; no renderer component exists on
any side. Video/audio previews return the `av` payload with metadata and a
null thumbnail (the pane renders its own truthful "preview not rendered"
hint); document/archive previews return the `office` payload with null
extracted text; PDFs classify as documents (the `pdf` payload kind is never
emitted — a zero-page payload would break the pager). Image and text/hex
previews are REAL (bounded: image assembly ≤ 16 MiB, hex ≤ 64 KiB).
Thumbnails are graphics-only (`thumbnailAvailable` is false for video —
no renderer exists to produce one).
**Follow-up:** none in scope — rendering is a new feature, not a wiring gap.

## G-5 — Recovery options `keepEmptyFolders` + `fileRepair`

No engine equivalents exist (`cr_ui_recovery_config` has no empty-folder or
during-recovery-repair knob; core repair is the pre-scan RawFixer). The
RecoveryPage carries truthful disabled notes for both options
(recovery.fileRepair / the keepEmptyFolders note in the options block); the
connector passes the remaining three options through the config map
(collision, verifyOnWrite, rebuildFolders).
**Follow-up:** engine-side additions would be behavior adds — not proposed.

## G-6 — Engine version string

No cr_ui export carries an engine version string (`cr_ui_abi_version` is the
ABI contract number 2, not a product version). `AppInfo.engineVersion` is
null; the About dialog shows the truthful "—" (about.engineNone).
**Follow-up:** propose `cr_ui_engine_version` (append-only, compatible) —
the report JSON already carries `engine_version` internally.

## G-7 — Engine-side query for the results list

`cr_ui_query_run` is session-only, AND its filter set cannot express the
contract's 'existing' atom (active-only structured files: `cr_ui_query` has
`deleted_only` but no `deleted_never` knob). The bridge therefore runs ONE
filter/sort/paging implementation over its own snapshot metadata cache for
BOTH live scans and loaded sessions — identical semantics mid-scan and
post-scan, and the REAL partial tree replaces the retired simulator's
visibleFraction heuristic. Measured cost vs the engine's own query contract
(<200 ms @ 1M nodes) is recorded in `docs/INTEGRATION-VALIDATION.md`.
**Follow-up:** add a `deleted_state_mask` (or `active_only`) query knob,
then migrate session trees onto `cr_ui_query_run` and retire the bridge
filter for that path.

## G-8 — Device disconnect mid-scan

The ABI has no device-arrival/gone events. `deviceConnected` is synthesized
from CR_EVT_HEARTBEAT liveness + recent progress samples (2× the engine's 5 s
watchdog window); the reconnect affordance degrades to pause/resume (resume
re-probes the device through the engine's transport fallback chain).
**Follow-up:** none proposed at the ABI level (RegisterDeviceNotification
would be a shell-side add; the synthesis is honest and bounded).

## G-9 — Device bus kinds ('sd' / 'optical')

`cr_ui_disk_info` carries only the `removable` flag — SD cards and optical
drives are indistinguishable from USB sticks. `Device.kind` maps
removable→'usb', else→'internal'; the SdCardPage hero stays the static
display it already was (decoupled from enumeration).
**Follow-up:** an `interface_type` field in `cr_ui_disk_info` (append-only)
would let the map become exact.

## G-10 — Pre-scan lost-partition hints

Lost candidates are scan-time `CR_EVT_LOST_CANDIDATE` events; no pre-scan
hint exists. `Volume.lostPartition` stays absent; the Home badge surfaces
post-scan knowledge only (`cr_ui_scan_stats.lost_partitions`); the
LostPartitionPage scans for real. The bridge keeps a truthful lost-candidate
counter per scan (logged; no payload field exists in the contract — the
badge surface is future page wiring, plan §3.2 #9).
**Follow-up:** a `lostCandidates` progress-detail field when a badge surface
lands.

## G-11 — Folder-scoped scans

`cr_ui_scan_config` has no folder-scope parameter. The bridge scans the
WHOLE volume/disk and applies path-prefix scoping to every result view
(tree, query, count, detail, recovery enqueue). Documented semantics delta:
scan duration and progress reflect the whole volume; only results are
scoped. Invalid scopes (non-absolute, wrong volume) fail fast with
`scan-scope-invalid`; a scope the scan finds nothing under yields the
truthful empty tree (existence cannot be pre-verified — the scan IS the
discovery).
**Follow-up:** engine-side folder scoping (the scan would finish faster and
report scoped progress) — an ABI/config extension proposal.

---

## Hygiene items carried from the plan (§3.11, non-behavioral)

- The driver repo ships tracked ELF fuzz binaries; `rda.cat` is declared in
  the INF but never built; the UI repo's LICENSE gap was closed by the merge
  (root LICENSE). Tracked in the source repos' own docs; no action here.
