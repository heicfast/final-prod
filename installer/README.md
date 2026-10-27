# installer/ — the combined product installer + portable build

One NSIS artifact installs the whole product — application, native engine,
the REQUIRED RDA kernel driver, and machine-level shared state — and one
uninstaller removes all of it. This directory implements
`docs/INTEGRATION-PLAN.md` §7 against the committed `build.ps1` contract,
as amended by the mandatory-driver session (2026-10-27): the driver is a
required component, per-user installs are gone, and the payloads carry the
development test signature (`docs/DEV-SIGNING.md`).

| File | Role |
|---|---|
| `recoverywizard.nsi` | The combined product installer (NSIS, Unicode, LZMA). Built by `build.ps1 -Target installer`. |
| `portable.ps1` | The no-installer zip (`build.ps1 -Target portable`). |
| `devsign.ps1` | The development test-signing tool: ephemeral certificate generation, embed-signing, certificate store import/remove (design: `docs/DEV-SIGNING.md`). |

## Architecture — the wrapper decision (and the plan §7.2 resolution)

`docs/INTEGRATION-PLAN.md` §7.2 describes the installer as "Tauri's NSIS
bundler with a custom template + hook page set"; the committed `build.ps1`
(T6) instead consumes the stock Tauri NSIS bundle as a **payload** and drives
it silently from an outer product installer. These contradict; the committed
code wins (it is the executable contract, it avoids version-locking Tauri's
installer template, and it keeps every product concern — engine kit, driver,
shared state, complete uninstall — in one reviewable script). This directory
implements the wrapper; the contradiction is recorded here and in the
worklog.

Flow (all-users only — the per-user mode was REMOVED with the mandatory
driver; a machine-level component cannot live per-user, and the product
without its driver is not the product):

1. **Welcome / License** (MIT, repo root).
2. **Components**: Application (required) · RDA kernel driver (**required,
   `SectionIn RO` — no consent checkbox**; installing the product installs
   the driver, which the mandate's "no hidden driver install" requirement
   satisfies by the components page + the driver page, both surfacing it).
