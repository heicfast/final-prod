# PARITY-UI — Pages / State / Wiring Parity Audit (Task 2-c)

<!-- session-96: the driver era ended - the kernel driver was replaced by direct raw I/O; the driver-era rows below are historical records. -->

**Auditor:** parity sub-agent 2-c (read-only; no repo modifications — only this file + worklog entry written)
**Date:** 2026-10-23

## Pins (audit basis)

| Repo | Commit / tag | Role |
|---|---|---|
| rda-wizard | `c2eaad6` + `c083bdd` (HEAD `c083bdd1`, clean tree) | ours — .NET WPF MVVM; **Core + Integration shipped, App layer not yet created** |
| core_recovery | `d42bb00` (HEAD; frozen-contract pin `183dfcd` cross-checked via `git show`) | ours — engine + `rrecov` CLI (the integration target) |
| rda-driver | not audited here (TRANSPORT-CONTRACT used as claims) | ours — kernel transport |
| easeus-drw-research | `18d6f14` | RE corpus (evidence base) |

**Coverage:**

| Target | Read | Coverage |
|---|---|---|
| rda-wizard `src/RdaWizard.Core` (28 files, ~3,000 LOC) | all, line-by-line | **100 %** |
| rda-wizard `src/RdaWizard.Integration` (8 files, ~1,750 LOC) | all, line-by-line | **100 %** |
| rda-wizard `tests` (9 files, 1,271 LOC; 99 cases counted statically) | all (fixtures cross-checked against engine source) | **100 %** |
| docs: UI-MAP.md (992), AUDIT-UI.md (259) | full | 100 % |
| docs: CORE-CONTRACT.md (§13–§15 full + TOC), TRANSPORT-CONTRACT.md (§9–§10), DESIGN-SYSTEM.md (skim, App concern) | targeted | ~30 % (by design — claims docs) |
| corpus: architecture/07-user-flow (full); UIUX artifacts 04, 08, 09, 11, 19, 31 (full); DRWUI-exe--main-ui-architecture §1–3.8; Languages census; FileFilter.xml (category census, 1,538 lines); `og-codebase/language/` listing; `re-workspace/reversed/DRWUI.exe/` listing (no .ui files exist — Qt UI is code-built; evidence = moc/RTTI/strings) | targeted | ~15 primary docs of the 40-artifact UIUX set (highest-risk claims) |
| core_recovery `src/app/rrecov.cpp` (1,234) | all output-format regions (~600 lines: list/tree/scan/recover/health/fix/usage) | ~50 % (complete for every format our parsers touch) |

**Method:** our code first → docs second (claims) → corpus + engine source third (evidence). Every MISMATCH below is independently checkable from the citations alone. Prior agents' docs (UI-MAP, AUDIT-UI ADR-1..12, CORE-CONTRACT) were treated as **claims to audit, not evidence**; the corpus and the actual engine source are the evidence. Tests were **not executed** (no dotnet SDK in this sandbox); the "99 green" claim is count-verified (91 `[Fact]` + 8 `[InlineData]` cases = 99; Core 52 / Integration 47) but execution-UNVERIFIED.

---

## 1. Audit grid — pages & subsystems

Verdict key: MATCH / MISMATCH / MISSING / DELIBERATE-DELTA (documented) / UNDOCUMENTED-DELTA / IN-FLIGHT (App layer not built) / UNVERIFIED.
"og" citations: UIUX artifacts under `re-workspace/artifacts/`, `architecture/07-user-flow-uiux.md` (U07), UI-MAP §.

