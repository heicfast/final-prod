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
| K74 silent/unattended surface (app level) | **FIXED (S-038, steps 1-4; run 40684304129)** — the Inno installer shipped (`installer/core-recovery.iss` + the three gates; repaired SA-10: this row's "all absent" verdict predated the PARITY-FIX landings) |
| K77 install-time registry + telemetry | **FIXED (S-038)** — the version-key write+VERIFY and the modernized OS gate shipped; telemetry OMIT by the recorded §8 decision |
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
  product decisions recorded in the doc. **S-038 is FIXED (steps 1-4; the
  PARITY-FIX-9 close-out + run 40684304129 all-green — repaired SA-10: this
  "stays ACTIVE" note predated the landings).**

## SA-6 probe lane (2026-10-28 — the M-064 close-out: the v0.2.4 remainder landed)

- **M-064 FULLY FIXED** (upstream v0.2.3 `6b50bb8` + v0.2.4 `323e75a`; the
  finding carries the full Resolution): `-Layout Versioned` (default) —
  `<ServiceName><MajorMinor>.sys` from the source image's version resource
  (the og's `%s%d` coexistence; an in-use image is never overwritten); the
  different-version in-place upgrade (stop-poll → `sc config binPath=` →
  the shared Parameters write+verify → the shared start leg; the old file
  stays; exit codes 8/9); `-Uninstall` (the current-engine delegation —
  one implementation, both entry points); the versioned-aware teardown
  (the service's actual ImagePath + the `<ServiceName><digits>.sys`
  leftover sweep); the CI PowerShell AST parse gate (the v0.2.3 lesson).
  The core's driver-status discovery needed no change (the service carries
  the binPath; the composite's assertions are dynamic).

## Session-9 addendum (2026-10-28, SA-9 lane — rda-driver @ `323e75a` v0.2.4)

- **M-086 (MAJOR — the session-9 lane filed this as M-071; renumbered at the push-time reconciliation, repaired SA-10)** — the FileVersion pin: `rda_version.h`/`rda.rc`/
  `rda.inf` carry 1.0.0.0 across all v0.x releases, so `-Layout Versioned`
  always computes `<name>10.sys` and the M-064 in-place-upgrade branch
  (`$imagePath -ne $destPath`) is unreachable — every reinstall exits 2.
  The M-064 feature is inert until the resource is un-pinned (release-tag
  mirror + a CI gate asserting tag == resource).
- **M-087 (MAJOR — filed as M-072 by that lane; renumbered at the reconciliation, repaired SA-10)** — the WinPE profile `scripts/winpe/regdrv-rda.reg`:
  unparseable `hex(7):r,d,a,\,0` (the og's own regdrv.reg demonstrates
  the UTF-16LE form `72,00,64,00,61,00,00,00,00,00`); the UpperFilters
  write targets GUID_DEVCLASS_DISKDRIVE where the og's file writes
  GUID_DEVCLASS_VOLUME; Start=3 + FilterMode=1 + UpperFilters cannot
  attach at stack-build (the og's filter service is Start=0 BOOT_START).
  The D4 handoff + two newly-verified parameters; the SURFACE-INVENTORY
  WinPE row now owns them here.
- The EPMNtDrv-v2/trio MATCH records and the D3/D2 doc-truth findings
  live in modules/transport.md's addendum (wire-side).


## SA-10 addendum (session 10 — the K71-K73 walk's DRWService slice: M-105)

The K-walk above scores the RDA-driver slice only. The DRWService (BitLocker FVE
service) slice — a DIFFERENT service, never scored — is now owned by **M-105**
(filed this session): the og's transient SCM lifecycle ("Read FVE Server",
AUTO_START/LocalSystem/OWN_PROCESS, /install replace-on-install + StartServiceA,
the stop-gated delete, per-call SCM error strings, DRWUI's per-use
uninstall→install→harvest→2s→uninstall orchestration) decoded at the decompile
level; the core has no counterpart and R-83's disposition note
(REFERENCE-API-GAP.md:425, "install lives in driver-repo CI scripts") is false
for this service. K71/K72/K73 gain the DRWService sub-verdict: absent (with the
fix plan's (a) NO-SERVICE / (b) TRANSIENT HELPER decision frame).

Currency note (SA-10): M-086/M-087/M-089/N-107/N-127 (the driver-contract
batch) were re-verified standing at the reconciliation HEAD by this session's
triage lane; the driver-repo-side items belong to that repo's lane per the
two-repo grant.
