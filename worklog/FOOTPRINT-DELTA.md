# FOOTPRINT-DELTA — the reference footprint vs the product, per module

> Session 2026-11-17 (T14). Sources: `worklog/REFERENCE-FOOTPRINT.md` (the
> census: whole capture 1,470 files / 670.3 MiB; live install tree 1,276 files
> / 494.7 MiB; BUILDPE 194 / 175.6 MiB; every figure re-measured this corpus,
> cross-checked 1:1 against `re-workspace/inventory.{csv,json}`) and the
> product's own `docs/INSTALL-FOOTPRINT-CURRENT.md` (8 files / 8.2 MiB). The
> delta is 159× files / 60× bytes — this sheet reconciles it per module, with
> a resolution for every row. The mandate's rule: the installer is not padded
> to a file count; it is made COMPLETE so the product does what it claims.

## 1 The reconciliation table

| Reference module | Files / MiB | What it is | Our resolution |
|---|---|---|---|
| Root exes + search DLLs (DRWUI, 9×SearchDLL, config) | ~24 exe + ~100 dll / ~130 MiB | The product engine + UI + runtimes | **CONSOLIDATED**: one engine (cr_ui.dll) + one Tauri app; the 9 per-FS search DLLs are one engine's per-FS modules (ntfs/fat/exfat/ext2/hfsplus/btrfs/zfs/refs + carve); capability parity tracked per-FS in PARITY-DELTAS |
| Qt 5.15 runtime (root + diskhealth/) | ~600 / ~150 MiB | Two full Qt stacks (Widgets for DRWUI, QML for DiskHealth) | **CONSOLIDATED**: WebView2 (the OS renderer) + React; no bundled GUI runtime |
| diskhealth/ (DiskHealth.exe companion) | 818 / 52.81 | The separate SMART surface (94% private Qt/QML runtime; the content is ~3 MiB) | **IMPLEMENTED-EQUIVALENT**: the separate disk-health window (D-P4, commit 2a4b02ed) — 840×590, all-disks selector, 4-state grades, the reference's table; shipped inside the app (recorded sub-divergences in D-P4) |
| VideoViewer/ | 76 / 99.59 | Video preview player + ffmpeg 7 + repairer | **DEFERRED** (D-P8): truthful metadata + not-rendered hint today; renderer bundling is a recorded supply-chain decision |
| EUFileRepair/ | 87 / 45.63 | Standalone file-repair workbench (ImageMagick + 4 engines) | **DEFERRED** (G-1(b)): the repair surface in-app is the transactional VOLUME repair (cr_fsck), not a file-repair workbench — the og's is a separate product surface |
| BUILDPE/ | 194 / 175.56 | Bootable WinPE media payload | **OUT OF SCOPE**: emergency-boot media is a separate deliverable; recorded |
| ResourceDark/ + Resource/ | 4 / 60.18 | Qt resource blobs (light/dark themes, icons) | **CONSOLIDATED**: the app's own themed assets |
| 17 product language packs + 23 qt_*.qm | 40 / ~15 | Localization | **PARTIAL**: en + zh-CN shipped (the product's audience); the i18n sweep tests keep parity |
| Cloud/telemetry/monetization (EPM, toasts, activation, Aliyun) | ~50 / ~10 | License, upsell, tracking | **OUT OF SCOPE** (deliberate): no telemetry, no activation theater (F-ACT-01 removed it), no upsell surfaces |
| 200 UCRT shims (VideoViewer's) | 200 / ~4 | MSVC runtime copies | N/A (not needed without VideoViewer) |
| SaveScan/*.rsf + logs | 11 / 66.0 | RUNTIME artifacts, not install-time | N/A (our sessions live in ProgramData) |

## 2 The honest summary

The functional gaps the user can perceive (vs the 1,276-file reference):
1. **Rendered video/document previews** — D-P8, DEFERRED with scope.
2. **The file-repair workbench** — G-1(b), DEFERRED (a separate product
   surface; our repair is the transactional volume repair).
3. **Bootable WinPE media** — out of scope for this product.
Everything else the reference ships is either implemented (the disk-health
surface, the scan pipeline, the FS engine set, the session format) or
consolidated by design (one engine, one GUI stack, no telemetry).

Installed-size posture: 8.2 MiB vs 494.7 MiB live — the difference is the
media-renderer bundle (D-P8's ~130 MiB family), the repair workbench (~46),
WinPE (~176), the language packs, and two Qt runtimes replaced by the OS's
WebView2. The product is not "missing 380 files' worth of functionality";
it is missing exactly the three items above, each recorded.

## 3 Session addendum (2026-11-18, Installer Integration session — W1)

The reference's `EUUserData` install-tree placement class (the 111-entry
ext-signature JSON in the install root; REFERENCE-FOOTPRINT §1.3 — the
engine-init default user-sign set feeding the CHK repair chain in the og)
had a product equivalent that existed but was NEVER placed:
`engine/data/userdata-defaults.json` (111 entries, own original data) sat
in-repo, both consumers probed it exe-adjacent at startup (CLI
`engine/src/app/rrecov.cpp:1354`, bridge `ui/src-tauri/src/commands/system.rs`
`USER_SIGN_DEFAULTS_FILE`), and no build lane staged it, the installer placed
nothing for it, and the portable zip missed it — every clean install started
at `userSigns: 0` while the capability and the file both existed
(INTEGRATION-MATRIX G-1/FM-14 — the one real placement defect of the
reconciliation). Resolution (W1, this session): `build.ps1` stages it into
the engine kit; the `.nsi` kit `File` places it flat in `$INSTDIR` (no
placement-rule change — the kit glob already covers data files); the
uninstaller deletes it explicitly (the `*.dll` catch-all does not cover
data files); `installer-matrix.ps1` asserts it in the exact module set;
the portable zip inherits it via its kit copy. `docs/INSTALLER.md`'s
placement table lists it. The og's own EUUserData file stays out of the
product (the IP boundary): the two share the CAPABILITY, not the bytes.
