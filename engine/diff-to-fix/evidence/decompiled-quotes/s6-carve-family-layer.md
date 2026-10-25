# Evidence — session 6: the RAWSearchDLL family layer (never-cited artifacts, first consumption)

**Session:** 6 deep-continuation (2026-10-26) · **Audited core commit:** `022c81d`-lineage · **Corpus:** `03ad46c`
**Scope:** the six carving-family artifacts + the analyser-registry census + the dual-registry u16 decode + the zip-analyser carving decode — cited by S-007 (addendum), N-052, N-053, N-054, M-042. All quotes verbatim from the corpus at `03ad46c`.

## 1. The two-generation fallback chain

`re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:43`:

> **The two-generation design**: every format has a modern analyser (namespaced, FileInfo-paired) and — where one exists — a legacy fallback `File*Analyser` (first generation, flat names). When the modern analyser rejects a candidate (validation failure), the chain falls to the legacy implementation before giving up. This is why carving rarely false-positives: two independent validators must both fail before a match is discarded, and one strong validator accepting is enough.

## 2. CAB dual-case keys

`rawsearchdll--analyser-registry-census.md:94` (magic-key table):

> | `MSCF` / `mscf` | 0x4643534d, 0x6663736d | Microsoft CAB (both cases — case-insensitive formats) | FileCABAnalyser |

`:127`:

> 3. **Case-insensitive formats get duplicate keys** (`MSCF`/`mscf`) rather than case-folding — deliberate registry bloat for correctness.

Core side: `src/core/carve/cr_registry.cpp:212` — `{FMT_CAB, "cab", {'M','S','C','F'}, 4, 0, 36, 2ull << 30, 870, validate_cab, "Cabinet"}` (uppercase only; grep `mscf` across carve: 0 hits).

## 3. ZIP three-anchor entry points + LFH sanity gates

`re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md:13-21`:

> ## 1. Entry points — all three ZIP structures accepted
>
> The analyser can begin carving from ANY of the three ZIP structures (first DWORD at candidate offset):
>
> | Magic (LE dword) | ZIP structure | Use as carving anchor |
> |---|---|---|
> | `0x04034b50` = PK\x03\x04 | Local File Header | accepted with sanity checks: `version_needed ≤ 10` (offset +8, ushort) and extradata/flags field at +0x1a in `[1, 0x102]` |
> | `0x02014b50` = PK\x01\x02 | **Central Directory entry** | preferred anchor — index-based walk |
> | `0x06054b50` = PK\x05\x06 | End Of Central Directory | accepted (truncated/EOCD-first zips) |
>
> Anything else → `param_2[2] = NULL; return` (no match — the analyser rejects the candidate outright).
>
> The registry binds the chain to `PK\x03\x04` only, but the analyser itself handles all three once invoked — meaning ZIPs whose first surviving structure in the raw stream is a CD entry or the EOCD are still carved.

## 4. ZIP inner-file inventory

`rawsearchdll--zip-analyser-carving.md:37-38`:

> - **Per-entry parser** (`FUN_180045690`) reads the standard ZIP fixed header (name len at +0x1c, extra len +0x1e, comment len +0x20 for CD entries; compressed/uncompressed sizes, local-header offset) and accumulates exact total size into `local_f0` — the final carve size is computed from the SUM of parsed entries, not guessed.
> - Entry names accumulate into a list (local_68/local_a8 vectors, FUN_180007060 = merge) — the analyser returns the complete inner file inventory, which the UI later presents as the archive's contents.

`:64`:

> - Entry list = merged into result via FUN_1800173f0 — consumed by the results-tree builder (ZIP files expand to their inner file list in the DRWUI tree)

## 5. MPEG frame-sync dual-registry keys

`re-workspace/artifacts/SearchDLL/raw-dual-registry--u16-magic-keys.md:38-41`:

> | 0xFFF1 | MPEG sync | **AAC ADTS** (MPEG-2/4 audio transport — CADTSAnalyser) |
> | 0xFFF2..0xFFFF | MPEG syncs | **MPEG audio layer family** (12 sync variants) |
> | 0xFFE2..0xFFE7 | MPEG syncs | **MPEG Layer I/II + MPEG-2 variants** (6 codes) |
> | 0xF9FF (63999) | 0xFF F9 | MPEG-2 audio extension sync variant |

Core side: `cr_registry.cpp:197` — `{FMT_MP3, "mp3", {'I','D','3'}, 3, 0, 512, 512ull << 20, 820, validate_mp3, "MP3 audio"}`; `cr_analysers.cpp:1247-1248` — `if (std::memcmp(h, "ID3", 3) != 0) return r;` (the validator itself requires ID3).

## 6. Overlap scoring

`re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md:73`:

> raw_signs.json entries carry explicit confidence (e.g. "0,44564456,0.96" for ifo). The engine accumulates header + tail confidences to rank candidate carves (multiple overlapping candidates resolved by score).

Core side: `cr_carve.cpp:135-136` — `emitted_at.insert(cand_abs); break;` (first accepting validator owns the position; no score comparison among candidates).

## 7. Family rosters (the ~45-format census, summarized)

`rawsearchdll--analyser-registry-census.md:45-117` — the magic-key table + family namespace census (`@BruteForce` 24 classes/20 analysers: CADTS, CAPE, CAsf, CDWG, CDXF, CEPS, CFCPEvt, CFH11, CGif, CM2TS, CMTS, CMP3, CJpeg, CMov, CPdf, CTiff + FileInfo pairs; RawSpeed/Brance/FileGen/Special/xjq families: GIF, BMP, SWF, TTF, WMF/EMF, WPD, OLE, MDB, MNY, ONE, PSP, SIT, TAR, GZ, HTML/SVG, LZH, MRW, CRW, X3F, DMG, MXF, OGG, RM, MPG, ADTS, APE, DWG, DXF, EPS, ICO, HLP, CHM, CPT, AU, AIF, VCF, GPX, AEP, 3DS, 3GP, JP2, MOBI, dBase, FCPEvt, FH11, WPG …). Full table in the artifact; the per-format core-side grep result (which of these exist in `cr_registry.cpp` / `cr_rawsigns_data.cpp`) is reproduced in the session-6 context sheet (scratch, not committed) — the census-classification: none of the listed family formats have core grammar validators; a subset ride the raw-sign table under different keys.

## 8. Core-side registry inventory (verification snapshot)

`src/core/carve/cr_registry.cpp` rows at the audited commit: JPG, PNG, TIFF, CR2, WEBP, HEIC, PDF, ZIP, DOCX, 7Z, SQLITE, MP4, MOV, AVI, MKV, RAR4, PST, RAR5, DNG, ORF, RAF, RW2, PSD, MP3(ID3), WAV, M4A, ASF, FLV, CAB(MSCF), ISO, TIFF_BE, ORF_IIRS, ORF_MMOR, ZIP_CD, MOV_NAKED (+ raw-sign layer: 139 formats long-tail). Verified by direct read; the census-format diff is the S-007 addendum's roster claim.
