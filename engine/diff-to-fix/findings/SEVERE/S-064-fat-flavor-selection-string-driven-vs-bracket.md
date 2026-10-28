# SEVERE S-064 — FAT engine flavor selection is string-driven in the og (FUN_18002f970) but cluster-bracket-driven in the core: on a "FAT16   "-string volume under 4085 clusters the og scans 16-bit FAT entries and the core decodes 12-bit — different chains, different recovered bytes

**Severity:** SEVERE
**Module:** fat (engine-open flavor selection)
**Parameters affected:** B1 (detection correctness), B2 (recovery correctness — wrong entry width ⇒ wrong file content), A25 (engine algorithm steps), C21 (flavor enum semantics)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` FUN_18002f970 (:38036-38090) — the engine-open flavor validator, re-read first-hand; consumer FUN_180021000 (:26318, bails only on flavor 0xE)
**Core references:** audited `34d6cf10` — `src/core/fat/cr_chain.cpp:609` (`flavor = flavor_from_clusters(cluster_count)`, brackets <4085/<65524/else), `:646-666` (`boot_passes_open_gates` — the string gates applied per the BRACKET flavor; the Fat12 case body empty by design), `:184-216` (fat_table_validate — Fat12 media-word check family-tolerant, so the mis-flavor volume is not caught downstream)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's FAT engine-open validator assigns the FAT flavor (1=FAT16, 2=FAT32) purely by matching the FS-name string ("FAT16   " at +0x36 / "FAT32   " at +0x52) plus BPB shape gates — there is no cluster-count bracket anywhere in that function (the bracket function FUN_180004610 belongs to the lost/CalcPartition side). The core computes the flavor from the cluster-count bracket and then applies the string gates per that flavor — with an intentionally empty case body for Fat12. On a volume whose string says FAT16 but whose cluster count falls in the FAT12 bracket (<4085), the og scans 16-bit FAT entries; the core decodes 12-bit entries. Chain termination values, cluster arithmetic, and recovered file content all diverge.

## What the reference does

FUN_18002f970 (`FATSearchDLL_decompiled.c:38036-38090`), re-read first-hand this session:

```c
iVar4 = _strnicmp((char *)(*param_1 + 0x36), "FAT16   ", 8);           /* :38044 */
if ((iVar4 == 0) && (… numFATs ∈ {1,2} … spc ∈ pow2-set/0x80 … bps == device …
                      root_entries ≥ 0xE0 … root×32 % bps == 0 … spf16 != 0 …
                      sig 0x55/0xAA …)) {
    *(undefined4 *)(param_1 + 1) = 1;                                   /* :38059 — flavor := FAT16 */
    return uVar6 / uVar5;
}
iVar4 = _strnicmp((char *)(*param_1 + 0x52), "FAT32   ", 8);           /* :38061 */
… /* FAT32 gates: spc, bps, spf32(+0x24) != 0 … */
    *(param_1 + 1) = 2;                                                 /* flavor := FAT32 */
```

The flavor is assigned by STRING MATCH + BPB shape gates. No cluster count is computed or compared in this function (grep-verified across the function body; the cluster-count bracket machinery — FUN_180004610 — is the PartitionRecoveryD lost-scan classifier, a different module the core correctly brackets on its own lost ladder). A "FAT16   " volume with 3000 data clusters is flavor 1, 16-bit entries, full stop.

## What our core does

`validate_boot` (`cr_chain.cpp:609`): `p.flavor = flavor_from_clusters(p.cluster_count)` — the Microsoft bracket rule (<4085 → Fat12, <65524 → Fat16, else Fat32). Then `boot_passes_open_gates` (`cr_chain.cpp:646-666`) applies the string gates **per the bracket flavor**:

```cpp
case Flavor::Fat16:
    if (!fs_name_matches(sector512 + 0x36, "fat16   ")) return ERR_BAD_PARAMETER;
    …
case Flavor::Fat12:
    /* Deliberate core superset (I-002 family, PARITY-DELTAS): the
     * og's validator has no FAT12 branch and declines such volumes
     * at quick-engine open; the core keeps structural FAT12
     * recovery. No string/geometry gate here. */
    break;
