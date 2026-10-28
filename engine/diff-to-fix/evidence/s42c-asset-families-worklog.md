> **S42-C pin-time + renumber note (added at the third reconciliation):** this
> sheet records the tree state as verified at the lane's pin **`869da779`** (core) /
> **`be0cfe9`** (corpus). THREE hot windows landed while this lane worked; the
> sheet's internal draft IDs are superseded as follows: this lane's "M-118"/"M-121"
> (the Stbio large-IDAT finding) → final **M-122**; "I-035"/"I-037" (the diskhealth
> register) → final **I-038**; "I-036"/"I-038" (the corpus-worklog register) → final
> **I-039**; the red-main draft "N-154" → folded as the N-157 corroboration addendum.
> The remote lanes' same-numbered findings (the S12 MountPoints2 M-118, the DBX
> chain's M-121, the decode register I-037, the S42 lane's config-layer I-036) are
> different subjects. The sheet's liveness verdicts for findings the windows have
> since FIXED (S-048, M-117/M-121-dbx, N-152, N-125, N-126, N-135, N-148,
> S-054-the-N-013-regression, N-149, M-076, the INFO wave, N-115's N-158 tail
> aside) are pin-time-historical (true at the pin, superseded at the reconciled
> tree). The landing-parity VERIFIED-PARITY verdicts are unaffected. This lane is
> the S42-C lane (rebranded twice: the remote's S42 and S42-B lanes pushed first).

# W3b — og-codebase asset-family disposition sweep + the corpus worklog cross-check

**Task ID:** S42/W3b · READ-ONLY audit (no repo file modified; only this scratch sheet written)
**Corpus (ground truth):** `/home/z/my-project/easeus-drw-research/` (og product tree: `og-codebase/`, 1470 files / 167 top-level entries / 660 MiB — counts re-verified first-hand this session and **byte-exact against SURFACE-INVENTORY §11's "1470 files / 167 top-level entries"**)
**Audit trail:** `/home/z/my-project/core_recovery/diff-to-fix/` + `docs/` registers
**Prior registers read in full:** I-027, I-033, I-034 (findings/INFORMATIONAL/), `docs/PARITY-OG-CORE-INVENTORY.md` (290 lines, the og-binary census — "row N" below = its table rows), `diff-to-fix/SURFACE-INVENTORY.md`
**Corpus worklog read:** `/home/z/my-project/easeus-drw-research/worklog.md` — 807 lines, **in full** (line numbers below cite it). Verified first-hand that the trail has never cited it (`rg "easeus-drw-research/worklog|corpus worklog"` over diff-to-fix/ → 0 hits) — this is the first lane to read it.

---

## Part 1 — The asset families, one by one

### F1. `language/` — 37 Qt compiled-translation catalogs (6.2 MiB)

**Inventory (first-hand `ls`):** 37 `.qm` files = 22 product locales (Arabic, Chinese, ChineseTrad, Danish, Deutsch, Dutch, English, French, Indonesian, Italian, Japanese, Korean, Malay, Mungarian, Norwegian, Polish, Portuguesa, Russian, Spanish, Svenska, Thai, Turkish) + 4 Mainland OEM variants (Mainland.qm 141 KB, Mainland_Foxit.qm 70 KB, Mainland_JZ.qm 141 KB, Mainland_LC.qm 83 KB) + 11 `qtbase_*.qm` (de/en/es/fr/it/ja/ko/nl/pt/zh_CN/zh_TW; **qtbase_en.qm is 16 bytes — header only**, the "English is Qt's native language" control case). A 37-file duplicate ships at `BUILDPE/EaseUS-x64/language/` (count verified: 37).

**What the corpus RE derived from them (the task's question):** YES — two artifact families ride these files:
- `artifacts/Languages/language-packs--family-census.md` (+ README): the 37-file census, the Mainland OEM set-analysis (4 independently-built sub-brand catalogs; Mainland_Foxit = NAS+BitLocker-forward marketing: "一键连接NAS…", "BitLocker 自动解密失败"; ∩-set math 98/90/142), the language-selection architecture (software.reg boot seed `Language=English`; Qt QTranslator::load; `version.ini` SelLanguage).
- `artifacts/UIUX/02-string-inventory.md`: the UI string table — per-language message counts (Japanese 1,556, Mainland 1,545, most European 1,505–1,528, Chinese/Trad 1,528), "Mainland_Foxit.qm (19 messages — the "FoxitRestore"-branded variant)", locale-gated China payment copy. Worklog L52: "English.qm parsed: 1537 UI strings"; L188: "strings (1,556x25 langs + 18,939 binary strings)".
- Tooling persisted: `scripts/qm_parse.py`, `scripts/parse_qm.py` (corpus).

**Does the core have ANY language/localization surface?** **NO.** `rg -il "qtranslator|localization"` over `src/ include/ tools/` → 0 hits (run this session). The only trail mentions are OOS registrations. Correct posture per the registered exclusion.

**Existing register rows:** PARITY-OG-CORE-INVENTORY header UI-exclusion line (:30) — "language/*.qm + translations/ + Japanese.dll (Win32 resource satellite)"; I-034's table — "Languages/language-packs--family-census.md | the translation family — multi-language OOS (D-26)"; SURFACE-INVENTORY §5 row "Cloud / licensing / languages / runtime families … OUT-OF-SCOPE (registered exclusions)"; Japanese.dll has its own row 91 ("Win32 resource satellite for the Japanese edition, confirmed 3-C").

**Verdict: COVERED** (UI/localization OOS, registered in three places). No NEVER-DISPOSITIONED file.

**New corpus-side defect found (CORPUS-ERRATA candidate, W3b-E1):** the census's group arithmetic is internally wrong — census §1 and README line 3 both say "**8 qtbase** Qt-internal" but enumerate 11 language codes (de/en/es/fr/it/ja/ko/nl/pt/zh_CN/zh_TW) and the tree holds **11 qtbase files** (verified first-hand); the headline "25 product locales + 4 Mainland OEM + 8 qtbase = 37" holds only by coincidence (actual split: **22 + 4 + 11 = 37**). Same class as the register-count defects N-129 owns trail-side.

### F2. `License/` — 19 third-party OSS license texts (160 KB)

**Inventory (first-hand):** aliyun, cjson, cpprest, firebase, heic, jsoncpp, libarchive, libcurl, libheif, libjpg, libraw, libzmq (16.7 KB), opencv, openssl, pdfium (62.5 KB), qt, tinyxml, xz, zlib license texts. **This is NOT an activation/license-server asset family** — the task's "license/activation assets" framing is corrected here: it is the third-party-license registry of the bundled runtimes. The product's own licensing/activation surface is a different family (below).

**The licensing flow (corpus docs, verified):** `artifacts/Licensing/licensing-activation-chain.md` (read in full): euactiveonline 3.1 statically linked in DRWUI; DRWUI activation dialog family; EUinApp.exe (store bridge); EuRepairHost.exe (activation.easeus.com[.cn]); Firebase A/B gating; free→paid gates ("Unlock Full Recovery", "1 License for %1"); reconstructed flow scan→quota→buy→COnlineActivation→writeActivateData.

**Existing register rows:** row 95 — "License/ | PACKAGING | Third-party license registry — the convention our dependency policy mirrors when adding deps | skip". The activation flow itself: SURFACE-INVENTORY §5 Cloud/licensing row (OOS, registered); I-027 #1 (recoveryFree.ini free-edition quota contract: HKLM SOFTWARE\DRWNewFree\Metadata QWORDs, 2 GiB default, 500 MB clamp); row 103 recoveryFree.ini OOS-LICENSING; row 82 EuRepairHost.exe (licensing domain); EUinApp.exe in the UI-exclusion line :30; I-034 dispositions `Licensing/euinapp--ie-bootstrapper.md` ("in-app-purchase IE-emulation bootstrapper — Licensing/monetization OOS (D-26)"); S-038 owns the installer telemetry/retention half.

**Verdict: COVERED** — the tree has its own row (95) and the flow's OOS registration exists and is multi-site. The trail's demanded registration is present.

### F3. `EuShare/` (3 files, 224 KB), `NasConfig/` (2 files, 16 KB)

**Inventory:** EuShare/ = EuMrx.sys (215,704 B, the RDBSS NAS mini-redirector) + install.bat (1,748 B) + uninstall.bat (1,150 B). NasConfig/ = NasModel.xml (6,138 B, Synology model DB per worklog L81) + ssh_config.ini (92 B).

**Existing register rows:** row 97 "NasConfig/ + EuShare/ + icloud_logger | EXCLUDED-NAS"; the F-Appendix EXCLUDED-NAS list (:283-285) names every file incl. "NasModel.xml, ssh_config.ini"; SURFACE-INVENTORY §1 EuMrx row (OUT-OF-SCOPE network/NAS; the 2-artifact correction in §9) + §5 EuMrx scripted-install row (ProviderOrder prepend/restore + the og's own uninstall.bat:39 malformed-`if` bug) + §6 items 19-20.

**Verdict: COVERED** (EXCLUDED-NAS, registered at family and file granularity).

### F4. `platforms/` (3), `imageformats/` (3), `log/` (4 shipped)

**Inventory (first-hand):** platforms/ = qminimal.dll, qoffscreen.dll, qwindows.dll (Qt platform plugins); imageformats/ = qgif.dll, qjpeg.dll, qsvg.dll (Qt image codecs); log/ = CalcPartition.log (31,592 B), DRWLog.log (0 B), DRWUI.log (41,646 B), DefaultLog.txt (40,735 B).

**Existing register rows:** the UI-exclusion line :30 ("all Qt5/QML/platforms/imageformats/iconengines/styles/… payloads"); row 75 (Eaolog.log/EPMLOG.log/reglog_win.txt/strutureReg.log/log/ → RUNTIME-ARTIFACT, skip); I-027's sweep cites the runtime logs (row 75 + the vendor-logs artifact: "EPMLOG corroborates N-071; DRWUI.log's auto-rsf names corroborate cr_crsf.cpp:363-404; CalcPartition corroborates M-017").

**A register claim verified first-hand (holds, with a nuance):** SURFACE-INVENTORY §8 registers "the log\BuildRaw.log + log\DelFat.log engine run-log channels (telemetry-equivalent, OUT)". **Neither file exists anywhere in the corpus** (`find` over the whole corpus → 0 hits — run this session). They are *runtime-written* channels: the format strings are embedded in the engines (`%s\log\DelFat.log` at FATSearchDLL_decompiled.c / rabin2_strings at 0x180052b48; `%sBuildRaw.log` in DRWUI_decompiled.c — both grepped first-hand). The registration is accurate as a channel decode; my report notes the shipped-tree nuance so a future filename-keyed sweep does not misread it.

**Verdict: COVERED.**

### F5. `Resource/` (30 MB) + `ResourceDark/` (31 MB)

**Inventory (first-hand + sampled, not enumerated):** each tree = `ImageResource.xml` (38,706 B, identical size both themes; device-type→icon map — `<Partition normal="/DriveTypes/drivetype_ico_disk.png">` + per-type rows, head read) + one Qt RCC v3 bundle: `drw_light.rcc` (30 MB) / `drw_dark.rcc` (31 MB). RCC magic verified first-hand: `q r e s` + version 3 (od dump).

**What they represent / corpus decode:** the complete UI asset store. Worklog L101-105 (uiux-1): the Qt RCC v3 binary format reversed from first principles; drw_light.rcc = **3,722 extracted files** (Home 435, Preview 340, Filter 221, LeftTree 199, ScanProgress 188, Evaluate 162, Titlebar 145, PopWnd 254, FileTypes 105 SVG, DriveTypes 60, AiAssistant 52, NAS 247, PEView 113, Repair 91…); drw_dark.rcc = 3,657; DiskHealth.rcc (in diskhealth/) = 125 incl. 54 QML components; DIN fonts extracted; the special classify folders "File Path Lost"/"Tags Extracted"/"Unimportant" seen in the FileTypes buckets (L106).

**Existing register rows:** UI-exclusion line :30 ("Resource/ + ResourceDark/ + style.css + themdark.ini + PriorityDisplayType.xml + shellexe.ini + usefullostfiles.xml … + ImageResource.xml + all *.png/*.bmp/*.ico/*.cur"); I-034 names drw_light.rcc explicitly ("the light-theme sibling of the registered ResourceDark" — in its never-mentioned-set disposition).

**Verdict: COVERED** (UI OOS; the extraction evidence lives corpus-side under re-workspace/reversed/ui-resources/).

### F6. `BUILDPE/` — 170 MB, 194 files — the WinPE build environment

**Inventory (first-hand):**
- `BUILDPE/EaseUS-x64/` — 131 top-level entries / **180 files recursive** (count matches the corpus PE census artifact's "180 files" EXACTLY). The PE product payload: the 9 *SearchDLL engines, DRW.exe/DRWUI.exe/DRWService.exe/DRWReport.exe, the 5 viewer/repair hosts, Dict, EUUserData, FileFilter.xml, raw_signs.json, Resource/, config/ (9 files incl. version.ini), language/ (37 qm), platforms/imageformats/iconengines/styles, **the Firebase chain retained** (firebasefetch.exe + firebaseonlineconfig.dll + google-services.json + remote_config_data — per I-033 #3; Aliyun dropped: `ls | grep -i aliyun` → empty), plus **PE-only** d3d8thk.dll, d3d9.dll, d3d11.dll, dxgi.dll, mfc90u.dll, EuSendMessage.exe.
- `BUILDPE/x64/` — 14 files: auto.bat, regdrv.reg, software.reg, wallpaper.bmp + Windows/SysWOW64/{msi.dll, oledlg.dll} + Windows/system32/{winpeshl.ini, Unattend.xml, msi.dll, oledlg.dll, ebrntdrv.sys, epmdkdrv.sys} + Windows/system32/drivers/{EUDCPEPM.sys, EUEDKEPM.sys}.
- **No boot WIM ships.** The WIM is WAIK-sourced at media-build time (dlconfig.xml byte-ranges → EuDownloader.exe → PE_Tools.dll DISM-mounts WINPE.WIM and injects this payload — row 100 + architecture/03:56-62). Drivers, not WIMs, are what the tree carries.

**Was BUILDPE ever dispositioned by a prior lane? YES — comprehensively (14 trail files grep "BUILDPE"):**
- Row 57: "BUILDPE/ | INSTALLER | WinPE boot payload; census proved 0 .mo / 0 .rsf in PE (`pe-payload-census--buildpe-easeus-x64.md`)" → **the WinPE capability boundary**: no lost-partition search in the bootable edition; SURFACE-INVENTORY §5 WinPE row ("DISPOSITIONED this session — … recorded as the boot-media builder's design input (roadmap.md:71 + S-038's K78)").
- The F-Appendix INSTALLER list (:274-282) enumerates BUILDPE/x64/Windows/**, regdrv.reg, software.reg, auto.bat, wallpaper.bmp, EUDCPEPM/EUEDKEPM/ebrntdrv/epmdkdrv, msi.dll, oledlg.dll, Unattend.xml, winpeshl.ini.
- **M-087** (read this session): BUILDPE/x64/regdrv.reg:35-36 is the og's own correct hex(7) UpperFilters encoding — the rda-driver's regdrv-rda.reg is the invalid one (PUSHED-FORWARD, driver-repo-resident); its SA-9 addendum carries the **BOOT_START posture** divergence (og: EUDCPEPM Start=0 BOOT_START vs rda Start=3 demand — the filter role can never engage at boot).
- **M-072** owns the regdrv-rda.reg unparseable-hex(7) + DISKDRIVE-vs-VOLUME class + Start posture parameters (SURFACE-INVENTORY §1 driver pre-registration row: "AUDITED (session 9) — … OWNED by M-072").
- I-033 #3/#4: BUILDPE keeps the Firebase chain while dropping only Aliyun; root-vs-PE remote_config_data differ by 36,040 bytes (runtime-refetched cache proof).
- M-105/M-108: DRWService install lifecycle (the PE copy included in the product frame).
- I-034 dispositions the PE-census artifact row; N-129's count corrections touch the api-ms set carried in PE.

**Verdict: COVERED — fully dispositioned; zero NEVER-DISPOSITIONED entries in the 194-file tree.**

### F7. `EUFileRepair/` (46 MB, 87 files) and `VideoViewer/` (100 MB, 76 files)

**EUFileRepair/ inventory (first-hand):** the 5 Fixo repair consoles — EUAdobeRepair.exe, EUArchiveRepair.exe, EUImgRepairFile.exe (5.2 MB), EuOfficeRepairFile.exe (10.5 MB), EuPDFRepairFile.exe (1.9 MB) — + the ImageMagick runtime (25 CORE_RL_* incl. Magick++/MagickCore/MagickWand; 7 IM_MOD_RL_* codecs) + 7z.dll + libcurl + libde265 + heif.dll (781 KB) + libcrypto-3/libssl-3 + postproc-58 + the CRT forwarder set.

**Does the existing disposition cover the TREE, not just the main exes? YES:**
- Row 79 names the bundle as a unit: "EUFileRepair/ (bundle) | SIDECAR-OOS | The Fixo repair product bundle (EUImgRepairFile.exe / EuPDFRepairFile.exe / EuOfficeRepairFile.exe + Magick++/heif/libde265/7z/libcurl runtimes) — separately-sold product line, DRWUI-only orchestration (`architecture/06-repair-chain.md:10-19,63-75`; I-010 family-wide)".
- The bundle's EUAdobeRepair.exe / EUArchiveRepair.exe duplicates have their own rows 76/77; **S-050** (read in full this session) owns the capability gap for the whole line (9 root binaries + 8 FileRepair artifacts + enable_imgrepair/enable_docrepair flags) with the UI-side addendum (UIUX/32 row contracts, queue, thread fleet).
- I-033 #10 confirms the corpus-side phantom correction: "the shipped bundle is 5 exes, all register-dispositioned" (EUZipRepairFile.exe/EURarRepairFile.exe/EUVideoRepairFile.exe in a ghidra REPORT.md do not exist).
- F-Appendix: "CORE_RL_* (24) + IM_MOD_RL_* (7) + 7z.dll + libcurl + postproc-58 (EUFileRepair bundle)".

**VideoViewer/ inventory (first-hand):** EaseUS_VideoViewer.exe (121 KB), **VideoFileRepairer.exe** (1.9 MB), EaseUS_MediaGraph.dll (99 KB), ffmpeg.exe (532 KB) + avcodec-62 (58 MB)/avdevice-62/avfilter-11/avformat-62/avutil-60/postproc-58/swresample-6/swscale-9, libzmq, **sfc** (211,614 B), msvcp140/vcruntime140/ucrtbase + CRT forwarders, 20 PNG/cur UI assets (play/pause/volume/hover cursors), VideoViewer.ico.

**Tree-level dispositions verified:** VideoFileRepairer.exe → **N-130** (register row 106, SIDECAR-OOS, full evidence chain: fix_main_code lineage, -r/-a CLI, MP4/QT atom grammar, avcodec-62 + faad2) — FIXED with the row landed; **sfc** → **N-138** (the "dead payload" verdict REFUTED: it is VideoFileRepairer.exe's LIVE encrypted engine-init database — magic " cfs" + 16-byte XOR key + custom-LZ, ctor load + rebuild-on-failure) — FIXED, ledger SUPERSEDED; EaseUS_MediaGraph + ffmpeg + the av* family → row 107 (FEATURE: the decode capability landed engine-side as cr_thumbnail — DONE, commit 05e6786; the viewer/protocol half stays UI) + F-Appendix ("VideoViewer bundle — decode path dispositioned with the thumbnail FEATURE row"); EaseUS_VideoViewer.exe + PNG/cur assets → UI-exclusion line :30 ("EaseUS_VideoViewer.exe and VideoViewer/ UI assets (incl. VideoFileRepairer.exe — the OOS repair child, row above)").

**Verdict: BOTH TREES COVERED — the dispositions are tree-complete, not exe-only.** One residual already tracked: N-138's open question (the 827,489-byte database's record semantics need the LZ decompressor ported — registered non-blocking).

### F8. `SaveScan/` — 66 MB, exactly 3 files (the real .rsf session corpus)

**Inventory (first-hand):** `auto_Local Disk (G_)10_15_00_44_10.rsf` (7,366,685 B), `auto_Local Disk (H_)10_14_23_42_58.rsf` (52,744,004 B), `auto_Local Disk (I_)10_14_23_13_21.rsf` (8,996,707 B). **Nothing else is in the tree** (no other files).

**Existing dispositions:** row 104 (SaveScan/*.rsf WRAPPER-VERIFIED: format fully reversed + byte-exact-validated, `rsf-format--scan-session.md`, core cr_crsf + docs/crsf-format.md); SURFACE-INVENTORY §11: "the three real SaveScan .rsf files parsed first-hand — the documented container grammar byte-confirmed on real artifacts (magic GUID 1BB3F4EA-4305-4AB6-B4B8-244A33D913BB, 0x8C file header, 0x24 record header, 4 KiB chunked zlib; evidence/executions/s10-rsf-ground-truth.md)"; S-049 (carved-file naming contract — the .rsf naming evidence "auto_*" corroborated by DRWUI.log per I-027) + S-048; modules/session.md (the record-4 tree grammar, the GetType()==0xB never-saved rule, progress formulas). The worklog's mining is the corpus-side counterpart: rsf_parser.py validated on all 3 samples (L131-132: 36 MB tree from the 8.99 MB record, 374 K strings, real QA scan data "Local Disk (I:)"); record-1/2/3 schemas byte-decoded (L225-232: rec2 147 B exact / rec3 7,185 B exact / the timestamp field-order CORRECTION to 2026-10-14 23:13:21.774); record-4 grammar validated to exact EOF (L323-326: 36,001,842 / 29,894,861 / 226,686,612 decompressed bytes, 1,986,806 nodes, roots "Local Disk(I:)" / "File Path Lost" / "Reconstructed").

**Verdict: COVERED.**

### F9. `Dict/`, `EUUserData/`, `config/` — register rows re-verified byte-level this session

- **Dict** (a FILE, ASCII, CRLF): 2,742 B / 227 lines → 228 entries — **matches** row 70 (WRAPPER-VERIFIED, N-064 FIXED, 228 entries byte-faithful) and I-033 #1 (228/228 content-identical to kDictPool) and worklog L164 ("Dict corrected to 228 entries/2742B").
- **EUUserData** (a FILE, JSON): 16,891 B, **111 top-level keys** (python count) — matches row 84 (WRAPPER-VERIFIED; M-065 FIXED; 111 entries/190 signs) and I-033 #1 (111/111 + 190/190 zero value diffs vs data/userdata-defaults.json).
- **raw_signs.json**: **139 extension keys** (python count) — matches row 102 (compiled-in, D-103 posture) + I-027 #5's sibling facts.
- **FileFilter.xml**: 54,597 B — matches row 87 (8-cat/1489-ext, M-023+M-035) and PARITY-ARCH's "verified present, 54,597 bytes".
- **config/** (9 files): config.ini (row 60: engine key `[User] only_common` VERIFIED / UI bulk OOS), FilterFolder.ini (row 61 VERIFIED), nasconfig.ini (row 62 EXCLUDED-NAS), RawConfig.ini (row 63 VERIFIED, 34 toggles), usefullostfiles.xml (row 64 OOS-UI), version.ini (row 65 RUNTIME), **PriorityDisplayType.xml + shellexe.ini + themdark.ini** (UI-exclusion line :30; the PriorityDisplayType/SumaryFileType "landed" claim drift is owned by **N-132** + M-108 per SURFACE-INVENTORY §11).

**Verdict: COVERED — the I-027/I-033 register rows still hold at the current tree (spot-verified by execution, not by trust).**

### F10. `PartitionRecovery/` — 27 MB, 59 entries — disposition currency spot-verified

**Inventory (first-hand):** the 30-.mo MOM module set + host DLLs (MainModule/RecoveryModule/ToolsModule/PartitionRecovery.dll/UILogic.dll/GetDriverInfo.dll/AutoUpdate.dll/BitLockerLib.dll/Boot.dll/BootDriver.dll/Burn.dll/CloneModule.dll/Win32Bcd.dll/WinChkdsk.exe/libvmdk.dll/RawFixer.dll/Logger.dll), the driver pair eprdtdrv.sys + eprdtdrv_win7older.sys, data files lostpart.dat + partinfo.dat, installer pair setupdrv.ini + setupdrvx64.exe, peformat.dll, Qt5Core/zlib runtimes.

**Currency check (the task's ask):**
- **modules/lost-partition.md** (read this session): current — audited `67193b6` + the GAP-S6-6 containment fix in-tree; S-006 FIXED (ext parity + superset anchors); M-017 scoring / OQ-T9 (CalcPartition internals undecoded) tracked-open. No staleness.
- **CPartitionVirtualVolumeDevice** (the volume-abstraction engine the prior lanes audited): dispositioned in modules/storage.md ("Reference (og ground truth): CPartitionVirtualVolumeDevice + IStorageDevice … volume-abstraction layer") — grep-verified.
- Section B of the census carries a row for every file (rows 116-166): AutoUpdate/CheckVersion WRAPPER-DEAD (confirmed by-name-load contract), EFIBoot/Fixup/Win32Bcd DONE (M-040, 18/18 re-run), Burn DISPOSITIONED, CloneModule FEATURE-OOS (registered), GetDriverInfo → N-106, libvmdk VERIFIED + D-64 remainder, lostpart/partinfo VERIFIED (I-024/M-042), Mom.mo D-59i deliberate non-port, RawFixer VERIFIED (see the 31→33 note below), setupdrv/x64 M-064, WinChkdsk DISPOSITIONED + M-075 (chkdsk launch gap, incl. the VSS/LogVss half).
- modules/ntfs.md carries $MFTMirr (N-001 FIXED d7f3c78); modules/check-phase.md the $MFT/$MFTMirr proof chain.

**Verdict: COVERED — the module-level dispositions are current at the audited tree.**

**One trail-side numeric defect found (N-129-class, W3b-E2):** census row 157 still reads "RawFixer.dll … (NTFS/FAT/exFAT fixers, **31-method inventory**)" while the trail's own modules/rawfixer.md, docs/AUDIT.md (row 36), docs/API-SURFACE-AUDIT.md and `src/core/repair/cr_rawfixer.cpp` all carry the **33-method** og table (upgraded per the corpus's RawFixer-v2 decode — the corpus worklog itself drifted 31 [L163] → 33 [L699]). The register row was never updated. Same class as N-129's six register-count defects; 2-word fix in row 157.

### F11. The root-level loose files — every one mapped to a register row

Method: the root listing (167 entries) walked against the census rows + the F-Appendix family lists + the UI-exclusion line + I-034. **Result: zero NEVER-DISPOSITIONED root files** — the I-034 census verdict ("no undispositioned product surface") holds at the current tree, re-verified by this lane's fresh 1470-file walk (the counts match §11 exactly).

| Root file (task's list → on-disk name) | Register disposition |
|---|---|
| DataFile.ini | row 67 REPORTING-OOS (84-B UTF-16 Aliyun upload cursor; AliyunWrap_decompiled.c:8723/8773/9726 — the DAT misattribution corrected 3-E) |
| AliyunConfig.ini | row 51 REPORTING-OOS + I-027 #3 (batching caps) + I-033 #6 (encrypted OSS credentials) |
| FileFilter.xml | row 87 WRAPPER-VERIFIED (M-023/M-035; engine-side consumption exFATSearchDLL:47102-47560) |
| PEtools.xml (+PE_Tools.dll) | row 100 INSTALLER (WinPE emergency-media builder; FEATURE-TRIAGE "PE format handler" hypothesis REFUTED 3-E) |
| Upgrade.ini | UI-exclusion line :30 + F-INSTALLER list + I-027's sweep (row 78) |
| recoveryFree.ini | row 103 OOS-LICENSING + I-027 #1 (quota contract decode) + I-033 #5 (concrete QWORD layout) |
| dlconfig.xml | row 71 INSTALLER (WAIK byte-range manifest; consumer EuDownloader) |
| auto.txt | row 55 DATA (FVE auto-unlock snapshot; plaintext-key security datum; N-097 owns the WinPE miscitation) |
| needcopy.txt | row 98 INSTALLER + I-033 #2 (true consumer = AliyunWrap crash chain AliyunWrap_decompiled.c:6579) |
| google-services.json | row 88 REPORTING-OOS (with firebasefetch/firebaseonlineconfig/remote_config_data) |
| style.css | UI-exclusion line :30 |
| strutureReg.log / reglog_win.txt / EPMLOG.log / Eaolog.log | row 75 RUNTIME-ARTIFACT + I-033 #7 (dev-build debug logging in retail — "视频重组/19.2回退分支/开发机器") + I-027 (EPMLOG corroborates N-071) |
| unins000.dat/.msg/.exe | UI-exclusion line :30 + F-INSTALLER list + **S-038** (the 13-parameter app-installer contract; ARP QuietUninstallString; unins%03d) |
| SetupUE.exe / InfoForSetup.exe | same sites + S-038 (16-entry OS gate; /SendInfo /SendInfoEx /UninstallStart /UninstallEnd telemetry) |
| Microsoft.VC90.CRT.manifest / .MFC.manifest (+ msvc*90*, msvcp120/msvcr120, msvcp140, ucrtbase, vcruntime140, concrt140) | row 96 + F-RUNTIME (VC runtime generations) |
| send_dmp_with_aliyun.exe (task wrote "Send_…" — on-disk is lowercase) | row 105 REPORTING-OOS (crash-dump uploader) |
| firebasefetch.exe / firebaseonlineconfig.dll | row 88 REPORTING-OOS (DRWUI-only consumer; online config ≠ NAS — 3-C) |
| cloud_session.dll | row 58 EXCLUDED-NAS (cloud recovery OAuth client) |
| EuICloudRecovery.dll | row 110 EXCLUDED-NAS (esicloud) |
| icloud_logger (a FILE, not a dir — ASCII) | EXCLUDED-NAS F-list :283 |
| remote_config_data (a FILE, data) | row 88 + I-027 #2 (json_raw shield) + I-033 #4 (root-vs-PE 36,040-B drift) + I-027 #5 (filter_generalraw 65-ext list) |
| EUinApp.exe | UI-exclusion line :30 ("orphaned IE-emulation bootstrapper — role-corrected in corpus") + I-034 |
| EuUserRate.dll | row 85 OOS-UI (x86 image; persists only unread UI state) |
| PubLog.dll | row 101 WRAPPER-VERIFIED (registered divergence N-048: host owns placement/rollover) |
| EUHwpFilePathCheckerModule.dll / EUHwpViewer.exe | UI-exclusion line :30 + S-050's HWP artifact (`hwp-pathchecker--module-decode.md`) |
| archive.dll | row 54 RUNTIME-OOS (sole importer DRWUI; the engine-side answer landed as M-066 FIXED + S-032) |
| legacy.dll | row 92 RUNTIME (OpenSSL-3 legacy provider on embedded libtomcrypt; .rsf-decryption hypothesis REFUTED) + I-033 #9 (closes N-134's decode: sole export OSSL_provider_init, zero importers) |
| 7za.exe / GDIPLUS.DLL / dbghelp.dll / de265.dll / heif.dll / pdfium.dll / Japanese.dll / lib* (OpenSSL trio, liblzma/libssh2/libx265/libzmq/zlib/zlib1) / Qt5* / api-ms-* (40) / *SearchDLL.dll (9) | rows 50/89/68/69/90/99/91/93/94/UI line/53/56 + F-RUNTIME families (I-034's never-mentioned set maps here exactly) |
| DRW.exe / DRWReport.exe(+.ini) / DRWService.exe / DRWUI.exe | rows 72/73/74 + UI line; M-105/M-108 (DRWService install lifecycle); S-014 (engine side FIXED) |
| EuDownloader.exe / EuOfficeTools.exe / EuOfficeRepairWin32.exe / EUOfficeViewer.exe / PDFViewer.exe / EURawImg.exe / EUImgRepair.exe / EUImageEnhance.dll / EuRepairHost.exe / EUUnZip.exe / EUVideoRepair.dll | rows 78/UI line/108/81/80/82/83/86 (EUVideoRepair.dll: the only repair component an og engine consumes → M-021 CarvePostProcessor hook landed; engine OOS) |
| diskhealth/ (77 entries, 55 MB) | UI-exclusion line :30 ("DiskHealth.exe app payload incl. diskhealth/ QML/Qt/bat scaffolding") + I-027 #4 (addtask.bat ONLOGON HIGHEST autostart) + R-60/R-61 (SMART companion; CrystalDiskInfo lineage) + FB-H01 (multi-transport) |
| iconengines/ + styles/ (root) | UI-exclusion line :30 |
| BUILDPE/ / EUFileRepair/ / VideoViewer/ / SaveScan/ / PartitionRecovery/ / License/ / language/ / Resource/ / ResourceDark/ / EuShare/ / NasConfig/ / platforms/ / imageformats/ / log/ / Dict / EUUserData / config/ | family verdicts F1-F10 above |

**NEVER-DISPOSITIONED root surfaces: NONE.**

---

## Part 2 — The corpus worklog cross-check (807 lines, read in full; first audit lane to do so)

### (a) Corpus-worklog discoveries with NO audit-trail disposition (grep-verified candidates)

| # | Worklog claim (line-cited) | Trail grep result | Assessment |
|---|---|---|---|
| W3b-C1 | **Internal codename "FoxitRestore"** — registry `SOFTWARE\Foxit Software\FoxitRestore` + agent version URL (L87); corroborated corpus-side at `UIUX/02-string-inventory.md:5` ("Mainland_Foxit.qm (19 messages — the "FoxitRestore"-branded variant)") and the Mainland OEM census | `rg FoxitRestore` over core_recovery/ → **0 hits** | A product-identity datum (the China OEM channel's internal brand) with zero trail registration. Maps to the UI/monetization OOS family (the Foxit channel is already described in the dispositioned language census), but the codename itself — incl. the registry identity the agent reads — is nowhere. **Candidate INFO finding (registration).** |
| W3b-C2 | **LM-hash DES NAS-auth material in DRWUI** — "LM-hash DES ("KGS!@#$%") identified in DRWUI (NAS auth)" (L142); corpus: DRWUI-exe--main-ui-architecture.md crypto map (FUN_140c35150/140c352a0, "network auth material (NAS flows)") | `rg "KGS!@#"` over core_recovery/ → **0 hits** | The decode is real corpus ground truth (DES LM-hash = the SMB/NAS credential path). The trail's NAS registration (rows 58/97/110, EXCLUDED-NAS "never reintroduced") covers the family but never cites this specific crypto datum. **Candidate INFO finding (family-extension registration, EXCLUDED-NAS)** — near-zero parity weight since the family is never reimplemented. |
| W3b-C3 | **Shipped-build identity**: "SHIPPED BUILD GIT COMMIT 40504ccb1923e316c9c9a1ffb06907c8307930de, build Oct 13 2026 13:33:43 (3-way consistent w/ version.ini SetupBuildTime=1013)" (L482-483, the vendor QA-night forensics) | `rg 40504ccb` → 0 hits (the trail carries SetupBuildTime — SURFACE-INVENTORY §6 item 27 — and the vendor-logs artifact row, but not the git-commit identity) | Corpus-forensics datum (provenance anchor for every shipped "data" file). No parity weight; would serve future corpus-version pinning. **Candidate INFO (corpus bookkeeping).** |

### (b) Worklog-claimed RE surfaces checked against the trail — all resolve (zero no-disposition surfaces)

Every candidate surface the worklog claims was RE'd was grepped against diff-to-fix/ + docs/. Disposition found for each (evidence type: trail cite):

| Worklog claim (lines) | Trail disposition (verified) |
|---|---|
| .rsf = zlib-chunked not CryptoAPI; records 1-3 schemas; record-4 grammar; type-0xB never persisted (L122/128, L225-232, L304-329) | modules/session.md (tree grammar, 0xB rule, progress formulas); row 104; row 92 (legacy.dll refutation); N-131; §11 s10-rsf-ground-truth |
| DRWCfg.bin AES-256-CBC hardcoded key XZJE151628AED2A6ABF7158809CF4F3C / IV XZJ2030405060708 (L133-134) | **R-57** REFERENCE-API-GAP.md ("AES-256-CBC config cache (hardcoded key)"), **D-60f** PARITY-DELTAS, **I-019** modules/orchestration.md |
| VAPreviewListener IPC TCP 127.0.0.1:20171/20172 + zmq REP; 602-B header; 1 MiB BMP; 10 s (L138, L707-708) | modules/preview.md + modules/visualization.md (ports + protocol verbatim); S-045; 3-D protocol bounds |
| EUEDKEPM identity = EuDskAcs.sys "Disk Access Driver", Todo-Backup 5.8 lineage, dormant (L218-224) | docs/DRIVER-CONTRACT.md (RDA merges EUEDKEPM-as-EuDskAcs), docs/DRIVER-UTILIZATION.md ("dormant deployment, zero in-tree openers") |
| EuShare SSH actor = runtime-downloaded connect.exe (CLinuxDownloadConnectZipThread → EUUnZip → NasConfig\connect.exe); libssh2 0 importers (L233-240) | row 83 EUUnZip ("(b) NAS-agent installer path (connect.zip → NasConfig\connect.exe)"); EXCLUDED-NAS family; row 94 libssh2 RUNTIME |
| EUDCPEPM 15-command custom IOCTL protocol; CSession; urgent-IRP (L330-341, L663-667) | docs/REFERENCE-DRIVER-BEHAVIOR.md (GET_VERSION "EASEUS Todo Backup Driver", UpperFilter, IoctlBase), DRIVER-CONTRACT (EuBackup.sys merge), S7 lane rows |
| RAWSearchDLL analyser census (182 RTTI / 106 classes / 70 magic keys); ZIP 5.24 MB padding cap (L394-396, L436) | modules/carve.md + the six family rows (S-007/A-010/S-037/…); PARITY-CORE-REC + cr_analysers.cpp + W2 CARVE-2 (5.24 MB cap verbatim) |
| FS-type factory codes 0-2 FAT/3 NTFS/4-5 EXT2/7 exFAT/8 HFS/9 EXT4/10 ReFS/13 ZFS (L88) | **R-10** REFERENCE-API-GAP.md (the identical map) |
| Advanced scan gated by settings+0x60 (L89) | SCAN-ENGINE-DELTA A1/A5 + modules/orchestration.md (settings+0x60 verbatim) |
| EuPEMaker = WinPE builder inside DRWUI (L86) | **R-92** REFERENCE-API-GAP.md + row 100 (PE_Tools/CPECreateThread) |
| RecoveryAssistant category-ID table 5..0xB = IFileProperties+0x60 space; CRecoverability grading (L703-706) | **R-26/R-27**, SCAN-ENGINE-DELTA C4, S-023 (cr_grading.cpp + tests) |
| PST/RAR4/MKV/HEIC analyser grammars (L709-712) | PARITY-CORE-REC F-A3-5 (PST byte-exact constants), CAPABILITY-MATRIX carve row, S-007 family |
| ZFS 0xBAB10C txg-gated uberblock walk; $MFTMirr fallback (L672-678, L713-714) | modules/zfs.md B1 (magic verified 4 decompile sites); modules/ntfs.md N-001 |
| NTFS deleted-record viability scoring ($Bitmap ALL-reused destroy rule etc.) (L672-674) | modules/ntfs.md (ntfs-engine-algorithms-v2--deleted-viability.md cited; viability_min_size=0x3FF) |
| Vendor QA-night forensics: EPMLOG/Eaolog/logs; I: scan session Seagate ST500DM002 StartSector 705794048 (L482-488) | I-027 (row 75 + corroborations), s3-calcpartition-scoring.md (705794048 verbatim), I-033 #7 |
| SetupUE/InfoForSetup installer pair, 16-OS gate, /SendInfo, DRW_Installer = EuSendMessage target (L464-465, L599-601) | S-038 (13-parameter contract + string-table evidence), row 109 (EuSendMessage WM_COPYDATA cmd 999) |
| Retention gates (RetentionExitDlg / RetentionActiveDlg periodic nag) (L599-601, L785-787) | PARITY-DELTAS (retention-nag row, A35 §12.15), PARITY-DELTAS-draft ("RetentionActiveDlg … uninstallretain" experiments); S-038 K-telemetry set |
| ChronoSnap "Time Machine for Windows" sidebar product (L788-791) | PARITY-DELTAS ("ChronoSnap backup product (P02e/P40)" registered exclusion) |
| DiskHealth CrystalDiskInfo lineage, CAtaSmart multi-transport, USB bridge tables (L435-436) | **R-60/R-61**, AUDIT-FALLBACKS FB-H01, REFERENCE-DRIVER-BEHAVIOR, modules/health.md |
| Engine cross-verification 9/9 factory uniformity; shared-props-in-engines (L491, L687-689) | I-016/D-21; docs/architecture.md ("shared-props hard links") + CORPUS-ERRATA (the shared-props finding) |
| MOM deepening: CMessageProxy, import matrix, 26 factory plugins vs 5 link-time libs (L559-566) | modules/mom-capabilities.md (MOM system AUDITED; D-59i deliberate non-port; the wave-7 stub spot-check) |
| Language family + QM string tables (L52, L427-428) | I-034 Languages row; UI-exclusion line; UIUX/02 dispositioned via I-034's UIUX rows |
| RCC extraction 3,722/3,657/125 files (L101-105) | UI-exclusion line + I-034 (drw_light.rcc named) |
| PE payload census 180 files, 0 .mo/0 .rsf (L389-393) | row 57 + §5 WinPE row (my count: 180 exact — the census holds) |
| Dynamic ground truth (splash→landing→scan→complete→results→retention) (L397-402, L437-440, L508-511, L596-623) | I-034's three dynamic-ground-truth artifacts row ("corpus-methodology artifacts — no parity surface"); the two-phase scan model + auto-rsf corroborations live in modules/orchestration.md + I-027 |
| EUinApp role correction — orphaned IE-bootstrapper, NOT purchase host (L288) | UI-exclusion line :30 ("role-corrected in corpus") + I-034 row |
| RawFixer v2 33-method inventory (L699-702) | modules/rawfixer.md ("S-008 evidence upgraded to the full 33-method og table") + AUDIT.md row 36 — **but census row 157 still says 31** (see E2) |
| iWork IWA correction (Office protobuf claim corrected) + PDF xref synthesis (L567-575) | S-050's corpus set (deepening-batch2--pdf-office-repair.md is S-050-cited); repair line OOS |
| Image-repair grammars/state machines incl. PNG/WebP/HEIC/GIF/BMP/SVG/TIFF (L241-246, L342-360, L543-551) | S-050 (+ the S41 addendum for UIUX/32) |
| Btrfs=ZLIB+RC4, exFAT=XOR+FNV capa digest (L74) | OQ-S6-2 (btrfs-rc4-prga-target) FIXED-ledger row; modules/exfat.md |
| .rsf record-4 empirical validation 1,986,806 nodes (L323-326) | modules/session.md tree-grammar section + §11's first-hand parse |

**Result: zero worklog-claimed RE surfaces lack a trail disposition.** The three items in (a) are data/identity-level discoveries, not surfaces.

### (c) Contradiction check (worklog vs trail) — none material; three near-misses cleared + two drifts filed

1. **DRWService pipe direction** — worklog L135-137: "service is pipe CLIENT, DRWUI is server". Trail rows 55/74: DRWUI `CreateNamedPipeA` (server), DRWService the elevation broker client. **CONSISTENT** (no contradiction).
2. **RegLib "hbin WRITER"** (worklog L680) vs row 160's "og hive-write consumers are read paths; hbin write-allocator has no decoded og consumer" — **CONSISTENT** (the writer exists inside RegLib; no og *consumer* of it was decoded — the row's phrasing is precise, not contradictory).
3. **EUinApp role** (worklog L288) vs UI line :30 — **CONSISTENT** (the line carries the correction).
4. **Retention-nag period drift (W3b-E3, cosmetic):** worklog L785 says the nag "fires ~10-60s"; PARITY-DELTAS' retention-nag row says "every 30–60 s". Both describe the same RetentionActiveDlg; the numbers differ (the trail's likely source is the corpus artifact rather than the worklog). OOS-UI either way — errata-note candidate, no parity weight.
5. **RawFixer 31→33 (W3b-E2, trail-side):** the corpus worklog itself drifted (L163 "31-method complete inventory" → L699 "complete 33-method vendor inventory"); the trail's module docs adopted 33 correctly, but census row 157 kept 31. **Not a corpus-vs-trail contradiction — a stale register row.** (Filed as E2 above.)

### (d) Worklog facts that corroborate trail rows (spot list)

- L485's "quick+deep StartSector 705794048" ↔ s3-calcpartition-scoring.md (verbatim).
- L204's Actions-minutes exhaustion ↔ I-034's dynamic-ground-truth capture-methodology rows.
- L115/L280 repo migration heictojpgpics→heicfast ↔ corpus provenance (the trail's corpus pins be0cfe9/03ad46c are post-migration).
- L131-132/L228-232 the .rsf sample facts ↔ modules/session.md + s10-rsf-ground-truth.md.
- L512-515 (Japanese.dll lang-1041 satellite; WinChkdsk mod.winchkdsk + VSS; setupdrvx64 WinDiskAccessDriverSetup %s%d; legacy.dll libtomcrypt) ↔ rows 91/165/161/92 verbatim.

---

## The findings list for the parent lane (all read-only; no repo file touched)

| ID (proposed) | Class | One-line | Evidence |
|---|---|---|---|
| W3b-C1 | INFO (registration) | The "FoxitRestore" internal codename/product identity (registry `SOFTWARE\Foxit Software\FoxitRestore`, agent version URL, the Mainland_Foxit OEM channel brand) has zero trail registration | worklog L87; UIUX/02-string-inventory.md:5; `rg FoxitRestore core_recovery/` → 0 |
| W3b-C2 | INFO (family-extension) | The LM-hash DES ("KGS!@#$%") NAS-auth decode in DRWUI is uncited; ride the EXCLUDED-NAS registration | worklog L142; DRWUI-exe--main-ui-architecture.md crypto map; `rg "KGS!@#"` → 0 |
| W3b-C3 | INFO (corpus bookkeeping) | The shipped-build identity (git commit 40504ccb…, build 2026-10-13 13:33:43, SetupBuildTime=1013) is corpus-worklog-only; the trail carries SetupBuildTime but not the commit | worklog L482-483; `rg 40504ccb` → 0 |
| W3b-E1 | CORPUS-ERRATA candidate | Languages census count defect: "8 qtbase" vs **11 qtbase files on disk**; the 25+4+8=37 split holds only by coincidence (actual 22+4+11) | first-hand ls count; language-packs--family-census.md §1 + Languages/README.md:3 |
| W3b-E2 | trail numeric defect (N-129 class) | PARITY-OG-CORE-INVENTORY row 157 still says RawFixer "31-method inventory"; the trail's own module/AUDIT/code docs carry 33 | row 157 vs modules/rawfixer.md + AUDIT.md:36 + worklog L163→L699 drift |
| W3b-E3 | errata note (cosmetic) | Retention-nag period: worklog "~10-60s" vs PARITY-DELTAS "30–60 s" | worklog L785 vs PARITY-DELTAS retention-nag row |

**Unverifiable items (explicitly marked):** none material — every claim above was verified against a file read or a command run this session. The only execution-bound item inherited from the trail (not re-run here): M-087's `reg import` transcript (Windows-bound; the encoding violation is statically decidable and the og-side regdrv.reg was read by the prior lane, not re-decoded here beyond M-087's own citation).

**Bottom line:** all nine families + the root loose-file set are **COVERED** by existing register rows (re-verified, not trusted); **zero NEVER-DISPOSITIONED product surfaces** found in this sweep (consistent with I-034's census, now independently reconfirmed at 1470 files); the corpus worklog yields **3 uncited data-level discoveries (C1-C3)**, **2 count drifts (E1-E2)**, **1 cosmetic numeric drift (E3)**, and **zero un dispositioned RE surfaces and zero material contradictions**.
