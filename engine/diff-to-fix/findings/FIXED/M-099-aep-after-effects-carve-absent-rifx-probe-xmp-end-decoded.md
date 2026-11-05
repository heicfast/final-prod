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

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the S42 fix lane's M-099 landing)
**Verified by:** Carve.AepRifxAccepted · Carve.AepXmpEndExtent · Carve.AepBeLengthByteSwap · Carve.AepCapThirtyMiB (4/4 GREEN; RED pre-fix proven by the stashed-engine rebuild — `FMT_AEP` not declared pre-fix); full suite 1181/1181 green post-fix; F8 gate + self-probe green; clang-format 18.1.8 clean.
**Resolution notes:** The full analyser landed with the weak grammar ported AS-IS (the finding's own directive): bare RIFX with NO body check, any RIFX passes (the false-positive posture is the parity target; no "Egg!" gate added). Beyond the finding's own text: (1) the name constant **DAT_180067008 dumped from the og binary** (VA 0x180067008 → `41 00 45 00 50 00` = UTF-16LE **L"AEP"**) — landed as `CarveResult::name_prefix` = u"AEP" (the S-049 naming chain: AEP_000001.AEP); (2) the walk's window geometry decoded exactly: the FIRST 0x200 window at size+8 (the +8 offset at :11688), then the size counter advances 0x200 per miss with the NEXT window fetched AT the counter (the og's 8-byte first-window overlap, :11784-11785) — ported exactly; (3) the zero-run sub-walk decoded (:11769-11840): a window whose LAST 8 bytes are all zero triggers a forward scan for the FIRST 8 consecutive zero bytes in the window (d+7 < 0x1ff), ending the extent at size+d (the zero-tail trim — a zero-padded torn AEP ends at its zero run, not at the cap); (4) the cap semantics: a declared length ≥ 0x1e00001 sizes at 0x1e00000 EXACT, and the scan-loop cap exit sets 0x1e00000 exact (:11787-11788). RawConfig AEP=1 (shipped, og-codebase/config/RawConfig.ini:32) — enabled by default; hosts disable via the M-024 list ("aep"). The row: FMT_AEP keyed "RIFX" (bytes 52 49 46 58 — distinct from the little-endian RIFF family), ext "aep" (the og's ext-table "AEP" display form; the query layer's existing `{"aep", Category::Video}` surfaces it), floor 8 (the RIFX head the size read needs), cap 0x1e00000. No safety hardening was touched by this fix.
