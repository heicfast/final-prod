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

## Resolution

**Status:** INVALID (premise refuted first-hand; residual question moved to tracked-open)
**Fixed in commit:** (this commit — audit-trail correction only; NO code change: loosening a working validator on a refuted premise would be a premise-driven regression)
**Verified by:** first-hand decompile re-read + a corrected constant search: five `-0x27` (= 0xD9, signed-char rendering) FF D9 sites inside the modern analyser body (FUN_180033a90, :39489-41110) — the finding's `grep -c "0xd9" -> 0` missed them; the accept/size-accumulation path is EOI-gated (`if (uVar21 == 0) goto <epilogue>`, :39760). Full quotes: `evidence/decompiled-quotes/s3-jpeg-eoi-encoder-correction.md`.
**Resolution notes:** The finding's corpus premise — "no 0xD9 constant exists anywhere in their decompiled bodies; neither generation requires or even checks for FFD9" — is FALSE for the primary analyser: the modern JpegAnalyzer searches for FF D9 with a backward scan (Ghidra renders the byte as the signed char literal `-0x27`, which a literal-text `0xd9` grep does not match) and its size-accumulation path returns early when no EOI pair is found in the buffer. The og's primary JPEG analyser is EOI-gated exactly as our validator is, by a different mechanism (backward last-FFD9 scan vs our forward marker walk — which of the two admits more is itself open). The legacy half of the claim stands for the dispatcher body (:57080-57567, no EOI reference), but that body branches to JFIF/Exif/generic sub-analysers (FUN_18004bbd0/b3b0/c1e0) whose accept criteria are NOT decoded — if any accepts without an EOI, the salvage claim could hold through the fallback, and the streamed continuation model (early return != final reject; the 0x7800000 default at :39757) adds a second unresolved acceptance path. Per the severity discipline (a false SEVERE must not drive a validator change) and the corpus-wins rule, the finding is INVALID as filed; the two residual decode questions are tracked in `tracked-open/OQ-S3-jpeg-fallback-and-continuation.md` with the exact functions and line ranges. `validate_jpeg` is UNCHANGED; no test changes (the core's EOI+SOF+SOS accept is consistent with the EOI-gated primary analyser as far as the corpus proves). The finding file is preserved per the audit-trail contract.

---

## Re-file + Resolution (session DFX31 — OQ-S3-JPEG resolved; the salvage claim REINSTATED and FIXED)

**Status:** RE-FILED as CONFIRMED-DIVERGENCE (the original claim's salvage
half) → **FIXED** (this session's commit)
**Fixed in commit:** (this session's S-018 salvage commit)
**Verified by:** `Carve.JpegTruncatedTailSalvagedAtSofEstimate` (fail-before
RED executed on the stash-isolated pre-fix tree: "Expected: (jpg) !=
(nullptr), actual: NULL" — the truncated photo was dropped; post-fix the
jpg node exists with extent [hit, hit + 64*48*3) = the og's h*w*3-from-start
sizing, grade PartialRisk) + `Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth`
(fail-before RED: no node; post-fix the dims-unknown salvage spans to the
device end — the og's /10 limit bounded by the getter-NULL accept) +
`Carve.JpegCompleteFileStopsAtEoiNotSofEstimate` (the pin: a complete JPEG
still accepts at its EOI with the 900 full-walk grade — the modern tier's
EOI gate, the original INVALID's evidence, is preserved exactly). Full
suite 755/755.
**Resolution notes:** The INVALID disposition's residual question — "if any
[sub-analyser] accepts without an EOI, the salvage claim could hold through
the fallback" — is now answered YES, first-hand (evidence
`s31-oq-s3-jpeg-fallback-decode.md`): the legacy `FileJPGAnalyser`
dispatcher (FUN_18004b170 :57100) branches JFIF/Exif/generic; ALL THREE
sub-analysers' header walks are SOS-gated (SOF0 precision-8 checked, SOF2
dims fill-if-empty), and the shared post-SOS walker FUN_18004c480
(:57942-57991) ACCEPTS AT ITS SCAN LIMIT with no EOI found — `if (limit <=
off || get() == NULL) goto ACCEPT` at :57958-59 — stamping the 0x23 format
code with size = the offset reached. The limit is h*w*3 measured from the
CANDIDATE START (dims from SOF; the caller :57803-07) or 20,000,000 (the
200,000,000 global cap at FUN_180001350+0x70, /10 — the dims-unknown and
header-walk bound); the sliding-window getter FUN_180042930 reads on demand
so the device end is the other bound. The census-documented two-generation
chain (modern EOI-gated + legacy permissive; one accept suffices) makes the
og's effective behavior: truncated-tail JPEGs ARE recovered, sized at the
estimate. The fix mirrors this in `validate_jpeg`: the SOF dims are
captured during the walk (SOF0 last-wins precision-8-gated, SOF2
fill-if-empty — the og's exact arms), and every no-EOI exit (entropy-scan
cap, walk cap, mid-scan I/O failure — the getter-NULL analog) now takes the
salvage when SOS was seen: end = min(hit + h*w*3 | hit + 20,000,000,
bound), confidence 600. **Deliberate delta (registered):** the og stamps
salvaged files identically to complete ones (no grade distinction found in
the legacy path); the core grades the salvage PartialRisk (600 < the 850
Excellent band) — recover more, grade honestly. The no-SOS case stays a
reject (the og's header-walk buffer-end reject), and the modern tier's
EOI-gate behavior is unchanged for complete files (the pin test). The
finding's original SEVERE framing (B2 recovery correctness — the canonical
damaged-disk photo dropped) stands as re-filed; the INVALID's correction of
the ORIGINAL premise (the modern analyser IS EOI-gated) remains part of the
record — the re-file cites the fallback, not the original "no 0xD9 anywhere"
claim.
