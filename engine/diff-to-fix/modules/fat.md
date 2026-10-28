# modules/fat.md — FAT engine audit verdicts

## S47 addendum (2026-10-29 — the session-47 severe-first fix lane)

**A-014 FIXED** (session-47): the time-word gate landed in the shared
`fat_datetime_to_unix_seconds` — the full DosDateTimeToFileTime reject set
(hour>23 / minute>59 / 2s-field>29 → the zero FileTime via the N-021 mapping),
verified first-hand against the corpus binary's own three zero-on-fail call
sites (0x1800289d1/:a1f/:a51) and the exFAT sibling's FUN_180002f40 boundary
set. The finding's exemplar date word 0x65C4 corrected to 0x64C4 (month 14 → 6).
Tests: `FatEngine.TimeWord*` (5 boundary pins, RED pre-fix) +
`FatEngine.ValidTimeWordBoundaryUnchanged` + the end-to-end
`FatEngine.InvalidTimeWordPersistsAsZeroFileTime`. See
../findings/FIXED/A-014-fat-time-word-validation-fabricates-shifted-timestamps.md.

## S12 addendum (2026-10-29 — the session-2 deep-dive; lane sheets at ../evidence/s12-lane-sheets/)

New active findings on this module: **A-014 FIXED** (ACUTE — time-word validation:
hour/min/2s unchecked in `fat_datetime_to_unix_seconds`, shifted plausible timestamps
where the og zeroes the slot, FAT:32135-54 — FIXED this session: the gate extended to
the time word, 5 tests, findings/FIXED/A-014); **M-127** (the O(n^2) visited-vector
cycle guard vs the og's range tree — stall-class); **M-130** (the dentry
shape-validator family unported: ext-tail, size/chain-XOR, dir, >9-repeat histogram
gates + walk-stop semantics); **N-162** (the og's 32-bit cluster-math wrap registered
— core u64 correct); **N-165** (the "File overflows partition!" scan-time refusal vs
<<<<<<< HEAD
deferred failure); **N-175** (NTRes case folding); **N-176** (the LFN illegal-char fallback).

Fixed since the S12 filing: **N-172** (deleted-first-char `'#'` aligned + the
LFN-checksum recovery registered as the deliberate superset D-120); **N-173** (the
2106/2107 u32 clamp dropped). A-014's convergence addendum #2 (the S52 lane): the
supplementary access-slot pin `FatEngine.InvalidTimeWordAccessSlotStaysReal` rides the
merged tree (the og passes time word 0 for the access slot — it keeps its real date
while the failing slots zero; neither earlier test family asserted it).
=======
deferred failure); **N-172** (deleted-first-char `'#'` vs `'_'` + the unregistered
LFN-checksum recovery superset — closes OQ-T2); **N-173** (the 2106/2107 u32 clamp);
~~**N-175**~~ **FIXED (S46)** (NTRes case folding landed — the four-way fold, NtResCaseFoldFourWay + the BitLocker re-pin); **N-176** (the LFN illegal-char fallback).
>>>>>>> b1b5eba0590fec0a8aca858737462cb41db29e03
Verified-MATCH this session: cluster-offset formula (u64), EOC/BAD sentinels, boot
geometry, first_data_sector, the 28-bit size field ( disproven hypothesis), grading
condition bytes.


Reference: FATSearchDLL per corpus artifacts `SearchDLL/fat-engine--deep-dive.md`,
`fat-exfat-engine-internals.md`, `fat-exfat-engine-algorithms-v2.md`, `CORRECTIONS.md` row 3;
decompiled evidence in `evidence/decompiled-quotes/exfat-dentry-validation.md` (Q2/Q3).

Core (audited commit `67193b6`, includes concurrent GAP-S6-1/S6-2 fix `9fc82b0`):
`src/core/fat/` (3 files, 747 LOC pre-fix + 109 fix lines) + `include/cr/fat/` +
`tests/unit/test_fat_volume.cpp`.

## Behavioral

- **B1 detection** — `M` flavor dispatch (0/1/2), OEM label offsets 0x36/0x52, dual-FAT
  load, bounds math `(c−2)·SPC + first_data_sector` (og :11237 verified).
- **B2 recovery correctness** — `M` chain follow with visited-set cycle guard (og uses an
  RB-tree range set — functionally equivalent); whole-run batch reads (og :11225–11270).
