# MAJOR M-106 — ONE and 3DS size-walks are decoded but still standing in: the school-1 landing shipped the DWG-pattern 64 MiB bounded extent where the og walks a chunk chain (ONE, max 500,000,000) and a section walk (3DS, max 200,000,000) — large OneNote/3DS files carve truncated

**Severity:** MAJOR
**Module / Surface:** carve (modules/carve.md) — the S-041 school-1 follow-ups
**Parameters affected:** A25 (engine algorithm steps — the size walk), A27 (fragment/extent reconstruction — carved extent end), A47-adjacent (the 64 MiB cap vs the og's own bounds), C-constants-adjacent (the per-format max)
**Corpus references:** `diff-to-fix/evidence/decompiled-quotes/s8-batch2-formats.md:29` (ONE, decoded: "cluster/chunk walker :52316-52358: per-chunk GUID re-check, chunk size u64@+0xC4 (fallback u64@+0x64), 0<size≤2×device; ONERawFileInfo max 500,000,000 (:17203)") and `:31` (3DS, decoded: "chunk walk :50921-71 (stride u16@+2, 0x58 shortcut when u16@(chunk+0x58)==0x4D4D), cap device×5, max 200,000,000 (:14559)") — both rows cite the RAWSearchDLL decompile sites first-hand; `include`-side ground truth re-verified at the core's own validator comments (cr_analysers.cpp:3767-3773, :3857-3864) which name the og functions (`FUN_180044e30` the ONE walk, `FUN_180042ff0` the 3DS walk) and the og ctor bounds (`FUN_180016080 :17191-17209`: FileInfo {20, 500,000,000}; `FUN_1800126c0 :14491-14510` / S-041's census bound 200 MB).
**Core references:** audited commit `ea1ac86`-lineage (the school-1 landing `b92ebef` + the M-084/M-091/M-090 carve fixes that shifted the file; cites re-verified at the reconciliation HEAD). `src/core/carve/cr_analysers.cpp:3846-3870` (`validate_one`: the stand-in comment `:3849-3850` "The chunk-chain size walk (FUN_180044e30) is the recorded follow-up; the DWG-pattern bounded extent stands in"; `r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap))` at `:3862`) and `:3936-3970` (`validate_3ds`: the stand-in comment `:3939-3940` "The section size-walk (FUN_180042ff0) is the recorded follow-up; the DWG-pattern bounded extent stands in. Row max = S-041's census bound (200 MB)"; both branches cap at `64 << 20` — `:3948` and `:3957`). The landing commit's own message records the follow-ups: "ONE/3DS carry the DWG-pattern bounded extent for their undecoded walks (recorded follow-ups)" (b92ebef).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The S-041 school-1 landing (b92ebef) shipped correct key/gate grammars for CHM, ONE, MNY, EMF, SWF and 3DS — but for ONE (.one OneNote) and 3DS (.3ds Autodesk) the og's size-determining walk was already DECODED in the trail's own evidence file, and the landing still shipped the generic DWG-pattern 64 MiB bounded extent as the size stand-in. The og walks ONE's chunk chain (per-chunk GUID re-check, size u64@+0xC4, fallback u64@+0x64, 0<size≤2×device, max 500,000,000) and 3DS's section chain (stride u16@+2, the 0x58 shortcut, cap device×5, max 200,000,000). Consequence: a OneNote file larger than 64 MiB (the og's own bound is ~477 MiB) or a 3DS file between 64 MiB and 200 MB carves TRUNCATED — the carved output is cut at the stand-in cap, the same truncation class as M-090 (DMG) and M-091 (MDB).

## What the reference does

**ONE (RawSpeed::OneAnalyser + FileONEAnalyser fallback, id 0x31):** after the GUID gate (u64@+4 == 0x7853B1AE4DA7D88C, u32@+0xC == 0xD39629D0 — landed correctly in core), the og determines the file size by WALKING the chunk chain (FUN_180044e30, decoded at s8-batch2-formats.md:29): each chunk re-checks the GUID, the chunk size is u64@+0xC4 (fallback u64@+0x64), each chunk must satisfy 0 < size ≤ 2×device size, and the walk accumulates until the chain breaks; the FileInfo bounds {20, 500,000,000} are the analyser ctor's own.

