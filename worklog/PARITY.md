# PARITY — final feature-matrix resolution (cross-repo harvest)

> Every row of `worklog/FEATURE-MATRIX.md` resolved to exactly one of:
> **IMPLEMENTED** (with the commit and the test/validation that proves it),
> **DELIBERATELY-DIVERGENT** (the product's behavior intentionally differs —
> divergence documented with its reason), **NOT-APPLICABLE** (decision-register
> row / superseded / no reference implements it), or **FOLLOW-UP** (precise
> scope in `worklog/FOLLOW-UP.md` — the row is real work not yet done, stated
> truthfully, never claimed).
>
> Matrix was consolidated at `201100aa` (v0.9.5 baseline). Implementation
> landed across the harvest sessions (`cb2c8e0a` .. `126ea398`). Engine proofs:
> Linux local suite 1822/1822 (linux-release, -Werror) at `126ea398`; Windows
> proofs: the repo's GitHub Actions Windows lanes (run IDs cited per row).

## Wave 1 — Foundations

| Row | Resolution | Evidence |
|---|---|---|
| F-00a recovered-entry node + provenance | **IMPLEMENTED** (pre-harvest, verified) | PRODUCT-STATE §2.5; exercised by every suite; no delta found by the sheets |
| F-00b tree model + deleted/live classification | **IMPLEMENTED** (pre-harvest, verified) | M-103 LostDir roots; session round-trip tests (test_ntfs_volume.cpp) |
| F-00c parser primitives + abstract access layer | **IMPLEMENTED** (pre-harvest, verified) | PRODUCT-STATE §1.2 (IStorageDevice/Slice/Retrying/CachedReader); ABI tests green |
| F-00d extract-plan model (named feature's base) | **IMPLEMENTED** | `cb2c8e0a` — engine/src/core/recovery/cr_extractplan.{h,cpp}; test_extractplan.cpp 15 cases |

## Wave 2 — NTFS deep recovery

| Row | Resolution | Evidence |
|---|---|---|
| F-02 $MFT run-list reconstruction + in-place patch | **IMPLEMENTED** (0dc07535) | engine/src/core/ntfs/cr_mftpatch.{h,cpp} + cr_rawfixer.cpp ActionKind::RebuildMftRunlist (fires only after the mirror path refused, S3); 22 tests (test_mftpatch 20 + test_rawfixer 2 end-to-end: torn-runlist+torn-mirror -> plan -> execute -> NtfsVolume::open OK -> undo byte-for-byte); local 1843/1843; spec worklog/VALIDATION/specs/F-02-mft-runlist-patch.md |
| F-03 parent-sequence validation | **IMPLEMENTED** | `126ea398` — cr_mftwalk.cpp parent_link_valid gate at both attachment sites (phase B + GUID-dump host); NtfsVolume.ParentSequenceGateOrphansReusedParents (reuse→LostDir, delete-bump accepted, file-parent rejected, pseq==pcur untouched); suite 1822/1822 |
| F-04 USN fixup all-zero rejection | **FOLLOW-UP** | cr_fixup.cpp accepts usn==0 with zero tails today; FOLLOW-UP notes the og-parity salvage tension and the exact gate |
| F-05 $ATTRIBUTE_LIST extension ownership | **FOLLOW-UP** | M-008 BFS chase exists; ownership-match rule (base_ref seq AND attrlist seq) not applied |
| F-06 NTFS index-slack deleted names | **FOLLOW-UP** | remnant pass harvests in-use entries only (og parity by design); slack harvest scope in FOLLOW-UP |
| F-07 record-number triple fallback | **FOLLOW-UP** | remnant scan emits records; numbering strategy differs |
| F-08 sequence-aware record dedup | **FOLLOW-UP** | M-007 single_link_per_record is the standing behavior; cross-check note in FOLLOW-UP |
| F-09 $LogFile analysis/replay | **NOT-APPLICABLE** | decision register: no reference implements it (NR §3.3 truth table; SK LSN-only); I-049 stands |
| F-10 $UsnJrnl deleted-name harvest | **IMPLEMENTED** (pre-harvest, AHEAD) | V2/V3 filtered on USN_REASON_FILE_DELETE; UsnJrnlHarvest + V3Resync tests |
| F-11 MFT slack-space carving | **NOT-APPLICABLE** | decision register: NOT FOUND IN CODE at mft-rescue (MR §5); the remnant scan + GUID-dump pass covers the adjacent ground |

## Wave 3 — Partition layer

| Row | Resolution | Evidence |
|---|---|---|
| F-12 backup boot-sector fallbacks | **DELIBERATELY-DIVERGENT (core IMPLEMENTED)** | NTFS volume-end backup: M-043/M-145/M-141 read-only fallback + boot_from_mftmirr S-063 (cr_bootstrap.cpp, tests in test_ntfs_volume). FAT32 sector-6 backup: R5.1 (cr_fat_quick.cpp, FAT32-restricted, geometry-aware). exFAT +12 backup: backup-rebase path (cr_exfat_quick.cpp:43). Divergence: the product validates the backup through full BPB gates at the FS open (stronger than TD's identity-compare-with-tolerance), and the TD per-MiB lost-volume lattice positions belong to F-13's deep-scan scope — the fixed spec positions (6 / 12 / volume-end) are what the open-path fallbacks consume |
| F-13 quick-mode lattice + hint list | **FOLLOW-UP** | 64 KiB full-sweep stride is thorough-but-slower by design; lattice scope in FOLLOW-UP |
| F-14 GPT table dual-write repair | **FOLLOW-UP** | GPT-damage is reported today; reconstruction scope (consent + undo) in FOLLOW-UP |
| F-15 $MFTMirr repair adjudication | **FOLLOW-UP** | fscheck $MFTMirr proofs + RawFixer record-0 restore exist; TD's listing-based winner-pick heuristic in FOLLOW-UP |
| F-16 EBR chain caps | **FOLLOW-UP** | visited-set cycle guard + torn-chain salvage (M-002) exist; the 32-logical cap unverified |
| F-17 Mac/BSD/Sun partition systems | **FOLLOW-UP** | MBR/GPT/LDM only; P3 |
| — GPT backup-header salvage | **IMPLEMENTED** | pre-harvest (closed per matrix note; PS §2.1) |

## Wave 4 — Carving

| Row | Resolution | Evidence |
|---|---|---|
| F-18 signature registry breadth | **FOLLOW-UP** (EXISTS-PARTIAL) | 155 grammar + 139 raw + user = 294 types; ~59-type gap scoped in FOLLOW-UP (family-batched, per-type validators + tests) |
| F-19 bounded fragmentation reassembly | **FOLLOW-UP** | opt-in backwards split-point search; full scope in FOLLOW-UP |
| F-20 two-level signature index | **FOLLOW-UP** | dispatch mechanism unverified vs the 256-bucket form |
| F-21 auto block-size detection | **FOLLOW-UP** | product scans every sector in 1 MiB windows |
| F-22 structural end-detectors | **PARTIAL → FOLLOW-UP** | JPEG/ZIP/TIFF/PST/MOV gates exist; PDF-incremental, MP4-completeness, OLE-FAT-sizing scoped |
| F-23 carve outcome honesty | **IMPLEMENTED** | with the named feature: per-item Complete/Incomplete/Failed + content-honesty notes (cb2c8e0a F-45 landing); `read_error`/`zero-filled`/`reused` note flags in the report schema (validator test_validate_report) |
| F-24 embedded-filename rename | **FOLLOW-UP** | P3 |
| F-25 low-memory forget() | **FOLLOW-UP** | P3 |
| F-26 whole/free-space gating | **IMPLEMENTED** (pre-harvest, AHEAD) | free-map cluster gate M-005 + occupancy gate M-039 |
| F-27 dedup + result merging | **IMPLEMENTED** (pre-harvest, AHEAD) | (first-cluster, size-class, hash) + extent-key dedup |

## Wave 5 — The named feature

| Row | Resolution | Evidence |
|---|---|---|
| **F-01 reconstructed deleted files with original directory structure, deleted-only, post-scan option** | **IMPLEMENTED** | `cb2c8e0a` (W1+W5): cr_extractplan plan walks (deleted-only listing fast-path, whole-subtree pruning, live ancestors as path components, `_orphaned/` + id-suffixed orphan roots, carved/raw excluded, depth 128); SafeWriter SfId scheme (`name (id N).ext`, second-collision refusal ERR_DEST_NAME_EXHAUSTED, never overwrite); sanitize_component_sf (reserved names incl. COM¹²³, trailing dot/space, UTF-16 240 cap); arm_output_guard + canonicalize (ERR_DEST_ESCAPE); EFS-encrypted → empty + note; ABI 95→99 exports (preview/enqueue deleted-tree); bridge command + connector + results-explorer action + recovery page mode + i18n; CLI `--reconstruct-deleted`. Tests: test_extractplan.cpp 15 cases + writer/opt-in pins. F-01b dot-transparency + fixture durability/rooting/probe fixes `be7966c5`. Windows closed-loop: named-feature-fixture lane, engine-flow job on the seeded 2 GB GPT/NTFS VHDX (run chain `44370686000`→`44391734922`→ run on `d65a5d12` — the run-14 red root-caused to the VERIFIER's dir-row defect, fixed `d65a5d12`, see VALIDATION/fixtures/named-feature/) |
| F-31 deleted-only listing fast-path | **IMPLEMENTED** | the F-31 pruned-walk in plan_deleted_tree (cb2c8e0a); pinned by the plan tests |
| F-45 per-item outcome badges + notes | **IMPLEMENTED** | RecoveryItem.outcome (complete/incomplete/failed) + note; UI badges on item rows (cb2c8e0a); the fixture verifier consumes the same honesty axes |
| F-28 FAT/exFAT orphan-dir subtree harvesting | **FOLLOW-UP** | deleted-slot + deleted-chain exist; the whole-subtree harvest scoped (HR §2.4/§2.5 gates) |
| F-29 exFAT allocation-bitmap authority | **FOLLOW-UP** | with F-28 |
| F-30 ext deleted dentry-slack names | **FOLLOW-UP** | deleted-inode + journal salvage exist; slack names scoped |
| F-32 FAT LFN checksum recovery | **IMPLEMENTED** (pre-harvest, AHEAD) | slot-set + first char via checksum brute-force (GAP-S6-1 closed); references only substitute `_` |

## Wave 6 — Imaging & drive health

| Row | Resolution | Evidence |
|---|---|---|
| F-33 two-pass ddrescue-style imaging | **IMPLEMENTED** | `7c7b62b6` — cr_rescue.{h,cpp} pass-1 forward/reverse + skip-max jump, pass-2 binary split to sector granularity with bounded retries; 12 tests incl. bad-patch isolated to exactly 1024 bad bytes, resume-never-redoes, wrong-size map refusal |
| F-34 rescue map persistence + resume | **IMPLEMENTED** | ddrescue-compatible mapfile (parse/round-trip/hostile-input tests), atomic tmp+rename with the 5×50 ms replace retry (cr_rescue_win.cpp), ≤5 s cadence + forced pass boundaries, refusal checks; win_syntax_check + engine-ci green (runs 44383135694/44383135656) |
| F-35 imaging pause/resume | **IMPLEMENTED** (subsumed by F-34's mapfile) | kill-safe: the mapfile is the commit marker (direct-to-final-path disposition), resume re-runs the same command with auto-resume detection + refusal checks (cr_rescue.h resume contract); cancel path: D-IMG-1 image_create_cancel end-to-end (76a6738c, connector tests) |
| F-36 sparse destination | **IMPLEMENTED** | FSCTL_SET_SPARSE best-effort on reverse/fresh (cr_rescue_win.cpp, 7c7b62b6); FAT/exFAT-destination advisory = FOLLOW-UP sliver |
| F-37 imaging presets + health-map UX | **FOLLOW-UP** | the engine knobs exist (chunk/skip/retries + CLI); the 4 presets + 1000-cell map viewer scoped |
| F-38 honest-partial reads with map-as-prior | **PARTIAL → FOLLOW-UP** | RetryingDevice zero-fill + ReadErrorMap (pre-harvest); the rescue mapfile-as-read-prior for the live-device carve path scoped |
| F-39 direct-access read modes (SCSI/ATA) | **FOLLOW-UP** | ATA passthrough exists for SMART; read-path OS-mode only |
| F-40 E01/AFF/split-raw image reading | **FOLLOW-UP** | flat/VMDK/VHD/VHDX exist (+ VHDX log-replay AHEAD); EWF/AFF scoped |
| F-41 same-drive guard | **FOLLOW-UP** | F12 extent-based hard refusal exists; unknown-backing case scoped |
| F-42 verify-history + survey banner | **FOLLOW-UP** | inspector + health UX exist (NG-5..10) |
| F-43 MR single-pass imager | **NOT-APPLICABLE** | superseded by F-33/F-34 (SF design strictly stronger; MR's progress.json never retries failed ranges) |

## Wave 7 — UX and output

| Row | Resolution | Evidence |
|---|---|---|
| F-44 evidence-weighted health scoring | **FOLLOW-UP** | 4-tier Confidence + og grading tree exist; the 0-100 numeric model + notes journal scoped |
| F-45 per-item outcome badges + notes | **IMPLEMENTED** | (Wave 5 row above — cb2c8e0a) |
| F-46 orphaned files pseudo-folder | **IMPLEMENTED** (pre-harvest) | "File Path Lost" root + LostDir groups (M-103); maps to `_orphaned/` in the F-01 plan |
| F-47 natural-language smart filter | **FOLLOW-UP** | generated filefilter vocabulary exists; NLQ scoped |
| F-48 mmls-style presentation | **DELIBERATELY-DIVERGENT** | the product's own partition table UI + diskmap cover the same presentation need; the 5-column mmls layout is forensic-console cosmetics, not a product gap |
| F-49 timeline / body-format export | **FOLLOW-UP** | forensic output; P3 |
| F-50 hash-database lookup | **FOLLOW-UP** | forensic; P3 |
| F-51 million-row column store | **PARTIAL → FOLLOW-UP** | virtual list + coalesced events exist; column-store unverified |
| F-52 recovery reports | **IMPLEMENTED** (pre-harvest, AHEAD) | cr_report.cpp + CI-validated schema + per-item SHA-256; `read_error` axis fixed with the W5 landing |

## Wave 8 — Persistence, session, performance

| Row | Resolution | Evidence |
|---|---|---|
| F-53 session save/resume cadence | **IMPLEMENTED** (pre-harvest, AHEAD) | CRSF + checkpoints + stop-window autosave N-211 |
| F-54 MFT batched reads (2 MiB) | **DELIBERATELY-DIVERGENT** | registered divergence D-51 (og parity work); measure-first note in FOLLOW-UP |
| F-55 read sizing / concurrency / buffering | **IMPLEMENTED** (pre-harvest, AHEAD) | IOCP ring + 64 KiB direct envelope + CachedReader |

## Wave 9 — Hardening

| Row | Resolution | Evidence |
|---|---|---|
| F-56 FAT per-sector copy fallback | **FOLLOW-UP** | table-level choose-healthy exists |
| F-57 distinct encryption-signature errors | **PARTIAL → FOLLOW-UP** | BitLocker FVE distinct; LUKS/FileVault/other signatures scoped |
| F-58 defensive hard caps | **PARTIAL → FOLLOW-UP** | product has own caps (depth 128 in the extract plan); cross-check scoped |

## Session-level validation state

- **Local Linux engine gate**: 1822/1822 tests, linux-release, gcc -Wall -Wextra
  -Werror -Wshadow, clang-format 18.1.8 clean (at `126ea398`).
- **Windows lanes**: engine-ci + bridge-integration in flight on `126ea398`;
  the named-feature-fixture F-01 closed loop in flight on `d65a5d12` (the
  verifier fix). Run IDs in `worklog.md`. ui-ci/ui-e2e green at `20efb62a`
  (no UI code changed since; worklog-only commits trigger no lane by the
  paths filters).
- **Release**: v0.9.6 tagged + built + published (release run 44391756880
  green, assets verified) — predates F-03/verifier fixes. v0.9.7 RC cut for
  the harvest set: see `worklog.md` final entry for its release + validation
  chain.
