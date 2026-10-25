/* cr_ext4_engine.cpp - ext2/3/4 scan engine.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ext4/cr_ext4_engine.h"
#include "cr/cr_bits.h" /* shared byte readers (W4 dedup, DC-12) */
#include "cr/cr_text.h" /* S-005: on-disk UTF-8 names decode, not byte-lift */
#include <cstring>
#include <functional>
#include <map>
#include <set>

namespace cr {
namespace ext4 {

namespace {

/* ---- superblock field offsets (canonical; decompile-cross-verified) ---- */
constexpr u32 SB_MAGIC = 0x38;
constexpr u32 SB_BLOCKS_LO = 0x04;
constexpr u32 SB_FIRST_DATA_BLOCK = 0x14;
constexpr u32 SB_LOG_BLOCK_SIZE = 0x18;
constexpr u32 SB_BLOCKS_PER_GROUP = 0x20;
constexpr u32 SB_INODES_PER_GROUP = 0x28;
constexpr u32 SB_REV_LEVEL = 0x4C;
constexpr u32 SB_COMPAT = 0x5C;
constexpr u32 SB_INCOMPAT = 0x60;
constexpr u32 SB_RO_COMPAT = 0x64;
constexpr u32 SB_UUID = 0x68;
constexpr u32 SB_VOLUME_NAME = 0x78;
constexpr u32 SB_JOURNAL_INUM = 0xE0;
constexpr u32 SB_DESC_SIZE = 0xFE;
constexpr u32 SB_BLOCKS_HI = 0x150;
constexpr u32 SB_FIRST_META_BG = 0x230; /* s_first_meta_bg u32 - meta_bg
                                         * front-region bound (e2fsprogs
                                         * ext2_fs.h; verified on real
                                         * mkfs images this session) */
constexpr u32 SB_BLOCK_GROUP_NR = 0x5A; /* s_block_group_nr u16 (kernel
                                         * ext4.h; verified on real backup
                                         * SBs this session) */
constexpr u32 SB_INODE_SIZE = 0x58;     /* u16; s_first_ino u32 owns 0x54..0x57 */
constexpr u16 SB_MAGIC_VALUE = 0xEF53;

constexpr u32 INCOMPAT_64BIT = 0x0080;
constexpr u32 INCOMPAT_FILETYPE = 0x0002;
constexpr u32 INCOMPAT_EXTENTS = 0x0040;
constexpr u32 INCOMPAT_BIGALLOC = 0x0400;
constexpr u32 INCOMPAT_META_BG = 0x0010;
constexpr u32 MAX_META_DESC_READS = 4096; /* lazy meta-group block-read budget
                                           * (bounded; one read per
                                           * meta-group, cached) */

/* S-027 backup-SB rescue bounds. Canonical ext4 group byte-size is
 * 8 * block_size^2 (bpg = 8*bs blocks): 8 MiB (1KiB blocks), 32 MiB (2KiB),
 * 128 MiB (4KiB) - every geometry's group boundaries are multiples of
 * 8 MiB, so probing g*8MiB + {0, 1024} for g >= 1 covers all of them
 * (the +1024 family is the 1KiB-block volume whose group 1 starts at
 * first_data_block + 8192 blocks = 8 MiB + 1024 bytes). Verified against
 * real mke2fs images this session: classic 1KiB image backup at block
 * 8193 = 8 MiB + 1024; 4KiB meta_bg image group-64 boundary at byte
 * 128 MiB. The og's own walker (FUN_180001b00, vtable slot 53, no
 * textual caller - host-side parameterization) probes at sector
 * granularity over a caller-supplied range; the corpus artifact
 * (artifacts/SearchDLL/ext4-engine--deep-dive.md §2) documents the
 * engine probing "the primary offset and backup superblock positions
 * (sparse/backup superblock blocks: 8193, 32768...)" - exactly the
 * canonical positions implemented here. */
constexpr u64 BACKUP_SB_GRID = 1ull << 20;     /* 1 MiB probe grid */
constexpr u64 BACKUP_SB_FIRST = 8;             /* first grid multiple worth
                                                   probing (backups live at
                                                   >= the first group
                                                   boundary = 8 MiB) */
constexpr u64 BACKUP_SB_MAX_GRID = 4ull << 20; /* 4 TiB coverage cap - a
                                                   bounded rescue walk */

constexpr u32 ROOT_INODE = 2;
constexpr u32 JOURNAL_INODE_DEFAULT = 8;
constexpr u32 EXTENTS_FL = 0x80000;
constexpr u16 EXT4_EXT_MAGIC = 0xF30A;
constexpr u32 EXTENT_DEPTH_LIMIT = 5;

constexpr u64 MAX_GROUPS = 1ull << 20;  /* 1M groups = 128 TiB @ 128 MiB/group
                                         * (4K blocks); GDT read is bounded
                                         * by groups*desc_size */
constexpr u64 MAX_INODES = 16ull << 20; /* 16M inodes sweep bound */
constexpr u32 MAX_DIR_BYTES = 64ull << 20;
constexpr int DIR_WALK_DEPTH = 64;

struct GroupDesc {
    u64 block_bitmap; /* absolute block */
    u64 inode_bitmap;
    u64 inode_table;
};

/* ---- inode on-disk layout ---- */
constexpr u32 I_MODE = 0x00;
constexpr u32 I_SIZE_LO = 0x04;
constexpr u32 I_ATIME = 0x08;
constexpr u32 I_CTIME = 0x0C;
constexpr u32 I_MTIME = 0x10;
constexpr u32 I_DTIME = 0x14;
constexpr u32 I_LINKS = 0x1A;
constexpr u32 I_BLOCKS_LO = 0x1C;
constexpr u32 I_FLAGS = 0x20;
constexpr u32 I_BLOCK = 0x28;     /* 15 x u32 */
constexpr u32 I_SIZE_HIGH = 0x6C; /* regular files only */
/* ext4 extended inode fields (kernel fs/ext4/ext4.h): they exist only
 * when inode_size > 128; offsets:
 *   i_extra_isize@0x80, i_ctime_extra@0x84, i_mtime_extra@0x88,
 *   i_atime_extra@0x8C, i_crtime@0x90, i_crtime_extra@0x94, i_version_hi@0x98
 * extra encoding: (nsec << 2) | epoch - seconds += epoch << 32. */
constexpr u32 I_EXTRA_ISIZE = 0x80;
constexpr u32 I_CTIME_EXTRA = 0x84;
constexpr u32 I_MTIME_EXTRA = 0x88;
constexpr u32 I_ATIME_EXTRA = 0x8C;
constexpr u32 I_CRTIME = 0x90;
constexpr u32 I_CRTIME_EXTRA = 0x94;

/* decode a 32-bit seconds field + its *_extra word (epoch bits 1..0,
 * nsec in bits 31..2 - kernel ext4_decode_extra_time). */
i64 decode_extra(i64 sec, u32 extra, u32& nsec_out) {
    if ((extra & 0x3u) != 0)
        sec += i64(u64(extra & 0x3u) << 32);
    nsec_out = (extra & 0xFFFFFFFCu) >> 2;
    return sec;
}

u16 ifmt(u16 mode) {
    return u16(mode & 0xF000);
}

} // namespace

bool validate_superblock(const u8 sb[1024], SuperBlock& out) {
    if (le16(sb + SB_MAGIC) != SB_MAGIC_VALUE)
        return false;
    if ((sb[SB_INCOMPAT] & 1) != 0)
        return false; /* compression unsupported (decompile low-bit guard) */
    const u32 inodes_count = le32(sb);
    const u32 blocks_lo = le32(sb + SB_BLOCKS_LO);
    const u32 bpg = le32(sb + SB_BLOCKS_PER_GROUP);
    const u32 ipg = le32(sb + SB_INODES_PER_GROUP);
    if (inodes_count == 0 || blocks_lo == 0 || bpg == 0 || ipg == 0)
        return false;
    const u32 first_data_block = le32(sb + SB_FIRST_DATA_BLOCK);
    const u32 log_bs = le32(sb + SB_LOG_BLOCK_SIZE);
    if (log_bs > 6)
        return false; /* 64KiB max block size (spec) */
    /* first_data_block x log_block_size kernel rule (decompile gate) */
    if (first_data_block == 0 && log_bs == 0)
        return false;
    if (first_data_block == 1 && log_bs != 0)
        return false;
    const u32 block_size = 1024u << (log_bs & 0x1F);
    if (block_size == 0)
        return false;
    const u64 blocks = blocks_lo | (u64(le32(sb + SB_BLOCKS_HI)) << 32);
    if (blocks == 0)
        return false;
    const u64 groups = blocks / bpg;
    if (groups == 0 || groups > blocks || groups > MAX_GROUPS)
        return false;

    out.block_size = block_size;
    out.blocks_count = blocks;
    out.blocks_per_group = bpg;
    out.inodes_per_group = ipg;
    out.inodes_count = inodes_count;
    out.first_data_block = first_data_block;
    out.rev_level = u16(le32(sb + SB_REV_LEVEL));
    out.inode_size = out.rev_level >= 1 ? le16(sb + SB_INODE_SIZE) : 128;
    if (out.inode_size < 128 || out.inode_size > 1024 || (out.inode_size & 7) != 0)
        out.inode_size = 128;
    out.feature_incompat = le32(sb + SB_INCOMPAT);
    out.feature_ro_compat = le32(sb + SB_RO_COMPAT);
    out.journal_inum = le32(sb + SB_JOURNAL_INUM);
    if (out.journal_inum == 0)
        out.journal_inum = JOURNAL_INODE_DEFAULT;
    out.groups = groups;
    /* S-026: meta_bg front-region bound. s_first_meta_bg counts the
     * descriptor blocks kept in the classic front GDT region (blocks
     * first_data_block+1 .. +s_first_meta_bg); meta-groups beyond it keep
     * their single descriptor block at their own meta-group boundary.
     * mkfs writes 0 for a full meta_bg layout (verified on real images:
     * 1K/64B-desc and 4K/64B-desc, this session). */
    out.first_meta_bg = le32(sb + SB_FIRST_META_BG);
    if (out.first_meta_bg > out.groups)
        out.first_meta_bg = u32(out.groups);
    out.block_group_nr = le16(sb + SB_BLOCK_GROUP_NR);
    if ((out.feature_incompat & INCOMPAT_64BIT) != 0) {
        out.desc_size = le16(sb + SB_DESC_SIZE);
        if (out.desc_size < 64)
            out.desc_size = 64;
        if (out.desc_size > 64)
            out.desc_size = 64; /* engine reads only the low words */
    } else {
        out.desc_size = 32;
    }
    std::memcpy(out.volume_name, sb + SB_VOLUME_NAME, 16);
    out.volume_name[16] = 0;
    out.valid = true;
    return true;
}

namespace {

/* ---------------------------------------------------------------------------
 * Volume context: device + geometry + GDT + bounded readers.
 * ------------------------------------------------------------------------- */
struct Volume {
    IStorageDevice* dev = nullptr;
    SuperBlock sb;

