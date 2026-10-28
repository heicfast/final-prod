# MAJOR M-081 — CRW misattribution: the Canon CIFF keys (II/MM `\x1A\x00`) landed as "Minolta MRW" rows whose PRD gate rejects genuine Canon CRW content

**Severity:** MAJOR
**Module:** carve
**Parameters affected:** B1 (detection correctness — genuine CRW rejected), C19 (magic bytes — key attribution), A28 (validator logic — the PRD anchor is the wrong grammar for the key), Q65 (the test fixture is synthetic and self-fulfilling)
**Corpus references:** `diff-to-fix/evidence/decompiled-quotes/s3-rawsearch-inventory.md:222-223` — the census re-read: `| 0x001a4949 | II\x1a\0 | Canon CRW (CIFF) | Brance::CRWAnalyser (ctor FUN_18003c9f0) | MISSING (N-026) |` and `| 0x001a4d4d | MM\x1a\0 | Canon CRW BE | Brance::CRWAnalyser | MISSING |`; `diff-to-fix/SCAN-ENGINE-DELTA.md:168-169` ("CRW = II/MM`\x1A\x00` (id 0x12) — the MRW key is `\0MRM` (id 0x66) only"); the Brance-family roster `re-workspace/artifacts/SearchDLL/rawsearchdll--family-brance.md` (CRWAnalyser is the family's Canon CIFF member)
**Core references:** audited `35adcb9`. `src/core/carve/cr_registry.cpp:709-738`: `FMT_MRW` (`{'I','I',0x1A,0x00}`, "Minolta MRW"), `FMT_MRW_MM` (`{'M','M',0x1A,0x00}`, "Minolta MRW (BE)"), `FMT_MRW_MRM` (`{0x00,'M','R','M'}`, "Minolta MRW (MRM)") — all three routed to `validate_mrw`; `src/core/carve/cr_analysers.cpp:3057-3088`: the II/MM forms gate on a **`PRD` block at the u32 offset@+4** (:3069-3074, off < 8 or > 4 MiB → reject; `rd(...,prd,3)` memcmp "PRD" → reject). Enum comments `include/cr/carve/cr_signatures.h:147-148`. Test fixture `tests/unit/test_carve.cpp:2141-2147` = synthetic `II1A` + PRD plant.
**Status:** RESOLVED (SCAN-4, `9e5dedc`) — every fix-plan step executed; moved MAJOR -> FIXED

## Summary

The S-007 census re-read established that the `II\x1A\x00` / `MM\x1A\x00` keys belong to **Canon CRW (CIFF)** — the only true Minolta MRW key is `\0MRM`. The batch-1 landing nevertheless shipped those two keys as `FMT_MRW`/`FMT_MRW_MM` rows ("Minolta MRW") with a validator that anchors II/MM-form detection on a `PRD` block — a Minolta-grammar gate on Canon-grammar keys. Consequence: genuine Canon CRW files (which begin `II\x1A\x00\x00\x00` + `HEAPCCDR`, with no PRD block and a first-u32 that decodes far past the 4 MiB bound) are **rejected** — the format is simultaneously mislabeled and unrecallable.

## What the reference does

The og's Brance family `CRWAnalyser` (ctor FUN_18003c9f0) owns the `II\x1A\x00` (0x001a4949) and `MM\x1A\x00` (0x001a4d4d) keys — Canon CRW/CIFF (the census rows, verified at the registration sites by the s3 leg). Canon CIFF files: `II`/`MM` + `\x1A\x00` header, `HEAPCCDR` image block structure. The `\0MRM` key (0x66) is the separate Minolta MRW format. Both are carved by the og.

## What our core does

All three keys route to `validate_mrw` (cr_registry.cpp:709-738). The II/MM forms are gated on a PRD anchor at the u32 offset read from +4 (cr_analysers.cpp:3069-3074). A genuine Canon CRW: bytes +4..+7 of `II\x1A\x00\x00\x00` + `HEAPCCDR` decode as a value ≫ 4 MiB (or, on the alternate alignment, no PRD exists) → the validator returns not-ok. The shipped test plants a synthetic `II1A`+PRD construct, so the suite validates the misreading against itself — no real-world CRW or MRW corpus is exercised.

## Gap

Genuine Canon CRW content in unallocated space is not carved (the PRD gate rejects it), and the two rows carry the wrong format attribution ("Minolta MRW" for Canon CIFF keys). Falsifiable: plant a real Canon CRW (any camera sample, `II\x1A\x00\x00\x00HEAPCCDR…`) — the og carves it; the core rejects it at :3070.

## Impact

Recall loss for the Canon CRW format (CIFF — the PowerShot/Early-EOS raw format, millions of files in the wild) plus a classification defect: results that DO pass the synthetic gate would be labeled MRW. The mislabel also poisons downstream report semantics (extension `.crw` rows would appear as `.mrw`).

## Reproduction / verification

- Corpus-derived: the census rows (s3-rawsearch-inventory.md:222-223, quoted) + SCAN-ENGINE-DELTA:168-169 — both first-hand re-reads of the registration sites.
- Core first-hand: the three registry rows read (cr_registry.cpp:709-738); `validate_mrw` read in full (:3057-3088 — the PRD gate and bounds verified); the synthetic test fixture read (test_carve.cpp:2141-2147).
- Logic-verified rejection: genuine CIFF header bytes at +4 fail either the bounds check (:3070, off ≫ 4 MiB) or the PRD memcmp (:3074).

## Fix plan (for the engineering team)

1. Re-attribute: rename `FMT_MRW`→`FMT_CRW` (II) and `FMT_MRW_MM`→`FMT_CRW_MM` (or keep enum values, fix labels/extensions to `crw`) — keep `FMT_MRW_MRM` (`\0MRM`) as the only MRW row. Update `cr_signatures.h:147-148` comments, the registry display names, and the extension fields.
2. Write a CIFF validator for the II/MM keys: verify the `HEAPCCDR` block structure (the `II\x1A\x00` + u16 header-length + `HEAPCCDR` + heap layout); keep the MRW validator (PRD anchor — correct grammar for `\0MRM`-family content) on the MRM row only.
3. Real-world fixtures: add a genuine Canon CRW sample class to the test corpus (or a byte-accurate synthetic reconstruction of the CIFF heap), and a genuine `\0MRM` MRW sample.
4. Reconcile the trail: the S-007 batch-1 addendum's "MRW(x3)" landing list and the stale SCAN-ENGINE-DELTA A4 phrasing get correction notes (the census re-read was right; the landing misread it).
5. Serialize note: the og's own registry shares id 0x33 across PNG/WPG — the core's append-only `carve_format` enum is unaffected by the rename (core-sequential ids stand).

## Tests to add

- `Carve.CanonCrwCiffAccepted` (genuine CIFF header → carved, labeled CRW).
- `Carve.CrwNotMislabeledMrw` (the row's format name + extension assertions).
- `Carve.MinoltaMrmStillAccepted` (the `\0MRM` row unchanged).
- `Carve.PrDSyntheticPlantNowInsufficient` (the old synthetic fixture alone must not pass the CRW row).

## Related findings

- S-041 (this session — the batch-2 remainder; CRW's recall was counted there, this finding owns the mislabel + validator fix).
- S-007 (FIXED — the batch-1 landing whose addendum this corrects).
- N-026 (the pre-batch census row that first named CRW MISSING).

## Open questions

The og CRWAnalyser's own CIFF grammar depth (block-walk vs magic+bounds) — read `FUN_18003c9f0`'s body at the decompile when implementing step 2 to match its accept/reject criteria exactly.

## Resolution record (SCAN-4, `9e5dedc` — every fix-plan step executed)

1. **Re-attributed:** `FMT_MRW`(II)→`FMT_CRW`, `FMT_MRW_MM`(MM)→`FMT_CRW_MM` (ext "crw"); `FMT_MRW_MRM` renamed `FMT_MRW` — the only MRW row. FormatId VALUES untouched (append-only).
2. **The og CIFF validator, first-hand** (the open question above resolved): `FUN_180052000` :62333-62353 — bytes@+4 == {00 00 'H' 'E'} (u32 0x45480000), u16@+8 == "AP", record-type u32@+0xa ∈ {CCDR, TIFP, ARCH}, version byte@+0xe == 2 → `validate_crw`. The CIFF heap size-walk (`FUN_180052160`) is the recorded follow-up; the DWG-pattern bounded extent stands in.
3. **MRW:** `validate_mrw` = the og size formula (`FUN_180012100` :14233-14258): `BE32(u32@+4) + 8 + ceil(u8@0x20 × BE16@+0x1a × BE16@+0x18 / 8)` — the u64 product replaces the og's wrapping signed multiply (identical for every non-UB input; the D-68 precedent).
4. **The twin fixed in the same commit:** DBA5/94A6 → `FMT_LOWDOC`/`FMT_LOWDOC2` (FileLowDocAnalyser id 0x17, ext "doc", the og formula-only validator `FUN_18001b650` :21167-21188); HLP = 3F 5F 03 00 only with the og size gate (`FUN_180041c50` :49883-49895).
5. **Trail reconciled:** the batch-1 addendum + the SCAN-ENGINE-DELTA session-5 log carry the correction notes; the retired II1A+PRD synthetic is asserted NOT carved by the new `S007M081CiwMrmLowdocHlpOgGrammar` test.
6. **Verification:** suite 1023/1023 green (gcc linux-release Werror incl. the win-stub parse gate); clang-format 18.1.8 clean. The 2 MiB CIFF ext-surface correction (mrw→crw ×2, hlp→doc ×2) is logged as the API-lane handoff in worklog.


## SA-9 convergence addendum (2026-10-28 — the third lane's instruction-decisive corroboration)

The SA-9 lane (audited `35adcb9`, pre-fix) independently decoded the same
misattribution from the registration sites: II/MM 1A 00 (0x1a4949/0x1a4d4d,
ghidra :1746-1762) bind `FUN_18003c9f0` = **Brance::CRWAnalyser id 0x12**
wrapping FileCRWAnalyser (:46100-46121), with the CIFF gate
`\x00\x00HEAP` + {CCDR, TIFP, ARCH} + length-byte 2 (FUN_18003cb10
:46148-46185); `FileMRWAnalyser` id 0x66 rides `\0MRM` only (:2500-2511).
The lane's dead-row analysis (real CRW files rejected at the PRD gate's
4 MiB offset bound — bytes +4..7 = `00 00 48 45` decode far past it) and
the full registration quotes are preserved at
`evidence/decompiled-quotes/s9-crw-mrw-registrations.md`. The lane also
corrected an adjacent key attribution for future work: MMOR/IIRS/FUJI
(0x524f4d4d/0x53524949/0x494a5546, :1727-1744) bind
`BruteForce::TiffAnalyzer` id 0x40 — NOT the CRW chain (the product's
ORF/RAF rows already cover those keys).
