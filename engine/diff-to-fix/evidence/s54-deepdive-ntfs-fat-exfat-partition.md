# Deep dive D — NTFS/FAT/exFAT/partition: sector math, scan strategy, validators (pin 2794da8b / corpus be0cfe9)

Read-only parity audit, axes: (1) sector/LBA/cluster arithmetic, (2) scan strategy,
(3) detection validators. Exclusion list (`scratch/s54/exclusion-list.md`) and
`docs/PARITY-DELTAS.md` were read first; every candidate below was cross-checked against
both. Corpus cites are `re-workspace/reversed/ghidra-*/..._decompiled.c` line numbers
unless another file is named. Core cites are `src/core/...` + `include/cr/...`.

Already-filed IDs cross-referenced (NOT re-filed): M-123, M-124, M-127, M-130, M-132,
M-119, N-161, N-162/D-119, N-173, A-014, S-058, N-163, N-165, S-052; PARITY-DELTAS
D-04, D-10, D-14(F4–F9), D-39, D-43, D-47, D-49, D-50, D-51, D-52, D-70, D-94, D-99,
D-116, D-119, plus the in-code-landed finding families M-001, M-002, M-012, M-015,
M-042, M-045, M-052, M-054, N-001, N-002, N-010a–f, N-011a–e, N-012, N-021, N-022,
N-034, N-035, N-039, N-040, N-050, N-057, N-067, N-100, S-006, S-009, S-012, S-013,
S-019, S-022, M-013, M-014.

## Coverage log (every function/site walked, corpus + core, with line ranges)

| # | Site walked | Notes |
|---|---|---|
| 1 | exclusion-list.md (full); PARITY-DELTAS.md engine sections (D-04/14/39/43/47/49/50/51/52/70/94/99/116/119/120, section J) | baseline |
| 2 | Core NTFS: `cr_bootstrap.cpp` (all 486), `cr_runlist.cpp` (all 201), `cr_fixup.cpp` (all 74), `cr_attributes.cpp` (all 194), `cr_mftwalk.cpp` (all 2239), `cr_bootstrap.h` (:1-80), `cr_ntfs_wire.h` (all 260) | boot ladder, MFT/mirror streams, runlist codec, walk + dump pass + remnant scan |
| 3 | Core FAT: `cr_fat_quick.cpp` (all 611), `cr_chain.cpp` (all 778), `cr_fat_engine.h` (all 279) | validate_boot, open gates, lost ladder, FatTable, chain walkers |
| 4 | Core exFAT: `cr_exfat_quick.cpp` (all 1133), `cr_exfat_dentry.cpp` (all 201), `cr_exfat_engine.h` (all 136), `cr_exfat_wire.h` (:1-80) | boot region validator, checksums, windowed FAT, entry walk |
| 5 | Core table/lost/orchestrator/storage: `cr_table_parse.cpp` (all 492), `cr_candidates.cpp` (all 1026), `cr_consistency.cpp` (all 187), `cr_scan.cpp` (:755-1050 probe_family + run; :1160-1310 dispatch; :1920-2050 run_deep), `cr_slice.cpp` (all 72) | MBR/EBR/GPT parse, lost discovery + probes + scoring, engine dispatch |
| 6 | Core `cr_ldm.cpp` — LBA-math skim only (:177,:400,:693-700,:796-806,:869-953,:986-1000); full LDM layout math belongs to filed S-052 | boundary documented |
| 7 | og NTFSLib (`ghidra-mo/NTFSLib_decompiled.c`): `CNtfsVolume::LoadNtfsVolume` (:19213-19353 full) | boot ladder first-hand (bps mask, spc decode, mft-lcn gate, record/index decodes, error-path structure) |
| 8 | og NTFSFileSystemAnalyser (`ghidra-mo/NTFSFileSystemAnalyser_decompiled.c`): FUN_180004830/870/8c0/8e0/8f0/900 (:3426-3612), FUN_180005470 (:4259-4342), FUN_180005180 region (:4083-4146, cite-verify) | analyser recognition gates, record/index getters incl. the 0x400/0x1000 zero-defaults, backup-DBR memcmp +0x510 store |
| 9 | og NTFSSearchDLL (`ghidra-searchdll/NTFSSearchDLL_decompiled.c`): FUN_18001acc0 (:21095-21200 dump-walk stride/gates), FUN_1800272b0 (:31530-31660 controller entry: MFT LBA product, record-sector gates, mirror fallback), FUN_18002a780 (:34100-34230 shifted-header salvage), remnant loop (:9480-9560), getters FUN_18003f5d0/630/8b0/940/970/990/9b0 (full bodies) | scan entry arithmetic; spc decode threshold variant; remnant stride 0x200 |
| 10 | og FATSearchDLL (`ghidra-searchdll/FATSearchDLL_decompiled.c`): FUN_180021000 (:26285-26420 quick open + FAT1/FAT2 media-word flavor refinement), FUN_18002f970 (:38037-38090 engine-open string gates), FUN_180007e50 (:5202-5260 flavor brackets), FUN_180012340 (:13160-13230 chain cluster→sector 32-bit product — D-119 pin re-verified) | |
| 11 | og exFATSearchDLL (`ghidra-searchdll/exFATSearchDLL_decompiled.c`): FUN_180001630 (:341-420 recognition: backup-boot re-gate + FAT media-word probes + −12 base shift), FUN_180021e80 (:28166-28220 boot gates), FUN_180021c60 tail (:28140-28164 windowed FAT `& 0x3ffff`), FUN_18001eba0 (:25150-25200 dual-form ROL15 — D-47 pin re-verified), cluster→heap math (:4060-4100) | ROL31 boot checksum search: no hits (see Axis 3) |
| 12 | og PartitionRecoveryD (`ghidra-mo/PartitionRecoveryD_decompiled.c`): FUN_180004610 (:2516-2606 classifier ladder), ctor wiring (:600-760 CBlockParseWin32/CExFat/Bitlocker), CNTFSBlockParser chain (:9790-9960, :10023-10120 FUN_180011560, :10216-10300 FUN_1800119c0/a10/a90/b10/b30/b90/bb0, :10300-10470 OEM helpers) | lost-scan NTFS acceptance: boot-pair + 4-record + mirror-retry |
| 13 | og Partition (`ghidra-mo/Partition_decompiled.c`): per-LBA probe classifiers (:1660-1790 exFAT/ReFS/BitLocker incl. "FSRS" ReFS gate), EBR/extended walk (:940-1100) | EBR link math, 0x42/0xEE flags, single-step cycle guard (D-14/F7 cross-ref) |
| 14 | Artifacts: ntfs-engine--deep-dive.md (all), exfat-engine--deep-dive.md (all), zfs-vdev--mftmirr--calcpartition.md (§2-3), rawsearch-carving--deep-dive.md (§1-3), searchdll-family--architecture.md (grep), fat-exfat-engine-internals.md (grep) | claim index only; all load-bearing claims re-derived from decompile |

## Axis 1 — sector/LBA/cluster arithmetic: comparison (site × og × core × verdict)

