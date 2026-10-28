# MINOR N-187 — validate_mpegps reads the MPEG-PS system-header and PES length fields as LITTLE-endian u16 — every length with a non-zero high byte walks to a corrupted extent (a 0x0123 PES walks as 0x2301)

**Severity:** MINOR (a wire-decoding defect in the generic MPEG-PS structural walk: extents inflate or collapse by the byte-swapped delta for every stream whose PES/system-header lengths carry a non-zero high byte — which is every real-world PS stream, since PES payloads are commonly > 255 bytes; discovered during M-098's verification per §8)
**Module:** carve (the generic MPEG-PS row, S-007's structural walk)
**Parameters affected:** P28 (the carve validator's length decoding — the MPEG-PS wire form is BE u16, ISO 13818-1 §2.4.3.7 PES_packet_length / §2.4.3.3 system_header_length), P1-adjacent (extent correctness on the generic stream rows)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:30489-30524` (the og's HIK walk reads every PS length BYTE-SWAPPED: `*(ushort *)(piVar1 + 1) << 8 | *(ushort *)(piVar1 + 1) >> 8` — the BE u16 wire form; the same byte-swap decodes the probe's PES length at :30441/:30476) — the og's own PS-family length convention is big-endian, and the MPEG-PS wire format itself (ISO 13818-1) defines both length fields as BE u16.
**Core references:** the fixed `src/core/carve/cr_analysers.cpp` validate_mpegps (:3288/:3296 pre-fix) — both length reads used `rd16` (the LE helper); the S-007 landing's own fixture `plant_mpegps` (tests/unit/test_carve.cpp:2040/:2048 pre-fix) planted the lengths LE (`put16`), so the fixture and the validator shared the same wire-inconsistent convention and the suite stayed green over the defect (the fixture could never have been a real PS stream).
**Status:** FIXED (discovered and fixed in the M-098 landing, per §8)
**Renumber:** N-156 → **N-187** (2026-10-29, the S56 lane: the N-160 doc-truth gate caught the collision live — the RRWR-direction N-156 was filed at `0ff6cc92` 20:30:07 UTC, this file's landing rode `c0efacc3` 21:04:29 UTC; the earlier push keeps the seat per SA-9; the ledger row and the M-098 Resolution cite updated)

## Summary

The generic MPEG-PS structural walk reads the system-header length (the u16 at packet+4) and every PES packet length with `rd16` — the little-endian helper — while the MPEG-PS wire form defines both as big-endian u16, and the og's own PS-family walks (the HIK analyser, decoded in the same session) read them byte-swapped. Every stream whose lengths carry a non-zero high byte — i.e. every real PS stream with PES payloads over 255 bytes — walks to a byte-swapped extent: a 0x0123-byte PES advances 0x2301 bytes (8 KiB past the payload), a 0x0020-byte PES advances 0x2000.

## What the reference does

The og's PS-family walks decode all PS lengths as BE u16 (the byte-swapped u16 reads at :30441/:30476/:30520-30524 — `uVar1 << 8 | uVar1 >> 8`); the MPEG-PS wire format (ISO 13818-1) defines PES_packet_length and system_header_length as big-endian u16.

## What our core does

Pre-fix: `rd16` (LE) at both length sites; the S-007 fixture planted the lengths LE too (`put16(img, p + 4, 6)`), a self-consistent but wire-impossible stream that kept the suite green over the defect.

## Gap

Falsifiable: plant a PS stream with a BE length whose LE misread differs (0x0123): the pre-fix walk advances 0x2301 bytes past the PES payload; the extent inflates by 0x21DE bytes (proven RED pre-fix by the M-098 landing's stashed-engine rebuild — the pre-fix tree carved the fixture at the inflated extent).

## Impact

Extent corruption on every generic MPEG-PS carve with real-world lengths: inflated extents swallow following streams/files into one carved row (and the F9 dedup then suppresses the swallowed files' own candidates at their positions — `emitted_at` covers only the exact start positions, so interior swallowed candidates still carve, but the inflated row itself misreports the file size and the recovery writes past the stream's real end).

## Reproduction / verification

Discovered first-hand during M-098's verification (the HIK fixture's generic-PS fallback carved 8234 bytes instead of 136: the 0x20-byte private_stream_1 PES walked as 0x2000). The og's byte-swapped length reads re-verified at the corpus cites during the fix.

## Fix plan (for the engineering team)

Both length reads become `rdbe16`; the S-007 fixture's two `put16` length plants become `putbe16` (the wire form). A dedicated BE-vs-LE discriminating test pins it.

## Tests to add

- `Carve.MpegPsPesLengthsAreBigEndian` — a stream with a 0x0123-byte PES: the extent is the exact BE walk (the LE misread would inflate by 0x21DE).

## Related findings

**M-098** (the HIK landing whose verification discovered this) · **S-007** (the generic row's original landing — the fixture shared the defect's convention) · the og's HIK walk (the corpus's own BE length convention).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the M-098 landing — discovered during its verification per §8, filed + fixed in the same landing)
**Verified by:** Carve.MpegPsPesLengthsAreBigEndian (GREEN post-fix; the pre-fix tree's RED proven by the M-098 stashed-engine rebuild — the HIK fixture's generic-PS fallback carved 8234 bytes pre-fix, 136 post-fix) + the HikGenericPsNotShadowed/HikPackSystemHeaderPrivateStreamAccepted fixtures (whose BE lengths exposed the misread); full suite 1157/1157 green post-fix.
**Resolution notes:** Both length reads in validate_mpegps switched `rd16`→`rdbe16` (the system-header length at +4 and the PES length at +4 — the ISO 13818-1 BE u16 wire form, and the og's own PS-family byte-swapped reads :30441/:30476/:30520-30524); the S-007 fixture's two length plants switched `put16`→`putbe16` so the fixture plants a real wire-form stream (the fixture's own extent expectation is unchanged — 6 and 8 read the same either way, only the byte order in the image changes); the dedicated Carve.MpegPsPesLengthsAreBigEndian test pins the BE decoding with a length whose LE misread differs by 0x21DE.
