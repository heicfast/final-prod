# SUMMARY.md — executive summary (session 2 — post-rebase with parallel leg fixes)

**Audited commit:** `5d6da5f` (session 1 baseline `67193b6` plus concurrent fixes through
`5d6da5f` for A-001/M-001/S-001/S-005 + GAP-S6-11 disk-map) → `8574039` (HEAD after parallel
remediation leg's S-002/S-003/S-004/S-008/M-006/N-002/N-003/N-010-theirs/I-001..I-005-ack
fixes + clang-format gate repair). **Corpus:** `03ad46c` (unmoved). **Audit dates:**
2026-10-25 (session 1) + 2026-10-25 (session 2). **Status:** session 2 wave 1+2+3 complete;
parallel remediation leg landed 6 more fixes (S-004, S-008, M-006-fuzz, N-002, N-003,
N-010-theirs) + acknowledged I-001..I-005 + filed 1 new finding (their N-010) during this
window. Both legs reconciled in this rebase.

## Session 2 + parallel-leg combined delta

Session 2's mandate (find what session 1 missed, continue the audit, remove the fixed
things) landed alongside a parallel remediation leg that closed 6 more findings + filed
1 new finding. Both legs reconciled here.

**Final state (active vs FIXED):**
- ACUTE: 0 (A-001 FIXED)
- SEVERE: 6 (S-002 open; S-003 OPEN per parallel-leg evidence-addendum; S-004 FIXED; S-006
  open; S-007 open; S-008 FIXED; S-009 partially-FIXED via GAP-S6-11; S-010 NEW session 2;
  S-011 NEW session 2) → 6 active + 3 FIXED (S-001, S-004, S-005, S-008)
- MAJOR: 4 active (M-002, M-005, M-007, M-008, M-009) + 1 FIXED (M-001)
- MINOR: 16 active (N-001, N-004..N-009, N-011..N-020, N-021) + 4 FIXED (N-002, N-003,
  N-010-theirs)
- INFO: 7 active (I-001..I-007 — all ACKNOWLEDGED per parallel leg, deliberate supersets)
- tracked-open: 8 (OQ-T2..OQ-T9; OQ-T1 RESOLVED)
- corpus-errata: 18 candidates cataloged (CORPUS-ERRATA.md)

## Counts per severity (active — excludes FIXED/ and ACKNOWLEDGED-but-not-moved)

| Severity | Active | FIXED (in `findings/FIXED/`) | IDs (active) |
|---|---|---|---|
| ACUTE | 0 | A-001 | — |
| SEVERE | 6 | S-001, S-004, S-005, S-008 | S-002, S-003, S-006, S-007, S-009 (partial), S-010, S-011 |
| MAJOR | 5 | M-001 | M-002, M-005, M-007, M-008, M-009 |
| MINOR | 16 | N-002, N-003, N-010 (parallel-leg-theirs) | N-001, N-004..N-009, N-011..N-020, N-021 |
| INFORMATIONAL | 7 (all ACKNOWLEDGED, not moved) | — | I-001..I-007 |
| tracked-open | 8 | OQ-T1 (RESOLVED) | OQ-T2..OQ-T9 |
| **Total active** | **42** | **9 FIXED** | |

**Note on S-009/M-006:** the S-009 component (disk-map visualization) was FIXED via GAP-S6-11
(commit `56cce87`); the M-006 component (fuzz CI) was FIXED by the parallel leg (commit
`e30b84d`). The combined finding file `S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md`
should be split: S-009 → FIXED/; M-006 → FIXED/. Tracked as a follow-up (N-020 documents
this drift). For now the combined finding file stays in `SEVERE/` until split.

**Note on N-010 ID conflict:** the parallel leg's N-010 (FAT BAD sentinel ordering) and
session 2's N-010 (FAT invalid-date sentinel) are DIFFERENT findings about FAT sentinel
handling. The parallel leg's N-010 was filed first (commit `cb8dafd`) and FIXED in the
same commit. Session 2's N-010 was renumbered to N-021 to avoid the ID conflict.

## Counts per module (active — excludes FIXED/)

| Module | ACUTE | SEVERE | MAJOR | MINOR | INFO | OPEN | Complete |
|---|---|---|---|---|---|---|---|
| ntfs | – | – | 2 (M-007, M-008) | 3 (N-001, N-011, N-012) | 1 (I-001 ack) | OQ-S2A-1..4 | ✔ |
| fat | – | – | – | 2 (N-004, N-021) | 1 (I-002 ack) | OQ-T2 | ✔ |
| exfat | – | 1 (S-011) | – | – | – | OQ-T5 | ✔ |
| ext4 | – | 1 (S-003 OPEN) | – | 1 (N-004) | 2 (I-006, I-007) | – | ✔ |
| hfsplus | – | 2 (S-002, S-003) | – | 1 (N-005) | 1 (I-007) | – | ✔ |
| btrfs | – | – | – | 1 (N-006) | 2 (I-003 ack, I-007) | – | ✔ |
| zfs | – | – | – | – | 1 (I-007) | OQ-T6 | ✔ |
| refs | – | – | (project GAP: ADS) | 1 (N-007) | 1 (I-004 ack) | – | ✔ |
| ldm/table | – | – | 1 (M-002) | – | – | OQ-T7 | ✔ |
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

