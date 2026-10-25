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

---

## Session-4 update (2026-10-26, commit `63a9167`) — PARTIAL RESOLUTION

1. **NTFS +0x68 bonus setters FOUND** (independent-verification leg, verbatim):
   `NTFSSearchDLL_decompiled.c:28804` `+= 0x32` (one-shot, flag at +0x178);
   `:28944/:28973/:29002` `+= 0x1e` (three sites); `:29263` generic adder
   `FUN_180024990` (called with 0x1e at :28996) — the MFT-run candidate-stitching
   code. Values 30/50 per site — consistent with the CalcPartition-log
   "NTFS 60 or 30" observations (two +30 sites, or 30 via one site).
   *Remaining:* the exact per-site trigger conditions (which stitching outcomes
   earn which bonus) — decode when porting M-017's scoring model.
2. **FAT32 "20" setter: NOT-FOUND** — searched FATSearchDLL/exFATSearchDLL for the
   `+0x68` accumulator patterns (`+ 0x14)`, `+ 0x1e)`, `+ 0x32)`): no match. Either
   the FAT32 bonus is set through the generic adder with a different value, or the
   "20" observation in the CalcPartition logs has a different source. Keep open.
3. **ZFS pool display name: negative evidence re-confirmed** — 0 hits for "name"/
   "pool" (case-insensitive) in `ZfsSearchDLL_decompiled.c`; the og engine never
   reads the label nvlist "name". The DRWUI render path for FS 0xD remains the
   undecoded half.
4. **Btrfs premise CONFIRMED** — `BtrfsSearchDLL_imports.txt`: zero MultiByteTo-
   WideChar/WideCharToMultiByte/mbstowcs imports (byte-oriented engine). The
   DRWUI-side render conversion remains the open half.

OQ-T9's full feature/weight table advances by item 1; the remaining decode work is
now concentrated in: per-site trigger conditions (NTFS), the FAT32-20 source, the
DRWUI render paths (ZFS name, Btrfs conversion).