- **B3 false positives** — `M` (deleted chains honesty-capped; inferred contiguous chains
  flagged and never Excellent — GAP-S6-2 fix, in-audit-window).
- **B4 deleted classification** — `M` deleted 0xE5 marker, 64 KB chain guard
  (`0x10000` both sides — og :14634 verified, core `cr_fat_quick.cpp:30`); deleted-LFN
  first-byte recovery via checksum bijection (core `cr_lfn.cpp:69–105`, landed `9fc82b0`)
  — deterministic and probe-verified (mtools, other session); og mechanism undecoded →
  cosmetic first-char question noted in OQ-T2 file. Grading: chain-end semantics
  **N-002**/**I-002**.
- **B5 orphan reconstruction** — `M`-note: og documents "orphan cluster re-linking (pairs
  with RAW pass)" in complete mode; core pairs orphan content with RAW carve at the
  orchestrator merge layer (orchestrator module verdict). No FAT-side contradiction found.
- **B6 post-format/remnant** — `M` per documented corpus behavior (no FAT data-area remnant
  sweep documented in the corpus either; the og's post-format yield comes from RAWSearch —
  see carve module). Bench `quick_format` gap is transport/NTFS-remnant-rooted
  (root-causes.json, other session's lane).
- **B7 sparse/ADS** — `NA` (not a FAT concept beyond chain model).
- **B9 timestamps** — `M` DOS date/time → 100ns-1601 (tenths discarded — 2 s resolution is
  the format's own). Time-word range gates now match the og's DosDateTimeToFileTime
  accept set exactly (A-014 FIXED this session; month/day were already N-021).
- **B10 filename/Unicode** — `M` LFN glue (0x40 last-entry + 0x0F attr checks, og :11442
  verified), 0x05→0xE5 KANJI escape, deleted-LFN recovery (above); 8.3 fallback '_'
  convention when no oracle.

## Constants and formats

- **C19/C20** — `M` (all offsets verified: OEM 0x36/0x52, dentry stride 32, LFN seq field,
  first-cluster +0x1A on 8.3 entries).
- **C21** — `M` FsKindCode Fat12/16/32 = 0/1/2 = og factory codes.

## Algorithms

- **A25/A26** — `M` flavor dispatch → dual FAT → dir walk (CFatFolderAnalyzeCB parity:
  "."-entry signature `0x202020202020202E` + attr 0x10 anchor) → chain follow.
- **A27 fragment handling** — **I-002** (FAT12 BAD canonicalization — pending
  acknowledgement), `M` post-N-002-fix (mask-24 EOC family ends chains
  cleanly, og Next parity; single-entry approximation documented),
  **OQ-T2** (mirror trigger), `M` on deleted-chain reconstruction
  (linked walk + contiguous-candidate completion + honesty cap, GAP-S6-2).

## State machine

- **S32 cancellation** — `M` per-cluster-run cancel (og per-run vt[0x88]; core
  CancelToken::checkpoint per chain step).
- **S35 error recovery** — `M` chain truncation tolerated with size = min(entry size,
  chain walk) (og internals §Recovery-relevant behaviors); **OQ-T2** mirror.

## Performance

- **X47/X51** — `M` whole-FAT-run batching both sides; core reads FAT whole then chains from
  memory (og streams per-run device reads); device-level traffic comparable.

## Quality

- **Q65 tests** — strong (8 areas + GAP-S6-1/2 fail-before/pass-after tests added in
  `9fc82b0`). Gaps: mirror branch (FAT2 fallback) untested; EOC-family variants untested
  (pin via N-002 tests); FAT16 0xFFF7 canonical regression guard.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-002 | INFO | FAT12 BAD canonicalized (chain truncated) vs og raw follow — registered delta |
| N-002 | MINOR | Alternate EOC families → broken chains vs og clean end — **FIXED (diff-to-fix session)** |
| OQ-T2 | tracked-open | CDoubleFATTable mirror switch trigger undecoded |

## Corpus errata candidates surfaced by this wave

- (via exFAT wave, filed here for the FAT-family artifact): none additional this module —
  see exfat.md.

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 invalid-date sentinel — N-010 (MINOR):** og's `FATSearchDLL` calls
  `DosDateTimeToFileTime` then explicitly zeros the props slot on failure
  (`FATSearchDLL_decompiled.c:32135-32138` `if (BVar5 == 0) *slot = 0;` —
  zero FILETIME = `1601-01-01 UTC`). The core's `fat_datetime_to_unix_seconds`
  returns 0 for invalid dates (`cr_fat_wire.h:130`), but the caller
  `cr_fat_quick.cpp:255-262` unconditionally adds `116444736000000000ULL`,
  yielding `1970-01-01 UTC` instead of `0`. The exFAT engine in the same
  codebase handles this correctly (`cr_exfat_quick.cpp:124-125` early-exit
  on `date == 0`). Only the FAT engine has the divergence. One-line guard fix.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-010 | MINOR | Invalid-date yields non-zero FileTime in core (1970-01-01), zero in og (1601-01-01) |

## Session-2 deep pass (T4) — CDoubleFATTable decoded

- **A27 mirror consult** — **N-022**: the og consults FAT2 per-entry ONLY when the
  primary's canonicalized GetNext(cluster) returns 0 (FREE); BAD values are served from
  the primary with no mirror consult (`FATSearchDLL_decompiled.c :54215-54219`, vtable
  layout RTTI-extracted — full decode in `evidence/decompiled-quotes/s2-fat-double-fattable.md`).
  The core consults on FREE *or BAD* (`cr_chain.cpp:104-110`) — superset on torn FAT pairs.
  When the og consults the mirror it GATES the answer (dirty-EOC family + three mirror
  self-consistency cross-checks) rather than serving raw — quoted in the evidence file.
- **Load-time FAT-location fallback** (observation, no finding): when the declared FAT
  region yields an invalid CDoubleFATTable (both children null) and volume flag 0x10 is
  set, the og walks an internal LBA-keyed structure map for an alternative FAT image and
  rebuilds (`:3805-3949`). Worth a capability comparison against the core's quick-format
  remnant path (cr_fat_quick) — tracked as an observation.
- **Child GetNext canonicalization** — CFAT12/16 canonicalize EOC to 0xFFFFFFFF/0xFFFFFF8
  and return BAD raw (`:53783-53808`); CFAT32 is raw (`:53955-53962`). Whole FAT cached
  in memory at construction (single bulk read, `:44687-44781`) with per-flavor
  cluster-count sanity windows.
- **OQ-T2 RESOLVED** (see `tracked-open/OQ-T2-T5-...md`).
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (decompile-exhaustive; quotes in `../evidence/decompiled-quotes/fat-exfat-session2.md`):**
- **A27/B2 → M-012 (new):** mirror policy — og trusts primary on ANY non-zero answer (BAD
  included), scrutinizes mirror answers (shifted-copy/cross-link rejects), has a mirror-only
  mode; core consults mirror on BAD, accepts unscrutinized, fails the volume when FAT1 is
  unreadable.
- **B1/S35 → M-011 (new):** table-size windows — og FAT32 spf up to 300 MiB (FAT16 ≥ 12
  sectors); core rejects > 16 MiB (volume-scan failure class).
- **B1/B3/A27 → M-015 (new):** og validates loaded tables (media 0xFFFFFFF8 canonical,
  cross-link wholesale rejection, 98% watermark); core loads unvalidated.
- **N-002 confirmed** (mechanism corrected — core's 0x0FFFFFFF mask vs og raw + walker bound).
- **I-002 confirmed verbatim** (FAT12 0xFF7 raw). **OQ-T2 RESOLVED** (see tracked-open/resolved).
- **N-010a/b/c/f filed** (flavor boundary 65524, BPB string/bps/spc gates, dentry date sanity,
  mid-chain FREE/BAD handling).
- **Matches confirmed:** EOC-family walker bounds at the directory level, cancel cadence,
  '.'/'..' anchors, LFN slot gate.

### Session-2 addendum (post-reconciliation)
Cross-references: N-010 (BAD-sentinel ordering, parallel leg, FIXED `cb8dafd` — distinct
from this leg's N-023 group), N-021 (invalid-date filetime, parallel leg — distinct from
N-023c's date-sanity dentry gate; both quote different code paths), N-022 (mirror-on-BAD,
parallel leg — the consult-policy sub-case of this leg's fuller M-012), and the FIXED N-002
(mask-24 family; this leg verified the behavior and corrected the citation — see the FIXED
file's session-2 note).

## Minors-fix session update (2026-10-26)

**N-022 FIXED** — the mirror consult now follows the og default (consult on
primary FREE only; primary BAD served directly, chain truncates). The torn-pair
BAD salvage is preserved behind `FatScanFlags::mirror_bad_salvage` (opt-in,
never the default) — D-51 delta 3. `FatEngine.BadPrimaryServedWithoutMirrorConsult`
(default truncation fail-before-proven) + `FatEngine.PrimaryFreeStillConsultsMirror`.
M-012's remaining surface (scrutinized mirror answers: shifted-copy/cross-link
rejects; mirror-only mode) stays open.

**N-023 (group) FIXED** —
- N-010a: `flavor_from_clusters` boundary is the og's `FUN_180007e50` brackets
  (65524 → FAT32).
- N-010b: new `fat::boot_passes_open_gates` (engine open): FS-name string
  (case-insensitive), numFATs {1,2}, bps == device sector, FAT16 root ≥ 224 +
  root×32 % bps == 0, spf ≠ 0 — the og's `FUN_18002f970`, wired into `scan_fat`.
  FAT12 exempt from the string gate (registered superset, D-51 delta 1).
- N-010c: date-sanity dentry gate behind `FatScanFlags::strict_date_sanity`
  (default OFF; the og's own gate is config-gated — D-51 delta 2). Whole-zero
  dates still flow to the N-021 zero-FileTime conversion (og composes both).
- N-010f: mid-chain FREE/BAD content-shape registered (D-51 delta 4, no chase).
- SPC sub-claim corrected: false positive (u8 field domain — the old pow2 check
  was set-equal to the og's); `FatEngine.SpcDomainMatchesOgSet` pins all 256
  values.
- exFAT half (N-010d/e) resolved in modules/exfat.md.

---

## Session-5 addendum (2026-10-26, audited `7cf6754`)

- **Boot-validation gate family — `M-045` FIXED (s10):** corpus re-verified
  first-hand (FUN_004610 + the three validators). (b)(c)(f) had already landed
  via N-034/N-010b — `lost_boot_flavor` IS the og classifier (band retry,
  512-multiple bps, label, numFATs, SPF≤12 cap, cluster cap) and
  `boot_passes_open_gates` carries the engine labels. This session closed the
  three validate_boot residuals that still gated probe_fat: any-512-multiple
  bps (documented 32768 ceiling, PARITY-DELTAS), numFATs==0 hard reject,
  cluster_count ≥ 0xFFFFFF0 reject. FatBps1536Accepted / FatZeroNumFatsRejected
  / FatClusterCountOverflowRejected / FatBps32768CeilingAndNonMultipleRejected;
  3 fail-before proven; suite 588/588. findings/FIXED/M-045.
- **FAT copy-compare reporting** — folded into `M-041` (the check phase):
  the og reports FAT1/FAT2 disagreement (0x1005) as an audit error; the
  core's mirror arbitration (cr_chain.cpp:109-118) chooses silently.
## Session-4 addendum (M-011 / M-012 / M-015 FIXED — the FAT factory trio)

- **B1/S35/X49 → M-011 FIXED:** per-flavor sectors-per-FAT windows exactly per
  the og's child-table validation (FUN_180036e90): FAT12 [1,12], FAT16 [12,256],
  FAT32 [256,614400] sectors (bps-scaled) - the 300 MiB whole-load ceiling
  matches the og factory's own allocation; the flat 16 MiB cap is gone.
- **A27/B2/S35 → M-012 FIXED:** the og's mirror scrutiny ported verbatim
  (FUN_180041de0 :54226-48): backward answers reject shifted copies and
  cross-links, believe only contiguity; forward answers disbelieve mirror
  BAD/EOC; mirror-only scans (child1==NULL) load the raw mirror instead of
  failing the volume. N-022's primary-FREE consult remains the entry gate.
- **B1/B3/A27 → M-015 FIXED:** whole-table validation (FUN_180041b00): exact
  media pair on FAT16 (family-tolerant 12/32), entry sweep (==1/self-loop/OOB
  in [n,BAD)), claims-map cross-link wholesale rejection, mirror promotion on
  FAT1 rejection; the 98% watermark stays advisory (deep-check gate unpinned).
- The FAT engine's damaged-volume posture now matches the og factory: windows,
  validation, scrutiny, retry, mirror-only.
## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| M-040 | MAJOR | FAT recycle-bin `$I/$R` pairing missing (og: IRecycleBinNode2UI bridge, fat-exfat-engine-internals.md:21,37-40; core grep 0 hits; NTFS has the full model at cr_mftwalk.cpp:162-250) |
| N-065 (writer side) | — | (writer module row) — collision/mkdir destination policy |
| MATCH-note | — | FAT alternate-EOC families: N-002's fix VERIFIED live this session (cr_chain.cpp:71-82,166-174 handles 0x0FFFFFF0..F7) — a stale divergence claim from the session's agent read was caught by verification and filed nowhere |

Function-map additions (per-engine-function-map.md, first-cited): `CFATQuickSearchWorker`
threaded quick scan is REGISTERED (PARITY-DELTAS D-30x — not a new finding); `Fat32CalcPartition`
scorer-class evidence appended to M-017. The `FATFilterName` ini runtime-filter surface belongs
to M-024's family (no separate finding).

---

## Session-24/35 verdict update (M-052 FIXED — DFX24 + the s35 convergent landing, 2026-10-26)

**Recycle-bin pairing — `M-052` FIXED.** The fat engine recognizes the
og's four bin roots (_stricmp set verified first-hand, FATSearchDLL
:1668-1677: Recycler / Recycled / $Recycle.Bin / $RECYCLE.BI0 — the
corpus-richer four-root set) and threads the bin context through the
whole subtree via the shared `cr/recycle` helper (the NTFS engine's
reference model extracted to its engine-neutral form). In-bin
`$I<token>` streams are parsed (v1 28+2*pathlen, v2 544 fixed, the
Win10+ variable form's repurposed @0x18 honored) and never surface;
in-bin `$R<token>` files are emitted under their ORIGINAL
name/size/deletion-time, forced Deleted, content extents untouched,
with the post-walk fixup for late-ordered pairs (the og pairs "across
bin subfolders"). Stats `recycle_info` / `recycle_remapped`. Proof:
RecyclePairingRestoresOriginalName + RecycleLegacyRoots +
NonRecycleDollarFilesStayRaw (fail-before EXECUTED — engine mutation
3/4 RED; the over-matching pin green by design). B4/B5/D11 verdict:
MATCH.


## Fix-session delta (N-087 closed — 2026-10-28)

- **N-087 → FIXED**: short_name_to_u16 decodes high-half 8.3 bytes
  through the standard-verified CP437 table (the og persists raw OEM
  bytes, ifileproperties-part1:121; the core's UTF-16 carrier converts
  at the boundary). ASCII unchanged; the 0x05 KANJI escape + the
  deleted-marker rewrite are caller behavior (unchanged) — 0x05 → 0xE5
  → CP437 sigma. 3 tests (2 fail-before-proven RED on the reverted
  build).

- **N-162 FIXED** (session-43): the cluster→sector u64 width registered as
  PARITY-DELTAS D-119 (the og's four 32-bit wrap sites vs the core's u64 — the
  D-56/D-68 numeric-hygiene family's FAT member);
  FatEngine.ClusterOffsetBeyond2TiB is the down-fix guard (the spec-max
  cluster's exact address, the wrapped form explicitly not-equal).

- **N-172 FIXED** (session-43): the deleted-name presentation decoded and aligned —
  the og's placeholder is '#' (FUN_180036350 :44166-44172 re-read first-hand; the
  B4/OQ-T2 cosmetic question closed); the no-oracle branch aligned from '_'; the
  LFN-checksum original-byte recovery registered as PARITY-DELTAS D-120 (the
  deliberate superset, GAP-S6-1's bench-proven bijection). Pins:
  DeletedFirstCharPlaceholderIsHash + DeletedLfnChecksumRecoversOriginalByte + the
  re-pinned #EL.TXT family (the placeholder sorts FIRST at 0x23 — the cr_ui
  name-paging order re-derived).

- **N-173 FIXED** (session-43): the u32 date clamp dropped — fat_date_to_unix
  returns u64 end-to-end (the og's DosDateTimeToFileTime has no 2106 cutoff; the
  FileTimes contract was always u64; the stale comment described a contract that
  never existed). The N-021 invalid-date zero-FileTime semantics preserved exactly
  (0 is now ONLY the invalid class). The FAT/exFAT decoder agreement restored
  (both ride the shared unclamped core). Pins: Date2107ConvertsFullyNoClamp (the
  0xFF9F/2107-12-31 value, the 0xFFFFFFFE boundary, the 0x100000000 regression
  step) + Date2107EndToEndFileTimeIsReal. The finding's 0xFFBF example word
  corrected (it decodes 2107-11-31 — invalid; 0xFF9F is 2107-12-31).
