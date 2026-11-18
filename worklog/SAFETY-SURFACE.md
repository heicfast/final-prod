# SAFETY-SURFACE — the complete memory-safety / stability surface inventory

> Agent A, Lane E (safety-hardening session), Phase-0. READ-ONLY static audit of
> `/home/z/my-project/final-prod` at HEAD `eb53a64e` (baselines final-prod@dedb0a46,
> corpus@be0cfe9 per the session prompt), VERSION 0.9.4. Date: 2026-11-18.
> Scope: every safety-relevant surface of the engine + Rust bridge — threads, locks,
> raw memory, handles, parse boundaries, error paths, recursion, prior-lane defect
> state. Windows-only code read as source, never executed (Linux container).
> Every `file:line` below was verified by reading the file this session; nothing is
> guessed. "unverified" marks what static reading could not decide. This sheet is
> the session's only created file. Paths are repo-relative.

## 1. Component map

Engine C++20, one static core + one `cr_ui` shim; `CR_UI_API` function declarations
in `engine/include/cr/cr_ui.h` number **94** (grep count), matched 1:1 by 94 `pub fn`
bindings in `ui/src-tauri/src/bridge/abi.rs`. LOC via `wc -l` (.cpp+.h).

| Directory | Files | LOC | Role | Portability |
|---|---|---|---|---|
| engine/src/core (28 subdirs) | 84 | 55,273 | pure engines: ntfs(6f/3705), fat(3/1967), exfat(2/1501), ext4(1/1553), hfsplus(1/1815), btrfs(2/1360), zfs(1/1809), refs(2/1723), ldm in table(3/2131), carve(6/12,199 incl. cr_analysers.cpp 7392), orchestrator(5/3887 incl. cr_scan.cpp 3295), session(3/1857), preview(5/3349), query(3/2462), recovery(4/1762), reghive(5/1131), repair(1/1052), check(2/1132), apply(1/760), health(7/1135), crypto(2/508), compress(1/286), storage(6/2176), tree(4/1632), lost(2/1556), bcdfix(1/111), nvramfix(1/300), err(3/233), cr_parallel.cpp(181) | portable; a few `#ifdef _WIN32` branches (cr_query.cpp, cr_apply.cpp, cr_mftwalk.cpp, cr_crsf.cpp, cr_lastscan.cpp, cr_refs_engine.cpp) |
| engine/src/app | 1 | 4,012 | rrecov.cpp — CLI (wmain :3890), bench verbs, pause-reader thread | Windows-only (CLI builds on Linux too, but verbs need the win layer) |
| engine/src/ui | 1 | 5,216 | cr_ui.cpp — the C-ABI shim: CrScan/CrDevice/... wrappers, heartbeat watchdog, guarded() exception fence | portable shim over win layer |
| engine/src/win | 12 | 3,847 | cr_transport(887L), cr_bitlocker, cr_bootfix, cr_destio, cr_elevate, cr_enumerate, cr_fsck, cr_health(+wmi), cr_image_create, cr_image_win, cr_mountpoints | **Windows-only** (11/12 include windows.h) |
| ui/src-tauri/src | 26 | 11,918 | bridge/{abi(1052),state(856),events(741),filefilter(1538),consent(264),logging(281),error(140),util(327)}.rs; commands/{results(1155),sessions(738),scan(717),diagnostics(656),imaging(631),devices(570),recovery(569),preview(486),system(328),lost_partition(246),maintenance(178),health(175),repair(72)}.rs; lib.rs(141) | Rust, Windows-target; 219 `unsafe` occurrences |

## 2. Threading map

Spawn sites (complete: `std::thread|CreateThread|_beginthreadex` grepped; no
`_beginthreadex`, no Win32 threadpool):

