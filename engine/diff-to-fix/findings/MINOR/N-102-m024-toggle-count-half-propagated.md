# MINOR N-102 — M-024's 37→34 RawConfig toggle correction is half-propagated (the finding's own header still says 37)

**Severity:** MINOR
**Module:** carve / config (audit-trail truth)
**Parameters affected:** C68 (documentation accuracy), D16 context (the RawConfig toggle census)
**Corpus references:** `re-workspace/artifacts/CORRECTIONS.md` #5 — "RawConfig.ini ships **34** toggles" (the mechanical recount; the original census claim was 37)
**Core references:** audited `ff879f9` — `findings/FIXED/M-024-rawconfig-toggles-absent.md:6,15` (both still read "37 toggles") vs `:130` (the correction note, "actual 34"); `diff-to-fix/SURFACE-INVENTORY.md:48` already flags "propagation pending this session"
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The corpus recount (34 toggles, not 37) landed only in M-024's
correction addendum; the finding's header and body still carry the old
count, and the inventory's own "pending" note is now two sessions
stale.

## Gap / Impact / Verification

Falsifiable: M-024:6 and :15 say 37; CORRECTIONS #5 says 34. Trail
truth only — the shipped RawConfig parsing is count-independent.

## Fix plan

Replace the two "37 toggles" mentions with 34 (keeping the historical
note that the census originally claimed 37), and flip the inventory's
"propagation pending" note to done.

## Tests to add

None.

## Related findings

N-101 (the same truth-pass wave), M-024 (the subject).

## Open questions

None.

---

## Session-10 addendum (2026-10-28, re-verification sweep at HEAD `84a1832`) — a THIRD uncorrected instance + this finding's own citation defect

Re-verified PARTIALLY-FIXED: the src-side halves the finding named were
corrected by `a972d29` (cr_rawsigns.h:7-8 now "one row per FormatId - the
cr_signatures.h registry, 108 rows"; cr_signatures.h:256 now "34 named
toggles in the shipped file"). But the sweep found (a) a THIRD instance
`a972d29` missed: `include/cr/carve/cr_carve.h:88` still reads "37 named
toggles, STL/VPJ ship default-off"; and (b) the finding-file half stands
verbatim: `findings/FIXED/M-024-no-runtime-format-filter-surface-vs-og-rawconfig-ini.md:6,15`
both still read "37 toggles"/"37 named toggles" (only the :127-139
addendum carries 34). Citation-defect note for the record: THIS finding's
cited filename "M-024-rawconfig-toggles-absent.md" never existed in git
history — the real file is `M-024-no-runtime-format-filter-surface-vs-og-rawconfig-ini.md`
(the substance is unaffected; the cite is corrected here). Fix plan update:
add cr_carve.h:88 to the sweep list (now three sites: FIXED/M-024:6,15;
cr_carve.h:88).
