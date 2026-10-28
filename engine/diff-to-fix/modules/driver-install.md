# modules/driver-install.md — the packaging/installer surface (created by the dev-sign session, 2026-10-27/28)

The corpus's installer ground truth (architecture/02 §5, the Installer/
artifact, the per-binary installer identity cards, deepening-batch1 §3, the
BUILDPE tree, the licensing retention surface) had no module row until this
session. PARAMETERS Group K (K71-K78) is the scoring axis.

## Phase 1 — the dev-sign validation + the driver-install slice (audited `dd82db9`, run 40549660074)

| Parameter | Verdict | Finding |
|---|---|---|
| K71 installer executability & error semantics | DIVERGENCE-SEVERE | **S-036** — the documented installer crashes at its elevation guard; the composite fallback is the live path |
| K71 start-failure semantics | DIVERGENCE-MAJOR | **M-062** — warning + exit 0 |
| K72 service configuration | DIVERGENCE-MAJOR | **M-060** — invented group, no Parameters, relative binPath (the fallback) |
| K72 copy destination / Type/Start/ErrorControl | MATCH | both paths comply (corpus §5.1 + regdrv.reg) |
| K73 uninstall lifecycle | DIVERGENCE-MAJOR | **M-061** — unexercised script + silent-failure semantics |
| K75 signing posture & environments | DIVERGENCE-MAJOR + INFO | **M-063** (single-environment matrix, HVCI constraint undocumented) + **I-026** (unsigned-by-design, deliberate, execution-proven) |
| K75 timestamp policy | DIVERGENCE-MINOR | **N-084** |
| K74 installer interaction model (driver slice) | DIVERGENCE-MINOR | **N-085** — elevated console vs the og's silent SCM EXE |
| dev-sign load/round-trip behavior | MATCH | run 40549660074: sign → install → RUNNING → control round-trip → ready facets → grammar-path refusal class |

## Waves 3 — the app-installer + deployer-contract slice (audited `dd82db9`)

| Parameter | Verdict | Finding |
|---|---|---|
| K74 silent/unattended surface (app level) | DIVERGENCE-SEVERE | **S-038** — the entire app-installer contract absent (Inno /SILENT//VERYSILENT, ARP QuietUninstallString, unins%03d, admin-required uninstall) |
| K77 install-time registry + telemetry | DIVERGENCE-SEVERE | **S-038** — SetupUE OS gate, DRW\Version key, /SendInfo //UninstallStart//UninstallEnd verbs: all absent |
| K71/K72/K76 deployer versioned/verified contract | DIVERGENCE-MAJOR | **M-064** — no -Uninstall verb, no %s%d coexistence, no write+verify, no per-call SCM errors |
| K78 boot-media deployment | DISPOSITIONED | the PE capability boundary = the roadmap builder's design input (roadmap.md:71; S-038's scope); no defect in shipped code |

Evidence: `evidence/executions/devsign-run-40549660074/` (the run transcript
+ the verbatim v0.2.2 release scripts) + the verified string tables cited in
S-038/M-064.

## Fix-session delta (the v0.2.3 batch close-out — 2026-10-28)

- **S-036 → FIXED** (upstream v0.2.3 `6b50bb8`): the elevation guard's
  InvokeMethodOnNull chain fixed in both scripts; the composite now fails
  LOUD on installer failure (fallback opt-in via allow_fallback) and pins
  FloorVersion v0.2.3.
- **M-060 → FIXED**: the opt-in fallback configures the service exactly
  like install.ps1 (Boot File System group, absolute binPath, the four
  Parameters values) and BOTH paths end with sc qc + Parameters assertions.
- **M-061 → FIXED** (upstream + CI): uninstall.ps1 is now the lane's
  teardown (poll STOP_PENDING, per-call SCM errors, registration-gone
  verify, loud locked-image failure); its exit code is the job's verdict +
  post-state evidence assertions.
- **M-062 → FIXED** (upstream): start failure fatal (exit 6 + testsigning
  guidance, single-string message) + a RUNNING confirmation (exit 7);
  -NoStart prints the installed-stopped state.
- **M-063 → FIXED**: the environments contract (PREBUILT-ARTIFACTS §2) +
  the [windows-2022, windows-2025] matrix on the driver lane + the rda-
  driver release body's build-vs-install evidence separation (`f569836`).
- **N-084 → FIXED**: the timestamp rationale (PREBUILT-ARTIFACTS §2 +
  DRIVER-RELEASE §7) + the composite's timestamp_server input.
- **N-085 → FIXED**: the operator-posture note (dev/CI only; the silent
  installer is S-038's packaging milestone).
- **M-064**: the SCRIPT half landed (Parameters write+verify, per-call SCM
  errors, the exit-code contract); the %s%d versioned-coexistence +
  -Uninstall-verb halves remain OPEN (tracked in this module row) — the
  driver's naming already supports the variants (rda_control.c), the
  deployer logic is the future batch.

## SA-6 wave-2 lane (2026-10-28 — S-038 step 1: the contract doc)

- **S-038 step 1 LANDED**: `docs/PACKAGING.md` — the 13-parameter contract
  (K74 silent flags 1-5, admin 6, ARP 7-8, K76 unins%.3d 9, Inno metadata
  10, K77 telemetry 11-12, version key 13), every parameter with its
  verbatim string-table citation; the reference's install architecture
  decoded; the technology decision (default: Inno Setup, the evidence-pinned
  choice), the modernized OS gate (Win7 SP1 → Win11 + Server 2025 —
  REGISTERED divergence), the write+verify version key, the single-instance
  default, the driver install as an internal step (absorbing N-085), the
  three test gates, and the milestone acceptance. The telemetry posture
  (default recommendation: omit) and the technology call are the two OPEN
  product decisions recorded in the doc. S-038 stays ACTIVE (steps 2-4:
  implementation, tests, acceptance) — its step-1 gate is closed.
