# S3-CARVE scratch report (consolidated by the session-3 lead)

**Agent:** S3-CARVE (parallel deep-pass) · **Outcome:** agent hit the context deadline
after filing 7 findings + the inventory evidence; the lead (session-3 main agent)
completed the consolidation steps (OQ-T8 resolution append, carve module rows, errata
E19–E29, I-009 filing, S-007 addendum, this report).

## (a) What was read (per the filed artifacts' citations)

- `RAWSearchDLL_decompiled.c` (80,614 lines): registry architecture
  (:4216-4434 singleton/init/getter; :1436-2515 u32-map builder; :2517-2914 u16-multimap
  builder FUN_180004320; :4386-4434 list builder; :4450-4484 find-or-create), dispatch
  (:9842-9925 FUN_18000cf60; :9933-10063 u16 chain walk FUN_18000d3e0; :7916-7961 node
  insert), scan-loop/chunk driver (:3013-3055 only_common config; :8xxx-10xxx sector
  loop + pending-object continuation :10114; EUVideoRepair feed :9625), per-analyser
  validator bodies cited throughout the inventory §3–§7 (JPEG both generations, TIFF
  family, ZIP, BMFF/MOV/HEIC, PST/OST, DBX, HLP, EPS, CRW, MRW, 3DS, MP3, OneNote,
  LZH, RawConfig.ini filter read :3013/:3017-3055).
- `RAWSearchDLL_functions.txt`, `RAWSearchDLL_imports.txt` (symbol/address/import index).
- `og-codebase/RAWSearchDLL.dll` (PE-walks for signature bytes/RTTI strings).
- Core: `src/core/carve/cr_carve.cpp`, `cr_registry.cpp`, `cr_analysers.cpp`,
  `cr_rawsigns.cpp`, `cr_rawsigns_data.cpp` (full read for the comparison table §9).
- Corpus docs compared against decode: `raw-dual-registry--u16-magic-keys.md`,
  `rawsearchdll--analyser-registry-census.md`, `rawsearchdll--zip-analyser-carving.md`.

## (b) Findings filed

| ID | Title | Gap (1-line) | Evidence type |
|---|---|---|---|
| S-012 | ftyp-family signatures unreachable (MP4/MOV/M4A/HEIC dead) | core's 8-byte signature encodes the ftyp box SIZE as literal zeros — real headers never match; og carves these via u32 4CC + MovAnalyzer/HEICAnalyser | decompile + executed probe (`evidence/executions/s3-ftyp-signature-deadness/`) |
| S-013 | JPEG EOI policy — og salvages truncated, core rejects | no 0xD9 anywhere in either og JPEG analyser generation; core requires EOI+SOF+SOS | decompile (both generations quoted) |
| M-011 | No runtime format-filter surface vs og RawConfig.ini [Filter] | og rebuilds registry per search with 37 named toggles (STL/VPJ default-off); core's table is compile-time | decompile + RawConfig.ini read |
| M-012 | Read model: og 8 MiB ping-pong + sector dispatch + streaming; core 1 MiB byte-granularity stateless | structural cost/yield divergence; core results equivalent-or-better (see I-009) | decompile (chunk driver decoded) |
| N-023 | Dispatch order decoded: u32 4CC first (OQ-T8 resolved; corpus doc errata) | no core behavioral gap; two corpus artifacts had the order/key-widths wrong | decompile |
| N-024 | Og carve feeds EUVideoRepair per chunk + per finalized video | carve-side engine integration absent in core (repair is a separate product surface) | decompile (:9625, :10114) |
| N-025 | Analyser registry exports consumed by FS engines; no core ABI | EUGetAnalyser{List,Map,Multimap} cloned by FS SearchDLLs; core has no enumeration API | decompile + imports census |
| I-009 | (filed by lead from the agent's inventory notes) Core byte-granularity superset | og dispatches only sector-aligned positions; core finds mid-sector starts | decompile + code read |

## (c) Newly verified MATCH parameters

- A25 (dispatch sequence/collision order) — MATCH for all shared formats (N-023;
  kTrialOrder equivalence verified per family, incl. TIFF-family relabeling).
- C19 (registry keys) — MATCH for the shared subset; the divergences are S-012/S-013
  (filed) and the missing families (S-007, amended).
- A28 — MATCH for shared formats except the two filed divergences (JPEG S-013, ZIP N-008
  prior).

## (d) UNVERIFIED residuals (with next artifacts)

1. `format id 8` continuation records: whether the position map's id-8 entries surface in
   results as generic files — next: consumers of `FUN_18000e980/e9d0`.
2. EUVideoRepair's repair-side detection — next: `EUVideoRepair.dll` binary (strings +
   exports) or its decompile if present.
3. Registry-clone call sites inside FAT scan path (members +0xb8/+0xd0/+0xe0) — next:
   FATSearchDLL_decompiled.c readers.
4. Naked-MOV salvage path per-box grammar (if S-012 fix step 3 is built, decode the SOF
   -equivalent box arithmetic first) — candidate formula noted in S-013.
5. RawConfig.ini's writer host component (DRWUI settings vs installed default) — low
   priority.
6. The og's UI progress weighting for the chunk driver (O38 re-score) — next: the
   controller's chunk-driver virtual in RecoveryModule decompile.

## (e) Verdicts appended

- `modules/carve.md` — Session-3 section (A25/A28/C19/A42/A45/P47–P53 verdicts + S3
  findings index) — appended by the lead from the agent's findings.
- `tracked-open/OQ-T8-T9-…md` — OQ-T8 RESOLVED section (appended by the lead).
- `CORPUS-ERRATA.md` — E19–E29 (appended by the lead).
- `findings/SEVERE/S-007-…md` — DBF-correction addendum (appended by the lead).
- Unfiled planned IDs: N-026 (per-layer format tables — folded into the inventory
  evidence §4–§9 instead of a separate finding; ID retired).
