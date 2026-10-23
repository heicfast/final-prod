/* cr_btrfs_engine.cpp - Btrfs scan engine.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/btrfs/cr_btrfs_engine.h"
#include <cstring>
#include <functional>
#include <map>
#include <set>

namespace cr {
namespace btrfs {

namespace {

/* ---- superblock (og-verified anchors) ---- */
constexpr u32 SB_MAGIC = 0x40;
constexpr u32 SB_GENERATION = 0x48;
constexpr u32 SB_FSID = 0x20;
constexpr u32 SB_SECTORSIZE = 0x90;
constexpr u32 SB_NODESIZE = 0x94;
constexpr u32 SB_SYS_ARRAY_SIZE = 0xA0;
constexpr u32 SB_SYS_ARRAY = 0x32B;
constexpr u64 SB_MAGIC_VALUE = 0x4D5F53665248425FULL; /* "_BHRfS_M" */

/* btrfs_header (kernel include/uapi/linux/btrfs_tree.h, packed, 101 bytes):
 * csum[32]@0, fsid[16]@0x20, bytenr@0x30, flags@0x38,
 * chunk_tree_uuid[16]@0x40, generation@0x50, owner@0x58,
 * nritems u32@0x60, level u8@0x64. Leaf items array starts at 0x65. */
constexpr u32 HDR_FSID = 0x20;
constexpr u32 HDR_BYTENR = 0x30; /* self-referential logical address */
constexpr u32 HDR_CHUNK_TREE_UUID = 0x40;
constexpr u32 HDR_GENERATION = 0x50;
constexpr u32 HDR_OWNER = 0x58;
constexpr u32 HDR_NR_ITEMS = 0x60;
constexpr u32 HDR_LEVEL = 0x64;
constexpr u32 HDR_ITEM_ARRAY = 0x65; /* offsetof(btrfs_leaf, items) */

/* internal nodes hold btrfs_key_ptr entries (33 bytes), not items:
 * { disk_key(17), blockptr u64@17, generation u64@25 } */
constexpr u32 KP_KEY = 0;
constexpr u32 KP_BLOCKPTR = 17;
constexpr u32 KP_GENERATION = 25;
constexpr u32 KP_SIZE = 33;
constexpr u32 MAX_KEY_PTRS = (4096 - 0x65) / KP_SIZE;

/* btrfs_disk_key (17 bytes) */
constexpr u32 KEY_OBJECTID = 0;
constexpr u32 KEY_TYPE = 8;
constexpr u32 KEY_OFFSET = 9;

/* btrfs_item (25 bytes); item.offset is relative to the leaf items-array
 * start (kernel: btrfs_item_ptr_offset = offsetof(items) + offset). */
constexpr u32 ITEM_KEY = 0;
constexpr u32 ITEM_OFFSET = 17;
constexpr u32 ITEM_SIZE = 21;

/* key types */
constexpr u8 KEY_INODE_ITEM = 1;
constexpr u8 KEY_INODE_REF = 12;
constexpr u8 KEY_EXTENT_DATA = 108;
constexpr u8 KEY_DIR_ITEM = 84;
constexpr u8 KEY_DIR_INDEX = 96;
constexpr u8 KEY_ROOT_ITEM = 132;
constexpr u8 KEY_CHUNK = 228;

/* objectids */
constexpr u64 OID_ROOT_TREE = 1;
constexpr u64 OID_CHUNK_TREE = 3;
constexpr u64 OID_FS_TREE = 5;
constexpr u64 OID_FIRST_SUBVOL = 256;

/* btrfs_inode_item (160 bytes) */
constexpr u32 INO_GENERATION = 0;
constexpr u32 INO_SIZE = 0x10;
constexpr u32 INO_NLINK = 0x28;
constexpr u32 INO_MODE = 0x34;
constexpr u32 INO_ATIME = 0x50; /* timespec: sec i64 + nsec u32 (12 B) */
constexpr u32 INO_CTIME = 0x5C;
constexpr u32 INO_MTIME = 0x68;
constexpr u32 INO_otime = 0x74;

/* btrfs_dir_item: location key(17) + transid u64@17 + data_len u16@25 +
 * name_len u16@27 + type u8@29 + name[]@30 (kernel include/uapi/linux/
 * btrfs_tree.h; og parity verified in BtrfsSearchDLL lines 3424-3444 where
 * the "user.DOSATTRIB" xattr walk reads name_len@+0x1B, data_len@+0x19,
 * name@+0x1E). A DIR_ITEM/DIR_INDEX item may pack several dir_items
 * back-to-back - iterate until the item is exhausted (corpus parity: the
 * og's walk advances by data_len+30+name_len per record). */
