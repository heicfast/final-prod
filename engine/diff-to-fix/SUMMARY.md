# SUMMARY.md — executive summary (session 3 reconciled; sessions 1+2 below)

**Audited commit:** session 1 `67193b6` → session 2 `5d6da5f`/`8574039`/`5f638a3` →
**session 3: `2ce2e6b` + in-flight fixes through `34b3441`+rebase** (S-010 `3e34ed2`,
M-009 `7fd00e0` verified landed mid-session). **Corpus:** `03ad46c` (unmoved).
**Audit dates:** 2026-10-25 (sessions 1, 2 and 3 — three legs in session 2, one in 3).
**Status:** session 2 waves 1–3 complete (parallel leg: 17 new findings + FIXED/ cleanup;
this leg: 8 more landed fixes independently re-verified first-hand and CLOSED, + M-010
filed). The decompile-vs-core deep pass continues below the session-1 module parameters.

## Session-3 delta (2026-10-25 — the "go deeper, remove the fixed, mark the remainder" session)

Mandate: find what sessions 1-2 missed, verify/retire the fixed things, go deeper on
the og-codebase vs core comparison. State at session-3 close:

- **Retired (verified first-hand):** S-010 FIXED by the fix leg (`3e34ed2`+`414f56c`+
  `34b3441`, already in FIXED/; this session reconciled the stale SUMMARY that still
  listed it as top risk #1); M-009 FIXED (`7fd00e0`, `RwDevice::read_at` now routes
  through `aligned_read_at` — verified in-tree, Resolution added).
- **Fixed MID-SESSION (verified first-hand):** A-002 — `7f2c83e` restored the real
  Microsoft/og LDM GUIDs (`…0434cfb3`/`…714a69ad`; this leg re-extracted them from four
  og binaries independently, converging with the third leg's evidence and the 6-source
  external table) and de-self-referenced the tests. The #1 roadmap item is CLOSED.
  **S-011 was FIXED by the fix session after session-3 close** (tz subtraction
  removed; on-disk time carried through the shared FAT-family datetime core
  exactly like FAT; 3 guards, fail-before proven; suite 325/325 Linux ASan).
- **New findings (7):** S-015 (SEVERE — raw_signs.json min/max are KiB in the og
  (`max << 10`, default 0x7d000 KiB) vs bytes in the core: the 139-format raw-sign
  recall window is 1024x too small, 50 KiB vs 50 MiB); M-022 (ReFS file-value grammar:
  og type@key+0 dual-layout {0x178 old / >=0x29 modern, gated on system-dir-count >= 8}
  + $I recycle handler + all-rows iteration vs core 0x80@key+8 first-match-break);
  M-023 (filter model: 8-cat/1,481-ext runtime XML vs 7-cat/64-ext hard-coded;
  Email+Bookmarks missing — filed from the old unnumbered GAP-cats row); N-030
  (raw_signs confidence field is dead data in the og — parser consumes offset+hex only;
  the core invents confidence semantics); N-031 (carve dispatch deltas: og
  u32→u16→linear→sign-DB order decoded, sector-relative stride, all-zero-sector skip,
  only_common config gate); N-032 (health transports: og companion's CSMI/USB-bridge/
  Realtek-NVMe accessors missing in core); I-012 (GAP-ADS RESOLVED: the og does not
  recover ReFS ADS either — .rsf/RTTI/emission evidence; core MATCH, shared limitation,
  mandate clause satisfied by parity).
- **OQ-T8 RESOLVED** (the decode above; the parallel s2-third-leg's convergent evidence
  block had the u16/sweep legs swapped — corrected with disassembly proof, addendum
  appended to their evidence file). OQ-T9 was resolved by the s2-third-leg (M-017,
  187-row-verified ln(count)/ln(max) scoring model — convergent read this session).
- **Cross-leg coordination:** IDs renumbered past the s2-third-leg's S-012..S-014 /
  N-023..N-029 (this leg's raw-signs findings became S-015/N-030/N-031); corpus errata
  19-21 filed (dispatch order inversion, dead confidence field, KiB units).
- **Architectural verification:** CPartitionVirtualVolumeDevice (the og's virtual-volume
  device layer) = the core's IStorageDevice + slice adapters (equivalent; GAP-S6-8's
  absolute_base covers the base semantics).
- **Coverage added:** ReFSSearchDLL file pipeline (first deep pass in any session),
  RAWSearchDLL raw_signs loader/parser/scan-loop/dispatch (first), FileFilter.xml
  (first), diskhealth companion transports (first), exFAT/FATCalcPartition verified
  convergent via M-017's decode.

## Session 2 + parallel-leg combined delta

Session 2's mandate (find what session 1 missed, continue the audit, remove the fixed
things) landed in two parallel legs, reconciled here:
- **Parallel leg (db7324a/1882847/5f638a3):** 17 new findings (S-010, S-011, M-007,
  M-008, M-009, N-011..N-020, N-021, I-006, I-007), FIXED/ split, RESOLVED-INDEX,
  CORPUS-ERRATA (18 candidates), module-file updates, new open questions
  (OQ-S2A-1..4, OQ-S2C-3).
