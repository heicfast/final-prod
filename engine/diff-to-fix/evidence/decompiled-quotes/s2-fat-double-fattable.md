# s2-fat-double-fattable — CDoubleFATTable: vtables (RTTI-extracted), mirror consult, construction fallback

**Session 2 · corpus `03ad46c` · `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` + binary RTTI extraction** (`evidence/executions/` probe scripts `probe_fat_vtables*.py`, `disasm_fat_slots.py` in the s2-ldm-real-guid-proof directory pattern).

## 1. Vtable layout (MSVC RTTI: class string → TypeDescriptor → COL → vftable)

```
IFATTable        vftable 0x53968: [dtor 41cd0, 41cc0, pure, pure, pure]
CDoubleFATTable  vftable 0x53998: [dtor 41d00, 41d80, 41de0, 41da0, 41dc0]
CFAT12Table      vftable 0x53908: [41670, 416f0, 416d0]
CFAT32Table      vftable 0x53928: [41760, 419e0, 417c0]
CFAT16Table      vftable 0x53948: [41aa0, 41c80, 41b00]
IRealFATTable    vftable 0x539c8: [dtor 41eb0, pure, pure]
```

Slot semantics (decoded this session):
- child slot 1 = **GetNext(cluster)** — canonicalized chain-next per flavor.
- CDouble slot 1 = 41d80 = `return (child1 != 0 || child2 != 0)` (IsValid).
- CDouble slot 2 = 41de0 = **merged GetNext with mirror consult** (below).
- CDouble slots 3/4 = 41da0/41dc0 = delegate-to-primary-only / delegate-to-mirror-only
  (disassembled directly — the decompiler skipped both thunks):

```
0x00041da0: 48 8b 49 08    mov 0x8(%rcx),%rcx      ; child1 (primary)
            48 85 c9       test %rcx,%rcx
            74 07          je  ret0
            48 8b 01       mov (%rcx),%rax
            48 ff 60 08    rex.W jmp *0x8(%rax)   ; child1->vtbl[1](args)  — primary-only
0x00041dc0: 48 8b 49 10    mov 0x10(%rcx),%rcx     ; child2 (mirror)
            ...            rex.W jmp *0x8(%rax)   ; child2->vtbl[1](args)  — mirror-only
```

## 2. Child GetNext — canonicalization per flavor (child slot 1)

CFAT12Table::GetNext = FUN_1800416f0 (:53783-53808):

```c
  if (buffer != 0) {
    uVar3 = (uint)(cluster * 3) >> 1;                    /* FAT12 byte packing */
    if (uVar3 + 1 <= bytes) {
      uVar1 = *(ushort *)(uVar3 + buffer);
      uVar2 = uVar1 & 0xfff;
      if ((-(char)cluster & 1U) == 1) uVar2 = uVar1 >> 4;  /* odd cluster: high 12 bits */
      if (uVar2 == 0xfff) return 0xffffffff;              /* EOC -> canonical 0xFFFFFFFF */
      if (0xff7 < uVar2) return 0xffffff8;                /* 0xFF8..0xFFE -> canonical EOC-min */
      return (ulonglong)uVar2;                            /* BAD 0xFF7 returns RAW; valid raw */
    }
  }
  return 0;
```

CFAT32Table::GetNext = FUN_1800419e0 (:53955-53962) — raw u32 read, no canonicalization:

```c
  if ((buffer != 0) && (cluster < entry_count))
    return *(undefined4 *)(buffer + (ulonglong)cluster * 4);
  return 0;
```

## 3. CDoubleFATTable::GetNext — the merged read with mirror consult (slot 2 = FUN_180041de0, :54207-54251)

