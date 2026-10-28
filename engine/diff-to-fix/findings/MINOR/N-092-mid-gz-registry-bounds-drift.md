# MINOR N-092 — MID/GZ registry bounds drift from the og ctors (64 MiB vs 10 MB; 512 MiB/min-18 vs 400 MB/min-20)

**Severity:** MINOR
**Module / Surface:** carve (the RAW-signature layer — batch-1 format bounds)
**Parameters affected:** C19 (magic tables — the min/max cap columns), B3-adjacent (false-positive pressure at the caps)
**Corpus references:** `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c` :21110-21127 (the MID ctor: min 14, max 0x989680), :20650-20666 (the GZ ctor: min 20, max 0x17D78400) — verbatim quotes in `evidence/decompiled-quotes/s9-mid-gz-bounds.md`
**Core references:** `src/core/carve/cr_registry.cpp:691` (FMT_MID: min 14, max `64ull << 20`), `:781` (FMT_GZ: min 18, max `512ull << 20`) — audited commit `35adcb9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered (the S-007 batch-1 landing `e018bf4` never claimed og bounds for MID/GZ — the census-derived maxima were quoted in SCAN-ENGINE-DELTA's A4 row ("MID … 14B..10MB", "GZ … max 400MB") but the registry rows shipped different values with no note)

## Summary

Two batch-1 formats shipped with registry caps that differ from the og's
FileInfo ctor bounds: MIDI accepts up to 64 MiB where the og caps at
10 MB (0x989680), and gzip accepts an 18-byte minimum and a 512 MiB cap
where the og requires 20 bytes minimum and caps at 400 MB (0x17D78400).
Neither divergence was claimed as og-exact (unlike WPG/MDB, whose bounds
the landing verified at their ctors), but both are og-registered formats
whose caps now drift without a recorded reason.

## What the reference does

The MID analyser's ctor (retdec :21110-21127, format id 38) sets min 14
and max 0x989680 = 10,000,000. The GZ analyser's ctor (:20650-20666,
format id 31) sets min 20 and max 0x17D78400 = 400,000,000. (The two
ctor families store the bounds at different member offsets — the MID
family at +216/+272/+280, the GZ family at +216/+272 — cross-checked
against the MDB/WPG ctors; the decoded values are what the analysers
enforce.)

## What our implementation does

`cr_registry.cpp:691`: `{FMT_MID, "mid", {'M','T','h','d'}, 4, 0, 14,
64ull << 20, 800, validate_mid, "MIDI"}` — min matches (14), max is
67,108,864 (6.7x the og).
`cr_registry.cpp:781`: `{FMT_GZ, "gz", {0x1F,0x8B}, 2, 0, 18,
512ull << 20, 600, validate_gz, "gzip"}` — min 18 (ours accepts 18-19
byte headers the og rejects), max 536,870,912 (1.34x the og).

## Gap

Falsifiable: the registry's declared (min, max) for FMT_MID is (14,
67108864) vs the og's (14, 10000000); for FMT_GZ (18, 536870912) vs
(20, 400000000).

## Impact

Superset behavior only (we accept strictly more than the og at both ends
that differ): a marginally larger false-positive surface at the caps and
slightly longer extent walks for degenerate inputs; no correctness loss.
The audit-trail cost: the caps are og-registered values that drift
silently — the same class the S-007 batch took pains to avoid for
WPG/MDB (whose bounds were verified at the ctors and carried exactly).

## Reproduction / verification

1. The ctor quotes (evidence file `s9-mid-gz-bounds.md`, read first-hand).
2. The registry rows (read first-hand, cited above).
3. The SCAN-ENGINE-DELTA A4 row's own census text ("MID (`MThd` id 0x26,
   BE grammar, 14B..10MB"; "GZ (u16 `1F 8B` id 0x1f, max 400MB)") — the
   trail already states the og values; the registry disagrees.

## Fix plan (for the engineering team)

1. One-line registry change (or a deliberate-superset note).
2. File: `src/core/carve/cr_registry.cpp:691` and `:781`.
3. The change: FMT_MID max → `10'000'000`; FMT_GZ min → 20, max →
   `400'000'000`. Alternatively keep the larger caps and add a comment +
   PARITY-DELTAS row documenting them as deliberate (looser salvage
   bounds) — but the og-exact values are the parity default.
4. Tests: `N092MidCapMatchesOg` (a >10 MB MIDI stream's extent clamps at
   the og cap), `N092GzMinMatchesOg` (an 18-byte gzip header is not
   carved), `N092GzCapMatchesOg` (a >400 MB stream clamps). Fail before,
   pass after.
5. Doc: the S-007 FIXED addendum gains the one-line correction (its
   census quotes were right; the rows drifted).

## Tests to add

- The three tests above.

## Related findings

- S-040 — the same landing's WMF defect (the batch-1 verification
  discipline's other gap).
- S-042 — the remaining batch-2 set.

## Open questions

None.
