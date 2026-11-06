# DIAGNOSIS — the entry-point consolidation (Phase 0 → Phase 1)

The six Phase 0 context sheets are the picture of the product as it exists
(REPO-STATE, UI-MAP, CORE-API, WIRING-AUDIT, REFERENCE-BEHAVIOR,
INSTALLER-AND-PRIVILEGE — all evidence-cited path:line). This sheet
consolidates them into the defect register that Phase 1 reproduces and
Phase 2 fixes at the root. Defect IDs are stable across the session.

## The user's report, mapped to root causes

### D-PRIV-01 — the app does not request administrator at launch

**Root cause (verified, three sheets agree):** the shipped exe carries NO
application manifest of its own. `ui/src-tauri/build.rs:38` calls bare
`tauri_build::build()`; upstream tauri-build's stock embed contains only
the Common-Controls dependency — no `requestedExecutionLevel` at all →
the loader default `asInvoker`. Elevation today = the in-app banner +
`ShellExecuteW "runas"` relaunch (`commands/system.rs:84-124`) — the
lazy, secondary flow the user rejects.

**Documentation hallucination (3 sites):** `recoverywizard.nsi:27-29`,
`docs/INTEGRATION-PLAN.md:943` (§7.4 "ADOPTED requireAdministrator"),
`docs/INSTALL-FOOTPRINT-CURRENT.md:205` all claim the app runs elevated.
Only the release notes tell the truth.

**Why the gates never caught it:** the forensic probe launches the app
pre-elevated (`launch-app.ps1:50-54`), the installer finish-page launches
it as an elevated child (`nsi:164-165`), and the unelevated twin EXPECTS
the banner — the gates codify asInvoker as correct.

**Reference standard (corpus):** DRW.exe ships `requireAdministrator`;
UAC fires ONCE at launch; the loader then spawns DRWUI elevated with
inherited handles — no second prompt, no restart-as-admin flow anywhere
in the reference decompile.

**Fix direction:** embed a `requireAdministrator` manifest in the Tauri
exe (custom manifest through the build, verified in the artifact), retire
the banner to a true edge case, re-truth the three lying docs, and
re-vehicle the forensic twin lane to the new posture.

### D-CONS-01 — visible console shells when opening disk health

**Root cause (verified):** `engine/src/win/cr_health.cpp:306` —
`_popen("fsutil behavior query disabledeletenotify 2>nul", "r")`. In a
GUI-subsystem process `_popen` spawns `cmd.exe` with a VISIBLE console
window — one flash per health read, on every drive queried
(`cr_ui_health_read` calls it unconditionally, `cr_ui.cpp:4948`).

**It is a regression:** the 0.6.x era fixed this natively (NG-8); the
engine re-merge (283a5d90) swapped in core_recovery's `_popen` version.

**No other visible spawn exists (verified census):** chkdsk
(`cr_fsck.cpp:50-51`) and PowerShell/manage-bde (`cr_bitlocker.cpp:64-65`)
already use `CREATE_NO_WINDOW` — the correct in-tree pattern to copy.

**Fix direction:** replace `_popen` with `CreateProcessW` +
`CREATE_NO_WINDOW` + piped stdout, identical parsing; add a regression
gate (app-lifetime console-window census in the probe).

### D-ENUM-01 — only one of three disks appears

**Root cause (verified, host-side):** the Home device list renders ONLY
`kind==='internal'` rows (`home-page.tsx:80-83`), while the bridge
degrades every removable-bus disk (USB/SD/MMC) to `kind:'usb'`
(`devices.rs:215`, BF-02), and the removable view (`/?media`) is
unreachable — no navigation exists to it. Engine-side collapse is ruled
out (merge dedups by disk number + PnP path only, `cr_enumerate.h:228-255`).
Any disk the bus mapper calls non-internal — an SD card, an external,
and any internal drive whose bus reports as removable/unknown — is
present in the payload but INVISIBLE in the default Home view. On the
user's machine (2 internal + 1 SD), the visible count depends on the bus
classification of each drive: the symptom "only one disk" follows
directly.

**Reference standard (corpus):** the landing page groups
"Internal drives (N)" / "External drives (%1)" — BOTH groups visible,
cards with icon + name + used/total capacity; SD cards are first-class
(a dedicated sidebar page); enumeration = WMI Win32_DiskDrive + mountmgr
volume extents.

**Fix direction:** reference-parity device list — both groups rendered,
SD/removable first-class, kind mapping audited against real bus types,
and a probe gate that asserts the UI's rows match the OS's disk census.

### D-SCAN-01 — "jumps to 75% advanced scanning, files found 21 (0 B),
no sector progress, all stuck"

Four stacked root causes, each verified:

1. **75% is exact math, not a fake number:** engine stage weights are
   40/35/25 (quick/deep/carve, `cr_scan.h:72-75`); quick+deep complete ⇒
   overall = 750‰ exactly. The quick pass runs `covers_both=true` and
   publishes quick AND deep permilles together (`cr_scan.cpp:722-724,
   1597`). The UI maps deep+carve to the same "Advanced Scanning" label
   (`contract.ts:171-175`, `scan-progress-page.tsx:179-181`). So a
   healthy scan shows exactly "Advanced Scanning 75%" at the carve
   boundary — and a stalled carve shows it FOREVER.

