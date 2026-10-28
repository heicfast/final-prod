# SEVERE S-042 — WMF placeable validator grammar contradicts the og: disjoint accept-sets (checksum word, XOR range, gates, size rule)

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** A28 (carving validator logic), C19 (magic bytes/signatures), B1 (detection correctness), B3 (false-positive rate)
**Corpus references:** `RAWSearchDLL_decompiled.c` `FUN_1800264d0` :29807-29845 (the WMF validator; placeable branch :29826-29837) — verbatim quote and gate table in `evidence/decompiled-quotes/s8-wmf-placeable-grammar.md`
**Core references:** audited `ff879f9` — `src/core/carve/cr_analysers.cpp:3383-3411` (`validate_wmf_placeable`), `tests/unit/test_carve.cpp:2257-2274` (`plant_wmf_placeable`); landed by S-007 batch-1 `e018bf4` whose commit message claims "WMF placeable … XOR-checksum VERIFIED - the stored word is the XOR of words 0..8" citing the og
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's placeable-WMF validator and the og's read different bytes:
the og compares the stored word at bytes +20-21 (word 10) against the
XOR of the TEN preceding words (0..9), requires a zero dword at bytes
16-19, requires word 19 (bytes 38-39) == 0, and sizes the hit as
std-Size-words×2 + 22. The core compares the stored word at +18-19
against XOR(words 0..8), gates an "inch" field at bytes 16-17 (the og
requires those bytes to be zero), and sizes by probe. No byte string
can satisfy both grammars.

## What the reference does

`FUN_1800264d0` placeable branch (first-hand read this session,
verbatim in the evidence file): key `0x9AC6CDD7` @+0; hmf word[2]==0;
`param_2[10] == XOR(param_2[0]..param_2[9])` — checksum stored at
bytes 20-21, XOR of the ten words before it; dword at words 8-9
(bytes 16-19) == 0; dword at byte +22 == 0x00900001 (the standard WMF
header there); word 19 (bytes 38-39) == 0; size = `dword@+28 × 2 +
0x16` — the standard header's Size field ×2 plus the 22-byte placeable
section; format code 0x46 (shared with the standard branch).

## What our core does

`validate_wmf_placeable` (cr_analysers.cpp:3383-3411): key @+0; `inch =
u16@h+16` must be 1..14400 (the og requires bytes 16-19 == 0 — a file
passing the og fails here with inch==0); checksum stored `u16@h+18`
must equal XOR(words 0..8) (the og stores at +20 and XORs words 0..9);
standard header at +22 (same); no hmf/reserved/tail gates; extent =
`min(hit + 64 MiB, probe_limit)` (the og computes the exact
std-size×2+22). The test `plant_wmf_placeable` plants the core's own
convention, so the suite is self-consistent but not og-consistent.

## Gap

Falsifiable: for every byte string S, `og_accepts(S) != core_accepts(S)`
whenever S is a placeable candidate — the og's reserved-dword-zero gate
forces the core's inch field to 0 (core rejects), and the core's
inch∈[1,14400] gate forces the og's reserved dword nonzero (og rejects).
The checksum comparisons read different offsets with different XOR
ranges, so the two accept-sets are disjoint. Additionally the core's
extent rule (64 MiB cap / probe) diverges from the og's exact
size formula on every accepted hit.

## Impact

Carve correctness for the WMF-placeable row: the core reports as
placeable-WMF a set of files the og would never report, and misses
every file the og would report (subject to S-041's note that the og's
own grammar is also non-canonical vs the 20-byte Aldus layout — but
parity is scored against the og). The batch-1 landing's evidence claim
about the og's checksum convention is factually wrong, so the audit
trail's "verified" note misleads future work. User-visible: wrong
carve results (false positives + false negatives) on this format row.

## Reproduction / verification

Decompiled evidence (first-hand, this session):
`evidence/decompiled-quotes/s8-wmf-placeable-grammar.md` — the og
function quoted verbatim with the gate-by-gate comparison table.
Core code read directly (cr_analysers.cpp:3383-3411, test_carve.cpp:
2257-2274). Cross-checked: the standard-WMF branch of both engines
agrees (key 0x90001, params-word zero gate, size=words×2, code 0x46) —
only the placeable branch diverges.

## Fix plan (for the engineering team)

1. Re-derive `validate_wmf_placeable` from the og grammar: add hmf
   gate (u16@+4==0), replace the inch gate with the reserved-dword
   gate (u32@+16==0), move the checksum to u16@+20 == XOR(words 0..9),
   add the tail gate (u16@+38==0), and compute extent as
   `u32@+28 (std Size words) × 2 + 22`.
2. Files: `src/core/carve/cr_analysers.cpp` (validator), the registry
   row's note (cr_registry.cpp), `tests/unit/test_carve.cpp`
   (`plant_wmf_placeable` must plant the og convention).
3. Add a shared-key/noise test: a canonical 20-byte-Aldus placeable
   file (checksum@18, standard@+20) must NOT carve as placeable under
   either engine's grammar — pin that the core matches the og, not the
   spec, and record the non-canonical-og note in CORPUS-ERRATA.
4. Correct the S-007 FIXED finding's WMF claim (addendum) and the
   E-row if the census carries the 0..8 reading.
5. Acceptance: a fixture built to the og's grammar carves with the
   exact og extent; the old core-grammar fixture no longer carves as
   placeable; suite green.

