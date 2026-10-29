# S84-L1 — the DWG size walk, decoded first-hand from the raw asm (M-107's DWG member, member 3 of 11)

**Session-84 fix lane (2026-10-29/30).** Method: `objdump -d` over
`og-codebase/RAWSearchDLL.dll` (the corpus binary itself — the committed
Ghidra decompile is CI-generated and used only as a cross-check); vtables
read byte-exact from .rdata via pefile.

## 1. The analyser identity (byte-exact)

The registry initializer at 0x1800036bf binds the u32 key
`0x30314341` ("AC10") to a 0x10-byte node with vtable **0x18006bc00** and
idx **0x18** — `BruteForce::CDWGAnalyser` (the Ghidra symbol confirms the
class name at RAWSearchDLL_decompiled.c:1986-1996). The node's vtable
+0x38 slot = **FUN_18002ff20** — the self-factory:

```
alloc(0x508)                          ; 0x18002ff2f-0x18002ff34
base-init; vtbl = 0x18006bc50         ; 0x18002ff49-0x18002ff56
[+0x110] = 0x15                       ; ★ the INITIAL SCAN POSITION = 21 (0x18002ff59)
[+0x118] = 0 ; [+0x11c] = 0           ; record_count, initialized flags
[+0x120] = 0x3d8                      ; mode-3's remaining-byte counter (984)
[+0x500] = 0                          ; the generation state
memset([+0x128], 0, 0x3d8)            ; mode-3's accumulator
```

