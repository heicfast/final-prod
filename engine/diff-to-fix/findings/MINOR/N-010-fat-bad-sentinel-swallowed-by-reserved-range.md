# MINOR N-010 — FatTable::next swallows the BAD sentinel into the reserved-range EOC branch (chains end "clean" at BAD clusters)

**Severity:** MINOR (grading-only: recovered bytes identical; chain grade diverges)
**Module:** fat
**Parameters affected:** A27 (fragment handling), B4 (grading via chain-end semantics), D15
**Discovered during:** the I-002 acknowledgement pin test (diff-to-fix session, T11) —
`FatEngine.Fat16BadClusterStillCanonical` failed against the current main, exposing a
divergence the audit's FAT wave did not flag.

**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`
:54125 (agent-B verified, B-fat sheet): the og's FAT16 `Next` canonicalizes
`0xFFF7 -> 0xFFFFFF7` and `>= 0xFFF8 -> 0xFFFFFF8` — a BAD entry therefore reads back
as an OUT-OF-RANGE value and the og's chain walker marks the chain **broken**.

**Core references:** `src/core/fat/cr_chain.cpp` `FatTable::next` — `entry_at`
correctly returns `CHAIN_BAD` (0x0FFFFFF7) for FAT16 0xFFF7 / FAT32 0x0FFFFFF7, but
`next()` then tests `v >= CHAIN_EOC_MIN && v < 0x0FFFFFF0` (false) and
`v >= 0x0FFFFFF0` (**true — 0x0FFFFFF7 is inside the reserved range**) and returns
`CHAIN_EOC_MIN`: a clean end. `follow_chain` never sees `CHAIN_BAD`, so BAD-terminated
chains grade as intact and never count in `chains_broken`.

## Gap

Falsifiable claim: a FAT16 chain {40 → 41 → 0xFFF7} (or FAT32 with 0x0FFFFFF7) ends
`broken` in the og and `broken == false` in the core, with identical recovered bytes.

## Impact

- Deleted-chain grading on volumes with media-declared BAD clusters: the core grades
  them intact (better-looking than reality) instead of honestly broken; `chains_broken`
  undercounts.
- The N-002/I-002 family semantics: chain-end honesty is the grading input; this bug
  defeated the BAD sentinel that the F-A1-1 canonicalization created.

## Fix plan

In `FatTable::next`, test `v == CHAIN_BAD` and return it **before** the
reserved-range branch (the sentinel is a canonical value in the 0x0FFFFFF0..F7 window
by construction; the range test must not re-interpret it).

## Tests

- `FatEngine.Fat16BadClusterStillCanonical` (the discovering test): chain
  {40 → 41 → 0xFFF7} → `broken == true`, clusters {40, 41}. Fails before, passes after.

## Related findings

- I-002 (FAT12 BAD canonicalization — the deliberate-delta discussion this test
  was pinning), N-002 (EOC families).

## Open questions

None. The FAT12 path is unaffected: its BAD value (0xFF7) canonicalizes to the same
CHAIN_BAD sentinel and hits the same fixed ordering.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: N-010`)
**Verified by:** the discovering test itself: `FatEngine.Fat16BadClusterStillCanonical`
(chain {40, 41, 0xFFF7} -> `broken == true`, clusters {40,41}) FAILED against the
pre-fix main and passes after; full suite 313/313 (Linux ASan).
**Resolution notes:** `FatTable::next` now tests `v == CHAIN_BAD` BEFORE the
reserved-range branch (the 0x0FFFFFF7 sentinel lives inside the 0x0FFFFFF0..F7
window by construction — the range test was re-interpreting it as a reserved-value
clean end). Og parity confirmed: the og canonicalizes 0xFFF7 -> 0xFFFFFF7 (out of
range) so its walker marks the chain broken (FATSearchDLL :54125); both sides now
grade BAD-terminated chains broken. Grading-only: recovered bytes are identical
(the walk stops at the BAD cluster on both sides). Discovered during the I-002
acknowledgement pinning — filed as a new finding rather than silently fixed, per
the session SOP.
