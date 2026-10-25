# MINOR N-054 — Carve overlap resolution: og ranks candidates by accumulated header+tail confidence; core uses trial-order first-wins. Two-generation validator fallback also missing

**Severity:** MINOR
**Module:** carve
**Parameters affected:** A28 (carving validator logic — accept/reject + overlap resolution), A29 (merge/dedup strategy), D15 (confidence grading model), B3 (false-positive rate)
**Corpus references:** `re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md:73` ("The engine **accumulates header + tail confidences to rank candidate carves** (multiple overlapping candidates resolved by score)"), `rawsearchdll--analyser-registry-census.md:43` (the two-generation design: "every format has a modern analyser … and — where one exists — a legacy fallback `File*Analyser` … two independent validators must both fail before a match is discarded, and one strong validator accepting is enough")
**Core references:** `src/core/carve/cr_carve.cpp:35-42,153-155,254-256` (fixed-confidence formulas: `(h+t)/2+100` / `(base+validator)/2`, capped 1000), `cr_carve.cpp:135-136` (`emitted_at.insert(cand_abs); break;` — the first validator to accept a position owns it; remaining trial candidates for the same position are never compared)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When multiple carve candidates overlap, the og resolves the winner by accumulated score (header confidence + tail confidence), and its validator chain has a two-generation fallback (modern analyser, then legacy `File*Analyser`) so a candidate is discarded only when both fail. The core resolves overlaps by trial order (first accepting validator wins, no score comparison among candidates) and has exactly one validator per format — no fallback chain.

## What the reference does

searchdll-family--architecture.md:73: raw_signs entries carry explicit confidence (e.g. "0,44564456,0.96" for ifo) and "the engine accumulates header + tail confidences to rank candidate carves (multiple overlapping candidates resolved by score)". census:43: the registry maps each magic to a modern analyser plus — where one exists — a legacy `File*Analyser` fallback; "two independent validators must both fail before a match is discarded, and one strong validator accepting is enough" (the stated reason carving "rarely false-positives").

## What our core does

The scan loop tries formats in registry order at each position; the first validator that accepts emits and `break`s (cr_carve.cpp:135-136) — overlapping candidates at the same position are never scored against each other. Confidence is a formula over one validator's verdict (`(base+validator)/2`), not an accumulation across header/tail evidence. There is one validator per format; a rejection is final.

## Gap

Falsifiable claim: for a position where two format hypotheses overlap (e.g. a TIFF-looking header that also satisfies a camera-RAW prologue), the og emits the higher-scoring candidate; the core emits whichever format row comes first in registry trial order.

## Impact

Tie-break policy difference on overlapping candidates (wrong-format wins are possible where the og would pick the better-scored one — e.g. CRW vs TIFF, the census's own example of a signature collision family) and a missing second-chance validator (a candidate the modern grammar rejects but the legacy analyser would accept is lost). In practice most positions have one plausible format, so the delta concentrates on the known collision families (TIFF `II*.` vs camera RAW, 4-JPG-magic families).

## Reproduction / verification

Corpus-derived (both artifacts first-cited this session; quotes above). Core side verified in-source: the trial loop, the break, and the single-validator chain read at cr_carve.cpp:35-42,135-136,153-155.

## Fix plan (for the engineering team)

1. Collect instead of break: at each candidate position, run all registry rows whose magic matches; score each accepting candidate as `base_confidence` + validator-verdict accumulation (keep the existing formula as the per-candidate score); emit the max-scoring one; ties break by registry order (deterministic).
2. Two-generation fallback: for the formats where the corpus evidences a legacy `File*Analyser` (the census's magic-key table marks the chains), add a lenient legacy validator invoked when the modern grammar rejects — the census's roster (census:45-117) is the work list. Alternatively register the fallback-chain architecture as a deliberate simplification in PARITY-DELTAS if the legacy grammars are not worth porting — but decide explicitly, don't leave it implicit.
3. Tests: `Carve.OverlapScorePrefersHigherConfidence` (construct a dual-magic position where the second row's validator is stronger), `Carve.TiffVsRawTiebreak`, `Carve.LegacyFallbackAccepts` (a candidate failing the modern grammar but passing the legacy one).
4. Acceptance: the dual-magic fixture emits the higher-scored format; the census collision families each have a deterministic winner.

## Tests to add

- `Carve.OverlapScorePrefersHigherConfidence` — fails today (first-in-order wins)
- `Carve.TiffVsRawTiebreak`
- `Carve.LegacyFallbackAccepts` — fails today (no fallback)

## Related findings

N-030 (raw_signs confidence field — the og's confidence semantics this finding builds on); S-007 addendum (this session — the family-layer roster); M-042 (same analyser family).

## Open questions

The exact accumulation arithmetic (sum vs. weighted mean of header/tail confidences) is not decoded beyond "accumulates header + tail confidences"; the census's 0.50-1.00 score domain suggests a 0-1000-scale sum. The fix plan's `base + verdict` accumulation approximates it; a precise decode of the ranking comparator (FUN_ at the dispatch site) would pin it — tracked as part of OQ-S4-2's RAW-hook decode if the team needs exactness.
