/* cr_hfsplus_engine.cpp - HFS+/HFSX scan engine.
 *
 * All on-disk integers are BIG-endian (Apple TN1150 layouts).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/hfsplus/cr_hfsplus_engine.h"
#include <cstring>
#include <functional>
#include <map>
#include <set>

namespace cr {
namespace hfsplus {

namespace {

/* ---- volume header offsets (from byte 1024) ---- */
constexpr u32 VH_SIG = 0x00;
constexpr u32 VH_VERSION = 0x02;
constexpr u32 VH_JOURNAL_INFO = 0x0C;
constexpr u32 VH_FILE_COUNT = 0x20;
constexpr u32 VH_FOLDER_COUNT = 0x24;
constexpr u32 VH_BLOCK_SIZE = 0x28;
constexpr u32 VH_TOTAL_BLOCKS = 0x2C;
constexpr u32 VH_FREE_BLOCKS = 0x30;
constexpr u32 VH_NEXT_CATALOG_ID = 0x40;
constexpr u32 VH_ALLOCATION_FORK = 0x70; /* ForkData, 80 bytes */
constexpr u32 VH_EXTENTS_FORK = 0xC0;
constexpr u32 VH_CATALOG_FORK = 0x110;
constexpr u32 VH_ATTRIBUTES_FORK = 0x160;
constexpr u32 VH_STARTUP_FORK = 0x1B0;

constexpr u16 SIG_HFSPLUS = 0x482B; /* "H+" */
constexpr u16 SIG_HFSX = 0x4858;    /* "HX" */

/* CNIDs */
constexpr u32 CNID_ROOT_PARENT = 1;
constexpr u32 CNID_ROOT_FOLDER = 2;
constexpr u32 CNID_EXTENTS_FILE = 3;
constexpr u32 CNID_CATALOG_FILE = 4;
constexpr u32 CNID_ALLOCATION_FILE = 6;
constexpr u32 CNID_ATTRIBUTES_FILE = 8;
constexpr u32 CNID_FIRST_USER = 16;

/* fork data (80 bytes) */
constexpr u32 FORK_LOGICAL_SIZE = 0x00;
constexpr u32 FORK_TOTAL_BLOCKS = 0x0C;
constexpr u32 FORK_EXTENTS = 0x10; /* 8 x (start u32, count u32) */

/* B-tree node header (14 bytes) — TN1150 BTNodeDescriptor; og gate
 * FUN_180010720 accepts kind in {-1,0,1,2} (kind+1 <= 3 unsigned) */
constexpr u32 NH_FORWARD = 0x00;
constexpr u32 NH_BACKWARD = 0x04;
constexpr i16 NH_KIND_LEAF = -1;  /* kBTLeafNode */
constexpr i16 NH_KIND_INDEX = 0;  /* kBTIndexNode */
constexpr i16 NH_KIND_HEADER = 1; /* kBTHeaderNode */
constexpr i16 NH_KIND_MAP = 2;    /* kBTMapNode */
constexpr u32 NH_KIND = 0x08;     /* SInt8 node type */
constexpr u32 NH_HEIGHT = 0x09;
constexpr u32 NH_NUM_RECORDS = 0x0A;

/* BTHeaderRec (after the 14-byte node descriptor of node 0; 106 bytes,
 * big-endian, packed — TN1150 / libyal layout table, og-verified):
 * +0x00 treeDepth u16, +0x02 rootNode u32, +0x06 leafRecords u32,
 * +0x0A firstLeafNode u32, +0x0E lastLeafNode u32, +0x12 nodeSize u16,
 * +0x14 maxKeyLength u16, +0x16 totalNodes u32, +0x1A freeNodes u32 */
constexpr u32 BTH_FIRST_LEAF = 0x0A;
constexpr u32 BTH_LAST_LEAF = 0x0E;
constexpr u32 BTH_NODE_SIZE = 0x12;

/* catalog record types */
constexpr i16 REC_FOLDER = 1;
constexpr i16 REC_FILE = 2;
constexpr i16 REC_FOLDER_THREAD = 3;
constexpr i16 REC_FILE_THREAD = 4;

