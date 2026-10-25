# CAPABILITY-MATRIX — core_recovery per-capability status vs the reference

> Single source of truth for "what can this build do, and how does that compare
> to the og reference". Statuses: **DONE** (og-parity or better, evidence-linked),
> **PARTIAL** (works with a registered, bounded gap), **MISSING** (absent; tracked),
> **EXCLUDED** (standing scope exclusion). Every row cites its evidence: a test
> name, a commit, and/or a registry entry (`PARITY-DELTAS.md` D-xx / `AUDIT-*.md`).
>
> **Scope correction carried forward (non-negotiable):** ReFS and LDM are
> FIRST-CLASS capabilities. Any text elsewhere claiming "ReFS detection
> report-only" or "LDM recognition-only through v0.2" is stale history — the
> engines are full implementations (rows below).

## 1. Filesystem engines

| Capability | og (corpus evidence) | Ours | Status | Evidence / remaining gap |
|---|---|---|---|---|
| NTFS live tree | full (NTFSSearchDLL) | $MFT walk, fixups, $ATTRIBUTE_LIST chase, hard links per-parent, ADS children, metafile filter | **DONE** | test_ntfs_volume.cpp (TreeStructure, HardLinks, Ads); AUDIT-NTFS MATCH rows |
| NTFS deleted grading | binary found/destroyed (v2 §1) | four-tier $Bitmap grading (Excellent/PartialRisk/Overwritten/MetadataOnly) | **DONE+** | D-14 F5; tests ViabilityGate*; boundary 0x3FF strict `>` (NTFS-1, W2) |
| NTFS remnant scan | stride salvage (corpus §5) | 4 MiB block-buffered stride + threads, meta-only nodes under Lost | **DONE** | scan_mft_remnants; test RemnantScan* |
| NTFS $MFTMirr bootstrap | mirror cross-check (Dec 52032/52102) | read_record mirror fallback for records 0..3 | **DONE** | cr_bootstrap read_record; test_ntfs_volume mirror cases |
| NTFS compressed $DATA (LZNT1/XPRESS/LZX) | RtlDecompressBuffer-family semantics | extent codec hook exists; codecs NOT implemented | **MISSING (GAP-P1-1)** | CORE-GAP-LIST GAP-P1-1 — silent raw-bytes output for compressed streams; tracked, not claimed |
| NTFS USN journal harvest | USN census (UNVERIFIED specifics) | $J run materialization + V2/V3 parse, DELETE-filtered, re-sync | **DONE (delta D-11)** | test USN harvest cases; UNVERIFIED-1 carried |
| Recycle Bin $I/$R remap | original names (bench 26/26) | token pairing (dot-stripped) + post-walk remap, both record orders | **DONE** | 632edc4; test RecycleBin* incl. late-remap shape |
| FAT12/16/32 live+deleted | FATSearchDLL | flavors, LFN (deleted-slot reversal), chain guards, mirrored-FAT salvage | **DONE+** | D-39; test_fat_volume.cpp; FAT-1..10 fixes W2 |
| FAT backup-boot fallback | none in-engine (RawFixer pre-scan) | FAT32-gated sector-6 fallback, 4Kn stride | **DONE (delta D-44/FAT-8)** | test Fat32BackupBootFallback + Fat16HasNoBackupBootFallback |
| exFAT | exFATSearchDLL | boot-region checksum, entry-set checksum, NoFatChain, FREE continuation salvage, boot gates | **DONE** | EXFAT-1..6 W2; test_exfat_volume.cpp |
| ext4 live+deleted | EXT4SearchDLL | SB/GDT/inodes/extent trees, deleted inodes, orphan sweep | **DONE** | test_ext4_volume.cpp |
| ext4 journal salvage | walk-all (E-2 decompile pin) | every block in [s_first, s_maxlen) feeds the dir parser | **DONE** | D-07 CLOSED; JournalSalvage test |
| HFS+ | HFSSearchDLL | volume header, catalog+extents B-trees, fork model, orphan sweep | **DONE** | test_hfsplus_volume.cpp; F-A2-17/18 verified W-audit |
| HFS+ attributes B-tree / journal replay | not evidenced engine-side | absent | **MISSING (registered gap, D-L)** | CAPABILITY note; og extent itself UNVERIFIED |
| Btrfs | BtrfsSearchDLL | mirror SB select, chunk mapping, tree walk, subvolume roots, inline extents, compressed extents (zlib/lz4/zstd) | **DONE** | BTR-2/3/4 W2; test_btrfs_volume.cpp incl. compressed fixtures |
| Btrfs old-generation trees | none (max-gen only) | same (parity) | **DONE (parity)** | D-06 header note |
| ZFS | ZfsSearchDLL | label nvlist, uberblock ring (txg-only), MOS/ZPL, dnode walk, embedded blocks, gang blocks, compressed extents | **DONE** | test_zfs_volume.cpp; D-05 stance |
| ZFS multi-vdev topology | UNVERIFIED in corpus | single-vdev only | **PARTIAL (gap, D-L)** | D-09 remains open — honest gap |
| **ReFS (FIRST-CLASS)** | ReFSSearchDLL (full MSB chain) | full MSB chain: page signatures, dual-checkpoint max-clock, versioned pointer lists (minor>=14), next-chains, ObjectID/directory tables, $DATA runs, deleted/ID2 harvest, allocator grading | **DONE** | W2 REFS-1/2/3 (next-chain + versioned checkpoint + stale pins); test_refs_ldm.cpp (11 tests); DBR gate 3.4+ (major==3 && minor>3, FUN_180012390) |
| ReFS containers / security / ADS | og extent UNDECODED | absent | **PARTIAL (tracked)** | AUDIT-LINUX-FS ReFS table — no silent claims |
| **LDM (FIRST-CLASS)** | LdmManager | full decode: TOCAB01 databases, VBLKs, component/partition/volume assembly, RAID-0/1 concatenation, cross-disk resolver; GPT-dynamic discovery (canonical type GUIDs, A-001 fix) | **DONE** | test_ldm.cpp (incl. GptDynamicDiskCanonicalGuidsParse); LDM-1 honesty note (RAID-5 rotation UNDECODED og-side; our raid5_ls is the libldm verdict) |

