# modules/ext4.md — ext4 engine audit verdicts

## S12 addendum (2026-10-29 — the session-2 deep-dive)

New active findings on this module: ~~**N-177**~~ **FIXED (S46)** (the size-exceeds-blocks damaged
flag landed — NODE_FLAG 0x0200 = the og's own bit, the clean 64-bit compute, D-128 registered (renumbered from the collided D-135 at N-207),
SizeExceedsBlocksSetsDamagedFlag + CleanFilesCarryNoBit + SessionRoundTrip); **I-040 (ACKNOWLEDGED — PARITY-DELTAS D-123 + the two anti-down-fix pins landed)** registers the og's i_size_high legacy-ACL
pollution non-adoption). Verified-MATCH: block-group math, groups math, blocks_hi.


Reference: EXT4SearchDLL (covers ext2/3/4 — DRWUI codes 4/5/9) per corpus artifacts
`SearchDLL/ext4-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md` §1–2,
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md`, `CORRECTIONS.md` rows 4/6.

Core (audited commit `67193b6`): `src/core/ext4/cr_ext4_engine.cpp` (851 LOC) +
`include/cr/ext4/` + `tests/unit/test_ext4_volume.cpp`.

## Behavioral

- **B1 detection** — `M` SB magic 0xEF53 @+0x38, field-set validation
  (inodes/blocks/bpg/ipg nonzero, first_data_block↔log_block_size consistency),
  block size `0x400 << (log & 0x1F)`, 64-bit blocks_hi concat, groups math + sanity.
  **Guard-field conflict settled in core's favor**: the `& 1` guard byte is at SB+0x60
  (s_feature_incompat bit 0 = COMPRESSION), not the artifact's "+0x18 log_block_size"
  reading — agent B's mechanical decode (:3223/:3304) and the core
  (`cr_ext4_engine.cpp:103`, `SB_INCOMPAT=0x60`) agree; the artifact is wrong (corpus
  erratum candidate #7).
- **B2 recovery correctness** — `M` extents + indirect-block ladder (ext3 fallback),
  BIGALLOC refused loudly (`ERR_UNSUPPORTED`) — og BIGALLOC behavior not decoded in corpus
  (noted; refusing is the safe posture, registered in FALLBACKS census).
- **B3** — `M` majority-valid gate on dir blocks (`sane*2 >= total`).
- **B4 deleted classification** — `M` dtime‖links==0 predicate + inode sweep + bitmap
  grading (agent D sheet; corpus does not decode the og's ext-deleted predicate in more
  detail).
- **B5 orphan** — `M` journal-supplied names for unvisited inodes ("InodeNNNN" og naming
  parity per :587–589 comment).
- **B6 damaged-volume** — **S-004** (journal salvage surface).
- **B9 timestamps** — `M` unix→100ns ns precision.
- **B10 Unicode** — `M` post-S-005-fix (shared `text::utf8_to_u16` decoder;
  `Ext4Engine.NonAsciiNamesDecodedNotByteLifted` proves CJK/accent/invalid
  names decode to true code points).
- **B7 sparse** — `M` unwritten-extent semantics (0x8000 hole/extents, :215).

## Constants and formats

- **C19/C20** — `M` (all SB offsets verified against decompile; GDT 32/64-byte entries
  with desc_size from the 64-bit feature flag, matching og feature-correction path).
- **C21** — `M` FsKindCode Ext2/3/4 = 4/5/9 = og factory codes.

## Algorithms

- **A25** — `M` structure walk (SB → GDT → inodes → extents/dirs); **S-004** journal.
- **A26** — `M` htree dirs treated as linear streams (og depth not decoded either — both
  sides linear; corpus does not document dx_dir hash descent in the engine).
- **A27** — `M` extent ladder + journal-extent bounded reads (64 MiB cap).

## State machine

- **S32** — `M` cancel per journal block / per inode window.
- **S35** — ~~N-004~~ **FIXED** (`FIXED/N-004-N-006` group: RetryPolicy::backtrack at the transport layer) + ~~N-073~~ **FIXED-ALREADY** (FIXED/N-073, session 14: the ext4-path residual closed by the same landing, proven with 3 engine-path pin tests); `M` on SB-reject posture (core rejects
  invalid SB like og; no clamp-vs-reject divergence here — both reject on the quoted
  field set).

## Performance

- **X51** — `M` GDT ≤8 MiB, journal ≤64 MiB, 1 MiB window discipline (og 1 MiB journal
  windows verified).

## Quality

- **Q65 tests** — strong (7 areas incl. jbd2 salvage + spec-true inode offsets). Gaps:
  unreferenced-journal-block case (S-004 fix), non-ASCII names (S-005 fix), BIGALLOC
  refusal pinned.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-004 | SEVERE | Journal salvage: descriptor-referenced only vs og every-block (+ jsb gate direction) — **FIXED `51ac671` (session-2 verified)** |
| S-005 | SEVERE | Names byte-lifted → mojibake (cross-engine, filed here for ext4) — **FIXED (diff-to-fix session)** |
| N-004 | MINOR | CatchBuffer backtrack lives in transport layer, not in-engine — **FIXED (transport landing, DFX12-reconciled)** |
| N-073 | MINOR (s6) | The engine-path residual of the same salvage (this finding's text) — **FIXED-ALREADY (session 14: N-004's landing + engine-path pin tests)** |

## Corpus errata candidates surfaced by this wave

1. `ext4-hfs-engine-algorithms-v2.md` §1 "+0x18 log_block_size low-bit guard" is actually
   byte SB+0x60 = s_feature_incompat bit 0 (COMPRESSION) — core right, artifact wrong.
2. `ext4-engine--deep-dive.md` backup-SB positions (8193, 32768, …) are kernel-knowledge
   overreach, not in the decompile (CORRECTIONS #4 already says UNVERIFIED; core does not
   probe backup SBs either — consistent).

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 nanoseconds — I-006 (INFO, superset):** core's `decode_extra()`
  (`cr_ext4_engine.cpp:88-93`) extracts nsec from `*_extra` word (kernel
  `ext4_decode_extra_time` algorithm). `unix_to_filetime_ns(sec, nsec)`
  (`cr_types.h:56-61`) preserves nsec at 100ns granularity. Og's
  `EXT4SearchDLL` reads only seconds (`i_ctime` at +0x0C, `i_mtime` at +0x10)
  and uses `_gmtime32_s` (second-granularity) + `wMilliseconds = 0`
  hardcoded — sub-second precision silently dropped. Core is strictly more
  precise; .rsf byte stream differs by sub-second 100ns units.
- **B9 extra timestamps — I-007 (INFO, superset):** core reads all 4
  timestamps (created/modified/changed/accessed) via `i_atime_extra`,
  `i_ctime_extra`, `i_mtime_extra`, `i_crtime`, `i_crtime_extra`
  (`cr_ext4_engine.cpp:283-297`). Og reads only 2 (ctime + mtime) at
  `EXT4SearchDLL_decompiled.c:6262-6282`; the 3rd/4th FILETIME slots
  (`props+0x4b`/`+0x55`) are initialized to 0 and NEVER assigned. Core
  superset — every ext4 file has non-zero `created`/`accessed` slots where
  the og's has zeros.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-006 | INFO | Nanoseconds superset (core preserves nsec; og drops to second-only) |
| I-007 | INFO | Core reads 4 timestamps, og reads 2 (created/accessed dropped) |
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/ext4-hfsplus-session2.md`):**
- **S-004 CONFIRMED + mechanism refined + NEW subsystem:** og journal passes run in the MAIN
  scan (healthy volumes too); pass 1 scans EVERY block from jsb.start (not
  descriptor-referenced); jsb gates (magic 0xC03B3998 bswapped, blocktype {3,4}) — core gates
  nothing; **journal inode-image salvage (extent magic 0xf30a, 0x40-byte image copy) has no
  core counterpart** — filed inside S-004's session-2 update as the missing-subsystem item.
