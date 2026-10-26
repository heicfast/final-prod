# Evidence — NTFS pass-2 / GUID-dump architecture (session 2)

Source: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` (corpus `03ad46c`).
Readers: agents S2-A2, S2-G1 (context sheets in session scratch); first-hand re-verification of
the three critical spans by the session-2 main auditor. Core side: `e93decf`.

## 1. Pass-1 per-record processor — size-0 rescue gate, not a destroy/grade gate

```
:18493  lVar11 = (**(code **)(*plVar10 + 0x18))(plVar10);            // GetSize
:18494  if (((lVar11 == 0) &&
:18495     (uVar6 = FUN_180041940((longlong)param_4), (int)... == 0)) &&   // record+0x16 DIR bit clear
:18496     (bVar5 = (**(code **)(*plVar10 + 0x68))(plVar10), (bVar5 & 1) != 0)) {  // deleted
:18498    FUN_18001e6c0(param_1 + 0x3b, param_4, plVar10, uVar12, plVar9);  // RESCUE chain
:18499    *(undefined2 *)(lVar1 + 0x2a) = 2;                          // item "rescued"
```

`FUN_18001e6c0` (:24022-24168): resident-content rescue — MFT-copy sector-list walk
(`FUN_18001e120 :23722`, requires `FUN_180041820(buf) == rec#` :23769, resident $DATA
extraction `FUN_18001dd80 :23575`, ≤9 fragments :23776), same-name merge stealing
runlist+size (`FUN_18001d690 :23215`, `:23340/:23348`, compressed 0x800 skipped :23239),
runlist adoption under a cluster cap (`FUN_18001d960 :23391`, gate `:23456
if (0 < *(struct+0x70) && total_clusters < *(struct+0x70)) { adopt; props+0x72 |= 4; }`),
re-emit with code 0xfe0001 (`FUN_180016440 :17715-17785`). **No claim-map probe, no grade,
no destroy anywhere in the chain** (props+0x72 bits 1/2 set at :24160-61).

## 2. Pass-1 unconditional emit

```
:18630  (**(code **)(*param_1 + 0x10))(param_1, param_4, plVar10, param_2,
             *(undefined8 *)(lVar1 + 0x20), *param_13);
```

Loop gates before it: fixup `FUN_180041bb0 != 0` (:18954), base-record `FUN_180041970 == 0`
(:18955), metafile gate (:18952-53). Deleted marking only: `:18562-66` item code 1.

## 3. Pass-2 (GUID-dump) collector and loop

```
:18584-18625  deleted?? live GUID-named files collected — name len 0x4C, '{' head,
              memcmp template "{3808876b-c176-4e48-b7ae-04046e6cc752}" (:18607-09)
              → engine[0x65] list          [S2-G1: GUID files are LIVE (state bit0 clear)]
:17983  engine[0x6d] = engine[0x4a];        // claim map copy for pass 2
:17984  FUN_18001a980(param_1 + 0x328);     // walk the GUID list
:21024  FUN_18001acc0(controller, buffer, chunk_len, sector_off) per ≤0x8000-sector chunk
```

Inside `FUN_18001acc0` (record parse from dump buffers):

```
:21152  'FILE' magic gate;  :21153 fixup;  :21155 base-record
:21186  filter: (flags & 4) || !name || name[0] == '$'  → skip
:21208  if (0x3ff < size) { ...viability... } else → falls into destroy label
:21220-21229  per RUNLIST ENTRY:  if (*entry != -1) { total++;
                 sector = spc(ctrl+0x28) * lcn;  if (FUN_18002c2e0(map, sector)) intact++; }
               // ONE probe at run START; map = pass-1 claimed extents (engine[0x4a])
:21249  if ((0 < iVar21) && (iVar20 == iVar21)) goto LAB_18001b06f;   // destroy
:21257  LAB_18001b06f:  (**(code **)(*plVar10 + 0x138))(plVar10,1);    // props Release
:21264  else → emit path LAB_18001afe6 (list insert, state |= 1)
```

Claim-map construction (pass 1): `FUN_180017190 :18323+`, bits set by `FUN_18002c1e0 :18409`;
claim condition `:18525` (live/dir always; deleted-with-size except the rescued subset);
`FUN_180034b20` metafile gate = 15-entry name table + SI hidden+system (`FUN_180044f60
(flags&4)&&(flags&2)`); props-namespace $FILE_NAME flags 0x10000000 → 0x10 DIRECTORY
(`FUN_180043700 :53481`; set at :5329-30).

## 4. NTFSLib boot + record-size ladder (M-001 revision)

