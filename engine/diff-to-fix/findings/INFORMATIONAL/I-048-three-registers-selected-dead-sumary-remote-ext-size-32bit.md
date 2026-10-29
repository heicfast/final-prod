# INFORMATIONAL I-048 — three deliberate-difference registers from the S69 local-DB and sector-math deep dives

**Severity:** INFORMATIONAL
**Module:** query (display config) / carve (sign data) / lost-partition (ext size math)
**Parameters affected:** #46 (documentation of the contract) for all three registers; #15/#30 for register C
**Corpus references:** (A) `og-codebase/FileFilter.xml` (the `Selected="2"` root attr + 15 `<Selected>2</Selected>` children at lines 1,3,33,328,765,769,790,794,1077,1081,1300,1304,1309,1530,1534) + the grep record over DRWUI/exFATSearchDLL/EUVideoRepair (only Qt stylesheet-state hits: DRWUI:1209830, :1209887); (B) `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:319007-319022` (`value_fastgoodfile` + `value_sumaryfiletype` remote-config keys read into the results-config singleton, stored at +0x280) + the shipped `og-codebase/remote_config_data` cache (both keys present); (C) `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:1407-1414` (`FUN_180003090`: `return *(int *)(param_2 + 4) * (int)uVar1;` — the 32-bit signed product) vs `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c:3234` (`CONCAT44(param_1[0x54],uVar1)` — the engine's 64-bit block count)
**Core references:** audited `3799dee2`, re-verified LIVE at `271f5d06` — (A) `src/core/query/cr_filefilter_data.cpp` (the generated M-023 table: no Selected field — correct); (B) `src/core/query/cr_config_display_data.cpp:27-116` (the N-132 static tables — the ini snapshot); (C) `src/core/ext4/cr_ext4_engine.cpp:152` (`blocks_lo | (u64(le32(sb + SB_BLOCKS_HI)) << 32)` — the engine-faithful 64-bit form) + `src/core/lost/cr_candidates.cpp` (`c.sector_count = v.blocks_count * v.block_size / ls` — the 64-bit probe size)
**Status:** READ-ONLY REGISTER — deliberate differences recorded, no defect

## Summary

Three registers from the S69 deep dives, each a deliberate or data-level difference that should be recorded so future lanes do not re-litigate them: (A) FileFilter.xml's `Selected` fields are dead data in the shipped format (no reader in any decompiled binary — the core's omission is correct); (B) the og's summary-filetype/fast-good-file groups are remote-tunable at runtime (the core's static tables are the ini snapshot — the remote layer is REPORTING-OOS by prior disposition); (C) the og's lost-partition ext size estimate is a 32-bit signed product while its own engine uses 64-bit — the core's probe uses the engine's 64-bit form, the deliberate engine-faithful choice.

## What the reference does / what our core does

**Register A — FileFilter.xml `Selected=2` is dead data.** The shipped file carries the attribute/elemant pair at 16 sites; the only "Selected" string consumers in the decompiled binaries are Qt stylesheet states (`WSHover`/`WSNormal`/`Selected` theming, DRWUI:1209830/:1209887). No XML reader touches the field. The core's M-023 table has no such field — correct by omission. Registered so no lane burns time decoding "Selected semantics" that do not exist.

**Register B — the summary-family groups are cloud-tunable.** DRWUI's results-config init reads `value_fastgoodfile` and `value_sumaryfiletype` from the remote-config settings singleton (DRWUI:319007-319022, stored at singleton+0x280); both keys ship in the `remote_config_data` cache. The core's N-132 tables are the static ini snapshot. The remote-config layer is REPORTING-OOS by prior disposition; the register records that the og's groups can drift from the shipped ini at runtime and the core's cannot — acceptable under the OOS disposition. (Precedence between the remote value and any ini default is undecoded — no ini-side SumaryFileType reader was found in DRWUI; the remote value may be the only runtime source.)

**Register C — the og's lost-partition ext size is 32-bit; its engine is 64-bit.** PRD `FUN_180003090` (PRD:1407-1414, read first-hand): `return *(int *)(param_2 + 4) * (int)uVar1;` — s_blocks_count read as a signed 32-bit int, multiplied as int×int (the product is 32-bit signed; ≥2^31 sectors goes negative). The og's own engine validator uses the full 64-bit count (EXT4SearchDLL:3234, `CONCAT44(blocks_hi, blocks_lo)`). The og is internally inconsistent: the lost-partition estimate truncates where the engine does not. The core's probe uses the 64-bit form (`cr_ext4_engine.cpp:152` + `probe_ext`'s sector_count) — the engine-faithful superset, registered as deliberate (the D-50 calibration family: adopting the og's own probe defect would be a down-fix).

## Gap

None — three non-defect registers (dead data correctly omitted; OOS-dispositioned remote layer; deliberate engine-faithful superset).

## Impact

None behaviorally. The registers prevent future duplicate investigation and pin the truth for the doc layer.

## Reproduction / verification

All corpus sites read first-hand this session (the FileFilter.xml line census, the DRWUI remote-key reads, the PRD/EXT4SearchDLL pair); all core sites re-verified at `271f5d06`. Evidence: `evidence/s69-lane-sheets/S69-A-localdb.md` §B.4/§D.4/§D.5 + `S69-C-sectormath.md` §2.

## Fix plan (for the engineering team)

Registration only: add the three rows to the PARITY-DELTAS register (or the PARITY-OG-CORE-INVENTORY's config-layer section for A/B) with these cites. No code change.

## Tests to add

None.

## Related findings

M-023 (the FileFilter table landing), N-132 (the display-config tables), D-50 (the deliberate-superset calibration family), N-214 (the ext probe's journal gate — the same probe, a real divergence this time).

## Open questions

Register B's precedence question (remote vs ini default) — immaterial under the OOS disposition; recorded for completeness.
