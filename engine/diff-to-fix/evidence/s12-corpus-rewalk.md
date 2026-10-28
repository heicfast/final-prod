# S12/C — Corpus behavioral surface re-walk (CORPUS BEHAVIORAL RE-WALK)

**Agent:** audit sub-agent C · read-only parity audit
**Date:** this session · corpus walked at `/home/z/my-project/audit-work/easeus-drw-research` (re-workspace/artifacts/, og-codebase/) · trail = `/home/z/my-project/audit-work/core_recovery/diff-to-fix/` (+ docs/ registers)
**Mandate:** re-walk the focus families whose behavioral content is engine-relevant but whose trail disposition is thin; surface documented og behavior with no core counterpart AND no finding. Method per family: read every artifact, extract every concrete behavioral claim (constant / algorithm / threshold / state machine / field / observable), grep the trail from multiple keyword angles, verdict each claim.

**Verdict vocabulary:** DISPOSITIONED-<where> · NOT-DISPOSITIONED (grepped from multiple angles, zero hits) · OUT-OF-SCOPE-<basis> · FIX-PLAN-GAP (the owning finding exists but its fix plan does not cover the documented behavior).

---

## Family 1 — DiskHealth (priority 1)

**Artifacts read (complete family):**
- `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md` (40 lines — the only artifact)
- Companion docs pulled in for the SMART surface: `artifacts/UIUX/19-diskhealth-quick-ui.md`, `og-codebase/diskhealth/` (file tree incl. addtask/deltask/querytask.bat, AliyunConfig.ini), `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` §2E, `artifacts/per-binary/DiskHealth.md` (no polling constants documented).

**Claims census (24 behavioral claims) and verdicts:**

