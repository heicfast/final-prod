# S84-L2 — the WPD size walk, decoded first-hand from the raw asm (M-107's WPD member, member 6 of 11)

**Session-84 fix lane (2026-10-30).** Method: `objdump -d` over
`og-codebase/RAWSearchDLL.dll`; the Ghidra decompile cross-checked.

## 1. The analyser identity

The registry initializer's WPD block (0x180003cb5-0x180003cd9): node
vtable **0x18006cff0**, idx **0x44**, bound to the u32 key
`0x435057ff` ("FF WPC") — `FileWPDAnalyser` (the Ghidra symbol at
RAWSearchDLL_decompiled.c:2250-2258). The File*Analyser family shape:
**the +0x10 slot is the WHOLE analyser** — the tier-1 validation AND
the size in one function (the FileX3FAnalyser FUN_1800535b0 precedent).
(An adjacent vtable 0x18006cfa0 carries the 0x180041eb0 walk — that is
FileRIFFAnalyser under the "RIFF" key, idx 0x38, NOT WPD; the
attribution was resolved by tracing the registry leas: 0x180003e61 →
0x18006cfa0 vs 0x180003cb5 → 0x18006cff0.)

## 2. The walk (FUN_180041f50, the vtable 0x18006cff0's +0x10 slot)

**The tier-1 pair table** (0x180041f63-0x180041ff5): a 0x24-byte compare
table at 0x18006ef40 copied to the stack (vmovdqu + the trailing dword
0x3D), then THREE compare groups (0x180041f8a, 0x180041fa8, 0x180041fc7),
each checking the buffer's u32@+8 and u32@+0xE against a known pair:

| group | u32 @ +8 | u32 @ +0xE | the result id (+0x18) |
|---|---|---|---|
| 1 | 0x01020a01 | 0x00050200 | 0x44 |
| 2 | 0x00021601 | 0x00030000 | 0x45 |
| 3 | 0x00010f0f | 0x00030000 | 0x3D |

An unknown pair → `test r10d,r10d; jne` fails → `xor eax,eax; ret` —
**return 0, the DECLINE** (0x180041ff0-0x180041ffb).

**The size** (0x18004200c-0x180042015): on the matched path,
```
mov    ecx,DWORD PTR [r9+0x14]   ; r9 = the buffer
mov    eax,0x1                   ; success
mov    QWORD PTR [r8+0x10],rcx   ; the result's extent
```
**extent = u32 @ file+0x14** — the declared document size (the real WPD
format's "document size" field), never a walk, never a refusal once the
pair held.

## 3. The landing

`validate_wpd` replaces the 64 MiB stand-in AND the invented 'nonzero
product word at +4' gate (the og checks +8/+0xE, not +4): the exact
three-pair tier-1 (the decline on unknown pairs) + the declared size at
+0x14, clamped by the device end. Tests:
WpdPairTableDeclaredSizeSetsExtent (RED first-hand at the stand-in tree:
the extent was the device-end-clamped 64 MiB) + WpdUnknownPairDeclines
(the pair-table decline); the S-007 batch's plant re-pinned to the
pair-table shape. The carve suite 288/288.
