# s2-ldm-raid5-rotation — Raid5VolumeStorage read/write/reconstruct (verbatim quotes)

**Session 2 (T7 deep pass) · corpus `03ad46c`** — function-name mapping established via
the SEH crash-handler strings in `LdmManager_decompiled.c :88592-:88709` (each handler
returns the address just past its function body), cross-checked against
`LdmManager_functions.txt`:

| Crash-handler string | Handler returns | Function (by RVA range) | Decompile lines |
|---|---|---|---|
| "Raid5VolumeStorage::WriteParitityDisk",0x240 | 0x180085c14 | FUN_180085860 (958 B) | :68159-:68391 |
| "Raid5VolumeStorage::ComputeDataWithOtherDisk",0x1e6 | 0x180085ed9 | FUN_180085c30 (688 B) | :68393-:68451 |
| "Raid5VolumeStorage::Open",0x18a | DAT_180086513 | FUN_180085f30 (1513 B) | :68547-:69000 |
| "Raid5VolumeStorage::WriteSector",0x118 | DAT_180086a74 | FUN_180086530 (1354 B) | :68700-:68776 |
| "Raid5VolumeStorage::ReadSector",0xad | DAT_180086ff7 | FUN_180086a90 (1389 B) | :68777-:68876 |

## 1. ReadSector — FUN_180086a90 (:68777-68876), the four rotation branches

```c
      for (; local_b8 < param_3 + param_4; local_b8 = local_b8 + local_e8) {
        uVar5 = (local_b8 + *(uint *)(param_1 + 0x28c)) / (ulonglong)*(uint *)(param_1 + 0x28c) - 1;
        local_80 = (ulonglong)*(uint *)(param_1 + 0x28c);
        uVar8 = local_b8 % local_80;
        local_78 = (ulonglong)iVar3;
        uVar6 = uVar5 / local_78;
        local_d8 = 0;
        iVar2 = *(int *)(param_1 + 0x2a8);
        if (iVar2 == 0) {
          local_70 = (longlong)iVar3;
          local_68 = (longlong)iVar3;
          local_d8 = ((local_70 - uVar6 % (ulonglong)(longlong)iVar1) + 1 + uVar5 % local_68) %
                     (ulonglong)(longlong)iVar1;
        }
        else if (iVar2 == 1) {
          local_60 = (longlong)iVar3;
          local_58 = (ulonglong)iVar3;
          local_d8 = uVar5 % local_58;
          if (local_60 - uVar6 % (ulonglong)(longlong)iVar1 <= local_d8) {
            local_d8 = local_d8 + 1;
          }
        }
        else if (iVar2 == 2) {
          local_50 = (ulonglong)iVar1;
          local_48 = (ulonglong)iVar3;
          local_d8 = (uVar6 % local_50 + 1 + uVar5 % local_48) % (ulonglong)(longlong)iVar1;
        }
        else if (iVar2 == 3) {
          local_40 = (ulonglong)iVar1;
          local_38 = (ulonglong)iVar3;
          local_d8 = uVar5 % local_38;
          if (uVar6 % local_40 <= local_d8) {
            local_d8 = local_d8 + 1;
          }
        }
        local_98 = 0;
        local_88 = uVar6 * *(uint *)(param_1 + 0x28c) + uVar8;
        lVar7 = FUN_180085700(param_1 + -8,local_d8,local_88,&local_98);
        if ((lVar7 == 0) || (*(ulonglong *)(lVar7 + 0x18) <= local_98)) break;
        local_e8 = *(int *)(param_1 + 0x28c) - (int)uVar8;
        ...
        uVar4 = (**(code **)(*(longlong *)(*(longlong *)(lVar7 + 8) + 8) + 8))
                          (*(longlong *)(lVar7 + 8) + 8,local_90,local_30,local_e8);
        if (local_e8 != uVar4) {
          local_20 = (ulonglong)local_e8;
          local_28 = param_1 + -8;
          uVar8 = FUN_180085c30(local_28,local_90,local_d8,local_88,(ulonglong)local_e8);
          if (local_20 != uVar8) break;
        }
```

Legend: `param_1+0x28c` = stripe size S (bytes); `param_1+0x2ac` = column count N;
`param_1+0x2a8` = rotation selector; `uVar5` = chunk index k; `uVar6` = syndrome
s = k/(N-1); `uVar5 % iVar3` = f = k mod (N-1); `local_d8` = member column.

## 2. ComputeDataWithOtherDisk — FUN_180085c30 (:68393-68451), XOR reconstruct

```c
    for (local_30 = 0; local_30 < *(int *)(param_1 + 0x2b4); local_30 = local_30 + 1) {
      if (local_30 != param_3) {                       /* skip the failed column */
        local_20 = 0;
        lVar3 = FUN_180085700(param_1,(longlong)local_30,param_4,&local_20);
        if ((lVar3 == 0) || (*(ulonglong *)(lVar3 + 0x18) < local_20)) break;
        ...
        uVar2 = (**(code **)(...))(...,*(undefined8 *)(param_1 + 0x2a0),local_18, param_5 & 0xffffffff);
        if (param_5 != uVar2) break;
        if (bVar1) {
          bVar1 = false;
          memcpy(param_2,*(void **)(param_1 + 0x2a0),param_5 * *(uint *)(param_1 + 0x290));
        }
        else {
          for (local_34 = 0; (ulonglong)(longlong)local_34 < param_5 * *(uint *)(param_1 + 0x290);
              local_34 = local_34 + 1) {
            *(byte *)((longlong)param_2 + (longlong)local_34) =
                 *(byte *)((longlong)param_2 + (longlong)local_34) ^
                 *(byte *)(*(longlong *)(param_1 + 0x2a0) + (longlong)local_34);
          }
        }
      }
    }
    if (local_30 < *(int *)(param_1 + 0x2b4)) {
      param_5 = 0;                                     /* second missing member -> failure */
    }
```

## 3. The rotation selector — the only write in 91,475 lines (constructor, :68942)

```c
  *(undefined ***)param_1 = Raid5VolumeStorage::vftable;
  *(undefined ***)(param_1 + 8) = Raid5VolumeStorage::vftable;
  *(undefined4 *)(param_1 + 0x290) = 0;
  *(undefined4 *)(param_1 + 0x294) = 0;
  *(undefined8 *)(param_1 + 0x298) = 0;
  *(undefined8 *)(param_1 + 0x2a0) = 0;
  *(undefined8 *)(param_1 + 0x2a8) = 0;      /* rotation selector = 0 (left-symmetric) */
  *(undefined4 *)(param_1 + 0x2b0) = 0;
  *(undefined4 *)(param_1 + 0x2b4) = 0;
```

`grep -n "0x2a8) = \|0x2a8)=" LdmManager_decompiled.c` → single hit (line 68942, above).
No VBLK decode path, setter, or config load writes `+0x2a8`; the R=1/2/3 branches are
unreachable dead code in this build.
