# A-005 fail-before / pass-after execution record

Date: 2026-10-26 (closing session, df5 lineage). Tree: `2aa4530` + this change.
Corpus anchor: TN1150 "Extents Overflow File Key" (verbatim quote in the finding)
+ session-2 evidence `ext4-hfsplus-session2.md` ("extents-overflow key {10, cnid,
startBlock} :7931-35").

## Fail-before (engine change stashed; test compiled against the pre-fix parser)

```
$ ./build/linux-release/tests/cr_tests --gtest_filter='HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork'
tests/unit/test_hfsplus_volume.cpp:891: Failure
Expected equality of these values:
  2u
    Which is: 2
  frag->props()->extents.size()
    Which is: 1
the overflow-sourced 9th extent must resolve
[  FAILED  ] HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork
```

One extent resolved instead of two: the overflow record's descriptors were read
from key+10, so the first "descriptor" was assembled from the key's startBlock
bytes (block 0x00080000-ish) whose byte offset sits far outside the 20-block
volume - resolve_file_extents's bounds clamp zeroed its length and the extent
was silently dropped. This is the production failure mode: fragmented tails
vanish with no error and a plausible file size.

## Pass-after (fix applied: descriptors at key+12, gate 12+64)

```
$ ./build/linux-release/tests/cr_tests --gtest_filter='HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork'
[  PASSED  ] 1 test.
```

extents = {block 11 x 8 (initial), block 19 x 1 (overflow-sourced)}; logical
size 9*BS; st.overflow_extents == 1. Full suite result recorded in the fix
commit message.
