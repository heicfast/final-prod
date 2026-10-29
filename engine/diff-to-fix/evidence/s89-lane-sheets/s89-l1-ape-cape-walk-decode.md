# S89-L1 — the APE (CAPEAnalyser) size walk, raw-asm decoded first-hand

The corpus binary: `og-codebase/RAWSearchDLL.dll` (SHA verified by the repo's
own inventory; objdump 2.44 over `.text` at the image base 0x180000000).
Every constant below was read from the disassembly directly, this session
(2026-10-30); the Ghidra decompile
(`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`) was used
for orientation only — two of its renderings were corrected by the asm
(see the corrections section).

## The chain

- **The registration** (`RAWSearchDLL_decompiled.c:1953-1959`): the map key
  `0x2043414d` = "MAC " → the `BruteForce::CAPEAnalyser` node, the format
  id **0x51** (81). The node's tier-1 `FUN_180033090`
  (0x180033090-0x18003309d) is the WPG-class one-function shape:

  ```asm
  180033090: 41 c7 47 18 51 00 00 00   movl $0x51,0x18(%r15)   ; the id, ONLY
  180033098: b8 01 00 00 00            mov  $0x1,%eax           ; return 1 (accept)
  ```

  No size is stamped — the extent is the +0x20 walk processor's job.

- **The factory** `FUN_1800330a0` (the CAPERawFileInfo ctor, 0x138 bytes):
  `[0x22] = 0` (the registered min = 0), the max field left at the
  allocator's zero (the walk's own cap does the bounding), the walk-state
  fields: +0x110 the accumulated position, +0x118 the pending scan bound,
  +0x120 the max seek entry, +0x128 the cached version, +0x12c the block
  align, +0x134 the seen-first-chunk flag, +0x10 the RESULT size.

- **The walk** `FUN_180033180` (0x180033180-0x180033603) — the size
  authority. The entry (raw-asm):

  ```asm
  1800331e7: e8 64 e1 fc ff         call 0x180001350      ; the singleton
  1800331ec: 48 8b 48 70            mov  0x70(%rax),%rcx  ; the registry bound
  1800331f0: 4c 8d 24 89            lea  (%rcx,%rcx,4),%r12
  1800331f4: 4d 03 e4               add  %r12,%r12        ; x10 -> the CAP
  ```

  **the cap = singleton+0x70 × 10 = 2,000,000,000** (singleton+0x70 =
  200,000,000, the DWG landing's established default).

## The version gate (the discovered live defect)

```asm
18003321c: 41 0f b7 46 04   movzwl 0x4(%r14),%eax    ; the u16 LE at +4
180033213: b9 10 27 00 00   mov   $0x2710,%ecx       ; 9999
180033228: 66 3b c1         cmp   %cx,%ax
18003322d: b9 e8 03 00 00   mov   $0x3e8,%ecx        ; 1000
180033232: 66 3b c1         cmp   %cx,%ax
```

**The version is the u16 LE at +4, gated to [1000, 9999]** — the version×100
encoding (3.95 = 3950, 3.98 = 3980, 4.xx = 4xxx). Outside the window, the
walk never writes +0x10 — the candidate never emits (the decline).

The core's stand-in read a **u32 BE** and gated **[300, 5000]** — a reading
under which every real Monkey's Audio file (u16 LE 3800..4400 → the u32 BE
0x0F8C-ish bytes read as 0x0F8C0000+) is REJECTED: the discovered live
defect (the M-156 ASCII-gate class — the format keyed correctly, real files
never carved).

## The format split at 3980 (0xf8c, raw-asm :272 and :56b)

### The OLD arm (< 3980, 0x180033366-0x180033478)

```asm
18003336b: c4 c3 f9 16 c0 01  vpextrq $0x1,%xmm0,%r8   ; the qword @ +8
180033374: 83 e2 04           and  $0x4,%edx
180033377: 83 ca 20           or   $0x20,%edx          ; base = 0x20 | (flags&4)
180033380: 41 f6 c0 10        test $0x10,%r8b          ; the descriptor dword
180033386: 46 8b 14 32        mov  (%rdx,%r14,1),%r10d ; aux = u32 @ base
18003338a: 83 c2 04           add  $0x4,%edx           ;   ... then base += 4
180033397: 44 39 7c 24 78     cmp  %r15d,0x78(%rsp)    ; u32@16 != 0:
18003339e: 83 c2 2c           add  $0x2c,%edx          ;   base += 0x2c
1800333a1..:                                          ; mult = ((flags&8)|0x10)>>3,
                                                      ;   8 if flags&1
1800333c7: 0f af c1           imul %ecx,%eax           ; align = u16@10 x mult
1800333d0: 85 c0              test %eax,%eax           ; align == 0 -> decline
1800333d4: 41 8d 43 ff        lea  -0x1(%r11),%eax
1800333d8: 83 f8 01           cmp  $0x1,%eax           ; u16@10 in {1,2} else decline
1800333ec: 8b 08              mov  (%rax),%ecx         ; first = u32 @ base
1800333ee: 85 c9              test %ecx,%ecx           ; 0 -> no size (decline)
1800333f6: 4c 3b e1           cmp  %rcx,%r12           ; first >= cap -> decline
1800333ff: b8 d8 0e 00 00     mov  $0xed8,%eax         ; 3800: the aux subtract
180033404..:                                          ; +0x118 = first (- aux if <= 3800)
```