## Top risks (ranked — post-rebase)

1. **S-010 (NEW session 2, safety) — RawFixer write device bypasses FSCTL_LOCK_VOLUME +
   FSCTL_DISMOUNT_VOLUME.** Highest-priority new finding. The og's `RecoveryModule.dll`
   performs the full lock+unmount+flush+sleep-2s-settle+verify sequence at
   `RecoveryModule_decompiled.c:44686-44720`. The core's `RwDevice` opens with
   `FILE_SHARE_READ|WRITE` and no lock. Concurrent processes can race with repair writes —
   silent volume corruption. Compounds S-008's wiring gap (now FIXED) with a safety gap on
   the write path.
2. **S-011 (NEW session 2, parity) — exFAT `tz_offset` applied (core) vs ignored (og).**
   Every non-UTC exFAT file's `.rsf` diverges by `tz_offset × 900 × 10⁷` (100ns units). For
   UTC+8 files that's 8 hours. Spec-correct UTC vs og's local-as-UTC. One-line fix.
3. **S-002 (open) — HFS+ attributes tree (decmpfs) missing.** macOS compressed files
   recover as empty shells. (Unchanged from session 1; S-003 evidence addendum landed but
   fix deferred.)
4. **S-003 (open) — HFS+ journal replay missing.** Crashed volumes lose journal-carried
   recent state. Parallel leg decoded the journal replay mechanics first-hand (commit
   `b85c084`) but the fix is deferred with precise scope.
5. **S-006/S-007 (open) — discovery coverage.** No lost-partition probes for Linux/Mac/ReFS
   filesystems; carve misses GIF/BMP/headerless-MP3/AAC/RTF/OLE-Office/MPEG/DWG. (Unchanged.)
6. **M-007/M-008 (NEW session 2, superset) — NTFS hard-link fan-out + `$ATTRIBUTE_LIST`
   multi-hop chase.** Both SUPERSET divergences where the og's behavior is a limitation.
   Opt-in parity flags recommended.
7. **M-009 (NEW session 2, perf) — RawFixer bypasses the 64 KiB direct-I/O envelope.**
   ~3.5× slow path on 4Kn NVMe (the documented GAP-S6-13 ratio). One-line fix.
8. **OQ-T7 — LDM RAID-5 rotation unverified.** Safety-critical; if the og's rotation differs
   from the assumed left-symmetric, every reconstructed RAID-5 member carries wrong bytes.
   (Unchanged.)

## Recommended fix order (first 20 — post-rebase)

