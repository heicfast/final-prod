# OQ-S42-2 — Btrfs nsec handling in the og: does the engine fold sub-second precision or carry seconds only?

**Origin:** S42 continuation lane, timestamp focus battery (Agent B's context sheet, residue #7)
**Status:** OPEN (this session)

## The precise question

The og's Btrfs engine reads the inode timespec fields at atime@0x70 / ctime@0x7C / mtime@0x88 / otime@0x94 (`BtrfsSearchDLL_decompiled.c:4430-4446`) but **otime is never converted and +0x94 is zeroed** (the trail's btrfs module rows). The inode timespec's on-disk layout is `{sec: i64, nsec: i32}` — the question is whether the og's conversion reads and folds the nsec half into the emitted FILETIME (sub-second precision) or truncates to whole seconds like its ext4 sibling does (`_gmtime32_s` with `wMilliseconds=0`, the D-59a registration).

The core folds nsec: `src/core/btrfs/cr_btrfs_engine.cpp:873-880` via `unix_to_filetime_ns`. If the og truncates, the core is an I-006-family precision superset (benign, registerable); if the og folds, they match. Either way the trail currently has **no og-side nsec cite for Btrfs** (D-59b covers only the 3-vs-4 attribute count), so the parity claim for Btrfs sub-second precision is unverified.

## Next probe (exact)

`re-workspace/reversed/ghidra-btrfs/BtrfsSearchDLL_decompiled.c:4430-4446` — read the timespec load: a single `i32/i64` load at each field address followed by a seconds-only epoch fold (`*10000000` style) means truncation; a paired sec+nsec load with an nsec/100 addend into the FILETIME means folding. One read resolves it.

## Consumers

The btrfs module row B9, I-006's superset family register (this would join or be refuted), S-057's restoration finding (precision of what gets restored).
