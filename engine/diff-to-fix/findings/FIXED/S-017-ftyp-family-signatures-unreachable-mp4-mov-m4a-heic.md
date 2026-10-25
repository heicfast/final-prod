# SEVERE S-017 — The ftyp-family signatures are structurally unreachable: MP4/MOV/M4A/HEIC can never be grammar-carved

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** B1 (detection — four advertised formats dead), B2 (recovery correctness — files surface only as wrong-name raw-sign claims), C19 (magic table encoding), A28 (validators unreachable), Q65 (no tests exercise them), P46 (docs claim them)
**Corpus references:** og detects MOV/MP4 via the LIST layer `BruteForce::MovAnalyzer` (registration `RAWSearchDLL_decompiled.c:2748` u16 key 0x0000 — dead by the zero-header guard :9954 — live copy in the list builder `FUN_180004fb0`, :2919-4208) and HEIC via `RawFormat_xjq::HEICAnalyser` (list builder, same function); per-chunk feed to `EUVideoRepair.dll` `GetIMovMp4Repair` (:9210-9236, :9623-9628). Full decode: `diff-to-fix/evidence/decompiled-quotes/s3-rawsearch-inventory.md` §2.5/§4b/§5.
**Core references:** `src/core/carve/cr_registry.cpp:112-131` (FMT_MP4/FMT_MOV/FMT_M4A/FMT_HEIC entries `{0,0,0,0,'f','t','y','p'}`, header_offset=4), `src/core/carve/cr_carve.cpp:83-101` (memcmp of the full 8-byte header at `win+off`, then `cand_abs = base + off − header_offset`), `src/core/carve/cr_analysers.cpp:436-503` (bmff_walk + brand lists), `docs/parity/PARITY-CORE-REC.md:175` ("HEIC ... have og counterparts"), `bench/seed/generate_seed.ps1:51,57,58` — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The four ISO-BMFF signatures encode the ftyp box SIZE field as four literal zero
bytes. `scan_one_window` memcmps the whole 8-byte header, so a real MP4/MOV/M4A/HEIC
head (size = 0x14/0x18/0x20) can never match; only a degenerate size-0 (to-EOF) box
matches, and even then `cand_abs = match − 4` puts the validators' brand read on the
"ftyp" bytes themselves, so every brand check rejects. An executed comparison
against the real core sources proves it: zero grammar hits for four planted
real-world BMFF files. The formats the og carves via MovAnalyzer/HEICAnalyser are
dead code in the core and surface (if at all) as raw-sign `.lrf` mislabels.

## What the reference does

The og has no BMFF u32/u16 key at all (a size-prefixed magic cannot be keyed on the
first 4 bytes). MOV/MP4 are carved by `BruteForce::MovAnalyzer` living in the LIST
layer (insertion at `FUN_180004fb0`; also registered under u16 key 0x0000 which the
dispatch's zero-header guard `if (*(int*)param_2 == 0) return 0;` —
RAWSearchDLL_decompiled.c:9954 — makes permanently dead), a buffer-scanning
BruteForce analyser fed `(header, ctx, sector_size)`, sized through the pending-list
continuation model (`FUN_18000d670` → analyser vtable slot 4 `ContinueScan`,
:10114-10116). HEIC is carved by `RawFormat_xjq::HEICAnalyser` (same list layer,
RawConfig.ini-gated, `og-codebase/config/RawConfig.ini [Filter] HEIC=1`). Carved
MOV/MP4 streams are additionally offered to the video-repair engine
(EUVideoRepair.dll, `GetIMovMp4Repair` :9220, per-chunk feed :9625-9627, finalize
submit :10130-10136).

## What our core does

