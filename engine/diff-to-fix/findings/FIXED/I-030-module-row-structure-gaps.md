# INFORMATIONAL I-030 — three module-map structure gaps: ldm.md references a nonexistent modules/table.md ("table.md covers MBR/GPT/EBR"), and the reghive and apply surfaces have no module rows at all (their coverage lives inside other findings)

**Severity:** INFORMATIONAL
**Module / Surface:** audit-trail structure (diff-to-fix/modules/)
**Parameters affected:** 46-adjacent (the roadmap's navigability), 68-adjacent
**Corpus references:** n/a — audit-trail structure finding.
**Core references:** audited commit `ff879f9` — `diff-to-fix/modules/ldm.md:1` ("table.md covers MBR/GPT/EBR" — `ls diff-to-fix/modules/`: **no table.md**; the MBR/GPT/EBR row is scored inside ldm.md B1 :26-30, EBR in M-002/N-057); no `reghive.md` (coverage: S-014 + the D-16/D-75 registrations — the reghive module read this session confirmed read-only-FVE scope, no missing og behavior); no `apply.md` (coverage: S-029 FIXED + D-98's safety family + M-040's apply endpoints).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** Session-4 created the modules/ map; table.md was referenced but never written (its content landed inside ldm.md); reghive/apply predate the map.

## Summary

("The module map has three structural holes: one dangling cross-reference (table.md) and two uncovered surfaces (reghive, apply) whose audit coverage exists but is not discoverable from the map. No content gap — the parameters are scored somewhere — but the roadmap's own navigation promise says every module has a row.",)

## What the reference does

n/a

## What our core does

As cited.

## Gap

Falsifiable: ls modules/ vs the three references.

## Impact

Navigability only (the audit mandate's own "make it navigable" clause).

## Reproduction / verification

The ls + greps this session.

## Fix plan (for the engineering team)

1. Either write the three rows (table.md as a thin pointer to ldm.md B1; reghive.md and apply.md as summary rows with their existing finding cites — content already exists, this is packaging) or fix ldm.md's cross-reference to point at ldm.md B1 directly and add reghive/apply index lines to README's module list. 2. Keep it read-only-truthful: the rows summarize scored coverage, they do not open new audits.

## Tests to add

None.

## Related findings

N-120 (the same navigability family).

## Open questions

None.


---

## Resolution (session-11B, 2026-10-29): RESOLVED — all three gaps closed

1. **The table.md dangling reference** — fixed earlier by N-104 (ldm.md:1 now
   self-references its own MBR/GPT/EBR rows; the I-028-adjacent note at
   modules/ldm.md:162).
2. **The reghive module row** — **`modules/reghive.md` written this session**
   (the summary row: S-014 FIXED + M-082 FIXED `1f714884` + D-16 + the M-082
   staging row `:3177+`; the regf constants MATCH block; the read-only safety
   posture; the test inventory).
3. **The apply module row** — **`modules/apply.md` written this session**
   (the summary row: S-029 FIXED + D-98 `:2313` + D-68 `:2010` + M-040
   `94c9dda`; the corpus IOCTL chain with the S-029 signature-regen correction;
   the v1 GPT-refusal boundary; the consent/transaction safety family; the test
   inventory).

Per this finding's own fix plan: "the rows summarize scored coverage, they do
not open new audits" — both rows are packaging, truthfully cited. The modules/
map now covers every src/core module directory (28 rows).