| # | Claim (cite) | Verdict |
|---|---|---|
| 1 | CAtaSmart multi-transport accessor set: `GetSmartAttributeSat -1A/-2/-2A`, `GetSmartAttributeCsmi`, `GetSmartAttributeNVMeRealtek`, Si (artifact:19) | DISPOSITIONED — B1 (modules/health.md:13-17), N-032 FIXED (CSMI + USB-bridge + Realtek-NVMe accessors; SUMMARY:4936, cr_health.h:4-9) |
| 2 | USB bridge database (`AddDisk - ASM1352R/Si/USB0A…USB20`, artifact:11) | DISPOSITIONED — N-032 FIXED (USB-bridge accessors) + D-78 (RAID-census routing/LSILOGIC sibling registered remainder) |
| 3 | SMART grading model: 4 statuses, vendor families, SSD/HDD gates, 5/197/198 caution, NVMe path, temp-never-graded | DISPOSITIONED — S-024 FIXED (s6-health-grading-decode.md, full rule set) |
| 4 | ATA thresholds read (0xD1 READ ATTRIBUTE THRESHOLDS) + Threshold column/firstCautionIndex (UIUX/19:26-27) | DISPOSITIONED — N-055 (health.md:88-93; S-024 C23 MATCH :109) |
| 5 | NVMe field surface (host r/w, controller-busy, error-log, data-units ×512,000, percentage_used u8) | DISPOSITIONED — M-036 FIXED (health.md:145-152), N-009a |
| 6 | NVMe transport route (IOCTL 0x2D1400, {proto 3, type 2, page 2, 512B}) | DISPOSITIONED — I-018 |
| 7 | SMART attribute row surface (id/value/worst/raw/threshold/name; 12-byte entries) | DISPOSITIONED — B1 + session-30 Q65 name-table pin |
| 8 | `Parallels Virtual NVMe Disk` early-Good rule | DISPOSITIONED — s6-health-grading-decode.md:93 |
| 9 | SSD/TRIM pairing with DRWUI warnings | DISPOSITIONED — M-076 (TRIM facet) |
| 10 | Bad-sector semantics (skip-bad-sector flows, "Program is trying to skip bad sectors") | DISPOSITIONED — F60/S60 (storage.md:28), N-073 (ext4 CatchBuffer), ntfs.md:111 |
| 11 | BitLocker auto-unlock (FVEAutoUnlock live+offline, Dict dictionary) | DISPOSITIONED — S-014 FIXED, N-064 FIXED |
| 12 | BitLocker turn-off (CBitlockerTurnOffOperation) | DISPOSITIONED — M-078 |
| 13 | auto.txt FVE key snapshot | DISPOSITIONED — N-097 |
| 14 | autostart (schtasks ONLOGON HIGHEST via addtask.bat) | DISPOSITIONED — I-027 item 4 |
| 15 | Boot-media deployment (auto.bat Phase E copies DiskHealth + api*/aliyun* + .lnk) | DISPOSITIONED — K78 (PARAMETERS:168), SURFACE-INVENTORY §5/§6 item 24, boot-config §2E |
| 16 | Aliyun telemetry DLLs (api*.* + aliyun*.*) | DISPOSITIONED — I-027 item 3 (caps), N-048 telemetry-OUT, inventory row 51 |
| 17 | Interface model strings ("ATA/SATA/SCSI Disk Device", INTERFACE_TYPE_NVMe) | DISPOSITIONED-weak — B1 transport labeling (core `transport` field) + UI-pure display naming (inventory :30 UI exclusion) |
| 18 | CSMI wire format (CC-0x19, Register-FIS task file, response @+0xA4) | DISPOSITIONED — N-032 session-30 leg (health.md:171) |
| 19 | Legacy SMART_RCV_DRIVE_DATA 0x7C088 route + IDEREGS stamps (count 1, sector 1, cyl 4F/C2) | DISPOSITIONED — health.md:158-165, cr_health.h:11-13 |
| 20 | **RefreshThread — background poller (periodic SMART refresh; "always-on monitoring", "tray-resident guarding", "Display alert when disk problem are found. (%1)")** (artifact:20; UIUX/19:11,26,34) | **NOT-DISPOSITIONED** — grepped: refresh/periodic/poll/interval/monitor/guarding/always-on → no row for a resident SMART monitor loop (N-062 is the DRWUI memory monitor; API-DELTA row 24 covers only the data contract `cr_ui_health_read`). Product-frame capability with no implementation and no registered exclusion (the S-050 class, small). No cadence constant is documented anywhere in the corpus. |
| 21 | **Notification IPC: "Click_Open_DiskHealth_By_Notification" + "EaseUS_DiskHealth"/"…Console" window names — the console opens from a Windows health alert** (artifact:28) | **NOT-DISPOSITIONED** — zero trail hits for the string or the notification-launch surface (product-frame; no UI exclusion row names it). |
| 22 | CTrayIcon tray integration, "(the auto.bat `trayicon` mode)" (artifact:21) | NOT-DISPOSITIONED-as-stated — but the artifact's own parenthetical is a **misattribution**: auto.bat's `trayicon` argument is the changeres resolution applet (boot-config:87 `launch.exe 1 changerres.exe trayicon`), not DiskHealth. Corpus-errata candidate; the tray itself is UI-pure (UIUX/19:11 close-hides-to-tray). |
| 23 | AttrModel QML table model / Qt RemoteObjects model sync | OUT-OF-SCOPE-UI (runtime-families; inventory :30) |
| 24 | 840×590 frameless QML window, 32 strings, palette | OUT-OF-SCOPE-UI (UIUX/19) |

**Family verdict:** engine-side surface (transports, grading, thresholds, NVMe, BitLocker, bad-sector) is fully and deeply dispositioned — the prior sessions' "CLEAN" claim holds at the data-contract level. The residue is the companion app's **resident monitoring mode** (claims 20-21): a product-frame capability neither implemented nor registered.

---

## Family 2 — FileRepair (priority 2)

**Artifacts read (8/8):** `repair-architecture.md` (176), `image-repair--format-grammar.md` (287), `image-repair--state-machines.md` (84), `image-repair--gif-bmp-svg-tiff-walks.md` (221), `archive-repair--7z-engine.md` (53), `x3f-repair--section-checkers-walk.md` (39), `deepening-batch2--pdf-office-repair.md` (168), `hwp-pathchecker--module-decode.md` (31).

