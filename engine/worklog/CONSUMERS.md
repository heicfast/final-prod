# CONSUMERS — every disk-read consumer, its exact access pattern, and its performance requirement

> Phase-0 context sheet (driver-replacement session, Task T2-b / Agent B).
> Written by exhaustive read of the tree at the session start commit
> (`619acec2...`). Every code claim carries a `path:line` citation; every
> performance number carries its GitHub Actions run ID and source file.
> Companion sheets: `worklog/ACCESS-LAYER.md` (Agent A),
> `worklog/DRIVER-CONTRACT.md` (Agent C), `worklog/REFERENCE-ACCESS.md` (Agent D).
>
> READ-ONLY RESEARCH: this file is the session's only output. No source was modified.

The one interface everything below sees is `cr::IStorageDevice` — `read_at(offset, buffer, length)`
in bytes, `geometry()`, `absolute_base()`, `is_static_media()`
(include/cr/storage/cr_device.h:125-142). Engines never know which transport they sit on
(cr_device.h:8-14). The current data-path transport is the RDA grammar path
(`\\.\SVC\<token>\<disk>\<part>`, src/win/cr_transport.cpp) with the F11
`Information = Length*512` unscale contract (cr_transport.cpp:53-82;
include/cr/win/cr_rda.h:63-65); the driver-independent classes (health, enumeration,
destination arming, images, BitLocker state) are listed separately in §1.

---

## §1 Consumer inventory table

Legend — Transport entry points (src/win/cr_transport.cpp unless noted):
- **ORP** = `open_rda_partition` (grammar 3-segment; cr_transport.h:64)
- **OD** = `open_disk` (whole-disk grammar view; cr_transport.h:89)
- **ODRW** = `open_disk_rw` (repair write view; cr_transport.h:100)
- **ODARW** = `open_disk_apply_rw` (apply write view + IOCTL chain; cr_transport.h:121)
- **OPD** = `open_physical_drive` (control-path `\\.\PhysicalDriveN` read-only; cr_transport.h:80)
- **IMG** = `image_file_io()` / `storage::open_image` (S-030 images; cr_transport.h:71)
- **VE** = `volume_extent(s)` / `destination_volume_letter` (driver-independent; cr_transport.h:127-152)
- **direct** = `CreateFileW` on `\\.\PhysicalDriveN` in the caller (health only)

| # | Consumer | Module | Transport entry point(s) used | Read / Write |
|---|---|---|---|---|
| 1 | Scan orchestrator (probe, boot hash, table re-read) | `src/core/orchestrator/cr_scan.cpp` | via caller's device (CLI `scan` → ORP; UI `cr_ui_scan` → ORP; `--image` → IMG) | Read |
| 2 | NTFS engine (MFT walk, $Bitmap, USN, GUID dumps) | `src/core/ntfs/cr_mftwalk.cpp`, `cr_bootstrap.cpp`, `cr_bitmap.cpp`, `cr_attributes.cpp` | via orchestrator ReadPipe over the slice | Read |
| 3 | NTFS remnant sweep (deep stage) | `cr_mftwalk.cpp:2283-2364` | via orchestrator ReadPipe | Read |
| 4 | FAT engine (FAT tables, dir chains, LFN) | `src/core/fat/cr_fat_quick.cpp`, `cr_chain.cpp`, `cr_lfn.cpp` | via orchestrator ReadPipe | Read |
| 5 | exFAT engine (1 MiB FAT windows, dentry clusters) | `src/core/exfat/cr_exfat_quick.cpp`, `cr_exfat_dentry.cpp` | via orchestrator ReadPipe | Read |
| 6 | ext4 engine (GDT/inode/bitmap/journal) | `src/core/ext4/cr_ext4_engine.cpp` | via orchestrator ReadPipe | Read |
| 7 | Btrfs engine (chunk tree, node reads) | `src/core/btrfs/cr_btrfs_engine.cpp` | via orchestrator ReadPipe | Read |
| 8 | ReFS engine (page cache, tables) | `src/core/refs/cr_refs_engine.cpp`, `cr_refs_detect.cpp` | via orchestrator ReadPipe | Read |
| 9 | HFS+ engine (B-tree nodes) | `src/core/hfsplus/cr_hfsplus_engine.cpp` | via orchestrator ReadPipe | Read |
| 10 | ZFS engine (labels, uberblocks, block pointers) | `src/core/zfs/cr_zfs_engine.cpp` | via orchestrator ReadPipe | Read |
| 11 | Lost-partition discovery + probes | `src/core/lost/cr_candidates.cpp` | via whole-disk scan view (deep stage, `--whole-disk`) | Read |
| 12 | Carve engine (raw sweep, signatures, validation) | `src/core/carve/cr_carve.cpp` (+ `cr_analysers.cpp`, `cr_rawsigns.cpp`, `cr_usersigns.cpp`) | via orchestrator ReadPipe (carve stage) | Read |
| 13 | Partition-table parser / LDM (scan, fix, apply, carve grouping) | `src/core/table/cr_table_parse.cpp`, `cr_ldm.cpp` | CLI `fix`/`apply`/`scan --whole-disk` → OD; LDM virtual devices read member disks | Read |
| 14 | Check module (pre-scan damage audit) | `src/core/check/cr_fscheck.cpp` | via the raw slice (pre-cache; cr_scan.cpp:1249-1259) | Read |
| 15 | Recovery writer (source reads) | `src/core/recovery/cr_safewriter.cpp` | CLI `recover` → ORP or IMG; UI → ORP; **through `build_read_pipe`** (M-136) | Read (dest via `DestIo` file writes) |
| 16 | Preview / archive list (head-window) | `src/core/preview/cr_preview.cpp` | via `PreviewReader` over the scan/session device or snapshot | Read |
| 17 | Thumbnail (decode-bound) | `src/core/preview/cr_thumbnail.cpp` | via `PreviewReader` | Read |
| 18 | Image creation (S-040) | `src/core/storage/cr_image.cpp` + `src/win/cr_image_create.cpp` | CLI `image` / UI `cr_ui_image_create` → OD (`create_disk_image`, cr_image_create.cpp:174-182) | Read source → **Write** file |
| 19 | Bench verb (the measurement harness) | `src/app/rrecov.cpp:2977-3178` | OD (rrecov.cpp:3037) | Read |
| 20 | Repair engine (RawFixer analyze/execute/undo) | `src/core/repair/cr_rawfixer.cpp` | analyze → OD read view; execute/undo → **ODRW** (rrecov.cpp:3720, 3846; UI cr_ui.cpp:2091) | Read + **Write** (consent-gated) |
| 21 | Apply engine (lost-partition MBR write) | `src/core/apply/cr_apply.cpp` | plan → OD (rrecov.cpp:3284); execute → **ODARW** (rrecov.cpp:3417; UI cr_ui.cpp:2755) + `refresh_disk_properties` (rrecov.cpp:3445) | Read + **Write** (consent-gated) |
| 22 | DecryptingDevice (BitLocker in-stack XTS) | `src/core/storage/cr_decrypt_device.cpp` | wraps an ORP volume device (UI `open_volume_with_key_impl`, cr_ui.cpp:1547-1603) | Read (passthrough+decrypt) |
| 23 | Health (SMART/NVMe/CSMI/TRIM) | `src/win/cr_health.cpp`, `src/core/health/*` | **direct** `CreateFileW(\\.\PhysicalDriveN, GENERIC_READ\|GENERIC_WRITE)` (rrecov.cpp:2769-2772) — driver-independent by classification (cr_transport.h:16-21) | Read (IOCTL passthrough) |
| 24 | Enumeration / disk list | `src/win/cr_enumerate.cpp` | **OPD** + direct volume opens (cr_enumerate.cpp:100-148, 201-228) — control-path, driver-independent | Read (geometry/topology only) |
| 25 | BitLocker unlock (`unlock --turn-off`) | `src/win/cr_bitlocker.cpp` | volume-letter fveapi (no raw disk data path) | management op |
| 26 | Destination arming / overlap refusal | `src/win/cr_transport.cpp` (`volume_extents`, `destination_volume_letter`) | **VE** — driver-independent | Read (IOCTL only) |
| 27 | Session save/load, lastscan, reghive, bcdfix, nvramfix, bootfix, fsck-launch, query, tree, compress, crypto | `src/core/session/`, `reghive/`, `bcdfix/`, `nvramfix/`, `win/cr_bootfix.cpp`, `check/cr_fsck_launch.cpp`, `query/`, `tree/`, `compress/`, `crypto/` | **none** — file/registry/volume-letter only (verified: zero `read_at`/device refs in those translation units) | n/a |

