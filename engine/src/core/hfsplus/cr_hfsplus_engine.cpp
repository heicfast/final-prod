/* cr_hfsplus_engine.cpp - HFS+/HFSX scan engine.
 *
 * All on-disk integers are BIG-endian (Apple TN1150 layouts).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/hfsplus/cr_hfsplus_engine.h"
#include "cr/cr_bits.h"            /* shared byte readers (W4 dedup, DC-12) */
#include "cr/compress/cr_decomp.h" /* S-002: kZlib for decmpfs extents */
#include <cstring>
#include <functional>
#include <cstdio>
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
constexpr u32 VH_ATTRIBUTES_FORK = 0x160; /* S-002: attributes B-tree fork */

constexpr u16 SIG_HFSPLUS = 0x482B; /* "H+" */
constexpr u16 SIG_HFSX = 0x4858;    /* "HX" */

/* CNIDs */
constexpr u32 CNID_ROOT_PARENT = 1;
constexpr u32 CNID_ROOT_FOLDER = 2;
constexpr u32 CNID_EXTENTS_FILE = 3;
constexpr u32 CNID_CATALOG_FILE = 4;
[[maybe_unused]] constexpr u32 CNID_ALLOCATION_FILE = 6;
constexpr u32 CNID_ATTRIBUTES_FILE = 8;
constexpr u32 CNID_FIRST_USER = 16;

/* fork data (80 bytes) */
constexpr u32 FORK_LOGICAL_SIZE = 0x00;
constexpr u32 FORK_TOTAL_BLOCKS = 0x0C;
constexpr u32 FORK_EXTENTS = 0x10; /* 8 x (start u32, count u32) */

/* B-tree node header (14 bytes) — TN1150 BTNodeDescriptor; og gate
 * FUN_180010720 accepts kind in {-1,0,1,2} (kind+1 <= 3 unsigned) */
constexpr u32 NH_FORWARD = 0x00;
[[maybe_unused]] constexpr u32 NH_BACKWARD = 0x04;
constexpr i16 NH_KIND_LEAF = -1;                   /* kBTLeafNode */
[[maybe_unused]] constexpr i16 NH_KIND_INDEX = 0;  /* kBTIndexNode */
[[maybe_unused]] constexpr i16 NH_KIND_HEADER = 1; /* kBTHeaderNode */
[[maybe_unused]] constexpr i16 NH_KIND_MAP = 2;    /* kBTMapNode */
constexpr u32 NH_KIND = 0x08;                      /* SInt8 node type */
[[maybe_unused]] constexpr u32 NH_HEIGHT = 0x09;
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
[[maybe_unused]] constexpr i16 REC_FOLDER_THREAD = 3;
[[maybe_unused]] constexpr i16 REC_FILE_THREAD = 4;

/* HFSPlusCatalogFile field offsets (from record body start) */
[[maybe_unused]] constexpr u32 CF_RECORD_TYPE = 0x00;
constexpr u32 CF_FLAGS = 0x02;
constexpr u32 CF_FILE_ID = 0x08;
constexpr u32 CF_CREATE_DATE = 0x0C;
constexpr u32 CF_CONTENT_MOD = 0x10;
constexpr u32 CF_ATTR_MOD = 0x14;
constexpr u32 CF_ACCESS_DATE = 0x18;
constexpr u32 CF_DATA_FORK = 0x58;
constexpr u32 CF_RESOURCE_FORK = 0xA8;

/* folder record fields */
[[maybe_unused]] constexpr u32 CD_VALENCE = 0x04;
constexpr u32 CD_FOLDER_ID = 0x08;
constexpr u32 CD_CREATE = 0x0C;
constexpr u32 CD_CONTENT_MOD = 0x10;

constexpr u32 MAX_TREE_NODES = 1u << 22; /* 4M nodes bound */
constexpr u32 MAX_RECORD_NAME = 255;
[[maybe_unused]] constexpr u32 DIR_DEPTH_LIMIT = 64;

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
    ForkData attributes_fork; /* S-002: may be empty (pre-10.4 volumes) */

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
    /* N-038/N-028c (diff-to-fix): the og's fork-selection predicate reads
     * the record's ownerFlags byte (+0x29, permissions block) - bit 0x20
     * selects the resource fork and marks the node for the decmpfs
     * completion path (FUN_180009940 :7665/:7677-78); the FinderInfo
     * fileType (+0x30) 'prdf' folderizes the record (:7687-89). */
    u8 owner_flags = 0;
    u32 finder_file_type = 0;
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
        out.owner_flags = body[0x29];             /* N-038: fork-selection bit */
        out.finder_file_type = be32(body + 0x30); /* N-028c: 'prdf' gate */
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

/* Walk every record of one node through the record-offset array.
 * A25/A-005 (diff-to-fix): the on-disk layout (TN1150, and the og's own
 * journal-image validator FUN_180010720 :13094-13108) packs records UP
 * from offset 14: record 0 always starts at 14, deeper array slots hold
 * HIGHER values, and the deepest slot (node_size - 2*(nrec+1)) is the
 * free-space start. Record i = [slot(i+1)'s value, slot(i+2)'s value);
 * the pre-fix reader took the END from the SHALLOWER slot (records-DOWN)
 * - self-consistent with the synthetic fixtures but inverted vs the
 * format, silently skipping every record except record 0 on real
 * volumes. */
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
    const u8* base = node.data();
    for (u32 i = 0; i < nrec; ++i) {
        /* record i starts at slot(i+1) and ends at slot(i+2) - the next
         * record's start; the last record ends at the free-space start
         * (slot(nrec+1)) */
        const u32 off_this = be16(base + node_size - (i + 1) * 2);
        const u32 off_next = be16(base + node_size - (i + 2) * 2);
        if (off_this < 14 || off_this >= node_size || off_next > node_size || off_next <= off_this)
            continue; /* damaged entry: skip */
        if (!fn(base + off_this, off_next - off_this))
            return false;
    }
    return true;
}

/* ---- S-002: attributes B-tree (CNID 8) ----------------------------------
 *
 * Attribute key layout (og-verified, HFSPlusSearchDLL FUN_180009c10
 * :7771-7791: keyLength = (nameLen+7)*2, fileId@+4, nameLen u16@+0xC,
 * name UTF16BE units@+0xE - the keyLength COUNTS ITSELF for attribute
 * keys, unlike the catalog convention):
 *   [keyLength u16][pad u16][fileId u32][startBlock u32]
 *   [nameLen u16][UTF16BE name[nameLen]]
 * Record data (TN1150/kernel hfsplus_attr_inline_data; the og gates on
 * recordType == 0x10 - FUN_180009c10 :7825 - and reads size@+12 with the
 * payload at +16, exactly this layout):
 *   [recordType u32 BE = 0x10 inline][reserved x2][length u32 BE][bytes[]]
 *
 * com.apple.decmpfs value (opaque xattr payload, LITTLE-endian x86
 * structs - the og reads the u64 at value+8 in one native load,
 * FUN_180009c10 :7841):
 *   [magic u32 LE = 0x66706D63 'cmpf'][mode u32 LE = 3|4]
 *   [uncompressedSize u32 LE][mode 3: zlib stream@+12 .. value end |
 *    mode 4: resourceForkOffset u32 LE @+12]
 */

constexpr u32 ATTR_KEY_FILE_ID = 0x04;
constexpr u32 ATTR_KEY_START_BLOCK = 0x08; /* key startBlock u32 (S-021 attr differential) */
constexpr u32 ATTR_KEY_NAME_LEN = 0x0C;
constexpr u32 ATTR_KEY_NAME = 0x0E;
constexpr u32 ATTR_DATA_HEADER = 16;   /* recordType+r1+r2+length */
constexpr u32 ATTR_INLINE_DATA = 0x10; /* og gate value (:7825) */
constexpr u32 DECMPFS_MAGIC_LE = 0x66706D63;

