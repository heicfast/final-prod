# MAJOR M-060 — the LIVE install path (composite fallback) configures the service differently from the documented path: invented group name, no Parameters registry values, relative binPath

**Severity:** MAJOR
**Module / Surface:** driver install / dev-sign path (service-configuration parameters of the packaging surface)
**Parameters affected:** service group; `Parameters\IoctlBase`; `Parameters\MaxDisks`; `Parameters\DebugLevel`; `Parameters\SessionsEnabled`; ImagePath form (absolute vs relative)
**Corpus references:** `easeus-drw-research/og-codebase/BUILDPE/x64/regdrv.reg` — the reference's service registrations: eprdtdrv/epmdkdrv `Type=1, Start=3, ErrorControl=1` with **no Group** for eprdtdrv/epmdkdrv, and `ebrntdrv` carries **`"Group"="Boot File System"`**; the EPM WinPE pair (EUDCPEPM/EUEDKEPM) carries `DeviceName` + `IoctlBase=0x10000` — the only registry parameters the corpus documents on any of these services; `easeus-drw-research/re-workspace/artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` (the reference eprdtdrv reads no Parameters — the rda Parameters block is our driver contract, not the og's)
**Core references:** `.github/actions/install-driver/action.yml:76-90` (the live fallback: `group= "FSFilter Boot File System"`, `binPath= "System32\drivers\rda.sys"`, no Parameters written); the documented path — rda-driver v0.2.2 `install.ps1:57-65` (verbatim copy at `diff-to-fix/evidence/executions/devsign-run-40549660074/release-scripts/install.ps1`: `group= "Boot File System"`, absolute `$destPath`, four Parameters values `IoctlBase=0/MaxDisks=200/DebugLevel=1/SessionsEnabled=1`) and `rda.inf:45-66` (same four defaults, `ServiceBinary=%12%\rda.sys`); audited commit `dd82db9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope (the composite action postdates the board close; no prior finding covers CI install-path service configuration)

## Summary

Because the documented installer crashes (S-036), every actual install on record goes through the composite's `sc.exe` fallback — and that fallback creates the service with a **different configuration** than the documented path: an **invented service group** (`"FSFilter Boot File System"` — appears nowhere in the corpus and nowhere in the repo's own docs; install.ps1 and the og's regdrv.reg both use `"Boot File System"` for ebrntdrv, and the INF doesn't set a group), **no `Parameters` registry values at all** (the four-value rda contract `IoctlBase/MaxDisks/DebugLevel/SessionsEnabled` is written by install.ps1 and the INF but not by the fallback), and a **relative binPath** (`System32\drivers\rda.sys`) where the documented path writes the absolute path.

## What the reference does

The corpus's only documented service registrations for this driver family are `regdrv.reg`'s WinPE entries: kernel driver (`Type=1`), demand start (`Start=3`), normal error control, absolute `ImagePath` (`\SystemRoot\system32\<name>.sys` hex(2)), and — for `ebrntdrv` only — `Group="Boot File System"`. No `Parameters` key exists on any of the eprdtdrv-family services; the decompiled eprdtdrv internals show no registry-parameter reads. The EPM pair (EUDCPEPM/EUEDKEPM) is the only one carrying registry values (`DeviceName`, `IoctlBase=0x10000`), and those belong to the WinPE hook-driver deployment, not the raw-access service profile. On live systems the og's setupdrvx64.exe creates + starts the service through the SCM APIs (02-kernel-driver-architecture.md:101-103); the corpus documents no group divergence and no tunables for that service.

## What our implementation does

Two install paths exist in the merged repo's CI, and they disagree:

| Parameter | Documented path (`install.ps1` v0.2.2 / `rda.inf`) | **Live path (composite fallback, action.yml:76-90)** | Reference (regdrv.reg ebrntdrv) |
|---|---|---|---|
| group | `"Boot File System"` | **`"FSFilter Boot File System"`** | `"Boot File System"` |
| binPath/ImagePath | absolute (`C:\Windows\System32\drivers\rda.sys`) | **relative (`System32\drivers\rda.sys`)** | absolute (`\SystemRoot\...`) |
| `Parameters\IoctlBase` | `0` | **not written** | n/a (no Parameters) |
| `Parameters\MaxDisks` | `200` (0xC8) | **not written** | n/a |
| `Parameters\DebugLevel` | `1` | **not written** | n/a |
| `Parameters\SessionsEnabled` | `1` | **not written** | n/a |

The live run's driver-status output (`state: ready`, `max_disks:200`, `ioctl_base:0`, `flags:2`) shows the driver's compiled-in defaults happen to equal the documented defaults today — so the divergence is currently value-neutral, but the **source of truth is now the driver binary's compiled defaults rather than the documented installer/INF contract**, and the fallback cannot express any non-default configuration (`install.ps1 -MaxDisks N`, `-IoctlBase N` are unreachable). A demand-start service's group is load-ordering metadata only, so the invented `"FSFilter Boot File System"` is inert in practice — but it is a value that exists nowhere in the corpus, the release, or the docs, and it silently diverges from what the docs tell a consumer they got.

## Gap

Falsifiable claim: **the service installed by the merged repo's live CI path has group `FSFilter Boot File System`, a relative ImagePath, and no `Parameters` key, while the documented installer (and the INF) specify group `Boot File System`, an absolute path, and four Parameters values; the corpus's reference registration uses `Boot File System` and no Parameters.**

## Impact

- **Configuration truth**: CI evidence (green install) no longer describes the configuration the release's own installer/INF would produce; anything auditing "what does a CI-installed rda service look like" sees an undocumented shape.
- **Tunability dead-end**: the documented knobs (`-MaxDisks`, `-IoctlBase`, `-DebugLevel`, `-NoStart` install.ps1 parameters) can never be exercised through the live path — a future need for >200 disks or a non-zero IOCTL base would silently not apply.
- **Diagnostic drift**: `sc qc rda` on a CI machine shows a group name that matches no documentation, sending an engineer to chase a non-existent "FSFilter" association (the rda driver is not an FSFilter-class driver; the corpus's FSFilter-adjacent entity is the EUDCPEPM hook, a different deployment profile).
- No current functional breakage (values coincide with compiled defaults; group is inert for demand-start) — hence MAJOR, not SEVERE.

## Reproduction / verification

Executed comparison (strength-1): run 40549660074 — the fallback created the service (job-log.txt:236-237 `[SC] CreateService SUCCESS` under the `group= "FSFilter Boot File System"` command line printed at :210); the driver-status probe (:279) reports `max_disks:200, ioctl_base:0, flags:2` — i.e. compiled defaults, since no Parameters key was written. Source comparison: `action.yml:76-90` vs `release-scripts/install.ps1:57-65` vs `release-scripts/rda.inf:62-66` (both verbatim copies of the v0.2.2 assets), vs corpus `regdrv.reg` (ebrntdrv `Group="Boot File System"`).

## Fix plan (for the engineering team)

1. Make the composite's fallback byte-parity with `install.ps1`'s documented shape: `group= "Boot File System"`, absolute `binPath= "$env:SystemRoot\System32\drivers\rda.sys"`, and write the four `Parameters` values (`IoctlBase=0`, `MaxDisks=200`, `DebugLevel=1`, `SessionsEnabled=1`) — or better, surface `install.ps1`'s own parameters as composite inputs (`max_disks`, `ioctl_base`) so the knobs are reachable.
2. Once S-036's upstream fix lands (v0.2.3+), the fallback becomes the drill-only path; keep it, but gate it behind an explicit input and assert parity with a `sc qc` + `reg query` check in both branches.
3. Add a one-line doc note to `docs/PREBUILT-ARTIFACTS.md` §2 stating the enforced service shape (group + Parameters) so the contract is written down in the core repo, not only implied by the release scripts.
4. Acceptance criteria: `sc qc rda` after a green install shows `GROUP: Boot File System` and an absolute BINARY_PATH_NAME; `reg query HKLM\SYSTEM\CurrentControlSet\Services\rda\Parameters` shows the four values; the two paths (install.ps1 vs fallback) produce byte-identical `sc qc` + Parameters output.

## Tests to add

- Composite step: `sc qc rda` assertions (group, path form) + `reg query` assertions for the four Parameters values — runs after either install branch.
- Parity test: run both install paths on two runners (or sequentially with uninstall between) and diff `sc qc` + `reg query` output — must be empty.
- A `-MaxDisks 8` drill install (pinned tag) asserting the driver reports `max_disks:8` proves the knob reaches the binary.

## Related findings

- **S-036** — why the fallback is the live path at all.
- **M-062**, **M-061** — the other defects in the same release scripts.

## Open questions

None.
