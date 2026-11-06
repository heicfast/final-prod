# DIAGNOSIS — the entry-point consolidation (T3)

Inputs: the six Phase-0 sheets (REPO-STATE, UI-MAP, CORE-API, WIRING-AUDIT,
REFERENCE-BEHAVIOR, INSTALLER-AND-PRIVILEGE) + the lead's own verification
pass (worklog.md T1 lane B). Every claim below carries its citation in the
sheets; this file is the fix-ordering decision record.

## The one-sentence picture

CI is green and the product is broken anyway: the CI lanes never asserted
launch elevation, console silence, consumer-hardware enumeration, or
scan-stage throughput, and the shipped exe carries **asInvoker** while four
separate docs *claim* requireAdministrator — a documentation hallucination
(INTEGRATION-PLAN §7.4 decided, never implemented; nsi:27-29 restates it).

## Defect register (root causes, confirmed at code level)

### D1 — Privilege model (no elevation at launch)
- **Root cause:** no application manifest exists anywhere (0 `.manifest`
  files, no requestedExecutionLevel, no mt.exe; `ui/src-tauri/build.rs:38`
  calls plain `tauri_build::build()`). Tauri 2 default = asInvoker.
- Current posture: amber banner + ShellExecuteW "runas" relaunch
  (`commands/system.rs:85-124`, `features/consent/elevation-banner.tsx`).
- **Reference:** DRW.exe AND DRWUI.exe embed requireAdministrator
  (corpus `rev/DRWUI.exe/strings_ascii.txt:41023`); privileged work rides
  a LocalSystem service so no mid-session UAC fires.
- **Fix:** `app.manifest` with requireAdministrator wired through
  tauri-build `WindowsAttributes::app_manifest`; banner removed (edge case
  gone by construction); forensic probe + docs updated to match reality;
  the unelevated-twin probe redesigned (a requireAdministrator exe cannot
  be launched by CreateProcessWithLogonW standard user).

### D2 — Visible console on disk health
- **Root cause:** `engine/src/win/cr_health.cpp:306`
  `_popen("fsutil behavior query disabledeletenotify 2>nul","r")` — _popen
  always creates a console in a GUI-subsystem host; the engine DLL is
  in-process in the Tauri app. Reached on EVERY health read:
  health.rs:19 → cr_ui_health_read → query_trim_state (unconditional,
  cr_ui.cpp:4946-4951).
- BF-04's "bridge unaffected" was wrong; FORENSIC-PROBE.md:90 ("replaced
  with native IOCTL") is stale — the _popen is live.
- **Reference:** fsutil runs hidden (CreatePipe+CreateProcessA,
  wShowWindow=SW_HIDE, `DRWUI_decompiled.c:178726-178733`).
- **Fix:** per-device `IOCTL_STORAGE_QUERY_PROPERTY` /
  `StorageDeviceTrimProperty` (no subprocess at all, no localized-output
  parsing — kills HEALTH-6's fragility too); deliberate divergence from
  the og's hidden-fsutil documented in PARITY-DELTAS (same user-visible
  truth, stronger vehicle). CI gains a tree-wide guard: no `_popen`/`system(`
  in shipping engine code.

