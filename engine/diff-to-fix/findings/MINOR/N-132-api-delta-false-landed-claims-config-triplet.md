# MINOR N-132 — API-DELTA rows 44-45 claim "landed" for three config surfaces that are absent: TxtViewFiles, SumaryFileType, PriorityDisplayType — contradicting REFERENCE-API-GAP's honest R-31 row

**Severity:** MINOR (doc-truth + registration; the TxtViewFiles behavioral half is owned by M-103)
**Module / Surface:** gate docs (API-DELTA §1.7) / query+summary model / config-layer registration
**Parameters affected:** P68 (documentation accuracy — false "landed" claims), P46 (contract truth), B13-adjacent (report/summary schema — the unification groups), B17-adjacent (the summary-count semantics)
**Corpus references:** `og-codebase/config/FilterFolder.ini` (`SumaryFileType=aiff_aif;m4p_m4a_m4v_mp4;html_htm;rm_rmvb;mpg_mpeg_mov;mkv_webm;asf_wma_wmv;jpg_jpeg;tif_tiff;ai_pdf;zip_docx_pptx_xlsx;`); `og-codebase/config/PriorityDisplayType.xml` (5 categories, 35 extensions, 2-digit alias sort keys); `re-workspace/artifacts/config-files/filtering-priority--config-layer.md:19-31,40-41,49` (both decoded), `:66-74` (the filtering pipeline synthesis)
**Core references:** audited commit `84a1832` — `diff-to-fix/API-DELTA.md:108-109` (rows 43-45); the absences: `src/core/query/cr_filefilter_data.cpp` (no unification groups; the only `m4p` hit :732 is a plain category row), `include/cr/tree/cr_grading.h:115` (`u64 category_counts[8]` — plain per-category counter), `src/core/tree/cr_grading.cpp:247` (the only summary-count site); 0 hits for alias/priority_display/display_order in query/tree (grep record: `evidence/decompiled-quotes/s10-config-layer-absence-greps.md`)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (push-time reconciliation):** filed as N-128 in this lane's working tree; renumbered **N-132** at the second push-time reconciliation — the parallel SA-10 lead lane had concurrently filed N-128 (capability-matrix-ci-row-truth); the earlier remote numbering wins.
**Prior audit check:** The trail's honest register is R-31 (`docs/REFERENCE-API-GAP.md:225`: "**Missing:** … usefullostfiles.xml whitelist promotion; PriorityDisplayType display ordering") — but usefullostfiles has since received an explicit OOS-UI row (`docs/PARITY-OG-CORE-INVENTORY.md:65`, sole consumer DRWUI:320396), while PriorityDisplayType got NO disposition, and SumaryFileType/TxtViewFiles got FALSE "landed" claims instead.

## Summary

API-DELTA §1.7 rows 44-45 state that TxtViewFiles (119-ext), SumaryFileType (unification groups), and PriorityDisplayType (presentation order) "landed". None of the three exists in the core. The register contradicts REFERENCE-API-GAP R-31, which truthfully lists them as missing components. The preview-behavior consequence of the TxtViewFiles absence is M-108; this finding owns the doc-truth defect and the two unowned config models.

## What the reference does

`filtering-priority--config-layer.md` decodes the og's config-driven filtering pipeline: FilterFolder.ini (noise suppression — landed in the core, byte-exact), FileFilter.xml (membership — landed, 1481 rows), **PriorityDisplayType.xml** (presentation order within category — 5 categories/35 extensions with 2-digit alias sort keys driving the Type-tree ordering), **SumaryFileType** (extension unification for counts — `m4p_m4a_m4v_mp4` counted as one family; `zip_docx_pptx_xlsx` as the office-in-zip family; "the groupings follow container identity, not just extension"), **TxtViewFiles** (text-preview whitelist — see M-103).

## What our core does

- TxtViewFiles: absent (M-103 owns the behavior; the row-44 "landed (cr_filterfolder_data.cpp)" citation points at a file that contains only the three junk pools — read in full this session, 65 lines).
- SumaryFileType: absent — the summary counts are the plain 8-bucket `category_counts` (cr_grading.h:115); no family unification exists anywhere (grep evidence filed).
- PriorityDisplayType: absent and undispositioned — no alias/sort-key surface; no OOS-UI row; no PARITY-DELTAS row.

## Gap

Falsifiable: `rg "TxtViewFiles|SumaryFileType|PriorityDisplay" src/ include/` → 0 code hits, while API-DELTA rows 44-45 say "landed". REFERENCE-API-GAP R-31 says Missing. The two gate docs cannot both be true.

## Impact

Engineering teams executing against the board read API-DELTA §1.7 as the data-asset scoreboard; a false "landed" row forecloses work on a real gap (exactly the SCAN-ENGINE-DELTA false-"fixed" precedent that became N-108). The summary-count semantics divergence (plain 8-bucket vs family-unified) is user-observable in any host that renders "MP4 family: N files" style counts.

## Reproduction / verification

Corpus-doc + core-grep cross-verification, both directions, this session: `evidence/decompiled-quotes/s10-config-layer-absence-greps.md` (the grep record + the byte-exact re-verification of what DID land — the junk pools and the 1481-row FileFilter table, proving the method finds present things too).

## Fix plan (for the engineering team)

