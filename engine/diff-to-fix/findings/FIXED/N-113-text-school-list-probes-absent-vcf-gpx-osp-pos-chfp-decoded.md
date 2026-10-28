# MINOR N-113 — the og's small text-format list probes are absent as a group: VCF (BEGIN:VCARD, 20 MiB), GPX (<?xml + <gpx, end-tag), OSP (OpenShot, "libopenshot": needle, 2 MiB), POS (ProcessOn {\"diagram\":), and CHFP (<BiffProject XML, 100 MiB) — every grammar now decoded

**Severity:** MINOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1-adjacent, A2-adjacent (five niche text formats), A28-adjacent
**Corpus references:** All cites: `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` — **VCF**: registration :3390-3453 (CVCFAnalyser id 0x88, RawConfig "VCF" token); probe :45316-45358 (`_memicmp(buf,"BEGIN:VCARD",11)==0`); walk :45363-45429 (chunked scan for END:VCARD + CRLF, **continues on another BEGIN:VCARD** for multi-card files; cap 0x1400001 ≈ 20 MiB). **GPX**: registration :3947-4074 (CGPXAnalyser id 0x97); probe :20860-20936 (the `<?xml version=\"` declaration, then a scan for u32 `<gpx` from +0x16 up to 0x200 iterations); walk FUN_18001b220 (the `</gpx>` family end-tag). **OSP** (OpenShot): :3695-3758 (COSPAnalyser id 0x8f); probe :50104-50154 (the 10-byte prefix `{\r\n "di": ` as a u64+u16 compare); walk :50159-50317 (chunked 0x200 scan for `"libopenshot":` 14-byte needle, size = needle + 0xF, cap 0x200000 = 2 MiB; not found → reject). **POS** (ProcessOn): :3631-3694 (CPOSAnalyser id 0x8b); probe :20203-20245 (`_memicmp(buf,"{\"diagram\":",11)==0`). **CHFP**: :3759-3822 (CHFPAnalyser id 0x90); probe :19939-19996 (the XML declaration AND `_memicmp(buf+0x28,"<BiffProject",12)==0`); walk :20001-20184 (chunked `</BiffProject>` scan, trailing-NUL trimming, cap 0x6400000 = 100 MiB).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — no BEGIN:VCARD, no <gpx, no libopenshot, no diagram, no BiffProject probes (grep-verified)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred the whole text school; this session decoded each grammar. Grouped into one finding (all niche, all list-layer, all the same fix shape); the common formats (HTML/XML/SVG) got their own findings M-092/M-093.

## Summary

Five small text formats the og carves through RawConfig-token-gated list probes are absent from core: vCard contacts (.vcf — with multi-card continuation semantics), GPX tracks, OpenShot projects (.osp), ProcessOn schema files (.pos), and the <BiffProject XML project format (.hfp — vendor unidentified in-binary). Each grammar is small and fully decoded (quoted in the corpus field); each is MINOR individually (niche, low volume), so they are grouped here with per-format fix bullets — but per the roadmap discipline each is a distinct row/validator when implemented.

## What the reference does

Per format: see the corpus field — each probe is a 1-2 memcmp/u32 gate; each walk is the family's chunked end-marker scan with its own cap; VCF uniquely continues across multiple BEGIN:VCARD blocks; OSP uniquely REJECTS when its size needle is absent (the only reject-on-missing in the family).

## What our core does

None of the five probes exist in core.

## Gap

Falsifiable per format: e.g. "BEGIN:VCARD" + ... + "END:VCARD\r\n" carves in the og (id 0x88, extent to the card end); core carves nothing. Same shape for the other four.

## Impact

Low-volume but high-sentiment data (contacts, GPS tracks); project-file recovery for three editors. The group also pins the RawConfig token-gating architecture for the list layer.

## Reproduction / verification

Decompile-direct: every probe/walk was read this session and is quoted with line cites in the corpus field.

## Fix plan (for the engineering team)

