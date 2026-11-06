# D-PRIV-01 — the app does not request administrator at launch

**Reproduced on:** the shipped v0.8.4 release, installed on a real
windows-2022 runner (forensic run
[42255492260](https://github.com/heicfast/final-prod/actions/runs/42255492260),
artifact `forensic-repro-evidence`).

## Expected

The product requires the administrator token for every raw-disk
surface; it must request elevation AT LAUNCH through the standard
Windows flow (one UAC prompt), like the reference (DRW.exe AND
DRWUI.exe embed `requireAdministrator`,
corpus rev/DRWUI.exe/strings_ascii.txt:41023).

## Observed

- `facts-postinstall-appmanifest.json` — the probe's own manifest
  classification of the installed exe: **`"no-manifest-match"`** (no
  `requireAdministrator`, no `asInvoker`, no trustInfo at all — the exe
  carries only tauri-build's stock Common-Controls manifest; the loader
  default is asInvoker).
- The app only ever ran elevated on the runner because the PROBE
  pre-elevates the launch (`launch-app.ps1`); the unelevated twin leg
  (`unelevated.json`, `unelevated-token.json`) shows the in-app
  restart-as-admin banner flow — the lazy secondary elevation the user
  rejected, and the off-theme banner surface (D-UI-01).
- Three documents claimed `requireAdministrator` while the binary had
  none (recoverywizard.nsi:27-29, docs/INTEGRATION-PLAN.md §7.4,
  docs/INSTALL-FOOTPRINT-CURRENT.md:205) — the documentation
  hallucination, recorded in REPO-STATE.

## Break point

`ui/src-tauri/build.rs` called bare `tauri_build::build()` — tauri-build
2.7.4's stock embed carries NO trustInfo section. No manifest existed
anywhere in the tree (0 `.manifest` files at the reproduction commit).

## Fix (landed)

`ui/src-tauri/app.manifest` (requireAdministrator + Common-Controls)
wired through `tauri_build::WindowsAttributes::app_manifest`
(ui/src-tauri/build.rs:38-53, commits 76bdf056 + 68460a4c); the banner
surface REMOVED (reference parity — no in-app elevation notice exists
in the og); the three docs trued. Validation on the NEXT release: the
release lane's manifest-dump assertion + the re-vehicled probe gate
(the v0.8.4 binary predates the fix by construction).