**Disposition level — CLEAN.** Every capability-level claim is owned:
- The line-level absence + false registration: **S-050** (SEVERE, cites all 8 artifacts + the 10 binaries + remote-config flags).
- Engine-side integration points: EUVideoRepair per-chunk/per-finalized feed → **M-021 FIXED** (CarvePostProcessor) + N-024; `useOrgFsDatarun` datarun-choice lives inside EUVideoRepair (S-050's umbrella).
- Repair eligibility: CHK chain (`CHKRepairFileSign`/adapter registry) → S-050 corpus refs + SCAN-ENGINE-DELTA F2 + CORRECTIONS #9; Pro-gating + `EuRepairHost.exe` → **R-78** (REFERENCE-API-GAP:139) + inventory :82; `enable_imgrepair`/`enable_docrepair` flags → **I-027**; repair UI flow surfaces (wizards/progress/stop/compare/save) → **R-72..R-78** (REFERENCE-API-GAP:301 "GAP (file-content repair)").
- Binaries: EUAdobeRepair/EUArchiveRepair/EUFileRepair-bundle/EUImageEnhance/EUImgRepair/EUUnZip/EURawImg → inventory 3-B SIDECAR-OOS rows (:76-:86); VideoFileRepairer.exe → **N-130**; sfc payload → OQ-S10-2 (closed).
- Dict/password pairing (CheckPassWord vs CheckPassWordByData; -mhe filename vs body errors) → S-050 fix-plan step 3 + N-064.
- `FileRepairSavePath` output location (repair-architecture.md:79; config-data-reference.md:52) → DISPOSITIONED as config.ini UI-bulk OOS (inventory :60 cites config-data-reference.md:49-55 which contains the key).
- Cloud-assisted repair (AdobeRepair `NETCLIENT::CClient::ConnectServer`, ConvertPsdbToJpeg; ImageRepair net/client.cpp advanced-repair upload) → DISPOSITIONED-weak — inventory :76 registers EUAdobeRepair as "PSD/PSB, cloud-assisted" SIDECAR-OOS (binary-level), but no capability-level row addresses the repair line's cloud sub-paths (CAPABILITY-MATRIX §3's Cloud exclusion covers cloud *recovery*).

