/*
 * cr_ext4_engine.h - ext2/3/4 scan engine.
 *
 * Corpus: EXT4SearchDLL (CEXT4QuickSearch/CEXT4CompleteSearch; superblock
 * gate FUN_180004710 decompile-verified: magic 0xEF53@+0x38, block size
 * 1024<<log@+0x18, first_data_block rules, 64-bit block count
 * lo@+0x04|hi@+0x150, groups = blocks/blocks_per_group; CatchBuffer-style
 * bad-sector salvage is delegated to the transport retry layer).
 *
 * Walks: superblock -> group descriptors (32/64-byte) -> inode tables ->
 * extent trees (0xF30A) with indirect-block fallback (ext2/3) ->
 * directories (linear + hashed, treated as linear streams) -> live tree +
 * deleted-inode pass (i_dtime set) graded against group block bitmaps,
 * plus journal salvage (descriptor chain -> directory-looking blocks).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_EXT4_ENGINE_H_
#define CR_EXT4_ENGINE_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace ext4 {

using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);

struct Ext4ScanStats {
    u64 files = 0;
    u64 dirs = 0;
    u64 deleted_inodes = 0;
    /* M-152: the orphan-active admissions - dtime==0 && links!=0 inodes
     * the live tree never claimed (unreachable-but-intact: torn parents,
     * unlinks without link decrements). The og's sweep filter has NO
     * dtime/links predicate (FUN_180005e50 :4415-4421, raw-asm re-verified
     * at 0x180005f5d-0x180005f87: size_lo, EXTENTS_FL, the 0xF30A magic,
     * eh_entries - four CONTENT gates only); the core's old
     * "orphan-reserved" skip dropped exactly this population. */
    u64 orphan_inodes = 0;
    u64 damaged_inodes = 0; /* unparseable extents / truncated records */
    u64 journal_blocks = 0;
    u64 journal_salvaged = 0; /* names recovered from journal dir blocks */
    u64 block_groups = 0;
    u64 backup_sb_probes = 0;     /* S-027: candidate positions examined */
    u64 backup_sb_block = 0;      /* S-027: volume-relative block of the adopted
                                    backup superblock (0 = primary in use) */
    bool gdt_from_backup = false; /* S-027: descriptors came from the backup
                                      GDT after the found SB, not the front */
    /* N-133: the engine-decoded s_volume_name (the og's Ext4Volume ctor
     * strncpy's it "for the UI" - algorithms-v2 SS2 step 5). NUL-padded;
     * empty = a label-less volume. The engine's own decode surfaced - it
     * was dead code before this field. */
    char volume_label[17] = {0};
};

struct Ext4ScanFlags {
    bool keep_deleted = true;
    bool scan_deleted_inodes = true; /* whole-table sweep for i_dtime */
    bool salvage_journal = true;     /* journal chain -> directory blocks */
    bool rescue_backup_sb = true;    /* S-027: primary SB invalid -> probe
                                        backup-SB positions (og stride walker
                                        FUN_180001b00 + sub-searcher spawn) */
};

/* Superblock geometry (validated by validate_superblock). */
struct SuperBlock {
    u32 block_size = 0;
    u64 blocks_count = 0;
    u32 blocks_per_group = 0;
    u32 inodes_per_group = 0;
    u32 inodes_count = 0;
    u32 first_data_block = 0;
    u16 inode_size = 0;
    u16 desc_size = 32; /* 32 or 64 */
    u64 groups = 0;
    u32 feature_incompat = 0;
    u32 feature_ro_compat = 0;
    u32 journal_inum = 0;
    u32 first_meta_bg = 0;  /* S-026: meta_bg front-region descriptor blocks
                             * (s_first_meta_bg @SB+0x230; 0 = fully
                             * distributed meta_bg layout) */
    u16 block_group_nr = 0; /* S-027: s_block_group_nr @SB+0x5A - which
                             * group this superblock copy belongs to (0 =
                             * primary; backups carry their group number -
                             * verified on real mke2fs images) */
    u16 rev_level = 0;
    u8 volume_name[17] = {0};
    bool valid = false;
};

/* The decompile-verified sanity gate (FUN_180004710) + canonical fields. */
bool validate_superblock(const u8 sb[1024], SuperBlock& out);

/* OQ-S52-1 (resolved): the family kind per the og's decoded analyser rule
 * (ExtFileSystemAnalyser :3616-3680): the 0x2c0 EXTENTS|64BIT|FLEX_BG
 * shape -> FsKindCode::Ext4 (9); the pre-ext4 compat-mask family shapes
 * -> FsKindCode::Ext2 (the og's engine kind 4, M-133's shift member);
 * og-analyser-refused shapes keep Ext4 (no og behavior exists). */
FsKindCode ext_family_kind(const SuperBlock& sb);

/* One resolved file extent in slice-relative BYTES. */
struct Extent {
    u64 offset; /* slice byte offset */
    u64 length;
};

/* Inode view (decoded from the on-disk inode). */
struct InodeView {
    u16 mode = 0;
    u64 size = 0;
    u32 blocks_512 = 0; /* N-177: raw i_blocks_lo (512-byte units) - the
                         * size-exceeds-blocks damaged-marker input */
    i64 atime = 0, ctime = 0, mtime = 0, dtime = 0, crtime = 0;
    u32 atime_nsec = 0, ctime_nsec = 0, mtime_nsec = 0, crtime_nsec = 0;
    u16 links = 0;
    u32 flags = 0;
    std::vector<Extent> extents; /* resolved through the extent tree */
    bool extents_ok = true;      /* false: indirect/inline path failed */
};

/* Quick + complete in one entry (flags toggle the complete behaviors).
 * Emits under root_active (live tree from inode 2) / root_deleted
 * (dtime-marked inodes) / root_lost (journal salvage). */
u32 scan_ext4(IStorageDevice* slice, const Ext4ScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              Ext4ScanStats& stats);

/* unix epoch seconds -> our 100ns/1601 FileTimes fields: shared fold in
 * cr_types.h (W4 dedup, AUDIT-LINUX-FS DC-13). */

} // namespace ext4
} // namespace cr

#endif // CR_EXT4_ENGINE_H_