`cr_registry.cpp:112-131` registers FMT_MP4/FMT_MOV/FMT_M4A/FMT_HEIC with header
`{0,0,0,0,'f','t','y','p'}` (len 8, header_offset 4). `cr_carve.cpp:85-94`:
first-byte dispatch on `header[0] == 0` then `memcmp(win+off, header, 8)` — the four
zero bytes are compared literally. `cr_carve.cpp:94` then computes
`cand_abs = base + off − sig.header_offset`. `cr_analysers.cpp:436-448` reads the
brand at `hit+8` and rejects unless it is in the per-format list
(`isom/mp42/mp41/iso2/avc1/dash`, `qt  `, `heic/heix/mif1/msf1`, `M4A /M4B /M4P`).
Executed probe (`diff-to-fix/evidence/executions/s3-ftyp-signature-deadness/`,
sources compiled unmodified at `34b3441`): a 6 MiB slice planted with (a) a valid
JPEG control, (b) real MP4/MOV/HEIC/M4A heads, (c) a degenerate size-0 ftyp box →
`hits=5 validated=1 rejected=4`; the JPEG carved (`fmt=0x0`, 521 B); **none of the
four BMFF files produced a single grammar hit**; the size-0 box produced 4 hits (one
per ftyp-family signature) and all 4 validators rejected; all five BMFF positions
were claimed by the raw-sign layer as `File00000N.lrf` (format 0x36 = "lrf",
51,200-byte cap, conf 1000 — the og's own raw_signs.json contains that "ftyp" sign,
so this part is og-parity, but the og's structural analysers claim the position
first).

## Gap

Falsifiable claim: a disk containing only `clip.mp4` (ftyp isom, size 0x18),
`movie.mov` (ftyp qt, size 0x14), `photo.heic` (ftyp heic, size 0x18) and
`song.m4a` (ftyp M4A, size 0x20) in unallocated space yields correctly-typed,
correctly-sized carve results in the og (MovAnalyzer/HEICAnalyser + video-repair
feed) and **zero** grammar-layer results in the core (only 50 KB `.lrf` raw-sign
mislabels). Proven by execution.

## Impact

- Four top-tier user formats (the video formats users most often lose) silently
  missing despite validators existing and the module doc claiming them
  (`docs/parity/PARITY-CORE-REC.md:175`, module row C19 "BMFF brand walks").
- Bench recall for mp4/mov/heic (`bench/seed/generate_seed.ps1` plants exactly these
  heads) is silently depressed; results show up as wrong-format `.lrf` entries.
- The F8 load-time registry gate (`validate_registry`) passes the broken entries —
  no self-check catches unreachable signatures.

## Reproduction / verification

Executed (strength 1): see `diff-to-fix/evidence/executions/s3-ftyp-signature-deadness/`
(probe2.cpp + build command + output + README analysis). Static corroboration:
`cr_carve.cpp:89` (memcmp over header_len), `cr_carve.cpp:94` (−header_offset shift),
`cr_analysers.cpp:440` (brand read at hit+8). Zero BMFF tests:
`grep -c "FMT_MOV\|FMT_MP4\|FMT_HEIC\|FMT_M4A" tests/unit/test_carve.cpp` → 0.

## Fix plan (for the engineering team)

1. **Re-key the BMFF family on the "ftyp" bytes** — the magic must be
   `{'f','t','y','p'}` (len 4) with `header_offset = −4`, or (simpler, no negative
   offsets) keep a 4-byte "ftyp" magic and make `scan_one_window` treat
   `header_offset` as "bytes between candidate start and magic start" with a signed
   shift: candidate `cand_abs = base + off + sig.header_offset` where the table sets
   `header_offset = (u32)(-4)`. Concretely: add a `magic_off` field (i8) to
   `Signature`; for MP4/MOV/M4A/HEIC set header=`"ftyp"`, `magic_off = −4`; compare
   at `win + off − magic_off`; candidate = `base + off − magic_off`; keep the F8
   rule that `header_len + magic_off ≥ 0`.
2. Update `validate_registry` (F8) to reject entries whose header bytes are all-zero
   in their first four positions AND whose validator reads a brand at a fixed
   positive offset (the structural smell that let this ship), or more directly: add
   a load-time self-probe — run each signature's magic against a synthetic
   well-formed header for that format (one fixture per format in a table-driven
   self-test) so unreachable signatures fail at startup.
