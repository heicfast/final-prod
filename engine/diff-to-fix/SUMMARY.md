# SUMMARY.md — executive summary (the S76 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-76 S76 lane delta (2026-10-29 — the user's MINOR-first directive continuation ["priority to minor first", 20+ fixes]; opened at core `1ec36d6c` (the racing S71/S73 lanes' M-136 convergent re-landing + M-143 absorbed at the push-rebase), corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; the racing lanes' own push gates held the moved tree green (M-143's 1507+ landing suite, M-136's convergent re-landing suite), this lane's first full-suite run was post-fix at 1526/1526 — the baseline at open was the build-started tree, not an independently re-run suite; honesty recorded)

- **N-131 FIXED (MINOR, session/fat/ntfs)** — both FS-native parent-anchor
  carriers landed: `ntfs_parent_ref` u64 (nflags bit-12 — the og's CNTFS
  +0x40, the $FILE_NAME parent reference masked to the 48-bit record number,
  FUN_18002a340's local_41 mask decoded first-hand at the fix; stamped
  per-link at the hard-link fan-out + the GUID-dump + the INDX
  second-source) and `fat_parent_cluster` u32 (nflags bit-13 — the og's CFAT
  +0x42, the containing directory node's own +0x3e origin cluster, the walk
  stamp decoded first-hand at FATSearchDLL :11654/:11500-04; the volume root
  anchors 0 on every flavor [engine init FUN_18000e130], a walked subdir
  anchors at its own first cluster). **The finding's exFAT +0x54 open
  question RESOLVED first-hand — NO CARRIER:** the exFAT engine never stamps
  +0x54 for children (:45155 stamps vt+0x28 only; the sole writes are
  root/lost-dir 0xFFFFFFFF inits at :3895/:5026/:11461 + clone copies) —
  the anchor-twin hypothesis and the census label both refuted; carrying
  nothing is the parity. RED→GREEN: 4 tests (Session round-trips ×2 with the
  zero-carrier gate + walk-level Fat/Ntfs stamps with session survival),
  RED as compile-errors on the stash-reverted tree (the N-100 new-carrier
  precedent); suite **1526/1526**.

**Board after this landing: 0 ACUTE · 0 SEVERE · 14 MAJOR · 10 MINOR · 0 INFO
active / 162 FIXED / 4 tracked-open** (M-086/M-087/M-089 + N-107/N-127
driver-repo-resident per OQ-S10-3's two-repo grant boundary). The lane-open
board re-truth corrected the `1ec36d6c` rebase message's own "10 N" off-by-one
(disk truth was 11 MINOR files; the S70 block carried the same 10). Next free
IDs: S-068+, M-149+, N-213+, I-048+, A-016+, D-149+.

## Session-70 S70 lane delta (2026-10-29 — the user's MINOR-first directive continuation ["priority to minor first", 20+ fixes]; opened at core `449fc594` (rebased over `2fb0cb95` — the S72/S73/S75 racing windows' M-136 + M-144 absorbed) (the S70/S71/S72 racing audit/repair windows absorbed at the reset; the trail-repair seat conceded to N-210's equivalent landing per SA-9; baseline suite 1422 tests / 1380 pass / 42 Win-skips / 0 fail re-run green on the moved tree before the first change), corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change)

- **N-201 FIXED (MINOR)** — the quick-open media-word re-flavor landed with
  the RAW-ASM-verified shape family: FUN_180021000's two-leg probe (FAT1
  then FAT2 first sectors, last read wins) transcribed from objdump of
  FATSearchDLL.dll 0x180021171-0x18002127c after Ghidra's decompiled text
  proved defective at this seam (a dropped mask digit making the F8-family
  tautologically false — caught by the core's -Werror — and a mis-rendered
  branch target hiding that the 0xFFFFFFFF media word assigns nothing).
  String-flavor-gated reachability (the D-94/I-002 Fat12 superset
  untouched); the flavor flip re-derives the flavor's own BPB field family;
  three registered boundaries (the no-assignment decline + backup retry not
  ported; the unconditional second read og-exact; the og's EOC-at-2,3
  mis-flip registered as an I-045-family defect with the spf-usability
  adoption rule). RED→GREEN: HybridStringFat16MediaFat32ReflavorsToFat32;
  pins Fat2ReadWinsOverFat1 + ConsistentVolumeUnreflavored; suite 1434/1434.

- **N-200 FIXED (MINOR)** — the lost-scan NTFS candidate acceptance now
  requires positive MFT evidence with the $MFTMirr retry exactly as the og's
  classifier (FUN_180011150 :9889-9906 → FUN_180011560 :10023-10075, both
  corpus sites re-read first-hand at the fix): the record-0 probe runs
  primary-first, then the og-exact mirror re-read; both legs failing drops
  the forged-OEM candidate as noise (the og's 0xfffffff1 slot). Mirror-only
  acceptance carries the `mft-mirror-probe` label + the mirror-blind
  strong-probe bonus. RED→GREEN: ForgedOemNoMftEvidenceRejected /
  TornPrimaryMftMirrorEvidenceScored / HealthyCandidateMftEvidenceUnchanged;
  suite 1425/1425. The boot-pair arm stays deferred on the finding's own
  open question (FUN_180011260's getter pair).

**Board after this landing (rebased over the racing S71/S72/S73 lanes'
N-210 + M-136 + M-144): 0 ACUTE · 0 SEVERE · 15 MAJOR · 9 MINOR · 0 INFO
active / 161 FIXED / 4 tracked-open** (N-107/N-127 + M-086/M-087/M-089
driver-repo-resident per OQ-S10-3).
# SUMMARY.md — executive summary (the S72 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-72 S72 fix-lane delta (2026-10-29 — the user's MAJOR-first directive continuation ["priority to major first", 20+ fixes]; opened at core `bbbe78b2` (the S71 seat conceded to the racing N-210 repair lane per SA-9; baseline suite 1503/1503 green re-run on the moved tree before the first change), corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change)

- **M-136 FIXED (MAJOR, writer/orchestrator)** — the recovery pass reads through the scan's own pipe: both entry points (the CLI recover verb + the ABI recovery_open) construct the writer's source via `build_read_pipe(src, 8 MiB, 256 KiB)` — the M-058 single construction seam with the scan's exact constants (SliceShim identity → RetryingDevice ×3 Skip-mode → CachedReader), the chain owned by the entry context and provably outliving the writer (local-before-writer in the CLI; a `read_pipe` member declared before `writer` in CrRecovery). The og's export reads are stream-mediated (arch/04 §7.5 + FUN_140540670's props+0x30 → vt+0x78 GetStream dispatch at DRWUI:873629-873671, re-read first-hand) — never a bare device handle; a transient read error the scan retried past no longer zero-fills the same span at export. RED: the wiring pin at the stash-revert state (both grep pins npos); GREEN: 4/4 Writer tests + the negative control pinning the removed bare-device behavior; full suite **1507/1507**. The LDM basis devices keep raw member reads on BOTH sides (scan + export — no disagreement; the og's candidate-side cache is the scan's own posture).

- **M-135 FIXED (MAJOR, writer)** — the og's error-9 family landed: a hard read
  failure (post-retry via the M-136 pipe's live error map — Skip-mode reads
  return OK with zero-filled bad sectors, the map is the only hard-failure
  signal; or the raw non-OK read on the pipe-less bases) truncates the item at
  the FIRST unreadable byte, KEEPS the partial (commit, never delete), records
  Outcome::ReadError + cause_id + first_failed_offset + bytes_written
  (additive-within-v1), fires the verbatim three-line "Failed to ReadSector!"
  trio through cr::log, and the pass continues. The straddling-read clamp
  (:875666-68) landed (cause 4 for beyond-volume starts); the ERR_TRUNCATED
  refusal is replaced by the og's partial-keep (cause 7 — the og silently ends
  short runlists; the core's report is the truthful superset); the per-class
  split keeps undecodable-compressed zero-fill + Salvaged (the og's chunked
  path). The cause list CORRECTED first-hand: SEVEN compiled-in entries
  (strings :30580-30596) — the finding's qm citation misread cause 4 (the
  runtime cause 4 is the sector-location string, not "file name is most
  likely lost") and missed cause 7; FUN_1400ed760 REFUTED as the mapper (a %d
  formatter — the open question resolved-as-refuted); the core maps by cause
  semantics, registered D-149. The CLI renders the numbered causes. RED: the
  link-failure with the implementation stashed + both re-pinned tests'
  first-hand failures; GREEN: 8 new/re-pinned tests; suite **1515/1515**.

**Board after this landing (M-136 + M-135 this lane; M-144 the racing S73 lane's): 0 ACUTE · 0 SEVERE · 14 MAJOR · 11 MINOR · 0 INFO active / 159 FIXED / 4 tracked-open.** (M-086/M-087/M-089/N-107/N-127 remain driver-repo-resident per OQ-S10-3's two-repo grant boundary.) Next free IDs: S-068+, M-148+, N-210+, I-048+, A-016+, D-149+ (D-148 taken by M-144's registration).


# SUMMARY.md — executive summary (the S73 MAJOR-first continuation fix lane on top; the prior lanes below, newest-first)


## Session-68 S68 continuation-audit lane delta (2026-10-29 — the user's second-session continuation mandate ["now you have go look for other things which session 1 missed and continue working"]; opened at core `f3da99f1`, the racing fix lanes' M-144 landing absorbed at the rebase (`ee19145c`), corpus `be0cfe9` throughout; the lane's filings verified at `9be37b68`; read-only lane — diff-to-fix/ + worklog* only)

- **N-212 filed (MINOR)** — the ASF validator's acceptance band: the
  header object-count gate (`count == 0 || count > 128`) and the header-size
  floor (`obj_size < 30`) reject torn-header candidates the og's analyser
  never gates on (the tier-1 at RVA 0x37030 raw-asm-verified as the 16-byte
  GUID compare ALONE — Ghidra's decompile dropped the buffer param; the
  count field at +0x18 is never read anywhere in the og's ctor/tier-1/walker);
  and the extent source (the og's FileProperties-declared-size at +0x28,
  maxed with the last top-level position, clamped by global×500 — vs the
  core's walked-object end). Both og walk phases decoded first-hand
  (FUN_180037210 :41994-:42110).

- **The registry-initializer decode (zero-divergence verification):** all
  66 u32 keys of the og's static registry mapped first-hand to core rows —
  ALL MATCH, including the census doc's four formerly-ambiguous chains
  (0x1cd = CD01-SVG → FMT_SVG_CD01 [the M-140 family]; 0xd302020 = DXF
  "  0\r" → FMT_DXF; 0x4e444221 = PST !BDN → FMT_PST; 0x694c5153 = the
  FCPEvt/SQLite chain → FMT_SQLITE + N-089's relabel). The raw_signs
  min/max ×1024 question RESOLVED as the og's own scan-time `<<10`
  (FUN_1800205b0 :25020+) — the core's generated table and its documented
  0x7d000 KiB absent-max default are exact. **All 199 header + 62 tail
  signs byte-exact** vs the corpus json.

- **The tier-1 acceptance-gate audit (zero-divergence verification):** the
  15 core long-magic rows verified against the og's analyser tier-1 checks
  (raw-asm where Ghidra mis-decompiled) — the ASF 16-byte GUID compare, the
  FCPEvt 15-byte "SQLite format 3" memcmp (+ the core validator's t3\0
  tail re-gate), the MOBI BOOK@60/MOBI@64 BE pair, TAR ustar@257, 7Z 6-byte,
  the LZH -lhX family + buf[6] second gate, VCF 11B, PNG 8B, PDF 5B,
  RAR4 7B. ISO/RAR5/M4A/ZIP_CD/MOV_NAKED confirmed as the D-60l registered
  core-only anchors.

- **Five census-doc label corrections** filed as CORPUS-ERRATA session-68
  rows 10-14 (the corpus's own doc vs its binary: the DXF key, the WMF
  std-header key, the PST !BDN key, the TTF-sfnt-vs-ICO/CUR conflation,
  the FCPEvt SQLite binding).

- **Board at this lane's close (re-truthed post-push):** 0 ACUTE · 0
  SEVERE · **15 MAJOR · 12 MINOR active** / **159 FIXED** / 4 tracked-open
  (M-144 [ext4] + M-136 [writer] landed by the racing fix lanes during this
  audit — both absorbed at the push-rebases; N-212 this lane's addition).


## Session-73 S73 lane delta (2026-10-29 — the user's MAJOR-first directive continuation ["priority to major first", 20+ fixes]; opened at core `f9f8c6e4` (the S68 continuation-audit lane absorbed; the racing S69/S70/S71/S72 audit+fix lanes conceded per SA-9 with disjoint slices taken), corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change)

- **M-144 FIXED (MAJOR, ext4)** — see the prior S73 lane delta below for
  the full Resolution (the 8 MiB single-shot GDT read cap replaced with
  an 8 MiB chunked reader; MAX_GROUPS raised 1<<20→1<<26; the og's
  unbounded stance registered in PARITY-DELTAS D-148).

- **M-143 FIXED (MAJOR, ext4)** — the journal pass's `parse_one` lambda
  is extended to call `parse_inode_image_block` after `parse_dir_block`
  rejects a block. The new function walks the block at `inode_size`
  stride and applies the og's exact gate set (FUN_180005e50 :4419-4420,
  re-read first-hand this session): `i_size_lo (+0x04) != 0 &&
  (i_flags (+0x20) & 0x80000 [EXTENTS_FL]) && extent magic (+0x28) ==
  0xF30A && eh_entries (+0x2A) != 0`. Records that pass the gate are
  decoded via the engine's existing extent walker over the inline
  extent root (i_block, 60 bytes), and emitted in pass 2 as orphan
  "Inode%N" records under root_lost with the inode's extents as
  content. The synthetic inode# is journal-positional
  (`0x10000000 + (jb << 10) + slot`) — above MAX_INODES so it never
  collides with real inode numbers. The og's exact inode# derivation
  is the finding's open question, preserved per the finding's
  "does not gate the fix" note. The "Inode%u" naming follows the
  engine's existing deleted-inode convention for consistency. The
  directory-model-takes-precedence order matches the og's branch
  order. 6 RED→GREEN tests: `JournalInodeImageSurfacesInodeRecord`
  + `ZeroSizeSkipped` + `NoExtentsFlagSkipped` +
  `BadExtentMagicSkipped` + `ZeroEhEntriesSkipped` +
  `DirPairTakesPrecedence`.

**Board after this landing (M-143 + M-144 absorbed): 0 ACUTE · 0 SEVERE ·
15 MAJOR · 11 MINOR · 0 INFO active / 158 FIXED / 4 tracked-open.**
Next free IDs: S-068+, M-148+, N-210+, I-048+, A-016+, D-149+.

## Session-73 S73 lane delta (M-144 first landing — prior delta preserved below)

- **M-144 FIXED (MAJOR, ext4)** — the 8 MiB single-shot GDT read cap at
  `cr_ext4_engine.cpp:785` (which refused any 64BIT volume whose GDT
  exceeded 8 MiB — i.e. 64-B descriptors above 16 TiB at 4 KiB blocks)
  is removed and replaced with an 8 MiB chunked reader that tolerates
  partial chunk failure (the dead-front behavior the prior single-shot
  read applied to the whole table now applies per-chunk). `MAX_GROUPS`
  at `:83` is raised from `1<<20` (~128 TiB) to `1<<26` (~1 PiB) —
  still a corrupt-SB guard that no real volume hits, but no longer a
  capability cliff that legitimate NAS arrays cross. The alt-GDT
  ladder path is chunked identically. The og's unbounded allocation
  (FUN_1800047c0 :3404-3408, re-read first-hand this session) is its
  own defect class (a corrupt SB could request an arbitrary buffer);
  the core's deliberate divergence is registered in PARITY-DELTAS
  **D-148**. `Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens` RED
  pre-fix (`0x50030003 ERR_BAD_PARAMETER`) → GREEN post-fix
  (`block_groups == 200000` on a 200k-group 64BIT volume with a 12.8
  MiB GDT); `Ext4EngineM144.CorruptHugeGroupCountStillRefused` guards
  the raised bound.

**Board after this landing (M-144 absorbed): 0 ACUTE · 0 SEVERE ·
16 MAJOR · 11 MINOR · 0 INFO active / 157 FIXED / 4 tracked-open.**
Next free IDs: S-068+, M-148+, N-210+, I-048+, A-016+, D-149+.

# SUMMARY.md — executive summary (the S66 MINOR-first fix lane on top; the prior lanes below, newest-first)

# SUMMARY.md — executive summary (the S71 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-71 S71 lane delta (2026-10-29 — the user's MAJOR-first fix directive ["priority to major first", 20+ fixes]; opened at core `c736e9b3`, corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; the lane's opening repair: N-210, the M-120 class recurrence — 4 files of committed conflict markers from the `f3da99f1` merge resolved per the stacked-history convention + the ci-local markdown marker lint)

- **M-136 FIXED (MAJOR)** — the recovery reads ride the scan's retry+cache
  pipe: both entry points (CLI recover + ABI `cr_ui_recovery` create) build
  `orchestrator::build_read_pipe(source, 8 MiB, 256 KiB)` and hand the writer
  the pipe (the ReadPipe outlives the writer beside it); the LDM basis arms
  ride the same substrate (per-member pipes on Windows, the recovery pipe as
  the arm/fallback source); `SafeWriter::set_read_error_source` keeps the
  pipe's Skip-mode zero-fill visible as Salvaged (never a clean Done). RED:
  the pre-fix bare-device shape is pinned as the executable contrast
  (`BareDeviceLegacyShapeZeroFillsTransients` — the finding's falsifiable
  case). GREEN: 4 new WriterReadPipe tests; suite 1507/1507.

## Session-66 S66 lane delta (2026-10-29 — the user's MINOR-first directive continuation ["priority to minor first", 20+ fixes]; opened at core `24e480c4` (the S60-B/S63 windows absorbed; baseline suite 1485/1485 green on Linux before the first change), corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; the S64/S65 racing seats absorbed at the rebase — their committed-conflict-marker SUMMARY repaired in the same push, the M-120 class)

- **N-203 FIXED (MINOR)** — the resume round-trip keeps the candidate's
  data-area extent: PartitionContext v4 (data_start_lba/data_sector_count,
  77-byte record, v1..v3 load zeroed), the load-path restore, and
  revalidate_candidate's scratch copy-back (the first-resume repair for
  pre-v4 records — the og's 0x448 image replays with the extent by
  construction; the {start,end,disk} dedup key re-verified first-hand at
  PRD:20825-20831). RED: the load fill + enrichment surgically disabled —
  3×FAILED; GREEN: ResumeRevalidation 11/11, Session/LostDiscovery/Crsf
  101/101. The finding's dedup-consumer case is architecturally unreachable
  as stated (deep-rerun resumes REPLACE the restored list) — the
  RestoredWindowMatchesFreshDiscovery pin holds the equivalent invariant.

- **N-194 FIXED (MINOR)** — the og's two byte-size display formatters landed:
  `query::display_size_log` (FUN_140163580/650, DRWUI:264525-264583 — KB/MB/GB
  only, `%.2f`, 1024-tier divisors, +1 flip boundaries, <1-writes-nothing) and
  `query::display_size_ui` (the tr-family, DRWUI:1556295-1556400 — the
  B..TB ladder, caller precision, zero-forced-0, the all-zero-fraction strip
  decoded first-hand from the replace tail). The CLI's four size prints wired
  through the log-faithful form (no surface pinned the old MiB forms —
  verified). RED: link-failure Not-Run with the implementation stashed; GREEN:
  Display pins + Query/Display/Error/CrUi 110/110 + win_syntax_check green.

**Board after this landing (M-142 + M-140 + N-194 + N-203 absorbed): 0 ACUTE ·
0 SEVERE · 18 MAJOR · 11 MINOR · 0 INFO active / 155 FIXED / 4 tracked-open.**
Next free IDs: S-068+, M-148+, N-210+, I-048+, A-016+, D-148+.

# SUMMARY.md — executive summary (the S65 diff-to-fix execution lane on top; the prior lanes below, newest-first)

## Session-65 S65 fix-lane delta (2026-10-29 — the user's fix mandate ["priority to informational first", 20+ fixes]; opened at core `24e480c4`, corpus `be0cfe9` throughout; baseline suite 1485/1485 green before the first change)

**The informational-first pass verified:** all 21 I- findings ride findings/FIXED/
with their ACKNOWLEDGED/INVALID/FIXED-ALREADY resolutions current — the INFO tier
is fully closed; no new INFO findings exist to acknowledge. The lane then works
the MINOR/MAJOR ladders (the SEVERE/ACUTE tiers are empty).

- **M-142 FIXED (MAJOR, hfsplus)** — the VH validator's fork clamps replaced with
  the og's actual gates (`free_blocks <= total_blocks && journal_info_block <=
  total_blocks`, FUN_18000c6e0 :10005-10006 re-read first-hand with the
  frame-offset derivation and the CONCAT13/CONCAT12 swap band). The finding's open
  question resolved first-hand: FUN_18000cf90 (:10335) is the 16-byte fork
  byte-swapper (driven by FUN_18000d030 over all five forks) — no fork compares
  anywhere in the og's VH path, so the clamps belong at no stage. N-005 blockSize
  bounds and total_blocks==0 refusal preserved. AUDIT-LINUX-FS §2.1 row 1
  corrected; CORPUS-ERRATA Session-64 row records the corpus deep-dive's
  misreading. RED 4/5 → GREEN 29/29 (HfsPlus family).

**Board after this landing:** 0 ACUTE · 0 SEVERE · 19 MAJOR · 13 MINOR · 0 INFO
active / 152 FIXED / 4 tracked-open. (Lane name S65 — the racing window took
the S64 seat with its own worklog open; both lanes ride the same mandate.) (M-086/M-087/M-089/N-107/N-127 remain
driver-repo-resident per OQ-S10-3's two-repo grant boundary.)

# SUMMARY.md — executive summary (the S64 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-64 S64 lane delta (2026-10-29 — the user's MINOR-first fix mandate ["priority to minor first", 20+ fixes]; opened at core `24e480c4` (the S60-B/S63 hot-window merge head), corpus `be0cfe9`; the S55 filings' audited commit `9633d94f` — every finding re-verified at the current tree before fixing)

**Opening repair (main was RED at open): the FOURTH born-collided
D-seat repair per SA-9.** The doc_truth_gate failed at the lane-open
run — PARITY-DELTAS carried two D-143 rows (the S60-B window's N-196
registration AND the S63 lane's S-040 registration; the S63 push raced
the window and took a seat that was taken). N-196 landed first and
keeps D-143; **S-040's registration renumbers D-143 → D-147** (the
N-205 ride filled D-144..D-146). Every live citation swept (the
PARITY-DELTAS row, four code comments, ABI-CONTRACT 7j,
REFERENCE-API-GAP ×2, API-DELTA, modules/{imaging,cli,abi,tree}.md, the
ledger row, two matrix rows, the SUMMARY S63 block, both finding
files); two stale mid-ride N-205 citations (matrix row 1184 and
modules/tree.md still saying D-143; the ledger row still saying D-144)
repaired to the D-146 terminal seat in the same pass. Dated worklog
history stays (the N-107 dated-record convention). Gate green at 140
unique D-rows after the repair. No behavior change — trail repair only.

**Board at lane open (re-counted first-hand from the filesystem):**
0 ACUTE · 0 SEVERE · 20 MAJOR · 13 MINOR · 0 INFO active / 151 FIXED /
4 tracked-open. N-107/N-127 (with M-086/M-087/M-089) are
driver-repo-resident (OQ-S10-3's two-repo grant boundary) — not
fixable in core, counted active-open. Next free IDs: S-068+, M-148+,
N-210+, I-048+, A-016+, D-148+.

# SUMMARY.md — executive summary (the S65 diff-to-fix execution lane on top; the prior lanes below, newest-first)

## Session-65 S65 fix-lane delta (2026-10-29 — the user's fix mandate ["priority to informational first", 20+ fixes]; opened at core `24e480c4`, corpus `be0cfe9` throughout; baseline suite 1485/1485 green before the first change)

**The informational-first pass verified:** all 21 I- findings ride findings/FIXED/
with their ACKNOWLEDGED/INVALID/FIXED-ALREADY resolutions current — the INFO tier
is fully closed; no new INFO findings exist to acknowledge. The lane then works
the MINOR/MAJOR ladders (the SEVERE/ACUTE tiers are empty).

- **M-142 FIXED (MAJOR, hfsplus)** — the VH validator's fork clamps replaced with
  the og's actual gates (`free_blocks <= total_blocks && journal_info_block <=
  total_blocks`, FUN_18000c6e0 :10005-10006 re-read first-hand with the
  frame-offset derivation and the CONCAT13/CONCAT12 swap band). The finding's open
  question resolved first-hand: FUN_18000cf90 (:10335) is the 16-byte fork
  byte-swapper (driven by FUN_18000d030 over all five forks) — no fork compares
  anywhere in the og's VH path, so the clamps belong at no stage. N-005 blockSize
  bounds and total_blocks==0 refusal preserved. AUDIT-LINUX-FS §2.1 row 1
  corrected; CORPUS-ERRATA Session-64 row records the corpus deep-dive's
  misreading. RED 4/5 → GREEN 29/29 (HfsPlus family).

- **M-140 FIXED (MAJOR, carve)** — the CSL/CD01 decline fall-through removed:
  the og's +0x120 latch (FUN_180041950, re-read first-hand) permanently binds
  CD01-key candidates to the 0x44c sizing and drops declined reads (no
  end-tag retry exists for latched candidates — the M-112 landing's
  "retries other sizing" claim refuted and corrected by erratum). The two
  M-112 decline tests re-pinned to the drop semantics (both RED pre-fix);
  the success path and the non-CD01 legs guarded green; Carve family
  156/156.

- **M-146 FIXED (MAJOR, orchestrator/ntfs)** — probe_family's NTFS gate family
  replaced with the og analyser's: the reserved-zero sweeps (frame-offset
  derived, with the finding's +0x0F annotation corrected — +0x0F and +0x15 are
  unswept), spc raw-nonzero (pow2 dropped), and FUN_180005470's record/index
  shape gates via the M-147 getter twins (index_block_size_from_dbr added).
  The misattributed comment re-truthed; bps range kept as registered
  hardening; two fixtures' unrealistic cib corrected. 5/7 RED pre-fix; suite
  1496/1496.

**Board after this landing:** 0 ACUTE · 0 SEVERE · 17 MAJOR · 13 MINOR · 0 INFO
active / 154 FIXED / 4 tracked-open. (Lane name S65 — the racing window took
the S64 seat with its own worklog open; both lanes ride the same mandate.) (M-086/M-087/M-089/N-107/N-127 remain
driver-repo-resident per OQ-S10-3's two-repo grant boundary.)

# SUMMARY.md — executive summary (the S62 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-62 S62 fix-lane delta (2026-10-29 — the user's MAJOR-first directive; core `98a5b139` at open, corpus `be0cfe9`; baseline suite 1460/1460)

**The NTFS open-ladder acceptance-band family closed (M-141 + M-145 + M-147
— one coherent pass over `validate_boot_sector`/`open()`, corpus re-verified
first-hand at every cited line before the fix):**

- **M-147 FIXED** — cpr==0/cib==0 are the og's four-site scan-path DEFAULTS
  (1024/4096; the analyser getters, NTFSSearchDLL, PartitionRecoveryD twice —
  each re-read at the exact lines). A single zeroed boot byte no longer
  refuses the volume; D-52's store-guard misread corrected in the same push.
- **M-141 FIXED** — the mft_lcn bound moved to the og's product form
  (`spc × mft_lcn < total_sectors`, NTFSLib :19294-19301) evaluated
  overflow-free (the wrap-passing refusal kept as registered hardening); the
  core-only mftmirr bound is GONE — a torn mirror LCN demotes to the
  report-only `mirror_invalid` flag (both mirror reads skipped + the Info
  diagnostic); the M-043 backup fallback gains an MFT-load-failure retry rung.
- **M-145 FIXED** — no 0x55AA gate anywhere on the NTFS open ladder (the og's
  NTFS path never reads +0x1FE; its FAT/exFAT paths do — NTFS-specific
  omission). Content gates are the proof on the primary AND the backup
  fallback. TornPrimaryBackupBootScans re-pinned (opens from the primary).
- RED proof: the stash-revert rebuild (fix stashed, the VolumeParams field
  kept so the tests compile) — 7/7 FAILED pre-fix, 108/108 NTFS+RawFixer
  GREEN post-fix, full suite **1465/1465**.

**Board after this landing: 0 ACUTE · 1 SEVERE [S-040] · 20 MAJOR · 22
MINOR · 2 INFO active / 139 FIXED / 4 tracked-open.** Next free IDs:
S-067+, M-148+, N-210+, I-048+, A-015+.

# SUMMARY.md — executive summary (the S52-B severe-first convergence lane on top; the prior lanes below, newest-first)

# SUMMARY.md — executive summary (the closing-fix lane on top; the prior lanes below, newest-first)

## Closing-fix lane delta (2026-10-29 — the INFO-first diff-to-fix execution lane per the user's directive "priority to informational first"; opened at core `98a5b139`, corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; absorbed the S60 MINOR-first window at the merge)

**The two INFORMATIONAL registers closed first (the user's priority
directive):** **I-045** landed as PARITY-DELTAS **D-141** (og-defect
non-adoption batch #3 — the raw ee_len multiply re-verified first-hand at
EXT4SearchDLL :4044-4048; the core stays kernel-correct) with the
anti-down-fix pin `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect`
(ee_len 0x8005 → one 5120-byte content extent, never the raw product);
**I-046** landed as PARITY-DELTAS **D-142** (the lostpart.dat/partinfo.dat
lifecycle + the stage-granular vs scan-position-granular resume delta,
registered against the M-042 architecture). Both preserved as deliberate
divergences in findings/FIXED/ — the audit trail keeps the full decode.

**Merge note (the S60 window absorbed):** the parallel S60 MINOR-first
lane pushed first with N-199 + N-192 (both FIXED, both RED→GREEN proven);
this lane's I-045/I-046 pair rides on top. Combined board after both
pushes: 46 active — 1 SEVERE · 23 MAJOR · 20 MINOR · 0 INFO — + 4
tracked-open; FIXED archive 140.

# SUMMARY.md — executive summary (the S60 MINOR-first execution lane (prior); the prior lanes below, newest-first)

# SUMMARY.md — executive summary (the S60-B MINOR-first execution lane on top of the closing-fix + S60 window lanes; the prior lanes below, newest-first)

## Session-60-B S60-B lane delta (2026-10-29 — the second MINOR-first diff-to-fix execution lane of the same user fix mandate ["priority to minor first", 20+ fixes]; opened at core `98a5b139`, corpus `be0cfe9`; every finding re-verified at the current tree before fixing; the racing lanes' landings absorbed per SA-9)

**N-195 convergence record:** this lane independently verified and fixed N-195 in the same window the S60 lane did (their 3fae0791 landing is the fix of record per SA-9 — both constants corrected identically, the pin test in the CrUi read_repo_file family). This lane's UNIQUE supplement rides its own push: **doc_truth_gate check 7** (`check_bench_ioctl_codes`, CI-wired at checks.yml:173) — every 0x6xxxxxxx-form CTL literal in bench/**/*.{ps1,cs,cpp} must byte-match a constant in include/cr/win/cr_rda.h, making the phantom-zero class a permanently-gated defect rather than a one-shot pinned one. RED exit-1 pre-fix (both violations named) → GREEN post-fix, first-hand.

**Fixes landed by this lane:**

- **N-196 FIXED (MINOR)** — the NTFS walk-entry record-geometry stance registered and pinned (option (a), the finding's recommendation): PARITY-DELTAS **D-143** carries the accept-direction superset (both og gate sites re-read first-hand — odd/>16-device-sector records refused 0x610016 at FUN_1800272b0 :31620-31621/:31599-31600; the core walks them, per-record validation carrying the safety). Pins: `NtfsVolume.OddSectorRecordWalkScans` (the 1536-byte odd-stride fixture surfaces its user file) + `OversizePositiveRecordOpensAndWalks` (the 17-sector stride); NtfsVolume family 73/73.

- **N-197 FIXED (MINOR)** — the spc shift-decode threshold registered: D-52's ADDENDUM documents the og's three internal forms (0xF0-nibble controller / >= 0x81 NTFSLib+analyser / raw PartitionRecoveryD — all four sites re-read first-hand) + the core's registered signed-byte choice + the 0x80 boundary nuance; the decode extracted to the pure wire helper `sectors_per_cluster_from_dbr` and pinned by `BootSpcShiftDecodeConventionPinned` (the eight-byte table). Behavior unchanged on every real-volume population.

- **N-198 FIXED (MINOR)** — the exFAT boot-gate numeric bounds registered as deliberate hardening: PARITY-DELTAS **D-144** (the og's nonzero-form conjunction re-read first-hand at :28180-28210; the no-real-formatter rationale; the > 2 TiB cluster_count reachability analysis) + `ExfatEngine.HardeningBoundsPinned` (the volume_length/fat_offset edges isolated on og-ACCEPTING ordering-consistent garbage, the cluster_count boundary pinned). ExfatEngine family 41/41; no behavior change.

- **N-202 FIXED (MINOR)** — the ext4 SB acceptance bands consolidated and registered: PARITY-DELTAS **D-145** (the four sub-items with the og sites re-read first-hand — the inode_size=0 128-fallback, the log_bs 7..20 og-defect band, the journal_inum unconditional-read stance, the desc_size 64BIT-form) + `Ext4Engine.SbBandsPinned` (the log_bs edge isolated from the adjacent fdb kernel rule; both desc_size combos). Ext4Engine family 44/44; no behavior change.

**Board at this push (filesystem truth):** 35 active — 1 SEVERE · 20 MAJOR · 14 MINOR · 0 INFO — + 4 tracked-open, FIXED archive at 145 files (the MAJOR count absorbs the parallel window's M-141/M-145/M-147 landings).

## Session-60 S60 lane delta (2026-10-29 — the MINOR-first diff-to-fix execution lane of the user's fix mandate ["priority to minor first", 20+ fixes]; opened at core `98a5b139` (baseline suite 1460/1460 green before the first change), corpus `be0cfe9`; the S55 filings' audited commit `9633d94f` — every finding re-verified at the current tree before fixing)

**Board at lane open (re-counted first-hand; corrected in the N-195 push — the lane's first block overstated MINOR at 24):** 48 active — 1 SEVERE · 23 MAJOR · 22 MINOR · 2 INFO — + 4 tracked-open, findings/FIXED/ at 136 files. N-107/N-127 (with M-086/M-087/M-089) are driver-repo-resident (OQ-S10-3's two-repo grant boundary) — not fixable in this lane, counted as active-open.

**Fixes landed this push:**

- **N-205 FIXED (MINOR)** — the recoverability rawCount population split: `AssistantStats::raw_tail_zero` now ticks the og's exact population (the forced observe-only tail-zero probe over grade-1 NTFS nodes with 0 < key < device-size, the 0x77fffff default size arm — FUN_1401c7d80 :331183-311 re-read first-hand, ported as grade_tree's pass 0); `raw_count` keeps the Origin::Carved census as the REGISTERED deliberate divergence (**D-146** [renumbered D-143->D-144->D-145->D-146 across the push-rebases: the racing windows' N-196/N-198/N-202 registrations keep the lower seats per SA-9], header mapping block); the M-134 resolution's phantom `raw_tail_zero` sentence repaired by addendum. Tests: Grading.RawTailZeroTicksForcedProbePopulation / RawTailZeroSizeArmBoundary / RawCountSemanticsRegistered (the registration leg RED until the D-row landed); Grading family 40/40.
- **N-195 FIXED (MINOR)** — the driver-verification bench's two phantom-zero extended-IOCTL constants righted to the contract codes (0x6002440 GET_INFO / 0x6002448 GET_VERSION, cr_rda.h CTL_CODE_RDA fn 0x910/0x912) — the bench can now actually exercise the GET_INFO/GET_VERSION paths it exists to verify. Pin test CrUi.BenchVerifyDriverIoctlCodesMatchContract (RED: both phantom literals present → GREEN: contract codes present, phantom absent); CrUi family 77/77.
- **N-199 FIXED (MINOR)** — probe_family's ReFS branch now gates on BOTH boot signatures ("ReFS"@+3 AND "FSRS"@+0x10, Partition FUN_180003900 :1747-1759 re-read first-hand): a forged OEM no longer dispatches the ReFS family. Test Refs.RecognitionRequiresFsrsMarker RED (forged-OEM fixture dispatched Refs) → GREEN; Refs family 41/41; lost/ldm/probe neighborhood 153/153.
- **N-192 FIXED (MINOR)** — FileProperties::clone() now copies `short_name` (the og's whole-block clone contract, part1 slot +0x08): the hard-link fan-out's cloned links keep the 8.3 name at the walk, through the session roundtrip (nflags bit 2), and at the UI preview clone. Test NtfsVolume.HardLinkFanOutClonesKeepShortName RED (cloned link's short_name empty, both axes) → GREEN; family 71/71.

**Board at this push:** 42 active — 1 SEVERE · 23 MAJOR · 18 MINOR · 0 INFO (the racing S60 window retired I-045/I-046 — D-141/D-142, convergent with this lane's plan) — + 4 tracked-open, FIXED archive at 142 files.

---

# SUMMARY.md — executive summary (the S52-B severe-first convergence lane (prior); the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S52-B severe-first convergence lane on top; the prior lanes below, newest-first)
## Session-58 S58 lane delta (2026-10-29 — the diff-to-fix execution lane; opened on `98a5b139`, corpus `be0cfe9`; the user's ordering override: INFORMATIONAL first, one finding one push)

**I-045 FIXED (ACKNOWLEDGED — the INFORMATIONAL-first pass's first landing):** the
ext4 unwritten-extent `ee_len` og-defect non-adoption register (batch #3, the
I-026/I-040/D-123 family) landed as PARITY-DELTAS **D-141** with the
anti-down-fix pin `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect`
(**RED-proven**: temporarily adopting the og's raw-multiply form —
EXT4SearchDLL FUN_180005670 :4044-4048, both multiply forms re-read first-hand —
fails the pin; the kernel-correct decode `raw_len > 0x8000 ? (raw_len & 0x7FFF)
: raw_len` passes). No production code change — the core's decode is the
registered deliberate non-adoption. findings/FIXED/I-045 + the ledger row +
modules/ext4.md ACKNOWLEDGED.

## Session-52-B S52-B convergence-lane delta (2026-10-29 — the severe-first diff-to-fix execution lane; opened on `659f0f6c`, absorbed the S45..S59 hot windows at the rebase; corpus `be0cfe9`)

**The lane's independent A-014 verification converged with FOUR prior landings**
(S45 `862d79ef` / S51 / S47 / S56 — all recorded as addenda I..III in
findings/FIXED/A-014; this lane's **addendum IV** rides its own push): the three
zero-on-fail `DosDateTimeToFileTime` slots re-read first-hand
(FATSearchDLL:32135-54), the byte-equivalent gate built, RED→GREEN proven by the
stashed-header rebuild (6/6 FAILED pre-fix). Three UNIQUE supplements survive:
(1) the exFAT twin's own three zero-on-fail slots quoted directly
(exFATSearchDLL:45174-94 — the per-component evidence beyond the FUN_180002f40
structural gate; with the citation corrected: the exFAT access call passes a REAL
time word, the time-0 form is the FAT twin's alone);
(2) `FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid` — the third
slot's independence pin (the last of the three slots unpinned by the merged
battery);
(3) the 0x5A46 decode erratum extended to the live fixture comment
(test_fat_volume.cpp:169 `2022-01-06` → `2025-02-06`; the finding's own
mistranslation class, repeated by addendum I's first push).

**The 625a1e59 committed-red window repair (the 4th M-133/N-157 collision
instance — convergent with the window's own repair lanes):** main was RED at the
merge head (5 failing tests + compile-red, executed proof) — (1) the ext4
duplicate `SizeExceedsBlocksSetsDamagedFlag` (two lanes' N-177 landings merged
with the same TEST name; the S53 copy's unique deleted-path coverage preserved
by this lane's re-title `SizeExceedsBlocksMarkerOnDeletedInodesToo`); (2) the
M-134 grading e1/e2 unused rows; (3) the S-058 re-pin's crossed fixtures
(a.doc/zzqx inverted against the test's own assertions — the engine gate verified
NOT-IN-table-skips); (4) the N-171 validated double-tick (every user-sign record
double-counted carved_found on committed main; the erratum addendum in
findings/FIXED/N-171); (5) the user-sign registry leaks — twelve window tests
installed signs without teardown; the decisive leak: the default set's lha sign
`2,2D6C68` ("-lh" at +2) re-matched the LZH wrong-dash fixture
(`Carve.LzhWrongDashByteRejects` red in every full run since the window's first
landing — reproduced at `ff5b96ad`; the window's "suite green" claims ran
sub-filters). The 82-commit window's repair lanes converged on the same fixes
for items 1-4 and eleven of the teardowns — their landing is the fix of record
per SA-9; THIS lane's push carries the three remaining teardowns
(CarveStats.UserSignFilesCountInCarvedFound / UserSigns.RegistrationRoundTrip-
ProceedsToScan / UserSigns.ShortFragmentMatchesInCore), the access-slot pin, and
the A-014/N-171 record addenda. Verification: full suite green (linux-release),
the touched families ASan-clean.

**Board at this push (filesystem truth, re-counted at the merged tree): 55
active — 0 ACUTE · 3 SEVERE · 24 MAJOR · 26 MINOR · 2 INFO — + 4 tracked-open,
FIXED archive at 128 files.** Remaining SEVERE: S-040 (image create absent),
S-063 (NTFS MFT bootstrap fallback absent), S-065 (ext4 indirect ladder
unbounded-freeze/OOM) — the next fix order for the continuation lanes.

# SUMMARY.md — executive summary (the S55 continuation audit lane on top of the S47..S58 parallel window; the prior lanes below, newest-first)



## Session-55 S55 lane delta (2026-10-29 — the SIXTH continuation audit lane of the user's second-session mandate: remove-the-fixed + go-deeper + mark-all-remaining, focus: sector numbers / scanning / accurate detection / calculation / no stuck or freeze / local database / state management; battery pinned at core `1fccb0a4`, filings verified at `9633d94f`, corpus `be0cfe9`)

**Method:** five parallel agents (2 liveness tiers + local-database deep dive +
scan/sector-math deep dive + grading deep dive) — context sheets at
`evidence/s55-lane-sheets/` — with the lead re-verifying every load-bearing new
claim first-hand before filing (the record builders, the W1 latch, the marker
restore, the QSDE formula, the rawCount arm, the ceil/floor pair, the
engine-layer buffer constants — all re-read at the decompile lines before the
finding was written).

**The liveness battery (convergent):** 69 active findings checked at the pin —
62 STILL-ACTIVE / 3 PARTIAL (N-154, N-170, N-185) / 2 FALSELY-ACTIVE
(N-142-rsf = the renumbered N-186, convergent with the S50 lane's own
retirement; I-035, retired-with-bookkeeping) / zero phantom-fixed. OQ-S42-1/2
resolved-in-substance (convergent re-decodes). The hot window's retirements
(S-055..S-061, M-119/M-126..M-134/M-137, the minors, I-035) all landed AFTER
this battery's pin — the S54/S50 batteries' verdicts converge on every
overlapping row. Full tables: `evidence/s55-liveness.md`.

**Five new findings filed (every one lead-verified first-hand):**

- **N-203 (MINOR)** — the resume round-trip drops the candidate's data-area
  extent: the og's lostpart.dat record replays the full CSearchedPartitionInfo
  image ({start,end,disk} dedup, EndSec data-area-anchored); the core's
  PartitionContext v3 carries no data-area fields, the load restores none, and
  revalidate_candidate discards the scratch that re-derives them — resumed
  candidates enter the dedup/containment/leader rules with [0,0) windows.
- **N-204 (MINOR)** — the og's engine-layer carve buffer geometry decoded for
  the first time (4 MiB blocks / 8 MiB double buffer / half-buffer look-back /
  sector-stride u32 probes, RAWSearchDLL:9271-9622 first-hand) —
  SCAN-ENGINE-DELTA F6's og-side cell was empty, leaving the registered core
  constants without a cited og baseline, and the engine-layer 4 MiB conflated
  with the device-layer 4 MiB.
- **N-205 (MINOR)** — the recoverability rawCount counts different populations
  (the og's forced deep-read probe ∧ FS-code size rule vs the core's
  Origin::Carved census) + the landed M-134 resolution's claimed
  `raw_tail_zero` AssistantStats member is phantom (grep zero hits at HEAD).
- **N-206 (MINOR)** — the device sector-count rounding: the og CEILS the last
  partial sector (geometry-EX path), the core FLOORS everywhere — truncated
  images' tails are structurally invisible to the core.
- **I-046 (INFO)** — the og's lostpart.dat/partinfo.dat lifecycle register:
  NOT a cross-session knowledge base — a per-process-generation cache (the
  first-search delete latch), per-hit mid-scan appends, interrupt
  progress-markers, and the scan-position-granular marker-restore resume path.

**OQ-S42-3 CLOSED** (retired to findings/FIXED/): the flag-setter origins +
Q5's vt+0xb8/vt+0x100 semantics decoded (0x23 is the ctor default in every
SearchDLL; 0xa8's producer has no writer in any decompiled module — the
likely-dead-branch residual registered). The grading deep-read decode
converges with the S48/S50/S52 lanes' products; this lane's unique additions
are the setter origins, the FS-code field semantics, and the 0xa8 observation.

**Verified-MATCH with new evidence:** the no-stuck-or-freeze clean bill (every
core walker carries explicit caps; the og's own Btrfs recursion has no visited
set — the core is the stricter side on every engine pair); the exFAT
chain-walk bounds parity; the recovery-side extent math u64-clean on both
sides; the byte-granular probe superset re-confirmed as already registered
(D-59c — the candidate verified covered, not re-filed); the QSDE score
formula `(m/t + 0.05) × 10` re-verified convergent with N-183's record.

**Board after this push (filesystem truth at filing): 60 active — 5 SEVERE ·
24 MAJOR · 30 MINOR · 1 INFO** (the S55 filings: +4 MINOR, +1 INFO; OQ-S42-3
retired from the 5 tracked-open → 4 tracked-open). Next free IDs: S-067+,
M-148+, N-207+, I-047+, A-015+.

**Recommended fix-order contribution (this lane's top):** N-205 step 1+3 (the
raw_tail_zero field + the resolution-truth repair — the phantom claim is the
audit-trail hazard), N-203 (the v4 tail — small, the resume-fidelity win),
N-206 (two one-line ceil divisions + the tail-read test), N-204 (the F6
og-side cell — documentation-only).

## Session-44 minors-lane delta, entry 3 (2026-10-29 — the N-167 found-size aggregate + the hot-window repair set)

**N-167 FIXED — the found-size aggregate.** The og's completion headline ("Files
Found: N (X GB)", dynamic-ground-truth--scan-phase-captured.md:125 — the corpus
capture pins 1,139,286 files / 147.12 GB on a 149.45 GB volume, the binary 1024^3
divisor GB-labeled per the Windows convention) is now renderable from core stats:
`ScanStats::bytes_found` stamped from `ResultTree::total_logical()` inside the
count refresh (signature widened const → mutable) so the counters and the size
ride the SAME observation points (boundary-stamped, staleness identical to
files_found — N-166's live cadence extends both together when it lands); the
session record grew to v3 (48 bytes, len-gated — v1/v2 load zeroed, the tree
re-stamps at the first refresh after resume); the CLI completion line renders
"N file(s) (X GB)" and the JSON summary carries bytes_found.
RED via the stashed-stamp rebuild (bytes_found 0 while the tree carried 1500);
GREEN: `ScanStats.BytesFoundAggregates` (the one-consistent-pair pin: the
aggregate IS tree.total_logical(); both the active and deleted populations
count) + `ScanStats.BytesFoundSessionRoundTrip`. Suite 1320/1320 (linux-release).

**Hot-window repair set (this entry's merge residue, fixed at the cause):** the
S-055-window re-landing re-introduced the N-171 double validated tick (collapsed
to one, third convergence recorded in the comment); the merge recomposed
Grading.ExtensionTableGatesDeepValidation's fixture rows against its assertions
(restored the M-134 landing's shape) and left orphaned variables (the dotless
pair's pin lives in its own test now); the S-055-window convergence resurrected
the S53 lane's duplicate Ext4Engine.SizeExceedsBlocksSetsDamagedFlag (dropped per
the superset rule). Suite verified green on the merged tree.

**Board after this push (filesystem truth): 20 active on this lane's targets —
0 ACUTE · 5 SEVERE · 19 MAJOR · 20 MINOR · 0 INFO — + 5 tracked-open / the FIXED
archive at 109 files.**
# SUMMARY.md — executive summary (the S47 INFO-first lane's block on top; the S44 minors lane + the prior lanes below, newest-first)


## Session-47 INFO-first lane delta (2026-10-29 — the user's INFORMATIONAL-first directive executed; core opened at `fa654673`/`d872452c`-claim, pushed through `cc81f2b6`; corpus pin `be0cfe9` throughout)

**The tier the directive named is retired: the open INFORMATIONAL board went 7 → 0**, and the
tracked-open set lost its two longest-standing S42 timestamp questions. Every disposition
below re-verified first-hand against the corpus and the live tree before any change (the
SOP's verify→reproduce→classify loop; the corpus wins, both directions):

- **I-036 FIXED (this lane, `55e88af0`)** — the user-sign default-load posture register: the
  load-site census re-executed (rrecov.cpp:1269-1310 the single CLI default site, cr_ui.cpp:4615
  the only ABI path, engine/orchestrator census-zero); D-104 + PARITY-OG-CORE-INVENTORY.md:84
  now read "CLI-app startup load; the library default is off (cr_ui_register_user_signs)".
- **I-040 CONVERGENT-MATCH (`1fccb0a4`; the S48 lane's D-123 landing wins per SA-9)** — this
  lane's independent decode of FUN_180007510 (:5455-5467, the unconditional +0x6C fold) and
  the 4-site `_gmtime32_s` census (:5495/:5506/:6262/:6273) converged; this lane's unique
  product: the fold-direction pin `Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect`
  (the regular-file 64-bit size the landing's dir-only pin leaves unpinned) +
  `Post2038FileTimeConversionStays64Bit`.
- **OQ-S42-1 RESOLVED-MATCH (this lane, `448a037e`)** — the NTFS $SI/$FN time-precedence seam
  decoded and closed: the og is $SI-first with $FN fallback (NTFSSearchDLL :5338-5353 — the
  FN defaults overwritten by the $SI getters behind the FUN_180044fb0 fit gate), the core's
  has_si rule verbatim; bonus seam: the og's third surfaced slot (props+0x35) carries the
  $SI's MFT-MODIFIED time (FUN_180044ff0 attr+0x30; the true A time is never surfaced) —
  parse_standard_information's mapping is EXACT parity, now pinned by
  `NtfsVolume.SiTimeSlotsMatchOgSurfacedSet` (distinct-time records, both directions) so no
  future cleanup swaps changed/accessed and silently diverges surfaced dates.
- **OQ-S42-2 RESOLVED-MATCH, CONVERGENT with the S48 audit lane (`1b1fbdc9`)** — the Btrfs
  nsec question closed by the exact one-read probe: FUN_180006ba0 :4435-4441 FOLDS
  `(nsec/100)+(sec+0x2b6109100)*10^7` — the core's unix_to_filetime_ns is the identical fold;
  not an I-006-family superset axis; this lane's unique product:
  `Btrfs.MtimeNsecFoldsLikeTheOg` (mtime nsec 123456789 → the exact folded FILETIME).
- **I-035/I-041..I-044** — retired by the parallel S48 mirror lane mid-flight; this lane's
  first-hand verification of all four I-035 corpus cites (the tif exclusion :726583, the
  rename formats :726699-706, the 36-KiB/2-MiB buffers :772543-553, the hex-prefix adapter
  registry :1008918+) recorded as corroboration in the trail.
- **Tracked-open sweep** — OQ-S10-1's by-name refusals verified current at HEAD
  (cr_thumbnail.cpp:85); OQ-S10-3's grant boundary re-affirmed (S48's fresh note, corroborated);
  OQ-S42-3 stays the S48 lane's active RESOLVED-IN-PART (no duplicate); OQ-S52-1 (the fresh
  ext2/3-vs-ext4 filing) gained this lane's evidence addendum: the analyser's own verdict masks
  decoded first-hand (incompat-within-0xffffffe9 ∧ ro_compat-within-0xfffffff8 → the family
  verdicts 5/6; the 0x2c0 EXTENTS|64BIT|FLEX_BG triple-gate → verdict 0x18; the analyser codes
  ≠ the DRWUI {4/5, 9} kind codes — the mapping is the remaining half for its owner).
- **M-115 CONVERGENT (`cc81f2b6`; the S47-MAJOR-first lane's `1d7a761b` wins per SA-9)** —
  this lane built the identical read_ok/promote-on-read/0x14-floor fix independently; unique
  supplements: the dims-seam refusal pin (`ThumbnailPromotesOnReadNotDecodeDimsRefusal` — the
  second DecodeFailed exit), the `PreviewPaneNeverPromotes` doctrine pin, the ABI-contract doc
  block. Family 11/11.
- **Main-green repairs the same windows owed** (the 09490fea precedent, all closed): the N-171
  hot-window double-increment deduped (the merge kept both landings' `validated` ticks — the
  suite's 1-vs-2/2-vs-4 caught it), the S-058 polarity test flipped to its own decode
  (out-of-table .zzqx enters the deep path; grading 25/25), the e1/e2 unused reds, the M-119
  `part_slice` retype (convergent with S53's N-188), Level::Warning→Warn ×2, and four
  format-drift sweeps (the 72a17b7c precedent — the Checks workflow's clang-format gate was
  red on every push in the S45..S58 window until the first sweep).

**Board contribution:** the INFO board 7→0 (1 owned fix + 1 convergence + corroboration), 2
OQ rows resolved with decode + pins (S42-1/S42-2), M-115's convergence supplements, OQ-S52-1's
evidence addendum, 7 new pin/proof tests landed across ext4/btrfs/ntfs/cr_ui, and the CI
gate restored to green on the drifted window. Suite at this lane's close: **1385/1385**.
## Session-44 minors-lane delta, entry 2 (2026-10-29 — the N-171 + N-186 carve-stats pair)

**N-171 FIXED — the user-sign carved_count undercount.** Verified before the change
(the SOP): the emission path re-read at the user-sign block (`sign_hits` the only tick)
and the assignment seam re-read (`stats.carved_found = cs.validated` — the grammar
layer's counter alone); the falsifiable claim executed RED first-hand: a DEADBEEF
user-sign fixture carved one 0xFFFE0000-tagged record with carved_found == 0, at the
carve level AND through a full orchestrator scan + session save/reload round-trip.
The landing per the fix plan's preferred step 1: the user-sign emission ticks
`validated` after `sign_hits` (the record IS validated — the sign matched; the og's
own stats map counts analyser detections the same way); no assignment change needed.
Tests: `CarveStats.UserSignFilesCountInCarvedFound` +
`CarveStats.UserSignCarvedFoundSessionRoundTrip` (both RED pre-fix).

**N-193 FIXED (NEW — the §8 discovery during N-171's verification; renumbered from the N-188 draft seat at the S56 collision repair).** The same
defect class proven on the STOCK raw-sign layer by an executed probe: a fixture whose
only content is the `sample` sign (`#!/bin/sh`, sign-table-only — no grammar) carved
exactly one record with `sign_hits=1 validated=0` — the og's own layer-1 sign-table
channel (139 formats) was invisible in the headline carved count the same way. Filed
as findings/MINOR/N-186 (now FIXED/) with the full evidence chain, then fixed in the
same session: the emission-site tick (post-dedup, on emission — matching the grammar
and user-sign layers; the candidate-site `sign_hits` semantics untouched), the
`CarveStats::validated` doc comment updated from "grammar-confirmed" to the
emitted-record truth (grammar + raw-sign + user-sign). Four stale pins re-pinned to
the honest count (GrammarValidationAndSpecificity 4→5, SkipsFsAllocatedRegions 3→4,
StructuredBoundaryPartialOverlap 4→5, OccupancyGateJumpsWholeInterval 1→2 — each
fixture's jfif raw-sign plant now counts; the pins' own comments had already
documented the raw-sign emission they were excluding from the counter). Test:
`CarveStats.StockRawSignFilesCountInCarvedFound` (RED pre-fix via the probe).
Suite 1236/1236 (linux-release).

**Board after this push (filesystem truth): 68 active — 0 ACUTE · 9 SEVERE ·
27 MAJOR · 29 MINOR · 3 INFO — + 6 tracked-open / the FIXED archive at 79 files.**
# SUMMARY.md — executive summary (the S50 continuation audit lane on top; the prior lanes below, newest-first)

## Session-50 S50 lane delta (2026-10-29 — the fourth continuation audit lane; the user's remove-the-fixed + go-deeper + mark-all-remaining directive; battery pinned at core `93461483` (= `0d7b1d9d` code-identical), the new filings verified at `34d6cf10`, corpus `be0cfe9`)

**Method:** four parallel liveness-battery batches (ACUTE+SEVERE / MAJOR x2 /
MINOR+INFO) + two probe agents (the tracked-open decompile probes; the 25-landing
parity sweep) + the part1 census agent; the lead re-verified every load-bearing
claim first-hand (both-side reads on all three divergences, the DXF/EXIF/D-118
secondaries, the clone() drop, the N-186 duplicate, the OQ-1/OQ-2 core sides)
before filing.

**The remove-the-fixed battery (74 checked at the pin):** 63 LIVE · 6 PARTIAL
(S-050 leg-2-at-filing; M-115's comment-cite leg via N-148; N-154 items 2+3-half;
N-160 items 4/5/7; N-170 item 4; N-185's owed erratum — all recorded in the
battery sheet) · **1 FIXED-ALREADY -> N-186 RETIRED** (the battery's single
falsely-active finding: the rsf variant device-descriptor duplicate of N-066's
`4d3f0409` landing; the false-grep-at-own-pin note recorded) · 5 CANNOT-VERIFY
(driver-repo M-086/M-087/M-089/N-107/N-127, tracked OQ-S10-3) · 4 REGISTER-STANDS.
Zero phantom-fixed; the post-pin fix-wave (A-014, S-055..S-058, S-061, M-115,
M-119, M-123, M-124, M-128..M-130, M-132, M-137, M-138, the minors fleet, the
INFO tier's retirement) CONVERGENT with the battery's pins — verdicts stand as
pinned truth per the S42-C precedent.

**Go-deeper results:**
- **OQ-S42-3 RESOLVED** — the grading deep-read sub-gates fully decoded
  (G0 early-outs; G1 the no-read grade-0 key-set gate; G2 the prev-sector
  16-zero-tail tentative-2; G3 the 32-zero first-sector grade-0; G4 the
  4-byte-0xFF grade-0; G5 the flag hook; G6 the max-ratchet commit + done
  bit; mode-1 = the ReFS 0->1 preview pass; the twin-key -2 shift confirmed
  for FS types 1/4 + the previous-twin->2 rule). **M-134's fix plan now has
  the decoded spec.** Evidence: decompiled-quotes/s50-grading-deep-read-subgates.md.
- **OQ-S42-1/OQ-S42-2 CONVERGENT** — this lane's independent probes produced
  the same decodes as the winning a75d8198/355472c4 landings (SI-preferred/
  FN-fallback; the nsec fold). Two benign-hardening nuances registered in the
  evidence (the og's 40-byte SI gate vs the core's 72; the core's pre-1601
  guard) — no findings, no code owed.
- **The 25-landing parity sweep (the regression watch):** 17 PARITY /
  5 LIGHT-CHECK-OK / **3 DIVERGENCE — all three in the fix lanes' newest
  carve code**, all mistranscription-class, all filed:
  **S-062** (SEVERE — the ONE walk's size-gate exits: the og publishes
  2x-device flat [the uVar5=uVar1 comma decode, Ghidra+retdec agree], the
  core keeps the last-valid end — **0 on a first-chunk decline** = silent
  missed recovery vs the reference), **M-139** (the HIK discriminator:
  og 00 00 01 **BC** [private_stream_2] vs core 0xBD — the M-098 landing's
  own title carries the transcription error), **M-140** (the CSL/CD01 latch:
  the og's +0x120 latch permanently binds CD01 candidates to the 0x44c
  sizing and drops declined reads; the core's end-tag fall-through carves
  them — the landed "retries other sizing" claim refuted).

**Mark-all-remaining results:** the **ifileproperties-part1 full census**
(51 MATCH / 13 DIVERGENT (15 of 17 non-MATCH trail-owned) / 4 ABSENT / 7 N-A;
the four module rows' under-cited flag DISCHARGED) surfaced **N-192**
(clone() drops short_name on the fan-out) + **I-047** (the +0x80 format/
icon-ID constant family register, acknowledged-at-filing). The sweep's
secondaries: **N-189** (the DXF canonical 8-vs-16-byte gate), **N-190**
(the EXIF 39-char prefix cap), **N-191** (the PARITY-DELTAS D-118 numbering
collision). The cosmetic residue (validate_mpegps's N-155-vs-N-156 comment
cite) recorded in the sweep sheet for the N-180-family doc-truth batch.

**Board at this push (filesystem truth at the pushed tree, the S52 lane's
M-134 landing absorbed mid-push — CONVERGENT with this lane's OQ-S42-3
probe; the same decode fed both lanes): 45 active — 5 SEVERE
(S-040/**S-050-disposition-landed**/**S-059-fixed**/**S-060-fixed**/**S-062**) · 19 MAJOR (M-139/M-140 new; M-134
FIXED at `290e9c16`) · 21 MINOR (N-189..N-192 new, N-186 retired) · 0 INFO
(the tier folded per convention) / 107 FIXED / 5 tracked-open (OQ-S42-3
resolved this lane, its remaining scope narrowed to the +0x72/+0x74
flag-setter origins; OQ-S52-1 new from the S52 lane).**
Next free IDs: S-063+, M-141+, N-193+, I-048+, A-015+.

**Recommended fix-order contribution (this lane's top):** S-062 first
(the ONE 0-extent class — one exit-value correction + 3 tests), then
M-139 (two byte constants + 2 tests), then M-140 (the fall-through
deletion + 3 tests) — all three are small, test-scoped, and riding the fix
lanes' own newest code; then N-191 (the registry renumber, bookkeeping
integrity for every future D-cite); then the four minors (N-186's sliver
exposure only if a consumer lands).

---

# SUMMARY.md — executive summary (the S54 continuation audit lane on top — the fifth continuation lane of the user's second-session mandate: the liveness battery + the focus deep dives [sector numbers / scanning / accurate detection / calculation & mathematics / no stuck or freeze / local database / state management] + the tracked-open probes + the remaining-surface census; the prior lanes below, newest-first)

## Session-54 S54 lane delta (2026-10-29 — opened at core `55e88af0`, renumbered from S52 at the rebase per SA-9 [the remote's S52 MAJOR-engine-math fix lane and S53 lane pushed first]; every filing lead-verified at `34d6cf10`, corpus `be0cfe9`)

**Lane scope:** read-only — remove-the-fixed (the liveness battery) + go-deeper
(the focus deep dives on the user's named parameters) + mark-all-remaining (the
remaining-surface census). No code changed; the delta vs origin/main is
`diff-to-fix/` + `worklog*` only.

**The liveness battery (directive 1):** three parallel agents over the 66
then-active findings + 6 tracked-open → 55 LIVE / 5 PARTIAL / 6 UNCERTAIN
(driver-repo per OQ-S10-3) / 1 falsely-active (**N-142→N-186** — the persistence
half had landed as the N-066 v4 tail before the finding's pin; scope-corrected by
addendum, not retired — the restored-summary mirror is the live remainder). The
hot window then retired 26 findings mid-verification (S-055..058, S-061,
M-118..M-137 band, the minors wave — the S51/S52/S46/S53/S56 fix lanes), all
absorbed convergently. Reconciliation: `evidence/s54-liveness-reconciliation.md`.

**The 20 filings (directive 2 — every one lead-verified first-hand: the lead
re-read every load-bearing corpus cite and every core defect site at the filing
pin, quote pack at `evidence/decompiled-quotes/s54-finding-quote-pack.md`):**

- **SEVERE ×4:** **S-066** (exfat — the lost-anchor base−12 re-base ladder absent:
  the og probes FAT media words at base/base−12 after a failed backup-boot gate
  and corrects the volume base; the core scans 12 sectors off), **S-063** (ntfs —
  the torn-$MFT bootstrap fallback to the $MFTMirr LCN; the core refuses and its
  own comment's premise is refuted), **S-064** (fat — flavor selection string-driven
  in the og vs bracket-driven in the core; string/bracket disagreement ⇒ 12-vs-16-bit
  entry decode ⇒ different recovered bytes), **S-065** (ext4 — the indirect-block
  ladder unbounded: ~10⁹ reads / ~17 GB extents per poison deleted inode; the og
  bounds by file size — the no-stuck-or-freeze focus hit).
- **MAJOR ×7:** **M-138** (ntfs cpr/cib zero: the og's four scan-path default sites
  0x400/0x1000 vs the core's rejection + D-52's premise correction), **M-145** (the
  core-only 55AA open gate), **M-146** (probe_family's NTFS gate family ≠ the
  analyser's — spc-POW2 core-only, the zero sweeps + record-shape gates missing,
  the comment's getter misattribution), **M-141** (the core-only mft/mftmirr LCN
  bounds), **M-142** (hfsplus — the VH validator gates the WRONG fields: the og
  gates freeBlocks/journalInfoBlock ≤ totalBlocks; the core gates fork clamps; two
  corpus docs carry the misread), **M-143** (ext4 — the journal inode-image harvest
  branch absent, the S-004 sibling surface), **M-144** (ext4 — the 8 MiB GDT cap
  refuses >16 TiB 64BIT volumes; MAX_GROUPS >128 TiB; the og unbounded).
- **MINOR ×9:** **N-187** (lost-scan NTFS acceptance weaker — no boot-pair/4-record/
  mirror-retry requirement), **N-188** (FAT media-word re-flavor absent — lands with
  S-064), **N-196** (the walk-entry record-geometry gate — core superset, register),
  **N-197** (the spc shift-threshold og-internal inconsistency), **N-198** (exFAT
  core-only numeric bounds — register), **N-199** (ReFS FSRS second signature),
  **N-193** (ext4 SB band deltas consolidated), **N-194** (the size display
  formatters absent), **N-195** (the bench phantom-zero IOCTLs — M-089's class
  surfaced core-side).
- **INFO ×1:** **I-045** (og-defect non-adoption register batch #3 — the ext4
  unwritten ee_len raw multiply).
- **Addenda ×3:** **M-126** (+the ETA budget seed `total/60MiB + 60s` decoded),
  **N-178** (+the og's engine-pause type gate), **N-186** (+the scope correction).
- **Errata ×4:** the CORPUS-ERRATA S54 rows (the HFS+ fork-clamp misread feeding
  M-142, the Btrfs mirror typo, the ZFS vdev_phys swap, the ring-stride figure).

**The deep-dive coverage (the focus battery):** six context sheets (three liveness
+ three deep dives: NTFS/FAT/exFAT/partition, ext4/HFS+/Btrfs/ZFS/ReFS/RAW,
orchestration/session/state/DB/math/hang) at `evidence/s54-*.md`; the state-
management and persistence axes corroborated the filed set (no new state/persistence
divergence beyond I-041/42/43, S-057, N-090, D-122 — the strongest confirmation
yet that the state surface is fully dispositioned); the sector-math and validator
axes produced the S/M-tier filings above; the hang-safety axis produced S-065 and
confirmed core-strictly-guarded on Btrfs/ZFS/HFS+ walks (the og's recursion is
its own risk there).

**Board at the rebase-absorbed close: 65 active — 0 ACUTE · 8 SEVERE · 26 MAJOR ·
30 MINOR · 1 INFO / 109 FIXED / 5 tracked-open** (the window landed S-060 and retired
N-186 convergently with this lane's battery, resolved OQ-S42-1/S42-2, and filed its
own S-062/M-139/M-140/N-189..N-192 twins — this lane's seven colliding filings
renumbered per SA-9 earlier-push-wins: my S-062→S-066, M-139→M-145, M-140→M-146,
N-189→N-196, N-190→N-197, N-191→N-198, N-192→N-199; single-pass simultaneous,
no cascade). Next free IDs: S-067+, M-147+, N-200+, I-046+, A-015+.

**Recommended fix order (the S54 contribution to the roadmap):** S-063 + S-062
first (the two damaged-volume salvage paths — the og's resilience posture, the
highest recovery-yield deltas), then S-064+N-188 as one FAT flavor pass, then
S-065 (the freeze), then the NTFS open-ladder family M-138/M-145/M-146/M-141 as
one coherent pass over validate_boot_sector + probe_family (they share the fix
surface and the D-52 correction), then M-142 (with its two doc corrections),
M-143/M-144 (ext4), then the minors. The addenda ride their findings.

# SUMMARY.md — executive summary (the S44 minors lane on top of the S47 MAJOR-first fix lane; the prior lanes below, newest-first — rebrand note: the remote's S47/S48..S50 window pushed first; this lane's N-161 rides the reconciliation per SA-9)

# SUMMARY.md — executive summary (the S57 parity-fix lane on top (rebranded S47->S52->S57 across the two hot-window arbitrations: the remote S47 and S52 lanes pushed first); the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S44 minors lane on top of the S47 MAJOR-first fix lane; the prior lanes below, newest-first — rebrand note: the remote's S47/S48..S50 window pushed first; this lane's N-161 rides the reconciliation per SA-9)
# SUMMARY.md — executive summary (the session-52 mid-board MAJOR lane on top; the prior lanes below, newest-first)





**S-058 CONVERGENT (SEVERE — session-57 lane; the S52 lane's ea405392 wins per
SA-9):** the grading extension-table gate polarity is corrected at the merged tree
— the twin/overlap/deep-validation machinery now runs for NOT-in-table extensions
exactly like the og (FUN_1401bebe0 = the inequality comparator; the control case
:585888 calibrates the direction; the "" runtime-set quirk skips dot-less names);
in-table extensions keep the engine-default grade. The s5 evidence erratum is
filed at its §5 item 4. The session-57 lane's independent RED battery (both
halves of the polarity pin) and convergence addendum ride in the finding file;
its redundant code/tests were dropped per the superset rule (the winning 23/23
battery incl. S-061's ReFS/Btrfs insert-time pins stands).

**Session-46 lane — wave 2 (2026-10-29):** the hot-window RED MAIN repaired before the landing (the N-177 convergence duplicate-TEST redefinition [build-breaking], the N-171 validated double-count, the 4d450da2 crossed grading assertions, and the S-055/S-056 battery's user-sign test-hygiene leak [the 190-sign default install never torn down — the og's weak "lha" '-lh' sign carved the wrong-dash LZH fixture; bisected first-hand; per-test teardowns]); **N-166 FIXED** (the live file-count telemetry: the tree-atomic attach counters + the emit-time monotonic refresh; deleted_found boundary-only [the post-attach truth]; the header contract re-truthed; TelemetryCountersMoveDuringCarveStage RED pre-fix; suite 1312 green).

**Session-46 lane landings (the other MINOR-first seat, 2026-10-29):** **N-158 FIXED** (`78b9b7aa` — the OfficeTagFromXML five-branch typing tail: XMIND/MMAP/EDDX/WFP/DRP, the disassembly-decoded XMIND probe + the no-stop chain walk + D-134), **N-159 FIXED** (`035508fa` — the failed-files report's tree_class axis + failed_bytes split + the three register repairs), **N-163 FIXED** (`142a7f40` — the NTFS runlist truncate-and-keep tolerance + the torn flag + PartialRisk grading + D-135), **N-165 FIXED** (`7f4f30d4` — the FAT out-of-volume stance registered D-136 + the stance pin). Suites green at each push; the full Resolutions ride the finding files + ledger rows.

## Session-44 minors-lane delta (2026-10-29 — the minor-first diff-fix execution lane per the user's operative directive; opened on `9807934` (the N-172/N-173 landing), corpus `be0cfe9`; suite at open 1203 green / 1161 pass · 42 win-skip · 0 fail)

**Board at open (this lane's view): 77 active — 1 ACUTE · 9 SEVERE · 28 MAJOR ·
32 MINOR · 7 INFO — + 6 tracked-open, FIXED archive at 64 files.** Fix order this lane:
MINOR → INFO → TRACKED-OPEN → MAJOR → SEVERE → ACUTE (the user's minor-first directive;
the ACUTE/SEVERE tiers execute after the minors board clears, per the session's ≥20-fix
mandate on this tier first). (At the push reconciliation the S47/S48..S50 window's
A-014 + M-124 + I-041..I-044 retirements landed first — earlier-push-wins; this lane's
N-161 lands on top of that board.)

**N-161 FIXED — the GPT entry-span underflow gate.** Verified before the change (the
SOP): the span math re-read at `emit_gpt_entries` (cr_table_parse.cpp:273-274 at the
finding's pin — the `last − first + 1` with no gate) and the `!= 0` zero-span gate it
slips past; the wrapped-count reproduction executed RED first-hand (the corrupt entry
[first=5, last=0, type GUID set] emitted as a second partition with sector_count
0xFFFFFFFFFFFFFFFC — the masquerade exactly as filed). The landing: the `first ≤ last`
gate after the used-test, the new `DiskTable::gpt_invalid_entries` counter, surfaced
through `ScanStats::gpt_invalid_entries` at the whole-disk target-set parse site; all
three emission paths (verified primary, salvaged primary, backup) share the lambda, so
the gate covers every side. The overflow boundary pinned on the other side of the seam:
first=0 / last=UINT64_MAX is NOT an inversion (counter stays 0; the +1 wraps to 0 and
the existing zero-span gate drops it). Tests: `Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted`
(RED pre-fix) + `Gpt.EntrySpanOverflowBoundary`. Suite 1205/1205 (linux-release).

**Board after this push (filesystem truth, post-reconciliation): 70 active — 0 ACUTE ·
9 SEVERE · 27 MAJOR · 31 MINOR · 3 INFO — + 6 tracked-open / the FIXED archive at 71 files.**

## Session-52 lane delta (2026-10-29 — the mid-board MAJOR engine-math set per the user's session directive "priority to major first"; core `f894549d` at open, corpus `be0cfe9`; the S45..S51 parallel windows absorbed at each pull)

**Method:** every finding independently re-verified before any change — the cited corpus
artifact re-read first-hand (decompiled line ranges, not the finding's excerpt), the
current code re-read at the live tree, RED-test-first, one finding one push.

**M-133 FIXED (the twin-key device-family shift set):** the shift narrowed from the FAT
family {FAT12, FAT16, FAT32, exFAT} to **{Fat16}** exactly — the og's set is kinds
{1, 4} (DRWUI :605752-756 + :605924-927, both re-read; the quick-factory dispatch
FUN_140577d40 :911110-911175 re-read to confirm the kind space). The finding's fix-plan
ext2/3 leg is NOT expressible in the core's kind space (the ext engine stamps the whole
family Ext4=9, unshifted in the og — naively mapping the leg would have shifted
true-ext4 volumes); the classification rule is undecoded in the corpus → **OQ-S52-1
filed** (the ExtFileSystemAnalyser decode as the next step). One finding correction
recorded: the "disjoint collision families" impact is structurally inert in the current
single-device ABI (production contexts carry one slot) — the fix repairs the multi-device
GradeContext surface the API exposes. Six tests: 3 RED pre-fix (the spurious FAT32/
exFAT/FAT12 twin pairs via a two-slot kDeviceUnit-spaced context), the FAT16 pin, the
EXT4/ZFS pins. Suite 1290/1290.

**Live counts after M-133:** ACUTE 0 · SEVERE 9 · MAJOR 26 · MINOR 32 · INFO 1 ·
tracked-open 7 (OQ-S52-1 new) = 75 active / 67 files in FIXED/.

# SUMMARY.md — executive summary (the S47 MAJOR-first fix lane on top; the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S42 diff-to-fix lane's convergence on top; the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S12 session-2 deep-dive lane on top — sector math / scan-progress math / local database / state management / detection-calculation; the prior lanes below, newest-first)

## Session-57 S57 fix-lane delta (2026-10-29 — the parity-fix lane over the standing board; core `fa654673` at open, corpus `be0cfe9`; the green baseline proven before the first change: linux-release build clean + full suite 1273/1273; rebranded S47-draft -> S52 -> S57 across the two hot-window arbitrations — the remote's S47..S56 lanes pushed first)

**Method:** the full SOP loop per finding — corpus cite re-read first-hand, current-tree
re-verification, classification, RED test, fix, GREEN proof, full-suite regression sweep,
trail update, push. One finding, one push. This block grows as the lane lands.

**Landings this lane:**

- **A-014 CONVERGENT (ACUTE — the FAT time-word gate).** This lane independently
  re-verified the corpus first-hand (FATSearchDLL:32131-32154, the three zero-on-fail
  DosDateTimeToFileTime slots; the exFAT twin exFATSearchDLL:45174-45194), confirmed the
  divergence unchanged at the pre-window HEAD, built the byte-equivalent gate
  `hour > 23 || minute > 59 || (time & 0x1F) > 29`, and proved it RED→GREEN with a
  3-test family BEFORE the window's landings became visible (the fabricated date+1day
  FileTime 134377920000000000 pinned pre-fix on the finding's own falsifier time word
  0xC000). Per SA-9 the remote's `862d79ef` (S45) wins the seat; this lane's duplicate
  family withdrawn at the merge; the unique residue riding the merged tree:
  `FatEngine.InvalidTimeWordAccessSlotStaysReal` (the access slot — always time word 0,
  FATSearchDLL :32151-32154 — keeps its real date while created/modified/changed zero
  out, and changed mirrors modified; neither remote family asserted the accessed slot)
  + convergence addendum #2 in findings/FIXED/A-014 (incl. the FUN_180002f40/f90
  exFAT-gate cite cross-check, verified first-hand at exFATSearchDLL:1756-1794).

**Landings this lane (unique fixes):**

- **N-206 FIXED (MINOR — the sector-count ceil).** The og's geometry-EX form
  (Device_decompiled.c:56536-56538, re-read first-hand: (L-1+ss)/ss overwriting
  the 0x74004 floor) lands across the geometry surface: Geometry::sector_count
  is the documented CEIL with the validity invariant as the ceil window;
  FlatImageDevice serves the driver-padded tail (zero-filled reads past EOF
  within the padded sector); both cr_transport.cpp divisions ceiled; the
  discovery tail bound + WindowScanner ride the ceiled count. RED→GREEN:
  Image.TruncatedFlatImageCeilsSectorCount / FlatGeometryValidOnOddSizedImage /
  LostDiscovery.TailBoundUsesCeiledSectorCount (all three RED pre-fix). Full
  suite 1447/1447. The test-design truth recorded in the Resolution: a
  55AA-gated VBR cannot anchor inside a partial tail on either side (the
  signature bytes are driver-pad zeros) - the bound math is the honest
  divergence surface.

- **N-208 FIXED (MINOR — the DXF canonical gate; renumbered from the born-collided N-189 seat at the CI-red repair — the remote lane's registry-sentinel finding holds N-189).** The og's two-qword 16-byte
  compare (RAWSearchDLL:42258-42275 re-read first-hand) lands in validate_dxf:
  "  0\r\nSECTION\r\n  " in full; the 8-byte-prefix collision class refused.
  RED→GREEN: Carve.DxfCanonicalRequiresFullSixteenBytePattern; the S-007 batch
  fixture re-truthed to the real covenant (the old 12-byte prefix-only shape
  was a synthetic the og itself refuses). Suite 1453/1453.

- **N-209 FIXED (MINOR — the EXIF prefix 39-char cap; renumbered from the born-collided N-190 seat at the CI-red repair — the remote lane's merge-red-build-gate finding holds N-190).** The og's strncpy(dst, src,
  0x27) sites re-read first-hand (RAWSearchDLL:40388-40389/:40483-40484); the tag
  string now resizes to 39 at capture - before BOTH the name prefix and the
  per-string counter key. RED proven by the stash-revert rebuild (the whole 45-char
  tag named the file pre-fix). Suite 1454/1454.

- **N-191 + N-207 FIXED (MINOR — the PARITY-DELTAS renumber batch).** N-191 as filed:
  M-125's row -> D-127, N-143 keeps D-118; the duplicate-D invariant landed as
  doc_truth_gate check 5 — and its first run surfaced NINE more born-collided seats
  (D-105/106/107/135/136/137 — the N-207 extension, filed per §8 and fixed in the
  same push: the later rows -> D-128/129/130/131/132/133/138/139/140, the stale
  storage.md D-75 cite re-truthed to D-72, the gate's ADDENDUM exclusion).
  Gate green: 135 unique D-rows.

**Board after this merge (filesystem truth, re-censed): 69 active — 0 ACUTE · 9 SEVERE ·
24 MAJOR · 29 MINOR · 0 INFO / 79 FIXED / 7 tracked-open** (the window's A-014 + M-124 +
M-123 + M-132 + M-133 + N-161 + N-158 + N-174 + I-036 + I-040..I-044 retirements + the
OQ-S42-1/S42-2 resolutions + the OQ-S52-1 filing absorbed; both sides' records kept).
Next free IDs: S-063+, M-139+, N-187+, I-046+, A-015+.

## Session-47 S47 lane delta (2026-10-29 — the MAJOR-first parity-FIX lane per the user's explicit directive; core `fa654673` at open → merged through the S48/S49/S50 windows; corpus `be0cfe9`)

**Lane scope:** the unclaimed MAJOR board (25 in-tree findings claimed at open; M-086/
M-087/M-089 stay tracked-OQ per OQ-S10-3 — their fix surfaces live in heicfast/rda-driver,
out of this session's two-repo grant, re-verified before claiming). One finding, one
verified fix, one push; every fix RED-test-first; every corpus cite re-read first-hand
before the change.

**Landings this lane so far:**

- **M-127 FIXED** (`0a46063c`) — the FAT/exFAT chain cycle guard adopts the og's
  range-tree shape (VisitedRuns, runs as intervals) in all three walkers: measured
  62.18 s → 1.55 s on the 100 × 65,536-cluster fixture (40×, the no-freeze
  directive's stall class closed); the caps and cancel checks untouched; the
  semantic pin proves identical stop points; the self-loop sub-case documented as
  unreachable through the og's whole-table validation. CycleGuardPerf RED pre-fix;
  suite 1296/1296.
- **M-126 FIXED** (`089b7cc8`) + the hot-window main-red repairs (`ae2f9206`): the
  og's ETA model landed as the query-layer eta_* helper family (FUN_1401af410's
  whole-run base, 0.2/0.8 rate EMA, x0.6 time-budget variant, "3m00s" placeholder,
  1 Hz countdown) + ScanTelemetrySnapshot.eta_* (the EMA state server-side) + the
  CLI remaining-time line; O39's wrong premise superseded. The repairs closed 4
  defects the parallel lanes' merged landings left: the N-171 double-tick
  (F-A3-6 class), the default-set registry leak (the LzhWrongDash order
  dependency), the N-177 same-name convergence (renamed), the S-058 fixture swap.
  5 Eta tests; suite 1280/1280.
- **M-134 FIXED** (`9455f50a`) — the og's deep-read grade validation: the two
  decoded sector-content rules (first-32-zero/first-4-0xFF → Low; the previous
  sector's last-16-zero → High), the GradeReader threaded at the completion pass
  (one bounded sector per not-in-table no-twin node), the graded one-shot bit,
  and the full-tree per-selection re-grade (twin lookup included). OQ-S42-3's
  sub-gates stay unadopted per the fix plan's allowance. 4 RED content tests
  (neutered-rule proof) + 2 pins; suites 1244/1244 + 71/71.
- **M-115 FIXED** (`1d7a761b`) — the thumbnail grade-promotion is now READ-gated,
  not decode-gated (Thumbnail::read_ok disclosed by the engine; the ABI promotes on
  the clean recovery read per DRWUI :422507-514; the og's 0x14 work-item floor at
  the ABI gate). The damaged-but-readable population grades 2 like the og; tiny
  decodables no longer over-promote. The TxtView/media triggers ride N-144/OQ-S10-1
  (the module row records the gaps). 2 RED-pre-fix tests (stashed-engine proof);
  suites 71/71 + 1228/1228.
- **M-119 FIXED** (`033b9cdb`) — the og's tolerant partition-boundary semantics at
  both halves: create clamps an over-long declared window to the device end (delta
  surfaced; N-046's start-past-device refusal kept), read implements the og ladder
  (parameter errors refuse; the straddle clamps + full-tail zero-fills + succeeds —
  the fix-time probe confirmed the ENGINE-side device policy first-hand at
  FUN_180009290). D-74's read-half superseded (dated addendum; its safety core and
  the under-zeroed-tail non-adoption kept). CLI warns-with-delta; both cr_ui opens
  clamp+log. 3 RED-pre-fix tests; suite 1216/1216 with zero engine fallout.
- **M-128 FIXED** (`2b0184f3`) + **M-129 FIXED** (`55b2fe32`) — the user-sign
  channel's semantics pair: the hex-case superset registered as D-125 (no code
  change — the og's %02X-only text compare is an og machinery defect leaving 11
  of its own shipped signs dead; the census corrected 12→11 first-hand) and the
  EUUserData loader's og-tolerance semantics landed (unknown members skipped,
  _stricmp dispatch, malformed signs skip-and-continue, over-cap still atomic
  per the S-039 doctrine, D-126 registered, the verbatim corpus file ships as
  the acceptance fixture). 5 RED-pre-fix tests via the stashed-parser proof;
  suite 1201/1201.
- **M-132 FIXED** (`d746a75b`) — the backup probe at device-last FIRST (the
  og's fixed-position architecture, FUN_180006e20:4567); the declared backup_lba
  demoted to a registered fallback (D-124a); the Case-A observable honestly
  recorded as already closed by M-123's position gate; the dead-LBA1 recovery
  stance KEPT + registered as D-124b with the pin test. Suite 1187/1187.
- **M-123 FIXED** (`10620f04`) — the og's GPT header verifier FUN_1800078d0 gate
  set landed in `load_gpt_side` (revision 0x10000, header_size exactly 92,
  reserved==0, the position pair mirrored by side, usable-range bounds, entries
  array outside the usable range — structural gates before the CRC, og order; the
  N-164 deliberate strictness kept; the salvage ladder unchanged by design). 9
  tests (8 RED pre-fix, incl. the truncated-image end-to-end); the 5 stale
  og-invalid fixtures re-shaped (entries outside the usable range — the fixtures
  were themselves headers the og would reject); suite 1178/1178.
- **M-124 FIXED** (`dcb123dd`) — the EBR link type 0x85 (Linux extended) joined the
  Extended arm of `mbr_type_to_kind`: whole 0x85-stamped chains no longer vanish, the
  bogus Other-volume at the link target is gone (Partition_decompiled.c:1006's `-0x7b`
  compare re-read first-hand — the same link path as 0x05/0x0F). Tests:
  Ebr.LinkType085WalksChain / Ebr.LinkType085SameBasisAs05 / Mbr.Extended085NotEmittedAsVolume
  (all RED pre-fix); suite 1164/1164.

**Board after this push (filesystem truth, post the parallel lanes' retirements
absorbed at the merges): 57 active — 0 ACUTE · 5 SEVERE · 17 MAJOR · 25 MINOR ·
0 INFO** (the parallel lanes' A-014 + I-041..I-044 retirements absorbed at the
merge; both sides' records kept). Next free IDs unchanged: S-062+, M-138+, N-186+, I-045+.

# SUMMARY.md — executive summary (the S46 MINOR-first parity-fix lane on top; the prior lanes below, newest-first)

## Session-46 lane delta (2026-10-29 — the user's MINOR-first directive; the S45 parallel lane owns ACUTE/SEVERE; audited core `659f0f6c`/`fa654673` at open, corpus `be0cfe9`)

**The lane's mandate:** the unclaimed MINOR set (30 findings claimed at
`659f0f6c` per the territory-claim entry; the driver-repo pair N-107/N-127 stays
tracked-OQ). One finding, one verified fix, one push; every corpus cite re-read
first-hand; every fix RED-test-first.

**Landings this lane (running):**

- **N-174 FIXED** — the exFAT `._` AppleDouble prefix sink (exFAT:45151/:45222
  re-read first-hand; the gate at the entry walk post-name-assembly, the parked-set
  oracle inherits it; `apple_doubles_sunk` counter; `ExfatEngine.AppleDoublePrefixSunk`
  RED→GREEN + `RegularDotFilesNotSunk` the over-blocking pin; exFAT battery 36/36).

- **N-175 FIXED** — the FAT NTRes 8.3 display-case fold (FATSearchDLL :44191-44216
  re-read first-hand; the four-way exact-value switch in short_name_to_u16 —
  0x00/0x08/0x10/0x18 fold, anything-else raw; `FatEngine.NtResCaseFoldFourWay` the
  5-path RED→GREEN pin + `NtResFoldLeavesLfnCaseUntouched`; the BitLocker NTRes=0
  fixture caught and re-pinned — the finding's own predicted bench-divergence class).

- **N-176 FIXED** — the FAT LFN illegal-char fallback (FATSearchDLL :32084-32099
  re-read first-hand; the 9-char gate `?*|\/:"<>` inside lfn_assemble beside the
  checksum — a match takes the same 8.3-fallback path; `LfnIllegalCharFallsBackTo83`
  the 9-iteration RED→GREEN pin + `LfnValidNameUnchanged`; the NUL early-return
  subtlety preserved in self-review).

- **N-177 FIXED** — the ext4 size-exceeds-blocks damaged marker (EXT4SearchDLL
  :6300-6303 + FUN_180007510 :5455-5467 re-read first-hand;
  NODE_FLAG_SIZE_EXCEEDS_BLOCKS = the og's own bit 0x200, the clean 64-bit compute
  on both emission paths, the D-135 non-adoption registration, the write_inode
  i_blocks_lo fixture repair; 3 tests incl. the session round-trip).

- **N-170 FIXED** — the pre-erratum EUUserData/GetRawUserInfoMap doc-truth batch
  (CORRECTIONS.md #9 = E-A/E-B re-read first-hand; 6 sites re-truthed — the
  cr_usersigns.h header's refuted "consults alongside raw_signs" claim, the
  kMaxEntries census math, the OQ-SA-3 resolution text, D-52's "seeds" grouping,
  D-60f's engine-reads list, SCAN-ENGINE-DELTA C4's "+ EUUserData MIME"; items 2/4
  already re-truthed by the parallel S-055/N-179 landings — FIXED-ALREADY, verified;
  acceptance grep zero).

- **N-180 FIXED** — the doc-truth staleness batch (each row verified beside its code
  cite: R-28 x2 + R-34 x2 in REFERENCE-API-GAP flipped to the landed thumbnail ABI +
  the shipped 100 MiB/15 s caps; ABI-CONTRACT §7i's max_read 200 MB; API-DELTA
  S3-W4a LANDED; AUDIT-PIPELINE's WRITER-1 row OPEN→MATCH; exfat.md B9's tz-offset
  phrasing re-truthed; the s5 polarity erratum already landed by S-058 — verified).

- **N-185 FIXED** — the b26017a7 record-truth erratum (the N-157 attribution line
  annotated with the executed refutation + the corrected repair sequence; the
  worklog's session-42 false-green claim dated per the 1233b4a0 precedent; the
  code was already green at HEAD — record-truth only).

- **N-183 FIXED** — the recoverability aggregate (the 20-format member-set
  population + the auto-High classes; the int((m/t+0.05)×10) verdict score over the
  gauge's own population; the QSDE re-mapping to the per-node non-existing count
  with the NTFS fixer-index DAT_141be5818 tick on its own counter; the ScanAnalysis
  append-only fields + the cr_ui copy; 3 RED→GREEN tests — two first-draft bugs
  caught by the pins).

- **N-184 FIXED** — the preview media metadata families (four bounded head-window
  extractors: AVI avih/LIST walk, Matroska EBML vint grammar, FLAC STREAMINFO, MP3
  ID3-skip + bitrate estimate; the M4A/MOV ftyp-branch pin; the UTF-16-BOM/MP3-sync
  collision caught by the two BOM text tests and excluded; 5 RED→GREEN tests, two
  real bugs caught by the pin during the RED pass; suite 1341/1299/42/0).

- **N-165 FIXED (S46/S57 convergent — the S57 stance stands)**

**Board (filesystem truth, running): 61 active — 0 ACUTE · 9 SEVERE · 27 MAJOR · 25 MINOR · 0 INFO / 77 FIXED files / 5 tracked-open.**

# SUMMARY.md — executive summary (the S42 diff-to-fix lane's convergence on top; the prior lanes below, newest-first)
# SUMMARY.md — executive summary (the S46 MINOR-first fix lane on top — the user's priority-to-minor directive; the prior lanes below, newest-first)

## Session-46 S46 lane delta (2026-10-29 — the MINOR-first fix lane; opened at core `659f0f6c` → territory claim `fa654673`, corpus `be0cfe9`; the parallel S45 lane holds the ACUTE/SEVERE side)

**Board at this lane's open (filesystem truth):** 77 active (1 ACUTE · 9 SEVERE ·
28 MAJOR · 32 MINOR · 7 INFO) / 64 FIXED files / 6 tracked-open. Baseline executed:
linux-release green, cr_tests 1203 (1161 pass / 42 env-skips / 0 fail),
cr_ui_tests 68/68 — **main was GREEN at the open** (the N-185 red-main long
repaired by the prior lanes).

**Landings this lane (running count):**

- **N-158 FIXED (MINOR, carve)** — the OfficeTagFromXML five-branch typing tail:
  the five typed ids land (FMT_XMIND/MMAP/EDDX/WFP/DRP, og 0x86/0x87/0x8c/0x8d/
  0x91). The finding's two open questions discharged by first-hand decode
  before landing: the XMIND content probe (disassembly 0x180045110-0x180045666
  — the 300-byte RAW window at record+30+strlen(name), the 16-byte
  `xmlns="urn:xmind` needle + the 14-byte .data prolog constant at RVA 0x93980,
  the already-0x86 return-1 re-stamp) and the chain-walk stop semantics
  (:18411-18450 — NO stop, last-match-wins; the N-115-era "map stop" comment
  was an erratum, corrected + pinned). The [Filter] coupling registered as
  PARITY-DELTAS D-134 (the conservative row-level disabled_formats gate; the
  og's IAnalyserFilter callback takes no id — undecodable). 6 tests, 3 groups
  RED pre-fix; suite 1209/1209; the module/matrix rows → MATCH.

- **N-159 FIXED (MINOR, writer+report)** — the failed-files report's
  tree-classification axis: the og's recovery-log truth (an HTML report,
  CSaveFailedFilesWithHtml, DRWUI :642669-642892) decoded and landed — the
  per-item `tree_class` wire axis (six-valued, root-role derived; the
  wire↔og-label↔core-role table in the schema doc) + the `failed_bytes`
  totals split (the +0x268 accumulator semantics; enqueue-time refusals
  excluded, truth rule 5) + the three register repairs (R-58 "text log" →
  the HTML-report decode; API-DELTA row 21 "NOT IN CORPUS" refuted; the
  writer-module D13 addendum) + additive-within-v1 per the schema doc's
  own contract (the finding's version-bump plan corrected). 4 tests
  RED-first; suite 1243/1243 (1201/42/0). P13/P11/P46/P68 → MATCH.

- **N-163 FIXED (MINOR, ntfs)** — the runlist malformed-input tolerance:
  the og's truncate-and-keep lands (FUN_1800347d0 re-read: every malformed
  branch a SUCCESS with the runs so far) — a torn $DATA fragment decodes
  its valid prefix with the `torn` flag, OR-merged at every parse site,
  and the walker grades the file PartialRisk (never silently pretending
  completeness); the zero-valid-runs class still rejects (the og's own
  pre-validator gate, FUN_18001d5f0 :23184); the clean buffer-end stop is
  distinguished from a tear (the old strict pin re-truthed); the
  sparse-first acceptance REGISTERED as PARITY-DELTAS D-135 (the deliberate
  superset over the og's validator rejection). 4 tests, the prefix-keep +
  the end-to-end PartialRisk grading RED pre-fix; suite 1280/1280
  (1238/42/0).
# SUMMARY.md — executive summary (the S12 session-2 deep-dive lane on top — sector math / scan-progress math / local database / state management / detection-calculation; the prior lanes below, newest-first)

## Session-12 S12 lane delta (2026-10-29 — the session-2 continuation audit per the user's second-session directive: remove-the-fixed + go-deeper, special focus on sector numbers / scanning / accurate detection / calculation / no-stuck-or-freeze / local database / state management; core `869da779` at open → rebased through the parallel windows to push at `f85def0f`, corpus `be0cfe9`)

**Method:** five parallel deep-dive lanes (A sector/LBA math · B scan strategy +
progress/ETA mathematics + liveness · C the EUUserData local-database family · D state
management · E detection-accuracy and calculation audit — context sheets at
`evidence/s12-lane-sheets/`) + three liveness batteries over the then-active board
(`evidence/s12-liveness/`). **Every load-bearing claim re-verified first-hand by the
lead before filing** — the two ACUTEs, all three SEVEREs, and the sector-math MAJORs
were re-read line-by-line on both sides (including catching and correcting one lane's
wrong-file line cites: the EBR/GPT decodes live in `ghidra-mo/Partition_decompiled.c`,
not the PartitionRecovery tree).

**The remove-the-fixed battery (49 checked at the pin):** 46 STILL-ACTIVE · 2 PARTIAL
(S-050 leg-2 already done by its own addendum; N-135's seven items fixed with a doc
residual) · **0 phantom-fixed**. The parallel windows' later retirements (S-048, M-103,
M-112, M-098, N-146, N-156, N-157, I-026..I-034) converged with the battery's pins.

**39 new findings filed (renumbered at the three-window reconciliation — earlier-push-
wins over the parallel S42 lanes' consumed seats M-118..122/N-154..160/I-037..039):**

- **A-013 (ACUTE)** — the CLI's `--resume`-without-`--out` opens the checkpoint sink
  `"wb"` on the resume target BEFORE the `"rb"` resume read (rrecov.cpp:1567/:1576/:1598):
  the save-on-cancel flow prints exactly this command as the recovery path — running it
  destroys the session. Silent data loss on the documented path.
- **A-014 (ACUTE)** — FAT time-word validation: the core validates month/day only;
  hour≥24/min≥60/2s-field≥30 flow into the epoch math and fabricate shifted plausible
  timestamps (cr_fat_wire.h:123-142) where the og zeroes the slot via
  `DosDateTimeToFileTime` failure (FAT:32135-54). Wrong metadata in output data, on the
  corrupt-dentry population recovery targets.
- **S-055 (FIXED session-51)** — the user-sign layer probed BEFORE the built-in raw-sign
  table, inverting the M-048/D-104 collision rule its own comment stated; the 8 overlap
  formats (amr/cap/class/cpl/flac/msf/pdb/wpl) flipped to device-end extents on the
  default path. Built-in-first restored (the user arm fires only when no built-in
  survived the disabled-format filter; the zero-skip guards the built-in arm only; the
  grammar-rejected no-reclaim rule decided + pinned via the bounded rejected_at set).
- **S-056 (FIXED session-51)** — `disabled_formats` gated the grammar and raw-sign
  layers but not the user-sign layer — D-104's "same format-toggle rules" registration
  claim was false in code. match_at's disabled_exts predicate now carries the og's
  removal-loop semantics (a disabled ext's signs never fire; sibling signs still can).
- **S-057 (FIXED session-51)** — the og auto-saved STOPPED scans (stop-organize
  "Scan stopped and found %1 files." → the shared completion handler
  FUN_140395e50 whose gates carry no stopped-check, DRWUI:1549399/:339076/:612572-87);
  the core never auto-saves a cancelled run and its comment mis-states the og.
- **M-123..M-132 (MAJOR ×10)** — GPT header validation laxity (no reserved/position/
  usable-range gates vs the og's `FUN_1800078d0`); EBR link type 0x85 missing (whole
  Linux-extended chains vanish); the og's piecewise progress remap + 99.99% cap decoded
  (O38 superseded); the og's rate-EMA ETA model decoded, no core surface (O39 premise
  superseded); the FAT/exFAT O(n²) cycle guard (stall-class, 100+ s on chain-heavy
  FAT32 — the no-freeze directive's hit); user-sign hex-case semantics (12 og-dead
  signs live in core); the EUUserData all-or-nothing parser vs the og's tolerant skip;
  the FAT dentry shape-validator family unported (histogram/size/chain gates — the og's
  main garbage-dentry FP guard); the CLI checkpoint truncate-in-place (the N-014 shape
  cr_ui already fixed); the GPT backup-probe asymmetry (both directions, unregistered).
- **N-161..N-178 (MINOR ×18)** — the span underflow, the FAT 32-bit wrap registration,
  the NTFS runlist salvage posture, the GPT strictness registration, the overflow-chain
  stance, live counters cadence, the found-size aggregate, the 2× bounds registration,
  the duplicate-asf semantics, the pre-erratum doc batch, the carved_found undercount,
  the deleted-first-char `'#'`/LFN-recovery register (OQ-T2 closed), the 2107 clamp,
  the exFAT AppleDouble sink, NTRes case folding, the LFN illegal-char fallback, the
  ext4 damaged flag, the pause CLI legs.
- **I-035/I-036/I-040/I-041/I-042/I-043 (INFO ×6)** — the CHK-chain decode addendum
  (S-050's evidence), the CLI-only default posture, the og-defect non-adoption batch #2
  (ext4 i_size_high, `_gmtime32_s`), and the three state-model registers
  (stopped-vs-finished, paused observability, state ownership).

**Verified-MATCH records (evidence in the lane sheets, no findings):** the S-052 LDM
span/stripe/RAID-5 math clean; EBR base/rel math; NTFS runlist codec on valid inputs;
exFAT 64-bit cluster math; ZFS DVA; the pause machinery RE-parity (PausableToken); the
auto-save gates/naming; the 7-input hang-trace table (no unregistered hang asymmetry —
the no-freeze directive's clean bill).

**Board after this push (filesystem truth, re-counted): 74 active — 2 ACUTE ·
5 SEVERE · 28 MAJOR · 33 MINOR · 6 INFO** (+ the parallel lanes' retirements absorbed
at the reconciliation). Next free IDs: S-058+, M-133+, N-179+, I-044+, A-015+.

**Recommended fix-order contribution (this lane's top):** A-013+M-131 together (one CkSink
adoption fixes both), then A-014 (one condition + 6 tests), then S-055+S-056+N-171 (one
carve patch), then S-057 (the stop-window auto-save factor), then M-124 (one case label
— whole-chain loss), then M-127 (the perf stall).

## Session-42-C S42-C lane delta (2026-10-29 — the THIRD continuation lane of the same user mandate, same pin `869da779`/`be0cfe9`; rebranded twice [the remote's S42 and S42-B lanes pushed first]; reconciled over THREE hot windows)

**The lane's own waves at the pin:** the T0 baseline EXECUTED (linux-release
build green; `cr_tests` 1134: 1091 pass / 42 env-skips / 1 FAILED — the red
main; `cr_ui_tests` 61/61), with the red-main reproduced by a SECOND method —
a clean `git worktree` build of the landing commit `67de3276` itself runs
`Carve.OoxmlRelsPathSplit` FAILED (-32), pinning the N-115 Resolution's
"Suite 1119/1119 (0 fail)" claim false at the pushed tree
(evidence/executions/s42c-red-main-reproduction.md). The ~22-landing parity
sweep: **all VERIFIED-PARITY at the pin — no code divergence**
(evidence/s42c-landing-parity.md; the S-054-precedent hunt's only catch was the
red-main bookkeeping defect). The never-walked surfaces: **the diskhealth
companion tree** (818 files, function-walked for the first time — 30
dispositions, the 5 never-dispositioned transport surfaces, the health module
verified CLEAN) and **the corpus's own worklog** (read for the first time — ~45
claimed RE surfaces cross-checked, zero gaps, zero contradictions; the
FoxitRestore / NAS-LM-hash / build-identity datums) + the core-side citation
census (232 files, zero behavior gaps) + the 49-active pin-time liveness
battery (CONVERGENT with the remote second lane's independent battery — two
lanes, one verdict: 49/49 STILL at the pin, zero falsely-active).

**The three-window reconciliation:** every draft seat collided with an
earlier-pushed remote lane and renumbered — **M-122** (the never-filed Stbio
§8 discovery of the `76e2d351` landing, retroactively filed + retired; the
phantom-SHA ledger rows 181-182 repaired `5103ecd8`→`76e2d351`; one live code
residual: the cr_thumbnail.cpp:163 "N-112"-cite comment), **I-038** (the
diskhealth register, acknowledged-retired), **I-039** (the corpus-worklog
register, acknowledged-retired), the red-main draft folded as the **N-157
corroboration addendum**. The trail repairs that survived all three windows
(the phantom-SHA rows, the N-032/N-112/N-113 dangling paths, the
`36c1d3e5`/`cd883754` pre-rebase phantoms in README/SUMMARY — the trail-SHA
guard re-run GREEN: 515 tokens resolving) were re-landed by this lane at the
final reconciliation.

**Board at this lane's close (filesystem truth, re-truthed at the rebase
over the window's M-112-CSL + N-146 retirements): 38 active (2 SEVERE
[S-040/S-050] · 20 MAJOR · 16 MINOR · 0 INFO) / 355 ledger rows /
3 tracked-open (OQ-S4-3; OQ-S10-1; OQ-S10-3).** This lane's three filings
all retire on filing — the board does not grow. Next free IDs: S-055+,
M-123+, N-161+, I-040+, A-013+.

**Handoff additions (riding the S42/S42-B lanes' standing orders):**
(a) M-122's one-line code residual (the cr_thumbnail.cpp:163 comment → the
next preview-touching format pass); (b) the docs queue (the RawFixer 31→33
one-worder at PARITY-OG-CORE-INVENTORY.md:157 + the AliyunConfig consumer
clause at :51 + the I-038 D-82 extension rows); (c) the
re-run-the-gate-after-every-rebase discipline (seconded — this lane's
worktree reproduction at `67de3276` is the strongest proof yet that a
pushed-red can hide behind a suite claim for a full day; the trail-SHA guard
must join ci-local.sh's gate set — it was RED at two consecutive mains).
# SUMMARY.md — executive summary (the S42 continuation lane on top; the prior lanes below, newest-first)

## Session-42 continuation delta (2026-10-29 — the remove-the-fixed liveness sweep + deeper comparison; core `869da779`, corpus `be0cfe9`)

**M-118 (MAJOR, NEW — the suite is RED at HEAD, by execution).** The certified
suite fails at `869da779`: `Carve.OoxmlRelsPathSplit` (landed with N-115 at
`67de3276`) expects the pre-S-049 lowercase ".docx/.xlsx/.pptx" tails while
`default_namer` (`cr_carve.cpp:1051-1090`) correctly emits the S-049 uppercase
contract. The landing rode a SKIPPED build+test CI job and a FAILED lint gate
(4 clang-format violations pinned: cr_signatures.h:325, cr_analysers.cpp:1370,
cr_registry.cpp:111, test_carve.cpp:7501); the follow-up bookkeeping commit's
own title admits the broken chain. Executed evidence:
`evidence/executions/s42-suite-red-at-head.md` (1134 ran / 1 failed / 85
Win-skipped on the Linux-release build; the tree BUILDS clean — the failure is
pure test-truth + gate-discipline). **Fix is mechanical** (uppercase rows +
clang-format -i on four files); every subsequent lane must certify against the
repaired baseline.

**N-154 (MINOR, NEW).** The S-039 cap-annotation set's one false claim:
`cr_usersigns.h:46` says the og's longest EUUserData ext is 4 ("epub","hqx")
— the stock DB ships `manifest` (8) + `class` (5) (111-key parse, 0 diffs).
The cap (8) is correct — zero behavior impact; comment-truth only.

**Retired: I-028 + I-032** (the sweep's only fixed findings; both verified
first-hand — the ABI-CONTRACT §7i thumbnail documentation landed with M-102;
S-053+N-141 landed `e10ec381`; ledger rows + matrix flips + in-file
Resolutions). **N-135 narrowed** to its open set (items 2/6/7 + two prose
phantom SHAs + the still-missing SHA-guard script). **The other 47 active
findings verified STILL-LIVE** (5 CANNOT-VERIFY here: the driver-repo set
M-086/M-087/M-089/N-107/N-127, tracked OQ-S10-2/3). The S40B lane's claim
set re-truthed: of its seven claimed minors only N-132 actually landed
(N-115/N-153 landed from other lanes; N-126/N-125/N-135/N-148/N-150 remain
open — the territory-claim-vs-landing gap recorded).

**The deeper comparison lanes (all clean — the config surface is genuinely
exhausted):** the 12-group config/data deep-diff (raw_signs.json 139/139 +
199/199 + 62/62 byte-exact; EUUserData 111/111 keys 190/190 signs 0 diffs;
Dict 228/228; SaveScan auto-naming exact; the five never-read config/ files
read fresh and dispositioned — config.ini's only_common is M-024's
CarveFlags, version.ini's scanext4/scanzfs is the registered I-016 policy
delta, nasconfig/themdark/shellexe are NAS/UI exclusions); the 0-1-citation
binary walks (archive.dll = libarchive, DRWUI enumeration-only, engine half =
M-066+S-032 with the open halves already M-116's; EUUnZip = SIDECAR-OOS with
both DRWUI consumer flows verified first-hand; PDFViewer = OOS-UI MFC+pdfium)
— zero new divergences, three register confirmations. I-033 carries the
row-88/row-65 refinements; CORPUS-ERRATA #8 records the remote_config
verbatim-payload framing decode.

**RECONCILED AT THE S42-D MERGE** (the parallel S42-main/B/C + S12 + S40-ML
windows all pushed while this lane audited the `869da779` pin — the hottest
window of the trail): the suite-red discovery was CONVERGENT (the remote
lanes' N-157/M-120 chain owns it and repaired it — b00118b5's both-sides
fold + 72a17b7c's format repair; this lane's M-118 filing is preserved as
**M-123**, retired-convergent, carrying the unique CI-API evidence [Build+test
SKIPPED + Lint FAILED at `67de3276`] and the four pinned format sites); the
I-028/I-032 retirements CONVERGENT (ledger rows merged, both Resolutions
in-file); N-135 closed by the remote's full fix (0eb5f82f + the SHA guard
`scripts/verify/verify_trail_shas.py` — this lane's "guard not landed" note
was true at the pin, superseded at the merge); this lane's usersigns finding
renumbered **N-161** (the remote's N-154 pushed first) and re-verified LIVE
at the merged tree (`cr_usersigns.h:46` still false — the parallel N-148
seven-item batch did not include this site). **Board at the S42-D close, post the S12 findings-wave absorption (filesystem
truth at `c466d732`): 75 active (2 ACUTE · 5 SEVERE · 28 MAJOR · 34 MINOR ·
6 INFO — the S12 wave's 39 filings dominate; this lane's one surviving active
filing among them is N-179, the usersigns comment-truth defect, renumbered
twice as both its earlier seats were consumed) / 360 resolved ledger rows /
3 tracked-open (OQ-S4-3, OQ-S10-1, OQ-S10-3).**

## Session-42 continuation lane (2026-10-29 — the user's second-session FOCUS BATTERY: file reconstruction / timestamp / recovery chances / partition recovery + API / preview / scan-mode semantics; audited core `b26017a7`, corpus `be0cfe9`)

**The lane executed the S42 plan's four waves re-scoped by the focus-battery
addendum** (`worklog/session-42-continuation-audit-prompt.addendum-focus-battery.md`).
Six parallel context agents (A–F, one per focus area) + lead first-hand verification
of every load-bearing claim (the SOP: no agent claim filed unverified — one agent
polarity transcription was REFUTED by the lead's own read and became the S-058
erratum chain instead).

**W1 (remove-the-fixed) discharged by git-ground**: `git diff 25e408b8..HEAD --
src/ include/` is EMPTY — the tree is byte-identical to the S12 live-verification
point; the four active SEVERE sites re-probed LIVE (S-040/S-048/S-050/S-054 all
still live); the board reconciled 53 active (4/23/17/9) + 19 FIXED + 3 tracked-open
entering the lane (`evidence/s42-liveness-sweep.md`).

**The filings — 16 findings + 3 tracked-open, every one focus-area named by the
directive, every one decompile-verified first-hand:**

- **S-058 (FIXED session-51 — recovery chances)**: the grading extension-table gate was
  **INVERTED**. The og runs the twin/overlap/deep-validation branch for extensions
  **NOT** in the 169-key set — `FUN_1401bebe0` is the *inequality* comparator
  (`:324923-39`), `FUN_140256d00` is `std::set::find` (`:413990-414019`), and the
  control case at `:585888` calibrates the direction; the core's
  `if (!in_table) continue` (`cr_grading.cpp:275`) twin-flags the exact inverse
  population, test-pinned to the wrong reading (`test_grading.cpp:166-187`), all
  inherited from the **s5 evidence's §5 polarity erratum**. The `""` quirk
  corroborates (both sides skip dot-less files — for opposite reasons).
- **S-059 (FIXED session-51 — file reconstruction)**: sparse/hole files were **left-shifted
  or refused** — `RunList::extents()` drops sparse runs, `Extent` has no logical
  offset, the writer's cumulative walk corrupts interior-hole layout and fails
  `ERR_TRUNCATED` with no output; the og always writes full logical size
  (zero chunks for holes, `:875661-673`/`:875062-75`). The mftwalk comments
  assert a zero-fill the writer cannot perform. No sparse-file writer test exists.
- **S-060 (SEVERE — timestamp)**: recovered files **lose their timestamps and DOS
  attributes** — the og restores both after every item (`:874686-718`, the
  binary's only `SetFileTime`, mask-gated per field; **the trail's N-009d row
  "og never calls SetFileTime" is REFUTED**); the core's DestIo has no metadata
  surface and zero restoration calls tree-wide.
- **S-061 (FIXED session-51 — recovery chances)**: **ReFS/Btrfs files never received the og's
  insert-time High grade** (`:605481-497` + the 5828/5830 gauge ticks); the core's
  early return (`cr_grading.cpp:217-219`) leaves both filesystems at Average.
- **M-133..M-137 (MAJOR)**: the twin-key device family ({FAT16, EXT2-family} vs
  the core's FAT-family); the deep-read validation + full-tree re-grade
  (`FUN_14038c470` decoded: first-sector zero/0xFF → Low, prev-sector tail-zero →
  High); the read-failure semantics batch (truncate+number+keep-partial vs
  zero-fill+delete); the recovery retry-bypass (the scan's Retry+Cache pipe unused
  at recovery); the lost-partition surfaced-shape batch (the "Lost Partition
  %d(%s)" template — **resolving M-104's deferred open question 1** — the
  10-target insertion cap, the per-candidate "Quick" registration, the 512-byte
  fallback, the Disk/Basic/Dynamic/MBR/GPT presentation rows).
- **N-180..N-185 (MINOR)**: the doc-truth staleness batch (six sites, incl. the
  s5 polarity erratum); RAW-first start ordering; the deep-range resolver; the
  gauge population/score-formula/QSDE mapping; preview media metadata families;
  and **N-185 — main is RED at the audited HEAD**: the S42 opening commit's
  OoxmlRelsPathSplit re-pin is self-defeating (expectations uppercased, the
  tail-fold left lowercase) and its "1092/1092 green" claim is **refuted by this
  lane's executed build+suite: 1136 ran / 1093 passed / 1 FAILED / 42 skipped**
  (`evidence/executions/s42-build-suite-execution.md`).
- **I-044 (INFO register)**: the scan-mode decode batch — the quick→deep
  auto-advance is engine-side and result-independent (the C1 rationale corrected),
  the settings gates are hardwired constants, the host-side 4 MiB chunk skip, the
  `FUN_140581500` creation-site citation erratum, and ZFS compiled out of the
  shipped og.
- **OQ-S42-1/2/3 (tracked-open)**: the NTFS $SI/$FN time precedence; the Btrfs
  nsec treatment; the grading deep-read sub-gates.

**Board at the lane's filings: 69 active (8 SEVERE · 28 MAJOR · 23 MINOR ·
10 INFO) / 6 tracked-open.** Evidence: the four decompile worksheets
(`evidence/decompiled-quotes/s42-{grading-gate-polarity,reconstruction-export-chain,lostpart-surface-batch,scanmode-state-machine}.md`)
+ the liveness discharge + the executed build log. Read-only guarantee held
(diff-to-fix/ + worklog only; the out-of-tree build dir removed).

**Handoff — the lane's recommended fix order:** (1) **N-185** (one-line test
repair — main is red, every subsequent lane's suite runs are polluted);
(2) **S-058** (the polarity flip + the test re-pin + the s5 erratum note — the
recovery-chances column is wrong on the whole population); (3) **S-059** (the
per-extent logical offset + hole extents — the data-loss refusal for sparse/
compressed files); (4) **S-060** (the DestIo metadata surface — every recovered
file's timestamps); (5) **S-061** (two lines in solo_rules); (6) M-136+M-135
(reads through the retry pipe, then the failure taxonomy); (7) M-133/M-134/M-137
(the grading sub-rules, the deep read, the lost-partition shape); then the standing
board order (S-054 re-landing first, per the S12 handoff).



<!-- S42 continuation reconciliation: the parallel S42/S12 fix lanes pushed first; this lane's block rides below theirs per earlier-push-wins; the lane's 16 filings renumbered at the push-time reconciliation (S-058..S-061 -> S-061..S-061, M-133..M-137 -> M-133..M-137, N-180..N-185 -> N-180..N-185, I-044 -> I-044). -->


## S42 fix-lane delta (2026-10-29 — the S-049 convergent-DU reconciliation; severe-first execution continues)

**Working tree:** core `4e74ee2f` at lane open, absorbed through the merge to
the session-40-severe lane's `4bd5ee7a` (their S-048/S-049/S-054-repair/M-121/
M-076/N-152 landings + the INFO-tier restorations all stood first — the
earlier-push-wins convention). Corpus `be0cfe9` throughout.

**S-049 — CONVERGENT-DU RECONCILED.** This lane landed S-049 independently
(local ee1c6760: the prefix/dims carriers, the session-wide counter map, the
EXIF/MP3/PDF/DOCX/ZIP extractors, the MP4/MOV template retirement); the
remote session-40-severe lane pushed theirs first (a richer extraction set:
MP4/MOV duration prefixes, PDF Title 25-cap/Creator-Producer, WAV INAM — and
the finding's own "57/57 plain MP4" claim corrected: the duration forms are
real). The merge keeps the UNION: their extraction layer + this lane's
**executed counter-key arbitration** (TWO key classes, first-hand from the
G/H session CSVs: the branch-1 per-format key shared by plain + derived
forms — the JPG plain+dims union 1..952 at 96.7% density, the MP4 families
interleaving one 1..8 counter, the date-alone DOCX rows 1..19 across three
dates; the branch-2 per-string key — every ZIP family 1-based dense, PDF
keyed by producer [TeX 1..453, GPL Ghostscript 9.56.1 1..158], DOCX keyed
by creator [jacko 1..6 across six dates], EXIF Adobe Photoshop 7.0 1..45),
the **DOCX creator+date composition** (jacko_/KIS2_/Wei Chen_/python-docx_
real forms), and the **session-wide CarveNameCounters** (one per
ScanController; the DRWUI map's session scope). The two DRWUI.exe data
constants extracted first-hand (L"%s" @0x14187f760, L"_" @0x14188017c) + the
caller-branch decode recorded in the finding's convergence addendum. Proof:
+3 tests (OgNamingDocxCreatorComposesWithCreated /
OgNamingCounterKeyTwoClasses / OgNamingSessionWideCounters); suite
**1229/1229** linux-release at the union; clang-format clean.

**Board at the union:** S-048 and S-049 both FIXED (the remote lane's
landings + this reconciliation); the SEVERE remainder: **S-040** (image
create) and **S-050** (the repair-line registration decision) — this lane's
next targets, severe-first per the mandate.
## Session-42 S42 continuation lane (2026-10-29 — the user's continuing second-session directive at the post-fix-window board; audited core `869da779` at open, absorbed through the parallel windows to `0ff6cc92`; corpus `be0cfe9`; REBASED over the hottest windows — the S12 filings, the S-048/S-054/S-049-convergence/M-117 landings, the INFO-tier restoration, N-156/N-157)

**The four-wave mandate discharged:**
- **W1 remove-the-fixed (verification form):** the board-truth verification over the fix window the S12 sweep pre-dated (S-049/S-051/S-052/S-053 + the N-wave) — every at-risk finding re-checked LIVE at the lane tree (the carve family, the whole-disk flow, the preview half-split findings); three stale-line cites noted; **no unretired fix found** (`evidence/s42-board-verification.md`).
- **W2 the HFS+ residual resolutions:** **N-038's field-name residual RESOLVED** — the record byte +0x29 IS `ownerFlags` (TN1150 byte-exact via the **ushort-units decode**: FUN_180009940's `short *` record parameter mixes ushort-offset and byte-offset reads; all TEN branch reads mapped — fileID@+8, three dates, FinderInfo 'prdf'@+0x30, dataFork.logicalSize@+0x58, totalBlocks@+0x64, resourceFork.logicalSize@+0xA8; corroborated by FUN_18000f7b0's own FileSize label); **the S-002 decmpfs-completion residual CLOSED core-superset** (the og's completion site wires SIZE only — no mode-3/4 content branch anywhere in the SearchDLL); **N-155 FILED** (MINOR: the og's zero-logical-size fallback `dataFork.totalBlocks << 12` absent in core — the journal-record population bites); the og's journal-copy name-length misalignment registered deliberate non-parity; I-014 sharpened (`evidence/decompiled-quotes/s42-hfs-forkfield-resolution.md`).
- **W3 the un-dispositioned config-layer batch:** **I-036** (INFO, acknowledged-at-filing, adopted by the S42-main reconcile) — the four surfaces with zero trail citations dispositioned: shellexe.ini (the Explorer-verb taglines), TreeType (the DRWUI view-layout gate — both readers decoded), themdark.ini (Qt theme tables), DRWLog.log (the empty channel); **I-034's census-claim corrected** ("no undispositioned product surface" was wrong by these four); SURFACE-INVENTORY §12 (`evidence/decompiled-quotes/s42-config-layer-dispositions.md`).
- **W4 the executed verification:** the from-scratch audit-side build at HEAD — cr_core/cr_ui 100% clean, **cr_ui_tests 63/63 executed green** (incl. the S-054 fence pair), cr_tests blocked by **committed conflict markers → M-120 FILED (MAJOR: main red, the 1fb7603b push's false "suite green" claim — the message-truth class at whole-commit scale)**; the parallel repair `b00118b5` landed in the same window and this lane **EXECUTED-VERIFIED the post-repair tree: the FULL SUITE 1104 PASSED / 85 Win-only SKIPPED / 0 FAILED** — M-120 retired convergent-FIXED (`evidence/executions/s42-build-at-head.md`).
- **W5 the DefaultLog real-run decode:** the three zero-coverage classes — **M-119's addendum** (the Ldq transport entry decoded first-hand: the three refuse classes + the clamp + the error-path zero-fill; **393 real-run Ldq refusals** corroborate that the og's scans over-read routinely), the bad-sector skip constants REGISTERED (2 MiB/×2-per-3/64 MiB/4 MiB/80 MiB gate vs the core's bisection — outcome-equivalent), the write-failure log observable registered (`evidence/decompiled-quotes/s42-defaultlog-ldq-decode.md`).

**Lane product:** 1 finding filed-and-convergently-resolved intra-session (M-120), 1 new MINOR active (N-155), 2 residuals resolved (N-038 field-name, S-002 completion), 1 register filed/adopted (I-036), 1 finding addendum (M-119 — sharpened acceptance), 2 registers decoded (bad-sector constants, write-failure observable), the matrix S42 block, the SURFACE-INVENTORY §12, hfsplus/storage module addenda. **Board at this lane's close (filesystem, pre-reconcile): 40 active (2 SEVERE / 22 MAJOR / 16 MINOR) / 35 FIXED files / 3 tracked-open.** Read-only guarantee: verified per-commit — nothing outside diff-to-fix/ + worklog* touched.


## S42 lane delta (2026-10-29 — the SECOND continuation audit lane per the user's second-session directive: remove-the-fixed, go-deeper, mark-all-remaining; audited core `869da779` at HEAD (a quiet window — no rebase needed), corpus `be0cfe9`; five parallel read lanes + the main-lane first-hand verification battery; evidence: `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md`)

**The remove-the-fixed wave (directive 1) — the full liveness battery at `869da779`:**
**49 of 49 then-active findings STILL, zero falsely-active, zero phantom-fixed.**
Every active finding's primary defect claim re-verified first-hand (grep/read per
finding: S-040's image_create absence; S-048's zero `Reconstructed` in src; S-050's
code absence AND its CAPABILITY-MATRIX §3 exclusion-list miss; the 21 MAJORs incl.
M-106's 64-MiB stand-in read and M-107's 12 stand-in sites counted; the 17 MINORs
grep-zero on their absences; N-148's UNDECODED comment read). The 5 driver-repo
findings (M-086/M-087/M-089/N-107/N-127) stand as PUSHED-FORWARD (the third repo
is out of the two-repo grant). The board's 64→49 movement was the fix lanes' own
proper retirements (S-049/S-051+M-104/S-052/S-053+N-141/S-054+M-117-suppress/
N-111..N-153/M-108..M-110/M-102/M-083/M-072/M-074/M-077/N-093 + the doc-truth
batch) — no silent retirements. The SUMMARY's own "64 active (6/22/28/8)" was the
stale arithmetic (never re-truthed after the retirement wave); the S42 filesystem
re-count is the truth below.

**This lane's unique product (all first-hand verified before filing):**
- **N-154** (MINOR) — OfficeTagFromXML's five-branch ZIP typing tail: the og stamps
  XMIND 0x86 / MMAP 0x87 / EDDX 0x8c / WFP 0x8d / DRP 0x91 (the WFP/DRP labels
  binary-resolved from .rdata at RVAs 0x6d940/0x67c88 this session); N-115's
  landing covered none; RawConfig's `[Filter]` keys couple to exactly these ids.
- **N-155** (MINOR) — the failed-files "recovery log" is an HTML report
  (`CSaveFailedFilesWithHtml`) whose rows carry a per-file tree-classification
  column (Lost/Reconstructed/Tags-Extracted/Existing/Deleted/custom); R-58's
  "text log" and API-DELTA row 21's "NOT IN CORPUS" refuted by the decode; the
  core's JSON report lacks the axis.
- **N-156** (MINOR) — the register-truth batch: I-034's census scope label + its
  PHANTOM script citations; N-129's OQ-S10-1↔S-10-2 swap; inventory rows 79/104;
  I-033 §12's overstated invariant; the M-117 ID-collision record.
- **I-035** (INFO) — the never-walked decode register: SearchResult.txt (the
  QA-gated full-result-list dump) + the DRWUI user-mode bad-sector skip policy
  (12-read trigger / 2→64 MiB adaptive chunk / 0x163 / 163,840-sector toast /
  1 GiB cap / cached bad-range short-circuit / file-backed bitmap — enriching
  D-38/FB-S01's og-side record) + the HTML fragment constants.
- **The M-117→M-118 renumber EXECUTED** (the SA-9 collision: the S41 lane's DBX
  filing took M-117 2.5 minutes AFTER 0f5d57fb renumbered the suppress_overlap
  finding into it — the retired row keeps the ID per earlier-push-wins; the DBX
  finding is M-118; the freedom-check class recorded in N-156 item 7).
- **N-150's S42 addendum** — the fix-wave-stale extension: 8 newly-underclaiming
  API-DELTA rows + 6 new defect sites (the phantom `CR_UI_ERR_FVE_LOCKED` macro;
  the dead duplicate `--label` handler with DIVERGENT semantics; ABI-CONTRACT §7i's
  32 MiB residue; testing.md's 1,014-vs-1,199 census; the 81/78-vs-83 export counts
  and 34/39-vs-42 code counts; bench/README's dead citations).
- **The S-048/S-050 evidence refresh** (the folder census's unique product):
  SaveScan/ = exactly 3 real .rsf sessions (magic verified first-hand; S-048's
  189,115-row per-volume tree shape stands); EUFileRepair/ = 87 files = 5 repair
  executables + 82 runtime DLLs, NO data assets (S-050's implement-vs-exclude
  decision still open). The og-file census's "no undispositioned product surface"
  verdict SURVIVES the S42 independent re-walk (1,470/1,470 files).
- **OQ-S4-3 unchanged (tracked-open)**: the transport-chunking half stays
  MEASURED; the TTFR half has no committed value (run 40747332987 in flight at the
  pin; the emission/extraction wiring verified); NOT closeable from committed
  evidence. bench/baseline.json still an empty placeholder.

**Board after this lane's push: 53 active (3 SEVERE · 21 MAJOR · 20 MINOR ·
9 INFO) / 328 ledger rows / 3 tracked-open (OQ-S4-3; OQ-S10-1; OQ-S10-3).**
Next free IDs: S-055+, M-119+, N-157+, I-036+, A-013+.

**Recommended fix-order contribution:** the S-048 tree-shape landing remains the
highest-value SEVERE (the S40-severe lane's claim stands — its decode is in hand);
N-154's entry-name half is mechanical on N-115's walk; N-155's classification axis
is a small schema-bump landing; N-156 + N-150's addendum compose into one doc-truth
sweep; the M-106/M-107 size-walk fleet remains the MAJOR tier's mechanical bulk.

## Session-42 main-lane reconciliation delta (2026-10-29 — the INFO-tier fix lane, opened at core `67de3276`/corpus `be0cfe9`, re-landed over the trail's hottest parallel window through `0bd04cf9`)

The lane opened on the user's "priority to informational first" directive and executed the
full INFORMATIONAL tier with the SOP verification loop (every corpus cite re-read first-hand;
every fix plan either executed or classified). The parallel window landed (convergently, in
flight): the OoxmlRelsSplit hygiene repairs (`b26017a7`/`d10e4aa2`/`1fb7603b` — the same red
this lane found at open), N-125, N-126, N-148, **S-048** (the Reconstructed carve tree), the
S-054 writer fix, the S12 filings (M-118/M-119/N-142/I-035), the S42 continuation lane's
HFS+ resolutions + **N-155** (their HFS+ finding), **I-036** (the config-layer register with
I-034's census-claim correction), and the INFO acknowledgment sweep (`e41cbb7e`, which
DELETED the INFO files per the purge convention). This lane's reconciliation (per the S-054
row's no-delete mandate + the user's put-fixed-in-fixed-directory directive):

- **The 8 INFO files RESTORED to findings/FIXED/** with enriched Resolutions (the doc-edit
  executions the acknowledge-only sweep did not carry): I-026 (+ the PREBUILT-ARTIFACTS
  mechanism refresh), I-027 (+ the five register-row folds), I-028 (convergent), **I-029
  (RECLASSIFIED INVALID — the corpus wins)**, I-031 (+ the D-59c/D-78/D-101/D-114 addenda +
  the A4 phrasing repair), I-032 (FIXED-ALREADY with reproduction), I-033 (+ the entries-2-9
  register applications), I-034 (+ the SURFACE-INVENTORY navigation index, with I-036's
  census-claim correction adopted).
- **I-029 INVALID — the four "unverifiable" codes ARE reference wire:** the EUDCPEPM
  FilterDispatch (`:1290-1360`) handles 0x800C2AC0 + the RRWR trio, encoded as NEGATIVE
  IoctlBase deltas (`-0x7ff3d540/-0x7ff3d4c2/-0x7ff3d4be/-0x7ff3d4c0`) the finding's grep
  could not match (the magic is `0x52455752`, not the "RRWR" string). The 19-code
  byte-for-byte claim now FIRST-HAND VERIFIED. Corpus-erratum class: the artifact's own
  15-code census missed the four filter-side codes.
- **N-156 (NEW, MINOR) — the RRWR direction decode:** the og maps 0x800C2B3E →
  FUN_000141f8 (IRP_MJ_READ, major 4) and 0x800C2B42 → FUN_00014300 (IRP_MJ_WRITE, major
  3); DRIVER-CONTRACT §4.4 named them inverted — both rows re-named with per-row cites +
  the 19-code provenance note; the driver-lane rda_filter.c verification is the finding's
  precise next step (outside the two-repo scope).
- **N-157 (NEW, MINOR, renumbered twice — N-154/N-155 consumed by the parallel window per
  earlier-push-wins):** the push-window test-truth defect class record — the N-115 landing
  asserted lowercase extensions against the S-049 contract landed 43s earlier; main arrived
  RED with a false green-suite claim. CONVERGENT with the parallel repairs (which stand).
- **Territory:** this lane claims next **S-040 + S-050** (the SEVERE pair) and the MAJOR
  tier; the 43-minors lane owns the MINOR set (their `db228b75` claim).
- The board re-truthed from the filesystem: **41 active (2 SEVERE · 23 MAJOR · 16 MINOR ·
  0 INFO) + 3 tracked-open; 346 resolved ledger rows (344 unique: 322 main-pattern + 22 OQ; the S-051/
  S-054 collision pairs documented in-row).** Suite green on the reconciled tree.






**N-179 FIXED — the S-039 cap-annotation false claim.** The EUUserData distribution
re-measured first-hand (111 keys; the >4 set exactly {"class","manifest"}; max = 8)
— the finding's measurement reproduced. The cr_usersigns.h provenance comment
re-truthed (kMaxExtLen = 8 stays: the exact fit, no headroom — now stated);
UserSigns.ManifestEightCharExtParses pins the boundary from both sides.

 the active MINOR set drops N-179
(the full board recount rides the session close); the FIXED archive at 28 files.


## Session-43 minors-lane delta (2026-10-29 — the minor-first diff-fix execution lane, per the user's operative directive; opened on `b00118b5` (post the S12/S42/S-054/N-125/INFO-tier windows), corpus `be0cfe9`)

**Hygiene repairs at open (three rounds, all merge-window classes):** (1) the b26017a7
half-applied OoxmlRelsPathSplit re-pin (one side case-folded — deterministically red);
(2) the 59bd5208 push's committed conflict-marker residue in test_carve.cpp (main failed
-Werror outright at db228b75 — the "1096/1096 green" claim in that commit's message
cannot have executed against the tree); both repaired, the both-sides case-folded form
kept, suite verified green from scratch (1211/1211 at the repair tree).

**N-152 FIXED — the DXF 999-comment entry key.** First-hand re-verification before the
change (the SOP): the u16 registry binding re-read (:2722-2733 — the 0x18-byte
CDXFAnalyser node, idx 0x19, under key 0x3939) and the probe FUN_1800376f0 re-read in
full (:42240-42290 — the canonical 16-byte "  0\r\nSECTION\r\n  " pattern at 0; the
comment branch: "999\n" → the 14-byte LF form "  0\nSECTION\n  " (_Size=0xe),
"999\r\n" → the 16-byte CRLF form (_Size=0x10); the NUL-guarded forward scan). The
landing: FMT_DXF_C999 on the SWF/CWS dual-key precedent, validate_dxf's comment branch
(4 KiB bounded scan — the posture registered in the Resolution), suppress_overlap on
both DXF rows (the N-151/N-153 same-document rule). One test re-pinned during
verification — the finding's own "99x" near-miss plan expected zero carves, but the
og's u32 map salvages the interior section header on that fixture (both maps dispatch
identically); the pin now asserts the og-faithful headless salvage. Fail-before 3/4
RED (stashed-validator rebuild); suite 1215/1215 (linux-release). The M-117 matrix
cell's residue note updated: only the DBX triple remains of the 57-key re-diff.

**Board after this push (filesystem truth):** 41 active (3 SEVERE · 23 MAJOR ·
15 MINOR · 0 INFO) / 3 tracked-open / the FIXED archive at 24 files.

**N-149 FIXED — the carve filter's always-on set.** Corpus re-verified first-hand (the
removal loop :4408-4435's mask test re-read; the three analyser ids re-pinned at their
registry builders :2337/:2304-2310/:2243; the shipped [Filter] re-read — 34 keys, none
of the three). The landing: `carve_format_always_on()` maps the og's three analysers
onto the core's 16-row family set (OLE; the RIFF per-form rows; the ZIP family + the
in-analyser typed rows), the disable surface refuses to switch them off, the raw-sign
list layer stays disableable (outside the mask range). The token-split pin discovered
during verification: "doc" addresses both the exempt OLE row and the disableable legacy
LOWDOC rows — row-level exemption, exactly the og's mask scope.
`CarveEngine.FilterExemptSetAlwaysOn` RED pre-fix → green; suite 1223/1223.

**Board after this push (filesystem truth):** 40 active (3 SEVERE · 23 MAJOR ·
14 MINOR · 0 INFO) / 3 tracked-open / the FIXED archive at 25 files.

**N-146 FIXED — the FH11 zero-end stand-in re-homed and pinned.** Corpus re-verified
first-hand (FUN_180029f30 :32612-32633: the constant predicate stamps the zero-end
FileInfo id 0x1d with no size walk). The site joins M-107's enumeration as the twelfth
member (the zero-end family class, the D-101 decline) — the registration gap was the
finding's actual defect; the 32 MiB salvage cap stays and is pinned
(`Carve.Fh11BoundedExtentPinned`, mutation-proven fail-before).

**Board after this push (filesystem truth):** 39 active (3 SEVERE · 23 MAJOR ·
13 MINOR · 0 INFO) / 3 tracked-open / the FIXED archive at 26 files.



## Session-42 fix-lane delta (2026-10-29 — the SEVERE-first diff-fix execution continuing the S40-severe territory; opened on `67de3276`, re-merged over the hottest parallel window of the trail through `a3741812`; corpus `be0cfe9` throughout)

**S-054 bookkeeping completed (FIXED-ALREADY, independently verified).** The
N-013-regression SEVERE (the multi-extent destination arming clobbered by the
unrelated M-036 commit `1ebd1bcc`) was fixed by the parallel lane at
`a3741812` (the 3b5397a1 shape restored verbatim at both production sites +
the `DestArming.ProductionWiringCallsSpansApi`/`HeaderContractStillClaimsAllSpans`
wiring fence) but the trail was left incomplete — the finding sat in SEVERE/
with its Resolution written. This lane verified the fix first-hand (wiring live
at `rrecov.cpp:2112-2132` + `cr_ui.cpp:4056-4063`, fence pair green,
`Report.MultiExtentSpannedVolumeRefused` green, suite 1094/1094) and completed
the retirement: file → findings/FIXED/, ledger row landed, matrix cells
F62/B8/R43/Q65 → MATCH, board SEVERE 4 → 3.

**Hygiene repairs at the lane open (main was RED twice in the merge window):**
(1) the N-115 landing's `Carve.OoxmlRelsPathSplit` expectation was pushed
un-re-pinned against the S-049 uppercase-ext contract — re-pinned; (2) the
N-126 lane's case-insensitive comparator and this lane's uppercase table were
spliced into a broken pair by the rebase — the earlier-push comparator kept
with the lowercase table. Suite 1094/1094 green on the repaired tree (Linux
gcc Release, from-scratch build).

**Board after this push (filesystem truth): 51 active (3 SEVERE · 23 MAJOR ·
16 MINOR · 9 INFO) / 21 files in FIXED/ / 332 ledger rows / 3 tracked-open.**
SEVERE tier remaining: S-048 (the carve tree-shape), S-050 (the file-repair
line), S-040 (the image-create producer).

## Session-40 severe-lane delta (2026-10-29 — the SEVERE-first diff-fix execution lane, parallel to the session-40 minor-first lane; opening on `461825d8`/corpus `be0cfe9`, re-merged over the hot parallel window through `7320a2f6`)

**S-049 FIXED — the og's universal carved-file naming contract.** Independently
re-verified against the corpus FIRST-HAND (the SOP): FUN_140c44770 read directly
(:1878330-81 — `%s_%06d`/`%06d` + the >99999 fallbacks, the 0xff buffer), both
caller sites traced (:1877600/:1877724 — the map-keyed counter + the
parenthesized-prefix / `%dx%d`-dims branches), DAT_14187f760 verified `L"%s"`
byte-exact, and THE THREE REAL OG SESSIONS re-mined with the core's own
compare_og_rsf.py (~2M rows). **The naming model extends the finding's own
decode**: the counter is PER-FORMAT (not global — JPG 1..3938 continuous across
its plain/dims/Model/Software prefix variants; GZ 1..27,694; PNG 1..17,580), the
default prefix is the format's own UPPERCASE EXTENSION (GZ_000001.GZ — the
display name "JPEG" never appears), and the content families are richer: MP4/MOV
carry the mvhd DURATION prefix (MP4_14s/MP4_08m07s/MOV_10s — 57/57 real MP4/MOV
rows), PDF prefixes by Title (25-char read cap — TWO independent truncation pins:
`LaTeX with hyperref packa`, ` TeX output 1998.06.28_17`) else Creator/Producer +
CreationDate (the plain D:YYYYMMDDHHMMSS form only; matplotlib's dashed form
never yields a date), DOCX by docProps created, WAV by LIST/INFO INAM
(BIRD/RAIN/CAR), ZIP by the first central-directory entry with path separators
mapped (dots PRESERVED — the finding's dot-translation example corrected by the
classes.dex/027.jpg families). **One supersession**: N-063's MP4/MOV "Lost Name
File (...)_EaseUS" template landing is superseded on the carve-tree application
(57/57 real rows are counter-named; the DRWUI tree-naming layer names every
unnamed carve record through the %s_%06d contract and the engine template does
not survive it) — PARITY-DELTAS D-76 rewritten with the full record; the
File%06u deliberate default RETIRED (pinned absent). Fail-before: 39 carve tests
asserting the superseded names; pass-after: 13 S-049 pin tests + the re-pinned
family set; suite 1185/1185 (linux-release, the merged tree over the parallel
S-051/S-052/M-104/N-133 landings); clang-format clean (the parallel M-108
landing's dirt repaired in the same push). Open nuances recorded in the
Resolution (JPEG Model-vs-Software precedence; PDF Creator-vs-Producer order;
the TTF SHIVxx family — 11/59 rows, a naive name-table read contradicts the 48
plain rows, the read-bound condition undecoded). Next: S-048 (the same
real-session evidence's tree-shape half), S-040/S-050.

**S-048 FIXED (the same lane, second push) — the Reconstructed per-volume carve tree.** The
real-session tree shape landed in full (all first-hand decoded this lane): the
`Reconstructed` root, the 7 EMPTY category skeleton folders (Pictures/Documents/Archives/
Videos/Audio/Emails/Others - every real session carries exactly these; a newly-decoded
surface beyond the finding's own fix plan), the LAZY per-volume group children (the
finding's "at scan-start" plan corrected by the G session: a carves-free volume gets NO
child), the empty per-ext folders, and every file directly under the volume child. The
volume attribution is the tree position (the og's architecture - no per-node field); the
query axis is the path-prefix filter. New surfaces: CarveFlags::volume_groups/
volume_group_name (the carve-layer grouping contract), ResultTree::add_folder (the
intermediate-folder API), the marker-2 session record (child folders; roots stay
marker=1 - old readers unchanged), ScanConfig::volume_letter (the CLI --volume letter +
the ABI open-volume letter). The whole-disk scans name their groups through the SAME
M-104 tier machinery the structured volume roots use (one display name per volume, both
trees - the og's own shape). Gap positions (outside every table volume) attach to the
Reconstructed root directly - the unattributable form, never a wrong-volume attribution
(no real-session sample; the honest fallback recorded in the Resolution). 8-test proof
set; suite 1199/1199; clang-format clean. Board: S-048 retired to findings/FIXED/ (2
SEVERE remain on the active board: S-040, S-050).
# SUMMARY.md — executive summary (the S42 fix lane on top; the prior lanes below, newest-first)


## S42 fix-lane delta (2026-10-29 — the diff-to-fix execution lane per the user's directive: priority to MAJOR first, verify-then-fix one finding at a time, ≥20 fixes, fixed findings land in findings/FIXED/)

**Working from: core `4e74ee2f` at lane open (baseline suite 1119/1119 green —
1077 passed / 42 platform skips, 0 failed), corpus `be0cfe9` throughout (read-only,
first-hand re-verification of every corpus cite before any change).**

**Landings this lane (each: RED pre-fix proven by the stashed-engine rebuild,
GREEN post-fix, full suite green, clang-format 18.1.8 clean, one finding = one
commit = one push):**

- **M-117 → M-118 → **M-121** (MAJOR — the never-filed Outlook Express Dbx analyser) →
  **FIXED, CONVERGENT-CORRECTED.** Two parallel fix lanes landed the same finding and
  the reconciliation caught BOTH lanes' byte errors against the disassembly (the
  ground-truth discipline working as designed): the parallel lane's JMF6 version word
  `03 00 10 00` was RIGHT (disasm 0x180016766/0x18001676f `movl $0x36464d4a` +
  `movl $0x00100003`; this lane's first `03 00 01 00` was a literal-expansion error —
  now the hostile fixture in Carve.DbxJmf6WrongVersionWordRejected), and its marked
  pattern byte 6 `0xDE` was WRONG (the dword 0x6deef0f7 is F7 F0 **EE** 6D on disk,
  disasm 0x18001698b — its marked form never matched a real file; corrected). The
  bisection decode pinned by raw disassembly (0x180042bb0-0x180042c07: `sar $1` +
  `lea 0x1(%rsi),%eax`): FUN_180042b30 returns the non-zero prefix ROUNDED UP TO
  EVEN (prefix 1→2, 3→4 … 511→512; 0 on all-zero) — the walk's extent lands on even
  bounds. The finding renumbers to **M-118** per SA-9 (the M-117 ID was
  triple-consumed: the session-40 suppress-overlap ledger row 442 keeps it; the parallel
  lostpart finding then consumed the M-118 this renumber first took; the stub stays at
  findings/FIXED/M-117-*, the A-005/A-006 pair precedent). Bounds re-truthed:
  min 0x80/0x10 (the size field's read bound / the pattern's whole gate), max
  0xBEBC200 (the og engine default — the 0xFFFFFFFF cap surface would run the marked
  walk 4 GiB per candidate). 9/9 tests GREEN. Original record of the parallel lane's
  landing follows; the merged Resolution lives in findings/FIXED/M-118-*.
  The full grammar landed beyond the finding's own evidence: `FUN_180042b30`
  decoded as the non-zero-prefix binary search (the marked form = the zero-end
  extent to the last non-zero byte), the 272-byte embedded-name buffer decoded
  DEAD (the shipped name is always empty — memset-then-strlen with nothing
  written), and the JMF6 version word CORRECTED to `03 00 10 00` by the
  corpus's u64 literal `0x0010000336464D4A` over the finding text's "03 00 01
  00" transcription (the corpus wins; the wrong-word rejection is pinned by
  test). Three registry rows (JMF6 / CFAD12FE / F789342C — the og's
  registration order, one row per key, shared `validate_dbx`), the 4-GUID
  identity probe, the plain form's self-declared u32@+0x7C size, the marked
  form's 512-block zero-end walk. The session-3 "MISSING (N-026)" markers now
  resolve to a filed-and-fixed finding. Proof: Carve.Dbx* (6 tests), suite
  1125/1125 green post-fix. The user's "fixed directory" directive adjudicates
  the S41-recorded FIXED-archive convention conflict in favor of the in-tree
  session-11 amendment — resolved findings land in `findings/FIXED/` from this
  lane forward.

- **M-120 (MAJOR — main RED: committed conflict markers, the 1fb7603b
  push) → FIXED.** The parallel repair lane's `b00118b5` removed the
  markers (the both-sides-folded compare — the fix plan's step-2 audit
  decides: the shipped case table is lowercase, the S-049 contract emits
  uppercase, the folded compare agrees with both); this lane's rebase
  kept it convergent and completed the green restoration; the format
  drift the red window accumulated is repaired at `72a17b7c`. Acceptance
  executed at origin/main HEAD: **0 markers** at origin/main, `cr_tests`
  builds, suite **1152/1152 green** (1110 passed / 42 skips), format
  clean. The step-4 discipline (re-run the gate after every rebase, not
  just before the push) is registered in the worklog handoff. The
  message-truth record (the "1096/1096 green" claim that could not have
  executed against the pushed tree) stands in the filing lane's
  `evidence/executions/s42-build-at-head.md`.
- **M-098 (MAJOR — the HIK/Hikvision private_stream_1 discriminator) →
  FIXED, with N-156 discovered+fixed in the same landing.** The full
  analyser landed: the BB→BD skip path (the og's LE-u32 skip length at
  +4 with the exact < 0x40000001 / < 0x400001 bounds), the direct-BD
  form, the 0x1A + gated-BE16 pack advance, the 6 + BE16 PES advance,
  the cap-exact 1 GiB walk, and the analyser's own name L"HIK"
  (DAT_18006b428 dumped from the og binary: 48 00 49 00 4B 00). The
  N-070 accumulated-score contest is encoded in the row constants
  (base 700: the specific discriminator 1350 outranks the generic
  MPEG-PS 1300 — the trial order alone cannot express the precedence
  under the accumulate-highest-score emission). Proof: Carve.Hik* (4
  tests) + the stashed-engine RED; suite 1157/1157 green.
  **N-156 (discovered during M-098's verification, per §8):**
  validate_mpegps read the BE-u16 MPEG-PS system-header/PES lengths as
  LE — every real PS stream (PES payloads > 255 bytes) walked to a
  byte-swapped extent (the HIK fixture's generic fallback carved 8234
  bytes instead of 136; a 0x0123 PES walked 0x2301). Both reads →
  rdbe16; the S-007 fixture's LE length plants → the BE wire form (the
  fixture could never have been a real stream);
  Carve.MpegPsPesLengthsAreBigEndian pins the decode.
- **M-099 (MAJOR — the AEP/After Effects project analyser) → FIXED.**
  The weak grammar ported AS-IS (the finding's own directive: bare
  RIFX, no "Egg!" body gate — the false-positive posture IS the parity
  target); the walk decoded exactly beyond the finding's text: the
  first 0x200 window at size+8, the 0x200 stepping with the window
  fetched at the counter (the og's 8-byte first-window overlap), the
  "<?xpacket end" marker sizing at marker-position + 6, the zero-run
  sub-walk (a window ending in 8 zeros trims at the first 8-zero run),
  the 0x1e00000 cap exact on both the over-cap and the cap-exit paths;
  the name L"AEP" dumped from the og binary (DAT_180067008:
  41 00 45 00 50 00). RawConfig AEP=1 shipped. Proof: Carve.Aep* (4
  tests) + the stashed-engine RED; suite 1181/1181 green post-fix.
- **M-106 (MAJOR — the ONE/3DS size walks, decoded but standing in) →
  FIXED.** Both walks landed from the first-hand re-read: ONE's
  GUID-recheck chunk chain (size u64@+0xC4 fallback +0x64, the
  0<size≤2x-device gate, the break-point size, the 500,000,000
  clamp), 3DS's MM-branch section walk (the 0x58 shortcut on the
  u16@+0x58 MM marker, the stride u16@+2, the 5x-device cap). A
  documented finding-text deviation: the NCSD/CCI branch KEEPS its
  bounded stand-in (the fix plan's "both stand-ins" step overreached —
  the MM-marked walk would reject every CCI file; the comment
  registers the stand-in). RED pre-fix by the stashed-engine rebuild:
  a 68 MiB ONE chain carved TRUNCATED at the 64 MiB stand-in pre-fix.
  The two school-1 fixtures updated to walk-valid forms with the SAME
  extents (the under-cap agreement the acceptance requires). Proof:
  Carve.OneChunkChain* / ThreeDsSectionWalk* (3 tests) +
  S007NicheBatch2School1Carve green; suite 1187/1187 green.
- **M-125 (MAJOR — the progress display math: the og's piecewise remap +
  99.99 cap, decoded but unregistered) → FIXED.** The display math
  landed as the OPTIONAL query-layer helper family
  (`query::progress_display_remap/_percent/_publish` — FUN_1401af820/
  :950/:a60 verbatim), the engine's truthful linear permille untouched
  (the regression pin). The finding's own test table carried FOUR
  transcription errors against the corpus constants (raw 20 → 21.8 not
  22; 29 → 23.6 not 23.8; 98 → 99.0 not 99.4; 99 → 99.0 not 99.99) —
  corrected per the corpus-wins rule; the 99.99 cap decoded as the
  og's DEAD defensive branch (reachable only for raw > 100); the
  98-boundary discontinuity (99.0 → 98.5) decoded and pinned — the very
  thing the monotonic publish guard exists to hide. O38 → MATCH;
  PARITY-DELTAS D-118 registers the linear-permille engine truth as
  the deliberate divergence. Proof: ProgressDisplay.* (5 tests) + the
  stashed-engine RED; suite 1197/1197 green.

**Board at this lane's M-117 landing (post-rebase over the parallel lanes'
S-048/S-049/S-054/S-053/N-115/N-126/N-148/INFO-tier landings): 39 active on
the filesystem re-count (2 SEVERE · 22 MAJOR · 15 MINOR · 0 INFO — the
INFORMATIONAL tier fully acknowledged+retired by the parallel session-40
lane) + 3 tracked-open (OQ-S4-3 / OQ-S10-1 / OQ-S10-3).** The parallel
lanes filed four new findings in flight (M-118 the lost-partition register
channel, M-119 the partition-boundary read semantics, N-142 renamed the
rsf-variant-descriptor record gap, N-154 the bookkeeping-truth batch 2) —
all in the active queue. The earlier board figures (64, 55, 51, 42) were
each lane-boundary snapshots; the filesystem re-count at each landing is
the operative truth (the N-129 count-drift rule).

## S41 lane delta (2026-10-29 — the continuation audit lane per the user's second-session directive: remove-the-fixed, go deeper, mark all remaining; RECONCILED at push over TWO parallel windows; three filings, all renumbered at the reconciliations)

**Audited: core `c7fa527c` (the pre-rebase pin — the liveness battery's claims are pinned
there); reconciled over the parallel windows to the remote close `e10ec381` (the S-053+
N-141 emission-gate landing on top of the 20+-commit window), corpus `be0cfe9` throughout.**

**This lane's unique product (all first-hand verified before filing; the local M-112/
N-142/I-033 IDs were concurrently consumed on the remote and renumbered per SA-9 — the
DXF-999 finding twice: N-142 → N-151 → **N-152**, each time over a concurrent remote
consumption):**
- **M-118 (MAJOR, renumbered from the lane's M-117 filing at S42 — the SA-9 collision with the retired suppress_overlap row)** — the Outlook Express **Dbx carve analyser entirely absent**: the og
  registers `RawSpeed::DbxAnalyser` (idx 0x15) under THREE magic keys (JMF6 / CF AD 12 FE /
  F7 89 34 2C — `RAWSearchDLL_decompiled.c:2192-2232`), validates via a 4-GUID Outlook
  Express identity table + the JMF6 version word (`FUN_180016740:17509`), sizes from
  `u32@+0x7C` with a marked-form block computation and a 272-byte embedded-name buffer
  (`FUN_180016950:17622`); the core has zero rows (byte-verified over all 122 FMT_
  entries), and **neither implementation's raw_signs.json carries dbx**. This is the
  trail's only never-filed analyser gap: session 3 marked the three keys "MISSING
  (N-026)", planned the filing, never made it — and the ID was consumed by the parallel
  refs-geometry finding (S3-CARVE.md:70 records the unfiled plan).
- **N-152 (MINOR)** — the DXF **999-comment entry key** absent: the og's u16 key 0x3939 →
  CDXFAnalyser (:2722-2733) + the probe's "999\n"/"999\r\n" leading-comment acceptance
  (:42240+); the core keys "  0\r" only and requires SECTION at offset 0 — comment-led
  CAD exports silently missed.
- **I-034 (INFO)** — the **never-cited corpus-artifact register**: 23 of 283 artifacts
  carried zero trail citations; dispositioned one-by-one (the three SearchDLL family
  censuses drove M-118/N-152 (the DBX finding's S42 renumber); EUDCPEPM = the Todo-Backup sibling driver; burn = N-134's
  media-builder family; UIUX/32 → S-050's addendum; the rest = D-26/OOS-UI). Plus the
  **750-entry og-file census**: NO undispositioned product surface.
- **The remove-the-fixed wave (directive 1), pinned at `c7fa527c`:** the full liveness
  battery — **54 of 55 then-active findings STILL**, one PARTIAL (N-132's TxtViewFiles
  leg landed with M-108 — re-scoped by addendum, convergent with the S11-R aggravation
  addendum). No phantom-fixed findings. The parallel windows' subsequent fixes
  (S-051/N-112/N-113/N-142-CrPreview/M-109/S-053+N-141) were retired by their own lanes,
  convergent with this battery's pin.
- **The board-truth repairs:** the "56 active (7/18/24/7)" figure was the S11-final
  truth commit's own arithmetic defect (MAJOR never decremented at M-110's retirement) —
  repaired locally to the filesystem truth 55 (7/17/24/7), convergent with the remote's
  S11B truth commit; the ledger's 296 rows verified TRUE (274 main + 22 OQ); N-111's
  ledger SHA filled (461825d8); OQ-S10-2's row now carries the N-138 refutation note.
- **The FIXED-archive convention conflict RECORDED (not adjudicated):** the session-39
  user amendment ("put fixed findings in fixed directory") vs the S11-R sweep's purge —
  the S41 lane restored M-108/M-110 per the amendment, then conformed to the remote's
  earlier-pushed purge at the reconciliation; the matrix block records the open question
  for the user.
- **S-050's S41 addendum** — the repair line's UI-side surface (row contracts, task
  queue, priority-7 fleet, Advanced Repair escalation) — complementary to the S11-lead's
  per-chunk-feed addendum.

**Board after this lane's push: 64 active (6 SEVERE · 22 MAJOR · 28 MINOR · 8 INFO) /
307 resolved rows (the S41 filesystem re-count: 285 main-pattern + 22 OQ-prefixed, all
unique — the remote's "302" arithmetic is the N-129 count-drift class on its own
ledger claims) / 3 tracked-open (OQ-S4-3; OQ-S10-1; OQ-S10-3).** Next free IDs: S-055+, M-118+,
N-153+, I-035+, A-012+. Evidence: `evidence/decompiled-quotes/s41-registry-key-residue.md`.

**Recommended fix-order contribution:** M-118 (the S42 renumber of the DBX filing) rides the carve-lane queue after the
school-2/3 children (the fix plan's two decode steps are the only prerequisites — the
grammar is 80% decoded in the finding); N-152 is a one-row + one-branch landing on the
existing DXF validator; N-132's remainder (row 45 + R-31's propagated clause + the two
config-model dispositions) is a docs-only commit.


## Session-40-MAIN-lane delta (2026-10-29 — the tracked-open-first lane; the user's "priority to tracked open first" mandate)

**Fix order this lane: TRACKED-OPEN → SEVERE → MAJOR → N-138..N-141.** Opens
at core `461825d8` (pulled), absorbed the S11/S11-R/S40C windows through the
landings below; corpus `be0cfe9` throughout. The CI red at open (two Checks
runs failed [1/2] clang-format — the M-108/M-110 waves' 18 violations in
cr_preview.cpp/test_preview.cpp/test_btrfs_volume.cpp) was REPAIRED first
(the push-window format-dirt class; local [1/10] clean; preview suites green).

**FIXED this lane:**
- **S-053** (SEVERE — refs): the name-destroyed row class now EMITS. The og's
  name builder no-ops on keySize<=4 and its emit path has no name gate
  (ReFSSearchDLL :8674-75/:8478-81 — re-verified first-hand, both sides): the
  core's `name16.empty()` drop removed; unnamed $DATA-only records flow the
  full path with the STORED name empty (content-based grading, identity map,
  session round-trip pinned — never a synthetic name in data); the CLI
  human-readable print carries a `File%06u` DISPLAY placeholder only. Tests:
  Refs.UnnamedDataOnlyRowEmitted / UnnamedRowGradeIsContentBased /
  UnnamedOddLengthKeyEmittedNoCrash / UnnamedNameSessionRoundTrip — all RED
  pre-fix (stashed-engine rebuild), GREEN post-fix.
- **N-141** (MINOR — refs, the S-053 pass over the same gate): the og's
  zero-size AND no-dot noise filter landed with the exact compound semantics
  (FUN_18000ba90 :8317-37 — the FileValues +0x30 field, confirmed the same
  field parse_general_info reads): zero-size-no-dot filtered (counted in
  stats.zero_size_nodot_filtered — never silent), zero-size-with-extension
  and nonzero-no-dot survive, unnamed-zero-size filters exactly as the og
  filters it. Tests: Refs.ZeroSizeNoDotRowSuppressed / ZeroSizeWithExtensionEmitted
  / NonZeroNoDotEmitted / UnnamedZeroSizeRowSuppressed.
- **Drive-by repair (a real memory bug, ASan-caught):** the N-093 test's
  registry iteration `for (s = reg; s->ext; ++s)` walked past kTable's end
  (a global-buffer-overflow under the full ASan lane; the table is
  count-based by contract — cr_signatures.h's "one entry per FormatId");
  repaired to the count-based loop every other consumer uses. The full ASan
  suite (1064 tests) is green with ZERO reports post-repair.
- **OQ-S4-3 in flight:** the Windows benchmark dispatch (run 40747332987,
  test-benchmark.yml smoke profile) is executing — the TTFR transcription
  that closes the OQ rides its completion.

**Board now: 61 active (6 SEVERE · 21 MAJOR · 27 MINOR · 7 INFO) · 306
resolved ledger rows (284 main-pattern + 22 OQ, filesystem-recounted) · 3
tracked-open.** Convention note: resolved findings retire to history with
the Resolution riding the fix commit (the S11-R-enforced convention; the
S-053/N-141 Resolutions live in this lane's fix commit and the ledger rows).

**Lane claims ahead (coordination):** the S-052 SEVERE fix is the parallel
SEVERE-first lane's in-flight work (not duplicated here). This lane takes
next: S-049 + S-048 (the carved naming + grouping pair, one carve wave),
then S-050, S-040, S-054, then the MAJOR tier (M-116 first), then N-138..N-141
remainders. S-053/N-141 are LANDED — the SEVERE-first lane's queue entry for
S-053 drops per the convergence rule.

---
## Session-40-continuation delta (2026-10-29 — the S40-continuation minors lane over the S11 filings; the user's "priority to minor first" mandate)

**Fix order this lane: MINOR → INFO** (the SEVERE tier is the parallel S40
SEVERE-first lane's declared territory — S-052 in flight, S-054/S-053/S-049/
S-048/S-050/S-040 their queue; no duplication). Opens at core `c0f69250`, corpus
`be0cfe9`.

**FIXED this lane so far:**
- **N-142** (MINOR — ui): the dead CrPreview origin pair (src_session/src_node_id)
  removed with its two preview-open writes; the M-083 comment re-truthed to the
  landed promotion's own-argument addressing (ABI-CONTRACT §7i). Acceptance grep
  zero hits; cr_ui_tests 59/59.

**Board now: 54 active (7 SEVERE · 20 MAJOR · 27 MINOR · 7 INFO) · 306 resolved
ledger rows · 3 tracked-open.**

---
# SUMMARY.md — executive summary (the S11-R reconciliation lane on top; the prior lanes below, newest-first)

## S11-R delta (2026-10-29 — the reconciliation lane: the 63-commit push window absorbed; the unique product landed)

**Audited: core `8365956e` (the lane's open baseline) → re-verified at
`b6e0a9ef` (the post-window HEAD, code-current).** The lane opened against
the S10-F close and executed all three asks (board truth · the 54-finding
re-triage · the S10-F queue: the RAWSearchDLL infrastructure walk, the
API-DELTA currency pass, the OQ-S8-1 trace) — then hit the push rejection:
**63 concurrent-lane commits** (the parallel S11/S11B/lead/39/40/S12 lanes
executing the same three-ask instruction + the fix wave: M-108/M-109/M-110/
M-072/M-074/M-077/M-102+M-083/N-093/N-111/N-112/N-113/S-051 landed).
Reconciled per the SOP (earlier remote push wins; extend-don't-refile; no
duplication): the lane's ABSORBED work is enumerated in the worklog and the
matrix block; the lane's UNIQUE product landed this push:

- **M-116** (MAJOR — the headline): **the M-100 re-open.** The session-10
  retirement's residue claim ("reachable today through
  cr_ui_preview_metadata") is FALSE — verified first-hand at TWO trees:
  `cr_ui_metadata` (cr_ui.h:1214-1224) has no entries field and
  `preview_metadata_impl` (cr_ui.cpp:3613-3631) never touches
  `Metadata.entries`. The archive inventory (M-066's engine, the CD walk,
  the 512-entry bound) is unreachable from every shipped consumer surface —
  the M-102 repair shape (two-call export) is the fix. No remote lane caught
  it (the findings-tree grep for the symbols: zero). The M-100 ledger row
  carries the re-open annotation.
- **N-150** (MINOR — the API doc-truth batch, re-verified CURRENT at
  b6e0a9ef): both staleness directions — the false-missing R-84 pair (the
  6-facet driver-status surface landed; the rows still say "rda_available
  probe ONLY"), the doubly-stale R-37 (→ M-116), the overclaiming counts
  (API.md:81 "81" vs the actual 83 · CORE-API-CURRENT:16 "34+OK/10-events"
  vs the actual 38/12 · :345's 3-way-stale status line), and the
  stale-landed API-DELTA rows (10/14/18/22/§3).
- **The addenda** (unique, the parallel lanes' re-triage did not carry
  them): N-131's fix-plan re-base (bit 512 TAKEN by N-100's landing); N-132's
  aggravation verified current (the e4f3184 R-31:225 spread); M-107's two
  factual nits (AI has its own validator; EPS/AI are 16 MiB).
- **I-031's register extension**: the dormant `only_common` vendor mode (a
  deliberate-divergence D-row) + the A4 device-slot phrasing correction.
- **The board-truth repair**: the re-created `findings/FIXED/` dir (7
  ledger-rowed files — the convention violation the ledger's own header
  warns about) swept to history.

**Board after S11-R (filesystem truth): 64 active files** (7 SEVERE ·
21 MAJOR · 29 MINOR · 7 INFO) · **300 resolved ledger rows** · **3
tracked-open** (OQ-S4-3, OQ-S10-1, OQ-S10-3). Next free IDs: S-055+,
M-117+, N-151+, I-034+, A-012+. **Recommended fix order contribution:
M-116 first** (the M-102-pattern two-call archive export — small, unblocks
a landed engine; composes with N-150's row-state sweep in the same wave).

## Session-40 delta (2026-10-29 — the diff-to-fix board execution: SEVERE-first)

**Audited/fixed: core `4a6a5164` at open, corpus `be0cfe9`; suites 1003 + 59 green
at open.** The fix session's board at open: **50 active (6 SEVERE · 18 MAJOR ·
21 MINOR · 5 INFO) + 3 tracked-open (OQ-S4-3, OQ-S10-1, OQ-S10-3) / 294
resolved rows.** Every finding is re-verified against the corpus and the running
code before any change (the S-051 evidence probe re-executed verbatim at open —
the RUN 2 silent-empty verdict reproduced on current main, then flipped by the
fix). Resolutions land per the retire convention (Resolution rides the fix
commit; the file retires to history with a ledger row).

- **S-051 FIXED at `6ecd6bb4`** — the whole-disk target set (the session's
  first SEVERE). The plain disk scan reads the partition list at scan setup and
  makes every used, non-container partition a scan target (the og's
  CDoScanPartition flow); the evidence probe's RUN 2 flipped to files=4/
  deleted=2; six whole-disk tests; the discovery no-double-scan reconciliation;
  honest skip accounting (table_volumes*). Matrix cell → MATCH; the retired
  file's Resolution carries the full record.
- **S-054 FIXED at `d7ba41f6`** — the TS/M2TS cap re-pinned to the og's 200 MB
  controller global (the S-047 landing's "x 10" provenance refuted first-hand;
  registry pin + the 220 MB behavior clamp test).
- **INFORMATIONAL tier ACKNOWLEDGED (9: I-026/I-027/I-028/I-029/I-031/I-032/I-033/I-034/I-035)** —
  every deliberate-difference rationale re-verified first-hand at the
  acknowledgment tree (the unsigned-by-design signing posture LIVE in
  DRIVER-RELEASE 63-66/180 + release.yml; the thumbnail superset at
  PARITY-DELTAS:3330; the 19-vs-15 contract grounding as recorded) and the
  register class confirmed on-file ground truth. No code change (the
  mandate's INFORMATIONAL contract); the safety-hardening divergences are
  preserved-for-future-work by the Resolution text.
- **M-117 FILED + FIXED at `d7ba41f6` (NEW FINDING, §8 — renumbered M-116→M-117 at the rebase (the S11-R lane's M-116 pushed first); discovered during
  S-054's verification)** — the suppress_overlap trial loop ran the full
  cap-sized walk for every mid-stream sync before the covered check discarded
  it (the 220 MB fixture: >600 s non-completion → 11.3 s). The covered-check
  reorder only; the post-emission jump variant REJECTED by the full-suite gate
  (the og ranks overlapping candidates — the S007 AAC ADTS lesson).

**Board at the session-40 close (the multi-lane truth): 42 active (3 SEVERE · 23
MAJOR · 16 MINOR · 0 INFO) + 3 tracked-open / 317 resolved rows.** The remaining
SEVERE set (S-048 carve grouping, S-050 the file-repair registration, S-040 image
create) is CLAIMED by the session-40-severe lane (worklog territory claim
75dd2c96: S-048 first, then S-050/S-040); the S-054 class (regressions) stays
zero-tolerance with the DestArming wiring fence landed. This lane's session-40
landings: S-051 (the whole-disk target set), S-054-the-TS-cap + M-117 (the
discovered-and-fixed quadratic re-walk), S-054-the-N-013-restore (the regression
+ the fence), the INFORMATIONAL tier (9 acknowledged), the tracked-open
dispositions, and the S-052 convergence (the remote lane's complete landing
stands; this lane's in-flight duplicate was dropped per earlier-push-wins).

---

# SUMMARY.md — executive summary

## Session-11 lane B delta (2026-10-29 — the continuation re-triage + landing-parity audit, RECONCILED at every push over the S11/S39/S40 parallel lanes)

**Audited: core `7990a86d` at open → `620f4497`-lineage at close** (the lane's
verification baseline `743d7b8d` incl. the `740d9b56` thumbnail ABI) · **corpus
`be0cfe9`**. The user's three asks, executed:

1. **Remove-the-fixed** — all 55 active-at-open findings re-verified (three parallel
   agents + lead spot-verification). This lane's moves: **M-102/M-083 conformed**
   (the e56baac7 landing's missed purge step 3), **N-134 retired FIXED-ALREADY**
   (the b202f9ed archaeology: all six register rows pre-dated the finding's own
   filing by 6.5h — verified `git show 44f043a5:...`), **I-030 resolved** (the
   module map completed: reghive.md + apply.md written per its own packaging plan).
2. **Go-deeper** — the T4 landing-parity wave: M-096/N-100/thumbnail-ABI
   PARITY-VERIFIED; M-093's residual filed; M-079's image-stem divergence confirmed
   N-133-owned (+ the registration leg: D-03 stale, no D-row); the phantom-SHA
   truth class repaired (11 cells, git-cat-file-verified; convergent with
   session-39's 73-row census).
3. **Mark-all-remaining** — **M-112** (MAJOR: the CSL 0x44c size branch
   decoded-but-unported with NO active owner — the M-107 class's twelfth site;
   renumbered from M-111 over the parallel lane's explorer-browse M-111),
   **N-142** (MINOR: the dropped thumbnail variant's dead CrPreview wiring;
   renumbered from N-137 over the parallel lane's thumbnail-header N-137),
   + 16 addenda (N-135's per-item status; N-133's registration leg; ten
   cite-refreshes; M-107's three in-file corrections).

**Board at this lane's close: 55 active (7 SEVERE · 18 MAJOR · 24 MINOR · 6 INFO) /
299 resolved rows (277 main + 22 OQ, whole-table re-count) / 3 tracked-open.**
The recommended first-20 fix order is in the worklog's S11B close entry (S-052
first — silently wrong recovery bytes; then the mechanical MAJORs M-109/M-103/M-112).

 (the S11 deep-audit lane on top; the prior lanes below, newest-first)

---

<!-- S12B reconciliation: the parallel continuation/fix lanes landed mid-rebase; their blocks stay above (earlier-push-wins), this lane's delta rides below with its post-rebase addendum. -->

## S12 lane delta (2026-10-29 — the second-session continuation: remove-the-fixed verified + the below-artifact deep walks; ONE REGRESSION FOUND — a landed safety fix silently reverted)

**Audited: core `c7fa527c` (worklog-only HEAD `36c1d3e5` in the tree — a pre-rebase phantom token, the message-truth class; the S12 product survives at `1bfeda19`/`3c5f2027`/`25e408b8` in main's history; the code tree is `c7fa527c`'s), corpus `be0cfe9`.**

**Wave 1 — remove-the-fixed (the mandate's first directive):** all 55 active findings'
defect sites re-verified at HEAD (`evidence/s12-liveness-sweep.md`): 41 LIVE, 4 PARTIAL
(N-132/N-135/N-138/I-030 — their residuals recorded in N-143), 5 N/A-driver-repo,
5 register. **No active finding is fixed-but-unretired** — the board is truthful. The
from-scratch build+suite EXECUTED at this tree: **1117/1117 green, 0 warnings under
-Werror** (`evidence/executions/s12-build-execution.md`) — the standing "1027" count
was stale (N-154's item 4).

**Wave 2 — the deep walks (the mandate's go-deeper + mark-all-remaining):** four
parallel agents (core module deep-read; corpus behavioral re-walk; build/exec; the
liveness sweep), every candidate then **first-hand verified by the lead** before
filing (the SOP): the N-013 git archaeology, the register-channel decode
(PartitionRecoveryD + DRWUI both), the CBasicPartitionDevice read-wrapper decode +
its real-log observation, the rsf variant-record field diff. Six findings + one
addendum filed:

- **S-054 (SEVERE)** — **the N-013 regression**: the multi-extent destination arming
  landed complete at `3b5397a1` and was CLOBBERED at both production call sites by the
  unrelated M-036 NVMe commit `1ebd1bcc` — `volume_extents()`/`set_destination_spans()`/
  `destination_volume_letter()` have ZERO production consumers at HEAD while
  `cr_transport.h:124-126` still publishes the all-spans requirement and the ledger
  says FIXED. The F12 overlap refusal is silently defeated for spanned/RAID
  second+ spans and junction-rooted destinations (the write-proceeds-onto-source-disk
  data-loss window); the suite is green over it because no test pins the wiring.
  The highest-value fix on the board (a re-landing + a wiring fence).
- **M-112 (MAJOR)** — the og's three-source lost-partition candidate assembly
  (**mbr / filesystem / register**) decoded engine-side: the register source is the
  Explorer `MountPoints2\CPC\Volume` walk (PartitionRecoveryD:8930-8965; the
  Data-blob '#'-delimited hex-placement decode in FUN_18000f5b0; observed firing in
  the real-run log with three register records). The channel uniquely recovers
  partitions whose on-disk remnants are destroyed; the core has no counterpart and
  S-051's fix plan builds table-only targets — the plan gains the channel merge.
- **M-113 (MAJOR)** — partition-boundary read semantics: the og's device read
  CLAMPS + zero-fills + SUCCEEDS (RTTI-confirmed CBasicPartitionDevice at
  DRWUI:183633-183666; the real log's `i64CountofSectors - i64RealReadSector = 10175`
  fired mid-scan and the probe continued); the core's `PartitionSlice` hard-errors
  per-read AND the CLI/UI partition opens refuse whole volumes when the table's
  declared length overruns the device (the damaged-table target market). The
  discovery path already clamps — the fix unifies the tolerant posture (read-side
  only; the write path keeps hard bounds).
- **N-142 (MINOR)** — the .rsf variant-save device-descriptor record (the 76a672b0
  GUID, [552-B struct]+[5400-B volumeDevice blob]): firmware/WWN/interface + the
  512-byte boot-sector snapshot have no identity-card equivalent (model/serial/
  boot_hash only).
- **N-143 (MINOR)** — bookkeeping batch #2: the **N-079 ID collision** (an in-source
  audit comment cites a filing the ledger gives to a different finding), the
  GPT-salvage strictness unregistered (PARITY-DELTAS row missing vs ldm.md's plain-M),
  the tree/err/reghive/apply module rows (I-030's remainder), the stale 1027-vs-1117
  suite-count claims.
- **I-033 (INFO)** — register batch: RegLib's hive-WRITE half (the hbin allocator),
  DiskHealth's resident-monitoring product frame, the Explorer context-menu verb, +
  the DiskHealth-artifact trayicon erratum.
- **S-050 ADDENDUM** — the fix plan's build order omits the VIDEO engine: the
  corpus's deepest-decoded repair algorithm (125 trace points) and the only repair
  surface with an engine-side seam (the datarun-selection half consumes
  `tree::FileNode::extents` — the core's own output); the eligibility-signature seam
  and the output-location contract also unnamed.

**Board at this lane's own filings (pre-rebase truth): 61 active (8 SEVERE · 19 MAJOR ·
26 MINOR · 8 INFO).**

**POST-REBASE RECONCILIATION (the hottest parallel window of the trail — 61 commits
absorbed mid-push):** the fix lanes landed S-049/S-051/S-052/**S-053** (the ReFS
emission-gate pass — the board's standing top recommendation), M-104, M-109, and the
minor set N-096/N-112/N-113/N-115/N-121/N-122/N-128/N-129/N-130/N-134/N-137/N-138/N-141
(I-030 too), filing M-114..M-117, N-144..N-153, I-034 alongside; **four of this lane's
six IDs collided with the parallel continuation lanes' filings** — the earlier pushes
keep the numbers, this lane's findings renumber: M-112→**M-118** (the register
channel), M-113→**M-119** (the read-clamp semantics), N-143→**N-154** (the
bookkeeping batch), I-033→**I-035** (the register batch); S-054 and N-142 keep theirs.
**Every defect site re-verified LIVE at the post-rebase HEAD `cd883754`** (a pre-rebase phantom token, the message-truth class — the re-verification rode the `25e408b8` reconcile, whose tree is the surviving record) (the N-013
arming still single-extent with zero spans-consumers; MountPoints2 still absent; the
slice still strict; the identity card still model/serial/boot_hash — only line drift,
all cites re-pinned). **Board at close (filesystem truth): 55 active (4 SEVERE · 23
MAJOR · 19 MINOR · 9 INFO) / 313 ledger rows / 3 tracked-open.** Next free IDs:
S-055+, M-120+, N-155+, I-036+, A-012+.

**Recommended fix-order contribution (this lane's filings first):**
(1) **S-054** — the re-landing is mechanical (the `3b5397a1` shape exists in git) and
the wiring fence prevents the class; the data-loss window makes it the board's top
priority alongside the standing S-051/S-052/S-053 set;
(2) **M-113** — the create-clamp + read-clamp pass (small, read-side only, with the
write-path fence test); composes with S-051's whole-disk fixtures;
(3) **M-112** — the register-channel seam (win-layer walk + candidate-source merge;
the decode is complete, one fix-time probe names the total-field split);
(4) N-154's bookkeeping batch (the N-079 comment repair is one source-comment edit);
(5) N-142's identity-card tail (a v6 tail, round-trip-tested); then the standing
board order (the S-053+N-141 ReFS pass, S-051/S-052, the S-050 decision now
carrying the video-engine amendment).

---

## S11 lane delta (2026-10-29 — the second continuation lane: the named-never-done walks EXECUTED — the RAWSearchDLL infrastructure function-level walk, the ZFS/ReFS function-level walk, the OQ traces, the 56-finding liveness triage; one FALSE RESOLUTION refuted by decode)

**Audited: core `46b81573` at close (code tree `432d15dc`'s; opened at `8365956e`,
reconciled mid-session over the hottest window of the trail — the parallel lanes'
M-072/M-074/M-077/M-096/N-093 fix landings, the M-102/M-083 retirement, the
N-129→N-135 renumber, the ledger conflict sweep, M-111's filing and BOTH OQ
resolutions all landed mid-flight and were absorbed by rebase + first-hand
re-verification), corpus `be0cfe9`.**

**This lane's unique product (all first-hand verified by the lead before filing):**
- **S-053 (SEVERE)** — the ReFS unnamed-$DATA-only row drop: the og EMITS
  name-destroyed records as recoverable unnamed rows (name builder no-ops on
  keySize≤4, no emit gate — ReFSSearchDLL `:8674-8675`/`:8478-8481`); the core
  drops them (`cr_refs_engine.cpp:1102`) — silent loss of recoverable content
  (the S-034 class). refs.md B5's "minor, parity unknown" note resolved the
  severe direction by decode.
- **N-138 (MINOR, process-truth)** — the S10-F **OQ-S10-2 "dead payload"
  resolution REFUTED by byte-level decode**: VideoViewer/sfc is
  VideoFileRepairer.exe's LIVE encrypted engine-init database (" cfs" magic,
  16-byte XOR key with "FAN." at bytes 4-7 — the "motif" was the key showing
  through zero plaintext, 827,489-byte clean u32 framing, custom-LZ v2, ctor
  load + rebuild-on-failure). The zero-hit grep could not match `DAT_` symbols
  or hex magic constants — the methodology note is recorded for every future
  "no consumer" claim. OQ-S10-2's file + N-134's register row corrected by
  addendum; the question stays closed with the corrected answer.
- **N-139/N-140 (MINOR)** — the RAWSearchDLL infrastructure walk's two residue
  findings: the carve OOM policy (og: 5-line diagnostic + non-continuable SEH;
  core: silent, and its own two paths disagree — 8 nothrow sites vs 2 throwing
  buffers with no catch upstream) and the per-extension carve-result
  aggregation surface (og's GetRawUserInfoMap + DRWUI breakdown vs CarveStats'
  13 scalars — no API tallies carve_format over results).
- **I-031/I-032 (INFO registers)** — the two walks' deliberate-divergence sets
  and new trail data: the pending-16 cap/oldest-first eviction, Stop()'s
  partial-file flush, the engine's zero progress emission, the 512-sector
  refusal, the full 36-slot controller vtable; **OQ-T6 RESOLVED** (the og
  resolves multi-vdev DVAs via first-registry-entry fallback — MATCH with the
  M-019 landing), **zfs B9 closed MATCH** (the og persists 8-byte times — the
  "nanoseconds" note refuted), **refs B4 decoded** (2-bit grade byte at
  node+0x70 — the core's finer grading stays the registered superset), the
  ReFS/ZFS volume-label closure (neither decodes one — N-133's completion set),
  and the E-S11 errata batch (7 precision corrections).
- **N-141 (MINOR)** — the og's ReFS zero-size∧no-dot junk filter decoded and
  registered absent (the M-022 OQ-3 completion; rides the S-053 fix pass).
- **M-111 corroboration** — this lane's independent CWindowsFS consumer trace
  (concurrent with S10-F's) agrees on every load-bearing cite; the
  corroboration addendum double-verifies their filing.
- **The triage wave's residue** — the 56-finding liveness sweep ran, but every
  repair it prescribed (M-102/M-083 retirement, N-129 collision, ledger
  sweeps) was landed CONCURRENTLY by the parallel lanes — reconciled, no
  duplication; the 5 driver-repo NDVs dispositioned as OQ-S10-3 by the driver
  lane; evidence/s11-triage-sweep.md carries the full table.

**Board after this lane's push (re-truthed over the mid-push M-108 AND N-111
retirements that landed under this lane's rebases; ledger per session-39B's
row-re-count): 56 active (7 SEVERE · 18 MAJOR · 24 MINOR · 7 INFO) / 296
resolved rows (FIXED-LEDGER) / 3 tracked-open
(OQ-S4-3 refined in place; OQ-S10-1; OQ-S10-3).** Next free IDs: S-054+, M-112+,
N-142+, I-033+, A-012+. **Recommended fix-order contribution:** (1) **S-053 +
N-141 as one ReFS emission-gate pass** (the S-053 half is silent content loss —
the highest-value single fix on the board per severity); (2) N-139's
bad_alloc-unification (small, contract-cleaning); (3) N-140's stats tally
(report parity, mechanical); (4) the N-138 doc-truth sweep (the OQ/N-134/
register corrections ride one docs commit); (5) I-031/I-032's register addenda
(D-78/D-101/I-009 + zfs/refs module rows — documentation-only).

**Territory note for the session-11-lead lane:** your claimed wave-1 (the
RAWSearchDLL infrastructure walk) was EXECUTED and filed by this lane before
your claim landed (agents spawned 17:50, results 18:0x, claim 18:15:38) —
reconcile extend-don't-refile against N-139/N-140/I-031 +
`s11-rawsearchdll-infra-walk.md` (the six-family carve-grammar half of your
wave-1 remains yours); wave-3's non-searchdll trees do not collide with this
lane's ZFS/ReFS searchdll walk.

---

## Session-11 delta (2026-10-29 — the user's session-2 continuation: remove-the-fixed + go-deeper + mark-all-remaining)

**Audited: core `7990a86d` at open (the M-096 mid-session landing absorbed), corpus
`be0cfe9`.** The continuation mandate decomposed into three waves. **Wave 1
(remove-the-fixed + board truth) is complete at this writing:** M-102/M-083
independently **PARITY-VERIFIED** and retired (their landing `e56baac7` appended
Resolutions + ledger rows but never executed purge step 3 — both files sat active
while the ledger claimed them resolved; the verification executed the engine layer
standalone, 40/40 checks, zero ASan; residuals filed as **N-136** — the thumbnail ABI
header's "ERR_TRUNCATED never returned" contradiction + the inert `allow_embedded`
knob — and **OQ-S11-1** — the og's promotion guard carries undecoded conditions
beyond `!= 2` at `DRWUI_decompiled.c:422508-511`); the **N-129 ID collision
repaired** (lane-B's register-counts finding renumbered **N-135** per the SA-9
earlier-push-wins rule); the FIXED-LEDGER's **committed merge-conflict markers
resolved**, the **missing M-082 row added**, the **M-088 fictional retired-path
corrected**, and **every recent (M-079+) phantom fix-SHA repaired** to the real
post-rebase commits (M-041's cell held a CORPUS-repo SHA; M-064's an upstream
rda-driver SHA; M-090's the decompile address `180053de0`); the 54 ancient
as-recorded-in-file cells censused in the ledger's S11 erratum (extends N-129's item
4); the gate docs (README / CROSS-REF-MATRIX / modules/preview / modules/carve)
brought current. Full record: `evidence/s11-board-truth.md`.

**Board at the wave-1 close, AFTER the rebase reconciliation over the concurrent
lanes (sessions 11B/11C/11D/39/39B all landed mid-window): 51 active (6 SEVERE ·
19 MAJOR · 21 MINOR · 5 INFO) / 289 resolved rows (the session-39B recount: 269
main + 20 OQ) / 5 tracked-open (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2 videoviewer,
OQ-S10-3 driver-repo — the S11-lead collision repairs). Next free IDs: S-053+,
M-111+, N-138+, I-031+, A-012+.** The reconciliation details: the N-129 direction
corrected per the file-addition timestamps (lane-B keeps N-129; the wave-3
bookkeeping file is N-135 — the S11-lead draft had it backwards, the remote 11B
repair stands); the S11-lead's N-136 draft renumbers **N-137** (the remote's N-136
pushed first); the driver-repo OQ-S10-2 renumbers **OQ-S10-3**; M-102/M-083's
retirement verification (engine executed standalone, 40/40, zero ASan) rides the
ledger cells + `evidence/s11-board-truth.md`; the S11-lead adds to the session-39B
phantom sweep the four rows it missed (M-041's corpus-SHA cell, M-088's placeholder +
fictional path, the M-083/M-102 verification annotations, the 54-row
as-recorded-in-file census erratum). **Waves 2-3 (the five go-deeper lanes) LANDED at the session-11-lead close:**
12 findings filed — **S-054** (SEVERE: the S-047 landing's TS/M2TS cap is 10×
the og's 200 MB clamp, the "×10" provenance refuted — 2e9 appears nowhere in the
og binary, no test pins it), **M-115** (MAJOR: the thumbnail grade-promotion
trigger is mis-modeled — the og promotes on clean recovery-READ success,
decode-independent, tier-gated, preview-pane-excluded; core promotes only on
decode Ok and UNDER-promotes damaged-but-readable images; OQ-S11-1 RESOLVED by
decode, binary-verified at 0x140263b2c-b9a), **M-113** (MAJOR: the recovered-OS
boot-environment write-side surgery absent — the TempSystem/TmpBCD/TmpSoftWare
chain; R-69's "PRESENT" verdict is over-coarse), **M-114** (MAJOR: MainModule's
silent System-Restore purge absent + undecided + undocumented), **N-149, N-143..N-146, N-148**
(MINOR: the filter's always-on OLE/RIFF/ZIP set; the ISpecial two-step dispatch;
the TxtView text-snapshot thumbnails; the pendingMarkedDisk cross-run marker; the
FH11 unowned stand-in; the seven-item comment-truth batch; the CSL 0x44c formula
converged with the S11B lane's M-112 — this lane's draft dropped per
extend-don't-refile),
**I-033** (INFO: the config/data-asset completeness pass — every engine-behavior
asset byte-identity re-verified; the register corrections; legacy.dll closed; the
tree bookkeeping) — plus the S-050 wiring addendum (the per-chunk repair feed),
the N-134 decode closure, and seven CORPUS-ERRATA rows. The negative results are
recorded (the 17-row infrastructure MATCH table, the 40-asset N/A inventory, the
19-behavior interaction map, the binary-level CLEAN re-confirmation, the
no-TODO/no-stub census). Evidence: `evidence/s11-lead-deep-lanes.md`. The board's
counts at this lane's close ride the next truth pass (the parallel lanes were
still landing).

# SUMMARY.md — executive summary (the session-11D fix lane on top: the diff-TO-FIX execution)
## Session-11D fix lane (2026-10-29 — lane D, the diff-TO-FIX execution session; first landings)

**Fixed this wave (this lane, verified against corpus `be0cfe9`):**
- **A-012 (ACUTE, DISCOVERED during S-051's verification)** — the 81ee9775
  rebase-window sweep dropped M-093's FMT_SVG carve-registry row (kTable 119
  vs FMT_COUNT 120): the F8 gate hard-downed EVERY `carve_slice` call with
  ERR_BAD_PARAMETER on all of main (`RegistryF8Gate` RED at `bcf31316`,
  reproduced first-hand; the row restored verbatim from `ca701711`; GREEN
  post-fix; the parallel lanes' 9 residual clang-format violations swept in
  the same commit). Full evidence:
  `findings/ACUTE/A-012-carve-registry-lost-fmt-svg-row-in-rebase-sweep.md`.
- **S-051 (SEVERE) FIXED** — the whole-disk scan now builds per-volume
  targets from the partition table (the og's CDoScanPartition flow,
  architecture/04 §3.4): every used non-container entry probed per-target,
  scanned under its display-name root, skipped-honestly when no filesystem
  (counted in `table_volume_skipped`), never double-scanned (discovery's
  table-match candidates for scanned targets dropped). The executed RUN 2
  (rc=0 + 0 files on the healthy two-volume fixture) reproduced at session
  open, then fixed: rc=0, files=4, deleted=2, VOLONE/VOLTWO roots. 8
  `Orchestrator.WholeDisk*` tests + 2 `FatEngine.ReadVolumeLabel*`.
- **M-104 (MAJOR) FIXED** (rides S-051's builder) — the display-name tiers:
  the FAT boot-sector label → the `NTFS @ <LBA>` FS marker → the anonymous
  form (last resort, now reserved for genuinely-lost candidates); one tree
  per volume with deleted-as-flag; names round-trip through the session.
  The NTFS/ext4/exFAT label decodes compose via N-133's landing (MINOR tier).
- **Conventions**: the second committed FIXED-LEDGER conflict (the N-093/
  M-096 rebase's unswept markers) resolved both-rows-kept per the 64a3b18
  convention; the go-forward finding-file convention now carries the
  resolved files in `findings/FIXED/` per the user's instruction.

**Board after this wave (merged over the parallel S11/39/40 lanes' 34-commit
window, re-counted at the filesystem truth post-rebase): 53 active
(6 SEVERE · 17 MAJOR · 24 MINOR · 6 INFO)** / 301 ledger rows (279 finding
rows + 22 OQ) / 3 tracked-open. This lane's suite proof at its tree:
**1105/1105** green; clang-format 18.1.8 clean over src/include/tests. Next
free IDs: S-053+, M-113+, N-143+, I-031+, A-013+. (The count reconciliations
with the parallel lanes' own close-line counts ride the next conform pass —
the remote's 602dd170 claimed 299 rows pre-this-landing; +A-012/S-051/M-104
= 302 data rows by the row-pattern count, the drift recorded for the next
whole-table re-count.)

---

# SUMMARY.md — executive summary (the SA-10 lane on top: the triage + reconciliation over the hot parallel window; SA-10B and the fix/closing lanes below)

## Session-11 fix lane (11C) delta (2026-10-29 — the user's fix-session mandate: verify, fix, prove, mark)

**Baselines: core `35bee726` at open → `f4e500de` (this lane's S-052
landing); corpus `be0cfe9` (unmoved). ctest 1121/1121 green at the
landing tree (Linux gcc -Werror Release; win_syntax_check green;
Windows CI rides the push).**

- **S-052 → FIXED (`f4e500de`)** — the LDM span recovery basis: the
  finding's full architectural plan landed (per-root basis ids in the
  tree + node-flags bit10 persistence; the controller's heap device
  registry replacing the dead stack device; the SafeWriter's
  LdmBasisTable per-item routing; the R7.2/F12 overlap refusal mapped
  to physical member spans; REC_LDM_LAYOUTS + root-stamp session
  persistence with the lazy re-arm at recovery/preview/thumbnail).
  GAP-S6-15 CLOSED. Proof: `LdmRecovery.*` (7 tests, the executed
  0x11-vs-0x22 fail-before embedded in the span test) +
  `Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips`.
  Divergences registered: D-106 (missing-member policy), D-107
  (conservative overlap families).
- **Convergent with the parallel lane:** M-108 + N-111 + N-112 landed
  by the concurrent fix lane mid-flight (this lane's independent M-108
  implementation was dropped at the pull — no duplication; the lane
  then took S-052, the board's top-recommended architectural fix).
- **Convention amendment (user instruction):** resolved findings now
  land IN-TREE under `findings/FIXED/` with their Resolution sections
  (the ledger indexes both forms; the purge convention applies only to
  pre-session-11 retirements).
- **Board at this push:** see the FIXED-LEDGER for the resolved set;
  the active folders carry the unresolved findings (the filesystem is
  the truth — counts recomputed at every push).


## Session-39 delta (2026-10-29 — the diff-fix session, the user's "priority to minor first" mandate; opens at core `35bee726`/corpus `be0cfe9`, rebased through the 11C/11D fix-lane opens)

**The fix order this session: MINOR → INFO → MAJOR → SEVERE → TRACKED-OPEN** (the
user's explicit line; the SEVERE/MAJOR territories belong to the parallel 11C/11D
lanes — the coordination contract in worklog). Board at the post-rebase open: 50
active (6 SEVERE · 20 MAJOR · 19 MINOR · 5 INFO) + 5 tracked-open (OQ-S4-3,
OQ-S8-1, OQ-S10-1, OQ-S10-2 now carrying M-086/M-087/M-089 + N-107/N-127's driver
halves per the driver-lane's grant-boundary disposition).

**Hygiene repairs at open (T1c/T1d/T1e):** the committed FIXED-LEDGER merge
conflict (5fecdae1) resolved over the convergent session-11B repair (both sides
kept as dated history, one truth line); the second committed-marker occurrence at
the table tail (the N-136/N-093 row collision) repaired — both rows kept; the
count truth re-executed twice (284 pre-rebase → **288 rows = 268 main-pattern +
20 OQ** post-rebase, no duplicate IDs); the N-129 ID collision resolved (wave-3's
bookkeeping file renumbers to N-135, convergent with session-11B's own renumber).

**FIXED this session so far:**
- **N-111** (MINOR — carve): the JP2/JPX/JPM family landed (FMT_JP2 on the og's
  own "jP  " signature-box key; FMT_JPX/FMT_JPM on the brand-position keys; the
  exact-match brand map via format_override; **the size machinery the finding had
  recorded as undecoded was decoded first-hand and landed** — the BE u32 box walk,
  the len==1 extended form, the len==0 FF D9 EOC scan with the 16-zero back-off,
  the jp2c-consecutive end rule, cap 500 MB; 7 tests; one finding correction:
  "corrupt-length still carves" is gate-level truth, extent-level false).
- **N-112** (MINOR — carve): the LZH family landed (the four method-key rows
  "-lh0-/-lh5-/-lhd-/-lh7-" at +2; the entry-chain walk fully decoded INCLUDING
  the .data method-constant dump from the corpus binary that settled the
  DAT_-label mapping; the -lh5 hdr-only stride quirk ported exactly and PINNED
  by its own test; cap 200 MiB byte-exact; 6 tests).
- **Session-39 convention amendment:** resolved findings now live in
  `findings/FIXED/` (the user's "put fixed findings in fixed directory"
  instruction; the ledger is the index — see the amended go-forward section).

- **N-113** (MINOR — carve): the text school landed — six rows
  (VCF/GPX/GPX-BOM/OSP/POS/CHFP), every decoded grammar ported including the
  multi-card continuation, the per-chunk find quirk, OSP's reject-on-missing,
  and the POS needle settled first-hand from FUN_18001a470 (the finding's open
  question); 9 tests.

- **N-133** (MINOR — orchestrator/engines): the engine-side on-disk volume
  labels landed — probe_slice_volume_label (the four family decoders, the
  NTFS $Volume path through the real bootstrap), the EngineRun/stats/cfg
  wiring, the corrected precedence at all three consumer sites, M-079's
  refuted-premise comment replaced, the ext4 dead decode surfaced, PARITY-
  DELTAS D-112 registered; two finding corrections (the exFAT dentry form;
  the HFS+ half as the known residual); 7 tests.

- **N-135** (MINOR — audit trail): the bookkeeping truth batch closed — the
  item-by-item verification (three items FIXED-ALREADY by the earlier repairs,
  reproduced; three repaired this session), SIX additional live phantom cites
  found by the finding's own guard and re-pinned, the I-011 magic-constant
  mis-paste repaired; scripts/verify/verify_trail_shas.py built and GREEN
  (470 tokens, all resolving).

**Board now: 45 active (6 SEVERE · 20 MAJOR · 14 MINOR · 5 INFO) · 292 resolved
ledger rows · 5 tracked-open.**

## Session-10 lane-B delta (2026-10-29 — the continuation deep audit, RECONCILED AT THE SECOND PUSH with the SA-10 lead close + the truth batches + the fix lanes)

**Audited: core `84a1832`** (the verification baseline; the concurrent lanes' landings
reconciled in `evidence/s10-verification-pass-b.md`'s superseded-verdict table) ·
**corpus `be0cfe9`**. Convergent with the parallel verification sweep
(`s10-reverification-sweep.md`, filed first — the 69 STILL-ACTIVE verdicts agree; this
lane's evidence keeps only the unique content). **Renumbered twice at push** (the
collision convention, earlier remote numbering wins): the lane's M-103 → M-105 →
**M-108** (the parallel lanes consumed M-103 lostdir / M-104 wholedisk / M-105
DRWService / M-106 ONE-3DS / M-107 batch-1-stand-in) and N-128 → **N-132** (the SA-10
lead's capability-matrix row owns N-128); N-129..N-131, OQ-S10-2 and E69 were free.
**CONVERGENT repair:** the S-050→S-051 collision resolution (this lane and the remote
SA-10 close made the same repair independently).

**This lane's UNIQUE product on the merged board:**
- **M-100 → RESOLVED (closed-as-covered)** — the one board move every parallel lane
  missed: the archive browse-inside capability landed in `6d0f16c` (M-066+N-088)
  BEFORE the finding's own baseline; the filing's central "absent" grep was false.
  Retired per the purge convention (FIXED-LEDGER row 279; the narrative in
  `evidence/s10-verification-pass-b.md`).
- **M-108** (MAJOR — EXECUTED): the text-preview eligibility model — the og gates by
  the 119-ext TxtViewFiles extension whitelist (decompile-verified first-hand:
  DRWUI:535858/:535937); the core content-sniffs and refuses UTF-8-BOM/UTF-16/
  Latin-1 text while admitting printable-head binaries
  (`evidence/executions/s10-preview-text-eligibility.md`).
- **N-132** (MINOR): API-DELTA rows 44-45 falsely claim TxtViewFiles/SumaryFileType/
  PriorityDisplayType "landed" — all three absent, contradicting the truthful R-31.
- **N-129→N-135** (MINOR): six register count/cite defects from the 1470-file re-count (renumbered S11: the ID collided with the wave-3 lane's earlier-pushed bookkeeping N-129).
- **N-130** (MINOR) + **OQ-S10-2** (tracked-open): the og-tree completeness sweep's
  two undispositioned files (the Fixo VideoFileRepairer.exe engine, registered
  SIDECAR-OOS; the opaque sfc payload, the Ghidra magic-search queued).
- **N-131** (MINOR) + **E69** + the field-parity table: the first per-field
  enumeration of the og's persisted node schema vs cr_records.cpp — every
  load-bearing family MATCH-with-re-encoding; 13 SUPERSET fields; the parent-anchor
  ABSENT class filed (`evidence/s10-field-parity-table.md`).
- Executed ground-truth: the three REAL SaveScan .rsf files parsed first-hand — the
  corpus container grammar byte-confirmed, no erratum
  (`evidence/executions/s10-rsf-ground-truth.md`).
- Historical records preserved in the lane-B evidence (findings retired by the
  parallel purge mid-flight): the M-071 git-archaeology completion, the M-050
  wrapper-scalar completeness enumeration, this lane's S-041/N-101/N-106/N-120
  addenda (those findings' resolutions landed with the concurrent lanes — N-101
  FIXED, N-106 FIXED-ALREADY, N-120's residues recorded in the batch-C truth commit,
  S-041 closed-as-umbrella with the children owning the remainders: 8 child findings
  still active after the M-092 HTML landing — M-093/M-096/M-098/M-099/N-111/N-112/
  N-113/N-115).

**Board after the lane-B push: 54 active** (6 SEVERE · 24 MAJOR (-M-100 +M-108) ·
**19 MINOR** (+N-129..N-132) · 5 INFO) · **281 resolved** (FIXED-LEDGER — 280 rows at the final rebase point + M-100 this push) ·
**4 tracked-open** (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2). **Recommended fix order
contribution:** M-108 first (the BOM head-gate fix is one branch; user-visible on the
most common Windows text shapes), then the N-129/N-132 gate-doc mechanical pass, then
N-131 + E69's serializer trace (one session closes the session-schema story), beside
the standing order (S-051/S-052, M-104..M-107, the carve children).


## SA-10 delta (2026-10-29 — the lead lane: board triage at 84a1832, then the push-time reconciliation over FIVE concurrent lanes)

**Audited: `84a1832` at open (75 active / 250 FIXED / 2 tracked-open); reconciled at the `ea1ac86` lineage after the hottest parallel window of the trail** — 42+ commits landed mid-session (the closing-fix lanes' S-042/S-044/M-071/M-090/M-084/M-091/M-080/M-085/S-043/M-082/M-088/N-092/N-094/N-116/N-124/N-103/N-117/N-118 landings; SA-10B's real-rsf mining + the user-mandated FIXED purge + the FIXED-LEDGER go-forward convention; the wave-2 lane's whole-disk scan-flow findings; the session-38 doc-truth batches). Every lane reconciled per the SOP: no duplication, extend-don't-refile, the earlier remote numbering wins collisions.

**This lane's work (the mandate's three asks):**
1. **Triage ("remove the fixed things")** — all 75 active findings re-verified at the base (LANE-A: 51 STILL-DEFECTIVE · 4 PARTIAL · 5 INFO accurate; 0 falsely-active), then the mid-flight landings re-triaged at the reconciliation HEAD. **Flipped by this lane: S-041** (FIXED-as-umbrella per the S-007 precedent — school 1 landed, DMG/MDB resolved mid-flight, every still-open school-2/3 format child-filed, the walk follow-ups owned by M-106) **and N-101** (every fix-plan step verified executed in-tree) — both retired to history with Resolution records + FIXED-LEDGER rows, along with the three pending-retire files the ledger itself asked the next audit lane to conform (S-043/M-080/M-085).
2. **Go deeper** — the named NEVER-AUDITED residual closed: **M-105** (the DRWService install lifecycle — the full K71-K73 parameter decode: "Read FVE Server" AUTO_START/LocalSystem/OWN_PROCESS, /install replace-on-install + StartServiceA, the stop-gated delete, DRWUI's per-use uninstall→install→harvest→2s→uninstall orchestration; the core-side absence grep-verified; R-83's false disposition note owned). **S-052** (the LDM span recovery misbase — the recovery-side half of B8/A27 never scored: layout-relative extents vs the single-disk SafeWriter, the scan's virtual device dies at scan end, no refusal path, the R7.2/F12 overlap check on the wrong basis; GAP-S6-15's audit-board owner).
3. **Mark all remaining** — the recorded-but-unfiled follow-ups swept and owned: **M-106** (the ONE/3DS size-walks — DECODED in the trail's own evidence but shipped as 64 MiB stand-ins; the M-090/M-091 truncation class), **M-107** (the batch-1 stand-in residue — ten sites' "recorded follow-up" comments, S-007's promised follow-ups, the X3F grammar corroborated by the never-cited FileRepair artifact), **N-128** (CAPABILITY-MATRIX's doubly-untrue prebuilt-CI row — dead workflow cites + the unowned ext4/HFS+/Btrfs/ZFS/ReFS staging follow-up buried in a DONE verdict).

**The ID-collision repair (process finding):** the wave-2 lane's S-050 (whole-disk scan targets) collided with SA-10B's S-050 (file-repair line) — both live on main. Per the SA-9 rule the earlier push (SA-10B's) keeps the ID; the wave-2 file renumbers to **S-051** with its orchestration/CROSS-REF/SUMMARY refs updated and a renumber note in the file.

**Convergence merges (no duplication):** this lane's M-100 re-scope and N-099 addendum were independently landed by the session-38 sweep (c75755d) — the lane's versions dropped (batch C then retired both findings whole, `32ef33b` — N-135: the original cite `13245c0` was a pre-rebase phantom, re-pinned; this lane's E69 erratum candidate — the retired S-007 file's own stale :215/:222 cites — is recorded here and in the worklog for the next corpus-erratus pass); the M-079/N-090 cite-refresh notes ride as dated addenda where still live.

**Gate-doc truth repairs (this lane):** SURFACE-INVENTORY row 67's pre-reconciliation IDs (the fourth and last stale-numbering site), the DRWService rows 43/95 (install lifecycle → M-105), driver-install.md's S-038-stays-ACTIVE rows (K74/K77 flipped to the shipped state) + the session-9 addendum's M-071/M-072 renumber annotations, carve.md's landed-format/M-081/S-041 currency annotations, the FIXED-LEDGER backfill (M-081's row + the five retirements + counts 265→270).

**Board after this lane (at its final push, post-rebase over the S-047 `a3abfbe` / S-045-half `021ae07` + OQ-S10-1 / N-095+N-108+N-119 `995ba5d` / batch-C `32ef33b`(N-135: the recorded `13245c0` was a pre-rebase phantom)+`0872ac2` landings — N-104/N-097/N-102/N-123/N-101/N-120 fixed + N-099/N-106 FIXED-ALREADY, all convergent with this lane's triage): 54 active (6 SEVERE · 25 MAJOR · 18 MINOR · 5 INFO) / 277 resolved-in-history (FIXED-LEDGER.md) / 3 tracked-open (OQ-S4-3 execution-bound; OQ-S8-1 the CWindowsFS consumer; **OQ-S10-1** S-045's codec-bound thumbnail halves with the named vendoring dependencies). Next free IDs: S-053+, M-108+, N-129+, I-031+, A-012+.**

**Recommended fix-order contributions:** (1) **S-052** — silently wrong recovery bytes on first-class LDM surfaces; the basis-descriptor fix is architectural but the multi-disk fixtures already exist (test_ldm.cpp:206+); (2) **S-051 + M-104** (the wave-2 lane's whole-disk pair) — rc=0/0-files on healthy disks is the worst first-run experience in the board; (3) **M-106** — mechanical against the existing decodes; (4) the carve school-2/3 children in SA-10B's real-world order (GZ/SVG/PNG first); (5) M-105's posture decision rides the driver-contract session (M-086/M-087 residuals + N-107/N-127).

---

## SA-10B delta (2026-10-29 — the B lane: real-.rsf session mining, the og data-table parity, the product-frame census, the FIXED purge)

## SA-10B delta (2026-10-29 — the B lane: real-.rsf session mining, the og data-table parity, the product-frame census, the FIXED purge)

**Audited: origin/main `62074bf`-lineage (product-code tip `26da616`+), corpus
`be0cfe9` (unmoved).** The user's session-2 instruction — "look for what
session 1 missed... remove the fixed things... go more deep, do more
comparison on og-codebase and our core and mark all remaining features,
implementations, functions, as finding too" — decomposed into five lanes
(L-A..L-E, worklog SA-10B/T1). The parallel-lane environment stayed HOT the
whole session (the S-042/S-044/M-071/M-090/N-094/M-084/M-091/N-092/N-124 fix
landings + the SA-10/S10-F/DFX-38 audit lanes all landed mid-flight; this lane
reconciled at every push).

**L-A — the FIXED purge (user-mandated): EXECUTED.** The 259 resolved findings
(250 at the session-9 truth pass + the 9 mid-flight fix-lane landings) are
**retired to git history** — `findings/FIXED/` no longer exists on main
(purge commit `530fa32`); `FIXED-LEDGER.md` is the navigable index (ID /
severity / fix-commit / history-path per row). Nothing is lost: every file
is one `git log --follow` away, and the active board is now the only thing
in the findings tree.

**L-B — the real-.rsf session mining (the session's headline): 3 new
findings, all executed-evidence class (§6.1).** The corpus's three REAL og
production sessions (`og-codebase/SaveScan/auto_*.rsf`, ~2M rows, 292 MiB of
record-4 payload, parsed with the core's own compare_og_rsf.py — clean to the
last byte) were mined for tree-shape and naming ground truth no prior session
had extracted:
- **S-048 (SEVERE)** — the og's carve branch is `Reconstructed/<volume-display-name>/`
  (per-volume children; 100% of 189,115 carved rows across all three sessions);
  the core has ONE flat "Other Deleted Files (Carved)" root, no volume
  attribution. DRWUI:320992/:357713/:611618/:642786 corroborate.
- **S-049 (SEVERE)** — the og's universal carved-file naming contract decoded:
  `<PREFIX>_%06d.EXT` (DRWUI FUN_140c44770:1878379, `%s_%06d` + the >99999
  `%s_%d` fallback), prefix = uppercase EXT by default, overridden by
  content-derived strings: SOF dimensions, EXIF Model/Software (JPG), ID3
  title (MP3), creation-timestamp strings (PDF/DOCX), ZIP first central-directory
  entries. The core's `File%06u` default fires for every format except MP4/MOV;
  **N-063's recorded remainder is resolved** (its "rule not decoded" registration
  falsified by real data).
- **M-103 (MAJOR)** — the og's orphan branch groups per-parent-record into
  `LostDir<NNNN>` synthetic folders under "File Path Lost" (NTFSSearchDLL:42041
  `LostDir%d`; 398 LostDirs / 938,982 rows = 62.6% of the H session — the
  post-format salvage majority); the core reparents flat under "Lost Files",
  destroying sibling grouping. SCAN-ENGINE-DELTA C2's "roles map 1:1" claim is
  falsified for both branches by the real payloads.
- Corroborations recorded in the evidence file: S-031 (every real carved row
  deleted-flagged) and S-041's school-2/3 real-world weight (GZ/SVG/PNG are
  the og's top-3 carve formats by volume — HTML/SVG the #2/#4).

**L-C — og shipped-data-table parity: MATCH (executed).** The raw_signs.json
regen sync ran (scripts/gen_rawsigns.py): 139 formats / 199 header / 62 tail
signs **byte-identical** to the committed table — the only diff is the
clang-format 2-per-line packing (a hygiene note, not a defect). EUUserData,
config/*.ini, Dict: every row verified dispositioned (F2/D-104, M-023/M-073,
I-027, N-064) — the census is CLEAN.

**L-D — the product-frame census: 1 new finding.** Every og product-frame
binary was mapped to its trail disposition (viewers → PARITY-OG-CORE-INVENTORY
UI-layer OOS; iCloud → EXCLUDED-NAS; runtime families → registered). One
surface failed the census:
- **S-050 (SEVERE)** — the og's **file-repair line** (photo/video/office/HWP/
  raw/archive repair: 9+ Fixo binaries, 8 never-cited corpus artifacts, the
  shipped Dict pairing, remote-config-flagged `enable_imgrepair`/
  `enable_docrepair`, wired into DRWUI's CHK/ln chain) has no core
  implementation AND no truthful registration — the SURFACE-INVENTORY row
  cites a CAPABILITY-MATRIX §3 that does not contain it (§3 lists only
  Cloud/NAS/BitLocker-derivation/signing/.rsf-import). Filed per the user's
  mark-remaining instruction; the registration drift is the finding's own
  second defect.

**Board after this lane's waves (at its final push): 70 active (8 SEVERE ·
25 MAJOR · 32 MINOR · 5 INFO incl. the 4 new: S-048/S-049/S-050 + M-103) /
259 FIXED-in-history (see FIXED-LEDGER.md) / tracked-open unchanged.** The
parallel fix lanes keep landing (S-043, S-047, the S-041 children, M-072..M-078
claimed by their territory commits) — pull before reading counts.

---

## (the prior executive summary continues below — the closing-fix wave and earlier sessions)

> The prior H1 read: "executive summary (the combined closing-fix lanes on
> top: session-38's minor-first wave, the parallel S-042/M-071 fix lanes, the
> session-8/9 audit lanes below)" — superseded by the SA-10B header above;
> its content blocks are unchanged below.
# SUMMARY.md — executive summary (the combined closing-fix lanes on top: session-38's minor-first wave, the parallel S-042/M-071 fix lanes, the session-8/9 audit lanes below)
## Session-38 reconciliation delta (2026-10-29 — this lane's M-071 convergence + the rrecov-gates execution proof the landed fix lacked)

This lane verified M-071 independently at `84a1832` (same census as the
landed fix: only cr_query.cpp:119 gated; both rrecov walks gate-less;
`git show 69507cb` for the original form; corpus FilterFolder.ini re-read)
and had prepared the identical two-gate restoration when the parallel
fix-lane's `ce2405a` landed first — no duplicate code kept (the
convergence precedent); the landed fix re-verified first-hand at the
reconciled main. This lane's surviving contribution, per the landed
Resolution's own named gap ("the rrecov.cpp pair cannot execute on
Linux"): **the Windows-CI behavioral execution of the two rrecov.cpp
gates** (new checks.yml step `CLI junk-filter contract (M-071 - tree +
recover-all)` — scan/tree/recover over a committed junk-carrying fixture
via the driverless `--image` path), **the file-backed fixture + generator**
(`tools/gen_junk_gate_fixture.py` + `tests/data/images/junk-gate-fat16.raw`,
SHA256SUMS-pinned; the generator's two bring-up slips caught by the test's
own assertions), and **the Linux gtest over the shipped file**
(`FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet` — flags exactly
4 junk nodes, query-gates them, TRACKER.TXT stays a result: the per-node
flag / subtree-skip two-surface semantic pinned on the CI step's
substrate) + the core-side explicit-selection pin
(`SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode`).

Trail repairs in the same landing: modules/driver-install.md's session-9
row mislabeling the driver FileVersion finding (M-086) as "M-071"
corrected; the ci-local.sh/ps1 commit-scope regex widened to the de-facto
convention (259 origin/main commits carry uppercase scopes — the regex was
the outlier, not the commits); docs/PACKAGING.md's S-038 link repointed to
FIXED/ (the PARITY-FIX-9 move, found by the ci-local doc-link check).
Board unchanged by this lane's landing (M-071 was already moved by
`ce2405a`; this lane adds proof, not a count change): **71 active** (6
SEVERE · 26 MAJOR · 34 MINOR · 5 INFO) · 254 FIXED · OQ-S4-3/OQ-S8-1
tracked-open.

**M-079 FIXED by the parallel fix-lane** (`0b76757` — the CLI-side
fill_volume_label: --label override > the --volume target's
GetVolumeInformationW > the image filename stem, at the finding's
recommended seam), with this lane's CONVERGENCE complement: the
Windows-CI junk-gate step now proves the landed fill end-to-end
(auto_junk-gate-fat16<stamp> from --autosave-dir over the committed
fixture; auto_CUSTOM<stamp> with --label) — the behavioral execution
the landed fix's source-truth pin does not give; plus the exFAT fixture
label-entry spec-repair (+2 → +4, spec 7.2) and the resume-path label
adoption (the SESS-2 card completion). This lane's alternative
orchestrator-side FS-derivation draft (FAT dentry/BPB, exFAT 0x83, NTFS
$VOLUME_NAME) was reverted per the convergence precedent — recorded in
worklog S38-M079-CONVERGE as a future deliberate-divergence option
(FS-truth labels on raw image scans).
Board: **70 active** (6 SEVERE · 25 MAJOR · 34
MINOR · 5 INFO) · the FIXED count moves in the ledger.


## Closing-fix wave delta (2026-10-28/29 — the fix mandate over the 75-active board, three concurrent lanes reconciled)

**Opening baseline: `84a1832`-lineage (product tip `b92ebef`), corpus `be0cfe9`; board 75
active (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) / 250 FIXED / 2 tracked-open; suite
1023/1023 linux-release green at every lane's open.** Scope (all lanes): the two-repo
grant (core + corpus only) — the driver-repo findings get dispositioned/pushed forward,
not driver-repo edits. Session-38 lane: MINOR tier first per its user directive; the
M-071/S-042 lanes: regression + SEVERE tier first.

**Fixed in this window (4, by lane):**
- **N-094** (MINOR → FIXED, session-38 lane `26da616`) — the `-h` usage text omitted
  the `sessions` verb; usage line + CORE-API-CURRENT count repair + the verb lock
  grown to four sites with a both-directions regression pin (suite 1024/1024).
- **S-042** (SEVERE → FIXED, parallel lane `f6e303e`) — the WMF placeable+standard
  validators re-derived from the og grammar (FUN_1800264d0): the disjoint
  accept-sets closed (checksum word@+20 == XOR words 0..9, zero dword@+16, tail
  word, exact size std_words*2+22).
- **M-071** (MAJOR → FIXED, closing-fix lane `ce2405a` — N-135 item 3: the
  original block cited `dd0ffd5`, a pre-rebase phantom absent from the object
  store; re-pinned) — the M-018 regression:
- **M-071** (MAJOR → FIXED, closing-fix lane `ce2405a` — the `dd0ffd5` token was a pre-rebase phantom, repaired session-42) — the M-018 regression:
  both rrecov.cpp gates restored byte-exact from the `69507cb` form; the cr_ui
  session/snapshot bulk walks (the W-lane surfaces that postdate M-018 and never
  carried the gate) gated identically; the explicit-selection semantic (junk stays
  recoverable when individually selected — the og's filter is a results-set
  default, not a hard block) pinned by `CrUi.RecoverExplicitSelectionStillAllowsJunk`;
  `SafeWriter::enqueue` stays un-gated by that same decision (the finding's open
  question settled). Proven fail-before/fail-after at the ABI surface (2 RED without
  the gate, 3 green with). Suite 1026/1026.
- **S-043** (SEVERE → FIXED, TWO LANDES reconciled) — the thumbnail EXIF
  orientation: the parallel lane's base (`d0787e9`: the walker orientation read
  before decode, the 8-case `apply_exif_orientation`, the `exif_orient{1..9}.jpg`
  battery) + the closing-fix lane's riders rebased on top: TIFF tag 274 (the GDI+
  0x112 datum for TIFF inputs) and the embedded fast path's D-105 rule made
  precise (serves as-stored per CIPA DC-008; honors an IFD1-DECLARED 0x112 as
  the not-already-oriented marker — the finding's own fix-plan conditional).
  Riders proven fail-before on the base alone; ASan-clean thumbnail suite;
  registered PARITY-DELTAS D-105; the finding's merged Resolution retired to
  history with its FIXED-LEDGER row (the retire also swept M-080's re-created
  FIXED/ file — both lanes predated the convention's readability).

- **M-088** (MAJOR → FIXED, DFX-38 lane) — the FloorVersion pin split closed:
  installer.yml bumped to v0.2.4 (both live pins equal, the M-061 teardown
  contract's own floor); CI-CURRENT/PREBUILT-ARTIFACTS/README/RELEASE-NOTES/
  CI-SCENARIOS/CI-LOCAL/DRIVER-RELEASE/DRIVER-CONTRACT rewritten to the
  4-lane truth; the same-class sweep went BEYOND the finding's census (the
  gate found 4 more stale sites: DRIVER-UTILIZATION ×2, PERFORMANCE-BASELINE,
  benchmarking.md, PLAN-CI-REBUILD); and `tools/doc_truth_gate.py` wired
  into checks.yml's lint job enforces both invariants forever (first run:
  8 named violations — the fail-before; green after).

- **M-082** (MAJOR → FIXED, DFX-38 lane `1f714884` — N-135 item 3: the
  original block cited `95122bd`, a pre-rebase phantom; re-pinned) — the DeviceManager
- **M-082** (MAJOR → FIXED, DFX-38 lane `1f714884` — the `95122bd` token was a pre-rebase phantom, repaired session-42) — the DeviceManager
  offline-OS profile landed corpus-exact: reghive::offlineos::profile_os()
  (the 8 CurrentVersion fields, the drive-letter SystemRoot gate, the
  12-byte MountedDevices system-partition location, the Select-aware
  ControlSet resolution, the ProductOptions classification — including
  the first-hand %03d format decode from DeviceManager.mo .rdata, which
  the test's RED forced and the binary settled). Core-only C++ API per
  the fix plan's staging; PARITY-DELTAS D-107 records the decision +
  the boot-fix auto-locate follow-up consumer; mom-capabilities.md
  re-scoped (the dead-shipped verdict covers only the CheckVersion
  copy). 5-test proof; suite 1045/1045.

**Board after this window (post-purge filesystem truth):** 68 active (8
SEVERE — incl. the session-10 lanes' new S-048/S-049/S-050 filings · 23
MAJOR · 32 MINOR · 5 INFO) / **262 resolved** (FIXED-LEDGER.md is the
index; the per-finding files are retired to git history per the SA-10B
purge) / 2 tracked-open (OQ-S4-3, OQ-S8-1).
- **N-116** (MINOR → resolved, session-38 lane, this replay) — the session-list
  row ordering: rows newest-first by save_unix (FUN_140273550 settled first-hand:
  !(ftA < ftB) = newest-first), path-ascending tie-break; the TRUNCATED answer is
  the time-ordered prefix; the CLI `sessions` verb re-sorts by the same key;
  `SessionListOgTimeOrdering` RED→GREEN; suite 1041/1041.

**Board after this window (filesystem truth, batches A+B+C+D+N-100 folded
in):** 46 active (6 SEVERE · 22 MAJOR · 13 MINOR · 5 INFO — the window's parallel landings
M-080/S-043/M-084/M-091/M-082 + the SA-10B new filings S-048..S-050/M-103 all
reconciled in the counts above; the purge-era rows carry their own snapshots) /
**276 resolved** (FIXED-LEDGER.md is the index) / 2 tracked-open (OQ-S4-3, OQ-S8-1).

**Session-38's own tally at this point:** 22 findings closed (N-094 + its
CORE-API-CURRENT residual, N-092, N-116, N-117, N-118, N-103, N-095, N-108, N-119,
N-104, N-097, N-102, N-123, N-101, N-120, N-091, N-098, and the FIXED-ALREADY
classifications N-099 + N-106 + N-105 with reproductions) — every one re-verified against the current tree
and the corpus before the fix, the code fixes proven RED→GREEN with named tests,
the doc fixes site-verified before/after, every Resolution recorded, the ledger
conformed per the purge convention.

**Board after this window (filesystem truth, all lanes reconciled):** 66
active (7 SEVERE · 23 MAJOR · 31 MINOR · 5 INFO) / **264 resolved**
(FIXED-LEDGER.md + history) / 2 tracked-open (OQ-S4-3, OQ-S8-1).

## Fix session (2026-10-29 — the parity close: every tier executed against corpus `be0cfe9`, core `84a1832`-lineage)

The execution of the roadmap this directory is: findings re-verified first-hand
against the corpus before every fix, one finding per commit, every fix proven
by tests that fail before and pass after. Board movements are recorded per-fix
in each finding's Resolution; the running counts live in this section.

**Board at session open:** 75 active (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) + 250 FIXED + OQ-S4-3/OQ-S8-1 tracked-open.

**Board after S-042:** 74 active (6 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) + 251 FIXED. S-042 (WMF placeable grammar) closed with both WMF branches re-derived from FUN_1800264d0 and spec-regenerated fixtures — see findings/FIXED/S-042.

**Board after S-044:** 73 active (5 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) + 252 FIXED. S-044 (auto-save gates) closed: the og's three trigger-site gates (results-found / not-import / not-already-saved) landed on the ABI completion window and the CLI twin; config-shape delta registered (D-105); Folder-suffix deferral registered (D-106). Tests: CrUi.AutoSaveSkipsEmptyResults / AutoSaveSkipsAfterExplicitSave / AutoSaveSkipsResumedRun, TSan-clean. See findings/FIXED/S-044.

**Board after S-047:** 72 active (4 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) + 253 FIXED. S-047 (MPEG-TS/M2TS) closed: FMT_TS/FMT_M2TS sync scanners landed on the keyed dispatch (1-byte 0x47 magic, two anchor offsets), the og's full packet-walk grammar, suppress_overlap, the 2 KiB progressive first block for noise cost. Tests: Carve.MpegTs188PacketStreamAccepted / M2Ts192PacketStreamAccepted / TsBelowMinimumExtentRejected / TsSyncLossCommitsRun / TsNullPacketTolerated / TsSparseSyncNoiseRejected; ASan+UBSan clean. See findings/FIXED/S-047.

**Board after M-092:** **71 active (4 SEVERE · 27 MAJOR · 35 MINOR · 5 INFO) + 254 FIXED** (counts also reflect the parallel lanes' same-window landings: M-084 ACCDB, M-085 TIFF LZW, M-088, M-090, M-091, N-093, N-094, N-103, N-117, N-118 by their lanes; S-045 partial → OQ-S10-1; new S-048/S-049/S-050 filed by the audit lanes — see the board files for the live truth). M-092 (HTML carve) closed: the four u16-key rows (<h/<H/<!/FF FE — census erratum E59's BOM chain) + the case-insensitive probes + the ASCII-only </html> walk (no wide needle exists in the DLL — the plan's parenthetical corrected) with sector round-up and the 1 MiB bounded fallback. Tests: Carve.Html* (4); the consistency vocabulary updated per the OQ-FLT-1 registry-derived design. See findings/FIXED/M-092.

(Counts in this section update as the session progresses; the per-severity deltas below in the historical sections are frozen history.)

# (the session-10 continuation-audit lane's section follows the fix-lane record above)

## Session-10 delta (2026-10-28 evening — the SECOND-CONVERSATION-SESSION continuation: re-verify the board, remove the fixed, then go deeper than the artifacts went)

**Audited: core `84a1832` (= origin/main, verified 0/0 post-fetch — the session-9 close), corpus `be0cfe9` (unchanged).** User directive: look for what session 1 missed; REMOVE THE FIXED THINGS (re-verify the active board vs HEAD); go deeper — more og-codebase ↔ core comparison; mark ALL remaining features/implementations/functions as findings. Scope pin per this session's prompt: TWO repositories (core + corpus); the six driver-repo findings (M-086/M-087/M-089, N-107, N-127, M-088's driver half) stand as filed at driver `323e75a` — not re-verified this session; M-088's core-side half WAS verified (STILL-ACTIVE, untouched).

**Wave 1 — the re-verification sweep (T2-T4):** four parallel agents re-verified all 76 actives against HEAD first-hand (every core-reference re-read at the current tree; the lead spot-verified the load-bearing verdicts). Full table: `evidence/s10-reverification-sweep.md`. Outcome: **74 of 75 in-scope findings STILL-ACTIVE; 1 FIXED (N-124 — the transport.md SUPERSEDED annotation, landed `2025554`); 1 re-scoped (M-100 — its "grep: absent" core-ref was FALSE AT FILING: `6d0f16c`/M-066 landed the engine-half enumeration pre-filing; the residual is the ABI/CLI exposure only)**. Five addenda filed with new defects the sweep caught: M-079 (a NEW false comment at cr_scan.h:236-239 claiming the CLI/win layer fills volume_label — no code does), N-094 (the verb-count drift compounded: CORE-API-CURRENT.md carries THREE contradictory counts 9/10/11), N-102 (a THIRD uncorrected "37 toggles" instance at cr_carve.h:88 + the finding's own cited M-024 filename never existed), N-099 (erratum E51 still carries the wrong MDB ctor cite), M-100 (the R-37 rows now doubly stale). Line-number re-pins recorded in the sweep evidence file (~+55-60 lines in cr_analysers.cpp from the b92ebef insertions).

**Board after wave 1 (at my HEAD `84a1832`): 75 active (7 SEVERE · 28 MAJOR · 34 MINOR · 5 INFO) / 251 FIXED / 1 tracked-open.**

**Wave 2 — the og-codebase deep-dive (T5-T9, first findings):** the lane's
declared complement to SA-10B (data-tables/product-frame/real-rsf mining)
and S10-F (function inventories): the WHOLE-DISK SCAN FLOW, executed
end-to-end on a controlled two-volume fixture vs the og's own persisted
real sessions. Filed **S-051** (SEVERE — renumbered from the lane's S-050 filing at the SA-10 reconciliation: SA-10B's file-repair S-050 was pushed first and keeps the ID; the plain disk target
`cr_ui_open_disk`/`scan --disk N` silently returns rc=0 with ZERO files on
a healthy partitioned disk: the probe reads the MBR, dispatches Unknown,
"quick yields nothing"; the og's CDoScanPartition reads the partition list
and scans every known volume — architecture/04 §2/§3.4 + the real G
session's both-volumes tree) and **M-104** (MAJOR — with `--whole-disk`,
the in-table volumes are rescued only by the discovery's +500 table-match
candidates as anonymous `Candidate @ <LBA>` roots: no display names, no
FS-type markers, known-vs-lost indistinguishable, Found/Deleted empty; the
og's real-session per-volume shape unreachable). Evidence class: EXECUTED
(evidence/executions/s10-wholedisk-scan-targets.md — the fixture builder +
probe committed beside it; the G session re-parsed first-hand this lane).
Board after wave 2: **77 active (8 SEVERE · 29 MAJOR · 34 MINOR · 5 INFO)
/ 255 FIXED-files / 2 tracked-open.**

**Wave 3 — the fix-wave landing-parity verification (the second half of the
user's remove-the-fixed mandate: verify the fixed):** three parallel agents
independently verified nine fresh landings (S-044, N-116, M-080, M-085,
S-042, M-090, M-082, M-071, N-117/N-118/N-103) against their findings'
og-decodes, the corpus/decompile, and the current tree — every
load-bearing claim re-verified by the lead. **Eight PARITY-VERIFIED** (S-044's
three og gates on both surfaces; N-116's newest-first order with the og
comparator re-read; M-085's early-change LZW independently reproduced
against a from-scratch reference; S-042 gate-for-gate vs FUN_1800264d0;
M-090's koly walk re-derived term-for-term with the ctor's own 0xC800000
cap; M-082's CPartitionBootInfo walk corpus-exact incl. the %03d re-check
against the .mo binary; M-071's gates byte-exact to 69507cb; N-117's
cites). **Two residuals filed: M-108** (the M-080 landing stopped at
32 MiB — the og's 200 MB anchor not adopted, the 32–200 MB band refused,
the test still citing the refuted no-cap doctrine) and **N-135** (the
bookkeeping truth batch — renumbered N-128→N-129 at the wave-3 push, then
N-129→N-135 at session-11B: the renumber had landed on lane-B's occupied ID; N-103's cited :376 site missed while its
Resolution claims both fixed; the gap-board's :379/:225 self-contradiction;
five phantom pre-rebase SHAs in landing records; the FIXED-LEDGER's own
gaps). Full-suite 1005/1005 at HEAD (independent run). Evidence:
evidence/s10-landing-parity-verification.md.

Board after wave 3 (this lane's ledger; the parallel lanes' concurrent
movements reconcile at push): +2 active from this wave's filings, minus
the lanes' concurrent fixes — see the FINAL-COUNTS section at the worklog
close for the push-time truth.

**Push-time reconciliation (the moving-target SOP executing):** while this
sweep ran, four concurrent fix lanes landed `26da616` (N-094 — the `-h`
usage line + the verb lock's fourth site), `f6e303e` (S-042 — the WMF
validators re-derived from FUN_1800264d0, disjoint accept-sets closed),
`ce2405a` (M-071 — the M-018 junk-gate regression closed; N-135: the recorded `dd0ffd5` twin was a pre-rebase phantom), both
rrecov.cpp gates restored + the cr_ui bulk walks gated + the
explicit-selection semantic pinned), and `9daa787`+`b500058` (M-090 — the
DMG koly trailer walk + the ctor's own 0xC800000 cap; the finding moved to
FIXED with the batch-1 mis-decode claims corrected). My sweep's
STILL-ACTIVE verdicts for those four stand as the record at `84a1832` and
are superseded at the rebase tip. **Board at the rebase tip: 70 active
(6 SEVERE · 26 MAJOR · 33 MINOR · 5 INFO) / 255 FIXED / 1 tracked-open
(OQ-S4-3; the remote lanes also carry OQ-S8-1).** The fix lanes' own
SUMMARY sections (above) own the per-fix resolution records; this lane's
concurrent-verdict record is preserved in evidence/s10-reverification-sweep.md
with a reconciliation note added at its head.

## Session-8 delta — third lane (2026-10-28 — the DRIVER lane + the duplicate merges + the board-truth repairs)

**Audited: core `35adcb9` (rebased over the parallel lanes through `9e5dedc`), driver `323e75a` (v0.2.4), corpus `be0cfe9`.** Method: five parallel read agents (corpus contracts for the new landings; corpus coverage/citation sweep; new-landing parity verification; board-truth sweep; driver+RE-tooling audit), then first-hand re-verification of every load-bearing claim by the lead before filing — including a personal re-read of both WMF decompile sites and the core validator, and byte-level decodes of regdrv-rda.reg and rda_version.h.

**Scope determination (from the trail state, T1):** the corpus delta since `03ad46c` is exactly one errata line (#9, already absorbed: E-A/E-B + D-104); the board read 0 active; the audit surface = (a) the post-dev-sign landings' parity claims, (b) the S-007 batch-2 remainder recorded outside an active finding, (c) the driver-lane handoffs D2/D3/D4 (no other session-8 lane took them — two lanes explicitly declared driver out of scope), (d) gate-doc currency.

**Filed this lane — the driver lane (M-086..M-089, N-107):** **M-086** (MAJOR — the FileVersion 1.0.0.0 pin + no release stamping defeats `-Layout Versioned`: every v0.2.x release derives tag "10" → genuine upgrades hit exit 2; the M-064 contract's version source starves it), **M-087** (MAJOR — regdrv-rda.reg:31 UpperFilters `hex(7):r,d,a,\,0` is 5 ASCII bytes, not a UTF-16LE multi-string; the WinPE disk-class filter never attaches; the og's regdrv.reg:35-36 shows the correct form — the D4 handoff owned), **M-088** (MAJOR — FloorVersion pinned at two sites at two floors (release.yml:291 v0.2.4 vs installer.yml:118 v0.2.3) while CI-CURRENT.md:59-61 asserts ONE pin source; plus the CI-shape doc cluster: README:125-132's wholly stale CI paragraph, the 3-lane description omitting installer.yml, PREBUILT-ARTIFACTS.md §2/§5 citing the deleted composite action + three deleted workflows, same-class sites in DRIVER-RELEASE/DRIVER-CONTRACT/CI-LOCAL/CI-SCENARIOS/RELEASE-NOTES), **M-089** (MAJOR — rdabench.c:390 probes the phantom-zero 0x6000244x codes; the live extended IOCTL handlers 0x6002440/44/48 get zero fuzz coverage), **N-107** (MINOR — the driver repo's 14 current-tense "15/18-command" sites vs the code's 19; the D3 handoff's full inventory). Also **N-108** (MINOR — docs/PARAMETERS.md:78-79 still carry the pre-fix constants (4-8 MiB / 8 signs / 16 bytes / offset 512) vs the code's 1 MiB+4 KiB / 12 / 48 / 1024 / 80; the SCAN-ENGINE-DELTA "fixed this session" claim proven FALSE by git log and corrected in place).

**Convergence merges (no duplication policy):** this lane's S-040/S-041 (WMF validator contradiction; batch-2 remainder) were filed in the same window as the continuation lane's S-042/S-041 — both merged into the earlier landings as addenda (two lanes, two decompilers, one conclusion on the WMF grammar: the og's checksum word sits at +20 over words 0..9 with a zero dword at +16, disjoint from the core's inch-gate/checksum@+18 convention; the batch-2 CHM/FH11 asm decode donated to S-041 with the census-:99 and retdec-truncation errata E65/E66). The M-081 fix landing (`9e5dedc`, the CRW/MRW + HLP/doc re-attributions) was verified first-hand in-source — the misattributions are gone, FMT_CRW/FMT_HLP3 carry the og grammars.

**Board-truth repairs:** the A-007 finding's dangling evidence cites (a006-… never existed) re-pointed at the real file with a dated note; SCAN-ENGINE-DELTA's two false "fixed" claims corrected in place; corpus-errata E65/E66 filed; CROSS-REF-MATRIX gained this lane's rows.

**Board after the third lane (all session-8 legs combined):** **47 active** (6 SEVERE · **19 MAJOR** (+M-086..M-089) · **19 MINOR** (+N-107, N-108) · I-026/I-027/I-028) + 249 FIXED + OQ-S4-3/OQ-S8-1 tracked-open. The driver lane is now fully dispositioned: every D-handoff (D2/D3/D4 + the FileVersion note) has an owner finding; the wire contract itself re-verified MATCH (identity IOCTLs, Information×512, LBA scaling, 19 session codes, NAS/Cloud exclusion — binary-probed).

## Session-8 delta (2026-10-28 — the second-session deep audit: missed-surface sweep + remaining-feature filing + landing verification + FIXED-truth spot-check)

**Audited tree: product code `35adcb9`** (HEAD `ff879f9`+ = this session's trail-only
commits) · **corpus `be0cfe9`** (one commit past the standing `03ad46c`; the
CORRECTIONS #9 absorption re-verified first-hand — CORPUS-ERRATA E-A/E-B,
PARITY-DELTAS D-104, SCAN-ENGINE-DELTA F2 all carry it). Method: five parallel
read lanes (og-codebase never-audited asset sweep; W1a/W1b/W1c/thumbnail
landing verification; carve-batch + NTFS-carrier + installer + FIXED-truth
spot-check; remaining-feature evidence packages; the two NEVER-CITED driver
docs + MOM decode candidates), then first-hand re-verification of every
load-bearing claim by the lead before filing.

**25 new items** — the session mandate's three asks, delivered:

**(1) Mark all remaining features/implementations/functions as findings** (the
plan-of-record R-rows + the S-007 batch-2 tail, previously "recorded outside an
active finding"): **S-040** (SEVERE — image create wholly absent, R-93),
**S-041** (SEVERE — the S-007 batch-2 carve remainder, ~24 formats), **M-072**
(query axes path_substring+state_mask, R-22), **M-073** (scan-time folder
filter, R-14), **M-074** (destination validation codes, R-41), **M-075**
(chkdsk launch, R-65), **M-076** (TRIM facet in enum rows, R-63), **M-077**
(preview caps defaults, R-34), **M-078** (BitLocker turn-off, R-88), **N-098**
(enum refresh contract sentence, R-86).

**(2) What session 1+ missed** (the deep sweep): **M-082** (the DeviceManager
offline-OS/boot-volume identification — live-module machinery the CheckVersion
dead-shipped verdict never covered; the mom-capabilities disposition re-scoped),
**M-081** (CRW misattribution — the Canon CIFF keys landed as "Minolta MRW"
rows whose PRD gate rejects genuine CRW), **M-071** (the M-018 REGRESSION —
rrecov.cpp's reconstruction in the N-064 wave dropped the junk-filter gates
from the CLI tree print and recover-all; git-proven `69507cb` → `cfa4c75`),
**M-079** (CLI auto-saves never carry the volume label — no fill site), **M-080**
(thumbnail default 4 MiB read budget starves ordinary photos — 6 MiB 12-MP
JPEGs get BudgetExceeded), **N-090..N-097** (session-list scope/row shape;
PARITY-DELTAS registration gaps; thumbnail refusal-matrix holes + no cache;
batch-1 MID/GZ bounds; CLI `-h` sessions verb missing; five stale-text doc-truth
sites; identity-IOCTL non-issuance; auto.txt misclassification — it is the FVE
auto-unlock key snapshot, not a WinPE payload), **I-027** (the og-codebase
data-asset ground-truth register: the 2 GiB free-quota contract, json_raw's
"TXT;HTML;LOG;H;CPP;MM;" shield, the Aliyun batching caps, the DiskHealth
schtasks autostart, filter_generalraw's 65-extension value), **OQ-S8-1**
(tracked-open — CWindowsFS decoded: the live-mounted-FS adapter; its og
consumer is the open question).

**(3) Verification of the new landings** (the T1 scope items): W1a/W1b/W1c
claims verified TRUE at the load-bearing points (cancel-per-node +
ERR_CANCELLED-never-flattened; cancelled-never-auto-saves; two-call pattern +
light-record summary; the found deltas are N-090/M-079/N-091 above); the
NTFS origin-LCN carrier verified on ALL contract parameters (both stamp
sites, sparse→0xFFFFFFFF, resident→0, bit-8 persistence, clone fan-out, 3
tests); S-007 batch-1 verified 27/27 magics byte-exact (bounds: N-093); the
installer steps 2-3 verified by the lane's own green dispatch record (run
40684304129 — no re-audit needed); the CI rebuild verified docs-only for
parity surfaces. **FIXED-truth spot-check: 10 findings sampled — 9
CONFIRMED-FIXED, 1 PARTIAL (M-018 → the M-071 regression finding).**

**Board after session 8:** **25 active** (2 SEVERE · 12 MAJOR · 9 MINOR · 1
INFORMATIONAL I-027 alongside I-026) · 249 FIXED · tracked-open 2 (OQ-S4-3
unchanged; **OQ-S8-1** new). **Top risks / recommended fix order:** (1)
**M-071** — a regression of a FIXED finding (recover-all writes junk; 2-line
fix); (2) **M-081** — Canon CRW recall + mislabel (small, self-contained);
(3) **S-041** — the batch-2 carve remainder (largest recall gap; M-081 first);
(4) **M-079** — the auto-save label fill (one function in fill_identity_card);
(5) **M-080** — the thumbnail budget (decide before the ABI handoff); (6)
**S-040** — image create (largest single feature); then the R-row majors
M-072..M-078 in plan-of-record wave order (W2a/W2b → W3 → W4 → W5); the
doc-truth minors N-094/N-095 batch-able in one pass. The driver-lane handoffs
recorded not fixed (READ-length semantics nuance; the disk.sys-comparison
field reading) belong to the rda-driver repo — noted in the worklog for that
lane, out of this audit's two-repo scope.

## Session-8 wave-2 delta (2026-10-28 — the continuation lane: batch-2 full decode, WMF grammar refutation, thumbnail verdicts, auto-save gates; reconciled over the parallel deep-audit lane)

**Audited `ff879f9` lineage against corpus `be0cfe9` (read-only).** This lane
ran in parallel with the deep-audit lane below (its 25 items are in the next
section); the push-time reconciliation renumbered this lane's IDs past that
lane's set and MERGED the two content-duplicate findings (the batch-2 decode
→ **S-041 addendum**; the thumbnail budget → **M-080 addendum** with the
og-side 200 MB eligibility gate that corrects that finding's "no input cap"
claim). **This lane's net: 16 new findings + 2 merges** — board after both
legs: **41 active (6 SEVERE · 15 MAJOR · 17 MINOR · 3 INFO) + 249 FIXED +
OQ-S4-3/OQ-S8-1 tracked-open.**

**Top risks added by this lane (fix order contribution):**
1. **S-042** (SEVERE) — the WMF-placeable validator contradicts the og on
   every gate (checksum word@+20 == XOR words 0..9, reserved dword@16-19,
   tail word, exact size std_words*2+22) — the two engines accept DISJOINT
   file sets; the batch-1 landing's "XOR of words 0..8" claim about the og
   is refuted first-hand (FUN_1800264d0 :29826-37).
2. **S-043** (SEVERE) — thumbnails are not EXIF-oriented: the og reads GDI+
   property 0x112 and RotateFlips BEFORE the 256-px pass (:521694-725,
   call order :652307-309); portrait phone photos thumbnail sideways in
   the core.
3. **S-044** (SEVERE) — auto-save gating: the og's three trigger sites gate
   on results-found (empty scans never auto-save), not-already-saved (2 of
   3), and not-import; the entry gate is a byte flag on a config singleton
   — the core fires on every completed scan with a dir set.
4. **S-045** (SEVERE) — thumbnail format coverage: the og ships
   video-keyframe (ffmpeg agent, ports 20171/2), RAW (LibRaw/EURawImg,
   ⅛-downscale→PNG), HEIC (heif.dll embedded-first), PDF (pdfium)
   pipelines; the core refuses all four by name.
5. **M-083/M-084/M-085** — the og promotes nodes to grade 2 on thumbnail
   success (:422505-13); the MDB family's "Standard ACE DB" (ACCDB 0x4C)
   second signature is missing; TIFF LZW (compression 5) refused.
6. **The S-041 addendum** — the batch-2 decode completed: the seven
   text-school grammars (VCF/GPX/HIK/OSP/POS/AEP/CHFP), the 18-site
   config-gated registration zone, the registry geometry (59 u32 + 35 u16
   bindings), the u16 BOM battery, FCPEvt end-to-end — the fix plan's
   decode preconditions are all discharged.

**The verification lane (new landings):** W1a/W1b CONFIRMED (scanTime
+0x6C; restore progress 40+50·done/(total+done); per-child IsCancelled;
the list's record-1-then-record-2 read — the W1a "record 2" phrasing
corrected, erratum E64); W1c PARTIAL (naming CONFIRMED, trigger placement
CONFIRMED, the gating chain MISSING = S-044); OQ-SA-1 carrier semantics
CONFIRMED with chain corrections (**N-100**: +0x53/+0x4f first hop, the
FUN_180033250 typo, and the og PERSISTS the +0x60 twin — sparse-zeroed,
not derivable — which the core does not carry); batch-1 constants verified
correct EXCEPT the WMF grammar (S-042) and the ACCDB branch (M-084); the
landing's registry-region cites corrected (**N-099**, erratum E55 — also
correcting the parallel lane's N-093 WPG-ctor cite: the bounds live at
:15700-01, not :20542-48, verified first-hand both regions this session).

**Errata E55-E64** (census corrections, all first-hand): registry
geometry; 0x11C=FreeHand-11 (MPEG-TS is unkeyed brute — corroborates the
parallel lane's resolution); E4 52 5C 7B=OneNote not "LZH arc";
II/MM+0x1A=CRW (corroborates M-081); New3DS=Autodesk not Nintendo; the
XML four-key router through the SVG analyser; the WMF checksum
convention; MDB's ACE second signature; the auto-save gates; the
session-list record-1 role.

**Thumbnail parameter verdicts rendered for the first time** (the engine
landed post-audit): 4 SEVERE/MAJOR defects + the superset register
(**I-028**: both-axis scaling vs the og's width-derived quirk — og
portrait thumbs exceed 256px on height — straight-alpha RGBA8 vs
white-flattened HBITMAPs, PSD/PNM/HDR, dims-first bomb guard,
UnreadableSpan truth gate) + 5 MATCH rows + 6 UNVERIFIED corpus-silent
residuals tracked in I-028's Open questions.

**Never-cited sweep (N-101):** the trail's citation arithmetic corrected
(~95 cited / ~187 never-cited, not 78/204 — brace-group/alias/short-stem
forms unfolded); per-binary 112/114; EuShare 2 never-cited (exclusion
re-anchored); EFIBoot.md never-cited (the decompile is the cite);
GetDriverInfo.dll — the one corpus-indexed surface with zero disposition
anywhere — registered OUT-by-lineage (**N-106**); the four data-asset
registrations this lane drafted defer to the parallel lane's I-027/N-097
deeper reads.

**Coverage:** no src/core module is without audit disposition; no
unregistered core superset; progressive carve emission dispositioned MATCH
(live tree growth in both engines — carve_slice streams nodes during the
pass, cr_scan.cpp:1877-88).

**Recommended fix-order additions from this lane:** S-042 (small, sharp,
wrong results) → S-043 (orientation) → M-084 (ACCDB) → S-044 gates →
M-083 (grade seam) → M-085 (TIFF LZW) → the S-041 batch plan (decode
complete) → S-045 phase-2 deps → N-100 (bit-9 or register) → the
N-101..N-106 truth batch.

## Session-9 delta (2026-10-28 — the SA-9 lane: the wave-2 continuation executed, then RECONCILED at push time into the parallel session-8 board)

**Audited:** core product code `35adcb9` · rda-driver `323e75a` (v0.2.4) ·
corpus `be0cfe9`. The lane executed the session-8 T2 lanes (V/R/G/C) plus
the D-lane (the driver-repo handoffs D3/D4 + FileVersion — in scope via the
two-repo grant), filed 8 findings — and at push time found the parallel
session-8 lanes had concurrently landed their own reconciled 75-active
board covering the same ground (including the M-081 CRW **fix as code**
`9e5dedc` and the school-1 batch-2 formats **as code** `b92ebef`).

**The reconciliation (this lane's 8 filings → 7 merged + 1 kept):**
- **S-040 (WMF)** → merged into the parallel lane's **S-042** as the SA-9
  addendum (the std-WMF stricter-gates observation — our validator adds
  version/objects/maxrec gates the og lacks; the canonical-layout evidence
  table; triple verification: two lanes, two decompilers, one conclusion).
- **S-041 (CRW)** → merged into **M-081 (FIXED)** as the convergence
  addendum (the instruction-decisive registration quotes + the dead-row
  offset-bound proof; the MMOR/IIRS/FUJI → TiffAnalyzer correction).
- **S-042 (batch-2)** → merged into **S-041** (the parent) as the
  consolidated-decode-table addendum (the per-format gates, the
  instruction-level CHM disassembly, the pre-landing registry inventory).
- **M-071 (FileVersion)** → merged into **M-086** (the CI tag==resource
  gate proposal + the wire-version decision frame).
- **M-072 (regdrv)** → merged into **M-087** as a NEW PARAMETER it lacked:
  **the Start posture** — Start=3 + active UpperFilters cannot attach at
  volume-stack build; the og's filter service is Start=0 BOOT_START.
- **N-090 (counts)** → converged with **N-107** (theirs: 14 sites, broader).
- **N-092 (MID/GZ)** → converged with **N-093** (theirs adds PSP/SIT).
- **N-091 (driver doc-vs-code)** → **KEPT, renumbered N-127** (the unique
  survivor: the admin-gate claim with no code behind it, the stale
  512-byte-sector prose, the S12 dead-code contradiction, the unreachable
  dispatch default, the undocumented 57th check — none on the parallel
  board).

**The lane's unique MATCH verdicts (no findings, rows flipped):** the
never-audited **EPMNtDrv v2 internals + trio naming/PID handshake** (our
driver byte-exact: the identity IOCTLs with their buffer gates, the
PID-token transform re-derived at CREATE, the service-name-derived naming
+ EPMNTDRV fallback, DO_DIRECT_IO, ×512 Information); the V-lane landing
verifications (W1a/W1b/W1c session surfaces, the NTFS origin-LCN carrier,
the thumbnail engine, the installer steps 2-3, the CI rebuild); the G-lane
gap-hunt dispositions (every candidate traced to existing module coverage
— no false findings filed).

**Erratum E68** (part1:85's refuted "+0x64 secondary ID" label — the
SCAN-2f queue's row, not previously carried) + the convergence note tying
the lane's capstone CHM evidence to E66. **Gate docs:** the corpus pin
(`be0cfe9`), the SURFACE-INVENTORY row flips + bookkeeping corrections
(the EuShare miscount, the dead citation globs, the citation counts), the
CROSS-REF-MATRIX session-9 block (the reconciled form), the PARAMETERS
currency note.

**Board after the reconciliation:** the session-8 board (75 active per
their close: 7 SEVERE · 29 MAJOR · 34 MINOR · 5 INFO) + **N-127** — 76
active · 250 FIXED · 1 tracked-open (OQ-S4-3, unchanged). **The lane's
recommended fix-order contributions** (folded into the standing order):
the M-087 Start-posture decision rides M-087's fix; N-127's item-1 posture
decision (og-parity vs real hardening) should be made alongside M-086/M-088
(the version-contract pass) — one driver-contract session can clear
M-086/M-087/M-088/N-107/N-127 together.

## Fix-session delta (2026-10-28 — the PARITY-FIX-9 close-out: S-038 FULLY FIXED — the board's last active SEVERE)

**S-038 (SEVERE) FIXED — steps 1-4 complete.** The installer lane's first
fully-green dispatch (run **40684304129** at `1c4db82`, windows-2022):
all three PACKAGING.md §11 gates GREEN — InstallerOSGateMatrix (the
OSGATE-V1 sync pin + 17 matrix probes: 9 in-envelope admits from the
Win7-SP1 floor to Server 2025, 5 below-floor refusals, 3 wrong-arch
refusals), InstallerSilentRoundTrip (18 assertions: /VERYSILENT
zero-prompt install → ARP UninstallString + QuietUninstallString ending
`" /SILENT"` + the full value set → driver service RDA RUNNING under the
Boot File System group (§10's internal step) → uninstall via the ARP
quiet string verbatim → NO residue across files/ARP/version-key/service/
driver-images), InstallerVersionKeyWritten (the write+verify read-back).
The dispatch-era defects (pinned-choco downgrade; QuietUninstallString
double-quoting) were repaired in-session with run-cited lessons. The
13-parameter contract is implemented in `installer/core-recovery.iss`
(Inno Setup; §2 = Inno confirmed, §8 = telemetry omit confirmed); the
finding moves to FIXED/ with the full resolution trail.

**Board after the PARITY-FIX waves:** **0 active** · **246 FIXED** (245 +
S-038) · tracked-open 2 (OQ-S4-3 — the TTFR harness landed, the
measurement = the rebuilt lane's next dispatch; OQ-SA-1 — the NTFS +0x64
decode done by SCAN-2f, the engine-side origin-LCN carrier pinned in
their fix queue). I-026 remains informational. The wave also repaired
three CI-red classes on the parallel landings (the thumbnail C4244 + the
GCC-only pragma under clang; the stb EOF-protocol fuzz timeout; the
fuzz-preview UBSan nonnull) and landed GAP-S6-5's physical-ceiling scorer
+ the M-068 hygiene move + the testing.md count truth.

## Fix-session delta (2026-10-28 — the PARITY-FIX-3 wave: M-068's board move + the recall-gap register truth pass + S-038 steps 2-3)

**M-068 (MAJOR) board-hygiene MOVE** (the fix itself landed in the SA-6
wave, `43f4962` + convergence `38ccecb`; this wave moved the file to
FIXED/ with the Resolution section + the RESOLVED-INDEX row): the finding
had stayed under `findings/MAJOR/` with its status line already saying
fixed — the board's "244 FIXED" undercounted by this row. The idempotent
start leg (RUNNING-query first; 1056-after-install = the v0.2.4 contract's
success shape; any other start failure fatal) is the landed shape.

**The recall-gap register truth pass (GAP-S6-5's action + the stale-row
flips)**: bench/root-causes.json (filed 2026-10-24, predating the fixes)
verified against code — ALL SEVEN engine-side recall gaps were already
fixed (GAP-S6-1/2 `9fc82b0`; S6-3/4 `7797bd6`; S6-6/7/8 `03ddcd2` +
F-1 `cd9ea16`) with tests; every entry now carries a resolution field; the
decay entry carries GAP-S6-5's root-cause CORRECTION (physical
sector-reuse, not an engine defect) and the scorer action LANDED: the
physical-ceiling metric (apply-scenario emits physical-ceiling.json via
`Invoke-PhysicalCeilingProbe` — each deletable file's seed-time extent
ranges hashed against the post-decay volume in one locked pass;
compute_metrics `--ceiling` adds the `physical_ceiling` block with
`recall_of_physically_intact`; compare.py surfaces it on BELOW rows;
self-test tools/test_ceiling_metric.py in ci-local step 5).
CORE-GAP-LIST.md S6-1..S6-8 flipped CLOSED with commit refs;
PERFORMANCE-PROFILE.md rows annotated (numbers stand until the next bench
dispatch re-measures).

**S-038 (SEVERE) steps 2-3 LANDED** (step 4 = the installer lane's first
green dispatch): the app installer per the PACKAGING.md 13-parameter
contract — Inno Setup (both owner decisions closed at the contract's
defaults: Inno + telemetry omit); ARP QuietUninstallString + the version
key (HKLM\SOFTWARE\heicfast\core_recovery\Version) write+VERIFY by
[Code]; the modernized OS gate (Win7 SP1 → Server 2025 x64) with the
OSGATE-V1 sync pin; the driver deployer as the default internal component
(exit codes propagate); the three test gates (run-gates.ps1 +
test_installer_osgate.py) in the new installer.yml lane.

**Board after this wave:** 1 active (S-038 — steps 2-3 landed; step 4 =
the lane's first green dispatch) · **245 FIXED** (244 + M-068's move) ·
tracked-open 2 (OQ-S4-3 — the TTFR half; OQ-SA-1 — the NTFS +0x64 half
DECODED by the SCAN-2f lane, the engine-side origin-LCN carrier pinned in
its fix queue). I-026 remains informational. CI red repaired on the
parallel thumbnail landing (cr_thumbnail.cpp C4244 + the GCC-only pragma
under clang, run 40680931476).

## Fix-session delta (2026-10-28 — the SA-6 probe lane: M-064 FULLY FIXED (the v0.2.4 remainder, upstream rda-driver))

**M-064 (MAJOR) FULLY FIXED** (the second half landed upstream): rda-driver
v0.2.4 (`323e75a`) — `-Layout Versioned` (default): the dest is
`<ServiceName><MajorMinor>.sys` from the source image's version resource
(the reference setupdrvx64 `%s%d` coexistence scheme — an in-use image is
never overwritten; no version resource = an honest Plain fallback, never a
guessed tag); a different-version install over an existing service is the
**in-place upgrade** (stop with the M-061 poll → `sc config binPath=` → the
shared Parameters write+verify → the shared start leg; the old file stays —
the reference's rebootless-upgrade story; new exit codes 8/9); `-Uninstall`
delegates to uninstall.ps1 by re-invoking the current engine (one
implementation, both entry points); the teardown captures the service's
actual ImagePath (the versioned name) + sweeps the versioned leftovers; the
CI lint lane gained the **PowerShell AST parse gate** (the v0.2.3 lesson: a
broken script broke every CI install). The core's driver-status discovery
needed NO change (the service carries the binPath; the composite's
assertions are dynamic — verified). The finding carries the full two-batch
Resolution; the matrix row -> MATCH (FIXED).

**Board after M-064:** 1 active (1 SEVERE (S-038 steps 2-4 — the installer
implementation, pending the owner's two PACKAGING.md decisions)) · **244
FIXED** (the M-064 file carried no prior row — the v0.2.3 half tracked in
the module doc only) · tracked-open 2 (OQ-S4-3 — the Windows CI dispatch;
OQ-SA-1 — the NTFS +0x64 residue). I-026 remains informational (no action).

## Fix-session delta (2026-10-28 — the SA-6 probe lane, wave 2: A-011 (NEW, ACUTE, filed+fixed) + N-089 FIXED)

**A-011 (ACUTE, NEW-FINDING — filed+fixed in one landing)**: the SQLite
validator read the page size and the page count LITTLE-ENDIAN — every
multi-byte SQLite header field is BIG-ENDIAN (the format spec; the og
composes the page size explicitly BE, FUN_18002e2b0's `v8 | (256 * v5 &
0xff00)`, the RetDec export :45142-45156 — the function was missing from
the Ghidra export). A real 4096-page-size database stored `10 00` at +16,
read as 16, failed the < 512 gate — **every real SQLite database was
grammar-rejected** and surfaced only through the raw-sign layer with
degraded extents. A SECOND latent bug fixed in the same never-exercised
path: `pages > 2ull << 30 / page_size` parsed as `2ull << (30/page_size)`
= 2 for 4096-byte pages (the precedence bug hid behind the endianness
bug; NO grammar-layer SQLite test existed — both prior fixtures were
raw-sign-layer tests). Both read BE now (`rdbe16`/`rdbe32`); the og's
text-encoding gate added (BE u32 at +56, valid 1..3).
`CarveEngine.SqliteRealHeaderByteOrder` (fail-before RED: no grammar
carve) + the two reject-guards. **The grammar emit now also propagates
the analyser evidence note to the node** (the og's CRAW second-string
field +0x5A equivalent — every grammar-carved file carries its family
evidence).

**N-089 (MINOR) FIXED** (rode the A-011 landing): the FCP-event-database
subtype relabel — the og's CFCPEvt deep walk relabels FCP event databases
(id 0x73 -> 0x75) on the 17-byte "tableactiveactive" schema marker
(FUN_18002de40, the RetDec export :44847-45100: a UTF-16 name buffer +
WideCharToMultiByte + the exact-17-char compare). The core's
validate_sqlite scans the first 4 pages for the marker in the DB's
declared text encoding; a hit relabels the note to
"page-math+fcpevent". `CarveEngine.SqliteFcpeventSubtypeRelabeled`
(fail-before RED) + `SqliteGenericLabelUnchangedWithoutMarker`.

**Board after A-011+N-089:** 3 active (1 SEVERE (S-038 steps 2-4) ·
1 MAJOR (M-064-remainder) · 1 INFO (I-026 — deliberate, no action)) ·
**242 FIXED** (240 + A-011 + N-089) · tracked-open 2 (OQ-S4-3, OQ-SA-1).
Suite 970/970 (+8 tests).

## Probe-session delta (2026-10-28 — the SA-6 probe lane: OQ-SA-2 + OQ-SA-3 RESOLVED (all four + one questions decoded first-hand); N-089 filed; 2 corpus-errata rows)

**OQ-SA-2 (the carve-family decode candidates) RESOLVED** — all four
questions decoded first-hand from the decompile:
(1) **CFCPEvt IS the og's SQLite analyser** — registered on the SQLi key
0x694C5153 (the "SQLite format 3" prefix); the census's "multimap
collision" reading corrected (errata E47). The deep walk RELABELS the id
0x73 → **0x75** on the 17-byte FCP schema marker "tableactiveactive" —
a subtype typing the core lacks → **N-089 filed (MINOR, labeling-only)**.
(2) **No accept/reject boundary divergence**: RIFF is map-registered
PRIMARY (Brance::RIFFAnalyser id 0x3b + FileRIFFAnalyser fallback on key
0x46464952); AAC's dispatch primary is CADTSAnalyser (u16 0xFFF1) with
CFileAACAnalyzer (id 0x4f) list-layer; M1V (id 0x7f) is list-layer only —
the core's primary grammar validators match the og's validation tier.
(3) **The WPG "canonical size 0x45" is the FORMAT-ID stamp, not a size**
(errata E48): the WPG Analyse is a constant predicate (0x677077 → id
stamp 0x45) + the FileInfo bounds 20 B..100 MB — the core's
signature-row shape.
(4) **CNeedRawMgr is a per-format NAME gate at list-construction time,
NOT a scan-mode gate**: the once-init singleton + a std::map name lookup,
its 20 call sites all in the analyser-LIST builder — the core's M-024
per-format disable list is the direct counterpart. No divergence.

**OQ-SA-3 (the EUUserData consumption mode) RESOLVED — on the Agent 2
scan-engine lane's refuting decode** (`151ce17`/`93097cb`, reconciled this
session): `GetRawUserInfoMap` is a per-scan **carve-statistics map**
(insert-side FUN_18000ed00: insert-or-increment per detection, cleared per
scan) whose DRWUI consumer is a post-scan REPORT reader (the counts
formatted %lld into the "Trigger_Raw"/"FileSystem" report tree — a report
section, not a trigger); EUUserData's actual consumer is the **CHK repair
chain** (CHKRepairFileSign, DRWUI_decompiled.c:1005387+); the RAW engine
carries ZERO EUUserData references. The M-065 lane's capability reading
(adopted then re-verified mechanically by this lane) had the data flow
inverted — the %lld COUNT formatting was in the quoted evidence and its
significance missed; the correction is recorded in the probe lane's
worklog. M-065's landed fix stands as the registered SUPERSET D-104.

**Corpus errata**: E47 (the SQLi "collision" is the actual binding) +
E48 (the WPG size misread) appended.

**Board after this landing:** 4 active (0 ACUTE · 1 SEVERE (S-038 steps
2-4) · 1 MAJOR (M-064-remainder) · 1 MINOR (N-089 NEW, READ-ONLY) ·
1 INFO (I-026)) · **240 FIXED** (238 + the two resolved OQs) ·
tracked-open 2 (OQ-S4-3, OQ-SA-1).

## Fix-session delta (2026-10-28 — the SA-6 wave-2 lane, S-038 step 1: the PACKAGING.md contract doc)

**S-038 step 1 (the contract doc) LANDED**: `docs/PACKAGING.md` — the
13-parameter installer contract (the silent flags /SILENT /VERYSILENT
/SUPPRESSMSGBOXES /NORESTART /NORESTARTAPPLICATIONS; admin-required
uninstall; ARP UninstallString + QuietUninstallString ending " /SILENT";
unins%.3d multi-instance; the Inno metadata keys; the /SendInfo//SendInfoEx
install telemetry + /UninstallStart//UninstallEnd uninstall telemetry; the
HKLM\SOFTWARE\...\Version write+verify key), every parameter with its
verbatim string-table citation (first-hand re-verified this session: the
telemetry field form is in SetupUE's strings, the upload verbs in
InfoForSetup's). The technology decision (default: Inno Setup — the
evidence-pinned choice), the modernized OS gate (Win7 SP1 -> Win11 +
Server 2025, REGISTERED divergence past the reference's Server-2016 table),
the telemetry posture (default recommendation: omit), the single-instance
default, the driver install as an internal step (absorbing N-085), and the
three test gates. S-038 stays ACTIVE (steps 2-4: implementation, tests,
acceptance).

## Fix-session delta (2026-10-28 — the SA-6 wave-2 convergence: M-067 filed+fixed, the RESOLVED-INDEX ledger reconciled)

**M-067 (MAJOR, NEW-FINDING — the S-039 convergence residue) FIXED**: the
parallel lane's S-039 caps fix (`b07ee9d`) landed without its ABI half —
`cr_ui_user_sign` still `mime[64]`/`bytes[16]`, so a legal 73-char MIME
surfaced with no terminator inside `mime[]` (a host string read ran past
the field) and only 16 of a legal 44-byte sign's bytes surfaced. The
struct is resized to the v3 caps (`mime[81]`/`bytes[48]`/`sign_len` 1..48/
`offset` 0..1024) and `CR_UI_ABI_VERSION` 2 → 3 per the stability policy's
first struct-FIELD change (ABI-CONTRACT §9/§7d). Fail-before PROVEN
compile-RED on the reverted v2 struct (`-Werror=array-bounds`: the
compiler's own model of the host OOB read);
`CrUi.UserSignAbiCarriesTheFullS039Widths` GREEN on v3. Suite 955/955.

**Ledger reconciliation**: the parallel lane's landings (`b07ee9d`
S-039, `d1fba5d` S-037, `5e7390b` the v0.2.3 deployer batch — S-036/
M-060/M-061/M-062/M-063/N-084/N-085, `37c48c6` M-065 + OQ-SA-3 resolved)
moved their finding files to FIXED/ with full Resolutions but wrote no
RESOLVED-INDEX rows; this session back-filled the 11 rows (plus the
A-010 row the earlier FIX-1 lane missed) — the index is current again.

**Board after M-067 (filed+fixed in one landing):** 5 active (0 ACUTE ·
1 SEVERE (S-038) · 1 MAJOR (M-064-remainder) · 2 MINOR (N-086, N-087) ·
1 INFO (I-026)) · **236 FIXED** (M-067 + the parallel lane's same-window
M-065/M-066/N-088 landings) · tracked-open 3 (OQ-S4-3 + OQ-SA-1/2 —
OQ-SA-3 resolved by the M-065 landing).

## Fix-session delta (2026-10-28 — N-086, the rawsigns posture registration)

**N-086 (MINOR) FIXED (registered)**: the compiled-in base-signature-
table posture is now a registered deliberate divergence —
**PARITY-DELTAS D-103** (option (a): reproducibility over the og's
field-mutable install-tree raw_signs.json; the runtime-extension
surface is the usersigns path with the shipped 111-entry default set;
option (b) — a data asset with a SHA-pinned fallback — is the tracked
design should field-updatable signs become a product requirement).
Content-identical on the corpus reference; a registration fix, no code
change.

**Board after N-086:** 3 active (1 SEVERE S-038 · 1 MAJOR M-064-rem ·
1 INFO I-026) · **237 FIXED** · tracked-open 3.

## Fix-session delta (2026-10-28 — N-087, the FAT 8.3 OEM decode)

**N-087 (MINOR) FIXED**: `short_name_to_u16`'s identity byte-widening
replaced with the full standard-verified CP437 table (128 entries,
byte-exact; ASCII passes through unchanged). The og persists raw OEM
bytes and lets the mounting codepage supply glyphs; the core's UTF-16
name carrier now gets the faithful boundary conversion (the finding's
step-1 semantics). The 0x05 KANJI escape and deleted-marker paths are
caller-side and unchanged — a live 0x05-led name decodes 0xE5 → CP437
sigma (U+03C3), where the pre-fix identity map produced U+00E5 (and
byte 0x93 produced the C1 control U+0093 instead of o-circumflex).
3 tests; the two behavior tests fail-before PROVEN on the
stash-reverted build.

**Board after N-087:** 4 active (1 SEVERE S-038 · 1 MAJOR M-064-rem ·
1 MINOR N-086 · 1 INFO I-026) · **236 FIXED** · tracked-open 3.

## Fix-session delta (2026-10-28 — M-066 + N-088, the preview parity pair)

**M-066 (MAJOR) FIXED**: preview-time archive-content enumeration —
`Metadata.entries` (name, size, compressed, crc_present) collected on the
CD walk that already hunted `word/document.xml` (the docx upgrade
coexists; no separate parse). FS-origin archives — the finding's named
case — now preview with the full inventory (the og's ArchivePreviewModel
browse-inside-archive contract). Bounded: 512 entries + the truncated
flag; partial CD reads also flag. Belt fix en route:
`extract_metadata`'s closing `m.truncated = w.truncated` was an
assignment that clobbered the inventory flag — now an OR. 4 tests, all
fail-before by construction (the field did not exist; compile-error
proven on the stash-reverted build).

**N-088 (MINOR) FIXED**: the M-028 propagation tail — cr_ui.h's
"the reference's IInWindowStream preview contract" claim, API-SURFACE-
AUDIT:94's row name, and REFERENCE-API-GAP R-33 all corrected (the
interface is the 7-Zip/LZMA SDK input, M-028's proof); the `mov` ghost
removed from the Metadata format comment; `PreviewDocs.NoIInWindow-
StreamDriftAndNoMovGhost` guards the three contract sites (R-42's
og-architecture description of the export loop is exempt — the og
genuinely reads engine streams through the LZMA SDK there).

**Board after M-066 + N-088:** 5 active (0 ACUTE · 1 SEVERE S-038 ·
1 MAJOR M-064-remainder · 2 MINOR N-086/N-087 · 1 INFO I-026) ·
**235 FIXED** · tracked-open 3.

## Fix-session delta (2026-10-28 — M-065, the shipped default user-sign set; OQ-SA-3 resolved)

**OQ-SA-3 RESOLVED** (first-hand decode): `GetRawUserInfoMap` is a
RAWSearchDLL export consumed by DRWUI at scan setup — the map's entries
(extension + numeric setting) build the `"Trigger_Raw"`/`"FileSystem"`
config tree that feeds the RAW searcher's trigger surface
(DRWUI_decompiled.c:227538-227585). The DB is capability-configuring, so
the default set is a recall-adjacent parity item and M-065's severity
stands.

**M-065 (MAJOR) FIXED**: `data/userdata-defaults.json` — the og's own
111-entry / 190-sign install-tree EUUserData, canonicalized (sorted keys,
verbatim signs/mime), parsed clean under the post-S-039 caps. rrecov now
loads it from the executable directory at engine init (absent explicit
`--user-signs`; an explicit file replaces; corrupt = degrade-empty with a
log line, absent = one log line, never an error — the og's config-file
posture). release.yml stages it beside rrecov.exe AND under data/;
PREBUILT-ARTIFACTS §3 documents the asset. Tests:
`UserSigns.DefaultSetShipsAndLoads` (111 exts / 190 signs) +
`UserSigns.OgAbsentFromStockExtensionsCarried` (the 47-name
absent-from-stock enumeration). Suite: 920 tests — 916 passed / 4
platform-skips / 0 failed; the Win7 parse gate green (GetModuleFileNameW
joined the win-stub SDK).

**Board after M-065:** 7 active (0 ACUTE · 1 SEVERE S-038 · 2 MAJOR
M-064-remainder + M-066 · 3 MINOR N-086/N-087/N-088 · 1 INFO I-026) ·
**233 FIXED** · tracked-open 3 (OQ-S4-3, OQ-SA-1, OQ-SA-2 — OQ-SA-3
resolved this pass).

## Fix-session delta (2026-10-28 — the v0.2.3 deployer batch: S-036 + M-060 + M-061 + M-062 + M-063 + N-084 + N-085)

**Upstream (rda-driver v0.2.3, tag `6b50bb8`→`f569836`)**: the elevation
guard's `[WindowsPrincipal]::[WindowsIdentity]` InvokeMethodOnNull chain
fixed in BOTH scripts (S-036 — the documented installer can now actually
complete on a stock host); install start-failure is FATAL (exit 6 + the
testsigning guidance, exit 7 for not-RUNNING; M-062); uninstall polls
STOP_PENDING 10×500 ms, checks $LASTEXITCODE on every SCM call, verifies
the registration is gone, and fails loudly on the locked image (M-061);
the four Parameters values are read back and verified (M-064's script
half); the README documents the full exit-code contract (install 0-7,
uninstall 0-6); the release body separates build-gate from install/load
evidence (M-063). No binary change — scripts/docs only.

**core_recovery close-out**: the composite pins `FloorVersion v0.2.3`, fails
LOUD on installer failure (the sc.exe fallback is opt-in via
`allow_fallback` and, when taken, configures the service exactly like
install.ps1 — Boot File System group, absolute binPath, the four Parameters
values — with `sc qc` + read-back assertions on BOTH paths: M-060); the
driver lane's teardown is now the release's uninstall.ps1 with post-state
assertions (M-061's CI half); the lane is a [windows-2022, windows-2025]
matrix (M-063's second runner class); `timestamp_server` composite input +
the /tr rationale in PREBUILT-ARTIFACTS §2 + DRIVER-RELEASE §7 (N-084); the
operator-posture note (N-085, absorbed by S-038's milestone); the
environments contract documented (M-063). M-064's script half landed with
this batch (write+verify + per-call SCM errors); its %s%d versioned-
coexistence + -Uninstall-verb halves remain open on the driver-install
module row.

**Board after the v0.2.3 batch:** 8 active (0 ACUTE · 1 SEVERE S-038 ·
2 MAJOR M-064 remainder + M-065 + M-066 — see below) — precisely:
**1 SEVERE (S-038) · 3 MAJOR (M-064-remainder, M-065, M-066) · 2 MINOR
(N-086, N-087, N-088 — 3 MINOR) · 1 INFO (I-026)** · **232 FIXED** ·
tracked-open 4 unchanged.

## Fix-session delta (2026-10-28 — S-037, the two missing carve formats)

**S-037 (SEVERE) FIXED**: FreeHand 11 and Corel PhotoPaint implemented
from the og's decompiled analysers (first-hand decode this session):
`validate_fh11` = the og's FUN_180029f30 constant predicate (u64@+4
`0x020014011C040002`, u32@+0xC `0x011C1400`, u16@+0x22 "11", key 0x11C;
CZeroEndRawFileInfo -> bounded extent), `validate_cpt` = the
FUN_180043630 Analyse gate ("FILE"@+4, 0x13C@+0x34) + the FUN_1800436e0
pointer-chase walk (n@+0x28 8-byte entries -> last -> +0x20/+0x3c
rebase pair -> data-descriptor size u32@(t2-8)+u32@(t2-4), any read miss
rejects). Four APPENDED ids (FMT_FH11/FMT_CPT7/8/9; serialized
carve_format stable); the CPT keys carry the og's duplicate-key registry
policy; trial order extended. 4 tests, fail-before PROVEN (both
detection tests RED on the stash-reverted pre-fix build); full suite
918 tests — 914 passed / 4 platform-skips / 0 failed.

**Board after S-037:** 15 active (0 ACUTE · 2 SEVERE S-036/S-038 ·
7 MAJOR M-060..M-066 · 5 MINOR N-084..N-088 · 1 INFO I-026) ·
**226 FIXED** · tracked-open 4 unchanged.

## Fix-session delta (2026-10-28 — S-039, the user-sign caps)

**S-039 (SEVERE) FIXED**: the caps in `include/cr/carve/cr_usersigns.h`
raised to the og's measured EUUserData maxima — `kMaxSignBytes` 16→48 (og
max 44), `kMaxSignsPerEntry` 8→12 (og 11), `kMaxOffset` 512→1024 (og 526),
`kMaxMimeLen` 63→80 (og 73) — the four false justification comments replaced
with the measured values, atomicity kept (honest caps + loud reject). The
og's own 111-entry DB now parses, installs, and the CLI proceeds to the
scan. Three tests, fail-before PROVEN on the stash-reverted pre-fix build
(the two behavior tests RED: the 44-byte sign and the 526-offset sign each
rejected the whole atomic parse); full suite 910 passed / 4 platform-skips /
0 failed. M-065 (the default set) is now unblocked. See
findings/FIXED/S-039-*.md for the Resolution.

**Board after S-039:** 16 active (0 ACUTE · 3 SEVERE S-036/S-037/S-038 ·
7 MAJOR M-060..M-066 · 5 MINOR N-084..N-088 · 1 INFO I-026) · **225 FIXED**
· tracked-open 4 unchanged.

## Dev-sign validation session delta (2026-10-27/28 — SA lane: Phase-1 driver dev-sign/install/uninstall validation on a real Windows runner, audited `dd82db9` / workflow content `3bda8e2`; corpus `03ad46c` unmoved)

**The mandate:** validate the merged product's driver under development
signing on real Windows environments, confirm the install/uninstall/load path
is production-shaped, capture the evidence, and file the installer-posture
gaps as findings audited against the corpus's documented reference installer
behavior. (Phase 2 of the same session — the corpus-driven parity re-audit —
follows; the parallel S7 lane owns the driver-only-era parity delta.)

**Execution evidence:** workflow `driver-install.yml` run **40549660074**
(windows-2022, `workflow_dispatch`, **success**) — the first green run of the
lane ever (the three prior dispatches failed on pre-install workflow-step
defects since fixed): unsigned rda.sys v0.2.2 fetched + SHA256SUMS-verified →
job-local cert (`CN=RRECOV-CI`) → `signtool /fd SHA256` → install → service
RUNNING → `\\.\RDA` control-device round-trip → `rrecov driver --json` 5-facet
`ready` (GET_INFO + GET_VERSION; version 1.0, flags=2, max_disks=200) →
grammar-path round-trip (`scan --disk 99` past the driver gate, transport
rc=50020001) → Phase-3 `not-started` class → teardown. Full transcript +
assets: `evidence/executions/devsign-run-40549660074/`; reproduction record:
`worklog/DEV-SIGN.md`.

**Findings filed (8 — all installer/dev-sign posture; no engine findings in
this phase):**

| ID | Severity | One-liner |
|---|---|---|
| S-036 | SEVERE | the documented installer (release `install.ps1`) crashes at its elevation guard (`InvokeMethodOnNull`) before installing anything; every green install on record went through the composite's undocumented `sc.exe` fallback |
| M-060 | MAJOR | the live install path configures the service differently from the documented one — invented group `FSFilter Boot File System` (corpus + install.ps1: `Boot File System`), no `Parameters` registry values, relative binPath |
| M-061 | MAJOR | the release `uninstall.ps1` is never exercised by any CI lane, carries the same broken elevation guard, stops with an unverified 1 s sleep, and prints `Removed` even when the locked-file delete silently fails |
| M-062 | MAJOR | `install.ps1` start-failure is a warning with exit 0 (success semantics on the canonical dev failure — signature rejection) + malformed concat in the message |
| M-063 | MAJOR | dev-sign validation is a single-environment data point (GitHub windows-2022, HVCI off); the HVCI/SecureBoot constraint and any Win7/Win10/Win11 matrix are undocumented while the release claims "Win7 SP1 → Win11" on PE-header evidence only |
| N-084 | MINOR | the sign step uses no timestamp server and the rationale (or the rule for when `/tr` is required) is undocumented in the copyable recipe |
| N-085 | MINOR | the documented install path requires an elevated interactive PowerShell console vs the og's product-orchestrated silent `setupdrvx64.exe` (SCM APIs) — installer-posture gap recorded for the packaging milestone |
| I-026 | INFORMATIONAL | unsigned-by-design driver + consumption-time signing (CI test-sign now execution-proven; OSSign production pipeline prepared-but-disabled) — deliberate divergence vs the og's vendor Authenticode chain, documented and verified |

**Board after Phase 1:** 8 active (1 SEVERE · 4 MAJOR · 2 MINOR · 1 INFO) ·
223 FIXED unchanged · tracked-open 1 (OQ-S4-3, unchanged). The dev-sign path
itself — the thing Phase 1 set out to validate — **works and is documented**
(`.github/actions/install-driver/action.yml` + `driver-install.yml`,
PREBUILT-ARTIFACTS §2 / DRIVER-RELEASE §7); `tools/dev-sign/` was correctly
NOT created (the mandate's precondition for creating it is absent).

## Dev-sign session delta — waves 1-2 (2026-10-28 — the never-cited corpus artifacts' first read: the carve family wave + the session field-schema wave; audited `dd82db9`)

**The method:** the session's corpus inventory (SURFACE-INVENTORY.md) found
204 of 282 corpus docs never cited; the highest-impact in-scope gaps were the
six RAWSearchDLL family artifacts (131 analyser classes — the carve module's
capability claims rested on the registry census, never the family semantics)
and ifileproperties-part1 (the NTFS/FAT/exFAT persisted-field schemas, cited
once vs parts 2/3's ×6/×8). Both were read in full and compared class-by-class
/ field-by-field, every load-bearing claim verified first-hand (including the
registry-key decodes from the og decompile: FH11 = key 0x11C id 0x1d; ASF =
key 0x75b22630 id 6).

**Findings filed (4 + 2 tracked-open):**

| ID | Severity | One-liner |
|---|---|---|
| A-010 | ACUTE | ASF/WMV/WMA carves end at the ASF **Header Object's** size field — every carved file of the family is a header-only fragment; the og's CAsfAnalyser walks to grammar break ("no premature truncation", deep-dive:87-91) |
| S-037 | SEVERE | FreeHand 11 (key 0x11C) + Corel PhotoPaint 3 generations (CPT7/8/9) escaped every format list — the core recognizes the FH11 *extension* (grading/filter tables) but can never produce a result; CPT absent entirely |
| N-086 | MINOR | the base signature table is compiled-in generated code vs the og's engine-init load of install-tree raw_signs.json — unregistered posture divergence |
| N-087 | MINOR | FAT 8.3 names identity-mapped to U+00XX vs the og's raw OEM-codepage bytes — non-ASCII 8.3 names display/restore with different glyphs |
| OQ-SA-1 | tracked-open | the part1 anchor-datum fields (FAT +0x3e lost for chainless files; exFAT +0x50, NTFS +0x64 undecoded) — probes specified (rsf_field_census.py correlation, og-on-exFAT run, FUN_14058c870 decompile) |
| OQ-SA-2 | tracked-open | four carve-family decode candidates (CFCPEvt attribution, AAC/M1V/RIFF tier roles, WPG size stamp, CNeedRawMgr gating) — vtable→Analyse probes specified |

**Also propagated:** CORRECTIONS.md #5 — the RawConfig [Filter] count is 34,
not 37 (recounted first-hand; 7 of the 8 corpus corrections were already
clean; M-024 addendum + matrix/SUMMARY/evidence notes). The class-by-class
disposition: 131 classes → 34 MATCH, 3 registered deltas, 64 covered by
S-007's open remainder, 26 N/A, 1 UNDETERMINED — **no new semantic divergence
among the implemented validators** (every evidenced difference maps to an
existing finding). Session-field disposition: 23 slots → 16 MATCH, 28 of 32
per-FS fields MATCH with the core's carriers richer in places.

**Board after waves 1-2:** 12 active (1 ACUTE · 2 SEVERE · 4 MAJOR · 4 MINOR ·
1 INFO) · 223 FIXED · tracked-open 3 (OQ-S4-3, OQ-SA-1, OQ-SA-2).

## Dev-sign session delta — waves 3-7 (2026-10-28 — the installer surface + preview + user-signature DB + the honest-zero waves; audited `dd82db9`)

**Wave 3 (installer):** the corpus's app-installer contract, verified from
the unins000/SetupUE/InfoForSetup string tables first-hand (every quoted
string re-read this session): the og ships a silent-capable Inno Setup
install/uninstall surface (ARP `QuietUninstallString` ending `" /SILENT"`,
/VERYSILENT, /SUPPRESSMSGBOXES, /NORESTART, admin-required uninstall,
`unins%03d` multi-uninstallers) wrapped by SetupUE (16-entry OS gate,
`SOFTWARE\EASEUS\DRW\Version`) + InfoForSetup (/SendInfo /SendInfoEx
/UninstallStart /UninstallEnd — uninstall is an instrumented lifecycle
event). The core has no app-installer surface at all → **S-038 (SEVERE, the
13-parameter contract on the board for the packaging milestone)**. The
driver deployer's versioned/verified contract (deepening-batch1 §3: -Uninstall
mode, `%s%d` versioned coexistence, Services write+verify, per-call SCM
errors) → **M-064 (MAJOR)**.

**Wave 5 (user-signature DB):** the grammar artifact + a first-hand
measurement of the og's own 16,891-byte EUUserData (111 entries) →
**S-039 (SEVERE)**: the core's caps reject the og's own DB — 15/111 entries
violate (44-byte signs vs the 16-byte cap, 11 signs vs 8, offset 526 vs 512,
mime 73 vs 63), the parse is atomic, ZERO entries load, and the CLI aborts
the scan with an error message that names "the og's EUUserData JSON shape";
the header's "the og's longest entry is 8 bytes" justification is factually
false → **M-065 (MAJOR)**: no default set ships (79 of the og's 111 default
extensions absent from every core stock source); severity calibrates on
**OQ-SA-3** (consumption mode: capability vs secondary typing).

**Wave 4 (preview):** UIUX/10's 19 parameters — 6 MATCH, 6 deliberate
UI-scope, 6 prior-covered (I-015/S-023/M-021/M-028) → **M-066 (MAJOR)**:
preview-time archive-content enumeration exists only on the carve path
(S-032's children); FS-origin archives preview opaque where the og's
ArchivePreviewModel browses damaged zips → **N-088 (MINOR)**: M-028's
IInWindowStream correction never reached cr_ui.h:1035 +
API-SURFACE-AUDIT.md:94, and cr_preview.h:116 lists a never-produced `mov`.

**Waves 6-7 (the honest-zero waves):** the WinPE payload census DISPOSITIONED
(the PE capability boundary — zero .mo = no lost-partition search in the
bootable edition, FREE-by-construction, installer-stamped version.ini —
recorded as the boot-media builder's design input; roadmap.md:71; no defect
in shipped code). The MOM-stubs spot-check: **zero new parameters** (13
ROLLUP-COVERED + 4 INFRASTRUCTURE-ONLY of 17 never-cited stubs), with an
honest correction of this session's own inventory (22→17 of 30 — five stubs
were cited in brace-group form). Decode candidates registered (CWindowsFS
×3, the DeviceManager offline-OS strings) as future-probe notes, not gaps.

**Board after waves 3-7 (as filed by the audit session):**
**18 active — 1 ACUTE (A-010) · 4 SEVERE (S-036, S-037, S-038, S-039) ·
7 MAJOR (M-060..M-066) · 5 MINOR (N-084..N-088) · 1 INFO (I-026)** —
plus 223 FIXED unchanged and tracked-open 4 (OQ-S4-3 + this session's
OQ-SA-1, OQ-SA-2, OQ-SA-3).

## Fix-session delta (2026-10-28 — the engineering pass over the audited board)

The fix session executing the handoff's priority queue. **A-010 (ACUTE)
FIXED**: `validate_asf` is now the BruteForce-family object walker (entry
gates unchanged; 5 known top-level GUIDs + size sanity + device-bound
torn-tail salvage; extent = last valid position) — 4 fail-before tests
(`AsfCarveExtentCoversDataObjects`, `AsfCarveTornTailSalvagesLastCompleteObject`
RED on the pre-fix build; grammar-break + count-gate rows pin the unchanged
halves). See findings/FIXED/A-010 for the Resolution.

**Board after A-010:** 17 active (0 ACUTE · 4 SEVERE · 7 MAJOR · 5 MINOR ·
1 INFO) · 224 FIXED · tracked-open 4 unchanged. The session's remaining open material is
precisely the four tracked-open files' probes; every other in-scope corpus
surface is dispositioned to a finding, a MATCH, a registered exclusion, or
an honest zero per SURFACE-INVENTORY.md.

## Review-and-validate session delta (2026-10-26 — the post-fix audit: every fix-lane landing and collision independently re-verified; the audit trail reconciled; the documentation suite created)

**The mandate:** review and validate the fix sessions' work — the
collision/union commits, the major removal commits, every Resolution's
claims — revert invalid removals, fix hallucinations, reopen
incorrectly-fixed findings, and overhaul the documentation.

**Removal-commit adjudications (all named and found):**
- `53b7d68` (M-052 convergence #1): VALID intent; its c33c64b engine
  choice superseded by `234bf0b`'s better-informed 76d4878 set (the
  N-079 defect its own test pins) — no revert needed (the convergence
  protocol had already corrected it). The .bak files it committed
  vanished via later merges; the tree is .bak-free.
- `83c796e` (the D-80 re-assertion): VALID. Follow-on finding:
  `97382b7`'s later "both-stand" restore re-introduced the superseded
  pwddict module as DEAD CODE (zero consumers at the reviewed tree) —
  **adjudicated: dropped again per D-80** (commit 28e9510).
- `a433c69` (dropped FatCheck tests): **PARTIALLY INVALID** — "the
  canonical covers the same behaviors" was false for two behaviors: the
  entry-hole half of 0x1006 (pin lost) and the FAT dir-walk cross-link
  0x1002 (pin lost AND the gate itself missing from the canonical
  module). Corpus re-verified first-hand (FUN_1800027a0 :1117-1230, the
  +0x60 shadow-registry re-entry :1156-1173). **FIXED: the gate
  implemented + FatCrossLinkedChainReported (fail-before EXECUTED) +
  FatEntryHoleReported** (commit 0a9f2aa; M-041 Resolution addendum).
- `5f2e2ad`/`7ec5446` union damage (the FatScanStats fields + the N-079
  scope test): found by this session AND repaired in-window by the
  parallel lane — verified restored.
- Junk committed by accident removed: `name` (a grep dump, b1f9210) and
  `cr_ui_abi_check.img.dest2/` (a test artifact, ca0a19e).

**The hallucination sweep:** every test name claimed in every
Resolution cross-checked against the actual suite (928 TEST sites) and
full git history (scripts/hallucination_sweep*.py, kept for future
audits). 162 claimed-but-missing tokens, ALL resolved as plan-section
names, re-suited landings, or word-order variants — every
behavior spot-checked has a live pin. Two evidence-wording overclaims
corrected in the trail (M-030's "Windows CI surface this fix wires";
M-044's convergence addendum citing plan names). **No hallucinated
fixes found.** One LOST HALF recovered: the canonical M-041 Resolution
claimed `scan --check` but it never existed in any commit — **landed
now** (the flag + the per-finding report; addendum 2).

**The CI-red wave:** the parallel lanes had pushed through red CI since
24ba1e7. All four gates diagnosed and fixed at the source: the ~100-file
clang-format drift, fuzz_image.cpp's born-broken map::insert (never
compiled since its S-030 landing), MSVC portability (gmtime_r, the
u32→u16 casts, the missing <algorithm>), and the TSan race in the test
event recorder (the product contract at cr_scan.h :110-113 explicitly
documents non-serialized worker delivery — the TEST now locks). TSan
870/870 + zero warnings across 5 stress runs.

**The ledger reconciliation (the DFX35 KNOWN DEBT, paid):** 12
same-number D-row collisions resolved per the errata convention
(D-91..D-102), 4 superseded copies dropped with the errata note, 7 live
references updated repo-wide, the CAPABILITY-MATRIX's four stacked S-014
paragraphs collapsed to the canonical dict-only posture. Ledger: 97
rows, zero collisions.

**Safety-hardening + first-class inventory verified intact**
post-collisions: destination-overlap refusal, BitLocker
legitimate-unlock-only, RawFixer consent, 4Kn gates, SDDL (driver-side
divergence), ReFS/LDM full engines.

**The documentation suite (the mandate's deliverable):** NEW
docs/API.md (the full CLI + 78-export ABI + C++ layers + threading
contract), docs/FORMATS.md (every format), docs/PARAMETERS.md (every
tunable), docs/CONNECTION-GUIDE.md (binding/devices/consent); README
refreshed; RELEASE-NOTES rewritten for v0.4.0-parity (referencing this
board + PARITY-DELTAS per the release mandate).

**Live counts after this session:** ACUTE 0 · SEVERE 0 · MAJOR 0 ·
MINOR 0 · INFO 0 open · tracked-open 1 (OQ-S4-3 — one live Windows
harness run) · FIXED/ 226 files (the two review addenda counted in the
M-041/N-064 files). **The findings board is EMPTY and now
independently reviewed.**


## Session-38 delta (2026-10-26 — v0.4.1-parity lane: the clang fuzz-lane warning backlog zeroed + the OQ-S4-3 transport measurement transcribed; docs truth pass)

**The zero-warning lane** (commit `095aab5`, on top of the v0.4.0-parity
tag commit `394a102`): the clang fuzz lane's entire 48-warning backlog
from the live job log (`-Wunused-const-variable` x44 on wire-format
constants in the zfs/hfsplus/btrfs/ext4/refs engines — now
`[[maybe_unused]]`, the constants stay as documentation;
`-Wmissing-field-initializers` x1, the refs Walker, trailing members now
written at their existing effective values, construction provably
byte-identical; `-Wtautological-constant-out-of-range-compare` x1, the
u32 RIFF-size > 4 GiB ceiling in validate_avi — structurally unreachable
on a u32, dead branch dropped with the contract recorded;
`-Wunused-but-set-variable` x1, btrfs subvol_index dead store;
`-Wunused-function` x1, cr_fscheck's caller-less rd64) plus the fuzz
Makefile now carries `-Werror` — every CI lane enforces the same
zero-warning gate (gcc/MSVC already did). The S-023 merge-storm duplicate
of the analysis-adoption block in cr_ui.cpp deleted (the live twin sets
`have_analysis` first; the second guard could never fire). Verified
locally: full linux-gcc ASan lane builds clean, **926/926**, incl.
win_syntax_check + cr_ui_abi_check. CI on `095aab5`: all six lanes green
(windows, linux, tsan, ubsan, lint, fuzz-with--Werror); API-Benchmark
green. The api-benchmark rda-driver pin moved v0.2.1 -> v0.2.2
(DRIVER-UTILIZATION pinning addendum).

**OQ-S4-3: the transport-chunking half is now a recorded measurement**
(alignbench run `40315255368` on `095aab5`, win22, `\\.\C:`, 512 MiB,
qd 1, RDA service RUNNING): og 4 MiB = 197.32 MiB/s vs core 64 KiB
envelope = 203.98 (og trails 3.4%); og 7 MiB = 196.15 vs core 203.98
(og trails 4.0%); core 256 KiB = 241.64 (og trails ~19%); CPU comparable
(593.8 vs 609.4/515.6 ms). Verdict transcribed in the OQ file's closing
addendum + M-030's Resolution measurement addendum. **Remaining open:
only the scan-level time-to-first-result half** (comparative-bench
dispatch; no engineering prerequisite). M-030's registered deliberate
improvement now carries evidence-grade numbers.

**The v0.4.1-parity docs truth pass:** CONTRIBUTING.md (build lanes,
local test runs, gate inventory, commit style) and SECURITY.md (the
BitLocker legitimate-unlock-only policy, no FVEK extraction, RawFixer
consent, Win7 floor, the driver-side SDDL boundary) created;
CONNECTION-GUIDE §8 (known host projects — the UI consumes nothing yet,
contracts kept in lockstep); DRIVER-UTILIZATION release-pinning
addendum; testing.md's 926 already matches the measured ctest count
(RELEASE-NOTES' older 924 described the pre-wave tree; v0.4.1-parity
notes state 926/926 measured).

**Live counts after this session:** ACUTE 0 · SEVERE 0 · MAJOR 0 ·
MINOR 0 · INFO 0 open · tracked-open 1 (OQ-S4-3 — **transport half
CLOSED by measurement**; scan-level TTFR half only) · FIXED/ 226
files. **The findings board remains EMPTY.**



## Session-37 delta (2026-10-26 — DFX37: M-040 stages (b) + the win layer FIXED; the SEVERE/MINOR tiers closed by the convergent lanes; the ASan pushed-reds repaired)

**M-040 FIXED (stages a+b)** — stage (b), the UEFI NVRAM Boot####/BootOrder
updater, landed as the pure module `cr/nvramfix` + the win layer
(`src/win/cr_bootfix.cpp`): the FUN_1800046f0 matcher (MBR sig+start+size /
GPT GUID+start) and the FUN_180004b40 full-position rewrite
(PartitionNumber/Start/Size/Signature) decoded first-hand; the firmware seam
resolved dynamically exactly like the og (EFIBoot :3281-3290 — ZERO new
static imports, the Win7 allowlist gate untouched); SeSystemEnvironmentPrivilege
via ntdll!RtlAdjustPrivilege (the og's XP path); the stage-a file side
(OVERLAPPED positional writes + FlushFileBuffers + the BCD00000000
force-unload retry under SeRestore/SeBackup). 11 NvramFix tests incl. the
finding's named NvramPatchUpdatesLoadOption. boot.ini + chkdsk stay at their
D-90 dispositions; the S-029 host wiring is the documented host-layer
boundary. The finding moved to findings/FIXED/.

**The SEVERE/MINOR tiers closed** — S-034 (the ring walk) FIXED by the
convergent parallel lane (their cb046ed landing adopted per
first-landing-wins; this lane's independent decode + implementation
reconciled and recorded); N-007/N-072/N-065/N-066 FIXED by the parallel
waves; M-030/M-039 landed. This lane's surviving artifacts: the two ASan
pushed-red repairs (the exFAT $I fixture overread + the filterfolder pool
NUL-termination, convergent with the lane's N-081/N-082), the cr_ui.h
union-marker repair, and the ZFS fuzz-seed generator.

**Live counts after this session:** ACUTE 0 · SEVERE 0 · MAJOR 0 · MINOR 0 ·
INFO 0 open · tracked-open 2 (OQ-S4-3, OQ-Z-E-FLT residuals) · FIXED/ 224
files. **The findings board is EMPTY — every filed finding is dispositioned.**


## Session delta (2026-10-26 — the SEVERE lane: S-034 FIXED — the ZFS ring walk)

**S-034 (SEVERE, S30 NEW-FINDING) FIXED** — the fix plan's three decode
items were resolved first-hand from the corpus BEFORE implementing (no
guessed parity): (a) the visitor's cross-txg registry (FUN_180005940 via
the lambda `_Do_call` 0x18000dd40; per-parent-path buckets through the
recursive path builder FUN_18001afb0; objid-keyed set/map helpers
FUN_180007db0/FUN_180007eb0): same (objid, path) skips, different path
re-emits under the new location with the registry overwritten — first-seen
state claims the identity; (b) each txg's walk re-reads the FULL config as
of that state — destroyed-in-window datasets/snapshots surface from the
older states' walks; (c) last_txg = progress display only (the previous
slot's txg string). Two additional decoded behaviors landed: the
`.Trash-1000/info` child suppression (FUN_180004f00 :3708; the /files
subtree NOT suppressed — FUN_180004ad0 :3447 verified at the disassembly
level, `mov $0x1,%al` on the equal path) and the label-0-only ring read
(FUN_180018220's 128 KiB @ +128 KiB) — the core keeps 4-label probing as
the registered superset, label 0 walked first for og-parity storage
order, each UNIQUE txg once. In-window deletions emit as REGULAR results
(the og has no per-engine deleted bit for ZFS) — the fix plan's "surface
as Deleted" guess corrected by the decode. Implementation: every valid
ring slot collected (RingSlot vector, txg-deduped, storage order) —
per-state `walk_state` (MOS → dataset hierarchy → ZPL) through a scan-wide
registry (dataset-name bucket → objid → {dataset-relative path, first-seen
node}); dataset nodes keyed (name, head_obj) — a recreated same-name
dataset emits a second node (per-objid semantics); a state whose tree
does not open is skipped (og addBlock no-op), scan fails only when NO
state opens; the per-txg "zfs: txg = %llu, last_txg = %llu" progression
rides the N-048 log seam; new trailing stats fields
ring_states_total/ring_states_walked. Proof: 2 new tests + 1 extended —
Zfs.RingWalkSurfacesDeletedInWindowFile (a two-state fixture: slot 0
txg 77 with gone.txt, slot 1 txg 78 without — the second chain built from
pristine copies with its own zap2/ZPL2/MOS2 blocks and the full
dependency-ordered fletcher4 re-stamping), Zfs.RingWalkRenameReemitsNewLocation
(the renamed object surfaces under BOTH locations, unchanged content
exactly once), NewestTxgAcrossLabelsWins extended (two states, same
rootbp → hello.txt exactly once + ring_states_walked==2) — ALL THREE
EXECUTED RED on the pre-fix engine (engine.cpp reverted, tests kept:
gone.txt count 0 / renamed.txt count 0 / ring_states_walked 0) and GREEN
after. Suite: 832 tests — 828 pass + 4 env-skips linux-release; 832
ASan-clean and 832 UBSan-clean (both exit 0) after TWO pre-existing
pushed-red ASan repairs landed in the same window (the exFAT recycle
test's past-the-literal path read + the FilterFolder pools' missing
trailing NUL — filed as N-081).

**Counts after this landing (the union tree, post-merge with the parallel
window's 70-commit wave — which independently drained M-030/M-034/M-039/
M-041/M-048, N-007/N-065/N-066/N-072 and landed its own N-081/N-082 ASan
repairs that this lane's convergent fixes yielded to per first-landing-wins):
SEVERE 0 · MAJOR 1 (M-040) · MINOR 0 · INFO 0 open · tracked-open 2
(OQ-S4-3, OQ-Z-E-FLT) · FIXED/ 221 files.**
# SUMMARY.md — executive summary (diff-fix session 33 on top; earlier sessions below)
# SUMMARY.md — executive summary (diff-fix session 37 on top; earlier sessions below)

## Session-37 delta, part 1 (2026-10-26 — DFX37 lane opens: the third-merge-storm repair + the count truing; S-034/M-040/N-072 claimed)

**The pushed-red repair (landing `6f0052b` → `c1e3ce5` through three
parallel-wave absorptions):** main was RED at session start (`875ed25` —
61/865 tests failing + win_syntax_check failing, the app layer unparsable).
Five defects verified first-hand and repaired: (1) the M-018 filterfolder
pools read as C strings with no NUL terminator — ASan global-buffer-overflow
on every junk-filter consumer (60 test failures; generator-level fix +
regeneration from the corpus FilterFolder.ini + test-side termination
pins); convergent with the parallel lane's N-081 length-bounded parse —
the final union keeps BOTH (terminated pools + bounded parse + both
guards). (2) the M-052 exFAT `$I` fixture overread (15 units from a
12-char literal) — convergent with their N-082. (3) cr_bitlocker.h's
N-064 comment block garbled by the union merge (4 parse errors). (4)
rrecov.cpp union damage — duplicated `open_volume_view`
signature/guard/args, tripled call, and `print_carve_formats()` DELETED
while its call sites survived (restored from the repaired parent
`cfa4c75`; the merge's genuine M-018 junk-filter gates KEPT). (5) the
merge union re-introduced the withdrawn cr::pwddict module — resolved to
the parallel lane's "both surfaces stand" convergence (their tested
union on origin/main; recorded here as the stable-main choice). Suite
green through every absorption: 894/894 → 898/898 → 902/902 → **902/902**
at the lane-claim tree `c1e3ce5`.

**Counts trued at lane claim (file inventory, `c1e3ce5`):** **FIXED 219**
· SEVERE **1** (S-034 — this lane) · MAJOR **1** (M-040 — this lane) ·
MINOR **1** (N-072 — this lane) · INFO 0 open · tracked-open **2**
(OQ-S4-3, OQ-Z-E-FLT/OQ-FLT-2). The parallel-lane waves this window
landed M-030, M-034 (scope-registered D-88), M-039, M-041, M-048, M-052,
N-007, N-065, N-066, N-081, N-082 — all absorbed green.

**Lane claim (this session's queue):** S-034 (the ZFS ring txg-history
walk — the worklog handoff's "highest-value ZFS work on the board"),
M-040 (the boot-chain family: disposition + the BCD offset rewriter),
N-072 (Btrfs re-scan cache + ZFS label polling), OQ-S4-3 (Windows bench
execution or evidenced push-forward), OQ-FLT-2 (the AdvanceFilterCondition
per-group decode). Session target: 20+ verified fixes — the three open
findings' sub-items plus the decode-driven NEW-FINDINGs the verification
pass is expected to surface.

## Session-37 delta, part 2 (2026-10-26 — DFX37: S-034 FIXED — the ZFS ring txg-history walk; the SEVERE tier is empty)

**S-034 FIXED (SEVERE — the worklog handoff's highest-value item):** the
og's quick searcher walks EVERY uberblock in the label ring — per-txg
state recovery (FUN_180006b90 :5685-5841 iterates every 1KiB slot ->
addBlock FUN_180003150 -> the from_txg=0 descent FUN_18001ac60) — and
the core read only the newest txg, leaving files deleted within the
ring's txg window (up to 128 transaction groups — the recent-deletions
population, a recovery product's primary scenario) invisible except via
RAW carving. All three of the finding's decode items were answered
first-hand from the corpus BEFORE implementation: (1a) the og has NO
engine-side emission dedup (the ZplNodeFile ctor FUN_180012450 is a
plain refcounted constructor; the visitor bulk-collects one ZplNode per
state per object); (1b) the per-txg walks re-walk each state's own
dataset/snapshot hierarchy through the same descent (no separate
MOS-snapshot-list step in the quick searcher); (1c) `last_txg` is
display-only — its sole occurrence in the entire binary is the progress
string at :5703. The fix: the geometry probe collects every valid ring
uberblock into a txg-keyed map (the og registry's semantics; a txg
mirrored across labels walks once), the per-state walk is factored into
walk_state(ub, live), the newest state walks as the live tree
(bit-for-bit the previous behavior), and every older state walks
newest->oldest into the deleted tree with cross-state identity dedup
(dataset head object + object id + the dnode's first block pointer's
LOGICAL birth txg): deleted-in-window files surface as Deleted results,
unchanged objects emit once, superseded incarnations surface as deleted
history, and live directories serve as unmarked containers in the
deleted tree. New stats ring_states_walked + ring_deleted_recovered;
root_deleted=nullptr (the pre-fix call shape) skips the old-state walks.
One REGISTERED divergence (PARITY-DELTAS D-05 verdict updated): one
emission per object instead of the og's one-per-state — same content
set, no duplicate rows. Proof: Zfs.RingWindowDeletedFileSurfaces
(fail-before RED EXECUTED: ring_states_walked 0, tank/GONE.TXT absent),
Zfs.RingUnchangedObjectEmittedOnce (the dedup pin),
Zfs.RingWalkSkippedWithoutDeletedRoot (the legacy call shape); the new
build_ring_pool() fixture builds a complete second pool state at txg 76
sharing every unchanged block with the live state (real-pool block
sharing); suite **905/905** linux-gcc (ASan) incl. win_syntax_check; all
18 pre-existing ZFS tests green (BE refusal, ashift-13 geometry,
foreign-vdev fallback, A-08 pins). Audit trail: findings/FIXED/
S-034 (Resolution with the decode answers + the fixture block-collision
lesson), CROSS-REF-MATRIX zfs B/D cells -> MATCH, modules/zfs.md
session-37 row + B1 detection update, PARITY-DELTAS D-05 verdict ->
closed-with-one-registered-divergence, CAPABILITY-MATRIX ZFS row, the
engine header's deleted-file story rewritten.

**Counts after this fix (file inventory):** SEVERE **0** · MAJOR 1
(M-040) · MINOR 1 (N-072) · tracked-open 2 (OQ-S4-3, OQ-FLT-2) ·
**FIXED 220**.

# SUMMARY.md — executive summary (diff-fix session 36 on top; earlier sessions below)

## Session-36 delta, part 1 (2026-10-26 — DFX36: S-034 FIXED — the last SEVERE closed)

**S-034 FIXED (SEVERE — the ZFS ring-txg history):** every decode item in
the fix plan was resolved first-hand from the corpus before any code
changed — the visitor `_Do_call` located by binary vtable walk
(FUN_18000dd40 at vftable RVA 0x309b8 slot+0x10), delegating to the
collector FUN_180005940 whose registry is keyed by the node's FULL PATH
(the recursive parent-chain concatenation FUN_18001afb0) holding
per-path object-id sets: files drop on id-hit (first storage-order state
wins), folders compare a display string (registered as a no-content-
effect residual), delete+recreate emits BOTH ids under the same path,
and last_txg is a cosmetic never-updated "0" in the log. The engine now
walks every DISTINCT-txg ring state (== the og's label-0 walk on intact
pools; the 4-label probe stays as the torn-slot superset) through the
existing MOS->dataset->ZPL machinery, with node-reusing (path, id)
dedup — later states attach new content to existing parents (the og's
descent continues through already-emitted subtrees). Deleted-in-window
files surface in the result set — the B1/B5 membership gap closed.
Proof: RingHistorySurfacesDeletedInWindowFile (fail-before EXECUTED RED
"deleted-in-window file lost"), RingHistoryUnchangedFileEmitsOnce,
RingHistoryDeleteAndRecreateEmitsBothIds; suite **905/905**
linux-release; ZFS 19/19 under ASan + UBSan. D-05 -> FIXED; the
session-5-era union regression (FatScanStats recycle fields + the N-079
test dropped by the 7ec5446 merge) repaired first (234bf0b-lineage
restore, suite 898/898 at that tree).

**Post-fix counts (this lane's view at its landing):** SEVERE **0** ·
MAJOR 1 open (M-040 — stage (a) landed by DFX34, stages (b)+win layer
remaining) · MINOR 0 (N-072 landed by the parallel lane — the btrfs
cache codec + the zfs label retry, both halves, verified in-source) ·
INFO 0 open · tracked-open 2 (OQ-S4-3 — Windows-lane; OQ-FLT-2). M-030
verified FIXED-ALREADY this session (the parallel lane's fa80496
landing re-verified in-source: win_pe_environment, the
IOCTL_VOLUME_OFFLINE terminal + the ONLINE/mount-point restore chain,
ENUMERATE_DISK_BOUND=200, cr_ui_last_transport_error). This lane's
convergent S-034 engine was reconciled to the canonical DFX33b
landing (first-landing-wins) with the binary-vtable decode correction
appended to the finding — see the Convergence addendum there.

## Session-33b delta, part 2 (2026-10-26 — DFX33b: M-040 CLOSED — the boot-chain repair family completed)

**M-040 FIXED (MAJOR — the board's last MAJOR):** stage (a) (the BCD
offset rewriter, `cr/bcdfix`) landed earlier this window by DFX34; this
session completed the family: **stage (b)** the UEFI NVRAM Boot####
updater as the pure `cr/nvramfix` module over an injected
firmware-variable store (the matcher FUN_1800046f0 decode: MBR
signature+start+size flags collected across the whole walk / GPT GUID
match; the state-4 update FUN_180004840 patches the PartitionNumber of
every HARDDRIVE node in matched options; the audit's "patch partition
start" was imprecise — the start is the MATCH KEY, the number is the
patched field; create/delete stays out per the plan); **the win
runtime** `src/win/cr_bootfix.cpp` (FwVarStore with dynamic
Get/SetFirmwareEnvironmentVariableW resolution + privilege + the
GetLastError mapping; bcd_rewrite_file with the consent gate + flush
tail; unload_bcd_hive SeBackup/SeRestore+ZwUnloadKey, ntdll dynamic —
no new PE import, the Win7 gate untouched; the win-stub grew the
consumed surface); **the S-029 post-apply wiring** (`rrecov apply
--boot-fix <old-lba>` opt-in: BCD offsets + NVRAM renumber after a
successful apply, boot-repair failures advisory). Proof: 9 new
NvramFix tests (incl. the finding's `NvramPatchUpdatesLoadOption`),
fail-before by construction; win_syntax_check green; suite 927/927.

**Board after this landing:** SEVERE 0 · MAJOR 0 · MINOR 0 ·
tracked-open 2 (OQ-S4-3 Windows bench; OQ-FLT-2 the DRWUI
AdvanceFilterCondition decode) · FIXED 222 files.

## Session-33b delta (2026-10-26 — DFX33b: S-034 FIXED — the ZFS ring txg-history walk; lane re-stamped DFX33b after a parallel DFX33 first-landing)

**S-034 FIXED (SEVERE — the board's last SEVERE):** the og's quick searcher
was re-decoded end-to-end first-hand (run loop :5312-5918 ring slot
iteration; addBlock :1967-2259; UberData registry :20085-20340; emission
walk :26173-26320 with the `from_txg < birth_txg || from_txg == 0` gate —
always true at the top level; the collector :5280-5311 is a plain
push_back with NO engine-side dedup; last_txg is display-only). The core
now walks EVERY valid uberblock state from all four labels' rings
(txg-deduped, newest-first): the first state whose MOS chain opens is the
LIVE tree; older states are RECALL walks with a cross-state identity
dedup (dataset, parent dir, entry name, object id, dnode bp0 birth) —
deleted-in-window files surface as Deleted nodes in place, unchanged
directories re-walk against their existing node (deleted descendants
surface under them), modified-in-window entries recall the prior version
as a Deleted sibling, torn states skip like the og's tree-open failure.
Proof: 4 fail-before-RED tests (RingWindowDeletedFileRecalled,
RingWindowUnchangedEntriesEmitOnce,
RingWindowModifiedFilePriorVersionRecalled,
RingTornNewestStateFallsBackToOlder); suite 903/903 post-merge.
`ZfsScanStats::ring_states` telemetry; D-05 re-verdicted FIXED in
PARITY-DELTAS.

**Ledger repair in the same landing:** PARITY-DELTAS.md's merge-storm
duplicate rows were deduplicated (172 duplicate blocks dropped — 156
content-identical + 16 divergent reconciled keeping the extended body;
99 D-rows remain, one per number, sections A-G intact). D-05 appeared
3x, D-06 3x, D-19/D-54/D-61/D-71/D-73/D-77/D-80/D-81/D-84 duplicated
with divergences — all reconciled.

**Board after this landing:** SEVERE 0 · MAJOR 1 (M-040) · MINOR 1
(N-072) · tracked-open 2 (OQ-S4-3, OQ-FLT-2) · FIXED 220 files.

## Session-33 delta, part 5 (2026-10-26 — DFX33: N-007 FIXED — ReFS page-signature discovery width)

**N-007 FIXED (MINOR — with the I-004 half staying ACKNOWLEDGED):** the
corpus was re-verified first-hand (ReFSSearchDLL :11692 — the og's page
dispatch compares the FIRST DWORD only; corrupt-tail pages open and the
ref-parser validates further). The core's detect now matches: first-4
discovery + the full memcmp as the grading signal
(RefsInfo::signature_partial). Proof: RefsDetect.PartialSignaturePageDetected
(fail-before EXECUTED RED on the full-width-only tree); refs suite 32/32;
full suite **891/891**.

**Post-fix counts (this landing):** SEVERE **0** · MAJOR 4 · MINOR 2 ·
INFO **0 open** · tracked-open 2.


## Session-33 delta, part 4 (2026-10-26 — DFX33: M-048 FIXED — the user-extensible signature database)

**M-048 FIXED (MAJOR — the EUUserData capability):** users can teach the
scanner new file types. The pure parser takes the og's JSON shape
verbatim (ext -> signs["offset,HEX"] + mime; atomic validation with
entry/sign/byte/offset caps); the registration surfaces are the cr_ui ABI
(since 2.1: register/count/info, replace-on-re-register) and the CLI
--user-signs <file> (honest failure on bad input); the carve layer probes
user signs at the raw-sign stride after the built-ins (the built-in-wins
collision rule), emitting with the user's extension, the MIME on
carve_note, tag 0xFFFE0000 and the documented fixed confidence 600.
Scope correction registered on fix-plan item 4: the og's EUUserData is
install-tree config re-read per scan — not session state — and the core
matches that posture. Proof: UserSigns.ParseTheOgsShape +
CarveEngine.UserSignatureProbedAtStride (fail-before EXECUTED RED) +
CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition +
CrUi.UserSignRegistrationSurface; suite **869/869** linux-release
(incl. the ABI gates over the new exports).

**Post-fix counts (this landing):** SEVERE **0** · MAJOR **5** open ·
MINOR per the live union · INFO **0 open** · tracked-open 2.


## Session-32 delta, part 2 (2026-10-26 — DFX32: M-025 FIXED — the carve window-aware read view + the pushed-red N-064 union repair)

**The pushed-red repair first** (main was RED from a parallel convergent
merge): the two N-064 landings (try_password_unlock/cr-pwddict vs
unlock_with_dictionary/cr-dict) merged with UNRESOLVED conflict markers in
6 files and a cr_bitlocker.cpp referencing the lost cr::dict module —
win_syntax_check failing, the app uncompilable. Repaired as the convergent
union (b00c02f): cr/pwddict is the canonical module; unlock_with_dictionary
re-implemented as its ABI-shaped adapter; BOTH CLI consent flags kept
(--bitlocker-pwddict <file|seed> + the valueless --bitlocker-dict); the
Windows contract tests rewritten onto the pwddict seam; docs keep-both.
Suite green again (win_syntax_check PASS).

**M-025 FIXED (the fix plan's steps 1 + 2a/2b + 4; step 3 evaluated and
not taken):** the WindowView — an IStorageDevice wrapper serving every
read intersecting the already-scanned window from the in-memory buffer,
wired at all 8 device-read sites of the window scan across BOTH emit
layers (grammar validator + M-051 checksum + small-file hash + M-021
feed; raw-sign tail walk + checksum + hash + feed); cross-window tails
split-read (in-window prefix + device tail). Fail-before RED on the
device-direct tree: the counting-device test saw **82 device reads** for
a one-window fixture (per-candidate probe + checksum + hash re-reads) vs
the expected **1**. MEASURED (scripts/bench_m025.cpp — 64 MiB sign-quiet
fixture + 1280 grammar-validated candidates; the LCG fill abandoned as
the bench lesson: random FF-sync bytes make the raw-sign tail walk read
11+ GB, measuring the tail-scan budget instead of this fix's delta):
device read_at calls 20,544 → 2,080 (**9.88×**), device bytes 2.20 GB →
1.06 GB (**2.08×**), wall 3,936 → 3,818 ms on the memcpy device (no
regression; the real-transport gain is ~18.5K fewer read round trips per
64 MiB). The geometry change (4-8 MiB ping-pong) NOT taken — dispatch is
byte-granularity-dominated (the I-009 superset) and the view already
removed the re-read margin. PARITY-DELTAS **D-80** registers the
stateless-view-vs-pending-object architecture (future stateful analysers
port behind the M-021 CarvePostProcessor seam).

**Counts after these landings (live tree):** SEVERE 1 · MAJOR **9** ·
MINOR 8 · INFO 0 open · tracked-open 10 · FIXED/ 191 files. Session tally
(this lane): M-029 + M-025 + the N-064 union repair.
# SUMMARY.md — executive summary (diff-fix session 33 on top; earlier sessions below)

## Session-30b delta (2026-10-26 — DFX30-MAJOR-tail lane: M-041 FIXED — the damage-audit check phase)

**M-041 FIXED (MAJOR — the og's FileSystemCheck parity, the last big
unclaimed MAJOR module):** a dedicated pure-core check module
(`cr/check/cr_fscheck.h/.cpp`) re-implemented from understanding of the
9,964-line FileSystemCheck.mo decompile (every gate re-verified first-hand
at corpus 03ad46c before implementation): the error taxonomy 0x1001..0x100e
in numeric parity (0x100c preserved-unused as in the og); the FAT pass in
the og's phase order — DBR validators per flavor (signature dispatch
"FAT1"+digit@+0x36 / "FAT3"+'2'@+0x52, ceilings spf×341.3333333 / spf×0x100
/ (bps×spf32)>>2, FAT32's spf16∈{0,spf32} + root-cluster range gates), the
volume-size floor (0x1001, non-terminal), per-copy dead-zone +
duplicate-reference scans (0x1006, terminal), the entry-by-entry pairwise
FAT-copy compare with the entry-0 media mask (0x1005, terminal; mask
mismatch = the og's silent stop), the FAT32 root-chain gate, the dir-tree
audit ('.'/'..' pair for non-root dirs, first-cluster==1/≥space, the
cluster-granular size-vs-chain compare — the og's sector-sum round-up
decoded, ancestor-array cycles; per-entry non-terminal), and the orphan
sweep (unvisited + FAT≠0/self/BAD → the host decision → single terminal
0x1003); the NTFS pass — the $MFT record-0 resident-"$MFT" proof (0x1008,
terminal), $MFTMirr slot-0 proof (0x1009), the version gate {0x102, 3,
0x301} from $Volume's $VOLUME_INFORMATION with the version word as the
param (0x100A), the $Bitmap structural floor (0x100B), and the MFT×$Bitmap
cross-check gated by $MFT::$BITMAP AND record in-use flags (per-LCN 0x1003
free-but-allocated / 0x1002 duplicate-claim — the two-bitmap decode). The
og's 8-event host callback surface as optional pointers; wired as the
optional pre-scan stage AFTER repair and BEFORE the engine pipe
(ScanConfig::pre_scan_check, default OFF), NEVER aborting the scan (the
escalation stance, pinned by test); REC_FS_CHECK session record (round-trip
+ load-path restore); the cr_ui ABI appends (cr_ui_scan_config
.pre_scan_check, stats check_ran/check_errors/check_detected, CR_EVT_FS_CHECK
= 10 with the M-049 error-context append unioned ahead of it); CLI
`scan --check` with the full per-finding report. Deliberate deltas
registered as PARITY-DELTAS D-87: the undecoded $Bitmap slack term
(checkntfs.cpp :3869-3871, CCheckNTFSVolume+0x30 — only the firing half
implemented, no constant guessed), the CheckIssue param/record superset
context, and the partial-bitmap-load refusal (never audit a zero-filled
tail). Proof: 15 tests (fail-before by absence — the module did not exist);
suites 758/758 linux-release AND linux-gcc ASan at the landing tree.

**N-079 CONVERGENT (this lane's eb3cb6d vs the parallel wave's canonical
deque-pool landing):** this lane independently found + fixed the N-048
seam's Site exit-leak (LSan red on 4 tests, fail-before EXECUTED on a clean
worktree at f5d4981); the parallel wave landed the same repair minutes
earlier — reconciled to theirs per first-landing-wins (my delete-under-mutex
design dropped cleanly; their deque-pool stands; the convergence recorded
in the worklog and the matrix note).

**Counts after this landing (single-lane view; parallel lanes active):**
SEVERE 1 open (S-034, filed by the tracked-open lane, unfixed) · MAJOR 6
open (M-030, M-034, M-039, M-040, M-048, M-052) · MINOR 4 open (N-007,
N-065, N-066, N-072) · INFO 0 · FIXED/ 208 (M-041 + the parallel wave's
landings) · tracked-open: OQ-FLT-1/OQ-Z5/OQ-Z2 resolved this window by the
parallel lanes.

## Session-33 delta, part 3 (2026-10-26 — DFX33: M-018 FIXED — the FilterFolder result-set junk filter)

**M-018 FIXED (MAJOR — the og's PushFolderFilter result-set policy):**
the corpus ini was re-read BYTE-EXACT first-hand (the finding carried two
typos vs the shipped file: `._.DS_Store` → the file's `._.DS_Store`,
`新建資料夘` → `新建資料夾`) and converted to generated data
(scripts/gen_filterfolder.py, the gen_dict/raw_signs precedent: 4 folder
+ 7 file + 14 fat-name entries). The pass (`filterfolder::apply`) runs
post-walk/pre-grading/pre-session-capture and marks matching nodes
NODE_FLAG_JUNK_FILTERED (bit 14, tree-internal); the query surface
(passes), the CLI tree and recover-all planning all skip flagged nodes —
sessions/reports/recovery agree with the og's presented tree, and
recover-all never writes the hidden set (the finding's own impact line).
Children of filtered nodes stay (the og hides entries, not contents).
ScanStats.junk_filtered added. Policy registered as D-82 (comparator
mode undecoded in the corpus: exact+ASCII-fold chosen; applied at
emission; sessions re-derive on resume). Proof:
FilterFolder.CorpusListsPinned (every entry of all three lists incl. the
10-locale New Folder family + SVI + the BitLocker artifacts; count pins;
negatives) + Orchestrator.JunkFilteredFromResults (fail-before EXECUTED
RED with the pass disabled: a FAT16 volume with DESKTOP.INI, $USNJRNL,
LFN System Volume Information and LFN zh-CN 新建文件夹 — junk_filtered
== 4, flags on exactly the junk nodes, KEEP.ME/DOC.TXT visible, the
query excludes the hidden set); suite **823/823** linux-release at the
fix commit (the N-019 scenario-8 test from a parallel lane converged to
the M-020 default-at-root rendering in the same commit — its leakage
assertions preserved).

**Post-fix counts (this landing):** SEVERE **0** · MAJOR **8** open ·
MINOR per the live union · INFO **0 open** · tracked-open 6.

# SUMMARY.md — executive summary (diff-fix session 24 (DFX24) on top; earlier sessions below)

## Session-24 delta (2026-10-26 — DFX24: the MAJOR lane — baseline repairs + the M-014 UTF-16-table completion + N-079)

Bootstrap: this lane claimed the open MAJOR tier at `02b950f` (baseline
737/737 linux-release verified green first-hand at the claim tree; corpus
`03ad46c` unmoved). The window's parallel lanes landed heavily during
this session's first fix (M-014, M-020, M-029 first landings among
them); every union was absorbed by pull + first-landing-wins, and this
lane's convergent M-014 implementation was DROPPED cleanly in favor of
the parallel landing — except the one piece the landing left open on a
factually wrong premise, which this lane completed (see below).

**Baseline repair 1 (pushed-red main, D-84 ledger row):** the DFX30-T4
delta-ledger renumbering renamed the N-071 two-stack row D-79→D-84 in
the ledger comment but dropped the ROW itself —
`Enumerate.ScopeDeltaRegisteredWithRevisitTrigger` (landed with the
N-071 test file) went red on the union. Row restored verbatim from the
canonical landing `7ee64fb` with the renumbered id. (A convergent
repair of the BtrfsTwoSubvolumesRootSeparately union-red landed by a
parallel lane first and was taken as-is.)

**N-079 NEW-FINDING (MINOR) filed + FIXED — the ASan lane back to
green:** the N-062 landing's `cr::log::set_sink` published each sink
through a bare `new Site` and never freed — LeakSanitizer red on the
ASan lane (4 tests: the three Orchestrator log tests +
CrUi.HeartbeatDuringStall; every report named `cr_log.cpp:55`;
reproduced first-hand at the baseline tree with the fix stashed). Fixed
with a `static std::deque<Site>` pool under the existing write mutex —
deque `push_back` preserves element addresses, so the lock-free
reader's no-UAF invariant is kept EXACTLY; retired sites stay alive for
the process (the intentional never-free-while-racing posture) but the
storage is owned and destroyed at exit. ASan lane 750/750 at the fix
tree, 826/826 on the post-union tree.

**M-014 completion (MAJOR — on top of the parallel first landing):**
the first landing left the PTR_180045a00 UTF-16 name-validity table
unimplemented on the premise "its bytes are not recoverable from the
corpus (no raw binary ships)". The premise was WRONG —
`og-codebase/exFATSearchDLL.dll` ships the binary. This session
extracted the 256 bytes at .rdata RVA 0x45A00 (file offset 0x44600,
PE section map), pinned the byte order by RAW DISASSEMBLY at VA
0x180003030 (invalid iff low byte == 0 AND table[high byte] == 0 —
chars with nonzero low bytes are never table-checked), and wired the
check into `name_fragment_gates_ok`. Proof:
`ExfatEngine.NameUtf16TableInvalidCharRejected` fail-before EXECUTED
(RED with the table loop disabled / GREEN with it);
`NameUtf16TableAllowedCharAccepted` pins the U+1A00-passes boundary.
The B10 (names) MATCH verdict now stands without the residual-FP
asterisk. (This lane's independent verification of the other three
gate families — including the raw-disassembly confirmation of
FUN_180002f90's timestamp gates and the all-zero-FAILS correction —
converged with the landing and is recorded in the finding file.)

**M-052 FIXED (MAJOR — the FAT/exFAT recycle-bin $I/$R pairing):** the
NTFS engine's in-repo reference model extracted into the shared
`cr/recycle` helper (name + content-stream parameterization, the fix
plan's step 1 verbatim) and wired into both walks: the three corpus
roots ($RECYCLE.BIN / Recycled / Recycler, case-blind) mark their
subtree; $I streams parsed (v1/v2 + the Win10+ variable form) and
never surfaced; $R files remapped to their ORIGINAL name/size/
deletion-time and marked Deleted (in-use inside the bin, deleted from
the user's location — the NTFS model); post-walk fixup for late $I
pairs. NTFS itself now consumes the shared helper through attribute-
view glue (identical behavior). Fail-before EXECUTED (engine mutation,
3/4 RED; the over-matching pin green by design). Stats
recycle_info/recycle_remapped on both engines.

**Baseline repair 2 (pushed-red main, committed conflict markers):** a
parallel merge (the 9a9340e/196d75a lineage) committed half-eaten
conflict markers into include/cr/win/cr_bitlocker.h and a structurally
broken src/app/rrecov.cpp — win_syntax_check RED (the gate's exact
purpose). Repaired as the canonical union: the 6afa8fc reconciliation's
cr::dict design (the standing first-landing-wins ruling) + the N-033
CLI additions (list-carve-formats + the cr_ui include). Fail-before:
win_syntax_check RED on the marker tree, green post-repair.

**Post-fix counts (live file inventory at this session's push):**
SEVERE 0 · MAJOR 7 open (M-018, M-030, M-034, M-039, M-040, M-041,
M-048) · MINOR 5 open (N-007, N-059, N-065, N-066, N-072) · INFO 0
actionable (all acknowledged) · FIXED/ grows with every parallel
landing (live count at the RESOLVED-INDEX; the window's parallel lanes
closed M-014/M-020/M-025/M-029/M-035/M-052 among the MAJORs).
**M-034 FIXED (MAJOR — resolved by scope registration):** the NAS/
remote-layer finding existed to force a disposition that had never
been made anywhere; PARITY-DELTAS D-88 now registers the exclusion
(what the og does: CRemoteDevice/CDoScanRemoteDevice + EuMrx; what
the core does: five local-device layers, transport-agnostic contract;
why: the EuMrx interop is host plumbing, the corpus proves scan-path
plumbing only — the REVISIT TRIGGER is a decode of the og's actual
deleted-file yield over SMB). cr_device.h divergence block +
`Storage.ParityDeltasListsNasExclusion` doc-truth pin.

**Convergence record (M-052):** a parallel lane (DFX35) landed a
convergent M-052 implementation that hit origin first and is
corpus-richer on one axis (the og's FOUR-name root set incl. the
"$RECYCLE.BI0" FAT-mangled form, verified in all three SearchDLLs);
their inline-header API is canonical after the union, this lane's
three-root .cpp implementation dropped cleanly. Their landing also
filed+fixed N-079 (the NTFS bin-scope defect — out-of-bin $I/$R
handling); this lane's log-seam leak finding renumbered N-079→N-080
(same-window ID collision, first-landing-wins). Two more pushed-red
unions repaired this window (the M-052-era source-file markers; the
re-broken rrecov.cpp/cr_bitlocker.h — the canonical cr::dict design
restored both times).

**M-039 FIXED (MAJOR — the cross-engine occupancy gate):** the
orchestrator aggregates every STRUCTURED node's extents into a
sorted/coalesced interval set (the fix plan's step 1 verbatim — the
emit path already built the extents) and feeds it to both carve
dispatch layers: in-interval positions are suppressed, the grammar
layer jumps to the interval end (the M-005 cluster-jump analog at
extent granularity), the raw-sign 512-strides suppress the same way.
Carved nodes never feed the gate (resume re-carve protection); the
dedup tree stays the safety net; interval-set memory shape (the
finding's "interval set first"). occupancy_skipped telemetry.
Fail-before EXECUTED (gate neutralized: 2/3 RED; the straddle pin
green by design). Suite 895/895.

**Post-fix counts (live file inventory at this session's push):**
SEVERE 0 · MAJOR 2 open (M-030, M-040) · MINOR 1 open (N-072)
· INFO 0 actionable · suite **895/895** linux-release. Next on this
lane: M-030 (transport) → M-040 (boot-chain disposition).


## Session-34 delta (2026-10-26 — DFX34: the N-064 convergent-landing reconciliation + the MINOR-tail lane)

Bootstrap at `65122f0`-lineage (the window's parallel claim wave absorbed
by union merges; ID re-stamped 27→32→34 across three push races, each
squatted pre-landing — first-landing-wins honored). Baseline linux-release
**737/737** green FIRST-HAND at the claim tree; corpus `03ad46c` unmoved
(verified at first corpus read). Lane: the MINOR tier tail back-to-front,
20+ fix target, groups per sub-fix, every parallel landing absorbed.

**N-064 — convergent two-lane landing RECONCILED (canonical: DFX31
`5a29dbd`, first-landing-wins):** this lane's independently developed
fix (`817740c`: pure `cr/pwddict` + `--bitlocker-pwddict <path|seed>` +
`try_password_unlock`; corpus Dict format re-verified BYTE-LEVEL
first-hand; 9 `Pwddict.*` tests; executed fail-before RED) reached main
in the same window as DFX31's `cr::dict` landing (generated corpus-exact
228-entry table per the raw_signs.json conversion precedent + the
consent-gated loop + `--bitlocker-dict` + `cr_ui_unlock_volume_with_dictionary`
ABI). The reconciliation kept the canonical surface and REMOVED the
duplicate: the pwddict module/flag/plumbing/tests deleted; this lane's
D-78 divergence row WITHDRAWN (obsolete — the canonical ships the
corpus-exact table; no divergence exists); the win-stub CP_UTF8/
MultiByteToWideChar declarations kept (the canonical widen needs them);
the format-compatible dictionary FILE loader recorded in the finding's
Convergence addendum as an unregistered future superset (NOT a parity
requirement — the og ships its table; user candidates arrive via the
cr_ui ABI). The reconciliation ALSO repaired the parallel-window merge
corruption (conflict markers left in rrecov.cpp/cr_bitlocker.h/audit
docs by an earlier union merge — main was RED on win_syntax_check) and
hardened the gate with `-D__declspec(x)=` (gcc on ELF has no __declspec
keyword; the app layer's new cr_ui.h include needs it — parse-gate
only). Reconciled tree: suite **810/810** linux-release green, win
layer parse-clean. Full detail: the finding's Resolution + Convergence
addendum.

**Post-reconciliation counts (this lane's tree, parallel lanes active):**
SEVERE **0** · MAJOR 11 · MINOR 8 · INFO 0 open · **FIXED/ 190 files** ·
tracked-open 9 OQs. This lane's remaining MINOR-tail queue: N-071 →
N-072 → N-066 → N-065 → N-059 → N-033 → N-007/I-004 → N-019 (the tier's
middle drained by the parallel lanes' landings — absorbed per
first-landing-wins), then MAJOR leftovers / tracked-open assist.


## Session-32 delta (2026-10-26 — DFX32: M-029 FIXED — the Date/Size bucket-ID translation + the general-raw naming surface; the query finding fully closed)

Baseline 737/737 linux-release verified first-hand at the claim tree (02b950f
content); corpus `03ad46c` unmoved. Lane: the unclaimed mid-MAJOR cluster
(claimed at bootstrap after the same-window ID churn — the ex-DFX26 lane
holds DFX30/MAJOR-tail, DFX27→DFX35 MAJOR-head, three lanes hold MINOR,
the OQ queue is split head/tail between DFX30/DFX31).

**The decode the finding was blocked on** (all first-hand,
ghidra-drwui/DRWUI_decompiled.c): the og's filter-bar bucket menus —
CTimeFilterBtn FUN_140705c50 (0x100 Today / 0x101 Yesterday / 0x102
"Last 7 days" / 0x103 "Last 1 month" / **0x104 SKIPPED** / 0x105 Custom)
and CSizeFilterBtn FUN_140706060 (0x1000 "< 512KB" / 0x1001 "512KB~1MB" /
0x1002 "1MB~512MB" / 0x1003 "512MB~1GB" / 0x1004 "> 1GB" / **0x1005
SKIPPED** / 0x1006 Custom — the audit's "<128MB/<1GB/<5GB" class-table
approximations corrected by the decode); the bounds initializer
FUN_140254670 (today/yesterday/−7d/−30d/−365d cached as day-truncated
LOCAL time, COleDateTimeSpan arithmetic); the node-side tag setters
(date tags node+0x89 bits 11-14; size-bucket tags bits 17-22 with
FUN_140255820's exact mutually-exclusive inclusive boundaries); the
custom-range matcher FUN_140256830 (local-day-truncated, inclusive both
ends).

**Landed:** `query::apply_date_bucket` / `apply_size_bucket`
(DateBucket/SizeBucket enums; skipped + unknown IDs REJECTED — no
invented constants) + the `cr_ui_query_apply_buckets` **added export**
(ABI-CONTRACT §9: no bump) + `ScanConfig::raw_category_name` /
`cr_ui_scan_config.raw_category_name_utf8` with resume-safe raw-root
matching (the "filter_generalraw" surface per the OQ-S4-2 correction —
the narrowed "server-side push"). 5 tests, all fail-before RED on the
stash-neutered tree (translation returning false / the default name
hard-wired), GREEN after; suite 805/805 linux-release on the final
union (the window's parallel landings absorbed: M-049/M-014/M-020/
N-050/M-058 + the OQ resolutions).

**Counts after this landing (live tree):** SEVERE 1 · MAJOR **10** ·
MINOR 8 · INFO 0 open · tracked-open 10 · FIXED/ **190 files**. M-029's
every sub-item is now closed across sessions (path group s6; Type sort +
modified-unit + paging s11; buckets + naming s32).



## Session-33 delta, part 2 (2026-10-26 — DFX33: M-025 FIXED — the carve read model: window-serving reads, 13x fewer device reads measured)

**M-025 FIXED (MAJOR — the carve scan's read model):** the og's
already-in-buffer model is now the core's: scan_one_window wraps the
slice in a WindowDevice for the window's duration — reads inside the
window are served from the buffer, reads straddling a window edge split
into buffer-prefix + device-tail (byte-identical by construction). Every
grammar validator call, both M-051 checksum sites, both hash sites and
the raw-sign tail scans read through it (the M-021 hook keeps the raw
slice — host contract clarity). MEASURED, the counting-device execution
the finding's own test list prescribes: 4 MiB fixture, 4 small PNG
plants, hashing on — **52 device reads pre-fix → 4 post-fix** (the
window fills only; zero per-candidate re-reads for in-window
candidates); straddling probes (1 MiB JPEG/MP3 blocks a 1 MiB window can
never contain) degrade to prefix+tail, never more bytes than before.
Geometry (1 MiB windows, byte-granular dispatch) intentionally unchanged
per the finding's own bench-evidence rule — the I-009 mid-sector
superset stays; the stateless-vs-pending model difference registered as
PARITY-DELTAS D-81 (the S-017 porting boundary). Proof:
CarveEngine.WindowServingEliminatesPerCandidateRereads (fail-before
EXECUTED: 52 vs 4, RED on the pre-fix wiring) +
CarveEngine.WindowServingServesStraddlingReadsCorrectly (exact
extent/length/checksum on a boundary-crossing plant); carve suite 46/46;
full suite **799/799** linux-release at the fix commit.

**Post-fix counts (this landing):** SEVERE **0** · MAJOR **10** open ·
MINOR per the live union · INFO **0 open** · tracked-open 11.


## Session-33 delta (2026-10-26 — DFX33: M-020 FIXED — the Btrfs profile machinery + default subvolume)

Baseline 737/737 linux-release verified first-hand at the session's union
tree (02b950f + the parallel claim landings); corpus `03ad46c` unmoved.
Lane: first-landing-wins on the MAJOR tier (M-020 first).

**M-020 FIXED (MAJOR — Btrfs chunk profiles + default subvolume):** the
corpus was re-verified line-by-line BEFORE the fix and the audit's Summary
carried a decode error, corrected in the record: the og's read path
(:2498-2583) serves ONLY the 2-copy class (SINGLE/DUP/RAID1 → rotating
mirror, :2534-2540); RAID0/RAID10 classes FALL TO THE FAILURE RETURN —
the og reads NOTHING from striped chunks (the audit's "2-copy profiles
(DUP/RAID1/RAID10) through a rotating mirror" was wrong; its own quote
block was right). Implemented og-exact: the RAID10 parse-time geometry
gate (:2162-2168), the per-chunk rotating mirror index for the 2-copy
class, the default-subvolume DIR_ITEM pick ((6, 0x54, 0x8dbfc2d2)
"default" with a ROOT_ITEM located key; root-5 fallback; :3975-4060) with
the default's children rendering at the volume root (ReFS convention) and
non-default subvols keeping their N-047 containers. Registered superset
D-80: RAID0/RAID10 kernel stripe math (the pre-fix stripe-0 read emitted
wrong bytes on striped chunks; now mapped correctly or refused+counted —
`unmappable_extents`), raw extents split at stripe boundaries,
compressed extents stay atomic. Fix-plan item 3 (error-map) verified
already-by-construction (the Skip-mode RetryingDevice pipe records
sector-granular failures below the engine; no duplicate recording).
Proof: 4 new fail-before tests (Raid1RotatingMirrorReads,
Raid10GeometryGate, Raid0StripeMathSplitsAtStripeBoundaries,
DefaultDirItemNonSubvolumeLocationFallsBack — all RED on the
pre-fix-behavior tree, 4/4) + the updated EndToEndWalk /
SubvolumeNamedFromRootRef (default-at-root expectations, RED on the same
tree); suite **747/747** linux-release at the fix commit (parallel-lane
test landings included in the count; incl. win_syntax_check).

**Post-fix counts (this landing):** SEVERE **0** · MAJOR **13** open ·
MINOR 18 (per the union; parallel lanes may land more in-flight) · INFO
**0 open** · FIXED **174** files · tracked-open 11. Next on this lane:
M-025 (carve 8 MiB pingpong → streaming) or whatever remains open at the
next pull.


---

# SUMMARY.md — executive summary (sessions 27+30+31+35 on top; earlier sessions below)

## Session-27 delta (2026-10-26 — DFX27: the MAJOR-head lane; M-058 NEW-FINDING filed + fixed; M-049 convergence)

Baseline: linux-release **743/743** at `db3de2c` (post-merge with the window's
parallel claims; suite re-verified after every pull). Corpus `03ad46c` unmoved.
Lane: the MAJOR tier head (M-049 → M-035 → M-052 → M-014 → …), disjoint from
the parallel lanes' tail/mid claims.

**M-058 NEW-FINDING (MAJOR) filed + FIXED (found during M-049 verification):**
reasoning about `retry_raw`'s lifetime on every path (the M-049 error-context
stamping) exposed that run()'s cache-failure fallback dereferenced the retry
device `CachedReader::create` destroys on its failure returns — a latent
use-after-free on the defensive path (unreachable at run()'s constant geometry,
**executed under ASan** in the standalone repro — evidence/executions/
s27-m058-readpipe-uaf-asan.md) and the direct contradiction of the adjacent
comment (which documented the bare-slice fallback). Fixed by extracting
`build_read_pipe()` with the degraded path as a pinned contract; 3 ReadPipe pin
tests; the N-045/D-72 budget pin re-anchored to the call site (lockstep
preserved). Suite **746/746**.

**M-049 convergence (first-landing-wins):** the parallel DFX24 lane landed
the error-context carrier (`8ba2316` — per-thread RAII stamps + the retry
device's owner fallback + the err_module/err_operation/err_attempts event/ABI
triple; 8 tests, suite 745/745 at its tree) while this lane's duplicate
implementation was mid-flight. The duplicate was dropped; the landing was
reviewed sound on the merged tree (suite green below).

**M-035 FIXED (MAJOR — the engine-side FileFilter validation):** the og's
exFAT-only lost-candidate content-consistency scorer (FUN_180025390, re-verified
line-by-line first-hand incl. the classifier and the filter-set helper) is now
the `cr::lost` scorer: considered/filterable/matched counters decompile-exact,
the reject rule `considered/2 <= filterable && ratio < 0.3 && filterable != 0`
(STRICT < — corrected from the decompile's `<= 0.3 && != 0.3`; the finding
text said `<=`), the substring classifier + MOV/MP4/JPG family specials,
out-of-bounds data-start invalidation. scan_candidate scores the walked files
after the engine dispatch and detaches the subtree on reject (the new
ResultTree::detach_subtree — the memory-safe drop mechanism), zeroes the
counts, bumps ScanStats::lost_content_rejected. The candidate stays listed
(per-hit persistence, M-042). 15 tests (the finding's 3 named + helpers +
detach + 2 end-to-end whole-disk ghost scans); fail-before EXECUTED; suite
**808/808**. Corpus corrections: the reject boundary is strict-<; the finding's
"failed" label for iVar17 is the CHECKABLE count (the majority gate is
checkable-dominated, not mismatch-dominated). This lane proceeds per the open
inventory (M-052/M-018/M-020 contested with the parallel lanes — pull-check
before each).


## Session-30 delta (2026-10-26 — DFX30: the tracked-open tail lane — OQ resolutions + NEW-FINDING S-034)

Bootstrap: absorbed the window's parallel claims (DFX23 MINOR, DFX24 MAJOR,
DFX25 INFO-closure, DFX26 MAJOR-tail, DFX27 MAJOR-head, DFX28 MINOR-rear)
via union merges; this lane claimed the DISJOINT tracked-open TAIL.
Baseline **737/737** linux-release green at the claim tree (re-verified
**749/749** after absorbing the parallel lanes' test additions, before this
session's first source-touching commit); corpus `03ad46c` unmoved.

**S-034 NEW-FINDING filed (SEVERE — from OQ-Z2's verification):** the og's
ZFS quick-searcher walks EVERY uberblock slot in the label-0 ring — per-slot
`txg` fed to `FUN_180003150` ("addBlock") with a per-txg
`VdevLoc::UberData` registry (`FUN_180017310`), per-state tree walks, and
`ZplNode` emission via a `bool(shared_ptr<ZplNode>)` visitor; "from_txg: 0"
descends unbounded. The core reads the newest txg only, on a parity premise
("the og reads the newest txg's live state only" — engine header + D-05)
that this session REFUTED first-hand (D-05's corpus citation over-read a
neighbor-txg ring comparison as newest-only selection; corpus erratum E45
filed). Files deleted within the ring's txg window are og-visible and
core-invisible (RAW carving is the core's only coverage — a strict subset).
Doc-truth landed at filing: engine header comment corrected, D-05
re-verdicted REFUTED-PREMISE→open gap, PARITY-CORE-FS2 zfs deleted row →
DIVERGENCE-SEVERE, matrix B/D cells updated. The behavioral fix is queued
behind three decode items (cross-txg emission/dedup semantics in
`ZplNodeFile`'s registration path; MOS snapshot re-emission; `last_txg`'s
role) — no guessed parity. Suite 749/749 at the filing commit.

**OQ-FLT-1 RESOLVED (M-035's blast-radius question):** the 169-entry
extension table (char[9], "Raw" first, all-zero sentinel slot 169) is
byte-identical to DRWUI's grading set and present in 4 engines — live in
FAT (2 consumer sites: the `< 0xAA` index-gate strstr filter path +
whole-table category iteration), exFAT (M-035's classifier+scorer), RAW
(index→name accessor); a DEAD .rdata copy in NTFS (zero code+pointer refs);
absent in EXT4/Btrfs/ReFS/HFS+/ZFS. The validation scorer
(majority-mismatch + ratio<0.3 reject) is exFAT-ONLY (zero 0.3-compare
sites elsewhere). M-035's fix scope stays exFAT; the shared extension-INDEX
vocabulary recorded for N-049/N-050. Evidence:
`evidence/decompiled-quotes/s30-oq-flt1-extension-table-distribution.md`.

**OQ-Z5 RESOLVED:** ZFS quick/complete = the product-wide
`IQuickFileSystemSearchController` (16 slots) vs
`ICompleteFileSystemSearchController` (34 slots) plugin split (RTTI-walked
first-hand). Complete adds streaming per-hit emission (std::function +
sector math + the ub-magic/ZEC_MAGIC label probe — zec at +0x1d8 = the
512-byte-slot assumption vs the core's kernel-correct shifts-10..13 ring
probe), a restart entry, and five config channels; quick is a real one-shot
scanner (bulk single-value collect; the BIGGER object, 0x120 vs 0xe8). The
core's orchestrator-layer quick/deep split is the union — no action.
Evidence: `evidence/decompiled-quotes/s30-oq-z5-zfs-quick-vs-complete.md`.

**Post-session counts (DFX30's lane complete at `49eed47`; parallel
lanes active in the same window):** SEVERE **1** (S-034, new — the
session's NEW-FINDING, fix queued behind its three decode items) · MAJOR
14 · MINOR 18 · INFO 0 open · FIXED/ 171+6 (the session's closed OQs:
T6-T7, S6-3, S6-2, S6-1, S5-1 + the DFX25-index rows) · tracked-open
reduced to **4 files** (OQ-RS, OQ-S2M-2, OQ-S3, OQ-S4-3 — DFX25's head
queue; OQ-S20-1 resolved by DFX31; OQ-Z-E-FLT stays open for OQ-FLT-2
alone, pushed forward with evidence). Session-30 dispositions: 8
sub-question resolutions, 2 convergent closures (S6-2, S5-1), 1
convergent closure WITH correction (S6-1 — DFX25's "not the registry"
refuted by the FUN_180022430 mode dispatch), 1 push-forward with new
evidence (OQ-FLT-2 → M-029), S-034 filed, D-09 re-verdicted, corpus
errata E45 + E46-convergence, M-035's blast-radius question answered,
mom-capabilities.md CheckVersion row closed.

# SUMMARY.md — executive summary (sessions 30+31+35 on top; earlier sessions below)

## Session-28 delta (2026-10-26 — DFX28: the rear-first MINOR lane)

Bootstrap: absorbed the window's parallel claims (DFX23 MINOR front-first,
DFX24 MAJOR, DFX25 INFO-closure 22/22, DFX26 MAJOR-tail, DFX27 MAJOR-head)
via union merges; this lane works the open MINOR tier from the REAR
(converging toward DFX23's front-first progress; ALREADY-FIXED
classification on collision). Baseline **737/737** linux-release green at
the claim tree (`354c555` content); corpus `03ad46c` unmoved.

**N-075 FIXED (MINOR — the event-bus contract shape):** the finding's own
"cheapest honest close" executed — `cr_ui_scan_busy` +
`cr_ui_scan_stopping` synchronous query exports (v2-era append, no bump):
the og's `sigIsBusy(bool&)` ×14 / `sigIsStopping(bool&)` ×8 reference-out
query signals surfaced at the ABI boundary (busy = the live run-lifecycle
atomic; stopping = the LIVE token cancel request, persisting after the
one-shot run — distinct from telemetry's post-run cancelled flag). The
pull-polled progress convention registered as PARITY-DELTAS **D-78**
(renumbered from a D-77 collision with the parallel lane's N-009d ACL
register — the errata convention; the og's 38-consumer poll farm vs the
core's coalesced push + any-thread telemetry pull); the typed
`IUIProgressInfo` mapping documented in ABI-CONTRACT §5
(recovery_progress_fn + queue_count + get_stats + report_json —
deliberately no parallel interface). Proof:
`CrUi.ScanBusyStoppingQueries` (mid-run pause-gate window: busy observed
live, stopping flips at cancel, one-shot persistence after return) +
plain-C edges in `cr_ui_abi_check`; fail-before EXECUTED at compile (the
symbols did not exist). Suite **738/738** at the fix commit.

**N-019 FIXED (MINOR — the 13-scenario per-parameter regression-test gap,
after full per-scenario re-verification on the current tree):** 6 scenarios
ALREADY-FIXED by later landings (ADS boundary, sparse ratio, FAT cross-link,
HFS+ resource fork, LDM stripe alignment, LDM mirror read — covering tests
re-cited in the Resolution); 1 SUPERSEDED (the overlap-scoring premise —
M-017's content-driven model replaced the penalty table the finding cited);
1 N/A (exFAT multi-fork); 1 routed to OQ-T6 with NEW evidence (ZFS snapshot:
the og surfaces DSL snapshot roots per the corpus capa digest; core
negative-space verified — the OQ's next decode target is now precise);
4 tests WRITTEN with executed mutation fail-befores:
`NtfsVolume.HardLinkDestroyRuleInteraction` (the destroy decision is
per-RECORD upstream of the fan-out — destroyed once, both links vanish
together in fan-out mode, partial reuse keeps PartialRisk),
`Ext4Engine.ExtentTreeDepth3` (root depth 3 → index → index → leaf; two
disjoint runs, byte-exact content markers),
`Btrfs.TwoSubvolumesRootSeparately` (subvol 5 + subvol 256; every file
under its own root, no leakage either direction), and
`Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk` (the honest rescope of
the corpus-silent integrity-stream scenario: property-flag rows 0x108/0xf8
+ unknown-type rows walked past, data byte-exact, granularity guard quiet).
Suite **753/753**.

**N-033 FIXED (MINOR — the CLI half + the refresh divergence):** the ABI
half landed session 6 (`cr_ui_carve_format_count/info`); this session
closed the rest — `rrecov list-carve-formats` (subcommand form, one row
per format over both carve layers, the FB-K01 exit discipline: an empty
registry or failed row is exit 1, never a silent "nothing to carve") +
the per-scan registry-refresh divergence REGISTERED as PARITY-DELTAS
D-80 (the og's FS engines LoadLibrary/clone the RAWSearchDLL registry
per scan — multi-DLL machinery; the core reads its compile-time
immutable registry directly, same content by construction) +
ABI-CONTRACT §7a's CLI projection note. Pinned by the existing
`CrUi.CarveFormatRegistryEnumeration` + the win-layer parse gate
(extended with `-D__declspec(x)=` parse-neutralization — ELF g++
rejects the `__declspec` that cr_ui.h's _WIN32 branch uses; the gate
checks structure, MSVC CI exercises the real macro).

**Pushed-red repaired en route (the M-055/M-057 family):** a same-window
parallel merge had committed conflict markers into rrecov.cpp +
cr_bitlocker.h + PARITY-DELTAS + CAPABILITY-MATRIX (win_syntax_check
RED on origin/main — executed first-hand). The two N-064 designs were
unioned (both consent surfaces: `--bitlocker-pwddict <file|seed>` AND
`--bitlocker-dict`; both functions stand; auto → shipped-dict →
file/seed → refuse-with-guidance); my session's M-020-stale
TwoSubvolumesRootSeparately re-verified and updated. 827/827 green
post-repair.

**Post-fix counts (session-28, after N-075 + N-019 + N-033):** SEVERE
**0** · MAJOR **14** · MINOR **5 open** (N-007, N-059, N-065, N-066,
N-072 — the parallel lanes drained the rest of the tier) · INFO 0 open
(22 acknowledged in FIXED/) · **FIXED/ 176 findings** (177 files incl.
RESOLVED-INDEX) · tracked-open 11 (OQ-T6 pushed forward with new
evidence). Next on this lane: N-066/N-065/N-059/N-072 rear-first.

## Session-31 delta, wave 4 (2026-10-26 — DFX31: OQ-S20-1 resolved — the BitLocker derivation premise corrected: no stretch exists in the og; M-033's surface IS the og's surface)

**OQ-S20-1 RESOLVED (premise-corrected, zero code change — the corpus
wins):** the decode proves the og's BitLockerLib does NOT implement the
standard recovery-password stretch (no 0x100000/0xFFFFF/0xF4240 loop
constant; the verified SHA-256 layer — FUN_1800011a0/:1da0/:1ea0 — has
three non-iterating consumers; the generic KDF framework defaults to
10000 with only the vendor "BITLOCKER"-labeled KDF at FUN_18003fd80) —
user-key unlock is DELEGATED TO WINDOWS (PSUnlockBitLocker
`Unlock-BitLocker -MountPoint`, the manage-bde fallback; the auto path
applies DRWService's external keys). The og's in-stack surface = the FVE
metadata parse + protector enumeration (SetProcessProtectorKey's
protector_t = 36-char GUID/type@+0x25/material@+0x26) + the 64-byte FVEK
extract (GetFullVolumeEncryptionKey) + the XTS decrypt — **exactly the
core's M-033 posture; D-75 CONFIRMED**. The KDF/AES-CCM/BEK port is
registered as a deliberate-superset scope decision (Windows-free unlock —
valuable where even the og's delegation fails, but an IMPROVEMENT, not
parity). Evidence: s31-oq-s20-1-bitlocker-derivation-scope.md.
**Counts after this landing: tracked-open 7** (OQ-Z-E-FLT, OQ-T6-T7,
OQ-S4-3, OQ-S5-1, OQ-S6-1, OQ-S6-2, OQ-S6-3 — all owned by the parallel
DFX30 tail lane). My lane's queue is drained; next: OQ-S4-3's
push-forward addendum, then absorb any orphans the parallel lanes leave.

## Session-31 delta, wave 3 (2026-10-26 — DFX31: OQ-RS resolved — the ReFS system-table set decoded and FIXED; 6/8 probes closed)

**OQ-RS RESOLVED (RS-2..8 dispositioned; one code fix with fail-before
proof):**
- **RS-7 FIXED:** the og registers system-table handlers for EXACTLY
  {0x500, 0x600, 0x701} (ReFSSearchDLL ctor :600-620) and NORMALIZES any
  id > 0x700 to the 0x701 family (:1150-1154). 0x520 appears nowhere in
  the engine — the core's FSMETADATA_TABLE=0x520 was an uncited T10-era
  guess. The fallback root-detection and the orphan sweep now exclude
  the og's system families {0x500, >0x700}.
  `Refs.FallbackRootSkipsOgSystemFamilies` fail-before RED (the 0x500
  system table elected as the user root — hello.txt absent from the
  walk) / GREEN post-fix; suite 755/755.
- **RS-5 RESOLVED:** the .mo ReFS analyser is structural-only (zero
  deleted-evidence handling in 42,013 lines) — the core's deleted
  surfacing is NOT double-counted; I-009's registration parity-safe.
- **RS-6 RESOLVED:** CRefsAppDataRunItem is a data-run item family
  member (sibling of CRefsFileDataRunItem), not ADS recovery — I-012's
  shared-limitation stands.
- **RS-8 RESOLVED:** the og validates only the MSB+ magic per page (the
  two 0x2b42534d sites); no volume-GUID cross-check — the core's dual
  validation registered as a deliberate superset.
- **RS-2/RS-3 SUPERSEDED** by the landed S-020 (resident rows) and
  N-036 (container map) decodes they gated.
- **RS-1/RS-4 PUSHED FORWARD** precisely: fixture-gated (no real ReFS
  volume in corpus; the cited run 40079065019 is an NTFS scenario) —
  the Windows bench stage's real-volume probe is the recorded next step.

Evidence: `s31-oq-rs-refs-residuals.md`; OQ → FIXED/. **Counts after
this landing: tracked-open 8** (OQ-Z-E-FLT, OQ-T6-T7, OQ-S4-3, OQ-S5-1,
OQ-S6-1, OQ-S6-2, OQ-S6-3, OQ-S20-1). Next on this lane: OQ-S20-1 (the
BitLocker FVE metadata grammar + KDF interop leg).

## Session-31 delta, wave 2 (2026-10-26 — DFX31: OQ-S3-JPEG resolved — S-018 RE-FILED CONFIRMED and FIXED: the truncated-JPEG salvage)

**S-018 RE-FILED + FIXED (SEVERE — the canonical damaged-disk photo
recovered):** the s3 INVALID's residual question ("if any sub-analyser
accepts without an EOI, the salvage claim could hold through the
fallback") is answered YES first-hand: the legacy FileJPGAnalyser tier
(dispatcher FUN_18004b170 → JFIF/Exif/generic) walks SOS-gated headers and
its shared post-SOS walker FUN_18004c480 (:57942-99) ACCEPTS AT ITS SCAN
LIMIT with no EOI — h*w*3 from the candidate start (SOF dims) or
20,000,000 (the 200,000,000 global cap /10), device-bounded by the
sliding getter FUN_180042930; the census-documented two-generation chain
makes one accept sufficient. `validate_jpeg` now mirrors the salvage on
every no-EOI exit (SOS-gated, the og's exact limits and sizing), graded
PartialRisk-600 — D-79 registered (the og stamps salvaged files
identically to complete ones; the core grades honestly). Proof: 2
fail-before RED executed on the stash-isolated pre-fix tree + the
complete-file EOI pin; **suite 755/755**. The modern tier's EOI gate
(the s3 correction) is preserved exactly — complete files keep the
900-tier accept.

OQ-S3-JPEG RESOLVED (both questions dispositioned; the continuation
residual dissolved — first-accept-wins dispatch, the fallback dominates
in-window). OQ → FIXED/; S-018's re-file Resolution appended; carve.md
verdict rows updated; D-79 registered.
**Counts after this landing: tracked-open 9** (OQ-Z-E-FLT, OQ-RS,
OQ-T6-T7, OQ-S4-3, OQ-S5-1, OQ-S6-1, OQ-S6-2, OQ-S6-3, OQ-S20-1).
Next on this lane: OQ-RS (the ReFS residual probes), then OQ-S20-1
(the BitLocker FVE metadata grammar leg).

## Session-31 delta (2026-10-26 — DFX31: the tracked-open HEAD lane opens; OQ-S2M-2's three probes RESOLVED)

Baseline 737/737 green at bootstrap (02b950f lineage through the window's
parallel claims); corpus `03ad46c` unmoved. Lane: the tracked-open HEAD set
(OQ-RS, OQ-S2M-2, OQ-S3-JPEG, OQ-S4-3, OQ-S20-1) — disjoint from the
parallel DFX30's tracked-open tail and every finding-tier lane.

**OQ-S2M-2 RESOLVED (all three probes; zero code change — the decode
confirms landed behavior):**
1. **The +0x68 bonus setter located and fully decoded** — per-engine
   scan-path composers, not the serializer: FAT/exFAT carry a two-factor
   ladder (`+0x68 = boot_quality + fs_quality`, each ∈ {10,45,50};
   FATSearchDLL :6946-6969/:7142-7167, exFATSearchDLL :39504-39526; the
   FAT32 fs bits come from FUN_180009960 with the backup-boot verdict from
   the classifier at :8004-8052; the boot pair's bit 0x100 has NO setter in
   the build — its 50-tier is unreachable); NTFS carries the composite
   (flag-weight base FUN_18000dc70 :10538-73 + boot-validation ±FUN_1800097d0
   :7229-33 + MFT-run stitching +30 per fragment / +50 one-shot for
   >3-gap survivals in FUN_180023cb0). The CalcPartition-observed
   "FAT32 20" = **10+10, both ladders at minimum** — no constant exists
   (session-14's zero-hit grep is explained: accumulator latches on
   registers, not immediates). M-017's landed `fs_bonus()` CONFIRMED — the
   flat values are the honest mapping for the core's single-probe
   architecture; the 187-row 6-decimal log parity stands.
2. **The ZFS "pool display name" never exists** — 0 "name"/"pool" hits in
   ZfsSearchDLL (re-verified); DRWUI's FS-name builder FUN_140510080
   (:842335-96) has no case 13 → "Unknow" in the XML `filesystem` field and
   the CalcPartition log; "ZFS partition" (strings:39717) is the GPT
   type-GUID table. The core's label-`name` read registered as a deliberate
   display-information superset (modules/zfs.md).
3. **The Btrfs name-conversion anchor = the DRWUI render layer,
   CP_UTF8/dwFlags=0** — FUN_140163af0 (:264664-89, restore-path invoke
   :357529) + the FUN_1400d9ef0 family (:190981-97); the engine is
   byte-oriented (0 conversion imports, re-verified); S-005 parity holds;
   the predicted residuals N-016a/N-016b already FIXED (N-029/M-010).

Evidence: `evidence/decompiled-quotes/s31-oq-s2m-2-bonus-setter-zfs-name-btrfs-anchor.md`
(20 verbatim quotes, three negative greps re-run). OQ → FIXED/ with
RESOLVED-INDEX row; modules/lost-partition.md + modules/zfs.md updated.
**Counts after this landing: tracked-open 10** (OQ-Z-E-FLT, OQ-RS, OQ-T6-T7,
OQ-S3-JPEG, OQ-S4-3, OQ-S5-1, OQ-S6-1, OQ-S6-2, OQ-S6-3, OQ-S20-1; the
DFX30 tail lane owns the first six of those). Next on this lane: OQ-S3-JPEG
(the fallback/continuation no-EOI decode).
# SUMMARY.md — executive summary (session 31 on top; earlier sessions below)

## Session-31 delta (2026-10-26 — DFX31: the MINOR tail-first lane opens with N-064)

Baseline 737/737 green first-hand (linux-release ctest at `02b950f`,
re-verified 754/754 on the merged tree after absorbing the window's
parallel landings — N-009c/d+M-057 `e1f17bc`, N-025 `e2b4549`); corpus
`03ad46c` unmoved. Lane: the MINOR tier from the tail (the crypto/win-heavy
segment the front-first lanes defer), one finding, one verified fix, one
push throughout.

**N-064 FIXED (MINOR — the Dict password-dictionary unlock aid):** the
og ships a 228-entry common-password dictionary (og-codebase/Dict, 2,742 B
byte-verified; config-data-reference.md :20) wired to BitLockerLib's
manage-bde -Password user-consented unlock and EuOfficeRepairFile's
encrypted-HWPX repair. The core had key-based unlock only (S-014). Landed:
`cr::dict` (pure core — scripts/gen_dict.py generates the byte-faithful
table incl. the og's seven duplicate lines, the raw_signs conversion
precedent; parse() implements the Dict file grammar; try_passwords() is
the consent-gated attempt loop: shipped-then-user order, stop-on-first-OK,
index-only log lines through the N-048/N-061 seam — password material
never logged, the og's qa-logs carry no password text either); the win
layer's `unlock_with_dictionary` (the EXPLICIT CONSENT ACTION riding the
unchanged PowerShell -Password / manage-bde -Password plumbing, grammar
extracted to detail::build_unlock_command for shape tests); the CLI
`--bitlocker-dict` consent flag (locked volumes stay refused without it —
F61 legitimate-unlock-only preserved, structural); the additive
`cr_ui_unlock_volume_with_dictionary` ABI export (M-033 precedent). The
Office/HWPX consumer stays out-of-scope (I-010). Fail-before EXECUTED
(the 4 attempt-loop tests RED on the loop-disabled tree); 11 pure-core +
3 win-lane tests; suite 754/754. Attempt policy documented per the
finding's open question: sequential, no retry, no parallelism.

**N-071 FIXED (MINOR — the two-stack disk enumeration):** the og's
CDiskEnumerator merges five discovery stacks (mom-module-system :57-59;
the QA census shows storage-spaces init + VDS load alongside the plain
enumerate — vendor-logs :13-29); the core had the numbered
PhysicalDrive loop only. Landed: enumerate_disk_interfaces (cfgmgr32
GUID_DEVINTERFACE_DISK present-instance pass — the documented GUID
locally defined; number via IOCTL_STORAGE_GET_DEVICE_NUMBER; the loop's
query factored into query_disk_handle so both stacks share one query
path; table parse through the pure core when numbered) +
merge_disk_enumeration (PURE header-inline merge: dedup by number — the
loop's richer entry wins — then by instance path; unknown-number
entries path-dedup only; any cfgmgr failure degrades to stack 1
untouched). Residual stacks (WMI/VDS/StorageSpaces/FunctionDiscovery/
hot-plug) REGISTERED as D-79 — UI metadata + eventing, not the
readable-device space. cfgmgr32 joined the strict Win7-inclusive import
allowlist (Win2000-era CM pair). Fail-before EXECUTED (3 RED on the
merge-disabled tree); 4 pure tests; win_syntax_check green (cfgmgr32
stub pair); suite 769/769. modules/storage.md's stale local N-055 row
for this finding corrected to the integrated N-071.

**N-059 FIXED (MINOR — the per-FS session extras):** the og's Btrfs
record (+0x3B/+0x3F secondary ids, +0x81 generation/transid qword), ZFS
record (+0x5A secondary flags), and — the session-6 addendum's fold —
HFS+ record (part2 +0xE mode/flags, +0x74 type id) now round-trip:
FileProperties carriers + engine emission (the btrfs inode generation;
the znode ZPL_FLAGS low u16; the HFS+ catalog flags + Finder type) + the
nflags bit-6 fixed 24-byte append-only block. Fail-before EXECUTED (2
RED on the bit-6-disabled tree); 3 round-trip tests; suite 822/822. The
same landing repaired the parallel lanes' N-019/M-020 convergence break
(Btrfs.TwoSubvolumesRootSeparately expectations corrected to the
corpus-verified default-subvol-at-root contract — the union had left
main red).

**N-066 FIXED (MINOR — the identity card's record-3 parity) + N-080
(NEW-FINDING, filed+fixed in the same landing):** the v4 card tail
(firmware/WWN/interface axes + the full 512-byte boot snapshot — the og
record-3's +0x206/+0x30A/+0x414/+0x518) with the scanner capturing the
sector image beside the hash and the PURE enumeration decoders feeding
the axes. N-080: the descriptor decode had read the product at +24 (the
SERIAL offset — model carried the serial text!) and the serial at +36
(inside RawDeviceProperties); fixed to the SDK-exact offsets with the
decode factored into a Linux-testable pure function. Fail-befores
EXECUTED for both (3 RED + 1 RED); suite 855/855.

Counts after this landing: **MINOR 2 open** (N-065, N-072 remain;
N-007/I-004 taken by the front lane), MAJOR 10 open, INFO 0,
tracked-open 6.

## Session-30 delta, part 2 (2026-10-26 — DFX30: N-071 closed — the deferred trio complete on this lane)

**N-071 FIXED (MINOR — enumeration breadth):** resolved by the finding's
own recommended option (a) — PARITY-DELTAS **D-80** registers the
PhysicalDrive-namespace scope with the revisit trigger the open
question demands — PLUS the ported load-bearing effect: query_disk's
0-access metadata-only retry on ERROR_ACCESS_DENIED/
ERROR_SHARING_VIOLATION (a disk share-blocked by backup/AV/imaging
stays listed with busy=true; the og's stacks get this by enumerating
without opening). Option (b) deliberately not built: the Storage
Spaces miss-case is UNVERIFIED-BY-EXECUTION per the finding's own text.
3 source-truth pins (the N-074 doc-truth pattern); win-stub extended
(ERROR_ACCESS_DENIED/ERROR_SHARING_VIOLATION); win_syntax_check green
over all 8 files. (N-064 absorbed by the parallel DFX31 lane
mid-flight — first-landing-wins; the deferred trio's third member
landed elsewhere.)

## Session-30 delta (2026-10-26 — DFX30: the deferred win-layer trio, 1/N-032)

Lane: the MINOR items every parallel lane deferred to last (N-032/N-064/
N-071), worked while the front/middle bands drain elsewhere. Baseline
743/743 at the union tree (737 claim + the parallel N-025/N-009c/d/M-057
landings absorbed); corpus `03ad46c` unmoved. (N-064 was absorbed by the
parallel DFX31 lane mid-flight — first-landing-wins; this lane's N-032
stands.)

**N-032 FIXED (MINOR — the CSMI health transport):** the corpus citation
was deepened before any code: the companion's actual wire was decoded
first-hand from DiskHealth_decompiled.c — GetSmartAttributeCsmi =
FUN_00425450: IOCTL_SCSI_MINIPORT (0x4D008), SRB_IO_CONTROL "CSMISAS"
CC 0x19, vendor fixed area {phy, port, SAS address, Register-FIS
(0x8027) task file, SMART magic LBA 0xA0C24F00, vendor field 0x11},
request 0x2A8, response data @+0xA4, transport-miss GetLastError set
{1, 0x32, 0x37}; plus the og's IOCTL_SCSI_GET_ADDRESS probe and its
legacy SMART IDEREGS (count 1, sector 1, cyl 4F/C2). Landed: pure wire
module (cr_csmi) + the pure transport-chain seam (cr_smart_ata — the
og's CAtaSmart accessor model: the first channel answering IDENTIFY
serves the whole command set; the read flow moved out of the win layer)
+ the three-transport win chain (ata-passthrough → legacy with og-exact
SMART stamping → CsmiMiniportChannel). 9 tests (wire pins + fallback
pair + parse proof); EXECUTED fail-before: the pre-fix chain shape
returns ERR_HEALTH_UNAVAILABLE on the RST-shaped mock. Registered
remainders D-78 (RAID-census routing, LSILOGIC sibling, SMART
auto-enable write — report-only module). Suite **752/752** at the fix
commit (754/754 on the post-union tree with the parallel N-064 tests).
## Session-23 delta (2026-10-26 — DFX23: the MINOR-first closing wave, batch 1 — N-009 group closed + M-057)

Baseline `716d121` verified green FIRST-HAND (linux-release **737/737**,
incl. win_syntax_check) before any change; corpus `03ad46c` unmoved; lane
claim pushed before finding work (protocol). Mandate: MINOR-first queue,
20+ fixes, fixed findings to FIXED/, docs/counts current after every landing.

**N-009 CLOSED (the group's last two halves):**
**N-009c** — the CLI `--filter` ASCII-mangling fixed for real: the new pure
`cr::text::wide_to_utf8` (locale-independent wide→UTF-8; lone surrogate
halves → one U+FFFD per the shared text policy; both wchar_t widths) feeds
all three filter sites (scan print / tree / recover), and the recover path's
match was aligned to the shared ASCII-case-insensitive contract
(`name_contains_ci` factored out of `name_matches` — the query layer's
`name_substring` semantics; the old recover match was case-SENSITIVE, an
internal inconsistency removed). 3 proof tests (`Text.WideToUtf8*` +
`Text.CliFilterUtf8MatchesDecodedNames` — the CJK-filter-matches-decoded-name
byte property, with the old `'?'` polarity pinned to never match).
**N-009d** — resolved by **D-77 registration** (inherit-destination ACL
posture; the finding's own optional-port text; `preserve_dacl` recorded as
the next step, not silently dropped). Finding → FIXED/ (a/b halves were
already fixed in the s2 wave; the group is now fully closed).

**M-057 NEW-FINDING (MAJOR) filed + FIXED (the gate extension's first
catch):** extending `win_syntax_check` to `src/app` (same M-055 rationale —
the app is equally Windows-only) immediately surfaced that **`rrecov.exe`
has been MSVC-uncompilable since `0a68937`** — `cmd_apply` (the S-029 apply
union landing) references `&e` with no `u32 e` declaration. Fixed (the
declaration, matching the sibling cmd_fix/cmd_undo shape) + the gate now
parses all 8 Windows-only files permanently; fail-before EXECUTED (the
exact `'e' was not declared` diagnostic), pass-after 8/8. Q67 buildability
now covers every Windows-only directory (src/win + src/app).

**Post-fix counts (this batch, union with the parallel lanes' INFO
closure; superseded by the session-28 union below — DFX23's N-025 landing
and DFX28's N-075 landing both post-date this paragraph):** SEVERE **0** ·
MAJOR 14 open (M-057 filed+fixed same session) · MINOR **18** · INFO
**0 open** (22 acknowledged — closed into FIXED/ by the parallel DFX25
lane in the same window) · FIXED **173** files (172 findings +
RESOLVED-INDEX — mine +2, theirs +22, overlapping base 149) ·
tracked-open 11.
Suite **740/740** linux-release at the fix commit (+3 Text tests, gate
extended). Next on this lane: N-007/I-004 → N-025 → N-026 → N-027 →
N-031 → N-036 → N-049 → N-059 → N-065 → N-066 → N-075 → N-019 → N-050 →
N-033 → N-072, then the win-layer/crypto-heavy N-032/N-071/N-064.
## Session-25 delta (2026-10-26 — DFX25: the INFORMATIONAL tier closed into FIXED/; tracked-open resolution wave)

Baseline `716d121` verified green first-hand (737/737 linux-release, full
suite); corpus `03ad46c` unmoved. User directive: INFORMATIONAL +
tracked-open first, 20+ resolutions, acknowledged findings moved to
findings/FIXED/, all docs/counts current.

**INFO tier closed (22/22 → FIXED/):** every INFORMATIONAL finding has been
ACKNOWLEDGED since sessions 2-14 (each file carries its own Resolution with
first-hand verification evidence; the DFX14-T5 pass re-verified 22/22 once).
This session re-verified the tier on the current tree before the move:
resolutions present 22/22; the five load-bearing pins executed green
(UserFileNamedDollarNotHidden, Fat16BadClusterStillCanonical,
MirrorSelectionNewestGeneration, NanosecondsPreservedFromExtraField,
ScanCacheBudgetIsTheRegisteredDivergence); two first-hand spot-checks
re-executed (I-001's blanket `$`-skip re-read at NTFSSearchDLL_decompiled.c
:21186; I-022's per-record decompressed-CRC32 re-read at cr_crsf.cpp
:78/:167/:319). All 22 files moved to findings/FIXED/ with RESOLVED-INDEX
rows; the deliberate divergences they register stay preserved with their pin
tests (the INFORMATIONAL safety-hardening rule — no code change).

**Post-move counts (session-25 close, pre-session-28):** SEVERE **0** · MAJOR 14 · MINOR 19 · INFO **0 open**
(22 acknowledged, all in FIXED/) · **FIXED/ 171 files** (170 findings +
RESOLVED-INDEX) · tracked-open 11 (resolution wave below — see the session's
worklog entries for the per-OQ dispositions).


## Session-23 delta, batch 2 (2026-10-26 — DFX29: N-036 — the ReFS container-map half closed)

**N-036 FIXED (the finding's remaining container-translation half; the
recognizer half had landed with S-020):** the invented top-nibble
translate model (feeding a map nothing ever populated) is REPLACED by the
og formula, re-verified first-hand at BOTH decoded consumers
(FUN_180011e90 :13223-13267 + FUN_180004820 :2355-2386, byte-identical):
identity when block < span / map empty / key absent; key
`(((block/span)>>1)-2)*span`; rewrite `(block & (span-1)) | base`; u64
wrap semantics preserved. The span is DBR +0x34 — the og's
volume+0x148 -> parser+0xa8 wiring (FUN_180011cd0) decoded THIS SESSION,
which settles the finding's "container-cluster count" identification —
now `RefsBoot::container_clusters`. The container map is populated from
FUN_180002a60's system-directory records (0xa0/0xe0-length keys with the
{base, span} tail pair; span == DBR+0x34 is the acceptance gate) during
the ObjectID-table walk (zero extra I/O) + a bounded system-table sweep
(IDs < 0x100 — never doubles the user-tree walk). Fail-before EXECUTED:
translate reverted to identity -> `Refs.ContainerRelativeRunTranslated…
PerOgFormula` RED (42*4096 vs 302*4096) AND `Refs.PhysicalRunFlag20…
Untranslated` RED (both files collapsed onto extent 42 by M-027's
placement-signature dedup — the loser never emitted; a genuine
second-order proof). 4 new tests; suite **744/744**. Residual (recorded
in the Resolution, not blocking): the og's record payload carries its key
at +0x20, so an alternative key-relative shape reading exists — the
auditor's reading is implemented with the unit==span gate as the
discriminator; the real multi-container volume probe (the finding's own
open question) settles it and is the bench stage's next step.

**Post-fix counts:** SEVERE **0** · MAJOR 14 · MINOR **17** · INFO 0 open
(22 acknowledged in FIXED/) · tracked-open 11 · FIXED/ **174** files
(173 findings + RESOLVED-INDEX). Next on this lane: N-049 (lost-record
field richness) → N-050 → N-033 → N-064.

## Session-23 delta, batch 3 (2026-10-26 — DFX29: N-049 verified already-landed + the missing pin)

**N-049 FIXED-ALREADY (via M-042's REC_PARTITION_CONTEXT v2, `63a3a77`):**
the finding's full pinned field list — bps, SPC, hidden_sectors, flags
bit0=size_from_device_end — landed with M-042's resume-disk-truth pass
before this session; verified in-source on the current tree (the v2
struct, both save/load sites, v1 back-compat zeroed). The per-hit
crash-safety title half = M-042's per-hit checkpoints + N-014's atomic
rename. The finding's named-but-missing reproduction ADDED:
`ResumeRevalidation.SizeSourceFlagRoundTrip` (flag true round-trips +
the inverse no-false-positive half) — passes on the current tree, the
FIXED-ALREADY proof. The optional capped-evidence list deliberately NOT
persisted (would fight M-042's re-probe-on-resume design — recorded as
reviewed in the Resolution). Suite **754/754**.

**Post-fix counts:** SEVERE **0** · MAJOR 14 · MINOR **13** · INFO 0
open (22 acknowledged in FIXED/) · tracked-open 11 · FIXED/ **178** files
(177 findings + RESOLVED-INDEX). Next on this lane: N-050 (recognition contract) →
N-033 → N-064.
## Session-35 delta (2026-10-26 — DFX35, re-stamped from 23: the MAJOR-closing wave, M-014 first)

**M-041 FIXED (MAJOR — the damage-audit check phase):** the og's
FileSystemCheck.mo is now a core module (include/cr/check/cr_fscheck.h):
a read-only pre-scan audit with the og's error taxonomy in numeric
parity (0x1001-0x100e). FAT pass in the og's phase order (DBR gates —
the ceilings CLAMP, the finding's "validator" framing corrected by the
decompile; hole/duplicate scans; pairwise copy compare; the dir audit
with `.`/`..`, size-vs-chain, cycles, cross-links; the orphan sweep
with the BAD-sentinel set). NTFS pass (volume-size gate, the $MFT /
$MFTMirr payload-name proofs, the version gate {1.2,3.0,3.1}, $Bitmap
structural, the MFT×$Bitmap cross-check with both og strings). Wired as
a pre-scan stage (default ON, opt-out, never aborts; reads the raw
slice so its failures never steal the error map's engine attribution —
the ErrorRange attribution test caught exactly that during the build).
New module doc: modules/check-phase.md. 14 tests (fail-before by
construction — the API did not exist). Suite 877/877.

**Counts after this landing:** MAJOR 5 · MINOR 10 · FIXED 209
findings. Next on this lane: pull-check, then the remaining MAJORs
(M-030/M-034/M-039/M-040/M-048) minus whatever the parallel lanes land.

**M-052 FIXED (MAJOR — the FAT/exFAT recycle-bin $I/$R pairing) +
N-079 NEW-FINDING FIXED (MINOR — the NTFS landing's missing bin
scope):** the recycle model is now shared across all three engines
(include/cr/cr_recycle.h): the $I v1/v2 record parser (format-dependent
pathlen, runner-benchmark evidence), the dot-stripped pairing token,
and the og's FOUR-name bin-root set (Recycler / Recycled /
$Recycle.Bin / $RECYCLE.BI0 — verified first-hand in all three
SearchDLLs; the mangled $RECYCLE.BI0 variant is deliberate og
matching). FAT and exFAT thread the bin context through the directory
recursion (and the M-013 oracle's re-walks): in-bin $I metadata never
surfaces; in-bin $R content remaps to the ORIGINAL name with the $I's
size/deletion-FILETIME, marked Deleted; the post-walk fixup pairs
across walk order ("across bin subfolders"). Verification of the NTFS
reference implementation found it matched $I/$R NAME PREFIXES
everywhere (user files named "$Image.txt" vanished; out-of-bin "$R…"
mis-paired) — filed as N-079 and fixed in the same change (parse-gated
skip + ancestor-chain-gated remap, the og's own scope test). New stats
recycle_info/recycle_remapped on both FAT-family engines. Proof: 3
fail-before EXECUTED tests + the no-over-match guard + the NTFS scope
pin (fail-before EXECUTED) + RecycleBinMapping regression green
throughout; suite 794/794.

**Counts after this landing:** MAJOR 11 · MINOR 10 · FIXED 186
findings (+ N-079 new). Next on this lane: M-020 (Btrfs chunk
profiles + default subvol).

Baseline `716d121` verified green first-hand (737/737 linux-release);
corpus `03ad46c` unmoved. Lane: MAJOR-first per the session mandate,
20+ fixes targeted. Open inventory at start: MAJOR 14 · MINOR 19 ·
INFO 24 · tracked-open 11 (SEVERE/ACUTE tiers empty).

**M-014 FIXED (MAJOR — the exFAT structural entry gates):** the og's
four gate families landed decompile-exact (FUN_180002f90 /
FUN_180002f40 / FUN_180029280 / FUN_180003030, every citation
re-read first-hand in the corpus, not from the finding text):
secondary_count ∈ [2,18]; create+modify+access timestamp field sanity
(day≠0, month∈[1,12], hour≤23, min≤59, 2sec≤29 — all-zero FAILS,
correcting the fix-plan guess); stream non-zero gates (data_length,
first_cluster, name_length — empty files never emitted, the og quirk
inherited as parity); name flags/first-char gates. Gates apply to
live, deleted and oracle re-walked sets; new stat `sets_gated`; the
oracle's declared bound tightened 512→19. Corpus-UNVERIFIED residue
documented, not guessed: the 0xHH00 UTF-16 validity table
(PTR_180045a00 bytes not recoverable). Six fixtures carried TS=0
stamps (the refuted premise) — updated to real stamps. Proof: 5
fail-before EXECUTED tests + 1 boundary guard; suite 743/743
linux-release. Environment note recorded for the next session: the
local ASan lane flags 4 LSan items (the N-048 sink's documented
immortal allocation) — verified identical at the baseline tree; CI
runners disable LSan (ptrace), so main stays green in CI.

**Counts after this landing (pre-merge with the parallel lanes' window):** MAJOR 13 · MINOR 19 · INFO 24 ·
FIXED 150 findings. Next on this lane: M-052 (FAT/exFAT recycle-bin
pairing — the NTFS implementation generalizes) → M-020 (Btrfs chunk
profiles) → M-035 (lost-partition content validation).
**A-008 NEW-FINDING (ACUTE) filed + FIXED (OQ-Z1's ACUTE gate was real):**
the ZFS dsl_dir child zap was read from bonus **+24 — the dd_origin_obj
slot**; kernel (OpenZFS master dsl_dir.h, fetched verbatim) puts
dd_child_dir_zapobj at **+32**. On normal pools (origin=0) **no child
dataset ever enumerated** — every `zfs create` under the pool silently
missing from results; on clones the origin was followed as a child zap
(ghost datasets — the clone-shape failure mode PROVEN in the executed
fail-before, not inferred). The og-side decode this session
(ZfsSearchDLL :20313-20788) is kernel-exact end-to-end: 256B dsl_dir bonus
copy / 320B dsl_dataset copy, head-dataset hop @+8, ds_bp@+128, child-zap
walk skipping `$FREE`/`$ORIGIN` — core==og==kernel after the fix. The
session-6 refinement's "kernel-correct on all three constants" claim
corrected in the OQ file (it repeated the engine comment's mis-transcribed
field list, which dropped dd_parent_obj); the test fixture was
self-confirming (stamped the same wrong offset) — de-self-referenced now.
Proof: fail-before RED 3/3 (`Zfs.EndToEndWalk`, `Zfs.ChildDirZapReadsKernelOffset32`,
`Zfs.OriginObjNotTreatedAsChildZap`), pass-after 16/16 Zfs, **suite
739/739**. `restamp_zpl_array` extended for the shared-ZPL second dataset.
OQ-Z1 RESOLVED (tracked-open 11 → the OQ-Z-E-FLT file carries the
resolution; its Z2..Z5/E1/FLT residues stay open).

**The tracked-open resolution wave (this lane, 6 more OQs):**
- **OQ-S6-3 RESOLVED** — the CRAW reader decode (FUN_140c40d80 :1875575):
  +0x56/+0x94/+0xF4 all pass-through at load; +0xF4 = a per-analyser-type
  marker (set iff type==0xA8, FUN_140c41450), the getters are dead
  accessors (zero callers, no vtable slot) — M-051's landed design
  CONFIRMED on all three fields; the decoded semantics documented at the
  carve_flag carrier; evidence s25-oq-s6-3-craw-reader-decode.md.
- **OQ-S6-2 RESOLVED — INVALID premise**: the capa "RC4 PRGA" finding
  (0x180012190) is a FALSE POSITIVE (chunked adler-32/crc32 checksum pump;
  no RC4 essentials anywhere in the binary); corpus erratum E46; parity
  by shared absence; evidence s25-oq-s6-2-btrfs-rc4-false-positive.md.
- **OQ-S6-1 RESOLVED** — the PartitionRecoveryD generator-set decode:
  RegSearch and CompleteSearch are behaviorally IDENTICAL (byte-level
  vtable dumps); the .dat files = module-dir cross-run caches
  (partinfo.dat = 24B "Part" table snapshot → layout_changed;
  lostpart.dat = 1096B candidate cache + dedup); I-020's upgrade
  condition REFUTED (stays acknowledged INFO); M-042 CONFIRMED at the
  mechanism level; evidence s25-oq-s6-1-generator-sets-and-dat-files.md.
- **OQ-S5-1 RESOLVED — dead-shipped / MATCH-by-omission**: the consumer
  sweep exhausted (the MOM load contract is on-demand BY NAME; zero
  name-requests / PE references / config loads / enumeration loaders);
  CheckVersion.mo's OS-aware recovery never executes in the shipped
  product; evidence s25-oq-s5-1-checkversion-dead-shipped.md.
- **OQ-S4-3 pushed forward precisely, prerequisite LANDED**: the
  alignbench gains the og's 7 MiB volume chunk + cpu_ms + the
  chunk_strategy_ratios verdict block (the OQ's required engineering,
  as a runtime variant); the parse gate's file-list gap closed; the
  remaining step = exactly one Windows execution + transcription.
- Verification-discipline note (near-miss, recorded): an apparent
  [m/[M-eating corruption of the bench fleet was REFUTED at byte level —
  a display-layer artifact (false ANSI SGR stripping); the files use
  lowercase [math]:: (valid PowerShell) and branches: [main]. No
  finding filed; no healthy file touched.

**Session-25 lane tally: 29 resolutions** (22 INFO closures into FIXED/,
A-008 ACUTE NEW-FINDING filed+FIXED with executed fail-before RED 3/3,
OQ-Z1/S6-3/S6-2/S6-1/S5-1 resolved, OQ-S4-3 prerequisite landed). Suites
green at every push (737 → 768 → 792 → 827 through the parallel-lane
union waves; every union tree re-verified full-suite before pushing).

## Session-23 delta, batch 4 (2026-10-26 — DFX29: N-050 — the two-stage recognition contract)

**N-050 FIXED:** the recognition contract is now boot gate -> CONTENT PROOF
-> engine on every family with a corpus-decoded analyser. NEW proofs: exFAT
(the engine's own validate_boot_region — OEM-first so a spoofed OEM fails the
MustBeZero sweep; the structural rc equals the engine's own gate, the
checksum verdict stays informational on both sides, no recovery-negative
strictness) and FAT (root-dir first-sector readability: FAT12/16 fixed root
= reserved+numFats*spf16; FAT32 = reserved+numFats*spf32+(rootCluster-2)*spc
— FUN_180002180's cheap form per the fix plan). NTFS/ext4/UDF content proofs
were already landed (M-043/S-006/M-044). Every proof emits
`recognition: <family> content-verified` on the N-048 logging seam; the
contract is documented as ground-truth-contracts.md §0 (with the honest
HFS+/Btrfs/ZFS/ReFS recognition-gate gap and the exFAT analyser's unpinned
body recorded). Fail-before EXECUTED (2 of 3 new tests RED on the reverted
probe — the third is the positive pin); two pre-existing expectations
CORRECTED toward og parity (spoofed-EXFAT-OEM -> Fat, the og's
multi-analyser outcome; the painted-FAT-boot UDF image -> Unknown — the old
Fat was the spoof-accepting answer). Suite **757/757**.

**Post-fix counts (at this batch's push, union with the window's parallel
landings M-020/M-014/M-049/M-058/N-058/N-032/N-064 + OQ resolutions):**
SEVERE **0** · MAJOR 11 · MINOR **8** · INFO 0 open · tracked-open 10 ·
FIXED/ **188** files (187 findings + RESOLVED-INDEX). N-064 (on this
lane's original claim) landed by the parallel window. Next on this lane:
N-033 (carve capability ABI), then the unclaimed MINOR remainder
(N-065/N-066/N-059/N-072), then feasible MAJORs.

## Session-23 delta, batch 5 (2026-10-26 — DFX29: N-033 closed — the CLI half + the refresh-divergence register)

**N-033 FIXED (the finding's last open scope):** `rrecov
--list-carve-formats` renders the cr_ui carve-format table (the ABI half
landed in session 6) — layer/ext/size bounds/base confidence/grammar
magic, checked before command dispatch (works bare), usage updated.
Parity tooling and CLI hosts self-describe the carve set. ABI-CONTRACT
§7a gains the per-scan registry-refresh divergence note (the og's FS
engines re-clone the cross-DLL RAWSearchDLL registry per scan,
FATSearchDLL :29290-29361; the core's tables are compile-time immutable
append-only — the enumeration is lifetime-valid, no refresh seam; the
future runtime-config path is named as a versioned-ABI question). Gate
hardening en route: cr_ui.h's `__declspec` expansion under the parse
gate's -D_WIN32 is neutralized in the gate's CXXFLAGS (include-order-
proof; never in shipping code) — win_syntax_check 8/8 GREEN. Suite
**800/800**.

**Post-fix counts:** SEVERE **0** · MAJOR 11 · MINOR **7** · INFO 0 open ·
tracked-open 10 · FIXED/ **189** files (188 findings + RESOLVED-INDEX).
Next on this lane: N-065 (writer destination policy) -> N-066 -> N-059
-> N-072, then feasible MAJORs (M-052/M-035/M-048/M-018).

<!-- DFX16 note: session 16 ran as a parallel lane in the same window as
     sessions 17-22; its delta is recorded here for chronology. -->

## Session-23 delta (2026-10-26 — DFX23: the MINOR-first close-out lane, N-025 first landing)

Session bootstrap at `716d121` (DFX16-M013 rebase reconciliation; N-062 the
last fix landing); corpus `03ad46c` unmoved. Baseline before any change:
**737/737** linux-release green. Mandate: minor findings first, 20+ fixes,
fixed findings moved to findings/FIXED/, all docs/counts current.

**N-025 FIXED (the LDM geometry group, both sub-items):** N-012a — the og's
1 MiB config-region gate (`config_size < 0x801` else "ConfigSize larger than
1m ?." -> rc 0xfffffff8, LdmManager_decompiled.c:59748-59759, re-verified
first-hand this session) is now first-class in parse_ldm_database; the old
8192-sector read clamp (an unregistered parse-anyway superset) removed as
unreachable. N-012b — a zero interleave defaults to the og's 64 KiB (0x10000
bytes = 0x80 sectors at the 512-LBA grid; ctor default :47665, use-site
re-default :68631-68637, sector fallback 0x80 :68638-68641 — all sites
re-verified first-hand) on BOTH the striped and RAID-5 layout paths; the old
code lost the volume entirely (zero-capacity layout). Tests:
Ldm.OversizeConfigRejected + Ldm.ZeroInterleaveDefaultsTo64K (both
fail-before RED on the stash-isolated pre-fix tree) + the
Ldm.ConfigSizeExactlyOneMebibyteAccepted boundary pin (2048 keeps parsing).
Suite **740/740**.

**N-027 FIXED (this lane's third — the carve/writer group, one real fix +
three verified FIXED-ALREADY):** N-014b — the carved-name split is closed:
the raw-sign emit layer now rides the same namer chain as the grammar layer
(caller namer, then the registered File%06u default), and the orchestrator's
"Carved%06u" lambda is removed — one File%06u family per scan (fail-before
RED: the grammar-layer JPEG carved as Carved000000.jpg while the raw-sign PSD
carried File%06u.psd; pass-after all File%06u;
Orchestrator.CarveSingleNamingFamily). N-014a FIXED-ALREADY (N-016's teardown
double-emit removal), N-014c FIXED-ALREADY (M-024's
CarveFlags.disabled_formats), N-014d FIXED-ALREADY (N-008's salvage-polarity
inversion + N-068's LFH gates; this session's re-read of :18479-18498
confirmed the direction verbatim and corrected the audit's "~4 GiB" cap to
the real 0x500001 loop bound). Suite **749/749**.
## Session-24 delta (2026-10-26 — DFX24: the disjoint MAJOR lane; DFX23 owns the MINOR queue)

Bootstrap: the parallel DFX23 lane claimed the full MINOR tier at 354c555
in the same window — this lane re-stamped 23→24 and claimed the DISJOINT
open MAJOR tier (14 findings). Baseline executed first-hand BEFORE the
claim: linux-release suite **737/737** green at 716d121 (= 354c555
content); corpus 03ad46c unmoved (re-verified at first read).

**M-049 FIXED (MAJOR — the error model, both halves now closed):** the
session-11 half (header value-space truth + the err_text coverage test)
was verified landed; this session closed the finding's registered step 2
— the engine-side error-context subset of the og's error-object
architecture (per-module registrars, Error.md census; corpus re-verified
first-hand: 05-scan-engine-architecture.md:62 + the Error.md RTTI
census). The error-range event family now carries the
module/operation/retry-state triple: `err_module`/`err_operation`/
`err_attempts` on `cr_ui_event` (v2-era append + alignment pad — the
8-byte-multiple POD pin stands, no ABI bump per §9), new CR_ERRMOD_*
(NONE/ORCHESTRATOR/9 structured engines/CARVE/LOST/LDM) + CR_ERROP_*
(READ) ABI constants pinned to the core enum by 15 static asserts. The
attribution is per-range and recorded at failure time: `cr::errmod`
(cr_errmod.h) — a per-thread stamp the orchestrator sets around every
sub-pass (probe/table = orchestrator, engine pass = family, remnant =
NTFS, discovery = LOST, candidates = per-candidate family stamped inside
scan_candidate, carve = CARVE, LDM database = LDM) + a per-device owner
fallback on the retry device covering engine-internal worker pools;
coalesced ranges keep the NEWEST attribution; sessions persist the
geometric trio unchanged (44-byte wire pin — reloaded ranges honestly
kNone). The N-048 read-failure log line carries the module id. D-68
updated (the subset is no longer the open item). Fail-before RED
EXECUTED at the single retry-device seam (attribution disabled: 5/6 RED
— the three event-attribution tests + the two retry-unit tests);
restored → suite **745/745** linux-release (8 new tests); ui_abi_check.c
extended (plain-C /W4 /WX readability of the triple).

**Counts after this landing (direct file inventory):** SEVERE 0 · MAJOR
13 · MINOR 19 (DFX23's queue) · INFO 24 · tracked-open 11 · **FIXED/ 150
files** (149 findings + RESOLVED-INDEX). Next on this lane: M-035
(engine-side FileFilter validation) → M-052 → M-014 → M-018.

**N-031 FIXED (the carve dispatch deltas):** the og's all-zero-sector fast-skip
landed at the raw-sign layer in an exact core form — a 512-byte stride whose
sign window (the first 47 bytes, probe_header's whole offset span) is all zero
skips the 47-lookup sign-table walk and is counted
(ScanCtx→CarveStats.zero_strides_skipped→ScanStats.carve_zero_strides); the
grammar layer's first-byte dispatch keeps running on zeros (the finding's own
profile rule). The 512-vs-device-sector stride difference is documented at
cr_rawsigns.h (the core's probe positions are a strict superset on 4Kn — bench
comparability registered); the only_common whole-map restrict is expressible on
M-024's landed disabled_formats surface (registered as host policy, no engine
constant); the collision-order equivalence pins (TiffVsRawTiebreak et al.) stay
green. Tests: CarveEngine.ZeroSectorSkipStats (zero-heavy image: >200 strides
skipped, the JPEG behind the zeros still carves; non-zero control skips 0) +
the pins re-run. Suite **766/766** (fail-before by construction — the counter
did not exist).

## Session-16 delta (2026-10-26 — DFX16: S-029 closed — the SEVERE tier is empty)

Baseline `53629a3` verified green first-hand (584/584 linux-gcc ASan);
corpus `03ad46c` unmoved. The session's first act repaired a pushed-red
main inherited from the window's parallel landings: the M-054 producer pin
`FreeMapProducerMatchesSpecBitMapping` asserted the pre-hardening
`cluster_count == 100` while the landed producer (0ba21d8 defect 3 + the
FreeMap contract + the FAT family's 5002 pin) covers 0..101 — the stale
pin corrected to 102u with the audit-honesty note in the M-054 finding
(the convergent DFX17-side correction eeeccbd is in history).

**S-029 FIXED (SEVERE — the lost-partition APPLY path, the recovery
endgame; the last open SEVERE):** a dedicated apply module
(`cr/apply/cr_apply.h/.cpp`) with the rawfixer transaction pattern
extended to the table writer — plan (read-only; refusals: overlap,
table-full, GPT single-entry v1 scope, capacity/32-bit-LBA-range) →
execute (core-carried consent callback default-deny, S-010 quiesce, A6
table-change race guard, CRUNDO01 snapshot, verified write-back, the
corpus device-control sequence per legacy/modern path — 0x7C100 →
sector-0 → 0x70140 → 0x7C010 vs the 0x7C058 0x18-byte record, then the
3x Sleep(1000) refresh tail) → undo (byte-exact + refresh replay). cr_ui
consent ABI (apply_lost_partition/undo_last/save_undo, ABI-since 1.3);
the Windows RwDevice carries the real IOCTL chain (kernel32-only — the
Win7-inclusive import gate untouched). One corpus correction in the
Resolution: GPT_GenerateSignature = the 32-bit disk signature (MBR +440),
not the GPT GUID (4-byte stack slot at all four call sites — PR:32805/
RM:46338/CM:118357/UL:37590). Safety deltas registered D-72a..d. Proof:
13 `LostApply.*` tests (the fix plan's five named proofs + 7 safety
pins), 2 `CrUi.LostApply*` C-ABI tests, the extended plain-C
`ui_abi_check.c` lifecycle; suite **610/610** at the fix commit.

**Post-fix counts (this session's commit, parallel lanes active in the
same window):** SEVERE **0** · MAJOR 28 · MINOR 41 · INFO 22 (all
acknowledged) · **FIXED/ 101 findings** (RESOLVED-INDEX current) ·
tracked-open 16. Parallel-lane landings absorbed this window: M-005,
M-007, M-008, M-045, M-054, N-030 (+ the M-054 pin correction).
Next on this lane: M-043 (NTFS content-proof + backup boot) → M-013/
M-014 (exFAT) → M-018 → M-029 → M-032 → M-035 → M-049 → M-050 → M-052,
then unclaimed MINORs.

**M-013 FIXED (MAJOR — the exFAT dual-form deleted-set checksum + the
reconstruction oracle):** FUN_18001eba0's dual ROL15 checksums (raw +
InUse-restored with the 0x85/0xC0/0xC1 substitution ladder) decoded
decompile-exact and landed as the UNCONDITIONAL deleted-set gate (failed
sets parked, never emitted — the pre-fix core's FP surface closed);
FUN_1800203a0's parked-set oracle (suffix substitution from
secondary_count down to 1, dual re-test) reconstructs torn sets from
parked orphan stream/name runs, re-walking accepted reconstructions
through the standard emission path. LIVE sets keep the flag-gated
downgrade (D-47 scope-corrected; N-003's premise recorded REFUTED).
Fixture truth: deleted sets stamp SetChecksum (the unstamped model
encoded the refuted premise). 2 fail-before-proven tests + the
inverse-asymmetry pin; suite **730/730**. M-043/M-032/M-050 landed by
the parallel lanes in the same window — next on this lane: M-014 →
M-018 → M-029 → M-035 → M-049 → M-052.

## Session-22 delta (2026-10-26 — DFX22: win-layer build-integrity M-055 + the MINOR-first queue)

Session lane re-stamped 19→22 (IDs 19/20/21 squatted by parallel legs in
the same window). Bootstrap: pulled the parallel wave (`e21de3e`→`93d7fa6`:
S-029 + S-030 landings close the SEVERE tier, M-005/M-008/M-016/M-032/
M-043/N-041/N-016a, D-70/D-71); baseline suite **629/629** Linux Release
green with this session's additions; corpus `03ad46c` unmoved.

**M-055 NEW-FINDING (MAJOR) filed + FIXED (first landing):** the
Windows-only platform layer has been UNCOMPILABLE since 2026-10-25 21:07 —
four independent structural breaks from three concurrent parity-fix
landings that no prior session executed: (1) `915bce3` (N-015) nested
`WinDestIo::free_bytes` inside `write()`'s body (missing closing brace);
(2) `3b5397a` (N-013) defined `volume_extents`/`destination_volume_letter`
AFTER the namespace closers (unqualified `VolExtent` error + the
header-declared functions left undefined — the claimed destination-arming
could never link); (3)+(4) `d8f6808` (N-041) left a stray `} // namespace
rda` closer that closes `namespace win` mid-file AND deleted
`RdaTargetDevice::heal_mu_` while keeping `read_at`'s heal lock on it.
The windows Actions job's build-step failures over this window were
visible and unaddressed. Fixed all four; landed the permanent gate:
`tools/win_syntax_check.sh` + `tests/win-stub/` (four declaration-only
SDK-faithful stub headers) wired as ctest `win_syntax_check` on the Linux
gcc lane — the fail-before/pass-after run (exit 1 pre-fix with the exact
diagnostics, exit 0 post-fix, all seven win-layer files parsing incl. the
S-030 `cr_image_win.cpp`) is the finding's proof. Suite **629/629**.
The N-056 queue proceeds on top of this (same file, its fix plan was
unexecutable while the layer didn't parse).

## Session-18 delta (absorbed by the DFX22 merge — parallel lane, verbatim) (2026-10-26 — DFX18: the minors-first wave, lane-disjoint from DFX17)

Session bootstrap at `eeeccbd` lineage (prompt push `e21de3e`, lane claim
`e61c2ff`); corpus `03ad46c` unmoved. Baseline before any change:
linux-release suite **592/592** green at the claim tree (the 584-at-clone
baseline absorbed the parallel landings M-007/M-054/M-045/M-046/M-037/
M-031/N-030 after the rebase). Mandate: minor findings first, 20+ fixes,
fixed findings moved to findings/FIXED/, all docs/counts current. Lane
claim posted in the worklog BEFORE any finding work: the DISJOINT
remainder of the MINOR tier (DFX17 owns N-067/N-068/N-063/N-065/N-056/
N-029 + group files).

**N-045 FIXED (registered deliberate divergence — the finding's own fix
plan):** the 8 MiB / 256 KiB reactive-LRU scan cache vs the og's 4 MiB
prefetch read-ahead `CStorageDeviceWithCache` (DRWUI_decompiled.c
:919080-:919092 `0x400000/bytes_per_sector` + 0x2000-block fallback;
ctor :962250-:962597 — re-verified first-hand this session, both the
budget cite and the session-6 prefetch-engine addendum). Registered as
PARITY-DELTAS **D-72** (budget + algorithm, benchmark-gated change
policy); divergence block in `include/cr/storage/cr_cached.h`;
construction-site cite in cr_scan.cpp; docs/benchmarking.md §4 intent
note. Doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence`
via the new `CR_SOURCE_DIR` test define (tests/CMakeLists.txt): reads
the source constant, the D-72 row, and the header block at runtime —
fail-before executed RED on the stash-isolated pre-fix tree, pass-after
green; suite 592/592. Finding → FIXED/; RESOLVED-INDEX row; matrix
storage P47/P48 cell → MATCH(registered); storage.md verdict updated.

**N-046 FIXED:** `PartitionSlice::refresh_geometry()` — the og's
RefreshSectorSize-from-parent surface (FUN_140643870,
DRWUI_decompiled.c:1044889-1044895, re-verified first-hand: parent
vcall +0x28, store at +0x38, returns 1 unconditionally) with
revalidation hardening over the og's blind store (misaligned /
shrunken / zero-sector media changes refused, snapshot untouched);
orchestrator auto-refresh excluded and documented in the header (the
corpus exposes no scan-path caller — an invented trigger would be
hallucinated parity). Slice.GeometryRefreshAfterMediaChange +
Slice.GeometryRefreshRefusesInvalidatingChange; fail-before by
construction (the API did not exist). Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-046 push):**
SEVERE 0 · MAJOR 24 · MINOR 35 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 112 files (111 findings + RESOLVED-INDEX). (The hot parallel
window absorbed S-029/S-030/M-008/M-016/M-032/M-043/M-050/N-041/N-057/
N-058/N-067/N-016a + the S-033 INVALID filing; counts are the live
inventory at this commit.)

**N-040 FIXED (D4 closed; D1/D2/D3/D5 verified already-landed):** the
five ABI/doc truthfulness defects re-verified on the current tree —
four had landed via parallel lanes (§8 SQLite REJECTED-BY-REFERENCE,
S-009's disk-map free state, §6's event-thread line, M-029's FILETIME
unit fix with its Query.ModifiedUnitIsFiletimeEverywhere pin). This
session closed D4: the seven fake "ABI-since 1.1/1.2" header
annotations rewritten to truthful era labels; ABI-CONTRACT §9 made
coherent (v2 = M-036's deliberate feature-batch bump — appends still
require no bump; no v1.1/1.2 ever shipped). New pins:
CrUi.AbiVersionAnnotationsAreTruthful (fail-before RED executed on the
stash-isolated pre-fix tree), CrUi.EventThreadContractPinned,
CrUi.SessionSaveWritesCrsfOnly. Suite 656/656. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-040 push):**
SEVERE 0 · MAJOR 24 · MINOR 34 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 113 files (112 findings + RESOLVED-INDEX).

# SUMMARY.md — executive summary (session 17's MINOR-first wave on top; earlier sessions below)

## Session-17 delta (2026-10-26 — DFX17: MINOR-first close-out wave)

Session start: the union tree at `b8b0318`+ (baseline verified **562/562** +
21/21 at bootstrap; the hot remote's parallel landings — M-054/M-045/M-002/
M-037/N-030/M-007/M-016/M-043/S-030/M-050+N-058/N-057/M-016 — absorbed by
rebase as they arrived, each re-verified). Corpus `03ad46c` unmoved.
Mandate: **MINOR findings first**, 20+ fixes.

**N-067 — CONVERGED (parallel DFX16 lane landed `6795918` while this lane's
implementation sat behind a rebase conflict):** this leg independently
re-verified the landed fix first-hand against the corpus decompile
(exFATSearchDLL_decompiled.c :24440-24452 case -0x7e — the og's 0x82-dentry
shape gates: reserved1@+1..+3 zero, checksum field @+4..+7 FREE (4 bytes),
reserved2@+8..+0xF zero, the bVar21-required u32@+0x10..+0x13 zero via
LAB_18001db06, first_cluster@+0x14 in [2,0xFFFFFFF5], u64 length@+0x18 in
cluster arithmetic). The landed implementation (og shape gates +
chain-read checksum32 stream validation, soft stats, grade-not-reject)
matches the decompile on every axis. This leg's competing implementation
was DROPPED with an honest record: its wire layout (u16 checksum@+2,
u32 length@+0x10) came from a faulty spec recollection and is REFUTED by
the og's own field map (the free zone is exactly +4..+7; +0x10 is
zero-checked, never a length). Lesson registered: the decompile is the
authority for the og's dentry shapes — never trust unverified spec memory
against it.

**Baseline repair 1 (M-054 test expectation):** the M-054 lane's
`FreeMapProducerMatchesSpecBitMapping` shipped `cluster_count 100u`
against the S-009-hardened coverage semantic (2 virtual + 100 data = 102)
and left main red; repaired to 102u by this lane and, in the same window,
by the parallel lanes (eeeccbd + the 8c02a57 dedupe) — first-landing-wins,
all attempts in history.

**Baseline repair 2 (LeakSanitizer red — converged with the parallel
session-22 lane's N-077 PropsRef landing 98b7d13; first-landing-wins, this
leg's equivalent guard dropped) + N-078 NEW-FINDING (filed + fixed):** main leaked three 264-byte `FileProperties` objects from
`deserialize_tree`'s corrupt-record early exits (the M-050/N-058 landing's
hardening paths — five break sites exit between the per-record allocation
and the node attach without releasing the creation ref). Repaired with a
scope guard covering every early exit; the attach path disarms it. The
new five-path regression test (`Session.CorruptRecordEarlyExitsReleaseProps`)
then caught a SECOND defect on one of those paths: the truncated-carve_note
exit broke without setting `c.fail` — a silent partial load violating
N-058's failed-stream rule — filed as **N-078** (renumbered when the parallel lane took N-077 for the
leak half) and fixed (one-line root
cause: the same fail propagation every sibling site already used).
Fail-before EXECUTED on both (LSan red on main; case (4) returned OK=0).
Post-fix: **614/614 + 21/21 UI + ABI check green, LeakSanitizer clean.**
Finding → FIXED/ (straight in, in-session file+fix); RESOLVED-INDEX row.
Their N-077 landing alone does NOT cover this: case (4) of the new test
fails against leak-only trees — the c.fail gap is real and pinned.

**N-063 FIXED (MINOR):** the og's per-analyser carved-file naming. The
MP4/MOV template decoded FIRST-HAND ("Lost Name File (<time>)_EaseUS" —
literals :33312/:33408, the "%04d:%02d:%02d %02d:%02d:%02d" _localtime64
format :33465/:33531, mvhd v0 BE u32@+0x10 / v1 FIRST-FOUR-BYTES of the
u64@+0x14): `CarveResult::embedded_name` carrier with emit precedence
(embedded_name > namer > default), TWO decode corrections recorded (the
field is the MODIFICATION time; there is NO 1904-epoch conversion in the
og — both quirks replicated + pinned by test), the false "corpus policy"
comment repaired, File%06u registered as the deliberate default
(PARITY-DELTAS D-87 with the UTC-formatting divergence). Fail-before 3/3
RED; 679/679 + 24/24. The CZeroEnd-family membership beyond MP4/MOV is
the honestly recorded remainder (decode each of the 22 SetName callers).

**N-056 FIXED (MINOR):** the WinDestIo failed-rename temp leak closed at
both sites the finding's plan named — commit() deletes the staged temp in
the rename-failure path, and close()'s delete gate is the RENAME STATE
(Open::temp_renamed) instead of handle liveness (the old gate provably
never fired after a failed rename: the handle was nulled pre-rename).
WinDestIo.FailedRenameCleansTemp (structural fail-before pinned in-test)
+ WinDestIo.UncShapedPathsFailCleanly (the UNC-family contracts). The
Windows/MSVC lane proves execution (Linux cannot exercise WinDestIo —
the finding's own deferral); Linux suite 684/684 unaffected.

**Counts after this wave (direct file inventory):** SEVERE 0 · MAJOR ~31 ·
MINOR 23 files · INFO 22 (0 actionable) · tracked-open 16 · FIXED 101
finding files + RESOLVED-INDEX. Queue head: N-046 (geometry refresh),
then N-043 (progress liveness watchdog) and the remaining MINOR tier
(N-068/N-065 left to the DFX16 lane's published queue).

# SUMMARY.md — executive summary (session 10's close-out on top; earlier sessions below)

---
**N-074 FIXED (doc-drift group #2, all five):** CAPABILITY-MATRIX LZNT1
row split (DONE with code+test cites; XPRESS/LZX the SHARED limitation
— the og is LZNT1-only too); GAP-S6-11 CLOSED (the disk-map surface
landed; S-009's free overlay noted; host-side rendering the design);
D-43 re-worded to the 256 KiB block reality (D-51/D-72 cross-refs); D-19
rewritten per the DECODED og collision policy (corpus
:357116-357131 re-verified first-hand — `%s_%d`/cap-10000; our
`name (N)`/cap-64 scheme now the REGISTERED divergence, the
atomic/hash/resume layers still registered supersets); cr_health.h's
command list corrected (0xD1 ships since S-024 — kept; 0xD8 never
issued — dropped). New pin: Health.SmartCommandListMatchesImplementation
(fail-before RED executed on the stash-isolated pre-fix tree). Suite
662/662. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-074 push):**
SEVERE 0 · MAJOR 24 · MINOR 33 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 114 files (113 findings + RESOLVED-INDEX).

**N-043 FIXED (the honest heartbeat — fix-plan option (b)):** new event
type CR_EVT_HEARTBEAT (appended, no version bump) fired from the shim's
watchdog thread every 5000 ms while run() is in flight and no
CR_EVT_SCAN_PROGRESS arrived in the window — the og's 5 s liveness
watchdog (FUN_140577100, DRWUI_decompiled.c:910565-910585, re-verified
first-hand) in the HONEST variant: the payload carries the standing
telemetry truth, never a synthetic permille advance (the og's +0xe8
coarse bump is the registered divergence, ABI-CONTRACT §6).
CrUi.HeartbeatDuringStall (pause-parked scan = deterministic stall;
fail-before RED executed — the run also caught and fixed a test-design
flaw where ASSERT-before-resume leaked the parked scan). TSan 686/686
CLEAN (new thread); both event consumers taught the type; the
EventThreadContractPinned pin caught the contract-text change and moved
with it in the same commit. Suite 686/686. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-043 push):**
SEVERE 0 · MAJOR 24 · MINOR 32 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 115 files (114 findings + RESOLVED-INDEX).

**N-060 FIXED (save progress + cancel + atomic saves):** the og's save
window (rsf-format flow section: 40 + 50·processed/(2·remaining) with
per-child IUIProgressInfo::IsCancelled teardown, re-verified first-hand)
mapped honestly: per-node `session::SerializeProgress` hook through
serialize_tree (ERR_CANCELLED on false) + save_session pass-through +
the shim's coalesced CR_EVT_SAVE_PROGRESS (9u; the save's own 0..1000
stage_permille, overall stays the standing 1000 — the og's 40→65%
window would regress our monotonic permille; not adopted, registered)
+ the SAVE-SCOPED cr_ui_scan_save_cancel export (the mid-fix
correction: the og's IsCancelled is the save object's state, NOT the
scan flag — save-on-scan-cancel stays intact) + atomic temp-then-rename
saves (shim + CLI — the N-014 discipline; a torn save never destroys
the previous good session). Session.SaveEmitsProgress /
SaveCancelsCleanly + CrUi.SaveProgressEventsAndAtomicCancel; fail-before
by construction. Suite 716/716. Finding → FIXED/.

**Counts after this landing (direct file inventory at the N-060 push):**
SEVERE 0 · MAJOR 24 · MINOR 31 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 116 files (115 findings + RESOLVED-INDEX).

**N-048 FIXED (the engine logging seam — the fix plan's recommended
minimal form):** `cr::log` (levels Error/Warn/Info; a host-registered
sink, default NULL = silent with a zero-overhead fast path; bounded
formatting; serialized synchronous deliveries) + the og's event set as
emission sites: scan START/END brackets (the qa-logs semantics —
extent/fs_hint/stages; rc/all counters/error-ranges/pause-compensated
duration), stage boundaries, committed checkpoints, lost-candidate
resolutions (the CalcPartition.log truth: lba/sectors/fs/score), and
the retry pipe's read-failure exhaust (Error — PartitionRecoveryD's
read-error trail). The CLI's --log file is the host sink (the
EPMLOG.log role). D-75 registers the single-process sink-not-transport
divergence (no shared mapping/ring/async queue — the core has no
second process). Orchestrator.LogSilentByDefault +
LogSinkReceivesScanBrackets — fail-before RED executed (the emission
sites reverted, the seam present: zero lines). Suite 732/732.

**Counts after this landing (direct file inventory at the N-048 push):**
SEVERE 0 · MAJOR 24 · MINOR 30 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 117 files (116 findings + RESOLVED-INDEX).

**N-061 FIXED (both halves):** the persistent run-log is the N-048
seam + the CLI's --log file (the EPMLOG.log role) with the qa-logs
semantic set at the core (START/END brackets with extent/stages and
rc/counters/error-ranges/pause-compensated duration, stage
boundaries, committed checkpoints, candidate resolutions with the
CalcPartition.log scored truth); default-silent — the conservative
read of the channel-gating open question. The session auto-naming
helper: session::auto_session_name_at(label, tm) — byte-exact vs the
corpus sample `auto_Local Disk (I_)10_14_23_13_21` (the forbidden-set
sanitization incl. ':' → '_', LOCAL-time M_D_H_m_s, UTF-8 out);
Session.AutoNamingConvention (fail-before by construction). The CLI's
--out stays explicit (frozen contract — auto-save-on-default is the
og's UI policy, not the library's). Suite 733/733.

**Counts after this landing (direct file inventory at the N-061 push):**
SEVERE 0 · MAJOR 24 · MINOR 29 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 118 files (117 findings + RESOLVED-INDEX).

**N-062 FIXED (the memory monitor on the same seam):** the og's
CMemMonitorManager pressure tracking mapped onto the N-048 cr::log
seam: the pure `check_memory_pressure(threshold_mb, working_set_mb)`
(Warn line with both numbers; OFF at 0 — the conservative library
default; the og's always-on monitor is its UI process's choice) + the
platform sampler in the shim (statm / GetProcessMemoryInfo; a failed
sample never warns) riding the heartbeat watchdog's 5 s cadence (the
watchdog now starts on events OR the armed monitor) +
`cr_ui_scan_config.memory_warn_mb` (appended, zero-init safe). NEVER
coupled to scheduling (the finding's own rule). Orchestrator.
MemoryMonitorEmitsUnderPressure + the HeartbeatDuringStall extension
(fail-before RED executed on the stash-isolated tree). Suite 734/734.

**Counts after this landing (direct file inventory at the N-062 push):**
SEVERE 0 · MAJOR 24 · MINOR 28 · INFO 22 (0 actionable) · tracked-open
16 · FIXED 119 files (118 findings + RESOLVED-INDEX).

## Session-10 delta — the SEVERE tier close-out (2026-10-26)

This lane (DFX10) executed the published order S-009 -> S-029 -> S-030
against a hot remote (three independent S-009 implementations converged;
S-030 landed from the DFX8 lane mid-session):

**S-009 — converged + hardened:** this lane's independent fix rebase-
absorbed into the landed storage::FreeMap implementation (c55bd45);
first-hand verification of THEIR code found and closed four defects (FAT
+ exFAT virtual-cluster false-free over the root-dir/boot tail; exFAT
2-cluster under-coverage; the torn-bitmap read painting zeros free) plus
the found-content rule convergence (deleted/carved content cells stay
Unknown — the finding's plan verbatim). 4 pin tests; suite 577/577.

**S-030 — landed by the parallel lane (b30354d):** flat/VMDK/VHD image
devices + --image CLI (re-verified green in this lane's baselines).

**S-029 — CLOSED (this lane):** the lost-partition apply path, the
recovery endgame. New consent-gated cr/apply module on the rawfixer
seams: plan_apply (read-only, containment-refusing) + execute_apply
(A1 consent callback — denied OR absent = ERR_APPLY_CONSENT_DENIED with
zero writes; A2 stale-table revalidation; quiesce first; A4 transaction
with read-back verify + rollback) + the shared CRUNDO01 undo. CLI
`rrecov apply --disk N --session F --candidate N [--apply]`; win layer
`refresh_disk_properties` (the og's UPDATE_PROPERTIES + settle). 10
Apply.* tests; suite **590/590** Linux ASan. PARITY-DELTAS D-68/D-69.

**The SEVERE tier is CLOSED** (0 open). Live inventory at this push:
MAJOR 33 · MINOR 44 · INFO 22 · FIXED 95 · tracked-open 16. Active lane
claims respected: DFX15 (MAJOR head), DFX9-T7 (M-052/M-035/M-018/
M-013/M-014), DFX14 (M-005).

## Session-21 delta (2026-10-26 — DFX21: the tail-half MAJOR lane opens with M-044)

Baseline the union tree at `b7dd874`-lineage (595/595 linux-release verified
first-hand; corpus `03ad46c` unmoved). Lane: the tail half of the unclaimed
MAJOR tier (queue split against the DFX19 head-half and the DFX20/ex-DFX18
M-033 lane — three same-window lane-ID collisions resolved per the
first-landing-wins protocol, this leg stamped DFX21).

**M-044 FIXED (UDF recognition):** the og's CFatVolumeAnalyser probe
(FUN_180002580 :1413-1435, re-verified first-hand) reproduced exactly in
`probe_family` — boot-bytes fast-reject (jump code short-circuits the probe),
AVDP sector at +0x100 device sectors, Main-VDS sector at u32@(AVDP+0x14),
"UDF"@[0x19..0x1b] -> `FsFamily::Udf` (new value 11, append-only). No
structured engine exists on either side (verified against the factory-map
artifact: the og's 9-engine set has no UDF SearchDLL) — carve-only
disposition, proven end-to-end (auto-probe -> carve recovers a planted JPEG,
quick/deep yield nothing, VolumeInfo.fs_kind == 11 round-trips). cr_ui ABI
fs_hint 11. 4 tests, all fail-before; suite 599/599.

Session counts at this landing: MAJOR open 30 -> 29 (M-044 moved to FIXED/),
FIXED set grows to 99 files in findings/FIXED/.

**M-028 FIXED (preview caps bypass):** the audit's executed bypass
(65,568 device bytes pulled against a 4,096-byte cap with truncated=false
on the docx/zip path) reproduced first-hand as the fail-before, then closed
architecturally: `PreviewReader::read_cost` prices every fetch before the
device is touched (compressed windows cost the whole psize block),
`Window::fetch_any` gates on cost, all four docx reads route through the
new `Window::checked_read` (honest archive/zip degrade + truncated=true),
the over-budget head path propagates truncated, and a post-extraction
device-delta belt guards future parsers. Header provenance corrected (og
preview cap = 1 MiB at the UI socket layer per I-015; IInWindowStream =
7-Zip's LZ input interface; the core's 4 MiB kept as documented superset)
+ the compressed-extent whole-block cost disclosed. 5 tests fail-before;
fuzz tight-budget trap added; suite 662/662. MAJOR open 28.

**M-021 FIXED (carve post-processor hook):** the og's EUVideoRepair binding
(controller+0x158, optional LoadLibrary; the per-candidate deferred poll at
FUN_18000d670 :10130-10136 — both re-verified first-hand) now has its core
equivalent: versioned `CarvePostProcessor` + `CarveFlags::post_processor`
(null = the unbound degrade); fires per validated candidate pre-insertion on
both emit layers; a FEED never a gate (the og's poll never drops a
candidate). The repair engine itself stays scope-excluded
(PARITY-CORE-REC:164) — the hook point is the parity surface. 2 tests
(hook-invoked fail-before-proven by wiring revert; degrade-when-absent);
suite 685/685. MAJOR open 27.

## Session-19 delta (2026-10-26 — DFX19: MAJOR-first lane, first landing M-016)

Bootstrap `ff734d7` (union of the parallel lanes' claims: DFX18 on the MINOR
tier, this leg on the unclaimed MAJOR batch); baseline suite **594/594**
Linux RelWithDebInfo + ASan. Corpus `03ad46c` unmoved.

**M-017 FIXED (fourth landing):** lost-partition scoring — the core's
absolute structural points are replaced by the og's content model:
every surviving candidate is materialized and walked by its engine
(progress-budgeted), the count is weighted per FS (f+50·d NTFS /
f+10·d HFS+ / (f+d)·50 FAT) plus the observed bonus (NTFS 60/30, FAT
20) plus the +500 table-match — the old model's overlap penalties were
the polarity INVERSION of exactly this bonus. Score = ln(count)/
ln(max) in permille with the og's edge cases (count<=1 identity
quirk); structural points tie-break. AdjustPartitionInfoOrder ported:
containment-in-known drop (table-matched candidates exempt), stable
sort, and the :842776-93 size-match promotion re-decoded first-hand
(the iVar16 counter — an adjacent matcher takes the break path the
naive reading misses). 4 fail-before tests incl. the CalcPartition
log-row parity test. Suite 691/691. findings/FIXED/M-017; matrix
lost-partition A30 MATCH.

**M-038 FIXED (third landing):** ZFS SA registry-driven decode — the
core's SA decode was layout-1-hardcoded (`layout != 1 -> nullptr`,
dropping every object with a registered non-1 layout). Now: the og's
three-object chain (master "SA_ATTRS" -> {REGISTRY, LAYOUTS},
:22934-23140), by-name attribute resolution (ZPL_PARENT :24324 family),
offsets by cumulative registered lengths over each layout's slot order,
and the og's zero-attrs surface for unregistered layouts (:17636 — the
object surfaces, attrs zeroed — where the core dropped the file
entirely). Layout 1 keeps the implicit-default fallback for
registry-less pools. Fixtures now carry the real registry chain (a 2K
fzap registry — 14 entries don't fit 512B — plus a layouts fzap with
the 14-slot layout 1 and a differently-ordered 4-slot layout 2, decoded
by the new registryfile object). ZapEntry gained raw_value (fzap arrays
exceed 8 bytes). 2 fail-before tests. Suite 687/687. findings/FIXED/
M-038; matrix zfs B11 MATCH.

**M-019 FIXED (second landing):** ZFS multi-vdev + integrity — read_raw
drops the `vdev != 0` refusal (the og's registry first-entry fallback,
FUN_1800168b0 :19544-56 — foreign reads now served and counted);
read_block gains the og's exact fletcher4 gate (`(zc==7)||(zc==0)`,
interleaved 4-accumulator port) over the RAW bytes with the 32-byte
stored compare and next-DVA retry on mismatch and read failure; the L0
extent walk plans foreign-vdev blocks at the fallback position instead
of skipping them. Fixtures upgraded to checksum-carrying pools (type-7
fletcher4 stamped in dependency order; the fixture uses an independent
textbook encoding so engine and fixture cross-check). Fail-before
proven both claims: the torn-DVA0 decoy zap entered results as valid
(now dropped + DVA1 retry recovers the real entries); the foreign-vdev
file was left extentless (now planned at the fallback position). Suite
614/614. findings/FIXED/M-019; matrix zfs B1/B2/A25/D15 MATCH.

**M-016 FIXED (first landing):** LDM TOCBLOCK candidate set/order — the
core's list was `{+1,+2,+2045,+2046}` (config+0 never probed; +1 outranked
+2 on conflicts) while the og probes `{+2,+0,+1}` first-valid-wins
(LdmManager FUN_180072ad0, re-verified first-hand incl. the hard-fail
name-check tail). Now `{2,0,1,2045,2046}` — og probe order first, kernel
tail-copy superset behind. Fail-before proven both claims: a config+0-only
disk went ERR_BAD_CHECKSUM (all volumes missed) → full parse; a +1/+2
conflict followed the stale +1 VMDB (committed_seq 42) → now the +2 anchor
(99). The og's stop-at-magic hard-fail on corrupt bitmap names stays
un-adopted (forgiving per-copy walk = registered recovery superset). 3
tests; suite 594/594; findings/FIXED/M-016; matrix table+ldm B1/C20 MATCH.

## Session-15 delta (2026-10-26 — DFX10: the S-009 third-implementation convergence + M-054) (2026-10-26 — DFX10: the S-009 third-implementation convergence + M-054)

Session bootstrap `7c44bdc` (baseline 533/533 Release verified first-hand);
mid-flight the remote advanced to `d487928` (the parallel lanes' S-009
free-overlay landing via `storage::FreeMap`, S-030 disk-image targets, M-002,
M-051, M-037, N-073, N-042/N-044 fixes) — absorbed by rebase; the union rules
applied (the landed implementation is the base). Corpus `03ad46c` unmoved.

**S-009 (convergence record):** this session had built a THIRD independent
implementation (`cr::AllocClusterMap` — used-bits, slice-relative,
byte_origin, fail-open truncation, RLE persistence; 5-test suite +
behavioral fail-before proofs) when the parallel lane's design landed first.
The landed `storage::FreeMap` stands; the third design is preserved on
branch `s009-allocmap-impl` (`fb2bb91`) for the record and dropped from the
tree. S-009's own Resolution (the landed one) remains operative.

**M-054 (NEW-FINDING, MAJOR, filed + FIXED this push):** discovered during
the third-implementation's exFAT producer verification — the engine's
allocation-bitmap `Bitmap::test` read bit `c` for cluster `c`, but the MS
exFAT spec §7.1 pins bit `i` = cluster `i+2` (the first cluster has index 2).
TWO consumers inherited the shift: deleted-viability grading (wrong
confidence grades on every real volume) AND the just-landed S-009 free-map
producer (free overlay painted cluster `c` per cluster `c+2`'s state). The
core's own fixtures encoded the same shift, so the suite was
self-consistently wrong. Fixed: `Bitmap::test` reads bit `c-2` (clusters 0/1
reserved-never-heap); both fixtures write the spec-correct bit; grading
expectations unchanged (the intended behavior); NEW pin test
`ExfatEngine.FreeMapProducerMatchesSpecBitMapping` guards the producer seam.
**Fail-before EXECUTED on the landed base**: fixtures corrected + engine
body reverted → `ExfatEngine.EndToEnd` red (over.dat Excellent instead of
Overwritten) AND the new producer pin red; fixed → green. Suite after:
**574/574 Release + 574/574 ASan + 574/574 UBSan** (three lanes, executed).

Also this push: a committed conflict-marker remnant repaired in worklog.md
(the trailing `
**Counts after this push (direct file inventory):** SEVERE **1** (S-029 —
S-030 landed by the parallel lane) · MAJOR 34 · MINOR 48 · INFO 22 (all
ACKNOWLEDGED) · FIXED/ family **87 files** (incl. RESOLVED-INDEX; +M-054
this push; S-009/S-030/M-002/M-051/M-037/N-073 by the parallel lanes).
Top remaining risks: S-029 (the apply-path scope decision — the last
SEVERE), then the M-tier by user impact (M-005 the carve gate next — same
bitmap plumbing, one consumer).
# (session-20 delta below — the session-22 file header above is current)

## Session-20 delta (2026-10-26 — DFX20: M-033, the BitLocker in-stack decrypt)

Leg ID history: stamped DFX17 at bootstrap, re-stamped DFX18 then DFX20
(two parallel legs squatted each name in the same window; the union kept
all three bootstrap entries verbatim). Bootstrap baseline **584/584**
linux-gcc (RelWithDebInfo + ASan) at `5d63124`; after absorbing the
parallel lanes' landings (M-007, M-008, M-045, M-054, M-005, N-030, N-041,
N-069) the pre-fix baseline re-verified **608/608** (the count grew with
the absorbed waves). Corpus `03ad46c` unmoved. Lane claim: the UNCLAIMED
MAJOR tier (M-033 first), posted before any finding work.

**M-033 FIXED (the in-stack BitLocker decrypt — CONFIRMED-DIVERGENCE,
verified against the corpus first-hand):** the og decrypts locked volumes
in-stack (architecture doc §8: "FVE keys -> XTS-AES decrypt
(BitLockerLib/OpenSSL)"), engines see plaintext; our core refused locked
volumes unless the OS had them unlocked. The fix lands the layer exactly:
`DecryptingDevice : IStorageDevice` (cr_decrypt_device.h) wraps the volume
slice with per-sector LE-index XTS tweaks — physical -> slice -> [XTS]
-> engine, the og's order. AES-128/256 + XTS are clean-room
(cr_aes.cpp, no new third-party dependency — the tree stays
zlib/lz4/zstd-only and Win7-inclusive) and pinned **byte-exact against
vectors generated independently by pycryptodome AND OpenSSL EVP 3.5**
(the EVP interface the og's BitLockerLib statically links) plus the
FIPS-197 Appendix C KATs; XTS weak-key halves are rejected (OpenSSL 3.x's
own rule). Key surface = the og's own API shape: the user supplies the
FVEK (GetFullVolumeEncryptionKey's >=0x40-byte output, decompile
:27075-27107) via `scan --volume X: --bitlocker-fvek <hex>` or the
additive ABI export `cr_ui_open_volume_with_key` (no version bump —
the documented additive policy). **Safety posture preserved**: locked
WITHOUT a key keeps the ERR_FVE_LOCKED refusal (S61
legitimate-unlock-only; FB-B01 never-scan-blind); auto-unlock-able
volumes still take the OS path; a wrong key yields honest detection
failure, never wrong-file results. The recovery-password/BEK -> VMK ->
FVEK unwrap chain is out-of-corpus (ParserBitLocker :27633's datum parse
sits behind opaque C++ graphs — FUN_180042350/FUN_180044e90; the
finding's own open question says so, confirmed first-hand) — filed as
tracked-open **OQ-S20-1** with the exact decompile entry points and next
step. Fail-before intrinsic (the device/crypto types did not exist);
13 tests in test_bitlocker_xts.cpp prove AES KATs, XTS cross-impl
vectors, whole-sector vectors, round trips, weak-key rejection, tweak
separation, plaintext reads through the wrapper, byte-exact
unaligned/cross-sector reads, transport-error propagation, and the
M-033 acceptance (an XTS-encrypted FAT16 volume is opaque to the raw
engine and scans correctly through the decrypt device with the key).
Suite 608/608 at the M-033 landing; **685/685** after M-042.

**M-042 FIXED (the resume disk-truth pass):** CONFIRMED-DIVERGENCE on all
three falsifiable deltas (blind restore, no partinfo equivalent,
config-gated candidate persistence, thin per-record payload), fixed per
the finding's own 5-step plan. lost::revalidate_candidate re-runs the
SAME per-family probe the discovery uses, at the persisted start LBA,
through the CURRENT disk, at both device-bearing surfaces (the listing
loads gained revalidate_disk - cmd_recover now opens the source disk
BEFORE the session load; cmd_apply's candidate listing is re-validated on
the once-opened view it reuses for plan_apply - and run() start for the
resume flows, whole-disk basis only). REC_PARTITION_CONTEXT v2
(append-only, length-versioned like SessionState v2) carries
bps/SPC/hidden/flags; REC_DISK_LAYOUT (type 8) is the partinfo.dat
equivalent with the layout_changed signal surfaced at resume (CLI warns;
never enforced); a checkpoint is FORCED the moment discovery produces
candidates (sink-gated - the no-.dat-side-channel residue registered, not
papered over). **Executed fail-before**: the re-validation gates disabled
-> ResumeDropsStaleCandidateAfterRelayout RED on both surfaces; restored
-> green. 6 tests in test_resume_revalidation.cpp; suite 685/685.

**Counts after this push (direct file inventory):** SEVERE **1** (S-029,
DFX10's claim) · MAJOR **26** (M-005/M-007/M-008/M-037/M-045/M-046/
M-054 landed by parallel lanes since session-15's count; M-033 + M-042 this push)
· MINOR **41** (parallel lanes landed N-041/N-069/N-030 since the
session-15 count) · INFO 22 (all ACKNOWLEDGED) · FIXED/ family
**103 files** (incl. RESOLVED-INDEX; +M-033 +M-042 this push; the parallel waves'
M-005/M-007/M-008/M-045/M-054/N-041/N-069/N-030 already inside) ·
tracked-open **17** (+OQ-S20-1 this push). Top remaining risks: S-029 (the last SEVERE), then
the MAJOR tier by user impact — M-042 (session-resume revalidation),
M-017 (lost-partition scoring), M-019/M-020/M-038 (ZFS/btrfs parity
depth), M-039 (occupancy bitmap) are this lane's next queue.
# SUMMARY.md — executive summary (session 11 DFX11 on top; earlier sessions below)

## Session-11 delta (DFX11, this leg — S-009 verified + M-007 landed)

Session arc: bootstrap 7c44bdc (baseline 534/534) -> the S-009 convergence
(this leg implemented the full free-overlay stack independently; the
parallel DFX8-legacy lane landed first at c55bd45/a3e5fd8 — first-landing
wins, this leg's implementation dropped cleanly and the landing verified
first-hand at 575/575 with two corpus arbitrations recorded in the worklog:
the FAT free-map is og-faithful per searchdll-family :98 "volume bitmap
from FS engine"; the bitmap-truth free rule is closer to the og than the
finding's plan text) -> M-007.

**M-007 FIXED (this push):** hard-link emission parity.
`WalkerFlags::single_link_per_record` (default TRUE = the og's model: ONE
node per record - the namespace-best $FILE_NAME from the FULL list,
first-seen wins ties, under ITS parent; corpus FUN_180006db0 :5273 +
FUN_1800448b0 :54124-54162 + FUN_180043330). FALSE = the registered
opt-in fan-out (per-parent nodes, per-link names; PARITY-DELTAS D-65);
the hardlinks stat counts only in fan-out mode. The fan-out path's
per-parent name selection was REPAIRED in the same change (per-parent
names from that parent's OWN entries - a pre-existing divergence-class
bug no earlier fixture exposed: on mixed-rank multi-parent records every
parent inherited the highest-rank sibling's name). 4 tests (2 new +
EndToEndWalk/AdsAttachesToFirstLink rewritten for both modes); fail-before:
walker-revert (3 nodes vs 1; the Win32/DOS links unfindable). Suite
582/582. Counts after this push: SEVERE 2 (S-029, S-030) · MAJOR 36 ·
MINOR 48 · INFO 22 (0 actionable) · tracked-open 16 · FIXED 83 files
(82 findings + RESOLVED-INDEX).

**M-008 FIXED (this push — premise corrected against the corpus):**
`WalkerFlags::single_hop_attrlist` (default FALSE = the multi-hop BFS
superset, D-73, per the finding's own recommendation); TRUE = og parity
(depth-1; corpus FUN_180041df0 :52615-52687). The finding's "deleted
records never chased" premise was STALE — it read FUN_18001acc0 (the
GUID-DUMP pass 2, S-012's decode) as a deleted-phase walk; pass 1
(FUN_180017390 :18423+) emits every record AND builds the child list
(:18461-18464, plVar9 non-NULL), so the og chases deleted records at
depth 1 too — the fix-plan's deleted-gate step was REJECTED (corpus
wins) and OQ-S2A-1 resolved by the same evidence. 3 tests (2 new +
NestedAttrlistChase as the default guard); fail-before: walker-revert
(2 vs 1 extents). Cross-lane repair in the same push: the M-054
landing's +2 coverage hardening missed the S-009 leg's
FreeMapProducerMatchesSpecBitMapping expectation (100 -> 102). Suite
593/593. Counts after this push: SEVERE 2 · MAJOR 34 · MINOR 48 ·
INFO 22 · FIXED 84 files (83 findings + index).

**M-032 FIXED (this push — the fix plan's own decision path 1):** slice
tail-read semantics. The decision gate executed first: the engine audit
found no reliance on the og's clamp+zero-fill contract (HFS+ journal ring
reads bounded within the ring; all other reads explicitly bounded;
refusals degrade via damage counters). The safer refusal stays; the
divergence registered D-74; pinned by Slice.TailReadRefusalPinned + the
updated BoundsEnforcement (corpus FUN_180009290 :7219-7264; the og's
sector-unit under-zeroing defect noted, NOT adopted). No behavior change
- the opt-in clamped read from the finding's path (2) is the documented
next step WHEN an engine needs it. Suite 594/594 (Slice +1).

**M-050 + N-058 FIXED (this push — one loader change, per M-050's plan):**
the carrier audit re-scoped post S-023/M-051/M-047: grade+overlap,
provenance, root stats, short names ALREADY carried (the predicted schema
debt was paid by those landings) — now PINNED by
Session.GradeRoleAndProvenanceRoundTrip + OldFormatStillLoads; roles
persist as strings (richer than the og's u32 pair). Deliberate
NOT-PERSISTED, OQ-tracked: hidden-root bit, f_6c/f_68, f_71 (undecoded —
carrying opaque u16s would invent structure). N-058's four loader
hardenings in the same change: forward parents DEFERRED + resolved
post-loop (reparented count surfaced via the new optional out-param);
marker outside {0,1} fails the record; c.fail alone fails the load;
name/role lengths bounds-checked BEFORE allocation. 6 tests; suite
602/602. Counts: SEVERE 2 · MAJOR 32 · MINOR 46 · INFO 22 · FIXED 86
files (85 findings + index).

**M-029 (partial — three sub-items closed, the finding stays ACTIVE):**
Type sort CLOSED (SortKey::Type, ABI code 7 append-only — iconIndex
category order then extension then name; pinned with the full 8-row
sequence); the modified-unit doc lie FIXED (FILETIME 100-ns since 1601 —
the old "Unix seconds" line was false; pinned by
Query.ModifiedUnitIsFiletimeEverywhere; the stale Category enum row
fixed too); paging DOCUMENTED as the deliberate difference (no og
page-size constant — UI-layer virtualization). Remaining (decode-blocked,
next steps recorded in the finding): the bucket-ID translation (the og's
buckets select by node+0x89 date TAGS, not ranges) + the general-raw
naming bucket. Suite 636/636.

**M-049 (partial):** the header's false parity claim REWRITTEN to the
value-space truth (family-prefix convention only, D-68; the og has only
two corpus-documented 12-digit codes — renumbering to them would break 26
others arbitrarily); err_text tested for the first time (18 codes,
distinct texts, fallback, OK=0). The engine-side error-context subset
stays open (design-gated; next step in the finding). Suite 643/643.

# SUMMARY.md — executive summary (diff-fix session 14 on top; earlier sessions below)
## Session-14 delta (2026-10-26 — DFX14: bootstrap, two merge repairs, the reconciliation wave)

Session bootstrap `7c44bdc` (baseline **533/533** Linux Release, no flakes);
corpus `03ad46c` unmoved. Two merges absorbed the parallel legs' landings
(S-009 free-overlay by the DFX8-legacy lane c55bd45/74e4d36; N-020 doc drift
+ the DFX12 bookkeeping wave; N-042/N-044 + the DFX11 hygiene wave). The
second merge caught and repaired a disguised rollback: **5aaf044
("style(lint)") was a four-finding semantic clobber** — S-009's cr_ui.cpp
snapshot capture, N-042's decidegrees print, N-044's --whole-disk rename, and
M-012's cr_chain mirror scrutiny, 83 lines deleted under a formatting label.
Restored from the verified-green f15c286 state in the same merge (c4e37d1);
suite 547/547 then 553/553 proved it. Protocol lesson recorded: never absorb
a "style" commit without diffing it.

**N-073 FIXED-ALREADY (this push):** the ext4 CatchBuffer engine-path residual
— N-004's transport-layer landing (`7f03c4b`) covers it by composition: every
orchestrator-driven engine read goes through the retry pipe (cr_scan.cpp:714/
723 pass `pipe` into run_quick/run_deep). Proven with the three engine-path
tests this finding's own plan named, via an ExactFaultDevice alignment-fault
model on the production pipe shape: `Ext4Engine.CatchBufferBacktrackSalvage`
(tree intact, salvaged range never recorded bad), `Ext4Engine.BacktrackOff-
ZeroFillsTheWindow` (the policy flag reproduces the pre-N-004 world — the
finding's exact claim), `Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails` (only
the truly unreadable half zeroed; error map exactly [5632,512)). Suite
**553/553**. Finding → FIXED/; RESOLVED-INDEX row; matrix ext4 row; ext4.md
S35 + both index rows (incl. the N-057→N-073 renumbering correction).

**M-029 duplicate reconciled (this push):** the merge-era fragment
`M-029-query-model-gaps-sort-paging-buckets-server-push.md` (a stray session-5
addendum under a duplicate ID — no title, no finding header) folded verbatim
into the canonical M-029 file and removed. The canonical remains ACTIVE (Type
sort key, bucket-ID translation, paging parity claim, server-side push).

**M-005 FIXED (this push):** the carve allocation-bitmap gate. Corpus
re-verified first-hand before implementing (RAWSearchDLL :9440-9466 +
FUN_180058df0 :67842-67856) — three details richer than the finding's
paraphrase, all folded in: OOB/no-bitmap dispatch proceeds (raw-disk rule);
the cluster jump requires cluster alignment; the og's bitmap is the USED
map so the core's inverted FreeMap flips the gate's polarity. The gate
lives in BOTH dispatch layers (grammar per-byte + raw-sign 512-stride);
the map enters via CarveFlags.free_map with the orchestrator rebasing
stats.free_map (S-009's producer surface) slice-relative at the call site.
Tests: Carve.BitmapSkipsUsedClusters + Carve.NoBitmapCarvesEverything
(null AND invalid maps carve all). Bench (scripts/bench_m005.cpp, 16 MiB
LCG-random, 50% used, median of 3, two invocations): 1137.8/1124.7 ms
ungated vs 606.3/594.4 ms gated = **1.88x/1.89x** (plan expected >= 1.5x).
Bench lesson recorded: all-0xFF fixtures are adversarial (FMT_MPGA_FF's
{0xFF,0xFF} magic — parity with the og's FF sync family); representative
mixed-content fill is the honest measurement. Suite 575/575.

**M-043 FIXED, one sub-claim DISPROVEN (this push):** NTFS recognition
content proof. Corpus re-verified FIRST — and it overturned the finding's
first half: FUN_180005470 decoded in full shows the analyser gates on the
PRIMARY boot and bails before result 4; its "backup-boot verification"
(:4309-4327) is a memcmp storing the verified location at +0x510 as the
REPAIR input — and NTFSSearchDLL carries ZERO backup reads. The og has NO
scan-path backup fallback; the core's refuse-then-rawfixer shape was
already parity on that half (no fallback code added — the finding's text
is corrected, not "fixed" by breaking parity). The CONFIRMED half landed
at the exact og boundary: probe_family's NTFS branch now applies the
og boot gates (bps 512-multiple <= 0x1000; SPC pow2 — NFA :4292-4307) +
record-0 proof (FILE magic; USA count [1,0x11); USA offset [0x2a,0x201) —
FUN_180005180 :4038-4058), one 512-byte read at $MFT LCN. Executed
fail-before: ProbeFamily's spoofed-OEM leg (a FAT volume with a spoofed
"NTFS    " OEM used to dispatch the NTFS engine — the existing test
ASSERTED the divergence). New: NtfsProbeContentProof (5 legs) +
TornPrimaryBootWithIntactBackupStillRefusedUntilRepair pin. Suite 599/599.

**Counts after this wave (direct file inventory):** SEVERE 2 (S-029, S-030) ·
MAJOR 35 · MINOR 48 · INFO 22 (0 actionable) · tracked-open 16 · FIXED 84
files (83 findings + RESOLVED-INDEX).

# SUMMARY.md — executive summary (diff-fix session 11 on top; earlier sessions below)

## Session-11 delta (2026-10-26 — the MINOR-first close-out wave)

Session start `a5bbcfa` (DFX7-T2 inventory head; S-032/N-018/N-042 legs
already landed by the parallel sessions); corpus `03ad46c` unmoved.
Baseline before any change: linux-gcc (RelWithDebInfo + ASan) **534/534**
green. Mandate: minor findings first, 20+ fixes, fixed findings moved to
findings/FIXED/, all docs/counts current.

**Audit-trail hygiene wave 1 (this entry):** four fully-resolved MINOR
findings moved from MINOR/ to FIXED/ with RESOLVED-INDEX rows — **N-013**
(all-extent destination arming; image→same-volume sub-claim REFUTED with
corpus evidence, `CrUi.ImageSourceDestinationNearImageAccepted` pins),
**N-015** (`ERR_DEST_FULL` end-to-end), **N-018** (UBSan lane, `b1f9210`),
**N-042** (one decidegree source, `3678cbf`). Matrix cells
(writer+report ×2, health ×2, quality) + modules/writer.md + modules/health.md
verdicts updated; a pre-existing joined-row corruption in the matrix
session-5 section (carve ‖ lost-partition fused into one line) repaired.
**Convergence note (rebase):** the parallel DFX12 lane landed an overlapping
reconciliation (`f4fe677`) in the same window — moving N-004/N-005/N-006
(now all three FIXED — the group file restored from the b2bd91b truncation)
and **N-044** (`779eb21`: `--deep` renamed `--whole-disk`, deprecated alias
kept) in addition to the same four. Union kept both records; the duplicate
RESOLVED-INDEX rows were reconciled to one set in the same push. The group
files **N-009 and N-036 stay in MINOR/** until their open remainders
(N-009c CLI --filter UTF-8, N-009d ACL preservation; N-036 container-map
half) close.

**Counts at session-11 start (direct file inventory at `a5bbcfa`, after
the hygiene move):** SEVERE 3 (S-009, S-029, S-030) · MAJOR 39 ·
MINOR 51 (44 fully-unresolved files + N-004/N-006 + N-009 + N-036 groups
with open remainders) · INFO 22 (all ACKNOWLEDGED) · FIXED 76 finding
files + RESOLVED-INDEX · tracked-open 16. Subsequent session-11 landings
append their deltas below as they push.

**Session-11 landing 1 — N-069 FIXED:** CAB lowercase-key parity. The og's
registry carries duplicate keys for case-insensitive formats ("MSCF"/"mscf"
— census :94, "deliberate registry bloat for correctness" :127); the core
keyed uppercase only. `FMT_CAB_LC` appended (serialized ids stable) with a
registry row sharing `validate_cab`; the fail-before run exposed a second
site the fix plan missed — `validate_cab`'s internal uppercase-only magic
re-check — now accepting exactly the two registered spellings.
`Carve.CabLowercaseMagic` (fail-before: lowercase cabinet never probed;
pass-after: both cases carve, lowercase node carries FMT_CAB_LC). Suite
568/568 ASan; lint clean. Post-fix counts: MINOR 48 · FIXED 80 findings.

# SUMMARY.md — executive summary (diff-fix session 10 on top; earlier sessions below)

## Session-10 delta (2026-10-26 — DFX10: the S-030 convergence push)

Session start `5ff4db7` (post-T1 bootstrap on `8e79960`-lineage); corpus
`03ad46c` unmoved. Baseline 533/533 Linux Release re-verified at start.
CONVERGENCE NOTE: a parallel session-8 lane landed S-030's first cut
(flat/VMDK/VHD, `b30354d`) mid-flight; this session had independently
implemented the full surface — the union keeps the first landing's audit
record and lands this lane's superset engine (VHDX + log overlay, the
differencing families, .dmg flat-parity correction, the qemu-fixture
compatibility proof layer, image_path widened 256→512). The finding file
carries both Resolutions.

**S-030 FIXED (converged):** the disk-image scan-target surface — the og's
"Import a disk image" input class (extension factory DRWUI:948237-948409,
flat fallback :948389-948393, LBA0 classify :275877-880; Device.mo's
libvmdk block device :49106-49124 with extent files + parent chains
:48965-49100). Pure-core `cr::storage::open_image` behind an injected
`ImageFileIo` seam: flat raw (.img/.dmg/.raw — .dmg opened raw, exactly the
og's fallback, NOT Apple-UDIF parsing), VMDK (monolithicSparse +
descriptor extents FLAT/SPARSE/ZERO, grain directory/table translation,
differencing via parentFileNameHint, depth cap), VHD (fixed + dynamic +
differencing: footer/dynamic-header one's-complement checksums, BAT,
512-byte sector bitmaps, W2ku/W2ru parent locators), VHDX dynamic (header
pair CRC-32C + highest-sequence, region table, metadata, chunk-ratio BAT)
with a READ-ONLY in-memory log replay overlay (qemu vhdx-log.c walk
semantics; D-64d — a recovery tool never rewrites its input). CLI
`scan --image` / `recover --image`, identity-card v3 tail `image_path` +
resume warning, repair-on-image stays analysis-only. Registered
non-adoptions D-64a/b/c: .edisk proprietary, compressed VMDK/VMFS, VHDX
differencing — all refused with precise errors, never silent mis-reads.
**Proof discipline worth recording:** every format constant was pinned
empirically on qemu-img-10.0.13-written files before coding; the committed
compat fixtures (tests/data/images) caught two transposition bugs
(VHDX log-descriptor field order; metadata-region GUID data3) that
in-memory builders — written from the same misread spec-memory — could
never catch. 20 new tests; suite **555/555** Linux Release. New module
row: modules/imaging.md.

**Counts after S-030 (direct file inventory at this push):** SEVERE 2
(S-009, S-029) · MAJOR 39 · MINOR 55 · INFO 22 = 118 active / 73 FIXED
finding files + RESOLVED-INDEX.

**Session-11 landing 2 — S-033 (NEW-FINDING) INVALID + N-067 FIXED:** during
the N-067 verification pass this leg filed S-033 (exFAT type-constants
"shifted") and REFUTED IT THE SAME SESSION with the og's own decompile walk
(:24380-24460): case 0x81 = the bitmap-shape validation (flags@+1 free,
reserved@+2..+0x13 zero, cluster@+0x14/length@+0x18); case 0x82 = the
upcase-shape validation (reserved1@+1..+3, checksum@+4 free, reserved2
@+8..+0x13). Bitmap=0x81/upcase=0x82/label=0x83 — the core's constants
were RIGHT all along; the local branch was reverted pre-push; corpus erratum
E43 filed (the v2 artifact's garbled §3 row labels). N-067 then landed on
the verified constants: the upcase volume-integrity check (og shape gates +
chain-read checksum32 vs TableChecksum, soft `ExfatScanStats` signal,
grade-not-reject) with a real 64-entry fixture table at cluster 60.
`UpcaseTableChecksumValidated` + `UpcaseLengthSanity` (fail-before: the
assertions were RED with the validation unwired). Suite 597/597 ASan;
42/42 UBSan on the touched paths; lint clean. Counts: MINOR 47 · SEVERE 1
(S-029) · FIXED 82 (incl. S-033 INVALID record).

**Session-11 landing 3 — N-008 FIXED (ZIP padding + ZIP64 chain; gate half via
N-068):** the salvage extension is now the og's (:18492-18508) — data-bearing
trailing sectors absorbed sector-strided after a BROKEN chain (cap 0x500001,
device sector size, 4Kn-aware), while the INVENTED zero-absorption after
clean EOCDs is REMOVED (intact ZIPs are never inflated with zero slack —
N-027d's settled reading; the old `ZipZeroPaddingAbsorbed` pin rewritten to
the correct `ZipCleanArchiveNotInflatedByZeroSlack`). ZIP64 indexless
archives no longer truncate: the 0xFFFFFFFF csize sentinel resolves through
the LFH extra field's 0x0001 record. The "admit 0x62" item is REFUTED at
the dispatch layer (the probe tier's ≤0x13||==0x62 is a pre-filter; the
dispatch's ≤10 is the final word — 0x62 > 10). Tests: ZipSalvageAbsorbsData
Sectors + ZipCleanArchiveNotInflatedByZeroSlack + Zip64ExtraFieldContinues
Chain. Suite 685/685 ASan; 55/55 UBSan carve. Counts: MINOR 28 · FIXED 85
(the parallel lanes' N-040/N-046/N-057/N-058/N-077/M-002/M-007/M-044/M-047/
M-050/M-051/M-054/M-056 landings share the window).

**Session-11 close-out (DFX16 leg):** this leg's verified landings —
**N-069** (CAB dual-key), **N-067** (exFAT upcase integrity), **N-068**
(ZIP LFH gates; EOCD half FIXED-ALREADY by the CD anchor), **N-008** (ZIP
salvage extension + clean-archive honesty + ZIP64 chain), **S-033**
(NEW-FINDING filed during N-067 verification and REFUTED the same session
by the og's own decompile walk — INVALID with corpus errata E43; the
exFAT type constants were RIGHT all along), plus the hygiene moves
(N-013/N-015/N-018/N-042 → FIXED/), corpus erratum E44 (the ZIP
entry-point table's labels/bounds), the 5aaf044 "style"-clobber
root-cause + the convergent N-077 leak repair, and multiple union-tree
repairs during the heavily-parallel window. **Final counts at this leg's
close (direct file inventory): SEVERE 0 · MAJOR 18 · MINOR 29 (27 fully
unresolved + N-009/N-036 partials) · INFO 22 (all ACKNOWLEDGED) · FIXED
135 finding files · tracked-open 11.** Suite 691/691 linux-gcc ASan +
640/640 linux-ubsan on the closing tree. No release tag: the audit is
not closed (the MAJOR tier and the session/health/log minor families
remain — see the worklog handoff for the lane map).

# SUMMARY.md — executive summary (diff-fix session 9 on top; earlier sessions below)

## Session-9 delta (2026-10-26 — baseline repair + the SEVERE/MAJOR/MINOR fix waves)

Session start `c231f2f` (audit-trail lineage; the S-021 journal-gate
re-append head); corpus `03ad46c` unmoved. Baseline pull found Linux CI RED:
`cr_ui_abi_check` SEGV — root-caused first, before any finding work (a
session does not fix findings on a red baseline).

**NEW-FINDING M-053 (MAJOR) filed — FIXED-ALREADY (rebase union):** the
M-015 landing (`6dc1e32`) updated the two C++ FAT16 fixture builders with
the canonical media pair but missed the third —
`tests/ui_abi_check.c:build_fat16()` (the ported C-consumer twin). From
`6dc1e32` the plain-C ABI proof was dead (scan `ERR_BAD_MAGIC` on the
invalid fixture → NULL rows → SEGV at :576) and the Linux CI gate red on
every push in the window. This session root-caused it independently
(worktree bisect: parent `63d9067` green / landing SEGV; standalone repro
of the full failure chain; own fixture repair 518/518 green) and filed the
finding per the discovered-divergence SOP — while the parallel DFX7 lane
landed the byte-equivalent fixture repair inside `b1f9210` (N-018's UBSan
defect set) WITHOUT filing the regression. The rebase union kept their
bytes; M-053 is the formal record of the red window, classified
FIXED-ALREADY with both proofs. Evidence:
`evidence/executions/s9-m053-abi-fixture-regression.md`.

**Counts at session-9 start (direct file inventory at `c231f2f`):**
SEVERE 5 (S-009 residual, S-029, S-030, S-031 stub — timestamp residual,
S-032) · MAJOR 40 · MINOR 55 · INFO 22 = 122 active; findings/FIXED/ 67
finding files + RESOLVED-INDEX; tracked-open 16 files. After the M-053
filing+fix: active count unchanged (122 — the finding was born fixed in the
same push); FIXED 68 finding files. Subsequent session-9 landings append
their deltas below as they push.

# SUMMARY.md — executive summary (session 12 on top of session 9; earlier sessions below)

## Session-12 delta (2026-10-26 — the minors-first reconciliation wave, DFX12)

Session start `7c44bdc` (bootstrapped at `7444fd6` after rebase onto the
parallel lanes' N-044/UBSan-CI landings); corpus `03ad46c` unmoved.
Baseline before any change: Linux Release suite **533/533** green.
Order per user mandate: MINOR-first waves; every finding independently
re-verified before fixing; fixed findings move to `findings/FIXED/`.

**Audit-trail reconciliation (this wave):** six MINOR files whose fixes
landed in prior sessions (commit evidence verified first-hand: `7f03c4b`,
`915bce3`, `b1f9210`, `3678cbf`, `779eb21`) had never been moved out of
MINOR/ — N-004/N-005/N-006 (group), N-013, N-015, N-018, N-042, N-044
now live in FIXED/ with RESOLVED-INDEX rows. **Audit-trail damage
repaired:** the b2bd91b "housekeeping" commit had accidentally truncated
the N-004/N-005/N-006 group file from 99 lines to the 7-line N-005 stub
(the original finding text + the N-004/N-006 Resolution sections were
lost from the working tree; git history held them) — restored in full
and all three cited test sets verified present (Retry.Backtrack* x3,
HfsPlus.LargeBlockSizeAccepted, Btrfs.NonPow2NodesizeRejectedRegistered).
N-009 (c/d sub-rows open) and N-036 (container-map half open) stay in
MINOR/ as partially resolved by design.

**Counts after the reconciliation (direct file inventory at this
commit):** SEVERE 3 (S-009, S-029, S-030) · MAJOR 39 (incl. the known
duplicate M-029 pair) · MINOR 49 · INFO 22 (all ACKNOWLEDGED — 0
actionable) · tracked-open 16 · **FIXED/ 79 files (78 findings +
RESOLVED-INDEX)**. Session fix tally (resolutions this session): 6
moved-with-evidence + the group-file restoration.

**Carve-magic quartet FIXED — CONVERGENT LANDINGS (this wave):** two
parallel legs independently implemented the same four findings from the
same corpus citations (the parallel lane's `b2f48c6` landed first and is
canonical: bounded-buffer MKV DocType webm/matroska walk + the 0x200
element-bounds gate; FMT_HEIF registry row for the HEIF relabel;
FMT_TIFF_EP claims the IIU key with FMT_RW2 as the registered salvage
D-63a; RAF keyed on the FUJI 4CC with the variant path, JPEG prefix
registered D-64a). This session's convergence contributions: the
independent corpus decode of the artifact's second constant 0x67805318
(the Segment element ID 0x18538067 read little-endian — recorded in the
finding + module doc for future audits), a second fail-before proof set
(8/10 of this leg's name-identical tests RED on the stash-isolated
pre-fix engine), and the finding-file moves to FIXED/ with the union
Resolutions. **NEW-FINDING N-076 (DFX12, unique to this session):**
while building N-054's fixtures: the canonical RAF version gate
rejected every real-world RAF — the version at +16 is a 4-byte ASCII
string ("0100"/"0201") misread as binary (LE values ~8e8 above the
0x2000 cap); BOTH quartet landings carried the defect; the row was dead
for real files since introduction, zero fixture coverage. Fixed in the
union follow-up: ASCII-digits OR small-binary acceptance;
Carve.RafRealAsciiVersionAccepted (fail-before executed RED on the
pre-fix engine). Suite **563/563** linux-release post-union.

**Counts after the quartet (direct file inventory at this commit):**
SEVERE 1 (S-029 — S-009 and S-030 landed by the parallel lanes in the
same window) · MAJOR 36 · MINOR 45 · INFO 22 (all ACKNOWLEDGED) ·
tracked-open 16 · **FIXED/ 89 files (88 findings + RESOLVED-INDEX)**. Session resolution tally so
far: 6 reconciliation moves + the N-076 discovery-fix + the quartet
convergence work. Ongoing: MINOR-first waves; N-073 remains a probable
N-004 duplicate — verification queued.

**Carve-minors batch FIXED (this wave):** N-069 (CAB lowercase mscf key —
the og's duplicate-key policy for case-insensitive formats, census
:94/:127 re-read first-hand: a second FMT_CAB_LC row claims the
lowercase magic with the same validate_cab;
Carve.CabLowercaseMagic, fail-before by construction — the byte-exact
scan could never match a key no row claimed) and N-030 (raw-sign
confidence semantics — resolved as the REGISTERED DELIBERATE SUPERSET
its own fix plan calls option (b): the field is dead data in the og
(FUN_18001e510 :23522-23765 never reads part[2]; erratum 19), the core's
winner-selection + (hdr+tail)/2+100 blend are pinned by
RawSignLayerHeaderOnlyAndSharedMagic / RawSignTailPairWithinFormat,
PARITY-DELTAS D-66 carries the do-not-remove guidance, and option (a)
stays gated on the undecoded FUN_18000d670 grade emission). Suite
**577/577** linux-release. Counts: MINOR 43 · FIXED/ 91 files (90
findings + index). Session unique resolutions: 9 (6 reconciliation +
N-076 + N-069 + N-030).

**Dead-surface batch FIXED (this wave):** N-041 — the wire-or-delete
decision executed as DELETE (the finding's own recommendation): the
structured-open RdaClient class + the plumbing only it reached
(rda_bind_structured, the device's structured_/OPEN_TARGET reopen
branch, the orphaned grammar-bridge helpers) + volume_letter_to_disk —
~16 KB / 511 diff lines of zero-caller code removed. The wire structs
stay in cr_rda.h as the documented driver-contract mirror (marked); a
CI dead-surface absence lint (the fix plan's
Transport.GrammarPathIsSoleSurface, implemented as a lint-job grep gate)
guards the symbols' return. The PARITY-REFACTOR-PLAN D-A KEEP ruling
was SUPERSEDED with evidence (its cited TRANSPORT-CONTRACT §8 no
longer exists in the tree; the "UI reads info()/ioctl_base()/session_*
through it" claim is disproven by the exhaustive zero-caller grep).
PARITY-ARCH.md:152's refuted claim corrected; AUDIT-ARCH ARCH-DEAD-1
RESOLVED / ARCH-DEAD-2 re-annotated; CAPABILITY-MATRIX +
DRIVER-UTILIZATION + PARITY-ORCH rows updated; FB-T06/FB-T07 moot.
Suite 584/584 linux-release; format-churn hunks reverted to HEAD bytes
so the format-18 CI gate sees only the intended diff. Counts: MINOR 42
· FIXED/ 96 files (95 findings + index). Session unique resolutions:
10 (6 reconciliation + N-076 + N-030 + N-041; N-069/N-051..N-054
convergent with the parallel lanes).

**Text-policy batch (this wave):** N-016a (the N-029 group's first half)
FIXED — text::utf8_to_u16 now follows the maximal-subpart policy (RFC 3629
§3.1; MultiByteToWideChar(CP_UTF8, flags=0) semantics, the og's
restore-path conversion): ONE U+FFFD per ill-formed maximal subpart
instead of one per bad byte — a truncated/interrupted sequence consumes
its lead + the continuations that extended it; a fully-read but invalid
sequence (overlong/surrogate/range) consumes all of it; C0/C1 excluded at
lead classification (RFC 3629). Text.MaximalSubpartReplacement (the
finding's named vectors) + four re-pinned policy tests; fail-before
executed on the stash-isolated pre-fix decoder (4 RED). N-016b (raw-bytes
persistence in sessions — versioned name_raw field) remains open with the
precise schema step recorded in N-029's Resolution. Suite 594/594
linux-release.

**Table-visualization batch FIXED (this wave):** N-057 — all three
sub-items: (1) TableLayout::Ebr assigned on extended chains (the
recommended SET; the ABI gains the APPENDED code 3 with 0/1/2 frozen,
the CLI prints EBR, plain MBR pinned; serialized-value stability is why
deletion was rejected); (2) the 64 MiB overlay-budget growth path tested
at the exact boundary (at-budget does NOT grow — the loop's `>`
semantics pinned); (3) the write-only `total` accumulator deleted.
Fail-before executed (both table tests RED on the pre-fix parse). Suite
600/600 linux-release. Counts: MINOR 41 · FIXED/ 97 files. Session
unique resolutions: 12 (6 reconciliation + N-076 + N-030 + N-041 +
N-016a + N-057).

## Session-8 delta (2026-10-26 — the closing-verification fix session, SEVERE lane first)

Session start `4a924d1` (rebased bootstrap `3494606`); corpus `03ad46c`
unmoved. Baseline before any change: Linux Release suite **500/500** +
`cr_ui_tests` 21/21. Order per mandate: SEVERE → MAJOR → MINOR → INFO →
TRACKED-OPEN; every finding independently re-verified before fixing.

**S-009 FIXED (SEVERE — the disk-map free overlay; the last open half of
the S-009/M-006 split file):** the engine bitmap truth is now produced,
consumed, and persisted. `storage::FreeMap` (new type) filled by NTFS
(`$Bitmap` inverted), FAT (table entry-0 = free), exFAT (allocation-bitmap
dentry inverted) — rebased to the absolute device basis exactly like the
tree extents; `build_disk_map` paints `kCellAllocFree` only on
still-unknown cells fully covered by bitmap-free clusters (USED and FsBad
are never overridden — hostile-map precedence proven by test);
`REC_ALLOC_MAP` (new optional record) persists the DOWNSAMPLED
cell-granular map (the same 64 MiB-budget sizing — ≤ 8 MiB on a 16 TiB
volume, never a raw $Bitmap copy) and restores it as the session's
cluster hint; both snapshot paths value-carry the truth (session
snapshots: errors + free map + view bounds; scan snapshots: the live
telemetry error map + the completed map). Doc drift retired (the header's
"not persisted (registered follow-up)" rule; ABI-CONTRACT §7's "3 bad
(marked by the FS)"). 4 fail-before-by-construction tests (the finding
itself grep-verified `kCellAllocFree` had ZERO assignment sites); suite
**515/515 + cr_ui_tests 21/21 + cr_ui_abi_check** green on Release AND
ASan (merged tree incl. the parallel lanes' S-032/N-018 landings).
N-057's bad-overlay coverage item paid; Ebr enum + budget-growth test
remain its scope.

**NEW-FINDING M-053 (MAJOR, filed + FIXED):** discovered during the S-009
verification pass — the C-ABI consumer check (`cr_ui_abi_check`, registered
in ctest) has been DEAD since M-015 landed (`6dc1e32`): the M-015 fixture
update missed `tests/ui_abi_check.c`, whose FAT16 image lacks the canonical
media words, so every scan of it returns `ERR_BAD_MAGIC` and the check's
non-halting CHECK macros cascade into a `rows[0]` NULL-deref crash
(release: silent exit 139 with buffered output lost; ASan: the SEGV
report). Root-caused with a deterministic plain-C reproducer (the sole
differing bytes between the passing C++ fixture and the failing C fixture
are the two media-word pairs). Fix: canonical media words in the fixture +
a precondition bail-out after a failed scan (a real consumer checks the
return code and stops). Engine untouched — M-015's exact-pair validator is
correct og parity. CONVERGENT with the parallel N-018 lane's fixture fix
(`b1f9210`/`abf5663`); this leg's unique contribution is the formal finding
record + the bail-out hardening. `cr_ui_abi_check` green on release AND
ASan (executed both lanes).

**S-030 FIXED (SEVERE — disk-image scan targets):** the og's import
surface re-implemented from format understanding in the new core module
`cr/storage/cr_image.{h,cpp}`: flat files, VMDK (monolithicSparse with
embedded descriptors + standalone descriptors whose FLAT/SPARSE/ZERO
extent lines open through the host-injected ops seam, grain-directory/
grain-table mapping with grain-segment batching), VHD (fixed = flat minus
the trailing footer; dynamic = BAT + per-block sector bitmaps), content
sniffing incl. the TRAILING-footer VHD identity (the tests caught the
head-only sniff missing fixed VHDs), and the og's LBA0 MBR/GPT
classification. The core stays OS-header-free (byte-source injection);
the CLI gains `scan --image FILE` / `recover --image FILE`; the session
identity card gains the v3 image-path tail (v1/v2 back-compat pinned).
8 tests incl. engine scans over a sparse VMDK (< half the flat size,
byte-identical readback) and a dynamic VHD; fuzz_image added to the fuzz
suite; D-64 registers the edisk/dmg exclusions + VHDX/parent-chain
remainders. Suite **525/525 + 21/21 + abi check** green on Release AND
ASan. SEVERE tier: **only S-029 remains** (the consent-gated
lost-partition apply path).

**Counts after the session-8 SEVERE lane (direct file inventory):**
ACUTE 0 · **SEVERE 1** (S-029 — S-032 was fixed by the parallel lane
in-window) · MAJOR 39 (M-053 filed+fixed in-session) · MINOR 55 ·
INFO 22 (all ACKNOWLEDGED) = **117 active / 74 FIXED** (live inventory:
findings/FIXED/ = 75 files incl. RESOLVED-INDEX).

## Session-6 delta (2026-10-26 — the S-024 decoded-truth upgrade)

Mandate: fix ALL tiers; this leg re-verified the landed S-024 grading
fix against the corpus and found the first landing's four documented
analog guesses superseded by decodeable truths. The corpus wins: the
analog temperature-Caution rule (a divergence — the og grader has NO
temperature branch, 0x1808 is never read), the is-SSD analog gate for
SSD-life (the og gates by decoded vendor families, FUN_0040dad0), the
NVMe spec-bit split for critical_warning (the og grades ANY nonzero
byte Bad) and worst-wins aggregation (the og is
bad-count/caution/graded-signal in FUN_0040cdf0) — all replaced with
the exact decoded rules, plus `model_says_ssd` (the 0x1724 writer list
:6300-6344 — the finding's open question #1, now decoded),
`classify_family` (30+ vendor families — open question #2, decoded),
the family-0x13 NVMe path, duplicate-attr-id Unknown, the 5/197/198
all-FF guard asymmetry, and the failing-bit superset properly
registered as D-63. Evidence:
`evidence/decompiled-quotes/s6-health-grading-decode.md`. 12 new/rewritten
Health tests (suite 464 on this tree: 460 pass + the 4 pre-existing
Ext4Engine.Backup* reds owned by the parallel S-027 lane); fail-before by
construction (the landed header lacks classify_family/make_grade_inputs).
Classifier residual: the exotic-vendor thunks stay generic 0/1 — a pure
data gap, engine consumes all family codes; exact next decode sites
listed in the evidence §5.

**M-036 FIXED (this push):** NVMe health's full row set — host reads /
writes, controller busy time, error-log entries (spec offsets; the og's
route was already a byte-parity MATCH per I-018) — plus the data-unit →
byte conversion exactly as the og computes it (×512,000,
DiskHealth_decompiled.c :5589-5614, re-verified first-hand; a full
64-bit multiply). NvmeHealthInfo extends, the cr_ui C-ABI report
appends the six fields with CR_UI_ABI_VERSION 1 → 2 (ABI-CONTRACT §9;
contract docs updated in the same change), the CLI prints bytes like
the og. 2 new tests + the moved 192-byte gate; suite 485/485 Linux
Release. N-009a's pin re-verified in the same file.

**Session-6 leg final counts (direct file inventory at 70ebced, after
the parallel legs' concurrent landings):** FIXED/ 70 findings ·
**SEVERE 4** (S-009, S-029, S-030, S-032) · MAJOR 39 · MINOR 55 ·
INFO 22 (all ACKNOWLEDGED) · tracked-open 16. This leg closed S-024
(decoded-truth upgrade) + M-036 + N-055; the parallel legs closed
S-007/S-025/S-027/S-028/S-031/M-011/M-012/M-015/A-005/A-007 and more
during the same window. Suite 526/526 at 26d842b. Top remaining risks:
S-029 (apply-path scope decision pending), S-030 (mid-flight disk-image
lane — re-verify landed state first), S-009 (unclaimed since session 1).

# SUMMARY.md — executive summary (session 7 on top of session 5b; earlier sessions below)

## Closing-session delta — S-032 (2026-10-26)

**S-032 FIXED (this push):** archive inner-entry inventories (the og's
analysers return the complete inventory; the results tree presents it —
rawsearchdll--zip-analyser-carving.md :29-38/:64,
analyser-granular-batch1 :36-41, all re-verified first-hand). ZIP CD walk +
indexless LFH chain + RAR4 FILE blocks collect bounded entries; carved
archive nodes gain Carved+Deleted children with precise in-parent extents
(registered superset: per-entry recovery). DISCOVERED+FIXED in the same
change: the RAR4 validator's HEAD_SIZE off-by-one (no positive real-layout
fixture had ever existed — real archives now parse). Remainders tracked:
RAR multi-part stitching; RAR5 entries (clean-room surface). 3 tests, all
fail-before; suite 517/517; ASan/UBSan clean over the carve suite.

## Session-7 delta (2026-10-26 — the close-all-tiers fix session, low-severity-first order)

Mandate (user-specified ordering): INFORMATIONAL → MINOR → MAJOR → SEVERE →
TRACKED-OPEN, with every finding independently re-verified against corpus
`03ad46c` and the running code before any change; fixes proven with tests;
audit trail exact. Session start `c75f646` + phantom-08c66f8 repair
(`4702ec8`); corpus `03ad46c` unmoved.

**INFO tier CLOSED (this push):** all 18 INFORMATIONAL findings now carry
Resolution sections — I-001/I-002/I-003 were already acknowledged (prior
sessions); the remaining 15 (I-006..I-020) were verified this session and
ACKNOWLEDGED with first-hand citations (no rationale was found wrong; no
code change was required by any). Actions landed with the batch:
the I-009 session-3 addendum's pin test
`CarveEngine.CarvesMidSectorStart` (PNG at sector+137 — the byte-granularity
superset over the og's whole-sector stepping; guards the M-025 rework);
PARITY-DELTAS **D-59a..D-59k** (the registration rows the findings' fix
plans demanded); modules/visualization.md's stale D16 gap figure updated
(I-011's leftover action — M-023 FIXED superseded it). Suite **440/440**
Linux ASan green. Committed conflict markers in CROSS-REF-MATRIX.md
(phantom `08c66f8` leg, introduced by `c75f646`) resolved as DFX7-T0.

**Counts after the INFO batch (direct file inventory at the push head):**
ACUTE 0 · SEVERE 9 · MAJOR 38 · MINOR 31 · INFO 18 (all ACKNOWLEDGED —
0 actionable) = 96 active / 49 FIXED; INFO tier now zero-open.

## Closing-session verification delta (2026-10-26 — the df5 lineage verification pass)

The S-003/S-021 lane (claimed at ff05314) was landed by the parallel close-all-tiers
leg at c75f646; this lineage executed the independent verification pass it had
already completed the corpus decode for (CHFSPlusJournal ctor/walker/validator
asm-exact + TN1150 journal-section fetch verbatim). **Corrections landed:** the
false-parity JIB layout (a 'jnlx'@+4 magic TN1150 does not have — device_signature[8]
lives there; a silent zero-harvest on every real journaled volume, fixtures encoded
the same fiction), the two missing og entry gates (VH kHFSVolumeJournaledBit,
JIB kJIJournalInFSMask), the asm-strict node validator (mid-chain offset-table
violations REJECT), the missing attributes branch (decmpfs size-only fallback), the
header-invalid fallback salvage ladder (OQ-S003 RESOLVED — FUN_180003ea0 decoded:
scratch-node harvest, never a live-tree install), and the in_fork u64 overflow.
The max_blocks probe-marker derivation is RESOLVED ((blhdr_size-16)/16). Fail-before:
6 tests red on the pre-correction tree; pass-after 491/491.

**NEW-FINDING A-005 (ACUTE) filed during the same verification:** the
extents-overflow record parser read its 8 descriptors from key+10 — the
TN1150-verbatim key is 12 bytes (keyLength field + 10 data bytes), so fragmented
forks (>8 extents) silently lost their tail extents (the bounds clamp dropped the
garbage descriptor). Zero prior test coverage (every fixture's overflow fork was
empty). FIXED this session with the first populated overflow fixture; fail-before
executed (1/2 extents) — `evidence/executions/a005-extents-off-by-two.md`.

**Counts after the closing-session verification pass (direct file inventory at
the correction commit):** ACUTE **1** (A-006 rawfixer 4Kn — the diff-fix s4 leg's
claimed lane; A-007 extents-overflow was filed AND fixed in-session, moved to
FIXED/) · SEVERE 5 (S-009-remainder, S-029, S-030, S-031, S-032 — the parallel
legs closed S-007/S-023/S-024/S-025/S-027/S-028 in the same window) · MAJOR 42 ·
MINOR 44 · INFO 20 = **112 active** / **63 FIXED-family files** (62 findings +
RESOLVED-INDEX, incl. OQ-S003 resolved this pass; A-007 among them). The parallel
legs' own SUMMARY sections below carry their in-flight counts; this paragraph is
the closing-session's file-inventory truth at its correction commit.

## Session-5b delta (2026-10-26 — the never-dispositioned MOM/product-capabilities leg)

Mandate: find what ALL prior sessions missed, verify landed fixes, go deeper on
og-vs-core, mark remaining surfaces as findings. Method: coverage-gap map (corpus
inventory 11,495 files vs every citation in diff-to-fix/+worklog+AUDIT docs)
exposed the entire `ghidra-mo`/`ghidra-partition` `.mo` family and the
ghidra-partition remainders as never-audited; four parallel read agents (A:
boot-repair/partition-write/OS-aware; B: disk-image/device layer; C: FS
analysers/check phase/virtual views; D: persistence/logging/LDM-write/FATX) +
lead verification of every load-bearing citation. **ID coordination:** this leg's
13 findings are renumbered past the storage/census parallel leg's
S-024..S-028/M-031..M-039/N-045..N-047/I-017/I-018 (no content overlap — pure ID
deconfliction, append-only convention).

**Counts after session 5b (direct file inventory at the rebase head on
`c75f646`):** ACUTE 0 · **SEVERE 9** (S-007, S-009-partial, S-023, S-024,
S-025, S-027, S-028, S-029, S-030) · **MAJOR 38** · **MINOR 31** ·
**INFO 18** = **96 active** / **49 FIXED** (findings/FIXED/ by file inventory
minus RESOLVED-INDEX; includes the fix legs' S-012/S-020/S-021/S-003/S-022/
S-026/M-022/M-023/N-037 landings visible at rebase time).

**The session's headline discoveries (all first-time findings; renumbered IDs):**

- **S-029 (SEVERE)** — the lost-partition APPLY path: the og writes the
  recovered partition into the table (RecoveryModule `CreateRecoveryPartitionObject`
  → PartitionRecovery.dll IOCTL chain 0x7C100/0x7C010/0x7C058 + GPT GUID
  regeneration); the core is contractually read-only (cr_lostpart.h:91-92). The
  endgame of lost-partition recovery is absent — scope decision forced
  (consent-gated apply vs documented exclusion).
- **S-030 (SEVERE)** — disk-image scan targets: the og scans
  .vmdk/.vhd/.vhdx/.edisk/.img/.dmg as first-class devices (DRWUI
  CImageFileDevice extension factory + Device.mo/libvmdk with extent/parent-chain
  support); the core accepts only --disk/--volume. A whole input class missing.
- **M-040 (MAJOR)** — the boot-chain repair family: Fixup.mo binary-patches BCD
  hive cells (newLBA<<9 at offset fields, dual-store Boot\BCD + ESP store,
  ZwUnloadKey force-unlock, DASD flush), EFIBoot.mo manipulates UEFI NVRAM
  Boot####/BootOrder (GUID 8BE4DF61), boot.ini ARC rewrite, post-op
  `ChkDsk.exe %C: /F /X` + Bcd_RestoreBcdFile undo — wired into the
  RecoveryModule/MainModule apply/undo chains; core has zero of it and no
  disposition.
- **M-041 (MAJOR)** — the damage-audit/check phase: CCheckFAT (pairwise
  FAT-copy compare 0x1005, entry-hole + duplicate-reference scans, dir-tree
  audit incl. cycles/cross-links/size≠chain, orphan sweep) and CCheckNTFSVolume
  ($MFT/$MFTMirr proofs, version gate {1.2,3.0,3.1}, $Bitmap structural,
  MFT×$Bitmap cross-check), error taxonomy 0x1001-0x100e, WinChkdsk delegation.
  Core has only repair-oriented rawfixer checks.
- **M-042 (MAJOR)** — lostpart.dat resume semantics: the og re-validates each
  persisted candidate against the disk and drops stale records (with a
  partinfo.dat whole-disk layout snapshot); the core's session restore is
  blind. Full 0x448-byte record layout decoded and cross-validated against the
  shipped .dat files.
- **M-043 (MAJOR)** — NTFS recognition: og requires MFT $Volume content proof +
  verifies the volume-end backup boot; the core's scanner never reads the NTFS
  backup DBR (torn-primary volumes require --repair).
- **M-044 (MAJOR)** — UDF recognition family missing (og analyser result 0xc;
  core has no UDF branch in probe_family).
- **M-045 (MAJOR)** — FAT boot-gate family: six concrete gate deltas (bps
  any-512-multiple vs pow2-512..4096; SPC ≤128 vs unbounded; "FAT32   "@+0x52
  label; numFATs≠0; cluster ≥0xFFFFFF0 cap; band-ordered flavor retry).
  Resolves S-006's FATX scoped remainder: **CFatxBlockParser is the
  FAT12/16/32 parser — no Xbox FATX exists in the og** (definitive negative
  grep; corpus-errata row 41).
- **N-048/N-049/N-050 (MINOR)** — engine logging seam absent (3 og layers);
  persisted lost records thinner (no bps/SPC/evidence); recognition contract
  boot-only (no content proof before engine dispatch).
- **I-019/I-020 (INFO)** — EPM tool ops registered as scope exclusion
  (EditLdm2*, Ntfs2FATProc conversion, partition merge/resize, FatFormat,
  Burn — no recovery-flow consumers); corpus-doc corrections register
  (CTbFile is a boot.ini wrapper not an image abstraction — the real image
  layer is DRWUI+libvmdk; CFatVirtualView is conversion not recovery;
  EFIBoot is NVRAM not ESP; Fixup is link-time not MOM; the grading table is
  .data/unsorted).

**Resolutions this session:** OQ-S4-1 (the 169-entry grading extension table
dumped — `.data` 0x141bb0500, 9-byte stride, first "Raw", last "COMMON";
S-023's fix-plan step 3 input complete); OQ-S4-2 (the RAW naming hook =
single-site "filter_generalraw" category display-name mapping via DRWUI's
firebase.json — no per-file filter; M-029 §Gap (b) corrected); S-006's FATX
remainder dissolved; S-019's fix verified in-source; two stale duplicate
matrix rows consolidated; two accidental finding-file duplicates caught and
fixed in-session.

**Top-risk update (post session 5b):** #1 S-023 (grading — NOW UNBLOCKED:
the extension table is dumped), #2 S-024 (health grading — the storage/census
leg's; same engine lane), #3 **S-029 (apply-path scope decision — blocks
M-040)**, #4 S-020's family remainder (S-025/S-027/S-028 — ext4/Btrfs
lanes claimed by the fix legs), #5 **S-030 (image scan targets — also
unblocks og-vs-core executed comparisons on identical image fixtures)**,
#6 **M-041 (check phase — error taxonomy test-scoped)**, #7 M-028 (preview
caps, executed proof), #8 S-007 (carve formats — lane claimed), then the
M-042/M-043/M-045 family and the prior backlog order below.



# SUMMARY.md — executive summary (session-5 audit leg on top of the closing/fix sessions; earlier sessions below)

## Session-5 audit delta (2026-10-26 — the citation-gap / remaining-surface READ-ONLY session; full detail in the Session-5 delta section below)

The audit leg's full delta (24 findings: A-005 ACUTE rawfixer 4Kn stack-buffer
overflow; S-031 carved-deleted; S-032 archive inventories; M-046..M-051;
N-051..N-064; I-021/I-022; OQ-S4-1 RESOLVED convergently with the fix leg —
structure+consumption decode here, the extension-list dump there, both folded
into S-023; OQ-S4-2 RESOLVED; corpus errata 37-40; suite 366/366 evidence at
`7cf6754`) is recorded verbatim in the "Session-5 delta" section below — keep
reading. **Counts after session-5 merge (direct file inventory at the merged tree):
1 ACUTE (A-005) · 10 SEVERE · 43 MAJOR · 44 MINOR · 20 INFO = 118 active ·
54 FIXED-family files (N-055 fixed by the parallel S-024 leg) (incl. RESOLVED-INDEX + the OQ-T1 record). The session-5
leg filed 25 findings (A-005; S-031/S-032; M-046..M-051; N-051..N-064;
I-021/I-022). The parallel fix legs' own counts sections below are historical
snapshots, superseded by this line.**

## diff-to-fix closing session delta (2026-10-26 — the "fix ALL tiers" session)

Session start `b65f1e6` (baseline 396/396 Linux Release, verified first-hand on
this host). Corpus `03ad46c` (unmoved). Mandate: close every finding — verify
each independently against the corpus AND the running code before any change;
fix with tests that fail before and pass after; keep the audit trail exact.

**S-026 FIXED (this push):** ext4 meta_bg group-descriptor resolution. The
verification went beyond the decompile — two REAL `mke2fs -O meta_bg` images
were built with e2fsprogs 1.47.2 (1KiB/64B-desc and 4KiB/64B-desc) and the
layout pinned empirically (dumpe2fs + hexdump byte-matching). Implemented the
kernel/e2fsprogs-correct arithmetic (`meta_group_size = block_size/desc_size`;
descriptor block at `first_data_block + m*meta_size*bpg`, +1 only for m==0;
`s_first_meta_bg` classic front region; lazy bounded per-meta-group cache).
The og's stride (`s_desc_size`) and missing-`first_data_block` defects are
deliberately NOT adopted — registered as **PARITY-DELTAS D-58**; OQ-E2
RESOLVED by the same evidence. 5 tests (4 fail-before proven on the
engine-reverted build); suite **401/401** Linux Release; ASan+UBSan fuzz
smoke clean over structured meta_bg seeds.

**S-027 FIXED (this push):** ext4 backup-superblock rescue (the og's
engine-side lost-volume walk, FUN_180001b00 slot 53 — re-verified first-hand
incl. the vtable .rdata reference). Implemented inside scan_ext4: canonical-
position probe grid (1 MiB, both +0/+1024 families), exact volume-base
recovery from the backup's own s_block_group_nr (verified on real images:
group-1 copy = 1), GDT ladder front-first then backup-GDT-after-SB (full
table classic / meta-group-0 for meta_bg, verified). Og boot-area rebase
NOT adopted → **PARITY-DELTAS D-59**. 4 tests (3 fail-before); suite
434/434 serial; ASan/UBSan fuzz smoke clean.

**S-025 FIXED (this push):** ext4 journal reparenting (the og's two-pass
model + orphan parent map + iNode%I64d fallback — FUN_180003de0 :2773/:2778,
FUN_1800043f0 :3077-3145, FUN_1800041e0 :2986-2991, all re-verified first-hand).
One-structural-pass harvest + topological pre-pass with full inode truth +
live anchors; claim checks via live_nodes (files too — latent duplicate gap
closed); sweep skip-set for journal-emitted inodes. 3 tests, all fail-before;
suite 465/465; ASan/UBSan fuzz smoke clean. The ext4 SEVERE family
(S-025/S-026/S-027) is now fully closed by this session.

**Counts after S-025 (direct file inventory):** ACUTE 0 · **SEVERE 7** ·
MAJOR 36 · MINOR 31 · INFO 18 = **92 active** / **53 FIXED** (counts
re-baselined against the parallel legs' concurrent landings: S-003/S-021
HFS+ journal pair, S-020 ReFS attribute layer, M-022/M-023/M-026/M-027,
the minors wave, INFO tier ACKNOWLEDGED).



## Close-all-tiers session delta (2026-10-26 — the "fix every finding" session)

Session start `3278a22` (bootstrap on `bf7df39`; suite 396/396 baseline). Corpus
`03ad46c` (unmoved). Mandate: verify every finding independently against the
corpus before fixing; resolve ALL tiers; resolved findings move to `findings/FIXED/`
with Resolution sections; matrix/modules/counts updated as fixes land.

- **M-023 FIXED** (category model — the S-023 dependency): Category enum
  +Email(7)/+Bookmarks(8); the 64-entry hand table replaced by the generated
  1,481-entry table (scripts/gen_filefilter.py from the corpus FileFilter.xml;
  first-writer-wins for the 8 cross-listed); binary-search lookup;
  category_mask default 0xFF→0x1FF (Bookmarks bit). Fail-before: behavioral
  probe (camera-RAW → Other pre-fix) + enum-by-construction (Email/Bookmarks
  absent pre-fix). 5 tests; suite 401/401 Release + 401/401 ASan(leaks).
- **OQ-S4-1 RESOLVED** (the 169-entry grading extension table at .rdata
  0x141bb0500 — extracted first-hand from the corpus DRWUI.exe: 169 × 9-byte
  entries {8-char NUL-padded key, flag byte 0}, first key "Raw"; the dump
  feeds S-023's deep-validation gate).
- **S-023 FIXED** (the grading engine — the largest missing capability):
  tree::Grade {Low,Average,High} with the og's exact setter semantics
  (monotonic-max, grade-2 sticky, overlap flag); the post-scan pass
  (cr_grading.cpp) implementing the decoded decision tree (size floor,
  Email-category fast path, existing-file rule, carve-device small-file
  rule, NTFS device-index rule + QuickScanDeleteExtend counter,
  extension-table-gated extent-end twin detection with the FAT-family −2
  address offset); per-selection regrade (grade-1 only); RecoveryAssistant
  statistics (8 counters + 8 category counts + matched/total with the
  small-result heuristic); ABI surface (node grade/overlap, query-row grade,
  sort key 6, cr_ui_scan_assistant, cr_ui_session_regrade); session
  persistence (node bit 3 + REC_ASSISTANT_STATS record 6); the report's
  scan_analysis block (CLI + ABI paths). Registered non-adoptions: the
  UNDECODED NTFS vt+0xb8/vt+0x100 sub-rule (no blind structural guess) and
  the og's internal matched/total composition (matched = grade-2 count,
  documented). 15 unit tests + C-consumer lifecycle; fail-before by
  construction (capability absent). Suite 429/429 Release + 429/429 ASan.
  **In-walk Confidence untouched** (allocation-axis superset).

**Counts after S-023 (direct file inventory):** ACUTE 0 · SEVERE 10 · MAJOR 33 ·
MINOR 37 · INFO 16 = **96 active** / **35 files in FIXED/** (34 findings +
RESOLVED-INDEX).

(Reconciled post-rebase: the parallel legs landed S-003/S-021/S-026, M-022,
N-037..N-040 and more while this session worked — the live counts below are
the file inventory at this commit.)

- **S-024 FIXED** (health grading): HealthGrade (Unknown/Good/Caution/Bad)
  + the pure-core rule set (cr_grade.cpp): 5/197/198 value<thr->Bad /
  raw>=1->Caution; attr-1 read-error rule; the SSD-life 10-attr vendor set
  (is-SSD gated; 0->Bad, <=10->Caution); temp 50/60 by class; NVMe
  critical-warning bit matrix + the life rule. 0xD1 READ_THRESHOLDS issued
  (vendor thresholds). CLI health prints per-attr threshold/grade + the
  disk verdict. 8 tests; suite 460/460 Release. AUDIT-PIPELINE's "failing
  derivation: UNVERIFIED" row -> SUPERSEDED (the 4-status model).

**Live counts (direct file inventory after S-024):** ACUTE 0 · SEVERE 7 ·
MAJOR 38 · MINOR 31 · INFO 18 · tracked-open 12 = **94 active** / **51 files in FIXED/**.

## S-012 verification-and-correction delta (2026-10-26 — diff-fix session 4, the reconciliation leg)

Bootstrap `597a9c6` (350/350 baseline re-run in-container); corpus `03ad46c`
(unmoved). This leg independently implemented S-012 steps 1+2 in parallel
with the landed `5f1a212` leg (local commit discarded during the
pull-frequently reconciliation — the landed leg's size-0 rescue made it the
more complete substrate), then **re-verified the landed implementation
line-by-line against the corpus** and corrected three divergences, each
corpus-proven and fail-before/pinned:

- **Dump-walk stride** — the og strides the RECORD SIZE on every path
  (uVar8 after holes LAB_18001b0cb :21293 and after emit :21151; uVar16 =
  max(0x400, rs) only after props-fail/skip/destroy, coinciding through the
  boot clamp). The landed code's one-sector hole stride examined positions
  the og never visits; its max(sector, 0x400) parse bound truncated 2/4 KiB
  records. Pinned by `GuidDumpStrideIsRecordAligned` +
  `GuidDumpLargeRecordParsedFully` (both fail on the landed tree).
- **Pseudo record#** — corrected to the og's exact `lVar18 =
  local_c0/record_size + chunk byte base` arithmetic (:21150-51).
- **N-040 (new, FIXED)** — the pass-1 attribute filter: the og's gate is
  (og-15 name table) AND ($SI SYSTEM+HIDDEN) — FUN_180040f20's 15-entry
  table decoded first-hand — with NO $FILE_NAME-flags system filter; the
  old `dos_flags & 4` gate dropped pagefile.sys-class user files the og
  keeps. Gate + flag + counter removed; pinned by
  `MainWalkKeepsSystemFlaggedUserFile`. In `findings/FIXED/`.
- D-51 corrected in place; the D-50/D-51 numbering collision (section-K
  bullets vs headings, plus a duplicated D-51 FAT heading) registered as
  errata with the FAT entry renumbered D-56. Suite 399/399 Linux ASan;
  hostile ASan smoke 300 mutated fixtures (297 exercising pass 2, clean).
## Session-6 delta (2026-10-26 — the deep-continuation audit: never-cited surfaces + fix verification)

Session start `022c81d`-lineage (landing `bf7df39` mid-bootstrap); corpus `03ad46c` (unmoved).
Mandate (user directive): find what earlier sessions missed, verify/update the fixed things,
go deeper on og-vs-core, mark ALL remaining features/implementations/functions as findings,
focus on minor detail. The session-5 citation-gap lanes (defined at `2001f7c`, never executed)
were this session's scope — executed end-to-end via 5 parallel read agents + first-hand
verification of every load-bearing citation before filing (three stale agent claims filtered
by verification: FAT alt-EOC [N-002 fix live], NTFS BAAD [MATCH both sides], FAT quick-scan
threading [registered D-30x]).

**Multi-leg ID reconciliation (the parallel audit legs landed S-029..S-032, M-040..M-051,
N-048..N-064, I-019..I-022 in the same window):** this session's local M-041/M-042/N-050
CONVERGED with the landed S-031/S-032/M-051+N-059 (same census artifacts, independently
decoded) — dropped with harvest-addenda on the landed findings; the 14 unique findings
RENAMED past the taken IDs (M-052, N-065..N-075, I-023/I-024). The parallel fix legs also
landed fixes this window (S-007 primary set incl. the MPEG sync family, S-020/S-023/S-024,
S-025..S-028, M-010, M-022..M-027, N-005/N-011/N-012/N-024/N-028/N-038/N-039 …) — this
session re-verified the overlaps first-hand and posted post-resolution corrections where
its addenda were authored pre-fix (S-007: bare-MPEG FIXED — registry now carries
FMT_MPGA_E2..FF + ADTS with validate_mp3_raw; S-024: 0xD1 now issued at cr_health.cpp:192-200,
only the 0xD8 claim remains stale).

**First-time-cited corpus artifacts (23):** the 6 rawsearchdll carving-family artifacts +
analyser-registry census + dual-registry u16 decode + zip-analyser carving; rsf-record4
grammar + ifileproperties-part3; per-engine-function-map; MOM Error/Thread/Common/Config/
PartitionRecoveryD + mom-module-system; WinPE boot-config + eusendmessage; config-files
drwcfg/userdata-remoteconfig/vendor-logs; UIUX 13/19/20/24/25. **First-time-cited core files
(8):** cr_elevate, cr_destio, cr_snapshot, cr_cached, cr_decomp, cr_sha256, cr_error,
cr_diskmap (header).

**This session's surviving new findings (14 unique + 3 convergent harvests):**
- **MAJOR (1):** M-052 (FAT/exFAT recycle-bin $I/$R pairing missing — og IRecycleBinNode2UI;
  core grep 0; the NTFS-side model exists in-repo to generalize).
- **MINOR (11):** N-065 (writer collision `name_N`/cap 10000 + mkdir 0/0xB7 gates — refutes
  D-19's "unevidenced" with decompile evidence), N-066 (identity card weaker: firmware/WWN/
  interface + 512-byte boot snapshot vs CRC32), N-067 (exFAT upcase validation skipped),
  N-068 (ZIP EOCD anchor + LFH sanity gates — re-verified absent post-S-007-fix), N-069
  (CAB mscf case key — re-verified absent), N-070 (overlap scoring + two-generation
  fallback), N-071 (enumeration breadth), N-072 (Btrfs re-scan cache + ZFS label polling),
  N-073 (ext4 catch-buffer shifted-window salvage), N-074 (doc-drift group #2 — five stale
  claims incl. the refuted D-19 and the 0xD8 residual), N-075 (event-bus query-signals +
  pull-progress + typed IUIProgressInfo).
- **INFO (2):** I-023 (DRWCfg persistent-config scope register), I-024 (PartitionRecoveryD
  generator-sets + .dat persistence register).
- **Convergent harvests (addenda on landed findings):** S-031 (session round-trip mechanics
  + timestamp shape + adjacent-fields pointer), S-032 (results-tree consumer cite + size-
  from-sum + hostile-cap + nested OQ), N-059 (HFS+ mode/type half + cross-engine ID space).

**Addenda to active findings (9):** S-007 (family layer — roster with per-family provenance
+ post-resolution correction), S-009 (stale premise corrected + free-overlay residual with
concrete fix plan), S-024 (doc-truth sub-item + post-resolution correction), M-017
(CalcPartition class evidence), M-030 (WinPE driver selection: eprdtdrv absent from PE,
ebrntdrv/epmdkdrv via NtLoadDriver, win7older==ebrntdrv MD5), M-029 + N-033 (partial fixes
verified in-tree), N-045 (cache algorithm divergence + cite refresh). **New tracked-open
(3):** OQ-S6-1 (PartitionRecoveryD generator-set semantics), OQ-S6-2 (Btrfs RC4 target),
OQ-S6-3 (CRAW +0xF4/anchor-B/checksum).

**Counts after the session-6 reconciliation (direct file inventory, refreshed through the
last mid-session landings):** ACUTE 0 (A-005/A-006 both fixed in-window) · SEVERE 5
(S-009 residual, S-029, S-030, S-031, S-032 — S-031's deleted-flag half fixed and
verified mid-session; the timestamp residual stays open) · MAJOR 40 · MINOR 55 · INFO 22
= **122 active / 68 FIXED**; 16 tracked-open files. (The parallel legs' fix wave moved
34 findings to FIXED in the same window — S-007, S-020..S-028, S-031, M-010, M-022..M-027,
N-005, N-011, N-012, N-024, N-028, N-038, N-039, A-005, A-006 among them — while their
audit legs filed A-006, S-029..S-032, M-043..M-051, N-048..N-064, I-021/I-022; this
session's addenda re-verified the overlapping fixes first-hand where its citations
touched them.)

**Recommended first-10 from this session's crop (for the fix legs):**
1. M-052 (recycle pairing — generalize the NTFS model; 3 fail-before tests specified)
2. S-031's two-site emission fix (this session's round-trip mechanics make the reload
   consequence precise — S-031 is the landed canonical finding)
3. N-074 (five doc fixes + the 0xD8 residual — an hour of work, consumer trust)
4. N-068 (ZIP EOCD anchor + LFH gates — two small validators; re-verified absent)
5. N-065 (writer collision/mkdir alignment + D-19 rewrite — decompile-evidenced)
6. N-070 (overlap scoring — collect-then-max instead of break)
7. S-032's inventory payload (CarveResult extension; this session's hostile-cap +
   size-from-sum additions fold into its fix plan)
8. N-067 (exFAT upcase validation — grade-not-reject)
9. N-075 (busy/stopping query exports or the deliberate-difference registration)
10. N-073 (ext4 catch-buffer backtrack — fault-injection test specified)

---



## Minors-fix session delta (2026-10-26 — the "fix all tiers, focus the 33 minors" session)

Session start `ed00020` (baseline 345/345 Linux Release; +1 test from the concurrent
transport wave `a4053c4` → 346). Corpus `03ad46c` (unmoved). Mandate: verify every
finding independently against the corpus before fixing; focus the MINOR tier.

**Batch 1 — FAT/exFAT/lost-partition minors (this push):**
- **N-022 FIXED** (mirror consult on primary FREE only by default; BAD-salvage opt-in;
  fail-before proven on the pre-fix worktree).
- **N-023 group FIXED** (N-010a flavor boundary 65524; N-010b engine-open gates;
  N-010c date sanity behind `strict_date_sanity`; N-010e exFAT byte-OR reserved form;
  N-010d FIXED-ALREADY via S-011; N-010f registered D-51). **SPC sub-claim corrected
  as a false positive** (u8 field domain — audit correction, no code break).
- **N-034 FIXED** (og deep-scan ladder `fat::lost_boot_flavor` + `probe_fat` rewiring;
  engine-open layering pinned; SPC sub-claim correction shared with N-010b).
- **N-001 duplicate reconciled** (stale copy in MINOR/ removed; FIXED/N-001 canonical).
- PARITY-DELTAS **D-51** added (5 registered deltas: FAT12 engine-open exemption,
  date-gate default OFF, mirror-BAD salvage opt-in, mid-chain FREE/BAD shape,
  underflow guard).
- Suite **366/366** Linux Release (346 + 20 new tests; 6 fail-before discriminators
  proven against the pre-fix source in a worktree).

**Counts after batch 1 (direct file inventory):** ACUTE 0 · SEVERE 8 · MAJOR 22 ·
**MINOR 29** · INFO 12 = **71 active** / **29 FIXED** (FIXED/ holds 29 findings +
RESOLVED-INDEX).

**Batch 2 — NTFS minors (this push):**
- **N-011 FIXED-ALREADY** (emit_ads flag landed with `367561d`; missing parity
  pin + superset guard added this session).
- **N-012 FIXED** ($SI attribute source + 0x7FF mask; 2 fail-before tests).
- **N-024 group FIXED/REGISTERED** (short_name field + session persistence;
  boot ladder = LoadNtfsVolume exactly — 5 fail-before/rescoped tests; stride
  quirk + namespace ranking + error-facility mapping registered; D-52).
- **N-039 FIXED** (0x20000 gate kept — the og's 0x200 mask registered as an og
  defect; >64 MiB $J tail mining; fixture comment corrected; 4 tests).
- Cross-lane green-main fix: the S-012 leg's ASan leak (caller-held
  FileProperties ref never released at the GUID-dump emit) — suite now
  384/384 under ASan detect_leaks=1 too.
- Suite **384/384** Linux Release (+18 tests since session start).
- NOTE: parallel audit legs landed concurrently (S-023..S-028, M-028..M-030+,
  N-040..N-047, I-015..I-018 inventory; their own fixes). The minors-focus
  session continues against the live inventory.

**Batch 3 — HFS+ minors (this push):**
- **N-028 group FIXED/REGISTERED** (sink filter both sources; 'prdf'
  folderization; rsrc superset registered D-53).
- **N-038 FIXED** (ownerFlags 0x20 → rsrc fork + decmpfs completion).
- **N-005 FIXED** (block bound 16 MiB + product sanity; floor registered).
- Suite **392/392** Linux Release (+8 tests; 3 fail-before discriminators
  proven against the pre-batch source).



**Audited commit:** session 1 `67193b6` → session 2 `5d6da5f`/`8574039`/`5f638a3` →
session 3 `2ce2e6b` + in-flight fixes through `34b3441`+rebase → session-3 continuation
through `43149ff` (S3 lead) → fix legs through `ed00020` → session 4 `63a9167`-lineage
rebased to `204137b` on `ed00020` → **session 5 (this leg): `d870dd3`-close + the S-019
landing at `7cf6754`. Corpus:** `03ad46c` (unmoved).
**Audit dates:** 2026-10-25 (sessions 1-3) · 2026-10-26 (session-3 continuation legs +
session 4 + session 5).
**Status:** session 5 = the never-cited-artifact + coverage-hole-closing leg: a systematic
citation-gap sweep (83 cited corpus paths vs the full artifact tree) drove a reading wave
over every never-cited artifact (6 RAWSearchDLL family files, rsf-record4,
ifileproperties-part3, 8 MOM censuses, 9 UIUX files, WinPE pair, config-files) and every
never-cited core file (cr_elevate, cr_destio, cr_snapshot, cr_cached, cr_decomp, cr_err,
cr_diskmap); 24 new findings including the first ACUTE since A-004; OQ-S4-1 RESOLVED (the
grading table dumped byte-exact); OQ-S4-2 RESOLVED-with-residual; corpus errata rows
37-40; S-019's landing verified. Suite 366/366 at the audited tree (evidence:
`evidence/executions/s5-baseline-suite-green.md`).

## Session-5 delta (2026-10-26 — the citation-gap / remaining-surface session)

Mandate: find what ALL prior sessions missed (by systematic inventory, not sampling),
verify the fixed things, go deeper on og-vs-core, mark every remaining feature/function.

- **The gap analysis (new method this session):** extracted every corpus path cited
  across findings+modules (83 unique) and diffed against the artifact tree; same method
  over core src/include basenames. Never-cited in-scope surfaces found and READ IN FULL:
  the 6 RAWSearchDLL family artifacts + per-engine-function-map; SaveScan rsf-record4
  (tree-node grammar) + ifileproperties-part3 (btrfs/zfs/RAW/generic); MOM
  Error/Thread/Common/Config/CheckVersion/Log/bitlockerhelp/FileSystemCheck; UIUX
  31/13/19/06/24/25/20/33/34; WinPE eusendmessage + boot-config; config-files
  drwcfg-bin/userdata-remoteconfig/vendor-logs; core cr_elevate/cr_destio/cr_snapshot/
  cr_cached/cr_decomp/cr_err/cr_diskmap; og-codebase data files (EUUserData, Dict,
  DataFile.ini dispositions).
- **A-005 (ACUTE — first since A-004):** rawfixer 4Kn stack-buffer overflow — the FIX-3
  4Kn offset fix changed read LENGTHS to `sector` but left the 512-byte stack buffers;
  3,584-byte overflow per read on 4Kn devices; CI blind (tests pin 512). Co-located
  M-046: exFAT 512-byte repair plans + LDM GPT-path `*512` hardcodes (4Kn wrong offsets).
- **S-031 (SEVERE):** carved files never marked deleted — og FLAGS=0x0020 constant on
  all carved records; the core's `deleted_only` query and `deleted_count` exclude the
  ENTIRE carve result set (cr_query.cpp:114 / cr_carve.cpp:174-182).
- **S-032 (SEVERE):** archive inner-entry inventories absent — the og returns the
  complete ZIP entry list (presented as archive contents) and stitches multi-part RARs;
  the core emits one flat node per archive.
- **MAJOR wave (6):** M-046 (4Kn LDM/exFAT plans), M-047 (session stats written-then-
  discarded + direct-only refresh vs the og's trusted recursive image), M-048 (EUUserData
  user-signature extensibility — 111 extensions, no counterpart), M-049 (error model:
  false "reproduces the reference convention" claim; og 12-digit documented codes +
  error objects), M-050 (~15 unpersisted og node fields incl. grading bits + hidden
  roots), M-051 (RAW provenance fields: timestamp/checksum/note/anchors).
- **MINOR wave (14):** N-051 MKV DocType, N-052 HEIC relabel, N-053 IIU binding,
  N-054 RAF/JPEG strictness (both directions), N-055 SMART 0xD1 thresholds (health
  disposition CORRECTED — data-layer gap, not presentation), N-056 WinDestIo temp leak,
  N-057 TableLayout::Ebr dead + diskmap test debt, N-058 session load validation deltas,
  N-059 btrfs/zfs session extras, N-060 save progress/cancel, N-061 run-log + session
  auto-naming, N-062 memory monitor, N-063 carve naming invented (OQ-S4-2 resolution +
  a corpus-artifact correction), N-064 Dict password-dictionary unlock aid.
- **INFO registers:** I-021 (og always-elevated manifests — new binary evidence;
  EuSendMessage installer-only; repair toggle; CRecoveryFilter layering; PENetwork SMB;
  DRWCfg out-of-scope), I-022 (core supersets: CRC32/resync/bomb-caps, structured btrfs
  extents, decoded zfs runlists, extra props).
- **OQ resolutions:** OQ-S4-1 RESOLVED (169-entry grading table dumped byte-exact from
  DRWUI.exe .data 0x141bb0500 — char[9] strided, last-dot/uppercase/exact-find
  consumption, `""` + dead-`Raw` quirks; S-023's fix plan step 3 now actionable).
  OQ-S4-2 RESOLVED-with-residual (the +0x48 hook = one-shot per-scan
  "filter_generalraw" config fetch; NO og FileNNNN scheme — corpus deep-dive artifact's
  contrary claim filed as errata row 37; residual = DRWUI merge-rename decode).
- **Fix-verification:** S-019's landing (7cf6754) verified in-tree with its audit trail
  (RESOLVED-INDEX row 27, matrix, module, 3 fail-before tests).
- **Corpus errata rows 37-40:** the File0001 naming claim, the family-roster count
  conflicts (census authoritative), the .rdata→.data grading-table correction, the
  MOM-censuses-are-structural meta-note.
- **Counts at session-5 close (direct file inventory):** **110 active** (1 ACUTE:
  A-005 · 10 SEVERE: S-003, S-007, S-009-partial, S-012, S-020, S-021, S-022, S-023,
  S-031, S-032 · 31 MAJOR: M-002, M-005, M-007, M-008, M-010..M-051 minus 6 in FIXED/ ·
  52 MINOR: N-001..N-064 minus the FIXED/INVALID ones · 16 INFO: I-001..I-022 minus
  I-004/I-005 in FIXED/) · **27 FIXED** (latest: S-019) · 8 tracked-open (2 resolved
  this session: OQ-S4-1, OQ-S4-2-resolved).

**Top-risk update (session 5):** #1 **A-005** (ACUTE — memory corruption on 4Kn; smallest
verified fix: resize 4 buffers + sector-correct plans, one review unit with M-046), then
S-031 (deleted-view excludes all carved files — one-line producer fix + tests), S-032
(archive inventories), S-023 (grading — now unblocked by the table dump), S-020 (ReFS
attribute layer), S-012 (NTFS pass-2), M-047/M-050 (session schema wave — reserve the
grading bits while landing M-047), then M-048 (user signatures), M-049 (error model),
and the M-011..M-015 family.
**Status:** session 4 = the never-audited-modules leg: preview, query, transport audited
for the first time; the bootstrapped-but-never-executed API-session lane executed
(factory contract, driver-client pattern, measured suite/ABI verification); grading
engine decoded end-to-end. 11 new findings; OQ-S2M-1 resolved; M-022 OQ-1/OQ-3
resolved; M-023 counts corrected; corpus errata rows 29-36 filed. Suite 306/306 +
C-ABI check OK at this leg's audited tree (evidence: `evidence/executions/s4-suite-green-and-abi-check.md`).

## Session-3 storage/census parallel-leg integration (2026-10-26)

A second session-3 parallel leg (the *storage/census* leg — raw_signs consumer
decode, OQ-T8 dispatch, lost-partition scoring, ReFS first-class,
CPartitionVirtualVolumeDevice, FileFilter+health, ext4/HFS+/Btrfs/ZFS census)
rebased onto the reconciled main state after the S3-lead/S4 legs landed. Its
**42 locally-filed findings were reconciled against the lead legs' filings**:

- **19 kept, renumbered** past the taken IDs (S-024..S-028, M-031..M-039,
  N-045..N-047, I-017/I-018) — the genuinely-new surfaces: **health grading
  (S-024)**, **ext4 journal reparenting / meta_bg / backup-SB scan /
  dir-parse resilience (S-025..S-027, M-037)**, **Btrfs xattrs (S-028)**,
  **ReFS per-file progress (M-031)**, **the storage/slice layer (M-032
  tail-read semantics, M-033 BitLocker in-stack decrypt, M-034 NAS, N-045/N-046,
  I-017 — first audit of that layer; new `modules/storage.md`, complementing
  `modules/transport.md`)**, **engine-side FileFilter validation (M-035)**,
  **NVMe field coverage (M-036)**, **ZFS SA registry (M-038)**, **cross-engine
  occupancy bitmap (M-039)**, **NVMe route MATCH record (I-018)**.
- **23 dropped as superseded** by the lead legs' filings (which landed first):
  local A-004→S-020; S-012→N-036; S-013→I-012; S-014/S-015/M-014/M-015/I-011→
  M-017 (independent numeric corroboration added as an addendum); S-016→M-023;
  S-022→S-015 (FIXED); M-011/M-013→M-022; M-022→M-020; M-025→N-030 (tail-formula
  + 2 MiB cap + 500 MiB default folded in as an addendum); N-023/N-024→N-026/
  N-037/I-013; N-027/N-028→N-020/N-040/M-029; I-009→M-026 (cross-ref added);
  I-010/I-016→M-017/N-031 (OQ-T9/T8 both resolved by the lead legs);
  I-014/I-015→I-014.
- **New evidence:** `s3-virtual-volume-device.md`, `s3-filefilter-health.md`,
  `s3-rawsigns-consumer-decode.md` (the CommonRawAnalyser loader/matcher/tail
  decode). New tracked-open files: OQ-RS (8 ReFS probes), OQ-Z-E-FLT (ZFS
  dsl-offsets **ACUTE-gated probe**, meta_bg stride, orphan reachability,
  FileFilter blast radius). OQ-S4-1 gains the scorer wiring answer.
- **Convergence highlights:** both S3 legs independently decoded the
  dSuggestted scorer (fits to 3e-5 vs 6 decimals on different log rounds) and
  the dead raw_signs confidence field — strong corroboration for the fix lanes.
- **Baseline note:** this leg's findings were authored against `7fd00e0` (the
  pre-S4 tree); the integration verified each against the reconciled main state
  (through `d374064`). A-002 was live at this leg's baseline and is FIXED on
  main (`7f2c83e`) — this leg's re-verification at `7fd00e0` is retained as
  historical evidence in its worklog entry.

## Counts per severity (active — excludes FIXED/) — post-integration (file inventory)

| Severity | Active | FIXED | Notes |
|---|---|---|---|
| ACUTE | 0 | A-001(p), A-002, A-003, A-004+S-016 | — |
| SEVERE | 13 | S-001/2/4/5/6/8/10/11/13/14/15/17(+18 invalid) | +S-024..S-028 (this integration) |
| MAJOR | 34 | M-001, M-006(in S-009), M-009 | +M-031..M-039 (this integration) |
| MINOR | 41 | N-002/3/10/21 | +N-045..N-047 (this integration) |
| INFORMATIONAL | 16 | — | +I-017/I-018 (this integration) |
| tracked-open | 11 files | OQ-T1/2/5/7/8/9 + S2 OQs resolved | +OQ-RS, OQ-Z-E-FLT (this integration) |
| **Total active** | **104** | **27** | by direct file inventory at `d374064`+this commit |

(The previous section's "61 active / 23 FIXED" reflected the S4 wave-1 close
before the S4 continuation and this integration; the inventory above is the
authoritative post-integration count. **S-023's grading engine + this leg's
S-024 health grading are natural fix-lane companions.**)

## Session-4 delta (2026-10-26 — the never-audited-modules + API-lane session)

Mandate: find what ALL prior sessions missed, remove/verify the fixed things, go
deeper on the og-codebase vs core comparison, mark remaining surfaces as findings.
State at session-4 wave-1 close (direct file inventory, `204137b`):

- **First-time module audits (zero prior coverage — matrix had no rows):**
  - **preview** → M-028 (MAJOR — Caps.max_read bypass on the zip/docx path with
    EXECUTED proof: 65,568 device bytes pulled against a 4,096-byte cap,
    `truncated=false`; plus the "4 MiB corpus preview window"/IInWindowStream
    provenance misattributions — the og's preview cap is 1 MiB and IInWindowStream
    is the 7-Zip LZ interface; compressed extents read whole psize) and I-015
    (INFORMATIONAL — the og's preview is a UI-layer socket protocol: 602-byte header,
    ≤1 MiB BMP, 10 s wait, 100 MB eligibility; the core's in-process reader is the
    deliberate equivalent).
  - **query** → M-029 (MAJOR — beyond M-023's category gap: sort keys (og 6 columns
    incl. Type; core 5, no Type), paging (og virtualized, no page-size constant; core
    offset/limit default 100 — parity-unclaimable, must be recorded deliberate),
    Date/Size bucket IDs (0x100..0x105 / 0x1000..0x1006 incl. Custom + skipped IDs),
    7-group AdvanceFilterCondition, two filter-bar modes, SERVER-SIDE filter push,
    FilterFolder/TxtViewFiles noise filters, 34 RawConfig carve toggles; plus the
    FROZEN `tree --json` contract's `modified` unit lie — docs say Unix seconds, wire
    is FILETIME 100-ns-since-1601, verified in the FAT engine store path).
  - **transport** → M-030 (MAJOR — driver-utilization pattern: no WinPE environment
    gate, no ERROR_INVALID_HANDLE reopen ladder, no terminal IOCTL_VOLUME_OFFLINE in
    the quiesce chain, 32-disk enumeration cap vs og's 200, silent RDA-open fallback
    at the ABI level, and the 64 KiB envelope provenance correction — the og device
    layer chunks at 4 MiB disk / 7 MiB volume buffered-synchronous, NO 64 KiB rule;
    M-009/GAP-S6-13's corpus-grounding narrative corrected to Windows-heuristic) and
    N-041 (dead RdaClient copy-session protocol + volume_letter_to_disk, zero callers).
- **The bootstrapped-but-never-executed API-session lane, executed:** S-023 (SEVERE —
  no post-scan grading engine: the og's CRecoverability "Recovery Chances" model —
  grades {0,1,2} in bits 5-7 of node+0x73, monotonic sticky setter, decision tree
  decoded rule-by-rule, per-selection re-grade, small-result heuristic — has NO core
  counterpart; the core's 5-tier in-walk Confidence is a different decision system;
  RecoveryAssistant statistics (8 counters + 6 category counts) absent; **resolves
  tracked-open OQ-S2M-1**: engines emit grade=1 only, all verdicts are DRWUI-side),
  I-016 (INFORMATIONAL — engine factory contract 9/9 verified: EUCreate pairs, ReFS
  3-arg arity exception, Btrfs NULL-stub, RAW's GetRawUserInfoMap, the host vtable
  interface incl. IAuxiliaryBitmap + cancel-poll; FS-code-space translation warning;
  engine-enable gates note).
- **MINOR findings:** N-040 (ABI doc truthfulness bundle: nonexistent SQLite
  session-index sidecar documented as shipped, disk-map "2 free" state, event-thread
  contract contradiction, unrepresentable ABI sub-versions, frozen-contract unit lie),
  N-042 (CLI health temperature fractional digit is the Kelvin remainder —
  mathematically wrong), N-043 (progress liveness watchdog: og advances coarse
  progress every 5 s on silent engines; core freezes honestly), N-044 (CLI `--deep`
  sets whole-disk discovery, not the deep stage — naming hazard vs the og's
  "Deep Scan" toggle).
- **Resolutions/addenda:** OQ-S2M-1 RESOLVED (→ S-023 evidence); OQ-S2M-2 partial
  (NTFS +30/+50 bonus setters found at NTFSSearchDLL :28804/:28944/:28973/:29002 +
  generic adder FUN_180024990; FAT32-20 setter NOT-FOUND; ZFS/Btrfs negative evidence
  re-confirmed); M-022 OQ-1 (FUN_18000ea30 = $I recycle run-list builder decoded —
  convergent with the S3-REFS leg's decode, which also filed S-020) + OQ-3 (ReFS
  quick-search dot-prefilter = memchr(name,'.') gate, dot-less names skipped,
  kind-flag bypass) RESOLVED; M-023 counts corrected (core = 6 real categories +
  None sentinel / 64 extensions vs og 8 / 1,481 distinct). Three new tracked-open
  OQs: S4-1 (169-entry grading extension table dump), S4-2 (RAW naming-hook
  consumption sites), S4-3 (chunking relative perf — Windows bench leg needed).
- **Verification hygiene:** A-004 + S-016 fixes verified first-hand in-source at the
  session-4 tree (`event_fn_` live-sink at every emit site); suite 306/306 Linux +
  `cr_ui_abi_check` OK executed (evidence file); parallel-leg fixes landing in the
  same window were reconciled by rebase (S-013 `d2d9b03`, S-014 `ed00020` verified
  present in-tree by the rebase read; their own legs' verification records stand).
- **Cross-leg coordination:** this leg's IDs renumbered past the parallel S3/S4 legs'
  S-016..S-022 / M-024..M-027 / N-033..N-039 / I-013/I-014 (my S-017→S-023,
  M-024..M-026→M-028..M-030, N-033..N-037→N-040..N-044, I-013/I-014→I-015/I-016);
  corpus errata rows 29-36 filed after the S3-CARVE leg's 22-28; the parallel legs'
  matrix gained a stale duplicate carve row during their own merge — removed with a
  note in the legend.
- **Counts at session-4 wave-1 close (direct file inventory):** **86 active**
  (9 SEVERE: S-003, S-007, S-009-partial, S-012, S-019..S-023 · 25 MAJOR: M-002,
  M-005, M-007, M-008, M-010..M-030 · 38 MINOR: N-001..N-044 minus 5 in FIXED/ ·
  14 INFO: I-001..I-016 minus I-004/I-005 in FIXED/) · **26 FIXED** (latest: S-013,
  S-014, S-015, S-016, S-017-ftyp, S-011, S-006, N-021).

**Top-risk update (post wave-1):** the roadmap's #1 is now **S-023** (grading engine
absent — the og's headline feature surface has no counterpart; the largest single
missing capability found by any session), then S-020 (ReFS attribute layer — S3-REFS),
S-012 (NTFS pass-2), S-019/S-021/S-022 (the S3 audit legs' SEVEREs), M-028 (preview
caps bypass — executed proof, smallest fix), M-029/M-030 (query/transport model
gaps), then the M-011..M-015 family and M-017 port.



## Session-3 delta (2026-10-25 — the "go deeper, remove the fixed, mark the remainder" session)

Mandate: find what sessions 1-2 missed, verify/retire the fixed things, go deeper on
the og-codebase vs core comparison. State at session-3 close:

- **Retired (verified first-hand):** S-010 FIXED by the fix leg (`3e34ed2`+`414f56c`+
  `34b3441`, already in FIXED/; this session reconciled the stale SUMMARY that still
  listed it as top risk #1); M-009 FIXED (`7fd00e0`, `RwDevice::read_at` now routes
  through `aligned_read_at` — verified in-tree, Resolution added).
- **Fixed MID-SESSION (verified first-hand):** A-002 — `7f2c83e` restored the real
  Microsoft/og LDM GUIDs (`…0434cfb3`/`…714a69ad`; this leg re-extracted them from four
  og binaries independently, converging with the third leg's evidence and the 6-source
  external table) and de-self-referenced the tests. The #1 roadmap item is CLOSED.
  **S-011 was FIXED by the fix session after session-3 close** (tz subtraction
  removed; on-disk time carried through the shared FAT-family datetime core
  exactly like FAT; 3 guards, fail-before proven; suite 325/325 Linux ASan).
- **New findings (7):** S-015 (SEVERE — raw_signs.json min/max are KiB in the og
  (`max << 10`, default 0x7d000 KiB) vs bytes in the core: the 139-format raw-sign
  recall window is 1024x too small, 50 KiB vs 50 MiB); M-022 (ReFS file-value grammar:
  og type@key+0 dual-layout {0x178 old / >=0x29 modern, gated on system-dir-count >= 8}
  + $I recycle handler + all-rows iteration vs core 0x80@key+8 first-match-break);
  M-023 (filter model: 8-cat/1,481-ext runtime XML vs 7-cat/64-ext hard-coded;
  Email+Bookmarks missing — filed from the old unnumbered GAP-cats row); N-030
  (raw_signs confidence field is dead data in the og — parser consumes offset+hex only;
  the core invents confidence semantics); N-031 (carve dispatch deltas: og
  u32→u16→linear→sign-DB order decoded, sector-relative stride, all-zero-sector skip,
  only_common config gate); N-032 (health transports: og companion's CSMI/USB-bridge/
  Realtek-NVMe accessors missing in core); I-012 (GAP-ADS RESOLVED: the og does not
  recover ReFS ADS either — .rsf/RTTI/emission evidence; core MATCH, shared limitation,
  mandate clause satisfied by parity).
- **OQ-T8 RESOLVED** (the decode above; the parallel s2-third-leg's convergent evidence
  block had the u16/sweep legs swapped — corrected with disassembly proof, addendum
  appended to their evidence file). OQ-T9 was resolved by the s2-third-leg (M-017,
  187-row-verified ln(count)/ln(max) scoring model — convergent read this session).
- **Cross-leg coordination:** IDs renumbered past the s2-third-leg's S-012..S-014 /
  N-023..N-029 (this leg's raw-signs findings became S-015/N-030/N-031); corpus errata
  19-21 filed (dispatch order inversion, dead confidence field, KiB units).
- **Architectural verification:** CPartitionVirtualVolumeDevice (the og's virtual-volume
  device layer) = the core's IStorageDevice + slice adapters (equivalent; GAP-S6-8's
  absolute_base covers the base semantics).
- **Coverage added:** ReFSSearchDLL file pipeline (first deep pass in any session),
  RAWSearchDLL raw_signs loader/parser/scan-loop/dispatch (first), FileFilter.xml
  (first), diskhealth companion transports (first), exFAT/FATCalcPartition verified
  convergent via M-017's decode.

## Session 2 + parallel-leg combined delta

Session 2's mandate (find what session 1 missed, continue the audit, remove the fixed
things) landed in two parallel legs, reconciled here:
- **Parallel leg (db7324a/1882847/5f638a3):** 17 new findings (S-010, S-011, M-007,
  M-008, M-009, N-011..N-020, N-021, I-006, I-007), FIXED/ split, RESOLVED-INDEX,
  CORPUS-ERRATA (18 candidates), module-file updates, new open questions
  (OQ-S2A-1..4, OQ-S2C-3).
- **This leg (rebased on top):** independently re-verified every landed fix first-hand
  against the corpus decompiles (S-002 `c9e8860`, S-004 `51ac671`, S-008 `e0175b3`,
  N-002/N-003 `fb44073`, N-010 `cb8dafd`, M-006 `e30b84d`, I-001..I-005 `f74a5dd`),
  re-ran the full suite at `8574039` (313/313 Linux ASan), CLOSED all of them, moved
  S-002 to FIXED/ (the parallel leg's summary listed it open — stale), and filed
  **M-010** (S-005's residual invalid-UTF-8 edge: ext4/Btrfs name bytes destroyed vs og
  raw passthrough; ZFS edge verified parity first-hand via FUN_18001c630).
## ⚠️ THIRD-LEG ADDENDUM (LDM deep-pass leg, session 2) — ~~ACUTE A-002 filed~~ **A-002 FIXED (fix session)**

**A-002 RESOLVED (fix session, this file's top-priority item):** the two corrupted LDM
GPT type-GUID constants were restored byte-for-byte to the real Microsoft
`PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` values (= the pre-`1cb6a43`
bytes, independently re-verified this fix session against the og binaries' own compare
operands: LdmManager.mo DAT_0x1800c4960, Partition.mo DAT_0x180036630/0x36640; full
1,467-file corpus scan: fabricated values in 0 files). The four regression tests were
de-self-referenced (fixtures now hard-code the real bytes / winioctl text forms instead
of copying the constants) and three new guards added
(`Ldm.GptDynamicDiskRealCanonicalGuidsParse`, `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText`,
`Ldm.GptDynamicDiskFabricatedGuidsRejected`). Fail-before proven: 6/7 guard tests fail
on the fabricated constants; suite 320/320 Linux ASan post-fix. See
`findings/FIXED/A-002-*.md` Resolution. A-002 is CLOSED; the remaining active
ACUTE is session-3's A-004 (event-range config-sink mis-delivery — see the
session-3 delta below), which the fix session takes up next.

**Session-2 third-leg state at close (2026-10-25):** this leg resolved OQ-T2 (FAT mirror
consult — per-entry zero-fallback, N-022 filed), OQ-T5 (exFAT ChecksumDBR — spec skip-set
{106,107,112}, core MATCH), OQ-T7 (RAID-5 rotation — left-symmetric both sides, I-008),
and bounded OQ-T6 (the og's ZFS engine is vdev-type-blind: zero vdev-type/geometry keys
in the binary; fixed 0x1C000 anchor quote-confirmed; id-only vdev registry — the core's
explicit unsupported_vdevs refusal is the honest superset). OQ-T9's CalcPartition
machinery is located (vtable slot 4, Fat32 direct-field fast path) for a future scoring
decode. **A-002 remains the roadmap's #1 item** (two-line constant restore + de-self-
referencing the two regression tests).


**A-002 (ACUTE, fix-induced regression, highest priority in the whole roadmap):** the
A-001 fix `1cb6a43` replaced the two LDM GPT type-GUIDs — which were **byte-for-byte
the real Microsoft `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID`** — with
fabricated values (`5808c8aa-8e9f-4c87-…`/`af9b60a0-384e-4f7d-…`) that appear in no og
binary, no authoritative source (Microsoft Learn `PARTITION_INFORMATION_GPT`, ntdoc,
MagnumDB, phnt, Linux kernel efi.h, Wikipedia all carry the real values), and no real
disk. **On the current tree every real Windows GPT dynamic disk fails LDM discovery
(ERR_BAD_MAGIC) — zero dynamic-volume recovery.** Session-1's A-001 LDM rows were a
false positive (mixed-endian mis-decode; its executed "proof" fixture carried the
fabricated GUIDs, which the pre-fix core correctly rejected); the fix then made the
wrong fixture pass, and the new regression tests are self-referential (they write the
constants into the fixtures). Executed proof: the suite's own
`GptDynamicDiskCanonicalGuidsParse` body with ONLY the entry GUIDs swapped to the real
Microsoft bytes **fails** on the current tree and **passes** with the pre-`1cb6a43`
constants — `findings/ACUTE/A-002-*.md` + `evidence/executions/s2-ldm-real-guid-proof.md`.
**Fix = restore the two pre-`1cb6a43` constant lines + hard-code the real bytes in the
two regression tests (de-self-reference them).**

Also from the same LDM deep pass: **OQ-T7 RESOLVED** (RAID-5 rotation = left-symmetric
on both sides, algebraically identical formulas; the og's other three rotation modes
are dormant dead code — finding **I-008**; reconstruct-on-failure semantics identical);
the GPT-LDM gating question resolved (og gates on the real metadata GUID, PRIVHEAD
candidate at the partition's last LBA, no CRC checks — all matching the core);
**A-001 REOPENED** for its false-positive LDM half (its ESP/MSR/BIOS-boot/Linux half
stands fixed). The invalid-UTF-8 name-fidelity finding this leg drafted (N-010) is
**superseded by M-010** (same substance, filed by the close-out leg — dropped here,
ID retired); this leg's evidence quote `evidence/decompiled-quotes/s2-zfs-utf8-conversion.md`
supports M-010. This leg's dormant-rotations finding is renumbered **I-008** (ID
collision with the parallel leg's I-006 ext4-nanoseconds, cleared).

## Counts per severity (active — excludes FIXED/) — session-3 reconciled

| Severity | Active | FIXED (in `findings/FIXED/`) | IDs (active) |
|---|---|---|---|
| ACUTE | **0** | A-001 (partial — ESP/MSR half stands fixed; LDM half false-positive), A-003, **A-002 (`7f2c83e` — real Microsoft/og GUIDs restored, session-3-verified; tests de-self-referenced)**, **A-004 + S-016 (`c33236f` — live-sink ErrorRange delivery + live snapshot error map)** | — |
| SEVERE | 5 | S-001, S-002, S-004, S-005, S-006, S-008, S-010, **S-011 (tz parity; D-49 + host-independence correction)**, **S-013 (windowed-FAT accessor)**, **S-015 (raw-sign KiB units + per-format tails)**, **S-017 (1cc08c3)**, **S-014 (s4 — live+offline auto-unlock, cr/reghive module)**, **S-019 (7cf6754 — lost BitLocker FVE probe)**, S-018 (INVALID), **S-012 (diff-fix session — two-pass NTFS; D-51)**, **S-022 (diff-fix session — INDX + $INDEX_ROOT second-source harvest)** | S-003, S-007, S-009 (split pending N-020), **S-020, S-021 (S3 audit legs)** |
| MAJOR | 18 | M-001, **M-009 (7fd00e0 — 64 KiB envelope on the repair read path, session-3-verified)** | M-002, M-005, M-007, M-008, M-010, M-011..M-021, **M-022 (S3: ReFS file-value grammar)**, **M-023 (S3: filter model)** |
| MINOR | 26 | N-002, N-003, N-010-theirs, **N-021 (this fix session)** | N-001, N-004..N-009 (grouped), N-011..N-022, N-023..N-029 (S2-third-leg, grouped), **N-030..N-032 (S3)** |
| INFORMATIONAL | 10 | — | I-001..I-011, **I-012 (S3: GAP-ADS resolved — parity, shared limitation)** |
| tracked-open | 6 files | OQ-T1/T2/T5/T7/T8/T9 + all 9 S2-third-leg OQs RESOLVED across legs | OQ-T3*, OQ-T4*, OQ-T6 (bounded), OQ-S2A-1..4, OQ-S2C-3, OQ-S2M-1/2 (see files) |
| **Total active findings** | **95** | **36** | |

*(Session-4 MERGE re-baseline (direct file inventory after integrating the parallel
diff-fix/api/audit legs): 11 SEVERE + 34 MAJOR + 37 MINOR + 16 INFO = 98 active files,
33 FIXED files, 11 tracked-open. This inventory supersedes every earlier count table in
this file — including the wave-1 "86 active" and the mid-session "104 active" figures
(both pre-merge snapshots). Session-4 S-012 was a CONVERGENT dual-agent fix — the landed
implementation (5f1a212 + leak follow-ups) stands; the parallel verification is in the
S-012 Resolution's convergence addendum.*

*(Session-4 continuation: **S-020 (SEVERE) + M-022 (MAJOR) + N-037 (MINOR) FIXED** by
the ReFS attribute-layer landing — the counts move to 10 SEVERE + 33 MAJOR + 36 MINOR
+ 16 INFO = 95 active, 36 FIXED (N-036 half-fixed stays active with its container-map
residual). Fail-before 6/6, suite 402/402 Linux ASan.)*

*(Session-4 continuation 2: **M-026 + M-027 (MAJOR) FIXED** — the ReFS deleted model
(stale-MSB-page sweep) + object identity/recycle routing. Counts: 31 MAJOR active,
38 FIXED. Fail-before 3/4 (the 4th a regression guard), suite 429/429 Linux ASan.
The whole ReFS engine tier is now landed: attribute layer + deleted model + identity
+ recycle; remaining ReFS items are N-036's container map and the OQ-2 real-volume
dump.)*

*(Session-4 continuation 3: **the INFORMATIONAL tier CLOSED** — I-006..I-020 all
ACKNOWLEDGED with first-hand verification of each deliberate-difference rationale
(code/corpus spot-checks recorded per finding; I-018 is a positive MATCH record;
I-012 closes the ReFS ADS question as shared limitation). The INFO tier joins
I-001/I-002/I-003 (acknowledged earlier). No code changes — the mandate's rule for
safety-hardening/superset registrations: preserve the divergences, never "fix"
down to the og.)*

**Note on S-009/M-006:** the S-009 disk-map component was FIXED via GAP-S6-11
(`56cce87` — `cr_diskmap.cpp/.h` verified in-tree by this leg); the M-006 fuzz-CI
component was FIXED (`e30b84d`). The combined finding file stays in `SEVERE/` until the
split tracked in N-020 lands.

**Note on ID collisions (both resolved):** (1) parallel-leg N-010 (FAT BAD sentinel,
FIXED) vs session-2 N-021 (FAT invalid-date) — renumbered to N-021. (2) this leg's
ext4/Btrfs invalid-UTF-8 finding (briefly M-007 in a local commit) renumbered to
**M-010** to avoid colliding with the parallel leg's M-007 (NTFS hard-link fan-out).

## Remediation addition
| 2026-10-26 (diff-fix) | S-012 | **FIXED** | Two-pass NTFS landed: destroy default off (pass-1 membership), GUID-dump pass 2 (claim map built during the walk, 0x400/512 stride, og-15 metafile gate, pseudo-# dedup, destroy on all-run-starts-claimed / <=0x3ff / $-name, folders never), size-0 rescue (mirror resident harvest + post-walk same-name merge, 0x800 skip). 5 tests (4 fail-before proven); suite 339/339 + ASan 339/339; D-51 |
| 2026-10-26 (diff-fix) | S-022 | **FIXED** | INDX + $INDEX_ROOT second-source harvest in the remnant path (og validator/chain/entry gates, in-use only never slack, 8 KiB window overlap, D-N re-decided on resolved evidence). 4 tests (3 fail-before proven); suite 358/358 + ASan 358/358 |

## Counts per module (active findings)

| Module | ACUTE | SEVERE | MAJOR | MINOR | INFO | OPEN | Complete |
|---|---|---|---|---|---|---|---|
| ntfs | – | – | 2 (M-007, M-008) | 3 (N-001, N-011, N-012) | 1 (I-001 ack) | OQ-S2A-1..4 | ✔ |
| fat | – | – | – | 3 (N-004, N-021, N-022) | 1 (I-002 ack) | OQ-T2 **RESOLVED** | ✔ |
| exfat | – | 1 (S-011) | – | – | – | OQ-T5 | ✔ |
| ext4 | – | 1 (S-003 OPEN) | 1 (M-010) | 1 (N-004) | 2 (I-006, I-007) | – | ✔ |
| hfsplus | – | 1 (S-003) | – | 1 (N-005) | 1 (I-007) | – | ✔ |
| btrfs | – | – | 1 (M-010) | 1 (N-006) | 2 (I-003 ack, I-007) | – | ✔ |
| zfs | – | – | – | – | 1 (I-007) | OQ-T6 | ✔ |
| refs | – | – | (project GAP: ADS) | 1 (N-007) | 1 (I-004 ack) | – | ✔ |
| ldm/table | – | – | 1 (M-002) | – | 1 (I-008) | OQ-T7 **RESOLVED (this leg)** | A-002 **FIXED** (fix session) ✔ |
| lost-partition | – | 1 (S-006) | – | – | – | OQ-T9 | ✔ |
| carve | – | 1 (S-007) | 1 (M-005) | 1 (N-008) | – | OQ-T8 | ✔ |
| session | – | – | – | 1 (N-014) | (I-005.7 ack) | – | ✔ |
| writer+report | – | – | – | 3 (N-009b, N-009d, N-013, N-015) | – | – | ✔ |
| health | – | – | – | 1 (N-009a) | – | – | ✔ |
| rawfixer | – | 1 (S-010) | 1 (M-009) | – | – | OQ-S2C-3 | ✔ |
| orchestration | – | – | – | 2 (N-016, N-017) | – | – | ✔ |
| visualization | – | 1 (S-009 partial) | (project GAP: filter cats) | – | – | – | ✔ |
| abi (cr_ui) | – | (S-009 partial) | – | – | – | – | ✔ |
| cli | – | – | – | 1 (N-009c) | – | – | ✔ |
| quality (cross-cutting) | – | – | (M-006 partial) | 3 (N-018, N-019, N-020) | – | – | ✔ |

*(M-010 counts once per affected engine row for navigation, as S-005 did.)*

## Top risks (ranked — fix-session state)

1. **A-004 (ACUTE — event-range fires through a config sink; the only active ACUTE
   post-`7f2c83e`)** — crash-class latent defect on the public API surface; one-line
   fix + two tests per its plan. **← the fix session's next item.** Then **S-015
   (session 3) — raw-sign recall window 1024x too
   small** (raw_signs.json min/max read as bytes vs the og's KiB): every raw-sign
   format caps at 50 KiB (200 KiB for the media entries) where the og carves 50 MiB
   (200 MiB) — the largest single recall gap found in session 3; one-generator fix.
2. ~~**S-011 — exFAT `tz_offset` applied (core) vs ignored (og).**~~ **FIXED**
   this fix session (tz subtraction removed; on-disk time through the shared
   FAT-family datetime core, host-independent on-disk-as-UTC; guards
   TimestampsMatchOgLocalAsUtc / TimestampsZeroTzPassThrough /
   TimestampsMatchExplorerDisplay).
3. **S-003 — HFS+ journal replay missing.** Crashed volumes lose journal-carried recent
   state. Journal replay mechanics decoded first-hand (`b85c084` evidence addendum);
   fix deferred with precise scope.
4. ~~**S-006/S-007 — discovery coverage.**~~ **S-006 FIXED** this fix session
   (ext = og parity; HFS+/Btrfs/ZFS/ReFS superset anchors; FATX scoped
   remainder); **S-007 remains open** — carve misses GIF/BMP/headerless-MP3/
   AAC/RTF/OLE-Office/MPEG/DWG.
5. **M-007/M-008 (session 2, superset) — NTFS hard-link fan-out + `$ATTRIBUTE_LIST`
   multi-hop chase.** Both SUPERSET divergences where the og's behavior is a limitation.
   Opt-in parity flags recommended.
6. **M-010 (this leg) — ext4/Btrfs invalid-UTF-8 names destroyed** (U+FFFD baked; og
   preserves raw bytes; ZFS edge is verified parity). Permanent loss of original name
   bytes for legacy-locale filenames.
7. **M-009 (session 2, perf) — RawFixer bypasses the 64 KiB direct-I/O envelope.**
   ~3.5× slow path on 4Kn NVMe. **FIXED** (`7fd00e0`, verified first-hand `14f3a29`;
   in `findings/FIXED/`).
8. ~~**OQ-T7 — LDM RAID-5 rotation unverified.**~~ **RESOLVED** (left-symmetric both
   sides; og's other rotations dormant — I-008).

## Recommended fix order (first 20 — post-reconciliation)

| # | Finding | Rationale |
|---|---|---|
| 1 | S-010 | safety regression on the repair path; can corrupt volumes during concurrent access |
| ~~2~~ | ~~S-011~~ | **FIXED this fix session** (tz subtraction dropped; 3 guards; suite 325/325) |
| 3 | S-003 | HFS+ journal replay (mechanics decoded; fix scoped) |
| 4 | S-006 | lost-partition probe families (engine wiring only) |
| 5 | S-007 | carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG |
| 6 | M-009 | RawFixer bypasses 64 KiB envelope; one-line fix (route through aligned_read_at) |
| 7 | M-007 | hard-link fan-out superset; opt-in parity flag (mirrors S-001's pattern) |
| 8 | M-008 | `$ATTRIBUTE_LIST` chase scope+depth superset; opt-in parity flag |
| 9 | M-010 | lossless invalid-byte mapping (U+E000+byte) for ext4/Btrfs; ZFS stays U+FFFD |
| 10 | M-002 | EBR torn-chain salvage |
| 11 | M-005 | carve bitmap gating (perf + FP surface) |
| 12 | N-021 | FAT invalid-date yields non-zero FileTime vs og zero; one-line guard |
| 13 | N-013 | dest-overlap optional + first-extent-only; composes with S-010's lock fix |
| 14 | N-014 | stage checkpoint not atomic; atomic-rename fix |
| 15 | N-015 | writer no disk-full pre-check; composes with S-010's lock fix |
| 16 | N-016 | emit_boundary_events double-call bug; one-line removal |
| 17 | N-017 | cancel watcher coverage inconsistent; one-line parallel_for poll |
| 18 | N-018 | UBSan CI lane; mirror the TSan lane |
| 19 | N-019 | 12 missing per-parameter regression tests |
| 20 | N-020 | 6 stale doc claims; split S-009/M-006 finding file; move both to FIXED/ |

## Audit coverage statistics

- **Core read (sessions 1+2):** 54 src .cpp (26,867 LOC), 56 headers, 38 test files
  (313 TEST cases / ~17,730 LOC at `8574039` — re-run green this leg) — agents C+D
  exhaustive; session-2 agents added line-by-line reads of the NTFS mftwalk, all 8
  engine .cpp, query/writer/session/transport/rawfixer/orchestrator.
- **Corpus read (sessions 1+2):** 9 architecture docs, 274 artifact docs, raw decompiled
  trees. Session 2 added first-hand reads of `ghidra-partition/*.c` (RecoveryModule,
  PartitionRecovery, RawFixer) that session 1 did NOT use — these contained the canonical
  FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME sequence (S-010) and the ACL-preservation
  `SetFileSecurityW` calls (N-009d refinement). This leg re-verified first-hand:
  HFSPlusSearchDLL FUN_180009c10 (S-002), EXT4SearchDLL jsb loop (S-004),
  RawFixer FUN_18005de40/FUN_18005df00 (S-008), FATSearchDLL mask-24 + FUN_180041c80
  (N-002/N-010), ZfsSearchDLL FUN_18001c630 (M-010), EXT4/Btrfs import censuses (M-010).
- **Executed verifications:** core suite 313/313 Linux ASan at `8574039` (this leg);
  fail-before/pass-after proofs cited in each fix's Resolution.
- **Not executable here:** og-execution (no Windows/wine) and new Ghidra decompiles —
  all such questions are in `tracked-open/` with precise probes.

## Honesty notes

- Session 1's NTFS B7 "MATCH" verdict for hard-link/ADS/sparse was **REFUTED** by the
  parallel leg's line-by-line decompile read. Three new findings (M-007, M-008, N-011)
  and one minor (N-012) surfaced. The session-1 verdict was based on a misread of the
  LOCK/unlock block as hard-link refcount management.
- Session 1's Q67 "M" (sanitizer cleanliness) was incomplete — UBSan is not enabled
  anywhere (N-018). Session 1's Q65 "M" (test coverage) was aspirational — 12 of 13
  audit-prompt scenarios lack isolated tests (N-019). Session 1's Q68 "M" (documentation
  accuracy) had 6 stale claims (N-020).
- The parallel leg's SUMMARY listed S-002 as open — **stale**: the fix (`c9e8860`) is
  in the `8574039` baseline and was independently re-verified first-hand this leg
  (FUN_180009c10 tree-8/keyLength/recordType/value-offset reads); S-002 now lives in
  `findings/FIXED/`.
- The fixed findings are preserved in `findings/FIXED/` for the audit trail and are NOT
  counted in the active totals. `findings/FIXED/RESOLVED-INDEX.md` catalogues each
  resolved finding's title, fix commit, and resolution summary.
- The 7 ACKNOWLEDGED informational findings (I-001..I-007) are deliberate supersets,
  pinned with regression tests; they stay in `findings/INFORMATIONAL/` because they are
  documented improvements, not bugs.
- The 18 corpus errata candidates are cataloged in `diff-to-fix/CORPUS-ERRATA.md`.

## Remediation log (combined: all legs)

| Date | Finding | Verdict | Evidence |
|---|---|---|---|
| 2026-10-26 (s5) | S-021 + S-003 | **FIXED** | HFS+ journal differential harvest (the og's PRIMARY deleted surface): JournalInfoBlock→jnl_header gates (LE)→ring walk with split reads→leaf images→fork classification→live differential→deleted-tier emission with bitmap grading + N-028a sink filter + extents fallbacks. S-003's "replay" premise corrected (scratch install + unchanged live walk); the fallback salvage tracked (OQ-S003). 4 tests (fail-before 3/4 on the stashed engine); 423/423 ASan; PARITY-DELTAS D-56 |

| 2026-10-26 (S4) | A-005 | **FIXED** (ACUTE) | sector-sized buffers + sector-correct plans; fail-before = the ASan stack-buffer-overflow itself (git-stash proof); 3 4Kn tests; suite 477/477 ASan; D-61 |
| 2026-10-26 (S4) | S-007 (primary) | **FIXED** (niche ~25-format tail scoped in the finding) | 27 registry entries + 9 validators, corpus-exact keys; suppress_overlap + 10-frame gate registered in D-57; fail-before 3/3; suite 436/436 ASan; hostile smoke clean || 2026-10-25 (s4) | S-019 | **FIXED** | probe_bitlocker with the og's exact classifier gate (OEM memcmp -> type 0xB), dispatch before the FAT catch-all; boot-sig/fve-shape scoring (grade-only shape); FsFamily::BitLocker + probe_family branch; marked device-tail size fallback. 3 fail-before/pass-after tests; suite 365/365 ASan. `findings/FIXED/S-019-*.md` |
| 2026-10-25 (s4) | S-014 | **FIXED** | Live path -> Control\FVEAutoUnlock (DRWService :104) + per-GUID Data blobs with og caps (50-char GUID / 256-B blob / 100 records); NEW portable cr/reghive regf parser (offsets decoded from the og's RegLib_decompiled.c) + offline hive path (bitlockerhelp flow, hardcoded ControlSet001 per the corpus) + mounted-volume probe; GUID-match-first selection. 11 Linux + 5 Windows tests + fuzz harness; suite 341/341 ASan. `findings/FIXED/S-014-*.md` |
| 2026-10-25 | A-001 | **FIXED** | canonical GUIDs (`1cb6a43`); in `findings/FIXED/` |
| 2026-10-25 (S3) | A-002 | **FIXED** | `7f2c83e` real Microsoft/og GUIDs restored; tests de-self-referenced + negative guards; session-3 independent og-binary re-extraction converges (4 binaries / 7 sites) |
| 2026-10-25 (S3) | S-010 | **FIXED** | `3e34ed2`+`414f56c`+`34b3441` volume quiesce on the repair write path; 3 fail-before lock tests; SUMMARY stale-row reconciled this session |
| 2026-10-25 (S3) | M-009 | **FIXED** | `7fd00e0` RwDevice::read_at routes through the 64 KiB envelope; verified first-hand this session |
| 2026-10-26 | **M-010** | **FIXED** | lossless invalid-UTF-8 name mapping for ext4/Btrfs (U+E000+byte, bijective, with the inverse); ZFS keeps U+FFFD (the og asymmetry pinned); Text.* + Btrfs.Latin1* + updated NonAscii* tests; suite 462/462 Linux ASan |
| 2026-10-26 | **A-005 (NEW, discovered during S-021 verification)** | **FIXED** | HFS+ B-tree record-span convention inverted vs TN1150/the og's validator — on real volumes only record 0 per node was emitted; for_each_record spans + all fixtures corrected (one builder point); LeafNodeLayoutMatchesOnDiskFormat guard; fail-before: the old reader fails EndToEndWalk + both Decmpfs tests against the corrected fixtures |
| 2026-10-26 | **S-021** | **FIXED** | the journal deleted-record harvest (the og's PRIMARY deleted HFS+ surface): JIB/jhdr gates, ring walk (u16 header test - a decode correction), og node-image validator, fork classification by volume ranges, catalog differential + extents side map, sweep dedup; 4 tests; suite 401/401 Linux ASan; decode corrections recorded in evidence s3-hfsplus-inventory.md §3a + errata E37 |
| 2026-10-26 | **A-005 (NEW, discovered during S-021 verification)** | **FIXED** | HFS+ B-tree record-span convention inverted vs TN1150/the og's validator — on real volumes only record 0 per node was emitted; for_each_record spans + all fixtures corrected (one builder point); LeafNodeLayoutMatchesOnDiskFormat guard; fail-before: the old reader fails EndToEndWalk + both Decmpfs tests against the corrected fixtures |
| 2026-10-26 | **S-021** | **FIXED** | the journal deleted-record harvest (the og's PRIMARY deleted HFS+ surface): JIB/jhdr gates, ring walk (u16 header test - a decode correction), og node-image validator, fork classification by volume ranges, catalog differential + extents side map, sweep dedup; 4 tests; suite 401/401 Linux ASan; decode corrections recorded in evidence s3-hfsplus-inventory.md §3a + errata E37 |
| 2026-10-25 | M-001 | **FIXED** | record-size ladder (`5d6da5f`); in `findings/FIXED/` |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run sub-fix (`e93decf`); in `findings/FIXED/` |
| 2026-10-25 | S-002 | **FIXED** | attributes B-tree + decmpfs modes 3/4 (`c9e8860`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-004 | **FIXED** | ext4 journal every-block salvage + jsb gate (`51ac671`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-005 | **FIXED** | shared UTF-8 decoder (`2f3237d`); residual edge split to M-010; in `findings/FIXED/` |
| 2026-10-25 | M-009 | **FIXED** | RwDevice::read_at routed through the GAP-S6-13 64 KiB envelope (same aligned_read_at the scan devices use); structural routing parity + envelope unit tests; 4Kn executed numbers deferred to the bench harness (no Windows/4Kn host here - same limitation the finding records) |
| 2026-10-26 | **A-002** | **FIXED** | Real Microsoft LDM GUIDs restored in `cr_table_wire.h` (PARTITION_LDM_METADATA_GUID {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3} / PARTITION_LDM_DATA_GUID {af9b60a0-1431-4f62-bc68-3311714a69ad}); regression tests de-self-referenced to hard-coded real bytes; negative guard (fabricated values → ERR_BAD_MAGIC) + bytes→text guard added; harness + probe log + parity doc corrected; suite 303/303 Linux ASan |
| 2026-10-26 | **A-004 + S-016** | **FIXED** (same delivery path, one commit) | A-004: ErrorRange delivered through the live sink (`event_fn_`), not the config copy (null-call crash / mis-delivery on the documented set_event_sink pattern); S-016 (discovered during A-004's fail-before proof): `tel_counts_` now feeds the live retry-pipe error map into the snapshot before every boundary emit — the ErrorRange event stream was dead code (snapshot error map was merged only after the emitters). Two ScanEvents tests (ErrorRangeGoesToLiveSink, SinkSwitchDeliversAllTypes); suite 305/305 Linux ASan || 2026-10-25 | S-010 | **FIXED** | volume quiesce on the repair write path (IWriteDevice lock contract, execute() lock-before-write + zero-write refusal, Windows RwDevice runs the corpus RecoveryModule LOCK/DeleteMountPoint/Flush/settle/verify/DISMOUNT sequence); finding's destination-lock bullet corrected (corpus: destination open is writability+detection only); 3 fail-before tests, suite 316/316 || 2026-10-26 | S-006 | **FIXED** | lost-partition probe families: ext (og CExtX accept gates decoded + GDT liveness, SB-derived size) + HFS+/Btrfs/ZFS/ReFS superset anchors (D-50; ReFS rescued from the FAT catch-all); 7 tests, 5 fail-before proven; suite 332/332 Linux ASan. FATX = scoped remainder (accept criteria not corpus-decoded); M-017 scoring = own finding |
| 2026-10-26 | S-011 | **FIXED** | exFAT tz subtraction removed — on-disk time through the shared FAT-family datetime core (og parity, host-independent); wire tz fields parsed-but-unapplied (documented in PARITY-DELTAS + AUDIT-FALLBACKS); 3 guards (TimestampsMatchOgLocalAsUtc 0x88 + TimestampsMatchExplorerDisplay 0x20 fail-before/pass-after, TimestampsZeroTzPassThrough sentinel); suite 325/325 Linux ASan |
| 2026-10-25 | S-008 | **FIXED** | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector (`e0175b3`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-26 | **S-015** | **FIXED** | raw_signs min/max KiB units restored (single conversion point in gen_rawsigns.py; zero-max asserted; table regenerated — every format window was 1024x too small, floor 1024x too low); tail walk rebuilt (chunked forward scan, named 8 MiB bound, og full-window range documented); per-format tail scoping (cross-format pairing fabricated close positions — discovered during the fix); 6 fail-before/pass-after tests; suite 310/310 Linux ASan |
| 2026-10-26 | **S-017** | **FIXED** | ftyp-family re-keyed on the box TYPE ("ftyp", header_offset 4 — the box size is data, the pre-fix magic baked literal zeros so NO real MP4/MOV/M4A/HEIC could ever match); match primitive extracted + shared with a NEW F8 self-probe (structural rule: four-zero-byte magic prefixes rejected; candidate-shift assertion); conservative naked-moov salvage (FMT_MOV_NAKED, og MovAnalyzer mirror, tried last); 6 fail-before/pass-after tests; suite 320/320 Linux ASan || 2026-10-25 | S-009 (disk-map half) | **FIXED** | GAP-S6-11 disk-map data producer (`56cce87`); split of the combined file tracked in N-020 |
| 2026-10-26 | **S-018** | **INVALID** (premise refuted) | "zero 0xD9 occurrences in the og analyser bodies" was a grep artifact — Ghidra renders 0xD9 as the signed char -0x27; 5 FF D9 sites exist in the modern JpegAnalyzer and its accept path is EOI-gated (`uVar21 == 0 -> early return`); core validator UNCHANGED; residual (fallback sub-analysers + continuation outcome) tracked-open as OQ-S3-JPEG || 2026-10-25 | M-006 | **FIXED** | fuzz CI re-enabled (`e30b84d`); verified this leg (ci.yml :147-160) |
| 2026-10-26 | **S-013** | **FIXED** | exFAT windowed-FAT accessor (og 1 MiB windows, no cap) as the only fallback — big-FAT volumes never truncate chains to one cluster; whole-FAT fast path kept; fat_window_refills telemetry; fail-before 2/2 (extent coverage 512->1536, second-cluster dir walk); suite 322/322 Linux ASan + hostile-entry smoke || 2026-10-25 | N-002 | **FIXED** | FAT mask-24 EOC family (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-003 | **FIXED** | exFAT checksum og-parity default (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-010 (FAT BAD sentinel) | **FIXED** | ordering fix (`cb8dafd`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | OQ-T1 | **RESOLVED** | secondary claim map semantics (decompile read); in `findings/FIXED/` |
| 2026-10-25 | I-001..I-005 | **ACKNOWLEDGED** | deliberate supersets verified + pin tests (`f74a5dd`); re-verified this leg |
| 2026-10-25 (S2) | S-010, S-011, M-007, M-008, M-009, N-011..N-020, N-021, I-006, I-007 | **NEW** | parallel session-2 leg, wave 2 |
| 2026-10-25 (S2, this leg) | M-010 | **NEW** | S-005 residual invalid-UTF-8 edge; ZFS-side parity verified first-hand (FUN_18001c630) |
| 2026-10-25 (fix session) | A-002 | **FIXED** | LDM GPT type-GUID constants restored to the real Microsoft values; 4 tests de-self-referenced + 3 new guards; fail-before 6/7, suite 320/320 Linux ASan; session-1 evidence chain corrected (see `findings/FIXED/A-002-*.md`) |

## Session-2 verification log (this leg, baseline `8574039`)

| Finding | Verdict | Independent evidence (first-hand this leg) |
|---|---|---|
| S-002 | **CLOSED** (`c9e8860`) | FUN_180009c10 re-read :7690-7859 (tree-8 search, keyLength `(nameLen+7)*2`, recordType 0x10 gate, value u64@+8); core walk_attributes_tree + kZlib extents in-tree; tests green 313/313 |
| S-004 | **CLOSED** (`51ac671`) | jsb loop re-read :12195-12310 (0xc03b3998 magic, blocktype {3,4}, 1 MiB windows, per-block cancel); core jsb gate + every-block parse in-tree; tests green |
| S-008 | **CLOSED** (`e0175b3`) | FUN_18005de40/FUN_18005df00 re-read :64981/:65013 ('FILE' magic + record-name); core method surface + pre-scan consent wiring in-tree; 16 tests green |
| N-002 | **CLOSED** (`fb44073`) | mask-24 family re-read :8030-8048; core cr_chain.cpp :73-80 raw-entry tolerance; test green |
| N-003 | **CLOSED** (`fb44073`) | validate_entry_sets default false (og parity); D-47 registered; both pins green |
| N-010 | **CLOSED** (`cb8dafd`) | FUN_180041c80 re-read :54124-54137 (0xfff7 → 0xffffff7 → BROKEN); core CHAIN_BAD-before-reserved-range; pin green |
| M-006 | **CLOSED** (`e30b84d`) | fuzz lane re-enabled in ci.yml :147-160 (codeql deferral documented) |
| I-001..I-005 | **ACKNOWLEDGED** (`f74a5dd`) | rationale re-verified; pins UserFileNamedDollarNotHidden / Fat16BadClusterStillCanonical / Btrfs.MirrorSelectionNewestGeneration green; D-12/D-39/D-46/D-47/D-48a-c registered |

---

# Third leg — main-auditor close-out (session 2, 2026-10-25; core `e93decf` audited, tree re-verified at `34b3441`)

The decompile-exhaustive pass session 1 never made (11 parallel read agents + main-auditor
first-hand verification of every ACUTE/SEVERE claim), the 424-file corpus coverage sweep,
and the post-closeout-code audit — re-applied onto this reconciled tree with IDs renumbered
past the two prior legs (**A-004, S-012..S-014, M-011..M-021, N-023..N-029, I-009..I-011**,
OQ-S2M-1/2; one candidate RETRACTED by this leg's own re-read).

## Verification of this tree's state (read-only facts the roadmap needs)

1. **A-002 is still UNFIXED on the current tree** — verified at `34b3441`:
   `include/cr/table/cr_table_wire.h:87-89` still carries the fabricated
   `{5808C8AA-8E9F-4C87-…}`/`{AF9B60A0-384E-4F7D-…}` LDM constants with comments claiming
   `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` identity (the real Microsoft
   values differ in fields 2-3: `…-7E8F-42E0-85D2-…`/`…-1431-4F62-…`). `23adad1` filed the
   proof and the regression fixture but the constant restore itself never landed (the
   commit touches no source file). **A-002 remains the roadmap's #1 item** — agreed with the
   third-leg close-out, now with a current-tree citation.
2. **M-006 verified FIXED** (`e30b84d`; fuzz job re-enabled, codeql deferral documented).
3. **The `fb44073` N-003 fix introduced a divergence:** its `validate_entry_sets = false`
   default rests on a "verified-absence" premise the decompile refutes (`FUN_18001eba0` =
   the dual-form checksum incl. deleted sets — quoted in the FIXED/N-003 note). Og parity
   = validating; **M-013** carries the correct fix. Recommend scheduling it with the next
   exFAT pass rather than leaving the false default.
4. **S-001's landed destroy rule is mis-scoped** (S-012): og pass 1 emits every live-MFT
   deleted record ungraded/undestroyed; the destroy+viability machinery lives in a
   GUID-dump pass 2 the core does not have. First fix step is one flag flip (restore
   membership parity); the real feature is pass 2.
5. **Empirical:** full Linux suite at `e93decf` → **282/282 PASS** (this leg's own run,
   Release build; `evidence/executions/build-suite-session2.md`). ASan build not runnable
   in this container (memory reaping) — CI carries it.

## Third-leg additions (all active; renumbered)

| Severity | New IDs | One-liners |
|---|---|---|
| ACUTE | **A-004** | ErrorRange events fire through the config sink while the guard checks the live sink (latent null-call crash on the documented `set_event_sink` pattern) |
| SEVERE | **S-012** | NTFS single-pass: og's GUID-dump pass 2 (viability/destroy/0x3ff/blanket-$) + size-0 rescue missing; destroy mis-scoped into the main walk |
| SEVERE | **S-013** | exFAT: FAT > 32 MiB silently dropped → every FAT-chained file/directory truncates to one cluster (~34 GB+ volumes) |
| SEVERE | **S-014** | BitLocker auto-unlock: wrong live registry path (Services vs Control) + offline-hive path absent + unassociated enumeration (F61 — never scored before) |
| MAJOR | **M-011..M-015** | FAT table-size windows · mirror policy (subsumes N-022's sub-case) · exFAT dual-form checksums+oracle (supersedes N-003's premise) · exFAT structural gates · FAT table validation |
| MAJOR | **M-016, M-017** | LDM TOCBLOCK candidates · lost-partition scoring formula (OQ-T9 resolved: ln(count)/ln(max), +500 table-match, count==1 quirk — 187-row-verified) |
| MAJOR | **M-018..M-021** | FilterFolder junk filter · ZFS multi-vdev fallback + fletcher4/DVA-retry · Btrfs chunk profiles + default subvol · carve video-repair hook |
| MINOR | **N-023..N-029** | FAT/exFAT gates · NTFS gates/fields · LDM geometry · ReFS pages · carve/writer · HFS+ sink/forks · U+FFFD residuals (cross-refs M-010) |
| INFO | **I-009..I-011** | carve probing supersets · scope-exclusion/supersets register · corpus errata (8 claims; feeds CORPUS-ERRATA.md) |
| tracked-open | **OQ-S2M-1/2** | NTFS grading locus (D15) · SearchDLL bonus setter / ZFS pool name / Btrfs anchor |

**Retraction (recorded):** the exFAT fixer-checksum "no-skip" candidate — this leg's own
decompile re-read found the skip-table {106,107,112} (`std::find == end()` over the
−1-terminated `DAT_180079a40`); the core's spec-skip is a MATCH, the parallel leg's binary
extraction was right. Corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`.
This leg also corrects its own first A-001 verification note (FIXED/A-001): the parallel
leg's executed proof wins — recorded in place.

**Direction corrections to prior findings (fix plans replaced in-place):** S-002 (og never
decompresses decmpfs — size-from-attributes-tree + raw-rsrc content is the parity target;
landed `c9e8860` should be audited against that boundary) · S-003 (journal =
differential-salvage evidence vs the overlay model — both readings recorded, consumption-
side quotes favor salvage) · S-004 residue (journal **inode-image salvage**, magic 0xf30a —
not covered by `51ac671`).

**Counts (all three legs combined, active — post session-3 A-002 fix):** 1 ACUTE
(A-004) · 9 SEVERE (S-003, S-006, S-007, S-009-remainder, S-011, S-012, S-013,
S-014 + S-010 per leg-3's table) · 17 MAJOR (M-002, M-005, M-007..M-021) · 24 MINOR
files · 11 INFO · 2 new tracked-open files. FIXED/: 13 findings (A-002 added — real
Microsoft LDM GUIDs restored; tests de-self-referenced; negative guard added) + all
OQ-T1..T9 resolutions. **Recommended order now: A-004 (one-line event-sink fix
restoring crash-safety), then S-012 step 1 (one flag flip restoring og deleted-file
membership), then S-011/S-013/S-014, then the leg-3 majors in M-011..M-021 order.**

Read-only compliance held throughout: only `diff-to-fix/` + `worklog.md` written; the
282/282 run was out-of-tree (Release); no source, test, build, workflow, or doc file of
either repository was modified.

---

## Session-3 PARALLEL-LEG addendum (2026-10-26 — S3 lead's wave-1, reconciled onto main)

A second session-3 leg (S3-CARVE / S3-LOST / S3-REFS parallel agents + lead) worked the
same mandate concurrently with the leg above and reconciles here. Its 17 findings were
**renumbered past the IDs that landed on main first**; five were merged as addenda into
this tree's findings (convergent decodes), one was dropped as a duplicate, one as a
superseded constant set. Final disposition:

**New standalone findings (14):**

| ID | Severity | Module | One-line |
|---|---|---|---|
| S-017 | SEVERE | carve | ftyp-family signatures unreachable — MP4/MOV/M4A/HEIC carve dead in core (box SIZE encoded as literal zeros; executed proof, 0 grammar hits for 4 planted real BMFF files) |
| S-018 | SEVERE | carve | JPEG EOI policy: neither og analyser generation requires EOI (size = SOF arithmetic); core requires EOI+SOF+SOS → truncated photos salvaged by og, dropped by core |
| S-019 | SEVERE | lost-partition | BitLocker "-FVE-FS-" lost-partition probe family missing (S-006 under-listed the og; executed string-probe evidence; distinct from S-014's win-layer unlock-path finding) |
| S-020 | SEVERE | refs | ReFS resident/inline content never materialized: FUN_18000e490/ea30 build CRefsResidentDataRunList (lead re-verified first-hand at :10413-10462); core's resident_data never populated + val_len ≥ 0x28 gate drops resident rows → inline files metadata-only. Answers M-022's fix-plan step 4; needs M-022's grammar walk first |
| M-024 | MAJOR | carve | No runtime format-filter surface vs og RawConfig.ini [Filter] (34 toggles — CORRECTIONS #5 recount; was 37, M-024 addendum; STL/VPJ default-off) — registry rebuilt per search |
| M-025 | MAJOR | carve | Read model: og 8 MiB ping-pong + sector dispatch + pending-CRawFileInfo streaming across chunks; core 1 MiB byte-granularity stateless + extent re-reads. M-021's hook fix depends on this state machine |
| M-026 | MAJOR | refs | ReFS deleted-recovery model: og stale-"MSB+"-page sweep (FUN_180005330) force-marks unreachable-page records deleted; core harvests row-flag 0x4 + ID2 rows — different on-disk premise (CoW medium favors the og's model) |
| M-027 | MAJOR | refs | ReFS object-identity keep-one + Recycle-Bin routing ($R*/S-*/SVI collection, $I resident read) missing; core emits hard-linked objects N times |
| N-033 | MINOR | carve | Analyser registry exports (List/Map/Multimap) consumed by the FS SearchDLLs (registry snapshot clones); no core ABI to enumerate carve formats |
| N-034 | MINOR | lost-partition | FAT lost-boot gate constants: numFATs ∈ {1,2}, FAT12 spf ≤ 200 probe, root_entries×32 % bps — complements N-010b's engine-side gates (same og constants, `src/core/lost/` consumer) |
| N-035 | MINOR | lost-partition | Lost-candidate extents are data-area-anchored in the og (EndSec = DataAreaStart + totalSector) vs core whole-volume — the M-017 containment/16-MiB rules consume different extents |
| N-036 | MINOR | refs | ReFS datarun flags: og requires 0x40 + container-map translation for non-physical runs; core requires 0x10, map never populated |
| N-037 | MINOR | refs | ReFS runlist granularity validation (og discards cluster-inconsistent runlists as FP guard) missing in core |
| I-013 | INFO | refs | ReFS host-OS runtime gate (og refuses < Win8 hosts) — core host-agnostic, deliberate portability superset |

**Merged as addenda (5) + drops (2):** dispatch chain semantics + id-8 continuation
records → **N-031 addendum** (convergent with its decode; adds u16 insertion-order
chains, TTF-vs-MDB u32 pre-emption, TIFF-family same-class relabeling); EUVideoRepair
per-chunk feed + stream-geometry submission → **M-021 addendum**; RTTI proof that
*CalcPartition classes are geometry calculators (slot 4 = sectors-per-cluster, executed
vtable walk) + full AdjustPartitionInfoOrder decode (containment drop, FS-grouped
dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) →
**M-017 addendum** (OQ-T9's premise corrected — the classes are NOT scorers; the
DRWUI decode above is the real model, independently confirmed to identical constants);
resident-materialization answer → **M-022 addendum**; byte-granularity pin test →
**I-009 addendum**; lost-probe superset row → **I-010 addendum**. Dropped: this leg's
N-029 (ReFS boot-geometry gate — duplicate of N-026's N-013b, which is more precise)
and its stand-alone I-009/I-010/M-013/M-014/N-023/N-024 (superseded by the merges
above).

**S-007 CORRECTED (decompile re-verified first-hand by this leg's lead at
RAWSearchDLL_decompiled.c:1985 / :2156-2177 / :3097):** the session-3 addendum inside
S-007 carries two wrong rows — `C5 D0 D3 C6` is **CEPSAnalyser (AI/EPS)**, not dBase
(no DBF analyser exists anywhere in the decoded registry), and the LZH row's bytes are
wrong on both counts: `E4 52 5C 7B` is **OneNote** (OneAnalyser, format id 0x31);
LZH is a list-layer structural analyser (LZHAnalyser). The fix plan's DBF validator is
dropped; OneNote magic + list-layer LZH take its place. Errata rows 22-28 filed.

**A-002 status chain (for the audit trail):** this leg independently re-verified the
og GUIDs (re-executed the LdmManager.mo/Partition.mo probe + external cross-check)
while the tree still carried the wrong constants — then found the fix leg had landed
`7f2c83e` mid-session on main; the fix is verified (constants = `5808c8aa-7e8f-42e0-
85d2-e1e90434cfb3` / `af9b60a0-1431-4f62-bc68-3311714a69ad`, tests de-self-referenced
with text-compare + negative guard at test_crc32_gpt.cpp:394-395/:432-433,
test_ldm.cpp:616-617). **A-002 stays CLOSED.**

**OQ-T9 tracking reconciled:** the tracked-open file's stale UNVERIFIED header is
replaced with the full resolution (both legs' decodes merged; residues → S-019/N-034/
N-035 + OQ-S2M-2's bonus setter).

**Counts after this addendum (by direct file inventory):** ACUTE **0** (A-004 now
FIXED — moved by `c33236f`, which also fixed S-016) · SEVERE 9+4 = **13** (S-003,
S-006, S-007, S-009-partial, S-011, S-012, S-013, S-014, S-015 + S-017..S-020) ·
MAJOR 18+4 = **22** · MINOR 27+5 = **32** · INFO 10+1 = **11** · **total active 78**
(20 in FIXED/). Top-risk insertion: **S-017/S-018 join the carve priorities** (media
formats the og recovers and the core cannot — S-017's ftyp bug makes the four most
common video/photo containers uncarvable TODAY), **S-020 joins the ReFS first-class
blockers** (with M-022/M-026/M-027), **S-019 joins S-006's probe wiring** (implement
together).

---

## Session-3 S3-parallel wave-2 addendum (2026-10-26 — engine remainders closed)

The continuation leg's second wave closed the engine remainders that survived both
prior legs' coverage (NTFS attributes/USN/second-source; HFS+ journal consumer;
ext4/HFS+ inventory residues):

| ID | Severity | Module | One-line |
|---|---|---|---|
| S-021 | SEVERE | hfsplus | Journal deleted-record harvester missing (GetDeleteRecord :13637-14252 + emit driver) — the og's PRIMARY deleted surface; core parses journal_info_block and never uses it |
| S-022 | SEVERE | ntfs | Orphaned-record sweep missing: INDX/MFT records outside live $MFT + $INDEX_ROOT $FILE_NAME harvest — second-source names for files whose MFT records are gone; core has NO INDX decode |
| N-038 | MINOR | hfsplus | Fork-selection predicate: og record byte +0x29 bit 0x20 → rsrc fork + decmpfs mark vs core empty-fork substitution (field-name UNVERIFIED; refines N-015b) |
| N-039 | MINOR | ntfs | $UsnJrnl gate: og reason & 0x200 (COMPRESSION_CHANGE) with dead harvest-all flag vs core true FILE_DELETE — core is the correct side; bounds deltas both ways |
| I-014 | INFO | ext4+hfsplus | Supersets registered: deleted-inode sweep; fragmented-journal-walk correctness (og physical+1 is wrong); allocation-grading (UNVERIFIED); hard links/hot files/decmpfs modes 1-2 MATCH-by-omission; ext4 X51 MATCH |

NTFS remainder closed with MATCH verdicts (lead-verified by targeted greps): $EA/
$EA_INFORMATION/volume/$LOGGED_UTILITY_STREAM ignored by BOTH engines (zero type
comparisons in the og decompile; core dispatches 0x10/0x20/0x30/0x80/0xA0 only);
compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via the
sparse+WOF path). EFS content-grading = UNVERIFIED (no fixture, no code either side).
OQ-T9's residue (the dead agent's USN probe) is dispositioned inside N-039 — its
premise was inverted, probe retained as core-behavior evidence.

**Counts after wave 2 (file inventory):** ACUTE **0** · SEVERE **15** (S-003, S-006,
S-007, S-009-partial, S-011..S-015, S-017..S-022) · MAJOR **22** · MINOR **34** ·
INFO **12** · **total active 83** (20 FIXED). The deleted-file recovery surface now
carries the two largest engine gaps: **S-021** (HFS+ journal harvest) and **S-022**
(NTFS INDX second source) — both are name/metadata-yield gaps on the product's
flagship scenario, ranking with S-012 (NTFS two-pass membership) and S-020 (ReFS
resident content) in the recommended fix order (see the fix session's lane note:
S-012's flag flip first, then the S-021/S-022 substrates, then the S-017/S-018 carve
pair with M-025's state machine).

**Wave-2 post-rebase correction (S3 lead, 2026-10-26):** while wave 2 was in
flight, the parallel fix legs landed `1cc08c3` (**S-017 FIXED** — BMFF entries
re-keyed on the ftyp box TYPE, match_signature extracted, F8 structural self-probe
rejects the all-zero-magic class, FMT_MOV_NAKED salvage, 6 fail-before/pass-after
tests, suite 320/320) and `2adfa29` (**S-018 INVALID** — the JPEG EOI premise was a
grep artifact: Ghidra renders 0xD9 as the signed char `-0x27`, and the modern
analyser's FF D9 sites at :39705/:39719/:39750/:39968/:39989 gate its accept/size
path; the S3 lead independently re-verified the five sites first-hand this session
— the refutation is correct; the legacy-body residual + fallback/continuation
questions are tracked in `tracked-open/OQ-S3-jpeg-fallback-and-continuation.md`;
no code change, correctly). Also landed: S-006 fixed (`abe8e36`), N-021 fixed
(`6fc8c0e`), S-015 fixed (`021e2e6`), and the api-session leg's ABI wave
(`a3fdab4` gate docs + `1f08c20` carve-format registry ops #7-#8 — which addresses
M-024/N-033's ABI surface; verify at the next fix-verification pass).
**Re-baselined counts:** ACUTE 0 · SEVERE **13** (S-003, S-007, S-009-partial,
S-011..S-014, S-019..S-022) · MAJOR 22 · MINOR 34 · INFO 12 · **total active 81**
(22 in FIXED/ — S-017, S-018(invalid), S-006, S-015, N-021 joined the fixed set).

**Final session-3 continuation close-out count correction (S3 lead, by direct file
inventory at `4d138f2`):** S-011's fix also landed mid-flight — the live inventory
is **0 ACUTE · 10 SEVERE (S-003, S-007, S-009-partial, S-012, S-013, S-014, S-019,
S-020, S-021, S-022) · 22 MAJOR · 33 MINOR · 12 INFO = 77 active / 24 FIXED**. The
session-3 continuation's own delta (all legs combined): +28 new findings filed
(S-017..S-022, M-024..M-027, N-033..N-039, I-013, I-014 + the merged addenda),
OQ-T8/OQ-T9/GAP-ADS resolved, 7 findings fixed or dispositioned by the parallel fix
legs in the same window (S-006, S-011, S-015, S-017, S-018-INVALID, N-021, plus the
A-002/A-004/S-016/M-009 retirements earlier in the session), and the remaining
unverifieds are all in tracked-open with precise next artifacts.

*(Session-4 continuation 4: **M-011 + M-012 + M-015 (MAJOR) FIXED** — the FAT
factory trio (table windows / mirror scrutiny + mirror-only / whole-table
validation + copy retry). Counts: 28 MAJOR active, 41 FIXED. Fail-before 2/2 +
3/3 + 2/2; suite 463/463 Linux ASan after five fixture builders gained the
canonical FAT16 media pair (real-volume shape).)*

---

## Fix-session-4 delta (2026-10-26 — A-005 + S-021; the S-012 convergence recorded in the worklog)

**A-005 (ACUTE, NEW — discovered verifying S-021):** the HFS+ B-tree record-span
convention was INVERTED vs the on-disk format (TN1150 records-UP from 14; the og's
journal-image validator FUN_180010720 hard-gates [end-2] == 14 + ascending offsets).
The core's reader and fixtures shared the inversion — on real volumes only record 0
of each leaf node was emitted (the entire HFS+ surface: catalog walk, attributes,
freed-node sweep, decmpfs). Fixed in for_each_record + make_leaf_node (single
fixture-builder point); fail-before proven (old reader + corrected fixtures →
EndToEndWalk/Decmpfs failures). The session-1 corpus artifact's "records grow from
the END backward" note was an inference from the core's own code — corrected as
errata E37.

**S-021 (SEVERE) FIXED:** the journal deleted-record harvest — the og's PRIMARY
deleted surface for HFS+ — implemented from a fresh first-hand decode (three
corrections to the audit's rendering: the u16 header test, the 16-byte entry
layout, the JIB/jhdr field map; all in evidence/decompiled-quotes/
s3-hfsplus-inventory.md §3a). Catalog records pulled from journal transaction
images are emitted as deleted results with the og's differential (absent-key or
different-fileID) + extents side map; the freed-node sweep dedups against the
harvest. 4 tests + the A-005 guard test; full suite **401/401** Linux ASan.

**Counts after this fix (file inventory):** ACUTE **0** · SEVERE **10** (S-003,
S-007, S-009-partial, S-020, S-023..S-028) · MAJOR 22 · MINOR 33 · INFO 12 ·
**total active 77** / **35 FIXED** (7 tracked-open files: OQ-T2/T5, OQ-T6/T7,
OQ-T8/T9, OQ-S2M-1/2, OQ-S3-JPEG, OQ-S012, + the S4-wave OQ files).


---

## Fix-session-4 delta (2026-10-26 — A-005 + S-021; the S-012 convergence recorded in the worklog)

**A-005 (ACUTE, NEW — discovered verifying S-021):** the HFS+ B-tree record-span
convention was INVERTED vs the on-disk format (TN1150 records-UP from 14; the og's
journal-image validator FUN_180010720 hard-gates [end-2] == 14 + ascending offsets).
The core's reader and fixtures shared the inversion — on real volumes only record 0
of each leaf node was emitted (the entire HFS+ surface: catalog walk, attributes,
freed-node sweep, decmpfs). Fixed in for_each_record + make_leaf_node (single
fixture-builder point); fail-before proven (old reader + corrected fixtures →
EndToEndWalk/Decmpfs failures). The session-1 corpus artifact's "records grow from
the END backward" note was an inference from the core's own code — corrected as
errata E37.

**S-021 (SEVERE) FIXED:** the journal deleted-record harvest — the og's PRIMARY
deleted surface for HFS+ — implemented from a fresh first-hand decode (three
corrections to the audit's rendering: the u16 header test, the 16-byte entry
layout, the JIB/jhdr field map; all in evidence/decompiled-quotes/
s3-hfsplus-inventory.md §3a). Catalog records pulled from journal transaction
images are emitted as deleted results with the og's differential (absent-key or
different-fileID) + extents side map; the freed-node sweep dedups against the
harvest. 4 tests + the A-005 guard test; full suite **401/401** Linux ASan.

**Counts after this fix (file inventory):** ACUTE **0** · SEVERE **10** (S-003,
S-007, S-009-partial, S-020, S-023..S-028) · MAJOR 22 · MINOR 33 · INFO 12 ·
**total active 77** / **35 FIXED** (tracked-open: the existing OQ files, now
including OQ-S012).


**Fix-session-4 addendum (M-010):** the invalid-UTF-8 name edge closed — ext4/Btrfs
names now carry a REVERSIBLE encoding of their raw bytes (U+E000 + byte) instead
of baked U+FFFD; ZFS stays replacement (the og's own asymmetry). Suite 462/462.

**ID-collision note (DF4, 2026-10-26):** two A-005s were filed in the same
window by parallel legs — the HFS+ record-span inversion (FIXED, this session)
and the rawfixer 4Kn stack-buffer overflow (open). The rawfixer finding is
renumbered **A-006** (findings/ACUTE/A-006-rawfixer-4kn-stack-buffer-overflow.md);
the hfsplus one keeps A-005 (its identity is baked into the Resolution,
RESOLVED-INDEX, errata E37 and the module docs). Same protocol as the two prior
collisions (recorded above).
- **S-025 FIXED** (ext4 journal reparenting): the og's two-pass model —
  pre-walk harvest (parent map from "."/".." pairs; "."-less blocks stay
  owner-less orphans per S-004), post-walk emit reparenting under live
  parents (record index) or salvaged parents (fixpoint chains); unresolvable
  chains flat with real names (the finding's iNode-naming expectation
  corrected per the corpus: that naming is the sweep's). FILE inodes now
  enter visited_inodes — a LATENT live-file duplicate bug the flat path
  hid, exposed by the claim-interaction test and fixed with it. 3 tests;
  473/473 Release + 473/473 ASan(leaks).

**Live counts (after S-025):** ACUTE 0 · SEVERE 5 · MAJOR 36 ·
MINOR 31 · INFO 18 · tracked-open 12 = **90 active** / **56 files in FIXED/**.

- **S-028 FIXED** (btrfs xattrs): KEY_XATTR(24) dispatch — user.DOSATTRIB
  lands the Windows bits on the node (Samba hex text parsed); other user.*
  xattrs surface as ..xattr/<name> child records with resident values
  (the N-011 ADS shape); btrfs.compression tags the per-file codec property
  (zlib=1/"zlo"=2/zstd=3, the og's exact strings incl. the vendor
  misspelling) as metadata alongside the per-extent truth (I-014 B4).
  3 tests; 484/484 Release.

**Live counts (after S-028):** ACUTE 0 · SEVERE 5 · MAJOR 42 ·
MINOR 44 · INFO 20 · tracked-open 12 = **111 active** / **60 files in FIXED/**.

**N-026 FIXED (the ReFS page/gate group — mixed resolution, 2 fixes + 2 audit
corrections):** N-013b — the DBR geometry gates now match the og exactly: bps
ladder {256,512,1024,2048,4096} (FUN_180012390:13544-13546, re-verified
verbatim) and the cluster product gate {4 KiB, 64 KiB} (:13547-13549); the old
core REFUSED 256/1024/2048-bps volumes the og opens and ACCEPTED 16/32-KiB
cluster shapes the og refuses — both directions closed (the 1.x-3.x version
superset stays: I-004/D-48b). N-013d — the volume open reads the og's 4 KiB
(:2425-2444). N-013a INVALID (first-hand decode: the og's "dynamic body
offset" is 0x50 BY CONSTRUCTION — the ref parser hardcodes the 0x30 extension,
FUN_18000ff60:11875; the audit's `*(uint*)(page+4)` was Ghidra's
parsed-object view, not a raw-page field). N-013c INVALID (the "mid-volume
second copy" is a misattribution: the cited formula is the container-map key
in FUN_180011e90 — N-036's decode, confirmed by the full-function read; the
og reads the superblock at the SINGLE metadata base spc×0x1E, :2457, and
probes no backup; the core's {30,n-3,n-2} set stays the superset —
implementing the audit's "mid-volume probe" would have been hallucinated
parity). Tests: Refs.Bps256Accepted / Bps1024Accepted / Cluster16KRejected /
OpenReadIsFourKiB (all fail-before RED) + Cluster64KAccepted boundary pin.
Suite **748/748**.



## Session-8 lead-lane delta (2026-10-28 — the deep-verification + batch-2 formalization
wave, FINAL reconciliation over THREE concurrent lanes; audited core `ff879f9`, corpus
`be0cfe9`)

**Method.** Five parallel read legs (the W1a/b/c contract verification; the origin-carrier +
batch-1 spot re-verification at the constructor/probe sites; the batch-2 key/grammar decode;
the citation-gap sweep; the thin-module sweep), then lead first-hand verification of every
load-bearing claim.

**The three-window reconciliation.** The push was rejected twice by concurrent lanes
landing first (the deep-audit lane's S-040..S-045/M-071..M-085/N-090..N-106; the fix lane's
CRW/MRW resolution `9e5dedc` + S-041 school-1 `b92ebef`; the third driver lane's
M-086..M-089/N-107/N-108). Merge outcome: **28 findings kept** in the final numbering
(S-047, M-090..M-102, N-111..N-126, I-029/I-030), **13 merged as duplicates** via addenda
(S-041 #2, S-042, N-091, N-100, N-101), **7 school-1 duplicate filings dropped** (CHM/ONE/
MNY/EMF/SWF/3DS landed; FCPEvt closed-as-covered — the lead lane's decodes convergent with
the remote errata E56-E66), **1 distinct erratum kept** (E67, the DMG bound/walk).

**The kept headlines the parallel waves did NOT have:**
- **M-090 (MAJOR)**: the DMG koly-trailer walk + DMG-specific cap divergence — carved DMGs
  truncated before the mandatory UDIF footer; NOT in the parallel batch-1 truth passes
  (erratum E67).
- **M-091 (MAJOR)**: the MDB page-chain size-walk decode — the landed row's "undecoded"
  comment falsifiable; complements the parallel M-084.
- **N-116 (MINOR)**: the session-list row-ORDERING divergence (the og's FILETIME
  time-sort) — a distinct axis from the parallel N-090.
- The **school-2/3 per-format fix units** with full grammars (S-047 + M-092/M-093/M-096/
  M-098/M-099 + N-111/N-112/N-113/N-115) — children of S-041, whose step-1 preconditions
  this wave's decodes resolve (convergent with SCAN-4).
- **M-100..M-102** (browse-inside, the Versions/Tags branches, the thumbnail ABI export),
  **N-119..N-126** (D-88's stale premise; the pointer drifts; the ESP automount; the NVRAM
  create/delete; cli/transport staleness; the drive-disconnect states — R-16 never
  formalized before; the Unsaved File category), **I-029/I-030**.
- The **fixture-regeneration requirement** on S-042's addendum: landed tests must derive
  fixtures from spec, never from the implementation — the class behind S-042/M-081.

**Board after the final merge: 75 active** (7 SEVERE · 29 MAJOR · 34 MINOR · 5 INFO
incl. I-026 — the third lane's 47 + the lead's 28) / 249 FIXED / 1 tracked-open
(OQ-S4-3). **Recommended fix order (the lead lane's additions)**: S-042's test
regeneration + M-090 first (the landed-but-broken pair with the smallest diffs); then
M-091 (the MDB walk); then the S-041 school-2/3 children in the addendum's order; then
the N-116..N-126 doc/registration batch.

**Verification discipline**: every load-bearing site re-read by the lead before filing; no
executed comparisons (read-only container) — recorded per finding; decompile-direct
(tier 2) with binary probes (tier 4) where constants were ambiguous. Consolidated:
`evidence/sa8-context-sheets.md`.

## Session-10 S10-F delta (2026-10-29 — the function-level completeness lane; audited `44f043a5`, corpus `be0cfe9`)

The lane split (declared at open over the parallel SA-10/DFX-38 pushes): this
lane took the mandate's function-level surface — the nine engine function
inventories + the per-engine class-coverage map + the nine ghidra trees'
103-binary disposition census — with every candidate verified first-hand
before filing (the T3a/T3b/T4 read-agents' reports + the lead's re-reads).

**Filed (3 + 1 addendum + 1 process repair):**
- **M-110** (MAJOR) — the og's Btrfs `@eaDir` subtree suppression
  (FUN_180001cd0 :704-711, a 7-byte prefix compare with no length check;
  Synology EA dirs appear EMPTY in the og tree) absent from the core — the
  M-018 divergence class at engine scope; the filter is undocumented in the
  corpus's own artifacts (new decode).
- **N-133** (MINOR) — engine-side volume labels never decoded/surfaced
  (NTFS $Volume $VOLUME_NAME extraction instruction-decisive at
  FUN_18003f3a0 :50963-50980; ext4 s_volume_name per the algorithms-v2
  artifact): the core's ext4 decode is dead code, NTFS/FAT/exFAT decode
  nothing; **M-079's "no image-scan label semantics" premise corrected by
  addendum** (the engines ARE the og's image-scan label source).
- **N-134** (MINOR) — the product-frame register completion: six
  never-dispositioned binaries (CloneModule.dll the clone/PreOS host;
  legacy.dll the LibTomCrypt substrate with the undecoded role;
  EuDownloader.exe; Boot.dll/BootDriver.dll/peformat.dll the media-builder
  family) + the six telemetry binaries name-absent — the 103-binary
  census's only residue.
- **M-079 addendum** — the image-target fix step corrected (engine-side
  label first, filename fallback).
- **M-108→M-109 renumber** — the filename collision from the parallel push
  race repaired per the SA-9 earlier-push-wins rule (preview-text keeps
  M-108; the M-080 residual is M-109; CROSS-REF/evidence mentions updated).

**Verified CLEAN (no findings):** the 25 never-cited-by-name engine classes
across NTFS/FAT/exFAT/Btrfs/EXT4/HFS+ — every one COVERED semantically under
different citation names (the T3a/T3b vtable-level reports; core
equivalents cited file:line); the engine-bearing ghidra trees
(searchdll/mo/partition/drivers/drwui) CLEAN at binary level; the three
infra-only MOM stubs re-confirm the inventory's rollup-covered claim.

**Board after this lane:** 57 active (6 SEVERE · 26 MAJOR · 21 MINOR ·
5 INFO) — the S10-F additions on top of the SA-10 close board; FIXED-LEDGER
unchanged by this lane (no fix verified here; the purge convention holds).

## Session-10 S10-F wave-2 delta (2026-10-29 — the tracked-open resolutions; audited `deea0241`)

Both resolvable tracked-open questions closed this wave:

- **OQ-S8-1 RESOLVED → M-111 (MAJOR)**: the CWindowsFS consumer trace (the
  T11a read-agent + the lead's first-hand verification of every load-bearing
  cite) decoded the og's in-app partition file browser end-to-end — DRWUI's
  browse button → the CreateExplorerPartitionObject export →
  ExplorerPartition::Open → the FileSystem module's vtable+0x38 factory →
  CWindowsFS (live-mounted volumes) / the raw parsers (lost partitions,
  behind the same IFSInterface). User-visible existing-file browsing IS an
  og product surface; the core has no counterpart seam and no scope
  registration — M-111 owns the capability decision. Two internal consumers
  ride the same factory (used-space, root-openability) — recorded in the
  trace evidence.
- **OQ-S10-2 RESOLVED (dead payload)**: the decisive probe executed — zero
  consumers of the " cfs-"/FAN. container across every reversed binary and
  string table; the video-repair/VideoViewer hypotheses refuted; registered
  dead-shipped via N-134's addendum.
- Remaining tracked-open: **OQ-S4-3** (execution-bound, Windows dispatch —
  unchanged) and **OQ-S10-1** (the thumbnail codec-bound formats question —
  the fix lanes' surface).

**Board after wave 2:** 58 active (6 SEVERE · 27 MAJOR · 21 MINOR · 5 INFO)
+ the two OQ closures in the ledger.

# SUMMARY.md — executive summary (the S57 severe-first fix lane's convergence records on top; the prior lanes below, newest-first)

## Session-57 S57 fix-lane delta (2026-10-29 — the user's severe-first directive; core `fa654673` at open, corpus `be0cfe9`; the lane seat rebranded S47-draft → S57 across the S45..S56 hot window per SA-9)

**A-014 CONVERGENT (the ACUTE tier is CLOSED at the merged tree):** this lane's
independent verification + fix collided with the S45 lane's winning landing
`862d79ef` (byte-equivalent gate: the identical DosDateTimeToFileTime reject set on
the shared converter). This lane's unique evidence preserved as the finding file's
Convergence addendum II: the RAW-DISASSEMBLY verification of the corpus binary itself
(the four KERNEL32!DosDateTimeToFileTime call sites at exact addresses — IAT
0x18004d088; the three dentry-stamp sites 0x1800289d1/:a1f/:a51, each carrying the
`test eax,eax; jne <skip>; mov rdi,[slot]` zero-on-fail tail; the fourth site
0x180032c38 outside the dentry path) — the strongest verification grade for the
finding's central claim, confirming the decompiled quote against the shipped binary.
The independent 6-test RED battery reproduced the fabrication on pre-fix main before
the pull; the redundant tests were DROPPED per the superset rule (the merged S45+S51
battery covers the full boundary set incl. the per-slot zeroing shape). **Board at
the merged tree (filesystem truth, this lane's re-count): 0 ACUTE · 9 SEVERE · 25
MAJOR · 30 MINOR · 0 INFO active (the S48 INFO retirements closed the tier) /
77 FIXED files / 6 tracked-open (OQ-S42-2's Btrfs-nsec decode landed; its finding
file's retirement pending its own lane's trail sweep).**

# SUMMARY.md — executive summary (the S56 ACUTE-first fix lane on top; the prior lanes below, newest-first)

**S-065 FIXED (SEVERE — the ext4 indirect-ladder unbounded freeze/OOM):** the
ladder is now bounded by ceil(i_size/block_size) exactly as the og
(FUN_180005490 :3952-3954 re-verified first-hand), with the
physically-contiguous merge (:3986-3996), a 65536-extent hardening budget,
and the cancel token consulted at every indirection block (Volume gains
`cancel`). 4 tests RED->GREEN: PoisonDoubleIndirectBoundedBySize (273 vs 15
reads), PoisonTripleIndirectBoundedBySize (3000-cap blown vs 15),
StaleIndirectCancelResponsive (257 vs ≤1 poison reads),
ContiguousPhysicalBlocksMergedToSingleExtent (4 vs 1). Ext4 family 42/42;
suite 1444/1444; win gate 10/10; format clean. Trail: findings/FIXED/S-065 +
ledger row + ext4.md cell -> MATCH + matrix row + this block.

**S-063 FIXED (SEVERE — the NTFS torn-$MFT bootstrap):** the record-0 read
is retried at the $MFTMirr LCN exactly as the og's walk entry does
(FUN_1800272b0's LAB_180027467, re-verified first-hand; both failing = the
0x610016 refusal class); `boot_from_mftmirr` lands as the open diagnostic
+ an Info log line; the wrong "nothing to mirror FROM" comment re-truthed.
3 tests RED->GREEN + the preserved-refusal control; the RawFixer
MftRestore pin re-truthed to the boot-source axis (mirror -> repair ->
undo); suite 1453/1453. Trail: findings/FIXED/S-063 + ledger row + ntfs.md
+ the matrix MATCH cell.

**Hot-window repairs this window:** the S-060/S-058 window residue (win-stub
grown for the metadata tail, the grading test's nested markers collapsed, the
preview leak closed, the _TRUNCATE format drift) + the S53's convergent
markers. Suite green throughout.

**S-040 FIXED (SEVERE — the image-create producer, this lane's centerpiece):**
the producer half the core wholly lacked: portable
`storage::create_image_stream` (64 KiB envelope reads, SHA-256 over the
stream, cancel at chunk heads, committed-prefix stats) + win-layer
`win::create_image_file` (atomic `.crimg-tmp`→rename commit + the
`.sha256` sidecar + the explicit kept `.partial` on cancel/failure —
reported, never a silent success) + `win::create_disk_image` (class (a)
over open_disk) + ABI `cr_ui_image_create` (87 exports, contract 7j,
the ERR_DRIVER_* gate, honest ERR_UNSUPPORTED off-Windows) + CLI verb 13
`image --create` (four-site lock; exit 3/2 with the kept-.partial
reporting). Flat raw only — VHD/VHDX writers out of scope, registered
D-147. 7 portable tests + 4 win-lane disposition tests + C-consumer
rows; R-93 PARTIAL→PRESENT; a Windows-lane probe added to
Run-Benchmark.ps1 (create → sidecar digest == Get-FileHash → size ==
disk size → scan --image exit 0). Board after this push: 45 active —
0 SEVERE · 23 MAJOR · 20 MINOR · 0 INFO — + 4 tracked-open; FIXED 141.
