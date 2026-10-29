# SUMMARY.md — executive summary (the S84 fix lane on top; the prior lanes below, newest-first)

## Session-84 S84 fix-lane delta (2026-10-29/30 — the user's fix mandate continuation ["fix all the audit findings (priority to major/informational first) one by one... Aim for atleast 20+ fixes in this session, dont stop before that"]; opened at core `842a587e` (the S83-close tree), corpus `be0cfe9` throughout)

- **N-219 FIXED (MINOR, NEW — filed+fixed at the open, the red-main
  repair):** main was RED at this session's open — the M-151 landing
  `27bda40e` committed its rebase resolution against a garbage-collected
  lane SHA (`9647e8b0`) with live conflict markers in
  `tests/unit/test_ntfs_volume.cpp` (two regions) and
  `diff-to-fix/CROSS-REF-MATRIX.md` (one region) — the M-120/N-210/N-217
  class's FOURTH recurrence, and the second to reach main red. gcc
  refuses the markers under -Werror (`version control conflict marker in
  file` at :5780), so cr_tests could not build. The repair recovered
  BOTH sides' content from the hunks themselves (the A-016 test
  byte-identical to `e2ea4d50:5764-5829`; the M-151 four-test battery +
  helper namespace restored; the matrix taking each parameter's later
  final state — M-151 FIXED from the landing lane, N-182 option-2 from
  main's own `868d1e85`). RED first-hand (build exit 2, 0 tests
  runnable) → GREEN: **full suite 1648/1648** (114 s), repo-wide marker
  sweep clean.

**Board after the N-219 repair and the racing window's absorption
(filesystem truth): 8 MAJOR (M-075 M-078 M-086 M-087 M-089 M-101 M-105
M-107) · 2 MINOR (N-107 N-127) · 0 INFO active / 198 FIXED /
4 tracked-open.** The racing lanes' same-window landings absorbed into
the board this lane re-verified: I-048 (ACKNOWLEDGED, D-160/D-161/D-162),
M-111 (the D-158 scope decision), M-114 (the D-159 posture decision),
M-154 (retention + pointer + the S82 hardening rider), N-181 (the
RAW-first reorder), M-107's CRW member (the CIFF heap walk, member 2 of
11). Of the open MAJORs, M-086/M-087/M-089 and the MINORs N-107/N-127
are driver-repo-resident (the OQ-S10-3 boundary — outside the two-repo
grant). Next free IDs: S-069+, M-157+, N-220+, I-049+, A-017+,
D-163+.

---

# SUMMARY.md — executive summary (the S79 MAJOR-first fix lane on top — the M-073 seat conceded to the racing window per SA-9 and re-verified; the prior lanes below, newest-first)

## Session-79 S79 fix-lane delta (2026-10-29 — the user's fix mandate continuation ["fix all the audit findings (priority to major first) one by one... Aim for atleast 20+ fixes in this session, dont stop before that"]; opened at core `e17a4e6`, corpus `be0cfe9` throughout; baseline suite green re-run first-hand at the open [1484/1442/0]; this lane's own M-073 implementation was CONCEDED to the racing window's landing at the push rebase per SA-9 — the seat's `cr_folderfilter` landing verified first-hand instead, and the verification CAUGHT TWO REAL DEFECTS, filed + fixed this session)

- **A-016 FIXED (ACUTE, NEW — filed+fixed this session at the convergent
  verification)** — the M-073 landing's folder-filter path walk treated
  the NTFS volume-root "." node (MFT record 5, $FILE_NAME ".", reparented
  to the Found root) as a path SEGMENT: every caller prefix turned
  incompatible, the "." directory detached WITH the whole volume, and a
  filtered NTFS scan returned an EMPTY tree with near-zero counters —
  wrong recovery results on the dominant Windows filesystem, invisible to
  the landing's FAT-only test battery (the FAT engines emit no "." node).
  The fix: the transparency rule in `prune_children` — a directory whose
  key is "." recurses with the PARENT's path (never a segment, never
  evaluated, never detached; "." is a forbidden user name on Windows so
  the rule cannot misfire). RED first-hand at `5291076`
  (`NtfsVolume.FolderFilterRootDotIsTransparent`: kept_in_scope 0) →
  GREEN (readme.txt/deleted.dat kept through the "." node, the $-meta
  siblings correctly pruned, a partial prune never the whole volume); the
  landing's 13 FolderFilter tests stay green.