/* HFSPlusCatalogFile field offsets (from record body start) */
constexpr u32 CF_RECORD_TYPE = 0x00;
constexpr u32 CF_FLAGS = 0x02;
constexpr u32 CF_FILE_ID = 0x08;
constexpr u32 CF_CREATE_DATE = 0x0C;
constexpr u32 CF_CONTENT_MOD = 0x10;
constexpr u32 CF_ATTR_MOD = 0x14;
constexpr u32 CF_ACCESS_DATE = 0x18;
constexpr u32 CF_TEXT_ENCODING = 0x50;
constexpr u32 CF_DATA_FORK = 0x58;
constexpr u32 CF_RESOURCE_FORK = 0xA8;

/* folder record fields */
constexpr u32 CD_VALENCE = 0x04;
constexpr u32 CD_FOLDER_ID = 0x08;
constexpr u32 CD_CREATE = 0x0C;
constexpr u32 CD_CONTENT_MOD = 0x10;

u16 be16(const u8* p) {
    return u16((u32(p[0]) << 8) | p[1]);
}
u32 be32(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}
u64 be64(const u8* p) {
    return (u64(be32(p)) << 32) | be32(p + 4);
}

constexpr u32 MAX_TREE_NODES = 1u << 22; /* 4M nodes bound */
constexpr u32 MAX_RECORD_NAME = 255;
constexpr u32 DIR_DEPTH_LIMIT = 64;
constexpr u64 MAX_FORK_BYTES = 64ull << 30;

struct ForkData {
    u64 logical_size = 0;
    u32 total_blocks = 0;
    struct Extent {
        u32 start_block;
        u32 block_count;
    };
    Extent extents[8];
};

ForkData parse_fork(const u8* p) {
    ForkData f;
    f.logical_size = be64(p + FORK_LOGICAL_SIZE);
    f.total_blocks = be32(p + FORK_TOTAL_BLOCKS);
    for (int i = 0; i < 8; ++i) {
        f.extents[i].start_block = be32(p + FORK_EXTENTS + u32(i) * 8);
        f.extents[i].block_count = be32(p + FORK_EXTENTS + u32(i) * 8 + 4);
    }
    return f;
}

struct Volume {
    IStorageDevice* dev = nullptr;
    VolumeHeader vh;
    ForkData allocation_fork;
    ForkData extents_fork;
    ForkData catalog_fork;

    /* per-tree geometry (set from each tree's header node) */
    u32 catalog_node_size = 0;
    u32 catalog_first_leaf = 0;
    u32 catalog_last_leaf = 0;

    /* overflow extents resolved lazily: fileID -> (fork type -> extents) */
    std::map<u32, std::map<u8, std::vector<ForkData::Extent>>> overflow;

    u64 total_bytes() const { return u64(vh.total_blocks) * vh.block_size; }
};

/* Read node 0 of a tree when the node size is still unknown: the header
 * node starts at the fork's first extent; block_size bytes cover it for
 * every legal node size. */
bool read_node0_bootstrap(Volume& v, const ForkData& fork, std::vector<u8>& out) {
    if (fork.extents[0].block_count == 0)
        return false;
    const u64 off = u64(fork.extents[0].start_block) * v.vh.block_size;
    out.resize(v.vh.block_size);
    return v.dev->read_at(off, out.data(), u32(out.size())) == OK;
}

/* Read one B-tree node by its index inside a fork. The initial extent
 * descriptors map fork-logical blocks to volume blocks; node N lives at
 * fork-logical byte N * node_size. Trees larger than the initial extents
 * (8 descriptors) resolve through the overflow map when present. */
bool read_tree_node(Volume& v, const ForkData& fork, u32 file_id, u32 node_size, u32 node_index,
                    std::vector<u8>& out) {
    if (node_index >= MAX_TREE_NODES || node_size == 0 || node_size < 512 || node_size > 65536 ||
        (node_size & (node_size - 1)) != 0)
        return false;
    const u64 logical = u64(node_index) * node_size;

    auto try_extents = [&](const std::vector<ForkData::Extent>& list) -> bool {
        u64 cursor = 0;
        for (const auto& e : list) {
            const u64 extent_bytes = u64(e.block_count) * v.vh.block_size;
            if (extent_bytes == 0)
                continue;
            if (logical >= cursor && logical + node_size <= cursor + extent_bytes) {
                const u64 phys = u64(e.start_block) * v.vh.block_size + (logical - cursor);
                out.resize(node_size);
                return v.dev->read_at(phys, out.data(), node_size) == OK;
            }
            cursor += extent_bytes;
        }
        return false;
    };

    std::vector<ForkData::Extent> initial(fork.extents, fork.extents + 8);
    if (try_extents(initial))
        return true;
    /* overflow extents for this tree's own data fork */
    auto it = v.overflow.find(file_id);
    if (it != v.overflow.end()) {
        auto fit = it->second.find(0);
        if (fit != it->second.end() && try_extents(fit->second))
            return true;
    }
    return false;
}

