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

## Fix progress (step 1 of the fix plan — LANDED)

**Step 1 (the contract doc) is complete** (this session's S-038-step-1
commit): `docs/PACKAGING.md` carries the 13-parameter contract, every
parameter cited to the verbatim string-table evidence (the telemetry field
form lives in SetupUE's strings — `/SendInfo "Window" "Install" "Activity"
"Info_Userinfo" "Attribute" "` + `"Country":"` + `","Timezone":"` — with
InfoForSetup carrying the Aliyun upload channel `/SendInfo`, `/SendInfoEx`,
`/UninstallStart`, `/UninstallEnd`; the GAC assembly-removal strings
(CreateAssemblyCache/InstallAssembly/UninstallAssembly) are additionally
documented). The technology decision (default: Inno Setup), the modernized
OS gate (REGISTERED divergence past Server 2016), the telemetry posture
(default recommendation: omit), and the multi-instance default
(single-instance) are recorded as the milestone's decisions-with-defaults.
**Status remains READ-ONLY-FINDING-with-step-1-landed**: steps 2-4
(implementation, the three test gates, acceptance) are the milestone's
remaining work; the finding moves to FIXED when the §11 gates are green.

## Fix progress (steps 2-3 of the fix plan — LANDED; step 4 = the CI proof)

**Steps 2 and 3 are complete** (the PARITY-FIX-1 session):

- **Step 2 (the installer)**: `installer/core-recovery.iss` — the Inno
  Setup implementation of the 13-parameter contract (the header comment maps
  every parameter to its section). Parameters 1-5/7/10 are native Inno;
  parameter 8 (ARP `QuietUninstallString`, the reference's exact `" /SILENT"`
  form) and parameter 13 (`HKLM\SOFTWARE\heicfast\core_recovery\Version`)
  are written AND read-back-verified by `[Code]` (the M-064 lesson), with
  the version key's removal owned by the uninstaller's own `[Code]` hook
  (Inno only uninstalls `[Registry]` entries). Parameter 6 is the admin
  manifest (`PrivilegesRequired=admin`). Parameter 9 is the frozen
  single-instance AppId (§9's default). Parameters 11-12 (telemetry) are
  OMIT per §8's confirmed decision. The OS gate (§6, the modernized
  Win7-SP1→Server-2025 x64 envelope, REGISTERED divergence) lives in
  `InitializeSetup` with the SAME decision table as `installer/osgate.ps1`
  (the OSGATE-V1 marker is the sync pin). The driver deployer (§10) runs as
  an INTERNAL step (`driver` component, default-checked): the bundled
  install.ps1 stays the source of truth, exit codes propagate (a failed
  driver install FAILS the app install), and the teardown runs before file
  deletion on uninstall.
- **Step 3 (the three test gates)**: `installer/run-gates.ps1` (gates 1+2 —
  the silent round trip: /VERYSILENT install → ARP both strings
  well-formed → uninstall via the ARP QuietUninstallString verbatim →
  no-residue assertions over files/ARP/version-key/service/driver-images;
  gate 2 is the version-key read-back) and
  `tools/test_installer_osgate.py` (gate 3 — the ISS↔module sync pin plus
  the injectable version matrix: every in-envelope OS admits exit-0, every
  below-floor tuple (XP/Vista/Win7-RTM) and non-x64 arch refuses exit-3).
  The lane: `.github/workflows/installer.yml` (gate 3 fast job + the
  full build→sign→install→uninstall round-trip job on windows-2022, Inno
  pinned 6.4.3 via choco, driver test-signed with the proven job-local
  cert mechanism). The installer PS files joined
  `scripts/ps_parsecheck.ps1`.
- **The two owner decisions are CLOSED at the contract's own defaults**:
  §2 = Inno Setup, §8 = omit (the owner may overturn either by reopening
  the PACKAGING.md row; the implementation maps to either choice).

**Status: steps 2-3 landed; step 4 (acceptance) flips when the installer
lane's first dispatch is green — the lane runs on every push touching
`installer/**`.**

## Resolution (step 4 — ACCEPTED; the PARITY-FIX-9 close-out)

**The installer lane's first fully-green dispatch: run 40684304129 at
`1c4db82`** (2026-10-28, windows-2022, Inno 6.7.1 from the runner image).
All three §11 gates GREEN:

- **Gate 3 InstallerOSGateMatrix** (job `osgate-matrix`): the OSGATE-V1
  sync pin holds (module == ISS decision table) and all 17 matrix probes
  behave — 9 in-envelope ADMITS (Win7 SP1 7601 floor, Win8/8.1, Win10
  1507→22H2, Win11 21H2/24H2, Server 2022/2025) exit 0; 5 below-floor
  REFUSES (XP, Vista SP1/SP2, Win7 RTM 7600) exit 3; 3 wrong-arch REFUSES
  (x86, ARM64, Itanium) exit 3.
- **Gates 1+2** (job `silent-roundtrip`): all 18 assertions GREEN —
  install exit 0 (/VERYSILENT /SUPPRESSMSGBOXES /NORESTART, zero prompts);
  version key `HKLM\SOFTWARE\heicfast\core_recovery\Version` reads back
  exactly the built version; ARP entry with UninstallString +
  QuietUninstallString (ends `" /SILENT"`, the reference's exact form) +
  DisplayName/DisplayVersion/Publisher/InstallLocation; driver service RDA
  RUNNING under the Boot File System group (§10's internal step, the
  bundled install.ps1 with exit-code propagation); silent uninstall via
  the ARP QuietUninstallString verbatim; NO residue (ARP, install dir,
  version key, service, System32\drivers\RDA*.sys).

The two dispatch-era defects (both fixed in-session): the pinned-choco
downgrade failure (the runner image ships Inno 6.7.1 — detect-first) and
the QuietUninstallString double-quoting (Inno writes UninstallString
already quoted; the ISS now strips before composing the canonical form,
run 40682713693's gate-1d lesson — 14/14 install-side assertions were
already green there).

**S-038 moves to FIXED**: the 13-parameter contract is implemented
(`installer/core-recovery.iss`), the three gates are automated and green
(`.github/workflows/installer.yml` + `installer/run-gates.ps1` +
`tools/test_installer_osgate.py`), and both owner decisions are recorded
(PACKAGING.md §2 Inno Setup, §8 omit). The reference's installer surface
is no longer absent.
