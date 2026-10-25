# SUMMARY.md — executive summary

**Audited commit:** `67193b6` (with concurrent fixes through `7777e1c` in-tree where
noted) · **Corpus:** `03ad46c` · **Audit date:** 2026-10-25 · **Status: COMPLETE** (all
19 modules audited; every UNVERIFIED either resolved or tracked with a precise question).
**Remediation session in flight** (the diff-to-fix executor): findings are closed in
mandate order ACUTE → SEVERE → MAJOR → MINOR → INFO → tracked-open; resolutions and fix
commits land in each finding file; this summary's Remediation log tracks the counts.

## Counts per severity (final)

| Severity | Count | IDs |
|---|---|---|
| ACUTE | 1 | A-001 |
| SEVERE | 9 | S-001 … S-009 |
| MAJOR | 4 | M-001, M-002, M-005, M-006 |
| MINOR | 12 | N-001 … N-009 (N-004..N-006 and N-009a-d grouped in shared files) |
| INFORMATIONAL | 5 | I-001 … I-005 (I-005 groups 7 reviewed supersets) |
| tracked-open (UNVERIFIED) | 9 | OQ-T1 … OQ-T9 (4 files) |

## Counts per module (final)

| Module | ACUTE | SEVERE | MAJOR | MINOR | INFO | OPEN | Complete |
|---|---|---|---|---|---|---|---|
| ntfs | – | 1 (S-001) | 1 (M-001) | 1 (N-001) | 1 (I-001) | OQ-T1 | ✔ |
| fat | – | – | – | 1 (N-002) | 1 (I-002) | OQ-T2 | ✔ |
| exfat | – | – | – | 1 (N-003) | – | OQ-T5 | ✔ |
| ext4 | – | 2 (S-004, S-005*) | – | 1 (N-004) | – | – | ✔ |
| hfsplus | – | 3 (S-002, S-003, S-005*) | – | 1 (N-005) | – | – | ✔ |
| btrfs | – | 1 (S-005*) | – | 1 (N-006) | 1 (I-003) | – | ✔ |
| zfs | – | 1 (S-005*) | – | – | – | OQ-T6 | ✔ |
| refs | – | – | (project GAP: ADS) | 1 (N-007) | 1 (I-004) | – | ✔ |
| ldm/table | 1 (A-001) | – | 1 (M-002) | – | – | OQ-T7 | ✔ |
| lost-partition | – | 1 (S-006) | – | – | – | OQ-T9 | ✔ |
| carve | – | 1 (S-007) | 1 (M-005) | 1 (N-008) | – | OQ-T8 | ✔ |
| session | – | – | – | – | (I-005.7) | – | ✔ |
| writer | – | – | – | 2 (N-009b/d) | – | – | ✔ |
| health | – | – | – | 1 (N-009a) | – | – | ✔ |
| rawfixer | – | 1 (S-008) | – | – | – | OQ-T5 | ✔ |
| orchestration | – | (S-008 wiring) | (M-005) | – | – | – | ✔ |
| visualization | – | 1 (S-009) | (project GAP: filter cats) | – | – | – | ✔ |
| abi | – | (S-009) | – | – | – | – | ✔ |
| cli | – | – | – | 1 (N-009c) | – | – | ✔ |

*S-005 is a cross-engine finding (ext4/btrfs/ZFS mojibake), counted once per affected
module row for navigation.

## Top risks (ranked)

1. **A-001 — FIXED by the remediation session** (canonical mixed-endian disk bytes,
   ESP un-duplicated from MSR, `FsKind::EfiSystem` added; GPT dynamic disks parse their
   LDM database end-to-end — `Ldm.GptDynamicDiskCanonicalGuidsParse`; the audit's
   executed probe is a permanent test). The finding's byte-order table carried ESP/Linux
   in text order + a 31-digit MSR row — corrected per the corpus-wins rule (556-binary
   og scan: disk-order hits only).
2. **S-005 — filename mojibake** on ext4/Btrfs/ZFS: every non-ASCII filename on those
   filesystems recovers with garbage names (byte-lift instead of UTF-8 decode); the og
   passes UTF-8 through end-to-end (.rsf names are UTF-8; EXT4SearchDLL imports no
   conversion API — probe-verified).
3. **S-002/S-003 — HFS+ attributes tree (decmpfs) and journal both missing**: macOS
   compressed files recover as empty shells; crashed volumes lose journal-carried recent
   state.
4. **S-008 — RawFixer not wired pre-scan + boot-sector-only subset**: the og repairs
   MFT/FAT-table/root before scanning; on damaged metadata the og gets structured
   recovery and the core gets carving-only.
5. **S-006/S-007 — discovery coverage**: no lost-partition probes for Linux/Mac/ReFS
   filesystems (og has CExtXBlockParser et al.); carve misses GIF/BMP/headerless-MP3/
   AAC/RTF/OLE-Office/MPEG/DWG.
6. **S-001 — result-set semantics**: fully-overwritten deleted files are emitted
   (graded) where the og destroys them — every og-comparison inventory diverges
   mechanically until the policy is fixed or registered.
