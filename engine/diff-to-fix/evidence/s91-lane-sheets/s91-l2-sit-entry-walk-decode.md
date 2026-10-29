# S91-L2 — the SIT next-entry walk, decoded first-hand from the raw asm (M-107's SIT member, member 7 of 11)

**Session-91 fix lane (2026-10-30).** Method: `objdump -d` over
`og-codebase/RAWSearchDLL.dll` (corpus `be0cfe9`); vtables read
byte-exact from .rdata via pefile. All addresses are instruction
addresses against the corpus binary.

## 1. The registry chain (byte-exact)

The registration cascade (the pipelined pattern — the node created in
block N is inserted for the key of block N+1, calibrated against the
FOVb/X3F and II/CRW anchors in S91-L1):

- Key `"Stuf"` (0x66757453, block at 0x180003a69): the map insert at
  0x180003a7c stores the node created in the preceding block —
  **{primary vtable 0x180067890, idx 0x5d (93 — the SIT chain),
  secondary {vtable 0x18006d5a8, idx 0x5d} linked at +0x10}**.

The primary vtable 0x180067890:

| slot | value | meaning |
|---|---|---|
| +0x00 | 0x1800159b0 | dtor |
| **+0x10** | **0x180015a20** | **the tier-1 (this decode)** |
| +0x38 | 0x180015a90 | **the SIT factory (node-specific — not the shared 0x18000ec90)** |
| +0x40 | 0x18000bae0 | the return-0 stub — **no GetFallback override → no secondary consult** |

## 2. The tier-1 (FUN_180015a20)

```
if (candidate == NULL) → decline
if (u16@4 != 0x4966 "fI") → decline        ; 0x180015a38-0x180015a4f
if (buffer[6] != 0x74 't') → decline        ; 0x180015a56
result.id = 0x5d                            ; 0x180015a5c
return 1
```

With the map key "Stuf", the full gate is the 7-byte "StuffIt"
signature — the core's current gate is byte-exact already. **The tier-1
stamps ONLY the id — NO size** (no write to result+0x10): the WPG
pattern; the extent comes from the child processor.

## 3. The factory (FUN_180015a90) — the child and its caps

`new(0x120)` + the generic base ctor (0x18000b520) + vtable
**0x180067828**; then:
- child+0xd8 = **0x5000 (20480)** — the min (the N-093-decoded
  SITRawFileInfo floor, now seen in its native slot)
- child+0x118 = **0xbebc200 (200,000,000)** — the max (N-093's cap)
- child+0x18 = 0x5d (the chain id)

## 4. The processor (FUN_180015d70, the child vtable's +0x20 slot) — the walk

Called per fed chunk with `(child, buffer, sector_size = ctx->[0xf0]
(the dispatch at 0x18000d7a4/0x18000d7ae), position, ctx)`:

```
rcx = position - child->[0x8]           ; the chunk distance from the hit
if (rcx == 0):                          ; the FIRST chunk (the hit itself)
    child->id = 0x5d
    format = buffer[7]                  ; the 8th byte
    if (format == 0x20 ' '): child->size = 0            ; 0x180015dd2
    if (format == 0x21 '!'): child->[0x110] = 1         ; the bang-format flag
    if (child->size > 0) → return 1                     ; (dead at first feed — size starts 0)
    → fall into the continue gate
else:                                   ; the LATER chunks
    format = buffer[7] - 0x20
    if (format <= 1):                   ; the chunk's first 8 bytes are "StuffIt "/"StuffIt!"
        if (u16@4 == 0x4966 && buffer[6] == 't') → FOUND
    if (u64@4 == 0x454b4941 "AIKE"+4NULs && u32@0xc == 0x7bbd74a3) → FOUND
FOUND:
    if (u32@0 == "Stuf") → FINALIZE     ; 0x180015eab
    if (u32@0 == "WEBB") → FINALIZE
    → fall into the continue gate
CONTINUE gate (0x180015e69):
    walked = sector_size * (position - child->[0x8])
    cap = 10 * child->[0x118] = 2,000,000,000            ; 0x180015e69-0x180015e81
    if (walked >= cap) → FINALIZE                        ; the walk cap
    if (ctx->[0x14] >= 5):
        if (!sub_check_180015b60(child, buffer, sector, pos, ctx)) → FINALIZE
    return 2                            ; CONTINUE — feed the next chunk
FINALIZE (0x180015eab):
    (a 0x104-byte name buffer is built and stored into child->[0x20]
     — the CZeroEndRawFileInfo-family name machinery, not decoded further)
    size = (position - child->[0x8] + 1) * sector_size   ; 0x180015f42-0x180015f55
    return 1
```

**The walk's semantic:** the extent runs from the hit to the END of
the SECTOR that contains the next StuffIt-family header — the
terminators being (a) the classic 8-byte `"Stuf" + "fI" + 't' +
format ∈ {' ','!'}` (the next archive's own header) or (b) the
wrapper family `u32@0 ∈ {"Stuf","WEBB"} + "AIKE" at +4 + four NULs +
the checksum 0x7bbd74a3 at +0xc`. The checks read ONLY the chunk's
first bytes — the terminator must be SECTOR-ALIGNED to stop the walk
(the pending child is re-fed each sector as the scan advances; the
racing S90 lane's WPG corroboration documents the same cadence).

The no-terminator exit: the 2 GB walk cap finalizes with the walked
chunk end — an extent far past the 200 MB row max, which the row gate
then rejects. **A no-terminator SIT does not carve in the og** (the
2 GB cap exists to bound the pending machinery, not to salvage).

## 5. The landing (the core's single-shot model)

`validate_sit` keeps the 7-byte tier-1 gate (byte-exact) and replaces
the 64 MiB stand-in with the walk: sector-stepped (512-byte) from
hit+512, the terminator gate at each sector-aligned position (the two
families above); found → `extent = hit + off + 512` (the walked
sector's end); no terminator in the probe window → the walked-bound
salvage at the probe limit (the X3F/CRW loop-exit family posture for
the device-end torn case — the og's own 2 GB cap path row-rejects, and
the device-end force-finalize is the pending machinery's walked bound;
the registered model translation, named below). The row gate
[20480, 200 MB] applies as the engine's own emission gate.

## 6. Open questions (honest)

(a) The sub-check FUN_180015b60 (the bang-format '!' validation, gated
on ctx->[0x14] >= 5 and child->[0x110]) was read only at its gate
structure — its inner entry-index parse was not decoded to landing
quality; the landing's terminator model is complete without it (it can
only FINALIZE earlier, and its finalize stamps the same walked-sector
formula).
(b) The 0x104 name-buffer machinery at FINALIZE (the name string) is
the CZeroEndRawFileInfo family — out of the size-walk scope.
(c) Whether the device-end force-finalize stamps the walked bound or
declines — the X3F/CRW loop-exit precedent supports the walked bound
(the registered posture this landing takes).
