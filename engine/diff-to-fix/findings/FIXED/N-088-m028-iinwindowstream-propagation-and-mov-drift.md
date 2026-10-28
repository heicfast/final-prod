# MINOR N-088 — M-028's IInWindowStream correction never propagated: the ABI header and the API-surface audit still call it "the reference's preview contract"; the preview Metadata comment lists a never-produced `mov` format

**Severity:** MINOR
**Module / Surface:** preview / documentation accuracy (modules/preview.md; Q68/P46)
**Parameters affected:** Q68 (documentation accuracy), P46 (documentation of the contract)
**Corpus references:** (via M-028's proven resolution, FIXED) `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:800729-800735` — **IInWindowStream = the 7-Zip/LZMA SDK input interface** (vftable adjacent CLZInWindow/IMatchFinder; zero occurrences in any other reversed tree) — NOT a preview streaming contract
**Core references:** audited commit `dd82db9` — `include/cr/cr_ui.h:1035-1036` ("the reference's IInWindowStream preview contract: range reads under explicit caps"); `docs/API-SURFACE-AUDIT.md:94` ("Preview stream read (`IInWindowStream`) ... MATCH"); `include/cr/preview/cr_preview.h:116` (the Metadata `format` comment lists `mov` — `rg '"mov"' src/core/preview/cr_preview.cpp` → zero producers)
**Status:** FIXED — see Resolution below
**Prior audit check:** expanded on prior finding M-028 (FIXED — which corrected the same misattribution in `cr_preview.h:10-11` and the 4 MiB provenance, but did not sweep the two remaining doc sites or the format-comment drift)

## Summary

M-028 proved that `IInWindowStream` is the 7-Zip/LZMA decoder's input interface, not a preview contract, and corrected the preview header's own comments — but the correction never reached the two outward-facing documents that still present the misattribution as the ABI's provenance (the cr_ui.h block comment and the API-SURFACE-AUDIT row, the latter rating the misnamed contract "MATCH"), and the preview Metadata's format comment advertises a `mov` kind the implementation never produces. Future hosts reading the ABI docs will inherit the falsified provenance M-028 already paid to correct.

## What the reference does

(M-028's resolved fact, cited above: IInWindowStream is LZMA SDK plumbing — the og's preview streaming has no such named contract; the og's real preview caps are the 100 MB/15 s UI-policy bounds and the 1 MiB payload gate.)

## What our implementation does

The code is correct (M-028's fix landed); the drift is purely documentary: `cr_ui.h:1035` names "the reference's IInWindowStream preview contract" as the design's provenance; `API-SURFACE-AUDIT.md:94` repeats the name in a MATCH-rated row; `cr_preview.h:116`'s format comment includes `mov` with no producer.

## Gap

Falsifiable claim: two shipped docs attribute the preview read contract to IInWindowStream (falsified by M-028), and one header comment lists a format constant never set.

## Impact

- Documentation truthfulness only: a host integrator reading the ABI header or the audit doc inherits a provenance claim the audit trail has already falsified; the `mov` entry misleads implementers about supported preview kinds. No behavior is wrong.

## Reproduction / verification

Core evidence (strength-3): the three quoted sites (read this session); `rg '"mov"' src/core/preview/cr_preview.cpp` → no hits. The falsification is M-028's already-executed resolution (cited).

## Fix plan (for the engineering team)

1. `cr_ui.h:1035-1036`: replace the provenance phrase with the true contract ("bounded, chunked range reads under explicit caps — the og's preview payload gate is 1 MiB (DRWUI_decompiled.c:654557/654601), the 100 MB/15 s bounds are UI policy (M-028)").
2. `docs/API-SURFACE-AUDIT.md:94`: drop the IInWindowStream name; point the row's provenance at M-028's resolution.
3. `cr_preview.h:116`: remove `mov` from the format comment (or implement it — but the corpus's preview table lists no mov-specific previewer; removal is the parity-true fix).
4. A one-line grep guard in the doc-drift test family (N-020/N-074 pattern): `rg -l "IInWindowStream" include/ docs/` must return only historical-audit contexts.

## Tests to add

- (Doc-drift guard) the grep above wired into the existing doc-truth test if one exists, else a scripts/ check.

## Related findings

- **M-028** (FIXED) — the original misattribution + its executed proof; this finding is the propagation tail.

## Open questions

None.

## Resolution (FIXED — all three drift sites corrected + a grep guard)

1. `include/cr/cr_ui.h` preview block: the "the reference's
   IInWindowStream preview contract" provenance claim replaced with
   the preview-true statement + M-028's correction (IInWindowStream =
   the 7-Zip/LZMA SDK input interface, not a preview surface).
2. `docs/API-SURFACE-AUDIT.md:94`: the row's contract name no longer
   cites IInWindowStream; provenance points at M-028's resolution.
   `docs/REFERENCE-API-GAP.md` R-33 also corrected (the og's preview
   streaming is PreviewDataSource/IPreviewSink; IInWindowStream removed
   from that row — it remains in R-42 where it correctly describes the
   og export loop's engine-stream reads).
3. `include/cr/preview/cr_preview.h:134`: `mov` removed from the format
   comment (no producer; the corpus's preview table lists no
   mov-specific previewer).
4. **The grep guard** (the finding's own acceptance item):
   `PreviewDocs.NoIInWindowStreamDriftAndNoMovGhost` reads the three
   contract sites and asserts every IInWindowStream mention sits inside
   a correction note naming the 7-Zip/LZMA SDK, plus the format comment
   carries no `|mov`. REFERENCE-API-GAP's og-architecture rows are
   exempt (R-42 describes the og's actual use).

**Landed:** same commit as M-066.
