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

## Evidence addendum (dated 2026-10-29 — S47 INFO-first lane's probe contribution; NOT a resolution, the OQ stays open with its owner)

The analyser probe the "Next concrete step" names was started this session and yielded
the analyser-internal half of the rule; the DRWUI-side kind mapping remains the open half:

- `FUN_180004670` (ExtFileSystemAnalyser_decompiled.c:3404-3434, re-read first-hand) is
  magic + geometry ONLY — `(short)sb[0xe] == -0x10ad` (0xEF53 @+0x38), nonzero inodes/
  blocks/blocks_per_group/inodes_per_group, `2 < s_first_ino`-class field, the group-count
  identity `(blocks-1)/bpg == (blocks - first_data_block - 1)/bpg`, blocksize
  `0x400 << (log_bs & 0x1f)` with the ro_compat 0x200 bigalloc-style double-shift, and
  the `first_data_block == (blocksize == 0x400)` canonical-1KiB check. **No feature-flag
  discrimination lives in the validator** — the family verdict is upstream of it.
- The verdict function (the FUN_180004670 caller at :3560-3720, the copy at :3581-3603
  feeds a 256-byte superblock view) writes the analyser's own verdict to analyser+0x58:
  - **verdict 5** vs **verdict 6**: taken when the copied superblock's incompat is
    within the `0xffffffe9` mask (allowed bits {0x2, 0x4, 0x10} — FILETYPE,
    needs_recovery, and 0x10) AND ro_compat within `0xfffffff8` (allowed {0x1, 0x2,
    0x4} — sparse_super, large_file, btree_dir); 5 when the `bStack_3dc & 4` journal-
    indicator leg is clear-or-absent, 6 when it is set with the `local_358` companion.
    This is the ext2-vs-ext3 split inside the family (the journal).
  - **verdict 0x18 (24)**: the strict ext4-shape gate — incompat must carry
    `0x2c0 == 0x40|0x80|0x200` (EXTENTS | 64BIT | FLEX_BG, ALL three) with a
    `local_33a == 0x40` companion check and a sign gate on the copied field; falls to
    0xff (refusal) when the shape fails.
  - The analyser's codes {5, 6, 0x18} are its own internal verdict space — NOT the
    DRWUI factory's {4/5, 9} kind codes; the mapping analyser-verdict → DRWUI kind
    (where 9 = ext4 gets stamped) is the remaining undecoded half.
- The stack-field → superblock-offset mapping for the companion legs (`bStack_3dc`,
  `local_358`, `local_33a`) was not fully resolved this pass; the three gates above are
  decoded from their mask constants, which are unambiguous. The next session should
  map those three locals to on-disk fields (they are within/copied-from the 256-byte
  superblock view + the second GDT-position buffer read at LAB_180004a9b) and then
  trace the analyser-verdict → DRWUI-kind assignment feeding FUN_140577d40's callers.
