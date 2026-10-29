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

## Session-78 fix-lane addendum (2026-10-29 — the per-format landings begin; X3F landed — **SUPERSEDED-BY-S71 for the X3F member**: this lane's count-walk decode read FUN_1800404a0, the Brance::X3FAnalyser streaming chunk callback — the wrong authority; the S71 lane's re-decode of FileX3FAnalyser FUN_1800535b0 (the File*Analyser secondary, the M-090 DMG precedent's established size-walk authority) is the corpus-true walk: SECd + 0x4C in all three found-branches, loop-exit salvage, the count never read. The M-155 repair resolved the ed501caa push's committed conflict markers in this lane's favor; this lane's three X3F tests were replaced by corpus-true pins.)

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

---

## Session-71 addendum — the X3F landing (the finding's own decode-then-land order, member 1 of 11)

**X3F FIXED** (this session; the remaining ten members stay with the finding): the og's X3F size walk is decoded first-hand from FileX3FAnalyser FUN_1800535b0 (RAWSearchDLL :63457-63560, read in full this session): a forward scan for the "SECd" section-directory marker (0x64434553), sector-stepped with overlapping 2-sector windows up to the device cap, with the extent = SECd_position + 0x4C in ALL THREE found-branch shapes (the sector-aligned hit `uVar7 += 0x4c; break;`, and both in-buffer search hits' `piVar5 + ... + 0x4c` forms — the directory's fixed 76-byte tail, which by construction IS the file's end: the X3F writer appends the directory last, corroborated by the FileRepair artifact x3f-repair--section-checkers-walk.md's directory-first repair model). The core's landing: `validate_x3f` scans 256 KiB windows (3-byte overlap — the straddle guard the og's 2-sector windows serve) from the header to the probe limit; found → `SECd + 0x4C` (clamped by the row bound); torn (no SECd in range) → the walked prefix at the row cap with the reduced confidence class (the og's loop-exit salvage shape — the scan stops at its cap with the last offset, never a refusal). The 64 MiB DWG-pattern stand-in and its "recorded follow-up" comment are GONE for X3F. Tests: `Carve.X3fSectionDirectoryWalkCarvesTrueEnd` (a 1 MiB true end on a 100 MiB device — the stand-in carved 64 MiB; RED by construction pre-fix) + `Carve.X3fTornSalvagesTheWalkedPrefix` (the loop-exit salvage at the row cap). Suite 1570/1570. Remaining members: CRW (FUN_180052160, the heap chain) next per the finding's order, then DWG/WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT.

---

## Session-80 addendum (this lane) — the dispatcher decode: the authority dispute settled first-hand (M-155's resolution CONFIRMED; the census's "modern first" prose corrected)

This lane opened at `04fb5e88` with main RED (the ed501caa markers — N-217) and
independently re-read BOTH og sites before touching anything: FUN_180040160
(the Brance::X3FAnalyser chunk processor, vftable-proven at RAWSearchDLL:48700)
with its walk core FUN_1800404a0 (:48851-48910, extent = SECd + 16 + count×12,
clamped, return-1 decline), and FUN_1800535b0 (FileX3FAnalyser :63457-63560,
extent = SECd + 0x4C in all three found-branches, loop-exit walked-prefix
salvage, the id 0x6C stamped unconditionally). An initial hypothesis — that the
registry census's two-generation prose ("the modern analyser first, the legacy
fallback") made the Brance count-walk the primary authority — was CORRECTED by
the dispatcher decode before any push:

