# CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

**Session 2 + parallel-leg combined changes:**
- New finding IDs (session 2): S-010, S-011, M-007, M-008, M-009, N-011, N-012,
  N-013, N-014, N-015, N-016, N-017, N-018, N-019, N-020, N-021 (was N-010,
  renumbered to avoid conflict with parallel-leg's N-010), I-006, I-007.
- New finding IDs (session-2 MAIN-AUDITOR leg, renumbered post-rebase to avoid the
  collisions above): **A-004, S-012, S-013, S-014, M-011..M-021, N-023..N-029,
  I-009, I-010, I-011**, plus OQ-S2M-1/OQ-S2M-2 (tracked-open). One candidate
  RETRACTED by the leg's own re-verification (exFAT fixer checksum predicate —
  core MATCH; see modules/exfat.md addendum).
- New finding ID (parallel leg): N-010 (FAT BAD sentinel) — FIXED in same commit.
- Resolved (moved to `findings/FIXED/`): A-001, S-001, S-004, S-005, S-008,
  M-001, M-006 (combined with S-009 — split tracked in N-020), N-002, N-003,
  N-010-theirs, OQ-T1.
- Acknowledged (stay in INFORMATIONAL/ with pin tests): I-001..I-005.
- S-009 partially FIXED (disk-map visualization via GAP-S6-11); the combined
  S-009/M-006 finding file remains in SEVERE/ pending split (tracked in N-020).

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | M-007, M-008 (S2), N-011, N-012 (S2); I-001 ack | M (S-001 fixed), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2) | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | **S-011 (S2)**, N-003 FIXED, OQ-T5 | N-003 FIXED; **S-011 (S2)** | M | M | M | NA | NA | M | M | M | M (debt) |
| ext4 | S-003 OPEN, S-004 FIXED; **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M | S-003 OPEN, S-004 FIXED | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 OPEN, S-003 OPEN, N-005; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | THIN |
| btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010**, I-007 (S2) | M; **M-010**, I-007 (S2) | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | N-007, I-004 ack, **M-022 (S3)**, I-012 (S3: GAP-ADS resolved — parity) | M | M | M | **M-022 (S3)** | M | NA | NA | M | M | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| lost-partition | S-006, OQ-T9 | NA | M | OQ-T9 | M | NA | NA | M | M | M | THIN |
| carve | S-007, M-005 | M | S-007, N-008 | S-007, N-008, M-005 | M | NA | NA | M-005 | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12), N-014 (S2) | M | NA | M; N-014 (S2) | NA | NA | M | M | I-005.7 ack | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2), N-015 (S2)**, N-009d | M (debt) |
| health | M | NA | N-009a | NA | M | NA | NA | NA | M | M | THIN |
| rawfixer | M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | M | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | M-005 | M; **N-016 (S2), N-017 (S2)** | M | NA | M | M | M | M |
| visualization | S-009 (partial-FIXED-in-tree, finding file stale — N-020) | GAP-cats, M | NA | M | M | NA | M | M | M | NA | M |
| abi (cr_ui) | NA | M | ok | NA | M | M | S-009 | M | M | M | M |
| cli | N-009c | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2) |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2), N-019 (S2), N-020 (S2)**; M-006 FIXED |

Cross-cutting rows: **Q66 = M-006 FIXED** (fuzz CI re-enabled by parallel leg `e30b84d`;
codeql deferral documented). **Q67 = M + N-018 (S2)** (ASan/TSan CI green; UBSan NOT enabled —
session 2 found this gap). **Q68 = N-020 (S2)** (session 2 found 6 stale doc claims;
previously marked M). **Q69 = M** (zero stub markers repo-wide, grep-verified). **Q70 = M**
(Win7 posture + import allowlist verified by CI PE-gate). **X47/X48/X49/X50 = tracked-open**
(og-execution not possible here; bench harness is the vehicle). **X51 = M post-GAP-S6-13 +
M-009 (S2)** (64 KiB envelope on the scan path; **session 2 found the repair-path `RwDevice`
bypasses it** — M-009). **X53 = M + N-017 (S2)** (cancel watcher 50 ms + per-record polls;
**session 2 found the orchestrator parallel candidate scan lacks the carve watcher** — N-017).

A cell reading `M (X-n fixed)` or `X-n FIXED` marks a finding the remediation session closed
— the finding file's Resolution section carries the fix commit and test evidence; the
underlying parameter is MATCH as of that fix. Cells with finding IDs marked **(S2)** are new
session-2 findings; the previous `M` verdict was refuted by first-hand decompile + core
source read. Cells marked `ack` are ACKNOWLEDGED informational findings (deliberate supersets
with pin tests, stay in INFORMATIONAL/).

