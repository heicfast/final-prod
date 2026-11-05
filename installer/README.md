# installer/ — the combined product installer + portable build

One NSIS artifact installs the whole product — application, native engine,
and machine-level shared state — and one uninstaller removes all of it.
This directory implements the committed `build.ps1` contract, as amended by
the consolidation session: the product is a **user-mode application with
direct raw-disk access** (no kernel driver, no service, no certificate-store
changes — nothing is loaded into the operating system), and the payloads
carry the development self-signature (`docs/DEV-SIGNING.md`).

| File | Role |
|---|---|
| `recoverywizard.nsi` | The combined product installer (NSIS, Unicode, LZMA). Built by `build.ps1 -Target installer`. |
| `portable.ps1` | The no-installer zip (`build.ps1 -Target portable`). |
| `devsign.ps1` | The development self-signing tool: ephemeral certificate generation, embed-signing, verification (design: `docs/DEV-SIGNING.md`). |

## Architecture — the wrapper decision (and the plan §7.2 resolution)

`docs/INTEGRATION-PLAN.md` §7.2 describes the installer as "Tauri's NSIS
bundler with a custom template + hook page set"; the committed `build.ps1`
(T6) instead consumes the stock Tauri NSIS bundle as a **payload** and drives
it silently from an outer product installer. These contradict; the committed
code wins (it is the executable contract, it avoids version-locking Tauri's
installer template, and it keeps every product concern — engine kit, shared
state, complete uninstall — in one reviewable script). This directory
implements the wrapper; the contradiction is recorded here and in the
worklog.

Flow (all-users only — the shared state is machine-level and the single
Add/Remove entry is a per-machine concern):

1. **Welcome / License** (MIT, repo root).
2. **Privilege page** (surfaced + explained, informational): what the product
   is — a user-mode application that reads and writes disks directly from
   user mode, installing no driver, no service, and no certificate — and
   what it needs: raw-disk operations require the application to run as
   administrator, with a plain in-app elevation-required notice and a
   restart-as-administrator action when it is not. No degraded mode, no
   fallback.
3. **Directory → Install**:
   - the inner Tauri setup runs `/S /allusers /D=$INSTDIR`
     (silent; same dir), integrity-checked (`recovery-wizard-ui.exe` +
     `uninstall.exe` must exist);
   - engine kit → `$INSTDIR\` (BESIDE the app exe — layout truth: the Rust
     bridge statically imports `cr_ui.dll`, and the loader resolves static
     imports from the exe's directory at process start; an `engine\`
     subfolder is off the search path and would fail with 0xc0000135) with an
     integrity gate (`cr_ui.dll` plus at least one runtime DLL — the
     0xc0000135 guard);
   - `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}` created
     (constants from `contracts/README.md` — defined ONCE);
   - the inner installer's uninstall registry entry is **taken over**: any
     Uninstall key pointing into `$INSTDIR` is removed and replaced by the
     single complete product entry → ONE Add/Remove entry with full scope.
4. **Finish**: optional app launch (the app itself is
   `requireAdministrator` — it needs the elevated token for raw-disk work).

**Uninstall order** (complete; nothing orphaned): ask-close → ProgramData
sessions **kept by default** (deleted only on explicit consent — reference
parity: user data survives) → inner Tauri uninstaller silent (`_?=`
choreography) → kit DLLs at `$INSTDIR` (explicit deletes — the inner
uninstaller's manifest covers only the Tauri bundle) → product registry
keys, final `RMDir`.

**Update**: rerun the installer (in-place; NSIS standard).
**Repair**: identical (reinstall-over).

**Silent installs (`/S`) — the `/SD` discipline**: every `MessageBox` in
this script that can execute on the silent path carries `/SD` with the
safe default (the Win10 gate → `IDOK` before its `Abort`; uninstall
ask-close → `IDCANCEL` — refuse rather than remove; sessions prompt →
`IDNO` under the `${Silent}` guard). Reason: NSIS
manual §4.12 — a silent installer's script "can still display whatever it
wants"; a `MessageBox` **without** `/SD` is displayed and **blocks** the
silent run. Validation round 11 proved it live (a wedged run and the
evidence trail that led to this rule). `AbortWithReason` (file + `Abort`)
is the silent-safe way to fail loudly instead.

## Known deviations (all recorded)

| # | Deviation | Rationale |
|---|---|---|
| 1 | Installer manifest is `RequestExecutionLevel admin` — one UAC at launch, not an unelevated launcher that elevates later | Per-machine install + the ProgramData shared state + the app's own elevated posture (raw-disk work needs the administrator token) all align on one prompt, honestly shown, before anything happens. |
| 2 | ProgramData directories inherit default ACLs (no custom SDDL) | The app runs elevated (full write). Standard users can create files via the inherited ProgramData ACL. A tighter per-user ACL model is a documented follow-up, not silently skipped. |
| 3 | The outer installer UI is English-only (the inner Tauri bundle retains English + Simplified Chinese) | v1 scope; recorded for the docs sweep. |
| 4 | Wrapper architecture instead of a custom Tauri NSIS template | See "Architecture" above. |
| 5 | Per-user installs not offered | The shared state is machine-level and the single Add/Remove entry is a per-machine concern; a per-user install would split the product's state across contexts. Recorded in `docs/PARITY-DELTAS.md`. |

## Validation status

- `recoverywizard.nsi` is compile-checked (makensis cross-compile: exit 0,
  zero warnings, valid PE32 — methodology per
  `docs/INTEGRATION-VALIDATION/installer-compile-check.md`; re-proven for
  the driver-free rewrite before the next tagged release).
- Full install/uninstall/update/repair behavior on a clean Windows machine
  is the T15 lane — which now asserts the driver-free install footprint
  (the exact product module set, the registry keys, the ProgramData state,
  and their complete removal on uninstall; evidence under
  `docs/INTEGRATION-VALIDATION/` and `worklog/RELEASE-VALIDATION/`).
- The payloads carry the development self-signature (`docs/DEV-SIGNING.md`);
  production signing remains pending and every release notes states it.
