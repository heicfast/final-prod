# S85-L1: the DWG three-generation size walk — raw-asm decode (M-107 member 3 of 11)

Session-85 (2026-10-30). First-hand decode against the corpus binary
(`og-codebase/RAWSearchDLL.dll`, objdump; the decompile
`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` used for
cross-reading). Every instruction address cited below was read this session.

## The chain (the M-156 dispatch pattern)

1. **Registry binding** — `0x1800036bf`: `movl $0x30314341,0x48(%rbp)` ("AC10")
   → `call 0x180006c80` (the registry insert) → the node bound at `0x1800036b3`:
   vtable `0x18006bc00`, idx **0x18** (`movl $0x18,0x8(%rbx)` at 0x1800036b3).
2. **Tier-1 gate** (node+0x10 = `FUN_18002fe30`): the NINE-version table,
   built at `:36766-36794` — AC1032, AC1027, AC1024, AC1021, AC1018, AC1015,
   AC1014, AC1013, AC1012 (the 6-byte-stride compare loop). On match:
   `*(param_3 + 0x18) = 0x18` (the result id) — **no extent**.
3. **Factory** (node+0x38 = `FUN_18002ff20`): allocates 0x508 bytes, calls the
   base ctor `FUN_18000b520` (**extent field +0x10 = 0**, +0x1c = 0xffff), sets
   vtable `0x18006bc50` (BruteForce::CDWGRawInfo), then the walk state:
   - `0x18002ff59`: `movq $0x15,0x110(%rbx)` — **the section-locator cursor
     starts at file offset 0x15 (21)**
   - `0x18002ff66`: +0x118 (the record count) = 0
   - `0x18002ff6c`: +0x11c (the count-read flag) = 0
   - `0x18002ff72`: `movq $0x3d8,0x120(%rbx)` — gen-3's expected accumulate = 0x3d8
   - `0x18002ff7d`: +0x500 (the generation) = 0; `memset(+0x128, 0, 0x3d8)`
4. **No secondary** — node+0x40 = `0x18000bae0` = `xor eax,eax; ret` (the
   return-0 stub; byte-exact at 0x18000bae0-0x18000bae2). The child walk is
   the SOLE size authority (the CRW/M-090 pattern).
5. **The walk** (child+0x20 = `FUN_18002f320`, the chunk processor).

## Generation 1 — AC1012/AC1013/AC1014/AC1015 (R13/R14/R2000)

The version table at `:36331-36346` (the four shorts "12","13","14","15");
generation stamp `*(param_1+0x500) = 1`.

The section-locator walk (`:36356-36403`):

- First feed: `count = u32 @ cursor(0x15)`; cursor += 4 → **records start at
  0x19**; the count is remembered (+0x118), the read-flag set (+0x11c).
- Each record is **9 bytes**: `number:u8 @ +0`, `seeker:u32 @ +1`,
  `size:u32 @ +5` (the DWG R13-R2000 section-locator record layout).
- **`record.number != record_index` → extent = 0 (the decline)** — the walk's
  only structural gate, `:36379-36383`.
- `extent = max(extent, seeker + size)` — the running max, `:36384-36386`
  (`uVar2 = (uint)(*(int*)(rec+5) + *(int*)(rec+1))`, clamped up by the
  previous extent).
- cursor += 9 per record; the loop runs `index < count`; a chunk boundary
  carries the cursor (`:36396-36400`).
- The extent starts at **0** (the base ctor) — nothing walked → 0.

## Generation 2 — AC1018/AC1024/AC1027/AC1032 (R2004/R2010/R2013/R2018)

The version table at `:36394-36408` (shorts "18","24","27","32"); generation
stamp 2. The path `:36419-36432`:

- `FUN_18002f630` decodes the **0x6c-byte struct at file offset 0x20** by
  XOR with the MSVC rand() LCG stream, seed 1: `state = state*0x343fd +
  0x269ec3`, byte = `(state >> 16) & 0xff`, 0x6c bytes (`:36481-36597` —
  the unrolled generation loop; the first store is after one LCG step).
- The gates on the DECODED struct (the caller's stack locals alias the
  buffer): `u32 @ 0x0b == 0`, `u32 @ 0x10 == 0x6c`, `u32 @ 0x14 == 4`,
  `u32 @ 0x44 == 0x20`, `u32 @ 0x48 == 0x80`, `u32 @ 0x4c == 0x40`.
- All pass → **extent = u64 @ 0x34 + 0x6c**. Any fail → the extent is left
  untouched (0 from the ctor — the decline class).

## Generation 3 — AC1021 (R2007) — the honest residue

`*:36408-36417` — "AC10" + 0x3132 ("21"); generation 3. The path accumulates
0x3d8 bytes from offset 0x20 into the +0x128 buffer (the chunk-carry at
`:36403-36416`), then `FUN_18002fc50` (`:36645+`): copies the 0x3d8 buffer,
**de-interleaves 3 phases x 0xef bytes stride-3** into a 0x2cd (717) buffer,
reads a count at +0x18 of the deshuffled data, and either copies 0x22 bytes
directly (count < 1) or runs `FUN_18002ef50` (the section-record chain —
the 0x20-family markers, the 0x40 record cap, `:36088+`); the extent =
the parsed struct's qword at +8 (`*(param_1 + 0x10) = uStack_500`,
`:36733`). **FUN_18002ef50's record chain (and its helper FUN_18002eb10)
are NOT decoded to landing quality this session** — the gen-3 (R2007-only)
member keeps its bounded extent, recorded as M-107's honest remainder
(the finding's fix-plan point 1(c) sanctions exactly this split).

## The walk cap (all generations)

`0x18002f5e9-0x18002f606`:
```asm
call  0x180001350            ; the singleton (object at 0x1800939a0)
imul  $0x64,0x70(%rax),%rax  ; cap = [singleton+0x70] * 100
cmp   %rax,0x10(%rbx)
jbe   done                   ; extent <= cap → keep
movq  $0x0,0x10(%rbx)        ; extent > cap → 0 (the decline)
```
`[0x180093a10]` (the singleton's +0x70) = **0xbebc200 = 200,000,000** (the
.data initial value, read this session) → **the DWG walk cap = 20 GB**
(0x4A817C800). The same singleton field, halved, is X3F's 100 MB cap (the
M-156 corroboration); DWG multiplies by 100.

## Discovered during verification (new defects, fixed in this landing)

The core's pre-fix tier-1 table (`kVersions`) was `{"12","14","15","18",
"21","24","27","30","32"}` — it carries **"30" (AC1030 — not a real DWG
release; no such version exists in the og's table or the format's history)
and lacks "13" (AC1013 = R13 — real, in the og's table)**. The og's exact
nine: {12,13,14,15,18,21,24,27,32}. Also: the core's extra sentinel check
(0D 0A 0D 0A 00) does not exist anywhere in the og's chain (the tier-1 is
the version table only) — dropped for exactness.

## The landing shape

`validate_dwg` routes by version: gen-1 → the section-locator walk (count
u32@0x15, 9-byte records from 0x19, number==index or decline, extent =
max(seeker+size), torn → the walked-prefix salvage at reduced confidence);
gen-2 → the rand-XOR struct (the exact LCG, the six gates, extent =
u64@0x34 + 0x6c; torn/unreadable → decline); gen-3 → the bounded extent
(retained, the recorded remainder). The row max carries the og walk's own
cap constant (0x4A817C800 = 200 MB x 100). The registry min stays 64.
