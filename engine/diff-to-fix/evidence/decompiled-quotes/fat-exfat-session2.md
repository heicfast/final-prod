# Evidence — FAT/exFAT session-2 quotes (mirror, windows, validators, gates)

Source: `re-workspace/reversed/ghidra-searchdll/{FATSearchDLL,exFATSearchDLL}_decompiled.c`,
`ghidra-partition/RawFixer_decompiled.c` (corpus `03ad46c`). Reader: agent S2-B (context
sheet `S2-B-context.md` in session scratch). Core side: `e93decf`.

## Mirror semantics (OQ-T2 → M-012)

```
FUN_180006140 :3809-14   children +8 = FAT1 @ lVar13, +0x10 = FAT2 @ lVar13 + spf
FUN_180041de0 :54216-18  if (child1 && (uVar2 = child1->Next(c)) != 0) return uVar2;   // primary wins
             :54226-48  mirror scrutiny: reject Next(c+1)==v+1 (shifted), reject
                        Next(v-1)==v (cross-link), accept iff Next(v)==v+1; forward:
                        reject v ∈ {0x0FFFFFF7, 0x0FFFFFF8, 0xFFFFFFFF}
             (child1 NULL → mirror answer returned RAW — mirror-only scan)
FUN_180006140 :3817-50   whole-table retry from RB-tree alternate-FAT map on validation fail
```
BAD canonicalization: CFAT16 `FUN_180041c80 :54135 if (uVar1 == 0xfff7) return 0xffffff7;`
(non-zero → no fallback) · FAT12 `FUN_1800416f0 :53798-804` returns 0xFF7 raw (I-002 confirmed).

## Table windows (M-011) + validation (M-015)

```
FUN_180036e90 :44743   FAT16 gate: 0xc/(bps>>9) <= spf                       (rejects spf<12)
             :44775   FAT32 gate: (0x100/(bps>>9)) <= spf <= (0x96000/(bps>>9))   [128KiB..300MiB]
FUN_180041b00 :54052   FAT16 validate: FAT[0] == 0xFFFFFFF8 exactly (media 0xF8)
             :54060+  entry sweep (OOB / ==1 / self-loop invalid)
             :54084-114  claim-map walk: cross-link or table-covering chain → table REJECTED
FUN_1800417c0 :53853-919 CFAT32 validate: family-tolerant FAT[0]/[1]; watermark DAT_1800728d4
             :44782   = entries * 0.98; gated by deep-check flag +0x30 (:53867)
```

## Flavor boundary (N-010a) + BPB gates (N-010b)

```
FUN_180007e50 :5219-31   FAT16 range [0xff5, 0xfff4) exclusive; FAT32 lower 0xfff4 inclusive
FUN_18002f970 :38048/:38062  _strnicmp(boot+0x36,"FAT16   ",8) / (boot+0x52,"FAT32   ",8)
             :38055  *(ushort*)(boot+0xb) == device_sector_size;  spc bitmask 0x800000008000808b
             :8012   flavor 0xE → engine declines
```

## exFAT FAT window (S-013) — no cap; 1 MiB windows

```
FUN_180021c60 :28039-28213  operator_new(0x100000) window; entry = *(int*)(win + (cluster & 0x3ffff) * 4)
                          (:28158); FREE-mid-chain assumed-contiguous (:28158-63)
```
Core: `cr_exfat_quick.cpp:499-505` (≤32 MiB load gate) → `:366-370 contiguous_chain(first,1)`.

## Entry-set checksums (M-013, refutes N-003's premise)

```
FUN_18001eba0 :25153-98  TWO ROL15 checksums over (sec_count+1)*32 bytes skipping idx 2..3:
                         raw + InUse-restored (byte0→0x85, type bytes→0xC0/0xC1 :25173-86);
                         valid iff either == stored *(u16*)(entries+2)
FUN_18001fef0 :26291    wired into set completion
FUN_1800203a0 :26642-64 reconstruction oracle: candidate substitution re-test
```

## Structural gates (M-014)

```
FUN_180002f90 :1787-91  secondary_count ∈ [2,18] ((byte)(*p - 2U) < 0x11); create-timestamp
                        sanity (month 1-12, day≠0, hour<24, min<60, sec<60); modify same
FUN_180029280 :34562-64 stream accept: data_length != 0 && first_cluster != 0 &&
                        name_length != 0 && next type&0x7F == 0x41   (empty files DROPPED)
FUN_180003030 :1814-29  name entry: flags&3==0, flags<4, first char ≠ 0, UTF-16 validity
FUN_180003090 :1840-46  reserved-field byte-OR tests; :1857-59 stream gate
                        first_cluster != 1 && < 0xFFFFFFF7   (polarity: core right, artifact wrong)
FUN_1800365a0 :44298-4315 FAT dentry date sanity (month/day per field)
FUN_18000f950 :11075-116  mid-chain FREE salvage gated on RB-tree claim/bitmap maps
FUN_18000fb80 :11231-43   canonical BAD mid-chain → directory read aborts (device-size check)
```

## Boot-checksum predicate (OQ-T5 — CORRECTED by the main auditor's own decompile re-read)

```
RawFixer FUN_18000e7e0 :9353-81  for (i=0; i < param_2*0xb; i++) {
                                    local_28 = FUN_180023230(begin, end, &i);   // std::find
                                    lVar2   = FUN_180023220(0x180079a40);        // end()
                                    if (local_28 == lVar2)                        // NOT in table
                                      chk = (chk << 0x1f | chk >> 1) + byte[i];  // ROL31 + add
                                  }
```
CORRECTION: the earlier reading ("ROL31 over ALL bytes, no skip") was WRONG — the loop
accumulates only when the index is NOT found in the -1-terminated int table at
`DAT_180079a40` (the same table is consumed as a plain array elsewhere: `:10949-50
while (*(int*)(&DAT_180079a40 + n*4) != -1)`). The skip table = **{106, 107, 112}**
(MS-spec skip set), matching the parallel legs' binary extraction. **Core MATCH**
(`cr_exfat_dentry.cpp:26-36` spec-skip) — the N-017 candidate filed from the earlier
mis-reading is RETRACTED by this leg (see worklog). CheckDBR gates on computed != 0 +
sector-11 fill-consistency (`:10539-41`); UpdateDBR fill-repeats the recomputed value
(`:11429-34`). Scanner side: exFATSearchDLL has ZERO checksum code (no checksum step in
`FUN_180021e80 :28180-210`) — scanner parity intact.
