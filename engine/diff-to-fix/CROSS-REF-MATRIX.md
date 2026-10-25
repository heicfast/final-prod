# CROSS-REF-MATRIX.md — module × parameter verdict matrix

Legend: `M` = MATCH · `A-n` / `S-n` / `M-n` / `N-n` / `I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`·` = pending audit wave.

Parameter groups: B behavioral · D data model · C constants/formats ·
A algorithms · S state machine · O orchestration · P API · X performance ·
R architecture · F safety · Q quality. Full definitions in `PARAMETERS.md`;
the per-cell parameter numbers (B1..Q70) are expanded in each `modules/*.md`.

Compact per-module rows (parameter columns grouped; see module files for the
full 70-column expansion):

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | · | · | · | · | · | NA | NA | · | · | · | · |
| fat | · | · | · | · | · | NA | NA | · | · | · | · |
| exfat | · | · | · | · | · | NA | NA | · | · | · | · |
| ext4 | · | · | · | · | · | NA | NA | · | · | · | · |
| hfsplus | · | · | · | · | · | NA | NA | · | · | · | · |
| btrfs | · | · | · | · | · | NA | NA | · | · | · | · |
| zfs | · | · | · | · | · | NA | NA | · | · | · | · |
| refs | · | · | · | · | · | NA | NA | · | · | · | · |
| table+ldm | · | · | · | · | · | NA | NA | · | · | · | · |
| lost-partition | · | · | · | · | · | NA | NA | · | · | · | · |
| carve | · | · | · | · | · | NA | NA | · | · | · | · |
| tree | · | · | NA | · | · | NA | NA | · | · | NA | · |
| session | · | · | · | NA | · | NA | NA | · | · | · | · |
| writer+report | · | · | NA | · | · | NA | NA | · | · | · | · |
| health | · | NA | · | NA | · | NA | NA | NA | · | · | · |
| rawfixer | · | NA | · | · | · | NA | NA | · | · | · | · |
| orchestrator | · | · | NA | · | · | · | NA | · | · | · | · |
| visualization (query/preview) | · | · | NA | · | · | NA | · | · | · | NA | · |
| abi (cr_ui) | NA | · | · | NA | · | · | · | · | · | · | · |
| cli | · | NA | · | NA | · | NA | · | NA | · | · | · |

This compact view is the navigation index. The authoritative per-parameter
verdicts with evidence links live in `modules/<module>.md` (one section per
parameter group, one line per parameter: verdict + finding ID + one-line
reason).