| # | Site | What | Notes |
|---|---|---|---|
| T1 | engine/src/core/cr_parallel.cpp:160 | `CancelPoolWatcher` watcher thread (50 ms poll → pool.abort()) | joined in `done()` at :178 and by dtor :174-176 |
| T2 | engine/src/core/cr_parallel.cpp:28 | `ThreadPool` worker ctor loop (N workers) | joined in dtor :38-40; workers 0→hw-1 clamp 1..8 (:13-20), hard cap 64 (:26-27) |
| T3 | engine/src/ui/cr_ui.cpp:2435 | heartbeat watchdog `std::thread(heartbeat_loop_, s, ctrl)` | joined at run end :2443-2444 and defensively at close :3074-3079 |
| T4 | engine/src/app/rrecov.cpp:1853 | `CreateThread(pause_stdin_reader)` | **detached on purpose** (comment :1845-1848); waits 1000 ms + CloseHandle at :1862-1866 — a reader stuck past 1 s leaks the handle until process exit (bounded, CLI-only) |
| T5 | engine/src/core/orchestrator/cr_scan.cpp:2473 | lost-candidate `ThreadPool pool(threads)` | per-stage scope; watcher T1; threads = resolve_threads :58-68 (0→hw-1 clamp 8, >64→64) |
| T6 | engine/src/core/ntfs/cr_mftwalk.cpp:2370 | remnant-sweep `ThreadPool pool(threads)` | `PoolCancelToken pool_cancel(&pool)` at :2371 is constructed but **never passed anywhere** — dead wiring (see §8) |
| T7 | engine/src/core/carve/cr_carve.cpp:1631 | carve window fan-out `ThreadPool pool(flags.threads)` | CancelPoolWatcher used (cr_scan.cpp:2487 pattern) |
| R1 | ui/src-tauri/src/commands/scan.rs:362 | run thread (`cr_ui_scan_run` sync) | std::thread; no join — detached by design, lifetime pinned by entry Arc + registry |
| R2 | ui/src-tauri/src/commands/recovery.rs:318 | process thread (`cr_ui_recovery_process`) | same pattern as R1 |
| R3 | ui/src-tauri/src/commands/imaging.rs:403 | image emitter thread | loop exits on `finished` flag (:411-415); no join |
| R4 | ui/src-tauri/src/commands/imaging.rs:481 | image worker thread (`cr_ui_image_create_ex`) | sets `finished` at tail; no join |
| R5 | ui/src-tauri/src/bridge/events.rs:344 (`spawn_scan_emitter`) | scan emitter drain thread | exits when channel closes (rx iterator end) or **poisons out** at :346 |
| R6 | ~40 `tauri::async_runtime::spawn_blocking` sites (scan.rs:102, devices.rs:496/506/528, health.rs:21, sessions.rs:91/151/363/378/417/463, recovery.rs:61/373/393, results.rs:502/521/895/911, preview.rs:471/483, system.rs:120/215/258/295, lost_partition.rs:61/193/214/239, maintenance.rs:84/155, diagnostics.rs:72/169/222/258/365/535/607, imaging.rs:251/555/613) | blocking pool wrappers | bounded by Tauri's runtime |

**Unbounded/interesting:** none of the pools exceeds hw-1 clamp 8 (cap 64). The
heartbeat thread T3 is the only engine thread touching shared `CrScan` state
(`heartbeat_mu`, telemetry read at :1162-1180 — lock released before `ctrl->telemetry()`,
documented no-nesting at :1160-1163). **Join-skippable:** none found engine-side —
T2/T1 dtor-join, T3 joins on both run-end and close. Bridge threads R1-R5 are never
joined by design (detached at spawn); their safety rests on the Arc/registry
discipline (see §9 BF-32 — one known gap).

## 3. Lock inventory