**S-050 fix-plan coverage check (the mission's question) — 5 gaps:**

| # | Documented repair-chain behavior | Fix-plan coverage |
|---|---|---|
| 1 | **Video repair** — the deepest documented algorithm (EUVideoRepair.dll: weighted atom grammar, two-scan, moov triage, sample-file transplant via CRepairVideoGetSampleWnd, `useOrgFsDatarun:%d usergedDatarun:%d`, HD rollback, fragment limits; VideoFileRepairer.exe -r/-a CLI) — repair-architecture.md:24-46/§B | **FIX-PLAN-GAP** — S-050's title/summary include video, but fix-plan step 2's build order enumerates image → 7z → office/PDF → X3F/HWP and **omits video entirely**; acceptance names only RepairImage/Repair7z/RepairOffice. N-130's SIDECAR-OOS row covers the register, not the build decision. |
| 2 | Repair-eligibility signature classification (CHKRepairFileSign reads the EUUserData signature DB; FileFilter.xml categories drive the repair context action) — repair-architecture.md:62,74,82 | FIX-PLAN-GAP (thin) — "integration: repair-in-the-recovery-flow through the ABI pair" gestures at it; the classification seam (signature DB → adapter dispatch) is not named as a step. |
| 3 | Output naming/location ("Output saved to FileRepairSavePath (config.ini [User])"; repaired-copy naming) — repair-architecture.md:79 | FIX-PLAN-GAP (thin) — no output-naming/path contract in the plan or acceptance. |
| 4 | Cloud-assisted sub-paths (Adobe server-side conversion; image advanced-repair upload) — repair-architecture.md:49-53; image-repair--format-grammar.md:149-150 | FIX-PLAN-GAP — the plan's step-1 product decision does not carve out the cloud-dependent halves; the standing Cloud exclusion is scoped to recovery. |
| 5 | Per-format progress contracts (PNG 20/30/50/80 + sample 15/25/61/85; BMP 20/50; SVG 55/66/76; PDF offset×80/file_size; phase code 0xD; _MsgData callback family) — state-machines §2/§6, gif-bmp-svg-tiff §5, 7z-engine §4 | Covered-by-artifact-reference — the plan names "the progress callback seam" only for 7z; the per-format percent maps are buildable from the artifacts but not enumerated; no progress-contract acceptance test. |

Also noted: the iWork IWA subsystem (deepening-batch2 §2.1 — Apple Numbers/Pages/Key repair via iwk:: ZIP + protobuf + Snappy) is covered only by the plan's "office/PDF (the deepening-batch2 decode)" phrase — the iWork format family is a distinct surface worth naming if (a) is chosen.

---

## Family 3 — SaveScan (priority 3)

**Artifacts read (5/5):** `rsf-format--scan-session.md` (494), `rsf-record4--tree-node-grammar.md` (121), `ifileproperties-part1--ntfs-fat-exfat.md` (189), `ifileproperties-part2--ext4-hfs-refs.md` (138), `ifileproperties-part3--btrfs-zfs-raw-generic.md` (141).

**Claims census (~60 claims) — verdict summary:**
- Container/chunking (magic GUID, 0x8C header, 0x24 record headers, SessionId chain-walk, 4 KiB chunks, level-5 deflate, store-if-smaller, raw fallback, `78 5E`): DISPOSITIONED — session.md chunk-framing M + F10 + **s10-rsf-ground-truth** (executed on the 3 real .rsf files).
- Record 1 (state + special-partition entry list + WinPE variant): DISPOSITIONED — session.md record-inventory M + **S-051 RESOLVED note** ("the multi-volume registry is record-1's special-partition list… record-level equivalent") + s8-autosave-and-stamps §4 (list-path record-1 read + WinPE-style variant).
- Record 2 (sig 0x7F, volumeName[100], deviceType + name map, SYSTEMTIME, stat1/stat2, flags, WinPE variant): DISPOSITIONED — session.md + N-090 (device-type/row shape) + M-079 (label) + wave-2 WinPE-variant note.
- Record 3 (partition struct 0x6F5 + 5400B device blob: bus type, model, serial, firmware, WWN, interface, 512-B boot snapshot): DISPOSITIONED — N-066 FIXED (v4 tail), N-049, session.md.
- Record 4 tree grammar (marker classes, wrapper fields f_48/f_6c/f_68/f_71/flag_72/73/91, stat blob, root roles, vector children, 0xFFFFFFFF sentinel, GetType()==0xB never-saved, dual child structures): DISPOSITIONED — M-050 FIXED + N-058 FIXED + **s10-field-parity-table** (the completeness record: every load-bearing family MATCH, 13 SUPERSET, the full ABSENT enumeration) + M-047 (stats) + M-041 (carved deleted flags).
- Per-FS content fields (all 10 classes; FAT 8.3 OEM bytes; anchors FAT+0x3e/exFAT+0x50/NTFS+0x64; NTFS +0x60 twin; parent anchors +0x40/+0x42/+0x54; Btrfs dual-runlist +0xB5; ZFS polymorphic runlist; HFS+ named-fork extents; ReFS +0x39 [refuted as flag-bundle, E69]; CRAW +0xF4 boolean [OQ-S6-3 s25 decode] + carve-provenance block): DISPOSITIONED — N-087, N-059 FIXED, N-100, N-131, OQ-SA-1 landings, parity table.
- Progress formulas (save 0x28+processed×0x32/(total×2), restore 40+50·done/(total+done), per-child IsCancelled): DISPOSITIONED — session.md progress-semantics M-note + N-060 (save progress/cancel) FIXED via S3-W1b.
- Auto-save naming/sanitizer/folder/auto_* enumeration: DISPOSITIONED — N-061, N-090, N-116, S-044 FIXED (trigger gates), s8-autosave-and-stamps.
- AES_CBC256/DRWCfg.bin (hardcoded key `XZJE151628AED2A6ABF7158809CF4F3C`, 0x80 records): DISPOSITIONED — I-023 FIXED + R-57.
- LM-hash DES generator ("KGS!@#$%", NAS/SMB auth): OUT-OF-SCOPE-basis (DRWUI UI-process + NAS — both registered exclusion families) — but no row names it; noted here, not filed.

**NOT-DISPOSITIONED (1):**
- **The 5th record-type: `DAT_1414d47a0` = GUID `76a672b048058b46ba9e91093a63b517` drives a record written by FUN_1403f1180 = `[552-byte struct] + [5400-byte blob]` — "used by a variant save path, not present in the shipped auto-save samples"** — rsf-format--scan-session.md §2.2 (the record-ID table, "A 5th constant…" row). Greps: `1414d47a0 / 76a672b0 / 1403f1180 / 552 / variant save / 5th record / 0x228` → zero hits across diff-to-fix/, docs/, src/, include/. The session.md record inventory maps records 1-4 only; the core's crsf has 9 record types of its own (nSESSION_STATE..nFS_CHECK) with no variant-partition-context concept. A documented record-type with an undecoded trigger and no disposition. INFO-class candidate (session-format completeness; the 5400-byte blob matches record-3's volumeDevice size, so it is plausibly a variant partition-context writer — the fix-time probe is one decompile function read).

**Family verdict:** the session format is the most thoroughly dispositioned surface in the audit (executed ground truth + a per-field completeness record). One thin variant record-type remains.

