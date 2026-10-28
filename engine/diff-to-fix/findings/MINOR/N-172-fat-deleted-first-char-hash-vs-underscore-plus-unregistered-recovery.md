# MINOR N-172 — FAT deleted-entry first character: the og substitutes `'#'`; the core uses `'_'` plus an unregistered LFN-checksum recovery superset that restores the ORIGINAL byte

**Severity:** MINOR
**Module:** fat (deleted-name presentation)
**Parameters affected:** filename handling (10), recovery correctness — name axis (2), deleted-file classification presentation
**Corpus references:** `FATSearchDLL_decompiled.c:44166-44172` (`uVar8 = 0x23`; `if ((char)local_a8 == -0x1b) cVar2 = '#';` — the 0xE5 deleted first byte becomes `'#'` in the 8.3 name builder `FUN_180036350`; NO recovery of the destroyed byte anywhere in the DLL)
**Core references:** `src/core/fat/cr_fat_quick.cpp:184-203` (deleted → `lfn_deleted_checksum_recover` restores the original byte when the LFN oracle exists, else `short_name[0] = '_'`), `src/core/fat/cr_lzn.cpp:76-111` (the bijection recovery); the GAP-S6-1 bench-proven comment at :177-186; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

For a deleted FAT 8.3 entry, the og renders the destroyed first byte as `'#'`
("#EADME.DAT"). The core renders `'_'` when no LFN oracle exists ("_EADME.DAT") and — a
documented, bench-proven superset — brute-forces the ORIGINAL byte through the stored
LFN checksum when slots survive ("readme.txt" where the og shows "#EADME.TXT"). This
closes OQ-T2's cosmetic question (fat.md B4): the og's convention is now decoded. The
placeholder mismatch is cosmetic; the recovery superset is unregistered.

## What the reference does

The name builder's substitution (quote above) is the only 0xE5 name treatment in the
DLL — verified by lane E's sweep.

## What our core does

The two-branch presentation (cites above; the recovery is deterministic bijection over
256 candidates validated against the stored checksum).

## Gap

Presentation char: `'#'` (og) vs `'_'` (core, no-oracle branch). Superset: the
original-byte recovery (core only), unregistered.

## Impact

Name-axis divergence on the deleted-FAT population: benches comparing name sets must
normalize the placeholder; the recovery superset means core name sets are strictly
richer (a deliberate quality win, invisible to parity tables until registered). No
recovery-content impact.

## Reproduction / verification

Corpus-derived (lead auditor verified FAT:44166-44172 directly — the 0x23 constant and
the -0x1b comparison). Core-derived (the two branches read directly). PARITY-DELTAS:
grep for 0xE5/placeholder/deleted-name registrations: zero rows. GAP-S6-1's comment
documents the recovery but no registry row exists.

## Fix plan

1. Register the LFN-checksum recovery as a deliberate superset (one PARITY-DELTAS row
   citing both sides + the bench probe).
2. Align the no-oracle placeholder to `'#'` for og parity (one character at
   cr_fat_quick.cpp:199) — or keep `'_'` and fold it into the same registration row;
   the og-parity choice is recommended for bench comparability.
3. Tests: the placeholder pin + the recovery pin (the recovery already has bench
   provenance; add a unit pin).
4. Acceptance: the fat.md B4/OQ-T2 row closes with the decode; benches normalize or
   match.

## Tests to add

- `Fat.DeletedFirstCharPlaceholderIsHash` (if aligned; else `...IsUnderscoreAndRegistered`)
- `Fat.DeletedLfnChecksumRecoversOriginalByte` (superset pin)

## Related findings

- N-175 / N-176 (the same name-builder family: NTRes case, illegal-char fallback)
- M-130 (the same walk's validator family)

## Open questions

None — OQ-T2 is closed by this finding.
