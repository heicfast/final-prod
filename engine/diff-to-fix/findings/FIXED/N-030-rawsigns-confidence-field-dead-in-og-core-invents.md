# MINOR N-030 — raw_signs.json confidence field is dead data in the og; the core invents confidence semantics for it

**Severity:** MINOR
**Module:** carve (raw-sign layer)
**Parameters affected:** D15 (confidence grading model), B3 (false-positive rate — ranking only), D13 (report fields for carved nodes)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :23522-23765 (sign-string parser FUN_18001e510: full comma split via FUN_180021570 :25885, `>= 2` parts gate, part[0]→offset digits, part[1]→hex body, **no read of part[2]**); `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md` §3 (the "confidence combination ranks overlapping candidates" claim this decode corrects)
**Core references:** audited commit `34b3441` — `scripts/gen_rawsigns.py` :27 (`int(round(float(conf) * 1000))`), :78-87 (conf emitted into every RawSign); `include/cr/carve/cr_rawsigns.h` :48 (`u16 conf; /* 0..1000 */`); `src/core/carve/cr_carve.cpp` :202-205 (highest-conf hit wins the position), :222 (`conf = (hdr + tail)/2 + 100` cap 1000), :273 (`props->carve_confidence = conf`), :297-298 (tail-paired → PartialRisk, else MetadataOnly)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Every sign string in `raw_signs.json` carries a third decimal field (e.g. `"0,44564456,0.96"`). The og's parser splits the string, consumes the offset and the hex body, and **never reads the third field** — the confidence values are dead data in RAWSearchDLL. Our core scales them (`0.96 → 960`) into every `RawSign`, uses them to pick the winning format at a shared position, blends a header+tail confidence with an invented `+100` bonus, and publishes the result as `carve_confidence` on the carved node.

## What the reference does

