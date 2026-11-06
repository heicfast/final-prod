# MSIX assessment — feasibility verdict for HeicFast Recovery Wizard

> Task T12 (docs/INTEGRATION-PLAN.md §12). Original verdict (2026-10-26
> era): **NOT FEASIBLE for this product — no MSIX artifact is produced.**
> **Re-evaluated 2026-11-05 (consolidation session): the assessment is
> REOPENED — the kernel-driver blocker below is GONE (the driver was
> removed from the product entirely, commit 318b9655), while the
> elevation-model blocker remains.** No new feasibility conclusion is
> claimed beyond what the surviving facts support; this document records
> the change honestly and leaves the decision to a future session. Until
> such a decision, `docs/RELEASE.md` carries the user-facing statement.

## Why MSIX could not carry this product (original reasons, statused)

1. **A Windows kernel driver cannot be installed by MSIX — WAS blocker
   #1, now MOOT.** MSIX packages execute inside an AppContainer-isolated,
   brokered environment: they cannot create SCM services, cannot write
   `System32\drivers`, and cannot register kernel modules; kernel-driver
   distribution requires classic SCM/PnP installation with WHQL /
   Hardware Dev Center attestation signing. Our product's RDA driver
   component (`rda.sys`, service `RDA`, `HKLM\SYSTEM\CurrentControlSet\
   Services\RDA` + Parameters — `contracts/README.md`, `driver/scripts/
   install.ps1`) was therefore impossible under MSIX. **That component no
   longer exists: the product is user-mode only (commit 318b9655) — no
   driver, no service, no certificate-store changes, nothing loaded into
   the OS. This reason is retired.**

2. **The application itself cannot run inside the MSIX AppContainer —
   blocker #2, STILL TRUE.** The recovery engine's raw-disk transport is
   administrator-only on stock Windows: direct reads and writes via
   `\\.\PhysicalDriveN` / `\\.\HarddiskVolumeN` require an elevated
   token, and the product's truthful posture is to refuse every raw-disk
   verb with `ERR_ELEVATION_REQUIRED` (0x500C0001) when it is not
   elevated (the `\\.\RDA` SDDL half of the original argument is gone
   with the driver; the direct-handle half stands). An
   AppContainer-packaged app cannot host an elevated full-trust process
   of this shape, and a brokered elevation model for raw disk access
   would be a product redesign, not a packaging task.

## Where this stands now

The original decision ("MSIX is dropped entirely; the NSIS installer is
the single distribution artifact, with the portable zip for the
no-installer case") was made when BOTH blockers stood. Blocker #1 is
gone; blocker #2 remains. The honest state:

- **Not decided anew.** Whether an MSIX shape is worth building around
  the surviving elevation blocker is a future session's call; the
  consolidation session changed the facts (driver removal) without
  re-running the packaging decision.
- The current distribution truth (3-artifact set: `-setup.exe`,
  `-portable.zip`, `SHA256SUMS`) is documented in `docs/RELEASE.md`.

## Where this is recorded

- `docs/RELEASE.md` — user-facing artifact list and the "MSIX is not
  provided" statement.
- `docs/PARITY-DELTAS.md` — the packaging-technology deviation row
  (reference uses Inno Setup; we use NSIS; MSIX excluded for the reasons
  above, as of the original assessment).
- Release notes template (release workflow) — no MSIX artifact listed,
  signing posture stated truthfully.

## If MSIX ever becomes possible

The blockers were structural (driver deployment + elevation model); the
driver half is now gone. A future MSIX shape would still require at
minimum: a brokered elevation model for raw disk access
(a requireAdministrator exe does not fit an AppContainer — the
manifest posture is incompatible with MSIX's brokered model), and an
AppContainer-compatible write surface for
recovery output. That is a product redesign, not a packaging task —
out of scope for the sessions recorded here, kept in this document so
the question never has to be re-litigated blind.