| Lock | Guards | Cite | Held-while-blocking / across IO / nesting |
|---|---|---|---|
| `CachedReader::mtx_` | whole read loop **incl. transport fill** | cr_cached.cpp:31 (take), :61 (fill under lock); decl cr_cached.h:81 | **fill under lock = device IO while held** — registered perf issue (DIAGNOSIS.md Layer 2), not a correctness bug (single-flight by ABI contract) |
| `RetryingDevice::map_mutex_` | error-map add/merge | cr_retry.cpp:33/:38/:112; decl cr_retry.h:77 | short critical sections only |
| `DirectDevice::heal_mu_` / `RwDevice::heal_mu_` | dead-handle reopen | cr_transport.cpp:287/:712; taken :272/:502/:520 | reopen (CreateFileW) while held — bounded, 1 attempt |
| `ResultTree::mutex_` | **one global tree mutex** — every add/reparent/detach/snapshot | cr_node.h:656; all uses cr_node.cpp:133-577 (reparent :390-392) | tree ops only, no IO; query path `query::run` walks `roots()` const with **no lock** (cr_query.cpp:173-190) — safety rests on ABI one-thread-per-handle + G-02 snapshots (cr_node.h:584) |
| `ScanController::tel_.m` | telemetry snapshot/counts/stage | cr_scan.h:830; ~20 uses cr_scan.cpp:479-2616 | taken briefly; **nesting rule documented**: ctrl_mu → tel_.m allowed, never the reverse (cr_ui.cpp:846 comment); heartbeat trampoline takes heartbeat_mu only after tel_.m released (cr_ui.cpp:1160-1163, cr_scan.cpp:805-820) |
| `CrScan::ctrl_mu` / `progress_mu` / `heartbeat_mu` | shim control / single-flight progress / watchdog state | cr_ui.cpp:848/:880/:903 | "ctrl_mu never nests with progress_mu" (cr_ui.cpp:846-847 comment); heartbeat cv wait is the intended block (heartbeat_loop_ :1142-1180) |
| carve `ctx.group_mtx` | volume-group child cache | cr_carve.cpp:294; taken :552 | held while creating nodes (tree mutex nested **inside** group_mtx → tree.mutex_ ordering: group_mtx → tree mutex; the reverse never occurs — verified by reading both sites) |
| carve `c.lock` (CarveNameCounters) | per-prefix name counters | cr_carve.cpp:261 | trivial |
| `ThreadPool::mutex_` + cv | task queue | cr_parallel.cpp:35-100 | standard cv pattern |
| `usersigns::g_mu` | process-global user-sign set | cr_usersigns.cpp:333; uses :340-365 | snapshot-once-per-carve (D4f fix, cr_carve.cpp:180-193) removed the hot-path take |
| `cr::log g_mu` | log site registry | cr_log.cpp:25; take :60 | read fast path lock-free by design (comment :26-30) |
| image `gt_cache->mu` | grain-table cache | cr_image.cpp:251; uses :260/:270 | short |
| `image_win mu_` | image handle state | cr_image_win.cpp:107; uses :24/:52/:62/:87/:98 | short |
| `PausableToken m_` + cv | pause/cancel state | cr_device.h:220-240 (checkpoint blocks at :220-223) | `checkpoint()` **blocks while paused holding m_ via cv wait** — intended; engines poll at loop heads |
| bridge: `Mutex<ScanLive>`, `Mutex<Terminal>`, `Mutex<registries>`, `AtomicBool` | live state, registries | events.rs:339-341, state.rs (consents :527-544), scan.rs:258 | sinks `try_send` only (never block, events.rs:59); **poisoned-lock exits** at events.rs:346/:586 (see §7) |
| `g_last_transport_error` atomic | last GLE | cr_ui.cpp:1422 | racy by design (diagnostic) |

No lock-ordering violation found; the two documented orderings (ctrl_mu→tel_.m;
group_mtx→tree mutex) are the only nestings observed.

## 4. Raw allocation audit

Raw C-heap sites (all found by grep; engine is otherwise RAII/`std::vector`):

| Site | Pattern | Read |
|---|---|---|
| cr_ui.cpp:273 `dup_str` malloc; :1391 free | ABI strdup | checked null, freed by `cr_ui_query_rows_free` :3953-3960 and row-error unwind :3933-3937 — **clean** |
| cr_ui.cpp:3546 malloc → :3629 `cr_ui_disk_map_free` free | overlay out-param | owner-frees contract, documented |
| cr_ui.cpp:3915 calloc row array | query rows | unwind on dup_str failure :3933-3937 — clean |
| cr_transport.cpp:628 `_aligned_malloc(len, ssec)` staged write | freed on all 3 exits (:642/:648/:654) — **clean** |
| cr_bitlocker.cpp:64-66 `_wcsdup`/`free(mutable_cmd)` | CreateProcessW needs mutable | freed immediately after call — clean |
| cr_fsck.cpp:48-52 same | same | clean |
| cr_image_create.cpp:99 `(void)DeleteFileW` | cleanup cast-to-void | intentional best-effort |
| rrecov.cpp:3081/:3127 `_aligned_malloc` bench buffers | freed at :3098/:3146/:3180 on every return path of each mode — verified read |
| cr_rawsigns.cpp:30 `call_once { new SignIndex() }` | **intentional leak** (process-global index) | benign by design |
| `new (std::nothrow) FileProperties` × ~30 sites (all engines) | node-held props; refcounted (cr_node.h:498-532), released on add; session-load uses `PropsRef` RAII (cr_records.cpp:386-429) | reviewed — ownership documented at cr_node.h:345 |
| cr_node.cpp:117 arena `new unsigned char[cap]` | node arena, freed with tree | fine |

**memcpy without visible bound (top sites, with my read):**

- cr_table_parse.cpp:47/:132 `memcpy(&h, sec.data(), sizeof(h))` — GPT header
  from a 1-sector read; `sec` is exactly 512 B, `GptHeader` ≤ 92+pad — **safe**;
  entry-array copies :110/:153 gated by `entry_size==sizeof(GptEntry)` and
  `GPT_ENTRIES_READ_CAP` (:141-143 salvage path) — **bounded**.