---

## Family 4 — MOM-modules (priority 4)

**Artifacts:** 33 files read/sampled (all 30 per-module stubs + `mom-modules--ui-bridge-and-dependency-matrix.md` + `module-internals-wave2--ldm-reglib-ctbfile.md` + `CallbackOperator/CheckVersion/…`).

**Findings:**
- The per-module stubs (Device, DeviceAdapter, Thread, Error, Config, Log, Partition, FatFormat, Fixup, FileSystemCheck, analysers, …) are **structural censuses only** (RTTI class lists, string tables, function counts — no constants, no algorithms). The board's wave-7 "rollup-covered, zero new parameters" claim and the S10-F binary-level re-verification (mom-capabilities.md:186 "27 name-cited, 3 infra-only") hold under this re-read. Notable strings map to owned rows: `\\.\EPRDTDRV\%d\%d` (transport.md), MSFT_* WMI namespaces (N-071/D-80 five-stack), `\\Registry\\Machine\\BCD00000000` + `\\ArcName` + BcdLog.txt (M-040), `\\EPMLOG.log` + `\\HD%03d.dpt` + CMemMonitorManager (N-048/N-061/N-062; s5-image-fscheck-persistence:206).
- LDM VMDB/TOCBLOCK/PRIVHEAD parser chain (module-internals-wave2 §1): DISPOSITIONED — modules/ldm.md + S-052/M-017 family.
- RegLib **reader** (OpenHiveByVolumeW, regf+checksum gates): DISPOSITIONED — S-014 FIXED (cr_reghive).
- CTbFile/ITbFile ".tbi image wrapper" claim: DISPOSITIONED — **refuted** by s5-image-fscheck-persistence.md:14-21 (CTbFile is a boot.ini wrapper, not an image abstraction; edisk registered in PARITY-DELTAS:1994).
- CheckVersion offline-OS detection: DISPOSITIONED — OQ-S5-1 RESOLVED (dead-shipped) + M-082 (the live DeviceManager copy).
- bitlockerhelp.mo string census (`\\.\root\CIMV2\Security\MicrosoftVolumeEncryption` WMI route; `bitlockerwizard.exe %1 U/%1 Y`; `C:\Program Files\EaseUS\BL`; temp.dat): capability-covered (S-014/M-078/N-064 own detection/unlock/turn-off), but the WMI-detection route and wizard-invocation strings are unnamed mechanism variants — noted, not filed.

**NOT-DISPOSITIONED (1):**
- **RegLib hive-WRITE — the hbin (0x6E696268) block allocator: stamps `_time64()` timestamps, computes 0x1000-granular bin sizes (`((size >> 12) + 2) * 0x1000` with edge cases), zeroes new bins — "an hbin allocator for hive modification, not just parsing"** — module-internals-wave2--ldm-reglib-ctbfile.md:73-78. Greps: `hbin` over diff-to-fix/ hits only the core's read-side comments (cr_reghive.cpp:10-12 — a read-only parser); no trail row dispositions an offline-hive *write/repair* capability. The named consumers in the artifact (CheckVersion, BitLocker FVE lookup) are both read-side; the write half has no named live consumer — but the capability is documented og behavior with no core counterpart (core is deliberately read-only) and no finding. INFO-class candidate (registration row: "og RegLib writes/repairs hives; core read-only by design").

---

## Family 5 — config-files (priority 5)

**Artifacts read (6/6):** `config-data-reference.md`, `drwcfg-bin--aes-config-cache.md`, `filtering-priority--config-layer.md`, `qa-logs--session-forensics.md`, `userdata-remoteconfig--signature-and-flags.md`, `vendor-logs--qa-session-forensics.md`.

