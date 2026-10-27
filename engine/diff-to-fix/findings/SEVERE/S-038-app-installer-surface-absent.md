# SEVERE S-038 — the app-level installer surface is entirely absent: the reference ships a silent-capable Inno Setup install/uninstall contract (ARP QuietUninstallString, /VERYSILENT, OS gate, telemetry verbs, version key) that the merged product has no counterpart for at all

**Severity:** SEVERE
**Module / Surface:** packaging / app installer (new module row: modules/driver-install.md's app-installer section; PARAMETERS Group K — K74, K76, K77)
**Parameters affected:** K74 (silent/unattended surface), K76 (versioned coexistence — unins%03d multi-instance uninstallers), K77 (install-time registry + telemetry contract)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` (the whole artifact — the bootstrap pair decoded); `re-workspace/reversed/unins000.exe/strings_utf16.txt` (verified this session): `" /SILENT"`, `QuietUninstallString`, `UninstallString`, `"Uninstaller requires administrator: %s"`, `unins%.3d`, `VerySilent`, `SuppressMsgBoxes`, `/Silent`, `/VerySilent`, `NoRestart`, `NoRestartApplications`, the Inno metadata keys ("Inno Setup: App Path / Icon Group / ... / User Info: Name / Organization / Serial / Setup Version"); `re-workspace/reversed/SetupUE.exe/strings_utf16.txt` (verified): `SOFTWARE\EASEUS\DRW\Version`, `InfoForSetup.exe`, the 16-entry OS table ("Microsoft Windows 2000" … "Microsoft Windows Server 2016"); `re-workspace/reversed/InfoForSetup.exe/strings_utf16.txt` (verified): `/SendInfo`, `/SendInfoEx`, `/UninstallStart`, `/UninstallEnd` (ascii table: the Aliyun* upload counterparts); `artifacts/Licensing/licensing-activation-chain.md:29,:45-46` (the serial-capture + uninstall-retention funnels); `architecture/02-kernel-driver-architecture.md:99-105` (§5 the lifecycle frame)
**Core references:** audited commit `dd82db9` — no app-installer surface exists: no Inno/ISS/WiX/MSI/setup script anywhere in the tree (verified by grep — the only hits are false positives in test/bench files), no ARP-writing code, no OS gate, no install telemetry, no version registry key; the product ships as the release zip (`docs/RELEASE.md`, `docs/PREBUILT-ARTIFACTS.md` §3) with the driver's `install.ps1` (Phase-1 findings S-036/M-060/M-061/M-062) as the only install story; `docs/roadmap.md` names no installer milestone
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope — the Installer/ artifact was NEVER-CITED by any prior finding (the session's corpus inventory); Phase 1 covered only the DRIVER install slice; N-085 registered the console-posture gap without the app-installer parameter set

## Summary

The corpus documents a complete, silent-capable app-installer surface for the reference product: an Inno Setup installer wrapped by a SetupUE bootstrap (16-entry OS-version gate, WOW64-aware) and an InfoForSetup telemetry helper, with an ARP contract that publishes both `UninstallString` and `QuietUninstallString` (ending `" /SILENT"`), full unattended flags (/SILENT /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NORESTARTAPPLICATIONS), admin-required uninstall, multi-instance uninstallers (`unins%03d`), Inno component/task/serial metadata, install AND uninstall telemetry verbs, and the `SOFTWARE\EASEUS\DRW\Version` registry contract. The merged product has none of this — not partially, entirely — and no roadmap row names it. This finding puts the 13-parameter contract on the board so the packaging milestone is built against the reference's actual surface instead of invented later.

## What the reference does

Per the Installer artifact + verified string tables: setup runs `SetupUE.exe` first — it gates the OS (a 16-entry table from Windows 2000 through Server 2016, editions included, via `GetNativeSystemInfo` + `RtlGetNtVersionNumbers`, WOW64-aware), reads/writes `HKLM\SOFTWARE\EASEUS\DRW\Version`, and launches `InfoForSetup.exe` (install telemetry: Activity/Attribute/Country/Timezone/Info_Userinfo/Install/Test_id fields; upload verbs `/SendInfo`, `/SendInfoEx`) — then the Inno wizard copies the tree and writes the `unins000.{exe,dat,msg}` trio plus the ARP entries carrying both `UninstallString` and `QuietUninstallString` (the silent form). Uninstall is equally scripted: `/SILENT`//`VERYSILENT`//`SUPPRESSMSGBOXES`//`NORESTART(+Applications)`, requires an administrator, supports `unins%03d` secondary uninstallers for shared/multi-copy installs, GAC assembly removal, and reports its own start/end via `/UninstallStart`//`/UninstallEnd` (the licensing layer even runs uninstall-time retention funnels — `UninstallDlgRetentionTrigger`, `value_uninstallretain172`). The Inno metadata keys record component/task selections and the user's name/organization/serial at install.

## What our implementation does

Nothing at the app level: the product is a CLI + libraries + `cr_ui.dll` shipped as a zip; the only install surface is the driver's PowerShell scripts (Phase 1). There is no installer EXE, no ARP contract, no OS gate, no silent flags, no telemetry verbs, no version key, no multi-instance story. The release zip's `MANIFEST.json`/`SHA256SUMS` conventions are integrity, not installation.

## Gap

Falsifiable claim: none of the 13 K74/K76/K77 parameters (enumerated in the corpus citations above) has any implementation, doc claim, or roadmap row in the merged product.

## Impact

- **Product-frame capability**: a customer-facing deployment of the merged product currently has no install/uninstall story at all beyond unzipping; enterprise deployment (the ARP/QuietUninstallString contract is what management tooling keys on) is impossible.
- **Roadmap honesty**: without this finding, the packaging milestone would be designed from scratch rather than against the reference's contract; the corpus has already paid the RE cost of enumerating it.
- Not a defect in shipped code — the surface does not exist to be wrong; hence SEVERE (missing user-observable capability the reference has), not ACUTE.

## Reproduction / verification

Corpus doc + string-table evidence (strength-3, every string quoted this session from the files under `re-workspace/reversed/{unins000.exe,SetupUE.exe,InfoForSetup.exe}/`): the citations above are verbatim. Core absence (strength-3): the grep results quoted in Core references.

## Fix plan (for the engineering team)

1. Create the packaging milestone's contract document from this finding's parameter list (the 13 parameters, each with its corpus citation) — `docs/PACKAGING.md` — before any installer code: technology choice (Inno Setup vs WiX vs custom), the ARP contract (both strings), the silent flag set, the admin requirement, the OS gate (modernized past Server 2016), the version key, the telemetry posture (the merged product's privacy stance decides: implement, no-op, or omit-with-rationale), and the multi-instance policy.
2. Implement the installer against that contract; the driver install (the Phase-1 scripts) becomes an internal step of the app installer, inheriting the silent posture (this supersedes the N-085 console-posture gap).
3. Tests: an automated clean-VM install (zero prompts, ARP entries present, `QuietUninstallString` well-formed), a silent uninstall round-trip (no residue), and an OS-gate matrix drill.
4. Acceptance criteria: the contract doc exists with corpus citations; the three tests green on the CI Windows lane.

## Tests to add

- `InstallerSilentRoundTrip` (install → ARP → quiet-uninstall → no residue).
- `InstallerVersionKeyWritten`; `InstallerOSGateMatrix` (drill).

## Related findings

- **S-036/M-060/M-061/M-062/N-085** (Phase 1) — the driver-install slice this surface will absorb.
- **M-064** (this session) — the driver deployer's versioned/verified contract, the same milestone's driver half.

## Open questions

Whether the merged product WANTS install telemetry (K77's /SendInfo family) is a product-privacy decision, not an audit question — the fix plan deliberately leaves it as implement/no-op/omit-with-rationale.
