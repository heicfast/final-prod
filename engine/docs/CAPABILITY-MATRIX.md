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
| NTFS live tree | full (NTFSSearchDLL) | $MFT walk, fixups, $ATTRIBUTE_LIST chase, hard links per-parent, ADS children, metafile filter (known-name list subsumes the og's og-15+$SI SYSTEM+HIDDEN gate - N-040) | **DONE** | test_ntfs_volume.cpp (TreeStructure, HardLinks, Ads); AUDIT-NTFS MATCH rows |
| NTFS deleted grading | binary found/destroyed (v2 §1) | four-tier $Bitmap grading (Excellent/PartialRisk/Overwritten/MetadataOnly) | **DONE+** | D-14 F5; tests ViabilityGate*; boundary 0x3FF strict `>` (NTFS-1, W2) |
| NTFS remnant scan | stride salvage (corpus §5) | 4 MiB block-buffered stride + threads, meta-only nodes under Lost | **DONE** | scan_mft_remnants; test RemnantScan* |
| NTFS $MFTMirr bootstrap | mirror cross-check (Dec 52032/52102) | read_record mirror fallback for records 0..3 | **DONE** | cr_bootstrap read_record; test_ntfs_volume mirror cases |
| NTFS compressed $DATA (LZNT1) | RtlDecompressBuffer-family semantics | full LZNT1 ladder (MS-XCA tagged-group; `cr_decomp.cpp` `lznt1_payload_decode`) + MFT-walk inflation (`expand_compressed_data`, cr_mftwalk.cpp) + codec-tagged extents the writer inflates | **DONE** | N-074 doc-truth fix (was stale MISSING); `NtfsVolume.Lznt1MsXcaDocumentedVector` + the compressed-$DATA recovery tests |
| NTFS compressed $DATA (XPRESS/LZX) | RtlDecompressBuffer-family semantics | not implemented | **MISSING (GAP-P1-1, SHARED limitation — the og is LZNT1-only too; no XPRESS/LZX surface in any corpus artifact)** | CORE-GAP-LIST GAP-P1-1; tracked, not claimed |
| NTFS USN journal harvest | USN census (UNVERIFIED specifics) | $J run materialization + V2/V3 parse, DELETE-filtered, re-sync | **DONE (delta D-11)** | test USN harvest cases; UNVERIFIED-1 carried |
| Damage-audit check phase | FileSystemCheck.mo (chkdsk-grade audit) | read-only FAT/NTFS pass, og taxonomy 0x1001-0x100e, never aborts (M-041) | **DONE (M-041)** | cr/check/cr_fscheck.*; tests FatCheck.* / NtfsCheck.* / CheckPhaseNeverAbortsScan |
| Recycle Bin $I/$R remap | original names (bench 26/26) | token pairing (dot-stripped) + post-walk remap, both record orders; shared model (cr_recycle.h) across NTFS + FAT + exFAT with bin-root scoping (four-name _stricmp set incl. Recycled/Recycler/$RECYCLE.BI0) | **DONE (all three engines; M-052)** | 632edc4; M-052: test RecyclePairing*/RecycleLegacyRoots/RecycleBinScope* |
| FAT12/16/32 live+deleted | FATSearchDLL | flavors, LFN (deleted-slot reversal), chain guards, mirrored-FAT salvage | **DONE+** | D-39; test_fat_volume.cpp; FAT-1..10 fixes W2 |
| FAT backup-boot fallback | none in-engine (RawFixer pre-scan) | FAT32-gated sector-6 fallback, 4Kn stride | **DONE (delta D-44/FAT-8)** | test Fat32BackupBootFallback + Fat16HasNoBackupBootFallback |
| exFAT | exFATSearchDLL | boot-region checksum, entry-set checksum, NoFatChain, FREE continuation salvage, boot gates | **DONE** | EXFAT-1..6 W2; test_exfat_volume.cpp |
| ext4 live+deleted | EXT4SearchDLL | SB/GDT/inodes/extent trees, deleted inodes, orphan sweep | **DONE** | test_ext4_volume.cpp |
| ext4 journal salvage | walk-all (E-2 decompile pin) | every block in [s_first, s_maxlen) feeds the dir parser | **DONE** | D-07 CLOSED; JournalSalvage test |
| HFS+ | HFSSearchDLL | volume header, catalog+extents B-trees, fork model, orphan sweep | **DONE** | test_hfsplus_volume.cpp; F-A2-17/18 verified W-audit |
| HFS+ attributes B-tree / journal replay | not evidenced engine-side | absent | **MISSING (registered gap, D-L)** | CAPABILITY note; og extent itself UNVERIFIED |
| Btrfs | BtrfsSearchDLL | mirror SB select, chunk mapping, tree walk, subvolume roots, inline extents, compressed extents (zlib/lz4/zstd) | **DONE** | BTR-2/3/4 W2; test_btrfs_volume.cpp incl. compressed fixtures |
| Btrfs old-generation trees | none (max-gen only) | same (parity) | **DONE (parity)** | D-06 header note |
| ZFS | ZfsSearchDLL | label nvlist, uberblock ring (every valid state walked — per-txg history, S-034 FIXED: newest state live, older states feed the deleted tree with identity dedup), MOS/ZPL, dnode walk, embedded blocks, gang blocks, compressed extents | **DONE** | test_zfs_volume.cpp incl. Zfs.Ring* (S-034); D-05 corrected to the fixed stance |
| ZFS multi-vdev topology | UNVERIFIED in corpus | single-vdev only | **PARTIAL (gap, D-L)** | D-09 remains open — honest gap |
| **ReFS (FIRST-CLASS)** | ReFSSearchDLL (full MSB chain) | full MSB chain: page signatures, dual-checkpoint max-clock, versioned pointer lists (minor>=14), next-chains, ObjectID/directory tables, $DATA runs, deleted/ID2 harvest, allocator grading | **DONE** | W2 REFS-1/2/3 (next-chain + versioned checkpoint + stale pins); test_refs_ldm.cpp (11 tests); DBR gate 3.4+ (major==3 && minor>3, FUN_180012390) |
| ReFS containers / security / ADS | attribute layer decoded (S-020/M-022: resident inline + all attribute rows + paged tables + granularity guard; container map N-036 open) | resident materialization, dual grammar, indirect chase, granularity discard | **DONE (attribute layer)** — container-map translation tracked (N-036) | findings/FIXED/S-020 + M-022; test_refs_volume.cpp (6 S-020 tests) |
| **LDM (FIRST-CLASS)** | LdmManager | full decode: TOCAB01 databases, VBLKs, component/partition/volume assembly, RAID-0/1 concatenation, cross-disk resolver; GPT-dynamic discovery (real Microsoft type GUIDs — restored by A-002 after the A-001 fix's regression; og-verified + de-self-referenced guards) | **DONE** | test_ldm.cpp (incl. GptDynamicDiskCanonicalGuidsParse / RealCanonicalGuidsParse / FabricatedGuidsRejected); LDM-1 honesty note (RAID-5 rotation og-side dormant per I-008 — left-symmetric on both sides, RESOLVED) |

## 2. Pipeline

| Capability | og | Ours | Status | Evidence / gap |
|---|---|---|---|---|
| Lost-partition discovery | CGenericRecoveryController insert | deep-stage whole-disk discover, candidate scoring, virtual volume materialization, parallel candidate scans; probe families NTFS/FAT/exFAT/GPT/ext/HFS+/Btrfs/ZFS/ReFS (S-006) + BitLocker "-FVE-FS-" (S-019, og type-0xB classifier gate; locked-volume surfacing) + UDF (M-044: the og's AVDP→Main-VDS descriptor probe, analyser result 0xc; no engine on either side — carve-only) | **DONE** | test_lostpart.cpp incl. LostBitlocker* S-019 tests; deleted_found merge F-B-11 |

| Lost-partition discovery | CGenericRecoveryController insert | deep-stage whole-disk discover, candidate scoring, virtual volume materialization, parallel candidate scans; probe families NTFS/FAT/exFAT/GPT/ext/HFS+/Btrfs/ZFS/ReFS (S-006) + BitLocker "-FVE-FS-" (S-019, og type-0xB classifier gate; locked-volume surfacing) | **DONE** | test_lostpart.cpp incl. LostBitlocker* S-019 tests; deleted_found merge F-B-11 |
| Lost-partition APPLY (S-029) | RecoveryModule CreateRecoveryPartitionObject -> PartitionRecovery FUN_1800566d0/FUN_180038b10/FUN_1800392f0 (delete-layout -> sector-0 rewrite -> set-layout / 0x7C058 record -> 3x UPDATE_PROPERTIES retry; GPT_GenerateSignature rebuild) | apply module (cr/apply): plan_apply (read-only, refusals: overlap/no-slot/GPT-v1/capacity/32-bit-range) -> execute_apply (consent-gated default-deny, S-010 quiesce, sector-0 race guard, CRUNDO01 transaction, verified write-back, corpus device-control sequence per path) -> undo_apply; cr_ui consent ABI (cr_ui_apply_lost_partition/undo_last/save_undo); Windows RW transport carries the real IOCTL chain (0x7C100/0x7C010/0x7C058/0x70140 + Sleep(1000) tail) | **DONE (deltas D-72a..d)** | test_apply.cpp (13 tests: the five finding-named proofs + legacy-sequence/GPT-signature/race/refresh/rollback/refusals); CrUi.LostApply* C-ABI tests; ui_abi_check.c plain-C lifecycle |
| Post-apply boot-chain repair (M-040) | Fixup.mo BCD offset surgery (FUN_180001980 cell decode + FUN_180002520 rewrite + Bcd_RestoreBcdFile undo + the BCD00000000 force-unload) + EFIBoot.mo Boot#### NVRAM patching (FUN_1800046f0 matcher + FUN_180004b40 full-position rewrite, dynamic firmware seam :3281-3290) | cr/bcdfix (stage a: cell decode + new_lba<<9 rewrite + plan/execute/undo + P62 consent) + cr/nvramfix (stage b: the Boot#### updater over the injected seam, MBR/GPT matching, no create/delete) + src/win/cr_bootfix.cpp (the file side: force-unload retry + flush tail; the firmware side: dynamic resolution + SeSystemEnvironmentPrivilege); boot.ini + chkdsk = registered dispositions (D-90); the post-apply wiring is the host-layer boundary (the og runs it in MainModule's host chain) | **DONE (stages a+b; D-90 dispositions)** | test_bcdfix.cpp (5) + test_nvramfix.cpp (11, incl. NvramPatchUpdatesLoadOption); the win layer on the Windows CI lane |
| GPT two-step salvage | IsGPTDisk + backup (FUN_180006e20) | primary-entry salvage + device-derived backup LBA + protective-only entry | **DONE** | 3267d26 (W2-19); 3 new tests |
| MBR/EBR chains | Partition.mo | EBR chain walk with cycle guards | **DONE** | test_crc32_gpt.cpp EBR cases |
| Carving registry | raw_signs.json 139 ext + the hardcoded dual-registry common set (S-007) | signature registry + load-time validator gate (F8), BE-TIFF/ORF wired, +27 S-007 entries (GIF/BMP/audio family/AAC/RTF/OLE/MPEG/DWG) | **DONE+** | W2 CARVE-3; D-57 (S-007); test_carve.cpp |
| Carving validators | grammar+sign + BruteForce frame walkers | grammar validators (JPEG/PNG/ZIP/7z/PST/OFFICE/TIFF/WAV/MP3/PDF...), ZIP 3-anchor + CD ownership + padding, PST floor, SOF0 gate; S-007: GIF/BMP/RTF/OLE/MPEG-PS/ES/DWG + 10-frame headerless MP3/AAC walkers | **DONE** (niche ~25-format tail scoped in S-007) | W2 CARVE-2/4/6/11; D-57 (S-007); test_carve.cpp fixtures |
| Merge/dedup | UI attach points | (first-cluster, size-class, hash) dedup pre-attach | **DONE+** | F9; "pass 2 adds zero nodes" test |
| Session (CRSF) | .rsf | own format: per-record CRC32, skip-forward, versioned records, five roles | **DONE (delta D-01)** | test_session.cpp round-trip + F10 |
| Checkpoint/resume | none located (UNVERIFIED-4) | stage-boundary CheckpointSink, crash-resume, --resume CLI | **DONE+** | G-13/G-07; test_pause_resume.cpp |
| Writer | direct write + name_N rename | queue, collision policy, atomic temp+rename, hash verify, dirs, Salvaged outcome truth | **DONE+** | F11/F12/D-17; test_writer_dirs, Salvaged tests |
| Report | UI-rendered | JSON report, CI-validated schema | **DONE (ours-only D-41)** | tools/validate_report.py |
| Health/SMART | SAT wrapper | ATA_PASS_THROUGH + NVMe log-page 2, 0xD1 vendor thresholds, the 4-status grading model (Unknown/Good/Caution/Bad per the DiskHealth rules), derived verdicts | **DONE (delta D-42 + S-024 grading)** | test_health parse cases + test_health_grade (8 rule tests) |
| RawFixer | FATFixer/CheckBackupDBR; NTFSFixer CheckMFT; FATFixer CheckFatTable/CheckRootDir; shared FixByScanSector; pre-scan position | analyze + dry-run + transactional apply/rollback, 4Kn-aware, consent-gated; CheckMFT ($MFT record 0 from $MFTMirr), CheckFatTable (sector-granular mirror restore), CheckRootDir/IsValidRootDir/CheckRoot advisories, FixByScanSector (bounded 64 MiB probe); wired pre-scan (CLI --repair/--repair-apply, cr_ui consent ABI, Repair events, undo-log persistence); volume quiesce S7 (lock before first write, refuse-not-race, corpus LOCK/DISMOUNT sequence) | **DONE (S-008+S-010)** | FIX-2/3 W3; test_rawfixer.cpp + PreScan* orchestrator/ABI tests |
| Damage-audit check phase (FileSystemCheck) | CCheckFAT (DBR ceilings, FAT hole/dup scans, pairwise copy compare, dir-tree audit, orphan sweep) + CCheckNTFSVolume ($MFT/$MFTMirr proofs, version gate, $Bitmap structural, MFT×$Bitmap cross-check), error taxonomy 0x1001..0x100e, 8-event host callbacks; never aborts the scan | full parity module: cr/check (both passes, og phase order + terminal semantics, the taxonomy, the callbacks incl. the orphan host-decision); wired pre-scan after repair (ScanConfig::pre_scan_check, cr_ui ABI pre_scan_check + check_* stats + CR_EVT_FS_CHECK 10, CLI scan --check); REC_FS_CHECK session record | **DONE (M-041, delta D-87)** | test_fscheck.cpp (15 tests incl. CheckNeverAbortsScan + the session round-trip); findings/FIXED/M-041 |
| Transport | EPRDTDRV grammar + direct | RDA grammar + direct duality, retry/bisection + ReadErrorMap (the structured-open RdaClient was deleted by N-041 - zero callers since introduction; wire structs stay documented in cr_rda.h) | **DONE** | D-37/D-38; test_retry.cpp |
| Orchestration | Quick+Advanced generations | staged single-pass (D-36), weighted monotonic progress, pause/cancel/checkpoint, error map with per-range module attribution (M-049: the engine-side error-context subset — CR_ERRMOD_* on the ErrorRange event) | **DONE** | ARCH-4 reworded claims; test_orchestrator.cpp |

## 3. Explicit exclusions (standing)

Cloud/NAS/remote recovery; BitLocker FVE-key DERIVATION from a recovery
password or .BEK inside our stack (tracked-open OQ-S20-1: the FVE metadata
datum grammar is not decoded in the corpus; in-STACK DECRYPTION ITSELF is
live since M-033 — a user-supplied FVEK, the og's GetFullVolumeEncryptionKey
output shape, flows through `DecryptingDevice`, the in-stack XTS-AES layer
matching the og's volume-IO placement: `--bitlocker-fvek` CLI /
`cr_ui_open_volume_with_key` ABI; detection + legitimate unlock remain the

S-014 posture: live-registry + offline-hive auto-unlock; the N-064
password-dictionary aid is the og's Dict unlock flow ported with the
consent gate intact, in BOTH same-window landing forms — explicit
candidates (`cr/pwddict` + `bitlocker::try_password_unlock` + CLI
`--bitlocker-pwddict <path|seed>`; the shipped seed is an authored
list, D-78: the og's 228-entry asset is not copied, the loader is
format-compatible so any dictionary in the og's format works including
the og's own file) and the shipped-dictionary action
(`unlock_with_dictionary` + CLI `--bitlocker-dict` /
`cr_ui_unlock_volume_with_dictionary`; the 228-entry list + user
candidates through the manage-bde/PowerShell plumbing). Explicit host
action only in every form; locked volumes WITHOUT user consent or key
material stay refused — ERR_FVE_LOCKED, no bypass); signing

S-014 posture: live-registry + offline-hive auto-unlock, the N-064
password-dictionary unlock aid (cr::dict + --bitlocker-dict /
cr_ui_unlock_volume_with_dictionary — user-consented only, 228-entry
shipped dictionary + user candidates through the manage-bde/PowerShell
plumbing), and locked volumes WITHOUT user key material or the explicit
dictionary consent stay refused — ERR_FVE_LOCKED, no bypass); signing
S-014 posture: live-registry + offline-hive auto-unlock; the N-064
password-dictionary aid (`cr/pwddict` + `bitlocker::try_password_unlock`
+ CLI `--bitlocker-pwddict <path|seed>`) is the og's Dict unlock flow
ported with the consent gate intact — explicit host action only, the
shipped seed is an authored list (D-78: the og's 228-entry asset is not
copied; the loader is format-compatible so any dictionary in the og's
format works, including the og's own file); and locked volumes WITHOUT
user consent or key material stay refused — ERR_FVE_LOCKED, no bypass);
signing
S-014 posture: live-registry + offline-hive auto-unlock, the N-064
password-dictionary unlock aid (cr::dict + --bitlocker-dict /
cr_ui_unlock_volume_with_dictionary — user-consented only, 228-entry
shipped dictionary + user candidates through the manage-bde/PowerShell
plumbing), and locked volumes WITHOUT user key material or the explicit
dictionary consent stay refused — ERR_FVE_LOCKED, no bypass); signing

(OSSign disabled); `.rsf` import (G-11, tracked disposition).

## 4. Open gap register (code-verified)

See `docs/CORE-GAP-LIST.md` for the live, evidence-cited gap register (GAP-P1-1
NTFS compression is the only recovery-output P1 still open). This matrix and
that list are updated together; a status change here requires a commit citing
the GAP id.