/* Resolve a FILE fork's full extent list (initial + overflow), in
 * slice-relative byte pairs. */
std::vector<std::pair<u64, u64>> resolve_file_extents(Volume& v, u32 cnid, u8 fork_type,
                                                      const ForkData& fork, u64 total_bytes) {
    std::vector<ForkData::Extent> all(fork.extents, fork.extents + 8);
    auto it = v.overflow.find(cnid);
    if (it != v.overflow.end()) {
        auto fit = it->second.find(fork_type);
        if (fit != it->second.end())
            for (const auto& e : fit->second)
                all.push_back(e);
    }
    std::vector<std::pair<u64, u64>> out;
    for (const auto& e : all) {
        if (e.block_count == 0)
            continue;
        const u64 off = u64(e.start_block) * v.vh.block_size;
        u64 len = u64(e.block_count) * v.vh.block_size;
        if (off + len > total_bytes)
            len = total_bytes > off ? total_bytes - off : 0;
        if (len)
            out.push_back({off, len});
    }
    return out;
}

/* Parse a catalog key: [keyLength u16 (data length, EXCLUDES this field)]
 * [parentCNID u32] [nameLength u16] [UTF16BE chars]. */
bool parse_catalog_key(const u8* p, u32 limit, u32& key_len, u32& parent_cnid,
                       std::u16string& name) {
    if (limit < 8)
        return false;
    key_len = be16(p);
    /* key data = parent(4) + nameLen(2) + name(2N) must fit in keyLength */
    if (key_len < 6 || key_len > 510 || u64(2) + key_len > limit)
        return false;
    parent_cnid = be32(p + 2);
    const u32 name_len = be16(p + 6);
    if (u64(6) + u64(name_len) * 2 > key_len)
        return false;
    name.clear();
    for (u32 i = 0; i < name_len && i < MAX_RECORD_NAME; ++i)
        name.push_back(be16(p + 8 + u32(i) * 2));
    return true;
}

/* One decoded catalog record. */
struct CatalogRecord {
    i16 rec_type = 0;
    u32 cnid = 0;
    u32 parent_cnid = 0;
    std::u16string name;
    /* file */
    ForkData data_fork;
    ForkData rsrc_fork;
    u32 create_date = 0, mod_date = 0, attr_date = 0, access_date = 0;
    u16 flags = 0;
};

/* Parse one leaf record body after the key. */
bool parse_catalog_record(const u8* key, u32 rec_bytes, CatalogRecord& out) {
    if (rec_bytes < 2)
        return false;
    u32 key_len = 0, parent = 0;
    std::u16string name;
    if (!parse_catalog_key(key, rec_bytes, key_len, parent, name))
        return false;
    /* total key = 2-byte length field + keyLength bytes of data; the whole
     * key is padded to even length */
    u32 off = 2 + key_len;
    if (off & 1)
        ++off;
    if (off + 2 > rec_bytes)
        return false;
    const i16 type = i16(be16(key + off));
    out.rec_type = type;
    out.parent_cnid = parent;
    out.name = name;

    const u8* body = key + off;
    const u32 body_len = rec_bytes - off;
    if (type == REC_FILE) {
        if (body_len < 0xF8)
            return false;
        out.cnid = be32(body + CF_FILE_ID);
        out.flags = be16(body + CF_FLAGS);
        out.create_date = be32(body + CF_CREATE_DATE);
        out.mod_date = be32(body + CF_CONTENT_MOD);
        out.attr_date = be32(body + CF_ATTR_MOD);
        out.access_date = be32(body + CF_ACCESS_DATE);
        out.data_fork = parse_fork(body + CF_DATA_FORK);
        out.rsrc_fork = parse_fork(body + CF_RESOURCE_FORK);
        return out.cnid >= CNID_FIRST_USER || out.cnid == CNID_ROOT_FOLDER;
    }
    if (type == REC_FOLDER) {
        if (body_len < 0x58)
            return false;
        out.cnid = be32(body + CD_FOLDER_ID);
        out.create_date = be32(body + CD_CREATE);
        out.mod_date = be32(body + CD_CONTENT_MOD);
        return out.cnid >= CNID_FIRST_USER || out.cnid == CNID_ROOT_FOLDER;
    }
    return false; /* threads: no new info (parent links already known) */
}

