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

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new carve-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
carve-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`):**
- **S-018 INVALID (session 3, diff-fix leg — premise refuted first-hand):** the
  finding claimed the og's JPEG analysers never check for FFD9 (a literal `0xd9`
  grep over the decompiled bodies returned 0). Ghidra renders the byte as the SIGNED
  CHAR literal `-0x27`: the modern JpegAnalyzer (FUN_180033a90) has FIVE FF D9 sites
  and its size-accumulation accept path is EOI-gated (`if (uVar21 == 0) goto
  <epilogue>`, :39760) - it scans BACKWARD for the last FF D9 in the buffer and
  returns without accumulating a size when none exists. Our validate_jpeg's
  EOI requirement is therefore consistent with the primary analyser as far as the
  corpus proves (different mechanism: forward walk vs backward scan - which admits
  more is open). NO code change; the residual (the legacy FileJPGAnalyser's
  JFIF/Exif/generic sub-analysers FUN_18004bbd0/b3b0/c1e0, and the streamed
  continuation outcome) is tracked in OQ-S3-JPEG-fallback-and-continuation-accept
  with the exact decode list. Evidence:
  evidence/decompiled-quotes/s3-jpeg-eoi-encoder-correction.md.
- **S-017 FIXED (session 3, diff-fix leg):** the four ftyp-family signatures baked
  the ISO-BMFF box SIZE (four literal zero bytes) into their magic, so no real
  MP4/MOV/M4A/HEIC head (size 0x14/0x18/0x20) could ever match — four advertised
  formats dead, surfacing only as raw-sign .lrf mislabels. Re-keyed on the "ftyp"
  box TYPE with header_offset 4; candidate now lands on the size field and the
  brand reads at candidate+8 land where real files carry it. The window-matching
  primitive is extracted (match_signature, cr_signatures.h) and shared with a NEW
  F8 load-time self-probe (self_probe_registry): unreachable magics and mis-shifted
  candidates fail at startup — the probe's synthetic-buffer match alone was proven
  WEAK during fail-before (it faithfully reproduces a broken magic), so the
  structural rule (four-zero-byte magic prefixes assert a 32-bit data field is
  constant) is the gate that catches the class. Conservative naked-moov salvage
  (FMT_MOV_NAKED, appended id): mirrors the og's list-layer MovAnalyzer
  (FUN_180004fb0 :2919-4208 — the live registration path; the u16 key at :2748 is
  dead under the zero-header guard :9954) for ftyp-destroyed files, tried LAST so
  ftyp-keyed entries claim intact positions first; og accept criteria undecoded —
  the follow-up surface is documented in S-017's Resolution. Tests:
  Carve.Mp4RealFtypIsom / MovRealFtypQt / HeicRealFtypHeic / M4aRealFtyp /
  MovMoovWithoutFtyp / RegistrySelfProbeReachesEveryFormat.
- **S-015 FIXED (session 3, diff-fix leg):** raw_signs min/max decoded as KiB in the
  og (`*0x400` gate / `<<10` size / `min<<10` acceptance) — the compiled table now
  converts once in the generator (zero-max entries refused; the og's 500 MiB default
  would be a silent divergence); every format's window was 1024x too small and the
  floor 1024x too low. Tail walk rebuilt: chunked forward scan (1 MiB reads +
  sign-length overlap), named `kRawSignTailScanLimit` (8 MiB) with the og's true
  full-window range (up to 200 MiB) documented in cr_rawsigns.h as the bound's
  rationale. DISCOVERED + fixed in-commit: per-format tail scoping — the og pairs
  tails within the candidate format's own record; the core's format-agnostic scan
  fabricated close positions (pinned by RawSignLayerHeaderOnlyAndSharedMagic:
  fst has no tail signs, stays header-only; RawSignTailPairWithinFormat/TailPairWindowScaled
  pin in-format far-tail pairing). Units + floor + far-window pinned by
  RawSigns.UnitsAreKib / CarvesFiveMiBVob / MinFloorOneKib.
- **OQ-T8 RESOLVED:** og dispatch = sector-aligned probing, 3-layer first-accept-wins
  (u32 RB-tree → u16 multimap → list sweep), per-key modern→legacy chains, no cross-layer
  resolution. Core byte-steps = detection superset (registered, I-009).
- **M-005 now decompile-proven:** og bitmap-gates carve (bit test + cluster skip +
  dispatch suppression); core has no gate — full standalone finding file created.
- **S-007 evidence completed both directions:** all 8 session-1 formats confirmed +
  ~26 additional og-only formats (bytes cited); ISO9660 is core-only (og cannot carve ISO).
- **N-014d filed:** ZIP padding semantic inversion (og salvages non-empty trailing sectors
  after broken chains, never inflates clean archives; two-tier version gates) — N-008 refined.
- **M-021 filed:** video-repair pre-scan wiring absent (og loads EUVideoRepair.dll and runs
  a per-candidate repair check).
- **N-014b/c filed:** carved-name split inside the core; runtime per-format config surface.

---

## Session-3 parallel-leg addendum (S3-CARVE; core `34b3441`, corpus `03ad46c`)

Complements the section above (same session, parallel leg — IDs renumbered past the
leg that landed first; registry decode evidence: `evidence/decompiled-quotes/s3-rawsearch-inventory.md`):

- **S-017 (SEVERE, executed proof):** the four ISO-BMFF signatures in the core's
  table encode the ftyp box SIZE as literal zero bytes — real MP4/MOV/M4A/HEIC
  headers (size 0x14/0x18/0x20) can never match; `cand_abs = match − 4` then puts
  the brand read on the "ftyp" bytes themselves. Executed comparison: zero grammar
  hits for four planted real-world BMFF files. The og carves these via its u32
  4CC layer + MovAnalyzer/HEICAnalyser. (evidence/executions/s3-ftyp-signature-deadness/)
- **S-018 (SEVERE):** JPEG accept policy — neither og JPEG analyser generation
  looks for EOI (no 0xD9 constant in either body); size = SOF-dimension
  arithmetic. Core requires EOI+SOF+SOS → truncated JPEGs (the canonical
  damaged-photo case) are salvaged by the og and silently dropped by the core.
- **M-024 (MAJOR):** no runtime format-filter surface — the og rebuilds its
  registry per search from `config/RawConfig.ini [Filter]` (37 named toggles,
  STL/VPJ default-off) and exports three registry projections
  (**N-033**: consumed by the FS SearchDLLs, which clone the master registry into
  per-engine state); the core's table is compile-time with no enumeration ABI
  (A42/A45).
- **M-025 (MAJOR):** read model — og = 4 MiB device reads into an 8 MiB ping-pong
  buffer, one dispatch per sector-aligned position, allocated-cluster skip,
  open-ended formats tracked as pending CRawFileInfo objects extended across
  chunk boundaries (streaming state machine); core = 1 MiB windows, byte
  granularity (superset, I-009), stateless validators that re-read extents from
  the device (≤8 MiB extra re-read for hashing). M-021's repair-hook fix depends
  on M-025's pending-object state machine.
- **Dispatch chain semantics (addendum to N-031):** u16 equal_range chains iterate
  in insertion order; plain-accept-after-special-reject records format id 8
  (generic) and continues; `00 01 00 00` = TTF (u32 layer) pre-empts MDB (u16).
- **S-007 correction (this leg, decompile re-verified at :1985/:2156-2177/:3097):**
  the addendum above carries two wrong rows — C5 D0 D3 C6 is **CEPSAnalyser**
  (AI/EPS), not dBase (no DBF analyser exists); E4 52 **5C** 7B is **OneNote**
  (OneAnalyser/FileONEAnalyser), not LZH (LZH = list-layer LZHAnalyser). Errata
  rows 22-28.

---

## Session-3 parallel-leg rows (integration 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| M-039 | MAJOR | Cross-engine occupancy-bitmap feed missing (og fills one per-volume bitmap in EXT4/Btrfs/ZFS engines — Btrfs's is sparse-file-backed — feeding downstream phases; our diskmap is visualization-only) |
| N-030 addendum | — | Tail-hit size formula (`sec*(cur-start) - tail_off + 50` vs our `lo+pos+512`) + the invented 2 MiB tail cap + 500 MiB default — this leg's decode folded into N-030 |

Evidence: `evidence/decompiled-quotes/s3-rawsigns-consumer-decode.md` (the
loader/matcher/CommonRawAnalyser decode, complementing the lead leg's
`s3-rawsigns-units-and-dispatch.md`).

---

## Session-5 addendum (2026-10-26, audited `7cf6754`)

- **M-029 §Gap (b) corrected via OQ-S4-2 resolution**: the RAW naming hook
  is a single-site category display-name mapping (reserved key
  "filter_generalraw" → the general-raw filter node's display name via
  DRWUI's firebase.json map) — NOT a per-file rename/filter gate. Decode:
  `evidence/decompiled-quotes/s5-oq-s4-2-raw-naming-hook.md`. No carve
  emission-path change required for parity; the fix narrows to the
  category-bucket naming surface.

## Session-5 audit (audited commit `7cf6754`) — the never-cited family-artifact wave

All 6 RAWSearchDLL family artifacts + per-engine-function-map were read for the first time
(any session). Verdict changes and additions:

- **B4 deleted classification — S-031 (SEVERE, NEW):** the og flags every carved record
  deleted (FLAGS=0x0020 constant, 12,147/12,147 census); the core's carve path never sets
  deleted state, so `deleted_only` queries and `deleted_count` stats exclude the entire
  carve set.
- **B1/B2 archive inventories — S-032 (SEVERE, NEW):** the og's ZIP analyser returns the
  complete inner-entry inventory (FUN_180045690 + FUN_180007060; LFH/CD/EOCD anchors) and
  the RAR walker stitches multi-part sets (272-byte name buffer); the core emits one flat
  node per archive.
- **A28 validator details — N-051/N-052/N-053/N-054 (MINOR, NEW):** MKV DocType
  discrimination + element-bounds gate; HEIC brand relabel to L"HEIF"; IIU bound to RW2 in
  the core vs the og's TIFF/EP chain; RAF 4-byte vs 16-byte magic strictness (miss) and
  JPEG 4th-byte superset (register).
- **B10 naming — N-063 (MINOR, NEW):** the core's `File%06u` default namer is an invention
  (its "corpus policy" comment is false); the og names per-analyser (content-embedded
  names; MP4/MOV "Lost Name File (...)_EaseUS" template). OQ-S4-2 RESOLVED (the +0x48 hook
  is a per-scan "filter_generalraw" config fetch).
- **Runtime extensibility — M-048 (MAJOR, NEW):** EUUserData (111-extension user signature
  DB, `"offset,hex"` + MIME) has no core counterpart.
- **D12 provenance — M-051 (MAJOR, NEW):** the og's RAW records carry a timestamp string,
  analyser checksum (0xFFFFFFFF=invalid marker), note, tail anchor, boolean; the core sets
  none.
- S-007 re-confirmed with the family rosters (no new formats beyond its addenda; the
  census's counts win where the family files conflict — corpus errata row 38).

## Session-4 update (2026-10-26, diff-fix session 4 — the S-007 lane)

**S-007 FIXED (primary set):** the og's hardcoded common formats landed
with corpus-exact keys — GIF8, BM, the 18-key headerless MPEG-audio
family (FF E2..E7/F2..F7/FA..FF), AAC ADTS (FF F1/FF F9), {\rt, OLE
D0CF11E0 (DIFAT + directory-chain walk, CFB (N+1)*ssz rule), MPEG PS/ES
start codes, DWG AC10 (version gate + bounded extent). 27 registry
entries + 9 validators; the audio family carries `suppress_overlap`
(one stream = one node — the og's walker semantics at the core's
byte-granular grid); the headerless audio gate is 10 consecutive frames
(byte-granular FP calibration, registered in D-57). 3 new tests
(S007CommonFormatsCarve / S007ValidatorsRejectNoise /
S007TruncatedSalvageSemantics), fail-before 3/3; suite 436/436; hostile
ASan smoke clean. **Remainder (precisely scoped in the finding):** the
~25 niche og-only formats (CHM/AU/MIDI/APE/EPS-AI/DXF/DMG/... — the
session-2/3-corrected table in the finding file is the implementation
list) + the bench seed-matrix extension (Windows runner).
## Session-6 rows (deep continuation, 2026-10-26 — the never-read family layer)

The six `rawsearchdll--family-*.md` artifacts + the registry census/dual-registry decode were
first-cited this session (evidence: `evidence/decompiled-quotes/s6-carve-family-layer.md`).

| ID | Severity | One-line |
|---|---|---|
| M-042 | MAJOR | Archive inner-file inventories: og analysers return the complete ZIP/RAR entry list; core CarveResult is {end,size} only (cr_signatures.h:75-80) |
| N-052 | MINOR | ZIP: EOCD (PK\x05\x06) anchor missing (truncated/EOCD-first zips) + LFH sanity gates (version_needed ≤10, extra ∈[1,0x102]) missing |
| N-053 | MINOR | CAB lowercase `mscf` registry key missing (og registers both cases — deliberate duplicate-key policy) |
| N-054 | MINOR | Overlap resolution: og ranks by accumulated header+tail confidence + two-generation validator fallback; core trial-order first-wins, one validator per format |
| S-007 addendum | SEVERE | Roster sharpened: bare-MPEG dispatch (18+ u16 sync keys vs ID3-only — complete recall gap for tagless MP3), the ~45-format family census, family provenance per format |

MATCH/superset registers (no findings): RAR4 fast-accept (core stricter), PST "MS"+AMap gates
(core superset), RAR5 (core implements; og rejects), tail-scope/8 MiB bound (S-015 FIXED family).
Runtime registry machinery (EUGetAnalyserList/Map/Multimap refresh, RawConfig 37-toggle gate)
belongs to M-024/N-033's family.

## Session-9 verdicts (continued — M-051)

| ID | Verdict |
|---|---|
| M-051 | **FIXED** — the RAW provenance fields land as the finding's plan scoped: carve_checksum = CRC32(extent's first 512 bytes) at both emit sites (0xFFFFFFFF = the og's invalid marker, the default for non-records incl. the S-032 inner-entry children); carve_anchor_b = the tail sign's exact position on raw-sign tail-paired hits (0 = header-only, the og's 261/12,147 census); carve_note + carve_flag carried and round-tripped (the og's note is "usually empty"; +0xF4's semantics undecoded — OQ-S6-3). Anchor A = the extent start (already carried). Record-4 nflags-bit4 block. 2 fail-before tests. |
## Session-12 update (2026-10-26, DFX12 — the carve-magic quartet, CONVERGENT with the parallel lane)

**N-051/N-052/N-053/N-054 FIXED by two convergent landings** (this
session verified the corpus first-hand — analyser-granular-batch1
:43-80, registry census :53-55, carving deep-dive :28-60 — while the
parallel lane's `b2f48c6` landed the same four findings in the same
window; the union takes their landing as canonical per the repo's
collision convention, and this session's independent verification +
fail-before execution stand as the second proof set):

- **N-051:** their validate_mkv (bounds gate + DocType webm/matroska
  discrimination) is canonical; this session's independent decode adds
  that the artifact's second constant 0x67805318 is the Segment element
  ID 0x18538067 read little-endian (a decode refinement recorded here
  for future audits — real matroska files carry both a matroska
  DocType and a Segment element, so both readings agree on real files).
- **N-052:** their FMT_HEIF registry row is canonical (heic-proper stays
  HEIC; siblings relabel HEIF). This session's alternative carrier
  (per-file CarveResult::ext_override keeping one type-id) is preserved
  in git history (017d63b) as the rejected design — the finding's
  "type-id unchanged" guidance vs the registry-row approach was decided
  by first-landing.
- **N-053:** same resolution both legs (FMT_TIFF_EP claims the IIU key
  with validate_tiff; FMT_RW2 = salvage fallback, their D-63a = this
  session's D-64).
- **N-054:** same resolution both legs (FUJI 4CC + canonical-string
  primary + variant path; JPEG prefix registered, their D-64a).
- **N-076 (S12 NEW-FINDING — THIS session's unique contribution):**
  discovered while building the N-054 fixtures: the canonical RAF
  version gate rejected every real-world RAF (the version field at +16
  is a 4-byte ASCII string "0100"/"0201"; LE-read values ~8e8 far above
  the 0x2000 binary cap) — the row was dead for real files since
  introduction, with zero fixture coverage. The parallel landing
  (b2f48c6) still carries the defect; fixed in this session's follow-up
  commit with Carve.RafRealAsciiVersionAccepted (fail-before executed
  on the stash-isolated pre-fix engine: RED).

Fail-before evidence for this session's independent run: 8/10 of this
leg's tests RED on the stash-isolated pre-fix engine (the parallel leg's
8 tests carry their own fail-by-absence proofs). Suite post-union:
549/549 linux-release.

**N-069 (MINOR, S5) — FIXED (s11):** the og's duplicate-key policy for
case-insensitive formats is now mirrored — FMT_CAB_LC (lowercase `mscf`)
is its own registry row sharing validate_cab, and the validator's internal
magic re-check accepts exactly the two registered spellings (census :94,
policy :127; mixed case never anchors — no row keys it).
`Carve.CabLowercaseMagic` proves both cases carve; the lowercase node
carries the distinct format id. B1/C19 verdict for this row: MATCH.
