# modules/ntfs.md — NTFS engine audit verdicts

Reference: NTFSSearchDLL (+ NTFSLib mirror handling) per corpus artifacts
`SearchDLL/ntfs-engine--deep-dive.md`, `ntfs-engine-internals.md`,
`ntfs-engine-algorithms-v2--deleted-viability.md`, `zfs-vdev--mftmirr--calcpartition.md` §2,
`CORRECTIONS.md` rows 1/2/7; decompiled evidence in
`evidence/decompiled-quotes/ntfs-viability-and-filters.md`.

Core (audited commit `67193b6`): `src/core/ntfs/` (6 files, 2,311 LOC) +
`include/cr/ntfs/` (7 headers) + `tests/unit/test_ntfs_volume.cpp` (1,726 LOC).

## Behavioral

- **B1 detection correctness** — `M` on magics/DBR gates (FILE 0x454C4946,
  OEM "NTFS    ", 0x55AA, mft/mftmirr LCN bounds); **M-001** (record-size
  reject vs clamp — pending fix); result-set membership: `M` post-S-001-fix
  (overwritten deleted destroyed, og parity; sparse runs excluded from the
  reuse ratio), **I-001** (known-metafile list vs blanket `$`-skip —
  deliberate, pending acknowledgement).
- **B2 recovery correctness** — `M` runlist codec (nibble semantics verified
  against decompile Q5; core documents the corpus prose misread), fixup
  (two-tail tolerance replicated from og loop bound `local_18 < 2`),
  compressed $DATA expansion (LZNT1 envelope → codec-tagged extents,
  zero-fill on inconsistency — never wrong bytes), resident content capture.
- **B3 false-positive rate** — `M` (viability-graded emission; no unguarded
  inference in this engine; inferred-chain honesty caps live in FAT module).
- **B4 deleted-file classification** — `M` post-S-001-fix (destroy rule in:
  fully-overwritten deleted files dropped at walk time, og parity
  FUN_18001acc0 :21249-21262; `WalkerFlags::destroy_fully_overwritten`
  default ON, honest-mode OFF pins the old keep-and-grade; folders never
  destroyed; `stats.destroyed_overwritten` telemetry). Strict `0x3ff < size`
  gate confirmed both sides (core `viability_min_size=0x3FF`,
  `cr_mftwalk.h:45`, strict `>` at `cr_mftwalk.cpp:668`). Secondary claim
  map **OQ-T1 RESOLVED**: same-direction (re-allocated counter), ctor-NULLed
  at bitmap+0x60, never assigned in the entire decompile — dormant.
- **B5 orphan reconstruction** — `M` phase-B reparenting to root_lost for
  unresolved parents (both deleted and active orphans), og equivalent
  documented as "lost tree reparenting" (internals §Record processing).
- **B6 post-format/remnant** — `M` 4 MiB remnant blocks, 'FILE' prefilter,
  shifted-header salvage at +0x58/+0x60 (og FUN_18002a780 parity), dedup vs
  structured results, skip-forward on unreadable blocks. Note (no finding):
  prefilter stride hardcoded 512 (`cr_mftwalk.cpp:1113`) — harmless for
  detection on 4Kn (4096 ⊂ 512-multiples), wasted probes only.
