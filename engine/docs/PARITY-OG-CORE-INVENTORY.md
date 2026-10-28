# PARITY-OG-CORE-INVENTORY — og-codebase → core_recovery classification (Agent 3)

> **Gate artifact.** Per-entry classification of the shipped og installer payload
> (og-codebase, mirrored in `easeus-drw-research/og-codebase/`) against
> core_recovery. Trees are inputs, not verdicts: every entry is classified before
> it may become an implementation task. No implementation without a row.
>
> **Source of truth:** `easeus-drw-research` (architecture docs + RE artifacts).
> **Target:** `core_recovery` (rda-driver = the unified driver; NAS / Cloud
> Recovery excluded per the unified-driver exclusion — cloud_session.dll,
> EuICloudRecovery.dll, icloud_logger, EuShare/EuMrx.sys, NasConfig/,
> nasconfig.ini and equivalents are never reintroduced).
>
> **Owner:** Agent 3 — modules, loaders, format handlers, repair/rebuild
> pipelines, sidecar services. Agent 1 owns API contracts; Agent 2 owns
> quick/advanced scan pipeline semantics. Handoffs live in `worklog.md`.
>
> Created: 2026-10-28 · Status conventions: `TRIAGE` (verification pending),
> `VERIFIED` (parity evidenced), `GAP` (open delta, finding-linked), `TODO` /
> `WIP` / `DONE` (implementation), `OOS-*` (out of scope, rationale given),
> `DEAD` (evidence says do not implement).

