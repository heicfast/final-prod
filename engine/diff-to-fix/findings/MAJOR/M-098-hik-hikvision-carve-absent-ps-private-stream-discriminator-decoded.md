# MAJOR M-098 — the HIK (Hikvision surveillance) carve format is absent: the og discriminates a private_stream_1 PES inside an MPEG-PS pack (00 00 01 BA → 00 00 01 BD) and walks the structure to a 1 GiB cap — CCTV footage, a flagship recovery scenario, is unrecoverable

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1, A2 (surveillance footage never carved), A28-adjacent
**Corpus references:** `easeus-drw-research/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:3823-3894` (CHIKAnalyser id 0x92 — list-layer, RawConfig "HIK" token); the probe `:30412-30484`: first-4 == 0xBA010000 (MPEG-PS pack header 00 00 01 BA); pack length = (buf[0xd]&7)+0xE; at the pack end either 0xBB010000 (system header 00 00 01 BB → bounded length checks < 0x40000001 / total < 0x400001) then 0xBD010000 (**private_stream_1 PES 00 00 01 BD**) → id 0x92; or a direct 00 00 01 BD after the pack → id 0x92 with size from the PES BE-u16 length; the walk `:30489-30524+` continues the PS structure walk with **cap 0x40000000 (1 GiB)**. Census context: the census's "(Hikvision?)" guess is now CODE-confirmed (erratum note).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — FMT_MPEGPS carves the generic PS stream but has no private-stream discriminator (a HIK file carves as generic MPEG-PS at best, unnamed .hik never)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred the text school; HIK's probe decode is this session's work. No prior finding.

## Summary

Hikvision DVR footage (.hik/.264 variants) is a flagship surveillance-recovery scenario and is absent from core. The og's list-layer analyser recognizes the vendor's private_stream_1 PES pattern inside an MPEG-PS pack and walks the PS structure to a 1 GiB cap. Core's generic MPEG-PS row would at best carve the stream under the wrong name/extent semantics; the HIK discriminator, its exact pack/system-header length gates, and the 1 GiB walk are decoded and portable.

## What the reference does

CHIKAnalyser (id 0x92) content-probes: the PS pack header, computes the pack length from the stuffing field, then requires the system-header → private_stream_1 sequence (or the direct BD form), stamps id 0x92, and walks the PS structure with a 1 GiB cap; sizes come from the PES length fields during the walk.

## What our core does

No HIK analyser; the generic FMT_MPEGPS row exists but neither the private-stream discriminator nor the vendor's length gates/1 GiB cap.

## Gap

Falsifiable: a PS pack followed by a system header + a private_stream_1 PES (the Hikvision layout) carves in the og as id 0x92; core has no such discriminator.

## Impact

Surveillance/CCTV recovery is a high-value commercial scenario (evidence footage); files are large and content-only-detectable (extensions are often generic .264/.hik).

## Reproduction / verification

Decompile-direct: the probe and walk were read this session and quoted above.

## Fix plan (for the engineering team)

1. Add FMT_HIK ("hik", display "Hikvision recording") as a list-layer probe riding the MPEG-PS scanner seam. 2. Implement the pack-length computation ((buf[0xd]&7)+0xE), the BB→BD sequence (with the og's exact length bounds), the direct-BD form, and the structural walk with the 1 GiB cap. 3. The generic MPEG-PS row must NOT claim a stream the HIK probe accepts first (trial order: HIK before MPEGPS).

## Tests to add

- `Carve.HikPackSystemHeaderPrivateStreamAccepted` — the BB→BD sequence carves as FMT_HIK.
- `Carve.HikDirectPrivateStreamAccepted` — the direct-BD form carves.
- `Carve.HikGenericPsNotShadowed` — a plain PS stream without private_stream_1 stays FMT_MPEGPS.
- `Carve.HikOneGiBCap` — the walk bound is honored.

## Related findings

S-007 (batch-2 parent); S-047 (the list-layer seam + MPEG family); N-113 (the smaller list formats).

## Open questions

None material — the probe, gates, and cap are decoded.