CLI verb → transport map (src/app/rrecov.cpp:4027-4070 dispatch; `require_driver()` gate at
rrecov.cpp:525-574, exit 4 on refusal — rrecov.cpp:55-69):

| Verb | Data-path opens | Driver gate |
|---|---|---|
| `scan` (disk/volume) | `open_disk_view` → **ORP** (rrecov.cpp:603) | yes (rrecov.cpp:1338) |
| `scan --image` | IMG (driver-independent; rrecov.cpp:576-584) | skipped |
| `tree` / `sessions` / `archive-list` | none (session/preview files) | no |
| `recover` | ORP or IMG (rrecov.cpp:2322/2518) + ReadPipe | yes for disk (rrecov.cpp:2271/2454) |
| `health` | direct handle (rrecov.cpp:2769-2772) | no |
| `image` | `create_disk_image` → **OD** (rrecov.cpp:2957) | yes (rrecov.cpp:2887) |
| `bench` | **OD** (rrecov.cpp:3037; open-churn loop OD at :3101) | yes (rrecov.cpp:2982; `--no-rda` REMOVED, :2978-2980) |
| `fix` | analyze **OD** (:3747) / execute **ODRW** (:3846) / undo **ODRW** (:3720) | yes (rrecov.cpp:3218) |
| `apply` | plan **OD** (:3284) / execute **ODARW** (:3417) + `refresh_disk_properties` (:3445) | yes (rrecov.cpp:3608) |
| `driver` | `query_driver_status` only (rrecov.cpp:3903) | n/a (is the gate) |

cr_ui ABI equivalents (src/ui/cr_ui.cpp): `open_disk_impl` → ORP (:1445),
`open_volume_impl` → extent lookup + ORP (:1463-1509), `open_volume_with_key_impl` →
ORP + DecryptingDevice (:1547-1603), scan reads through the same device, recovery builds
`build_read_pipe(dev, 8 MiB, 256 KiB)` (:2192, :2251, :4577 — the M-136 seam), repair
writer **ODRW** (:2091), apply **ODARW** (:2755), image create (cr_ui.cpp:1825).

---

## §2 Per-consumer access pattern detail

### 2.0 The read pipe every engine reads through

Every engine read on the scan path goes through one chain, constructed at one place
(`build_read_pipe`, cr_scan.cpp:164-203, invoked at cr_scan.cpp:1298):

```
PartitionSlice (bounds, cr_slice.h:46)
  → SliceShim (non-owning, cr_slice.h:98)
    → RetryingDevice (retry ×3, Skip-mode zero-fill, backtrack; cr_retry.cpp)
      → CachedReader (8 MiB LRU, 256 KiB blocks; cr_cached.h:50)
        → transport (RDA grammar today / direct tomorrow)
```

- Retry policy: `max_attempts = 3`, `RetryMode::Skip` (surface + zero-fill, never abort),
  `backtrack = true` (the og CatchBuffer shifted-window second chance, step
  `max(0x400, logical_sector)`; cr_scan.cpp:179-183, cr_retry.cpp:83-99).
- Cache: `build_read_pipe(slice, 8ull << 20, 256 * 1024)` — 8 MiB total = 32 × 256 KiB
  blocks (cr_scan.cpp:1298; the cache budget is REGISTERED DIVERGENCE N-045/D-72 — og is
  4 MiB + a prefetch thread; cr_cached.h:9-35). A miss fills the whole 256 KiB block in
  ONE inner `read_at` (cr_cached.cpp:57-61) — the transport's per-request size for the
  engine path is therefore **256 KiB, 64 KiB-aligned** (PERFORMANCE-BASELINE.md:62).
- Error-map accumulation: every un-salvageable range lands in the `RetryingDevice`'s
  `ReadErrorMap` with per-range module attribution (M-049; cr_retry.cpp:103-114),
  merged into `ScanStats.errors` at run end and into sessions (cr_scan.cpp:1397-1400).
- Cancellation: engines poll `CancelToken::checkpoint()` at loop heads (cr_device.h:144-167);
  transports do NOT poll (F-B-16 scope, cr_device.h:146-149).
- The degraded path (cache-creation failure) is the BARE SLICE — pinned contract
  (cr_scan.cpp:170-198; pinned by test_orchestrator).

### 2.1 Orchestrator (probe + identity + table)

- `probe_family`: one **512-byte read at offset 0** (cr_scan.cpp:877-882); a blind probe
  (Skip-mode zero-fill) is distinguished from a clean-but-sigless volume via the error map
  (cr_scan.cpp:1318-1352).
- Volume identity: one **512-byte read at 0** → CRC32 + full snapshot (cr_scan.cpp:1377-1389).
- Whole-disk scans re-parse the table (`table::parse_disk_table`) before discovery
  (cr_scan.cpp:2215-2217) — reads LBA 0 + GPT header/entries (see 2.13).
- Carve-stage grouping re-parses the table + probes each used partition (cr_scan.cpp:2546-2571).
- Pattern class: **tiny random probes**; count O(1) per volume + O(partitions).

### 2.2 NTFS engine

- `NtfsVolume::open`: boot sector (512 B) + backup boot probe at `total - sec`
  (cr_bootstrap.cpp:397, 435); MFT run growth probes **4 KiB windows** at each run's tail,
  forward, until an invalid record (cr_bootstrap.cpp:526-581).
- Main walk (`walk_mft`, cr_mftwalk.cpp:415-524): **one `read_record` per MFT record**
  (record size 1 KiB typical, ≥0x400 by boot clamp) — sequential-in-run but
  record-granular (cr_bootstrap.cpp:599-635). These are the "MFT re-reads" the
  CachedReader exists to survive (cr_cached.h:4-6). Progress every 64 records
  (cr_mftwalk.cpp:484); cancel polled per record (cr_mftwalk.cpp:482-483).
- $Bitmap: loaded whole (`VolumeBitmap::load`, cr_bitmap.cpp; the S-009 free-map truth —
  cr_scan.cpp:1780-1790).
- USN journal harvest: **≤64 MiB** mined (`USN_HARVEST_CAP`, cr_mftwalk.cpp:25, 1279-1293).
- S-012 GUID-dump pass: $DATA chunks of **0x8000 sectors** (32,768 × sector ≈ 16 MiB at
  512 B) with +stride slack (cr_mftwalk.cpp:1457-1477).
- Attribute-list chases read child records explicitly (cr_mftwalk.cpp:579;
  cr_attributes.cpp:204).
- Pattern class: **record-granular metadata walk (1 KiB random-ish reads, ~sequential
  within runs) through the 256 KiB cache** — the cache hit-rate hot spot.

### 2.3 NTFS remnant sweep (deep)

