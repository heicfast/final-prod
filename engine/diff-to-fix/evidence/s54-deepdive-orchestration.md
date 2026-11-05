# Deep dive F — orchestration/session/UI: state, persistence, math, hang policy (pin 2794da8b / corpus be0cfe9)

Read-only parity-audit deep dive on four axes: (1) scan-lifecycle state management, (2) local
database / result persistence, (3) progress / rate / size mathematics, (4) no-stuck-or-freeze
policy. Everything in the two repos was read; nothing was modified. Every load-bearing corpus
claim below was re-decoded first-hand from `DRWUI_decompiled.c` this session (line cites are
the corpus file's own line numbers); every core claim was read at pin `2794da8b`.

Exclusions honored: S-057, M-126, M-073, M-101, M-134, M-135/M-136, M-118, M-119, I-041,
I-042, I-043, N-142 (liveness note only, below), M-107 — plus the adjacent families verified
as already-filed during the walk (M-125, N-166, N-167, N-178, N-090, N-183, M-127, M-129,
M-131, A-013, S-044/W1c gates, N-043, N-062, D-18/D-121/D-122/D-118).

---

## Coverage log

**Core (pin 2794da8b), read fully unless noted:**
- `include/cr/orchestrator/cr_scan.h` (831 ln, full) — ScanConfig/ScanStats/StageWeights/
  event stream/telemetry/ScanController contract.
- `src/core/orchestrator/cr_scan.cpp` (2,788 ln, full) — run()/stages/engine dispatch/LDM/
  S-051 table targets/lost discovery + candidate fan-out/carve wiring/telemetry + events/
  checkpoint emit/save_session/load_session_summary/read_summary/resume; probe_family.
- `include/cr/session/cr_records.h` (251 ln, full) + `include/cr/session/cr_crsf.h` (169 ln,
  full) + `docs/crsf-format.md` (full) — the CRSF record grammar v1..v5.
- `src/core/session/cr_crsf.cpp` / `cr_records.cpp` — targeted reads (chunk framing, CRC,
  skip-forward, tree serialize hooks; grep-verified behaviors cited).
- `src/ui/cr_ui.cpp` (4,665 ln) — full read of: CrScan/CrSession/CrSnapshot/CrRecovery
  structs (:780-960), event trampoline + heartbeat watchdog + memory monitor (:965-1100),
  progress bridge (:1100-1180), scan_run_impl + autosave + checkpoint sink (:2104-2250),
  pause/resume/cancel + busy/stopping queries (:2252-2330), save-session atomic + user-save
  (:2668-2730), session resume (:3274-3370), disk-map path (:3195-3270); remainder skimmed
  via symbol census (recover/preview/thumbnail surfaces — M-115/M-135/M-136 territory).
- `src/app/rrecov.cpp` (3,496 ln) — full read of: CLI options + resume wiring (:1365-1710),
  resume verify hooks (:1705-1730), scan loop + stamp + save-on-cancel + exit codes
  (:1730-1990), autosave leg (:1990-2050); remainder skimmed (recover/report verbs).
- `include/cr/storage/cr_device.h` (:138-240 CancelToken/PausableToken, full),
  `include/cr/storage/cr_retry.h` + `src/core/storage/cr_retry.cpp` (grep-targeted),
  `include/cr/cr_parallel.h` (CancelPoolWatcher, full), `src/core/tree/cr_grading.cpp`
  (351 ln, full), `include/cr/cr_ui.h` (event defines + session row, targeted),
  `src/core/query/cr_query.cpp` (progress_display helpers + size buckets, targeted).
- `docs/PARITY-DELTAS.md` (D-18, D-118..D-123 sections), `docs/crsf-format.md`,
  `diff-to-fix/SUMMARY.md` (S12/S42 lane blocks), `diff-to-fix/findings/` (I-041/I-042/I-044,
  M-125, M-126, N-090, N-142, N-178, N-183 files read in full), `diff-to-fix/FIXED-LEDGER.md`
  (N-066 row).

**Corpus (pin be0cfe9), first-hand decodes this session (all line cites re-read):**
- `DRWUI_decompiled.c`: the 14-value state enum + name map (:208243-208296); the start
  setter FUN_1400f1a90 (:205773-205789); the finish/stop classifier FUN_1400f1b10
  (:205791-205827); the pause writer (:208960-209026); the resume setter FUN_1400f9200
  (:209028-209038); the Click_Stop analytics (:209040-209130); the stop-confirm dialog
  state writes (:210770-210830); the stop-reason store FUN_1400f94a0 (:209142-209152);
  the CStopScanThread ctor + stop relay (:1096652-1096673); the state-freeze gate + restart
  (:339283, :339395-339412); the CUIScanProgressInfo locked carrier — SetStatus with the
  pause-archive protocol (:313636-313673), GetStatus effective-status (:313713-313734),
  GetEffectiveStatus (:313735-313760), the monotonic max-merge GetProgress (:313762-313815);
  CScanMgr ctor (:313817-313860); the pause machinery FUN_1401ae530/FUN_1401ae720 (engine
  vtable+0x88 notify :314402/:314486, the type gate, the paused-duration accumulator
  :314470-481); doStartLogic — ETA budget seed (:315140-315160) + SetEvent (:315171-315173);
  the liveness watchdog FUN_140577100 (:910565-910585) + its spawn/teardown (:910857-910864,
  :911013-911019) + the per-partition coarse-progress model (:910862, :910984-910988);
  the two size formatters FUN_140163580/FUN_140163650 (:264525-264583) and the tr-family
  B/KB/MB/GB/TB formatter (:1556295-1556400, TB at :1556359); the countdown label seed +
  1 Hz tick (:1527645-1527780); the 99→100 telemetry clamp family (:204366, :206959,
  :208521, :209004, :209105, :210022, :210138, :210541, :210725, :210893, :213184, :217627,
  :219454, :219571 + more — `progress(+0x1b0) < 99 ? progress : 100`); the completion
  handler auto-save gates (:612540-612620); CSaveAndRestoreResult::SaveResult (:440936-
  441120) + the record-header writer path (corpus artifact §4 cites re-read); the restore
  flow + all-or-nothing teardown (:432440-432520, :187040-187160); RestoreFileTree reader
  contract (artifact §5.4 cites); the device-classification state writes (:837720-837810 —
  confirmed to be device-type 6..10 classification, NOT a session-verdict restore; I-041's
  ":837740-800" cite is this region).
- `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (full, 420 ln) — record
  1/2/3/4 grammars, chunk framing, writer/reader function map, save/restore progress
  contracts.
- `re-workspace/artifacts/UIUX/`: 04-userflow-state-machine.md (full), 08-scan-phase-deep-
  dive.md (full), dynamic-ground-truth--scan-phase-captured.md (full incl. both addenda),
  35-redesign-reference-atlas.md (paused/stop-confirm sections), 02-string-inventory
  (grep), 11-recovery-phase-funnel.md (grep), 12-repair-ui-flows.md (grep).
- `re-workspace/artifacts/DRWUI-exe--thread-architecture.md` (full) — the 45-thread census
  (CStopScanThread, SaveRestoreThread, EndRecoverThread, CommonThreadPoolMgr, MainWndExitThread).
- `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md` (grep/targeted).
- `og-codebase/` persistence census: `EUUserData` (JSON, 16,891 B), `Dict` (ASCII, 2,742 B),
  `raw_signs.json`, `config/`, `SaveScan/` (3 real .rsf), `PartitionRecovery/lostpart.dat`
  (1,096 B, "Part" magic) + `partinfo.dat` (24 B, "Part" magic), registry/QSettings surfaces;
  DRWUI decompile grep for sqlite/jet/oledb/ESE imports: ZERO hits.
- `ghidra-core/DRW_decompiled.c` (surveyed — it is the loader: CFileMappingLog shared-memory
  log protocol, not a scan orchestrator).
- `ghidra-mo/Device_decompiled.c` (grep: DeviceIoControl sites, no timeout wrappers).

---

## Axis 1 — state management comparison

### The og's master state model (decoded first-hand)

**The 14-value master enum at the service singleton +0x78** (name map FUN_1400f7290,
DRWUI:208243-208296):

| Value | Name | Meaning |
|---|---|---|
| 0 | `notstarted` | idle |
| 1 | `quickscanning` | quick pass running |
| 2 | `quickscanpaused` | quick pass paused |
| 3 | `quickscanstopped` | quick pass stopped by user |
| 4 | `quickscanfinish` | quick pass completed |
| 5 | `advancedscanning` | advanced pass running |
| 6 | `advancedscanpaused` | advanced pass paused |
| 7 | `advancedscanstopped` | advanced pass stopped |
| 8 | `advancedscanfinish` | advanced pass completed |
| 9 | `importedscan` | a saved session was imported (results view, no live scan) |
| 10 | `repairvideo_scanning` | video-repair scan running |
| 11 (0xb) | `repairvideo_stop` | video-repair stopped |
| 12 (0xc) | `repairvideo_pause` | video-repair paused |
| 13 (0xd) | `repairvideo_complete` | video-repair completed |

Singleton progress fields around it: `+0x1b0` = current progress percent (int); `+0x8c` =
first-ever quick start tick; `+0x90` = current quick start tick; `+0x94` = quick duration;
`+0x98` = advanced start tick; `+0x9c` = advanced duration; `+0x7c/+0x80` = stop-reason
payload (stored only when state == 3, FUN_1400f94a0 :209132-209145).

**Transition writers (who writes +0x78):**

| Trigger | Writer | Decoded behavior |
|---|---|---|
| Scan start | FUN_1400f1a90 (:205773-205789) | mode 0 → state 5, `+0x98=GetTickCount()`; mode 1 → state 1, `+0x90=tick` (and `+0x8c=tick` once if 0). No guard on prior state — an unconditional overwrite. |
| Engine/scan end | FUN_1400f1b10 (:205798-205827) | advanced: `if (state==5 && progress(+0x1b0) < 0x62) state=7 else state=8`; a global flag (FUN_1401b3ec0()+0x100, the repair-video coupling) remaps 7→0xb / 8→0xd. `+0x9c = tick − +0x98`. quick: `+0x94 = tick − +0x8c`; `if (state != 3) state=4` (a previously-stopped quick stays stopped). **The `<0x62` (98%) classifier is I-041's registered heuristic.** |
| Pause | :208984-208996 | `state = 6 (mode 0) / 2 (mode 1)`; repair-video flag → 0xc. Then a "Click_Pause" analytics payload carrying progress clamped `(<99 ? p : 100)`. |
| Resume | FUN_1400f9200 (:209041-209048) | `state = 5 / 1` directly from paused — a plain flip, no re-entry bookkeeping at this layer. |
| Stop confirm | :210806-210809 | from the confirm dialog: `state = 7 (param_3==0) / 3` then the stop relay FUN_1400fcd80. "Click_Close"/"Click_Continue" leave the state untouched. |
| Restart/freeze | FUN_1401d4550 (:339395-339412) | sets the freeze flag `+0xa0`, notifies the module chain, then **unconditionally writes state=1** (quickscanning) and re-arms the grading container — the re-scan entry. |
| Broadcast (frozen) | :339283 | every broadcast writer is gated `if (+0xa0 == 0) +0x78 = new_state` — the master enum can be LOCKED OUT during transitions (a freeze/unfreeze pair around the restart flow). |
| Imported session | state 9 (`importedscan`) | set by the session-import flow (recorded in the enum name map; the writer site is the import path, corpus-dispositioned with N-090/I-041's restore family). |

**Readers:** the analytics name-map (:208243-296), the stop-confirm dialog, the stop-reason
store, the service-layer observers (I-042's nStatus==2 UI strip, sigTaskBarPaused), and the
finish classifier itself. **Locking: none on +0x78** — the og's own mitigation is the
parallel locked carrier (below). This is I-043's registered ownership divergence (core:
single-owner + mutexed telemetry snapshots, D-122).

**The locked parallel carrier — CUIScanProgressInfo** (mutex at +0x90):
- `SetStatus` (:313636-313673): under the mutex — when the new status ∈ {3,4,5} while
  status == 2 (paused) and not already archived: copy the 0x28-byte progress block
  (+0x40..0x68) to the archive slot (+0x68), set the archived flag (+0x98), zero the live
  block, set the archived status field (+0x6c) = 2. Then `+0x44 = new`.
- `GetStatus` (:313713-313734): returns the ARCHIVED status (2) while the archive flag is
  set and the live status != 5 — every query keeps reporting "paused" until the advanced
  state arrives.
- `GetProgress` (:313762-313815): monotonic max-merge of the done/total counters
  (`+0x60 = max(+0x60, +0x88)`, `+0x5c = max(+0x5c, +0x84)`) — the og's monotonicity
  enforcement lives HERE, at the carrier, not in the engines; plus the one-shot archive
  restore (clears the flag, zeroes the trio, returns the archived block).
- Observed nStatus values: 2 (paused), 3/4/5 (transitions), {4,5,7,8} (the
  terminal/advanced set returned verbatim by the effective-status query :313735-313760).

**Pause semantics (engine side)** — FUN_1401ae530/FUN_1401ae720 (:314388-314500):
- Pause: request flag `+0x149=1`; **engine notified `engine->vt+0x88(1)`** (:314402);
  **gated on `scan_type(+0xD8) != 1`**: only then `paused flag +0xdd=1`, pause-start tick
  `+0xe4=GetTickCount()`, and the manual-reset **event gate at +0xF8 is Reset** (workers
  block on it). For scan type 1 the UI state flips but the engine gate never closes.
- Resume: `+0x149=0`; service notified (state 5/1 again); **`paused_ms = tick − +0xe4`
  (clamped ≥0) is ADDED to the accumulator at +0xE0** (:314470-475) — the pause-duration
  compensation the core's `PausableToken::paused_ms()` mirrors (verified RE-parity, S12
  lane D); engine notified `vt+0x88(0)` (:314486); gate Set; a status field (+0x1b4) = 4.
- Cancel-during-pause: the stop-confirm path (:210806-09) writes 7/3 from ANY state and
  runs the stop relay; nothing in the pause path blocks it. The core: `PausableToken::
  cancel()` wakes paused waiters (cr_device.h:195-208) — same observable.
- What happens to in-flight engine work on pause: the og's engines observe the event gate
  (and the vt+0x88 notification; the engine-side consumer is undecoded — N-178's register
  note). The core: engines block in `checkpoint()` at loop heads; in-flight reads complete
  on their own (cr_device.h:138-145 contract). Resume continues from the same position on
  both sides — neither restarts the pass.

**Cancellation path**: user → confirm dialog → state 7/3 → CStopScanThread (cooperative
stop; its body relays into the module singleton `FUN_1401b19d0()->vt+0x28(0)`,
:1096652-1096673) → engines poll their cancel callbacks in loop heads (D-18's registered
model; no IRP cancellation in the driver — driver F4). Partial-result retention after
cancel: S-057 (og auto-saves stopped scans; core does not) — not re-filed. Cancel during
recovery vs scan: the og's recovery has its own progress dialog + EndRecoverThread; the
core's recovery is a separate PausableToken on SafeWriter (cr_safewriter.cpp:313-631) —
per-file failure isolation + cooperative cancel, M-135/M-136 own the read semantics.

**Checkpointing**: the og writes NO mid-scan file-tree checkpoint — .rsf saves happen at
completion/stop windows (S-057's three trigger sites) and manually; the only per-hit
mid-scan persistence is `lostpart.dat` (PRD:20409, per-found-partition append). The core:
stage-boundary checkpoints through the configured sink (cr_scan.cpp emit_checkpoint_,
G-13) + the forced per-lost-candidate checkpoint (M-042's mirror) — a deliberate superset,
registered in the M-042 comment ("the core has no module-local .dat side-channel").

### The core's state surface (comparison table)

| Og master state (+0x78) | Core equivalent at pin 2794da8b |
|---|---|
| 0 notstarted | `ScanController` constructed, `run()` not called; `telemetry().running == false` (cr_scan.h:516-526) |
| 1/5 scanning | `tel_running_` true + stage telemetry (quick/deep/carve); `cr_ui_scan_busy` (N-075) |
| 2/6 paused | `ScanTelemetrySnapshot.paused` (telemetry-only — I-042's registered shape) |
| 3/7 stopped | run() returned `ERR_CANCELLED` → `snap.cancelled = true` (cr_scan.cpp stamp()); stage bits NOT stamped (I-041's truthful replacement) |
| 4/8 finish | `stage_done_mask` bits (STAGE_QUICK/DEEP/CARVE) stamped only on truthful completion |
| 9 importedscan | no dedicated state — a loaded session surfaces via `read_summary`'s `stage_done_mask`/`configured_mask` (cr_records.h:238-246); hosts derive "results view, no live scan" from all-stages-done + `running=false`. Covered by the I-041/I-043 register family (no separate finding). |
| 10/0xb/0xc/0xd repairvideo | out of scope (the repair product lane; S-050 family) |
| freeze gate +0xa0 | no analog needed — single-owner state (I-043/D-122) |
| stop reason +0x7c/+0x80 | `ERR_*` return codes + `stats` fields (exit 0/1/3/5 taxonomy, rrecov.cpp:1868-1912) |

Transition trigger parity: start (run()), pause/resume (token), stop (cancel + stamp),
finish (stage end + mask), disk-change (N-125 filed), disconnect (N-125/exit-5). The
core's quick→deep auto-advance is inside one run() (I-044 registered the og's engine-side
state queue as the equivalent). The og's pause-type gate (scan type 1 skips the engine
gate) has NO core analog — the core pauses in every stage; direction: core superset
(N-178 addendum note below, not a standalone file).

**Verdict:** the state-model comparison is fully dispositioned by I-041/I-042/I-043/I-044 +
N-178; no new state-management divergence survives exclusion. Two mechanism datums are new
(the pause type-gate, the +0xa0 freeze gate) and are recorded here for the trail.

---

## Axis 2 — persistence / .rsf / local database comparison

### The og's complete persistence census (verified against og-codebase/)

| Mechanism | What it holds | Evidence |
|---|---|---|
| `.rsf` session (SaveScan/) | the ONLY found-file store: record 1 (session state: entryCount + 0xA8-stride "special partitions" list + 4 flag bytes), record 2 (volume/session info: sig 0x7F, volumeName[100], deviceType, SYSTEMTIME, stat1/stat2, flagB/D), record 3 (partition struct 0x6F5 + volumeDevice blob 0x1518: bus type, model, serial, firmware, WWN, interface, 512-B boot snapshot), record 4 (the full tree, BFS, per-node content objects for 10 FS classes) | rsf-format artifact §2-§5 (validated on 3 shipped samples) |
| `lostpart.dat` (PartitionRecovery/) | per-found-lost-partition records, appended per hit mid-scan ("Part" magic; 1,096 B shipped) | PRD:20409; og-codebase file |
| `partinfo.dat` (PartitionRecovery/) | the whole-disk layout snapshot at search start ("Part" magic; 24 B shipped = header-only) | PRD:20998-21039; M-042's registered semantics |
| `EUUserData` (file, JSON) | the user-sign database (16,891 B; 111 keys/190 signs) | M-129/M-133 territory |
| `Dict` (file, ASCII) | the carve dictionary (2,742 B) | I-033 register |
| `raw_signs.json`, `config/`, `FileFilter.xml`, `NasConfig/` | static config/data assets | I-027/I-033 registers |
| Registry `SOFTWARE\EASEUS\...` + QSettings | product/scenario state, window geometry | artifact 04 §7 |

**There is NO sqlite/jet/EDB/OLE-DB found-files database anywhere**: DRWUI's import table
has no DB-engine APIs (grep: zero sqlite/jet/oledb hits in the decompile and function
list); the og-codebase tree contains no *.db/*.edb files; the .rsf IS the result store,
reloaded wholesale. The core's model (one CRSF session = the result store; EUUserData JSON
ported; lostpart/partinfo semantics folded into the session as REC_PARTITION_CONTEXT +
REC_DISK_LAYOUT) is the same architecture — the module-local .dat sidecars were registered
as the honest difference in M-042.

### Field-by-field: og record set vs core CRSF record set

| Og (.rsf) | Core (CRSF) | Verdict |
|---|---|---|
| R1 session state: entry list (0xA8-stride: 2×u64, u32, u8, u64, name) + trailer + 4 flags (is-whole-disk, module flags, device vt+0xf0) | REC_SESSION_STATE: stage_done_mask, files/deleted/carved counters, bytes_scanned, error_map_ranges (v1/v2); the scan-target identity rides REC_VOLUME_INFO + REC_DISK_LAYOUT instead | Dispositioned (S12 corpus rewalk: "Record 1 ... session.md + N-090 + M-079 + WinPE-variant note") |
| R2: sig 0x7F, volumeName[100], deviceType (2=Desktop/3=RecycleBin/else +0x120), SYSTEMTIME local+ms, stat1 elapsed-ms, stat2 tick counter, flagA, flagB (partition-table-scan bool), flagC/D | REC_VOLUME_INFO v1..v5: fs_kind, slice_offset/bytes, logical/physical sector, label[32], scanned_bytes, elapsed_ms, flags(configured stages), model/serial/boot_hash (v2), image[256] (v3), firmware/wwn/interface/boot_snapshot[512] (v4), save_unix (v5) | N-090 filed (label width, 1-s save precision, no deviceType); N-066 landed the v4 tail; stat2 undecoded (N-090 open question) |
| R3: partition struct (raw 0x6F5 memcpy) + volumeDevice blob 0x1518 (bus type, model, serial, firmware, WWN, interface, boot snapshot) | REC_PARTITION_CONTEXT v1/v2 (start/count/fs/score/kind + bps/SPC/hidden/flags) + the v4 identity tail | N-142 filed; **liveness note below** |
| R4: full tree, BFS, marker byte, per-FS content objects (NTFS/FAT/exFAT/RAW/HFS+/ReFS/Btrfs/ext4/Generic/ZFS), DataRunList, FLAGS u16 (has-children 0x10), timestamps (RAW: formatted string + u64 filetime), sizes, type ids | REC_RESULT_TREE: DFS with parent ids, node flags, marker, origin/deleted_state/confidence, record_ref, fs_kind, logical/allocated size, 4×filetime (100 ns), attributes, flags, name, extents, carve_format/confidence, sha-128, role+stats (full nodes), basis ids | M-101/N-131/N-142 families; the tree grammar is the corpus-pattern port (cr_records.h:25-40) |
| — (no og equivalent) | REC_ERROR_MAP, REC_ASSISTANT_STATS, REC_ALLOC_MAP, REC_FS_CHECK, REC_LDM_LAYOUTS | core supersets, registered (S-023/S-009/M-041/S-052) |

### Session size / rotation / compression / triggers

| Property | og | core |
|---|---|---|
| Compression | chunked zlib level 5, 4 KiB chunks, stored form when deflate does not shrink; inflate failure → raw-payload fallback | identical scheme (proven from the og; cr_crsf.h:4-12) + per-record CRC32 the og lacks (F10) |
| Size cap/rotation | none observed (shipped samples 7.37/9.0/52.7 MB; record 4 up to 226 MB decompressed) | none (same full-save model); free map persisted only downsampled (≤8 MiB) |
| Incremental saves | no (whole record set per save; writer seeks EOF per record within one save) | no (full record set per save/checkpoint; the checkpoint sink rewrites from position 0 — cr_scan.cpp:709-716) |
| Auto-save triggers | completion windows ×3 (stop-organize / quick-complete / advanced-complete) with S-044's gates (not-import, results-found, not-already-saved), stop-window (S-057), manual SaveScan (progress 40→65% + per-child IsCancelled) | completion auto-save (W1c, same gates minus the og's stop-window — that delta IS S-057), save-on-cancel (CLI --out only), manual (ABI + CLI, N-060 progress+cancel), stage-boundary checkpoints (G-13, superset), per-lost-hit checkpoint (M-042, superset) |
| Save progress contract | writer 40% + 50·done/(2·remaining); restore 40 + 50·done/(total+done) | SerializeProgress/DeserializeProgress hooks per node (N-060/W1b verified parity) |
| Crash-safety | fresh header per save; torn chunk degrades to raw bytes; no CRC (silent field corruption possible) | tmp+rename commit (N-014), per-record CRC + skip-forward (F10), torn load discarded (W1b) — strictly safer |
| Append-vs-truncate on manual save into an existing .rsf | writer seeks EOF and appends record headers; the reader chain-walks to the first matching record-type GUID — the truncate-vs-append question is UNDECODED (see UNRESOLVED) | one session per file; manual save rewrites via tmp+rename |

**N-142 liveness note (requested):** N-142's core-side premise ("no firmware/WWN/interface
string/boot-sector bytes anywhere in the session layer") is **STALE at pin 2794da8b** —
N-066's v4 tail landed exactly those fields (`cr_records.h:119-122` firmware[32]/wwn[32]/
interface_desc[32]/boot_snapshot[512]; populated at `cr_scan.cpp` save path from
`vol_boot_snapshot_`/`cfg_.volume_firmware` etc.). What remains of N-142 is (a) the
modules/session.md parity-table row disposition and (b) its open question (the variant
save path's trigger). The active-board entry should be narrowed or retired-by-N-066 at the
next bookkeeping pass; no new finding filed (exclusion honored).

---

## Axis 3 — progress / rate / size math comparison

Already-filed and verified standing (not re-filed): M-125 (the piecewise display remap
FUN_1401af820 + 99.99 defensive cap + monotonic publish + per-phase restart; the query
helpers landed), M-126 (ETA rate-EMA + time-budget variant ×0.6 + "3m00s" seed + 1 Hz
countdown — still open), I-041 (the 98% stopped/finished classifier — re-verified at
:205805 this session), N-166 (counter cadence), N-167 (found-size aggregate), N-183
(grading gauge population + score formula + QSDE mapping), S-058/S-061/M-134 (grading
rules), M-107 (size-walk stand-ins).

**Newly decoded this session (recorded for the trail):**

1. **Three distinct percent-cap families coexist in the og** — worth keeping separate in
   any parity table: (a) the display cap 99.99 + the five-segment remap (M-125);
   (b) the stopped/finished classifier threshold 98 (0x62) (I-041, :205805); (c) the
   **telemetry/analytics clamp `progress < 99 ? progress : 100`** — a 20+-site family
   (:204366, :206959, :208521, :209004, :209105, :210022, :210138, :210541, :210725,
   :210893, :213184, :217627, :219454, :219571, …) on every Click_Pause/Click_Stop/
   completion analytics payload. The core has no analytics-payload surface (telemetry
   carries truthful permille) — no divergence to file (the core's event payloads are the
   honest surface; D-118's doctrine), but parity tables must not conflate the three caps.

2. **The ETA time-budget SEED** (addendum for open M-126, not a new finding):
   `budget(+0x120) = (vtA() × vtB()) / 0x3C00000 + 60` at scan start
   (DRWUI:315143-315150 — the device/product object from FUN_1401b4fd0, its +0x18/+0x28
   accessors multiplied, almost certainly total-bytes) — i.e. the og seeds the time budget
   M-126's type-gated variant consumes at **1 MiB/s assumed throughput + a 60 s base**
   (0x3C00000 = 60 MiB; 60 = 0x3C). M-126's decode references `budget(+0x120)` but not
   its seed; the fix-plan helper needs this constant to be complete.

3. **The og's coarse-progress model for partition-driven scans** (:910857-910988):
   `expected_steps = 100 / partition_count` (integer truncation), progress = expected ×
   partition_index, watchdog +1%/5 s top-up (Axis 4). With 3 partitions the curve steps
   0/33/66/99 and only completion forces 100. The core's whole-disk flow reports the
   weighted permille over stages (no per-partition display model). Registered under
   D-118's engine-truth doctrine + I-044's stage-model register; no separate file.

4. **The countdown label's stale-estimate behavior** (:1527645-1527780): the og's
   "Remaining time" label is seeded "3m00s" when empty and then **ticks down locally at
   1 Hz regardless of estimate refreshes** — on a stall the displayed remaining time
   walks to zero while the gauge freezes, then the label is hidden/reset when it reaches
   negative. M-126 owns the ETA family; this sub-datum (countdown-driven-to-zero on
   stall) belongs in its fix-plan notes.

5. **Size formatting** — the two og formatter families (candidate F-1 below).

6. **Units**: the og's progress message family carries sector-based done/total (M-125's
   decode; the live ticker "Reading sector: 134316032/31413598" — semantics UNRESOLVED,
   below); the core's carve stage counts bytes and quick/deep count engine units with
   `bytes_done` stage-true only where honest (cr_scan.h:507-515). The core's StageWeights
   40/35/25 are the core's own model registered by D-118 (the og has no cross-stage
   weights — its display restarts per phase).

---

## Axis 4 — watchdog / timeout / stall policy comparison

**The og's liveness watchdog (FUN_140577100, :910565-910585; spawn :910864; teardown
:911013-911019):** `while (stop == 0) { Sleep(5000); if (step_counter(+0x8fc) <
expected(+0x8f8)) { coarse_progress(+0xe8) += 1; step_counter += 1; re-fire the progress
callback (observer vt+8 with the progress payload); } }` — i.e. every 5 s with no engine
step, the og FABRICATES one percent of coarse progress and re-fires the UI callback. It
spawns from the partition-scan driver (the DoScan flow that also drives the main
whole-disk scan's per-partition controllers). **Teardown is bounded: stop flag set, then
`WaitForSingleObject(watchdog, 10000)` and `CloseHandle` regardless** (:911016-911019) —
a 10 s join cap; an unresponsive watchdog thread is abandoned (handle closed, thread
leaks). Other bounded joins in the og: pool shutdown 60 s per worker + 60 s
WaitForMultipleObjectsEx (:644514-644521, :653624), process-exit waits 5/8/10/30 s
(:249485, :873896, :178734, :670399), VAPreviewListener 100×100 ms bounded sync wait
(:654531-654553).

**Core (pin 2794da8b):**
- The heartbeat watchdog (N-043, landed): fires CR_EVT_HEARTBEAT every 5 000 ms while
  run() is in flight and no CR_EVT_SCAN_PROGRESS arrived in the window; carries the
  STANDING telemetry truth (permille never advances without work — the honest variant of
  the og's fabricated +1); the N-062 memory monitor rides the same tick. Cradle:
  cr_ui.cpp:992-1090, started :2157-2167, joined :2170-2175. Join is unbounded
  (`std::thread::join`) but structurally bounded (cv wait_for 5 s + predicate — it cannot
  hang), so the og's 10 s cap has no core analog *needed*; core strictly safer.
- Progress-stall detection: NEITHER side aborts on stall — the og fabricates progress,
  the core reports a heartbeat; both remain cancelable (D-18's first-class cancel:
  CancelToken in every loop head + overlapped/CancelIoEx propagation, vs the og's engine
  callback polls + the driver's uncancellable KeWaitForSingleObject — driver F4).
- Pool cancel responsiveness: the core's CancelPoolWatcher polls the token every 50 ms
  and aborts the pool (cr_parallel.h:106-127; N-017) — the og has no equivalent queue-
  drain guard (a queued og candidate would run to its first loop-head poll).
- Bad-sector/stall-adjacent: the retry pipe (max_attempts 3, bisection to sector
  granularity, Skip-mode zero-fill, error map) — the og's bad-sector skip escalation
  (2 MiB → ×2-per-3-steps → 64 MiB) is registered (S42 wave-5); the og's
  "Program is trying to skip bad sectors to find data." message surface (:1551139) is
  the og's stall-adjacent UX the core surfaces as the error-map/warn lines.
- UI message-pump discipline: the og is Qt with every non-trivial operation off-thread
  (45-thread census; UI-thread purity) — the core is a library; cr_ui's run() is
  synchronous with callbacks + the event stream, and the ABI documents the threading
  contract (ABI-CONTRACT §6). D-18/D-122 territory; no divergence.
- Per-file recovery timeouts: none on either side (the og's recovery failures surface
  through its per-file error objects; the core through SafeWriter's error code + stats —
  M-135/M-136). Infinite-retry guards: the og's retry ladder is the registered escalation
  (capped 64 MiB); the core's retry is bounded at 3 attempts + bisection; neither loops
  forever.

**Verdict:** the no-stuck policy comparison was already dispositioned (N-043's honest
watchdog variant + D-18's cancellable-first design + the registered escalation ladder);
the S12 lane's "7-input hang-trace table (no unregistered hang asymmetry)" conclusion is
corroborated by this walk — the only NEW hang-policy datum is the og's bounded-join
family (10 s watchdog / 60 s pool), which the core exceeds in safety (bounded-by-
construction joins), so no finding.

---

## Candidate findings (numbered; severity/module/cites+quotes/gap/impact/fix direction) — excluding already-filed IDs

### F-1 (MINOR) — the og's two byte-size display formatters (binary /1024 tiers) have no core counterpart; hosts cannot render og-faithful sizes from any core surface

- **Severity:** MINOR (display-math helper absent; no data corruption; the M-125
  progress-display family is the landed precedent for this class).
- **Module:** query / display math (orchestration-adjacent).
- **Corpus cites (first-hand this session):**
  - `DRWUI_decompiled.c:264525-264558` (FUN_140163580, wide-char) and `:264559-264583`
    (FUN_140163650, ascii): `if (bytes < 1) return 0; else if (bytes < 0x40000001) {
    if (bytes < 0x100001) v = bytes/1024.0, "%.2f KB"; else v = bytes/1048576.0,
    "%.2f MB"; } else v = bytes/1073741824.0, "%.2f GB"` — **binary units under decimal
    labels; NO bytes tier; NO TB tier; the unit flips only above 1 MiB+1 B and 1 GiB+1 B
    (exactly 1 MiB renders "1024.00 KB")**.
  - `DRWUI_decompiled.c:1556295-1556400` (the tr-family QString formatter): tiers
    `"%1 B"` (< 0x401), `"%1 KB"` (< 0x100001), `"%1 MB"` (< 0x40000001), `"%1 GB"`
    (< 0x10000000001), `"%1 TB"` (÷ 1099511627776.0, :1556359) — the full tier set with
    a B tier and a TB tier, same binary steps, `'f'` number format with a caller
    precision (forced 0 when bytes == 0) plus a separator-strip replace.
  - Runtime anchor: "Files Found: 1139157 (143.74 GB)" / "147.12 GB" on a 149.45 "GB"
    volume (dynamic-ground-truth--scan-phase-captured.md:95,134-135) — consistent with
    GiB values rendered under "GB" labels.
- **Core cites:** grep over `src/` + `include/` for a human size formatter (GB/MB/KB/TB
  tiering): ZERO hits. The CLI prints raw bytes / one fixed `%.1f MiB` site
  (rrecov.cpp:1915-1917, `double(stats.bytes_unreadable) / (1024.0 * 1024.0)`); the ABI
  carries raw u64 bytes; the query layer's only size math is the bucket filter
  (cr_query.cpp size buckets). The M-125-landed precedent family lives at
  `include/cr/query/cr_query.h:109-119` (`progress_display_*`) with no size sibling.
- **Falsifiable gap:** at pin 2794da8b, no core function maps a byte count to the og's
  display string; two og formatters with DIFFERENT tier sets (log-side: KB/MB/GB only;
  UI-side: B..TB) mean a host's naive decimal-GB rendering diverges from the og at every
  boundary (e.g. 1 MiB exactly → og "1024.00 KB", naive "1.00 MB"; 2 TiB → og "2048.00
  GB" on the log-side formatter, "2.00 TB" on the UI-side one).
- **Impact:** presentation-layer only — but it is the sizing shown beside every found-file
  count, every recoverable-size aggregate (N-167's missing aggregate would render through
  this), and every disk-map tooltip; hosts building og-faithful UX must reimplement an
  undocumented tier set (including which of the two og formatters applies where).
- **Fix direction:** land `query::size_display_string(u64 bytes, u32 precision)` (the
  tr-family tier math verbatim: 0x401/0x100001/0x40000001/0x10000000001 thresholds,
  /1024 steps, 'f' format) and optionally `query::size_display_log_string` (the
  KB/MB/GB-only variant) beside the progress_display family; pin the boundary values
  (1024 B stays "B"; 1 MiB exact stays "1024.00 KB"); note which surface uses which in
  the helper docs. Tests: `SizeDisplay.TierBoundaries`, `SizeDisplay.ZeroForcesZero`
  precision, `SizeDisplay.TBExists`.
- **Exclusion check:** N-090 (session-row shape), N-167 (the aggregate value), M-107
  (carve size walks) — none cover the formatter. Not filed before (no cite of
  FUN_140163580/650 or the tier ladder anywhere in the core tree).

### F-2 (ADDENDUM to open M-126 — not a new ID) — the og's ETA time-budget seed decoded: 1 MiB/s assumption + 60 s base

- **Corpus cite:** `DRWUI_decompiled.c:315143-315150` — `budget(+0x120) =
  (FUN_1401b4fd0-object vt+0x18() × vt+0x28()) / 0x3C00000 + 0x3C` at scan start
  (doStartLogic), where the product is the scan-target total (bytes) and 0x3C00000 =
  60 MiB.
- **Core cite:** M-126's finding text references `budget(+0x120)` in the type-gated
  variant but decodes neither the field's seed nor its writer; the core has no ETA
  surface (M-126 open).
- **Gap/impact:** M-126's fix-plan helper cannot reproduce the og's early-scan estimates
  without the seed (before the first progress message the budget IS the estimate's
  basis). Falsifiable: with a 60 GiB volume the og seeds budget = 60×1024 MiB/60 MiB/s +
  60 = 1084 s.
- **Fix direction:** fold into M-126's helper (`eta_budget_seed(total_bytes) =
  total_bytes/(60 MiB) + 60`), cite :315150 in M-126's corpus references at its next
  revision; no separate file (M-126 owns the family and is open).

### F-3 (N-178 ADDENDUM note — not a new ID) — the og's pause machinery gates the ENGINE pause on a scan-type field; type 1 skips the gate

- **Corpus cite:** `DRWUI_decompiled.c:314405-314410` (pause) and `:314476-314481`
  (resume): both apply the event-gate transition (ResetEvent/SetEvent on the +0xF8
  manual-reset event, the paused flag, the pause-start tick / duration accumulator) only
  `if ((int)param_1[0x1b] != 1)` — for scan type 1 the service-layer state still flips
  to paused (FUN_1400f8f40) but the engine gate never closes (and the engine vt+0x88
  notification still fires at :314402).
- **Core cite:** `PausableToken::pause()` gates on nothing (cr_device.h:170-178); every
  stage's loop-head `checkpoint()` blocks. Direction: core superset (pausable in all
  stages).
- **Disposition:** the writer of CScanMgr+0xD8 is undecoded (which mode is type 1 —
  quick scan is the leading hypothesis given m_iScanType 1=quick elsewhere, but
  unproven); until decoded this is a register note on N-178 (whose pause-machinery cites
  are the same region), not a standalone finding. If a future probe shows type 1 = the
  quick phase, the divergence is "core pauses the quick phase, og cannot (engine-side)"
  — a benign superset worth one sentence in N-178's register.

### N-142 liveness (no new ID) — see Axis 2: the field gap is closed by N-066's v4 tail at the audited pin; only the parity-table row + the variant-path-trigger open question remain.

---

## UNRESOLVED (precise next probes)

1. **"Reading sector: 134316032/31413598" semantics** (dynamic-ground-truth :102,135).
   The format string is NOT in DRWUI's `tr()` census (grep "Reading sector" in the
   decompile: zero hits) and the observed X > Y (134 M > 31 M) rules out a plain
   numerator/denominator. Probe: grep the SearchDLL/RAWSearchDLL decompiles + the
   ui-resources RCC/qm string tables for the literal; decode the producing call site;
   determine the units (bytes? clusters? cumulative-read vs remaining?) and whether the
   og mislabels them. Until decoded, no parity claim about the core's bytes-based
   `Reading`-equivalent surfaces should be filed.
2. **The pause gate's scan-type field**: find the writer of CScanMgr+0xD8 (the
   `param_1[0x1b]` int) — the FUN_1401ad1e0 ctor region and the doStartLogic call chain
   are the starting points; the value set will pin which scan mode cannot engine-pause
   (F-3 above).
3. **The .rsf save-path open mode**: decode the CRSDataEx vtable+0x18 open
   (FUN_1405197d0's class; the `(*vt[0x18])(obj, path, 1)` call at :440984) — truncate
   vs append on a manual SaveScan into an existing .rsf, and what the reader
   (chain-walk to the FIRST matching record-type GUID, FUN_14051b0b0) then returns.
   Matters only for the manual-save-over-existing-file edge; the auto-save path always
   names a fresh file.
4. **stat2 (record-2 +0x84) semantics** — N-090's open question. This session's datum:
   stat2/stat1 = 821,833,551,923/239,480 = 3.431×10⁹ ticks/s on sample I_ — consistent
   with a QueryPerformanceCounter/invariant-TSC rate, i.e. a redundant high-resolution
   elapsed. Probe: the scan-session +0x38 writer to confirm QPC before mirroring anything.
5. **Which duration feeds record-2 stat1** (scan-session +0x30 → SaveResult's
   `vt+0x58` read at :441020): quick duration (+0x94, measured from the FIRST-EVER
   session tick +0x8c per FUN_1400f1a90/FUN_1400f1b10), advanced duration (+0x9c), or
   the pause-compensated accumulator (+0xE0, FUN_1401ae720)? The core persists one
   pause-compensated run-total `elapsed_ms` (verified RE-parity for the compensation
   model itself); the og's source-field choice is unpinned.
6. **The organize phase's state position** ("Organizing found files..." modal,
   CScanCommonPopDlg; CSetClassifyThread/CSortNodeThread workers): does the master enum
   stay 5/1 during organize, and can the "Stop Organizing Found Files?" path leave the
   tree half-classified? Probe the organize entry/exit handlers around the completion
   chain (:612540-612620 is the entry). The core has no organize phase (query-time
   classification); if the og's organize-stop persists a partially-classified tree,
   that is a presentation-layer divergence for the UI repo, not the core.
7. **The og's main-scan (non-partition-driver) stall surface**: the watchdog spawn
   (:910864) sits in the partition-container driver; confirm every CDetailPage scan
   routes through it (the controller factory at :903252-380 suggests yes), or whether
   the pure volume scan (no partition list) has NO watchdog at all — in which case the
   core's heartbeat is a superset for single-volume scans too (register-grade).