struct AttrEntry {
    std::u16string name; /* UTF-16 (decoded from the BE key) */
    u64 value_off = 0;   /* absolute slice offset of the value bytes */
    u32 value_len = 0;   /* value byte length (from the record header) */
    u32 start_block = 0; /* key startBlock (S-021 live-attr differential) */
    bool decmpfs = false;
    u32 decmpfs_mode = 0;
    u32 uncompressed_size = 0;
    u32 rsrc_offset = 0; /* mode 4: offset into the resource fork */
};

/* Walk the attributes B-tree leaf chain once, collecting every inline
 * attribute with its on-disk value position and decmpfs facts. */
bool walk_attributes_tree(Volume& v, CancelToken* cancel,
                          std::map<u32, std::vector<AttrEntry>>& out, u64& unsupported) {
    if (v.attributes_fork.extents[0].block_count == 0)
        return true; /* no attributes file on this volume: nothing to do */
    std::vector<u8> node;
    if (!read_node0_bootstrap(v, v.attributes_fork, node))
        return false;
    const u32 node_size = be16(node.data() + 14 + BTH_NODE_SIZE);
    if (node_size < 512 || node_size > 65536 || (node_size & (node_size - 1)) != 0)
        return false;
    const u32 first_leaf = be32(node.data() + 14 + BTH_FIRST_LEAF);
    const u32 last_leaf = be32(node.data() + 14 + BTH_LAST_LEAF);

    u32 cur = first_leaf;
    u64 guard = 0;
    while (cur != 0 && guard++ < MAX_TREE_NODES) {
        if (cancel && cancel->checkpoint())
            return false;
        /* absolute disk offset of this node (extent-mapped like every
         * other tree read - read_tree_node recomputes it; recompute here
         * for the value-offset math) */
        const u64 logical = u64(cur) * node_size;
        u64 node_abs = 0;
        bool have_abs = false;
        {
            u64 cursor = 0;
            std::vector<ForkData::Extent> ext(v.attributes_fork.extents,
                                              v.attributes_fork.extents + 8);
            auto it = v.overflow.find(CNID_ATTRIBUTES_FILE);
            if (it != v.overflow.end()) {
                auto fit = it->second.find(0);
                if (fit != it->second.end())
                    for (const auto& e : fit->second)
                        ext.push_back(e);
            }
            for (const auto& e : ext) {
                const u64 extent_bytes = u64(e.block_count) * v.vh.block_size;
                if (extent_bytes == 0)
                    continue;
                if (logical >= cursor && logical + node_size <= cursor + extent_bytes) {
                    node_abs = u64(e.start_block) * v.vh.block_size + (logical - cursor);
                    have_abs = true;
                    break;
                }
                cursor += extent_bytes;
            }
        }
        std::vector<u8> leaf;
        if (!have_abs ||
            !read_tree_node(v, v.attributes_fork, CNID_ATTRIBUTES_FILE, node_size, cur, leaf))
            break;
        for_each_record(node_size, leaf, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
            if (len < 16)
                return true;
            const u32 key_len = be16(rec);
            if (key_len < 16 || u64(key_len) + ATTR_DATA_HEADER > len)
                return true; /* truncated/bogus key */
            const u32 file_id = be32(rec + ATTR_KEY_FILE_ID);
            const u32 name_len = be16(rec + ATTR_KEY_NAME_LEN);
            if (key_len < 14 + 2 * u64(name_len) || name_len > MAX_RECORD_NAME)
                return true;
            AttrEntry a;
            for (u32 i = 0; i < name_len; ++i)
                a.name.push_back(be16(rec + ATTR_KEY_NAME + u32(i) * 2));
            const u8* data = rec + key_len; /* keyLength counts itself */
            if (u64(key_len) + ATTR_DATA_HEADER > len)
                return true;
            const u32 record_type = be32(data);
            if (record_type != ATTR_INLINE_DATA) {
                ++unsupported; /* og parity: "unsupported attribute node
                                  format" is logged and the record skipped */
                return true;
            }
            a.value_len = be32(data + 12);
            a.start_block = be32(rec + ATTR_KEY_START_BLOCK); /* S-021 key */
            if (a.value_len == 0 || u64(key_len) + ATTR_DATA_HEADER + a.value_len > len)
                return true; /* value must live inside the record */
            a.value_off = node_abs + u64(rec - leaf.data()) + key_len + ATTR_DATA_HEADER;
            /* decmpfs facts (value fields are LITTLE-endian) */
            if (a.value_len >= 12 && a.name == u"com.apple.decmpfs" &&
                le32(data + ATTR_DATA_HEADER) == DECMPFS_MAGIC_LE) {
                a.decmpfs = true;
                a.decmpfs_mode = le32(data + ATTR_DATA_HEADER + 4);
                a.uncompressed_size = le32(data + ATTR_DATA_HEADER + 8);
                if (a.decmpfs_mode == 4 && a.value_len >= 16)
                    a.rsrc_offset = le32(data + ATTR_DATA_HEADER + 12);
            }
            if (out.size() < (1u << 20))
                out[file_id].push_back(a);
            return true;
        });
        if (cur == last_leaf)
            break;
        const u32 next = be32(leaf.data() + NH_FORWARD);
        if (next == cur)
            break;
        cur = next;
    }
    return true;
}

/* Map a logical offset inside a file fork to its absolute slice offset. */
u64 fork_offset_to_absolute(Volume& v, u32 cnid, u8 fork_type, const ForkData& fork, u64 logical,
                            bool& ok) {
    ok = false;
    std::vector<ForkData::Extent> all(fork.extents, fork.extents + 8);
    auto it = v.overflow.find(cnid);
    if (it != v.overflow.end()) {
        auto fit = it->second.find(fork_type);
        if (fit != it->second.end())
            for (const auto& e : fit->second)
                all.push_back(e);
    }
    u64 cursor = 0;
    for (const auto& e : all) {
        const u64 extent_bytes = u64(e.block_count) * v.vh.block_size;
        if (extent_bytes == 0)
            continue;
        if (logical < cursor + extent_bytes) {
            ok = true;
            return u64(e.start_block) * v.vh.block_size + (logical - cursor);
        }
        cursor += extent_bytes;
    }
    return 0;
}

/* ---- S-021: journal deleted-record harvest --------------------------------
 *
 * The og's PRIMARY deleted-file surface for HFS+ (HFSPlusSearchDLL
 * CHFSPlusJournal::GetDeleteRecord :13637-14252 + the FUN_18000c8f0 emit
 * driver :10017-10127; the vendor path string "hfsplusjournal.cpp" appears
 * at :13417/:13794/:14235). The journal is a ring of transaction
 * block-lists; each entry carries a B-tree node image and its owning
 * volume block. Leaf images of the catalog tree hold catalog records that
 * may no longer be in the live tree; the og keeps a harvested record only
 * when the live catalog does not have the same key, or has it with a
 * different fileID (:13958-62). Extents-overflow images feed a {cnid ->
 * extents} side map for harvested files whose own fork is empty.
 *
 * On-disk shape (all decoded first-hand against the decompile this
 * session; see the S-021 Resolution + the evidence addendum for the
 * derivations and the open constant question):
 *   JournalInfoBlock (180 bytes, BE, at block vh.journal_info_block):
 *     +0x00 u32 flags (bit 0 required set - the og's gate), +0x24 u64
 *     journal byte offset, +0x2C u64 journal size.
 *   journal header (44 bytes, LE - jnl.c stores its structures in host
 *     order; magic 0x4A4E4C78 'JNLx' + endian 0x12345678):
 *     +0x08 u64 start (oldest transaction), +0x10 u64 ring walk origin,
 *     +0x18 u64 journal size (cross-checked), +0x20 u32 block-list
 *     extent (probe re-read size + ring advance + the header-test base),
 *     +0x24 u32 jhdr size, +0x28 u32 block size (probe size + the
 *     target-block multiplier).
 *   Transaction block list (the block-list extent bytes at the ring
 *     cursor): first u32 == (blhdr_size >> 4) - 1 is the header test;
 *     u16 entry count at +2; entries at +0x20, 16 bytes each:
 *       +0x00 u64 target volume block, +0x08 u32 image size, +0x0C next.
 *     The images follow the block list serially in the ring.
 */

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
    /* N-005 (diff-to-fix): the og's gate (FUN_180012c00-class, HFSPlus-
     * SearchDLL :10005-10010) has NO upper bound on blockSize beyond the
     * 512-multiple mask - large-allocation-block volumes (legal on very
     * large disks formatted with big blocks) open on the og. Upper bound
     * relaxed 64 KiB -> 16 MiB; the total-volume product sanity runs at
     * the caller (device size known there). The >= 512 floor stays
     * (blockSize 0 is geometrically unusable - registered hardening). */
    if ((block_size & 0x1FF) != 0 || block_size < 512 || block_size > (1u << 24))
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
    out.attributes = be32(vh + 4);
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

