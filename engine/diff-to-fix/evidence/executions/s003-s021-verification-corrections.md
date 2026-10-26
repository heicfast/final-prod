# S-003/S-021 verification-correction fail-before / pass-after record

Date: 2026-10-26 (closing session, df5 lineage). Pre-correction tree: `3b81dbc`
(the parallel legs' S-021/S-003 arc, JIB offsets converged but the 'jnlx'@+4
magic check kept). Corrections: JIB magic check removed (TN1150 verbatim:
device_signature[8]@+4..+0x23, no magic field; the og's FUN_18000db70 swaps only
[0..3]/+0x24/+0x2C and its ctor never compares +4), VH kHFSVolumeJournaledBit
gate added (og :13456), the node validator made asm-strict (mid-chain offset-table
violation REJECTS - 0x18001080c/:812 jump past `mov edi,1` at :825), the
attributes branch added (og :14096, decmpfs size-only fallback per
FUN_180009c10 :7840-42 + FUN_180009f40 :7937-82), the header-invalid fallback
salvage ladder implemented (:1715-1760 + FUN_180003ea0), in_fork u64 arithmetic.

## Fail-before (pre-correction worktree at 3b81dbc + the new tests/header overlaid)

```
$ cr_tests --gtest_filter='HfsPlus.Journal*'
[  FAILED  ] HfsPlus.JournalHarvestEmitsDeletedRecords
[  FAILED  ] HfsPlus.JournalDifferentialDifferentFileIdKept
[  FAILED  ] HfsPlus.JournalWrapHandled
[  FAILED  ] HfsPlus.JournalInfoBlockHasNoMagicField
[  FAILED  ] HfsPlus.JournalAttributeDecmpfsSizeFallback
[  FAILED  ] HfsPlus.JournalFallbackSalvageWhenHeaderInvalid
 6 FAILED TESTS
```

The three PRE-EXISTING tests fail because the fixtures now write the
TN1150-verbatim JournalInfoBlock (device_signature zeros at +4) - exactly what
real internal-journal volumes carry - and the pre-correction engine's
'jnlx'@+4 check rejects it: the silent zero-harvest every real journaled volume
would have hit. JournalRequiresVolumeJournaledBit and
JournalNodeImageRejectsInconsistentOffsetTable pass on the pre-correction tree
only because the earlier magic gate masks them (they pin the new gates on the
fixed tree).

## Pass-after (corrected engine)

```
$ cr_tests --gtest_filter='HfsPlus.Journal*'
[  PASSED  ] 11 tests.     (all 5 pre-existing + 6 new)
$ ctest  -> 100% tests passed out of 491
```

JournalInfoBlockHasNoMagicField additionally poisons the whole device_signature
region (0xAB x 32) to prove the bytes are ignored, matching the og's swap scope.
