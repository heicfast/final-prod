# MINOR N-034 — FAT lost-candidate boot gates: constant/strictness divergences (SPC cap, numFATs, name string, root-entry floor, FAT12 small-FAT probe)

**Severity:** MINOR
**Module:** lost-partition (FAT probe)
**Parameters affected:** B3 (false-positive rate), B1 (detection tolerances), C19-adjacent (constant set)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:2613-2644` (FAT12 gate), `:2650-2680` (FAT16 gate), `:2685-2726` (FAT32 gate), `:2516-2606` (flavor ladder + cluster-count brackets); `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c:38037-38090` (engine-open validator with the SPC bitmask)
**Core references:** `src/core/fat/cr_chain.cpp:261-319` (`fat::validate_boot`), `:12-19` (`flavor_from_clusters` — MATCH with og brackets), `src/core/lost/cr_candidates.cpp:109-126` (`probe_fat`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's FAT lost-candidate gates are both narrower and stricter than our
`validate_boot` in specific, named constants: SPC limited to
{1,2,4,8,16,32,64,128} (we accept any power of two), numFATs ∈ {1,2} on open
(we never check), the "FAT16   "/"FAT32   " name string required in the
reserved area on both the deep FAT32 gate and the engine-open validator (we
never check), root entries ≥ 224 and root_entries×32 % bps == 0 for FAT16 (we
never check), and a FAT12-only "small FAT" probe (spf ≤ 200 sectors) (we have
no FAT12-specific gate). Conversely our bps window (512..4096) is wider than
the engine-open validator's bps == 512 but narrower than the deep scan's
512-multiple. These change the false-positive/noise profile of the FAT
catch-all, which is our only heuristic family.

## What the reference does

Deep-scan gates (PartitionRecoveryD.mo — candidate generation):

- FAT12 (`FUN_1800048f0`, :2613-2644): `numFATs(+0x10) != 0`; a total-sector
  field nonzero; `SPC(+0xD) ∈ {1,2,4,8,0x10,0x20,0x40,0x80}` (:2625-2629);
  **`u16@+0x16 ∈ [1,200]`** — the FAT must be ≤ 200 sectors (:2630-2632);
  signature 0x55AA (:2634).
- FAT16 (`FUN_180004a40`, :2650-2680): same shape without the ≤200 bound
  (`u16@+0x16 != 0`, :2666-2668).
- FAT32 (`FUN_180004b70`, :2685-2726): **`memcmp(boot+0x52, "FAT32   ", 8)`**
  (:2692); `numFATs != 0`; `total32(+0x20) != 0`; SPC ∈ {1,…,0x80}
  (:2700-2704); `spf32(+0x24) != 0` (:2705); 0x55AA (:2711).
- Flavor ladder (`FUN_180004610`, :2516-2606): classic cluster-count brackets
  `clusterCount < 0xFF5` → FAT12-first, `< 0xFFF5` → FAT16-first, `< 0xFFFFFF0`
  → FAT32-first (:2547/:2564/:2581), each bracket trying the expected flavor's
  gate before the fallbacks.

Engine-open validator (FATSearchDLL `FUN_18002f970`, :38037-38090):

- FAT16: `_strnicmp(boot+0x36,"FAT16   ",8)`; `numFATs ∈ {1,2}` (via
  `(byte)(n-1) < 2`, :38049); SPC set via bitmask `0x800000008000808b` on
  spc−1 plus `spc == 0x80` → **{1,2,4,8,16,32,64,128}** (:38050-38052);
  `bps == sector size (512)` (:38053); **root entries > 0xDF (≥224)**
  (:38054); **`root_entries*32 % bps == 0`** (:38055); `spf16 != 0`
  (:38056); 0x55AA → type 1.
- FAT32: `_strnicmp(boot+0x52,"FAT32   ",8)`; numFATs ∈ {1,2}; SPC set;
  bps == 512; `spf32(+0x24) != 0`; 0x55AA → type 2; else type 0xE.

## What our core does

`fat::validate_boot` (cr_chain.cpp:261-319): bps ∈ [512,4096] and power of 2
(:266-268); **SPC any nonzero power of 2** (:270 — 256, 512, … accepted);
`fat_count` read but unvalidated (:274); no FS-name string check; no
root-entry floor or divisibility check; flavor picked by `spf16 != 0 &&
root_entries != 0` shape (:286-293) then refined by cluster count
(:310). `flavor_from_clusters` (:12-19) uses `< 4085 / < 65525` —
**exactly the og's 0xFF5/0xFFF5 brackets (MATCH)**.

## Gap

Falsifiable claims: (a) a 0x55AA sector with SPC = 256 and an otherwise
well-formed BPB yields a FAT candidate in our scanner and is rejected by every
og gate (SPC ∉ {1..128}); (b) a torn-reserved-area FAT32 boot sector (name
string overwritten) yields a candidate in ours, none in the og's deep scan
(FAT32 gate requires the name string); (c) a FAT16 boot sector with
root_entries = 16 passes our validator and fails the og's engine-open
validator (< 224).

## Impact

- Noise profile (B3): our FAT catch-all accepts boot sectors the og classifies
  as non-FAT; on signature-dense disks this adds false lost-partition
  candidates that then each get a full engine scan.
- Edge tolerances (B1): the og's bps==512 open validator is narrower than our
  512..4096 window — but that check sits at engine-open, not discovery, so
  the og's *discovery* is more tolerant than its *open*.

## Reproduction / verification

- Decompiled quotes as cited (all constants with file:line).
- Core quotes; deterministic model reading. The bracket MATCH
  (0xFF5/0xFFF5 ↔ 4085/65525) is recorded as a MATCH note.

## Fix plan (for the engineering team)

1. In `validate_boot` (or a lost-scan-specific wrapper so volume-open stays
   tolerant): clamp SPC to ≤ 128; require `fat_count ∈ {1,2}`; for the lost
   FAT32 gate require the "FAT32   " name string at +0x52 (case-insensitive,
   matching `_strnicmp`); for FAT16 require `root_entries >= 224` and
   `root_entries*32 % bps == 0`.
2. Add a FAT12 small-FAT probe for the `< 4085` bracket (spf ≤ 200 sectors)
   if FAT12 candidate flavor labeling is surfaced.
3. Keep the cluster-count brackets as-is (already og-equal).
4. Tests: the three falsifiable claims above as fixtures.

## Tests to add

- `Lost_Fat_Spc256_Rejected` / `Lost_Fat_NumFats3_Rejected`
- `Lost_Fat32_NameString_Torn_NoCandidate`
- `Lost_Fat16_RootEntries_Low_Rejected`

## Related findings

- M-017 (addendum)/M-017 (scoring/merge — noise candidates flow into those layers).
- S-006 (FAT catch-all is the noisiest family; new families will not use it).

## Open questions

None.

## Resolution

**Status:** FIXED (all five constant families + the ladder; falsifiable claim (a) corrected as a false positive)
**Fixed in commit:** this session's FAT batch commit (new `fat::lost_boot_flavor` + `probe_fat` rewiring + PARITY-DELTAS D-51)
**Verified by:** `LostFatGates.Spc256Rejected`, `LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates` (incl. the engine-open {1,2} decline assertion), `LostFatGates.Fat32NameStringTornYieldsNoCandidate`, `LostFatGates.Fat32GateStringIsCaseSensitive`, `LostFatGates.Fat12SmallFatProbe`, `LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200`, `LostDiscovery.FatBootWithOgGatesBecomesCandidate` (integration through the full discovery pass). The API-dependent tests cannot compile against the pre-fix source (fail by construction); the integration-level behavior change (deep-gate rejection of og-rejected boots) is proven by the suite's lost-partition tests staying green plus the new gate tests. Suite 366/366.
**Resolution notes:** All og constants re-verified first-hand this session from `PartitionRecoveryD_decompiled.c:2516-2726` (ladder `FUN_180004610` + gates `FUN_1800048f0`/`FUN_180004a40`/`FUN_180004b70`) and `FATSearchDLL_decompiled.c:38025-38090`. The new `fat::lost_boot_flavor` implements the ladder exactly: 0x55AA (in-gate), 512-multiple bps, og cluster-count arithmetic (total32 preferred when nonzero — `:2531-2535`; floor root sectors), bracket-ordered gate walk (< 0xFF5: 12,16,32; < 0xFFF5: 16,32,12; < 0xFFFFFF0: 32,16,12), FAT12 spf16 ∈ [1,200] probe, FAT16 spf16 != 0 (no string), FAT32 case-SENSITIVE memcmp "FAT32   " + total32 + spf32. `probe_fat` now requires a gate pass (the old any-valid-BPB acceptance generated candidates the og's deep scan never surfaces — noise into M-017's scoring layers). Engine-open gates (the {1,2} numFATs + bps == device + string + root floor) live in `fat::boot_passes_open_gates` per the og's layering — the deep gates accept numFATs=3 candidates that engine open then declines, exactly like the og (pinned by test). **Corrections to the finding:** (1) falsifiable claim (a) ("SPC = 256 yields a candidate in our scanner") is a false positive — SPC is a u8 field, 256 is unrepresentable, and the old pow2 check was domain-equal to the og's set (pinned by `FatEngine.SpcDomainMatchesOgSet`); (2) the deep FAT32 string gate is case-SENSITIVE (memcmp) unlike the engine-open `_strnicmp` — both implemented as decoded; (3) the og's ladder prefers total32 when nonzero (not total16-first as the finding's summary implied); (4) the og's ladder computes the root term with floor division — matched. The underflow wraparound in the og's uint arithmetic is replaced by an explicit reject (D-51 delta 5).
