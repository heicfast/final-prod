# Integration gaps — capabilities the UI expected that the real core does not provide

Honest gaps, not failures: a truthful disabled surface beats a false one.
Every row records the precise scope, the shipped behavior, and the follow-up
plan. Cross-references: `docs/INTEGRATION-PLAN.md` §3.11 (the ratified
register), `contracts/error-codes.json` (the error vocabulary),
`docs/PARITY-DELTAS.md` (deliberate divergences from the reference layout).

---

## G-1 — Standalone repair workbench (dryRunRepair / getUndoLog / undoRepairs) + the consent flow

**Scope.** The 78-export cr_ui ABI exposes NO standalone repair surface:
RawFixer analyze/execute/undo exist in the C++ core and cross the ABI only
as pre-scan repair inside a scan (`cr_ui_scan_config.pre_scan_repair` +
`cr_ui_repair_consent_fn`, outcomes via `CR_EVT_REPAIR`, undo log via
`cr_ui_scan_repair_save_undo`). The RepairPage workbench's three methods
(dryRunRepair, getUndoLog, undoRepairs) therefore throw the typed
`not-implemented` from the bridge (`repair_dry_run` / `repair_undo_log` /
`repair_undo` in contracts/command-surface.json) and the UI renders the
truthful message — never a dead button.

**applyRepair.** The standalone apply is refused identically. The REAL
repair path is the scan's pre-scan analysis: `scan_start` arms
`pre_scan_repair = 1` with `repair_consent = NULL` — the engine's own
DOCUMENTED default-deny ("Zero, a NULL callback, or no available write view
= the plan is surfaced via CR_EVT_REPAIR events and stats and NEVER
executed", cr_ui.h S-008). Analysis evidence (planned action counts,
outcomes) rides `cr_ui_scan_stats` + the session record; nothing is ever
written to a volume from this build.

**The consent surface itself.** The ratified plan's consent flow (mid-scan
typed-REPAIR prompt + `consent_respond` command) has NO UI surface to drive
it today — the scan-progress page consumes only `ScanProgressEvent` and no
consent prompt component exists. Wiring the callback to a blocking oneshot
with no listener would deadlock the engine's run thread; inventing a prompt
component is a feature add, out of the session's scope. The bridge therefore
ships the engine-documented NULL-consent posture and the command manifest
records `consent_respond` in its `removed` array with this rationale.

**Follow-up.** (a) Build the scan-page consent prompt (typed-REPAIR token
flow reused) + a `consent-request:<id>` listener; re-add `consent_respond`
when it lands. (b) Propose the append-only ABI additions the plan §3.8
named: `cr_ui_repair_analyze` / `cr_ui_repair_apply` / `cr_ui_repair_undo`
(ABI policy allows appends without a version bump). (c) Until (b), undo is
CLI-only: the installer ships `rrecov.exe` beside the app; `rrecov fix
--undo-apply <file>` restores from an undo log saved via
`cr_ui_scan_repair_save_undo`.

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