- **S-005 (fixed) og-anchor re-confirmed:** EXT4 passthrough (zero conversion imports).
- **Matches confirmed:** SB gate (`:3223` ↔ `:102-104`), **primary-SB-only is og parity**
  (`:3303` — no backup probes; settles the CORRECTIONS row 4 question), 64-bit GDT, block-size
  ladder, journal escape restoration (core strictly better, og-compatible), naming/cancel
  discipline. BIGALLOC: og gate NOT-FOUND in detection; core refusal stays a registered
  safety choice.

### Session-2 addendum (post-reconciliation)
S-004's core half is FIXED (`51ac671` — every-block + jsb gate, verified against the
decompile in the FIXED file); the journal **inode-image salvage** residue (0xf30a) and the
runs-in-main-scan condition remain open — see the S-004 FIXED file's session-2 note.
S-005 (FIXED) og-anchor re-confirmed: ext4 passthrough. Parallel-leg cross-refs: I-006
(nanoseconds superset), I-007 (timestamp breadth), M-010 (invalid-UTF-8 destruction —
complementary to this leg's N-029).

---

## Session-3 parallel-leg addendum (S3-EHB residues; core `34b3441`, corpus `03ad46c`)

The parallel leg's ext4 inventory (`evidence/decompiled-quotes/s3-ext4-inventory.md`)
deepens the rows above without changing verdicts:

- **Journal-image mechanics completed (S-004's missing-subsystem item):** the
  pre-pass `FUN_18000e5b0` feeds every journal block to the inode-image walk
  `FUN_180005e50` (stride = inode_size, bound = block_size, :4417/:4422), collecting
  {size≠0, EXTENTS_FL, 0xF30A} images — the og's only mechanism that gives zero-size
  deleted dirs a size. Core has no counterpart (per S-004's session-2 note); this
  file is now the mechanics evidence for that note.
- **I-014 (rows 1-3):** (1) core's deleted-inode sweep = superset (og has NO dtime
  sweep; its deleted surface = journal remnants + stale merges + image sizes);
  (2) og's journal inode walk steps physical+1 per logical block (:12361/:12247) —
  WRONG for fragmented journal inodes; the core's extent-map read (:678-680) is
  correct — register "do not replicate"; (3) X51 read patterns MATCH (og
  block-granular in-engine cache vs core 8 MiB/64 KiB CachedReader — equivalent
  device traffic).
- **Dedup semantics:** og skips physical blocks already seen (:12329-12343); core
  dedups by {inode,name} (:830) — different keys, same net effect on yield (no
  finding; matrix note).
- **Re-verified MATCH:** SB gate `:3223` ↔ `:102-104`; primary-SB-only parity
  (`:3303`); 64-bit GDT; block-size ladder; journal escape restoration (core
  strictly better, og-compatible); BIGALLOC refusal stays a registered safety
  choice (og gate NOT-FOUND — unchanged).

---

## Session-3 parallel-leg function census (integration 2026-10-26; baseline `7fd00e0`')

(Local IDs S-018/S-019/S-020/M-021/I-014 renumbered to S-025/S-026/S-027/M-037
during reconciliation; the ext4 supersets register itself is the lead leg's I-014.)

| ID | Severity | One-line |
|---|---|---|
| S-025 | SEVERE | Journal-salvaged entries flat vs og two-pass + reparenting — **FIXED (closing session): orphan map from "."".." heads, topological pre-pass with full inode truth + live anchors, iNode%u fallback for unresolvable chains, claim checks via live_nodes, sweep skip-set; 3 tests all fail-before; suite 465/465** |
| S-026 | SEVERE | meta_bg group-descriptor resolution missing — files silently missing on meta_bg volumes — **FIXED (closing session): kernel/e2fsprogs-correct resolver, empirically verified on real mke2fs meta_bg images; og stride/offset defects not adopted (D-58); 5 tests, 4 fail-before; suite 401/401** |
| S-027 | SEVERE | Backup-superblock lost-volume scan missing — **FIXED (closing session): engine-side rescue in scan_ext4 — 1 MiB-grid canonical-position probe, exact s_block_group_nr rebasing, GDT front-to-backup ladder; og rebase defect NOT adopted (D-59); 4 tests, 3 fail-before; suite 434/434** |
| M-037 | MAJOR | Dir-block parse resilience — **FIXED (s10)**: the og's 4-byte resync slide + padded floor + type gate + inode#-count gate (live walk); inode-0 continue kept as pinned superset. findings/FIXED/M-037 |

Residual probes: OQ-E1 (orphan reachability semantics), OQ-E2 (meta_bg stride
constant — **RESOLVED by the S-026 fix**: the true stride is block_size/desc_size,
empirically verified; the og's s_desc_size seed is the defect, D-58) —
`tracked-open/OQ-Z-E-FLT-session3.md`.
constant) — `tracked-open/OQ-Z-E-FLT-session3.md`.

---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-057→N-073 | MINOR | (renumbered at integration — the table's original N-057 ID was a pre-coordination session-6 local number) CatchBuffer bad-sector salvage: og re-reads a 1 KiB-backtrack shifted window on failure (ext4-engine--deep-dive.md:25-33); core zero-fills the extent after transport retries — **FIXED-ALREADY (session 14: N-004's transport landing covers it; 3 engine-path pin tests in test_ext4_volume.cpp)** |

## Session-10 S10-F rows (function-level completeness pass, 2026-10-29; audited `44f043a5`)

| Parameter | Verdict |
|---|---|
| volume-label decode — s_volume_name surface | **DIVERGENCE-MINOR — N-133** (the og strncpy's s_volume_name for the UI, algorithms-v2 §2 step 5; the core decodes it at cr_ext4_engine.cpp:200-201 into `Volume::volume_name` with ZERO consumers — dead code; the engine-side label is the image-scan source M-079's addendum now points at) |
| class surface — CEXT4FileProperties / CEXT4QuickSearch / EXT4CatchBufferObj / Ext4Volume / CExt4DataRunList | **MATCH (semantic)** — the T3b class pass: props field set full-parity (I-006/I-007 + superset registered), quick/complete one-entry flags (I-016), the catch-buffer surface fully dispositioned (backtrack N-004/N-073; spawn S-027/D-59; buffering I-014-3/X51 — the og's 20 MiB window size is uncited but perf-only), the volume context and extent adapter mapped — evidence s10f-product-frame-register.md §5 |


## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

New active findings on this module: **S-065** **FIXED** (the S56 ACUTE-first
lane: the ladder bounded by ceil(i_size/bs) + the contiguous merge + the
65536-extent hardening budget + the cancel token inside the walk —
findings/FIXED/S-065, 4 RED->GREEN read-count/cancel/merge pins), **M-143** (the journal inode-image
harvest branch absent — the S-004 sibling surface), **M-144** (the 8 MiB GDT cap
refusing >16 TiB 64BIT volumes + MAX_GROUPS >128 TiB; the og unbounded),
**N-193** (four SB acceptance-band deltas consolidated — register; RENUMBERED **N-202** at the S56 lane's collision repair, seat 202), **I-045** (the
unwritten ee_len og-defect non-adoption register, batch #3) **— I-045 ACKNOWLEDGED-RETIRED (the S58 diff-to-fix execution lane: PARITY-DELTAS D-141 registered + the `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect` anti-down-fix pin RED-proven — findings/FIXED/I-045)**. Verified-MATCH this
session: the size-bounded indirect walk's og form (FUN_180005490), the journal
dir-block classifier's '.'/'..' test, the GDT allocation math, and the
extents-path budget (the S-065 gap is the non-extents path only — now closed).

## Closing-fix lane addendum (2026-10-29 — the INFO-first fix lane)

**I-045 FIXED (ACKNOWLEDGED)** — the unwritten ee_len register landed as
PARITY-DELTAS **D-141** (batch #3 of the I-026/I-040/D-123 family) with the
anti-down-fix pin `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect`: an
unwritten extent (ee_len 0x8005) surfaces a 5-block content extent, never
the og's raw product — the core's kernel-correct decode
(walk_extent_node :346-350) stays; the divergence is deliberate and must
be preserved in future work. The module's B7 row (unwritten-extent
semantics) is now backed by both the D-141 register and the pin.

## S72 fix-lane addendum (2026-10-29 — the MAJOR-first continuation lane)

**M-144 FIXED (MAJOR)** — the 8 MiB single-shot GDT read cap at
`cr_ext4_engine.cpp:785` is removed and replaced with an 8 MiB chunked
reader that tolerates partial chunk failure (the dead-front behavior the
prior single-shot read applied to the whole table now applies per-chunk);
`MAX_GROUPS` at `:83` is raised from `1<<20` (~128 TiB at 4 KiB blocks)
to `1<<26` (~1 PiB) — still a corrupt-SB guard that no real volume hits,
but no longer a capability cliff that legitimate NAS arrays cross. The
alt-GDT ladder path is chunked identically. The og's unbounded allocation
(FUN_1800047c0 :3404-3408, re-read first-hand) is its own defect class (a
corrupt SB could request an arbitrary buffer); the core's deliberate
divergence is registered in PARITY-DELTAS **D-148**.
`Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens` RED pre-fix
(`0x50030003 ERR_BAD_PARAMETER`) → GREEN post-fix (`block_groups == 200000`);
`Ext4EngineM144.CorruptHugeGroupCountStillRefused` guards the raised
bound. The module's X49/X52/B1 capacity-edges rows are now MATCH
(parallel to the og's unbounded scan, with the corrupt-SB guard
deliberately kept and registered).

## S73 fix-lane addendum #2 (2026-10-29 — M-143 the journal inode-image
harvest)

**M-143 FIXED (MAJOR)** — the journal pass's `parse_one` lambda is
extended to call `parse_inode_image_block` after `parse_dir_block`
rejects a block. The new function walks the block at `inode_size`
stride and applies the og's exact gate set (FUN_180005e50 :4419-4420,
re-read first-hand): `i_size_lo (+0x04) != 0 && (i_flags (+0x20) &
0x80000 [EXTENTS_FL]) && extent magic (+0x28) == 0xF30A && eh_entries
(+0x2A) != 0`. Records that pass the gate are decoded via the engine's
existing extent walker over the inline extent root (i_block, 60 bytes),
and emitted in pass 2 as orphan "Inode%N" records under root_lost with
the inode's extents as content. The synthetic inode# is
journal-positional (`0x10000000 + (jb << 10) + slot`) — above
MAX_INODES so it never collides with real inode numbers. The og's
exact inode# derivation is the finding's open question, preserved per
the finding's "does not gate the fix" note. The "Inode%u" naming
follows the engine's existing deleted-inode convention for consistency.
The directory-model-takes-precedence order matches the og's branch
order. 6 RED→GREEN tests in
`tests/unit/test_ext4_m143_journal_inode_image.cpp`. The module's
B2/A26/A25 journal-salvage rows are now MATCH (the inode-image surface
joins the directory-block surface as the og's two-branch journal
classifier).

## Session-71 addendum — OQ-S52-1 resolved (2026-10-29)

**The ext-family kind classification (the tracked-open item closed).** The
analyser's verdict rule decoded first-hand (ExtFileSystemAnalyser :3616-3680,
confirming and extending the S47 probe): the strict ext4 gate is
feature_incompat == EXTENTS|64BIT|FLEX_BG (0x2c0, all three) with the
desc_size==0x40 companion; the pre-ext4 family requires incompat within
{FILETYPE, HAS_JOURNAL, META_BG} and ro_compat within {SPARSE_SUPER,
LARGE_FILE, DIR_INDEX}, HAS_JOURNAL splitting ext3/ext2 inside the analyser's
verdict space — which the ENGINE space collapses to the single family code 4
(M-133's twin-key shift member; the DRWUI factory creates the EXT2 searcher
for cases 4 AND 5 — the collapse is proven, the verdict→engine renumbering
site itself is cross-DLL and not load-bearing). The display layer's verdict
table decoded at DRWUI:1432740-1432815 (5=(EXT2), 6=(EXT3), 0x18=(EXT4),
4=(NTFS), 9=(SWAP), 0xd=(REFS)...). Landing: `FsKindCode::Ext2 = 4`; the
engine stamps `ext_family_kind(sb)` at every node/root site; twin_key_of's
shift set extends to {Fat16, Ext2} = the og's {1, 4}; og-analyser-refused
shapes keep the unshifted Ext4 stamp (no og behavior exists for them). Proof:
`Ext4Engine.FamilyKindStampsPerDecodedAnalyserRule` (the four shapes) +
`Grading.TwinKeyExt2FamilyShifts` (the previously-unexpressible shift leg) +
the 5 prior M-133 pins. Suite 1541/1541.
## Session-77 addendum (the S79 continuation audit lane; core `3799dee2`, re-verified at `0da3490b`, corpus `be0cfe9`)

- **M-152 (MAJOR, NEW):** the sweep's orphan-reserved skip — the og's inode-table harvest
  filter (FUN_180005e50 :4415-21, first-hand, corrected decode: skip iff `i_size_lo==0 ∨
  ¬EXTENTS_FL ∨ magic≠0xF30A ∨ eh_entries==0` — the +0x2A word is eh_entries, NOT the
  depth) applies NO dtime/links predicate; the core's `dtime==0 && links!=0` skip
  (`cr_ext4_engine.cpp:1260-61` at the re-verified pin) drops the orphan-active population
  (links>0, dtime==0, unreachable from the dir tree — the lost-file population a deep scan
  exists for), unregistered. The og-side emission chain for unclaimed harvest was traced to
  the filter, the +0x4a8 map, and the finalize pattern — the fix plan's first test
  falsifies it end-to-end. Distinct from M-143's journal-inode harvest (the image branch).
  Evidence: `s79-lead-verification.md` §A.
---

## Session-82 addendum — M-152 FIXED: the orphan-active population surfaces (2026-10-29)

**The sweep's orphan-reserved skip is gone.** The og's per-inode harvest
filter (EXT4SearchDLL :4415-4421, re-verified first-hand) gates on the
content shape alone — i_size_lo, EXTENTS_FL, the 0xF30A magic,
eh_entries — with NO deleted-state predicate; the harvested set resolves
against the live tree (the claim resolution), so orphan-actives (dtime
0, links != 0, referenced by no directory — the lost-file population the
deep scan exists for) surface as recoverable entries. The core's sweep
now harvests the same population: the `dtime == 0 && links != 0` skip is
removed, the visited-inodes guard is the claim resolution, the journal
inode is claimed before the sweep (engine state, not a result), and the
regular/dir mode gate stays as the registered D-153 hardening (renumbered from the born-collided D-150 seat at the S84 repair).
**B1/B5: MATCH.** Tests: OrphanActiveInodeSurfacesInSweep /
OrphanSweepDedupsAgainstLiveTree / SweepModeGateHolds (RED->GREEN);
the new orphan_admitted counter surfaces the population; ext4 family
56/56.

---

## Session-83 addendum — M-152 FIXED: the orphan-active population surfaces (2026-10-29)

**The skip is gone.** The core's `dtime==0 && links!=0` "orphan-reserved"
skip dropped exactly the population the og's predicate-free filter
admits. Corpus re-verified FIRST-HAND from the raw asm this session
(objdump over EXT4SearchDLL.dll; the filter at FUN_180005e50's sweep loop
0x180005f5d-0x180005f87: `cmp [r14+0x4],0` / `test [r14+0x20],0x80000` /
`cmp [r14+0x28],0xF30A` / `cmp [r14+0x2a],0` — the four content gates,
byte-exact, NO dtime (+0x14) or links (+0x1A) read anywhere in the loop).

The landing: the population split (dtime-set-or-links-0 → deleted_inodes,
unchanged; dtime==0 && links!=0 → the new orphan_inodes counter); both
surface under the deleted root with Origin::Structured and the existing
extent/bitmap confidence grading; the orphan-actives carry the honest
DeletedState::Active (no deletion evidence on the inode). The live-claim
dedup holds (visited_inodes — the og resolves claims the same way); the
mode gate stays as the registered hardening; the journal inode is claimed
by the journal machinery (PARITY-DELTAS D-163's claim row; the mode-gate half of the old D-152 combined section is D-153). RED→GREEN
first-hand: the old skip line reintroduced alone fails
OrphanActiveInodeSurfacesInSweep (:2500/:2506) and
SweepStatsCountOrphanAdmissions (:2576); the fix lands all 49 Ext4Engine
tests green. Tests: OrphanActiveInodeSurfacesInSweep /
OrphanSweepDedupsAgainstLiveTree / SweepModeGateHolds /
SweepStatsCountOrphanAdmissions.
