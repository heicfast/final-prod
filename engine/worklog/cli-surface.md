# CLI surface — Session-84 Phase 0 (baseline e8dcaba8)

Audited: `src/app/rrecov.cpp` (3,922 lines, read in full), `src/app/CMakeLists.txt`,
`src/CMakeLists.txt`, root `CMakeLists.txt`, `tests/unit/*` (CLI-exercising tests),
`.github/workflows/checks.yml`, `.github/scripts/Run-Benchmark.ps1`, `tools/win_syntax_check.sh`,
and the CLI-relevant docs (`docs/API.md`, `docs/PARAMETERS.md`, `docs/DRIVER-REQUIRED.md`,
`docs/ABI-CONTRACT.md`, `docs/CORE-API-CURRENT.md`, `docs/PARITY-DELTAS.md`,
`diff-to-fix/modules/cli.md`, `README.md`).

Entry point: `int wmain(int argc, wchar_t** argv)` — rrecov.cpp:3814. Dispatcher: plain
`if (cmd == L"<verb>")` chain at rrecov.cpp:3882–3919. Bare `rrecov` (argc < 2) prints the
usage block (rrecov.cpp:3819–3877) and exits 0 (rrecov.cpp:3878). Unknown verb →
`unknown command: %s`, exit 1 (rrecov.cpp:3920–3921).