    /* S-027: when the volume was adopted from a BACKUP superblock found
     * at slice byte X, all volume-relative block numbers must be offset
     * by base = X - (first_data_block + s_block_group_nr*bpg)*block_size
     * (the true volume start inside the slice). Primary-SB scans keep
     * base == 0. The og's sub-searcher rebases by found-0x400 - a boot-area
     * model that misplaces every structure by 1024 bytes for backup SBs
     * (group g's copy has no boot skip); our s_block_group_nr arithmetic is
     * exact and is the documented non-adoption (PARITY-DELTAS D-59). */
    u64 base = 0;

    /* S-027: when the front GDT is dead, descriptors are read from the
     * backup GDT block that follows the adopted backup SB (e2fsprogs
     * layout, verified on real images: "Backup superblock at B, Group
     * descriptor(s) at B+1"). */
    u64 alt_gdt_block = 0;
    u64 backup_sb_block = 0;

    /* GDT bytes (raw) - descriptors are indexed per group. For meta_bg
     * volumes gdt stays EMPTY: descriptors resolve per meta-group through
     * the lazy cache below (S-026). */
    std::vector<u8> gdt;
    u64 gdt_block = 0;

    /* S-026 meta_bg state (kernel/e2fsprogs-correct, og divergences
     * deliberately NOT adopted - see PARITY-DELTAS):
     *  - meta_group_size = block_size / desc_size (EXT2_DESC_PER_BLOCK);
     *    the og seeds its stride from s_desc_size (:3368-3373), which only
     *    coincides when desc_size^2 == block_size (4K/64B, 1K/32B) and is
     *    wrong for 2K/32B and 4K/32B layouts.
     *  - descriptor block for meta-group m sits at the meta-group boundary:
     *    first_data_block + (m * meta_group_size) * blocks_per_group,
     *    +1 only for m == 0 (the primary superblock's group). Verified
     *    empirically on real mke2fs images (dumpe2fs + hexdump, this
     *    session): meta-group starts carry NO backup superblock, the
     *    descriptor block IS the group's first block. The og reads at
     *    m*blocks_per_group with no first_data_block adjustment
     *    (FUN_180004c70 :3582-3583, asm-verified), which is correct only
     *    for first_data_block == 0 geometries.
     *  - meta-groups below s_first_meta_bg keep the classic front GDT
     *    location first_data_block + 1 + m (e2fsprogs
     *    ext2fs_descriptor_block_loc2).
     */
    bool meta_bg = false;
    u32 meta_group_size = 0;
    u32 meta_desc_reads = 0;                   /* one per cached meta-group */
    std::map<u64, std::vector<u8>> meta_cache; /* meta-group -> desc block */

    u64 desc_block_for(u64 m) const {
        if (m < sb.first_meta_bg)
            return (m == 0 && alt_gdt_block != 0) ? alt_gdt_block : sb.first_data_block + 1 + m;
        const u64 bg = m * meta_group_size; /* first group of the meta-group */
        /* bg == 0 only when first_meta_bg == 0: group 0 hosts the primary
         * superblock, its descriptor block follows it. Every other
         * meta-group boundary empirically has no backup SB (powers of two
         * never collide with the sparse_super 3/5/7 powers). */
        if (bg == 0 && alt_gdt_block != 0)
            return alt_gdt_block;
        return sb.first_data_block + bg * u64(sb.blocks_per_group) + (bg == 0 ? 1 : 0);
    }

