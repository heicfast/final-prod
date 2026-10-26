# INFORMATIONAL I-002 — FAT12 BAD-cluster canonicalization truncates chains the og follows raw

**Severity:** INFORMATIONAL (deliberate, registered delta — content-length divergence on BAD-containing chains)
**Module:** fat
**Parameters affected:** A27 (fragment handling), B2 (recovery correctness — length only)
**Corpus references:** `re-workspace/artifacts/CORRECTIONS.md` row 3; decompiled `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` FAT12 `Next` :53790–53807 (agent-B verified: `if v == 0xFFF → return 0xFFFFFFFF; if 0xFF7 < v → return 0xFFFFFF8; return v;` — 0xFF7 falls through RAW)
**Core references:** `src/core/fat/cr_chain.cpp:30–53` (FAT12 branch), `:160–165` (`follow_chain` `n == CHAIN_BAD → broken`), `src/core/fat/cr_fat_quick.cpp:30` (0x10000 guard, og-parity) — audited commit `67193b6`
**Status:** ACKNOWLEDGED in `f74a5dd` (verified independently by the session-2 audit: CORRECTIONS row 3 + the raw-follow semantics confirmed; pin test Fat16BadClusterStillCanonical green (and it discovered N-010, now fixed); PARITY-DELTAS D-39; future work must PRESERVE the FAT12 delta)

## Summary

The og's FAT12 chain walker returns the BAD-cluster value 0xFF7 (4087) raw as the "next
cluster", so a chain that passes through a BAD-marked entry continues into cluster 4087 and
beyond (garbage-tolerant continuation). Our core canonicalizes 0xFF7 to `CHAIN_BAD`, which
terminates the chain and marks it broken. Files whose chains contain BAD clusters recover
**fewer bytes** than the og (truncated at the BAD entry), with a broken-chain grade instead of
a longer chain.

## What the reference does

Decompiled FAT12 `Next` (B-fat.md §3.1, verified :53790–53807):

```
off = (cluster*3)>>1; v = u16[base+off]; v = (cluster odd) ? v>>4 : v&0xFFF;
if v == 0xFFF → return 0xFFFFFFFF; if 0xFF7 < v → return 0xFFFFFF8; return v;
```

0xFF7 is not special-cased: it is returned as a plain cluster number (4087). CORRECTIONS.md
row 3 records this as a deliberate og behavior (garbage-tolerant), correcting an earlier
artifact that claimed canonicalization.

## What our core does

`src/core/fat/cr_chain.cpp:48–53`:

```cpp
if (v == 0xFF7)
    return CHAIN_BAD;
if (v >= 0xFF8)
    return CHAIN_EOC_MIN;
return v;
```

with the comment citing `PARITY-AUDIT F-A1-1` (without the canonicalization, "every FAT12
chain ended broken — the raw 0xFFF EOC value is < CHAIN_EOC_MIN"). `follow_chain`
(`:160–165`) sets `r.broken = true` and stops at `CHAIN_BAD`.

## Gap

Falsifiable claim: for a FAT12 file whose FAT chain contains a 0xFF7 entry at position k, the
og's recovered extent list has length > k and the core's has length k (truncated), with
`broken=true` grading.

## Impact

FAT12 only (floppies, small legacy volumes, embedded). Content recovered by core is a strict
prefix of the og's; the og's extra bytes come from a cluster the media itself marks bad.
Downstream: fewer bytes recovered; different chain-grade; different inventory hashes in
comparative benches on FAT12 fixtures with bad clusters.

## Reproduction / verification

- Corpus-derived: decompile quote + CORRECTIONS row 3.
- Core-derived: code quotes (file:line).
- Executed: synthetic FAT12 image with a chain [5, 6, 0xFF7, 8, EOC] — predicted og chain
  {5,6,4087,...}, predicted core chain {5,6} broken. Not executed against og (no Windows);
  both code paths quoted.

## Fix plan

Registered deliberate delta — the decision to keep or restore og behavior belongs to the
parity owner. If restoring:

1. In the FAT12 branch of `entry_at`, return the raw value for 0xFF7 (drop the `CHAIN_BAD`
   mapping); keep the 0xFFF→EOC and >0xFF7→EOC mappings (those are og-parity).
2. Keep the FAT16/32 BAD canonicalization unchanged (og canonicalizes those — `:54132`).
3. Add a walk-flag `follow_fat12_bad_raw` (default = og-parity ON for complete scans) so the
   safer truncation remains available for honest-recovery mode.
4. Tests: `test_fat_volume.cpp::FAT12_Bad_Cluster_Followed_Raw` (chain through 0xFF7
   continues; extent includes cluster 4087's FAT successor) — fails before, passes after.

If keeping the current behavior: register I-002 in `docs/parity/PARITY-DELTAS.md` as ACCEPTED
and exclude FAT12-with-bad-cluster fixtures from og-content-comparison benches.

## Tests to add

- `test_fat_volume.cpp::FAT12_Bad_Cluster_Followed_Raw` (above).
- `test_fat_volume.cpp::FAT16_Bad_Cluster_Still_Canonical` (regression guard: FAT16 0xFFF7
   must remain CHAIN_BAD — og canonicalizes there).

## Related findings

- N-002 (EOC family tolerance — same chain-termination family).

## Open questions

None.


## Resolution

**Status:** ACKNOWLEDGED (deliberate delta — kept; the parity-owner call was
made and documented)
**Verified by:** CORRECTIONS row 3 re-confirmed (the og follows FAT12 BAD
0xFF7 raw — garbage-tolerant continuation); the core's honest truncation
plus the F-A1-1 canonicalization history support KEEPING the current
behavior. Already registered: PARITY-DELTAS D-39 verdict (FAT-4 cross-ref).
The requested FAT16 regression guard was added —
`FatEngine.Fat16BadClusterStillCanonical` — and it DISCOVERED a real adjacent
divergence (N-010: the BAD sentinel was swallowed by the reserved-range EOC
branch), filed separately and FIXED. FAT12 content parity on
BAD-containing chains remains a registered divergence (recovered bytes are a
strict prefix of the og's; both graded honestly). Future work must PRESERVE
the FAT12 delta as a reviewed decision.
