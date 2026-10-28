# MINOR N-168 — the og's user-sign bounds guard demands offset + hex-STRING length (2× the bytes actually read): fragments in the gap never match — the core uses the correct bound (unregistered superset)

**Severity:** MINOR
**Module:** carve (user-sign matching edge)
**Parameters affected:** detection correctness edge (1), false-negative semantics of the og's own consumer
**Corpus references:** `DRWUI_decompiled.c:1003371`, `:1003545` — the guard `filesize < offset + hexstrlen` while the matcher reads only `hexlen/2` bytes
**Core references:** `src/core/carve/cr_usersigns.cpp:255` — `if (u64(s.offset) + s.len > avail) continue` (s.len = decoded byte length); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's sign matcher refuses to test any file whose length is below `offset +
hexStringLength` — twice the bytes it actually reads (a 16-byte sign at offset 0
requires ≥32 bytes). Fragments of length in [offset+signbytes, offset+2·signbytes) are
never tested. The core uses the correct byte bound and matches them. A core superset,
unregistered.

## What the reference does

The guard at both sites (cites above) compares against the hex TEXT length.

## What our core does

The decoded byte-length bound (cite above).

## Gap

Short-fragment acceptance band: og never matches, core does.

## Impact

Edge-only (the CHK-repair identify flow's short fragments; in the core's carve channel,
device-end extents make the band mostly moot) — registered-superset hygiene, no user
harm.

## Reproduction / verification

Corpus-derived (lane C first-hand: both guard sites). Core-derived (the bound read
directly). Registry: no row.

## Fix plan

1. Register as a deliberate superset (one PARITY-DELTAS line citing both sides); no code
   change (the og's 2× guard is an og defect — adopting it would lose matches).
2. Optional pin test: `UserSigns.ShortFragmentMatchesInCore` documents the stance.
3. Acceptance: the registry row exists.

## Tests to add

- `UserSigns.ShortFragmentMatchesInCore` (the superset pin)

## Related findings

- M-128 / M-129 (the same channel's semantics family)
- LC-9/I-035 (the CHK consumer this guard belongs to)

## Open questions

None.
