# S69-A context sheet — local database & persisted state

Agent A of lane S69 (Task ID: S69-A), read-only. Corpus pin `be0cfe9`, core pin
`3799dee2` (both re-verified this session). Abbreviations: **DRWUI** =
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`; **RAW** =
`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`; **exFAT** =
`.../ghidra-searchdll/exFATSearchDLL_decompiled.c`; **EVR** =
`.../ghidra-repair/EUVideoRepair_decompiled.c`; **PRD** =
`.../ghidra-mo/PartitionRecoveryD_decompiled.c`; core paths relative to
`core_recovery/`.

## A. Prior-coverage verification

| Topic | Covered-by | Still accurate? |
|---|---|---|
| lostpart.dat/partinfo.dat lifecycle | S55-D1 sheet + I-046 | Yes (not re-walked; no contradiction found) |
| .rsf session format (records 1-4) | `artifacts/SaveScan/*`, S-044/S-057 | Yes — the 3 real `auto_*.rsf` files re-listed this session confirm the naming formula |
| User-sign collision rule / disabled-format gate / parser strictness | S-055, S-056, M-129 (all FIXED) | Yes — re-verified at HEAD: `cr_usersigns.cpp:365-379` (toggle gate), `cr_carve.cpp:1441-1461` (disable-map build, drifted from S-056's :1118-1131 cite) |
| EUUserData per-scan stats map | N-140 (MINOR, ACTIVE), errata #9 | Yes — og-side cites re-verified first-hand (`GetRawUserInfoMap` family per errata #9; DRWUI report reader :227538-625 per errata) |
| FilterFolder.ini junk pools | M-018 (landed) | Yes; the two FileName reader sites re-seen first-hand (DRWUI:337918, :749898) |
| Marked-disk marker rwConfig.ini | N-145 (ACTIVE) | Yes — **LastScanInfo.ini decoded below is a different, unregistered file** |
| Dict password dictionary | S-050 (FIXED) | Not re-walked; no contradiction |
| PriorityDisplayType/SumaryFileType core data | N-132 (FIXED, `cr_config_display_data.cpp`) | Yes — table re-verified byte-exact vs the XML first-hand |
| FileFilter.xml 4 DRWUI sites + exFAT engine side | M-023 (FIXED) + `s3-filefilter-health.md` | Yes — all 4 sites + exFAT:47165 re-read first-hand |
| RawConfig.ini 34 toggles | M-024 (FIXED), S-056 | Yes — parser grammar re-decoded first-hand (below) |
| Log channels (Eaolog/strutureReg/reglog_win/DRWUI.log/DRWLog) | N-048 census + s12-corpus-rewalk + I-027 row 75 | Yes — writers cited first-hand below; **no reader found for any** (write-only telemetry; no new divergence) |
| auto.txt / needcopy.txt / DataFile.ini | N-097 / I-027 row 98 / I-027 row 67 | Yes |
| version.ini TreeType/scanext4 | I-036/S42 register (`s42-config-layer-dispositions.md` §2) | Yes — **MaxFileSize is NOT in that disposition (new, below)** |

## B. Fresh decodes (og side)

### 1. config/usefullostfiles.xml — the "Unimportant files" demotion gate (lost-files branch)
- **Reader** `FUN_1401b8380` (DRWUI:320366-320439): builds `<exe-dir>\config\usefullostfiles.xml` (:320396), TinyXml load; **on parse failure or missing root → hardcoded fallback** `FUN_1401b8560` (:320400/:320406 → :320441-320643): clears the set (`FUN_1401bc880` :320517) then inserts **27 hardcoded exts** (:320518-320643) — same 27 as the shipped XML (count verified). Live parse: iterates child elements (:320417-320432), inserts each non-empty text into a `std::set` at **singleton+0xa0** (:320423-320424), **cap 0x65 = 101 entries** (:320435).
- **When**: lazily — `FUN_1403bb1d0` reloads iff the set is empty (`FUN_1401bc920(singleton+0xa0)` :635527-635529); the gate itself runs at **results-tree build time**: `FUN_1401d37d0` ("Build tree finish." :339014) fetches the mode-2 subtree (`FUN_1401d7950(...,{2})` :339021-339024) and calls `FUN_1403bb1d0(subtree, …)` (:339020) — i.e. in the same completion/stop-organize handler S-057 decoded, **not at scan start**.
- **What it gates** — `FUN_1403bb970` (:635764-635910): creates a bucket node **"Unimportant files"** (:635826, flagged :635831-635835) under the branch root, then partitions the branch's found items:
  - folders: name `strlen < 3` → Unimportant (:635850-635862); else `FUN_1403bbdc0(tree, folder, 2)` — a **recursive any-descendant-has-useful-ext** walk (singleton+0xa0, `_strlwr`, set-find, :636028-636041) — no useful descendant → Unimportant (:635869-635877);
  - files: extension lowercased (`_strlwr` :635890-635891), set membership (`FUN_140256d00` find + end-compare :635892-635895): **∈ set → attached to the branch ROOT; ∉ set → "Unimportant files"** (:635896-635899).
  - Files are never dropped — purely a display-tier demotion. (Confirms + extends the inventory row's "display-tier whitelist promoting … out of Unimportant".)

### 2. config/PriorityDisplayType.xml — Type-tree ext ordering attached at tree-model build
- **Reader** `FUN_1401b8f50` (DRWUI:320683-320777): path :320726; per `<type>`: `Description` attr (:320737) mapped to a category id by `FUN_1401b8e90` (:320647-320677: **Picture→1 (default), Video→2, Document→3, Audio→4, Archive→5**); per `<ext>`: text + `alias` attr (:320744-320747) stored as a pair into an ext→alias map, the whole per-category map inserted at **singleton+0xb0** (:320762-320765); ≤100 categories (:320770-320774). No fallback on parse failure (just closes, :320732-320737).
- **When**: lazy-once via `FUN_1401b5d50` (:318999-319001, load iff map+0xb0 empty); that init runs at results/session-window setup and restore (:441717, :442615, :1106737, :1106816, :1106866).
- **Consumer** — `CTreeDataModel` method `FUN_1403933b0` (:611190-611276, class vftable `CTreeDataModel::vftable` :611089): iterates 16 tree-node kinds (skip 0xc/0xd), creates each node (:611252), then **if the node's category id exists in the priority map (singleton+0xb0, find :611252-611260, end-check :611261-611263) attaches the ext→alias map to the node via its vtable+0x250 setter** (:611264-611268). So the alias strings ("00jpg","01png"…) ride on the type-category nodes as their within-category sort contract. Adjacent: category display-order remaps in the same class — `FUN_1401b8250` (:320125-320134: 0→1,1→2,2→5) and `FUN_1401b82b0` (:320137-320163: 0→7,1→8,2→1,3→6,4→2,5→9,6→10,7→4,8→3,9/10→5).
- **Adjacent NEW fact**: the same init reads **remote-config** keys `value_fastgoodfile` (:319007) and `value_sumaryfiletype` (:319016) through the settings singleton (FUN_140161fb0 → FUN_140160370 :262968), storing the summary-filetype string at singleton+0x280 (:319022). Both keys exist in the shipped `og-codebase/remote_config_data` cache (verified by `strings`) — the og's summary-family groups are **cloud-tunable at runtime**; precedence vs FilterFolder.ini's `SumaryFileType` key UNDECODED (no ini-side read of that key found in DRWUI).

### 3. config/RawConfig.ini — engine-side per-format carve toggles (grammar first-hand)
- **Reader** `FUN_180010b10` (RAW:13066-13217), called at engine init (`FUN_180006730` :4244): builds `<dll-dir>config/RawConfig.ini` (:13128-13133), `GetPrivateProfileSectionA("Filter", buf, 0x1000, path)` (:13136) — reads **every** key=value pair; splits at `'='` (:13172); **`value == "1"` (exactly one char '1') → enabled pair, anything else (0/empty/garbage) → disabled pair** (:13175-13196), each inserted into a map on the engine object (`FUN_180011970` lookup + `FUN_180011a10` insert). 34 keys shipped (count verified; STL=0, VPJ=0 default-off).
- **Enforcement**: `FUN_180006b30` (RAW:4386-4440, called :9182/:9264) — the registry-removal loop that drops disabled formats' analysers so their signs never fire (the M-024/S-056 decode, verified first-hand at its site). It tunes **format on/off only** — no thresholds, sizes, or strides in this file.

### 4. FileFilter.xml — 4 DRWUI parse sites + engine site; Selected=2 has NO consumer
- Site 1 (:541322, parse-once global `DAT_1425f50b0`): root tag must be `xml` (:541336); walks `FileCategory`→`FileType`→`Extention` (:541339-541366), reads the **`iconIndex` attribute** (:541341-341), strips the leading `.` from each ext (:541371), joins into a per-category list stored in a **global map iconIndex → ext-list** (:541381-541385); membership lookup `FUN_14031f1e0` (:541403-541480) = build `".ext"` and `strstr` in the list.
- Site 2 (:665156, `FUN_1403ed150`): category whose iconIndex remaps to 3, FileType `Description` containing **"TIFF"** (:665186-665188), collects its extensions **excluding two constants** (`.tif`/`.tiff` per the DAT compares :665191-665221) — the camera-RAW list.
- Site 3 (:714705, `FUN_140440340`, `CMatchingFileTypeWorker` :721145): iconIndex remap `FUN_140440290` (:714626-714657: **0→3, 1→7, 2→2, 3→4, 4→5, 5→8, 6→6, default→9**), finds the category whose remap == requested id, collects all its extensions (:714728-714776), hands to the matcher (:714789).
- Site 4 (:827740): TIFF-family again, exts **`_strupr`'d into a global set** (`DAT_142644130`).
- Engine side: exFAT:47165 opens `<dll-dir>FileFilter.xml` (TinyXml), Graphic(iconIndex 0)/TIFF-only → the engine's RAW set (the s3 decode, verified).
- **Selected=2 semantics: UNDECODED-by-construction — dead data.** The file carries `Selected="2"` on the root and `<Selected>2</Selected>` children (15 lines; FileFilter.xml:1,3,33,328,765,769,790,794,1077,1081,1300,1304,1309,1530,1534). Grep for `"Selected"` across DRWUI/exFAT/EVR: the only hits are Qt **stylesheet states** (`QVariant("Selected")` DRWUI:1209830/:1209887 — `WSHover`/`WSNormal`/`Selected` theming), not XML reads. No `RootName`/`ShowComment`/`IsMutex` readers either. Claim: no decompiled consumer reads the Selected fields.

### 5. EUUserData — repair-chain sign DB, never a scan-input merge
- Loader re-verified first-hand: `sprintf("%s/EUUserData", exe-dir)` DRWUI:1005386-100387, `_wfopen_s` :1005412; parser `FUN_140606e80` :1005497+ (JSON-object gate `'\a'` :1005566 — M-129's cites re-confirmed).
- Consumer chain re-verified: `CHKRepairFileSign::vftable` :1002677/:1002782; the MatchedFile/NotMatchedFile signature checker :721627/:721651 — **the CHK repair chain only**. **EUUserData is never merged with raw_signs.json**: `rg EUUserData` over RAWSearchDLL = 0 hits (errata #9; re-confirmed by grep this session); the engine's own table is loaded separately at init (RAW:23836 `FUN_1800098d0(…,"raw_signs.json")` — runtime file, ifstream). So og merge semantics/precedence with raw_signs.json: **N/A — the two tables never meet in the og**; the core's merged user-sign channel (D-104) is a deliberate superset.
- No writer for EUUserData found in DRWUI (read-mode `_wfopen_s` only) — the shipped file is install-tree data; "user-editable" only in the filesystem sense (no in-app editor surface decoded).

### 6. Autosave policy — naming/gates known; **LastScanInfo.ini + retention=1 are new**
- Naming writer `FUN_1402857c0` (DRWUI:442747-443060), first-hand: dir `<exe-dir>\SaveScan` (GetModuleFileNameW → truncate at `\` → "SaveScan", :442833-842849, create-if-missing :442850-854); volume label fetched :442859-881; **sanitizer `FUN_140286a80` :442731-442757 replaces exactly `/ * : ? " < > |` (8 chars) with `_`**; `GetLocalTime` + `sprintf(L"%02d_%02d_%02d_%02d_%02d", Month, Day, Hour, Min, Sec)` :442949-961; inserts `"\auto_"` at position 0 of the label string (:442963), appends the stamp and `.rsf` (:442964-966) → `SaveScan\auto_<label>_<MM_DD_HH_MM_SS>.rsf` — matches all 3 shipped samples byte-for-byte. Existing target path is deleted before write (:442968-974).
- **Triggers**: exactly the three completion-family handlers (:612583, :615166, :616764) with the S-044 gates (not-import `+0x1c8==0`, results-found `FUN_1403964b0`; site 3 adds the `+0x388` already-saved check). **No timer/interval autosave exists** — first-hand at all three call sites.
- **NEW — the LastScanInfo.ini pointer file** (0 hits across `core_recovery/diff-to-fix` and the corpus artifacts; distinct from N-145's rwConfig.ini):
  - Location: `%APPDATA%\EaseUS\<subfolder>\LastScanInfo.ini` — resolver `FUN_14031e7a0` (DRWUI:541001-541057): `SHGetFolderPathA(CSIDL_APPDATA)` → `\EaseUS` (create) → subfolder constant `DAT_141211ee0` (create; exact string UNRESOLVED), with exe-dir fallbacks (:541018-541057). Not shipped in the install tree (explains its absence from og-codebase/).
  - **Writer** `FUN_140285f40` (:442975-443090): `WritePrivateProfileStringA` `[Info] SavePath=<full .rsf path>`, `Name=<display name>`, `Count=<found count>`, `Time=<FILETIME hex>` (:443064-443078), plus a **registry mirror** `HKLM\SOFTWARE\EASEUS\DRW` values `LastScanConfig` (ini path) and `LastScanFile` (rsf path) (:443079-443086). No static call site (vtable-dispatched on the CSaveAndRestoreResult/record singleton); after each auto-save the record is appended to the in-memory list (:442941-942).
  - **Reader** `FUN_140286460` (:443158-443215): reads Name(+8), SavePath(+0x28), Count(+0x48, `GetPrivateProfileIntA`), Time(+0x50, `_atoi64`→FILETIME→`FileTimeToSystemTime` :443199-443207); validates `PathFileExistsA(SavePath)` → flag (:443209-443214).
  - **Retention**: `FUN_1402857c0` reads the ini (:442815-442816) and **`DeleteFileA`s the PREVIOUS auto-saved .rsf (the ini's SavePath) before writing the new one** (:442817-442818) → the og keeps **exactly one live auto session**. (The shipped tree's 3 auto files are consistent only with the ini being absent/reset between QA runs — when the ini is live, the older file is deleted.)
  - **Resume**: `FUN_140286390` (:443092-443120) copies SavePath and restores via `FUN_140285020` (:443127); called from `SaveRestoreThread::run` (:1569345) when no explicit .rsf was chosen — the og's "Continue last scanning" is a **pointed** flow.
  - The auto-session **list** surface `FUN_140273940` (:432256-432341) enumerates `<dir>\*.*` keeping only files whose names contain **"auto_"** (:432326) **and ".rsf"** (:432331) — manual saves are excluded from that list.
- Core counterpart for §6: `autosave_if_due_` `src/ui/cr_ui.cpp:2256-2292` (gates :2257-2258 = autosave_dir + !is_resumed + !user_saved + results-found; name :2277), called from completion (:2406) **and** the ERR_CANCELLED stop window (:2387, S-057); CLI twin `src/app/rrecov.cpp:1879-1935` (stop window :1935). Naming `session::auto_session_name_at` `src/core/session/cr_crsf.cpp:363-393` ("auto_" :368, forbidden set :355-358, stamp `"%02d_%02d_%02d_%02d_%02d"` mon/mday/h/min/sec :385-387). Session list `session_list_impl` `cr_ui.cpp:3152-3222` sorts by `save_unix` DESC (:3204-3211) over **all** `*.crsf` (`list_crsf_files` :390-454 — no auto_ filter, no pruning, no delete of previous autosaves anywhere).

### 7. Persistent log channels — writers cited, **no readback anywhere**
- `Eaolog.log`: DRWUI `FUN_1404224e0` :696565-963578 — `CreateFileW(L"Eaolog.log", GENERIC_READ|WRITE, share-all, OPEN_ALWAYS)` + `SetFilePointer(END)` = **append-only**; formatter `FUN_1404225a0` (:696596+) stamps time/PID/TID. No `ReadFile` anywhere in the surrounding module family (grep-verified) → write-only telemetry.
- `strutureReg.log`: EVR:19285 — video-repair JSON debug logger built on `<dll-dir>\strutureReg.log` (single reference; no reader) → write-only.
- `reglog_win.txt`: EVR:35381 — `"./reglog_win.txt"` (CWD-relative), the integrationalg/movmp4repair trace channel; single reference, no reader → write-only.
- `log/DRWUI.log`: DRWUI:264253/:264337 (`%s\log\DRWUI.log`) — the `FUN_140162e00` funnel ("Build tree finish." etc.) → write-only.
- **Verdict: none of the og's local-DB-family logs is functional state** — no divergence filed for them.

## D. Divergence candidates

1. **Auto-save retention & the last-scan pointer: og keeps exactly ONE auto session (deleting the previous) and points at it via LastScanInfo.ini + an HKLM mirror; the core never prunes and has no pointer.**
   - Falsifiable: og — run two auto-saving scans; after the second, `SaveScan` holds only the second `auto_*.rsf` (delete at DRWUI:442817-442818 via the ini's SavePath read at :443187-443189, validated :443209-443214) and `LastScanInfo.ini` names it (:443064-443086). Core — `autosave_if_due_` (cr_ui.cpp:2256-2292) and rrecov's lambda (:1879-1935) contain no delete; `list_crsf_files` (:390-454) enumerates everything → N auto sessions accumulate (shipped og samples run 7-52 MB each — a scan-a-day user accumulates GBs where the og holds one).
   - Impact: user-observable disk-growth/behavior divergence + a different "continue last scanning" mechanism (pointed vs list+sort). Suggested severity: **MAJOR**.
2. **R-20's mechanism attribution is wrong: the og's "Unimportant files" bucket is fed by the usefullostfiles.xml gate, not the M-018 junk-name filter.**
   - Corpus: DRWUI:635886-635899 (ext∈useful-set test), :635850-635862 (name<3), :635869 + :636028-636041 (recursive any-useful subtree check) — none of these consult FilterFolder.ini. Core/doc: `docs/REFERENCE-API-GAP.md` R-20 ("'Unimportant files' ≈ M-018 junk filter"). Impact: a future implementer of the Unimportant branch would build the wrong predicate (junk-name hide ≠ useful-ext demotion; two different og layers). Suggested severity: **MINOR (doc-truth)**.
3. **config/version.ini `MaxFileSize ` key: an undocumented per-file size cap surface with two consumers; core has no equivalent.**
   - Corpus: DRWUI:337924 — IDirectoryTree ctor (`FUN_1401d1210`, the 0x5c0-alloc object also at :318937) reads `FUN_1401d08e0("config/version.ini", <section>, "MaxFileSize ", 0x400)` and stores `value << 0x1e` (GiB→bytes) at +0x60; DRWUI:749902 — the scan-task config object (ctor `FUN_140478d10`, which also loads FilterFolder.ini FileName at :749898) stores the same shifted value at +0xa0. Default 1024 → 1 TiB; the shipped version.ini has **no** MaxFileSize key → default applies. What the cap gates is **UNDECODED** (no consumer of the +0x60/+0xa0 fields traced this session). Core: `rg MaxFileSize` → 0 hits. Suggested severity: **MINOR** (semantics undecoded — flag for a consumer-trace follow-up). Adjacent registers: version.ini `VHDSimulateUSB` gate (DRWUI:318219) and remote key `value_vrpartisize` (GiB→bytes, :318239-318241 / :322318-322325 — VR-partition simulate sizing), both likewise uncovered.
4. **FileFilter.xml `Selected=2` (root attr + 15 child elements) is dead data in the shipped format** — no reader in any decompiled binary (grep record: only Qt stylesheet-state variants at DRWUI:1209830/:1209887). Core correctly omits it (M-023 table has no such field). Suggested severity: **INFO** (register so no future lane burns time on it).
5. **The og's summary-filetype groups ("SumaryFileType") are remote-tunable at runtime** — `value_sumaryfiletype`/`value_fastgoodfile` keys exist in the shipped `remote_config_data` cache and are read into the results-config singleton (DRWUI:319007-319022, stored at +0x280); the core's N-132 table is the ini's static groups. Precedence between the remote value and any ini default is UNDECODED. Remote-config layer is REPORTING-OOS by prior disposition. Suggested severity: **INFO register**.

## E. MATCH confirmations

- **Auto-session naming**: og `"\auto_"+<label>+"%02d_%02d_%02d_%02d_%02d"+".rsf"` (DRWUI:442949-442966) ≡ core `auto_session_name_at` (`cr_crsf.cpp:363-393`) — byte-parity re-verified against all 3 shipped samples (N-061).
- **Name sanitizer**: og replaces exactly 8 chars (DRWUI:442731-442757) vs core's superset (+`\` + C0 range, `cr_crsf.cpp:355-358`) — already registered D-110 ACCEPTED SUPERSET; re-confirmed.
- **PriorityDisplayType table**: core's 35-entry/5-category order table (`cr_config_display_data.cpp:44-116`) is byte-exact vs the XML read first-hand — N-132 landing verified.
- **SumaryFileType groups**: core's 11 groups (`cr_config_display_data.cpp:27-35`) = the ini's 11 groups verbatim.
- **RawConfig gating semantics**: og `value=='1'`→enabled else disabled (RAW:13175-13196) ≡ core's `carve_token_listed` gating of all three carve layers (`cr_carve.cpp:1441-1461`, `cr_usersigns.cpp:365-379`).
- **FileFilter membership data**: core's generated 1,481-unique-ext/8-category table = the XML's 1,489 entries minus the 8 documented cross-listed dupes (`cr_filefilter_data.cpp:1-5,1509`); the og's iconIndex remap table verified (DRWUI:714626-714657).
- **Autosave trigger gates + stop window**: core `cr_ui.cpp:2257-2258/:2387/:2406` + `rrecov.cpp:1879-1884/:1935` implement the og's not-import/results-found/(already-saved) gates incl. the stop window (S-044/S-057, re-verified at HEAD).
- **raw_signs content parity**: og runtime-file table (RAW:23836) vs core compiled-in generated table — same corpus source, delivery vehicle differs (file vs generated code); known landed state, no defect.
- **Log channels**: all four og local-DB-family logs verified write-only telemetry → core's single-sink seam (cr_log.h) needs no per-channel state; consistent with N-048's registered divergence.
- **Prior-sheet spot checks** (S55-D1's EUUserData loader cites :1005387/:1005497+, M-129's gates, I-027's runtime-log dispositions): all re-confirmed accurate this session.

**Not decoded (honest register)**: the consumer of the `MaxFileSize` cap fields (+0x60/+0xa0); the precedence/consumer of `value_sumaryfiletype` beyond the singleton store; the subfolder constant `DAT_141211ee0` in the APPDATA path; whether `[Info] Name` is used anywhere besides display (SavePath is the restore+delete key); the static call site of `FUN_140285f40` (vtable-dispatched); the og-side sorting that consumes the attached alias maps inside the tree-node class (the attach site is decoded, the sort itself is below the node vtable).