- **This leg (rebased on top):** independently re-verified every landed fix first-hand
  against the corpus decompiles (S-002 `c9e8860`, S-004 `51ac671`, S-008 `e0175b3`,
  N-002/N-003 `fb44073`, N-010 `cb8dafd`, M-006 `e30b84d`, I-001..I-005 `f74a5dd`),
  re-ran the full suite at `8574039` (313/313 Linux ASan), CLOSED all of them, moved
  S-002 to FIXED/ (the parallel leg's summary listed it open — stale), and filed
  **M-010** (S-005's residual invalid-UTF-8 edge: ext4/Btrfs name bytes destroyed vs og
  raw passthrough; ZFS edge verified parity first-hand via FUN_18001c630).
## ⚠️ THIRD-LEG ADDENDUM (LDM deep-pass leg, session 2) — ~~ACUTE A-002 filed~~ **A-002 FIXED (fix session)**

**A-002 RESOLVED (fix session, this file's top-priority item):** the two corrupted LDM
GPT type-GUID constants were restored byte-for-byte to the real Microsoft
`PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` values (= the pre-`1cb6a43`
bytes, independently re-verified this fix session against the og binaries' own compare
operands: LdmManager.mo DAT_0x1800c4960, Partition.mo DAT_0x180036630/0x36640; full
1,467-file corpus scan: fabricated values in 0 files). The four regression tests were
de-self-referenced (fixtures now hard-code the real bytes / winioctl text forms instead
of copying the constants) and three new guards added
(`Ldm.GptDynamicDiskRealCanonicalGuidsParse`, `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText`,
`Ldm.GptDynamicDiskFabricatedGuidsRejected`). Fail-before proven: 6/7 guard tests fail
on the fabricated constants; suite 320/320 Linux ASan post-fix. See
`findings/FIXED/A-002-*.md` Resolution. A-002 is CLOSED; the remaining active
ACUTE is session-3's A-004 (event-range config-sink mis-delivery — see the
session-3 delta below), which the fix session takes up next.

**Session-2 third-leg state at close (2026-10-25):** this leg resolved OQ-T2 (FAT mirror
consult — per-entry zero-fallback, N-022 filed), OQ-T5 (exFAT ChecksumDBR — spec skip-set
{106,107,112}, core MATCH), OQ-T7 (RAID-5 rotation — left-symmetric both sides, I-008),
and bounded OQ-T6 (the og's ZFS engine is vdev-type-blind: zero vdev-type/geometry keys
in the binary; fixed 0x1C000 anchor quote-confirmed; id-only vdev registry — the core's
explicit unsupported_vdevs refusal is the honest superset). OQ-T9's CalcPartition
machinery is located (vtable slot 4, Fat32 direct-field fast path) for a future scoring
decode. **A-002 remains the roadmap's #1 item** (two-line constant restore + de-self-
referencing the two regression tests).


**A-002 (ACUTE, fix-induced regression, highest priority in the whole roadmap):** the
A-001 fix `1cb6a43` replaced the two LDM GPT type-GUIDs — which were **byte-for-byte
the real Microsoft `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID`** — with
fabricated values (`5808c8aa-8e9f-4c87-…`/`af9b60a0-384e-4f7d-…`) that appear in no og
binary, no authoritative source (Microsoft Learn `PARTITION_INFORMATION_GPT`, ntdoc,
MagnumDB, phnt, Linux kernel efi.h, Wikipedia all carry the real values), and no real
disk. **On the current tree every real Windows GPT dynamic disk fails LDM discovery
(ERR_BAD_MAGIC) — zero dynamic-volume recovery.** Session-1's A-001 LDM rows were a
false positive (mixed-endian mis-decode; its executed "proof" fixture carried the
fabricated GUIDs, which the pre-fix core correctly rejected); the fix then made the
wrong fixture pass, and the new regression tests are self-referential (they write the
constants into the fixtures). Executed proof: the suite's own
`GptDynamicDiskCanonicalGuidsParse` body with ONLY the entry GUIDs swapped to the real
Microsoft bytes **fails** on the current tree and **passes** with the pre-`1cb6a43`
constants — `findings/ACUTE/A-002-*.md` + `evidence/executions/s2-ldm-real-guid-proof.md`.
**Fix = restore the two pre-`1cb6a43` constant lines + hard-code the real bytes in the
two regression tests (de-self-reference them).**

