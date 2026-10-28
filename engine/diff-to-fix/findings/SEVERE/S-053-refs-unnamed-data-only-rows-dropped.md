# SEVERE S-053 — ReFS name-destroyed records: the og emits unnamed $DATA-only rows (recoverable content); the core drops them silently

**Severity:** SEVERE (silent loss of recoverable content on a first-class filesystem — the S-034 precedent class: rows the reference surfaces that the core never shows the user)
**Module:** refs (ReFS engine row-emission gate)
**Parameters affected:** B2 (recovery correctness — content present in the reference's result set is absent in ours), B3 (false-negative side of detection), B4 (deleted-file classification — the name-destroyed class is precisely the deleted-and-overwritten-name population), D11 (node fields — the unnamed-row shape)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c:8674-8675` (the name builder `FUN_18000c2b0` no-ops on `keySize ≤ 4` — an empty name is a legal result, not an error); `:8478-8481` (the emit path: no name-liveness gate before the row is enqueued — empty-name rows ARE emitted); `:8317-8337` (`FUN_18000ba90` — the og's actual emission filter is size- and dot-shaped, NOT name-shaped; see N-141); first-hand decode this session (`evidence/decompiled-quotes/s11-zfsrefs-function-walk.md` §3 C1)
**Core references:** audited `46b81573` — `src/core/refs/cr_refs_engine.cpp:1101-1103` (`const std::u16string name16 = name_from_key(ev.key, ev.key_len); if (name16.empty() || ev.val_len < 0x28) return;` — the empty-name row is dropped before `parse_node` ever runs); the prior trail note: `modules/refs.md` B5 ("unnamed $DATA-only records noted — naming fallback parity unknown, minor" — the parity is now decoded: the og has NO fallback name and NO drop; the note's "minor" guess is refuted)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** refs.md B5 flagged the class but marked it "parity unknown" because the og side was undecoded. The S11 function-level walk decoded both sides; this finding is the resolution of that note.

## Summary

When a ReFS record's name key has been destroyed (overwritten metadata, partial
format, corruption — the classic deleted-file aftermath), the og's ReFS engine
still emits the record as a row with an empty name: the name builder returns
empty as a legal value and the emit path has no name gate. The core's engine
treats an empty name as a malformed record and returns before parsing — so the
record's $DATA content never enters the tree. On a volume with name-destroyed
records, the og recovers content the core silently never shows.

## What the reference does

The name builder `FUN_18000c2b0` (`ReFSSearchDLL_decompiled.c:8674-8675`)
returns without writing a name when `keySize ≤ 4` — empty is a legal return.
The row-emission path (`:8478-8481`) enqueues the record with whatever the
builder produced; there is no name-liveness check anywhere before emission.
The og's only emission filter in this area is `FUN_18000ba90` (`:8317-8337`),
which is size-shaped and dot-shaped (zero-size records without a '.' in the
name — N-141), never name-shaped. Net: a record whose name key is gone but
whose $DATA runs survive is surfaced by the og as an unnamed row — recoverable
content.

## What our core does

`cr_refs_engine.cpp:1101-1103`: `name_from_key` produces the (possibly empty)
name; `if (name16.empty() || ev.val_len < 0x28) return;` drops the record
before `parse_node` — the node is never built, never graded, never emitted.
(The `val_len < 0x28` half is a legitimate minimum-record-shape guard; the
`name16.empty()` half is the divergence.)

## Gap

Falsifiable: construct a ReFS fixture with one healthy named record and one
record whose name key is empty/destroyed but whose value blob ($DATA runs)
is intact. Run the og's ReFS engine (or cite the decompile): the unnamed row
is emitted. Run the core: the row is absent (the `name16.empty()` early
return). The recovered-bytes sets differ by exactly that record's content.

## Impact

Users recovering damaged/partially-formatted ReFS volumes (the exact
population ReFS recovery targets — ReFS has no undelete semantics, so
post-deletion recovery is dominated by name-destroyed remnants) lose every
name-destroyed-but-content-intact record in the core vs the og. Silent: no
error, no log line, no count — the files simply do not exist in the result
tree. Downstream (session/report/export) inherits the hole.

## Reproduction / verification

Decompiled evidence (strength 2): the og cites above, re-read first-hand by
the lead; the full decode walk in
`evidence/decompiled-quotes/s11-zfsrefs-function-walk.md` §3 C1. Core-side:
the `:1101-1103` gate read at `46b81573`. An executed fixture comparison
would require a synthetic ReFS image with a destroyed name key (the
`tests/unit/test_refs_volume.cpp` fixture builder is the natural place for
the RED test) — left to the fix plan.

## Fix plan (for the engineering team)

1. In `cr_refs_engine.cpp`, split the gate: keep `ev.val_len < 0x28` (shape
   guard); replace the `name16.empty()` drop with an unnamed-row emission —
   an empty display name with the og's row shape (the og renders the empty
   name; the core's tree needs a display fallback for the UI, e.g. the
   carve-style `File%06u` synthetic name ONLY at the display layer, keeping
   the stored name empty — do not bake a synthetic name into the session).
2. Grade the unnamed row by content integrity alone (the $DATA runlist
   validation already runs in `parse_node`).
3. Placement: the og emits unnamed rows into the same parent context as
   named ones (no special branch); match that — no separate "Lost" synthetic
   parent at the engine layer (S-048/M-103 own the tree-shape question for
   the lost branches).
4. Tests: `Refs.UnnamedDataOnlyRowEmitted` (fixture: name key zeroed, $DATA
   intact → row present, empty name, content recoverable) — RED before,
   GREEN after; `Refs.UnnamedRowGradeIsContentBased`; session round-trip
   (`Session.RefsUnnamedNameRoundTrip` — the empty name persists and does
   not mutate into a synthetic one).
5. Acceptance: the fixture pair above yields identical row COUNTS between a
   reference-behavior oracle (the decompile's no-gate emit) and the core;
   docs/REFERENCE-API-GAP.md gains no new row (this is engine-internal).

## Tests to add

- `Refs.UnnamedDataOnlyRowEmitted` (the RED/GREEN oracle above)
- `Refs.UnnamedRowGradeIsContentBased`
- `Session.RefsUnnamedNameRoundTrip`
- Fuzz adjunct: `Refs.Fuzz.NameKeyTruncated` (truncated key stream → empty
  name, row still emitted, no crash — the `name_from_key` boundary)

## Related findings

- N-141 (the SAME emission area, the opposite direction: rows the og filters
  that the core emits — fix both in one pass over the emission gate)
- S-048 / M-103 (tree-shape for non-standard branches — the unnamed rows
  must not grow a synthetic parent without those findings' say-so)
- refs.md B5 (the prior "minor, parity unknown" note this resolves)

## Open questions

None — both sides are decoded. The only deferred item is whether the og's UI
renders empty names as-is or applies a display fallback at the DRWUI layer
(UI-side cosmetic; does not affect the engine contract).