```c
uint FUN_180041de0(longlong this, uint cluster)
{
  uVar2 = 0;
  if ((child1 != 0) &&
     (uVar2 = child1->vtbl[1](child1, cluster), uVar2 != 0)) {
    return uVar2;                          /* (1) PRIMARY non-zero result — INCLUDING BAD —
                                              served from primary; mirror NOT consulted */
  }
  plVar1 = child2;
  if (plVar1 == 0) return uVar2;           /* (2) no mirror: primary's 0 */
  uVar2 = child2->vtbl[1](child2, cluster); /* (3) primary said 0 (FREE): consult MIRROR */
  if (child1 != 0) {                        /*     primary exists (it returned 0) */
    if (uVar2 < cluster) {                  /*     mirror value points backward / small */
      iVar3 = child2->vtbl[1](child2, cluster + 1);
      if (uVar2 + 1 == iVar3) return 0;     /*     mirror has contiguous run -> free */
      iVar3 = child2->vtbl[1](child2, cluster);
      uVar4 = child2->vtbl[1](child2, cluster - 1);
      if (cluster != 0) {
        if (uVar4 == cluster) return 0;     /*     cluster is continuation of cluster-1 -> free */
        if (iVar3 == cluster + 1) return cluster;  /* mirror says contiguous next */
      }
    }
    else if (((1 < uVar2 + 0xf0000009) && (uVar2 != 0xffffffff)) &&
             (uVar2 != 0xfffffff)) {
      return uVar2;                          /*     dirty-EOC family (>=0x0FFFFFF9, excl. 0x0FFFFFFF
                                                 and 0xFFFFFFFF) -> serve mirror */
    }
    return 0;
  }
  return uVar2;                              /* (4) NO primary table: serve mirror directly */
}
```

## 4. Construction + load-time location fallback (:3805-3949, inside CFatCompleteSearch::FUN_180005b70)

```c
plVar6 = FUN_180036e90(reader, fat_type, fat_start, fat_size, param_3);   /* primary */
plVar7 = FUN_180036e90(reader, fat_type, fat_start + fat_size, fat_size, param_3); /* mirror */
puVar8 = new CDoubleFATTable(plVar6, plVar7);
if (FUN_180041d80(puVar8) == 0) {            /* invalid: both children null */
  if ((volume_flags & 0x10) != 0) {          /* gate flag */
    /* walk the internal LBA-keyed map (param_1 + 0x160) for the greatest
       entry <= fat region start; take its LBA as an ALTERNATIVE FAT location,
       rebuild the double table from there (twice — two fallback tiers),
       log the chosen location, release the first table, use the rebuilt one */
  }
}
```

FUN_180036e90 (:44687-44781): constructs CFAT12/16/32Table by flavor, **caches the
whole FAT in memory** (`operator_new(sector_size * fat_size)` + one bulk read), with
per-flavor cluster-count sanity windows (FAT16: `0xC/(ss>>9) <= count <= 0x100/(ss>>9)`;
FAT32: `0x100/(ss>>9) <= count <= 0x96000/(ss>>9)` — outside the window the table object
is returned with a NULL buffer, i.e. invalid).

## 5. Interpretation (OQ-T2 resolution)

- **The mirror consult trigger is per-entry and decoded**: the og consults FAT2 only
  when the primary's canonicalized GetNext(cluster) returns **0** (FREE / dead).
  Non-zero primary results — **including BAD** — are served from the primary with no
  mirror consult.
- When the mirror IS consulted, its answer is **gated** (EOC-family accept-set
  `>=0x0FFFFFF9`, excluding 0x0FFFFFFF/0xFFFFFFFF, plus three mirror self-consistency
  cross-checks) rather than served raw; with NO primary table at all the mirror is
  served directly.
- Load-time fallback: an invalid declared FAT location triggers a search of an
  internal LBA-keyed structure map for an alternative FAT image (the quick-format
  remnant map), gated by volume-flag 0x10.

Residual (exact intent of the three cross-checks and the `return cluster` branch —
deleted-chain reconstruction gating for quick-format volumes) is quoted verbatim above;
the caller context (CFatCompleteSearch chain walk) is the next decode target if needed.
