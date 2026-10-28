# MAJOR M-084 — MDB family: the og's second signature "Standard ACE DB" (ACCDB, type 0x4C) is missing from the core's MDB row

**Severity:** MAJOR
**Module:** carve
**Parameters affected:** C19 (magic bytes/signatures), A28 (validator logic), B1 (detection correctness)
**Corpus references:** `RAWSearchDLL_decompiled.c` `FUN_180015260` :16497-16543 — the u16 0x0100-chain validator gates BOTH signatures: `00 01 00 00` + `"Standard Jet DB\0"` at +4 → type 0x27 (MDB) at :16509-11, and `"Standard ACE DB"` → type 0x4C (ACCDB) at :16515-17; bounds {0x1000 min, 0xBEBC200 max} at :16539-41 (field order differs from WPG's ctor — see N-099); `+0x124 = 1` at :16543
**Core references:** audited `ff879f9` — `src/core/carve/cr_analysers.cpp:3484-3505` (`validate_mdb` gates `00 01 00 00` + `"Standard Jet DB\0"` only — the ACCDB branch is absent), `cr_registry.cpp` MDB row, landed by S-007 batch-1 `e018bf4`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's CMDB analyser accepts two Jet-family signatures: the legacy
"Standard Jet DB" (.mdb, type 0x27) and the ACE-generation
"Standard ACE DB" (.accdb, type 0x4C). The core's batch-1 validator
implements only the first — Access 2007+ databases (ACCDB) do not
carve.

## What the reference does

One validator, two signature branches, two type codes (:16509-17).
Both share the `00 01 00 00` u16-key prefix and the FileInfo bounds
{0x1000, 0xBEBC200}.

## What our core does

`validate_mdb` memcmp's `"Standard Jet DB\0"` only (cr_analysers.cpp:
3497); there is no FMT_ACCDB id (FMT_ enum verified this session).

## Gap

Falsifiable: a buffer `00 01 00 00` + `"Standard ACE DB"` plants an
ACCDB hit in the og and nothing in the core.

## Impact

Every Access 2007+ database (.accdb — the default Access format since
2007) is unrecoverable through carve. A mainstream document format on
the batch-1 family's own key chain.

## Reproduction / verification

Decompiled evidence: the FUN_180015260 decode (both branches cited
above; first-hand region read this session). Core: validator read
directly; FMT_ enum grep (ACCDB: 0 hits).

## Fix plan (for the engineering team)

1. Add `FMT_ACCDB` (append-only) + registry row on the same u16 0x0100
   chain (trial order: keep TTF-before-MDB and MDB-before-ACCDB, the
   og's chain order).
2. Extend `validate_mdb` (or add `validate_accdb`) with the
   `"Standard ACE DB"` branch, type-distinct so the two families name
   differently.
3. Tests: ACCDB plant/carve; Jet-vs-ACE shared-key discrimination;
   hostile variants of both.
4. Acceptance: both signatures carve with distinct names and og-exact
   bounds; suite green.

## Tests to add

AccdbCarves (Standard ACE DB); MdbAccdbSharedKeyDiscriminates;
AccdbRejectsNoise.

## Related findings

S-041 (the batch-2 tail), S-042 (the WMF grammar — the other batch-1
correction), N-099 (batch-1 citation truth pass).

## Open questions

None.
