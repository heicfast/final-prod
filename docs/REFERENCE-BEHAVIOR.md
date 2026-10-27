# REFERENCE-BEHAVIOR — EaseUS Data Recovery Wizard behavioral contract

**Purpose.** The reference product's ACTUAL behavior on the two surfaces our product
ships broken — (1) the SMART/disk-health surface and (2) the scan pipeline — plus the
data-source policy underneath both. This is the contract our DRW work-alike implements
against. Nothing here is invented: every claim cites the research corpus
(`/home/z/my-project/easeus-drw-research`, read-only ground truth). Anything the corpus
does not document is marked **NOT-IN-CORPUS**. Hypotheses are labeled **HYPOTHESIS**.

**Corpus map** (citation shorthand):
- `arch/` = `architecture/` distilled docs (00-INDEX, 04-recovery-workflow, 05-scan-engine, 05-uiux, 07-user-flow, 02-kernel-driver)
- `artifacts/` = `re-workspace/artifacts/` deep per-target docs
- `qml/` = `re-workspace/reversed/ui-resources/diskhealth/qml/` (extracted DiskHealth QML — shipped UI source, byte-exact)
- `DH.c` = `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` (93,316 lines, Ghidra)
- `DRWUI.c` = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (2.38M lines, Ghidra)
- `qt-strings` = `re-workspace/reversed/DRWUI.exe/qt_strings_english.txt` (shipped English string table)

---

## A. SMART / health surface contract

### A.1 Where health lives in the reference product (two surfaces, not one)

The reference splits disk health across TWO binaries:

1. **Inside DRWUI** (the main app): health facts are minimal and recovery-oriented —
   per-partition **TRIM status chips** ("TRIM OFF" / "TRIM ON", qt-strings:1277-1278) on
   home-page rows as an SSD-aware recovery-chance indicator (artifacts/UIUX/07-home-phase-deep-dive.md
   §2), plus bad-sector copy ("There are %1 bad sectors in the disk.", qt-strings:1310;
   "Program is trying to skip bad sectors to find data.", qt-strings:884). **No health
   percentage and no SMART table exist inside DRWUI** — nothing in the 1,537-string
   English table shows one.
2. **A separate companion app, DiskHealth.exe** (`og-codebase/diskhealth/DiskHealth.exe`,
   internal product name **DiskSmart**), launched from DRWUI's "More Recovery Tools" menu —
   card "Disk Health", description "Check disk before recovery." (DRWUI.c:1334066-1334071;
   artifacts/UIUX/35-redesign-reference-atlas.md line 144) — and from Windows notifications
   ("Click_Open_DiskHealth_By_Notification", DH.c:39309). It is a **CrystalDiskInfo-lineage**
   engine under a Qt Quick/QML re-skin (artifacts/DiskHealth/diskhealth--smart-companion.md §1).

**Contract consequence:** the full SMART surface (percentage, temperature, per-attribute
table) belongs to the companion-app surface, enumerated over ALL physical disks, not to a
per-volume row inside the recovery UI.

### A.2 DiskHealth window shell (shipped QML, byte-exact)

- Frameless `ApplicationWindow` 840×590 (856×606 with shadow-blur mode), title
  "EaseUS DiskHealth" (qml/main.qml:11-19; artifacts/UIUX/19-diskhealth-quick-ui.md §1).
- Busy mask ("RotatingWait") during `refreshDisks()`; hidden on `sigDiskInfosChanged` (main.qml:218-239).
- Tray-resident: closing hides to tray when the tray icon exists (`close.accepted=false;
  window.hide()`, main.qml:62-69). Tray-guard tagline: "Disk Health is guarding your disk
  health." (qml/HoverWidget.qml:50).
- Dashboard layout: title bar → "Which disk do you care about?" (main.qml:268) → **disk
  selector combobox** (width 746, model `diskInfoMgr.diskInfos`, main.qml:274-283) +
  refresh button → **Status card + Temperature card** (side-by-side, 140×70 each) →
  left info panel (288×330 arg/value list) + **right SMART attribute table** (504 wide)
  → Done button. This is a single dashboard, not tabs; per-disk detail IS the main view.

### A.3 Which drives are listed

- **All physical disks**, presented as `Disk %1 (%2)` selector entries (DH.c tr-string
  "Disk %1 (%2)"; CautionWidget/HoverWidget rows "Disk %1" + model description).
- Enumeration is physical-disk-oriented with: drive-letter mapping that requires
  `DRIVE_FIXED` ("Drive Letter Mapping - != DRIVE_FIXED", DH.c:16770), bus-type detection
  ("Check Bus Type" / "Bus Type = USB", DH.c:16325-16333), USB vendor/product bridge
  tables ("AddDisk - USB0A…USB20", "AddDisk - ASM1352R", "AddDisk - Si", Sunplus/Cypress/
  JMicron/IO-DATA/Initio/Oxford/Logitec vendors — DH.c:10761-11791), CSMI/SAS RAID arrays
  ("CMD_TYPE_CSMI_PHYSICAL_DRIVE", DH.c:3610; "CSMISAS", DH.c:4642), and NVMe incl.
  virtual disks ("SmartNVMe" DH.c:5653; "Parallels Virtual NVMe Disk" DH.c:5823).
- Interface model strings: "ATA Disk Device", "SATA Disk Device", "SCSI Disk Device",
  "INTERFACE_TYPE_NVME" (artifacts/DiskHealth/diskhealth--smart-companion.md §1).
- Selector auto-jumps to the first caution disk via `firstCautionIndex()` (main.qml:48-55).

### A.4 Per-drive summary cards (what the user sees per disk)

