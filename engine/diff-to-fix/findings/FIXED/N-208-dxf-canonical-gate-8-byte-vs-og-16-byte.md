# MINOR N-208 — The DXF canonical acceptance gate (renumbered from its born-collided N-189 seat at the S57 lane's CI-red repair: the remote lane's registry-sentinel-walk finding [c481b812, 01:02Z] holds N-189 per earlier-push-wins) is 8 bytes where the og requires the full 16-byte "  0\r\nSECTION\r\n  " pattern

**Severity:** MINOR
**Module:** carve (DXF analyser — the canonical branch; N-152's 999 branch is parity)
**Renumber:** N-189 → N-208 (2026-10-29, the S57 lane: the hot-window lane filed and retired a registry-sentinel-walk finding under N-189 at 01:02Z, 55 minutes before this lane's DXF retirement landed at 01:57Z — their seat wins per SA-9; the doc_truth_gate's ID-uniqueness check caught the double claim on CI)
**Parameters affected:** B1 (false-positive rate — the looser gate accepts non-DXF byte streams), B19 (the acceptance pattern constants), A28 (carving validator accept/reject criteria)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` FUN_1800376f0 :42258-42275 (read first-hand this session): the canonical check is the full 16-byte two-qword compare — `local_50 = 0xd302020; uStack_4c = 0x4345530a; local_48 = 0x4e4f4954; uStack_44 = 0x20200a0d; ... if ((*param_2 == 0x4345530a0d302020) && (param_2[1] == 0x20200a0d4e4f4954))` — i.e. "  0\r\nSECTION\r\n  " (the 16-byte CRLF group-code covenant), beside the 999-comment entry (:42272-42275) the N-152 landing ported exactly
**Core references:** audited `34d6cf10` — `src/core/carve/cr_analysers.cpp` validate_dxf :5596-5612 (the canonical branch `bool ok = std::memcmp(h, "  0\r\nSECTION", 8) == 0;` at :5605 — an 8-byte prefix check; the 16-byte form is not re-checked after)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The DXF analyser's canonical (non-999) acceptance path checks only the
8-byte prefix "  0\r\nSECTION" where the og compares the full 16-byte
pattern "  0\r\nSECTION\r\n  " (two consecutive 8-byte qword compares). A
byte stream matching the 8-byte prefix but not the continuation (e.g.
"  0\r\nSECTION" followed by anything but "\r\n  " — a truncated header at
a window edge, or a same-prefix different-file stream) is accepted by the
core and refused by the og.

## What the reference does

The og builds the full 16-byte pattern on the stack and compares both
qwords: `*param_2 == 0x4345530a0d302020` ("  0\r\nSECT") AND
`param_2[1] == 0x20200a0d4e4f4954` ("ION\r\n  ") — the pattern
"  0\r\nSECTION\r\n  " (the ASCII group-code covenant followed by the
start of the next group code). Only the 999-comment leg (:42272+, the
14-byte LF / 16-byte CRLF forms) enters differently — that leg is the
N-152 landing, verified parity this session.

## What our core does

`validate_dxf` reads 8 bytes and memcmps "  0\r\nSECTION" (:5605) — the
first qword of the og's pattern only. The 16th-byte continuation is never
re-checked on the canonical path.

## Gap

Falsifiable: a stream starting "  0\r\nSECTIONX..." (or the 8-byte prefix
at the end of a readable window) carves as DXF in the core and is refused
by the og's canonical branch.

## Impact

A narrow false-positive class on the DXF key (extra DXF-stamped rows from
prefix-colliding streams); the real-DXF population is unaffected (real
files carry the full pattern). Low frequency, cosmetic-to-results quality
impact — MINOR.

## Reproduction / verification

Decompiled-evidence class, both sides read first-hand this session:
`evidence/decompiled-quotes/s50-landing-divergences.md` §3's sibling
records — the og quote reproduced in
`evidence/executions/s50-landing-parity-sweep.md` (row 18's footnote, the
canonical-gate secondary finding) and re-read at RAWSearchDLL:42258-42275
by the lead; the core at cr_analysers.cpp:5596-5612.

## Fix plan (for the engineering team)

1. Architectural: none.
2. Files: `src/core/carve/cr_analysers.cpp` (validate_dxf).
3. Algorithm: read 16 bytes on the canonical path and memcmp the full
   "  0\r\nSECTION\r\n  " (mirror the og's two-qword form); keep the
   999-comment leg as landed (N-152).
4. Tests: `Carve.DxfCanonicalRequiresFullSixteenBytePattern` — a fixture
   "  0\r\nSECTIONX" (16 bytes, wrong continuation) refused; the real
   16-byte form accepted. RED before.
5. Contract/doc: none beyond the test names.
6. Acceptance: the new test green; the existing DXF tests (incl. the N-152
   family) green.

## Tests to add

`Carve.DxfCanonicalRequiresFullSixteenBytePattern` (RED before).

## Related findings

N-152 (FIXED — the 999 leg, parity-verified this session), M-093 (the
registry-row lineage).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** 92858a74 (the S57 lane's landing, originally filed as N-189; the ledger row carries the SHA)
**Verified by:** Carve.DxfCanonicalRequiresFullSixteenBytePattern (RED pre-fix: the "  0\r\nSECTIONXWRONG" prefix collision carved; post-fix refused while the real 16-byte form still carves with the canonical FMT_DXF stamp) + the S-007 batch family re-verified green after its fixture was re-truthed. Full suite 1453/1453.
**Resolution notes:** Corpus re-verified first-hand: RAWSearchDLL_decompiled.c:42258-42275 read live - the canonical branch is the two-qword compare (*p == 0x4345530a0d302020 && p[1] == 0x20200a0d4e4f4954), the full "  0\r\nSECTION\r\n  " 16-byte covenant, with the 999-comment leg entering separately (the N-152 parity, untouched). The fix is the finding's own plan: validate_dxf's canonical gate memcmps the full 16 bytes (a header truncated under 16 readable bytes falls to the 999 leg exactly as the og's short-buffer probe does). One fixture correction recorded: plant_dxf in the S-007 batch test planted only the 12-byte prefix + counter filler - a synthetic shape the og's own gate refuses; it now plants the real covenant (the test's intent, "DXF must carve", is served by a real-DXF-shaped fixture).
