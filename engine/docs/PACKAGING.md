# PACKAGING — the app-installer contract (the packaging milestone's design input)

> **Status: IMPLEMENTED (S-038 step 2+3 landed; step 4 acceptance = the
> three gates green on the installer CI lane).** This document exists
> because [S-038](../diff-to-fix/findings/SEVERE/S-038-app-installer-surface-absent.md)
> put the reference product's installer surface on the board as a 13-parameter
> contract. The packaging milestone is built AGAINST this contract, never
> invented from scratch: every parameter below carries its corpus citation
> (string-table verbatim or artifact line), and every deliberate divergence
> is marked REGISTERED with its rationale. Both formerly-open decisions are
> CLOSED (confirmed by the implementing session per the contract's own
> defaults; the owner may overturn either by reopening the row):
> **§2 technology = Inno Setup** (confirmed), **§8 telemetry = omit**
> (confirmed). The implementation lives in
> [installer/](../installer/README.md) (`core-recovery.iss` + the gate
> runners; the three §11 gates run in
> [installer.yml](../.github/workflows/installer.yml)).
>
> Scope: the APP installer (the product's install/uninstall surface). The
> DRIVER deployer (rda-driver's `install.ps1`/`uninstall.ps1` + the
> [DRIVER-RELEASE](DRIVER-RELEASE.md) contract) is a separate, already-shipped
> surface that this milestone ABSORBS as an internal step (§10) — it does not
> replace it.

## 1. The reference's install architecture (what the corpus shows)

The reference product ships a silent-capable, scriptable install/uninstall
chain (all citations verbatim from the corpus string tables):

```
user runs setup (Inno Setup wizard)
  └─ SetupUE.exe (the pre-install bootstrap)
       ├─ OS-version gate: a 16-entry table, "Microsoft Windows 2000"
       │   through "Microsoft Windows Server 2016", edition variants
       │   (Standard/Enterprise/Datacenter/Web/Compute Cluster/Storage/
       │   Itanium/x64), via GetNativeSystemInfo + RtlGetNtVersionNumbers
       │   (native, WOW64-aware)
       ├─ reads/writes HKLM\SOFTWARE\EASEUS\DRW\Version
       ├─ spawns InfoForSetup.exe (the install-info companion:
       │   Aliyun upload channel — AliyunSendInfo/AliyunUninstallStart/
       │   AliyunUninstallEnd)
       └─ owns the "DRW_Installer" window (the EuSendMessage WM_COPYDATA
           cmd-999 status channel, WinPE contexts)
  └─ Inno Setup proper: copies the tree, writes unins000.{exe,dat,msg}
       └─ ARP entries: UninstallString + QuietUninstallString (" /SILENT")
```

Corpus citations: `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`
(the bootstrap-pair decode); `re-workspace/reversed/SetupUE.exe/strings_utf16.txt`
(the OS table, `SOFTWARE\EASEUS\DRW\Version`, `InfoForSetup.exe`, and the
`/SendInfo "Window" "Install" "Activity" "Info_Userinfo" "Attribute" "` +
`"Country":"` + `","Timezone":"` telemetry command form);
`re-workspace/reversed/InfoForSetup.exe/strings_ascii.txt`
(`/SendInfo`, `/SendInfoEx`, `/UninstallStart`, `/UninstallEnd`, the Aliyun*
counterparts, `AliyunEnableUserInfoCollect`);
`re-workspace/reversed/unins000.exe/strings_utf16.txt` (the Inno metadata keys,
the ARP strings, the silent flags, `unins%.3d`, the admin requirement, the GAC
assembly-removal strings); `architecture/02-kernel-driver-architecture.md`
§99-105 (the lifecycle frame).

**The 13-parameter contract** (S-038's enumeration — K74/K76/K77):

| # | Parameter | Reference behavior | Citation |
|---|---|---|---|
| 1 | `/SILENT` | the unattended install/uninstall flag; ARP `QuietUninstallString` ends `" /SILENT"` | unins000 strings |
| 2 | `/VERYSILENT` | the no-UI-at-all tier | unins000 strings |
| 3 | `/SUPPRESSMSGBOXES` | suppress message boxes under /SILENT ("Defaulting to %s for suppressed message box (%s):") | unins000 strings |
| 4 | `/NORESTART` | never reboot | unins000 strings |
| 5 | `/NORESTARTAPPLICATIONS` | don't restart running apps the installer closed | unins000 strings |
| 6 | Admin-required uninstall | `"Uninstaller requires administrator: %s"` | unins000 strings |
| 7 | ARP `UninstallString` | the standard uninstall entry (Add/Remove Programs) | unins000 strings |
| 8 | ARP `QuietUninstallString` | the silent form management tooling keys on, ending `" /SILENT"` | unins000 strings |
| 9 | `unins%.3d` multi-instance | secondary uninstallers (`unins001.exe`...) for shared/multi-copy installs | unins000 strings |
| 10 | Inno metadata keys | `Inno Setup: App Path / Icon Group / No Icons / Setup Type / Selected Components / Deselected Components / Selected Tasks / Deselected Tasks / User Info: Name / Organization / Serial / Setup Version / Language` | unins000 strings |
| 11 | Install telemetry | `/SendInfo` + `/SendInfoEx` with fields Window/Install/Activity/Info_Userinfo/Attribute/Country/Timezone (+ Test_id A/B bucketing); upload via InfoForSetup's Aliyun channel | SetupUE + InfoForSetup strings |
| 12 | Uninstall telemetry | `/UninstallStart` + `/UninstallEnd` (via the Aliyun channel) | InfoForSetup strings |
| 13 | Version registry key | `HKLM\SOFTWARE\EASEUS\DRW\Version` — written and read back by the bootstrap | SetupUE strings |

## 2. Technology decision (CLOSED — Inno Setup confirmed)

The reference used **Inno Setup** (the `unins000` trio and the Inno metadata
keys are Inno's signature — the corpus's own conclusion). The options for the
merged product:

| Option | Strengths | Weaknesses |
|---|---|---|
| **Inno Setup (the reference's choice)** | the 13-parameter contract maps 1:1 (all of §1 is native Inno behavior + `[Setup]`/`[Code]` sections); `unins%.3d`, the ARP strings, and the metadata keys come for free; ISS is scriptable/diffable | Windows-only; the OSS-signing pipeline must sign setup.exe too |
| **WiX (MSI)** | enterprise-standard (GPO/SCCM deployment); transactional install | MSI's ARP + silent model differs from Inno's flag set — parameters 1-10 need re-mapping; higher build complexity |
| **Custom (NSIS/MSIX/zip+script)** | full control | reinvents every parameter; the corpus already paid the RE cost of the Inno surface |

**DECISION: Inno Setup** — the contract's default recommendation, adopted by
the implementing session (evidence-pinned; the 13-parameter contract maps
1:1). The implementation: `installer/core-recovery.iss` (Inno 6.3+; the windows-2022
runner image ships 6.7.1 preinstalled, choco is the fallback).

## 3. The ARP contract (parameters 7-8)

Both entries are REQUIRED — this is the enterprise-deployable surface:

- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<ProductId>`:
  - `UninstallString` = the interactive uninstall command
  - `QuietUninstallString` = the silent uninstall command, ending `" /SILENT"`
    (the reference's exact form)
  - plus the standard `DisplayName`/`DisplayVersion`/`Publisher`/
    `InstallLocation`/`EstimatedSize` values
- The silent round-trip is a TEST GATE (§11): install → ARP present →
  quiet-uninstall → no residue (no files, no ARP entry, no version key, no
  driver service — unless a scan is running).

## 4. The silent flag set (parameters 1-5)

The installer MUST accept the reference's exact five flags with the reference's
semantics: `/SILENT` (unattended, UI suppressed except progress),
`/VERYSILENT` (no UI at all), `/SUPPRESSMSGBOXES` (no message boxes under the
silent tiers — a documented default is chosen per suppressed box, as the
reference's "Defaulting to %s for suppressed message box (%s):" shows),
`/NORESTART` (never reboot; the installer must not set
`REBOOTREQUIRED`), `/NORESTARTAPPLICATIONS` (don't restart closed apps).
Exit codes are nonzero on failure (never success-on-failure — the Phase-1
M-062 lesson applied at the app tier).

## 5. The admin requirement (parameter 6)

Install and uninstall both require elevation. The uninstaller enforces it
itself (the reference's `"Uninstaller requires administrator: %s"`), and the
installer's UAC manifest (`requestedExecutionLevel="requireAdministrator"`)
matches. The failure path is a loud message + nonzero exit, never a partial
install without the rights (the Phase-1 S-036/M-062 exit-code lessons are
the precedent).

## 6. The OS gate (modernized)

The reference's 16-entry table stops at Server 2016. The merged product's
gate (REGISTERED divergence, by evidence): the **documented target envelope
is Win7 SP1 → Win11 + Server 2025** (the same envelope the driver's
[DRIVER-RELEASE](DRIVER-RELEASE.md) §7 environments contract carries — the
PE-header contract gates the BINARY; the installer gate is the deployment
surface). The gate mechanism stays the reference's: native version probing
(`RtlGetNtVersionNumbers`/`GetNativeSystemInfo`, WOW64-aware), a table of
supported major.minor.build ranges with edition strings for the failure
message, and a REFUSAL (not a warning) outside the envelope. x64-only
(the reference's Itanium/Compute-Cluster rows have no successor meaning).

## 7. The version key (parameter 13)

`HKLM\SOFTWARE\<Vendor>\<Product>\Version` — written at install, read back
(verified) before the ARP entry is written. The reference's path
(`SOFTWARE\EASEUS\DRW\Version`) is the SHAPE; the merged product's path is
**REGISTERED (implemented): `HKLM\SOFTWARE\heicfast\core_recovery\Version`**
(vendor `heicfast` — the same vendor string the driver's version resource
carries; product `core_recovery` — the repo/product identity). The
write+verify (not write-and-hope) is the M-064 lesson applied at the app
tier: the ISS `[Code]` write is followed by a read-back comparison, and a
mismatch FAILS the install. The value is removed by the uninstaller's own
`[Code]` hook (Inno only uninstalls `[Registry]`-section entries — the
residue the round-trip gate exists to catch).

## 8. The telemetry posture (CLOSED — omit, confirmed)

The reference's installer reports install and uninstall events (parameters
11-12: `/SendInfo`//`/SendInfoEx` with locale/environment fields +
`/UninstallStart`//`/UninstallEnd`, via the Aliyun channel). The merged
product's posture is a DECISION, not an audit question (S-038's own framing):
**implement** (an opt-in/no-default telemetry with a documented endpoint),
**no-op** (the verbs exist as local no-ops so script parity is preserved), or
**omit with rationale**. **DECISION: omit** — the merged product has no
telemetry endpoint today, and a stub verb that pretends to upload is worse
than absence (the contract's default recommendation, adopted by the
implementing session; parameters 11-12 are therefore intentionally absent
from the implementation, recorded in the ISS header's contract map).

## 9. The multi-instance policy (parameter 9)

The reference supports shared/multi-copy installs via `unins%.3d` secondary
uninstallers. The merged product's default: **single instance** (one
`ProductId`, one ARP entry, an in-place upgrade path), with the multi-instance
capability REGISTERED as out-of-scope-until-needed — a data-recovery tool's
second copy on the same machine adds no capability (the driver service name
collision is the Phase-1 M-064-remainder's territory, not the app's).

## 10. The driver install as an internal step

The rda-driver's `install.ps1`/`uninstall.ps1` (v0.2.3+, the Phase-1 hardened
scripts: verified elevation guard, loud start-failure, verified teardown) run
as an INTERNAL step of the app install — inheriting the silent posture, the
elevation, and the exit-code discipline; the console-posture gap (N-085)
closes here. The scripts stay the source of truth (the app installer calls
them; it does not reimplement SCM). The M-064 remainder (the `%s%d` versioned
coexistence + the `-Uninstall` verb) lands upstream first — this milestone
consumes it, it does not fork it.

## 11. The test gates (from S-038's fix plan)

Three automated gates on the CI Windows lane, red-blocking the milestone:

1. **InstallerSilentRoundTrip** — clean-VM install (zero prompts under
   /VERYSILENT) → ARP `UninstallString` + `QuietUninstallString` present and
   well-formed → silent uninstall → no residue (files, ARP, version key,
   driver service).
2. **InstallerVersionKeyWritten** — the version key's write+verify path.
3. **InstallerOSGateMatrix** — the gate admits the supported envelope and
   REFUSES outside it (a scripted outside-range probe).

## 12. Milestone acceptance

- This contract exists with citations (this document).
- The three §11 gates green on the CI Windows lane — **the lane exists
  ([installer.yml](../.github/workflows/installer.yml)); the gates run on
  every push touching `installer/**`; S-038's step-4 acceptance flips when
  the first dispatch is green**.
- Every REGISTERED divergence above carries its rationale; the two OPEN
  decisions (§2 technology, §8 telemetry) are closed and recorded (both
  confirmed at the contract's own defaults by the implementing session).
