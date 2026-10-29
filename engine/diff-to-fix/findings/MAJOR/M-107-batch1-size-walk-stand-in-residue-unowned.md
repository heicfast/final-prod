# MAJOR M-107 — the batch-1 size-walk stand-in residue is unowned: eleven formats still carve at the DWG-pattern 64 MiB cap (X3F CRW WPG WPD PSP SIT APE EPS AI DXF + DWG's own section walk + SWF's RECT bit-walk) where the og performs a per-format size walk — S-007's "recorded follow-ups" never became findings

**Severity:** MAJOR
**Module / Surface:** carve (modules/carve.md) — the S-007 batch-1 follow-up residue
**Parameters affected:** A25 (engine algorithm steps — per-format size walks), A27 (carved extent end), A47-adjacent (the 64 MiB stand-in cap vs per-format og bounds)
**Corpus references:** `diff-to-fix/findings/FIXED/S-007-carve-missing-common-formats.md:245-248` (the standing registration: "Formats whose og size-walk is undecoded (WPG WPD PSP SIT APE X3F MRW HLP MDB EPS AI DXF) use the DWG-pattern bounded extent … the per-format size-decoding walks are the recorded follow-ups, exactly like DWG's section walk before them"); `re-workspace/artifacts/FileRepair/x3f-repair--section-checkers-walk.md:9-33` (the never-cited residue's corroboration: the X3F section-directory grammar — SECi/SECe/SECm/SECp/SECc section checkers, directory-first validation — sitting in the FileRepair line's repair-walk artifact; first read this session, SA-10/LANE-C); the og walk function sites recorded per format in the core's own validator comments (cr_analysers.cpp — the sites enumerated below), each naming its undecoded og walk.
**Core references:** audited commit `ea1ac86`-lineage (cites re-verified at the reconciliation HEAD — the M-090/M-084/M-091 carve fixes shifted the file AND resolved two members of S-007's original list: MDB's walk landed with M-091, DMG's with M-090). The stand-in sites, each verified first-hand with its "recorded follow-up"/"stands in" comment: `src/core/carve/cr_analysers.cpp:3079-3084` (X3F — "the section walk is the recorded follow-up"), `:3104-3109` (CRW — "The og's CIFF heap size-walk (FUN_180052160) is the recorded follow-up; the DWG-pattern bounded extent stands in"), `:3193-3197` (WPG — "the size walk is the recorded follow-up"), `:3210-3214` (WPD — "product-version word at +4 carry the gate, bounded extent stands in"), `:3232-3236` (PSP — "the 19-byte prefix is the real discriminator; bounded extent stands in"), `:3249-3253` (SIT — "bounded extent stands in for the undecoded index walk"), `:3325-3329` (APE — "bounded extent stands in for the undecoded frame walk"), `:3698-3702` (EPS — "trailer scan is the recorded follow-up"), `:3737-3741` (DXF — "bounded extent stands in for the text walk"), `:2639-2645` (DWG — the comment's "a full section walk is a follow-up" at `:2644` — "a full section walk is a follow-up; the version gate + bounded extent is the honest floor"), `:3910-3914` (SWF — "The RECT bit-walk past byte 8 is the recorded follow-up"); plus the pattern header `:2795-2806` (the S-007-era rule, unchanged) (the S-007-era rule: "formats whose og size-walk is undecoded use the DWG-pattern bounded extent heuristic (the sanctioned honest cap, conf <= 650, note names the heuristic)"). AI shares the EPS-family trailer/text walk (the EPS validator's family; no separate AI validator exists — the EPS row's `.ai` extension is served by the same stand-in).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

S-007's batch-1 landing (27 formats) shipped honest key/gate grammars but used a generic 64 MiB "DWG-pattern bounded extent" wherever the og's per-format size walk was not yet decoded, recording the walks as "follow-ups" in the finding and in code comments. That was the sanctioned posture for a first landing — but the follow-ups were never re-registered as findings, so the residue has lived as comments in source with no board owner. Ten sites stand today (eleven minus MDB, resolved mid-flight): X3F, CRW (the M-081 re-landing kept the heap-walk follow-up), WPG, WPD, PSP, SIT, APE, EPS, DXF, DWG's own section walk (the pattern's namesake), and SWF's RECT bit-walk. (MRW/HLP were resolved with og formulas in M-081's fix; MDB's page-chain walk landed with M-091's fix and DMG's koly walk with M-090's — both now FIXED; JP2 is owned by N-111; ONE/3DS by M-106.) The og determines each of these formats' sizes by a real walk (section directory, heap chain, index, frame chain, trailer scan, group-code text walk); the core caps at 64 MiB — files whose true size exceeds the cap carve truncated, and files smaller than it carve without the og's walk-broken salvage semantics.

