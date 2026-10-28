# Evidence: the og's placeable-WMF gate vs our validator (filed as S-042 after the push-time reconciliation; the SA-9 lane's convergent decode)

**Corpus:** `easeus-drw-research` @ `be0cfe9`
**Source:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
**Function:** `FUN_1800264d0` (the FileWMFAnalyser validation entry; the
vtable-bearing ctor `FUN_180026480` at :29794 precedes it) — lines 29805-29840.
**Verified:** first-hand by the session-9 lead (sed read at the cited lines).

## The og's gate (verbatim, ghidra decompile)

```c
void FUN_1800264d0(undefined8 param_1,ushort *param_2,longlong param_3)
{
  ...
  local_20 = 0x90001;
  local_28 = 0x9ac6cdd7;
  if (*(int *)param_2 == 0x90001) {                       // standard WMF header
    if (param_2[8] == 0) {                                // u16@+16 == 0 (numOfParams)
      *(ulonglong *)(param_3 + 0x10) = (ulonglong)(uint)(*(int *)(param_2 + 3) * 2);
                                                          // extent = u32 size@+6 * 2
      *(undefined4 *)(param_3 + 0x18) = 0x46;             // format id 70
      return;
    }
  }
  else if ((*(int *)param_2 == -0x65393229) && (param_2[2] == 0)) {
                                                         // key D7 CD C6 9A AND u16@+4 (HWmf) == 0
    if (((param_2[10] ==
          (ushort)(param_2[9] ^ param_2[7] ^ param_2[6] ^ param_2[5] ^ param_2[4] ^ param_2[3] ^
                   param_2[1] ^ *param_2 ^ param_2[2] ^ param_2[8])) &&
                                                         // u16@+20 == XOR(words 0..9)  [10 words]
        (((param_2[2] == 0 && (*(int *)(param_2 + 8) == 0)) && (*(int *)(param_2 + 0xb) == 0x90001))
        )) && (param_2[0x13] == 0)) {
                                                         // u32@+16..19 (Reserved) == 0
                                                         // u32@+22 == 01 00 09 00 (std header follows)
                                                         // u16@+38 == 0 (std numOfParams)
      *(ulonglong *)(param_3 + 0x10) = (ulonglong)(*(int *)(param_2 + 0xe) * 2 + 0x16);
                                                         // extent = u32 size@+28 * 2 + 22
      *(undefined4 *)(param_3 + 0x18) = 0x46;
      return;
    }
  }
}
```

`param_2` is `ushort*` (u16 pointer), so `param_2[N]` = byte offset `2*N`:
- `param_2[2]` = +4 (HWmf), `param_2[8]` = +16, `param_2[10]` = +20 (checksum),
  `*(int*)(param_2+8)` = +16 (4-byte Reserved), `*(int*)(param_2+0xb)` = +22,
  `param_2[0x13]` = +38, `*(int*)(param_2+0xe)` = +28 (std-header size field),
  `*(int*)(param_2+3)` = +6 (std-header size field, standard path).

## Decoded og gate — placeable path

| # | Check | Canonical field |
|---|---|---|
| 1 | u32@+0 == 0x9AC6CDD7 | Key `D7 CD C6 9A` |
| 2 | u16@+4 == 0 | HWmf (must be 0) |
| 3 | u16@+20 == XOR(u16 words 0..9) | Checksum (words = bytes 0..19) |
| 4 | u32@+16 == 0 | Reserved (4 bytes, must be 0) |
| 5 | u32@+22 == 0x0090001 (bytes `01 00 09 00`) | standard WMF header follows |
| 6 | u16@+38 == 0 | std header's numOfParameters |
| 7 | extent = u32@+28 * 2 + 22 | std size-in-words * 2 + placeable header |

NO `Inch` validation exists in the og (no 14400 bound, no inch != 0 check).

## Our validator (verbatim, core @ 35adcb9)

`src/core/carve/cr_analysers.cpp:3383-3411`:

```cpp
CarveResult validate_wmf_placeable(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[22];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\xD7\xCD\xC6\x9A", 4) != 0)
        return r;
    const u16 inch = u16_from_bytes_le(h + 16);        // <- reads +16 = RESERVED, not Inch
    if (inch == 0 || inch > 14400)
        return r;                                       // <- rejects every canonical file (Reserved == 0)
    /* the placeable checksum: the stored word at +18 is the XOR of the
     * first 9 words ... */
    u16 x = 0;
    for (int i = 0; i < 9; ++i)
        x = u16(x ^ u16_from_bytes_le(h + 2 * i));      // <- words 0..8 (bytes 0..17)
    const u16 stored = u16_from_bytes_le(h + 18);       // <- reads +18 = inside Reserved
    if (x != stored)
        return r;
    u8 sh[4];
    if (!rd(s, hit + 22, sh, 4) || sh[0] != 0x01 || sh[1] != 0x00 || sh[2] != 0x09 || sh[3] != 0x00)
        return r;
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));  // <- bounded probe, not size*2+22
    ...
}
```

## The test plant mirrors the wrong convention (cannot catch the bug)

`tests/unit/test_carve.cpp:2257-2274`:

```cpp
size_t plant_wmf_placeable(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\xD7\xCD\xC6\x9A", 4);
    put16(img, at + 4, 0); /* handle */
    put16(img, at + 6, 0);
    put16(img, at + 8, 0);
    put16(img, at + 10, 100); /* bbox */
    put16(img, at + 12, 100);
    put16(img, at + 14, 200);      // <- canonical Inch position (+14) gets a bbox value
    put16(img, at + 16, 1440); /* inch */   // <- writes 1440 INTO the Reserved field
    u16 x = 0;
    for (int i = 0; i < 10; ++i)
        x = u16(x ^ u16_from_bytes_le(&img[at + 2 * size_t(i)]));
    put16(img, at + 18, x);        // <- checksum written at +18 (canonical: +20)
    std::memcpy(img.data() + at + 22, "\x01\x00\x09\x00", 4);
    ...
}
```

The plant produces a header no real application writes: `Inch=1440` inside
Reserved (canonical files carry 0 there), no checksum at +20. The og itself
would REJECT this plant (Reserved != 0; checksum@+20 = 0 != XOR(words 0..9)).

## Canonical layout (for the fix)

22-byte placeable header: Key(+0,4) HWmf(+4,2) BoundingBox(+6,8) **Inch(+14,2)**
**Reserved(+16,4)** **Checksum(+20,2)** = XOR of the first 10 words (bytes 0..19).
Standard header follows at +22 (18 bytes); the og additionally requires its
numOfParams@+38 == 0 and computes extent = size@+28*2 + 22.