The seek-table scan (0x180033421-0x180033478): the dwords from the base;
**a 0 entry or an entry >= the cap stops the scan; the max entry is the
walked size** (`0x120`).

### The NEW arm (>= 3980, 0x180033294-0x180033353)

```asm
1800332a9: c4 c3 79 16 c8 02  vpextrd $0x2,%xmm1,%r8d  ; desc = u32 @ +8
1800332bb: 45 85 c0           test %r8d,%r8d           ; <= 0 -> skip
1800332d8: 48 8d 41 18        lea  0x18(%rcx),%rax     ; desc+0x18 bounds gate
1800332e5: c5 fb 10 49 10     vmovsd 0x10(%rcx),%xmm1  ; the qword @ desc+0x10
180033300: 0f b7 44 24 78     movzwl 0x78(%rsp),%eax   ; u16 @ desc
180033305: c1 e8 03           shr  $0x3,%eax           ; >> 3
180033308: 0f af c1           imul %ecx,%eax           ; x u16 @ desc+2 -> align
180033317: 85 c0              test %eax,%eax           ; the {1,2} gate on the second
```

The size sum (0x180033323-0x180033353) — the og's own arithmetic:

```asm
180033323: 8b 44 24 40   mov  0x40(%rsp),%eax    ; u32@16
180033331: 41 03 c0     add  %r8d,%eax           ; + u32@8
180033334: 48 8b 4c 24 38   mov 0x38(%rsp),%rcx ; (>>0x20) -> u32@12
18003333d: 48 03 4c 24 48   add 0x48(%rsp),%rcx ; + u32@24
180033342: 48 c1 ea 20   shr  $0x20,%rdx        ; u32@20
180033349: 8b 44 24 50   mov  0x50(%rsp),%eax   ; u32@32  <-- the MD5 term
180033350: 48 03 c1     add  %rcx,%rax          ; the sum
180033353: e9 43 01 00 00  jmp  0x18003349b     ; -> size
```

**size = u32@8 + u32@12 + u32@16 + u32@20 + u32@24 + u32@32** — the
descriptor's size fields with the **dword@32 (the first MD5 dword)
included** — the og's arithmetic, kept verbatim (the registered quirk).

## The epilogue (0x1800334a3-0x1800335c1)

The gates: size != 0, align > 0, **accum >= size** (the feed chunk where
the accumulated position first reaches the size — the finalizing chunk =
the 512 bytes ending at round_up(size, 512)). The backward scan:

```asm
1800334e4: 48 8b 35 85 8a 03 00  mov 0x38a85(%rip),%rsi  # 0x18006bf70
```

The .rdata constant at 0x18006bf70, read byte-exact from the PE:
**"APETAGEX"** — the APEv2 footer magic. The footer gates:

```asm
180033522: 3d ff ff 00 00   cmp  $0xffff,%eax        ; items (u32@f+16) <= 0xFFFF
18003352d: 8d 42 e0         lea  -0x20(%rdx),%eax    ; (tag_size - 0x20)
180033530: 3d 00 00 00 01   cmp  $0x1000000,%eax     ; signed <= 16 MiB
18003354e: 41 c1 e8 1d      shr  $0x1d,%r8d          ; flags bit 29 (is-header)
180033556: 85 c0            test %eax,%eax           ; flags bit 31 (has-header)
180033545: 8d 7a 20         lea  0x20(%rdx),%edi     ; span = size + 0x20 (bit31 set)
18003355a: 8b fa            mov  %edx,%edi           ; span = size (bit31 clear)
```

The extent arms (raw-asm 0x180033567-0x1800335bd):

- **OLD + tag** (0x180033593): `r8 = r15d(tag_start) + 0x28(%rsp)
  (prev_accum) + rcx(span)` — the span algebra cancels to
  **the absolute footer end (footer + 0x20)**.
- **NEW + tag** (0x18003357c-:591): `r8 = size + span`, then
  `r8 = r8 - (r8 % 512) + 512` — **round_up(size + span, 512)**.
- **no tag** (0x1800335a0-:5b9): `accum - zero_tail` (the trailing zeros
  of the finalizing chunk), rounded UP to the chunk, **+ 512** (the og's
  own slack, kept).

The final clamp (0x1800335c6): `if (cap < size) size = cap` — the 2e9
walk cap.

## The corrections to the Ghidra renderings (asm-arbitrated)

1. The decompile's `local_78._0_4_` "u32@32" term looked like a mis-offset;
   the asm confirms it IS 0x50(%rsp) = the buffer's bytes 32-35 (the MD5
   term) — the og sums it deliberately.
2. The decompile's `if (*(int *)(param_5 + 0x14) < 1)` epilogue gate is the
   `cmp %r15d,0x14(%r13); jle` — the context's +0x14 field against the
   chunk length (the pending-list cadence gate the S90 WPG corroboration
   decoded); the whole-file model takes the finalize arm directly (the
   registered CRW/WPG-class model divergence).

## The return protocol

`edi` = 2 at entry (continue feeding); the epilogue-complete arms set
`edi = 1` (done) — the same one-shot state machine as WPG's
"returning 1 (done) after the first walked chunk", with the continuation
driven by the pending re-feed cadence (the registered model divergence).
