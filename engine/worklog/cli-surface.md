# CLI surface — re-verified at HEAD 3b203fe1 (main)

Audited: `src/app/rrecov.cpp` (4,065 lines, read in full), `src/app/CMakeLists.txt`,
`src/CMakeLists.txt`, root `CMakeLists.txt`, `tests/unit/*` (CLI-exercising tests),
`.github/workflows/checks.yml`, `.github/scripts/Run-Benchmark.ps1`, `tools/win_syntax_check.sh`,
`tools/validate_report.py`, `docs/recovery-report-schema.md`, and the CLI-relevant docs
(`docs/API.md`, `docs/PARAMETERS.md`, `docs/DRIVER-REQUIRED.md`, `docs/ABI-CONTRACT.md`,
`docs/CORE-API-CURRENT.md`, `docs/CI-CURRENT.md`, `diff-to-fix/modules/cli.md`, `README.md`).
Session-84 Phase 0 baseline e8dcaba8; the Session-90 addendum and the v0.6.0 re-stamp
(both preserved at the bottom) are now folded into the body — the body below is the
current-tree truth.

Entry point: `int wmain(int argc, wchar_t** argv)` — rrecov.cpp:3940. Dispatcher: plain
`if (cmd == L"<verb>")` chain at rrecov.cpp:4023–4062. Bare `rrecov` (argc < 2) prints the
usage block (rrecov.cpp:3945–4018) and exits 0 (rrecov.cpp:4019). Unknown verb →
`unknown command: %s`, exit 1 (rrecov.cpp:4063–4064).

