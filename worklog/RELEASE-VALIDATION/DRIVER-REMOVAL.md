# DRIVER-REMOVAL — the §9 gate-1 evidence: the kernel driver is gone from the product tree

> Sweep executed 2026-11-05 17:34 UTC at commit a0fe4be0 (the P3 driver-removal sweep: commits 318b9655, b9b5015f, e432a8c9, a0fe4be0).


## What was removed (the tree, not the history)

- `driver/` — all 63 files (rda.sys WDM sources, rda.sln, INF, SCM install/uninstall
  scripts, fuzz harness, signing tooling, docs). **Wiped, not deprecated, not gated.**
- `.github/workflows/driver-ci.yml` — the whole driver lane.
- `build.ps1` — the driver lane, the DRIVER_STAGE define, the Invoke-Driver function.
- `installer/recoverywizard.nsi` — SEC_DRIVER (the service create/start, the
  System32\drivers copy, the certutil Root+TrustedPublisher imports, the Parameters
  block), the driver page, the uninstaller's driver teardown (809 → 531 lines).
- `installer/portable.ps1` — the driver stage + the install-the-driver instructions.
- `installer/devsign.ps1` — install-cert/remove-cert (kernel trust-anchor machinery).
- The release artifact `-testsigning.cer` (release.yml rewritten).
- The UI's DriverStatus surface (contract op, mock, queries, About/Settings rows, i18n).
- contracts/: the system_driver_status command row, the §10 DriverStatus type map,
  the driver service-name constant, the ERR_DRIVER_REQUIRED mapping (repurposed to
  ERR_DEVICE_DISCONNECTED per the v4 header).
- Every product doc (README, ARCHITECTURE, BUILD, RELEASE, TESTING, INSTALLER,
  DEV-SIGNING, PRODUCT-WIRING, footprints, gaps, MSIX, PARITY-DELTAS) — status-
  corrected to the user-mode truth.

The git HISTORY retains the driver lineage (the root history is the rda-driver
lineage — positions 1..1100); the mandate preserves history. The TREE is clean.

## The sweep

Patterns (git grep -I -E, word-bounded where marked), scope: the whole tree,
**minus the audit-trail exemptions** (each classified below).

| Pattern | What it catches | Total hits | Live-tree hits |
|---|---|---|---|
| `\brda\b` | the driver name (lowercase, word-bounded) | 814 | 42 |
| `rda\.sys` | the driver binary | 163 | 22 |
| `rdacli` | the driver CLI | 63 | 6 |
| `\\\\\.\\RDA` | the driver device path | 73 | 11 |
| `\bRDA\b` | the driver name (uppercase, word-bounded) | 430 | 72 |
| `ERR_DRIVER_REQUIRED` | the driver error code | 48 | 11 |
| `testsigning` | the kernel test-signing policy | 61 | 11 |
| `bcdedit` | the boot-config test-signing tool | 31 | 4 |
| `DriverStatus|system_driver_status` | the UI driver surface | 170 | 3 |
| `DRIVER_STAGE` | the driver build stage | 14 | 2 |
| `Win32_System_Services` | the SCM NuGet feature | 3 | 0 |
| | **TOTAL** | **1870** | **184** |

## Why 1,870 → every hit is either exempt-class or classified live

The driver existed for the product's entire prior life: its name appears throughout
the audit history, the reference-product research (the reference product ships its
own kernel drivers — documented as clean-room research facts), and the removal
narratives themselves. A raw grep count is therefore meaningless without
classification. Every one of the 184 live hits falls into exactly one of these
classes:

### A. Audit-trail exemptions (1,686 hits — preserved per the mandate §8, never edited)

| Exempt scope | Classification |
|---|---|
| `docs/INTEGRATION-PLAN.md` | the PRIOR session's plan — supersession banner added, body preserved as audit trail |
| `docs/INTEGRATION-VALIDATION/` | prior-session release evidence (frozen transcripts/JSON — mandated untouched) |
| `docs/REFERENCE-` | clean-room research docs about the REFERENCE product (its drivers are its facts) |
| `engine/AUDIT` | core_recovery's audit documents (same class) |
| `engine/RELEASE-NOTES.md` | core_recovery's release history (same class) |
| `engine/diff-to-fix/` | core_recovery's parity audit trail (preserved verbatim per the mandate) |
| `engine/docs/` | core_recovery's own docs: reference-product parity research (the reference product HAS drivers), dated audit narratives, release notes, baselines — reference facts and core history, not our product's claims |
| `engine/scripts/verify/` | core_recovery's trail-SHA verifier (verifies audit evidence citing the retired upstream repo tags) |
| `engine/worklog.md` | core_recovery's own records (source-of-truth tree) |
| `engine/worklog/` | core_recovery's own records |
| `ui/docs/historical/` | the fresh UI repo's dated historical docs (dated addenda only, its own convention) |
| `ui/worklog.md` | the fresh UI repo's own session records (arrived with the P1 merge) |
| `ui/worklog/` | the fresh UI repo's own archived prompts/records |
| `worklog.md` | this session's chronological record (quotes driver facts by design) |
| `worklog/` | the Phase 0 sheets + INTEGRATION-PLAN + the archived bannered driver-era context sheets + this evidence tree |

