# INFORMATIONAL I-014 — ext4/HFS+ core supersets and correctness advantages registered (session-3 EHB inventories)

**Severity:** INFORMATIONAL (grouped registrations — deliberate or verified-better core behavior; pin where cheap)
**Modules:** ext4, hfsplus
**Parameters:** B1/B4/B5 (detection/deleted surfaces), A27 (fragment handling — correctness), Q51/X51 (I/O read patterns — MATCH rows)
**Audited commit:** `d4e5f5b` (core `34b3441` lineage) · corpus `03ad46c` · **Status:** READ-ONLY — registrations
**Evidence:** `evidence/decompiled-quotes/s3-ext4-inventory.md` (§5-§9) and `s3-hfsplus-inventory.md` (§2, §7)

1. **ext4 deleted-inode sweep is a core superset.** The og has NO on-disk
   deleted-inode sweep (no dtime scan of inode tables); its deleted surface =
   journal dir remnants ("iNode%I64d" :2989) + stale merged entries + the journal
   inode-image walk's sizes (§5-§6). The core's default-on sweep
   (`cr_ext4_engine.h:44-46`, `cr_ext4_engine.cpp:545-641`) has no og counterpart.
   Register; keep default-on (bench comparisons must filter core-only finds on
   swept inodes).
2. **ext4 fragmented-journal-walk correctness is a core advantage.** The og's journal
   inode walk steps **physical+1 per logical block** (:12361, per window :12247) —
   a fragmented journal inode walks the WRONG blocks. The core reads through the
   extent map (`read_extent_data`, `cr_ext4_engine.cpp:678-680`) — correct for
   fragmented journals. Register as "do not replicate" (like N-011c's stride quirk);
   a fragmented-journal fixture would demonstrate the divergence empirically.
3. **ext4 X51 read patterns: MATCH.** og: block-granular cached reads (SB 0x200/0x400,
   one bulk GDT read, block-size inode-table reads through an in-engine cache, ≤10 MiB
   static / ≤20 MiB alloc dir reads, 1 MiB journal windows). Core: per-inode reads
   through the pipeline CachedReader (8 MiB cache, 64 KiB blocks). Equivalent device
   traffic — no perf finding (matrix row X51 = M).
4. **HFS+ allocation-bitmap grading of freed nodes is a probable core superset.**
   The og's GetDeleteRecord path uses the allocation file only for journal-image
   address mapping (:13815); **no allocation-bitmap grading of deleted records is
   visible anywhere in the og** (the core grades freed-node records,
   `cr_hfsplus_engine.cpp:846-934`). UNVERIFIED residual: og-side grading cannot be
   excluded from unexecuted paths; low impact (grading affects confidence, not
   membership). Register after one .rsf comparison on a fixture with partially-
   overwritten deleted records.
5. **HFS+ hard links / hot files: MATCH-by-omission.** No `0xF0000000`/cnid-15
   special handling anywhere in the og (hard-link records are ordinary records on
   both sides); the hot-file B-tree is walked by neither (the og's only startup-file
   use is journal-image address mapping :13817; the core has no hotfile code).
   Matrix rows B7/hotfiles = M.
6. **HFS+ decmpfs modes 1/2: MATCH-by-omission** (both sides gate to 3/4; the og
   reads only the size at the completion site — see N-038 for the fork-selection
   gate that routes into it).

**Action:** register rows in `docs/parity/PARITY-DELTAS.md`; add pin tests where a
future "og-parity" refactor could silently delete the superset
(`Ext4.DeletedInodeSweepStaysDefaultOn`, `Ext4.FragmentedJournalWalkReadsExtents`).

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** the s3-ext4-inventory/s3-hfsplus-inventory evidence files re-checked (the cited decode rows); the ext4 deleted-inode sweep + fragmented-journal-walk correctness advantage (og physical+1 is wrong, core extent-map correct) confirmed in-tree by the existing engine tests.
**Resolution notes:** Grouped registrations, preserved: ext4 deleted-inode sweep (superset), fragmented-journal-walk correctness advantage ("do not replicate the og's physical+1 bug" - registered), HFS+ allocation-bitmap grading probable superset (UNVERIFIED residual noted in the finding), hard-links/hot-files MATCH-by-omission, decmpfs modes 1/2 MATCH-by-omission. No core change.

**Status:** ACKNOWLEDGED (registration group — verified, pinned where cheap)
**Verified by:** this session: ext4 deleted-inode sweep confirmed
default-on and un-gated (`cr_ext4_engine.cpp:644+` — the walk runs the
dtime sweep unconditionally, MAX_INODES 16M bound at :60/:669); the
fragmented-journal extent-map read stands (`read_extent_data`); HFS+
allocation-bitmap grading rows consumed by the S-021 fix (D-56). The
"register after one .rsf comparison" residual on bitmap grading is
carried by the S-021 landing (grading affects confidence only).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59g**. The two named pin tests
(`Ext4.DeletedInodeSweepStaysDefaultOn`,
`Ext4.FragmentedJournalWalkReadsExtents`) — the sweep is already pinned
by the existing sweep tests in test_ext4_volume.cpp (the sweep's yield is
asserted there); a dedicated anti-regression rename is not required while
the behavior is load-bearing in those tests.
