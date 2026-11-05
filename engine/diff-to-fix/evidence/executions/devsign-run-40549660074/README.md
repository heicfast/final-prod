# devsign-run-40549660074 — the dev-sign + install + uninstall execution evidence

**What this is:** the captured transcript of the merged product's documented
dev-sign / install / uninstall path executed on a real GitHub-hosted Windows
runner, plus the exact release assets the run consumed.

**Run:** https://github.com/heicfast/core_recovery/actions/runs/40549660074
(workflow `driver-install.yml`, run #4, `workflow_dispatch`, **conclusion:
success**) · audited repo commit `dd82db9` (workflow content = `3bda8e2`'s fix
state) · runner `windows-2022` (env `ImageOS=win22`, GitHub-hosted).

**Inputs resolved by the run:**
- driver release: `heicfast/rda-driver` tag **v0.2.2** (latest, ≥ v0.2.1 floor)
  — all 7 assets downloaded, SHA256SUMS-verified (`resolved-release.json`)
- core release: `heicfast/core_recovery` tag **v0.5.1** — `core-recovery-v0.5.1.zip`
  + tier-mate `rda.sys`/`rda.inf`/`install.ps1`/`uninstall.ps1` + `MANIFEST.json` +
  `SHA256SUMS`

## Files

| File | What it proves |
|---|---|
| `job-log.txt` | The full job transcript: fetch, SHA256 verification, cert creation, signtool, install.ps1 failure + sc.exe fallback, `sc query` RUNNING, control-device open, all 3 phases, uninstall |
| `env-manifest.json` | Runner image + resolved tags (the run's own manifest) |
| `resolved-release.json` | The exact driver release resolved + the verified asset list |
| `driver-status-ready.txt` | The 5-facet driver-status probe (present/loaded/started/authorized/healthy all `yes`, state `ready`, version 1.0, flags=2, max_disks=200) |
| `release-scripts/install.ps1` | The exact installer script the run executed (rda-driver v0.2.2 asset, verbatim copy) |
| `release-scripts/uninstall.ps1` | The release's uninstaller — NOT executed by the run (see finding N-085-adjacent / M-061) |
| `release-scripts/rda.inf` | The release INF (service profile + Parameters defaults) |
| `release-scripts/SHA256SUMS` | The release's checksums |

## The key transcript lines (job-log.txt)

- `:229-230` — cert created (`CN=RRECOV-CI`, CodeSigningCert, 2-day validity)
  and `Successfully signed: driver\rda.sys` (signtool `/fd SHA256`, **no
  timestamp server** — see N-084)
- `:232-234` — **install.ps1 throws** `You cannot call a method on a
  null-valued expression` (InvokeMethodOnNull, `install.ps1`) — the documented
  installer fails at its elevation guard before doing anything (no
  `Installing...` output precedes it; the only method-call site before the
  first output is the guard's
  `[Security.Principal.WindowsPrincipal]::[Security.Principal.WindowsIdentity]::GetCurrent()`)
- `:236` — `install.ps1 failed (1) - manual sc.exe fallback` — the composite's
  undocumented fallback is what actually installs
- `:237-248` — `[SC] CreateService SUCCESS` + `sc query` → `STATE: 4 RUNNING`,
  KERNEL_DRIVER, PID 0
- `:249` — `driver installed: service RUNNING + control device open OK`
  (`\\.\RDA` open/close round-trip)
- `:279` — `rrecov driver --json` → `state:"ready"`, all 5 facets `yes`,
  `major:1 minor:0 flags:2 max_disks:200` (GET_INFO + GET_VERSION control
  round-trip)
- `:289,290` — `scan --disk 99` proceeds past the driver gate, transport
  attempts the grammar path (rc=50020001 open failed on the nonexistent disk)
  → the round-trip beyond open/close
- `:308-309` — Phase 3: `sc stop` → state `not-started`,
  `error_code 1342898178` (0x500B0002 ERR_DRIVER_NOT_STARTED)
- `:334-337` — uninstall: raw `sc.exe stop/delete` + `Remove-Item` (NOT
  uninstall.ps1) — exit 0

## Reproduction

```
# from the audited repo (workflow_dispatch; ~30 s wall clock):
POST /repos/heicfast/core_recovery/actions/workflows/driver-install.yml/dispatches
     {"ref":"main"}
# evidence: the run's job log + the driver-install-evidence-<run_id> artifact
```

The same mechanism (cert + sign + sc create + start) is reproducible on any
windows-2022-class runner; on HVCI/SecureBoot-enforcing stock Windows 11 it
does NOT load without test-signing — see finding M-063.