**The scan dispatcher (RAWSearchDLL :9850-9876, the magic-key path; the same
shape in the full-registry sweep at :9900-9935) calls the SECONDARY first.**
The map node's registered analyser (`plVar7` = the Brance object) is queried
via `vtbl[+0x40]` (the GetFallback accessor returning the +0x10 secondary —
the FileX3FAnalyser, per the dual registration at :1786-1806); when a
secondary exists, the dispatcher invokes **the secondary's slot-0 method with
the candidate buffer AND the result object** (`(**(code**)*puVar4)(puVar4,
param_2, param_3, ...)` — the result object being the +0x10-size/+0x18-id
shape FUN_1800535b0 writes), and emits on its acceptance; the Brance primary
runs only as a 2-arg second-chance (`vtbl[+0x10](plVar7, param_2)`) that
emits with a null analyser. Since FileX3FAnalyser never declines (FUN_1800535b0
stamps the extent + id unconditionally at :63556-63558), **the Brance
count-walk never contributes to a carved X3F extent** — M-155's resolution
(the S71 FileX3FAnalyser walk kept, the S78 count-walk superseded) is the
corpus-true semantics, now confirmed by the dispatch decode rather than by
precedent alone. The analyser-registry census's "modern first, legacy
fallback" prose is CONTRADICTED by the actual dispatch order — recorded here
as a census erratum (the prose was an inference from the registration
structure, not a decode of the dispatch).

Residual model divergences (documented, deliberate): (a) the core's 256 KiB
windowed search vs the og's sector-stepped 2-sector windows — every found is
the og's in-buffer shape; the og's sector-aligned-tail branch stamps
`uVar7 + 0x4C` where its own arithmetic would give `uVar7 + sector − 4 +
0x4C` (an og quirk the window model never reproduces); (b) the og's walk cap
is `*(FUN_180001350() + 0x70) >> 1` (the singleton's undecoded field — the
halved bound), approximated by the row-max probe limit. X3F's member status
stands as LANDED per the S71/M-155 resolution; the remaining ten members (CRW
next) stay with this finding. N-217 carries the build-repair record.

---

## Session-83 addendum — the X3F member's FOURTH landing (M-156: the ASCII gate's live bug + the dispatcher-mechanism refutation + the chain router)

The S82-close tree still carried two defects on the X3F surface that the
four-lane arbitration left behind (filed + fixed this session as
**M-156**, findings/FIXED/): (1) **the ASCII version gate rejects every
real Sigma X3F file** — real files carry a BINARY u32 version
(0x00020001), the gate demanded an ASCII digit + point, so real RAWs
never carved at all (the og's chain never jointly rejects a readable
FOVb: the modern tier-1 FUN_180040080 accepts exactly the binary shape,
the legacy tier-1 FUN_180053590 accepts any readable header); (2) **the
N-217 dispatcher mechanism does not fire for the X3F node** — the
secondary-first consult requires the node's class to override vtable
+0x40 with the real getter (FUN_180013700), and a full .rdata scan finds
exactly 8 node classes that do; the X3F node (vtable 0x18006cbc0) and
the DMG node (0x18006cd90 — the M-090 precedent) both carry the return-0
STUB at +0x40, byte-exact from the PE. The magic-key dispatch path
(FUN_18000cf60's map route, 0x18000d1e4-0x18000d2c9) provably wires the
MODERN path for binary-version files: the node's own +0x10 binary gate
accepts, the invocation core calls the factory (+0x38 → the 0x120-byte
walker, cap 0x6400000), and the walker registers for the feeding
protocol (+0x20 = FUN_180040160's return-2/return-1 state machine →
FUN_1800404a0's count-walk, extent = SECd + 16 + count*12, decline on
no-directory). The full decode, instruction-address-cited:
`evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md`.

The S83 landing: the version gate is GONE (the union acceptance); the
version shape routes the generation (binary → the modern count-table
walk; anything else → the legacy fixed-tail walk — the four-lane
convergent semantics preserved for the non-binary population; torn → the
legacy walked-prefix salvage); the row bound carries the og walker's own
cap constant (0x6400000 = 100 MiB, raw-asm at 0x1800400ee — the 64 MiB
stand-in value leaves the row). Tests: the five legacy-path pins (all
ASCII plants) stand unchanged; five new pins cover the modern path and
the router — X3fModernCountWalkSetsExtent /
X3fModernGarbageHugeCountClampsToBound /
X3fModernTornFallsBackToLegacySalvage / X3fChainRoutesByVersionShape /
X3fRealSigmaBinaryVersionCarves (the headline RED-at-open pin: the
binary-version plant does not carve under the ASCII gate). The prior
lanes' Ghidra line cites are recorded as unverifiable-from-corpus (the
decompile is CI-generated, not committed) — this addendum's cites are
instruction addresses against the corpus binary itself.

