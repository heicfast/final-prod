/* cr_btrfs_engine.cpp - Btrfs scan engine.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/btrfs/cr_btrfs_cache.h" /* N-072: the re-scan cache */
#include "cr/cr_bits.h"              /* shared byte readers (W4 dedup, DC-12) */
#include "cr/cr_text.h"              /* S-005: on-disk UTF-8 names decode, not byte-lift */
#include "cr/compress/cr_decomp.h"
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
/* SB+0x60 is log_root in the kernel layout (BTR-2): NOT a uuid source. */
constexpr u64 SB_MAGIC_VALUE = 0x4D5F53665248425FULL; /* "_BHRfS_M" */

/* btrfs_header (kernel include/uapi/linux/btrfs_tree.h, packed, 101 bytes):
 * csum[32]@0, fsid[16]@0x20, bytenr@0x30, flags@0x38,
 * chunk_tree_uuid[16]@0x40, generation@0x50, owner@0x58,
 * nritems u32@0x60, level u8@0x64. Leaf items array starts at 0x65. */
constexpr u32 HDR_FSID = 0x20;
constexpr u32 HDR_BYTENR = 0x30; /* self-referential logical address */
constexpr u32 HDR_CHUNK_TREE_UUID = 0x40;
[[maybe_unused]] constexpr u32 HDR_GENERATION = 0x50;
[[maybe_unused]] constexpr u32 HDR_OWNER = 0x58;
constexpr u32 HDR_NR_ITEMS = 0x60;
constexpr u32 HDR_LEVEL = 0x64;
constexpr u32 HDR_ITEM_ARRAY = 0x65; /* offsetof(btrfs_leaf, items) */

/* internal nodes hold btrfs_key_ptr entries (33 bytes), not items:
 * { disk_key(17), blockptr u64@17, generation u64@25 } */
[[maybe_unused]] constexpr u32 KP_KEY = 0;
constexpr u32 KP_BLOCKPTR = 17;
[[maybe_unused]] constexpr u32 KP_GENERATION = 25;
constexpr u32 KP_SIZE = 33;

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
constexpr u8 KEY_EXTENT_DATA = 108;
constexpr u8 KEY_DIR_ITEM = 84;
constexpr u8 KEY_ROOT_ITEM = 132;
constexpr u8 KEY_ROOT_REF = 156; /* N-047: btrfs_root_ref names child
                                  * subvols (kernel btrfs_tree.h: dirid@0,
                                  * sequence@8, name_len@16, name@18;
                                  * key.objectid = the child subvol id) */
constexpr u8 KEY_CHUNK = 228;
constexpr u8 KEY_XATTR = 24; /* S-028: BTRFS_XATTR_ITEM_KEY (og type 0x18) */

/* objectids */
constexpr u64 OID_CHUNK_TREE = 3;
constexpr u64 OID_FS_TREE = 5;
constexpr u64 OID_ROOT_TREE_DIR = 6; /* M-020: BTRFS_ROOT_TREE_DIR_OBJECTID
                                      * - the root tree dir that holds the
                                      * "default" DIR_ITEM (kernel ctree.h;
                                      * the og reads the objectid from its
                                      * volume context +0x80, kernel value 6) */
constexpr u64 OID_FIRST_SUBVOL = 256;

/* btrfs_inode_item (160 bytes = 0xA0, kernel include/uapi/linux/btrfs_tree.h):
 * generation u64@0, transid@8, size@0x10, nbytes@0x18, block_group@0x20,
 * nlink u32@0x28, uid@0x2C, gid@0x30, mode u32@0x34, rdev@0x38, flags@0x40,
 * sequence@0x48, reserved[4]@0x50 (32 bytes), then the four 12-byte
 * timespecs atime@0x70, ctime@0x7C, mtime@0x88, otime@0x94. */
constexpr u32 INO_GENERATION = 0;
constexpr u32 INO_SIZE = 0x10;
constexpr u32 INO_NLINK = 0x28;
constexpr u32 INO_MODE = 0x34;
constexpr u32 INO_ATIME = 0x70; /* timespec: sec i64 + nsec u32 (12 B) */
constexpr u32 INO_CTIME = 0x7C;
constexpr u32 INO_MTIME = 0x88;
constexpr u32 INO_otime = 0x94;
constexpr u32 INO_MIN = 0xA0; /* through otime */

/* btrfs_dir_item: location key(17) + transid u64@17 + data_len u16@25 +
 * name_len u16@27 + type u8@29 + name[]@30 (kernel include/uapi/linux/
 * btrfs_tree.h; og parity verified in BtrfsSearchDLL lines 3424-3444 where
 * the "user.DOSATTRIB" xattr walk reads name_len@+0x1B, data_len@+0x19,
 * name@+0x1E). A DIR_ITEM/DIR_INDEX item may pack several dir_items
 * back-to-back - iterate until the item is exhausted (corpus parity: the
 * og's walk advances by data_len+30+name_len per record). */