`ghidra-mo/NTFSLib_decompiled.c` `CNtfsVolume::LoadNtfsVolume :19292-19344`:
OEM 'NTFS ' (`*(u32*)(b+3) == 0x5346544e`); bps multiple-of-512 only (`& 0x1ff == 0`);
`spc×$MFT_LCN < total_sectors`; cpr `i8@+0x40`: `neg → (2^(-cpr))/bps`, `pos → cpr×spc`,
**`== 0` or 0-result → "It is not a NTFS volume!" REJECT**; index-block `u8@+0x44` same
pattern, 0 → reject; **no pow2 checks, no 0x400 clamp, no 0xAA55 check**.

## 5. $MFTMirr capacity (N-001)

`NTFSLib_decompiled.c`:
```
:17407-13  this+0x70 = ((last_run.off + last_run.len) × spc-factor) / record-factor   // computed
:17460     if ((-1 < param_1) && (param_1 <= *(longlong *)(this + 0x70)))             // inclusive gate
:17185     GetFileRecordCount → this+0x70
:17379      "$MFTMirror::$DATA is resident!" hard abort
```

## 6. Stride quirk + error codes (N-011c/e)

Pass-2 stride: 0x400 after successful parse / 512 after hole (:21158/:21258/LAB_18001b0cb).
Error facility codes: 0x2980005/0x2a30006/0x2a90006 (:18908-27), 0x3a90001 OOM (:18966),
0x17c0004 (:17897). Names: props name = char* UTF-8 via `WideCharToMultiByte(CP 0xFDE9)` with
ACP+`'_'` fallback (`FUN_180006610 :4860-4906`). No namespace keep/skip logic (core ranking is
superset). SI-times override FN when valid (:5344-51) — core matches (`cr_mftwalk.cpp:734`).

## 7. Claim-population gate constants (fix-session addendum, 2026-10-26 — DF4-S012)

First-hand decode of the pass-1 claim condition (`FUN_180017390 :18501-18549`), fed by a
fresh binary extraction:

```
:18501  uVar12 = FUN_180034b20(param_4,*param_5);      // != 0 iff $SI dos_flags
                                                        // carries BOTH 0x04|0x02
                                                        // (FUN_180040ea0 :51960-75:
                                                        //  FUN_180041b70(ctx,0x10,0) =
                                                        //  $STANDARD_INFORMATION;
                                                        //  flags&4 && flags&2 -> 1)
:18503  bVar19 = (int)uVar12 == 0;                     // TRUE for regular files
:18505-27  6-byte compare of name[0..5] vs DAT_18004cd70, with the $LogFile
           exception: engine+0x381 == 0 && strcmp(name,"$LogFile")==0 -> bVar3 = false
:18525  if (((char)param_1[0x70] != '\0') || ((!bVar19 && (bVar3)))) && (bVar4)
             -> FUN_180017190(engine, map, props, engine+0x258)
```

- **`DAT_18004cd70` = `"$Boot"`** (5 chars + NUL = the 6 compared bytes) — extracted from
  `NTFSSearchDLL.dll` `.rdata` (paddr 0x4b570, VA 0x18004cd70; rabin2 strings dump line 16).
- **Pointer-arithmetic map:** `param_1[0x70]` (longlong\*) = engine byte **+0x380**;
  `*(char*)(param_1 + 0x4b)` = engine byte **+0x258**; `*(char*)(param_1 + 0x381)` =
  engine byte **+0x381** (adjacent to +0x380 — a flag pair: claim-all / claim-$LogFile).
- **Observation:** engine bytes +0x380/+0x381 are **never written anywhere in the DLL's
  decompile** (only the two read sites above) — they are zero-initialized unless the host
  (DRWUI/loader) sets them. With +0x380 = 0 the textual gate reads "only hidden+system
  metafile records claim" (and pass 1 filters those at the walk gate, so the map would stay
  empty and pass-2 viability would never destroy anything but `size <= 0x3ff` records,
  making FUN_1800161f0's claim probe dead code) — the landed fix therefore implements the
  audit's three-agent-verified semantics (live + deleted-with-size + dir extents claim;
  $LogFile never) and registers this ambiguity here for the OQ-S2M-1 engine-config decode
  (add +0x380/+0x381 to its +0x258/+0x259 probe list).
- **Record-key decode (pass-2 identity):** `FUN_18001acc0 :21186-87` —
  `lVar18 = local_c0 / record_size + param_4` where param_4 = the chunk's absolute start
  sector (FUN_18001a980 :21016: `spc * lcn + base`): the pass-2 claim key is the
  **dump position**, a key space disjoint from pass-1's MFT-position keys — first-hand
  confirmation that the finding's "dedup by record#" plan text was a misreading while its
  test design (reused-slot records recovered) is correct.
