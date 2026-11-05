# S55-D1 — local-database family deep-dive (lostpart.dat / partinfo.dat / EUUserData)

Task S55-D1, read-only. Core HEAD `1fccb0a4`, corpus pin `be0cfe9`.
Paths cited below (abbreviated):
- **PRD** = `easeus-drw-research/re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c`
- **DRWUI** = `easeus-drw-research/re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`
- **samples** = `easeus-drw-research/og-codebase/PartitionRecovery/{lostpart.dat(1096 B), partinfo.dat(24 B)}`
- Core = `core_recovery/src/...`, `core_recovery/include/...`
- Priors: `diff-to-fix/evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §D (the earlier decode — re-verified today, all offsets re-confirmed first-hand); `s25-oq-s6-1-generator-sets-and-dat-files.md` §2.

The M-042/N-049 finding files referenced by FIXED-LEDGER.md do not exist on disk
(`findings/FIXED/` has neither); their landing state is carried by
`diff-to-fix/CROSS-REF-MATRIX.md:682` ("M-042 FIXED (DFX20)") and the code comments.

---

## (a) Record decodes

### partinfo.dat — 0x18 (24) B/record, cross-run partition-TABLE snapshot

Reader `FUN_180023120` (PRD:21241-21284): opens `<DLL-dir>\partinfo.dat`
(path builder `FUN_180025dc0` PRD:23151-23166 = GetModuleFileNameW + truncate at
last `\`), loops ReadFile of 0x18 B, gate `magic=="Part"` (0x74726150, PRD:21277),
hands each record to `FUN_180025af0`→`FUN_180023e80` (PRD:23007-23011 /
21749-21767 — container push). Writer `FUN_180025f90` (PRD:23220-23271).
Record built by the live-table enumerator `FUN_1800264e0` (PRD:23453-23552,
record fill at :23531-23541).

| off | size | field | evidence | shipped sample value |
|---|---|---|---|---|
| +0x00 | u32 | magic `"Part"` | PRD:23531 | `50 61 72 74` |
| +0x04 | u64 | **start LBA** | PRD:23534 (vtable+0x28 getter) | `0x800` = 2048 |
| +0x0C | u64 | **size in sectors** | PRD:23537 (vtable+0x30 getter) | `0x0749cfc0` = 122,277,824 (58.3 GiB) |
| +0x14 | u32 | **MBR type** | PRD:23540 (vtable+0x58 getter) | 7 = NTFS |

Arithmetic cross-check vs the lostpart sample: 2048 + 122,277,824 − 1 =
0x0749d7bf = exactly the lostpart record's end-LBA field (below) — the two
shipped files describe the SAME partition (live NTFS entry + its lost-candidate
twin). Comparator `FUN_1800229c0` (PRD:20998-21039): loads partinfo.dat, re-
enumerates the live table, compares count then `memcmp` of each 0x18 record
(:21026-21034) → the layout_changed flag.

### lostpart.dat — 0x448 (1096) B/record, cross-run lost-partition CANDIDATE cache

Reader `FUN_180023280` (PRD:21289-21332, same magic gate :21325, consumer
`FUN_180025b20`→`FUN_180023ef0` PRD:23017-23022 / 21772-21790). Writer
`FUN_1800260f0` (PRD:23276-23327, append-at-EOF via GetFileSizeEx+
SetFilePointerEx :23312-23317). Two builders fill the record:

- **found-candidate record** — `FUN_180020a60` (PRD:20154-20238), header fill
  :20210-20221, payload copy :20223-20229 (0x430 blob copied from the object the
  found-callback `FUN_1800025a0` filled, PRD:976-994).
- **progress-marker record** (header-only, payload zeroed) — `FUN_180025c60`
  (PRD:23088-23121): `local_468=magic` :23109, `local_464=param_2` :23116,
  `local_460=param_3`, `local_45c=min(param_4,param_5)` :23111-23115,
  `local_454=0` :23110.

| off | size | field | evidence | sample |
|---|---|---|---|---|
| +0x00 | u32 | magic `"Part"` | PRD:20215 | `50 61 72 74` |
| +0x04 | u32 | **phase/progress counter** (candidates: `pool+0xdc`, or `pool+0xe0` when `pool+0x3c==0`, PRD:20216-20219; markers: `pool+0xb4`, PRD:20080) — restored into progress counter `pool+0xe4` at replay PRD:20755-20762 | 1 |
| +0x08 | u32 | **disk number** (`pool+0x40` PRD:20221; s5:184; 3rd dedup key PRD:20831; the device-index scalar used by `FUN_1800262c0` device lookup) | 3 |
| +0x0C | u64 | **scan-position LBA** (markers only: `min(cur,end)` PRD:23111-23115; restored into `pool+0xe8` PRD:20764; candidates leave 0) | 0 |
| +0x14 | u32 | **kind flag** — 1 = candidate (re-apply/revalidate on load), 0 = progress marker (`local_894=1` PRD:20220 vs `local_454=0` PRD:23110; replay branch `*(rec+0x14)!=0` PRD:20753/20819) | 1 |
| +0x18..0x447 | 0x430 | **CSearchedPartitionInfo image** (the same blob handed to the found-callback and the UI listener, PRD:20205-20206/:20231) | see below |

Payload blob (offsets relative to record start; blob base = +0x18), decoded from
the filler `FUN_1800025a0` (PRD:976-994) — FIRST-HAND today, extends s5:185-190:

| record off | size | field | evidence (PRD:983-993 unless noted) | sample |
|---|---|---|---|---|
| +0x18 | u32 | pool+0x80 (scan/session id) | `*param_6 = param_2` | 1 |
| +0x1C | u32 | candidate ordinal (`pool+0x84`, post-increment PRD:20203-20204) | `param_6[1] = param_4` | 0 |
| +0x20 | u32 | **disk number** (same source as header +0x08) | `param_6[2] = param_3` | 3 |
| +0x24 | u32 | **fs type** (partition object+0x18; type map 1=FAT12/2=FAT16/3=FAT32/4=NTFS/8=exFAT/0xB=BitLocker, s5:189-190 citing PRD:9856/:803/:66) | `param_6[3]` | 4 = NTFS |
| +0x28 | u64 | **start LBA** (object+0x00) | `*(u64*)(param_6+4) = *param_1` | 0x800 |
| +0x30 | u64 | **end LBA** = start+size−1 (+1 extended-partition quirk `FUN_180002760` PRD:1035-1045, s5:186-187) | `*(u64*)(param_6+6)` | 0x0749d7bf |
| +0x38 | u32 | **bytes/sector** (object+0x20) | `param_6[8]` | 512 |
| +0x441 | 1 | **CHS-align bool** (`FUN_180002660` gate) | `*(bool*)(param_6+0x429)` PRD:991-992 | 1 |
| +0x444 | u32 | **sectors/cluster** (object+0x1C) | `param_6[0x10b]` PRD:993 | 8 (NTFS) |
| rest | — | wider CSearchedPartitionInfo fields (score double at +0x70 per `evidence/decompiled-quotes/s3-calcpartition-scoring.md:115`; DataAreaStart per N-035/`modules/lost-partition.md:154-156`; zeros in the shipped sample) | not filled by this path | 0 |

The engine-side replay/dedup key is **{+0x28 start, +0x30 end, +0x08 disk}**
(PRD:20825-20831) — matching s5:178 "{start +0x28, end +0x30, type +8}".

## (b) Writer/trigger decode — what causes a save

All writes are **append-at-EOF** (OPEN_ALWAYS + seek-to-end, PRD:23248-23264 /
23304-23320). Triggers, first-hand:

| # | Trigger | Site | Writes |
|---|---|---|---|
| W1 | **First search in the process** (latch `*pool+0xf0==0`): `DeleteFileW` BOTH .dat files, then rebuild partinfo from the live table, set latch | `FUN_180025e30` PRD:23189-23211 (delete :23195-23198, rebuild :23200-23208); called at search start PRD:20745 and at phase-1 completion PRD:20885 (`FUN_180022430`, state==1) | partinfo.dat rebuilt (0x18 × live entries) |
| W2 | **Each FOUND lost partition, mid-scan** (the scan loop's found-handler) | `FUN_180020a60` PRD:20230 (call context: the scan loop `FUN_180021080`, s5:167) | one 0x448 candidate record appended |
| W3 | **Clone/search interrupt checkpoint** (cancelled OR state==4) | call site PRD:20079-20082 → `FUN_180025c60` PRD:23118 | one 0x448 header-only progress-marker record |

So: **the og writers are NOT scan-end writers** — they are per-hit (mid-scan),
interrupt, and first-use/phase-boundary writers. The og database is **NOT
cross-session accumulating**: the first search of each process deletes both
files (W1) and the cache re-grows only within that process lifetime; the
previous process's file is *consumed* (read → replayed → re-validated → rebuilt)
at the next process's first search. One-generation-deep cache, per-boot scratch
(s25 §2 consequence 3 says the same).

Read-side flow at search start (`FUN_180021e90`, PRD:20704-20854) — re-verified:
1. :20742 `FUN_1800229c0` loads OLD partinfo.dat, compares vs live table → layout_changed.
2. :20744 loads OLD lostpart.dat (before W1 deletes it).
3. :20745 W1 first-use reset.
4. :20748-20812 **replay**: records with `+0x14==0` (markers) restore progress
   `pool+0xe4 ← rec+0x04` (max) and `pool+0xe8 ← rec+0x0C` (:20753-20765) —
   FIRST-HAND decode today, not in s5/s25; records with `+0x14!=0` (candidates)
   re-read the disk at `rec+0x28` (vtable read :20778) and re-register via
   `FUN_180021080` (:20787) — stale ones drop (s5:175-179).
5. :20813-20847 if layout UNCHANGED: reload lostpart.dat; any OLD candidate whose
   {start,end,disk} is absent from the reloaded file → changed flag → **progress
   counters reset** (`pool+0xe4/+0xe8 = 0`, :20848-20851).

## (c) Core-vs-og persistence field comparison

Core record = `REC_PARTITION_CONTEXT` (codec `src/core/session/cr_records.cpp:902-935`,
struct `include/cr/session/cr_records.h:150-172`, written per lost candidate
`src/core/orchestrator/cr_scan.cpp:2446-2466`, restored+revalidated
`cr_scan.cpp:2644-2686`). Layout snapshot = `REC_DISK_LAYOUT`
(`cr_records.cpp:939-968`, `cr_records.h:174-190`, captured at run() start
`cr_scan.cpp:979-992`, compared `cr_scan.cpp:993-1009`).

### partinfo.dat (og) vs REC_DISK_LAYOUT (core) — full parity

| og field | core field | verdict |
|---|---|---|
| +0x04 u64 start LBA | `DiskLayoutEntry.start_lba` (cr_records.cpp:946) | MATCH |
| +0x0C u64 size | `DiskLayoutEntry.sector_count` (:947) | MATCH |
| +0x14 u32 MBR type | `DiskLayoutEntry.mbr_type` (:948) | MATCH |
| — | u32 count header (:944; og: count = filesize/24) | equivalent |

M-042 landed this (CROSS-REF-MATRIX.md:682: "REC_DISK_LAYOUT type 8 (partinfo
equivalent — layout_changed surfaced at resume, never enforced)"). The og's
per-record memcmp semantics (PRD:21026-21034) = the core's elementwise compare
(cr_scan.cpp:994-1005). MATCH.

### lostpart.dat 0x448 (og) vs REC_PARTITION_CONTEXT (core)

| og field (off) | core field | verdict |
|---|---|---|
| magic +0x00 | CRSF typed record header | covered (different container) |
| +0x04 phase/progress counter | — (SessionState.stage_done_mask, global) | **ABSENT per-candidate** |
| +0x08 disk number | — (session file is per-target; implicit) | **ABSENT by design** |
| +0x0C scan-position LBA (markers) | — (SessionState.bytes_scanned only) | **ABSENT** |
| +0x14 kind flag (candidate/marker) | `kind=1` (candidates only; cr_scan.cpp:2452) | PARTIAL — core persists no marker records |
| blob start LBA (+0x28) | `start_lba` (cr_scan.cpp:2448) | MATCH |
| blob end LBA (+0x30) | `sector_count` (:2449) | MATCH (equivalent info) |
| blob fs type (+0x24) | `fs_kind` (:2450) | MATCH |
| blob bps (+0x38) | `bytes_per_sector` (:2454, M-042 v2) | MATCH |
| blob SPC (+0x444) | `sectors_per_cluster` (:2455) | MATCH |
| (CSearchedPartitionInfo hidden field; zeros in sample, source offset unpinned) | `hidden_sectors` (:2456; cr_records.h:152-155) | MATCH (core cites +0x38/+0x444) |
| blob score (double, +0x70 per s3 evidence) | `score` u32 (:2451) | covered (narrower type) |
| blob CHS-align bool (+0x441) | — | **ABSENT** |
| blob DataAreaStart (N-035/ln) | — | **ABSENT** |
| blob candidate ordinal / session id | — | ABSENT (trivial) |
| og evidence nuance | core `flags` bit0 `size_from_device_end` bit1 `have_geometry` (:2457-2461) | covered for those two |

**Core's own in-memory model is wider than its persistence** (`include/cr/lost/
cr_lostpart.h:56-105`): `data_start_lba`/`data_sector_count` (the ln/s22 FIXED
data-area extents), `structural_points`, `content_count`, `table_match`,
`evidence[]` are NOT in REC_PARTITION_CONTEXT and NOT restored on resume
(`cr_scan.cpp:2654-2667` fills neither). The og, persisting the whole 0x430
struct image, round-trips whatever the struct carried.

**Is the og database cross-session accumulating?** No (see (b)): delete-on-
first-use bounds it to one process generation. The core's session files are
durable per-session by design; core save triggers = save_session (end-of-scan),
stage-boundary checkpoints (cr_scan.cpp:1298/1306/1319), and the **forced
per-hit checkpoint when discovery produces candidates** (cr_scan.cpp:2028-2040,
comment: "the og's per-hit persistence (PRD:20409 …) — still gated on a
configured sink (the core has no module-local .dat side-channel … the honest
architecture difference registered with the finding)"). Core is a checkpoint
SUPERSET at stage granularity (lane-D-statemgmt.md:129, G-13) but has no
intra-stage scan-position persistence.

## (d) EUUserData tolerance verification (ITEM 3)

Artifact read: `re-workspace/artifacts/config-files/userdata-remoteconfig--
signature-and-flags.md` — documents the SHAPE (111 entries
`{ext: {signs: ["offset,HEX"...], mime}}`, runtime-extensible, same grammar as
raw_signs.json) but documents **no failure semantics** — the tolerance decode is
decompile-only (below). File location: `<exe-dir>/EUUserData` (DRWUI
`sprintf "%s/EUUserData"` DRWUI:1005387; loader `FUN_140606ab0` :1005324-1005492,
lazy-once on a `param_1[2]==0` buffer, `_wfopen_s` read; no EUUserData string
exists in RAWSearchDLL or any other reversed binary — grep over
`re-workspace/reversed` hits DRWUI.exe only → **the file is a UI-layer load;
the engine never re-reads it**).

Og consumption semantics (first-hand, `FUN_140606e80` :1005497-1005682 →
`FUN_1406072a0` :1005687-1005931 → `FUN_140607930` :1005936-1006119):

| condition | og behavior | cite |
|---|---|---|
| whole file fails JSON parse | silent: nothing loads, no abort | :1005557-1005558 (`cVar2 != 0` gate) |
| entry value not a JSON object | **skip entry** | :1005566 (`'\a'` type gate) |
| "signs" member missing | entry contributes nothing; loop continues | :1005818 (`_stricmp "signs"` is the only dispatch) |
| "signs" not an array (empty string / number / null) | **skip entry**, no error | :1005825 (`'\x06'` array-type gate) |
| **"signs" == `[]` (empty array)** | zero iterations → nothing registered for the entry, **no error, file continues** (count 0 → `goto LAB_1406075a3` cleanup :1005876/:1005914-1005925, member loop advances :1005928) | :1005841 (`0 < iVar5`), :1005876 |
| non-string array element | **skip that element** | :1005844 (`'\x04'` string-type gate) |
| malformed sign string (comma-split ≠ exactly 2 tokens) | **skip that sign**, entry continues | :1005986 (`!= 2 → LAB_140607dbb`) |
| odd-length hex / garbage offset | loads; atoi→0; odd hex never matches (dead entry) | M-129 citing :1003452; atoi :1005991 |
| unknown members (incl. "mime") | ignored | :1005818 |

**Verification answer: og = SKIP ENTRY (and skip-sign granularity); never
skip-file (except an unparseable file, silently), never abort.** Core
`src/core/carve/cr_usersigns.cpp`: `"signs": []` → `sign_count == 0` →
`return ERR_BAD_PARAMETER` for the **WHOLE file** (:201-202, "an entry with no
signs is dead config"); same whole-file rejection for one malformed sign
(:194-195), unknown key (:206-207), missing signs (:210), trailing garbage
(:220-221). Default load → zero user signs; explicit `--user-signs` → CLI abort
(M-129 core refs, `src/app/rrecov.cpp:1285-1289`). One hand-edited entry with
empty signs therefore destroys all 111 default entries' worth of custom
signatures in the core, while the og loses only that entry.

**Covered by M-129** (`findings/MAJOR/M-129-euuserdata-parser-all-or-nothing-vs-
og-tolerant-skip.md`, READ-ONLY/unfixed): its corpus refs cite :1005818,
:1005986, :1003452 — the empty-array/type-gate/skip-entry path (:1005825,
:1005841, :1005876, :1005566) and the whole-file-parse silent skip (:1005557)
are ADDITIONAL citations this audit contributes; the verdict is unchanged and
CONFIRMED. N-169 (duplicate-asf merge) is the sibling quirk finding.

## (e) DIVERGENCE CANDIDATES

1. **EUUserData strictness — empty/invalid "signs" entry rejects the whole file
   in core; og skips the entry.** Og: DRWUI:1005566/1005825/1005841/1005986
   (skip-entry/skip-sign, no abort). Core: cr_usersigns.cpp:194-195/201-202/
   206-207/210/220-221 (whole-file ERR_BAD_PARAMETER); rrecov.cpp:1285-1289
   (CLI abort). Proposed severity: MAJOR (as filed). **COVERED by M-129**
   (READ-ONLY, unfixed) — this audit CONFIRMS and adds the missing og-side
   citations. No new finding.

2. **ln-fixed data-area extents are not persisted — resumed candidates degrade
   to whole-volume extents.** Og: the 0x448 record is a full CSearchedPartitionInfo
   image (PRD:20223-20229), so DataAreaStart-class fields round-trip; core:
   REC_PARTITION_CONTEXT has no data-area fields (cr_records.cpp:902-916) and the
   restore path fills neither (cr_scan.cpp:2654-2667), while the fresh-run
   containment/dedup consumes `data_start_lba` (cr_candidates.cpp:899-900,
   943-944) per the ln/s22 fix (CROSS-REF-MATRIX "ln (S2) FIXED s22").
   Impact honest-scoped: standard resume consumes candidates without re-running
   discovery, so the loss bites only re-classification/re-save flows of resumed
   candidates. Proposed severity: **NEW, MINOR** (not covered: ln fixed the
   fresh-run path only; M-042 added geometry, not extents).

3. **Og persists intra-driver progress (phase counter + scan-position LBA) via
   header-only lostpart records and restores it across process restarts
   (PRD:20753-20765 ← PRD:20079-20082/23111-23115); core has stage-granular
   resume only** (adopt_resume_state_ cr_scan.cpp:386-393 skips finished stages;
   no intra-stage position anywhere). The restore path is decoded here
   first-hand; s5/s25 do not carry it. Adjacent registrations exist:
   lane-D-statemgmt.md:109-112 ("no scan-position continue and no stage
   tracking in the format" — for the og's .rsf) and the G-13 SUPERSET row
   (:129). Proposed severity: **NEW, INFORMATIONAL register** (the og's restore
   covers only the partition-search driver's own counters; the core's
   checkpoint cadence is a deliberate superset at stage granularity).

4. **CHS-align bool (og record +0x441, PRD:991-992) has no core persisted
   equivalent** (PartitionContext has no such flag; cr_records.h:160-171).
   Impact: none observed in core consumers; og consumer of the flag beyond the
   write is undecoded. Proposed severity: **NEW, INFORMATIONAL** (or fold into
   the N-035/ln geometry family as a one-line register note).

5. **Disk number / candidate ordinal / scan-session id not persisted per
   candidate in core.** Og: PRD:20221/20216-20219 (header), filler PRD:983-985.
   Core: session-per-target makes the disk implicit — the architecture
   difference is REGISTERED with M-042's landing (cr_scan.cpp:2034-2038 comment;
   s25 consequence 3). **COVERED (architecture note) — no action.**

6. **Persistence-trigger parity: og per-hit append is unconditional
   (file-system side effect always available); core per-hit checkpoint is
   sink-gated** (no `--out`/checkpoint sink → nowhere to write). Og: PRD:20230;
   core: cr_scan.cpp:2028-2040 + emit_checkpoint_ :705-743 (returns early when
   `!cfg_.checkpoint`). **COVERED — the honest architecture difference is
   registered with M-042 (same comment); note only.**

7. **Reset semantics: og deletes both .dat files on the first search of each
   process** (PRD:23195-23198); core session files are durable and never
   auto-deleted. **COVERED by s25 §2/s5 §D (per-boot scratch, not durable
   knowledge) — intentional core superset, no action.**

Non-divergences verified today: partinfo↔REC_DISK_LAYOUT full field parity;
lostpart start/end/fs/bps/SPC/hidden↔PartitionContext v2 parity; revalidate-on-
resume + layout-changed-surfaced-never-enforced parity (cr_scan.cpp:961-1009 vs
PRD:20742-20851) — all consistent with the M-042 FIXED ledger row
(CROSS-REF-MATRIX.md:682).