### D3 — One disk of three
- **Root cause class:** both enumeration stacks silently DROP a present
  disk whose open fails with anything other than ACCESS_DENIED/
  SHARING_VIOLATION, or whose geometry query fails
  (cr_enumerate.cpp:98-127, header doc "A disk that cannot even be
  metadata-opened stays absent"). The og lists devices WITHOUT opening
  them (five-stack fan-in, `mom-module-system--architecture.md:93`).
- NOT reproduced on CI (FORENSIC #29: 4 drives selectable on seeded
  vdisks) — user hardware has a device our open-based stacks refuse.
- **Fix (two layers):** (1) D1 removes the entire ACCESS_DENIED class
  (elevated opens succeed); (2) never drop a PRESENT disk interface —
  when both opens fail, still list the disk with cfgmgr-sourced identity
  (friendly name/model/size via CM_Get_DevNode_Registry_Property /
  interface instance metadata), `table_ok=0`, honest "unavailable" status;
  the scan attempt then reports the truthful open error. This matches the
  reference's "list every present device" contract.

### D4 — Scan "stuck at 75%, 21 files (0 B)"
Decomposed into four root causes (three wiring/engine truth gaps + one
throughput wall):

- **D4a — frozen sector counter:** `tel_.snap.bytes_scanned` is stamped
  ONLY at stage boundaries (cr_scan.cpp:535 tel_counts_; carve bytes land
  in stats at cr_scan.cpp:2662 after carve_slice returns). The event
  stream DOES carry live carve bytes (`e.bytes_done`, cr_scan.cpp:596)
  but the bridge emitter reads only the permilles (events.rs:266-273).
  "Reading sector: X/Y" freezes for the whole deep+carve span.
  **Fix:** bridge consumes evt.bytes_done/bytes_total for stage 2 (deep
  too — engine side gives deep byte units in the same field), and the
  engine stamps live bytes into telemetry at every publish.
- **D4b — "0 B" found size:** no `bytes_found` in any ABI struct; the
  engine has the datum (ScanStats.bytes_found = tree_.total_logical(),
  N-167) but it crosses only at terminal (scan.rs:419-434 re-sums the
  snapshot). Mock connector computes it live — masking the gap in e2e.
  **Fix:** W1c-precedent ABI append (cr_ui_event + cr_ui_telemetry gain
  bytes_found), engine stamps it live (atomic tree counter), bridge +
  UI render it live. Reference: count+size grow live
  (`dynamic-ground-truth--scan-phase-captured.md:102`).
- **D4c — cancel latency:** inside the carve, cancel checkpoints exist
  per 1-MiB window only; a pathological validator window grinds minutes
  past a stop request (BF-25 measured 180 s+ on a healthy 8 GB exFAT).
  **Fix:** checkpoint + progress tick inside scan_one_window's per-byte
  loop at bounded cadence; heartbeat stays liveness-only.
- **D4d — the throughput wall:** carve is validation-CPU-bound —
  8.45 MiB/s effective vs 544 MiB/s device (PERF-LEVERS wall math, run
  41140212296); 76-124 s/GiB. On consumer multi-TB disks the last 25%
  (carve weight) runs hours-to-days: the user's "stuck". The 75% jump
  itself is legitimate (quick 40% + deep 35% complete fast on a healthy
  volume; carve holds 25%).
  **Fix path this session:** (1) verify the S-009 free-map gate actually
  receives a valid map from every FS engine (an exFAT/NTFS engine that
  leaves stats.free_map invalid nulls the gate → carve sweeps USED space
  at CPU cost — suspected, to be proven by test); (2) safe output-identical
  matcher wins only if provable; (3) whatever remains is BEHAVIOR-FINDINGS
  scope with the measured evidence (PERF-LEVERS §3.1 was already scoped as
  dedicated perf work). D4a/b/c make the wall VISIBLE and HONEST (moving
  counter, live size, real ETA, instant stop) even where throughput stands.

### D5 — Restart-as-admin prompt theming
Subsumed by D1 (with launch-elevation the prompt cannot occur). The
amber Tailwind banner (elevation-banner.tsx:27) is removed with D1;
`system_security_posture` stays as the truthful About/Settings datum.

## Beyond the register (highest-impact finds from the sheets)

- **User-sign import always fails** — Settings passes file.name instead
  of the document text (settings-page.tsx:253 vs contract.ts:764-767).
- **"Open Location" inert** — menu item enabled, no onClick
  (results-explorer.tsx:476-484); system_open_path implemented, never
  called.
- **About dialog "Mock" badge mislabel** — system_app_info omits
  connectorMode (system.rs:133-147).
- **6/10 bridge events have no frontend listener** (save/load progress,
  lost-candidate, autosaved, scan/recovery-finished) — feedback survives
  only via polling.
- **Registries append-only** — jobs/scans never released (recovery.rs:282,
  scan_close gating) → handle/thread leak in long sessions; scan_close
  mid-drain race + emitter Arc gap (state.rs:291-327).
- **Banner hides silently when the posture probe errors**
  (elevation-banner.tsx:21, retry:false) — goes away with D1 anyway.
- **SD cards render 'usb'** — no bus_type axis (BF-02); SD page's
  kind==='sd' can never match.
- **~420 hardcoded hex colors** in the UI vs the token system — the
  visual-identity lane (T16).

## Phase-1 reproduction plan (evidence before claim)

1. **CI-side (forensic-repro, Windows runner):** add a Phase-1 probe run
   that captures, on the seeded multi-vdisk runner: (a) the manifest
   level of the shipped exe (before/after D1); (b) a console-window
   sentinel during a health read (before/after D2 — enumerate visible
   windows owned by the process tree during the operation); (c) the
   device list vs the OS's own view (Get-Disk comparison) including a
   hot-added offline/locked vdisk to exercise the D3 drop class; (d) a
   scan transcript on the 8 GB seed with per-second samples of
   overall/bytes_scanned/bytes_found/cancel-latency (before/after D4).
2. **User-machine parity notes:** the runner cannot reproduce the user's
   exact second internal drive; the D3 fix is written against the DROP
   CLASS (never drop a present interface), not a specific device.

## Fix order (this session)

1. D1 manifest (+ probe/doc truth alignment)
2. D2 TRIM IOCTL (+ no-_popen CI guard)
3. D4a/D4b live bytes + found size (ABI append, engine stamp, bridge, UI)
4. D4c carve cancel checkpoints
5. D3 never-drop enumeration
6. The wiring quick-wins (user-sign import, open-location, connectorMode)
7. D4d evidence pass (free-map validity per FS engine) + what lands safely
8. CI gates for all of the above; release; live validation