    GroupDesc group(u64 g) {
        GroupDesc d{};
        if (g >= sb.groups)
            return d;
        const u8* p = nullptr;
        if (!meta_bg) {
            if (gdt.empty())
                return d;
            p = gdt.data() + size_t(g) * sb.desc_size;
        } else {
            if (meta_group_size == 0)
                return d;
            const u64 m = g / meta_group_size;
            const u32 idx = u32(g % meta_group_size);
            auto it = meta_cache.find(m);
            if (it == meta_cache.end()) {
                if (meta_desc_reads >= MAX_META_DESC_READS)
                    return d; /* bounded: stop resolving new meta-groups */
                ++meta_desc_reads;
                const u64 db = desc_block_for(m);
                std::vector<u8> buf(size_t(sb.block_size));
                if (db == 0 || db >= sb.blocks_count ||
                    dev->read_at(base + db * sb.block_size, buf.data(), u32(buf.size())) != OK)
                    buf.clear();
                it = meta_cache.emplace(m, std::move(buf)).first;
            }
            if (it->second.empty())
                return d;
            if (size_t(idx) * sb.desc_size + sb.desc_size > it->second.size())
                return d;
            p = it->second.data() + size_t(idx) * sb.desc_size;
        }
        d.block_bitmap = le32(p + 0x00);
        d.inode_bitmap = le32(p + 0x04);
        d.inode_table = le32(p + 0x08);
        if (sb.desc_size >= 64) { /* 64-bit high words (ext4_group_desc):
                                   * block_bitmap_hi@0x20, inode_bitmap_hi
                                   * @0x24, inode_table_hi@0x28 */
            d.block_bitmap |= u64(le32(p + 0x20)) << 32;
            d.inode_bitmap |= u64(le32(p + 0x24)) << 32;
            d.inode_table |= u64(le32(p + 0x28)) << 32;
        }
        return d;
    }