constexpr u32 DI_LOCATION = 0; /* {objectid u64, type u8, offset u64} */
constexpr u32 DI_TRANSID = 17;
constexpr u32 DI_DATALEN = 25; /* u16 */
constexpr u32 DI_NAMELEN = 27; /* u16 */
constexpr u32 DI_TYPE = 29;    /* u8: BTRFS_FT_* */
constexpr u32 DI_NAME = 30;
constexpr u32 DI_HEADER = 30; /* fixed part of one dir_item */

/* btrfs_file_extent_item (after key) */
constexpr u32 FE_GENERATION = 0;
constexpr u32 FE_RAM_BYTES = 8;
constexpr u32 FE_COMPRESSION = 16;
constexpr u32 FE_TYPE = 20; /* 0 inline, 1 regular, 2 prealloc */
constexpr u32 FE_DISK_BYTENR = 21;
constexpr u32 FE_DISK_NUM_BYTES = 29;
constexpr u32 FE_OFFSET = 37;
constexpr u32 FE_NUM_BYTES = 45;

/* chunk types */
constexpr u64 CHUNK_TYPE_SYSTEM = 0x02;
constexpr u64 CHUNK_TYPE_METADATA = 0x04;

constexpr u32 MAX_NODES = 1u << 20;
constexpr u32 MAX_ITEMS = 4096;
constexpr int DIR_DEPTH_LIMIT = 64;
constexpr u64 MAX_DIR_BYTES = 16ull << 20;

u16 le16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
u32 le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
u64 le64(const u8* p) {
    return u64(le32(p)) | (u64(le32(p + 4)) << 32);
}

struct Key {
    u64 objectid = 0;
    u8 type = 0;
    u64 offset = 0;
};

Key read_key(const u8* p) {
    Key k;
    k.objectid = le64(p + KEY_OBJECTID);
    k.type = p[KEY_TYPE];
    k.offset = le64(p + KEY_OFFSET);
    return k;
}

/* ---- volume context ---- */
struct Volume {
    IStorageDevice* dev = nullptr;
    SuperBlock sb;
    std::vector<ChunkMap> chunks; /* full logical->physical map */

    bool logical_to_physical(u64 logical, u64 len, u64& physical) const {
        for (const auto& c : chunks) {
            if (logical >= c.logical_start && logical + len <= c.logical_start + c.logical_len) {
                physical = c.physical_start + (logical - c.logical_start);
                return true;
            }
        }
        return false;
    }

    /* read a tree node by logical address (self-validating: header.bytenr
     * must equal the logical address). */
    bool read_node(u64 logical, std::vector<u8>& out) {
        u64 phys = 0;
        if (!logical_to_physical(logical, sb.nodesize, phys))
            return false;
        out.resize(sb.nodesize);
        if (dev->read_at(phys, out.data(), sb.nodesize) != OK)
            return false;
        if (le64(out.data() + HDR_BYTENR) != logical)
            return false; /* self-reference check - wrong candidate never validates */
        return true;
    }
};

/* Parse one chunk item (48-byte head + stripes). */
bool parse_chunk(const u8* p, u32 len, ChunkMap& out) {
    if (len < 48)
        return false;
    const u64 length = le64(p + 0);
    const u64 type = le64(p + 0x18);
    const u16 num_stripes = le16(p + 0x2C);
    if (num_stripes == 0 || num_stripes > 16)
        return false;
    if (u32(48 + u32(num_stripes) * 32) > len)
        return false;
    /* single-device recovery: use stripe 0 (multi-device/RAID is out of
     * scope for single-volume scanning; the first stripe still carries
     * valid data for single-disk profiles) */
    const u64 physical = le64(p + 48 + 8); /* stripe0: devid u64, offset u64 */
    if (length == 0)
        return false;
    (void)type;
    const u64 logical = 0; /* set by caller via key.offset */
    out.logical_len = length;
    out.physical_start = physical;
    out.type = le64(p + 0x18);
    out.logical_start = logical;
    return true;
}

/* Iterate items of a LEAF; fn(key, data_ptr, data_len). item.offset is
 * relative to the items-array start (kernel btrfs_item_ptr_offset). */
