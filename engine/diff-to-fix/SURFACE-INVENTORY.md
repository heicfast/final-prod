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
>
> **Session-9 currency pass (2026-10-28, SA-9 lane):** corpus pin advanced to
> `be0cfe9` (CORRECTIONS.md #9 — unchanged from session-8's T1; every citation
> above remains valid at that tree). The session-9 full rewalk corrected this
> header's own bookkeeping: 274 artifact files / 244 unique basenames (30
> shared between MOM-modules/ and per-binary/); **92 cited by literal
> basename / 110 in any form / 134 never-cited in any form**; the EuShare
> row's "5 artifacts" was wrong (2, both dir-shorthand-cited); the
> raw-signs/family-layer globs matched no artifact basename (corrected in
> place to the real artifacts). The never-audited EPMNtDrv v2 + trio rows
> are **AUDITED this session (MATCH)**; the carve row gains S-040/S-041/
> S-042/N-092; the WinPE pre-registration row is now OWNED by M-072.

## 1. Kernel-driver / transport surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| EPMNtDrv raw-disk engine (eprdtdrv/ebrntdrv/epmdkdrv/win7older) | `architecture/02-kernel-driver-architecture.md`; `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` (v1, cited ×2); `re-workspace/reversed/ghidra-drivers/` | The raw-disk proxy driver family: grammar, device namespace, read flow, PID checks | AUDITED (modules/transport.md) — **driver-only era re-audit in S7 lane** |
| EPMNtDrv identity IOCTLs + async completion engine (**v2 internals**) | `artifacts/PartitionRecovery/epmntdrv-internals-v2--completion-engine-identity-ioctls.md` | The identity-IOCTL contract (GET_INFO/GET_VERSION-class), async completion engine, `\Volume\{N}` resolution chain | **AUDITED (session 9) — MATCH** (C-sheet corpus decode vs E-sheet code read, cross-verified: 0x60002400 GetProcName in==4/out>0x207, 0x60002404 GetFileObjName, 0x60002408 GetDiskContextMode out==4, method-aware pass-through, ×512 Information; corpus-internal V1-§4 mislabel noted for errata) |
| EPMNtDrv trio naming + PID handshake | `artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md` | Runtime-composed device naming + the full two-sided PID-token handshake | **AUDITED (session 9) — MATCH** (the PID-token byte-sum transform re-derived at CREATE, mismatch → STATUS_INVALID_PARAMETER; `\Device\<UPPERCASED service name>` + EPMNTDRV fallback; DO_DIRECT_IO; benchmark-proven 64/64) |
| EPMNtDrv trio byte-diff | `artifacts/PartitionRecovery/epmntdrv-trio--byte-diff-verification.md` | ebrntdrv ≡ eprdtdrv_win7older byte identity etc. | AUDITED (M-030 ×1) |
| EUDCPEPM/EUEDKEPM WinPE hook pair | `architecture/02` §3; `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md`; `--hook-framework-urgent-cache.md` | Disk-stack hook + named raw-disk door (WinPE) | AUDITED (M-030/I-021 context; S7 lane active on driver family) |
| EuMrx NAS redirector | `architecture/02` §4; `artifacts/EuShare/*` (2 artifacts — `eumrx-nas-architecture.md`, `eumrx-rdbss-internals--minirdr-dispatch.md`; dir-shorthand-cited; the earlier "5 artifacts" was a miscount) | RDBSS mini-redirector for NAS scan | OUT-OF-SCOPE (network/NAS exclusion) — install scripts noted in §6 below |
| Driver install lifecycle (live) | `architecture/02` §5; `per-binary/setupdrvx64.md`; `deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3 | setupdrvx64 copy+create+start; `-Uninstall` mode; `%s%d` versioned coexistence; write+verify; per-call SCM errors | **AUDITED this session** (Phase 1: S-036/M-060/M-062/N-085; wave 3: **M-064** the versioned/verified contract absent) |
| Driver pre-registration (WinPE) | `og-codebase/BUILDPE/x64/regdrv.reg`; `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` | Offline-hive service matrix (4 services + volume-class UpperFilters), eprdtdrv absent from PE | **AUDITED (session 9)** — the UpperFilters write / Start-split / eprdtdrv-absence parameters OWNED by **M-072** (regdrv-rda.reg: unparseable hex(7) + DISKDRIVE-vs-VOLUME class + the Start posture; the D4 handoff expanded with two new parameters) |
| DRWService (BitLocker FVE service) | `architecture/02` §5.2; `artifacts/DRWService-exe--bitlocker-service.md` | On-demand service: `/install` `/uninstall`; AUTO_START "Read FVE Server", LocalSystem | **AUDITED (session 10, M-108)** — data path S-014 (retired-FIXED, modules/health.md); **install lifecycle AUDITED as a dedicated finding (M-108)**: the full CreateService shape + /install + /uninstall + the DRWUI per-use orchestration decoded at the decompile level, the core-side absence grep-verified, R-83's false disposition note owned by M-108's fix plan |
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
| FileFilter / RawConfig config | `artifacts/config-files/*` (cited); `og-codebase/config/RawConfig.ini` | AUDITED (M-023/M-024) — **M-024's "37 toggles" count superseded by CORRECTIONS #5 (actual 34) — propagated (N-102): every live count site now reads 34 (PARITY-CORE-REC via N-095; the retired finding file's own :6/:15 stay historical in git history)** |
| User-extensible signature DB (EUUserData) | `artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` | **AUDITED this session** (wave 5: S-039 the caps reject the og's own DB — 15/111 violate, 0 load, scan aborts; M-065 no default set — 79 extensions stock-absent; OQ-SA-3 the consumption-mode probe) |

## 3. Carve / raw-signature surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| Raw-signature table + dispatch | `artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` + `raw-dual-registry--u16-magic-keys.md` (the census pair; the earlier `rawsearchdll--raw-signs*` glob matched no artifact basename — corrected session 9) | The signature registry | AUDITED (modules/carve.md) — **the six family artifacts AUDITED (SA wave 1)** (131 classes → A-010/S-037/N-086 + OQ-SA-2); **session 9 (IDs reconciled SA-10 — the lane's pre-merge numbering corrected): S-042 (WMF placeable layout — FIXED f6e303e), M-081 (CRW/MRW key attribution — FIXED 9e5dedc), S-041 (the batch-2 parent — FIXED-as-umbrella; the school-2/3 children S-047/M-092/M-093/M-096/M-098/M-099/N-111/N-112/N-113/N-115 + the walk follow-ups M-106/M-107), N-093 (MID/GZ caps); corpus errata E53/E54; the CHM/MPEG-TS/EMF/LZH deferral causes RESOLVED** |
| **BruteForce frame-walker family (23 classes)** | `artifacts/SearchDLL/rawsearchdll--family-bruteforce.md` | MP3/ADTS/APE/ASF/M2TS/MTS/PDF/TIFF/DWG/DXF/EPS/FH11/FCPEvt/GIF/JPEG frame-sync validators | **AUDITED this session** (wave 1: A-010 ASF walk semantics + S-037 FH11; the family contract decoded from deep-dive:87-91) |
| **First-generation File*Analyser family (47 classes)** | `artifacts/SearchDLL/rawsearchdll--family-filegen.md` | Primary+fallback dual-role validators | **AUDITED this session** (wave 1: presence/absence dispositioned; semantics map to existing findings; residuals in OQ-SA-2) |
| **RawSpeed/xjq/special/Brance families (61 classes)** | `artifacts/SearchDLL/rawsearchdll--family-{rawspeed,xjq,special,brance}.md` | HEIC, RAR4, registry infrastructure, ForCellTest + the rest | **AUDITED this session** (wave 1: S-037 CPT7/8/9 from the RawSpeed roster; 64 of 67 absences covered by S-007's remainder) |
| Carve family layer (two-generation chains) | `artifacts/SearchDLL/searchdll-family--architecture.md` + the `rawsearchdll--family-{filegen,brance,rawspeed,special}.md` set (the earlier `family-layer*` glob matched no basename — corrected session 9) | The generation/fallback architecture | AUDITED (S-007 family; session 9's S-042 completes the recorded remainder) |

## 4. Orchestration / product-capability surfaces

| Surface | Corpus sources | Status |
|---|---|---|
| Scan orchestration, progress, stages | `architecture/01/05/06`; MOM roll-ups (cited) | AUDITED (modules/orchestration.md) |
| MOM module system (PartitionRecovery .mo) | `artifacts/MOM-modules/` roll-ups (mo-module-map ×16, mom-module-system ×6, censuses) | AUDITED (modules/mom-capabilities.md) — **17 of 30 per-module stubs never-cited by basename, but the wave-7 spot-check verified every one is rollup-covered or infrastructure-only — zero new parameters** (correction: 5 of the inventory's "never-cited 22" — Common, Config, CheckVersion, bitlockerhelp, FileSystemCheck — are cited in brace-group form at s5-session5-corpus-quotes.md:64; the honest count is 17 of 30). Decode candidates registered: CWindowsFS ×3, the DeviceManager offline-OS-detection strings |
| Writer / apply / report | `artifacts/MOM-modules/{apply,write,...}` (cited) | AUDITED (modules/writer.md) |
| Boot repair (BCD/NVRAM) | `artifacts/MOM-modules/EFIBoot*` (EFIBoot.md itself never-cited — M-040 cites the decompile; N-101) | AUDITED (modules/boot-repair.md) |
| Health grading (SMART/NVMe) | `artifacts/DiskHealth/` (1/1 cited) | AUDITED (modules/health.md) |
| Preview subsystem | `artifacts/UIUX/10-preview-subsystem.md` | **AUDITED this session** (wave 4: 19 parameters — 6 MATCH, 6 deliberate-UI-scope, 6 prior-covered; NEW: M-066 archive enumeration for FS-origin archives, N-088 the M-028 propagation tail; OQ-SA-3-adjacent EXIF probe noted in the wave sheet) |
| Query / filter / tree | `artifacts/UIUX/*` (partially cited) | AUDITED (modules/query.md) — UIUX docs beyond the cited set are UI-pure (no UI in the merged product) |
| Imaging (disk images) | cited set | AUDITED (modules/imaging.md) |
| Check phase (damage audit) | `artifacts/MOM-modules/FileSystemCheck` (roll-up cited) | AUDITED (modules/check-phase.md; M-041) |
| GetDriverInfo.dll (TB-lineage drive-letter topology) | `artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md` + `per-binary/GetDriverInfo.md`; `architecture/00-INDEX.md:64` | **REGISTERED session-8 wave-2 (N-106)** — the one surface the corpus index carried with zero trail disposition: GetLogicalDrives/GetDriveTypeW letter-topology MOM module (TB lineage). OUT-OF-SCOPE by lineage for the recovery engine (the DRW flow's own enumeration = the N-071 census); the sixth-source note added to N-071's row |

## 5. Product-frame surfaces

| Surface | Corpus sources | What it does | Status |
|---|---|---|---|
| App installer (live Windows) | `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`; `per-binary/{SetupUE,unins000,InfoForSetup}.md`; unins000/SetupUE/InfoForSetup string tables | Inno Setup + SetupUE OS-gate bootstrap + InfoForSetup telemetry; silent surface (/SILENT //VERYSILENT /SUPPRESSMSGBOXES /NORESTART); ARP QuietUninstallString; unins%03d multi-uninstallers; admin-required uninstaller | **AUDITED this session** (wave 3: S-038 the app-installer surface absent — the 13-parameter contract on the board; string-table evidence verified first-hand) |
| Install/uninstall telemetry + retention | InfoForSetup strings (`/SendInfo` `/SendInfoEx` `/UninstallStart` `/UninstallEnd`); `artifacts/Licensing/licensing-activation-chain.md` (:29,:45-46) | Uninstall is an instrumented, monetized lifecycle event | **AUDITED this session** (folded into S-038's K77 parameter set; the string-table evidence verified first-hand) |
| WinPE boot/deployment | `architecture/03-winpe-boot-environment.md`; `artifacts/WinPE/{boot-config,pe-payload-census}.md`; BUILDPE tree (winpeshl.ini, auto.bat, regdrv.reg, software.reg, Unattend.xml) | Shell-replaced PE: launch order, pseudo-desktop build, OEM branch, companion deployment, media builder (PE_Tools + WAIK carve + DISM + IMAPI2) | **DISPOSITIONED this session** — the capability boundary (zero .mo = no lost-partition search in PE; FREE-by-construction; installer stamps version.ini) recorded as the boot-media builder's design input (roadmap.md:71 + S-038's K78); no defect in shipped code |
| DRWService install lifecycle | see §1 | see §1 | **AUDITED (session 10, M-108)** — the §9 correction below was right: the pre-session-10 "folded into S-038" claim had no trail backing; M-108 is the dedicated finding (the K71-K73 walk's DRWService slice: the CreateService shape, the /install //uninstall verbs, the stop-gated delete, the DRWUI per-use orchestration); the service's data path was already S-014 |
| EuMrx scripted install/uninstall | `og-codebase/EuShare/{install,uninstall}.bat` | sc create filesys + NetworkProvider + ProviderOrder prepend/restore (with the og's own malformed-`if` bug at uninstall.bat:39) | OUT-OF-SCOPE (NAS) — **recorded here because the ProviderOrder write/restore pattern is installer-surface ground truth** |
| Crash supervisor / launcher chain | `artifacts/DRW-exe--launcher.md`, `DRWReport-exe--crash-reporter.md` | DRW.exe → DRWUI supervision | OUT-OF-SCOPE (no UI/launcher in the merged product) |
| File repair line | `artifacts/FileRepair/` (8/8 — all first-cited by S-050) + the 9 og binaries + `remote_config_data` flags | EUFileRepair/Fixo product line (photo/video/office/HWP/raw/archive repair) | **AUDITED this session (SA-10B): S-050** — unimplemented AND the prior row's OUT-OF-SCOPE (CAPABILITY-MATRIX §3) citation was FALSE (§3 does not list the repair line); the capability gap + the registration drift are S-050's two defects |
| Cloud / licensing / languages / runtime families | `artifacts/Cloud/`, `Licensing/` (beyond retention), `Languages/`, `runtime-families/` | Cloud pairing, activation, localization, runtime DLL census | OUT-OF-SCOPE (registered exclusions) |
| per-binary identity cards (114) | `artifacts/per-binary/` (112/114 never-cited; libvmdk ×1 via modules/mom-capabilities.md:40 + deepening-batch1 ×1 via M-064 — the "113/114" row was the N-101/N-120 arithmetic drift, corrected) | Per-binary RTTI/import/strings censuses | Reference material — consulted per-surface above; not individually auditable surfaces |

## 6. The corpus's installer-behavior parameters no finding owns (the §5 app-installer ground truth, enumerated)

From the corpus (A2's extraction, this session): (1) Inno Setup + bootstrap pair; (2) /SILENT; (3) /VERYSILENT; (4) /SUPPRESSMSGBOXES; (5) /NORESTART(+Applications); (6) ARP `UninstallString` + `QuietUninstallString`; (7) unins%03d multi-uninstallers; (8) Inno component/task/user-info-serial metadata; (9) admin-required uninstaller; (10) SetupUE 16-entry OS gate (Win2000→Server 2016, WOW64-aware); (11) `SOFTWARE\EASEUS\DRW\Version` registry contract; (12) install telemetry (/SendInfo fields); (13) uninstall telemetry + retention triggers; (14) setupdrvx64 `-Uninstall`; (15) `%s%d` versioned driver coexistence; (16) Services write+verify + per-call SCM errors; (17) DRWService `/install`//`/uninstall`; (18) AUTO_START LocalSystem "Read FVE Server"; (19) EuMrx install.bat pattern; (20) EuMrx uninstall.bat (+ its own bug); (21) winpeshl launch order; (22) regdrv.reg UpperFilters + Start-matrix + eprdtdrv-absence; (23) auto.bat OEM branch; (24) USB companion deployment; (25) software.reg + Unattend display floor; (26) the WAIK/DISM/IMAPI2 media builder; (27) installer stamping of version.ini (SetupBuildTime, xurlPageID).

## 7. Session wave order (corpus-determined: highest integration-risk / user-impact first)

1. **Carve families** (23+47+61 analyser classes vs the core's grammar set) — carve results are user-facing; the core's capability row claims rest on the census, not the family semantics.
2. **Session field schemas** (`ifileproperties-part1` NTFS/FAT/exFAT vs the crsf round-trip).
3. **Installer surface** (§5/§6 — extends the Phase-1 findings into the app-installer lane).
4. **Preview subsystem** (UIUX/10 vs modules/preview.md).
5. **User signature DB grammar** — **AUDITED this session** (S-039 caps reject the og's own DB; M-065 no default set; OQ-SA-3 consumption-mode probe) · **Preview subsystem** — **AUDITED this session** (M-066 archive enumeration; N-088 M-028 propagation tail; the caps/grading/socket cells already I-015/S-023)
6. **WinPE payload census** — **DISPOSITIONED this session** (the PE capability boundary — zero .mo = no lost-partition search in the bootable edition, FREE-by-construction, installer stamps version.ini — recorded as the design input for the roadmap's boot-media builder + S-038's K78 scope; no defect in shipped code; the roadmap already lists the builder at docs/roadmap.md:71)
7. **MOM stubs spot-check** — **DISPOSITIONED this session** (zero new parameters; the SURFACE-INVENTORY count corrected 22→17; decode candidates noted)
8. **CORRECTIONS #5 propagation** (M-024's 37→34 toggles across the trail).
(S7 lane, parallel: the driver-only-era wire/IOCTL/taxonomy audit + the two NEVER-CITED driver docs flagged to it in §1.)

## 8. Coverage arithmetic (this inventory)

282 docs inventoried (283 incl. CORRECTIONS.md meta) · **~95 cited (90
artifacts + 5 architecture — corrected session-8 wave-2, N-101: brace-group,
alias and short-stem citation forms were not folded into the original 78)**
· ~187 never-cited, of which: OUT-OF-SCOPE ≈ 164 (per-binary **112** —
deepening-batch1 is cited by M-064; runtime-families 10, FileRepair 8, UI-pure
UIUX ~23, MOM stubs 17 rollup-covered, Languages 2, EuShare **2 never-cited**
(exclusion stands on architecture/05:124 + DRWUI-exe--main-ui:214-15 +
rtti_classes.json, not on the EuShare deep-dives), Cloud/Licensing remainder,
launcher/crash-reporter) · S7-flagged 2 · IN-SCOPE NEVER/PARTIAL ≈ 50 (the
§2–§6 rows; waves 1-8 landed on them). **Session-8 registrations (both lanes):
the deep-audit lane's I-027 owns the data-asset register (DataFile.ini row 67,
DRWReport.ini row 73, Upgrade.ini row 78, auto.txt → N-097); the wave-2 lane
adds the Eaolog.log 4th-log-channel note (extends N-048's census) and the
log\BuildRaw.log + log\DelFat.log engine run-log channels (telemetry-equivalent,
OUT). GetDriverInfo.dll gets its own row below (N-106).**

## 9. Session-8 currency update (2026-10-28 — the second-session deep audit)

- **Corpus commit:** the inventory's standing cite was `03ad46c`; the corpus is
  now **`be0cfe9`** (one commit: CORRECTIONS #9 — EUUserData → CHKRepairFileSign;
  GetRawUserInfoMap = per-scan stats map). **Absorption verified first-hand** —
  CORPUS-ERRATA E-A/E-B, PARITY-DELTAS D-104, and SCAN-ENGINE-DELTA F2 already
  carry it; no inventory row depends on the superseded reading.
- **The two §1 NEVER-CITED driver docs** (epmntdrv-internals-v2; trio naming/PID
  handshake) — **READ IN FULL this session** (the session-8 E lane): the
  user-mode half is MATCH throughout (IOCTL constants, in/out gates, grammar,
  PID-token both-sides-exact, 200-disk cap, failure mapping); the residuals are
  **N-096** (identity-IOCTL non-issuance), two DRIVER-REPO-ITEM handoffs
  (READ-length semantics nuance; the disk.sys-comparison field reading — the
  rda-driver lane's items), and the transport.md grammar misquote (**N-095**).
  §1's "flagged to S7" posture is discharged for the corpus-half audit; the
  driver-repo halves remain that lane's.
- **The §4 MOM decode candidates** — CWindowsFS decoded (**OQ-S8-1**, the
  consumer question); the DeviceManager offline-OS strings decoded (**M-082** —
  live-module machinery, the mom-capabilities disposition re-scoped).
- **The og-codebase data assets** — the never-audited set swept (session-8 A
  lane): every row maps to a register/exclusion; the five decoded ground-truth
  contracts are **I-027**; auto.txt's misclassification is **N-097**.

---

## §9. Session-8 third-lane corrections (2026-10-28 — coverage-sweep verification; extends, never rewrites, the rows above)

A full citation re-sweep this lane (every one of the 274 corpus artifacts + 9
architecture docs, basenames grepped against the entire trail) verified the
inventory's claims post-waves. Corrections and confirmations:

- **Coverage verdict after all waves: CLEAN.** Zero in-scope corpus artifacts
  document engine-side parameters the trail never dispositioned (the
  never-cited residue is per-binary reference material, registered OOS
  families, UI-pure UIUX, and rollup-covered MOM stubs — spot-reads confirm
  each). The §8 arithmetic was directionally right, numerically fuzzy; the
  continuation lane's N-101 owns the refreshed counts.
- **§1/§5 DRWService contradiction — the §5 row's "folded into S-038's
  packaging contract" claim has NO trail backing** (grep for the service
  facts `/install` / AUTO_START / "Read FVE Server" / LocalSystem across the
  trail: only this inventory and the PARAMETERS Group-K derivation list hit
  them; S-038 contains no DRWService-service content). §1's PARTIAL/NEVER-
  AUDITED status for the install lifecycle is the truthful one; the §5 row is
  corrected to read: "install lifecycle NEVER-AUDITED as a dedicated finding —
  the facts live only in PARAMETERS Group K's derivation sources; a future
  packaging session may fold them into the K71-K78 walk."
- **§1 EuShare parenthetical corrected**: "(5 artifacts, cited)" is false —
  EuShare/ ships 2 .md artifacts, both never-cited (the NAS OOS registration
  itself stands, via I-010/D-26 + the install.bat/uninstall.bat rows in §5/§6).
- **§3 glob names corrected**: `rawsearchdll--raw-signs*` and
  `rawsearchdll--family-layer*.md` match no corpus files — the intended
  citations are `raw-dual-registry--u16-magic-keys.md` (the "×13" count) and
  `rawsearchdll--analyser-registry-census.md` (the census); the "family-layer"
  citation resolves to the trail's own evidence file
  `evidence/decompiled-quotes/s6-carve-family-layer.md`.
- **MOM stub count corrected**: 17 → **16** never-cited in any form (21 by
  strict basename, minus the 5 brace-cited: Common, Config, CheckVersion,
  bitlockerhelp, FileSystemCheck). All 16 rollup-covered per the wave-7
  precedent (mo-module-map.md ×16 + modules/mom-capabilities.md rows).
- **§1 hook-pair row softened**: the EUDCPEPM/EUEDKEPM row's "AUDITED"
  overstates — both named artifacts are never-cited; the behavior is
  dispositioned via API-DELTA/I-010/N-041/transport.md, and the urgent-cache/
  VolSnap/DrvHK semantics rest with the S7 driver lane (unchanged flag).
- **Citation-state refresh**: SearchDLL 30/30 cited; SaveScan 5/5;
  config-files 6/6; per-binary now 3/114 (M-064 added two); part1 ×3; corpus
  commit at this lane: `be0cfe9`.
- **New surfaces this session's lanes registered** (rows above unchanged):
  the driver-install findings M-086..M-089 + N-107 (this lane) extend the §1
  driver rows; the WMF/batch-2 carve rows (S-042/S-041) and the thumbnail/
  session rows (S-043..S-045, M-080/M-083..M-085, N-092/N-102..N-105) belong
  to the parallel session-8 legs and are mirrored in CROSS-REF-MATRIX.

---

## §11. Session-10 lane-B currency update (2026-10-29 — the continuation deep audit)

- **Corpus commit:** `be0cfe9` (unmoved).
- **The full-tree completeness sweep (1470 files / 167 top-level entries vs the
  PARITY-OG-CORE-INVENTORY's 112 rows):** 1468/1470 dispositioned. The two exceptions
  are now owned: **VideoViewer/VideoFileRepairer.exe** (the Fixo video-repair console
  engine — no row; **N-130** registers it SIDECAR-OOS per the I-010/D-26
  repair-family precedent; the engine-side touchpoint remains the M-021
  CarvePostProcessor hook) and **VideoViewer/sfc** (211,614 B opaque " cfs-"/FAN.
  payload — no consumer string, no corpus characterization; **tracked-open
  OQ-S10-2**, the VideoFileRepairer Ghidra magic-search queued). The sweep's six
  register count/cite corrections are **N-129** (api-ms 44→40 · CORE_RL 19→24 · .mo
  25→30 — all 30 .mo rows exist, the count was wrong · the DRWReport cite :84→:97 ·
  the EuOfficeTools casing · the stale self-nested row).
- **The config-layer rows (§6-adjacent):** the FilterFolder.ini row's TxtViewFiles
  half and the PriorityDisplayType/SumaryFileType surfaces — claimed "landed" by
  API-DELTA §1.7 rows 44-45, absent in the tree (grep record:
  `evidence/decompiled-quotes/s10-config-layer-absence-greps.md`) — are owned by
  **N-132** (registration) + **M-108** (the preview behavior, EXECUTED). The
  junk-filter pools themselves re-verified byte-exact this session (25/25 entries).
- **Executed ground-truth added:** the three real SaveScan .rsf files parsed
  first-hand — the documented container grammar byte-confirmed on real artifacts
  (magic GUID 1BB3F4EA-4305-4AB6-B4B8-244A33D913BB, 0x8C file header, 0x24 record
  header, 4 KiB chunked zlib; `evidence/executions/s10-rsf-ground-truth.md`).

---

## §11. Session-10 S10-F currency (2026-10-29 — the function-level completeness pass; corpus `be0cfe9`, core `44f043a5`)

The mandate's "mark all remaining features, implementations, functions as
finding too" taken to the corpus's own function inventories: the nine engine
`*_functions.txt` lists parsed (9,618 functions / 3,088 named), the class
surface mapped to trail citations (191 classes cited by name / 49 never —
the 49 semantically dispositioned one-by-one, all COVERED except the two
behavioral gaps filed below), and the nine ghidra trees' 103 decompiled
binaries censused for disposition. Findings: **M-110** (the og's Btrfs
`@eaDir` descent suppression — a NEW decode, undocumented in the corpus's
own artifacts), **N-133** (engine-side volume labels — the ext4 decode is
dead code; M-079's premise corrected), **N-134** (the six never-dispositioned
product-frame binaries + the telemetry name-absent set — the census's only
residue; the engine-bearing trees re-confirmed CLEAN at binary level).
Board hygiene: the M-108 filename collision repaired (M-108 preview-text
keeps the ID per the SA-9 rule; the M-080-residual renumbered M-109).
Evidence: `evidence/decompiled-quotes/s10f-*.md`.

---

## §12. Session-12 continuation currency update (2026-10-29 — the second-session continuation deep audit; audited core `c7fa527c`, corpus `be0cfe9`)

- **The continuation mandate's three waves executed:** (1) remove-the-fixed — the
  55-finding liveness sweep, all defect sites re-verified at HEAD (41 LIVE / 4
  PARTIAL / 5 driver-repo / 5 register; **no unretired fix** — the inventory's
  coverage claims hold); (2) go-deeper — the below-artifact walks (core module
  deep-read: all 28 modules dispositioned-in-substance, the four missing module-row
  files recorded as N-143's item 3; corpus behavioral re-walk: DiskHealth/FileRepair/
  SaveScan/MOM/config-files + the runtime logs — 8 NOT-DISPOSITIONED claims found,
  all now filed or registered); (3) mark-all-remaining — every surfaced gap filed.
- **New rows this lane owns:** the register-channel surface (the og's MountPoints2\
  CPC\Volume candidate source — **M-112**; was invisible to every prior inventory
  because no artifact documents it — the runtime log + the engine-side decompile
  were the only witnesses); the partition-boundary read semantics (**M-113**; the
  CBasicPartitionDevice clamp decode + the real-log observation); the variant-save
  device-descriptor record (**N-142**; the rsf artifact's own parenthetical, now
  owned); the N-013 regression (**S-054**; a landed-fix reversion, not a corpus
  surface — the board-truth class); the register batch (**I-033**: RegLib's write
  half, DiskHealth's resident shell, the Explorer verb) and the bookkeeping batch
  (**N-143**: the N-079 collision, the salvage register, module rows, the suite
  count).
- **S-050's row extended by addendum:** the fix plan's build order gains the video
  engine + the eligibility-signature seam + the output-location contract (the
  repair-architecture row's deepest-decoded algorithm was unowned by the plan).
- **Corpus erratum candidate for the next batch (I-033's item 4):** the DiskHealth
  artifact's auto.bat `trayicon` attribution (changeres.exe per boot-config:87).
