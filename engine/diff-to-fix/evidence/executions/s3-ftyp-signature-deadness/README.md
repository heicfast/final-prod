# S3-CARVE executed probe — the ftyp-family signatures are unreachable (S-012 evidence)

**Probe:** compile the REAL core carve sources (`cr_carve.cpp`, `cr_registry.cpp`,
`cr_analysers.cpp`, `cr_rawsigns.cpp`, `cr_rawsigns_data.cpp` + tree/crypto/parallel)
from `core_recovery @ 34b3441` and run `carve::carve_slice` on synthetic slices.

**Build command (g++ 14, -std=c++20 -O1):**

```
g++ -std=c++20 -O1 -g -I <core>/include -I <core>/src probe2.cpp \
    <core>/src/core/carve/{cr_carve,cr_registry,cr_analysers,cr_rawsigns,cr_rawsigns_data}.cpp \
    <core>/src/core/tree/{cr_node,cr_diskmap,cr_snapshot}.cpp \
    <core>/src/core/crypto/cr_sha256.cpp <core>/src/core/cr_parallel.cpp \
    -o probe2 -lpthread
```

**Fixtures planted in a 6 MiB zeroed slice (all at 512-aligned offsets):**

| offset | bytes | intent |
|---|---|---|
| 512 | grammar-valid JPEG (test_carve.cpp `plant_jpeg` shape, 521 B) | control: machinery works |
| 1 MiB | `00 00 00 18 "ftyp" "isom" 00 00 02 00 "isom" "iso2"` + `free` + `moov` boxes | real-world MP4 head |
| 2 MiB | `00 00 00 14 "ftyp" "qt  "` + `free` | real-world QuickTime MOV head |
| 3 MiB | `00 00 00 18 "ftyp" "heic" 00 00 00 00 "mif1" "heic"` | real-world HEIC head |
| 4 MiB | `00 00 00 20 "ftyp" "M4A " 00 00 00 00 "M4A " "mp42"` + `free` | real-world M4A head |
| 5 MiB | `00 00 00 00 "ftyp" "heic"` | degenerate size-0 (to-EOF) ftyp box |

## Result (`probe2-output.txt`)

```
rc=0 hits=5 validated=1 rejected=4
node[0] name=File000000.jpg fmt=0x0 conf=900 size=521 ext=512
node[1] name=File000001.lrf fmt=0xffff0036 conf=1000 size=51200 ext=1048576
node[2] name=File000002.lrf fmt=0xffff0036 conf=1000 size=51200 ext=2097152
node[3] name=File000003.lrf fmt=0xffff0036 conf=1000 size=51200 ext=3145728
node[4] name=File000004.lrf fmt=0xffff0036 conf=1000 size=51200 ext=4194304
node[5] name=File000005.lrf fmt=0xffff0036 conf=1000 size=51200 ext=5242880
total=6
```

## Decoded reading

1. **Control JPEG: carved** (`fmt=0x0` = FMT_JPG, conf 900, size 521, extent 512) —
   the grammar layer and harness demonstrably work.
2. **The four real-world BMFF files (MP4/MOV/HEIC/M4A) produced ZERO grammar hits.**
   `hits=5` = 1 JPEG + 4 ftyp-family signatures matching ONLY the degenerate
   size-0 box at 5 MiB. Reason: `cr_registry.cpp:112-131` keys MP4/MOV/M4A/HEIC on
   `{0,0,0,0,'f','t','y','p'}` — `cr_carve.cpp:89` memcmps all 8 bytes, so a real
   box (size = 0x14/0x18/0x20, i.e. bytes `00 00 00 xx`) can never match; only a
   size-0 (to-EOF) box matches.
3. **Even the degenerate size-0 box (hits 2–5) was REJECTED by every validator**
   (`rejected=4`, `validated=1`): `cr_carve.cpp:94` computes the candidate start as
   `match_pos − header_offset` (= `match_pos − 4`), so the validators
   (`cr_analysers.cpp:436-448 bmff_walk`) read the brand at `cand_abs + 8` = the
   "ftyp" bytes themselves → no brand list (`isom/mp42/...`, `qt  `,
   `heic/heix/mif1/msf1`, `M4A /M4B /M4P `) can ever match → all four reject.
   **Conclusion: FMT_MP4/FMT_MOV/FMT_M4A/FMT_HEIC are dead code — no ISO-BMFF file
   can ever be grammar-carved by the core.**
4. **All BMFF positions were instead claimed by the raw-sign layer as `.lrf`**
   (`fmt=0xffff0036` = raw-sign format 0x36 = "lrf", 51,200-byte cap, conf 1000):
   the og's own `raw_signs.json` contains a "lrf" header sign
   `offset=4, bytes="ftyp", confidence=1000` (mirrored faithfully in
   `cr_rawsigns_data.cpp` kHeaderSigns) — so BOTH the og and the core label
   ftyp-box files ".lrf" at the sign layer. The og recovers because
   `BruteForce::MovAnalyzer` / `RawFormat_xjq::HEICAnalyser` (list layer) claim
   the position first; the core has no working equivalent (S-012).

## Why the unit suite never caught it

`grep -c "FMT_MOV\|FMT_MP4\|FMT_HEIC\|FMT_M4A" tests/unit/test_carve.cpp` → **0**:
no carve test exercises any BMFF fixture. The only ftyp fixture in the repo is
`tests/unit/test_preview.cpp:374-378` (preview path, not the carve registry). The
bench seed DOES plant real MP4/MOV/HEIC heads
(`bench/seed/generate_seed.ps1:51,57,58`), so bench recall for these three types
reflects the failure (they can only surface as raw-sign `.lrf`/false-name claims).

## Secondary observation (probe1, malformed-JPEG control)

The first probe planted a deliberately malformed JPEG (SOF0 with 1 declared
component but 3 component entries): `rejected` — the grammar layer correctly
refused it, and the raw-sign layer still claimed the position as `.jfif`
(raw-sign format 0x2f = "jfif", `FFD8FFE0 0010 JFIF` sign, conf 780). See
`probe1-output.txt`. This is the known B3 raw-sign-unvalidated surface
(N-008/M-005 already track it).
