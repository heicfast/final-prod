# MAJOR M-099 — the AEP (After Effects project) carve format is absent: the og probes bare RIFX (no body check — the og's own weak grammar), sizes by the BE chunk length + an XMP end-marker scan (30 MiB cap) — the weak probe must be ported as-is for parity

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1, A2-adjacent (pro-media project files), A28-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:4024-4074` (AEPAnalyser id 0x9c — list-layer, RawConfig "AEP" token); the probe `:11550-11598` (first-4 == **RIFX** 0x58464952 — big-endian RIFF; **NO "Egg!" body check** — any RIFX passes, the og's deliberate weak grammar); the size walk `:11603-11764` (size = 8 + BE-u32 length at +4 (the byteswapped RIFX chunk length :11690-11695); if < 0x1e00001, extended by a chunked scan for **`<?xpacket end`** (the embedded XMP terminator, needle :11658-11666), size = marker + 6 (:11760-11762); cap ~30 MiB).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — no RIFX probe (the only RIFF-family carve is the little-endian forms if any)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred AEP ("key sites not decoded first-hand"); this session decoded the probe and the XMP walk.

## Summary

After Effects projects (.aep, big-endian RIFX containers) are absent from core. The og's analyser deliberately accepts ANY RIFX stream (no "Egg!" section check) — a weak grammar whose false-positive posture is itself the parity target — then sizes by the BE chunk length, extended to the embedded XMP packet terminator when present, capped at ~30 MiB. A faithful port must copy the weak probe, not strengthen it.

## What the reference does

AEPAnalyser (id 0x9c) probes bare RIFX at offset 0 with no secondary gate; size = 8 + the byteswapped u32@+4; if that is under ~30 MiB the og scans for the `<?xpacket end` XMP marker and takes marker+6; cap ~0x1e00001.

## What our core does

No RIFX/AEP analyser of any form.

## Gap

Falsifiable: a RIFX stream with BE length 0x1000 and an XMP packet carves in the og (id 0x9c, size to the XMP end); core carves nothing.

## Impact

Pro-media project recovery (AE projects are large, high-value, and often on media disks); the weak probe also means other big-endian RIFF collateral carves under the AEP name in the og — parity requires the same behavior.

## Reproduction / verification

Decompile-direct: probe + walk read this session and quoted above.

## Fix plan (for the engineering team)

1. Add FMT_AEP ("aep", display "After Effects project") as a list-layer probe on RIFX. 2. Implement the size walk verbatim (BE length + the `<?xpacket end` extension + the 30 MiB cap). 3. Port the WEAK grammar as-is (record the deliberate FP posture in the validator comment; do not add an "Egg!" gate the og lacks).

## Tests to add

- `Carve.AepRifxAccepted` — a bare RIFX with sane length carves (weak grammar parity).
- `Carve.AepXmpEndExtent` — the `<?xpacket end` marker extends the size to marker+6.
- `Carve.AepBeLengthByteSwap` — the BE u32 is read byteswapped (little-endian RIFF must NOT claim this row).
- `Carve.AepCapThirtyMiB` — the walk cap.

## Related findings

S-007 (batch-2 parent); M-092..M-098 (the batch-2 family).

## Open questions

None material.
