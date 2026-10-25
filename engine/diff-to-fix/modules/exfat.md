# modules/exfat.md — exFAT engine audit verdicts

Reference: exFATSearchDLL per corpus artifacts `SearchDLL/exfat-engine--deep-dive.md`,
`fat-exfat-engine-internals.md`, `fat-exfat-engine-algorithms-v2.md`,
`PartitionRecovery/rawfixer-v2--method-inventory.md` §4; decompiled evidence in
`evidence/decompiled-quotes/exfat-dentry-validation.md` (Q1/Q4 — Q1 polarity settled by
main-auditor read this session).

Core (audited commit `67193b6`): `src/core/exfat/` (2 files, 678 LOC) +
`include/cr/exfat/` + `tests/unit/test_exfat_volume.cpp`.

## Behavioral

- **B1 detection** — `M` boot-region 24 sectors, backup LBA 12, up-case checks,
  `CExFatVolumeAnalyer`-equivalent volume assessment; boot checksum computed
  (`boot_checksum_ok`) but not gating detection — og gating behavior undecoded (**OQ-T5**);
  `M` on the stream first-cluster gate: **polarity conflict settled in core's favor**
  (decompile Q1: valid iff `first_cluster != 1 && < 0xFFFFFFF7`; the v2 artifact's polarity
  is inverted — corpus erratum candidate #6).
- **B2 recovery correctness** — `M` dentry-triple reconstruction (file→stream→names,
  backward walk, `(t−X)&0x7F` class tests — core `entry_type()` masks 0x80 the same way,
  so deleted sets reconstruct identically); NoFatChain contiguous resolution incl.
  multi-cluster directories (EXFAT-6).
- **B3 false positives** — `M` structural validators match og exactly (reserved zeros,
  volume-label triple 0x81/0x82 placement, UTF-16 fragment validation).
- **B4 deleted classification** — `M` post-N-003-fix (entry-set checksum
  validation defaults OFF = og parity; honest-mode strictness registered
  as PARITY-DELTAS D-47); `M` on
  deleted-set acceptance (0x80-cleared dentries accepted through the same class tests —
  core `entry_in_use()`/`entry_type()` masking verified).
- **B5 orphan reconstruction** — `M`-note (assumed-contiguous salvage for unchained
  deleted sets is core-side; og's DelexFat pass mechanics undecoded — folded into OQ-T5
  file notes; no corpus contradiction).
- **B6 post-format** — `M` per documented corpus behavior (no exFAT remnant sweep
  documented in corpus; RAWSearch covers content).
- **B9 timestamps** — `M` exFAT 10ms timestamps with tz-offset conversion
  (`exfat_time_to_unix100(ts, tz)`); og conversion semantics not itemized in corpus — no
  contradiction found.
- **B10 filename/Unicode** — `M` UTF-16 name fragments, checksum-validated glue for live
  sets.

## Constants and formats

- **C19/C20** — `M` dentry types (0x03/0x83, 0x05/0x85, 0x40/0xC0, 0x41/0xC1, File 0x85),
  InUse 0x80, stride 0x20, boot checksum accumulator `(c<<31)|((c>>1)+b)`.
- **C21** — `M` FsKindCode::Exfat = 7 = og factory code 7 (core also writes FS-type field
  0xE in quick-search parity metadata — og `*(u32*)(obj+2)=0xE`).
- **FNV-1a** — note: og engine contains FNV-1a (basis 0xcbf29ce484222325) for internal
  map keying; core does not need it (different internal data structures) — no parity
  impact.

## Algorithms

- **A25** — `M` bitmap-first quick scan (allocation bitmap built during walk —
  `CQuickSearchAndBuildBitmap` parity: core builds `ctx.bitmap` from the bitmap dentry
  chain with FAT/no-FAT fallback and 8 MiB cap).
- **A27** — `M` chain follow + assumed-contiguous salvage (honesty-capped).

## State machine

- **S32** — `M` cancel polling per dentry window (note: no per-dentry cancel inside a
  32 KiB window — bounded, acceptable; og cadence per chain-run).
- **S35** — `M` damaged sets counted, metadata-only emission (EXFAT-1 family).

## Safety

- **F63 (rawfixer coupling)** — `M`: `boot_region_checksum` shared between scanner and
  repair engine ("the fixer and the scanner can never disagree") — see rawfixer module.

## Quality

- **Q65 tests** — strong (6 areas incl. EXFAT-1..4 og gates). Gaps: deleted-set
  acceptance path (0x80-cleared triples) not directly pinned; boot-checksum failure
  behavior unpinned.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-003 | MINOR | Entry-set checksum enforced on live sets beyond og (grading-only) — **FIXED (diff-to-fix session)** |
| OQ-T5 | tracked-open | ChecksumDBR per-byte predicate (skip-set) undecoded |

## Corpus errata candidates surfaced by this wave

1. `fat-exfat-engine-algorithms-v2.md` §3 stream first-cluster polarity inverted (Q1
   evidence) — core is right.
2. `rawfixer-v2--method-inventory.md` §4 ChecksumDBR byte-range description ("bytes
   11..209 + 510") contradicts the verified loop bound (`sector_size × 0xB`).