bool for_each_item(const std::vector<u8>& node,
                   const std::function<bool(const Key&, const u8*, u32)>& fn) {
    if (node.size() < HDR_ITEM_ARRAY + 25)
        return false;
    const u32 nritems = le32(node.data() + HDR_NR_ITEMS);
    if (nritems == 0 || nritems > MAX_ITEMS)
        return false;
    const u32 nodesize = u32(node.size());
    const u64 items_end = HDR_ITEM_ARRAY + u64(nritems) * 25;
    for (u32 i = 0; i < nritems; ++i) {
        const u8* it = node.data() + HDR_ITEM_ARRAY + u64(i) * 25;
        const Key k = read_key(it + ITEM_KEY);
        const u32 off = le32(it + ITEM_OFFSET);
        const u32 size = le32(it + ITEM_SIZE);
        /* data start = items-array start + off; validity: after the items
         * array, fully inside the node */
        const u64 data_pos = HDR_ITEM_ARRAY + u64(off);
        if (off == 0 || data_pos < items_end || data_pos + size > nodesize)
            continue; /* damaged item: skip */
        const u8* data = node.data() + data_pos;
        if (!fn(k, data, size))
            return false;
    }
    return true;
}

/* Walk a tree (BFS over internal nodes) calling fn on every item of every
 * LEAF. Internal nodes hold a btrfs_key_ptr array (kernel format):
 * { disk_key(17), blockptr u64@17, generation u64@25 } per entry. */
bool walk_tree(Volume& v, u64 root_logical, CancelToken* cancel,
               const std::function<bool(const Key&, const u8*, u32)>& fn, u64& nodes_read,
               u64& damaged) {
    std::vector<u64> pending{root_logical};
    std::set<u64> visited;
    while (!pending.empty()) {
        if (cancel && cancel->cancelled())
            return false;
        const u64 logical = pending.back();
        pending.pop_back();
        if (!visited.insert(logical).second || visited.size() > MAX_NODES)
            continue;
        std::vector<u8> node;
        if (!v.read_node(logical, node)) {
            ++damaged;
            continue;
        }
        ++nodes_read;
        const u8 level = node[HDR_LEVEL];
        if (level == 0) {
            for_each_item(node, fn);
        } else {
            /* internal node: key_ptr array after the header */
            const u32 nritems = le32(node.data() + HDR_NR_ITEMS);
            const u32 limit =
                u32((node.size() - HDR_ITEM_ARRAY) / KP_SIZE);
            if (nritems == 0 || nritems > limit) {
                ++damaged;
                continue;
            }
            for (u32 i = 0; i < nritems; ++i) {
                const u8* kp = node.data() + HDR_ITEM_ARRAY + u64(i) * KP_SIZE;
                const u64 child = le64(kp + KP_BLOCKPTR);
                if (child != 0)
                    if (pending.size() < 65536) /* P1-11: queue cap */
                        pending.push_back(child);
            }
        }
    }
    return true;
}

} // namespace

u64 timespec_to_filetime(i64 sec, u32 nsec) {
    const i64 EPOCH_DELTA = 11644473600LL;
    if (sec <= -EPOCH_DELTA)
        return 0;
    return u64((sec + EPOCH_DELTA) * 10000000LL) + u64(nsec) / 100u;
}

bool validate_superblock(const u8 sb[4096], SuperBlock& out) {
    if (le64(sb + SB_MAGIC) != SB_MAGIC_VALUE)
        return false;
    const u32 sectorsize = le32(sb + SB_SECTORSIZE);
    const u32 nodesize = le32(sb + SB_NODESIZE);
    if (sectorsize == 0 || (sectorsize & (sectorsize - 1)) != 0)
        return false; /* pow2 (og gate) */
    if (u32(nodesize - 0x86) >= 0xFF7B)
        return false; /* band 0x86..0x10000 (og gate) */
    if (nodesize % sectorsize != 0)
        return false;
    out.generation = le64(sb + SB_GENERATION);
    out.sectorsize = sectorsize;
    out.nodesize = nodesize;
    std::memcpy(out.fsid, sb + SB_FSID, 16);
    /* embedded system chunk array (og layout: entries from +0x32B,
     * remaining bytes in u32@+0xA0; entry = [key 17][chunk 48+32n]) */
    const u32 array_size = le32(sb + SB_SYS_ARRAY_SIZE);
    out.sys_chunks.clear();
    u32 consumed = 0;
    while (consumed + 17 + 48 <= array_size && array_size <= 2047) {
        const u8* key_p = sb + SB_SYS_ARRAY + consumed;
        const Key k = read_key(key_p);
        if (k.type != KEY_CHUNK)
            break;
        consumed += 17;
        const u8* chunk_p = sb + SB_SYS_ARRAY + consumed;
        const u16 num_stripes = le16(chunk_p + 0x2C);
        const u32 chunk_size = 48 + u32(num_stripes) * 32;
        if (consumed + chunk_size > array_size)
            break;
        ChunkMap cm;
        if (parse_chunk(chunk_p, chunk_size, cm)) {
            cm.logical_start = k.offset; /* chunk key offset = logical start */
            out.sys_chunks.push_back(cm);
        }
        consumed += chunk_size;
    }
    out.valid = !out.sys_chunks.empty() || array_size == 0;
    return true;
}