| # | Surface (ours) | og counterpart (evidence) | Verdict | Notes / key citations |
|---|---|---|---|---|
| 1 | **Shell/`MainViewModel`** (state machine owner, stepper phases, busy gate) — MainViewModel.cs:14-185 | `DrwMainViewManagerWizard` 4-page QStackedWidget (UIUX/04 §1-2) | **DELIBERATE-DELTA** | og = stack-index switching, scan inside results page; ours = 6-state machine + 10 PageKeys; documented (WizardState.cs:4-9, UI-MAP §4 reduction, ADR-12). Stepper phases Select/Scan/Recover (WizardState.cs:36-52) = ADR-12 documented improvement |
| 2 | **`WizardStateMachine`** 13 edges + guards — WizardStateMachine.cs:31-70 | og 30-edge table (UI-MAP §4.1:432-463; evidence UIUX/04 §4/§6, 08 §3-4, 11 §1-6) | **MATCH (reduction) + 1 DELTA + 2 gaps** | All 13 edges trace to og edges (see §2 below). Stop-semantics delta (og preserves partial results & resumable, ours discards) = ADR-3/G-07 documented. Quick→advanced continuation has no counterpart → F-C-10; BitLocker unlock chain & smart-partition scan (og P05/P09) have no UI → §5 |
| 3 | **`PageKey`/`NavigationService`** state-gated pages + back stack — PageKey.cs:8-39, NavigationService.cs:18-114 | og: sidebar + stack + scenario windows (UIUX/04 §2; U07 §1) | **MATCH + 2 P3 defects** | State-pages table (NavigationService.cs:21-26) is coherent with the machine; back-refusal destroys history entry (F-C-11); ResetToHome rewrites the gate it does not own (F-C-12) |
| 4 | **Home VM** (device list, driver status, recent sessions, import) — HomeViewModel.cs | og P02/P03 landing (UI-MAP §3.3-3.4; U07 §1) | **MATCH (core) + IN-FLIGHT + omissions** | Device cards ≈ og tiles; "Continue last scanning" ≈ RecentSessions (metadata sparse, G-14 documented). og "Specify a location" trio (Recycle Bin/Desktop/Select Folder — UI-MAP §3.3:157-158) not modeled; recycle-bin scan special case (og `RecycleBinScanNoRltWnd`) absent — undocumented → §5 F-C-19 |
| 5 | **ScanConfig VM** (mode, threads, name filter, deleted-only, category chips) — ScanConfigViewModel.cs | og: modes surfaced on results/scan page, filter pushed into scan controller (UI-MAP §5:566-568; UIUX/31 §7) | **DELIBERATE-DELTA** | Separate step = stepper adaptation (ScanConfigViewModel.cs:11-14, documented). Flags map exactly to rrecov grammar (verified vs rrecov.cpp usage :1203-1210 + cmd_scan) |
| 6 | **ScanProgress VM** (truthful %/elapsed/ETA/read-errors; no pause; destructive cancel) — ScanProgressViewModel.cs | og P06 bottom bar: gauge, files-found live, remaining time, Pause/Stop (UI-MAP §3.6.1:215; 08 §1) | **DELIBERATE-DELTA + 1 P1** | ADR-2/3/4 deltas documented (files-found at completion, no pause). Live read-error claim NOT implemented → **F-C-2 (P1)**. ETA wording vs ADR-4 → F-C-13 |
| 7 | **Results VM** (filter bar, tree/list modes, selection, preview node, session copy) — ResultsViewModel.cs | og P06 results anatomy (UI-MAP §6; UIUX/09) | **PARTIAL / IN-FLIGHT** | Path/Type + Detail/Icons modes ✓ (ResultsViewModel.cs:188-198); 5 special branches (Tags/Name-Lost/Path-Lost/Versions/Unimportant — UIUX/09 §2) not modeled (carved/lost groupings exist via `LoadedSession.CarvedNodes/LostNodes`); only 2 of 8 filter dimensions exposed → F-C-17; virtualization promised, absent (ADR-9) → §4 |
| 8 | **Recovery VM** (destination, preflight, queue, cancel, report) — RecoveryViewModel.cs | og P23/P24/P25/P26 funnel (UIUX/11; UI-MAP §3.12, §8) | **MISMATCH ×3** | Selection ignored by engine request → **F-C-3 (P1)**; space gate ≠ og P25 semantics → **F-C-4 (P1)**; progress dead during run → F-C-6 (P2); collision/folder-structure dead controls → F-C-7 (P2); imported-session source disk −1 → F-C-8 (P2). Hard overlap gate ADR-7 ✓ (machine guard + preflight + engine F12 defense-in-depth) |
| 9 | **Completion VM** (success/partial/failed/cancelled headlines, open destination, report) — CompletionViewModel.cs | og P26 `RecoverResultDlg` (UIUX/11 §5; U07 §5) | **MATCH (reduction)** | Truthful outcome mapping incl. exit-2 partial (RecoveryModels.cs:36-65 ↔ rrecov.cpp:716-722); no survey/urgency upsell (omitted per UI-MAP §13.8) ✓ |
| 10 | **Health VM** (SMART table, NVMe, SSD heuristic, TRIM, warnings) — HealthViewModel.cs + HealthOutputParser | og P31 DiskHealth.exe QML dashboard (UIUX/19 §3) | **MATCH (report-only reduction)** | Parser verified line-by-line vs rrecov.cpp:759-804 incl. the `read=%llu units …` NVMe detail row — all five regexes match the engine exactly. G-15/G-16 truthful degradation ✓ (SmartUnavailable, NeedsElevation). Tray/auto-start/buy surfaces omitted (policy) ✓ |
| 11 | **Repair VM** (dry-run → typed ACK gate → confirm → apply → undo) — RepairViewModel.cs + FixOutputParser | og P20-P22r Fixo file-repair wizards (UI-MAP §3.13) | **DELIBERATE-DELTA (documented)** | Ours fronts *our* RawFixer (`rrecov fix`), not og's video/PDF repair — UI-MAP §13 omit "EU repair executables beyond core-provided repair". Parser verified vs rrecov.cpp:1135-1192 (REFUSED/clean/plan/`[kind] offset= len= : reason`/execute OK/undo log saved). "Maximum gravity" consent gates exceed og ✓. P3: ApplyAsync greps "undo FAILED" but apply path prints "execute FAILED" → F-C-15 |
| 12 | **Settings VM** (theme, language, scan mode, destination, collision, log, preview caps) — SettingsViewModel.cs | og P32 menu Settings submenus (Theme/DPI) — contents `[CORPUS-AMBIGUOUS]` (UI-MAP §3.14:419-420) | **MATCH (adjudicated)** | AUDIT-UI §5.1 adjudication followed; no DPI/zoom setting (og has Display zoom 100/150/200 % — UI-MAP §3.14:418) → minor undocumented reduction (§5) |
| 13 | **About VM** (version, driver, engine, licenses) — AboutViewModel.cs | og P33 About family (UI-MAP §3.14) | **MATCH (reduction)** | OEM variants omitted per §13.12 ✓ |
| 14 | **Filters: `ResultFilter`/`FilterEvaluator`** — ResultFilter.cs | og `CFilterBar` family, `AdvanceFilterCondition` 7 groups (UIUX/31 §1/§6; FileFilter.xml) | **PARTIAL** | Model covers name/path/category/date/size/confidence/extension/deleted/raw (all 7 og dimensions) with honest date-null rule (ResultFilter.cs:109-113 ↔ G-24) ✓. Taxonomy: 7 categories vs og 8 + Databases tab + 9 search types incl. "Unsaved File" → F-C-19 |
| 15 | **Engine host** (`EngineProcessHost`) — \r/\n line splitting, bounded head/tail+kept retention, kill-tree cancel, timeout | og: engines in-proc via DLLs + `CStopScanThread` cooperative cancel (DRWUI-exe--main-ui-architecture §2; UIUX/08 §6) | **DELIBERATE-DELTA + MATCH (discipline)** | ADR-1 subprocess isolation documented. Progress `\r` handling matches rrecov.cpp:83 exactly; cancel = Kill(entireProcessTree) and session only written on success (rrecov.cpp:503-534) → ADR-3 semantics hold ✓ |
| 16 | **`ScanService`** flag grammar + summary/transport parsing — EngineServices.cs:62-199 | CORE-CONTRACT §13 ↔ rrecov.cpp (wmain :1199-1210, cmd_scan :481-536) | **MATCH** | `scan --disk|--volume / --quick-only / --no-carve / --threads / --filter / --deleted-only / --out / --log / --json` byte-verified vs engine; `--deep` only for whole-disk deep modes (guarded, EngineServices.cs:166-173) ✓; JSON keys files/deleted/carved/lost_partitions/bytes_scanned/elapsed_ms ✓ (rrecov.cpp:508-511); exit-0/1 + "scan failed:" ✓; G-20 (never trust list exit) ✓ in enumeration |
| 17 | **`SessionService`** (list/load CRSF) — SessionRecoveryServices.cs:17-95 | og .rsf sessions (UI-MAP §10; ADR-10) | **MATCH (documented delta)** | CRSF-only per ADR-10/G-11 ✓; corrupt-session → engine diagnostic, never partial tree ✓ (rrecov.cpp:570-582 "cannot open session"/"session load failed"); auto-save-on-completion holds via engine `--out` ✓; session metadata sparse = G-14 documented |
| 18 | **`ListOutputParser`** — OutputParsers.cs:12-91 | CORE-CONTRACT §13 ↔ rrecov.cpp:227-250 `print_disks` | **MISMATCH (P0)** | Contract says `[removable|fixed] [GPT|MBR|no-table]` (two bracket groups); the **engine prints the table kind unbracketed** (`"  Disk %u: %llu MiB (%s) %s [%s] %s\n"`, rrecov.cpp:237-242, verified at both `183dfcd` and HEAD) → **F-C-1** |
| 19 | **`TreeCsvParser`** — TreeCsvParser.cs | CORE-CONTRACT §13 ↔ rrecov.cpp:285-326 `print_tree` | **MATCH (with P3 notes)** | Grammar `"path",size,origin,deleted,confidence,extent` ✓; engine does **not** escape embedded quotes (plain `"%hs"`, rrecov.cpp:300) so our naive first-quote parse matches engine reality; dirs derived by successor ✓ engine emits no dir flag; empty dirs misclassify as files → F-C-16 |
| 20 | **`RecoveryService`** — SessionRecoveryServices.cs:98-359 | rrecov.cpp:592-723 `cmd_recover` | **MATCH (semantics) + P2s** | exit 0/1/2 + `recovered: N done, N refused, N error(s), N hash mismatch` ✓ (:720-722); `[F12]`/queued lines kept ✓; overlap preflight → F-C-5; space gate → F-C-4; progress parsing misses engine percent lines → F-C-6 |
| 21 | **`DriverStatusDetector`** — DriverStatusDetector.cs | TRANSPORT-CONTRACT §9 five-state model | **PARTIAL / DELIBERATE-DELTA** | All five states exist (HealthReport.cs:100-119); GLE 2/5 mapping + SCM installed-vs-not ✓; but no IOCTLs → WrongVersion unreachable, LoadedOk can mask an ancient driver (contract: open+GET_INFO+GET_VERSION all needed). Documented in-class (DriverStatusDetector.cs:11-17, :46-48); WireVersion deferred to engine `[transport]` line ✓. Label collapses states → F-C-18 |
| 22 | **Localization** — LocalizationService.cs | og `language/` 25 .qm + 4 OEM (Languages census §1; U07 §7) | **DELIBERATE-DELTA + MISSING adoption** | EN-first documented (UI-MAP §13.11); JSON tables + English fallback + system-resolve mirror og's Qt fallback shape ✓. Zero string tables exist and every VM string is an inline literal → **F-C-9 (P2)** |
| 23 | **Threading** — ViewModelBase.PostOnUi, `Progress<T>`, ConfigureAwait(true), linked CTS | og 45-thread architecture, workers at TimeCritical (DRWUI thread artifact) | **MATCH (discipline)** | No Dispatcher references in Core ✓; engine-thread → UI marshaling designed via marshaler (install = App, in-flight); cancellation propagates to Kill(processTree) ✓ (EngineProcessHost.cs:119-146) |
| 24 | **Feature gates / monetization** | og 12 free/pro gates (UI-MAP §12) | **DELIBERATE-DELTA (policy)** | Zero license/quota/retention surfaces in code ✓ — consistent with UI-MAP §13.8 omit + AUDIT-UI §4 "every recovered byte is real" |
| 25 | **Error surfaces** | og ~30 dialogs (UI-MAP §11) | **PARTIAL** | Session-load failure, repair confirmations, report-save alerts implemented via `IDialogService`; og's Retry/Cancel triads largely belong to omitted NAS/cloud chains; local drive-removed-mid-scan Retry/Cancel has no counterpart → F-C-18 |
| 26 | **App layer (views, dialogs, preview, wiring)** | — | **IN-FLIGHT** | `src/RdaWizard.App` does not exist; see §4 inventory |

