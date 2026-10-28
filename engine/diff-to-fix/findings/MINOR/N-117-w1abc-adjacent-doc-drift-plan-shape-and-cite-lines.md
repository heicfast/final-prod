# MINOR N-117 — W1*-adjacent documentation drift: PLAN-API-EXPANSION still describes the un-landed W1b shape (cr_ui_session_load_cancel) and ABI-CONTRACT §7f/§7g cite stale cr_ui.h line ranges after the header grew

**Severity:** MINOR
**Module / Surface:** docs (the W1a/W1b/W1c landing docs)
**Parameters affected:** 46 (contract documentation), 68-adjacent (doc accuracy)
**Corpus references:** n/a — this is an audit-trail doc-truth finding (the corpus-side contracts are correctly cited in the landing findings; the drift is in core's own plan/contract docs).
**Core references:** audited commit `ff879f9` — `docs/PLAN-API-EXPANSION.md:40` (the plan's W1b row: "`cr_ui_session_load_cancel(h)`" — the landed shape is the cancel_flag argument on cr_ui_session_load_with_events, cr_ui.h:914-916); `docs/ABI-CONTRACT.md:479,519` (§7f/§7g cite cr_ui.h:823-865/:867-886 — the actual decls are at :853-895/:897-916 after the header grew).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** The W1a/b/c landings updated ABI-CONTRACT's content sections but the plan's shape row and the contract's line-range cites drifted.

## Summary

("Two small documentation drifts from the W1a/b/c wave: the plan-of-record still describes the pre-landing W1b export shape, and ABI-CONTRACT's §7f/§7g cite cr_ui.h line ranges that the subsequent header growth invalidated. The landed behavior is correct; the docs misdescribe it.",)

## What the reference does

n/a

## What our core does

As cited — the plan row and the two line-range cites.

## Gap

Falsifiable: read PLAN-API-EXPANSION.md:40 (wrong shape) and ABI-CONTRACT.md:479/519 against the current cr_ui.h decl lines.

## Impact

Doc-truth discipline (the repo's own cite-mechanization precedent — the 8ff17fc mechanical-re-pin pattern exists for exactly this).

## Reproduction / verification

Read the cited lines this session.

## Fix plan (for the engineering team)

1. PLAN-API-EXPANSION W1b row: note the landed shape (load_with_events + cancel_flag). 2. ABI-CONTRACT §7f/§7g: re-pin the cr_ui.h line cites (the mechanical re-pin script pattern).

## Tests to add

None (doc fix).

## Related findings

N-091 (the registration finding); the 8ff17fc cite-re-pin precedent.

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-117 doc-truth batch — the session-38 minor-first lane)
**Verified by:** direct site reads before and after (doc-truth finding; the audit's own plan carries no test): PLAN-API-EXPANSION's W1b row now names the landed shape; ABI-CONTRACT §7f/§7g cites re-measured against the current cr_ui.h block boundaries (865-914 / 916-944).
**Resolution notes:** Re-verified at current main: PLAN-API-EXPANSION:40 still described the un-landed `cr_ui_session_load_cancel(h)` export while the shipped surface is the `cancel_flag` argument on `cr_ui_session_load_with_events` (cr_ui.h:933-944); ABI-CONTRACT:479/:525 still cited cr_ui.h:823-865/867-886 while the actual blocks sit at 865-914 (the W1a list surface + row struct) and 916-944 (the W1b load-events surface). Both fixed: the plan row now records the LANDED shape (and flags the divergence from the sketch — the cancel-flag seam mirrors the save-side machinery rather than a dedicated export), and both contract cites are re-pinned to the measured ranges. The mechanical re-pin pattern (the 8ff17fc precedent) applied at the finding's own prescribed sites.
