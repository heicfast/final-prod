# MINOR N-051 — MKV/WebM validator lacks the og's DocType discrimination and element-bounds gate

**Severity:** MINOR
**Module:** carve
**Parameters affected:** A28 (carving validator logic), B3 (false positives — no DocType gate), C19 (magic/constant table — the og's webm/matroska constants absent)
**Corpus references:** `re-workspace/artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md:43-66` (FileMKVAnalyser FUN_180016c70: full EBML vint decoder; "reads the DocType element (webm vs matroska) and validates element sizes"; constants **0x6D626577 ("webm")** and **0x67805318** present; element bounds `n + value + 4 > 0x200: reject`)
**Core references:** `src/core/carve/cr_analysers.cpp:846-885` (`validate_mkv` — EBML walk without any DocType check; grep for doctype/webm/0x6D62: empty) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's MKV analyser decodes the EBML DocType element and distinguishes webm
(0x6D626577) from matroska, with an element-bounds gate (header id+size+payload ≤ 0x200).
Our `validate_mkv` walks EBML structure but never reads DocType — every EBML container that
passes the structural walk is accepted and labeled MKV regardless of DocType, and the
bounds gate differs.

## What the reference does

Per the granular-batch1 decode (quoted in Corpus references): vint decode of ID and size,
then DocType discrimination between the webm and matroska constants, then the element
bounds check `n + value + 4 ≤ 0x200` before declaring a hit. The artifact notes this is the
only analyser in the set with a true variable-length-integer grammar.

## What our core does

`validate_mkv` (`cr_analysers.cpp:846-885`) parses the EBML header elements for structural
validity but has no DocType read, no webm/matroska constant comparison, and its own bounds
policy.

## Gap

Falsifiable: an EBML file whose DocType is neither webm nor matroska (e.g. an exotic EBML
use) is rejected by the og's analyser and accepted by the core; webm files are labeled
indistinguishably from matroska in the core.

## Impact

Small false-positive surface (non-webm/matroska EBML accepted) and a labeling distinction
(webm vs matroska) the og's type system carries and the core's does not. Low volume in real
recovery corpora.

## Reproduction / verification

Corpus artifact read first-hand (constants + bounds quoted); core grep verified empty this
session.

## Fix plan

1. In `validate_mkv`, after the EBML header walk: read the DocType element; accept webm
   (0x6D626577) and matroska constants per the og; reject others; add the og's
   `n + value + 4 ≤ 0x200` bounds gate on header elements.
2. Optionally surface the DocType in the carve label (og distinguishes the two).

## Tests to add

- `Carve.MkvDocTypeWebmAccepted`
- `Carve.MkvDocTypeExoticRejected`
- `Carve.MkvElementBoundsGate`

## Related findings

- S-007 (format coverage — MKV exists both sides; this is the validator-detail delta)
- N-052 (HEIC label family — the same granular-batch1 decode)

## Open questions

The exact role of 0x67805318 (second constant) is decode-level in the artifact — the fix
leg should treat it as the matroska-family constant the og compares; if a re-decode
clarifies, update the constant set.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's carve-magic-quartet batch
**Verified by:** `Carve.MkvDocTypeWebmAccepted` (webm + matroska carve), `Carve.MkvDocTypeExoticRejected` (DocType "foob" - an EBML use the og rejects and the pre-fix core accepted), `Carve.MkvElementBoundsGate` (size vint 520: 2+520+4 = 526 > 0x200 -> rejected before any payload read). Fail-before: the tests do not exist on the pre-fix tree; the pre-fix validator accepted any structurally-valid EBML regardless of DocType. Suite 525/525.
**Resolution notes:** implemented the og's FileMKVAnalyser semantics (analyser-granular-batch1 :43-66, first-hand re-read): the header element's total id_len+size_len+payload+4 must stay <= 0x200 (the element-bounds gate) - the payload is read once into a bounded <=0x200 buffer and walked locally with a new ebml_vint_local (same vint algorithm as ebml_vint, buffer-based); the DocType element (full EBML ID 0x4282 - decoded marker-stripped as 0x0282) decides: webm (the og's 0x6D626577 constant) or matroska accept, everything else rejects, no DocType rejects. The walk then continues as before. First application note: the second corpus constant 0x67805318 from the artifact is the analyser's TYPE-ID space, not a DocType value - the DocType discrimination is the string pair, per the artifact's own "reads the DocType element (webm vs matroska)".
