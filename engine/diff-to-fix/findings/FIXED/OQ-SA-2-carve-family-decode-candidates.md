# OQ-SA-2 — the carve-family decode candidates: CFCPEvt attribution, AAC/M1V/RIFF tier roles, WPG canonical-size stamp, CNeedRawMgr gating

**Status:** RESOLVED (this session's OQ-SA-2 probe lane — all four questions decoded first-hand from the decompile; the resolutions below)
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

## Resolutions (all four decoded first-hand, this session)

1. **CFCPEvt = the og's SQLite analyser — CONFIRMED, and the census's "multimap collision" reading CORRECTED.** The registry initializer (:1962-1971) constructs `BruteForce::CFCPEvtAnalyser` (id **0x73**) and binds it DIRECTLY to key **`0x694c5153`** ("SQLi" — the first 4 bytes of "SQLite format 3"; the full 16-byte magic lives in .rdata at 0x664b0, rabin2 :7734). There is no separate SQLite analyser and no collision — the SQLi key's registration IS CFCPEvtAnalyser (the class is named for its target: FCP event bundles are SQLite databases). **No format gap.** The deep walk (`FUN_18002dfc1`, 660 B) does page-size/page-count math (`max % page_size == 0` via the FileInfo's +0x110/+0x118 fields), caps at 200 MB, reads name columns (UTF-16 → `WideCharToMultiByte`), and RELABELS the id 0x73 → **0x75** on the 17-byte FCP schema marker `"tableactiveactive"` (exact-length-0x11 + memcmp; .rdata 0x6a0c0, rabin2 :7968) — a subtype typing the core lacks → **filed as N-089 (MINOR)**.
2. **Tier roles RESOLVED — no accept/reject boundary divergence.** **RIFF is map-registered PRIMARY** with the two-generation chain (`Brance::RIFFAnalyser` id 0x3b + `FileRIFFAnalyser` id 0x3b fallback at +4, bound to key `0x46464952` "RIFF", :2295-2317) — the core's primary RIFF-family grammar validators (AVI/WAV) match the og's tier. **AAC**: the map/u16 dispatch primary is `BruteForce::CADTSAnalyser` (id 0x2D, u16 0xFFF1 family); `CFileAACAnalyzer` (id **0x4f**) is a LIST-LAYER analyser (constructed only in FUN_180004fb0's `std::list` builder, :4160-4185) — the core's primary `validate_aac_adts` matches the og's PRIMARY tier. **M1V**: `RawFormat_xjq::M1VAnalyser` (id **0x7f**) is LIST-LAYER only — no magic-key map binding (headerless MPEG-1 video has no magic; the og's detection rides the MPEG-ES surface) — the core's MPEGES validator covers the same family. The og's list-layer analysers are NOT fallbacks with different accept semantics — the list is the capability-enumeration surface (gated by CNeedRawMgr, below), and the magic-key dispatch (u16 → u32 → raw_signs.json) is the validation tier the core's grammar layer implements.
3. **WPG "canonical size 0x45" — the artifact's reading CORRECTED: 0x45 is the FORMAT-ID stamp, not a size.** The WPG Analyse (`FUN_180014030`, :15658-15674) is a constant predicate: `*param_2 == 0x677077` → stamp the FileInfo id slot (+0x18) = **0x45** — the same id slot RAR stamps 0x3A and FCPEvt stamps 0x73. The `WPGRawFileInfo` ctor carries the size bounds: **min 0x14 (20 B) at +0x110 and max 100,000,000 (100 MB) at +0x118**. So the og does NOT stamp a canonical size — id stamp + min/max bounds, the same shape as the core's signature row (min_size/max_size in the registry row). No divergence. → CORPUS-ERRATA row (the "size 0x45" misread).
4. **CNeedRawMgr is a per-format NAME gate at list-construction time — NOT a scan-mode gate.** `FUN_180010a40` (:13034-13056) is the once-init process-global singleton (`_Init_thread_header/footer`, `atexit` teardown); `FUN_180010f50` (:13235-13324) is a `std::map<std::string, …>` name lookup (RB-tree walk + memcmp on the name key). Its consumers — 20 call sites, ALL inside the analyser-LIST builder `FUN_180004fb0` (:3334…3895) — follow one pattern: build the format's name string, `query(mgr, name)`, and construct the list-layer analyser (HEIC id 0x7c, M1V 0x7f, CFileAACAnalyzer 0x4f, …) only when the name is found/enabled. The gate is the RawConfig.ini per-format toggle surface applied at LIST MEMBERSHIP (once per process), never per scan mode — quick vs deep never changes the validator set. The core's M-024 runtime per-format disable list is the direct counterpart (probe-time vs construction-time — behaviorally equivalent: a disabled format never validates in either). No divergence.

