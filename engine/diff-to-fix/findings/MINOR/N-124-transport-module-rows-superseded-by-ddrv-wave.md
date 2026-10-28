# MINOR N-124 — modules/transport.md still records the pre-D-DRV posture ("fallback ladder exists and works", "WinPE gate … FIXED") while cr_transport.cpp:563-565 says both are REMOVED — the module contradicts the code it documents

**Severity:** MINOR
**Module / Surface:** transport (modules/transport.md)
**Parameters affected:** 68 (documentation accuracy), A42-adjacent
**Corpus references:** n/a — audit-trail doc-drift finding.
**Core references:** audited commit `ff879f9` — `diff-to-fix/modules/transport.md:92-94,98-99` (the M-030-era rows: the fallback-ladder and WinPE-gate fix records) vs `src/win/cr_transport.cpp:563-565` ("The direct fallback and the WinPE gate are REMOVED") + `docs/PARITY-DELTAS.md:2999-3020` (D-DRV-1/D-DRV-2 registering the removals); N-041 deleted the RdaClient (cr_transport.cpp:6/342/358).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** The D-DRV wave (the driver-only consolidation) removed the surfaces but never annotated the module rows that documented them.

## Summary

Two transport-module rows describe mechanisms the product no longer contains (the fallback ladder and the WinPE gate) with no superseded-note, and the module's session-23 record ends without the D-DRV context. A future reader would re-audit against removed code.

## What the reference does

n/a

## What our core does

As cited.

## Gap

Falsifiable: read transport.md:92-99 vs cr_transport.cpp:563-565 + D-DRV-1/2.

## Impact

Roadmap truth for the transport surface.

## Reproduction / verification

Read both this session.

## Fix plan (for the engineering team)

1. Annotate the two rows as SUPERSEDED by D-DRV-1/D-DRV-2 (keep the historical record, add the pointer). 2. Add a session-8 line to the module noting the removal wave. 3. Same-lane sweep: check modules/driver-install.md against the D-DRV wave for the same class.

## Tests to add

None (doc fix).

## Related findings

N-123 (the same recurrence class); D-DRV-1/D-DRV-2 (the superseding registers); M-030/N-041 (the superseded rows' subjects).

## Open questions

None.