/* Walk every record of one node through the record-offset array. */
bool for_each_record(u32 node_size, const std::vector<u8>& node, i16 want_kind,
                     const std::function<bool(const u8* rec, u32 len)>& fn) {
    if (node.size() < 14 + 2 || node_size == 0)
        return false;
    /* node type is a SInt8 at +8 — SIGN-extend before the kind compare
     * (PARITY-AUDIT F-A2-17: i16(u8) zero-extended, so a real leaf node
     * (0xFF = -1) could never match a negative kind constant) */
    const i16 kind = static_cast<i16>(static_cast<i8>(node[NH_KIND]));
    if (kind != want_kind)
        return false;
    const u16 nrec = be16(node.data() + NH_NUM_RECORDS);
    if (nrec == 0 || nrec > (node_size - 14) / 2)
        return false;
    /* record offsets: last (nrec) u16s of the node, BE, offsets from node
     * start; record i occupies [off(i), off(i-1)) growing downward */
    const u8* base = node.data();
    for (u32 i = 0; i < nrec; ++i) {
        const u32 off_this = be16(base + node_size - (i + 1) * 2);
        const u32 off_prev = i == 0 ? node_size : be16(base + node_size - i * 2);
        if (off_this < 14 || off_this >= node_size || off_prev > node_size || off_prev <= off_this)
            continue; /* damaged entry: skip */
        if (!fn(base + off_this, off_prev - off_this))
            return false;
    }
    return true;
}

} // namespace

u64 mac_to_filetime(u32 mac_sec) {
    /* Mac epoch 1904-01-01 -> 1601: 9561628800 seconds */
    return (u64(mac_sec) + 9561628800ULL) * 10000000ULL;
}

bool validate_volume_header(const u8 vh[512], VolumeHeader& out) {
    const u16 sig = be16(vh + VH_SIG);
    if (sig != SIG_HFSPLUS && sig != SIG_HFSX)
        return false;
    const u16 version = be16(vh + VH_VERSION);
    if (version >= 0x11)
        return false;
    const u32 block_size = be32(vh + VH_BLOCK_SIZE);
    if ((block_size & 0x1FF) != 0 || block_size < 512 || block_size > 65536)
        return false;
    const u32 total_blocks = be32(vh + VH_TOTAL_BLOCKS);
    if (total_blocks == 0)
        return false;
    /* fork block-count clamps (decompile-verified gate) */
    const ForkData alloc = parse_fork(vh + VH_ALLOCATION_FORK);
    const ForkData ext = parse_fork(vh + VH_EXTENTS_FORK);
    if (alloc.total_blocks > total_blocks || ext.total_blocks > total_blocks)
        return false;

    out.signature = sig;
    out.version = version;
    out.block_size = block_size;
    out.total_blocks = total_blocks;
    out.free_blocks = be32(vh + VH_FREE_BLOCKS);
    out.file_count = be32(vh + VH_FILE_COUNT);
    out.folder_count = be32(vh + VH_FOLDER_COUNT);
    out.next_catalog_id = be32(vh + VH_NEXT_CATALOG_ID);
    out.journal_info_block = be32(vh + VH_JOURNAL_INFO);
    out.valid = true;
    return true;
}