/* ---- S-021: HFS+ journal deleted-record harvest -------------------------
 *
 * The og's PRIMARY deleted-file surface on journaled volumes
 * (CHFSPlusJournal::GetDeleteRecord, HFSPlusSearchDLL FUN_180011300
 * :13637-14252; emit driver FUN_18000c8f0 :10017-10127 - both first-hand
 * re-verified this session at corpus 03ad46c). The live catalog carries no
 * deleted records (HFS+ delete overwrites in place); recent deletions and
 * pre-crash recent state remain visible ONLY as transaction block images
 * in the journal ring. The og harvests them with a differential against
 * the live catalog; the live walk itself runs unchanged afterwards
 * (:10122) - there is NO live-tree overlay/replay (corrects S-003's
 * original framing; the images install into a scratch record, FUN_180010110,
 * never the volume trees).
 *
 * On-disk chain (Apple jnl.c layout, LE structures inside the journal):
 *   VH+0x0C journalInfoBlock -> JournalInfoBlock (TN1150, BE fields:
 *   flags u32@0 [bit 0 = the og's gate], magic 0x6A6E6C78 u32@4, journal
 *   byte offset u64@+0x24, size u64@+0x2C [the og's FUN_18000db70 swaps
 *   exactly the three BE fields it reads: [0..3], [0x24..0x2B],
 *   [0x2C..0x33]; the ring reader's raw longlong arithmetic reads the
 *   base at +0x24 - the DF4-S021 convergence correction, og-decode over
 *   the earlier +8/+16 rendering]) -> jnl_header at the journal start (LITTLE-endian:
 *   magic 0x4A4E4C78 u32@0, endian 0x12345678 u32@4, end u64@8,
 *   start u64@0x10, size u64@0x18 cross-checked against the info block,
 *   :13542-13554, size_a u32@0x20, size_b u32@0x28).
 *
 * Ring walk (FUN_180011300 mechanics):
 *   - probe-read size_b bytes at the ring offset (wrap-aware reads,
 *     FUN_180011220 splits images spanning the ring end);
 *   - a block whose FIRST U16 == (size_a >> 4) - 1 is a transaction
 *     block-list: re-read size_a bytes; entry count = u16@+2; entries at
 *     +0x20 with 16-byte stride; the og loop processes count-1 entries
 *     (:13751-14188). DERIVATION OF THE MAGIC CONSTANT IS UNVERIFIED
 *     (corpus OQ - a wrong constant yields a silent zero-harvest, never
 *     wrong results; the test fixtures encode the same formula);
 *   - entry: {u64 target_block LE@+0, u32 image_size LE@+8}; the image is
 *     read at the ring cursor (advance by image_size, wrap % size);
 *   - node gate (FUN_180010720): size is a 512 multiple, kind byte @+8 in
 *     {-1..2}, leaf height == 1 / index height == 0, record-offset array
 *     monotonic (the last check is subsumed by for_each_record's bounds);
 *   - leaf images only; the target address (size_b * entry.target_block)
 *     is classified by fork ranges in og order catalog -> extents ->
 *     attributes -> allocation -> startup (:13809-17);
 *   - catalog branch: records with cnid == key.parentCNID are skipped;
 *     the differential keeps a record only if the live catalog does not
 *     hold the same (parent, name) key or the live record's fileID
 *     differs (:13939-14130). Extents branch: overflow-format records
 *     feed a {cnid -> extents} fallback map.
 *
 * Safety hardening over the og: the ring walk is bounded by the journal
 * size (a corrupt ring can cycle without ever re-hitting jhdr.start - the
 * og's own loop has no such cap; ours terminates after one full ring of
 * advances). Image sizes are sanity-bounded; every read is bounds-checked
 * by the device layer. */

