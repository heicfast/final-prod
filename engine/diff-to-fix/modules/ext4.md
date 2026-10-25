# modules/ext4.md — ext4 engine audit verdicts

Reference: EXT4SearchDLL (covers ext2/3/4 — DRWUI codes 4/5/9) per corpus artifacts
`SearchDLL/ext4-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md` §1–2,
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md`, `CORRECTIONS.md` rows 4/6.

Core (audited commit `67193b6`): `src/core/ext4/cr_ext4_engine.cpp` (851 LOC) +
`include/cr/ext4/` + `tests/unit/test_ext4_volume.cpp`.

## Behavioral

- **B1 detection** — `M` SB magic 0xEF53 @+0x38, field-set validation
  (inodes/blocks/bpg/ipg nonzero, first_data_block↔log_block_size consistency),
  block size `0x400 << (log & 0x1F)`, 64-bit blocks_hi concat, groups math + sanity.
  **Guard-field conflict settled in core's favor**: the `& 1` guard byte is at SB+0x60
  (s_feature_incompat bit 0 = COMPRESSION), not the artifact's "+0x18 log_block_size"
  reading — agent B's mechanical decode (:3223/:3304) and the core
  (`cr_ext4_engine.cpp:103`, `SB_INCOMPAT=0x60`) agree; the artifact is wrong (corpus
  erratum candidate #7).
- **B2 recovery correctness** — `M` extents + indirect-block ladder (ext3 fallback),
  BIGALLOC refused loudly (`ERR_UNSUPPORTED`) — og BIGALLOC behavior not decoded in corpus
  (noted; refusing is the safe posture, registered in FALLBACKS census).
- **B3** — `M` majority-valid gate on dir blocks (`sane*2 >= total`).
- **B4 deleted classification** — `M` dtime‖links==0 predicate + inode sweep + bitmap
  grading (agent D sheet; corpus does not decode the og's ext-deleted predicate in more
  detail).
- **B5 orphan** — `M` journal-supplied names for unvisited inodes ("InodeNNNN" og naming
  parity per :587–589 comment).
- **B6 damaged-volume** — **S-004** (journal salvage surface).
- **B9 timestamps** — `M` unix→100ns ns precision.
- **B10 Unicode** — `M` post-S-005-fix (shared `text::utf8_to_u16` decoder;
  `Ext4Engine.NonAsciiNamesDecodedNotByteLifted` proves CJK/accent/invalid
  names decode to true code points).
- **B7 sparse** — `M` unwritten-extent semantics (0x8000 hole/extents, :215).

## Constants and formats

- **C19/C20** — `M` (all SB offsets verified against decompile; GDT 32/64-byte entries
  with desc_size from the 64-bit feature flag, matching og feature-correction path).
- **C21** — `M` FsKindCode Ext2/3/4 = 4/5/9 = og factory codes.

## Algorithms

- **A25** — `M` structure walk (SB → GDT → inodes → extents/dirs); **S-004** journal.
- **A26** — `M` htree dirs treated as linear streams (og depth not decoded either — both
  sides linear; corpus does not document dx_dir hash descent in the engine).
- **A27** — `M` extent ladder + journal-extent bounded reads (64 MiB cap).

## State machine

- **S32** — `M` cancel per journal block / per inode window.
- **S35** — **N-004** (CatchBuffer layering), `M` on SB-reject posture (core rejects
  invalid SB like og; no clamp-vs-reject divergence here — both reject on the quoted
  field set).

## Performance

- **X51** — `M` GDT ≤8 MiB, journal ≤64 MiB, 1 MiB window discipline (og 1 MiB journal
  windows verified).

## Quality

- **Q65 tests** — strong (7 areas incl. jbd2 salvage + spec-true inode offsets). Gaps:
  unreferenced-journal-block case (S-004 fix), non-ASCII names (S-005 fix), BIGALLOC
  refusal pinned.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-004 | SEVERE | Journal salvage: descriptor-referenced only vs og every-block (+ jsb gate direction) — **FIXED `51ac671` (session-2 verified)** |
| S-005 | SEVERE | Names byte-lifted → mojibake (cross-engine, filed here for ext4) — **FIXED (diff-to-fix session)** |
| N-004 | MINOR | CatchBuffer backtrack lives in transport layer, not in-engine |

## Corpus errata candidates surfaced by this wave

1. `ext4-hfs-engine-algorithms-v2.md` §1 "+0x18 log_block_size low-bit guard" is actually
   byte SB+0x60 = s_feature_incompat bit 0 (COMPRESSION) — core right, artifact wrong.
2. `ext4-engine--deep-dive.md` backup-SB positions (8193, 32768, …) are kernel-knowledge
   overreach, not in the decompile (CORRECTIONS #4 already says UNVERIFIED; core does not
   probe backup SBs either — consistent).

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 nanoseconds — I-006 (INFO, superset):** core's `decode_extra()`
  (`cr_ext4_engine.cpp:88-93`) extracts nsec from `*_extra` word (kernel
  `ext4_decode_extra_time` algorithm). `unix_to_filetime_ns(sec, nsec)`
  (`cr_types.h:56-61`) preserves nsec at 100ns granularity. Og's
  `EXT4SearchDLL` reads only seconds (`i_ctime` at +0x0C, `i_mtime` at +0x10)
  and uses `_gmtime32_s` (second-granularity) + `wMilliseconds = 0`
  hardcoded — sub-second precision silently dropped. Core is strictly more
  precise; .rsf byte stream differs by sub-second 100ns units.
- **B9 extra timestamps — I-007 (INFO, superset):** core reads all 4
  timestamps (created/modified/changed/accessed) via `i_atime_extra`,
  `i_ctime_extra`, `i_mtime_extra`, `i_crtime`, `i_crtime_extra`
  (`cr_ext4_engine.cpp:283-297`). Og reads only 2 (ctime + mtime) at
  `EXT4SearchDLL_decompiled.c:6262-6282`; the 3rd/4th FILETIME slots
  (`props+0x4b`/`+0x55`) are initialized to 0 and NEVER assigned. Core
  superset — every ext4 file has non-zero `created`/`accessed` slots where
  the og's has zeros.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-006 | INFO | Nanoseconds superset (core preserves nsec; og drops to second-only) |
| I-007 | INFO | Core reads 4 timestamps, og reads 2 (created/accessed dropped) |
