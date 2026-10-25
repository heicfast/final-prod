# MAJOR M-050 — Session node-model round-trip gaps: the og persists ~15 wrapper/node state fields the core's wire cannot carry (hidden roots, grading bits, orphan hint, roles, fixup)

**Severity:** MAJOR
**Module:** session + tree
**Parameters affected:** D11 (node fields), D12 (session persistence format — field equivalence), B4 (deleted/grading state — grading bits 5-7 of node+0x73 unpersisted), Q68 (documentation accuracy of the session contract)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-record4--tree-node-grammar.md` (record-4 grammar: type_flags incl. the −0x10 hidden bit, flag_72/flag_73 bit fields — flag_73 carries the grading bits 5-7 per S-023's decode —, f_6c/f_68=0xFFFF sentinels, f_71 enum with 2↔5 remap, root-role u32s, fixup field, marker/parent/name validation), `rsf-format--scan-session.md:315-322` (the CFileSystemNode extension: type_flags re-serialization, 40B stat blob, f_1c8/f_1d0/f_1d4)
**Core references:** `src/core/session/cr_records.cpp` (serialize/deserialize_tree — the core's per-node wire carries: marker, parent_id, name, props {marker/origin/deleted/confidence/fs_kind/size/extents/times/hash}); `include/cr/tree/cr_node.h` (node state surface) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's tree-node session record round-trips roughly fifteen state fields beyond the core's
carrier: the wrapper's type_flags (including the hidden-root −0x10 bit), the flag_72/flag_73
bit fields (flag_73 bits 5-7 are the og's grading values — see S-023), f_6c/f_68 sentinel
words, the f_71 enum (with a 2↔5 load-time remap), root-level role u32s, and a fixup field.
The core's node wire carries a four-field state subset (origin, deleted, confidence,
ext-shifted). Any og session state expressed through those fields — hidden roots, graded
nodes, orphan-section hints — cannot be represented, and the core's own equivalents (where
they exist) are not persisted.

## What the reference does

`rsf-record4--tree-node-grammar.md` (first full read by any session — this artifact was
never cited before session 5) documents the record-4 tree-node grammar in full: per node the
wrapper persists type_flags (with a documented hidden-root bit), flag_72 and flag_73 bit
fields, two 0xFFFF-sentinel words (f_6c/f_68), the f_71 enum (the loader remaps value 2 to 5),
root-role u32s, a fixup field, plus the standard name/size/time/runlist fields — and applies
per-node validation on load (marker ∈ {0,1}, name-length bounds, parent-reference checks).
`rsf-format--scan-session.md:315-322` adds the CFileSystemNode extension fields
(type_flags re-serialized, the 40-byte stat blob, f_1c8/f_1d0/f_1d4).

## What our core does

`cr_records.cpp`'s node wire: marker byte, parent_id u32, name (u16 length + UTF-16), then
props — logical_size, times, fs_kind, extents, sha128/hash_valid, carve_format,
carve_confidence — and a small state byte family: origin (2 bits), deleted_state (2 bits),
confidence enum, plus NODE_FLAG_EXT_SHIFTED (0x8000) for rebased carved nodes. There is no
hidden-root bit, no grading-bits carrier, no role u32s, no fixup field, no enum-remap
handling, and no per-node marker/parent validation beyond structural resync.

## Gap

Falsifiable claim: an og session containing a hidden root (type_flags −0x10), a graded node
(flag_73 bits 5-7 = 2), and a role-carrying root round-trips all of that state; the core's
session format has no field that can hold any of the three — the states are lost at
serialization time even when the in-memory tree could represent approximations.

## Impact

Session resume is the og's headline workflow; any state the og preserves through save/load
and the core drops changes post-resume behavior observably: hidden roots reappear, grading
vanishes (compounding S-023 — when the grading engine lands, its state has no session
carrier), roles/sections degrade. This finding is the schema debt that S-023's fix and any
hidden/orphan work will hit; filing it now pins the carrier work.

## Reproduction / verification

- Corpus: rsf-record4 (never cited before this session — the citation-gap sweep surfaced it)
  + rsf-format extension fields, both read first-hand.
- Core: the wire field list enumerated from cr_records.cpp first-hand; the negative space
  (no hidden/grading/role/fixup carriers) verified by field-by-field comparison.

## Fix plan (for the engineering team)

1. Extend the core's node wire append-only: add a state-flags u16 (bit 0x10 hidden-root,
  bits 5-7 grading value per S-023's decode — reserve now, populate when the grading engine
  lands), a role u32 pair for roots, and an orphan-hint byte; document each in the session
  contract doc with the rsf-record4 field it mirrors.
2. Add the loader-side validation the og applies (marker, name-length, parent-ref) — see
  N-058 for the validation-delta specifics; land as one loader hardening change.
3. Round-trip tests for each new field; keep old sessions loadable (unknown bits ignored
  forward-compatibly — the core's own format versioning rule).
4. When S-023's grading engine lands, its per-node grades must persist through the new bits —
  add the acceptance test to the S-023 fix plan.

## Tests to add

- `Session.HiddenRootRoundTrip`
- `Session.GradingBitsReserveRoundTrip` (write-read the reserved bits)
- `Session.RootRolesRoundTrip`
- `Session.OldFormatStillLoads` (forward-compat guard)

## Related findings

- S-023 (grading engine — bits 5-7 are its persistence carrier; this finding reserves them)
- S-031 (carved deleted flag — the deleted bits already have a carrier; producer-side fix)
- M-047 (stats round-trip — same loader path, land together)
- N-058 (load validation deltas — the og's per-node checks the core lacks)

## Open questions

The exact semantics of f_6c/f_68 (0xFFFF sentinels) and the 2↔5 remap target of f_71 are
censused but not decoded in the corpus artifact — the core should carry them as opaque u16s
if byte-fidelity matters, or document them as NOT-PERSISTED (deliberate) once a decode lands.
Tracked here, not blocking the carrier work.
