# MINOR N-214 — the lost-partition ext probe's journal-indicator consistency gate absent: the og rejects candidates with exactly one journal indicator set; the core accepts them

**Severity:** MINOR
**Module:** lost-partition (ext probe acceptance rule)
**Parameters affected:** #1 (detection correctness — false-positive candidate class), #15 (confidence grading model — the accept/reject boundary), #30 (candidate scoring inputs)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:1273-1293` (`FUN_180002b90`'s ext classification: `*(uint*)(param_2+0x45c) & 4` = feature_compat HAS_JOURNAL, `*(int*)(param_2+0x4e0)` = journal_inum — ext2 = BOTH clear; ext3/4 = BOTH set; **exactly one set → NO classification at all**, the candidate is dropped), read first-hand this session; companion analyser rule `ExtFileSystemAnalyser_decompiled.c:3616-3680` (the OQ-S52-1 decode, landed by the racing fix lane at `982efeb5`)
**Core references:** audited `3799dee2`, re-verified LIVE at `271f5d06` — `src/core/lost/cr_candidates.cpp:380-441` (`probe_ext`: the SB magic/geometry gates, `s_block_group_nr == 0`, `s_first_ino >= 3`, the two-formula group-count cross-check — **no journal-indicator check anywhere**); `src/core/ext4/cr_ext4_engine.cpp:239-250` (`ext_family_kind`, the landed kind stamp — kind stamping only, no probe-side reject)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's lost-partition ext classifier (`CExtXBlockParser`'s parse, PRD `FUN_180002b90`) requires the two journal indicators — `feature_compat & HAS_JOURNAL` and `s_journal_inum` — to AGREE: both clear → ext2 (type 5); both set → ext3/4 (type 6); exactly one set → the candidate is rejected outright (no classification, no size derivation, no candidate). The core's `probe_ext` validates geometry but never consults the journal indicators, so a torn ext superblock with a set journal flag but a zeroed journal inode (or vice versa) is proposed as a lost-partition candidate where the og refuses it.

## What the reference does

PRD `FUN_180002b90` (PRD:1273-1293), read first-hand this session:

```c
uVar3 = FUN_180002880(puVar2);                        /* the SB validator */
if (((uVar3 & 0xff) != 0) && (*(short *)(param_2 + 0x45a) == 0)) {   /* + s_block_group_nr == 0 */
    FUN_180001a00((longlong)param_4,local_14);
    if (((*(uint *)(param_2 + 0x45c) & 4) == 0) || (*(int *)(param_2 + 0x4e0) == 0)) {
        if (((*(uint *)(param_2 + 0x45c) & 4) == 0) && (*(int *)(param_2 + 0x4e0) == 0)) {
            FUN_180001a20((longlong)param_4,5);        /* ext2 — BOTH indicators clear */
            ... size derivation, FUN_1800039b0 consistency check ...
        }
        /* else: MIXED (exactly one set) — NOTHING: no classification, no candidate */
    }
    else {
        FUN_180001a20((longlong)param_4,6);            /* ext3/4 — BOTH indicators set */
        ... size derivation, FUN_1800039b0 consistency check ...
    }
}
```

The offsets: `param_2+0x45c` = SB+0x5C (`s_feature_compat`), `param_2+0x4e0` = SB+0xE0 (`s_journal_inum`) — the SB buffer base is `param_2+0x400`. The mixed case falls through with no type stamped and no size derived — the partition entry is never surfaced as an ext candidate.

## What our core does

`probe_ext` (`src/core/lost/cr_candidates.cpp:380-441` at the re-verified pin) applies the og's lost-scan gates (magic, geometry, `s_block_group_nr == 0`, `s_first_ino >= 3`, the two-formula group-count cross-check) and scores `+40` base + `+30` GDT-liveness — with no journal-indicator check. A torn SB (compat&4 set, journal_inum zeroed — a classic torn-write shape) passes every core gate and is proposed with evidence.

## Gap

Stated falsifiably: construct a lost-partition fixture with a valid ext SB geometry where `feature_compat & 0x4` is set but `s_journal_inum == 0` (or the converse); the og's PRD classifier refuses the candidate; the core's `probe_ext` accepts it (score 40 or 70).

## Impact