- **N-218 FIXED (MINOR, NEW — filed+fixed this session)** — both detach
  paths (`detach_child` — the landing's primitive — and
  `detach_subtree`) subtracted FullNode intermediates from `live_files_`,
  which only `add_file` ever ticks (N-166's documented contract): a
  detached subtree holding an intermediate folder (the session-loaded
  marker=2 shape under the prunable roots) WRAPPED the unsigned counter
  (18446744073709551615 in the reproduction), corrupting the mid-stage
  telemetry samples. Both paths now subtract non-full nodes only.
  RED (`NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles`) → GREEN
  (0, node_count 1).

- **M-073 CONCEDED-VERIFIED (the seat)** — this lane's convergent
  `cr_scopefilter` implementation was dropped at the rebase per SA-9
  (the racing window's `cr_folderfilter` landing took the seat); the
  landing's corpus ground (DRWUI :567668-567690, UIUX/31:142) re-read
  and its 13 tests + the full suite verified first-hand on the merged
  tree — the verification is what surfaced A-016/N-218 above. The
  lane's trail block is recorded at the finding file's history.

**Board after this landing (filesystem truth): 0 ACUTE · 1 SEVERE (S-068)
· 17 MAJOR · 10 MINOR · 1 INFO active / 176 FIXED / 4 tracked-open.**
(A-016 and N-218 were both filed AND fixed this session — they land on the
FIXED side directly; the 10 active MINORs are the S81 board's set.)
Next free IDs: S-069+, M-156+, N-219+, I-049+, A-017+, D-151+.

---

# SUMMARY.md — executive summary (the S83 fix lane on top — the M-156 X3F correction; the prior lanes below, newest-first)

## Session-83 S83 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first", 20+ fixes]; opened at core `2d614cf2` (the S82-close tree: the racing lanes had repaired the red main, landed S-067, M-073, N-211, N-214, N-215 and the X3F four-lane arbitration while this lane's first composite was being built — the push rejection on the moved remote was the save; the composite was withdrawn unpushed per the S82 lane's own precedent), corpus `be0cfe9` throughout)

- **M-156 (NEW, filed+fixed this session — the X3F surface's FIFTH
  touch):** the four-lane arbitration left TWO defects standing. (1)
  **The live bug:** the landed ASCII version gate rejects every real
  Sigma X3F file — real files carry the BINARY u32 version 0x00020001,
  the gate demanded an ASCII digit + point; real RAWs never carved at
  all. The og's chain never jointly rejects a readable FOVb (the modern
  tier-1 FUN_180040080 accepts the binary shape; the legacy tier-1
  FUN_180053590 accepts any readable header). (2) **The refuted
  mechanism:** N-217's dispatcher claim ("vtbl[+0x40] consults the
  secondary FIRST — the Brance count-walk never contributes") does not
  fire for the X3F node: the consult requires the node class to
  override +0x40 with the real getter (FUN_180013700), which exactly 8
  node classes do — the X3F node AND the DMG node (the M-090 precedent
  the lanes leaned on) both carry the return-0 STUB at +0x40,
  byte-exact from .rdata. The magic-key path provably wires the MODERN
  count-walk for binary versions (the node's binary gate → the factory
  → the 0x120 walker with the 100 MiB cap → the +0x20 feeding state
  machine → extent = SECd + 16 + count*12). Evidence:
  `evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md`
  (instruction-address cites against the corpus binary — the prior
  lanes' Ghidra line cites are CI-generated and unverifiable from the
  corpus).

- **The landing:** the version gate is GONE (the union acceptance); the
  version shape routes the generation (binary → the modern count-table
  walk; anything else → the legacy fixed-tail walk, the four-lane
  convergent semantics preserved; torn → the legacy salvage); the row
  bound takes the og walker's own cap constant (0x6400000 = 100 MiB).
  Tests: the five legacy-path pins unchanged + five new pins incl.
  `X3fRealSigmaBinaryVersionCarves` (RED at the open tree — the
  headline pin — and GREEN post-fix) and `X3fChainRoutesByVersionShape`
  (the router itself). The X3F battery 10/10; the full suite green.
  Honest residue recorded in M-156: the +0x20 feed-loop caller and the
  registry container merge are undecoded — the binary-with-directory
  extent (count-walk vs legacy constant) disagreement is RECORDED with
  its decisive next probe, not papered over.

- **M-152 FIXED (MAJOR, ext4 — this lane's second landing):** the
  orphan-reserved skip is GONE. Corpus re-verified first-hand from the
  raw asm (the filter at 0x180005f5d-0x180005f87: four content gates,
  NO dtime/links read); the orphan-active population (unreachable-but-
  intact inodes — torn parents, unlinks without link decrements) now
  surfaces under the deleted root with its extents, the honest Active
  state, and the new orphan_inodes counter. The live-claim dedup and the
  mode-gate hardening pinned; the journal-inode claim registered
  (PARITY-DELTAS D-152). RED→GREEN first-hand via the skip-line
  reintroduction. Suite 49/49 ext4 + full suite green.

- **M-107's CRW member LANDED (member 2 of 11 — this lane's third
  landing):** the CIFF heap size walk, raw-asm decoded first-hand
  (evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md): the CRW
  child walker searches zero-tailed sectors for CIFF directory entries
  {u16 0x2807, u32 len ∈ {0xa8→+0x46, 0xb2→+0x3c, 0xe4→+0x32}} with the
  walker's OWN 50 MiB cap; no entry → the cap salvage. validate_crw
  replaces the blind 64 MiB stand-in; both registry rows carry the og's
  cap constant. RED→GREEN via the stash dance; the carve suite 174/174.

**Board after this landing: 0 ACUTE · 0 SEVERE · 15 MAJOR · 10 MINOR ·
1 INFO active / 177 FIXED (M-156 + M-152 [convergent, conceded] + M-107's
X3F and CRW members this lane; the racing window's S-067/S-068/M-073/
M-113/M-149/M-150/N-211/N-213/N-214/N-215/N-216/N-182/A-016/N-218/M-151/
N-178 absorbed) / 4 tracked-open.** Next free IDs: S-069+, M-157+,
N-219+, I-049+, A-017+, D-151+.

# SUMMARY.md — executive summary (the S80 parity fix lane on top; the prior lanes below, newest-first)

## Session-80 S80 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first", 20+ fixes]; opened at core `04fb5e88` — main was RED at open; the racing M-155 lane `6d74ec2c` + the S81 lane `9508b90b`/`567694e7`/`3dfb2a5a`/`59a431d3` landed mid-session and were absorbed at the rebases; corpus `be0cfe9` throughout)

- **N-217 FIXED (the red-main repair, convergent with M-155)** — the ed501caa
  push's committed conflict markers (the M-120/N-210 class's third recurrence,
  plus a false "suite 1570/1570" claim). This lane's decisive contribution: the
  SCAN-DISPATCHER decode (RAWSearchDLL :9850-9876, :9900-9935) — the secondary
  (File*Analyser) is called FIRST with the result object, the Brance primary
  is the 2-arg second-chance; M-155's X3F resolution (the S71 FileX3FAnalyser
  walk, SECd + 0x4C, the count never read) confirmed by dispatch evidence
  rather than precedent. The registry census's "modern first" prose recorded
  as an erratum (the M-107 Session-80 addendum). This lane's own initial
  "unified chain" hypothesis was corrected by the decode before any code
  seesaw.