static u32 le32j(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
static u64 le64j(const u8* p) {
    return u64(le32j(p)) | (u64(le32j(p + 4)) << 32);
}

/* Wrap-aware ring read: fills buf from ring [off, off+len) with the ring
 * end split back to offset 0 (og FUN_180011220 semantics). Advances off. */
static bool journal_ring_read(IStorageDevice* dev, u64 base, u64 jsize, u64& off, u8* buf,
                              u32 len) {
    if (jsize == 0 || len == 0 || len > jsize)
        return false;
    u32 done = 0;
    while (done < len) {
        u64 contiguous = jsize - off;
        u32 take = u32(std::min<u64>(contiguous, u64(len) - done));
        if (take == 0) {
            off = 0;
            continue;
        }
        if (dev->read_at(base + off, buf + done, take) != OK)
            return false;
        done += take;
        off += take;
        if (off >= jsize)
            off = 0;
    }
    return true;
}

/* B-tree node image gate (og FUN_180010720 subset: 512-multiple size,
 * kind in {-1..2}, leaf height 1 / index height 0 - the record-offset
 * monotonicity is enforced by for_each_record's bounds checks). */
static bool journal_node_image_ok(const u8* img, u32 size) {
    if (size < 0x200 || (size & 0x1ff) != 0 || size > (16u << 20))
        return false;
    const int kind = int(i8(img[8]));
    if (kind < -1 || kind > 2)
        return false;
    const u8 height = img[9];
    if (kind == -1 && height != 1)
        return false; /* kBTLeafNode */
    if (kind == 1 && height != 0)
        return false; /* kBTIndexNode (og gate) */
    /* the og's full validator (FUN_180010720 :13094-13108, converged from
     * the DF4 A-005 decode): the zero/record header must be nonzero, the
     * free-space sentinel (BE-u16 at node end-2) == 14 (TN1150: record 0
     * always at 14 - the record-span convention A-005 fixed), and the
     * offset chain ASCENDS toward deeper slots (a mid-chain violation
     * stops the walk but does not reject - the og is lenient there). */
    if (be32(img + 8) == 0)
        return false; /* zero/garbage region */
    const u32 n = be16(img + 0xA);
    if (n == 0 || n > (size - 14) / 2)
        return false;
    if (be16(img + size - 2) != 14)
        return false;
    u32 prev = 14;
    for (u32 i = 2; i <= n + 1; ++i) {
        const u64 slot = u64(size) - u64(i) * 2;
        const u32 off = be16(img + slot);
        if (slot < off || off <= prev)
            return false; /* asm-exact: 0x18001080c/:812 jump past the
                           * success flag at :825 - the og REJECTS */
        prev = off;
    }
    return true;
}

/* One harvested record pending emission (catalog branch output). */
struct HarvestedRec {
    CatalogRecord rec;
    std::vector<ForkData::Extent> ext_fallback; /* extents-branch images */
};

/* The og's volume-open salvage ladder (:1715-1760 + installer
 * FUN_180003ea0 :2344-2560, decoded first-hand this session): when the
 * jnl_header at jbase is invalid but the block there is itself
 * node-shaped (BTNodeDescriptor with reserved == 0 and kind leaf/header
 * + the height gates), scan 32 KiB at jbase for the first size on the
 * ladder 1x,2x,4x... x blockSize (<= 0x8000) whose image passes the node
 * gate, then salvage that node's records, shape-classified per
 * TestNodeBelongToFile (:12903-13100: catalog folder 0x58/type 1, file
 * 0xF8/type 2; extents keyLength 10; attribute keys) into the same
 * harvest maps - FUN_180003ea0 loads a SCRATCH node object and harvests;
 * it never installs into the live tree readers. Recovery-grade: a
 * damaged journal header must not hide the node images after it. */
static void journal_fallback_salvage(
    Volume& v, u64 jbase, const std::set<std::tuple<u32, u32, std::u16string>>& live_attr_keys,
    std::map<std::tuple<u32, std::u16string, u32>, CatalogRecord>& kept,
    std::map<u32, std::vector<ForkData::Extent>>& ext_out,
    std::map<u32, std::vector<AttrEntry>>& journal_attrs, HfsScanStats& stats) {
    /* the node-shaped trigger on the first block at jbase */
    u8 trig[16] = {0};
    if (v.dev->read_at(jbase, trig, sizeof(trig)) != OK)
        return;
    const int kind = int(i8(trig[8]));
    if (be16(trig + 12) != 0) /* BTNodeDescriptor.reserved must be zero */
        return;
    if (kind == -1) {
        if (trig[9] != 1) /* leaf height 1 */
            return;
    } else if (kind == 1) {
        if (trig[9] != 0) /* header height 0 */
            return;
    } else {
        return; /* the og's trigger accepts leaf/header shapes only */
    }
    /* 32 KiB scan window at jbase, short reads zero-filled (deterministic;
     * the og scans whatever its device returned) */
    std::vector<u8> win(size_t(0x8000), 0);
    const u64 dev_size = v.dev->size();
    const u64 avail = dev_size > jbase ? dev_size - jbase : 0;
    const u64 want = std::min<u64>(0x8000, avail);
    if (want == 0 || v.dev->read_at(jbase, win.data(), u32(want)) != OK)
        return;
    for (u64 sz = u64(v.vh.block_size); sz <= 0x8000; sz <<= 1) {
        if (!journal_node_image_ok(win.data(), u32(sz)))
            continue;
        ++stats.journal_salvage;
        if (kind == 1)
            return; /* a header node carries no user records (the og
                     * remembers it as a tree-root hint, ctx+0xf0) */
        /* shape-classify by the FIRST record (TestNodeBelongToFile), then
         * run the matching parser over the whole node */
        u32 first_len = 0;
        const u8* first = nullptr;
        {
            const u32 o0 = be16(win.data() + u32(sz) - 2); /* record 0 at 14 */
            const u32 o1 = be16(win.data() + u32(sz) - 4); /* record 1 / free */
            if (o0 >= 14 && o1 > o0 && o1 <= u32(sz)) {
                first = win.data() + o0;
                first_len = o1 - o0;
            }
        }
        if (first != nullptr && first_len >= 12) {
            const u32 klen = be16(first); /* key data length (catalog/
                                           * extents convention: excludes self) */
            const u32 ktotal = 2 + klen + (klen & 1);
            const u32 body = first_len > ktotal ? first_len - ktotal : 0;
            const i16 rtype = i16(be16(first + ktotal));
            const bool cat_shape = (body == 0x58 && rtype == 1) || /* folder */
                                   (body == 0xF8 && rtype == 2);   /* file */
            const bool ext_shape = (klen == 10 && first_len >= 12 + 64);
            const bool attr_shape = !cat_shape && !ext_shape && klen >= 14 && (klen & 1) == 0 &&
                                    first_len > ktotal + 15 &&
                                    be32(first + ktotal) == ATTR_INLINE_DATA;
            if (cat_shape) {
                for_each_record(u32(sz), win, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                    CatalogRecord cr;
                    if (!parse_catalog_record(rec, len, cr))
                        return true;
                    kept[{cr.parent_cnid, cr.name, cr.cnid}] = cr;
                    return true;
                });
            } else if (ext_shape) {
                for_each_record(u32(sz), win, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                    if (len < 12 + 64)
                        return true;
                    const u32 file_id = be32(rec + 4);
                    for (int i = 0; i < 8; ++i) {
                        ForkData::Extent ex;
                        ex.start_block = be32(rec + 12 + u32(i) * 8);
                        ex.block_count = be32(rec + 12 + u32(i) * 8 + 4);
                        if (ex.block_count)
                            ext_out[file_id].push_back(ex);
                    }
                    return true;
                });
            } else if (attr_shape) {
                for_each_record(u32(sz), win, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                    if (len < 16)
                        return true;
                    const u32 key_len = be16(rec);
                    if (key_len < 16 || u64(key_len) + ATTR_DATA_HEADER > len)
                        return true;
                    const u32 file_id = be32(rec + ATTR_KEY_FILE_ID);
                    const u32 start_block = be32(rec + ATTR_KEY_START_BLOCK);
                    const u32 name_len = be16(rec + ATTR_KEY_NAME_LEN);
                    if (key_len < 14 + 2 * u64(name_len) || name_len > MAX_RECORD_NAME)
                        return true;
                    const u8* data = rec + key_len;
                    if (be32(data) != ATTR_INLINE_DATA)
                        return true;
                    const u32 value_len = be32(data + 12);
                    if (value_len == 0 || u64(key_len) + ATTR_DATA_HEADER + value_len > len)
                        return true;
                    AttrEntry a;
                    for (u32 i = 0; i < name_len; ++i)
                        a.name.push_back(be16(rec + ATTR_KEY_NAME + u32(i) * 2));
                    a.start_block = start_block;
                    a.value_len = value_len;
                    a.value_off = jbase + u64(rec - win.data()) + key_len + ATTR_DATA_HEADER;
                    if (live_attr_keys.count({file_id, start_block, a.name}))
                        return true;
                    if (a.value_len >= 12 && a.name == u"com.apple.decmpfs" &&
                        le32(data + ATTR_DATA_HEADER) == DECMPFS_MAGIC_LE) {
                        a.decmpfs = true;
                        a.decmpfs_mode = le32(data + ATTR_DATA_HEADER + 4);
                        a.uncompressed_size = le32(data + ATTR_DATA_HEADER + 8);
                        if (a.decmpfs_mode == 4 && a.value_len >= 16)
                            a.rsrc_offset = le32(data + ATTR_DATA_HEADER + 12);
                    }
                    auto& list = journal_attrs[file_id];
                    if (list.size() < (1u << 16))
                        list.push_back(std::move(a));
                    ++stats.journal_attr_records;
                    return true;
                });
            }
        }
        return; /* the og salvages the FIRST valid node only */
    }
}

/* The harvest pass. Returns OK on completion (a damaged journal degrades
 * to a no-harvest, never a scan failure - the og treats it as evidence,
 * not an error). */
