# CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

**Session 2 + parallel-leg combined changes:**
- New finding IDs (session 2): S-010, S-011, M-007, M-008, M-009, N-011, N-012,
  N-013, N-014, N-015, N-016, N-017, N-018, N-019, N-020, N-021 (was N-010,
  renumbered to avoid conflict with parallel-leg's N-010), I-006, I-007.
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
| refs | N-007, I-004 ack, GAP-ADS | M | M | M | M | NA | NA | M | M | M | M |
| table+ldm | **A-002 (S2-LDM leg)**, M-002 | M | **A-002 (S2-LDM leg)**, M (A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
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