### B. Live-tree hits by class (184 — every one enumerated below)

| Class | Meaning |
| `README.md` | REMOVAL NARRATIVE — the merge table records the driver row as REMOVED; the intro states no kernel driver exists |
| `contracts/error-codes.json` | MAPPING HISTORY — the ERR_DEVICE_DISCONNECTED row documents that 0x50020007 was ERR_DRIVER_REQUIRED in v2 (the value was repurposed; the mapping must record it) |
| `docs/ARCHITECTURE.md` | REMOVAL NARRATIVE — "the kernel driver was removed in the consolidation session" |
| `docs/DEV-SIGNING.md` | STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix |
| `docs/FORENSIC-PROBE.md` | STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim |
| `docs/INSTALL-FOOTPRINT-CURRENT.md` | STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free |
| `docs/INSTALL-FOOTPRINT-DELTA.md` | STATUS-CORRECTED — "No analog — driver removed" row |
| `docs/INSTALLER.md` | REMOVAL NARRATIVE — the installer doc states what no longer installs |
| `docs/INTEGRATION-GAPS.md` | STATUS-CORRECTED rows |
| `docs/INTEGRATION-VALIDATION.md` | STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt |
| `docs/MSIX-ASSESSMENT.md` | STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity |
| `docs/PARITY-DELTAS.md` | REVERSAL RECORD — D-FALL documents the driver removal |
| `docs/PRODUCT-WIRING.md` | STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical |
| `docs/RELEASE.md` | REMOVAL NARRATIVE — the release notes template states the driver is gone (mandated) |
| `engine/.github/scripts/` | core's own repo scripts (benchmark pinning against the retired upstream tags) |
| `engine/include/` | core's own historical header comments (same class) |
| `engine/scripts/` | core's own research/audit scripts |
| `engine/src/` | core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') |
| `engine/src/ui/CMakeLists.txt` | core's own historical comment |
| `engine/tests/` | core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) |
| `engine/tools/` | core's own tool comments (doc_truth_gate's own driver-lane citation checks) |
| `tools/validation/installer-matrix.ps1` | ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) |

### C. The complete live-hit enumeration


