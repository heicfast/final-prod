# CONSUMER-PERFORMANCE — every disk-read consumer, its metric, and its driver-vs-direct target

> Phase-0 context sheet (performance-parity session-97, Task T2-d / Agent D).
> Written 2026-11-05 by exhaustive read of the tree at **HEAD `e6ee97da`** plus
> the two fresh evidence artifacts. Every code claim carries a `path:line`
> citation; every number carries its GitHub Actions run ID and source file.
> Companion sheets: `worklog/CONSUMERS.md` (the pre-replacement consumer
> inventory, re-verified and corrected here), `worklog/PERFORMANCE.md` (the
> baseline of record + the §2 table this sheet feeds).
>
> READ-ONLY RESEARCH: this file is the session's only output. No source was modified.

**Provenance of every measurement in this sheet:**

| Run | Release | Tree | Transport | Profile | Artifact |
|---|---|---|---|---|---|
| **41042267586** | v0.6.1 (reason `release-v0.6.1`, 2026-10-30T10:39Z) | session-start class `619acec2` | **driver** (RDA grammar, service `rda` installed+verified; assets `rda.sys`/`rda.inf` in the release) | smoke | `/home/z/my-project/artifacts/evidence-41042267586/` |
| **41140212296** | v0.7.0 (reason `release-v0.7.0`, 2026-10-30T22:45Z) | release `04f8869f`-class | **direct** (`\\.\PhysicalDriveN`, user mode, elevated; every bench/api JSON reports `"transport":"direct"`) | smoke | `/home/z/my-project/artifacts/evidence-41140212296/` |

Both runs: `windows-2022` hosted runner (win22 image), elevated by construction,
benchmark disk 4 = 8 GiB fixed VHDX (8,589,934,592 B; 512 logical / 4096
physical = 512e; not 4Kn), benchmark volume CRBIG 8,572,108,800 B NTFS
(`evidence-*/02-disk-setup/disk-state.json`, `07-performance/clean-volume-benchmark.json`).
Scenario volumes: CRNTFS 3,210,739,712 B, CREXFAT 2,139,815,936 B, CRFAT
2,137,980,928 B on multi-gpt disk 3 (S-PART-DELETE deletes the exFAT entry on
disk 3).

