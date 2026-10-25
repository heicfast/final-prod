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