The honest residue (M-156's open questions, named with their decisive
next probe): the engine's +0x20 feed-loop caller was not located; the
registry's container merge (the sweep list's membership) is undecoded —
so for a binary-version file with a directory the og emits EITHER the
count-walk extent OR the legacy constant; the wiring evidence favors the
count-walk (this landing), three convergent prior lanes favor the
constant (the prior). The disagreement is recorded, not papered over.
**X3F's member status: LANDED (the chain, fourth landing); the remaining
ten members (CRW's CIFF heap chain next) stay with this finding.**

---

## Session-83 addendum 2 — the CRW member LANDED (member 2 of 11: the CIFF heap size walk)

**CRW FIXED (this session; the remaining nine members stay with the
finding).** The walk decoded first-hand from the raw asm
(objdump over RAWSearchDLL.dll; evidence:
`evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md`): the CRW
child's vtable 0x18006e008 slot +0x08 = FUN_180052150 → FUN_180052160
(BOTH the walker's table-builder AND the walk — the prior lanes' cite
pointed here). The walk: sector-stepped from the file start with the
walker's OWN cap **0x3200000 = 50 MiB** (raw-asm 0x18005219d);
non-zero-tailed sectors skipped (the streaming chunk-tail gate — the
registered model-divergence class); zero-tailed sectors searched for CIFF
directory entries **{u16 type == 0x2807, u32 length ∈ {0xa8, 0xb2,
0xe4}}** (the three 6-byte signatures 07 28 XX 00 00 00); a hit stamps
**extent = entry_position + TAIL[length]** with the walker's own table
**{0xa8 → 0x46, 0xb2 → 0x3c, 0xe4 → 0x32}** (the map built at
0x1800521dc-0x18005234b); no entry anywhere → the loop-exit stamps the
CAP itself (never a refusal — the same salvage class as X3F's legacy
walk).

The landing: `validate_crw` keeps the M-081 grammar gates (verified
byte-exact against FUN_180052000) and replaces the blind 64 MiB
stand-in with the walk (the windowed entry search with the straddle
overlap; the hit stamps entry + TAIL; torn → the walked bound at reduced
confidence). Both registry rows (II and MM) carry the og walker's OWN
cap constant (0x3200000 — the 64 MiB stand-in value gone). RED→GREEN
first-hand via the stash dance: `CrwDirectoryEntryWalkSetsExtent` and
`CrwWalkCapIsTheOgsOwnConstant` both RED at the stand-in tree (the
stand-in carved 64 MiB / the entry walk did not exist), GREEN with the
walk; the 174-test carve suite green. Tests: CrwDirectoryEntry-
WalkSetsExtent (all three tail constants pinned) /
CrwTornSalvagesTheWalkedBound / CrwWalkCapIsTheOgsOwnConstant. The
S-007 batch's CRW plant (header-only) exercises the torn-salvage path —
its ≥ assertion holds unchanged. Remaining members: DWG (the
section-locator map) next per the fix plan's order, then
WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT.


---

## Session-86 addendum — the S86 lane's DWG landing CONCEDED to the racing S85 window per SA-9; the two riders carried forward (2026-10-29/30)

**The seat conceded.** This lane decoded the DWG walk independently
(FUN_18002f320, the CDWGRawInfo feed, RAWSearchDLL_decompiled.c:
36303-36461 + the raw asm 0x18002f3a7-0x18002f480, objdump over
og-codebase/RAWSearchDLL.dll) and landed it unpushed (ab5bbb24) while
the racing S85 window landed the same member (ddab68b4 + the f7e49eaf
gen-2 struct-offset repair). Per SA-9 the racing seat stands; the
convergent findings on both sides:

- AGREED (both decodes): the three-generation state machine keyed by
  the 6-byte version; gen-1 = the section-locator walk (u32 count at
  file+0x15 — the birth cursor 0x15 IS the row floor the factory wrote
  into the walk-state slot; 9-byte records {u8 id, u32 off@+1, u32
  size@+5} at +0x19; the id-sequence gate; the monotonic max of
  off+size; the torn walked-prefix salvage). The racing decode went
  FURTHER: gen-2 (the rand-LCG struct at +0x80 bytes, the six gates,
  extent = u64@0x34 + 0x6c) landed where this lane recorded it as the
  follow-up — the racing seat's superiority is the concession's own
  justification.
- THIS LANE'S UNIQUE CONTRIBUTIONS (the riders, landed separately this
  session): (1) the ROW FLOOR — the racing landing kept min 64 (the
  S-007 superset); the og's registered floor is 0x15 = 21 (the factory
  FUN_18002ff20's qword write at +0x110 — puVar1[0x22] = 0x15, the same
  write that seeds the walk cursor; the PNG/ONE ctor pattern
  [0x22]=min/[0x23]=max), and the gen-1 walk's residual 512-byte floor
  (inherited from the stand-in era) contradicts it; (2) the
  ID-SEQUENCE GATE — the racing landing compares rec[0] != u8(i)
  (truncated); the og compares the byte against the FULL 32-bit counter
  (cmp %ecx,%eax at 0x18002f40a, eax = movzbl of the id, ecx = the
  running counter) so the walk SELF-LIMITS at 256 records (no byte can
  match counter >= 256) — a count>256 fixture with wrapped ids is
  accepted by the racing port and refused by the og. Both riders:
  RED at the racing tree (DwgRowCarriesTheOgsRegisteredFloor — the
  21-byte walked extent refused by the 512 floor AND the min-64 row;
  DwgWalkSelfLimitsAt256Records — the 260-record wrapped-id fixture
  carved), GREEN with the riders.