## Tests to add

WmfPlaceableOgGrammarCarves (og-convention plant: reserved dword 0,
checksum@20=XOR(words 0..9), tail word 0, extent == std_words*2+22);
WmfPlaceableOldCoreGrammarRejected (inch@16 nonzero, checksum@18
convention — must reject); WmfPlaceableCanonicalAldusRejected
(checksum@18/standard@+20 — must reject: parity with the og).

## Related findings

S-041 (the batch-2 remainder; same evidence file), S-007 (batch-1,
FIXED — the claim corrected here), N-099 (batch-1 citation truth pass).

## Open questions

None — the grammar is decoded end-to-end; the only open item is the
MNY-style question of whether any real-world producer writes the og's
22-byte layout (irrelevant to parity scoring; relevant only to
field-impact estimates).

---

## Addendum (session-8 third lane — independent convergence, merge of the duplicate S-042 filing)

A parallel session-8 lane (this filing) independently read both decompile sites
first-hand — ghidra `RAWSearchDLL_decompiled.c:29820-29845` AND retdec
`retdec_decompiled.c:38495-38515` — and reached the identical gate set. The
retdec rendering independently resolves the word-list question term-by-term:
`v13 = *(int16_t*)(a2+20)` (stored checksum) is compared against the XOR of
the words at byte offsets 0,2,4,6,8,10,12,14,16,18 — i.e. words 0..9 exactly,
with the word@+4 term (`v3`) gated ==0 by the caller condition, agreeing with
ghidra's explicit `param_2[0]..param_2[9]` list. Two lanes, two decompilers,
one conclusion. The second evidence file is preserved at
`evidence/decompiled-quotes/s8-wmf-placeable-validator.md` (it carries the
full gate-comparison table and the exact core quote at the `35adcb9` audit
tree).

Line-cite refresh: after the M-081 fix landing (`9e5dedc`), the inch gate sits
at `cr_analysers.cpp:3444` (the function moved; the grammar is unchanged and
STILL unfixed at the current tree — verified by grep this session).

The duplicate filing `S-042-wmf-placeable-validator-contradicts-og.md` (filed
by this lane in the same window, renumbered from S-040 after the collision
with the deep-audit lane's S-040/S-041 set) is removed by this merge; its
unique content — the gate table, the retdec term mapping, and the test names
(`S040WmfPlaceableOgGrammarAccepted` / `S040WmfPlaceableCoreConventionRejected`
/ `S040WmfPlaceableChecksumAt20` / `S040WmfPlaceableZeroDwordGate`) — is
donated to this file's Tests-to-add section by reference.

**S-007 addendum correction rider:** the batch-1 addendum's WMF line
(`S-007...md:222-223`, "the stored checksum is the XOR of words 0..8, the +18
word included in neither side") is refuted by both lanes' reads — the +18
word IS included in the og's XOR (it is word 9), and the stored word sits at
+20. The e018bf4 commit message's "XOR-checksum VERIFIED" was performed
against a misread.


---

## Session-8 lead-lane reconciliation addendum (2026-10-28, the push-window merge)

Convergent independent verification — the lead lane decoded the same probe
(RAWSearchDLL_decompiled.c:29829-29840) and reached the same gates (stored word 10 = XOR of
words 0..9; HWmf==0; Reserved DWORD==0; the standard header 01 00 09 00 at +22;
NumParameters==0 at +38; size = the standard-header Size dword x 2 + 22 — the exact file
length, NOT a heuristic). Two additions for the fix's acceptance criteria: (a) the landed
fixtures must be regenerated from a spec-derived canonical header (or a real .wmf sample),
NEVER from the validator's own output — the batch-1 tests stayed green precisely because
their fixtures were generated from the mis-implementation (self-consistent, not
og-consistent); (b) the og gates on NO inch-range at all — core's `inch == 0 || inch >
14400` rejection is an invented gate on a field the og's placeable probe never reads.


## SA-9 addendum (2026-10-28 — the third lane's convergent decode; merged at the push-time reconciliation)

A second session (SA-9, audited `35adcb9`) independently decoded the same
gate from the ghidra decompile and reached the identical conclusion
(checksum@+20 / XOR words 0..9 / zero dword @+16 / size = u32@+28*2+22 /
no Inch gate in the og; the core's inch@+16 reads Reserved and rejects
every real file; the plant mirrors the wrong convention). Two lanes, two
decompilers, one conclusion — the finding stands triple-verified.

This lane's additional observations, not previously on the record:
1. **The standard-WMF path also diverges**: `validate_wmf_std`
   (cr_analysers.cpp:3417-3445) adds version ∈ {0x0100,0x0300},
   objects ≤ 256, and maxrec ≤ words gates that the og's standard branch
   does not have (the og checks only `01 00 09 00` + u16@+16==0, extent =
   u32@+6*2) — a stricter-than-og subset that can reject og-accepted
   files (version 0x0200 streams; >256-object files).
2. The og's placeable gate order and the canonical field map (Inch@+14,
   Reserved@+16..19, Checksum@+20) are tabulated with both sides quoted
   side-by-side in `evidence/decompiled-quotes/s9-wmf-placeable-gate.md`.
3. The og's numOfParameters gate is at u16@+38 (the following standard
   header's field), which the core's fix plan should carry.