`FUN_18001e510` (first-hand read, quoted in `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §3): the sign string is split on every `,` (the split helper loops `find` until no delimiter remains — it is not a 2-way split). The parser then:

- requires at least 2 parts (`if (count < 2) return;` — so a bare `"offset,hex"` is legal and `"offset"` alone is rejected);
- parses part[0] as an unsigned decimal offset (digit-table loop with overflow guard);
- hex-decodes part[1] into the sign's byte vector;
- destroys the parts vector and returns. There is **no reference to the third element** anywhere in the function — no `+0x20` element access on the parts vector exists.

The corpus artifact `rawsearch-carving--deep-dive.md` §3 claims "Header+tail+size-gate confidence combination ranks overlapping candidates (raw_signs.json `confidence` fields, e.g. ifo `0,44564456,0.96`)". That claim is not supported by the parser: whatever ranking the og applies among overlapping sign hits comes from the sign-table ORDER (insertion order in the header/tail vectors) and the probe's own logic, not from the decimal field. (Erratum filed in our CORPUS-ERRATA copy, entry 19.)

## What our core does

The generator keeps the third field as the sign's authority (`int(round(float(conf) * 1000))`), the carving pass uses it three ways: highest-confidence header hit owns a shared position (`cr_carve.cpp:202-205`), a tail pairing produces `(header + tail) / 2 + 100` capped at 1000 (`:222`), and the value lands in `props->carve_confidence` (`:273`) plus the coarse `Confidence` grade (tail-paired → PartialRisk; header-only → MetadataOnly, `:297-298`).

## Gap

Falsifiable claim: the og's raw-sign hits carry no per-sign confidence derived from `raw_signs.json`'s third field; the core's `carve_confidence` for raw-sign-tagged nodes (`carve_format >= 0xFFFF0000`) is a value the og cannot produce, and the core's shared-position winner-selection rule (max conf) is an invention where the og uses table order.

## Impact

Data-model divergence only (D15/D13): our session/report surfaces expose a confidence number the og does not compute, and two raw-sign candidates sharing a start position can resolve to different formats than the og would pick. No recovery-correctness impact: extents and content are unaffected; both sides emit one candidate per position. Users comparing reports side-by-side see differing confidence numbers on raw-sign files.

## Reproduction / verification

Decompiled evidence (first-hand): parser read in full, part[2] access absent — `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §3. Core cites at `34b3441` above. Not execution-verified (requires the og product's report output for a raw-sign fixture; the bench harness's `compare_report.py` is the vehicle if desired).

## Fix plan (for the engineering team)

1. Decide policy: (a) og-parity — stop consuming conf (pick winner by table order like the og; set `carve_confidence` to whatever fixed grade the og emits, once decoded), or (b) keep the superset and DOCUMENT it as a deliberate divergence (the INFORMATIONAL pattern used for I-001..I-005).
2. If (b): change `cr_rawsigns.h`'s divergence block to state that the confidence field is dead in the og (cite this finding), and pin the behavior with a test so a future session cannot "fix" it accidentally.
3. If (a): remove the `conf` member from `RawSign` (or ignore it), replace `:202-205` winner selection with first-in-table-order, and drop the `+100` blend at `:222`.
4. The og's own emitted grade for raw-sign files (what its report shows) is NOT decoded — before choosing (a), decode `FUN_18000d670`'s result emission (the file-info fill after the probe accepts) or run the og on a raw-sign fixture and read its report.

## Tests to add

- `RawSigns.ConfidenceFieldDocumented` — pin test asserting the current blend (if superset kept), with a comment citing N-030 so the semantics cannot drift silently.
- If og-parity chosen: `RawSigns.SharedPositionUsesTableOrder` — two signs matching at one start position, first-in-table wins (fails under max-conf selection).

## Related findings

S-015 (min/max units — same decode), N-031 (dispatch/stride deltas), OQ-T8 (RESOLVED).

## Open questions

What confidence/grade value the og's report emits for a raw-sign carved file (the result-emission side of `FUN_18000d670`) — decode target for a future session, or an og execution + report read.

---

## Session-3 parallel-leg addendum (2026-10-26) — tail-hit size formula + invented 2 MiB cap

The *storage/census parallel leg* decoded the same layer independently (evidence:
`evidence/decompiled-quotes/s3-rawsigns-consumer-decode.md`) and confirms the
dead part[2] discovery. Two aspects NOT covered above, to fold into this
finding's fix plan:

1. **Tail-hit size formula diverges too.** The og sizes a tail-paired file as
   `sector_size * (cur_sector - start_sector) - tail_sign->offset + 50`
   (RAWSearchDLL `FUN_1800205b0` :25266-25267; literal 0x32 = 50). Our
   `cr_carve.cpp:221` computes `len = lo + th.pos + 512` — no sign-offset
   subtraction, flat +512 instead of +50. Sizes differ by
   `(512 - 50 + tail_sign_offset)` bytes on every paired hit.
2. **The 2 MiB tail-window cap is ours, not the og's.** `cr_carve.cpp:215`
   bounds the tail scan at 2 MiB; the og scans incrementally (return-2
   keep-scanning protocol) out to the format's max **KiB** bound (up to
   200 MiB; default 500 MiB when max==0, `0x7d000` at :25187-25196). After
   S-015's unit fix lands, this cap becomes the binding limit for large
   formats and must be raised to the format bound.
3. The consumer is the og's LIST-tier analyser `RawFormat_xjq::CommonRawAnalyser`
   (format id 0xa8) — placement matches our post-grammar second pass (see
   N-031's dispatch decode).

## Resolution

**Status:** FIXED (as a REGISTERED DELIBERATE SUPERSET — the fix plan's option (b))
**Fixed in commit:** this session's carve-minors batch (DFX12)
**Verified by:** the two existing pins, re-cited and now load-bearing for this resolution: `CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic` (the best-confidence header sign wins the shared flp/fst position — winner selection by conf, not table order) and `CarveEngine.RawSignTailPairWithinFormat` (the (header+tail)/2 + 100 blend asserted on the emitted node's carve_confidence). Suite 577/577 linux-release.
**Resolution notes:** the fix plan's own decision gate governs here: option (a) — removing the confidence semantics — is gated on decoding the og's emitted grade for raw-sign files (FUN_18000d670's result fill), which remains UNDECODED in the corpus; adopting (a) blind would replace a working ranking with an unknown. Option (b) executed as written: the cr_rawsigns.h divergence block now states the field is DEAD DATA in the og with the full citation chain (FUN_18001e510 :23522-23765 — the parser consumes part[0]/part[1], never part[2]; the rawsearch-carving deep-dive's "confidence combination ranks overlapping candidates" claim corrected — corpus erratum 19), PARITY-DELTAS D-66 registers the superset with do-not-remove guidance, and the two pins guard both consumed semantics (winner selection + the tail blend) so a future parity session cannot "fix" them accidentally. The honest remainder — decoding FUN_18000d670 to learn what grade the og actually emits for raw-sign files — is recorded in D-66 as the precondition for any future option-(a) revisit.
