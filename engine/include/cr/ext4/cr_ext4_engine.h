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
    u64 damaged_inodes = 0; /* unparseable extents / truncated records */
    u64 journal_blocks = 0;
    u64 journal_salvaged = 0; /* names recovered from journal dir blocks */
    u64 block_groups = 0;
};

struct Ext4ScanFlags {
    bool keep_deleted = true;
    bool scan_deleted_inodes = true; /* whole-table sweep for i_dtime */
    bool salvage_journal = true;     /* journal chain -> directory blocks */
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
    u16 rev_level = 0;
    u8 volume_name[17] = {0};
    bool valid = false;
};

/* The decompile-verified sanity gate (FUN_180004710) + canonical fields. */
bool validate_superblock(const u8 sb[1024], SuperBlock& out);

/* One resolved file extent in slice-relative BYTES. */
struct Extent {
    u64 offset; /* slice byte offset */
    u64 length;
};

/* Inode view (decoded from the on-disk inode). */
struct InodeView {
    u16 mode = 0;
    u64 size = 0;
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

/* unix epoch seconds -> our 100ns/1601 FileTimes fields */
u64 unix_to_filetime(i64 unix_sec);
u64 unix_to_filetime_ns(i64 unix_sec, u32 nsec);

} // namespace ext4
} // namespace cr

#endif // CR_EXT4_ENGINE_H_