u32 scan_hfsplus(IStorageDevice* slice, const HfsScanFlags& flags, CancelToken* cancel,
                 WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
                 tree::FullNode* root_active, tree::FullNode* root_deleted,
                 tree::FullNode* root_lost, HfsScanStats& stats) {
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    u8 vh_raw[512];
    if (slice->read_at(1024, vh_raw, sizeof(vh_raw)) != OK)
        return ERR_READ_FAILED;
    Volume v;
    if (!validate_volume_header(vh_raw, v.vh))
        return ERR_BAD_MAGIC;
    v.dev = slice;
    v.allocation_fork = parse_fork(vh_raw + VH_ALLOCATION_FORK);
    v.extents_fork = parse_fork(vh_raw + VH_EXTENTS_FORK);
    v.catalog_fork = parse_fork(vh_raw + VH_CATALOG_FORK);

    /* 1. extents-overflow B-tree: preload all leaf records into the map
     * (fragmented-fork resolution; typically tiny). */
    u32 ext_node_size = 0;
    {
        std::vector<u8> node;
        if (v.extents_fork.extents[0].block_count &&
            read_node0_bootstrap(v, v.extents_fork, node) && node.size() >= 14 + 0x14) {
            ext_node_size = be16(node.data() + 14 + BTH_NODE_SIZE);
            if (ext_node_size == 0 || (ext_node_size & (ext_node_size - 1)) != 0)
                ext_node_size = 0;
        }
        if (ext_node_size) {
            const u32 fl = be32(node.data() + 14 + BTH_FIRST_LEAF);
            const u32 ll = be32(node.data() + 14 + BTH_LAST_LEAF);
            u32 cur = fl;
            int guard = 0;
            while (cur != 0 && guard++ < 65536) {
                std::vector<u8> leaf;
                if (!read_tree_node(v, v.extents_fork, CNID_EXTENTS_FILE, ext_node_size, cur, leaf))
                    break;
                for_each_record(ext_node_size, leaf, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                    /* overflow record: key{keyLength u16, forkType u8, pad u8,
                     * fileID u32, startBlock u32}, body = 8 extent descriptors */
                    if (len < 10 + 64)
                        return true;
                    const u8 fork_type = rec[2];
                    const u32 file_id = be32(rec + 4);
                    ForkData::Extent ext[8];
                    for (int i = 0; i < 8; ++i) {
                        ext[i].start_block = be32(rec + 10 + u32(i) * 8);
                        ext[i].block_count = be32(rec + 10 + u32(i) * 8 + 4);
                    }
                    auto& list = v.overflow[file_id][fork_type];
                    for (int i = 0; i < 8; ++i)
                        if (ext[i].block_count)
                            list.push_back(ext[i]);
                    ++stats.overflow_extents;
                    return true;
                });
                if (cur == ll)
                    break;
                cur = be32(leaf.data() + NH_FORWARD);
                if (cancel && cancel->checkpoint())
                    return ERR_CANCELLED;
            }
        }
    }

    /* 2. catalog B-tree geometry (node 0 = header node, bootstrapped). */
    {
        std::vector<u8> node;
        if (!read_node0_bootstrap(v, v.catalog_fork, node))
            return ERR_BAD_MAGIC;
        v.catalog_node_size = be16(node.data() + 14 + BTH_NODE_SIZE);
        if (v.catalog_node_size < 512 || v.catalog_node_size > 65536 ||
            (v.catalog_node_size & (v.catalog_node_size - 1)) != 0)
            return ERR_BAD_MAGIC;
        v.catalog_first_leaf = be32(node.data() + 14 + BTH_FIRST_LEAF);
        v.catalog_last_leaf = be32(node.data() + 14 + BTH_LAST_LEAF);
    }

    /* 3. leaf-chain walk: collect every file/folder record. */
    struct Collected {
        CatalogRecord rec;
    };
    std::vector<CatalogRecord> records;
    u32 cur = v.catalog_first_leaf;
    u64 chain_len = 0;
    while (cur != 0 && chain_len < MAX_TREE_NODES) {
        if (cancel && cancel->checkpoint())
            return ERR_CANCELLED;
        ++chain_len;
        if (progress && (chain_len & 15) == 0)
            (void)progress(chain_len, 1024, progress_user);
        std::vector<u8> leaf;
        if (!read_tree_node(v, v.catalog_fork, CNID_CATALOG_FILE, v.catalog_node_size, cur, leaf)) {
            ++stats.bad_nodes;
            break;
        }
        for_each_record(v.catalog_node_size, leaf, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
            CatalogRecord cr;
            if (parse_catalog_record(rec, len, cr)) {
                if (records.size() < (4u << 20)) /* P1-14: record cap */
                    records.push_back(cr);
                ++stats.leaf_records;
            }
            return true;
        });
        if (cur == v.catalog_last_leaf)
            break;
        const u32 next = be32(leaf.data() + NH_FORWARD);
        if (next == cur)
            break; /* self loop guard */
        cur = next;
    }

    /* 4. build the hierarchy: cnid -> node map, two passes (folders may
     * appear after their children in leaf order). */
    std::map<u32, tree::FileNode*> by_cnid;
    tree::FileNode* root_node = nullptr;

    /* pass A: create nodes */
    std::vector<tree::FileNode*> nodes(records.size(), nullptr);
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        if (r.cnid < CNID_FIRST_USER && r.cnid != CNID_ROOT_FOLDER)
            continue;
        auto* props = new (std::nothrow) tree::FileProperties();
        if (!props)
            return ERR_NO_MEMORY;
        const bool is_dir = r.rec_type == REC_FOLDER;
        props->name = r.name;
        props->fs_kind = 8; /* HFS (corpus FS code) */
        props->attributes = is_dir ? 0x10 : 0;
        props->logical_size = is_dir ? 0 : r.data_fork.logical_size;
        props->times.created = mac_to_filetime(r.create_date);
        props->times.modified = mac_to_filetime(r.mod_date);
        props->times.changed = mac_to_filetime(r.attr_date ? r.attr_date : r.mod_date);
        props->times.accessed = mac_to_filetime(r.access_date);
        props->record_ref = r.cnid;
        for (const auto& ex : resolve_file_extents(v, r.cnid, 0, r.data_fork, v.total_bytes()))
            props->extents.push_back({ex.first, ex.second});
        nodes[i] = tree.add_file(root_active, props); /* re-parented below */
        props->release();
        if (!nodes[i])
            return ERR_NO_MEMORY;
        if (is_dir)
            ++stats.dirs;
        else
            ++stats.files;
        if (r.cnid == CNID_ROOT_FOLDER)
            root_node = nodes[i];
        by_cnid[r.cnid] = nodes[i];
    }

    /* pass B: reparent to the real parents (catalog order independent) */
    for (size_t i = 0; i < records.size(); ++i) {
        if (!nodes[i])
            continue;
        const auto& r = records[i];
        tree::FileNode* parent = nullptr;
        if (r.parent_cnid == CNID_ROOT_PARENT || r.parent_cnid == CNID_ROOT_FOLDER) {
            parent = root_node ? root_node : nullptr;
            if (r.cnid == CNID_ROOT_FOLDER)
                continue; /* the root stays at top level */
        } else {
            auto it = by_cnid.find(r.parent_cnid);
            if (it != by_cnid.end())
                parent = it->second;
        }
        if (parent && parent != nodes[i])
            tree.reparent(nodes[i], parent);
        else if (!parent && r.cnid != CNID_ROOT_FOLDER)
            tree.reparent(nodes[i], root_lost ? root_lost : root_active); /* orphan */
    }

    /* 5. resource forks (..namedfork/rsrc convention). */
    if (flags.resource_forks) {
        for (size_t i = 0; i < records.size(); ++i) {
            if (!nodes[i] || records[i].rec_type != REC_FILE)
                continue;
            const auto& rf = records[i].rsrc_fork;
            if (rf.logical_size == 0)
                continue;
            auto* props = new (std::nothrow) tree::FileProperties();
            if (!props)
                break;
            props->name = u"..namedfork/rsrc";
            props->fs_kind = 8;
            props->logical_size = rf.logical_size;
            props->record_ref = records[i].cnid;
            for (const auto& ex :
                 resolve_file_extents(v, records[i].cnid, 0xFF, rf, v.total_bytes()))
                props->extents.push_back({ex.first, ex.second});
            tree::FileNode* n = tree.add_file(nodes[i], props);
            props->release();
            if (!n)
                break;
        }
    }

    /* 6. orphan sweep: catalog nodes OUTSIDE the active leaf chain hold
     * stale records from deleted files (HFS+ delete overwrites catalog
     * records; freed nodes keep the old bytes until reused). */
    if (flags.scan_freed_nodes && flags.keep_deleted && root_deleted) {
        std::set<u32> live_chain;
        {
            u32 c = v.catalog_first_leaf;
            u64 guard = 0;
            while (c != 0 && guard++ < MAX_TREE_NODES) {
                live_chain.insert(c);
                if (c == v.catalog_last_leaf)
                    break;
                std::vector<u8> leaf;
                if (!read_tree_node(v, v.catalog_fork, CNID_CATALOG_FILE, v.catalog_node_size, c,
                                    leaf))
                    break;
                c = be32(leaf.data() + NH_FORWARD);
            }
        }
        /* allocation-bitmap grading cache */
        std::vector<u8> alloc_map;
        if (v.allocation_fork.total_blocks) {
            alloc_map.resize(size_t(v.allocation_fork.logical_size), 0);
            u64 done = 0;
            for (const auto& e : v.allocation_fork.extents) {
                if (!e.block_count)
                    continue;
                const u64 off = u64(e.start_block) * v.vh.block_size;
                u64 len = u64(e.block_count) * v.vh.block_size;
                if (done + len > alloc_map.size())
                    len = alloc_map.size() > done ? alloc_map.size() - done : 0;
                if (!len)
                    break;
                (void)v.dev->read_at(off, alloc_map.data() + done, u32(len));
                done += len;
            }
        }
        auto block_free = [&](u32 block) -> bool {
            if (block / 8 >= alloc_map.size())
                return false;
            return (alloc_map[block / 8] & (1u << (7 - block % 8))) == 0; /* MSB-first */
        };

        const u64 total_nodes = v.catalog_fork.logical_size / v.catalog_node_size;
        const u64 sweep_limit = total_nodes < 65536 ? total_nodes : 65536;
        for (u32 ni = 0; ni < sweep_limit; ++ni) {
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (live_chain.count(ni) || ni == 0)
                continue;
            std::vector<u8> node;
            if (!read_tree_node(v, v.catalog_fork, CNID_CATALOG_FILE, v.catalog_node_size, ni,
                                node))
                continue;
            const i16 kind = static_cast<i16>(static_cast<i8>(node[NH_KIND]));
            if (kind != NH_KIND_LEAF) /* only freed LEAF nodes hold stale records */
                continue;
            for_each_record(v.catalog_node_size, node, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                CatalogRecord cr;
                if (!parse_catalog_record(rec, len, cr))
                    return true;
                if (cr.cnid < CNID_FIRST_USER)
                    return true;
                if (by_cnid.count(cr.cnid))
                    return true; /* still live */
                ++stats.orphans;
                auto* props = new (std::nothrow) tree::FileProperties();
                if (!props)
                    return false;
                props->name = cr.name.empty() ? u"unnamed" : cr.name;
                props->fs_kind = 8;
                props->logical_size = cr.rec_type == REC_FILE ? cr.data_fork.logical_size : 0;
                props->attributes = cr.rec_type == REC_FOLDER ? 0x10 : 0;
                props->times.created = mac_to_filetime(cr.create_date);
                props->times.modified = mac_to_filetime(cr.mod_date);
                props->record_ref = cr.cnid;
                for (const auto& ex :
                     resolve_file_extents(v, cr.cnid, 0, cr.data_fork, v.total_bytes()))
                    props->extents.push_back({ex.first, ex.second});
                tree::FileNode* n = tree.add_file(root_deleted, props);
                props->release();
                if (!n)
                    return false;
                n->set_deleted_state(tree::DeletedState::Deleted);
                n->set_origin(tree::Origin::Structured);
                /* allocation-bitmap grading */
                bool all_free = !props->extents.empty();
                bool any_free = false;
                for (const auto& e : cr.data_fork.extents) {
                    if (!e.block_count)
                        continue;
                    bool fr = true;
                    for (u32 b = e.start_block;
                         b < e.start_block + e.block_count && b - e.start_block < 64; ++b)
                        if (!block_free(b))
                            fr = false;
                    if (fr)
                        any_free = true;
                    else
                        all_free = false;
                }
                if (cr.rec_type != REC_FILE || cr.data_fork.logical_size == 0)
                    n->set_confidence(tree::Confidence::MetadataOnly);
                else if (all_free)
                    n->set_confidence(tree::Confidence::Excellent);
                else if (any_free)
                    n->set_confidence(tree::Confidence::PartialRisk);
                else
                    n->set_confidence(tree::Confidence::Overwritten);
                return true;
            });
        }
    }

    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace hfsplus
} // namespace cr
