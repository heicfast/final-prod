# OQ-SA-2 — the carve-family decode candidates: CFCPEvt attribution, AAC/M1V/RIFF tier roles, WPG canonical-size stamp, CNeedRawMgr gating

**Status:** UNVERIFIED (moved from the session's wave-1 class-by-class comparison — four residual decode questions the family artifacts' roster-level evidence cannot settle)
**Module / Surface:** carve / raw-signature engine (modules/carve.md)
**Parameters affected:** A28 (validator semantics), C19 (keys), B1-adjacent (attribution)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SearchDLL/rawsearchdll--family-bruteforce.md` (CFCPEvt in the roster + family description "FCPEvt"); `rawsearchdll--analyser-registry-census.md` (:105 CFCPEvt in @BruteForce; the SQLite `SQLi` key binding per the s3 registration decode); `rawsearchdll--family-xjq.md` / `--family-special.md` (AAC/M1V/RIFF list-layer roles, WPG); `ghidra-searchdll/RAWSearchDLL_decompiled.c` (the vtables to resolve)
**Core references:** audited commit `dd82db9` — the relevant validators in `src/core/carve/cr_analysers.cpp` (AAC-family, RIFF/WAV-family, the WPG-relevant signature rows)
**Prior audit check:** not in prior scope (the family artifacts were never cited before this session)

## The precise questions

1. **CFCPEvt attribution**: is `BruteForce::CFCPEvtAnalyser` a Final-Cut-Pro-Events validator (the family description's "FCPEvt") or is the class actually bound to the SQLite `SQLi` key (the s3 registration decode's claim)? Both outcomes are individually covered by the core (SQLite carving exists; FCP Events does not) — the attribution decides whether a format gap exists or not.
2. **AAC/M1V/RIFF list-layer tier roles**: the census places `CFileAAC`/`CM1V`/the RIFF cluster in the list-layer ("second-generation") registry — do they act as primary validators for their formats or only as fallbacks in chains behind other analysers? The core implements them as primary grammar validators; if the og's are fallback-only with different accept semantics, the accept/reject boundary differs.
3. **WPG canonical-size stamp**: does the og's WPG analyser stamp a canonical/maximum size where the core's signature row infers from the header? (The family-special artifact notes a size-stamp pattern without decoding it.)
4. **CNeedRawMgr gating**: the census names CNeedRawMgr as an infrastructure class — does it gate which analysers run in which scan mode (quick vs deep), which would make some family validators deep-scan-only in the og while the core runs all validators in the carve pass unconditionally?

## The probe that resolves each

1. CFCPEvt: resolve `BruteForce::CFCPEvtAnalyser::vftable` in the decompile's registry initializer (the same pattern that settled FH11/CPT/ASF this session — the key bound immediately before the vtable assignment) — the key decides the attribution.
2. Tier roles: resolve each class's vtable → Analyse method and read the call sites' registration layer (the dual-registry decode artifact documents the two-layer structure and the function that populates each).
3. WPG: resolve the WPG analyser's Analyse and read the size computation (canonical constant vs header-derived).
4. CNeedRawMgr: locate its two call sites (the census names the class) and read the gating condition (scan-mode flag? engine config?).

## Why not resolved this session

Each requires a vtable→Analyse resolution chain in the 80k-line decompile; the wave's class-by-class pass prioritized the 131 roster entries' presence/absence (which produced A-010/S-037/N-086) — these four semantics questions are the residue, each with a bounded probe specified above. None of them blocks the filed findings.

## Related findings

- **A-010, S-037, N-086** (this session) — the resolved outcomes of the same family-artifact wave.
