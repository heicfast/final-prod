# AUDIT-FAT.md — Refactor-D: FAT12/16/32 + exFAT deep-parity audit (read-only)

> **Task ID:** Refactor-D. **Mode:** READ-ONLY audit + this one output file. No source
> modifications, no git state changes (verified: `git status` clean at completion; only
> this file + worklog append produced).
>
> **Pins:** core_recovery `08b44ac28bdcdd6263e7fe20fe0b597c141f87d9` (main);
> easeus-drw-research `18d6f14a1add9fc761ed7b3732bb1715b15aa79f` (main).
> **Date:** 2026-10-24 (session clock). **Auditor:** Refactor-D sub-agent.
>
> **Prior-claim context under test:** PARITY-AUDIT.md's F-A1 batch ("FAT12-EOC fixed,
> otherwise near-MATCH"). Re-verified against decompiled ground truth — several "MATCH"
> verdicts are **overturned** by newly decoded og semantics (see §5 Fix-state register).

## 1. Methodology

1. Read fully (our side): `src/core/fat/{cr_chain,cr_fat_quick,cr_lfn}.cpp`,
   `include/cr/fat/{cr_fat_engine,cr_fat_wire}.h`, `src/core/exfat/{cr_exfat_dentry,
   cr_exfat_quick}.cpp`, `include/cr/exfat/{cr_exfat_engine,cr_exfat_wire}.h`,
   `tests/unit/test_fat_volume.cpp`, `tests/unit/test_exfat_volume.cpp`,
   `fuzz/fuzz_fat.cpp`/`fuzz_exfat.cpp` (surface), orchestrator wiring (`cr_scan.cpp:549-566`).
2. Read fully (corpus): SearchDLL `fat-engine--deep-dive.md`,
   `fat-exfat-engine-internals.md`, `fat-exfat-engine-algorithms-v2.md`,
   `exfat-engine--deep-dive.md`; MOM `FatLib.md`, `FatFormat.md`,
   `FATFileSystemAnalyser.md`, `ExFatFileSystemAnalyser.md` (these four are
   auto-generated censuses — low signal, no algorithm content; noted in §6);
   `PartitionRecovery/rawfixer-fs-repair.md` (boot-repair ownership).
3. Decompiled ground truth: `FATSearchDLL_decompiled.c` (64,658 ln) and
   `exFATSearchDLL_decompiled.c` (58,043 ln). Every FUN_ cited by an artifact was
   located and read ±150 lines; additional functions decoded on own initiative:
   FAT `FUN_1800416f0` (CFAT12Table::Next), `FUN_180041de0` (CDoubleFATTable::Next),
   `FUN_1800365a0` (dentry plausibility gate); exFAT `FUN_180021e80` (boot validator),
   `FUN_180021c60` (FAT cache + free-entry continuation), `FUN_180002f40` (timestamp
   validator), inlined dentry-switch at exFAT 24379-24451.
4. Dead-code candidates Grep-verified across `src/include/tests/fuzz` (zero-call proof).
5. Verdict vocabulary: MATCH / MISMATCH / MISSING / DELIBERATE-DELTA / UNVERIFIED
   (inherited from PARITY-AUDIT.md). Line numbers are at the pinned commits.

## 2. FAT parameter table (checklist 1–9)