**Status card** (`InfoItem`, title "Status", main.qml:407-420; C++ side DH.c:35622-35870):
- `statusText` = health state string; `statusColor`/icon by `statusIndex`:
  0 = none/unknown (grey `pic_status_none.png`), 1 = Good (green `#2dbd8b`,
  `pic_status_good.png`), 2/3 = bad/red `#ff5327` (`pic_status_bad.png`).
- `infoText` = **health/life percentage**, e.g. "97%" (default literal main.qml:419;
  C++ key `"life"`, value `number(+0x17e0) + "%"`; when the drive reports no life value
  (-1) it degrades to "*" — DH.c:35968-35990 and DH.c:37120-37140).

**Temperature card** (title "Temprature" [sic — shipped typo], main.qml:422-446):
- Value like "42°C" default; click toggles °C/°F (`isTempF`; F = C×1.8+32, DH.c:35905-35920).
- `tempIndex`: 0 = unknown ("- -" shown, `pic_temprature_none.png`), 1 = Normal (blue
  `#308fff`), 2/3 = warning (red `#ff5327`). Decoded threshold: temp > 0x46 = **70°C**
  → warning; out-of-range (-100..199 invalid) → unknown (DH.c:35855-35864:
  `iVar8 = (0x46 < temp) + 1; if (temp < -100 || 199 < temp) iVar8 = 0;`).
- `tempDesc` = "Normal"/warning word from a string list.

### A.5 Per-drive info panel (left, arg/value list — `getDiskStatusInfo()`)

Shipped rows (DH.c:36108-36290, tr-verified order):
1. **Power On Hours** (fallback chain between two counters at +0x17ac/+0x17a8)
2. **Power On Count**
3. **Rotation Rate** — "--" when RPM < 2 (SSD), else "%1 RPM"
4. **Total Host Writes** — "--", "%1 MB" (48-bit raw ÷ shifts) or "%1 GB"
5. **Total Host Read** — "--", "%1 KB" or "%1 GB"
6. **Serial Number**
7. **Transfer Mode** — "current | max" pair (e.g. "SATA/600 | SATA/600"), "--" when absent
8. Feature-flags string: "S.M.A.R.T., ", "APM, ", "AAM, ", "NCQ, " joined with
   newline-wrapping (DH.c:36290-36320), and "Vendor Specific" where applicable.

### A.6 SMART attribute table (the per-attribute surface our product lacks)

Shipped columns (qml/InfoList.qml:10-64, `TableViewColumn` order and widths):
| # | Column (qsTr) | Role | Width | Visible |
|---|---|---|---|---|
| 1 | "ST" (status) | `status` | 36 | always — colored dot per row |
| 2 | "ID" | `id` | 36 | always |
| 3 | "Attribute" | `attribute` | 268 (raw mode) / 228 | always |
| 4 | "RawValue" | `raw` | 200 | **raw mode only** |
| 5 | "Current" | `current` | 90 | full mode only |
| 6 | "Threshold" | `threshold` | 100 | full mode only |

- **Two display modes**: `fullMode()` = Current + Threshold columns; `rawMode()` =
  RawValue column. Mode is chosen per-disk by `diskInfoMgr.showRaw(index)` (main.qml:352-356).
- **No "Worst" column ships** in this product (despite CrystalDiskInfo lineage; the
  "Worst→(ST)" note in artifacts/UIUX/19-diskhealth-quick-ui.md §3 is a lineage remark —
  the actual QML columns are ST/ID/Attribute/{RawValue | Current+Threshold}).
- **Per-row status dot** (InfoList.qml:111-122): value 1 = green `#10a91d`, 2 = amber
  `#f59b00`, else red `#ee0909`; row text is tinted by the same status via model role 257.
- Rows come from `diskInfoMgr.getModel()` (C++ `AttrModel` QAbstractTableModel, DH.c:32122,
  40102) fed by `loadInfoList(diskIndex)`.
- Attribute IDs 0x00 and 0xFF are skipped; raw value formats as 7-byte hex
  ("%02X%02X%02X%02X%02X%02X%02X") or scaled decimal; unavailable raw → "--------"
  (DH.c:34125-34145).
- Attribute **names**: looked up per-disk-model in an override map; formatted ID is
  uppercase hex; unknown/unmapped names degrade to "Vendor Specific" / "Unknown"
  (DH.c:33015-33110). (The full standard-name table is NOT-IN-CORPUS as a literal list —
  names render through the CAtaSmart/CrystalDiskInfo table; only the fallbacks are proven.)
- Special handling for attribute **0xC2 (194, Temperature)** and **0xB8 (184)** in the
  status evaluator (DH.c:34155-34165).

### A.7 Grading model (good/caution/bad)

- **Disk-level status string list, in index order**: `["Unknown", "Good", "Caution",
  "Bad"]` (DH.c:37082-37098) — `statusIndex` 0-3 stored per disk at +0x1800 and clamped < 5.
- **Per-attribute status**: 1 = OK; any of a set of threshold-comparison evaluators
  (DH.c thunks FUN_00458350/004584b0/004580f0/004582c0/00458010) firing → non-OK
  (caution/bad); drives whose attributes lack valid threshold data (flags at
  +0x16d8/+0x16dc/+0x16f8) get "Unknown" status rows instead (DH.c:34140-34160).
- **Health percentage ("life")**: a per-disk integer (offset +0x17e0); -1 = unknown → "*";
  otherwise rendered "N%" under the Status card. Exact aggregation formula (e.g. NVMe
  Percentage-Used vs SSD wear attribute) is NOT-IN-CORPUS — only the field, its
  degradation value, and its rendering are proven.
- **Telemetry shape** (proves the data model): per-disk JSON
  `{"disk":"%d","disk_model":"%s","status":"%s/%s","temprature":"%d","smart_indicators":
  {"<attrname>/<id>":"<value>/<threshold>", ...}}` (DH.c:37100-37106).