**Zero `cr_ui_*` call sites.** rrecov.cpp links only `cr::core` + `cr::win`
(src/app/CMakeLists.txt:2) and deliberately calls the core directly — the
"no-second-singleton rule" (rrecov.cpp:859–867, 1414–1417, 2229–2233: linking cr_ui.dll
would give the process two engine copies; a `--user-signs` install would land in the copy
the exe's scan never reads). The verb tables below therefore record the **core/win layer**
calls each subcommand makes, plus the cr_ui wrapper that mirrors the same surface.

---

## Subcommands

13 dispatched verbs (rrecov.cpp:3882–3919). Aliases: `apply` accepts `--in` as an alias
for `--session` (rrecov.cpp:3197); `scan --deep` is a deprecated alias of `--whole-disk`
that prints a `[note]` (rrecov.cpp:1501–1509). No other aliases.

| Verb | Purpose | Flags (code) | Core/win calls (file:line) | Exit codes |
|---|---|---|---|---|
| `list` | device census: disks + volumes (+ optional TRIM chip) | `--trim` | `win::enumerate_disks` (813), `win::enumerate_volumes` (819, **rc dropped**), `win::health::query_trim_state` (825), `query::display_size_log` (831/843) | 0 ok / 1 enum-failed (815) or unknown flag (3891) |
| `list-carve-formats` | the two-layer carve registry projection | — | `carve::registry_size`/`registry()` (868/885), `carve::rawsigns::kFormats/kHeaderSigns` (895/903) | 0 ok / 1 empty registry (873) |
| `sessions` | `*.crsf` directory listing with light per-file summary (og LastScanRecordView) | `--dir D` (default `.`), `--json` | `orchestrator::ScanController::read_summary` (1198), `session::Reader` (1197), `text::wide_to_utf8`, `recovery::report::escape_json` (1226) | 0 ok (1255/1300) / 1 unknown option (1136), missing dir (1161) |
| `scan` | the scan pipeline over disk/volume/image; save/resume/checkpoint | 26 flags (table below) | `win::query_driver_status` (501/567), `win::open_rda_partition` (577), `win::open_disk` (1731), `win::volume_extent` (746), `PartitionSlice::create` (753), `DecryptingDevice::create` (769), `storage::open_image`/`classify_extension`/`classify_lba0` (461/471/481), `win::bitlocker::status/try_auto_unlock/unlock_with_dictionary` (671/707/682), `carve::usersigns::parse/install_set/active_count` (1382–1429), `dict::parse` (645), `win::mountpoints2_volume_history` (1644), `table::parse_ldm_database` (1722), `orchestrator::build_read_pipe` (1743), `orchestrator::ScanController::{resume,run,save_session}` (1817/1879/1980), `session::Writer` (1977), `session::auto_session_name/last_scan_retention_delete_previous/last_scan_info_write` (2016/2033/2043), `win::open_disk_rw` (369, scan-repair writer) | 0 ok / 1 failure / 3 cancelled (2077) / 4 driver (1312) / **5 device disconnected (2094)** |
| `tree` | load a session, print / export the result tree | `--in`, `--filter`, `--deleted-only`, `--csv`, `--json` | `orchestrator::ScanController::load_session` (2397), `query::run` (2192, `--json`), `text::wide_to_utf8` (2367) | 0 ok (2406/2409) / 1 unknown option (2383), missing `--in` (2387), open fail (2392), load fail (2401) |
| `archive-list` | browse-inside inventory over one session node (M-116 smoke surface) | `--in`, `--disk`, `--image`, `--node`, `--json` | `require_driver` unless `--image` (2237); `orchestrator::ScanController::load_session` (2292), `preview::PreviewReader` + `preview::extract_metadata` (2323/2324) | 0 ok (2347/2356) / 1 unknown option (2271), usage (2274/2275), open fail (2280/2288/2290), load fail (2296), node not found (2316), not previewable (2322) / 4 driver (2239) |
| `recover` | session → files (safe writer) | `--in`, `--to`, `--disk`, `--image`, `--filter`, `--report`, `--deleted-only` | `require_driver` unless `--image` (2421); `load_session` (2488), `win::volume_extent(s)`/`destination_volume_letter` (2500/2546/2548), `orchestrator::build_read_pipe` (2527), `recovery::SafeWriter::{enqueue,process,report_items,report_totals,queue}` (2592–2693), `recovery::report::build_report` (2691), `recovery::failure_cause_text` (2653) | 0 ok / **2 partial** (errors‖hash_mismatch, 2712) / 1 failure (2456/2460/2471/2492/2637/2702/2708) / 4 driver (2423) |
| `health` | SMART + NVMe + TRIM read-only report | `--disk` | `CreateFileW` \\\\.\\PhysicalDriveN (2738), `win::health::read_smart` (2750), `read_nvme_health` (2787), `looks_like_ssd` (2816), `query_trim_state` (2818), `cr::health::*` grading (2762–2797) | 0 some source / **3 every source unavailable (2826)** / 1 unknown option (2729), missing `--disk` (2733), open fail (2745) |
| `fix` | RawFixer analyze / dry-run / consent apply / undo replay | `--disk`, `--part`, `--apply`, `--undo`, `--undo-apply` | `require_driver` (3520); `win::open_disk` (3615), `win::open_disk_rw` (3588/3714), `table::parse_disk_table` (3630), `PartitionSlice::create` (3642), `repair::analyze/rebase_plan/execute/undo` (3670/3696/3723/3603), `repair::UndoLog::deserialize` (3581) | 0 ok/refused-clean/dry-run (3610/3680/3685/3710/3746) / 1 unknown option (3554), missing `--disk` (3562), undo-log open fail (3570) / **2 open/plan/execute/undo failure** (3584/3594/3607/3618/3633/3637/3645/3673/3720/3726) / 4 driver (3522) |
| `apply` | lost-partition MBR entry apply (+ boot-chain repair) | `--disk`, `--session` (alias `--in`), `--candidate`, `--apply`, `--undo`, `--boot-fix`, `--boot-fix-bcd` | `require_driver` (3171); `win::open_disk` (3237), `win::open_disk_apply_rw` (3371), `load_session_summary` (3255), `apply::plan_apply/execute_apply/inflight_probe/inflight_begin/inflight_end` (3304/3385/3353/3382/3388), `win::refresh_disk_properties` (3399), `win::bootfix::bcd_file_plan/bcd_file_execute/nvram_plan/nvram_execute` (3450/3457/3487/3490), `repair::UndoLog` (3381/3409) | 0 ok/refused/dry-run (3313/3334/3512) / 1 unknown option (3224), usage (3228), session open fail (3248), no candidates (3270), bad candidate (3298) / **2 open/load/plan/execute failure** (3240/3264/3307/3377/3392) / 4 driver (3173) |
| `image` | flat raw disk-image producer + `.sha256` sidecar (S-040) | `--disk`, `--out` — **`--create` is claimed by help+docs but NOT parsed (see Findings F1)** | `require_driver` (2854); `win::create_disk_image` (2910), `crypto::to_hex` (2911), `g_cancel_token` | 0 ok (2916) / 1 unknown option (2875), usage (2879), non-flat extension (2887) / **3 cancelled (2922, keeps `<out>.partial`)** / **2 failed (2927, keeps partial)** / 4 driver (2856) |
| `bench` | transport/device microbenchmarks | `--disk`, `--mode seq\|rand\|open\|read\|info`, `--mb`, `--block-kb`, `--iterations`, `--seed`, `--json` | `require_driver` (2935); `win::open_disk` (2990/3054), `dev->read_at`/`geometry()` | 0 ok (3020/3044/3066/3095/3126) / 1 unknown option (2980), missing `--disk` (2984), unknown mode (3130) / **2 open/read failure or zero hits** (2995/3028/3044/3074/3095/3126) / 4 driver (2937) |
| `driver` | the driver-status surface (docs/DRIVER-REQUIRED.md) | `--service NAME` (default `win::kDefaultDriverService`), `--json` | `win::query_driver_status` (3771), `win::driver_state/_error/_state_name`, `kDriverWireMajorRequired` (3776–3788) | 0 ready (3789/3804) / 1 query failed (3774) or unknown option (3768) / **4 not ready (3789 / 3808–3809)** |

---

## Flags/options

Types/defaults as parsed in code. `get_opt` = value option; `has_flag`/`wcscmp` = boolean
flag. All names are long-only, `--`-prefixed; there are **no short flags** anywhere.

### `list` (wmain:3886–3893)
- `--trim` — flag; adds the per-row TRIM chip (`TRIM:ON`/`TRIM:OFF`/`TRIM ?`) + header chip (rrecov.cpp:824–840)

### `list-carve-formats` — none

### `sessions` (rrecov.cpp:1121–1137)
- `--dir D` — path; default `.` (the current directory)
- `--json` — flag; frozen-schema row array

### `scan` (rrecov.cpp:1307–1615)
| Flag | Type / default | Notes |
|---|---|---|
| `--disk N` | u32 (`wcstoul`) | target; opens RDA view (1447) |
| `--volume C:` | drive letter (first wchar) | target (1453) |
| `--image file` | path | target; `.img/.vmdk/.vhd/.vhdx/.raw/.dmg` (1480) |
| `--bitlocker-fvek hex` | hex string, 64 or 128 chars (AES-128/256-XTS) | pre-pass anywhere on line (1327); M-033 |
| `--bitlocker-dict [file]` | optional value (next arg if not `-`-leading) | consent flag; shipped 228-entry table + optional user dict (1333–1342, N-064) |
| `--user-signs file` | path | pre-pass (1351); replaces the default set entirely; invalid → honest exit 1 |
| `--whole-disk` | flag | lost-partition discovery (1488) |
| `--deep` | flag | **deprecated alias** of `--whole-disk`; prints `[note]` (1501) |
| `--quick-only` | flag | `run_deep=false, run_carve=false` (1511) |
| `--no-carve` | flag | `run_carve=false` (1516) |
| `--threads N` | u32; default `cfg.threads`=0 (auto) | (1520) |
| `--filter-folder DIR` | repeatable UTF-8 path prefix | M-073 scan-time folder filter (1524) |
| `--out file.crsf` | path | session + auto-checkpoints (1535) |
| `--autosave-dir D` | path → `cfg.autosave_dir` (truncated at field size) | W1c/R-52 completion auto-save (1537) |
| `--label s` | string → `cfg.volume_label` | M-079 explicit override (1547) |
| `--resume file.crsf` | path | resume; without `--out` updates in place (1556) |
| `--log file` | path | opens the log sink (1558/1628) |
| `--pause-stdin` | flag | N-178 keyboard pause surface; no-op without a console (1564) |
| `--filter s` | substring → UTF-8 | display/recover filter (1579) |
| `--deleted-only` | flag | print filter (1583) |
| `--csv` / `--json` | flags | tree print mode (1587/1591) |
| `--repair` / `--repair-apply` | flags; `--repair-apply` implies `--repair` | S-008 pre-scan repair (1595–1603) |
| `--repair-undo file` | path; default `rrecov-scan-repair-disk<N>.undo` (1928) | (1604) |
| `--check` | flag | M-041 read-only damage audit (1606) |

### `tree` (rrecov.cpp:2362–2384)
`--in file.crsf`; `--filter s`; `--deleted-only`; `--csv`; `--json`

### `archive-list` (rrecov.cpp:2246–2272)
`--in file.crsf`; `--disk N`; `--image file`; `--node ID` (u64, `wcstoull`); `--json`

### `recover` (rrecov.cpp:2429–2457)
`--in file.crsf`; `--to DIR`; `--disk N`; `--image file`; `--filter s`; `--report file.json`; `--deleted-only`

### `health` (rrecov.cpp:2722–2734)
`--disk N` (required)

### `fix` (rrecov.cpp:3530–3555)
`--disk N` (required); `--part P` (int; −1 = whole disk); `--apply` (flag); `--undo file`
(default `rrecov-fix-disk<N>.undo`, 3732); `--undo-apply file`

### `apply` (rrecov.cpp:3191–3225)
`--disk N` (required); `--session file` (alias `--in`, 3197); `--candidate N` (1-based;
required in practice — out-of-range exits 1 at 3296); `--apply` (flag); `--undo file`
(default `rrecov-apply-disk<N>.undo`, 3407); `--boot-fix old-lba` (u64); `--boot-fix-bcd path`

### `image` (rrecov.cpp:2863–2876)
`--disk N` (required); `--out file.img|file.raw` (required). **`--create` appears in the
usage text (3858) and header comment (43) but is NOT parsed — `rrecov image --create ...`
prints `unknown image option: --create` and exits 1.**

### `bench` (rrecov.cpp:2948–2981)
`--disk N` (required); `--mode` default `seq` (seq/rand/open/read/info); `--mb` default
256 (clamped 1..65536); `--block-kb` default 1024 (clamped 4..16384); `--iterations`
default 1000 (open clamped ≤10000, rand ≤100000); `--seed` default 4242; `--json`

### `driver` (rrecov.cpp:3757–3769)
`--service NAME` (default `win::kDefaultDriverService` = "RDA"); `--json`

**Totals: 13 subcommands; 70 distinct flag forms** (73 tokens counting `--in` alias and
reuse across verbs; 69 excluding the deprecated `--deep`). No short options. No positional
arguments anywhere (everything is flag-driven).

---

## Exit codes

Full mapping, as returned from `wmain`'s dispatch chain (rrecov.cpp:3882–3921):

| Code | Meaning | Where (file:line) |
|---|---|---|
| **0** | success; also: bare `rrecov` usage (3878); `list`/`list-carve-formats`/`sessions` success (849, 920, 1255, 1300); `scan` success (2171); `tree` (2406, 2409); `archive-list` (2347, 2356); `health` some-source (2826); `image` OK (2916); `bench` all modes (3020, 3044, 3066, 3095, 3126); `fix` ok/refused/clean/dry-run/undo-complete (3610, 3680, 3685, 3710, 3746); `apply` ok/refused/dry-run (3313, 3334, 3512); `driver` ready (3789, 3804) | see left |
| **1** | generic failure: unknown command (3921); unknown option per verb (1136, 1613, 2271, 2382, 2455, 2728, 2874, 2979, 3130, 3223, 3553, 3767, 3891); usage/missing-required (1617, 2274, 2386, 2459, 2732, 2878, 3227, 3561, 2984); open/enum/load failures (815, 873, 1161, 1406, 1426, 1434, 1449, 1457, 1483, 1777, 1803, 1821, 2279, 2288, 2290, 2296, 2316, 2322, 2392, 2400, 2470, 2483, 2485, 2492, 2745, 3248, 3270, 3298, 3570); scan run failure ≠ cancelled/disconnected (2114); session/report save failure (2155, 2701, 2707); `driver` query failed (3774) | see left |
| **2** | `recover` partial — item errors or hash mismatch (2712); `apply`/`fix` open/plan/execute/undo failure (3240, 3264, 3307, 3377, 3392, 3584, 3594, 3607, 3618, 3633, 3637, 3645, 3673, 3720, 3726); `bench` open/read failure or zero hits (2995, 3028, 3044, 3074, 3095, 3126); `image` failed — partial kept (2927) | see left |
| **3** | `scan` cancelled — session saved when `--out` given (2077); `image` cancelled — `<out>.partial` kept (2922); `health` every source unavailable (2826) | see left |
| **4** | driver prerequisite not met — returned by `require_driver()` (504, 547) and propagated by `scan` (1312), `archive-list` (2239), `recover` (2423), `fix` (3522), `apply` (3173), `image` (2856), `bench` (2937), `driver --json` not-ready (3789) and `driver` human not-ready (3808–3809) | see left |
| **5** | `scan`: target device DISCONNECTED mid-scan (`ERR_DEVICE_DISCONNECTED`, N-125) — distinct from failure(1) and cancel(3); reattach-then-resume contract (2084–2094) | rrecov.cpp:2094 |

Documented mapping (header docstring rrecov.cpp:49–60) covers 0/1/2/3/4 + image 3/2 —
**it omits exit 5** (see Findings F7). `docs/API.md:58–66` (0/1/2/3/4) and
`docs/DRIVER-REQUIRED.md:110–111` (0/1/2/3/4) likewise omit 5; only
`docs/PARITY-DELTAS.md:3267` (D-121) records it.

---

## Output formats

### Human-readable line prefixes (stdout `wprintf` unless noted)

- Usage header: `rrecov - core_recovery CLI` (3820)
- `[image] …` — open/classify (467, 486); `[transport] …` (578, 585); `[driver] …` (571);
  `[volume] …` (780); `[bitlocker] …` (631–774); `[user-signs] …` (1387–1428);
  `[repair] …` / `[repair][advisory] …` (346–358, 1919–1943); `[check] …` (1952–1961);
  `[resume] …` / `[resume] WARNING: …` (1830–1857, 1910–1913); `[pause] …` (223, 229, 1874);
  `[scan] …` (2121–2127); `[dest] …` / `[F12] WARNING: …` (2502–2560); `[apply] …` /
  `[apply] WARNING: …` (3148–3403); `[boot-fix] …` (3163–3510); `[autosave] FAILED: …` (2048);
  `[note] …` (1506, 1624)
- Driver refusals (the ONLY stderr output in the file): `rrecov: …` lines via
  `fwprintf(stderr, …)` — rrecov.cpp:503, 512–517, 519–523, 526–530, 532–537, 539–544,
  546. Everything else prints to stdout.
- Error/diagnostic lines WITHOUT a bracket prefix: `enumeration failed` (814),
  `cannot enumerate session directory %s` (1160), `unknown <verb> option: %s`
  (1135/1613/2270/2382/2455/2728/2874/2979/3223/3553/3767/3890), `unknown command: %s`
  (3920), `missing value for %s` (791), `scan needs --disk N, --volume C:, or --image
  file…` (1617), `cannot open session %s` (1803/2279/2391/2470/3247), `resume failed: %hs
  (0x%08x)` (1821), `session load failed: %hs (0x%08x)` (2295/2400/2491), `session save
  failed` (1992), `scan failed: %hs (0x%08x)` (2112), `scan REFUSED: …` (2100),
  `target device DISCONNECTED mid-scan …` (2085), `scan cancelled at %.1f%%` (2057),
  `scan complete: %llu file(s) (%.2f GB) …` (2144), `queued %zu item(s)…` (2610),
  `recovered: %llu done, %llu refused, …` (2639), `recovery failed: %hs (0x%08x)` (2636),
  `report: %s` (2710), `cannot open disk %u (rc=%08x)…` (2992/3239/3617),
  `plan failed: rc=%08x %hs` (3306), `execute FAILED: …` (3390/3725), `undo FAILED …` (3606),
  `REFUSED: %hs` (3310/3677), `dry-run only …` (3333/3709), `undo log saved: …` (3414/3740),
  `image complete/FAILED/CANCELLED …` (2913–2926), `session saved[: (cancelled)] …`
  (2044–2046, 2060, 2156), tree rows `name (size) [deleted|risk] [carved|lost]` (1020),
  root headers `== name` (1029).
- `--csv` row: `"path",size,origin,deleted,confidence,first-extent-offset` (1006)

### JSON output paths
- `scan --json` (2129–2137): `{"files","deleted","carved","lost_partitions","bytes_scanned","bytes_unreadable","bytes_found","error_ranges","probe_unreadable","elapsed_ms"}`
- `sessions --json` (1216–1254): row array `{"path","valid","error","fs_family","stage_done_mask","configured_mask","files_found","deleted_found","carved_found","bytes_scanned","elapsed_ms","save_unix","label"}` (FROZEN schema)
- `tree --json` → `print_tree_json` (2187–2223) via `query::run` (G-12, FROZEN): `{"total":N,"rows":[{"path","name","size","modified","origin","deleted","confidence","category"}]}`
- `archive-list --json` (2325–2346): `{"kind","format","truncated","entries":[{"name","size","compressed","crc_present"}]}`
- `bench --json`: per-mode objects — info (3011), read (3031), open (3059), seq (3087), rand (3118)
- `driver --json` (3780–3788): `{"service","state","present","loaded","started","authorized","healthy","major","minor","required_major","flags","max_disks","ioctl_base","last_gle","error","error_code"}`
- `recover --report file.json` (2666–2711): G-09 machine-readable report via `recovery::report::build_report` (schema: docs/recovery-report-schema.md); emitted even on partial/failed passes

### Progress lines
- scan/recover `ProgressDisplay` (139–172): `\r  %5.1f%%  (%lld s)  remaining: %ls   `
  with the M-126 ETA render (`XmYs`, placeholder literal `3m00s`); success-only completion
  `\r  100.0%%  done          \n` (170) — a failed/cancelled run never prints "done"
  (AUDIT-ARCH F-B-15)
- `image` (2896–2905): `image: %llu / %llu bytes (%llu%%)` once per ~10% (throttled)
- pause transitions (223, 229): `[pause] scan paused - press 'r' or Enter to resume…` /
  `[pause] scan resumed`; banner at 1874

### Log format (`--log`, scan verb only — `log_open` at 1628)
- sink lines: `[I] / [W] / [E] <core log line>` (246–252)
- raw `log_line` entries (271–280): `rrecov scan: threads=%u quick=%d deep=%d carve=%d
  whole_disk=%d repair=%d/%d` (1629), `scan start` (1878), `scan end rc=0x%08x files=…
  elapsed_ms=%llu errors=%zu` (1899), `  read-error range: @%llu +%llu (attempts %u)`
  (1904), `ttfr_ms=%llu first_files=%llu stage=%u` (298, OQ-S4-3 TTFR probe),
  `rrecov scan: register channel armed: %zu remembered placement(s)` (1646),
  `ldm cross-disk registry: %zu disk(s)` (1749)

---

## Startup behavior

1. **Ctrl-C handler installed first** — `SetConsoleCtrlHandler(ctrl_c_handler, TRUE)`
   (rrecov.cpp:3818); handler cancels the shared `g_cancel_token` (PausableToken,
   rrecov.cpp:181–189). Cooperative cancel: run() returns `ERR_CANCELLED`, the session
   saves when `--out`/`--autosave-dir` known, exit 3.
2. **Bare invocation** (`argc < 2`): usage block → exit 0 (3819–3878).
3. **There is NO apply-journal probe at process startup.** The in-flight journal
   (N-145/CRIFJ01) is probed only inside `cmd_apply`, only after `--candidate` selection
   and only when `--apply` consent was given: `apply::inflight_probe` at rrecov.cpp:3353
   over `<session-path>.inflight` (or `rrecov-apply-inflight.jrn` when the session path is
   empty, 3345–3346), warning `[apply] WARNING: an earlier apply on disk %u was
   interrupted…` (3356–3360). The undo path journals `<undo-log>.inflight` (3598–3604).
   The journal filename is **not** `rrecov-apply.ifj` and the probe is **not** "at every
   startup" — see Findings F4.
4. **Driver gate first** in every driver-requiring verb — `require_driver()` (499–548)
   runs before any other output; refusal (exit 4, stderr, names the exact ERR_DRIVER_*
   class) is the whole response. `scan`/`recover`/`archive-list` skip the gate when the
   argv tail carries `--image` (`argv_has_image_target`, 553–558; classification (b)).
   `image`/`bench`/`fix`/`apply` always gate.
5. **Logging init** — only the scan verb: `log_open(log_path…)` at 1628 (swaps the core
   log sink); `log_close()` at 1964. No other verb logs.
6. **user-signs default load** — scan verb only: executable-adjacent
   `userdata-defaults.json` (M-065; 1364–1401); corrupt/absent degrades loudly, never
   aborts.
7. **scan-only extras at setup**: register channel (M-118 HKCU MountPoints2 walk, whole-disk
   only, 1640–1650), identity card fill (SESS-2, 1651), volume-label fill (M-079/N-133,
   1652/420–452), LDM cross-disk member enumeration + `build_read_pipe` wrapping
   (1720–1757), TTFR event probe armed before controller construction (1795–1797),
   atomic checkpoint sink `CliCkSink` staged at `<out>.tmp` (1771–1786, A-013).

---

## CLI tests

The exe is Windows-only (`if(WIN32) add_subdirectory(app)`, src/CMakeLists.txt:10–14;
option `CR_BUILD_APP` default ON, root CMakeLists.txt:40), so the Linux suite pins the
CLI by **source inspection** (`cr_test::read_repo_file("src/app/rrecov.cpp")`) plus a
**parse gate**; the behavioral CLI tests live in the Windows CI workflow.

### Linux unit-suite pins (source-truth)
| Test (file:line) | What it pins |
|---|---|
| `CrUi.SessionListCliAndAbiOrderAgree` (test_cr_ui.cpp:1637) | `sessions` rows re-sorted by the ABI's time key (save time DESC, path tie-break); the stale path-only order must not return |
| `CrUi.CliHelpListsEveryDispatchedVerb` (test_cr_ui.cpp:3935) | every `if (cmd == L"<verb>")` after wmain appears in the `-h` usage block AND the header verb table; and no usage line for an undispatched verb (N-094). NOTE: checks only rrecov.cpp's two internal sites — does NOT enforce docs/API.md or CORE-API-CURRENT.md (see Findings F8) |
| `CrUi.CliScanFillsAutoSaveVolumeLabel` (test_cr_ui.cpp:4363) | `fill_volume_label(target, cfg)` call site, `--volume` letter capture, `GetVolumeInformationW`, `L"--label"` parse + `[--label s]` usage text, cr_scan.h comment truth (M-079) |
| `DestArming.ProductionWiringCallsSpansApi` (test_cr_ui.cpp:4441) | CLI recover path uses `destination_volume_letter` + `volume_extents` + `set_destination_spans` (S-054/N-013 regression fence) |
| `CrUi.CliResumeSinkNeverTruncatesTheSessionSource` (test_cr_ui.cpp:4784) | A-013: CliCkSink + all four checkpoint callbacks wired; staging open on the TMP path; no truncating open of `effective_out`; resume reads "rb"; error paths remove the tmp |
| `CrUi.PauseStdinReaderMapsKeysOntoTheToken` (test_cr_ui.cpp:4968) | N-178: `--pause-stdin` key mapping ('p'/SPACE→pause(), 'r'/Enter→resume()), 200 ms poll (never blocking), terminates on token cancel, flag parses, `[--pause-stdin]` string present (satisfied by the header docstring line 15, not the -h block) |
| `CrUi.RecoverAllSkipsJunkFlaggedNodes` (test_cr_ui.cpp:2120) | M-071: the junk gate class the CLI's `plan()` walk carries (pinned at the ABI on the same walk) |
| `Enumerate.TrimChipOptInShipsInAbiAndCli` (test_enumerate_win.cpp:~78–109) | `rrecov list [--trim]` usage text, `L"--trim"` parse, `TRIM:ON`/`TRIM ?` render (M-076) |
| `Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe` (test_orchestrator.cpp:3304) | CLI LDM member resolver wraps members through `build_read_pipe(…, 8 MiB, 256 KiB)`; raw `dev.get()` handoff forbidden (M-148) |
| `Writer.RecoveryEntryPointsConstructThroughBuildReadPipe` (test_report.cpp:845) | CLI recover verb constructs the writer's source through `build_read_pipe` (M-136) |
| `UserSigns.RegistrationRoundTripProceedsToScan` (test_carve.cpp:9197) | the `--user-signs` CLI round-trip in miniature: parse + install succeeds, count surfaces (S-039 — pre-fix the CLI aborted exit 1 on this input) |
| `UserSigns.DefaultSetShipsAndLoads` (test_carve.cpp:9291) | shipped `data/userdata-defaults.json` (111 entries/190 signs) parses under the caps — the file staged beside rrecov.exe (M-065) |
| `test_config_display.cpp` (whole file) | the CLI-relevant display layers: N-132 summary-family/type-display data, M-125 progress remap curve / stall cap / percent dispatch / monotonic publish guard / linear core permille, M-126 ETA model/EMA/time-budget/placeholder/countdown |
| `Text.WideToUtf8*` (test_text.cpp:228, 246) | N-009c: the `--filter` wide-argv → UTF-8 seam (never '?') |
| `win_syntax_check` (tests/CMakeLists.txt:214–217 → tools/win_syntax_check.sh:68–80) | `g++ -fsyntax-only` over `src/app/*.cpp` (incl. rrecov.cpp) against the win-stub SDK — structural parse gate; tests/win-stub/windows.h:116/278/570 stub the Ctrl-C, userdata-defaults and console-input (N-178) surfaces rrecov consumes |

### Windows CI behavioral probes (.github/workflows/checks.yml)
- **Driverless refusal-contract probes** (checks.yml:266–310): bare `rrecov` exit 0;
  `list` exit 0; `list-carve-formats` exit 0 (output checked); `driver --json` exit 4
  with `"state":"absent"`; `scan --disk 0` exit 4 with the refusal and nothing past the
  gate (docs/CI-CURRENT.md:48–53)
- **CLI junk-filter contract (M-071 + A-013 + M-079)** (checks.yml:313–433): `scan
  --image` tree print never lists DESKTOP.INI/$USNJRNL/SVI/TRACKER.TXT while controls
  (DOC.TXT/KEEP.ME/_ELFILE.TXT) print; `tree --in` round-trip; `scan --image --resume`
  (A-013: session survives non-empty); `recover --in --image --to` never writes the junk
  set; `scan --autosave-dir` name derivation + `--label CUSTOM` override
- **Benchmark lane** (.github/scripts/Run-Benchmark.ps1:184–196): `image --create
  --disk … --out …` exit-0 + `.sha256` sidecar + hash equality + `scan --image` of the
  product — **this invocation currently cannot pass** (see Findings F1)

---

## Docs vs code

| # | Doc claim | Code reality | Doc file:line | Code file:line |
|---|---|---|---|---|
| 1 | `rrecov image --create --disk N --out file.img` is the create-image invocation | `cmd_image` parses only `--disk`/`--out`; `--create` → `unknown image option: --create`, exit 1. Git-proven: the parser was born in 13fe7258 without `--create`; the flag has NEVER been parsed | rrecov.cpp header comment :43; **-h usage :3858**; docs/DRIVER-REQUIRED.md:38; docs/PLAN-API-EXPANSION.md:64; docs/REFERENCE-API-GAP.md:336,435; docs/CI-SCENARIOS.md:30; docs/PLAN-CI-REBUILD.md:118; docs/PARITY-DELTAS.md:4276; .github/scripts/Run-Benchmark.ps1:184 | rrecov.cpp:2863–2876 (parser), 2874–2875 (rejection) |
| 2 | API.md §1 verb list | Lists 11 verbs; code dispatches 13 — `archive-list` and `image` missing from the doc block | docs/API.md:27–48 | rrecov.cpp:3904–3915 |
| 3 | Exit-code table 0/1/2/3/4 | Code also returns **5** (scan, device disconnected, N-125) — absent from API.md, DRIVER-REQUIRED.md, PARAMETERS.md and the rrecov.cpp header docstring; only PARITY-DELTAS.md:3267 records it | docs/API.md:58–66; docs/DRIVER-REQUIRED.md:110–111; rrecov.cpp:49–60 | rrecov.cpp:2084–2094 |
| 4 | In-flight journal is `rrecov-apply.ifj` beside the undo log, "probed at every startup regardless of CWD" + "the every-startup warning naming the recovery path" | Journal is `<session>.inflight` (or `rrecov-apply-inflight.jrn`), probed ONLY inside `cmd_apply` when `--apply` is given; nothing at wmain/startup; undo path uses `<undo-log>.inflight` | docs/PARITY-DELTAS.md:4226–4228 (D-152); diff-to-fix/CROSS-REF-MATRIX.md:21 | rrecov.cpp:3345–3362 (probe site), 3598–3604 (undo journal), 3814–3878 (wmain — no probe) |
| 5 | `scan` flag table includes `--part P` ("limit to partition P (with --disk)") | `--part` is a **fix** verb flag only; scan rejects it (`unknown scan option: --part`, exit 1) | docs/PARAMETERS.md:18 | rrecov.cpp:3536–3539 (fix parse); 1613–1614 (scan rejection) |
| 6 | `bench` flags include `[--no-rda]` | `--no-rda` is REMOVED (PLAN-DRIVER-ONLY R10); bench rejects it, and the code comment says so explicitly | docs/PARAMETERS.md:50–51 | rrecov.cpp:2930–2933, 2979–2980 |
| 7 | PARAMETERS.md §1 documents the CLI verbs: scan, tree/recover/health/fix/bench, `apply`/`undo`, list, list-carve-formats, info | `undo` is not a verb (it is `fix --undo-apply`); `sessions`, `driver`, `image`, `archive-list` verbs are undocumented in PARAMETERS.md; API.md's scan line also omits `--label` and `--pause-stdin` (both parsed + in the header docstring) | docs/PARAMETERS.md:36–53; docs/API.md:30–38 | rrecov.cpp:3544–3547 (fix --undo-apply); 3898–3915 (sessions/image/archive-list dispatch); 1547, 1564 (--label/--pause-stdin parse) |
| 8 | "The four locked doc sites (header comment, docs/API.md §1, CORE-API-CURRENT §c, the -h usage block) … are enforced by CrUi.CliHelpListsEveryDispatchedVerb" | That test reads only src/app/rrecov.cpp (its two internal sites); docs/API.md and CORE-API-CURRENT.md are NOT enforced — and API.md has in fact drifted (item 2). doc_truth_gate.py counts exports/error-codes/event-types but NOT verbs | diff-to-fix/modules/cli.md:35–37 | tests/unit/test_cr_ui.cpp:3943 (single read_repo_file of rrecov.cpp); tools/doc_truth_gate.py:314–344 |
| 9 | "`rrecov --list-carve-formats` prints the whole table" | The verb is `rrecov list-carve-formats` (no leading dash); a `--list-carve-formats` argv[1] hits `unknown command`, exit 1 | docs/ABI-CONTRACT.md:506 | rrecov.cpp:3896–3897, 3920–3921 |
| 10 | cli.md: "wmain dispatches exactly 11 commands" | 13 verbs dispatch (archive-list M-116 + image S-040 landed after the recount; the same file's later addendum says "CLI verb 13") | diff-to-fix/modules/cli.md:29–32 | rrecov.cpp:3882–3919; docs/CORE-API-CURRENT.md:16–18 (correct at 13) |
| 11 | `-h` driver note: "scan/recover/fix/apply/bench (disk targets) refuse with exit 4" | `image` ALWAYS requires the driver (no `--image` bypass) and `archive-list --disk` is also gated — both omitted from the help text's gated-verb list; DRIVER-REQUIRED.md §1(a) likewise omits archive-list from the classification table | rrecov.cpp:3874–3877 (help text); docs/DRIVER-REQUIRED.md:27–38,129–130 | rrecov.cpp:2851–2857 (image gate), 2235–2241 (archive-list gate) |
| 12 | `-h` usage block shows scan flags | The -h block omits 6 live scan flags: `--autosave-dir`, `--label`, `--pause-stdin`, `--user-signs`, `--bitlocker-dict`, `--check` (all in the header docstring :12–18 and parsed) | rrecov.cpp:3819–3877 (usage block) | rrecov.cpp:1537, 1547, 1564, 1473, 1466, 1606 (parse sites) |

---

## Findings

**Phantom flag / broken documented invocation**
- **F1 (SEVERE)** — `image --create` is a phantom flag. The `-h` usage line
  (rrecov.cpp:3858), the header comment (rrecov.cpp:43), docs/DRIVER-REQUIRED.md:38,
  docs/PLAN-API-EXPANSION.md:64, docs/REFERENCE-API-GAP.md:336/435, docs/CI-SCENARIOS.md:30,
  docs/PLAN-CI-REBUILD.md:118, docs/PARITY-DELTAS.md:4276 and the CI benchmark script
  (.github/scripts/Run-Benchmark.ps1:184, which asserts exit 0 at :188) all invoke
  `rrecov image --create …`, but cmd_image's parser (rrecov.cpp:2863–2876) accepts only
  `--disk`/`--out` and rejects `--create` with exit 1. The documented form has never
  worked (parser born without the flag in 13fe7258). Either parse-and-ignore `--create`
  or fix the 9+ doc/help/CI sites.

**User-facing text naming nonexistent surface**
- **F2 (MINOR)** — scan's disconnect hint prints `` `rrecov resume %ls` `` — there is no
  `resume` verb; the real command is `rrecov scan --resume <file>`
  (rrecov.cpp:2088–2089 vs dispatcher 3882–3919; cf. the correct hint at 2061).

**Dropped / ignored error codes**
- **F3 (MINOR)** — `win::enumerate_volumes(vols)` return code dropped (rrecov.cpp:819):
  volume-enumeration failure prints "0 volume(s)" with exit 0 while disk-enumeration
  failure is exit 1 (FB-K01, rrecov.cpp:813–816). Inconsistent truth posture.
- **F3a (MINOR)** — `(void)session::last_scan_retention_delete_previous(...)` (rrecov.cpp:2033)
  and `(void)session::last_scan_info_write(...)` (rrecov.cpp:2043): M-154 retention/pointer
  return codes dropped silently (a hostile sidecar refusal never surfaces).
- **F3b (MINOR)** — undo path drops `apply::inflight_begin` rc without the warning the
  apply path prints (rrecov.cpp:3602 vs 3382–3384 — inconsistent journal-failure posture);
  `inflight_end` dropped at 3388/3604 (by design: op ended).
- **F3c (INFO)** — LDM member open failures silently `continue` (rrecov.cpp:1731–1733,
  deliberate: "a member that will not open simply does not resolve"); `fill_identity_card`
  quiet-on-failure is documented (rrecov.cpp:392–395).

**Journal claim mismatch (startup probe)**
- **F4 (MINOR/DOC)** — the N-145/D-152 claims (`rrecov-apply.ifj` beside the undo log,
  "probed at every startup regardless of CWD", "every-startup warning") vs code reality
  (`<session>.inflight` / `rrecov-apply-inflight.jrn`, probe only inside `apply --apply`,
  rrecov.cpp:3345–3362). The task brief's "apply-journal probe at startup
  (rrecov-apply.ifj)" repeats the doc claim; the code has no such startup probe and no
  such filename.

**Exit-code documentation gaps**
- **F5 (MINOR/DOC)** — exit 5 undocumented in API.md:58–66, DRIVER-REQUIRED.md:110–111,
  PARAMETERS.md and the header docstring (rrecov.cpp:49–60); only PARITY-DELTAS.md:3267.
- **F6 (INFO)** — exit-code semantics per verb are otherwise faithful to the docstring:
  sessions unreadable rows are per-row data (1263–1265), list-carve-formats empty→1 (873),
  health all-unavailable→3 (2826), recover partial→2 (2712), apply/fix→2 family, bench
  zero-hits→2, image cancel/failed→3/2 with `.partial` kept (2918–2927).

**Help-text gaps**
- **F7 (MINOR)** — the `-h` usage block omits 6 live scan flags (`--autosave-dir`,
  `--label`, `--pause-stdin`, `--user-signs`, `--bitlocker-dict`, `--check`) and the two
  additional driver-gated verbs (`image`, `archive-list`) (rrecov.cpp:3819–3877). The
  header docstring (rrecov.cpp:12–22) is complete for scan flags; `PauseStdinReaderMaps
  KeysOntoTheToken`'s usage-block assertion is actually satisfied by the docstring, not
  the -h block (test_cr_ui.cpp:4998–5000).
- **F8 (MINOR/DOC)** — the claimed "four-site verb lock" is only two sites in practice:
  `CrUi.CliHelpListsEveryDispatchedVerb` enforces rrecov.cpp's internal pair; docs/API.md
  (11 verbs listed vs 13, missing archive-list + image) and CORE-API-CURRENT.md are
  unguarded — API.md has drifted exactly as the unenforced site would.

**Stale docs**
- **F9 (MINOR/DOC)** — docs/PARAMETERS.md:18 (`--part` under scan), :50–51 (`--no-rda`
  under bench), :48 (`undo` as a verb), :53 (no sessions/driver/image/archive-list rows);
  docs/ABI-CONTRACT.md:506 (`rrecov --list-carve-formats` leading-dash form);
  diff-to-fix/modules/cli.md:30 ("exactly 11 commands").

**Unwired / hidden surfaces (no defect, recorded for completeness)**
- **F10 (INFO)** — `apply --in` alias is undocumented in both the header docstring
  (rrecov.cpp:31) and the -h block (3867); only `--session` is shown.
- **F11 (INFO)** — no cr_ui_* call sites exist by design (no-second-singleton rule,
  rrecov.cpp:859–867); the cr_ui ABI mirrors the same surfaces (cr_ui_session_list,
  cr_ui_carve_format_count/info, cr_ui_register_user_signs, cr_ui_image_create) but is
  never invoked from the CLI process.

**Build**
- Target: `add_executable(rrecov rrecov.cpp)` linking `cr::core cr::win`
  (src/app/CMakeLists.txt:1–2) + vcpkg DLL deployment backstop (:5); Windows-only
  (src/CMakeLists.txt:10–14; root CMakeLists.txt:40 `CR_BUILD_APP` ON). No linkage to
  cr_ui — consistent with the zero-call-site observation.

---

## Session-90 coherence addendum (re-stamped at the pre-v0.5.3 close)

Deltas against the Session-84 record (13 verbs, e8dcaba8):

- **The verb count is 14, not 13.** The M-075/M-078 window (d10ae333,
  fcd3ba10 — both post-e8dcaba8) added `unlock` (dispatch
  src/app/rrecov.cpp:4037 -> cmd_unlock :3545; exits 0 ok / 1
  unknown-option+usage / 2 refused) and `fix --chkdsk` (:3621, the
  M-075 posture flag; exits 2 on refusal, 3 on errors-remain). The
  Session-84 verb table otherwise holds verbatim (list,
  list-carve-formats, sessions, scan, tree, archive-list, recover,
  health, fix, apply, image, bench, driver).
- **The `image --create` selector parses** since 32441353 (the five-site
  wiring repair: the usage header, -h, docs, and parser all agree now).
- Exit-code surface: 0-5 as recorded + the unlock/chkdsk rows above.
- The release-notes' CLI section (RELEASE-NOTES.md, v0.5.3 entry) was
  reconciled to this count this session.

— Session-90 coherence lane. End of addendum.

### Post-window correction: re-verified at the v0.6.0 cut - the 14-verb
count HOLDS at the merged HEAD (the dispatch recounted first-hand; the
racing window's M-116 archive-list and the unlock/fix --chkdsk surface
were already in this addendum's count).
