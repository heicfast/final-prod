# MAJOR M-108 — text-preview eligibility: the og's TxtViewFiles 119-extension whitelist is absent and the core's content-sniff replacement refuses BOM/UTF-16/Latin-1 text files while admitting printable-head binaries (EXECUTED proof)

**Severity:** MAJOR
**Module / Surface:** preview (cr_preview metadata extraction)
**Parameters affected:** B10-adjacent (filename/extension handling in preview eligibility), P46 (contract documentation — the whitelist is undocumented), A42-adjacent (preview surface shape); P1/P2 NOT affected (no recovery impact)
**Corpus references:** `og-codebase/config/FilterFolder.ini` (`[Filter] TxtViewFiles=.TXT;.CFG;…119 case-mixed extensions…;.msbak;`); `re-workspace/artifacts/config-files/filtering-priority--config-layer.md:40,48` ("the text-preview whitelist: extensions the preview panel will render as text"); `re-workspace/artifacts/config-files/config-data-reference.md:39`; **decompile (first-hand this session):** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:535858` and `:535937` — the og loads `config/FilterFolder.ini` `[Filter]` `TxtViewFiles` once (thread-safe static init at `DAT_142643850`), extracts the candidate's extension (`FUN_140089050`), builds the `;ext;` haystack, substring-searches (`FUN_140310aa0`), and sets the text-viewable flag on membership — **extension gating, never content sniffing**
**Core references:** audited commit `84a1832` — `src/core/preview/cr_preview.cpp:868-875` (the 16-byte printable-ASCII head gate: any byte ≥ 0x7F → `kind="unsupported"`), `:579-607` (`text_sniff`: 0x20-0x7E + \n/\t-as-space, NUL stop), `:605-606` (`kind="text"`, `format="txt"`); `include/cr/preview/cr_preview.h:108-111` (Caps; no extension gate surface); tests pin the heuristic: `tests/unit/test_preview.cpp:409-452,513` (no BOM/UTF-16 case exists)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (push-time reconciliation):** filed as M-103 in this lane's working tree; renumbered **M-108** at the second push-time reconciliation — the parallel lanes had concurrently consumed M-103 (lostdir-parent-record-grouping), M-104 (wholedisk-volume-roots), M-105 (DRWService install lifecycle), M-106 (ONE/3DS size-walks) and M-107 (batch-1 stand-in residue); per the session-9 collision convention the earlier remote numbering wins.
**Prior audit check:** M-018's resolution explicitly scoped TxtViewFiles OUT ("TxtViewFiles/SumaryFileType … are NOT in this finding's scope (presentation niceties)"). M-029's fix plan item 4 listed "TxtViewFiles whitelist for preview eligibility" but the resolution landed only the buckets/raw_category_name. **No active finding owns the preview-eligibility behavior.** API-DELTA row 44 claims it "landed (cr_filterfolder_data.cpp)" — FALSE (see N-128; `cr_filterfolder_data.cpp` carries only the three junk pools).

## Summary

The og decides which files preview as text by extension membership in a 119-entry whitelist (TxtViewFiles), decompile-verified. The core instead sniffs the first 16 bytes for printable ASCII. The two models accept disjoint file sets: an executed probe this session shows the core returns `kind=unsupported` (no text snippet) for UTF-8-BOM, UTF-16LE, and Latin-1 text files — the most common real-world text shapes on Windows — while simultaneously classifying printable-head unknown binaries as `text` with a garbage snippet.

## What the reference does

The og's preview panel consults `config/FilterFolder.ini` `[Filter]` `TxtViewFiles` — a 119-extension, case-mixed whitelist (`.TXT;.CFG;.CONFIG;.CSS;.CSV;.HTML;.INF;.INI;.JS;.LOG;.XML;…` source-code families + logs + configs, down to `.A8S;.ARENA;.FOUNTAIN;.MCMETA;.PNPROJ;.msbak`). The consumer is decompile-verified at DRWUI:535858/:535937: the ini is read once into a static, the file's extension is extracted, and a `;ext;`-haystack substring search sets the text-viewable flag. Content is never sniffed for eligibility — a `.txt` file renders as text regardless of its bytes, and a `.xyz` file never does.

## What our core does

`extract_metadata`'s fall-through branch (`cr_preview.cpp:867-876`) gates on content: if ANY of the first 16 bytes is ≥ 0x7F, the file is `unsupported`; otherwise `text_sniff` builds a 256-byte snippet from printable bytes only. There is no extension surface anywhere in the preview module (0 hits for any TxtViewFiles-listed distinctive extension or whitelist concept — `evidence/decompiled-quotes/s10-config-layer-absence-greps.md`). The executed probe (`evidence/executions/s10-preview-text-eligibility.md`):

```
1 ASCII text                 -> kind=text        snippet present
2 UTF-8 BOM text             -> kind=unsupported (og: text)
3 UTF-16LE BOM text          -> kind=unsupported (og: text)
4 Latin-1 text (0xE9)        -> kind=unsupported (og: text)
5 printable-head binary      -> kind=text + garbage snippet (og: not text)
```

## Gap

Falsifiable: `extract_metadata` on a UTF-8-BOM `.txt` returns `kind="unsupported"`; the og's whitelist membership for `.txt` is true, so the og renders it as text. Symmetrically, a `.xyz` binary with 16 printable head bytes returns `kind="text"` in the core; the og's whitelist membership for `.xyz` is false.

## Impact

Preview/ABI consumers (the `cr_ui_preview_metadata` surface; any host building a preview pane). The false-negative class hits the most common Windows text shapes: Notepad-historic UTF-8 BOM files, UTF-16 files, and any text whose first 16 bytes contain a non-ASCII character (accents, CJK, emoji). The false-positive class renders garbage snippets for unknown binaries. No recovery-correctness impact (preview only), hence MAJOR not SEVERE.

## Reproduction / verification

**Executed** (strongest class): the probe source, build line, and verbatim output are at `evidence/executions/s10-preview-text-eligibility.md` (compiled against `84a1832`'s `cr_preview.cpp` + deps, in-memory device harness). **Decompile-direct** for the reference: DRWUI:535858/:535937 read verbatim this session. Cross-verified both directions.

## Fix plan (for the engineering team)

1. Decide the model: land the whitelist as data (the `gen_filterfolder.py` precedent — a `cr_preview_text_exts` pool generated from the corpus FilterFolder.ini, byte-exact, 119 entries) and gate text eligibility on extension membership, keeping the content sniff as the fallback for extensionless files (the og's behavior for those is undecoded — register in PARITY-DELTAS).
2. Fix the false-negative class regardless of the model decision: the head gate must accept UTF-8 BOM (`EF BB BF`) and multi-byte UTF-8, and `text_sniff` should decode UTF-16LE with BOM to text rather than refusing (the og renders the bytes its viewer handles; at minimum, BOM-prefixed `.txt` must not be `unsupported`).
3. Fix the false-positive class: if the whitelist lands, the fall-through for non-whitelisted extensions becomes `unsupported` (matching the og).
4. Files: `src/core/preview/cr_preview.cpp`, a new generated data file beside `cr_filterfolder_data.cpp`, `include/cr/preview/cr_preview.h` (document the eligibility rule).
5. Tests: `PreviewMeta.TextEligibilityWhitelist` (BOM/UTF-16/Latin-1 `.txt` → `kind=text`), `PreviewMeta.UnknownBinaryNotText` (printable-head `.xyz` → `unsupported`), the generated-pool byte-exactness test (the `gen_filterfolder` pattern).
6. Contract: ABI-CONTRACT's preview section documents the eligibility rule; N-128's API-DELTA row-44 correction lands with this.

## Tests to add

- `PreviewMeta.Utf8BomTextIsText` (RED today: returns `unsupported`)
- `PreviewMeta.Utf16BomTextIsText` (RED today)
- `PreviewMeta.Latin1TextIsText` (RED today)
- `PreviewMeta.PrintableHeadBinaryNotText` (RED today: returns `text`)
- `PreviewTextWhitelist.ByteExactAgainstCorpus` (guards the generated pool)

## Related findings

**N-128** (the API-DELTA false "landed" claims for this surface) · **M-018/N-024** (the junk-filter half of FilterFolder.ini — landed, byte-exact) · **M-029** (fix-plan item 4 named this surface, never landed) · **S-045** (thumbnail format coverage — a different module's extension-family question)

## Open questions

The og's eligibility for extensionless files (no extension to test) is not decoded in the corpus — the whitelist consumer's empty-extension path needs a decompile read if the fallback matters. Registered in the fix plan step 1.
