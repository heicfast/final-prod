# INSTALLER-AND-PRIVILEGE — the product's privilege posture, end to end (research only)

> Task T2-F (Agent F, installer/privilege lane). READ-ONLY analysis of
> `/home/z/my-project/final-prod` (v0.8.4 tree). Nothing modified except this
> sheet. Every claim cites `file:line`; claims not verifiable in this tree are
> marked **UNVERIFIED-LOCALLY**.

## VERDICT

The **installer** elevates (`RequestExecutionLevel admin`, installer/recoverywizard.nsi:151) — one UAC at setup launch, correct for a per-machine install. The **installed app does NOT elevate**: `recovery-wizard-ui.exe` is built by Tauri 2's default pipeline (ui/src-tauri/build.rs:38 calls `tauri_build::build()` with zero attributes); there is **no `.manifest`, no `.rc`, no `mt.exe` step, no `requestedExecutionLevel` anywhere in the UI build** (repo-wide glob `**/*.manifest` = 0 hits; repo-wide grep hits only docs/corpus/probe code). The exe therefore carries Tauri's default embedded manifest — **asInvoker** (upstream default; exact 2.7.4 text UNVERIFIED-LOCALLY, §2.2/§5.1). The engine's raw-disk gates then refuse everything (`ERR_ELEVATION_REQUIRED 0x500C0001`), the amber off-theme banner appears, and elevation happens only via the secondary "runas" relaunch. The NSIS header claim "the app itself runs elevated (requireAdministrator)" (nsi:29) is unsupported — the parallel session's D-PRIV finding is confirmed (worklog.md:2173-2182). The disk-health console flash is the engine's single `_popen("fsutil …")` (cr_health.cpp:306) — the only un-suppressed child spawn in the product (§4.1).

## 1. The installer

### 1.1 recoverywizard.nsi — pages, privilege, files, uninstaller