- **S-067 FIXED (SEVERE — the exFAT chain walker's out-of-range FAT-entry
  clamp)** — `ExfatFat::next()` clamps every entry ≥ cluster_count to EOC
  (the og's walk-loop clamp at exFATSearchDLL:41348-41351; the count's
  provenance: volume-context +0x18 = the boot +0x5C field verbatim,
  RAWSearchDLL:36750 — both re-verified first-hand); the loop gains the og's
  `c < 0xFFFFFFF6` bound (:41315). Tests: the finding's 3-cluster mid-chain
  fixture shape, RED 3/3 → GREEN 3/3 via the stash dance
  (OutOfRangeFatEntryEndsChainCleanly / ExactClusterCountEntryClampsToEoc /
  ReservedFFF6EntryEndsChainCleanly); the EXFAT-2/EXFAT-3 families re-pinned;
  exFAT suite 44/44. The damaged-FAT semantics family is now three-for-three.

- **S-068 FIXED (SEVERE, fat — the SEVERE tier closes; the S80
  diff-to-fix lane)** — the og's deleted-FAT32 high-16 re-base: the
  scan-global last-good first-cluster hint (DAT_1800728c8, :13753-13758)
  + the pre-walk low-16 re-base of torn-high-word deleted dentries
  (`hint_high | dentry_low16`, the < 0x10000 && < hint gates,
  :14753-14755) + the repaired-pair emission (the :14840-14842
  write-back semantics). The adoption criterion is the og's
  walk-confirmation shape: the re-based cluster's own FAT entry must be
  live — FREE never adopts (the raw walk decides, GAP-S6-2 intact on
  the raw path). The assumed provenance (never Excellent) +
  FatScanStats.high16_rebiased surface the population. Tests RED
  first-hand (stash dance): DeletedHigh16ZeroRebasesFromHint /
  DeletedHigh16RebaseFallsBackToRawWalk /
  LastGoodHintUpdatesOnlyOnFileWalks / FAT16NoRebase; suite 1604/1604.
  Trail: findings/FIXED/S-068 + the S80 verification addendum, matrix
  MATCH, modules/fat.md S80 addendum.

