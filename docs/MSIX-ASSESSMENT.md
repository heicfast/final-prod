# MSIX assessment — feasibility verdict for HeicFast Recovery Wizard

> Task T12 (docs/INTEGRATION-PLAN.md §12). Verdict: **NOT FEASIBLE for this
> product — no MSIX artifact is produced.** This document is the truthful
> record; `docs/RELEASE.md` carries the user-facing statement.

## Why MSIX cannot carry this product

1. **A Windows kernel driver cannot be installed by MSIX.** MSIX packages
   execute inside an AppContainer-isolated, brokered environment: they cannot
   create SCM services, cannot write `System32\drivers`, and cannot register
   kernel modules. Kernel-driver distribution requires classic SCM/PnP
   installation with WHQL / Hardware Dev Center attestation signing — none of
   which the MSIX deployment model exposes. Our product's RDA driver
   component (`rda.sys`, service `RDA`, `HKLM\SYSTEM\CurrentControlSet\
   Services\RDA` + Parameters — `contracts/README.md`, `driver/scripts/
   install.ps1`) is therefore impossible under MSIX.

2. **The application itself cannot run inside the MSIX AppContainer.** The
   recovery engine's transport legs are administrator-only on stock Windows:
   the driver control device `\\.\RDA` is SDDL-restricted to
   Administrators/SYSTEM (`D:P(A;;GA;;;BA)(A;;GA;;;SY)` — deliberate driver
   hardening), and direct raw reads via `\\.\PhysicalDriveN` /
   `\\.\HarddiskVolumeN` require elevation. The product therefore ships with
   a `requireAdministrator` application manifest (INTEGRATION-PLAN §7.4,
   reference parity: the reference product's `DRW.exe` and `DRWUI.exe` both
   carry `requireAdministrator` manifests — REFERENCE-LAYOUT §6). An
   AppContainer-packaged app cannot host an elevated full-trust process of
   this shape.

An MSIX of this product would be a driver-less, disk-less shell that cannot
perform its core function — a worse-than-useful artifact. **Decision: MSIX
is dropped entirely; the NSIS installer is the single distribution artifact,
with the portable zip for the no-installer case.** The mandate's requirement
is satisfied the way it anticipates: "if MSIX cannot install the driver
legally, the release uses an NSIS installer … and the release notes state
this."

## Where this is recorded

- `docs/RELEASE.md` — user-facing artifact list and the "MSIX is not
  provided" statement.
- `docs/PARITY-DELTAS.md` — the packaging-technology deviation row
  (reference uses Inno Setup; we use NSIS; MSIX excluded for the reasons
  above).
- Release notes template (release workflow) — no MSIX artifact listed,
  signing posture stated truthfully.

## If MSIX ever becomes possible

The blockers are structural (driver deployment + elevation model), not
tooling. A future MSIX shape would require at minimum: a driver distributed
outside the package (separate SCM installer), a brokered elevation model for
raw disk access, and an AppContainer-compatible write surface for recovery
output. That is a product redesign, not a packaging task — out of scope by
the session mandate (no behavior changes), recorded here so the question
never has to be re-litigated blind.