    u64 total_bytes() const { return sb.blocks_count * sb.block_size; }
};

/* Decode one extent-tree node buffer (block_size bytes) at depth; appends
 * resolved slice-relative extents. Returns false on structural break. */
bool walk_extent_node(Volume& v, const u8* node, u32 depth, u64& max_blocks,
                      std::vector<Extent>& out, u32 level) {
    if (max_blocks == 0)
        return false; /* P1-11: global node-read budget spent */
    --max_blocks;
    if (le16(node) != EXT4_EXT_MAGIC)
        return false;
    const u16 entries = le16(node + 2);
    if (u32(entries) > (v.sb.block_size - 12) / 12) /* per-node entry bound */
        return false; /* P0-5: bound must follow the actual block size */
    if (level > EXTENT_DEPTH_LIMIT)
        return false;
    const u8* body = node + 12;
    if (depth == 0) {
        for (u32 i = 0; i < entries; ++i) {
            const u8* e = body + u64(i) * 12;
            const u32 logical = le32(e);
            /* kernel ext4_extents.h: raw ee_len == 0x8000 (EXT_INIT_MAX_LEN)
             * is a WRITTEN 32768-block extent; > 0x8000 means unwritten
             * (bit 15) - decode accordingly */
            const u16 raw_len = le16(e + 4);
            const u32 len = raw_len > 0x8000 ? (raw_len & 0x7FFF) : raw_len;
            const u64 phys = le32(e + 8) | (u64(le16(e + 6)) << 32);
            if (len == 0 || phys == 0 || phys + len > v.sb.blocks_count)
                continue;
            const u64 off = phys * v.sb.block_size;
            u64 blen = u64(len) * v.sb.block_size;
            if (off + blen > v.total_bytes())
                blen = v.total_bytes() > off ? v.total_bytes() - off : 0;
            if (blen == 0)
                continue;
            if (!out.empty() && out.back().offset + out.back().length == off)
                out.back().length += blen; /* coalesce */
            else
                out.push_back({off, blen});
            (void)logical;
        }
        return true;
    }
    /* interior: read children (bounded) - ext4_extent_idx: ei_block u32@0,
     * ei_leaf_lo u32@4, ei_leaf_hi u16@6, ei_unused u16@8 */
    for (u32 i = 0; i < entries; ++i) {
        const u8* e = body + u64(i) * 12;
        const u64 child = le32(e + 4) | (u64(le16(e + 6)) << 32);
        if (child == 0 || child >= v.sb.blocks_count)
            continue;
        std::vector<u8> buf(size_t(v.sb.block_size));
        if (v.dev->read_at(v.base + child * v.sb.block_size, buf.data(), u32(buf.size())) != OK)
            continue; /* bad child: salvage the rest */
        if (!walk_extent_node(v, buf.data(), depth - 1, max_blocks, out, level + 1))
            return false;
    }
    return true;
}

/* Read an inode (1-based number) and decode its extents. */
bool read_inode(Volume& v, u32 ino, InodeView& iv) {
    if (ino == 0 || ino > v.sb.inodes_count)
        return false;
    const u64 group = (ino - 1) / v.sb.inodes_per_group;
    const u32 index = u32((ino - 1) % v.sb.inodes_per_group);
    const GroupDesc gd = v.group(group);
    if (gd.inode_table == 0)
        return false;
    const u64 table_off = v.base + gd.inode_table * v.sb.block_size + u64(index) * v.sb.inode_size;
    u8 raw[1024] = {}; /* s_inode_size can be up to the block size */
    if (v.sb.inode_size > sizeof(raw))
        return false;
    if (v.dev->read_at(table_off, raw, v.sb.inode_size) != OK)
        return false;

    iv = InodeView{};
    iv.mode = le16(raw + I_MODE);
    iv.atime = i64(le32(raw + I_ATIME));
    iv.ctime = i64(le32(raw + I_CTIME));
    iv.mtime = i64(le32(raw + I_MTIME));
    iv.dtime = i64(le32(raw + I_DTIME));
    iv.links = le16(raw + I_LINKS);
    iv.flags = le32(raw + I_FLAGS);
    u64 size = le32(raw + I_SIZE_LO);
    if (ifmt(iv.mode) == 0x8000) /* regular: 64-bit size */
        size |= u64(le32(raw + I_SIZE_HIGH)) << 32;
    iv.size = size;
    /* extended fields present when the inode is larger than the 128-byte
     * good-old layout and i_extra_isize covers them (kernel rule). */
    if (v.sb.inode_size >= 0x98u && v.sb.rev_level >= 1) {
        const u32 extra_isize = le16(raw + I_EXTRA_ISIZE);
        const u32 covered =
            128u + (extra_isize <= v.sb.inode_size - 128u ? extra_isize : v.sb.inode_size - 128u);
        if (I_ATIME_EXTRA + 4 <= covered)
            iv.atime = decode_extra(iv.atime, le32(raw + I_ATIME_EXTRA), iv.atime_nsec);
        if (I_CTIME_EXTRA + 4 <= covered)
            iv.ctime = decode_extra(iv.ctime, le32(raw + I_CTIME_EXTRA), iv.ctime_nsec);
        if (I_MTIME_EXTRA + 4 <= covered)
            iv.mtime = decode_extra(iv.mtime, le32(raw + I_MTIME_EXTRA), iv.mtime_nsec);
        if (I_CRTIME + 4 <= covered)
            iv.crtime = i64(le32(raw + I_CRTIME));
        if (I_CRTIME_EXTRA + 4 <= covered)
            iv.crtime = decode_extra(iv.crtime, le32(raw + I_CRTIME_EXTRA), iv.crtime_nsec);
    }

    u64 budget = 65536; /* P1-11: per-inode node-read budget */
    if ((iv.flags & EXTENTS_FL) != 0) {
        std::vector<u8> node(size_t(v.sb.block_size));
        /* the extent root lives inline in i_block[15] (60 bytes) */
        std::memcpy(node.data(), raw + I_BLOCK, 60);
        const u16 depth = le16(node.data() + 6);
        if (!walk_extent_node(v, node.data(), depth, budget, iv.extents, 0))
            iv.extents_ok = false;
    } else if (ifmt(iv.mode) == 0x8000 || ifmt(iv.mode) == 0x4000) {
        /* ext2/3 indirect blocks: single + double indirection (files < 64KiB
         * are fully direct; larger get the classic ladder) */
        const u32 per_block = v.sb.block_size / 4;
        const u32 direct = 12;
        for (u32 i = 0; i < direct; ++i) {
            const u32 b = le32(raw + I_BLOCK + u32(i) * 4);
            if (b == 0)
                continue;
            const u64 off = u64(b) * v.sb.block_size;
            if (off + v.sb.block_size <= v.total_bytes())
                iv.extents.push_back({off, v.sb.block_size});
        }
        std::function<bool(u32, u32, std::vector<Extent>&)> read_ladder =
            [&](u32 indirection_block, u32 levels, std::vector<Extent>& out) -> bool {
            if (levels == 0 || indirection_block == 0)
                return true;
            std::vector<u8> blk(size_t(v.sb.block_size));
            if (v.dev->read_at(v.base + u64(indirection_block) * v.sb.block_size, blk.data(),
                               u32(blk.size())) != OK)
                return false;
            for (u32 i = 0; i < per_block; ++i) {
                const u32 b = le32(blk.data() + u64(i) * 4);
                if (b == 0)
                    continue;
                if (levels == 1) {
                    const u64 off = u64(b) * v.sb.block_size;
                    if (off + v.sb.block_size <= v.total_bytes())
                        out.push_back({off, v.sb.block_size});
                } else if (!read_ladder(b, levels - 1, out)) {
                    return false;
                }
            }
            return true;
        };
        (void)per_block;
        (void)direct;
        for (u32 lvl = 1; lvl <= 3; ++lvl) {
            const u32 b = le32(raw + I_BLOCK + (11 + lvl) * 4);
            if (b != 0)
                (void)read_ladder(b, lvl, iv.extents);
        }
    }
    return true;
}

/* Read a whole file's/dir's bytes through its extents (bounded). */
bool read_extent_data(Volume& v, const InodeView& iv, u64 max_bytes, std::vector<u8>& out) {
    u64 want = iv.size < max_bytes ? iv.size : max_bytes;
    out.clear();
    if (want == 0)
        return true;
    out.resize(size_t(want), 0);
    u64 done = 0;
    for (const auto& e : iv.extents) {
        if (done >= want)
            break;
        u64 take = e.length;
        if (take > want - done)
            take = want - done;
        if (v.dev->read_at(v.base + e.offset, out.data() + done, u32(take)) != OK) {
            /* salvage: keep zeros for this extent, continue (catch-buffer
             * behavior - the transport retry layer already isolated the
             * bad sectors) */
        }
        done += take;
    }
    return true;
}

/* Directory entry parser over one contiguous buffer. */
struct DirEntry {
    u32 inode = 0;
    std::u16string name;
    u8 type = 0;
};

bool parse_dir_block(const u8* p, u64 len, bool filetype, u32 inodes_count,
                     std::vector<DirEntry>& out) {
    u64 off = 0;
    int sane = 0, total = 0;
    while (off + 8 <= len) {
        const u32 ino = le32(p + off);
        const u16 rec_len = le16(p + off + 4);
        const u8 name_len = p[off + 6];
        /* M-037 (diff-to-fix) — the og's damage model, decoded first-hand
         * from EXT4SearchDLL FUN_180005820 (:4146-4168): a structurally
         * invalid entry is SKIPPED by a 4-byte resync slide (the og's walk
         * never trusts a bad rec_len for its stride — the slide
         * re-syncs on the next 4-aligned valid entry, so the entries
         * AFTER a corruption survive), with the og's exact gate set:
         *   - type byte must be 1..7 (when the filesystem carries types)
         *   - rec_len 4-aligned and >= (name_len+11)&~3 (the padded floor)
         *   - inode# <= inodes_count
         * inode==0 TERMINATES the og's block walk (:4155); the core
         * deliberately keeps walking past holes (recovers more; pinned
         * by Ext4Engine.InodeZeroHoleContinues, I-014-style superset). */
        const u32 floor = (u32(name_len) + 11u) & ~3u;
        const u8 ftype = p[off + 7];
        const bool type_bad = filetype && !(ftype >= 1 && ftype <= 7);
        const bool struct_bad =
            rec_len < 8 || (rec_len & 3) != 0 || off + rec_len > len || rec_len < floor;
        const bool ino_bad = inodes_count != 0 && ino > inodes_count;
        if (struct_bad || type_bad || ino_bad) {
            off += 4; /* the og's resync slide (FUN_180005820 skip path) */
            continue;
        }
        ++total;
        if (ino != 0 && name_len > 0) {
            ++sane;
            DirEntry de;
            de.inode = ino;
            de.type = filetype ? ftype : 0;
            /* S-005 + M-010 (diff-to-fix): ext4 directory names are raw
             * bytes the og passes through UNCONVERTED (EXT4SearchDLL has
             * no conversion import; .rsf stores UTF-8) - decode strictly,
             * and keep invalid bytes LOSSLESSLY (U+E000 + byte, bijective)
             * so the original name bytes round-trip (never byte-lift:
             * "é" must not surface as "Ã©"; never U+FFFD:
             * the og keeps the raw bytes) */
            de.name = text::utf8_to_u16_lossless(
                std::string_view(reinterpret_cast<const char*>(p) + off + 8, name_len));
            out.push_back(de);
        }
        off += rec_len;
    }
    return total > 0 && sane * 2 >= total; /* majority-valid gate */
}

} // namespace

/* ---------------------------------------------------------------------------
 * S-027: backup-superblock rescue (the og's engine-side lost-volume walk).
 *
 * Og mechanics (first-hand, this session): EXT4SearchDLL FUN_180001b00
 * (CEXT4CompleteSearchController vtable slot 53 - the only reference, at
 * .rdata 0x180016880; no textual caller, the host parameterizes it) reads
 * candidate positions, validates each 512-byte window with THE SAME
 * superblock validator as the primary open (FUN_180004710 at :636), and on
 * a hit at byte X spawns a CEXT4CompleteSearch sub-searcher rebased at
 * X - 0x400 (:637-645), collecting it only if it yields >= 1 (:646-660).
 * The corpus artifact (artifacts/SearchDLL/ext4-engine--deep-dive.md SS2)
 * documents the probed positions as "the primary offset and backup
 * superblock positions (sparse/backup superblock blocks: 8193, 32768...)"
 * - the canonical group-boundary backups. Deliberate divergences (D-59):
 * (a) we probe the canonical positions (8 MiB-multiple group boundaries +
 * the 1 KiB +1024 family) rather than an unbounded sector sweep - the og
 * walker's caller-side range is unpinned, the artifact's positions are
 * these; (b) a failed probe read skips onward instead of aborting the
 * whole walk (the og returns 0 on the first failed read - its CatchBuffer
 * proxy pre-salvages; our transport retry layer already isolated the bad
 * sectors upstream); (c) the adopted volume is rebased by its true start
 * computed from s_block_group_nr (SB+0x5A, verified on real backup SBs),
 * not the og's found-0x400 boot-area model (which misplaces every
 * structure by 1024 bytes for backup copies - group g has no boot skip).
 * ------------------------------------------------------------------------- */
enum class BackupSbResult { NotFound, Found, Cancelled };

BackupSbResult rescue_backup_superblock(IStorageDevice* slice, CancelToken* cancel, SuperBlock& out,
                                        u64& found_byte_out, u64& probes) {
    const u64 dev_bytes = slice->geometry().total_bytes;
    u64 max_m = dev_bytes / BACKUP_SB_GRID;
    if (max_m > BACKUP_SB_MAX_GRID)
        max_m = BACKUP_SB_MAX_GRID;
    /* one 8 KiB window per grid step covers BOTH candidate positions
     * (m*1MiB + 0 and m*1MiB + 1024: each needs 1024 bytes) */
    constexpr u64 WIN = 8192;
    std::vector<u8> win{std::vector<u8>(size_t(WIN))};
    for (u64 m = BACKUP_SB_FIRST; m < max_m; ++m) {
        if (cancel && cancel->checkpoint())
            return BackupSbResult::Cancelled;
        const u64 wbase = m * BACKUP_SB_GRID;
        if (wbase + WIN > dev_bytes)
            break;
        if (slice->read_at(wbase, win.data(), u32(WIN)) != OK)
            continue; /* bad region: skip, keep walking (the og aborts the
                       * whole walk on the first failed read - its
                       * CatchBuffer proxy pre-salvages; our transport retry
                       * layer already isolated the bad sectors upstream) */
        for (u64 skip = 0; skip <= 1024; skip += 1024) {
            const u64 at = wbase + skip;
            ++probes;
            /* cheap gate first: the magic u16 at +0x38 (the og's walker
             * reads the full window; the magic gate is behavior-equal -
             * validate_superblock rejects anything without it) */
            if (le16(win.data() + skip + 0x38) != SB_MAGIC_VALUE)
                continue;
            SuperBlock cand;
            if (!validate_superblock(win.data() + skip, cand))
                continue;
            /* consistent backup? its group number must fit the volume and
             * the implied volume start must be block-aligned to this
             * geometry (the copy sits at (first_data_block +
             * group_nr*bpg) blocks from the volume start) */
            const u64 group_first =
                u64(cand.first_data_block) + u64(cand.block_group_nr) * cand.blocks_per_group;
            if (cand.block_group_nr == 0 || group_first * cand.block_size > at)
                continue; /* primary-shaped or inconsistent copy */
            const u64 implied_start = at - group_first * cand.block_size;
            if (implied_start % cand.block_size != 0)
                continue; /* unaligned volume start: not this geometry's
                           * backup (a later probe may match) */
            found_byte_out = at;
            out = cand;
            return BackupSbResult::Found;
        }
    }
    return BackupSbResult::NotFound;
}

u32 scan_ext4(IStorageDevice* slice, const Ext4ScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              Ext4ScanStats& stats) {
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    /* 1. superblock (primary, then the S-027 backup-SB rescue ladder) */
    u8 sb[1024];
    const bool primary_read_ok = slice->read_at(1024, sb, sizeof(sb)) == OK;
    Volume v;
    bool primary_valid = primary_read_ok && validate_superblock(sb, v.sb);
    if (!primary_valid) {
        /* S-027: a destroyed/damaged primary superblock is exactly the og's
         * engine-side lost-volume case (FUN_180001b00 + sub-searcher). The
         * rescue probes the canonical backup positions and adopts the first
         * valid backup SB. */
        if (!flags.rescue_backup_sb)
            return primary_read_ok ? ERR_BAD_MAGIC : ERR_READ_FAILED;
        u64 found_byte = 0;
        const BackupSbResult rr =
            rescue_backup_superblock(slice, cancel, v.sb, found_byte, stats.backup_sb_probes);
        if (rr == BackupSbResult::Cancelled)
            return ERR_CANCELLED;
        if (rr == BackupSbResult::NotFound)
            return primary_read_ok ? ERR_BAD_MAGIC : ERR_READ_FAILED;
        /* exact volume start from the backup's own s_block_group_nr: the
         * backup sits at (first_data_block + group_nr*bpg) blocks from the
         * true volume start (verified on real images: group-1 copy at block
         * 8193 on a 1 KiB volume, s_block_group_nr == 1) */
        const u64 group_first =
            u64(v.sb.first_data_block) + u64(v.sb.block_group_nr) * v.sb.blocks_per_group;
        v.base = found_byte - group_first * v.sb.block_size;
        /* volume-relative block of the adopted copy (invariant across
         * dispatch contexts); the backup GDT follows it at +1 */
        v.backup_sb_block = group_first;
        stats.backup_sb_block = v.backup_sb_block;
    }
    v.dev = slice;
    if ((v.sb.feature_incompat & INCOMPAT_BIGALLOC) != 0)
        return ERR_UNSUPPORTED; /* cluster-granular bitmaps/extents: emitting
                                 * block-granular extents would be WRONG -
                                 * refused loudly, never silently faked */

    /* 2. group descriptors. Non-meta_bg: the whole table is the front GDT
     * at block first_data_block + 1 (classic layout). meta_bg (S-026): the
     * front region holds only the s_first_meta_bg descriptor blocks (a
     * full-meta_bg mkfs writes 0 - one descriptor block at group 0's start
     * +1); every later meta-group keeps its single descriptor block at its
     * own boundary, resolved lazily by Volume::group() with a bounded read
     * budget. Bulk-reading groups*desc_size from the front block would
     * swallow bitmap/inode-table blocks as "descriptors" - exactly the
     * silent-missing-files bug this fix closes. */
    v.meta_bg = (v.sb.feature_incompat & INCOMPAT_META_BG) != 0;
    v.gdt_block = v.sb.first_data_block + 1;
    bool front_gdt_dead = false;
    if (v.meta_bg) {
        v.meta_group_size = v.sb.desc_size ? v.sb.block_size / v.sb.desc_size : 0;
        if (v.meta_group_size == 0)
            return ERR_BAD_PARAMETER; /* desc_size > block_size: impossible
                                       * geometry, refuse loudly */
        /* liveness probe: meta-group 0's descriptor must reference a real
         * inode table on any live volume (lazy read through the resolver) */
        front_gdt_dead = v.group(0).inode_table == 0;
    } else {
        const u64 gdt_bytes = v.sb.groups * v.sb.desc_size;
        if (gdt_bytes == 0 || gdt_bytes > (8ull << 20))
            return ERR_BAD_PARAMETER;
        v.gdt.resize(size_t(gdt_bytes));
        if (slice->read_at(v.base + v.gdt_block * v.sb.block_size, v.gdt.data(), u32(gdt_bytes)) !=
            OK) {
            if (v.backup_sb_block == 0)
                return ERR_READ_FAILED;         /* primary scan, no ladder to walk */
            v.gdt.assign(size_t(gdt_bytes), 0); /* dead front: ladder below */
        }
        front_gdt_dead = v.group(0).inode_table == 0;
    }
    /* S-027 GDT ladder: the e2fsck order - front GDT first, then the backup
     * GDT immediately following the adopted backup SB (e2fsprogs layout,
     * verified on real images: classic "Backup superblock at 8193, Group
     * descriptors at 8194-8197" - the FULL table; meta_bg "Group descriptor
     * at 8194" - meta-group 0's block, later meta-groups stay distributed
     * and resolve through the S-026 resolver). */
    if (front_gdt_dead && v.backup_sb_block != 0) {
        v.alt_gdt_block = v.backup_sb_block + 1;
        if (!v.meta_bg) {
            const u64 gdt_bytes = v.sb.groups * v.sb.desc_size;
            std::vector<u8> alt(size_t(gdt_bytes), 0);
            if (slice->read_at(v.base + v.alt_gdt_block * v.sb.block_size, alt.data(),
                               u32(gdt_bytes)) == OK)
                v.gdt.swap(alt);
        } else {
            v.meta_cache.clear(); /* re-resolve m == 0 through the alt block */
            v.meta_desc_reads = 0;
        }
        stats.gdt_from_backup = v.group(0).inode_table != 0;
        if (!stats.gdt_from_backup)
            v.alt_gdt_block = 0; /* both dead: honest empty resolution */
    }
    stats.block_groups = v.sb.groups;

    const bool filetype = (v.sb.feature_incompat & INCOMPAT_FILETYPE) != 0;

    /* S-025 pass 1: the PRE-WALK journal harvest (og FUN_180003de0 :2773 -
     * the journal pass runs BEFORE the root walk and again AFTER :2778).
     * Pass 1 collects the parent map + entry lists from every journal dir
     * block (no emission); the live walk then runs; pass 2 emits what the
     * live tree did NOT claim, reparented under reachable parents (the
     * og's orphan finalize :3077-3119 + the reachability check
     * :2807-2871 - a child the live tree claimed stays there).
     *
     * The S-004 gates are intact: the jsb gates the WHOLE harvest (magic
     * + blocktype {3,4} at journal block 0), the range is the jsb's own
     * s_first/s_maxlen, every block in range feeds the dir parser, and
     * ESCAPE-tagged blocks additionally parse with the magic restored
     * (descriptor-tag driven, never a zero-byte heuristic). Blocks WITH
     * "." link into the parent map; blocks WITHOUT "." (torn/unlinked
     * remnants - S-004's core case) contribute owner-less entries that
     * surface flat under the lost root exactly like the og. */
    struct JournalDirInfo {
        u32 parent = 0;                /* from ".." */
        std::vector<DirEntry> entries; /* named children */
    };
    std::map<u32, JournalDirInfo> journal_dirs; /* owning dir inode -> info */
    std::vector<DirEntry> journal_orphans;      /* "."-less blocks' entries */
    if (flags.salvage_journal && root_lost) {
        InodeView jiv;
        if (read_inode(v, v.sb.journal_inum, jiv) && !jiv.extents.empty()) {
            std::vector<u8> jdata;
            read_extent_data(v, jiv, 64ull << 20, jdata);
            const u64 bs = v.sb.block_size;
            const u64 jblocks = jdata.size() / bs;
            stats.journal_blocks = jblocks;

            constexpr u32 JBD2_MAGIC = 0xC03B3998u;
            auto jbe32 = [&](u64 blk, u32 off) -> u32 {
                if (blk >= jblocks || blk * bs + off + 4 > jdata.size())
                    return 0;
                const u8* q = jdata.data() + blk * bs + off;
                return (u32(q[0]) << 24) | (u32(q[1]) << 16) | (u32(q[2]) << 8) | u32(q[3]);
            };
            auto jbe16 = [&](u64 at, u32 off) -> u32 {
                if (at + off + 2 > jdata.size())
                    return 0;
                const u8* q = jdata.data() + at + off;
                return (u32(q[0]) << 8) | u32(q[1]);
            };
            auto is_header = [&](u64 blk, u32 type, u32& seq) -> bool {
                if (blk >= jblocks)
                    return false;
                if (jbe32(blk, 0) != JBD2_MAGIC)
                    return false;
                if (jbe32(blk, 4) != type)
                    return false;
                seq = jbe32(blk, 8);
                return true;
            };
            u32 jsb_seq = 0;
            if (is_header(0, 3, jsb_seq) || is_header(0, 4, jsb_seq)) {
                const u32 incompat = jbe32(0, 0x28);
                u32 tag_size = 8;
                u32 flags_off = 6;
                if (incompat & 0x8) { /* CSUM_V2 */
                    tag_size = 14;
                    flags_off = 6;
                }
                if (incompat & 0x10) { /* CSUM_V3: block_tag3_t, flags BE32@+4 */
                    tag_size = 16;
                    flags_off = 4;
                }
                if (incompat & 0x2) { /* 64BIT: blocknr_high extends the tag */
                    if (tag_size < 12)
                        tag_size = 12;
                }
                const u64 begin = jbe32(0, 0x14);
                const u64 declared = jbe32(0, 0x10);
                u64 end = begin + declared;
                if (end > jblocks)
                    end = jblocks;

                /* descriptor scan: the ESCAPE tags drive the restored parse
                 * (S-004's rule - the tag says the first 4 bytes were
                 * zeroed; a zero-byte heuristic would corrupt genuine
                 * zero blocks) */
                std::map<u64, bool> escape_tags;
                {
                    u64 blk = 0;
                    while (blk < jblocks) {
                        u32 seq = 0;
                        if (is_header(blk, 1, seq)) { /* DESCRIPTOR */
                            u64 off = blk * bs + 12;
                            const u64 dend = (blk + 1) * bs;
                            u64 expect = blk + 1;
                            bool first_tag = true;
                            bool last = false;
                            while (off + tag_size <= dend && !last) {
                                const u32 fl = jbe16(off, flags_off);
                                bool escaped = (fl & 1) != 0;
                                last = (fl & 8) != 0;
                                if (expect < jblocks)
                                    escape_tags[expect] = escaped;
                                off += tag_size;
                                if (first_tag && (fl & 2) == 0)
                                    off += 16; /* journal UUID after tag1 */
                                first_tag = false;
                                ++expect;
                            }
                        }
                        ++blk;
                    }
                }

                auto parse_one = [&](const u8* blk) {
                    std::vector<DirEntry> entries;
                    /* M-037: the inode#<=inodes_count gate belongs to the
                     * og's LIVE dir walk (FUN_180005820 :4168); the
                     * journal-salvage surface (S-004's decode) has its own
                     * parser whose gate set carries no count check — a
                     * journal snapshot legitimately references inodes the
                     * truncated/rebuilt superblock no longer counts (the
                     * S-004 fixtures model exactly that: ghost inode 50 on
                     * an 8-inode volume). Gate OFF here, ON in walk_dir. */
                    if (!parse_dir_block(blk, bs, true, 0, entries))
                        return;
                    u32 dir_ino = 0, parent_ino = 0;
                    for (const auto& de : entries) {
                        if (de.name == u".")
                            dir_ino = de.inode;
                        else if (de.name == u"..")
                            parent_ino = de.inode;
                    }
                    if (dir_ino != 0) {
                        auto& info = journal_dirs[dir_ino];
                        if (parent_ino != 0)
                            info.parent = parent_ino;
                        for (const auto& de : entries) {
                            if (de.name == u"." || de.name == u"..")
                                continue;
                            if (de.inode == 0 || de.name.empty())
                                continue;
                            info.entries.push_back(de);
                        }
                    } else {
                        /* no ".": unlinked remnant - entries surface flat
                         * (S-004's core case; the og's parent map gets
                         * no linkage from this block either) */
                        for (const auto& de : entries) {
                            if (de.inode == 0 || de.name.empty())
                                continue;
                            journal_orphans.push_back(de);
                        }
                    }
                };
                std::vector<u8> restored;
                for (u64 jb = begin; jb < end; ++jb) {
                    if (cancel && cancel->checkpoint())
                        break;
                    const u8* raw = jdata.data() + jb * bs;
                    parse_one(raw);
                    auto et = escape_tags.find(jb);
                    if (et != escape_tags.end() && et->second) {
                        restored.assign(raw, raw + bs);
                        restored[0] = 0xC0;
                        restored[1] = 0x3B;
                        restored[2] = 0x39;
                        restored[3] = 0x98;
                        parse_one(restored.data());
                    }
                }
            }
        }
    }

    /* 3. live tree walk from root inode 2 */
    struct WalkCtx {
        Volume* v;
        tree::ResultTree* tree;
        tree::FullNode* ra;
        tree::FullNode* rd;
        Ext4ScanStats* st;
        bool filetype;
        std::set<u32> visited_inodes;
        u64 inode_budget;
    } ctx{&v, &tree, root_active, root_deleted, &stats, filetype, {}, 200000};

    std::function<u32(u32, tree::FileNode*, int)> walk_dir = [&](u32 ino, tree::FileNode* parent,
                                                                 int depth) -> u32 {
        if (cancel && cancel->checkpoint())
            return ERR_CANCELLED;
        if (depth > DIR_WALK_DEPTH || ctx.inode_budget == 0)
            return OK;
        if (!ctx.visited_inodes.insert(ino).second)
            return OK; /* hard-link cycle */
        --ctx.inode_budget;

        InodeView iv;
        if (!read_inode(*ctx.v, ino, iv))
            return OK;
        if (ifmt(iv.mode) != 0x4000)
            return OK; /* not a directory */

        std::vector<u8> data;
        read_extent_data(*ctx.v, iv, MAX_DIR_BYTES, data);
        std::vector<DirEntry> entries;
        if (!parse_dir_block(data.data(), data.size(), ctx.filetype,
                             ctx.v->sb.inodes_count, entries))
            return OK; /* unparseable dir: metadata-only below */

        for (const auto& de : entries) {
            if (de.inode == 0)
                continue;
            if (de.name == u"." || de.name == u"..")
                continue;
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            InodeView child;
            if (!read_inode(*ctx.v, de.inode, child) || child.mode == 0) {
                /* dangling entry: parent dir may be deleted; keep as lost */
                continue;
            }
            const bool is_dir = ifmt(child.mode) == 0x4000;
            auto* props = new (std::nothrow) tree::FileProperties();
            if (!props)
                return ERR_NO_MEMORY;
            props->name = de.name;
            props->fs_kind = u32(FsKindCode::Ext4); /* corpus factory code */
            props->logical_size = child.size;
            props->attributes = is_dir ? 0x10 : 0;
            props->times.created =
                unix_to_filetime_ns(child.crtime ? child.crtime : child.ctime,
                                    child.crtime ? child.crtime_nsec : child.ctime_nsec);
            props->times.modified = unix_to_filetime_ns(child.mtime, child.mtime_nsec);
            props->times.changed = unix_to_filetime_ns(child.ctime, child.ctime_nsec);
            props->times.accessed = unix_to_filetime_ns(child.atime, child.atime_nsec);
            for (const auto& e : child.extents)
                props->extents.push_back({v.base + e.offset, e.length});
            props->record_ref = de.inode;
            tree::FileNode* n = ctx.tree->add_file(parent, props);
            props->release();
            if (!n)
                return ERR_NO_MEMORY;
            /* S-025: FILES enter the visited set too (directories enter
             * at walk_dir entry - inserting here would make the recursive
             * call's own insert a no-op and SKIP THE SUBTREE). Without
             * this, a journal snapshot entry pointing at a LIVE file
             * re-emitted it as a duplicate - the reachability rule needs
             * every claimed inode. */
            if (!is_dir)
                ctx.visited_inodes.insert(de.inode);
            if (child.dtime != 0 || child.links == 0)
                n->set_deleted_state(tree::DeletedState::Deleted);
            if (is_dir) {
                ++ctx.st->dirs;
                if (u32 rc = walk_dir(de.inode, n, depth + 1))
                    return rc;
            } else {
                ++ctx.st->files;
            }
        }
        return OK;
    };

    tree::FileProperties* root_props = new (std::nothrow) tree::FileProperties();
    if (root_props) {
        root_props->name = u"/";
        root_props->fs_kind = u32(FsKindCode::Ext4);
        root_props->attributes = 0x10;
        root_props->record_ref = ROOT_INODE; /* S-025: the record index
                                              * answers the root chain */
        tree::FileNode* root_node = tree.add_file(root_active, root_props);
        root_props->release();
        if (root_node == nullptr)
            return ERR_NO_MEMORY;
        if (u32 rc = walk_dir(ROOT_INODE, root_node, 0))
            return rc;
    }

    if (progress)
        (void)progress(v.sb.groups, v.sb.groups, progress_user);

    /* 4. deleted-inode sweep: every table entry with dtime set that the
     * live walk did not emit (graded via group block bitmaps). */
    if (flags.scan_deleted_inodes && flags.keep_deleted && root_deleted) {
        /* group block-bitmap cache (one read per group) */
        std::map<u64, std::vector<u8>> bitmaps;
        auto block_free = [&](u64 block) -> bool {
            const u64 g = block / v.sb.blocks_per_group;
            auto it = bitmaps.find(g);
            if (it == bitmaps.end()) {
                const GroupDesc gd = v.group(g);
                std::vector<u8> bm(size_t(v.sb.block_size));
                if (gd.block_bitmap == 0 ||
                    slice->read_at(v.base + gd.block_bitmap * v.sb.block_size, bm.data(),
                                   u32(bm.size())) != OK)
                    bm.clear();
                it = bitmaps.emplace(g, std::move(bm)).first;
            }
            if (it->second.empty())
                return false; /* unknown: assume in use */
            const u64 bit = block % v.sb.blocks_per_group;
            const u64 byte = bit / 8;
            if (byte >= it->second.size())
                return false;
            return (it->second[size_t(byte)] & (1u << (bit % 8))) == 0;
        };

        const u64 sweep = v.sb.inodes_count < MAX_INODES ? v.sb.inodes_count : MAX_INODES;
        for (u32 ino = 1; ino <= sweep; ++ino) {
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if ((ino & 0xFFF) == 0 && progress)
                (void)progress(ino, sweep, progress_user);
            if (ctx.visited_inodes.count(ino))
                continue; /* emitted in the live tree */
            InodeView iv;
            if (!read_inode(v, ino, iv) || iv.mode == 0)
                continue;
            if (iv.dtime == 0 && iv.links != 0)
                continue; /* live but unreferenced (orphan-reserved) - skip */
            if (ifmt(iv.mode) != 0x8000 && ifmt(iv.mode) != 0x4000)
                continue;

            ++stats.deleted_inodes;
            const bool is_dir = ifmt(iv.mode) == 0x4000;
            auto* props = new (std::nothrow) tree::FileProperties();
            if (!props)
                return ERR_NO_MEMORY;
            /* display name: unknown -> inode-based (og names journal
             * children by their on-disk names when found; sweep nodes get
             * "InodeNNNN" with the journal pass supplying real names) */
            char buf[32];
            snprintf(buf, sizeof(buf), "Inode%u%s", ino, is_dir ? "" : "");
            std::u16string nm;
            for (const char* p = buf; *p; ++p)
                nm.push_back(u16(u8(*p)));
            props->name = nm;
            props->fs_kind = u32(FsKindCode::Ext4);
            props->logical_size = iv.size;
            props->attributes = is_dir ? 0x10 : 0;
            props->times.created = unix_to_filetime_ns(iv.crtime ? iv.crtime : iv.ctime,
                                                       iv.crtime ? iv.crtime_nsec : iv.ctime_nsec);
            props->times.modified = unix_to_filetime_ns(iv.mtime, iv.mtime_nsec);
            props->times.changed = unix_to_filetime_ns(iv.ctime, iv.ctime_nsec);
            props->times.accessed = unix_to_filetime_ns(iv.atime, iv.atime_nsec);
            for (const auto& e : iv.extents)
                props->extents.push_back({v.base + e.offset, e.length});
            props->record_ref = ino;
            tree::FileNode* n = tree.add_file(root_deleted, props);
            props->release();
            if (!n)
                return ERR_NO_MEMORY;
            n->set_deleted_state(tree::DeletedState::Deleted);
            n->set_origin(tree::Origin::Structured);
            /* bitmap grading: extents still free = likely intact */
            bool all_free = !iv.extents.empty();
            bool any_free = false;
            for (const auto& e : iv.extents) {
                const u64 b0 = e.offset / v.sb.block_size;
                const u64 b1 = (e.offset + e.length - 1) / v.sb.block_size;
                bool free_range = true;
                for (u64 b = b0; b <= b1 && b - b0 < 64; ++b)
                    if (!block_free(b))
                        free_range = false;
                if (free_range)
                    any_free = true;
                else
                    all_free = false;
            }
            if (iv.extents.empty())
                n->set_confidence(tree::Confidence::MetadataOnly);
            else if (all_free)
                n->set_confidence(tree::Confidence::Excellent);
            else if (any_free)
                n->set_confidence(tree::Confidence::PartialRisk);
            else
                n->set_confidence(tree::Confidence::Overwritten);
        }
    }

    /* 5. journal salvage emission (pass 2; the S-004 on-disk-truth
     * documentation lives with the pass-1 harvest above): the og's orphan
     * finalize - every harvested entry the live tree did NOT claim,
     * reparented under reachable parents; see the pass-1 comment for the
     * corpus citations. */
    if (flags.salvage_journal && root_lost) {
        /* S-025 pass 2: emit the harvested entries RE-PARENTED
         * (the og's orphan finalize): a child whose parent chain
         * resolves in the final tree attaches under that node -
         * live parents through the record index, salvaged parents
         * through the local map (chains resolve fixpoint-style);
         * unresolvable chains land flat under the lost root with
         * their real names (the og's iNode%I64d fallback is the
         * SWEEP surface's naming - journal children keep on-disk
         * names). Children the live tree claimed stay there
         * (FUN_180003fb0 reachability: the live claim wins). */
        std::map<u32, tree::FileNode*> salvaged_nodes;
        auto attach_point_for = [&](u32 dir_ino) -> tree::FileNode* {
            auto ln = salvaged_nodes.find(dir_ino);
            if (ln != salvaged_nodes.end())
                return ln->second;
            if (ctx.visited_inodes.count(dir_ino)) {
                tree::FileNode* live = tree.find_by_record(dir_ino);
                if (live)
                    return live;
            }
            return nullptr;
        };
        auto emit_entry = [&](u32 dir_ino, const DirEntry& de) {
            if (ctx.visited_inodes.count(de.inode))
                return; /* live claim wins (the og's reachability) */
            if (salvaged_nodes.count(de.inode))
                return; /* already emitted via another block */
            if (de.inode == 0 || de.name.empty() || de.name.size() < 2)
                return;
            tree::FileNode* attach = attach_point_for(dir_ino);
            if (!attach)
                attach = root_lost; /* unresolvable chain: flat,
                                     * real name kept (S-025) */
            auto* props = new (std::nothrow) tree::FileProperties();
            if (!props)
                return;
            props->name = de.name;
            props->fs_kind = u32(FsKindCode::Ext4);
            props->record_ref = de.inode;
            props->attributes = de.type == 2 ? 0x10 : 0;
            tree::FileNode* n = tree.add_file(attach, props);
            props->release();
            if (n) {
                n->set_origin(tree::Origin::Lost);
                n->set_deleted_state(tree::DeletedState::Deleted);
                n->set_confidence(tree::Confidence::PartialRisk);
                salvaged_nodes[de.inode] = n;
                ++stats.journal_salvaged;
            }
        };
        /* fixpoint: parents first (chains), then the rest flat -
         * iterate until no new attachment resolves (bounded by
         * the entry count; every round either emits or stabilizes) */
        bool progressed = true;
        size_t rounds = 0;
        const size_t max_rounds = journal_dirs.size() + 2;
        while (progressed && rounds < max_rounds) {
            progressed = false;
            ++rounds;
            for (const auto& kv : journal_dirs) {
                const u32 d = kv.first;
                if (!attach_point_for(d) && !ctx.visited_inodes.count(d))
                    continue; /* this dir's attach point may resolve
                               * in a later round */
                for (const auto& de : kv.second.entries)
                    emit_entry(d, de);
                progressed = true;
            }
        }
        /* the remainder: owning dirs that never resolved - their
         * entries still emit (flat under the lost root, the og's
         * unreferenced-orphan placement with real names kept) */
        for (const auto& kv : journal_dirs) {
            for (const auto& de : kv.second.entries)
                emit_entry(kv.first, de);
        }
        /* owner-less remnant entries (S-004's "."-less blocks): flat under
         * the lost root - the og's unlinked-dir surface, unchanged */
        for (const auto& de : journal_orphans) {
            if (ctx.visited_inodes.count(de.inode))
                continue; /* live claim wins */
            if (salvaged_nodes.count(de.inode))
                continue;
            if (de.inode == 0 || de.name.empty() || de.name.size() < 2)
                continue;
            auto* props = new (std::nothrow) tree::FileProperties();
            if (!props)
                break;
            props->name = de.name;
            props->fs_kind = u32(FsKindCode::Ext4);
            props->record_ref = de.inode;
            props->attributes = de.type == 2 ? 0x10 : 0;
            tree::FileNode* n = tree.add_file(root_lost, props);
            props->release();
            if (n) {
                n->set_origin(tree::Origin::Lost);
                n->set_deleted_state(tree::DeletedState::Deleted);
                n->set_confidence(tree::Confidence::PartialRisk);
                salvaged_nodes[de.inode] = n;
                ++stats.journal_salvaged;
            }
        }
    }

    return OK;
}

} // namespace ext4
} // namespace cr