| # | Parameter | Ours (file:line) | OG / corpus (cite) | Verdict |
|---|---|---|---|---|
| 1 | Flavor by cluster count: <4085 FAT12, <65525 FAT16, else FAT32 | `cr_chain.cpp:12-19` (`<` boundaries) | MS whitepaper thresholds; og dispatches by partition-context flavor + OEM label `_strnicmp("FAT16   ")` (FAT decomp 38048), no count threshold | **MATCH (spec)** — count-only is registered delta #10 (F-A1-18) |
| 2a | FAT16 entry: BAD 0xFFF7→canon 0x0FFFFFF7; ≥0xFFF8→EOC 0x0FFFFFF8 | `cr_chain.cpp:55-59` | `FUN_180041c80` @54135-54141 — byte-exact | **MATCH** |
| 2b | FAT32 mask 0x0FFFFFFF; BAD 0x0FFFFFF7; EOC ≥0x0FFFFFF8 | `cr_chain.cpp:66-71` | mask-28 in `FUN_1800417c0` @53854; deep-dive §3 (8035-8060 masks) | **MATCH** |
| 2c | FAT12: BAD 0xFF7 / EOC ≥0xFF8 canonicalization | `cr_chain.cpp:44-47` (F-A1-1 fix) | `FUN_1800416f0` @53798-53804: 0xFFF→0xFFFFFFFF, 0xFF8-0xFFE→0xFFFFFF8, **0xFF7 returned raw** (followed as cluster!) | **MATCH on EOC** (F-A1-1 verified fixed); BAD diverges → FAT-4 |
| 2d | Reserved values 0xFF0-0xFF6 / 0x0FFFFFF0-6 | `cr_chain.cpp:113-119` → EOC_MIN | og returns raw → bounds-check kills next step (salvage-keep) | **MISMATCH (minor)** → FAT-5 |
| 3 | Mirrored FAT: primary-first, mirror-on-zero, cross-checks | `cr_chain.cpp:103-123` (mirror consulted when FAT1 = 0 **or** BAD; no cross-validation; divergence counter `:93-97`) | `FUN_180041de0` @54207-54251 [CODE]: primary wins when ≠0 (BAD included); mirror only when primary = 0; mirror result cross-validated (backward-pointer, +1-contiguity, EOC-family screens); corpus v2 §1 marked fallback [INFERRED] — now decoded | **MISMATCH** → FAT-2 |
| 4 | Cluster→offset `(c-2)*spc + first_data`·bps; FAT12/16 fixed root area; FAT32 root chain | `cr_chain.cpp:21-25, 285-293`; `cr_fat_quick.cpp:324-341` | decomp 11239 (`(uVar7-2)*P[4]+P[8]`); root area per BPB; artifacts v1 §3 | **MATCH** |
| 5 | Dentry walk: 0x00 end, 0xE5 deleted, LFN-before-8.3, checksum, volume-id skip, dots skip, cycle guard | `cr_fat_quick.cpp:130-177`; `cr_lfn.cpp:14-51`; visited-vector `cr_chain.cpp:182-195` | 0xE5 `-0x1b` ×6 sites (14634 + `<0x10000` 64KB guard); LFN attr 0x0F + 0x40 flag (44287, 28446); og visited = RB-tree range insert (decomp 11250-11266) | **MATCH (core)**; gaps: dot-identity validation (FAT-6), plausibility gates (FAT-7), deleted-LFN order (FAT-1), LFN slot first_cluster==0 (FAT-10) |
| 6 | Backup boot sector (FAT32) | `cr_fat_quick.cpp:285-301` — in-engine fallback at `6×logical_sector` for **all** flavors; ignores parsed `p.backup_boot_sector` | none found in FATSearchDLL — backup handling is RawFixer `FATFixer::CheckBackupDBR/FixFromDBR` (rawfixer-fs-repair.md) — pre-scan repair | **DELTA (registered)** + FAT-8 (flavor scope + hardcoded 6) |
| 7 | Date/time incl. tenths (+0xD, 10-ms units) | `cr_lfn.cpp:71-95` — tenths ignored (`(void)tenths`); month/day sanitize | og timestamp **validator** ranges decoded (`FUN_180002f40` exFAT mirror; FAT `FUN_1800365a0` @44307-44318 month/day gates); no epoch conversion found in SearchDLL | **MATCH (spec math)** / og conversion **UNVERIFIED**; tenths dropped → FAT-9 |
| 8 | Cancel polling in chain walk + dentry loop | `cr_fat_quick.cpp:119-120` (once per directory), `:332-341` (scan checkpoints); `follow_chain` none | og polls per chain-run (11235-11238, vt+0x88) **and** per dentry (28499-28505) | **MISMATCH** (F-A1-11 partial) → FAT-3 |
| 9 | Deleted name (0xE5 first byte) | `cr_fat_quick.cpp:156` `'_'` substitution | og persists raw 0xE5-marked bytes (ifileproperties census; F-A1-12) | **DELIBERATE-DELTA** (registered, PARITY-DELTAS #5) |

Tally (9 rows): MATCH 4, MATCH-with-flagged-gaps 1 (#5), MISMATCH 3 (2d/3/8), DELTA 1... plus spec-MATCH #1/#7 counted MATCH. Clean MATCH or registered DELTA: 6/9 ≈ **67%**; strict MATCH 4/9 ≈ 44%.

## 3. exFAT parameter table (checklist 10–14)

| # | Parameter | Ours (file:line) | OG / corpus (cite) | Verdict |
|---|---|---|---|---|
| 10a | Boot: OEM "EXFAT   " + 0xAA55; shifts 9..12 matching device bps; spc-shift sum <26 | `cr_exfat_dentry.cpp:73-83` | `FUN_180021e80` @28180-28199 — byte-exact incl. `param_2 == 1<<shift` device-geometry tie | **MATCH** |
| 10b | MustBeZero +0x0B..+0x3F; fat_count ∈[1,2]; fat_length ≠0; heap-fit bound | **absent** | `FUN_180021e80` @28183-28196 (53-byte zero sweep), @28200 (fat_count-1<2), @28200 (`+0x54 != 0`), @28206-28208 (`(count-1)<<spc + heap ≤ volume_length`) | **MISMATCH** → EXFAT-4 |
| 10c | Boot-region checksum: 32-bit rotate-add over sectors 0..10, skip 106/107/112 of sector 0; value fill-repeated at 11/23 | `cr_exfat_dentry.cpp:26-37, 121-132` (spec 3.1.14.1; shared with RawFixer path `cr_rawfixer.cpp:251-347`) | SearchDLL does **not** verify it — `ExFATFixer::ChecksumDBR` in RawFixer (rawfixer-fs-repair.md); no rotate-add loop found in exFATSearchDLL | **MATCH (spec)**; og-engine-side UNVERIFIED (ours = scanner+repair union, stricter) |
| 10d | Backup boot (sector 12) | `cr_exfat_dentry.cpp:113-115` (OEM+sig only, recorded `boot_backup_ok`, not gating) | og `FixFromDBR` is RawFixer-side; SearchDLL evidence none | **MATCH (lenient=og)** / UNVERIFIED |
| 11a | Dentry types & InUse 0x80; deleted keeps type with 0x80 cleared | `cr_exfat_wire.h:51-61, 137-142` | `(t-X)&0x7F` tests @1846-1864; v2 §3 table | **MATCH** |
| 11b | Volume-label triple 0x03/0x83 → +0x20 must be 0x81 → +0x40 must be 0x82 | skipped as non-result (`cr_exfat_quick.cpp:267-270`) | `FUN_180003090` @1847 (`[0x20]==0x81 && [0x40]==0x82`) — og uses it as root-anchor validation | **MISMATCH (minor)** — we skip without anchoring; effect nil for results (UNVERIFIED og use) |
| 11c | Stream gates: reserved +2/+6/+7/+0x10..0x13 | strict AND, live-only (`cr_exfat_quick.cpp:287-290`) | TWO og forms: permissive OR @1854-1856 (quick grammar, live+deleted) **and** strict AND @24402-24403 (deep pass, live+deleted) | **MISMATCH (form/posse)** → EXFAT-5 |
| 11d | Stream first-cluster validity gate | **absent** (only `first_cluster >= 2` at `:331`) | `FUN_180003090` @1857-1859 + inlined @24404-24412: valid iff `fc != 1 && fc < 0xFFFFFFF7` (i.e. 0 or 2..0xFFFFFFF6); PLUS deep-pass VDL/DL ≤ cluster-space and NoFatChain `fc + ceil(DL/clsize) < 0xFFFFFFF6` fit checks | **MISMATCH** → EXFAT-1 (F-A1-7 not fixed; artifact polarity was inverted) |
| 11e | Entry-set checksum: u16 rotate-add, exclude SetCheckSum u16@file+2 | `cr_exfat_dentry.cpp:47-59` (spec-exact, test-verified `SetChecksumMismatchMarksDamaged`) | no set-checksum validation found in exFATSearchDLL (field gates instead) | **MATCH (spec)**; og UNVERIFIED → EXFAT-7 |
| 11f | File-dentry fields: secondary_count ∈[2,18], timestamp ranges, reserved zero | not validated (name/times consumed raw) | `FUN_180002f90` @1787-1798 (count, create/modify/access ranges: month 1-12, day≠0, hr≤23, min≤59, sec2<30, reserved1/2) | **MISMATCH (tolerance)** → EXFAT-4b |
| 11g | Filename fragments: flags low-2 zero, first char ≠0, ≤15, UTF-16 table screen | not validated (consumed raw) | `FUN_180003030` @1808-1832 (+ inlined @24417-24428) | **MISMATCH (tolerance)** → EXFAT-4b |
| 12a | Cluster heap: cluster n at heap + (n-2)·cluster_bytes; root ≥2 | `cr_exfat_quick.cpp:85-87`; `cr_exfat_dentry.cpp:102` | implied by `FUN_180021e80` gate 15 + first-cluster gates `fc-2 < 0xFFFFFFF4` (bitmap/upcase cases @24431-24436) | **MATCH** |
| 12b | FAT chain vs NoFatChain contiguous | `:327-345` (NoFatChain → contiguous; dirs capped to 1 cluster) | og NoFatChain fit gate @24408-24412; contiguous read per data_length | **MATCH*** (dir cap → EXFAT-6) |
| 12c | exFAT FAT entry semantics: EOC 0xFFFFFFFF, BAD 0xFFFFFFF7, FREE 0 | `cr_exfat_quick.cpp:15-35` (only exact 0xFFFFFFFF = EOC; FREE → broken) | `FUN_180021c60` @28158-28163 [CODE]: entry==0 → **next = cluster+1 with "assumed" flag** (contiguous continuation); EOC-family check in caller UNVERIFIED | **MISMATCH** → EXFAT-2, EXFAT-3 |
| 13 | Deleted grading via bitmap | `:382-400` (on-disk bitmap: 100% set→Overwritten, >0→PartialRisk, else Excellent) | `CQuickSearchAndBuildBitmap` (deep-dive §2): builds occupancy **in memory while walking** | **MATCH (concept)** / specifics UNVERIFIED → EXFAT-10 |
| 14 | First-cluster gate (F-A1-7) + stream VDL gates | not implemented | see 11d | **MISSING** (= EXFAT-1) |

Tally (14 rows): MATCH 5 (10a, 10c-spec, 11a, 12a, 13), MATCH* 1 (12b), MISMATCH 7, MISSING 1 → clean ≈ **43%**; counting registered/unverifiable og-side items ≈ 57%.

**Overall: ~55% of checklist rows are clean MATCH or registered DELTA — the prior
"otherwise near-MATCH" claim is overturned for mirrors (FAT-2), cancel (FAT-3) and the
exFAT gates (EXFAT-1..4).**

## 4. Findings

Severity: P1 = correctness/user-visible on common volumes; P2 = parity gap on damaged
volumes; P3 = tolerance/doc/hygiene. Each carries an acceptance test for fix time.

### FAT

- **FAT-1 (P1) — Deleted-LFN fragment order is reversed.** Deleted LFN slots have their
  sequence byte overwritten with 0xE5; `lfn_assemble` sorts by `sequence & 0x3F`
  (`cr_lfn.cpp:27-29`) → all deleted slots share key 0x25 → disk order preserved →
  fragments are stored on disk in *descending* sequence → concatenated name is
  fragment-reversed (garbage), while the per-slot checksum still matches (it is computed
  vs the 8.3 alias), so the wrong name wins over the 8.3 fallback. Live-LFN files are
  unaffected. **Evidence:** ours `cr_lfn.cpp:21-51`, `cr_fat_quick.cpp:161-171`; og
  slot-0x40-flag handling (deep-dive §4; decomp 44287 area). **Fix:** when slots are
  deleted (seq ≥ 0xE0 or 0xE5), reverse disk order before concatenation (the 0x40
  last-fragment bit is also destroyed). **Acceptance:** fixture with a deleted LFN file
  (3 slots, 0xE5 first bytes + matching checksums) → recovered name equals original.
- **FAT-2 (P2) — CDoubleFATTable semantics mis-implemented + mis-documented.** Og
  (`FUN_180041de0`, decoded this session — supersedes the corpus [INFERRED]): primary
  FAT wins whenever it returns non-zero **including BAD**; mirror consulted only when
  primary returns 0; mirror result then cross-validated (backward-pointer screens,
  `mirror(c+1)==m+1` → reject, forward-contiguity accept, EOC-family exclusions).
  Ours (`cr_chain.cpp:103-135`): mirror consulted when primary is 0 **or** BAD, no
  cross-validation; header comment claims "choose-healthy ... CDoubleFATTable behavior"
  (`cr_fat_engine.h:60-63`) — wrong attribution. Effect: we salvage chains og doesn't
  (FAT1 BAD + FAT2 valid) — defensible hardening but must be a *registered delta*, and
  the comment corrected. Divergence counter (`:93-97`) has no og counterpart (also feeds
  a dead accessor, §6). **Acceptance:** unit test: FAT1[10]=BAD, FAT2[10]=11 → document
  chosen behavior; comment cites FUN_180041de0.
- **FAT-3 (P2) — Cancel granularity: per-directory, not per-dentry/per-run.** Og polls
  the cancel vtable+0x88 at every chain-run (decomp 11235-11238) and every dentry
  (28499-28505). Ours: `walk_dir_entries` polls once on entry (`cr_fat_quick.cpp:119`),
  `scan_fat` after the root (`:332,340`); `follow_chain`/`read_dir_chain` never poll —
  a 4 MiB directory (131k dentries) or a 0x10000-cluster chain is unresponsive once
  entered. F-A1-11 is only partially landed. **Acceptance:** cancelled-token test on a
  large fixture returns ERR_CANCELLED within N dentries (N ≤ 128).
- **FAT-4 (P2) — FAT12 BAD (0xFF7) divergence.** Og `FUN_1800416f0:53801` returns raw
  0xFF7 (chain walker then follows cluster 4087 — garbage-tolerant); ours canonicalizes
  to CHAIN_BAD → `broken`. Ours is spec-correct and safer; register as delta (with
  F-A1-1's EOC half remaining a verified fix). **Acceptance:** FAT12 fixture with a BAD
  mid-chain entry → chain ends at BAD, `chains_broken` incremented (already true).
- **FAT-5 (P3) — Reserved-entry handling differs.** Ours: FAT32 reserved 0x0FFFFFF0-6 →
  EOC (`cr_chain.cpp:115-118`); FAT12/16 0xFF0-6 → raw → out-of-range → `broken`.
  Og: follows raw until the bounds check drops it (dir content so far kept, no
  broken-mark). Salvage-vs-flag nuance only. **Acceptance:** documented delta row.
- **FAT-6 (P3) — '.' identity validation missing; `is_dot_entry` still dead+wrong.**
  Og validates the dot-entry signature (`0x202020202020202E`+u16@+8 spaces+`+0xA ' '`+
  attr 0x10, decomp 11283-11285; '..' @11295-11297) and **rejects the directory** on
  mismatch (11292-11305). Ours only filters dot entries from results with a looser
  inline test (`cr_fat_quick.cpp:145-149`, no ext-byte check); `is_dot_entry`
  (`cr_fat_wire.h:145-149`) has zero callers and checks the wrong byte set
  (F-A1-13 open). **Acceptance:** delete `is_dot_entry` or fix to og signature; decide
  identity-rejection policy (register if kept lenient).
- **FAT-7 (P3) — Dentry plausibility gates absent.** Og `FUN_1800365a0` (44263-44374)
  screens: valid dates (month 1-12, day ≠0), LFN slot first_cluster==0 (`:44287`),
  directory file_size==0 (`:44346`), FAT12/16 first_cluster_high==0 (`:44350`),
  `~n` alias sanity (name[6]=='~', name[7] digit 1-5, `:44335-44339`). Ours emits
  everything (more salvage, more junk). **Acceptance:** delta-registry decision; if
  implemented, count rejections in stats.
- **FAT-8 (P3) — Backup-boot fallback: all-flavor scope + hardcoded sector 6.**
  `cr_fat_quick.cpp:285-301` reads sector 6 as a boot candidate even on FAT12/16 (no
  backup exists there — it reads FAT-area bytes; harmless because full validation
  follows, but spec-unclean) and ignores the parsed `p.backup_boot_sector` value.
  Og: in-engine fallback not evidenced (RawFixer `CheckBackupDBR` is pre-scan).
  **Acceptance:** restrict fallback to FAT32 or to `backup_boot_sector`-derived offset;
  keep 4Kn-aware stride (registered delta D-family #3).
- **FAT-9 (P3) — Creation tenths dropped.** `fat_date_to_unix` ignores the +0xD
  10-ms field (`cr_lfn.cpp:81`). Og conversion not located in SearchDLL (props-layer,
  UNVERIFIED). Spec-verdict: minor precision loss on created-time only.
  **Acceptance:** optional: fold tenths into the 100-ns result.
- **FAT-10 (P3) — LFN slot grammar unchecked.** Og requires slot first_cluster
  (+0x1A)==0 before treating an 0x0F entry as an LFN slot; ours keys on attr alone
  (`cr_fat_quick.cpp:135-139`) — junk slots with stray +0x1A bytes are absorbed.
  **Acceptance:** add `l->first_cluster == 0` to slot acceptance.

### exFAT

- **EXFAT-1 (P2) — Stream first-cluster validity gate missing (F-A1-7 NOT fixed; the
  prior artifact mis-transcribed the polarity).** Og rejects `first_cluster == 1` and
  `first_cluster >= 0xFFFFFFF7` (FUN_180003090:1857-1859; inlined 24404-24412;
  backward-walk form 2109). The v2 artifact's "must be 1 or ≥ 0xFFFFFF7" is inverted —
  the decompile is unambiguous across three call sites. Ours accepts both (`:331` gate
  is only `>= 2`, so 1 is excluded but EOC-family values pass and get "resolved").
  **Acceptance:** reject (count `sets_damaged`, keep entry) when live-or-deleted stream
  has `fc == 1 || fc >= 0xFFFFFFF7`; regression test both values.
- **EXFAT-2 (P2) — exFAT FAT-entry==0 should continue contiguously, not break.**
  Og `FUN_180021c60:28160-28163`: FAT entry 0 (unallocated/no chain info) →
  `next = cluster+1` with an "assumed" out-flag — the exFAT salvage heuristic for
  FAT-zeroed volumes. Ours (`cr_exfat_quick.cpp:62-65`) marks the chain `broken`.
  Effect: truncated extents + downgraded confidence on damaged-but-recoverable
  volumes. **Acceptance:** entry==0 mid-chain → continue with cluster+1 (flagged, e.g.
  a new `assumed_contiguous` stat), stop at cluster-count bound.
- **EXFAT-3 (P2) — EOC family too narrow.** Only exact 0xFFFFFFFF terminates
  (`cr_exfat_quick.cpp:31-32`); spec EOC is 0xFFFFFFF8-0xFFFFFFFF. 0xFFFFFFF8..E values
  are followed as cluster numbers → bounds fail → broken chains on spec-legal volumes.
  (Og caller check UNVERIFIED.) **Acceptance:** `v >= 0xFFFFFFF8` → EOC; test with
  0xFFFFFFFA terminator.
- **EXFAT-4 (P2) — Boot validation gaps vs `FUN_180021e80`.** Missing: (a) MustBeZero
  sweep +0x0B..+0x3F (28183-28196); (b) fat_count ∈ [1,2] (28200); (c) og rejects
  fat_length==0 — ours accepts a "no-FAT" volume (`cr_exfat_dentry.cpp` has no gate;
  `cr_exfat_quick.cpp:468` merely skips FAT load); (d) heap-fit bound
  `(cluster_count-1)<<spc_shift + heap_offset ≤ volume_length` (28206-28208).
  Sub-finding **EXFAT-4b (P3)**: og also validates file-dentry fields
  (`FUN_180002f90`: secondary_count 2-18, timestamp ranges, reserved1/2 zero) and
  filename-fragment grammar (`FUN_180003030`); ours consumes raw (tolerance choice).
  **Acceptance:** add (a)-(d) as `ERR_BAD_PARAMETER`/`boot_*` flags; 4b either
  implemented or registered as deliberate tolerance.
- **EXFAT-5 (P3) — Reserved-field gate form/position.** Ours: strict AND, live-only
  (`cr_exfat_quick.cpp:287-290`). Og: permissive OR in the quick grammar
  (FUN_180003090:1854-56, live+deleted) and strict AND in the deep pass
  (24402-24403, live+deleted). F-A1-8 picked one of two og forms. **Acceptance:**
  align to OR for the walk (parity) or register the strict-AND choice; extend to
  deleted entries in either case.
- **EXFAT-6 (P3) — NoFatChain directories capped at 1 cluster.**
  `contiguous_chain(first, 1)` (`cr_exfat_quick.cpp:336-338`) ignores data_length;
  multi-cluster NoFatChain dirs (spec-legal) truncate. Og bounds-checks by
  data_length>>shift (24406-24412). **Acceptance:** read
  `ceil(data_length/cluster_bytes)` clusters for NoFatChain dirs.
- **EXFAT-7 (P3) — Entry-set checksum is our hardening, not og parity.** No set-checksum
  validation found in exFATSearchDLL (field gates instead). Keep (spec-exact, test
  `SetChecksumMismatchMarksDamaged`), register as deliberate delta; `sets_damaged` has
  no og counterpart metric.
- **EXFAT-8 (P3) — name_hash unused.** Stream +0x04 NameHash never validated/used;
  og uses FNV/name-hash keying internally (deep-dive §4). No dup-detection parity.
  **Acceptance:** optional validation of name-hash vs assembled name (spec checksum16
  of upcased name).
- **EXFAT-9 (P3) — Dual-FAT exFAT ignored.** `fat_count==2` second FAT never read;
  VolumeFlags bit0 (ActiveFat) ignored (`volume_flags` parsed, unread — §6). Og boot
  gate requires fat_count ∈[1,2]; selection semantics UNVERIFIED. **Acceptance:**
  register or implement ActiveFat selection.
- **EXFAT-10 (P3) — Deleted-grading source.** Ours grades from the on-disk bitmap only
  when a live 0x81 dentry survives; og builds in-memory occupancy during the walk
  (CQuickSearchAndBuildBitmap). Concept registered MATCH; exact og thresholds
  UNVERIFIED. **Acceptance:** fallback occupancy map from live-file chains when the
  bitmap dentry is gone.

## 5. Prior-claim fix-state register (verification results)

| Claim (PARITY-AUDIT F-A1) | Stated | Verified state |
|---|---|---|
| F-A1-1 FAT12 EOC canonicalization | fixed | **FIXED** — `cr_chain.cpp:44-47` + regression `Fat12ChainsCanonicalEocAndBad` (test_fat_volume.cpp:487-528). Residual: FAT12 BAD divergence (FAT-4). |
| F-A1-7 exFAT first-cluster gate | to fix | **NOT FIXED** — no gate; and og polarity was inverted in the artifact (EXFAT-1). |
| F-A1-8 reserved-field nuance | to fix | **PARTIAL** — strict-AND live-only implemented; og has OR(quick)+AND(deep), live+deleted (EXFAT-5). |
| F-A1-11 FAT cancel polling | to fix | **PARTIAL** — per-directory checkpoint only; og per-dentry + per-run (FAT-3). |
| F-A1-12 deleted-name '_' | delta | **REGISTERED** — PARITY-DELTAS #5; no change needed (confirmed). |
| F-A1-13 is_dot_entry dead | cleanup | **STILL OPEN** — dead + wrong signature (FAT-6, §6). |
| "FAT engine otherwise near-MATCH" | — | **OVERTURNED** — FAT-2 (mirror semantics), FAT-3 (cancel) newly decoded from FUN_180041de0/11235/28499. |
| "exFAT clean (P2s)" | — | **OVERSTATED** — EXFAT-1..4 are P2 gaps with decompiled evidence. |

## 6. Dead / duplicate code register (Grep-verified zero-call or duplication)

| # | Symbol | Location | Status |
|---|---|---|---|
| 1 | `is_dot_entry` | cr_fat_wire.h:145-149 | DEAD + wrong og signature (F-A1-13) |
| 2 | `chain_is_bad` / `chain_is_eoc` / `chain_is_reserved` | cr_fat_wire.h:130-138 | DEAD helpers |
| 3 | `CHAIN_FREE` | cr_fat_wire.h:128 | DEAD constant |
| 4 | `ATTR_READ_ONLY/HIDDEN/SYSTEM/ARCHIVE`, `ATTR_MASK_LONGNAME` | cr_fat_wire.h:82-89 | DEAD constants (only VOLUME_ID/DIRECTORY/LFN/DELETED used) |
| 5 | `BpbCommon` / `BootSectorFat12_16` / `BootSectorFat32` | cr_fat_wire.h:25-76 | DEAD structures — validate_boot reads raw offsets; static_asserts only |
| 6 | `FatTable::set_raw` | cr_chain.cpp:137-174 | DEAD (no callers in src/tests/fuzz) |
| 7 | `FatTable::divergences()` / `mirror_loaded()` | cr_fat_engine.h:74-77 | DEAD accessors; the divergence *computation* (cr_chain.cpp:93-97) is dead work per scan |
| 8 | `FatParams::first_usable_cluster` | cr_fat_engine.h:48, cr_chain.cpp:306 | set-never-read |
| 9 | `FatParams::backup_boot_sector` | cr_fat_engine.h:47, cr_chain.cpp:281 | parsed-never-used (fallback hardcodes 6; FAT-8) |
| 10 | `FatScanFlags::keep_deleted` | cr_fat_engine.h:132 | never read by FAT engine (hfsplus/ext4 read theirs — API inconsistency) |
| 11 | `DENTRY_END` | cr_fat_wire.h:81 | referenced only in a comment (code uses literal 0x00) |
| 12 | `TYPE_VOLUME_GUID` | cr_exfat_wire.h:59 | DEAD constant |
| 13 | `EXFAT_EOC_VALUE` / `EXFAT_BAD_VALUE` | cr_exfat_wire.h:155-156 | **DUPLICATE** of local `EXFAT_EOC`/`EXFAT_BAD` (cr_exfat_quick.cpp:15-16); wire versions unused |
| 14 | `CHECKSUM_OFF` | cr_exfat_wire.h:160 | DEAD (fill-repeat check reads 11·bps directly) |
| 15 | `EXFAT_SIGNATURE` | cr_exfat_wire.h:21 | DEAD (inline 0x55/0xAA used) |
| 16 | `UpcaseEntry` / `VolumeLabelEntry` | cr_exfat_wire.h:116-133 | DEAD structures (entries skipped by type only) |
| 17 | `ExfatScanFlags::keep_deleted` | cr_exfat_engine.h:71 | never read |
| 18 | `ExfatParams::volume_flags` | cr_exfat_engine.h:46 | set-never-read (ties to EXFAT-9) |
| 19 | `checksum16()` public helper | cr_exfat_dentry.cpp:39-45 | used only by fuzz; `entry_set_checksum` re-implements the rotate inline — duplication |
| 20 | civil-days date math | cr_lfn.cpp:85-93 vs cr_exfat_quick.cpp:114-119 | duplicated algorithm body (extract shared helper at fix time) |

Not dead (checked): `boot_region_checksum` (shared with `cr_rawfixer.cpp:251-347`),
`EmitCtx::root_lost` (used at cr_fat_quick.cpp:109), `boot_*_ok` flags (test-consumed),
`lfn_recovered`/`no_fat_chains` stats (test-asserted).

## 7. UNVERIFIED register (no RE evidence — tracked, not guessed)

1. FAT epoch conversion + tenths handling in og (no 1980/epoch math in FATSearchDLL;
   likely props/UI layer).
2. og FAT16 `Next` bounds field +0x1C semantics (cluster count vs max-cluster) — ±1
   boundary at top cluster (FUN_180041c80:54131).
3. LFN checksum loop byte-exactness in og (artifact asserts standard; loop not located).
4. FAT backup-boot usage inside SearchDLL (evidence points to RawFixer pre-scan; our
   in-engine fallback is a registered superset).
5. Deleted-chain confidence grading thresholds (ours by design; og "truncation
   tolerated" is concept-level, fat-exfat v1 §5).
6. exFAT entry-set checksum validation anywhere in og SearchDLL (field gates found
   instead — EXFAT-7).
7. exFAT EOC-family check in og chain-follow caller (EXFAT-3 fix should follow spec).
8. exFAT deleted-grading thresholds in og (EXFAT-10).
9. exFAT ActiveFat / second-FAT selection in og (EXFAT-9).
10. Volume-label-triple anchoring semantics beyond validation (11b).
11. MOM modules FatLib/FatFormat/FAT(Fat)FileSystemAnalyser artifacts are census-only
    (no algorithms) — no parity extraction possible from them.

## 8. Recommended fix order (for the fix-wave owner)

1. **FAT-1** (P1, small, user-visible on every deleted-LFN volume) + test.
2. **EXFAT-1** (P2, polarity-corrected gate) + **EXFAT-3** (EOC family) — one test file.
3. **FAT-3** (cancel per-dentry) — contract §2 compliance.
4. **EXFAT-2** (free-entry contiguous continuation) — biggest salvage win on damaged
   exFAT.
5. **FAT-2** comment/delta registration (or behavior alignment) + **EXFAT-4** boot gates.
6. Dead-code sweep §6 (items 1-20) + `keep_deleted` API harmonization.
7. P3 tolerance decisions (FAT-5/6/7/8/9/10, EXFAT-4b/5/6/8/9/10) → implement or
   register in docs/PARITY-DELTAS.md.

— Refactor-D, 2026-10-24. Read-only audit; no source files were modified.