**Verdicts (all claims):**
- raw_signs.json (139 ext, header+tail signs, confidence 0.50-1.00, min/max): DISPOSITIONED — S-037/A-010 family + s3-rawsigns-consumer-decode.
- EUUserData (111 entries, user-extensible): DISPOSITIONED — S-039/M-065/OQ-SA-3 (SURFACE-INVENTORY §2 wave-5 row).
- Dict (228 entries): DISPOSITIONED — N-064 FIXED (D-78 authored seed, format-compatible loader).
- FileFilter.xml (8 categories/1489 ext/Selected states/iconIndex): DISPOSITIONED — M-023/M-024 + s3-filefilter-health.
- FilterFolder.ini (blocklists, localized FatFilterName ×10 locales, TxtViewFiles 119 ext, SumaryFileType groups): DISPOSITIONED — M-018 + N-132 + M-108 + the 25/25 byte-exact re-verification (SURFACE-INVENTORY §11).
- usefullostfiles.xml / PriorityDisplayType.xml / shellexe.ini / themdark.ini: DISPOSITIONED — inventory :30/:273 display-tier OOS; PriorityDisplayType+TxtViewFiles claimed-landed-but-absent surfaces owned by N-132+M-108.
- RawConfig.ini (34 toggles): DISPOSITIONED — M-024/N-102 (count corrected).
- config.ini ([User] only_common engine key; UI bulk incl. FileRepairSavePath/VideoVolume/ENS/BadSectorTip): DISPOSITIONED — inventory :60 WRAPPER-VERIFIED.
- DRWCfg.bin AES cache + DRWUI crypto inventory: DISPOSITIONED — I-023 FIXED + R-57 (the xxhash/LM-hash/Schannel entries are DRWUI-internal, UI OOS).
- remote_config_data flag cache (env string `abroad__free__easeus__21000__1013`, ~600 flags, suffix segmentation, shipped-default fallback): DISPOSITIONED — I-027 (the fetched-cache decode + the json_raw shield).
- PEtools.xml/dlconfig.xml/nasconfig.ini/AliyunConfig.ini/google-services.json/DataFile.ini/Upgrade.ini/recoveryFree.ini/unins000.*/style.css/LICENSE: DISPOSITIONED — 3-E/I-027/S-038/inventory rows.

**NOT-DISPOSITIONED (1):**
- **The Windows Explorer context-menu integration — shellexe.ini is "Consumed by the Windows Explorer context-menu integration ('Recover any deleted data' verb on files/folders — the shellexe component referenced by DRW's install)", 25-locale taglines** — filtering-priority--config-layer.md:60-62. Greps: `shellexe / context menu / shell verb / Recover any deleted data` over diff-to-fix/ + docs/ → only the inventory's display-tier file listing (:30/:273 — the *string file*, not the capability). The §6 installer ground-truth enumeration (27 items, SURFACE-INVENTORY:104) contains no shell-extension registration item; S-038's installer contract does not enumerate it. The capability (a shell verb registering the product into Explorer) has no finding and no register row. INFO-class candidate (product-frame registration, same bookkeeping class as N-134).

---

## Family 6 — og-codebase runtime logs (priority 6)

**Files walked first-hand:** `log/DRWUI.log` (691 lines), `log/CalcPartition.log`, `log/DefaultLog.txt`, `log/DRWLog.log` (empty), `EPMLOG.log`, `Eaolog.log`, `strutureReg.log`, `reglog_win.txt` — plus the two forensics artifacts that summarize them.

**Log-CHANNEL level — DISPOSITIONED:** N-048's channel census (+ the Eaolog 4th-channel note and BuildRaw/DelFat engine channels, SURFACE-INVENTORY §8), the inventory RUNTIME-ARTIFACT row (Eaolog/EPMLOG/reglog_win/strutureReg/log/), I-027's corroboration register (EPMLOG→N-071; DRWUI.log auto-rsf names→cr_crsf:363-404; CalcPartition.log→M-017 verified against all 187 rows).

**Behavioral-OBSERVABLE level — verdicts on everything distinctive found in the raw logs:**

