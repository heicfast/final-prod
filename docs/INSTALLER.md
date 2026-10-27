# INSTALLER — the product's installation experience (user-facing contract)

The installer is `installer/recoverywizard.nsi` (NSIS; the technology
decision and its evidence live in `docs/INSTALLER-DECISION.md`). This
document is the contract a user — and the validation lanes — hold it to.

## User experience

1. **Elevation**: one standard UAC prompt (the installer carries an
   `admin` manifest — the driver and cert stores are machine-level state).
   No script windows, no secondary elevation prompts.
2. **Pages**: Welcome → License (MIT) → Components (Application required ·
   RDA kernel driver required) → the driver information page (what it does,
   the SYSTEM-privilege reality, the test-signing truth — see below) →
   Install directory → Progress → Finish (with "Launch" checkbox).
3. **No visible consoles anywhere.** The entire lifecycle (install, abort,
   update, repair, uninstall) runs `powershell.exe` **zero times**; the
   helper invocations that remain (`certutil`, `sc.exe`) run HIDDEN with
   their output captured into the details view and the install log
   (`nsExec::ExecToLog`). The only child setup that shows anything is the
   bundled application setup, which runs silently inside the progress page.
4. **Silent mode**: `setup.exe /S` (documented; the T15 lane runs it).
   Message boxes carry `/SD` defaults so silent runs never block.

## File placement (everything the installer puts on the machine)

| Location | Contents |
|---|---|
| `$INSTDIR` (default `C:\Program Files\HeicFast\Recovery Wizard`) | the app (`recovery-wizard-ui.exe`, Tauri bundle) · the engine kit BESIDE the exe (`cr_ui.dll`, `rrecov.exe`, `z/lz4/zstd` runtime DLLs — loader truth: static imports resolve from the exe dir) · `driver\` (the staged driver kit: `rda.sys`, `rdacli.exe`, `rda.inf`, the driver's own ops scripts, the public `.cer` + thumbprint) · `uninstall.exe` (inner) · `HeicFastUninstall.exe` (ours) |
| `System32\drivers` | `RDA.sys` (copied WOW64-safely — the 32-bit installer disables file-system redirection around the copy) |
| Service Control Manager | service `RDA`: kernel driver, demand-start, error=normal, group "Boot File System", `ImagePath=System32\drivers\RDA.sys`, Parameters `{IoctlBase=0, MaxDisks=200, DebugLevel=1, SessionsEnabled=1}` — the same profile the reference installs for its driver |
| Certificate stores | the build's development certificate in LocalMachine `Root` + `TrustedPublisher` (test-signing posture — removed at uninstall) |
| `%ProgramData%\HeicFast\Recovery Wizard` | `sessions\` (the product's scan sessions) · `logs\` (`install.log` — see Diagnostics) |
| Registry | `HKLM\Software\HeicFast\Recovery Wizard` (Version, InstallDir, InstallMode, DriverInstalled, DriverCertThumbprint) · the Add/Remove entry `HKLM\...\Uninstall\Recovery Wizard` |

## Driver installation flow (install)

1. Stage the driver kit into `$INSTDIR\driver` (the signed-drop-in surface).
2. Import the development certificate into `Root` + `TrustedPublisher`
   (hidden, logged; failure aborts — without the certificate the driver
   cannot load on this posture).
3. Update path: an existing `RDA` service is stopped and deleted first.
4. Copy `rda.sys` → `System32\drivers\RDA.sys` with WOW64 redirection
   disabled; **verify** the file landed in the real System32 (the copy is
   fatal-checked — a SysWOW64 copy is the classic ERROR_FILE_NOT_FOUND
   trap).
5. `sc.exe create RDA` with the reference profile (above) — hidden, logged.
6. Write the Parameters block (registry; `HKLM\SYSTEM` is not redirected).
7. `sc.exe start RDA`: `0`/`1056` → installed-and-running; anything else →
   the truthful "registered but the machine's signing policy refused to
   load it" branch (message box explains Secure Boot / testsigning; the
   service is demand-start and loads on the next boot once policy allows).
8. **Abort path**: a registration failure removes the just-imported
   certificate (no orphan trust anchor), writes the reason to the log, and
   aborts — the product never installs half-way.

## Uninstall

Close-app check → stop + delete the `RDA` service → delete the service key
and `System32\drivers\RDA.sys` (both WOW64 views) → remove the development
certificate from both stores → **sessions are user data**: kept in silent
mode unconditionally; interactive uninstall asks, defaulting to KEEP → run
the inner application uninstaller silently → delete every remaining
installer-owned file (the engine kit's DLL catch-all included) → delete the
uninstall + product registry keys (and the seeded inner-install key) →
delete the installer's own `install.log` → remove empty directories.
The T15 matrix verifies: zero leftovers (service, driver file, certs from
both stores, registry keys — sessions kept).

## Update

Re-running the new setup over an existing install: the existing service is
stopped + deleted and recreated from the new payload; the certificate
import is idempotent (`-f`); files are overwritten in place; `%ProgramData%`
sessions survive (T15-verified with a session marker across reinstall);
version fields update from the single `VERSION` source.

## Repair

Rerun the installer (the Add/Remove entry sets `NoRepair`; the honest
repair story is the verified reinstall-over). T15 proves the repair case:
a corrupted `cr_ui.dll` is restored byte-exact by a rerun.

## Signing posture

This session's artifacts are **development test-signed** (ephemeral
per-build certificate; `docs/DEV-SIGNING.md`). The installation flow is
production-shaped: a production-signed driver + installer drop into the
same flow with **zero script changes** — the certificate import step
becomes a no-op that `devsign.ps1` skips when it finds no `.cer` staged
(the staged-certificate file's presence is what drives the import), and
`release.yml`'s verification gate already demands real signatures.

## Diagnostics

`%ProgramData%\HeicFast\Recovery Wizard\logs\install.log` — one line per
lifecycle fact: every phase, every child-process exit code, every registry
read-back, the driver result classification. It survives the install (and
the uninstall removes it with the product). `%TEMP%` carries the same
content (`HeicFastRecoveryWizard-install-facts.txt`) plus
`...-install-abort.txt` with the abort reason — the evidence contract the
validation lanes read.

## Reproducibility

One command from a clean checkout — `.\build.ps1 -Target all` — builds and
signs everything and produces the installer; `VERSION` (repo root) is the
single version source, enforced by `!error` guards in the script and the
release workflow's tag gate.
