# modules/exfat.md — exFAT engine audit verdicts

## S12 addendum (2026-10-29 — the session-2 deep-dive)

New active findings on this module: ~~**N-174**~~ **FIXED (S46)** (the `._` AppleDouble
sink filter landed — the gate at the entry walk post-name-assembly, the
`apple_doubles_sunk` counter, `ExfatEngine.AppleDoublePrefixSunk` +
`RegularDotFilesNotSunk`; corpus cites :45151/:45222 re-read first-hand); **M-127**'s exFAT twin (the
chain-guard complexity); note: A-014 does NOT affect exFAT (the M-014 dentry gate
rejects the invalid-time class upstream). Verified-MATCH: 64-bit cluster math (the og
itself is 64-bit here — the FAT 32-bit sites are the inconsistency), the 0x81/0x82/
0x83/0x85 dispatch, 10ms/tz non-use both sides.


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
- **B9 timestamps** — `M` exFAT 10ms timestamps; the tz-offset bytes are
  parsed but NOT applied (N-180 re-truth per the module's own session-2
  re-audit at :93: the og reads them and converts to local system time,
  which equals the fixed local-as-UTC stamp)
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
| N-174 | MINOR | `._` AppleDouble prefix sink — **FIXED (S46)** |
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

## Session-10 row — M-054 (NEW-FINDING) FIXED

| ID | Severity | One-line |
|---|---|---|
| M-054 | MAJOR — **FIXED** (findings/FIXED/M-054) | Allocation bitmap read at bit `c` for cluster `c` (MS exFAT spec §7.1: bit `i` = cluster `i+2`) — deleted-viability grading AND the landed S-009 free-map producer consulted the wrong clusters on every real volume; both fixtures encoded the same shift (suite self-consistently wrong). `Bitmap::test` reads bit `c-2`; fixtures write the spec-correct bit; `ExfatEngine.FreeMapProducerMatchesSpecBitMapping` pins the producer seam. Fail-before EXECUTED on the landed base (EndToEnd + producer pin both red with the body reverted). |

**N-067 (MINOR, S5) — FIXED (s11):** the volume-integrity upcase check now
exists — the in-use 0x82 dentry passes the og's shape gates (reserved1/2
zero-checked, checksum field free, cluster >= 2; decompile :24440-24452),
then the table stream is chain-read in bounded 64 KiB chunks through the
shared checksum32 and compared against TableChecksum. Verdict surfaces in
`ExfatScanStats.upcase_validated` / `.upcase_mismatches` — a soft signal:
the scan never stops on a corrupt table (EXFAT-2 posture). B1/A25/S35
verdicts for this row: MATCH (grade-not-reject is the recorded honest
default; the og's exact failure path stays an open question inside the
finding). The fixture carries a real 64-entry table at cluster 60.
Preceded by the S-033 INVALID record (the constants were verified right
BEFORE this fix — bitmap=0x81/upcase=0x82/label=0x83 stands).

| N-067 upcase-table validation | **MATCH (grade-not-reject)** | `ExfatEngine.UpcaseTableChecksumValidated`, `ExfatEngine.UpcaseTableValidVolumeUnflagged`, `ExfatEngine.UpcaseLengthSanity` (spec seeded CRC via table::crc32_seeded; ExfatScanStats.upcase_checked/upcase_invalid) |

---

## Session-16 verdict update (M-013 FIXED, 2026-10-26)

**Deleted-set checksum integrity — `M-013` FIXED.** The dual-form
SetChecksum (FUN_18001eba0: raw ROL15 + the InUse-restored form with
0x85/0xC0/0xC1 type-byte substitutions, valid iff either matches) is now
the DELETED-set accept/reject gate in the walk (unconditional og parity —
a failed set is parked, never emitted as a confident node), and the
parked-set reconstruction oracle (FUN_1800203a0's suffix substitution,
re-tested against the stored checksum) recovers torn sets from surviving
orphan stream/name runs. Torn-at-buffer sets are parked rather than
discarded; orphan tails are parked rather than silently skipped. LIVE
sets keep the flag-gated downgrade (D-47, scope-corrected — N-003's
"no checksum loop anywhere" premise is recorded REFUTED). New stats:
sets_rejected / sets_reconstructed. Proof:
`DeletedSetChecksumRejectsTornSet` + `DeletedSetOracleReassembly`
(fail-before executed) + `LiveCompleteSetBadChecksumUngraded` (the
asymmetry pin); the dual arithmetic is on the fuzz surface (fuzz_pure
case 3). Remaining on this module: M-014 (structural entry gates).

---

## Session-35 verdict update (M-014 FIXED, 2026-10-26)

