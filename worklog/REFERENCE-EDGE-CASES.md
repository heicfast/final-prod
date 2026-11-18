# REFERENCE-EDGE-CASES — What the ORIGINAL (EaseUS DRW) Survives That We Must Too

> Phase-0 / Lane E safety-hardening session (Agent B, reference-corpus side).
> Source corpus: `/home/z/my-project/easeus-drw-research` (~300 RE markdown docs derived
> from the og binaries). Every row cites `corpus-file §section` (paths relative to the
> corpus root; `artifacts/` = `re-workspace/artifacts/`). CORRECTIONS.md rows override
> earlier artifact text — applied throughout. No row is speculation; where the corpus
> lacks material for a section, it says so in one line.
> Purpose: defect-hunt feed. For every edge case below, our engine needs a
> validation / bounds / cancel answer.

---

## 1. Scan / enumeration edge cases

| # | Edge case | What the og does (cited) |
|---|---|---|
| 1.1 | Damaged partition table / lost partitions | Lost partitions are inserted as additional scan targets ("Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d"); PartitionRecovery analysers generate candidates, engines scan them via `CPartitionVirtualVolumeDevice` (works on volumes the OS can't mount). `architecture/04-recovery-workflow.md §3, §4a`; `artifacts/SearchDLL/ntfs-engine--deep-dive.md §4.2` |
| 1.2 | Overlapping partition candidates (ghost partitions) | Real QA log shows 15+ overlapping NTFS candidates; `AdjustPartitionInfoOrder` resolves the final set by fileCount ranking (2048-start/16-files wins over 0-file ghosts). `artifacts/config-files/qa-logs--session-forensics.md §3`; `artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md §3` (CalcPartition scorers) |
| 1.3 | Fragmented $MFT ($ATTRIBUTE_LIST, type 0x20) | Dedicated fragmented-MFT handler: attribute list chased across records to reassemble $DATA runs; log "MFT file record has 0x20 attribute, MFT start sector is %I64d". `artifacts/SearchDLL/ntfs-engine--deep-dive.md §2`; `artifacts/SearchDLL/ntfs-engine-internals.md §MFT-acquisition item 3` |
| 1.4 | Cyclic/looped FAT chains (backward-pointing clusters) | Visited-range red-black tree (`range_tree.insert(cluster)`); each cluster range inserted once, repeated range stops the walk — corrupt FAT loops cannot hang the scan. `artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md §2` |
| 1.5 | Corrupt/nonstandard boot sector | Pre-scan repair: NTFSFixer/FATFixer/ExFATFixer `CheckDBR`/`FixDBR`/`FixFromDBR` (restore from backup DBR)/`FixByScanSector` (deep content scan) — transactional with `Undo`. `artifacts/PartitionRecovery/rawfixer-fs-repair.md` (Class API + Repair strategy) |
| 1.6 | Boot-sector geometry corrupt (SPC field) | ExFATFixer::`TryFixFromSectorsPerCluster` re-derives geometry from content scan. `artifacts/PartitionRecovery/rawfixer-fs-repair.md` (method table); `artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md §3` |
| 1.7 | Encrypted volumes (BitLocker) | Full clean-room FVE stack: detect `-FVE-FS-` OEM signature, protector enumeration (password/recovery-key/BEK), VMK→FVEK derivation, XTS-AES sector decrypt (OpenSSL EVP); auto path via DRWService pipe `\\.\Pipe\DRWBitlocker` reading FVEAutoUnlock; PowerShell `Unlock-BitLocker` / legacy manage-bde fallback. `artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md` (API surface + Recovery flow) |
| 1.8 | Dynamic disks (LDM spanned/striped/mirrored/RAID-5) | Offline LDM parse chain: VMDB (magic 0x564D4442) → TOCBLOCK three-location search → PRIVHEAD GUID/size validation; VBLK volume composition; pairs with CRepairRaid5. `artifacts/MOM-modules/module-internals-wave2--ldm-reglib-ctbfile.md §1.1–1.2`; `artifacts/PartitionRecovery/pending-operation-engine.md` (operation vocabulary, CRepairRaid5) |
| 1.9 | 4Kn/512e sector sizes | EPMNtDrv hardcodes 512-byte sector contract (`Length << 9`); user mode normalizes — `InitSectorSize` in EUDCPEPM + user-mode geometry queries; RAW analysers query device real sector size (vtable slot 5) "rather than assuming 512 — correct on 4Kn disks". `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md §3 + §512-byte note`; `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §5.3` |
| 1.10 | USB / target disconnect mid-scan | "Drive Disconnected" — "Unable to continue scanning. Reconnect your drive and try again." with Retry/Cancel; `CDevNotifyManager`/`CDevNotifyWnd` WM_DEVICECHANGE window. `artifacts/UIUX/08-scan-phase-deep-dive.md §5`; `artifacts/PartitionRecovery/mom-module-system--architecture.md` (device-change window) |
| 1.11 | Whole-disk raw access on locked/damaged volumes | Engines open `\\.\EPRDTDRV\{disk}\{part}` → whole-disk absolute-LBA path (DR{n} fallback naming, retry once); no OS filesystem stack involved. `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md §2, §7` |
| 1.12 | Healthy vs damaged volume IO switching | Volume IO picks normal file API when mounted & healthy, else raw IOCTL path; `CStorageDeviceWithCache` read cache in front to survive MFT re-reads economically. `architecture/05-scan-engine-architecture.md §8`; `artifacts/SearchDLL/ntfs-engine-internals.md §IO & robustness` |
| 1.13 | Newest-transaction crash consistency (COW filesystems) | Btrfs picks max(generation) across all mirror superblocks; ZFS picks max(txg) walking the 128-slot uberblock ring backward (magic 0x00BAB10C); ReFS chains CHKP→SUPB→MSB pages. `artifacts/SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md §2–§4` |
| 1.14 | ZFS dead-pool reconstruction | Vdev labels probed at 4 offsets × 2 copies per disk; hand-rolled nvlist parser recurses `vdev_tree` (type 9) with big-endian u64 accumulation to recover geometry. `artifacts/SearchDLL/zfs-engine--deep-dive.md §2`; `artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md §1` |
| 1.15 | SSD/TRIM data-loss advisory | "TRIM OFF/ON" home-page warnings present recovery-affecting health facts (paired with DiskHealth companion). `artifacts/DiskHealth/diskhealth--smart-companion.md §3` |

## 2. Filesystem-specific edge cases (per-FS)

| FS | Corrupt-structure case the og detects and survives (cited) |
|---|---|
| NTFS | USA/fixup validation per record (`fixup_validate`: min 0x400 parse size, zero fixup-offset reject, USN match per sector, reapply step); fixup failure skips the record, never aborts the walk. `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §2`; `artifacts/SearchDLL/ntfs-engine--deep-dive.md §9` |
| NTFS | BAAD/'FILE0' corrupted-record family discriminated and skipped; secondary corruption check FUN_180034b20. `artifacts/SearchDLL/ntfs-engine-internals.md §MFT-acquisition item 5`; `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §5` |
| NTFS | Deleted-record viability: data runs cross-checked against $Bitmap; fully re-allocated clusters → candidate destroyed; partially-intact still surfaces; files <1023 bytes fast-path with no bitmap check (strict `0x3ff < size`, CORRECTIONS row 7). `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1`; `re-workspace/artifacts/CORRECTIONS.md` row 7 |
| NTFS | $MFTMirr fallback: fixed-location estimate first; on failure reads MFT record 1's own $DATA runlist (survives moved/fragmented mirror); resident-mirror and empty-runlist aborts logged. `artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md §2` |
| NTFS | Copy-on-wrap fixup semantics: parse buffer duplicated when copy_flag set so fixup application cannot corrupt the shared scan buffer. `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §2` |
| FAT | Tolerant EOC termination: 0xFFFFFFF8 exact, 28-bit mask family, 24-bit dirty/reserved variants, alternate 0xFFFFFF0 forms — real-world BIOS-written chains tolerated. `artifacts/SearchDLL/fat-engine--deep-dive.md §3` |
| FAT | BAD cluster canonicalization: FAT16 0xFFF7 → canonical 0xFFFFFF7; EOC ≥0xFFFFFF8 (but FAT12 BAD 4087 followed RAW, garbage-tolerant — CORRECTIONS row 3: og does NOT canonicalize at the 12-bit boundary). `artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md §1`; `re-workspace/artifacts/CORRECTIONS.md` row 3 |
| FAT | Torn FAT1: `CDoubleFATTable` consults the mirror FAT2 when primary walk hits corruption (fallback switch [INFERRED] in artifact). `artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md §1`; `artifacts/SearchDLL/fat-exfat-engine-internals.md §Recovery-relevant behaviors` |
| FAT | Deleted 8.3 entries (0xE5) kept with 64KB-chain-length sanity guard (`< 0x10000` clusters) bounding corrupt chains; LFN fragments checksum-verified against 8.3 alias. `artifacts/SearchDLL/fat-engine--deep-dive.md §4` |
| FAT | Chain truncation tolerated: partial-file recovery, size = min(entry size, chain walk). `artifacts/SearchDLL/fat-exfat-engine-internals.md §Recovery-relevant behaviors` |
| exFAT | Dentry triple validation: volume-label sequence (0x81/0x82 following), bitmap descriptor checks, stream-extension reserved-zero fields + first-cluster ∈ {1} ∪ EOC-family, UTF-16 name-fragment validation; deleted triples (0x80 cleared) reconstructed identically by backward walk. `artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md §3` |
| exFAT | Boot-sector self-verification: ExFATFixer::`ChecksumDBR` (CRC32) — only fixer with a self-verifying boot sector; `CheckMustBeZero` reserved-field probe. `artifacts/PartitionRecovery/rawfixer-fs-repair.md` (method table) |
| ext4 | Superblock gate: magic 0xEF53, nonzero inode/block/group counts, first_data_block × log_block_size cross-checks (1KiB→1, else→0), 64-bit block count, group-count geometry invariants. `artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md §1` |
| ext4 | 64-bit GDT feature: desc_size u16 honored (32- and 64-byte group descriptors both parsed). `artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md §2` |
| ext4 | Bad sectors: `CEXT4CatchBufferDeviceProxy` — read failure retried with fresh 0xE00 buffer and 1KB-backtrack shifted window; skip-and-salvage instead of failing the group scan. `artifacts/SearchDLL/ext4-engine--deep-dive.md §3` |
| ext4 | Journal salvage: gates ONLY on jsb magic 0xC03B3998 + blocktype {3,4}; every block in [s_first, s_maxlen) feeds the directory parser — no commit/revoke semantics (CORRECTIONS row 6). Journal-walk traces "GotJournalDir/TraverseJournalDir". `re-workspace/artifacts/CORRECTIONS.md` row 6; `artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (EXT4 key behavior) |
| HFS+ | Volume-header gate: "H+" or "HX" (HFSX), block size multiple of 512, version < 0x11, allocation/extents clamped to totalBlocks; BE decode first. `artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md §3`; `artifacts/SearchDLL/hfsplus-engine--deep-dive.md §1` |
| HFS+ | Fragmented forks: Extents Overflow B-tree followed for forks with >8 extents. `artifacts/SearchDLL/hfsplus-engine--deep-dive.md §2` |
| HFS+ | Deleted files (catalog overwritten on delete): orphaned-cnid sweeps + RAW carving engine pick up what the catalog lost. `artifacts/SearchDLL/hfsplus-engine--deep-dive.md §2` |
| ReFS | Page signatures "SUPBMSB+ReFSFSRS" / "CHKPSUPBMSB+ReFSFSRS" / "MSB+ReFSFSRS" chained through RefsBaseDataImplManager; OS-version gating distinguishes ReFS metadata layouts per Windows build. `artifacts/SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md §1`; `artifacts/SearchDLL/refs-engine--deep-dive.md §3` |
| Btrfs | Superblock invariants: magic `_BHRfS_M`, sector/node size power-of-two, node-size sane band 0x86..0x86+0xFF7B, node%sector==0; newest-generation mirror wins. No chunk-tree-uuid validation (CORRECTIONS row 8). `artifacts/SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md §2`; `re-workspace/artifacts/CORRECTIONS.md` row 8 |
| Btrfs | Compressed/encrypted datasets: ZLIB inflate for zlib extents; RC4 PRGA for encrypted datasets; crc32c/adler32 checksums. `artifacts/SearchDLL/btrfs-engine--deep-dive.md §2`; `artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (BtrfsSearchDLL) |
| ZFS | Uberblock ring: 128-slot × 1KiB walk, magic 0x00BAB10C + txg ordering; delay-execution polling with retries/backoff across vdev offsets. `artifacts/SearchDLL/zfs-engine--deep-dive.md §2–§3` |
| (all) | Engine errors never abort the product: HRESULT-style 0x5002xxxx codes (e.g. open-fail, read-fail) surface through controller plumbing; cout diag "Return Error:0x… File… Line… Fuction…" compiled into release. `architecture/05-scan-engine-architecture.md §4`; `artifacts/SearchDLL/hfsplus-engine--deep-dive.md §3` |
| (all) | APFS: the corpus contains no APFS engine (FS codes 0-2/3/4-5/7/8/9/10/13+Btrfs only) — no APFS edge cases exist to mirror. `architecture/05-scan-engine-architecture.md §2` |

## 3. Carving / validation edge cases (RAWSearchDLL)

| # | Edge case | What the og does (cited) |
|---|---|---|
| 3.1 | Adversarial/mimic magic bytes on damaged media | Dual validation: registry magic + internal re-validation (e.g. WPG `0x677077` re-check) minimizes false positives where random data mimics magics. `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §5.4` |
| 3.2 | Two-generation analyser chain | Modern (namespaced) analyser, then legacy `File*Analyser` fallback: two independent validators must both fail before a match is discarded; one strong accept suffices. `artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md §2` |
| 3.3 | Overlapping carve candidates | Header+tail+size-gate confidence combination ranks overlapping candidates (raw_signs.json confidence fields, e.g. ifo `0,44564456,0.96`). `artifacts/SearchDLL/rawsearch-carving--deep-dive.md §3`; `artifacts/SearchDLL/searchdll-family--architecture.md §4.4` |
| 3.4 | ZIP with destroyed central directory | All three ZIP structures accepted as anchor (LFH / CD entry / EOCD-first truncated zips); indexless result carries status 0xd vs 0x48 full carve — "recoverable but structure incomplete" surfaced differently. `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §1, §2` |
| 3.5 | Deleted archives followed by zero padding | Forward-scan through zero padding (last 4 dwords of each sector zero) with **5.24 MB cap** (0x500001); clean boundaries instead of multi-MB zero tails. `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §2` (padding-extension loop) |
| 3.6 | Formats with no crisp tail marker | `BruteForce::*` analysers walk forward to EOF validating structure at every step; report last valid position as size — "maximize salvage" trade-off (more false-positive tail, no premature truncation). `artifacts/SearchDLL/rawsearch-carving--deep-dive.md §5` |
| 3.7 | Multi-generation formats (PST ANSI vs Unicode) | Version byte 0x0E/0x17 selects size field at 0xA8 vs 0xB8; unknown versions floored at 1 MiB. RAR4 fast validator keys marker+version; RAR5 rides 4CC/fallback tables. `artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md §1–§2` |
| 3.8 | Pathological element sizes (EBML/MKV) | Complete vint decoder: max 8-byte length, element bounds header ≤ 0x200, else reject. `artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md §3` |
| 3.9 | JPEG structure validation | Marker walk incl. SOF0 dimension checks with 8-bit precision (`0xC0 && precision==8`). `artifacts/SearchDLL/rawsearch-carving--deep-dive.md §3` |
| 3.10 | User-extensible signatures | **Correction:** EUUserData never reaches the RAW carving engine (zero references in 80,614 lines); sole consumer is the CHK repair chain. `GetRawUserInfoMap` is per-scan carve-statistics output, never input. `re-workspace/artifacts/CORRECTIONS.md` row 9 |
| 3.11 | Structured-vs-carved merge | Structured nodes win; carved nodes attach as "Lost"/"Other deleted files"; cross-correlated in merge phase (repair side: regroupfile.cpp "use filesystem DataRun"). `architecture/05-scan-engine-architecture.md §7`; `artifacts/PartitionRecovery/rawfixer-fs-repair.md §Why this matters` |
| 3.12 | Carver self-test | `EUCreateRAWSearcherForCellTest` + `CRawFileCellTestStorageDevice` — synthetic sector images feed carvers in a shipped unit-test harness. `artifacts/SearchDLL/rawsearch-carving--deep-dive.md §1` |

## 4. Recovery / writer edge cases

| # | Edge case | What the og does (cited) |
|---|---|---|
| 4.1 | Same-disk (source) recovery | Destination validated by opening `\\.\X:` raw volume of destination (writability + same-volume detection); UI hard-blocks "Cannot save to original partition" + warning ladder ("Data might be %1 and %2…"). `architecture/04-recovery-workflow.md §7 step 3`; `artifacts/UIUX/11-recovery-phase-funnel.md §2` |
| 4.2 | Invalid / too-long destination path | "Might be invalid path to save." / "2.The recovery path is too long." — numbered error taxonomy shared with the recovery log. `artifacts/UIUX/11-recovery-phase-funnel.md §2` |
| 4.3 | Network destination | `PathIsNetworkPathW` branch → attribute checks (vs local raw-volume open). `architecture/04-recovery-workflow.md §7 step 3` |
| 4.4 | Per-file failures mid-export | Failures collected for the recovery log: "%1 files/%2 failed; for details, you may check the recovery log" — partial success is a first-class result state. `architecture/04-recovery-workflow.md §7 step 5`; `artifacts/UIUX/11-recovery-phase-funnel.md §3` |
| 4.5 | Path separators / reconstruction | Dispatcher normalizes separators (`replace('\\','/')`), reconstructs paths via find_last_of('/'), `CreateDirectoryW` for destination dirs. `architecture/04-recovery-workflow.md §7 steps 4–5` |
| 4.6 | Illegal characters in saved names | File-name sanitizer replaces `/ * : ? " < > |` with `_` (exact set, byte-loop) — used for .rsf session names and recovered-file naming. `artifacts/SaveScan/rsf-format--scan-session.md §5.5` |
| 4.7 | ACL preservation | `CAccessControlList`/`CFileSecurityInfo` epmpublic utilities preserve NTFS security descriptors (DACL/SACL) when files are copied — recovered files keep original ACLs. `artifacts/PartitionRecovery/pending-operation-engine.md` (shared utilities) |
| 4.8 | Locked / in-use volumes | EUDCPEPM/EUEDKEPM hook drivers (device-stack UpperFilter) + volume lock via the pending-op engine; lock/hold = custom IOCTL modes 3/4 pair. `architecture/04-recovery-workflow.md §10`; `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (command table 0x800C2B18/0x800C2B20) |
| 4.9 | Damaged-volume source reads during export | Source reads come from the engine/scan-session abstraction (IFileProperties/IInWindowStream), which reads through EPMNtDrv raw sector IO; writes go via normal file APIs. `architecture/04-recovery-workflow.md §7 step 5` |
| 4.10 | Free-quota exhaustion mid-recovery | CRecoverLimitDlg gate: "Recovered: %1/%2 files", Give Up vs Recover Remaining; hard-stop error "6.The data amount you want to recover has exceeded the free quota." `artifacts/UIUX/11-recovery-phase-funnel.md §4` |
| 4.11 | Progress truth on completion | Progress forced to 100.0 (0x4059000000000000) on completion callback; "Recovery Successful"/"Recovered: %1/%2 files" surfaced. `architecture/04-recovery-workflow.md §7 step 3, §7 result` |

## 5. Cancel / stop semantics

| # | Guarantee | Evidence (cited) |
|---|---|---|
| 5.1 | Cooperative cancel thread | `CStopScanThread` cancels the engine chain cooperatively (UI confirm dialog first: "Stop searching for lost data?" with risk framing). `artifacts/DRWUI-exe--thread-architecture.md §1`; `artifacts/UIUX/08-scan-phase-deep-dive.md §3, §6` |
| 5.2 | Per-record cancel checks inside engine loops | NTFS deleted-batch loop: `controller.cancel_callback.vt[0x10]() != 0 → break` (checked per record); FAT chain walk: `cancel.vt[0x88]() → break` (per cluster); HFS+ B-tree sibling walk: per-node cancel + EOF checks. `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1`; `artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md §2`; `artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md §4` |
| 5.3 | Cancel during session save/restore | Record-4 writer/reader check `IUIProgressInfo::IsCancelled` (vtable+0x20) after every child; on cancel the writer tears down all 3 lists and returns 0; the reader additionally calls EndRecord before aborting. `artifacts/SaveScan/rsf-format--scan-session.md §5.4.4` |
| 5.4 | Organize-phase cancel | "Stop Organizing Found Files?" — the tree-building phase (classify/sort workers) is itself cancellable. `artifacts/UIUX/08-scan-phase-deep-dive.md §3` |
| 5.5 | Repair-coupled cancel | "Stop Repairing for corrupted video?" / "The corrupted videos will not be repaired if you stop." — repair-during-recovery pipeline has its own stop contract. `artifacts/UIUX/08-scan-phase-deep-dive.md §3` |
| 5.6 | Driver-level abort budget | EUDCPEPM CANCEL/ABORT IOCTL: abort state 3, bounded kick counter `session+0x80 < 301` — worker-notify retry budget of 301 (no infinite abort spinning). `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (command table 0x800C2B38) |
| 5.7 | NAS/Linux/cloud connect cancel | Dedicated cancel threads (`CNasCancelConnectThread`/`CLinuxCancelConnectThread`) — the triple-connector pattern gives every connect a cancel twin. `artifacts/DRWUI-exe--thread-architecture.md §2` |
| 5.8 | Note: `CancelSynchronousIo` | The corpus does not name `CancelSynchronousIo` anywhere; the documented mechanism is the vtable cancel-callback contract above (searched all corpus .md). — |

## 6. Repair / health edge cases

| # | Edge case | What the og does (cited) |
|---|---|---|
| 6.1 | Bad sectors during scan | UX contract "Program is trying to skip bad sectors to find data." (config.ini BadSectorTip); EXT4 CatchBuffer proxy implements skip-and-salvage most literally. `artifacts/config-files/config-data-reference.md` (config.ini section); `artifacts/SearchDLL/ext4-engine--deep-dive.md §3` |
| 6.2 | Transactional FS repair | Every RawFixer fixer implements `Undo`; exFAT undo via `CopySectors` undo-log (original sectors copied aside before any write, replayed back). `artifacts/PartitionRecovery/rawfixer-fs-repair.md`; `artifacts/PartitionRecovery/rawfixer-v2--method-inventory.md §1, §4` |
| 6.3 | FAT32 backup boot sector | `CheckBackupDBR` validates sector 6 (FAT32 backup DBR) as repair source. `artifacts/PartitionRecovery/rawfixer-v2--method-inventory.md §5` |
| 6.4 | Corrupt video files | Weighted ISO-BMFF atom grammar, "file has no moov"/"is bad moov" validation, moof-fragment pre-align, per-run verify ("org data run is correct, will not fix it"), regroup rebuild; advanced mode transplants a healthy sample file's box template. `artifacts/FileRepair/repair-architecture.md §2` |
| 6.5 | SMART query transport variance | CAtaSmart multi-transport accessors: SAT/ATA passthrough variants (-1A/-2/-2A), CSMI (Intel RST arrays), NVMe-Realtek bridge; USB-bridge database ("AddDisk - ASM1352R" etc.); interface model incl. `INTERFACE_TYPE_NVME` and virtual NVMe disks. `artifacts/DiskHealth/diskhealth--smart-companion.md §1–§2` |
| 6.6 | Health-state grammar | "Good"/"Caution!"/"Normal"/"Your disk may have a physical failure:" + SMART attribute grid (ID/Attribute/Current/Worst/Threshold/RawValue/Status); jumps to first Caution disk. `artifacts/UIUX/19-diskhealth-quick-ui.md §3` |
| 6.7 | Physical-failure advisory + repair pairing | DiskHealth pairs with DRW's "Rescue Crashed PC"/bad-sector flows; notification can launch the console ("Click_Open_DiskHealth_By_Notification"). `artifacts/UIUX/19-diskhealth-quick-ui.md §4`; `artifacts/DiskHealth/diskhealth--smart-companion.md §3` |

## 7. Session / persistence edge cases (.rsf)

| # | Edge case | What the og does (cited) |
|---|---|---|
| 7.1 | Corrupted compressed chunk in session file | On any decompression failure the reader falls back to memcpy of the raw payload — graceful degradation for corrupted files. `artifacts/SaveScan/rsf-format--scan-session.md §3.2` |
| 7.2 | Incompressible data | Compressed chunk only emitted if smaller than raw; otherwise stored form (explicit size comparison) — chunk-level format never grows data. `artifacts/SaveScan/rsf-format--scan-session.md §3.2` |
| 7.3 | Session restore identity check | Record 3 stores the "device identity card": disk model/serial/firmware/WWN/interface + full 512-byte boot-sector snapshot — enough to verify the same volume is being re-attached and re-materialize the partition context without re-enumerating. `artifacts/SaveScan/rsf-format--scan-session.md §5.3` |
| 7.4 | Reader robustness (record lookup) | Chain-walk by 16-byte session id; non-matching records skipped by `chain` length; record-4 grammar validated byte-exact to EOF on 1,986,806 nodes across 3 shipped samples. `artifacts/SaveScan/rsf-format--scan-session.md §2.1, §8` |
| 7.5 | Crash / resume-after-crash | DRWLoader watchdog: DRW_Crash / DRW_Memory_Crash (separate OOM vs SEH) named events → DRWReport.exe minidump + Elapsed/Error/Result JSON → DRWUI restart; crash-loop guard limits auto-restarts (`1 < uVar17 → break`); 5 s shared-log flusher keeps evidence current while UI hangs. `artifacts/DRW-exe--launcher.md §2–§4` |
| 7.6 | Auto-save sessions | `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` naming; SaveScan folder derived from module dir; enumeration skips "."/"..". `artifacts/SaveScan/rsf-format--scan-session.md §5.5` |
| 7.7 | Concurrent instances / log races | Named-mutex arbitration (`DRWLOG_{GUID}`) over the 2 MB shared log mapping; loader copies the whole mapping under the mutex. `artifacts/DRW-exe--launcher.md §1–§3` |
| 7.8 | Re-scan performance on Btrfs | `BtrfsCache.txt` metadata cache persisted in the DLL directory to speed re-scans (subvolume/chunk trees cached). `artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (BtrfsSearchDLL) |

## 8. Resource limits (documented bounds in the original)

| # | Bound | Value + citation |
|---|---|---|
| 8.1 | MFT read batch | 64 KB (0x10000 stack buffers = 16×4KB or 64×1KB records); batch loader reads 4 KiB pages (0x1000-granular) unpacked into record-size slots. `artifacts/SearchDLL/ntfs-engine-internals.md §MFT-acquisition item 2`; `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §5` |
| 8.2 | NTFS record parse floor | Record size from $MFT, clamped to ≥ 0x400 (1024) bytes; fallback 0x200; min parse capacity max(0x400, record_size). `artifacts/SearchDLL/ntfs-engine--deep-dive.md §3`; `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1` |
| 8.3 | FAT corrupt-chain bound | 64KB-chain-length sanity guard (`*param_7 < 0x10000` clusters) on deleted entries. `artifacts/SearchDLL/fat-engine--deep-dive.md §4` |
| 8.4 | Deleted-viability threshold | Files ≥ 0x400 bytes (strict `size >= 0x3FF` i.e. >1023) get the bitmap walk; smaller fast-path. `artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1`; `re-workspace/artifacts/CORRECTIONS.md` row 7 |
| 8.5 | Memory-adaptive batch sizes | GlobalMemoryStatus logging ("Total physical memory… Available… page files…") — batch sizes adapt to RAM. `artifacts/SearchDLL/ntfs-engine-internals.md §IO & robustness` |
| 8.6 | .rsf compression buffers | 4 KB input staging from 512 KB (0x80000) buffer; 8,190-byte output buffer; zlib level 5; chunk payload ≤ 4096 bytes; CRSDataEx object 528,632 bytes. `artifacts/SaveScan/rsf-format--scan-session.md §3.2, header` |
| 8.7 | Shared log ring | 2 MB file mapping, flushed every 5 s. `artifacts/DRW-exe--launcher.md §1–§2` |
| 8.8 | Driver IO unit | 512-byte sector contract, read length expressed in sectors (`Information = Length << 9`); MDL async with completion event wait. `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md §3` |
| 8.9 | Abort retry budget | 301 kicks (0x12D) bounded counter. `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (0x800C2B38) |
| 8.10 | Preview timer | 15 s free-tier video preview cap (QTimer on VideoPreviewWnd). `artifacts/UIUX/18-preview-architecture.md §1, §3` |
| 8.11 | Tree-build batching | ~1000-item incremental batches (BuildTree cycles) on a 223M-item cumulative workload — incremental construction keeps UI responsive. `artifacts/config-files/qa-logs--session-forensics.md §4` |
| 8.12 | Worker model | Threaded per-worker quick scan (CFATQuickSearchWorker, 13+ ctor params); Vista+ condition variables dynamically resolved with pre-Vista fallback; every long UI operation a QThread subclass. `artifacts/SearchDLL/fat-engine--deep-dive.md §1`; `artifacts/SearchDLL/ntfs-engine-internals.md §IO & robustness`; `artifacts/UIUX/13-ui-core-wiring-state.md` |
| 8.13 | Threading safety of result objects | Reference-counted property objects with interlocked AddRef/Release (LOCK/UNLOCK) — engine is thread-safe for the UI's async workers. `artifacts/SearchDLL/ntfs-engine--deep-dive.md §3` |
| 8.14 | Zip padding cap | 5.24 MB (0x500001) zero-padding extension cap. `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §2` |

## 9. Error paths and user-facing failure grammar

| Failure class | The og's grammar (cited) |
|---|---|
| Engine open/read failures | HRESULT-style facility codes `0x5002xxxx` (e.g. 0x5002200000004 open-fail, 0x5002c0000004 read-fail); HFS+ family `0x5017900000002`. Verbose diag channel: `"Return Error:0x%x File:%s Line:%d Fuction:%s"` [sic] in release builds. `architecture/05-scan-engine-architecture.md §4`; `artifacts/SearchDLL/hfsplus-engine--deep-dive.md §1, §3` |
| Volume/FS check failures | Check-phase classes CCheckNtfsPartition/CCheckFatPartition/CCheckExFatPartition/CCheckBitlockerPartition pair with the fixers; "Fix failed: {}\n" error-catch at CRawFixer::Start. `artifacts/PartitionRecovery/rawfixer-fs-repair.md` (Role + CRawFixer table) |
| Recovery-failure taxonomy (numbered, 1–6+) | "2.The recovery path is too long." / "6.The data amount you want to recover has exceeded the free quota." / "Might be invalid path to save." — numbered list shared with the recovery log. `artifacts/UIUX/11-recovery-phase-funnel.md §2, §4` |
| Per-file export failures | "%1 files/%2 failed; for details, you may check the recovery log". `architecture/04-recovery-workflow.md §7 step 5` |
| Drive/NAS/Linux/Cloud disconnects | "Drive Disconnected" / "NAS Disconnected" / "Disconnect with Linux" / "Cloud Drive Disconnected" — each with Retry/Cancel(/Disconnect) controls; generic "Check your network to make sure the %1 server is accessible and then r[etry]". `artifacts/UIUX/08-scan-phase-deep-dive.md §5` |
| NAS connect failure | "Incorrect IP or port or disabled SSH service"; retry logic in CRemoteDevice::Connect ("retry %d"). `architecture/04-recovery-workflow.md §10` |
| Stop-confirm grammar | "Stop searching for lost data?" → "Remaining data can be rescanned and recovered anytime later, but continuing now …" (risk framing). `artifacts/UIUX/08-scan-phase-deep-dive.md §3` |
| License failure states | Expired / Invalid length / Invalid / Locked / Pirate license code; anti-tamper "host settings have been maliciously tampered…" → PirateVictimsTipDlg. `artifacts/UIUX/11-recovery-phase-funnel.md §4` |
| Indexless-archive recovery | Carve status 0xd (partial/indexless zip) vs 0x48 (full) — distinct UI presentation. `artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §2` |
| Crash telemetry | Loader JSON `{"Elapsed":"%I64d","Error":"%d","Result":"%s"}` → DRWReport minidump → send_dmp_with_aliyun upload. `artifacts/DRW-exe--launcher.md §2` |
| Smart-partition scan states | "Smart Partition Scan Completed" / "Scan completed for Disk %1!" / stopped: "Smart Partition Scan Stopped" ("Scanned disk sectors: %1"). `artifacts/UIUX/08-scan-phase-deep-dive.md §2` |
| Empty-recycle-bin result | `RecycleBinScanNoRltWnd` dedicated result state. `artifacts/UIUX/08-scan-phase-deep-dive.md §6` |

## 10. Must-test matrix (fuzzer / stress intersection)

Rows = the edge cases our fuzzers/stress runs must cover; every row needs a
validation/bounds/cancel answer in `final-prod` (engine parity target).

| # | Test | Inject | Must not | Ref |
|---|---|---|---|---|
| 10.1 | FAT cycle bomb | FAT chain pointing backward / self-loop | hang or unbounded growth — visited-range guard must terminate | §1.4, §2-FAT |
| 10.2 | NTFS fixup garbage | Random USA offsets/values in MFT records | abort the walk or corrupt the shared scan buffer (copy-on-wrap) | §2-NTFS |
| 10.3 | $ATTRIBUTE_LIST fragmentation | MFT record 0 with 0x20 attribute + multi-record chase | mis-assembled $DATA runs or crash | §1.3 |
| 10.4 | Bitmap-boundary sectors | bitmap test at pos ≥ limit / NULL buffer | OOB read — must return false (bounds test code path) | §2-NTFS (FUN_18002c2e0) |
| 10.5 | Overlapping lost-partition candidates | 15+ overlapping candidates incl. 0-file ghosts | wrong winner — fileCount-ranked resolution | §1.2 |
| 10.6 | 4Kn disks | Geometry with 4096-byte sectors | 512-byte assumption corruption; carvers must query real sector size | §1.9 |
| 10.7 | USB yank mid-scan | Device removal during MFT/superblock read | crash/hang — "Drive Disconnected" + Retry/Cancel | §1.10 |
| 10.8 | BitLocker locked volume | FVE volume, all protector types, wrong keys | plaintext leakage or hang; unlock fallbacks (pipe/PS/manage-bde) | §1.7 |
| 10.9 | Bad sectors | Read failures at group/superblock offsets | whole-group loss — skip-and-salvage with shifted-window retry | §2-ext4, §6.1 |
| 10.10 | Truncated/corrupt .rsf | Bad zlib chunk, truncated record 4, wrong session id | crash on load — raw-payload fallback + id chain-walk | §7.1, §7.4 |
| 10.11 | Cancel at every loop granularity | Cancel signal during record/cluster/child-node iteration | ignored cancel or torn state — writer tears down 3 lists, returns 0 | §5.2, §5.3 |
| 10.12 | Indexless ZIPs / EOCD-first archives | ZIP body with CD destroyed, zero padding after | wrong size (must cap 5.24 MB padding), lost archive (0xd status preserved) | §3.4, §3.5 |
| 10.13 | Mimic magics | Random data matching format magics | false-positive storm — dual validation + confidence ranking | §3.1–§3.3 |
| 10.14 | Same-disk recovery attempt | Destination on source volume | overwrite — raw-open detection + hard block | §4.1 |
| 10.15 | Pathological names | `/ * : ? " < > \|` in names, overlong paths | unrecoverable export failure — sanitize `_`, path-too-long error class | §4.2, §4.6 |
| 10.16 | Free-quota exhaustion mid-copy | Quota hit during export | silent truncation — Give Up/Recover Remaining gate + truthful counters | §4.10 |
| 10.17 | Newest-transaction selection | Btrfs mirror gens / ZFS txg rings out of order | stale-transaction reconstruction — max(gen)/max(txg) must win | §1.13 |
| 10.18 | Engine error propagation | Open-fail/read-fail at volume level | silent empty results — 0x5002xxxx-class codes must surface | §9 row 1 |
| 10.19 | 223M-item tree stress | Multi-hundred-million-node result trees | UI freeze — ~1000-item incremental batches + threaded workers | §8.11, §8.12 |
| 10.20 | Progress truth | Completion/cancel paths | stuck <100% or forced-fake progress — forced 100.0 only on completion | §4.11 |

---

*Corpus gaps (stated per rules): APFS — no engine exists in the og (§2 last row).*
*`CancelSynchronousIo` — not named anywhere in the corpus (§5.8).*
*Destination-full (ENOSPC) handling — the corpus documents the destination-validation
chain (`\\.\X:` raw open, writability) and the numbered recovery-log error taxonomy
(items 1–6+), but no explicit disk-full copy string; treat the taxonomy as the contract.*
