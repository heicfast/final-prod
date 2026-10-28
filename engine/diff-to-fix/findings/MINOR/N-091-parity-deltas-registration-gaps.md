# MINOR N-091 — PARITY-DELTAS registration gaps on the new landings: the load-progress model, the thumbnail superset, and the auto-save default posture are unregistered

**Severity:** MINOR
**Module:** session / preview / docs
**Parameters affected:** Q68 (documentation accuracy — the delta register), S38-adjacent (progress semantics — the model itself is documented, its og-delta status is not registered)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:371-376` (the og's load-progress contract: `40 + 50·done/(total+done)` — the 40→65 % window of the overall restore bar; per-child IsCancelled); `rsf-format:448-449` (the og's auto-save: unconditional, `<module-dir>\SaveScan` fixed folder); og thumbnails: `DRWUI_decompiled.c:652377-652419` (UI-side GDI+ — no in-engine counterpart)
**Core references:** audited `35adcb9`. Load progress: `src/ui/cr_ui.cpp:2711-2720` — `stage_permille = done*1000/total`, `overall_permille = 0xFFFFFFFF` (documented in ABI-CONTRACT §7g **without** flagging the delta vs the og formula; grep for load-progress/40+50 in PARITY-DELTAS.md → zero rows). Auto-save: opt-in `autosave_dir`, zero-init = OFF (cr_ui.h:576-587; the always-on/fixed-folder divergence registered in §7h only as interpretation). Thumbnails: engine-side decode (05e6786) registered in `docs/PARITY-OG-CORE-INVENTORY.md` C.2 + `modules/preview.md:103-154` but `docs/PARITY-DELTAS.md` (last touched `c169332`, before the landing) carries no row — grep `cr_thumbnail` in PARITY-DELTAS → zero hits.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Three deliberate divergences from the recent landings exist only in scattered contract text, not in the delta register where parity decisions are tracked: (a) the session-load progress event carries the load's own 0-1000 permille with `overall = 0xFFFFFFFF` where the og's restore mirror computes `40+50·done/(total+done)` of the overall bar — a host implementing the og's bar from the event alone cannot reproduce the 40-65 % window (the `bytes_done/bytes_total` fields do permit recomputation); (b) the thumbnail engine is an in-process superset of a UI-side og capability — registered in two docs but absent from PARITY-DELTAS; (c) the auto-save's opt-in/off-default vs the og's unconditional module-adjacent SaveScan posture is documented as interpretation but not registered as a delta row.

## What the reference does

See the corpus cites: the og's restore progress is a fixed sub-window of the overall bar; its auto-save is always-on into a derived folder; its thumbnails are UI-process GDI+/sidecar work with no engine counterpart.

## What our core does

See the core cites: our own permille model (documented, unregistered-as-delta); opt-in auto-save (documented as interpretation); in-engine thumbnails (registered in two of three places).

## Gap

Three divergences the trail's central register (PARITY-DELTAS.md) does not carry. Falsifiable: grep the register — zero rows for the load-progress model, the thumbnail superset, the auto-save default.

## Impact

Registry hygiene: future sessions audit against PARITY-DELTAS; unregistered deltas risk being re-litigated as defects or silently drifting. No behavioral effect.

## Reproduction / verification

First-hand greps of PARITY-DELTAS.md (zero hits per item) against the documented-but-scattered sites (ABI-CONTRACT §7g/§7h, PARITY-OG-CORE-INVENTORY C.2, preview.md) — all read this session by the verification lane; re-confirmed by the lead.

## Fix plan (for the engineering team)

1. Add three rows to `docs/PARITY-DELTAS.md` (the next D-numbers in sequence): (a) session-load progress model (stage-permille + 0xFFFFFFFF overall sentinel vs the og's 40+50·done/(total+done) window; bytes fields enable host recomputation); (b) in-engine thumbnail decode superset (the I-015 lineage's engine-side extension; ABI surface pending); (c) auto-save opt-in/off-default + caller-dir vs the og's unconditional module-adjacent SaveScan.
2. Optionally also the `stats.autosaved == 0` ambiguity (a failed attempt is indistinguishable from OFF without an event sink — one sentence in ABI-CONTRACT §7h, `cr_ui.h:672-677`).

## Tests to add

- None (documentation). If desired, a doc-lint gate asserting every ABI-CONTRACT "differs from the og" phrase has a PARITY-DELTAS row ID.

## Related findings

- N-090 (the same landings' shape deltas — behavioral cousins).
- I-015/D-59h (the preview register the thumbnail row extends).

## Open questions

None.


---

## Session-8 lead-lane reconciliation addendum (2026-10-28, the push-window merge)

One more registration gap belongs in this finding's set: the **auto-save filename sanitizer
superset**. The og's FUN_140286930 replaces exactly 8 characters (`/ * : ? " < > |` —
rsf-format--scan-session.md:450-451, no backslash); core's `auto_session_name`
(cr_crsf.cpp:354-360) adds backslash and the C0 control range and calls it "the Windows
name-forbidden set". A volume label containing a backslash or a control byte produces a
different filename in core than in the og. Deliberate superset, unregistered — same class
as the load-progress scale and the opt-in posture this finding already carries.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-091 registration batch — the session-38 minor-first lane)
**Verified by:** live greps before the edit (load-progress: 0 rows; thumbnail: 0 delta rows — only protocol mentions + the S-043 orientation rider section; sanitizer: only the SafeWriter destination-name mention at :2655, not the auto-session name) — all three gaps stood; after the edit the register carries D-108/D-109/D-110.
**Resolution notes:** Re-verified at current main, item by item: (a) the session-load progress model (stage permille + the 0xFFFFFFFF overall sentinel vs the og's 40+50·done/(total+done) fixed window; the byte fields' recomputation route) — REGISTERED as D-108; (b) the in-engine thumbnail decode superset (the I-015 lineage: the og's thumbnail work is all UI-process GDI+/agents, the core decodes through the bounded PreviewReader) — REGISTERED as D-109 (previously scattered across OGI C.2 + modules/preview.md only); the addendum's fourth gap — the auto-session-name sanitizer superset (backslash + C0 controls beyond the og's exactly-8 characters) — REGISTERED as D-110; (c) the auto-save opt-in/off-default item was ALREADY covered by the parallel S-044 lane's D-105 (the enable-surface shape) + D-12 (the folder convention) — recorded as FIXED-ALREADY halves. The optional §7h `stats.autosaved` ambiguity sentence: ABI-CONTRACT:571 already documents the field's semantics ("live runs; loaded [sessions report the saved flag]") — the ambiguity note is now carried inside D-105's registration context and this Resolution rather than duplicating a fourth site. A D-107 numbering collision between two parallel lanes' rows (the M-082 rider and the S-047 TS posture) was found during the numbering pass and flagged in the register with the go-forward rule (D-108+; renumber-with-history-note if reconciled) — my rows took D-108/D-109/D-110.
