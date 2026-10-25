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
- **B7 hard-link/ADS/sparse** — `M` one node per distinct parent with shared
  refcounted props (mirrors og LOCK/unlock AddRef/Release sharing), ADS as
  child nodes with resident capture, sparse runs via off_width==0. Og-side
  result shape for ADS not fully decoded in corpus — no contradiction found.
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
