# SCAN-ENGINE-DELTA.md — the Scan Engine Inventory / Delta Report (Agent 2, scan-engine scope)

**Scope:** the scan subsystem only — quick scan, advanced/deep scan, the quick⇄advanced
combination/transformation layer, scan modules/architecture, and scan formats & parameters.
API contracts (Agent 1), non-scan subsystems, NAS/Cloud Recovery (excluded from the unified
driver by design), and the Fixo file-repair chain are out of scope except where they
touch the scan pipeline.

**Method:** three parallel context agents (research corpus deep-dive; core_recovery state
audit; rda-driver scan-surface audit), then first-hand re-verification of every claim this
report acts on (decompile reads in the corpus; file reads in core). Every item below is
marked **present | missing | inconsistent | hallucinated** in core_recovery with the
exact gap. Deliberate registered deltas (PARITY-DELTAS) are marked **present (registered
delta)** — they are not defects.

**Sources:** corpus `architecture/04-recovery-workflow.md` + `05-scan-engine-architecture.md`
+ `re-workspace/artifacts/SearchDLL/*` + `SaveScan/*` + `config-files/*` + the Ghidra
decompiles (`re-workspace/reversed/ghidra-searchdll/`, `ghidra-local/`, `ghidra-drwui/`); core
`src/core/orchestrator/cr_scan.{h,cpp}` + per-FS engines + `diff-to-fix/*`; driver
`rda-driver` shared headers + docs.

**Session re-verification (SCAN-2f round 4 + SCAN-3, 2026-10-28 — two convergent
A2 legs):** context recovery re-run per protocol — SCAN-2f's four read legs
(corpus carve-family census + dual registry; corpus NTFS decompile trace; core
carve-state audit; rda-driver scan-surface audit) and SCAN-3's four agents
(research quick/advanced spec + the NTFS +0x64 decode probe; research carve
census; core state audit; rda-driver scan surfaces). **Both legs independently
decoded OQ-SA-1's NTFS +0x64 to the same value** (first $DATA run start LCN,
low dword); every load-bearing claim below re-verified first-hand at the
decompiles (the +0x64 producer chain re-read at `NTFSSearchDLL_decompiled.c`
:5105-5131/:1986-1988/:41028/:2264-2265; the DRWUI twin serializer re-read at
`DRWUI_decompiled.c`:853463-853464; the carve sub-item FIXED states re-verified
against the tree). See the session-3 resolution log for the full decode chain
and census table.

---

## 1. QUICK SCAN