UI/UX layer: owned by the downstream project, not in scope — covers DRWUI.exe and UILogic.dll, the viewer/editor UI processes (PDFViewer.exe, EUOfficeViewer.exe, EUHwpViewer.exe, EUOfficeTools.exe, EuOfficeRepairWin32.exe, EUHwpFilePathCheckerModule.dll / EUHWP*, EUinApp.exe, EaseUS_VideoViewer.exe and VideoViewer/ UI assets, DiskHealth.exe app payload incl. diskhealth/ QML/Qt/bat scaffolding), all Qt5/QML/platforms/imageformats/iconengines/styles/scenegraph/bearer/qmltooling/Qt/QtQuick payloads, language/*.qm + translations/ + Japanese.dll, Resource/ + ResourceDark/ + style.css + themdark.ini + PriorityDisplayType.xml + shellexe.ini + ImageResource.xml + all *.png/*.bmp/*.ico/*.cur, BUILDPE/ + x64/Windows/** + SetupUE.exe + InfoForSetup.exe + unins000.* + Upgrade.ini + *.reg + auto.txt/needcopy.txt/install.bat/auto.bat/querytask.bat/deltask.bat/addtask.bat, and UI snapshot / vlm-evidence references.

## Legend

| Class | Meaning | Action |
|---|---|---|
| FEATURE | A real capability implemented by the entry | Implement in core_recovery against the research spec |
| WRAPPER | Packaging shim around a capability core_recovery already has | Verify parity; `VERIFIED` or open a gap |
| SIDECAR | Standalone engine/helper process with a functional role | Implement only if research defines it in-scope; else `SIDECAR-OOS` with rationale |
| RUNTIME | CRTs, Qt, OpenSSL, zlib, codecs, api-ms-win-*, 7za etc. | Skip — packaging concern |
| INSTALLER | Setup/update/uninstall/PE payload | Skip |
| EXCLUDED-NAS | NAS/Cloud per the unified-driver exclusion | Skip — never implement |
| REPORTING | Crash reporting / telemetry / online config | Triage; opt-in sidecar only if research proves in-scope |
| TRIAGE-* | Ambiguous row pending corpus verification | Resolve via research before any action |

---

## A. og top-level entries (non-UI)

| Entry | Class | Rationale (evidence) | Target in core_recovery | Status |
|---|---|---|---|---|
| 7za.exe | RUNTIME | 7-Zip CLI extractor; dep of the repair/installer paths, not an engine feature | none | skip |
| AliyunConfig.ini | REPORTING | Aliyun OSS upload config for the crash-report chain (`DRWReport-exe--crash-reporter.md`) | none | TRIAGE-3C |
| AliyunWrap.dll / AliyunWrapExe.exe | REPORTING | Aliyun upload wrapper used by DRW.exe/DRWReport telemetry (`DRW-exe--launcher.md`, `DRWReport-exe--crash-reporter.md`) | none | TRIAGE-3C |
| api-ms-win-* (44 DLLs) | RUNTIME | UCRT API-set forwarders (`runtime-families/ucrt-forwarders.md`) | none | skip |
| archive.dll | FEATURE-TRIAGE | Archive read/extract library (libarchive per og `License/`); pairs with the og's archive preview browsing (M-066) and EUArchiveRepair | `cr_preview` zip branch; carve inner entries (S-032 FIXED) | TRIAGE-3B |
| auto.txt | INSTALLER | WinPE auto.bat payload (`WinPE/boot-config--winpeshl-autobat-regdrv.md`) | none | skip |
| *SearchDLL.dll (Btrfs/EXT4/FAT/HFSPlus/NTFS/RAW/ReFS/Zfs) | WRAPPER | Per-FS engine plugins, the SearchDLL family contract (`SearchDLL/searchdll-family--architecture.md`) | `cr_*_engine` + `cr_carve` | TRIAGE-3F (verify same surface, no regression from the module split) |
| BUILDPE/ | INSTALLER | WinPE boot payload; census proved 0 .mo / 0 .rsf in PE (`WinPE/pe-payload-census--buildpe-easeus-x64.md`); runtime/config entries are duplicates of the canonical top-level ones | none | skip |
| cloud_session.dll | EXCLUDED-NAS | Cloud recovery OAuth client (`Cloud/cloud-recovery-architecture.md`) | none | skip |
| concrt140.dll | RUNTIME | VC CRT concurrency runtime | none | skip |
| config/config.ini | WRAPPER-VERIFY | Engine config channel (`drwcfg` persistent config; I-023 FIXED) | config surface (I-023) | TRIAGE-3E |
| config/FilterFolder.ini | WRAPPER-VERIFIED | Junk-folder filter (M-018 FIXED; `cr_filterfolder` + `gen_filterfolder.py`) | `cr_filterfolder` | VERIFIED |
| config/nasconfig.ini | EXCLUDED-NAS | NAS config — unified-driver exclusion | none | skip |
| config/RawConfig.ini | WRAPPER-VERIFIED | Runtime per-format carve switches (M-024 FIXED; `filtering-priority--config-layer.md`) | carve filter surface | VERIFIED |
| config/usefullostfiles.xml | FEATURE-TRIAGE | Filtering-pipeline config (`filtering-priority--config-layer.md`); functional only if research proves it drives recovery filtering | filter model | TRIAGE-3E |
| config/version.ini | RUNTIME | Build-pair state file (PE census) | none | skip |
| cpprest141_2_10.dll | RUNTIME | cpprest SDK — dep of the excluded cloud/telemetry paths | none | skip |
| DataFile.ini | WRAPPER-VERIFY | Generator-set / DAT-file persistence posture (s25-oq-s6-1; I-024 FIXED) | `cr_records` | TRIAGE-3E |
| dbghelp.dll | RUNTIME | Crash-dump helper — REPORTING family dep | none | skip |
| de265.dll | RUNTIME | libde265 HEVC codec (`runtime-families/media-codecs.md`) — HEIC decode dep | none (HEIC thumbnails = phased follow-up) | skip |
| Dict | WRAPPER-VERIFIED | Brute-force password dictionary for encrypted-volume recovery (N-064 FIXED; `cr_dict` + `cr_dict_data` + `gen_dict.py`) | `cr_dict` | VERIFIED |
| dlconfig.xml | FEATURE-TRIAGE | Download config; corpus cites it for the WinPE WAIK ISO carve (arch doc 03) — verify functional vs UI-adjacent | none | TRIAGE-3E |
| DRW.exe | REPORTING | DRWLoader watchdog/supervisor: crash events, 2 MiB shared log, supervisor restart loop, DRWReport handoff (`DRW-exe--launcher.md`) | none (core is engine + CLI; the host is downstream) | TRIAGE-3C |
| DRWReport.exe + DRWReport.ini | REPORTING | XCrashRreport crash reporter: minidump + telemetry + Aliyun upload (`DRWReport-exe--crash-reporter.md`) | none | TRIAGE-3C |
| DRWService.exe | SIDECAR | "Read FVE Server": BitLocker auto-unlock key server, pipe wire 0x138 stride (`DRWService-exe--bitlocker-service.md`); the ENGINE-side capability (offline-hive FVEAutoUnlock harvest) is already `cr_bitlocker` (S-014 FIXED) — the live pipe server is a host deployment concern | `cr_bitlocker` / `cr_reghive` | TRIAGE-3C |
| Eaolog.log / EPMLOG.log / reglog_win.txt / strutureReg.log / log/ | RUNTIME-ARTIFACT | Installed-product runtime logs (qa-logs session forensics) — not features | none | skip |
| EUAdobeRepair.exe | SIDECAR-TRIAGE | Fixo PDF/Adobe repair engine (`FileRepair/repair-architecture.md`) | none | TRIAGE-3B |
| EUArchiveRepair.exe | SIDECAR-TRIAGE | Archive repair engine, C7zNextHeaderInfo footer repair (`FileRepair/archive-repair--7z-engine.md`) | none | TRIAGE-3B |
| EuDownloader.exe | INSTALLER | Update-channel downloader; the shipped update path is dead (s25-oq-s5-1 CheckVersion evidence) | none | skip (dead family, see AutoUpdate row) |
| EUFileRepair/ (bundle) | SIDECAR-TRIAGE | The Fixo repair bundle: EUImgRepairFile.exe / EuPDFRepairFile.exe / EuOfficeRepairFile.exe + Magick++ CORE_RL_* / IM_MOD_RL_* / heif / libde265 / 7z.dll runtimes (`FileRepair/*`) | none | TRIAGE-3B |
| EUImageEnhance.dll | FEATURE-TRIAGE | Image enhancement (OpenCV per og `License/`); decode/enhance path may belong to the preview/thumbnail pipeline | none / `cr_thumbnail` | TRIAGE-3B/3D |
| EUImgRepair.exe | SIDECAR-TRIAGE | Image repair front-end (`FileRepair/image-repair--format-grammar.md`) | none | TRIAGE-3B |
| EuRepairHost.exe | SIDECAR-TRIAGE | Repair host process (process topology in `FileRepair/repair-architecture.md`) | none | TRIAGE-3B |
| EUUnZip.exe | SIDECAR-TRIAGE | Archive extraction sidecar | none | TRIAGE-3B |
| EUUserData | WRAPPER-VERIFIED | User-taught signature DB (111 entries / 190 signs; M-065 FIXED — `data/userdata-defaults.json` ships + engine-init load; OQ-SA-3 RESOLVED) | `cr_usersigns` + `data/userdata-defaults.json` | VERIFIED |
| EuUserRate.dll | FEATURE-TRIAGE | Rate/feedback surface; in scope ONLY if research shows it persists recovery state the engine reads | none | TRIAGE-3C |
| EUVideoRepair.dll | SIDECAR-TRIAGE | DRW-world video repair engine (`FileRepair/repair-architecture.md`) | none | TRIAGE-3B |
| FileFilter.xml | WRAPPER-VERIFIED | 8-category / 1489-extension filter model (M-023 + M-035 FIXED; `cr_filefilter_data` + `gen_filefilter.py`) | `cr_query` filter model | VERIFIED |
| firebasefetch.exe / firebaseonlineconfig.dll / google-services.json / remote_config_data | REPORTING-TRIAGE | Firebase online-config/crash chain (`Cloud/cloud-recovery-architecture.md` — remote config) | none | TRIAGE-3C |
| GDIPLUS.DLL | RUNTIME | GDI+ imaging (UI + UI-side thumbnail decode dep) | none (engine-side decode is our own `cr_thumbnail`) | skip |
| heif.dll | RUNTIME | libheif HEIC decode — dep of HEIC preview/repair | none (HEIC thumbnails = phased follow-up) | skip |
| Japanese.dll | UI (localization) | Language resource DLL | none | covered by the UI exclusion line |
| legacy.dll | RUNTIME-TRIAGE | Compat shim; verify role (`runtime-families/misc-windows-shims.md`) | none | TRIAGE-3C |
| libcrypto-* / libssl-* / libeay32* / ssleay32* / libssl32.dll | RUNTIME | OpenSSL trio (`runtime-families/openssl-crypto-trio.md`) | none | skip |
| liblzma.dll / libssh2.dll / libx265.dll / libzmq-mt-4_3_5.dll / zlib.dll / zlib1.dll | RUNTIME | Compression / SSH(NAS dep) / x265 codec / ZeroMQ (MediaGraph IPC) / zlib (`runtime-families/*`) | none | skip |
| License/ | PACKAGING | Third-party license registry — the convention our dependency policy mirrors when adding deps | none (convention source) | skip |
| Microsoft.VC90.*.manifest / msvc*90* / msvcp120 / msvcr120 / msvcp140 / msvcr90 / msvc m90 / ucrtbase.dll / vcruntime140.dll | RUNTIME | VC runtime generations (`runtime-families/vc-runtime-generations.md`) | none | skip |
| NasConfig/ + EuShare/ + icloud_logger | EXCLUDED-NAS | NAS redirector + iCloud logging (`EuShare/eumrx-nas-architecture.md`, `Cloud/cloud-recovery-architecture.md`) | none | skip |
| needcopy.txt | INSTALLER | File-copy manifest | none | skip |
| og-codebase/ (dir) | RESEARCH-COPY | The corpus's own source snapshot inside the installer tree — not a runtime feature | none | skip |
| pdfium.dll | RUNTIME | PDF rendering (PDFViewer dep; pdfium per og `License/`) | none (PDF page thumbnails = phased follow-up if mandated) | TRIAGE-3D |
| PE_Tools.dll + PEtools.xml | FEATURE-TRIAGE | PE format tool surface — verify what consumes it | none | TRIAGE-3E |
| PubLog.dll | WRAPPER-VERIFY | Public logging library (MOM dependency matrix: Log family, 15+ importers) | `cr_log` | TRIAGE-3A |
| raw_signs.json | WRAPPER-GAP | 139-extension header/tail signature table, loaded at engine init by RAWSearchDLL (deep-dive :54-59); the core compiles it in — open posture divergence | `cr_rawsigns` (+ file-load posture) | GAP-N086 (open MINOR) |
| recoveryFree.ini | FEATURE-TRIAGE | Free-edition gating config — likely licensing/packaging, verify | none | TRIAGE-3C |
| SaveScan/*.rsf | WRAPPER-VERIFIED | Scan-session persistence, format FULLY REVERSED (`SaveScan/rsf-format--scan-session.md`); core owns `cr_crsf` + `docs/crsf-format.md` (the samples themselves are runtime artifacts) | `cr_crsf` | VERIFIED |
| send_dmp_with_aliyun.exe | REPORTING-TRIAGE | Crash-dump uploader (DRWReport chain) | none | TRIAGE-3C |
| EaseUS_MediaGraph.dll + VideoViewer/ffmpeg.exe + avcodec-62/avformat-62/avutil-60/... | FEATURE (TRIAGE-UI-AMBIGUOUS) | The og's media decode path (zmq MediaGraph + ffmpeg); per the session-owner directive the DECODE/THUMBNAIL path is engine-side FEATURE (see Section D) — the viewer UI itself stays under the UI exclusion | `cr_thumbnail` (new) | TRIAGE-3D → FEATURE |
| EURawImg.exe | SIDECAR-TRIAGE | RAW image processor (LibRaw static link; 21 RAW formats per repair-chain) | none (RAW thumbnails = phased follow-up) | TRIAGE-3B/3D |
| EuSendMessage.exe | SIDECAR-TRIAGE | Message/elevation helper (PE-only per the PE census) | none | TRIAGE-3C |
| EuICloudRecovery.dll | EXCLUDED-NAS | iCloud client (`esicloud`) | none | skip |

## B. PartitionRecovery/* modules (the og engine runtime library surface)

| Entry | Class | Rationale (evidence) | Target in core_recovery | Status |
|---|---|---|---|---|
| AutoUpdate.dll | WRAPPER-DEAD | Auto-update; the shipped update path is dead (s25-oq-s5-1: CheckVersion consumer dead in the shipped build) | none | DEAD — do not implement (pending 3C confirmation) |
| bitlockerhelp.mo | WRAPPER-VERIFY | BitLocker assist module (mo-module-map) | `cr_bitlocker` + `cr_decrypt_device` | TRIAGE-3A |
| BitLockerLib.dll | WRAPPER-VERIFY | BitLocker FVE library | `cr_bitlocker` (S-014, M-033, OQ-S20-1 lineage) | TRIAGE-3A |
| Boot.dll | FEATURE | Boot-chain repair, Fixup family (BCD offset rewriter) | `cr_bcdfix` + `cr_bootfix` | DONE-M040 (verify this session) |
| BootDriver.dll | WRAPPER-VERIFY | Boot driver install surface (eprdtdrv deployment) — likely driver packaging | driver-install docs / rda-driver | TRIAGE-3A |
| Burn.dll | SIDECAR-TRIAGE | Optical disc burn (recovery media authoring) | none | TRIAGE-3B |
| CallbackOperator.mo | WRAPPER-VERIFY | Scan callback operator (event/progress surface) | `cr_scan` orchestrator (N-016/N-017 FIXED) | TRIAGE-3F |
| CheckVersion.mo | WRAPPER-DEAD | Version check — dead in the shipped build (s25-oq-s5-1 evidence) | none | DEAD — do not implement (pending 3C confirmation) |
| CloneModule.dll | FEATURE-TRIAGE | Disk/partition clone; sole importer of Win32Bcd's media authoring (M-040 corpus refs) | none | TRIAGE-3A/3B (research scope decision) |
| Common.mo | WRAPPER-VERIFY | Common runtime types | `cr_types` / `cr_error` | TRIAGE-3F |
| Config.mo | WRAPPER-VERIFY | Engine config module | config surface (I-023 FIXED) | TRIAGE-3F |
| Device.mo / DeviceAdapter.mo / DeviceManager.mo | WRAPPER-VERIFY | Device enumeration/manager layer | `cr_device` / `cr_devmap` / `cr_enumerate` (N-071 FIXED) | TRIAGE-3F |
| EFIBoot.mo | FEATURE | UEFI NVRAM Boot####/BootOrder updater | `cr_nvramfix` + `cr_bootfix` | DONE-M040 (verify this session) |
| eprdtdrv.sys / eprdtdrv_win7older.sys | RUNTIME-DRIVER | The RDA driver binaries — driver packaging, the rda-driver repo owns the driver | rda-driver | skip |
| Error.mo | WRAPPER-VERIFY | Error model module | `cr_errmod` (M-049 FIXED) | TRIAGE-3F |
| ExFatFileSystemAnalyser.mo | WRAPPER-VERIFY | exFAT analyser | `cr_exfat_engine` (+ wire/dentry layers) | TRIAGE-3F |
| ExtFileSystemAnalyser.mo | WRAPPER-VERIFY | ext2/3/4 analyser | `cr_ext4_engine` | TRIAGE-3F |
| FATFileSystemAnalyser.mo | WRAPPER-VERIFY | FAT analyser | `cr_fat_engine` (+ chain/lfn) | TRIAGE-3F |
| FatFormat.mo | WRAPPER-VERIFY | FAT format/boot-gate helpers | `cr_fat_wire` / FAT boot gates (M-045 FIXED) | TRIAGE-3F |
| FatLib.mo | WRAPPER-VERIFY | FAT chain/LFN library | `cr_chain` / `cr_lfn` | TRIAGE-3F |
| FileSystem.mo | WRAPPER-VERIFY | Filesystem interface layer | engine factory contract (I-016 FIXED) | TRIAGE-3F |
| FileSystemAnalyser.mo | WRAPPER-VERIFY | Analyser registry | `cr_analysers` (N-033 FIXED) | TRIAGE-3F |
| FileSystemCheck.mo | WRAPPER-VERIFY | Filesystem check phase | `cr_fscheck` (M-041 FIXED) | TRIAGE-3F |
| Fixup.mo | FEATURE | Post-operation fixups (the BCD/boot offset repair library; M-040's primary corpus source) | `cr_bcdfix` / `cr_bootfix` | DONE-M040 (verify this session) |
| GetDriverInfo.dll | WRAPPER-VERIFY | Driver info client | `cr_driver_status` + the rda client contract (s4 evidence) | TRIAGE-3A |
| LdmManager.mo | WRAPPER-VERIFY | LDM dynamic-disk manager | `cr_ldm` (A-001/A-002, I-008 FIXED) | TRIAGE-3F |
| libvmdk.dll | WRAPPER-VERIFY | VHD/VHDX/VMDK virtual-disk mount library | `cr_image` (S-030 FIXED; qemu vhd/vhdx/vmdk fixtures in tests) | TRIAGE-3F (coverage depth) |
| Log.mo | WRAPPER-VERIFY | Logging module | `cr_log` | TRIAGE-3F |
| Logger.dll | WRAPPER-VERIFY | Logger library | `cr_log` | TRIAGE-3F |
| lostpart.dat | WRAPPER-VERIFIED | Lost-partition persistence (I-024 FIXED; generator sets + DAT files) | `cr_records` | VERIFIED |
| MainModule.dll | WRAPPER-VERIFY | Module host + pending-operation engine (apply/undo chains) | `cr_scan` / `cr_apply` (S-029 FIXED; M-040 wiring) | TRIAGE-3A |
| Mom.mo | WRAPPER-VERIFY | MOM module manager (in-house COM loader; loading contract decoded) | engine factory/registry — the core has no MOM by design (I-016 deliberate) | TRIAGE-3A |
| NTFSFileSystemAnalyser.mo | WRAPPER-VERIFY | NTFS analyser | `cr_ntfs_engine` (mftwalk/attributes/runlist) | TRIAGE-3F |
| NTFSLib.mo | WRAPPER-VERIFY | NTFS library (310 exports) | `cr_ntfs/*` | TRIAGE-3F |
| NTFSUtil.mo | WRAPPER-VERIFY | NTFS utilities (2871 fns per MOM-modules artifact) | `cr_ntfs/*` | TRIAGE-3F |
| partinfo.dat | WRAPPER-VERIFIED | Partition-info persistence (I-024 FIXED) | `cr_records` | VERIFIED |
| Partition.mo | WRAPPER-VERIFY | Partition model | `cr_table_parse` / `cr_node` | TRIAGE-3F |
| PartitionRecovery.dll | WRAPPER-VERIFY | Engine aggregate host | core aggregate (orchestrator+engines) | TRIAGE-3A |
| PartitionRecoveryD.mo | WRAPPER-VERIFIED | Generator-set engine + DAT persistence (I-024 FIXED; s25-oq-s6-1) | `cr_records` | VERIFIED |
| peformat.dll | FEATURE-TRIAGE | PE format handler — verify role (per-binary artifacts) | none | TRIAGE-3E |
| Qt5Core.dll / zlib.dll / zlib1.dll | RUNTIME | Host/runtime deps inside the module dir | none | skip |
| RawFixer.dll | WRAPPER-VERIFIED | Transactional FS repair (NTFS/FAT/exFAT fixers, 31-method inventory) | `cr_rawfixer` (S-008/S-010/A-005/A-006/M-009 FIXED) | VERIFIED |
| RecoveryModule.dll | WRAPPER-VERIFY | Recovery host module (CRecoveryMgr chain) | orchestrator + `cr_safewriter` | TRIAGE-3A |
| RefsFileSystemAnalyser.mo | WRAPPER-VERIFY | ReFS analyser | `cr_refs_engine` (+ detect) | TRIAGE-3F |
| RegLib.mo | WRAPPER-VERIFY | Registry HIVE library | `cr_reghive` | TRIAGE-3F |
| setupdrv.ini / setupdrvx64.exe | INSTALLER | Driver installer for eprdtdrv | rda-driver deployer (M-064 remainder is driver-install domain) | skip |
| Thread.mo | WRAPPER-VERIFY | Threading module | `cr_parallel` | TRIAGE-3F |
| ToolsModule.dll | WRAPPER-VERIFY | Tools host (WinChkdsk/Win32Bcd/clone/burn hosts) | none (per-tool dispositions below) | TRIAGE-3A |
| Win32Bcd.dll | FEATURE | BCD authoring (bcdedit CLI wrapper; media BCD for WinPE/clone) | `cr_bcdfix` (BCD store surgery; media authoring rides the clone/burn disposition) | DONE-M040 for repair surface; clone scope TRIAGE-3A |
| WinChkdsk.exe | SIDECAR-OOS | chkdsk delegation host — M-040 stage (d) REGISTERED UI-LAYER: the core's contract ends at the undo record; the host owns process delegation (PARITY-DELTAS D-90) | none | DISPOSITIONED (D-90) |
| UILogic.dll | UI | UI logic module | none | covered by the UI exclusion line |

## C. FEATURE queue (sorted by criticality from research)

1. **Boot chain repair family (M-040)** — Boot.dll / BootDriver.dll / EFIBoot.mo /
   Fixup.mo / Win32Bcd.dll / eprdtdrv.sys → `cr_bcdfix` + `cr_nvramfix` +
   `cr_bootfix`. Expected DONE (finding FIXED, stages a+b landed; this session
   re-verifies code + tests and stamps the row).
2. **Thumbnail generation engine** — session-owner directive ("I think we need
   thumbnail generation"). The og generates thumbnails on the UI side (socket
   media-agent, 1 MiB BMP payload cap, I-015); the core's doctrine is in-process
   bounded readers, so the engine-side thumbnail capability is a deliberate,
   documented superset: bounded decode (cost-gated reads off `PreviewReader`),
   decompression-bomb guards, aspect-preserving downscale, honest
   unsupported/truncated reporting. Target: `include/cr/preview/cr_thumbnail.h`
   + `src/core/preview/cr_thumbnail.cpp`. TRIAGE-3D supplies the corpus decode
   matrix (imageformats plugins + GDI+ + heif + libraw + MediaGraph/ffmpeg) that
   sizes the format phases.
3. **M-066 preview archive enumeration** (open MAJOR) — the og's
   ArchivePreviewModel browses damaged zips at preview time; core's zip branch
   hunts only word/document.xml. Fix plan exists in the finding (bounded entry
   inventory, cap + truncated flag, 4 named tests). Engine-side; ABI exposure is
   an Agent 1 handoff.
4. **N-086 raw_signs file-load posture** (open MINOR) — og loads
   raw_signs.json at engine init; core compiles it in. Posture decision (a)
   register compiled-in as deliberate or (b) ship the data file + fallback.
5. Verify-rows that may open gaps: CloneModule (clone feature), Burn (optical),
   PE_Tools/peformat, libvmdk VMDK coverage depth, usefullostfiles.xml,
   dlconfig.xml, config.ini surface.

## D. Handoffs

- **handoff: Agent 1** — once the thumbnail engine lands, the cr_ui ABI surface
  for thumbnail retrieval (entry points, ownership, error codes) is Agent 1's
  contract to define; Agent 3 implements engine-side only.
- **handoff: Agent 1** — M-066's archive-entry Metadata extension needs an ABI
  surface decision when it lands (engine-side field first).
- **handoff: Agent 2** — none anticipated: no scan-output shape or scan-mode
  parameter changes in this inventory's FEATURE queue.

## E. Triage log (verification sub-agents 3-A..3-F)

| Row(s) | Verifier | Question |
|---|---|---|
| Boot family + M-040 DONE stamp | 3-A | code + tests + finding evidence reconcile? |
| Mom.mo / MainModule / RecoveryModule / ToolsModule / PartitionRecovery.dll / GetDriverInfo / PubLog / BootDriver / BitLockerLib / bitlockerhelp / CloneModule | 3-A | per-entry corpus role vs core module; clone in research scope? |
| File-repair family (EUFileRepair/, EU*Repair*, EuRepairHost, EUUnZip, archive.dll, Burn, EURawImg, EUImageEnhance) | 3-B | SIDECAR in-scope per research, or SIDECAR-OOS with rationale? |
| REPORTING family (firebase*, send_dmp_with_aliyun, AliyunWrap*, google-services.json, remote_config_data, AliyunConfig.ini, DRW.exe, DRWReport.exe, DRWService.exe, EuSendMessage, legacy.dll, recoveryFree.ini, EuUserRate, AutoUpdate/CheckVersion dead-confirm) | 3-C | in-scope opt-in sidecar vs OOS; dead-path confirmation |
| Thumbnail/decode pipeline (MediaGraph, ffmpeg, EUImageEnhance, heif, pdfium, libraw, imageformats, GDI+, 1 MiB BMP protocol, sizes) | 3-D | corpus decode matrix + protocol bounds that spec cr_thumbnail |
| Config/format surface (raw_signs.json N-086, PEtools/PE_Tools/peformat, DataFile.ini, dlconfig.xml, usefullostfiles.xml, config.ini) | 3-E | functional vs packaging; parity mapping |
| Per-FS .mo analysers + SearchDLL wrappers + libvmdk + FatFormat/NTFSUtil + persistence dats | 3-F | same-surface verification (analyser-registry, mom-capabilities, driver-client contract refs) |

## F. Appendix — full file enumeration (compact)

Every non-UI og-tree file maps to exactly one row above or one family below:

- **RUNTIME families:** api-ms-win-core-* / api-ms-win-crt-* (44+49+44 forwarder
  sets at root + BUILDPE + diskhealth + EUFileRepair + VideoViewer);
  concrt140/msvcp140/msvcp140_1/msvcr*/msvcp90/msvcm90/msvcp120/msvcr120/
  ucrtbase/vcruntime140 (+ .VC90 manifests, mfc90u/mfc140u in diskhealth);
  Qt5Core/Gui/Network/Svg/Widgets/WinExtras/Qml/QmlModels/QmlWorkerScript/Quick/
  QuickControls2/QuickShapes/QuickTemplates2/RemoteObjects; OpenSSL set
  (libcrypto-1_1/-3 x86+x64, libssl-1_1/-3, libeay32, libeay32MD, ssleay32,
  ssleay32MD, libssl32); liblzma/libssh2/libx265/libzmq-mt-4_3_5/zlib/zlib1/
  cpprest141_2_10; codecs de265/heif/libx265; pdfium; GDIPLUS; dbghelp;
  d3d8thk/d3d9/d3d11/dxgi (BUILDPE); CORE_RL_* (19) + IM_MOD_RL_* (7) +
  7z.dll + libcurl + postproc-58 (EUFileRepair bundle); avcodec-62/avdevice-62/
  avfilter-11/avformat-62/avutil-60/postproc-58/swresample-6/swscale-9 +
  ffmpeg.exe (VideoViewer bundle — decode path dispositioned with the
  thumbnail FEATURE row).
- **UI exclusion line covers:** every remaining file — see the single line in
  the header (viewers, Qt payloads, resources, locales, PE payload, installer).
- **INSTALLER:** SetupUE.exe, InfoForSetup.exe, unins000.{dat,exe,msg},
  Upgrade.ini, auto.txt, needcopy.txt, BUILDPE/ (incl. x64/Windows/**,
  regdrv.reg, software.reg, auto.bat, wallpaper.bmp, EUDCPEPM.sys,
  EUEDKEPM.sys, ebrntdrv.sys, epmdkdrv.sys, msi.dll, oledlg.dll,
  Unattend.xml, winpeshl.ini), diskhealth task bats, EuDownloader.exe,
  PartitionRecovery/setupdrv.{ini} + setupdrvx64.exe.
- **EXCLUDED-NAS:** cloud_session.dll, EuICloudRecovery.dll, icloud_logger,
  EuShare/ (EuMrx.sys + install/uninstall.bat), NasConfig/ (NasModel.xml,
  ssh_config.ini), config/nasconfig.ini.
