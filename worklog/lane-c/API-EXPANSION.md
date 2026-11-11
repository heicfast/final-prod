# Lane C — the API-surface expansion sheet

Open: 2026-11-11, post-v0.9.2 (main at 8f3bedd1). Lane C is the
API-surface lane this session: the user's mandate is a production-grade
expansion of the Tauri 2 command surface — more capability exposed to the
React 19 UI, better wiring, better compatibility — while the defect lane
works the reported breakages (scan stuck, SMART first-disk-only, missing
disks). Lane C does not touch the defect lane's active files without an
entry here first (the seam rule below).

## Why this lane exists (the user's words)

"your job is to increase the api support [...] make our api more
supportive and more better and production grade, better compatibility,
better response, better wiring. Also increase the support and surface of
api too, implement more features and functions and expose more core to
api so ui interface can utilize it more better."

## The evidence base (Phase C0 — read before write)

The engine ABI (`engine/include/cr/cr_ui.h`, all declared in
`ui/src-tauri/src/bridge/abi.rs`) carries capability the bridge never
surfaced. INTEGRATION-PLAN §4.7 rows 16-27 mark them DISABLE with "no
reference screen" rationales. The corpus CONTRADICTS several of those
rationales (sources: `worklog/REFERENCE-BEHAVIOR.md` + the research
corpus artifacts cited per row below):

| §4.7 row | Capability | Corpus evidence the reference DOES expose it |
|---|---|---|
| 18 disk map | `cr_ui_disk_map` (+_free) | `AnalysisGridWidget` (54 v-methods) paints the live sector map inside CDetailPage (UIUX/08:11, UIUX/13:47) |
| 19 image create | `cr_ui_image_create` | Home "Create Image" -> DiskBackupDialog chain, .edisk, byte-for-byte, sector range, elapsed/remaining, stop-confirm (strings:14534, UIUX/07:37, UIUX/03:125) |
| 20 fsck | `cr_ui_fsck_volume` | FileSystemCheck.mo ladder (strings:126; the M-075 diff-to-fix lineage) |
| 21 bitlocker off | `cr_ui_turn_off_bitlocker` | CBitlockerTurnOffOperation (M-078 lineage) |
| 22 TRIM chip | `enumerate_disks_ex` FLAG_TRIM | Home-row "TRIM OFF/ON" chip (UIUX/07:17,46) |
| 23 image targets | `cr_ui_open_image` | "Import a disk image" + "Choose a file to Scan" `*.vmdk *.vhd *.vhdx *.edisk *.dmg *.img` (strings:11803-11810) |
| 16/17 carve catalog/tally | `carve_format_count/_info`, `scan_carve_tally_*` | GetRawAnalyserList/GetRawUserInfoMap exports (RAWSearchDLL :72213; the "found N .jpg" report :227538) |
| 25 assistant + regrade | `cr_ui_scan_assistant`, `cr_ui_session_regrade` | RecoveryAssistant block + CRecoverability verdict + "Recovery Chances: High" gauge (UIUX/30:146, subsystems:9-52) |

Lane C closes these rows with real bridge commands, real connector
methods, real UI surfaces — every one backed by the ABI that already
exists. No engine change is in lane C's scope (engine/** stays with the
defect lane this session; the one engine-side gap found — no
`cr_ui_image_cancel` export; the win layer's CancelToken is not ABI-
visible — is recorded below as the honest limitation, not papered over).

## Scope (what lane C builds)

New bridge modules (new files — zero overlap with the defect lane):

