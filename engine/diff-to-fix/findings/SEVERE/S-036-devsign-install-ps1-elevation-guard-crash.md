# SEVERE S-036 — the documented driver installer (release install.ps1) crashes at its elevation guard; only the composite's undocumented sc.exe fallback ever installs

**Severity:** SEVERE
**Module / Surface:** driver install / dev-sign path (packaging surface — the documented consumer install path per docs/PREBUILT-ARTIFACTS.md §2)
**Parameters affected:** installer-posture (documented install path executes at all); service-creation semantics (the og's SCM-API create+start, corpus 02-kernel-driver-architecture.md §5)
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:25,101-103` — the og installs its raw-disk driver with a native setup EXE (`PartitionRecovery/setupdrvx64.exe`, PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`) that "copies to `%SystemRoot%\System32\Drivers\%s.sys`" and "creates + starts the service (SCM APIs)" — an installer that performs its steps and reports failure; `og-codebase/BUILDPE/x64/regdrv.reg` (service values: Type=1/Start=3/ErrorControl=1)
**Core references:** `docs/PREBUILT-ARTIFACTS.md` §2 "Install posture on runners" (installs "via the release's own `install.ps1 -ServiceName rda -Source rda.sys`"); `docs/DRIVER-RELEASE.md` §1 step 8 + §2 (install.ps1 is a required release asset, "the service installer (sc create/start)"); `.github/actions/install-driver/action.yml:66-90` (invokes `powershell -ExecutionPolicy Bypass -File install.ps1`, then the fallback); the exact asset executed: `rda-driver` release **v0.2.2** `install.ps1` lines 38-41 (verbatim copy at `diff-to-fix/evidence/executions/devsign-run-40549660074/release-scripts/install.ps1`); audited commit `dd82db9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope (prior sessions audited the core engine; the prebuilt/driver-only CI era postdates the board close; no prior finding covers the release installer's executability)

## Summary

The merged product's documented driver install path runs the rda-driver release's `install.ps1`, and that script **crashes unconditionally at its elevation guard** — `You cannot call a method on a null-valued expression` (`InvokeMethodOnNull`) — before performing any install action. Every green CI install on record succeeds only through the composite action's *fallback* (`sc.exe create` + manual copy), i.e. an undocumented path that installs a differently-configured service (M-060). A developer following the documented path (`docs/PREBUILT-ARTIFACTS.md` §2 / `docs/DRIVER-RELEASE.md`) on a fresh machine gets a hard script crash and no driver.

## What the reference does

The og product installs its raw-disk kernel driver with a purpose-built native installer, `PartitionRecovery/setupdrvx64.exe` (PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`), which per the corpus's driver-architecture document (a) copies the driver to `%SystemRoot%\System32\Drivers\%s.sys` and (b) creates and starts the service through the SCM APIs (02-kernel-driver-architecture.md:101-103). The corpus knows of no script crash in this path — the installer's steps are enumerated as working behavior, and the WinPE variant pre-registers the same service values through `regdrv.reg` (Type=1, Start=3, ErrorControl=1). The reference installer performs exactly its documented steps; installation is orchestrated by the product itself at app start (§5.1) and torn down at exit (§5.3).

## What our implementation does

The merged repo consumes the rda-driver release's `install.ps1` as "the service installer" (docs/DRIVER-RELEASE.md §1 step 8) and documents installing "via the release's own install.ps1 -ServiceName rda -Source rda.sys" (docs/PREBUILT-ARTIFACTS.md §2). The composite action `.github/actions/install-driver/action.yml:66-90` executes it as `powershell -ExecutionPolicy Bypass -File install.ps1 -ServiceName rda -Source <path>\rda.sys -IoctlBase 0`. In the verified run (40549660074, windows-2022, driver tag v0.2.2):

```
D:\a\core_recovery\core_recovery\driver\install.ps1 : You cannot call a method
on a null-valued expression.
    + CategoryInfo          : InvalidOperation: (:) [install.ps1], RuntimeException
    + FullyQualifiedErrorId : InvokeMethodOnNull,install.ps1
install.ps1 failed (1) - manual sc.exe fallback (the proven path from the api-benchmark lane)
[SC] CreateService SUCCESS
```
(job-log.txt:232-237)

The crash site is the elevation guard, `install.ps1` lines 38-41:
```powershell
$identity = [Security.Principal.WindowsPrincipal]::[Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $identity.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
```
The nested type-literal static access `[WindowsPrincipal]::[WindowsIdentity]` does not resolve to a valid static member under Windows PowerShell 5.1 (the interpreter the composite invokes via bare `powershell`), so the subsequent `::GetCurrent()` invocation targets a null value. Placement is proven by elimination: the script's first output (`Write-Host "Installing ..."`, line 53) never appears in the transcript, and lines 38-41 contain the only method call before it. The script aborts before preflight, copy, service creation, or start — nothing of the documented install happens.