- **M-149 FIXED (MAJOR, lost-partition)** — the ghost-score two-pass
  form restored (the M-017 convergence debt): the +500 table-match
  bonus is NUMERATOR-only (the og's max loop :842210-842240 has no
  bonus walk), the count==0 edge is the og's 0.0 floor
  (CalcPartition.log's zero row), and a matched leader legitimately
  exceeds 1000 permille (the og's raw double >1.0 — PARITY-DELTAS
  D-151 registers the representation). Seven stale empty-candidate
  pins re-truthed to the zero floor (the finding's own inflation
  class). Tests RED first-hand (stash dance):
  ScoringTableMatchBonusIsNumeratorOnly / ScoringZeroCountScoresZero /
  the MatchesLogRows re-pin; suite **1624/1624**. Trail:
  findings/FIXED/M-149, matrix MATCH, modules/lost-partition.md S80
  addendum.

**Board after this landing: 0 ACUTE · 0 SEVERE · 13 MAJOR · 10 MINOR ·
1 INFO active / 179 FIXED / 4 tracked-open.** Next free IDs:
S-069+, M-156+, N-218+, I-049+, A-016+, D-150+.

# SUMMARY.md — executive summary (the S81 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-81 S81 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to minor first", 20+ fixes, fixed findings into findings/FIXED/]; opened at core `04fb5e88`, corpus `be0cfe9` throughout; the S81 lane executed the identical M-155 repair independently before pulling — the convergence addendum in the M-155 finding records it; the seat conceded to the racing `6d74ec2c` per SA-9; this block re-landed after the N-217 lane's rebase dropped the S80-parallel blocks)

- **N-211 FIXED (MINOR, orchestrator/api-docs):** the `autosave_dir`
  contract comments re-truthed at BOTH headers — `cr_scan.h`'s "a
  cancelled run never auto-saves" and the twin at `cr_ui.h:636` the new
  gate caught (the shim's own comment, whose code has stop-window-saved
  since S-057). The og truth now stated: the field fires on completion
  AND on the stop window (DRWUI :1549399-421 → FUN_140395e50, gates
  :612571-587 no stopped-check — corpus re-verified this session). The
  guard: `doc_truth_gate.py`'s `check_header_contract_phrases()` — RED
  pre-fix (the cr_ui.h catch), GREEN post-fix.

**Board (filesystem truth at this push): 0 ACUTE · 2 SEVERE · 17 MAJOR ·
10 MINOR · 1 INFO active / 173 FIXED / 4 tracked-open.** Next free IDs:
M-156+, N-218+, I-049+, A-016+, D-150+.
# SUMMARY.md — executive summary (the S80 fix lane on top; the prior lanes below, newest-first)

## Session-80 S80 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to severe first, one by one, 20+ fixes"; opened at core `04fb5e88` — RED at open, see M-155; corpus `be0cfe9` throughout])

- **M-155 FIXED (MAJOR, NEW — filed+fixed this session)** — **main was
  RED at open**: the `ed501caa` push (the S71 X3F landing) committed
  unresolved merge conflict markers in `src/core/carve/cr_analysers.cpp`
  (`validate_x3f`) and in the M-107 finding file, with a false
  "suite 1570/1570" claim in the commit message (the tree cannot
  compile; the N-210 gate was not run before that push). Resolved
  corpus-first: FUN_1800535b0 (FileX3FAnalyser) re-read in full —
  SECd + 0x4C in all three found-branches, loop-exit salvage, the count
  never read; the M-090 DMG precedent establishes the File*Analyser
  secondary as the size-walk authority (the S78 lane's
  FUN_1800404a0 count-walk decode — the Brance streaming callback —
  superseded). The S71 body's own coordinate bug fixed (the absolute
  probe limit mixed with relative offsets: reads past the device end on
  small volumes fell to the torn path even with SECd in range —
  `Carve.X3fExtentIsSecdPlusFixedTail` RED against the ed501caa body).
  The S78 test trio replaced by corpus-true pins. Suite **1578/1578**
  at the repair; N-210 lint clean.

