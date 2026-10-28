# MAJOR M-105 — DRWService install lifecycle absent and never scored: the og ships a transient, silent, per-use SCM service ("Read FVE Server": AUTO_START / LocalSystem / OWN_PROCESS, /install + /uninstall verbs with per-call SCM error semantics and a stop-gated delete) that the core has no counterpart for — and the one registration that names it (R-83) carries a false disposition note

**Severity:** MAJOR
**Module / Surface:** packaging (modules/driver-install.md — the K71-K78 walk's DRWService slice) / health (BitLocker, modules/health.md F61 lane)
**Parameters affected:** K71 (installer executability & error semantics — per-call SCM error strings, no warning-and-exit-0), K72 (service configuration parity — name/display/type/start/error-control/account/description/ImagePath/deps), K73 (uninstall lifecycle — stop → verify stopped → delete; "Service not installed" no-op), F61-adjacent (elevation posture for the FVE harvest), Q68 (the R-83 gap-row note's truth)
**Corpus references:**
- `re-workspace/artifacts/DRWService-exe--bitlocker-service.md:9-30` (§1 the full install shape: CreateServiceA AUTO_START/OWN_PROCESS/ERROR_NORMAL/ALL_ACCESS, description via ChangeServiceConfig2A, immediate StartServiceA, the AUTO_START-vs-demand correction `:23-25`, uninstall stop→delete summary `:26-27`) + the verification ledger `:100-111`
- `re-workspace/reversed/ghidra-core/DRWService_decompiled.c` (first-hand, LANE-B read): `:1205-1257` (install: `OpenSCManagerA(NULL,NULL,0xF003F)`; exists → uninstall-first → "Service already installed" bail; `GetModuleFileNameA` ImagePath; `CreateServiceA("Read FVE Server","Read FVE Server",0xF01FF,0x10,2,1,path,NULL…NULL)`; `ChangeServiceConfig2A(1,"DRW read registry key value.")`; `StartServiceA`; per-call error strings), `:1266-1330` (uninstall: query probe, "Service not installed" no-op, `0x10024` reopen, `QueryServiceStatus`, stop-if-RUNNING, delete ONLY if `dwCurrentState==1` (STOPPED), "Uninstall service succeed/failed"), `:1337-1374` (SCM handler: "Service stopping...", SetEvent), `:1381-1451` (ServiceMain: `RegisterServiceCtrlHandlerA`, waitHint 3000, "Service starting..."), `:1661-1718` (dispatch: `_stricmp "/install" | "/uninstall" | StartServiceCtrlDispatcherA` service mode)
- `re-workspace/reversed/ci-multitool/core/DRWService/floss_strings.txt:236-264` (verbatim string block: "Read FVE Server", "Service already installed", "DRW read registry key value.", "Install service succeed", "Service not installed", "Uninstall service succeed/failed", "/install", "/uninstall", PDB `…\BitlockerReadFVEService.pdb`) and `:318-329` (the ANSI SCM import set)
- `re-workspace/reversed/DRWUI.exe/strings_ascii.txt:25749-25753` ("\DRWService.exe /install", "\DRWService.exe /uninstall" ×2)
- `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (first-hand): `:430634-430827` (FUN_140270640 — the per-use orchestration: pre-uninstall `:430709` → `CreateNamedPipeA("\\\\.\\Pipe\\DRWBitlocker",…)` `:430715` → `/install` `:430725/:430735` → ConnectNamedPipe/ReadFile `:430737-430742` → auto.txt rewrite `:430781-430798` → Sleep(2000) `:430807` → `/uninstall` `:430808/:430816`) and `:430953-431000` (FUN_140271120 — `CreateProcessA(…, CREATE_NO_WINDOW, …)` + `WaitForSingleObject(hProcess, 10000)`)
- `re-workspace/reversed/DRWService.exe/strings_ascii.txt:201` (manifest asInvoker) + `docs/PARITY-DELTAS.md:1824-1827` (D-60a: DRW.exe/DRWUI.exe = requireAdministrator)
- `architecture/02-kernel-driver-architecture.md:99-105` (the lifecycle frame: "a separate, on-demand service … installed/uninstalled via \DRWService.exe /install /uninstall", "At app exit / driver uninstall, service is stopped and deleted")
- `re-workspace/artifacts/per-binary/DRWService.md:9-25` (identity + capa: create/modify/start service, persist via Windows service)
- `re-workspace/artifacts/UIUX/dynamic-ground-truth--wine-loader-run.md:20,26` (dynamic corroboration: the NT service stack spawning under the loader)
- `re-workspace/artifacts/WinPE/pe-payload-census--buildpe-easeus-x64.md:12` (the service ships in BUILDPE too)
- Absence verifications: `re-workspace/artifacts/per-binary/unins000.md` (whole file — no service content) and `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` (grep service/DRWService/FVE → 0) — no unins000/setup interaction

**Core references:** audited commit `ea1ac86`-lineage (greps re-run at the reconciliation HEAD: still zero code hits). No service-install surface of any kind for a BitLocker helper: `rg 'DRWService'` (excl. diff-to-fix/) hits only comments/docs (`src/win/cr_bitlocker.cpp` header comments, `tests/unit/test_bitlocker_win.cpp:6,211-213`, `docs/PARITY-*.md`, `docs/REFERENCE-*.md`); the only CreateService-family code is the RDA driver path (`.github/scripts/Install-ReleaseDriver.ps1:113`; `installer/core-recovery.iss:53` `#define DriverService "RDA"` + `:201-229` driver-only deploy/teardown); `src/win/cr_transport.cpp:386-410` is an `OpenSCManagerW(CONNECT)` status query only; zero `SERVICE_AUTO_START|LocalSystem` constants in code (verified first-hand, SA-10). `docs/PACKAGING.md` (S-038's 13-parameter contract) carries zero DRWService/FVE content (grep clean). The engine reads FVEAutoUnlock in-process (`src/win/cr_bitlocker.cpp:27-28,450,496` — S-014's FIXED data path); the core deliberately supports non-elevated runs (`include/cr/win/cr_elevate.h:19-23`, D-60a). Trail ownership: `rg 'Read FVE'|'AUTO_START' diff-to-fix/` → SURFACE-INVENTORY only; R-83 registered GAP at `docs/REFERENCE-API-GAP.md:159,425` with the false note "install lives in driver-repo CI scripts" (those scripts install the RDA driver, not "Read FVE Server" — verified); Group K lists the DRWService artifact as a derivation source (`diff-to-fix/PARAMETERS.md:153`) but `modules/driver-install.md` scores K71-K73 for the RDA-driver slice only.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og ships a complete, self-contained SCM service lifecycle for BitLocker FVE key access: a WinMain dispatching three modes (/install, /uninstall, SCM service mode), a CreateService shape pinned parameter-by-parameter ("Read FVE Server" name+display, WIN32_OWN_PROCESS, SERVICE_AUTO_START, ERROR_NORMAL, SERVICE_ALL_ACCESS, LocalSystem, no deps, ImagePath = its own module path, description "DRW read registry key value."), replace-on-install semantics (uninstall-first, "Service already installed" bail), immediate StartService, per-call SCM error strings, and a stop-gated uninstall (delete only from STOPPED; clean no-op when absent). DRWUI orchestrates it per key fetch — uninstall → install → pipe harvest → auto.txt snapshot → 2 s sleep → uninstall — as a silent CREATE_NO_WINDOW child, so the AUTO_START type is transient in practice. The core has none of this: no service, no verbs, no orchestration; its engine reads the same registry in-process. The K71-K73 parameters that own this slice were only ever walked for the RDA driver, and the one registration that names it (R-83) carries a factually wrong disposition note.

## What the reference does

**Service configuration (the CreateServiceA shape, `DRWService_decompiled.c:1233-1244`):** service name AND display name "Read FVE Server"; desired access SERVICE_ALL_ACCESS (0xF01FF); service type 0x10 (WIN32_OWN_PROCESS); start type 2 (SERVICE_AUTO_START — the artifact's own correction at `:23-25`: "the service is installed SERVICE_AUTO_START, not demand-start. Once installed it is present at every boot; its run body harmlessly idles when DRW isn't running"); error control 1 (SERVICE_ERROR_NORMAL); binary path = `GetModuleFileNameA(NULL,…)` (its own current location — no System32 copy); group/tag/dependencies all NULL; account NULL → LocalSystem; description "DRW read registry key value." via `ChangeServiceConfig2A(svc,1,…)`.

**/install (FUN_140002550, `:1192-1261`):** SCM open ALL_ACCESS; `OpenServiceA` probe — if the service exists, run the uninstall routine FIRST, re-probe, and only if it STILL exists print "Service already installed" and abort (uninstall-then-reinstall repair semantics, hard bail if the old instance survives). Fresh install: CreateService → description → immediate StartService. Per-call error/success strings (OutputDebugStringA): "OpenSCManager failed" / "CreateService failed" / "StartService failed" / "Install service succeed".

**/uninstall (FUN_1400027b0, `:1266-1330`):** absent → "Service not installed" clean no-op; reopen with DELETE|STOP|QUERY_STATUS; QueryServiceStatus; if RUNNING → ControlService(STOP); the delete gate (first-hand decompile reading): `DeleteService` runs ONLY when the status reports STOPPED — a stop still pending skips the delete silently; "Uninstall service succeed/failed".

**Dispatch (FUN_140003110, `:1661-1718`):** whole-commandline `_stricmp` — "/install" → install; "/uninstall" → uninstall; anything else → service mode via `StartServiceCtrlDispatcherA` with `SERVICE_TABLE_ENTRY{"Read FVE Server", ServiceMain}`.

**Run mode (ServiceMain `:1381+`; handler `:1337-1374`):** RegisterServiceCtrlHandler; START_PENDING with waitHint 3000; manual-reset stop event; on SERVICE_CONTROL_STOP: "Service stopping..." + SetEvent; after the one-shot key push: WaitForSingleObject(stopEvent, INFINITE) → STOPPED. Tracing is OutputDebugStringA only — no event-log channel.

**DRWUI's per-use orchestration (FUN_140270640, `:430634-430827`):** per key fetch — (1) `/uninstall` pre-clean no-op; (2) CreateNamedPipe `\\.\Pipe\DRWBitlocker`; (3) `/install` + launch (silent child, CREATE_NO_WINDOW, ≤10 s wait — FUN_140271120); (4) ConnectNamedPipe → ReadFile(0xC800) → 0x138-stride parse; (5) auto.txt delete + rewrite (N-097's scope); (6) Sleep(2000); (7) `/uninstall` teardown. Consequence: AUTO_START is transient in practice — the service is deleted ~2 s after each harvest (the artifact's "present at every boot" describes only the uninstall-failed case).

**Packaging facts:** asInvoker manifest (elevation inherited from the requireAdministrator parents — D-60a); ships in the live tree AND BUILDPE; 40,168 bytes, GUI subsystem, PDB `BitlockerReadFVEService.pdb`; ANSI SCM import set (Win7-safe); NO unins000/setup interaction (both verified zero-service-content) — the lifecycle is owned entirely by DRWUI at runtime.

## What our core does

Nothing on this surface (all greps first-hand, SA-10): no SCM create/start/stop/delete code outside the RDA driver scripts; no /install //uninstall verbs; no elevation broker; no packaging-contract row (PACKAGING.md grep DRWService/FVE → 0). The FVE auto-unlock harvest is in-process (S-014, live + offline hive, the og's caps applied) and requires an elevated token to succeed against the live hive (FVEAutoUnlock is SYSTEM/admin-ACL'd). Existing registrations cover the service's DATA-PATH absence generically (PARITY-ARCH.md:51 DELIBERATE-DELTA "detect + legitimate unlock only"; PARITY-OG-CORE-INVENTORY.md:74 SIDECAR-OOS "pipe service = host deployment") but never enumerate or score the install lifecycle parameters. The K71-K73 walk in modules/driver-install.md covers the RDA driver slice only (S-036/M-060/M-061/M-062/M-064).

## Gap

Falsifiable claims:
1. No core code, doc row, or packaging-contract parameter implements or names the og's FVE service lifecycle: `rg 'CreateService|StartServiceCtrlDispatcher' src/` (excl. driver scripts) → 0; `docs/PACKAGING.md` grep DRWService/FVE → 0.
2. The K71-K73 DRWService slice is unscored: modules/driver-install.md contains no DRWService row (verified by read).
3. R-83's disposition note ("install lives in driver-repo CI scripts") is false for this service — the driver-repo scripts install the RDA driver service only (verified: `.github/scripts/Install-ReleaseDriver.ps1` + `installer/core-recovery.iss` both RDA-only).
4. In a non-elevated core run (a posture the core deliberately supports, D-60a), a BitLocker auto-unlock volume the og unlocks (via its LocalSystem service or its always-elevated UI) stays locked in the core, with no surfaced elevation diagnosis specific to FVE (the harvest returns ERR_FVE_LOCKED without distinguishing "no keys configured" from "keys exist but unreadable by this token").

## Impact

- **Capability/host-integration:** host integrators running the core's UI/library surface without elevation lose the og's standard-user FVE auto-unlock path entirely; the elevation requirement for the harvest is documented nowhere.
- **Packaging-contract honesty:** the K71-K73 walk is the packaging milestone's scoring axis; its DRWService slice being unscored means the milestone could ship "K71-K73 green" while a decoded og surface stays unowned — exactly the class of hole SURFACE-INVENTORY §9 flagged.
- **Doc truth:** R-83's note misdirects readers to the driver scripts.
- Not a recovery-results defect (elevated runs harvest identically); not user-visible in the og's own frame (the og's UI is always elevated, so the service's standard-user rationale is itself corpus-ambiguous — see Open questions).

## Reproduction / verification

Corpus side: decompiled evidence (strength 2) — every SCM call quoted with file:line from `DRWService_decompiled.c` (first-hand LANE-B read; the delete gate at `:1310-1311` is a decompile reading beyond the artifact's coarser summary, flagged as such), string tables verbatim (`floss_strings.txt:236-264`), capa corroboration (`per-binary/DRWService.md:25`), dynamic Wine-run corroboration (`dynamic-ground-truth--wine-loader-run.md:20,26`). Core side: executed greps at `84a1832` (strength 1 for absence claims). Not executed: no Windows SCM lab in this environment; install/uninstall execution belongs to the Windows CI lane per the fix plan.

## Fix plan (for the engineering team)

1. **Decision first (owner call, recorded in `docs/PACKAGING.md`):** does the merged product implement the elevation-broker pattern at all?
   - **Option (a) NO-SERVICE (recommended default):** the core keeps the in-process harvest; extend `try_auto_unlock`'s failure path to distinguish ACL-denied from no-keys using the `cr_elevate` seam (an elevation-specific diagnosis: "FVE auto-unlock keys exist but require an elevated token"), and register the whole surface as a DIVERGENCE-INFORMATIONAL row (documented subset) citing this finding's parameter table.
   - **Option (b) TRANSIENT HELPER (only if a non-elevated host surface is a product requirement):** implement the decoded contract, not an invention — silent SCM EXE (no console — the N-085 lesson), `/install` with replace-on-install, the exact CreateService shape (name/display/type/start/error-control/account/description/ImagePath), the stop-gated uninstall with "Service not installed" no-op, per-call SCM error strings (K71's no warning-and-exit-0), and the host-side per-use orchestration pattern (uninstall → install → harvest → teardown), LocalSystem, asInvoker manifest, elevation inherited from the parent.
2. Either way, fold the parameter table into the K-walk: add the DRWService row to `modules/driver-install.md` (K71/K72/K73 sub-verdicts: absent-or-registered) — PARAMETERS.md Group K already cites the artifact as a derivation source.
3. Fix R-83's note in `docs/REFERENCE-API-GAP.md:425`: "none in core; the driver-repo CI scripts install the RDA driver service, not 'Read FVE Server' — disposition M-105".
4. Add the SECURITY.md sentence this implies (extends N-097's): the core never installs a persistent service and never persists key material (the positive posture statement), with the og's contrast (transient LocalSystem service + plaintext auto.txt).

## Tests to add

(Only if option (b) is chosen; option (a) needs the doc rows + the diagnosis-string unit.)
- `FveService.InstallConfigurationMatchesOg` — Windows lane: after `/install`, `sc qc` asserts name/display/type=0x10/start=auto/error=normal/LocalSystem/no-deps/description, ImagePath = the exe's own path (fails against any invented shape — K72).
- `FveService.InstallReplacesExistingInstance` — pre-installed service → `/install` performs uninstall-then-create; a surviving old instance prints "Service already installed" and changes nothing.
- `FveService.UninstallNoOpWhenAbsent` — `/uninstall` with no service → clean exit, no error (K71 loud-only-on-failure).
- `FveService.UninstallStopsBeforeDelete` — running service → stop issued, delete verified; the delete is skipped (and reported) when the stop is still pending (the decoded gate).
- `FveService.NonElevatedInstallFailsLoudly` — SCM error surfaced (no exit-0; the S-036/M-062 lessons apply verbatim).
- Option (a) variant: `Bitlocker.AutoUnlockAclDeniedDiagnosis` — harvest against a token-restricted fixture reports the elevation-specific failure, not "no keys".

## Related findings

- **S-014 (FIXED)** — the FVE harvest data path (live + offline hive); the boundary this finding does NOT cross.
- **S-038 (FIXED)** — the app-installer packaging contract (K74/K76/K77); its 13 parameters do NOT include the DRWService facts (grep-verified) — this finding is the missing K71-K73 slice.
- **M-061 / M-064 (FIXED)** — the driver uninstall/deployer contract: the same K71-K73 axis, driver half.
- **M-078** — BitLocker turn-off absent (the other BitLocker interplay chain) (the other end of the BitLocker interplay chain).
- **N-097** — auto.txt = the FVE key snapshot DRWUI persists during the same orchestration this finding decodes.
- **modules/health.md** — the F61 lane this surface's elevation posture touches.
- **D-60a (PARITY-DELTAS)** — the elevation posture both sides of this finding rest on.

## Open questions

1. The corpus's own rationale is contradictory: the artifact claims the service exists "so a standard-user DRW instance can obtain the keys without UAC" (`:5`, `:79-82`), but DRWUI's manifest is requireAdministrator (D-60a). If DRWUI is always elevated, the service's marginal value is the SYSTEM-token read (and WinPE), not standard-user support. A corpus decode of the exact FVEAutoUnlock ACL expectation would settle it; until then the fix-plan default (option a) is the conservative reading.
2. Whether the BUILDPE copy of DRWService.exe is ever /install'ed inside WinPE (the app runs as SYSTEM there; the flow would work unchanged — not decoded in the corpus).
3. The delete-gate nuance (delete only from STOPPED) is a decompile-first-hand reading; the artifact's coarser summary is the citable ground truth. If the engineering team implements option (b), a Windows-lab observation of the 2-s-sleep → uninstall sequence would pin the real-world stop-pending behavior.