---

## 2. State-machine trace (ours ↔ UI-MAP ↔ corpus)

Our 13 edges (WizardStateMachine.cs:33-69) vs UI-MAP §4.1's 30 og edges (lines 432-463). Corpus evidence for the og side: UIUX/04 §2/§4/§6 (4-page stack, start idx 2 = `PeripheralHomeView`, `scanClickedSlot` → idx 3 + scan start), UIUX/08 §3-§4 (stop-confirm, quick-complete, completion), UIUX/11 §1-§6 (destination → progress → result), UIUX/09 §1 (`sigLoadFinished` session import).

| Ours (edge, guard) | og edge(s) | Verdict |
|---|---|---|
| Select→Configure `BeginScanSetup` (needs target) | #3 scan click → P06 | DELTA documented: separate Configure step (ADR-12/stepper; ScanConfigViewModel.cs:11-14) |
| Select→Results `ImportSession` (needs session) | #25 session Load → P06 restored | MATCH |
| Configure→Select `BackToSelect` | #19 Back/Home (`CGoHomeThread`) | MATCH |
| Configure→Scanning `StartScan` (needs target) | #3 scan start | MATCH |
| Scanning→Results `ScanCompleted` (needs session) | #9 full completion → results | MATCH (celebration P08a omitted per §13.8-adjacent policy; ADR-2 defers live tree) |
| Scanning→Select `CancelScan` (confirm; results lost) | #5-#7 Stop → confirm → stopped (og **keeps** partial results, resumable) | DELIBERATE-DELTA (ADR-3/G-07 — engine writes sessions only on success, rrecov.cpp:521-534) |
| Results→Configure `ChangeScanSetup` | #19 (+ new scan) | MATCH |
| Results→Select `BackToSelect` | #19 | MATCH |
| Results→Recovering `StartRecovery` (session + selection + overlap-passed) | #13-#15 Recover → P23 → P24; P23 source-block "Cannot save to original partition" (UIUX/11 §2) | MATCH + ADR-7 up-front hard gate (defense in depth over engine `[F12]`, rrecov.cpp:690-694) |
| Recovering→Complete `RecoveryFinished` | #17 progress → P26 | MATCH |
| Recovering→Results `CancelRecovery` (confirm; partial kept) | P24 cancel-confirm; og keeps completed files | MATCH |
| Complete→Results `BackToResults` | P26 Close → P06 | MATCH |
| Complete→Select `Finish` | P26 → home | MATCH |