### A.8 Transports per drive class (CAtaSmart engine — CrystalDiskInfo solution set)

Documented accessor set (DH.c + artifacts/DiskHealth/diskhealth--smart-companion.md §2):
- **SAT / ATA passthrough** — `GetSmartAttributeSat - 1A`, `- 2`, `- 2A` variants (DH.c:4208-4236).
- **CSMI** (Intel RST / SAS RAID) — `GetSmartAttributeCsmi`, `DoIdentifyDeviceCsmi`,
  `CMD_TYPE_CSMI_PHYSICAL_DRIVE`, `CC_CSMI_SAS_GET_{DRIVER_INFO,PHY_INFO,RAID_CONFIG}` (DH.c:3610, 4766, 4620-4818).
- **NVMe** — `GetSmartAttributeNVMeRealtek` (Realtek NVMe bridge), `SmartNVMe`,
  `INTERFACE_TYPE_NVME` (DH.c:5653; companion artifact §2).
- **USB bridges** — per-vendor/product table: ASM1352R, Sunplus, Cypress, JMicron,
  IO-DATA, Initio, Oxford, Logitec, Si ("AddDisk - USB*" sites, DH.c:10761-11791) —
  bridge selection by (usbVendorId, productId).
- **WMI fallback** — `CAtaSmart::Init WMI on/off - Start`, `OK:WMI Init`/`NG:WMI Init 1/2/3`
  (DH.c:15182-15370).
- Background refresh: `RefreshThread` periodic poller; `CTrayIcon` + QMenu/QTimer tray
  integration (companion artifact §2).
- **Our product's mapping**: per-drive-class transport selection is REQUIRED behavior —
  SAT for plain ATA/SATA, CSMI for RAID stacks, NVMe health-log for NVMe, USB-bridge
  passthrough for externals, WMI as fallback. A single transport for all drives is not
  the reference behavior.

### A.9 Drives that do not support a transport / no SMART data

Proven degradation surfaces (not an error dialog):
- per-attribute: status "Unknown", raw "--------" (DH.c:34130-34160);
- life: "*" instead of "N%" (DH.c:37120-37140);
- temperature: "- -" + `pic_temprature_none.png` (DH.c:35930-35945);
- status card: `pic_status_none.png` grey state for statusIndex 0 (main.qml:319-321).
- **NOT-IN-CORPUS**: a dedicated "SMART not supported" dialog/message. None ships.

### A.10 Detail-below-summary presentation

- Main window: summary cards (Status/Temperature) on top, **info panel + attribute table
  side-by-side below** — a dashboard, not expandable rows and not tabs (main.qml:407-514).
- **Tray hover widget** (HoverWidget.qml): compact all-disks list, each row = "Disk %1",
  model, status pill (green/amber/red), temperature pill; click → jumps main window to
  that disk.
- **Caution popup** (CautionWidget.qml): "Caution!" + "Your disk may have a physical
  failure:" + per-caution-disk rows + "Launch Disk Health" + "View Solutions" (opens
  `getPopLink(languageShort())` URL) + "Don't remind me again". Pops only when
  `popCondition()` and window hidden (main.qml:180-186).

### A.11 DRWUI inline health-adjacent copy (byte-exact, qt-strings)

- "TRIM OFF" / "TRIM ON" chips (1277-1278); "Disk Health" card (385 + DRWUI.c:1334066);
- "There are %1 bad sectors in the disk." (1310); "Program is trying to skip bad sectors
  to find data." (884); "Disk failure or other physical issues" (context of 387);
- "Invalid sector" (642); disk-image guidance for bad-sector disks (389-390).

---

## B. Scan pipeline contract

### B.1 The stage model (names, order, what each does)

Documented sequence for a standard partition scan (arch/04-recovery-workflow.md §3-§5;
artifacts/UIUX/08-scan-phase-deep-dive.md; artifacts/UIUX/34-navigatebar-bottomarea-chrome.md §2.1):

| # | Stage | Engine work | User-visible surface |
|---|---|---|---|
| 0 | Pre-scan prep | `Result_Pre_Scan` telemetry; partition enumeration (`Result_Enumpartition`, `CEnumPartitionThread`); lost partitions inserted as targets ("Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d") | Home → target pick → stack switches to CDetailPage (scan runs INSIDE the results page) |
| 0' | (conditional) Smart Partition Scanning | PartitionRecovery lost-partition analysis, sector walk | "Smart Partition Scanning" → "Scanning disk sectors: %1" → "Smart Partition Scan Completed"/"Stopped"; "We found partition(s):" rows (artifact 08 §2) |
| 1 | **Quick scan** | one engine via `EUCreate<FS>QuickSearcher` — FS metadata pass (NTFS: MFT walk incl. deleted records; `fastsearch` default ON) | Status "Quick Scanning %1, Files Found: N (S)" |
| 2 | **Quick→Advanced boundary** | status-bar state machine fires `sigScanFinished(iScanTypeChanged, iScanType)` with **m_iScanType=3** (bottomarea.cpp:838, "quick pass finished") | Offer dialog `CQuickScanCompleteWnd` (see B.4); advanced continues in background |
| 3 | **Advanced scan** | full searcher set `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` + `EUCreateRAWSearcher` carving, per-FS and RAW run concurrently; naming/filter callback installed into RAW engine | "Advanced Scanning %1, Files Found: N (S)"; sector ticker; remaining time |
| 4 | Advanced boundary | `sigScanFinished(..., m_iScanType=4)` (bottomarea.cpp:856, "advanced pass finished") | Completion modal (see B.5) |
| 5 | **Organize** | tree build → classify (FileFilter.xml 8 cats/1,489 ext) → check (`CCheckNodeThread` recoverability grading) → filter → sort — all in background threads | "Organizing Found Files..." + "Stop Organizing Found Files?" confirm |
| 6 | Results / recovery | selection, preview, Recover flow | Results tree fully browsable |