- **Identity/output**: `Name "HeicFast Recovery Wizard ${PRODUCT_VERSION}"` (nsi:133), `OutFile …RecoveryWizard-<v>-setup.exe` (nsi:134), `InstallDir "$PROGRAMFILES64\HeicFast\Recovery Wizard"` (nsi:135); app binary `recovery-wizard-ui.exe` = Cargo package name (nsi:127; no mainBinaryName override in tauri.conf.json).
- **Privilege**: `RequestExecutionLevel admin` (nsi:151) → NSIS embeds a requireAdministrator manifest **in the installer exe**. All-users only: `SetShellVarContext all` in `.onInit` (nsi:406), SHCTX=HKLM (nsi:147-149). Win10+ registry gate (nsi:394-402). The nsi:27-29 comment "the app itself runs elevated (requireAdministrator)" is **false for the app** (§2).
- **Pages** (nsi:161-174): MUI Welcome → License (MIT, nsi:168) → **custom privilege-information page** (nsi:169, 308-330: three static labels — user-mode product / raw-disk needs administrator / the installer itself elevates once; informational, no choice, `PrivilegePageLeave` records nothing nsi:328-330) → Directory → InstFiles → Finish with `MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"` (nsi:164-165).
- **SEC_APP** (nsi:180-263, RO): stages the inner Tauri NSIS bundle to `$PLUGINSDIR\app-setup.exe` (nsi:197); seeds the inner setup's install-location key (nsi:208); drives it **silently**: `ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR'` (nsi:210; ExecWait-not-nsExec quoting rationale nsi:186-192); nonzero inner exit aborts (nsi:212-215); integrity + default-dir adoption (nsi:222-231). Engine kit lands **beside the exe** — static-import loader truth (nsi:235-239): `SetOutPath $INSTDIR` + `File /x cr_ui.lib "${ENGINE_KIT}\*"` (nsi:243-244); gates `cr_ui.dll` present (nsi:246) and ≥2 DLLs (nsi:253-262, the 0xc0000135 lesson).
- **"-post"** (nsi:265-302): `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}` (nsi:129-131, 269-270); `HKLM\Software\HeicFast\Recovery Wizard` {InstallDir,Version,InstallMode=AllUsers} with read-backs (nsi:274-285); sweeps stale inner uninstall keys (nsi:291, 333-381); writes the single Add/Remove entry `HKLM\…\Uninstall\Recovery Wizard` with NoModify/NoRepair (nsi:292-299); `WriteUninstaller "$INSTDIR\HeicFastUninstall.exe"` (nsi:301).
- **Logging/abort**: persistent `${PROGRAMDATA_LOGS}\install.log` + `%TEMP%` facts file (InstallFact nsi:87-111); abort reasons to `%TEMP%\…-install-abort.txt` (nsi:78-85) — silent installs show nothing.
- **Uninstaller** (nsi:423-531): close-app check via FindWindow on window text "Recovery Wizard" (nsi:127-128, 432-444, `/SD IDCANCEL`); sessions = user data, kept in silent mode, interactive Yes/No defaults KEEP (nsi:447-469); inner Tauri uninstaller run silently (`ExecWait '… /S _?=$INSTDIR'`, nsi:472-477); kit deletes + `*.dll` catch-all (nsi:484-501); both registry trees + seeded key removed (nsi:504-519); own install.log + empty dirs (nsi:521-530).
- **No console spawns of its own**: all native NSIS ops; the only child is the silent inner setup (docs/INSTALLER.md:30-36). The driver-era certutil/powershell flashes (FORENSIC-PROBE.md:53-71) died with the driver (DRIVER-REMOVAL.md:124-125).

### 1.2 devsign.ps1 / portable.ps1

- **devsign.ps1** (`new-cert|sign|verify`, :48): ephemeral self-signed CodeSigningCert in `Cert:\CurrentUser\My` — works elevated AND unelevated (:119-125); exports .cer/.pfx + random-password + thumbprint into the stage dir (:107-141); `signtool sign /fd SHA256` + store-independent verify (:144-165). Signs **cr_ui.dll, rrecov.exe, the outer installer** (build.ps1:130, 210); the **Tauri app bundle is NOT signed** (docs/DEV-SIGNING.md:19 — the production gap).
- **portable.ps1**: `RecoveryWizard-<v>-portable.zip` = raw `target/release/recovery-wizard-ui.exe` (:32,45) + engine kit beside it (:47) + portable marker `recovery-wizard.portable` (:76) + README whose first instruction is **"RUN AS ADMINISTRATOR"** (:54-60). No manifest — the README is the portable build's only elevation affordance.

### 1.3 tauri.conf.json bundle

`targets: ["nsis"]` (:34); `installMode: "both"` (:45 — the inner setup could do per-user; the OUTER forces `/allusers`, nsi:210); EN+Simplified Chinese + selector (:46-47); `webviewInstallMode: downloadBootstrapper` (:49-51) — the INNER setup downloads+runs `MicrosoftEdgeWebview2Setup.exe` if the runtime is missing (network at install time; INSTALL-FOOTPRINT-CURRENT.md:315, :332 flags "visible UI possible, only when WebView2 is missing" — exact behavior lives in Tauri's bundler template, **UNVERIFIED-LOCALLY**; CI runners ship WebView2 so it is never exercised). Window title "Recovery Wizard" (:16) = the uninstaller's FindWindow target (nsi:128).

### 1.4 Installer elevates vs app elevates

The installer's requireAdministrator covers **file placement only**; it ends when setup exits. The app's token is decided by the **app exe's own manifest** — asInvoker (§2). A Start Menu / Explorer launch is unelevated; the only paths where the current build starts elevated are (a) the Finish-page "Launch" checkbox (executed from the *elevated installer* process, nsi:164-165 — token inherited) and (b) an explicitly elevated manual launch (the portable README instruction, portable.ps1:56-59).

## 2. The app's elevation posture — the definitive audit

### 2.1 What exists in-tree (exhaustive)

- `**/*.manifest` repo-wide: **0 files**. `**/*.rc` repo-wide: **0 files** (Tauri generates its own .rc from `bundle.icon`; no user .rc rides along).
- grep `requestedExecutionLevel|requireAdministrator|asInvoker|mt.exe|app_manifest|WindowsAttributes` repo-wide: hits only in (a) og reference/corpus docs (docs/REFERENCE-FOOTPRINT.md:275-288 — DRW.exe/DRWUI.exe shipped requireAdministrator; engine/worklog/REFERENCE-ACCESS.md:348-359), (b) our own docs' claims (§2.4), (c) the forensic byte-probe (tools/forensic/collect-facts.ps1:32), (d) engine decision records for **rrecov.exe** — engine/worklog/DESIGN-CONSTRAINTS.md:583-589: "NO embedded requireAdministrator manifest on rrecov.exe in-tree (no .manifest, no mt.exe in any CMakeLists) … the exe stays manifestless unless the user approves otherwise (OQ-1)". **None of these embeds anything.**
- `ui/src-tauri/build.rs` (39 lines): cr_ui.lib link wiring (:23-37) then `tauri_build::build()` — **the zero-attribute call** (:38).
- `ui/src-tauri/Cargo.toml`: `tauri-build = { version = "2", features = [] }` (:12); windows-sys probe features (:27-34); no manifest tooling. Cargo.lock pins **tauri-build 2.7.4** (:3436-3439) and **embed-resource 3.0.12** (:845-848 — the rc→RES compiler tauri-build drives; in the graph, unused for any custom manifest).
- `main.rs:1-2`: `#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]` — GUI subsystem in release; **debug builds show a console by design** (dev-only).
- `.github/workflows/release.yml`: builds via `./build.ps1 -Target all` (release.yml:62-67); ui lane = `pnpm tauri build` (build.ps1:165); installer lane = makensis on recoverywizard.nsi (build.ps1:199-201). **No step references a manifest / mt.exe / post-link elevation edit.**

