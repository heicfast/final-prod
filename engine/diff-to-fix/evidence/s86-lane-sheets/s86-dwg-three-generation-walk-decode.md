# S86 lane sheet — the DWG three-generation walk decode (M-107 member 3)

**Decoded first-hand this session from the raw asm** (objdump over
`og-codebase/RAWSearchDLL.dll`, the corpus binary at `be0cfe9`; every cite
below is an instruction address against the PE image base 0x180000000, the
S83 arbiter convention — the committed Ghidra decompile is CI-generated and
its line cites drift).

## The object model (BruteForce::CDWGAnalyser / CDWGRawInfo)

- The gate `FUN_18002fe30` (@ 0x18002fe30): compares dword@file+0 ("AC10" =
  0x30314341) and u16@file+4 against a ten-pair table built on the stack
  (`mov DWORD PTR [rbp-0x61],0x30314341` @ 0x18002f345 + the ten u16
  suffixes, loop bound `cmp edx,0x9` @ 0x18002f39f). The ten pairs, LE u16
  suffix values read from the stack constants:
  **{0x3233 "32", 0x3732 "27", 0x3432 "24", 0x3132 "21", 0x3831 "18",
  0x3531 "15", 0x3431 "14", 0x3331 "13", 0x3231 "12", 0x3930 "09"}**.
  A match stamps the result id 0x18 (`mov DWORD PTR [param_3+0x18],0x18`).
  **"AC1030" (0x3033) is NOT in the table** — the core's S-007-era gate
  carried a phantom "30" and lacked "09"/"13".
- The chunk processor `FUN_18002f320` (@ 0x18002f320): the size walk, a
  state machine keyed on the generation field [+0x500] (1/2/3), with the
  per-file state initialized by the factory `FUN_18002ff20`
  (@ 0x18002ff20): `obj[0x110] = 0x15` (the gen-1 cursor — a full u64
  store that also zeroes the record index at +0x114), `obj[0x120] = 0x3d8`
  (the gen-3 accumulation target), `obj[0x128..]` = a 0x3d8-byte zeroed
  accumulation buffer, latch [+0x11c] = 0. The base ctor `FUN_18000b520`
  zeroes the size field ([+0x10]) — the decline default.

## Generation 1 — the section-locator-records walk ({"12","13","14","15"})

Table built at 0x18002f345-0x18002f36b (pairs 0x3531 "15", 0x3431 "14",
0x3331 "13", 0x3231 "12"; the compare loop `cmp edx,0x4` @ 0x18002f39c).

- First call (latch [+0x11c] == 0, set at 0x18002f3af): the record COUNT is
  the dword at file+0x15 (the cursor init), the cursor advances 4 → records
  start at file+0x19.
- The loop (@ 0x18002f400-0x18002f441):
  - the id byte at [cursor] is zero-extended and compared against the FULL
    u32 record index (`movzx eax,BYTE PTR [rdx+rdi]` + `cmp eax,ecx` @
    0x18002f406-0x18002f40a) — **an index above 255 can never match**, so
    over-long tables decline;
  - `end = [cursor+5] + [cursor+1]` as a wrapping 32-bit sum
    (`mov eax,[rdx+rdi+0x5]; add eax,[rdx+rdi+0x1]` @ 0x18002f40e-0x18002f412);
  - the running max keeps the prior size when the sum is smaller
    (`cmp rax,r9; cmovbe rax,r9` @ 0x18002f41c-0x18002f41f), stored to the
    size field EVERY iteration (`mov [rbx+0x10],rax` @ 0x18002f429 — the
    torn-salvage property);
  - cursor += 9 (`add r8d,0x9` @ 0x18002f423), index++;
  - a mismatched id zeroes the size at 0x18002f46b — the hard decline;
  - count 0 or the sign bit set never enters (`cmp ecx,edx; jge` @
    0x18002f3f0 — the signed compare): garbage-huge counts decline.
- The chunk-carry (cursor -= chunk_len @ 0x18002f452) is the og's streaming
  model; the core's random-access reads are the registered model-divergence
  equivalent.

## Generation 2 — the obfuscated AcDb:Header walk ({"18","24","27","32"})

Table built at 0x18002f478-0x18002f49d (pairs 0x3831 "18", 0x3432 "24",
0x3732 "27", 0x3233 "32").

- The de-obfuscator `FUN_18002f630` (@ 0x18002f630): the 0x6c bytes at
  file+0x80 (`lea rdx,[rdi+0x80]` @ 0x18002f5a0) are XORed with the MSVC
  rand() byte stream seeded 1 — the software-pipelined LCG at
  0x18002f670-0x18002f687: `state = 1; state = state*0x343fd + 0x269ec3
  (mod 2^32); byte = (state >> 16) & 0xFF` (first byte 0x29 = 0x29B2C0 >>
  16; the `sar ecx,0x10` + `mov cl` pipeline is byte-identical to `& 0xFF`
  of the shifted value).
