# modules/carve.md — carve module audit verdicts

Reference: RAWSearchDLL per corpus artifacts `SearchDLL/rawsearch-carving--deep-dive.md`,
`rawsearchdll--analyser-registry-census.md`, `raw-dual-registry--u16-magic-keys.md`,
`rawsearchdll--family-{brance,bruteforce,rawspeed,filegen,special,xjq}.md`,
`rawsearchdll--zip-analyser-carving.md`, `analyser-granular-batch1--pst-rar-mkv-heic.md`,
`CORRECTIONS.md` row 5; og-codebase raw_signs.json (parsed this session).

Core (audited commit `67193b6` + concurrent `03ddcd2` GAP-S6-8 fix in tree):
`src/core/carve/` (5 files, 2,969 LOC) + `include/cr/carve/` +
`tests/unit/test_carve.cpp` (14 areas).

## Behavioral

- **B1 detection (carve set)** — **S-007** (missing GIF/BMP/headerless-MP3/AAC-ADTS/
  RTF/OLE-Office/MPEG-video/DWG + ~10 niche; executed set-comparison vs og registry ∪
  raw_signs).
- **B2 recovery correctness** — `M` on the implemented validators (JPEG marker walk with
  SOF precision-8; PNG IHDR/chunk walk; TIFF family IFD walks; BMFF brand walks; PDF
  next-%%EOF boundary; ZIP CD-anchor + indexless LFH-chain salvage + zero-padding
  extension; 7z header math; SQLite page math; RAR4 block walk; RAR5 vint walk
  (clean-room beyond og); PST !BDN+MS; camera RAW relabeling; MP3 full frame walk;
  WAV/ASF/FLV/CAB/ISO).
- **B3 false positives** — `M`-note: raw-sign layer emits **unvalidated** unless
  tail-paired (by og design — the og's raw_signs.json layer is likewise
  confidence-scored, not grammar-validated); grammar layer first-format-wins; **M-005**
  exposes the FP surface to allocated regions the og never carves.
- **B5 orphan** — `M` carved nodes attach under the carved root ("Other deleted files"
  analog); structured-wins merge (og IRawDirectoryTreeBuilder parity).

## Constants and formats

- **C19 magics** — `M` on the implemented 31 grammar signatures (spot-verified against
  og keys: JPEG FFD8FF family incl. e0/e1/db/ee variants — og's four u32 keys; PNG
  89504E47; ZIP 04034B50 + CD 02014B50 preferred-anchor parity CARVE-2); **S-007**
  (missing keys); **N-008** (ZIP padding cap/alignment; version-gate 0x13/0x62 vs
  artifact's wrong "≤ 10" — decompile authoritative, core must admit 0x62).
- **C20** — `M` (PST size offsets 0xA8/0xB8 + 1 MiB floor CARVE-4; EBML vint ≤ 8; HEIC
  brand 0x68656963; JPEG marker guard).

## Algorithms

- **A28 carving validator logic** — `M` on implemented validators (several exceed og:
  PST AMap probe = deliberate hardening CARVE-4-aligned; RAR5 clean-room); **S-007**
  missing validators; **N-008** ZIP semantics.
- **A29 merge/dedup** — `M` post-GAP-S6-8: carve emits disk-absolute extents with
  NODE_FLAG_EXT_SHIFTED; dedup vs structured on the absolute basis (the resume
  extra-entries bug fixed in-audit-window with fail-before/pass-after tests).
- **A25** — **M-005** (whole-slice vs og's bitmap-gated free-space pass on volume
  scans).

## State machine

- **S32** — `M` per-window checkpoints + 50 ms watcher thread; abort flag per byte
  offset (parallel mode).
- **S33/S34** — `M` pause-aware worker heads (G-07); no intra-carve checkpointing
  (stage-level checkpoints cover it).

## Performance

- **X47/X51** — `M` 1 MiB windows + 4 KiB overlap; P1-12 probe blocks (1 MiB windowed
  field scans replacing per-512B stepping); raw-sign 512-byte strides; SHA-256 hashing
  of small files (≤8 MiB re-read). **M-005** cost note (whole-slice carving ≈ 2× the og
  on half-full volumes).

## Safety

- **F60** — `M` unreadable windows skipped + counted (not scanned) — honest fill-rate
  accounting G-05.

## Quality

- **Q65 tests** — strong (14 areas incl. all og-parity carve fixes). Gaps: the S-007
  format set (nothing to test yet), ZIP64 extra-field path, 4Kn ZIP padding.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-007 | SEVERE | Missing common formats (GIF/BMP/headerless-MP3/AAC/RTF/OLE-Office/MPEG/DWG) |
| M-005 | MAJOR | Whole-slice carving vs og's bitmap-gated free-space pass |
| N-008 | MINOR | ZIP padding cap off-by-one + alignment; v0x62/ZIP64 chain semantics |
| OQ-T8 | tracked-open | Dispatch order (u16→u32→signs); JPEG full grammar; no-bitmap og behavior |

## Corpus errata candidate

`rawsearchdll--zip-analyser-carving.md` §1 "version_needed ≤ 10" is wrong — decompile
says ≤ 0x13 or == 0x62 (:18294, agent-B verified). Core's own gate must be checked
against the corrected value when N-008 is fixed.
