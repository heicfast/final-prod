# evidence/decompiled-quotes/exfat-dentry-validation.md

Exact snippets quoted during the FAT/exFAT audit wave (T6). Source repository:
`easeus-drw-research` @ `03ad46c`.

## Q1 — exFAT stream-dentry structural validation (polarity settled by main-auditor read)

File: `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c`
Lines: 1846–1875 (read directly by the main auditor this session; agent-B quote concurs)

```c
in_RAX = (ulonglong)(bVar1 - 3);
if ((((bVar1 - 3 & 0x7f) != 0) || (param_1[0x20] != -0x7f)) || (param_1[0x40] != -0x7e)) {
  if ((bVar1 - 5 & 0x7f) == 0) {
    in_RAX = FUN_180002f90(param_1 + 1);
  }
  else {
    in_RAX = (ulonglong)(bVar1 - 0x40);
    if ((bVar1 - 0x40 & 0x7f) == 0) {
      if (((param_1[2] == '\0') && ((param_1[6] == '\0' || (param_1[7] == '\0')))) &&
         ((param_1[0x10] == '\0' ||
          (((param_1[0x11] == '\0' || (param_1[0x12] == '\0')) || (param_1[0x13] == '\0')))))) {
        uVar2 = *(uint *)(param_1 + 0x14);
        in_RAX = (ulonglong)uVar2;
        if ((uVar2 != 1) && (uVar2 < 0xfffffff7)) goto LAB_180003119;
      }
      goto LAB_180003106;
    }
    if ((bVar1 + 0x3f & 0x7f) != 0) goto LAB_180003106;
    in_RAX = FUN_180003030((byte *)(param_1 + 1));
  }
    if ((char)in_RAX == '\0') {
LAB_180003106:
      return in_RAX & 0xffffffffffffff00;    // FALSE (invalid)
    }
  }
LAB_180003119:
  return CONCAT71((int7)(in_RAX >> 8),1);    // TRUE (valid)
```

**Reading (settles the artifact-vs-core polarity conflict):** the stream-extension dentry is
VALID iff reserved fields are zero AND `first_cluster != 1` AND `first_cluster < 0xFFFFFFF7`.
`goto LAB_180003119` (the TRUE return) is reached only in that case; `LAB_180003106` (FALSE)
is reached by the reserved-nonzero branch, by the first-cluster gate failure
(`uVar2 == 1 || uVar2 >= 0xFFFFFFF7`), and by unknown type classes.

**Consequence:** the artifact `fat-exfat-engine-algorithms-v2.md` §3 line "u32@+0x14 must be
1 or ≥ 0xFFFFFF7" has the polarity **inverted** — the og REJECTS those values. The core's
`fc_invalid = first_cluster == 1 || first_cluster >= 0xFFFFFFF7u`
(`src/core/exfat/cr_exfat_quick.cpp:343`) is **correct**. Corpus erratum candidate #6
(feed back to corpus owners).

## Q2 — FAT12 Next (BAD 0xFF7 returned raw)

File: `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`
Lines: ~53790–53807 (agent-B verified)

```
off = (cluster*3)>>1; v = u16[base+off]; v = (cluster odd) ? v>>4 : v&0xFFF;
if v == 0xFFF → return 0xFFFFFFFF; if 0xFF7 < v → return 0xFFFFFF8; return v;
```

Cross-cite: `re-workspace/artifacts/CORRECTIONS.md` row 3. Used by: I-002.

## Q3 — FAT32 tolerant EOC family

File: `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`
Lines: ~8030–8048 (agent-B verified)

```
entry == 0xFFFFFFF8                                        → EOC
(entry & 0x0FFFFFFF) == 0xFFFFFF8 && (next & 0xFFFFFFF8) == 0xFFFFFFF8 → EOC
(next & 0x07FFFFF8) == 0x07FFFFF8                          → EOC (mask-24)
(entry & 0x0FFFFFFF) == 0xFFFFFF0 && (next & 0xFFFFFF0) == 0xFFFFFF0 → EOC (alternate)
```

Used by: N-002.

## Q4 — RawFixer exFAT boot checksum (undecoded predicate)

File: `re-workspace/reversed/ghidra-partition/RawFixer_decompiled.c`
Function: `FUN_18000e7e0`
Lines: ~9353–9381 (agent-B verified)

```c
local_38 = 0;                       // u32 checksum
local_34 = param_2 * 0xb;           // sector_size × 11 sectors
for (i = 0; i < local_34; i++) {
    …predicate via exception machinery (FUN_180023220/180001700/180023230)…
    if (local_28 == lVar2)
        local_38 = local_38 << 0x1f | (local_38 >> 1) + (uint)*(byte *)(param_1 + i);
}
```

Used by: OQ-T5.