- cr_mftwalk.cpp:496/:582/:801/:2106/:2241 record-header copies — all preceded by
  buffer-size ≥ record checks (`avail < stride` at :1488; `fix` sized
  `record_size` at :2105); **checked** at each read site.
- cr_mftwalk.cpp:1861-1876 fixed-offset copies from `$INDEX_ROOT` entries —
  guarded by `pos+26 <= dlen` class checks in the entry loop (cr_attributes.cpp:87-92 pattern); spot-checked OK.
- cr_ui.cpp:255-291 `copy_bstr/copy_cstr/dup_str/copy_card` — all cap-bounded
  (`n < cap-1`); **safe by construction**.
- cr_ui.cpp:4471 thumbnail `memcpy(rgba, thumb.rgba.data(), bytes_needed)` —
  preceded by `rgba_cap < bytes_needed → ERR_NO_MEMORY` at :4466-4469 — **checked**.
- cr_ui.cpp:2156/:2219/:2245 `memcpy(g.b, sec+0x30, 16)` — from 512 B sector
  buffers; offsets < 512 — safe.
- cr_health.cpp:281 `memcpy(out512, req.data, 512)` — fixed 512→512 — safe.
- lznt1_payload_decode (cr_decomp.cpp:147-190) — every copy bounded by `d_end`/
  `s_end` checks (:154/:158/:165-166/:171-172); **tightly checked**.

**Fixed-size stack buffers fed from external input:**

- cr_scan.cpp:966 `u8 sec[512]` — read_at(0, sec, 512) fixed length; safe.
- cr_scan.cpp:1110/:1114 `u8 anchor[4096]`/`vds[4096]` — read of `ss` bytes
  (512..4096 gated at :1109) into 4096 — **safe** (ss ≤ 4096 gate present).
- cr_vollabel.cpp:147 `u8 sb[1024]`, :169 `u8 boot[512]` — fixed-size reads; safe.
- cr_smart_ata.cpp:102-127 `u8 identify/table/thr [512]` — DeviceIoControl out
  buffers, fixed — safe.
- cr_query.cpp:659/:697/:735 `char buf[32/48]` — size-format only, snprintf-family — safe.
- cr_ui.cpp:130-133 `char buf[512]` + `vsnprintf(buf, sizeof(buf), ...)` — bounded — safe.
- win layer `_snwprintf_s(..., _TRUNCATE, ...)` throughout (cr_enumerate.cpp:139/:298,
  cr_transport.cpp:329/:335/:369/:427/:731/:772/:802, cr_bitlocker.cpp:42/:133/:218/:239/:533,
  cr_health.cpp:177 `swprintf_s(port_path, 32, ...)`) — all `_TRUNCATE`/explicit-bound — safe.
- cr_enumerate.cpp:274-286 `wchar_t stack_buf[512]` + exact-size heap retry — the
  E-01 fix; **safe** (see §9).
- cr_bitlocker.cpp:41 `read_boot_sector(wchar_t letter, u8 sec[512])` — ReadFile 512 — safe.
- fsck.cpp:71 `wchar_t buf[MAX_PATH+1]` GetWindowsDirectoryW — bounded — safe.

No sprintf/strcpy/strcat (unchecked) anywhere — verified by grep.

## 5. Handle audit

