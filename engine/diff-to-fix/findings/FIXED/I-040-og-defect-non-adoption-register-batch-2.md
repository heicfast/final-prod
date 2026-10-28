# INFORMATIONAL I-040 — og-defect non-adoption register batch #2: the ext4 i_size_high legacy-ACL pollution and the `_gmtime32_s` post-2038 garbage (both verified og defects the core correctly avoids — registration required)

**Severity:** INFORMATIONAL
**Module:** ext4 (size decode) / cross-engine (time conversion) / registry discipline
**Parameters affected:** calculation mathematics (session directive), registry truthfulness
**Corpus references:** (1) ext4: `EXT4SearchDLL_decompiled.c` — the og unconditionally polls i_size_high for ALL inodes, polluting legacy-ACL directory sizes (the sign-extension quirk at the N-177 compute chain, `:5455-5467` context; lane E's decode notes the unconditional read); (2) time: the og's `_gmtime32_s` usage produces garbage for timestamps past 2038 (32-bit time_t paths in the SearchDLL family — lane E's sweep)
**Core references:** (1) the core computes ext4 sizes with the clean conditional (high-half only where the inode format carries it — `src/core/ext4/cr_ext4_engine.cpp` size decode); (2) the core's 64-bit unix/FILETIME paths throughout (`cr_types.h` FileTimes u64); audited at core `8d0d88b8
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

Two verified og arithmetic defects the core does NOT adopt — the mirror of the D-58/D-59
and N-162 class. Per the registry's own rule, deliberate non-adoption must be
registered or a future session will "fix" the core down to the defect. Batched here
with N-162's FAT width member for one registration commit.

## What the reference does

(1) Unconditional i_size_high poll (legacy-ACL dirs get polluted sizes); (2) 32-bit
time_t conversion garbage post-2038.

## What our core does`

(1) Clean conditional decode; (2) 64-bit throughout.

## Gap

Registration only.

## Impact

None behavioral; drift-risk hygiene.

## Reproduction / verification

Lane E's first-hand decodes (both items carry decompile cites in the lane sheet,
evidence/s12-lane-sheets/lane-E-detection-calc.md — the ext4 quirk sits in the N-177
compute chain, the _gmtime32_s usage in the time-conversion sweep).

## Fix plan

1. One PARITY-DELTAS batch row ("og-defect non-adoption #2": ext4 i_size_high,
   _gmtime32_s post-2038, FAT cluster-math width → fold N-162 here or cross-cite).
2. No code change.

## Tests to add

- Optional anti-down-fix pins: `Ext4.LegacyAclDirSizeClean` and a post-2038 conversion
  pin.

## Related findings

- N-162 (the FAT width member of the same class)
- N-177 (the ext4 compute chain the quirk lives in)
- D-58/D-59 (the precedent family)

## Open questions

None.

## Resolution

**Status:** FIXED (ACKNOWLEDGED — registered D-123 + anti-down-fix pins landed)
**Fixed in commit:** (this session's I-040 landing, S48 INFO-first lane — SHA recorded in the worklog entry)
**Verified by:** corpus re-read first-hand this session: FUN_180007510 at ghidra-searchdll/EXT4SearchDLL_decompiled.c:5455-5467 confirmed on-disk — the unconditional `CONCAT44(*(int*)(param_2+0x6c), *(undefined4*)(param_2+4))` size compute (the +0x6c high dword folded for EVERY inode); the `_gmtime32_s` import confirmed at re-workspace/reversed/EXT4SearchDLL.dll/rabin2_imports.txt:41 (api-ms-win-crt-time-l1-1-0.dll). Core side re-read: the clean conditional at src/core/ext4/cr_ext4_engine.cpp:404-407 (high half only when ifmt(mode)==0x8000) + the full-u64 time paths (cr_types.h unix_to_filetime_ns). Tests: Ext4Engine.LegacyAclDirSizeClean — RED-PROVEN this session (temporarily adopting the og's unconditional poll fails the test; the clean conditional passes) — and the extended Ext4Engine.UnixTimeConversion (exact post-2038/2100 filetimes + the ns variant). Suite 1204: 1162 pass / 42 env-skip / 0 fail.
**Resolution notes:** Registered as PARITY-DELTAS D-123 (the og-defect non-adoption family: D-58/D-59 + D-119/N-162 cross-cited) with both defects' first-hand corpus cites. The finding's OPTIONAL anti-down-fix pins were landed as required tests, not left optional: a future session that "fixes" the core down to the og's unconditional i_size_high poll or a 32-bit time path will fail Ext4Engine.LegacyAclDirSizeClean or the extended UnixTimeConversion. modules/ext4.md row updated to ACKNOWLEDGED. No production code change — the core's behavior is the registered deliberate non-adoption. N-177 (the damaged-flag carrier in the same compute chain) remains open on its own file.