### NTFS

| Site | og (first-hand) | core | verdict |
|---|---|---|---|
| MFT LBA (engine open / scan entry) | `FUN_18001a3c0(spc_decoded × mft_lcn + volume_base)` — NTFSSearchDLL FUN_1800272b0 :31605/:31627 (u64 product); PartitionRecoveryD CNTFSBlockParser FUN_180011560 :10048-10049 uses the **RAW spc byte**: `local_60 = mft_lcn * (ulonglong)raw_spc + base` | `dev_->read_at(mft_lcn * cluster_bytes)` — `cr_bootstrap.cpp:101` (u64; cluster_bytes = decoded spc × bps) | parity on direct spc; **raw-vs-decoded spc divergence on shift-form bytes** (F-D7, F-D11) |
| MFT-mirror LBA (walk entry) | same product with `FUN_18003f970` = boot+0x38 ($MFTMirr LCN) — :31595-31596, :31605 (the mirror **bootstrap fallback**, F-D2) | `load_mftmirr_stream` reads mirror record-1 copy at `mftmirr_lcn × cluster_bytes + record_size` (`cr_bootstrap.cpp:288`) — mapping only, no bootstrap fallback | **mirror-as-MFT fallback missing** (F-D2) |
| Record size decode (positive cpr) | analyser FUN_180004830:3438 + NTFSSearchDLL FUN_18003f630 + PartitionRecoveryD FUN_180011a10: `cpr × bps × RAW_spc` (bytes) | `record_size_from_dbr`: `cpr × RAW_spc × bps` (`cr_ntfs_wire.h:225-229`) | parity (matches 3 of 4 og sites; NTFSLib :19324 uses decoded spc — og-internal inconsistency, core picked the majority side) |
| Record size cpr==0 | **default 0x400** — analyser :3435-3437, NTFSSearchDLL FUN_18003f630, PartitionRecoveryD FUN_180011a10 | `record_size_from_dbr` returns 0 → `ERR_BAD_PARAMETER` reject (`cr_bootstrap.cpp:80-82`) | **divergence — core refuses what og defaults** (F-D3) |
| Index-block size, cib==0 | **default 0x1000** — analyser FUN_180004870:3453-3454, PartitionRecoveryD FUN_180011a90 | `clusters_per_index_block == 0 → ERR_BAD_PARAMETER` (`cr_bootstrap.cpp:45-46`) | **divergence — same family** (F-D3) |
| Record size negative cpr | `1 << (-cpr & 0x1f)` unbounded (1 GiB at −30) — all three og getters | P1-4 cap: exponent > 20 → 0 → reject (`cr_ntfs_wire.h:230-233`) | registered hardening (D-52) ✓ |
| spc decode threshold | THREE og forms: NTFSLib :19307 `< 0x81` direct; analyser FUN_1800048c0 :3479 `0x80 < b` shift; **NTFSSearchDLL FUN_18003f5d0 :`(b & 0xF0) == 0xF0` shift (0x80..0xEF direct!)** | `< 0x80` direct else `1 << (-spc & 31)` (`cr_bootstrap.cpp:47-51`) — matches NTFSLib/analyser | og-internal inconsistency; core picked one side, unregistered for the 0xF0 variant (F-D11) |
| mft_lcn sanity gate | `raw_spc × mft_lcn < total_sectors` (i64 product, raw byte, overflow-wrappable) — NTFSLib :19301; **no mftmirr_lcn gate** | `mft_lcn >= total_clusters` reject AND `mftmirr_lcn >= total_clusters` reject (floor boundary; u64, no overflow) — `cr_bootstrap.cpp:67-70` | **core stricter; mftmirr gate is core-only** (F-D7) |
| Walk-entry record geometry gate | record/device_sector even && ≤16 sectors (primary branch :31620-31621), else record/512 even && < 0x11 (fallback :31599-31600); fail → walk refused (0x610016) | none (P1-4 caps at 1 MiB = 2048 sectors) | core superset, unregistered (F-D10) |
| Record→byte addressing (walk) | `uVar8 × param_3` chunk math; per-record `local_c0 / record_size + chunk_base` pseudo-number (:21150-51) | `vcn = record_no × record_size / cluster_bytes; rel = … % cluster_bytes` u64 (`cr_bootstrap.cpp:435-436`) | parity (D-51 stride correction landed) |
| Runlist VCN/LCN decode | high nibble = offset width, low = length width (artifact §7, FUN_180003440 :2378-2384) | low nibble = len width, high = off width (`cr_runlist.cpp:75-76` — SPEC order, artifact prose corrected in-code) | parity of arithmetic; N-163 tolerance delta cross-ref |
| Cluster→byte (extents) | run builder `off = cluster × cluster_bytes` (u64; CExFatDataRunList mirrors CNoneResidentNTFS…) | `e.lcn × cluster_bytes` u64 (`cr_mftwalk.cpp:936-937`) | parity |
| $MFTMirr span | runlist-derived (`InitMFTMirror` writes span; ReadFileRecord gate) — zfs-vdev artifact §2 | `mirr_record_capacity_ = runlist span / record_size`, cap 1024 (`cr_bootstrap.cpp:340-346`) | parity (N-001 landed) |

### FAT

| Site | og | core | verdict |
|---|---|---|---|
| Cluster→sector | `(uint)(cluster−2) × (int)spc + first_data_sector` **32-bit product** — FATSearchDLL :13194 (re-verified) plus :11239, :28172, :39068-80 | full u64 (`cr_chain.cpp:24-28`) | registered deliberate non-adoption (D-119/N-162) ✓ |
| first_data_sector | reserved + numFATs×spf (+ root sectors 12/16); u64 (P1-15) — `cr_chain.cpp:589-598` | og equivalents per-site; the engine open declines only when `total < numFATs×spf` (FUN_180021000 :26316) while core rejects `first_data_sector >= total` | core stricter on the underflow family — cross-ref D-94 item 5 (registered guard) |
| Flavor brackets | engine side: string-driven (FUN_18002f970); bracket fn FUN_180007e50 [1,0xFF5)/[0xFF5,0xFFF4)/[0xFFF4,0xFFFFFF4) | `flavor_from_clusters` <4085/<65524/else; `>= 0xFFFFFF0` reject (M-045) | bracket constants parity (D-94); **string-vs-bracket engine flavor divergence** (F-D4); upper cut 0xFFFFFF4-vs-0xFFFFFF0 is a 4-cluster og-internal split (core matches the lost ladder) |
| FAT table windows | per-flavor spf windows (FUN_180036e90) | M-011 landed windows (`cr_chain.cpp:94-127`) | parity (M-011) |
| Root dir (FAT32) | root-cluster chain walk (FUN_180001d70) | `recurse_dir(root_cluster)` (`cr_fat_quick.cpp:574-577`) | parity (N-050) |
| FAT32 EOC family | mask-24 tolerant (`(raw & 0x07FFFFF8) == 0x07FFFFF8`) — FUN_180021000 :26331-26333 shape tests + chain walker | N-002 landed (`cr_chain.cpp:75-85`) | parity |