### 2.2 What Tauri 2 embeds by default

tauri-build compiles a generated .rc (icon + version) that includes an application manifest with requestedExecutionLevel **`asInvoker`** — upstream-documented default for tauri-build 2.x. The local Cargo registry (`~/.cargo`) does not exist on this host, so the 2.7.4 source cannot be read: **UNVERIFIED-LOCALLY** for the exact manifest text. Corroborated three ways in-tree: (1) the shipped v0.8.4 app launches from Start Menu/Explorer with no UAC prompt (the user's report, worklog.md:2159-2161); (2) the forensic twin launched the same exe under a genuinely standard-user token and it **ran and rendered the amber banner** (RELEASE-RUN.md:415-420; unelevated-launch.ps1:104-105, 177-179) — a requireAdministrator exe would have failed at CreateProcess instead; (3) the parallel session's own pre-read (worklog.md:2177 "Tauri's default embed is asInvoker").

### 2.3 Definitive answer

**The shipped recovery-wizard-ui.exe does NOT carry requireAdministrator. It is asInvoker.** nsi:29's claim is unsupported by any artifact in the tree.

### 2.4 The documentation-hallucination chain

1. `docs/INTEGRATION-PLAN.md:941-946` — §7.4 decision **(a) "ADOPTED. The app manifest is requireAdministrator (reference parity …)"** — decided, never implemented.
2. `installer/recoverywizard.nsi:27-29` — repeats it as an installer comment.
3. `docs/INSTALL-FOOTPRINT-CURRENT.md:205-206` — "The app itself carries `requireAdministrator` (INTEGRATION-PLAN §7.4 decision (a))".
4. `engine/docs/PACKAGING.md:130` — "the installer's UAC manifest (requireAdministrator) matches" — true for the installer exe, misleading in an app-privilege section.
5. `docs/INSTALLER.md:17-23` + the NSIS privilege page (nsi:320) — the only doc layer that matches reality (installer elevates; app shows banner + restart-as-admin).
6. `worklog/INTEGRATION-PLAN.md:858` planned to verify "the installed app's requireAdministrator manifest fact" — a gate that could never pass.

### 2.5 Byte-level corroboration available to any verifier

The in-tree probe (`collect-facts.ps1:32`) reads the installed exe's bytes and
regex-matches the manifest string; running `strings`-equivalent over
`target/release/recovery-wizard-ui.exe` would show the Tauri default manifest
(asInvoker, DPI awareness, common-controls) — and NOT the og's literal
`<requestedExecutionLevel level='requireAdministrator' uiAccess='false'/>`
that the corpus found in DRWUI.exe (engine/worklog/reference/
og-access-architecture-record.md:123). No such binary exists on this Linux
analysis host (the exe is built on the windows-2022 runner,
release.yml:31) — **UNVERIFIED-LOCALLY** beyond the behavioral proof in §2.2/§6.

## 3. Every elevation point in the codebase (current flow, mapped)

### 3.1 Engine (cr_ui.dll)

- `win::is_elevated()` — TokenElevation probe (cr_elevate.cpp:12-21; contract cr_elevate.h:19-23: false for standard users AND non-elevated admins; probe failure = false).
- `ERR_ELEVATION_REQUIRED = 0x500C0001` (cr_error.h:114-120; ABI-CONTRACT.md:94) — the direct era's single refusal class (cr_transport.h:52-53). Gates: `open_disk_impl` (cr_ui.cpp:1433-1438), `open_volume_impl` (:1488-1493), the scan-run gate (:1576-1577), `image_create_impl` (:1725-1730). Contract text: cr_ui.h:318-329 ("the host triggers its own elevation flow (the standard OS UAC prompt) and retries; nothing elevates silently") + the advisory channel `cr_ui_last_transport_error` (cr_ui.h:333-340).
- `health_read_impl` opens `\\.\PhysicalDriveN` GENERIC_READ|WRITE (cr_ui.cpp:4886-4888); the failure text distinguishes "administrator rights required (elevate and retry)" from "denied even elevated" via the probe (:4889-4894).
- BitLocker: separate `ERR_FVE_ELEVATION_REQUIRED 0x50060002` ACL diagnosis (cr_ui.cpp:1468-1475). CLI twin: exit code 4 (engine/docs/API.md:80; rrecov.cpp:535,557).
- **No relaunch helper in the engine** — elevation mechanics are host-side by design (DESIGN-CONSTRAINTS.md:576-578 "the host process owns its launch model"; rrecov stays asInvoker, :583-589).

### 3.2 Bridge (Rust)

- `is_elevated()` — same TokenElevation probe via windows-sys (system.rs:37-60).
- `system_security_posture` → `{administrator}` (system.rs:70-79; registered lib.rs:84).
- `system_relaunch_elevated` (system.rs:84-124): `ShellExecuteW` verb **"runas"** on the OWN exe (:88-103), `lpParameters = null` (:99 — **no arguments carried**; route/session context is lost), `SW_SHOWNORMAL`; rc ≤ 32 → DEVICE_PERMISSION "the elevation request was refused" (:105-111); on success `app.exit(0)` closes the unelevated instance (:113). This exists **because** the exe is asInvoker.

### 3.3 Frontend

- `useSecurityPosture` (staleTime 60 s, retry:false) + `useRelaunchElevated` (system.ts:42-57) over the connector (tauri-connector.ts:425-428 → `invoke('system_security_posture')` / `invoke('system_relaunch_elevated')`).
- `ElevationBanner` (elevation-banner.tsx:16-47): renders only when posture resolved AND `!administrator` (:21-22); mounted at the top of the main content area on **every route** (shell.tsx:33, 350). Visual: `role="alert"`, ShieldAlertIcon, **amber** palette — `border-amber-300 bg-amber-50 text-amber-900` (:27) + amber-bordered secondary button (:34) — while the product shell is blue (`#EBF2FF` frame, blue accent `rgba(22,119,255,0.04)` card, shell.tsx:341-343): the user's "weird popup colour". Copy: "Direct disk access requires administrator privileges — scanning and recovery are disabled until the app runs elevated." + "Restart as administrator" (en.ts:486-488; zh-CN.ts:473-474). Refusal toast: "Administrator privileges are required for direct disk access. Restart the app as administrator." (en.ts:536-537).

### 3.4 The full current flow

1. Double-click/Start Menu → **launches unelevated** (asInvoker, no UAC).
2. Boot → posture probe → TokenElevation false.
3. Amber banner renders on every page.
4. Any raw-disk verb (open/scan/recover/health/image) refused at the engine gate with 0x500C0001 → toast.
5. Click "Restart as administrator" → ShellExecuteW "runas" → **standard UAC consent** (the only acceptable interruption in this design).
6. Elevated instance starts (fresh state, no args); old instance exits (`app.exit(0)`, system.rs:113); banner disappears.

## 4. Every visible window/console the product can show

### 4.1 THE console flash: engine `_popen` fsutil (confirmed)

- Source: `query_trim_state` — `_popen("fsutil behavior query disabledeletenotify 2>nul", "r")` (cr_health.cpp:306; function :301-323). `_popen` spawns **cmd.exe**; a GUI-subsystem parent (main.rs:2) has no console, so the child allocates a NEW VISIBLE console → the flash. It is the **only** `_popen`/`system()` in the engine (grep engine/src: 1 hit).
- **Path A (what users hit)**: Health page → `health_read` (health.rs:19) → `cr_ui_health_read` (health.rs:35, 64) → `health_read_impl` **unconditionally** calls `query_trim_state` per report (cr_ui.cpp:4946-4951) → cmd.exe flash. `trimEnabled` rides to the UI at health.rs:147. **Every disk-health read flashes a console.**
- **Path B (latent)**: `cr_ui_enumerate_disks_ex(flags)` calls the same query when `CR_ENUM_FLAG_TRIM` (0x1) is passed (cr_ui.cpp:1843-1846; flag cr_ui.h:432; unknown bits refused :1937-1939). The bridge declares the ex export + flag (abi.rs:200, 715-716) but its util layer calls the plain `cr_ui_enumerate_disks` (util.rs:158-166) — the TRIM opt-in is NOT taken (RELEASE-VALIDATION/CAPABILITY-MAP.md:31: "TRIM chip at enumerate — DISABLE — one fsutil spawn per refresh avoided"). Today only the health path flashes.
- Captured evidence: io-timeline.jsonl's `children` recorded `cmd.exe /c fsutil behavior query disabledeletenotify 2>nul` during health reads (docs/FORENSIC-PROBE.md:87-91, run 40465852724; io-probe watches exactly this — io-probe.ps:2, 58-60). **FORENSIC-PROBE.md:90-91's "(now replaced with the native IOCTL)" is STALE** — the current tree still has the `_popen` (cr_health.cpp:306) and no IOCTL TRIM query exists (callers: cr_ui.cpp:1846, 4948; rrecov.cpp:807, 2804).

### 4.2 Correctly suppressed spawns (the in-tree counter-pattern)

- chkdsk: `CreateProcessW … CREATE_NO_WINDOW` (cr_fsck.cpp:50-51, WoW64 System32 redirection :39-54).
- BitLocker unlock/off: `run_command` → `CreateProcessW … CREATE_NO_WINDOW` (cr_bitlocker.cpp:64-65) for `powershell -NoProfile -NonInteractive … Unlock-BitLocker/Disable-BitLocker` (:358-360, 376-381) and the `manage-bde` fallbacks (:360, :381) — invisible despite powershell.exe being the child.
- The bridge/frontend spawn nothing else (system.rs ShellExecuteW is the only launcher; "runas" shows UAC, not a console). rrecov.exe is a console-subsystem CLI by design (engine/src/app/CMakeLists.txt:1 — no WIN32 flag); shipped in the kit but terminal-only, so not a product flash.

### 4.3 UAC prompts (standard, acceptable)

(1) Installer launch — nsi:151 (silent `/S` also needs the elevated context); (2) the app's "runas" relaunch (system.rs:88-103); (3) if the app ships requireAdministrator (§5), every app launch becomes a prompt site.

### 4.4 Installer/launcher surfaces

- Full-UI install: MUI wizard pages (nsi:167-172) — standard windows, no consoles; the inner setup runs silently inside InstFiles (nsi:210; docs/INSTALLER.md:30-36; INSTALL-FOOTPRINT-CURRENT.md:332 "No window — inner Tauri/NSIS setup is GUI-subsystem and silent").
- Silent install: `setup.exe /S` (docs/INSTALLER.md:37-38; CI: install-probe.ps1 `-Silent`, forensic-repro.yml:100, install-probe.ps1:70). MessageBoxes carry `/SD` so silent never blocks (nsi:397-401, 437-439, 457-461).
- WebView2 bootstrapper (tauri.conf.json:49-51): downloads+runs the Microsoft bootstrapper when the runtime is absent — visible-UI possibility flagged at INSTALL-FOOTPRINT-CURRENT.md:332; exact behavior in Tauri's bundler template (**UNVERIFIED-LOCALLY**).
- Forensic tooling's own hidden wrapper cmd (unelevated-launch.ps1:92-98, 104 `-WindowStyle Hidden`) is diagnostic, not product.

### 4.5 The complete visible-surface matrix

| # | Surface | Trigger | Visible? | Source |
|---|---|---|---|---|
| 1 | cmd.exe console flash | **any disk-health read** (path A) | **YES — the reported bug** | cr_health.cpp:306 `_popen` |
| 2 | cmd.exe console flash (latent) | enumerate with CR_ENUM_FLAG_TRIM — not taken by the UI | no (today) | cr_ui.cpp:1845-1846; util.rs:158-166; CAPABILITY-MAP.md:31 |
| 3 | chkdsk | repair check-phase | no — CREATE_NO_WINDOW | cr_fsck.cpp:50-51 |
| 4 | powershell.exe / manage-bde | BitLocker unlock / turn-off | no — CREATE_NO_WINDOW | cr_bitlocker.cpp:64-65, 358-381 |
| 5 | UAC — installer | every setup launch | yes (standard) | nsi:151 |
| 6 | UAC — app relaunch | "Restart as administrator" click | yes (standard) | system.rs:88-103 |
| 7 | UAC — app launch | (future) requireAdministrator manifest | yes per launch | §5.1 |
| 8 | NSIS wizard | interactive install | yes (standard pages) | nsi:167-172 |
| 9 | inner Tauri setup | inside outer install | no — driven `/S` | nsi:210 |
| 10 | WebView2 bootstrapper | install on a machine without the runtime | **UNVERIFIED-LOCALLY** (possible UI) | tauri.conf.json:49-51; INSTALL-FOOTPRINT-CURRENT.md:332 |
| 11 | app's own console | never in release; debug builds only | no (release) | main.rs:2 `windows_subsystem` |
| 12 | amber elevation banner | every page while unelevated | yes (in-app, off-theme) | elevation-banner.tsx:27; shell.tsx:350 |

## 5. The fix surface (research only — nothing changed)

### 5.1 Embedding requireAdministrator the Tauri 2 way

Replace `tauri_build::build()` (build.rs:38) with the attributes form: `tauri_build::try_build(Attributes::default().windows_attributes(WindowsAttributes::new().app_manifest(include_str!("app.manifest"))))`, where app.manifest is the standard trustinfo manifest with `<requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>`. The pinned toolchain supports the mechanism (tauri-build 2.7.4, Cargo.lock:3436-3439; embed-resource 3.0.12, :845-848) — but the exact `WindowsAttributes::app_manifest` API shape for 2.7.4 could not be read from a local registry (~/.cargo absent): **UNVERIFIED-LOCALLY**; verify against the 2.7.4 docs.rs before landing. The og reference shipped exactly this on DRW.exe + DRWUI.exe (REFERENCE-FOOTPRINT.md:275-288) and decision (a) was already signed off (INTEGRATION-PLAN.md:941-946). CI needs no new step — release.yml's `build.ps1 -Target all` → `pnpm tauri build` (build.ps1:165) picks it up at the next build.

### 5.2 CI consequences

- **Dev loop**: `pnpm tauri dev` elevates every start (UAC on the dev box; debugger attachment across the elevation boundary needs noting).
- **Forensic probes**: launch-app.ps1 (Start-Process from the elevated runner shell, :23-24 — token inherited, still fine) unchanged, but the **unelevated twin breaks by design**: a requireAdministrator exe launched via CreateProcessWithLogonW as standard user `rwprobe` (unelevated-launch.ps1:104) fails with ERROR_ELEVATION_REQUIRED instead of running-and-showing-the-banner — unelevated-probe.mjs's banner/refusal assertions (:74-86, :92-111) must be redesigned (assert the OS-level launch refusal, or keep an asInvoker test binary). RELEASE-RUN.md:415-420 records the current twin as the banner's proof.
- **collect-facts.ps1:32**: the byte-scan regex flips from as-invoker/no-match to `requireAdministrator`; recorded values live in workflow artifacts (forensic-repro.yml:92, 104, 213), not in-tree (**UNVERIFIED-LOCALLY** what the 0.8.4 runs recorded).
- **Banner code path**: banner + `system_relaunch_elevated` become dead-in-practice (keep as a defensive surface for token-filtered edge cases, or retire deliberately). Engine gates + 0x500C0001 stay correct under any manifest.
- **Runner UAC**: `ConsentPromptBehaviorAdmin` on the GitHub Windows image (**UNVERIFIED-LOCALLY** — image policy not in-tree) governs whether elevated-context relaunches prompt; current probes never trigger UAC on the runner (the runner shell is already elevated, validation-evidence.yml:53-69).
- **Alternative on record**: keep the JIT model (asInvoker + banner/relaunch) — the engine lane frames this as a user-approved choice, never silent (DESIGN-CONSTRAINTS.md:587-589, OQ-1). The product requirement (default elevation) points to §5.1.

### 5.3 The console-flash fix surface (companion defect D-CONS)

Replace `_popen` in `query_trim_state` (cr_health.cpp:306) with (a) the in-tree CREATE_NO_WINDOW pattern (CreateProcessW + pipe, as cr_fsck.cpp:50-51 / cr_bitlocker.cpp:64-65) or (b) a native IOCTL_STORAGE_QUERY_PROPERTY TRIM query (what FORENSIC-PROBE.md:90-91 *believed* had landed). Keep callers honest: cr_ui.cpp:1846, 4948; rrecov.cpp:807, 2804.

## 6. The forensic probes' evidence

- **How CI launches the app**: install-probe.ps1 `-Silent` (forensic-repro.yml:100), then **launch-app.ps1** from the elevated runner shell (forensic-repro.yml:115) — direct `Start-Process` (launch-app.ps1:23-24), so the app **inherits the elevated runner token**; the probe records the actual TokenElevation via P/Invoke, labeled "expected true for the main instance" (:50-54, 84-87). io-probe samples children every 2 s for the fsutil/cmd flashes (io-probe.ps1:58-60; forensic-repro.yml:121).
- **The unelevated twin**: fresh standard user `rwprobe`, provisioned WebView2 scratch, `Start-Process -Credential` (unelevated-launch.ps1:64-105), and **asserts the spawned token is genuinely not-elevated** before any assertion (:155-179). The first `runas /trustlevel:0x20000` attempt could not de-elevate on the runner (RELEASE-RUN.md:103-108).
- **What the facts say**: FORENSIC #29 (run 42226613660, v0.8.4) — the twin under a genuinely restricted token showed **the elevation banner** ("Direct disk access requires administrator privileges …") and **the scan-start refusal** (RELEASE-RUN.md:415-420; first green twin :372-374 after the WebView2 user-data fix :360-370). This is behavioral proof the shipped exe does NOT carry requireAdministrator (such an exe cannot run-and-refuse under a standard-user token — CreateProcess fails instead).
- **The manifest byte-probe**: collect-facts.ps1:32 classifies the installed exe bytes as requireAdministrator / as-invoker-or-highestAvailable / no-manifest-match into facts-<label>.json — a probe built precisely for this defect class; values in workflow artifacts, not in-tree (**UNVERIFIED-LOCALLY**).
- Engine-side gate proof in CI: LUA-token `rrecov scan` exits 4 with 0x500C0001; the elevated path passes (engine-ci.yml:336-374; engine/docs/CAPABILITY-MATRIX.md:18).

## 7. Source index (primary files read in full for this sheet)

installer/recoverywizard.nsi (532 L) · installer/devsign.ps1 · installer/portable.ps1 · build.ps1 (lanes) · ui/src-tauri/{tauri.conf.json, Cargo.toml, build.rs, main.rs, Cargo.lock pins} · ui/src-tauri/src/commands/{system.rs, health.rs} · ui/src-tauri/src/bridge/{util.rs, abi.rs} · ui/src/{app/shell.tsx, features/consent/elevation-banner.tsx, queries/system.ts, i18n/en.ts, connector/tauri/tauri-connector.ts} · engine/src/win/{cr_elevate.cpp, cr_health.cpp, cr_fsck.cpp, cr_bitlocker.cpp} · engine/src/ui/cr_ui.cpp (gates + health + enumerate) · engine/include/cr/{cr_ui.h, cr_error.h, win/cr_elevate.h, win/cr_transport.h} · .github/workflows/{release.yml, forensic-repro.yml, engine-ci.yml, validation-evidence.yml} · tools/forensic/{launch-app.ps1, unelevated-launch.ps1, unelevated-probe.mjs, io-probe.ps1, collect-facts.ps1, install-probe.ps1} · docs/{INSTALLER.md, DEV-SIGNING.md, INSTALL-FOOTPRINT-CURRENT.md, FORENSIC-PROBE.md, INTEGRATION-PLAN.md §7.4, REFERENCE-FOOTPRINT.md} · engine/{docs/PACKAGING.md §5, worklog/DESIGN-CONSTRAINTS.md §manifest} · worklog.md:2156-2210 · worklog/RELEASE-VALIDATION/*.md.

## Bottom line

| Question | Answer |
|---|---|
| Does the installer elevate? | Yes — `RequestExecutionLevel admin`, one UAC (nsi:151) |
| Does the installed app elevate by default? | **No** — asInvoker; no manifest anywhere (build.rs:38 default; twin evidence RELEASE-RUN.md:415-420) |
| Where did "requireAdministrator" come from? | INTEGRATION-PLAN §7.4 decision (a) (docs/INTEGRATION-PLAN.md:941-946) — decided, never implemented, then restated by nsi:29 + INSTALL-FOOTPRINT-CURRENT.md:205 + engine/docs/PACKAGING.md:130 |
| Visible consoles | (1) engine `_popen` fsutil on every disk-health read (cr_health.cpp:306 — the user's flash; latent 2nd path CR_ENUM_FLAG_TRIM unused by the UI); (2) WebView2 bootstrapper only-if-missing (UNVERIFIED-LOCALLY); chkdsk + BitLocker powershell/manage-bde are CREATE_NO_WINDOW; UAC prompts standard |
| Default-elevation fix | tauri-build `WindowsAttributes::app_manifest` + requireAdministrator .manifest (tauri-build 2.7.4 pinned, Cargo.lock:3436; API shape for 2.7.4 UNVERIFIED-LOCALLY); redesign the unelevated twin; expect dev-mode UAC |