constexpr u32 DI_LOCATION = 0; /* {objectid u64, type u8, offset u64} */
[[maybe_unused]] constexpr u32 DI_TRANSID = 17;
constexpr u32 DI_DATALEN = 25; /* u16 */
constexpr u32 DI_NAMELEN = 27; /* u16 */
constexpr u32 DI_TYPE = 29;    /* u8: BTRFS_FT_* */
constexpr u32 DI_NAME = 30;
constexpr u32 DI_HEADER = 30; /* fixed part of one dir_item */

/* btrfs_file_extent_item (after key) */
[[maybe_unused]] constexpr u32 FE_GENERATION = 0;
[[maybe_unused]] constexpr u32 FE_RAM_BYTES = 8;
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
[[maybe_unused]] constexpr u64 MAX_DIR_BYTES = 16ull << 20;

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
    /* BTR-2: adopted from the chunk-root node header (not the SB -
     * SB+0x60 is log_root in the kernel layout) */
    u8 chunk_tree_uuid[16] = {0};
    bool have_chunk_tree_uuid = false;

    /* M-020: find the chunk covering [logical, logical+len). */
    const ChunkMap* find_chunk(u64 logical, u64 len) const {
        for (const auto& c : chunks)
            if (logical >= c.logical_start && logical - c.logical_start < c.logical_len &&
                c.logical_len - (logical - c.logical_start) >= len)
                return &c;
        return nullptr;
    }

    /* M-020 profile classes - the og's decode at :2498-2543, first-hand:
     * DUP(&0x20), RAID1(&0x10) and SINGLE(no profile bits) all fall into
     * the 0x20 class served by the rotating mirror; RAID0(&8) is class 8;
     * RAID10(&0x40) is class 0x40. The og's read path serves ONLY the
     * 0x20 class (class 8/0x40 fall to LAB_1800043fe = read failure);
     * our RAID0/RAID10 math is the registered superset. */
    static bool class_2copy(u64 t) {
        return (t & (0x20u | 0x10u | 8u | 0x40u | 0x80u | 0x100u)) == 0 || (t & 0x20u) != 0 ||
               (t & 0x10u) != 0;
    }

    /* Map one logical read to its physical target. Single-piece only:
     * on striped profiles the piece must not cross a stripe boundary
     * (the extent path splits first; node reads never span in practice
     * and a spanning request is refused rather than mis-mapped).
     * Non-const: 2-copy/RAID10 access advances the per-chunk rotating
     * mirror state exactly like the og (:2534-2537). */
    bool logical_to_physical(u64 logical, u64 len, u64& physical) {
        const ChunkMap* c = find_chunk(logical, len);
        if (c == nullptr)
            return false;
        const u64 rel = logical - c->logical_start;
        const u64 n = c->stripes.size();
        if (n == 0)
            return false;
        if (class_2copy(c->type)) {
            /* og :2534-2540: index = saved; saved = (index+1) % num_stripes;
             * physical = stripes[index].offset + (logical - chunk_start) */
            ChunkMap& cm = const_cast<ChunkMap&>(*c);
            const u64 idx = cm.mirror_index % n;
            cm.mirror_index = u16((cm.mirror_index + 1) % n);
            if (idx != 0)
                ++mirror_rotations;
            physical = c->stripes[idx].offset + rel;
            return true;
        }
        if ((c->type & (8u | 0x40u)) == 0)
            return false; /* classes &0x80/&0x100: refused like the og's failing read path */
        if (c->stripe_len == 0)
            return false;
        if (rel / c->stripe_len != (rel + len - 1) / c->stripe_len)
            return false; /* crosses a stripe boundary - split first */
        const u64 stripe_nr = rel / c->stripe_len;
        const u64 in_stripe = rel % c->stripe_len;
        ChunkMap& cm = const_cast<ChunkMap&>(*c);
        if (c->type & 8u) {
            /* RAID0 (REGISTERED SUPERSET - the og fails class 8): kernel
             * stripe math - device i holds every n-th stripe unit. */
            const u64 idx = stripe_nr % n;
            physical = c->stripes[idx].offset + (stripe_nr / n) * c->stripe_len + in_stripe;
            return true;
        }
        /* RAID10 (REGISTERED SUPERSET - the og fails class 0x40): mirror
         * sets of sub_stripes; any member of the correct set holds the
         * bytes (read-only recovery), the member rotates og-style. The
         * parse gate guarantees sub_stripes >= 1 and n >= sub_stripes. */
        const u64 s = c->sub_stripes ? c->sub_stripes : 1;
        const u64 sets = n / s;
        if (sets == 0)
            return false;
        const u64 set = (stripe_nr % sets) * s;
        const u64 member = cm.mirror_index % s;
        cm.mirror_index = u16((cm.mirror_index + 1) % s);
        if (member != 0)
            ++mirror_rotations;
        physical = c->stripes[set + member].offset + (stripe_nr / sets) * c->stripe_len + in_stripe;
        return true;
    }

    /* Map a whole (uncompressed) extent, splitting at stripe boundaries on
     * striped profiles: out receives (physical, length) pieces in logical
     * order. 2-copy chunks map whole (copies mirror the full chunk). */
    bool map_extent_pieces(u64 logical, u64 len, std::vector<std::pair<u64, u64>>& out) {
        const ChunkMap* c = find_chunk(logical, len);
        if (c == nullptr)
            return false;
        const bool striped = (c->type & (8u | 0x40u)) != 0;
        if (!striped) {
            u64 phys = 0;
            if (!logical_to_physical(logical, len, phys))
                return false;
            out.push_back({phys, len});
            return true;
        }
        if (c->stripe_len == 0)
            return false;
        u64 done = 0;
        while (done < len) {
            const u64 rel = (logical - c->logical_start) + done;
            const u64 in_stripe = rel % c->stripe_len;
            const u64 piece = std::min<u64>(len - done, c->stripe_len - in_stripe);
            u64 phys = 0;
            if (!logical_to_physical(c->logical_start + rel, piece, phys)) {
                out.clear();
                return false;
            }
            out.push_back({phys, piece});
            done += piece;
        }
        return true;
    }

    u64 mirror_rotations = 0; /* stats tap: served-by-non-first-stripe accesses */

    /* read a tree node by logical address (self-validating: header.bytenr
     * must equal the logical address, header.fsid must equal the
     * superblock's fsid, and chunk-tree nodes must carry the SB's
     * chunk_tree_uuid - kernel disk-io.c btrfs_buffer_uptodate gates). */
    bool read_node(u64 logical, std::vector<u8>& out, bool chunk_tree = false) {
        u64 phys = 0;
        if (!logical_to_physical(logical, sb.nodesize, phys))
            return false;
        out.resize(sb.nodesize);
        if (dev->read_at(phys, out.data(), sb.nodesize) != OK)
            return false;
        if (le64(out.data() + HDR_BYTENR) != logical)
            return false; /* self-reference check - wrong candidate never validates */
        if (std::memcmp(out.data() + HDR_FSID, sb.fsid, 16) != 0)
            return false; /* cross-linked/wrong-fs node */
        if (chunk_tree) {
            /* BTR-2 (AUDIT-LINUX-FS, re-pinned): the kernel superblock has
             * NO chunk_tree_uuid field (SB+0x60 is log_root) - the expected
             * uuid is COPIED from the chunk-root node's header
             * (fs_info->chunk_tree_uuid = header's +0x40, kernel
             * btrfs_read_chunk_tree semantics). The og does no uuid check
             * at all (only two memcmps in the whole decompile, both name
             * compares). Adopt on the first self-validating chunk-tree
             * node (the bytenr+fsid gates above make a wrong adopt
             * impossible), then verify every subsequent chunk-tree node. */
            if (!have_chunk_tree_uuid) {
                std::memcpy(chunk_tree_uuid, out.data() + HDR_CHUNK_TREE_UUID, 16);
                have_chunk_tree_uuid = true;
            } else if (std::memcmp(out.data() + HDR_CHUNK_TREE_UUID, chunk_tree_uuid, 16) != 0) {
                return false;
            }
        }
        return true;
    }
};

