# INSTALLER-DECISION — the technology research and the decision

The mandate (§4): replace an installer that "launches a PowerShell popup and
does not feel like a production product." This document records the
technology research, the decision, and the trade-offs — evidence-based, not
preference-based.

## What was actually broken (the evidence)

The forensic reproduction of the shipped v0.6.0-rc1 (run 40465852724,
`docs/FORENSIC-PROBE.md` §S1) captured every visible window the installer
created:

| Spawn | Visibility evidence |
|---|---|
| `powershell.exe -File install.ps1` (driver registration) | **~10.4 s visible console window** (26 samples × 400 ms; `ConsoleWindowClass` titled `...WindowsPowerShell\v1.0\powershell.exe`) |
| `certutil -addstore -f Root` (cert import) | visible console (`C:\Windows\SYSTEM32\certutil.exe`) |
| `certutil -addstore -f TrustedPublisher` | visible console |
| `powershell.exe -File uninstall.ps1` (uninstall) | the same trio on the uninstall side |

Root cause: `ExecWait` on console executables (`recoverywizard.nsi:301,307,349`
install; `:608,649,651` uninstall). ExecWait = a visible console child.

## The candidate technologies

| Criterion | NSIS (incumbent, fixed) | WiX/MSI | Inno Setup | Custom bootstrapper |
|---|---|---|---|---|
| No visible consoles | ✓ achievable (`nsExec` hides + logs; native ops for the rest) | ✓ | ✓ | ✓ |
| Standard UAC elevation | ✓ already (`RequestExecutionLevel admin`) | ✓ | ✓ | ✓ (must build it) |
| Driver install honoring Windows norms | ✓ SCM via `sc.exe`/registry — same mechanism the reference uses natively | DIFxApp is **deprecated**; the WiX driver extension targets PnP driver packages, our kernel service is SCM-registered | ✓ via [Run] sc.exe/pascalscript | ✓ but hand-rolled |
| Reference product's own mechanism | — | — | the reference uses Inno + a bootstrapper, but that is *its* choice, not a norm | its bootstrapper exists for 16-OS gating we do not need |
| Deterministic uninstall / update / repair | ✓ (T15-verified reinstall-over + byte-exact repair; repair = rerun) | ✓✓ (MSI-native repair/upgrade semantics) | ✓ | ✗ all hand-built |
| Reproducible from clean checkout | ✓ already (build.ps1 → makensis, VERSION single-source, !error define contract) | needs WiX toolchain in every build lane | needs Inno toolchain in the lanes | needs a full C++/MSVC app + its own build |
| Diagnostic install log | ✓ added (ProgramData install.log, every step + exit code + read-back) | ✓ MSI logging | ✓ | hand-built |
| Migration cost from the incumbent | **low** (same technology; the broken flows are three function calls) | **high** (full rewrite of a 736-line verified choreography + re-validating every flow the T15 matrix already proves) | high (same scope) | highest |

## The decision

**Keep NSIS; rebuild the broken flows to production standard.** The defects
were implementation defects — three `ExecWait` calls on console programs —
not technology defects. The evidence:

1. The visible-console class is eliminated in NSIS by construction:
   `nsExec::ExecToLog` runs the same commands hidden with captured output,
   and the service/cert/file operations are native NSIS operations.
2. The driver is an SCM-registered kernel service (like the reference's
   `eprdtdrv` — `CreateService`, group "Boot File System"), NOT a PnP
   driver package; MSI's driver-installation story (DIFxApp) is deprecated
   and would not change the mechanism anyway.
3. The T15 runtime matrix already validates this installer's install /
   update / repair / uninstall choreography end to end on clean Windows
   runners (run 40442056802, GREEN); a technology migration would discard
   that verified state and re-incur the entire validation debt.
4. MSI's superior repair/upgrade semantics are real but are a migration for
   their own sake in this session: update = reinstall-over preserving
   ProgramData (verified), repair = rerun (verified byte-exact restore of a
   corrupted `cr_ui.dll`), both documented in `docs/INSTALLER.md`.

## What changed (the rebuild)

- **Driver registration is native** (no `install.ps1` execution): WOW64-safe
  copy to `System32\drivers` (`x64.nsh` redirection control — the 32-bit
  installer's SysWOW64 trap is handled explicitly), `sc.exe create` (hidden,
  logged) with the reference's profile (kernel / demand / error=normal /
  group "Boot File System"), the Parameters block as plain registry writes,
  `sc.exe start` with the truthful signing-policy branch.
- **Every `certutil` call** (install, abort-path, uninstall) runs hidden and
  logged via `nsExec::ExecToLog`.
- **No `powershell.exe` anywhere** in install, abort, or uninstall.
- **A persistent, discoverable install log**: every lifecycle fact (phase,
  child exit code, registry read-back) appends to
  `%ProgramData%\HeicFast\Recovery Wizard\logs\install.log` in addition to
  the T15 `%TEMP%` facts file.
- **The abort path no longer strands the imported certificate** (best-effort
  removal before the abort) — the orphan defect agent B found.
- **Uninstall removes the installer-placed log** (complete-uninstall
  guarantee; sessions stay behind explicit interactive consent, unchanged).

## What was rejected, and why (honestly)

- **MSI/WiX**: deprecated driver-installation integration for the SCM
  service model we ship; full choreography rewrite discarding verified
  behavior. Revisit only if the product moves to PnP driver packages or
  needs enterprise policy deployment (AD/GPO MSI distribution).
- **Inno Setup**: capable, but migrating to the reference's installer
  technology is mimicry, not engineering; the incumbent with native flows
  meets every requirement.
- **Custom native bootstrapper**: strictly more code to own, zero capability
  we lack; the reference's own bootstrapper exists for constraints (16-OS
  gating, co-bundled upsells) we do not have.

## Validation of the rebuild

- Cross-compile proof: `makensis 3.12` exit 0, **zero warnings**, valid
  self-extracting artifact (the define-contract `!error` guards intact) —
  same method as `docs/INTEGRATION-VALIDATION/installer-compile-check.md`.
- Runtime proof: the forensic-repro + T15 lanes on real Windows runners
  (the evidence lands in `docs/INTEGRATION-VALIDATION/` and
  `docs/INTEGRATION-VALIDATION.md`).