Group keys: B behavioral · D data model · C constants/formats · A algorithms ·
S state machine · O orchestration · P API · X performance · R architecture ·
F safety · Q quality (definitions in `PARAMETERS.md`).


---

# Main-auditor leg cells (session 2, third leg — decompile-exhaustive reads; core `e93decf` re-verified on `34b3441`)

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/*session2*`) |
|---|---|---|
| ntfs | B1/B4/D15/A26/B5 | **S-012** — two-pass architecture: og pass 1 emits deleted ungraded/undestroyed; destroy+viability+0x3ff+blanket-$ live in the missing GUID-dump pass 2; size-0 rescue missing; S-001's fix mis-scoped (see FIXED/S-001 postscript) |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024** (8.3 short-name field; two-sided boot ladder; pass-2 stride quirk superset; namespace-ranking superset; og error facility codes) |
| fat | B1/S35/X49 | **M-011** (table-size windows: og FAT32 128 KiB–300 MiB, FAT16 ≥12 sectors; core 16 MiB reject = scan-failure class) |
| fat | A27/B2/S35 | **M-012** (mirror policy: primary-wins-on-nonzero incl. BAD, scrutinized mirror answers, mirror-only mode; supersedes the N-022 sub-case) |
| fat | B1/B3/A27 | **M-015** (og validates loaded tables: media-word canonical, cross-link wholesale rejection, 98% watermark) |
| fat | B1/C19/C20/B3 | **N-023** (65524 flavor boundary; BPB FS-string/bps/spc-set gates; dentry date sanity; mid-chain FREE/BAD handling) |
| exfat | B2/A27/X47 | **S-013** (FAT > 32 MiB silently dropped → 1-cluster truncation of every FAT-chained file) |
| exfat | B3/B4/B5/D15 | **M-013** (og DOES validate entry-set checksums — dual-form incl. deleted sets + reconstruction oracle; refutes the N-003 fix's verified-absence premise; supersedes N-003) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| hfsplus | B3/B5/B7/C20 | **N-028** (dotfile + "HFS+ Private Data" sink filter; `..namedfork/rsrc` superset; 'prdf' folderization) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| refs | B1/C19/C20/A25 | **N-026** (checkpoint dynamic body offset vs hardcode; two-sided boot gates; second-copy location START+MID vs START+TAIL) |
| table+ldm | B1/C20 | **M-016** (TOCBLOCK candidates: og {+2,+0,+1} vs core {+1,+2,+2045,+2046} — config+0 missed, conflict preference +2 vs +1) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| lost-partition | A30 | **M-017** (OQ-T9 resolved: og ln(count)/ln(max) content scoring, +500 table-match, count==1 quirk, AdjustPartitionInfoOrder rules — verified vs 187 log rows; core absolute points divergent) |
| carve | B1/X47/X51 | **I-009** (byte-stepped probing = detection superset; block-shape constants) |
| carve | B2-adj/R54/R57 | **M-021** (video-repair pre-scan hook absent) |
| carve | D13/B10 | **N-027** (carved-name split inside the core; runtime per-format config surface; ZIP padding inversion + two-tier version gates — refines N-008) |
| session | B10/D12 | **N-029** (U+FFFD replacement-count policy + substitution timing in saved sessions; cross-refs parallel leg's M-010) |
| health | F61/B1 | **S-014** (BitLocker auto-unlock: wrong live registry path, offline-hive path absent, unassociated enumeration — F61 was never scored before) |
| orchestrator | O38/P42/R56 | **A-004** (ErrorRange fires through the config sink while the guard checks the live sink — latent null-call crash) |
| orchestrator/wide | B1/D16 | **M-018** (FilterFolder junk-filter family absent and unregistered) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| cross-cutting | Q68 (corpus docs) | **I-011** (8 corpus artifact claims corrected against the decompile; see CORPUS-ERRATA.md addendum) |
| cross-cutting | Q68/Q46 | **I-010** (scope exclusions + supersets registered: cloud/NAS/repair/WinPE/installer/licensing/i18n; ZFS label-field superset; ext4 primary-SB-only = og parity) |

**Retracted by this leg's own re-verification (recorded, not filed):** the exFAT
ChecksumDBR "no-skip" candidate — the og's fixer skips {106,107,112} via a
`std::find == end()` gate over the −1-terminated table at `DAT_180079a40`; the core's
spec-skip is a MATCH (the parallel leg's binary extraction was right; this leg's first
reading was wrong — corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`).
