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

---

## Session-10 addendum (2026-10-28, re-verification sweep at HEAD `84a1832`)

Re-verified STILL-ACTIVE: FIXED/S-007's stale batch-1 cites stand verbatim
at :215-223 (the ":5450-6600"/":6205-6590" regions vs the real registries
:1728-2509/:2536-2911 per E55; "WPG :20515-20550, MDB :21290-21330" vs the
real ctors :15700-01/:16539-41; the DMG "0xBEBC200 mirrors the og" claim
refuted by M-090/E67; the WMF checksum sentence refuted by S-042/E60). New
residual the sweep caught: erratum **E51 itself still carries the wrong MDB
ctor cite "(ctor :21316-21318)"** — the same defect class E63 fixed for
WPG was never fixed for MDB's E51 row (the real MDB ctor bounds sit at
:16539-41 per the session-5 decode). Add to the fix plan: correct E51's
cite alongside the S-007 addendum.

---

## Resolution

**Status:** FIXED-ALREADY
**Fixed in commit:** (landed by the parallel session-8/SCAN-4 lanes; classified this session — the session-38 minor-first lane)
**Verified by:** the live-trail reproduction: CORPUS-ERRATA **E55** (read this session) carries exactly this finding's corrections — "the u32 keyed registry's bindings live at :1728-2509 (59 entries) and the u16 registry's at :2536-2911 (35 entries) — total 94 keyed bindings; the landing's cited regions ':5450-6600'/':6205-6590' are INI-parsing and std::streambuf code (N-99; evidence s8-autosave-and-stamps.md §7)" — with the evidence file's §7 read this session (:94/:101 quote the corrected regions); the WPG constant's byte order (0x00677077 LE, not 0x77706700) is corrected at SCAN-ENGINE-DELTA:78/:260 and modules/carve.md:630; the WPG/MDB ctor-line re-pins ride E55's evidence file; the WMF grammar correction is S-042's own resolution (f6e303e).
**Resolution notes:** The finding's asks were (1) the S-007 addendum and (2) the census errata rows — both landed by parallel lanes before this session reached the item: the corrections live in E55 + the live gate docs (the retired S-007 file's history copy carries the landing-time cites; the LIVE truth is E55 + the corrected SCAN-ENGINE-DELTA/carve.md rows). No code change was ever needed (every shipped constant was correct except the WMF grammar = S-042, separately resolved). This classification is recorded so the board reflects that the citation-truth tail is closed.