### exFAT

| Site | og | core | verdict |
|---|---|---|---|
| Cluster→heap byte | `(cluster−2) × cluster_bytes + heap_base`, u64 — exFATSearchDLL :4082 (`(uVar11 - 2) * uVar16 + heap_base`), end-rounding :4090 | `(u64(c)-2) × cluster_bytes + heap_offset × bps` u64 (`cr_exfat_quick.cpp:146-148`) | parity |
| FAT window addressing | 1 MiB windows, `win + (cluster & 0x3ffff) × 4`, no total cap — FUN_180021c60 :28140-28164 | same (`cr_exfat_quick.cpp:54-69`, S-013 landed) | parity |
| Backup boot position | sector 12 (base+0xC) — FUN_180001630 :355; on mis-anchor, **base −= 12** (:372-378) | `region + BACKUP_BOOT_SECTOR × bps` read; params always from the main boot (`cr_exfat_dentry.cpp:174-177`); no base correction | **base-shift correction missing** (F-D1) |
| Volume length | u64 @+0x48, og gate `!= 0` only (FUN_180021e80) | full u64, `>= 2048`, `× bps <= device size` (`cr_exfat_dentry.cpp:150-152`) | core stricter (F-D12) |
| fat_offset lower bound | og `!= 0` only (:28200 block) | `fat_offset < 24 (BOOT_REGION_SECTORS)` reject (`cr_exfat_dentry.cpp:164-165`) | core stricter (F-D12) |
| Heap/FAT ordering | `fat_offset + fat_length <= heap_offset`, `heap <= volume_length`, `(cluster_count−1) << spc_shift + heap <= volume_length` — :28206-28208 | identical three (:166-171) | parity (EXFAT-4 (d)) ✓ |

### Partition table

| Site | og | core | verdict |
|---|---|---|---|
| EBR link LBA | `next = base + entry.lba_start`; base set from the first link (Partition_decompiled.c :1003-1012); ANY entry (0..3) with type 05/0F/85 links, last wins; single-step cycle guard only | `parse_ebr_chain`: volume = `walk + e.lba_start` (relative to THIS EBR), next = `base + e.lba_start`; visited-set cycle guard (`cr_table_parse.cpp:229-268`) | link math parity; 0x85 = M-124 (fixed) ✓; cycle guard = D-14/F7 ✓; any-entry/last-wins vs core first-wins — malformed-EBR edge, table note only |
| GPT header/entries | FUN_1800078d0 gates + 0x80 stride + saturating alloc (:4530-4541, :4703) | mirrored gates + registered strictness (entry_size==128 refusal, 1 MiB cap, salvage bounds) | M-123 / N-161 / D-116 / M-132 ✓ (no new findings) |
| GPT entry span | og consumption undecoded (OQ-1) | first<=last gate + UINT64_MAX wrap guard (`cr_table_parse.cpp:332-338`) | N-161 fixed ✓ (core honesty doctrine) |
| Partition-entry LBA accumulation | Partition.mo per-LBA probes (exFAT/ReFS/FVE at :1660-1790) | `cr_candidates.cpp` probes (probe_ntfs/fat/exfat/…) | parity of anchor model; per-family gate deltas below |

## Axis 2 — scan strategy: comparison