- **M-150 FIXED (MAJOR, session)** — the count pre-allocation bomb
  gate: deserialize_tree's count field is bounded by the wire length
  (count > (len-4)/10 refused instantly with ERR_SESSION_FORMAT — each
  node consumes ≥ 10 wire bytes; the og's per-node factory reader is
  bounded by the bytes present, rsf-record4:16,26). No count-scaled
  allocation ever happens on the load path — the old value-initialized
  by_id(count) touched up to 34.4 GB at the u32 max (the M-131-class
  torn-write read-side complement: pagefile thrash on Windows,
  OOM-kill on Linux overcommit). Tests RED first-hand (stash dance):
  HostileCountBombRefused / CountAtWireBoundFailsCleanlyBeyondIt /
  TornTailGarbageCountStillFailsCleanly; suite **1627/1627**. Trail:
  findings/FIXED/M-150, matrix MATCH, modules/session.md S80 addendum.

- **M-151 FIXED (MAJOR, ntfs)** — the truncated-$MFT run-growth probe:
  per run, 4 KiB windows probed past the declared end, consecutive
  records validated (FILE magic + fixup_validate with N-213's pre-gate
  inside), the run length grown by the validated bytes (cluster floor);
  termination per the og (first invalid / failed read / next run's
  start / device end). A runlist that LOADS but under-declares has its
  tail annexed — the recently-written MFT-tail records become walkable.
  The mirror population unaffected (the S-063 fixtures' holes made
  physical, re-pinned). Tests RED first-hand (stash dance):
  TruncatedRunlistGrowsToValidRecords / RunGrowthPartialWindowCounts
  ValidPrefix; + RunGrowthStopsAtFirstInvalidRecord /
  MirrorFallbackUnaffectedByGrowthPass; NTFS suite 95/95; suite
  **1634/1634**. Trail: findings/FIXED/M-151, matrix MATCH,
  modules/ntfs.md S80 addendum.

