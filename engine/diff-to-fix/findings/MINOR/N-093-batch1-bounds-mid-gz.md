# MINOR N-093 — Batch-1 carve bounds: MID and GZ diverge from the og's registered caps without a documented deviation

**Severity:** MINOR
**Module:** carve
**Parameters affected:** C19-adjacent (the bounds alongside the magic), A28 (validator bounds), Q68 (the heuristic's documentation)
**Corpus references:** `RAWSearchDLL_decompiled.c` FileInfo ctors (read first-hand by this session's verification lane): `MIDRawFileInfo` :16326-16327 (`[0x1b]=0xe, [0x23]=10000000` — min 14, max **10,000,000**); `GZRawFileInfo` :15824-15829 (`[0x1b]=0x14, [0x22]=400000000` — min 20, max **400,000,000**); `PSPRawFileInfo` :17373-17378 (36 / 20,000,000); `SITRawFileInfo` :16886-16892 (20,480 / 200,000,000); `WPG` ctor :20542-20548 (20 / 100,000,000)
**Core references:** audited `35adcb9`, `src/core/carve/cr_registry.cpp` (the batch-1 block, verified first-hand): MID :691 `{14, 64ull<<20}` (67.1 MB vs og 10 MB); GZ :781 `{18, 512ull<<20}` (536.9 MB vs og 400 MB; min 18 vs 20); PSP :766-767 `{32, 128 MiB}` (vs 36 / 20 MB); SIT :776-777 `{16, 512 MiB}` (vs 20,480 / 200 MB). The S-007 record's documented-heuristic list ("Formats whose og size-walk is undecoded … DWG-pattern bounded extent") names WPG/WPD/PSP/SIT/APE/X3F/MRW/HLP/MDB/EPS/AI/DXF — **MID and GZ are not on it**, and PSP/SIT's og bounds were in fact present at their ctors.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The batch-1 verification re-read the og's per-format FileInfo ctors and found four bounds divergences from the registered core rows — all in the detection-superset direction (smaller min, larger max), so nothing is missed, but the og's tighter caps (which bound false-positive extent and walk time) are not carried. PSP and SIT fall under the S-007 record's documented bounded-extent heuristic; MID (max 67 MB vs og 10 MB) and GZ (both bounds) deviate with **no documented rationale** — their og bounds were decoded and available at landing time.

## What the reference does

The og registers per-format min/max in the FileInfo ctors (the cites above) — e.g., MIDI capped at 10 MB, gzip at 400 MB — bounding both FP surface and walk cost.

## What our core does

MID `{14, 64 MiB}`, GZ `{18, 512 MiB}`, PSP `{32, 128 MiB}`, SIT `{16, 512 MiB}` — supersets, two documented (by the heuristic note), two not.

## Gap

The four bound pairs differ from the og's registered values; only two have a documented reason. Falsifiable: a 60 MB `MThd`-keyed garbage blob carves in core (bound 64 MiB) where the og's 10 MB registration rejects it.

## Impact

False-positive exposure and walk time on adversarial/keyed-garbage inputs (superset direction only — no recall loss). Low practical risk (the validators' grammar checks still gate), but the og's caps are cheap insurance the core declined without recording why on two formats.

## Reproduction / verification

Executed-class: the og ctor reads (decompile, first-hand, line-cited) vs the core rows (read first-hand) — the comparison table is the verification; no behavioral execution needed (the bounds are row constants).

## Fix plan (for the engineering team)

1. Carry the og bounds for MID `{14, 10,000,000}` and GZ `{20, 400,000,000}` (two-line registry change; the og values are cited above).
2. Either carry PSP `{36, 20,000,000}` and SIT `{20,480, 200,000,000}` too, or document the deliberate superset in the S-007 heuristic note (name the formats and the og values it overrides — currently it implies the og bounds were unknown when PSP/SIT's were present at the ctors).
3. Add bounds to the batch-1 test table (each format's plant already exists — assert the boundary: at-max carves, over-max rejects).

## Tests to add

- `Carve.MidBound10MB` (at 10 MB carves, 10 MB+1 rejects).
- `Carve.GzBounds400MB` (both edges).
- `Carve.PspSitBoundsPinned` (whichever values land).

## Related findings

- S-007 (FIXED — batch-1; this finding pins its bounds tail).
- S-041 (this session — the batch-2 remainder; same registration-bounds discipline for the future landings).

## Open questions

None.