| Aspect | og | core | verdict |
|---|---|---|---|
| NTFS volume-scan entry | analyser proves NTFS by content (boot gates + record-0 probe FUN_180005180), then the controller FUN_1800272b0: MFT LBA = decoded-spc × mft_lcn + base; primary $MFT bootstrap, **fallback to the $MFTMirr LCN on failure** (:31628); record-size even/≤16 gates; walk/dump two-pass (D-51) | probe_family content-verifies (magic + USA bounds), single engine pass + dump pass + remnant sweep; **no mirror bootstrap fallback** | F-D2 (mirror fallback), F-D10 (record gate) |
| NTFS boot-sector probe | LBA 0 (volume start) via the analyser; backup DBR **verified by memcmp and stored at +0x510 as the REPAIR input** (FUN_180005470 :4309-4327) — never a scan source (D-99) | primary → volume-end backup fallback at open (D-99 registered superset) ✓ | parity per D-99; complement gap = F-D5 (55AA) |
| NTFS remnant/raw scan | 512-byte stepping through cluster-mapped raw space (`uVar13 = 0x200` advance, NTFSSearchDLL :9528); 8-sector jump detection; INDX split-block window (FUN_18000b4c0) | 4 MiB windows, 'FILE'/'INDX' probe at 512-byte strides, 8-KiB INDX overlap (`cr_mftwalk.cpp:1961-2185`) | parity (D-10 full-record salvage superset registered) |
| FAT engine open | FUN_180021000: BPB string gates (FUN_18002f970) → total ≥ numFATs×spf → **read FAT1 first sector (reserved+base) and FAT2 first sector (reserved+spf+base), media-word shape tests, flavor overwrite** | scan_fat: validate_boot (bracket flavor) → boot_passes_open_gates (string gates per flavor) → FAT32 sector-6 backup fallback → table load | F-D4 (flavor source), F-D9 (media-word refinement); sector-6 backup = core design (R5.1/D-04), og engine path has no backup read observed |
| exFAT recognition | FUN_180021e80 structural gates on main → **full gate re-run on backup boot at base+12** → on backup failure, FAT[0]/FAT[1] canonical-media probes at base+fat_offset, then at (base−12)+fat_offset → **base −= 12 correction** | validate_boot_region (main gates; backup OEM+55AA informational only); no FAT probe, no correction | **F-D1** |
| Lost-partition deep scan | PartitionRecoveryD CBlockParseWin32 = {CFatx, CNTFS, CExFat, Bitlocker} + Linux ext; per-block classifier ladder FUN_180004610; grid/stride constants corpus-undecoded (D-50 remainder) | discover(): 64 KiB windows, every sector (stride 1), 55AA+OEM dispatch to family probes, superset anchors (D-50) | parity per D-50; stride = UNRESOLVED-1; NTFS candidate acceptance gates differ (F-D8) |
| Carve stepping (RAWSearchDLL) | registry dispatch per signature (artifact §1-3); window stepping covered by carve-lane findings | `cr_carve.cpp` (out of this dive's three-engine scope; deep-dive E covered) | boundary noted |

## Axis 3 — validators: gate-by-gate table (og gates vs core gates)

### NTFS boot/DBR

| Gate | og | core | verdict |
|---|---|---|---|
| OEM string | analyser/lost-parser: **8-byte "NTFS    "** (FUN_180004900 :3568; PartitionRecoveryD :10346); NTFSLib open: **4-byte "NTFS"** u32 (:19297) | 8-byte everywhere (`oem_is_ntfs`) | parity on the recognition path (the governing layer); NTFSLib 4-byte is og-internal noise |
| 0x55AA signature | **never checked** on the NTFS path (analyser FUN_180004900, LoadNtfsVolume, NTFSSearchDLL getters — no 0x1FE read) | `boot_signature_ok` REQUIRED at engine open (`cr_bootstrap.cpp:385-386`) | **core-only gate** (F-D5) |
| bps | mask `& 0x1ff == 0` only (open) / + `≤0x1000` in the analyser path? — no: the analyser's 0x1000 cap is on RECORD size (:4299) | 512-multiple + [512,4096] + device-match (D-52 registered) | D-52 ✓ |
| BPB zero-byte fields | analyser requires +0x0E/0F==0, +0x10..0x14/0x16/0x17==0, +0x20..0x23==0 (FUN_180004900 :3568-3577); spc != 0 | none of these sweeps | **og gates core lacks** (F-D6) |
| Record size shape | record % 512 == 0 && record ≤ 0x1000 (FUN_180005470 :4292-4299) | none in probe_family; open rejects only 0/sub-sector | **og gates core lacks** (F-D6) |
| Index block shape | != 0 && % 0x1000 == 0 (FUN_180005470 :4300-4307) | != 0 only (open) | og gate core lacks (F-D6/F-D3) |
| spc | != 0 (analyser); any value accepted at open (LoadNtfsVolume decodes non-pow2) | probe_family requires **pow2**; open accepts any | **core-only pow2 gate on the recognition path** (F-D6) |
| mft_lcn | gate `raw_spc × mft_lcn < total_sectors` (open); analyser: no explicit gate (record-0 probe reads it) | `< total_clusters` (both mft and mftmirr) | F-D7 |
| Record-0 probe (recognition) | FUN_180005180: FILE magic + USA count ∈ [1,0x11) + USA offset ∈ [0x2a, 0x2a+0xd7) | probe_family identical bounds (`cr_scan.cpp:796-807`) | parity ✓ |
| cpr/cib zero | **default 0x400/0x1000** (4 og sites) | reject | **F-D3** (D-52 premise correction) |
| Lost-scan acceptance (CNTFSBlockParser) | OEM + (boot-pair cross-check at candidateLBA − total_sectors **OR** 4-MFT-records validation at mft_lcn×raw_spc with **mirror retry**) — FUN_180011150/260/560 | probe_ntfs: OEM + validate_boot_sector; MFT record-0 probe is **scoring-only** (never rejects); no boot-pair, no mirror retry | **core weaker accept** (F-D8) |

### FAT boot/BPB

| Gate | og | core | verdict |
|---|---|---|---|
| 0x55AA | in-gate (engine open :38058-38059 `'U'`/`-0x56`; lost ladder in-gate) | `boot_signature_ok` first (validate_boot), lost ladder first | parity ✓ |
| FS-name strings | `_strnicmp` "FAT16   " +0x36 / "FAT32   " +0x52 (engine open); **case-SENSITIVE memcmp** "FAT32   " in the lost gate FUN_180004b70 | identical split (D-94 landed: engine ci, lost case-sensitive) | parity ✓ (D-94) |
| numFATs | {1,2} | {1,2} + `== 0` hard reject in validate_boot | parity ✓ (D-94) |
| bps | engine: == device sector; lost ladder: 512-multiple, no ceiling (M-045/D-70) | same split, 32768 ceiling registered | parity ✓ (D-70) |
| SPC set | bitmask 0x800000008000808b + 0x80 (engine+lost) | explicit set {1,2,4,8,16,32,64,128} | parity ✓ (D-94 correction) |
| FAT16 root floor / divisibility | ≥224, root×32 % bps == 0 | identical | parity ✓ (D-94) |
| spf != 0 | per-flavor | per-flavor + M-011 windows | parity ✓ |
| FAT media-word shape (open) | FAT1+FAT2 first-sector reads; FAT32 family-tolerant / FAT16 exact 0xFFFFFFF8; **flavor overwrite** | absent at open; M-015 exact-FAT16 whole-table check later (reject) | **F-D9** |
| FSI sector ('RRaA', free-count) | **no FSI validation anywhere** (no 0x41615252 constant in FATSearchDLL/PartitionRecovery) | none | parity (both ignore) — mission question answered |
| Total-vs-geometry | `total < numFATs×spf` decline only | `first_data_sector >= total` reject | cross-ref D-94 item 5 family |

### exFAT boot

| Gate | og (FUN_180021e80) | core (validate_boot_region) | verdict |
|---|---|---|---|
| 0x55AA + OEM | `+0x1FE == 0xAA55` && "EXFAT   " 8-byte | same | parity ✓ |
| MustBeZero +0x0B..+0x3F | 53-byte sweep (4-byte then 1-byte steps) | same 53 | parity ✓ (EXFAT-4 (a)) |
| bps shift / device match | shift ∈ [9,12] && `device_sector == 1 << shift` | same | parity ✓ |
| spc shift bound | `bps_shift + spc_shift < 0x1a` | `> 25 − bps_shift` reject (⇔ same) | parity ✓ |
| numFATs | [1,2] | [1,2] | parity ✓ |
| fat_length / fat_offset / heap / cluster_count / root != 0 | all `!= 0` | fat_offset additionally `>= 24`; cluster_count `≤ 0xFFFFFFF5`; volume_length `>= 2048` + device-fit | core stricter (F-D12) |
| Boot-region checksum (sectors 0..10, skip 106/107/112 of sector 0, fill-repeated at sector 11) | **not validated anywhere in exFATSearchDLL** (no ROL31 loop; ChecksumDBR lives only in RawFixer repair per exfat deep-dive §6) | computed + fill-pattern checked, **informational only** (boot_checksum_ok flag; acceptance gates on the structural rc) | parity of acceptance semantics; core computes extra info (harmless) |
| Backup boot | full gate re-run at base+12; FAT media-word probes; base −= 12 correction | OEM+55AA informational flag only | **F-D1** |
| Entry-set checksum (deleted sets) | dual ROL15 (FUN_18001eba0 :25150-25200 — re-verified byte-for-byte) | identical dual form + oracle | parity ✓ (D-47/M-013) |

### Partition table / probe families

| Gate | og | core | verdict |
|---|---|---|---|
| MBR/EBR 0x55AA, entry-used | 0x55AA required; extended types 05/0F/85 | same (`mbr_signature_ok`, `mbr_type_to_kind`) | parity ✓ (M-124 fixed) |
| GPT header gates | signature/rev/header_size/reserved/position-pair/usable-range/entries-outside/entry_size 0x80 (FUN_1800078d0) | mirrored + registered strictness | M-123/D-116 ✓ |
| ReFS probe | "ReFS" @+3 **AND "FSRS" @+0x10** (Partition_decompiled.c:1745-1759 FUN_180003900) | "ReFS" @+3 only (`cr_scan.cpp:834-835`) | **og gate core lacks** (F-D13) |
| BitLocker probe | memcmp "-FVE-FS-" @+3 + 3-byte version compare (type 0x17 vs 0x11) | memcmp + graded version shape | parity of the gate (S-019 landed) |
| exFAT lost probe | OEM+55AA+volume-length!=0 (Partition.mo FUN_180003750: store volume_length @+0x48) | probe_exfat: OEM + volume_length ≥ 2048 + cluster_count bounds | core stricter (F-D12 family) |
| FAT lost ladder | FUN_180004610 + 48f0/4a40/4b70 brackets (re-verified :2516-2606) | `lost_boot_flavor` | parity ✓ (D-94/N-034) |

## Candidate findings (numbered; severity/module/cites+quotes/gap/impact/fix direction) — EXCLUDING already-filed IDs

### F-D1 · SEVERE · exfat · Lost-anchor correction: the og re-bases an exFAT volume whose scan anchor is the BACKUP boot (base −= 12) after FAT media-word probes; the core scans 12 sectors off

- **Corpus:** `ghidra-searchdll/exFATSearchDLL_decompiled.c` FUN_180001630 (:341-420). After the main-boot gates pass, the og reads the backup boot at `*(param_5 + 2) + 0xc` and re-runs the FULL structural gates on it (`FUN_180021e80(0x180061280,iVar1)` :358-359). When the backup fails AND the FAT at the current base lacks the canonical media words — `if ((DAT_180061280 != -8) || (DAT_18061284 != -1))` :366 (FAT[0]==0xFFFFFFF8, FAT[1]==0xFFFFFFFF) — it probes the FAT relative to a base shifted back 12 sectors: `(*(ulonglong *)(param_5 + 2) - 0xc) + (ulonglong)*(uint *)(param_3 + 0x50)` :372-373, and on a media-word match **shifts the volume base**: `*(longlong *)(param_5 + 2) = *(longlong *)(param_5 + 2) + -0xc;` :377-378.
- **Core:** `cr_exfat_dentry.cpp:104-197` `validate_boot_region` reads main+backup regions, gates only on the main boot's structural fields; `boot_backup_ok` is an informational OEM+55AA flag (:174-177); `scan_exfat`/`probe_exfat` use the boot's `fat_offset`/`cluster_heap_offset` as-is. No FAT media-word probe, no base correction anywhere.
- **Gap (falsifiable):** a lost exFAT volume whose primary boot sector is destroyed and whose scan anchor is the surviving backup boot (anchor LBA = true start + 12): the og corrects the base to LBA−12 and scans the true volume; the core materializes the candidate at the anchor, passes `validate_boot_region` (the backup boot is a full valid copy), then reads the FAT at anchor+fat_offset = true FAT start + 12 sectors and the heap 12 sectors off — garbage chains or read failure.
- **Impact:** full-volume recovery loss (or garbage extents) on exactly the damaged-volume population a recovery product targets; user-observable "og finds it, we don't".
- **Fix direction:** port the recognition ladder into `validate_boot_region` (or the orchestrator dispatch): on backup-boot gate failure, probe FAT[0]/FAT[1] at base+fat_offset, then at (base−12)+fat_offset for the canonical pair {0xFFFFFFF8, 0xFFFFFFFF}, and adopt base−12 (flagged) on match. Verify the PartitionRecoveryD CExFatBlockParser twin carries the same ladder (see UNRESOLVED-4).

### F-D2 · SEVERE · ntfs · MFT-bootstrap fallback to the $MFTMirr LCN when the primary $MFT record-0 read fails; the core refuses the volume

- **Corpus:** `ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_1800272b0. The walk entry bootstraps the MFT at `mft_lcn × decoded_spc + base` (:31627) and **on failure falls back to the $MFTMirr LCN**: `if (iVar5 == 0) goto LAB_180027467;` (:31628) → `lVar11 = FUN_18003f970((longlong)pvVar1);` (:31596, returns boot+0x38 = $MFTMirr LCN) → `FUN_18001a3c0((ulonglong)uVar3 * lVar11 + lVar2, …)` (:31605-31606) — the same chunk-reader call with the mirror LCN. (FUN_18001a3c0 returns nonzero on success, 0 on failure — established from the :31609 `if (iVar5 != 0) goto LAB_1800274f5` continuation.)
- **Core:** `cr_bootstrap.cpp:96-119` `load_mft_stream` reads record 0 at `mft_lcn × cluster_bytes`; magic/fixup/enum failure returns the error and `NtfsVolume::open` returns nullptr (`cr_bootstrap.cpp:412-421`) — after only the DBR-backup fallback (D-99). There is no retry of the MFT bootstrap at `mftmirr_lcn`.
- **Gap (falsifiable):** a volume whose $MFT record 0 (first record of the primary $MFT) is torn but whose $MFTMirr region is intact: the og bootstraps from the mirror's record-0 copy (which carries the same $DATA runlist, so the whole MFT remains reachable) and scans normally; the core refuses the volume (zero structured files).
- **Impact:** whole-volume recovery loss on the torn-MFT population; the og's classic NTFS resilience path (the artifacts document the mirror as "a repair/verification source … exploited deliberately").
- **Fix direction:** in `NtfsVolume::open`, when `load_mft_stream` fails on the primary, retry the record-0 read at `mftmirr_lcn × cluster_bytes` (the mirror's record-0 slot) before giving up; flag `boot_from_mirror` analogous to `boot_from_backup`. Note the og also routes odd/>16-sector record geometry (in device-sector units) through this same fallback branch (:31620-31621) — a 4Kn/4-KiB-record quirk worth pinning in tests.

### F-D3 · MAJOR · ntfs · cpr==0 / index-byte==0: the og's scan path DEFAULTS (0x400 / 0x1000); the core rejects the volume — D-52's "REJECTED" premise is a decompile misread

- **Corpus (four independent og sites with the default):**
  - analyser `NTFSFileSystemAnalyser_decompiled.c` FUN_180004830:3435-3437: `if (*(char *)(param_1 + 0x40) == '\0') { iVar1 = 0x400; }` (record size);
  - same file FUN_180004870:3453-3454: `if ((byte)uVar1 == 0) { iVar2 = 0x1000; }` (index block);
  - `NTFSSearchDLL_decompiled.c` FUN_18003f630 (identical 0x400 default);
  - `PartitionRecoveryD_decompiled.c` FUN_180011a10 (:10231-10247, 0x400) and FUN_180011a90 (:10254-10268, 0x1000).
  NTFSLib `LoadNtfsVolume` :19322/:19331 — cited by D-52 as the rejection — are **store-guards** (`if (local_1014 != 0) { … }`); both fall through to `goto LAB_18001683f` (:19340), the success-exit label. The only paths that log `"It is not a NTFS volume!"` (:19344) are the OEM mismatch, the bps mask, and the `spc × mft_lcn >= total_sectors` gate (:19297-19301).
- **Core:** `cr_bootstrap.cpp:45-46` (`clusters_per_index_block == 0 → ERR_BAD_PARAMETER`) and `:80-82` (`record_size == 0 → ERR_BAD_PARAMETER`, from `record_size_from_dbr` returning 0 for cpr==0, `cr_ntfs_wire.h:234`). Pin test `BootRecordSizeZeroRejected` exists (D-52).
- **Gap (falsifiable):** a torn BPB byte at +0x40 (zeroed) on a standard 1-KiB-record NTFS volume: og scans with 1024-byte records (all four scan-path getters default); core refuses the volume outright (unless the backup DBR is intact).
- **Impact:** whole-volume false negative on a single-byte tear — the recovery product's core population. Also a registry-integrity issue: D-52's WHAT-OG-DOES claim is factually wrong for the scan path.
- **Fix direction:** adopt the og defaults at open (cpr==0 → 0x400; cib==0 → 0x1000), keep the P1-4 exponent cap, and correct the D-52 record (premise correction + re-pin: `BootRecordSizeZeroDefaultsTo1024`). If the rejection is kept deliberately, it must be REGISTERED as hardening with the four og default sites acknowledged.

### F-D4 · MAJOR · fat · Engine flavor selection: og is STRING-driven (FUN_18002f970), core is cluster-BRACKET-driven — a "FAT16   "-string volume under 4085 clusters is scanned FAT16 by the og and FAT12 by the core

- **Corpus:** `ghidra-searchdll/FATSearchDLL_decompiled.c` FUN_18002f970:38044-38046: `iVar4 = _strnicmp((char *)(*param_1 + 0x36),"FAT16   ",8); if ((iVar4 == 0) && … numFATs/spc/bps/root gates …) { *(undefined4 *)(param_1 + 1) = 1; … }` — flavor 1/2 is assigned purely by the FS-name string; there is **no cluster-count bracket anywhere in the engine-open validator** (the bracket function FUN_180007e50 belongs to the lost/CalcPartition side). The quick-open consumer bails only on flavor 0xE (FUN_180021000:26318 `if (local_1040 == 0xe) goto LAB_18002128e;`).
- **Core:** `cr_fat_quick.cpp:480` → `validate_boot` sets `p.flavor = flavor_from_clusters(p.cluster_count)` (`cr_chain.cpp:610`, brackets <4085/<65524/else); `boot_passes_open_gates` then applies the string gate **per the bracket flavor** — for `Flavor::Fat12` the case body is empty by design ("Deliberate core superset … the og's validator has no FAT12 branch", `cr_chain.cpp:658-663`).
- **Gap (falsifiable):** a volume with "FAT16   " at +0x36, valid FAT16 BPB gates, and < 4085 data clusters (e.g. a small FAT16 volume formatted by a tool that ignores Microsoft's bracket rule): og → flavor 1 → 16-bit FAT entries; core → flavor Fat12 → the no-gate branch → **12-bit entry decoding**. Different chains, different extents, different recovered bytes.
- **Impact:** wrong-content recovery (not just detection loss) on string/bracket-disagreeing volumes; the inverse of the D-94-registered FAT12 exemption, which registered only the og's DECLINE of string-less FAT12 volumes.
- **Fix direction:** make the engine flavor string-first when a string gate passes (mirror FUN_18002f970: string match ⇒ that flavor, bracket only as fallback), or at minimum refuse/handle the string-says-FAT16-bracket-says-FAT12 combination explicitly. Keep `flavor_from_clusters` for the lost ladder (where the og also brackets — FUN_180004610 parity is already correct there).

### F-D5 · MAJOR · ntfs · Engine-open 0x55AA signature gate is core-only; the og never checks the DBR signature on the NTFS path

- **Corpus:** no NTFS-side og layer reads +0x1FE: analyser FUN_180004900's gate list (:3568-3577) is OEM + bps-mask + reserved-zeros + spc!=0 + BPB zeros — no signature; `LoadNtfsVolume` (:19292-19353) reads only OEM/bps/spc/total/mft fields; the NTFSSearchDLL getters likewise. (The exFAT and FAT og paths DO check 0x55AA — this is NTFS-specific.)
- **Core:** `cr_bootstrap.cpp:385-386`: `if (!boot_signature_ok(reinterpret_cast<const u8*>(&bs))) rc = ERR_BAD_MAGIC;` — a hard gate before `validate_boot_sector`, with the same requirement on the backup fallback (:395-396). Note the core's own `probe_family` NTFS branch deliberately does NOT check 55AA (og parity, `cr_scan.cpp:769-811`), so dispatch succeeds and then the engine open fails — the divergence is observable as "recognized but unopenable".
- **Gap (falsifiable):** an NTFS DBR with a torn 2-byte signature (0x55AA destroyed) and a torn/absent volume-end backup: og scans the volume fully; core surfaces no structured files (carve only). This is the complement of the registered D-99 backup fallback (which covers torn-primary-with-intact-backup).
- **Impact:** damaged-volume detection loss; inconsistent with the og's documented salvage-over-purity posture ("Fixup failure → record skipped — never aborts the whole walk").
- **Fix direction:** drop the 55AA requirement from `NtfsVolume::open` (keep it on the FAT/exFAT paths where the og has it), or register it as hardening in D-52's kept list with a rationale. `lost::probe_ntfs`'s upstream 55AA gate (the discover loop) matches the og's lost ladder convention and can stay.

### F-D6 · MAJOR · ntfs · probe_family's NTFS gate family is not the og analyser's gate family (missing zero-byte/record-shape gates; core-only spc-POW2 requirement)

- **Corpus (the og analyser's actual gates):** `NTFSFileSystemAnalyser_decompiled.c` FUN_180004900 (:3568-3577): OEM "NTFS    ", `bps & 0x1ff == 0`, bytes +0x0E/+0x0F == 0, **spc != 0 (any value)**, bytes +0x10..0x14/+0x16/+0x17 == 0, bytes +0x20..0x23 == 0. FUN_180005470 (:4292-4307): `FUN_180004830(local_248) & 0x800001ff == 0` (**record size a 512-multiple**), `0x1000 < record → bail` (**record ≤ 4096**), index block `!= 0` and `& 0x80000fff == 0` (**index block a 4096-multiple**), then the record-0 probe. The core's own comment at `cr_scan.cpp:770-779` misattributes these getters to bps/spc ("the boot gates (bps 512-multiple <= 0x1000, SPC pow2 nonzero; FUN_180005470 :4292-4307)") — FUN_180004830/870 are the RECORD/INDEX-BLOCK size getters (verified: they read +0x40/+0x44 with the 0x400/0x1000 zero-defaults).
- **Core:** `cr_scan.cpp:792`: `if (bps >= 512 && bps <= 0x1000 && (bps % 512) == 0 && spc != 0 && (spc & (spc - 1)) == 0 && mft_lcn != 0)` — gates on bps and **spc-pow2** (no og site requires pow2 spc), no zero-byte sweeps, no record-size shape gates, no index-block gates.
- **Gap (falsifiable), both directions:** (a) a volume with non-pow2 spc (3,5,6,… sectors/cluster — accepted by every og layer): og dispatches the NTFS engine; core's probe_family declines → the volume falls through to other families and is never engine-scanned. (b) a spoofed/corrupt DBR whose record 0 happens to pass the magic+USA probe but whose BPB zero fields or record/index shape are garbage: og analyser declines; core dispatches.
- **Impact:** (a) is a detection-rate divergence (false negative on exotic-but-real formatter output); (b) is a false-positive-dispatch divergence (mostly absorbed by the engine's own open, but the og never reaches the engine).
- **Fix direction:** replace probe_family's NTFS geometry gate with the og analyser's actual family (OEM, bps%512, the three zero-byte sweeps, spc!=0 — drop pow2, add mft_lcn!=0 only if kept as registered hardening), plus the record/index shape gates; correct the code comment's getter misattribution.

### F-D7 · MAJOR · ntfs · Core-only mft_lcn/mftmirr_lcn < total_clusters refusal; og gates only mft_lcn (raw-spc product, floor-boundary and overflow differences)

- **Corpus:** `ghidra-mo/NTFSLib_decompiled.c:19301`: `if ((local_104b & 0xff) * local_1028 < local_1030) {` — i.e. **RAW spc byte × $MFT LCN < total_sectors** (i64 product; a corrupt huge mft_lcn wraps and passes). **No mftmirr_lcn gate anywhere** in LoadNtfsVolume or the analyser.
- **Core:** `cr_bootstrap.cpp:67-70`: `if (bs.mft_lcn >= total_clusters) return ERR_BAD_PARAMETER; if (bs.mftmirr_lcn >= total_clusters) return ERR_BAD_PARAMETER;` — decoded-spc cluster count, floor boundary (one cluster stricter than the og's sector-product form at non-integral total/spc), no overflow, and a **core-only mftmirr bound**.
- **Gap (falsifiable):** a torn BPB with mftmirr_lcn ≥ total_clusters (or mft_lcn exactly at the floor boundary): og opens and scans (mirror read just fails/garbage, best-effort); core refuses the whole volume.
- **Impact:** single-field tears refuse the volume — same population class as F-D3. Neither gate is in D-52's registered-hardening list (which names only the 4096 bps cap, P1-4, P1-3, and the bps-vs-device match).
- **Fix direction:** drop the mftmirr bound (or demote it to a mirror-invalidating flag), align the mft bound to the og's product form (decoded spc × mft_lcn < total_sectors), or register both bounds as deliberate hardening.

### F-D8 · MINOR · lost/ntfs · Lost-scan NTFS candidate acceptance is weaker in the core: no boot-pair cross-check, no 4-record MFT validation requirement, no mirror retry

- **Corpus:** `ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_180011150 (:9880-9914): the candidate is accepted only if `FUN_180011260` (boot-pair: reads the sector at `param_3 − FUN_180011bb0(param_4)` = candidateLBA − total_sectors and requires a second NTFS boot with matching fields) **or** `FUN_180011560` (:10023-10074) succeeds — the latter reads **4 MFT records** (`local_70 << 2` bytes) at `mft_lcn × raw_spc + base`, validates them (FUN_180011440), and **retries the same read at the $MFTMirr LCN** (`FUN_180011b30` = +0x38) on failure; both failing rejects the candidate (0xfffffff1).
- **Core:** `cr_candidates.cpp:70-122` `probe_ntfs`: OEM + `validate_boot_sector`; the MFT record-0 probe adds +30 points on success but **never rejects** on failure; no boot-pair check; no mirror retry.
- **Gap (falsifiable):** a garbage sector with a forged "NTFS    " OEM and a plausible BPB: the og's lost scan rejects (no boot pair, no valid 4-record MFT); the core surfaces a low-scoring candidate (which then materializes and fails at engine open). Conversely, a candidate with a torn primary MFT but intact mirror passes the og's probe and fails the core's (scoring only).
- **Impact:** lost-list noise differences (FP direction) and score differences; no wrong-content risk (the engine still validates).
- **Fix direction:** make the record-0 probe a soft requirement (reject when both the primary and mirror record-0 reads fail), optionally add the boot-pair cross-check as scoring evidence.

### F-D9 · MINOR · fat · Quick-open FAT1/FAT2 media-word flavor refinement (two extra reads + shape tests) not ported

- **Corpus:** `ghidra-searchdll/FATSearchDLL_decompiled.c` FUN_180021000:26325-26350: after the BPB gates, reads one sector at `reserved + base` (FAT1 start) and tests `local_1038 == 0xffffffff` / `(FAT[0] & 0xfffffff) == 0xffffff8 && (FAT[1] & 0xffffff8) == 0xffffff8 || (FAT[1] & 0x7fffff8) == 0x7fffff8` → flavor 2; else `FAT[0] == 0xfffffff8` → flavor 1; then repeats the shape test on a second read at `spf + reserved + base` (FAT2 start) and **overwrites the flavor again** (last word wins).
- **Core:** no equivalent at open; the flavor comes from the bracket (F-D4) and M-015's whole-table validation later — whose **exact** FAT16 media check (`media != 0xFFFFFFF8u → invalid`, `cr_chain.cpp:197-202`) refuses volumes the og would have re-flavored to FAT32 and scanned.
- **Gap (falsifiable):** an internally inconsistent volume (string/bracket say FAT16, FAT media words FAT32-shaped): og re-flavors to FAT32 and scans; core validates the FAT16 table, fails the exact media check, and refuses the volume (ERR_BAD_MAGIC) unless the mirror validates.
- **Impact:** narrow (torn/hybrid volumes), but the outcome is volume-level refusal vs og recovery.
- **Fix direction:** port the two media-word reads as a flavor-refinement step after `boot_passes_open_gates`, or relax M-015 to family-tolerant media forms when the shape test indicates the other flavor.

### F-D10 · MINOR · ntfs · Walk-entry record-geometry gate (even sector count, ≤16 sectors) missing in the core — core is a superset

- **Corpus:** `NTFSSearchDLL_decompiled.c` FUN_1800272b0: primary branch `uVar8 = record_bytes / device_sector; if (((uVar8 & 1) != 0) || (0x10 < (byte)uVar8)) goto LAB_180027467;` (:31620-31621); fallback branch `uVar7 = (record_bytes + …) >> 9; if (((uVar7 & 1) == 0) && ((byte)uVar7 < 0x11))` (:31599-31600); failing both → `uVar8 = 0x610016` — the walk refuses the volume.
- **Core:** no equivalent; P1-4 caps record size at 1 MiB (2048 sectors) and non-pow2 sizes pass `validate_boot_sector`.
- **Gap (falsifiable):** a volume with a 1536-byte record (3 sectors, odd) or ≥ 17-sector records: og's walk refuses (0x610016); core scans. Also note the og accepts NON-POW2 even sizes (3072/5120/6144 bytes) — parity there.
- **Impact:** core superset (recovers volumes the og declines); unregistered divergence in the accept direction.
- **Fix direction:** register as deliberate non-adoption (like D-119's numeric-hygiene family) or port the gate into `walk_mft`'s entry (not into open — matches the og's layering).

### F-D11 · MINOR · ntfs · spc shift-form threshold: NTFSSearchDLL decodes at `(spc & 0xF0) == 0xF0`, core/NTFSLib/analyser at ≥ 0x80/0x81 — og-internal inconsistency, core picked one side unregistered

- **Corpus:** `NTFSSearchDLL_decompiled.c` FUN_18003f5d0: `if ((*(byte *)(param_1 + 0xd) & 0xf0) == 0xf0) { uVar1 = 1 << (~*(byte *)(param_1 + 0xd) + 1 & 0x1f); } else { uVar1 = (uint)*(byte *)(param_1 + 0xd); }` — bytes 0x80..0xEF are taken as DIRECT sector counts (e.g. 0x85 → 133), unlike NTFSLib :19307 (`bVar2 < 0x81`) and the analyser FUN_1800048c0 (`0x80 < bVar1`) which shift-decode them; PartitionRecoveryD's block parser uses the RAW byte with no decode at all (FUN_180011560 :10048-10049).
- **Core:** `cr_bootstrap.cpp:47-51` — shift at ≥ 0x80 (D-52 registered this as *the* og form, citing LoadNtfsVolume only).
- **Gap (falsifiable):** a volume with an spc byte in 0x80..0xEF and positive cpr: four layers compute three different MFT addresses/record sizes; the core matches NTFSLib but not the scan controller that actually drives the og's MFT reads.
- **Impact:** deep edge (shift-form spc implies ≥ 256-sector clusters); wrong MFT addressing only on that geometry.
- **Fix direction:** document the og-internal inconsistency in D-52 (the 0xF0 variant at FUN_18003f5d0), keep the current decode, add a pin test asserting the chosen convention.

### F-D12 · MINOR · exfat · Core boot-gate strictness beyond the og (fat_offset ≥ 24, volume_length ≥ 2048 + device-fit, cluster_count ≤ 0xFFFFFFF5)

- **Corpus:** `exFATSearchDLL_decompiled.c` FUN_180021e80 (:28180-28208): requires only `fat_offset(+0x50) != 0`, `fat_length(+0x54) != 0`, `heap_offset(+0x58) != 0`, `volume_length(+0x48) != 0`, `cluster_count(+0x5c) != 0`, plus the ordering/fit inequalities.
- **Core:** `cr_exfat_dentry.cpp:150-152` (`volume_length < 2048 || > u64max/bps || × bps > device size`), `:158` (`cluster_count > 0xFFFFFFF5`), `:164-165` (`fat_offset < 24` → reject).
- **Gap (falsifiable):** a (garbage) boot sector with fat_offset ∈ [1,23], or volume_length ∈ (0, 2048) sectors, or cluster_count ∈ (0xFFFFFFF5, 0xFFFFFFFF]: og accepts into the engine (scans nonsense); core refuses.
- **Impact:** nonsense-BPB noise only; core stricter in the FP-suppression direction.
- **Fix direction:** register the three extra bounds as deliberate hardening (the D-70 precedent) or relax to the og's `!= 0` forms; no behavior change recommended.

### F-D13 · MINOR · orchestrator/refs · ReFS recognition lacks the og's second signature ("FSRS" @+0x10)

- **Corpus:** `ghidra-mo/Partition_decompiled.c` FUN_180003900:1745-1759: `else if ((*(int *)(param_2 + 3) == 0x53466552) && (*(int *)(param_2 + 0x10) == 0x53525346))` — "ReFS" at +3 **AND** "FSRS" at +0x10 (then volume length stored from +0x18).
- **Core:** `cr_scan.cpp:834-835`: `if (std::memcmp(sec + 3, "ReFS", 4) == 0) return FsFamily::Refs;` — single signature.
- **Gap (falsifiable):** a sector with a forged "ReFS" OEM and no FSRS marker at +0x10: og declines the ReFS classification; core dispatches the ReFS family (engine then finds nothing — noise, not wrong content).
- **Impact:** minor FP-dispatch surface; also the og's probe captures the volume length (+0x18) at classification time, which the core's probe_family does not.
- **Fix direction:** add the 0x53525346 check at +0x10 to probe_family's ReFS branch (one line).

## UNRESOLVED (precise next probes)

1. **Deep-scan grid/stride constants (PartitionRecoveryD).** D-50's own remainder notes the og's deep-scan grid is undecoded; the core strides 1 sector with 64 KiB windows. Next probe: in `ghidra-mo/PartitionRecoveryD_decompiled.c`, find the loop driver that feeds the block parsers (callers of the CBlockParseWin32 parse dispatch — the vtable slot reached from the scan controller; the classifiers are constructed via FUN_180004070/FUN_1800040b0 at :2207/:2218's callers) and read the LBA increment constant + read-ahead window size. Look for a `+ N` advance on the sector cursor between FUN_180004610 invocations.
2. **LoadNtfsVolume's return value on the decode-guard fall-through.** My structural read (F-D3) shows spc==0 / record-0 / index-0 fall to the success label without storing fields; the return is `extraout_EAX` (undecoded). Next probe: disassemble the tail of `CNtfsVolume::LoadNtfsVolume` (VA 0x1800165d0 region, the LAB_18001683f path) to see which EAX value each path returns — this decides whether NTFSLib's open truly accepts cpr==0 (my finding assumes the scan-path getters govern, which four sites support).
3. **CNTFSBlockParser boot-validator branch content (FUN_180011bf0).** Ghidra elided the branch on the `memcmp(local_215,"NTFS    ",8)` at :10346/:10365 — whether it also gates 0x55AA or bps is undecoded. Next probe: raw disassembly of 0x180011bf0/0x180011e30 in PartitionRecoveryD (the lost-scan NTFS boot check) to enumerate the full gate set before relying on F-D8's gate inventory.
4. **The exFAT backup/base-shift ladder in PartitionRecoveryD's CExFatBlockParser.** F-D1 is proven on the exFATSearchDLL side (FUN_180001630). Next probe: walk CExFatBlockParser's parse chain (vtable installed by FUN_180001e60, `ghidra-mo/PartitionRecoveryD_decompiled.c:732` region) for the same `−0xc` base-shift pattern and the FAT[0]/FAT[1] constants (0xFFFFFFF8/0xFFFFFFFF) to confirm the lost-scan side carries the twin.
5. **FAT quick-open flavor-refinement reachability matrix.** FUN_180021000's media-word overwrite can flip an accepted flavor; the exact set of inputs where the second (FAT2) read's shape test disagrees with the first is not fully mapped. Next probe: trace `*param_5` (the flavor out) through the two reads' LAB_18002124b/LAB_180021269/LAB_18002128e edges and enumerate which edge combinations leave flavor 0/1/2 unchanged vs overwritten, to bound F-D9's population precisely.
6. **Whether any og layer validates the exFAT boot-region checksum on a scan path.** Verified absent in exFATSearchDLL (no ROL31 loop; only RawFixer's ChecksumDBR per the artifact). Next probe: grep PartitionRecoveryD's CExFatBlockParser chain and the ExFatFileSystemAnalyser (`ghidra-mo/ExFatFileSystemAnalyser_decompiled.c`) for a `((sum & 1) << 31) + (sum >> 1) + byte` loop over 11 sectors — confirms the core's informational-only treatment is full parity.