The instance vtable 0x18006bc50: +0x00 = the ctor (0x18002f2d0), **+0x20 =
FUN_18002f320 (the chunk-feeding WALK — the X3F-family slot)**, +0x10 =
0x18002fe30 (the tier-1 version gate). The registry node's +0x40 slot =
0x18000bae0 (the return-0 stub) — **CDWGAnalyser has NO File*Analyser
secondary; the BruteForce walk IS the size authority** (the M-155/N-217
dispatch decode's 8-override census).

## 2. FUN_18002f320 — the three-generation walk

The version pair (bytes 4-5) routes one of three generations. The version
tables are built as 6-byte entries {"AC10" + the pair} on the stack
(0x18002f345-0x18002f3f8; 0x18002f47b-0x18002f4ee):

- **Mode 1 — {AC1015, AC1014, AC1013, AC1012} (R2000/R14/R13/R12):**
  state 1; the section-locator record walk (§3).
- **Mode 2 — {AC1018, AC1024, AC1027, AC1032} (R2004/R2010/R2013/R2018):**
  state 2; the XOR-decoded section page map (§4).
- **Mode 3 — {AC1021} (R2007):** state 3; the 0x3D8-byte header
  accumulator + FUN_18002fc50's three-plane interleave (0xef×3) — the
  third generation, NOT decoded this session (the finding's recorded
  remainder for this member).

## 3. Mode 1: the section-locator record walk (0x18002f3b8-0x18002f473)

```
count = u32 @ chunk[+0x110]          ; the factory's 0x15 (file offset 0x15)
scan_pos = 0x19                       ; 0x15 + 4 (0x18002f3c9)
while (record_index < count):
    if (chunk[scan_pos] != record_index):    ; 0x18002f406-0x18002f40c
        extent = 0 ; DECLINE                 ; the index gate
    candidate = u32@[scan_pos+5] + u32@[scan_pos+1]   ; size + seeker
    extent = max(extent, candidate)          ; cmovbe (0x18002f41c-0x18002f41f)
    scan_pos += 9 ; record_index++           ; the 9-byte record stride
    if (scan_pos > chunk_len):                ; the streaming carry
        scan_pos -= chunk_len ; return FEED-ME-MORE
```

Record layout: **{u8 index, u32 seeker @+1, u32 size @+5}**, 9-byte stride,
records from file+0x19. The extent = the max over records of
(seeker + size). A read failure past the device end ends the walk with
the extent so far (the chunk-starvation salvage — the feeding stops, the
extent stands).

## 4. Mode 2: the XOR-decoded section page map (0x18002f591-0x18002f5e5)

```
FUN_18002f630(decoded, chunk+0x80)    ; 0x18002f5a0: lea rdx,[rdi+0x80]
```

**The keystream** (FUN_18002f630, 0x18002f667-0x18002fb84): 108
sequential bytes of the MSVC LCG — `seed = seed*0x343fd + 0x269ec3`
(32-bit wrap, seed0 = 1), `byte = (seed >> 16) & 0xFF`. (The Ghidra
decompile's stride-0x36 rendering is the two-register unroll; the writes
cover [rsp+0x20 .. rsp+0x8B] = the full 108 bytes, the SSE XOR tail at
0x18002fc10-0x18002fc28 completes 0x40..0x6C.) The decode:
`decoded[i] = enc[i] ^ keystream[i]` for the 108 bytes at file+0x80 —
the R2004+ obfuscated section page map.

**The six validation gates** (0x18002f5b0-0x18002f5db; the decoded
buffer at [rbp-0x41]):

| field | gate |
|---|---|
| u32 @ decoded+0x0B | == 0 |
| u32 @ decoded+0x10 | == 0x6C (108) |
| u32 @ decoded+0x14 | == 4 |
| u32 @ decoded+0x44 | == 0x20 (32) |
| u32 @ decoded+0x48 | == 0x80 (128) |
| u32 @ decoded+0x4C | == 0x40 (64) |

Any gate failing → the extent stays 0 (the decline).

**The extent**: `extent = u64 @ decoded+0x34 + 0x6C`
(0x18002f5dd-0x18002f5e5: `mov rax,[rbp-0xd]; add rax,0x6c; mov [rbx+0x10],rax`).

## 5. The sanity cap (0x18002f5e9-0x18002f5f9)

```
if (FUN_180001350()->[+0x70] * 100 < extent) extent = 0
```

The scan-singleton's +0x70 field × 100 — the same undecoded field the
X3F family's cap reads (`>> 1` there, `* 100` here). The registered
approximation (the X3F/CRW precedent): the row-max probe limit —
within-device walks emit, beyond-device claims decline. Recorded in the
finding's addendum as the named approximation.

## 6. The landing

`validate_dwg` keeps the registry key gate and replaces the 64 MiB
DWG-pattern stand-in for the R12-R2000 and R2004-R2018 generations with
the two walks (the 256 KiB windowed read — the direct-read efficiency
convention; the og's streaming chunk-carry is the registered
model-divergence class, the X3F/CRW stance). AC1021 (R2007) alone rides
the bounded extent — the third generation's accumulator
(FUN_18002fc50's interleave + the count-gated record parse) stays on the
finding's recorded remainder. The version set tightens to the og's exact
tables {12,13,14,15,18,21,24,27,32} (the old list's "30" is not an og
version; "13" is).

RED→GREEN first-hand via the stash dance:
DwgSectionLocatorWalkSetsExtent + DwgWalkIndexMismatchDeclines +
DwgR2004SectionMapWalkSetsExtent + DwgR2004SectionMapGateFailsDeclines
RED at the stand-in tree; GREEN with the walk;
DwgR2007StandsInBounded pins the recorded remainder. The carve suite
279/279.

## 7. Open (the recorded remainder)

- Mode 3 (AC1021/R2007): FUN_18002fc50 — the 3×0xef interleave over the
  0x3D8 accumulated header bytes, the count gate at +0x18, the 0x20-stride
  record parse at +0x20, the two-variant extent fold. The next probe: the
  interleave order (which of the three 0xef planes leads) and the extent
  field's offset in the de-interleaved map.
- The sanity cap's singleton field (+0x70) — the same undecoded field as
  X3F's; the row-max approximation stands until a decode of the singleton
  init pins it.