3. Add a MOV/MP4 no-ftyp salvage path to mirror the og: a list-layer-equivalent
   entry (e.g. FMT_MOV_NAKED) keyed on `moov`/`mdat`/`wide`/`free` box types at
   aligned offsets (the og's MovAnalyzer semantics), tried after the ftyp entries.
4. Tests (fail before, pass after): `Carve_Mp4_Real_Ftyp_Isom`,
   `Carve_Mov_Real_Ftyp_Qt`, `Carve_Heic_Real_Ftyp_Heic`, `Carve_M4A_Real_Ftyp`,
   `Carve_Mov_Moov_Without_Ftyp` (og MovAnalyzer parity), and
   `Registry_SelfProbe_Reaches_Every_Format` (F8 startup self-test).
5. Docs: correct `docs/parity/PARITY-CORE-REC.md:175` and the carve module rows
   (B1/B2/C19) once fixed; note the raw-sign `.lrf`-on-ftyp quirk is og-parity.

## Tests to add

(fix plan step 4 — all fail before, pass after)

## Related findings

- S-007 (missing formats — same user impact class; this one is worse because the
  code exists and passes F8).
- N-008 (ZIP semantics — unrelated but same validator family).
- M-005 (bitmap gating — the og only ever BMFF-scans free sectors).

## Open questions

The og's MovAnalyzer accept criteria (moov/mdat box grammar depth) are not fully
decoded — the pending-continuation model (:10114) and the EUVideoRepair feed
(:9625) are established; per-box grammar decoding is a follow-up if the naked-MOV
salvage path (fix plan step 3) is built.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the S-017 fix on main)
**Verified by:** fail-before/pass-after — with the pre-S-017 table restored and the new tests in place: Carve.Mp4RealFtypIsom, Carve.MovRealFtypQt, Carve.HeicRealFtypHeic, Carve.M4aRealFtyp, Carve.MovMoovWithoutFtyp FAILED 5/5 (zero BMFF grammar hits for real heads - the executed deadness the finding proved, now pinned); Carve.RegistrySelfProbeReachesEveryFormat FAILED with the broken table (the strengthened F8 self-probe catches the class). Post-fix: all six PASS; full suite 320/320 Linux ASan.
**Resolution notes:** The four BMFF entries are re-keyed on the "ftyp" BOX TYPE with header_offset 4 (magic {'f','t','y','p'}, len 4) - the box SIZE at candidate+0 is data (0x14/0x18/0x20 for real files) and must not be in the magic; the candidate now lands on the size field and the validators read the brand at candidate+8 where real files carry it. The window-matching primitive is extracted into match_signature (cr_signatures.h) and shared by the scan dispatch and the NEW F8 self-probe (self_probe_registry) so a magic change cannot desynchronize scan and gate; the probe asserts (a) the magic is reachable through the scan's own primitive with the candidate shift landing exactly at the synthetic file start (the candidate-4 mis-shift bug class), and (b) the structural rule from the finding's fix plan: a magic whose first four bytes are all zero asserts a 32-bit data field is constant (the pre-S-017 smell - the probe rejects the class; the synthetic-buffer match alone was proven WEAK during fail-before verification: it faithfully reproduces a broken magic and passes, so the structural rule is what catches the class). Fix-plan step 3 implemented conservatively: FMT_MOV_NAKED (appended id, serialized values stable) keys on a top-level "moov" box (header_offset 4, after the size field) and validates a coherent box chain (moov size sane + type + a following concrete-size box), accepting at raw-sign-class confidence 620 - the og's exact MovAnalyzer accept criteria remain undecoded (pending-continuation model, the finding's Open questions) and the conservative accept is the documented follow-up surface; mdat/wide/free were NOT keyed (too generic - "free" especially). Trial order: FMT_MOV_NAKED runs LAST so ftyp-keyed entries claim intact positions first. The .lrf raw-sign claim over ftyp positions is og-parity (unchanged). docs/parity/PARITY-CORE-REC.md:175 HEIC-row now true rather than aspirational. Win7-inclusive gate unaffected (no new imports).
