# MAJOR M-138 — the twin-key device-family offset: the og adjusts kinds {FAT16, EXT2-family}; the core adjusts {FAT12, FAT16, FAT32, exFAT}

**Severity:** MAJOR
**Module:** grading / query (recovery-chances focus battery)
**Parameters affected:** #29 Merge and dedup strategy (twin/overlap detection); #15 Confidence grading model
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:605752-605756` (the `kind ∈ {1, 4} → offset − 2` adjustment in the completion-pass generic path); `:605924-605927` (the same adjustment in the device-walk thread); `architecture/04-recovery-workflow.md` §3.2 (the FS code space: 0-2 FAT, 3 NTFS, 4-5 EXT2, 7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 13 ZFS); `re-workspace/artifacts/SearchDLL/engine-matrix--factory-contract-verification.md:27` (the per-code breakdown: 0=FAT12, 1=FAT16, 2=FAT32)
**Core references:** `src/core/tree/cr_grading.cpp:130-142` (`twin_key_of`: `fat_family = Fat12|Fat16|Fat32|Exfat`; `index = ordinal − (fat_family ? 2 : 0)`); `include/cr/tree/cr_grading.h:65-87` (GradeContext/DeviceSlot) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** M-133 → **M-138** (2026-10-29, the S56 lane: the N-160 doc-truth gate caught the collision live — the remote's suite-red convergent M-133 retired first at `c466d732` 21:08:06 UTC vs this finding's first push `27a70db0` 21:48:00 UTC; the retired row keeps the seat per SA-9 earlier-push-wins; no content change)

## Summary

Both sides fold a per-device ordinal into the twin address key with a −2 shift applied to a chosen family, but they choose **different families**: the og shifts kinds 1 and 4 — FAT16 and the EXT2/3 family code 4 — while the core shifts FAT12, FAT16, FAT32, and exFAT (all four FAT-family kinds) and leaves the ext family unshifted. The shift determines which devices' files can collide as twins in the address map; the collision families are disjoint from the og's, so cross-filesystem twin/overlap detection fires on different device pairs.

## What the reference does

In the completion-pass generic path (verified first-hand at `:605752-605756` this session):

```c
605752        local_a8 = (**(code **)(*local_b0 + 0x30))(local_b0);   /* device offset */
605753        local_bc = (**(code **)(*local_b0 + 0xa0))(local_b0);   /* FS kind code */
605754        if ((local_bc == 1) || (local_bc == 4)) {
605755            local_a8 = local_a8 + -2;
605756        }
```

and the same shape in the device-walk thread at `:605924-605927`. The kind space is the engine FS-code space (`vt+0xa0`): **0 = FAT12, 1 = FAT16, 2 = FAT32, 3 = NTFS, 4-5 = EXT2/3, 7 = exFAT, 8 = HFS, 9 = EXT4, 10 = ReFS, 0xB = Btrfs, 0xD = ZFS** (arch/04 §3.2 + engine-matrix :27). So the og's shifted set is {**FAT16**, **EXT2-family (code 4)**}. The device-walk variant then multiplies by the run unit and adds the fixer offset (`:605939`, `local_b8[0]` — the unit value the core registers as its `kDeviceUnit = 2^40` stand-in).

## What our core does

`cr_grading.cpp:130-142`:

```cpp
const u32 fs = ctx.fs_kind_of(n);
const bool fat_family = fs == u32(FsKindCode::Fat12) || fs == u32(FsKindCode::Fat16) ||
                        fs == u32(FsKindCode::Fat32) || fs == u32(FsKindCode::Exfat);
