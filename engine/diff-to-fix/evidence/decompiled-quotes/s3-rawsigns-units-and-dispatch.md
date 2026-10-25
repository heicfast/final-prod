# Evidence — RAWSearchDLL raw_signs.json semantics (session 3 decode)

All quotes from `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(corpus `03ad46c`), plus disassembly of `og-codebase/RAWSearchDLL.dll` (x64,
imagebase 0x180000000, .text VA 0x180001000 / file 0x400). Every claim below was
read first-hand this session; addresses are VA.

## 1. raw_signs.json is loaded by RAWSearchDLL at DLL-scope lazy init

- String `"raw_signs.json"` occurs exactly once in the binary (RAWSearchDLL.dll
  strings census; decompile :23836).
- Loader `FUN_18001ea40` (:23765-24190): `std::ifstream` seekg/tellg whole-file
  read → hand-rolled JSON tokenizer → per-extension map entry + header/tail sign
  vectors. Loader is invoked from the one-time init thunk at **VA 0x180022b90**
  (verified by E8-rel32 scan: single direct call at 0x180022bbe → 0x18001ea40),
  which is registered with `std::_Execute_once` by the lazy accessor
  `FUN_18001fae0` (:24533, once_flag at DAT_1801183a8). Accessor returns
  `{&DAT_180118380 (ext→{min,max} map), &DAT_180118358 (header index),
  &DAT_180118328 (tail index)}` (:24610-24612).

## 2. min/max JSON values are stored, and consumed as KiB (×1024)

### 2a. Key strings (binary .rdata)

- `0x18006b048` = `"max\0"`; `0x18006b04c` = `"min\0"` (dumped first-hand).
- Loader `FUN_18001ea40` looks up `"min"` first (:23971 region,
  `FUN_18005e140` picojson get), then `"max"` (:24047 region), accepts int
  (json type 3) or double (type 4, truncated), and inserts the pair
  `{min@+0, max@+8}` into the per-extension map via `FUN_180022590`
  (:24100-24116: `local_358 = lVar7 /*min*/; ... local_350 = lVar13 /*max*/`).

### 2b. Consumer: the CommonRawAnalyser probe scales by 1024

`FUN_1800205b0` (:25020, `RawFormat_xjq::CommonRawAnalyser::CommonRawFileInfo`
— class name from the adjacent dtor `FUN_180020500` :24992 which writes
`RawFormat_xjq::CommonRawAnalyser::CommonRawFileInfo::vftable`):

```c
/* :25200 region — the matched format's max: */
lVar19 = *(longlong *)(*(longlong *)(param_1 + 0x118) + 8);   /* fmt->max */
lVar17 = lVar19;
if (lVar19 == 0) {
    lVar17 = 0x7d000;              /* default max = 512000 KiB = 500 MiB */
}
if ((ulonglong)(((param_4 - *(longlong *)(param_1 + 8)) + 1) * (longlong)iVar11)
    < (ulonglong)(lVar17 * 0x400)) {     /* byte-span < max * 1024 */
    return 2;                        /* need more data before accepting */
}
lVar19 = lVar19 << 10;              /* size = max KiB -> bytes */
*(longlong *)(param_1 + 0x10) = lVar19;
```

- `iVar11 = param_3` is the device sector size (see scan loop below), so
  `(last_sector - first_sector + 1) * sector_size` is a byte span, compared to
  `max * 0x400` (0x400 = 1024). **The JSON max is a KiB count.** The same
  branch appears twice for the `param_5+0x14` (sector-shift) `< 8` and `> 8`
  device classes (:25204-25240).
- Emitted candidate size for a header-only hit = `max << 10` bytes
  (`*(param_1 + 0x10)`, the probe's size result field).
- Consequence for raw_signs.json values: `max=51200` → **50 MiB** (not 50 KiB);
  `max=204800` → **200 MiB** (VOB/IFO/cpi/lrv class); `min=1` → 1 KiB floor;
  `min=10240` → 10 MiB.

## 3. The per-sign confidence field is never consumed

Sign-string parser `FUN_18001e510` (:23522-23765):

- `FUN_180021570` (:25885) splits the sign string on `,` — **full split**
  (loop `FUN_18000f820` find-next until miss; the `{&DAT_18006b014, 1}` operand
  is {delimiter ",", delimiter length 1}, not a max-split). `"0,44564456,0.96"`
  yields 3 parts.
- The parser requires `>= 2` parts (else returns, :23579), parses **part[0]**
  as the decimal offset (digit-table loop, :23599-23618 → `local_58`), and
  consumes **part[1]** only, as the hex body (`local_f0._0_8_ + 0x10` :23630,
  hex-decoded via `FUN_18001d930`+`FUN_1800071e0` into the byte vector).
- **No access to part[2] (`local_f0._0_8_ + 0x20`) exists anywhere in the
  function** (verified by full read). The confidence decimal in
  raw_signs.json is parsed-as-split-output and dropped.

## 4. Scan loop: sector-stride probing, all-zero-sector skip, dispatch order

`FUN_18000c820` (CRAWSearchController vtable +0x48; :9385-9633):

- Advances `local_1e0` one **sector index** per iteration; sector size comes
  from the device (`iVar8`, device vtable+5, default 0x200; CellTest +0xc)
  (:9490-9520). Sector pointer: `iVar8 * local_1e0 + (param_1[0x36]>>1) +
  *(param_1+0x38)`.
- **All-zero-sector metrics**: leading zero dwords (`if (*puVar19 != 0)` else
  count 4; backward dword walk from `param_1[0x3c] - 4`, 0x1c bytes / 7 dwords)
  (:9525-9540) — feeds a fast-skip for zero sectors.
- Dispatch per sector: `FUN_18000cf60(this, sector, buf, ptr, &out)`
  (:9547 = VA 0x18000cadc, verified by disassembly:
  `call 0x18000cf60; mov rsi,rax; test rax,rax; je 0x18000ccfc`).
  On NULL → **`FUN_18000d670`** (:10068) = the raw_signs sign-DB fallback
  (consults the DAT_180118380/358/328 bundle). On non-NULL → analyser
  vtable+0x38 builds the result (VA 0x18000caf0-0x18000cb34).

`FUN_18000cf60` (dispatch, :9730-9930):

1. Reads `config/config.ini` section **`User`**, key **`only_common`**
   (`0x18006b048` region strings; `FUN_180007890` ini getter, once-flag at
   DAT_180118208) → `DAT_180118204`.
2. If not only_common and **first u32 != 0**: **u32 4CC registry first** —
   `std::map` RB-descent exact match on the first 4 bytes
   (:9778-9800, `*(uint*)(node+4) < uVar2` descent,
   lower_bound-miss check `uVar2 < key`). On hit: analyser vt+0x40 fast
   validator, else vt+0x10 main Analyse; acceptance recorded via
   `FUN_18000d920`.
3. Then **u16 registry** `FUN_18000d3e0` (:9933, `*(ushort*)(node+4)` descent
   on the first 2 bytes, skipped when first u32 == 0).
4. Then (both missed, or only_common mode): **linear iteration of the whole
   analyser registry** (:9898-9928) trying each analyser.
5. If everything misses → caller falls to `FUN_18000d670` = the
   **raw_signs.json layer, last**.

The prior corpus artifact's "[INFERRED]" order (u16 first, then 4CC, then
signs) is **inverted on the first two legs** — see CORPUS-ERRATA (our copy)
entry 19.

## 5. Core-side citations (audited commit 34b3441)

- `scripts/gen_rawsigns.py` :8 comment `"min": <min bytes>, "max": <max bytes>`
  (byte-unit assumption), :27 `int(round(float(conf) * 1000))` (confidence
  scaled and kept).
- `src/core/carve/cr_rawsigns_data.cpp` :12-14 `kFormatCount = 139`;
  `kFormats` entries `{"a", 1ull, 51200ull}` (min=1, max=51200 **bytes**).
- `src/core/carve/cr_carve.cpp` :186-301 raw-sign layer: 512-stride probe
  (:192 `off += 512`), `len = fmt.max_size` (:207, bytes), tail window
  `[start+min, start+max)` bytes with a 2 MiB bound (:211-228), confidence
  blend `(hdr + tail)/2 + 100` cap 1000 (:222), `carve_format` tag
  `0xFFFF0000u + fmt` (:272).
- `include/cr/carve/cr_rawsigns.h` :11-20 — the layer's own header states
  "documented divergences from the og, whose exact hit handler was not RE'd to
  completion". This session RE'd it; the divergences are now findings
  S-015 / N-030 / N-031.
