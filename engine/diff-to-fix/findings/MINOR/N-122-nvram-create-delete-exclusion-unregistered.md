# MINOR N-122 — the NVRAM Boot#### create/delete exclusion lives only inside M-040's resolution text ("No create/delete (the finding's v1 scope)") — D-90 registers only boot.ini/chkdsk; a system with a MISSING Boot#### variable cannot be repaired by core and the exclusion has no register row

**Severity:** MINOR
**Module / Surface:** boot repair (modules/boot-repair.md)
**Parameters affected:** A42-adjacent, 62-adjacent (the missing-variable case is the og's create path)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-mo/EFIBoot_decompiled.c` (the create/patch/delete state machine — :251-693, :3405-3480, :4130-4273 — the cites already recorded in M-040's own corpus-reference block); the EFIBoot.md module census (189 functions).
**Core references:** audited commit `ff879f9` — `src/win/cr_bootfix.cpp` — the nvram stage PATCHES existing entries and the walk STOPS on a missing Boot#### variable (the test NvramFix.MissingBootVariableStopsTheWalk pins it); the create/delete halves are absent, and the absence is recorded only in M-040's resolution text (findings/FIXED/M-040...md:160-161 "No create/delete (the finding's v1 scope)") — D-90 (:2966+, PARITY-DELTAS) registers boot.ini/chkdsk dispositions only, grep-verified.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** M-040 landed with the v1 scope note; the scope-out never became a register row.

## Summary

("The og's EFIBoot module can create and delete NVRAM Boot#### entries (a missing entry — the common corrupted-boot case — gets created). Core's nvramfix only patches existing entries and stops the walk when the variable is missing, so a system whose Boot#### entry was destroyed keeps a broken boot chain. The exclusion of the create/delete halves is recorded only as an in-finding remark; the register row that D-90 should carry does not exist.",)

## What the reference does

As cited — EFIBoot's create/patch/delete state machine, the same decode M-040 already carries.

## What our core does

As cited — patch-only + stop-on-missing.

## Gap

Falsifiable: a fixture with a missing Boot#### for the target OS: the og's flow creates it; core's walk stops (its own test pins the stop).

## Impact

The repair story for the destroyed-entry case (the patch path cannot help); the registration gap is the trail-truth half.

## Reproduction / verification

Corpus evidence: the EFIBoot cites (M-040's block + this session's re-read); core behavior pinned by the existing test.

## Fix plan (for the engineering team)

1. Register first: add the create/delete exclusion to D-90 with the v1-scope rationale (or file the implementation as follow-up work). 2. If implementing: the EFIBoot state machine's create path (:3405-3480 decode) — allocate the next free Boot#### slot, build the entry (the og's template), set BootOrder; delete = the :4130-4273 path (the walk-then-remove semantics). 3. Update NvramFix.MissingBootVariableStopsTheWalk's expectation if the behavior changes.

## Tests to add

- `NvramFix.MissingBootVariableCreated` — the missing-entry case ends with a created entry (RED as long as the exclusion stands — the test documents the delta).
- `NvramFix.BootOrderUpdatedOnCreate` — the og's BootOrder semantics.

## Related findings

M-040 (the parent landing); N-121 (the same boot-chain reachability family); D-90 (the register to amend).

## Open questions

The create path's entry-template details (the og's exact friendly-description string and the :3405-3480 walk) — one decode pass before implementation; the capability's existence is pinned.
