# MAJOR M-064 — the driver deployer's versioned/verified contract is absent: no uninstall verb in the installer, no `%s%d` versioned coexistence, no registry write+verify, no per-call SCM error surfacing

**Severity:** MAJOR
**Module / Surface:** driver install / packaging (modules/driver-install.md; PARAMETERS K71, K72, K76)
**Parameters affected:** K71 (installer error semantics — per-call SCM error strings), K72 (service configuration — write+verify), K76 (versioned coexistence — the `%s%d` numbered layout)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3 (:35-38) — verified this session: "**Full SCM lifecycle**: `CreateService`/`OpenService`/`StartService`/`ControlService` with per-call error strings ('CreateService failed! Error = %d')"; "Installs to three layouts: `\System32\Drivers\%s.sys`, `\SystemRoot\System32\drivers\%s.sys`, `%s\System32\Drivers\%s%d.sys` — **the `%s%d` numbered variant is the driver versioning scheme** (eprdtdrv2.sys-style coexisting versions)"; "Service config: 'Boot File System' group ..., `-Uninstall` mode"; "Registry: `SYSTEM\CurrentControlSet\Services\%s` (write + verify)"; `architecture/02-kernel-driver-architecture.md:101-103` (the §5 frame)
**Core references:** audited commit `dd82db9` — the rda-driver v0.2.2 release scripts (verbatim copies at `diff-to-fix/evidence/executions/devsign-run-40549660074/release-scripts/`): `install.ps1` has **no `-Uninstall` mode** (its only uninstall story is the separate script), **no versioned layout** (always `<ServiceName>.sys` — one version per service name), **no registry write+verify** (it writes the `Parameters` values blind, lines 60-65, and never reads them back), and its SCM failures are either silent (M-062: start failure exits 0) or unattributed (`Write-Error "CreateService failed ($LASTEXITCODE)"` — one message, no per-call context); `uninstall.ps1` (M-061) has no version-awareness either
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope — the deepening-batch1 §3 decode was NEVER-CITED (the session's inventory); Phase 1's S-036/M-060/M-061/M-062 covered the crash/config/silent-failure axes, not the versioned/verified contract axes

## Summary

The reference's driver deployer is a version-aware, verified-write, two-way tool: it installs under a `%s%d` numbered layout so a new driver version coexists with the old (`eprdtdrv2.sys`-style), carries a `-Uninstall` mode in the same binary, verifies its `Services\%s` registry writes by reading them back, and surfaces every SCM call's failure with per-call context ("CreateService failed! Error = %d"). The merged product's release scripts have none of these four properties: one unversioned layout per service name, no uninstall verb in the installer, blind parameter writes, and error semantics already shown broken (M-062). The consequences compound: a driver update today means replacing the loaded file or hand-choosing a new service name, with no coexistence story and no verification that the service landed as written.

## What the reference does

The deepening-batch1 §3 decode (quoted in the citations): `setupdrvx64.exe` performs the full SCM lifecycle with per-call error strings; installs to three layouts including the `%s%d` numbered variant explicitly identified as the driver versioning scheme (coexisting versions); configures the service with the "Boot File System" group + DisplayName/ErrorControl/DeleteFlag; carries a `-Uninstall` mode; and writes `SYSTEM\CurrentControlSet\Services\%s` with verification (write + read-back).

## What our implementation does

`install.ps1` (v0.2.2, the audited asset): copies `<Source>` to `System32\drivers\<ServiceName>.sys` — a single unversioned path; `sc.exe create` with one generic error branch; writes four `Parameters` values with `Set-ItemProperty` and no read-back verification; no uninstall mode (the separate `uninstall.ps1`, itself unexercised per M-061); start failure exits 0 (M-062). Version updates require either overwriting the loaded image (locked until stop) or choosing a new service name by hand — no coexistence scheme exists. The composite's fallback (the live path per S-036) writes no Parameters at all (M-060).

## Gap

Falsifiable claim: none of the four versioned/verified-contract parameters (`-Uninstall` mode in the installer, `%s%d` versioned layout, registry write+verify, per-call SCM error strings) exists anywhere in the merged product's install path.

## Impact

- **Upgrades**: driver version N → N+1 has no story; the og's scheme deliberately keeps both binaries serviceable under distinct names so an in-use version is never clobbered.
- **Reliability**: a parameter write that silently fails (registry ACL oddities, hive issues) is never detected — the service starts with compiled defaults while the operator believes their configuration applied (compounding M-060's source-of-truth problem).
- **Diagnostics**: one generic "CreateService failed" where the og names the failing call and the code — the difference between a support ticket and a self-service fix.

## Reproduction / verification

Corpus doc evidence (strength-3): the deepening-batch1 §3 lines quoted verbatim above. Core evidence (strength-3): the v0.2.2 release scripts (the exact assets the validated run consumed — copies in the Phase-1 evidence directory) — no `-Uninstall` param in the param block, no numbered layout, no read-back after `Set-ItemProperty`, the single generic error branch quoted.

## Fix plan (for the engineering team)

1. In the rda-driver release scripts (the v0.2.3 batch with S-036/M-061/M-062's fixes): add `-Uninstall` to `install.ps1` (stop → verify stopped → delete → remove file → verify gone — the same verified sequence M-061's uninstall fix specifies, so one implementation serves both entry points).
2. Add `-Layout Versioned|Plain` (default Versioned for upgrades): Versioned copies to `<ServiceName><MajorMinor>.sys` and derives the service name accordingly, so `rda.sys` v0.2.3 coexists with an in-use v0.2.2 service; the core's driver-status probe already enumerates by service name (verify the discovery order prefers the highest version — the og's scheme implies newest wins for new sessions, old service left running).
3. After the Parameters writes, read all four values back and fail loudly on mismatch (K72's write+verify).
4. Wrap each SCM call (`create`/`start`/`stop`/`delete`) with its own named error ("sc create failed (%d) — <context>"), matching the og's per-call contract.
5. Tests: a two-version coexistence drill (install vA, install vB versioned, both services present, the newest answering the probe); a forced registry-mismatch drill (pre-create the Parameters key read-only → install must fail loudly); the -Uninstall round-trip from M-061's acceptance criteria, run through both entry points.
6. Acceptance criteria: the drills green on the CI Windows lane; the scripts' README documents the layout + the coexistence rule.

## Tests to add

- `DriverVersionedCoexistenceAB`.
- `DriverParametersWriteVerified` (fails before: blind write accepted).
- `DriverInstallUninstallVerbRoundTrip`.
- `DriverScmErrorsArePerCall` (a mocked-failure matrix asserts the message names the call).

## Related findings

- **S-036/M-060/M-061/M-062** (Phase 1) — the script-defect family this finding extends with the versioning/verification axes; one v0.2.3 release carries all the fixes.
- **S-038** (this session) — the app-installer surface that will absorb these scripts.

## Open questions

Whether the core's driver-status discovery (service enumeration by name) prefers the newest version automatically — the fix plan's step 2 says "verify"; if it enumerates only the fixed name `RDA`, the versioned layout needs the discovery update in the same change (one line in the probe's service-name resolution).
