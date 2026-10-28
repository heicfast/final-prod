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
> quick/advanced scan pipeline semantics. Handoffs live in commit messages
> and this inventory (the repo worklog.md was retired 2026-10-28).
>
> Created: 2026-10-28 · **v2: every TRIAGE row resolved by the six verification
> sub-agents (3-A..3-F) against the corpus — see the Verification record.**
> **v2.1 (same day): re-landed on main after the parallel A2/API lanes moved
> the repo — M-066, N-086 (D-103), N-087, N-088 and M-067 (ABI v3) landed
> by those lanes; the queue below reflects that. Repo worklog.md was retired
> by the parallel lane — handoffs live in commit messages + this inventory.**
> Status conventions: `VERIFIED` (parity evidenced), `GAP` (open delta,
> finding-linked), `TODO` / `WIP` / `DONE` (implementation), `OOS-*` (out of
> scope, rationale given), `DEAD` (evidence says do not implement),
> `DISPOSITIONED` (registered exclusion in PARITY-DELTAS).

UI/UX layer: owned by the downstream project, not in scope — covers DRWUI.exe and UILogic.dll, the viewer/editor UI processes (PDFViewer.exe, EUOfficeViewer.exe, EUHwpViewer.exe, EuOfficeTools.exe (lower-case u — the file on disk), EuOfficeRepairWin32.exe, EUHwpFilePathCheckerModule.dll / EUHWP*, EUinApp.exe [orphaned IE-emulation bootstrapper — role-corrected in corpus], EaseUS_VideoViewer.exe and VideoViewer/ UI assets (incl. VideoFileRepairer.exe — the OOS repair child, row above), DiskHealth.exe app payload incl. diskhealth/ QML/Qt/bat scaffolding), all Qt5/QML/platforms/imageformats/iconengines/styles/scenegraph/bearer/qmltooling/Qt/QtQuick payloads, language/*.qm + translations/ + Japanese.dll (Win32 resource satellite), Resource/ + ResourceDark/ + style.css + themdark.ini + PriorityDisplayType.xml + shellexe.ini + usefullostfiles.xml (DRWUI-only display-tier whitelist — 3-E verified) + ImageResource.xml + all *.png/*.bmp/*.ico/*.cur, BUILDPE/ + x64/Windows/** + SetupUE.exe + InfoForSetup.exe + unins000.* + Upgrade.ini + *.reg + auto.txt/needcopy.txt/install.bat/auto.bat/querytask.bat/deltask.bat/addtask.bat (I-027 register note — the DiskHealth autostart datum: `diskhealth/addtask.bat` installs a logon scheduled task at HIGHEST for the companion app, `schtasks.exe /create /f /sc ONLOGON /rl HIGHEST`; host-concern — the core ships no autostart, the merged product's installer would own any equivalent decision), and UI snapshot / vlm-evidence references.

## Legend

| Class | Meaning | Action |
|---|---|---|
| FEATURE | A real capability implemented by the entry | Implement in core_recovery against the research spec |
| WRAPPER | Packaging shim around a capability core_recovery already has | Verify parity; `VERIFIED` or open a gap |
| SIDECAR | Standalone engine/helper process with a functional role | Implement only if research defines it in-scope; else `SIDECAR-OOS` with rationale |
| RUNTIME | CRTs, Qt, OpenSSL, zlib, codecs, api-ms-win-*, 7za etc. | Skip — packaging concern |
| INSTALLER | Setup/update/uninstall/PE-media-builder payload | Skip |
| EXCLUDED-NAS | NAS/Cloud per the unified-driver exclusion | Skip — never implement |
| REPORTING | Crash reporting / telemetry / online config | Triage; opt-in sidecar only if research proves in-scope |

---

## A. og top-level entries (non-UI)

| Entry | Class | Rationale (evidence) | Target in core_recovery | Status |
|---|---|---|---|---|
| 7za.exe | RUNTIME | 7-Zip CLI; dep of the repair/installer paths (bit7z backend of EUArchiveRepair per `archive-repair--7z-engine.md:12-13`) — family dispositioned OOS | none | skip |
| AliyunConfig.ini | REPORTING-OOS | Aliyun OSS upload config; consumers = the crash-report chain only; og ships `bAllowSendInfo=0` (3-C). I-027 register note — the batching caps (the spec for any host-side telemetry parity): `nMaxItemCount=1000`, `nMaxItemDays=7`, `nMaxIdleMinutes=5` (idle-flush), `nbLimitKeyvalueMaxLen=60000`, `nLimitPairsCount=20`, the `AllowSendUrl` gate, machine-SID uid; I-033 security datum: the file ships ENCRYPTED OSS credentials at the install root (`:3-8` LOGENDPOINT/ACCESSKEYID/ACCESSKEYSECRET/PROJECTNAME/LOGSTORENAME) — a host-deployment security datum beside N-097's plaintext auto.txt key | none | OOS (verified) |
| AliyunWrap.dll / AliyunWrapExe.exe | REPORTING-OOS | Product telemetry host (zlib+base64 dump packing; spawned eagerly by DRWUI at startup — a DRWUI child, not engine; `deepening-batch3 §1/§6`, `dynamic-ground-truth--wine-loader-run.md:17,25,59`); core has zero network code | none | OOS (verified) |
| api-ms-win-* (40 DLLs: 25 core + 15 crt; 40 unique basenames across 200 copies in 5 payload roots — the corpus's own inventory.csv agrees: 40) | RUNTIME | UCRT API-set forwarders (`runtime-families/ucrt-forwarders.md`) | none | skip |
| archive.dll | RUNTIME-OOS | libarchive; sole importer tree-wide = DRWUI.exe, enumeration-only surface (`rabin2_imports.txt:2939-2955`) = the UI ArchivePreviewModel backend; no engine DLL imports it — the engine-side answer (bounded CD-walk entry inventory) LANDED as M-066 FIXED (Metadata.entries, 512-cap + truncated flag, commit 6d0f16c) + S-032 carve inner entries (3-B) | `cr_preview` Metadata.entries (M-066 FIXED) | skip (capability landed engine-side) |
| auto.txt | DATA (FVE) | the BitLocker FVE auto-unlock key snapshot DRWUI persists at the install root: DRWUI creates `\\.\Pipe\DRWBitlocker` (`CreateNamedPipeA(...,3,6,0xff,0xc800,0xc800,0,0)` — DRWUI_decompiled.c:430715), reads the 0x138-stride count-prefixed key push, deletes + rewrites auto.txt (:430781-430798); the 316-B og file = u32 count + one 0x138-B record (volume GUID, two FILETIME-like stamps, key-blob tail). SECURITY DATUM: the og persists key material PLAINTEXT at the install root — the core never persists key material (unlock-only, in-memory; S-014). The WinPE miscitation was N-097's correction. | none (the core's no-persistence posture is deliberate and safer) | OOS-UI/DRWService sidecar; security datum for host deployers (N-097) |
| *SearchDLL.dll (NTFS/FAT/exFAT/EXT4/HFSPlus/Btrfs/ReFS/RAW/Zfs) | WRAPPER | Per-FS engine plugins, the EUCreate*Searcher factory contract (`searchdll-family--architecture.md`); core equivalent = the in-process FsFamily switch (`cr_scan.cpp:1314-1430`, I-016/D-21/D-59i) — all 9 verified surface-equal (3-F) | `cr_*_engine` + `cr_carve` | VERIFIED (all 9; Btrfs complete-engine = registered I-003 superset; RAWSearchDLL = cr_carve family) |
| BUILDPE/ | INSTALLER | WinPE boot payload; census proved 0 .mo / 0 .rsf in PE (`pe-payload-census--buildpe-easeus-x64.md`) | none | skip |
| cloud_session.dll | EXCLUDED-NAS | Cloud recovery OAuth client (`Cloud/cloud-recovery-architecture.md`) | none | skip |
| concrt140.dll | RUNTIME | VC CRT concurrency runtime | none | skip |
| config/config.ini | WRAPPER-VERIFIED (engine key covered; UI bulk OOS) | 22-languages/fonts/window/UI-state bulk is DRWUI's (`config-data-reference.md:49-55`); the ONE engine-consumed key `[User] only_common` gates RAWSearchDLL carve dispatch (`RAWSearchDLL_decompiled.c:9766-9842`) and is expressible on M-024's runtime surface (N-031 FIXED; I-023/D-60f) | CarveFlags (M-024) | VERIFIED (engine key) / OOS-UI (bulk) |
| config/FilterFolder.ini | WRAPPER-VERIFIED | Junk-folder filter (M-018 FIXED; `cr_filterfolder` + `gen_filterfolder.py`; D-102) | `cr_filterfolder` | VERIFIED |
| config/nasconfig.ini | EXCLUDED-NAS | NAS config — unified-driver exclusion | none | skip |
| config/RawConfig.ini | WRAPPER-VERIFIED | 34 per-format carve toggles (M-024 FIXED; D-97: ini = HOST policy, engine all-enabled; count corrected CORRECTIONS #5) | CarveFlags::disabled_formats | VERIFIED |
| config/usefullostfiles.xml | OOS-UI | Sole consumer = DRWUI (`DRWUI_decompiled.c:320396`) — display-tier whitelist promoting carved RAW photos out of "Unimportant" (3-E); files are recovered regardless | none (hosts build display tiers on cr_query categories, M-023) | OOS (verified — moved into the UI exclusion line) |
| config/version.ini | RUNTIME | Build-pair state file (PE census) | none | skip |
| cpprest141_2_10.dll | RUNTIME | cpprest SDK — dep of the excluded cloud/telemetry paths | none | skip |
| DataFile.ini | REPORTING-OOS | 84-byte UTF-16 upload-batch cursor (`[HEAD_DATA] TOTALCOUNT/STARTINDEX`); sole consumers = AliyunWrap.dll + AliyunWrapExe.exe (`AliyunWrap_decompiled.c:8723/8773/9726`, 3-E) — NOT the generator-set/DAT story (that is lostpart.dat/partinfo.dat, I-024) | none | OOS (verified; reclassified from WRAPPER-VERIFY) |
| dbghelp.dll | RUNTIME | Crash-dump helper — REPORTING family dep | none | skip |
| de265.dll | RUNTIME | libde265 HEVC decoder (preview) — HEIC decode dep (`runtime-families/media-codecs.md:47-49`) | none (HEIC thumbnails = cr_thumbnail phase 2: libheif+libde265, embedded-first) | skip |
| Dict | WRAPPER-VERIFIED | Brute-force password dictionary (N-064 FIXED; D-80; 228 entries byte-faithful; `--bitlocker-dict` og-format loader) | `cr_dict` + `cr_dict_data` | VERIFIED |
| dlconfig.xml | INSTALLER | WAIK download manifest (byte-ranges+MD5 carved from Microsoft's WAIK ISO; consumer = EuDownloader.exe; `architecture/03:59`, `config-data-reference.md:63-67`) — media-builder chain, not an engine config (3-E) | none | skip |
| DRW.exe | REPORTING-OOS | DRWLoader watchdog over the DRWUI UI process (restart loop, 2 MiB shared log, telemetry JSON, DRWReport handoff — `DRW-exe--launcher.md:26-66`); supervision is a downstream-host concern; crash-resilience value already engine-side via cr_crsf sessions (3-C) | none | OOS (verified) |
| DRWReport.exe + DRWReport.ini | REPORTING-OOS | XCrashReport crash reporter (minidump + Aliyun upload); prior dispositions register it OUT-OF-SCOPE (SURFACE-INVENTORY.md:97, I-010/D-26) | none | OOS (verified) |
| DRWService.exe | SIDECAR-OOS | "Read FVE Server": LocalSystem elevation broker pushing FVEAutoUnlock keys over `\\.\Pipe\DRWBitlocker` (0x138 stride, 100-cap) to DRWUI; the ENGINE-side capability (live + offline-hive FVEAutoUnlock harvest) is cr_bitlocker — S-014 FIXED, 11 Linux + 5 Windows tests (3-C) | `cr_bitlocker` / `cr_reghive` | VERIFIED (engine side) / OOS (pipe service = host deployment) |
| Eaolog.log / EPMLOG.log / reglog_win.txt / strutureReg.log / log/ | RUNTIME-ARTIFACT | Installed-product runtime logs; I-033 hygiene datum: the video-repair family ships dev-build debug logging in retail (reglog_win.txt's `movmp4repair.cpp[132] "new algorithm work"`; strutureReg.log's module "视频重组"/version "19.2回退分支"/environment "开发机器", level DEBUG) | none | skip |
| EUAdobeRepair.exe | SIDECAR-OOS | Fixo AdobeRepair (PSD/PSB, cloud-assisted); DRWUI-spawned CLI child (`repair-architecture.md:16,49-53`) | none | OOS (verified) |
| EUArchiveRepair.exe | SIDECAR-OOS | Fixo ArchiveRepair (7z next-header, password-aware via Dict); DRWUI-spawned; preview pairing is UI-layer only (M-066 notes) | none | OOS (verified) |
| EuDownloader.exe | INSTALLER | WAIK/agent/holiday-res resumable range-downloader for the media-builder chain (NOT the update checker — that dead path is CheckVersion/AutoUpdate; 3-C correction) | none | skip |
| EUFileRepair/ (bundle) | SIDECAR-OOS | The Fixo repair product bundle (EUImgRepairFile.exe / EuPDFRepairFile.exe / EuOfficeRepairFile.exe + Magick++/heif/libde265/7z/libcurl runtimes) — separately-sold product line, DRWUI-only orchestration, engines never invoked by recovery (`architecture/06-repair-chain.md:10-19,63-75`; I-010 family-wide) | none | OOS (verified) |
| EUImageEnhance.dll | SIDECAR-OOS | Repair-only OpenCV post-processing; sole consumer = EUImgRepair.exe (`EnhanceMemData/EUCalcDamageProportion/EUbeSimilarPictures`); NOT the preview/thumbnail path (3-B correction) | none | OOS (verified) |
| EUImgRepair.exe | SIDECAR-OOS | Window-IPC host/queue between DRWUI CImgRepairClient and EUImgRepairFile.exe (`image-repair--format-grammar.md:128-138`) | none | OOS (verified) |
| EuRepairHost.exe | SIDECAR-OOS | 50-function activation/licensing bridge (activation.easeus.com); repair is a Pro-gated product capability — licensing domain (3-B) | none | OOS (verified) |
| EUUnZip.exe | SIDECAR-OOS | (a) MFC extractor verifying repaired archives; (b) NAS-agent installer path (connect.zip → NasConfig\connect.exe) — EXCLUDED-NAS-family dependency (3-B) | none | OOS (verified) |
| EUUserData | WRAPPER-VERIFIED | User-taught signature DB (111 entries / 190 signs; M-065 FIXED — `data/userdata-defaults.json` ships + CLI-app startup load (the library default is off — I-036 precision; hosts install via `cr_ui_register_user_signs`); OQ-SA-3 RESOLVED: GetRawUserInfoMap feeds the RAW scan-setup trigger surface) | `cr_usersigns` + `data/userdata-defaults.json` | VERIFIED |
| EuUserRate.dll | OOS-UI | Rate/recommendation prompt (x86 image — cannot load into the x64 engine host); persists only `UserRated` UI state in config.ini which no engine code reads (3-C) | none | OOS (verified) |
| EUVideoRepair.dll | SIDECAR-OOS (engine-side hook already landed) | The ONLY repair component an og engine consumes: RAWSearchDLL's optional per-candidate feed (never a filter; DLL-absent → pipeline unchanged) — landed engine-side as CarvePostProcessor (M-021 FIXED); the repair engine itself stays excluded | `cr_carve` CarvePostProcessor (M-021) | VERIFIED (hook) / OOS (engine) |
| FileFilter.xml | WRAPPER-VERIFIED | 8-cat/1489-ext filter model (M-023 + M-035 FIXED; engine-side consumption proven at `exFATSearchDLL_decompiled.c:47102-47560`). I-033 posture note (the D-103/D-80 sibling sentence, so the runtime-XML-vs-compiled question is not re-litigated per-asset): the core compiles the table in (`cr_filefilter_data.cpp`, kCount=1481 unique — byte-identity re-proven by content diff) exactly as raw_signs (D-103) and Dict (D-80); the og's runtime-XML mutability is covered by the same registered reproducibility-over-field-mutability posture | `cr_query` filter model | VERIFIED |
| firebasefetch.exe / firebaseonlineconfig.dll / google-services.json / remote_config_data | REPORTING-OOS | I-027 register note — `json_raw` decoded: `{"switch":true,"shield":"TXT;HTML;LOG;H;CPP;MM;"}` split on `;` by DRWUI into the raw-tree extension-exclusion list (`DRWUI_decompiled.c:499195-499228`; UI-layer posture OOS — the value is the ground truth for any host-side results filter). I-033 datum: remote_config_data is a runtime-REFETCHED cache — root 62,579 B vs PE 62,611 B differ by 36,040 bytes (the fetched-cache + local-fallback decode's physical proof). Firebase online-config/feature-flag surface — consumer is DRWUI (`DRWUI_decompiled.c:262933` LoadLibraryA; ~600 Firebase feature flags; `Licensing/licensing-activation-chain.md:14` A/B gating of monetization); ZERO references across SearchDLL/MOM/PartitionRecovery artifacts — engine config is local-only (RawConfig/FileFilter/raw_signs/EUUserData, D-60f) | none | OOS (verified; not EXCLUDED-NAS — online config ≠ cloud recovery) |
| GDIPLUS.DLL | RUNTIME | GDI+ — the og's UI-side still-image thumbnail decoder (26 Gdip* imports in DRWUI incl. GdipGetImageThumbnail; 3-D); engine-side decode is our own `cr_thumbnail` | none | skip |
| heif.dll | RUNTIME | libheif — HEIC decode dep (**EUImgRepairFile imports `heif_image_handle_get_thumbnail` — the embedded-thumbnail-first strategy, 3-D; EURawImg imports the decode set** (`heif_context_read_from_memory_without_copy`/`get_plane_readonly`/`scale_image`… — ghidra-repair/EURawImg_imports.txt:1-8, no get_thumbnail) — the N-103 attribution correction) | none (HEIC = cr_thumbnail phase 2) | skip |
| Japanese.dll | UI (localization) | Win32 resource satellite for the Japanese edition (confirmed 3-C) | none | covered by the UI exclusion line |
| legacy.dll | RUNTIME | OpenSSL-3 legacy crypto provider on embedded libtomcrypt (sole export `OSSL_provider_init`); the .rsf-decryption hypothesis REFUTED (.rsf is zlib-deflated, `rsf-format--scan-session.md:15-22`); I-033 closure (N-134's step 2): zero importers/loaders across all 103 reversed trees' imports and decompiles — the loader is libcrypto-3's provider machinery itself, NO og-binary consumer exists | none | skip (verified) |
| libcrypto-* / libssl-* / libeay32* / ssleay32* / libssl32.dll | RUNTIME | OpenSSL trio (`runtime-families/openssl-crypto-trio.md`) | none | skip |
| liblzma.dll / libssh2.dll / libx265.dll / libzmq-mt-4_3_5.dll / zlib.dll / zlib1.dll | RUNTIME | Compression / SSH (NAS dep) / x265 encoder (repair re-encode) / ZeroMQ (MediaGraph IPC) / zlib | none | skip |
| License/ | PACKAGING | Third-party license registry — the convention our dependency policy mirrors when adding deps | none (convention source) | skip |
| Microsoft.VC90.*.manifest / msvc*90* / msvcp120 / msvcr120 / msvcp140 / ucrtbase.dll / vcruntime140.dll | RUNTIME | VC runtime generations (`runtime-families/vc-runtime-generations.md`) | none | skip |
| NasConfig/ + EuShare/ + icloud_logger | EXCLUDED-NAS | NAS redirector + iCloud logging (`EuShare/eumrx-nas-architecture.md`, `Cloud/cloud-recovery-architecture.md`) | none | skip |
| needcopy.txt | INSTALLER | I-033 consumer correction: NOT an installer manifest — `AliyunWrap_decompiled.c:6579` loads it as the crash-report chain's gather list (AliyunWrap itself + the CRT set); the INSTALLER classification outcome (skip) is unchanged, the mechanism text corrected | none | skip |
| pdfium.dll | RUNTIME | PDF page pixmaps — PDFViewer.exe host + DRWUI CPDFRepairGetPixmapThread (3-D matrix) | none (PDF page thumbnails = cr_thumbnail phase 3 if mandated) | skip |
| PE_Tools.dll + PEtools.xml | INSTALLER | The WinPE EMERGENCY-MEDIA BUILDER ("PE" = WinPE, not PE-executable): downloads WAIK objects → stages tree per PEtools.xml → DISM-mount WINPE.WIM → injects BUILDPE payload → writes ISO/USB via Burn.dll (`architecture/03-winpe-boot-environment.md:56-62`, `per-binary/PE_Tools.md` pet_* exports; 3-E) — invoked by DRWUI's CPECreateThread, never imported by any engine module | none | skip (verified; FEATURE-TRIAGE hypothesis REFUTED) |
| PubLog.dll | WRAPPER-VERIFIED | 2 MiB cross-process log ring — the other end of DRWLoader's flush loop (`deepening-batch3 §2`); core = single-process sink seam, registered divergence (N-048 FIXED: host owns placement/rollover/compaction/async) | `cr_log` | VERIFIED (registered divergence) |
| raw_signs.json | WRAPPER-VERIFIED (registered divergence) | 139-ext signature table loaded at engine init (istream tokenizer, `rawsearch-carving--deep-dive.md:54-59`); core compiles it in (`cr_rawsigns_data.cpp`) — posture (a) REGISTERED as D-103: reproducibility over field mutability; the og never writes the file (byte-identical live vs BUILDPE); runtime-mutable surfaces are EUUserData (M-065) + RawConfig policy (D-97) | `cr_rawsigns` | VERIFIED (D-103; N-086 FIXED by the A2 lane) |
| recoveryFree.ini | OOS-LICENSING | 128-byte free-edition quota state, read only by DRWUI's trial registry family; the quota gate lives in DRWUI's CRecoveryMgr upsell flow (3-C). I-027 register note — the decoded quota contract (the spec if the downstream product ever implements gating): HKLM `SOFTWARE\DRWNewFree\Metadata` (sibling `…DRWNewTrial\Metadata`) QWORDs `EnableTotalSize` default 0x80000000 = 2 GiB, `EnableTotalFileCount` default 2^31, `RecoveredTotalSize`, `RecoveredTotalFileCount`, `Times`; a 500 MB (0x1f400000) clamp constant caps a quota parameter; the ini files are the on-disk mirrors of the same record family (consumer `DRWUI_decompiled.c:550794-551204`; I-033 concrete layout: the three nonzero QWORDs at +0x38 = 512 MiB, +0x60 = 2 GiB [the decoded EnableTotalSize default], +0x70 = 1.5 GiB) | none | OOS (verified) |
| SaveScan/*.rsf | WRAPPER-VERIFIED | Scan-session persistence, format FULLY REVERSED + byte-exact-validated (`rsf-format--scan-session.md`); core `cr_crsf` + `docs/crsf-format.md` (D-60g robustness supersets) | `cr_crsf` | VERIFIED |
| send_dmp_with_aliyun.exe | REPORTING-OOS | Crash-dump uploader (`DRWReport-exe--crash-reporter.md:21-28`) | none | OOS (verified) |
| VideoViewer/VideoFileRepairer.exe | SIDECAR-OOS | Fixo video-repair console engine (1.9 MB PE32 x86, fix_main_code lineage; DRWUI-spawned `-r`/`-a` CLI with taskId/port progress IPC; MP4/QuickTime atom grammar moov/trak/mdhd/co64/stsc/stsz/elst; ffmpeg avcodec-62 + 14 faad2 NeAACDec* exports; `easeus_video_repair.log`; the `<dir>\sfc` encrypted engine-init database — N-138's decode, magic " cfs" + the 16-byte XOR key + custom-LZ) — the repair-family exclusion (I-010/D-26, architecture/06-repair-chain.md:10-19); the repair line is separately-sold, DRWUI-only orchestration. Engine-side touchpoint = the M-021 CarvePostProcessor hook (FIXED). Per-binary artifact: `re-workspace/artifacts/per-binary/VideoFileRepairer.md`; `repair-architecture.md:11` | none | OOS (verified — N-130) |
| EaseUS_MediaGraph.dll + VideoViewer/ffmpeg.exe + avcodec-62/avformat-62/avutil-60/... | FEATURE (TRIAGE-UI-AMBIGUOUS resolved) | The og's media decode path: MediaGraph = full ffmpeg pipeline (open_input → find_stream_info → decoder → frame → DIB) hosted by the UI viewer over zmq/TCP; the og's engines never decode. Per session-owner directive the DECODE capability is engine-side FEATURE (cr_thumbnail); viewer/protocol/process stays UI. ffmpeg also appears repair-side (AVIF shell-out) — repair stays OOS (3-D) | `cr_thumbnail` (new; video key-frames = phase 2 with the ffmpeg dependency decision) | DONE (v1 still images, commit 05e6786; see Section C.2) |
| EURawImg.exe | SIDECAR-OOS | DRWUI's hidden RAW-decode sidecar (LibRaw static, 467 exports; mutex/event protocol, ~200 MiB request bound; 3-B/3-D) — the DECODE capability maps to cr_thumbnail phase-2 RAW (LibRaw thumbnail API) | none (phase-2 cross-ref) | OOS (verified) |
| EuSendMessage.exe | INSTALLER | PE-only one-shot WM_COPYDATA messenger to the "DRW_Installer" wizard window (cmd 999) — scripted builder/installer flows (`WinPE/eusendmessage--wm-copydata-messenger.md:9-33`) | none | skip (verified; reclassified from SIDECAR-TRIAGE) |
| EuICloudRecovery.dll | EXCLUDED-NAS | iCloud client (`esicloud`) | none | skip |

## B. PartitionRecovery/* modules (the og engine runtime library surface)

| Entry | Class | Rationale (evidence) | Target in core_recovery | Status |
|---|---|---|---|---|
| AutoUpdate.dll | WRAPPER-DEAD | CONFIRMED DEAD: zero importers and zero by-name loads (`GetAutoUpdateInstance` appears only in its own binary); the shipped product's live update check is DRWUI's own CVersionCtrl::doCheckUpdate — UI-layer (3-C) | none | DEAD — do not implement |
| bitlockerhelp.mo | WRAPPER-VERIFIED | CBitlockerHelpModule pairs with BitLockerLib (`mo-module-map.md:40`) | `cr_bitlocker` + `cr_decrypt_device` | VERIFIED |
| BitLockerLib.dll | WRAPPER-VERIFIED | Full read-side surface (status/parser/protectors/AutoUnlock/manage-bde fallback); og's write-side (encrypt/convert/EOW) deliberately not ported (cr_bitlocker.h F13 "DETECT + LEGITIMATE WINDOWS UNLOCK ONLY"; FVEK-from-password derivation = OQ-S7-1) | `cr_bitlocker` (S-014, M-033 FIXED) | VERIFIED |
| Boot.dll | INSTALLER/tool-op OOS | The EPM legacy mod.boot PREOS BOOT-ITEM INSTALLER (CBoot::Install/PreInstallCheck, boot names "EaseUS DC Linux"/"EaseUS DC Windows PE", bcdedit.cpp wrapper — `deepening-batch3--eight-stubs-resolved.md` §3 re-verified first-hand; 3-A) — NOT part of M-040's post-recovery repair family | none | OOS (verified; reclassified — the M-040 family is EFIBoot.mo/Fixup.mo/Win32Bcd.dll/WinChkdsk.exe) |
| BootDriver.dll | INSTALLER OOS | The BOOT MEDIA BUILDER ("Create Recovery USB/CD" — CBurnBootCD callbacks, .iso authoring; `deepening-batch3` §3; 3-A) — pairs with Burn.dll + PE_Tools in the media-builder family | none | OOS (verified; reclassified) |
| Burn.dll | SIDECAR-OOS (DISPOSITIONED) | IMAPI2 disc writer; sole documented consumer = the PE_Tools WinPE media builder; the master recovery workflow has ZERO burn references; already registered I-019/D-59k + D-26 (3-B — do not re-open) | none | DISPOSITIONED |
| CallbackOperator.mo | WRAPPER-VERIFIED | Event/callback plumbing ↔ typed event stream + progress/cancel/pause tokens (N-016/N-017/N-043 FIXED; s6 §4 poll/push hybrid) | `cr_scan` | VERIFIED |
| CheckVersion.mo | WRAPPER-DEAD | CONFIRMED DEAD-SHIPPED: MOM loads .mo modules on-demand BY NAME and no call site anywhere requests "CheckVersion" (the string appears only in the module's own decompile; `s25-oq-s5-1` exhaustive sweep + first-hand re-verification; 3-C) | none | DEAD — do not implement |
| CloneModule.dll | FEATURE-OOS (registered) | Disk clone + ISO9660 + WinPE/BCD builder (`per-binary/CloneModule.md:52`): the byte-verified master recovery workflow (architecture/04 §7) is FILE-LEVEL EXPORT — no clone step anywhere, no UIUX flow exposes clone; whole-disk recovery is served by cr_safewriter export + cr_apply table write; the exclusive-access hook drivers that power disk copy are WinPE-only; the media/preOS half rides the media-builder family. Register the exclusion D-59k-style; product may revisit (3-A triage) | none (revisit would ride cr_apply + a new sector-copy module) | OOS (registered) |
| Common.mo | WRAPPER-VERIFIED | CConfig/CConfigSection ↔ cr_types/cr_text (s6 MATCH; N-029) | `cr_types` | VERIFIED |
| Config.mo | WRAPPER-VERIFIED | Config surface = the data-file findings (M-024/M-023/M-035/M-018/M-065) + I-023/D-60f (DRWCfg = host scope; per-call config, no settings store — deliberate) | config surface | VERIFIED |
| Device.mo / DeviceAdapter.mo / DeviceManager.mo | WRAPPER-VERIFIED | Device layer: IStorageDevice + transport/rda client (M-030), slice seam, two-stack enumeration + busy-disk retry (N-071 FIXED; residuals D-79/D-85); layout R/W half → cr_apply (S-029) | `cr_device` / `cr_slice` / `cr_enumerate` | VERIFIED |
| EFIBoot.mo | FEATURE | UEFI NVRAM Boot####/BootOrder updater — M-040 stage (b) landed as cr_nvramfix (13 tests, incl. the 4 corpus-proven corrections); re-verified 18/18 green this session | `cr_nvramfix` + `cr_bootfix` | DONE (M-040) |
| eprdtdrv.sys / eprdtdrv_win7older.sys | RUNTIME-DRIVER | The RDA driver binaries — the rda-driver repo owns the driver | rda-driver | skip |
| Error.mo | WRAPPER-VERIFIED | CErrorManager ↔ module-attribution error model (M-049 FIXED; D-15/D-68); the manager/track-manager bus layer is UI-side | `cr_errmod` | VERIFIED |
| ExFatFileSystemAnalyser.mo | WRAPPER-VERIFIED | Analyser type-code probe ↔ content-verified probe_family (N-050) + M-045/M-023 gates | `cr_exfat_engine` + probe_family | VERIFIED |
| ExtFileSystemAnalyser.mo | WRAPPER-VERIFIED | ext2/3/4 one-family engine (I-016: dead Ext2SearchDLL codes 4/5 registered — must not be "fixed") | `cr_ext4_engine` + probe_family | VERIFIED |
| FATFileSystemAnalyser.mo | WRAPPER-VERIFIED | FAT probe gates (M-045) + S-019 BitLocker-FAT + the M-044 UDF probe hosted before FAT classification — exactly the og's layout | `cr_fat_engine` + probe_family | VERIFIED |
| FatFormat.mo | WRAPPER-VERIFIED (scope-excluded half) | CFatFormat = WinPE media-prep formatting — a tool op (I-019/D-59k, grep-verified zero code); the DBR/boot-gate overlap is cr_fat_engine validate_boot (M-045) | `cr_fat_engine` (gates only) | VERIFIED |
| FatLib.mo | WRAPPER-VERIFIED | CFAT12/16/32Table + CDirectoryEntry ↔ chain/LFN (M-011/M-012/N-002/N-010/N-022) | `cr_chain` / `cr_lfn` | VERIFIED |
| FileSystem.mo | WRAPPER-VERIFIED | IFSInterface family ↔ engine factory contract (I-016) + cr_node extent/props model (s5/s6 MATCH) | factory + `cr_node` | VERIFIED |
| FileSystemAnalyser.mo | WRAPPER-VERIFIED | CFileSystemAnalyser + module factory ↔ probe_family dispatch + per-family detect (I-016/D-21; N-050 recognition contract boot-only) | `probe_family` (cr_scan) — NOT cr_analysers (carve registry = N-033, both verified) | VERIFIED |
| FileSystemCheck.mo | WRAPPER-VERIFIED | CCheckFAT/CCheckNTFSVolume ↔ cr_fscheck (M-041 FIXED, per-error decompile citations; pairs with cr_rawfixer) | `cr_fscheck` | VERIFIED |
| Fixup.mo | FEATURE | The BCD/boot offset repair library — M-040's primary corpus source; landed as cr_bcdfix + the cr_bootfix win layer | `cr_bcdfix` + `cr_bootfix` | DONE (M-040) |
| GetDriverInfo.dll | WRAPPER-VERIFIED | NOT a driver-info/install surface: a MOM-loadable DRIVE-TOPOLOGY service (GetLogicalDrives + GetDriveTypeW, TB entry pair, async events, NO SCM APIs — `getdriverinfo--tb-module-decode.md`); target corrected from cr_driver_status | `cr_enumerate` / `cr_devmap` (N-071) | VERIFIED |
| LdmManager.mo | WRAPPER-VERIFIED | VMDB/TOCBLOCK/PRIVHEAD decode + RAID-5 rotations + 4Kn LBA (A-001/A-002, I-008, M-016, N-025, M-046 all FIXED); write-side EditLdm2* = I-019/D-59k deliberate | `cr_ldm` | VERIFIED |
| libvmdk.dll | WRAPPER-VERIFIED (partial, registered) | Hosted surface verified: VMDK sparse/flat/differencing (parent chains), VHD fixed/dynamic/differencing, VHDX dynamic incl. log replay, flat fallback (S-030 FIXED + 20 tests + qemu fixtures). Registered remainder D-64: VMDK streamOptimized (compressed grains, `libvmdk_decompress_data` string-proven) + ESX VMFS extent families — refused honestly with pin tests | `cr_image` | VERIFIED / GAP-D64 (registered) |
| Log.mo | WRAPPER-VERIFIED | CFileMappingLog/CMemMonitorManager ↔ single-process sink seam + memory-pressure check (N-048/N-061/N-062) | `cr_log` | VERIFIED |
| Logger.dll | WRAPPER-VERIFIED | Async queue + compaction = host-side (N-048 registered divergence) | `cr_log` | VERIFIED |
| lostpart.dat | WRAPPER-VERIFIED | 0x448-byte candidate cache; session-embedded equivalents (cr_records + M-042 revalidation) | `cr_records` | VERIFIED |
| MainModule.dll | WRAPPER-VERIFIED | Pending-operation engine (apply/undo chains) ↔ cr_apply (S-029 FIXED) + the M-040 boot-fixup wiring; the MOM-host half is D-59i deliberate | `cr_apply` + bootfix modules | VERIFIED |
| Mom.mo | WRAPPER (deliberate non-port) | MOM = in-house COM loader (name→.mo→LoadLibraryA→GetModule); core uses compile-time linking by design — registered PARITY-ARCH.md:166 / D-59i / I-016 | none (factory/registry instead) | VERIFIED (registered deliberate) |
| NTFSFileSystemAnalyser.mo | WRAPPER-VERIFIED | Analyser boot gates + record-0 USA probe decoded into probe_family (M-043 FIXED) | `cr_ntfs` + probe_family | VERIFIED |
| NTFSLib.mo | WRAPPER-VERIFIED | CAttributeI/CResident/CExtendMFT/mftwith0x20 ↔ the cr_ntfs module family (M-008 superset) | `cr_ntfs/*` | VERIFIED |
| NTFSUtil.mo | WRAPPER-VERIFIED (recovery half; tool half scope-excluded) | Recovery utilities ↔ cr_ntfs; Ntfs2FAT/merge/resize = EPM tool ops (I-019/D-59k; I-020 corrections: CFatVirtualView = conversion engine, NOT damaged-disk views) | `cr_ntfs/*` | VERIFIED |
| partinfo.dat | WRAPPER-VERIFIED | 0x18-byte "Part"-magic live-table snapshot; delete-both-on-first-use + layout_changed semantics in cr_records (I-024 + M-042) | `cr_records` | VERIFIED |
| Partition.mo | WRAPPER-VERIFIED | CMBR/CExtendMBR/CPartitionEntry{,GPT} ↔ cr_table_parse (M-002, N-057, A-001/A-002) | `cr_table_parse` | VERIFIED |
| PartitionRecovery.dll | WRAPPER-VERIFIED | Engine aggregate host + the partition-table write path ↔ cr_apply + engines (S-029; module bus = D-59i) | core aggregate | VERIFIED |
| PartitionRecoveryD.mo | WRAPPER-VERIFIED | Generator sets + lost-partition discovery ↔ cr_lostpart/cr_candidates + cr_records (I-024, M-017, S-006; CFatxBlockParser = FAT boot parser NOT Xbox FATX — I-020 #6) | `cr_lostpart` + `cr_records` | VERIFIED |
| peformat.dll | INSTALLER OOS | USB-boot volume formatter (46 exports; MOM "DeviceManager"+"FatFormat" consumer; formats the "usbboot" FAT volume for emergency media; sole consumer = Boot.dll's media-builder flow — `per-binary/peformat.md` + `Boot_decompiled.c:4755/4784`; 3-E) — the FEATURE-TRIAGE "PE format handler" hypothesis is REFUTED | none | skip (verified; reclassified) |
| Qt5Core.dll / zlib.dll / zlib1.dll | RUNTIME | Host/runtime deps inside the module dir | none | skip |
| RawFixer.dll | WRAPPER-VERIFIED | Transactional FS repair (NTFS/FAT/exFAT fixers, 31-method inventory) | `cr_rawfixer` (S-008/S-010/A-005/A-006/M-009 FIXED) | VERIFIED |
| RecoveryModule.dll | WRAPPER-VERIFIED | The scan-orchestration module for lost partitions (CreateScanLostPartition/Check/Enum/Explorer/RecoveryPartitionObject factories; `mom-module-system--architecture.md` §2:85) ↔ orchestrator + cr_safewriter + cr_apply (CRecoveryMgr itself is DRWUI-side — wording corrected, 3-A) | `cr_scan` + `cr_safewriter` + `cr_apply` | VERIFIED |
| RefsFileSystemAnalyser.mo | WRAPPER-VERIFIED | CRefsAnalyze/MSB procs ↔ cr_refs_detect + cr_refs_engine (N-007/I-004) | `cr_refs` | VERIFIED |
| RegLib.mo | WRAPPER-VERIFIED | regf/hbin read-only parser + FVE harvest (S-014); og hive-write consumers are read paths; hbin write-allocator has no decoded og consumer | `cr_reghive` + `cr_bcdfix` | VERIFIED |
| setupdrv.ini / setupdrvx64.exe | INSTALLER | Driver installer for eprdtdrv | rda-driver deployer (M-064 remainder is driver-install domain) | skip |
| Thread.mo | WRAPPER-VERIFIED | CThread/CMutex/CEvent ↔ cr_parallel + first-class cancellation (D-18/D-21/D-24) | `cr_parallel` | VERIFIED |
| ToolsModule.dll | WRAPPER-VERIFIED | The toolbox: "WinPE media building, burning, imaging (links PE_Tools, Burn.dll, libvmdk)" (`mom-module-system--architecture.md` §2:87) — each leg dispositioned separately (media family OOS; libvmdk VERIFIED; cr_fscheck/cr_apply/cr_bootfix carry the recovery-relevant ops; I-019/D-59k) | per-tool dispositions | VERIFIED |
| Win32Bcd.dll | FEATURE | BCD store surgery for repair — landed in cr_bcdfix (M-040); the media-authoring half (bcdedit wrapper for WinPE/clone media) rides the media-builder family exclusion | `cr_bcdfix` | DONE (M-040, repair surface) / OOS (media authoring) |
| WinChkdsk.exe | SIDECAR-OOS (DISPOSITIONED) | chkdsk delegation — M-040 stage (d) registered UI-LAYER; the core's contract ends at the undo record (D-90) | none | DISPOSITIONED |
| UILogic.dll | UI | UI logic module | none | covered by the UI exclusion line |

## C. FEATURE queue (sorted by criticality from research)

1. **Boot chain repair family (M-040)** — EFIBoot.mo / Fixup.mo / Win32Bcd.dll /
   WinChkdsk.exe → `cr_bcdfix` + `cr_nvramfix` + `cr_bootfix` (+ the rrecov
   `apply --boot-fix` wiring). **DONE — verified this session: 18/18 BcdFix +
   NvramFix tests re-run green (incl. the finding's five named tests); stages
   (c)/(d) registered at D-90; scope corrected — Boot.dll/BootDriver.dll belong
   to the preOS/media-builder family, not this row.**
2. **Thumbnail generation engine (cr_thumbnail)** — session-owner directive.
   The og decodes thumbnails entirely UI-side (GDI+ ≤256 px via
   GdipGetImageThumbnail; MediaGraph/ffmpeg → BMP ≤1 MiB over zmq; EURawImg for
   RAW; libheif embedded thumbnails; pdfium for PDF); the core's in-process
   doctrine makes the engine-side capability a deliberate documented superset
   (I-015 lineage). **DONE (commit 05e6786): v1 still images — JPEG
   (baseline+progressive), PNG, BMP/DIB, GIF (first frame), PSD/PNM/HDR via the
   vendored public-domain decoder (third_party/stb, LICENSE + BUILD-SUPPRESSIONS
   S-1) + native baseline TIFF (uncompressed/PackBits/Deflate strips, 8/16-bit,
   gray/palette/RGB/RGBA, both endians — the TIFF6 signed PackBits count 1-n
   pinned) + native EXIF embedded-thumbnail fast path; dims-first
   decompression-bomb guard with NATIVE header parsers (never stbi_info — stb
   refuses x*y*n ~ 2^30 shapes as corrupt, which would misreport bombs);
   cost-gated bounded reads off PreviewReader (max_read + work units, M-028
   lineage); unreadable-span truth gate (never a thumbnail over zero-fill);
   truncated-stream disclosure on failures AND on Ok; aspect-preserving
   premultiplied box downscale to max_edge (default 256, BOTH axes — correcting
   the og's width-only quirk), never upscaled. Verified: 20 unit tests
   (Thumbnail.* incl. the 6-variant TIFF suite; fixtures
   tests/data/thumbnails + scripts/gen_thumbnail_fixtures.py + SHA256SUMS) +
   fuzz/fuzz_thumbnail.cpp harness; full suites 1003/1003 linux-release AND
   1003/1003 linux-gcc ASan. Phase 2 (deferred, named deps): WebP (libwebp),
   HEIC (libheif+libde265, embedded-first), RAW (LibRaw thumbnail API), video
   key-frames (ffmpeg — the biggest dependency decision, behind an honest
   availability query), TIFF phase-2 (JPEG-in-TIFF, tiles). Phase 3: PDF page
   pixmaps (pdfium). Out: SVG rasterization (og ships qsvg for UI assets only),
   post-repair enhancement (EUImageEnhance domain), thumbcache recovery
   (separate future feature row — carve signatures already known:
   01 00 09 00, F7 89 34 2C).**
   Target: `include/cr/preview/cr_thumbnail.h` + `src/core/preview/cr_thumbnail.cpp`
   (landed) + `diff-to-fix/modules/preview.md` (spec section).
3. **M-066 preview archive enumeration** — **LANDED by the parallel A2 lane
   (commit 6d0f16c): Metadata.entries (name/size/compressed/crc_present)
   collected on the existing CD walk, 512-entry cap + truncated flag,
   docx upgrade coexists, N-088 propagation tail fixed with it.** The
   engine-side surface this inventory called for exists; ABI exposure of
   the entries remains an Agent 1 decision.
4. **N-086 raw_signs posture** — **CLOSED by the A2 lane: posture (a)
   registered as PARITY-DELTAS D-103** (compiled-in generated source,
   reproducibility over field mutability — the same posture 3-E
   recommended); finding FIXED.
5. **Also landed by the parallel lanes while this inventory verified** (not
   Agent 3 items, recorded for board truth): N-087 (FAT 8.3 CP437 boundary
   decode), M-067 (cr_ui_user_sign v3 caps — ABI version 3), OQ-SA-1/2/3
   resolved, S-038 step 1 (PACKAGING.md), M-064 fully fixed (the v0.2.4
   versioned-coexistence remainder landed upstream, dfb2761), N-089 fixed
   (rode with A-011), A-011 (SQLite header endianness). Remaining board:
   S-038 remainder (installer surface — OOS for Agent 3) + tracked-open
   OQ-S4-3. **The thumbnail engine is the only open Agent 3 FEATURE row.**
6. **Registered OOS clusters (no implementation)**: media/preOS-builder family
   (Boot.dll, BootDriver.dll, PE_Tools/PEtools.xml, peformat.dll, Burn.dll,
   dlconfig.xml, EuDownloader.exe, EuSendMessage.exe, CloneModule's media half,
   Win32Bcd's authoring half); the Fixo/DRW repair family (SIDECAR-OOS; the
   M-021 CarvePostProcessor hook already landed); the REPORTING family
   (firebase*, Aliyun chain, DRW.exe/DRWReport.exe, DRWService.exe pipe
   service); CheckVersion.mo/AutoUpdate.dll (DEAD). libvmdk remainder D-64
   (VMDK streamOptimized + VMFS) stays registered, not scheduled.

## D. Handoffs

- **handoff: Agent 1** — once the thumbnail engine lands, the cr_ui ABI surface
  for thumbnail retrieval (entry points, ownership, error codes) is Agent 1's
  contract to define; Agent 3 implements engine-side only.
- **handoff: Agent 1** — M-066's archive-entry Metadata extension needs an ABI
  surface decision when it lands (engine-side field first).
- **handoff: Agent 2** — none (confirmed: nothing in this queue changes scan
  output shape or quick/advanced scan parameters).

## E. Verification record (2026-10-28, sub-agents 3-A..3-F)

| Verifier | Domain | Decisive outcomes |
|---|---|---|
| 3-A | boot family + host modules | M-040 DONE-stamp verified (18/18 tests re-run); Boot.dll/BootDriver.dll reclassified to preOS/media family; GetDriverInfo target corrected to cr_enumerate/cr_devmap (drive-topology service, no SCM APIs); Mom.mo = D-59i deliberate non-port; CloneModule = FEATURE-OOS (master workflow has no clone step); MainModule/RecoveryModule/ToolsModule/PartitionRecovery.dll mapped VERIFIED |
| 3-B | file-repair family | Whole family SIDECAR-OOS with rationale (two repair worlds, both UI-routed Pro-gated, engines never invoke — sole exception M-021's CarvePostProcessor already landed); archive.dll = RUNTIME-OOS (DRWUI-only libarchive); Burn.dll already dispositioned D-59k/D-26; EUImageEnhance = repair-only (corrects the preview-path hypothesis) |
| 3-C | REPORTING + services | firebase* = REPORTING-OOS (consumer DRWUI, zero engine refs — online config ≠ NAS); Aliyun chain + DRW.exe + DRWReport.exe = REPORTING-OOS (product telemetry; I-033 correction: the PE tree drops ALIYUN ONLY — firebasefetch.exe + firebaseonlineconfig.dll + google-services.json + remote_config_data + DRWReport.exe + DRW.exe all ship in BUILDPE, `ls BUILDPE/EaseUS-x64 | grep -i aliyun` → empty); DRWService = SIDECAR-OOS (engine side = S-014 cr_bitlocker); EuSendMessage = INSTALLER; EuDownloader = INSTALLER (WAIK downloader — rationale corrected); legacy.dll = OpenSSL-3 legacy provider; recoveryFree.ini = licensing; EuUserRate = no engine-read persistence; AutoUpdate/CheckVersion WRAPPER-DEAD CONFIRMED (by-name load contract) |
| 3-D | thumbnail/decode pipeline | Full decode matrix (GDI+ / Qt plugins / libheif / LibRaw-EURawImg / MediaGraph-ffmpeg / pdfium) + protocol bounds (602-byte header, 1 MiB BMP payload, 10 s wait, 100 MB eligibility) + the 256-px GDI+ thumbnail evidence (DRWUI_decompiled.c:652395-652406) + the v1 spec in Section C.2; TRIAGE-UI-AMBIGUOUS resolved: decode = FEATURE, viewer/protocol = UI |
| 3-E | config/format surface | PE_Tools/peformat = WinPE media builder (INSTALLER — the PE-executable hypothesis REFUTED); DataFile.ini = Aliyun upload cursor (REPORTING — the generator-set attribution corrected); dlconfig.xml = WAIK manifest (INSTALLER); config.ini = UI bulk + only_common engine key covered (I-023/N-031/D-60f); usefullostfiles.xml = DRWUI display-layer (OOS-UI); all six VERIFIED stamps CONFIRMED; N-086 → posture (a) |
| 3-F | per-FS surface | All 9 SearchDLL wrappers + all 30 .mo runtime modules WRAPPER-VERIFIED (I-016/D-21 factory contract; standing notes: scanext4/scanzfs enable gates absent = I-016 note 1); libvmdk PARTIAL (D-64 registered remainder: VMDK streamOptimized + VMFS); UDF fully covered (M-044 — probe hosted before the FAT gate, exactly the og's layout); FileSystemAnalyser target corrected to probe_family |

## F. Appendix — full file enumeration (compact)

Every non-UI og-tree file maps to exactly one row above or one family below:

- **RUNTIME families:** api-ms-win-core-* / api-ms-win-crt-* (forwarder sets at
  root + BUILDPE + diskhealth + EUFileRepair + VideoViewer); VC runtimes
  (concrt140/msvcp140*/msvcr*/msvcp90/msvcm90/msvcp120/msvcr120/ucrtbase/
  vcruntime140 + .VC90 manifests + mfc90u/mfc140u); Qt5* family;
  OpenSSL set (libcrypto-1_1/-3 x86+x64, libssl-1_1/-3, libeay32,
  libeay32MD, ssleay32, ssleay32MD, libssl32, legacy.dll); liblzma/libssh2/
  libx265/libzmq-mt-4_3_5/zlib/zlib1/cpprest141_2_10; codecs de265/heif;
  pdfium; GDIPLUS; dbghelp; d3d8thk/d3d9/d3d11/dxgi (BUILDPE);
  CORE_RL_* (24) + IM_MOD_RL_* (7) + 7z.dll + libcurl + postproc-58
  (EUFileRepair bundle); avcodec-62/avdevice-62/avfilter-11/avformat-62/
  avutil-60/postproc-58/swresample-6/swscale-9 + ffmpeg.exe (VideoViewer
  bundle — decode path dispositioned with the thumbnail FEATURE row); 7za.exe.
