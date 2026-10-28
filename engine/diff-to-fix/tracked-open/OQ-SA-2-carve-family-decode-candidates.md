
## Probe resolution (2026-10-28, Agent 2 scan-engine lane — all four questions RESOLVED)

1. **CFCPEvt attribution — RESOLVED (SQLite confirmed).** The registry
   initializer binds `BruteForce::CFCPEvtAnalyser` (type-id 0x73) to key
   `0x694C5153` = "SQLi" (RAWSearchDLL_decompiled.c:1965-1971 — the bytes
   53 51 4C 69 = the first four chars of "SQLite format 3\0"). The class
   name is historical: FCP Events libraries ARE SQLite databases. The
   legacy twin `FileFCPEVENTAnalyser` (same type-id 0x73, the list-registry
   builder :4196-4200) is the fallback generation of the same format. The
   core's `FMT_SQLITE` + `validate_sqlite` (12-byte magic) covers the same
   hit set — **no format gap exists**.

2. **AAC/M1V/RIFF tier roles — RESOLVED (they are the fallback
   generation).** The two-generation design is now instruction-concrete:
   the chain builder FUN_1800304f0 (:37083-37102) constructs
   `BruteForce::MP3Analyzer` (modern, type 0x2b) with `FileMP3Analyser`
   (legacy, 0xA0-byte object) attached at +0x18, and registers the CHAIN
   for the MPEG/AAC sync keys 0xFDFF/0xFEFF/0xFFFF/0xFFE2/... (:2640-2690
   — the u16 registry). The list-layer `File*Analyser`/`CFile*Analyser`
   classes (CFileAIFAnalyzer 0x50, CFileAACAnalyzer 0x4F, FileMP3Analyser
   0x2b, ...) are the LEGACY FALLBACK generation; the primaries for their
   formats are the modern family analysers bound to the magic keys. The
   ADTS frame math lives inside the modern analyser (:65536+, the 12-bit
   sync/0xFFF1 walker). The core's single-primary posture (no fallback
   generation) is the REGISTERED D-75 — with the og-side boundary now
   decoded: modern reject → legacy try → both reject → discard.

3. **WPG canonical-size stamp — RESOLVED (100,000,000 bytes; the core has
   no WPG at all).** `RawSpeed::WpgAnalyser` (type-id 0x45, key
   `0x677077` = "wpg", registration :2180-2187) magic-gates on the 3-byte
   "wpg" ASCII (FUN_180014030 :15653-15666 — pure magic, no content walk)
   and its `WPGRawFileInfo` factory (FUN_180014090 :15685-15699) stamps a
   CANONICAL MAXIMUM of 100,000,000 bytes (slot +0x23) with the extent
   header-derived and sanity-bounded (the ×5 factor, FUN_180014160
   :15712-15748). The core carries NO WPG format (zero rows in the
   grammar registry, validators, and raw-sign table) — the size-semantics
   question is superseded by an absence: WPG belongs to the S-007
   remainder, now with the og-side facts measured (key "wpg", type 0x45,
   max 100 MB) for the format-implementation lane.

4. **CNeedRawMgr gating — RESOLVED (per-format config, NOT scan-mode).**
   CNeedRawMgr is a config singleton (once-init FUN_180010a40
   :13056-13073; ctor vftable + a map) initialized at engine init and
   populated from `config/RawConfig.ini`'s [Filter] section
   (FUN_180010b10 :13120+, `GetPrivateProfileSectionA("Filter", ...)`).
   Every analyser consults it AT DETECTION TIME, extension-keyed (~20
   uniform sites: :3334/:3390/:3454/.../:17818/:23896/:33184/:38587 — each
   builds the detected format's extension string after a successful
   identify and queries the manager). No scan-mode parameter exists
   anywhere in the consult pattern — **there is no quick-vs-deep
   validator-mode gating in the og's RAW engine**. The core's
   `disabled_formats` (M-024) is the equivalent gate; the core's
   "all validators run in the carve pass" matches the og (gated only by
   per-format config, which the core implements).

**Verdict: OQ-SA-2 CLOSES.** Two probes landed pure decode resolutions
(no core change needed — D-75/M-024 were already the registered posture);
the WPG absence is a data point on the S-007 remainder trail (not a new
finding — the 67-of-131 not-implemented census already owns it; the
measured og facts recorded here feed that lane).