Also from the same LDM deep pass: **OQ-T7 RESOLVED** (RAID-5 rotation = left-symmetric
on both sides, algebraically identical formulas; the og's other three rotation modes
are dormant dead code — finding **I-008**; reconstruct-on-failure semantics identical);
the GPT-LDM gating question resolved (og gates on the real metadata GUID, PRIVHEAD
candidate at the partition's last LBA, no CRC checks — all matching the core);
**A-001 REOPENED** for its false-positive LDM half (its ESP/MSR/BIOS-boot/Linux half
stands fixed). The invalid-UTF-8 name-fidelity finding this leg drafted (N-010) is
**superseded by M-010** (same substance, filed by the close-out leg — dropped here,
ID retired); this leg's evidence quote `evidence/decompiled-quotes/s2-zfs-utf8-conversion.md`
supports M-010. This leg's dormant-rotations finding is renumbered **I-008** (ID
collision with the parallel leg's I-006 ext4-nanoseconds, cleared).

## Counts per severity (active — excludes FIXED/) — session-3 reconciled

| Severity | Active | FIXED (in `findings/FIXED/`) | IDs (active) |
|---|---|---|---|
| ACUTE | **0** | A-001 (partial — ESP/MSR half stands fixed; LDM half false-positive), A-003, **A-002 (`7f2c83e` — real Microsoft/og GUIDs restored, session-3-verified; tests de-self-referenced)**, **A-004 + S-016 (`c33236f` — live-sink ErrorRange delivery + live snapshot error map)** | — |
| SEVERE | 7 | S-001, S-002, S-004, S-005, S-008, **S-010 (3e34ed2)**, **S-011 (this fix session)**, **S-006 (this fix session — ext probes = og-parity accept gates; HFS+/Btrfs/ZFS/ReFS = documented supersets, D-50; FATX scoped remainder)** | S-003, S-007, S-009 (split pending N-020), S-012, S-013, S-014, **S-015 (S3: raw_signs KiB units)** |
| MAJOR | 18 | M-001, **M-009 (7fd00e0 — 64 KiB envelope on the repair read path, session-3-verified)** | M-002, M-005, M-007, M-008, M-010, M-011..M-021, **M-022 (S3: ReFS file-value grammar)**, **M-023 (S3: filter model)** |
| MINOR | 26 | N-002, N-003, N-010-theirs, **N-021 (this fix session)** | N-001, N-004..N-009 (grouped), N-011..N-022, N-023..N-029 (S2-third-leg, grouped), **N-030..N-032 (S3)** |
| INFORMATIONAL | 10 | — | I-001..I-011, **I-012 (S3: GAP-ADS resolved — parity, shared limitation)** |
| tracked-open | 6 files | OQ-T1/T2/T5/T7/T8/T9 + all 9 S2-third-leg OQs RESOLVED across legs | OQ-T3*, OQ-T4*, OQ-T6 (bounded), OQ-S2A-1..4, OQ-S2C-3, OQ-S2M-1/2 (see files) |
| **Total active findings** | **61** | **23** | |

*(Counts by file inventory, fix-session state after A-004/S-016 (c33236f), S-011, S-006 and
N-021 (this session): 0+7+18+26+10 = 61 active, 23 FIXED.)*

**Note on S-009/M-006:** the S-009 disk-map component was FIXED via GAP-S6-11
(`56cce87` — `cr_diskmap.cpp/.h` verified in-tree by this leg); the M-006 fuzz-CI
component was FIXED (`e30b84d`). The combined finding file stays in `SEVERE/` until the
split tracked in N-020 lands.

**Note on ID collisions (both resolved):** (1) parallel-leg N-010 (FAT BAD sentinel,
FIXED) vs session-2 N-021 (FAT invalid-date) — renumbered to N-021. (2) this leg's
ext4/Btrfs invalid-UTF-8 finding (briefly M-007 in a local commit) renumbered to
**M-010** to avoid colliding with the parallel leg's M-007 (NTFS hard-link fan-out).

## Counts per module (active findings)

| Module | ACUTE | SEVERE | MAJOR | MINOR | INFO | OPEN | Complete |
|---|---|---|---|---|---|---|---|
| ntfs | – | – | 2 (M-007, M-008) | 3 (N-001, N-011, N-012) | 1 (I-001 ack) | OQ-S2A-1..4 | ✔ |
| fat | – | – | – | 3 (N-004, N-021, N-022) | 1 (I-002 ack) | OQ-T2 **RESOLVED** | ✔ |
| exfat | – | 1 (S-011) | – | – | – | OQ-T5 | ✔ |
| ext4 | – | 1 (S-003 OPEN) | 1 (M-010) | 1 (N-004) | 2 (I-006, I-007) | – | ✔ |
| hfsplus | – | 1 (S-003) | – | 1 (N-005) | 1 (I-007) | – | ✔ |
| btrfs | – | – | 1 (M-010) | 1 (N-006) | 2 (I-003 ack, I-007) | – | ✔ |
| zfs | – | – | – | – | 1 (I-007) | OQ-T6 | ✔ |
| refs | – | – | (project GAP: ADS) | 1 (N-007) | 1 (I-004 ack) | – | ✔ |
| ldm/table | – | – | 1 (M-002) | – | 1 (I-008) | OQ-T7 **RESOLVED (this leg)** | A-002 **FIXED** (fix session) ✔ |
| lost-partition | – | 1 (S-006) | – | – | – | OQ-T9 | ✔ |
| carve | – | 1 (S-007) | 1 (M-005) | 1 (N-008) | – | OQ-T8 | ✔ |
| session | – | – | – | 1 (N-014) | (I-005.7 ack) | – | ✔ |
| writer+report | – | – | – | 3 (N-009b, N-009d, N-013, N-015) | – | – | ✔ |
| health | – | – | – | 1 (N-009a) | – | – | ✔ |
| rawfixer | – | 1 (S-010) | 1 (M-009) | – | – | OQ-S2C-3 | ✔ |
| orchestration | – | – | – | 2 (N-016, N-017) | – | – | ✔ |
| visualization | – | 1 (S-009 partial) | (project GAP: filter cats) | – | – | – | ✔ |
| abi (cr_ui) | – | (S-009 partial) | – | – | – | – | ✔ |
| cli | – | – | – | 1 (N-009c) | – | – | ✔ |
| quality (cross-cutting) | – | – | (M-006 partial) | 3 (N-018, N-019, N-020) | – | – | ✔ |

*(M-010 counts once per affected engine row for navigation, as S-005 did.)*

## Top risks (ranked — fix-session state)

1. **A-004 (ACUTE — event-range fires through a config sink; the only active ACUTE
   post-`7f2c83e`)** — crash-class latent defect on the public API surface; one-line
   fix + two tests per its plan. **← the fix session's next item.** Then **S-015
   (session 3) — raw-sign recall window 1024x too
   small** (raw_signs.json min/max read as bytes vs the og's KiB): every raw-sign
   format caps at 50 KiB (200 KiB for the media entries) where the og carves 50 MiB
   (200 MiB) — the largest single recall gap found in session 3; one-generator fix.
2. ~~**S-011 — exFAT `tz_offset` applied (core) vs ignored (og).**~~ **FIXED**
   this fix session (tz subtraction removed; on-disk time through the shared
   FAT-family datetime core, host-independent on-disk-as-UTC; guards
   TimestampsMatchOgLocalAsUtc / TimestampsZeroTzPassThrough /
   TimestampsMatchExplorerDisplay).
3. **S-003 — HFS+ journal replay missing.** Crashed volumes lose journal-carried recent
   state. Journal replay mechanics decoded first-hand (`b85c084` evidence addendum);
   fix deferred with precise scope.
4. ~~**S-006/S-007 — discovery coverage.**~~ **S-006 FIXED** this fix session
   (ext = og parity; HFS+/Btrfs/ZFS/ReFS superset anchors; FATX scoped
   remainder); **S-007 remains open** — carve misses GIF/BMP/headerless-MP3/
   AAC/RTF/OLE-Office/MPEG/DWG.
5. **M-007/M-008 (session 2, superset) — NTFS hard-link fan-out + `$ATTRIBUTE_LIST`
   multi-hop chase.** Both SUPERSET divergences where the og's behavior is a limitation.
   Opt-in parity flags recommended.
6. **M-010 (this leg) — ext4/Btrfs invalid-UTF-8 names destroyed** (U+FFFD baked; og
   preserves raw bytes; ZFS edge is verified parity). Permanent loss of original name
   bytes for legacy-locale filenames.
7. **M-009 (session 2, perf) — RawFixer bypasses the 64 KiB direct-I/O envelope.**
   ~3.5× slow path on 4Kn NVMe. **FIXED** (`7fd00e0`, verified first-hand `14f3a29`;
   in `findings/FIXED/`).
8. ~~**OQ-T7 — LDM RAID-5 rotation unverified.**~~ **RESOLVED** (left-symmetric both
   sides; og's other rotations dormant — I-008).

## Recommended fix order (first 20 — post-reconciliation)

| # | Finding | Rationale |
|---|---|---|
| 1 | S-010 | safety regression on the repair path; can corrupt volumes during concurrent access |
| ~~2~~ | ~~S-011~~ | **FIXED this fix session** (tz subtraction dropped; 3 guards; suite 325/325) |
| 3 | S-003 | HFS+ journal replay (mechanics decoded; fix scoped) |
| 4 | S-006 | lost-partition probe families (engine wiring only) |
| 5 | S-007 | carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG |
| 6 | M-009 | RawFixer bypasses 64 KiB envelope; one-line fix (route through aligned_read_at) |
| 7 | M-007 | hard-link fan-out superset; opt-in parity flag (mirrors S-001's pattern) |
| 8 | M-008 | `$ATTRIBUTE_LIST` chase scope+depth superset; opt-in parity flag |
| 9 | M-010 | lossless invalid-byte mapping (U+E000+byte) for ext4/Btrfs; ZFS stays U+FFFD |
| 10 | M-002 | EBR torn-chain salvage |
| 11 | M-005 | carve bitmap gating (perf + FP surface) |
| 12 | N-021 | FAT invalid-date yields non-zero FileTime vs og zero; one-line guard |
| 13 | N-013 | dest-overlap optional + first-extent-only; composes with S-010's lock fix |
| 14 | N-014 | stage checkpoint not atomic; atomic-rename fix |
| 15 | N-015 | writer no disk-full pre-check; composes with S-010's lock fix |
| 16 | N-016 | emit_boundary_events double-call bug; one-line removal |
| 17 | N-017 | cancel watcher coverage inconsistent; one-line parallel_for poll |
| 18 | N-018 | UBSan CI lane; mirror the TSan lane |
| 19 | N-019 | 12 missing per-parameter regression tests |
| 20 | N-020 | 6 stale doc claims; split S-009/M-006 finding file; move both to FIXED/ |

## Audit coverage statistics

- **Core read (sessions 1+2):** 54 src .cpp (26,867 LOC), 56 headers, 38 test files
  (313 TEST cases / ~17,730 LOC at `8574039` — re-run green this leg) — agents C+D
  exhaustive; session-2 agents added line-by-line reads of the NTFS mftwalk, all 8
  engine .cpp, query/writer/session/transport/rawfixer/orchestrator.
- **Corpus read (sessions 1+2):** 9 architecture docs, 274 artifact docs, raw decompiled
  trees. Session 2 added first-hand reads of `ghidra-partition/*.c` (RecoveryModule,
  PartitionRecovery, RawFixer) that session 1 did NOT use — these contained the canonical
  FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME sequence (S-010) and the ACL-preservation
  `SetFileSecurityW` calls (N-009d refinement). This leg re-verified first-hand:
  HFSPlusSearchDLL FUN_180009c10 (S-002), EXT4SearchDLL jsb loop (S-004),
  RawFixer FUN_18005de40/FUN_18005df00 (S-008), FATSearchDLL mask-24 + FUN_180041c80
  (N-002/N-010), ZfsSearchDLL FUN_18001c630 (M-010), EXT4/Btrfs import censuses (M-010).
- **Executed verifications:** core suite 313/313 Linux ASan at `8574039` (this leg);
  fail-before/pass-after proofs cited in each fix's Resolution.
- **Not executable here:** og-execution (no Windows/wine) and new Ghidra decompiles —
  all such questions are in `tracked-open/` with precise probes.

## Honesty notes

- Session 1's NTFS B7 "MATCH" verdict for hard-link/ADS/sparse was **REFUTED** by the
  parallel leg's line-by-line decompile read. Three new findings (M-007, M-008, N-011)
  and one minor (N-012) surfaced. The session-1 verdict was based on a misread of the
  LOCK/unlock block as hard-link refcount management.
- Session 1's Q67 "M" (sanitizer cleanliness) was incomplete — UBSan is not enabled
  anywhere (N-018). Session 1's Q65 "M" (test coverage) was aspirational — 12 of 13
  audit-prompt scenarios lack isolated tests (N-019). Session 1's Q68 "M" (documentation
  accuracy) had 6 stale claims (N-020).
- The parallel leg's SUMMARY listed S-002 as open — **stale**: the fix (`c9e8860`) is
  in the `8574039` baseline and was independently re-verified first-hand this leg
  (FUN_180009c10 tree-8/keyLength/recordType/value-offset reads); S-002 now lives in
  `findings/FIXED/`.
- The fixed findings are preserved in `findings/FIXED/` for the audit trail and are NOT
  counted in the active totals. `findings/FIXED/RESOLVED-INDEX.md` catalogues each
  resolved finding's title, fix commit, and resolution summary.
- The 7 ACKNOWLEDGED informational findings (I-001..I-007) are deliberate supersets,
  pinned with regression tests; they stay in `findings/INFORMATIONAL/` because they are
  documented improvements, not bugs.
- The 18 corpus errata candidates are cataloged in `diff-to-fix/CORPUS-ERRATA.md`.

## Remediation log (combined: all legs)

| Date | Finding | Verdict | Evidence |
|---|---|---|---|
| 2026-10-25 | A-001 | **FIXED** | canonical GUIDs (`1cb6a43`); in `findings/FIXED/` |
| 2026-10-25 (S3) | A-002 | **FIXED** | `7f2c83e` real Microsoft/og GUIDs restored; tests de-self-referenced + negative guards; session-3 independent og-binary re-extraction converges (4 binaries / 7 sites) |
| 2026-10-25 (S3) | S-010 | **FIXED** | `3e34ed2`+`414f56c`+`34b3441` volume quiesce on the repair write path; 3 fail-before lock tests; SUMMARY stale-row reconciled this session |
| 2026-10-25 (S3) | M-009 | **FIXED** | `7fd00e0` RwDevice::read_at routes through the 64 KiB envelope; verified first-hand this session |
| 2026-10-25 | M-001 | **FIXED** | record-size ladder (`5d6da5f`); in `findings/FIXED/` |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run sub-fix (`e93decf`); in `findings/FIXED/` |
| 2026-10-25 | S-002 | **FIXED** | attributes B-tree + decmpfs modes 3/4 (`c9e8860`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-004 | **FIXED** | ext4 journal every-block salvage + jsb gate (`51ac671`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-005 | **FIXED** | shared UTF-8 decoder (`2f3237d`); residual edge split to M-010; in `findings/FIXED/` |
| 2026-10-25 | M-009 | **FIXED** | RwDevice::read_at routed through the GAP-S6-13 64 KiB envelope (same aligned_read_at the scan devices use); structural routing parity + envelope unit tests; 4Kn executed numbers deferred to the bench harness (no Windows/4Kn host here - same limitation the finding records) |
| 2026-10-26 | **A-002** | **FIXED** | Real Microsoft LDM GUIDs restored in `cr_table_wire.h` (PARTITION_LDM_METADATA_GUID {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3} / PARTITION_LDM_DATA_GUID {af9b60a0-1431-4f62-bc68-3311714a69ad}); regression tests de-self-referenced to hard-coded real bytes; negative guard (fabricated values → ERR_BAD_MAGIC) + bytes→text guard added; harness + probe log + parity doc corrected; suite 303/303 Linux ASan |
| 2026-10-26 | **A-004 + S-016** | **FIXED** (same delivery path, one commit) | A-004: ErrorRange delivered through the live sink (`event_fn_`), not the config copy (null-call crash / mis-delivery on the documented set_event_sink pattern); S-016 (discovered during A-004's fail-before proof): `tel_counts_` now feeds the live retry-pipe error map into the snapshot before every boundary emit — the ErrorRange event stream was dead code (snapshot error map was merged only after the emitters). Two ScanEvents tests (ErrorRangeGoesToLiveSink, SinkSwitchDeliversAllTypes); suite 305/305 Linux ASan || 2026-10-25 | S-010 | **FIXED** | volume quiesce on the repair write path (IWriteDevice lock contract, execute() lock-before-write + zero-write refusal, Windows RwDevice runs the corpus RecoveryModule LOCK/DeleteMountPoint/Flush/settle/verify/DISMOUNT sequence); finding's destination-lock bullet corrected (corpus: destination open is writability+detection only); 3 fail-before tests, suite 316/316 || 2026-10-26 | S-006 | **FIXED** | lost-partition probe families: ext (og CExtX accept gates decoded + GDT liveness, SB-derived size) + HFS+/Btrfs/ZFS/ReFS superset anchors (D-50; ReFS rescued from the FAT catch-all); 7 tests, 5 fail-before proven; suite 332/332 Linux ASan. FATX = scoped remainder (accept criteria not corpus-decoded); M-017 scoring = own finding |
| 2026-10-26 | S-011 | **FIXED** | exFAT tz subtraction removed — on-disk time through the shared FAT-family datetime core (og parity, host-independent); wire tz fields parsed-but-unapplied (documented in PARITY-DELTAS + AUDIT-FALLBACKS); 3 guards (TimestampsMatchOgLocalAsUtc 0x88 + TimestampsMatchExplorerDisplay 0x20 fail-before/pass-after, TimestampsZeroTzPassThrough sentinel); suite 325/325 Linux ASan |
| 2026-10-25 | S-008 | **FIXED** | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector (`e0175b3`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-26 | **S-015** | **FIXED** | raw_signs min/max KiB units restored (single conversion point in gen_rawsigns.py; zero-max asserted; table regenerated — every format window was 1024x too small, floor 1024x too low); tail walk rebuilt (chunked forward scan, named 8 MiB bound, og full-window range documented); per-format tail scoping (cross-format pairing fabricated close positions — discovered during the fix); 6 fail-before/pass-after tests; suite 310/310 Linux ASan |
| 2026-10-26 | **S-017** | **FIXED** | ftyp-family re-keyed on the box TYPE ("ftyp", header_offset 4 — the box size is data, the pre-fix magic baked literal zeros so NO real MP4/MOV/M4A/HEIC could ever match); match primitive extracted + shared with a NEW F8 self-probe (structural rule: four-zero-byte magic prefixes rejected; candidate-shift assertion); conservative naked-moov salvage (FMT_MOV_NAKED, og MovAnalyzer mirror, tried last); 6 fail-before/pass-after tests; suite 320/320 Linux ASan || 2026-10-25 | S-009 (disk-map half) | **FIXED** | GAP-S6-11 disk-map data producer (`56cce87`); split of the combined file tracked in N-020 |
| 2026-10-26 | **S-018** | **INVALID** (premise refuted) | "zero 0xD9 occurrences in the og analyser bodies" was a grep artifact — Ghidra renders 0xD9 as the signed char -0x27; 5 FF D9 sites exist in the modern JpegAnalyzer and its accept path is EOI-gated (`uVar21 == 0 -> early return`); core validator UNCHANGED; residual (fallback sub-analysers + continuation outcome) tracked-open as OQ-S3-JPEG || 2026-10-25 | M-006 | **FIXED** | fuzz CI re-enabled (`e30b84d`); verified this leg (ci.yml :147-160) |
| 2026-10-25 | N-002 | **FIXED** | FAT mask-24 EOC family (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-003 | **FIXED** | exFAT checksum og-parity default (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-010 (FAT BAD sentinel) | **FIXED** | ordering fix (`cb8dafd`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | OQ-T1 | **RESOLVED** | secondary claim map semantics (decompile read); in `findings/FIXED/` |
| 2026-10-25 | I-001..I-005 | **ACKNOWLEDGED** | deliberate supersets verified + pin tests (`f74a5dd`); re-verified this leg |
| 2026-10-25 (S2) | S-010, S-011, M-007, M-008, M-009, N-011..N-020, N-021, I-006, I-007 | **NEW** | parallel session-2 leg, wave 2 |
| 2026-10-25 (S2, this leg) | M-010 | **NEW** | S-005 residual invalid-UTF-8 edge; ZFS-side parity verified first-hand (FUN_18001c630) |
| 2026-10-25 (fix session) | A-002 | **FIXED** | LDM GPT type-GUID constants restored to the real Microsoft values; 4 tests de-self-referenced + 3 new guards; fail-before 6/7, suite 320/320 Linux ASan; session-1 evidence chain corrected (see `findings/FIXED/A-002-*.md`) |

## Session-2 verification log (this leg, baseline `8574039`)

| Finding | Verdict | Independent evidence (first-hand this leg) |
|---|---|---|
| S-002 | **CLOSED** (`c9e8860`) | FUN_180009c10 re-read :7690-7859 (tree-8 search, keyLength `(nameLen+7)*2`, recordType 0x10 gate, value u64@+8); core walk_attributes_tree + kZlib extents in-tree; tests green 313/313 |
| S-004 | **CLOSED** (`51ac671`) | jsb loop re-read :12195-12310 (0xc03b3998 magic, blocktype {3,4}, 1 MiB windows, per-block cancel); core jsb gate + every-block parse in-tree; tests green |
| S-008 | **CLOSED** (`e0175b3`) | FUN_18005de40/FUN_18005df00 re-read :64981/:65013 ('FILE' magic + record-name); core method surface + pre-scan consent wiring in-tree; 16 tests green |
| N-002 | **CLOSED** (`fb44073`) | mask-24 family re-read :8030-8048; core cr_chain.cpp :73-80 raw-entry tolerance; test green |
| N-003 | **CLOSED** (`fb44073`) | validate_entry_sets default false (og parity); D-47 registered; both pins green |
| N-010 | **CLOSED** (`cb8dafd`) | FUN_180041c80 re-read :54124-54137 (0xfff7 → 0xffffff7 → BROKEN); core CHAIN_BAD-before-reserved-range; pin green |
| M-006 | **CLOSED** (`e30b84d`) | fuzz lane re-enabled in ci.yml :147-160 (codeql deferral documented) |
| I-001..I-005 | **ACKNOWLEDGED** (`f74a5dd`) | rationale re-verified; pins UserFileNamedDollarNotHidden / Fat16BadClusterStillCanonical / Btrfs.MirrorSelectionNewestGeneration green; D-12/D-39/D-46/D-47/D-48a-c registered |

---

# Third leg — main-auditor close-out (session 2, 2026-10-25; core `e93decf` audited, tree re-verified at `34b3441`)

The decompile-exhaustive pass session 1 never made (11 parallel read agents + main-auditor
first-hand verification of every ACUTE/SEVERE claim), the 424-file corpus coverage sweep,
and the post-closeout-code audit — re-applied onto this reconciled tree with IDs renumbered
past the two prior legs (**A-004, S-012..S-014, M-011..M-021, N-023..N-029, I-009..I-011**,
OQ-S2M-1/2; one candidate RETRACTED by this leg's own re-read).

## Verification of this tree's state (read-only facts the roadmap needs)

1. **A-002 is still UNFIXED on the current tree** — verified at `34b3441`:
   `include/cr/table/cr_table_wire.h:87-89` still carries the fabricated
   `{5808C8AA-8E9F-4C87-…}`/`{AF9B60A0-384E-4F7D-…}` LDM constants with comments claiming
   `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` identity (the real Microsoft
   values differ in fields 2-3: `…-7E8F-42E0-85D2-…`/`…-1431-4F62-…`). `23adad1` filed the
   proof and the regression fixture but the constant restore itself never landed (the
   commit touches no source file). **A-002 remains the roadmap's #1 item** — agreed with the
   third-leg close-out, now with a current-tree citation.
2. **M-006 verified FIXED** (`e30b84d`; fuzz job re-enabled, codeql deferral documented).
3. **The `fb44073` N-003 fix introduced a divergence:** its `validate_entry_sets = false`
   default rests on a "verified-absence" premise the decompile refutes (`FUN_18001eba0` =
   the dual-form checksum incl. deleted sets — quoted in the FIXED/N-003 note). Og parity
   = validating; **M-013** carries the correct fix. Recommend scheduling it with the next
   exFAT pass rather than leaving the false default.
4. **S-001's landed destroy rule is mis-scoped** (S-012): og pass 1 emits every live-MFT
   deleted record ungraded/undestroyed; the destroy+viability machinery lives in a
   GUID-dump pass 2 the core does not have. First fix step is one flag flip (restore
   membership parity); the real feature is pass 2.
5. **Empirical:** full Linux suite at `e93decf` → **282/282 PASS** (this leg's own run,
   Release build; `evidence/executions/build-suite-session2.md`). ASan build not runnable
   in this container (memory reaping) — CI carries it.

## Third-leg additions (all active; renumbered)

| Severity | New IDs | One-liners |
|---|---|---|
| ACUTE | **A-004** | ErrorRange events fire through the config sink while the guard checks the live sink (latent null-call crash on the documented `set_event_sink` pattern) |
| SEVERE | **S-012** | NTFS single-pass: og's GUID-dump pass 2 (viability/destroy/0x3ff/blanket-$) + size-0 rescue missing; destroy mis-scoped into the main walk |
| SEVERE | **S-013** | exFAT: FAT > 32 MiB silently dropped → every FAT-chained file/directory truncates to one cluster (~34 GB+ volumes) |
| SEVERE | **S-014** | BitLocker auto-unlock: wrong live registry path (Services vs Control) + offline-hive path absent + unassociated enumeration (F61 — never scored before) |
| MAJOR | **M-011..M-015** | FAT table-size windows · mirror policy (subsumes N-022's sub-case) · exFAT dual-form checksums+oracle (supersedes N-003's premise) · exFAT structural gates · FAT table validation |
| MAJOR | **M-016, M-017** | LDM TOCBLOCK candidates · lost-partition scoring formula (OQ-T9 resolved: ln(count)/ln(max), +500 table-match, count==1 quirk — 187-row-verified) |
| MAJOR | **M-018..M-021** | FilterFolder junk filter · ZFS multi-vdev fallback + fletcher4/DVA-retry · Btrfs chunk profiles + default subvol · carve video-repair hook |
| MINOR | **N-023..N-029** | FAT/exFAT gates · NTFS gates/fields · LDM geometry · ReFS pages · carve/writer · HFS+ sink/forks · U+FFFD residuals (cross-refs M-010) |
| INFO | **I-009..I-011** | carve probing supersets · scope-exclusion/supersets register · corpus errata (8 claims; feeds CORPUS-ERRATA.md) |
| tracked-open | **OQ-S2M-1/2** | NTFS grading locus (D15) · SearchDLL bonus setter / ZFS pool name / Btrfs anchor |

**Retraction (recorded):** the exFAT fixer-checksum "no-skip" candidate — this leg's own
decompile re-read found the skip-table {106,107,112} (`std::find == end()` over the
−1-terminated `DAT_180079a40`); the core's spec-skip is a MATCH, the parallel leg's binary
extraction was right. Corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`.
This leg also corrects its own first A-001 verification note (FIXED/A-001): the parallel
leg's executed proof wins — recorded in place.

**Direction corrections to prior findings (fix plans replaced in-place):** S-002 (og never
decompresses decmpfs — size-from-attributes-tree + raw-rsrc content is the parity target;
landed `c9e8860` should be audited against that boundary) · S-003 (journal =
differential-salvage evidence vs the overlay model — both readings recorded, consumption-
side quotes favor salvage) · S-004 residue (journal **inode-image salvage**, magic 0xf30a —
not covered by `51ac671`).

**Counts (all three legs combined, active — post session-3 A-002 fix):** 1 ACUTE
(A-004) · 9 SEVERE (S-003, S-006, S-007, S-009-remainder, S-011, S-012, S-013,
S-014 + S-010 per leg-3's table) · 17 MAJOR (M-002, M-005, M-007..M-021) · 24 MINOR
files · 11 INFO · 2 new tracked-open files. FIXED/: 13 findings (A-002 added — real
Microsoft LDM GUIDs restored; tests de-self-referenced; negative guard added) + all
OQ-T1..T9 resolutions. **Recommended order now: A-004 (one-line event-sink fix
restoring crash-safety), then S-012 step 1 (one flag flip restoring og deleted-file
membership), then S-011/S-013/S-014, then the leg-3 majors in M-011..M-021 order.**

Read-only compliance held throughout: only `diff-to-fix/` + `worklog.md` written; the
282/282 run was out-of-tree (Release); no source, test, build, workflow, or doc file of
either repository was modified.

---

## Session-3 PARALLEL-LEG addendum (2026-10-26 — S3 lead's wave-1, reconciled onto main)

A second session-3 leg (S3-CARVE / S3-LOST / S3-REFS parallel agents + lead) worked the
same mandate concurrently with the leg above and reconciles here. Its 17 findings were
**renumbered past the IDs that landed on main first**; five were merged as addenda into
this tree's findings (convergent decodes), one was dropped as a duplicate, one as a
superseded constant set. Final disposition:

**New standalone findings (14):**

| ID | Severity | Module | One-line |
|---|---|---|---|
| S-017 | SEVERE | carve | ftyp-family signatures unreachable — MP4/MOV/M4A/HEIC carve dead in core (box SIZE encoded as literal zeros; executed proof, 0 grammar hits for 4 planted real BMFF files) |
| S-018 | SEVERE | carve | JPEG EOI policy: neither og analyser generation requires EOI (size = SOF arithmetic); core requires EOI+SOF+SOS → truncated photos salvaged by og, dropped by core |
| S-019 | SEVERE | lost-partition | BitLocker "-FVE-FS-" lost-partition probe family missing (S-006 under-listed the og; executed string-probe evidence; distinct from S-014's win-layer unlock-path finding) |
| S-020 | SEVERE | refs | ReFS resident/inline content never materialized: FUN_18000e490/ea30 build CRefsResidentDataRunList (lead re-verified first-hand at :10413-10462); core's resident_data never populated + val_len ≥ 0x28 gate drops resident rows → inline files metadata-only. Answers M-022's fix-plan step 4; needs M-022's grammar walk first |
| M-024 | MAJOR | carve | No runtime format-filter surface vs og RawConfig.ini [Filter] (37 toggles, STL/VPJ default-off) — registry rebuilt per search |
| M-025 | MAJOR | carve | Read model: og 8 MiB ping-pong + sector dispatch + pending-CRawFileInfo streaming across chunks; core 1 MiB byte-granularity stateless + extent re-reads. M-021's hook fix depends on this state machine |
| M-026 | MAJOR | refs | ReFS deleted-recovery model: og stale-"MSB+"-page sweep (FUN_180005330) force-marks unreachable-page records deleted; core harvests row-flag 0x4 + ID2 rows — different on-disk premise (CoW medium favors the og's model) |
| M-027 | MAJOR | refs | ReFS object-identity keep-one + Recycle-Bin routing ($R*/S-*/SVI collection, $I resident read) missing; core emits hard-linked objects N times |
| N-033 | MINOR | carve | Analyser registry exports (List/Map/Multimap) consumed by the FS SearchDLLs (registry snapshot clones); no core ABI to enumerate carve formats |
| N-034 | MINOR | lost-partition | FAT lost-boot gate constants: numFATs ∈ {1,2}, FAT12 spf ≤ 200 probe, root_entries×32 % bps — complements N-010b's engine-side gates (same og constants, `src/core/lost/` consumer) |
| N-035 | MINOR | lost-partition | Lost-candidate extents are data-area-anchored in the og (EndSec = DataAreaStart + totalSector) vs core whole-volume — the M-017 containment/16-MiB rules consume different extents |
| N-036 | MINOR | refs | ReFS datarun flags: og requires 0x40 + container-map translation for non-physical runs; core requires 0x10, map never populated |
| N-037 | MINOR | refs | ReFS runlist granularity validation (og discards cluster-inconsistent runlists as FP guard) missing in core |
| I-013 | INFO | refs | ReFS host-OS runtime gate (og refuses < Win8 hosts) — core host-agnostic, deliberate portability superset |

**Merged as addenda (5) + drops (2):** dispatch chain semantics + id-8 continuation
records → **N-031 addendum** (convergent with its decode; adds u16 insertion-order
chains, TTF-vs-MDB u32 pre-emption, TIFF-family same-class relabeling); EUVideoRepair
per-chunk feed + stream-geometry submission → **M-021 addendum**; RTTI proof that
*CalcPartition classes are geometry calculators (slot 4 = sectors-per-cluster, executed
vtable walk) + full AdjustPartitionInfoOrder decode (containment drop, FS-grouped
dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) →
**M-017 addendum** (OQ-T9's premise corrected — the classes are NOT scorers; the
DRWUI decode above is the real model, independently confirmed to identical constants);
resident-materialization answer → **M-022 addendum**; byte-granularity pin test →
**I-009 addendum**; lost-probe superset row → **I-010 addendum**. Dropped: this leg's
N-029 (ReFS boot-geometry gate — duplicate of N-026's N-013b, which is more precise)
and its stand-alone I-009/I-010/M-013/M-014/N-023/N-024 (superseded by the merges
above).

**S-007 CORRECTED (decompile re-verified first-hand by this leg's lead at
RAWSearchDLL_decompiled.c:1985 / :2156-2177 / :3097):** the session-3 addendum inside
S-007 carries two wrong rows — `C5 D0 D3 C6` is **CEPSAnalyser (AI/EPS)**, not dBase
(no DBF analyser exists anywhere in the decoded registry), and the LZH row's bytes are
wrong on both counts: `E4 52 5C 7B` is **OneNote** (OneAnalyser, format id 0x31);
LZH is a list-layer structural analyser (LZHAnalyser). The fix plan's DBF validator is
dropped; OneNote magic + list-layer LZH take its place. Errata rows 22-28 filed.

**A-002 status chain (for the audit trail):** this leg independently re-verified the
og GUIDs (re-executed the LdmManager.mo/Partition.mo probe + external cross-check)
while the tree still carried the wrong constants — then found the fix leg had landed
`7f2c83e` mid-session on main; the fix is verified (constants = `5808c8aa-7e8f-42e0-
85d2-e1e90434cfb3` / `af9b60a0-1431-4f62-bc68-3311714a69ad`, tests de-self-referenced
with text-compare + negative guard at test_crc32_gpt.cpp:394-395/:432-433,
test_ldm.cpp:616-617). **A-002 stays CLOSED.**

**OQ-T9 tracking reconciled:** the tracked-open file's stale UNVERIFIED header is
replaced with the full resolution (both legs' decodes merged; residues → S-019/N-034/
N-035 + OQ-S2M-2's bonus setter).

**Counts after this addendum (by direct file inventory):** ACUTE **0** (A-004 now
FIXED — moved by `c33236f`, which also fixed S-016) · SEVERE 9+4 = **13** (S-003,
S-006, S-007, S-009-partial, S-011, S-012, S-013, S-014, S-015 + S-017..S-020) ·
MAJOR 18+4 = **22** · MINOR 27+5 = **32** · INFO 10+1 = **11** · **total active 78**
(20 in FIXED/). Top-risk insertion: **S-017/S-018 join the carve priorities** (media
formats the og recovers and the core cannot — S-017's ftyp bug makes the four most
common video/photo containers uncarvable TODAY), **S-020 joins the ReFS first-class
blockers** (with M-022/M-026/M-027), **S-019 joins S-006's probe wiring** (implement
together).
