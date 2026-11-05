# s10-rsf-ground-truth.md — EXECUTED: first-hand parse of the og's three real SaveScan .rsf files

**Session 10 (2026-10-29). Corpus `be0cfe9`. Read-only probe
(`/home/z/my-project/scripts/rsf-probe.py`) over
`og-codebase/SaveScan/auto_Local Disk (G_)10_15_00_44_10.rsf` (7,366,685 B),
`auto_Local Disk (H_)10_14_23_42_58.rsf` (52,744,004 B),
`auto_Local Disk (I_)10_14_23_13_21.rsf`.**

## Why this probe was run

The five SaveScan artifacts (rsf-format, record-4 grammar, ifileproperties parts 1-3) are
cited throughout the trail, and the corpus's own `scripts/rsf_parser.py` parses "all
samples" — but the trail had no execution record of the three REAL session files shipped in
the og tree (the artifacts' samples and the real files were never cross-checked in an
evidence file). This probe closes that gap: it hexdumps the header, decodes the magic GUID,
and censuses the zlib streams.

## Results (all three files agree)

```
magic[0:16] = EA F4 B3 1B 05 43 B6 4A B4 B8 24 4A 33 D9 13 BB
magic guid  = 1BB3F4EA-4305-4AB6-B4B8-244A33D913BB
+0x10       = 43 1F 00 00 | 39 D7 00 00 | 01 00 00 00  (identical constants in all 3 files)
first zlib stream starts at file offset 0x0000B2 in every file
zlib census: streams decoding to exactly 4096 bytes (plus 45/147/3089-byte small streams)
```

## Verification against the documented grammar (rsf-format--scan-session.md)

| Documented | Real-file observation | Verdict |
|---|---|---|
| Magic[16] `EA F4 B3 1B 05 43 B6 4A B4 B8 24 4A 33 D9 13 BB` (:27) | byte-identical in all 3 files | **CONFIRMED** |
| File header 0x8C bytes; reader seeks to 0x8C (:51) | first record header would start at 0x8C; record header 0x24 bytes + 2-byte chunk header = 0x8C+0x24+2 = **0xB2** = the observed first-zlib offset | **CONFIRMED (arithmetic exact)** |
| chunked zlib, 4 KB (0x1000) input blocks, header `u16le`, payload ≤ 4096 (:83-95) | census shows repeated streams decoding to exactly 4096 bytes; small streams (45/147/3089 B) = tail chunks | **CONFIRMED** |
| "compressed emitted only if smaller than raw" (:95) | mixed stream presence consistent | consistent (not falsifiable by census alone) |
| naming `auto_<vol>_<M>_<D>_<H>_<m>_<s>.rsf` (§5.5) | filenames `auto_Local Disk (G_)10_15_00_44_10.rsf` etc. — label with drive letter in parens, then M_D_H_m_s | **CONFIRMED** (and corroborates M-079's label-fill gap + N-105's naming surface) |

## Conclusion

**MATCH — no divergence.** The corpus's .rsf container documentation is byte-exact against
the real shipped session files; the core's CRSF is a registered clean-room equivalent
(API-DELTA row 19, G-09/F10 supersets) and nothing in this probe challenges that. This
file exists so future sessions can cite an EXECUTED confirmation of the container grammar
(the strongest evidence class) instead of doc-only citations.

No new finding from this probe; the naming observation strengthens existing M-079/N-105.
