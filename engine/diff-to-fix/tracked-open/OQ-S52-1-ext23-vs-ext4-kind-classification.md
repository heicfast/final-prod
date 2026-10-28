# OQ-S52-1 — the og's ext2/3-vs-ext4 volume classification rule (which volumes report kind 4/5 vs 9)

**Filed:** session-52 lane, discovered during M-133 verification (2026-10-29)
**Parent finding:** M-133 (the twin-key device-family shift set — FIXED this session)
**Severity of the residual:** MINOR-class bookkeeping gap in kind reporting; MAJOR-class
only if a future multi-device ABI makes the twin pass's ext2/3 leg load-bearing

## The question

The og's twin-key shift set is kinds {1, 4} (DRWUI_decompiled.c:605752-756, :605924-927,
re-read first-hand this session): FAT16 and the EXT2/3 family code 4. The og's FS-code
space assigns 4-5 = EXT2/3 and 9 = EXT4 (architecture/04-recovery-workflow.md §3.2;
engine-matrix--factory-contract-verification.md:27), and DRWUI's quick-searcher factory
dispatch (FUN_140577d40, :911110-911175) resolves code 4/5 → `EUCreateEXT2QuickSearcher`
and code 9 → `EUCreateEXT4QuickSearcher` — the SAME engine DLL serves both, but the kind
code differs per volume.

**Undecoded:** the rule by which a volume is classified kind 4/5 (ext2/3) versus kind 9
(ext4). The classification happens upstream of the factory dispatch — in the volume
analysis layer (ExtFileSystemAnalyser.mo per the MOM map; its superblock validator
FUN_180004670 checks magic 0xEF53 + geometry but the ext2-vs-ext4 decision was not
decodable from the artifact's current depth).

## Why this matters

The core's FsKindCode has no Ext2/Ext3 members; the ext engine stamps the whole family
`Ext4` (9) — which the og does NOT shift. Consequence: a true ext2/ext3 volume's files
would carry unshifted twin keys in the core where the og shifts them (ordinal −2). In
today's single-device scans the shift is inert for cross-device collisions (absolute
extent ends of disjoint devices never coincide), so no observable divergence exists in
the current ABI surface; the gap becomes load-bearing only when a multi-device session
ABI lands.

## Why it was NOT fixed this session

Implementing a classifier (e.g., "no extents feature → ext2") would be a guess: the
corpus does not decode the rule, and the mandate forbids hallucinated parity. Registering
dead kind codes without a producer would be bookkeeping without behavior.

## Next concrete step

Decode the volume-classification rule from
`re-workspace/reversed/ghidra-mo/ExtFileSystemAnalyser_decompiled.c` (6,021 lines, 297
functions — the IFSAnalyser that owns the volume verdict) and/or the DRWUI volume-kind
assignment feeding FUN_140577d40's callers. When the rule is decoded: add the kind codes,
have `src/core/ext4/cr_ext4_engine.cpp` stamp them per the decoded rule, and extend the
M-133 shift set with the decoded ext2/3 members (the twin_key_of comment reserves the
seat). The M-133 test family (Grading.TwinKey*) extends naturally: a
`TwinKeyExt2FamilyShifts` test per decoded member.
