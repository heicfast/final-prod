# MINOR N-099 — S-007 batch-1 citation truth pass: registry-region and ctor-line cites corrected (evidence hygiene)

**Severity:** MINOR
**Module:** carve (audit-trail truth)
**Parameters affected:** C65-adjacent (evidence accuracy — the audit trail's own citations), C19 context
**Corpus references:** `RAWSearchDLL_decompiled.c` — actual registries: u32 keyed block entries :1728-2509 (59 bindings, builder region :1436-2512 `FUN_180002a50`), u16 block :2536-2911 (35 bindings, builder `FUN_180004320` :2517), list layer :2919-4214; WPG ctor bounds at :15700-01 (magic constant 0x00677077 LE at :15674, registration :2183-91); MDB bounds at :16539-41; the WMF validator at :29807-45 — decode in `evidence/decompiled-quotes/s8-autosave-and-stamps.md` §7 and `s8-wmf-placeable-grammar.md`
**Core references:** audited `ff879f9` — `findings/FIXED/S-007-carve-missing-common-formats.md` (the batch-1 addendum whose cites are corrected here); the landing `e018bf4` commit message (regions ":5450-6600"/":6205-6590"; WPG ":20542-20548"; MDB ":21316-21318")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The batch-1 landing's constant values are correct but several of its
decompile line citations point at the wrong regions: the cited u32/u16
"registry blocks" (:5450-6600, :6205-6590) are INI-parsing and
std::streambuf code; the real registries are :1728-2509 (59 u32
bindings) and :2536-2911 (35 u16 bindings). WPG's and MDB's ctor
bounds live at :15700-01 and :16539-41, not at the cited :20542-48 /
:21316-18. The WPG magic constant is 0x00677077 (LE "wpg\0"), not
0x77706700.

## What the reference does / What our core does

(Not applicable — this is an audit-trail truth finding: the og
behavior is correctly implemented; only the trail's citations drifted.
The corrected regions and counts are in the evidence file.)

## Gap

Falsifiable: open the decompile at :5450 — it is
`FUN_180010b10`-region INI parsing, not analyser registrations.

## Impact

Future sessions re-verifying batch-1 rows against the cited lines will
find unrelated code and may wrongly conclude the landing's evidence is
fabricated. Census counts derived from the wrong regions (the "70
unique keys"/"39 u16" figures) undercount: the true totals are 59 u32
+ 35 u16 = 94 keyed bindings.

## Reproduction / verification

First-hand region reads this session (evidence file §7); two
independent agents converged on the same regions.

## Fix plan (for the engineering team)

1. Add a corrections addendum to the S-007 FIXED finding: registry
   regions, entry counts, WPG/MDB ctor lines, the WPG constant's
   byte order, and the WMF checksum correction (S-042's subject).
2. Add CORPUS-ERRATA rows for the census artifact's wrong rows
   (see this session's errata batch).
3. No code change — every shipped constant verified correct except
   the WMF grammar (S-042, separate finding).

## Tests to add

None (documentation-only).

## Related findings

S-042 (the one real code defect among the corrections), S-041,
M-084.

## Open questions

None.
