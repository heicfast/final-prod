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