static void
journal_harvest_pass(Volume& v, CancelToken* cancel,
                     const std::map<std::pair<u32, std::u16string>, u32>& live_by_key,
                     const std::set<std::tuple<u32, u32, std::u16string>>& live_attr_keys,
                     tree::ResultTree& tree, tree::FullNode* root_deleted, HfsScanStats& stats,
                     bool emit, std::map<u32, std::vector<ForkData::Extent>>& ext_out,
                     std::set<u32>& emitted_cnids) {
    /* og entry gate :13456 - kHFSVolumeJournaledBit (0x2000) in the VH
     * attributes must be set (TN1150: the journalInfoBlock field is only
     * valid when it is); plus a non-zero info block number. */
    if ((v.vh.attributes & 0x2000u) == 0 || v.vh.journal_info_block == 0)
        return;
    const u64 bs = v.vh.block_size;
    const u64 info_off = u64(v.vh.journal_info_block) * bs;
    u8 info[512];
    if (bs < 512 || v.dev->read_at(info_off, info, 512) != OK)
        return;
    /* JournalInfoBlock (TN1150 verbatim, fetched this session): BE fields
     * flags u32@0 [bit 0 kJIJournalInFSMask - the og's :13513 gate],
     * device_signature[8]@+4..+0x23 [reserved, zero on internal journals
     * - NO magic field; the og's FUN_18000db70 swaps exactly [0..3],
     * [0x24..0x2B], [0x2C..0x33] and its ctor never compares +4],
     * offset u64@+0x24, size u64@+0x2C. The earlier 'jnlx'@+4 gate was a
     * false-parity block: real internal journals carry zeros there, so
     * the harvest never ran on real volumes (fixtures encoded the same
     * fiction - both corrected in the verification pass). */
    if ((be32(info + 0) & 1u) == 0) /* the og's flags gate (:13513) */
        return;
    const u64 jbase = be64(info + 0x24);
    const u64 jsize = be64(info + 0x2C);
    if (jsize == 0 || jsize > (512u << 20)) /* hard cap: 512 MiB */
        return;
    /* SHARED harvest outputs - both the ring walk and the header-invalid
     * fallback salvage feed them; the differential + emission below runs
     * for either source (the og's volume-open salvage merges into the
     * same harvest maps its scan consumes).
     *
     * The og's harvest map is ordered by {key, fileID} (the dedup compare
     * at :13974+ matches both): same key + same fileID -> the newest
     * image wins; same key under a DIFFERENT fileID -> both entries
     * survive. There is no cnid==parentCNID skip - the audit's reading
     * mis-parsed `*(int*)(puVar25 + 4)` as the key's parent field;
     * puVar25 is the record DATA (ushort* arithmetic -> +8 bytes), so
     * both operands are the canonical fileID (S-021 convergence
     * correction, decoded from the decompile pointer types). */
    std::map<std::tuple<u32, std::u16string, u32>, CatalogRecord> kept;
    /* journal attribute records (which==3): fileID -> inline entries with
     * values living inside the journal images (absolute slice offsets) -
     * the og's decmpfs-size fallback for harvested files (FUN_180009f40
     * :7937-82 attribute consumption) */
    std::map<u32, std::vector<AttrEntry>> journal_attrs;
    bool walk_ok = false;

    /* jnl_header: 44 bytes, LITTLE-endian (jnl.c) */
    u8 jh[44];
    if (v.dev->read_at(jbase, jh, 44) == OK && le32j(jh + 0) == 0x4A4E4C78 &&
        le32j(jh + 4) == 0x12345678 &&
        le64j(jh + 0x18) == jsize) { /* og cross-check vs the info size */
        const u64 jstart = le64j(jh + 0x10);
        const u32 size_a = le32j(jh + 0x20); /* transaction block-list size */
        const u32 size_b = le32j(jh + 0x28); /* walk/classification block size */
        if (size_a != 0 && size_b != 0 && size_a <= (16u << 20) && size_b <= (16u << 20) &&
            jstart < jsize) {
            walk_ok = true;
            std::vector<u8> block(std::max<size_t>(size_a, size_b));
            std::vector<u8> image;
            u64 off = jstart;
            u64 advanced = 0;
            /* fork classification helper: is byte address inside a fork's extents? */
            auto in_fork = [&](const ForkData& fork, u32 tree_cnid, u64 addr) -> bool {
                std::vector<ForkData::Extent> ext(fork.extents, fork.extents + 8);
                auto it = v.overflow.find(tree_cnid);
                if (it != v.overflow.end()) {
                    auto fit = it->second.find(0);
                    if (fit != it->second.end())
                        for (const auto& e : fit->second)
                            ext.push_back(e);
                }
                for (const auto& e : ext)
                    if (e.block_count != 0 && addr >= u64(e.start_block) * bs &&
                        addr < u64(e.start_block + e.block_count) * bs)
                        return true;
                return false;
            };

            while (advanced <= jsize) { /* one full ring of advances at most */
                if (cancel && cancel->checkpoint())
                    return;
                if (!journal_ring_read(v.dev, jbase, jsize, off, block.data(), size_b))
                    return;
                const u16 first = u16(block[0]) | (u16(block[1]) << 8);
                if (first == u16((size_a >> 4) - 1)) {
                    /* transaction block-list: re-read the full size_a bytes */
                    u64 tx_off = off < size_b ? off + jsize - size_b : off - size_b;
                    if (!journal_ring_read(v.dev, jbase, jsize, tx_off, block.data(), size_a))
                        return;
                    off = tx_off;
                    advanced += size_a;
                    ++stats.journal_tx;
                    const u32 count = u16(block[2]) | (u16(block[3]) << 8);
                    /* og loop: entries [0 .. count-2] (local_108 = 1 .. count-1) */
                    const u32 entries = count > 0 ? count - 1 : 0;
                    const u8* ep = block.data() + 0x20;
                    for (u32 ei = 0; ei < entries && 0x20 + u64(ei + 1) * 16 <= size_a; ++ei) {
                        if (cancel && cancel->checkpoint())
                            return;
                        const u8* e = ep + u64(ei) * 16;
                        const u64 target_block = le64j(e + 0);
                        const u32 image_size = le32j(e + 8);
                        if (image_size == 0 || image_size > (16u << 20))
                            continue;
                        image.assign(image_size, 0);
                        const u64 img_ring_off = off; /* ring read advances off */
                        if (!journal_ring_read(v.dev, jbase, jsize, off, image.data(), image_size))
                            continue; /* damaged image: skip, keep walking */
                        advanced += image_size;
                        ++stats.journal_images;
                        if (!journal_node_image_ok(image.data(), image_size))
                            continue;
                        if (int(i8(image[8])) != -1) /* leaves only (og :13799) */
                            continue;
                        ++stats.journal_leaves;
                        /* absolute slice offset of the image (journal attribute
                         * values live inside the image bytes) */
                        const u64 image_abs = jbase + img_ring_off;
                        /* classify the target by fork ranges, og order */
                        const u64 addr = u64(size_b) * target_block;
                        int which = 0; /* 1 catalog, 2 extents, 3 attributes, 4 allocation */
                        if (in_fork(v.catalog_fork, CNID_CATALOG_FILE, addr))
                            which = 1;
                        else if (in_fork(v.extents_fork, CNID_EXTENTS_FILE, addr))
                            which = 2;
                        else if (v.attributes_fork.extents[0].block_count != 0 &&
                                 in_fork(v.attributes_fork, CNID_ATTRIBUTES_FILE, addr))
                            which = 3;
                        else if (in_fork(v.allocation_fork, 6, addr))
                            which = 4;
                        if (which == 1) {
                            for_each_record(image_size, image, NH_KIND_LEAF,
                                            [&](const u8* rec, u32 len) {
                                                CatalogRecord cr;
                                                if (!parse_catalog_record(rec, len, cr))
                                                    return true;
                                                kept[{cr.parent_cnid, cr.name, cr.cnid}] = cr;
                                                return true;
                                            });
                        } else if (which == 2) {
                            for_each_record(
                                image_size, image, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                                    /* overflow record format (same as the
                                     * live overflow walk; A-006: 12-byte key +
                                     * 64-byte descriptor block at key+12) */
                                    if (len < 12 + 64)
                                        return true;
                                    const u32 file_id = be32(rec + 4);
                                    for (int i = 0; i < 8; ++i) {
                                        ForkData::Extent ex;
                                        ex.start_block = be32(rec + 12 + u32(i) * 8);
                                        ex.block_count = be32(rec + 12 + u32(i) * 8 + 4);
                                        if (ex.block_count)
                                            ext_out[file_id].push_back(ex);
                                    }
                                    return true;
                                });
                        }
                        /* attributes branch (which==3): the og harvests inline
                         * attribute records into volume+0x78's map (:14085-14145,
                         * not-found-only differential vs the live attributes tree)
                         * and consumes them as the decmpfs fallback for harvested
                         * files (FUN_180009f40). Allocation (which==4) / startup
                         * images have no harvest branch og-side. */
                        if (which == 3) {
                            for_each_record(
                                image_size, image, NH_KIND_LEAF, [&](const u8* rec, u32 len) {
                                    /* attribute record: keyLength (counts itself) +
                                     * pad + fileId + startBlock + nameLen + name, then
                                     * [recordType BE][r1][r2][length BE][value] */
                                    if (len < 16)
                                        return true;
                                    const u32 key_len = be16(rec);
                                    if (key_len < 16 || u64(key_len) + ATTR_DATA_HEADER > len)
                                        return true;
                                    const u32 file_id = be32(rec + ATTR_KEY_FILE_ID);
                                    const u32 start_block = be32(rec + ATTR_KEY_START_BLOCK);
                                    const u32 name_len = be16(rec + ATTR_KEY_NAME_LEN);
                                    if (key_len < 14 + 2 * u64(name_len) ||
                                        name_len > MAX_RECORD_NAME)
                                        return true;
                                    const u8* data = rec + key_len;
                                    if (be32(data) != ATTR_INLINE_DATA)
                                        return true; /* og: inline records only */
                                    const u32 value_len = be32(data + 12);
                                    if (value_len == 0 ||
                                        u64(key_len) + ATTR_DATA_HEADER + value_len > len)
                                        return true;
                                    AttrEntry a;
                                    for (u32 i = 0; i < name_len; ++i)
                                        a.name.push_back(be16(rec + ATTR_KEY_NAME + u32(i) * 2));
                                    a.start_block = start_block;
                                    a.value_len = value_len;
                                    a.value_off = image_abs + u64(rec - image.data()) + key_len +
                                                  ATTR_DATA_HEADER;
                                    /* live-tree differential: not-found-only (the og
                                     * searches the live attributes tree by key
                                     * {fileId, startBlock, name} before keeping) */
                                    if (live_attr_keys.count({file_id, start_block, a.name}))
                                        return true;
                                    /* decmpfs facts (value fields LITTLE-endian) */
                                    if (a.value_len >= 12 && a.name == u"com.apple.decmpfs" &&
                                        le32(data + ATTR_DATA_HEADER) == DECMPFS_MAGIC_LE) {
                                        a.decmpfs = true;
                                        a.decmpfs_mode = le32(data + ATTR_DATA_HEADER + 4);
                                        a.uncompressed_size = le32(data + ATTR_DATA_HEADER + 8);
                                        if (a.decmpfs_mode == 4 && a.value_len >= 16)
                                            a.rsrc_offset = le32(data + ATTR_DATA_HEADER + 12);
                                    }
                                    auto& list = journal_attrs[file_id];
                                    if (list.size() < (1u << 16))
                                        list.push_back(std::move(a));
                                    ++stats.journal_attr_records;
                                    return true;
                                });
                        }
                    }
                } else {
                    /* the probe read already advanced `off` by size_b */
                    advanced += size_b;
                }
                if (off == jstart)
                    break; /* wrapped back to the start: walk complete (og gate) */
            }
        }
    } else {
        /* the jnl_header gates failed - the og's volume-open salvage
         * ladder (:1715-1760 + FUN_180003ea0): if the block at jbase is
         * node-shaped, salvage the first validator-passing node image on
         * the size ladder into the same harvest maps. */
        journal_fallback_salvage(v, jbase, live_attr_keys, kept, ext_out, journal_attrs, stats);
    }
    (void)walk_ok;

    /* differential + emission */
    if (!emit)
        return;

    /* allocation-bitmap grading (same discipline as the freed-node sweep:
     * all-extent-blocks-free -> Excellent, some -> PartialRisk, none ->
     * Overwritten; the og's own grade-5 maps downstream - OQ-S2M-1) */
    std::vector<u8> jalloc;
    if (v.allocation_fork.total_blocks != 0) {
        jalloc.resize(size_t(v.allocation_fork.logical_size), 0);
        u64 jdone = 0;
        for (const auto& e : v.allocation_fork.extents) {
            if (!e.block_count)
                continue;
            const u64 eoff = u64(e.start_block) * v.vh.block_size;
            u64 elen = u64(e.block_count) * v.vh.block_size;
            if (jdone + elen > jalloc.size())
                elen = jalloc.size() > jdone ? jalloc.size() - jdone : 0;
            if (!elen)
                break;
            (void)v.dev->read_at(eoff, jalloc.data() + jdone, u32(elen));
            jdone += elen;
        }
    }
    auto block_free = [&](u32 blk) -> bool {
        if (blk / 8 >= jalloc.size())
            return false;
        return (jalloc[blk / 8] & (1u << (7 - blk % 8))) == 0; /* MSB-first */
    };
    for (const auto& kv : kept) {
        const CatalogRecord& cr = kv.second;
        auto lit = live_by_key.find({cr.parent_cnid, cr.name});
        if (lit != live_by_key.end() && lit->second == cr.cnid)
            continue; /* live with the same fileID: not deleted evidence */
        /* N-028a parity: the og's result sink is global (one callback) -
         * journal-harvested names pass it too */
        if (!cr.name.empty() && cr.name[0] == u'.') {
            ++stats.sink_filtered;
            continue;
        }
        if (cr.name == u"HFS+ Private Data") {
            ++stats.sink_filtered;
            continue;
        }
        ++stats.journal_records;
        emitted_cnids.insert(cr.cnid); /* the sweep dedups against these */
        auto* props = new (std::nothrow) tree::FileProperties();
        if (!props)
            return;
        const bool is_dir = cr.rec_type == REC_FOLDER;
        props->name = cr.name.empty() ? u"unnamed" : cr.name;
        props->fs_kind = u32(FsKindCode::Hfsplus);
        props->logical_size = is_dir ? 0 : cr.data_fork.logical_size;
        props->attributes = is_dir ? 0x10 : 0;
        props->times.created = mac_to_filetime(cr.create_date);
        props->times.modified = mac_to_filetime(cr.mod_date);
        props->times.changed = mac_to_filetime(cr.attr_date ? cr.attr_date : cr.mod_date);
        props->times.accessed = mac_to_filetime(cr.access_date);
        props->record_ref = cr.cnid;
        /* N-059: the record's own u16 flags + the Finder file type — the
         * og's HFS+ "+0xE mode/flags secondary" and "+0x74 type id"
         * (part2 :46-48; semantics census-level, the carriers are the
         * engine's read of the catalog record). */
        props->hfs_secondary_flags = cr.flags;
        props->hfs_type_id = cr.finder_file_type;
        for (const auto& ex : resolve_file_extents(v, cr.cnid, 0, cr.data_fork, v.total_bytes()))
            props->extents.push_back({ex.first, ex.second});
        /* og fallback: extents-branch images supply the data runs when the
         * record's own fork is empty (FUN_180009f40 :7937-82) */
        if (props->extents.empty() && !is_dir) {
            auto eit = ext_out.find(cr.cnid);
            if (eit != ext_out.end()) {
                for (const auto& e : eit->second) {
                    const u64 o = u64(e.start_block) * v.vh.block_size;
                    u64 l = u64(e.block_count) * v.vh.block_size;
                    if (o + l > v.total_bytes())
                        l = v.total_bytes() > o ? v.total_bytes() - o : 0;
                    if (l)
                        props->extents.push_back({o, l});
                }
            }
        }
        /* og fallback #2 (S-021 attributes branch): a harvested file with
         * an empty data fork whose com.apple.decmpfs attribute survives
         * only in the journal resolves its content from the journal image
         * (mode 3: zlib stream inside the attribute value; mode 4: zlib
         * stream at an offset inside the record's resource fork) - the
         * same codec-extent contract the live decmpfs path uses */
        if (props->extents.empty() && !is_dir && cr.data_fork.logical_size == 0) {
            auto ait = journal_attrs.find(cr.cnid);
            if (ait != journal_attrs.end()) {
                const AttrEntry* dec = nullptr;
                for (const auto& a : ait->second)
                    if (a.decmpfs && (a.decmpfs_mode == 3 || a.decmpfs_mode == 4) &&
                        a.uncompressed_size != 0) {
                        dec = &a;
                        break;
                    }
                if (dec != nullptr) {
                    bool ext_ok = false;
                    u64 abs = 0;
                    u64 psize = 0;
                    if (dec->decmpfs_mode == 3) {
                        if (dec->value_len > 12) {
                            abs = dec->value_off + 12;
                            psize = dec->value_len - 12;
                            ext_ok = true;
                        }
                    } else if (cr.rsrc_fork.logical_size > dec->rsrc_offset) {
                        bool map_ok = false;
                        abs = fork_offset_to_absolute(v, cr.cnid, 0xFF, cr.rsrc_fork,
                                                      dec->rsrc_offset, map_ok);
                        if (map_ok) {
                            psize = cr.rsrc_fork.logical_size - dec->rsrc_offset;
                            ext_ok = true;
                        }
                    }
                    if (ext_ok) {
                        props->extents.clear();
                        props->extents.push_back(
                            {abs, u64(dec->uncompressed_size), u32(compress::kZlib), psize});
                        props->logical_size = dec->uncompressed_size;
                        ++stats.decmpfs_files;
                    }
                }
            }
        }
        tree::FileNode* n = tree.add_file(root_deleted, props);
        props->release();
        if (!n)
            continue;
        n->set_deleted_state(tree::DeletedState::Deleted);
        n->set_origin(tree::Origin::Structured);
        if (cr.rec_type != REC_FILE || props->logical_size == 0)
            n->set_confidence(tree::Confidence::MetadataOnly);
        else {
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
            if (all_free)
                n->set_confidence(tree::Confidence::Excellent);
            else if (any_free)
                n->set_confidence(tree::Confidence::PartialRisk);
            else
                n->set_confidence(tree::Confidence::Overwritten);
        }
        ++stats.journal_emitted;
    }
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
    /* N-005: total-volume product sanity (the og has no such check; a
     * torn blockSize/totalBlocks pair would feed absurd offsets - mirror
     * of the NTFS P1-3 guard, 2x slack for truncated-tail volumes). */
    if (slice->size() != 0 && u64(v.vh.block_size) * u64(v.vh.total_blocks) > slice->size() * 2)
        return ERR_BAD_PARAMETER;
    v.dev = slice;
    v.allocation_fork = parse_fork(vh_raw + VH_ALLOCATION_FORK);
    v.extents_fork = parse_fork(vh_raw + VH_EXTENTS_FORK);
    v.catalog_fork = parse_fork(vh_raw + VH_CATALOG_FORK);
    v.attributes_fork = parse_fork(vh_raw + VH_ATTRIBUTES_FORK);

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
                    /* overflow record: key{keyLength u16 = 10 (data length;
                     * the field itself makes the key 12 bytes), forkType u8,
                     * pad u8, fileID u32, startBlock u32} then the 64-byte
                     * HFSPlusExtentRecord (8 descriptors) at key+12
                     * (A-006: TN1150-verbatim - was off by two) */
                    if (len < 12 + 64)
                        return true;
                    const u8 fork_type = rec[2];
                    const u32 file_id = be32(rec + 4);
                    ForkData::Extent ext[8];
                    for (int i = 0; i < 8; ++i) {
                        ext[i].start_block = be32(rec + 12 + u32(i) * 8);
                        ext[i].block_count = be32(rec + 12 + u32(i) * 8 + 4);
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

    /* 1.5 S-002: attributes B-tree (CNID 8) - preload every inline
     * attribute with its on-disk value position and decmpfs facts, so
     * compressed files resolve at emission and named forks surface as
     * children (the og reads this tree for com.apple.decmpfs lookups -
     * FUN_180009c10, first-hand verified). */
    std::map<u32, std::vector<AttrEntry>> attributes;
    {
        u64 unsupported = 0;
        walk_attributes_tree(v, cancel, attributes, unsupported);
        stats.attr_unsupported = unsupported;
        for (const auto& kv : attributes)
            stats.attr_records += kv.second.size();
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
        /* N-028a (diff-to-fix): the og's result sink (FUN_1800057d0
         * :3714-3718) destroys dot-prefixed names and the hardlink
         * metadata directory before emission:
         *   if ((*_Str == '.') || (len == 17 && strstr(_Str,
         *       "HFS+ Private Data"))) { destroy; return 1; }
         * (.DS_Store, .Spotlight-V100, the metadata dir itself). */
        if (!r.name.empty() && r.name[0] == u'.') {
            ++stats.sink_filtered;
            continue;
        }
        if (r.name == u"HFS+ Private Data") {
            ++stats.sink_filtered;
            continue;
        }
        auto* props = new (std::nothrow) tree::FileProperties();
        if (!props)
            return ERR_NO_MEMORY;
        bool is_dir = r.rec_type == REC_FOLDER;
        /* N-028c: the og folderizes a FILE record whose FinderInfo
         * fileType is 'prdf' (FUN_180009940 :7687-89: mark +0xce |= 2,
         * folder kind, forks not parsed). */
        if (!is_dir && r.finder_file_type == 0x70726466u /* BE "prdf" */) {
            is_dir = true;
            ++stats.prf_folderized;
        }
        /* N-038: the og's fork-selection predicate - record ownerFlags
         * (+0x29) bit 0x20 selects the RESOURCE fork as the node's
         * content and marks the node for the decmpfs completion path
         * (FUN_180009940 :7665-7684). */
        const bool rsrc_marked = !is_dir && (r.owner_flags & 0x20) != 0;
        if (rsrc_marked)
            ++stats.rsrc_marked_files;
        const ForkData& content_fork = rsrc_marked ? r.rsrc_fork : r.data_fork;
        props->name = r.name;
        props->fs_kind = u32(FsKindCode::Hfsplus); /* corpus factory code */
        props->attributes = is_dir ? 0x10 : 0;
        props->logical_size = is_dir ? 0 : content_fork.logical_size;
        /* N-155: the og's zero-logical-size fallback - a data-fork record
         * with logicalSize == 0 and totalBlocks > 0 sizes as
         * totalBlocks << 12 (FUN_180009940 :7666-7671: the size read at
         * ushort +0x2c [dataFork.logicalSize] with the fallback
         * *(uint*)(+0x32) << 0xc [dataFork.totalBlocks]). The og
         * HARDCODES 4096 - the volume's real allocation-block size is not
         * consulted (an og quirk; the observable on 4096-blocksize
         * volumes - the overwhelmingly common case - is exact, so the
         * observable is the parity target, registered in the test).
         * Data-fork path ONLY: the og's rsrc path (ownerFlags 0x20) has
         * no fallback (evidence s42-hfs-forkfield-resolution.md §2) - a
         * zero-size rsrc fork stays 0 (the decmpfs completion path
         * supplies the size for compressed files). The affected
         * population is the journal/deleted path's stale records
         * (zeroed size field, intact block count). */
        if (!is_dir && !rsrc_marked && content_fork.logical_size == 0 &&
            content_fork.total_blocks > 0)
            props->logical_size = u64(content_fork.total_blocks) << 12;
        props->times.created = mac_to_filetime(r.create_date);
        props->times.modified = mac_to_filetime(r.mod_date);
        props->times.changed = mac_to_filetime(r.attr_date ? r.attr_date : r.mod_date);
        props->times.accessed = mac_to_filetime(r.access_date);
        props->record_ref = r.cnid;
        /* N-059: the record's own u16 flags + the Finder file type — the
         * og's HFS+ "+0xE mode/flags secondary" and "+0x74 type id"
         * (part2 :46-48; semantics census-level, the carriers are the
         * engine's read of the catalog record). */
        props->hfs_secondary_flags = r.flags;
        props->hfs_type_id = r.finder_file_type;
        if (!is_dir)
            for (const auto& ex : resolve_file_extents(v, r.cnid, rsrc_marked ? 0xFF : 0,
                                                       content_fork, v.total_bytes()))
                props->extents.push_back({ex.first, ex.second});
        /* S-002: decmpfs-compressed files - the data fork is empty and the
         * content lives in com.apple.decmpfs (mode 3: zlib stream inside
         * the attribute record; mode 4: zlib stream at an offset inside
         * the resource fork). Emit a codec-tagged extent (kZlib) so the
         * recovery writer inflates at write time - the core's existing
         * compressed-extent contract (cr_safewriter decode path), no
         * memory-resident blob and no size cap below the writer's own
         * 256 MiB guard. */
        if (flags.resolve_decmpfs && !is_dir && (rsrc_marked || r.data_fork.logical_size == 0)) {
            auto ait = attributes.find(r.cnid);
            if (ait != attributes.end()) {
                const AttrEntry* dec = nullptr;
                for (const auto& a : ait->second)
                    if (a.decmpfs && (a.decmpfs_mode == 3 || a.decmpfs_mode == 4) &&
                        a.uncompressed_size != 0) {
                        dec = &a;
                        break;
                    }
                if (dec != nullptr) {
                    bool ext_ok = false;
                    u64 abs = 0;
                    u64 psize = 0;
                    if (dec->decmpfs_mode == 3) {
                        if (dec->value_len > 12) {
                            abs = dec->value_off + 12;
                            psize = dec->value_len - 12;
                            ext_ok = true;
                        }
                    } else { /* mode 4: resource-fork payload at rsrc_offset */
                        if (r.rsrc_fork.logical_size > dec->rsrc_offset) {
                            bool map_ok = false;
                            abs = fork_offset_to_absolute(v, r.cnid, 0xFF, r.rsrc_fork,
                                                          dec->rsrc_offset, map_ok);
                            if (map_ok) {
                                psize = r.rsrc_fork.logical_size - dec->rsrc_offset;
                                ext_ok = true;
                            }
                        }
                    }
                    if (ext_ok) {
                        props->extents.clear();
                        props->extents.push_back(
                            {abs, u64(dec->uncompressed_size), u32(compress::kZlib), psize});
                        props->logical_size = dec->uncompressed_size;
                        ++stats.decmpfs_files;
                    }
                }
            }
        }
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
            props->fs_kind = u32(FsKindCode::Hfsplus);
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

    /* 5.5 S-002: named attributes surface as child nodes with their raw
     * value content (the ..namedfork/<name> convention the resource-fork
     * pass established; the og's attribute tree exists for exactly this
     * named-fork surface). Values live inline in the attributes B-tree
     * record - a plain byte extent, no codec. */
    if (flags.named_forks) {
        for (size_t i = 0; i < records.size(); ++i) {
            if (!nodes[i] || records[i].rec_type != REC_FILE)
                continue;
            auto ait = attributes.find(records[i].cnid);
            if (ait == attributes.end())
                continue;
            for (const auto& a : ait->second) {
                if (a.decmpfs || a.value_len == 0)
                    continue; /* decmpfs is the file's content, not a fork */
                auto* props = new (std::nothrow) tree::FileProperties();
                if (!props)
                    break;
                props->name = u"..namedfork/";
                props->name += a.name;
                props->fs_kind = u32(FsKindCode::Hfsplus);
                props->logical_size = a.value_len;
                props->record_ref = records[i].cnid;
                props->extents.push_back({a.value_off, u64(a.value_len)});
                tree::FileNode* n = tree.add_file(nodes[i], props);
                props->release();
                if (!n)
                    break;
                ++stats.named_forks;
            }
        }
    }

    /* 6. orphan sweep: catalog nodes OUTSIDE the active leaf chain hold
     * stale records from deleted files (HFS+ delete overwrites catalog
     * records; freed nodes keep the old bytes until reused). */
    /* 5.5 S-021: the journal deleted-record harvest - the og's PRIMARY
     * deleted surface - runs BEFORE the sweep so the sweep (a documented
     * core superset, I-014) can dedup against the harvested cnids (both
     * surfaces finding the same record emit once). */
    std::set<u32> journal_harvested;
    if (flags.journal_harvest && flags.keep_deleted && root_deleted) {
        std::map<std::pair<u32, std::u16string>, u32> live_by_key;
        for (const auto& r : records)
            live_by_key[{r.parent_cnid, r.name}] = r.cnid;
        /* live attribute keys {fileId, startBlock, name} for the
         * attributes-branch not-found-only differential (S-021) */
        std::set<std::tuple<u32, u32, std::u16string>> live_attr_keys;
        for (const auto& akv : attributes)
            for (const auto& a : akv.second)
                live_attr_keys.insert({akv.first, a.start_block, a.name});
        std::map<u32, std::vector<ForkData::Extent>> harvest_ext;
        journal_harvest_pass(v, cancel, live_by_key, live_attr_keys, tree, root_deleted, stats,
                             true, harvest_ext, journal_harvested);
    }

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
                /* N-028a: the og's sink filter is global (one result
                 * callback) - orphan-harvested names pass it too. */
                if (!cr.name.empty() && cr.name[0] == u'.') {
                    ++stats.sink_filtered;
                    return true;
                }
                if (cr.name == u"HFS+ Private Data") {
                    ++stats.sink_filtered;
                    return true;
                }
                if (journal_harvested.count(cr.cnid))
                    return true; /* already emitted by the journal harvest
                                  * (S-021 - the sweep is a core superset;
                                  * both surfaces found the same record) */
                ++stats.orphans;
                /* N-028c/N-038: the record-processor gates apply to every
                 * record source on the og (FUN_180009940 is shared). */
                bool o_dir = cr.rec_type == REC_FOLDER;
                if (!o_dir && cr.finder_file_type == 0x70726466u) {
                    o_dir = true;
                    ++stats.prf_folderized;
                }
                const bool o_rsrc_marked = !o_dir && (cr.owner_flags & 0x20) != 0;
                if (o_rsrc_marked)
                    ++stats.rsrc_marked_files;
                const ForkData& o_fork = o_rsrc_marked ? cr.rsrc_fork : cr.data_fork;
                auto* props = new (std::nothrow) tree::FileProperties();
                if (!props)
                    return false;
                props->name = cr.name.empty() ? u"unnamed" : cr.name;
                props->fs_kind = u32(FsKindCode::Hfsplus);
                props->logical_size = cr.rec_type == REC_FILE && !o_dir ? o_fork.logical_size : 0;
                props->attributes = o_dir ? 0x10 : 0;
                props->times.created = mac_to_filetime(cr.create_date);
                props->times.modified = mac_to_filetime(cr.mod_date);
                props->record_ref = cr.cnid;
                /* N-059: the record's own u16 flags + the Finder file type — the
                 * og's HFS+ "+0xE mode/flags secondary" and "+0x74 type id"
                 * (part2 :46-48; semantics census-level, the carriers are the
                 * engine's read of the catalog record). */
                props->hfs_secondary_flags = cr.flags;
                props->hfs_type_id = cr.finder_file_type;
                if (cr.rec_type == REC_FILE && !o_dir)
                    for (const auto& ex : resolve_file_extents(v, cr.cnid, o_rsrc_marked ? 0xFF : 0,
                                                               o_fork, v.total_bytes()))
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
