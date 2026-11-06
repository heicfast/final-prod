# INSTALLER — the product's installation experience (user-facing contract)

The installer is `installer/recoverywizard.nsi` (NSIS; the technology
decision and its evidence live in `docs/INSTALLER-DECISION.md`). This
document is the contract a user — and the validation lanes — hold it to.

The product is **user-mode**: the installer places files and prepares the
shared ProgramData area, and nothing else. No driver, no service, no
certificate-store changes — nothing is loaded into the operating system,
and uninstall removes everything the installer placed. (The driver-era
flows this document used to describe — `SEC_DRIVER`, the `RDA` service,
the `System32\drivers` copy, the `certutil` imports — were removed in the
consolidation session, commit 318b9655.)

## User experience

1. **Elevation**: one standard UAC prompt (the installer carries an
   `admin` manifest — the all-users install writes Program Files + HKLM
   and prepares the machine-shared ProgramData state). No script windows,
   no secondary elevation prompts.
2. **Pages**: Welcome → License (MIT) → Components (Application, the
   only and required component) → the privilege information page →
   Install directory → Progress → Finish (with "Launch" checkbox).
   The privilege page states the product truth: a user-mode application
   with direct raw-disk access whose exe requests the administrator
   token AT LAUNCH through its requireAdministrator manifest (one UAC
   prompt, reference parity) — a non-elevated context (an edge case)
   refuses truthfully, never degrading silently. The page is
   informational; there is no driver
   step, no service choice, no signing-policy branch.
3. **No visible consoles anywhere.** The entire lifecycle (install, abort,
   update, repair, uninstall) runs `powershell.exe` **zero times**; no
   helper processes are spawned at all — file placement, registry, and
   directory preparation are native NSIS operations, logged into the
   details view and the install log. The only child setup that shows
   anything is the bundled application setup, which runs silently inside
   the progress page.
4. **Silent mode**: `setup.exe /S` (documented; the T15 lane runs it).
   Message boxes carry `/SD` defaults so silent runs never block.

## File placement (everything the installer puts on the machine)

| Location | Contents |
|---|---|
| `$INSTDIR` (default `C:\Program Files\HeicFast\Recovery Wizard`) | the app (`recovery-wizard-ui.exe`, Tauri bundle) · the engine kit BESIDE the exe (`cr_ui.dll`, `rrecov.exe`, `z/lz4/zstd` runtime DLLs — loader truth: static imports resolve from the exe dir) · `uninstall.exe` (inner) · `HeicFastUninstall.exe` (ours) |
| `%ProgramData%\HeicFast\Recovery Wizard` | `sessions\` (the product's scan sessions) · `logs\` (`install.log` — see Diagnostics) |
| Registry | `HKLM\Software\HeicFast\Recovery Wizard` (Version, InstallDir, InstallMode) · the Add/Remove entry `HKLM\...\Uninstall\Recovery Wizard` |

That is the complete list. There is no `System32\drivers` row, no service
row, and no certificate-store row anymore — the machine's OS-level state
is untouched by both install and uninstall.

## Install flow

1. Run the inner Tauri application setup silently inside the progress
   page (`app-setup.exe /S /allusers /D=$INSTDIR`); integrity-check that
   `recovery-wizard-ui.exe` and the inner `uninstall.exe` landed.
2. Lay the engine kit beside the exe (`File` on the staged kit, excluding
   the link-time `cr_ui.lib`); gates: `cr_ui.dll` present, ≥2 DLLs total
   in `$INSTDIR`.
3. Create `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}\`.
4. Write the product key (Version, InstallDir, InstallMode) with
   read-backs, sweep any stale inner-setup uninstall keys, write the
   Add/Remove entry, and write the outer uninstaller
   (`HeicFastUninstall.exe`).
5. **Abort path**: a failed inner setup or a failed engine-kit gate
   aborts with the reason logged — no partial product.

## Uninstall

Close-app check → **sessions are user data**: kept in silent mode
unconditionally; interactive uninstall asks, defaulting to KEEP → run the
inner application uninstaller silently → delete every remaining
installer-owned file (the engine kit's DLL catch-all included) → delete
the product registry keys (and the seeded inner-install key) → delete
the installer's own `install.log` → remove empty directories.
The T15 matrix verifies: zero leftovers (no service, no driver file, no
certificates, no registry keys — sessions kept).

## Update

Re-running the new setup over an existing install: files are overwritten
in place; `%ProgramData%` sessions survive (T15-verified with a session
marker across reinstall); version fields update from the single `VERSION`
source. There is no service teardown/recreate step anymore — the
driver-era stop/delete/recreate choreography is gone with the driver.

## Repair

Rerun the installer (the Add/Remove entry sets `NoRepair`; the honest
repair story is the verified reinstall-over). T15 proves the repair case:
a corrupted `cr_ui.dll` is restored byte-exact by a rerun.

## Signing posture

This session's artifacts are **development self-signed** (ephemeral
per-build certificate; `docs/DEV-SIGNING.md`). The signature is an
integrity marker on the engine kit and the installer — it is NOT a load
prerequisite (user-mode binaries need no kernel-signing trust), and the
installer imports NO certificate into ANY store on ANY machine. A
production-signed build drops into the same flow with zero script
changes, and `release.yml`'s verification gate already demands real
signatures.

## Diagnostics

`%ProgramData%\HeicFast\Recovery Wizard\logs\install.log` — one line per
lifecycle fact: every phase, every registry read-back, the file-placement
gates. It survives the install (and the uninstall removes it with the
product). `%TEMP%` carries the same content
(`HeicFastRecoveryWizard-install-facts.txt`) plus
`...-install-abort.txt` with the abort reason — the evidence contract the
validation lanes read.

## Reproducibility

One command from a clean checkout — `.\build.ps1 -Target all` — builds and
signs everything and produces the installer; `VERSION` (repo root) is the
single version source, enforced by `!error` guards in the script and the
release workflow's tag gate.