## Why resolved now (the probe lane's record)

Each answer came from the bounded probe the file specified: the registry-initializer binding pattern (question 1 — the same pattern that settled FH11/CPT/ASF), the vtable→registration-layer reads (question 2), the Analyse body read (question 3), and the singleton + query + call-site walk (question 4). None of the four changes any filed finding's verdict: S-037's validators stand (FH11/CPT untouched by these), N-086's posture stands, and the one NEW fact (the 0x75 FCP subtype relabel) is filed as **N-089 (MINOR)**.

## Related findings

- **A-010, S-037, N-086** (this session's earlier waves) — the resolved outcomes of the same family-artifact wave.
- **N-089** (NEW, this resolution) — the FCP-event-database subtype relabel.



## The Agent 2 scan-engine lane's convergent resolution (landed `1463221` — merged here per the convergent-landing convention)

The Agent 2 lane resolved the same four questions independently in the same
window. Agreements (both lanes, first-hand): **CFCPEvt IS the SQLite analyser
on the SQLi key** (no format gap; the 0x75 FCP subtype → N-089, now FIXED),
and **CNeedRawMgr is per-format config, NEVER scan-mode gating** (the core's
M-024 disable list is the equivalent). The Agent 2 lane's complementary
decodes, adopted into the record:

2a. **Tier roles (their reading)**: the list-layer classes are the legacy
FALLBACK generation — the chain decoded at the FileInfo's +0x18 slot
(`MP3Analyzer -> FileMP3Analyser`); the ADTS frame math lives inside the
modern analyser (:65536+, the 12-bit sync/0xFFF1 walker); the core's
single-primary posture is the REGISTERED D-75, with the og-side boundary
now decoded: modern reject -> legacy try -> both reject -> discard. (This
lane's reading — the list is the capability-enumeration surface and the
magic-key dispatch is the validation tier — and theirs describe the same
two-layer architecture from its two ends; D-75 stands either way.)

3a. **WPG (their reading, completing this lane's id-stamp decode)**: the
`WPGRawFileInfo` factory stamps a CANONICAL MAXIMUM of 100,000,000 bytes
(slot +0x23) with the extent header-derived and sanity-bounded (the x5
factor, FUN_180014160 :15712-15748). **The core carries NO WPG format at
all** (zero rows — verified by both lanes) — the absence is owned by the
S-007 remainder census (the 64 not-implemented classes), now with the
og-side facts measured for the format-implementation lane (key "wpg"
0x677077, type id 0x45, min 20 B, max 100 MB, the x5 extent factor).

4a. **CNeedRawMgr's population (their decode)**: the manager's map is
populated from `config/RawConfig.ini`'s [Filter] section
(`GetPrivateProfileSectionA("Filter", ...)`, FUN_180010b10 :13120+). (This
lane's decode of the consult sites — the name-string + query + conditional
construction pattern in the analyser-list builder FUN_180004fb0 — and their
detection-time phrasing describe the same uniform pattern; the essential
verdict is shared.)

**Convergence verdict: OQ-SA-2 CLOSES** (both lanes) — no core change from
the probes themselves (D-75/M-024 stand as registered); the WPG absence
feeds the S-007 remainder lane with its measured og facts; N-089 (the one
new observable this wave produced) is FIXED.
