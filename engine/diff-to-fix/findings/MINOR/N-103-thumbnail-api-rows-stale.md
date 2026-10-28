# MINOR N-103 — Thumbnail API rows stale: R-28 still reads "no dedicated thumbnail op"; the heif-attribution line is half-wrong

**Severity:** MINOR
**Module:** abi / preview (documentation truth)
**Parameters affected:** A42-adjacent (public surface documentation), C68
**Corpus references:** `EUImgRepairFile_imports.txt:6` (`heif_image_handle_get_thumbnail` — imported by EUImgRepairFile only); EURawImg imports the full heif.dll decode set (import tables grep-verified this session)
**Core references:** audited `ff879f9` — `docs/REFERENCE-API-GAP.md:222,376` (R-28 "no dedicated thumbnail op / PARTIAL" — the engine landed `05e6786`), `docs/PLAN-API-EXPANSION.md:81` (same stale claim), `docs/PARITY-OG-CORE-INVENTORY.md` §E(3-D) line ~90 ("EURawImg + EUImgRepairFile use heif_image_handle_get_thumbnail" — only EUImgRepairFile imports that symbol), `modules/preview.md:153` (the open ABI handoff — still true: cr_ui.h has zero thumbnail entry points)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The thumbnail engine exists (05e6786) but three API documents still
say it does not; and the inventory's HEIC evidence line attributes the
embedded-thumbnail symbol to two binaries when only one imports it.

## Gap / Impact / Verification

Falsifiable: grep cr_ui.h for thumbnail — zero (the handoff is real);
grep the three docs — the stale claims are there verbatim. Trail
truth; no user impact.

## Fix plan

1. Update R-28 (REFERENCE-API-GAP:222/376) and PLAN-API-EXPANSION:81
   to "engine landed (05e6786); cr_ui ABI exposure = open handoff
   (Agent-1)" — the ABI row becomes the tracked item.
2. Correct the inventory's 3-D line (EUImgRepairFile imports
   heif_image_handle_get_thumbnail; EURawImg imports the decode set).
3. When the ABI lands: a `cr_ui_thumbnail` export following the
   two-call/caps contract, C-gate smoke, ABI-CONTRACT row.

## Tests to add

None now; `CrUi.ThumbnailSmoke` when the ABI lands.

## Related findings

S-043/M-080/S-045/M-083/M-085 (the engine's parameter verdicts),
I-028.

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-103 doc-truth batch — the session-38 minor-first lane)
**Verified by:** the corpus import tables re-read first-hand before the edit: `heif_image_handle_get_thumbnail` appears in ghidra-repair/EUImgRepairFile_imports.txt:6 ONLY; ghidra-repair/EURawImg_imports.txt:1-8 carries the decode set (context_read_from_memory_without_copy / get_plane_readonly / scale_image / ...) with NO get_thumbnail — the finding's attribution confirmed verbatim.
**Resolution notes:** Re-verified at current main: REFERENCE-API-GAP:222/:376 still read "no dedicated thumbnail op / PARTIAL" against the landed engine (05e6786 + this session's N-092/M-080-era updates and the parallel S-043 orientation landing); PLAN-API-EXPANSION:81 still framed R-28 as an un-landed protocol question; PARITY-OG-CORE-INVENTORY's heif.dll row attributed the embedded-thumbnail symbol to both binaries. All fixed: the two R-28 rows now carry the engine-landed state with the cr_ui ABI exposure named as the tracked M-102 handoff (async stays host-side by design); the plan row gained the engine-landed note distinguishing it from the protocol question; the inventory row now attributes the symbol to EUImgRepairFile alone with EURawImg's decode-set imports cited (the N-103 correction). `CrUi.ThumbnailSmoke` remains the test for the M-102 ABI landing, as the finding's own plan states.
