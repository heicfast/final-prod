# SEVERE S-018 — JPEG EOI policy: the og salvages tail-truncated JPEGs; the core requires FFD9 + SOF + SOS and drops them

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** B2 (recovery correctness — truncated-photo salvage), A28 (validator accept/reject criteria + size inference), B3 (FP surface, inverse direction)
**Corpus references:** `RAWSearchDLL_decompiled.c` — JPEG = 4 u32 keys (0xe0ffd8ff/0xe1ffd8ff/0xdbffd8ff/0xeeffd8ff, :1458-1572) each → `BruteForce::JpegAnalyzer` (id 0x23) + `FileJPGAnalyser` fallback (ctor FUN_180033620 :39280-39305); the modern analyser body `FUN_180033a90` spans :39489-41110 and contains the SOF0 precision-8 gate (`bVar1==0xc0 && pbVar25[4]==8`, :39653) plus a size accumulation tied to SOF parsing (:39653-39660) — and **zero occurrences of the EOI constant 0xd9** in the entire body (grep over :39489-41110 = 0 hits; likewise :57080-57567 for the FileJPGAnalyser body = 0 hits). `artifacts/SearchDLL/rawsearch-carving--deep-dive.md` §5: BruteForce analysers "keep consuming until the grammar breaks, then report the last valid position as the file size … no premature truncation."
**Core references:** `src/core/carve/cr_analysers.cpp:214-291` (validate_jpeg): accept only at `mk == 0xD9` with `saw_sof && saw_sos` (:232-237), `return r` (ok=false) when the cap is reached without EOI (:290) — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Both og JPEG analyser generations accept a JPEG without ever looking for the EOI
(FFD9): no 0xD9 constant exists anywhere in their decompiled bodies; the size is
inferred from SOF0 dimension arithmetic. The core's validator accepts only when it
walks to an FFD9 **and** saw a SOF **and** a SOS before it — so any JPEG whose tail
(entropy data + EOI) is destroyed — the canonical carve scenario for photos on
damaged disks — is silently dropped by the core and salvaged by the og.

## What the reference does

Dispatch: the 4 JPEG u32 keys each map to a `BruteForce::JpegAnalyzer` (id 0x23)
holding a `FileJPGAnalyser` (two-generation, evidence file §1.1). The modern
analyser (`FUN_180033a90`) parses APP0/APP1 ("JFIF"/"Exif" constants :39542-39557),
walks markers, gates SOF0 on precision 8 (:39653), and accumulates a size estimate
into the analysis context (ctx+0xd8, :39653-39660) — i.e. it computes the file end
from **structure, not from an EOI marker**. A grep for the EOI immediate `0xd9`
over the entire analyser body (:39489-41110) and the legacy body (:57080-57567)
returns zero: neither generation requires or even checks for FFD9. Consistent with
the corpus deep-dive §5 (BruteForce = walk until grammar breaks, size = last valid
position) — truncated-tail JPEGs validate and get a SOF-derived size.

## What our core does

`validate_jpeg` (cr_analysers.cpp:214-291) walks markers after SOI; on `0xDA` it
scans entropy data for the next marker; the ONLY accept path is
`mk == 0xD9 → r.ok = saw_sof && saw_sos` (:232-237); reaching the cap without EOI
returns ok=false (:290); an EOI without prior SOF+SOS also rejects. So: (a) a JPEG
truncated inside entropy data (tail destroyed) → reject; (b) a JPEG whose EOI byte
pair was overwritten → reject. Both classes are common on damaged media and both
are accepted by the og.

## Gap

Falsifiable claim: a disk whose unallocated space holds `photo.jpg` with intact
SOI+APP0+SOF0+SOS but a destroyed tail (no FFD9 within the 96 MiB cap) yields a
carved, correctly-sized-to-SOF JPEG in the og and **zero** JPEG results in the core.
(Partially static evidence: the og-side no-EOI policy is proven by the constant
absence + corpus doc; the core-side EOI requirement is code-cited. A full executed
comparison would require reimplementing the og analyser — the claim's og half rests
on strength-2/3 evidence.)

## Impact

- Yield loss on the single most-carved format exactly in the damaged-media cases
  carving exists for; users see missing photos the og product recovers.
- The core's stricter policy does cut the opposite tail (noise that happens to
  start FF-D8-FF can't validate without a full marker walk to EOI) — that is a
  deliberate-looking hardening, but it is not documented as a divergence and the
  og's two-generation validators already police noise with SOF/dimension gates.

## Reproduction / verification

- Og: `grep -c "0xd9" <JpegAnalyzer body>` → 0 (lines 39489-41110);
  `grep -c "0xd9" <FileJPGAnalyser body>` → 0 (lines 57080-57567); SOF0 gate at
  :39653; corpus doc quote above. Evidence file §2.4.
- Core: cr_analysers.cpp:232-237, :290 (cited).

## Fix plan (for the engineering team)

1. Add a truncated-tail salvage path to `validate_jpeg`, mirroring the og: if the
   marker walk reaches the cap (or the entropy scan finds no closing marker) but
   `saw_sof` is true, accept with size = SOF-derived estimate (height ×
   bytes-per-row from SOF0 sampling factors, the og's model) capped at
   `min(cap, 96 MiB)`, flagged at a lower confidence (e.g. 600, note
   "SOF-size-salvage") so the UI can grade it PartialRisk — the og-equivalent
   behavior with our honesty-cap conventions.
2. Keep the EOI path as the preferred one (confidence 900) — superset of both
   policies.
3. Tests: `Jpeg_Truncated_No_Eoi_Salvaged_With_Sof_Size` (SOI+APP0+SOF0+SOS+partial
  entropy, no FFD9 → ok, size>0), `Jpeg_No_Sof_Rejected` (noise with EOI bytes
   still rejects), `Jpeg_Full_Walk_Confidence_900` (unchanged).

## Tests to add

(fix plan step 3 — fail before, pass after)

## Related findings

- S-007 (JPEG key coverage is fine in the core — magic FF-D8-FF matches any 4th
  byte, a superset over the og's E0/E1/DB/EE keys; I-009 records it).
- S-017 (BMFF signatures — same module, same "validator claims vs reality" shape).

## Open questions

The exact og SOF→size arithmetic (the ctx+0xd8 accumulation at :39653-39660) is
partially decoded; if the fix adopts the SOF-size model, decode the formula fully
first (candidate: bytes-per-row = ⌈(width × component_h_samp)⌉ × component count,
size = height × bpr + header extent).