**Zero `cr_ui_*` call sites.** rrecov.cpp links only `cr::core` + `cr::win`
(src/app/CMakeLists.txt:2) and deliberately calls the core directly — the
"no-second-singleton rule" (rrecov.cpp:886–893, 1440–1443, 2256–2258: linking cr_ui.dll
would give the process two engine copies; a `--user-signs` install would land in the copy
the exe's scan never reads). The verb tables below therefore record the **core/win layer**
calls each subcommand makes, plus the cr_ui wrapper that mirrors the same surface.

---

## Subcommands

14 dispatched verbs (rrecov.cpp:4023–4062). Aliases: `apply` accepts `--in` as an alias
for `--session` (rrecov.cpp:3238); `scan --deep` is a deprecated alias of `--whole-disk`
that prints a `[note]` (rrecov.cpp:1527–1535). No other aliases.

| Verb | Purpose | Flags (code) | Core/win calls (file:line) | Exit codes |
|---|---|---|---|---|
| `list` | device census: disks + volumes (+ optional TRIM chip) | `--trim` | `win::enumerate_disks` (839), `win::enumerate_volumes` (845, **rc dropped**), `win::health::query_trim_state` (851), `query::display_size_log` (857/870) | 0 ok (875) / 1 enum-failed (841) or unknown flag (4032) |
| `list-carve-formats` | the two-layer carve registry projection | — | `carve::registry_size` (894), `carve::rawsigns::kFormatCount` (895), `carve::registry()` (911), `carve::rawsigns::kFormats/kHeaderSigns` (921/929) | 0 ok (946) / 1 empty registry (898–901) |
| `sessions` | `*.crsf` directory listing with light per-file summary (og LastScanRecordView) | `--dir D` (default `.`), `--json` | `orchestrator::ScanController::read_summary` (1224), `session::Reader` (1223), `text::wide_to_utf8` (1252), `recovery::report::escape_json` (1252/1276) | 0 ok (1281, 1326) / 1 unknown option (1162), missing dir (1187) |
| `scan` | the scan pipeline over disk/volume/image; save/resume/checkpoint | 26 flags (table below) | `win::query_driver_status` (527/593), `win::open_rda_partition` (603), `win::open_disk` (1757), `win::volume_extent` (772), `PartitionSlice::create` (779), `DecryptingDevice::create` (795), `storage::open_image`/`classify_extension`/`classify_lba0` (487/497/507), `win::bitlocker::status/try_auto_unlock/unlock_with_dictionary` (697/733/708+741), `carve::usersigns::parse/install_set/active_count` (1408–1455), `dict::parse` (671), `win::mountpoints2_volume_history` (1670), `table::parse_ldm_database` (1748), `orchestrator::build_read_pipe` (1768), `orchestrator::ScanController::{resume,run,save_session}` (1843/1905/2006), `session::Writer` (2003), `session::auto_session_name/last_scan_retention_delete_previous/last_scan_info_write` (2042/2059/2069), `win::open_disk_rw` (395, scan-repair writer) | 0 ok / 1 failure / 3 cancelled (2104) / 4 driver (1340) / **5 device disconnected (2121)** |
| `tree` | load a session, print / export the result tree | `--in`, `--filter`, `--deleted-only`, `--csv`, `--json` | `orchestrator::ScanController::load_session` (2424), `query::run` (2219 via `print_tree_json`, `--json`), `text::wide_to_utf8` (2394) | 0 ok (2433/2436) / 1 unknown option (2410), missing `--in` (2414), open fail (2419), load fail (2428) |
| `archive-list` | browse-inside inventory over one session node (M-116 smoke surface) | `--in`, `--disk`, `--image`, `--node`, `--json` | `require_driver` unless `--image` (2264–2268); `orchestrator::ScanController::load_session` (2319), `preview::PreviewReader` + `preview::extract_metadata` (2350/2351) | 0 ok (2374/2383) / 1 unknown option (2298), usage (2302), open fail (2307/2315/2317), load fail (2323), node not found (2343), not previewable (2348) / 4 driver (2267) |
| `recover` | session → files (safe writer) | `--in`, `--to`, `--disk`, `--image`, `--filter`, `--report`, `--deleted-only` | `require_driver` unless `--image` (2447–2451); `load_session` (2515), `win::volume_extent(s)`/`destination_volume_letter` (2527/2575/2573), `orchestrator::build_read_pipe` (2553), `recovery::SafeWriter::{enqueue,process,report_items,report_totals,queue}` (2619–2724), `recovery::report::build_report` (2718), `recovery::failure_cause_text` (2680) | 0 ok / **2 partial** (errors‖hash_mismatch, 2739) / 1 failure (2498/2511/2513/2519/2664/2729/2735) / 4 driver (2450) |
| `health` | SMART + NVMe + TRIM read-only report | `--disk` | `CreateFileW` \\\\.\\PhysicalDriveN (2764–2766), `win::health::read_smart` (2777), `read_nvme_health` (2814), `looks_like_ssd` (2843), `query_trim_state` (2845), `cr::health::*` grading (2789/2798/2824) | 0 some source / **3 every source unavailable (2853)** / 1 unknown option (2756), missing `--disk` (2760), open fail (2772) |
| `unlock` | M-078 consented BitLocker turn-OFF (decrypt, never bypass) | `--turn-off C:` | `cr::win::bitlocker::turn_off` (3586) — **no driver gate** (volume-letter op, classification (b)) | 0 accepted (3594) / 1 unknown option (3572), usage (3576), bad letter (3581) / **2 refused** (3591) |
| `fix` | RawFixer analyze / dry-run / consent apply / undo replay + chkdsk rung | `--disk`, `--part`, `--apply`, `--undo`, `--undo-apply`, `--chkdsk` | `require_driver` (3602); `cr::win::fsck::fsck_volume` + `cr::check::fsck_launch::chkdsk_exit_means_errors_remain` (3658/3674, M-075); `win::open_disk` (3741), `win::open_disk_rw` (3714/3840), `table::parse_disk_table` (3756), `PartitionSlice::create` (3768), `repair::analyze/rebase_plan/execute/undo` (3796/3822/3849/3729), `repair::UndoLog::deserialize` (3707) | 0 ok/refused-clean/dry-run (3806/3811/3836/3736/3872) / 1 unknown option (3641), missing `--disk` (3688), bad `--chkdsk` letter (3654), undo-log open fail (3696) / **2 open/plan/execute/undo failure** (3710/3720/3733/3744/3759/3763/3772/3799/3846/3852; chkdsk launch/elevation 3662/3666) / **3 chkdsk errors-remain (3677)** / 4 driver (3604) |
| `apply` | lost-partition MBR entry apply (+ boot-chain repair) | `--disk`, `--session` (alias `--in`), `--candidate`, `--apply`, `--undo`, `--boot-fix`, `--boot-fix-bcd` | `require_driver` (3212); `win::open_disk` (3278), `win::open_disk_apply_rw` (3411), `load_session_summary` (3296), `apply::plan_apply/execute_apply/inflight_probe/inflight_begin/inflight_end` (3345/3425/3394/3422/3428), `win::refresh_disk_properties` (3439), `win::bootfix::bcd_file_plan/bcd_file_execute/nvram_plan/nvram_execute` (3490/3497/3527/3530), `repair::UndoLog` (3421/3449) | 0 ok/refused/dry-run (3354/3375/3552) / 1 unknown option (3265), usage (3269), session open fail (3289), no candidates (3311), bad candidate (3339) / **2 open/load/plan/execute failure** (3281/3305/3348/3417/3432) / 4 driver (3214) |
| `image` | flat raw disk-image producer + `.sha256` sidecar (S-040) | `--create` (valueless mode selector, **now parsed**), `--disk`, `--out` | `require_driver` (2881); `win::create_disk_image` (2951), `crypto::to_hex` (2952), `g_cancel_token` (2951) | 0 ok (2957) / 1 unknown option (2916), usage (2920), non-flat extension (2928) / **3 cancelled (2963, keeps `<out>.partial`)** / **2 failed (2968, keeps partial)** / 4 driver (2883) |
| `bench` | transport/device microbenchmarks | `--disk`, `--mode seq\|rand\|open\|read\|info`, `--mb`, `--block-kb`, `--iterations`, `--seed`, `--json` | `require_driver` (2976); `win::open_disk` (3031/3095), `dev->read_at`/`geometry()` | 0 ok (3061/3085/3107/3136/3167) / 1 unknown option (3021), missing `--disk` (3025), unknown mode (3171) / **2 open/read failure or zero hits** (3036/3069/3085/3136/3167) / 4 driver (2978) |
| `driver` | the driver-status surface (docs/DRIVER-REQUIRED.md) | `--service NAME` (default `win::kDefaultDriverService`), `--json` | `win::query_driver_status` (3897), `win::driver_state/_error/_state_name` (3902–3933), `kDriverWireMajorRequired` (3913/3925) | 0 ready (3915/3930) / 1 query failed (3900) or unknown option (3894) / **4 not ready (3915 / 3934–3935)** |

Dispatch order (4023–4062): list, list-carve-formats, sessions, scan, tree, archive-list,
recover, health, **unlock**, fix, apply, image, bench, driver.

---

## Flags/options

Types/defaults as parsed in code. `get_opt` = value option (rrecov.cpp:812–824);
`has_flag`/`wcscmp` = boolean flag (826–831). All names are long-only, `--`-prefixed; there
are **no short flags** anywhere.

### `list` (wmain:4023–4035)
- `--trim` — flag; adds the per-row TRIM chip (`TRIM:ON`/`TRIM:OFF`) + header chip
  (`[TRIM ON]`/`[TRIM OFF]`/`[TRIM ?]`) (rrecov.cpp:852–866)

### `list-carve-formats` — none

### `sessions` (rrecov.cpp:1147–1163)
- `--dir D` — path; default `.` (the current directory)
- `--json` — flag; frozen-schema row array

### `scan` (rrecov.cpp:1471–1641)
| Flag | Type / default | Notes |
|---|---|---|
| `--disk N` | u32 (`wcstoul`) | target; opens RDA view (1474) |
| `--volume C:` | drive letter (first wchar) | target (1480–1486) |
| `--image file` | path | target; `.img/.vmdk/.vhd/.vhdx/.raw/.dmg` (1506) |
| `--bitlocker-fvek hex` | hex string, 64 or 128 chars (AES-128/256-XTS) | pre-pass anywhere on line (1352–1358); M-033 |
| `--bitlocker-dict [file]` | optional value (next arg if not `-`-leading) | consent flag; shipped 228-entry table + optional user dict (1359–1368, N-064) |
| `--user-signs file` | path | pre-pass (1374–1381); replaces the default set entirely; invalid → honest exit 1 (1432/1452) |
| `--whole-disk` | flag | lost-partition discovery (1514) |
| `--deep` | flag | **deprecated alias** of `--whole-disk`; prints `[note]` (1527–1535) |
| `--quick-only` | flag | `run_deep=false, run_carve=false` (1537) |
| `--no-carve` | flag | `run_carve=false` (1542) |
| `--threads N` | u32; default `cfg.threads`=0 (auto) | (1546) |
| `--filter-folder DIR` | repeatable UTF-8 path prefix | M-073 scan-time folder filter (1550) |
| `--out file.crsf` | path | session + auto-checkpoints (1561) |
| `--autosave-dir D` | path → `cfg.autosave_dir` (truncated at field size) | W1c/R-52 completion auto-save (1563) |
| `--label s` | string → `cfg.volume_label` | M-079 explicit override (1573) |
| `--resume file.crsf` | path | resume; without `--out` updates in place (1582) |
| `--log file` | path | opens the log sink (1584/1654) |
| `--pause-stdin` | flag | N-178 keyboard pause surface; no-op without a console (1590) |
| `--filter s` | substring → UTF-8 | display/recover filter (1605) |
| `--deleted-only` | flag | print filter (1609) |
| `--csv` / `--json` | flags | tree print mode (1613/1617) |
| `--repair` / `--repair-apply` | flags; `--repair-apply` implies `--repair` | S-008 pre-scan repair (1621–1629) |
| `--repair-undo file` | path; default `rrecov-scan-repair-disk<N>.undo` (1954) | (1630) |
| `--check` | flag | M-041 read-only damage audit (1632) |

### `tree` (rrecov.cpp:2389–2411)
`--in file.crsf`; `--filter s`; `--deleted-only`; `--csv`; `--json`

### `archive-list` (rrecov.cpp:2273–2299)
`--in file.crsf`; `--disk N`; `--image file`; `--node ID` (u64, `wcstoull`); `--json`

### `recover` (rrecov.cpp:2456–2484)
`--in file.crsf`; `--to DIR`; `--disk N`; `--image file`; `--filter s`; `--report file.json`; `--deleted-only`

### `health` (rrecov.cpp:2749–2761)
`--disk N` (required)

### `unlock` (rrecov.cpp:3563–3582)
`--turn-off C:` (required; value must start with a letter A–Z/a–z, 3578–3582)

### `fix` (rrecov.cpp:3613–3642)
`--disk N` (required unless `--chkdsk`); `--part P` (int; −1 = whole disk); `--apply` (flag);
`--undo file` (default `rrecov-fix-disk<N>.undo`, 3858); `--undo-apply file`;
`--chkdsk C:` (M-075 volume-letter escalation; runs chkdsk /f, 900 s bound, 3650–3680)

### `apply` (rrecov.cpp:3232–3266)
`--disk N` (required); `--session file` (alias `--in`, 3238); `--candidate N` (1-based;
required in practice — out-of-range exits 1 at 3339); `--apply` (flag); `--undo file`
(default `rrecov-apply-disk<N>.undo`, 3447); `--boot-fix old-lba` (u64); `--boot-fix-bcd path`

### `image` (rrecov.cpp:2890–2917)
`--create` (valueless create-mode selector — parses since 32441353, 2891–2904);
`--disk N` (required); `--out file.img|file.raw` (required; `.img`/`.raw` only — D-147, 2922–2928)

### `bench` (rrecov.cpp:2989–3022)
`--disk N` (required); `--mode` default `seq` (seq/rand/open/read/info); `--mb` default
256 (clamped 1..65536, 3112); `--block-kb` default 1024 (clamped 4..16384, 3111);
`--iterations` default 1000 (open clamped ≤10000 at 3091, rand ≤100000 at 3142);
`--seed` default 4242; `--json`

### `driver` (rrecov.cpp:3883–3895)
`--service NAME` (default `win::kDefaultDriverService` = "RDA"); `--json`

**Totals: 14 subcommands; 73 per-verb flag forms** (74 tokens counting the `--in` alias;
72 excluding the deprecated `--deep`). No short options. No positional
arguments anywhere (everything is flag-driven).

---

## Exit codes

Full mapping, as returned from `wmain`'s dispatch chain (rrecov.cpp:4023–4064):

| Code | Meaning | Where (file:line) |
|---|---|---|
| **0** | success; also: bare `rrecov` usage (4019); `list`/`list-carve-formats`/`sessions` success (875, 946, 1281, 1326); `scan` success (2198); `tree` (2433, 2436); `archive-list` (2374, 2383); `health` some-source (2853); `unlock` accepted (3594); `image` OK (2957); `bench` all modes (3061, 3085, 3107, 3136, 3167); `fix` ok/refused/clean/dry-run/undo-complete/chkdsk-timeout (3806, 3811, 3836, 3736, 3872, 3672); `apply` ok/refused/dry-run (3354, 3375, 3552); `driver` ready (3915, 3930) | see left |
| **1** | generic failure: unknown command (4064); unknown option per verb (list 4032, sessions 1162, scan 1640, archive-list 2298, tree 2410, recover 2483, health 2756, image 2916, bench 3021, unlock 3572, fix 3641, apply 3265, driver 3894); usage/missing-required (scan 1645, archive-list 2302, tree 2414, recover 2487, health 2760, image 2920, bench 3025, apply 3269, fix 3688 + `--chkdsk` letter 3654, unlock 3576/3581); open/enum/load failures (841, 901, 1187, 1432, 1452, 1460, 1475, 1481/1483, 1508/1510, 1804, 1836, 1854, 2307, 2315, 2317, 2323, 2343, 2348, 2419, 2428, 2498, 2511/2513, 2519, 2772, 3289, 3311, 3339, 3696); scan run failure ≠ cancelled/disconnected (2141); session/report save failure (2182, 2729, 2735); `driver` query failed (3900) | see left |
| **2** | `recover` partial — item errors or hash mismatch (2739); `apply`/`fix` open/plan/execute/undo failure (3281, 3305, 3348, 3417, 3432, 3710, 3720, 3733, 3744, 3759, 3763, 3772, 3799, 3846, 3852); `fix --chkdsk` launch/elevation refusal (3662, 3666); `bench` open/read failure or zero hits (3036, 3069, 3085, 3136, 3167); `image` failed — partial kept (2968); `unlock` turn-off refused (3591) | see left |
| **3** | `scan` cancelled — session saved when `--out` given (2104); `image` cancelled — `<out>.partial` kept (2963); `health` every source unavailable (2853); `fix --chkdsk` errors-remain (3677) | see left |
| **4** | driver prerequisite not met — returned by `require_driver()` (530, 573) and propagated by `scan` (1340), `archive-list` (2267), `recover` (2450), `fix` (3604), `apply` (3214), `image` (2883), `bench` (2978), `driver --json` not-ready (3915) and `driver` human not-ready (3934–3935). `unlock`, `sessions`, `tree`, `list` never gate | see left |
| **5** | `scan`: target device DISCONNECTED mid-scan (`ERR_DEVICE_DISCONNECTED`, N-125) — distinct from failure(1) and cancel(3); reattach-then-resume contract (2111–2121) | rrecov.cpp:2121 |

Documented mapping (header docstring rrecov.cpp:55–69) now covers 0/1/2/3/4/5 including
the chkdsk 2/3 classes. `docs/API.md:67–72` covers 0–5 (fixed since Session-84);
`docs/DRIVER-REQUIRED.md:110–111` still enumerates only 0/1/2/3 beside the 4 (see F5).

---

## Output formats

### Human-readable line prefixes (stdout `wprintf` unless noted)

- Usage header: `rrecov - core_recovery CLI` (3947)
- `[image] …` — open/classify (493, 512); `[transport] …` (604, 611); `[driver] …` (597);
  `[volume] …` (806); `[bitlocker] …` (693–800); `[user-signs] …` (1413–1455);
  `[repair] …` / `[repair][advisory] …` (372–384, 1945–1968); `[check] …` (1978–1987);
  `[resume] …` / `[resume] WARNING: …` (1856–1883, 1936–1940); `[pause] …` (249, 255, 1900);
  `[scan] …` (2148, 2151); `[dest] …` / `[F12] WARNING: …` (2529–2587, 2659); `[apply] …` /
  `[apply] WARNING: …` (3189–3400); `[boot-fix] …` (3204–3549); `[autosave] FAILED: …` (2074);
  `[note] …` (1532, 1650, 1882)
- Driver refusals (the ONLY stderr output in the file): `rrecov: …` lines via
  `fwprintf(stderr, …)` — rrecov.cpp:529, 538–542, 545–549, 552–555, 558–562, 565–569, 572.
  Everything else prints to stdout.
- Error/diagnostic lines WITHOUT a bracket prefix: `enumeration failed` (840),
  `cannot enumerate session directory %s` (1186), `unknown <verb> option[/flag]: %s`
  (1161, 1639, 2297, 2409, 2482, 2755, 2915, 3020, 3571, 3640, 3264, 3893, 4031),
  `unknown command: %s` (4063), `missing value for %s` (817), `scan needs --disk N,
  --volume C:, or --image file…` (1643), `cannot open session %s` (1829, 2306, 2418,
  2497, 3288), `resume failed: %hs (0x%08x)` (1847), `session load failed: %hs…` (2322,
  2427, 2518, 3304), `session save failed` (2018), `scan failed: %hs (0x%08x)` (2139),
  `scan REFUSED: …` (2127), `target device DISCONNECTED mid-scan …` (2112),
  `scan cancelled at %.1f%%` (2084), `scan complete: %llu file(s) (%.2f GB) …` (2171),
  `queued %zu item(s)…` (2637), `recovered: %llu done, %llu refused, …` (2666),
  `recovery failed: %hs (0x%08x)` (2663), `report: %s` (2737), `cannot open disk %u
  (rc=%08x)…` (3033/3280/3743), `plan failed: rc=%08x %hs` (3347), `execute FAILED: …`
  (3430, 3851), `undo FAILED …` (3732), `REFUSED: %hs` (3351, 3803), `dry-run only …`
  (3374, 3835), `undo log saved: …` (3454, 3866), `image complete/FAILED/CANCELLED …`
  (2954/2965/2960), `session saved[: (cancelled)] …` (2087, 2183), `turning BitLocker
  off on %c: …` / `turn-off refused (rc=%08x %hs) …` / `turn-off request accepted on %c:`
  (3583/3588/3593), `chkdsk %c: exit code %d` (3673) + the chkdsk guidance lines (3661,
  3669, 3675), tree rows `name (size) [deleted|risk] [carved|lost]` (1046), root headers
  `== name` (1055).
- `--csv` row: `"path",size,origin,deleted,confidence,first-extent-offset` (1032)

### JSON output paths
- `scan --json` (2156–2164): `{"files","deleted","carved","lost_partitions","bytes_scanned","bytes_unreadable","bytes_found","error_ranges","probe_unreadable","elapsed_ms"}`
- `sessions --json` (1245–1280): row array `{"path","valid","error","fs_family","stage_done_mask","configured_mask","files_found","deleted_found","carved_found","bytes_scanned","elapsed_ms","save_unix","label"}` (FROZEN schema)
- `tree --json` → `print_tree_json` (2214–2250) via `query::run` (G-12, FROZEN): `{"total":N,"rows":[{"path","name","size","modified","origin","deleted","confidence","category"}]}`
- `archive-list --json` (2353–2374): `{"kind","format","truncated","entries":[{"name","size","compressed","crc_present"}]}`
- `bench --json`: per-mode objects — info (3052–3055), read (3072–3083), open (3100–3102), seq (3128–3130), rand (3159–3161)
- `driver --json` (3906–3914): `{"service","state","present","loaded","started","authorized","healthy","major","minor","required_major","flags","max_disks","ioctl_base","last_gle","error","error_code"}`
- `recover --report file.json` (2693–2738): G-09 machine-readable report via `recovery::report::build_report` (schema: docs/recovery-report-schema.md); emitted even on partial/failed passes

### The G-09 report schema (docs/recovery-report-schema.md + tools/validate_report.py)
Required top-level keys: `schema_version` (=1), `engine_version`, `source`, `destination`,
`elapsed_ms`, `totals`, `items` (schema doc :15–43; validator :40–43). `totals` requires
`queued/done/dirs/refused/errors/hash_mismatch/resumed/salvaged/nothing_to_recover/
pending/bytes` (+`failed_bytes`/`read_error` additive-within-v1 per the doc :81–94; the
validator's TOTALS_KEYS set is the :18–22 list). Each `items[]` row requires
`item_id/path/name/kind/outcome/reason/bytes/duration_ms/sha256/hash_status`
(validator :26–29); outcome ∈ {done, refused, error, hash_mismatch, resumed, pending,
salvaged, nothing_to_recover, read_error-in-doc}; hash_status ∈ {verified, computed,
mismatch, not_computed}; sha256 `^[0-9a-f]{64}$` or empty; dir rows must be
`not_computed`; `len(items) == totals.queued` (validator :65–66). Optional
`scan_analysis` block (S-023 + N-140 carve_formats) when the session had grading context
(schema doc :163–183). Truth rules :185–198 (exit-status mapping stays in the CLI:
errors/mismatch → 2, report still emitted). CI validates the ctest-written sample
`%TEMP%\cr_report_sample.json` (checks.yml:249–259).

### Progress lines
- scan/recover `ProgressDisplay` (166–198): `\r  %5.1f%%  (%lld s)  remaining: %ls   `
  with the M-126 ETA render (`XmYs`, placeholder literal `3m00s`, 189–190); success-only
  completion `\r  100.0%%  done          \n` (196) — a failed/cancelled run never prints
  "done" (AUDIT-ARCH F-B-15)
- `image` (2941): `image: %llu / %llu bytes (%llu%%)` once per ~10% (throttled, 2937–2946)
- pause transitions (249, 255): `[pause] scan paused - press 'r' or Enter to resume…` /
  `[pause] scan resumed`; banner at 1900

### Log format (`--log`, scan verb only — `log_open` at 1654)
- sink lines: `[I] / [W] / [E] <core log line>` (272–278)
- raw `log_line` entries (297–306): `rrecov scan: threads=%u quick=%d deep=%d carve=%d
  whole_disk=%d repair=%d/%d` (1655), `scan start` (1904), `scan end rc=0x%08x files=…
  elapsed_ms=%llu errors=%zu` (1925), `  read-error range: @%llu +%llu (attempts %u)`
  (1930), `ttfr_ms=%llu first_files=%llu stage=%u` (324, OQ-S4-3 TTFR probe),
  `rrecov scan: register channel armed: %zu remembered placement(s)` (1672),
  `ldm cross-disk registry: %zu disk(s)` (1775)

---

## Startup behavior

1. **Ctrl-C handler installed first** — `SetConsoleCtrlHandler(ctrl_c_handler, TRUE)`
   (rrecov.cpp:3944); handler cancels the shared `g_cancel_token` (PausableToken,
   rrecov.cpp:207, handler 209–215). Cooperative cancel: run() returns `ERR_CANCELLED`,
   the session saves when `--out`/`--autosave-dir` known, exit 3.
2. **Bare invocation** (`argc < 2`): usage block → exit 0 (3945–4019).
3. **There is NO apply-journal probe at process startup.** The in-flight journal
   (N-145/CRIFJ01) is probed only inside `cmd_apply`, only after `--candidate` selection
   and only when `--apply` consent was given: `apply::inflight_probe` at rrecov.cpp:3394
   over `<session-path>.inflight` (or `rrecov-apply-inflight.jrn` when the session path is
   empty, 3386–3388), warning `[apply] WARNING: an earlier apply on disk %u was
   interrupted…` (3397–3400). The undo path journals `<undo-log>.inflight` (3724–3730).
   The journal filename is **not** `rrecov-apply.ifj` and the probe is **not** "at every
   startup" — see Findings F4.
4. **Driver gate first** in every driver-requiring verb — `require_driver()` (525–574)
   runs before any other output; refusal (exit 4, stderr, names the exact ERR_DRIVER_*
   class) is the whole response. `scan`/`recover`/`archive-list` skip the gate when the
   argv tail carries `--image` (`argv_has_image_target`, 579–584; classification (b)).
   `image`/`bench`/`fix`/`apply` always gate. `unlock` (volume-letter op) never gates.
5. **Logging init** — only the scan verb: `log_open(log_path…)` at 1654 (swaps the core
   log sink); `log_close()` at 1990. No other verb logs.
6. **user-signs default load** — scan verb only: executable-adjacent
   `userdata-defaults.json` (M-065; 1391–1427); corrupt/absent degrades loudly, never
   aborts.
7. **scan-only extras at setup**: register channel (M-118 HKCU MountPoints2 walk, whole-disk
   only, 1666–1676), identity card fill (SESS-2, 1677), volume-label fill (M-079/N-133,
   1678/446–478), volume-letter config fill (S-048, 1683), LDM cross-disk member
   enumeration + `build_read_pipe` wrapping (1746–1783), TTFR event probe armed before
   controller construction (1821–1823), atomic checkpoint sink `CliCkSink` staged at
   `<out>.tmp` (1797–1812, A-013).

---

## CLI tests

The exe is Windows-only (`if(WIN32) add_subdirectory(win); if(CR_BUILD_APP)
add_subdirectory(app)`, src/CMakeLists.txt:10–14; option `CR_BUILD_APP` default ON,
root CMakeLists.txt:40), so the Linux suite pins the CLI by **source inspection**
(`cr_test::read_repo_text("src/app/rrecov.cpp")`) plus a **parse gate**; the behavioral
CLI tests live in the Windows CI workflow.

### Linux unit-suite pins (source-truth)
| Test (file:line) | What it pins |
|---|---|
| `CrUi.SessionListCliAndAbiOrderAgree` (test_cr_ui.cpp:1657) | `sessions` rows re-sorted by the ABI's time key (save time DESC, path tie-break); the stale path-only order must not return |
| `CrUi.SessionListVolumeLabelFullWidthRoundTrips` (test_cr_ui.cpp:1468) | N-090: the full-width volume label round-trips through the ABI/session card (the N-150 dead `--label` handler removal's pin) |
| `CrUi.CliHelpListsEveryDispatchedVerb` (test_cr_ui.cpp:4130) | every `if (cmd == L"<verb>")` after wmain appears in the `-h` usage block AND the header verb table; and no usage line for an undispatched verb (N-094). NOTE: checks only rrecov.cpp's two internal sites — does NOT enforce docs/API.md or CORE-API-CURRENT.md (see Findings F8) |
| `CrUi.CliHelpListsEveryScanFlag` (test_cr_ui.cpp:5474) | D14 pin: every flag cmd_scan's parse loop accepts appears in the -h scan section (the six missing flags of Session-84 F7, now repaired) |
| `CrUi.CliImageCreateFlagParses` (test_cr_ui.cpp:5435) | the `image --create` five-site surface: `has_flag(..., L"--create")` parser site + usage-header/-h/dispatch/`image needs` refusal |
| `CrUi.CliDisconnectHintNamesRealVerb` (test_cr_ui.cpp:5458) | the exit-5 hint says `rrecov scan --resume %ls`, never a phantom `rrecov resume` verb |
| `CrUi.CliScanFillsAutoSaveVolumeLabel` (test_cr_ui.cpp:4561) | `fill_volume_label(target, cfg)` call site, `--volume` letter capture, `GetVolumeInformationW`, `L"--label"` parse + `[--label s]` usage text, cr_scan.h comment truth (M-079) |
| `DestArming.ProductionWiringCallsSpansApi` (test_cr_ui.cpp:4639) | CLI recover path uses `destination_volume_letter` + `volume_extents` + `set_destination_spans` (S-054/N-013 regression fence) |
| `CrUi.CliResumeSinkNeverTruncatesTheSessionSource` (test_cr_ui.cpp:4982) | A-013: CliCkSink + all four checkpoint callbacks wired; staging open on the TMP path; no truncating open of `effective_out`; resume reads "rb"; error paths remove the tmp |
| `CrUi.PauseStdinReaderMapsKeysOntoTheToken` (test_cr_ui.cpp:5166) | N-178: `--pause-stdin` key mapping ('p'/SPACE→pause(), 'r'/Enter→resume()), 200 ms poll (never blocking), terminates on token cancel, flag parses, `[--pause-stdin]` in the -h block |
| `CrUi.RecoverAllSkipsJunkFlaggedNodes` (test_cr_ui.cpp:2315) | M-071: the junk gate class the CLI's `plan()` walk carries (pinned at the ABI on the same walk) |
| `Enumerate.TrimChipOptInShipsInAbiAndCli` (test_enumerate_win.cpp:73) | `rrecov list [--trim]` usage text, `L"--trim"` parse, `TRIM:ON`/`TRIM ?` render (M-076) |
| `Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe` (test_orchestrator.cpp:3308) | CLI LDM member resolver wraps members through `build_read_pipe(…, 8 MiB, 256 KiB)`; raw `dev.get()` handoff forbidden (M-148) |
| `Writer.RecoveryEntryPointsConstructThroughBuildReadPipe` (test_report.cpp:845) | CLI recover verb constructs the writer's source through `build_read_pipe` (M-136) |
| `UserSigns.RegistrationRoundTripProceedsToScan` (test_carve.cpp:10559) | the `--user-signs` CLI round-trip in miniature: parse + install succeeds, count surfaces (S-039) |
| `UserSigns.DefaultSetShipsAndLoads` (test_carve.cpp:10653) | shipped `data/userdata-defaults.json` parses under the caps — the file staged beside rrecov.exe (M-065) |
| `test_config_display.cpp` (whole file) | the CLI-relevant display layers: N-132 summary-family/type-display data, M-125 progress remap curve / stall cap / percent dispatch / monotonic publish guard, M-126 ETA model/EMA/placeholder |
| `test_fscheck_launch.cpp` (FsckVolume.CommandConstruction/RefusesInvalidVolume/ExitCodeClass, :15/:30/:49) | M-075: the `fix --chkdsk` grammar — exact chkdsk command line per posture, letter validation, the documented exit-code classes |
| `test_driver_status.cpp` (DriverStatus.*, :35–137) | the `driver_state` classifier pins (require_driver + `driver` verb taxonomy: Absent/NotStarted/Unauthorized/VersionMismatch/Errored transitions, fail-fast order, total state-name/error functions) |
| `test_transport_logic.cpp` (Transport.*, :23–78) | M-030 environment-gate/quiesce-chain constants and the N-125 disconnect classification (`DisconnectClassifiedDistinctly`, :65 — the exit-5 class the scan verb surfaces) |
| `Text.WideToUtf8*` (test_text.cpp:228, 246) | N-009c: the `--filter` wide-argv → UTF-8 seam (never '?') |
| `win_syntax_check` (tests/CMakeLists.txt:223–226 → tools/win_syntax_check.sh:58–84) | `g++ -fsyntax-only` over `src/win/*.cpp`, `src/app/*.cpp` (incl. rrecov.cpp), `src/ui/*.cpp` and the Windows-only tests against the win-stub SDK — structural parse gate |

### Windows CI behavioral probes (.github/workflows/checks.yml)
- **Driverless refusal-contract probes** (checks.yml:266–311): bare `rrecov` exit 0
  (274–275); `list` exit 0 (278–279); `list-carve-formats` exit 0, output checked
  (282–283); `driver --json` exit 4 with `"state":"absent"` (286–291); `scan --disk 0`
  exit 4 with the refusal (`driver required`) and nothing past the gate — no
  `starting scan|stage|progress|[transport]` may leak (294–302). Documented at
  docs/CI-CURRENT.md:48–53.
- **CLI junk-filter contract (M-071 + A-013 + M-079)** (checks.yml:323–431): `scan
  --image junk-gate-fat16.raw --out` tree print never lists DESKTOP.INI/$USNJRNL/SVI/
  TRACKER.TXT while controls (DOC.TXT/KEEP.ME/_ELFILE.TXT) print (344–353); `tree --in`
  round-trip (357–368); `scan --image --resume` without `--out` leaves a non-empty
  session + a working tree after (A-013, 377–387); `recover --in --image --to` never
  writes the junk set (391–401); `scan --autosave-dir` name derivation
  (`auto_junk-gate-fat16<stamp>`) + `--label CUSTOM` override (406–426).
- **G-09 report-schema validation** (checks.yml:249–259): `tools/validate_report.py` over
  the `%TEMP%/cr_report_sample.json` written by `Report.EmitsCiSchemaSample` during ctest.
- **Benchmark lane** (.github/scripts/Run-Benchmark.ps1, dispatched by
  test-benchmark.yml): `health --disk` exit 0-or-3 (172–178); `image --create --disk …
  --out …` exit 0 + `.sha256` sidecar + hash equality + exact size + `scan --image` of
  the product exit 0 (189–215) — **this invocation passes now** (F1 resolved); `fix
  --disk` dry-run exit 0 (230); `apply` dry-run exit 0 (657).

---

## Docs vs code

| # | Doc claim | Code reality | Doc file:line | Code file:line |
|---|---|---|---|---|
| 1 | ~~`image --create` phantom~~ **RESOLVED** (32441353): parser accepts `--create` as a valueless selector | `has_flag(argc, argv, i, L"--create")` — accepted, no-op mode selector | rrecov.cpp header :49; -h usage :3993; docs/DRIVER-REQUIRED.md:38; docs/PARAMETERS.md:67; Run-Benchmark.ps1:189 | rrecov.cpp:2891–2904 (parser); pinned by test_cr_ui.cpp:5435 |
| 2 | API.md §1 verb list | Lists 13 verbs; code dispatches **14** — `unlock` missing from the doc block (also missing from PARAMETERS.md §1) | docs/API.md:23–53 | rrecov.cpp:4051–4052 |
| 3 | Exit-code table | **Fixed for API.md + header**: both cover 0–5. Residual: DRIVER-REQUIRED.md §4 enumerates only 0/1/2/3 beside the 4 | docs/API.md:67–72 ✓; rrecov.cpp:55–69 ✓; docs/DRIVER-REQUIRED.md:110–111 (no 5) | rrecov.cpp:2111–2121 |
| 4 | In-flight journal is `rrecov-apply.ifj` beside the undo log, "probed at every startup regardless of CWD" + "the every-startup warning naming the recovery path" | Journal is `<session>.inflight` (or `rrecov-apply-inflight.jrn`), probed ONLY inside `cmd_apply` when `--apply` is given; nothing at wmain/startup; undo path uses `<undo-log>.inflight` | docs/PARITY-DELTAS.md (D-152); diff-to-fix/CROSS-REF-MATRIX.md:21 | rrecov.cpp:3386–3402 (probe site), 3724–3730 (undo journal), 3940–4019 (wmain — no probe) |
| 5 | ~~PARAMETERS.md `--part P` under scan~~ **RESOLVED** — the row is gone; `--part` documented under fix only | scan rejects `--part` (`unknown scan option`, exit 1) | docs/PARAMETERS.md:49 (fix row) | rrecov.cpp:3619–3622 (fix parse); 1639–1640 (scan rejection) |
| 6 | ~~`bench [--no-rda]`~~ **RESOLVED** — documented as REMOVED | bench rejects it; the code comment says so explicitly | docs/PARAMETERS.md:61–62 | rrecov.cpp:2972–2974, 3020–3021 |
| 7 | PARAMETERS.md §1 verb coverage | Mostly repaired (sessions/archive-list/image/driver rows present; `undo`-as-verb confusion gone). **Live drift**: `--deep` documented as "deep stage selector … the flag is the explicit form" — in code it is the deprecated `--whole-disk` alias (sets whole_disk=true + prints `[note]`); `recover` line omits the `--image file` target; API.md's scan line still omits `--label` and `--pause-stdin` (both parsed + in -h); API.md's fix line omits `--chkdsk` | docs/PARAMETERS.md:19 (`--deep`), :44–46 (recover), :49 (fix); docs/API.md:29–37 (scan), :45 (fix) | rrecov.cpp:1527–1535 (--deep); 2467 (recover --image); 1573/1590 (--label/--pause-stdin parse); 3636 (--chkdsk parse) |
| 8 | "The four locked doc sites … are enforced by CrUi.CliHelpListsEveryDispatchedVerb" | That test reads only src/app/rrecov.cpp (its two internal sites); docs/API.md and CORE-API-CURRENT.md are NOT enforced — and both have drifted (API.md missing `unlock`; CORE-API-CURRENT headline says "13 verbs" vs 14). doc_truth_gate.py counts exports/error-codes/event-types but NOT verbs | diff-to-fix/modules/cli.md:35–37; docs/CORE-API-CURRENT.md:16 | tests/unit/test_cr_ui.cpp:4138 (single read_repo_file of rrecov.cpp); tools/doc_truth_gate.py:314–352 |
| 9 | ~~ABI-CONTRACT `rrecov --list-carve-formats` leading-dash form~~ **RESOLVED** — now the no-dash verb form | `rrecov list-carve-formats` dispatch | docs/ABI-CONTRACT.md:505–507 | rrecov.cpp:4037–4038, 4063–4064 |
| 10 | cli.md: "wmain dispatches exactly 11 commands" | 14 verbs dispatch (historical audit-trail doc; its own later addendum corrects to 13, still one short of 14) | diff-to-fix/modules/cli.md:29–32 | rrecov.cpp:4023–4062; docs/CORE-API-CURRENT.md:16 (stale at 13) |
| 11 | `-h` driver note: "scan/recover/fix/apply/bench (disk targets) refuse with exit 4" | `image` ALWAYS requires the driver (no `--image` bypass) and `archive-list --disk` is also gated — both omitted from the help text's gated-verb list; DRIVER-REQUIRED.md §1(a) now carries the image-create row but still omits archive-list from the classification table | rrecov.cpp:4015–4018 (help text); docs/DRIVER-REQUIRED.md:27–38 | rrecov.cpp:2881–2883 (image gate), 2264–2268 (archive-list gate) |
| 12 | ~~-h usage block omits 6 live scan flags~~ **RESOLVED** — all 26 scan flags now appear in the -h block (3959–3981), pinned by `CrUi.CliHelpListsEveryScanFlag` | — | rrecov.cpp:3959–3981 | tests/unit/test_cr_ui.cpp:5474 |

---

## Findings

Status re-verified at HEAD 3b203fe1. F1/F2 (and the flag halves of F5/F7/F9) were
repaired after Session-84 and are now pinned by tests; the rest remain live.

**Phantom flag / broken documented invocation**
- **F1 — RESOLVED.** `image --create` parses since 32441353 as a valueless mode selector
  (rrecov.cpp:2891–2904); the documented invocation `rrecov image --create --disk N
  --out file.img` now works, and the CI benchmark script's exit-0 assertion
  (Run-Benchmark.ps1:189–193) is satisfiable. Pinned by `CrUi.CliImageCreateFlagParses`
  (test_cr_ui.cpp:5435).

**User-facing text naming nonexistent surface**
- **F2 — RESOLVED.** The scan disconnect hint now prints `` `rrecov scan --resume %ls` ``
  (rrecov.cpp:2115); the phantom `rrecov resume` wording is gone. Pinned by
  `CrUi.CliDisconnectHintNamesRealVerb` (test_cr_ui.cpp:5458).

**Dropped / ignored error codes**
- **F3 (MINOR)** — `win::enumerate_volumes(vols)` return code dropped (rrecov.cpp:845):
  volume-enumeration failure prints "0 volume(s)" with exit 0 while disk-enumeration
  failure is exit 1 (FB-K01, rrecov.cpp:839–842). Inconsistent truth posture.
- **F3a (MINOR)** — `(void)session::last_scan_retention_delete_previous(...)` (rrecov.cpp:2059)
  and `(void)session::last_scan_info_write(...)` (rrecov.cpp:2069): M-154 retention/pointer
  return codes dropped silently (a hostile sidecar refusal never surfaces).
- **F3b (MINOR)** — undo path drops `apply::inflight_begin` rc without the warning the
  apply path prints (rrecov.cpp:3728 vs 3422–3424 — inconsistent journal-failure posture);
  `inflight_end` dropped at 3428/3730 (by design: op ended).
- **F3c (INFO)** — LDM member open failures silently `continue` (rrecov.cpp:1757–1759,
  deliberate: "a member that will not open simply does not resolve"); `fill_identity_card`
  quiet-on-failure is documented (rrecov.cpp:416–417).

**Journal claim mismatch (startup probe)**
- **F4 (MINOR/DOC)** — the N-145/D-152 claims (`rrecov-apply.ifj` beside the undo log,
  "probed at every startup regardless of CWD", "every-startup warning") vs code reality
  (`<session>.inflight` / `rrecov-apply-inflight.jrn`, probe only inside `apply --apply`,
  rrecov.cpp:3386–3402). The code has no such startup probe and no such filename.

**Exit-code documentation gaps**
- **F5 — mostly RESOLVED.** Exit 5 is now documented in API.md:72 and the header docstring
  (rrecov.cpp:56–58). Residual: docs/DRIVER-REQUIRED.md:110–111 still enumerates only
  0/1/2/3 beside the 4; PARAMETERS.md has no exit-code table (n/a).
- **F6 (INFO)** — exit-code semantics per verb are otherwise faithful to the docstring:
  sessions unreadable rows are per-row data (1289–1291), list-carve-formats empty→1 (898–901),
  health all-unavailable→3 (2853), recover partial→2 (2739), apply/fix→2 family, bench
  zero-hits→2, image cancel/failed→3/2 with `.partial` kept (2959–2968), unlock refused→2
  (3591), chkdsk errors-remain→3 (3677).

**Help-text gaps**
- **F7 — half RESOLVED.** All scan flags are now in the `-h` block (3959–3981; pinned by
  test_cr_ui.cpp:5474). Still live: the `-h` footer's gated-verb list (4015–4018) omits
  `image` (always gated) and `archive-list --disk` (2264–2268).
- **F8 (MINOR/DOC)** — the claimed "four-site verb lock" is only two sites in practice:
  `CrUi.CliHelpListsEveryDispatchedVerb` enforces rrecov.cpp's internal pair; docs/API.md
  and CORE-API-CURRENT.md are unguarded — API.md is missing `unlock` and
  CORE-API-CURRENT.md:16 says "13 verbs" vs the 14 dispatched (doc_truth_gate.py counts
  exports/error-codes/event-types, not verbs).

**Stale docs**
- **F9 — mostly RESOLVED.** PARAMETERS.md `--part`-under-scan, `--no-rda`, the `undo`
  verb and the missing sessions/driver/image/archive-list rows are all repaired; the
  ABI-CONTRACT.md leading-dash form is fixed. **Still live**: PARAMETERS.md:19 misdocuments
  `--deep` as a deep-stage selector (code: deprecated `--whole-disk` alias, 1527–1535);
  PARAMETERS.md:44–46 recover line omits `--image`; API.md §1 omits the `unlock` verb and
  `fix --chkdsk`; diff-to-fix/modules/cli.md:29–32 still says "exactly 11 commands"
  (historical audit doc).

**Unwired / hidden surfaces (no defect, recorded for completeness)**
- **F10 (INFO)** — `apply --in` alias is undocumented in both the header docstring
  (rrecov.cpp:37) and the -h block (4008); only `--session` is shown.
- **F11 (INFO)** — no cr_ui_* call sites exist by design (no-second-singleton rule,
  rrecov.cpp:886–893); the cr_ui ABI mirrors the same surfaces (cr_ui_session_list,
  cr_ui_carve_format_count/info, cr_ui_register_user_signs, cr_ui_image_create) but is
  never invoked from the CLI process.

**Verb-count doc drift (new at this re-stamp)**
- **F12 (MINOR/DOC)** — `unlock` (dispatch rrecov.cpp:4051–4052, M-078) is absent from
  docs/API.md §1 (23–53) and docs/PARAMETERS.md §1 (40–71), and
  docs/CORE-API-CURRENT.md:16 still headlines "CLI **13 verbs**". The verb IS in the
  header docstring (34–36), the -h block (4005–4007) and REFERENCE-API-GAP.md:331 —
  the internal lock cannot see the doc-side omission (cf. F8).

**Build**
- Target: `add_executable(rrecov rrecov.cpp)` linking `cr::core cr::win`
  (src/app/CMakeLists.txt:1–2) + vcpkg DLL deployment backstop (:5); Windows-only
  (src/CMakeLists.txt:10–14; root CMakeLists.txt:40 `CR_BUILD_APP` ON). No linkage to
  cr_ui — consistent with the zero-call-site observation.

---

## Session-90 coherence addendum (re-stamped at the pre-v0.5.3 close)

Deltas against the Session-84 record (13 verbs, e8dcaba8):

- **The verb count is 14, not 13.** The M-075/M-078 window (d10ae333,
  fcd3ba10 — both post-e8dcaba8) added `unlock` and `fix --chkdsk`. The
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

### Re-stamp at HEAD 3b203fe1 (this session): the 14-verb count holds
again (dispatch recounted at rrecov.cpp:4023–4062). Body line numbers
re-based to the current 4,065-line rrecov.cpp; F1/F2/F5-core/F7-flags/F9
verified repaired (with test pins); new live drift recorded as F12 and
row 7 (the `--deep` misdocumentation, missing `--image` on the PARAMETERS
recover line, missing `unlock`/`--chkdsk` in API.md §1).
