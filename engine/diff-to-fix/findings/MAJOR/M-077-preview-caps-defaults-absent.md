# MAJOR M-077 — Preview caps: the og's shipped 100 MB / 15 s product caps have no core defaults (no eligibility gate, no time bound)

**Severity:** MAJOR
**Module:** preview / ABI
**Parameters affected:** X48/X49 (latency/memory bounds the caps enforce), P42 (the caps seam), Q68 (the shipped-defaults statement)
**Corpus references:** `re-workspace/artifacts/UIUX/10-preview-subsystem.md:22-27` (§3 "Limits & gating (free vs Pro — QM-verified copy)"): `:23` "- **Size cap**: 'Files <font color=red>larger than 100MB</font> are not available for preview.'"; `:24` "- **Time cap (video)**: '15s preview only'"; `:26` version gating; `:33-34` the `IInWindowStream` range-read streaming context at the 100 MB cap edge
**Core references:** audited `35adcb9`. The seam: `include/cr/preview/cr_preview.h:108-112` `struct Caps { u32 max_read = 4u<<20; u32 max_work = 1u<<22; u32 text_snippet = 256; };` — an extraction budget (4 MiB reads/work units), not the og's product caps. `rg -n "100\s*MB|100MB|15\s*s|104857600"` over `src/` → 0 code hits (matches are docs only: PLAN-API-EXPANSION:63, REFERENCE-API-GAP:65/233/382, UI-READINESS-PLAN:16, modules/preview.md:9). ABI: `cr_ui_preview_read` caps 4 MiB per call (cr_ui.h:1170-1173), no total-eligibility gate, no time cap. Board-phrasing correction verified this session: "zero-init = uncapped" is imprecise — zero-init is the 4 MiB/4M-unit extraction budget; the og's 100 MiB eligibility gate and 15 s time bound exist nowhere in the preview path.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og ships preview product caps as policy: files over 100 MB are not previewable ("larger than 100MB are not available for preview") and video previews are 15 s only. The core's preview seam has only extraction budgets (4 MiB read/work); no 100 MB eligibility gate and no time bound exist anywhere in the path. Hosts get unbounded preview attempts where the og would refuse.

## What the reference does

The og's preview subsystem enforces two product caps before preview (UIUX/10 §3, QM-verified copy): the 100 MB file-size eligibility gate (HTML-styled refusal for larger files) and the 15 s video-preview time cap. These are product policy, not engine limits — the audit's own register (API-SURFACE-AUDIT.md:98) notes they are "UI-policy (host-side), NOT IN CORPUS as engine limits"; the plan-of-record's decision is that the core ships them as config-overridable defaults.

## What our core does

`preview::Caps` (cr_preview.h:108-112) carries read/work/text budgets only. The ABI's `cr_ui_preview_read` enforces a 4 MiB per-call payload cap with no cumulative eligibility gate and no decode-time bound. The grep over `src/` finds no 100 MiB constant and no time guard. Effect: a 4 GB file's preview metadata extraction is attempted (and may succeed up to budget) where the og refuses at the gate.

## Gap

The preview surface enforces neither the og's 100 MB file-size eligibility gate nor a 15 s decode-time cap: `preview::Caps` contains only read/work/text budgets, and no constant equal to 100 MiB or any time bound exists in the preview path. Falsifiable: a 200 MB file's preview metadata request proceeds (to budget) in core; the og refuses with the styled cap message.

## Impact

Behavioral gap at the product-policy layer: hosts cannot rely on the core to enforce the og's preview posture (they can implement it themselves from `size_bytes`, but the plan-of-record's decision is core-side defaults). Memory/time exposure on pathological inputs (a 256 MiB decode guard exists separately — `kMaxDecode`, cr_preview.h:59-61 — but not the og's 100 MB eligibility semantics).

## Reproduction / verification

- Corpus-derived (UIUX/10:23-24 quoted verbatim).
- Core first-hand: Caps struct read; the grep census (0 code hits); ABI preview section read.

## Fix plan (for the engineering team)

1. `include/cr/preview/cr_preview.h`: extend `Caps` with the shipped product defaults — `max_file_bytes = 100 MiB` (eligibility gate) and `max_decode_ms = 15 s` (time guard), zero-init = defaults ON (the og's caps are product policy, not user opt-in), config-overridable by the host.
2. `src/core/preview/cr_preview.cpp`: enforce — oversized request → truncated + flagged in `Metadata::truncated` (never silent); decode-time guard via a std::chrono check in the extract loop.
3. Surface: `cr_ui_preview_metadata`'s `truncated` flag (cr_ui.h:1189) carries the gate result; document the gate in the cr_ui.h preview section + ABI-CONTRACT.
4. Tests: >100 MiB logical size → truncated+flagged; slow-decode guard pinned through the seam (inject a stalling reader); defaults-on zero-init; override honored.

## Tests to add

- `Preview.SizeGate100MiB` (200 MB fixture → truncated + flag).
- `Preview.TimeGuard15s` (stalling-reader injection → bounded).
- `Preview.CapsOverride` (host override honored).
- `Preview.DefaultsOnByDefault` (zero-init pin).

## Related findings

- I-015/M-028/M-066/N-088 (FIXED — the preview architecture register and enumeration; the caps were explicitly the deferred tail).
- M-080 (the thumbnail budget — a different seam, same product-cap class).

## Open questions

Whether the 15 s cap applies to the metadata-extraction path or only decode-heavy formats (the corpus ties it to video preview) — the fix plan applies it to decode work generally; pin the scope decision in the contract text when landing.
