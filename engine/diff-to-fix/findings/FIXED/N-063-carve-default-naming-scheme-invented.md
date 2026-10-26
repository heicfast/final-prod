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

## Resolution

**Status:** FIXED (the decode-verified scope: comment repair + the MP4/MOV per-analyser template + the embedded-name carrier + the deliberate-default registration; the CZeroEnd-family membership beyond MP4/MOV is the recorded remainder with a precise next step)
**Fixed in commit:** (this session's N-063 commit, DFX17)
**Verified by:** `CarveEngine.Mp4NamedByMetadataTemplate` (mvhd v0 1738843200 → "Lost Name File (2025:02:06 12:00:00)_EaseUS"), `CarveEngine.MovNamedByMetadataTemplate` (v0 time 0 → the epoch form), `CarveEngine.MovV1MvhdHighWordQuirkReplicated` (v1 high-word quirk pinned — never silently "corrected"), `CarveEngine.FallbackNamerDocumented` (the File%06u deliberate default stays for rule-less formats). Fail-before EXECUTED (emit-hook-disabled pre-fix simulation: 3/3 naming tests RED); pass-after 679/679 + 24/24 UI.
**Resolution notes:** Corpus re-verified FIRST-HAND at RAWSearchDLL_decompiled.c: the template literals "Lost Name File (" / ")_EaseUS" (:33312/:33408, assembled :33412-33434), the time format "%04d:%02d:%02d %02d:%02d:%02d" via _localtime64 (:33465/:33531), and the field reads — mvhd v0: the BE u32 at +0x10 (:33322-33327); mvhd v1: the FIRST FOUR BYTES of the u64 at +0x14 (:33335-33338, the high 32 of the BE value). Two decode corrections to the finding recorded: (a) the field is the MODIFICATION time under the ISO layout (v0 mod@+16, v1 mod@+20u64) — the finding's "creation-time" label corrected; (b) there is NO 1904 MP4-epoch conversion in the og (the raw value feeds _localtime64 as Unix seconds). Both og quirks (the no-epoch shift and the v1 high-word truncation that formats every sane v1 time as 1970:01:01) are REPLICATED faithfully and pinned by test. Implementation: `CarveResult::embedded_name` (the per-analyser name carrier, emit precedence embedded_name > namer > default), `mp4_og_name()` in the analysers (moov→mvhd walk, version-branch field reads, gmtime_r formatting — the UTC-vs-host-local divergence registered as PARITY-DELTAS D-76 alongside the deliberate File%06u default); the false "corpus policy" comment on default_namer repaired. REMAINDER (recorded, not silent): the CZeroEndRawFileInfo content-embedded-name family beyond MP4/MOV — which of the 22 SetName call sites extract content names (vs metadata templates) is not decoded per-format in the corpus; the next concrete step is decoding each FUN_1800173f0 caller shape the way this fix decoded the MP4/MOV analyser, wiring each onto embedded_name as its evidence lands.
