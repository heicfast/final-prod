# modules/carve.md — carve module audit verdicts

## S12 addendum (2026-10-29 — the session-2 deep-dive — the local-database/user-sign family)

New active findings on this module: **S-055** (FIXED session-51 — the user-sign layer
probed before the built-in table, inverting the M-048/D-104 collision rule; 8 overlap
formats flipped to device-end extents; built-in-first restored, the no-reclaim rule
pinned); **S-056** (FIXED session-51 — disabled_formats did not gate the user-sign
layer; match_at's disabled_exts predicate now carries the removal-loop semantics);
**M-128** (hex-case semantics — 12
og-dead signs live in core); **M-129** (the all-or-nothing parser vs the og's tolerant
skip); **N-168** (the og's 2x bounds guard, core correct); **N-169** (duplicate-asf
merge semantics); **N-170** (8 stale pre-erratum doc claims); **N-171** (user-sign
files never tick carved_found); **I-035 (ACKNOWLEDGED — folded into S-050 as the dated addendum)** (the CHK-chain decode addendum for S-050);
**I-036** (the default-on posture is CLI-app-level only). Key architecture decode
(this session, lane C): EUUserData's sole og consumer = DRWUI's CHK repair chain
(renames identified .chk fragments by hex-prefix adapters); GetRawUserInfoMap = the
per-scan carve-statistics OUTPUT map (cleared per searcher init, read post-scan) —
never an engine input; NO writer exists for EUUserData in any og binary.


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
- **A25** — ~~M-005~~ **FIXED (s14, FIXED/M-005)** (the allocation-bitmap gate: CarveFlags.free_map -> both dispatch layers; used clusters suppressed wholesale, cluster-aligned jumps; absent/invalid map carves everything — the og's raw-disk rule)
  scans).

## State machine

- **S32** — `M` per-window checkpoints + 50 ms watcher thread; abort flag per byte
  offset (parallel mode).
- **S33/S34** — `M` pause-aware worker heads (G-07); no intra-carve checkpointing
  (stage-level checkpoints cover it).

## Performance

- **X47/X51** — `M` 1 MiB windows + 4 KiB overlap; P1-12 probe blocks (1 MiB windowed
  field scans replacing per-512B stepping); raw-sign 512-byte strides; SHA-256 hashing
  of small files (≤8 MiB re-read). **M-005** cost note (whole-slice carving ≈ 2× the og — **CLOSED s14**: the gate halves dispatch on 50%-used volumes; measured 1.88x/1.89x
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
| M-005 | MAJOR | Whole-slice carving vs og's bitmap-gated free-space pass — **FIXED (s14: gate in both dispatch layers + telemetry; bench 1.88x/1.89x on 50% used; suite 575/575)** |
| N-008 | MINOR | ZIP padding cap off-by-one + alignment; v0x62/ZIP64 chain semantics |
| OQ-T8 | tracked-open | Dispatch order (u16→u32→signs); JPEG full grammar; no-bitmap og behavior |
| N-139 | MINOR | **(session-11 infra walk)** Carve OOM policy: og logs 5-line memory diagnostic then non-continuable SEH; core silent — and the core's own two paths disagree (nothrow→ERR_NO_MEMORY vs the two throwing buffers' uncaught bad_alloc) |
| N-140 | MINOR | **(session-11 infra walk)** Per-extension carve-result aggregation surface (og GetRawUserInfoMap + DRWUI report breakdown) absent — CarveStats scalar-only, no API tallies carve_format |
| I-031 | INFO | **(session-11 infra walk)** The RAWSearchDLL infrastructure register: pending-16 cap/oldest-first eviction, Stop() flushes partials, engine emits no progress, 512-sector refusal, the full 36-slot controller vtable, the singleton layout |

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
- **S-018 RE-FILED CONFIRMED + FIXED (s31, OQ-S3-JPEG resolved — the INVALID
  above stands for the MODERN tier only):** the legacy FileJPGAnalyser tier
  salvages no-EOI JPEGs. The dispatcher FUN_18004b170 branches JFIF/Exif/
  generic; all three sub-analysers' header walks are SOS-gated (SOF0
  precision-8 gate, SOF2 dims fill-if-empty) and the shared post-SOS walker
  FUN_18004c480 (:57942-57991) ACCEPTS AT ITS SCAN LIMIT with no EOI
  (:57958-59 `if (limit <= off || get() == NULL) goto ACCEPT` → the 0x23
  stamp + size = the offset reached); the limit is h*w*3 from the candidate
  start (dims from SOF) or 20,000,000 (the 200,000,000 global cap / 10);
  the sliding getter FUN_180042930 is device-bounded; the census-documented
  two-generation chain makes one accept sufficient. validate_jpeg now
  mirrors the salvage on every no-EOI exit (SOS-gated; end = min(hit +
  h*w*3 | hit + 20,000,000, bound); PartialRisk 600 — the registered
  grading delta D-79). 2 fail-before tests + the EOI pin; evidence:
  s31-oq-s3-jpeg-fallback-decode.md.
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
- **M-005 now decompile-proven and FIXED (s14):** og bitmap-gates carve (bit test + cluster skip +
  dispatch suppression); core has no gate — full standalone finding file created.
- **S-007 evidence completed both directions:** all 8 session-1 formats confirmed +
  ~26 additional og-only formats (bytes cited); ISO9660 is core-only (og cannot carve ISO).
- **N-014d filed:** ZIP padding semantic inversion (og salvages non-empty trailing sectors
  after broken chains, never inflates clean archives; two-tier version gates) — N-008 refined.
- **M-021 filed → FIXED (s21):** video-repair pre-scan wiring — versioned
  `CarvePostProcessor` bound through `CarveFlags::post_processor` (null = the og's
  DLL-absent degrade); fires per validated candidate pre-insertion on BOTH emit
  layers (grammar + raw-sign); a feed never a gate; findings/FIXED/M-021.
- **N-014b/c:** N-014b FIXED (s28) — the naming split is closed: the raw-sign layer
  rides the same namer chain as the grammar layer and the orchestrator's
  "Carved%06u" lambda is removed; one File%06u family per scan
  (Orchestrator.CarveSingleNamingFamily, fail-before showed the split). N-014c
  FIXED-ALREADY via M-024 (CarveFlags.disabled_formats — the RawConfig.ini [Filter]
  surface). N-014d FIXED-ALREADY via N-008+N-068 (salvage polarity + LFH gates;
  cap 0x500001). N-014a FIXED-ALREADY via N-016. Group resolved: findings/FIXED/N-027.

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
- **S-018 (SEVERE, re-filed s31 — FIXED):** JPEG accept policy — the modern
  tier is EOI-gated (the s3 INVALID correction stands for it), but the LEGACY
  fallback tier accepts tail-truncated JPEGs at its estimate limit; the core
  now mirrors the salvage (PartialRisk-graded).