#### `\brda\b` — 42 live hit(s)
- `README.md:37` — REMOVAL NARRATIVE — the merge table records the driver row as REMOVED; the intro states no kernel driver exists — `| ~~driver/~~ | ~~heicfast/rda-driver (main)~~ | da7efb9 — **REMOVED** (tree-only; the git lineage is preserved in history) |`
- `docs/DEV-SIGNING.md:6` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `> kernel-signing era this document originally described: no `rda.sys``
- `docs/DEV-SIGNING.md:102` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — ``driver\bin\x64\Release\rda.sys` was embed-signed after the WDK build`
- `docs/FORENSIC-PROBE.md:64` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `- `certutil -addstore -f Root ...rda-testcert.cer` and `certutil -addstore`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:62` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 9 | `driver\rda.sys` | `driver\bin\x64\Release\rda.sys`, embed-signed (`build.ps1:161-165`) | 50,640 | the RDA raw-disk kernel d`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:64` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 11 | `driver\rda.inf` | `driver\src\driver\rda.inf` verbatim (`build.ps1:170`) | 2,038 | INF profile of the driver — **staged bu`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:67` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 14 | `driver\rda-testcert.cer` | stage cert (`build.ps1:175`) | 834 | PUBLIC half of the ephemeral dev signing cert (also publis`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:68` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 15 | `driver\rda-testcert-thumbprint.txt` | stage (`build.ps1:175`) | 40 | SHA-1 thumbprint; read at install into `DriverCertThu`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:70` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `The driver staging dir is the "signed-drop-in surface": replace `driver\rda.sys` + reinstall`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:78` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Driver file | `C:\Windows\System32\drivers\RDA.sys` (renamed from `rda.sys` to the service name) | `driver\install.ps1:58-59, 83`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:82` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Certificate stores | `LocalMachine\Root` + `LocalMachine\TrustedPublisher` ← `rda-testcert.cer` | `certutil -addstore -f Root` (`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:119` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 8–14 | `driver\{rda.sys, rdacli.exe, rda.inf, install.ps1, uninstall.ps1, rda-testcert.cer, rda-testcert-thumbprint.txt}` | driv`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:140` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     nuget restore (WDK 10.0.26100.6584) → msbuild rda.sln x64      (:137-140)`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:142` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     devsign new-cert (ephemeral, idempotent) + sign rda.sys        (:161-165; installer/devsign.ps1:118-176)`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:143` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     stage → stage-<v>\driver\{rda.sys,rdacli.exe,rda.inf,install.ps1,uninstall.ps1,rda-testcert.cer,rda-testcert-thumbprint.txt}`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:223` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `"$INSTDIR\driver\install.ps1" -ServiceName RDA -Source "$INSTDIR\driver\rda.sys"` via`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:333` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 2 | `:301` | `certutil -addstore -f Root "<…>\rda-testcert.cer"` (ExecWait) | **VISIBLE console window** (flash) |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:334` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 3 | `:307` | `certutil -addstore -f TrustedPublisher "<…>\rda-testcert.cer"` (ExecWait) | **VISIBLE console window** (flash) |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:337` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 6 | `:349` | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "<…>\driver\install.ps1" -ServiceName RDA -Source "<…>\dri`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:388` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `plain `Copy-Item` to System32 (§5.1 step 3, sheet line ~253) and §7 states "rda.sys`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:391` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `and verified (`driver/scripts/install.ps1:61-100`), and `rda.sys` is embed-signed at`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:394` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `9. **`DRIVER-DEPLOYMENT.md` §3 / rda.inf** — `rda.cat` declared, never built`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:395` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `(`driver/src/driver/rda.inf:21`); still true; the INF remains staged-but-unused by the`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:405` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `in both stores, dev-signed rda.sys), reinstall-over (marker survived), repair`
- `docs/INSTALL-FOOTPRINT-DELTA.md:34` — STATUS-CORRECTED — "No analog — driver removed" row — `| `eprdtdrv.sys` raw-disk driver, SCM service (CreateService, group "Boot File System", demand) | **No analog — driver removed** (`
- `docs/INTEGRATION-VALIDATION.md:29` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row`
- `docs/INTEGRATION-VALIDATION.md:40` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| driver-ci | green at `00f5cae5` (the latest commit touching its domain) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matri`
- `docs/INTEGRATION-VALIDATION.md:44` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| release | **`v0.5.0` run 4 GREEN** (unsigned era, history); **`v0.6.0-rc1` — the first DEVELOPMENT TEST-SIGNED release, cut on t`
- `docs/INTEGRATION-VALIDATION.md:79` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `staged `rda.sys` embed the exact DER of the published `.cer` — the`
- `docs/MSIX-ASSESSMENT.md:21` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `component (`rda.sys`, service `RDA`, `HKLM\SYSTEM\CurrentControlSet\`
- `docs/PARITY-DELTAS.md:40` — REVERSAL RECORD — D-FALL documents the driver removal — `| **D-FALL** | **The driver is MANDATORY — every driver-absent fallback removed** (user directive, 2026-10-27). The reference's De`
- `docs/PRODUCT-WIRING.md:31` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `(`engine/src/core/**`) → Windows platform layer (`engine/src/win/*`) → **`rda.sys` kernel`
- `docs/PRODUCT-WIRING.md:511` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `- **OQ-1**: Does the shipped `rda.sys` actually complete the grammar open for disks ≥ 1 and`
- `engine/.github/scripts/Run-Benchmark.ps1:735` — core's own repo scripts (benchmark pinning against the retired upstream tags) — `# 'rda' expectation here false-failed every API lane in run`
- `engine/.github/scripts/fetch-release.ps1:6` — core's own repo scripts (benchmark pinning against the retired upstream tags) — `#   ./fetch-release.ps1 -Repo heicfast/rda-driver -OutDir driver ``
- `engine/.github/scripts/fetch-release.ps1:7` — core's own repo scripts (benchmark pinning against the retired upstream tags) — `#       [-Tag v0.2.2] [-Required rda.sys,rda.inf,install.ps1,uninstall.ps1,SHA256SUMS] ``
- `engine/include/cr/cr_types.h:7` — core's own historical header comments (same class) — `* same methodology as rda-driver's shared headers).`
- `engine/tests/unit/test_cr_ui.cpp:4029` — core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) — `* removed "rda" value. */`
- `engine/tests/unit/test_cr_ui.cpp:4038` — core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) — `EXPECT_EQ(around.find("\"rda\""), std::string::npos) << "the removed rda value must not return";`
- `engine/tools/doc_truth_gate.py:85` — core's own tool comments (doc_truth_gate's own driver-lane citation checks) — `r"driver repo|rda-driver|driver-repo|out-of-tree|out of this repo",`
- `engine/tools/stamp_osversion_um.py:6` — core's own tool comments (doc_truth_gate's own driver-lane citation checks) — `User-mode port of the rda-driver tool (tools/stamp_osversion.py). The`
- `engine/tools/verify_pe_um.py:6` — core's own tool comments (doc_truth_gate's own driver-lane citation checks) — `User-mode port of the rda-driver Win7 gate (tools/verify_pe.py), adapted:`

#### `rda\.sys` — 22 live hit(s)
- `docs/DEV-SIGNING.md:6` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `> kernel-signing era this document originally described: no `rda.sys``
- `docs/DEV-SIGNING.md:102` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — ``driver\bin\x64\Release\rda.sys` was embed-signed after the WDK build`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:62` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 9 | `driver\rda.sys` | `driver\bin\x64\Release\rda.sys`, embed-signed (`build.ps1:161-165`) | 50,640 | the RDA raw-disk kernel d`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:70` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `The driver staging dir is the "signed-drop-in surface": replace `driver\rda.sys` + reinstall`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:78` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Driver file | `C:\Windows\System32\drivers\RDA.sys` (renamed from `rda.sys` to the service name) | `driver\install.ps1:58-59, 83`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:119` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 8–14 | `driver\{rda.sys, rdacli.exe, rda.inf, install.ps1, uninstall.ps1, rda-testcert.cer, rda-testcert-thumbprint.txt}` | driv`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:142` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     devsign new-cert (ephemeral, idempotent) + sign rda.sys        (:161-165; installer/devsign.ps1:118-176)`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:143` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     stage → stage-<v>\driver\{rda.sys,rdacli.exe,rda.inf,install.ps1,uninstall.ps1,rda-testcert.cer,rda-testcert-thumbprint.txt}`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:223` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `"$INSTDIR\driver\install.ps1" -ServiceName RDA -Source "$INSTDIR\driver\rda.sys"` via`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:337` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 6 | `:349` | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "<…>\driver\install.ps1" -ServiceName RDA -Source "<…>\dri`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:388` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `plain `Copy-Item` to System32 (§5.1 step 3, sheet line ~253) and §7 states "rda.sys`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:391` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `and verified (`driver/scripts/install.ps1:61-100`), and `rda.sys` is embed-signed at`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:405` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `in both stores, dev-signed rda.sys), reinstall-over (marker survived), repair`
- `docs/INSTALL-FOOTPRINT-DELTA.md:34` — STATUS-CORRECTED — "No analog — driver removed" row — `| `eprdtdrv.sys` raw-disk driver, SCM service (CreateService, group "Boot File System", demand) | **No analog — driver removed** (`
- `docs/INTEGRATION-VALIDATION.md:29` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row`
- `docs/INTEGRATION-VALIDATION.md:40` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| driver-ci | green at `00f5cae5` (the latest commit touching its domain) | lint, fuzz, unit MSVC /W4 /WX, WDK Debug+Release matri`
- `docs/INTEGRATION-VALIDATION.md:44` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| release | **`v0.5.0` run 4 GREEN** (unsigned era, history); **`v0.6.0-rc1` — the first DEVELOPMENT TEST-SIGNED release, cut on t`
- `docs/INTEGRATION-VALIDATION.md:79` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `staged `rda.sys` embed the exact DER of the published `.cer` — the`
- `docs/MSIX-ASSESSMENT.md:21` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `component (`rda.sys`, service `RDA`, `HKLM\SYSTEM\CurrentControlSet\`
- `docs/PRODUCT-WIRING.md:31` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `(`engine/src/core/**`) → Windows platform layer (`engine/src/win/*`) → **`rda.sys` kernel`
- `docs/PRODUCT-WIRING.md:511` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `- **OQ-1**: Does the shipped `rda.sys` actually complete the grammar open for disks ≥ 1 and`
- `engine/.github/scripts/fetch-release.ps1:7` — core's own repo scripts (benchmark pinning against the retired upstream tags) — `#       [-Tag v0.2.2] [-Required rda.sys,rda.inf,install.ps1,uninstall.ps1,SHA256SUMS] ``

#### `rdacli` — 6 live hit(s)
- `docs/FORENSIC-PROBE.md:220` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `- **`rdacli.exe` (shipped in the install) cannot talk to the shipped`
- `docs/FORENSIC-PROBE.md:223` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — ``driver/src/client/rdacli.c:32-36`). facts-postinstall.json `rdacli``
- `docs/INSTALL-FOOTPRINT-CURRENT.md:63` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 10 | `driver\rdacli.exe` | same driver lane (`build.ps1:169`) | 48,640 | driver client/contract harness CLI |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:119` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 8–14 | `driver\{rda.sys, rdacli.exe, rda.inf, install.ps1, uninstall.ps1, rda-testcert.cer, rda-testcert-thumbprint.txt}` | driv`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:143` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     stage → stage-<v>\driver\{rda.sys,rdacli.exe,rda.inf,install.ps1,uninstall.ps1,rda-testcert.cer,rda-testcert-thumbprint.txt}`
- `docs/INTEGRATION-VALIDATION.md:57` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **rdacli ↔ driver** | Forensic facts (run 40465852724: `rdacli info` gle=2 on `\\.\EPRDTDRV`) | BEFORE: the shipped CLI tool cou`

#### `\\\\\.\\RDA` — 11 live hit(s)
- `docs/FORENSIC-PROBE.md:222` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `addresses `\\.\RDA` (`engine/src/win/cr_transport.cpp:306-323`,`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:81` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Kernel device | `\Device\RDA` + `\DosDevices\RDA` → user path `\\.\RDA` | derived from service name at DriverEntry (DRIVER-DEPLO`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:202` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — ``\\.\RDA`, SYSTEM privileges + admin-only DACL, no-fallback truth, development`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:307` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| SCM service `RDA` existing + running; `\\.\RDA` opens; `IOCTL_RDA_GET_VERSION` 0x60002448 | `ui/src-tauri/src/commands/system.rs`
- `docs/INTEGRATION-GAPS.md:247` — STATUS-CORRECTED rows — `service RUNNING, `\\.\RDA` opens) — **historical record of the 0.6.x`
- `docs/INTEGRATION-VALIDATION.md:30` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **T14 — measured latency/throughput vs the retired simulator's budgets, through the mandatory RDA driver**: run **40442056802**`
- `docs/INTEGRATION-VALIDATION.md:57` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **rdacli ↔ driver** | Forensic facts (run 40465852724: `rdacli info` gle=2 on `\\.\EPRDTDRV`) | BEFORE: the shipped CLI tool cou`
- `docs/MSIX-ASSESSMENT.md:34` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `elevated (the `\\.\RDA` SDDL half of the original argument is gone`
- `docs/PRODUCT-WIRING.md:250` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``\\.\RDA` open + `IOCTL_RDA_GET_VERSION (0x60002448)` (`system.rs:15-17`). The engine↔driver`
- `docs/PRODUCT-WIRING.md:251` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `contract: PID-token grammar `\\.\RDA\<token>\<disk>\<part>` (`cr_transport.cpp:318-343`,`
- `docs/PRODUCT-WIRING.md:513` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``cr_ui_last_transport_error()` trace or Process Monitor log of `\\.\RDA\<token>\…` creates`

#### `\bRDA\b` — 72 live hit(s)
- `docs/ARCHITECTURE.md:107` — REMOVAL NARRATIVE — "the kernel driver was removed in the consolidation session" — ``System32\drivers` row, the `RDA` service, and the certificate-store`
- `docs/DEV-SIGNING.md:107` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — ``LocalMachine\Root` + `TrustedPublisher` before creating the `RDA``
- `docs/FORENSIC-PROBE.md:9` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `> probe facts that used to assert "RDA service Running / dev cert in`
- `docs/FORENSIC-PROBE.md:13` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `> they land, read every driver/RDA/certificate observation below as`
- `docs/FORENSIC-PROBE.md:49` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `- Install: exit 0 in 15.7 s; 15 files placed; RDA service **Running**;`
- `docs/FORENSIC-PROBE.md:222` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `addresses `\\.\RDA` (`engine/src/win/cr_transport.cpp:306-323`,`
- `docs/FORENSIC-PROBE.md:230` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `- The RDA service is **Running** after install (facts-postinstall.json) and`
- `docs/FORENSIC-PROBE.md:231` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `the engine is RDA-only (`cr_transport.cpp:5-8`, no fallback) — every raw`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:62` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 9 | `driver\rda.sys` | `driver\bin\x64\Release\rda.sys`, embed-signed (`build.ps1:161-165`) | 50,640 | the RDA raw-disk kernel d`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:78` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Driver file | `C:\Windows\System32\drivers\RDA.sys` (renamed from `rda.sys` to the service name) | `driver\install.ps1:58-59, 83`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:79` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| SCM service `RDA` | `HKLM\SYSTEM\CurrentControlSet\Services\RDA`: `type= kernel start= demand error= normal group= "Boot File Sy`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:80` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Service Parameters | `...\Services\RDA\Parameters`: `IoctlBase=0`, `MaxDisks=200`, `DebugLevel=1`, `SessionsEnabled=1` (DWORDs)`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:81` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| Kernel device | `\Device\RDA` + `\DosDevices\RDA` → user path `\\.\RDA` | derived from service name at DriverEntry (DRIVER-DEPLO`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:198` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `3. Components: "HeicFast Recovery Wizard (required)" + "RDA raw-disk kernel driver`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:202` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — ``\\.\RDA`, SYSTEM privileges + admin-only DACL, no-fallback truth, development`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:220` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `5. **Update pre-step** (if `HKLM\…\Services\RDA\ImagePath` exists): `sc.exe stop` + sleep`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:223` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `"$INSTDIR\driver\install.ps1" -ServiceName RDA -Source "$INSTDIR\driver\rda.sys"` via`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:226` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `existence preflight (`:52-55`), WOW64-safe copy to `System32\drivers\RDA.sys``
- `docs/INSTALL-FOOTPRINT-CURRENT.md:245` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `RDA`, `:608`; fallback `sc stop/delete` `:614-621`; belt-and-braces `Sysnative` +`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:246` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `literal `System32\drivers\RDA.sys` deletes `/REBOOTOK`, `:631-636`) → **cert removal by`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:256` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — ``leftovers: 0`, `installDirExists: false`, service gone, `System32\drivers\RDA.sys` gone,`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:307` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| SCM service `RDA` existing + running; `\\.\RDA` opens; `IOCTL_RDA_GET_VERSION` 0x60002448 | `ui/src-tauri/src/commands/system.rs`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:308` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| `System32\drivers\RDA.sys` at the ImagePath | `install.ps1:58-59, 83` | copied + renamed, WOW64-safe | ✅ (T15 `sysDriverFile: fa`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:335` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 4 | `:330` | `sc.exe stop RDA` (nsExec::ExecToLog; only when an existing service was found) | Hidden |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:336` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 5 | `:333` | `sc.exe delete RDA` (nsExec) | Hidden |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:337` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 6 | `:349` | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "<…>\driver\install.ps1" -ServiceName RDA -Source "<…>\dri`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:338` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 7 | `:366` | `sc.exe start RDA` (nsExec) | Hidden |`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:344` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `| 8 | `:608` | `powershell.exe … -File "<…>\driver\uninstall.ps1" -ServiceName RDA` (ExecWait) | **VISIBLE PowerShell console wind`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:433` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `- **Installed product (v0.6.0-rc1, historical):** 15 files in `$INSTDIR` + `System32\drivers\RDA.sys` + 2 ProgramData`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:434` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `dirs + 4 registry keys + service `RDA` (demand-start) + cert in 2 stores + 2 `%TEMP%``
- `docs/INSTALL-FOOTPRINT-DELTA.md:34` — STATUS-CORRECTED — "No analog — driver removed" row — `| `eprdtdrv.sys` raw-disk driver, SCM service (CreateService, group "Boot File System", demand) | **No analog — driver removed** (`
- `docs/INSTALL-FOOTPRINT-DELTA.md:75` — STATUS-CORRECTED — "No analog — driver removed" row — `- **The 6 non-RDA kernel drivers** (WinPE hooks, the backup filter, the`
- `docs/INSTALLER.md:11` — REMOVAL NARRATIVE — the installer doc states what no longer installs — `flows this document used to describe — `SEC_DRIVER`, the `RDA` service,`
- `docs/INTEGRATION-GAPS.md:242` — STATUS-CORRECTED rows — `since-removed RDA driver — historical evidence, superseded by the`
- `docs/INTEGRATION-GAPS.md:247` — STATUS-CORRECTED rows — `service RUNNING, `\\.\RDA` opens) — **historical record of the 0.6.x`
- `docs/INTEGRATION-GAPS.md:251` — STATUS-CORRECTED rows — `- The no-silent-fallback property the RDA row recorded carries over:`
- `docs/INTEGRATION-VALIDATION.md:15` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `> `-testsigning.cer` asset. Read every RDA/driver/certificate claim`
- `docs/INTEGRATION-VALIDATION.md:29` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **T15 — the installer's runtime lifecycle on a clean Windows runner** (install / update / repair / uninstall, silent): every row`
- `docs/INTEGRATION-VALIDATION.md:30` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **T14 — measured latency/throughput vs the retired simulator's budgets, through the mandatory RDA driver**: run **40442056802**`
- `docs/INTEGRATION-VALIDATION.md:41` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| engine-ci | green at `1a7d05d5` (the latest commit touching engine/ — the mandatory-driver rewrite tree) | lint, Linux ASan full`
- `docs/INTEGRATION-VALIDATION.md:54` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **Rebuilt installer runtime matrix** | validation-evidence run **40474514493** at `b93e6be8` (the rebuilt NSIS): install / updat`
- `docs/INTEGRATION-VALIDATION.md:57` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| **rdacli ↔ driver** | Forensic facts (run 40465852724: `rdacli info` gle=2 on `\\.\EPRDTDRV`) | BEFORE: the shipped CLI tool cou`
- `docs/MSIX-ASSESSMENT.md:20` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `Hardware Dev Center attestation signing. Our product's RDA driver`
- `docs/MSIX-ASSESSMENT.md:21` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `component (`rda.sys`, service `RDA`, `HKLM\SYSTEM\CurrentControlSet\`
- `docs/MSIX-ASSESSMENT.md:22` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `Services\RDA` + Parameters — `contracts/README.md`, `driver/scripts/`
- `docs/MSIX-ASSESSMENT.md:34` — STATUS-CORRECTED — the reversal record (kernel-driver blocker GONE) mentions the driver by necessity — `elevated (the `\\.\RDA` SDDL half of the original argument is gone`
- `docs/PARITY-DELTAS.md:40` — REVERSAL RECORD — D-FALL documents the driver removal — `| **D-FALL** | **The driver is MANDATORY — every driver-absent fallback removed** (user directive, 2026-10-27). The reference's De`
- `docs/PRODUCT-WIRING.md:72` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `**through the RDA driver** (`cr_enumerate.cpp:165-170` `open_rda_partition(L"RDA", n, 0)`)`
- `docs/PRODUCT-WIRING.md:147` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `- open: lettered volume → `cr_ui_open_volume` (bitlocker gate → whole-disk RDA open +`
- `docs/PRODUCT-WIRING.md:149` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``cr_ui_open_disk` (RDA grammar only; driver missing ⇒ `ERR_DRIVER_REQUIRED`,`
- `docs/PRODUCT-WIRING.md:244` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `**[HISTORICAL — the driver, the `RDA` service, and this command's driver`
- `docs/PRODUCT-WIRING.md:249` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``system_driver_status` (`ui/src-tauri/src/commands/system.rs:39-134`): SCM service `RDA` +`
- `docs/PRODUCT-WIRING.md:250` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``\\.\RDA` open + `IOCTL_RDA_GET_VERSION (0x60002448)` (`system.rs:15-17`). The engine↔driver`
- `docs/PRODUCT-WIRING.md:251` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `contract: PID-token grammar `\\.\RDA\<token>\<disk>\<part>` (`cr_transport.cpp:318-343`,`
- `docs/PRODUCT-WIRING.md:513` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``cr_ui_last_transport_error()` trace or Process Monitor log of `\\.\RDA\<token>\…` creates`
- `engine/include/cr/win/cr_transport.h:5` — core's own historical header comments (same class) — `* The RDA driver era is GONE - removed, not gated, not deprecated, not left`
- `engine/scripts/dfx7_info_resolutions.py:135` — core's own research/audit scripts — `"GetVersion" reference in the tree is `kIoctlGetVersion` — an RDA-driver`
- `engine/src/ui/CMakeLists.txt:4` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `# platform layer with the RDA transport, enumeration, health and WinDestIo),`
- `engine/src/ui/cr_ui.cpp:1413` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `* X" - the silent RDA->direct fallback overwrote the RDA leg's error`
- `engine/src/ui/cr_ui.cpp:2087` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `* member that will not open through RDA simply does not resolve */`
- `engine/src/ui/cr_ui.cpp:2661` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `* gone; the S-029 device-control chain issues through the RDA bound`
- `engine/src/win/cr_transport.cpp:177` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `* The pre-unification RDA path used a sector-only envelope (the 'kernel`
- `engine/src/win/cr_transport.cpp:253` — core's own historical code comments (the source-of-truth tree — core documents its eras in prose: 'the RDA driver era is GONE - removed, not gated') — `* device of the direct era (the RDA grammar-path twin was removed with`
- `engine/tests/unit/test_image_create_win.cpp:10` — core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) — `* The class-(a) driver path (open_disk + the RDA transport) is exercised`
- `engine/tests/unit/test_transport_logic.cpp:9` — core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) — `* silent RDA fallback). This file's pins fail at COMPILE time pre-fix`
- `tools/validation/installer-matrix.ps1:198` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `rdaServiceExists = [bool](Get-Service -Name 'RDA' -ErrorAction SilentlyContinue)`
- `tools/validation/installer-matrix.ps1:199` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `sysDriverFile    = Test-Path "$env:SystemRoot\System32\drivers\RDA.sys"`
- `tools/validation/installer-matrix.ps1:225` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `# only. A driver directory, an RDA service, a System32 driver file, or a`
- `tools/validation/installer-matrix.ps1:232` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `throw 'install verification failed: an RDA service exists - the product installs no service'`
- `tools/validation/installer-matrix.ps1:235` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `throw 'install verification failed: System32\drivers\RDA.sys exists - the product places no file outside its install dir + Pro`
- `tools/validation/installer-matrix.ps1:345` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `rdaServiceExists = [bool](Get-Service -Name 'RDA' -ErrorAction SilentlyContinue)`
- `tools/validation/installer-matrix.ps1:346` — ABSENCE GUARD — the T15 matrix probes for the RDA service/driver file/HeicFast certs and FAILS if any exists (install AND uninstall sides) — `sysDriverFile    = Test-Path "$env:SystemRoot\System32\drivers\RDA.sys"`

#### `ERR_DRIVER_REQUIRED` — 11 live hit(s)
- `contracts/error-codes.json:69` — MAPPING HISTORY — the ERR_DEVICE_DISCONNECTED row documents that 0x50020007 was ERR_DRIVER_REQUIRED in v2 (the value was repurposed; the mapping must record it) — `"rule": "P2 core advance: the v2 engine's ERR_DRIVER_REQUIRED at this seat is retired with the driver — v4 repurposes the`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:236` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `(`ERR_DRIVER_REQUIRED`).`
- `docs/INTEGRATION-VALIDATION.md:38` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| contracts-ci | green at `b9d1a366` (the `ERR_DRIVER_REQUIRED` vocabulary commit) | validate.mjs phase 1+2 + filefilter freshness`
- `docs/INTEGRATION-VALIDATION.md:41` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| engine-ci | green at `1a7d05d5` (the latest commit touching engine/ — the mandatory-driver rewrite tree) | lint, Linux ASan full`
- `docs/PARITY-DELTAS.md:40` — REVERSAL RECORD — D-FALL documents the driver removal — `| **D-FALL** | **The driver is MANDATORY — every driver-absent fallback removed** (user directive, 2026-10-27). The reference's De`
- `docs/PRODUCT-WIRING.md:9` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `> administrator token; the `ERR_DRIVER_REQUIRED` wiring documented below`
- `docs/PRODUCT-WIRING.md:84` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``devices.rs:77-83`, including `ERR_DRIVER_REQUIRED` when the driver is missing).`
- `docs/PRODUCT-WIRING.md:149` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``cr_ui_open_disk` (RDA grammar only; driver missing ⇒ `ERR_DRIVER_REQUIRED`,`
- `docs/PRODUCT-WIRING.md:255` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``ERROR_INVALID_HANDLE` (`cr_transport.cpp:217-268`). Missing driver ⇒ `ERR_DRIVER_REQUIRED``
- `docs/PRODUCT-WIRING.md:314` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``cr_ui_open_disk` probe fails (driver missing ⇒ `ERR_DRIVER_REQUIRED`, elevation, busy) ⇒`
- `docs/PRODUCT-WIRING.md:457` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``ERR_DRIVER_REQUIRED 0x50020007 → driver-required` (`tauri-connector.ts:93`).`

#### `testsigning` — 11 live hit(s)
- `docs/DEV-SIGNING.md:105` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `public half was published as the `RecoveryWizard-<version>-testsigning.cer``
- `docs/DEV-SIGNING.md:119` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `| Test signing ON (`bcdedit /set testsigning on` + reboot) | **YES** | test-signing mode accepts any embed signature — no root tru`
- `docs/DEV-SIGNING.md:131` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — ``testsigning Yes` as an image validation gate (since PR #4098,`
- `docs/FORENSIC-PROBE.md:42` — STATUS-CORRECTED — dated banner; the v0.6.0-rc1 evidence body kept verbatim — `- windows-2022 runner, elevated; Secure Boot `false`; `testsigning Yes``
- `docs/INSTALL-FOOTPRINT-CURRENT.md:129` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `**[HISTORICAL driver-era chain — post-consolidation (commit 318b9655) the `Invoke-Driver` lane, the WDK restore, the `testsigning.`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:144` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     publish cert copy → build\out\RecoveryWizard-<v>-testsigning.cer (:179)`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:166` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — ``RecoveryWizard-0.6.0-rc1-testsigning.cer`, `SHA256SUMS`, plus the stage dir`
- `docs/INTEGRATION-VALIDATION.md:15` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `> `-testsigning.cer` asset. Read every RDA/driver/certificate claim`
- `docs/INTEGRATION-VALIDATION.md:44` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `| release | **`v0.5.0` run 4 GREEN** (unsigned era, history); **`v0.6.0-rc1` — the first DEVELOPMENT TEST-SIGNED release, cut on t`
- `docs/INTEGRATION-VALIDATION.md:74` — STATUS-CORRECTED INDEX — dated banner + the [Status 2026-11-05] T14 note; the evidence subtree is exempt — `as a release asset; the notes state the Secure-Boot/testsigning`
- `docs/RELEASE.md:24` — REMOVAL NARRATIVE — the release notes template states the driver is gone (mandated) — `into the OS (the driver-era `-testsigning.cer` asset is gone with it).`

#### `bcdedit` — 4 live hit(s)
- `docs/DEV-SIGNING.md:74` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `- Not a test-signing requirement on ANY machine: no `bcdedit`, no`
- `docs/DEV-SIGNING.md:119` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `| Test signing ON (`bcdedit /set testsigning on` + reboot) | **YES** | test-signing mode accepts any embed signature — no root tru`
- `docs/DEV-SIGNING.md:128` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — ``bcdedit.exe /set TESTSIGNING ON` at image build time`
- `docs/DEV-SIGNING.md:141` — STATUS-CORRECTED — user-mode rescope up top; the kernel-era policy matrix preserved as the dated §6 appendix — `That entire matrix — and the Secure-Boot/`bcdedit` guidance the 0.6.x`

#### `DriverStatus|system_driver_status` — 3 live hit(s)
- `docs/PRODUCT-WIRING.md:14` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — `> `system_driver_status` driver surface is deleted from the UI (the`
- `docs/PRODUCT-WIRING.md:249` — STATUS-CORRECTED HISTORY — bannered; §A.8 driver-status marked historical — ``system_driver_status` (`ui/src-tauri/src/commands/system.rs:39-134`): SCM service `RDA` +`
- `engine/tests/unit/test_privilege.cpp:63` — core's own absence tests + historical test comments (e.g. test_cr_ui.cpp pins the removed rda value must not return) — `EXPECT_EQ(hdr.find("DriverStatus"), std::string::npos);`

#### `DRIVER_STAGE` — 2 live hit(s)
- `docs/INSTALL-FOOTPRINT-CURRENT.md:129` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `**[HISTORICAL driver-era chain — post-consolidation (commit 318b9655) the `Invoke-Driver` lane, the WDK restore, the `testsigning.`
- `docs/INSTALL-FOOTPRINT-CURRENT.md:156` — STATUS-CORRECTED HISTORY — the [HISTORICAL driver-era chain] sections carry the post-consolidation banner; the current-footprint sections are driver-free — `│     makensis /DPRODUCT_VERSION /DAPP_BUNDLE /DENGINE_KIT /DDRIVER_STAGE /DOUT_DIR /DPATHSEP=\  (:248-251)`


## The enforcement (this stays GREEN going forward)

1. **engine-ci.yml's session-96 lint** fails the build if any driver-era SYMBOL
   (`open_rda_partition`, `query_driver_status`, `RdaTargetDevice`,
   `cr_ui_driver_status`, `ERR_DRIVER_`, ...) reappears in `engine/src|include`.
   The lint patterns mention the driver names BY DESIGN — they are the tripwire
   that proves the removal holds.
2. **engine/tests/unit/test_cr_ui.cpp** pins the removed `rda` value must not
   return in any ABI output; **test_privilege.cpp** pins the privilege header
   carries no DriverStatus.
3. **tools/validation/installer-matrix.ps1** (the T15 lane) FAILS if an RDA
   service, a System32 driver file, a driver directory, or a HeicFast machine-store
   certificate exists after install — and after uninstall.
4. **contracts/validate.mjs** phase 1 fails if the error map or event map drifts
   from the engine headers (which no longer define any driver-era code).

## Verdict

**No driver code, build target, workflow stage, installer component, release
asset, capability claim, or current-truth documentation reference exists in the
product tree.** The live hits are: core's own historical prose and absence-tests
(source-of-truth tree), bannered/status-corrected audit history, the removal
narratives the mandate itself requires ("what has changed since the prior
release"), one error-mapping history row (the repurposed 0x50020007), and the
absence guards that enforce this state. Zero unclassified hits.
