# INFORMATIONAL I-029 — DRIVER-CONTRACT's "EUDCPEPM contract — 19 recognized codes, byte-for-byte" overstates the corpus grounding for 4 of them: 0x800C2AC0 (DISCOVERY) and the RRWR trio 0x800C2B3E/B40/B42 appear in no corpus artifact, decompile, or og binary string — they are core-driver wire, not reference wire

**Severity:** INFORMATIONAL
**Module / Surface:** transport / driver contract (docs/DRIVER-CONTRACT.md)
**Parameters affected:** 46 (contract documentation), B22-adjacent (the wire-code registry's provenance)
**Corpus references:** Corpus coverage: `easeus-drw-research/re-workspace/artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md:52-75` (the EUDCPEPM session codes, range 0x800C2AC8…0x800C2B38 = **15 codes**); grep-verified across the corpus this session: zero hits for 0x800C2AC0, 0x800C2B3E/B40/B42, or the "RRWR" magic in any decompile (ghidra-drivers/), artifact, or the og binaries' strings.
**Core references:** audited commit `ff879f9` — `docs/DRIVER-CONTRACT.md:131-153` (§4.4: "the EUDCPEPM contract — 19 recognized codes … preserved byte-for-byte"); `include/cr/win/rda_ioctl.h:224-225` (the RRWR magic) + :118-142 (the code set, kept after N-041 deleted the RdaClient that issued them — "driver-wire mirror").
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** The v0.2.4 verification (session-3 legs) re-verified "all 19 session codes first-hand" — against the DRIVER repo's own header, not the corpus; the corpus covers 15. The four extras may exist in an og filter-driver build not represented in the corpus.

## Summary

The four extra codes are not verifiable against the corpus ground truth — they come from the core driver's own wire contract. No behavioral impact (the issuing client is deleted; the codes are a mirror), but the doc's "byte-for-byte with the og" phrasing claims a provenance the corpus cannot support for those four. Informational: the fix is the provenance note, not the codes.

## What the reference does

The corpus's 15-code range as cited; no og evidence for the four extras (absence verified by grep this session).

## What our core does

19 codes with the byte-for-byte claim as cited.

## Gap

Falsifiable: grep the corpus for the four constants/the RRWR magic — zero hits (done this session).

## Impact

Doc-truth precision on the driver wire contract; zero runtime impact.

## Reproduction / verification

The grep sweep (this session) + the artifact's 15-code table.

## Fix plan (for the engineering team)

1. Amend DRIVER-CONTRACT §4.4: 15 codes carry og-corpus provenance; 4 (0x800C2AC0 + the RRWR trio) are core-driver wire with no corpus counterpart (kept for the driver contract's completeness; issuer deleted by N-041). 2. If the driver lane ever wants them verified: the only path is an og filter-driver build not in the corpus — record that as the open corpus question.

## Tests to add

None (doc fix).

## Related findings

N-041 (the deleted client whose codes these mirror); D-DRV-5 (the identity-IOCTL register with real corpus provenance).

## Open questions

Whether an og EUDCPEPM build using the four codes exists outside the corpus — a corpus-lane question (this session's two-repo scope: recorded, not pursued).
