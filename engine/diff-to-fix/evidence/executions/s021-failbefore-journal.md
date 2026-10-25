# S-021 / S-003 fail-before / pass-after execution log (2026-10-26)

Host: this session's container (gcc + ASan, preset linux-gcc). Corpus: `03ad46c`.

## Corpus first-hand re-verification (this session, before coding)

- `HFSPlusSearchDLL_decompiled.c` read span-by-span: journal-object gates
  (:13530-13560: magic 0x4a4e4c78 LE @+0, endian 0x12345678 @+4, size@+0x18
  cross-checked); GetDeleteRecord ring walk (:13637-14190: start@+0x10,
  "Journal Start Offset:0x" trace, probe reads of u32@+0x28 bytes, the
  FIRST-U16 == (u32@+0x20 >> 4) - 1 transaction test, entry list at +0x20
  with `local_d0 += 8` (ushort) = 16-byte stride, image reads with
  `% journal_size` wrap, per-entry cancel polls, termination when the
  offset returns to jhdr.start); leaf gate (`*(char*)(image+8) == -1`,
  :13799); fork classification order catalog→extents→attributes→allocation
  →startup (:13809-13817, target address = u32@+0x28 × entry u64@+0);
  entry loop count semantics (`local_108 = 1 .. < u16@block+2` → count-1
  entries); the node-kind gate FUN_180010720 (:13049-13100: 512-multiple
  size, kind ∈ {-1..2}, leaf height 1 / index height 0, offset-array
  monotonicity); the fallback salvage reader (:1690-1790).
- **Endianness correction found during the proof:** journal-internal
  structures are LITTLE-endian (jnl.c; the og's ushort loads are native
  LE) - the first fixture draft wrote the transaction magic BE and the
  harvest silently found nothing (the exact "wrong constant = zero
  harvest, silent" failure mode S-021's open question warned about).
  Caught via instrumented gates; fixture corrected; the OQ stands.
- **The og's own robustness defect observed:** the ring walk terminates
  only on `offset == jhdr.start` or cancel - a corrupt ring can cycle
  without ever re-hitting start (no total-bound). Our walk is hard-bounded
  by one full ring of advances (documented hardening, PARITY-DELTAS D-56).

## Fail-before (pre-fix engine, tests kept)

```
$ git stash push src/core/hfsplus/cr_hfsplus_engine.cpp   # header+tests stay
$ ./tests/cr_tests --gtest_filter='HfsPlus.Journal*'
  HfsPlus.JournalHarvestEmitsDeletedRecords  FAILED  (journal_* stats 0,
                                                      node missing)
  HfsPlus.JournalDifferentialDifferentFileIdKept FAILED
  HfsPlus.JournalWrapHandled                  FAILED
  HfsPlus.JournalBadHeaderGates               OK      (no-harvest is its
                                                      expectation)
```

## Pass-after (fix applied)

```
$ ./tests/cr_tests --gtest_filter='HfsPlus.*'   → 14/14 PASS
$ ctest -j$(nproc)                              → 423/423 PASS (ASan)
```

New tests (all in tests/unit/test_hfsplus_volume.cpp):
- `HfsPlus.JournalHarvestEmitsDeletedRecords` — the flagship case: a
  journal transaction image carrying a catalog leaf with a deleted file
  record surfaces as a Deleted-tier node with content extents + bitmap
  grading; the same image's copy of a LIVE record is suppressed by the
  differential.
- `HfsPlus.JournalDifferentialDifferentFileIdKept` — same (parent,name)
  as a live record but a different fileID (re-created file) is kept.
- `HfsPlus.JournalBadHeaderGates` — bad info magic / bad jnl magic /
  size cross-check mismatch each degrade to a no-harvest (scan stays OK,
  the freed-node sweep still works).
- `HfsPlus.JournalWrapHandled` — an image spanning the ring end is
  reconstructed by the split read (its records + offset array live in the
  wrapped tail; the jnl header overlaps only free space).
