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

---

## Session-14 addendum (DFX14 — pushed forward; probe 1 run, NEGATIVE)

Probe 1 (the per-FS bonus setter +0x68) was run this session as specified:
grep over NTFSSearchDLL/FATSearchDLL/exFATSearchDLL decompiles for direct
stores of 0x3c/0x1e/0x14 into candidate-info +0x68 — zero hits. The store
must go through a register/local (or the candidate-info serializer's own
layout); the next concrete step is refined: decode the SearchDLLs'
candidate-info serializer entry points (engine-matrix--factory-contract.md
lists the factory surface) and read the +0x68 fill from the serializer,
not from a grep. Probes 2-3 (ZFS pool display name; Btrfs name-conversion
anchor) remain as filed.

---

## Resolution (session DFX31 — all three items RESOLVED)

**Status:** RESOLVED (all three probes; evidence `evidence/decompiled-quotes/s31-oq-s2m-2-bonus-setter-zfs-pool-name-btrfs-anchor.md`, every quote re-read first-hand this session, corpus `03ad46c`)

1. **The +0x68 bonus setter — located and fully decoded.** The fill happens in
   the scan-path composers (the serializer at NTFSSearchDLL:45692-45709 copies
   the field verbatim; ctor default `param_1[0xd] = 1`). Session-14's "zero
   hits" is explained: the stores are accumulator latches on local/base
   registers, not direct immediates. Decoded:
   - **FAT** (FATSearchDLL:6946-6969 + :7142-7167, in FUN_180009cb0):
     `+0x68 = boot_quality + fs_quality`, each ∈ {10, 45, 50} — boot from
     flags(+0xa0)&0x140, fs from &0xc (FAT32, flavor +8==2) / &3 (FAT12/16).
     Flag producers: FUN_180009960 (:6693/:6704-6707/:6719) — FAT12 bit0 /
     FAT16 bit1 = `param_4 == 0`; FAT32 = `param_4*4+4` (bit2 always, bit3 =
     the backup-boot verdict from the classifier at :8004-8052 — the sector
     at BPB+0x1C offset +6, media-descriptor checks, FAT32-only). Bit 8
     (0x100) of the boot pair has NO setter in the 64,658-line decompile —
     the boot 50-tier is unreachable in this build.
   - **exFAT** (exFATSearchDLL:39504-39526): the same ladder on bits 30/31
     (boot) + bits 0/1 (fs) of flags(+0xe0).
   - **NTFS**: composite — base score from +0x9c flag bits (30/10/30/10/10/
     10/10) + three clamped-10 overlap terms (FUN_18000dc70 :10538-10573);
     boot-sector validation ±(FUN_1800097d0 :7229-7233: validated at a
     negative relative offset → +50; unreadable → hard −10); MFT-run
     stitching +30 per merged fragment (:28944/:28973/:29003 + the generic
     adder FUN_180024990 at :28996/:29263) and the one-shot +50 for chains
     that survived >3 broken links (:28803-28806, latch +0x178).
   - **The "FAT32 20" decoded**: `10 + 10` — both ladders at minimum, for
     candidates whose creation path set no ladder bits (e.g. the :5322-5324
     creator sets only `| 0x10`). No "20" constant exists anywhere.
   - **M-017 fold**: the core's landed `fs_bonus()` (NTFS 60/30, FAT 20)
     reproduces the observed CalcPartition rows (187 rows, 6 decimals); the
     decode grounds those numbers as generator outputs and confirms the flat
     FAT 20 is the honest parity for the core's single-probe architecture
     (the og's tiers key on internal creation-path identity the IPC surface
     does not carry). No code change — the landed model is correct.
2. **ZFS pool display name — there is none.** ZfsSearchDLL has 0 hits for
   "name"/"pool" (re-verified); DRWUI's FS-name builder FUN_140510080
   (:842335-842396) has no case 13 (ZFS) → **"Unknow"** (consumers: the XML
   `<filesystem>` field :207358 and the CalcPartition log row :845776). The
   "ZFS partition" string (strings:39717) is the GPT type-GUID table name,
   not a volume label. The core's label-`name` read
   (cr_zfs_engine.cpp:1014) registered as a deliberate superset in
   modules/zfs.md.
3. **Btrfs name-conversion anchor — the DRWUI render layer, CP_UTF8,
   dwFlags=0.** BtrfsSearchDLL imports zero conversion APIs (re-verified);
   names are raw UTF-8 through IPC into the .rsf session; the conversion is
   FUN_140163af0 (DRWUI:264664-264689, invoked on the restore path at
   :357529 — convergent with N-029's cite) + the FUN_1400d9ef0 family
   (:190981-190997, ~10 call sites). S-005 parity holds for Btrfs; the
   predicted residuals N-016a/N-016b are already FIXED (N-029/M-010). No
   code change.

**Verified by:** the s31 evidence artifact (20 verbatim decompile quotes, all
line-cited; three negative greps re-run first-hand: ZfsSearchDLL "name"/"pool"
= 0/0, BtrfsSearchDLL conversion imports = 0, FATSearchDLL `| 0x100` setter =
absent); the M-017 log-row parity suite (unchanged and green — the landed
model is the decode's confirmation target).

**Resolution notes:** The OQ's three residues close with zero code change:
the +0x68 decode CONFIRMS the landed M-017 model (the observed values are
generator outputs of the decoded ladders; the core's flat values are the
correct mapping for its architecture), the ZFS pool name never existed in the
og (superset note registered), and the Btrfs conversion anchor lands exactly
where S-005/N-029 predicted. The audit-trail updates: this file → FIXED/,
RESOLVED-INDEX row, modules/zfs.md superset note, modules/lost-partition.md
bonus-model note, SUMMARY counts.
