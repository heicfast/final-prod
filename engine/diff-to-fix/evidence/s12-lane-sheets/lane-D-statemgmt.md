# LANE D — STATE MANAGEMENT (read-only audit): scan lifecycle, cancel/pause/resume, checkpointing, session lifecycle, error-recovery transitions

Auditor: LD deep-dive lane. Core tree = `8d0d88b8` (`/home/z/audit/core_recovery`); corpus =
`be0cfe9` (`/home/z/audit/easeus-drw-research`). Every cite below was read first-hand this
session. `DRWUI:<line>` = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`; corpus-doc
cites name the artifact; core cites are file:line in the core tree. One scratch file written
(this one); no repo files modified.

Prior-board hygiene read before filing: `docs/PARITY-DELTAS.md` (D-78 progress delivery,
D-105 auto-save enable shape, D-106 Folder suffix, section W = S-052 riders), active findings
N-090/N-125/N-131 (read in full), `diff-to-fix/modules/session.md` + `modules/orchestration.md`,
S-044's evidence `evidence/decompiled-quotes/s8-autosave-and-stamps.md`. No candidate below
duplicates a registered delta or an active finding; overlaps are cross-referenced.

Headline decodes this lane contributes (NEW, first-hand):

1. **The og's master scan-lifecycle state enum** — 14 values with their own telemetry names
   (`notstarted … repairvideo_pause`), on the DRWUI service singleton at `+0x78`
   (DRWUI:208243-208296). Never enumerated before; the board's O31–S35 "M" verdicts describe
   stage bits, not this enum.
2. **The og's pause machinery** — engine-side `Pause(1)/(0)` vtable call + a run/pause event
   gate + GetTickCount pause compensation + a `quickscanpaused/advancedscanpaused` state pair,
   plus pixel-verified Pause (❚❚) / Stop (■) controls (DRWUI:314388-314490, UIUX/35:175-182).
   The core's PausableToken is a documented "RE parity" of exactly this — the parity holds and
   is here pinned with og cites.
3. **The og's stopped-scan auto-save** — the stop path runs its own organize phase
   ("Scan stopped and found %1 files.", DRWUI:1549395-1549415) and lands in the SAME
   completion handler whose auto-save gates have NO stopped/cancelled check (DRWUI:612479-612587,
   gates at :612572-612587). The core's "a cancelled run never auto-saves" contract diverges.
4. **A core-internal ACUTE found in passing** — the CLI's `--resume`-without-`--out` flow
   opens the resume target with `"wb"` (the checkpoint sink) BEFORE the resume read, truncating
   the session to zero bytes in place (rrecov.cpp:1576 vs :1598). Mechanism proven by execution
   (see LD-1).

---

## 1. CONTEXT SHEET

### 1.1 The og's state machine — where it lives, the states, the transitions

The og's scan lifecycle state is DISTRIBUTED over four objects (all first-hand):

| Object | Field | Locking | Role |
|---|---|---|---|
| DRWUI service singleton (`FUN_1400ec7b0()`, TLS-guarded global) | `+0x78` int — the master lifecycle enum | **unlocked** (plain writes from scan + UI threads) | the 14-value scan state; telemetry source (`FUN_1400f7290` maps it to names) |
| `CUIScanProgressInfo` (progress object, `param_1+0x48` of the strategy) | `+0x44` nStatus; `+0x6c` pause substate (2 = paused) | mutex at `+0x90` (`FUN_1401acf10` :313726-313757 read / `FUN_1401acd30` write under `FUN_140335e80/EC0`) | the locked status the bottom-area consumes |
| `CBottomArea` (status-bar widget) | `+0xe0` m_iScanType (3 = quick pass done → advanced, 4 = advanced) | Qt UI thread | the phase tracker; emits `sigScanFinished(iScanTypeChanged, iScanType)` at each phase boundary (UIUX/34:50-59) |
| `CDoScanDeviceBase` strategy (`CDoScanPartition` / `CDoScanLostPartition`, ctors DRWUI:314296/:314305) | `+0x149` paused, `+0x158` stopped, `+0x15a` finished, `+0x1c8` import/finalize guard, `+0x388` already-saved, `+0xe8` stop-once, `+0xf8` pause-gate event, `+0x108` abort event, `+0xe4` pause-start tick, `+0x1c` compensated elapsed | kernel events thread-safe; flags plain | the worker-side run state |

The master enum (`FUN_1400f7290`, DRWUI:208243-208296 — names verbatim from the telemetry
mapper): `0 notstarted · 1 quickscanning · 2 quickscanpaused · 3 quickscanstopped ·
4 quickscanfinish · 5 advancedscanning · 6 advancedscanpaused · 7 advancedscanstopped ·
8 advancedscanfinish · 9 importedscan · 10 repairvideo_scanning · 11 repairvideo_stop ·
12 repairvideo_pause · 13 repairvideo_complete`.

Transitions (trigger → action → persistence effect; all DRWUI cites):

| From → To | Trigger | Action (cite) | Persistence effect |
|---|---|---|---|
| 0 → 1/5 | scan thread start | `FUN_1400f1a90(svc, isQuick)` sets state + start ticks `+0x8c/+0x98/+0x90` (DRWUI:205771-205791), called from `CScanMgr::doScan` after the strategy's start vcall `+0x38(advanced, resume?)` (:314330-314336) and before the thread spawn `FUN_1401b02a0` (:314352) | none yet |
| 1/5 → 2/6 (→12) | **Pause button** (`pause_button` objectName :1097699; pixel-verified UIUX/35:175) | strategy pause `FUN_1401ae530` (:314388-314415): paused flag `+0x149=1`, `engine->vtable+0x88(1)`, pause tick `+0xe4=GetTickCount()`, **ResetEvent(pause gate `+0xf8`)**; state setter `FUN_1400f8f40` → 2/6, or 12 if video-repair mode (module `+0x100`, :208944-208996) | **none** (no save on pause) |
| 2/6 → 1/5 | **Resume** | `FUN_1401ae720` (:314452-314490): paused=0, paused-duration added to compensated elapsed `+0x1c`, **SetEvent(gate)**, `engine->vtable+0x88(0)`, state back via `FUN_1400f9200` → 1/5 (:209041-209049) | none |
| 1 → 3 / 5 → 7 | **Stop-confirm "Stop"** (`CScanCommonPopDlg`: "Stop searching for lost data?" → Stop/Continue) | `FUN_1400fcb90` (:210774-210810): param_3 selects 3 (quick) vs 7 (advanced); telemetry Click_Close/Click_Continue/Stopscanning; the machinery `FUN_1401ae610` (:314416-450, once-guard `+0x1c8`) → `FUN_1401b0c40` (:315617-315646): stopped `+0x158=1`, config stop `+0x68=1/+0x6c=reason`, once-guard `+0xe8`, **SetEvent(abort `+0x108`)**, `CStopScanThread` joins (`FUN_1406a6ff0` :1096667-1096675 → engine mgr vtable+0x28(0)) | **partial results kept + auto-saved** (see below) |
| 5 → 7 vs 5 → 8 | engine run ends | `FUN_1400f1b10` (:205793-205840): if state still 5 AND progress `+0x1b0` < 0x62 (98 %) → **7 stopped**; else → **8 finish**; quick: → 4 unless already 3; elapsed stamps `+0x9c/+0x94`. `CScanMgr::ScanFinished` (:314678-314800, trace :314741) re-derives 7/8 from the stopped flag `+0x158`, waits for the stop flag module`+0x99` (`Sleep(500)` loop :314759-762), stamps nStatus via `FUN_1401acd30`, telemetry "Completed"/"Stoppted" [vendor typo] (:314778-787) | auto-save fires (below) |
| → 4 / 8 | phase completion (quick pass done; advanced done) | bottom-area `nStatus=4, m_iScanType=3` then `=4` (UIUX/34:55-59; runtime UIUX/30:82-84); `sigScanFinished(iScanTypeChanged, iScanType)` | **auto-save of the whole tree** (below) |
| → 9 | **.rsf import/load** ("Continue last scanning … Load" banner, UIUX/07:21; Click_Loadsession :205873) | load classification restores the lifecycle state from the saved result code → 6/7/8/9/10 (:837740-837800); import-mode `param+0x20==3` marks the run pre-finished (:314207-210) | none on load; import flag `+0x1c8` suppresses re-save |
| → 10/11/12/13 | repair-during-scan sub-machine | `FUN_14011be80(svc, 10)` etc. (:919247/:919275; :224511-224513) | S-050's scope |

**User actions on the scan** (og): Pause (❚❚), Resume (same button), Stop (■) with a
Stop/Continue confirm. There is **no cancel-and-discard option** — the stop ALWAYS keeps the
partial results, organizes them ("Scan stopped and found %1 files." — the stop-organize modal
case 3, DRWUI:1549395-1549415, QSS state `FreezeScanStopArbitrate`; the complete twin case 2
"Scan completed and found %1 files." :1549360-1549390), and then flows into the completion
handler `FUN_140395e50` (:612479) from the "Build tree finish." callback (:339005-339080) —
whose auto-save block (:612572-612587) gates ONLY on not-import (`+0x1c8`), results-found
(`FUN_1403964b0` :612648-612696), and the SD/NAS device check (module `+0x69` + strategy
vtable+0x78, :612575-612581). **No stopped/cancelled check exists** — the S-044 evidence file
itself records "No `IsCancelled` check exists at any trigger site … the corpus does not
evidence a 'cancelled never auto-saves' gate" (`evidence/decompiled-quotes/s8-autosave-and-stamps.md:18-21`).
The stop-confirm copy confirms the intent: "Remaining data can be rescanned and recovered
anytime later" (UIUX/35:182).

### 1.2 The og's auto-save / checkpoint semantics

- **Trigger sites** — three, all in completion-family handlers: `FUN_1402857c0` called at
  :612583 (completion main path, inside `FUN_140395e50`), :615166 (early-finalize
  `FUN_14039b470`), :616764 (`FUN_14039ec00`, results-found + not-already-saved `+0x388`).
- **Gates**: config byte flag (`FUN_1400b5270`, 0 = enabled, :442822-824 — registered D-105),
  not-import, results-found, not-already-saved (2 of 3 sites), SD/NAS device gate.
- **What runs**: `FUN_1402857c0` (:442747-442944): SaveScan dir from the module path
  (:442838-850), volume name (+ `Folder` suffix for folder scans — D-106), `GetLocalTime`
  (:442887), `sprintf "%02d_%02d_%02d_%02d_%02d"` = `M_D_h_m_s` (:442893), append `\auto_` +
  ts + `.rsf` (:442898-900), **`DeleteFileW` of an existing same-name file** (:442903-907),
  then the CRSDataEx save (`"SaveResultPath = %S"` trace :440979; per-node progress 40→65 %
  + per-child `IsCancelled` teardown, rsf-format §5.4.4 @ :186575/:186644).
- **Cadence**: the og writes auto-saves at **phase-completion windows only** — the dynamic
  trace shows the quick pass finishing → Freeze → auto-save → Unfreeze → the advanced pass
  continuing (UIUX/30:60-105: auto-save at 16:25:59 with `m_iScanType=3`, advanced-complete
  line at 16:26:03). There is **no periodic mid-stage checkpoint** and **no engine-state
  persistence**: the .rsf records are session flags (record 1), volume identity + times
  (record 2), partition/device context incl. the 512-byte boot snapshot (record 3), and the
  full result tree (record 4) — no MFT position, no FAT chain cache, no carve position, no
  stage progress (rsf-format §2.2/§5.1-5.4). The 40→65 % / `40+50·done/(total+done)` formulas
  are save/load PROGRESS fractions, not scan-position state.
- **The one mid-run persistence the og has**: lost-partition finds are appended
  unconditionally to a module-local side-channel as they are discovered (PRD:20409 — "a
  killed scan still leaves its finds on disk"); the core's equivalent is the sink-gated
  per-hit checkpoint (cr_scan.cpp:1911-1919, registered with M-042's resolution).
- **Resume semantics**: "resume" = **import the result tree** (state 9/6/7/8/10 restored from
  the session) for browse/recover; the only continuation is the quick→advanced phase advance
  (m_iScanType 3→4), which runs the FULL deep searcher set + RAW fresh — there is no
  scan-position continue and no stage tracking in the format.

### 1.3 The core's state model (side by side)

| Surface | Og | Core | Verdict |
|---|---|---|---|
| Master lifecycle states | 14-value enum, unlocked, on the service singleton (DRWUI:208243-296) | No lifecycle enum; the truth is `stage_done_mask` (STAGE_QUICK/DEEP/CARVE, cr_scan.h:517-522) + the live telemetry snapshot `running/paused/cancelled` (cr_scan.h:490-513) | **DIVERGENT-model, REGISTERED-adjacent** — the mask is the G-07 honest replacement for the og's stopped-vs-finished inference (LD-6) |
| Stage model | quick pass → advanced pass (m_iScanType 3→4, two user-visible phases, quick browsable while advanced runs) | one run(): quick → deep → carve, stage-boundary events `ScanStage` (cr_scan.cpp:1168-1205; cr_ui.h `CR_EVT_SCAN_STAGE`) | **MATCH** (O37; a host can render the two-phase UX off the stage events) |
| Pause | engine `Pause(1)/(0)` + event gate + tick compensation (DRWUI:314388-314490) | `PausableToken` pause/resume/cancel, `checkpoint()` blocks engine workers, `paused_ms()` compensation — the header cites "RE parity: the og subtracts pause duration from used-time" (cr_device.h:148-259; cr_scan.cpp:1128-1130) | **MATCH** (the compensation formula is pinned by `test_pause_resume.cpp`) |
| Pause observability | state 2/6 + nStatus==2 (locked getter :313726-757) + "Scanning Paused." strip + `sigTaskBarPaused(bool)` (:1323035) | telemetry `paused` flag (cr_ui.h:658) + busy/stopping queries (N-075); no taskbar surface (host concern) | **MATCH-with-note** (LD-7) |
| Pause surface (CLI) | visible Pause button (UIUX/35:175) | `cr_ui_scan_pause/resume` ABI exists (cr_ui.cpp:2790-2803); **the CLI has no pause verb** (rrecov.cpp:3149-3160 usage) | **LD-4 (MINOR)** |
| Stop/cancel semantics | stop-confirm → stopped flag + abort event → engines abort cooperatively (`FUN_1401aee10` blocks on the abort event :314616-624) → organize → keep results | `cancel()` on the token; ERR_CANCELLED propagates from every path; cancelled stages never stamped done (cr_scan.cpp:2014-2016); results stay in the tree; save-on-cancel with an explicit `--out` (rrecov.cpp:1760-1775) | **MATCH** (keep-results both sides) |
| Cancel pool-latency | abort event wakes everything | `CancelPoolWatcher` 50 ms watcher (cr_scan.cpp:1960-1968, N-017 fixed) | **MATCH** (post-fix) |
| Auto-save on stop | **fires** (stop → organize case 3 → completion handler → gates, no stopped check) | **never** — `if (rc != OK) return` precedes the auto-save block (cr_ui.cpp:2147-2148 vs :2166); CLI comment "never a cancelled run" (rrecov.cpp:1392) | **LD-2 (SEVERE)** |
| Auto-save gates | not-import / results-found / not-already-saved / SD-NAS / config byte (D-105) | not-resumed / results-found-or-lost / not-user-saved (cr_ui.cpp:2166-2168) | **MATCH** (S-044 FIXED; D-105/D-106 registered) |
| Auto-save naming | `auto_<vol>_<M>_<D>_<h>_<m>_<s>.rsf`, sanitizer `/ * : ? " < > |`, no year | `auto_session_name` identical incl. the no-year quirk (cr_crsf.cpp:363-403) | **MATCH** |
| Auto-save timing | also at the QUICK-phase completion window (UIUX/30:66-67) mid-run (fresh file) | completion-only auto-save; the stage-boundary checkpoint covers mid-run ONLY when `--out`/checkpoint_path is wired (rrecov.cpp:1574-1583) | folded into **LD-2** |
| Mid-scan checkpointing | none (phase-completion auto-saves only; per-hit lostpart side-channel) | stage-boundary checkpoints + per-discovery-hit checkpoint, sink-gated (cr_scan.cpp:1178/1186/1199, :1911-1919) | **SUPERSET** (G-13; the no-sink case is the honest architecture difference, registered with M-042) |
| Checkpoint atomicity | fresh timestamped file per save (a torn new file leaves the previous intact) | cr_ui: `<path>.tmp` + commit-rename (cr_ui.cpp:2078-2092, N-014 fixed, test `CrUi.CheckpointAtomicStagingAndCommit`); **CLI: `"wb"` truncate + in-place overwrite, no commit hook** (rrecov.cpp:1574-1583) | **LD-3 (MAJOR)** |
| Resume semantics | import-tree + optional fresh advanced pass (no stage tracking) | `resume()` loads tree + mask, skips completed stages, re-runs incomplete stages FROM STAGE START, adopts fs_hint/identity/label (cr_scan.cpp:2578-2609, :266-300) | **SUPERSET** (G-07; the og has no equivalent) |
| Engine re-arm on load | none persisted (no MFT position / FAT cache / carve position in .rsf — rsf-format §2.2) | none persisted either (stage mask only) — LDM basis re-arm LANDED (LDM_LAYOUTS record cr_records.cpp:598-632 + root basis ids :292-293/:507-513 + `session_arm_ldm_basis` cr_ui.cpp:1961-2023 + resume adoption cr_scan.cpp:2601-2606) | **MATCH (no-engine-state both sides)**; S-052's LDM re-arm verified landed |
| Load-time identity re-check | record-3 identity card (model/serial/boot snapshot, rsf-format §5.3) | boot-hash verify + warnings (rrecov.cpp:1622-1646), layout-changed signal, stale-candidate drop (M-042/FIXED) | **MATCH/superset** |
| Save/load progress + cancel | per-node 40→65 % + per-child `IsCancelled` (rsf-format §5.4.4) | N-060 save + W1b load hooks (cr_records.h:33-59) | **MATCH** |
| Session list | `auto_*`-only enumeration, record-1-then-2 light read (DRWUI:432326-333) | every `*.crsf` (N-090 scope), light read (W1a), time-ordered (N-116 FIXED) | **COVERED-BY-N-090** |
| Progress delivery | poll-dominant timer farm + query signals | push events + telemetry pull | **COVERED-BY-D-78 / N-075 (FIXED)** |
| **Disconnect mid-scan** | modal "Drive Disconnected" Retry/Cancel (FUN_1406c3d20 :1110914-1111230; telemetry Click_Retry/Click_Cancel "Disconnect" :218373-218404); **no dedicated scan-state value — the state stays 1/5 while the modal pumps the UI thread** | ERR_READ_FAILED ladder; read-truth refusal when nothing could be read (rrecov.cpp:1776-1790); continuation only at session-resume granularity | **COVERED-BY-N-125** + the state-machine observation (neither side has a disconnect scan-state; the og's retry is a UI-modal, not a transition) |
| Bad-sector storms | engine-internal error maps (N-045/D-72 adjacent) | retry+bisection + zero-fill salvage + live error map (cr_scan.cpp build_read_pipe; FB-R02) | **MATCH** (documented superset) |
| Engine crash/hang | in-process 5 s progress-stall watchdog bumps a coarse counter (FUN_140577100 :910565-910585 — no death detection, no state change); actual crash = process death → DRWLoader watchdog → DRWReport → restart | in-process; N-043 heartbeat `CR_EVT_HEARTBEAT` (the honest variant — standing truth, no synthetic advance; cr_ui.h:164-177 cites the og's :910565-910585) | **MATCH-with-improvement** (neither detects engine-thread death in-process; the og's watchdog is the cited lineage) |
| Driver absent | file-API fallback / driver surfaces (BootDriver artifacts) | `rrecov driver` report + read-truth refusal | **INFO** (different architecture, both surface it) |
| Who owns state / locking | 4 objects, mixed discipline: the master enum is UNLOCKED (cross-thread plain writes), nStatus mutexed, events kernel-safe, m_nFreeze LOCK/UNLOCK-guarded (:612604-612612) | ONE owner (ScanController + caller's token); telemetry under `tel_.m`; token under mutex+cv; documented lock order + "events may fire from worker threads, must not block" (cr_ui.h:143-146) | **MATCH/superset** (the og's unlocked master enum is an og-side hazard the core avoids — LD-9 INFO) |
| Callback threading | engine workers → Qt queued signal/slot to UI (the moc machinery; UIUX thread doc: "every non-trivial operation off-thread") | engine worker threads → typed event fn (documented; single-flight is the host's discipline); progress via the bridge (cr_scan.cpp:520-554) | **MATCH** (D-78 registered for push-vs-poll) |

### 1.4 The state diagram (og, text form)

```
                         user: Scan (quick)                user: Scan (advanced)
              ┌────────── FUN_1400f1a90 ─────┐   ┌──────── FUN_1400f1a90 ─────┐
              ▼                              │   ▼                            │
 [0 notstarted]                       [1 quickscanning]                 [5 advancedscanning]
              │                              │   ▲        ▲                   │        │
              │  .rsf import                 │   │resume │pause              │pause   │engine
              └──────────► [9 importedscan]  ▼   │        ▼                  ▼        │ ends
                          (state restored   [2 quickscanpaused]        [6 advancedscanpaused]
                           6/7/8/9/10 per   │   │                            │      │
                           :837740-800)     └───┘                            │      │
                                                                             │      │
   user: Stop-confirm ──► [3 quickscanstopped] ◄──── FUN_1400fcb90 ──────────┘      │
   (FUN_1400fcb90)         [7 advancedscanstopped] ◄─ stop OR FUN_1400f1b10           │
                                                        (state==5 && progress<98%)    │
   engine ends / phase completes:                                                      ▼
   FUN_1400f1b10 ──► [4 quickscanfinish]                                 [8 advancedscanfinish]
   bottom-area: nStatus=4, m_iScanType 3 (quick) → 4 (advanced); sigScanFinished ×2

   persistence (phase-completion AND stop windows — same completion handler):
     [4]/[8]/[3]/[7] ──► organize ("Scan completed/stopped and found %1 files.")
                    ──► "Build tree finish." ──► FUN_140395e50 ──► auto_<vol>_<M_D_h_m_s>.rsf
                          (gates: not-import · results-found · not-already-saved · SD/NAS)
     pause/resume: no persistence · elapsed compensated (+0x1c += now−pause_tick)
     import: no re-save (+0x1c8 import flag)

   video-repair sub-machine (S-050's scope): [10 scanning] [11 stop] [12 pause] [13 complete]
```

---

## 2. DIVERGENCE CANDIDATES

### LD-1 — ACUTE (core-internal, found in passing): the CLI's `--resume`-without-`--out` flow truncates the session file BEFORE reading it — the documented resume command destroys the user's only session copy

- **Severity:** ACUTE (loss — the session being resumed is zeroed before the read; the
  save-on-cancel flow prints exactly this command as THE recovery path).
- **Og (cite):** n/a (core-internal defect; the og's import path opens the .rsf read-only —
  the CRSDataEx reader chain, rsf-format §4 `FUN_14051b0b0` — and never truncates the source).
- **Core (cite):** `src/app/rrecov.cpp:1567` `effective_out = out_path.empty() ? resume_path : out_path`;
  `:1576` `_wfopen_s(&ck_file, effective_out.c_str(), L"wb")` ← **truncates the resume target**;
  `:1598` `_wfopen_s(&fin, resume_path.c_str(), L"rb")` ← reads the now-zero-length file. The
  checkpoint sink wiring block (1569-1583) precedes the resume-read block (1596-1615). The
  save-on-cancel message prints the destructive command: "resume with: rrecov scan --resume %s
  --disk N | --volume C:" (`:1767-1769`) — no `--out`.
- **Scenario:** `rrecov scan --volume C: --out F.crsf` → Ctrl-C after an hour → "session saved
  (cancelled): F.crsf" → the user runs the printed `rrecov scan --resume F.crsf --volume C:`
  → F.crsf is truncated to 0 bytes at :1576 → the resume read at :1598 sees an empty container
  → "resume failed" → **the interrupted scan's results are permanently lost**.
- **Mechanism proven by execution this session** (the exact fopen sequence compiled and run:
  the "rb" read sees 0 bytes; a 1000-byte file, opened "wb" then "rb", reads as 0 — trivial
  C semantics, but the source order makes it real). No CLI e2e resume test exists
  (`tests/` has unit + ui_abi_check only; the Windows CI junk-gate step runs scan/tree/recover,
  not resume-in-place).
- **Fix shape:** open the checkpoint sink on `<effective_out>.tmp` + commit-rename (the
  cr_ui `CkSink` shape at cr_ui.cpp:2078-2092), or defer the ck open until after the resume
  read; also fixes LD-3's CLI half.

### LD-2 — SEVERE: stopped scans auto-save partial results in the og; the core never auto-saves a cancelled run — a user who stops a long scan keeps their results in the og and loses them in the core

- **Og (cite):** the stop path keeps and organizes partial results — the stop-organize modal
  "Organizing Found Files / **Scan stopped and found %1 files.**" + QSS `FreezeScanStopArbitrate`
  (DRWUI:1549395-1549415, case 3; the completed twin case 2 at :1549360-1549390); the organize
  finishes → "Build tree finish." (:339005) → the completion handler `FUN_140395e50` (:612479)
  whose auto-save block (:612572-587, call `FUN_1402857c0` at :612583) gates ONLY on
  not-import/results-found/SD-NAS — **no stopped check**; the early-finalize twin
  `FUN_14039b470` (:615148-615166) auto-saves too. The user-facing contract: "Remaining data
  can be rescanned and recovered anytime later" (UIUX/35:182). The `CStopScanThread` stop
  passes reason 0 (`FUN_1406a6ff0` :1096667-75 → engine-mgr vtable+0x28(0)), which leaves the
  config `+0x6c` gate (:612528-530) at 0 — the completion handler RUNS for user stops.
  Second leg: the og also auto-saves at the QUICK-phase completion window mid-run (the
  dynamic trace: auto-save at 16:25:59 while the advanced pass continues, UIUX/30:66-84).
- **Core (cite):** `cr_ui.cpp:2147-2148` `if (rc != OK) return ret_(rc, …)` precedes the
  completion auto-save block at :2166 — a cancelled run (rc == ERR_CANCELLED) never reaches it;
  the comment at :2154-2156 asserts "a cancelled run never does [auto-save] - the og's flow is
  the completion window"; `rrecov.cpp:1831-1838` — "below on run() completion only - never a
  cancelled run". S-044's own evidence concedes the corpus shows no cancelled-never gate
  (`evidence/decompiled-quotes/s8-autosave-and-stamps.md:18-21`).
- **Scenario:** a 2-hour scan with 500 K files found; the user stops it. Og: an
  `auto_<vol>_<ts>.rsf` appears in SaveScan and in the "Continue last scanning" banner — the
  results are recoverable anytime. Core (CLI `--autosave-dir D`, no `--out`): nothing is
  written — no checkpoint sink was wired, the auto-save is completion-only, and the process
  exit drops the tree. The user re-scans from zero.
- **Note:** the core's save-on-cancel (`--out`) and stage checkpoints are the ARGUABLY better
  resumable machinery — but the auto-save DIVERGENCE is real and user-observable, and the
  comment's og claim ("the og's flow is the completion window") is contradicted by the
  stop-organize decode. Either land stop-window auto-save (gated on results-found, like the
  og) or re-register the completion-only posture as a deliberate delta with this evidence.

### LD-3 — MAJOR: the CLI's checkpoint sink is the truncate-in-place shape N-014 fixed in cr_ui — a crash mid-emit destroys the previous good checkpoint (and a pre-existing session at the --out path is truncated at scan start)

- **Og (cite):** fresh timestamped file per save (`\auto_` + ts, DRWUI:442898-900) — a torn
  new file never destroys the previous auto-save (rsf-format §5.5; the registered crash-safety
  model N-014 cites as "the og's fresh-file-per-save crash-safety").
- **Core (cite):** `rrecov.cpp:1574-1583` — `_wfopen_s(…, L"wb")` on the FINAL path (truncates
  whatever session was there, including under `--out` over an existing file), `seek(0)` +
  overwrite per emit (`emit_checkpoint_` cr_scan.cpp:585-600), **no `commit` hook** — the
  cr_ui twin stages `.tmp` + rename (cr_ui.cpp:2078-2092, N-014 FIXED, pinned by
  `test_cr_ui.cpp:2650-2656` whose own comment describes this exact pre-fix shape: "the file
  was opened 'wb' on the FINAL path and patched in place - torn on crash"). The CLI's final
  save and save-on-cancel DO use tmp+rename (rrecov.cpp:1731-1757) — only the in-run
  checkpoint kept the torn shape.
- **Scenario:** `rrecov scan --disk 2 --out S.crsf` runs 3 hours; quick+deep checkpoints
  landed; the process is killed during the carve-boundary emit → S.crsf is a torn container
  with the previous good checkpoint overwritten → resume from S.crsf fails or salvages less
  than the last committed stage. With `--out` pointing at a prior session (overwriting
  intent), the truncation at :1576 destroys it even before the first checkpoint.
- **Note:** LD-1 is the acute special case of this same wiring (the resume target as the
  truncate victim). The fix is one shape: the CLI adopts the CkSink tmp+commit pattern.

### LD-4 — MINOR: no pause verb in the CLI (the og ships a visible Pause control; the core's ABI has pause, the CLI never exposes it)

- **Og (cite):** the Pause (❚❚) + Stop (■) bottom-bar controls, pixel-verified
  (UIUX/35:175-176; UIUX/dynamic-ground-truth--scan-phase-captured.md:99); the paused UI state
  ("Scan Paused" + red "Scanning Paused." strip, gauge frozen — UIUX/35:182; DRWUI:1098406-24,
  :1100349-65); the pause machinery DRWUI:314388-314490.
- **Core (cite):** `cr_ui_scan_pause/resume` exist and are tested (cr_ui.cpp:2790-2803;
  test_pause_resume.cpp); the CLI's only control is Ctrl-C = cancel — the usage block has no
  pause flag (rrecov.cpp:3149-3160; the header comment :142-145 mentions "a UI host
  additionally uses pause()").
- **Scenario:** a CLI user needs to suspend a long scan (laptop sleep, disk contention) —
  the only option is cancel (and with no `--out`, LD-2's loss). The og user pauses and
  resumes with zero loss and compensated elapsed.
- **Fix shape:** a `--pause-input`/stdin command or SIGUSR mapping onto the PausableToken;
  or document the ABI-only posture.

### LD-5 — MINOR: the og notifies the ENGINE of pause (vtable+0x88); the core's pause is a pure token wait — no engine-visible pause event exists to mirror

- **Og (cite):** DRWUI:314401-314404 (`engine->vtable+0x88(1)` on pause) and :314482-484
  (`(0)` on resume) — an explicit engine-object call beside the `+0xf8` gate.
- **Core (cite):** pause is only the `checkpoint()` condition-variable wait
  (cr_device.h:229-234); engines observe nothing else.
- **Scenario:** if the og's engines do pause-specific work (flush caches, checkpoint internal
  state), the core cannot. No decoded og engine-side pause behavior exists in the corpus
  (no pause-specific engine code found in the searchdll decompiles this session) — so this is
  a register-or-note, not a build.

### LD-6 — INFO (modeling difference; core improvement): stopped-vs-finished classification — the og infers it from progress<98 % and persists the verdict into the session; the core derives it from the stage mask

- **Og (cite):** `FUN_1400f1b10` :205797-205813 — advanced end while state==5 AND progress
  (`+0x1b0`) < 0x62 (98) → 7 stopped, else 8 finish; `ScanFinished` re-derives 7/8 from the
  stopped flag (:314692-314712); the verdict is restored on session load (:837740-800).
- **Core (cite):** stage bits stamped only on success; "a cancelled discovery must not stamp
  the stage done (G-07 integrity)" (cr_scan.cpp:2014-2016); resume skips only truthfully
  completed stages (cr_scan.cpp:2583-2589).
- **Verdict:** the core's model is the honest replacement — no action; recorded so the
  mapping is explicit (og states 3/7 ↔ core cancelled-stage; og 4/8 ↔ core stage-done bit).

### LD-7 — INFO: paused-state observability — og surfaces pause in the master enum, nStatus, the UI strip, and the taskbar; core surfaces it in telemetry only (no `cr_ui_scan_paused` query, no taskbar concept)

- **Og (cite):** state 2/6 (DRWUI:208944-208996); nStatus==2 branch (DRWUI:1098851-1098859);
  the `paused` widget property ("Scanning Paused.") (DRWUI:1098139, :1098406-24);
  `sigTaskBarPaused(bool)` wiring (:1323035).
- **Core (cite):** `ScanTelemetrySnapshot.paused` (cr_scan.h:509; cr_ui.h:658
  `paused, running, cancelled`); the N-075 query pair is busy/stopping only
  (cr_ui.cpp scan_busy_impl/scan_stopping_impl).
- **Verdict:** hosts can observe pause via telemetry; a dedicated query would be cosmetic.
  INFO.

### LD-8 — COVERED-BY-N-125 (state-machine note): disk disconnect mid-scan — the og has NO disconnect scan-state either; it is a UI-modal + retry loop, not a state transition

- **Og (cite):** the "Drive Disconnected" Retry/Cancel modal `FUN_1406c3d20` (:1110914-1111230,
  telemetry Click_Retry/Click_Cancel "Disconnect" :218373-218404) pumps on the UI thread while
  the lifecycle state stays 1/5; the retry re-enters the read path in place (no engine state
  change decoded).
- **Core (cite):** ERR_READ_FAILED storm → read-truth refusal (rrecov.cpp:1776-1790);
  continuation only via session resume.
- **Verdict:** N-125 owns the classification/flow gap. Lane-D's contribution: on the STATE
  axis both sides lack a disconnect value — the og's answer is a modal, the core's is an error
  code; a core-side "disconnected" classification (N-125's fix plan) would be a superset, not
  a parity repair.

### LD-9 — INFO (core superset): state ownership — the og's master lifecycle enum is an UNLOCKED int written cross-thread; the core's state is single-owner with mutexed telemetry

- **Og (cite):** plain `*(undefined4 *)(svc + 0x78) = N` writes from the scan thread
  (FUN_1400f1a90/b10) and UI handlers (FUN_1400fcb90) with no lock observed; the locked
  discipline exists only for nStatus (`FUN_1401acf10` :313726-757, mutex `+0x90`) and
  m_nFreeze (:612604-612612).
- **Core (cite):** `tel_.m` guards every telemetry read/write incl. the teardown race
  (cr_scan.cpp:556-582, :1142-1160); the PausableToken is mutex+cv (cr_device.h:187-259);
  the event contract documents worker-thread delivery (cr_ui.h:143-146).
- **Verdict:** no action; the core's shape is the improvement.

---

## 3. OPEN QUESTIONS

1. **The og's stop-path auto-save (LD-2's premise) — one decode gap:** the completion
   handler's first gate reads config `+0x6c` (the stop reason set by `FUN_1401b0c40`
   :315626-628) and skips the handler when nonzero (:612528-530). The CStopScanThread path
   passes 0 (handler runs), and the stop-organize modal (case 3) proves the organize phase
   runs on stop — but the OTHER stop entries (`FUN_1401ae610`'s param_2) are undecoded. A
   short probe of `FUN_1401ae610`'s callers would close the last alternative explanation
   (a stop variant that skips the auto-save).
2. **The pause gate's engine-side wait** — the `+0xf8` event is closed on pause
   (ResetEvent, DRWUI:314408) and reopened on resume (SetEvent, :314484), but no
   `WaitForSingleObject` on it was found in DRWUI or the searchdll decompiles this session
   (the engine's cancel poll `FUN_1401aee10` waits on the ABORT event `+0x108`, :314616-624).
   The gate's waiter is likely a UI/timer thread (the elapsed-time ticker). One probe of
   handles passed at engine-observer wiring would settle whether engines block on the gate
   or only on the explicit `Pause(1)` call (LD-5's depth).
3. **The og's disconnect retry mechanics** — N-125's open question (engine error code vs
   UI heuristic) stands; the state-machine half is now answered (no state transition — the
   modal blocks while the state stays scanning), so any core-side disconnect classification
   should be designed as an error-code + host-flow surface, not a lifecycle state.
4. **Whether the og's `FUN_14039ec00` third auto-save site** (the not-already-saved save,
   :616748-616764, reached via the message-dispatch layer `FUN_1401d43a0` :339321) fires on
   app exit / go-home (a save-on-exit window the core lacks entirely) — its dispatch caller
   is vtable-mediated and undecoded.
5. **Session-load state restoration fidelity** — the og restores the lifecycle state
   (6/7/8/9/10) from the session at load (:837740-800) so the loaded session presents as
   "stopped" or "finished"; the core's summary carries masks/counters only. If a host wants
   the og's "this session was stopped at 34 %" presentation, the summary would need a
   stopped-at-save flag (derivable: configured_mask ∧ ¬stage_done_mask ≠ 0). Cosmetic;
   noting for the UI-readiness plan.

---

## Candidate table (compact)

| ID | Sev | One-line |
|---|---|---|
| LD-1 | ACUTE | CLI `--resume` w/o `--out` truncates the session at the ck-sink open (rrecov.cpp:1576) BEFORE the resume read (:1598) — the printed resume command destroys the session (mechanism executed) |
| LD-2 | SEVERE | Og auto-saves STOPPED scans' partial results (stop-organize → completion handler, no stopped gate); core's cancelled runs never auto-save (cr_ui.cpp:2147/2166, rrecov.cpp:1392) |
| LD-3 | MAJOR | CLI checkpoint = truncate-in-place, no commit hook (rrecov.cpp:1574-1583) — the N-014 shape cr_ui fixed; crash mid-emit destroys the previous good checkpoint |
| LD-4 | MINOR | No CLI pause verb (og ships a pixel-verified Pause control; the ABI has pause) |
| LD-5 | MINOR | Engine pause notification absent (og: engine->Pause(1/0) vtable+0x88; core: token wait only; no decoded og engine-side pause behavior to mirror) |
| LD-6 | INFO | Stopped-vs-finished: og's progress<98 % heuristic + persisted verdict vs core's truthful stage mask (core improvement; mapping recorded) |
| LD-7 | INFO | Paused-state observability: telemetry-only vs og's enum + nStatus + UI strip + taskbar |
| LD-8 | COVERED-BY-N-125 | Disconnect: og has NO disconnect scan-state either (UI modal + retry); core = error code + session-resume continuation |
| LD-9 | INFO | State ownership: og's master enum unlocked cross-thread vs core single-owner + mutexed telemetry (core superset) |

Verified-MATCH surfaces (no candidate): pause/resume semantics incl. elapsed compensation;
cancel keep-results; cancel pool latency (N-017 fixed); auto-save gates + naming + sanitizer;
no-engine-state-persisted parity (S-052's LDM re-arm landed, others N/A both sides); load
identity re-check + stale-candidate drop; save/load progress + cancel (N-060/W1b); session
list scope (N-090); progress delivery (D-78); bad-sector error map; watchdog lineage (N-043);
the discovery per-hit persistence (registered with M-042).