- `commands/imaging.rs` — the Create Image flow: `image_create_start`
  (pre-gates: target disk present, destination letter valid + free space,
  same-disk destination warning axis), `image_create_state`. Progress on
  `image-progress:<id>` + terminal `image-finished:<id>` (the
  spawn_job_emitter shape: bounded channel + coalescing emitter; the
  engine's own `recovery_progress_sink` signature is reused verbatim —
  image_create takes the same `cr_ui_recovery_progress_fn`). Own
  `ImagingState` registry (Tauri-managed; zero edits to bridge/state.rs).
  NO cancel command: the ABI exports none (cr_ui_image_create runs
  cancel=nullptr; the engine keeps `<path>.partial` on failure — the
  honest contract). Divergence from the reference's stop-confirm dialog
  is PARITY-DELTAS-registered, not faked.
- `commands/diagnostics.rs` — the observability surface:
  `scan_telemetry` (live stage telemetry + checkpoint count; live scans
  only — loaded sessions keep their final stats on session_meta),
  `scan_assistant_stats` (the S-023 recoverability counters; live scans),
  `carve_catalog` (the static format registry), `scan_carve_tally`
  (per-format found counts + the user-sign aggregate slot),
  `disk_map` (allocation/coverage/found/confidence overlays + bad
  ranges; bounded cell budget, caller-tunable window),
  `device_transport` (the transport string + last_transport_error
  advisory), `session_regrade` (grade-1-only re-grade, session trees,
  the UI node-id format resolved through the snapshot metadata cache).
- `commands/maintenance.rs` — the privileged one-shots:
  `volume_fsck` (chkdsk check/fix posture, hidden vehicle, raw exit code
  + verdict), `volume_bitlocker_off` (the destructive decrypt twin —
  explicit user action only; the UI gates it behind a confirm dialog).

Seam crossings (announced here, additive only):

- `commands/mod.rs` + `src-tauri/src/lib.rs` — the three `pub mod` lines
  + the generate_handler registrations. Nothing else in those files.
- `contracts/command-surface.json` — new rows (validate.mjs phase 2:
  manifest <-> Rust <-> connector, both directions).
- `contracts/event-mapping.md` — the image-progress/image-finished
  bridge emissions documented.
- `contracts/error-codes.json` — bridge-only `image-not-found` (the
  registry miss) if the vocabulary needs it (decided at implementation:
  reuse `scan-not-found`? No — a distinct code keeps the UI's error
  surface truthful; added as bridgeOnlyCodes + ConnectorErrorCode).
- `ui/src/connector/contract.ts` — the ImagingDomain + the extended
  Scan/Results/System domain methods (types only, no behavior).
- `ui/src/connector/tauri/tauri-connector.ts` — the implementations.
- `ui/src/connector/mock/*` — dev-mode parity.
- `ui/src/i18n/{en,zh-CN}.ts` — the new reason keys + surface strings
  (i18n-parity gate).
- UI surfaces (new component files, minimal single-line mounts in the
  existing screens; screens currently under the defect lane's active
  work get their mount landed only after the defect lane's fix for that
  screen is green): the sector-map panel, the Create Image dialog, the
  recoverability summary, the scan telemetry drawer.

## Out of scope for lane C (recorded, not hidden)

- `cr_ui_image_cancel` (engine ABI gap) — needs a handle-ized engine
  export; engine/** is the defect lane's active surface this session.
  Registered for the next session in BEHAVIOR-FINDINGS.
- `.edisk`/`.dmg` image formats (the og's pair) — the engine writes flat
  `.img` + `.sha256` only (PARITY-DELTAS D-147 stands).
- TRIM chip at enumerate (`CR_ENUM_FLAG_TRIM`) — touches devices.rs
  (the defect lane's active file: the enumeration defect is theirs);
  lane C takes it after their fix lands, as the chip's one-fsutil-spawn
  latency was the original DISABLE reason and the opt-in flag design
  already answers it.
- Core-side query engine adoption (§4.7 row 24) — unchanged posture.

## Push protocol (lane C)

Fetch-first before every push; rebase on the defect lane's commits;
small coherent units; never force-push; main green at every push (the
lane-B clang-format lesson applies: run the local gates — prettier,
eslint, tsc, the contracts validator — before every push).
