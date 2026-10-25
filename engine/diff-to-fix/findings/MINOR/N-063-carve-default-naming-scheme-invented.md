# MINOR N-063 — Carve default naming is a core invention ("File%06u.ext"); the og derives names per-analyser (content-embedded names + "Lost Name File (...)EaseUS" template) — OQ-S4-2 RESOLVED with a corpus-artifact correction

**Severity:** MINOR
**Module:** carve
**Parameters affected:** B10 (filename handling for nameless recoveries), A28 (analyser detail), Q68 (doc accuracy — a corpus artifact's own claim corrected)
**Corpus references:** OQ-S4-2 resolution (this session): `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:33312/33408/33447/33531` (MP4/MOV naming template "Lost Name File (<mvhd-derived metadata>)_EaseUS" + creation-time "%04d:%02d:%02d %02d:%02d:%02d"), `:35668` (CZeroEndRawFileInfo content-embedded name pattern), `:9055-9148` (the vtable+0x48 hook is a ONE-SHOT per-scan "filter_generalraw" config fetch, NOT a per-file namer); `re-workspace/artifacts/SearchDLL/rawsearchdll--family-brance.md` / deep-dive artifacts' "File0001.jpg lives in the UI" claim — **CORRECTED** (no FileNNNN scheme exists anywhere in the og)
**Core references:** `src/core/carve/cr_carve.cpp:376-387` (`default_namer` — "File%06u.%s", comment claims "corpus policy"), `:149` (per-file invocation) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core names every carved file `File%06u.<ext>` and the code comment claims corpus
policy. The og has no such scheme: names are assigned per-analyser at carve time —
content-embedded names where the format carries one (CZeroEndRawFileInfo pattern) and a
metadata-derived template for MP4/MOV ("Lost Name File (...)EaseUS"); the vtable+0x48 hook
session-4 tracked as the naming hook is actually a one-shot per-scan filter-config fetch
(key "filter_generalraw"), not a namer. The corpus's own deep-dive artifact claiming a
"File0001.jpg lives in the UI" scheme is wrong (corpus-errata candidate).

## What the reference does

Per the OQ-S4-2 decode (this session, decompile-cited): `SetName`-family functions per
analyser (FUN_1800173f0, ≥8 emit sites); MP4/MOV template quoted above; content-embedded
names via CZeroEndRawFileInfo for formats carrying names; the sole std::function call in
the DLL is the per-scan config fetch.

## What our core does

`default_namer` = `File%06u.%s` for every format, per file, with a comment asserting
"File000001.jpg - zero-padded for stable ordering (corpus policy)" — the asserted policy
does not exist in the corpus.

## Gap

Falsifiable: a carved MP4 in the og is named "Lost Name File (<creation date>)_EaseUS"
(metadata-derived); the core names it "File000123.mp4". A carved format carrying an
embedded name is named by that name in the og; the core ignores it.

## Impact

Presentation-level (names differ; content identical) — but the content-embedded-name
formats lose real information (the og surfaces the document's own title/name). MINOR with
a user-visible quality delta.

## Reproduction / verification

OQ-S4-2 agent decode this session with exact decompile addresses (quoted above); core
namer read first-hand; the "corpus policy" comment's claim searched and refuted across the
carve artifacts.

## Fix plan

1. Correct the comment (remove the false corpus-policy claim).
2. Add per-analyser naming hooks: MP4/MOV template per the og decode; a
   content-embedded-name extractor for the CZeroEndRawFileInfo format family (start with
   the formats whose grammars carry names — ZIP entries are S-032's inventory; document
   metadata types).
3. Keep `File%06u` as the fallback for formats with no og naming rule (register as a
   deliberate default).

## Tests to add

- `Carve.Mp4NamedByMetadataTemplate`
- `Carve.EmbeddedNameFormatsSurfaced`
- `Carve.FallbackNamerDocumented` (the deliberate default)

## Related findings

- S-032 (archive inventory — the embedded-name work shares the analyser-detail wave)
- M-029 addendum (the naming correction folds into the query-model doc family)
- CORPUS-ERRATA (the deep-dive artifact's "File0001.jpg" claim — errata row this session)

## Open questions

The DRWUI merge-rename path for CRAWFileProperties names (how carved names interact with
structured-tree merge — the corpus's "shot0017.jpg" samples) has a residual decode gap
(vftable 0x140f5f528 users); the engine-side naming contract above is fully pinned.
