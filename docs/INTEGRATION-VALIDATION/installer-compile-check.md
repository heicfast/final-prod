# Installer compile validation — `recoverywizard.nsi`

> Evidence artifact for `docs/INTEGRATION-VALIDATION.md` (T11 acceptance,
> compile-scope). Captured 2026-10-26T12:00Z by the integration lane.

## What was validated

`installer/recoverywizard.nsi` — the combined product installer (517 lines:
dual-mode MultiUser flow, silent inner Tauri setup orchestration, engine-kit
integrity gates, consent-gated RDA driver step via the driver's own
`install.ps1`, ProgramData shared state, inner-uninstall-key takeover,
complete uninstall choreography) — **compiles clean with zero warnings and
produces a valid Windows installer artifact.**

## Toolchain

Cross-compilation on Linux (no Windows host needed for compile validation):

| Component | Version | Source |
|---|---|---|
| makensis | 3.12-1 | Debian `nsis` package (compiler) + `nsis-common` (Include/Stubs/Plugins data), extracted locally, `NSISDIR` pointed at the data set |
| Script | NSIS 3 syntax, Unicode, LZMA solid | `installer/recoverywizard.nsi` |
| Headers used | MUI2, LogicLib, FileFunc, MultiUser, nsDialogs, StrFunc (`${Using:StrFunc}` activation), WinVer-free Win10 gate (registry-based) | all from the stock Include set |

## Command (reproducible)

```sh
makensis -V2 \
  -DPRODUCT_VERSION=0.5.0 \
  -DAPP_BUNDLE=<staged>/app-setup.exe \
  -DENGINE_KIT=<staged>/engine \
  -DDRIVER_STAGE=<staged>/driver \
  -DOUT_DIR=<out> \
  /abs/path/to/installer/recoverywizard.nsi
```

`<staged>` carried stand-in payload files with the real kit's names
(`cr_ui.dll`, `cr_ui.lib`, `rrecov.exe`, `zlib1.dll`, `lz4.dll`, `zstd.dll`;
`rda.sys`, `rdacli.exe`, `rda.inf`, `install.ps1`, `uninstall.ps1`) so every
`File` instruction resolved exactly as the real build lanes stage them
(build.ps1 `Invoke-Engine` / `Invoke-Driver`).

## Results

| Check | Result |
|---|---|
| makensis exit code | **0** |
| Warnings | **0** (after fixes: StrFunc `${Using:StrFunc}` activation; description block reordered after Section definitions — the original order produced `unknown variable/constant {SEC_APP}` warnings and would have broken hover descriptions at runtime; `MULTIUSER_EXECUTIONLEVEL Highest` define-name fix; registry-based Win10 gate) |
| Output artifact | `RecoveryWizard-0.5.0-setup.exe` — **valid PE32 Windows GUI executable, Nullsoft Installer self-extracting archive, 7 sections** (192,538 bytes with stand-in payload) |
| Negative test (defines omitted) | **fails loudly**: `!error: PRODUCT_VERSION must be defined (build.ps1 passes it from the root VERSION file)` — exit 1, before any output is written |
| License / icon embedding | MIT `LICENSE` from repo root + `ui/src-tauri/icons/icon.ico` both resolved and embedded |

## Scope (what this does and does not prove)

**Proves**: the script is valid NSIS 3; every macro/include/plugin resolves;
every compile-time `File` reference matches the staged kit names; the define
contract with `build.ps1` is enforced (`!error` guards); the MUI page set,
sections, custom driver page, and uninstaller section compile into a real
self-extracting installer.

## Re-validation — round 11 (`/SD` silent-mode fixes)

Re-compiled after the validation-round-11 fixes (every silent-path
`MessageBox` gained `/SD`; see `installer/README.md` "Silent installs —
the `/SD` discipline" for the root cause and evidence): makensis 3.11-1
(Debian, cross-compile, `NSISDIR` at the extracted data set, stand-in
payloads with the real kit names, same defines as `build.ps1`) —
**exit 0, zero warnings** (`-V3` full-verbosity output greps clean),
artifact `RecoveryWizard-0.5.0-setup.exe` (173,960 bytes with stand-in
payloads, 7 sections).

**Does not prove (pending, by design)**: runtime behavior on Windows —
install layout, inner-setup orchestration, SCM service registration, ProgramData
creation, uninstall completeness, update/repair. That is the T15 lane
(clean Windows runner matrix: per-user/per-machine × driver/no-driver ×
install/update/uninstall/repair), whose evidence lands in this directory as
`installer-vm-matrix.md` when it runs. The driver's own `install.ps1` /
`uninstall.ps1` semantics invoked by this installer are verified by the driver
repository's own test plan (DRIVER-DEPLOYMENT §8, §13).
