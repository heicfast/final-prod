# S8 evidence — the og's WMF placeable grammar (first-hand read)

Read directly this session (session-8 wave 2) from
`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(corpus `be0cfe9`). Function `FUN_1800264d0` (:29807-29845), verbatim
(whitespace collapsed only):

```c
void FUN_1800264d0(undefined8 param_1,ushort *param_2,longlong param_3)
{
  ...
  if (*(int *)param_2 == 0x90001) {                    /* standard-WMF key */
    if (param_2[8] == 0) {                             /* params word == 0  */
      *(ulonglong *)(param_3 + 0x10) =
          (ulonglong)(uint)(*(int *)(param_2 + 3) * 2);/* size = Size-words*2 */
      *(undefined4 *)(param_3 + 0x18) = 0x46;          /* format code 0x46  */
      ...
    }
  }
  else if ((*(int *)param_2 == -0x65393229) && (param_2[2] == 0)) {
    /* key == 0x9AC6CDD7 (D7 CD C6 9A), hmf word[2] == 0 */
    if (((param_2[10] ==
          (ushort)(param_2[9] ^ param_2[7] ^ param_2[6] ^ param_2[5] ^
                   param_2[4] ^ param_2[3] ^ param_2[1] ^ *param_2 ^
                   param_2[2] ^ param_2[8])) &&
        (((param_2[2] == 0 && (*(int *)(param_2 + 8) == 0)) &&
          (*(int *)(param_2 + 0xb) == 0x90001))
        )) && (param_2[0x13] == 0)) {
      *(ulonglong *)(param_3 + 0x10) =
          (ulonglong)(*(int *)(param_2 + 0xe) * 2 + 0x16);
      *(undefined4 *)(param_3 + 0x18) = 0x46;
      ...
    }
  }
}
```

## Decode (word indices are `u16*` elements; byte offset = 2×index)

| Gate | Og expression | Meaning |
|---|---|---|
| Key | `*(int*)param_2 == 0x9AC6CDD7` | placeable magic at +0 |
| Hmf | `param_2[2] == 0` | word 2 (bytes 4-5) zero |
| Checksum | `param_2[10] == XOR(param_2[0..9])` | **stored word at bytes 20-21 equals XOR of the TEN preceding words (0..9)** |
| Reserved | `*(int*)(param_2 + 8) == 0` | dword at bytes 16-19 (words 8 AND 9) zero — 4 reserved bytes |
| Standard header | `*(int*)(param_2 + 0xb) == 0x90001` | `01 00 09 00` at byte +22 |
| Tail word | `param_2[0x13] == 0` | word 19 (bytes 38-39) zero |
| Size | `*(int*)(param_2 + 0xe) * 2 + 0x16` | standard-Size-words (dword at +28) × 2 **+ 22** |

## The core's grammar (cr_analysers.cpp:3383-3411, read this session)

| Gate | Core | Og |
|---|---|---|
| Key | memcmp D7 CD C6 9A @+0 | same |
| Hmf | not checked | word@+4 == 0 |
| Inch | `u16@h+16` must be 1..14400 | **no inch gate; bytes 16-19 must be 0** |
| Checksum | stored `u16@h+18` == XOR(words 0..8) | **stored u16@+20 == XOR(words 0..9)** |
| Reserved | not checked | dword@+16-19 == 0 |
| Standard header | `01 00 09 00` at +22 | same (+22) |
| Tail word | not checked | word@+38 == 0 |
| Size | `min(hit + 64 MiB, probe_limit)` | `std_size_words*2 + 22` (exact) |

The two grammars are **mutually exclusive**: a file that satisfies the og's
reserved-dword-zero gate has core-"inch" == 0 (core rejects); a file that
satisfies the core's inch gate (bytes 16-17 in 1..14400) violates the og's
reserved gate (og rejects). The checksum words are read from different
offsets with different XOR ranges, so no byte string satisfies both.

Both engines place the standard header at +22. (The canonical Aldus
placeable header is 20 bytes with checksum@18 == XOR(words 0..8) and the
standard header at +20 — i.e. the og's own grammar is non-canonical, and
the core's checksum formula is canonical but its inch/standard offsets are
not. Parity is against the og, not against the canonical spec: the og's
grammar is the reference behavior this audit scores.)

The S-007 batch-1 landing commit message claimed "WMF placeable … XOR-checksum
VERIFIED - the stored word is the XOR of words 0..8" citing the og — that
claim is refuted by the read above (the og XORs words 0..9 into word 10).