**Structural entry gates — `M-014` FIXED.** The og's four cheap structural
gate families are now in the walk, decompile-exact and applied to live,
deleted AND oracle re-walked sets: (1) file-dentry gates
(FUN_180002f90): secondary_count ∈ [2,18], create+modify+ACCESS
timestamp field sanity (day≠0, month∈[1,12], hour≤23, min≤59, 2sec≤29 —
an all-zero stamp FAILS, correcting the finding's fix-plan guess),
reserved1/reserved2 zero; a failing head is skipped without parking.
(2) stream emission gates (FUN_180029280): data_length, first_cluster,
name_length all nonzero — zero-length files are never emitted (the og
quirk inherited as parity). (3) name-fragment gates (FUN_180003030):
flags&3==0, flags<4, first needed fragment carries a nonzero first
char. (4) the oracle's declared bound tightened 512→19 (parked heads
are sc-gated before parking). New stat `sets_gated`. Not implemented,
documented in the Resolution: the 0xHH00 UTF-16 validity table
(PTR_180045a00 — bytes not recoverable from the corpus) and the
root-triple recognition signal (an og empty-directory recognition
check, not an emission gate). Proof: 5 fail-before tests + 1 boundary
guard; suite 743/743. B3/B4/B10 verdict: MATCH (the empty-file policy
is the og's own — matched, not diverged). Remaining on this module:
none from this finding's registry.

## Session-24 completion verdict (M-014 UTF-16 table, DFX24, 2026-10-26)

The session-35 "not implemented" note on the 0xHH00 UTF-16 validity
table is superseded: the table IS recoverable — the corpus ships
`og-codebase/exFATSearchDLL.dll`, and the 256 bytes at .rdata RVA
0x45A00 (file offset 0x44600, PE section map) were extracted
byte-exact, with the byte order pinned by the raw disassembly at VA
0x180003030 (invalid iff low byte == 0 AND table[high byte] == 0; the
zero-flag set is the control/Windows-illegal positions). Wired into
`name_fragment_gates_ok` (terminator-aware loop over the fragment's 15
chars); `NameUtf16TableInvalidCharRejected` fail-before EXECUTED (RED
with the table loop disabled), `NameUtf16TableAllowedCharAccepted`
pins the U+1A00-passes/U+0100-fails boundary. The B10 (names) cell
keeps its MATCH verdict — now without the residual-FP asterisk.

---

## Session-24/35 verdict update (M-052 FIXED — DFX24 + the s35 convergent landing, 2026-10-26)

**Recycle-bin pairing mirrored — `M-052` FIXED.** "$RECYCLE.BIN
handling mirrored from FAT engine" (fat-exfat-engine-internals.md :71)
is now true in the core too: the exfat engine carries the $I/$R
pairing through the shared `cr/recycle` helper (the NTFS engine's
reference model extracted to its engine-neutral form), with the bin
context threaded through walk_entries/recurse_dir (and through the
M-013 oracle's parked-set re-walks — ParkedSet.in_bin). The og's
four-root recognition set (exFATSearchDLL :40583-40592, verified
first-hand: Recycler / Recycled / $Recycle.Bin / $RECYCLE.BI0) marks
the subtree; in-bin `$I<token>` streams are parsed (v1 28+2*pathlen,
v2 544 fixed, the Win10+ variable form's repurposed @0x18 honored)
and never surface; in-bin `$R<token>` files are remapped to their
original name/size/deletion-time with the Deleted state, content
extents untouched, with the post-walk fixup for late-ordered pairs.
Stats `recycle_info` / `recycle_remapped`. Proof:
ExfatEngine.RecyclePairingMirrored (fail-before EXECUTED — engine
mutation 3/4 RED; the over-matching pin green by design). B4/B5/D11
verdict: MATCH.



## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

New active findings on this module: **S-066** (SEVERE — the lost-anchor base−12
re-base ladder absent: the og probes FAT media words at base and base−12 after a
failed backup-boot gate and corrects the volume base; the core scans 12 sectors
off), **N-198** (the three core-only numeric bounds — register the hardening).
Verified-MATCH this session: the og gate set (FUN_180021e80's full `!= 0` family),
the dual ROL15 entry checksum, the boot-region checksum parity (informational on
both sides — confirmed neither validates it at open outside RawFixer), the
cluster→heap u64 math, and the exFAT engine's non-use of FSI 'RRaA' (both sides).

## Session-77 addendum (the S79 continuation audit lane; core `3799dee2`, re-verified at `0da3490b`, corpus `be0cfe9`)

- **S-067 (SEVERE — FIXED, the S80 parity fix lane):** the chain walker's out-of-range
  FAT-entry clamp — the og maps any entry ≥ cluster_count to EOC (the walk loop's clamp at
  exFATSearchDLL :41315/:41348-41351, re-verified first-hand at the fix; the volume-context
  +0x18 bound is the boot +0x5C field verbatim, RAWSearchDLL:36750). LANDED:
  `ExfatFat::next()` clamps every entry ≥ cluster_count to EOC (the count threaded from
  `p.cluster_count` at scan setup) + the loop's `c < 0xFFFFFFF6` belt-and-braces
  (`follow_exfat_chain`). Tests: OutOfRangeFatEntryEndsChainCleanly /
  ExactClusterCountEntryClampsToEoc / ReservedFFF6EntryEndsChainCleanly — RED 3/3 via the
  stash dance, GREEN 3/3 post-fix; the EXFAT-2/3 families re-pinned; the exFAT suite
  44/44. The damaged-FAT semantics family is now three-for-three
  (EXFAT-2/EXFAT-3/S-067). Resolution: `findings/FIXED/S-067-...md`. Evidence:
  `evidence/s79-lane-sheets/s79-lead-verification.md` §A.