**Og edges with no counterpart in ours** (all cross-checked against UI-MAP §13 omit/replace lists):
#1-2 splash (REPLACE-4 keeps a splash enumeration gate — Home `IsLoading` covers the state; splash chrome = App in-flight); #4 quick-pass → P07 upsell + background advanced (REPLACE-10 says *keep* two-phase UX as non-blocking banner — **not modeled anywhere** → F-C-10); #8 Pause (ADR-3 documented); #14/#18/#20/#21 retention/quota/exit interception (omit §13.8 ✓); #22 BitLocker unlock chain (NOT in omit list → §5); #23 scenario chains NAS/Linux/RAID/Cloud/WinPE (omit §13.1-§5 ✓); #24 smart partition scan P09 (replaced by whole-disk `--deep` scan with `[lost]` roots — documented in code, ScanOptions.cs:4-8, not in UI-MAP §13 → §5); #26 repair branch (replaced by RawFixer, documented); #27 NAS overlay (omit ✓); #28 Add-Driver (omit §13.13 ✓ — replaced by driver-status surface, implemented); #29 crash reporter (REPLACE-3 ✓); #30 drive-removed dialog (→ F-C-18).

**Page-set parity:** PageKey 10 keys ↔ og 50 inventory rows; every omitted family is covered by UI-MAP §13 omissions **except** the items flagged above (BitLocker P05, smart-partition P09, recycle-bin/Desktop location trio, quick-scan banner).

---

## 3. Findings

Severity: P0 (breaks a core flow against evidence) / P1 (wrong behavior or missing promised behavior on a main flow) / P2 (parity/contract gap on a shipped surface) / P3 (polish, dead code, doc-code drift).

---

### F-C-1 — P0 · Axis: Integration/parsers · Verdict: **MISMATCH (code vs engine; root cause = CORE-CONTRACT transcription error)**

- **Ours:** `src/RdaWizard.Integration/Parsers/OutputParsers.cs:18-20` — `DiskLine` regex requires `\[(removable|fixed)\]\s+\[(GPT|MBR|no-table)\]$` (two bracket groups). Golden fixture enshrines the same shape: `tests/RdaWizard.Integration.Tests/Parsers/OutputParserTests.cs:20,23,88` (`… [fixed] [GPT]`). The unused `[GeneratedRegex] DiskLineRegex()` at OutputParsers.cs:14-15 has the same defect.
- **Evidence (engine, primary):** `core_recovery/src/app/rrecov.cpp:237-242` — `wprintf(L"  Disk %u: %llu MiB (%s) %s [%s] %s\n", number, size>>20, model, serial, removable?L"removable":L"fixed", table)` — **six format specifiers; the table kind is a bare trailing token**, not bracketed: real output is `  Disk 0: 476940 MiB (Samsung…) S4EX [fixed] GPT`. Verified identical at the contract pin `183dfcd` (`git show 183dfcd:src/app/rrecov.cpp`) and HEAD `d42bb00`, so this is **doc-born drift**: CORE-CONTRACT §13 (line ~629) transcribed `[GPT|MBR|no-table]` with brackets the code never prints. Additionally the engine can print `-` (table_ok, layout neither Gpt nor Mbr) — not in our alternation.
- **Impact:** against the real engine, `ListOutputParser.TryParse` parses **zero disks**; `DeviceEnumerationService` (EngineServices.cs:29-42) then reports "The engine reported no disks" on every launch — the Home page, and therefore the entire product, never gets past first enumeration. The 99 passing tests pass because the fixture shares the contract's error (tests validate the fixture, not the engine).
- **Suggested fix:** regex → `^Disk\s+(\d+):\s+(\d+)\s+MiB\s+\((.*?)\)\s+(\S+)\s+\[(removable|fixed)\]\s+(GPT|MBR|no-table|-)$`; update the golden fixtures (incl. a `-` case); correct CORE-CONTRACT §13; add a CI contract test that runs the actual `rrecov list` on a Windows runner and diffs parser output (G-17 discipline).

### F-C-2 — P1 · Axis: Data flow & wiring / progress truthfulness (ADR-4) · Verdict: **MISMATCH (promised behavior not implemented)**

- **Ours:** `EngineServices.cs:89-99` — `StageLogParser.TryParseReadError` is applied to **stdout** lines inside `ScanService.OnLine`. `ScanOptions.cs:94` claims the stage log is "read live for read-error ranges"; `ScanProgressViewModel.cs:12-13` and `ScanProgress.cs:19` repeat the claim; `ReadErrors`/`ReadErrorBytes` surface in the VM (ScanProgressViewModel.cs:30-31, 93-95).
- **Evidence (engine):** `rrecov.cpp:109-118` — `log_line()` writes **only to the `--log` file** (`g_log`); `rrecov.cpp:497-500` emits `  read-error range: @%llu +%llu (attempts %u)` via `log_line`. Stdout during scan carries only `[transport]` lines (:139-156), `\r` progress (:83), summary/JSON (:504-516), the tree, and `session saved:` (:534). No code in either project tails the stage-log file (`rg StageLogPath` → only the `--log` argument construction, EngineServices.cs:193-195).
- **Impact:** live read-error ranges (the bad-sector indicator — og surfaces bad-sector coaching, UI-MAP §11 `BadSectorTipsDlg`) will always be empty. ADR-4's explicit commitment ("read-error ranges stream live from the stage log … parsed real-time", AUDIT-UI.md:85-88) is unmet by the shipped Integration code.
- **Suggested fix:** add a stage-log tailer started alongside the scan (file-poll from `options.StageLogPath`, parse `read-error range` + `scan end` lines, merge into the same `ScanProgressSnapshot`); alternatively file a NEEDS-CORE-ISSUE asking the engine to mirror read-error lines to stdout and document the interim degradation.

### F-C-3 — P1 · Axis: Recovery funnel semantics · Verdict: **MISMATCH (vs og P23/P24)**

- **Ours:** `RecoveryViewModel.cs:79-89` — `RecoveryRequest` is always built with `NameFilter = null, DeletedOnly = false` (the comment admits "Selection-wide recovery is implemented by the engine's full-session queue plus UI-side subset filters" — **no such subset filters exist**); `RecoveryModels.cs:4-20` — the request model cannot express an arbitrary node subset at all (only name-substring/deleted-only).
- **Evidence (og):** UIUX/11 §1 + UI-MAP §3.12 — P23 header "You have N file(s) (size) to recover."; P24 counters "Recovered: %1/%2 files" (artifact 11 §3, QM-verified). The og recovers the *selection*; U07 §5 "Select checkboxes (files/folders; CSelNodeThread) → Recover".
- **Impact:** with a 3-file selection the engine recovers **every file in the session** (`rrecov recover` enqueues every matching node, rrecov.cpp:676-698). The wizard even *guards* on `HasRecoverySelection` (WizardStateMachine.cs:57) and then discards the selection — an untruthful surface by the project's own standing rule.
- **Suggested fix:** short term — distinct "Recover selection" vs "Recover all" with selection recovery implemented as UI-orchestrated per-node `--filter <exact-name>` runs (engine filter is a name substring, rrecov.cpp:688) or repeated single-item sessions; long term — NEEDS-CORE-ISSUE for a selection manifest / per-path recover. At minimum, block selection-recovery honestly until then.

