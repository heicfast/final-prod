# SURFACE-INVENTORY.md — the corpus-derived surface inventory (the audit's scope)

> **Gate document.** The authoritative enumeration of every surface the
> research corpus documents, with its corpus sources and its audit-coverage
> status. The audit's scope is this inventory — derived from the corpus, not
> from any prompt. Statuses:
> - **AUDITED** — a module file + findings cover it (see `modules/`, the matrix).
> - **PARTIAL** — covered with named gaps.
> - **NEVER-AUDITED** — corpus documents it; no module row / finding dispositions it.
> - **OUT-OF-SCOPE** — a registered scope exclusion (`docs/PARITY-DELTAS.md` / CAPABILITY-MATRIX §3), recorded, not a silent gap.
> - **S7-ACTIVE** — the parallel driver-only-era lane's claimed territory.
>
> Compiled by the dev-sign+parity session (SA lane, 2026-10-27/28) from a full
> corpus walk: 282 inventoried docs (273 artifacts + 9 architecture docs);
> citation status cross-checked per artifact against the entire `diff-to-fix/`
> trail (78 cited / 204 never-cited by basename; context-verified). Corpus
> `03ad46c`; core `dd82db9`-lineage (see README).

## 1. Kernel-driver / transport surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| EPMNtDrv raw-disk engine (eprdtdrv/ebrntdrv/epmdkdrv/win7older) | `architecture/02-kernel-driver-architecture.md`; `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` (v1, cited ×2); `re-workspace/reversed/ghidra-drivers/` | The raw-disk proxy driver family: grammar, device namespace, read flow, PID checks | AUDITED (modules/transport.md) — **driver-only era re-audit in S7 lane** |
| EPMNtDrv identity IOCTLs + async completion engine (**v2 internals**) | `artifacts/PartitionRecovery/epmntdrv-internals-v2--completion-engine-identity-ioctls.md` | The identity-IOCTL contract (GET_INFO/GET_VERSION-class), async completion engine, `\Volume\{N}` resolution chain | **NEVER-AUDITED (NEVER-CITED)** — ground truth for the S7 lane's identity-IOCTL adjudication; flagged to S7 |
| EPMNtDrv trio naming + PID handshake | `artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md` | Runtime-composed device naming + the full two-sided PID-token handshake | **NEVER-AUDITED (NEVER-CITED)** — S7 lane (transport territory) |
| EPMNtDrv trio byte-diff | `artifacts/PartitionRecovery/epmntdrv-trio--byte-diff-verification.md` | ebrntdrv ≡ eprdtdrv_win7older byte identity etc. | AUDITED (M-030 ×1) |
| EUDCPEPM/EUEDKEPM WinPE hook pair | `architecture/02` §3; `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md`; `--hook-framework-urgent-cache.md` | Disk-stack hook + named raw-disk door (WinPE) | AUDITED (M-030/I-021 context; S7 lane active on driver family) |
| EuMrx NAS redirector | `architecture/02` §4; `artifacts/EuShare/*` (5 artifacts, cited) | RDBSS mini-redirector for NAS scan | OUT-OF-SCOPE (network/NAS exclusion) — install scripts noted in §6 below |
| Driver install lifecycle (live) | `architecture/02` §5; `per-binary/setupdrvx64.md`; `deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3 | setupdrvx64 copy+create+start; `-Uninstall` mode; `%s%d` versioned coexistence; write+verify; per-call SCM errors | **PARTIAL** — SA-lane Phase 1 filed S-036/M-060/M-062/N-085; the -Uninstall/versioning/write-verify parameters NEVER-AUDITED (see §6) |
| Driver pre-registration (WinPE) | `og-codebase/BUILDPE/x64/regdrv.reg`; `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` | Offline-hive service matrix (4 services + disk-class UpperFilters), eprdtdrv absent from PE | **PARTIAL** — M-030 evidence cites; no dedicated finding owns the UpperFilters write / Start-split / eprdtdrv-absence parameters |
| DRWService (BitLocker FVE service) | `architecture/02` §5.2; `artifacts/DRWService-exe--bitlocker-service.md` | On-demand service: `/install` `/uninstall`; AUTO_START "Read FVE Server", LocalSystem | **PARTIAL** — data path AUDITED (S-014, modules/health.md); **install lifecycle NEVER-AUDITED** |
| RDA driver contract (our driver) | core-side: `docs/DRIVER-CONTRACT.md`, `docs/DRIVER-RELEASE.md`, `docs/PREBUILT-ARTIFACTS.md` | The merged product's own driver + prebuilt conventions | Phase-1 AUDITED this session (S-036..I-026); wire-constant parity = S7 lane |

## 2. Core engine surfaces (filesystems)

| Surface | Corpus sources | Status |
|---|---|---|
| NTFS engine | `artifacts/SearchDLL/ntfs*` (multiple, cited); ghidra-searchdll decompiles; `ifileproperties-part1` (NTFS schemas) | AUDITED (modules/ntfs.md; 40+ findings) — **part1 field-schema artifact under-cited (×1)** |
| FAT engine | `artifacts/SearchDLL/fat*`; `ifileproperties-part1` (FAT schemas) | AUDITED (modules/fat.md) — part1 under-cited |
| exFAT engine | `artifacts/SearchDLL/exfat*`; `ifileproperties-part1` (exFAT schemas) | AUDITED (modules/exfat.md) — part1 under-cited |
| ext4 engine | `artifacts/SearchDLL/ext4*` (cited); CORRECTIONS #4/#6 applied | AUDITED (modules/ext4.md) |
| HFS+ engine | `artifacts/SearchDLL/hfsplus*` (cited) | AUDITED (modules/hfsplus.md) |
| Btrfs engine | `artifacts/SearchDLL/btrfs*` (cited); CORRECTIONS #8 applied | AUDITED (modules/btrfs.md) |
| ZFS engine | `artifacts/SearchDLL/zfs*` (cited) | AUDITED (modules/zfs.md; S-034/A-008 lineage) |
| ReFS engine | `artifacts/SearchDLL/refs*` (cited); tracked-open OQ residuals | AUDITED (modules/refs.md; N-036 registered) |
| LDM dynamic disks | `artifacts/SearchDLL/ldm*` (cited) | AUDITED (modules/ldm.md) |
| Session .rsf format | `artifacts/SaveScan/*` (5/5 cited); `ifileproperties-part1/2/3` | AUDITED (modules/session.md) — **part1 (NTFS/FAT/exFAT schemas + 23-slot interface) under-cited ×1 vs parts 2/3 (×6/×8)** |
| FileFilter / RawConfig config | `artifacts/config-files/*` (cited); `og-codebase/config/RawConfig.ini` | AUDITED (M-023/M-024) — **M-024's "37 toggles" count superseded by CORRECTIONS #5 (actual 34) — propagation pending this session** |
| User-extensible signature DB (EUUserData) | `artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` | **NEVER-AUDITED (NEVER-CITED)** — M-048 (user-signature aid) cites only the roll-up summary, never the DB grammar |

## 3. Carve / raw-signature surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| Raw-signature table + dispatch | `artifacts/SearchDLL/rawsearchdll--raw-signs*` (cited ×13 census) | The signature registry | AUDITED (modules/carve.md) |
| **BruteForce frame-walker family (23 classes)** | `artifacts/SearchDLL/rawsearchdll--family-bruteforce.md` | MP3/ADTS/APE/ASF/M2TS/MTS/PDF/TIFF/DWG/DXF/EPS/FH11/FCPEvt/GIF/JPEG frame-sync validators | **NEVER-AUDITED (NEVER-CITED)** — the carve module's capability row names "BruteForce frame walkers" as the og reference but was evidenced only from the registry census |
| **First-generation File*Analyser family (47 classes)** | `artifacts/SearchDLL/rawsearchdll--family-filegen.md` | Primary+fallback dual-role validators | **NEVER-AUDITED (NEVER-CITED)** |
| **RawSpeed/xjq/special/Brance families (61 classes)** | `artifacts/SearchDLL/rawsearchdll--family-{rawspeed,xjq,special,brance}.md` | HEIC, RAR4, registry infrastructure, ForCellTest + the rest | **NEVER-AUDITED** (brance ×1 only) |
| Carve family layer (two-generation chains) | `artifacts/SearchDLL/rawsearchdll--family-layer*.md` (cited) | The generation/fallback architecture | AUDITED (S-007 family) |

## 4. Orchestration / product-capability surfaces

| Surface | Corpus sources | Status |
|---|---|---|
| Scan orchestration, progress, stages | `architecture/01/05/06`; MOM roll-ups (cited) | AUDITED (modules/orchestration.md) |
| MOM module system (PartitionRecovery .mo) | `artifacts/MOM-modules/` roll-ups (mo-module-map ×16, mom-module-system ×6, censuses) | AUDITED (modules/mom-capabilities.md) — **22 of 32 per-module stubs NEVER-CITED** (per-module RTTI/function censuses for exactly the implemented modules: DeviceManager, Device, FileSystemCheck, NTFSLib, EFIBoot, CheckVersion…) |
| Writer / apply / report | `artifacts/MOM-modules/{apply,write,...}` (cited) | AUDITED (modules/writer.md) |
| Boot repair (BCD/NVRAM) | `artifacts/MOM-modules/EFIBoot*` (cited) | AUDITED (modules/boot-repair.md) |
| Health grading (SMART/NVMe) | `artifacts/DiskHealth/` (1/1 cited) | AUDITED (modules/health.md) |
| Preview subsystem | `artifacts/UIUX/10-preview-subsystem.md` | **NEVER-AUDITED (NEVER-CITED)** — the og preview contract (per-type previewers, 15 s video cap, 100 MB cap, damage-aware Recovery-Chances grading, streaming); only the socket-protocol divergence was registered (I-015, via a different doc) |
| Query / filter / tree | `artifacts/UIUX/*` (partially cited) | AUDITED (modules/query.md) — UIUX docs beyond the cited set are UI-pure (no UI in the merged product) |
| Imaging (disk images) | cited set | AUDITED (modules/imaging.md) |
| Check phase (damage audit) | `artifacts/MOM-modules/FileSystemCheck` (roll-up cited) | AUDITED (modules/check-phase.md; M-041) |

## 5. Product-frame surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| App installer (live Windows) | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`; `per-binary/{SetupUE,unins000,InfoForSetup}.md`; unins000/SetupUE/InfoForSetup string tables | Inno Setup + SetupUE OS-gate bootstrap + InfoForSetup telemetry; silent surface (/SILENT //VERYSILENT /SUPPRESSMSGBOXES /NORESTART); ARP QuietUninstallString; unins%03d multi-uninstallers; admin-required uninstaller | **NEVER-AUDITED** (SA-lane Phase 1 covered only the DRIVER install slice; the app-installer surface has no finding) |
| Install/uninstall telemetry + retention | InfoForSetup strings (`/SendInfo` `/SendInfoEx` `/UninstallStart` `/UninstallEnd`); `artifacts/Licensing/licensing-activation-chain.md` (:29,:45-46) | Uninstall is an instrumented, monetized lifecycle event | **NEVER-AUDITED** |
| WinPE boot/deployment | `architecture/03-winpe-boot-environment.md`; `artifacts/WinPE/{boot-config,pe-payload-census}.md`; BUILDPE tree (winpeshl.ini, auto.bat, regdrv.reg, software.reg, Unattend.xml) | Shell-replaced PE: launch order, pseudo-desktop build, OEM branch, companion deployment, media builder (PE_Tools + WAIK carve + DISM + IMAPI2) | **PARTIAL** — M-030/I-021 cite fragments as scope context; the launch chain, OEM branch, media-builder pipeline, and installer stamping of version.ini are NEVER-AUDITED |
| DRWService install lifecycle | see §1 | see §1 | **NEVER-AUDITED** (install half) |
| EuMrx scripted install/uninstall | `og-codebase/EuShare/{install,uninstall}.bat` | sc create filesys + NetworkProvider + ProviderOrder prepend/restore (with the og's own malformed-`if` bug at uninstall.bat:39) | OUT-OF-SCOPE (NAS) — **recorded here because the ProviderOrder write/restore pattern is installer-surface ground truth** |
| Crash supervisor / launcher chain | `artifacts/DRW-exe--launcher.md`, `DRWReport-exe--crash-reporter.md` | DRW.exe → DRWUI supervision | OUT-OF-SCOPE (no UI/launcher in the merged product) |
| File repair line | `artifacts/FileRepair/` (8/8 never-cited) | EUFileRepair product line | OUT-OF-SCOPE (not implemented; CAPABILITY-MATRIX §3) |
| Cloud / licensing / languages / runtime families | `artifacts/Cloud/`, `Licensing/` (beyond retention), `Languages/`, `runtime-families/` | Cloud pairing, activation, localization, runtime DLL census | OUT-OF-SCOPE (registered exclusions) |
| per-binary identity cards (114) | `artifacts/per-binary/` (113/114 never-cited; libvmdk ×1) | Per-binary RTTI/import/strings censuses | Reference material — consulted per-surface above; not individually auditable surfaces |

## 6. The corpus's installer-behavior parameters no finding owns (the §5 app-installer ground truth, enumerated)

From the corpus (A2's extraction, this session): (1) Inno Setup + bootstrap pair; (2) /SILENT; (3) /VERYSILENT; (4) /SUPPRESSMSGBOXES; (5) /NORESTART(+Applications); (6) ARP `UninstallString` + `QuietUninstallString`; (7) unins%03d multi-uninstallers; (8) Inno component/task/user-info-serial metadata; (9) admin-required uninstaller; (10) SetupUE 16-entry OS gate (Win2000→Server 2016, WOW64-aware); (11) `SOFTWARE\EASEUS\DRW\Version` registry contract; (12) install telemetry (/SendInfo fields); (13) uninstall telemetry + retention triggers; (14) setupdrvx64 `-Uninstall`; (15) `%s%d` versioned driver coexistence; (16) Services write+verify + per-call SCM errors; (17) DRWService `/install`//`/uninstall`; (18) AUTO_START LocalSystem "Read FVE Server"; (19) EuMrx install.bat pattern; (20) EuMrx uninstall.bat (+ its own bug); (21) winpeshl launch order; (22) regdrv.reg UpperFilters + Start-matrix + eprdtdrv-absence; (23) auto.bat OEM branch; (24) USB companion deployment; (25) software.reg + Unattend display floor; (26) the WAIK/DISM/IMAPI2 media builder; (27) installer stamping of version.ini (SetupBuildTime, xurlPageID).

## 7. Session wave order (corpus-determined: highest integration-risk / user-impact first)

1. **Carve families** (23+47+61 analyser classes vs the core's grammar set) — carve results are user-facing; the core's capability row claims rest on the census, not the family semantics.
2. **Session field schemas** (`ifileproperties-part1` NTFS/FAT/exFAT vs the crsf round-trip).
3. **Installer surface** (§5/§6 — extends the Phase-1 findings into the app-installer lane).
4. **Preview subsystem** (UIUX/10 vs modules/preview.md).
5. **User signature DB grammar** (userdata-remoteconfig vs cr_usersigns).
6. **WinPE payload census** (PE capability boundaries vs imaging/boot media claims).
7. **MOM stubs spot-check** (22 never-cited per-module censuses vs mom-capabilities roll-ups).
8. **CORRECTIONS #5 propagation** (M-024's 37→34 toggles across the trail).
(S7 lane, parallel: the driver-only-era wire/IOCTL/taxonomy audit + the two NEVER-CITED driver docs flagged to it in §1.)

## 8. Coverage arithmetic (this inventory)

282 docs inventoried · 78 cited (73 artifacts + 5 architecture) · 204 never-cited,
of which: OUT-OF-SCOPE ≈ 152 (per-binary 113, runtime-families 10, FileRepair 8,
UI-pure UIUX ~11, Cloud/Licensing/Languages remainder) · S7-flagged 2 ·
IN-SCOPE NEVER/PARTIAL ≈ 50 (the §2–§6 rows above). The §7 waves target them.
