# Evidence — M-113: the og's partition-boundary read semantics (clamp + zero-fill + success)

Session-12 continuation lane, 2026-10-29. Corpus `be0cfe9`, core `c7fa527c`.
Every quote below was read first-hand this session.

## 1. The og's read wrapper — DRWUI `FUN_1400d0510`
(`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:183633-183666`)

The `__RTDynamicCast` against `CBasicPartitionDevice::RTTI_Type_Descriptor`
inside the function proves the class (the wrapper reads through a child device
at `this+0x28`, base offset at `this+0x30`):

```c
undefined4 FUN_1400d0510(longlong param_1,longlong param_2,longlong param_3,longlong param_4)
{
  ...
  if ((param_2 != 0) && (*(longlong **)(param_1 + 0x28) != (longlong *)0x0)) {
    lVar3 = (**(code **)(**(longlong **)(param_1 + 0x28) + 0x18))();   /* sector count */
    uVar5 = 0;
    lVar4 = __RTDynamicCast(*(undefined8 *)(param_1 + 0x28),0,
                            &IStorageDevice::RTTI_Type_Descriptor,
                            &CBasicPartitionDevice::RTTI_Type_Descriptor,0);
    if ((lVar4 != 0) && (*(longlong **)(lVar4 + 0x28) != (longlong *)0x0)) {
      lVar3 = (**(code **)(**(longlong **)(lVar4 + 0x28) + 0x18))();   /* the partition's own count */
    }
    if ((((-1 < param_3) || (-1 < *(longlong *)(param_1 + 0x30) + param_3)) &&
        (param_3 < lVar3 + -1)) && (param_4 <= lVar3 + -1)) {
      lVar4 = param_4;
      if (lVar3 < param_3 + param_4) {
        lVar4 = lVar3 - param_3;              /* CLAMP: in-bounds prefix only */
      }
      uVar1 = (**(code **)(**(longlong **)(param_1 + 0x28) + 8))
                (*(longlong **)(param_1 + 0x28),param_2,
                 *(longlong *)(param_1 + 0x30) + param_3,lVar4,uVar5 & 0xffffff00);
      _Size = param_4 - lVar4;                /* the unread tail (sectors)    */
      *(undefined4 *)(param_1 + 8) = *(undefined4 *)(*(longlong *)(param_1 + 0x28) + 8);
      if ((longlong)_Size < 1) {
        return uVar1;                         /* fully in-bounds: done        */
      }
      FUN_140162e00("i64CountofSectors - i64RealReadSector = %I64d",_Size);  /* diagnostic */
      iVar2 = (**(code **)(**(longlong **)(param_1 + 0x28) + 0x28))();       /* sector size */
      memset((void *)(iVar2 * lVar4 + param_2),0,_Size);  /* ZERO-FILL the tail */
      return uVar1;                           /* SUCCESS over clamp+zeros     */
    }
  }
  return 0;
}
```

Semantics: a read whose `(offset, count)` straddles the partition end returns
the in-bounds prefix from the parent device, ZEROES the remainder of the
caller's buffer, and reports success. The diagnostic line is what the real-run
log captured.

## 2. The real-run observation — `og-codebase/log/DRWUI.log:638`

```
01:21:44:469 i64CountofSectors - i64RealReadSector = 10175
```

Fired during the deep-scan lost-partition probing (immediately before
"add lost partition from filesystem start=2048..."): the og's probe issued a
past-end read, received the clamped+zero-filled result, and CONTINUED — the
partition was subsequently found. This is the product-level behavior, observed
in the field log, not only in the decompile.

(The string exists only in DRWUI's string table across the whole reversed
corpus — `grep -rln i64RealReadSector re-workspace/reversed/` returns
`DRWUI.exe/strings_ascii.txt` and `ghidra-drwui/DRWUI_decompiled.c` — so the
decoded wrapper is the UI-side device; the engine-side CPartitionVirtualVolumeDevice
read slot is M-113's named fix-time probe.)

## 3. The core side at `c7fa527c`

`src/core/storage/cr_slice.cpp:46-49` — per-read hard error:

```cpp
u32 PartitionSlice::read_at(u64 offset, void* buffer, u32 length) {
    if (length == 0)
        return OK;
    if (offset > length_ || u64(length) > length_ - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;
    return parent_->read_at(start_ + offset, buffer, length);
}
```

`src/core/storage/cr_slice.cpp:25-28` — creation refuses the whole slice on
length overrun:

```cpp
if (start_byte + length_bytes > parent->size()) {
    if (err)
        *err = ERR_RANGE_OUT_OF_BOUNDS;
    return nullptr;
}
```

The raw-length callers (the refusal surfaces):

```cpp
// src/app/rrecov.cpp:2925-2931 — the CLI partition open (table's raw sector_count)
part_slice = cr::PartitionSlice::create(
    view.get(), part_base, u64(p.sector_count) * view->geometry().logical_sector, &prc);
if (!part_slice) {
    wprintf(L"cannot slice partition %d (rc=%08x)\n", part, prc);
    return 2;                       /* the whole scan aborts */
}

// src/ui/cr_ui.cpp:1349-1352 and :1433-1436 — the ABI volume opens, same shape
auto slice = PartitionSlice::create(whole.get(), start, len, nullptr);
if (!slice)
    return ret_(ERR_OPEN_FAILED, "slice creation failed for %c: (@%llu +%llu)", ...);
```

The internal inconsistency — the discovery path ALREADY clamps ragged tails
(`src/core/lost/cr_candidates.cpp:493-501`):

```cpp
std::unique_ptr<PartitionSlice> materialize(IStorageDevice* disk, const Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    u64 bytes = c.sector_count * ls;
    const u64 max = disk->size() - c.start_lba * ls;
    if (bytes > max)
        bytes = max; /* clamp ragged tails to the device */
    ...
}
```

## 4. The engine-side counterpart context (the fix-time probe's starting point)

- RTTI census (`re-workspace/rtti_classes.json`):
  `CPartitionVirtualVolumeDevice` present in **DRWUI.exe** and
  **HFSPlusSearchDLL.dll**; `CBasicPartitionDevice` in DRWUI.exe.
- The engine's usage: `HFSPlusSearchDLL_decompiled.c:454-461` — the controller
  reads the partition context, then `FUN_18000af90(device, 0, totalSectors, ...)`
  builds the volume reader the tree consumes; the device's per-read policy
  resolves through its vtable (the slot walk is the probe).
- The core's `PartitionSlice` cites this family directly
  (`cr_slice.cpp:1-6`: "Reference: CPartitionVirtualVolumeDevice ... The bound
  is enforced HERE").