/* Parse one chunk item (48-byte head + stripes).
 * M-020: the full stripe array is decoded; the RAID10 geometry gate
 * rejects sub_stripes==0 / num_stripes<sub_stripes chunks exactly like
 * the og's parse (:2162-2168 - the free+reject path). */
bool parse_chunk(const u8* p, u32 len, ChunkMap& out) {
    if (len < 48)
        return false;
    const u64 length = le64(p + 0);
    const u64 stripe_len = le64(p + 0x10);
    const u64 type = le64(p + 0x18);
    const u16 num_stripes = le16(p + 0x2C);
    const u16 sub_stripes = le16(p + 0x2E);
    if (num_stripes == 0 || num_stripes > 16)
        return false;
    if (u32(48 + u32(num_stripes) * 32) > len)
        return false;
    /* og :2162-2168: RAID10 geometry gate (type bit 0x40) */
    if ((type & 0x40) != 0 && (sub_stripes == 0 || num_stripes < sub_stripes))
        return false;
    if (length == 0)
        return false;
    /* striped profiles divide by stripe_len (the og never gets there - its
     * read path fails classes 8/0x40 - so this gate is ours: a zero
     * stripe_len on a striped chunk is unmappable, refuse the chunk) */
    if ((type & (8u | 0x40u)) != 0 && stripe_len == 0)
        return false;
    out.stripes.clear();
    out.stripes.reserve(num_stripes);
    for (u16 i = 0; i < num_stripes; ++i) {
        const u8* st = p + 48 + u32(i) * 32;
        ChunkStripe cs;
        cs.devid = le64(st);
        cs.offset = le64(st + 8);
        out.stripes.push_back(cs);
    }
    out.logical_len = length;
    out.physical_start = out.stripes[0].offset;
    out.type = type;
    out.stripe_len = stripe_len;
    out.num_stripes = num_stripes;
    out.sub_stripes = sub_stripes;
    out.mirror_index = 0;
    const u64 logical = 0; /* set by caller via key.offset */
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
        if (cancel && cancel->checkpoint())
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
            const u32 limit = u32((node.size() - HDR_ITEM_ARRAY) / KP_SIZE);
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

bool validate_superblock(const u8 sb[4096], SuperBlock& out) {
    if (le64(sb + SB_MAGIC) != SB_MAGIC_VALUE)
        return false;
    const u32 sectorsize = le32(sb + SB_SECTORSIZE);
    const u32 nodesize = le32(sb + SB_NODESIZE);
    if (sectorsize == 0 || (sectorsize & (sectorsize - 1)) != 0)
        return false; /* pow2 (kernel btrfs_validate_super: 4K..64K) */
    if (nodesize == 0 || (nodesize & (nodesize - 1)) != 0)
        return false; /* kernel: nodesize must be pow2 */
    if (nodesize < 0x86 || nodesize > 0x10000)
        return false; /* band 0x86..0x10000 (kernel: <= 64K) */
    if (nodesize % sectorsize != 0 || nodesize < sectorsize)
        return false;
    out.generation = le64(sb + SB_GENERATION);
    out.sectorsize = sectorsize;
    out.nodesize = nodesize;
    std::memcpy(out.fsid, sb + SB_FSID, 16);
    /* BTR-2: chunk_tree_uuid removed from the SB model - SB+0x60 is
     * log_root in the kernel layout; the expected uuid is adopted from
     * the chunk-root node header at runtime. */
    /* embedded system chunk array (entries from +0x32B, byte count in
     * u32@+0xA0; entry = [key 17][chunk 48+32n]). Kernel allows up to
     * BTRFS_SYSTEM_CHUNK_ARRAY_SIZE=2048; 0 is accepted at recovery
     * grade (detection anchor without a chunk map - the tree walk
     * then fails honestly for lack of mapping). */
    const u32 array_size = le32(sb + SB_SYS_ARRAY_SIZE);
    if (array_size > 2048)
        return false;
    out.sys_chunks.clear();
    u32 consumed = 0;
    while (consumed + 17 + 48 <= array_size) {
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

u32 scan_btrfs(IStorageDevice* slice, CancelToken* cancel, WalkProgressLike progress,
               void* progress_user, tree::ResultTree& tree, tree::FullNode* root_active,
               tree::FullNode* root_deleted, tree::FullNode* root_lost, BtrfsScanStats& stats,
               const BtrfsCacheConfig* cache) {
    (void)root_deleted;
    (void)root_lost;
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    /* 1. mirror superblock select: newest generation wins (og rule).
     * Ground truth extracted from BtrfsSearchDLL.dll .rdata (VA 0x180018D60):
     * {0x10000, 0x4000000, 0x4000000000, 0x4000000000000} - identical to the
     * kernel btrfs_sb_offset(): 64 KiB primary + mirrors at 64 MiB, 256 GiB
     * and 1 PiB (16K << 12*mirror). */
    static const u64 kMirrorOffsets[] = {0x10000, 0x4000000, 0x4000000000ULL, 0x4000000000000ULL,
                                         0};
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

    /* N-072: the re-scan metadata cache. The og persists BtrfsCache.txt
     * beside its DLL (string evidence; format undecoded - ours is the
     * registered format in cr_btrfs_cache.h). OFF by default; when the
     * caller supplied a cache dir + io and a VALID blob exists for this
     * volume (fsid + generation match), the chunk map and the two tree
     * roots are adopted and the chunk-tree walk + root probing below are
     * SKIPPED (the re-scan's repeated derivations). Any mismatch
     * discards the blob - the walk runs as if no cache existed. */
    const bool cache_on = cache != nullptr && cache->io != nullptr && !cache->dir.empty();
    BtrfsCacheData cached;
    bool cache_hit = false;
    if (cache_on) {
        std::vector<u8> blob;
        if (cache->io->read_blob(cache->dir, best.fsid, blob) &&
            cache_parse_validate(best.fsid, best.generation, blob, cached) &&
            cached.chunk_root != 0 && cached.root_tree != 0) {
            cache_hit = true;
            ++stats.cache_hits;
        }
    }

    Volume v;
    v.dev = slice;
    v.sb = best;
    v.chunks = cache_hit ? cached.chunks : best.sys_chunks;

    u64 chunk_root = 0;
    u64 root_tree = 0;
    if (cache_hit) {
        /* the cached derivations skip the chunk-root candidate chase, the
         * chunk-tree walk and the root-tree probe - the trees themselves
         * are still walked and re-validated fresh every scan */
        chunk_root = cached.chunk_root;
        root_tree = cached.root_tree;
        stats.chunks = v.chunks.size();
    } else {
        /* 2. chunk tree walk: build the full logical->physical map. The chunk
         * tree root's logical address: candidates validated by the node's
         * self-referential bytenr (a wrong offset can never validate) AND by
         * the presence of CHUNK items (only the chunk tree holds them). BTR-2:
         * the expected chunk_tree_uuid is adopted from the first
         * self-validating chunk-tree node header (kernel semantics). */
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
                if (!v.read_node(cand, node, true))
                    continue;
                /* confirm this is the chunk tree: it must hold CHUNK items */
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
                    chunk_root = cand;
                    break;
                }
                /* not the chunk tree: forget the adopted uuid so the next
                 * candidate adopts its own */
                v.have_chunk_tree_uuid = false;
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
                        if (!v.read_node(logical, node, true))
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
                        v.have_chunk_tree_uuid =
                            false; /* not the chunk tree: re-adopt on the next */
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
    } /* cache-miss derivation path (N-072: the chunk chase, the chunk walk
       * and the root probe are the re-scan's repeated derivations) */

    /* N-072 save point: persist this scan's derivations for the next one.
     * A failed write is non-fatal (the scan already has its results); a
     * blob over the size bound is skipped (bounded cache). */
    if (cache_on && !cache_hit) {
        BtrfsCacheData d;
        d.generation = best.generation;
        d.chunk_root = chunk_root;
        d.root_tree = root_tree;
        d.chunks = v.chunks;
        std::vector<u8> blob;
        if (cache_serialize(best.fsid, d, cache->max_bytes, blob) &&
            cache->io->write_blob(cache->dir, best.fsid, blob))
            ++stats.cache_writes;
    }
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
    u64 default_subvol = 5; /* M-020: og fallback (FUN_1800061f0 :4060) */
    {
        /* N-047: ROOT_REF names arrive in the same root-tree walk; they
         * are buffered (item order between ROOT_ITEM 132 and ROOT_REF 156
         * is ascending per key order, but the walk tolerates any order)
         * and applied after the walk. */
        std::map<u64, std::u16string> ref_names;
        auto on_item = [&](const Key& k, const u8* data, u32 len) {
            if (k.type == KEY_ROOT_REF && len >= 18) {
                const u32 nl = u32(le16(data + 16));
                if (nl != 0 && nl <= 255 && len >= 18u + nl) {
                    std::u16string w = text::utf8_to_u16_lossless(
                        std::string_view(reinterpret_cast<const char*>(data + 18), nl));
                    if (!w.empty())
                        ref_names[k.objectid] = std::move(w);
                }
                return true;
            }
            /* M-020 default-subvolume pick (og FUN_1800061f0 :3975-4060):
             * the root tree's DIR_ITEM at
             * (ROOT_TREE_DIR_OBJECTID 6, 0x54, crc32c("default")=0x8dbfc2d2)
             * names the default subvolume; the record must spell
             * "default" (len 7) and its located key must be a ROOT_ITEM
             * (0x84) - the og's exact gates with its error strings
             * ("root DIR_ITEM had same CRC32, but was not \"default\"",
             * "expected subvolume"). */
            if (k.type == KEY_DIR_ITEM && k.objectid == OID_ROOT_TREE_DIR &&
                k.offset == 0x8dbfc2d2ULL && len >= DI_HEADER + 7) {
                const u16 nl = le16(data + DI_NAMELEN);
                const u16 dl = le16(data + DI_DATALEN);
                if (nl == 7 && dl == 0 && std::memcmp(data + DI_NAME, "default", 7) == 0 &&
                    data[DI_LOCATION + 8] == KEY_ROOT_ITEM)
                    default_subvol = le64(data + DI_LOCATION);
                return true;
            }
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
        /* apply the ROOT_REF names (N-047): the og surfaces each subvolume
         * with its ROOT_REF-derived display name; the subvol-<id> fallback
         * stays for unnamed/damaged root trees */
        for (auto& s : subvols) {
            const auto it = ref_names.find(s.id);
            if (it != ref_names.end())
                s.name = it->second;
        }
        /* the DIR_ITEM target must exist among the collected roots; the
         * og logs "could not find root %I64x, using default instead" and
         * falls back to root 5 */
        bool have_default = false;
        for (const auto& s : subvols)
            if (s.id == default_subvol) {
                have_default = true;
                break;
            }
        if (!have_default)
            default_subvol = 5;
    }
    if (progress)
        (void)progress(3, 4, progress_user);

    if (subvols.empty())
        return ERR_BAD_MAGIC;

    /* 5. per-subvolume FS tree walk: inodes + dir entries + extents.
     * M-020: the default subvolume (og FUN_1800061f0's pick) renders its
     * children DIRECTLY at the volume root (the ReFS convention - the
     * default IS the volume's content in the og); every other subvolume
     * stays a named container (ROOT_REF name / subvol-<id>). */
    for (const auto& sv : subvols) {
        if (cancel && cancel->checkpoint())
            return ERR_CANCELLED;
        const bool is_default = sv.id == default_subvol;
        tree::FileNode* sv_root = root_active;
        if (!is_default) {
            /* N-047: the ROOT_REF-derived name when the root tree carries
             * one (the og's per-subvol display name); subvol-<id> stays as
             * the fallback for unnamed subvolumes */
            std::u16string role16;
            if (!sv.name.empty()) {
                role16 = sv.name;
            } else {
                char role[32];
                snprintf(role, sizeof(role), "subvol-%llu", static_cast<unsigned long long>(sv.id));
                for (const char* p = role; *p; ++p)
                    role16.push_back(u16(u8(*p)));
            }
            auto* sv_props = new (std::nothrow) tree::FileProperties();
            if (!sv_props)
                continue;
            sv_props->name = role16;
            sv_props->fs_kind =
                u32(FsKindCode::Btrfs); /* factory 11 (row 45: 7 collided with exFAT) */
            sv_props->attributes = 0x10;
            sv_props->record_ref = sv.id;
            sv_root = tree.add_file(root_active, sv_props);
            sv_props->release();
            if (!sv_root)
                continue;
        }
        ++stats.dirs; /* the subvol node / the default's root dir is the root directory */

        /* phase 1: inode items */
        struct InodeInfo {
            u64 size = 0;
            u32 mode = 0;
            u32 nlink = 0;
            u64 generation = 0; /* N-059: inode-item qword@0 (og +0x81) */
            u64 atime = 0, ctime = 0, mtime = 0, crtime = 0;
            struct Ext {
                u64 off, len; /* slice bytes */
                u32 codec;    /* cr::compress::Codec (0 = raw) */
                u64 psize;    /* on-disk compressed size (raw: == len) */
            };
            std::vector<Ext> extents;
            std::vector<u8> inline_bytes; /* BTR-3: inline extent content */
            /* S-028: xattr-derived state (BtrfsSearchDLL :3424-3602) */
            u32 dos_attrib = 0; /* user.DOSATTRIB value (Windows bits) */
            bool have_dos = false;
            u8 comp_property = 0; /* btrfs.compression: zlib=1, "zlo"=2, zstd=3 */
            std::vector<std::pair<std::string, std::vector<u8>>> user_xattrs;
        };
        std::map<u64, InodeInfo> inodes;
        auto on_item = [&](const Key& k, const u8* data, u32 len) {
            if (k.type == KEY_INODE_ITEM && len >= INO_MIN) {
                InodeInfo& ii = inodes[k.objectid];
                ii.generation = le64(data + INO_GENERATION); /* N-059 */
                ii.size = le64(data + INO_SIZE);
                ii.nlink = le32(data + INO_NLINK);
                ii.mode = le32(data + INO_MODE);
                ii.atime =
                    unix_to_filetime_ns(i64(le64(data + INO_ATIME)), le32(data + INO_ATIME + 8));
                ii.ctime =
                    unix_to_filetime_ns(i64(le64(data + INO_CTIME)), le32(data + INO_CTIME + 8));
                ii.mtime =
                    unix_to_filetime_ns(i64(le64(data + INO_MTIME)), le32(data + INO_MTIME + 8));
                ii.crtime =
                    unix_to_filetime_ns(i64(le64(data + INO_otime)), le32(data + INO_otime + 8));
            } else if (k.type == KEY_EXTENT_DATA && len >= FE_TYPE + 1) {
                /* BTR-3: INLINE items may be as short as 21 bytes (header +
                 * payload) - the old len>=53 gate dropped every small
                 * inline file before the type byte was even read (the
                 * 53-byte floor only applies to REGULAR/PREALLOC bodies). */
                InodeInfo& ii = inodes[k.objectid];
                const u8 ftype = data[FE_TYPE];
                if (ftype == 0) {
                    /* BTR-3 (AUDIT-LINUX-FS, F-A2-13): inline extent - the
                     * item bytes after the 21-byte header ARE the file
                     * content (og collects them as resident runs; files
                     * < ~2 KiB recovered as size-correct, content-empty
                     * nodes before). Appended in key order (multiple
                     * inline extents per file are legal). */
                    if (len > FE_TYPE + 1)
                        ii.inline_bytes.insert(ii.inline_bytes.end(), data + FE_TYPE + 1,
                                               data + len);
                } else if (ftype == 1 || ftype == 2) {
                    const u8 comp = data[FE_COMPRESSION];
                    const u64 disk_bytenr = le64(data + FE_DISK_BYTENR);
                    const u64 disk_num = le64(data + FE_DISK_NUM_BYTES);
                    const u64 offset_in = le64(data + FE_OFFSET);
                    const u64 num = le64(data + FE_NUM_BYTES);
                    if (disk_bytenr == 0 || disk_num == 0)
                        return true; /* hole */
                    const u64 extent_logical = disk_bytenr + offset_in;
                    if (comp == 0) {
                        /* M-020: raw extents map through the profile-aware
                         * splitter - striped profiles (RAID0/RAID10) yield
                         * per-stripe pieces; 2-copy chunks rotate copies
                         * og-style per access. */
                        std::vector<std::pair<u64, u64>> pieces;
                        if (v.map_extent_pieces(extent_logical, num, pieces)) {
                            for (const auto& pr : pieces)
                                ii.extents.push_back({pr.first, pr.second, 0, 0});
                        } else {
                            ++stats.unmappable_extents;
                        }
                    } else {
                        /* BTR-4 (AUDIT-LINUX-FS): compressed extents carry
                         * psize bytes on disk that decode to `num` logical
                         * bytes - map them ZFS-style {codec, psize} so the
                         * writer decodes at recovery time. A compressed
                         * extent is atomic (no stripe splitting) - on
                         * striped profiles a spanning one is refused and
                         * counted, never emitted with wrong bytes.
                         * Btrfs compression ids: 1 zlib (raw deflate ->
                         * kGzip), 2 lzo (no codec in our layer - counted,
                         * extent left unmapped rather than emitting wrong
                         * raw bytes), 3 zstd. */
                        u64 phys = 0;
                        if (v.logical_to_physical(extent_logical, num, phys)) {
                            if (comp == 1)
                                ii.extents.push_back(
                                    {phys, num, u32(compress::Codec::kGzip), disk_num});
                            else if (comp == 3)
                                ii.extents.push_back(
                                    {phys, num, u32(compress::Codec::kZstd), disk_num});
                            else if (comp == 2)
                                ++stats.lzo_extents;
                        } else if (comp == 2) {
                            ++stats.lzo_extents;
                        } else {
                            ++stats.unmappable_extents;
                        }
                        if (comp != 0)
                            ++stats.compressed_extents;
                    }
                }
            } else if (k.type == KEY_XATTR && len >= DI_HEADER) {
                InodeInfo& ii = inodes[k.objectid];
                /* S-028 (BtrfsSearchDLL :3424-3602): XATTR items carry the
                 * SAME dir_item record shape (the kernel shares the struct
                 * for dir entries, xattrs and verity descriptors). The og
                 * extracts user.DOSATTRIB (:3424-3441, name_len 0xE + the
                 * 14 name bytes), maps btrfs.compression values
                 * {"zlib"->1, "zlo"->2, "zstd"->3} (:3517-3543), and
                 * surfaces every other user.* xattr (:3544-3602). */
                u32 off = 0;
                while (off + DI_HEADER <= len) {
                    const u8* di = data + off;
                    const u16 data_len = le16(di + DI_DATALEN);
                    const u16 name_len = le16(di + DI_NAMELEN);
                    if (u64(off) + DI_HEADER + u64(name_len) + u64(data_len) > len)
                        break; /* truncated tail: stop, keep what we have */
                    const char* nm = reinterpret_cast<const char*>(di) + DI_NAME;
                    const u32 nlen = name_len > 255 ? 255 : name_len;
                    const std::string name(nm, nm + nlen);
                    const u8* val = di + DI_HEADER + name_len;
                    if (name == "user.DOSATTRIB" && data_len > 0 && data_len <= 16) {
                        /* Samba stores the Windows attribute set as text
                         * ("0x20" hex). The og copies the raw value out
                         * (:3431-3440); the text->bits conversion: */
                        char txt[17];
                        std::memcpy(txt, val, data_len);
                        txt[data_len] = 0;
                        ii.dos_attrib = u32(strtoul(txt, nullptr, 0));
                        ii.have_dos = true;
                    } else if (name == "btrfs.compression" && data_len > 0 && data_len <= 8) {
                        char txt[9];
                        std::memcpy(txt, val, data_len);
                        txt[data_len] = 0;
                        if (strcmp(txt, "zlib") == 0)
                            ii.comp_property = 1;
                        else if (strcmp(txt, "zlo") == 0)
                            ii.comp_property = 2;
                        else if (strcmp(txt, "zstd") == 0)
                            ii.comp_property = 3;
                        /* any other value: the og writes 0 (:3529 LAB) */
                    } else if (name.size() > 5 && name.compare(0, 5, "user.") == 0) {
                        /* the og's per-file attribute list (:3544-3602):
                         * every other user.* xattr is a record */
                        ii.user_xattrs.emplace_back(name, std::vector<u8>(val, val + data_len));
                    }
                    off += DI_HEADER + name_len + data_len;
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
            std::string name_raw; /* N-016b: original bytes when mapped */
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
                /* S-005 + M-010 (diff-to-fix): Btrfs dir-item names are
                 * raw bytes the og passes through unconverted - decode
                 * strictly, invalid bytes LOSSLESSLY (U+E000 + byte,
                 * bijective; M-010: the og keeps the raw bytes) */
                const u16 capped_len = name_len > 255 ? 255 : name_len;
                de.name = text::utf8_to_u16_lossless(
                    std::string_view(reinterpret_cast<const char*>(di) + DI_NAME, capped_len));
                /* N-016b (N-029 residual): original bytes alongside the
                 * mapped name (the og's session keeps them verbatim). */
                if (text::name_has_raw_band(de.name))
                    de.name_raw.assign(reinterpret_cast<const char*>(di) + DI_NAME, capped_len);
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
                    if (cancel && cancel->checkpoint())
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
                    cp->name_raw = de.name_raw;           /* N-016b: og-parity raw bytes */
                    cp->fs_kind = u32(FsKindCode::Btrfs); /* factory 11 (row 45) */
                    cp->logical_size = cit->second.size;
                    /* N-047: the og's FILE_ATTRIBUTE_NORMAL (0x80) on
                     * regular files - dirs 0x10 (BtrfsSearchDLL_decompiled.c
                     * :3574-3577: uStack_92 = 0x80 default, 0x10 when the
                     * type byte == 2). The S-028 DOSATTRIB OR below still
                     * applies on top. */
                    cp->attributes = is_dir ? 0x10 : 0x80;
                    cp->times.created = cit->second.crtime;
                    cp->times.modified = cit->second.mtime;
                    cp->times.changed = cit->second.ctime;
                    cp->times.accessed = cit->second.atime;
                    cp->record_ref = de.child;
                    /* N-059: the per-FS secondary payload — the owning
                     * subvol root id (the naming/N-047 datum; og +0x3B
                     * census-level) and the inode generation (og +0x81
                     * "generation/transid family"). id_b has no decoded
                     * source; it stays 0 (never invented). */
                    cp->btrfs_id_a = u32(sv.id);
                    cp->btrfs_generation = cit->second.generation;
                    for (const auto& e : cit->second.extents)
                        cp->extents.push_back({e.off, e.len, e.codec, e.psize});
                    if (!cit->second.inline_bytes.empty()) {
                        /* BTR-3: inline content travels as resident data
                         * (emitted before extents, ZFS-embedded parity) */
                        cp->resident_data = cit->second.inline_bytes;
                        ++stats.inline_files;
                    }
                    /* S-028: user.DOSATTRIB carries the Windows attribute
                     * bits onto the node (the og applies them at :3424-
                     * 3441's consumer; the dir bit stays). */
                    if (cit->second.have_dos)
                        cp->attributes |= cit->second.dos_attrib;
                    if (cit->second.comp_property != 0)
                        ++stats.compression_property_files;
                    tree::FileNode* child_node = tree.add_file(node, cp);
                    cp->release();
                    if (!child_node)
                        continue;
                    /* S-028: other user.* xattrs surface as per-file
                     * attribute records (the og's per-file list :3544-3602;
                     * our shape: the N-011 ADS-children pattern - a
                     * ..xattr/<name> child carrying the value as resident
                     * content, mirroring the HFS+ ..namedfork shape S-002
                     * established). */
                    for (const auto& xa : cit->second.user_xattrs) {
                        auto* xp = new (std::nothrow) tree::FileProperties();
                        if (!xp)
                            break;
                        std::u16string xn;
                        xn.assign(u"..xattr/");
                        for (char c : xa.first)
                            xn.push_back(u16(u8(c)));
                        xp->name = xn;
                        xp->fs_kind = u32(FsKindCode::Btrfs);
                        xp->logical_size = xa.second.size();
                        xp->resident_data = xa.second;
                        tree::FileNode* xnode = tree.add_file(child_node, xp);
                        xp->release();
                        if (xnode) {
                            xnode->set_origin(tree::Origin::Structured);
                            ++stats.xattr_records;
                        }
                    }
                    emitted.insert(de.child);
                    if (is_dir) {
                        ++stats.dirs;
                        /* M-110: the og's Synology "@eaDir" descent
                         * suppression (BtrfsSearchDLL FUN_180001cd0
                         * :704-711). Instruction-verified against the
                         * binary this session: the compare is 7 bytes
                         * INCLUDING the literal's NUL terminator
                         * (180001e30 movzx cl,[rdx+rax] /
                         * cmp cl,[r12+rax-1] / cmp rax,7 - byte 6 of
                         * "@eaDir" at 0x180018580 is 0x00), so the
                         * match is EXACT: a directory named exactly
                         * "@eaDir" is emitted EMPTY (the node itself
                         * stays; the og's entry-emit ran before the
                         * pop-time name check) and its children -
                         * Synology's SYNO@EA@… EA entries - are never
                         * walked. A longer name such as "@eaDirBackup"
                         * mismatches at index 6 and descends normally
                         * (the finding's prefix-match reading is
                         * corrected by this decode). NOT routed through
                         * cr_filterfolder: the M-018 pass flags nodes
                         * by name-pool membership (subtree kept); this
                         * is a descent suppression - different
                         * mechanism, different shape. */
                        if (de.name != u"@eaDir")
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
    stats.mirror_rotations = v.mirror_rotations;
    return OK;
}

} // namespace btrfs
} // namespace cr