1. Correct API-DELTA row 44 to: "FilterFolder.ini noise blocklists + FatFilterName 14 locales — landed; **TxtViewFiles — NOT landed, see M-108**". Correct row 45 to: "usefullostfiles.xml — OOS-UI (registered); **PriorityDisplayType — registration required; SumaryFileType — registration required**".
2. Decide the two dispositions: (a) PriorityDisplayType → OOS-UI row in PARITY-OG-CORE-INVENTORY (same class as usefullostfiles: its consumer is the DRWUI type-tree rendering, CLeftTreeView/CClassifyLeftTreeView per the artifact :76) unless the CLI `tree` output wants category-internal ordering parity; (b) SumaryFileType → either land a `family_of(ext)` unification helper beside the category map (a ~11-group table, generated-data precedent) feeding `category_counts` or a parallel `family_counts`, or register the plain 8-bucket counter as a deliberate delta in PARITY-DELTAS with the count-semantics note.
3. Sync R-31's row to the outcomes so the two registers agree.
4. Acceptance: `rg` over both docs shows no "landed" claim without a code cite that greps true.

## Tests to add

- If SumaryFileType lands: `Grading.FamilyCountsUnifyM4pM4aM4vMp4` (4 extensions → 1 family count) + the generated-table byte-exactness test.

## Related findings

**M-108** (the TxtViewFiles behavior) · **M-018** (landed half of row 44) · **M-023/M-029** (the landed FileFilter/query layers row 45 partially leans on) · **N-108** (the false-"fixed" precedent class) · **N-101** (citation-truth sweep class)

## Open questions

None. (Whether the CLI `tree` wants PriorityDisplayType ordering is a product decision recorded in the fix plan, not an evidence gap.)


---

## Session-11B addendum (2026-10-29): cite refresh + the propagating aggravation

The false rows are now at `diff-to-fix/API-DELTA.md:110-111` (was :108-109 — insertions above). **Aggravation:** the false "landed" claim now propagates into the PRIMARY gap table — `docs/REFERENCE-API-GAP.md:225` (R-31 primary) claims PRESENT citing "PriorityDisplayType ordering (the query/filefilter data layers, API-DELTA row 45)" while the second-table `:379` still says PARTIAL — the document self-contradicts on the strength of this finding's false row. The fix plan's step 1 (flip rows 44-45 to the truthful state) now also requires the `:225` refresh.


---

## S11-R addendum (2026-10-29, the reconciliation lane; re-verified at `b6e0a9ef`)

**Aggravation re-verified at `b6e0a9ef` (this lane's unique product):** the
false "landed" claim PROPAGATED into the product gap-board — at the filing
baseline `docs/REFERENCE-API-GAP.md:225` (R-31 primary) honestly read
"PARTIAL | Missing: … PriorityDisplayType display ordering"; the doc-truth
batch-A landing (`e4f3184`) flipped it to "PRESENT | … PriorityDisplayType
ordering (the query/filefilter data layers, API-DELTA row 45)" — citing the
false API-DELTA row as its source. The row still reads that way at the
current HEAD (re-read this session): the two gate docs AGREE with each other
and are BOTH false vs the code (the repo grep stays zero). The fix must
revert/correct R-31's display-priority clause in addition to rows 44-45 —
N-150's batch owns the row-state mechanics; this addendum records that the
spread is verified current, not historical.
---

## S41 addendum (2026-10-29, the continuation audit lane — the TxtViewFiles leg RESOLVED by the M-108 landing; the finding re-scoped to the standing remainder)

The liveness re-verification at core `c7fa527c` (this lane's T4-T7 battery, first-hand)
found the TxtViewFiles leg **landed**:

- **Row 44 corrected at the landing:** `diff-to-fix/API-DELTA.md` row 44 now reads
  "TxtViewFiles landed at the M-108 fix (`cr_preview_text_exts_data.cpp` — the preview
  eligibility pool, byte-exact)" — the false-"landed" claim for this row is gone, and the
  correction also repaired the finding-time "119-ext" figure to the artifact's own 97-token
  count (538 bytes, 92 unique case-folded).
- **The code surface exists:** `src/core/preview/cr_preview_text_exts.cpp` +
  `cr_preview_text_exts_data.cpp` (the generated whitelist pool), gated at
  `src/core/preview/cr_preview.cpp:1061` and declared at
  `include/cr/preview/cr_preview.h:147-165` — all carrying the M-108 marker. Verified:
  `rg "TxtViewFiles" src/ include/` → 10 hits, every one at the M-108 sites.

**The standing remainder (this finding's scope, unchanged and convergent with the S11-R
addendum above):** row 45 still claims "PriorityDisplayType.xml / usefullostfiles.xml /
SumaryFileType groups — landed (query/filefilter data layers)"; the S11-R addendum's
aggravation record (the false claim propagated into REFERENCE-API-GAP R-31) compounds
it. Re-verified at `c7fa527c`: `rg "SumaryFileType|PriorityDisplay" src/ include/` →
**0 code hits**; `include/cr/tree/cr_grading.h` still carries the plain 8-bucket
`category_counts` with no family unification. The finding stays ACTIVE, re-scoped to
row 45 + R-31's propagated clause + the two config-model dispositions.

**Core references re-pinned:** audited `c7fa527c` (this addendum's cites; the original
file's cites remain the `84a1832` filing-time record).
