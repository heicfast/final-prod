# OQ-S4-2 — The RAW per-file naming/filter hook's engine-side consumption sites are undecoded

> **RESOLVED (session 5, 2026-10-26):** full decode at
> `diff-to-fix/evidence/decompiled-quotes/s5-oq-s4-2-raw-naming-hook.md`.
> Answer: **display-name mapping only, single site, single key** — the
> naming hook (stored at CRAWSearchController+0x30 by vtable slot +0x48
> setter FUN_18000b8d0, RAW:8695-8705) is invoked at exactly ONE site
> (RAW:9090, inside vtable-slot-13 method FUN_18000beb0) with the literal
> key **"filter_generalraw"** + empty string; the returned string NAMES the
> general-RAW filter/category node (tree node type 6) in the RAW result
> tree. No per-file rename, no emission gating, no server-side filter
> push — M-029 §Gap (b)'s assumption is corrected. The DRWUI-side
> implementation maps the key via `<appdir>/firebase.json` (FUN_140161460)
> with a callback fallback (FUN_140160ab0). M-029's fix-plan step 1 now has
> the exact reserved key and semantics.

**Status:** RESOLVED (session 5; was OPEN session 4) · **Module:** carve/raw · **Parameters:** P16 (filter model — server-side filter push), P25 (engine algorithm steps — the hook's role in emission)
**Audited commit:** `63a9167` · corpus `03ad46c`

## The precise question

DRWUI registers a `std::function<string(string,string)>` hook on the RAW searcher
(vtable +0x48, `DRWUI_decompiled.c:917292-917296`) before deep scan — the og's
server-side per-file naming/filter mechanism (M-029 §Gap (b)). The REGISTRATION is
verified; the engine-side CONSUMPTION sites (where RAWSearchDLL invokes the hook,
what it does with the returned name — rename only, or rename+filter gating) were not
decoded this session.

## Corpus artifacts to examine next

1. `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` — the stored
   `std::function` call sites (grep the object's +0x48-slot reads; the emission path
   around the per-file emit FUNs).
2. `re-workspace/artifacts/SearchDLL/rawsearchdll--family-*.md` — the per-family emit
   paths for candidate call-site line ranges.

## Acceptance

RESOLVED when the hook's call site(s) are quoted with semantics (rename vs gate);
fold into M-029's fix plan step 1 (server-side filter push decision).
