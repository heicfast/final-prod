# installer/ — the combined product installer + portable build

One NSIS artifact installs the whole product — application, native engine,
RDA kernel driver (consent-gated), and machine-level shared state — and one
uninstaller removes all of it. This directory implements
`docs/INTEGRATION-PLAN.md` §7 against the committed `build.ps1` contract.

| File | Role |
|---|---|
| `recoverywizard.nsi` | The combined product installer (NSIS, Unicode, LZMA). Built by `build.ps1 -Target installer`. |
| `portable.ps1` | The no-installer zip (`build.ps1 -Target portable`). |

## Architecture — the wrapper decision (and the plan §7.2 resolution)

`docs/INTEGRATION-PLAN.md` §7.2 describes the installer as "Tauri's NSIS
bundler with a custom template + hook page set"; the committed `build.ps1`
(T6) instead consumes the stock Tauri NSIS bundle as a **payload** and drives
it silently from an outer product installer. These contradict; the committed
code wins (it is the executable contract, it avoids version-locking Tauri's
installer template, and it keeps every product concern — engine kit, driver,
shared state, complete uninstall — in one reviewable script). This directory
implements the wrapper; the contradiction is recorded here and in the
worklog. Both shapes satisfy every §7 requirement (single artifact, surfaced
consent-gated driver step, complete uninstall, update/repair flows).

Flow (per-machine; per-user varies as noted):

1. **Welcome / License** (MIT, repo root).
2. **Components**: Application (required) · RDA kernel driver (recommended,
   default ON). In per-user installs the driver is force-deselected, made
   read-only, and labeled truthfully — a machine-level component (SCM +
   `System32\drivers`) cannot live in a per-user install.
3. **Driver page** (surfaced + explained): what the driver does
   (`\\.\RDA` raw sector reads), what happens without it (direct-handle
   fallback: slower on some media, locked volumes unopenable), the SYSTEM
   privilege + admin-only DACL reality, and the **unsigned-driver truth**
   (won't load on stock Win10/11 until signing lands). The checkbox is the
   consent control; default ON, synced with the components page.
4. **Directory → Install**:
   - the inner Tauri setup runs `/S /allusers|/currentuser /D=$INSTDIR`
     (silent; same dir), integrity-checked (`recovery-wizard-ui.exe` +
     `uninstall.exe` must exist);
   - engine kit → `$INSTDIR\` (BESIDE the app exe — layout truth: the Rust
     bridge statically imports `cr_ui.dll`, and the loader resolves static
     imports from the exe's directory at process start; an `engine\`
     subfolder is off the search path and would fail with 0xc0000135) with an
     integrity gate (`cr_ui.dll`
     plus at least one runtime DLL — the 0xc0000135 guard);
   - driver staged to `$INSTDIR\driver\`, then registered via **the driver's
     own `install.ps1`** (exact SCM semantics: `type= kernel start= demand
     error= normal group= "Boot File System"`, file renamed to `RDA.sys`,
     Parameters `{IoctlBase=0, MaxDisks=200, DebugLevel=1,
     SessionsEnabled=1}`). An existing service is stopped + deleted first
     (update/repair semantics; no reboot). Start failures surface the
     signing guidance truthfully and are **non-fatal** (direct fallback).
   - `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}` created
     (constants from `contracts/README.md` — defined ONCE);
   - the inner installer's uninstall registry entry is **taken over**: any
     Uninstall key pointing into `$INSTDIR` is removed and replaced by the
     single complete product entry → ONE Add/Remove entry with full scope.
5. **Finish**: optional app launch (the app itself is
   `requireAdministrator` — INTEGRATION-PLAN §7.4, reference parity).

**Uninstall order** (complete; nothing orphaned): ask-close → driver
(`uninstall.ps1`, with `sc stop`/`sc delete`/`Delete /REBOOTOK` fallback and
a belt-and-braces file check) → ProgramData sessions **kept by default**
(deleted only on explicit consent — reference parity: user data survives) →
inner Tauri uninstaller silent (`_?=` choreography) → kit DLLs at `$INSTDIR` (explicit
 deletes — the inner uninstaller's manifest covers only the Tauri bundle) → `driver\`,
product registry keys, final `RMDir`.

**Update**: rerun the installer (in-place; NSIS standard). The driver step
detects the existing service and does stop → delete → recreate.
**Repair**: identical (reinstall-over).

**Silent installs (`/S`) — the `/SD` discipline**: every `MessageBox` in
this script that can execute on the silent path carries `/SD` with the
safe default (driver notices → `IDOK`; the Win10 gate → `IDOK` before its
`Abort`; uninstall ask-close → `IDCANCEL` — refuse rather than remove;
sessions prompt → `IDNO` under the `${Silent}` guard). Reason: NSIS
manual §4.12 — a silent installer's script "can still display whatever it
wants"; a `MessageBox` **without** `/SD` is displayed and **blocks** the
silent run. Validation round 11 proved it live: the first T15 run past the
app-integrity gate wedged ~40 minutes on the driver-not-started box
(unsigned driver on a stock runner — the expected branch there); the
cancelled run's evidence held only `01-baseline.json`, and the fix landed
with this rule. `AbortWithReason` (file + `Abort`) is the silent-safe way
to fail loudly instead.

## Known deviations (all recorded)

| # | Deviation | Rationale |
|---|---|---|
| 1 | Installer manifest is `RequestExecutionLevel admin` — one UAC at launch, not the mandate's ideal "unelevated launch, elevate at the driver step" | Both integration lanes converged on this (INTEGRATION-PLAN §7.2/§7.4): per-machine install + SCM + `System32\drivers` all need elevation, the app itself runs elevated, and the unelevated-launch pattern (UAC plugin re-launch dance) adds fragility for no functional gain. One prompt, honestly shown, before anything happens. |
| 2 | ProgramData directories inherit default ACLs (no custom SDDL) | The app runs elevated (full write). Standard users can create files via the inherited ProgramData ACL. A tighter per-user ACL model is a documented follow-up, not silently skipped. |
| 3 | The outer installer UI is English-only (the inner Tauri bundle retains English + Simplified Chinese) | v1 scope; recorded for the docs sweep. |
| 4 | Wrapper architecture instead of a custom Tauri NSIS template | See "Architecture" above. |

## Validation status

- `recoverywizard.nsi` is compile-checked on every CI run (the installer lane)
  and was compile-validated with the Tauri NSIS toolchain (makensis) — see
  `docs/INTEGRATION-VALIDATION.md` for the transcript evidence.
- Full install/uninstall/update/repair behavior on a clean Windows machine is
  the T15 lane (evidence under `docs/INTEGRATION-VALIDATION/`).
- The driver remains **unsigned by design**; release notes state signing is
  pending (see `docs/MSIX-ASSESSMENT.md` for the packaging-signing posture).
