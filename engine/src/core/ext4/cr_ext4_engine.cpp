/* cr_ext4_engine.cpp - ext2/3/4 scan engine.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ext4/cr_ext4_engine.h"
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
constexpr u32 SB_INODE_SIZE = 0x58; /* u16; s_first_ino u32 owns 0x54..0x57 */
constexpr u16 SB_MAGIC_VALUE = 0xEF53;

constexpr u32 INCOMPAT_64BIT = 0x0080;
constexpr u32 INCOMPAT_FILETYPE = 0x0002;
constexpr u32 INCOMPAT_EXTENTS = 0x0040;
constexpr u32 INCOMPAT_BIGALLOC = 0x0400;
constexpr u32 RO_COMPAT_METADATA_CSUM = 0x0400;

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

u16 rd16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

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

u64 unix_to_filetime(i64 unix_sec) {
    /* unix 1970 -> 1601 epoch: 11644473600 seconds; 100ns units */
    const i64 EPOCH_DELTA = 11644473600LL;
    if (unix_sec <= -EPOCH_DELTA)
        return 0;
    return u64((unix_sec + EPOCH_DELTA) * 10000000LL);
}

u64 unix_to_filetime_ns(i64 unix_sec, u32 nsec) {
    /* same conversion with the ext4 *_extra nsec field folded in
     * (kernel ext4_decode_extra_time: nsec = (extra & 0xFFFFFFFC) >> 2) */
    return unix_to_filetime(unix_sec) + u64(nsec / 100u);
}

