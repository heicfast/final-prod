# CROSS-REF-MATRIX.md — module × parameter verdict matrix

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | M (S-001, M-001 fixed), I-001 | M (S-001 fixed) | M | M | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt) |
| fat | M | M | M | I-002, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | N-003, OQ-T5 | N-003 | M | M | M | NA | NA | M | M | M | M (debt) |
| ext4 | S-004, M (S-005 fixed) | ok | M (erratum) | S-004 | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002, S-003, N-005 | M | M | M | M | NA | NA | M | M | M | THIN |
| btrfs | I-003, M (S-005 fixed), N-006 | M | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | N-007, I-004, GAP-ADS | M | M | M | M | NA | NA | M | M | M | M |
| table+ldm | M (A-001 fixed), M-002 | M | M (A-001 fixed) | M-002, OQ-T7 | ok | NA | NA | M | M | M | M (debt) |
| lost-partition | S-006, OQ-T9 | NA | M | OQ-T9 | M | NA | NA | M | M | M | THIN |
| carve | S-007, M-005 | M | S-007, N-008 | S-007, N-008, M-005 | M | NA | NA | M-005 | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12) | M | NA | M | NA | NA | M | M | I-005.7 | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | N-009d | M (debt) |
| health | M | NA | N-009a | NA | M | NA | NA | NA | M | M | THIN |
| rawfixer | S-008 | NA | M | S-008 | M | NA | NA | M | M | S-008 | M |
| orchestrator | M | M | NA | M-005 | M | M | NA | M | M | M | M |
| visualization | S-009 | GAP-cats, M | NA | M | M | NA | M | M | M | NA | M |
| abi (cr_ui) | NA | M | ok | NA | M | M | S-009 | M | M | M | M |
| cli | N-009c | NA | M | NA | M | NA | M | NA | M | M | ok |

Cross-cutting rows: **Q66 = M-006** (fuzz CI disabled) · **Q67 = M** (ASan/TSan CI
green; TSan separate job) · **Q68 = M** (docs current; the A-001 false comment is the
counterexample, fixed by its own finding) · **Q69 = M** (zero stub markers repo-wide,
grep-verified) · **Q70 = M** (Win7 posture + import allowlist verified by CI PE-gate)
· **X47/X48/X49/X50 = tracked-open** (og-execution not possible here; bench harness is
the vehicle — see `tracked-open/` notes in OQ files) · **X51 = M** post-GAP-S6-13
(64 KiB envelope; og driver is 512-sector contract — core superset) · **X53 = M**
(cancel watcher 50 ms + per-record polls).

A cell reading `M (X-n fixed)` marks a finding the remediation session closed —
the finding file's Resolution section carries the fix commit and test evidence;
the underlying parameter is MATCH as of that fix.

Group keys: B behavioral · D data model · C constants/formats · A algorithms ·
S state machine · O orchestration · P API · X performance · R architecture ·
F safety · Q quality (definitions in `PARAMETERS.md`).
