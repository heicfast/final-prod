# OQ-S3-JPEG — do the og's FALLBACK / CONTINUATION JPEG paths accept tail-truncated (no-EOI) files?

**Origin:** re-verification of S-018 during the diff-fix session-3 leg (2026-10-26). S-018
was filed INVALID — its premise ("neither generation requires or even checks for FFD9")
was refuted first-hand: the MODERN `BruteForce::JpegAnalyzer` (FUN_180033a90,
RAWSearchDLL_decompiled.c :39489-41110) contains five FF D9 sites (Ghidra renders the
byte as the signed char `-0x27`, which the audit's literal `0xd9` grep missed) and its
size-accumulation path is EOI-gated (`if (uVar21 == 0) goto LAB_1800342b6;` :39760 —
the epilogue). Quotes: `../evidence/decompiled-quotes/s3-jpeg-eoi-encoder-correction.md`.

**The unresolved residual:** whether the og still recovers tail-truncated JPEGs through
paths OTHER than the primary analyser's accept gate:

1. **The legacy fallback chain.** `FileJPGAnalyser`'s dispatcher (FUN_18004b170,
   :57080-57567) has no EOI reference itself, but dispatches to three sub-analysers:
   - `FUN_18004bbd0` — the JFIF branch (after `_strnicmp(local_50, ..., "JFIF") == 0`)
   - `FUN_18004b3b0` — the Exif branch
   - `FUN_18004c1e0` — the generic branch
   Decode each sub-analyser's accept criteria (size accumulation + what gates it).
   Question: does any of the three accept and size a JPEG with SOI+APPn+SOF0+SOS but
   no FF D9 anywhere in the scanned span? If YES, S-018's salvage claim holds through
   the fallback and the finding must be re-filed with the correct citation.
2. **The streamed continuation model.** The primary analyser returns early when no EOI
   is in the CURRENT buffer; the pending-continuation machinery (`FUN_18000d670` →
   analyser vtable slot 4 `ContinueScan`, :10114-10116) feeds more sectors. Question:
   does a no-EOI candidate eventually emit via continuation (e.g. at the
   `ctx+0x128 = 0x7800000` 120 MiB default, :39757), or does it die quietly?
   Probe: follow the pending-list lifetime for a JPEG analyser fed a truncated stream.

**Artifacts to examine (re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c):**
FUN_18004bbd0 / FUN_18004b3b0 / FUN_18004c1e0 (fallback sub-analysers);
FUN_18000d670 + the slot-4 continuation (streamed outcome);
ctx+0x10 (accumulated size) and ctx+0x128 (0x7800000 default) consumers.

**Current core state (unchanged, deliberately):** `validate_jpeg` accepts only at a
forward-walked FF D9 with prior SOF+SOS (cr_analysers.cpp:232-237, :290) — consistent
with the EOI-gated primary analyser as far as the corpus proves. IF the fallback or
continuation path proves to accept no-EOI files, the S-018 fix plan (truncated-tail
salvage at lower confidence) becomes valid AGAIN with corrected citations.

**Next concrete step:** decode FUN_18004bbd0's accept path (the JFIF branch is the
most common real-world case) and post the verdict + quotes to
`evidence/decompiled-quotes/`; re-file or close accordingly.

---

## Session-14 addendum (DFX14 — pushed forward, unchanged)

Not resolvable this session (needs the JPEG analyser's fallback/continuation
path decompile — the corpus artifact set for it is already cited in the
file). The standing question and next artifact are precise; no new evidence.
