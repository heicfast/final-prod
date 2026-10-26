# s2-exfat-checksumdbr — ExFATFixer::ChecksumDBR skip-set decoded (OQ-T5)

**Session 2 · corpus `03ad46c` · `re-workspace/reversed/ghidra-partition/RawFixer_decompiled.c` + binary extraction**

## The loop (FUN_18000e7e0, :9353-9381 — decompiled)

```c
void FUN_18000e7e0(longlong param_1,int param_2)   /* ExFATFixer::ChecksumDBR(dbr, sector_size) */
{
  local_38 = 0;                                     /* checksum */
  local_34 = param_2 * 0xb;                         /* sector_size * 11 bytes */
  for (local_20[0] = 0; local_20[0] < local_34; local_20[0] = local_20[0] + 1) {
    local_30 = FUN_180023220(0x180079a40);          /* = &DAT_180079a40 + 0x10 (END)  */
    uVar1 = FUN_180001700(&DAT_180079a40);          /* = &DAT_180079a40      (BEGIN) */
    local_28 = FUN_180023230(uVar1,local_30,(int *)local_20);  /* std::find(BEGIN, END, &i) */
    lVar2 = FUN_180023220(0x180079a40);             /* END again */
    if (local_28 == lVar2) {                        /* not found -> include byte i */
      local_38 = local_38 << 0x1f |
                 (local_38 >> 1) + (uint)*(byte *)(param_1 + (ulonglong)local_20[0]);
    }
  }
}
```

The rolling checksum is the spec rotation (`c = (c >> 1 | c << 31) + byte`); the per-byte
predicate is `std::find` over a global int array at `0x180079a40` (FUN_180023230 →
FUN_180033970 → **FUN_18003ab40** at :41083-41092, verbatim):

```c
int * FUN_18003ab40(int *param_1,int *param_2,int *param_3)
{
  for (local_res8 = param_1; (local_res8 != param_2 && (*local_res8 != *param_3));
      local_res8 = local_res8 + 1) {
  }
  return local_res8;                                 /* std::find(begin, end, value) */
}
```

## The skip-set array (extracted from RawFixer.dll RVA 0x180079a40, 4 ints)

```
[0] = 106 (0x6a)   — VolumeFlags low  (spec skip)
[1] = 107 (0x6b)   — VolumeFlags high (spec skip)
[2] = 112 (0x70)   — PercentInUse     (spec skip)
[3] = -1  (0xffffffff) — terminator; no loop index (0 .. sector_size*11-1) can match it
```

## Resolution

**The og's ChecksumDBR skips exactly the exFAT spec skip-set {106, 107, 112}** — the
fourth array element (-1) is a terminator sentinel, never matched by any loop index.
The core's implementation (`src/core/exfat/cr_exfat_dentry.cpp:26-36`, spec skip-set,
shared with the repair engine) **matches the og exactly. OQ-T5 → MATCH, resolved.**

Corpus-erratum confirmation: `PartitionRecovery/rawfixer-v2--method-inventory.md` §4's
"bytes 11..209 + 510" description of the coverage is wrong in a second way (it was
already contradicted by the verified loop bound `sector_size * 0xB`); the coverage is
`sector_size * 11` bytes with skip-set {106, 107, 112} — feed back as corpus erratum.
