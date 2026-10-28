# MINOR N-104 — Two doc-truth defects: ldm.md's dangling table.md reference; CAPABILITY-MATRIX's stale HFS+ MISSING row

**Severity:** MINOR
**Module:** audit-trail / docs (truth pass)
**Parameters affected:** C68 (documentation accuracy)
**Corpus references:** none needed (internal-consistency defects)
**Core references:** audited `ff879f9` — `diff-to-fix/modules/ldm.md:1,9` (the title parenthetical references a nonexistent `table.md` — the partition-table module is covered inside ldm.md itself), `docs/CAPABILITY-MATRIX.md:43` (HFS+ row still "attributes B-tree / journal replay — absent — MISSING" while S-002/S-003/S-021 are FIXED: the journal differential harvest lives at cr_hfsplus_engine.cpp:514-602, with D-46/D-53/D-91/D-93 registering the semantics)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two stale doc rows found by the session's module-vs-source sweep: a
dangling cross-reference and a capability row that contradicts six
landed fixes.

## Gap / Impact / Verification

Falsifiable: `ls diff-to-fix/modules/table.md` — absent;
S-003's Resolution cites the landed journal replay; the matrix row
says absent. Trail truth only.

## Fix plan

1. ldm.md: drop the table.md reference (the module doc itself covers
   src/core/table/).
2. CAPABILITY-MATRIX:43: flip to the landed state with the finding
   refs (S-002/S-003/S-021) and the engine cite.

## Tests to add

None.

## Related findings

N-101/N-102/N-103 (the same truth-pass wave).

## Open questions

None.