- The gate chain (@ 0x18002f5b0-0x18002f5db), all six must hold on the
  de-obfuscated header: `u8[0xb] == 0` (the "AcDb:Header" string
  terminator), `u32[0x10] == 0x6c`, `u32[0x14] == 4`, `u32[0x44] == 0x20`,
  `u32[0x48] == 0x80`, `u32[0x4c] == 0x40` — any failure jumps to the tail
  with the size still at the ctor's 0 (the decline).
- The extent: **`u64[0x34] + 0x6c`** — the 8-byte read + add at
  0x18002f5dd-0x18002f5e5 (`mov rax,QWORD PTR [rbp-0xd]; add rax,0x6c`).

## Generation 3 — the R2007 accumulate + compressed-map chain ({"21"})

- The pair check @ 0x18002f4eb-0x18002f505 (0x3132 "21" only).
- The accumulate state machine @ 0x18002f519-0x18002f57f: the first chunk
  contributes bytes from buffer+0x80 onward (`lea rax,[rdi+0x80]; lea
  esi,[r8-0x80]`), later chunks whole; the target is the factory's 0x3d8;
  when the remainder goes non-positive the parser `FUN_18002fc50` runs.
- `FUN_18002fc50` (@ 0x18002fc50): copies 0x3d8 bytes from the object's
  accumulation buffer, then de-interleaves THREE 239-byte passes at stride 3
  (the `3 x 0xef` strided copy @ 0x18002fc24-0x18002fce2) into a 717-byte
  (0x2cd) buffer, then reads dword@0x18 of the de-interleaved buffer and,
  when `< 1`, walks the R2007 compressed section map via `FUN_18002ef50`
  (@ 0x18002ef50 — a record walker with its own size-class grammar: the
  high-nibble class codes, the 0x17 special record, the 0xFF skip runs,
  the `0x40 < size` cap, `FUN_18002eb10` the record parser).
- **NOT decoded to landing depth this window** (the record grammar's
  size-class table + the final size accumulation are a follow-up): the
  bounded extent stands in for THIS generation only, pinned by
  `Carve.DwgR2007BoundedExtentRemainderPinned` so the remainder is
  conscious, never silent.

## The global cap

@ 0x18002f5ee: `imul rax,QWORD PTR [rax+0x70],0x64` — the engine-singleton
field [+0x70] times 100; a size above it is zeroed (the reject). The
singleton field is undecoded (the X3F S71/S80 precedent registered the same
field with a `>> 1` scale); the core approximates the cap by the registry
row bound + device end (the registered approximation).

## The observable deltas the landing fixes

1. **AC1013** files (in the og's gate AND gen-1 walk): the old core gate
   declined them → now carved by the locator walk.
2. **AC1030** files: the old core gate accepted a pair the og never lists →
   now declined.
3. **AC1012/14/15**: the blind 64 MiB stand-in → the locator walk (og-exact
   extents; id-mismatch corruption now declines instead of blind-carving;
   torn tables salvage the walked max).
4. **AC1018/24/27/32**: the blind stand-in → the de-obfuscated-header walk
   (extent = u64[0x34] + 0x6c; failed gate constants decline).
5. **AC1009**: gate-passed, un-walked in the og (size stays 0) → the core
   mirrors the decline (the old core also declined, at its own gate — the
   observable outcome unchanged, the mechanism now og-exact).

## Test pins (RED first-hand via the stash dance at this lane)

- `Carve.DwgLocatorWalkSetsExtent` — RED (the stand-in carved 64 MiB).
- `Carve.DwgLocatorWalkIdMismatchDeclines` — RED (the stand-in carved
  regardless of the id).
- `Carve.DwgLocatorWalkTornTableSalvagesWalkedMax` — RED (the stand-in
  carved the blind cap).
- `Carve.DwgGateMatchesOgTenPairTable` — RED ("13" declined; "30" carved).
- `Carve.DwgR2004ObfuscatedHeaderWalkSetsExtent` — RED (the stand-in carved
  64 MiB).
- `Carve.DwgR2004HeaderGateConstantFailsDeclines` — RED (the stand-in
  carved regardless of the constants).
- `Carve.DwgR2007BoundedExtentRemainderPinned` — the remainder pin.

The batch S-007 DWG plant re-pinned to carry a real locator table (the
house re-pin convention from the X3F/CRW landings; `expect_carved`'s
minimum semantics keep the batch expectation at the true 4096).