- **M-024 (MAJOR):** no runtime format-filter surface — the og rebuilds its
  registry per search from `config/RawConfig.ini [Filter]` (37 named toggles,
  STL/VPJ default-off) and exports three registry projections
  (**N-033**: consumed by the FS SearchDLLs, which clone the master registry into
  per-engine state); the core's table is compile-time with no enumeration ABI
  (A42/A45).
- **M-025 (MAJOR) — FIXED (session 32, DFX32):** read model — og = 4 MiB device
  reads into an 8 MiB ping-pong buffer, one dispatch per sector-aligned
  position, allocated-cluster skip, open-ended formats tracked as pending
  CRawFileInfo objects extended across chunk boundaries (streaming state
  machine); core = 1 MiB windows, byte granularity (superset, I-009 kept
  deliberately), stateless validators. The fix landed the og's
  buffer-residency BENEFIT without the state machine: a WindowView
  (IStorageDevice wrapper) serves all 8 device-read sites of the window scan
  (validator probes, M-051 checksums, small-file hashes, raw-sign tail walks,
  post-processor feeds — both emit layers) from the already-read window,
  split-reading only the cross-window tails. MEASURED (scripts/bench_m025.cpp,
  64 MiB / 1280 candidates, sign-quiet fill): read_at calls 20,544 → 2,080
  (9.88×), device bytes 2.20 GB → 1.06 GB (2.08×), no wall regression on the
  memcpy device (the real-transport gain is the ~18.5K fewer read round trips
  per 64 MiB). Fail-before: the counting-device test (82 reads pre-fix vs 1).
  The pending-object model itself is registered as PARITY-DELTAS D-80 — future
  stateful analysers (the og's MovAnalyzer/MTS family) port behind the M-021
  CarvePostProcessor seam, not by rewriting the scan loop; the M-021 hook as
  landed is a per-candidate feed and never depended on continuation state
  (the finding's dependency note is resolved by that shape). The geometry
  change (4-8 MiB ping-pong) was evaluated and NOT taken: dispatch cost is
  byte-granularity-dominated (the I-009 superset), and the window size only
  affects the re-read margin the view already removed.
- **Dispatch chain semantics (addendum to N-031):** u16 equal_range chains iterate
  in insertion order; plain-accept-after-special-reject records format id 8
  (generic) and continues; `00 01 00 00` = TTF (u32 layer) pre-empts MDB (u16).
- **S-007 correction (this leg, decompile re-verified at :1985/:2156-2177/:3097):**
  the addendum above carries two wrong rows — C5 D0 D3 C6 is **CEPSAnalyser**
  (AI/EPS), not dBase (no DBF analyser exists); E4 52 **5C** 7B is **OneNote**
  (OneAnalyser/FileONEAnalyser), not LZH (LZH = list-layer LZHAnalyser). Errata
  rows 22-28.

- **N-031 FIXED (s28):** the og's all-zero-sector fast-skip landed at the raw-sign
  layer (exact form: a stride whose 47-byte sign window is all zero skips the
  probe_header walk; counted in ScanStats.carve_zero_strides —
  CarveEngine.ZeroSectorSkipStats with a non-zero control); the 512-vs-device-sector
  stride superset documented at cr_rawsigns.h; the only_common gate is expressible
  on M-024's disabled_formats surface (registered); the collision-order pins
  (TiffVsRawTiebreak et al.) stay green. Resolution in findings/FIXED/N-031.
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
- **I-027 register note (the filter_generalraw VALUE, session-8 sweep)**:
  the concrete junk list behind the display-name mapping — the 65-extension
  `filter_generalraw` value from `remote_config_data`
  (`loggz, crash, evtx, lrprev, tmb, peak, default, sqm, pf, icns, …,
  sqlite, m3u, amr, moff`) — on-file here as the ground truth for any
  host-side general-raw display bucket. The semantic (display-name mapping)
  is the OQ-S4-2 decode above; the value is the session-8 datum.

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
~25 niche og-only formats (**landed since: school 1 `b92ebef` CHM/ONE/MNY/
EMF/SWF/3DS + M-081 `9e5dedc` CRW/MRW/LowDoc/HLP + M-090 `9daa787b` DMG (S11 repair: 771e2ec phantom) +
M-084/M-091 `16475b8` MDB/ACCDB — repaired SA-10; the remainder is the
school-2/3 set, each with its own finding) + the bench seed-matrix extension
(Windows runner).
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
## Session-12 carve-minors batch (N-030 — N-069 landed by the parallel s11 lane, canonical b8b0318)

- **N-030 (D15/B3/D13) FIXED as the registered deliberate superset:**
  the raw_signs.json confidence field is DEAD DATA in the og (parser
  FUN_18001e510 consumes offset + hex body, never part[2] — decompiled
  :23522-23765; the deep-dive's ranking claim corrected, erratum 19).
  Option (b) of the finding's own fix plan: cr_rawsigns.h's divergence
  block now carries the dead-data citation + do-not-remove guidance;
  PARITY-DELTAS D-66; winner selection and the tail-pair blend pinned
  by the existing RawSignLayerHeaderOnlyAndSharedMagic /
  RawSignTailPairWithinFormat tests. Option (a) (table-order parity)
  stays gated on decoding the og's own emitted grade — FUN_18000d670's
  result fill — recorded as the revisit precondition.

**N-068 (MINOR, S5) — FIXED (s11; EOCD half FIXED-ALREADY):** the LFH
anchor now carries the og's two candidate gates (decompile :18385-18388
byte-exact: u16@+8 <= 10 and the [1, 0x103] window at +0x1a — the
artifact's labels are off; E44), applied in zip_common so FMT_ZIP and
FMT_DOCX share them. The EOCD-anchor recall claim is covered by the
existing FMT_ZIP_CD + EOCD reconciliation (head-destroyed zip
reproduction passes pre-fix); a registry EOCD row would double-emit or be
dead code — the analysis is recorded in the finding's Resolution.
B1/A28 verdicts: MATCH.

## Session-22 carve batch (N-070 + M-056 — the overlap-scoring wave)

- **N-070 (A28/A29/D15/B3) FIXED (half 1 implemented, half 2 registered):**
  the grammar-layer trial loop now ACCUMULATES every accepting candidate
  at a magic position and emits the highest `base_confidence + validator
  confidence` one (ties: registry trial order — deterministic); the og
  ranks overlapping candidates by accumulated header+tail confidence
  (searchdll-family--architecture.md:73, re-verified first-hand). Every
  gate's break/continue semantics preserved exactly (already-emitted
  break, suppress_overlap mid-stream skip, F9 dedup resolution). The
  two-generation `File*Analyser` fallback chain is REGISTERED as
  PARITY-DELTAS **D-75** (legacy bodies undecoded in the corpus; the two
  fully-decoded legacy grammars — MKV vint walk, RAR4 fast validator —
  are already the core's validators). Tests:
  `CarveEngine.OverlapScorePrefersHigherConfidence` (fail-before: trial
  order emitted CR2 1700 over DNG 1760 — the discriminator fixture),
  `CarveEngine.TiffVsRawTiebreak` (the census's collision family pinned,
  deterministic single-acceptor outcome unchanged).
- **M-056 (NEW-FINDING, filed+fixed in the same batch):** tiff_common's
  out-of-line-entry end was computed ABSOLUTE but consumed RELATIVE —
  every TIFF-family carve with count>4 entries (all real multi-strip
  TIFFs: StripOffsets/StripByteCounts) sized at `hit + real_size`, which
  on real disks clamped to the 256 MiB cap and absorbed the neighborhood.
  `data_at = u64(val)` (TIFF-base-relative, matching the inline branch).
  Both new tests pin exact structural sizes (1208 / 1210); fail-before
  executed on the stash-isolated pre-fix tree with the hit-inflated
  values quoted in the finding. A28/B1 verdict: MATCH.

## Session-17 fix row (2026-10-26, DFX17 — N-063 closed)

**N-063 FIXED** (findings/FIXED/): the og's per-analyser carved-file
naming landed — `CarveResult::embedded_name` carrier with emit
precedence (embedded_name > namer callback > default). The MP4/MOV
template is decode-pinned first-hand: "Lost Name File (<time>)_EaseUS"
with <time> = "%04d:%02d:%02d %02d:%02d:%02d" of the RAW mvhd value
(v0: BE u32@+0x10; v1: the FIRST FOUR BYTES of the u64@+0x14 — the og's
high-word quirk, every sane v1 time formats as the epoch) fed as Unix
seconds with NO 1904-epoch conversion — both og quirks replicated and
pinned by test, never "corrected". The field is the MODIFICATION time
under the ISO layout (the finding's "creation" label corrected by the
decode). The false "corpus policy" comment on default_namer repaired;
File%06u is now the REGISTERED deliberate default (PARITY-DELTAS D-76,
with the UTC-vs-host-local template formatting divergence). 4 tests,
fail-before 3/3 RED. B10/A28 → MATCH for the decoded surface; the
CZeroEnd-family membership beyond MP4/MOV is the recorded remainder.


## Session-33 verdict update (2026-10-26, DFX33 — M-025 FIXED)

**X51/X47 → MATCH (post-fix, the read-model half):** scan_one_window now
wraps the slice in a window-serving device for the window's duration —
reads inside [base, base+want) are served from the window buffer, reads
straddling a window edge split into buffer-prefix + device-tail
(byte-identical by construction). Validators, both M-051 checksum sites,
both hash sites and the raw-sign tail scans all read through it. The
counting-device execution the finding's own test list prescribes:
**52 → 4 device reads** on the 4 MiB / 4-plant fixture (pre-fix: 4 window
fills + 48 per-candidate re-reads; post-fix: fills only — 13x fewer,
zero per-candidate re-reads for in-window candidates). Geometry (1 MiB
windows + byte-granular dispatch) intentionally unchanged: the finding's
fix-plan item 3 requires bench evidence for geometry changes, and the
executed-transport comparison is the Windows bench lane's job (the
finding itself records no og-execution possible here; M-009 posture).
The I-009 mid-sector superset stays. Stateless-vs-pending model
difference registered (PARITY-DELTAS D-81) — the porting boundary the
S-017 plan must respect.


## Session-33 verdict update, part 2 (2026-10-26, DFX33 — M-048 FIXED)

**B1/A28/D16 → MATCH (post-fix):** the user-extensible signature database
is live: the og's EUUserData JSON shape parses (targeted reader, atomic
validation), registers through the cr_ui ABI (since 2.1) or the CLI
--user-signs flag, and probes at the raw-sign stride AFTER the built-in
layers (built-in-wins collision rule). Emission carries the user's
extension + MIME (carve_note), tag 0xFFFE0000, confidence 600 (the
documented fixed value — EUUserData carries none). Source-of-truth
posture is the og's own: an install-tree config re-read per scan, not
session state.

## Session-24 verdict update (M-039 FIXED, DFX24, 2026-10-26)

**Cross-engine occupancy feed — `M-039` FIXED.** The orchestrator
aggregates every STRUCTURED node's extents (post-engine-pass, from
the result tree) into a sorted, coalesced interval set and feeds it
to both carve dispatch layers as a skip gate: positions inside an
interval are suppressed (the grammar layer jumps to the interval
end); the raw-sign 512-strides suppress the same way; carved nodes
never feed the gate; the dedup tree stays the safety net.
`occupancy_skipped` telemetry on CarveStats/ScanStats. A28 (carve
target policy) / B29 (dedup, net) / O37 (cross-stage data flow)
verdicts: MATCH (the interval shape is the finding's own sanctioned
first step; the og's sparse-file mmap stays profiling-gated).

---

## Dev-sign session wave (2026-10-28, audited `dd82db9` — the never-cited family artifacts' first read)

The session's corpus inventory found the six RAWSearchDLL family artifacts
(bruteforce/filegen/rawspeed/xjq/special/brance — 131 analyser classes)
NEVER-CITED by any prior finding; S-007's format additions were evidenced from
the registry census, not the family semantics. The class-by-class comparison
(evidence in this wave's findings):

- **A-010 (ACUTE, FIXED)** — `validate_asf` ended the carve extent at the ASF
  **Header Object's** size field; the og's CAsfAnalyser is a BruteForce walker ("keeps
  consuming until the grammar breaks, then reports the last valid position as
  the file size" — rawsearch-carving--deep-dive.md:87-91). Every ASF/WMV/WMA
  carve was a header-only fragment. FIXED: the object walker (5 known
  top-level GUIDs + size gates + device-bound torn-tail salvage); A28/B2 cells
  → MATCH (4 fail-before tests in test_carve.cpp).
- **S-037 (SEVERE)** — two formats escaped every format list: FreeHand 11
  (CFH11Analyser, key 0x11C = bytes `1C 01 00 00`, registry id 0x1d,
  decompile:1952-1961) and Corel PhotoPaint 3 generations (CPT7/8/9 =
  0x37545043/38545043/39545043, census:93/:128). Core: FH11 exists only in
  grading/filter tables; CPT absent entirely. A28/C19 cells → SEVERE.
- **N-086 (MINOR)** — the base signature table is compiled-in generated code
  where the og loads install-tree raw_signs.json at engine init
  (deep-dive:54-59); unregistered posture divergence. R56/Q68 cells → MINOR.
- Class disposition: 131 classes — 34 MATCH, 3 DIVERGENT (all registered
  deliberate deltas: ADTS/MP3 D-57, DWG D-80), 67 NOT-IMPLEMENTED (64 covered
  by S-007's open remainder), 1 UNDETERMINED (CFCPEvt attribution →
  tracked-open OQ-SA-2), 26 N/A (infrastructure/harness). No new semantic
  divergence among the implemented validators — every evidenced difference
  maps to an existing finding.
- Residual decode questions (AAC/M1V/RIFF tier roles, WPG size stamp,
  CNeedRawMgr gating, CFCPEvt) → `tracked-open/OQ-SA-2-carve-family-decode-candidates.md`.
- CORRECTIONS #5 propagated: the RawConfig [Filter] count is 34, not 37
  (M-024 addendum; matrix/SUMMARY/evidence notes updated).

## Fix-session delta (N-086 closed — 2026-10-28)

- **N-086 → FIXED (registered)**: the compiled-in raw-signs posture
  registered as PARITY-DELTAS D-103 (option (a): reproducibility over
  field mutability; the usersigns surface — the shipped default set +
  --user-signs + the cr_ui ABI — covers the runtime-extension story;
  option (b) is the tracked design with its conditional test pair).
  No code change: a registration fix.

## Fix-session delta (M-065 closed — 2026-10-28)

- **M-065 → FIXED** — the shipped default set: data/userdata-defaults.json
  (the og's own 111-entry/190-sign EUUserData, canonicalized; parses clean
  under the S-039 caps), the rrecov engine-init load (exe-adjacent file,
  explicit --user-signs replaces, corrupt = degrade-empty, absent = one
  log line), the release.yml staging (beside rrecov.exe + data/), 2 tests
  (DefaultSetShipsAndLoads 111/190 + OgAbsentFromStockExtensionsCarried).
  OQ-SA-3 resolved en route: GetRawUserInfoMap is consumed by DRWUI at
  scan setup (Trigger_Raw/FileSystem config tree) - capability-
  configuring, MAJOR stands.
  **[D-104 supersession note (N-095 item 3): the be0cfe9 corpus decode
  REFUTED the "capability-configuring" reading — GetRawUserInfoMap is the
  per-scan carve-statistics map (FUN_18000ed00 insert-or-increment +
  searcher-init clear; the DRWUI post-scan report reader). The "MAJOR
  stands" verdict above is SUPERSEDED; OQ-SA-3's consumer question is
  closed by CORPUS-ERRATA E-A/E-B + PARITY-DELTAS D-104.]**

## Fix-session delta (S-037 closed — 2026-10-28)

- **S-037 → FIXED** — FH11 (key 0x11C, the og's FUN_180029f30 constant
  predicate: u64@+4 0x020014011C040002, u32@+0xC 0x011C1400, u16@+0x22
  "11") + CPT7/8/9 (keys CPT7/CPT8/CPT9 -> the shared FUN_180043630 gate
  "FILE"@+4 + 0x13C@+0x34 + the FUN_1800436e0 pointer-chase) implemented
  from the decompile; ids APPENDED (FMT_FH11/FMT_CPT7/FMT_CPT8/FMT_CPT9);
  trial order extended; 4 fail-before-proven tests; suite 914/4/0.

## Fix-session delta (S-039 closed — 2026-10-28)

- **S-039 → FIXED** — the user-sign caps raised to the og's measured EUUserData
  maxima (48/12/1024/80 vs the og's 44/11/526/73), the four false justification
  comments replaced with the measured values, atomicity kept. The og's own
  111-entry DB now parses, installs, and the CLI proceeds to the scan (the
  pre-fix exit-1 abort happened only on the over-cap atomic reject). Three
  fail-before-proven tests (`ParseOgShippedExtremes` RED on the pre-fix build,
  `HostileBlobStillAtomic`, `RegistrationRoundTripProceedsToScan` — the
  offset-526 sign fires at the stride). M-065 (the default set) is unblocked.

## Dev-sign session waves 3+5 additions (2026-10-28, audited `dd82db9`)

- **S-039 (SEVERE)** — the user-signature caps reject the og's own EUUserData
  (15/111 violate: 44-byte signs vs 16, 11 signs vs 8, offset 526 vs 512,
  mime 73 vs 63 — measured first-hand); the atomic parse loads 0 entries and
  the CLI aborts the scan (`rrecov.cpp:922-931`), in an error message naming
  "the og's EUUserData JSON shape". The header's "og's longest" comments are
  factually false.
- **M-065 (MAJOR)** — no default user-sign set ships (the og consults 111
  install-tree defaults; 79 extensions stock-absent from every core source);
  severity calibrates on OQ-SA-3 (consumption mode).
- **M-064 (MAJOR)** — the driver deployer's versioned/verified contract
  absent (see modules/driver-install.md).
- Wave 6 (WinPE payload census): the PE capability boundary (zero .mo = no
  lost-partition search in the bootable edition; FREE-by-construction) is
  DISPOSITIONED as the boot-media builder's design input — no carve defect.


## Session-8 section (2026-10-28, audited `35adcb9`)

Findings touching this module (currency pass SA-10): **S-041** (FIXED-as-umbrella
— retired to history this session; every still-open school-2/3 format child-filed:
S-047/M-092/M-093/M-096/M-098/M-099/N-111/N-112/N-113/N-115), **M-081** (FIXED
`9e5dedc` — the CRW re-attribution with the og HEAP grammar landed; repaired
SA-10: the open "fix = re-attribute" phrasing above predated the landing),
**N-093** (batch-1
bounds — MID max 64 MiB vs og 10 MB, GZ 512 MiB/18 vs og 400 MB/20,
undocumented supersets; PSP/SIT under the recorded heuristic but their og
bounds were present at the ctors). Verification: batch-1's 27 magics
re-checked byte-exact at 12 representative registration sites. The
`modules/carve.md:546-548` stale OQ-SA-3 text → **N-095** item 3.


## Session-8 wave-2 (audited `ff879f9` lineage, corpus `be0cfe9`; IDs reconciled past the parallel deep-audit lane)

- **S-041 addendum (SEVERE)** — this lane completed the batch-2 decode the
  deep-audit lane's owner finding pinned: the seven text-school grammars
  (VCF/GPX/HIK/OSP/POS/AEP/CHFP), the 18-site config-gated registration
  zone, registry geometry (59 u32 + 35 u16 bindings), the u16 BOM battery,
  FCPEvt end-to-end. Evidence:
  `../evidence/decompiled-quotes/s8-batch2-formats.md`.
- **S-042 (FIXED this session)** — WMF placeable grammar contradicts the og
  (checksum word@+20 == XOR(words 0..9), reserved dword@16-19, tail word, exact
  size std_words*2+22 vs the core's inch-gate/checksum@18/XOR 0..8/probe-extent)
  — disjoint accept-sets; the batch-1 landing's checksum claim refuted. **FIXED:
  both WMF validators re-derived from FUN_1800264d0 (the placeable gate set
  above; the std branch's invented version/objects/maxrec gates removed — the
  og gates key+params only); fixtures regenerated from the canonical layout;
  WmfPlaceableOgGrammarCarves / OldCoreGrammarRejected / TwentyByteVariantRejected
  / WmfStdOgGatesAcceptNonCanonicalHeaders prove it. findings/FIXED/S-042.**
- **M-084 (MAJOR)** — the MDB family's second signature "Standard ACE DB"
  (ACCDB, type 0x4C) missing.
- **N-099 (MINOR)** — batch-1 citation truth pass (registry regions
  :1728-2509/:2536-2911 with 59+35 bindings; WPG ctor at :15700-01 — also
  correcting N-093's inherited cite, erratum E63; the WPG constant is
  0x00677077 LE).
- Progressive carve-tree growth (UIUX/30:14) dispositioned MATCH this
  session: the core's carve stage attaches nodes live during carve_slice
  (cr_scan.cpp:1877-88) and counters refresh at progress ticks — same
  live-growth behavior as the og's advanced pass.


## Session-8 lead-lane wave (2026-10-28, audited `ff879f9`, corpus `be0cfe9` — the final
push-window numbering, reconciled over the deep-audit/fix/third lanes)

**Batch-1 verification failures the parallel waves did not cover:**
- **M-090** — the DMG extent-walk/cap divergence: the og walks the "koly" UDIF trailer
  (+0x200) with DMG's own 0xC800000 bound (:49436-49440/:63806) — core walks the bzip2 EOS
  (+10) with the global 0xBEBC200 cap, truncating real DMGs before the mandatory footer
  (erratum E67). NOT covered by the parallel lanes' batch-1 truth passes.
- **M-091** — the MDB page-chain size walk DECODED (0x1000 strides, page-type word <=
  0x901, cap 0x6400000, :52001-52039): the landed row's "page-chain walk is undecoded"
  comment is falsifiable now; the 64 MiB bounded-extent stand-in sizes nearly every real
  Jet DB wrong. (Complements M-084 — the ACCDB second signature.)
- S-042's lead addendum — the WMF convergent decode + the two fix-criteria additions:
  the exact Size x 2 + 22 formula; the invented inch-range gate; the fixture-regeneration
  requirement (fixtures from spec, never from the implementation).

**School-2/3 fix units** (children of S-041 per its addendum #2, grammars decoded):
**S-047 FIXED this session** (FMT_TS/FMT_M2TS: the 0x47 sync scanners — 188/192-byte
auto-detect by anchor offset, per-packet verify, null-packet tolerance, 0x5000 min,
x10 cap, m2ts extension stamp; tests Carve.MpegTs188*/M2Ts192*/Ts*; findings/FIXED/S-047) ·
**M-092 FIXED this session** (the four u16-key rows <h/<H/<!/FF FE + the
case-insensitive <HTML/<!DOCTYPE HTML/wide probes + the ASCII-only </html>
end-tag walk with sector round-up, 1 MiB cap; the vocabulary-consistency
surface updated — HTML is checkable now, the OQ-FLT-1 registry-derived
design; findings/FIXED/M-092) · **M-093** XML+SVG · **M-096** MOBI ·
**M-098 FIXED (S42 fix lane)** HIK (the private_stream_1 discriminator: the BB->BD skip + the direct-BD form, the 0x1A+gated-BE16 pack walk, the 1 GiB cap-exact, L"HIK" from DAT_18006b428 dumped from the binary, base 700 for the N-070 score contest; tests Carve.Hik*; N-156 discovered+fixed in the same landing — the generic MPEG-PS lengths now read BE; findings/FIXED/M-098 + N-156) · **M-099 FIXED (S42 fix lane)** AEP (the bare-RIFX weak grammar ported as-is, the BE-length + "<?xpacket end" scan with the +6 extent, the zero-run trim, the 30 MiB cap exact, L"AEP" from DAT_180067008; tests Carve.Aep*; findings/FIXED/M-099) ·
**N-111 FIXED session-39** (FMT_JP2/FMT_JPX/FMT_JPM: the "jP  " box key at +4 with the
payload/ftyp/brand gates, the exact-match brand map via format_override (the M-093
pattern), the fully-decoded size machinery — the BE u32 box walk with the len==1
extended form, the len==0 FF D9 EOC scan with the 16-zero back-off (guard
0x1F3FFFFF), and the jp2c-consecutive end rule; ids appended, cap 500 MB; tests
Carve.Jp2BrandMap/SignaturePayloadGate/UncheckedBoxLength/UnknownBrandRejected/
BoxWalkExtent/EocScan/Jp2cFinalAtSliceEdgeFallsBackBounded; findings/FIXED/N-111) ·
**N-112 FIXED session-39** (FMT_LZH/LZH5/LZHD/LZH7: the four
method-key rows, the "-lh?-" gate at +2, the entry-chain walk fully decoded — the
per-method strides from the .data constant dump (the binary read that settled the
DAT_ labels), the ext-flag CRC suppression, the -2/+2 step-back dance, size = pos+1,
cap 200 MiB; the -lh5 hdr-only quirk PINNED as decoded; tests Carve.Lzh*;
findings/FIXED/N-112) ·
**N-113 FIXED session-39** (the six-row text school: VCF's END:VCARD walk with the
multi-card continuation, GPX's declaration+<gpx window + </gpx> walk (cap 100 MiB),
OSP's libopenshot needle with the unique reject-on-missing, POS's needle settled
from FUN_18001a470 ("ProcessOn Schema File"), CHFP's +0x28 BiffProject anchor;
the per-chunk find quirk ported exactly; tests Carve.TextSchool*;
findings/FIXED/N-113) · **N-115** OfficeTagFromXML. · **M-121 FIXED — CONVERGENT (the renumber chain M-117 → M-118 [never landed] → M-121
per SA-9: M-117 was triple-consumed — session-40 suppress-overlap row 442 keeps it;
the parallel fix lane's 1d4c96b6 landing + this lane's correction push)** — the Outlook Express Dbx
analyser LANDED: FMT_DBX_JMF6 ("JMF6") / FMT_DBX (CF AD 12 FE) / FMT_DBX_MARKED
(F7 89 34 2C), one shared validator; the identity probe ported exactly (the
JMF6+`03 00 10 00` u64 0x0010000336464D4A — disasm-settled at 0x180016766/0x18001676f,
the parallel lane's reading confirmed over my first landing's 03 00 01 00 expansion
error; the four OE store GUIDs; the marked 16-byte pattern with byte 6 = 0xEE — the
parallel lane's 0xDE corrected, disasm 0x18001698b `movl $0x6deef0f7`); the plain-form
size u32@+0x7C (clamped per the PST family precedent, rejected below the 0x80 read
bound); the marked-form FUN_180042b30 zero-walk (0 on an all-zero window, else the
non-zero prefix ROUNDED UP TO EVEN — disasm 0x180042bb0-0x180042c07, the `sar $1`
bisection + `lea 0x1(%rsi),%eax` return; the pending-object recompute adapted to the
one-shot validator per D-101); the og's embedded name provably EMPTY (both lanes
converged); tests Carve.Dbx* (9, RED pre-fix — the formats did not exist); the s3
"MISSING (N-026)" markers re-pointed at M-121; findings/FIXED/M-121) · **M-112 FIXED (S42)** — the CSL size branch landed: the CD 01 00 00 key leg's own
size field (dword@0x44c + 0x480, gated by the ctor-pinned 1 MiB cap
[SVGRawFileInfo +0x118 = 0x100000]); the finding's premise corrected (the branch is
the CD01 key's, DAT_180093918 = 0x1CD — binary-dumped; the probe stamps that key
0x5b, DAT_180093940 = the same dword; the "csl"-content leg never fires the walker's
latch); declination falls to the end-tag walk; tests Carve.Csl* + SvgLeg (4, the RED
case stashed-validator-proven); findings/FIXED/M-112) ·
**N-152** (S41: the DXF 999-comment entry key — the og's u16 key 0x3939 → CDXFAnalyser +
the probe's "999\n"/"999\r\n" leading-comment acceptance; the core keys "  0\r" only;
renumbered twice at the push-time reconciliations per SA-9 — the local N-142 lost to the
remote's CrPreview finding, the local N-151 lost to the remote's nested-declaration
re-finding)
The school-1 formats (CHM/ONE/MNY/EMF/SWF/3DS) landed via `b92ebef`; FCPEvt closed
as covered — the lead lane's duplicate filings dropped, its decodes convergent with the
remote errata E56-E66.
## Session-9 addendum (2026-10-28, SA-9 lane — audited product code `35adcb9`, corpus `be0cfe9`; reconciled into the session-8 board at push time)

This lane's carve decodes converged with the parallel session-8 lanes; the
unique artifacts kept (all cited by the reconciled findings):
- The **instruction-level CHM verification** (capstone on
  `og-codebase/RAWSearchDLL.dll` at 0x180016b40 — the u64 GUID compares,
  the stack-constant table; evidence/decompiled-quotes/s9-chm-gate-asm.md)
  — corroborates erratum E66 and the b92ebef school-1 landing's gate.
- The **canonical-layout evidence table + the plant-mirrors-the-bug
  analysis** for WMF placeable (s9-wmf-placeable-gate.md — cited by the
  S-042 SA-9 addendum; the std-WMF stricter-gates observation there).
- The **CRW/MRW registration quotes** (s9-crw-mrw-registrations.md — cited
  by the M-081 FIXED convergence addendum; the MMOR/IIRS/FUJI →
  BruteForce::TiffAnalyzer attribution correction for future key work).
- The **consolidated batch-2 decode table** (s9-batch2-registrations.md —
  cited by the S-041 SA-9 addendum).
- The **MID/GZ ctor quotes** (s9-mid-gz-bounds.md — corroborate N-093).
- The **registry inventory** (evidence/executions/s9-carve-registry-inventory.md
  — the 101-row product inventory + the absence greps, taken at `35adcb9`,
  i.e. immediately pre-b92ebef; school-1 formats are present post-landing).


## SA-10B addendum (2026-10-29 — the real-.rsf session mining)

Two new findings + one executed MATCH, all from the first mining of the
corpus's three REAL og sessions (evidence/executions/s10b-real-rsf-session-mining.md):

- **S-048 (SEVERE) — FIXED (session-40 severe lane)** — the og's carve branch landed in full:
  the `Reconstructed` root (the og's own name; the M-029 config override retained; the legacy
  name stays a recognized resume role) + the 7 EMPTY category skeleton folders
  (Pictures/Documents/Archives/Videos/Audio/Emails/Others - every real session's persisted
  shape, a newly-decoded surface) + the LAZY per-volume group children (the
  "<label> (<L>:" + ")" / " (H:)" / "System Reserved" forms; a carves-free volume gets NO
  child - the G session's shape) + the empty per-ext folders (lowercase) with every carved
  file attached DIRECTLY to the volume child. The volume attribution is the tree position
  (the og's architecture); the query axis is the path-prefix filter. VolumeGroup ranges +
  names through CarveFlags (the M-104 tier names for whole-disk scans; the composed
  label+letter for volume scans - ScanConfig::volume_letter plumbed through the CLI and the
  ABI); ResultTree::add_folder + the marker-2 session record carry the intermediate folders.
  8-test proof set (Orchestrator.CarveVolumeGroup* + CarveEngine.Reconstructed* +
  Crsf.ReconstructedCarveTreeRoundTrip + Query.CarvedVolumeFilterAxis).
- **S-049 (SEVERE) — FIXED (session-40 severe lane)** — the og's universal naming contract
  `<PREFIX>_<%06d>.<UPPEREXT>` (DRWUI FUN_140c44770:1878330-81; prefix = the UPPERCASE EXT by
  default, PER-FORMAT counters — JPG 1..3938 continuous across prefix variants, GZ 1..27,694,
  PNG 1..17,580; overridden by SOF-dims `JPG_<W>x<H>`/EXIF-Model/EXIF-Software (JPEG),
  mvhd-duration `MP4_08m07s` (MP4/MOV), ID3-title (MP3), Title(25-char cap)/Creator-Producer+
  CreationDate (PDF), docProps-created (DOCX), first-CD-entry-with-/-mapping (ZIP), INAM (WAV)).
  The core's File%06u default fires for every format except MP4/MOV — **RETIRED**: default_namer
  emits the og contract; CarveResult::embedded_name -> name_prefix; N-063's MP4/MOV template
  superseded on the carve-tree application (57/57 real rows are MP4_%06d.MP4 / duration forms;
  the engine-side SetName decode stays documented in PARITY-DELTAS D-76). 13 pin tests
  (OgNaming* + duration + FileNnnnFallbackRemoved); PARITY-DELTAS D-76 rewritten;
  B10/A28/D12 cells → MATCH.
- **Executed MATCH**: the raw_signs.json regen sync — scripts/gen_rawsigns.py
  regenerated the compiled table from the og's shipped JSON: 139 formats / 199
  header / 62 tail signs byte-identical (layout-only clang-format packing diff).
  The layer-1 table is in sync with the og's shipped data at the audited tree.
- Corroborations: S-031 (all real carved rows deleted-flagged), S-041 school-2/3
  real-world weight (GZ/SVG/PNG = the og's top-3 carve volumes).


## SA-10 addendum (session 10 — the walk follow-ups owned: M-106 + M-107; S-041 closed)

The carve surface's size-walk residue now has owners:
- **M-106 FIXED (S42 fix lane)** — the school-1 walks landed from the
  first-hand re-read (ONE: the GUID-recheck chunk chain, size u64@+0xC4
  fallback +0x64, 0<size<=2x-device, the break-point size, the 500,000,000
  clamp; 3DS: the MM branch's section walk — the 0x58 shortcut on the
  u16@+0x58 MM marker, the stride u16@+2, the 5x-device cap; the NCSD/CCI
  branch KEEPS its bounded stand-in — the MM-marked walk would reject every
  CCI file, the deviation documented in the Resolution): the M-090/M-091
  truncation class closed on both formats (tests Carve.OneChunkChain* /
  ThreeDsSectionWalk*; findings/FIXED/M-106).
- **M-107** — the batch-1 undecoded-walk residue (ten sites: X3F/CRW/WPG/WPD/
  PSP/SIT/APE/EPS/DXF/DWG + the SWF RECT validation walk), S-007's promised
  follow-ups never previously owned; the X3F grammar corroborated by the
  never-cited FileRepair artifact (x3f-repair--section-checkers-walk.md:9-33).
- **S-041** closed FIXED-as-umbrella (retired to history; the FIXED-LEDGER row
  carries the closure record).

## Session-11D fix lane (2026-10-29 — A-012, DISCOVERED during S-051's verification: the registry row restore)

- **A-012 (ACUTE)** — the 81ee9775 rebase-window conflict sweep (the
  M-093/M-096 push window) dropped M-093's FMT_SVG row from kTable: 119
  rows against FMT_COUNT's 120, the mobi row displaced (id 119 at index
  118), and the F8 gate (`validate_registry != 0` at carve_slice's head)
  rejected EVERY call with ERR_BAD_PARAMETER — the whole carve stage was
  hard-down on main (the pre-existing `CarveEngine.RegistryF8Gate` pin RED
  at `bcf31316`, executed first-hand this session as the fail-before).
  The row is restored verbatim from M-093's landing `ca701711` (the
  direct "<svg" magic, validate_xmlsvg, 1 MiB cap) with an in-table
  provenance comment; the registry is 120/120 clean, `RegistryF8Gate`
  GREEN, and the parallel lanes' 9 residual clang-format violations
  (cr_signatures.h / cr_analysers.cpp / test_carve.cpp /
  test_thumbnail.cpp — their M-093/M-096/thumbnail-ABI landings) swept in
  the same commit so the pinned-version format gate is green. Process
  lesson recorded in the finding: a rebase-window conflict between two
  DIFFERENT table rows must keep both sides.
### Session-11-lead rows (2026-10-29)

- **S-054** (SEVERE): the TS/M2TS cap at 2e9 is 10x the og's 200 MB clamp (the S-047 landing's refuted "x 10" provenance; unpinned).
- **N-149** (MINOR): the filter's always-on set — the og exempts OLE/RIFF/ZIP from every filter (mask 0x1000801); the disable surface can switch them off.
- **N-143** (MINOR): the ISpecial two-step dispatch with the id-8 generic continuation (the D-75 sibling).
- **N-146** (MINOR): the FH11 zero-end stand-in unowned (S-037's purge residue; 32 MiB).
- **M-112** (MAJOR, the S11B lane's convergent filing — this lane's draft N-147 dropped per extend-don't-refile): the CSL 0x44c size formula decoded-but-unported (FUN_180041950 :49749-49828, the span gate + the +0x118 cap check).
- **N-148 items 4-5** (MINOR): the EPS/AI validator caps drifted to 16 MiB vs the 64 MiB rows; six dangling in-code Resolution pointers.
- The **S-050 addendum**: the repair line's second wiring channel (the per-chunk feed, DDC:9623-27) — the post-processor contract must decide it.
- Verified MATCH this session: the whole infrastructure table (17 rows — dispatch order, registry duality, geometry, gates, dedup, progress, cancel) at `evidence/s11-lead-deep-lanes.md` §Lane-A; the family map (5 families -> core schools present).

## Session-42-B addendum (2026-10-29, audited `869da779`, corpus `be0cfe9` — the continuation lane B's carve filings; renumbered at the S42-B merge: N-154→N-158; the M-118 renumber REVERTED per SA-9 [the parallel window fixed the DBX finding under M-117 first])

**N-158 FIXED (session-53: the five branches landed in the N-115 `zip_common` walk —
XMIND (the FUN_180045110 predicate decoded first-hand: `content.xml` + the 300-byte
`xmlns="urn:xmind` window), MMAP, EDDX (both forms), WFP (both forms), DRP; five
appended enum ids + registry rows with the og's own label constants; the
`disabled_formats` toggle gated at WINNER SELECTION — a disabled typed candidate
loses the position to the undisableable generic row, keeping the interior covered
[the emission-time alternative was implemented and rejected on executed evidence:
the raw-sign DOTX claim]; findings/FIXED/N-158 — the original filing text below):**
the og's ZIP
per-entry typing walk carries five further branches N-115's landing did not port —
XMIND 0x86 (the `FUN_180045110` content branch), MMAP 0x87
(`xsd/MindManagerApplication.xsd`), EDDX 0x8c (both rels entry-name forms), WFP
0x8d (both ProjectFolder forms; the label binary-resolved "WFP" at .rdata RVA
0x6d940), DRP 0x91 (`MediaPool/Master/MpFolder.xml`; "DRP" at RVA 0x67c88) —
`RAWSearchDLL_decompiled.c:53025-53100`, first-hand. The core's nine override rows
cover none of them; RawConfig.ini's `[Filter]` keys couple to exactly these ids
(the core's `disabled_formats` surface has no ids to gate). The entry-name half is
a mechanical landing on N-115's existing walk; the XMIND content branch's exact
predicate (FUN_180045110) needs a first-hand re-read before landing.

**Liveness at `869da779`:** every active carve-scope finding re-verified STILL
(M-098 HIK / M-099 AEP grep-zero; M-106's ONE 64-MiB stand-in at
cr_analysers.cpp:5530 read; M-107's 12 `64 << 20` sites counted; M-112's
"decoded only partially and stays unported" comment read at :3631-3632; M-117-dbx (now FIXED by the parallel window, `1d4c96b6`)'s pre-fix grep-zero; N-146's `32ull << 20` at :4379; N-149/N-139/N-140/N-152 grep-zero).
Evidence: `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md` §1.

## Session-43 minors lane (2026-10-29 — the N-152 landing; audited `b00118b5`, corpus `be0cfe9`)

- **N-152 FIXED** (MINOR — the DXF 999-comment entry key): the u16 "99"
  0x3939 second key lands as `FMT_DXF_C999` (the SWF/CWS dual-key
  precedent — the og's :2722-2733 binding stamps the SAME CDXFAnalyser
  idx 0x19 under the u16 magic map), and `validate_dxf` gains the probe's
  comment branch (FUN_1800376f0 :42240-42290, re-read first-hand):
  "999\n" → the 14-byte LF section form "  0\nSECTION\n  ", "999\r\n" →
  the 16-byte CRLF form "  0\r\nSECTION\r\n  ", a forward NUL-guarded
  scan (the og's byte-by-byte memcmp loop; the core's window bounded at
  4 KiB — the comment-to-SECTION span in real exports is tens of bytes,
  the bound registered in the Resolution). Both DXF rows carry
  `suppress_overlap` (the N-151/N-153 repeating-key rule: a comment-led
  document re-matches "  0\r" strictly inside itself; a canonical
  document's interior "99" bytes never re-carve). Tests:
  `Carve.Dxf999CommentHeaderLfCarves` / `Dxf999CommentHeaderCrLfCarves`
  / `Dxf999RejectsNonCommentContinuation` (re-pinned to the og-faithful
  headless-canonical salvage — the interior u32 key claims the position,
  exactly the og's dispatch) / `Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve`.
  Fail-before: 3 of 4 RED (stashed-validator rebuild; the near-miss
  re-pin documented in the Resolution). Suite 1215/1215 (linux-release).

- **N-149 FIXED** (MINOR — the filter's always-on set): the mask 0x1000801
  over id−0x30 (RAWSearchDLL :4412-4418, re-read first-hand — set bits 0/11/24
  → ids 0x30 FileOLEAnalyser / 0x3b FileRIFFAnalyser / 0x48 FileZIPAnalyser,
  the ids re-pinned at the registry builders :2337/:2304-2310/:2243) lands as
  `carve_format_always_on()`: the disable surface refuses to switch the three
  families off (FMT_OLE; FMT_AVI/FMT_WAV/FMT_WEBP — the RIFF key's per-form
  rows; FMT_ZIP/FMT_ZIP_CD + the typed office/ODF rows — the ZIP analyser's
  in-analyser typing, the N-115 decode). The raw-sign LIST layer stays
  disableable (its formats sit outside the og's mask range — the shipped
  [Filter] targets exactly that layer; no raw-sign row exists for the three
  families). The token-split pin: "doc" addresses both the exempt OLE row
  and the non-exempt legacy LOWDOC rows — the exemption is row-level, the
  legacy family stays disableable (the mask protects the OLE analyser only).
  Tests: `CarveEngine.FilterExemptSetAlwaysOn` (RED pre-fix: zip/wav/ole all
  dropped under a naming list; the jpg control still disabled),
  `CarveEngine.FilterExemptSetCoversTypedRows` (the 16-row family mapping +
  the exhaustive complement + the LOWDOC split). Suite 1223/1223.

- **N-146 FIXED** (MINOR — the FH11 zero-end stand-in re-homed + pinned):
  FUN_180029f30 re-read first-hand (:32612-32633 — the constant predicate
  stamps CZeroEndRawFileInfo id 0x1d with NO size walk). The site joins
  M-107's enumeration as its twelfth member (the zero-end family's 32 MiB
  salvage posture — a different class than the batch-1 64 MiB DWG pattern;
  the D-101 decline covers the og's pending-object zero-end metric).
  `Carve.Fh11BoundedExtentPinned` holds the exact constant (fail-before
  proven by mutation: 32→16 MiB RED). The registration gap — the finding's
  actual defect — closed.
### Session-42-C rows (2026-10-29 — the S42-C continuation lane, post-third-reconciliation) (2026-10-29 — the S42-C continuation lane, post-reconciliation)

- **N-157 corroboration** (MINOR, retired): this lane's independent T0 discovery of
  the red main — reproduced by the second method (a clean `git worktree` build of
  the landing commit `67de3276` itself: `Carve.OoxmlRelsPathSplit` FAILED with the
  identical -32 delta, pinning the N-115 Resolution's 0-fail claim false at the
  pushed tree) — folds as the corroboration addendum on the S42 lane's N-157
  (extend-don't-refile; evidence/executions/s42c-red-main-reproduction.md).
- The landing-parity sweep of the freshest carve landings (S-049 naming, S-054's
  successor state, N-112 LZH, N-113 text school, N-115+N-153 OOXML/ZIP, N-151
  nested-declaration, A-012 registry): **all VERIFIED-PARITY at the pin** — the
  S-054-precedent hunt found no code divergence (evidence/s42c-landing-parity.md).
- The window's S-048/M-117/N-152 landings (verified LIVE at this lane's pin,
  landed by the parallel lanes mid-window) acknowledged: the pin-time liveness
  rows are historical; the reconciled tree carries their Resolutions.

- **N-179 FIXED** (MINOR — the S-039 cap-annotation set's false data claim):
  the EUUserData length distribution re-measured first-hand (111 keys / 190
  signs; the >4 set = {"class","manifest"} exactly; max = 8). The
  cr_usersigns.h comment re-truthed with the measured provenance (the exact
  wording: the longest is 8 "manifest", "class" is 5 — 8 = exact fit, NO
  headroom, the boundary fact stated); kMaxExtLen = 8 unchanged (zero
  behavioral divergence — the finding's own verdict).
  UserSigns.ManifestEightCharExtParses pins the boundary from both sides
  (manifest@8 parses; a 9-char ext rejects) so a future caps-tightening
  pass cannot silently break og-stock-DB compatibility.

- **N-168 FIXED** (MINOR — the user-sign bounds-guard superset registered):
  both og guard sites re-read first-hand (DRWUI :1003371/:1003545 — the guard
  demands offset + hex-STRING length while the matcher reads hexstrlen >> 1).
  The core's decoded-byte bound is the deliberate SUPERSET, registered as
  PARITY-DELTAS D-117 (the og's 2× guard is an og defect — adopting it would
  lose the CHK-repair short-fragment matches). No code change;
  UserSigns.ShortFragmentMatchesInCore pins the stance (the 5-byte fragment
  matched, the below-byte-bound reject held, the prevailing state restored).

- **N-143 FIXED** (MINOR — the ISpecial two-step dispatch posture): all three
  cites re-read first-hand (:9877-9883 the two-step, :10020-10039 the id-8
  stamp + chain-continue, :2235-2238 the dual vtables). The DECLINE branch:
  the generic id-8 fallback class stays absent (the honest-refusal doctrine),
  registered as PARITY-DELTAS D-118 (the D-75 family). The pin:
  Carve.SpecialRejectPlainAcceptRefusedByName — the corrupt-RIFF form (the
  grammar-only family; no raw-sign key fires on RIFF — the layer-1 signs
  enumerated this session) is refused by name while the valid WAV control
  carves. The corrupt-ZIP half of the class fires the apk raw-sign row
  (PK\x03\x04 is apk's layer-1 key — the og's layer-1 admits it identically);
  both layers' interplay recorded in the Resolution.

## Session-46 addendum (2026-10-29, audited `659f0f6c`/`fa654673`, corpus `be0cfe9` — the MINOR-first fix lane)

**N-158 FIXED — the OfficeTagFromXML five-branch typing tail** (the S42-B filing
above, resolved): the five typed ids land — FMT_XMIND/FMT_MMAP/FMT_EDDX/
FMT_WFP/FMT_DRP (og 0x86/0x87/0x8c/0x8d/0x91, ids appended; override-target
rows with the og's own display labels; kTrialOrder slots). The XMIND content
probe decoded from disassembly before landing (FUN_180045110: the 300-byte RAW
window at record+30+strlen(name); the 16-byte `xmlns="urn:xmind` needle + the
14-byte .data prolog constant at RVA 0x93980; the already-0x86 return-1
re-stamp at 0x18004516d). The chain walk's stop semantics corrected to the
og's truth: NO stop, last-match-wins within the 32-entry/256 KiB bounds
(:18411-18450 — the N-115-era "map stop" comment was an erratum). The
[Filter] coupling: the conservative row-level gate through disabled_formats
(PARITY-DELTAS D-134; the N-115 nine stay always-on per N-149). Tests:
ZipTypingXmindContentBranch / ZipTypingXmindNeedleMissStaysGeneric /
ZipTypingDocumentToolEntryNames / ZipTypingLaterEntryOverwritesEarlierStamp /
ZipTypingUnrelatedEntriesStayGeneric / ZipTypedIdDisableGateSuppressesTyping
(3 groups RED pre-fix; suite 1209 green). P1/P11/P19/P25/P65 → MATCH.