| Observable (file:line) | Verdict |
|---|---|
| Build identity (git commit `40504ccb…`, build date, env string) | DISPOSITIONED — qa-logs artifact §1 (evidence datum) |
| Scan flow: doScan → PushScanDevice → PushFolderFilter → DoScan → AdvanceFilter → ScanThread type=0/1 → UpdateScanFinishedInfo; quick StartSector == deep StartSector | DISPOSITIONED — orchestration module + S-051's flow decode |
| "Insert lost partition … genericrecoverycontroller.cpp :130/:737/:162" + "Found lost partition(…)" | DISPOSITIONED — S-051's own evidence (the 0xa8-stride insertion loop) |
| Three auto-saves (`SaveResultPath = …auto_Local Disk (…)….rsf`) ~11s after "Build tree finish." | DISPOSITIONED — S-044 (completion trigger) + I-027 (names) |
| `m_bNeedViedoRepair = 0/1`, `m_pVideoRepair = 1` | DISPOSITIONED — M-021 domain (the EUVideoRepair binding state) |
| `OfficeTmp=D:\OfficeRepairTmp` created at scan start | DISPOSITIONED-weak — S-050's line-level umbrella (repair chain temp dir) |
| `VAPreviewListener::SyncWait4DataBack:397 failed got socket` | DISPOSITIONED — preview module domain |
| CameraDeviceManager classification + `nMaxRemovebleDiskSize=549755813888` (512 GiB) + type=12/13 notices (DRWUI.log:17-24) | OUT-OF-SCOPE-basis — DRWUI device-tree UI classification (D-80's "MOM UI machinery" scope registration); the 512 GiB constant itself is unnamed anywhere in the trail — noted, not filed |
| Storage Spaces + VDS enumeration; ReFS probes fail gracefully ×4; used-space routing (CNTFSLib for NTFS vs Win32 API) | DISPOSITIONED — N-071/D-80 + M-111 (used-space consumers) + recognition-contract rows |
| DefaultLog.txt CDirectoryTreeBuilder stage machine (3-branch tree, ~1000-item incremental batches, "notify UI to remove system node") | DISPOSITIONED — the 3-branch architecture (S-048/M-103/S-051/C2) + UI-side tree building (UI OOS) |
| Timing patterns (quick ~21-25 s per 131 GB volume; deep ~15 min; matched-counter cadence) | DISPOSITIONED — the OQ-S4-3/bench lane owns perf verdicts |
| reglog_win.txt video-repair traces (integrationalg.cpp[444], movmp4repair.cpp[132] "new algorithm work") | DISPOSITIONED — S-050's line + the RUNTIME-ARTIFACT inventory row |
| strutureReg.log (video-repair regroup JSON debug logger, "19.2回退分支") | DISPOSITIONED — repair line (S-050/N-130) + RUNTIME-ARTIFACT row |
| Eaolog CEaoActiveOnline init + developer attribution | DISPOSITIONED — telemetry/licensing OOS + N-048 census extension |

**NOT-DISPOSITIONED (2) — the raw-log observables no finding owns:**

1. **The lost-partition target-source chain: "add lost partition from mbr / from filesystem / from register"** — DRWUI.log:637-641 (the 01:21 run; `from register` fires 3×): `add lost partition from mbr start=2048, total=122277823` → `add lost partition from filesystem start=2048,…` → `add lost partition from register start=2048,…` ×3 → `Insert lost partition … genericrecoverycontroller.cpp, 162` → `Found lost partition(StartSector:2048, TotalSector:122277823, FileSystem:NTFS)`. Decompiled first-hand this session: **DRWUI_decompiled.c:921820-921890 (FUN_140588820)** — three distinct 0xA8-stride candidate lists: the MBR-table list (gated on whole-disk flags, filled via FUN_140589070), a filesystem-probe list (FUN_140588ba0), and a **"register" list (FUN_1405895d0)** — a third source channel the trail never dispositioned. Greps: `add lost partition / from register / from mbr / from filesystem / registry candidate / candidate source / known partition` over diff-to-fix/ → **zero hits** (the lost-partition module's B1 lists content-probe families; S-051's fix plan builds targets from "the partition table" only and would reproduce just the MBR channel; M-082's MountedDevices decode is the offline-OS profile — a different consumer). **Engine-side, scan-target assembly — the strongest new item in this re-walk.** MINOR-to-MAJOR candidate: the semantics of "register" (registry-derived known-partition hints vs an in-memory register) is one fix-time decompile probe away; if registry-derived, the core's whole-disk target builder misses a candidate source the og consults.
2. **`i64CountofSectors - i64RealReadSector = 1017…`** — DRWUI.log:638 (mid deep-scan): a sector-accounting diagnostic (declared sector count minus real-read sectors — plausibly the deep-scan progress denominator / short-read ledger). Greps: `i64RealReadSector / RealReadSector` over the whole core repo → zero hits. Thin (one log line, semantics undecoded); flag as a fix-time probe for the scan-progress/bench lane, not a standalone finding.

---

## Counts per family

| Family | Artifacts/claims walked | DISPOSITIONED | OUT-OF-SCOPE (basis exists) | NOT-DISPOSITIONED | FIX-PLAN-GAP (S-050) |
|---|---|---|---|---|---|
| 1 DiskHealth | 1 artifact + 2 companions; 24 claims | 19 | 2 (UI-pure) | **3** (monitor loop; notification IPC; tray-as-stated [errata]) | — |
| 2 FileRepair | 8 artifacts; ~40 capability claims | ~38 | (cloud sub-paths: binary-level only) | 0 at disposition level | **5** (video build-order omission; eligibility seam; output naming; cloud sub-paths; progress/acceptance) |
| 3 SaveScan | 5 artifacts; ~60 claims | ~58 | 1 (LM-hash, UI+NAS basis, unnamed) | **1** (5th record-type variant save path) | — |
| 4 MOM-modules | 33 files | all stub claims rollup-covered | — | **1** (RegLib hive-WRITE hbin allocator) | — |
| 5 config-files | 6 artifacts; ~30 claims | ~29 | — | **1** (Explorer context-menu shell integration) | — |
| 6 runtime logs | 8 log files + 2 forensics artifacts; ~20 observables | 15 | 3 (device-tree UI/D-80; repair-line temps; bench lane) | **2** (lost-partition 3-source chain incl. "register"; i64RealReadSector ledger) | — |

**Total NOT-DISPOSITIONED: 8** (6 substantive + 2 thin/errata-class) · **FIX-PLAN-GAPS on S-050: 5.**

## Top candidates worth filing as findings (value order)

1. **[MAJOR/MINOR — engine-side] The og's whole-disk scan-target assembly consults THREE lost-partition candidate sources — MBR entries, filesystem probes, and a "register" source — and the trail dispositions none of the channel model.** Evidence: `og-codebase/log/DRWUI.log:637-641` (first-hand) + `DRWUI_decompiled.c:921820-921890` (FUN_140588820 — three 0xA8-stride lists; "register" filled by FUN_1405895d0). S-051 (still READ-ONLY) should absorb this: its fix plan's step 1 (table-only target building) would reproduce only channel 1 of 3. Fix-time probe: decompile FUN_1405895d0 to pin "register" semantics (registry MountedDevices-style known-partition hints vs in-memory register).
2. **[MINOR/INFO] RegLib's hive-WRITE half (hbin allocator: `_time64` stamps, `((size>>12)+2)*0x1000` bin sizing, zeroed new bins) has no disposition** — module-internals-wave2--ldm-reglib-ctbfile.md:73-78; the core's cr_reghive is read-only by design. A registration row (like N-130's SIDECAR-OOS pattern) closes it.
3. **[INFO] The .rsf 5th record-type (GUID 76a672b0…, `[552-byte struct]+[5400-byte blob]`, variant save path FUN_1403f1180) is documented but undispositioned** — rsf-format--scan-session.md §2.2 record-ID table. One decompile read (FUN_1403f1180's callers) resolves the trigger; until then the session-format completeness record (s10-field-parity-table) has an unnamed member.
4. **[INFO] DiskHealth's resident monitoring mode (RefreshThread periodic SMART refresh + tray guarding + "Display alert when disk problem are found" + the Click_Open_DiskHealth_By_Notification console launch) is a product-frame capability with no implementation and no registered exclusion** — diskhealth--smart-companion.md:20,28; UIUX/19:26,34. Same registration class as S-050's step 1(b), tiny surface.
5. **[INFO] The Explorer context-menu shell integration ("Recover any deleted data" verb, 25-locale taglines via shellexe.ini) has no register row** — filtering-priority--config-layer.md:60-62; the inventory lists only the string file (display-tier). N-134-class bookkeeping row closes it.
6. **[S-050 fix-plan amendment] Add video repair to the build order/acceptance (or explicitly exclude it), name the eligibility-signature seam and the output-location contract, and carve out the cloud-assisted sub-paths for their own product decision.**

**Corpus-errata candidate:** diskhealth--smart-companion.md:21 attributes the auto.bat `trayicon` mode to DiskHealth's tray; boot-config--winpeshl-autobat-regdrv.md:87 shows that argument belongs to `changeres.exe` (the resolution tray applet).

**Bottom line:** the prior sessions' artifact-level CLEAN claim survives this re-walk at the disposition level for DiskHealth/FileRepair/SaveScan/MOM/config-files engine surfaces — the deep trail (S-024/N-032/M-036 grading+transport decode, the session-format parity table, M-017's log-verified scoring, the 3-B register) is genuinely thorough. The residue this walk surfaces is (a) one real engine-side observable the trail never touched (the 3-source lost-partition target chain), (b) five small registration/completeness rows, and (c) five S-050 fix-plan coverage gaps — none of which change a shipped-code verdict, all of which are cheap to close.