| # | Og behavior (research) | Core state | Verdict |
|---|---|---|---|
| Q1 | Plugin/factory contract: `EUCreate<FS>QuickSearcher` per DLL, FS codes 0-2 FAT/3 NTFS/4-5 EXT2/7 exFAT/8 HFS/9 EXT4/10 ReFS/0xB Btrfs/0xD ZFS; dual dispatcher FUN_140577d40 + FUN_14057afd0; `scanext4` (shipped=1) / `scanzfs` (shipped: absent) gates in config/version.ini | Single in-process dispatch table (ARCH-DUP-4); gates dispositioned I-016 ("policy question, not a defect"); FS-code spaces never equated | **present (registered delta)** |
| Q2 | NTFS quick: $MFT bootstrap (DBR→64 KiB batch, fixup "FILE" magic), record walk with base-record skip (CORRECTIONS #2), `$`-name/temp/flags&4 filters, deleted-viability: strict `size > 0x3FF` bitmap walk (CORRECTIONS #7), `intact==total → destroy`, cancel per record; `fastsearch` config default ON | `ntfs::walk_mft` single pass, `WalkerFlags` mirror every gate; viability 0x3FF strict (`test_ntfs_volume` boundary test); `destroy_fully_overwritten=false` documented as og pass-1 parity (D-51/D-56/D-57 register the two-pass model) | **present** |
| Q3 | FAT quick: threaded worker, FAT12/16/32 + CDoubleFATTable, EOC/BAD family masks, deleted 0xE5 entries + LFN reassembly, `.`-entry directory identity, visited-range cycle guard, chain sanity `< 0x10000`; FAT12 BAD 4087 raw (CORRECTIONS #3) | `fat::scan_fat` flags-toggle complete; LFN glue + checksum reversal, cycle guard + depth 64, chain cap 0x100000 (core constant registered), og gate ladder N-034; FAT12 raw-BAD registered delta | **present** (chain cap value registered) |
| Q4 | exFAT quick: 0x8E0 object, allocation-bitmap build during walk, dentry triples (0x03/0x05/0x40/0x41 + InUse bit 0x80), backward triple reconstruction, upcase validation | `exfat::scan_exfat`; M-013 dual-checksum oracle landed; S-013 windowed FAT (1 MiB windows, no cap); EXFAT-2 salvage; N-067 upcase | **present** |
| Q5 | ext4 quick: SB magic 0xEF53@+0x38 + structural gates (`block_size = 0x400 << (log&0x1F)` etc.), GDT 32/64 B, inode walk + journal harvest gated on 0xC03B3998 + blocktype {3,4} (CORRECTIONS #6) | `ext4::scan_ext4`; S-027 backup-SB rescue registered (backup probing unverified-in-engine, CORRECTIONS #4); journal differential harvest D-93 | **present** |
| Q6 | HFS+ quick: QuickSearchFiles = FUN_180001760 (partition ctx → virtual volume reader → tree attach → DetourDirectoryTree unwrap → build), catalog/extents/attributes B-trees, H+/HX sig, error ladder 0x5002xxxx | `hfsplus::scan_hfsplus`; S-002 decmpfs/named-forks, S-021 journal; error space D-68 (core-internal numerics registered) | **present** |
| Q7 | ReFS / Btrfs / ZFS quick: per-engine metadata walks (page signatures; superblock mirrors + newest generation, no uuid check (CORRECTIONS #8); uberblock ring txg-gated) | `refs::scan_refs`, `btrfs::scan_btrfs` (D-06 newest-gen), `zfs::scan_zfs` (S-034) | **present** |
| Q8 | Quick is metadata-only + used-bitmap reasoning; no sector-by-sector content pass; same StartSector range reused by deep | Single-walk quick/deep (D-36 registered); stage telemetry + weighted progress 40/35/25 | **present (registered delta)** |
| Q9 | Quick-scan error paths: HRESULT-style 0x5002xxxx facility ladder (89 distinct constants in HFS+ engine; "Return Error:0x…Fuction:" verbose channel); record-skip salvage semantics | `cr::Err` families 0x5002/0x5003/... with salvage-over-purity; numeric values core-internal (D-68 registered); read-truth gate ERR_READ_FAILED (anti-fake-success, stronger than og) | **present (registered delta)** |
| Q10 | FAT 8.3 names persist as raw OEM-codepage bytes (part1:121; control-byte names survive verbatim) | **FIXED (8451913, other lane, verified by A2)**: `short_name_to_u16` CP437 boundary decode (`kCp437[128]`), corpus-cited, 0xE5-name-byte semantics preserved | ~~inconsistent~~ **present (fixed)** |
| Q11 | IFileProperties anchor-datum fields: FAT +0x3e first-cluster origin kept for chainless deleted files; exFAT +0x50; NTFS +0x64 | **FIXED (s2, A2)**: `fat_origin_cluster` carrier stamped from the dentry/stream-dentry independent of chain resolution (FAT/exFAT emission decode: FUN_180012e60's unconditional closing stamp); persisted as nflags bit-7; runtime fail-before proven. **NTFS +0x64 DECODED (SCAN-2f + SCAN-3 convergent, instruction-decisive — see the session-3 resolution log)**: the low dword of the STARTING LCN of the file's FIRST $DATA run — NOT the parent reference, NOT the own record number, NOT a type id. The $FILE_NAME path (FUN_18002a340 :33912) zeroes the slot; the value is stamped by the $DATA-attach phase: FUN_1800069e0 :5126-5127 (`raw+0x53 <- (u32)out1` = the runlist encoder's first-run start, FUN_1800336f0 :41028; `raw+0x4f <- (u32)out2` = Σ run lengths, sparse-zeroed :41096-98) -> FUN_180003020 :1987-1988 (`+0x64 <- raw+0x53`, `+0x60 <- raw+0x4f`); attach twin FUN_180033250 :2264-2265. **Companion engine-side +0x60 = TOTAL DATA-RUN CLUSTERS**; the "+0x60 type-registry id" census reading (part1:86) is refuted three ways (encoder outputs at every writer site; the +0x20 vtable getter is a return-0 stub for 8 of 10 per-FS classes — a Type-column feed cannot be NTFS-only; the census's per-extension value multiplicity is allocated-clusters behavior). The DRWUI twin serializer (FUN_140520d20, DRWUI:853463-853464) persists the ENGINE-POPULATED +0x64/+0x60 verbatim — no wire-side type-id overwrite exists at that site (SCAN-2f's "DRWUI wire-side overwrite" phrasing corrected by SCAN-3's re-read). Family corroboration: FAT +0x3e / exFAT +0x50 / NTFS +0x64 = the vtable+0x30 first-data-location anchor triple. Resident-$DATA caveat (SCAN-2f): the og's resident wrapper carries a synthetic pair {walk-context anchor, 1} — MFT-resident files carry the wrapper anchor, not a real LCN; the core's resident path stamps 0 (the honest no-runs value). **OQ-SA-1 CLOSED on this decode; the core carrier `ntfs_origin_lcn` + nflags bit-8 LANDED (1285be3: both walk stamps + clone carry + bit-8 persistence; fail-before proven; suite 1010/1010 then 1012/1012 merged)** | **present (fixed — all three anchor halves decoded, carried, persisted)** |

## 2. ADVANCED / DEEP SCAN

| # | Og behavior | Core state | Verdict |
|---|---|---|---|
| A1 | FUN_140581500 creates full searcher set in order NTFS→FAT→exFAT→ZFS(gate)→HFS→ReFS→RAW(settings+0x60), `std::list`, final RAW naming callback vt+0x48 = FUN_140161fb0 | Stage model quick→deep→carve (O37 M-equivalent); composition deltas registered S-006/M-005; RAW naming = `raw_category_name` (M-029 `filter_generalraw`) | **present (registered delta)** |
| A2 | Deep = complete FS behaviors (NTFS $Bitmap+USN+orphans+MFTMirr; FAT deleted chains + cross-FAT; HFS+ freed-node sweep; ReFS harvest) + same StartSector as quick | `deep_extras` flags per engine + single-walk (D-36); NTFS remnant sweep R2.5; two-pass S-012 | **present (registered delta)** |
| A3 | Lost partitions: PartitionRecovery CalcPartition scoring, `Insert lost partition %s…`, sector size 512 default | `lost::discover` + `materialize` + parallel candidate scans; scoring matches the 187 CalcPartition.log rows (M-017); exFAT content-consistency reject (M-035) | **present** |
| A4 | RAWSearchDLL carving: ~106 analyser classes (131 roster rows = 106 analysers + 7 FileInfo result classes + 18 infrastructure — count reconciled SCAN-2f; the "190+" was a stale approximation), dual registry (u32 fourcc RB-tree + u16-key multimap + list layer), 6 families, two-generation fallback chains, raw_signs.json (139 ext) loaded at engine init, RawConfig.ini 34 per-format toggles, ZIP 5,242,881 B padding cap, global carve cap 0xBEBC200 (200,000,000 B, singleton +0x70), PST/RAR/MKV/HEIC grammars, sector-size query via vtable slot 5 | Grammar registry (69 rows) + raw-sign table (compiled-in generated, byte-equal to corpus file) + user-signs layer; validators decompile-exact for the implemented set; M-024 disabled_formats = RawConfig toggles; D-75 registers the omitted fallback generation; N-086 FIXED (D-103, e81b98f); **N-068/N-069/N-070/S-032/S-037 sub-items ALL FIXED in tree (verified SCAN-2f + SCAN-3)**. **S-007 niche remainder (the tracked recall residue): ~30 absent formats** — keyed, keys census-verified at the registration sites (the s3 corrections folded): MPEG-TS (MTS/M2TS, list-layer), APE (`MAC ` id 0x51), MID (`MThd` id 0x26, BE grammar, 14B..10MB), OGG (`OggS` id 0x52), RM (`.RMF` id 0x5e), MXF (`06 0E 2B 34` id 0x78), DMG (`BZh1` id 0x71), X3F (`FOVb` id 0x6c), **CRW (`II\x1A\x00`/`MM\x1A\x00` id 0x12 — NOT MRW)** + **MRW (`\0MRM` id 0x66 only)**, WPG (key `wpg\0` 0x00677077, min 20, max 100,000,000; registry id 0x33 at the registration site per the s3 census re-read — the 0x45 stamp is E48's format-id disposition, the og id number is documentation-only for the core's append-only enum), WPD (`FF WPC` id 0x44), PSP (`Pain` id 0x37, 36B..20MB), SIT (`Stuf` id 0x5d, 20,480B..200MB), GZ (u16 `1F 8B` id 0x1f, max 400MB), MDB (u16 `00 01` id 0x27, max 200MB + "Standard Jet DB" family), MNY (u16 `MS` id 0x28), CHM (`ITSF` id 0x0e, full GUID grammar :22635 region, size=u64@+0x68), **HLP (`3F 5F 03 00` id 0x20 only — `DB A5 2D 00`/`94 A6 2E 00` = FileLowDocAnalyser, the old-Word .doc family id 0x17)**, AU (`.snd` id 0x09), AIFF (`FORM` id 0x50), ICO/CUR (`00 00 01 00`/`00 00 02 00` = LE keys 0x00010000/0x00020000, both id 0x22), WMF/EMF (placeable `D7 CD C6 9A` + std `01 00 09 00`, one analyser id 0x46; EMF itself is LIST-layer), TTF (key `00 01 00 00` 0x00000100, id 0x41), ONE (`E4 52 5C 7B` id 0x31, 20B..500MB), EPS/AI (u32 `C5 D0 D3 C6` id 0x1b + u16 `%!` second registration), 3GP (ftyp-brand dispatch id 0x02), **3DS (u16 `MM` id 0x01 with `==`@+16 chunk gate — the "TIFF-BE collision" census row was the erratum, corrected)**, DXF (u32 `"  0\r"` 0x0d302020 + u16 `99` 0x3939, id 0x19, full `"  0\r\nSECTION\r\n  "` grammar + 999-comment scan); list-layer/structural: TAR (`ustar`@+257 id 0x3f), SWF (`FWS`/`CWS`, size=u32@+4, ZWS NOT accepted, id 0x3e), HTML (u16 `<h`/`<H`/`<!`/UTF-16-BOM id 0x21) + SVG/XML (u32 `<?xm`/BOM`<`/`<svg`/0x000001cd id 0x5b), MOBI (BOOKMOBI, id 0x85, RawConfig-gated), JP2 (sig box + `jp2c` keys 0x80/0x81/0x82, id 0x80, gated), LZH ("-lh" structural id 0x24 — NOT the 0x7B5C52E4 key, which is OneNote), EMF (record-type-1 header, id 0x1a), FCPEvt (XML, id 0x73 — the SQLite container analyser), OfficeTagFromXML; standalone XML family: VCF/GPX/HIK/OSP/POS/AEP/CHFP. **NO DBF/dBase analyser exists in the og** (C5 D0 D3 C6 = CEPSAnalyser = AI/EPS — session-3 correction; implementing DBF would EXCEED the og). Full per-format grammar table + the 68-row u32 registry map: session-3 resolution log below | **partially present — niche remainder in fix (per-school batches, this session)** |
| A5 | "Sector by sector" = whole-range FS passes + carve; settings+0x60 gate for RAW engagement | `run_carve` stage, `free_map` gate (M-005), occupancy gate (M-039), zero-stride skip (N-031) | **present** |
| A6 | Carve validator gating (CNeedRawMgr) | **RESOLVED (OQ-SA-2, this session)**: CNeedRawMgr = the RawConfig.ini [Filter] per-format manager consulted per-detection (extension-keyed); NO quick-vs-deep mode gating exists in the og; the core's M-024 disabled_formats = the equivalent, and the core's all-validators carve matches the og. The File*/CFile* analysers = the legacy fallback generation (chain demonstrated: MP3Analyzer→FileMP3Analyser at +0x18) — D-75 registers the core's single-validator posture | **present (registered delta D-75/M-024)** |

## 3. COMBINATION / TRANSFORMATION (quick ⇄ advanced)

| # | Og behavior | Core state | Verdict |
|---|---|---|---|
| C1 | Quick completion → `CQuickScanCompleteWnd` offers advanced while user may already recover; auto-recover engages `CAutoRecoverMgr` (recovery type 5 "Auto_Trigger_Recover") | UI-layer state machine; engine exposes stage events + truthful stage masks (G-07); no auto-recover in a CLI product | **N/A (UI layer; engine side present)** |
| C2 | Merge: engine controllers ARE tree builders (DetourDirectoryTree decorator intercepts insertion: progress + dedup + raw/structured merge); structured nodes win; carved → "Lost"/"Other deleted files" (LostTreeModel); engine-side "File Path Lost" root (FLAGS|=0x10, marker 8) + FAT/exFAT "LostDir%d" | One shared ResultTree; carve dedups against structured via `find_by_extent`; carved nodes under the carve root ("Other Deleted Files (Carved)", M-029-true name); remnant/lost under "Lost Files" | **present (registered delta)** — root labels differ from og UI strings; structural roles map 1:1 |
| C3 | 3-branch result tree: normal root + "File Path Lost" + "Reconstructed" (all 3 .rsf samples + vendor log + live capture) | Roots Found/Deleted/Lost Files + carve root, role-tagged and session-persisted (G-07) | **present (registered delta)** |
| C4 | Classification: CSetClassifyThread + FileFilter.xml 8 categories / **1,481** extensions (CORRECTIONS #5) + EUUserData MIME | `query::filefilter_entry_count() == 1481` test-pinned; category IDs Graphics=5/Document=6/Archive=7/Video=8/Audio=9 preserved in grading | **present** |
| C5 | Validation: CCheckNodeThread/OnCheckNode per-node recoverability; CRecoverability grading (size<1→0; ZFS→2; NTFS/ReFS/Btrfs deep path; device.kind {1,4}→−2; grades 0/2) | S-023 grading: size<1→Low; email→High; existing≥1→High; carve<0x400→High; NTFS device-index rule; 169-entry ext table; monotonic-max + sticky; `AssistantStats` persisted | **present** |
| C6 | Filtering: CAdvanceFileFilterMgr (ext/size/date buckets 0x100..0x1004, 0x1000..0x1004), CSetFilterThread, FilterFolder.ini junk sets | FilterFolder M-018 landed (three pools, og entry order); query buckets M-029; D-102 comparator registered | **present** |
| C7 | Sorting CSortNodeThread / selection / thumbnails | `query::run` sort/page; thumbnails are UI | **present (engine side)** |
| C8 | Session persistence: .rsf (magic EA F4 B3 1B…, 36-B record headers, zlib level-5 chunks ≤4096+stored fallback, 4 records, record-4 BFS tree grammar, IFileProperties wire per FS, writer CSaveAndRestoreResult::SaveResult) | CRSF (magic CRSFMT01, own record set, truthful stage masks, CRC + skip-forward, identity cards v4, per-FS extras N-059, boot snapshot N-066, checkpoint G-13, resume revalidation) — deliberate clean-room container (G-07), content-level semantics preserved | **present (registered delta)** |
| C9 | Scan-level time-to-first-result comparative bench (chunking strategies) | OQ-S4-3: transport half MEASURED (alignbench 40315255368: core 64 KiB envelope meets og 4/7 MiB within 3.8%, 256 KiB cache block beats it by 18-19% at lower CPU); scan-level half open — probe-target corrected (97cc3b2): the comparative-bench lane was wiped by the CI rebuild (8943993); the TTFR half's surface is the staged-scenarios lane's event stream (first FilesFound vs scan start), no engineering prerequisite; the Windows dispatch itself remains out of this container's reach (the standing execution-bound ground, unchanged across sessions) | **open (tracked; Windows-execution-bound)** |

## 4. MODULES & ARCHITECTURE

| # | Og | Core | Verdict |
|---|---|---|---|
| M1 | 10 lazily-loaded engine DLLs + DRWUI orchestration; shared SDK classes compiled into every engine | In-process single dispatch (I-016 registered; ARCH-DUP-4) | **present (registered delta)** |
| M2 | Volume IO: CPartitionVirtualVolumeDevice → file API or raw IOCTL → EPMNtDrv `\\.\EPRDTDRV\<d>\<p>` (512-B sectors, MDL async) | storage stack → rda transport (EPMNtDrv-exact IOCTLs, Information×512 scaling F11, byte-offset contract CORRECTION-2, geometry probe pass-through); driver byte-identical proof 64/64 ranges; direct-IO fallback D-37 | **present** (2-c audit: no NAS/Cloud violations anywhere in the driver) |
| M3 | Driver lifecycle: service RUNNING before scan; deployer write+verify | install exit-code contract 0-7 (v0.2.3); core FloorVersion flip blocked on driver tag (M-064/S-036 trail) | **present (tag pending upstream)** |
| M4 | Engine emit: nodes carry path chain, name, size, 4 timestamps, attributes, deletion state, extents/datarun, per-FS secondary fields | `tree::FileProperties` mirrors the wire census (cr_node.h; N-059 carriers; N-011a short names; N-016b name_raw) | **present** |
| M5 | Threading: 45-thread UI model, workers at priority 7; engine callbacks | ThreadPool hw−1 clamp 1..8, parallel candidate/carve/remnant fan-out, 50 ms cancel watcher N-017, worker-thread callback delivery documented | **present (registered delta)** |

## 5. FORMATS & PARAMETERS

| # | Og | Core | Verdict |
|---|---|---|---|
| F1 | raw_signs.json: 139-ext header/tail/min/max grammar, istream load at engine init (RAWSearchDLL :23836-23850); nothing ever writes it in-process | Generated compiled-in table (byte-equal content); posture **REGISTERED as D-103 (e81b98f, other lane)**: compiled-in over field-mutable (reproducibility; user-taught signs cover the runtime-extension surface) | **present (registered delta D-103)** |
| F2 | EUUserData: 111-entry install-tree JSON, read by DRWUI into `CHKRepairFileSign` (repair chain) — **NOT consumed by the RAW scan engine** (zero engine references; GetRawUserInfoMap = per-extension carve-counter map: FUN_18000ed00 = map<wstring,count> insert/increment, cleared at searcher init FUN_18000beb0 :9044-9051; DRWUI :227538+ walks it post-scan into the "Trigger_Raw" report) | **FIXED (37c48c6, other lane)**: default set shipped + engine-init load — but landed under a refuted justification; the landed behavior = deliberate superset (the og's engine has no user-sign channel). **Superset registration LANDED (D-104, 93097cb)** — the audit-trail correction batch closed this row's residue | **present (registered superset D-104)** |
| F2b | SQLite grammar validation on real databases: the og detects + page-math-sizes every real (BE-header) database (the CFCPEvt analyser's BE compose + the +56 encoding gate, RetDec :45126-45166) | **FIXED (A-011 + N-089, this window — convergent discovery by two lanes)**: the BE reads + the parenthesized 2 GiB page gate + the encoding gate + the encoding-aware FCP-subtype marker scan; pre-fix the surface was dead code on every real database (the LE reads + the `2ull<<(30/page_size)`=2 cap); the tree's first spec-correct fixtures landed | **present (fixed)** |
| F3 | User-sign caps (og's shipped DB measured): sign bytes ≤44, signs/entry ≤11, offset ≤526, mime ≤73, ext ≤8 | S-039 caps kMaxSignBytes=48/kMaxSignsPerEntry=12/kMaxOffset=1024/kMaxMimeLen=80 (measured maxima + headroom, atomic parse proven on the og's own DB) | **present** |
| F4 | RawConfig.ini 34 per-format toggles; default-off set (STL=0, VPJ=0, …) | `disabled_formats` M-024 + D-97 registered | **present** |
| F5 | ScanConfig surface: sector size 512 default; volume/partition/device identity; mode toggles | `ScanConfig` + CLI flags; `--whole-disk` (lost discovery); `--quick-only`; threads clamp; identity cards N-066 | **present** |
| F6 | Carve params: window/overlap, hash threshold, free-map gate, max extents | 1 MiB window + 4 KiB overlap, 8 MiB hash cap, 256 MiB max extent — all registered constants | **present (registered)** |
| F7 | Engine error taxonomy: 0x50-facility (HRESULT-style), DRWUI 0x23c0004/0x2410002 alloc/start | `cr::Err` families; D-68 registered | **present (registered delta)** |

---

## Session-3 resolution log (2026-10-28 — the fresh context-recovery leg; every claim below verified first-hand in the decompiles)

**OQ-SA-1 NTFS half — the +0x64 decode, INSTRUCTION-DECISIVE (all sites read
first-hand):**

- `FUN_1800336f0` (NTFSSearchDLL :40999-41128) — the runlist encoder. Line 41026
  fetches the FIRST run cursor entry; line 41028 `*param_2 = *plVar5` = **out1 =
  the first run's stored start LCN**; line 41043 `*param_3 += run.length` =
  **out2 = Σ run lengths (total clusters)**; lines 41037-41041 detect sparse runs
  (stored start == −1) and 41096-41098 zero out2 when any run is sparse. The
  nibble header at 41045 (`offsetSize<<4 | lenSize`) confirms this is the NTFS
  datarun encoder and out1 is the standard runlist's absolute first offset.
- `FUN_1800069e0` (the $DATA attribute parse inside the main walk's raw-block
  builder, :5095-5154): line 5115 calls the encoder; line 5126 `raw+0x53 =
  (u32)out1`; line 5127 `raw+0x4f = (u32)out2`. Line 5144's size-class formula
  `(allocSize/512)/plVar12` independently confirms plVar12=out2=total clusters.
- `FUN_180003020` (the raw-block→object copier, :1972-2005): line 1986 `+0x58 ←
  raw+0x47` (own MFT record number); line 1987 **`+0x64 ← raw+0x53`**; line 1988
  `+0x60 ← raw+0x4f`; lines 1996-1997 `+0x68`/`+0x6a` own/parent sequence numbers.
- `FUN_18002a340` (the $FILE_NAME deep path, :33862-33941): `local_39 = 0`
  (:33912) occupies the raw+0x4f qword — **+0x60/+0x64 are 0 on this path**; the
  runs attach later. `local_41 = *param_1 & 0xffffffffffff` (:33900) is the OWN
  48-bit reference → +0x58. The PARENT reference is returned to the caller via
  `*param_4` (:33932-33934) — **never stored in the object**.
- `FUN_180033250` (the data-stream attach helper, :2240-2266): line 2253 calls
  the encoder; lines 2264-2265 stamp `+0x64 = out1`, `+0x60 = out2` onto the
  props whenever a stream object attaches. The GUID-dump twin `FUN_18001c660`
  (:22470-22471) and the main-walk manual copy block (:5427-5428) agree.
- Serializer `FUN_180003440` (:2338-2407): lines 2362-2363 persist +0x64 then
  +0x60 (4 bytes each) immediately after +0x40 (:2361) — both fields are
  session-persisted in the og.
- Cross-class corroboration (`re-workspace/raw-evidence/props-vtables.txt` +
  DRWUI twins): the +0x64 getter is vtable slot +0x30 — a per-FS **location-
  anchor family** (FAT `FUN_1405af310` → +0x3e; exFAT `FUN_14058cbb0` → +0x50;
  CRAW → +0xec; HFS+/ReFS/Generic stubs return −1) — independently confirming
  +0x64 as "first cluster of the file's data".

**Consequence 1 — the N-050/N-059 "+0x60 type-registry id" reading is REFUTED**
(three independent ways): (1) every writer sites the runlist encoder's outputs
into +0x60/+0x64 — no type-detection call exists in the fill chain; (2) the
+0x60 getter (vtable slot +0x20) is the shared return-0 stub for 8 of 10
per-FS classes — a Type-column feed cannot be NTFS-only; (3) the RSF census
values attributed to "+0x60 type ids" (.lnk→1 ×410, .mp3→6 ×2,841, multiple
values per extension) are exactly what allocated-size-in-clusters looks like
on a duplicated-file QA disk. **Core impact: none** — no core code ever landed
on the refuted reading (the N-059 block covers Btrfs/ZFS/HFS+ only; the
+0x60-equivalent datum is derivable from allocated_size+extents). Corpus errata
rows filed (ifileproperties-part1 +0x60/+0x64/+0x40 rows; rsf-format :385;
ntfs-engine-algorithms-v2's vtable+0x28 "record_size" label — it is the SECTOR
size, making +0x40 the record's start sector).

**Consequence 2 — the +0x40 row also corrects**: +0x40 = the MFT record's
absolute start sector (batchByteOffset/stride + batchBase, stride = vtable+0x28
= 512 default = SECTOR size), 0 on deep paths. Core parity: the core's
record_ref already carries the record number; the record-sector datum is
reconstructible from the walk geometry — no carrier required (the og never
reads it back for recovery either).

**S-007 remainder — the s3 carve census (registry decode re-verified):** the
work list with corpus-exact keys and per-family provenance stands in
findings/FIXED/S-007 (session-2/3 addenda) with these census-artifact
CORRECTIONS absorbed into the implementation plan: DXF key `0x0d302020` +
u16 `99` (id 0x19, full grammar `"  0\r\nSECTION\r\n  "` / `999`-comment scan);
EPS/AI `C5 D0 D3 C6` (id 0x1b) + u16 `%!` second registration; OneNote
`E4 52 5C 7B` (id 0x31, 20B..500MB); CRW = II/MM`\x1A\x00` (id 0x12) — the
MRW key is `\0MRM` (id 0x66) only; placeable WMF `D7 CD C6 9A` + std WMF
`01 00 09 00` (id 0x46, one analyser); ICO/CUR keys `00 00 01 00` /
`00 00 02 00` (both id 0x22); TTF key `00 01 00 00` (id 0x41); u16 `MM` = 3DS
(id 0x01, `==`@+16 chunk gate, max 200MB); HLP `3F 5F 03 00` (id 0x20);
MNY u16 `MS` (id 0x28); MDB u16 `00 01` (id 0x27, max 200MB); WPG key
0x00677077 (id 0x33 — the "0x45" in the census is the SIZE stamp, erratum
E48; 20B..100MB); CHM `ITSF` (id 0x0e, full GUID grammar, size=u64@+0x68);
AU `.snd` (id 0x09); MIDI `MThd` (id 0x26, BE grammar, 14B..10MB); APE `MAC `
(id 0x51, BruteForce frame-sync); DMG `BZh1` (id 0x71); SWF = LIST-layer
`FWS`/`CWS` (id 0x3e, size=u32@+4, ZWS NOT accepted); TAR = LIST-layer
`ustar`@257 (id 0x3f); LZH = LIST-layer (id 0x24, max 200MB); MTS/M2TS/EMF/
MOBI/JP2 = LIST-layer (ids 0x76/0x74/0x1a/0x85/0x80); 3GP = ftyp-brand
dispatch (id 0x02); AIFF `FORM` (id 0x50); OGG `OggS` (id 0x52); MXF
`06 0E 2B 34` (id 0x78); HTML = u16 `<h`/`<H`/`<!`/UTF-16-BOM (id 0x21);
SVG/XML = u32 `<?xm`/BOM`<`/`<svg`/0x000001cd (id 0x5b); gzip u16 `1F 8B`
(id 0x1f, max 400MB); StuffIt `Stuf` (id 0x5d, 20,480B..200MB); RM `.RMF`
(id 0x5e); Dbx ×3 keys (id 0x15); PSP `Pain` (id 0x37, 36B..20MB); X3F `FOVb`
(id 0x6c); MRW `\0MRM` (id 0x66); WPD `FF WPC` (id 0x44); MNY/MDB as above.
PNG and WPG share id 0x33 in the og registry (duplicate ids exist in the og's
own id space — the core's serialized carve_format enum stays append-only and
core-sequential, unaffected).

**rda-driver verification (v0.2.4, `323e75a`):** IOCTL contract unchanged
(identity 0x60002400/04/08 raw values + extended 0x6002440/44/48; 19 live
session codes — the README's "15 commands" prose is stale, header + switch
authoritative; F11 Information×512; session LBA×probed-sector-size); zero
NAS/Cloud paths anywhere (case-insensitive sweep — only exclusion declarations
in docs); install exit codes 0-9 + `-Layout Versioned` + `-Uninstall`; sector
512 default with per-target 4Kn probing. **No core scan-engine change
required.** Driver-side doc-drift notes (the 15/18-vs-19 session-code prose,
the PE-version-1.0 suffix) belong to the driver lane — handoff noted.

**Core state verification:** suite green at last full run (970/970 gcc-13 ASan,
c899998); every commit since is docs/CI-only. OQ-SA-1 FAT/exFAT halves landed
(409503c); OQ-SA-3 correction batch landed (93097cb — CORPUS-ERRATA E-A/E-B +
PARITY-DELTAS D-104). Two stale rows found in docs/PARAMETERS.md §3 (the
user-sign caps row predates the S-039/M-067 v3 values 12/48/1024/80; the
"4-8 MiB carve window" row predates the landed 1 MiB+4 KiB model) — fixed
this session (formats/parameters phase). Board hygiene note for the owning
lane: M-068's finding file still sits in MAJOR/ though its own text records
the fix (43f4962) — move to FIXED/ + amend the SUMMARY count.

## Session-2 resolution log (prior leg, retained)

**OQ-SA-3 — RESOLVED (first-hand decompile chain, all sites cited):**
DRWUI reads `%s/EUUserData` (:1005387, `_wfopen_s`+fseek/ftell) → JSON parse (:1005497+)
→ per-extension "signs" arrays split "offset,HEX" (`_stricmp "signs"` :1005818; `atoi`
offset; hex token) → **`CHKRepairFileSign`** manager (vtable :1002677; maps by-offset +0x20 /
by-ext +0x30) → consumed by the MatchedFile/NotMatchedFile file-signature checker
(:721520-721680, strings `*****MatchedFile:%s,FileSign:%s` / `xxxxxxxxxx
NotMatchedFile:%s,FileSign:%s`), RTTI `.?AVCHKRepair@@/.?AVICHKRepairObject@@/
.?AVCHKRepairFileSign@@/.?AVCHKRepairAdapterMgr@@` — the **CHK repair chain**.
RAWSearchDLL contains **zero** EUUserData references (rg over 80,614 lines); its only
runtime-loaded signature data is raw_signs.json (:23836, read-only ifstream; nothing in
DRWUI writes it — rg zero hits) + config/RawConfig.ini (:13133). The searcher-init block
(FUN_18000beb0 :9044-9051) clears the +0x90 stats list, sets the +0x70 size cap
(200000000, read by the MPEG-TS walk bound :30331-30338) and +0x60; analysers insert
detected extensions into the +0x60 set (FUN_18000ed00 sites :19626/:20222/:20892/:21574/
:29402/:30443); `GetRawUserInfoMap` (= singleton+0x90) feeds DRWUI's "Trigger_Raw" /
"FileSystem" telemetry (:227538-227625). The per-searcher start sequence (FUN_140581360
:916931-917010) passes only config/auxiliary/path arguments — no signature handoff.

**Consequences:**
1. **M-065 recalibrates**: the 79-extension "recall risk" premise is refuted for the
   scan engine — the og's scan never consults EUUserData. The default-set gap belongs to
   the CHK-repair (file-repair) surface, which is out of the core's product scope (zero
   file-repair surfaces in core; rawfixer = pre-scan FS repair only). The finding's fix
   plan items 2-3 (ship a default carve-feeding set) would *exceed* the og, not match it.
2. **Corpus erratum** (CORPUS-ERRATA + corpus CORRECTIONS.md): the userdata artifact's
   "user-extensible **carving**" phrasing and the rawsearch deep-dive's
   "consulted-alongside-raw_signs" phrasing are corrected — EUUserData feeds CHKRepair.
3. **PARITY-DELTAS row (new)**: core's `--user-signs` carve capability = deliberate
   product superset (the og's engine has no user-sign channel); the parallel lane's
   landed default set (37c48c6) extends that superset with the og's own shipped data —
   legitimate once registered as such (A2 correction batch lands the row).
4. **OQ-SA-3 closes**; OQ-SA-1/OQ-SA-2 remain tracked with bounded probes.

## Fix queue (session 3 — priority order; merges the SCAN-2f pinning (A-lane niche batch → F-lane NTFS carrier) with the SCAN-3 execution order — quick scan first, then advanced/deep, then formats/parameters)

1. **OQ-SA-1 NTFS carrier (quick scan, FIRST — the F-lane item)** — `ntfs_origin_lcn`
   (u32) = low 32 bits of the first $DATA run's start LCN (sparse first run →
   0xFFFFFFFF per the og's stored-start read), stamped at both walk emission
   sites (main walk cr_mftwalk.cpp:893 block; GUID-dump path :1480 block),
   persisted as nflags bit-8 (4-byte append-only block, fires only when
   nonzero — mirrors bit-7). Tests: NTFS engine carrier + session round-trip,
   fail-before proven.
2. **S-007 remainder (advanced/deep, per-school batches — the A-lane item)** —
   the ~30 niche formats from the merged census table above, each: append FMT
   id + registry row (og-exact key bytes) + validator (og grammar where
   extracted; bounds + magic where the census is bounds-only) + per-format
   tests (plant/carve/reject-noise pattern). Batch order: u32-keyed bounded
   validators → u16-keyed → LIST-layer structural (SWF/TAR/3GP; LZH/MTS/EMF/
   MOBI/JP2 stay registered as the og's list-layer posture — a core-side
   equivalent sweep hook if the grammar is extractable, else documented
   remainder).
3. **Formats & parameters** — the two stale docs/PARAMETERS.md §3 rows fixed
   (user-sign caps; carve window). No open rows in diff-to-fix/PARAMETERS.md.
4. **Combination layer** — audit-verified present (roots/dedup/Lost Files/
   find_by_extent all landed); no action.
5. **Corpus errata** — the +0x60/+0x64/+0x40 correction rows (core
   CORPUS-ERRATA.md; the corpus repo's artifact fixes left to the corpus lane
   — handoff note).
6. **OQ-S4-3** — the TTFR harness landed (2f3f798/3307980, the API lane); the
   scan-level comparison itself stays execution-bound (Windows dispatch) —
   tracked-open until the next green run transcribes the numbers.

**Landed by the parallel lane while the prior session's context recovery ran (verified by A2):**
- **N-087 FIXED** (8451913): FAT 8.3 CP437 boundary decode — corpus-faithful (part1:121),
  0xE5/0x05 semantics preserved, kCp437 table cited.
- **N-086 dispositioned** (e81b98f): D-103 registers the compiled-in raw-signs posture
  (option (a) of the finding's fix plan — a knowing choice, now on the audit trail).
- **M-065 FIXED** (37c48c6) + **M-067** ABI resize (04dd769): the default set ships and
  loads — behaviorally a deliberate superset, registered D-104 (93097cb).

**Prior session's queue — outcomes:**
1. **The OQ-SA-3/M-065 audit-trail correction batch — LANDED** (93097cb/151ce17): OQ-SA-3
   resolved with the CHKRepair evidence chain; corpus errata E-A/E-B; D-104 superset row;
   handoff logged.
2. **OQ-SA-1 — FAT/exFAT halves LANDED** (409503c); NTFS +0x64 half now **DECODED
   (SCAN-2f, this session)**: first-$DATA-extent start LCN low dword — see Q11. Carrier
   implementation is this session's formats-lane fix.
3. **OQ-SA-2 — CLOSED** (1463221); **A-011/N-089 — LANDED** (c899998, convergent with the
   probe lane's landing; reconciled fc74677).
4. OQ-S4-3 scan-level half: stays open (Windows-hardware execution-bound; probe-target
   corrected 97cc3b2 — the staged-scenarios lane is the surface).

**SCAN-2f queue (this session, in the mandated fix order):**
1. **Quick scan (Q1-Q11)**: re-verified via the context legs — zero open defects; the only
   Q-lane change is the Q11 NTFS +0x64 decode (above). No code fix required in the quick
   lane.
2. **Advanced/deep (A-lane) — THE S-007 NICHE BATCH — BATCH 1 LANDED (`e018bf4`)**: 27
   og-only formats / 31 rows / 26 validators, every key re-read first-hand at its
   RAWSearchDLL registration site (retdec :5450-6600 + u16 layer :6205-6590 + the WPG/MDB
   analyser bodies) before the rows landed: MID OGG RM MXF DMG X3F MRW(x3) WPG WPD PSP SIT
   GZ APE AU AIFF HLP(x3) ICO CUR WMF-PL WMF-STD TTF MDB TAR EPS AI DXF 3GP. Fail-before
   proven (validators neutralized: 3/3 positive RED); suite 941/0 ASan; hostile smoke 400
   iters zero findings. Three census corrections landed as errata E49-E52 (3F5F03=HLP not
   NEF; DXF is keyed-u32 not list-layer; MDB is the u16 0x0100 chain shared with TTF).
   Shipped mechanical gates: `static_assert(kTrialCount == FMT_COUNT)` (a FormatId absent
   from kTrialOrder was silently never scanned — the F8/self-probe gates never covered
   trial-order membership); kMaxTrialSlots 96→160; the DMG seam made
   forward-progress-safe. **Batch-2 remainder with causes pinned in the S-007 addendum:**
   CHM (u64-widened GUID compares vs canonical ITSF layout — needs asm-level width read),
   MNY (MS key's gates undecoded — FP-unsafe without), EMF (no keyed registration found),
   ONE, MPEG-TS (numeric key 284), 3DS (MM/TIFF-BE multimap treatment), SWF/JP2/MOBI, the
   list-layer/text school (HTML/XML/SVG/LZH/FCPEvt/OfficeTagFromXML/CRW/VCF/GPX/HIK/OSP/
   POS/AEP/CHFP). NO DBF by design (no og analyser).
3. **Combination lane (C-lane)**: no engine-side defect open; C9 TTFR stays
   execution-bound (see row C9).
4. **Formats & parameters (F-lane) — OQ-SA-1 closure**: the NTFS `ntfs_origin_lcn`
   carrier (first-$DATA-extent LCN, stamped at $DATA attach; resident files carry the
   record-anchor caveat form), session-wire persistence in the nflags bit-8 form (the
   FAT/exFAT bit-7 precedent), runtime fail-before tests, OQ-SA-1 closure addendum +
   corpus erratum row (part1:85 "+0x64 secondary ID" → the decode).

**Driver-lane handoffs (Agent 2-d leg, recorded not fixed — single-repo commit discipline):**
rda-driver v0.2.4 wire contract byte-stable (identity 0x60002400/04/08 + extended
0x6002440/44/48 + all 19 session codes re-verified first-hand; NAS/Cloud exclusion holds).
Driver-repo doc drifts D2-partial/D3/D4 remain that lane's items (D3: the "15/18-command"
prose vs the 19-code set is now stale in seven driver-repo sites incl.
docs/ground-truth-contracts.md §5.2; D4: regdrv-rda.reg hex(7) UpperFilters still invalid).
Installer note for the packaging lane: driver binary FileVersion stays pinned 1.0.0.0 while
the release tag is v0.2.4 — same-build reinstalls hit exit 2 without -Uninstall.

**Contradiction register (A2 vs the parallel lane's FIX-6 note):** the lane's OQ-SA-3
reading ("GetRawUserInfoMap … feeds the RAW searcher's scan-setup trigger surface.
Capability-configuring") is refuted by the insert-side decode: FUN_18000ed00
(RAWSearchDLL :11376-11525) is `std::map<std::wstring, count>::insert-or-increment` —
first detection creates the entry with value 1, repeat detection increments
(`plVar10[8] = plVar10[8] + 1`); the map is keyed by the DETECTED EXTENSION and cleared
at every searcher init (:9044-9051). A per-scan counter map cannot configure scan
capability — it is the carved-file statistics the DRWUI report consumer (:227538+)
formats as `%lld` into the "Trigger_Raw"/"FileSystem" report tree. EUUserData's signs
never enter the engine through any channel (zero engine references; the DB's consumer
is CHKRepairFileSign — MatchedFile/NotMatchedFile).