## What the reference does

Per format, the og's analyser walks the file's own structure to determine the carve extent — the sites are named in the core's own comments (each from a first-hand registration/analyser read at landing time): X3F walks the section directory (SECi/SECe/SECm/SECp/SECc — corroborated by the FileRepair artifact `x3f-repair--section-checkers-walk.md:9-33`, which documents the same section checkers for the repair line); CRW walks the CIFF heap chain (FUN_180052160); WPG walks its size field chain; WPD gates on the product-version word and walks the object header chain; PSP discriminates on the 19-byte "Paint Shop Pro Image File" prefix and walks its header-declared extent; SIT walks the StuffIt entry index; APE walks the frame chain; EPS scans for the trailer; DXF walks the ASCII group-code text; DWG walks the version-dependent section-locator map (R13-R2000) or the compressed R2004+ map; SWF parses the RECT bit-field past byte 8 to validate beyond the FileLength field. The common contract: the carved extent is the walk's natural end (or the walk's break point, salvaging the walked prefix), bounded by the FileInfo row max — never a fixed 64 MiB.

## What our core does

Each of the eleven validators gates correctly on its key/grammar (the S-007/M-081 landings' work — verified) and then returns `r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap))` with a comment recording the stand-in (the sites and comments enumerated in Core references, all first-hand at HEAD). The pattern header at `:2795-2806` documents the rule under which this was sanctioned: the honest cap, confidence ≤ 650, the note naming the heuristic.

## Gap

Falsifiable: for each of the eleven formats, a fixture whose true structure-derived size exceeds 64 MiB (within the row's og bound) carves at exactly 64 MiB in core, while the og carves it at the walk's end; and a fixture whose structure breaks mid-walk carves differently (the og salvages the walked prefix at the break; core takes the blind 64 MiB or the row cap). The follow-up registration exists (S-007:245-248 + the code comments) but no finding on the board owned it until this one — the exact "recorded outside an active finding" state the session mandates close.

## Impact

User-observable truncation for over-cap files in eleven families (the same M-090/M-091/M-104 class), plus missing walk-break salvage semantics (a torn file carves at a structure-aware boundary in the og, at a blind cap in core — the og's carved prefix is a valid partial file; core's can cut mid-record). The practical frequency is lower than the school-2/3 absences (these formats all carve — the defect is the extent, not the presence), but the class is identical and the residue was explicitly promised follow-up status by S-007's own resolution.

## Reproduction / verification

Corpus-derived + decompile-site citations (strength 2/3): S-007:245-248 (the registration, quoted), the per-format og walk function sites in the core's landing-time comments (each was a first-hand read at its landing; re-verified as comments at HEAD this session), and the X3F section-directory grammar corroborated by the never-cited FileRepair artifact (first read this session). Core side: the eleven stand-in sites verified first-hand at HEAD with line numbers. Not executed: per-format plant fixtures belong to each landing's tests (the fix plan's order).

## Fix plan (for the engineering team)

1. **Per-format decode-then-land order (the S-007 zero-invention bar — each landing re-reads its og walk first-hand before implementing):** start where corroboration already exists — (a) **X3F** first (the section-directory grammar is documented in `FileRepair/x3f-repair--section-checkers-walk.md:9-33` — the only walk with a written grammar in the corpus); (b) **CRW** second (FUN_180052160, the heap chain — the M-081 landing's own recorded follow-up); (c) **DWG** third (the R13-R2000 section-locator map; the R2004+ compressed map may stay behind a follow-up of its own, recorded honestly); then WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT in registry order.
2. Each landing: replace the `64 << 20` stand-in with the walk, keep the row max as the clamp, extend the plant suite with an over-cap fixture (fail-before: carves at 64 MiB) and a torn-walk fixture (walk-break salvage semantics), update the comment to name the implemented walk.
3. If a walk cannot be decoded in a landing window, land the others and leave the stand-in WITH its finding updated (this finding's row in the matrix gains the per-format status) — never silently.
4. **Acceptance:** the eleven comments' "recorded follow-up"/"stands in" phrases are gone (grep clean) or owned by an explicitly re-registered remainder; the over-cap plants carve og-exact; the school-1/batch-1 suites stay green.

## Tests to add

Per format (in the decode-then-land order): `plant_<fmt>_over_cap` (true size > 64 MiB within the row bound — carves og-exact, fails before at 64 MiB) and `plant_<fmt>_torn` (structure break mid-walk — the walked prefix salvages; core's carve ends at the break, not the blind cap).

## Related findings

- **S-007 (retired to history with the purge; the FIXED-LEDGER row + the retired file carry the registration)** — the parent landing that sanctioned the stand-ins and recorded the follow-ups (the registration quote: "Formats whose og size-walk is undecoded … the per-format size-decoding walks are the recorded follow-ups, exactly like DWG's section walk before them").
- **M-106** — the two school-1 walks that WERE decoded (ONE/3DS) — the same pattern, fixable now.
- **M-090 / M-091** — the decoded-walk precedents (DMG/MDB): truncation class + the bounded-extent stand-in replaced by a real walk.
- **M-081 (FIXED)** — the CRW re-landing whose heap-walk follow-up this finding owns.

## Open questions

1. AI (Adobe Illustrator): served by the EPS-family validator's stand-in — whether the og's AI walk differs from EPS's trailer scan is undecoded (the EPS landing read only the EPS analyser); the AI row should be re-read at its own registration site during the EPS landing.
2. DWG's R2004+ compressed section map: likely its own follow-up even after the R13-R2000 map lands (the landing-time comment already splits the two eras).
3. SWF's RECT bit-walk: the landing gated on FileLength (u32@4) which IS the og's size source; the RECT walk's marginal value is validation depth (rejecting corrupt FileLengths), not extent — verify at FUN_180026940 during the landing whether the og's RECT parse changes the extent or only the accept/reject decision, and scope the fix accordingly (it may downgrade to a validation-only change).


---

## Session-11B addendum (2026-10-29, re-triage at `743d7b8d`): all eleven sites verified standing; three in-file details corrected; + the enumeration extended by M-111

**All eleven sites re-verified standing** (cites drifted ~+300-400 lines after the M-092/M-093/M-096 landings; every site re-located by its comment): X3F :3465-3478, CRW :3490-3512, WPG :3578-3590, WPD :3595-3612, PSP :3617-3629, SIT :3634-3646, APE :3710-3725, EPS :4083-4095, DXF :4122-4134, DWG :3020-3056, SWF :4295-4312 (all in src/core/carve/cr_analysers.cpp). The M-093/M-096 landings cleared NONE of them (their hunks confined to the HTML/XML/SVG/MOBI validators).

**Three in-file corrections (the verdict is unaffected):**
1. The "no separate AI validator exists" note is stale — `validate_ai` has existed since the S-007 batch-1 commit `e018bf4f` (`git log -S'validate_ai'` shows only that commit); AI stands in via its OWN validator's 16 MiB bounded extent (comment :4097-4100, cap :4112), not the EPS validator's.
2. EPS and AI stand at **16 MiB**, not 64 MiB (`16 << 20` at :4091 and :4112) — the finding's "64 MiB" figure covers the other nine sites only.
3. SWF's site takes its size from FileLength (:4308) with no 64 MiB cap — as the finding's own open question 3 anticipated.

**Enumeration extended:** the M-093 landing's own residual — the CSL 0x44c size branch (dword@0x44c + 0x480, corpus FUN_180041950 :49749+) — is the same unowned-stand-in class; filed as **M-112** this session (this lane filed it as M-111; renumbered at the push-time reconciliation over the parallel S11 lane's explorer-browse M-111) (it was recorded only in a code comment + the retired M-093 Resolution). The class now has twelve member sites across two findings (M-107's eleven + M-112's CSL).


---

## S11-R addendum (2026-10-29, the reconciliation lane; re-verified at `b6e0a9ef`)

**Two factual corrections to this finding's own text (this lane's re-read;
the 0x44c CSL fold is already landed above as M-112 — the enumeration stands
at twelve member sites + M-112's):** (a) AI **has its own validator**
(`validate_ai` in cr_analysers.cpp, FMT_AI row in cr_registry.cpp) — the
finding's "no separate AI validator exists" parenthetical is wrong (the
stand-in substance holds: the validator's size walk is the DWG-pattern
bounded extent, not the og's); (b) the EPS/AI stand-ins are **16 MiB**, not
the title's blanket 64 MiB — already so at the filing baseline (verified via
the filing commit's tree). Cosmetic to the finding's own text; recorded so
the fix wave does not re-derive the wrong bounds.
---

## Session-43 addendum (2026-10-29 — the N-146 re-homing: the FH11 site joins this enumeration)

**N-146 resolved the one un-owned stand-in this finding's enumerations missed:** the
FH11 row's zero-end stand-in (`cr_analysers.cpp`'s `validate_fh11`, `end = hit +
(32ull << 20)` — the S-037-era landing, its owner purged with the S41 sweep) is now
REGISTERED here: the twelfth site, a DIFFERENT stand-in class than the eleven batch-1
members (the og's FUN_180029f30 :32612-32633 stamps the CZeroEndRawFileInfo id 0x1d
with NO size walk — the pending-object engine's zero-end metric carries the extent,
the D-101-registered architecture decline; the 32 MiB cap is the zero-end family's
salvage posture, not the batch-1 64 MiB DWG pattern). The constant is PINNED
(`Carve.Fh11BoundedExtentPinned` — the exact 32 MiB extent on an unclamped 40 MiB
device; the pin's fail-before proven by mutation: 32→16 MiB goes RED). The site's
disposition: stand-in stays (the D-101 decline covers it); the registration gap that
was N-146's actual defect is closed.


---

## Session-78 fix-lane addendum (2026-10-29 — the per-format landings begin; X3F landed)

**Per-format status (the fix plan's landing-order record):**
1. **X3F — LANDED (this push, S78 lane):** the section-directory walk
   (Brance::X3FAnalyser, RAWSearchDLL FUN_1800404a0 :48851-48910 + the
   magic finder FUN_18003ffc0 :48624-48638, both re-read first-hand this
   session): the directory is SEARCHED for ("SECd" = 0x64434553, the same
   literal the walk's chunk-tail probe compares — Sigma writes the
   directory at a variable position after the section data); the entry
   count at magic+8; the extent = magic + 16 + count×12 (the end of the
   12-byte-stride entry table behind the 16-byte directory header; the
   caller's entry[0] reach-through at :48741/:48770 corroborates the
   layout), clamped by the row max + device end (the og's own clamp at
   :48904-48906/:48745-48750). No directory in the window = the og's
   decline (the return-1 family) — the blind 64 MiB stand-in is gone.
   The og's streaming chunk-tail gate is its chunked-feeding model's
   artifact (documented divergence of model, not semantics). Tests:
   X3fSectionDirectoryWalkSetsExtent / X3fNoSectionDirectoryDeclines /
   X3fGarbageHugeCountClampsToWindow — RED first-hand via the stash
   dance (2/3 FAILED pre-fix: the stand-in's blind extent + blind
   carve); the S-007 batch re-pinned (the plant now carries the real
   directory; the compact payload keeps the batch slot). The FileRepair
   artifact's directory-first grammar corroborated
   (x3f-repair--section-checkers-walk.md §2).
2. **CRW — NEXT** (FUN_180052160, the CIFF heap chain).
3. DWG, then WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT — pending.
