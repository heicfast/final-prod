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

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 tz_offset — `M` (**S-011 FIXED**: tz subtraction removed from
  `exfat_time_to_unix100` — on-disk time carried through the shared
  FAT-family datetime core exactly like the FAT engine (host-independent,
  on-disk-as-UTC), which is the parity model the FAT engine holds a MATCH
  verdict on. og re-verified first-hand in the fix session: zero TZ-conversion
  imports beyond DosDateTimeToFileTime, six call sites taking (date_word,
  time_word, &ft) only, zero `*900` arithmetic in the DLL. Wire tz fields stay
  parsed, unapplied (og parity); guards: TimestampsMatchOgLocalAsUtc (0x88),
  TimestampsZeroTzPassThrough, TimestampsMatchExplorerDisplay (0x20)).**
  exFAT stores each file's create/modify/access timestamps as local time on
  disk PLUS a per-timestamp `UtcOffset` byte (7-bit two's-complement
  quarter-hours from UTC). The og's `exFATSearchDLL.dll` imports NONE of
  `LocalFileTimeToFileTime`/`FileTimeToLocalFileTime`/`GetTimeZoneInformation`
  (verified by full read of the imports census). The og's `FUN_180001370` at
  `exFATSearchDLL_decompiled.c:5215-5235` calls `DosDateTimeToFileTime` on the
  raw date+time words (host-timezone-dependent local-as-UTC).

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-011 | SEVERE | tz_offset applied (core) vs ignored (og); FileTime diverges by tz hours |

## Session-2 deep pass (T4) — ChecksumDBR skip-set decoded

- **F63/C19 ChecksumDBR** — `M` (MATCH, OQ-T5 resolved): the og's per-byte predicate is
  `std::find` over a 4-int array extracted from RawFixer.dll RVA 0x180079a40:
  **{106, 107, 112, -1}** — exactly the exFAT spec skip-set (VolumeFlags@106-107,
  PercentInUse@112) plus a -1 terminator no loop index matches. Loop = sector_size x 11
  bytes, spec rotation. The core's spec skip-set (`cr_exfat_dentry.cpp:26-36`) matches
  exactly. Evidence: `evidence/decompiled-quotes/s2-exfat-checksumdbr.md`. Corpus erratum
  confirmed: rawfixer-v2--method-inventory.md S4's "bytes 11..209 + 510" is wrong.
- **OQ-T5 RESOLVED** (see `tracked-open/OQ-T2-T5-...md`).
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/fat-exfat-session2.md`):**
- **B2 → S-013 (SEVERE) — FIXED (session 3):** FAT > 32 MiB used to be silently
  dropped → every FAT-chained file/dir truncated to ONE cluster. Fixed with the
  og's windowed accessor (FUN_180021c60 parity: 1 MiB cached windows, no size
  cap) as the ONLY fallback — have_fat is never false while fat_length != 0;
  extent coverage (not logical_size — it mirrors the stream entry) is pinned by
  ExfatEngine.LargeFatFullChains/LargeFatDirectoryWalk; per-window read failures
  degrade to the EXFAT-2 assumed-contiguous salvage; fat_window_refills telemetry
  added. The original B2 bullet for the finding read: FAT > 32 MiB silently
  dropped → every FAT-chained
  file/directory truncated to ONE cluster. og = 1 MiB windowed reads, no cap.
- **B3/B4/B5 → M-013 (new):** og validates entry-set checksums DUAL-FORM (raw + InUse-restored)
  including deleted sets and uses them as a torn-set reconstruction oracle; core = live-only
  downgrade. **N-003's corpus claim REFUTED** (superseded by M-013).
- **B3/B4/B10 → M-014 (new):** structural gates missing (secondary_count ∈ [2,18], timestamp
  sanity, stream fields non-zero → empty files dropped, name-entry validation, root triple).
- **Stream-gate polarity re-confirmed** (core right, artifact wrong — `:1857-59` ↔
  `cr_exfat_quick.cpp:343`). **OQ-T5 RESOLVED** (scanner never gates on boot checksum; fixer
  predicate divergence → N-017).
- **N-010d/e filed** (tz offset core-only; reserved-field byte-OR nuance). Matches: boot
  ladder EXFAT-4, NoFatChain bit, EXFAT-2 assumed-contiguous semantics, checksum-sector
  fill-consistency.

### Session-2 addenda (post-reconciliation)
- **Retraction:** the N-017 candidate (og fixer "no-skip" checksum) is RETRACTED — the main
  auditor's own decompile re-read found the `std::find == end()` skip-table pattern over
  `DAT_180079a40` = {106,107,112} (MS-spec skip); the core's spec-skip is a MATCH. Full
  correction in `../evidence/decompiled-quotes/fat-exfat-session2.md`.
- **Divergence introduced by the N-003 fix:** `fb44073` set `validate_entry_sets = false` on
  a "verified-absence" premise that is refuted by the decompile (`FUN_18001eba0` = dual-form
  checksum incl. deleted sets — quoted in `../findings/FIXED/N-003-*.md`). The og-parity
  direction is validating, not skipping; M-013 carries the correct fix.
- Cross-references: S-011 (tz, parallel leg — cross-confirmed) · M-013/M-014/N-023 (this leg).

## Minors-fix session update (2026-10-26)

**N-010d FIXED-ALREADY** — S-011's fix (D-49) removed the tz subtraction;
the tz bytes stay parsed-but-unapplied, exactly the og's behavior. No further
change.

**N-010e FIXED** — the stream-entry reserved-field tests now use the og's
byte-OR form (`FUN_180003090:1843-1846`, single-byte loads verified in the
decompile): reserved1 strictly zero; reserved2 passes when either byte is 0;
reserved3 when any of its four bytes is 0. Partially-nonzero reserved fields
(BIOS/embedded formatter artifacts) no longer count the set as damaged.
`ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr` (fail-before on
the word-test form) + two guards.

---

## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| M-040 | MAJOR | exFAT recycle-bin handling mirrored from FAT in the og (fat-exfat-engine-internals.md:71) — same missing pairing as the FAT side |
| N-051 | MINOR | Upcase-table validation skipped entirely (TYPE_UPCASE bare-continue at cr_exfat_quick.cpp:319-321; og validates via CExFatVolumeAnalyer, :63-64) |