- **B7 hard-link/ADS/sparse** — hard-link emission `MATCH` as of M-007
  (default single_link_per_record: ONE node per record, the namespace-best
  name under its parent — the og's decoded model; the per-parent fan-out
  is the registered opt-in D-65); ADS as child nodes with resident capture
  (the og drops named $DATA - N-011's registered superset), sparse runs
  via off_width==0.
- **B8 multi-volume/cross-partition** — `NA` for the engine (orchestrator/LDM
  scope).
- **B9 timestamps** — `M` raw 100ns-1601 stored; SI-times-preferred-else-
  FILE_NAME. Og precedence between SI/FILE_NAME times not explicitly decoded —
  no observable contradiction in corpus; note only.
- **B10 filename/Unicode** — `M` UTF-16LE names, namespace ranking
  POSIX(4) > WIN32_AND_DOS(3) > WIN32(2) > DOS(1); og keeps DOS+Win32
  variants (both persisted in .rsf grammar); core ranks and keeps best —
  result-shape difference only, both names recoverable from session in og,
  one canonical in core. Recorded as note (no user-visible gap identified).

## Data model

- **D11 node fields** — `M` props field map mirrors corpus serializer table
  `{flags, name, size, times, attributes, runlist}` (cr_node.h comment).
- **D15 confidence grading** — `M` post-S-001-fix (the og-parity default
  drops the fully-Overwritten tier at walk time; the tier remains reachable
  in honest mode via the walker flag).

## Constants and formats

- **C19 magics** — `M` (all verified: record magic, OEM, 55AA, attr types
  0x10/0x20/0x30/0x40/0x80).
- **C20 offsets/sizes** — `M` (attr header fields, FILE_NAME layout at
  parent@0/times@8..32/alloc@40/real@48/flags@56/reparse@60/namelen@64/
  ns@65/name@66; SI layout; USA at +0x1E, sector slots +0x1FE). `M` post-N-001-fix (mirror span
  derives from the runlist, og this+0x70 parity; resident $DATA aborts
  the mirror exactly like the og logged path).
- **C21 enum values** — `M` FsKindCode::Ntfs=3 = og factory code 3.

## Algorithms

- **A25 engine steps** — `M` boot parse → $MFT load (fragmented path via
  $ATTRIBUTE_LIST probe type 0x20) → record loop (magic/fixup/base-record/
  props/filters/viability) → tree. Batch I/O note under X51.
- **A26 tree walk** — `M` record-scan model both sides (no INDX walk in og
  either; core documents D-N ruling).
- **A27 fragment handling** — `M` attrlist chase (BFS, depth 64, cycle guard,
  child-record pool), VCN-sorted $J merge, extents() adjacency merge.

## State machine

- **S32 cancellation** — `M` per-record cancel checkpoint; og per-record
  vt[0x10]; both keep already-emitted partials.
- **S35 error recovery** — `M` post-M-001-fix (clamp-vs-reject ladder in;
  torn-DBR volumes open and remnant-scan), `M` fixup-fail →
  skip-record (both), $MFTMirr fallback (**N-001** span), skip-forward on
  unreadable remnant blocks.

## Performance

- **X47/X51** — `M` (device level): og 64 KB $MFT batches + 0x1000-granular
  reads; core per-record reads behind CachedReader (64 KiB blocks, 8 MiB LRU)
  — equivalent device traffic, one caveeat: og memory-adaptive batch sizing
  (GlobalMemoryStatus) not replicated (core fixed budgets); not user-observable
  at current fixture scales. Note only.
- **X40 threading** — `M` single-threaded main walk both sides; remnant sweep
  parallel in core (og remnant threading undocumented in corpus).

## Safety

- **F60 bad-sector handling** — `M` (skip-forward remnant policy, retry
  delegated to storage layer; og equivalent documented in deep-dive §9).

## Quality

- **Q65 tests** — strong (15 areas, 1,726 LOC). Coverage gaps recorded (no
  finding, tracked as test debt): USN_HARVEST_CAP boundary, 64Mi bitmap
  per-run cap, 16 MiB bitmap piece skip, mirror read path records 1..3, 4Kn
  fixup path.
- **Q69 stubs** — `M` zero markers; deliberate scope refusals documented
  (encryption F13, INDX D-N).

## Corpus errata candidates surfaced by this wave (feed back to corpus)

1. `ntfs-engine--deep-dive.md` §7 runlist nibble prose contradicts the
   decompile (Q5) — core is right, prose is wrong.
2. OQ-1 (props flags bit 0x10 folder-vs-inuse) resolved by cross-evidence:
   the shared props library (fat-exfat-algorithms-v2 §4) documents 0x10 as
   the FOLDER bit; core's `attributes |= 0x10 if dir` matches. The
   deep-dive §3 "in-use maps to bit 4" reading is the misread. (Record-level
   in-use is the MFT header flag 0x0001, distinct layer.)

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-001 | SEVERE | Fully-overwritten deleted files emitted (graded) vs og destroy — **FIXED (diff-to-fix session; incl. sparse-run ratio sub-fix)** |
| M-001 | MAJOR | Bootstrap rejects record sizes og clamps (torn-DBR availability) — **FIXED (diff-to-fix session)** |
| N-001 | MINOR | $MFTMirr fallback hardcoded records 0..3 vs runlist span — **FIXED (d7f3c78)** |
| I-001 | INFO | Known-20 metafile filter vs blanket `$`-skip (deliberate, documented) |
| OQ-T1 | tracked-open | Secondary claim map semantics undecoded — **RESOLVED (decompile read, this session)** |

## Post-baseline note (recorded for truthfulness)

Audited baseline was `67193b6`. Concurrent session commit `7797bd6` ("GAP-S6-3 +
GAP-S6-4") landed after the NTFS wave completed and changes this module:
the remnant path now parses $DATA runlists (`cr_mftwalk.cpp` remnant reader,
+82 lines with tests). At the audited baseline the remnant path emitted
$FILE_NAME-derived nodes without $DATA extents — the audit's context sheets
had this registered as open GAP-S6-4 (root-causes.json). The B6 verdict above
describes `67193b6`; the fix direction closes the gap. **Re-audit of the
remnant $DATA path against the corpus remnant artifacts is queued for the next
full audit pass** (the corpus's remnant scan documentation is thin —
FUN_18002a780 shifted-salve is the primary decoded reference).

---

## Session 2 re-audit (audited commit `5d6da5f`)

Session 1's NTFS B7 "MATCH" verdict for hard-link/ADS/sparse was **REFUTED**
by session 2's line-by-line decompile read. Three new findings + one minor:

- **B7 hard-link fan-out — M-007 (MAJOR, superset):** og's `FUN_180006db0`
  calls `FUN_1800448b0` exactly ONCE per record (decompile `:5273`); the
  function fetches at most TWO `$FILE_NAME` attributes (indices 0 and 1) and
  picks ONE by namespace ranking. The core's `cr_mftwalk.cpp:711-724` collects
  ALL distinct parents and emits ONE node per parent (`:796-841`). Test
  `EndToEndWalk` `stats.hardlinks == 1u` proves the divergence. The session-1
  "M one node per distinct parent with shared refcounted props" verdict was
  based on a misread of the LOCK/unlock block as hard-link refcount management
  (the cited lines `21170-21205` are actually the viability+destroy block —
  see S-001's errata section).
- **B7 ADS — N-011 (MINOR, superset):** og's `FUN_1800069e0:5159-5160`
  explicitly checks `is_named($DATA)` and skips when true — ADS silently
  dropped. The core's `cr_mftwalk.cpp:854-899` emits ADS as child nodes,
  capturing resident ADS content + nonresident ADS extents. Tests
  `ResidentAdsCarriesContent` and `EndToEndWalk` `stats.ads == 1u` pass on
  the core; would fail on the og.
- **A25/A27 `$ATTRIBUTE_LIST` chase scope + depth — M-008 (MAJOR, superset):**
  og's `FUN_180041df0` (`:52615-52687`) is depth-1 with no recursion into
  child records' `$ATTRIBUTE_LIST`. The deleted-phase walk `FUN_18001acc0`
  (`:21095-21302`) passes NULL `param_6` at `:21161-21162`, disabling the
  chase entirely for deleted records. The core's `cr_mftwalk.cpp:475-572`
  is a multi-hop BFS with depth bound 64 (`FRAG_DEPTH_LIMIT` at `:23`),
  cycle guard via `visited` vector (`:489-501`), and fires for ALL records
  (no in-use gate). Test `NestedAttrlistChase` exercises multi-hop chase
  for record 23 → 24 → 25.
- **D11 compressed/sparse flag — N-012 (MINOR):** og's `+0x23` serialized
  flags sourced from `$SI.dos_flags` (`:5351`); core's `props->attributes`
  sourced from `$FILE_NAME.dos_flags` (`cr_mftwalk.cpp:735`) and masked to
  `& 0xFF` (drops 0x400 OFFLINE). Source mismatch + bit-mask divergence.

### Confirmed MATCH items (session 2 re-verified)

- **B7 sparse accounting post-S-001 sub-fix:** og
  `FUN_18001acc0:21220` `if (*plVar13 != -1)` skips sparse runs in viability
  iterator; core `cr_bitmap.cpp:96-97` `if (r.sparse) continue;` skips sparse
  in `reused_ratio`. Both exclude sparse from `total` and `reused`/`intact`.
  The candidate-4 concern (sparse-but-allocated falsely destroyed) is NOT
  a divergence: a sparse-only deleted file grades `Excellent` (ratio 0.0)
  and is NOT destroyed by either side.
- **B7 reparse points:** og `FUN_180006db0` only handles types 0x10/0x30/0x80,
  no 0xC0 dispatch. Core `cr_mftwalk.cpp:78-153` `build_props` switch only
  handles `ATTRIBUTE_LIST`/`FILE_NAME`/`STANDARD_INFORMATION`/`DATA`/
  `INDEX_ALLOCATION`. Both ignore `$REPARSE_POINT`.
- **B7 `$OBJECT_ID` (0x40):** both ignore. MATCH.
- **B7 `$VOLUME_NAME`/`$VOLUME_INFORMATION`:** both ignore at the engine
  level (volume label is an orchestrator/UI concern). MATCH.
- **B9 `$SI` vs `$FILE_NAME` time precedence:** og
  `FUN_180006db0:5338-5341` sets from `$FILE_NAME`, then `:5347-5350`
  OVERRIDES with `$SI` times if `$SI` exists and is valid. Core
  `cr_mftwalk.cpp:734` `props->times = has_si ? si_times : best->times;`.
  `$SI` wins when present, both sides. (Minor note: og stores 3 times
  [created/modified/changed — no accessed] per the serializer field map;
  core stores 4 via `FileTimes`. Extra `accessed` is informational.)
- **B7 `$FILE_NAME` with parent ref to deleted dir:** both reparent to
  `root_lost` for unresolved parents. MATCH.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| M-007 | MAJOR | Hard-link fan-out: og emits 1 node per record, core emits N |
| M-008 | MAJOR | `$ATTRIBUTE_LIST` chase scope + depth: core multi-hop all-records, og depth-1 in-use-only |
| N-011 | MINOR | ADS: og ignores named `$DATA`, core emits ADS as child nodes |
| N-012 | MINOR | Compressed/sparse flag: $SI vs $FILE_NAME source mismatch + 0xFF mask drops 0x400 |
| OQ-S2A-1..4 | open questions | In-use-phase record selection; downstream hard-link/ADS handling; $MFT $AttrList depth |

## Session 1 findings index (preserved for navigation)

| ID | Severity | One-line |
|---|---|---|
| S-001 | ~~SEVERE~~ → FIXED | Fully-overwritten deleted files emitted (graded) vs og destroy — moved to `findings/FIXED/` |
| M-001 | ~~MAJOR~~ → FIXED | Bootstrap rejects record sizes og clamps — moved to `findings/FIXED/` |
| N-001 | MINOR | $MFTMirr fallback hardcoded records 0..3 vs runlist span |
| I-001 | INFO | Known-20 metafile filter vs blanket `$`-skip (deliberate, documented) |
| OQ-T1 | ~~tracked-open~~ → FIXED | Secondary claim map semantics — moved to `findings/FIXED/` |
## Session-2 update (2026-10-25, core `e93decf`) — decompile-exhaustive re-read

**Verdict changes (decompile-verified; quotes in `../evidence/decompiled-quotes/ntfs-pass2-guid-dump.md`):**
- **B1/B4/D15 → S-012 FIXED (diff-fix session; two-pass landed):** the og is two-pass — pass 1 emits every live-MFT
  record (deleted included) **ungraded, never destroyed**; the viability/destroy/0x3ff machinery
  lives in a **GUID-dump pass 2** the core does not have. The S-001 destroy fix (landed, moved
  to resolved/) implemented a pass-2 rule in the core's only walk — mis-scoped; default flags
  now make the core a subset of the og on live-MFT deleted files.
  **Session-4: S-012 FIXED** — the two-pass architecture landed (membership flip + full
  pass-2 dump walk); see FIXED/S-012 Resolution. B1/B4/A26 = MATCH-with-superset
  (main-walk grading documented); B5 = OQ-S4 residual.
- **B1 (metafile filter) → I-001 refined:** og pass 1 = 15-entry name table + SI
  hidden+system; blanket-`$` is pass-2-only. Core's blanket-`$` skips live `$foo` files og
  emits.
- **B4 (viability accounting) → S-012:** og = per-RUNLIST-ENTRY, one probe at run START against
  a pass-1 claim map (live+dir+deleted-size>0 extents); core = per-cluster vs volume $Bitmap.
  **Session-4: S-012 FIXED** — pass-2 GUID-dump walk implements exactly this accounting
  (claim map + run-START probes + 0x3ff/destroy rules); the main-walk per-cluster $Bitmap
  grading stays as the documented informational superset (D15) with destroy OFF by default
  (pass-1 membership parity).
- **B5 (rescue) → S-012:** og rescues size-0 deleted records from MFT-copy resident data +
  same-name merges; core has none. **Session-4: rescue chain split to
  `tracked-open/OQ-S4-ntfs-size0-rescue-chain.md`** with the chain mechanics decoded
  first-hand (trigger gates, merge suffix-match rule, 9-fragment cap, 0x400 content cap,
  0xfe0001 re-emit); the undecoded sector-list/registry population is the blocker recorded
  there. The core's resident-$DATA extraction already covers the direct-extraction path.
- **C19/C20 (boot ladder) → M-001 revised + N-011b:** og rejects cpr==0/0-result, no pow2, no
  0x400 clamp, no 0xAA55; core pow2+clamp+%bps — two-sided divergence; the session-1 ladder
  quote was mis-attributed (bytes-per-sector lines).
- **N-001 CONFIRMED** (mirror capacity computed from mirror runlist, inclusive bound) +
  **N-002 mechanism-corrected** (claim stands; evidence was FAT[0]/[1] signature probes).
- **Matches confirmed:** SI-times precedence, $DATA size selection, compression/sparse flag
  propagation, fixup apply/undo equivalence, single-FN pick (core namespace ranking = superset,
  register).
- **Open:** grading locus for UI "Recovery Chances" (OQ-S2M-1) + engine +0x258/0x259 defaults.

### Session-2 addendum (post-reconciliation)
Cross-references to the parallel legs' NTFS findings this leg did not file duplicates of:
M-007 (hard-link fan-out: og one props per record vs core n-per-parent), M-008
($ATTRIBUTE_LIST chase scope/depth superset), N-011 (ADS child nodes vs og drops), N-012
(compressed/sparse flag source+mask) — all consistent with this leg's pass-architecture
read (S-012: pass 1 emits one item per record; namespaces unranked; single-FN pick). The
session-1 session-1 findings fixed in the meantime (N-001 runlist span, M-001 ladder) are
verified in their FIXED/ files with this leg's decompile quotes.

---

## Session-3 parallel-leg addendum (S3-NTFS2; core `34b3441`, corpus `03ad46c`)

- **S-022 (SEVERE):** the og's full scan carries an "orphaned-record sweep" the corpus
  never decoded — a raw-cluster walk (claim-map gated) recognizing INDX blocks and
  MFT records in volume space OUTSIDE the live $MFT, plus a $INDEX_ROOT entry
  harvest: directory entries' embedded $FILE_NAME (record ref + parent + times +
  real size + name) become real CNTFSFileProperties per entry; files feed a
  cross-source orphan collection the post-walk merger consumes; directories emit
  directly (dedup'd). Recovers names/metadata for files whose MFT records are gone
  but whose parent's index blocks survive — the classic second source. The core has
  NO INDX/INDEX_ROOT decode at all. Evidence:
  `evidence/decompiled-quotes/s3-ntfs-indx-second-source.md`.
- **N-039 (MINOR):** $UsnJrnl harvest gate — og: `reason & 0x200` (COMPRESSION_CHANGE
  per winioctl.h, not FILE_DELETE 0x20000); the `||`-combined harvest-all byte
  (+0x80) is write-once-zero with no setter anywhere (exhaustively verified) — dead
  code; bounds: og 512 B record cap / no version gate / no journal-size cap; core:
  true FILE_DELETE bit, V2+V3 gates, 64 MiB cap, UTF-16. The dead first-wave agent's
  USN probe premise was inverted (its 0x80000200 = CLOSE|COMPRESSION_CHANGE, not
  delete) — probe output retained as core-behavior evidence, interpretation
  corrected. Evidence: `evidence/decompiled-quotes/s3-ntfs-usn-gate.md` +
  `evidence/executions/s3-ntfs-usn-reason/`.
- **Attribute-type dispatch (remaining types) — MATCH-by-omission, lead-verified by
  targeted greps (2026-10-26):** the og's entire decompile carries ZERO type
  comparisons against $EA (0xE0), $EA_INFORMATION (0xD0), $VOLUME_NAME (0x60),
  $VOLUME_INFORMATION (0x70), $REPARSE as a type-constant, or
  $LOGGED_UTILITY_STREAM (0x100) — its handled set is the same family the core
  dispatches (0x10 SI / 0x20 ATTR_LIST / 0x30 FILE_NAME / 0x80 DATA+named / 0xA0
  INDEX_ALLOC; cr_mftwalk.cpp:79-139). Both engines therefore ignore $EA and
  volume attributes identically.
- **EFS — MATCH-by-omission (type level):** no EFS/encrypt strings and no
  FILE_ATTRIBUTE_ENCRYPTED checks in the og; encrypted files emit as their
  (encrypted) $DATA on both sides. Content-level grading (does either flag EFS
  files as unrecoverable?) is UNVERIFIED — neither tree has a fixture; next
  artifact: an EFS fixture against both (low priority: neither side has any
  EFS-specific code to diverge).
- **Compression engines — MATCH (LZNT1-only, both sides):** the og's compression
  constants are the LZNT1 unit-size ladder (0x3f / 0x800 / 0x8000 comparisons at
  :2916/:3814; 0x8000 chunk allocations) with no LZX/XPRESS engine strings or
  dispatch; the core implements MS-XCA LZNT1 only (cr_decomp.cpp:131). compact.exe
  /LZX and WOF-compressed files use a sparse+reparse storage path outside BOTH
  engines' scope — register as a shared limitation (PARITY-DELTAS row), not a
  divergence.
- **$VOLUME_NAME/$VOLUME_INFO both-ignore:** re-confirmed (session-2 verdict
  stands; zero 0x60/0x70 type comparisons in the og).
- **Open (from S-022's decode, for the next pass):** the og's orphan-collection
  `$`-name filtering / re-parenting by the dir map (:20104-20110) is undecoded;
  whether FUN_18000c890 (run coalescer) and FUN_18002a780 (B6 remnant scanner) are
  one sweep's two modes or two entries.


## Diff-fix session addendum — S-022 (A26 corrected)

A26's session-1 og-side premise ("no INDX walk in og either") is REFUTED (S-022 evidence:
the og's orphaned-record sweep harvests INDX-block + $INDEX_ROOT entries). The core now
implements the same harvest in the remnant path — in-use entries only, never slack, og
gates exact (validator, chain parent-consistency, entry flag gates, 0x1f5 cap,
$-name filter for directories). The wire family lives documented in `cr_ntfs_wire.h`
(D-N ruling re-decided). See findings/FIXED/S-022-*.md.
## Minors-fix session update (2026-10-26)

**N-011 FIXED-ALREADY + pinned** — `WalkerFlags::emit_ads` (367561d) already
implements the finding's opt-in parity shape; the missing parity pin
(`NtfsVolume.AdsDroppedInParityMode`) and a default-superset guard are new.

**N-012 FIXED** — serialized attributes now source from
$STANDARD_INFORMATION's dos_flags (og FUN_180044f60 @ :5351-5368) with the
$FILE_NAME fallback; mask 0xFF → 0x7FF preserves the OFFLINE bit. Dump-pass
emit applies the same rule with the dumped record's own $SI.

**N-024 group FIXED/REGISTERED** —
- N-011a: `FileProperties::short_name` (distinct DOS-namespace $FILE_NAME),
  persisted as session-record nflags bit 2 (versioned optional).
- N-011b: volume open = NTFSLib LoadNtfsVolume exactly: bps 512-multiple,
  signed spc decode (non-pow2 accepted), cpr==0/0-result/sub-sector REJECTED,
  index-block 0 rejected, no 0x400 clamp at open. The M-001 walk ladder's
  evidence stands (buffer capacity inside the walk). Registered hardening
  kept (D-52): 4096 bps cap, P1-4 exponent guard, P1-3 size sanity,
  bps-vs-device match.
- N-011c/d: stride quirk + namespace ranking registered as core supersets.
- N-011e: og error-facility mapping table registered in ABI-CONTRACT §4.

**N-039 FIXED** — 0x20000 gate kept (og's 0x200 mask = an og defect,
registered); >64 MiB $J streams mine the TAIL (circular journal — newest
records last); the fixture's mislabeled 0x100 constant renamed
`kFixtureReasonBit` (HARD_LINK_CHANGE, not FILE_CREATE).

**Tests:** +11 (2 fail-before discriminators for N-012, 3 for N-011b via the
rescoped M-001 trio + 2 new, ADS pins ×2, USN pins ×3 + tail-mining).
Suite 384/384.


## Session-4 verification-and-correction update (2026-10-26, diff-fix session 4)

**S-012 addendum (finding in FIXED/):** the landed two-pass implementation
was re-verified line-by-line against the corpus; three corrections landed:
- **B6/B8 (dump-walk stride):** the og strides the RECORD SIZE on every
  path (uVar8 after holes LAB_18001b0cb and after emit :21151; uVar16 =
  max(0x400, rs) only after props-fail/skip/destroy — coinciding through
  the boot clamp). The one-sector hole stride and the max(sector, 0x400)
  parse bound are corrected; 2/4 KiB-record volumes parse fully again.
  Pinned by GuidDumpStrideIsRecordAligned + GuidDumpLargeRecordParsedFully.
- **B1 (pass-1 filter, N-040 FIXED):** the og's gate is (og-15 name table)
  AND ($SI SYSTEM+HIDDEN) — FUN_180040f20's table decoded first-hand. No
  $FILE_NAME-flags system filter exists in pass 1; the core's old
  `dos_flags & 4` gate (which dropped pagefile.sys-class user files) is
  removed along with the flag and counter. Pinned by
  MainWalkKeepsSystemFlaggedUserFile.
- **Pseudo record#** corrected to the og's lVar18 arithmetic.
- D-51 corrected in place; D-50/D-51 numbering collisions registered as
  errata (the FAT entry renumbered D-56).

---

## Session-5 addendum (2026-10-26, audited `7cf6754`)

- **Content-proof recognition + scanner backup-boot — `M-043` FIXED (s10):** open() falls back to the volume-end backup DBR (boot_from_backup flag; repair stays consent-gated); probe_family requires FILE magic at $MFT LCN. TornPrimaryBackupBootScans/TornPrimaryShapeBackupBootScans/BackupBadFallsBackToRefusal/BootFromBackupFlagged/SpoofedOemRejectedEarly; suite 600/600. findings/FIXED/M-043. ~~original text:
  the og's NTFSFileSystemAnalyser verifies the volume-end backup boot
  sector and requires the $Volume MFT signature before recognition; the
  core's probe_family is boot-bytes-only and NO scanner path reads the
  NTFS backup DBR (cr_bootstrap.cpp:329-376 — only the consent-gated
  rawfixer does, cr_rawfixer.cpp:589-600). Torn-primary + intact-backup
  volumes require --repair in the core where the og proceeds.
- **NTFS version gate + MFT×$Bitmap cross-check** — folded into `M-041`
  (check phase): og gates {1.2, 3.0, 3.1} (0x100a) and cross-checks every
  allocated MFT record against $Bitmap (0x1002/0x1003); core has neither
  (cr_bitmap.cpp is grading-only).

---

## Session-11 fix row (2026-10-26, DFX11 — M-007 closed)

**M-007 FIXED** (findings/FIXED/M-007, this session's `fix: M-007` commit):
the hard-link emission model now matches the og by default.
- `WalkerFlags::single_link_per_record` (default TRUE): ONE node per MFT
  record - the namespace-best $FILE_NAME from the FULL list (first-seen
  wins ties), emitted under ITS parent. Corpus: FUN_180006db0 :5273 (one
  props build per record) + FUN_1800448b0 :54124-54162 (two-name fetch) +
  FUN_180043330 (namespace ranking) - the finding's citations, re-verified
  first-hand.
- FALSE = the registered opt-in fan-out (one node per distinct parent,
  per-link names; D-65) - the "show all hard-links" mode. The hardlinks
  stat counts only there (the og's scan has no such counter).
- The fan-out path's per-parent name selection REPAIRED: per-parent names
  are now selected from that parent's OWN entries (the old code seeded
  the search from the GLOBAL first name, so on mixed-rank multi-parent
  records every lower-rank parent inherited the highest-rank name - a
  real pre-existing divergence-class bug that no earlier fixture exposed).
- B7 verdict moves to MATCH for the emission model; the finding's OQ-S2A-2
  (consumer-side fan-out) remains open by design. Suite 582/582.

---

## Session-11 fix row 2 (2026-10-26, DFX11 — M-008 closed)

**M-008 FIXED** (with the premise corrected against the corpus):
`WalkerFlags::single_hop_attrlist` (default FALSE = the multi-hop BFS
superset, D-73 — the finding's own recommendation); TRUE = og parity
(depth-1, corpus FUN_180041df0 :52615-52687). The deleted-gate premise
was STALE: FUN_18001acc0 is the GUID-DUMP pass 2 (S-012's decode), not a
deleted-phase walk; pass 1 (FUN_180017390) emits every record AND builds
the child-record list (:18461-18464, plVar9 non-NULL) — the og chases
deleted records at depth 1 too. OQ-S2A-1 resolved by the same evidence;
OQ-S2A-4 ($MFT bootstrap depth) stays open. 3 tests; suite 593/593.