### F-C-4 — P1 · Axis: Destination gates (P25 parity) · Verdict: **UNDOCUMENTED-DELTA (weaker than og)**

- **Ours:** `SessionRecoveryServices.cs:139-151` — free-space check is a **fixed 64 MiB floor**, hard refusal, no comparison against the selected byte total, no proceed-anyway path.
- **Evidence (og):** UI-MAP §3.12:355-357 + §11 row (`P25` gate, `[DYN]`): "Insufficient Disk Space… Need an additional 146.41 GB…" with **Continue (proceed anyway) / Reselect (primary)** — i.e., og computes needed-space from the selection and offers a soft gate. UIUX/11 §2 (QM-verified).
- **Impact:** a 500 GB selection onto a disk with 100 GB free passes our preflight (only the 64 MiB floor applies) and fails mid-recovery with engine errors; conversely a tiny selection on a <64 MiB-free disk is hard-refused with no continue option. Neither AUDIT-UI §5 nor UI-MAP §13 records this delta.
- **Suggested fix:** compute needed ≈ `SelectedBytes` (available in `ResultsViewModel.SelectedBytes`, ResultsViewModel.cs:184) + margin; refuse when `free < needed`; keep an explicit "continue anyway" secondary action (matching og P25) or document the hard-gate deviation in UI-MAP §13.

### F-C-5 — P2 · Axis: Truthful degradation (ADR-7) · Verdict: **MISMATCH (silent pass)**

- **Ours:** `SessionRecoveryServices.cs:310-338` — `TryMapRootToDisk` returns false on any WMI failure (catch-all :332-336) and the caller then adds **no refusal and no warning** (only `OperatingSystem.IsWindows()` guards the call, :115). The comment (:334-335) claims "overlap check reports 'unavailable' (truthful)" — nothing is reported.
- **Evidence:** ADR-7 (AUDIT-UI.md:117-125) — "the UI computes disk identity … and refuses up-front with a hard gate"; TRANSPORT-CONTRACT §9 verdict discipline (label truthfully when a check cannot run).
- **Impact:** `DestinationOverlapCheckPassed` in `WizardContext` will read as "passed" whenever WMI is unavailable; the up-front gate silently degrades to engine-only `[F12]` protection (items then counted "refused" at run time — visible only post-hoc).
- **Suggested fix:** emit a distinct preflight result state ("overlap could not be verified — the engine will refuse overlapping items") so the shell can either block or badge the gate as degraded; never conflate "checked-OK" with "not checked".

### F-C-6 — P2 · Axis: Recovery progress semantics · Verdict: **MISMATCH (dead progress)**

- **Ours:** `SessionRecoveryServices.cs:178-192` — the only progress parse is `Regex.Match(line, @"(\d+)\s+done")`, which matches only the **final** `recovered: N done, …` line (rrecov.cpp:720). `RecoveryViewModel.cs:117-127` — the progress callback sets `FilesDone`/`ElapsedText` but **never `Percent`**; Percent goes 0 → 100 across the run (:118, :132).
- **Evidence (engine + og):** the engine *does* stream live progress during recover — the same `ProgressDisplay` `\r  %5.1f%%  (%lld s)` lines (rrecov.cpp:706-715, callback `done*1000/total`). og P24 shows per-file + aggregate progress and "Recovered: %1/%2 files" (UIUX/11 §3, QM-verified; UI-MAP §3.12:361-363).
- **Impact:** our recovery surface shows a frozen bar for the whole run — parity gap vs og and a truthfulness gap vs our own scan page.
- **Suggested fix:** parse the percent line in `RecoverAsync` (same regex as `ScanOutputParser.TryParseProgress`) and map `done/total` (or fraction) → `RecoveryProgress.FilesDone/BytesDone` + a `Percent` in the VM.

### F-C-7 — P2 · Axis: Dead controls / collision policy (AUDIT-UI §5.7) · Verdict: **MISMATCH (claimed behavior unimplemented)**

- **Ours:** `RecoveryViewModel.cs:26-27, 58-59` — `CollisionPolicy` and `PreserveFolderStructure` are loaded from settings and exposed, but never influence `RecoveryRequest` (RecoveryModels.cs:4-20 has no such fields) or the engine invocation (`rrecov recover` accepts only `--deleted-only/--filter`, rrecov.cpp:1206). `RecoverySummary.Items`/`RecoveredItem`/`RecoveredItemState` queue models exist (RecoveryModels.cs:48-84) but **nothing ever populates `Items`**.
- **Evidence:** AUDIT-UI.md §5.7 — adjudicated rule: "never silently overwrite; auto-rename `name (2).ext` with an explicit policy picker (rename/skip/ask), stated in the recovery confirmation". og collision behavior is `[CORPUS-AMBIGUOUS]` (UI-MAP §8.7:694-698). Engine DestIo same-name behavior: **UNVERIFIED** (not decoded in this audit).
- **Impact:** violates the project's own "no dead buttons" standing rule (the same rule the ScanConfig page cites for refusing fake read-retry controls, ScanConfigViewModel.cs:14-16).
- **Suggested fix:** implement rename/skip/ask UI-side (pre-scan the destination for collisions; rename before invoking; "ask" pauses the queue), populate `Items` from engine kept-lines/progress, or remove the controls until implemented. Verify engine collision behavior and file a NEEDS-CORE-ISSUE if it silently overwrites.

### F-C-8 — P2 · Axis: Session import → recovery wiring (G-14/G-21) · Verdict: **MISMATCH (promised degradation surface absent)**