- **The tier-1 is the EXACT 32-byte signature** (memcmp 0x20):
  "Paint Shop Pro Image File" + 0x0a + 0x1a + 5 NULs — the core's old
  16-byte prefix accepted files the og refuses (a discovered defect,
  pinned by PspExactSignatureRequired).
- **The walk**: from 0x24, each block is {u32 magic 0x004b427e
  ("~BK\0"), u32 length @ +6}; the position advances 10 + length; the
  extent = the walked END — the first non-~BK block, the 20 MB row
  bound, or the device end all stop the walk there (never a refusal
  once the signature held; the torn straddle salvages the walked end).

RED→GREEN via the stash dance: PspBkBlockChainWalkSetsExtent +
PspExactSignatureRequired both RED at the stand-in validator (the blind
extent; the short-signature acceptance), GREEN with the walk. The
S-007 batch-1 PSP plant re-pinned (the og's exact 32-byte signature +
a real two-block chain; the old 26-byte plant lacked the 0x0a/0x1a).
The carve suite 183/183. Remaining members: WPG, WPD, SIT, APE, EPS,
DXF, SWF-RECT (seven) + the DWG gen-3 remainder.

---

## Session-85 addendum 3 — the WPG member LANDED (member 5 of 11: the zero-tail walk)

**WPG FIXED (this session; six members remain).** The chain decoded
first-hand: the "wpg\0" tier-1 FUN_180014030 (:15660-15683, id 0x45),
the factory FUN_180014090 (the 0x120 WPGRawFileInfo child: min 20, max
100 MB — already in the row), and the child's +0x20 processor
FUN_180014160 (raw asm 0x180014160-0x1800142b0): **extent = chunk
offset + zero_tail(chunk)**, returning 1 (done) after the first walked
chunk. The zero-tail walk is FUN_180042b30 (:50703-50750): a binary
search for the FIRST position whose remainder is all zeros (+1); the
whole buffer zero → 0 (the decline class); no zeros → the buffer
length. The chunk length the driver passes is the DEVICE SECTOR SIZE
(ctx+0xF0, the storage device's sector query at :9192-9195, default
0x200; the +0x20 dispatch at 0x18000d7ae).

The model translation (the registered divergence class, the CRW/X3F
stance): the core stamps the trailing-zero-run position of the hit's
own 512-byte window — the og's semantic (the extent ends at the file's
last non-zero byte within the first sector window) without the
streaming artifacts. **The deferral residue, recorded:** the
processor's gate `[result+0x14] >= 0x10` (the result-object field is
undecoded — the same field gates GZ at >= 8 and the third zero-tail
caller at >= 8) — if it defers the walk to a later sector, the extent
shifts by multiples of 512; the landing takes the first window (the
gate's most coherent reading: the candidate's available bytes in the
feed). A future decode of the result object's +0x14 field settles the
window position; the walk itself and the extent formula are
byte-exact.