Stage names for other targets: "Cloud Scanning %1, Files Found:" (cloud), NAS/Linux
variants, "Recycle Bin" direct scan (arch/07-user-flow-uiux.md §2-§4).

### B.2 Progress model per stage

- **Universal status line** (DRWUI.c:1100081-1100450, decoded): the bottom bar renders
  `"<Stage> <percent>%, Files Found: <count> (<size>)"` — stage label "Quick Scanning" /
  "Cloud Scanning" (scanType==1) or "Advanced Scanning" (other), percent in brand blue
  (`#0560fd` light / `#5094ff` dark), count hidden while 0, size formatted 2-dp ("143.74 GB").
  Paused state substitutes "Scan Paused%1, Files found:".
- **Quick scan**: percent exists in the same format (qt-strings:898 "Quick Scanning %1,
  Files Found:"); the quick pass is FAST — **≈21 s on a 149 GB NTFS volume with 1.11 M
  files** (vendor QA log timing match, scan-phase capture §8). Its percentage basis is
  NOT-IN-CORPUS (metadata walk, not sector-linear).
- **Advanced scan**: **sector-based linear model**. Live proof (capture addendum §7):
  at 34% the UI showed "Remaining time: 07m43s | Reading sector: 134316032/31413598" —
  i.e. a reading-position/total ticker + a linear throughput estimate. Captured pairs:
  9% @75 s, 34% @≈460 s, complete @≈880 s (14.7 min, 149 GB disk).
- **Counters shown live**: "Files Found: N (S)" (files + bytes); per-branch counts in the
  tree ("Windows (C:) 1.11M", "File Path Lost 4.4K", "Reconstructed N"); rotating coach
  line "%1 file chunks scanned..." (qt-strings:49) via `ScaningRecoverMessage`;
  "Estimated Time for Advanced Scan:" label (qt-strings:455) and "Estimated time
  remaining: %1s" (456).
- **Sector counts meaning**: sectors read = position in the carving/deep pass across the
  volume (advanced); "Scanning disk sectors: %1" / "Scanned disk sectors: %1" belong to
  the Smart Partition Scan stage (qt-strings:1121/1116). Default sector size 512
  (arch/04 §3.4). Bad-sector states: "Program is trying to skip bad sectors to find data."
  (qt-strings:884).

### B.3 When results become visible (the core of our bug)

- **Results stream into the tree DURING the scan — from the quick pass onward.** Live
  capture at t=75 s (quick done ≈21 s, advanced at 9%): full 3-branch tree populated,
  list browsable, preview panel live, "browse-while-scanning" confirmed behaviorally
  (scan-phase capture §3.2). The UI is fully interactive while scanning; pause/stop and
  "Recover All …" are offered DURING the scan (capture §7).
- **Three (of five) roots visible while scanning**: `Windows (C:)` (existing FS
  structure), `File Path Lost` (warning icon; deleted files whose paths couldn't be
  reconstructed), `Reconstructed` (RAW-carved regrouped) — exactly the .rsf record-4
  roots. On the full results screen five special branches exist: Tags Extracted /
  File Name Lost / File Path Lost / Versions / Unimportant (artifact 09 §2).
- **The Reconstructed branch grows live during advanced scanning** (1 → 16 entries over
  6 minutes — direct runtime proof RAW carving results stream in as found; capture §7).
  File Path Lost count was stable after the quick phase (deleted-file detection finished early).
- Progressive fill mechanics: engine tree builders → `CDirectoryTreeManager`; fill events
  `sigQuickScanFinished(int)` / `sigScanFinished` ×3 / `sigLoadFinished(bool)` trigger
  rebuilds; classify/filter/sort/check run in `CSetClassifyThread`/`CSetFilterThread`/
  `CSortNodeThread`/`CCheckNodeThread` so the UI never blocks (artifact 09 §1, §7).
  Recoverability grading re-fires during browsing/selection ("Quick scan matched=0,
  total=64" telemetry, artifact 30 line 158).
- **Files are recoverable before scan completion**: quick-complete CTA "Recover Found
  Deleted %1 files %2" (qt-strings:927); "Recover All …" active mid-advanced-scan.

### B.4 Quick → Advanced transition

- **Automatic continuation**: after the quick pass finishes (m_iScanType=3 boundary) the
  advanced scan starts by itself — the 75 s capture shows advanced at 9% with zero user
  input after the single start click; the offer dialog's own copy says "Advanced Scan is
  trying to find more files for you." (qt-strings:170) / "Advanced Scan is trying to find
  more other lost files for you while recove[ring…]" (artifact 08 §4).
- **Offer dialog `CQuickScanCompleteWnd`** (4 tr strings): "Quick Scan Completed" /
  "You may now" / **"Recover Found Deleted %1 files %2"** (primary CTA) / coaching line
  above; upsell variant "Advanced Scan can find more files - many users recover their
  data after a full scan." (qt-strings:169). Alternative result-state string: "Good news -
  some files are recoverable, you can try recovery or scan deeper." (artifact 08 §4).
  Whether this dialog is modal-blocking or auto-dismissed in the default flow is
  NOT-IN-CORPUS at the pixel level (the 75 s census shows no modal — see HYPOTHESIS below).
- **Engine-side**: advanced = the FULL searcher set + RAWSearcher + naming callback,
  gated by settings flag +0x60 (arch/04 §3.3, FUN_140580720). Status-bar scan-type field
  switches from 1 (quick) to advanced (DRWUI.c:1100081-1100260).
- **HYPOTHESIS (D)**: the reference treats quick and advanced as separate progress
  domains with a hard boundary event; a UI that reports one blended percentage is not
  the reference model (see B.10).

### B.5 Completion behavior

- Bottom bar: **"Scan Completed, Files Found: N (S)"** (qt-strings:1095; capture §8
  verbatim "Scan Completed, Files Found: 1139286 (147.12 GB)").
- Centered modal (540×380 standalone OS child window, blocks toolbar until dismissed —
  artifact 30): shield graphic + **"Congratulations! Scan Completed!"** + "Great news:
  Your files are highly recoverable" + **[Go to Recover]** (blue). "Go to Recover" lands
  in the results browse with an "AI Recommend available." toast (artifact 30) — not the
  destination dialog.
- Completion stats grid (artifact 08 §3): "Files found · Files scanned · Used Time ·
  Size · Organized · Completed · Video Repaired · Video to be repaired".
- **Auto-session-save on completion**: cascade cache-reset → Freeze →
  `auto_<Volume>_<timestamp>.rsf` auto-save → Unfreeze → `CScanMgr::ScanFinished:636` →
  bottomarea.cpp:838/856 (nStatus=4, m_iScanType 3→4) (artifact 30 lines 73-74, 90).

### B.6 Session save/load

- "Save Scan Session" → `.rsf` (zlib-chunked records; FULL found-file tree; format fully
  reversed — artifacts/SaveScan/rsf-format--scan-session.md). Menu ids: Import/Save Scan
  Result 0x10/0xF (atlas line 85).
- Home banner: "Continue last scanning · Scan time: · Scan files: · Scan disk: · Load"
  (artifact 07 §2). Load: "Load Scan Session Successful, Files Found:" (qt-strings:692);
  corruption surface: "Scan session might be corrupted. Please rescan to recover your
  data." (qt-strings:1111). Import path "(1-Import Scan Result".
- Save-progress model (for our progress-bar semantics): record-4 tree serialization
  reports `percent = 40% + (processed×50%)/(total×2)` — i.e. **40%→65% band during the
  tree walk**, remainder on compression/finalize (artifacts/SaveScan/rsf-record4--tree-node-grammar.md §5).

### B.7 Pause / Stop semantics (decoded from DRWUI.c:1098380-1098520)

- **Pause button**: toggles state property "paused"; stops the progress timers; shows
  "Scanning Paused." (qt-strings:1120); status line becomes "Scan Paused%1, Files found:
  N (S)" (qt-strings:1098). Resume recomputes and shows "Remaining time: %1".
- **Stop button**: opens confirm dialog ("Stop searching for lost data?" body "Remaining
  data can be rescanned and recovered anytime later, but continuing now save you time.",
  qt-strings:974); on confirm sets state "stopped", disables stop, dispatches scan-abort
  (Win32 event at CScanMgr+0x108, arch/05-uiux §3) via `CStopScanThread` cooperative
  cancel; final line "Scan Stopped, Files Found: N (S)" (qt-strings:1101). Risk-framing
  variants for repair-coupled stops (qt-strings:975-983).
- Stop-organize confirm: "Stop Organizing Found Files?" / "Some unsorted data will not be
  displayed if you stop." / "It may take a while to stop organizing files." (qt-strings:651, 1206).

### B.8 Error surfaces when a scan cannot proceed (tr-verified, artifact 08 §5)

| Situation | Copy | Controls |
|---|---|---|
| Drive removed mid-scan | "Drive Disconnected" — "Unable to continue scanning. Reconnect your drive and try again." | Retry / Cancel |
| NAS offline | "NAS Disconnected" — "Check network and make sure NAS is on. Then try again." | Retry/Cancel/Disconnect |
| Linux box offline | "Disconnect with Linux" flow | Retry/Cancel/Disconnect |
| Cloud session lost | "Cloud Drive Disconnected" — "Please check your network connection and try again." | Retry/Cancel/OK |
| Network generic | "Check your network to make sure the %1 server is accessible and then r[etry]" | Retry/Cancel |
| Access denied (app-level gate) | "Access restricted" — "You do not have permission to access. Click \"OK\" to exit." (DRWUI.c:1312316) | OK → exit |
| Account permission (remote login) | "Account without sufficient permission. Try with your Administrator account." | — |
| Bad sectors | "Program is trying to skip bad sectors to find data." / "There are %1 bad sectors in the disk." | continues (skip) |
| Corrupt session | "Scan session might be corrupted. Please rescan to recover your data." | — |
| Missing storage driver (WinPE) | "Add Driver" menu item (id 0xC) → "Load Driver %1 Success" / "Successfully add driver %1" / "Failed to add driver. Error Message: %1" (qt-strings:687-688; DRWUI.c:1321463) | dialog |
| Empty recycle bin | `RecycleBinScanNoRltWnd` result state | — |

**NOT-IN-CORPUS**: a user-facing "kernel driver required / driver install failed, scan
refused" message on live Windows. The driver (`eprdtdrv.sys`) is installed by
`PartitionRecovery/setupdrvx64.exe` (copies to `%SystemRoot%\System32\drivers`, SCM
create+start) at app runtime without a documented refusal dialog; live-system behavior
when the service cannot start is NOT-IN-CORPUS.

### B.9 Expected durations (measured, 149.45 GB volume, 1,139,286 files)

- Quick pass ≈ **21 s**; advanced 9% @75 s, 34% @≈460 s (remaining 07m43s), **complete
  ≈14.7 min** (capture §7-§8). ≈98.5% of disk capacity enumerated.

### B.10 What "~40%, crawl, no results" maps to in the reference model (labeled hypotheses)

- **HYPOTHESIS-1 (most likely)**: our scan conflates the two phases. In the reference,
  quick (metadata) and advanced (sector-linear) are distinct progress domains with a
  boundary event; the quick domain finishes in tens of seconds WITH results already in
  the tree. A bar that jumps then crawls looks like an advanced-only sector counter
  starting from a quick-phase offset (or a percentage basis switching mid-bar) — the
  reference never blends them.
- **HYPOTHESIS-2**: our bar is tracking the post-scan ORGANIZE phase: the reference's
  organize/save progress starts at **40%** and climbs to 65% during the tree walk
  (rsf-record4 §5) — a jump to ~40% then slow crawl is exactly that curve if results are
  (incorrectly) withheld until organization finishes. In the reference, results are
  visible during the scan, so the organize band is never user-blocking.
- **HYPOTHESIS-3**: raw carving throughput: at 9-34% the reference carves sectors
  linearly for many minutes — a "crawl" is CORRECT advanced-scan behavior, but only
  acceptable with (a) results already visible, (b) the sector ticker + remaining-time
  estimate communicating it, (c) pause/stop available.
- Not a hypothesis (fact): the reference NEVER shows a results-empty screen at 40%
  advanced progress — the 3-branch tree is fully live by then.

---

## C. Data-source contract

### C.1 Disk/volume addressing (documented strings)

- **Raw partition/disk IO**: `\\.\EPRDTDRV\%d\%d` (disk# \ partition#) and
  `\\.\EPRDTDRV\%d\Volume\%d` (disk# \ volume#) — served by `eprdtdrv.sys` (EPMNtDrv,
  device `\Device\EPMNTDRV` + `\DosDevices\EPMNTDRV`), which resolves to
  `\Device\Harddisk%u\Partition0` / `\Device\Harddisk%u\DR%u` / `\Device\HarddiskVolume%u`
  (arch/02 §2.2; artifacts/MOM-modules/Device.md:39-41).
- **Direct Windows device paths** (used by Device.mo): `\\.\PhysicalDrive%d`,
  `\\?\PhysicalDrive%d`, `\\.\HarddiskVolume%d`, `\\.\%c:` (Device.md:42-51).
- **WMI storage namespace**: `\\.\ROOT\microsoft\windows\storage:MSFT_Disk`,
  `MSFT_PhysicalDisk`, `MSFT_Volume`, `MSFT_StoragePool`, `MSFT_StorageSubSystem`,
  `MSFT_VirtualDisk` (Device.md:45-50) — enumeration/model data.
- **Volume letters** for UI targeting: home list "SSD (C:)" per partition + "Disks and
  Partitions" whole-disk group (type codes 22-25); landing device cards show live
  capacity "Windows(C:) 30.96 GB/149.45 GB" (landing capture; artifact 07 §2, §4).
- **Destination validation**: `CRecoveryMgr::ExportThread` opens `\\.\X:` raw volume of
  the destination (writability + same-volume detection), `PathIsNetworkPathW` for
  network (arch/04 §7.3).

### C.2 Driver vs direct-access policy per operation

- **Scan engine volume IO** (arch/05-scan-engine §8; arch/04 §4a): normal file API when
  the volume is mounted & healthy → else raw IOCTL path through EPMNtDrv
  (`\\.\EPRDTDRV\<d>\<p>`, MDL async sector IO, 512 B sectors). The engine-side device
  abstraction is `CPartitionVirtualVolumeDevice`/`CStorageDeviceWithCache` over
  `IStorageDevice` — lost partitions (no OS volume object) are handled through the same
  abstraction (NTFS deep-dive §8).
- **Driver deployment**: `eprdtdrv.sys` installed on live Windows by
  `PartitionRecovery/setupdrvx64.exe` (SCM service); WinPE build preloads
  `ebrntdrv.sys`/`epmdkdrv.sys` via regdrv.reg; `eprdtdrv_win7older.sys` for Win7-
  (arch/02 §2.1, §5). Hook/raw-door drivers EUDCPEPM/EUEDKEPM are **WinPE-only**
  (volume lock / disk copy) — NOT loaded during normal Windows operation (arch/02 §3).
- **Process gating**: EPMNtDrv validates the calling process (PsGetCurrentProcessId/
  ZwQueryInformationProcess imports) — only authorized DRW/EPM processes may use the raw
  path (arch/02 §2.4). Custom IOCTLs: 0x60002400 (set/resolve global target),
  0x60002404 (GetFileObjName handle→name), 0x60002408 (GetDiskContextMode); all other
  IOCTLs pass through to the disk stack transparently (arch/02 §7; eprdtdrv artifact §4).
- **BitLocker volumes**: FVE keys via `\\.\Pipe\DRWBitlocker` (DRWService.exe reads
  HKLM FVEAutoUnlock), content XTS-AES via BitLockerLib/OpenSSL (arch/04 §4a).
- **NAS**: EuMrx.sys redirector letter (`X:\EuShare.com\...` → `\\127.0.0.1` tunnel) —
  scan reads flow through the redirector, not raw disk (arch/05 §8).
- **SMART queries (DiskHealth)**: user-mode multi-transport (SAT/CSMI/NVMe/USB bridge/
  WMI — §A.8); DiskHealth does NOT use the kernel drivers for SMART (its imports are
  user-mode; CrystalDiskInfo-lineage accessors).

### C.3 Refusal conditions and their UI messages

- App-level permission gate → "Access restricted" / "You do not have permission to
  access. Click \"OK\" to exit." (DRWUI.c:1312316-1312320).
- Administrator-elevation requirement for the app itself: NOT-IN-CORPUS (no
  requireAdministrator/manifest claim documented).
- Scan-blocking device states → the B.8 table (drive/NAS/cloud disconnect dialogs with
  Retry/Cancel).
- Missing driver on WinPE → "Add Driver" menu flow (B.8). Live-system driver-install
  failure surface: NOT-IN-CORPUS.
- BitLocker locked → unlock dialogs (password/recovery key) before scan proceeds
  (arch/07 §2).

---

## D. Health / scan query API shapes (as documented — shape level)

### D.1 Engine plugin contract (scan queries)

```
HMODULE h = LoadLibraryW("NTFSSearchDLL.dll");            // DRWUI FUN_14056c0d0 loads all 10
FARPROC  f = GetProcAddress(h, "EUCreateNTFSQuickSearcher"); // or EUCreate<FS>Searcher
ISearcher* p = nullptr; f(&p);                            // factory fills the vptr
```
- Every engine exports exactly `EUCreate<FS>Searcher` + `EUCreate<FS>QuickSearcher`
  (arch/05-scan-engine §2). RAWSearchDLL additionally: `EUGetAnalyserList`,
  `EUGetAnalyserMap`, `EUGetAnalyserMultimap`, `GetRawUserInfoMap`,
  `EUCreateRAWSearcherForCellTest`.
- Interface family (RTTI both sides): `IFileSystemSearchController`,
  `IQuickFileSystemSearchController`, `ICompleteFileSystemSearchController`,
  `IRawDirectoryTreeBuilder`; engine controllers multiply-inherit the tree-builder
  (`DetourDirectoryTree` decorator intercepts node insertion for progress+dedup+merge).
- FS codes in DRWUI: 0-2 FAT, 3 NTFS, 4-5 EXT2, 7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 13 ZFS
  (+Btrfs) → factory-name mapping (FUN_140577d40); advanced set built in FUN_140580720.
- Engine error codes: HRESULT-style `0x5002xxxx` (e.g. 0x5002200000004 open-fail,
  0x5002c0000004 read-fail); verbose diag channel `"Return Error:0x%x File:%s Line:%d
  Fuction:%s"` compiled into release (arch/05 §4).

### D.2 DRWUI orchestration shapes

- `CScanMgr` singleton (FUN_140149c30) owning `IScanMgr`; scan start = orchestrator
  vtable+0x18; licensing hooks at +0x1f0/+0x1f8/+0x220 at every scan start; scan-abort
  Win32 event at CScanMgr+0x108 (arch/05-uiux §3).
- Strategy objects `CDoScanPartition` / `CDoScanLostPartition` / `CDoScanCloudDevice` /
  `CDoScanRemoteDevice` (44 v-methods each) run the engine loop (artifact 08 §6).
- Progress plumbing: `CUIScanProgressInfo` models, `CGenericRecoveryProgress` +
  `CGenericRecoveryController`/`CCompleteRecoveryController`/`CDeletedRecoveryController`
  (vtables 18/37/25); completion signal `sigScanFinished(iScanTypeChanged, iScanType)`
  emitted from bottomarea.cpp:838/856 at each phase boundary (artifact 34 §2.1).
- Engine→UI event bridge: `CMessageProxy` singleton (MI QObject + INotifyObserver at
  +0x10) registered with the MOM service; engine side constructs a 24-byte
  `CMessageSend` envelope → `CMessageSend::dispatch(int, void*, int&)` Qt signal with
  thread-affinity-aware (queued) marshaling → `CMessageProxy::doDispatchMessage` hub
  (artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md §1).

### D.3 DiskHealth QML-bridge API (health queries — the `diskInfoMgr` context object)

Methods invoked by the shipped QML (main.qml / CautionWidget.qml / HoverWidget.qml):
`refreshDisks()`, `clickRefresh()`, `refreshForPop()`, `selectDisk(int)`,
`getDiskInfos()`, `getDiskStatus(int)` → map `{statusIndex:int, statusInfo:str,
life:str, diskIndex:int, diskDes:str, tempIndex:int, tempValue:str, tempDesc:str}`
(DH.c:35622-36000), `getDiskStatusInfo(int)` → list of `{arg, value}` (DH.c:36072-36290),
`getCautionDiskStatusInfo()` → per-caution-disk rows, `firstCautionIndex()`,
`diskIndexArg()`, `getModel()` → `AttrModel` (roles: status(257)/id/attribute/raw/
current/threshold), `loadInfoList(int)`, `showRaw(int)`, `isBusy()`, `isTempF` (bool,
notif `sigIsTempFchanged`), `popCondition()`, `showWindow()`, `isTrayExist()`,
`setWindowIcon()`, `mousePos()`, `screenSize()`, `showWindowShadow()`, `openUrl()`,
`getPopLink(lang)`, `showFrame()`/`sigShowFrame`, `doExit()`, `quit()`.
Signals: `sigDiskInfosChanged`, `sigAboutRefresh`, `sigExit`, `sigHoverEnter(x,y)`,
`sigHoverLeave`, `sigCheckForPop`, `sigSwitchDiskIndex`, `sigLanguageChanged`,
`sigIsTempFchanged`.
C++ engine side: `DiskInfoMgr` (disk collection manager, refresh cycles),
`CAtaSmart` (SMART engine; `CArray<ATA_SMART_INFO>`, `CArray<EXTERNAL_DISK_INFO>`,
`CArray<DISK_POSITION>`; transport accessors per §A.8), `RefreshThread`,
`CTrayIcon`, `AttrModel` (companion artifact §2; DH.c:1336-2299).

### D.4 Raw-IO driver API shape (eprdtdrv/EPMNtDrv)

- Open `\\.\EPRDTDRV\{disk}\{part}` or `\\.\EPRDTDRV\{disk}\Volume\{n}` → IRP_MJ_READ
  MDL async sector IO (len<<9 = 512 B sectors); custom IOCTLs 0x60002400/04/08 (§C.2);
  16-byte "esdr" context in FsContext2; PID-token handshake on the EUEDKEPM variant
  (arch/02 §2.2-2.4, §7).

---

## E. Definition of done — checklist for our product

Derived directly from §A-§D. Each row is a testable assertion for our implementation.

### E.1 SMART/health surface

1. **All physical disks enumerated** in the health surface (disk selector listing every
   physical drive, "Disk N (model)" form) — not a single drive. [A.3]
2. **Per-drive summary cards**: Status card = state word (Good/Caution/Bad/Unknown) +
   life percentage "N%" (or "*" when unavailable); Temperature card = value + °C/°F
   toggle + Normal/Warning grading (warning threshold 70°C per decompile). [A.4, A.7]
3. **Per-drive info panel** with at least: Power On Hours, Power On Count, Rotation
   Rate ("--" for SSD), Total Host Writes, Total Host Read, Serial Number, Transfer
   Mode ("cur | max"). [A.5]
4. **Per-attribute SMART table** with columns ST/ID/Attribute + (Current, Threshold)
   OR RawValue depending on mode toggle; per-row status dot green/amber/red; attribute
   IDs 0x00/0xFF skipped; unknown names degrade to "Vendor Specific"; unsupported
   values degrade to "--------". [A.6, A.9]
5. **Multi-transport querying**: SAT passthrough for ATA/SATA, CSMI for RAID stacks,
   NVMe health for NVMe drives, USB-bridge handling for externals, WMI fallback —
   per-drive-class selection. [A.8]
6. **Grading model**: disk status ∈ {Unknown, Good, Caution, Bad}; per-attribute status
   from threshold comparison with Unknown for no-threshold drives. [A.7]
7. Health-percentage-only with no attribute table and one drive = FAIL by definition of
   this contract.

### E.2 Scan pipeline

8. **Two-phase stage model with distinct labels**: "Quick Scanning %, Files Found:" then
   "Advanced Scanning %, Files Found:" — never a blended unlabeled bar. [B.1, B.2]
9. **Quick pass completes in metadata time** (seconds-to-minutes, not hours) and fires
   a boundary event; the quick→advanced transition is automatic, with an offer surface
   ("Recover Found Deleted %1 files %2") available while advanced continues. [B.4, B.9]
10. **Results visible DURING the scan**: the results tree is live from the quick pass
    onward — existing-structure root + "File Path Lost" + "Reconstructed" branches with
    live per-branch counts; the Reconstructed branch grows during advanced scanning;
    browse/filter/preview/Recover-All interactive mid-scan. [B.3]
11. **Live counters**: files + bytes ("Files Found: 1,139,157 (143.74 GB)" format);
    rotating "%1 file chunks scanned..." milestone line. [B.2, B.3]
12. **Advanced progress is sector-linear with a reading-position ticker**
    ("Reading sector: X/Y") + remaining-time estimate ("Remaining time: %1" /
    "Estimated time remaining: %1s"); hours-long crawls are legitimate but must be
    communicated. [B.2, B.9]
13. **Pause/Stop semantics**: pause toggles "Scan Paused%1, Files found:"; stop requires
    a confirm dialog ("Stop searching for lost data?" + rescan-later risk framing) then
    "Scan Stopped, Files Found:". [B.7]
14. **Completion**: "Scan Completed, Files Found: N (S)" + "Congratulations! Scan
    Completed!" modal with "Go to Recover"; organize phase ("Organizing Found Files...")
    is non-blocking background work, not a results gate. [B.5]
15. **Session save/load**: save to .rsf on demand AND auto-save at completion; home
    "Continue last scanning … Load" banner; corrupt-session error copy. [B.6]
16. **Error surfaces**: drive-disconnected / network / access-restricted dialogs with
    Retry/Cancel per the B.8 table; bad-sector skip is communicated, not fatal. [B.8]

### E.3 Data-source policy

17. Volume IO prefers the normal file API for healthy mounted volumes and falls back to
    a raw path (driver namespace or direct physical-drive open) for damaged/lost
    volumes; addressing by disk#\\partition# AND volume#, plus direct
    `\\.\PhysicalDriveN`/`\\.\HarddiskVolumeN` and WMI MSFT_* for enumeration. [C.1, C.2]
18. Driver install (live system) is silent SCM service setup; a WinPE "Add Driver"
    surface exists for missing storage drivers with success/failure copy. [C.2, B.8]
19. Destination validation opens the destination volume raw (`\\.\X:`) for writability
    and same-volume checks. [C.1]

---

## F. NOT-IN-CORPUS register (do not implement from imagination)

- Quick-scan percentage basis (metadata-domain denominator) — only its format is proven. [B.2]
- Whether `CQuickScanCompleteWnd` is modal-blocking or auto-dismissed in the default
  flow (pixel census at 75 s shows no modal). [B.4]
- Health-"life" aggregation formula per drive class (NVMe percentage-used vs SSD wear
  attribute) — only the field/rendering are proven. [A.7]
- The literal standard SMART attribute-name table (names render via the CAtaSmart
  lookup; only fallbacks "Vendor Specific"/"Unknown" are byte-proven). [A.6]
- App-level administrator-elevation requirement (manifest) — only the "Access
  restricted" runtime gate is documented. [C.3]
- Live-Windows driver-install-failure refusal message. [B.8, C.3]
- A "SMART not supported" dialog — the reference degrades to "Unknown"/"--------"/"*". [A.9]

*Corpus version: easeus-drw-research @ 03ad46c (11,495 files). Compiled by Agent D
(T4-D) from the sources cited inline; all quotes byte-exact from shipped binaries.*
