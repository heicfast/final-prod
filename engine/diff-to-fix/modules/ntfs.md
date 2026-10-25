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
  reject vs clamp); result-set membership: **S-001** (overwritten deleted
  emitted vs destroyed), **I-001** (known-metafile list vs blanket `$`-skip).
- **B2 recovery correctness** — `M` runlist codec (nibble semantics verified
  against decompile Q5; core documents the corpus prose misread), fixup
  (two-tail tolerance replicated from og loop bound `local_18 < 2`),
  compressed $DATA expansion (LZNT1 envelope → codec-tagged extents,
  zero-fill on inconsistency — never wrong bytes), resident content capture.
- **B3 false-positive rate** — `M` (viability-graded emission; no unguarded
  inference in this engine; inferred-chain honesty caps live in FAT module).
- **B4 deleted-file classification** — **S-001** (destroy-vs-grade);
  strict `0x3ff < size` gate confirmed both sides (core
  `viability_min_size=0x3FF`, `cr_mftwalk.h:45`, strict `>` at
  `cr_mftwalk.cpp:668`); secondary claim map **OQ-T1 (tracked-open)**.
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
- **D15 confidence grading** — **S-001** (Overwritten tier has no og
  counterpart; og drops the node).

## Constants and formats

- **C19 magics** — `M` (all verified: record magic, OEM, 55AA, attr types
  0x10/0x20/0x30/0x40/0x80).
- **C20 offsets/sizes** — `M` (attr header fields, FILE_NAME layout at
  parent@0/times@8..32/alloc@40/real@48/flags@56/reparse@60/namelen@64/
  ns@65/name@66; SI layout; USA at +0x1E, sector slots +0x1FE). **N-001**
  (mirror span hardcoded 4).
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
- **S35 error recovery** — **M-001** (clamp-vs-reject), `M` fixup-fail →
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
| S-001 | SEVERE | Fully-overwritten deleted files emitted (graded) vs og destroy |
| M-001 | MAJOR | Bootstrap rejects record sizes og clamps (torn-DBR availability) |
| N-001 | MINOR | $MFTMirr fallback hardcoded records 0..3 vs runlist span |
| I-001 | INFO | Known-20 metafile filter vs blanket `$`-skip (deliberate, documented) |
| OQ-T1 | tracked-open | Secondary claim map semantics undecoded |
