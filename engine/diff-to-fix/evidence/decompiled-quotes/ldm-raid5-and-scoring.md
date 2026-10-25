# Evidence — LDM RAID-5 rotation + lost-partition scoring (OQ-T7/T9 closures)

Source: `re-workspace/reversed/ghidra-mo/LdmManager_decompiled.c` (91,475 lines),
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`,
`re-workspace/raw-evidence/ui-interaction-recovery-flow/applog_log_CalcPartition.log` (208
lines). Readers: agents S2-D1, S2-G3 (scratch `S2-D1-context.md`, `S2-G3-context.md`). Core: `e93decf`.

## OQ-T7 — og RAID-5 rotation (RESOLVED: left-symmetric, hardwired)

```
Raid5VolumeStorage ctor FUN_180087150 :68924-62
  :68943  *(undefined4 *)(param_1 + 0x2b0) = 0;      // layout selector ZEROED
  :68948  SPEC copy = 0x275 bytes to outer+0x18      // ends at +0x28C — never reaches 0x2b0
  → no other writer of +0x2b0 anywhere in ghidra-mo: PERMANENTLY layout 0 (left-symmetric)
  (SPEC layout proven by GetSpec FUN_180056810 :47300-05 0x275-byte copy; SetSpec :47830-49;
   member array at SPEC+0x26d, count at +0x269, interleave BYTES at +0x25d)

SPEC builder FUN_180073400 :59787+
  :59856  match VOLUME by GUID memcmp (volume+0x32)
  :59879-88  match COMPONENT by child extents
  :59907  spec+0x269 = member count (component children)
  :59924  spec+0x25d = *(int*)(comp+0x4c) * 0x200      // interleave bytes from VBLK
  :59925  memcpy(spec+0x10, volumeGUID, 0x10)
  :59929-44  kind enum 1→1, 6→2, 7→4, 0xb→3, else 9

Read path FUN_180086a90 :68822-58
  r = (offset + S)/S - 1;   g = r/(N-1);   c = offset % S
  layout 0: col = ((N-1) - (g % N) + 1 + (r % (N-1))) % N       // LEFT-SYMMETRIC
  member offset = g*S + c
  on read failure: FUN_180085c30 XOR-reconstructs across the other members
Write path FUN_180086530 :68704-48 computes both parity col and data col
  (layouts 1/2/3 branches present but DEAD — selector never leaves 0)
Core: cr_ldm.cpp:1346-59 ci=s/chunk; row=ci/(cols-1); i=ci%(cols-1);
  parity=(cols-1)-(row%cols); col=(parity+1+i)%cols; member LBA row*chunk+in — IDENTICAL.
```
Geometry edges: og interleave 0 → 0x10000 default (ctor `:47665`, Open `:68632-41`);
core refuses `chunk==0` (`cr_ldm.cpp:1344`) → N-012b. Config-size: og rejects ≥ 0x801 sectors
(`:59750-59 "ConfigSize[%I64d] larger than 1m ?."`) vs core clamp 8192 → N-012a.

## TOCBLOCK candidates (M-016)

```
FUN_180072ad0 :59573  probe config+2;  :59584  config+0;  :59593  config+1;  first valid wins
             :59606  "disk%d Cannot find TOCBLOCK sign \r\n" (rc 0xfffffffd)
             :59612  "config"@+0x24 name check;  :59615 second bitmap name@+0x46
             :59625  "bitmaps are out of range. Giving up."
             :59630-32  VMDB at TOCBLOCK-declared bitmap1 (+0x90 = configStart + toc+0x2e)
config start :59748  PRIVHEAD@0x12B → controller+0x80
Core: cr_ldm.cpp:1115  toc_offs[4] = {1, 2, 2045, 2046}
```

## OQ-T9 — lost-partition scoring (RESOLVED: ln(count)/ln(max), content-driven)

```
DRWUI FUN_14050fda0 :842164-278  (per candidate CSearchedPartitionInfo:
    +8 FS, +0x18 DataAreaStart, +0x20 files, +0x28 folders, +0x68 bonus, +0x70 score)
  :842248-56  count: FS==8 → files + folders*10;  FS 1|2 → (files+folders)*50;
              else → files + folders*50        (NTFS 3, exFAT 7, …)
  :842258     count += *(longlong *)(info + 0x68)                 // per-FS bonus
  :842260-66  if (FS == entry FS && DataAreaStart + param_4 == entry start)
                 count += 500;  log "Found once partition:%I64d\r\n"
  :842226-39  maxCount = max over scan;  denom = (maxCount > 1) ? ln(maxCount) : 1.0
  :842269-72  num = count; if (1 < count) num = ln(num);         // count==1 stays 1.0 (!)
  :842274     score = num / denom                                 // "dSuggestted"
AdjustPartitionInfoOrder FUN_140510410 :842756-851
  :842756-69  0.1 / 0.3 score-gap keep rules
  :842776-93  over-large duplicate (bytes < 0x1000001 ≈ 16 MiB tolerance) → flag
  :842851     score-copy from the first candidate + splice-to-front
  :842812-21  FAT multi-child keep (CFATSearchedPartitionInfo array > 1)
sort comparator :846927  descending by score (merge sort FUN_1405178a0)
log gate :845547/:845642  c:\log\CalcPartiton.log existence; format :845789
```
Verified against all 187 log rows to 6 decimals: 429f → ln(429)/ln(168849)=0.503579 ·
(0,25)+bonus20 → 0.585518 · (0,1)+20 → 0.348069 · 1f → 1/ln(168849)=0.081928 · 16f+2d+30+500 →
ln(681)/ln(681)=1.0 · over-large zero-file → raw 0.0, re-scored 0.632748 = ln(60)/ln(646) (the
first candidate's score copied). Core `cr_candidates.cpp:61-158` absolute structural points —
divergent (M-017).
