# MAJOR M-092 — the HTML carve format is absent: the og keys it four ways (u16 <h / <H / <! / UTF-16 BOM FF FE) with ASCII+UTF-16 content probes and a </html> end-tag size walk (1 MiB cap) — core has no HTML analyser

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1 (detection correctness), A2 (recovery correctness — a ubiquitous text format never carved), A28-adjacent, B19-adjacent (four u16 keys incl. the FF FE BOM chain missing from the census)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:14263-14281` (chain RawSpeed::HTMLAnalyser id 0x21 → FileHTMAnalyser id 0x21); `:2849/2858/2867` (the u16-key registrations `<h` 0x683c, `<H` 0x483c, `<!` 0x213c) and **`:2876` (a 4th chain under u16 key 0xfeff — the UTF-16LE BOM — absent from the dual-registry census table, session-8 erratum E59)**; the modern probe `:14307-14380` (accept if `_memicmp(buf,"<HTML",5)==0` OR `_memicmp(buf,"<!DOCTYPE HTML",14)==0` OR (u32==0x3cfeff AND wide "<HTML>"/"!" probes)); the legacy probe `:50817-50837`; the size walk `:50865-50869` (chunked scan for `</html>` 0x74682f3c+0x3e6c6d, len 7, **cap 0x100000 = 1 MiB**) with sector round-up (:50880-50884); FileInfo bounds {min 20, max 0x100000} (:14395-14396).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — no HTML probe, no "</html>" needle, no FF FE chain
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred the text school ("HTML/XML/SVG/…") with key sites not decoded; this session decoded the HTML registration + grammar in full.

## Summary

HTML — the most common text format on any disk — is absent from core's carve reach. The og dispatches it through four u16 keys (the <h / <H / <! prefixes plus the UTF-16LE BOM FF FE chain, which the census never listed) and validates by content probe (case-insensitive <HTML / <!DOCTYPE HTML / the UTF-16 wide forms), sizes by a chunked </html> end-tag scan capped at 1 MiB with sector round-up. Deleted web archives, saved pages, and email-export HTML are unrecoverable by content in core. The finding formalizes the batch-2 remainder.

## What the reference does

Four u16-key registrations (:2849/:2858/:2867/:2876) bind the HTMLAnalyser chain (id 0x21). The modern probe (:14307-14380) accepts: case-insensitive `<HTML` prefix, case-insensitive `<!DOCTYPE HTML` prefix, or the UTF-16LE BOM followed by a wide `<HTML>`/`<!` probe. Size: the family's chunked end-marker scan for `</html>` (7-byte needle), cap 1 MiB, then sector round-up; FileInfo bounds {20 B, 1 MiB} (:14395-14396).

## What our core does

No HTML analyser exists — no u16-key rows for <h/<H/<!/FF FE, no content probes, no end-tag walk. The only XML-adjacent byte in core's carve is DOCX's first-entry gate.

## Gap

Falsifiable: a stream starting `<html>` with a closing `</html>` at offset K (< 1 MiB) is carved by the og (id 0x21, size K+7 rounded to sector); core carves nothing.

## Impact

Recovery correctness: saved/deleted web pages (.html/.htm), email exports, and documentation are silently unrecoverable; the UTF-16 variant (the FF FE chain) covers the Windows Notepad default encoding — a common real-world case.

## Reproduction / verification

Decompile-direct: registration sites, probe bodies, and the size-walk needle/cap read this session and quoted above; the `<html>`/`</html>` strings cross-check in the shipped binary's string tables.

## Fix plan (for the engineering team)

1. Add FMT_HTML ("html", display "HTML document") with four u16-key rows (<h 0x683c, <H 0x483c, <! 0x213c, FF FE 0xfeff). 2. Implement `validate_html`: the case-insensitive ASCII probes + the UTF-16 wide probe; size = chunked `</html>` scan (ASCII and the wide form for the BOM chain), cap 1 MiB, sector round-up; if no end tag within cap, the bounded-extent fallback with the heuristic note (batch-1 convention). 3. Wire into kTrialOrder. 4. Census correction E59 already filed.

## Tests to add

- `Carve.HtmlAsciiLowerUpperDoctypeAccepted` — the three ASCII probe forms carve.
- `Carve.HtmlUtf16BomChainAccepted` — FF FE + wide <html> carves (the 4th chain).
- `Carve.HtmlEndTagExtent` — </html> at K sizes K+7 sector-rounded.
- `Carve.HtmlNoEndTagFallsBackBounded` — heuristic note, 1 MiB cap.
- Hostile: random `<`-prefixed buffers must not fire beyond the probe grammar.

## Related findings

S-007 (batch-2 parent); M-093 (the XML/SVG chain sharing the text-school walk); S-041 (school 1, landed `b92ebef`)/S-047 (the other batch-2 findings).

## Open questions

None material — the grammar is decoded end-to-end.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** `eb1f116`
**Verified by:** `Carve.HtmlAsciiLowerUpperDoctypeAccepted` (the three ASCII probe forms carve; key-exact rows <h/<H/<!; the exact sector-rounded extents), `Carve.HtmlUtf16BomChainAccepted` (the 4th chain — FF FE + wide "<HTML>" carves via FMT_HTML_UTF16 with the "html-endtag-walk+utf16" note), `Carve.HtmlNoEndTagFallsBackBounded` (the 1 MiB bounded fallback + "html-bounded-cap" heuristic note), `Carve.HtmlNonHtmlPrefixesRejected` (a <head> document and a wide "<h" non-form reject — the probe gates). Full suite 1071/1071 green (Linux release). The Consistency.CheckableVocabulary test updated: HTML became checkable when its carve family landed (the OQ-FLT-1 registry-derived vocabulary — the vocabulary follows the registry by design; NEF stays uncheckable).
**Resolution notes:** The corpus was re-verified first-hand before the fix: the chain constructor (:14263-14281, HTMLAnalyser+FileHTMAnalyser id 0x21), the modern probe FUN_180012270 (:14307-14380 — case-insensitive "<HTML" 5B / "<!DOCTYPE HTML" 14B / the BOM chain dword@0==0x003CFEFF with the wide "<HTML>" OR the wide '!' at +4), and the size walk FUN_180042e60 (:50845-50884 — the 7-byte "</html>" needle {0x74682f3c, 0x3e6c6d}, cap 0x100000, the sector round-up reading the storage cell's logical sector). One plan correction against the corpus: **no wide end-tag needle exists anywhere in the DLL** (grep for the wide-form constants returns zero sites) — the fix plan's "(ASCII and the wide form for the BOM chain)" parenthetical was an inference; the landed walk scans the ASCII needle only, so a UTF-16 document without an ASCII "</html>" falls to the bounded cap, exactly the og's behavior. Four registry rows (FMT_HTML/FMT_HTML_H/FMT_HTML_BANG/FMT_HTML_UTF16 — the og's duplicate-key policy, one shared validator), FileInfo bounds {20, 0x100000}, the sector round-up clamped to the slab (node extents stay inside the device), and the no-marker fallback to the cap with the heuristic note per the RTF/slab-end family convention.
