# MINOR N-115 — the og's OfficeTagFromXML ZIP-inner classifier is only partially mirrored: core's DOCX gate checks the first-entry [Content_Types].xml name only — the og decompresses every entry (10 KiB each) and discriminates DOCX/XLSX/PPTX/ODF/EPUB/iWork/HWP/CDR by inner tag paths plus metadata extraction

**Severity:** MINOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A28-adjacent (inner-entry typing), B13-adjacent (typing affects the carved row's format id), A1-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:53202-53224` (the OfficeTagFromXML singleton ctor, 0x2801 = 10 KiB scratch buffer); :53164-53194 (the per-entry decompress + tag scan: `word/_rels/document.xml.rels` → DOCX id 0x49, `xl/_rels/workbook.xml.rels` → XLSX 0x4a, `ppt/_rels/presentation.xml.rels` → PPTX 0x4b); the wider tag corpus (floss strings :6699-6729 + strings table :768-905): application/vnd.oasis.opendocument.{text,spreadsheet,presentation} (ODF), application/epub+zip + OPS/chapter1.html (EPUB), Index/*.iwa + index.apxl (Apple iWork), application/hwp+zip (Hangul), content/riffData.cdr (CorelDRAW), xsd/MindManagerApplication.xsd, xmlns="urn:xmind, "libopenshot":, ProjectFolder/project_info.json, MediaPool/Master/MpFolder.xml, plus the metadata tags dc:creator/dc:title/meta:initial-creator/dcterms:*; the ZIP size calc consults it ("Exception:ZIP CalcRawSize(%d), office?%d", strings :779).
**Core references:** audited commit `ff879f9` — `src/core/carve/cr_analysers.cpp:827-854` — the DOCX row re-validates `[Content_Types].xml` as the FIRST ZIP entry name only; no xlsx/pptx rels discrimination (they ride the same OOXML row), no ODF/EPUB/iWork/HWP/CDR inner tags, no metadata extraction. S-032 (FIXED) landed the archive inner-entry inventory; the typing half is the residual.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-032 (FIXED, the archive inner-entry inventory) + the DOCX first-entry gate; S-007's batch-2 notes the OfficeTagFromXML deferral. No prior finding covers the typing gap.

## Summary

The og's ZIP-path classifier decompresses each inner entry (up to 10 KiB) and types the container by its inner tag paths — splitting OOXML into DOCX/XLSX/PPTX by the rels path, and recognizing ODF/EPUB/iWork/Hangul/CorelDRAW and a dozen niche project formats, plus dc:title/dc:creator metadata. Core types OOXML via the first entry's [Content_Types].xml name and nothing else. The gap is typing fidelity (the data still carves, but under coarser ids) and the metadata channel. MINOR: the files ARE recovered; the typing/metadata surface is what the og has that core lacks.

## What the reference does

See corpus field — the singleton walks ZIP entries, decompresses <=10 KiB each, matches the rels-path map for the OOXML trio, and the wider tag corpus for the rest; the ZIP size calculator consults the classifier (office? flag).

## What our core does

The [Content_Types].xml first-entry gate only; no per-entry decompression scan, no rels-path split, no ODF/EPUB/iWork/HWP/CDR tags, no metadata extraction.

## Gap

Falsifiable: an XLSX (whose [Content_Types].xml may not be the literal first entry in a damaged zip) is still typed XLSX by the og's per-entry scan; core either misses the OOXML typing or mislabels it. An ODS (ODF spreadsheet) is typed by the og; core carves it as a generic zip.

## Impact

Format-id fidelity in the carved tree (host displays and downstream filters key on ids); damaged-zip salvage (the og tolerates entry reordering); the metadata channel (title/creator) is a recovery-report feature the og surfaces.

## Reproduction / verification

Decompile-direct: the singleton + the tag strings are cited above; the string tables cross-check in the corpus strings dumps.

## Fix plan (for the engineering team)

1. Extend the OOXML validation (or add a post-inventory typing pass, the S-032 seam) to decompress entries up to 10 KiB and match the rels-path map (word/xl/ppt). 2. Add the ODF/EPUB/iWork/HWP/CDR tag rows. 3. Capture dc:title/dc:creator into the carved node's metadata when present (the tree already has a metadata channel if the M-032 inventory landed one — else record as the follow-up). 4. Keep the [Content_Types] gate as the fast path.

## Tests to add

- `Carve.OoxxmlRelsPathSplit` — DOCX/XLSX/PPTX plant correctly by rels path.
- `Carve.OdfEpubIworkTyped` — the tag rows fire.
- `Carve.ZipEntriesReorderedStillTyped` — the damaged-zip tolerance.
- `Carve.MetadataCaptured` — dc:title lands in the node metadata.

## Related findings

S-032 (the inner-entry inventory this extends); S-007 (batch-2 parent); M-093 (XML — the declaration grammar the tags sit behind).

## Open questions

Whether the carved-node metadata channel exists in core's tree model for dc:title/creator — if absent, that half becomes a tree-schema follow-up (recorded; not blocking the typing rows).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-115+N-153 landing — recorded post-push in the ledger row)
**Verified by:** `Carve.OoxmlRelsPathSplit` (the three rels paths stamp FMT_DOCX/FMT_XLSX/FMT_PPTX with their extensions), `Carve.ZipEntriesReorderedStillTyped` (a [Content_Types].xml that is NOT the first entry still types — the old gate's damaged-zip miss), `Carve.OdfEpubTypedByMimetype` (the ODF trio + EPUB + HWP by the mimetype entry's content, stored or deflated ≤10 KiB via the compress layer's kGzip), `Carve.IworkAndCdrTypedByEntryName` (the iWork Index/.iwa family + index.apxl + CorelDRAW's content/riffData.cdr), `Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip` (an office zip the map cannot refine stays DOCX — the og's dispatch id 0x49; an untyped zip stays the generic row). All RED pre-fix (the formats/discrimination did not exist). Suite 1119/1119 (1077 pass / 42 env skips / 0 fail, linux-release); cr_ui_tests 61/61.
**Resolution notes:** The og's OfficeTagFromXML typing pass ported (RAWSearchDLL :53164-53194 re-read first-hand): the office branch of zip_common now walks the LFH chain (bounded 32 entries / 256 KiB — the typed names live in the first entries of real files; the og's window bounds it similarly) matching the per-entry rels-path map (word/xl/ppt + the ≥5-char "word"-prefix fallback, office-gated exactly as the decompile's param_3 flag), the mimetype/[Content_Types].xml content tags (≤10 KiB, stored directly or deflated through compress::decompress_block — the og's 0x2800 scratch), and the name-tag set (iWork, EPUB's OPS/chapter1.html, CDR). Nine override-target rows appended (FMT_XLSX/FMT_PPTX — the og's ids 0x4a/0x4b — and FMT_ODT/ODS/ODP/EPUB/IWORK/HWP/CDR; the og's id mapping for the non-OOXML families is undecoded, documented in the enum). The [Content_Types].xml fast path is preserved (first-entry hits short-circuit as before); the first-entry REQUIREMENT is gone (the og matches names anywhere — the finding's damaged-zip tolerance). The metadata half (dc:title/dc:creator into the node) is the recorded follow-up exactly as the finding's open question anticipated: the tree carries no general metadata channel (the N-063 embedded_name is the analyser-name surface, not a report-metadata surface; the og's naming rule for office files is undecoded) — the typing rows land without it, per the finding's own "not blocking the typing rows". **N-153 (NEW) was discovered and fixed during this verification**: multi-entry archives double-carved every inner LFH position (the pre-existing M-117 dedup gap; see N-153's file) — the family's three rows + the nine new ones now carry suppress_overlap, and the S-032 inventory (the og's own presentation of inner entries as children) is what the tree shows instead.