- **Ours:** `RecoveryViewModel.cs:188-194` + `SessionSourceRegistry` (:214-224) — `ResolveDisk` returns **-1** for any session not scanned in this process (imports, app restarts). No code path lets the user re-select the source disk; `SessionService.ListRecentAsync` leaves `TargetDescription = null` (SessionRecoveryServices.cs:83, G-14).
- **Evidence:** AUDIT-UI gap table (AUDIT-UI.md:215) — "Session reload: tree restored; **source disk re-selected manually** (G-14/G-21)". Engine requires `--disk N` (rrecov.cpp:617-619) and parses it with `wcstoul` — `-1` becomes 4294967295 → "cannot open disk …" setup failure with a confusing diagnostic.
- **Suggested fix:** preflight refusal when `SourceDiskIndex < 0` ("choose the source disk for this session") + a disk picker fed by `IDeviceEnumerationService`; or persist a sidecar (session→disk) next to the sessions folder to restore the mapping across restarts.

### F-C-9 — P2 · Axis: Localization structure parity · Verdict: **MISSING (adoption)**

- **Ours:** `LocalizationService.cs` — complete infrastructure (JSON tables `strings/en.json`/`zh-CN.json`, English fallback :58-66, system-culture resolve :107-117, missing-file degradation :124-127). **No `strings/` directory exists anywhere in the repo**, and every user-visible string is an inline English literal: guard reasons (WizardStateMachine.cs:34-65), headlines (CompletionViewModel.cs:45-61), diagnostics (HomeViewModel.cs:66,83,143; ScanProgressViewModel.cs:111,117; RecoveryViewModel.cs:106,156; RepairViewModel.cs:73,94-103,118,139), settings labels, eta prefix (ScanProgressViewModel.cs:84).
- **Evidence:** UI-MAP §13.11 (UI-MAP.md:875-877) — "the rewrite ships EN first; **keep the string-table discipline (no hardcoded literals scattered in markup)** so localization remains possible"; og ships 25 `.qm` catalogs (Languages census §1; `og-codebase/language/` listing).
- **Impact:** the discipline the docs mandate is not held by the shipped Core; retrofitting keys post-App will be expensive.
- **Suggested fix:** extract VM strings to keys now (guard reasons and headlines first — they are the user-facing "truthful degradation" surfaces), ship `strings/en.json` + `zh-CN.json`, and add a test asserting no VM file contains user-facing literals (allowlist for diagnostics pass-through of engine text).

### F-C-10 — P2 · Axis: Page graph / two-phase scan · Verdict: **MISSING (promised by UI-MAP REPLACE-10, undocumented as delta)**

- **Ours:** one-shot `ScanMode` (Quick/Deep/DeepPlusCarve, ScanOptions.cs:42-52); `ScanStage`/`ScanStageStatus` enums declared but referenced **nowhere** (ScanProgress.cs:54-70; `rg ScanStage` → declaration only). No state, event, or copy models "quick pass done — advanced continuing in background".
- **Evidence (og):** UI-MAP §3.6.2:219-221 + §4 edges 4/9 + §13 REPLACE-10 (UI-MAP.md:904-905): "Two-phase quick→advanced scan with live tree → **keep exactly (core UX)**; the quick-scan upsell dialog becomes a non-blocking banner offering 'continue deeper scan'". UIUX/08 §4 (`CQuickScanCompleteWnd`, tr-verified: advanced scan continues while user may already recover); dynamic scan-phase capture (quick ≈21 s → advanced continues).
- **Impact:** our Quick mode is a terminal one-shot; the og's signature single-click "quick then deeper automatically" flow is absent, and nothing defers it (ADR-2 defers only the *live tree*, not the phase continuation).
- **Suggested fix:** implement quick→deep continuation as a chained run (scan quick → surface the non-blocking banner → on accept, re-scan with deep flags, replacing the session), using the dead `ScanStage` model; or record the reduction in UI-MAP §13 with rationale.

### F-C-11 — P3 · Axis: Navigation back-stack integrity · Verdict: **MISMATCH (minor)**

- **Ours:** `NavigationService.cs:90-97` — `TryGoBack` **pops** the history entry before checking state legality; on refusal the entry is lost (stack silently shrinks; a later-legal return can no longer reach it). Test `Back_RefusedWhenTargetIllegalInNewState` (NavigationServiceTests.cs:81-90) documents the refusal but not the lost entry.
- **Fix:** peek before pop; re-push on refusal.

### F-C-12 — P3 · Axis: Layering (gate ownership) · Verdict: **MISMATCH (minor)**

- **Ours:** `NavigationService.cs:106-114` — `ResetToHome()` forcibly writes `_stateGate = Select`, although the doc (:8-11) and `SetStateGate` (:39-44) establish `MainViewModel` as the gate's owner. `MainViewModel.NavigateForState`'s fallback (MainViewModel.cs:133-138) calls `ResetToHome()` while `WizardState` may still be e.g. `Scanning` → gate/`WizardState` desync (Home/Health become "legal" during a running scan until the next transition re-syncs).
- **Fix:** `ResetToHome` should not touch the gate; the fallback should re-assert `SetStateGate(WizardState)` (or reset the machine first).

### F-C-13 — P3 · Axis: ADR-4 wording vs code (ETA) · Verdict: **DELIBERATE-DELTA candidate, doc-code drift**

- ADR-4 (AUDIT-UI.md:83-88): "ETA is shown only where computable (carve, from `bytes_scanned` + throughput)". Code: linear ETA whenever `EngineFraction > 0.02` (ScanProgressViewModel.cs:80-85), `ScanSummary.BytesScanned` parsed but never used for ETA. The engine % is a truthful weighted figure (weights 40/35/25 per `cr_scan.cpp`, confirmed by sibling audit 2-b) and og showed "Remaining time" during the advanced pass (UI-MAP §5:546-548), so the *behavior* is defensible — the ADR text is stale. Fix: update ADR-4 (or gate ETA to the carve stage).

### F-C-14 — P3 · Axis: Parser hygiene · Verdict: **MISMATCH (minor)**

- `OutputParsers.cs:14-15` — `[GeneratedRegex] DiskLineRegex()` is dead code (never invoked; also carries the F-C-1 defect). Model group `(.*?)\)` breaks if a disk model contains `)`; serial `(\S+)` breaks on empty serial (engine prints bare `%s`, possibly empty, rrecov.cpp:237-238). Empty volume labels parse correctly (verified against the two-space engine output, rrecov.cpp:245-246). Fix: delete the dead regex; tolerate `)` in models and empty serials.

### F-C-15 — P3 · Axis: Repair diagnostics · Verdict: **MISMATCH (minor)**