## 2. Pipeline

| Capability | og | Ours | Status | Evidence / gap |
|---|---|---|---|---|
| Lost-partition discovery | CGenericRecoveryController insert | deep-stage whole-disk discover, candidate scoring, virtual volume materialization, parallel candidate scans | **DONE** | test_crc32_gpt.cpp lost cases; deleted_found merge F-B-11 |
| GPT two-step salvage | IsGPTDisk + backup (FUN_180006e20) | primary-entry salvage + device-derived backup LBA + protective-only entry | **DONE** | 3267d26 (W2-19); 3 new tests |
| MBR/EBR chains | Partition.mo | EBR chain walk with cycle guards | **DONE** | test_crc32_gpt.cpp EBR cases |
| Carving registry | raw_signs.json 139 ext | signature registry + load-time validator gate (F8), BE-TIFF/ORF wired | **DONE+** | W2 CARVE-3; test_carve.cpp |
| Carving validators | grammar+sign | grammar validators (JPEG/PNG/ZIP/7z/PST/OFFICE/TIFF/WAV/MP3/PDF...), ZIP 3-anchor + CD ownership + padding, PST floor, SOF0 gate | **DONE** | W2 CARVE-2/4/6/11; test_carve.cpp fixtures |
| Merge/dedup | UI attach points | (first-cluster, size-class, hash) dedup pre-attach | **DONE+** | F9; "pass 2 adds zero nodes" test |
| Session (CRSF) | .rsf | own format: per-record CRC32, skip-forward, versioned records, five roles | **DONE (delta D-01)** | test_session.cpp round-trip + F10 |
| Checkpoint/resume | none located (UNVERIFIED-4) | stage-boundary CheckpointSink, crash-resume, --resume CLI | **DONE+** | G-13/G-07; test_pause_resume.cpp |
| Writer | direct write + name_N rename | queue, collision policy, atomic temp+rename, hash verify, dirs, Salvaged outcome truth | **DONE+** | F11/F12/D-17; test_writer_dirs, Salvaged tests |
| Report | UI-rendered | JSON report, CI-validated schema | **DONE (ours-only D-41)** | tools/validate_report.py |
| Health/SMART | SAT wrapper | ATA_PASS_THROUGH + NVMe log-page 2, derived verdicts | **DONE (delta D-42)** | test_health parse cases |
| RawFixer | FATFixer/CheckBackupDBR; NTFSFixer CheckMFT; FATFixer CheckFatTable/CheckRootDir; shared FixByScanSector; pre-scan position | analyze + dry-run + transactional apply/rollback, 4Kn-aware, consent-gated; CheckMFT ($MFT record 0 from $MFTMirr), CheckFatTable (sector-granular mirror restore), CheckRootDir/IsValidRootDir/CheckRoot advisories, FixByScanSector (bounded 64 MiB probe); wired pre-scan (CLI --repair/--repair-apply, cr_ui consent ABI, Repair events, undo-log persistence); volume quiesce S7 (lock before first write, refuse-not-race, corpus LOCK/DISMOUNT sequence) | **DONE (S-008+S-010)** | FIX-2/3 W3; test_rawfixer.cpp + PreScan* orchestrator/ABI tests |
| Transport | EPRDTDRV grammar + direct | RDA grammar + direct duality, RdaClient surface (wizard), retry/bisection + ReadErrorMap | **DONE** | D-37/D-38; test_retry.cpp |
| Orchestration | Quick+Advanced generations | staged single-pass (D-36), weighted monotonic progress, pause/cancel/checkpoint, error map | **DONE** | ARCH-4 reworded claims; test_orchestrator.cpp |

## 3. Explicit exclusions (standing)

Cloud/NAS/remote recovery; BitLocker decryption (detection + legitimate unlock
only, F13); signing (OSSign disabled); `.rsf` import (G-11, tracked disposition).

## 4. Open gap register (code-verified)

See `docs/CORE-GAP-LIST.md` for the live, evidence-cited gap register (GAP-P1-1
NTFS compression is the only recovery-output P1 still open). This matrix and
that list are updated together; a status change here requires a commit citing
the GAP id.