**Honest caveat on the driver baseline of record (verified from the artifact,
correcting PERFORMANCE.md §1.1's "completed green"):** run 41042267586's job
started 10:39:02Z and its teardown log stamps 12:39:08Z — the 120-minute smoke
budget (`test-benchmark.yml:95`). The artifact contains the complete
clean-volume benchmark (the §1.1 baseline of record — captured 10:51-11:16Z,
before the timeout) and **4 of 7 smoke scenarios** with verdicts; S-BINEMPTY-NTFS's
deletion state was applied at 12:31:23Z but its lanes never ran, S-FMT-FAT32
and S-PART-DELETE were never reached, and phase 4 never executed (no
`06-comparison/`, no `07-performance/perf-summary.json` in that artifact). All
driver-era scenario walls in §4 are therefore hand-assembled from the
per-scenario `ours-run.json` / `api-result.json` files (they are complete and
citable). The direct run 41140212296 completed the entire 7-scenario matrix +
phase 4 in **1 h 42 m** (22:45→00:27Z) — under the budget the driver era could
not meet; that is itself a lane-level wall-clock EXCEEDS.

---

## §1 The consumer inventory, re-verified at HEAD e6ee97da

CONSUMERS.md (written at `619acec2`, pre-replacement) remains structurally
accurate: every consumer still sees only `cr::IStorageDevice::read_at(offset,
buffer, length)` (include/cr/storage/cr_device.h). What changed at HEAD is the
transport underneath: the RDA grammar path and `open_rda_partition` are **gone**;
headers moved to `include/cr/win/cr_transport.h` and the surface is now (all
citations at HEAD):

- **OD** = `open_disk(disk, err)` — whole-disk `\\.\PhysicalDriveN` GENERIC_READ
  view, 64 KiB envelope, true byte counts (cr_transport.h:60-67;
  cr_transport.cpp:359)
- **ODRW** = `open_disk_rw(disk, err)` — repair write view, GENERIC_READ|WRITE,
  sector-staged writes (cr_transport.h:75-82; cr_transport.cpp:755)
- **ODARW** = `open_disk_apply_rw(disk, err)` — apply view + the corpus IOCTL
  chain (cr_transport.h:84-94; cr_transport.cpp:759)
- **OPD** = `open_physical_drive(disk, err)` — control-path read-only open,
  enumeration only (cr_transport.h:69-73; cr_transport.cpp:327)
- **VE** = `volume_extent` / `volume_extents` / `destination_volume_letter`
  (cr_transport.h:105-134) — driver-independent, unchanged
- **IMG** = `image_file_io()` / `storage::open_image` (cr_transport.h:55-58) —
  file-backed, exempt from elevation
- **direct** = caller's own `CreateFileW` on `\\.\PhysicalDriveN` (health only)

The one structural difference vs CONSUMERS.md §1: **there is no partition-scoped
open anymore.** A volume is opened as `volume_extent(letter) → open_disk(parent)
→ PartitionSlice::create(whole, start, len)` — CLI at rrecov.cpp:728-735,
ABI at cr_ui.cpp:1482-1517. The privilege gate is `require_privilege()`
(rrecov.cpp:526, exit 4 = `ERR_ELEVATION_REQUIRED`), checked per verb:
scan :1294, recover :2225/:2408, image :2841, bench :2934, fix :3170, apply :3559.

| # | Consumer | Entry point(s) at HEAD | Transport | R/W |
|---|---|---|---|---|
| 1 | Scan orchestrator (probe, identity, table re-read) | `src/core/orchestrator/cr_scan.cpp` (`probe_family` :877; whole-disk table re-parse before discovery; carve-stage grouping re-probe) | caller's device (CLI scan → OD via `open_disk_view` rrecov.cpp:549-573; `--image` → IMG) | Read |
| 2 | NTFS engine (MFT walk, $Bitmap, USN, GUID dumps) | `src/core/ntfs/cr_mftwalk.cpp`, `cr_bootstrap.cpp`, `cr_bitmap.cpp`, `cr_attributes.cpp` | orchestrator ReadPipe over the slice | Read |
| 3 | NTFS remnant sweep (deep stage) | `cr_mftwalk.cpp:2283-2364` (`REMNANT_BLOCK` :231) | orchestrator ReadPipe | Read |
| 4 | FAT engine (FAT tables, dir chains, LFN) | `src/core/fat/cr_fat_quick.cpp`, `cr_chain.cpp`, `cr_lfn.cpp` (DirBuffer cap `4u<<20` cr_fat_quick.cpp:135) | orchestrator ReadPipe | Read |
| 5 | exFAT engine (1 MiB FAT windows, dentry clusters) | `src/core/exfat/cr_exfat_quick.cpp:27-39,69-91`, `cr_exfat_dentry.cpp` | orchestrator ReadPipe | Read |
| 6 | ext4 engine (GDT/inode/bitmap/journal) | `src/core/ext4/cr_ext4_engine.cpp` (`GDT_CHUNK = 8 MiB` :839,:870) | orchestrator ReadPipe | Read |
| 7 | Btrfs engine (chunk tree, node reads) | `src/core/btrfs/cr_btrfs_engine.cpp` | orchestrator ReadPipe | Read |
| 8 | ReFS engine (page cache, tables) | `src/core/refs/cr_refs_engine.cpp`, `cr_refs_detect.cpp` | orchestrator ReadPipe | Read |
| 9 | HFS+ engine (B-tree nodes) | `src/core/hfsplus/cr_hfsplus_engine.cpp` | orchestrator ReadPipe | Read |
| 10 | ZFS engine (labels, uberblocks, block pointers) | `src/core/zfs/cr_zfs_engine.cpp` | orchestrator ReadPipe | Read |
| 11 | Lost-partition discovery + probes | `src/core/lost/cr_candidates.cpp` (`WindowScanner` 64 KiB :32-58; N-206 CEIL bound :49) | whole-disk scan view (OD) | Read |
| 12 | Carve engine (raw sweep, signatures, validation) | `src/core/carve/cr_carve.cpp` (`WINDOW=1 MiB`, `OVERLAP=4096`, `MAX_FILE_READ_FOR_HASH=8 MiB` :45-47) + `cr_analysers.cpp`, `cr_rawsigns.cpp`, `cr_usersigns.cpp` | orchestrator ReadPipe (carve stage) | Read |
| 13 | Partition-table parser / LDM | `src/core/table/cr_table_parse.cpp` (:26,:989,:1023,:1030), `cr_ldm.cpp:93` | CLI scan/fix/apply → OD; LDM member reads | Read |
| 14 | Check module (pre-scan damage audit) | `src/core/check/cr_fscheck.cpp` (reads via `dev->read_at` :120), invoked pre-cache at cr_scan.cpp:1251-1259 | raw slice (pre-cache) | Read |
| 15 | Recovery writer (source reads) | `src/core/recovery/cr_safewriter.cpp`; `read_chunk = 1<<20` (cr_safewriter.h:158); **through `build_read_pipe`** — CLI rrecov.cpp:1724/:2514, ABI cr_ui.cpp:2100/:2159/:4485 (all `8 MiB, 256 KiB`) | OD or IMG + ReadPipe | Read (dest = file writes via DestIo) |
| 16 | Preview / archive list (head-window) | `src/core/preview/cr_preview.cpp:16-25` (`PreviewReader`, `kMaxRead` clamp); CLI archive-list rrecov.cpp:2310; ABI cr_ui.cpp:4024/:4077 (PreviewReader :4072,:4110) | PreviewReader over device/snapshot | Read |
| 17 | Thumbnail (decode-bound) | `src/core/preview/cr_thumbnail.cpp` (Budget over PreviewReader :125-203); ABI `cr_ui_thumbnail_generate` cr_ui.cpp:4284/:4341 | PreviewReader | Read |
| 18 | Image creation (S-040) | `src/core/storage/cr_image.cpp` (`kChunk = 64*1024` :1560) + `src/win/cr_image_create.cpp:174-182` (`open_disk` :178) | OD, **no cache/retry layer** | Read source → **Write** file |
| 19 | Bench verb (the measurement surface) | `src/app/rrecov.cpp:2930-3130` (`cmd_bench`; OD :2989; info :3008; read :3022; open churn :3046-3066; seq :3075; rand :3097; `"direct"` :2997) | OD | Read |
| 20 | Repair engine (RawFixer) | `src/core/repair/cr_rawfixer.cpp`; analyze → OD (rrecov.cpp:3698); execute/undo → **ODRW** (rrecov.cpp:3671/:3797; ABI cr_ui.cpp:1999) | OD / ODRW | Read + **Write** (consent-gated) |
| 21 | Apply engine (lost-partition MBR write) | `src/core/apply/cr_apply.cpp`; plan → OD (rrecov.cpp:3236); execute → **ODARW** (rrecov.cpp:3369; ABI cr_ui.cpp:2663) + `refresh_disk_properties` | OD / ODARW | Read + **Write** (consent-gated) |
| 22 | DecryptingDevice (BitLocker in-stack XTS) | `src/core/storage/cr_decrypt_device.cpp:12-60` (whole-sector staging ≤2 sectors) | wraps the volume slice device (cr_ui.cpp:1533-1603) | Read (passthrough+decrypt) |
| 23 | Health (SMART/NVMe/CSMI/TRIM) | `cmd_health` rrecov.cpp:2705-2759; **direct** `CreateFileW(\\.\PhysicalDriveN, GENERIC_READ\|GENERIC_WRITE)` :2723-2726; `src/win/cr_health.cpp` | direct handle (IOCTL) | Read (passthrough) |
| 24 | Enumeration / disk list | `src/win/cr_enumerate.cpp` (**OPD** :148,:201 + volume opens) | OPD + direct | Read (geometry/topology) |
| 25 | BitLocker unlock (`unlock`) | `src/win/cr_bitlocker.cpp` + rrecov.cpp:653-725 | volume-letter fveapi | management op (no raw data path) |
| 26 | Destination arming / overlap refusal | `volume_extents` / `destination_volume_letter` (cr_transport.h:111-134) | VE — driver-independent | Read (IOCTL only) |
| 27 | Session-only modules (save/load, lastscan, reghive, bcdfix, nvramfix, bootfix, fsck-launch, query, tree, compress, crypto) | `src/core/{session,reghive,bcdfix,nvramfix,query,tree,compress,crypto}/`, `src/win/cr_bootfix.cpp`, `check/cr_fsck_launch.cpp` | **none** — re-verified at HEAD: zero `->read_at(` in those translation units | n/a |

The read pipe every engine consumer rides (re-verified at HEAD):
`build_read_pipe(slice, 8 MiB, 256 KiB)` — cr_scan.cpp:164-203 (RetryPolicy
`max_attempts=3`, `RetryMode::Skip` :179-181), invoked at cr_scan.cpp:1298;
the budget is the registered divergence N-045/D-72 (cr_cached.h:17). A cache
miss fills a whole 256 KiB block in one inner `read_at` (cr_cached.cpp) — the
engine path's transport request is therefore **256 KiB, 64 KiB-aligned**; the
degraded bare-slice path is pinned. Parallelism: worker pool ≤8, every transport
read synchronous QD1; carve windows / remnant chunks / candidate scans fan out
(CONSUMERS.md §3 carries over — cr_parallel.h:40-87 unchanged in shape).

CLI verb → transport at HEAD (dispatch rrecov.cpp:3913-3950): `scan` →
`open_disk_view`/`open_volume_view` (OD + slice, rrecov.cpp:549/:644);
`recover` → OD or IMG + ReadPipe (rrecov.cpp:2225/:2408/:2514); `health` →
direct handle; `image` → OD (rrecov.cpp:2841); `bench` → OD (rrecov.cpp:2934);
`fix` → OD/ODRW (rrecov.cpp:3170/:3698/:3797); `apply` → OD/ODARW
(rrecov.cpp:3236/:3369). The driver-era `require_driver()` gate is gone,
replaced by `require_privilege()` (exit 4 refusal, rrecov.cpp:526).

---

## §2 Per-consumer: access pattern, the metric that matters, and the target

Format: **pattern** → **metric** → **target (driver baseline, run ID)** — or,
where no driver number exists for the pattern, how it will be measured.
The verdict column is §6's; this section justifies it.

**2.1 Scan orchestrator.** Tiny random probes: one 512 B read at offset 0
(`probe_family`, cr_scan.cpp:877-882) + identity CRC snapshot + GPT/MBR re-parse
(table reads, §2.13) — O(1)+O(partitions) reads per volume, QD1.
**Metric: time-to-first-decision — the quick-mode wall** (probe+table+stage-0).
Target: driver quick wall **150 ms** (41042267586 clean-volume-benchmark.json).
Direct: **27 ms** — EXCEEDS. The per-open cost underneath also collapsed:
bench `open` 17,019 ns → 9,213 ns (same runs; the driver's SDDL/token dance is
gone). No further target needed; keep the 27 ms as the new floor.

**2.2 NTFS engine (MFT walk).** Record-granular walk: one read per MFT record
(1 KiB typical), sequential-in-run but record-granular, through the 256 KiB
cache; boot+backup probes 512 B; $Bitmap whole-load; USN ≤64 MiB
(USN_HARVEST_CAP cr_mftwalk.cpp:25); S-012 GUID dumps 0x8000-sector chunks.
**Metric: deep-mode wall on a live NTFS volume + (native) cache hit-rate and
small-random latency.** Target: driver deep wall **16,505 ms** (8 GiB clean
vol, 41042267586). Direct: **16,467 ms** — parity (−0.2%). The scenario-lane
deep wall (3 GiB seeded) moved 7.56 → 8.10 s (+7.1%, §4 — the one worse point;
single-sample, needs a repeat). The native metric is UNMEASURED both eras:
add `CachedReader` hits/misses to `scan --json` (the counters exist —
docs/benchmarking.md §4) and a `bench rand --block-kb 1` leg.

**2.3 NTFS remnant sweep.** Whole-volume sweep in 4 MiB blocks + overlap
(REMNANT_BLOCK cr_mftwalk.cpp:231), parallel chunk fan-out (cr_mftwalk.cpp:2331).
**Metric: sequential MB/s at 4 MiB QD1** (plus the deep-stage wall that contains
it). Driver baseline at that size: **202.8-242.4 MiB/s** (run 40227375329
sweep, `\\.\C:`) — no driver-era 4 MiB point on the benchmark VHDX and no
direct-era 4 MiB point at all. UNMEASURED at native size; measured only via the
deep wall (2.2). Measure with: `bench seq --block-kb 4096` on the benchmark
disk, added to the harness bench loop (Run-Benchmark.ps1:362).

**2.4 FAT engine.** Whole-FAT single reads (up to ~300 MiB legal) + run-batched
dir reads capped at 4 MiB (cr_fat_quick.cpp:135). **Metric: deep/deepcarve wall
on a FAT32 volume.** Driver: S-DEL-FAT32 deepcarve **235.34 s** (41042267586
ours-run.json). Direct: **146.71 s** (41140212296) — EXCEEDS (−37.7%), but the
engine itself is degenerately unexercised (0 files found, recall-0 state); the
wall measures carve+I/O, not FAT parsing. A live-file FAT32 deep scan fixture
is the honest measurement (§3.6).

**2.5 exFAT engine.** 1 MiB-aligned windowed lookups (cr_exfat_quick.cpp:27-39,
69-91), cluster-granular dentry reads. **Metric: deepcarve wall (window
locality rides it).** Driver: S-DEL-EXFAT **234.76 s**. Direct: **146.09 s** —
EXCEEDS (−37.8%). Same recall-0 caveat as 2.4 (0 files).

**2.6-2.10 ext4 / Btrfs / ReFS / HFS+ / ZFS.** Same ReadPipe shapes as
CONSUMERS.md §2 (ext4 8 MiB GDT chunks + 4 KiB metadata; Btrfs/HFS+ random
node reads; ReFS page reads; ZFS 4×256 KiB labels + psized block reads).
**Metric: per-engine scan wall on a live volume of that FS.** **No driver
baseline and no direct measurement exists** — the smoke matrix has no such
volumes (disk-state.json: FAT32/NTFS/exFAT only). UNMEASURED by fixture
absence, not by transport difference (the engines are transport-agnostic).
Measure with: an ext4/Btrfs/ReFS/HFS+/ZFS seeded volume in the full profile
(`.github/scenarios/benchmark-matrix.json` full scenarios) or a dedicated
dispatch.

**2.11 Lost-partition discovery.** Pure sequential 64 KiB whole-disk sweep
(WindowScanner, cr_candidates.cpp:32-58) + sparse 512 B boot/MFT probes at
derived offsets. **Metric: whole-disk deepcarve wall on a partition-deleted
disk (discovery time) + 64 KiB sequential MB/s.** Driver: **none** — 41042267586
never reached S-PART-DELETE (job timeout, §5). Direct: the S-PART-DELETE lane
failed pre-measurement (whole-disk deepcarve exits ~0.5 s with no session,
run-ours.ps1:120-122 fast-fail — §5). UNMEASURED both eras. The transport-level
proxy target: driver-era 64 KiB seq = **532.9 MiB/s** (run 39807929475,
driver's own parity table) / **201.9-208.6 MiB/s** (run 40227375329, `\\.\C:`).
Measure with: fix the S-PART-DELETE fast-exit (root-cause first — the scan
stdout was lost, §5.4), then `bench seq --block-kb 64` + the scenario lane.

**2.12 Carve engine.** Sequential 1 MiB windows + 4096 B overlap, one
`read_at` per window (cr_carve.cpp:45-46), parallel window fan-out (≤8 workers
sharing the handle), validation re-reads ≤8 MiB/file; unreadable windows
skip-not-count. **Metric: deepcarve wall (the dominant product cost) +
sequential MB/s at 1 MiB.** Targets: driver deepcarve **1,501,622 ms** (8 GiB
clean vol, 41042267586) and scenario walls 392.11/234.76/235.34/393.39 s;
driver bench seq 1 MiB **536.92 MiB/s**. Direct: **1,014,235 ms** (−32.5%, 8.45 vs 5.71 MB/s effective), scenarios 243.44/146.09/146.71/248.09 s
(−37…−38%), bench seq **544.17 MiB/s** (+1.4%). EXCEEDS on every wall.

**2.13 Table parser / LDM.** Small structured reads at fixed offsets: LBA0
512 B, GPT header + one entries-array read (cr_table_parse.cpp:989-1030);
LDM TOCBLOCK/VMDB + member-disk chunked reads (cr_ldm.cpp:93).
**Metric: rides the quick wall (probe+table path)** — 150 → 27 ms (EXCEEDS
proxy). LDM multi-member reads: no LDM fixture in the matrix → UNMEASURED.

**2.14 Check module.** A handful of small reads on the raw slice, pre-cache
(cr_scan.cpp:1251-1259; cr_fscheck.cpp:120). **Metric: none of its own — must
not regress scan start.** Rides the quick wall (27 ms). MEETS by proxy.

**2.15 Recovery writer.** Sequential per-extent 1 MiB chunks + exact tail
(cr_safewriter.h:158) through the same 8 MiB/256 KiB retry+cache pipe (M-136;
rrecov.cpp:1724/:2514, cr_ui.cpp:2100/:2159/:4485); holes write zeros with no
read; serial `SafeWriter::process`. **Metric: user-facing recovery wall +
sustained read+write duplex at 1 MiB.** Target: driver API recovery **49.2 ms**
(82 files, 41042267586 api-result.json) / driver bench seq 1 MiB 536.92 MiB/s.
Direct: **36.8 ms** (−25.2%); scenario API recoveries 77.9→61.2, 5.1→3.8,
4.6→4.0, 119.9→78.6 ms — EXCEEDS on every measured point. **Caveat:** until
recall-0 is fixed (§5.3) the recovered payload is 0-byte placeholders only —
the *sustained duplex* number (MiB/s of real file content) is UNMEASURED-in-
effect. Measure with: fix recall-0, then the recovery wall on real content +
`bench seq --block-kb 1024`.

**2.16 Preview.** Bounded head-window reads, every call clamped at 4 MiB
(kMaxRead, cr_preview.h:59; cr_preview.cpp:19-25), stream formats bounded 256 KiB.
**Metric: TTFB — time from preview-open to first bytes placed.** The API lane
exercises the probe (api_harness.c:377-405) but records only `ok/size/kind`,
**no timing, either era**. UNMEASURED. Measure with: QPC-wrap the preview
probe in api_harness.c and emit `preview_ms` in api-result.json.

**2.17 Thumbnail.** Decode-bound, Budget-capped reads over PreviewReader
(cr_thumbnail.cpp:125-203; EXIF ≤256 KiB). **Metric: thumbnail wall (read +
decode).** Not present in the CI harness at all (no `cr_ui_thumbnail_generate`
probe). UNMEASURED. Measure with: a thumbnail probe in api_harness.c
(`--thumbnail` mode) on the seeded corpus images, timed.

**2.18 Image creation.** 64 KiB pure sequential chunks end-to-end, SHA-256
streamed, cancel per chunk (cr_image.cpp:1560); source = raw OD, **no
retry/cache** (cr_image_create.cpp:174-182). **Metric: sustained duplex MB/s +
wall for a whole-disk image.** Correctness probe ran in BOTH eras (5 GiB
round-trip, hash sidecar match, scan --image exit 0 — image-create.txt in both
artifacts; the direct run's probe passed the hash/size gates, no Fail in
gates.md). **Timing was never captured** — the probe prints progress lines but
nothing records the wall. UNMEASURED (timing). Measure with: stopwatch the
probe in Run-Benchmark.ps1 phase 1 + `bench seq --block-kb 64` as the
transport-level proxy (driver-era bar 532.9/201.9-208.6 MiB/s, runs
39807929475 / 40227375329).

**2.19 Bench verb.** Self-measuring: `seq` 4..16384 KiB blocks over 1..65536 MiB
span, `rand` LCG mean-µs+IOPS, `open` churn ≤10000, `read` LBA0, `info`
(rrecov.cpp:2930-3130). Works in both eras (`--json` schemas identical; the
`transport` field flipped `rda`→`direct` by truth, rrecov.cpp:2997). MEETS by
construction; it is the measurement instrument for everything else.

**2.20 Repair (fix).** Analyze = OD read view (rrecov.cpp:3698); execute/undo =
ODRW with sector-span snapshot→write→verify (rrecov.cpp:3671/:3797; ABI
cr_ui.cpp:1999). **Metric: analyze wall + execute transaction wall on a damaged
fixture.** Both eras ran only the dry-run-on-clean probe (exit 0, no timing).
UNMEASURED. Measure with: a damaged-boot-sector fixture + timed fix dry-run in
the feature probes (phase 1).

**2.21 Apply.** Plan = OD (rrecov.cpp:3236); execute = ODARW + the IOCTL chain
+ 3×1 s refresh settle (rrecov.cpp:3369; cr_ui.cpp:2663). **Metric: plan wall +
execute-to-visible wall.** Never exercised in either era (S-PART-DELETE is the
vehicle; §5.2). UNMEASURED.

**2.22 DecryptingDevice.** Passthrough + ≤2-sector staging + XTS per data unit
(cr_decrypt_device.cpp:22-58). **Metric: throughput vs the inner device
(AES-XTS cost).** No BitLocker volume in the matrix; UNMEASURED. Measure with:
a known-FVEK BitLocker fixture (format, mount, scan through the key path) or,
minimally, a unit-level XTS-over-ReadPipe throughput probe on an image.

**2.23 Health.** Direct IOCTL passthrough (rrecov.cpp:2723-2726; SMART/NVMe/
CSMI ladders). **Metric: TTFB — time to first health verdict** (the UI's first
paint of the disk panel). Probe ran both eras (health.txt: SMART rc=50070001 →
NVMe fallback — the honest multi-source walk); **no timing captured**.
UNMEASURED. Measure with: QPC-wrap the health probe in Run-Benchmark.ps1
phase 1 (currently un-timed, :170-179).

**2.24 Enumeration.** OPD geometry + direct volume opens (cr_enumerate.cpp:148,
:201). **Metric: `rrecov list` wall + per-open ns.** Proxy measured: bench
`open` (the same CreateFile+geometry class) 17,019 → 9,213 ns — EXCEEDS proxy.
The list wall itself untimed; add a stopwatch to the phase-3 staging probe
(test-benchmark.yml:194-199).

**2.25 BitLocker unlock.** Management op (fveapi); no data path. Out of perf
scope by classification (cr_transport.h:20-25) — n/a.

**2.26 Destination arming.** IOCTL-only (volume_extents ≤8, destination path
resolve). Exercised implicitly by every recovery (overlap refusal stayed quiet
by construction). µs-class; UNMEASURED, low priority — a timing field on the
recovery open is sufficient when needed.

**2.27 Session-only modules.** No device reads (re-verified: zero `->read_at(`
in src/core/{session,reghive,bcdfix,nvramfix,query,tree,compress,crypto,check}).
Out of scope; their perf surfaces (query 200 ms gate etc.) are unchanged by the
transport swap. n/a.

---

## §3 The metrics not yet measured anywhere — probes to build

The mandate (session-performance-parity-prompt.md:69-79) requires throughput,
latency, CPU, memory, and cancellation per consumer. Five metric families have
**no captured number in either era**; each below names the consumer that needs
it, the concrete probe, and where it slots into the existing harness
(`rrecov bench` + `test-benchmark.yml` + `Run-Benchmark.ps1` + `api_harness.c`).

**3.1 CPU per byte.** Needed by: carve (the 4-worker CPU burner), remnant
sweep, recovery writer, every engine (UI-starvation guard, prompt:110).
**The data already exists and is simply not transcribed**: run-ours.ps1:44-73
samples `cpu_seconds` + `working_set_bytes` every 500 ms into
`samples-<mode>.csv`, and Run-Benchmark.ps1:695 archives the deepcarve CSV.
Assembled from the two artifacts (S-DEL-NTFS deepcarve, 3,210,739,712 B):
driver **1,501.0 CPU-s / 392.1 s wall = 502 CPU-s/GiB (3.84 cores)**;
direct **910.5 CPU-s / 243.4 s = 305 CPU-s/GiB (3.75 cores)** — direct is
**−39% CPU per byte**, already superior, unclaimed. Probe: (a) Run-Benchmark
phase 4 (lines 756-781) parses the archived CSVs and writes
`cpu_seconds_total`, `cpu_s_per_gib`, `avg_cores` into perf-summary.json per
scenario/mode; (b) archive quick/deep samples too (add to the Copy-Evidence
list, :688-697); (c) optional `bench --cpu` field (GetProcessTimes around the
seq loop) so the verb itself reports cpu_ms/GiB.

**3.2 Memory footprint / boundedness during large scans.** Needed by: carve
(dedup tree), NTFS walk (record cache), ReFS (page cache + live-reachability
set), session save (M-150 prealloc). Same CSVs: working set stayed **19.9→39.8 MB
(driver) vs 21.7→39.1 MB (direct)** over the whole 3 GiB deepcarve — bounded
and flat, but only one volume size is captured and only deepcarve is archived.
Probe: transcribe `ws_peak_bytes` + `ws_end_bytes` from the CSVs into
perf-summary.json (same slot as 3.1); add the full-profile 8 GiB scenarios
(S-DECAY/S-OVR/S-MFT) to the dispatch to get the large-volume shape; the
existing M-150-class unit gates cover the allocation side.

**3.3 Cancellation responsiveness (time-to-stop).** Needed by: every
interactive scan (the UI's cancel button), carve fan-out, recovery queue.
Current state: the API cancel probe fires `cr_ui_scan_cancel` after N seconds
(api_harness.c:107-112) and the run returns `cancelled=true`, but **nothing
measures the interval between cancel issue and `cr_ui_scan_run` return** —
the number the mandate asks for. The probe data shows only when cancel fired
(4,264.7 ms into a 230.6 s scan, driver; 3,122.4 ms into 164.9 s, direct —
both scans then ran to their poll points and saved sessions). Probe: in
api_harness.c, timestamp the cancel-thread fire and `t_scan_run1` (already
QPC-captured, :291-293), emit `cancel_to_stop_ms` in api-result.json; keep the
50 ms watcher tick + per-item poll cadence as the unit-pinned floor
(test_parallel.cpp `CancelPoolWatcherDrainsQueueWithinOneTick`). Optionally
port run-behavior.ps1's CTRL_BREAK hard-kill probe into the lane for the
process-level number.

**3.4 TTFR (time to first result).** Needed by: interactive scan UX (quick
mode especially), API event stream. Current state: the CLI probe exists
(`ttfr_ms=`/`first_files=` log lines, extracted at Run-Benchmark.ps1:348-356)
and both eras recorded **0 ms** for deep/deepcarve (the counters coalesce at
stage boundaries — the honest floor) and **null** for quick (the clean volume
holds no deleted files at quick stage, so the probe never fires). Probe: (a)
run one clean-volume quick scan on the **seeded, pre-deletion** volume (files
present) so the quick-mode TTFR is meaningful — a one-line addition to phase 2
before the scenario loop; (b) add the API-side TTFR: timestamp the first
`files_found` event in `event_fn` (case 3, api_harness.c:75) and emit
`ttfr_ms` in api-result.json.

**3.5 Scalability at volume sizes.** Needed by: carve + lost discovery (whole-
disk sweep cost is the product's perceived speed), image create. Current walls
cover 2.1 / 3.0 / 8 GiB but never the same scenario at multiple sizes, so no
scaling curve exists (is deepcarve MB/s flat in size, or does the 8 GiB clean volume's 8.45 MB/s vs the 3 GiB scenario's 13.19 MB/s reflect size, fill, or
both?). Probe: a size ladder — the bench verb already sweeps spans
(`seq --mb`); the scenario-side needs a dispatch with the same scenario on
2/8/32 GiB volumes (extend the full profile's disk set in
benchmark-matrix.json; the 8 GiB CRBIG disk already exists, a 32 GiB disk is a
New-TestDisk.ps1 parameter), reporting MB/s vs size in perf-summary.json.

**3.6 The missing engine fixtures (per-consumer walls for 2.4-2.10, 2.13 LDM,
2.22 BitLocker).** Not a metric family but the same gap class: FAT32/exFAT
walls are carve-dominated (0 files found — recall-0), and ext4/Btrfs/ReFS/
HFS+/ZFS/LDM/BitLocker have no volumes in the matrix at all. Probe: extend the
full profile with one seeded volume per missing FS (generate_seed.ps1 is
FS-parameterized) + a locked-BitLocker volume with a known FVEK; until then
those rows stay UNMEASURED with the reason recorded (fixture absence, not
transport doubt — the engines sit on the same ReadPipe).

**Slot summary:** bench verb → 3.1(c); Run-Benchmark.ps1 → 3.1(a)(b), 3.2,
3.4(a) (phase 2/4 edits at :324-392 and :756-781); api_harness.c → 3.3, 3.4(b)
(both inside the existing lifecycle — no new mode needed);
test-benchmark.yml → unchanged shape (the evidence tree already carries
07-performance); benchmark-matrix.json → 3.5, 3.6.

---

## §4 The end-to-end walls: driver baseline vs current direct

Clean volume (disk 4, 8 GiB 512e VHDX, NTFS, 1209 live files; both runs'
`07-performance/clean-volume-benchmark.json`):

| Wall / metric | Driver (41042267586) | Direct (41140212296) | Δ | Verdict |
|---|---:|---:|---:|---|
| scan quick | 150 ms | 27 ms | −82% | **EXCEEDS** |
| scan deep | 16,505 ms | 16,467 ms | −0.2% | **MEETS** (parity) |
| scan deepcarve | 1,501,622 ms | 1,014,235 ms | −32.5% | **EXCEEDS** |
| deepcarve effective throughput | 5.71 MB/s | 8.45 MB/s | +48% | **EXCEEDS** |
| bench seq 1 MiB × 256 MiB | 536.92 MiB/s | 544.17 MiB/s | +1.4% | **MEETS+** |
| bench rand 1 MiB × 1000 | 1993.22 µs / 502 IOPS | 1976.72 µs / 506 IOPS | −0.8% / +0.8% | **MEETS+** |
| bench open × 1000 | 17,019 ns / 58,757 ops/s | 9,213 ns / 108,542 ops/s | −45.9% | **EXCEEDS** |
| API open (mask 7, 4 threads) | 0.9 ms | 0.5 ms | −44% | **EXCEEDS** |
| API scan_run | 1,513,221.7 ms | 1,113,445.2 ms | −26.4% | **EXCEEDS** |
| API save_session | 10.8 ms | 9.3 ms | −13.9% | **MEETS+** |
| API query | 0.2 ms | 0.1 ms | −50% | **MEETS+** |
| API recovery (82 files) | 49.2 ms | 36.8 ms | −25.2% | **EXCEEDS** |
| TTFR deep / deepcarve | 0 / 0 ms | 0 / 0 ms | — | **MEETS** (floor) |
| TTFR quick | null (no files) | null (no files) | — | UNMEASURED (§3.4) |
| carved_found (clean vol) | 2138 | 2134 | −4 | observation — 0.19% carve-result drift between eras, recorded, ungated |

Scenario CLI walls (driver: per-scenario `ours-run.json` in 41042267586 —
assembled by hand, see the §-header caveat; direct: 41140212296
`07-performance/perf-summary.json`):

| Scenario | Volume | Mode | Driver | Direct | Δ | Verdict |
|---|---|---|---:|---:|---:|---|
| S-DEL-NTFS | 3.0 GiB | quick | 0.51 s | 0.51 s | 0 | **MEETS** |
| S-DEL-NTFS | 3.0 GiB | deep | 7.56 s | 8.10 s | **+7.1%** | **GAP (marginal, unconfirmed)** — the single worse point; single sample per era, clean-volume deep is parity; re-measure with an N-run median before optimizing |
| S-DEL-NTFS | 3.0 GiB | deepcarve | 392.11 s | 243.44 s | −37.9% | **EXCEEDS** |
| S-DEL-EXFAT | 2.1 GiB | deepcarve | 234.76 s | 146.09 s | −37.8% | **EXCEEDS** |
| S-DEL-FAT32 | 2.1 GiB | deepcarve | 235.34 s | 146.71 s | −37.7% | **EXCEEDS** |
| S-RECYCLE-NTFS | 3.0 GiB | deepcarve | 393.39 s | 248.09 s | −36.9% | **EXCEEDS** |
| S-BINEMPTY-NTFS | 3.0 GiB | deepcarve | (timeout — never ran) | 248.06 s | — | **UNMEASURED** (driver side) |
| S-FMT-FAT32 | 2.1 GiB | deepcarve | (timeout — never reached) | 151.65 s | — | **UNMEASURED** (driver side) |
| S-PART-DELETE | whole disk 3 | deepcarve | (timeout — never reached) | FAILED ~0.5 s, no session | — | **UNMEASURED + lane defect (§5.2)** |

Scenario API lanes (`api-result.json` per scenario):

| Scenario | Driver scan_run | Direct scan_run | Δ | Driver recovery | Direct recovery | Verdict |
|---|---:|---:|---:|---:|---:|---|
| S-DEL-NTFS | 384,776.9 ms | 284,735.9 ms | −26.0% | 77.9 ms | 61.2 ms | **EXCEEDS** |
| S-DEL-EXFAT | 229,571.7 ms | 171,479.5 ms | −25.3% | 5.1 ms | 3.8 ms | **EXCEEDS** |
| S-DEL-FAT32 | 229,899.4 ms | 168,726.6 ms | −26.6% | 4.6 ms | 4.0 ms | **EXCEEDS** |
| S-RECYCLE-NTFS | 384,679.0 ms | 279,944.8 ms | −27.2% | 119.9 ms | 78.6 ms | **EXCEEDS** |
| S-BINEMPTY-NTFS | (timeout) | 275,236.8 ms | — | — | 40.1 ms | UNMEASURED (driver side) |
| S-FMT-FAT32 | (timeout) | 177,609.9 ms | — | — | 4.0 ms | UNMEASURED (driver side) |
| S-PART-DELETE | (timeout) | no session | — | — | — | lane defect (§5.2) |

Observations that belong to the record: (a) in the direct run the API scan wall
exceeds the CLI deepcarve wall on the same volume (284.7 vs 243.4 s,
S-DEL-NTFS) while the driver era matched them (384.8 vs 392.1) — plausibly the
API lane's event/session machinery (1,001 save-progress events) or cache-state
differences; worth one probe, not a gate; (b) the cancel+resume probe:
cancel fired 4,264.7 ms (driver) / 3,122.4 ms (direct) into multi-minute scans,
both resumed to completion (230.6 s / 164.9 s, −28.5%) — the resume path
EXCEEDS, the stop-latency itself unmeasured (§3.3); (c) recall/precision are
0.0 in BOTH eras (§5.3) — every recovery wall above moves placeholders only.

---

## §5 Harness-state findings that gate consumer measurement

**5.1 The stale `rda` transport expectation in the API gate (CONFIRMED, blocks
every API-lane verdict).** Run-Benchmark.ps1:628-631 asserts
`$apiResult.transport -eq 'rda'` — but at HEAD `cr_ui_device_transport` returns
`"direct"` (cr_ui.cpp:1445/:1501/:1588, captured by api_harness.c:240-242 and
emitted at :494). Result in run 41140212296: all six completed API lanes carry
the reason `"api transport was 'direct' (expected rda)"` (06-comparison/gates.md
lines 15-20) — the recall failures are the same as the driver era's, but this
stale string adds a spurious failure to every lane. The CLI lane's equivalent
was already updated (`DriverInfo = 'transport=direct'` Run-Benchmark.ps1:518 →
run-ours.ps1:144-149 matches the `[transport] direct raw I/O` line, emitted at
rrecov.cpp:561; assertion hits=1 on every mode in ours-run.json). Fix: expect
`'direct'` (or assert non-empty + recorded), one line at :628.

**5.2 The S-PART-DELETE no-session fast-fail (CONFIRMED, kills the discovery
scenario and its APPLY probe).** The CLI lane runs whole-disk scans for
`delete_partition` (run-ours.ps1:80-83: `--disk N --deep`); the deepcarve leg
exited in ~0.5 s **without writing a session**, and run-ours.ps1:120-122 threw
`"scan deepcarve produced NO session and was not capped - refusing to continue
(no silent skip)"`. The throw propagated to Run-Benchmark.ps1's catch (:711-717):
`lanes: {}`, `passed: false` (04-scenarios/S-PART-DELETE/verdict.json), the API
lane never ran, and the post-action lost-partition APPLY probe (:646-686) never
executed. Root cause is undiagnosed **because the scan's stdout/stderr were
lost** (5.4). Note the driver era cannot supply a comparison: 41042267586
never reached this scenario (120-min timeout). This is the only scenario whose
direct-path behavior is both broken and unmeasured.

**5.3 Recall-0 is pre-existing in BOTH eras (CONFIRMED — not a direct-path
regression, but it gates the gates).** Every scenario lane in both runs scored
hash-recall 0.0 / name-recall ≤0.0003 (41140212296 06-comparison/summary.json;
41042267586 per-scenario verdict.json — the driver era failed the same floors:
S-DEL-NTFS/EXFAT/RECYCLE `passed=false`, cli+api recall 0.0). The recovered
inventories in both artifacts contain only 0-byte directory placeholders
(`bytes_seen: 0`; every record's sha256 = e3b0c44…, the empty-string digest;
S-DEL-NTFS cli recovered-inventory.json, both runs). Downstream effects on
performance measurement: (a) the floor gates can never pass, so the lane exits
1 regardless of transport speed; (b) recovery walls move no payload — the
writer's sustained-duplex metric is unmeasurable-in-effect (§2.15); (c) the
cancel+resume probe fails on `resumedFiles=0` (gates.md line 14) — the resume
itself works (both eras), there is simply nothing to find. Fixing recall-0
(or re-pointing the probe volumes at states with recoverable content) is a
prerequisite for the recovery-consumer targets to be meaningful.

**5.4 The exception-path evidence gap (CONFIRMED — why 5.2 is undiagnosed).**
Run-Benchmark.ps1 copies the CLI lane's scan stdout/stderr/stage logs into the
evidence tree only in the success path (:688-697, after both lanes complete).
The catch block (:711-717) writes only `verdict.json`. S-PART-DELETE's evidence
directory contains just `state/` + `verdict.json` — the `scan-deepcarve.stdout`
that would show the fast exit's code/rc/stderr stayed in the work directory and
was destroyed with the runner. The quick/deep legs of the same scenario (which
did produce sessions in the work dir) were lost the same way, so even the
partial walls are unrecorded. Fix: mirror the Copy-Evidence list (or at least
`scan-*.stdout/.stderr` + `stage-*.log` + `samples-*.csv`) into the catch
block, and/or have run-ours.ps1 copy-on-throw before the fast-fail.

**5.5 The driver baseline of record is partial (bounds §4's driver columns).**
See the header caveat: 41042267586 hit the 120-min smoke budget mid-scenario
(teardown.log 12:39:08Z = job start + 120:00; S-BINEMPTY-NTFS state applied
12:31:23Z, no lanes/verdict; no 06-comparison/, no perf-summary.json — phase 4
never ran). Consequences: S-BINEMPTY/S-FMT-FAT32/S-PART-DELETE have no driver
walls; cross-era scenario comparisons must be assembled from per-scenario JSONs
(as §4 does); and the driver-era gate summary (including whether its own
cancel+resume probe recorded the same resumedFiles=0 failure) is unrecoverable.
PERFORMANCE.md §1.1's "completed green" should be corrected to "completed with
its clean-volume benchmark and 4 of 7 scenario lanes; timed out before the
rest" when §2 of that sheet is filled. The direct run finishing all 7 scenarios
+ phase 4 in 1 h 42 m is the like-for-like lane-level improvement (the deepcarve
−32…−38% walls are what bought it).

---

## §6 The per-consumer target table (THE DELIVERABLE)

Verdicts: **MEETS** = parity within noise or better; **EXCEEDS** = materially
better than the driver baseline; **GAP** = worse (with the lever);
**UNMEASURED** = no number for that consumer's own metric in either era (with
how it gets one). Driver baseline = run 41042267586 unless another run ID is
named; direct = run 41140212296.

| # | Consumer | Pattern (size / align / seq-rand / QD / threads) | Metric that matters | Driver baseline | Current direct | Verdict | Lever if GAP / UNMEASURED |
|---|---|---|---|---|---|---|---|
| 1 | Scan orchestrator | 512 B probes @0 + table re-reads; QD1 | Quick-mode wall (TTSD) | 150 ms | 27 ms | **EXCEEDS** (−82%) | keep 27 ms as the new floor |
| 2 | NTFS engine (MFT walk) | 1 KiB records ~seq-in-run via 256 KiB cache; QD1 | Deep wall + cache hit-rate + small-random latency | 16,505 ms (8 GiB clean) | 16,467 ms | **MEETS** (wall −0.2%); hit-rate/1 KiB latency UNMEASURED | expose CachedReader hits/misses in `scan --json`; `bench rand --block-kb 1`; re-measure the scenario deep wall (§4's +7.1% point) with an N-run median |
| 3 | NTFS remnant sweep | 4 MiB + overlap, seq, parallel chunks | 4 MiB QD1 MB/s + deep wall | 202.8-242.4 MiB/s @4 MiB (run 40227375329, `\\.\C:`); deep wall 16,505 ms | no 4 MiB point; deep wall 16,467 ms | **UNMEASURED** at native size | `bench seq --block-kb 4096` leg in the harness bench loop (Run-Benchmark.ps1:362) |
| 4 | FAT engine | whole-FAT ≤300 MiB single reads + 4 MiB dir batches | Deep/deepcarve wall on FAT32 | deepcarve 235.34 s (0 files found) | 146.71 s (0 files) | **MEETS+/EXCEEDS** wall (−37.7%) — engine degenerately unexercised | live-file FAT32 deep-scan fixture (recall-0 fix or seeded live volume) |
| 5 | exFAT engine | 1 MiB-aligned windows, chain-random | Deepcarve wall | 234.76 s (0 files) | 146.09 s (0 files) | **EXCEEDS** (−37.8%) | same fixture caveat as #4 |
| 6 | ext4 engine | 8 MiB GDT + 4 KiB metadata | Engine scan wall | none (no fixture) | none | **UNMEASURED** | ext4 seeded volume in full profile |
| 7 | Btrfs engine | random 4-64 KiB node reads | Engine scan wall | none | none | **UNMEASURED** | Btrfs seeded volume |
| 8 | ReFS engine | random 16-64 KiB page reads | Engine scan wall | none | none | **UNMEASURED** | ReFS seeded volume |
| 9 | HFS+ engine | block_size node reads | Engine scan wall | none | none | **UNMEASURED** | HFS+ seeded volume |
| 10 | ZFS engine | 4×256 KiB labels + 512 B..16 KiB block reads | Engine scan wall | none | none | **UNMEASURED** | ZFS seeded volume (or image fixture) |
| 11 | Lost-partition discovery | 64 KiB seq whole-disk + sparse 512 B probes | Discovery wall on partition-deleted disk + 64 KiB MB/s | none (run timed out pre-scenario); 64 KiB class: 532.9 MiB/s (run 39807929475) / 208.6 (`\\.\C:`, 40227375329) | lane FAILED (~0.5 s, no session) | **UNMEASURED + DEFECT** | root-cause the S-PART-DELETE fast-exit (recover its stdout first — §5.4); `bench seq --block-kb 64` leg |
| 12 | Carve engine | 1 MiB + 4 KiB seq windows, ≤8 parallel workers | Deepcarve wall + 1 MiB MB/s | 1,501,622 ms clean / 392.11 s scenario; bench seq 1 MiB 536.92 MiB/s | 1,014,235 ms (−32.5%) / 243.44 s (−37.9%); 544.17 MiB/s | **EXCEEDS** | none — this is the flagship parity win; watch the −4 carved_found drift |
| 13 | Table parser / LDM | LBA0 512 B + GPT array reads; LDM member chunks | Rides quick wall | 150 ms (proxy) | 27 ms (proxy) | **MEETS+** (proxy); LDM multi-member UNMEASURED | LDM fixture in full profile |
| 14 | Check module | handful of small pre-cache reads | Must not regress scan start | rides 150 ms | rides 27 ms | **MEETS** (proxy) | — |
| 15 | Recovery writer | 1 MiB seq chunks + exact tail through 8 MiB/256 KiB pipe; serial | Recovery wall + sustained duplex at 1 MiB | 49.2 ms / 82 files (API); 77.9-119.9 ms scenarios; seq 1 MiB 536.92 MiB/s | 36.8 ms (−25.2%); 61.2/3.8/4.0/78.6 ms; 544.17 MiB/s | **EXCEEDS** on walls; sustained payload duplex UNMEASURED-in-effect (recall-0: placeholders only) | fix recall-0 (§5.3), then re-measure recovery on real content |
| 16 | Preview | ≤4 MiB head-window, extent-assembled | TTFB (open→first bytes) | not timed (`preview.ok` only) | not timed | **UNMEASURED** | time the api_harness.c:377-405 probe → `preview_ms` |
| 17 | Thumbnail | Budget-capped PreviewReader reads, decode-bound | Thumbnail wall | absent from CI | absent | **UNMEASURED** | `--thumbnail` probe in api_harness.c over seeded images |
| 18 | Image create | 64 KiB pure seq duplex, streamed SHA-256, no retry/cache | Whole-image wall + 64 KiB duplex MB/s | correctness PASS, untimed (5 GiB probe) | correctness PASS, untimed | **UNMEASURED** (timing); MEETS (correctness) | stopwatch the phase-1 probe; `bench seq --block-kb 64` proxy vs 532.9 MiB/s (39807929475) |
| 19 | Bench verb | self-measuring (seq 4-16384 KiB / rand / open / read / info) | instrument validity | JSON both eras | JSON, `transport:"direct"` | **MEETS** by construction | add 4/64/256/4096 KiB legs (serves #2,#3,#11,#18) |
| 20 | Repair (fix) | sector-span RMW, consent-gated, ODRW | Analyze + execute walls | dry-run exit 0, untimed | dry-run exit 0, untimed | **UNMEASURED** | damaged-fixture fix dry-run, timed, in phase 1 |
| 21 | Apply | table write + IOCTL chain + 3×1 s settle, ODARW | Plan + execute-to-visible wall | never exercised (timeout) | never exercised (5.2) | **UNMEASURED** | unblocks with #11 (same scenario) |
| 22 | DecryptingDevice | passthrough + ≤2-sector staging, XTS per unit | Throughput vs inner device | none | none | **UNMEASURED** | known-FVEK BitLocker fixture |
| 23 | Health | direct IOCTL passthrough (SMART/NVMe/CSMI) | TTFB (first health verdict) | probe ran, untimed | probe ran, untimed | **UNMEASURED** | QPC-wrap the health probe (Run-Benchmark.ps1:170-179) |
| 24 | Enumeration | OPD geometry + volume opens | List wall + per-open ns | bench open 17,019 ns (same open class) | 9,213 ns | **MEETS+/EXCEEDS** (proxy); `rrecov list` wall untimed | stopwatch the list probe (test-benchmark.yml:194-199) |
| 25 | BitLocker unlock | fveapi management op | n/a (no data path) | n/a | n/a | **n/a** (out of perf scope by classification) | — |
| 26 | Destination arming | IOCTL only (≤8 extents) | Arming latency | implicit, untimed | implicit, untimed | **UNMEASURED** (µs-class, low priority) | timing field on recovery open when needed |
| 27 | Session-only modules | no device reads | n/a | n/a | n/a | **n/a** | — |

**Verdict distribution (27 rows): EXCEEDS 4 (orchestrator, exFAT, carve,
writer) + 2 proxy-exceeds (FAT wall, enumeration-open); MEETS 6 (NTFS wall,
table, check, bench verb, + the two proxy-meets inside #4/#24); GAP 1
marginal-unconfirmed (the scenario deep wall +7.1%, carried on row 2's lever);
UNMEASURED 15 (rows 3, 6-11, 16-18, 20-23, 26 — of which 11 are fixture-
absence, 4 are untimed-existing-probes); n/a 2.** Zero confirmed transport
regressions; the single GAP-class signal is one 0.54 s single-sample deep wall
that the clean-volume parity result argues is noise until re-measured.

**The three standing obstructions, in fix-order:** (1) §5.1's one-line stale
gate (every API verdict is currently false-failed on a string); (2) §5.4's
exception-path evidence copy (without it, §5.2's S-PART-DELETE root cause —
the only genuinely broken lane — stays unknowable); (3) §5.3's recall-0 (both
eras — without it, floor gates, the cancel-resume probe, and the writer's
real-payload metric all stay red or meaningless). The §3.1/3.2 CPU-and-memory
transcription needs no new runs at all: the numbers are already in the archived
CSVs and already favor the direct path (−39% CPU per byte, flat ≤40 MB WS).
