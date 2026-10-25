# MINOR N-068 — ZIP carve anchors: EOCD (PK\x05\x06) anchor missing; LFH sanity gates (version_needed ≤ 10, extra ∈ [1,0x102]) missing

**Severity:** MINOR
**Module:** carve
**Parameters affected:** B1 (detection correctness — anchor coverage), A28 (carving validator logic — LFH sanity grammar), B3 (false-positive rate)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md:13-21` (entry-point table: LFH "accepted with sanity checks: `version_needed ≤ 10` (offset +8, ushort) and extradata/flags field at +0x1a in `[1, 0x102]`"; CD entry PK\x01\x02 "preferred anchor — index-based walk"; **EOCD PK\x05\x06 "accepted (truncated/EOCD-first zips)"**; "the analyser itself handles all three once invoked — meaning ZIPs whose first surviving structure in the raw stream is a CD entry or the EOCD are still carved")
**Core references:** `src/core/carve/cr_registry.cpp:73-83` (FMT_ZIP anchored on PK\x03\x04 LFH only), `:261-270` (FMT_ZIP_CD anchored on PK\x01\x02 — the CD anchor landed with the prior zip work), no PK\x05\x06 anchor anywhere; `src/core/carve/cr_analysers.cpp:592-708` (validate_zip walks LFH→CD without the version_needed/extra-range gates)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ZIP analyser accepts all three ZIP structures as carving anchors (LFH with sanity gates, CD as the preferred anchor, EOCD for truncated/EOCD-first zips). The core has the LFH and CD anchors (FMT_ZIP_CD landed earlier) but no EOCD anchor, and its LFH acceptance lacks the og's two sanity gates (version_needed ≤ 10; extra/flags ∈ [1, 0x102]).

## What the reference does

rawsearchdll--zip-analyser-carving.md:13-21 (first-cited this session): the analyser begins from ANY of the three structures. The LFH anchor carries two sanity gates (`version_needed ≤ 10` at +8; extradata/flags at +0x1a within `[1, 0x102]`) — cheap false-positive suppressors. The EOCD anchor specifically recovers **truncated/EOCD-first** zips: archives whose head structures were destroyed but whose end-of-central-directory record survives (the EOCD carries the CD offset+size, from which the CD can be re-located and walked).

## What our core does

FMT_ZIP (PK\x03\x04) and FMT_ZIP_CD (PK\x01\x02) anchors exist; validate_zip walks forward from an LFH and validate_zip_cd walks the CD. No PK\x05\x06 anchor is registered, so an EOCD-first zip (head destroyed, CD+offset recoverable from the EOCD) never triggers a carve. validate_zip does not apply the version_needed/extra-range gates.

## Gap

Falsifiable claims: (1) a zip whose first 512 bytes are zeroed but whose EOCD survives is carved by the og (EOCD → CD offset → walk) and missed by the core; (2) a non-zip stream starting with PK\x03\x04 + version_needed 0x0B / extra 0xFFFF is rejected by the og's LFH gates and accepted (up to the walk's own limits) by the core's.

## Impact

Recall gap on head-truncated zips (a classic partial-overwrite case) and a small false-positive-rate delta on LFH lookalikes. Both narrow; the anchor asymmetry is the bigger half.

## Reproduction / verification

Corpus-derived (artifact first-cited this session; the entry-point table quoted above). Core side verified in-source: registry rows read at cr_registry.cpp:73-83/261-270 (grep for 0x504b0605 / PK\x05\x06 across carve: 0 hits); validate_zip read for the gate absence.

## Fix plan (for the engineering team)

1. Register a third ZIP anchor row `FMT_ZIP_EOCD` on `{'P','K',0x05,0x06}` with `validate_zip_eocd`: parse the EOCD (this-disk/entries/CD-size/CD-offset, u16/u16/u32/u32 at +4..+15, zip64-aware via the 0xFFFF sentinel check), then hand off to the existing `validate_zip_cd` walk at the computed CD offset. Cap: same 2 GiB size gate as the zip rows.
2. Add the og's LFH sanity gates to `validate_zip` (version_needed ≤ 10 at +8; extra/flags field at +0x1a within [1, 0x102]) — reject the candidate when violated (the og's cheap FP suppressors).
3. Tests: `Carve.ZipEocdFirstTruncated` (zeroed head + intact EOCD/CD → carved via CD), `Carve.ZipLfhSanityGates` (version_needed 0x0B → rejected; extra 0x1FF → rejected), `Carve.ZipNormalUnaffected` (regression pin).
4. Acceptance: the truncated-head fixture carves; the two gate fixtures reject; the existing zip suite stays green.

## Tests to add

- `Carve.ZipEocdFirstTruncated` — fails today
- `Carve.ZipLfhSanityGates` — fails today (no gates)
- `Carve.ZipNormalUnaffected`

## Related findings

S-007 (carve missing common formats — same analyser family; this finding is the zip-internal refinement); N-008 (FIXED — zip padding/target policy; the sector-wise padding nuance from the same artifact is registered there); S-032 (archive inventories — same analyser decode).

## Open questions

None — the artifact's entry-point table is explicit on all three anchors and both gates.
