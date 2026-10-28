# MAJOR M-112 — the thumbnail StbioSource read callback issued a single 4 MiB-clamped read: every PNG whose IDAT exceeds 4 MiB (any photo-scale PNG) failed with outofdata — a latent decode-path defect discovered during M-109's band test

**Severity:** MAJOR (a whole format class silently undecodable at photo scale — the M-080 headline class, latent since the thumbnail engine's landing)

**Module / Surface:** preview (the thumbnail engine's stb adapter — src/core/preview/cr_thumbnail.cpp StbioSource)

**Parameters affected:** A25-adjacent (the decode pipeline's IO adapter), 65-adjacent (format coverage in practice), the M-080/M-109 band in fact

**Corpus references:** n/a (a core-side implementation defect, not an og divergence — the og's GDI+ path streams its input; the corpus's PNG behavior is the ordinary decode-everything-up-to-200 MB posture M-109 pins)

**Core references:** the defect (pre-fix): `src/core/preview/cr_thumbnail.cpp` StbioSource::read_cb — the callback issued ONE `pr.read_at(pos, data, want)` per stb request, but `PreviewReader::read_at` clamps every call to `kMaxRead` (4 MiB, cr_preview.h:54-57); `stbi__getn` requires the FULL count in one callback return (`count == n - blen`, stb_image.h:1666-1679) and stb's PNG loader accumulates the whole IDAT with a single `stbi__getn(s, z->idata+ioff, c.length)` (stb_image.h:5194) — so any PNG whose IDAT chunk exceeds 4 MiB returned outofdata (DecodeFailed). Every existing PNG fixture was < 4 MiB, so no prior test caught it (the suite's PNG coverage: 64x48 fixtures only).

**Status:** FIXED (this session — discovered during M-109's LargeStillImageWithinOgBand test, filed per the §8 duty)

**Prior audit check:** NEW-FINDING — no prior finding covers it (the M-080/M-085/S-045 family covers budgets/formats, not the adapter); discovered first-hand when M-109's in-band test (a real 34 MiB PNG) failed with `outofdata (eof=1 over=0)` while the same bytes decoded through raw stb (`stbi_load` from file) — the isolation pointed at the callback adapter, and the single-shot read + the 4 MiB clamp is the mechanism.

## Summary

The thumbnail engine's stb adapter satisfied each read callback with a single `read_at` call — clamped by `PreviewReader::kMaxRead` to 4 MiB — while stb's PNG loader requests the entire IDAT chunk in one `getn` and treats a short return as corrupt data. The consequence: PNGs whose IDAT exceeds 4 MiB (every photo-scale PNG — a 12 MP photo saved as PNG is 15-25 MiB) never produced a thumbnail, failing with DecodeFailed instead. JPEG was unaffected in practice (stb reads JPEG in segment-sized pieces), which is why the M-080-era fixtures (a 4.07 MiB JPEG) passed while the PNG class was broken. No test covered a > 4 MiB IDAT, so the defect was latent through every prior green suite.

## What the reference does

The og's GDI+ thumbnail path decodes still images up to the 200 MB eligibility gate (M-109's anchor) — a photo-scale PNG thumbnails normally.

## What our core does (pre-fix)

`StbioSource::read_cb`: one `read_at` per callback; `read_at` clamps to 4 MiB; stb's `getn` for the IDAT demands the full chunk length; the short return → `outofdata` → DecodeFailed. Verified first-hand: the 34 MiB in-band fixture failed with `outofdata (eof=1 over=0)` while `stbi_load` on the same file succeeded; the mechanism read directly in both code paths.

## Gap

Falsifiable (executed): a valid PNG with a > 4 MiB IDAT through `make_thumbnail` with default caps returned DecodeFailed pre-fix; Ok post-fix. The fail-before proof is the M-109 band test itself (RED pre-fix — first with BudgetExceeded at the 32 MiB default, then with outofdata after the cap raise isolated the adapter defect; green after the loop fix).

## Impact

Every photo-scale PNG was silently unthumbnailable (a named DecodeFailed, but wrong); the format class is common (lossless editing, screenshots at scale, design exports). The defect also masked M-109's band test from ever passing — the two findings' proofs are entangled by design (the band test is the regression proof for both).

## Reproduction / verification

Executed first-hand this session: (1) the 34 MiB real-PNG fixture through the engine → `outofdata`; (2) the same bytes through raw stb → Ok (the isolation); (3) the single-shot read + kMaxRead clamp read in source (both sites cited); (4) post-fix: the loop assembles the full request across the 4 MiB-clamped reads — the test green.

## Fix plan (executed)

The read_cb loop: assemble the request across `read_at` iterations (each budget-spent via `spend_read`) until the request is satisfied or a true EOF (a 0-byte return). The budget clamp (`want < read_left`) and the EOF disclosure semantics are unchanged.

## Tests

- `Thumbnail.LargeStillImageWithinOgBand` (the M-109 band test) is the regression proof: its 34 MiB PNG requires 9 looped reads to satisfy stb's single IDAT getn — RED pre-fix (outofdata), green post-fix. No separate test is needed (the fixture IS the > 4 MiB-IDAT class), and the full thumbnail suite (30 tests) re-verifies the adapter's small-read paths.

## Related findings

**M-109** (the band fix whose test exposed this — the proofs are entangled) · M-080 (the budget family; its JPEG fixture could never catch this) · M-085 (the TIFF decoder rides the same adapter — its large-file paths benefit identically)

## Open questions

None. (The 4 MiB kMaxRead clamp itself is the correct per-call device hygiene; the adapter, not the clamp, was the defect.)

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's fix commit — the M-109+M-112 landing, session 39B)
**Verified by:** Thumbnail.LargeStillImageWithinOgBand — the fail-before executed first-hand in three stages: (1) at the 32 MiB default the 34 MiB PNG refused BudgetExceeded (M-109's RED); (2) with the cap raised to the og's 200 MB the decode failed `outofdata (eof=1 over=0)` — the adapter defect isolated (raw `stbi_load` on the same bytes: Ok); (3) with the read loop, Ok + the 256x235 thumbnail. The full thumbnail suite 30/30 and the full ctest 1119/1119 green post-fix.
**Resolution notes:** The read_cb loop landed in StbioSource: the request assembles across read_at iterations (each spend_read-budgeted) until satisfied or a true 0-byte EOF; the budget clamp and the EOF-disclosure semantics unchanged. The discovery classification: NOT an og divergence (the og's GDI+ streams its input — the corpus-side posture is the ordinary decode-to-200 MB M-109 pins) but a core implementation defect the audit missed because no fixture exceeded the 4 MiB clamp; filed per the §8 duty with the executed isolation evidence. The entanglement with M-109 is by design: the band test's 34 MiB PNG is exactly the > 4 MiB-IDAT class, so one fixture proves both findings.
