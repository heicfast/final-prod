# SEVERE S-054 — the S-047 landing's TS/M2TS cap is 10× the og's: core carves MPEG-TS/AVCHD up to 2 GB where the og clamps at exactly 200 MB (the "×10" provenance is refuted — 2e9 appears nowhere in the og binary)

**Severity:** SEVERE (wrong recovery results vs the reference on a first-class format family — oversized carved files, 10× the og's bound; introduced by a fix landing, unpinned by any test)
**Module:** carve (the registry rows for the MPEG-TS family)
**Parameters affected:** C20 (on-disk struct offsets/sizes — the size bound), A27 (fragment/extent handling — the extent cap), P47-adjacent (the oversized emission wastes I/O and memory on false tails)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:35507-35514` — the TS/M2TS size walk's clamp, verbatim: `if (200000000 < uVar7) { uVar7 = 200000000; }` … `*(ulonglong *)(… + 0x10) = uVar7;` and the else-branch `*(ulonglong *)(…) = 200000000;` (the fallback assigns 200 MB directly); `:9049` — the controller global the walk clamps against: `*(ulonglong *)(puVar3 + 0x70) = 200000000`; whole-file grep for `2000000000` / `0x77359400` → **0 hits** (verified twice: the S11-lead Lane A walk and the lead's first-hand re-grep at filing)
**Core references:** audited `461825d8`-lineage (HEAD at filing) — `src/core/carve/cr_registry.cpp:1015-1041`: the FMT_TS row's max_size `2000000000ull` with the comment "cap = the 200 MB controller global x 10" (`:1016`, `:1027`) and FMT_M2TS's `2000000000ull` (`:1038`); the values landed in `a3abfbeb` (fix(S-047), 2026-10-28 17:07Z) and no test pins the cap (`grep -rn "2000000000" tests/unit/` → 0; the only TS test is `MpegTs188PacketStreamAccepted`, `tests/unit/test_carve.cpp:3615`)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane A infrastructure walk (the TS-family cap re-check flagged by the walk's verdict table row 15); NOT covered by S-047 (retired — its scope was the sync scanners' presence, and its landing introduced this value), NOT by I-031 (the parallel infra register — "no ACUTE/SEVERE/MAJOR residue in the infrastructure" — the cap is a registry-row value, not infrastructure), NOT by N-093 (the batch-1 bound pairs: MID/GZ/PSP/SIT — a different family set).

## Summary

The S-047 fix landed FMT_TS/FMT_M2TS with a 2,000,000,000-byte max size, justified in-code as "the og's controller global (200 MB) x 10". First-hand decode of the og's TS/M2TS size walk shows the og clamps at **exactly 200,000,000 bytes** — both the walk's computed size and its fallback path — against the controller's 200 MB global. The ×10 multiplier has no basis in the binary: the value 2,000,000,000 (0x77359400) appears nowhere in the decompiled engine. Core therefore emits carved TS/M2TS candidates up to 2 GB where the reference stops at 200 MB — a 10× divergence in recovery output on the MPEG-TS/AVCHD family, invisible to the test suite (nothing pins the cap).

## What the reference does

The og's TS-family analyser computes a candidate size and clamps it to the controller
global: at `RAWSearchDLL_decompiled.c:35507-35509`, `if (200000000 < uVar7) {
uVar7 = 200000000; }` followed by storing the clamped value as the FileInfo size
(`:35510`); when the walk cannot compute a size at all, the else-branch assigns the
literal 200,000,000 (`:35513`). The global it clamps against is initialized at
`:9049` (`*(ulonglong *)(puVar3 + 0x70) = 200000000` — the config manager's +0x70
field). No TS-family code path in the binary multiplies this by 10, and the literal
2e9/0x77359400 does not occur anywhere in the 64k-line decompile (grep verified by
two independent reads this session). The og's maximum MPEG-TS/AVCHD carve size is
therefore 200 MB, hard.

## What our core does

`cr_registry.cpp:1021-1041` registers FMT_TS and FMT_M2TS with `max_size =
2000000000ull` on both rows, annotated "the og's controller global (200 MB) x 10"
(`:1016` prose, `:1027` inline). The validators (`validate_ts`/`validate_m2ts`) walk
188-byte packets and the row's max_size bounds the emitted extent — so a contiguous
or fragmented TS stream with ≥0x5000 bytes of sync-valid packets emits a carved
candidate as large as 2 GB. The value landed in the S-047 fix commit `a3abfbeb` and
has never been pinned: `grep -rn "2000000000" tests/unit/` returns nothing, and the
only TS-family test (`MpegTs188PacketStreamAccepted`, `test_carve.cpp:3615`) asserts
acceptance, not the cap.

## Gap

Falsifiable on both sides: (a) corpus side — `grep -c "2000000000\|0x77359400"
RAWSearchDLL_decompiled.c` → 0, and `sed -n '35507,35514p'` shows the 200,000,000
clamp + fallback; (b) core side — a ≥200 MB sync-valid TS fixture (or a
`cr_ui_carve_format_info`/registry readout) shows the row's cap at 2,000,000,000.
Net behavior: for any TS/M2TS stream whose true extent is in the (200 MB, 2 GB]
band, the og emits a 200 MB-clamped file and core emits the full (or up-to-2 GB)
extent — the recovered file sizes differ from the reference by up to 10×.

## Impact

Recovery-result divergence on a mainstream video-capture format (AVCHD/.m2ts from
camcorders, .ts broadcast captures): every recovered file in the affected size band
has a different size than the reference product produces, and the oversized reads
multiply I/O and memory cost on large disks (a disk full of 1 GB .ts captures
yields 10× the emission volume the og would produce). Users comparing outputs
between the products see different results; downstream sort-by-size and
quota-style behaviors diverge. Not silent data loss (the extra bytes are real stream
bytes), hence SEVERE rather than ACUTE — but it is a wrong-output divergence
introduced by a fix and entirely unpinned.

## Reproduction / verification

Corpus-derived, first-hand (the lead re-read the decompile at filing; the Lane A
agent independently produced the same three cites):
- `sed -n '35507,35514p' RAWSearchDLL_decompiled.c` → the 200,000,000 clamp and
  fallback (quoted above).
- `sed -n '9049p'` → the controller global = 200000000.
- `grep -c "2000000000\|0x77359400" RAWSearchDLL_decompiled.c` → 0.
- Core: `sed -n '1015,1041p' src/core/carve/cr_registry.cpp` → both rows at
  `2000000000ull`; `git show a3abfbeb` → the value's landing; `grep -rn 2000000000
  tests/unit/` → 0 pins.
Full record: `evidence/s11-lead-deep-lanes.md` §Lane-A (the infrastructure verdict
table row 15 and Section 6 item 2).

## Fix plan (for the engineering team)

1. Change both rows' max_size from `2000000000ull` to `200000000ull`
   (`cr_registry.cpp:1027`, `:1038`) and replace the two "× 10" comments (`:1016`,
   the inline `:1027`) with the true provenance: "the og's TS walk clamps to the
   200 MB controller global (FUN at :35507-35514; the global at :9049) — no ×10
   exists in the binary".
2. Add the pinning test the landing lacked: a registry-shape test asserting
   FMT_TS/FMT_M2TS max_size == 200000000 (the M-072/N-093 cap-pin pattern), plus a
   behavior test that a sync-valid synthetic TS stream longer than 200 MB (or a
   truncated-at-cap variant using the existing fixture generator) emits at exactly
   the 200 MB cap.
3. Sweep the registry for any other row citing the same "× 10"/controller-global
   rationale (grep "x 10" in cr_registry.cpp — `:712`'s "max 10,000,000" is a
   different, already-verified bound; confirm no other row inherits the refuted
   multiplier).
4. Contract/doc updates: none required (the cap is not documented externally); note
   the repair in RELEASE-NOTES if the release train already shipped S-047.
5. Acceptance: the two rows read 200000000; the new pin test passes; `grep -rn
   "2000000000" src/core/carve/` returns nothing.

## Tests to add

- `Carve.TsM2TSCapIsTheOg200MB` (registry-shape pin, both rows).
- `Carve.TsStreamClampedAt200MB` (behavior: a >200 MB sync-valid stream emits at
  exactly 200,000,000 bytes — fails before, passes after).

## Related findings

S-047 (retired — the landing that introduced the value; this finding is its
residual, the same class as M-109 is to M-080); N-093 (the bound-pair family whose
pins this fix should mirror); I-031 (the parallel infra register whose controller-
global decode corroborates the 200 MB value).

## Open questions

None — the clamp, the fallback, the global, and the absence of 2e9 in the binary
are all directly cited.
## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-054+M-116 landing; the ledger row carries the final SHA)
**Verified by:** Carve.TsM2TsCapIsTheOg200MB (the registry-shape pin, both rows == 200000000, min 0x5000) + Carve.TsStreamClampedAt200MB (the behavior pin: a 220 MB contiguous sync-valid stream emits NO node above 200,000,000 bytes, the cap-sized node within one 188-byte packet of the clamp; pre-fix the whole stream emitted as one ~219,999,852-byte node). Full suite 1030/1030 + cr_ui 59/59 (Linux release). The corpus cites re-verified first-hand at the fix: sed -n '35507,35514p' shows the 200000000 clamp + fallback; :9049 the +0x70 global; grep 2000000000|0x77359400 -> 0 hits.
**Resolution notes:** Both rows' max_size are the og's 200 MB controller global exactly (cr_registry.cpp FMT_TS/FMT_M2TS: 2000000000ull -> 200000000ull); the two "x 10" comments replaced with the true provenance (the walk clamp at :35507-35509, the fallback literal at :35513, the global at :9049, and the absence of 2e9 in the binary). The "x 10" sweep found no other row inheriting the refuted multiplier (the :712 "max 10,000,000" MID row is the og's own verified bound). The behavior test's construction exposed M-116 (filed + fixed in the same landing, announced per section 8 of the session mandate): the trial loop ran the cap-sized walk for every mid-stream sync before the covered check discarded it - the 220 MB fixture did not complete inside 600 s pre-M-116; the pre-validator covered-check reorder (semantics preserved exactly; the post-emission jump variant was tried and REJECTED - it broke S007CommonFormatsCarve's AAC ADTS start-inside-another-extent, the og ranks overlapping candidates rather than skipping them) brings the identical fixture to 11.3 s. The test disables the non-TS registry rows via the M-024 host list so the pin exercises the TS row's cap without paying the whole-registry per-byte dispatch (the disable list is built from the registry itself - no drift). Safety: no contract surface changed (the cap was never documented externally); RELEASE-NOTES gains the S-047-residual note.