One list-probe seam (shared with M-092..M-099), then per format: VCF (BEGIN gate, END walk with multi-card continue, 20 MiB cap); GPX (declaration + <gpx window, </gpx> walk); OSP (10-byte prefix, libopenshot needle REQUIRED, 2 MiB cap); POS ({\"diagram\": gate); CHFP (declaration + <BiffProject@0x28, </BiffProject> walk with NUL trim, 100 MiB cap). Each gets its own FMT id + RawConfig-parity CarveFlags toggle.

## Tests to add

Per format: an accepted canonical fixture, a reject mutation, and (VCF) a two-card continuation case; (OSP) the missing-needle reject case.

## Related findings

S-007 (batch-2 parent); M-092/M-093 (the text-school majors); S-047 (the list-layer seam).

## Open questions

CHFP's vendor identity is not identified in-binary ("HitFilm Project" is an inference from the token + XML root — recorded as INFERRED, not load-bearing: the grammar is what parity needs). POS's size-walk needle is inferred from the ProcessOn string cluster — one read of FUN_18001a470 settles it before landing.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-113 landing (the session-39 fix lane; cite-by-title: "fix(carve): N-113 - the text school lands (VCF / GPX / OSP / POS / CHFP)")
**Verified by:** `Carve.TextSchoolVcfAcceptedAndExtent` + `TextSchoolVcfMultiCardContinuation` (the two-card continuation) + `TextSchoolVcfNoEndTagFallsBackBounded`; `Carve.TextSchoolGpxAcceptedAndExtent` + `TextSchoolGpxRootBeforeWindowRejects` (the +0x16 window semantics); `Carve.TextSchoolOspAcceptedAndRejectOnMissing` (both halves: the needle+0xF extent AND the reject-on-missing); `Carve.TextSchoolPosAcceptedAndExtent` (the settled needle); `Carve.TextSchoolChfpAcceptedAndExtent` + `TextSchoolChfpRootOffAnchorRejects` (the +0x28 anchor). All RED pre-fix by construction (none of the five probes existed). Full suite 1073/1073 (1031 passed / 42 Win-only skipped / 0 failed, linux-release); clang-format clean; F8 + S-017 self-probe green with the six new rows.
**Resolution notes:** Every probe and walk re-verified first-hand in the corpus before the fix (the line cites in the finding held up exactly). **The POS open question settled by the demanded read**: FUN_18001a470's walk needle is "ProcessOn Schema File" (21 bytes), extent = find + 21 + 0x13, cap 0x6400000 - the finding's inference direction was right, the exact needle now decoded. Additional decode beyond the finding: GPX's walk cap is 0x6400000 (100 MiB - the id != 0xa0 branch of the shared walk FUN_18001b220); GPX's probe window semantics pinned by test (a root element BEFORE +0x16 rejects - the short-declaration form misses the window); VCF's per-chunk scan over-reads 10 bytes past the 0x400 advance (the u64 compare chain) - ported; the 0x200-chunk finds are PER-CHUNK (the og's chunked std::string find misses needles straddling a 512-byte boundary) - ported exactly via chunk_find; the og's 64 KiB sliding-window trailing-NUL trim adapted as the family's bounded fallback at the slice cap (the M-092/M-093 convention for this school). The case-insensitive og gates (_memicmp: VCF's BEGIN:VCARD, POS's {"diagram":, CHFP's <BiffProject) are expressed by the canonical-case registry magics - the exotic-case subset beyond the magic is a registered detection subset (the M-092 HTML class); the validators' internal re-checks are case-insensitive where the og's are (CHFP's root gate). CHFP's vendor identity remains INFERRED ("HitFilm") as the finding records - not load-bearing; the ext "hfp" per the og's extension table. Architecture: six rows (the GPX BOM-form twin mirrors the M-093 XML pair); GPX/CHFP share the "<?xm" key with the M-093 XML rows - the grammars reject each other's content (N-070 ranks the accepting candidate; verified: validate_xmlsvg rejects GPX content - no svg/csl marker). Ids APPENDED (serialized carve_format values stay stable).
