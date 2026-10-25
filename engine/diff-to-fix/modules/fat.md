# modules/fat.md — FAT engine audit verdicts

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
  the format's own).
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