- **UI exclusion line covers:** every remaining file — see the single line in
  the header (viewers, Qt payloads, resources, locales incl. usefullostfiles.xml
  and PriorityDisplayType.xml/shellexe.ini/themdark.ini, PE payload, installer).
- **INSTALLER:** SetupUE.exe, InfoForSetup.exe, unins000.{dat,exe,msg},
  Upgrade.ini, auto.txt, needcopy.txt, BUILDPE/ (incl. x64/Windows/**,
  regdrv.reg, software.reg, auto.bat, wallpaper.bmp, EUDCPEPM.sys,
  EUEDKEPM.sys, ebrntdrv.sys, epmdkdrv.sys, msi.dll, oledlg.dll,
  Unattend.xml, winpeshl.ini), diskhealth task bats, EuDownloader.exe,
  EuSendMessage.exe, dlconfig.xml, PE_Tools.dll + PEtools.xml,
  PartitionRecovery/{setupdrv.ini, setupdrvx64.exe, peformat.dll},
  PartitionRecovery/{Boot.dll, BootDriver.dll} (preOS/media-builder family),
  PartitionRecovery/CloneModule.dll (registered FEATURE-OOS).
- **EXCLUDED-NAS:** cloud_session.dll, EuICloudRecovery.dll, icloud_logger,
  EuShare/ (EuMrx.sys + install/uninstall.bat), NasConfig/ (NasModel.xml,
  ssh_config.ini), config/nasconfig.ini.
- **REPORTING-OOS:** AliyunConfig.ini, AliyunWrap.dll, AliyunWrapExe.exe,
  send_dmp_with_aliyun.exe, DataFile.ini, firebasefetch.exe,
  firebaseonlineconfig.dll, google-services.json, remote_config_data,
  DRW.exe, DRWReport.exe + DRWReport.ini, DRWService.exe (SIDECAR-OOS),
  PartitionRecovery/{AutoUpdate.dll, CheckVersion.mo} (DEAD).
