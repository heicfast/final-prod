# S88-L1 — the WPD "\xFFWPC" version-table + declared-size tier-1, decoded first-hand from the raw asm (M-107's WPD member, member 6 of 11)

**Session-88 fix lane (2026-10-30).** Method: `objdump -d` over
`og-codebase/RAWSearchDLL.dll` (corpus `be0cfe9`); vtables and the
version table read byte-exact from .rdata via pefile. Every address
below is an instruction address against the corpus binary itself.

## 1. The registry chain (byte-exact)

The registration cascade is compiler-pipelined: the node created in
block N is inserted for the key of block N+1 (calibrated against two
known anchors this session: the FOVb/X3F binding and the II/CRW
binding — both re-derived independently and matching the S83 sheets).

- Key `"PK\x03\x04"` (0x04034b50, block at 0x180003c7f): creates node
  N1 = {vtable **0x18006cff0**, idx **0x44**} (vtable store at
  0x180003cbc, idx store at 0x180003cbf).
- Key `"\xFFWPC"` (0x435057ff, block at 0x180003ccb): the map insert at
  0x180003cde stores N1 — **map["\xFFWPC"] = the WPD node
  (vtable 0x18006cff0, chain idx 0x44)**.
- The following keys 0x00090001 (block at 0x180003d17) and 0x9ac6cdd7
  (block at 0x180003d5c) map to a DIFFERENT node class
  {vtable 0x18006b200, idx 0x46} — NOT WPD siblings (see §4).

The node vtable 0x18006cff0:

| slot | value | meaning |
|---|---|---|
| +0x00 | 0x180041f00 | dtor |
| +0x08 | 0x18000cf30 | (shared) |
| **+0x10** | **0x180041f50** | **the tier-1 grammar — the size authority (this decode)** |
| +0x38 | 0x18000ec90 | the factory (new(0x110) + the generic ctor 0x18000b520) |
| +0x40 | 0x18000bae0 | **the return-0 stub** (`xor eax,eax; ret` — verified) — no GetFallback override, no secondary consult |

## 2. The tier-1 (FUN_180041f50, 0x180041f50-0x18004202d)

```
vmovdqu 0x2cfd5(%rip),%ymm0   # .rdata 0x18006ef40 -> rsp[0..31]
movl   $0x3d,0x20(%rsp)       # entry 3's id literal -> rsp[0x20]
loop (3 unrolled groups, rax = rsp+4, +0x24/group):
    group g compares  cand.u32@8   == table[g].A      (0x180041f8a / fa8 / fc7)
                     cand.u32@0xe  == table[g].B      (0x180041f93 / fb1 / fd0)
    on match:  matched = 1;  result.id = table[g].id  (0x180041fa0 / fbf / fde)
if (!matched) return 0                               (0x180041ff5)
result.size = cand.u32@0x14                          (0x18004200c-0x180042015)
return 1
```

The version table (bytes at .rdata 0x18006ef40, read via pefile):

| entry | A = u32@8 (LE) | A bytes | B = u32@0xe (LE) | B bytes | id |
|---|---|---|---|---|---|
| 1 | 0x01020a01 | 01 0a 02 01 | 0x00050200 | 00 02 05 00 | 0x44 |
| 2 | 0x00021601 | 01 16 02 00 | 0x00030000 | 00 03 00 00 | 0x45 |
| 3 | 0x00010f0f | 0f 0f 01 00 | 0x00030000 | 00 03 00 00 | 0x3d (the rsp+0x20 literal) |

(Entries 1-2's ids come from the .rdata load; entry 3's id is the
`movl $0x3d` immediate at 0x180041f6e — the 32-byte ymm load covers
only A3/B3, the id is code, not data.)

**The size: u32@0x14 verbatim.** The og never walks the object header
chain for the "\xFFWPC" family — the M-107 filing's "walks the object
header chain" was the audit's hypothesis; the decode shows a direct
header-field extent. The S-007-era core's "product-version word at +4"
gate exists nowhere in the og's chain: the version PAIR at (+8, +0xe)
is the gate, and the old core's `h[4]==0 && h[5]==0` refusal was an
invention.

## 3. The zero-size path (the decline)

The scan loop's post-accept flow (the caller at 0x18000cad0-0x18000cc05,
first-hand): the factory (+0x38 → 0x18000ec90 → `new(0x110)` + ctor
0x18000b520) creates the generic pending child (vtable 0x180066e00);
`child+0x10 = result.size` (0x18000cb15-0x18000cb19). The child's size
!= 0 → direct emit (0x18000cbb5-0x18000cbd0). Size == 0 → the child's
walk slot +0x8 = **0x18000b760 = `xor al,al; ret`** — the generic child
never walks, so a "\xFFWPC" with u32@0x14 == 0 does not carve at all.

## 4. The sibling keys are NOT WPD (the census grouping corrected)

The census (`rawsearchdll--analyser-registry-census.md` §3) groups
"\xFFWPC / D7 CD C6 9A → FileWPDAnalyser (WordPerfect WPG/doc)". The
node-level trace refutes the second half of that grouping: both
0x00090001 and 0x9ac6cdd7 map to node {vtable 0x18006b200, idx 0x46},
whose tier-1 (FUN_1800264d0, decoded first-hand this session) is the
**WMF grammar** — and it matches the core's already-landed S-042
validators byte-for-byte:

- 0x9ac6cdd7 form (the Aldus placeable WMF): HMF(u16@4)==0, the XOR
  checksum over u16 words 0..9 == u16@0x14, reserved(u32@0x10)==0, the
  standard header's first u32@0x16 == 0x00090001, params(u16@0x26)==0,
  extent = 0x16 + 2 × u32@0x1c — EXACTLY `validate_wmf_placeable`.
- 0x00090001 form (the bare standard WMF: type=1 + headersize=9 as the
  KEY): params(u16@0x10)==0, extent = 2 × u32@6 — EXACTLY
  `validate_wmf_std`.

The census's "01 00 09 00 → Windows thumbnail cache" label is likewise
wrong — 0x00090001 is the WMF type/headersize pair. (The census row is
an inference from byte-adjacent construction, the class of erratum the
S83 lane already recorded once for the dispatch order.) This
cross-validation doubles as an independent re-verification of S-042's
landing: my fresh decode of FUN_1800264d0 confirms every gate the
S-042 landing shipped.

## 5. The landing (the core's single-shot model)

`validate_wpd` gates on the og's three-entry version-pair table,
requires u32@0x14 != 0 (the generic-child decline), and takes the
extent = the declared u32@0x14 clamped by the row bound + device end
(probe_limit — the X3F/CRW registered approximation posture for the
og's controller-side cap). The row max stays the registry's sanity
clamp (the og's own chain-row caps for ids 0x44/0x45/0x3d are
undecoded — the same posture the X3F/CRW/DWG landings registered for
their cap fields). The og's three chain ids (0x44/0x45/0x3d) share the
core's single FMT_WPD row — the id distinction selects og-side
FileInfo rows only.

## 6. Open questions (honest)

(a) The semantic identity of the three version pairs (which Corel
release each row serves) was not traced — the og's table is ported
verbatim.
(b) The og chain-row min/max caps for ids 0x44/0x45/0x3d remain
undecoded; the row-bound clamp rides the approximation posture.
(c) Whether the og's controller applies any additional cap to the
declared size beyond the chain row (the pending-object machinery's
consumer) — the clamp posture is the registered family default.