3. **Driver page** (surfaced + explained, informational): what the driver
   does (`\\.\RDA` raw sector reads — the engine's ONLY transport), that the
   product does not function without it (every scan refuses with
   `ERR_DRIVER_REQUIRED`), the SYSTEM privilege + admin-only DACL reality,
   and the **development test-signing truth**: the installer imports the
   build certificate into Root + Trusted Publishers; Secure-Boot-on
   machines need `bcdedit /set testsigning on` + reboot (or Secure Boot
   off) for the driver to load — stated on the page, not hidden.
4. **Directory → Install**:
   - the inner Tauri setup runs `/S /allusers /D=$INSTDIR`
     (silent; same dir), integrity-checked (`recovery-wizard-ui.exe` +
     `uninstall.exe` must exist);
   - engine kit → `$INSTDIR\` (BESIDE the app exe — layout truth: the Rust
     bridge statically imports `cr_ui.dll`, and the loader resolves static
     imports from the exe's directory at process start; an `engine\`
     subfolder is off the search path and would fail with 0xc0000135) with an
     integrity gate (`cr_ui.dll` plus at least one runtime DLL — the
     0xc0000135 guard);
   - **certificate import** (`certutil -addstore` Root + TrustedPublisher,
     the staged `rda-testcert.cer`) — BEFORE the service create, so the
     test-signed driver is loadable the moment it is registered
     (`docs/DEV-SIGNING.md` §2-§3); the thumbprint is recorded in the
     product registry key for the uninstaller;
   - driver staged to `$INSTDIR\driver\`, then registered via **the driver's
     own `install.ps1`** (exact SCM semantics: `type= kernel start= demand
     error= normal group= "Boot File System"`, file renamed to `RDA.sys`,
     Parameters `{IoctlBase=0, MaxDisks=200, DebugLevel=1,
     SessionsEnabled=1}`). An existing service is stopped + deleted first
     (update/repair semantics; no reboot). A registration failure
     **aborts the install** (a mandatory component cannot fail quietly);
     a start failure is non-fatal and states the policy truth — the
     service is registered demand-start and the product tells the user
     which policy must change (no degraded mode exists to fall back to).
   - `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}` created
     (constants from `contracts/README.md` — defined ONCE);
   - the inner installer's uninstall registry entry is **taken over**: any
     Uninstall key pointing into `$INSTDIR` is removed and replaced by the
     single complete product entry → ONE Add/Remove entry with full scope.
5. **Finish**: optional app launch (the app itself is
   `requireAdministrator` — INTEGRATION-PLAN §7.4, reference parity).

**Uninstall order** (complete; nothing orphaned): ask-close → driver
(`uninstall.ps1`, with `sc stop`/`sc delete`/`Delete /REBOOTOK` fallback and
a belt-and-braces file check) → **the imported development certificate
removed from Root + TrustedPublisher by thumbprint** → ProgramData sessions
**kept by default** (deleted only on explicit consent — reference parity:
user data survives) → inner Tauri uninstaller silent (`_?=` choreography) →
kit DLLs at `$INSTDIR` (explicit deletes — the inner uninstaller's manifest
covers only the Tauri bundle) → `driver\`, product registry keys, final
`RMDir`.

**Update**: rerun the installer (in-place; NSIS standard). The driver step
detects the existing service and does stop → delete → recreate (the
certificate import is `-f` idempotent).
**Repair**: identical (reinstall-over).

**Silent installs (`/S`) — the `/SD` discipline**: every `MessageBox` in
this script that can execute on the silent path carries `/SD` with the
safe default (driver notices → `IDOK`; the Win10 gate → `IDOK` before its
`Abort`; uninstall ask-close → `IDCANCEL` — refuse rather than remove;
sessions prompt → `IDNO` under the `${Silent}` guard). Reason: NSIS
manual §4.12 — a silent installer's script "can still display whatever it
wants"; a `MessageBox` **without** `/SD` is displayed and **blocks** the
silent run. Validation round 11 proved it live (a wedged run and the
evidence trail that led to this rule). `AbortWithReason` (file + `Abort`)
is the silent-safe way to fail loudly instead.

## Known deviations (all recorded)

| # | Deviation | Rationale |
|---|---|---|
| 1 | Installer manifest is `RequestExecutionLevel admin` — one UAC at launch, not the mandate's ideal "unelevated launch, elevate at the driver step" | Both integration lanes converged on this (INTEGRATION-PLAN §7.2/§7.4): per-machine install + SCM + `System32\drivers` all need elevation, the app itself runs elevated, and the unelevated-launch pattern (UAC plugin re-launch dance) adds fragility for no functional gain. One prompt, honestly shown, before anything happens. |
| 2 | ProgramData directories inherit default ACLs (no custom SDDL) | The app runs elevated (full write). Standard users can create files via the inherited ProgramData ACL. A tighter per-user ACL model is a documented follow-up, not silently skipped. |
| 3 | The outer installer UI is English-only (the inner Tauri bundle retains English + Simplified Chinese) | v1 scope; recorded for the docs sweep. |
| 4 | Wrapper architecture instead of a custom Tauri NSIS template | See "Architecture" above. |
| 5 | Per-user installs removed (user directive, 2026-10-27) | The driver is mandatory and machine-level (SCM + `System32\drivers` + certificate stores); a per-user install would either be driver-less (not the product) or machine-mutating from a user context (worse). Recorded in `docs/PARITY-DELTAS.md`. |
| 6 | Driver start failures are non-fatal to the INSTALL while being fatal to the PRODUCT's scans | The service registers fine; the machine's signing policy (Secure Boot ON without test signing) refuses the LOAD. Aborting the install would not fix the policy — the truthful outcome is a registered demand-start service, a page/notice that names the exact remedy, and an engine that refuses scans with `ERR_DRIVER_REQUIRED` until the driver loads. |

## Validation status

- `recoverywizard.nsi` is compile-checked (makensis cross-compile: exit 0,
  zero warnings, valid PE32, 7 sections — re-proven after the
  mandatory-driver rewrite with the same methodology as
  `docs/INTEGRATION-VALIDATION/installer-compile-check.md`).
- Full install/uninstall/update/repair behavior on a clean Windows machine
  is the T15 lane — which now asserts the driver service RUNNING, the
  development certificate present in Root + TrustedPublisher, the exact
  product module set, and the certificate's removal on uninstall (evidence
  under `docs/INTEGRATION-VALIDATION/`).
- The payloads carry the development test signature (`docs/DEV-SIGNING.md`);
  production signing remains pending and every release notes states it.
