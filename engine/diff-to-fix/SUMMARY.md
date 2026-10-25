# SUMMARY.md — executive summary (session 2 — both legs reconciled, second close-out pass)

**Audited commit:** `5d6da5f` (session 1 baseline `67193b6` plus concurrent fixes) →
`8574039` (first close-out baseline) → `5f638a3` (current; includes the Windows CRT
exclusive-open CI repair on top of the parallel audit leg). **Corpus:** `03ad46c`
(unmoved). **Audit dates:** 2026-10-25 (session 1) + 2026-10-25 (session 2, two legs).
**Status:** session 2 waves 1–3 complete (parallel leg: 17 new findings + FIXED/ cleanup;
this leg: 8 more landed fixes independently re-verified first-hand and CLOSED, + M-010
filed). The decompile-vs-core deep pass continues below the session-1 module parameters.

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
## ⚠️ THIRD-LEG ADDENDUM (LDM deep-pass leg, session 2) — ACUTE A-002 filed

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

## Counts per severity (active — excludes FIXED/)

| Severity | Active | FIXED (in `findings/FIXED/`) | IDs (active) |
|---|---|---|---|
| ACUTE | 1 | A-001, **A-003** (discovered by the continuation leg; `repair::rebase_plan` parent-basis fix — RENUMBERED from its local A-002 to clear the collision with this leg's A-002 GUID-regression finding, which landed on main first) | **A-002 (third-leg addendum — fix-induced regression)** |
| SEVERE | 6 | S-001, S-002, S-004, S-005, S-008 | S-003, S-006, S-007, S-009 (partial), S-010, S-011 |
| MAJOR | 6 | M-001 | M-002, M-005, M-007, M-008, M-009, M-010 |
| MINOR | 17 | N-002, N-003, N-010 | N-001, N-004..N-009, N-011..N-021, **N-022 (third leg: FAT mirror consult on BAD)** |
| INFORMATIONAL | 8 (7 ACKNOWLEDGED + 1 informational-record) | — | I-001..I-007, I-008 (dormant RAID-5 rotations — a MATCH record) |
| tracked-open | 12 | OQ-T1, OQ-T2 (RESOLVED), OQ-T5 (RESOLVED), OQ-T7 (RESOLVED) | OQ-T3*, OQ-T4*, OQ-T6 (bounded), OQ-T8, OQ-T9 (machinery located), OQ-S2A-1..4, OQ-S2C-3 |
| **Total active findings** | **45** | **12** | |

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
| ldm/table | 1 (**A-002**) | – | 1 (M-002) | – | 1 (I-008) | OQ-T7 **RESOLVED (this leg)** | ✔ |
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

## Top risks (ranked — post-reconciliation)

1. **S-010 (session 2, safety) — RawFixer write device bypasses FSCTL_LOCK_VOLUME +
   FSCTL_DISMOUNT_VOLUME.** The og's `RecoveryModule.dll` performs the full
   lock+unmount+flush+sleep-2s-settle+verify sequence at
   `RecoveryModule_decompiled.c:44686-44720`. The core's `RwDevice` opens with
   `FILE_SHARE_READ|WRITE` and no lock. Concurrent processes can race with repair
   writes — silent volume corruption. Compounds the (now-fixed) S-008 wiring with a
   safety gap on the write path.
2. **S-011 (session 2, parity) — exFAT `tz_offset` applied (core) vs ignored (og).**
   Every non-UTC exFAT file's `.rsf` diverges by `tz_offset × 900 × 10⁷` (100ns units).
   Spec-correct UTC vs og's local-as-UTC. One-line fix.
3. **S-003 — HFS+ journal replay missing.** Crashed volumes lose journal-carried recent
   state. Journal replay mechanics decoded first-hand (`b85c084` evidence addendum);
   fix deferred with precise scope.
4. **S-006/S-007 — discovery coverage.** No lost-partition probes for Linux/Mac/ReFS
   filesystems; carve misses GIF/BMP/headerless-MP3/AAC/RTF/OLE-Office/MPEG/DWG.
5. **M-007/M-008 (session 2, superset) — NTFS hard-link fan-out + `$ATTRIBUTE_LIST`
   multi-hop chase.** Both SUPERSET divergences where the og's behavior is a limitation.
   Opt-in parity flags recommended.
6. **M-010 (this leg) — ext4/Btrfs invalid-UTF-8 names destroyed** (U+FFFD baked; og
   preserves raw bytes; ZFS edge is verified parity). Permanent loss of original name
   bytes for legacy-locale filenames.
7. **M-009 (session 2, perf) — RawFixer bypasses the 64 KiB direct-I/O envelope.**
   ~3.5× slow path on 4Kn NVMe. One-line fix.
8. **OQ-T7 — LDM RAID-5 rotation unverified.** Safety-critical; if the og's rotation
   differs from the assumed left-symmetric, every reconstructed RAID-5 member carries
   wrong bytes.

## Recommended fix order (first 20 — post-reconciliation)

| # | Finding | Rationale |
|---|---|---|
| 1 | S-010 | safety regression on the repair path; can corrupt volumes during concurrent access |
| 2 | S-011 | exFAT tz_offset parity break; one-line fix (drop the tz subtraction) |
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
| 2026-10-25 | M-001 | **FIXED** | record-size ladder (`5d6da5f`); in `findings/FIXED/` |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run sub-fix (`e93decf`); in `findings/FIXED/` |
| 2026-10-25 | S-002 | **FIXED** | attributes B-tree + decmpfs modes 3/4 (`c9e8860`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-004 | **FIXED** | ext4 journal every-block salvage + jsb gate (`51ac671`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-005 | **FIXED** | shared UTF-8 decoder (`2f3237d`); residual edge split to M-010; in `findings/FIXED/` |
| 2026-10-25 | M-009 | **FIXED** | RwDevice::read_at routed through the GAP-S6-13 64 KiB envelope (same aligned_read_at the scan devices use); structural routing parity + envelope unit tests; 4Kn executed numbers deferred to the bench harness (no Windows/4Kn host here - same limitation the finding records) |
| 2026-10-26 | **A-002** | **FIXED** | Real Microsoft LDM GUIDs restored in `cr_table_wire.h` (PARTITION_LDM_METADATA_GUID {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3} / PARTITION_LDM_DATA_GUID {af9b60a0-1431-4f62-bc68-3311714a69ad}); regression tests de-self-referenced to hard-coded real bytes; negative guard (fabricated values → ERR_BAD_MAGIC) + bytes→text guard added; harness + probe log + parity doc corrected; suite 303/303 Linux ASan |
| 2026-10-25 | S-010 | **FIXED** | volume quiesce on the repair write path (IWriteDevice lock contract, execute() lock-before-write + zero-write refusal, Windows RwDevice runs the corpus RecoveryModule LOCK/DeleteMountPoint/Flush/settle/verify/DISMOUNT sequence); finding's destination-lock bullet corrected (corpus: destination open is writability+detection only); 3 fail-before tests, suite 316/316 |
| 2026-10-25 | S-008 | **FIXED** | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector (`e0175b3`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | S-009 (disk-map half) | **FIXED** | GAP-S6-11 disk-map data producer (`56cce87`); split of the combined file tracked in N-020 |
| 2026-10-25 | M-006 | **FIXED** | fuzz CI re-enabled (`e30b84d`); verified this leg (ci.yml :147-160) |
| 2026-10-25 | N-002 | **FIXED** | FAT mask-24 EOC family (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-003 | **FIXED** | exFAT checksum og-parity default (`fb44073`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | N-010 (FAT BAD sentinel) | **FIXED** | ordering fix (`cb8dafd`); independently re-verified this leg; in `findings/FIXED/` |
| 2026-10-25 | OQ-T1 | **RESOLVED** | secondary claim map semantics (decompile read); in `findings/FIXED/` |
| 2026-10-25 | I-001..I-005 | **ACKNOWLEDGED** | deliberate supersets verified + pin tests (`f74a5dd`); re-verified this leg |
| 2026-10-25 (S2) | S-010, S-011, M-007, M-008, M-009, N-011..N-020, N-021, I-006, I-007 | **NEW** | parallel session-2 leg, wave 2 |
| 2026-10-25 (S2, this leg) | M-010 | **NEW** | S-005 residual invalid-UTF-8 edge; ZFS-side parity verified first-hand (FUN_18001c630) |

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