- `SessionRecoveryServices.cs:404-409` — `ApplyAsync` searches kept lines for `"undo FAILED"` on exit 2, but the apply path prints `execute FAILED: rc=… (all writes rolled back)` (rrecov.cpp:1171); `"undo FAILED"` belongs to the `--undo-apply` path (rrecov.cpp:1084). The `TailText` fallback keeps severity right; the matched line can never exist in apply runs. Fix: match `execute FAILED` (and keep the undo variant in `UndoAsync`, which correctly throws on any non-zero exit, :416-420).

### F-C-16 — P3 · Axis: TreeCsvParser edge cases · Verdict: **MATCH-with-caveats**

- No embedded-quote handling (TreeCsvParser.cs:64-67) — matches engine reality (the engine does not escape either, rrecov.cpp:300), so this is faithful; a `"` in a name corrupts the row on the *engine* side first and our parser then rejects it (numeric-field check) rather than mis-lighting it. Empty directories (engine prints dir rows with attributes 0x10, rrecov.cpp:293/299-304, but CSV carries no dir flag) classify as files — acknowledged in the parser doc (TreeCsvParser.cs:13-15). Suggested: NEEDS-CORE-ISSUE to add an `attributes`/`is_dir` column to the frozen CSV grammar; note duplicate-path "last wins" (:30).

### F-C-17 — P3 · Axis: Filter surface wiring · Verdict: **PARTIAL**

