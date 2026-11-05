# DEV-SIGN — development-signing validation of the merged product's driver

> Phase 1 deliverable of the dev-sign + parity re-audit session (SA lane).
> Audited repo: **https://github.com/heicfast/core_recovery** @ `dd82db9`
> (workflow content identical to `3bda8e2`). Evidence run:
> [40549660074](https://github.com/heicfast/core_recovery/actions/runs/40549660074)
> (2026-10-27, `workflow_dispatch`, windows-2022 GitHub-hosted, **success**).
> Transcript + assets: `diff-to-fix/evidence/executions/devsign-run-40549660074/`.

## 1. Verdict

**The dev-sign path EXISTS, is DOCUMENTED, and WORKS on a real Windows
runner** — with three caveats that are filed as findings:

1. The **documented** installer (`install.ps1` from the rda-driver release)
   **crashes at its elevation guard**; the install only succeeds through the
   composite action's *undocumented* `sc.exe` fallback (S-036). The fallback
   installs a differently-configured service than the documented path
   (M-060).
2. The mechanism is validated on **exactly one environment class**
   (GitHub windows-2022, no HVCI/SecureBoot enforcement). No Win7/Win10/Win11
   matrix exists; the HVCI constraint is undocumented (M-063).
3. The release's `uninstall.ps1` is **never exercised** by any CI lane and
   has silent-failure semantics (M-061); the sign step uses **no timestamp
   server** (N-084).

Per the session mandate, `tools/dev-sign/` was **NOT** created: the merged
repository already documents a dev-sign path (a script + a workflow —
`.github/actions/install-driver/action.yml` +
`.github/workflows/driver-install.yml`, documented in
`docs/PREBUILT-ARTIFACTS.md` §2 and `docs/DRIVER-RELEASE.md` §7). The mandate
for that case is "verify it works and record how to reproduce it" — this file
is that record. The dev-sign path location, for anyone looking from the repo
root: **`.github/actions/install-driver/action.yml` (the sign+install
recipe)**, **`.github/workflows/driver-install.yml` (the runnable
workflow_dispatch lane)**.

## 2. The dev-sign recipe (as implemented and verified)

```powershell
# 0. driver binary: the UNSIGNED prebuilt rda.sys (rda-driver release vX.Y.Z,
#    SHA256SUMS-verified). The release ships unsigned BY DESIGN
#    (DRIVER-RELEASE.md §7: SignMode=Off; the release asserts no signature).
# 1. job-local code-signing certificate:
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=RRECOV-CI" `
  -KeyExportPolicy Exportable -NotAfter (Get-Date).AddDays(2) `
  -CertStoreLocation Cert:\LocalMachine\My
Export-PfxCertificate -Cert $cert -FilePath rda-ci.pfx -Password <pw>   # job-local
Export-Certificate    -Cert $cert -FilePath rda-ci.cer
Import-Certificate -FilePath rda-ci.cer -CertStoreLocation Cert:\LocalMachine\Root
Import-Certificate -FilePath rda-ci.cer -CertStoreLocation Cert:\LocalMachine\TrustedPublisher
# 2. sign (SHA-256 file digest; NO timestamp server — see N-084):
& $signtool sign /fd SHA256 /f rda-ci.pfx /p <pw> driver\rda.sys
# 3. install (the documented installer — crashes, see S-036) ->
#    the live fallback:
Copy-Item driver\rda.sys "$env:SystemRoot\System32\drivers\rda.sys" -Force
sc.exe create rda type= kernel start= demand binPath= "System32\drivers\rda.sys" `
  group= "FSFilter Boot File System"          # NOTE: diverges from install.ps1 — M-060
sc.exe start rda
# 4. round-trip:
$h = [IO.File]::Open('\\.\rda', 'Open', 'Read', 'ReadWrite'); $h.Close()   # control device
rrecov driver --json     # GET_INFO + GET_VERSION -> state ready, 5 facets green
# 5. uninstall (the workflow's path — NOT the release's uninstall.ps1):
sc.exe stop rda; sc.exe delete rda
Remove-Item "$env:SystemRoot\System32\drivers\rda.sys" -Force
```

**Test-signing boot mode is NOT required on windows-2022 GitHub runners** —
the self-signed cert in `LocalMachine\Root` + `LocalMachine\TrustedPublisher`
suffices there (empirically: run 40549660074 loaded and started the driver;
also the api-benchmark lineage runs 40227375329 / 40315255368). This does NOT
generalize to stock Win11 with HVCI/VBS — see §5/M-063.

## 3. Certificate details (no private key material)

| Field | Value |
|---|---|
| Subject | `CN=RRECOV-CI` |
| Type | `CodeSigningCert` (self-signed) |
| Store | `LocalMachine\My` (exported to Root + TrustedPublisher) |
| Validity | +2 days from creation (job-local, per-run) |
| Key export | Exportable (PFX, job-local password) |
| Signature | `signtool sign /fd SHA256` — SHA-256 file digest, **no /tr, no /t** |
| Timestamp | none — the signature dies with the 2-day cert (harmless for CI load-and-test; N-084 records the missing rationale) |

## 4. Install / load / round-trip / uninstall evidence (run 40549660074)

**Install + load** (job-log `:229-249`):
- `Successfully signed: driver\rda.sys`
- `install.ps1 failed (1)` → `InvokeMethodOnNull` at the elevation guard
  (S-036) → manual fallback: `[SC] CreateService SUCCESS` → `sc query rda`:
  `TYPE: 1 KERNEL_DRIVER`, `STATE: 4 RUNNING`, `PID: 0`
- Control-device round-trip: `\\.\RDA` open + close OK
- **Control-interface round-trip beyond open/close:** `rrecov driver --json`
  exits 0 with `state:"ready"`, all five facets green
  (`present/loaded/started/authorized/healthy: yes`), `major:1 minor:0`
  (GET_VERSION), `flags:2 max_disks:200 ioctl_base:0` (GET_INFO) — the
  documented control interface answers.
- **Grammar-path round-trip:** `rrecov scan --disk 99` proceeds past the
  driver gate; the transport attempts `\\.\RDA\<token>\99\...` and gets the
  driver's open-failure class `rc=50020001` (target error, not a refusal) —
  proving a full user-mode → driver → error-mapping round trip.

**Device-object enumeration:** the run captures no WinObj/device-tree
snapshot. For a legacy (non-PnP) kernel service driver the canonical
enumeration proof IS the named control device opening — `\Device\RDA` exists
because `CreateFile("\\\\.\\RDA")` succeeds (v0.2.1's floor exists precisely
because bare opens used to fail with error 87 — PREBUILT-ARTIFACTS.md "the
bare-open error-87 fix"). The evidence is therefore adequate, though
implicit; a stronger future capture would log
`Get-WinEvent System` driver-load events 219/7045 and the
`\Device\` object directory. Recorded as an observation, not a finding.

**Uninstall** (job-log `:334-337`): raw `sc.exe stop rda` → `sc.exe delete
rda` → `Remove-Item ...\drivers\rda.sys` — exit 0. Phase 3 had already proven
the stopped-service state (`not-started`, `error_code 1342898178` =
0x500B0002). **No post-uninstall `sc query` capture** and **the release's
`uninstall.ps1` is never executed** (M-061).

**Windows test-signing mode:** not enabled, not needed on this runner class
(§2). Enabling it (`bcdedit /set testsigning on`) changes boot configuration
and shows a desktop watermark — only for dedicated development machines.

## 5. Production-shaped assessment (vs the corpus's reference installer)

The corpus's documented reference installer behavior
(`easeus-drw-research/architecture/02-kernel-driver-architecture.md` §1, §5;
`og-codebase/BUILDPE/x64/regdrv.reg`):

| Aspect | Reference (og) | Merged product (as executed) | Verdict |
|---|---|---|---|
| Installer unit | native EXE `setupdrvx64.exe` (PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`), SCM APIs, silent | PowerShell `install.ps1` (documented) / inline `sc.exe` (live) — elevated console required | **gap** — N-085 |
| Copy destination | `%SystemRoot%\System32\Drivers\%s.sys` | `System32\drivers\<svc>.sys` | match |
| Service shape | `Type=1`, `Start=3` (demand), `ErrorControl=1` (regdrv.reg: eprdtdrv/epmdkdrv/ebrntdrv) | `type= kernel`, `start= demand`, `error= normal` | match |
| Service group | `Group="Boot File System"` (ebrntdrv, regdrv.reg) | install.ps1: `"Boot File System"` (match) · **live fallback: `"FSFilter Boot File System"` (no corpus basis)** | **divergence in the live path** — M-060 |
| Registry parameters | none for eprdtdrv (only the EPM WinPE pair carries `IoctlBase=0x10000`) | rda contract: `IoctlBase/MaxDisks/DebugLevel/SessionsEnabled` — set by install.ps1 + INF, **set by NEITHER in the live fallback** (compiled defaults observed: max_disks=200, flags=2) | **divergence** — M-060 (rda's own contract, three sources of truth) |
| Lifecycle | installed at app start (setupdrvx64.exe), stopped + deleted at app exit | installed by CI script; uninstall = raw sc.exe; app-level install/uninstall orchestration absent from the merged repo (the core only REFUSES without the driver — e58bde1) | **gap** — covered by the driver-only-era audit lane (S7) |
| Signing | Authenticode, cross-signed vendor chain ("Chengdu Yiwo Tech Development Co., Ltd") | unsigned-by-design + CI/dev test-sign; production path = OSSign (prepared, DISABLED) | deliberate — I-026 |
| WinPE | drivers pre-registered via `regdrv.reg`, no signature enforcement | INF profile 2 + winpe reg script (rda-driver repo); unvalidated in core CI | out of this lane's evidence; noted |

## 6. Environments validated

| Environment | Result |
|---|---|
| GitHub-hosted `windows-2022` (ImageOS win22), no HVCI, test-signing OFF | **install + load + round-trip + uninstall: PASS** (run 40549660074) |
| Win7 SP1 / Win10 / Win11 retail (HVCI on) / WinPE | **NOT VALIDATED** — no lane exists (M-063) |

## 7. Gaps observed → findings (this session)

| ID | Severity | One-liner |
|---|---|---|
| S-036 | SEVERE | documented installer `install.ps1` crashes at its elevation guard; only the undocumented composite fallback installs |
| M-060 | MAJOR | the live install path (fallback) configures the service differently from the documented path (group name, no Parameters) |
| M-061 | MAJOR | release `uninstall.ps1` never exercised by CI + silent-failure semantics (locked-file remove reports success) |
| M-062 | MAJOR | `install.ps1` start-failure path exits 0 (warning only) + broken string-concat in the warning |
| M-063 | MAJOR | dev-sign validation is a single-environment data point; HVCI/SecureBoot constraint + Win7/10/11 matrix undocumented |
| N-084 | MINOR | sign step uses no timestamp server; rationale undocumented |
| N-085 | MINOR | documented install path requires an elevated interactive PowerShell console (vs the og's silent SCM-API EXE) — installer-posture gap |
| I-026 | INFORMATIONAL | unsigned-by-design driver + CI test-sign posture is deliberate and correctly documented (corpus: vendor Authenticode chain) |

## 8. Reproduction steps (end-to-end)

1. Dispatch: `gh workflow run driver-install.yml --repo heicfast/core_recovery`
   (or the API POST in the evidence README).
2. Read the job log — the phases assert themselves (`PHASE 1/2/3 ... verified`).
3. Download the `driver-install-evidence-<run_id>` artifact (env manifest +
   resolved releases + driver-status) and the job log for the transcript.
4. Local-machine variant (dev box, HVCI off or testsigning on): follow §2's
   recipe verbatim with a locally-built or release-fetched `rda.sys`. On
   stock Win11 expect load failure 0xC0000428 until HVCI is off or
   testsigning is on (M-063).
