# Evidence: the CHM gate — the deferred compare-width puzzle, resolved at instruction level

**Corpus:** `easeus-drw-research` @ `be0cfe9`
**Sources:**
- `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` FUN_180016b40 (:17703-17740)
- `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c` function_180016b40 (:22629-22643)
- The og binary itself: `og-codebase/RAWSearchDLL.dll` — disassembled with
  capstone 5.0.7 by the session-9 lead (.text vaddr 0x180001000, paddr 0x400;
  function VA 0x180016b40 -> file offset 0x15f40).
**Verified:** first-hand; the two decompilers DISAGREE and the asm settles it.

## The two decompilers

Ghidra (:17716-17730):
```c
  if ((((((*param_2 == 0x46535449) && (param_2[2] == 0x60)) &&
        (*(longlong *)(param_2 + 6) == 0x11d07baa7c01fd10)) &&
       ((*(longlong *)(param_2 + 8) == -0x1319dd365ffff362 &&
        (*(longlong *)(param_2 + 10) == 0x11d07baa7c01fd11)))) &&
      ((*(longlong *)(param_2 + 0xc) == -0x1319dd365ffff362 &&
       ((param_2[0x18] == 0x1fe && (param_2[0x1c] == 0)))))) && (param_2[0x1d] == 0)) {
    *(undefined4 *)(param_3 + 0x18) = 0xe;                        // id 0x0E
    *(undefined8 *)(param_3 + 0x10) = *(undefined8 *)(param_2 + 0x1a);  // size u64@+0x68
```

RetDec (:22635-22638) — WRONG constants (mis-fused stack locals):
```c
  if (... || *(int64_t *)(a2 + 24) != 0x7c01fd10 || *(int64_t *)(a2 + 32) != -0x5ffff362
       || *(int64_t *)(a2 + 40) != 0x7c01fd11 || *(int64_t *)(a2 + 48) != -0x5ffff362 || ...)
```

## The instruction-level truth (capstone, og binary)

```asm
0x180016b57:  cmp  dword ptr [rdx], 0x46535449        ; "ITSF" @+0
0x180016b5d:  mov  dword ptr [rbp-0x30], 0x7c01fd10   ; } stack-built GUID1 lo-half
0x180016b64:  mov  dword ptr [rbp-0x2c], 0x11d07baa   ; }
0x180016b6b:  mov  dword ptr [rbp-0x28], 0xa0000c9e   ; } stack-built GUID1 hi-half
0x180016b72:  mov  dword ptr [rbp-0x24], 0xece622c9   ; }
0x180016b79:  mov  dword ptr [rbp-0x20], 0x7c01fd11   ; } GUID2 lo  (+0x11d07baa)
0x180016b87:  mov  dword ptr [rbp-0x18], 0xa0000c9e   ; } GUID2 hi
0x180016b8e:  mov  dword ptr [rbp-0x14], 0xece622c9   ; }
0x180016b97:  cmp  dword ptr [rdx + 8], 0x60          ; header length == 96
0x180016b9d:  mov  rax, qword ptr [rdx + 0x18]        ; u64 @+0x18
0x180016ba1:  cmp  rax, qword ptr [rbp - 0x30]        ; == 0x11D07BAA7C01FD10
0x180016ba7:  mov  rax, qword ptr [rdx + 0x20]        ; u64 @+0x20
0x180016bab:  cmp  rax, qword ptr [rbp - 0x28]        ; == 0xECE622C9A0000C9E
0x180016bb1:  mov  rax, qword ptr [rdx + 0x28]        ; u64 @+0x28
0x180016bb5:  cmp  rax, qword ptr [rbp - 0x20]        ; == 0x11D07BAA7C01FD11
0x180016bbb:  mov  rax, qword ptr [rdx + 0x30]        ; u64 @+0x30
0x180016bbf:  cmp  rax, qword ptr [rbp - 0x18]        ; == 0xECE622C9A0000C9E
0x180016bc5:  cmp  dword ptr [rdx + 0x60], 0x1fe      ; u32 @+0x60 == 510
0x180016bce:  cmp  dword ptr [rdx + 0x70], 0          ; u32 @+0x70 == 0
0x180016bd4:  cmp  dword ptr [rdx + 0x74], 0          ; u32 @+0x74 == 0
```

(then: `*(u32*)(param_3+0x18) = 0xe`; `*(u64*)(param_3+0x10) = *(u64*)(rdx+0x68)` —
the size field.)

**Ghidra is right; RetDec mis-fused the stack-built constants.** The compares
are genuine 64-bit reads compared against the full 16-byte GUID halves.

## The canonical ITSF layout — and why the prior session's expectation was wrong

The authoritative reverse-engineered CHM spec (Russotto, "Microsoft's HTML
Help (.chm) format"; corroborated by the CHM Spec page maintainers) defines
the initial header:

```
0000: char[4]  'ITSF'
0004: DWORD    3 (version)
0008: DWORD    total header length (0x60)
000C: DWORD    1 (unknown)
0010: DWORD    a timestamp            <- DWORD, not QWORD
0014: DWORD    Windows Language ID
0018: GUID     {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC}
0028: GUID     {7C01FD11-7BAA-11D0-9E0C-00A0C922E6EC}
```

followed by the header section table (2 x 0x10) + 8 bytes = header ends at
0x60. **Header Section 0** then begins at +0x60:

```
HS0+0000: DWORD  $01FE (unknown)
HS0+0004: DWORD  0
HS0+0008: QWORD  File Size
HS0+0010: DWORD  0
HS0+0014: DWORD  0
```

The og's gate checks EXACTLY this canonical layout: GUID1@+0x18 (u64s at
+0x18/+0x20), GUID2@+0x28 (u64s at +0x28/+0x30), HS0 magic 0x1FE@+0x60,
zeros at +0x70/+0x74 (HS0+0x10/+0x14), and the file size u64@+0x68
(HS0+0x08) as the extent. (The og does not check HS0+0x04 == 0.)

**Resolution of the deferred puzzle:** there is no "u64-widened GUID
compare" anomaly and no non-canonical byte layout. The GUID compares are
full-width (16 bytes, two u64 halves each) at the canonical +0x18/+0x28
offsets. The prior session's expectation placed the GUIDs at +0x20/+0x30 —
that is the in-memory struct layout used by some reimplementations (e.g.
chmlib's header struct with an 8-byte timestamp), not the on-disk file
layout, whose timestamp is a 4-byte DWORD at +0x10.

## Registration (for completeness)

u32 registry, no fallback chain (ghidra :2488-2499):
```c
    local_res8[2] = 0xe;                       // id 0x0E
    *(undefined ***)local_res8 = FileCHMAnalyser::vftable;
  local_res10[0] = 0x46535449;                 // "ITSF"
  plVar3 = FUN_180006c80(param_1,local_res10);
```

Bounds: the analyser's FileInfo sets no min/max pair in the ctor the census
quotes; the extent is taken directly from the file's own HS0 File Size
field (u64@+0x68) — a self-describing size, the strongest possible gate.

## Product status (core @ 35adcb9)

No FMT_CHM row; grep of `src/core/carve/` for ITSF/FileCHM: zero hits.
The core does not carve CHM at all (part of S-042's batch-2 set).
