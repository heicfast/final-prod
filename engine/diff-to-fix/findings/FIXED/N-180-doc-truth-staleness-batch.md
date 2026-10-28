# MINOR N-180 — the doc-truth staleness batch: five shipped-defaults/API claims contradicted by the current tree (plus the s5 polarity erratum)

**Severity:** MINOR
**Module:** docs / diff-to-fix (documentation accuracy — the preview + grading focus lanes' residue)
**Parameters affected:** #68 Documentation accuracy
**Corpus references:** (n/a — core-side truth defects; the corpus anchor for the corrected values: `re-workspace/artifacts/UIUX/10-preview-subsystem.md:22-27` (100 MB / 15 s), the M-077/M-080/M-109 evidence chain)
**Core references:** `docs/REFERENCE-API-GAP.md:222` + `:376` (R-28 rows: "no cr_ui thumbnail export yet … PARTIAL (ABI open)" + "32 MiB default budget per M-080"); `docs/REFERENCE-API-GAP.md:233` + `:382` (R-34 rows: "no shipped 100 MB size cap / 15 s video cap defaults"); `docs/ABI-CONTRACT.md:617-619` ("max_read 32 MiB (M-080)"); `diff-to-fix/API-DELTA.md:258` (S3-W4a row: "no shipped defaults (zero-init = uncapped today)"); `AUDIT-PIPELINE.md:172` (the stale "WRITER-1 (OPEN)" claim vs the same file's :179-186 FIXED block); `diff-to-fix/modules/exfat.md:36-38` (stale "with tz-offset conversion" vs its own session-2 re-audit at :93); `diff-to-fix/evidence/decompiled-quotes/s5-grading-extension-table-dump.md` §5 item 4 (the inverted polarity line — corrected by S-058) — versus the code truth: `include/cr/preview/cr_preview.h:125-126` (`max_file_bytes = 100u<<20`, `max_decode_ms = 15000`), `include/cr/preview/cr_thumbnail.h:97` (`max_read = 0xC800000` = 200 MB), `src/ui/cr_ui.cpp:3873-3975` + `include/cr/cr_ui.h:1233-1297` (the landed thumbnail ABI), `src/core/recovery/cr_safewriter.cpp:616-644` (per-item failure collection — WRITER-1's actual state) — all at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Six documentation rows still describe pre-landing states of the code: the thumbnail ABI is described as unexported (it landed in the e56baac7-lineage window), the thumbnail read budget is still documented as 32 MiB where the code pins 200 MB (M-109's re-pin updated "all four sites" except these), the preview caps are described as unshipped where M-077 shipped 100 MB/15 s defaults, the API-DELTA row never flipped, the WRITER-1 pipeline row says the writer fails on first per-item error (it collects), one exFAT module row still carries the refuted tz-offset text, and the s5 grading evidence's polarity line is inverted (the source of S-058's implementation defect). Each is a reader-misleading claim about the current tree.

## What the reference does

n/a — this is a core-internal doc-truth batch. The corrected values' corpus anchors: UIUX/10:22-27 (the 100 MB preview panel gate + 15 s line) and the M-109 still-image eligibility gate (DRWUI `:422425-429`, 0xC800000).

## What our core does

The code is correct on every axis (the ABI exists; the caps are 100 MiB/15 s/200 MB; the writer collects per-item failures; exFAT stamps local-as-UTC per S-011); only the docs lag. Verified cell-by-cell this session; the full table with both sides per row is in `diff-to-fix/evidence/s42-doc-truth-batch.md` (folded into the s42 worksheet set).

## Gap

Six doc sites assert states the tree disproves (see Core references for the exact line-by-line pairs).

## Impact

Doc-drift: an integrator reading ABI-CONTRACT or the gap matrix plans around an ABI that exists and caps that are one-quarter their documented size; audit consumers re-derive already-settled questions. No runtime effect.

## Reproduction / verification

Each pair verified first-hand this session by reading both the doc line and the code line cited above (the workbook is in the s42 evidence directory; the s5 polarity erratum is proven in S-058's worksheet).

## Fix plan (for the engineering team)

1. `docs/REFERENCE-API-GAP.md` R-28 rows (:222, :376): flip to PRESENT (the ABI landed; cite `cr_ui.h:1233-1297`); replace "32 MiB default budget per M-080" with "200 MB (0xC800000) per M-109".
2. R-34 rows (:233, :382): flip to PRESENT citing `cr_preview.h:125-126`.
3. `docs/ABI-CONTRACT.md:617-619`: "max_read 200 MB (0xC800000, M-109)".
4. `diff-to-fix/API-DELTA.md:258` (S3-W4a): flip the row's Ours column (pointer at :234 already says M-077).
5. `AUDIT-PIPELINE.md:172`: WRITER-1 state → FIXED (cite `cr_safewriter.cpp:616-644`; keep the historical note).
6. `diff-to-fix/modules/exfat.md:36-38`: strike the tz-offset phrasing (the :93 session-2 text is authoritative).
7. `s5-grading-extension-table-dump.md` §5 item 4: append the erratum note pointing at S-058 (the table dump §2-§3 stands).

## Tests to add

None (doc-only). CI note: no doc-lint exists for these files; the fix is the seven line edits.

## Related findings

S-058 (the polarity erratum's consumer), N-137/M-077/M-080/M-109 (the landings the docs missed), M-135 (WRITER-1's true semantics).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-180 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** per-row first-hand verification — each doc line re-read beside the code line it describes (`cr_preview.h:125-126` the 100 MiB/15 s defaults; `cr_thumbnail.h:76` the 200 MB max_read; `cr_ui.h`'s landed thumbnail ABI; `cr_safewriter.cpp:616-644` the per-item failure collection). Doc-only; the suite green at the landing. Item 7 (the s5 polarity erratum) was ALREADY landed by the S-058 Resolution — verified FIXED-ALREADY (the erratum block at the evidence file's :307 + the inline note at :258).
**Resolution notes:** Six rows re-truthed: (1) REFERENCE-API-GAP R-28 both rows (:222/:376) — the thumbnail ABI landed (cr_ui.h:1233-1297) and the read budget is 200 MB per M-109, not "no cr_ui export yet"/"32 MiB per M-080"; (2) R-34 both rows (:233/:382) — the 100 MB/15 s defaults SHIP (M-077), not "no shipped defaults"; (3) ABI-CONTRACT §7i's caps line — max_read 200 MB (0xC800000, M-109); (4) API-DELTA S3-W4a — the row's Ours column flipped to LANDED with the code cite; (5) AUDIT-PIPELINE's WRITER-1 row — the "still returns on first per-item error / OPEN" claim flipped to the collect-and-continue MATCH (the same file's own findings block below the row already said FIXED; the row had drifted); (6) modules/exfat.md's B9 — the refuted "with tz-offset conversion" phrasing replaced with the S-011 local-as-UTC truth. The N-180 verification confirmed the finding's core claim exactly: the code was correct on every axis; only the docs lagged.