A false-positive lost-partition candidate class (torn ext superblocks the og never proposes). The candidate would still pass through scoring/dedup, so the user impact is a spurious entry in the lost-partition set on damaged disks — not wrong recovered content. Consistent with the N-200 calibration (lostpart probe acceptance weaker = MINOR).

## Reproduction / verification

Corpus: PRD:1273-1293 quoted above, read first-hand. The kind-stamping half of the family is landed (the OQ-S52-1 resolution `982efeb5`: the analyser's verdict rule + `ext_family_kind`), so this finding is scoped precisely to the PROBE acceptance rule. Core: `probe_ext` read at `3799dee2` and re-verified at `271f5d06` — no journal check (grep over the function body: zero hits for `journal|compat`). Evidence: `evidence/s69-lane-sheets/S69-C-sectormath.md` §1.d.

## Fix plan (for the engineering team)

1. Add the consistency gate to `probe_ext` after the existing gates: read `feature_compat` (SB+0x5C) and `s_journal_inum` (SB+0xE0); if `(compat & 0x4) != 0` XOR `journal_inum != 0` → return not-ok (the og's silent drop; `-100` in the core's convention).
2. File: `src/core/lost/cr_candidates.cpp` (one gate in `probe_ext`).
3. Optionally record the ext2-vs-ext3/4 distinction in the candidate's evidence tags (type 5/6 semantics) — the kind stamping is already landed, so this is a probe-side provenance nicety, not a requirement.
4. Tests: `Lost.ExtMixedJournalIndicatorsRejected` (both torn shapes: flag-set/inum-zero and flag-clear/inum-nonzero), `Lost.ExtJournalConsistentAccepted` (both clear → accepted; both set → accepted — the two og-accepted shapes still pass).
5. Acceptance: the two mixed fixtures are refused; the consistent fixtures' scores unchanged; the existing ext probe test family stays green.

## Tests to add

`Lost.ExtMixedJournalIndicatorsRejected` / `Lost.ExtJournalConsistentAccepted`.

## Related findings

N-200 (the lostpart NTFS probe's positive-evidence requirement — the same acceptance-rule family), OQ-S52-1's resolution `982efeb5` (the kind-stamping half, landed), M-152 (the ext4 sweep's orphan-reserved skip — a different ext acceptance seam).

## Open questions

None. (The PRD type-5/6 space vs the DRWUI kind 4/5/9 space mapping is dispositioned by the OQ-S52-1 resolution; this finding needs only the XOR gate.)

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's push (the S80/S81 MINOR-first fix lane's N-214 landing)
**Verified by:** RED-first via the stash dance — `LostDiscovery.ExtMixedJournalIndicatorsRejected` FAILED at the pre-fix tree (both torn shapes anchored candidates, exactly the finding's claim) and PASSES post-fix; `LostDiscovery.ExtJournalIndicatorsConsistentAccepted` (the both-set ext3/4 control) green on both sides; the existing ext probe family (Ext4SuperblockAnchorsDeletedPartition, Ext4BlockGroupCopyNotACandidateStart) green; full suite 1497 ran / 1455 passed / 42 Win-only skipped / 0 FAILED.
**Resolution notes:** The XOR gate landed in `probe_ext` after the group-count cross-check, reading the RAW SB bytes (`sb_le32(sb + 0x5C)` feature_compat, `sb_le32(sb + 0xE0)` journal_inum): `(compat & 4) != 0` XOR `journal_inum != 0` → return −100 (the og's silent drop; its −6 return convention maps to the core's not-ok). One implementation subtlety discovered during the landing: the gate MUST read the raw bytes, not the parsed `SuperBlock` — `validate_superblock` defaults `journal_inum` to 8 (JOURNAL_INODE_DEFAULT) when the field is zero, which would corrupt the XOR's journal leg on every ext2 volume (the default is an engine-side access convenience, not a probe input; noted in the in-code comment). Corpus re-verified first-hand this session: PRD:1273-1293 quoted verbatim in the finding — both clear → type 5, both set → type 6, mixed → `local_18` stays −6 with no classification. The finding's fix plan followed exactly (step 3's evidence-tag nicety not taken — the kind stamping is OQ-S52-1's landed surface, and the PRD type-5/6 space vs the core's CandidateFs collapse is already dispositioned there).
