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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-168 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `UserSigns.ShortFragmentMatchesInCore` (the 5-byte fragment vs the 4-byte sign: matched in core, never tested by the og — the og's 2× guard would demand 8; the below-decoded-bound reject still holds; the prevailing installed state restored after the test). Suite green at the landing tree.
**Resolution notes:** Corpus re-verified first-hand: both guard sites re-read (DRWUI :1003371 — `local_d0 < local_d8 + uVar2` with uVar2 the hex-string length; :1003545 — `param_3 < param_5[2] + param_4` with the matcher at the same site using `param_5[2] >> 1`); the core's bound re-read live (cr_usersigns.cpp's decoded-byte check). The fix followed the finding's plan: the superset registered as PARITY-DELTAS D-117 with the og-defect rationale (adopting the 2× guard would lose matches — the CHK-repair identify flow's short-fragment population), NO code change (the core is correct), and the optional pin test landed as the stance's executable documentation.