- `scan_mft_remnants` (cr_mftwalk.cpp:2313-2359): whole-volume sweep in
  **4 MiB blocks (`REMNANT_BLOCK`, cr_mftwalk.cpp:231) + 8 KiB..record-size overlap**
  (cr_mftwalk.cpp:2286-2307), sequential; parallel chunk fan-out when threads > 1
  (chunk = `total/threads` rounded to 512; cr_mftwalk.cpp:2335). Unreadable blocks skip
  forward, never abort (cr_mftwalk.cpp:2299-2303). Cancel per block (cr_mftwalk.cpp:2292-2293).
- Pattern class: **sequential bulk scan, 4 MiB reads** (bypasses cache alignment concerns;
  4 MiB is a 64 KiB multiple).

### 2.4 FAT engine

- FAT table load: **whole-table single reads** of FAT1 (fallback FAT2; mirror validation
  and promote; cr_chain.cpp:130-169). Table sizes: FAT12 spf ≤12 sectors, FAT16 12..256,
  FAT32 128 KiB..300 MiB whole-loaded at 512 B sectors (cr_chain.cpp:95-129) — i.e. a
  single `read_at` of up to ~300 MiB is legal on this path.
- Directory reads: chain-followed, **whole-contiguous-run batched reads** with a
  **4 MiB DirBuffer cap** (cr_fat_quick.cpp:26-52, 135). Cancel polled per cluster step
  (F-A1-11, cr_fat_quick.cpp:27-28).
- Deleted-chain reconstruction: FAT-only arithmetic (no extra device reads beyond the
  loaded tables).
- Pattern class: **few very large reads (whole FAT) + run-batched dir reads (≤4 MiB)**.

### 2.5 exFAT engine

- FAT window cache: **1 MiB windows, 1 MiB-aligned** (`win`, cr_exfat_quick.cpp:27-39,
  69-75) — windowed lookups of cluster chains, no total-size cap (og parity).
- Dentry/cluster reads: cluster-granular (cr_exfat_dentry.cpp).
- Pattern class: **1 MiB windowed random** (chain-dependent), cache-fronted.

### 2.6 ext4 engine

- Superblock at 1024 (cr_ext4_engine.cpp:765); backup-SB grid probe capped at
  **4 TiB coverage** (`BACKUP_SB_MAX_GRID`, cr_ext4_engine.cpp:90).
- GDT (group descriptor table): read in **8 MiB chunks** (`GDT_CHUNK`,
  cr_ext4_engine.cpp:839, 844; alt-table same, :868-875); dead chunks zero-fill (tolerant).
- Block/inode-table/bitmap reads: block-granular (4 KiB typical) through the read pipe
  (cr_ext4_engine.cpp:344, 419, 440, 543, 1282).
- Journal harvest: ≤64 MiB read into one buffer (cr_ext4_engine.cpp:989).
- Directory cap: 64 MiB (`MAX_DIR_BYTES`, cr_ext4_engine.cpp:112); the 8 KiB
  superblock-scan window at :712 (`WIN = 8192`).
- Pattern class: **metadata walk (4 KiB block reads) + 8 MiB bulk table loads**.

### 2.7 Btrfs engine

- Node reads: `read_node` = **one read of `nodesize`** (pow2, 0x86..0x10000 — 134 B..64 KiB,
  cr_btrfs_engine.cpp:275-280, 432-439) at logical→physical mapped offsets —
  **random 4-64 KiB metadata reads** (chunk tree, fs trees).
- Superblock mirror probes: 64 KiB-aligned label scan (cr_btrfs_engine.cpp:599).
- Pattern class: **random node-granular reads** — worst case for over-fetch on cache
  misses (256 KiB block fill per miss, 4x over-fetch noted at cr_scan.cpp:1288-1289).

### 2.8 ReFS engine

- Page assembly: **cluster-granular reads per page** (page = 4 clusters at 4 KiB, 1 at
  64 KiB cluster; cr_refs_engine.cpp:80-93, 133-143) with a **bounded page cache keyed
  by first LCN** plus an unbounded live-reachability set (cr_refs_engine.cpp:86-93).
- Pattern class: **random page reads (16-64 KiB units), tree walk**.

### 2.9 HFS+ engine

- Node reads: **one block_size read per B-tree node** (catalog/extents/attributes;
  cr_hfsplus_engine.cpp:145-149 bootstrap node; tree node size from header).
- Pattern class: **random node-granular reads** (like Btrfs).

### 2.10 ZFS engine

- Labels: 4 × 256 KiB label regions (8K pad + 8K bootblk + 112K vdev_phys + 128K
  uberblock ring; cr_zfs_engine.cpp:5-6); every uberblock slot iterated
  (cr_zfs_engine.cpp:26-27). Robustness retries are SKIPPED on static media (N-072,
  cr_device.h:135-141).
- Block reads: `dev->read_at(vdev_offset + off, out.data(), psz)` — **psized
  (ashift-derived, 512 B..16 KiB) random reads** per block pointer (cr_zfs_engine.cpp:540).
- Pattern class: **256 KiB label probes + small random block reads**.

### 2.11 Lost-partition discovery