- **M-154 FIXED (MAJOR, session/orchestrator)** — the auto-session
  lifecycle: retention-one (the previous auto session named by the
  pointer is deleted before every auto-save — both autosave bands; the
  unbounded accumulation ends), the pointer (session::LastScanInfo with
  the og's [Info] four fields, the last_scan.info sidecar —
  PARITY-DELTAS D-152 registers the location convention; the HKLM
  mirror dropped), and the pointed resume
  (cr_ui_session_last_autosave_path, PathFileExistsA-validated). The
  auto_-name retention guard = the registered safety superset. Tests:
  AutosaveRetentionDeletesPreviousAndPointerNamesIt (RED: BUILD-absent)
  / ManualSaveSurvivesRetention / DeadPointerDoesNotBlock + the C
  consumer check; suite **1651/1651**. Trail: findings/FIXED/M-154,
  matrix MATCH, modules/session.md S80 addendum, PARITY-DELTAS D-152.

**Board after this landing: 0 ACUTE · 2 SEVERE · 18 MAJOR · 11 MINOR ·
1 INFO active / 170 FIXED / 4 tracked-open** (M-155 filed+fixed same
session; the +1 MAJOR is M-155 itself, closed on filing).


# SUMMARY.md — executive summary (the S69 continuation audit lane on top; the prior lanes below, newest-first)

## Session-82 S82 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first, 20+ fixes, one finding / one verified fix / one push"]; opened at core `04fb5e88` on the RED tree, re-based at `d37840c4` after the racing lanes' X3F/S-067/N-211/N-215 landings were absorbed; corpus `be0cfe9` throughout)

- **The X3F arbitration conceded after first-hand dispatcher
  verification:** this lane initially reached the OPPOSITE (composite,
  modern-primary-first) arbitration from the census artifact's protocol
  prose and its own reads of both walks; the racing lanes' dispatcher
  decode (:9849-9876, :9900-9935) was re-verified directly in the corpus
  before conceding — the secondary File*Analyser runs FIRST with the
  result object, and FUN_1800535b0's never-decline stamp means the
  Brance count-walk never contributes to a carved X3F extent. The
  wrong-order composite was withdrawn pre-push (the push rejection was
  the save); the convergence record + the census-prose erratum
  confirmation are in N-217's S82 addendum (findings/FIXED/). No
  code change by this lane on the X3F surface.
- **S-068 — CONCEDED to the racing `ad1bcb9e` (the third independent
  implementation of the same finding this window):** this lane's variant
  (the walked-END hint, the 0x310000-gated re-base, the contiguous
  completion at the re-based anchor, four tests RED->GREEN on a sparse
  0x320000-cluster fixture) was withdrawn pre-push per SA-9; the S82
  convergence addendum in findings/FIXED/S-068 records the two open
  decode deltas (the hint's value; the 0x310000 gate's placement) with
  their one-volume synthetic discriminators for the next audit. The
  SEVERE tier is closed either way.
- **The lane's remaining queue (the user's order):** the MAJOR tier next
  (M-149 the scoring fold, M-150 the count bomb, M-152 the ext4 orphan
  skip, M-151 the NTFS run growth, M-154 the autosave retention, M-107's
  remaining members [CRW next], M-075/M-078/M-101/M-105/M-111/M-113/
  M-114), then the MINOR queue (N-216/N-145/N-178/N-181/N-182), then
  I-048's registers — one finding, one verified fix, one push. Already
  landed by the racing lanes this window (not re-fixed here): S-067,
  S-068, N-211, N-213, N-214, N-215, M-073, X3F.