7. **OQ-T7 — LDM RAID-5 rotation unverified** vs LdmManager.mo: if the og's rotation
   differs from the assumed left-symmetric, every reconstructed RAID-5 member carries
   wrong bytes — resolve before shipping dynamic-disk recovery (and A-001 must land
   first or GPT dynamic disks never reach it).

## Recommended fix order (first 20)

| # | Finding | Rationale |
|---|---|---|
| 1 | A-001 | one header, restores GPT dynamic disks + correct classification |
| 2 | S-005 | shared UTF-8 decoder; fixes 3 engines + CLI filter + report |
| 3 | S-002 | decmpfs content recovery (HFS+ primary user scenario) |
| 4 | S-008 | pre-scan repair wiring + CheckMFT (damaged-volume on-ramp) |
| 5 | S-006 | lost-partition probe families (engine wiring only) |
| 6 | S-007 | carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG |
| 7 | S-003 | HFS+ journal replay |
| 8 | S-004 | ext4 journal every-block surface |
| 9 | S-001 | overwritten-destroy policy (decide + register) |
| 10 | S-009 | disk-map surface (ABI-CONTRACT §7 already specifies it) |
| 11 | M-002 | EBR torn-chain salvage |
| 12 | M-001 | NTFS record-size clamp ladder |
| 13 | M-005 | carve bitmap gating (perf + FP surface) |
| 14 | M-006 | re-enable fuzz CI |
| 15 | N-008 | ZIP padding/version/ZIP64 semantics |
| 16 | N-007 | ReFS first-DWORD discovery + grading split |
| 17 | N-001 | MFTMirr runlist span |
| 18 | N-002/N-003 | EOC families + entry-set checksum defaults |
| 19 | N-004..N-006 | layering/bounds minors |
| 20 | N-009a-d | grouped minors |

**Parallel RE track (Ghidra sessions):** OQ-T7 (RAID-5 rotation — safety-critical),
OQ-T1 (NTFS secondary claim map), OQ-T5 (exFAT ChecksumDBR predicate), OQ-T6 (ZFS
multi-vdev + nvlist anchor), OQ-T8 (carve dispatch order), OQ-T9 (CalcPartition scoring).

## Audit coverage statistics

- **Core read:** 52 src .cpp (24,793 LOC), 53 headers, 37 test files (248 TEST cases /
  ~15,077 LOC), build/CI/fuzz/tools/docs — agents C+D exhaustive.
- **Corpus read:** 9 architecture docs, 274 artifact docs (all SearchDLL 31 +
  PartitionRecovery 14 + SaveScan + config-files + UIUX + runtime-families + per-binary
  indexes), raw decompiled trees spot-verified (ghidra-searchdll/mo/partition), 38 PE
  surfaces dumped, IOCTL census, raw_signs.json + FileFilter.xml decoded — agents A+B+E.
- **Executed verifications this session:** core compiled standalone on Linux (libcr.a);
  table parser run on synthetic GPT/MBR/EBR fixtures (agent E) and on the canonical-GUID
  dynamic-disk fixture (A-001 proof); og-binary GUID scans; raw_signs.json parse +
  set-comparison vs core carve registry (S-007 proof).
- **Not executable here:** og-execution (no Windows/wine) and new Ghidra decompiles —
  all such questions are in `tracked-open/` with precise probes.

## Honesty notes

- The concurrent core-final session landed GAP-S6-1..4/6/7/8/13 fixes DURING this audit;
  all are in the audited tree and their behavior was re-read (each noted in the
  affected module files). FAT deleted-LFN/chain, ReFS detection, NTFS remnant $DATA,
  lost-partition dual discovery, metadata-only skip, resume carve dedup, and the 64 KiB
  transport envelope were verified as fixed in-tree.
- A parallel "diff-fix" audit session also writes to this worklog; its outputs had not
  landed in `diff-to-fix/` at audit close (no merge conflicts with this audit's files).
- The project's own PARITY-DELTAS/AUDIT docs were treated as claims, not truth: every
  gap they register that this audit touched was re-verified (ReFS ADS re-verified open;
  ZFS D-09 re-verified open; the false GUID-verification comment is exactly the kind of
  self-documentation this audit exists to catch).

## Remediation log (diff-to-fix executor session)

| Date | Finding | Verdict | Evidence |
|---|---|---|---|
| 2026-10-25 | A-001 | **FIXED** | canonical GUIDs + 4 regression tests; pre-fix probe reproduced ERR_BAD_MAGIC, post-fix suite 263/263; byte-order table corrected (finding + evidence file) |
| 2026-10-25 | M-001 | **FIXED** | record-size degradation ladder; 5 fail-before/pass-after tests incl. end-to-end torn-DBR remnant reachability |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run ratio sub-fix + counter; runtime fail-before/pass-after on EndToEndWalk; honest-mode pin test |
| 2026-10-25 | S-005 | **FIXED** | shared decoder + 3 engines + zfs pool name; fail-before/pass-after proven on all three engine tests |
| 2026-10-25 | OQ-T1 | **RESOLVED (decompile read)** | secondary claim map: same-direction (re-allocated), ctor-NULLed, never assigned anywhere in NTFSSearchDLL — dormant hook; write-up in tracked-open/OQ-T1 |
