# S83-L2 — the CRW CIFF heap size walk, decoded first-hand from the raw asm (M-107's CRW member, member 2 of 11)

**Session-83 fix lane (2026-10-29).** Method: `objdump -d` over
`og-codebase/RAWSearchDLL.dll`; vtables read byte-exact from .rdata via
pefile. The prior lanes' cite for this member ("FUN_180052160, the CIFF
heap chain" — recorded in M-081's landing comment) pointed at the right
function; this session decoded it in full and found it is BOTH the CRW
walker's table-builder AND the walk itself (FUN_180052150, the CRW
child's vtable +0x08 slot, is a 3-instruction trampoline into
FUN_180052160 — the function builds its type-dispatch tables on entry,
then runs the walk).

## 1. The registry chain (byte-exact)

The CRW registry binding at 0x18000308b/0x1800030c4: `map['II\x1A\x00']`
and `map['MM\x1A\x00']` → the node from the factory **FUN_18003c9f0**:
a 0x18-byte registry node (vtable 0x18006c840, idx **0x12** — the CRW
chain index, matching the M-081 decode) with a **0x418-byte child**
(vtable **0x18006e008**, idx 0x12, a 0x400-byte zeroed buffer at
child+0x10 — the type-dispatch tables' storage).

The child's vtable 0x18006e008:
- +0x00: 0x180051fb0 (dtor)
- **+0x08: FUN_180052150 → FUN_180052160 (the WALK — this decode)**
- **+0x10: FUN_180052000 (the tier-1 grammar — the M-081 landing's
  "00 00 HEAP + {CCDR,TIFP,ARCH} + version 2" gates; the core's current
  validate_crw grammar ports it exactly — verified unchanged)**
- +0x38: 0x18000ec90 (the self-factory class)
- +0x40: the return-0 stub (the CRW node class does not override
  GetFallback — the same shape as X3F/DMG, see the S83-L1 sheet §3)

## 2. The walk (FUN_180052160's walk section, 0x1800524e4-0x1800526d7)

```
cap = 0x3200000 (50 MiB)            ; 0x18005219d `mov r15d,0x3200000`
sector_size = device->vt[+0x28]()   ; the cell-test branch reads [dev+0xc]
pos = 0
loop while (pos <= cap):
    buf = read(ctx, pos)            ; FUN_180042930, file-relative
    if (buf == NULL) → exit          ; extent = r15 (the cap / last value)
    ; the sector-tail gate:
    tail = *(u32*)(buf + (sector_size - pos % sector_size) - 4)
    if (tail != 0) { pos += sector_size; continue }
    ; zero-tailed sector: search the WHOLE sector (bound = sector-6)
    ; for the three 6-byte CIFF directory-entry signatures:
    ;   {u16 type == 0x2807, u32 length ∈ {0xa8, 0xb2, 0xe4}}
    ; (the signatures built as locals at 0x1800521a3-0x1800521c4:
    ;  dword 0xa82807/0xb22807/0xe42807 + word 0 → bytes 07 28 XX 00 00 00;
    ;  the compare sequence at 0x1800525c8-0x18005262f)
    on match at buf+i:
        len = *(u32*)(match + 2)     ; 0x18005267e
        ; the walker's own tail table (the map built 0x1800521dc-0x18005234b,
        ; values written at 0x18005224f / 0x1800522cb / 0x18005234b):
        ;   {0xa8 → 0x46, 0xb2 → 0x3c, 0xe4 → 0x32}
        extent = pos + i + TAIL[len]  ; 0x1800526b8-0x1800526c4
        → exit
    ; no match: the backup-80 re-probe (0x18005264d): read at pos-0x50,
    ; prefix-match the 80-byte window against the marker map
    ; (FUN_180051ed0 — memcmp against the same 07 28 XX family)
    if (matched) { extent = pos + sector_size; → exit }   ; 0x1800526d3
    pos += sector_size
exit: [param2 + 0x10] = extent       ; 0x1800526d7
```

**The no-directory exit stamps the CAP itself** (r15 is reset to
0x3200000 at each miss — 0x180052641 — and the read-fail/cap-exit paths
carry it out at 0x1800526d7): a torn CRW with no directory entry
anywhere carves at the 50 MiB walked bound — the same never-a-refusal
salvage class as the X3F legacy walk (S83-L1 §2).

## 3. The landing (the core's single-shot model)

`validate_crw` keeps the M-081 grammar gates (verified byte-exact against
FUN_180052000) and replaces the blind 64 MiB stand-in with the walk: the
windowed byte-exact search for {u16 0x2807, u32 len ∈ {0xa8, 0xb2, 0xe4}}
with the straddle overlap; a hit stamps `entry_position + TAIL[len]`
({0xa8→0x46, 0xb2→0x3c, 0xe4→0x32}), clamped by the row bound + device
end; no entry → the walked bound (the og's own cap constant 0x3200000 or
the device end, whichever clamps) at the reduced confidence class. The
row bound (both the II and MM rows) now carries the og walker's OWN cap
constant (0x3200000 = 50 MiB — the 64 MiB DWG-pattern stand-in value is
gone from these rows). The og's streaming artifacts (the chunk-tail
gate, the backup-80 re-probe) are the same registered model-divergence
class as X3F: the unconditional windowed search is what they accumulate
to (the backup-80's markers are found by the unconditional search
anyway, at a MORE precise extent).

## 4. Open questions (honest)

(a) The CIFF record type 0x2807's NAME in the og's type table (the
table maps the three lengths to tail constants; the semantic of what a
0x2807/0xa8 record IS — likely a props-family record — was not traced;
the og's own tail constants are opaque small values 0x46/0x3c/0x32).
(b) Whether the walker's map carries MORE entries than the three (the
full map build was read to 0x18005234b; the three inserts are what the
walk section consumes — additional entries would be unreachable from
the six-byte signature filter anyway).
