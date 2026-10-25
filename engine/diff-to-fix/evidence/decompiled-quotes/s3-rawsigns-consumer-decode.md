# S3 evidence — raw_signs.json loader + matcher + consumer decode (RAWSearchDLL)

All quotes from `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(cited as `Dec:LINE`), cross-verified against `og-codebase/RAWSearchDLL.dll`
(string/data extraction, probe: `python3 scripts/extract_dat_strings.py`-style PE RVA dump)
and `og-codebase/raw_signs.json` (md5 `bd6aef646edff8eadd62397f5e794c29`, identical to
`BUILDPE/EaseUS-x64/raw_signs.json`). Decoder: main session-3 auditor, 2026-10-26.

## 1. JSON schema (artifact + loader agree)

```json
{ "<ext>": { "header": { "signs": ["<offset>,<hexbytes>,<confidence>", ...] },
             "tail":   { "signs": [...] },
             "min": <int>, "max": <int> } }
```
- 139 top-level extensions; 199 header sign strings; 62 tail sign strings;
  46 formats carry tails, 93 are header-only (mechanical count, this session).
- Field-name constants extracted from the binary at `.rdata`:
  - RVA 0x6b048 = `"max"`, RVA 0x6b04c = `"min"` (both looked up per entry,
    `FUN_18005e140(entry, name)` at Dec:24011 / Dec:24064)
  - RVA 0x6b040 vicinity: `"signs"`, `"count"`, `"tail"`, `"head"` (the
    `"tail"`/`"header"` literals also appear inline at Dec:24266-24268)
  - RVA 0x6b014 = `","` — the sign-string CSV delimiter (Dec:23613)
  - RVA 0x6b030 = `"raw_signs.json"` (the ifstream name, Dec:23836)

## 2. Loader `FUN_18001ea40` (Dec:23765-24227)

- Opens `ifstream("raw_signs.json")` (Dec:23836), reads whole file, parses with
  a statically-linked jsoncpp (`FUN_180057b80` = Reader::parse; member lookup
  `FUN_18005e140`; type tags: 3=int, 4=real, 6=array, 7=object — Dec:24013-24020,
  Dec:24324, Dec:23902/24002).
- Iterates the ROOT object's 139 members (ext names). Per member:
  1. looks up `"min"` and `"max"`, accepts int (tag 3) or real (tag 4)
     (Dec:24011-24021, Dec:24064-24075);
  2. `FUN_18001f460(entry, vec, is_header)` parses `entry["header"]["signs"]`
     and `entry["tail"]["signs"]` (string literals `"header"`/`"tail"` at
     Dec:24266-24268, `"signs"` at Dec:24305-24315; array iteration
     Dec:24342-24380, 0x48-byte sign elements);
  3. stores three maps into the out-manager: minmax map at +0x00
     (`FUN_180025710`, Dec:24172), TAIL-sign map at +0x10 (`FUN_1800255c0`,
     Dec:24175), HEADER-sign map at +0x20 (`FUN_1800255c0`, Dec:24178).

## 3. Sign-string parser `FUN_18001e510` (Dec:23522-23702)

- Strips JSON quotes (Dec:23592-23598), splits on `","` (Dec:23613-23615;
  **requires >= 2 parts**, else bails).
- Part[0] = decimal **offset** (char->digit table at RVA 0x6ae70, accumulate
  `uVar12*10+digit`, Dec:23633-23648; stored `local_58`).
- Part[1] = **hex bytes** (Dec:23657-23674), decoded by `FUN_18001d930`
  (Dec:22973-23068): toupper pairs, `'0'-'9'`/`'A'-'F'`, any invalid nibble ->
  empty result (Dec:23031-23039); result stored as a std::string of raw bytes
  (Dec:23680).
- **Part[2] (the "0.54"/"1.00" confidence) is NEVER READ.** The only element
  accesses are part[0] (Dec:23633) and part[1] (Dec:23657). The og DROPS the
  per-sign confidence at load time.
- Sign struct (0x48 bytes, copied by `FUN_18001e970` Dec:23718-23760):
  `{ std::string @+0x00 (ext name, filled by the vector wrapper), std::string
  @+0x20 (raw sign bytes), u64 @+0x40 (offset) }` — **no confidence field
  exists in the og's in-memory sign**.

## 4. Header matcher `FUN_18001fd30` (Dec:24621-24771)

- Thread-safe singleton globals (`FUN_18001fae0` Dec:24533-24614 exposes
  `&DAT_180118380` (minmax), `&DAT_180118358`, `&DAT_180118328`).
- Iterates the global header-sign vector (0x48 stride, Dec:24766):
  `pauVar19 = sign.offset + buffer` (Dec:24727) — **offset is relative to the
  candidate start**; compare = `memcmp` for len < 0x11 (Dec:24729), AVX2
  `vpcmpeqb/vptest` for longer (Dec:24735-24763). **First match wins**
  (returns `puVar16`, Dec:24731).

## 5. Consumer = `RawFormat_xjq::CommonRawAnalyser` (format id 0xa8)

- The raw-signs layer is an ANALYSER in the engine's LIST dispatch tier
  (`RawFormat_xjq::CommonRawAnalyser::vftable` Dec:24908; constructed in the
  list builder). It runs AFTER the u32-key and u16-key tiers (see
  s3-rawsearch-registry.md).
- **Shallow validate** `FUN_180020330` (Dec:24920-24967): on header-sign hit
  sets context format `0xa8` (Dec:24943) and the name from the matched ext
  (`FUN_180017550(ctx, 9, ext)` Dec:24957), returns 1 (accept).
- **Deep parse** `FUN_1800205b0` (Dec:25020-25340): incremental per-sector
  protocol with a three-valued return: **1 = done (size set), 2 = keep
  scanning (next sector), 0 = reject**. Decoded arithmetic:
  - Caches the matched sign (+0x110) and ext entry (+0x118) (Dec:25063-25121).
  - `min`/`max` are **KiB units**: every comparison converts with `<< 10` or
    `* 0x400` (both = x1024):
    - `if (scanned_bytes < min << 10) -> size 0 (reject)` (Dec:25176-25181,
      same gate on the tail-hit path Dec:25266-25273)
    - `limit = (max != 0) ? max : 0x7d000` KiB — **default 500 MiB when max
      is 0** (Dec:25187-25196; identical at Dec:25232-25241, Dec:25318-25329);
      `if (scanned_bytes < limit * 0x400) return 2;` (keep scanning)
    - exceeded limit without tail -> size 0 (reject) (Dec:25196-25202)
  - Tail scan (only when the ext has tail signs): scans onward **from the
    `min` bound** (`uVar20 = min + sector_size` when not at device end,
    Dec:25204-25230), per-stride over the ext's tail-sign list
    (Dec:25250-25315), memcmp/AVX2 like the header matcher.
  - **Tail-hit size formula** (Dec:25266-25267):
    `size = sector_size * (cur_sector - start_sector) - tail_sign_offset + 50`
    (literal `0x32` = 50 constant in the decompile), then the `min << 10`
    floor gate applies again (Dec:25269-25272).

## 6. Verdict highlights (vs our `cr_rawsigns`)

| semantic | og (evidence) | ours | verdict |
|---|---|---|---|
| sign count | 199 header / 62 tail / 139 formats | same (cr_rawsigns_data.cpp) | MATCH |
| header match | memcmp(buf+off, bytes, len), first wins | probe_header buckets, all matches returned | MATCH (we return all, pick best-conf — superset) |
| min/max units | **KiB** (<<10 / *0x400 everywhere) | raw JSON values as BYTES | **DIVERGENCE (1024x)** |
| default max | 0x7d000 KiB = 500 MiB when max=0 | none (no max=0 entries — latent) | INFO |
| confidence | **dropped at load** (never parsed) | conf 0..1000, best-of, (h+t)/2+100 cap 1000 | DIVERGENCE (invented) |
| tail-hit size | `sec*(cur-start) - tail_off + 50` | `lo + th.pos + 512` | DIVERGENCE (formula) |
| tail window | from min to max KiB (up to 200 MiB), incremental | `[start+min, start+max)` capped **2 MiB** | DIVERGENCE (cap invented) |
| placement | list-tier analyser after u32/u16 tiers | after grammar hits per stride | MATCH (equivalent layering) |

---

## Integration note (2026-10-26)

This file is the *storage/census parallel leg's* raw-signs decode (baseline `7fd00e0`).
It complements the S3 lead leg's `s3-rawsigns-units-and-dispatch.md` (which landed
first and carries the units fix S-015 + the dispatch tiers). Unique content here:
the loader/matcher/sign-struct decode (§2-4), the CommonRawAnalyser consumer
placement (§5), and the tail-hit size formula `sec*(cur-start) - tail_off + 50`
(:25266-25267) — folded into N-030's addendum. The confidence-dead-field discovery
was made independently by both legs (N-030).
