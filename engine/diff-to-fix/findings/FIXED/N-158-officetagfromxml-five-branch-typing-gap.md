# MINOR N-158 — OfficeTagFromXML's five remaining ZIP-content typing branches (XMIND / MMAP / EDDX / WFP / DRP) absent from core with no owning finding — the N-115 landing's uncovered tail

**Severity:** MINOR
**Module:** carve (RAWSearchDLL ZIP typing parity surface)
**Parameters affected:** P1 (detection correctness — five typed formats surface as generic ZIP), P11 (node fields — the typed format id + display label), P19 (magic/content keys: the content/entry-name branches), P25 (engine algorithm steps — the typing walk's branch set), P65 (test coverage — zero tests for the five branches)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :53025-53100 (the five branches, first-hand this session — the ZIP per-entry typing walk inside `FUN_180006b30`'s rebuild loop): the `FUN_180045110` content-match branch stamping id **0x86 + L"XMIND"** (:53029-53035); the entry-name `xsd/MindManagerApplication.xsd` compare stamping **0x87 + L"MMAP"** (:53041-53065); `rels/_rels.xml` OR `rels/page1_rels.xml` stamping **0x8c + L"EDDX"** (:53066-53074); `ProjectFolder/Anon/` OR `ProjectFolder/project_info.json` stamping **0x8d + DAT_18006d940** (:53075-53083); `MediaPool/Master/MpFolder.xml` stamping **0x91 + DAT_180067c88** (:53084-53090); the label constants binary-resolved this session from `RAWSearchDLL.dll` .rdata: **DAT_18006d940 (RVA 0x6d940) = UTF-16 `"WFP"`** (bytes `57 00 46 00 50 00 00 00`), **DAT_180067c88 (RVA 0x67c88) = UTF-16 `"DRP"`** (bytes `44 00 52 00 50 00 00 00`). The runtime toggle surface for exactly these typed ids: `og-codebase/config/RawConfig.ini` — `XMIND=1` (:12), `MMAP=1` (:15), `EDDX=1` (:17), `WFP=1` (:18), `DRP=1` (:22), loaded at `RAWSearchDLL_decompiled.c:13133` (CNeedRawMgr::LoadConfig). Trail's own inventory of the same block: `diff-to-fix/SCAN-ENGINE-DELTA.md:177` (the tag strings listed, unowned).
**Core references:** audited `869da779`. `src/core/carve/cr_analysers.cpp` — the N-115 landing's `zip_common` office branch (grep `xmind|MindManager|ProjectFolder|MediaPool|page1_rels` over `src/core/carve/` → **zero hits**, verified this session); `include/cr/carve/cr_signatures.h:315-341` (the nine override rows N-115 landed: OOXML trio + ODF trio + EPUB/IWORK/HWP/CDR — no XMIND/MMAP/EDDX/WFP/DRP row); `src/core/query/cr_filefilter_data.cpp` (`xmind` present as a display category extension — the query layer knows the extension, the carve layer cannot type it). The toggle surface exists (`CarveFlags::disabled_formats`, `include/cr/carve/cr_carve.h:86-96`) with no typed id to gate.
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** N-154 → **N-158** (S42-B merge, 2026-10-29, SA-9 earlier-push-wins: the parallel S42-main window consumed N-154/N-155/N-156/N-157 first — their bookkeeping-batch-2/HFS-zero-blocks/RRWR/OoxmlRels findings keep the IDs; this lane's filings renumber past their N-157). Discovered by the S42 config deep-diff; the parallel window's N-152 fix (the DXF key, `4f8fa661`) landed after this finding's corpus verification — convergent, no overlap.
**Prior audit check:** the tag strings are cited in N-115's corpus field (`diff-to-fix/findings/FIXED/N-115-officetagfromxml-partial-content-types-gate-only.md:6`) and inventoried at `SCAN-ENGINE-DELTA.md:177`, but N-115's Resolution (its own :59 "Resolution notes") lands only the OOXML/ODF/EPUB/IWORK/HWP/CDR set and neither lands nor defers the five branches; no other finding owns them. Discovered by the S42 config-file deep-diff lane's agent-A pass, verified first-hand before filing.

## Summary

Inside the og's ZIP per-entry typing walk — the same `OfficeTagFromXML` surface N-115 ported — five additional branches stamp dedicated format ids and display labels when a zip's inner entries match known document-tool layouts: MindManager maps (`xsd/MindManagerApplication.xsd` → 0x87 MMAP), XMind workbooks (an XML-namespace content match → 0x86 XMIND), Edraw project diagrams (`rels/_rels.xml`/`rels/page1_rels.xml` → 0x8c EDDX), Wondershare Filmora projects (`ProjectFolder/Anon/`/`ProjectFolder/project_info.json` → 0x8d WFP), and DaVinci Resolve media pools (`MediaPool/Master/MpFolder.xml` → 0x91 DRP). The core's N-115 landing covers none of them: such archives carve as generic ZIP with no typed id, no display label, and no RawConfig toggle coupling. The og ships runtime toggles for exactly these five ids in `RawConfig.ini`'s `[Filter]` section — the toggle surface the core already implements (`disabled_formats`) has no ids to gate for them.

## What the reference does

The five branches run in the per-entry loop of the ZIP analyser's typing pass (decompile read first-hand this session, quotes in `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md` §1):

1. **XMIND (0x86):** for each entry, `FUN_180045110(param_1, local_10c0, param_5, entry_name, param_1)` is called; a non-zero return (the XML-namespace content match — the trail's SCAN-ENGINE-DELTA row records the `urn:xmind` namespace check) stamps `*(u32*)param_5 = 0x86` and sets the display label to `L"XMIND"` via the label-inserter `FUN_18000ed00`.
2. **MMAP (0x87):** the entry name is compared to `xsd/MindManagerApplication.xsd` (the length+`memcmp` pair at :53041-53062); on match, id 0x87 + `L"MMAP"`.
3. **EDDX (0x8c):** `strcmp(entry, "rels/_rels.xml") == 0` OR `strcmp(entry, "rels/page1_rels.xml") == 0` → id 0x8c + `L"EDDX"`.
4. **WFP (0x8d):** `strcmp(entry, "ProjectFolder/Anon/") == 0` OR `strcmp(entry, "ProjectFolder/project_info.json") == 0` → id 0x8d + label `DAT_18006d940` = **"WFP"** (binary-resolved this session from the DLL's .rdata at RVA 0x6d940).
5. **DRP (0x91):** `strcmp(entry, "MediaPool/Master/MpFolder.xml") == 0` → id 0x91 + label `DAT_180067c88` = **"DRP"** (binary-resolved at RVA 0x67c88).

The og's `[Filter]` runtime toggles in `config/RawConfig.ini` carry keys for exactly the typed set — `XMIND=1`, `MMAP=1`, `EDDX=1`, `WFP=1`, `DRP=1` (all default-enabled; `STL=0`/`VPJ=0` are the two disabled keys) — consumed by `CNeedRawMgr::LoadConfig` (`:13133`). The typed ids are the join key between the analyser's typing pass and the filter surface.

## What our core does

The N-115 landing (`67de3276`) ported the typing pass's OOXML trio (`word/`/`xl/`/`ppt/` + the ≥5-char prefix fallback), the `[Content_Types].xml`/`mimetype` content tags, and the name-tag set (iWork, EPUB's OPS/chapter1.html, CDR) — nine override rows. The five branches above have no counterpart: `grep -ri 'xmind|mindmanager|projectfolder|mediapool|page1_rels' src/core/carve/ include/cr/carve/` returns zero hits (verified this session). An XMind workbook or a Filmora project archive recovered from a lost partition therefore surfaces as `FMT_ZIP` with a generic name and no typed display; the RawConfig toggle surface (`CarveFlags::disabled_formats`, which the core implements for the 34-key set) holds keys that cannot affect these formats because the typed ids never exist.

## Gap

Falsifiable: plant a stored zip whose entries include `ProjectFolder/project_info.json` (or any of the five branch triggers). The og's engine stamps format id 0x8d (or 0x86/0x87/0x8c/0x91) and the label "WFP" (XMIND/MMAP/EDDX/DRP respectively); the core's carve emits `FMT_ZIP` — no typed id, no label, and the `WFP`/`XMIND`/`MMAP`/`EDDX`/`DRP` `disabled_formats` toggles are no-ops for the fixture.

## Impact

Recovery correctness is unaffected (the archive still carves and recovers byte-identically as ZIP). The impact is typing/display fidelity and the filter contract: five real-world document-tool formats present as generic archives in the results tree, their per-type recovery statistics and type-tree grouping are lost, and the `[Filter]` toggle keys that the og couples to exactly these ids are dead. Users who lost an XMind/Filmora/Edraw project and filter by type cannot find them by type. Downstream: N-115's own recorded follow-up (the metadata half) compounds here — the typed id is also the og's join key for per-format naming/statistics.

## Reproduction / verification

Corpus-derived, verified first-hand this session (the S42 SOP — both the decompile read and the binary label resolution):
1. `sed -n '53025,53110p' re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` — the five branch blocks quoted verbatim in `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md` §1 (the id stamps 0x86/0x87/0x8c/0x8d/0x91, the `strcmp` keys, the two `DAT_` label references).
2. Binary resolution of the two `DAT_` labels: PE section-table parse of `og-codebase/RAWSearchDLL.dll` (image base 0x180000000, .rdata at RVA 0x66000) → RVA 0x6d940 reads `57 00 46 00 50 00 00 00` = UTF-16 "WFP"; RVA 0x67c88 reads `44 00 52 00 50 00 00 00` = UTF-16 "DRP" (the verification script's output is quoted in the evidence file §1).
3. Core absence: `grep -ri 'xmind|mindmanager|projectfolder|mediapool|page1_rels' src/core/carve/ include/cr/carve/` → 0 hits (executed this session).
4. The toggle coupling: `config/RawConfig.ini:12,15,17,18,22` (XMIND/MMAP/EDDX/WFP/DRP keys, default 1).

## Fix plan (for the engineering team)

1. No architectural change — the branches land inside N-115's existing `zip_common` typing walk.
2. Files to modify: `src/core/carve/cr_analysers.cpp` (the office branch of the per-entry walk — add the five entry-name compares + the XMIND content probe), `include/cr/carve/cr_signatures.h` (five new enum members + override rows: `FMT_XMIND`/`FMT_MMAP`/`FMT_EDDX`/`FMT_WFP`/`FMT_DRP` — note the og's id values 0x86/0x87/0x8c/0x8d/0x91 for the enum comment; the core's ids are own-namespace, document the mapping), the `cr_registry.cpp` display-label map (the five labels, "WFP"/"DRP" per the .rdata resolution).
3. Algorithm: the entry-name branches are exact `strcmp` compares on the walked LFH chain entries (N-115's walk already enumerates them); the XMIND branch needs the content probe — `FUN_180045110`'s exact predicate (the `urn:xmind` namespace match per SCAN-ENGINE-DELTA:177) should be decoded first-hand before landing; a conservative first landing can take the entry-name set alone and register the content branch as the follow-up.
4. Tests: one per branch + a negative (a zip with unrelated entries stays FMT_ZIP) + a toggle test (`disabled_formats` with the WFP key suppresses the row).
5. Contract/doc: `docs/FORMATS.md` gains the five formats; the RawConfig key↔id coupling documented in `modules/carve.md`'s RawConfig row.
6. Acceptance: the five fixtures carve with the typed ids + labels; the toggle tests pass; the generic-zip negative passes; `SCAN-ENGINE-DELTA.md:177`'s tag list is fully owned by findings/landings.

## Tests to add

- `Carve.ZipTypingXmindContentBranch` (urn:xmind fixture → FMT_XMIND + label)
- `Carve.ZipTypingMmapEntryName` (xsd/MindManagerApplication.xsd → FMT_MMAP)
- `Carve.ZipTypingEddxEntryNames` (both rels forms → FMT_EDDX)
- `Carve.ZipTypingWfpEntryNames` (both ProjectFolder forms → FMT_WFP)
- `Carve.ZipTypingDrpEntryName` (MediaPool/Master/MpFolder.xml → FMT_DRP)
- `Carve.ZipTypingUnrelatedEntriesStayGeneric` (negative)
- `Carve.RawConfigToggleGatesTypedIds` (WFP disabled → no row)

## Related findings

N-115 (FIXED — the parent surface; its Resolution's uncovered tail is this finding); N-152 (the same walk's DXF key gap — FIXED by the parallel window at `4f8fa661`); M-121-dbx (the same registry-key-residue class — FIXED by the parallel window, final ID M-121 per the renumber chain `1d4c96b6`/`3ac4b99c`/`5a84b63c`); N-140 (the per-extension stats surface that the typed ids feed); S-049 (FIXED — the naming contract's per-format counters key on format ids).

## Open questions

The XMIND content branch's exact predicate (`FUN_180045110`) was not re-decoded first-hand this session — the trail's `urn:xmind` namespace record (SCAN-ENGINE-DELTA:177) is the working decode; the fix lane should re-read `FUN_180045110` before landing that branch (it is a ~30-line read). The coupling between the `[Filter]` keys and the og's `IAnalyserFilter` callback (`RAWSearchDLL:4386-4434`, the `FUN_180006b30` rebuild loop's vtable+8 filter object) is UNDETERMINED — whether disabling the key suppresses the typed row or the whole analyser; the conservative landing (row-level gating) matches the core's existing `disabled_formats` semantics and should be registered as a delta if the og's behavior differs.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-158 landing, session-46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Carve.ZipTypingXmindContentBranch` (all three XMIND forms: content.xml + meta.xml namespace needles + the 14-byte prolog needle, RED pre-fix), `Carve.ZipTypingXmindNeedleMissStaysGeneric` (the content-gate negative), `Carve.ZipTypingDocumentToolEntryNames` (all six entry-name cases across the four name branches + their typed extensions, RED pre-fix), `Carve.ZipTypingLaterEntryOverwritesEarlierStamp` (the no-stop chain-walk semantics, RED pre-fix), `Carve.ZipTypingUnrelatedEntriesStayGeneric` (the negative), `Carve.ZipTypedIdDisableGateSuppressesTyping` (the [Filter]-coupled disable gate). Fail-before: the three typing groups RED (all emitted FMT_ZIP / first-stamp EDDX pre-fix); suite 1209/1209 (linux-release: 1167 pass / 42 env-skips / 0 fail; cr_ui_tests 68/68).
**Resolution notes:** The finding's open questions were both discharged by first-hand decode before landing. (1) The XMIND content predicate: FUN_180045110 re-read in full + disassembly-verified (0x180045110-0x180045666) — the probe builds a 300-byte RAW window at record+30+strlen(name) and types on EITHER the 16-byte ASCII needle `xmlns="urn:xmind` (FUN_18000f820, the std::string find, disasm 0x180045616) OR the 14-byte binary constant at .data RVA 0x93980 (f4 4a c6 40 1a bc a7 76 cf e1 a0 d1 cc 9d — resolved from RAWSearchDLL.dll this session; the deflated-stream prolog fingerprint; the second C-buffer find FUN_180016fc0 len 0xe, disasm 0x180045629-64f, cmovne-stamped); the already-0x86 early-out returns 1 (disasm 0x18004516d `mov eax,1` — the idempotent re-stamp; the Ghidra `void` return type is a tail-elision artifact). The entry set is exactly {content.xml, meta.xml} (the thread-safe static list built in the prologue). (2) The chain-walk stop semantics: the og's record walk (:18411-18450, the do/while dispatching every record through the shared id slot) has NO typing stop — the N-115-era "first decisive tag wins (the og's map stop)" comment was an erratum (FUN_180046120 returns after one record; the walk itself never stops), corrected in this landing: the walk now continues to its 32-entry/256 KiB bound with last-match-wins, pinned by ZipTypingLaterEntryOverwritesEarlierStamp. (3) The [Filter] coupling stays the conservative row-level gate the finding prescribed: the five ids gate through the M-024 disabled_formats surface (registered PARITY-DELTAS D-134 — the og's IAnalyserFilter callback at vtable+8 takes no id argument, so the og's exact suppression mechanism remains undecodable from the corpus; the og ships [Filter] keys for exactly these five ids, RawConfig.ini :12/:15/:17/:18/:22, and the core's row-level gate is the documented equivalent). The disable gate drops the contest candidate (not the override resolution — the claiming office row would otherwise emit a non-office archive as DOCX); the N-115 nine stay exempt via carve_format_always_on (N-149), untouched. Landed: FMT_XMIND/FMT_MMAP/FMT_EDDX/FMT_WFP/FMT_DRP (ids appended, og ids 0x86/0x87/0x8c/0x8d/0x91 documented at the enum), five override-target registry rows (the og's own display labels XMIND/MMAP/EDDX/WFP/DRP), the kTrialOrder slots, the probe helper + the five branches in zip_common's walk (no office-context gate — the og's LFH handler runs them unconditionally, and the office rows' validator reaches every LFH hit through the contest). SCAN-ENGINE-DELTA.md:177's unowned tag list is now fully owned.

### Convergence addendum (S53 minors lane, 2026-10-29)

This lane implemented and RED-proved the same fix independently (the 7-test battery:
five branch tests + the negative + the toggle; 6/7 RED via the stashed-engine rebuild
with the branches disabled at `f0b35591`) before pulling the winning `78b9b7aa`
landing (SA-9). The two landings agree on the branch set, the walk placement, the
appended-enum discipline, and — decisively — the TOGGLE GATE PLACEMENT: both lanes
implemented the emission-time suppression first, both discovered it flawed by
execution, and both moved the gate to the trial-loop winner selection (this lane's
evidence: the raw-sign layer's DOTX sign claimed the unemitted archive interior;
the winner's comment records the DOCX-mislabel path). The winner's XMIND probe (two
needles, disassembly-verified) is the more complete decode and stands. This lane's
kept supplements: the FORMATS.md typing-walk paragraph (docs/FORMATS.md §3) and the
carve-module verdict annotation.