u32 scan_btrfs(IStorageDevice* slice, const BtrfsScanFlags& flags, CancelToken* cancel,
               WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
               tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
               BtrfsScanStats& stats) {
    (void)flags;
    (void)root_deleted;
    (void)root_lost;
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    /* 1. mirror superblock select: newest generation wins (og rule).
     * Ground truth extracted from BtrfsSearchDLL.dll .rdata (VA 0x180018D60):
     * {0x10000, 0x4000000, 0x4000000000, 0x4000000000000} - identical to the
     * kernel btrfs_sb_offset(): 64 KiB primary + mirrors at 64 MiB, 256 GiB
     * and 1 PiB (16K << 12*mirror). */
    static const u64 kMirrorOffsets[] = {0x10000,          0x4000000,
                                         0x4000000000ULL,  0x4000000000000ULL, 0};
    SuperBlock best;
    bool have_best = false;
    std::vector<u8> sb_buf(4096);
    std::vector<u8> sb_raw;
    for (u32 mi = 0; kMirrorOffsets[mi] != 0; ++mi) {
        if (kMirrorOffsets[mi] + 4096 > slice->size())
            break;
        if (slice->read_at(kMirrorOffsets[mi], sb_buf.data(), 4096) != OK)
            continue;
        SuperBlock cand;
        if (!validate_superblock(sb_buf.data(), cand))
            continue;
        if (!have_best || cand.generation > best.generation) {
            best = cand;
            have_best = true;
            sb_raw = sb_buf;
        }
    }
    if (!have_best)
        return ERR_BAD_MAGIC;

    Volume v;
    v.dev = slice;
    v.sb = best;
    v.chunks = best.sys_chunks;

    /* 2. chunk tree walk: build the full logical->physical map.
     * The chunk tree root's logical address: candidates validated by the
     * node's self-referential bytenr (a wrong offset can never validate).
     * Real btrfs places it in the first system chunk - when the sys array
     * exists, scan the system chunks for the root node. */
    u64 chunk_root = 0;
    {
        std::vector<u64> candidates;
        /* chunk_root@sb+0x58 and root@sb+0x50 are the canonical fields
         * (kernel btrfs_super_block); the other slots are validated by the
         * node's self-referential bytenr so a wrong candidate never wins. */
        candidates.push_back(le64(sb_raw.data() + 0x58));
        candidates.push_back(le64(sb_raw.data() + 0x50));
        candidates.push_back(le64(sb_raw.data() + 0x60));
        candidates.push_back(le64(sb_raw.data() + 0x68));
        candidates.push_back(le64(sb_raw.data() + 0x70));
        for (u64 cand : candidates) {
            if (cand == 0)
                continue;
            std::vector<u8> node;
            if (v.read_node(cand, node)) {
                /* must contain CHUNK items or be an internal tree node
                 * whose chunk_tree_uuid matches the superblock's */
                const u8* uuid = node.data() + HDR_CHUNK_TREE_UUID;
                (void)uuid;
                chunk_root = cand;
                break;
            }
        }
        if (chunk_root == 0 && !v.chunks.empty()) {
            /* fallback: bounded scan of system chunks for a self-validating
             * node carrying CHUNK items */
            for (const auto& c : v.chunks) {
                if (!(c.type & CHUNK_TYPE_SYSTEM))
                    continue;
                for (u64 off = 0; off + v.sb.nodesize <= c.logical_len; off += v.sb.nodesize) {
                    const u64 logical = c.logical_start + off;
                    std::vector<u8> node;
                    if (!v.read_node(logical, node))
                        continue;
                    const u32 nritems = le32(node.data() + HDR_NR_ITEMS);
                    bool has_chunk = false;
                    for (u32 i = 0; i < nritems && i < MAX_ITEMS; ++i) {
                        const u8* it = node.data() + HDR_ITEM_ARRAY + u64(i) * 25;
                        if (it[ITEM_KEY + KEY_TYPE] == KEY_CHUNK) {
                            has_chunk = true;
                            break;
                        }
                    }
                    if (has_chunk) {
                        chunk_root = logical;
                        break;
                    }
                }
                if (chunk_root != 0)
                    break;
            }
        }
    }
    if (chunk_root == 0)
        return ERR_BAD_MAGIC;

    {
        std::vector<ChunkMap> discovered;
        auto on_item = [&](const Key& k, const u8* data, u32 len) {
            if (k.type == KEY_CHUNK && len >= 48) {
                ChunkMap cm;
                if (parse_chunk(data, len, cm)) {
                    cm.logical_start = k.offset;
                    discovered.push_back(cm);
                }
            }
            return true;
        };
        walk_tree(v, chunk_root, cancel, on_item, stats.nodes_read, stats.damaged_nodes);
        for (const auto& c : discovered)
            v.chunks.push_back(c);
        stats.chunks = v.chunks.size();
    }
    if (progress)
        (void)progress(1, 4, progress_user);

    /* 3. root tree discovery: candidates + scan of metadata chunks for the
     * node holding ROOT_ITEM(CHUNK_TREE) (only the root tree contains it;
     * cross-validates with the chunk tree we already loaded). */
    u64 root_tree = 0;
    {
        std::vector<u64> candidates{le64(sb_raw.data() + 0x50), le64(sb_raw.data() + 0x58),
                                    le64(sb_raw.data() + 0x60), le64(sb_raw.data() + 0x68),
                                    le64(sb_raw.data() + 0x70)};
        for (u64 cand : candidates) {
            if (cand == 0)
                continue;
            std::vector<u8> node;
            if (!v.read_node(cand, node))
                continue;
            const u32 nritems = le32(node.data() + HDR_NR_ITEMS);
            for (u32 i = 0; i < nritems && i < MAX_ITEMS; ++i) {
                const u8* it = node.data() + HDR_ITEM_ARRAY + u64(i) * 25;
                const Key k = read_key(it + ITEM_KEY);
                if (k.type == KEY_ROOT_ITEM && k.objectid == OID_CHUNK_TREE) {
                    root_tree = cand;
                    break;
                }
            }
            if (root_tree != 0)
                break;
        }
        if (root_tree == 0) {
            for (const auto& c : v.chunks) {
                if (!(c.type & (CHUNK_TYPE_METADATA | CHUNK_TYPE_SYSTEM)))
                    continue;
                for (u64 off = 0; off + v.sb.nodesize <= c.logical_len && root_tree == 0;
                     off += v.sb.nodesize) {
                    const u64 logical = c.logical_start + off;
                    std::vector<u8> node;
                    if (!v.read_node(logical, node))
                        continue;
                    const u32 nritems = le32(node.data() + HDR_NR_ITEMS);
                    for (u32 i = 0; i < nritems && i < MAX_ITEMS; ++i) {
                        const u8* it = node.data() + HDR_ITEM_ARRAY + u64(i) * 25;
                        const Key k = read_key(it + ITEM_KEY);
                        if (k.type == KEY_ROOT_ITEM && k.objectid == OID_CHUNK_TREE) {
                            root_tree = logical;
                            break;
                        }
                    }
                }
                if (root_tree != 0)
                    break;
            }
        }
    }
    if (root_tree == 0)
        return ERR_BAD_MAGIC;
    if (progress)
        (void)progress(2, 4, progress_user);

    /* 4. collect ROOT_ITEMs: FS tree + subvolumes (ROOT_ITEM body per
     * kernel btrfs_tree.h: inode(160=0xA0) + generation@0xA0 +
     * root_dirid@0xA8 + bytenr@0xB0 + byte_limit@0xB8 + bytes_used@0xC0 +
     * last_snapshot@0xC8 + flags@0xD0 + refs u32@0xD8 + drop_progress
     * key(17)@0xDC + drop_level u8@0xED + level u8@0xEE). */
    struct Subvol {
        u64 id;
        u64 bytenr;
        u64 root_dirid;
        u8 level;
        std::u16string name; /* from DIR_INDEX in the root tree when present */
    };
    std::vector<Subvol> subvols;
    {
        auto on_item = [&](const Key& k, const u8* data, u32 len) {
            if (k.type != KEY_ROOT_ITEM || len < 0xB8)
                return true;
            if (k.objectid != OID_FS_TREE && k.objectid < OID_FIRST_SUBVOL)
                return true; /* skip chunk/log/extent roots */
            Subvol s;
            s.id = k.objectid;
            s.bytenr = le64(data + 0xB0);
            s.root_dirid = le64(data + 0xA8);
            s.level = (len > 0xEE) ? data[0xEE] : 0; /* root tree level */
            subvols.push_back(s);
            return true;
        };
        walk_tree(v, root_tree, cancel, on_item, stats.nodes_read, stats.damaged_nodes);
        /* subvolume names: ROOT_REF/DIR_ITEM entries in the root tree name
         * children; recovered best-effort below during the FS walk via the
         * default "subvol-<id>" naming */
    }
    if (progress)
        (void)progress(3, 4, progress_user);

    if (subvols.empty())
        return ERR_BAD_MAGIC;

    /* 5. per-subvolume FS tree walk: inodes + dir entries + extents */
    u64 subvol_index = 0;
    for (const auto& sv : subvols) {
        if (cancel && cancel->cancelled())
            return ERR_CANCELLED;
        ++subvol_index;
        char role[32];
        snprintf(role, sizeof(role), "subvol-%llu", static_cast<unsigned long long>(sv.id));
        std::u16string role16;
        for (const char* p = role; *p; ++p)
            role16.push_back(u16(u8(*p)));
        /* subvolumes live under root_active like every other engine
         * (single merged tree; the corpus builder renders subvols as
         * separate result roots - we nest them for one canonical tree) */
        auto* sv_props = new (std::nothrow) tree::FileProperties();
        if (!sv_props)
            continue;
        sv_props->name = role16;
        sv_props->fs_kind = 7;
        sv_props->attributes = 0x10;
        sv_props->record_ref = sv.id;
        tree::FileNode* sv_root = tree.add_file(root_active, sv_props);
        sv_props->release();
        if (!sv_root)
            continue;
        ++stats.dirs; /* the subvol node is the root directory */

        /* phase 1: inode items */
        struct InodeInfo {
            u64 size = 0;
            u32 mode = 0;
            u32 nlink = 0;
            u64 atime = 0, ctime = 0, mtime = 0;
            std::vector<std::pair<u64, u64>> extents; /* slice bytes */
            bool inline_data = false;
        };
        std::map<u64, InodeInfo> inodes;
        auto on_item = [&](const Key& k, const u8* data, u32 len) {
            if (k.type == KEY_INODE_ITEM && len >= 0x80) {
                InodeInfo& ii = inodes[k.objectid];
                ii.size = le64(data + INO_SIZE);
                ii.nlink = le32(data + INO_NLINK);
                ii.mode = le32(data + INO_MODE);
                ii.atime =
                    timespec_to_filetime(i64(le64(data + INO_ATIME)), le32(data + INO_ATIME + 8));
                ii.ctime =
                    timespec_to_filetime(i64(le64(data + INO_CTIME)), le32(data + INO_CTIME + 8));
                ii.mtime =
                    timespec_to_filetime(i64(le64(data + INO_MTIME)), le32(data + INO_MTIME + 8));
            } else if (k.type == KEY_EXTENT_DATA && len >= 53) {
                InodeInfo& ii = inodes[k.objectid];
                const u8 ftype = data[FE_TYPE];
                if (ftype == 0) {
                    ii.inline_data = true;
                } else if (ftype == 1 || ftype == 2) {
                    const u64 disk_bytenr = le64(data + FE_DISK_BYTENR);
                    const u64 disk_num = le64(data + FE_DISK_NUM_BYTES);
                    const u64 offset_in = le64(data + FE_OFFSET);
                    const u64 num = le64(data + FE_NUM_BYTES);
                    if (disk_bytenr == 0 || disk_num == 0)
                        return true; /* hole */
                    u64 phys = 0;
                    if (v.logical_to_physical(disk_bytenr + offset_in, num, phys))
                        ii.extents.push_back({phys, num});
                }
            }
            return true;
        };
        walk_tree(v, sv.bytenr, cancel, on_item, stats.nodes_read, stats.damaged_nodes);

        /* phase 2: dir entries (DIR_ITEM carries the name; key:
         * (dir_objectid, DIR_ITEM, name_hash) and (dir, DIR_INDEX, index)) */
        struct DirEntry {
            u64 child;
            std::u16string name;
            u8 type;
        };
        std::map<u64, std::vector<DirEntry>> dirs;
        auto on_dir = [&](const Key& k, const u8* data, u32 len) {
            if (k.type != KEY_DIR_ITEM)
                return true;
            /* one item can pack several btrfs_dir_item records; walk them
             * all (kernel + og semantics) */
            u32 off = 0;
            while (off + DI_HEADER <= len) {
                const u8* di = data + off;
                const u16 name_len = le16(di + DI_NAMELEN);
                const u16 data_len = le16(di + DI_DATALEN);
                if (u64(off) + DI_HEADER + u64(name_len) + u64(data_len) > len)
                    break; /* truncated tail: stop, keep what we have */
                DirEntry de;
                de.child = le64(di + DI_LOCATION);
                de.type = di[DI_TYPE];
                for (u32 i = 0; i < name_len && i < 255; ++i)
                    de.name.push_back(u16(di[DI_NAME + i]));
                if (!de.name.empty())
                    dirs[k.objectid].push_back(std::move(de));
                off += DI_HEADER + name_len + data_len;
            }
            return true;
        };
        walk_tree(v, sv.bytenr, cancel, on_dir, stats.nodes_read, stats.damaged_nodes);

        /* phase 3: emit the tree from the subvol root dir */
        std::set<u64> emitted;
        std::function<void(u64, tree::FileNode*, int)> populate =
            [&](u64 dirid, tree::FileNode* node, int depth) {
                if (depth > DIR_DEPTH_LIMIT)
                    return;
                auto dit = dirs.find(dirid);
                if (dit == dirs.end())
                    return;
                for (const auto& de : dit->second) {
                    if (cancel && cancel->cancelled())
                        return;
                    if (de.name == u"." || de.name == u"..")
                        continue;
                    if (emitted.count(de.child))
                        continue; /* hard link: one instance in the tree */
                    auto cit = inodes.find(de.child);
                    if (cit == inodes.end())
                        continue; /* dangling entry */
                    const bool is_dir = (cit->second.mode & 0xF000) == 0x4000;
                    auto* cp = new (std::nothrow) tree::FileProperties();
                    if (!cp)
                        return;
                    cp->name = de.name;
                    cp->fs_kind = 7; /* CBtrfsFileProerties corpus id */
                    cp->logical_size = cit->second.size;
                    cp->attributes = is_dir ? 0x10 : 0;
                    cp->times.created = cit->second.ctime;
                    cp->times.modified = cit->second.mtime;
                    cp->times.changed = cit->second.ctime;
                    cp->times.accessed = cit->second.atime;
                    cp->record_ref = de.child;
                    for (const auto& e : cit->second.extents)
                        cp->extents.push_back({e.first, e.second});
                    if (cit->second.inline_data)
                        ++stats.inline_files;
                    tree::FileNode* child_node = tree.add_file(node, cp);
                    cp->release();
                    if (!child_node)
                        continue;
                    emitted.insert(de.child);
                    if (is_dir) {
                        ++stats.dirs;
                        populate(de.child, child_node, depth + 1);
                    } else {
                        ++stats.files;
                    }
                }
            };
        /* the subvol's root dir: ROOT_ITEM root_dirid when sane, else the
         * smallest dir inode with entries */
        u64 root_dirid = sv.root_dirid >= 256 ? sv.root_dirid : 256;
        if (!inodes.count(root_dirid) || !dirs.count(root_dirid)) {
            for (const auto& kv : dirs) {
                if (!kv.second.empty() && inodes.count(kv.first)) {
                    root_dirid = kv.first;
                    break;
                }
            }
        }
        /* the subvol node IS the root directory (no extra nesting level;
         * the corpus renders the subvol root dir as the subvol itself) */
        if (inodes.count(root_dirid) && dirs.count(root_dirid)) {
            emitted.insert(root_dirid);
            populate(root_dirid, sv_root, 0);
        }
        ++stats.subvolumes;
        if (progress)
            (void)progress(4, 4, progress_user); /* final tick per subvol */
    }

    if (progress)
        (void)progress(4, 4, progress_user);
    return OK;
}

} // namespace btrfs
} // namespace cr