## Session-69 S69 lane delta (2026-10-29 — the NINTH continuation audit lane of the user's second-session mandate [remove-the-fixed + go-deeper + mark-all-remaining, focus: sector numbers / scanning / accurate detection / calculation / mathematics / no stuck or freeze / local database / state management]; the S68 seat conceded to the racing citation-census lane per SA-9 — this lane executed the S61 seat's never-landed plan, a disjoint slice; opened at core `3799dee2`, findings verified there and re-verified LIVE at the rebase head `271f5d06` after absorbing the S71..S79 fix windows [M-118/M-135/M-136/M-143/N-140/N-212/N-131/N-200/N-210/OQ-S52-1 landed by the racing lanes — zero of this lane's filings collided], corpus `be0cfe9` throughout)

- **The liveness battery (28/28 at the open pin):** every then-active
  finding's core-side claim re-verified first-hand — 23 core-side STILL +
  5 driver-repo ACTIVE-OPEN (OQ-S10-3); zero falsely-active, zero
  phantom-fixed. The racing fix lanes retired M-118/M-135/M-136/M-143/
  N-140/N-200 + OQ-S52-1 in the same window — all correctly in FIXED/.
  Evidence: `evidence/s69-lane-sheets/liveness-battery.md`.

- **Four new findings + one register filed** (all corpus cites read
  first-hand by the lead; all core cites re-verified LIVE at `271f5d06`):
  **M-154** (MAJOR — the auto-session lifecycle: the og deletes the
  previous auto .rsf via the LastScanInfo.ini pointer before writing the
  new one [DRWUI:442815-442833], keeps exactly one live auto session,
  writes the 4-field pointer + an HKLM `SOFTWARE\EASEUS\DRW` mirror
  [:443060-443086], resumes pointed [:443092-443127]; the core never
  prunes — unbounded accumulation, no pointer); **N-214** (MINOR — the
  lost-partition ext probe's journal-indicator XOR gate: PRD:1273-1293
  rejects torn SBs with exactly one journal indicator set; core's
  probe_ext accepts them); **N-215** (MINOR — R-20's mechanism
  mis-attribution: the og's "Unimportant files" bucket is the
  usefullostfiles.xml extension gate at tree-build [DRWUI:635764-635910],
  not the M-018 junk-name filter); **N-216** (MINOR — version.ini's
  `MaxFileSize ` cap surface [GiB units, default 1 TiB, two config
  objects, consumer undecoded] has no core counterpart);
  **I-048** (INFO — three registers: FileFilter.xml Selected=2 dead
  data; the remote-tunable summary-family groups; the og's 32-bit
  lost-partition ext size vs its own engine's 64-bit).

- **Two addenda to active findings:** M-151 gains the truncated-$MFT
  family's INITIAL-bound site (FUN_18000ecb0, NTFS:11274-11277 — the
  device-end-derived record count when no declared size exists);
  N-182 gains the deep-range resolver's full first-hand decode
  (FUN_14035a6d0, DRWUI:575024-575170 — the <300 MB rule is a backward
  extension to LBA 0; the neighbor extension is bounded to ONE following
  FAT16-family entry; the device-type-6 open question narrowed to
  CDiskDeviceType's +0x20 field, data-driven, ctor default 4).

- **MATCH confirmations:** the FAT DOS-timestamp invalid class (A-014,
  re-verified against FAT:32135/:32144/:32151 first-hand); the auto-session
  naming (N-061, byte-parity against all three shipped samples); the
  RawConfig gating semantics; the FileFilter membership data; the autosave
  trigger gates (S-044/S-057); the racing lanes' OQ-S52-1 landing verified
  LIVE. The prior-coverage spot checks (S55-D1's EUUserData cites, M-129's
  gates, the log-channel dispositions) all re-confirmed.

- **The not-reached register (honest):** the engine-side cancel-pause poll
  census and the quick→advanced transition trigger were not decoded (the
  subagent infrastructure timed out twice; the lead re-scoped to the
  resolver decode + the three deep dives). The MaxFileSize consumer and
  the APPDATA subfolder constant remain undecoded (N-216's open
  questions). No UNVERIFIED finding was left untracked: every candidate
  either filed with evidence, registered INFO, or carried in an active
  finding's open questions.

**Board at this lane's filing: 32 active — 2 SEVERE (S-067/S-068, the
racing S79 lane's) · 17 MAJOR (16 racing + M-154) · 12 MINOR (9 racing +
N-214/N-215/N-216) · 1 INFO (I-048) / 169 FIXED / 4 tracked-open
(OQ-S79-1 new).** Next free IDs: S-069+ (unused — this lane filed none),
M-155+, N-217+, I-049+, A-016+, D-150+.

