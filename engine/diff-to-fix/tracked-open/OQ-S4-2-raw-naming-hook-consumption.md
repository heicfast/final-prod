# OQ-S4-2 — The RAW per-file naming/filter hook's engine-side consumption sites are undecoded

**Status:** OPEN (session 4) · **Module:** carve/raw · **Parameters:** P16 (filter model — server-side filter push), P25 (engine algorithm steps — the hook's role in emission)
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