const i64 index = i64(ctx.disk_ordinal_of(n)) - (fat_family ? 2 : 0);
return u64(i64(base + len) + index * i64(kDeviceUnit));
```

The core shifts the whole FAT family {FAT12, FAT16, FAT32, exFAT} and does **not** shift EXT2-family kinds. Two errors relative to the og: FAT12/FAT32/exFAT are shifted when the og does not shift them (kinds 0, 2, 7), and the ext family (kinds 4-5) is not shifted when the og shifts code 4.

## Gap

Stated falsifiably: for a FAT32-or-exFAT device at ordinal O, the og computes the twin key with the unshifted ordinal while the core uses O−2 — files on such a device can spuriously twin with files on a neighboring device at ordinal O−2's key in the core but not the og; conversely, files on an EXT2-family device shift in the og but not in the core. The twin collision sets differ on both sides of the FAT family and the ext family.

## Impact

Wrong overlap flags and overlap-induced High grades on multi-device scans where FAT and ext volumes coexist (the dual-boot and NAS-adjacent cases the product targets). Bounded blast radius (the twin pass only fires for the population S-058 governs, on the generic path), but the keys are structurally wrong for four of the ten FS kinds. MAJOR — a decoded constant-set divergence in the recovery-chances engine.

## Reproduction / verification

- **Decompiled evidence (first-hand):** `:605752-756` and `:605924-927` read this session; the kind-code space from arch/04 §3.2 + engine-matrix :27.
- **Core evidence (first-hand):** `cr_grading.cpp:130-142` quoted above; the header's own admission at `cr_grading.h:70-75` that the unit constant is a registered stand-in (the family set was not registered as undecoded — it was implemented as FAT-family without og evidence).

## Fix plan (for the engineering team)

1. Change `twin_key_of` (`cr_grading.cpp:138-140`): the shifted set becomes `{Fat16}` ∪ the ext-family kinds the GradeContext's `fs_kind_of` reports for EXT2/3 volumes (the core's `FsKindCode` — map whatever the engine reports for ext2/ext3 family, i.e. the kind(s) whose og code is 4; note ext4 is code 9 and is NOT shifted). FAT12/FAT32/exFAT lose the shift.
2. Keep `kDeviceUnit` as the registered stand-in (unchanged by this finding — the og's `local_b8[0]` unit value remains undecoded; OQ-S42-3 records the question).
3. Tests (below); re-pin any existing twin test that used a FAT32/exFAT fixture under the old shift.
4. Acceptance: a twin test matrix over {FAT12, FAT16, FAT32, exFAT, ext2-family, NTFS} × neighboring ordinals matches the og's shifted/unshifted pattern exactly.

## Tests to add

- `Grading.TwinKeyShiftsFat16AndExt2FamilyOnly` — parametrized over the FS kinds, asserting shift/no-shift per the og's {1, 4} set (fails today on the four wrong members).
- `Grading.TwinKeyFat32DoesNotShift`, `Grading.TwinKeyExfatDoesNotShift` (both fail today).

## Related findings

- **S-058** — the gate polarity (which population reaches this key at all).
- **M-134** — the deep-read validation (consumes the same key).

## Open questions

The og's run-unit value (`local_b8[0]`, `:605939`) — tracked as OQ-S42-3 (it only rescales keys uniformly, so it cannot change which pairs collide; the family-set fix is independent).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-133 landing, session-52 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Grading.TwinKeyFat32DoesNotShift` / `TwinKeyExfatDoesNotShift` / `TwinKeyFat12DoesNotShift` (all three RED pre-fix — the exact spurious cross-device twin pairs the finding describes, via a two-slot GradeContext spaced one kDeviceUnit apart where same-in-slot-extent-end files collide iff the kind shifts), `Grading.TwinKeyFat16StillShifts` (the pin: kind 1 keeps the −2 shift, passes pre- and post-fix), `Grading.TwinKeyExt4NtfsNeverShift` (the unshifted majority pins — EXT4 9 and ZFS 13, both reaching the twin path so the verdicts are non-vacuous). Suite 1290/1290 green (linux-release; the six new twin tests are the delta over this lane's pre-fix run of the same tree).
**Resolution notes:** The shift set narrowed from the FAT family {0,1,2,7} to {Fat16} exactly (cr_grading.cpp twin_key_of) — the corpus-verified og set is kinds {1, 4} (DRWUI :605752-756 + :605924-927, both re-read first-hand this session; the quick-factory dispatch FUN_140577d40 :911110-911175 re-read to confirm the kind space). The finding's fix-plan step 1 mapped the ext2/3 leg to "whatever the engine reports for ext2/3" — this session's verification found the core's engine stamps the whole ext family Ext4 (9), a kind the og does NOT shift, so naively mapping the leg would have shifted true-ext4 volumes (a worse divergence than the status quo). The ext2/3 leg is therefore NOT expressible in the core's current kind space and is registered as OQ-S52-1 (tracked-open): the og's ext2/3-vs-ext4 volume classification rule is undecoded in the corpus (the ExtFileSystemAnalyser superblock validator FUN_180004670 checks magic + geometry; the family decision sits upstream and is not in the artifact), and inventing a classifier would be hallucinated parity. One finding-text correction: the "disjoint collision families" impact claim is structurally inert in the current single-device ABI (production GradeContexts carry one slot; cross-device collisions require equal absolute extent ends which disjoint slices cannot produce) — the fix repairs the constant set for the multi-device GradeContext surface the API already exposes and the tests exercise. kDeviceUnit untouched per fix-plan step 2 (OQ-S42-3 owns the unit question).