CreateFileW sites (26 total; INVALID_HANDLE_VALUE checked at every one — verified
by reading each site): cr_transport.cpp:241 (reopen), :292 (open_path R),
:337 (volume), :428 (volume RW dismount), :686 (volume RW #2), :732 (open_rw_device),
:773, :803, :861; cr_enumerate.cpp:101/:110 (two-step query open), :299;
cr_bootfix.cpp:127/:139; cr_image_create.cpp:56; cr_destio.cpp:40/:156;
cr_bitlocker.cpp:44/:219 (temp .bek)/:261; cr_image_win.cpp:37; cr_health.cpp:178
(SCSI port); cr_ui.cpp:4953 (health open RW); rrecov.cpp:2724. 47 CloseHandle
calls; every open site pairs with a close on the success and error paths in the
functions read (e.g. read_boot_sector closes at :52; run_command closes
pi.hProcess+pi.hThread at :73-74; live_spawn at cr_fsck.cpp:66-67).
**No RAII handle wrapper exists** — discipline is manual per-function; the one
aggregate owner, `WinDestIo` (cr_destio.h:17-56), has **no destructor**: a
SafeWriter abort that bypasses per-item `close()` (cr_safewriter.cpp calls
`io_.close(item_id)` on 15+ exits — spot-verified :406/:412/:534/:570/:593/:610/:644/:669/:692/:757/:769)
would leak the still-open temp HANDLEs until process exit. I could not construct
a path that skips all closes (every early-exit read has one) — **unverified
whether a leak path exists**; flagged as a design gap (no dtor safety net).
Registry: RegOpenKeyExW/RegCloseKey paired (cr_bitlocker.cpp:432/:474, :502/:506;
cr_mountpoints.cpp:52/:63/:84/:86). Processes: 3 CreateProcessW sites
(cr_bitlocker.cpp:64, cr_fsck.cpp:50, cr_health.cpp:511) all CREATE_NO_WINDOW,
handles closed on both exits; **on timeout the child is left running on purpose**
(cr_fsck.cpp:68-71 comment — chkdsk-kill risk). Events: no CreateEvent usage
(engine uses cv; bridge uses channels).

## 6. Parse boundaries (external input entry points)

| Boundary | Entry | Validation visible? |
|---|---|---|
| MBR/GPT/EBR | `parse_disk_table` (cr_table_parse.h:102; impl cr_table_parse.cpp:226+) | signature + header_size==92 + reserved + entries CRC + array-inside-disk + `GPT_ENTRIES_READ_CAP`; EBR `depth < EBR_DEPTH_LIMIT` loop :208, returns ERR_CYCLE_LIMIT :270 |
| GPT salvage | `salvage_primary_entries` :121-160 | entry_size==128, count cap, CRC, LBA-in-disk :141-143 — strict |
| LDM | `parse_ldm_database` (cr_ldm.h:299); VBLK walk cr_ldm.cpp:699-711 | bounded walk_bytes, "VBLK" terminator |
| NTFS boot/MFT | probe_family (cr_scan.cpp:964-1030: bps∈[512,4096], %512, spc!=0, USA bounds); record walk cr_mftwalk.cpp:481-496; fixup `fixup_validate` (cr_fixup.cpp:48-73: USA offset band [0x2A/0x28,0x100], count ≤0x10, sectors×ss ≤ record) | **multi-gate, dense** |
| NTFS attrs/runs | `parse_attr_list_entries` (cr_attributes.cpp:79-105, MAX_ENTRIES 512, reclen bounds); `parse_data_runs` (cr_runlist.cpp, RUNLIST_MAX_RUNS 0x100000 at cr_runlist.h:91) | bounded |
| LZNT1 | `expand_compressed_data` (cr_mftwalk.cpp:282-345: cluster ≤4096, cunit ≤15, fail→no-extents) → `lznt1_payload_decode` (cr_decomp.cpp:147) | **tightly bounded** |
| FAT/exFAT | `scan_fat` (cr_fat_engine.h:331)/`scan_exfat` (:142); bps/spf bounds cr_chain.cpp:98-103; dir walk `walk_dir_entries` (cr_fat_quick.cpp:120) per-dentry cancel + MAX_DEPTH 64 (:79/:125) | bounded |
| ext4 | `scan_ext4` (cr_ext4_engine.h:129); extent recursion `walk_extent_node` (cr_ext4_engine.cpp:372-383: per-node entry bound, EXTENT_DEPTH_LIMIT 5 :97, global node budget max_blocks) | bounded |
| HFS+ | `scan_hfsplus` (:98); btree nodes gated MAX_TREE_NODES 4M, node_size∈[512,65536] (cr_hfsplus_engine.cpp:98/:159); leaf/index height gates :702-705 | bounded (iterative walk :402) |
| Btrfs | `scan_btrfs` (:126 in engine); `walk_tree` **iterative** with visited-set + MAX_NODES + queue cap 65536 (cr_btrfs_engine.cpp:385-420); `populate` lambda recursion depth-capped DIR_DEPTH_LIMIT 64 (:131/:1049) | bounded |
| ZFS | `scan_zfs` (:88); `nv_parse_stream` depth ≤6, pairs <512 (cr_zfs_engine.cpp:408-418); gang recursion `plan_gang_pieces` depth ≤4 (:1037-1041) | bounded |
| ReFS | `scan_refs` (:121); `walk_table` MAX_TABLE_DEPTH 32 + MAX_NODES_PER_TABLE 4096 + visited-set (cr_refs_engine.cpp:63-64/:268-271); `attr_walk_runs` depth ≤32, hops ≤64 (:433-436) | bounded |
| Carve | `carve_slice` (cr_carve.cpp:1494; sole orchestrator call cr_scan.cpp:2762); 1 MiB+4 KiB windows (:45-46), per-window cancel + OOM guard :1504-1513; registry `validate_registry` F8 gate :1499 | bounded |
| User signs (.crsus text) | `usersigns::parse` (cr_usersigns.cpp:207; JSON-ish scanner with depth/skip logic :85-130) | bounded scanner |
| Disk images | `open_image_impl` dispatch (cr_image.cpp:1473-1480 depth>32 guard); VMDK :522 (sparse hdr :217, descriptor :387), VHD :784, VHDX :1240 — all depth-capped 32 | bounded |
| Sessions (.crsf) | `Reader::open` (cr_crsf.cpp:216-276): magic, version, chain walk guard 65536, `pos+hdr+chain > total` gate :253, torn-tail clamp :262-268; `crsf_chunk_parse` :45-62 (payload_len vs len) | bounded |
| Session records | `cr_records.cpp` decode (fixed-size Cursor `g8/g32/g64` with fail flag; props under PropsRef :386-429) | bounded cursor |
| Registry hives | `cr_reghive.cpp` regf/hbin/nk/vk (offsets corpus-decoded, header :1-45); `collect_list_offsets` depth ≤8, list cap 4096 (:228-229) | bounded |
| Thumbnails | `cr_thumbnail.cpp` Budget{read_left,max_work} (:102-124), caps default 200 MB read / 4 Mi work / max_pixels→256 MiB RGBA (cr_thumbnail.h:76-98; cr_ui.h:1422-1428) | bounded |
| Preview | `cr_preview.cpp:24-25` len clamp 4 MiB; kMaxDecode 256 MiB gate :59-60; zip central-directory walk capped `kMaxPreviewArchiveEntries` 512 (cr_preview.h:145, walk cr_preview.cpp:1161-1189) | bounded |
| Health | NVMe query static_assert-placed buffer (cr_health.cpp:418-437); SAT passthrough :260-282 | fixed shapes |
| BitLocker keys | offline hive read_file_bytes bounded (cr_bitlocker.cpp:260+); AesXts init rejects len ∉{32,64} and weak keys (cr_aes.cpp:304-316) | bounded |
| BCD/NVRAM | `plan_rewrites` over hive bytes (cr_bcdfix.cpp:33), consent-gated writes :65 | bounded |

## 7. Error-path audit

- **Empty catch (10 sites, all in cr_ui.cpp)**: `catch (...) {}` around
  `delete` in the five `*_close` exports — device :1864, session :3637, snapshot
  :3823, preview :4298, recovery :4893 — plus scan_close :3081 and LDM arm
  :2262 (break-with-comment). The close-family empties are deliberate (a throwing
  dtor must not cross the C ABI) but **swallow the failure silently** — no
  `ret_` diagnostic is possible on a void return; acceptable, worth a log line.
- `guarded()` (cr_ui.cpp:138-145) converts exceptions to ERR_INTERNAL with a
  message — the main fence; **every** `cr_ui_*` numeric export goes through it
  (verified by reading the export bodies).
- carve parallel_for catch → ERR_INTERNAL, never crosses the thread edge
  (cr_parallel.cpp:126-128) — good.
- **(void)-cast ignores**: only 2 engine-wide (exfat_quick.cpp:482/1003 progress
  callbacks — display-only, intended; image_create.cpp:99 delete). Rust bridge
  `let _ =` on ~25 sites (events.rs:59/:276-278/:302-303 emits and telemetry
  probes; consent.rs:165/:228; scan.rs run-tail) — emits are best-effort by
  design; the telemetry `let _` at events.rs:276-278 means **a failed probe reads
  as zeroed flags** (comment at :291-295 says honest-no-data) — intentional.
- **Poisoned-lock permanent exit**: `let Ok(mut live) = live.lock() else { break }`
  at events.rs:346 (scan emitter) and :586 (recovery emitter) — one panic
  anywhere holding `live` kills all further progress events with no restart
  (T6 in WIRING-AUDIT; **still open** at HEAD).
- run_command/live_spawn return −1 on CreateProcessW failure with no
  GetLastError capture (cr_bitlocker.cpp:67-68, cr_fsck.cpp:54-55) — diagnosable
  only via the engine log.
- `session_resume_impl` leak-check: `delete scan` on every early return
  (cr_ui.cpp:3572-3579...) — read clean.

## 8. TODO/FIXME/HACK/dead wiring

- **Zero TODO/FIXME/HACK/XXX markers** in engine/src and ui/src-tauri/src (grep;
  the only hits are literal `\\uXXXX` / `WD-XXXX` strings in comments).
- Dead wiring found: `PoolCancelToken pool_cancel(&pool);` constructed at
  **cr_mftwalk.cpp:2371 and never used** — the remnant sweep relies on workers
  polling the external token (chunk lambda :2376-2382) with **no 50 ms watcher**,
  unlike the carve (:1631+) and lost (:2473+) pools which use CancelPoolWatcher.
  Effect: a cancel during the remnant fan-out leaves QUEUED chunks starting until
  each reaches its own checkpoint — the exact class N-017 fixed elsewhere
  (cr_parallel.h:103-109 comment). **Live defect candidate (small, bounded).**
- Rust test-only `unwrap()`s at util.rs:317-325 are inside `#[cfg(test)]`
  (:310) — fine.
- Unbound-but-declared ABI surface (94 bindings, several never invoked — e.g.
  `cr_ui_query_run` family) is documented policy (abi.rs header), not dead wiring.

## 9. Prior-lane defect register — claimed state vs re-verified code

| ID | Sheet claim | My re-verification at HEAD eb53a64e |
|---|---|---|
| BF-ABI-01/L1 (disk_info stride drift) | fixed via abi.rs re-pin | **FIXED**: `bus_type` present cr_ui.h:435 ↔ abi.rs:211; layout gate `tools/gen_abi_layouts.py` + bridge-integration.yml exist |
| L2 (smart_report +16 stack smash) | fixed | **FIXED**: `power_cycles_nvme` cr_ui.h:1603 ↔ abi.rs:730; health.rs:35 local now matches; ex2 twin used first (health.rs:38-44) |
| L3 (smart_attr.threshold) | size-neutral | **CONFIRMED**: cr_ui.h:1562 ↔ abi.rs:683 `pub threshold: u32` (72==72) |
| D-NVMe-01 (NVMe pack) | fixed | **FIXED**: static_assert placement cr_health.cpp:418-437 (offset 8, proto 40, payload 48) |
| D-ENUM-02 (bus table shift) | fixed | **FIXED**: SDK-order switch cr_enumerate.h:130+ |
| M4 (kind 'sd' never fired) | open→fixed | **FIXED**: `map_kind` bus 12/13→"sd", 7→"usb" (devices.rs:151-161) |
| D4f (usersigns mutex freeze 605 s) | fixed | **FIXED**: per-carve snapshot + lock-free probe (cr_carve.cpp:180-193 comment, cr_usersigns.cpp:333 registry) |
| E-01 (volume strings overflow) | fixed | **FIXED**: 512-wchar stack + exact-size retry (cr_enumerate.cpp:274-286) |
| P-HB-01 (heartbeat swallowed) | fixed 0.9.3 | **FIXED**: CR_EVT_HEARTBEAT refresh+emit (events.rs:435-448) |
| T5 (poll never refreshes telemetry) | open | **FIXED**: `probe_scan` in scan_state (scan.rs:596) — heartbeat also refreshes (events.rs:438-448) |
| T6 (emitter dies on poisoned lock) | open | **STILL OPEN**: events.rs:346/:586 `else { break }` |
| BF-32 (emitter UAF window) | open, deferred W10 | **STILL OPEN**: emitter takes `SendPtr` raw scan ptr (events.rs:341, scan.rs:348-353), not the entry Arc; run tail drops Arc (scan.rs:362+); window between final emits and last drain |
| D4a-d (live carve bytes/ticks) | fixed | present: cr_carve.cpp:156-166 ctx comment, scan counters |
| BF-04 (_popen console) | fixed 68460a4c | **FIXED**: CreateProcessW+CREATE_NO_WINDOW (cr_bitlocker.cpp:64, cr_fsck.cpp:50, cr_health.cpp:511); no _popen/system( anywhere |
| BF-22/BF-25 (75% freeze / unbounded cancel) | fixed via D4c | intra-window checkpoint present (cr_carve.cpp:158-161); CancelPoolWatcher on carve pool |
| BF-06 (NG-21 whole-disk repair guard) | mitigated bridge-side | bridge-side only; engine-side guard **unverified** beyond the bridge comment (scan.rs NG-21 pre_scan_repair=0 whole-disk) |
| BF-30 (carve 8.45 MiB/s wall) | open, perf | perf-scope, not memory-safety; unchanged |
| D-IMG-1/D-IMG-3 (image cancel/range) | closed W4 | `cr_ui_image_create_ex` with cancel flag + sector range wired (imaging.rs:483-492) |
| I-STALE-01 / I-CLEAN-01 / CI-STALE-01 | installer-lane items | out of this audit's code scope (build.ps1/.nsi) — **unverified here** |
| G-1..G-11 capability gaps | per API-DELTA | functional, not safety — not re-verified |
| D-48c/D-51/D-72/D-78/D-88/D-104/D-DRV-6 divergences | registered | registered parity items, not defects — present as documented |

## 10. Recursion sites (self-recursive, input-depth-controlled)

| Function | Cite | Depth control | Attacker-controlled depth? |
|---|---|---|---|
| `refs::walk_table` | cr_refs_engine.cpp:268 (self-call :300) | `depth > MAX_TABLE_DEPTH(32)` + node_count > 4096 + visited-set :271 | yes (MSB chain) — **bounded** |
| `refs::attr_walk_runs` | :433 (self :455) | depth ≤ 32, hops ≤ 64 :434-436 | yes — **bounded** |
| `ext4::walk_extent_node` | cr_ext4_engine.cpp:372 (self :421) | EXTENT_DEPTH_LIMIT 5 :97/:382 + global node budget :375-378 | yes — **bounded** |
| `fat::recurse_dir`/`walk_dir_entries` | cr_fat_quick.cpp:120/:126 (mutual) | MAX_DEPTH 64 :79/:125 + visited_dirs cycle set :127-132 | yes — **bounded** |
| `exfat` dentry walk | cr_exfat_quick.cpp (same shape as fat) | depth cap + visited set (spot-read) | yes — bounded |
| `btrfs populate` lambda | cr_btrfs_engine.cpp:1048 (self :1159) | DIR_DEPTH_LIMIT 64 :131/:1049 | yes — **bounded** |
| `zfs::nv_parse_stream` | cr_zfs_engine.cpp:408 (self for embedded nvlists) | depth > 6 :410 | yes — **bounded** |
| `zfs::plan_gang_pieces` | :1037 (self :1053) | depth > 4 :1040 | yes — **bounded** |
| `reghive::collect_list_offsets` | cr_reghive.cpp:228 (self :252) | depth > 8, out ≥ 4096 :229 | yes ('ri' indirect lists) — **bounded** |
| `cr_fscheck::fat_audit_dir` | cr_fscheck.cpp:302 (self :424) | **ancestor-cycle guard only** (:403-407) — **no numeric depth cap found**; depth bounded by distinct ancestor clusters, each recursion pushes one cluster — worst case O(distinct clusters) stack frames with a `std::vector<u32> ancestors` **copied by value per call** (:303) — deep-but-distinct chains (huge corrupt FAT) could stack-pressure; flag for review |
| `query::walk` | cr_query.cpp:173 (self :191) | **no depth cap** — walks the result tree; tree depth is engine-built (NTFS path depth unbounded in principle); reparent cycle guard (cr_node.cpp:394-396) prevents loop trees, but a pathological deep tree recurses per level | tree-built depth — **indirect**; review candidate |
| `folderfilter::prune_children`/`count_subtree` | cr_folderfilter.cpp:95/:111 | no cap — same tree-depth exposure as above |
| `lost::count_tree` | cr_candidates.cpp:725 | no cap — tree walk, same class |
| `carve::carve_slice` | cr_carve.cpp:1494 | grep hit is a comment ("re-scanned"), **not** self-recursive — excluded |
| image `open_image_impl` | cr_image.cpp:1473 (self via open_vmdk parent :643/:891) | depth > 32 :1476/:524/:786 | yes (VMDK parent chains) — **bounded** |

**Bottom line for §10:** every on-disk structure walker that recurses has an
explicit numeric depth/cycle bound except the tree-consumer walkers
(query/folderfilter/count_tree — bounded only by engine-built tree shape) and
`fat_audit_dir` (cycle-guarded, no numeric cap, by-value ancestor vectors).

## Priority pointers for the defect-hunting campaign

1. BF-32 emitter UAF window (events.rs:341 + scan.rs:348-362) — the one known
   open memory-safety race.
2. Poisoned-lock emitter death (events.rs:346/:586) — stability, no restart.
3. `fat_audit_dir` unbounded-depth recursion + per-call vector copies
   (cr_fscheck.cpp:302-303) — hostile-FAT stack pressure.
4. Dead `PoolCancelToken` at cr_mftwalk.cpp:2371 — cancel-latency gap on the
   remnant fan-out (N-017 class).
5. `WinDestIo` has no destructor (cr_destio.h:17-56) — no close-all safety net
   behind the 15+ manual `io_.close()` calls in cr_safewriter.cpp.
6. Cache-mutex-holds-fill (cr_cached.cpp:31/:61) — known perf/serialization
   point (not corruption).