RED→GREEN via the stash dance: all three tests RED at the stand-in
(WpgZeroTailWalkSetsExtent / WpgZeroTailFullWindowIsTheBoundary /
WpgAllZeroWindowDeclines — the stand-in's blind 64 MiB and its
all-zero acceptance), GREEN with the walk. The carve suite 186/186.
Remaining: WPD, SIT, APE, EPS, DXF, SWF-RECT (six) + the DWG gen-3
remainder.

---

## Session-84 convergence record (the DWG member's convergent repair — the gen-2 struct offset)

The DWG member was decoded and landed CONVERGENTLY by two lanes in the
same window: the racing S85 lane's landing (8272daf2 — the seat, the
structure: the gen-1 locator walk, the gen-2 rand-XOR struct walk, the
gen-3 recorded remainder, the version-table correction
{12,13,14,15,18,21,24,27,32}) and this lane's S84 decode (evidence:
`evidence/s84-lane-sheets/s84-l1-dwg-walk-decode.md` — the same
three-generation model, the same factory cursor 0x15 at 0x18002ff59, the
same keystream's first byte 41). **The one material delta, arbitrated
by the raw asm: the gen-2 encrypted struct's offset.** The landing read
`hit + 0x20`; the asm at 0x18002f5a0 is `lea rdx,[rdi+0x80]` with rdi =
the chunk base (set once at 0x18002f342, never reloaded — the phantom
`mov edi,0x6c428d44` in naive disassembly is the tail byte of
`lea rcx,[rbp-0x41]` at 0x18002f593, a mid-instruction start). The Ghidra
decompile's `param_2 + 0x20` is INT* arithmetic = **0x80 BYTES** — the
landing ported the unscaled constant. Repaired: the read, both test
plants, and this finding's gen-2 bullet now carry +0x80 with the asm
citation. The offset matches the real R2004 format (the section page
map at 0x80) — the convergent external corroboration.

---

## Session-84 addendum 4 — the WPD member LANDED (member 6 of 11: the pair-table tier-1 + the declared size)

**WPD's walk decoded first-hand from the raw asm** (evidence:
`evidence/s84-lane-sheets/s84-l2-wpd-pair-walk-decode.md`). The
analyser: **FileWPDAnalyser** (the registry key 0x435057ff 'FF WPC', the
node vtable 0x18006cff0 idx 0x44; the +0x10 slot FUN_180041f50 = the
File*Analyser one-function shape: tier-1 + size).

- **The tier-1 pair table**: (u32@+8, u32@+0xE) ∈ {(0x01020a01,
  0x00050200), (0x00021601, 0x00030000), (0x00010f0f, 0x00030000)} —
  the three compare groups at 0x180041f8a/0x180041fa8/0x180041fc7
  (the table at 0x18006ef40); an unknown pair returns 0 — the DECLINE
  (0x180041ff0). The S-007-era 'nonzero product word at +4' gate was
  invented (the og never reads +4 as a gate) — GONE.
- **The size**: extent = u32@+0x14 (0x18004200c: `mov ecx,[r9+0x14]`
  → the result's +0x10) — the declared document size. Never a walk,
  never a refusal once the pair held.
- (The adjacent vtable 0x18006cfa0's 0x180041eb0 walk is
  FileRIFFAnalyser's under the 'RIFF' key — NOT WPD; the attribution
  resolved by the registry leas.)

RED→GREEN first-hand: WpdPairTableDeclaredSizeSetsExtent RED at the
stand-in tree (the 64 MiB device-end extent) → GREEN (the declared
size); WpdUnknownPairDeclines pins the decline. The S-007 batch's
plant re-pinned. The carve suite 288/288. **WPD's member status:
LANDED (members 1-6 of 11; remaining: SIT APE EPS AI DXF SWF-RECT).**
