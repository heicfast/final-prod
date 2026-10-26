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

---

## Resolution (session DFX31 — both questions dispositioned; S-018 re-filed and FIXED)

**Status:** RESOLVED (evidence `evidence/decompiled-quotes/s31-oq-s3-jpeg-fallback-decode.md`, all quotes first-hand, corpus `03ad46c`)

1. **The legacy fallback chain — YES, it accepts and sizes no-EOI files.**
   All three sub-analysers (JFIF FUN_18004bbd0 / Exif FUN_18004b3b0 /
   generic FUN_18004c1e0 behind the dispatcher FUN_18004b170) run the same
   shape: a SOS-gated header walk (SOF0 precision-8 gate, SOF2
   dims-fill-if-empty, buffer-end without SOS = reject) then the shared
   post-SOS walker FUN_18004c480 (:57942-57991), which scans for FF D9 up
   to h*w*3 (from the candidate start) or 20,000,000 (the 200,000,000
   global cap / 10) and **accepts at the limit / device end with no EOI**
   (:57958-59 `if (limit <= off || get() == NULL) goto ACCEPT` → the 0x23
   stamp + size = the offset reached). The sliding-window getter
   FUN_180042930 supplies data on demand (device-bounded).
   → **S-018's salvage claim REINSTATED** through the fallback with these
   citations; the finding is re-filed CONFIRMED and FIXED this session
   (validate_jpeg now mirrors the salvage; see the finding's Re-file
   Resolution).
2. **The streamed continuation model — residual dissolved.** The
   continuation machinery (FUN_18000d670 → slot-4) is the MODERN tier's
   mechanism; with the fallback decoded, no-EOI candidates emit in-window
   through the legacy tier at the estimate, and the dispatch is
   first-accept-wins (:9869-9916) — the og never needs the continuation to
   surface a truncated JPEG. The core's single-pass walk finds the EOI
   when it exists (900-tier) and only falls to the salvage estimate when
   no EOI exists within the probe limit — a superset of the og's
   first-accept preference for complete files, exact parity for truncated
   ones. No further decode is required for parity; the continuation's
   remaining nuance (>window EOI vs in-window fallback estimate size
   preference) is covered by that superset relationship.

**Verified by:** the three fail-before/pass-after tests in the S-018
re-file Resolution (2 RED pre-fix executed on the stash-isolated tree; 3
GREEN post-fix; suite 755/755).

**Resolution notes:** Zero open residue: the OQ's two questions are
answered with quoted sites, S-018 carries the re-filed Resolution with the
correct citations, and the code change (the salvage in validate_jpeg +
the deliberate PartialRisk grading delta registered in PARITY-DELTAS)
closes the original SEVERE's user-visible gap — the canonical damaged-disk
photo (destroyed tail) now recovers in the core exactly as it does in the
og, sized by the og's own estimate arithmetic.