bool validate_superblock(const u8 sb[1024], SuperBlock& out) {
    if (rd16(sb + SB_MAGIC) != SB_MAGIC_VALUE)
        return false;
    if ((sb[SB_INCOMPAT] & 1) != 0)
        return false; /* compression unsupported (decompile low-bit guard) */
    const u32 inodes_count = rd32(sb);
    const u32 blocks_lo = rd32(sb + SB_BLOCKS_LO);
    const u32 bpg = rd32(sb + SB_BLOCKS_PER_GROUP);
    const u32 ipg = rd32(sb + SB_INODES_PER_GROUP);
    if (inodes_count == 0 || blocks_lo == 0 || bpg == 0 || ipg == 0)
        return false;
    const u32 first_data_block = rd32(sb + SB_FIRST_DATA_BLOCK);
    const u32 log_bs = rd32(sb + SB_LOG_BLOCK_SIZE);
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
    const u64 blocks = blocks_lo | (u64(rd32(sb + SB_BLOCKS_HI)) << 32);
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
    out.rev_level = u16(rd32(sb + SB_REV_LEVEL));
    out.inode_size = out.rev_level >= 1 ? rd16(sb + SB_INODE_SIZE) : 128;
    if (out.inode_size < 128 || out.inode_size > 1024 || (out.inode_size & 7) != 0)
        out.inode_size = 128;
    out.feature_incompat = rd32(sb + SB_INCOMPAT);
    out.feature_ro_compat = rd32(sb + SB_RO_COMPAT);
    out.journal_inum = rd32(sb + SB_JOURNAL_INUM);
    if (out.journal_inum == 0)
        out.journal_inum = JOURNAL_INODE_DEFAULT;
    out.groups = groups;
    if ((out.feature_incompat & INCOMPAT_64BIT) != 0) {
        out.desc_size = rd16(sb + SB_DESC_SIZE);
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

    /* GDT bytes (raw) - descriptors are indexed per group */
    std::vector<u8> gdt;
    u64 gdt_block = 0;

    GroupDesc group(u64 g) const {
        GroupDesc d{};
        if (g >= sb.groups || gdt.empty())
            return d;
        const u8* p = gdt.data() + size_t(g) * sb.desc_size;
        d.block_bitmap = rd32(p + 0x00);
        d.inode_bitmap = rd32(p + 0x04);
        d.inode_table = rd32(p + 0x08);
        if (sb.desc_size >= 64) { /* 64-bit high words (ext4_group_desc):
                                   * block_bitmap_hi@0x20, inode_bitmap_hi
                                   * @0x24, inode_table_hi@0x28 */
            d.block_bitmap |= u64(rd32(p + 0x20)) << 32;
            d.inode_bitmap |= u64(rd32(p + 0x24)) << 32;
            d.inode_table |= u64(rd32(p + 0x28)) << 32;
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
    if (rd16(node) != EXT4_EXT_MAGIC)
        return false;
    const u16 entries = rd16(node + 2);
    if (u32(entries) > (v.sb.block_size - 12) / 12) /* per-node entry bound */
        return false; /* P0-5: bound must follow the actual block size */
    if (level > EXTENT_DEPTH_LIMIT)
        return false;
    const u8* body = node + 12;
    if (depth == 0) {
        for (u32 i = 0; i < entries; ++i) {
            const u8* e = body + u64(i) * 12;
            const u32 logical = rd32(e);
            /* kernel ext4_extents.h: raw ee_len == 0x8000 (EXT_INIT_MAX_LEN)
             * is a WRITTEN 32768-block extent; > 0x8000 means unwritten
             * (bit 15) - decode accordingly */
            const u16 raw_len = rd16(e + 4);
            const u32 len = raw_len > 0x8000 ? (raw_len & 0x7FFF) : raw_len;
            const u64 phys = rd32(e + 8) | (u64(rd16(e + 6)) << 32);
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
        const u64 child = rd32(e + 4) | (u64(rd16(e + 6)) << 32);
        if (child == 0 || child >= v.sb.blocks_count)
            continue;
        std::vector<u8> buf(size_t(v.sb.block_size));
        if (v.dev->read_at(child * v.sb.block_size, buf.data(), u32(buf.size())) != OK)
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
    const u64 table_off = gd.inode_table * v.sb.block_size + u64(index) * v.sb.inode_size;
    u8 raw[1024] = {}; /* s_inode_size can be up to the block size */
    if (v.sb.inode_size > sizeof(raw))
        return false;
    if (v.dev->read_at(table_off, raw, v.sb.inode_size) != OK)
        return false;

    iv = InodeView{};
    iv.mode = rd16(raw + I_MODE);
    iv.atime = i64(rd32(raw + I_ATIME));
    iv.ctime = i64(rd32(raw + I_CTIME));
    iv.mtime = i64(rd32(raw + I_MTIME));
    iv.dtime = i64(rd32(raw + I_DTIME));
    iv.links = rd16(raw + I_LINKS);
    iv.flags = rd32(raw + I_FLAGS);
    u64 size = rd32(raw + I_SIZE_LO);
    if (ifmt(iv.mode) == 0x8000) /* regular: 64-bit size */
        size |= u64(rd32(raw + I_SIZE_HIGH)) << 32;
    iv.size = size;
    /* extended fields present when the inode is larger than the 128-byte
     * good-old layout and i_extra_isize covers them (kernel rule). */
    if (v.sb.inode_size >= 0x98u && v.sb.rev_level >= 1) {
        const u32 extra_isize = rd16(raw + I_EXTRA_ISIZE);
        const u32 covered =
            128u + (extra_isize <= v.sb.inode_size - 128u ? extra_isize : v.sb.inode_size - 128u);
        if (I_ATIME_EXTRA + 4 <= covered)
            iv.atime = decode_extra(iv.atime, rd32(raw + I_ATIME_EXTRA), iv.atime_nsec);
        if (I_CTIME_EXTRA + 4 <= covered)
            iv.ctime = decode_extra(iv.ctime, rd32(raw + I_CTIME_EXTRA), iv.ctime_nsec);
        if (I_MTIME_EXTRA + 4 <= covered)
            iv.mtime = decode_extra(iv.mtime, rd32(raw + I_MTIME_EXTRA), iv.mtime_nsec);
        if (I_CRTIME + 4 <= covered)
            iv.crtime = i64(rd32(raw + I_CRTIME));
        if (I_CRTIME_EXTRA + 4 <= covered)
            iv.crtime = decode_extra(iv.crtime, rd32(raw + I_CRTIME_EXTRA), iv.crtime_nsec);
    }

    u64 budget = 65536; /* P1-11: per-inode node-read budget */
    if ((iv.flags & EXTENTS_FL) != 0) {
        std::vector<u8> node(size_t(v.sb.block_size));
        /* the extent root lives inline in i_block[15] (60 bytes) */
        std::memcpy(node.data(), raw + I_BLOCK, 60);
        const u16 depth = rd16(node.data() + 6);
        if (!walk_extent_node(v, node.data(), depth, budget, iv.extents, 0))
            iv.extents_ok = false;
    } else if (ifmt(iv.mode) == 0x8000 || ifmt(iv.mode) == 0x4000) {
        /* ext2/3 indirect blocks: single + double indirection (files < 64KiB
         * are fully direct; larger get the classic ladder) */
        const u32 per_block = v.sb.block_size / 4;
        const u32 direct = 12;
        for (u32 i = 0; i < direct; ++i) {
            const u32 b = rd32(raw + I_BLOCK + u32(i) * 4);
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
            if (v.dev->read_at(u64(indirection_block) * v.sb.block_size, blk.data(),
                               u32(blk.size())) != OK)
                return false;
            for (u32 i = 0; i < per_block; ++i) {
                const u32 b = rd32(blk.data() + u64(i) * 4);
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
            const u32 b = rd32(raw + I_BLOCK + (11 + lvl) * 4);
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
        if (v.dev->read_at(e.offset, out.data() + done, u32(take)) != OK) {
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

bool parse_dir_block(const u8* p, u64 len, bool filetype, std::vector<DirEntry>& out) {
    u64 off = 0;
    int sane = 0, total = 0;
    while (off + 8 <= len) {
        const u32 ino = rd32(p + off);
        const u16 rec_len = rd16(p + off + 4);
        const u8 name_len = p[off + 6];
        if (rec_len < 8 || (rec_len & 3) != 0 || off + rec_len > len)
            break;
        if (name_len + 8 > rec_len)
            break;
        ++total;
        if (ino != 0 && name_len > 0) {
            ++sane;
            DirEntry de;
            de.inode = ino;
            de.type = filetype ? p[off + 7] : 0;
            for (u32 i = 0; i < name_len; ++i) {
                u16 c = p[off + 8 + i];
                de.name.push_back(c);
            }
            out.push_back(de);
        }
        off += rec_len;
    }
    return total > 0 && sane * 2 >= total; /* majority-valid gate */
}

} // namespace

u32 scan_ext4(IStorageDevice* slice, const Ext4ScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              Ext4ScanStats& stats) {
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    /* 1. superblock */
    u8 sb[1024];
    if (slice->read_at(1024, sb, sizeof(sb)) != OK)
        return ERR_READ_FAILED;
    Volume v;
    if (!validate_superblock(sb, v.sb))
        return ERR_BAD_MAGIC;
    v.dev = slice;
    if ((v.sb.feature_incompat & INCOMPAT_BIGALLOC) != 0)
        return ERR_UNSUPPORTED; /* cluster-granular bitmaps/extents: emitting
                                 * block-granular extents would be WRONG -
                                 * refused loudly, never silently faked */

    /* 2. group descriptors (block first_data_block + 1) */
    v.gdt_block = v.sb.first_data_block + 1;
    const u64 gdt_bytes = v.sb.groups * v.sb.desc_size;
    if (gdt_bytes == 0 || gdt_bytes > (8ull << 20))
        return ERR_BAD_PARAMETER;
    v.gdt.resize(size_t(gdt_bytes));
    if (slice->read_at(v.gdt_block * v.sb.block_size, v.gdt.data(), u32(gdt_bytes)) != OK)
        return ERR_READ_FAILED;
    stats.block_groups = v.sb.groups;

    const bool filetype = (v.sb.feature_incompat & INCOMPAT_FILETYPE) != 0;

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
        if (!parse_dir_block(data.data(), data.size(), ctx.filetype, entries))
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
            props->fs_kind = 9; /* ext family (corpus FS code) */
            props->logical_size = child.size;
            props->attributes = is_dir ? 0x10 : 0;
            props->times.created =
                unix_to_filetime_ns(child.crtime ? child.crtime : child.ctime,
                                    child.crtime ? child.crtime_nsec : child.ctime_nsec);
            props->times.modified = unix_to_filetime_ns(child.mtime, child.mtime_nsec);
            props->times.changed = unix_to_filetime_ns(child.ctime, child.ctime_nsec);
            props->times.accessed = unix_to_filetime_ns(child.atime, child.atime_nsec);
            for (const auto& e : child.extents)
                props->extents.push_back({e.offset, e.length});
            props->record_ref = de.inode;
            tree::FileNode* n = ctx.tree->add_file(parent, props);
            props->release();
            if (!n)
                return ERR_NO_MEMORY;
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
        root_props->fs_kind = 9;
        root_props->attributes = 0x10;
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
                if (gd.block_bitmap == 0 || slice->read_at(gd.block_bitmap * v.sb.block_size,
                                                           bm.data(), u32(bm.size())) != OK)
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
            props->fs_kind = 9;
            props->logical_size = iv.size;
            props->attributes = is_dir ? 0x10 : 0;
            props->times.created = unix_to_filetime_ns(iv.crtime ? iv.crtime : iv.ctime,
                                                       iv.crtime ? iv.crtime_nsec : iv.ctime_nsec);
            props->times.modified = unix_to_filetime_ns(iv.mtime, iv.mtime_nsec);
            props->times.changed = unix_to_filetime_ns(iv.ctime, iv.ctime_nsec);
            props->times.accessed = unix_to_filetime_ns(iv.atime, iv.atime_nsec);
            for (const auto& e : iv.extents)
                props->extents.push_back({e.offset, e.length});
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

    /* 5. journal salvage (jbd2). On-disk truth (include/linux/jbd2.h,
     * fs/jbd2/{journal.c,recovery.c,commit.c}): EVERY journal structure is
     * BIG-ENDIAN. journal_header_t {h_magic BE32, h_blocktype BE32,
     * h_sequence BE32}; blocktypes DESCRIPTOR=1 COMMIT=2 SUPERBLOCK_V1=3
     * SUPERBLOCK_V2=4 REVOKE=5; jsb s_feature_incompat@0x28 with bits
     * REVOKE=0x1, 64BIT=0x2, ASYNC_COMMIT=0x4, CSUM_V2=0x8, CSUM_V3=0x10;
     * tag widths per journal_tag_bytes(): V3 = 16 (flags BE32@+4), V2-csum
     * = 14 with 64BIT / 10 without (flags BE16@+6), plain = 12 / 8; the
     * FIRST tag of each descriptor carries no SAME_UUID -> a 16-byte
     * journal UUID follows it (commit.c jbd2_journal_write_tag). Data
     * blocks follow the descriptor sequentially in the journal; a block
     * whose tag has JBD2_FLAG_ESCAPE had its leading 0xC03B3998 bytes
     * zeroed in the journal copy - replay restores cpu_to_be32(magic).
     *
     * D-07 RE-PINNED (AUDIT-LINUX-FS E-2; EXT4SearchDLL FUN_18000e5b0 +
     * FUN_18000e890, decompiled :12159-12368): the og gates on the jsb
     * magic + blocktype {3,4} and then feeds EVERY block of the journal
     * range to the directory-block parser ("GotJournalDir /
     * TraverseJournalDir" traces) - it implements NO descriptor/commit/
     * revoke transaction semantics; a REVOKE block simply fails the
     * dir-parse. We match that salvage surface: every descriptor-referenced
     * data block is parsed, committed or not (the old commit gate dropped
     * uncommitted-tail dir blocks the og surfaces). Revoked-target
     * filtering is not applied either - the live-tree visit set still
     * filters entries that survived, and stale dir content is exactly
     * what a recovery product wants (stale-snapshot salvage, now
     * og-exact rather than a documented divergence). */
    if (flags.salvage_journal && root_lost) {
        InodeView jiv;
        if (read_inode(v, v.sb.journal_inum, jiv) && !jiv.extents.empty()) {
            std::vector<u8> jdata;
            read_extent_data(v, jiv, 64ull << 20, jdata);
            stats.journal_blocks = jdata.size() / v.sb.block_size;

            constexpr u32 JBD2_MAGIC = 0xC03B3998u;
            constexpr u32 JBD2_DESCRIPTOR_BLOCK = 1;
            constexpr u32 JBD2_COMMIT_BLOCK = 2;
            constexpr u32 JBD2_SUPERBLOCK_V1 = 3;
            constexpr u32 JBD2_SUPERBLOCK_V2 = 4;
            constexpr u32 JBD2_REVOKE_BLOCK = 5;
            constexpr u32 JBD2_FLAG_ESCAPE = 1;
            constexpr u32 JBD2_FLAG_SAME_UUID = 2;
            constexpr u32 JBD2_FLAG_LAST_TAG = 8;
            constexpr u32 JBD2_INCOMPAT_64BIT = 0x2;
            constexpr u32 JBD2_INCOMPAT_CSUM_V2 = 0x8;
            constexpr u32 JBD2_INCOMPAT_CSUM_V3 = 0x10;
            const u64 jblocks = stats.journal_blocks;
            const u64 bs = v.sb.block_size;

            /* jbd2 reads are BIG-endian */
            auto jbe32 = [&](u64 blk, u32 off) -> u32 {
                if (blk >= jblocks || blk * bs + off + 4 > jdata.size())
                    return 0;
                const u8* p = jdata.data() + blk * bs + off;
                return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
            };
            auto jbe16 = [&](u64 at, u32 off) -> u32 {
                if (at + off + 2 > jdata.size())
                    return 0;
                const u8* p = jdata.data() + at + off;
                return (u32(p[0]) << 8) | u32(p[1]);
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

            /* journal superblock at block 0 (type 3 or 4 - jbd2 journal_load
             * accepts both): s_feature_incompat@0x28 decides the tag width */
            u32 tag_size = 8;  /* plain: {blocknr, flags} */
            u32 flags_off = 4; /* t_flags position in the tag */
            bool v3_csum = false;
            {
                u32 seq = 0;
                if (is_header(0, JBD2_SUPERBLOCK_V1, seq) ||
                    is_header(0, JBD2_SUPERBLOCK_V2, seq)) {
                    const u32 incompat = jbe32(0, 0x28);
                    v3_csum = (incompat & JBD2_INCOMPAT_CSUM_V3) != 0;
                    if (v3_csum) {
                        tag_size = 16; /* journal_block_tag3_t */
                        flags_off = 4; /* t_flags BE32@+4 */
                    } else {
                        /* journal_block_tag_t: {blocknr BE32, checksum BE16,
                         * flags BE16@+6, blocknr_high BE32@+8} = 12 with
                         * csum/64BIT, 12-4 without 64BIT, +2 with CSUM_V2 */
                        tag_size = 12;
                        flags_off = 6;
                        if (incompat & JBD2_INCOMPAT_64BIT)
                            tag_size = 12;
                        else
                            tag_size = 8;
                        if (incompat & JBD2_INCOMPAT_CSUM_V2)
                            tag_size += 2;
                    }
                }
            }

            /* pass 1: descriptors -> tags (journal-sequential data blocks).
             * Commit blocks only mark txn boundaries - the og has NO commit
             * gating (E-2) and neither do we now. */
            struct Txn {
                u64 desc_block;
                u32 sequence;
                std::vector<std::pair<u64, bool>> data_blocks; /* {jblock, escaped} */
            };
            std::vector<Txn> txns;
            {
                u64 blk = 0;
                while (blk < jblocks) {
                    u32 seq = 0;
                    if (is_header(blk, JBD2_DESCRIPTOR_BLOCK, seq)) {
                        Txn t;
                        t.desc_block = blk;
                        t.sequence = seq;
                        u64 off = blk * bs + 12; /* after journal_header_t */
                        const u64 end = (blk + 1) * bs;
                        u64 expect = blk + 1;
                        bool first_tag = true;
                        bool last = false;
                        while (off + tag_size <= end && !last) {
                            const u32 fl = jbe16(off, flags_off);
                            bool escaped = (fl & JBD2_FLAG_ESCAPE) != 0;
                            last = (fl & JBD2_FLAG_LAST_TAG) != 0;
                            t.data_blocks.push_back({expect, escaped});
                            off += tag_size;
                            if (first_tag && (fl & JBD2_FLAG_SAME_UUID) == 0) {
                                /* first tag without SAME_UUID: a 16-byte
                                 * journal UUID follows the tag bytes
                                 * (commit.c jbd2 write order: tag, uuid) */
                                if (off + 16 > end)
                                    break;
                                off += 16;
                            }
                            first_tag = false;
                            ++expect;
                        }
                        txns.push_back(std::move(t));
                    } else if (is_header(blk, JBD2_COMMIT_BLOCK, seq)) {
                        /* boundary only - no gating (E-2) */
                    } else if (is_header(blk, JBD2_REVOKE_BLOCK, seq)) {
                        /* revoke blocks are not dir content - they fail the
                         * dir-parse in the og too; nothing to do */
                    }
                    ++blk;
                }
            }

            /* pass 2: salvage directory entries from EVERY descriptor's data
             * blocks (committed or not - og parity, E-2) */
            std::set<std::pair<u32, std::u16string>> seen;
            for (const Txn& t : txns) {
                if (cancel && cancel->checkpoint())
                    break;
                for (const auto& [jb, escaped] : t.data_blocks) {
                    if (jb >= jblocks)
                        break;
                    u8* raw = jdata.data() + jb * bs;
                    /* escaped block: the tag says the first 4 bytes were
                     * zeroed because they collided with the journal magic
                     * (recovery.c jbd2_journal_recover... restores
                     * cpu_to_be32(JBD2_MAGIC) = bytes C0 3B 39 98) */
                    std::vector<u8> unescaped;
                    const u8* blk = raw;
                    if (escaped) {
                        unescaped.assign(raw, raw + bs);
                        unescaped[0] = 0xC0;
                        unescaped[1] = 0x3B;
                        unescaped[2] = 0x39;
                        unescaped[3] = 0x98;
                        blk = unescaped.data();
                    }
                    std::vector<DirEntry> entries;
                    if (!parse_dir_block(blk, bs, true, entries))
                        continue;
                    for (const auto& de : entries) {
                        if (de.inode == 0 || de.name.empty() || de.name.size() < 2)
                            continue;
                        if (!seen.insert({de.inode, de.name}).second)
                            continue;
                        if (ctx.visited_inodes.count(de.inode))
                            continue; /* already in the live tree */
                        auto* props = new (std::nothrow) tree::FileProperties();
                        if (!props)
                            break;
                        props->name = de.name;
                        props->fs_kind = 9;
                        props->record_ref = de.inode;
                        props->attributes = de.type == 2 ? 0x10 : 0;
                        tree::FileNode* n = tree.add_file(root_lost, props);
                        props->release();
                        if (n) {
                            n->set_origin(tree::Origin::Lost);
                            n->set_deleted_state(tree::DeletedState::Deleted);
                            n->set_confidence(tree::Confidence::PartialRisk);
                            ++stats.journal_salvaged;
                        }
                    }
                }
            }
        }
    }

    return OK;
}

} // namespace ext4
} // namespace cr
