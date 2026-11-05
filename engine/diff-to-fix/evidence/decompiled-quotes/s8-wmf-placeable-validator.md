# Evidence — S-040 WMF placeable validator (both decompilers + core, first-hand)

Session 8 (2026-10-28). Corpus `be0cfe9`; core `35adcb9`. Both decompile sites re-read
directly this session; the core source re-read directly. No intermediate agent claim
was relied on for the load-bearing gates.

## 1. Ghidra — `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:29820-29843`

```c
  local_28 = 0x9ac6cdd7;
  local_24 = 0;
  local_22 = 0;
  if (*(int *)param_2 == 0x90001) {
    if (param_2[8] == 0) {
      *(ulonglong *)(param_3 + 0x10) = (ulonglong)(uint)(*(int *)(param_2 + 3) * 2);
      *(undefined4 *)(param_3 + 0x18) = 0x46;
      __security_check_cookie(local_18 ^ (ulonglong)&local_28);
      return;
    }
  }
  else if ((*(int *)param_2 == -0x65393229) && (param_2[2] == 0)) {
    if (((param_2[10] ==
          (ushort)(param_2[9] ^ param_2[7] ^ param_2[6] ^ param_2[5] ^ param_2[4] ^ param_2[3] ^
                   param_2[1] ^ *param_2 ^ param_2[2] ^ param_2[8])) &&
        (((param_2[2] == 0 && (*(int *)(param_2 + 8) == 0)) && (*(int *)(param_2 + 0xb) == 0x90001))
        )) && (param_2[0x13] == 0)) {
      *(ulonglong *)(param_3 + 0x10) = (ulonglong)(*(int *)(param_2 + 0xe) * 2 + 0x16);
      *(undefined4 *)(param_3 + 0x18) = 0x46;
      __security_check_cookie(local_18 ^ (ulonglong)&local_28);
      return;
    }
  }
```

`param_2` is `ushort*`; `-0x65393229` as unsigned = `0x9AC6CDD7` (bytes D7 CD C6 9A).
Index→byte-offset mapping: `param_2[10]`=+20 (stored checksum), `param_2[9]`=+18,
`param_2[8]`=+16, `*(int*)(param_2+8)`=dword@+16, `*(int*)(param_2+0xb)`=dword@+22,
`param_2[0x13]`=+38, `*(int*)(param_2+0xe)`=dword@+28.

Placeable-branch gates: key dword@+0; word@+4==0; dword@+16==0; stored word@+20 ==
XOR(words 0..9); dword@+22==0x00090001; word@+38==0; extent = 2×dword@+28 + 22; type 0x46.
The standard branch (first `if`): dword@+0==0x90001, word@+16==0, extent 2×dword@+6 —
this branch MATCHES the core's `validate_wmf_std`.

## 2. retdec — `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c:38495-38515`

```c
    int16_t v4 = *(int16_t *)(a2 + 18); // 0x180026561
    int16_t v5 = *(int16_t *)(a2 + 14); // 0x180026565
    int16_t v6 = *(int16_t *)(a2 + 12); // 0x180026569
    int16_t v7 = *(int16_t *)(a2 + 10); // 0x18002656d
    int16_t v8 = *(int16_t *)(a2 + 8);  // 0x180026571
    int16_t v9 = *(int16_t *)(a2 + 6);  // 0x180026575
    int16_t v10 = *(int16_t *)(a2 + 2); // 0x180026579
    int64_t v11 = a2 + 16;              // 0x180026583
    int16_t v12 = *(int16_t *)v11;      // 0x180026583
    int16_t v13 = *(int16_t *)(a2 + 20); // 0x180026587
    if (v13 != (v3 ^ (int16_t)a2 ^ v4 ^ v5 ^ v6 ^ v7 ^ v8 ^ v9 ^ v10 ^ v12) || *(int32_t *)v11 != 0 || *(int32_t *)(a2 + 22) != 0x90001 || *(int16_t *)(a2 + 38) != v3) {
        return function_18005ef20(v1);
    }
    // 0x1800265a7
    *(int64_t *)(a3 + 16) = (int64_t)(2 * *(int32_t *)(a2 + 28) + 22);
    *(int32_t *)(a3 + 24) = 70;
    return function_18005ef20(v1);
```

`v13` = word@+20 (stored checksum); XOR operands are the words at byte offsets
0,2,6,8,10,12,14,16,18 plus `v3` (word@+4, gated ==0 in the caller condition) —
i.e. words 0..9 exactly, agreeing with ghidra term-for-term. `*(int32_t*)v11 != 0`
= dword@+16 must be zero. Extent `2 * *(int32_t *)(a2 + 28) + 22`, type 70 (0x46).

## 3. Core — `src/core/carve/cr_analysers.cpp:3379-3408` at `35adcb9`

```cpp
    const u16 inch = u16_from_bytes_le(h + 16);
    if (inch == 0 || inch > 14400)
        return r;
    /* the placeable checksum: the stored word at +18 is the XOR of the
     * first 9 words (computed with the checksum field zero) - so the
     * verification compares XOR(words 0..8) against the stored word
     * (equivalently XOR of all 10 words == 0) */
    u16 x = 0;
    for (int i = 0; i < 9; ++i)
        x = u16(x ^ u16_from_bytes_le(h + 2 * i));
    const u16 stored = u16_from_bytes_le(h + 18);
    if (x != stored)
        return r; /* placeable-header checksum */
```

(plus the key memcmp `D7 CD C6 9A`, the std-header bytes `01 00 09 00` at +22 — both
matching the og — and the bounded `64 << 20` extent replacing the og's exact extent.)

## 4. The contradiction table

| Gate | og (both decompilers) | core @35adcb9 | Compatible? |
|---|---|---|---|
| Key dword@+0 | 0x9AC6CDD7 | D7 CD C6 9A memcmp | yes |
| word@+4 (hmf) | == 0 | not checked | (core weaker) |
| dword@+16 | == 0 | u16@+16 ∈ [1,14400] | **NO — mutually exclusive** |
| stored checksum | word@+20 == XOR(words 0..9) | word@+18 == XOR(words 0..8) | **NO** |
| std header | dword@+22 == 0x00090001 | bytes 01 00 09 00 @+22 | yes |
| word@+38 (NumParams) | == 0 | not checked | (core weaker) |
| extent | 2×dword@+28 + 22 (exact) | min(64 MiB, probe_limit) | **NO** (heuristic class) |
| type id | 0x46 | FMT id (append-only) | n/a |

## 5. The refuted prior claim

`findings/FIXED/S-007-carve-missing-common-formats.md:222-223` (batch-1 addendum):
"WMF(placeable — the stored checksum is the XOR of words 0..8, the +18 word included
in neither side)" — and the `e018bf4` commit message: "WMF placeable (D7 CD C6 9A,
XOR-checksum VERIFIED - the stored word is the XOR of words 0..8)". Both decompilers
refute this: the og's stored word is at +20 and the XOR covers words 0..9 (the +18
word IS included in the og's XOR). The "VERIFIED" was performed against a misread.
