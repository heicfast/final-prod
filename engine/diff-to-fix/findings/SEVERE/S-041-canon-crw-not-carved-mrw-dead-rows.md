# SEVERE S-041 — Canon CRW is not carved at all; the II/MM 1A 00 keys are occupied by two dead MRW rows

**Severity:** SEVERE
**Module / Surface:** carve (the RAW-signature layer — CRW/MRW keys)
**Parameters affected:** B1 (detection correctness), A28 (validator logic), C19 (magic tables), P29 (format identity/ids)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :1746-1762 (the II/MM 1A 00 registrations), :46100-46121 (FUN_18003c9f0 = the Brance::CRWAnalyser/FileCRWAnalyser ctor, id 0x12), :46148-46185 (FUN_18003cb10 = the CIFF gate), :2500-2511 (the FileMRWAnalyser registration — `\0MRM` only, id 0x66), :62290-62300 (the validator's expected-pattern table); census erratum at `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:56` (see CORPUS-ERRATA E53)
**Core references:** `src/core/carve/cr_registry.cpp:709-738` (FMT_MRW / FMT_MRW_MM / FMT_MRW_MRM rows), `src/core/carve/cr_analysers.cpp:3057-3088` (validate_mrw) — audited commit `35adcb9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered (the rows landed in the S-007 batch-1 fix `e018bf4` as "MRW x3 rows (II1A/MM1A with the PRD-block anchor + the \0MRM marker, idx 102)"; the SCAN-ENGINE-DELTA's A4 row already carries the corrected attribution — "CRW (II\x1A\x00/MM\x1A\x00 id 0x12 — NOT MRW) + MRW (\0MRM id 0x66 only)" — but no finding owns the product-side defect)

## Summary

In the og, the u32 keys `II\x1A\x00` (0x1A4949) and `MM\x1A\x00` (0x1A4D4D)
are registered to the **Canon CRW** analyser chain (Brance::CRWAnalyser,
format id 0x12, CIFF grammar gate), and the Minolta MRW analyser
(FileMRWAnalyser, id 0x66) is registered on **`\0MRM` only**. Our product
implemented the opposite: it put MRW rows on II/MM 1A 00 with a
"PRD-block anchor" gate. Real Canon CRW files (`II\x1A\x00\x00\x00HEAPCCDR…`)
hit those rows, fail the PRD gate, and are silently not carved; and since
real MRW files begin `\x00MRM` (served correctly by the FMT_MRW_MRM row),
the two II/MM 1A 00 rows can never accept any real file — they are dead rows
occupying the og's CRW key space. Net effect: Canon CRW — a mainstream
camera RAW format — is completely absent from our carve results.

## What the reference does

Registration (ghidra :1746-1762): both `0x1a4949` and `0x1a4d4d` are bound
(via the u32 registry insert `FUN_180006c80`) to the analyser constructed by
`FUN_18003c9f0`, which builds **Brance::CRWAnalyser (id 0x12)** wrapping a
0x418-byte **FileCRWAnalyser (id 0x12)** fallback (:46100-46121). The
validation gate `FUN_18003cb10` (:46148-46185) checks, after the key: u32@+4
== 0x48450000 (bytes `00 00 'H' 'E'`), u16@+8 == 0x5041 (`'AP'`), u32@+0xA
in {`"CCDR"`, `"TIFP"`, `"ARCH"`}, u8@+0xE == 0x02 — i.e. the CIFF heap
structure `II\x1A\x00\x00\x00HEAPCCDR…` (plus the TIFP/ARCH heap variants),
with block-length byte 2 — and stamps format id 0x12. The validator's
expected-pattern table (`FUN_180052000`, :62290+) carries the II/MM 1A +
"HEAP" + type-block constants for all combinations.

FileMRWAnalyser (id 0x66 = 102) is registered **only** on
`0x4d524d00` (`\x00MRM`) at :2500-2511; the full-decompile grep shows no
other binding (vftable references :2504/:14222 = ctor/dtor pair only).

All quotes verbatim in
`diff-to-fix/evidence/decompiled-quotes/s9-crw-mrw-registrations.md`.

## What our implementation does

`cr_registry.cpp:709-728` registers:
- `FMT_MRW` on `{'I','I',0x1A,0x00}` (min 512, max 64 MiB, conf 650);
- `FMT_MRW_MM` on `{'M','M',0x1A,0x00}` (same);
both gated by `validate_mrw`'s II/MM branch (`cr_analysers.cpp:3068-3080`):
it reads a u32 offset at +4, requires 8 ≤ offset ≤ 4 MiB, and requires the
3 bytes at `hit+offset` to be `"PRD"` — a Minolta MRW block structure.
`FMT_MRW_MRM` on `{0x00,'M','R','M'}` (:729-738) handles real MRW files via
the MRM branch (marker + nonzero tail, bounded extent).

There is no CRW/CIFF analyser anywhere in the carve layer (registry grep:
no `HEAP`/`CCDR`/`FMT_CRW`; the format inventory lists 101 FMT_* ids without
CRW).

Consequences, both mechanically verified:
1. A real Canon CRW file begins `II\x1A\x00\x00\x00HEAPCCDR` — it hits the
   FMT_MRW row, `validate_mrw` reads offset u32@+4 = 0x00000000 < 8 and
   rejects. **No CRW file is ever carved.**
2. A real Minolta MRW file begins `\x00MRM` — it never matches the II/MM
   1A 00 keys. **The FMT_MRW and FMT_MRW_MM rows can never accept any real
   file** (dead rows).

## Gap

Falsiable claim: the og's acceptance set for the keys {0x1A4949, 0x1A4D4D}
is {CIFF files: HEAP + CCDR/TIFP/ARCH + length byte 2}, while ours is
{files whose +4 u32 is a 8..4 MiB offset to a "PRD" block} — sets that are
disjoint on real-world data (CRW headers carry `\x00\x00HEAP` at +4; the
PRD anchor is an MRW-only structure that appears behind `\x00MRM`, not
behind II/MM 1A 00). Therefore: Canon CRW recall = 0% (og: carves CRW),
and MRW-over-II/MM recall = 0% for both engines (only the \0MRM row works,
matching the og's actual key).

## Impact

- **User-observable correctness:** Canon CRW (`.crw`, PowerShot era, still
  present on old cards and backups) is silently missing from every RAW
  carve. This is exactly the class S-007 was filed for (missing common
  formats) — but worse, because the key space is *occupied* by rows that
  look implemented, so no census count will flag the absence.
- **Format identity:** the two dead rows also mislabel any hypothetical
  future hit as "Minolta MRW".
- **Audit-trail truth:** the S-007 batch-1 commit's "MRW x3 rows (II1A/
  MM1A …)" claim encodes the census erratum; the trail's SCAN-ENGINE-DELTA
  A4 row already corrects it — this finding aligns the code with the
  corrected decode.

## Reproduction / verification

1. Corpus: the registration quotes and ctor/gate decodes above, re-read
   first-hand this session (evidence file
   `evidence/decompiled-quotes/s9-crw-mrw-registrations.md`).
2. Product: `cr_registry.cpp:709-738` + `cr_analysers.cpp:3057-3088` read
   directly; registry inventory (see
   `evidence/executions/s9-carve-registry-inventory.md`) shows no CRW row.
3. Byte-level proof: Canon CRW = `49 49 1A 00 00 00 48 45 41 50 43 43 44 52`
   ("II\x1A\x00\x00\x00HEAPCCDR") — bytes +4..7 = `00 00 48 45` → the u32
   offset read is 0x45480000 = 1,163,046,912, far beyond the 4 MiB bound →
   rejected at `cr_analysers.cpp:3070` before the PRD read even runs.

## Fix plan (for the engineering team)

1. No architectural change — re-key + new validator.
2. Files to modify: `src/core/carve/cr_registry.cpp` (rows), `src/core/carve/cr_analysers.cpp` (validator), `tests/unit/test_carve.cpp` (plants/tests), the carve-format enum header where FMT_* ids live (append-only: add FMT_CRW; do not renumber).
3. The exact change:
   - Replace the FMT_MRW / FMT_MRW_MM rows on II/MM 1A 00 with TWO FMT_CRW
     rows (same keys), pointing at a new `validate_crw` implementing the og's
     gate: key re-check; u32@+4 bytes `00 00 48 45` (`\x00\x00HE`); u16@+8
     `AP`; u32@+0xA ∈ {"CCDR","TIFP","ARCH"}; u8@+0xE == 0x02; accept.
     Extension "crw"; format label "Canon CRW (CIFF)"; bounds: min 20 B
     (a CIFF header is at least 26 bytes; keep the og's FileInfo floor if
     the ctor decode pins one — the census quotes 20 B..500 MB class
     bounds; if the ctor read is desired first, mirror the M-068-style
     probe and record the follow-up).
   - Keep FMT_MRW_MRM as the sole MRW row (it matches the og's actual key).
   - Remove the II/MM branch from `validate_mrw` (dead code after re-keying).
4. Tests: CRW plants for both keys and all three heap types (CCDR image
   data, TIFP, ARCH); a real-layout plant `II\x1A\x00\x00\x00HEAPCCDR` + a
   minimal CIFF block; hostile variants (wrong heap type, length byte != 2)
   rejected; MRW \0MRM still carves; a regression guard asserting the II/MM
   rows' format id is CRW (not MRW).
5. Contract/doc updates: FORMATS.md gains CRW; the S-007 FIXED addendum
   gains the correction note; CORPUS-ERRATA E53 stands (the census row).
6. Acceptance: a fixture image containing a canonical CRW plant carves as
   FMT_CRW with the CIFF-gated extent; zero FMT_MR* emissions from II/MM
   keys; full suite green.

## Tests to add

- `S041CanonCrwCarvesBothKeys` — II and MM key plants with HEAPCCDR carve
  as FMT_CRW.
- `S041CanonCrwHeapVariants` — HEAPTIFP / HEAPARCH accepted.
- `S041CanonCrwRejectsNoise` — `II\x1A\x00` + garbage, wrong heap type,
  length byte != 2 → rejected.
- `S041MrwMrmOnlyRowSurvives` — `\0MRM` file still carves; II/MM 1A 00
  with MRW-style PRD data (the old dead shape) is NOT labeled MRW anymore.
- Registry identity test: the II/MM 1A 00 rows report the CRW format id.

## Related findings

- S-042 (batch-2 formats absent) — CRW is also part of the absent-formats
  set; this finding owns the key-occupation mechanism, S-042 owns the rest.
- S-007 (FIXED) — the landing that introduced the dead rows.
- N-089 (FIXED) — the same census-vs-decode correction pattern (MRW/CRW
  mis-attribution class).

## Open questions

None for the defect itself. One decode follow-up for the fixing session,
non-blocking: the CRW FileInfo ctor's exact min/max pair (the census-class
bounds vs a first-hand ctor read, same discipline as WPG's M-068 verification).