- `discover` (cr_candidates.cpp:942-1031): sequential **64 KiB WindowScanner** sweep of
  the whole disk (cr_candidates.cpp:32-58, 974-985); per-sector stride checks (stride 1
  default — every sector's last two bytes checked for 55 AA; cr_candidates.cpp:988).
  Read failures count and continue (cr_candidates.cpp:980-983). Cancel per window
  (cr_candidates.cpp:976-977).
- Per-candidate probes: **512-byte boot-sector reads + $MFT record probes** at derived
  offsets (cr_candidates.cpp:78-165) — small random reads.
- Pattern class: **pure sequential 64 KiB whole-disk sweep + sparse 512 B probes** —
  the canonical sequential consumer.

### 2.12 Carve engine (the dominant scan cost)

- Sweep: **1 MiB windows (`WINDOW = 1<<20`) + 4096-byte overlap** (`OVERLAP`,
  cr_carve.cpp:45-46); each window read in ONE `read_at` of up to `WINDOW + OVERLAP`
  (cr_carve.cpp:1349-1390). Progress counts window bytes; unreadable windows are
  SKIPPED, not counted (G-05 honest fill-rate; cr_carve.cpp:1380-1387).
- Validation re-reads: analyser probes re-read candidate extents; hash validation reads
  capped at **8 MiB per file** (`MAX_FILE_READ_FOR_HASH`, cr_carve.cpp:47).
- Raw-sign layer strides at 512 B inside windows, with all-zero strides skipped
  (N-031, cr_carve.cpp:202-204, 956-998); grammar-rejection tracking bounded at 1 Mi
  entries (cr_carve.cpp:48-54).
- Gates: allocation-bitmap skip (M-005), structured-occupancy skip (M-039), dedup tree —
  all reduce bytes actually scanned (cr_scan.cpp:2585-2649).
- Pattern class: **sequential 1 MiB reads, whole-volume**; the deepcarve wall-time
  dominates every scenario (baseline-candidate.json: quick 0.5 s / deep 8 s /
  deepcarve 90 s on the smoke volume).

### 2.13 Table parser / LDM

- MBR: LBA 0 (512 B); GPT: header + entries array (typically 32 sectors, read in one
  chunk; cr_table_parse.cpp:13-26, 41-110); CRC-verified; backup-side salvage.
- LDM: TOCBLOCK/VMDB sector reads + member-disk reads through `LdmVolumeDevice`
  (cr_ldm.cpp:93; multi-member reads chunked).
- Pattern class: **small structured reads at fixed offsets**.

### 2.14 Check module (pre-scan audit)

- `run_fscheck` reads the slice DIRECTLY (pre-cache, post-repair: "must see the same
  bytes repair wrote, uncached"; cr_scan.cpp:1249-1259) — boot sector + family-specific
  structures (cr_fscheck.cpp:120, 994). Read-only; never gates the scan (cr_scan.cpp:1252-1256).
- Pattern class: **a handful of small reads**.

### 2.15 Recovery writer (source reads)

- `recover_item` reads the SOURCE through `read_dev->read_at` in
  **1 MiB chunks (`WriterConfig::read_chunk = 1<<20`, cr_safewriter.h:158)** with an
  exact tail chunk (cr_safewriter.cpp:543-577); hole/logical-gap spans write zeros with
  NO read (S-059; cr_safewriter.cpp:584-650).
- Since M-136 the CLI `recover` and ABI `recovery_open` build the SAME pipe the scan uses
  — `build_read_pipe(src, 8 MiB, 256 KiB)` — so writer reads are retry-isolated,
  Skip-mode zero-filled, and cached (cr_safewriter.h:243-253; cr_ui.cpp:1031-1038;
  diff-to-fix/findings/FIXED/M-136-...md:66). The retry error map is consulted per read
  to implement the og error-9 truncation family (M-135, cr_safewriter.h:136-154).
- Compressed-attribute items read whole packed extents (cr_safewriter.cpp:677).
- LDM-basis items read through the LdmVolumeDevice registered for the node's root
  (S-052; cr_safewriter.h:199-225).
- Pattern class: **sequential per-file, 1 MiB chunks**; the only consumer whose reads
  are user-facing recovery time.
- Destination side: `DestIo` (Win32 `WinDestIo`, src/win/cr_destio.cpp) — temp file +
  atomic rename + fsync; destination-overlap REFUSAL against every armed dest span
  (R7.2/F12; cr_safewriter.h:6-13, 259-264).

### 2.16 Preview / thumbnails

- `PreviewReader::read_at` caps every call at **4 MiB (`kMaxRead`)** and assembles from
  extents (cr_preview.h:59; cr_preview.cpp:19-103); parsers bounded by
  `Caps.max_read` (**4 MiB total** default; cr_preview.h:114-115, 150-153). Head-window
  walks bounded at 256 KiB for stream formats (cr_preview.cpp:403, 636).
- Thumbnails: decode from the same bounded reader; EXIF limit 256 KiB
  (cr_thumbnail.cpp:403-406); pixel guard 64 MP (cr_thumbnail.cpp:1251-1252).
- Pattern class: **bounded head-window reads (≤4 MiB total per item)** — latency, not
  throughput, is the requirement.

### 2.17 Image creation (S-040)

- `create_image_stream`: **64 KiB sequential chunks (`kChunk = 64*1024`)** end-to-end,
  SHA-256 streamed, cancel-checked per chunk (cr_image.cpp:1553-1610). The source is the
  RAW `open_disk` device — **no cache, no retry layer** (cr_image_create.cpp:174-182):
  a failed chunk fails the image (the committed prefix is reported truthfully).
- Pattern class: **pure sequential 64 KiB** — the cleanest bandwidth consumer; the
  transport's 64 KiB envelope is exactly its native shape.

### 2.18 Bench verb (the measurement surface)

- `seq`: sector-aligned buffer, block 4..16384 KiB (`--block-kb`), span 1..65536 MiB
  (`--mb`), sequential `read_at` loop (rrecov.cpp:3116-3143).
- `rand`: LCG offsets in the span (seed `--seed`), 1..100000 iterations, measures mean
  latency µs + IOPS (rrecov.cpp:3145-3174).
- `open`: open/close churn, 1..10000 iterations, mean ns + ops/s (rrecov.cpp:3094-3114).
- `read`: single LBA-0 sector read (contract probe; rrecov.cpp:3070-3092).
- `info`: geometry report (rrecov.cpp:3056-3068).
- QPC timing; `--json` output; driver-only (`--no-rda` removed; rrecov.cpp:2978-2985).

### 2.19 DecryptingDevice (BitLocker volumes)

- Wraps an inner ORP volume device; `read_at` decomposes into **whole-sector staging
  reads** (≤2 sectors of scratch; cr_decrypt_device.cpp:22-50), XTS-decrypts per data
  unit. Geometry passthrough. Pattern class: **passthrough + small staging** —
  throughput ≈ inner transport minus AES cost.

### 2.20 Driver-independent consumers (for completeness)

- Health: direct `CreateFileW(\\.\PhysicalDriveN, GENERIC_READ|GENERIC_WRITE)` +
  `IOCTL_ATA_PASS_THROUGH` / `SMART_RCV_DRIVE_DATA 0x7C088` / NVMe
  `IOCTL_STORAGE_QUERY_PROPERTY` / SCSI-address probe (rrecov.cpp:2769-2772;
  cr_health.cpp:59, 105, 290, 337). No `read_at` path — unchanged by the replacement.
- Enumeration: OPD (`open_physical_drive`) for geometry + direct volume opens
  (cr_enumerate.cpp:100-148, 201-228).
- Destination arming: `volume_extents` (IOCTL, ≤8 extents; cr_transport.h:129-141).

---

## §3 The parallelism model

- Fixed worker pool, MPMC FIFO, cooperative abort (include/cr/cr_parallel.h:40-87;
  implementation src/core/cr_parallel.cpp, 181 lines).
- Thread count: `resolve_threads(cfg_.threads)` — 1 = single; N>1 clamped to 64;
  0 = auto = **hardware_concurrency − 1, clamped 1..8** ("disks, not CPUs, are the
  scan bottleneck"; cr_scan.cpp:58-68; cr_parallel.h:45-47). CLI `--threads N`
  (rrecov.cpp:1546-1547). **No queue depth** — the pool is a task queue of work units,
  not an I/O queue; every underlying transport read is synchronous QD1
  (PERFORMANCE-BASELINE.md:95-99 — the og-matched synchronous QD1 model).
- Work units fanned out (cr_parallel.h:10-15):
  1. **per-candidate lost-partition volume scans** — deep stage, whole-disk basis:
     up to `kLostInsertCap = 10` candidates (the og's cap, cr_scan.cpp:2300-2310),
     each scanned by `scan_candidate` on its own worker (cr_scan.cpp:2362-2428).
     Per-candidate stats are private; merged on the caller thread (F-B-11).
  2. **carve window ranges** — `parallel_for(pool, window_count, worker_job)`, one
     1 MiB window per task (cr_carve.cpp:1540-1594). Workers poll `checkpoint()`
     (pause-aware, G-07) and the pool abort flag.
  3. **remnant-sweep offset chunks** — chunk = `total/threads` rounded up to 512
     (cr_mftwalk.cpp:2331-2350).
  4. **recovery-queue items** are NOT pooled — `SafeWriter::process` is serial over the
     queue (I/O-bound writer; cr_safewriter.h:277-278).
- Cancel semantics: `CancelPoolWatcher` — a background thread polls the external token
  every **50 ms** and aborts the pool so QUEUED work never starts after cancel
  (N-017; cr_parallel.h:106-127). Running jobs finish to their own poll point.
  Used by the carve fan-out (cr_carve.cpp:1582) and the candidate fan-out
  (cr_scan.cpp:2391).
- Implication for the replacement: the transport must tolerate **concurrent callers on
  one shared handle** (per-handle single-flight is the ABI contract; the CachedReader
  mutex serializes the engine path, but the carve/remnant/candidate workers can hit the
  transport concurrently — hence the thread_local bounce in `aligned_read_at`,
  cr_transport.cpp:207-210). Worker count ≤ 8 in practice.

---

## §4 The write paths (exactly what writes, where, under what consent)

### 4.1 Repair (RawFixer) — `fix` verb / `cr_ui_repair`

- **Analyze is read-only** (S1; cr_rawfixer.h:152-158): boot-sector family detection,
  structure validation, plan derivation. Scope (S6): boot sectors, checksums, derived
  geometry, FAT mirror-copy sector restores, $MFT record 0 from $MFTMirr slot 0, and
  `DeepScanBootRestore` (the only boot-shaped candidate in a bounded **64 MiB** window;
  cr_rawfixer.h:92-105). Directory content and FAT entries beyond the mirror are
  NEVER rewritten (advisories only).
- **Execute** (cr_rawfixer.cpp:921-1031): for every action —
  1. coalesce to the **containing sector span** (`sector_bytes()`, 4 Kn-aware;
     cr_rawfixer.cpp:954-960),
  2. snapshot the full span into the CRUNDO01 undo log BEFORE writing (S4;
     cr_rawfixer.cpp:987-995),
  3. `write_at(span_start, span, span_len)` (cr_rawfixer.cpp:1001),
  4. read back + memcmp verify (S5; cr_rawfixer.cpp:1017-1022),
  5. any failure rolls back EVERY applied action in reverse order (cr_rawfixer.cpp:933-952).
- **Volume quiesce (S-010)**: `lock_volume()` before the first write — FSCTL_LOCK_VOLUME
  → DeleteVolumeMountPoint → FlushFileBuffers → settle → verify → FSCTL_DISMOUNT_VOLUME;
  a busy volume = clean refusal `ERR_VOLUME_LOCK_FAILED`, ZERO writes (cr_rawfixer.h:77-89).
- **Consent gates**: the CLI is DRY-RUN by default — nothing is written without `--apply`
  (rrecov.cpp:3808-3818 refusal/empty-plan paths; usage at rrecov.cpp:4007). The
  orchestrator's pre-scan repair calls a `consent` callback before execute
  (cr_scan.cpp:1211-1246; `repair.consent`, denied = scan continues unrepaired). Plan
  offsets are rebased slice→parent before execute (A-002; cr_scan.cpp:1226-1233;
  rebase_plan, cr_rawfixer.h:166-175).
- **Undo** (`fix --undo-apply file`): replays the undo log byte-exact, verified
  (cr_rawfixer.cpp:1034-1053; rrecov.cpp:3697-3743).
- Transport: **ODRW** — RDA handle opened GENERIC_READ|GENERIC_WRITE; unaligned caller
  writes staged through a sector-aligned buffer (cr_transport.h:91-100).

### 4.2 Apply (lost-partition table writes) — `apply` verb / `cr_ui_apply`

- **Plan is read-only** (A1; cr_apply.h:205-230): MBR validation, first free slot,
  containment refusals (LBA 0, past disk end, >32-bit LBA, overlap with live entries,
  GPT in SingleEntry scope), CHS derivation, **sector-0 snapshot** for revalidation.
- **Execute** (A1-A6 sequence, cr_apply.h:232-247): revalidate sector 0 (stale plan =
  `ERR_APPLY_STALE`) → **consent callback** (denied/absent = `ERR_APPLY_CONSENT_DENIED`,
  zero writes, zero locks) → `lock_volume()` (S-010) → revalidate again → optional
  legacy `delete_drive_layout()` → sector-granular snapshot→write→verify transaction →
  device-control chain: legacy `set_drive_layout_mbr` or modern `set_partition_info_ex`
  (the 0x18-byte 0x7C058 record) → 3× settle+`update_properties()` refresh tail
  (failure = `ERR_APPLY_REFRESH_FAILED`, table stays written; D-72c).
- Writes: the 16 MBR entry bytes at 446+slot*16, optionally the 0x55AA signature
  restore; SingleEntry preserves the other three entries byte-for-byte (D-72b superset);
  RebuildTable zero-wipes all four (the og semantics, explicit opt-in).
- **Consent gates**: CLI dry-run by default; `--apply` is the explicit consent
  (rrecov.cpp:3180-3188, 3274); the N-145 cross-run in-flight journal is written beside
  the session/undo artifacts (`inflight_begin/end`, cr_apply.h:263-314; rrecov.cpp:3728-3736).
- **Undo**: `undo_apply` replays the CRUNDO01 log + properties refresh (cr_apply.h:249-255).
- Transport: **ODARW** — RDA RW handle that forwards the corpus IOCTL chain
  (IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100, IOCTL_DISK_SET_DRIVE_LAYOUT 0x7C010,
  0x7C058 record, IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with 3× Sleep(1000);
  cr_transport.h:102-122); plus `refresh_disk_properties` on a direct
  `\\.\PhysicalDriveN` handle (S-029; cr_transport.h:102-110).

### 4.3 Image creation (S-040)

- Writes: the flat-raw image FILE (temp `*.crimg-tmp` → atomic rename; cancel keeps
  `<out>.partial`) + `.sha256` sidecar (cr_image_create.cpp:122-172). Dest-side file
  writes only — **no disk writes**; the disk is only READ (64 KiB sequential, §2.17).

### 4.4 Recovery destination (SafeWriter)

- Writes: recovered FILES on the destination volume via `DestIo` (create temp
  `.part-<id>` → positioned writes → fsync → atomic rename; cr_safewriter.h:40-94).
  No raw-disk writes. **Hard refusal** when any destination span intersects a queued
  item's source extents (R7.2/F12, N-013 multi-extent; cr_safewriter.h:6-13, 261-264).
  Post-commit metadata restoration (SetFileAttributesW + mask-gated SetFileTime) is
  best-effort non-fatal (S-060; cr_safewriter.h:65-93).

---

## §5 Performance requirements and all captured baselines

### 5.1 The hard transport requirements (correctness-of-performance invariants)

1. **The 64 KiB envelope (GAP-S6-13 / mandate §4, non-negotiable)**: every direct read
   whose LENGTH is a multiple of 4 KiB but not 64 KiB takes a **~3.5× slow path**
   (~830 vs ~2900 MB/s on mid-range NVMe). The envelope rounds offset down / length up
   to 64 KiB multiples, clamped at the device end; aligned requests pass through with no
   bounce (include/cr/storage/cr_directalign.h:4-20, 33-73; enforced in
   `aligned_read_at` for BOTH transports — cr_transport.cpp:154-219).
2. **Sector-multiple lengths** (bench run 40073093997 evidence): the Windows disk stack
   rejects ReadFile IRPs whose length or offset is not a logical-sector multiple — every
   recovered file with `size % 512 != 0` was zero-filled "salvaged" before the shim
   (PERFORMANCE-BASELINE.md:49-56; cr_transport.cpp:160-166). 64 KiB subsumes 512/4096/65536.
3. **F11 unscale** on the RDA path: Information = Length×512 (cr_rda.h:63-65;
   cr_transport.cpp:77-81). (Goes away with the driver; recorded as a driver-path fact.)
4. **QD1 synchronous model**: production reads are synchronous QD1 (the og's own model;
   validated size-flat on the runner — PERFORMANCE-BASELINE.md:95-99). No architectural
   queue-depth change is justified by current measurement; QD>1 exists only in the
   alignbench harness.
5. **Handle self-heal**: on ERROR_INVALID_HANDLE(6) the transport re-Creates the handle
   in place once (og parity; cr_transport.cpp:221-253).
6. **Retry semantics** (through RetryingDevice): ×3 attempts, shifted-window backtrack,
   bisection to sector granularity (≤24 depth; cr_retry.cpp:15-18), Skip-mode zero-fill,
   error-map accumulation — a replacement must keep identical observable behavior
   (zero-fill + recorded range), never a different failure mode.

### 5.2 Every captured measurement (with run IDs)

**(a) Driver microbench — run 39807929475** (2026-10-22, windows-2022 runner, VHDX;
rdatest ours-as-eprdtdrv vs original eprdtdrv; PERFORMANCE-BASELINE.md:16-35):

| Metric | Original eprdtdrv | Ours R1 | Ours R2 |
|---|---:|---:|---:|
| Sequential 64 KiB read | 391.0 MiB/s | 371.1 | **532.9** |
| Sequential 1 MiB read | 500.6 MiB/s | 458.6 | — |
| Random 4 KiB IOPS | 8,170 | 7,932 | 7,932 (−2.9%, the cancel-safety+DACL price) |
| Random 4 KiB latency | 122.4 µs | 126.1 | 126.1 µs (+3.0%) |
| Device open (mean) | 8,042 ns | 10,993 | **8,217 ns** (+2.2% = SDDL check) |
| Open churn (mean) | 8,537 ns | 11,331 | **8,532 ns** (parity) |
| Hostile fuzz dispatch | 553,752 IOCTL/s | 707,161 | **841,698 IOCTL/s** (+52%) |
| Data correctness | — | — | 64/64 SHA-256 ranges byte-identical |

**(b) alignbench run 40227375329** (first Windows CI alignbench; windows-2022, 512 MiB
span/cell, two passes checksum-gated, vcpkg-release build, driver v0.2.1; PERFORMANCE-BASELINE.md:73-118):
- Alignment demo (NO_BUFFERING QD1, `\\.\C:`): 128 KiB = **240.0 MiB/s** vs
  132 KiB = **233.6 MiB/s** → ratio **1.03×** (the documented ~3.5× does NOT manifest on
  the runner's virtualized C: — recorded honestly; the 830-vs-2900 figure is mid-range
  NVMe hardware).
- Read-size × QD sweep (MiB/s, `\\.\C:`): 64 KiB QD1 208.6 / QD2 233.1 / QD4 231.1 /
  QD8 235.5; 256 KiB 238.9/240.6/236.1/237.3; 1 MiB 224.4/241.9/241.2/241.3;
  4 MiB 202.8/241.3/238.1/242.4. **Plateau ~240 MiB/s, size-flat, QD-insensitive.**
- RDA grammar-path round-trip (F11 verified on every read): RDA 64 KiB QD1
  **201.9 MiB/s** (8192 reads), RDA 256 KiB **169.3** (2048), RDA 1 MiB **160.5** (512);
  direct `\\.\PhysicalDrive0` 256 KiB QD1 **151.5 MiB/s** — on this runner the DRIVER
  beats direct at the same chunk (169.3 vs 151.5) because its direct-IRP forwarding
  repays the per-read overhead.

**(c) alignbench run 40228880804** (first fully green run): legs 1-3 re-confirmed within
noise (alignment ratio 0.99); cr_ui cheap-op API latency (2M iterations/op):
`cr_ui_abi_version` **10.7 ns**, `cr_ui_last_error` **11.3 ns**,
`cr_ui_carve_format_count` **12.6 ns** (count 174 = 35 grammar + 139 raw-sign),
`cr_ui_carve_format_info` **166.3 ns** (PERFORMANCE-BASELINE.md:120-134).

**(d) alignbench run 40315255368 — THE chunk-strategy verdict (OQ-S4-3 transport half)**
(2026-10-26T11:32Z, push on `095aab5`, image win22, `\\.\C:`, span 512 MiB, qd=1,
RDA service RUNNING, driver-info verified `device_open ok`;
diff-to-fix/tracked-open/OQ-S4-3-chunking-relative-perf.md:137-162; also
RELEASE-NOTES.md:200-211):

| Pair (og buffered-sync vs core) | og MiB/s | og cpu_ms | core MiB/s | core cpu_ms | throughput_ratio og/core | cpu_ratio og/core |
|---|---:|---:|---:|---:|---:|---:|
| og 4 MiB vs core 64 KiB envelope | 197.32 | 609.4 | **203.98** | 593.8 | 0.967 | 1.026 |
| og 4 MiB vs core 256 KiB cache block | 197.32 | 609.4 | **241.64** | 562.5 | 0.817 | 1.083 |
| og 7 MiB vs core 64 KiB envelope | 196.15 | 515.6 | **203.98** | 593.8 | 0.962 | 0.858 |
| og 7 MiB vs core 256 KiB cache block | 196.15 | 515.6 | **241.64** | 562.5 | 0.812 | 0.906 |

Verdict: core 64 KiB envelope MEETS/EXCEEDS og 4/7 MiB chunking (og trails 3.3-3.8%)
at comparable CPU; **core 256 KiB cache block EXCEEDS it by 18-19% at lower CPU**.
`alignment_ratio_64k_over_4k` = 1.001. RDA-path legs confirm through the driver:
64 KiB **194.34** / 256 KiB **237.54** MiB/s on `\\.\rda\…`.

**(e) Comparative smoke run 40073093997-smoke** (ours leg; bench/baseline-candidate.json)
— scan wall seconds (per scenario × mode):

| Scenario | quick | deep | deepcarve |
|---|---:|---:|---:|
| delete_partition | 0.51 s | 8.03 s | 90.06 s |
| permanent_delete | 0.52 s | 7.05 s | 59.87 s |
| quick_format | 0.53 s | 2.01 s | 41.98 s |

(also recall/precision: delete_partition 0.4299/0.1981; permanent_delete
0.8716/0.3026; quick_format 0.0/0.0 — the recorded numbers predate the
GAP-S6-x fixes and stand until the next dispatch.)

**(f) Recovery-quality matrix — runs 40073093997 / 40079065019** (PERFORMANCE-PROFILE.md:29-51;
docs/BENCHMARK-REPORT.md): exFAT native 99.94% recall / 99.16% precision; NTFS
permanent_delete 87.2/30.3; NTFS quick_format 0.0 (fixed 7797bd6, re-measure pending;
og 51.5% on same state); delete_partition 43.0 (fixed 03ddcd2; og 42.7 cap-limited);
decay_50 96.4% / decay_80 0.2% (physical ceiling); FAT32 0-0.3% (fixed 9fc82b0/N-087);
ReFS 0.0% (fixed 7797bd6); recycle 26/26 name recall post GAP-P1-3 (og run 39924932944
26/26 on the same volume).

**(g) Carve CPU-side fix — M-117** (worklog.md:6632-6642): the 220 MB contiguous TS
fixture did **not** complete inside **600 s** pre-fix (quadratic suppress_overlap
re-walk); **11.3 s** post-fix (covered-check reorder, commit `d7ba41f6`). Scan-level
throughput class adjacent to OQ-S4-3.

**(h) Local logic-shape baselines** (Linux container, gcc 14.2 -O2; PERFORMANCE-PROFILE.md:16-27):
full unit suite (225 tests at session-6) **3.04 s wall** / 8.46 s ASan; query/filter on
1M-node session tree **362 ms cold** (31-36 ms under the ASan timing gate, 200 ms
budget); NTFS+ReFS engine fixture walks **22 ms** (synthetic fixtures — not
volume-scale truth).

**(i) TTFR (the still-open OQ-S4-3 half)**: probe harness landed (`TtfrProbe` +
`ttfr_ms=`/`first_files=` log lines, rrecov.cpp; extraction in
`.github/scripts/Run-Benchmark.ps1`); dispatches: run **40289658166** (build-stage
fail), runs **40290109004**/**40290704073** (identical build fails), run **40291877684**
(in flight then), run **40783364141** (stale/dead), run **40956286997** (dispatched
2026-10-30, profile smoke, modes quick/deep/deepcarve — in progress at the last
worklog entry, worklog.md:15118-15128). No TTFR number has been transcribed yet.

**(j) Transitions-per-GiB profile** (code-derived, PERFORMANCE-BASELINE.md:58-69):
engine path ≈ 4,096 transitions/GiB (256 KiB blocks); carve ≈ 1,024 (1 MiB windows);
writer ≈ 1,024 + tail bounces (1 MiB chunks); og ≈ 256-512 (4 MiB device / 7 MiB volume
chunks / 2 MiB MFT batches) — "the og still issues 4-8× fewer transitions per byte
than our engine path", measured to be immaterial on the runner (d) but re-measurable
on fast local NVMe via the weekly dispatch.

### 5.3 Per-consumer performance requirement (the acceptance bar)

The session mandate (worklog/session-driver-replacement-prompt.md:90-106) sets the bar:
**every consumer's throughput, latency, CPU, memory, and cancellation responsiveness
must meet or beat its driver baseline on real Windows hardware.** Concretely, against
the numbers above, the replacement must:

| Consumer | Baseline to meet/beat |
|---|---|
| Scan pipe (engine pass, 256 KiB cache blocks) | ≥ 241.6 MiB/s direct / ≥ 169.3 MiB/s driver-path equivalents at 256 KiB QD1 (runs 40315255368 / 40227375329); 64 KiB envelope shape ≥ 203.98 MiB/s |
| Carve (1 MiB windows) | ≥ 160.5 MiB/s driver-path at 1 MiB QD1 (run 40227375329 leg 3); deepcarve wall-time ≤ 90.06 s on the smoke volume class (40073093997) |
| Remnant sweep (4 MiB blocks) | 4 MiB QD1 ≥ 202.8-242.4 MiB/s sweep class (run 40227375329) |
| Lost discovery (64 KiB sequential) | 64 KiB QD1 ≥ 208.6 MiB/s direct / 201.9 driver-path (runs 40227375329) |
| Recovery writer (1 MiB) | ≥ 224.4 MiB/s at 1 MiB QD1 (run 40227375329); og-trailing tolerated only if ≥ the driver's number |
| Image creation (64 KiB sequential) | ≥ 203.98 MiB/s (64 KiB envelope, run 40315255368) |
| Bench verb | self-measuring; alignment ratio ≥ ~1.0 on virtualized, ~3.5× on real NVMe |
| Preview/thumbnail | head-window ≤ 4 MiB reads; latency-dominated (no captured baseline — record one) |
| Random 4 KiB (ZFS/record paths) | ≥ 7,932 IOPS / ≤ 126.1 µs class (run 39807929475) |
| Open churn (bench `open`, per-volume opens) | ≤ ~8.2-11.3 µs mean per open (runs 39807929475, 40227375329) |
| Cancellation | ≤ one 50 ms watcher tick + current-item poll point (N-017; pinned by test_parallel.cpp:273-330) |

---

## §6 What each consumer needs from the replacement (contract + perf)

Universal contract (every consumer, from cr_device.h:118-142 + cr_scan.cpp build_read_pipe):
`read_at(offset, buffer, length)` returns OK with exactly `length` bytes or an error
code; bounds-checked; safe from worker threads; byte-anchored offsets; geometry via
`IOCTL_DISK_GET_LENGTH_INFO` (exact) + `GET_DRIVE_GEOMETRY_EX` +
`STORAGE_QUERY_PROPERTY` access-alignment (512e/4Kn correct fields at +0x10/+0x14 —
the VHD cache-line regression class, cr_transport.cpp:107-129); sector_count uses the
CEIL form (N-206, cr_device.h:47-60).

| Consumer | Must-have from the direct layer |
|---|---|
| **All engines (via ReadPipe)** | 64 KiB-envelope reads (offset-down/length-up, tail clamp); zero-bounce fast path for already-aligned 256 KiB/1 MiB/4 MiB requests (cr_directalign.h:67-70); per-range error reporting preserved through Retry/Skip zero-fill; error-map identical; `ERR_DEVICE_DISCONNECTED` classification for the GLE-5-family (cr_transport.h:154-183) so N-125 reattach-resume works; self-heal on GLE 6 |
| **NTFS MFT walk** | record-granular 1 KiB random-ish reads fronted by the 256 KiB cache — the cache MUST stay in front of the transport, and the envelope's bounce path must not fire for the (rare) unaligned probe reads without correct results; geometry must report 512e physical 4096 (fixup math + alignment) |
| **FAT** | single whole-FAT reads up to ~300 MiB must succeed (length is a 64 KiB multiple by construction on real sizes — verify tail clamp on odd sizes); mirror-fallback reads at table offsets |
| **exFAT** | 1 MiB-aligned window reads |
| **ext4** | 8 MiB chunk reads; tolerant zero-fill on dead chunks preserved via Skip-mode |
| **Btrfs/HFS+/ReFS/ZFS** | small random reads (4-64 KiB) at arbitrary offsets — the envelope bounce must be correct (read-around) and cheap; ZFS label reads at the 4×256 KiB end regions incl. the device tail; static-media flag passthrough (N-072) |
| **Lost discovery** | 64 KiB sequential whole-disk sweep at ≥ 208 MiB/s class; the CEIL'd addressable bound (N-206) must remain addressable (driver padded the final partial sector — direct handles must reproduce the bound semantics via `sector_count × logical_sector`, cr_candidates.cpp:45-49) |
| **Carve** | 1 MiB+4 KiB reads, sequential, parallel-window concurrency (≤ 8 workers sharing the handle); unreadable windows skip-not-count (G-05) — the transport's failure semantics must let Skip-mode zero-fill preserve that accounting |
| **Remnant sweep** | 4 MiB+overlap reads, parallel chunks |
| **Recovery writer** | 1 MiB sequential per extent through the SAME pipe construction (M-136: `build_read_pipe(src, 8 MiB, 256 KiB)` at CLI recover / ABI recovery_open); exact-tail chunk correctness (512-multiple zero-fill fix class from run 40073093997 must never regress); LDM multi-basis reads |
| **Preview/thumbnail** | bounded head-window reads (≤ 4 MiB); short-read semantics at logical EOF only (cr_preview.h:71) |
| **Image creation** | 64 KiB pure-sequential at ≥ 203.98 MiB/s; NO retry/cache layer today (decide: keep raw for byte-truth or add retry — current contract fails the image on a failed chunk, cr_image.cpp:1584-1592) |
| **Repair (fix)** | GENERIC_READ\|GENERIC_WRITE whole-disk handle; sector-granular read-modify-write + read-back verify; the S-010 volume-lock sequence must be implementable on a direct handle (FSCTL_LOCK_VOLUME needs a VOLUME handle — today the RDA bound handle forwards it; the replacement must reproduce the quiesce, likely per-mounted-volume direct handles as the og did); write path staging for unaligned spans |
| **Apply** | same RW posture + the device-control IOCTL chain (DELETE/SET_DRIVE_LAYOUT, 0x7C058, UPDATE_PROPERTIES + 3× settle) on direct handles; `refresh_disk_properties` is already direct (S-029) |
| **Health** | unchanged — already direct CreateFile + ATA/NVMe passthrough; keep GENERIC_READ\|WRITE on `\\.\PhysicalDriveN` working under admin |
| **Enumeration** | keep the control-path `open_physical_drive` (read-only share) + direct volume opens |
| **Bench** | must measure the NEW layer truthfully: `open` churn (mean ns), `seq` (MiB/s at 4..16384 KiB blocks), `rand` (µs + IOPS), `read` (LBA0 contract), `info` (geometry + is_4kn); `--json` schemas unchanged |
| **CLI/ABI exit codes** | the `require_driver()` gate (exit 4) is REMOVED with the driver — but every other exit code (scan 0/1/3/5, recover 2, apply/fix 2, bench 2, image 2/3; rrecov.cpp:55-69) must be byte-identical |

Threading/lifecycle: per-handle single-flight is the current ABI contract
(cr_transport.cpp:188-191); the thread_local bounce tolerates concurrent callers
(cr_transport.cpp:207-210). Devices are constructed per verb and live for the verb's
duration; the ReadPipe owns the retry+cache chain and must outlive the writer (cr_ui.cpp:1031-1038).

---

## §7 Test coverage per consumer

Unit suites (tests/unit/, 75 files, 1742+ tests at the last session close —
worklog.md:15134; 926 at v0.4.1-parity, RELEASE-NOTES.md:231):

| Consumer | Pinning suites | What they pin |
|---|---|---|
| ReadPipe / orchestrator | `test_orchestrator.cpp`, `test_scan_truth.cpp`, `test_telemetry.cpp`, `test_pause_resume.cpp`, `test_resume_revalidation.cpp` | pipe construction + degraded bare-slice contract, read-truth (no fake scans), stage masks, pause/compensated elapsed, resume revalidation (identity hash) |
| CachedReader | `test_cached.cpp` (incl. `ScanCacheBudgetIsTheRegisteredDivergence`, :109) | hit/miss/bridging, bounds, failed-blocks-not-cached-forever, the 8 MiB/256 KiB budget pin |
| Retry layer | `test_retry.cpp` | ×3 attempts, backtrack, bisection to sector, Skip zero-fill, error-map coalescing, module attribution |
| Direct envelope | `test_directalign.cpp` | exhaustive property sweep of the 64 KiB envelope (`UnifiedEnvelopeExhaustivePropertyBothDevices`) |
| Transport logic | `test_transport_logic.cpp`, `test_driver_status.cpp`, `test_bitlocker_win.cpp`, `test_enumerate_win.cpp`, `test_destio_win.cpp` | F11 unscale + disconnect classification logic, driver-status facets, destination arming |
| Parallel model | `test_parallel.cpp` (`ParallelCarve.MatchesSingleThreaded` :159, `ParallelRemnants.SameRecordsAsSequential` :249, `CancelPoolWatcherDrainsQueueWithinOneTick` :273) | fan-out result equivalence with 1 thread, watcher abort cadence, exception isolation |
| NTFS | `test_ntfs_volume.cpp`, `test_fixup.cpp`, `test_runlist.cpp`, `test_m101_branches.cpp` | boot/validation, fixup, runlists, M-101 branches |
| FAT/exFAT | `test_fat_volume.cpp`, `test_exfat_volume.cpp` | chain walks, LFN, windows |
| ext4 | `test_ext4_volume.cpp`, `test_ext4_m143_journal_inode_image.cpp`, `test_ext4_m144_large_volume.cpp` | GDT chunking, journal salvage, large-volume reads |
| Btrfs/HFS+/ZFS/ReFS | `test_btrfs_volume.cpp`, `test_hfsplus_volume.cpp`, `test_zfs_volume.cpp`, `test_refs_volume.cpp`, `test_refs_ldm.cpp` | node reads, label handling, page walks |
| Lost discovery | `test_lostpart.cpp`, `test_consistency.cpp` | candidate probing, scoring, consistency |
| Carve | `test_carve.cpp` (1570+ assertions class; e.g. `Carve.X3fSectionDirectoryWalkCarvesTrueEnd`, `Carve.TsStreamClampedAt200MB`) | window semantics, formats, overlap ranking, caps |
| Table/LDM | `test_crc32_gpt.cpp`, `test_ldm.cpp`, `test_ldm_recovery.cpp`, `test_slice_devmap.cpp` | GPT CRC salvage, LDM multi-basis |
| Writer | `test_writer_dirs.cpp`, `test_writer_readpipe.cpp` (M-136), `test_report.cpp`, `test_dict.cpp` | 1 MiB chunking, ReadPipe mediation, dir reconstruction, reports |
| Preview/thumbnail | `test_preview.cpp`, `test_thumbnail.cpp` | caps, head windows, decode verdicts |
| Image | `test_image.cpp`, `test_image_create_win.cpp` | reader formats (flat/VHD/VHDX/VMDK), create/commit/.partial sweep |
| Repair | `test_rawfixer.cpp` | S1-S7: consent, refusal, transaction, rollback, verify |
| Apply | `test_apply.cpp` | A1-A6: consent order, revalidation, containment, undo, journal |
| Health | `test_health_grade.cpp`, `test_health_csmi.cpp`, `test_nvme_health.cpp`, `test_fve_policy.cpp` | grading, transports, policy |
| Check | `test_fscheck.cpp`, `test_fsck_launch.cpp`, `test_fsck_win.cpp` | audit report, chkdsk ladder |
| Sessions | `test_session.cpp`, `test_snapshot.cpp`, `test_diskmap.cpp`, `test_vollabel.cpp` | CRC survival, snapshots, map basis |
| Query | `test_query.cpp`, `test_query_perf.cpp` | filters + the 200 ms perf gate |

End-to-end / behavior lanes (no `tests/e2e/` directory exists — the e2e surface is the
CI harness):
- `.github/workflows/test-benchmark.yml` (the surviving truth matrix; windows-2022,
  elevated, prebuilt release + test-signed driver, fixed-VHD matrix, CLI + cr_ui API
  both): recovery across the scenario matrix (permanent delete, recycle, quick format,
  …; FAT32/NTFS/exFAT, MBR+GPT, multi-partition), per-mode scan timings, recovery
  throughput, the bench verb (info/seq/rand/open), process sampling (CPU/WS) —
  artifacts under `01-driver/… 04-scenarios/<id>/{cli,api,state} 07-performance`
  (test-benchmark.yml:1-51).
- `bench/ours/run-ours.ps1`: quick/deep/deepcarve scans with `--out session.crsf
  --log stage.log --json`, tree CSV/JSON, recover report, sampling (run-ours.ps1:1-12).
- `bench/ours/run-behavior.ps1`: (1) driver-absent probe, (2) cancellation
  responsiveness via CTRL_BREAK hard-kill, (3) resume equivalence — killed-then-resumed
  scan must match the uninterrupted tree (run-behavior.ps1:1-113).
- `bench/ours/run-alignbench.ps1` + `verify-driver.ps1`: the transport benchmark
  (alignment demo, size×QD sweep, RDA round-trip, API latency; run-alignbench.ps1:1-34).
- `bench/scenarios/apply-scenario.ps1`, `bench/volume/volume.ps1`,
  `bench/seed/generate_seed.ps1`: the seeded deletion matrix + VHDX construction +
  1 MiB-block before/after hash maps (bench/README.md:26-39).
- `bench/og/run-og.ps1` + `bench/og/og-driver.ps1`: the og comparison leg (`.rsf`
  harvest, Recover-All funnel; og-side TTFR probe still missing — OQ-S4-3's open half).

---

## Appendix — the five most performance-critical consumers (ranked)

1. **Carve engine** — the deepcarve stage is 7-45× the deep stage's wall time on every
   captured scenario (90.06 vs 8.03 s, 59.87 vs 7.05 s, 41.98 vs 2.01 s;
   baseline-candidate.json). Pure sequential 1 MiB+4 KiB reads, parallel windows. Any
   transport regression here dominates the product's perceived speed.
2. **The scan read pipe (CachedReader 256 KiB block fills over all engines)** — the
   256 KiB block is the core's measured best shape (241.64 MiB/s, +18-19% over the og's
   4/7 MiB chunks at lower CPU; run 40315255368); every structured engine rides it.
3. **NTFS MFT walk + remnant sweep** — record-granular random reads (cache hit-rate
   sensitive) + the 4 MiB parallel deep sweep; NTFS is the dominant real-world target.
4. **Recovery writer source reads** — the only consumer whose read speed is the
   user-facing "recovery time"; 1 MiB sequential per extent through the same
   retry+cache pipe (M-136).
5. **Image creation** — 64 KiB pure sequential at the envelope's native size; the
   safe-scan posture's producer (UIUX/11:17) and a pure bandwidth consumer (≥203.98
   MiB/s bar from run 40315255368). (Lost-partition discovery shares the sequential
   profile and the same ~208 MiB/s @64 KiB bar.)