```

So on the string/bracket-disagreeing volume: the core computes Fat12, the Fat12 case applies no gate, and the engine proceeds to decode **12-bit** FAT entries. The downstream whole-table validation does not catch it: `fat_table_validate` (`cr_chain.cpp:184-216`) checks the exact 0xFFFFFFF8 media word only for Fat16 — "FAT12/FAT32 are family-tolerant" — so the mis-flavor decode stands.

## Gap

Falsifiable: a volume with "FAT16   " at +0x36, valid FAT16 BPB shape (root ≥ 224, spf16 ≠ 0, sig present), and < 4085 data clusters — a real population: many embedded formatters and older tools format small FAT16 volumes irrespective of Microsoft's bracket rule. The og: flavor 1 → 16-bit entries → chains terminate at 0xFFF8-0xFFFF → correct file content. The core: flavor Fat12 → 12-bit entries → the same FAT bytes decode as different chains (entry boundaries shift every 3 half-bytes) → chain terminations misread (0xFF8-0xFFF vs 0xFFF8-0xFFFF at different entry indices) → truncated or run-away chains → **recovered file content differs from the og's** or validation rejects chains the og recovers.

## Impact

Wrong-content recovery (not merely detection loss) on the string/bracket-disagreeing population — the worst class: a user recovers "successfully" and gets different bytes than the reference product. This is the inverse direction of the D-94-registered FAT12 exemption (which registered only the og's DECLINE of string-less FAT12 volumes); the string-present/bracket-disagreeing case was never registered.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: the complete FUN_18002f970 read (string compare, gate set, flavor stores), the absence of any cluster-count term in the function, and the consumer FUN_180021000's flavor-0xE-only bail (:26318). Core side: the bracket assignment, the per-flavor switch, and the Fat12-tolerant media check all quoted at `34d6cf10`. Executable pin: the fix plan's first test constructs the disagreement fixture directly.

## Fix plan (for the engineering team)

1. Make the engine-open flavor selection string-first when a string gate passes, mirroring FUN_18002f970: if the "FAT16   " string + FAT16 BPB shape gates pass → flavor := Fat16 (regardless of cluster count); same for "FAT32   " → Fat32. Fall back to the bracket only when no string matches (preserving the core's deliberate FAT12 superset for string-less volumes — keep the D-94 registration wording accurate for that case).
2. Alternatively (more conservative): special-case only the disagreement — when the string says FAT16/32 and the bracket says Fat12, take the string's flavor; leave all current behavior otherwise.
3. Keep `flavor_from_clusters` for the lost ladder (`lost_boot_flavor`, where the og also brackets — FUN_180004610 parity is already correct there).
4. Files: `src/core/fat/cr_chain.cpp` (the assignment site + gate switch), tests in the FAT engine test home.
5. Acceptance criteria: the disagreement fixture recovers byte-identical content to a 16-bit-decoded control; all existing FAT tests green; the D-94/I-002 register rows re-worded to the narrowed scope.

## Tests to add

- `FatEngine.StringFat16LowClusterCountScansAsFat16` (the disagreement fixture: "FAT16   ", 3000 clusters, a chain spanning entries > 4085's 12-bit termination class → recovered content equals the FAT16 control) — RED today.
- `FatEngine.StringFat32UnderBracketScansAsFat32` (the FAT32 twin).
- `FatEngine.StringlessVolumeStillBrackets` (the D-94 superset preserved).
- `FatEngine.StringGateRejectsBracketAgreeingButShapelessVolume` (the og's shape gates still enforced on the string path).

## Related findings

- N-188 (the og's FAT1/FAT2 media-word flavor refinement — the SECOND og mechanism the core lacks on the same surface; fixing both together avoids double-touching the open path).
- D-94 / I-002 family (the registered FAT12 superset this finding NARROWS, not contradicts: the superset's justification was string-less volumes, which remain covered).

## Open questions

Whether the og's quick-open consumer (FUN_180021000) re-flavors after the media-word reads (N-188's mechanism) in a way that could override the string flavor on the disagreement fixture — decode N-188's two reads first; if the og re-flavors FAT16→FAT32 on FAT32-shaped media words, the disagreement fixture should use media words consistent with the string to isolate this finding's behavior.
