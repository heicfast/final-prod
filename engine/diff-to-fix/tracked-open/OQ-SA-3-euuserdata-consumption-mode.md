# OQ-SA-3 — the og's EUUserData consumption mode: capability-creating signs vs. secondary typing (the M-065 severity calibrator)

**Status:** UNVERIFIED (moved from wave 5 — the corpus carries both phrasings and the difference decides whether the missing default set is a recall gap or a labeling gap)
**Module / Surface:** carve / user-extensible signature DB (modules/carve.md)
**Parameters affected:** B1-adjacent (detection coverage weight of user signs)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:61` — "secondary typing" phrasing for the JSON-signs layer; the same artifact family elsewhere describes EUUserData as consulted alongside raw_signs.json at scan setup; `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:72213-72221` — `GetRawUserInfoMap` = `FUN_180001350()+0x90` (the user-info map's fetch point, already partially decoded by M-024's trail)
**Core references:** audited commit `dd82db9` — `src/core/carve/cr_usersigns.cpp` (install_set installs into the same engine registry the grammar validators use — capability-creating by construction in the core)
**Prior audit check:** not in prior scope (the grammar artifact was never cited; M-048 landed the surface without the consumption-mode decode)

## The precise question

When the og's RAWSearch engine consults EUUserData's signs, does it (a) create/extend detection capability for the 79 stock-absent extensions (the signs can produce carve hits raw_signs.json and the C++ analysers cannot), or (b) only refine typing/labels for hits other layers produce? The deep-dive's ":61 secondary typing" phrasing suggests (b); the consult-alongside phrasing suggests (a); the two imply different severities for M-065 (recall gap vs. labeling gap) and different fix priorities.

## The probe that resolves it

Decode the consumers of the user-info map in `RAWSearchDLL_decompiled.c`: `GetRawUserInfoMap` (`FUN_180001350()+0x90`) returns the map; find its call sites (the searcher init M-024's trail decoded at :9180-9182 is one) and read whether the map's signs enter the SIGNATURE SCAN loop (the raw_signs scanning pass — capability) or only the result-typing path (post-hit classification — labeling). One decompile read, two call sites, resolved.

## Why it was not resolved this session

The wave's budget went to the grammar decode + the og-DB measurement (which produced S-039/M-065's filed facts); the consumption-mode decode is a bounded follow-up that does not block either finding (S-039's caps are wrong under either mode; M-065's gap exists under either mode — only its severity moves).

## Related findings

- **S-039, M-065** (this session) — the findings this calibrates.