# SUMMARY.md — executive summary (the S78 MINOR-tail fix lane on top [re-homed s70->s76->s77->s78 per SA-9: the racing windows took S70-S77]; the prior lanes below, newest-first)

# SUMMARY.md — executive summary (the S79 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-79 S79 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first, 20+ fixes"]; opened at core `e17a4e6d`, corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; baseline suite green first-hand [1581/1581 post-M-073]; the lane's board re-truth at open: 12 MAJOR / 7 MINOR active [the S78 block's "8 MINOR" was an off-by-one against the disk] / 168 FIXED / 4 tracked-open, of which M-086/M-087/M-089 + N-107/N-127 are driver-repo-resident per OQ-S10-3 and out of this repo's scope)

- **M-073 FIXED (MAJOR, orchestration/query/ABI/CLI)** — the scan-time
  folder filter (the og's PushFolderFilter, UIUX/31:142 re-verified
  first-hand): ScanConfig.filter_folders + the cr_folderfilter prune at
  every stage boundary BEFORE the checkpoint emit (out-of-prefix folders
  never reach a session/checkpoint/snapshot/query/recovery; counters
  decrement to the pruned tree) + ResultTree::detach_child + the ABI
  append + the CLI --filter-folder + the REC_SCAN_FILTER round-trip.
  Carve/pathless roots exempt (the registered scoping decision); composes
  with M-018's junk pass. 15 tests RED (wiring-disabled) -> GREEN.

- **M-113 FIXED (MAJOR, boot-repair/reghive)** — the recovered-OS
  write-side surgery landed core-side: reghive::HiveEditor (the offline
  hive VALUE editor - rewrite-never-create, inline/cell-reuse/new-hbin
  semantics, seq+checksum discipline) + reghive::surgery
  (rewrite_mounted_devices [12-byte DosDevices family, no-partial-write
  guarantee], repoint_bcd_device_element [the {9dea862c} 23000003 walk;
  REG_SZ wide bytes - the finding's REG_BINARY corrected by the decode],
  the bootstat contract [offset 10, byte 1]). Two finding annotations
  corrected by the first-hand decode (the BCD write type; the
  SOFTWARE-hive leg is a READ). R-69 flipped PRESENT->PARTIAL (truthful).
  11 tests RED (compile-error class) -> GREEN; suite 1607/1607.

**Board after this landing (reconciled over the racing S69 audit lane's
filings + the N-144/X3F landings absorbed at the push-rebase): 0 ACUTE ·
2 SEVERE · 15 MAJOR · 12 MINOR · 1 INFO active / 171 FIXED / 4
tracked-open** (M-086/M-087/M-089 + N-107/N-127 driver-repo-resident per
OQ-S10-3). Next free IDs: S-068+, M-155+, N-217+, I-049+, A-016+, D-150+.