| # | Finding | Rationale |
|---|---|---|
| 1 | **S-010 (NEW)** | safety regression on the repair path; can corrupt volumes during concurrent access |
| 2 | **S-011 (NEW)** | exFAT tz_offset parity break; one-line fix (drop the tz subtraction); matches og's local-as-UTC |
| 3 | S-002 | decmpfs content recovery (HFS+ primary user scenario) |
| 4 | S-003 | HFS+ journal replay (mechanics decoded by parallel leg; fix scoped) |
| 5 | S-006 | lost-partition probe families (engine wiring only) |
| 6 | S-007 | carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG |
| 7 | **M-009 (NEW)** | RawFixer bypasses 64 KiB envelope; one-line fix (route through aligned_read_at) |
| 8 | **M-007 (NEW)** | hard-link fan-out superset; opt-in parity flag (mirrors S-001's pattern) |
| 9 | **M-008 (NEW)** | `$ATTRIBUTE_LIST` chase scope+depth superset; opt-in parity flag |
| 10 | M-002 | EBR torn-chain salvage |
| 11 | M-005 | carve bitmap gating (perf + FP surface) |
| 12 | **N-021 (NEW, was N-010)** | FAT invalid-date yields non-zero FileTime vs og zero; one-line guard |
| 13 | **N-013 (NEW)** | dest-overlap optional + first-extent-only; composes with S-010's lock fix |
| 14 | **N-014 (NEW)** | stage checkpoint not atomic; atomic-rename fix |
| 15 | **N-015 (NEW)** | writer no disk-full pre-check; composes with S-010's lock fix |
| 16 | **N-016 (NEW)** | emit_boundary_events double-call bug; one-line removal |
| 17 | **N-017 (NEW)** | cancel watcher coverage inconsistent; one-line parallel_for poll |
| 18 | **N-018 (NEW)** | UBSan CI lane; mirror the TSan lane |
| 19 | **N-019 (NEW)** | 12 missing per-parameter regression tests |
| 20 | **N-020 (NEW)** | 6 stale doc claims; split S-009/M-006 finding file; move both to FIXED/ |

## Audit coverage statistics

- **Core read (session 1 + session 2):** 52 src .cpp (24,793 LOC), 53 headers, 37 test
  files (313+ TEST cases after parallel leg's S-002/S-003/S-004/S-008/N-002/N-003/N-010
  additions) — agents C+D exhaustive; agents S2-A/B/C/D added line-by-line reads of the
  NTFS mftwalk, all 8 engine .cpp, query/writer/session/transport/rawfixer/orchestrator.
- **Corpus read (session 1 + session 2):** 9 architecture docs, 274 artifact docs, raw
  decompiled trees. **Session 2 added first-hand reads of the
  `re-workspace/reversed/ghidra-partition/*.c` decompiled files** (RecoveryModule,
  PartitionRecovery, RawFixer) that session 1 did NOT use — these contained the canonical
  FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME sequence (S-010 evidence) and the
  ACL-preservation `SetFileSecurityW` calls (N-009d refinement).
- **Executed verifications:** all new findings verified by first-hand decompile reads
  (file:line cited) + first-hand core reads (file:line cited) + cross-engine consistency
  checks. The parallel leg added fail-before/pass-after test proofs for each of their 6
  fixes (suite 313/313).
- **Not executable here:** og-execution (no Windows/wine) and new Ghidra decompiles — all
  such questions are in `tracked-open/` with precise probes, or in each finding's "Open
  questions" section.

## Honesty notes

- Session 1's NTFS B7 "MATCH" verdict for hard-link/ADS/sparse was **REFUTED** by session 2's
  line-by-line decompile read (S2-A agent). Three new findings (M-007, M-008, N-011) and one
  minor (N-012) surfaced. The session-1 verdict was based on a misread of the LOCK/unlock
  block as hard-link refcount management.
- Session 1's Q67 "M" (sanitizer cleanliness) was incomplete — UBSan is not enabled anywhere
  (N-018). Session 1's Q65 "M" (test coverage) was aspirational — 12 of 13 audit-prompt
  scenarios lack isolated tests (N-019). Session 1's Q68 "M" (documentation accuracy) had 6
  stale claims (N-020).
- The 9 fixed findings (A-001, S-001, S-004, S-005, S-008, M-001, M-006, N-002, N-003,
  N-010-theirs) are preserved in `findings/FIXED/` for audit trail. They are NOT counted
  in the active severity totals above. `findings/FIXED/RESOLVED-INDEX.md` catalogues each
  resolved finding's title, fix commit, and resolution summary.
- The 7 ACKNOWLEDGED informational findings (I-001..I-007) are deliberate supersets the
  parallel leg verified and pinned with regression tests. They stay in
  `findings/INFORMATIONAL/` (not moved to FIXED/) because they're not bugs — they're
  documented improvements. The parallel leg's pin tests guard against anyone "fixing"
  parity by removing the superset behavior.
- The N-010 ID conflict (parallel leg's FAT BAD sentinel vs session 2's FAT invalid-date)
  was resolved by renumbering session 2's finding to N-021. Both findings are valid and
  distinct.
- The 18 corpus errata candidates are cataloged in `diff-to-fix/CORPUS-ERRATA.md`.

## Remediation log (combined: parallel leg + session 2 cleanup)

| Date | Finding | Verdict | Evidence |
|---|---|---|---|
| 2026-10-25 | A-001 | **FIXED** | canonical GUIDs (`1cb6a43`); moved to `findings/FIXED/` |
| 2026-10-25 | M-001 | **FIXED** | record-size ladder (`5d6da5f`); moved to `findings/FIXED/` |
| 2026-10-25 | S-001 | **FIXED** | destroy rule + sparse-run sub-fix (`e93decf`); moved to `findings/FIXED/` |
| 2026-10-25 | S-004 | **FIXED** | ext4 journal every-block salvage (`51ac671`/`e0175b3`); moved to `findings/FIXED/` |
| 2026-10-25 | S-005 | **FIXED** | shared UTF-8 decoder (`2f3237d`); moved to `findings/FIXED/` |
| 2026-10-25 | S-008 | **FIXED** | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector (`e0175b3`); moved to `findings/FIXED/` |
| 2026-10-25 | M-006 | **FIXED** | fuzz CI re-enabled (`e30b84d`); finding file combined with S-009 — split tracked in N-020 |
| 2026-10-25 | N-002 | **FIXED** | FAT mask-24 EOC family (`fb44073`); moved to `findings/FIXED/` |
| 2026-10-25 | N-003 | **FIXED** | exFAT checksum og-parity default (`fb44073`); moved to `findings/FIXED/` |
| 2026-10-25 | N-010 (parallel-leg-theirs) | **FIXED** | FAT BAD sentinel ordering (`cb8dafd`); moved to `findings/FIXED/` |
| 2026-10-25 | OQ-T1 | **RESOLVED (decompile read)** | secondary claim map semantics — moved to `findings/FIXED/` |
| 2026-10-25 | I-001..I-005 | **ACKNOWLEDGED** | deliberate supersets verified + pin tests added (`f74a5dd`); stay in `findings/INFORMATIONAL/` |
| 2026-10-25 (S2) | S-010, S-011, M-007, M-008, M-009, N-011..N-020, N-021, I-006, I-007 | **NEW (session 2)** | authored in session 2 wave 2 from 4 parallel agents' context sheets |