**3DS (RawSpeed::New3DSAnalyser + File3DSAnalyser, id 1):** after the MM/== gate (or the NCSD alternate branch — both landed correctly in core), the og walks the section chain (FUN_180042ff0, decoded at s8-batch2-formats.md:31): stride u16@+2 per chunk, a 0x58 shortcut when u16@(chunk+0x58)==0x4D4D, cap device×5, max 200,000,000.

## What our core does

Both validators gate correctly and then cap at 64 MiB: `validate_one` (cr_analysers.cpp:3774-3789) — "r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap))" with the comment explicitly recording the stand-in (`:3772-3773`); `validate_3ds` (:3868-3887) — both the MM and NCSD branches cap at `64 << 20` with the same recorded-follow-up comment (`:3862-3863`). The registry row for 3DS carries S-041's census bound 200 MB as the row max (the cap the walk would respect), but the validator never reaches beyond 64 MiB.

## Gap

Falsifiable: a planted 100 MiB .one fixture (GUID header + a chunk chain whose declared sizes sum past 64 MiB within the og's 500,000,000 bound) carves at exactly 64 MiB in core; the og carves it at the chain's true end. Same for a 100 MiB .3ds fixture (MM + == gate + section chain) against the og's 200 MB/section-walk semantics. The decodes for both walks already exist in the trail's evidence file — no further RE work is a precondition for the fix.

## Impact

User-observable truncation on large OneNote notebooks and 3DS scenes: the carved file is cut mid-chunk, producing corrupt output the recovery UI reports as success (the carve result's size is the stand-in cap, not the og's walk result). OneNote notebooks exceeding 64 MiB are common in real deployments (section files grow with embedded media); 3DS scenes between 64 MiB and 200 MB are rarer but the same class. This is the M-090/M-091 truncation family on the two school-1 formats whose walks happened to be decodable.

## Reproduction / verification

Decompiled evidence (strength 2): both walks decoded with decompile-site citations in `evidence/decompiled-quotes/s8-batch2-formats.md:29,:31`; the core-side stand-ins verified first-hand at HEAD (`cr_analysers.cpp:3767-3789`, `:3857-3906`, comments quoted). Not executed: a plant-and-carve demo belongs to the fix's tests (the school-1 test file already plants the six formats byte-exact — extend `S007NicheBatch2School1Carve` with over-cap fixtures).

## Fix plan (for the engineering team)

1. **ONE:** implement the chunk-chain walk per the decode — from the hit, iterate chunks: re-check the GUID at each chunk head, size = u64@+0xC4 (fallback u64@+0x64), validate 0 < size ≤ 2×device, advance; end = the chain's break point, clamped to the FileInfo bound 500,000,000. Replaces the `64 << 20` stand-in at cr_analysers.cpp:3783.
2. **3DS:** implement the section walk per the decode — stride u16@+2, the 0x58 shortcut when u16@(chunk+0x58)==0x4D4D, cap min(device×5, 200,000,000). Replaces both `64 << 20` stand-ins in `validate_3ds`.
3. **Tests:** extend the school-1 plant suite — `plant_one_large` (chunk chain summing past 64 MiB within 500 MB) and `plant_3ds_large` (section chain past 64 MiB within 200 MB): both must carve at the walk's end (fail-before: they carve at 64 MiB today); plus hostile variants (chunk size 0, chunk size > 2×device, broken GUID mid-chain → salvage at the last valid chunk, matching the og's walk-break semantics).
4. **Registration note:** the 3DS row's max stays the census bound (200 MB); the ONE row's max stays 500,000,000 — both already correct in the registry; only the validator changes.
5. **Acceptance:** the two new plants carve og-exact; the existing 8-plant school-1 test stays green (the under-64-MiB fixtures are unaffected — the walk and the stand-in agree below the cap); suite green on gcc linux-release Werror + clang-format clean.

## Tests to add

(Embedded in fix-plan step 3: `plant_one_large` / `plant_3ds_large` + the hostile variants — fail-before/pass-after on the two over-cap fixtures.)

## Related findings

- **S-041 (FIXED-as-umbrella — retired to history this session; see FIXED-LEDGER)** — the parent whose school-1 landing recorded these two follow-ups; this finding is their owner.
- **M-107** — the undecoded-walk stand-in residue (the same pattern where the walk is NOT yet decoded).
- **M-090 / M-091** — the same truncation class (DMG footer walk / MDB page-chain walk).
- **S-007 (FIXED)** — the DWG-pattern bounded-extent precedent (the sanctioned honest cap, conf ≤ 650).

## Open questions

None — both walks are decoded; the fix is mechanical against the decode table.
