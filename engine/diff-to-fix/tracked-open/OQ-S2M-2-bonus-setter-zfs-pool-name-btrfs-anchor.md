# OQ-S2M-2 — SearchDLL bonus-field setter (+0x68), ZFS pool display name, and the Btrfs name-conversion anchor

**Status:** OPEN (session-2 residues) · **Modules:** lost-partition, zfs, btrfs · **Parameters:** A30, B10, D13-adjacent
**Audited commit:** `e93decf` · corpus `03ad46c`

Three small open residues surfaced while closing OQ-T9/T6 and re-verifying S-005. None blocks a fix; each pins an exact constant or site the fix plans cite.

## 1. The per-FS bonus field setter (+0x68)

OQ-T9's decoded scorer consumes a per-filesystem bonus at `CSearchedPartitionInfo+0x68` (DRWUI deserializer `:942701`; ctor default 1 at `:942638`). Observed values from the CalcPartition log: **NTFS 60 or 30, FAT32 20, usually 0**. The setter lives inside the SearchDLLs (they serialize the candidate info over IPC) — not yet located.

**Next probe:** grep `NTFSSearchDLL_decompiled.c` / `FATSearchDLL_decompiled.c` / `exFATSearchDLL_decompiled.c` for stores of 60/30/20 (0x3C/0x1E/0x14) into the candidate-info struct near the ICalcPartition family (`artifacts/SearchDLL/engine-matrix--factory-contract.md` documents the factory; the per-binary function maps exist). Affects M-017's default weights.

## 2. ZFS pool display name source

The og never reads the label nvlist "name" field (`ZfsSearchDLL_decompiled.c` — zero hits; only "vdev_tree"/"id"; `root_dataset` drives enumeration). The core reads label "name" for the pool display (`cr_zfs_engine.cpp:1014`) — a superset. **Question:** what does the og actually display as the pool name — a constant, the dataset name from `root_dataset`, `$ORIGIN`, or the device label?

**Next probe:** DRWUI volume-label render path for FS code 13 (ZFS) — `ghidra-drwui` volume-tree builder; alternatively the SaveScan writer's session header for ZFS scans.

## 3. Btrfs name-conversion anchor (S-005 og-side)

EXT4 passthrough and the ZFS `MB2WC(0xFDE9,0,…)` site (`:24985/:25014`) are proven. **BtrfsSearchDLL imports no conversion API at all** — no MultiByteToWideChar/WideCharToMultiByte — so where does the og convert on-disk UTF-8 dir-item names for display?

**Next probe:** the SearchDLL→controller string contract (is the Btrfs path byte-oriented end-to-end until DRWUI, like ext4?), or the MO-layer bridge. If DRWUI converts at render time with dwFlags=0, S-005 parity holds for Btrfs too and N-016a is the only residual.

## Acceptance

Each item resolves with a single quoted site (grep-sized probes, ≤ 30 min each with the function maps). Fold: #1 → M-017 defaults; #2 → `modules/zfs.md` B10 note; #3 → `resolved/findings/S-005-*.md` addendum.