2. **The stall is real engine behavior (BF-22, upgraded from
   "environmental" to defect):** the carve matcher grinds per-byte over
   1 MiB windows (`cr_carve.cpp:45,597`) with progress/bytes ticking only
   per COMPLETED window (`cr_carve.cpp:1530-1566`), no intra-window
   checkpoint, and in-window reads served by memcpy — device IO idles,
   percent/files/bytes freeze, and the 5 s heartbeat does NOT re-emit
   progress (`events.rs:326-329`). The user's machine and the Azure
   runner show the identical signature (75%→76.1% over 15+ min).

3. **"21 (0 B)" — the count is real, the bytes are a designed-in seam
   gap:** `filesFound` is the live engine counter (events.rs:275/361);
   `dataFoundBytes` is pinned 0 for the entire live scan (G-3,
   `state.rs:107-109`) and only latched at completion by walking the
   frozen snapshot (`scan.rs:416-443`) — NO ABI event carries found-bytes
   (the engine computes `stats.bytes_found`, `cr_scan.cpp:529`, but only
   the session persists it). The reference shows cumulative GB live
   (corpus: status bar `Files Found: %2` count + GB).

4. **"No sector progress":** `bytesScanned` is carve-true-bytes ONLY —
   zeroed for stages 0/1 (`cr_scan.cpp:596-597`) — so the
   "Reading sector" row reads 0/N through quick+deep, then freezes in
   the carve stall. The reference runs a live "Reading sector: X/Y"
   ticker throughout.

**Reference standard (corpus):** Quick Scan auto-continues into Advanced
Scan which runs its OWN 0–100% (moving), with files-found count +
cumulative GB, remaining-time estimate, sector ticker, "Searching:"
path ticker, Pause/Stop, and a live-growing results tree
(browse-while-scan; the Reconstructed/carved branch grows in real time).

**Fix direction (root, no fake values):** intra-window carve progress +
cancellation checkpoints (kills both the frozen percent and BF-25's
unbounded drain); found-bytes telemetry through the ABI (live, real);
bytes-scanned across all read stages; percent/stage semantics aligned to
the reference's moving advanced-phase display; results tree growth while
scanning.

### D-UI-01 — the restart-as-admin prompt is visually unacceptable

**Root cause (verified):** `elevation-banner.tsx:25-45` hardcodes raw
Tailwind amber classes (`bg-amber-50/border-amber-300/text-amber-900`),
bypassing the semantic `--warning` tokens (`styles/tokens.css:71-73`)
every other banner uses — clashing with the `#EBF2FF`/`#1677FF` chrome.
With D-PRIV-01 fixed the banner becomes a true edge case; any residual
prompt is re-styled on tokens.

## Secondary defects (Phase 0-found, to validate and fix)

From WIRING-AUDIT + UI-MAP (all file:line-cited in the sheets):

- **W-01 dead-letter events:** 5 bridge events have NO frontend listener
  (scan-finished, save-progress, lost-candidate, session-load-progress,
  scan-autosaved).
- **W-02 no exit-save:** window close mid-scan saves nothing (no exit
  handler in `lib.rs`).
- **W-03 mislabeled errors:** health/open failures label
  `device-permission`, never `elevation-required` (engine returns
  ERR_OPEN_FAILED unelevated at one site vs ERR_ELEVATION_REQUIRED at
  others — inconsistent gate, `cr_ui.cpp` sites in CORE-API).
- **U-01 false "mock" badge:** About dialog shows a mock badge because
  real `system_app_info` lacks `connectorMode`.
- **U-02 dead "Open location":** context item never calls `openPath`.
- **U-03 user-signs import:** passes `file.name` instead of contents.
- **U-04 recovery space estimate:** broken by the live 0 B
  (recovery-page.tsx:75-82).
- **E-01 volume enumeration buffer:** 64-wchar
  `GetLogicalDriveStringsW` buffer fails wholesale at 16+ letters
  (`cr_enumerate.cpp:216-218`).

## Phase 1 plan (reproduction on real Windows)

The forensic-repro lane is the reproduction vehicle: it installs the
SHIPPED v0.8.4 release on a windows-2022 runner with 3 physical disks
(OS + seeded 64GB NTFS + 8GB exFAT vdisks), captures every child
process + top-level window at 400 ms (D-CONS-01 evidence), drives the
real UI via CDP (D-ENUM-01 DOM evidence vs the OS disk census), and
records the full scan timeline (D-SCAN-01 percent/sector/counter
evidence). Dispatch on v0.8.4; curate artifacts into
`worklog/REPRODUCTION/<defect-id>/`.

One known gap: the v0.7.0-era probe asserts the OLD posture (banner
expected, pre-elevated launch) — reproduction runs are expected RED on
the gates that codify asInvoker; the raw evidence artifacts are what
Phase 1 needs. The probe is re-vehicled in Phase 2 to gate the NEW
posture (manifest elevation, no console, complete device rows, moving
scan progress).