The composite then rescues the run with its inline fallback (`Copy-Item` + `sc.exe create rda type= kernel start= demand binPath= System32\drivers\rda.sys group= "FSFilter Boot File System"` + `sc.exe start`), which the comment itself describes as "the proven path from the api-benchmark lane" — i.e. the fallback, not the documented installer, has been the real install mechanism all along.

## Gap

Falsifiable claim: **executing the documented installer (`powershell -ExecutionPolicy Bypass -File install.ps1 -ServiceName rda -Source rda.sys`) on Windows exits 1 with `InvokeMethodOnNull` at the elevation guard before copying anything or creating the service.** Verified on windows-2022 (run 40549660074); the crash is unconditional (the failing expression is a static parse/resolution failure, not environment-dependent), and the run record shows the fallback has masked it since the api-benchmark era.

## Impact

- **Developers and downstream consumers** following `docs/PREBUILT-ARTIFACTS.md` §2 or `docs/DRIVER-RELEASE.md` cannot install the driver at all — the documented path is a hard crash (install-blocking for the documented route; CI is green only via the fallback, which hides the defect).
- **CI truthfulness**: the composite's fallback converts an installer regression into a silent mode switch — the service that gets installed differs from the documented one (see M-060 for the configuration divergence), so CI evidence does not describe the artifact consumers are told to use.
- **Uninstall symmetry**: the crashed installer never creates the service, so failure recovery on a half-installed state (e.g. a manually pre-created service name) is never exercised.
- Not a data-path defect: the driver binary itself loads and answers correctly once installed by the fallback.

## Reproduction / verification

Executed comparison (strength-1 evidence): GitHub-hosted windows-2022, run 40549660074, `driver-install.yml` (workflow_dispatch) — full transcript at `diff-to-fix/evidence/executions/devsign-run-40549660074/job-log.txt:229-249`, the exact `install.ps1` asset (rda-driver v0.2.2, SHA256SUMS-verified in-run) at `.../release-scripts/install.ps1`. The prior three dispatches (runs 40537187339, 40537808535, 40538949278) failed before reaching install for unrelated workflow-step reasons (fixed by the `exit 0` commits), so run #4 is the first install-phase execution on record — it both proves the crash and proves the fallback works.

## Fix plan (for the engineering team)

1. **Fix the elevation guard in the rda-driver release's `scripts/install.ps1`** (upstream of the core repo): replace line 38 with a valid form, e.g.
   `$identity = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())`
   and add a PowerShell 5.1 + pwsh 7 matrix check to its CI (the same fix applies to `uninstall.ps1:15`, which carries the identical expression).
2. Cut `rda-driver v0.2.3` with the repaired scripts (byte-comparable `rda.sys` is fine — "No binary change" precedent: v0.2.2).
3. In core_recovery, flip the composite's posture once the fixed release is the resolved latest: **fail loudly when `install.ps1` fails** instead of silently falling back (keep the fallback behind an explicit `allow_fallback` input for pin-drills), so a broken installer can never again hide behind CI.
4. Add a step asserting the installed service's group + `Parameters` values match the documented ones (closes the M-060 detection gap at the same time).
5. Document in `docs/PREBUILT-ARTIFACTS.md` §2 that the floor for a script-fixed release is v0.2.3 (extend the `FloorVersion` argument in the composite's fetcher call).

Acceptance criteria: a fresh windows-2022 dispatch of `driver-install.yml` shows `install.ps1` completing (no fallback message), `sc qc rda` reporting `GROUP: Boot File System` and the four `Parameters` values present, and a deliberate `install.ps1` failure (e.g. missing source) turning the workflow red.

## Tests to add

- CI guard: assert the job log does NOT contain "manual sc.exe fallback" (fails today, passes after the fix).
- `sc qc rda` + `reg query ...\Services\rda\Parameters` assertions in the composite (group + 4 values).
- rda-driver side: a Pester/unit test invoking the elevation guard expression under `powershell` (5.1) and `pwsh` (7) to prevent reintroduction.
- A drill dispatch pinned to a pre-v0.2.3 tag must FAIL (proves the loud-failure posture).

## Related findings

- **M-060** — the fallback's divergent service configuration (group name, missing Parameters) is what actually lands on machines today.
- **M-062** — the same script's start-failure path exits 0; fix both in one release.
- **M-061** — `uninstall.ps1` carries the same broken guard expression and is never exercised.
- **I-026** — the unsigned-by-design posture this installer's docs assume.

## Open questions

None. (The exact PowerShell parse semantics of `[Type]::[Type]::member` under 5.1 vs pwsh 7 are not load-bearing for this finding — the executed crash is transcript-proven either way; the fix avoids the pattern entirely.)