- `ResultFilter` models all og dimensions (name/path/category/date/size/confidence/extension/deleted/raw — ResultFilter.cs:12-54, mapping cleanly onto og's 7 `AdvanceFilterCondition` groups, UIUX/31 §6), but `ResultsViewModel` exposes only category chips + name search (ResultsViewModel.cs:93-111); PathPrefix, size range, date range, confidence, extensions, IncludeRaw, DeletedOnly have no VM commands/properties. og filter bar: Show / File Type / Date Modified / File Size dropdowns + toggles (UI-MAP §6:601-609; UIUX/31 §1). Fix: add typed filter commands (or expose sub-VMs) before the App layer binds the bar.

### F-C-18 — P3 · Axis: Error surfaces / transport labels · Verdict: **PARTIAL**

- No Retry/Cancel surface for "drive removed mid-scan" (og: UI-MAP §11 row 1 / §4 edge 30, `[DYN]` "Drive Disconnected … Retry / Cancel"; ours: engine failure → `ScanFailed` event → message only, ScanProgressViewModel.cs:114-119). `DriverStatus.TransportLabel` collapses four states into "Direct-access mode" (HealthReport.cs:93-97) vs TRANSPORT-CONTRACT §9's per-state message table ("not installed — direct-access mode", "installed but not started", "administrator rights required", "incompatible (wire vX.Y)"). Fix: per-state labels (the `DriverState` enum already carries the information); add a retry affordance on scan failure.

### F-C-19 — P3 · Axis: Scope-reduction register (undocumented deltas) · Verdict: **UNDOCUMENTED-DELTA (batch)**

- Filter taxonomy: ours 7 categories / ~85 extensions (ResultNode.cs:130-171) vs og 8 FileFilter.xml categories (`Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks`, FileFilter.xml:2+, 1,489 entries — 1,481 unique, CORRECTIONS #5) + Databases classify tab (UIUX/09 §3) + 9-type search dropdown incl. "Unsaved File" (UIUX/09 §4). Neither the category reduction nor Bookmarks/Databases/Unsaved-File appears in UI-MAP §13.
- Home "Specify a location" trio (Recycle Bin / Desktop / Select Folder, UI-MAP §3.3:157-158) and the recycle-bin scan result state (`RecycleBinScanNoRltWnd`) are absent from our Home/ScanTarget model.
- BitLocker unlock chain (og P05) not in the omit list; our surface is `VolumeInfo.IsBitLocker` (parsed from list, OutputParsers.cs:64) with engine-side auto-unlock at scan time (CORE-CONTRACT §13 `--volume` behavior) and no unlock UI.
- Smart-partition scan (og P09) replaced by whole-disk `--deep` with `[lost]` result roots — documented in code (ScanOptions.cs:4-8) but not in UI-MAP §13.
- Settings lacks og's Display-zoom (DPI) option (UI-MAP §3.14:418).
- Fix: append these to UI-MAP §13 (omit or replace with rationale) so the map remains the authoritative reduction contract.

---

## 4. IN-FLIGHT inventory — what the App layer still owes (per UI-MAP / ADRs)

The WPF App project (`src/RdaWizard.App`) does not exist yet. Precisely these are missing (each mapped to its claim):

1. **Views + shell** — XAML per PageKey, DataTemplates PageKey→View, shell window, WizardStepper rail (ADR-12; WizardState.cs:32-41), custom title bar replacement (native chrome per UI-MAP §13 REPLACE-6/7).
2. **`IDialogService` implementation** — zero implementations exist in Core/Integration; 7 VMs take it as a constructor dependency (Home/ScanProgress/Results/Recovery/Completion/Health/Repair), so **DI cannot resolve the VM graph until the App registers one** (Core DI registers VMs unconditionally, DependencyInjection.cs:32-42).
3. **`IPreviewService` implementation** (ADR-8) — bounded workers, materialize-via-`recover --filter`, 15 s / 100 MB caps (settings exist: AppSettings.cs:27-31), per-type previewers (`PreviewKind` enum exists, Interfaces.cs:154-163). Nothing implements it; `ResultsViewModel.Preview` only sets `PreviewNode`.
4. **UI marshaler installation** — `ViewModelBase.InstallUiMarshaler` has no caller (ViewModelBase.cs:17).
5. **Shell event wiring (the whole machine-flow)** — VM events are raised but subscribed **nowhere**: `Home.SessionLoaded` (→ `ImportSession` edge), `ScanConfig.ScanStarted`/`CategoriesConfirmed` (→ `StartScan` + `Results.InstallSession`), `ScanProgress.ScanCompleted`/`ScanFailed` (→ `ScanCompleted`/`CancelScan`+home), `Recovery.RecoveryCompleted`/`RecoveryCancelled` (→ `RecoveryFinished`/`CancelRecovery`), plus continuous `WizardContext` maintenance (HasScanTarget/HasLoadedSession/HasRecoverySelection/DestinationOverlapCheckPassed/CancelConfirmed) and the ADR-3 cancel-confirmation dialogs. (`rg` for the event names shows only raise sites.)
6. **Session-path policy** — `ScanConfigViewModel.BuildScanOptions(sessionPath, stageLogPath)` requires a caller that generates `%LOCALAPPDATA%\RdaWizard\sessions\…` names (auto-save UX per UI-MAP §10) and records `SessionSourceRegistry.Record` on scan start.
7. **String tables** — `strings/en.json`, `strings/zh-CN.json` (see F-C-9).
8. **Data virtualization** for 1M+ nodes (ADR-9) — `VisibleFiles` is a flat materialized array (ResultsViewModel.cs:79-83).
9. **Static-event hooks** — `CompletionViewModel.OpenDestinationRequested`, `AboutViewModel.LicensesRequested` (App provides launcher/licenses text; note: static events are an app-boundary seam, acceptable but worth a comment).
10. **Health/Repair/About activation on navigation** — `Health.OpenAsync(disk)`, `Repair.OpenAsync(disk, part)`, `About.LoadAsync()` have no callers; `NavigationParameter` exists but nothing consumes it on the receiving side.
11. **Splash/enumeration gate** (UI-MAP §13 REPLACE-4) — Home `IsLoading` covers the state; the splash chrome is App work.
12. **Driver/engine status bar** — `DriverStatus.TransportLabel` + `EngineInfo` exist; no surface.

Not counted as in-flight (real findings instead): F-C-2 (stage-log tailer belongs in Integration, not App), F-C-3/F-C-4/F-C-6/F-C-7/F-C-8 (Recovery semantics in Core+Integration), F-C-1 (parser).

---

## 5. UNVERIFIED list

1. **"99 tests green locally"** — count verified statically (91 `[Fact]` + 8 `[InlineData]` = 99; Core 52 / Integration 47 exactly as claimed); **not executed** (no dotnet SDK in this sandbox).
2. **Engine DestIo same-name collision behavior** at the destination (matters for F-C-7) — not decoded in this audit; corpus is also silent (UI-MAP §8.7 `[CORPUS-AMBIGUOUS]`).
3. **UI-MAP traceability rows citing artifacts not read here** (13, 16, 17, 20, 22, 25, 30, 33, 34, 35 — e.g. retention-gate trigger paths, 722-pair connect graph, objectName closure). Spot-audit of 04/08/09/11/19/31 found those rows accurate; the rest are accepted on the map's own citation discipline.
4. **og quick/advanced mode gating flag (+0x60)** and its UI-availability effects (SearchDLL artifacts) — outside this audit's axis set.
5. **`rrecov` `--volume C:` BitLocker auto-unlock failure path** (what the UI would show for a locked volume when the engine cannot unlock) — engine code not traced into `cr_bitlocker`; relevant to the omitted P05 unlock chain.
6. **Whether `-` ever occurs in real `list` output** for `table_ok` with non-GPT/MBR layout (theoretical from rrecov.cpp:239-242; fixture should still cover it).
7. **DESIGN-SYSTEM token fidelity** — visual layer, App in-flight; not audited (no XAML exists to audit).

---

## 6. Deliberate-delta register (documented, ratified)

| Delta | Where documented | Verdict |
|---|---|---|
| WizardStepper 3-phase rail (og has none) | AUDIT-UI ADR-12; WizardState.cs:32-41 | ratified, implemented (phases + `Phase()` mapping, tested WizardStateMachineTests.cs:186-194) |
| Destination-overlap hard gate up-front | AUDIT-UI ADR-7 | implemented twice (machine guard WizardStateMachine.cs:54-60 + preflight SessionRecoveryServices.cs:113-121) — but see F-C-5 for the silent-degrade hole |
| Destructive cancel, no pause | ADR-3/G-07 | implemented (confirm-guarded edges; kill-tree; engine writes sessions only on success, rrecov.cpp:521-534) |
| Results populate at completion (no live tree / mid-scan recovery) | ADR-2/G-02/G-03 | implemented (Results installed only via `ScanCompleted`/import) |
| Truthful-coarse progress | ADR-4 | implemented for %/elapsed; read-errors NOT (F-C-2); ETA drift (F-C-13) |
| Client-side filters over snapshot | ADR-9/G-12 | implemented (`FilterEvaluator`), surface partial (F-C-17), virtualization pending |
| CRSF-only sessions, no .rsf import | ADR-10/G-11 | implemented (`SessionService`, `PickOpenFileAsync("*.crsf")`) |
| Scan-config as an explicit step | UI-MAP §5 + ScanConfigViewModel.cs:11-14 | implemented |
| Repair = RawFixer (not og Fixo file repair) | UI-MAP §13 omit; AUDIT-UI §4 | implemented (dry-run default + typed-ACK + confirm + undo — exceeds og gravity) |
| No monetization/telemetry/retention/AI/cloud/NAS/RAID/WinPE | UI-MAP §13.1-§13.9; AUDIT-UI §4 | confirmed absent from code ✓ |
| EN-first localization (2 cultures vs og 25) | UI-MAP §13.11 | infrastructure ✓, adoption missing (F-C-9) |
| Driver probe without IOCTLs (no GET_INFO/GET_VERSION in UI) | DriverStatusDetector.cs:11-17 | implemented; WrongVersion state effectively unreachable — acceptable, documented; label gap F-C-18 |

---

## 7. Next actions (ordered)

1. **Fix F-C-1 immediately** (parser regex + fixtures + CORE-CONTRACT §13 correction + engine-diff contract test) — it gates every dynamic flow behind the first screen.
2. Implement the stage-log tailer (F-C-2) in Integration; wire read-error ranges into the progress snapshot.
3. Decide and implement selection-vs-all recovery (F-C-3) — including an honest interim block if per-node recovery waits on core.
4. Rework preflight space gate to needed-bytes + continue-anyway (F-C-4); surface overlap-check degradation (F-C-5).
5. Recovery progress parsing + Percent mapping (F-C-6); implement or remove collision/folder-structure controls (F-C-7); source-disk re-selection for imported sessions (F-C-8).
6. Extract VM strings to key tables; ship `strings/en.json` + `zh-CN.json` (F-C-9) before the App layer multiplies literals.
7. Resolve the two-phase quick→advanced flow (F-C-10) — implement or document the reduction.
8. Fold the §5 scope-reduction register into UI-MAP §13 (F-C-19) and update ADR-4's ETA wording (F-C-13).
9. P3 hygiene pass: F-C-11/12/14/15/16/17/18.
10. App-layer build-out per the §4 inventory (other agents, in flight) — with item 2 (IDialogService) first, since DI cannot resolve the VM graph without it.

---

*End of PARITY-UI.md — Task 2-c deliverable. Corrections should cite the findings IDs above.*
