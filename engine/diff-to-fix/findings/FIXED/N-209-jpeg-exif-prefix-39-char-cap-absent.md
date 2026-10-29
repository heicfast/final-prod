# MINOR N-209 — The JPEG carve-name prefix path accepts EXIF Model/Software strings up to 256 chars (renumbered from its born-collided N-190 seat at the S57 lane's CI-red repair: the remote lane's hot-window-merge-red-build-gate finding holds N-190 per earlier-push-wins) where the og's strncpy caps at 39 (0x27)

**Severity:** MINOR
**Module:** carve (the S-049 content-derived naming — the JPEG EXIF prefix)
**Renumber:** N-190 → N-209 (2026-10-29, the S57 lane: the hot-window lane's merge-red-build-gate finding holds the N-190 seat per SA-9; the doc_truth_gate's ID-uniqueness check caught the double claim on CI)
**Parameters affected:** B10 (naming divergence), A28 (the prefix walk's bounds — the og's own quirk)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :40382-40510 (read first-hand this session): the EXIF Software tag (0x131) branch — `uVar30 = 0x27; strncpy((char *)&local_4b0, (char *)pdVar33, 0x27);` — the tag value is copied with a 39-char cap (0x27) before feeding the name composition (the Model 0x110 branch carries the same cap shape)
**Core references:** audited `34d6cf10` — `src/core/carve/cr_analysers.cpp` jpeg_name_prefix :555-681 (read first-hand: the EXIF walk rejects counts `> 256` and stores `model`/`software` whole; `prefix_from_bytes(model)` at :660-663 consumes the untruncated string; the counter key at :656-658 likewise)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The S-094/S-049 JPEG naming path reads EXIF Model (0x0110) / Software
(0x0131) tag values and uses them whole as the carved file's name prefix
(up to 256 chars, the walk's own count bound). The og strncpy's the tag
value into a 39-byte buffer (`strncpy(dst, src, 0x27)`), so its prefix is
capped at 39 chars. On tags longer than 39 chars the two engines emit
different names for the same file (and different per-string counter keys).

## What the reference does

At RAWSearchDLL:40382-40510, the Software-tag branch zeroes a 0x27-byte
stack block and `strncpy`s the tag pointer into it with the 0x27 cap — the
prefix source is hard-truncated at 39 chars regardless of the tag's
declared count.

## What our core does

`jpeg_name_prefix` (cr_analysers.cpp:555-681) validates the IFD entry
(typ == 2, 0 < cnt <= 256), copies the value, and uses the whole string:
`model`/`software` feed `prefix_from_bytes` and `*counter_key` untruncated
(:660-663, :656-658). No 39-char cap exists on this path (the only 39-char
bound in the file is a different analyser's office walk).

## Gap

Falsifiable: a JPEG whose EXIF Software tag is e.g. 60 chars yields a
60-char-derived name prefix in the core and a 39-char-truncated one in the
og.

## Impact

Naming-only divergence on the >39-char-tag population (the trail's own
189k-row fixture census observed a max of 36 chars — real-world tags are
camera/device strings well under the cap, so the firing population is
thin); the name also keys the per-string carve counters, so two cameras
sharing a >39-char prefix collide identically in both engines anyway.
MINOR.

## Reproduction / verification

Decompiled-evidence class, both sides read first-hand this session; the
sweep record `evidence/executions/s50-landing-parity-sweep.md` row 3's
residual-edge note (the S-049 verdict itself PARITY — this cap is the one
unimplemented og edge).

## Fix plan (for the engineering team)

1. Architectural: none.
2. Files: `src/core/carve/cr_analysers.cpp` (jpeg_name_prefix).
3. Algorithm: truncate `model`/`software` to 39 chars (0x27) at capture,
   before `prefix_from_bytes` and the counter key (one line each; note the
   og's strncpy does NOT NUL-terminate at exactly 39 — the 39th char is
   kept, cap = 39).
4. Tests: `Carve.JpegExifPrefixCappedAtThirtyNine` — a fixture with a
   60-char Software tag: expect the 39-char prefix. RED before.
5. Contract/doc: the S-049 Resolution note in modules/carve.md gains the
   cap line.
6. Acceptance: the new test green; the existing OgNaming* tests green
   (their fixtures are all < 39 chars).

## Tests to add

`Carve.JpegExifPrefixCappedAtThirtyNine` (RED before).

## Related findings

S-049 (FIXED — the naming contract this path implements).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** fd3b9af3 (the S57 lane's landing, originally filed as N-190; the ledger row carries the SHA)
**Verified by:** CarveEngine.JpegExifPrefixCappedAtThirtyNine (RED pre-fix, proven by the stash-revert rebuild: the whole 45-char tag named the file "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghi_000001.JPG"; post-fix the 39-char cap "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abc_000001.JPG") + the OgNamingJpgExifModelAndSoftware family green (all its fixtures < 39 chars). Full suite 1454/1454.
**Resolution notes:** Corpus re-verified first-hand: both strncpy(dst, src, 0x27) sites read live (RAWSearchDLL:40388-40389 the Software 0x131 branch, :40483-40484 the Model 0x110 twin). The cap lands at capture - the tag string is resized to 39 AFTER the NUL/space trim and BEFORE both the prefix_from_bytes consumption and the per-string counter key assignment, so the name and the counter key both ride the truncated value exactly as the og's strncpy feeds its composition. The 39th character is kept (resize, not truncate-to-38-plus-NUL) matching strncpy's semantics with a zeroed stack buffer.
